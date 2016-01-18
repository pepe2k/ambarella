
/*
 * video_transcoder.h
 *
 * History:
 *    2011/9/20 - [GangLiu] create file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __VIDEO_TRANSCODER_H__
#define __VIDEO_TRANSCODER_H__

class CVideoTranscoder;
class CVideoTranscoderInput;
class CVideoTranscoderOutput;
//-----------------------------------------------------------------------
//
// CVideoTranscoder
//
//-----------------------------------------------------------------------
class CVideoTranscoder: public CActiveFilter, public IVideoTranscoder, public IClockObserver
{
    typedef CActiveFilter inherited;
    friend class CVideoTranscoderInput;
    friend class CVideoTranscoderOutput;

public:
    static IFilter* Create(IEngine *pEngine);

private:
    CVideoTranscoder(IEngine *pEngine):
        inherited(pEngine, "VideoTranscoder"),
        mIavFd(-1),
        mTotalInputPinNumber(0),
        mDspIndex(-1),
        mbIsTranscoding(false),
        mWantedFrms(0),
        mpClockManager(NULL),
        mbSetClockMgr(false),
        mRenderMethord(2),
        mbEnablePreview(false),
        mbUseDefaultEncInfo(true),
        mbUseDefaultEncConfig(true),
        mbCtrlFPS(false),
        mFpsBase(3003),
        mbStopped(false),
        mpMutex(NULL)
    {
        mpInputPin[0]=NULL;
        mpInputPin[1]=NULL;
        mbInputPin[0]=false;
        mbInputPin[1]=false;
        mpOutputPin=NULL;
        memset(&mEncInfo, 0, sizeof(iav_enc_info2_t));
        memset(&mEncConfig, 0, sizeof(iav_enc_config_t));
        mbWaitEOS[0]=false;
        mbWaitEOS[1]=false;
        mbEOS[0]=false;
        mbEOS[1]=false;
        memset(mEOSPTS, 0, 2*sizeof(iav_udec_status_t));
        memset(mLastPTS, 0, 2*sizeof(iav_udec_status_t));
        mFrames[0]=0;
        mFrames[1]=0;
        mHasInput[0]=0;
        mHasInput[1]=0;
        pFrame2Render = &mFrame[0];
    }
    AM_ERR Construct();
    virtual ~CVideoTranscoder();
    AM_ERR SetInputFormat(CMediaFormat *pFormat);

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
    {
        if (refiid == IID_IVideoTranscoder)
            return (IVideoTranscoder*)this;
        if (refiid == IID_IClockObserver)
            return (IClockObserver*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete() { return inherited::Delete(); }

public:
    //IFilter
    virtual AM_ERR Run();
    virtual AM_ERR Start();

    virtual void GetInfo(INFO& info);
    virtual IPin* GetInputPin(AM_UINT index);
    virtual IPin* GetOutputPin(AM_UINT index);
    virtual AM_ERR AddInputPin(AM_UINT& index, AM_UINT type);

    // IActiveObject
    virtual void OnRun();
    void PrintState();

    // IClockObserver
    virtual AM_ERR OnTimer(am_pts_t curr_pts)
    {
        mpWorkQ->PostMsg(CMD_TIMERNOTIFY);
        return ME_OK;
    }

    //IVideoTranscoder
    virtual AM_ERR StopEncoding(AM_INT stopcode=2);
    virtual void setTranscFrms(AM_INT frames);
    //virtual AM_ERR SetEncoderParamaters(AM_UINT in_w, AM_UINT in_h, AM_UINT en_w, AM_UINT en_h, bool hasVout, bool ctrlfps, bool hasBfrm, AM_UINT bitrate);
    virtual AM_ERR SetEncParamaters(AM_UINT en_w, AM_UINT en_h, AM_UINT fpsBase, AM_UINT bitrate, bool hasBfrm);
    virtual AM_ERR SetTranscSettings(bool RenderHDMI, bool RenderLCD, bool PreviewEnc, bool ctrlfps);

private:
    AM_ERR InitEncoder();
    AM_ERR SetupEncoder();
    AM_ERR SetConfig();
    void DoStop();
    virtual bool ProcessCmd(CMD& cmd);
    AM_ERR ProcessBuffer();
    AM_ERR ProcessEOS();
    AM_ERR RenderBuffer();
    AM_ERR setupVideoTranscoder();
    AM_ERR GetOutPic(iav_transc_frame_t& frame, AM_INT Index);
    AM_ERR WaitAllInputBuffer();

private:
    int mIavFd;
    CVideoTranscoderInput* mpInputPin[2];
    CVideoTranscoderOutput* mpOutputPin;
    AM_UINT mTotalInputPinNumber;
    AM_INT mDspIndex;
    bool mbMemMapped;
    bool mbIsTranscoding;
    AM_UINT mWantedFrms;

    //clock ctrl
    IClockManager *mpClockManager;
    bool mbSetClockMgr;

    IBufferPool *mpBufferPool;
    iav_frm_buf_pool_info_t mFrmPoolInfo;

    iav_enc_info2_t mEncInfo;
    iav_enc_config_t mEncConfig;
    AM_UINT mRenderMethord;
    bool mbEnablePreview;
    bool mbUseDefaultEncInfo;
    bool mbUseDefaultEncConfig;
    bool mbCtrlFPS;
    AM_UINT mFpsBase;

    bool mbWaitEOS[2];//Get eos from udec, wait decpp's EOS
    bool mbEOS[2];//Get eos from decpp
    iav_udec_status_t mEOSPTS[2];
    iav_udec_status_t mLastPTS[2];
    CBuffer *mpBuffer[2];
    iav_transc_frame_t mFrame[2];
    iav_transc_frame_t *pFrame2Render;
    bool mbInputPin[2];//this pin is used
    AM_INT mHasInput[2];//this pin get pic nums: -1(eos), 0 or 1
    AM_UINT mFrames[2];
    bool mbStopped;
    CMutex *mpMutex;
};

//-----------------------------------------------------------------------
//
// CVideoTranscoderInput
//
//-----------------------------------------------------------------------
class CVideoTranscoderInput: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
    friend class CVideoTranscoder;

public:
    static CVideoTranscoderInput* Create(CFilter *pFilter);

protected:
    CVideoTranscoderInput(CFilter *pFilter):
        inherited(pFilter),
        mnFrames(0),
        mIndex(0)
    {}
    AM_ERR Construct();
    virtual ~CVideoTranscoderInput() {}

public:
    // IPin
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);

private:
    CMediaFormat mMediaFormat;
    AM_UINT mnFrames;

private:
    AM_UINT mIndex;
};

//-----------------------------------------------------------------------
//
// CVideoTranscoderOutput
//
//-----------------------------------------------------------------------
class CVideoTranscoderOutput: public COutputPin
{
    typedef COutputPin inherited;
    friend class CVideoTranscoder;

public:
    static CVideoTranscoderOutput* Create(CFilter *pFilter);

private:
    CVideoTranscoderOutput(CFilter *pFilter):
        inherited(pFilter), mnFrames(0)
    {
        mMediaFormat.pMediaType = &GUID_Video;
        mMediaFormat.pSubType = &GUID_AmbaVideoAVC;
        mMediaFormat.pFormatType = &GUID_NULL;
    }
    AM_ERR Construct();
    virtual ~CVideoTranscoderOutput() {}

public:
    // IPin
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
    {
        pMediaFormat = &mMediaFormat;
        return ME_OK;
    }

private:
    AM_ERR SetOutputFormat();

private:
    CMediaFormat mMediaFormat;
    AM_UINT mnFrames;
};

#endif // __VIDEO_TRANSCODER_H__
