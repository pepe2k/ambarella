
/*
 * video_decoder_ffmpeg.cpp
 *
 * History:
 *    2011/5/27 - [Zhi He] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "vdecoder_ffmpeg"
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
#include "libavformat/avformat.h"
#include "libavcodec/amba_dsp_define.h"
}
#include <basetypes.h>
#include "iav_drv.h"

#include "amdsp_common.h"
#include "am_util.h"
#include "video_decoder_ffmpeg.h"

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

#define fake_addr 0x12345678
#define fake_stride 256

//ffmpeg related callback
static AM_INT _callback_decode(void* p_context, void* p_param)
{
    AM_INT ret;
    amba_decoding_accelerator_t* pac = (amba_decoding_accelerator_t*)p_context;
    if (!pac || !pac->p_opaque || !p_param) {
        AM_ASSERT(0);
        AM_ERROR("NULL pointer in _callback_decode, pac %p.\n", pac);
        return (-4);
    }
    IUDECHandler* pUdecHandler = (IUDECHandler*) pac->p_opaque;
    AM_ASSERT(pUdecHandler);

#ifdef AM_DEBUG
    iav_udec_decode_s debug_iav_s;
    udec_decode_t debug_ff_t;
    //check iav's struct is sync with ffmpeg's struct
    AM_ASSERT(sizeof(debug_iav_s) == sizeof(debug_ff_t));
    AM_ASSERT(sizeof(debug_iav_s.u) == sizeof(debug_ff_t.uu));
    AM_ASSERT(sizeof(debug_iav_s.u.mp4s) == sizeof(debug_ff_t.uu.mpeg4s));
    AM_ASSERT(sizeof(debug_iav_s.u.rv40) == sizeof(debug_ff_t.uu.rv40));
    AM_ASSERT(sizeof(debug_iav_s.u.fifo) == sizeof(debug_ff_t.uu.fifo));
#endif

    AM_DEBUG_INFO("before pUdecHandler->AccelDecoding.\n");
    ret = pUdecHandler->AccelDecoding(pac->decode_id, p_param, pac->decode_type == UDEC_RV40);
    if (ret == ME_UDEC_ERROR) {
        AM_PRINTF("detect udec error in _callback_decode.\n");
        return (-3);//hard code here
    }
    AM_DEBUG_INFO("pUdecHandler->AccelDecoding done.\n");

    return 0;
}

static int _callback_get_decoded_frame(void* p_context, void* p_param)
{
    AM_INT ret;
    AVFrame* pic = (AVFrame*)p_param;
    amba_decoding_accelerator_t* pac = (amba_decoding_accelerator_t*)p_context;
    if (!pac || !pac->p_opaque || !p_param) {
        AM_ASSERT(0);
        AM_ERROR("NULL pointer in _callback_decode, pac %p.\n", pac);
        return (-4);
    }
    IUDECHandler* pUdecHandler = (IUDECHandler*) pac->p_opaque;
    AM_ASSERT(pUdecHandler);

    AM_DEBUG_INFO("before pUdecHandler->GetDecodedFrame.\n");
    ret = pUdecHandler->GetDecodedFrame(pac->decode_id, pic->user_buffer_id, pic->is_eos);
    if (ret == ME_UDEC_ERROR) {
        AM_PRINTF("detect udec error in _callback_get_decoded_frame.\n");
        return (-3);//hard code here
    }
    AM_DEBUG_INFO("pUdecHandler->GetDecodedFrame done.\n");
    return 0;
}

static int _callback_request_inputbuffer(void* p_context, int num_of_frame, unsigned char* start)
{
    AM_INT ret;
    AM_UINT size;
    amba_decoding_accelerator_t* pac = (amba_decoding_accelerator_t*)p_context;
    if (!pac || !pac->p_opaque || !start) {
        AM_ASSERT(0);
        AM_ERROR("NULL pointer in _callback_decode, pac %p.\n", pac);
        return (-4);
    }
    IUDECHandler* pUdecHandler = (IUDECHandler*) pac->p_opaque;
    AM_ASSERT(pUdecHandler);

    AM_ASSERT(num_of_frame == 1);
    size = pac->amba_idct_buffer_size * ((unsigned int)num_of_frame);

    AM_DEBUG_INFO("before pUdecHandler->RequestInputBuffer.\n");
    ret = pUdecHandler->RequestInputBuffer(pac->decode_id, size, start, (unsigned int)num_of_frame);
    if (ret == ME_UDEC_ERROR) {
        AM_PRINTF("detect udec error in _callback_request_inputbuffer.\n");
        return (-3);//hard code here
    }
    AM_DEBUG_INFO("pUdecHandler->RequestInputBuffer done.\n");
    return 0;
}

static int _callback_get_buffer(AVCodecContext *s, AVFrame *pic)
{
    CBuffer *pBuffer=NULL;
    AM_INT ret;
    pic->opaque=NULL;
    VideoDecoderFFMpeg* pDecoder = (VideoDecoderFFMpeg*)s->opaque;
    if (!pDecoder) {
        AM_ASSERT(0);
        AM_ERROR("NULL pointer s->opaque in get_buffer callback.\n");
        return (-1);
    }

    COutputPin* pOutPin;
    IUDECHandler* pUdecHandler;

    pDecoder->GetHandler(pUdecHandler, pOutPin);
    if (!pUdecHandler || !pOutPin) {
        AM_ASSERT(0);
        AM_ERROR("NULL pointer in VideoDecoderFFMpeg outputpin %p, udecHandler %p.\n", pOutPin, pUdecHandler);
        return (-2);
    }

    //get CBuffer(CVideoBuffer)
    if (!pOutPin->AllocBuffer(pBuffer, 0)) {
        AM_ASSERT(0);
        AM_ERROR("cannot allocate CBuffer in outputpin?\n");
        return (-4);
    }
    pBuffer->AddRef();
    CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pBuffer;

    //get frame buffer from ucode
    ret = pUdecHandler->RequestFrameBuffer(pDecoder->GetDecoderInstance(), pVideoBuffer);
    AM_VERBOSE("*****request frame buffer %p, %p, pic %p, buffer id %d, real buffer id %d.\n", pBuffer, pVideoBuffer, pic, pVideoBuffer->buffer_id, pVideoBuffer->real_buffer_id);

    if (ret == ME_UDEC_ERROR) {
        AM_PRINTF("detect udec error in _callback_get_buffer.\n");
        return (-3);//hard code here
    }

    //pts
    pBuffer->SetPTS((AM_U64)s->reordered_opaque);
    pBuffer->SetType(CBuffer::DATA);

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
    } else {
        AM_ASSERT(!pVideoBuffer->picYoff);
        pic->data[0] = (uint8_t*)pVideoBuffer->pLumaAddr;
        pic->linesize[0] = pVideoBuffer->fbWidth;

        pic->data[1] = (uint8_t*)pVideoBuffer->pChromaAddr;
        pic->linesize[1] = pVideoBuffer->fbWidth;

        pic->data[2] = pic->data[1] + 1;
        pic->linesize[2] = pVideoBuffer->fbWidth;
    }
    pic->user_buffer_id = pVideoBuffer->buffer_id;
    pic->age = 256;
    pic->type = FF_BUFFER_TYPE_USER;
    pic->opaque = (void*)pBuffer;
    pic->real_fb_id = pVideoBuffer->real_buffer_id;
    return 0;

}

static void _callback_release_buffer(AVCodecContext *s, AVFrame *pic)
{
    CBuffer *pBuffer = (CBuffer*)pic->opaque;
    if(NULL==pBuffer)
    {
        AM_WARNING("opaque warning!! %s,%d\n",__FILE__,__LINE__);
        return;
    }
    AM_VERBOSE("*****release frame buffer %p, pic %p, buf id %d, real id %d.\n", pBuffer, pic, ((CVideoBuffer*)pBuffer)->buffer_id, ((CVideoBuffer*)pBuffer)->real_buffer_id);

    pBuffer->Release();
    if (pic->type == DFlushing_frame) {
        //flushing, CBuffer will not send to video_renderer for render/release, so release here
        pBuffer->Release();
    }

    pic->data[0] = NULL;
    pic->type = 0;
}

static int _callback_fake_get_buffer(AVCodecContext *s, AVFrame *pic)
{
    pic->data[0] = pic->data[1] = pic->data[2] = (unsigned char*)fake_addr;
    pic->linesize[0] = pic->linesize[1] = pic->linesize[2] = fake_stride;

    pic->age = 256;
    pic->type = FF_BUFFER_TYPE_USER;
    pic->opaque = NULL;
    return 0;
}

static void _callback_fake_release_buffer(AVCodecContext *s, AVFrame *pic)
{
    pic->data[0] = NULL;
    pic->type = 0;
}
//-----------------------------------------------------------------------
//
// CVideoDecoderFFMpeg
//
//-----------------------------------------------------------------------

GDecoderParser gVideoFFMpegDecoder = {
    "Decoder-FFMpeg-VG",
    CVideoDecoderFFMpeg::Create,
    CVideoDecoderFFMpeg::ParseBuffer,
    CVideoDecoderFFMpeg::ClearParse,
};

//
AM_INT CVideoDecoderFFMpeg::ParseBuffer(const CGBuffer* gBuffer)
{
    STREAM_TYPE type = gBuffer->GetStreamType();
    if(type != STREAM_VIDEO)
        return -10;

    OWNER_TYPE otype = gBuffer->GetOwnerType();
    AM_ASSERT(otype == DEMUXER_FFMPEG);

    if(*(gBuffer->codecType) != GUID_AmbaVideoDecoder)
        return -20;

    mConfigIndex  = gBuffer->GetIdentity();
    return 90;
}

AM_ERR CVideoDecoderFFMpeg::ClearParse()
{
    return ME_OK;
}
//-----------------------------------------------------------------------
//
// CVideoDecoderDsp
IGDecoder* CVideoDecoderFFMpeg::Create(IFilter* pFilter, CGConfig* pconfig)
{
    CVideoDecoderDsp *result = new CVideoDecoderDsp(pFilter, pconfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
        }
    return result;
}

CVideoDecoderFFMpeg::CVideoDecoderFFMpeg(IFilter* pFilter, CGConfig* pconfig):
    inherited("videoDecoderFFmpeg"),
    //msStatus(DecoderStatus_notInited),
    mpFilter(pFilter),
    mpGConfig(pconfig),
    mpOutputPin(NULL),
    mpSharedRes(pSharedRes),
    mpUDECHandler(NULL),
    mUdecIndex(eInvalidUdecIndex),
    mbExtEdge(0),
    mpAccelerator(NULL),
    mpCodec(NULL),
    mpStream(NULL),
    tobeDecodedBuffer(NULL),
    mpBuffer(NULL),
    mbOpened(0),
    mbUseDSPAcceleration(0),
    mDecoderType(DECType_notInited),
    mDecoderFormat(UDEC_NONE),
    mCurrInputTimestamp(0),
    decodedFrame(0),
    droppedFrame(0),
    totalTime(0)
{
    mpConfig = &(mpGConfig->decoderConfig[mConfigIndex]);
    mpAVFormat = (AVFormatContext* )mpConfig->decoderEnv;
    mStreamIndex = mpConfig->opaqueEnv;

    mLogLevel = LogVerboseLevel;
    mLogOption = LogCMD | LogState;
}

AM_ERR CVideoDecoderFFMpeg::Construct()
{
    AM_ERR err;
    err = inherited::Construct();
    if (err != ME_OK)
    {
        AM_ERROR("VideoDecoderFFMpeg ,inherited::Construct fail err = %d .\n", err);
        return err;
    }
    ConstructContext();
    PostCmd(CMD_RUN);
    DSetModuleLogConfig(LogModuleVideoDecoderFFMpeg);
    return ME_OK;
}

AM_ERR CVideoDecoderFFMpeg::ConstructContext(DecoderConfig* configData)
{
    enum CodecID ori_codec_id;
    AM_ERR err;

    if (!configData || !configData->pHandle || !configData->p_filter || !configData->p_outputpin ||!mpSharedRes) {
        AM_ASSERT(0);
        AMLOG_ERROR("NULL pointer in VideoDecoderFFMpeg::ConstructContext.\n");
        return ME_BAD_PARAM;
    }

    mpOutputPin = (COutputPin*)configData->p_outputpin;
    mpFilter = (CInterActiveFilter*)configData->p_filter;

    mpStream = (AVStream*)configData->pHandle;
    mpCodec = (AVCodecContext*)mpStream->codec;

    if (!mpCodec) {
        AM_ASSERT(0);
        AMLOG_ERROR("NULL pointer(mpCodec) in VideoDecoderFFMpeg::ConstructContext.\n");
        return ME_BAD_PARAM;
    }

    ori_codec_id = mpCodec->codec_id;
    AM_ASSERT(ori_codec_id != CODEC_ID_NONE);
    AM_ASSERT((mpCodec->codec_type == CODEC_TYPE_VIDEO));

    //select FFMpeg decoder
    if (mpSharedRes->mDecoderSelect == 0) {
        //find parallel nv12 format codec first
        AMLOG_INFO("Find parallel nv12 decoder first.\n");
        mpCodec->codec_id = (enum CodecID)((int)ori_codec_id + (int)CODEC_ID_PARALLEL_OFFSET);
        mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
        if (mpDecoder == NULL) {
            AMLOG_INFO("Cannot find a parallel nv12 decoder\n");
            mpCodec->codec_id = (enum CodecID)((int)ori_codec_id + (int)CODEC_ID_NV12_OFFSET);
            mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
            if(mpDecoder == NULL) {
                AMLOG_INFO("Cannot find a nv12 decoder\n");
                mpCodec->codec_id = ori_codec_id;
                mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
                if(mpDecoder == NULL) {
                    AM_ERROR("ffmpeg video decoder not found, mpCodec->codec_id=%x. \n",mpCodec->codec_id);
                    return ME_ERROR;
                } else {
                    mDecoderType = DECType_YUVPlane;
                }
            } else {
                mDecoderType = DECType_NV12DirectRendering;
            }
        } else {
            mDecoderType = DECType_PipelineNV12DirectRendering;
        }
    }else if (mpSharedRes->mDecoderSelect == 1) {
        AMLOG_INFO("Find nv12 decoder first.\n");
        mpCodec->codec_id = (enum CodecID)((int)ori_codec_id + (int)CODEC_ID_NV12_OFFSET);
        mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
        if(mpDecoder == NULL) {
            AMLOG_INFO("Cannot find a nv12 decoder\n");
            mpCodec->codec_id = ori_codec_id;
            mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
            if(mpDecoder == NULL) {
                AM_ASSERT(0);
                AMLOG_ERROR("ffmpeg video decoder not found, mpCodec->codec_id=%x. \n",mpCodec->codec_id);
                return ME_ERROR;
            } else {
                mDecoderType = DECType_YUVPlane;
            }
        } else {
            mDecoderType = DECType_NV12DirectRendering;
        }
    } else {
        AMLOG_INFO("Find original decoder first\n");
        mpCodec->codec_id = ori_codec_id;
        mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
        if(mpDecoder == NULL) {
            AM_ASSERT(0);
            AMLOG_ERROR("ffmpeg video decoder not found, mpCodec->codec_id=%x. \n",mpCodec->codec_id);
            return ME_ERROR;
        } else {
            mDecoderType = DECType_YUVPlane;
        }
    }

    //create DSP handler(UDEC and VOUT)
    if (mpSharedRes->udecHandler == NULL) {
        AM_ASSERT(!mpSharedRes->voutHandler);

        AMLOG_PRINTF("@@ before AM_CreateDSPHandler().\n");
        AM_CreateDSPHandler((void*)mpSharedRes);
        AMLOG_PRINTF("@@ after AM_CreateDSPHandler(), udecHandler %p, voutHandler %p.\n", mpSharedRes->udecHandler, mpSharedRes->voutHandler);

        AM_ASSERT(mpSharedRes->udecHandler);
        AM_ASSERT(mpSharedRes->voutHandler);
    }
    mpUDECHandler = mpSharedRes->udecHandler;

    if (!mpUDECHandler) {
        AMLOG_ERROR("must have error, NULL mpUDECHandler here?\n");
        return ME_ERROR;
    }
    AM_ASSERT(mpSharedRes->mbIavInited == 1);

    switch (mpCodec->codec_id) {
        case CODEC_ID_AMBA_P_RV40:
            mDecoderFormat = (DEC_HYBRID==mpSharedRes->decCurrType)?UDEC_RV40:UDEC_SW;
            break;

        case CODEC_ID_AMBA_P_MPEG4:
        case CODEC_ID_AMBA_P_H263:
        case CODEC_ID_AMBA_P_MSMPEG4V1:
        case CODEC_ID_AMBA_P_MSMPEG4V2:
        case CODEC_ID_AMBA_P_MSMPEG4V3:
        case CODEC_ID_AMBA_P_WMV1:
        case CODEC_ID_AMBA_P_H263I:
        case CODEC_ID_AMBA_P_FLV1:
            mDecoderFormat = (DEC_HYBRID==mpSharedRes->decCurrType)?UDEC_MP4S:UDEC_SW;
            break;

        //pure sw decoder, can use nv12 buffer
        case CODEC_ID_AMBA_RV40:
        case CODEC_ID_AMBA_MPEG4:
        case CODEC_ID_AMBA_H263:
        case CODEC_ID_AMBA_MSMPEG4V1:
        case CODEC_ID_AMBA_MSMPEG4V2:
        case CODEC_ID_AMBA_MSMPEG4V3:
        case CODEC_ID_AMBA_WMV1:
        case CODEC_ID_AMBA_H263I:
        case CODEC_ID_AMBA_FLV1:
            mDecoderFormat = UDEC_SW;
            break;

        //use pure sw decoder, use yuv420 buffer, cannot use nv12 buffer
        default:
            mDecoderFormat = UDEC_SW;
            break;
    }
    AMLOG_INFO("mpCodec->codec_id 0x%x, mDecoderType %d.\n", mpCodec->codec_id, mDecoderType);

    if (mDecoderType != DECType_YUVPlane || mDecoderFormat != UDEC_SW) {
        mpAccelerator = &mAccelerator;
        memset(mpAccelerator, 0x0, sizeof(mAccelerator));
    }
    AM_ASSERT(mDecoderType != DECType_notInited);

    //ffmpeg seems have bugs, set rv40 to CODEC_FLAG_EMU_EDGE, to do
    if (mDecoderFormat == UDEC_RV40)
        mpCodec->flags |= CODEC_FLAG_EMU_EDGE;

    if ((mpCodec->flags & CODEC_FLAG_EMU_EDGE) || !mDecoderType == DECType_YUVPlane) {
        mbExtEdge = 0;
    } else {
        mbExtEdge = 1;
    }

    AMLOG_INFO("mpAccelerator %p, decFormat %d, decType %d, extEdge %d.\n", mpAccelerator, mDecoderFormat, mDecoderType, mbExtEdge);

    //init udec instance
    err = mpUDECHandler->InitUDECInstance(mUdecIndex, &mpSharedRes->dspConfig, (void *)mpAccelerator, mDecoderFormat, mpCodec->width, mpCodec->height, mbExtEdge, true);
    if (err != ME_OK || mUdecIndex == eInvalidUdecIndex) {
        AMLOG_ERROR("InitUDECInstance fail, ret %d, udec_index %d.\n", err, mUdecIndex);
        return ME_OS_ERROR;
    }

    //assert user config ppmode, same as udec's ppmode
    AM_ASSERT(mpSharedRes->dspConfig.modeConfig.postp_mode == mpSharedRes->ppmode);

    if (mDecoderFormat == UDEC_MP4S && mpSharedRes->ppmode == 2) {
        AM_ASSERT(mDecoderType == DECType_PipelineNV12DirectRendering);
        mpSharedRes->get_outpic = 0;
        AMLOG_PRINTF("Hybird mpeg4, ppmode == 2, same with HW mode, ARM no need access frame buffer pool.\n");
    } else {
        mpSharedRes->get_outpic = 1;
        AMLOG_PRINTF("Create frame buffer pool, width %d, height %d, index %d.\n", mpCodec->width, mpCodec->height, mUdecIndex);
        mpUDECHandler->InitFrameBufferPool(mUdecIndex, mpCodec->width, mpCodec->height, 1, 0, mbExtEdge);
    }

    //set ffmpeg direct rendering
    switch (mpCodec->codec_id) {
        case CODEC_ID_AMBA_P_RV40:
            if (DEC_HYBRID==mpSharedRes->decCurrType) {
                mpCodec->p_extern_accelerator =(void*)(mpAccelerator);
                mpCodec->acceleration = _callback_decode;//blocked until dsp process done
                mpCodec->get_decoded_frame = NULL;
                mpCodec->request_inputbuffer = _callback_request_inputbuffer;
                mpCodec->extern_accelerator_type = accelerator_type_amba_hybirdrv40_mc;

                mpCodec->get_buffer = _callback_get_buffer;
                mpCodec->release_buffer = _callback_release_buffer;
            } else {
                mpCodec->p_extern_accelerator =(void*)(mpAccelerator);
                mpCodec->acceleration = NULL;
                mpCodec->get_decoded_frame = NULL;
                mpCodec->request_inputbuffer = NULL;
                mpCodec->extern_accelerator_type = accelerator_type_amba_sw_direct_rendering;

                mpCodec->get_buffer = _callback_get_buffer;
                mpCodec->release_buffer = _callback_release_buffer;
            }
            break;

        case CODEC_ID_AMBA_P_MPEG4:
        case CODEC_ID_AMBA_P_H263:
        case CODEC_ID_AMBA_P_MSMPEG4V1:
        case CODEC_ID_AMBA_P_MSMPEG4V2:
        case CODEC_ID_AMBA_P_MSMPEG4V3:
        case CODEC_ID_AMBA_P_WMV1:
        case CODEC_ID_AMBA_P_H263I:
        case CODEC_ID_AMBA_P_FLV1:
            if (DEC_HYBRID==mpSharedRes->decCurrType) {
                mpCodec->p_extern_accelerator =(void*)(mpAccelerator);

                mpCodec->acceleration = _callback_decode;
                mpCodec->request_inputbuffer = _callback_request_inputbuffer;
                //hybird mpeg4, dsp will take idct/mc, sw decoding need not video buffer
                mpCodec->get_buffer = _callback_fake_get_buffer;
                mpCodec->release_buffer = _callback_fake_release_buffer;

                mpCodec->extern_accelerator_type = accelerator_type_amba_hybirdmpeg4_idctmc;

                if (mpSharedRes->ppmode == 2) {
                    mpCodec->get_decoded_frame = NULL;
                } else if (mpSharedRes->ppmode == 1) {
                    mpCodec->get_decoded_frame = _callback_get_decoded_frame;
                } else {
                    AM_ASSERT(0);
                    AMLOG_ERROR("must not come here, error ppmode value %d.\n", mpSharedRes->ppmode);
                }
            } else {
                mpCodec->p_extern_accelerator =(void*)(mpAccelerator);

                mpCodec->acceleration = NULL;
                mpCodec->request_inputbuffer = NULL;
                mpCodec->get_decoded_frame = NULL;
                //hybird mpeg4, dsp will take idct/mc, sw decoding need not video buffer
                mpCodec->get_buffer = _callback_get_buffer;
                mpCodec->release_buffer = _callback_release_buffer;
                mpCodec->extern_accelerator_type = accelerator_type_amba_sw_direct_rendering;
            }
            break;

        //pure sw decoder, can use nv12 buffer
        case CODEC_ID_AMBA_RV40:
        case CODEC_ID_AMBA_MPEG4:
        case CODEC_ID_AMBA_H263:
        case CODEC_ID_AMBA_MSMPEG4V1:
        case CODEC_ID_AMBA_MSMPEG4V2:
        case CODEC_ID_AMBA_MSMPEG4V3:
        case CODEC_ID_AMBA_WMV1:
        case CODEC_ID_AMBA_H263I:
        case CODEC_ID_AMBA_FLV1:
            mpCodec->p_extern_accelerator =(void*)(mpAccelerator);

            mpCodec->acceleration = NULL;
            mpCodec->request_inputbuffer = NULL;
            mpCodec->get_decoded_frame = NULL;
            mpCodec->get_buffer = _callback_get_buffer;
            mpCodec->release_buffer = _callback_release_buffer;
            mpCodec->extern_accelerator_type = accelerator_type_amba_sw_direct_rendering;
            break;

        //use pure sw decoder, use yuv420 buffer, cannot use nv12 buffer
        default:
            mpCodec->p_extern_accelerator = NULL;
            mpCodec->acceleration = NULL;
            mpCodec->extern_accelerator_type = accelerator_type_none;
            mpCodec->request_inputbuffer = NULL;
            mpCodec->get_decoded_frame = NULL;
            break;
    }

    mpCodec->opaque = (void*)this;

    if (mDecoderFormat == UDEC_SW || mDecoderFormat == UDEC_RV40) {
        AM_ASSERT(mpSharedRes->udec_state != IAV_UDEC_STATE_RUN);
        mpUDECHandler->StartUdec(mUdecIndex, mDecoderFormat, (void*)(mpAccelerator));
        mpUDECHandler->GetUDECState(mUdecIndex, mpSharedRes->udec_state, mpSharedRes->vout_state, mpSharedRes->error_code);
    }

    AMLOG_INFO("**avcodec_open %s\n", mpDecoder->name);
    AM_INT rval = avcodec_open(mpCodec, mpDecoder);
    if (rval < 0) {
        AMLOG_ERROR("avcodec_open failed %d\n", rval);
        return ME_ERROR;
    }
    AMLOG_INFO("**avcodec_open %s success.\n", mpDecoder->name);

    msStatus = DecoderStatus_ready;
    mbOpened = 1;
    return ME_OK;
}

VideoDecoderFFMpeg::~VideoDecoderFFMpeg()
{
    DestroyDecoderContext();
}

void VideoDecoderFFMpeg::ConvertFrame(CVideoBuffer *pBuffer)
{
	AM_ASSERT(mFrame.data[0]);
	AM_ASSERT(mFrame.data[1]);
	AM_ASSERT(mFrame.data[2]);
	AM_ASSERT(mFrame.linesize[0]);
	AM_ASSERT(mFrame.linesize[1]);
	AM_ASSERT(mFrame.linesize[2]);

	//AMLOG_VERBOSE("mFrame.linesize[0] %d, mFrame.linesize[1] %d, mFrame.linesize[2] %d.\n", mFrame.linesize[0], mFrame.linesize[1], mFrame.linesize[2]);
	//AMLOG_VERBOSE("pBuffer->fbWidth %d, pBuffer->picHeight %d, pBuffer->picWidth %d.\n", pBuffer->fbWidth, pBuffer->picHeight, pBuffer->picWidth);

	AM_UINT i = 0, cnt;
	AM_U32  u, v;

	AM_U8* pdes = pBuffer->pLumaAddr + pBuffer->picYoff * pBuffer->fbWidth + pBuffer->picXoff;

	AM_U16* pu = (AM_U16*)mFrame.data[1];
	AM_U16* pv = (AM_U16*)mFrame.data[2];
	AM_U32* p = (AM_U32*)(pBuffer->pChromaAddr + (pBuffer->picYoff >> 1) * pBuffer->fbWidth + pBuffer->picXoff);

	if (mpCodec->pix_fmt == PIX_FMT_YUV420P || mpCodec->pix_fmt == PIX_FMT_YUVJ420P) {
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
		}
	}else if (mpCodec->pix_fmt == PIX_FMT_YUV422P || mpCodec->pix_fmt == PIX_FMT_YUVJ422P) {
		//AMLOG_VERBOSE("PIX_FMT_YUV422P.\n");

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
		AM_U8* pu_u8 = mFrame.data[1];
		AM_U8* pv_u8 = mFrame.data[2];
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
				p[cnt] = ((v & 0xff) << 8) | ((v & 0xff00) << 16) | (u & 0xff) | ((u & 0xff00) << 8);;
			}
			p_u += pBuffer->fbWidth >> 2;
			p_v += pBuffer->fbWidth >> 2;
			p += pBuffer->fbWidth>>2;
		}
	} else {
		AM_ERROR("not support mpCodec->pix_fmt %d.\n", mpCodec->pix_fmt);
	}
      //update pic_size
      //AM_INFO("%dx%d\n",pBuffer->picWidth,pBuffer->picHeight);
       pBuffer->picWidth = mpCodec->coded_width;
       pBuffer->picHeight = mpCodec->coded_height;
}
void VideoDecoderFFMpeg::planar2x(AM_U8* src, AM_U8* des, AM_INT src_stride, AM_INT des_stride, AM_UINT width, AM_UINT height)
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

bool VideoDecoderFFMpeg::SendEOS()
{
    CBuffer *pBuffer;
    AM_ASSERT(mpOutputPin);
    if (!mpOutputPin) {
        AMLOG_ERROR("NULL pointer(mpOutputPin) in VideoDecoderFFMpeg::SendEOS.\n");
        return false;
    }

    if (!mpOutputPin->AllocBuffer(pBuffer)) {
        AM_ASSERT(0);
        AMLOG_ERROR("AllocBuffer fail in VideoDecoderFFMpeg::SendEOS.\n");
        return false;
    }

    pBuffer->SetType(CBuffer::EOS);
    mpOutputPin->SendBuffer(pBuffer);

    return true;
}

void VideoDecoderFFMpeg::SendAllVideoPictures()
{
    AM_INT frame_finished, ret;
    AM_UINT udec_state, vout_state, error_code;
    AVPacket packet;
    packet.data = NULL;
    packet.size = 0;
    packet.flags = 0;
    CBuffer *pOutputBuffer = NULL;
    CVideoBuffer *pVideoBuffer = NULL;
    AM_ASSERT(mpUDECHandler);

    AMLOG_INFO("VideoDecoderFFMpeg::SendAllVideoPictures, start get out all pictures from decoder.\n");
    while(1) {
        AMLOG_INFO("loop start:\n");
        ret = avcodec_decode_video2(mpCodec, &mFrame, &frame_finished, &packet);

        if(0>=ret)//all frames out
            break;

        if (frame_finished) {

            if (mDecoderType == DECType_PipelineNV12DirectRendering || mDecoderType == DECType_NV12DirectRendering) {
                pOutputBuffer = (CBuffer *)(mFrame.opaque);
                AM_ASSERT(pOutputBuffer);
                if(pOutputBuffer)
                {
                    if (mDecoderType == DECType_PipelineNV12DirectRendering) {
                        pOutputBuffer->mPTS = mFrame.pts;
                    }
                    mpOutputPin->SendBuffer(pOutputBuffer);
                }
            } else {
                if (!mpOutputPin->AllocBuffer(pOutputBuffer, 0)) {
                    AM_ASSERT(0);
                    AMLOG_ERROR("VideoDecoderFFMpeg::SendAllVideoPictures get output buffer fail, exit.\n");
                    return;
                }
                pOutputBuffer->mPTS = mFrame.reordered_opaque;
                CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pOutputBuffer;
                ConvertFrame(pVideoBuffer);
                mpOutputPin->SendBuffer(pOutputBuffer);
            }
            decodedFrame ++;
            AMLOG_VERBOSE("CFFMpegDecoder[video] eos send a decoded frame done.\n");
        }
    }
    AMLOG_INFO("VideoDecoderFFMpeg::SendAllVideoPictures, end.\n");
}

AM_ERR VideoDecoderFFMpeg::ProcessEOS()
{
    AM_ERR err;
    AM_INT frame_finished, ret;
    AVPacket * mpPacket = NULL;
    CBuffer* pOutputBuffer = NULL;
    CVideoBuffer *pVideoBuffer = NULL;
    AM_ASSERT(mpUDECHandler);
    //AM_ASSERT(mpBuffer);
    //AM_ASSERT(mpBuffer->GetType() == CBuffer::EOS);

    //if (!mpBuffer) {
    //    AMLOG_ERROR("NULL pointer(pBuffer) in VideoDecoderFFMpeg::ProcessEOS.\n");
    //    return ME_BAD_PARAM;
    //}

    //handle eos here
    //if (mpBuffer->GetType() == CBuffer::EOS) {

        //check parameters
        AM_ASSERT(mpOutputPin);
        if (!mpOutputPin) {
            AMLOG_ERROR("NULL pointer(mpOutputPin) in VideoDecoderFFMpeg::ProcessEOS.\n");
            return ME_ERROR;
        }

        AMLOG_INFO("before SendAllVideoPictures, mpCodec->codec->capabilities 0x%x.\n", mpCodec->codec->capabilities);
        //send all remaining video buffer to downstream filters
        if (mpCodec->codec->capabilities & CODEC_CAP_DELAY) {
            SendAllVideoPictures();
        }
/*    AMLOG_INFO("after SendAllVideoPictures, before SendEOS.\n");

        //send eos buffer to down-stream filters
        if (SendEOS()) {
            AMLOG_INFO("SendEOS done.\n");
            return ME_OK;
        } else {
            AM_ASSERT(0);
            AMLOG_ERROR("SendEOS fail in VideoDecoderFFMpeg.\n");
            return ME_ERROR;
        }*/
    //}

    AMLOG_VERBOSE(" VideoDecoderFFMpeg::ProcessEOS end.\n");
    return ME_OK;
}

AM_ERR VideoDecoderFFMpeg::ProcessBuffer()
{
    AM_ERR err;
    AM_INT frame_finished, ret;
    AVPacket * mpPacket = NULL;
    CBuffer* pOutputBuffer = NULL;
    CVideoBuffer *pVideoBuffer = NULL;
    AM_ASSERT(mpUDECHandler);
    AM_ASSERT(mpBuffer);
    AM_ASSERT(mpBuffer->GetType() == CBuffer::DATA);

    if (!mpBuffer) {
        AMLOG_ERROR("NULL pointer(pBuffer) in VideoDecoderFFMpeg::ProcessBuffer.\n");
        return ME_BAD_PARAM;
    }

    if (mpBuffer->GetType() == CBuffer::DATA) {
        mpPacket = (AVPacket*)((AM_U8*)mpBuffer + sizeof(CBuffer));

        if (mDecoderType != DECType_PipelineNV12DirectRendering) {
            if (mpPacket->pts >= 0) {
                mCurrInputTimestamp = mpPacket->pts;
                AMLOG_PTS(" VideoDecoderFFMpeg::Decode, recieve a PTS, mpPacket->pts=%llu.\n",mpPacket->pts);
            }
            else {
                mCurrInputTimestamp = 0;
                AMLOG_PTS(" VideoDecoderFFMpeg::Decode, has no pts.\n");
            }
            mpCodec->reordered_opaque = mCurrInputTimestamp;
        }

        AMLOG_VERBOSE("start decoding video packet, data %p, size %d.\n", mpPacket->data, mpPacket->size);
        if ((mpPacket->data == NULL) && (mpPacket->size == 0)) {
            AM_ASSERT(0);
            AMLOG_ERROR(" invalid avpacket.\n");
            return ME_BAD_PARAM;
        }

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

        AMLOG_VERBOSE(" VideoDecoderFFMpeg::Decode, decoding done, ret %d.\n", ret);

        if (!(decodedFrame & 0x1f) && decodedFrame)
            AMLOG_PERFORMANCE("video [%d] decoded [%d] dropped frames' average = %d uS  \n", decodedFrame, droppedFrame, totalTime/(decodedFrame+droppedFrame));

        if (ret < 0) {
            AMLOG_PRINTF("--- while decoding video---, err=%d.\n",ret);
            droppedFrame ++;
        } else if (frame_finished) {
            if (mDecoderType == DECType_PipelineNV12DirectRendering || mDecoderType == DECType_NV12DirectRendering) {
                pOutputBuffer = (CBuffer *)(mFrame.opaque);
                AM_ASSERT(pOutputBuffer);
                AM_ASSERT(mpOutputPin);
                if(pOutputBuffer)
                {
                    AMLOG_VERBOSE("*****decoded done %p, buf id %d, real id %d.\n", pOutputBuffer, ((CVideoBuffer*)pOutputBuffer)->buffer_id, ((CVideoBuffer*)pOutputBuffer)->real_buffer_id);
                    if (mDecoderType == DECType_PipelineNV12DirectRendering) {
                        pOutputBuffer->mPTS = mFrame.pts;
                    }
                    mpOutputPin->SendBuffer(pOutputBuffer);
                }
            } else {

                //chech valid, for sw decoding and convert from yuv420p to nv12 format
                if (!mFrame.data[0] || !mFrame.data[1] || !mFrame.data[2] || !mFrame.linesize[0] || !mFrame.linesize[1] || !mFrame.linesize[2] ) {
                    AM_ASSERT(0);
                    AMLOG_ERROR("get not valid frame, not enough memory?, or not-supported pix_fmt %d.\n", mpCodec->pix_fmt);
                    return ME_NO_MEMORY;
                }

                if (!mpOutputPin->AllocBuffer(pOutputBuffer, 0)) {
                    AM_ASSERT(0);
                    AMLOG_ERROR("mpOutputPin->AllocBuffer fail.\n");
                    return ME_ERROR;
                }
                pOutputBuffer->mPTS = mFrame.reordered_opaque;
                pVideoBuffer = (CVideoBuffer*)pOutputBuffer;
                ConvertFrame(pVideoBuffer);
                pOutputBuffer->SetType(CBuffer::DATA);
                mpOutputPin->SendBuffer(pOutputBuffer);
            }
            decodedFrame ++;
            AMLOG_VERBOSE(" VideoDecoderFFMpeg::Decode, send a decoded frame done.\n");
        }
    }

    AMLOG_VERBOSE(" VideoDecoderFFMpeg::ProcessBuffer end.\n");
    return ME_OK;
}

void VideoDecoderFFMpeg::Flush()
{
    mpWorkQ->SendCmd(CMD_FLUSH);
}

void VideoDecoderFFMpeg::Stop()
{
    mpWorkQ->SendCmd(CMD_STOP);
}

void VideoDecoderFFMpeg::DoFlush()
{
    AM_ASSERT(mpCodec);
    AM_ASSERT(!mpBuffer);
    msStatus = DecoderStatus_busyFlushing;
    avcodec_flush_buffers(mpCodec);
    if (tobeDecodedBuffer) {
        tobeDecodedBuffer->Release();
        tobeDecodedBuffer = NULL;
    }
    msStatus = DecoderStatus_ready;
}

void VideoDecoderFFMpeg::DoStop()
{
    DestroyDecoderContext();
}

AM_UINT VideoDecoderFFMpeg::GetDecoderStatus()
{
    return msStatus;
}

//direct rendering
void VideoDecoderFFMpeg::SetOutputPin(void* pOutPin)
{
    mpOutputPin = (COutputPin*)pOutPin;
    msStatus = DecoderStatus_ready;
}

void* VideoDecoderFFMpeg::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IActiveObject)
        return (IActiveObject*)this;
    return inherited::GetInterface(refiid);
}

void VideoDecoderFFMpeg::DestroyDecoderContext()
{
    msStatus = DecoderStatus_notInited;
    if (mbOpened) {
        mbOpened = 0;
        //clear pipeline
        avcodec_close(mpCodec);
    }

    if (mpUDECHandler) {
        mpUDECHandler->ReleaseUDECInstance(mUdecIndex);
        mUdecIndex = eInvalidUdecIndex;
    }

    mpUDECHandler = NULL;
    mpOutputPin = NULL;
}

AM_ERR VideoDecoderFFMpeg::FillEOS()
{
    mpWorkQ->SendCmd(CMD_EOS);
    return ME_OK;
}

AM_ERR VideoDecoderFFMpeg::Decode(CBuffer* pBuffer)
{
    AM_ASSERT(msStatus == DecoderStatus_ready);
    AM_ASSERT(!tobeDecodedBuffer);
    AM_ASSERT(mpCodec);
    AM_ASSERT(mpDecoder);
    AM_ASSERT(mpCodec->codec);

    AMLOG_VERBOSE("state decode, %p.\n", pBuffer);
    if (msStatus == DecoderStatus_ready) {
        tobeDecodedBuffer = pBuffer;
        msStatus = DecoderStatus_busyDecoding;
        mpWorkQ->PostMsg(CMD_DECODE);
        AMLOG_VERBOSE("decode done.\n");
        return ME_OK;
    } else if (msStatus == DecoderStatus_udecError) {
        pBuffer->Release();
        return ME_UDEC_ERROR;
    } else {
        AMLOG_ERROR("must have errors, should not comes here.\n");
        return ME_ERROR;
    }

}

AM_ERR VideoDecoderFFMpeg::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("**VideoDecoderFFMpeg cmd %d.\n", cmd.code);
    AM_ERR err = ME_OK;
    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            DoStop();
            CmdAck(ME_OK);
            break;

        case CMD_PAUSE:
            break;

        case CMD_RESUME:
            break;

        case CMD_FLUSH:
            DoFlush();
            CmdAck(ME_OK);
            break;

        case CMD_DECODE:
            AM_ASSERT(!mpBuffer);
            AM_ASSERT(tobeDecodedBuffer);
            AM_ASSERT(tobeDecodedBuffer->GetType() == CBuffer::DATA);
            AMLOG_DEBUG("mpBuffer %p, tobeDecodedBuffer %p.\n", mpBuffer, tobeDecodedBuffer);
            mpBuffer = tobeDecodedBuffer;
            tobeDecodedBuffer = NULL;

            if (mpBuffer->GetType() == CBuffer::DATA) {
                msStatus = DecoderStatus_busyDecoding;
                err = ProcessBuffer();
            } /*else if (mpBuffer->GetType() == CBuffer::EOS) {
                msStatus = DecoderStatus_busyHandlingEOS;
                err = ProcessEOS();
            }*/ else {
                err = ME_ERROR;
            }

            mpBuffer->Release();
            mpBuffer = NULL;

            AM_MSG msg;

            if (err == ME_OK) {
                msStatus = DecoderStatus_ready;
                msg.code = GFilter::MSG_READY;
                mpFilter->PostMsgToFilter(msg);
            } else if (err == ME_UDEC_ERROR) {
                AM_ASSERT(mpFilter);
                msg.code = GFilter::MSG_UDEC_ERROR;
                mpFilter->PostMsgToFilter(msg);
                msStatus = DecoderStatus_udecError;
            } else {
                msStatus = DecoderStatus_genericError;
            }
            break;

        case CMD_EOS:
            AM_ASSERT(!mpBuffer);
            AM_ASSERT(!tobeDecodedBuffer);
            ProcessEOS();
            mpWorkQ->CmdAck(ME_OK);
            break;

        default:
            AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    AMLOG_CMD("**VideoDecoderFFMpeg cmd done.\n");
    return err;
}

void VideoDecoderFFMpeg::OnRun()
{
    AM_ERR err;
    CMD cmd;
    mbRun = true;
    AMLOG_PRINTF("VideoDecoderFFMpeg OnRun start.\n");
    while(mbRun) {
        AMLOG_STATE("VideoDecoderFFMpeg state %d.\n", msStatus);
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        err = ProcessCmd(cmd);
        if (err != ME_OK) {
            AMLOG_ERROR("ProcessCMD return err %d.\n", err);
        }
    }
    AMLOG_PRINTF("VideoDecoderFFMpeg OnRun end.\n");
    return;
}

