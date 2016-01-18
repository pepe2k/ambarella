/**
 * subtitle_renderer.cpp
 *
 * History:
 *    2011/6/17 - [Xing Chong] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#define LOG_NDEBUG 0
#define LOG_TAG "subtitle_renderer"
//#define AMDROID_DEBUG

#include <basetypes.h>
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#include "am_pbif.h"
#include "pbif.h"
#include "engine_guids.h"
#include "subtitle_renderer.h"

#include "filter_list.h"
#include <string.h>
extern "C"
{
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "libavutil/avstring.h"
#include "libavutil/colorspace.h"
#include "libavcodec/amba_dsp_define.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/ass_split.h"

}

filter_entry g_subtitle_renderer = {
    "SubtitleRenderer",
    CSubtitleRenderer::Create,
    NULL,
    CSubtitleRenderer::AcceptMedia,
};


//-----------------------------------------------------------------------
//
// CSFixedBufferPool
//
//-----------------------------------------------------------------------

CSFixedBufferPool *CSFixedBufferPool::Create(AM_UINT size, AM_UINT count)
{
    CSFixedBufferPool *result = new CSFixedBufferPool;
    if (result != NULL && result->Construct(size, count) != ME_OK)
    {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CSFixedBufferPool::Construct(AM_UINT size, AM_UINT count)
{
    AM_ERR err;
    if ((err = inherited::Construct(count)) != ME_OK)
        return err;

    if ((_pBuffers = new CSubtitleBuffer[count]) == NULL)
    {
        AM_ERROR("Subtitle Renderer allocate CBuffers fail.\n");
        return ME_ERROR;
    }

    size = ROUND_UP(size, 4);
    if ((_pMemory = new AM_U8[count * size]) == NULL)
    {
        AM_ERROR("Subtitle Renderer allocate data buffer fail.\n");
        return ME_ERROR;
    }

    AM_U8 *ptr = _pMemory;
    CSubtitleBuffer *pSubtitleBuffer = _pBuffers;
    CBuffer* pBuffer;
    for (AM_UINT i = 0; i < count; i++, ptr += size, pSubtitleBuffer++)
    {
        pBuffer = (CBuffer*)pSubtitleBuffer;
        pBuffer->mpData = ptr;
        pBuffer->mBlockSize = size;
        pBuffer->mpPool = this;

        err = mpBufferQ->PostMsg(&pBuffer, sizeof(pBuffer));
        AM_ASSERT_OK(err);
    }

    return ME_OK;
}

CSFixedBufferPool::~CSFixedBufferPool()
{
    if(_pMemory) {
        delete[] _pMemory;
        _pMemory = NULL;
    }
    if(_pBuffers) {
        delete _pBuffers;
        _pBuffers = NULL;
    }
}

void CSFixedBufferPool::Delete()
{
    if(_pMemory) {
        delete[] _pMemory;
        _pMemory = NULL;
    }
    if(_pBuffers) {
        delete _pBuffers;
        _pBuffers = NULL;
    }
    inherited::Delete();
}


//-----------------------------------------------------------------------
//
// CSubtitleRenderer
//
//-----------------------------------------------------------------------
IFilter* CSubtitleRenderer::Create(IEngine *pEngine)
{
    CSubtitleRenderer *result = new CSubtitleRenderer(pEngine);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

int CSubtitleRenderer::AcceptMedia(CMediaFormat& format)
{
    if (*format.pMediaType == GUID_Decoded_Subtitle)
        return 1;
    return 0;
}

AM_ERR CSubtitleRenderer::Construct()
{
    AM_ERR err = inherited::Construct();
    if(err != ME_OK)
        return err;

    if((mpSubtitleInputPin = CSubtitleRendererInput::Create(this)) == NULL)
        return ME_ERROR;

    if((mpSBP = CSFixedBufferPool::Create(19200, 32)) == NULL)
        return ME_ERROR;

    mpSubtitleInputPin->SetBufferPool(mpSBP);

    return ME_OK;
}

CSubtitleRenderer::~CSubtitleRenderer()
{
    AM_DELETE(mpSubtitleInputPin);
    //AM_RELEASE(mpSBP);
}

void CSubtitleRenderer::Delete()
{
    AM_DELETE(mpSubtitleInputPin);
    mpSubtitleInputPin = NULL;
    //AM_RELEASE(mpSBP);
    //mpSBP = NULL;
    inherited::Delete();

}

AM_ERR CSubtitleRenderer::Start()
{
    mpWorkQ->Start();
    return ME_OK;
}

AM_ERR CSubtitleRenderer::Run()
{
    mpClockManager = (IClockManager *)mpEngine->QueryEngineInterface(IID_IClockManager);
    if(!mpClockManager) {
        AM_ERROR("CSubtitleRenderer::Run without mpClockManager?\n");
        return ME_ERROR;
    }

    mpWorkQ->Run();
    return ME_OK;
}

void CSubtitleRenderer::ClearInternalSetting()
{
    mbSubtitleStarted = false;
}

void CSubtitleRenderer::GetInfo(INFO& info)
{
    info.nInput = 1;
    info.nOutput = 0;
    info.mPriority = 4;
    info.mFlags = 0;
    info.pName = "SubtitleRenderer";
}

IPin* CSubtitleRenderer::GetInputPin(AM_UINT index)
{
    if (index == 0)
        return mpSubtitleInputPin;
    return NULL;
}


void* CSubtitleRenderer::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IClockObserver)
        return (IClockObserver*)this;

    if (refiid == IID_IRenderer)
        return (IRender*)this;

    return inherited::GetInterface(refiid);
}

AM_ERR CSubtitleRenderer::OnTimer(am_pts_t curr_pts)
{
    mpWorkQ->PostMsg(CMD_TIMERNOTIFY);
    return ME_OK;
}

void CSubtitleRenderer::RenderBuffer(CBuffer*& pBuffer)
{
    CSubtitleBuffer* pSubBuffer = (CSubtitleBuffer*)pBuffer;
    if(pSubBuffer->subType == SUBTYPE_ASS) {
        AMLOG_DEBUG("---------SUB:: %s---------\n", (char*)(pSubBuffer->mpData));
#if SHOW_SUB_ON_OSD
        if(ConvertTEXTandShow(pBuffer) != ME_OK) {
            AMLOG_ERROR("Convert TEXT ERROR!!!\n");
        }
#endif
    }
    else if(pSubBuffer->subType != SUBTYPE_ASS) {
#if SHOW_SUB_ON_OSD
        if(ConvertSubPicandShow(pBuffer) != ME_OK) {
            AMLOG_ERROR("Convert BITMAP ERROR!!!\n");
        }
#endif
    }
    pBuffer->Release();
    pBuffer = NULL;
}

void CSubtitleRenderer::DiscardBuffer(CBuffer*& pBuffer)
{
    //to do, just rendering here for testing
    AMLOG_DEBUG("---DiscardBuffer---\n");
    mbDiscard = 1;
    RenderBuffer(pBuffer);
}

bool CSubtitleRenderer::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("****CSubtitleRenderer::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);
    switch (cmd.code) {

        case CMD_START:
            CmdAck(ME_OK);
            break;

        case CMD_STOP:
            mbRun = false;
            CmdAck(ME_OK);
            break;

        case CMD_TIMERNOTIFY:
            if ( msState == STATE_READY ) {
                RenderBuffer(mpBuffer);
                msState = STATE_IDLE;
            } else if (msState == STATE_PENDING && mpBuffer) {
                //paused
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            break;

        case CMD_PAUSE:
            mbPaused = true;
            break;

        case CMD_RESUME:
            if (msState == STATE_PENDING) {
                msState = STATE_IDLE;
            }
            mbPaused = false;
            break;

        case CMD_FLUSH:
            AMLOG_INFO("Subtitle renderer Flush...\n");
            mbSubtitleStarted = false;
            if(!mbInitFBFailed)
                msState = STATE_PENDING;
            if(mpBuffer){
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            CmdAck(ME_OK);
            AMLOG_INFO("Subtitle renderer Flush done.\n");
            break;
        case CMD_BEGIN_PLAYBACK:
            AM_ASSERT(msState == STATE_PENDING);
            AM_ASSERT(!mpBuffer);
            if(!mbInitFBFailed)
                msState = STATE_IDLE;
            ClearInternalSetting();
            break;

        case CMD_SOURCE_FILTER_BLOCKED:
            break;

        default:
            AM_ERROR("Subtitle renderer wrong cmd.code: %d", cmd.code);
            break;
    }
    return false;
}

void CSubtitleRenderer::OnRun()
{
    CmdAck(ME_OK);

    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    CQueueInputPin* pPin;
    am_pts_t curTime;
    AM_S64 diff;
#ifdef AM_DEBUG
    am_pts_t lastPts = 0;
#endif

    mbRun = true;
    msState = STATE_IDLE;
    mbSubtitleStarted = false;

    //send ready
    AMLOG_PRINTF("SRenderer: post IEngine::MSG_READY.\n");
    PostEngineMsg(IEngine::MSG_READY);
    AM_ASSERT(mpClockManager);
#if SHOW_SUB_ON_OSD
    if(InitFB() != ME_OK) {
        AMLOG_ERROR("------InitFB  fail!!!----\n");
        mbInitFBFailed = 1;
        PostEngineMsg(IEngine::MSG_SUBERROR);
    }else {
        ClearScreen();
        mActive_fb = 0;
    }
#endif
    AMLOG_PRINTF("SRenderer: start.\n");
    while (mbRun) {
        AMLOG_STATE("SRenderer: start switch, msState=%d, %d input data.\n", msState, mpSubtitleInputPin->mpBufferQ->GetDataCnt());

        switch (msState) {
            case STATE_IDLE:
                AM_ASSERT(!mpBuffer);

                //goto pending if is pasued
                if(mbPaused) {
                    msState = STATE_PENDING;
                    break;
                }
                //go to error if initfb failed
                if(mbInitFBFailed) {
                    msState = STATE_ERROR;
                    break;
                }
                //wait input data, process msg
                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                if(type == CQueue::Q_MSG) {
                    ProcessCmd(cmd);
                } else {
                    pPin = (CQueueInputPin*)result.pOwner;
                    if (!pPin->PeekBuffer(mpBuffer)) {
                        AM_ERROR("No buffer?\n");
                        break;
                    }
                    if (mpBuffer->GetType() == CBuffer::EOS) {
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        PostEngineMsg(IEngine::MSG_EOS);
                        AMLOG_PRINTF("SRenderer get EOS.\n");
                        msState = STATE_PENDING;
                        ClearInternalSetting();
                        mTimeOffset = 0;
                        mbStreamStart = false;
                        break;
                    }
                    msState = STATE_HAS_INPUTDATA;
                }
                break;

            case STATE_HAS_INPUTDATA:
                AM_ASSERT(mpBuffer);
                if(!mbSubtitleStarted) {
                    mbSubtitleStarted = true;

                    //reset mSubtitleTotalSamples and mTimeOffset
                    if(!mbStreamStart) {
                        mbStreamStart = true;
                        mFirstPTS = mpBuffer->mPTS;
                        AMLOG_INFO("**** subtitle first pts = %llu.\n", mFirstPTS);
                    }
                    mTimeOffset =mpBuffer->mPTS - mFirstPTS;
                    AMLOG_INFO("WAIT_RENDERER  subtitle start PTS = %llu, first pts = %llu.\n", mpBuffer->mPTS, mFirstPTS);
                }
                curTime = mpClockManager->GetCurrentTime();
                diff = ((CSubtitleBuffer*)mpBuffer)->start_time - curTime;

                //slave should consider time/PTS differerence: discard/render immediately/render later
                if(diff < (-2 * mWaitThreshold)) {
                    DiscardBuffer(mpBuffer);
                }else if (diff > mWaitThreshold) {
                    mpClockManager->SetTimer(this, ((CSubtitleBuffer*)mpBuffer)->start_time - mWaitThreshold);
                    msState = STATE_READY;
                    break;
                } else {
                    //proper time
                    RenderBuffer(mpBuffer);
                }
                msState = STATE_IDLE;
                break;

            case STATE_PENDING:
            case STATE_READY:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_ERROR:
                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                if(type == CQueue::Q_MSG) {
                    ProcessCmd(cmd);
                } else {
                    pPin = (CQueueInputPin*)result.pOwner;
                    if (!pPin->PeekBuffer(mpBuffer)) {
                        AM_ERROR("No buffer?\n");
                        break;
                    }
                    if (mpBuffer->GetType() == CBuffer::EOS) {
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        PostEngineMsg(IEngine::MSG_EOS);
                        AMLOG_PRINTF("SRenderer get EOS.\n");
                        msState = STATE_PENDING;
                        ClearInternalSetting();
                        mTimeOffset = 0;
                        mbStreamStart = false;
                        break;
                    }
                    mpBuffer->Release();
                }
                break;

            default:
                AM_ERROR("error state: %d.\n", msState);
                break;
        }
    }

    if(mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }
#if SHOW_SUB_ON_OSD
    if(mGgl_framebuffer != NULL && mfb_fd != -1) {
        ClearScreen();
        SetActiveFrameBuffer(0);
        ExitFB();
    }
#endif
    AMLOG_INFO("SRenderer OnRun: end.\n");
}

AM_ERR CSubtitleRenderer::GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs)
{
    if(!mbSubtitleStarted) {
        absoluteTimeMs = 0;
        relativeTimeMs = 0;
        return ME_OK;
    }
    AMLOG_DEBUG("CSubtitleRenderer::GetCurrentTime return absoluteTimeMs=%llu relativeTimeMs=%llu.\n", absoluteTimeMs, relativeTimeMs);
    return ME_OK;
}

#ifdef AM_DEBUG
void CSubtitleRenderer::PrintState()
{
    AMLOG_PRINTF("SRenderer: msState=%d, %d input data.\n", msState, mpSubtitleInputPin->mpBufferQ->GetDataCnt());
}
#endif
AM_ERR CSubtitleRenderer::ConvertSubPicandShow(CBuffer*& pBuffer)
{
    AVSubtitle* pSub = (AVSubtitle*)(pBuffer->mpData);
    for (AM_UINT i = 0; i < pSub->num_rects; i++)
    {
        AM_UINT w = pSub->rects[i]->w;
        AM_UINT h = pSub->rects[i]->h;
        //Get the radio of width and height to scale the original bitmap
        float radio_pic = (float)w / (float)h;
        float radio_fb = (float)(mGgl_framebuffer[0].width)/(float)(mGgl_framebuffer[0].height);
        AM_UINT draw_w, draw_h;
        if(radio_pic < radio_fb) {
            draw_h = (h > mGgl_framebuffer[0].height) ? mGgl_framebuffer[0].height : h;
            draw_w = draw_h * radio_pic;
        }else {
            draw_w = (w > mGgl_framebuffer[0].width) ? mGgl_framebuffer[0].width : w;
            draw_h = draw_w / radio_pic;
        }
        AVFrame* pConvertFrame= NULL;
        AVFrame* pScale= NULL;
        AM_INT num = avpicture_get_size(PIX_FMT_RGB24, w, h);
        AM_INT numScale = avpicture_get_size(PIX_FMT_RGB565, draw_w, draw_h);
        AM_U8* pConvertTmp = new AM_U8[num];
        AM_U8* pScaleTmp = new AM_U8[numScale];
        AM_INT scaled = 0;
         pConvertFrame= avcodec_alloc_frame();
         pScale= avcodec_alloc_frame();
        if(!pConvertFrame)
        {
            AMLOG_ERROR("failed to allocate memory for a AVFrame object.\n");
            return ME_ERROR;
        }
        if(!pScale)
        {
            AMLOG_ERROR("failed to allocate memory for a AVFrame object.\n");
            return ME_ERROR;
        }
        avpicture_fill((AVPicture *)pConvertFrame, pConvertTmp, PIX_FMT_RGB565, w, h);
        avpicture_fill((AVPicture *)pScale, pScaleTmp, PIX_FMT_RGB565, draw_w, draw_h);
        //Convert the original bitmap, wich is always 2bit bitmap,  to RGB565
        Convert2RGB565(pSub->rects[i]->pict.data[1], pSub->rects[i]->nb_colors, pSub->rects[i]->pict.data[0], (AM_U16*)(pConvertFrame->data[0]), pSub->rects[i]->w, pConvertFrame->linesize[0], pSub->rects[i]->w, pSub->rects[i]->h);
        if(draw_w != w || draw_h != h) {
            AM_Scale(PIX_FMT_RGB565, pConvertFrame->data, pScale->data, pConvertFrame->linesize, pScale->linesize, w, h, draw_w, draw_h);
            scaled = 1;
        }
        //Show the picture
        int i = 100;
        CopyFrametoFB(mActive_fb, (scaled ? pScale : pConvertFrame), draw_w, draw_h);
        while((mbDiscard || (((CSubtitleBuffer*)pBuffer)->end_time ==  ((CSubtitleBuffer*)pBuffer)->start_time)) ? i : mpClockManager->GetCurrentTime() < ((CSubtitleBuffer*)pBuffer)->end_time - 1800) {
            //CopyFrametoFB(mActive_fb, (scaled ? pScale : pConvertFrame), draw_w, draw_h);
            SetActiveFrameBuffer(mActive_fb);
            //ClearActiveBuffer();
            //mActive_fb = !mActive_fb;
            i--;
        }
        ClearActiveBuffer();
        //mActive_fb = !mActive_fb;
        //Clear data
        if (pScale) {
            pScale->data[0] = NULL;
            av_free(pScale);
        }
        if (pScaleTmp) {
            delete []pScaleTmp;
        }
        if (pConvertFrame) {
            pConvertFrame->data[0] = NULL;
            av_free(pConvertFrame);
        }
        if (pConvertTmp) {
            delete []pConvertTmp;
        }
    }
    return ME_OK;
}
AM_INT CSubtitleRenderer::GetFrameBuffer(GGLSurface * fb)
{
    int fd;
    void *bits;

    fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd < 0) {
        AMLOG_ERROR("cannot open /dev/graphics/fb0, retrying with /dev/fb0\n");
        if ((fd = open("/dev/fb0", O_RDWR)) < 0) {
            AMLOG_ERROR("cannot open /dev/fb0");
            return -1;
        }
    }

    if(ioctl(fd, FBIOGET_FSCREENINFO, &mFixinfo) < 0) {
        AMLOG_ERROR("failed to get fb0 info");
        return -1;
    }

    if(ioctl(fd, FBIOGET_VSCREENINFO, &mVarinfo) < 0) {
        AMLOG_ERROR("failed to get fb0 info");
        return -1;
    }

    bits = mmap(0, mFixinfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(bits == MAP_FAILED) {
        AMLOG_ERROR("failed to mmap framebuffer");
        return -1;
    }

    fb->version = sizeof(*fb);
    fb->width = mVarinfo.xres;
    fb->height = mVarinfo.yres;
    fb->stride = mFixinfo.line_length / (mVarinfo.bits_per_pixel >> 3);
    fb->data =(GGLubyte*) bits;
    fb->format = GGL_PIXEL_FORMAT_RGB_565;

    fb++;

    fb->version = sizeof(*fb);
    fb->width = mVarinfo.xres;
    fb->height = mVarinfo.yres;
    fb->stride = mFixinfo.line_length / (mVarinfo.bits_per_pixel >> 3);
    fb->data = (GGLubyte*) (((unsigned) bits) + mVarinfo.yres * mVarinfo.xres * 2);
    fb->format = GGL_PIXEL_FORMAT_RGB_565;

    return fd;
}
AM_ERR CSubtitleRenderer::InitFB()
{
    int fd = -1;

    if (!access("/dev/tty0", F_OK)) {
        fd = open("/dev/tty0", O_RDWR | O_SYNC);
        if(fd < 0) {
            AMLOG_ERROR("-----Can't open /dev/tty0---\n");
            return ME_ERROR;
        }

        if(ioctl(fd, KDSETMODE, (void*) KD_GRAPHICS)) {
            AMLOG_ERROR("-----KDSETMODE  ERROR---\n");
            close(fd);
            return ME_ERROR;
        }
    }else {
        AM_ERROR("----Can not access tty0----\n");
        return ME_ERROR;
    }

    mfb_fd = GetFrameBuffer(mGgl_framebuffer);

    if(mfb_fd < 0) {
        if (fd >= 0) {
            ioctl(fd, KDSETMODE, (void*) KD_TEXT);
            close(fd);
        }
        return ME_ERROR;
    }

    mvt_fd = fd;

    // start with 0 as front
    mActive_fb = 0;
    SetActiveFrameBuffer(mActive_fb);

    return ME_OK;
}
void CSubtitleRenderer::ExitFB()
{
    close(mfb_fd);
    mfb_fd = -1;

    if (mvt_fd >= 0) {
        ioctl(mvt_fd, KDSETMODE, (void*) KD_TEXT);
        close(mvt_fd);
        mvt_fd = -1;
    }
}
void CSubtitleRenderer::SetActiveFrameBuffer(AM_UINT buffer_num)
{
    if(buffer_num > 1)
        return;
    mVarinfo.yres_virtual = mVarinfo.yres * 2;
    mVarinfo.yoffset = buffer_num * mVarinfo.yres;
    if(ioctl(mfb_fd, FBIOPUT_VSCREENINFO, &mVarinfo) < 0) {
        AMLOG_INFO("active fb swap failed!\n");
    }
}
void CSubtitleRenderer::CopyFrametoFB(AM_INT active,AVFrame *pFrame, AM_UINT w, AM_UINT h)
{
    AM_UINT i = 0;
    AM_INT yoff = 0;
    AM_INT srcstride = pFrame->linesize[0];
    AM_INT desstride = mGgl_framebuffer[active].stride * 2;
    AM_U8* psrc = pFrame->data[0];
    AM_U8* pdes = mGgl_framebuffer[active].data;
    if(h < mGgl_framebuffer[0].height) {
        yoff = mGgl_framebuffer[0].height - h;
        pdes += desstride * (yoff - 1);
    }
    for(i = 0; i < h; i++)
    {
        memcpy(pdes, psrc, w * 2);
        pdes += desstride;
        psrc += srcstride;
    }
}

void CSubtitleRenderer::ClearActiveBuffer()
{
    memset(mGgl_framebuffer[mActive_fb].data, 0x0, mFixinfo.smem_len);
}

void CSubtitleRenderer::ClearScreen()
{
    memset(mGgl_framebuffer[0].data, 0x0, mFixinfo.smem_len);
}
void CSubtitleRenderer::Convert2RGB565(AM_U8* palette, AM_INT color_num, AM_U8* src, AM_U16* des, AM_INT src_stride, AM_INT des_stride, AM_INT width, AM_INT height)
{
    //palette == NULL means it is a 256-gray bitmap
    if(palette == NULL) {
        for(AM_INT j = 0; j < height; j++)
        {
            for(AM_INT i = 0; i < width; i++)
            {
                des[i] = (((AM_U16)(src[i]) &0xF8) << 8) | (((AM_U16)(src[i]) &0xF9) << 3) |(((AM_U16)(src[i]) &0xF8) >> 3);
            }
            src += src_stride;
            des += des_stride / 2;
        }
    }
    //palette != NULL means it is a 2bit bitmap
    if(palette && color_num) {
        AM_U16 Color[color_num];
        for (AM_INT j = 0; j < color_num; j++)
        {
            Color[j] = *((AM_U16*)palette + j);
        }
        for(AM_INT j = 0; j < height; j++){
            for(AM_INT i = 0; i < width; i++) {
                des[i] = Color[*( src++)];
            }
            des += (des_stride) / 2;
        }
    }
}
//Show text subtitle
AM_ERR CSubtitleRenderer::InitFTlibary()
{
    if(FT_Init_FreeType( &mlibrary) != 0 )
        return ME_ERROR;
    return ME_OK;
}
AM_ERR CSubtitleRenderer::InitFont(AM_U8* fontname, AM_INT size)
{
    if(InitFTlibary() != ME_OK) {
        return ME_ERROR;
    }

    if(!strncasecmp("Arial", (char*)fontname, strlen("Arial"))) {
        //The font file need be somewhere, I only added Arial.ttf in /system/fonts
        if(FT_New_Face(mlibrary, "/system/fonts/DroidSerif-Regular.ttf", 0, &mFontFace) != 0)
            return ME_ERROR;
    }else {
         if(FT_New_Face(mlibrary, "/system/fonts/DroidSans.ttf", 0, &mFontFace) != 0)
            return ME_ERROR;
    }
    //The size unit is pt
    //if(FT_Set_Char_Size(mFontFace, 0, size * 64, mVarinfo.xres, mVarinfo.yres / 2) != 0 )
    //The size unit is pixel
    if(FT_Set_Pixel_Sizes(mFontFace, 0, size) != 0)
        return ME_ERROR;
    return ME_OK;
}
AM_INT CSubtitleRenderer::ConvertText2Bmp(AM_U8* text, AM_UINT* max_linewidth, AM_UINT* min_linewidth, AM_UINT* max_height)
{
    AM_INT lines = 0;
    AM_UINT linewidth = 0;
    *max_linewidth = 0;//get the maximum width of one line
    *min_linewidth = 0;//get the minimum width of one line
    *max_height = 0;//get the maximum height of one char
    //Convert char to bitmap Only Support English now!
    for(AM_UINT i = 0; i < strlen((char*)text); i++) {
        AM_UINT ucode = text[i];
        FT_UInt glyph_index = FT_Get_Char_Index(mFontFace, ucode);
        if(!glyph_index && text[i] != '\n') {
            continue;
        }

        if (FT_Load_Glyph(mFontFace, glyph_index, FT_LOAD_DEFAULT)) {
            continue;
        }
        if (mFontFace->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
            if (FT_Render_Glyph(mFontFace->glyph, FT_RENDER_MODE_NORMAL)) {
                continue;
            }
        }
        TEXTBMP* bmp = (TEXTBMP*)new(TEXTBMP);
        bmp->next = NULL;
        if(text[i] == '\n') {
            lines++;
            AMLOG_INFO("-----line break char-----\n");
        }else {
            bmp->left = mFontFace->glyph->bitmap_left;
            bmp->top = (AM_INT)(mFontFace->size->metrics.ascender >> 6) - mFontFace->glyph->bitmap_top;
            bmp->width = mFontFace->glyph->bitmap.width;
            bmp->height = mFontFace->glyph->bitmap.rows;
            bmp->linesize = mFontFace->glyph->bitmap.pitch;
            bmp->advance = (AM_INT)(mFontFace->glyph->metrics.horiAdvance >> 6);
            bmp->data = (AM_U8*)calloc(bmp->height * bmp->linesize, sizeof(AM_U16));
            Convert2RGB565(NULL, 0, mFontFace->glyph->bitmap.buffer, (AM_U16*)(bmp->data), mFontFace->glyph->bitmap.pitch, bmp->linesize * 2, bmp->width, bmp->height);
        }
        if(text[i] != '\n') {
            linewidth += bmp->advance;
        }else {
            *max_linewidth = (linewidth > *max_linewidth ? linewidth : *max_linewidth);
            if(lines == 1) {
                *min_linewidth = linewidth;
            }else {
                *min_linewidth = (linewidth < *min_linewidth ? linewidth : *min_linewidth);
            }
            linewidth = 0;
        }
        *max_height = ((AM_INT)*max_height > (bmp->height + bmp->top) ? *max_height : (bmp->height + bmp->top));
        if(!mpTextBmpList) {
            mpTextBmpList = bmp;
            mpLastTextBmp = bmp;
            continue;
        }
        //Add this to the list
        mpLastTextBmp->next = bmp;
        mpLastTextBmp = bmp;
    }
    *max_linewidth = (linewidth > *max_linewidth ? linewidth : *max_linewidth);
    if(lines == 0) {
        *min_linewidth = linewidth;
    }
    *min_linewidth = (linewidth < *min_linewidth ? linewidth : *min_linewidth);
    if(!mpLastTextBmp->data)
        lines -= 1;
    return lines + 1;
}
void CSubtitleRenderer::CombineBMP(AM_U8* des, AM_INT des_stride, AM_INT width, AM_INT height)
{
    if(!mpTextBmpList && !mpLastTextBmp) {
        return;
    }
    mpLastTextBmp = mpTextBmpList;
    AM_INT n = 0;
    AM_U16* pDes = (AM_U16*)des;
    while(mpLastTextBmp) {
        AM_INT curr_width = width - mpLastTextBmp->advance;
        AM_U16* pTmp = pDes;
        while(mpLastTextBmp && curr_width > 0) {
            if(!mpLastTextBmp->data) {
                // This is a line break
                mpLastTextBmp = mpLastTextBmp->next;
                break;
            }
            if(!mpLastTextBmp->linesize && !mpLastTextBmp->width) {
                // This is a space
                curr_width -= 10;
                if(mpLastTextBmp->next)
                    pTmp += 10;
                mpLastTextBmp = mpLastTextBmp->next;
                continue;
            }else {
                curr_width -= mpLastTextBmp->advance;
            }
            AM_U16* pSrc = (AM_U16*)(mpLastTextBmp->data);
            //Add the begine coordinate
            AM_U16* pdes = pTmp + mpLastTextBmp->left + mpLastTextBmp->top * des_stride / 2;
            for(AM_INT j = 0; j < mpLastTextBmp->height; j++) {
                ::memcpy(pdes, pSrc, mpLastTextBmp->linesize * 2);
                pSrc += mpLastTextBmp->linesize;
                pdes += des_stride / 2;
            }
            if(mpLastTextBmp->next)
                pTmp += mpLastTextBmp->advance;
            mpLastTextBmp = mpLastTextBmp->next;
            n++;
        }
        if(mpLastTextBmp)
            pDes += height * des_stride / 2;
    }
}
AM_ERR CSubtitleRenderer::ConvertTEXTandShow(CBuffer*& pBuffer)
{
    CSubtitleBuffer* pSrc = (CSubtitleBuffer*)pBuffer;
    AM_UINT max_linewidth = 0;
    AM_UINT min_linewidth = 0;
    AM_UINT max_height = 0;
    //init font
    if(InitFont(pSrc->fontname, pSrc->fontsize) != ME_OK) {
        return ME_ERROR;
    }
    //convert
    AM_INT lines = ConvertText2Bmp(pSrc->mpData, &max_linewidth, &min_linewidth, &max_height);
    //combine
    AM_UINT width = mGgl_framebuffer[0].width;
    AM_UINT height = 0;
    if(mGgl_framebuffer[0].width >= max_linewidth) {
        height = max_height * lines + 5;
    }else {
        //just set height as twice of the lines
        height = max_height * lines * 2 +10;
    }
    if(height > mGgl_framebuffer[0].height) {
        return ME_ERROR;
    }
    AVFrame* pFrame= NULL;
    AM_INT num = avpicture_get_size(PIX_FMT_RGB565, width, height);
    AM_U8* pTmp = new AM_U8[num];
    pFrame= avcodec_alloc_frame();
    if(!pFrame)
    {
        AMLOG_ERROR("failed to allocate memory for a AVFrame object.\n");
        return ME_ERROR;
    }
    avpicture_fill((AVPicture *)pFrame, pTmp, PIX_FMT_RGB565, width, height);
    ::memset(pFrame->data[0], 0, pFrame->linesize[0] * height);
    CombineBMP(pFrame->data[0], pFrame->linesize[0], width, max_height);
    //show
    int i = 200;
    CopyFrametoFB(mActive_fb, pFrame, width, height);
    while((mbDiscard || pSrc->end_time ==  pSrc->start_time) ? i : mpClockManager->GetCurrentTime() < pSrc->end_time - 1800) {
        //CopyFrametoFB(mActive_fb, (scaled ? pScale : pConvertFrame), draw_w, draw_h);
        SetActiveFrameBuffer(mActive_fb);
        //ClearActiveBuffer();
        //mActive_fb = !mActive_fb;
        i--;
    }
    ClearActiveBuffer();
    //clear data
    if (pFrame) {
        pFrame->data[0] = NULL;
        av_free(pFrame);
    }
    if (pTmp) {
        delete[] pTmp;
    }
    //free font face
    FT_Done_Face(mFontFace);
    FT_Done_FreeType(mlibrary);

    DeleteBMPList();
    return ME_OK;
}
void CSubtitleRenderer::DeleteBMPList()
{
    if(!mpTextBmpList && !mpLastTextBmp) {
        return;
    }
    while(mpTextBmpList) {
        TEXTBMP* p = mpTextBmpList;
        mpTextBmpList = p->next;
        if(p->data)
            free(p->data);
        delete p;
    }
    mpLastTextBmp = NULL;
}
//-----------------------------------------------------------------------
//
// CSubtitleRendererInput
//
//-----------------------------------------------------------------------
CSubtitleRendererInput* CSubtitleRendererInput::Create(CFilter *pFilter)
{
    CSubtitleRendererInput* result = new CSubtitleRendererInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }

    return result;
}

AM_ERR CSubtitleRendererInput::Construct()
{
    AM_ERR err = inherited::Construct(((CSubtitleRenderer*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

CSubtitleRendererInput::~CSubtitleRendererInput()
{
    // todo
}

AM_ERR CSubtitleRendererInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    CSubtitleRenderer* pfilter = (CSubtitleRenderer*) mpFilter;
    if (*pFormat->pMediaType == GUID_Decoded_Subtitle) {
        if (*pFormat->pFormatType == GUID_Format_FFMPEG_Media)
            return ME_OK;
    }
    return ME_NOT_SUPPORTED;
}

