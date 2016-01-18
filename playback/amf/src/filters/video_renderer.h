
/*
 * video_renderer.h
 *
 * History:
 *    2010/1/29 - [Oliver Li] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __VIDEO_RENDERER_H__
#define __VIDEO_RENDERER_H__

class CFrameBufferPool;
class CVideoRenderer;
class CVideoRendererInput;

#define AV_NOSYNC_THRESHOLD   (50 * 10000)	// 50ms,unit 100ns


//-----------------------------------------------------------------------
//
// CVideoRendererInput
//
//-----------------------------------------------------------------------
class CVideoRendererInput: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
    friend class CVideoRenderer;

public:
    static CVideoRendererInput *Create(CFilter *pFilter);

protected:
    CVideoRendererInput(CFilter *pFilter):
        inherited(pFilter)
    {}
    AM_ERR Construct();
    virtual ~CVideoRendererInput();

public:

    // CPin
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
};

//-----------------------------------------------------------------------
//
// CVideoRenderer
//
//-----------------------------------------------------------------------
class CVideoRenderer: public CActiveFilter, public IClockObserver, IVideoOutput, IRender
{
    typedef CActiveFilter inherited;
    friend class CVideoRendererInput;

    //MODE1 means sw/hybird rv40 and ppmode==1 hybird mpeg4, need render buffer, buffer from previous filter
    //MODE2 means ppmode == 2, hw/hybird mpeg4, no need render buffer
    //MODE3 means ppmode == 3, or ppmode ==1 && hw mode, need process buffer, buffer from iav driver
    enum {
        RUNNING_MODE_INVALID = 0,
        RUNNING_MODE1 = 1,
        RUNNING_MODE2 = 2,
        RUNNING_MODE3 = 3,
    };

    //filter related sub state
    enum {
        STATE_WAIT_UDEC_RUN = LAST_COMMON_STATE + 1,

        STATE_WAIT_RENDERER_READY_MODE1, //ppmode ==1, and sw, hybird rv40, need feed buffer
        STATE_WAIT_RENDERER_READY_MODE2, //ppmode==2 (hw and hybird mpeg4), no need feed buffer
        STATE_WAIT_PROPER_START_TIME,

        STATE_RUNNING_MODE2,
        STATE_RUNNING_MODE3,

        STATE_WAIT_EOS_MODE2,

        STATE_FILTER_STEP_MODE,
    };

public:

    static IFilter* Create(IEngine *pEngine);
    static int AcceptMedia(CMediaFormat& format);

public:
    // IFilter
    virtual AM_ERR  Run();
    virtual AM_ERR  Start();
    virtual void    GetInfo(INFO& info);
    virtual IPin*   GetInputPin(AM_UINT index);
#ifdef AM_DEBUG
    virtual void PrintState();
#endif

    // IActiveObject
    virtual void OnRun();

    // IClockObserver
    virtual AM_ERR OnTimer(am_pts_t curr_pts);

    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();

    // IRender
    virtual AM_ERR GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs);

    //IVideoOutput
    virtual AM_ERR Step();
    virtual int    SetInputCenter(AM_INT pic_x, AM_INT pic_y);
    virtual AM_ERR ChangeInputCenter(AM_INT input_center_x, AM_INT input_center_y);
    virtual AM_ERR SetDisplayPositionSize(AM_INT vout, AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height);
    virtual AM_ERR GetDisplayPositionSize(AM_INT vout, AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height);
    virtual AM_ERR SetDisplayPosition(AM_INT vout, AM_INT pos_x, AM_INT pos_y);
    virtual AM_ERR SetDisplaySize(AM_INT vout, AM_INT width, AM_INT height);
    virtual AM_ERR GetDisplayDimension(AM_INT vout, AM_INT* width, AM_INT* height);
    virtual AM_ERR GetVideoPictureSize(AM_INT* width, AM_INT* height);
    virtual AM_ERR GetCurrentVideoPictureSize(AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height);
    virtual AM_ERR VideoDisplayZoom(AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height);
    virtual AM_ERR SetDisplayFlip(AM_INT vout, AM_INT flip);
    virtual AM_ERR SetDisplayMirror(AM_INT vout, AM_INT mirror);
    virtual AM_ERR SetDisplayRotation(AM_INT vout, AM_INT degree);
    virtual AM_ERR EnableVout(AM_INT vout, AM_INT enable);
    virtual AM_ERR EnableOSD(AM_INT vout, AM_INT enable);
    virtual AM_ERR EnableVoutAAR(AM_INT enable);

    virtual AM_ERR SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h);
    virtual AM_ERR SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h);
    virtual AM_ERR SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y);

    virtual AM_ERR SetDeWarpControlWidth(AM_UINT enable, AM_UINT width_top, AM_UINT width_bottom);

    // inherited from CActiveFilter
    virtual bool ProcessCmd(CMD& cmd);

protected:
    AM_ERR Construct();

    CVideoRenderer(IEngine *pEngine);
    virtual ~CVideoRenderer();

    AM_ERR SetInputFormat(CFFMpegMediaFormat *pFormat);

private:
    void reset();
    void handleEOS();

    bool requestRun();

    int configVout(
        AM_INT vout,
        AM_INT video_rotate,
        AM_INT video_flip,
        AM_INT target_pos_x, AM_INT target_pos_y,
        AM_INT target_width, AM_INT target_height);

    AM_INT getOutPic(iav_frame_buffer_t& frame);
    AM_INT getOutPicTransc(iav_transc_frame_t& frame);

    void renderBuffer(CBuffer*& pBuffer);
    void renderBufferI1(int iavFd, int DspIndex, CVideoBuffer *pVideoBuffer, SRect* srcRect, AM_UINT mode);

    void discardBuffer(CBuffer*& pBuffer);

    void syncWithUDEC(AM_INT no_wait);
    void reportUDECErrorIfNeeded(AM_UINT& udec_state, AM_UINT& vout_state, AM_UINT& error_code);
    bool waitVoutWaken();
    void wakeVout();
    void trickPlay(AM_UINT mode);
    void sendSyncInErrorCase();

    void onStateIdle();
    void onStateHasInputData();
    void onStateWaitRendererReadyMode1();
    void onStateWaitRendererReadyMode2();
    void onStateWaitProperStartTime();
    void onStateRunningMode2();
    void onStateRunningMode3();
    void onStateWaitEOSMode2();
    void onStateStepMode();

private:
    CVideoRendererInput *mpInput;
    CIAVVideoBufferPool *mpFrameBufferPool;

    IClockManager       *mpClockManager;
    CBuffer             *mpBuffer;

    AM_INT          mIavFd;
    AM_INT          mDspIndex;
    DSPHandler      *mpDSPHandler;

    AM_UINT         mPicWidth, mPicHeight; //video codec size
    AM_UINT         mDisplayWidth, mDisplayHeight; //video render size, for app to GetVideoPictureSize().
    SVoutConfig     mVoutConfig[eVoutCnt];//0:lcd, 1:hdmi
    bool            mbDisplayRectNeedUpdate;
    SDisplayRectMap mDisplayRectMap[eVoutCnt];

    bool        mbStepMode;
    AM_UINT     mStepCnt;

    bool        mbVideoStarted;
    am_pts_t    mFirstPTS;
    am_pts_t    mLastPTS;
    am_pts_t    mLastSWRenderPTS;
    am_pts_t    mTimeOffset;//time offset of seek pointer
    AM_UINT     mAdjustInterval;

    AM_UINT mRunningMode;
    AM_UINT mEstimatedLatency;
    bool mbRecievedSynccmd;
    bool mbRecievedBegincmd;
    bool mbRecievedUDECRunningcmd;
    bool mbPostAVSynccmd;
    bool mbVoutWaken;//for debug checking
    bool mbVoutPaused;//for debug checking
    bool mbVoutAAR;//for debug checking, vout auto aspect ratio
    AM_UINT msResumeState;

    iav_audio_clk_offset_s mAudioClkOffset;
    bool mbEOSReached;
    iav_udec_status_t mStatusEOS;

    AM_UINT mWaitingUdecEOS;
    AM_INT mRet;
    AM_UINT mDebugWaitCount;

//variable with less importance
private:
    AM_INT mPreciseSyncCheckCount;
    AM_INT mPreciseSyncCheckCountThreshold;
    AM_S32 mPreciseSyncAccumulatedDiff;
};


#endif
