
/*
 * ffmpeg_decoder.cpp
 *
 * History:
 *    2010/1/27 - [Oliver Li] created file
 *    2010/07/20 - [He Zhi] correct UV buffer address and stride for NV12 pix-format with enmu-edge
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "ffmpeg_decoder"
//#define AMDROID_DEBUG
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "fcntl.h"
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

#include "am_pbif.h"
#include "pbif.h"
#include "engine_guids.h"
#include "filter_list.h"

extern "C" {
#include "libavutil/avstring.h"
#include "libavutil/colorspace.h"
#include "libavformat/avformat.h"
#include "libavcodec/amba_dsp_define.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/ass_split.h"
}
#include <basetypes.h>
#include "iav_drv.h"

#include "amdsp_common.h"
#include "am_util.h"
#include "ffmpeg_decoder.h"

//#define __use_hardware_timmer__
#ifdef __use_hardware_timmer__
#include "am_hwtimer.h"
#else
#include "sys/time.h"
#endif

#if !FFMPEG_VER_0_6
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_SUBTITLE AVMEDIA_TYPE_SUBTITLE
#define guess_format av_guess_format
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif
//get from ffplay.c in ffmpegv0.7. added by cx
#define RGBA_IN(r, g, b, a, s)\
{\
    unsigned int v = ((const uint32_t *)(s))[0];\
    a = (v >> 24) & 0xff;\
    r = (v >> 16) & 0xff;\
    g = (v >> 8) & 0xff;\
    b = v & 0xff;\
}

#define YUVA_IN(y, u, v, a, s, pal)\
{\
    unsigned int val = ((const uint32_t *)(pal))[*(const uint8_t*)(s)];\
    a = (val >> 24) & 0xff;\
    y = (val >> 16) & 0xff;\
    u = (val >> 8) & 0xff;\
    v = val & 0xff;\
}

#define YUVA_OUT(d, y, u, v, a)\
{\
    ((uint32_t *)(d))[0] = (a << 24) | (y << 16) | (u << 8) | v;\
}
//Convert RGB to RGB565
#define RGB565_OUT(d, r, g, b)\
{\
    ((AM_U16*)(d))[0] = (r << 11) | (g << 5) | (b);\
}
//Convert RGB to yuv
#define RGB2Y(d, r, g, b)\
{\
    ((AM_U8*)(d))[0] = ( ( 66 * r + 129 * g + 25 * b + 128) >> 8) + 16;\
}
#define RGB2U(d, r, g, b)\
{\
    ((AM_U8*)(d))[0] = ( ( -38 * r - 74 * g + 112 * b + 128) >> 8) + 128;\
}
#define RGB2V(d, r, g, b)\
{\
    ((AM_U8*)(d))[0] = ( ( 112 * r - 94 * g - 18 * b + 128) >> 8) + 128;\
}

//end. added by cx
filter_entry g_ffmpeg_decoder = {
    "ffmpegDecoder",
    CFFMpegDecoder::Create,
    NULL,
    CFFMpegDecoder::AcceptMedia,
};

IFilter* CreateFFMpegDecoder(IEngine *pEngine)
{
    return CFFMpegDecoder::Create(pEngine);
}

//-----------------------------------------------------------------------
//
// CFFMpegDecoder
//
//-----------------------------------------------------------------------
IFilter *CFFMpegDecoder::Create(IEngine *pEngine)
{
    CFFMpegDecoder *result = new CFFMpegDecoder(pEngine);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

int CFFMpegDecoder::AcceptMedia(CMediaFormat& format)
{
    if (*format.pFormatType != GUID_Format_FFMPEG_Stream)
        return 0;

    if (*format.pMediaType == GUID_Video) {
        AM_INFO("ffmpeg: video, %d, %p\n", format.preferWorkMode, &format);
        if (PreferWorkMode_Duplex == (format.preferWorkMode)) {
            AM_ERROR("NOT support in duplex mode.\n");
            return 0;
        }
        return 1;
    }

    if (*format.pMediaType == GUID_Audio) {
        AM_INFO("ffmpeg: audio\n");
        return 1;
    }
    //add support for subtitle. added by cx
    if (*format.pMediaType == GUID_Subtitle) {
        AM_INFO("ffmpeg: subtitle\n");
        return 1;
    }
    return 0;
}

AM_ERR CFFMpegDecoder::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	DSetModuleLogConfig(LogModuleFFMpegDecoder);

	if ((mpInput = CFFMpegDecoderInput::Create(this)) == NULL)
		return ME_ERROR;

    if ((mpOutput = CFFMpegDecoderOutput::Create(this)) == NULL)
        return ME_ERROR;
    return ME_OK;
}

CFFMpegDecoder::~CFFMpegDecoder()
{
    AMLOG_DESTRUCTOR("~CFFMpegDecoder.\n");
    AM_DELETE(mpInput);
    AM_DELETE(mpOutput);
    if (mbPoolAlloc) {
        AM_RELEASE(mpBufferPool);
    }
    if (mpAccelerator) {
        AM_ASSERT(mpDSPHandler);
        mpDSPHandler->DeleteHybirdUDEC(mpAccelerator);
        mpAccelerator = NULL;
        AM_ASSERT(mpDSPHandler->mIavFd >= 0);
    }
    if (mpDSPHandler) {
        AM_ASSERT(mpDSPHandler->mIavFd >= 0);
        delete mpDSPHandler;
        mpDSPHandler = NULL;
    }
    AMLOG_DESTRUCTOR("~CFFMpegDecoder done.\n");
}


//AM_U64 audio_last_pts = 0;

//buffer may have extended edge, and the iav and ffmpeg's buffer may have different dimension
//convert yuv420p/yuv422 to nv12 format
void CFFMpegDecoder:: ConvertFrame(CVideoBuffer *pBuffer)
{
    if (mpCodec->pix_fmt == PIX_FMT_YUV420P || mpCodec->pix_fmt == PIX_FMT_YUVJ420P
        || mpCodec->pix_fmt == PIX_FMT_YUV422P || mpCodec->pix_fmt == PIX_FMT_YUVJ422P
        || mpCodec->pix_fmt == PIX_FMT_YUV410P) {

        AM_ASSERT(mFrame.data[0]);
        AM_ASSERT(mFrame.data[1]);
        AM_ASSERT(mFrame.data[2]);
        AM_ASSERT(mFrame.linesize[0]);
        AM_ASSERT(mFrame.linesize[1]);
        AM_ASSERT(mFrame.linesize[2]);
    }
    if(mpCodec->pix_fmt == PIX_FMT_BGRA) {
        AM_ASSERT(mFrame.data[0]);
        AM_ASSERT(mFrame.linesize[0]);
    }
    //AMLOG_VERBOSE("mFrame.linesize[0] %d, mFrame.linesize[1] %d, mFrame.linesize[2] %d.\n", mFrame.linesize[0], mFrame.linesize[1], mFrame.linesize[2]);
    //AMLOG_VERBOSE("pBuffer->fbWidth %d, pBuffer->picHeight %d, pBuffer->picWidth %d.\n", pBuffer->fbWidth, pBuffer->picHeight, pBuffer->picWidth);

    AM_UINT i = 0, j = 0, cnt;
    AM_U32  u, v;

    AM_U8* pdes = pBuffer->pLumaAddr + pBuffer->picYoff * pBuffer->fbWidth + pBuffer->picXoff;
    AM_U32* p = (AM_U32*)(pBuffer->pChromaAddr + (pBuffer->picYoff >> 1) * pBuffer->fbWidth + pBuffer->picXoff);


    //modified by liujian
    //update buffer picWidth and picHeight before pix_fmt converting .
    //     otherwise, memory access voilation may occur.
    //use mpCodec->width/height instead of coded_width/codec_height
    //    mpCodec->width/height means picture width/height
    //AMLOG_PRINTF("ffmpeg::ConvertFrame() --- picture w:h[%d:%d]\n",pBuffer->picWidth,pBuffer->picHeight);
    pBuffer->picWidth = mpCodec->width;
    pBuffer->picHeight = mpCodec->height;

    if (mpCodec->pix_fmt == PIX_FMT_YUV420P || mpCodec->pix_fmt == PIX_FMT_YUVJ420P) {
        AM_U16* pu = (AM_U16*)mFrame.data[1];
        AM_U16* pv = (AM_U16*)mFrame.data[2];
        //AMLOG_VERBOSE("PIX_FMT_YUV420P.\n");
        if (pBuffer->fbWidth == ((AM_UINT)mFrame.linesize[0])) {
            // copy the Y plane
            ::memcpy(pdes, mFrame.data[0], pBuffer->fbWidth * pBuffer->picHeight);

            // re-arrange U's and V's
            cnt = (pBuffer->fbWidth * pBuffer->picHeight) >>3;
            do {
                u = *pu++;
                v = *pv++;
                *p++ = ((v & 0xff) << 8) | ((v & 0xff00) << 16) | (u & 0xff) | ((u & 0xff00) << 8);
            } while (--cnt);

        } else {

          //modified by liujian to fix bug 1069
#if  0
            AM_U8* psrc = mFrame.data[0];

            // copy the Y plane
            for (i = pBuffer->picHeight; i ; i--, pdes += pBuffer->fbWidth, psrc += mFrame.linesize[0])
                ::memcpy(pdes, psrc , pBuffer->picWidth);

            // re-arrange U's and V's
            AM_U32 gap=(pBuffer->fbWidth-(mFrame.linesize[1]<<1))>>2;

            for (i=pBuffer->picHeight >> 1; i ; i--, p += gap) {
                cnt = mFrame.linesize[1]>>1;
                do {
                    u = *pu++;
                    v = *pv++;
                    *p++ = ((v & 0xff) << 8) | ((v & 0xff00) << 16) | (u & 0xff) | ((u & 0xff00) << 8);
                }
                while (--cnt);
            }

#else
           //AMLOG_PRINTF("ffmpeg::ConvertFrame() --- AVContext  w:h[%d:%d], coded w:h[%d:%d]\n",mpCodec->width,mpCodec->height,mpCodec->coded_width,mpCodec->coded_height);
	    //AMLOG_PRINTF("ffmpeg::ConvertFrame() --- linesize [%d:%d:%d]\n",mFrame.linesize[0],mFrame.linesize[1],mFrame.linesize[2]);

            // copy the Y plane
            for(i = 0; i < pBuffer->picHeight; i ++){
                AM_U8 *src = (AM_U8*)(((AM_U8*)mFrame.data[0]) +  i * mFrame.linesize[0]);
                AM_U8 *dst = (AM_U8*)(((AM_U8*)&pdes[0]) + i * pBuffer->fbWidth);
                ::memcpy(dst, src ,  pBuffer->picWidth);
            }
            // copy u and v
            for (i = 0; i < pBuffer->picHeight/2; i ++) {
                AM_U8* pu_ = (AM_U8*)(((AM_U8*)mFrame.data[1]) + i * mFrame.linesize[1]);
                AM_U8* pv_ = (AM_U8*)(((AM_U8*)mFrame.data[2]) + i * mFrame.linesize[2]);
                AM_U8* dst =  (AM_U8*)(((AM_U8*)&p[0]) + i * pBuffer->fbWidth);
                for(AM_UINT  j = 0; j <  pBuffer->picWidth/2; j ++){
                    *dst++ = *pu_++;
                    *dst++ = *pv_++;
                }
            }
#endif
        }
    }else if (mpCodec->pix_fmt == PIX_FMT_YUV422P || mpCodec->pix_fmt == PIX_FMT_YUVJ422P) {
        //AMLOG_VERBOSE("PIX_FMT_YUV422P.\n");
        AM_U16* pu = (AM_U16*)mFrame.data[1];
        AM_U16* pv = (AM_U16*)mFrame.data[2];

        // copy the Y plane
        if (pBuffer->fbWidth == ((AM_UINT)mFrame.linesize[0])) {
            ::memcpy(pdes, mFrame.data[0], pBuffer->fbWidth * pBuffer->picHeight);
        } else {
            AM_U8* psrc = mFrame.data[0];
            for (i = pBuffer->picHeight; i ; i--, pdes += pBuffer->fbWidth, psrc += mFrame.linesize[0])
                ::memcpy(pdes, psrc , pBuffer->picWidth);
        }

        AM_ASSERT(mFrame.linesize[1] == mFrame.linesize[2]);
        // re-arrange U's and V's
        for (i = 0; i< (pBuffer->picHeight>>1); i++) {
            for(cnt = 0; cnt < (pBuffer->picWidth>>2); cnt++) {
                u = pu[cnt];
                v = pv[cnt];
                p[cnt] = ((v & 0xff) << 8) | ((v & 0xff00) << 16) | (u & 0xff) | ((u & 0xff00) << 8);
            }
            //skip a line
            pu += mFrame.linesize[1];
            pv += mFrame.linesize[2];
            p += pBuffer->fbWidth>>2;
        }

    }else if(mpCodec->pix_fmt == PIX_FMT_YUV410P) {
        //AMLOG_INFO("PIX_FMT_YUV410P.\n");
//        AM_U8* pu_u8 = mFrame.data[1];
//        AM_U8* pv_u8 = mFrame.data[2];
        // copy the Y plane
        if (pBuffer->fbWidth == ((AM_UINT)mFrame.linesize[0])) {
            ::memcpy(pdes, mFrame.data[0], pBuffer->fbWidth * pBuffer->picHeight);
        } else {
            AM_U8* psrc = mFrame.data[0];
            for (i = pBuffer->picHeight; i ; i--, pdes += pBuffer->fbWidth, psrc += mFrame.linesize[0])
                ::memcpy(pdes, psrc , pBuffer->picWidth);
        }
        AM_ASSERT(mFrame.linesize[1] == mFrame.linesize[2]);
        // re-arrange U's and V's
        AM_U8 pdu[(pBuffer->picHeight >> 1) * (pBuffer->fbWidth >> 1)];
        AM_U8 pdv[(pBuffer->picHeight >> 1) * (pBuffer->fbWidth >> 1)];
        AM_U16* p_u = (AM_U16*)pdu;
        AM_U16* p_v = (AM_U16*)pdv;
        planar2x(mFrame.data[1], pdu, mFrame.linesize[1], pBuffer->fbWidth >> 1, pBuffer->picWidth >> 2, pBuffer->picHeight >> 2);
        planar2x(mFrame.data[2], pdv, mFrame.linesize[2], pBuffer->fbWidth >> 1, pBuffer->picWidth >> 2, pBuffer->picHeight >> 2);
        for (i = 0; i< (pBuffer->picHeight>>1); i++) {
            for(cnt = 0; cnt < (pBuffer->picWidth>>2); cnt++) {
                u = p_u[cnt];
                v = p_v[cnt];
                p[cnt] = ((v & 0xff) << 8) | ((v & 0xff00) << 16) | (u & 0xff) | ((u & 0xff00) << 8);
            }
            p_u += pBuffer->fbWidth >> 2;
            p_v += pBuffer->fbWidth >> 2;
            p += pBuffer->fbWidth>>2;
        }
    } else if (mpCodec->pix_fmt == PIX_FMT_BGRA
            || mpCodec->pix_fmt == PIX_FMT_RGB24
            || mpCodec->pix_fmt == PIX_FMT_BGR24) {

        AM_U8* pSrc = mFrame.data[0];
        AM_U8* pUV = (AM_U8*)p;
        AM_UINT nBytePerPixel;
        AM_UINT idxR, idxG, idxB;
        switch (mpCodec->pix_fmt) {
            case PIX_FMT_BGRA:
                AMLOG_DEBUG("--- PIX_FMT_BGRA ---\n");
                nBytePerPixel = 4;
                idxR = 2;
                idxG = 1;
                idxB = 0;
                break;
            case PIX_FMT_RGB24:
                AMLOG_DEBUG("--- PIX_FMT_RGB24 ---\n");
                nBytePerPixel = 3;
                idxR = 0;
                idxG = 1;
                idxB = 2;
                break;
            case PIX_FMT_BGR24:
                AMLOG_DEBUG("--- PIX_FMT_BGR24 ---\n");
                nBytePerPixel = 3;
                idxR = 2;
                idxG = 1;
                idxB = 0;
                break;
            default:
                AM_ERROR("Currently, not support mpCodec->pix_fmt %d.\n", mpCodec->pix_fmt);
                return ;
        }
        for(cnt = 0; cnt < pBuffer->picHeight; cnt++) {
            for(i = 0; i < pBuffer->picWidth; i++) {
                RGB2Y(&pdes[i], pSrc[nBytePerPixel * i + idxR], pSrc[nBytePerPixel * i + idxG], pSrc[nBytePerPixel * i + idxB]);
                if(cnt % 2 == 0 && i % 2 == 0) {
                    RGB2U(&pUV[i], pSrc[nBytePerPixel * i + idxR], pSrc[nBytePerPixel * i + idxG], pSrc[nBytePerPixel * i + idxB]);//U
                    RGB2V(&pUV[i + 1], pSrc[nBytePerPixel * i + idxR], pSrc[nBytePerPixel * i + idxG], pSrc[nBytePerPixel * i + idxB]);//V
                }
            }
            pdes += pBuffer->fbWidth;
            if(cnt % 2 ==0)
                pUV += pBuffer->fbWidth;
            pSrc += mFrame.linesize[0];
        }
    } else if (mpCodec->pix_fmt == PIX_FMT_RGB555LE) {
        AMLOG_DEBUG("-----PIX_FMT_RGB555LE---\n");
        AM_U16* pSrc = (AM_U16*)mFrame.data[0];
        AM_U8* pUV = (AM_U8*)p;
        AM_U8 r, g, b;
        for(cnt = 0; cnt < pBuffer->picHeight; cnt++) {
            for(i = 0; i < pBuffer->picWidth; i++) {
                r = (pSrc[i] & 0x7C00) >> 7;
                g = ((pSrc[i] & 0x3E0)) >> 2;
                b = (pSrc[i] & 0x1F) << 3;
                RGB2Y(&pdes[i], r, g, b);
                if(cnt % 2 == 0 && i % 2 == 0) {
                    RGB2U(&pUV[i], r, g, b);
                    RGB2V(&pUV[i + 1], r, g, b);
                }
            }
            pdes += pBuffer->fbWidth;
            if(cnt % 2 ==0)
                pUV += pBuffer->fbWidth;
            pSrc += mFrame.linesize[0] >> 1;
        }
    } else if(mpCodec->pix_fmt == PIX_FMT_PAL8) {
        struct RGB_QUAD{
            unsigned char blue;
            unsigned char green;
            unsigned char red;
            unsigned char alpha;
        };
        AM_U8* pSrc = (AM_U8*)mFrame.data[0];
        RGB_QUAD *pQuad = (RGB_QUAD*)mFrame.data[1];
        AM_U8* pUV = (AM_U8*)p;
        AM_U8 r, g, b;
        for(cnt = 0; cnt < pBuffer->picHeight; cnt++) {
            for(i = 0; i < pBuffer->picWidth; i++) {
                int index = pSrc[i];
                r = pQuad [index].red;
                g = pQuad [index].green;
                b = pQuad [index].blue;
                RGB2Y(&pdes[i], r, g, b);
                if(cnt % 2 == 0 && i % 2 == 0) {
                    RGB2U(&pUV[i], r, g, b);
                    RGB2V(&pUV[i + 1], r, g, b);
                }
            }
            pdes += pBuffer->fbWidth;
            if(cnt % 2 ==0)
                pUV += pBuffer->fbWidth;
            pSrc += mFrame.linesize[0] ;
        }
    } else if (mpCodec->pix_fmt == PIX_FMT_YUV420P10LE || mpCodec->pix_fmt == PIX_FMT_YUV420P9LE) {
        AM_U16 mask = 0xFFFF;
        AM_U16 offset = 0;
        switch (mpCodec->pix_fmt) {
            case PIX_FMT_YUV420P10LE:
                mask = 0x3FF;
                offset = 2;
                //AMLOG_VERBOSE("PIX_FMT_YUV420P10LE.\n");
                break;
            case PIX_FMT_YUV420P9LE:
                mask = 0x1FF;
                offset = 1;
                //AMLOG_VERBOSE("PIX_FMT_YUV420P9LE.\n");
                break;
            default:
                assert(0);
        }

        if (pBuffer->fbWidth == ((AM_UINT)mFrame.linesize[0])) {
            AM_U16* pu = (AM_U16*)mFrame.data[1];
            AM_U16* pv = (AM_U16*)mFrame.data[2];

            // copy the Y plane
            for(i = 0; i < pBuffer->picHeight; i ++) {
                AM_U16 *src = (AM_U16*)(((AM_U8*)mFrame.data[0]) +  i * mFrame.linesize[0]);
                AM_U8 *dst = (AM_U8*)(((AM_U8*)&pdes[0]) + i * pBuffer->fbWidth);
                for (j = 0; j < pBuffer->picWidth; j++) {
                    *dst = (*src&mask)>>offset;
                    dst++;
                    src++;
                }
            }

            // re-arrange U's and V's
            cnt = (pBuffer->fbWidth * pBuffer->picHeight) >>3;
            do {
                *p++ = ((*pu&mask)>>offset) | (((*pv&mask)>>offset)<<8) | (((*(pu+1)&mask)>>offset)<<16) | (((*(pv+1)&mask)>>offset)<<24);
                pu += 2;
                pv += 2;
            } while (--cnt);

        } else {
            // copy the Y plane
            for(i = 0; i < pBuffer->picHeight; i ++) {
                AM_U16 *src = (AM_U16*)(((AM_U8*)mFrame.data[0]) +  i * mFrame.linesize[0]);
                AM_U8 *dst = (AM_U8*)(((AM_U8*)&pdes[0]) + i * pBuffer->fbWidth);
                for (j = 0; j < pBuffer->picWidth; j++) {
                    *dst = (*src&mask)>>offset;
                    dst++;
                    src++;
                }
            }

            // copy u and v
            for (i = 0; i < pBuffer->picHeight/2; i ++) {
                AM_U16* pu_ = (AM_U16*)(((AM_U8*)mFrame.data[1]) + i * mFrame.linesize[1]);
                AM_U16* pv_ = (AM_U16*)(((AM_U8*)mFrame.data[2]) + i * mFrame.linesize[2]);
                AM_U8* dst =  (AM_U8*)(((AM_U8*)&p[0]) + i * pBuffer->fbWidth);
                for(j = 0; j <  pBuffer->picWidth/2; j ++){
                    *dst++ = (*pu_&mask)>>offset;
                    pu_++;
                    *dst++ = (*pv_&mask)>>offset;
                    pv_++;
                }
            }
        }
    } else {
        AM_ERROR("not support mpCodec->pix_fmt %d.\n", mpCodec->pix_fmt);
    }
}
void CFFMpegDecoder::planar2x(AM_U8* src, AM_U8* des, AM_INT src_stride, AM_INT des_stride, AM_UINT width, AM_UINT height)
{
    //AMLOG_INFO("Planar2x=====\n");
    //first line
    AM_UINT i, j;
    des[0] = src[0];
    for(i = 0; i < width -1; i++)
    {
        des[2*i + 1] = (3 * src[i] + src[i + 1])>>2;
        des[2*i + 2] = (src[i] + 3 * src[i + 1])>>2;
    }
    des[2 * width - 1] = src[width - 1];
    des += des_stride;
    //AMLOG_INFO("---finish the first line\n");
    for(j = 1; j < height; j++)
    {
        des[0]= (3*src[0] + src[src_stride])>>2;
        des[des_stride]= (src[0] + 3*src[src_stride])>>2;
        //LOGE("---finish %d line d0 elements\n", j);
        for(i = 0; i < width - 1; i++)
        {
            des[2*i + 1]= (3*src[i + 0] +   src[i + src_stride + 1])>>2;
            des[2*i + des_stride + 2]= (src[i + 0] + 3*src[i + src_stride + 1])>>2;
            des[2*i + des_stride + 1]= (src[i + 1] + 3*src[i + src_stride])>>2;
            des[2*i + 2]= (3*src[i + 1] +src[i + src_stride])>>2;
            //LOGE("---finish %d line d%d elements\n", j, i);
        }
        des[width*2 -1]= (3*src[width-1] + src[width-1 + width])>>2;
        des[width*2 -1 + des_stride]= (src[width - 1] + 3*src[width -1 + width])>>2;

        des +=des_stride * 2;
        src +=src_stride;
        //LOGE("---finish %d line d(2w-1) elements\n", j);
    }
    //AMLOG_INFO("----finish the middle lines\n");
    //last line
    des[0]= src[0];

    for (i = 0; i < width-1; i++) {
        des[2 * i + 1]= (3 * src[i] + src[i + 1])>>2;
        des[2 * i + 2]= (src[i] + 3 * src[i + 1])>>2;
    }
    des[2 * width -1]= src[width -1];
    //AMLOG_INFO("========end of  Planar2x\n");

}
void CFFMpegDecoder::Delete()
{
    AMLOG_DESTRUCTOR("CFFMpegDecoder::Delete().\n");
    CloseDecoder();

    AMLOG_DESTRUCTOR("CFFMpegDecoder::Delete(), before AM_DELETE(mpOutput).\n");
    AM_DELETE(mpOutput);
    mpOutput = NULL;

    AMLOG_DESTRUCTOR("CFFMpegDecoder::Delete() before AM_DELETE(mpInput).\n");
    AM_DELETE(mpInput);
    mpInput = NULL;

    if (mbPoolAlloc) {
        AMLOG_DESTRUCTOR("CFFMpegDecoder::Delete() before AM_RELEASE(mpBufferPool).\n");
        AM_RELEASE(mpBufferPool);
    }
    mpBufferPool = NULL;

    if (mpAccelerator && mpDSPHandler) {
        if (mpSharedRes && (mpDSPHandler->mIavFd >= 0) && (mpSharedRes->mIavFd == mpDSPHandler->mIavFd)) {
            mpSharedRes->mIavFd = -1;
            mpSharedRes->mbIavInited = 0;
        }
    }

    if (mpAccelerator) {
        AM_ASSERT(mpDSPHandler);
        mpDSPHandler->DeleteHybirdUDEC(mpAccelerator);
        mpAccelerator = NULL;
        AM_ASSERT(mpDSPHandler->mIavFd >= 0);
    }
    if (mpDSPHandler) {
        AM_ASSERT(mpDSPHandler->mIavFd >= 0);
        delete mpDSPHandler;
        mpDSPHandler = NULL;
    }

    AMLOG_DESTRUCTOR("CFFMpegDecoder::Delete(), before inherited::Delete().\n");
    inherited::Delete();
    AMLOG_DESTRUCTOR("CFFMpegDecoder::Delete() done.\n");
}

//only one inputpin
bool CFFMpegDecoder::ReadInputData()
{
	AM_ASSERT(!mpBuffer);
	AM_ASSERT(!mpPacket);

	if (!mpInput->PeekBuffer(mpBuffer)) {
		AM_ERROR("No buffer?\n");
		return false;
	}

	if (mpBuffer->GetType() == CBuffer::EOS) {
		AMLOG_INFO("CFFMpegDecoder %p get EOS.\n", this);
		//SendEOS(mpOutput);
		//msState = STATE_PENDING;
		ProcessEOS();
		return false;
	}

    mpPacket = (AVPacket*)((AM_U8*)mpBuffer + sizeof(CBuffer));
    return true;
}

bool CFFMpegDecoder::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("****CFFMpegDecoder::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);
    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            CmdAck(ME_OK);
            AMLOG_INFO("****CFFMpegDecoder::ProcessCmd, STOP cmd.\n");
            break;

        case CMD_OBUFNOTIFY:
            if (mpBufferPool->GetFreeBufferCnt() > mnFrameReservedMinus1) {
                if (msState == STATE_IDLE)
                    msState = STATE_HAS_OUTPUTBUFFER;
                else if(msState == STATE_HAS_INPUTDATA)
                    msState = STATE_READY;
            }
            break;

        case CMD_PAUSE:
            AM_ASSERT(!mbPaused);
            mbPaused = true;
            break;

        case CMD_RESUME:
            if(msState == STATE_PENDING)
                msState = STATE_IDLE;
            mbPaused = false;
            break;

        case CMD_FLUSH:
            avcodec_flush_buffers(mpCodec);
#if 0//STOP(1) should always sent to DSP, no depend on flush cmd flow
            if (mpDecoder->type == CODEC_TYPE_VIDEO) {
                AM_ASSERT(mpDSPHandler);
                StopUDEC(mpDSPHandler->mIavFd, 0, STOPFLAG_FLUSH);
            }
#endif
            mbStreamStart = false;
            msState = STATE_PENDING;
            if (mpBuffer) {
                if(mpOriDataptr) {
                    mpPacket->data = mpOriDataptr;
                    mpPacket->size += mDataOffset;
                    mpOriDataptr = NULL;
                    mDataOffset = 0;
                }

                mpBuffer->Release();
                mpBuffer = NULL;
                mpPacket = NULL;
            }
            CmdAck(ME_OK);
            break;

        case CMD_AVSYNC:
            CmdAck(ME_OK);
            break;

        case CMD_BEGIN_PLAYBACK:
            AM_ASSERT(msState == STATE_PENDING);
            AM_ASSERT(!mpBuffer);
            mbStreamStart = false;
            msState = STATE_IDLE;
            if (mpDecoder->type == CODEC_TYPE_VIDEO) {
                if (mpSharedRes->get_outpic) {
                    AM_ASSERT(mpDSPHandler);
                    AM_ASSERT(mpAccelerator);
                    AM_ASSERT(mpSharedRes->udec_state == IAV_UDEC_STATE_READY);
                    //start decoding for sw and hybird rv40 mode
                    if (mpDSPHandler && mpAccelerator) {
                        mpDSPHandler->StartUdec(mpAccelerator);
                        mbStreamStart = true;
                        pthread_mutex_lock(&mpSharedRes->mMutex);
                        GetUdecState(mpDSPHandler->mIavFd, &mpSharedRes->udec_state, &mpSharedRes->vout_state, &mpSharedRes->error_code);
                        pthread_mutex_unlock(&mpSharedRes->mMutex);
                        AM_ASSERT(IAV_UDEC_STATE_RUN == mpSharedRes->udec_state);
                        PostEngineMsg(IEngine::MSG_NOTIFY_UDEC_IS_RUNNING);
                    }
                }
            }
            break;

        case CMD_FORCE_LOW_DELAY:
            AM_ASSERT(mpSharedRes);
            AM_ASSERT(mpCodec);
            if (mpSharedRes && mpCodec) {
                mpSharedRes->mForceLowdelay = 1;
                mpCodec->flags |= CODEC_FLAG_LOW_DELAY;
                AMLOG_INFO("try force low delay mode (skip B picture).\n");
            }
            break;

        case CMD_SOURCE_FILTER_BLOCKED:
            break;

        case CMD_REALTIME_SPEEDUP:
            mbNeedSpeedUp = 1;
            break;

        default:
            AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return false;
}

void CFFMpegDecoder::ProcessVideo()
{
     AM_ASSERT(msState == STATE_READY);

    AM_INT frame_finished, ret;

    //get from AV_NOPTS_VALUE(avcodec.h), use (mpPacket->pts!=AV_NOPTS_VALUE) will have warnings, use (mpPacket->pts >= 0) judgement instead
    //convert to unit ms
    if (!mbParallelDecoder) {
        if (mpPacket->pts >= 0) {
            mCurrInputTimestamp = mpPacket->pts;
            AMLOG_PTS("CFFMpegDecoder[video] recieve a PTS, mpPacket->pts=%llu.\n",mpPacket->pts);
        }
        else {
            mCurrInputTimestamp = 0;
            AMLOG_WARN("CFFMpegDecoder[video] has no pts.\n");
        }
        mpCodec->reordered_opaque = mCurrInputTimestamp;
    }

    //for bug917/921, hy pts issue
    if(mpAccelerator && mpSharedRes)
    {
        if(mpAccelerator->video_ticks!=mpSharedRes->mVideoTicks)
        {
            mpAccelerator->video_ticks = mpSharedRes->mVideoTicks;
            AMLOG_DEBUG("now mpAccelerator->video_ticks = %d.\n", mpAccelerator->video_ticks);
        }
    }

    AMLOG_VERBOSE("CFFMpegDecoder[video] decoding start.\n");

#ifdef __use_hardware_timmer__
    time_count_before_dec = AM_get_hw_timer_count();
    ret = avcodec_decode_video2(mpCodec, &mFrame, &frame_finished, mpPacket);
    time_count_after_dec = AM_get_hw_timer_count();
    totalTime += AM_hwtimer2us(time_count_before_dec - time_count_after_dec);
#else
    struct timeval tvbefore, tvafter;
    gettimeofday(&tvbefore,NULL);
    ret = avcodec_decode_video2(mpCodec, &mFrame, &frame_finished, mpPacket);
    gettimeofday(&tvafter,NULL);
    totalTime += (tvafter.tv_sec -tvbefore.tv_sec)*1000000 + tvafter.tv_usec - tvbefore.tv_usec;
#endif

    AMLOG_VERBOSE("CFFMpegDecoder[video] decoding done.\n");

    if (!mbStreamStart) {
        pthread_mutex_lock(&mpSharedRes->mMutex);
        GetUdecState(mpDSPHandler->mIavFd, &mpSharedRes->udec_state, &mpSharedRes->vout_state, &mpSharedRes->error_code);
        if (IAV_UDEC_STATE_RUN == mpSharedRes->udec_state) {
            PostEngineMsg(IEngine::MSG_NOTIFY_UDEC_IS_RUNNING);
            mbStreamStart = true;
        }
        pthread_mutex_unlock(&mpSharedRes->mMutex);
    }

    if (!(decodedFrame & 0x1f) && decodedFrame)
        AMLOG_PERFORMANCE("video [%d] decoded [%d] dropped frames' average = %d uS  \n", decodedFrame, droppedFrame, totalTime/(decodedFrame+droppedFrame));

    if (ret < 0) {
        AMLOG_ERROR("--- while decoding video---, err=%d.\n",ret);
        droppedFrame ++;
    } else if (frame_finished) {
        CBuffer *pOutputBuffer;
        if (mbNV12) {
            pOutputBuffer = (CBuffer *)(mFrame.opaque);
            AM_ASSERT(pOutputBuffer);
            if(pOutputBuffer)
            {
                if (mbParallelDecoder == true) {
                    pOutputBuffer->mPTS = mFrame.pts;
                }
                mpOutput->SendBuffer(pOutputBuffer);
            }
        } else {

            //chech valid, for sw decoding and convert from yuv420p to nv12 format
            if (!mFrame.data[0] /*|| !mFrame.data[1] || !mFrame.data[2] */|| !mFrame.linesize[0] /*|| !mFrame.linesize[1] || !mFrame.linesize[2] */) {
                AM_ASSERT(0);
                AM_ERROR("get not valid frame, not enough memory? or not supported pix_fmt %d?.\n", mpCodec->pix_fmt);
                msState = STATE_HAS_OUTPUTBUFFER;
                mpBuffer->Release();
                mpBuffer = NULL;
                mpPacket = NULL;
                return;
            }

            if (!mpBufferPool->AllocBuffer(pOutputBuffer, 0)) {
                AMLOG_INFO("%p, get output buffer fail, exit.\n", this);
                //mbRun = false;
                msState = STATE_PENDING;
                return;
            }
            pOutputBuffer->mPTS = mFrame.reordered_opaque;
            CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pOutputBuffer;
            ConvertFrame(pVideoBuffer);
            mpOutput->SendBuffer(pOutputBuffer);
        }
        decodedFrame ++;
        AMLOG_VERBOSE("CFFMpegDecoder[video] send a decoded frame done.\n");
    }

    AM_ASSERT(mpBuffer);
    mpBuffer->Release();
    mpBuffer = NULL;
    mpPacket = NULL;
    msState = STATE_IDLE;
}

void CFFMpegDecoder::ProcessAudio()
{
    AM_ASSERT(msState == STATE_READY);

    CBuffer *pOutputBuffer;
    AM_INT decodedSize;
    AM_INT SampleNum = 0;
    if (!mpOutput->AllocBuffer(pOutputBuffer)) {
        AMLOG_INFO("%p, get output buffer fail, exit.\n", this);
        mbRun = false;
        return;
    }

    AM_INT out_size = pOutputBuffer->mBlockSize;

    //get from AV_NOPTS_VALUE(avcodec.h), use (mpPacket->pts!=AV_NOPTS_VALUE) will have warnings, use (mpPacket->pts >= 0) judgement instead
    //convert to ms
    if(mpPacket->pts >= 0) {
        mCurrInputTimestamp = mpPacket->pts;
    }
    AMLOG_PTS("CFFMpegDecoder[audio] recieve a PTS, mpPacket->pts=%llu, mCurrInputTimestamp =%llu ms, num=%d, den=%d.\n",mpPacket->pts, mCurrInputTimestamp, mpStream->time_base.num, mpStream->time_base.den);

    // ddr 2010.09.07: as AudioSink support only stereo/mono (<=2 channels), so force downmix if needed.
    mpCodec->request_channels = 2;
    // ddr: end

    if ( mbIsFirstPTS ) {
        mbIsFirstPTS = false;
        mStartPTS = mCurrInputTimestamp;
    }

#ifdef __use_hardware_timmer__
    time_count_before_dec = AM_get_hw_timer_count();
    decodedSize = avcodec_decode_audio3(mpCodec, (short *)(pOutputBuffer->mpData), &out_size, mpPacket);
    time_count_after_dec = AM_get_hw_timer_count();
    totalTime += AM_hwtimer2us(time_count_before_dec - time_count_after_dec);
#else
    struct timeval tvbefore, tvafter;
    gettimeofday(&tvbefore,NULL);
    decodedSize = avcodec_decode_audio3(mpCodec, (short *)(pOutputBuffer->mpData), &out_size, mpPacket);
    gettimeofday(&tvafter,NULL);
    totalTime += (tvafter.tv_sec -tvbefore.tv_sec)*1000000 + tvafter.tv_usec - tvbefore.tv_usec;
#endif

    if (!(decodedFrame & 0x1f) && decodedFrame)
    {
        AMLOG_PERFORMANCE("audio [%d] decoded [%d] dropped frames' average = %d uS  \n", decodedFrame, droppedFrame, totalTime/(decodedFrame+droppedFrame));
    }

    /* modified by liujian, to process the case "decodedSize == 0"
    */
    if (decodedSize <= 0) {
        AMLOG_ERROR("CFFMpegDecoder[audio] decoding,error, decodedSize =0x%x Bytes.\n", decodedSize);
        droppedFrame ++;
        pOutputBuffer->Release();
    }
    else if(decodedSize > mpPacket->size)    // consumed size > input size, must be imcomplete input frames, drop it.
    {
        AMLOG_ERROR("CFFMpegDecoder[audio] decoding,error, decodedSize[0x%x] Bytes > inputSize[0x%x] Bytes.\n", decodedSize, mpPacket->size);
        droppedFrame ++;
        pOutputBuffer->Release();
    }
    else {

        if (out_size >= 0) {
            /*if a frame has been decoded, output it*/
            AM_INT SampleRate = mpCodec->sample_rate;
            //Check sample_rate is valid
            if(SampleRate <= 0)
            {
                //TO DO : may use bit_rate and file_size to get a valid value
                //Here just give it a fixed value
                SampleRate = 48000;
            }
            pOutputBuffer->SetType(CBuffer::DATA);
            pOutputBuffer->SetDataSize(out_size);
            switch(mpCodec->sample_fmt) {
                case SAMPLE_FMT_U8:
                    SampleNum = out_size / mpCodec->channels;
                    break;
                case SAMPLE_FMT_S16:
                    SampleNum = (out_size / mpCodec->channels) >> 1;
                    break;
                case SAMPLE_FMT_S32:
                case SAMPLE_FMT_FLT:
                    SampleNum = (out_size / mpCodec->channels) >> 2;
                    break;
                case SAMPLE_FMT_DBL:
                    SampleNum = (out_size / mpCodec->channels) >> 3;
                    break;
                default:
                    SampleNum = 0;
                    break;
            }
            if(!mFrameCount)
                pOutputBuffer->mPTS = mCurrInputTimestamp;
            else {
                mAccFramePTS += SampleNum * TICK_PER_SECOND / SampleRate;
                pOutputBuffer->mPTS = mCurrInputTimestamp + mAccFramePTS;
            }

            //set audio info
            ((CAudioBuffer*)pOutputBuffer)->sampleRate = SampleRate;
            ((CAudioBuffer*)pOutputBuffer)->numChannels = mpCodec->channels;
            ((CAudioBuffer*)pOutputBuffer)->sampleFormat = mpCodec->sample_fmt;

            mpOutput->SendBuffer(pOutputBuffer);
            AMLOG_VERBOSE("CFFMpegDecoder[audio] send a decoded data done.\n");
        } else {
            AMLOG_ERROR("out_size<0, %d?", out_size);
            pOutputBuffer->Release();
        }

        decodedFrame ++;

        if(decodedSize == mpPacket->size) { //decode done

            AMLOG_DEBUG("CFFMpegDecoder[audio] decoding,done, OutputSample=%d,  decodedSize=0x%x Bytes.\n",
                SampleNum, decodedSize);
        } else { //has remainning data in avpacket

            if(!mDataOffset) {
                //save original data ptr
                mpOriDataptr = mpPacket->data;
            }
            AMLOG_DEBUG("CFFMpegDecoder[audio] decoding, remain ES, OutputSample=%d, decodedSize=0x%x Bytes, mpPacket->size=0x%x Bytes. mpPacket->data=%p.\n",
                out_size/mpCodec->channels/((mpCodec->sample_fmt==SAMPLE_FMT_S16) ? 2 :1),
                decodedSize, mpPacket->size, mpPacket->data);

            mpPacket->data += decodedSize;
            mpPacket->size -= decodedSize;
            mDataOffset += decodedSize;
            //if has output buffer, continue to process remaining data
            if(!(mpBufferPool->GetFreeBufferCnt() > mnFrameReservedMinus1))
            {
                msState = STATE_HAS_INPUTDATA;
            }

            //if there's CMD, deal with it. in Case of input packet contains to much data, and won't respond to the msg/cmd
            {
                CMD cmd;
                if(mpWorkQ->PeekCmd(cmd))
                {
                    ProcessCmd(cmd);
                }
            }
            mFrameCount++;
            return;
        }
    }

    if(mDataOffset) {
        AM_ASSERT(mpOriDataptr);
        AM_ASSERT(mDataOffset);
        //restore original data ptr
        mpPacket->data = mpOriDataptr;
        mpPacket->size = mDataOffset;
        mpOriDataptr = NULL;
        mDataOffset = 0 ;
    }

    mpBuffer->Release();
    mpBuffer = NULL;
    mpPacket = NULL;
    msState = STATE_IDLE;
    mFrameCount = 0;
    mAccFramePTS = 0;
}

void CFFMpegDecoder::ProcessSubtitle()
{
    AM_ASSERT(msState == STATE_READY);
    CBuffer *pOutputBuffer;
    AM_INT decodedSize = 1;
    AVSubtitle* psub = NULL;
    AM_INT r, g, b, a;//, y, u, v
    if (!mpOutput->AllocBuffer(pOutputBuffer)) {
        AMLOG_ERROR("%p, get output buffer fail, exit.\n", this);
        mbRun = false;
        return;
    }
    AM_INT out_size = pOutputBuffer->mBlockSize;

    if(mpPacket->pts >= 0) {
        mCurrInputTimestamp = mpPacket->pts;
        ConvertSubtitlePTS(&mCurrInputTimestamp);
        pOutputBuffer->mPTS = mCurrInputTimestamp;
    }
    if ( mbIsFirstPTS ) {
        mbIsFirstPTS = false;
        mStartPTS = mCurrInputTimestamp;
    }
    switch(mpCodec->codec_id) {
        case CODEC_ID_SSA:
            ((CSubtitleBuffer*)pOutputBuffer)->subType = SUBTYPE_ASS;
            break;
        case CODEC_ID_DVB_SUBTITLE:
            ((CSubtitleBuffer*)pOutputBuffer)->subType = SUBTYPE_DVDSUP;
            break;
        case CODEC_ID_TEXT:  ///< raw UTF-8 text
            ((CSubtitleBuffer*)pOutputBuffer)->subType = SUBTYPE_TEXT;
            break;
        case CODEC_ID_XSUB:
            ((CSubtitleBuffer*)pOutputBuffer)->subType = SUBTYPE_XSUB;
            break;
        default:
            ((CSubtitleBuffer*)pOutputBuffer)->subType = SUBTYPE_OTHER;
            break;
    }
    if(mpCodec->codec_id != CODEC_ID_SSA) {
        decodedSize = avcodec_decode_subtitle2(mpCodec, (AVSubtitle*)(pOutputBuffer->mpData), &out_size, mpPacket);
    }else if(ParseSubAssDialogue(mpPacket->data, pOutputBuffer) == ME_OK) {
        ASSStyle *pStyle = ass_style_get((ASSSplitContext*)mpCodec->priv_data, (char*)(((CSubtitleBuffer*)pOutputBuffer)->style));
        if(!pStyle) {
            AMLOG_DEBUG("NO  %s style information.\n", ((CSubtitleBuffer*)pOutputBuffer)->style);
            pStyle = ass_style_get((ASSSplitContext*)mpCodec->priv_data, NULL);
            if(!pStyle) {
                AMLOG_INFO("NO default style information.\n");
                decodedSize = -1;
            }
        }
        if(pStyle) {
            strncpy((char*)(((CSubtitleBuffer*)pOutputBuffer)->fontname), pStyle->font_name, strlen((char*)(pStyle->font_name)) + 1);
            ((CSubtitleBuffer*)pOutputBuffer)->fontsize = pStyle->font_size;
            ((CSubtitleBuffer*)pOutputBuffer)->PrimaryColour = pStyle->primary_color;
            ((CSubtitleBuffer*)pOutputBuffer)->BackColour = pStyle->back_color;
            ((CSubtitleBuffer*)pOutputBuffer)->Bold = pStyle->bold;
            ((CSubtitleBuffer*)pOutputBuffer)->Italic = pStyle->italic;
            decodedSize = mpPacket->size;
        }
    }else {
        decodedSize = -1;
    }
    if (!(decodedFrame & 0x1f) && decodedFrame)
    {
        //AMLOG_PERFORMANCE("subtitle [%d] decoded [%d] dropped frames' average = %d uS  \n", decodedFrame, droppedFrame, totalTime/(decodedFrame+droppedFrame));
    }
    if (decodedSize < 0 || decodedSize > mpPacket->size || out_size <= 0) {
        AMLOG_ERROR("CFFMpegDecoder[subtitle] decoding,error, decodedSize =0x%x Bytes.\n", decodedSize);
        droppedFrame ++;
        pOutputBuffer->Release();
    }
    else {
        if(((CSubtitleBuffer*)pOutputBuffer)->subType != SUBTYPE_ASS) {
            psub = (AVSubtitle*)(pOutputBuffer->mpData);
            mCurrInputTimestamp = mpPacket->pts;
            //get palette and convert it to YUV.added by cx
           for (AM_UINT i = 0; i < psub->num_rects; i++)
           {
                for (AM_INT j = 0; j < psub->rects[i]->nb_colors; j++)
                {
                    RGBA_IN(r, g, b, a, (uint32_t*)psub->rects[i]->pict.data[1] + j);
                    av_log(NULL, AV_LOG_DEBUG, "a=%d,r=%d,g=%d,b=%d\n", a,r,g,b);
                    //Convert to RGB565
                    r = (r & 0xF8) >> 3;
                    g = (g & 0xFC) >> 2;
                    b = (b & 0xF8) >> 3;
                    //Save as RGB565
                    RGB565_OUT((AM_U16*)psub->rects[i]->pict.data[1] + j, r, g, b);
                }
            }
            ((CSubtitleBuffer*)pOutputBuffer)->start_time = psub->start_display_time * 90;
            ((CSubtitleBuffer*)pOutputBuffer)->end_time = psub->end_display_time * 90;
        }
        if(((CSubtitleBuffer*)pOutputBuffer)->start_time == 0) {
            ((CSubtitleBuffer*)pOutputBuffer)->start_time = pOutputBuffer->mPTS;
            ((CSubtitleBuffer*)pOutputBuffer)->end_time += pOutputBuffer->mPTS;
        }
        mpOutput->SendBuffer(pOutputBuffer);
        decodedFrame ++;
    }

    mpBuffer->Release();
    mpBuffer = NULL;
    mpPacket = NULL;
    msState = STATE_IDLE;

}
//Parse dialogure information in ASS/SSA subtitle packet.
AM_ERR CFFMpegDecoder::ParseSubAssDialogue(AM_U8* dialog, void* output)
{
    AM_U8 start_time[128];
    AM_U8 end_time[128];
    CSubtitleBuffer* pOutbuffer = (CSubtitleBuffer*)output;
    while(*dialog && dialog) {
        SkipSpace(dialog);
        if(dialog[0] == ';' || (dialog[0] == '!' && dialog[1] == ':')) {
            AMLOG_DEBUG("skip comments.\n");
        }
        else if(!strncmp((char*)dialog, "Dialogue:", 9)) {
            AMLOG_DEBUG("has Dialogure information.\n");
            dialog += 9;
            dialog += SkipSpace(dialog);
            //dialog format:Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
            //every field segmented by comma.
            //skip Marked field.
            dialog += GetSubAssFieldContent(NULL, dialog);
            //Get start time.
            dialog += GetSubAssFieldContent(start_time, dialog);
            if(ConvertSubTimestamp(start_time, &(pOutbuffer->start_time)) < 0)
                return ME_ERROR;
            //Get end time.
            dialog += GetSubAssFieldContent(end_time, dialog);
            if(ConvertSubTimestamp(end_time, &(pOutbuffer->end_time)) < 0)
                return ME_ERROR;
            //Get style name.
            dialog += GetSubAssFieldContent(pOutbuffer->style, dialog);
            //Skip Name, MarginL, MarginR, MarginV, Effect  field.
            AM_INT i = 5;
            while(i) {
                dialog += GetSubAssFieldContent(NULL, dialog);
                i--;
            }
            //Get text, the last field.
            dialog += SkipSpace(dialog);
            strcpy((char*)(pOutbuffer->mpData), (char*)dialog);
            ConvertSubAssString(pOutbuffer->mpData);
            return ME_OK;
        }
        dialog += strcspn((char*)dialog, "\n") + 1;
    }
    AMLOG_ERROR("There is no dialogure information.");
    return ME_ERROR;
}
//Get the field content and move the source point to next item
 AM_UINT CFFMpegDecoder::GetSubAssFieldContent(AM_U8* dest, AM_U8* source)
{
    AM_INT len1 = 0;
    AM_INT len2 = 0;
    len1 = SkipSpace(source);
    if(len1 < 0) {
        AMLOG_ERROR("At the end of buffer, there is no field any more.\n");
        return 0;
    }
    source += len1;
    len2 = strcspn((char*)source, ",");
    len1 = len2;
    if(!len1 && source[0] != ',') {
        AMLOG_ERROR("There is no field");
        return 0;
    }
    //Skip the spaces before comma.
    while(len1 && source[len1 - 1] == ' ') {len1--;}
    if(!len1) {
        AMLOG_DEBUG("This field is empty.\n");
        if(dest) {
            dest[0] = '\0';
        }
        return len2 + 1;
    }
    //Copy content to destination string.
    if(dest) {
        strncpy((char*)dest, (char*)source, len1);
        dest[len1] = '\0';
    }
    return len2 + 1;
}
//Skip space, return the space number, return -1 when at the end of buffer.
AM_INT CFFMpegDecoder::SkipSpace(AM_U8* buffer)
{
    AM_INT i = 0;
    while(buffer[i] == ' ') {
        i++;
    }
    if(buffer[i]) {
        return i;
    }
    else {
        AMLOG_ERROR("At the end now!\n");
        return -1;
    }
}
//Convert the timestamp format to ms according to 90khz time base.
AM_ERR CFFMpegDecoder::ConvertSubTimestamp(AM_U8* buffer, AM_U64* time)
{
    AM_INT c, h, m, s, cs;
    if ((c = sscanf((char*)buffer, "%d:%02d:%02d.%02d", &h, &m, &s, &cs)) == 4) {
        *time = (3600000*h + 60000*m + 1000*s + 10*cs) * 90;
        return ME_OK;
    }
    return ME_ERROR;
}
//Convert \N to '\n', and any other convertion
void CFFMpegDecoder::ConvertSubAssString(AM_U8* ass_text)
{
    while(ass_text[0]) {
        //skip command like:{\command(parameters separated by comma)}
        if(ass_text[0] == '{') {
            AM_INT pos = 0;
            while(ass_text[pos] && ass_text[0] != '}')
                pos++;
            if(ass_text[0] == '}')
                strcpy((char*)ass_text, (char*)ass_text + pos +1);
            else {
                AMLOG_ERROR("Dialog string format wrong!\n");
                return;
            }
        }
        //convert "\n"or"\N" to ' \n'
        if(ass_text[0] == '\\' && (ass_text[1] == 'n' || ass_text[1] == 'N')) {
            ass_text[0] = '\n';
            strcpy((char*)ass_text + 1, (char*)ass_text +2);
        }
        ass_text++;
    }
}
void CFFMpegDecoder::ConvertSubtitlePTS(AM_U64 * pts)
{
    *pts *= mPTSSubtitle_Num;
    if (mPTSSubtitle_Den != 1) {
        *pts /=  mPTSSubtitle_Den;
    }
}
void CFFMpegDecoder::ProcessEOS()
{
    //if video format with CODEC_CAP_DELAY , get out all frames left then send eos out
    if (mpDecoder->type == CODEC_TYPE_VIDEO && (mpCodec->codec->capabilities & CODEC_CAP_DELAY))
    {
        AM_INT frame_finished, ret;
        AVPacket packet;
        packet.data = NULL;
        packet.size = 0;
        packet.flags = 0;

            AMLOG_INFO("CFFMpegDecoder[video] eos wait left frames out start.\n");
            while(1)
            {
                AMLOG_INFO("CFFMpegDecoder[video] eos wait left frams out......\n");

            ret = avcodec_decode_video2(mpCodec, &mFrame, &frame_finished, &packet);

            if(0>=ret)//all frames out
                break;

            if (frame_finished) {
                CBuffer *pOutputBuffer;
                if (mbNV12) {
                    pOutputBuffer = (CBuffer *)(mFrame.opaque);
                    AM_ASSERT(pOutputBuffer);
                    if(pOutputBuffer)
                    {
                        if (mbParallelDecoder == true) {
                            pOutputBuffer->mPTS = mFrame.pts;
                        }
                        mpOutput->SendBuffer(pOutputBuffer);
                     }
                } else {
                    if (!mpBufferPool->AllocBuffer(pOutputBuffer, 0)) {
                        AMLOG_INFO("%p, get output buffer fail, exit.\n", this);
                        //mbRun = false;
                        msState = STATE_PENDING;
                        return;
                    }
                    pOutputBuffer->mPTS = mFrame.reordered_opaque;
                    CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pOutputBuffer;
                    ConvertFrame(pVideoBuffer);
                    mpOutput->SendBuffer(pOutputBuffer);
                }
                decodedFrame ++;
                AMLOG_VERBOSE("CFFMpegDecoder[video] eos send a decoded frame done.\n");
            }
        }
        AMLOG_INFO("CFFMpegDecoder[video] eos wait left frames out done.\n");
    }

    //if not video format, send eos out directly
    SendEOS(mpOutput);
    mpBuffer->Release();
    mpBuffer = NULL;
    AMLOG_INFO("CFFMpegDecoder send EOS done.\n");
}

void CFFMpegDecoder::OnRun()
{
    /*if (OpenDecoder() != ME_OK)
    {
        AMLOG_ERROR("CFFMpegDecoder OpenDecoder error.\n");
        CmdAck(ME_ERROR);
        return;
    }
    else
    {
        AMLOG_VERBOSE("CFFMpegDecoder OpenDecoder OK.\n");
        CmdAck(ME_OK);
    }*///move OpenDecoder() to SetInputFormat() for thread safety, always ack ok
    CmdAck(ME_OK);

    //AMLOG_VERBOSE("CFFMpegDecoder %p before avcodec_flush_buffers.\n", this);
    //avcodec_flush_buffers(mpCodec);
    //AMLOG_VERBOSE("CFFMpegDecoder %p after avcodec_flush_buffers.\n", this);

#ifdef __use_hardware_timmer__
    AM_open_hw_timer();
#endif

    totalTime = 0;
    decodedFrame = 0;
    droppedFrame = 0;

#ifdef AM_DEBUG
    //assert here
    if (mpDecoder->type == CODEC_TYPE_VIDEO) {
        AM_ASSERT(mpBufferPool);
        AM_ASSERT(mpBufferPool == mpOutput->mpBufferPool);
    }
#endif

    mpBufferPool = mpOutput->mpBufferPool;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    mbRun = true;
    msState = STATE_IDLE;

    AMLOG_INFO("CFFMpegDecoder %p start OnRun loop.\n", this);

    while (mbRun) {

#ifdef AM_DEBUG
    if (mpDecoder->type == CODEC_TYPE_VIDEO) {
        AMLOG_STATE("**Tag video: ");
    } else if (mpDecoder->type == CODEC_TYPE_AUDIO) {
        AMLOG_STATE("**Tag audio: ");
    }else if (mpDecoder->type == CODEC_TYPE_SUBTITLE) {
        AMLOG_STATE("**Tag subtitle: ");
    }
    AMLOG_STATE("start switch, msState=%d, %d input data, %d free buffers.\n", msState, mpInput->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt());
#endif

        switch (msState) {

            case STATE_IDLE:
                if(mbPaused){
                    msState = STATE_PENDING;
                    break;
                }

                if (mbNeedSpeedUp) {
                    if (CODEC_TYPE_AUDIO == mpDecoder->type) {
                        speedUp();
                    }
                    mbNeedSpeedUp = 0;
                    break;
                }

                if(mpBufferPool->GetFreeBufferCnt() > mnFrameReservedMinus1) {
                    msState = STATE_HAS_OUTPUTBUFFER;
                } else {
                    type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                    if(type == CQueue::Q_MSG) {
                        ProcessCmd(cmd);
                    } else {
                        AM_ASSERT((CQueueInputPin*)result.pOwner == (CQueueInputPin*)mpInput);
                        if (ReadInputData()) {
                            msState = STATE_HAS_INPUTDATA;
                        }
                    }
                }
                break;

            case STATE_HAS_OUTPUTBUFFER:
                AM_ASSERT(mpBufferPool->GetFreeBufferCnt() > mnFrameReservedMinus1);
                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                if(type == CQueue::Q_MSG) {
                    ProcessCmd(cmd);
                } else {
                    AM_ASSERT((CQueueInputPin*)result.pOwner == (CQueueInputPin*)mpInput);
                    if (ReadInputData()) {
                        msState = STATE_READY;
                    }
                }
                break;

            case STATE_PENDING:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_HAS_INPUTDATA:
                AM_ASSERT(mpPacket);
                AM_ASSERT(mpBuffer);

                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_READY:
                AM_ASSERT(mpPacket);
                AM_ASSERT(mpBuffer);

                if(NULL==mpPacket->data && 0==mpPacket->size && 0==mpPacket->flags)//drop invalid data
                {
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    mpPacket = NULL;
                    msState = STATE_IDLE;
                    break;
                }

                if(mpDecoder->type == CODEC_TYPE_VIDEO) {
                    ProcessVideo();
                } else if (mpDecoder->type == CODEC_TYPE_AUDIO) {
                    ProcessAudio();
                } else if (mpDecoder->type == CODEC_TYPE_SUBTITLE) {
                        ProcessSubtitle();
                }else {
                    AM_ERROR("wrong ffmpeg decoder type\n");
                }
                break;

            default:
                AM_ERROR(" %d",(AM_UINT)msState);
                break;
        }
        AMLOG_STATE("CFFMpegDecoder %p end switch, msState=%d, mbRun = %d.\n", this, msState, mbRun);
    }
    //purge buffer when it is video codec
    if(mpDecoder->type == CODEC_TYPE_VIDEO && mpOutput)//check for safe, delete mpOutput from diff thread
    {
            if(mpOutput->mpPeer)//check for safe, delete mpOutput from diff thread
                mpOutput->mpPeer->Purge();
    }
    //mpBufferPool->SetNotifyOwner(NULL);
    if(mpBuffer) {

        //restore original data ptr, ugly here
        if(mpOriDataptr) {
            mpPacket->data = mpOriDataptr;
            mpPacket->size += mDataOffset;
             mpOriDataptr = NULL;
            mDataOffset = 0;
        }

        mpBuffer->Release();
        mpBuffer = NULL;
    }

    //do close decoder
    //CloseDecoder();
    AMLOG_INFO("CFFMpegDecoder %p OnRun exit, msState=%d.\n", this, msState);
}

bool CFFMpegDecoder::SendEOS(CFFMpegDecoderOutput *pPin)
{
    CBuffer *pBuffer;

    AMLOG_INFO("Send EOS\n");

    #ifdef AM_DEBUG
    if(mpDecoder->type == CODEC_TYPE_AUDIO)
        AMLOG_INFO("Audio");
    else if(mpDecoder->type == CODEC_TYPE_VIDEO)
        AMLOG_INFO("Video");
    else
        AMLOG_INFO("Subtitle");
    #endif

    if(decodedFrame || droppedFrame)
        AMLOG_INFO("[%d] decoded [%d] dropped frames' average = %d uS  \n", decodedFrame, droppedFrame, totalTime/(decodedFrame+droppedFrame));

    if (!pPin->AllocBuffer(pBuffer))
        return false;

    pBuffer->SetType(CBuffer::EOS);
    pPin->SendBuffer(pBuffer);

    return true;
}

AM_ERR CFFMpegDecoder::Stop()
{
    //should signal ffmpeg decoder exit done, then call CloseDecoder [avcodec_close(mpCodec)];
    //tmp modify: check if hybrid mp4 in pause
    if(mpDSPHandler && mpAccelerator && accelerator_type_amba_hybirdmpeg4_idctmc==mpCodec->extern_accelerator_type) {
        ResumeUdec(mpDSPHandler->mIavFd, 0);
    }

    AMLOG_INFO("start quit decoder's OnRun thread.\n");
    inherited::Stop();
    AMLOG_INFO("quit decoder's OnRun thread done, start destroy ffmpeg decoder's content.\n");

    if (mpDSPHandler && mpAccelerator) {
        AMLOG_INFO("[flow cmd]: Call IAV_IOC_UDEC_STOP\n");
        if ((::ioctl(mpDSPHandler->mIavFd, IAV_IOC_UDEC_STOP, 0)) < 0) {
            AM_PERROR("IAV_IOC_STOP_DECODE");
            //return ME_BAD_STATE;
        }
        AMLOG_INFO("[flow cmd]: Call IAV_IOC_UDEC_STOP done\n");
    }

    CloseDecoder();
    AMLOG_INFO("destroy ffmpeg decoder's content done.\n");
    return ME_OK;
}

AM_ERR CFFMpegDecoder::SetInputFormat(CMediaFormat *pFormat)
{
    AM_INT error = 0;

    if (*pFormat->pFormatType != GUID_Format_FFMPEG_Stream)
        return ME_ERROR;

    if (*pFormat->pMediaType != GUID_Video && *pFormat->pMediaType != GUID_Audio && *pFormat->pMediaType != GUID_Subtitle)
        return ME_ERROR;

    //mpCodec = (AVCodecContext*)pFormat->format;
    mpStream = (AVStream*)pFormat->format;
    mpCodec = (AVCodecContext*)mpStream->codec;
    //mpCodec->get_buffer = DecoderGetBuffer;
    //mpCodec->release_buffer = DecoderReleaseBuffer;

    if(*pFormat->pMediaType == GUID_Video) {
        AMLOG_INFO("Vdecoder SetInputFormat, 0x%x.\n", mLogOutput);
        enum CodecID ori_codec_id=mpCodec->codec_id;

        if(!(mpSharedRes->mDecoderSelect))//sw and hy all use pipeline
        {    //find parallel nv12 format codec first
            AMLOG_INFO("Find parallel nv12 decoder first.\n");
            mpCodec->codec_id = (enum CodecID)((int)ori_codec_id + (int)CODEC_ID_PARALLEL_OFFSET);
            mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
            if (mpDecoder == NULL) {
                AMLOG_INFO("Cannot find a parallel nv12 decoder\n");
                mpCodec->codec_id = (enum CodecID)((int)ori_codec_id + (int)CODEC_ID_NV12_OFFSET);
                mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
                if(mpDecoder == NULL) {
                    AMLOG_INFO("Cannot find a nv12 decoder\n");
                    mbNV12=false;
                    mpCodec->codec_id = ori_codec_id;
                    mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
                    if(mpDecoder == NULL) {
                        AM_ERROR("ffmpeg video decoder not found, mpCodec->codec_id=%x. \n",mpCodec->codec_id);
                        return ME_ERROR;
                    }
                }
            } else {
                mbParallelDecoder = true;
                mnFrameReservedMinus1 = 3; // 3 stage pipeline
            }
        }
        else if (mpSharedRes->mDecoderSelect == 1)
        {    //find nv12 format codec first
            AMLOG_INFO("Find nv12 decoder first.(debug mode)\n");
            mpCodec->codec_id = (enum CodecID)((int)ori_codec_id + (int)CODEC_ID_NV12_OFFSET);
            mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
            if (mpDecoder == NULL) {
                AMLOG_INFO("Cannot find a nv12 decoder\n");
                mpCodec->codec_id = (enum CodecID)((int)ori_codec_id + (int)CODEC_ID_PARALLEL_OFFSET);
                mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
                if(mpDecoder == NULL) {
                    AMLOG_INFO("Cannot find a parallel nv12 decoder\n");
                    mbNV12=false;
                    mpCodec->codec_id = ori_codec_id;
                    mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
                    if(mpDecoder == NULL) {
                        AM_ERROR("ffmpeg video decoder not found, mpCodec->codec_id=%x. \n",mpCodec->codec_id);
                        return ME_ERROR;
                    }
                } else {
                    mbParallelDecoder = true;
                    mnFrameReservedMinus1 = 3; // 3 stage pipeline
                }
            }
        }else if (mpSharedRes->mDecoderSelect == 2){
            AMLOG_INFO("Find ori sw decoder only.(debug mode)\n");
            mbNV12=false;
            mpCodec->codec_id = ori_codec_id;
            mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
            if(mpDecoder == NULL) {
                AM_ERROR("ffmpeg video decoder not found, mpCodec->codec_id=%x. \n",mpCodec->codec_id);
                return ME_ERROR;
            }
        }
        AMLOG_INFO("@@ before DSPHandler::Create(), mbParallelDecoder %d.\n", mbParallelDecoder);
        mpDSPHandler = DSPHandler::Create();
        AM_ASSERT(mpDSPHandler);
        AM_ASSERT(mpSharedRes);

        AM_ASSERT(mpSharedRes);
        AM_ASSERT(!mpSharedRes->mbIavInited);
        if (mpSharedRes && (mpDSPHandler->mIavFd >= 0)) {
            mpSharedRes->mIavFd = mpDSPHandler->mIavFd;
            mpSharedRes->mbIavInited = 1;
        }

        //try get dsp accelerator, now it would be mpeg4-idct-mc and rv40-mc hybird decoder
        //if (mpDSPHandler && mbParallelDecoder) {
            //hack assert here, amba's parallel decoder's name should be "amba-xxxx"
            //AM_ASSERT(!strncmp(mpDecoder->name, "amba", 4));
            AMLOG_INFO("@@ before CreateHybirdUDEC, pic w %d, h %d, mpDecoder->name %s.\n", mpCodec->width, mpCodec->height, mpDecoder->name);
            mpAccelerator = mpDSPHandler->CreateHybirdUDEC(mpCodec, mpSharedRes, error);
        //}

        AM_ASSERT(error == 0);
        if (error != 0) {
            AMLOG_ERROR("DSP related error.\n");
            return ME_ERROR;
        }

        //create pure sw decoding buffer pool
        //if (mpDSPHandler) {

        //ffmpeg seems have bugs, set rv40 to CODEC_FLAG_EMU_EDGE, to do
        if (mpCodec->codec_id == CODEC_ID_RV40 || mpCodec->codec_id == CODEC_ID_AMBA_RV40 || mpCodec->codec_id == CODEC_ID_AMBA_P_RV40)
            mpCodec->flags |= CODEC_FLAG_EMU_EDGE;

            AMLOG_INFO("@@ before mpDSPHandler->CreateBufferPool, mpAccelerator %p, mbNV12 %d.\n", mpAccelerator, mbNV12);
            AMLOG_INFO("@@ (mpCodec->flags & CODEC_FLAG_EMU_EDGE) %d, mpCodec->flags %x.\n",  (mpCodec->flags & CODEC_FLAG_EMU_EDGE), mpCodec->flags);
            if ((mpCodec->flags & CODEC_FLAG_EMU_EDGE) || !mbNV12) {
                mpBufferPool = (IBufferPool *)mpDSPHandler->CreateBufferPool(mpCodec->width, mpCodec->height, false, mpAccelerator);//use buffer without ext-edge
            } else {
                mpBufferPool = (IBufferPool *)mpDSPHandler->CreateBufferPool(mpCodec->width, mpCodec->height, true, mpAccelerator);//use buffer with ext-edge
            }
            AMLOG_INFO("@@ mpBufferPool %p.\n", mpBufferPool);
        //}

        AM_ASSERT(mpBufferPool);
        mbPoolAlloc = true;
        if (!mpDSPHandler->mfpDecoderGetBuffer || !mpDSPHandler->mfpDecoderReleaseBuffer) {
            //a5s legacy
            #if TARGET_USE_AMBARELLA_A5S_DSP
            #else
            AM_ERROR("target is not A5S? should not come here.\n");
            AM_ASSERT(0);
            #endif
            if (mbNV12) {
                mpCodec->opaque = (void *)this;//(void *)mpOutput;//(void *)(mpOutput->mpBufferPool);
                mpCodec->get_buffer = DecoderGetBuffer;
                mpCodec->release_buffer = DecoderReleaseBuffer;
            }
        } else {
            mpCodec->opaque = (void *)mpBufferPool;
            //mpCodec->get_buffer = mpDSPHandler->mfpDecoderGetBuffer;
            //mpCodec->release_buffer = mpDSPHandler->mfpDecoderReleaseBuffer;
        }

        //AM_ASSERT(mpSharedRes->mUdecState == _UDEC_IDLE);
        if (mpAccelerator->decode_type == UDEC_SW || mpAccelerator->decode_type == UDEC_RV40) {
            mpSharedRes->get_outpic = 1;
            mpDSPHandler->StartUdec(mpAccelerator);
            pthread_mutex_lock(&mpSharedRes->mMutex);
            GetUdecState(mpDSPHandler->mIavFd, &mpSharedRes->udec_state, &mpSharedRes->vout_state, &mpSharedRes->error_code);
            pthread_mutex_unlock(&mpSharedRes->mMutex);
            AM_ASSERT(IAV_UDEC_STATE_RUN == mpSharedRes->udec_state);
            PostEngineMsg(IEngine::MSG_NOTIFY_UDEC_IS_RUNNING);
            mbStreamStart = true;
        } else {
            AM_ASSERT(mpAccelerator->decode_type == UDEC_MP4S);
            mpSharedRes->get_outpic = (1==mpSharedRes->ppmode);
        }

        /*To fix bug#1435, with resolution 560x420, bitrate485 kbps, has performance issue
        *       so modify the judgement resolution to 560x420
        */
        if (mpCodec->codec_id == CODEC_ID_AMBA_P_RV40 && (mpCodec->width*mpCodec->height >= 560*420)) {
            //tmp modify, for rv40 performance
            AMLOG_INFO("try Low delay mode RV40, for current poor performance.\n");
            mpSharedRes->mForceLowdelay = 1;
        }

        if (mpSharedRes->mForceLowdelay) {
            mpCodec->flags |= CODEC_FLAG_LOW_DELAY;
            AMLOG_INFO("try force low delay mode (skip B picture).\n");
        }

    } else if (*pFormat->pMediaType == GUID_Audio) {
        AMLOG_INFO("Adecoder SetInputFormat.\n");
        mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
        if(mpDecoder == NULL) {
            AM_ERROR("ffmpeg audio decoder not found, mpCodec->codec_id=%x. \n",mpCodec->codec_id);
            return ME_ERROR;
        }
    }else if (*pFormat->pMediaType == GUID_Subtitle) {
        //find the subtitle stream codec. added by cx
        AMLOG_INFO("Sdecoder SetInputFormat.\n");
        mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
        if(mpDecoder == NULL) {
            AMLOG_ERROR("ffmpeg subtitle decoder not found, mpCodec->codec_id=%x. \n",mpCodec->codec_id);
            return ME_ERROR;
        }
        AM_U64 nPTSNum = 0;
        AM_U64 nPTSDen = 0;
        AM_ASSERT(mpStream->time_base.num ==1);
        AMLOG_INFO(" [PTS] FFDecoder:  Subtitle stream num %d, den %d.\n", mpStream->time_base.num, mpStream->time_base.den);
        nPTSNum = (AM_U64)(mpStream->time_base.num) * mpSharedRes->clockTimebaseDen;
        nPTSDen = (AM_U64)(mpStream->time_base.den) * mpSharedRes->clockTimebaseNum;
        if ((nPTSNum % nPTSDen) == 0) {
            nPTSNum /= nPTSDen;
            nPTSDen = 1;
        }
        mPTSSubtitle_Num = nPTSNum;
        mPTSSubtitle_Den = nPTSDen;
        AMLOG_INFO("    Subtitle convertor num %u, den %u\n", mPTSSubtitle_Num, mPTSSubtitle_Den);
    }
    mpOutput->mMediaFormat.format = (AM_INTPTR)mpDSPHandler;
    AM_ERR err = mpOutput->SetOutputFormat();
    if (err != ME_OK)
        return err;

    if (mpBufferPool) {
        //only video allocate buffer here
        AM_ASSERT(*pFormat->pMediaType == GUID_Video);
        mpOutput->SetBufferPool(mpBufferPool);
    }

    if (OpenDecoder() != ME_OK) {
        AMLOG_ERROR("CFFMpegDecoder OpenDecoder error.\n");
        return ME_ERROR;
    } else {
        AMLOG_VERBOSE("CFFMpegDecoder OpenDecoder OK.\n");
    }

    return ME_OK;
}

AM_ERR CFFMpegDecoder::OpenDecoder()
{
        if (mbOpen)
            return ME_OK;

        // if the input ES type can't find a decoder, mpDecoder=NULL
        if(mpDecoder == NULL)
        {
            AMLOG_INFO("CFFMpegDecoder Can't find codec for this stream\n");
            return ME_ERROR;
        }

    AMLOG_INFO("**OpenDecoder %s\n", mpDecoder->name);

    int rval = avcodec_open(mpCodec, mpDecoder);
    if (rval < 0) {
        AM_ERROR("avcodec_open failed %d\n", rval);
        return ME_ERROR;
    }

    AMLOG_INFO("**OpenDecoder %s success.\n", mpDecoder->name);

    mbOpen = true;

    return ME_OK;
}

void CFFMpegDecoder::CloseDecoder()
{
    if (mbOpen) {
        mbOpen = false;
        avcodec_close(mpCodec);
    }
}

void CFFMpegDecoder::ErrorStop()
{
    mbError = true;
    PostEngineMsg(ME_ERROR);
}


#ifdef AM_DEBUG
void CFFMpegDecoder::PrintState()
{
    if (mpDecoder->type == CODEC_TYPE_VIDEO) {
        AMLOG_INFO("VDecoder: msState=%d, %d input data, %d free buffers.\n", msState, mpInput->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt());
    } else if (mpDecoder->type == CODEC_TYPE_AUDIO) {
        AMLOG_INFO("ADecoder: msState=%d, %d input data, %d free buffers.\n", msState, mpInput->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt());
    }else if (mpDecoder->type == CODEC_TYPE_SUBTITLE) {
        AMLOG_INFO("SDecoder: msState=%d, %d input data, %d free buffers.\n", msState, mpInput->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt());
    }
}
#endif

void CFFMpegDecoder::speedUp()
{
    AM_ASSERT(mpInput);
    if (!mpInput) {
        return;
    }

    AM_ASSERT(!mpBuffer);
    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    AMLOG_WARN("[Request speed up]: (ffmpeg_decoder), start purge input queue.\n");

    //purge input queue
    while (mpInput->PeekBuffer(mpBuffer)) {
        if (CBuffer::DATA == mpBuffer->GetType()) {
            mpBuffer->Release();
            mpBuffer = NULL;
            AMLOG_WARN("[Request speed up]: (ffmpeg_decoder), discard input data.\n");
        } else if (CBuffer::EOS == mpBuffer->GetType()) {
            AMLOG_WARN("[Request speed up]: get EOS buffer.\n");
            ProcessEOS();
            return;
        } else {
            AM_ERROR("BAD buffer type %d.\n", mpBuffer->GetType());
            mpBuffer->Release();
            mpBuffer = NULL;
        }
    }

    AMLOG_WARN("[Request speed up]: (ffmpeg_decoder) done.\n");

}

//-----------------------------------------------------------------------
//
// CFFMpegDecoderInput
//
//-----------------------------------------------------------------------
CFFMpegDecoderInput *CFFMpegDecoderInput::Create(CFilter *pFilter)
{
    CFFMpegDecoderInput *result = new CFFMpegDecoderInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CFFMpegDecoderInput::Construct()
{
    AM_ERR err = inherited::Construct(((CFFMpegDecoder*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

CFFMpegDecoderInput::~CFFMpegDecoderInput()
{
    AMLOG_DESTRUCTOR("~CFFMpegDecoderInput.\n");
}

AM_ERR CFFMpegDecoderInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    return ((CFFMpegDecoder*)mpFilter)->SetInputFormat(pFormat);
}

//-----------------------------------------------------------------------
//
// CFFMpegDecoderOutput
//
//-----------------------------------------------------------------------
CFFMpegDecoderOutput *CFFMpegDecoderOutput::Create(CFilter *pFilter)
{
    CFFMpegDecoderOutput *result = new CFFMpegDecoderOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CFFMpegDecoderOutput::Construct()
{
    return ME_OK;
}

CFFMpegDecoderOutput::~CFFMpegDecoderOutput()
{
    AMLOG_DESTRUCTOR("~CFFMpegDecoderOutput.\n");
}

AM_ERR CFFMpegDecoderOutput::SetOutputFormat()
{
    AVCodec *pDecoder = ((CFFMpegDecoder*)mpFilter)->mpDecoder;
    AVCodecContext *pCodec = ((CFFMpegDecoder*)mpFilter)->mpCodec;

    if (pDecoder->type == CODEC_TYPE_VIDEO) {
        mMediaFormat.pMediaType = &GUID_Decoded_Video;
        mMediaFormat.pSubType = &GUID_Video_YUV420NV12;
        mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Media;
        mMediaFormat.mDspIndex = ((CFFMpegDecoder*)mpFilter)->mDspIndex;
        CalcVideoDimension(pCodec);
        return ME_OK;
    }
    if (pDecoder->type == CODEC_TYPE_AUDIO) {
        mMediaFormat.pMediaType = &GUID_Decoded_Audio;
        //mMediaFormat.pSubType = &GUID_Video_YUV420NV12;
        mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Media;
        mMediaFormat.auSamplerate = pCodec->sample_rate;
        mMediaFormat.auChannels = pCodec->channels;
        mMediaFormat.auSampleFormat = pCodec->sample_fmt;
        mMediaFormat.isChannelInterleave = 0;
        return ME_OK;
    }
    //subtitle stream need new structure. added by cx
    if (pDecoder->type == CODEC_TYPE_SUBTITLE) {
        mMediaFormat.pMediaType = &GUID_Decoded_Subtitle;
        mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Media;
        return ME_OK;
    }

    else {
        AM_ERROR("Cannot decoder other format yet\n");
        return ME_ERROR;
    }
}

void CFFMpegDecoderOutput::CalcVideoDimension(AVCodecContext *pCodec)
{
    int w = pCodec->width;
    int h = pCodec->height;

    mMediaFormat.picWidth = w;
    mMediaFormat.picHeight = h;

    if (pCodec->codec_id == CODEC_ID_VP8) {
        w += 64;
        h += 64;
        mMediaFormat.picXoff = 32;
        mMediaFormat.picYoff = 32;
        AMLOG_INFO("CODEC_ID_VP8\n");
    }
    else if (!(pCodec->flags & CODEC_FLAG_EMU_EDGE)) {
        w += 32;
        h += 32;
        mMediaFormat.picXoff = 16;
        mMediaFormat.picYoff = 16;
        AMLOG_INFO("CODEC_FLAG_EMU_EDGE\n");
    }
    else {
        mMediaFormat.picXoff = 0;
        mMediaFormat.picYoff = 0;
    }

    mMediaFormat.picWidthWithEdge = w;
    mMediaFormat.picHeightWithEdge = h;

    w=(w+(CFFMpegDecoder::VIDEO_BUFFER_ALIGNMENT_WIDTH-1))&(~(CFFMpegDecoder::VIDEO_BUFFER_ALIGNMENT_WIDTH-1));
    h=(h+(CFFMpegDecoder::VIDEO_BUFFER_ALIGNMENT_HEIGHT-1))&(~(CFFMpegDecoder::VIDEO_BUFFER_ALIGNMENT_HEIGHT-1));

    mMediaFormat.bufWidth = w;
    mMediaFormat.bufHeight = h;
}

int CFFMpegDecoder::DecoderGetBuffer(AVCodecContext *s, AVFrame *pic)
{
    CBuffer *pBuffer=NULL;
    pic->opaque=NULL;
    CFFMpegDecoder *pFilter = (CFFMpegDecoder*)s->opaque;
    if(NULL==pFilter)
    {
        AM_WARNING("opaque warning!! %s,%d\n",__FILE__,__LINE__);
        return -EPERM;
    }
    IBufferPool *pBufferPool = pFilter->mpOutput->mpBufferPool;

    //AM_INFO("*****CFFMpegDecoder[video] DecoderGetBuffer callback.\n");

    if (!pBufferPool->AllocBuffer(pBuffer, 0)) {
        AM_ASSERT(0);
        return -1;
    }
    pBuffer->AddRef();
    if (!pFilter->mbParallelDecoder)
        pBuffer->mPTS = pFilter->mCurrInputTimestamp;

    CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pBuffer;
    pVideoBuffer->picXoff = pFilter->mpOutput->mMediaFormat.picXoff;
    pVideoBuffer->picYoff = pFilter->mpOutput->mMediaFormat.picYoff;

    //only nv12 format comes here
    if (pVideoBuffer->picXoff > 0) {
        pic->data[0] = (uint8_t*)pVideoBuffer->pLumaAddr +
            pVideoBuffer->picYoff * pVideoBuffer->fbWidth +
            pVideoBuffer->picXoff;
        pic->linesize[0] = pVideoBuffer->fbWidth;

        pic->data[1] = (uint8_t*)pVideoBuffer->pChromaAddr +
            (pVideoBuffer->picYoff / 2) * pVideoBuffer->fbWidth +
            pVideoBuffer->picXoff;
        pic->linesize[1] = pVideoBuffer->fbWidth;

        pic->data[2] = pic->data[1] + 1;
        pic->linesize[2] = pVideoBuffer->fbWidth;
    }
    else {
        AM_ASSERT(!pVideoBuffer->picYoff);
        pic->data[0] = (uint8_t*)pVideoBuffer->pLumaAddr;
        pic->linesize[0] = pVideoBuffer->fbWidth;

        pic->data[1] = (uint8_t*)pVideoBuffer->pChromaAddr;
        pic->linesize[1] = pVideoBuffer->fbWidth;

        pic->data[2] = pic->data[1] + 1;
        pic->linesize[2] = pVideoBuffer->fbWidth;
    }

    pic->age = 256*256*256*64;
    pic->type = FF_BUFFER_TYPE_USER;
    pic->opaque = (void*)pBuffer;
    pic->user_buffer_id = pVideoBuffer->buffer_id;
    //pic->fbp_id = pVideoBuffer->fbp_id;
    return 0;

}

void CFFMpegDecoder::DecoderReleaseBuffer(AVCodecContext *s, AVFrame *pic)
{
    CBuffer *pBuffer = (CBuffer*)pic->opaque;
    if(NULL==pBuffer)
    {
        AM_WARNING("opaque warning!! %s,%d\n",__FILE__,__LINE__);
        return;
    }
    pBuffer->Release();
    if (pic->type == DFlushing_frame) {
        //flushing, CBuffer will not send to video_renderer for render/release, so release here
        pBuffer->Release();
    }

    pic->data[0] = NULL;
    pic->type = 0;
}

