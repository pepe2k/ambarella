
/*
 * video_mem_encoder.h
 *
 * History:
 *    2011/7/29 - [GangLiu] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __VIDEO_MEM_ENCODER_H__
#define __VIDEO_MEM_ENCODER_H__

#define IAV_ENC_BUFFER 0x08
class CVideoMemEncoder;
class CVideoMemEncoderInput;
class CVideoMemEncoderOutput;

//-----------------------------------------------------------------------
//
//  CVideoMemEncoderBufferPool
//
//-----------------------------------------------------------------------
class CVideoMemEncoderBufferPool: public CSimpleBufferPool
{
    typedef CSimpleBufferPool inherited;
    friend class CVideoMemEncoder;
public:
    static CVideoMemEncoderBufferPool* Create(const char* name, AM_UINT count)
    {
        CVideoMemEncoderBufferPool* result = new CVideoMemEncoderBufferPool(name);
        if(result && result->Construct(count) != ME_OK)
        {
            delete result;
            result = NULL;
        }
        return result;
    }
    void setIavFd(AM_INT fd)
    { iavfd = fd; }
protected:
    CVideoMemEncoderBufferPool(const char* name):
        inherited(name),
        iavfd(-1)
        {}
    AM_ERR Construct(AM_UINT count)
    {
        return inherited::Construct(count);
    }
    virtual ~CVideoMemEncoderBufferPool() {}

protected:
    virtual void OnReleaseBuffer(CBuffer *pBuffer);
private:
    AM_INT iavfd;
};

//-----------------------------------------------------------------------
//
// CVideoMemEncoder
//
//-----------------------------------------------------------------------
class CVideoMemEncoder: public CActiveFilter, public IVideoMemEncoder
{
    typedef CActiveFilter inherited;
    friend class CVideoMemEncoderInput;
    friend class CVideoMemEncoderOutput;

public:
    static IFilter* Create(IEngine *pEngine);

private:
    CVideoMemEncoder(IEngine *pEngine):
        inherited(pEngine, "VideoMemEncoder"),
        mFrameLifeTime(270000),//hard code default value, about 3 seconds
        mLastFrameExpiredTime(0),
        mpInputPin(NULL),
        mpOutputPin(NULL),
        mpClockManager(NULL),
        mIavFd(-1),
        mDspIndex(-1),
        mbWaitEOS(false),
        mbEOS(false),
        mbEncoderStarted(false),
        mFrames(0)
    {
        //memset(&mFifoInfo, 0 , sizeof(bs_fifo_info_t));
        memset(&mFrame, 0 , sizeof(mFrame));
    }
    AM_ERR Construct();
    virtual ~CVideoMemEncoder();
    AM_ERR SetInputFormat(CMediaFormat *pFormat);


public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
    {
        if (refiid == IID_IVideoMemEncoder)
            return (IVideoMemEncoder*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete() { return inherited::Delete(); }
    // IFilter
    virtual void GetInfo(INFO& info);
    virtual IPin* GetInputPin(AM_UINT index);

    virtual IPin* GetOutputPin(AM_UINT index);
    // IActiveObject
    virtual void OnRun();
    void PrintState();

    // IVideoMemEncoder
    //AM_ERR StopEncoding();

private:
    AM_ERR SetConfig();
    AM_ERR StartEncoder();
    //AM_ERR DoStop();
    virtual bool ProcessCmd(CMD& cmd);
    AM_ERR ReadInputData();
    AM_ERR ProcessBuffer();
    AM_ERR ProcessEOS();
    AM_ERR setupVideoMemEncoder();

//for discard data if downstream filter is blocked or too slow
protected:
    AM_UINT mFrameLifeTime;//estimate from BSB buffer size and DSP's bit rate
    AM_U64 mLastFrameExpiredTime;

private:
    CVideoMemEncoderInput* mpInputPin;
    CVideoMemEncoderOutput* mpOutputPin;
    IClockManager* mpClockManager;
    int mIavFd;
    AM_INT mDspIndex;
    bool mbMemMapped;
    iav_h264_config_t mH264Config;
    //bs_fifo_info_t mFifoInfo;
    iav_frame_desc_t mFrame;
    bool mbWaitEOS;
    bool mbEOS;
    bool mbEncoderStarted;
    iav_frm_buf_pool_info_t mFrmPoolInfo;
    AM_UINT mFrames;

    CBuffer *mpBuffer;
    IBufferPool *mpBufferPool;

    //Debug
#ifdef RECORD_TEST_FILE
    IFileWriter *mpVFile;
#endif
};

//-----------------------------------------------------------------------
//
// CVideoMemEncoderInput
//
//-----------------------------------------------------------------------
class CVideoMemEncoderInput: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
    friend class CVideoMemEncoder;

public:
    static CVideoMemEncoderInput* Create(CFilter *pFilter);

private:
    CVideoMemEncoderInput(CFilter *pFilter):
        inherited(pFilter), mnFrames(0)
    {}
    AM_ERR Construct();
    virtual ~CVideoMemEncoderInput() {}

public:
    // IPin
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);

private:
    CMediaFormat mMediaFormat;
    AM_UINT mnFrames;
};

//-----------------------------------------------------------------------
//
// CVideoMemEncoderOutput
//
//-----------------------------------------------------------------------
class CVideoMemEncoderOutput: public COutputPin
{
    typedef COutputPin inherited;
    friend class CVideoMemEncoder;

public:
    static CVideoMemEncoderOutput* Create(CFilter *pFilter);

private:
    CVideoMemEncoderOutput(CFilter *pFilter):
        inherited(pFilter), mnFrames(0)
    {
        mMediaFormat.pMediaType = &GUID_Video;
        mMediaFormat.pSubType = &GUID_AmbaVideoAVC;
        mMediaFormat.pFormatType = &GUID_NULL;
    }
    AM_ERR Construct();
    virtual ~CVideoMemEncoderOutput() {}

public:
    // IPin
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
    {
        pMediaFormat = &mMediaFormat;
        return ME_OK;
    }

private:
	AM_ERR SetOutputFormat();

protected:
    CVideoMemEncoderBufferPool* mpBSB;
private:
    CMediaFormat mMediaFormat;
    AM_UINT mnFrames;
};

#endif // __VIDEO_MEM_ENCODER_H__
