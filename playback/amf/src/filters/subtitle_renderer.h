
/*
 * subtitle_renderer.h
 *
 * History:
 *    2011/6/17 - [Xing Chong] create file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __SUBTITLE_RENDERER_H__
#define __SUBTITLE_RENDERER_H__

#define SHOW_SUB_ON_OSD 0

#include <media/MediaPlayerInterface.h>
extern "C"
{
#include "libavformat/avformat.h"

#include <linux/fb.h>
#include <linux/kd.h>
#include <pixelflinger/pixelflinger.h>
}
#include <ft2build.h>
#include FT_FREETYPE_H
class CSFixedBufferPool;
class CSubtitleRenderer;
class CSubtitleRendererInput;

using namespace android;


//-----------------------------------------------------------------------
//
// CSFixedBufferPool
//
//-----------------------------------------------------------------------
class CSFixedBufferPool: public CBufferPool
{
    typedef CBufferPool inherited;
    friend class CSubtitleRendererInput;

public:
    static CSFixedBufferPool* Create(AM_UINT size, AM_UINT count);
    virtual void Delete();

protected:
    CSFixedBufferPool(): inherited("SubtitmeBuffer"), _pBuffers(NULL), _pMemory(NULL) {}
    AM_ERR Construct(AM_UINT size, AM_UINT count);
    virtual ~CSFixedBufferPool();

private:
    CSubtitleBuffer *_pBuffers;
    AM_U8 *_pMemory;
};


//-----------------------------------------------------------------------
//
// CSubtitleRenderer
//
//-----------------------------------------------------------------------
class CSubtitleRenderer: public CActiveFilter, public IClockObserver, IRender
{
    typedef CActiveFilter inherited;
    friend class CSubtitleRendererInput;

public:
    static IFilter* Create(IEngine *pEngine);
    static int AcceptMedia(CMediaFormat& format);

private:
    CSubtitleRenderer(IEngine *pEngine):
        inherited(pEngine, "SubtitleRenderer"),
        mpSubtitleInputPin(NULL),
        mpSBP(NULL),
        mpBuffer(NULL),
        mbDiscard(0),
        mbSubtitleStarted(false),
        mTimeOffset(0),
        mFirstPTS(0),
        mpClockManager(NULL),
        mLatency(0),
        mpGgl_context(NULL),
        mActive_fb(0),
        mfb_fd(-1),
        mvt_fd(-1),
        mbInitFBFailed(0),
        mpTextBmpList(NULL),
        mpLastTextBmp(NULL)
    {mWaitThreshold = 900;}
    AM_ERR Construct();
    virtual ~CSubtitleRenderer();

public:
    //IFilter
    virtual AM_ERR Run();
    virtual AM_ERR Start();

#ifdef AM_DEBUG
    virtual void PrintState();
#endif

    virtual void GetInfo(INFO& info);
    virtual IPin* GetInputPin(AM_UINT index);

    //IActiveObject
    virtual void OnRun();

    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();

    //IClockObserver
    virtual AM_ERR OnTimer(am_pts_t curr_pts);

    //IRender
    virtual AM_ERR GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs);

private:
    void ClearInternalSetting();

protected:
    virtual bool ProcessCmd(CMD& cmd);
    void RenderBuffer(CBuffer*& pBuffer);
    void DiscardBuffer(CBuffer*& pBuffer);
    //For show picture-subtitle
    AM_ERR ConvertSubPicandShow(CBuffer*& pBuffer);
    AM_INT GetFrameBuffer(GGLSurface *fb);
    AM_ERR InitFB();
    void ExitFB();
    void SetActiveFrameBuffer(AM_UINT buffer_num);
    void CopyFrametoFB(AM_INT active,AVFrame *pFrame, AM_UINT w, AM_UINT h);
    void ClearActiveBuffer();
    void ClearScreen();
    //Convert 2bit or 256gray bitmap to RGB565
    void Convert2RGB565(AM_U8* palette, AM_INT color_num, AM_U8* src, AM_U16* des, AM_INT src_stride, AM_INT des_stride, AM_INT width, AM_INT height);
    //For show text subtitle
    AM_ERR InitFTlibary();
    AM_ERR InitFont(AM_U8* fontname, AM_INT size);
    AM_INT ConvertText2Bmp(AM_U8* text, AM_UINT* max_linewidth, AM_UINT* min_linewidth, AM_UINT* max_height);
    void CombineBMP(AM_U8* des, AM_INT des_stride, AM_INT width, AM_INT height);
    AM_ERR ConvertTEXTandShow(CBuffer*& pBuffer);
    void DeleteBMPList();
private:
    CSubtitleRendererInput *mpSubtitleInputPin;
    CSFixedBufferPool *mpSBP;
    CBuffer* mpBuffer;
    AM_INT mbDiscard;

protected:
    bool mbSubtitleStarted;//when render starts to work or resumes, it is true, and when render is stopped or is flushed, it is false;
    AM_UINT mSubtitleBufferSize;
    AM_U64 mTimeOffset;//latest seek's position
    AM_U64 mFirstPTS;
    IClockManager* mpClockManager;
    AM_U32 mLatency;

    //For show subtitle on OSD
    GGLContext *mpGgl_context ;
    GGLSurface mGgl_framebuffer[2];
    AM_UINT mActive_fb ;
    AM_INT mfb_fd;
    AM_INT mvt_fd;
    AM_INT mbInitFBFailed;
    struct fb_var_screeninfo mVarinfo;
    struct fb_fix_screeninfo mFixinfo;

    //Convert text to bmp
    FT_Library mlibrary;
    FT_Face mFontFace;
    //save bitmap converted by freetype as RGB565
    struct TEXTBMP{
        AM_INT left;
        AM_INT top;
        AM_INT width;
        AM_INT height;
        AM_INT linesize;
        AM_INT advance;
        AM_U8* data;
        TEXTBMP* next;
        TEXTBMP(){
            left = 0;
            top = 0;
            width = 0;
            height = 0;
            linesize = 0;
            advance = 0;
            data = NULL;
            next = NULL;
        };
    };
    //text list
    TEXTBMP* mpTextBmpList;
    TEXTBMP* mpLastTextBmp;
};

//-----------------------------------------------------------------------
//
// CSubtitleRendererInput
//
//-----------------------------------------------------------------------
class CSubtitleRendererInput: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
    friend class CSubtitleRenderer;

public:
    static CSubtitleRendererInput* Create(CFilter *pFilter);

private:
    CSubtitleRendererInput(CFilter *pFilter):
        inherited(pFilter)
    {}
    AM_ERR Construct();
    virtual ~CSubtitleRendererInput();

public:
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
};


#endif
