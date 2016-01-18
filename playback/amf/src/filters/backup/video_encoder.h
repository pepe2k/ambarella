
/*
 * video_encoder.h
 *
 * History:
 *    2011/7/8 - [Jay Zhang] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __VIDEO_ENCODER_H__
#define __VIDEO_ENCODER_H__

//#define __print_time_info__

class CVideoEncoderOutput;

//===============================================
//Record struct support
//===============================================
#define MAX_NUM_STREAMS 4

typedef struct
{
    AM_INT stream_format;
    AM_UINT width;
    AM_UINT height;
    AM_UINT framerate;
    AM_UINT entropy_type;
    AM_UINT M, N, IDRInterval;
} StreamDesc;

typedef struct
{
    StreamDesc stream_info[MAX_NUM_STREAMS];
} EncoderConfig;

//-----------------------------------------------------------------------
//
// CVideoEncoder
//
//-----------------------------------------------------------------------
class CVideoEncoder: public CActiveFilter, public IVideoEncoder
{
    typedef CActiveFilter inherited;
    friend class CVideoEncoderOutput;

public:
    static IFilter* Create(IEngine *pEngine, AM_UINT request_stream_number);

private:
    CVideoEncoder(IEngine *pEngine, AM_UINT request_stream_nuber):
        inherited(pEngine, "VideoEncoder"),
        mIavFd(-1),
        mbMemMapped(false),
        mbStop(false),
        mStreamsMask(0x0),
        mVinAntiFlicker(-1),
        mVinMirrorPattern(-1),
        mVinMirrorMode(-1),
        mVinFrameRate(0),
        mVinSource(-1),
        mpVFile(NULL)
    {
        mnReservedBufferNum = 0;
        mStreamNum = request_stream_nuber;
        if (mStreamNum > MAX_NUM_STREAMS) {
            AM_ASSERT(0);
            AM_ERROR("request_stream_nuber > MAX_NUM_STREAMS in CVideoEncoder.\n");
            mStreamNum = MAX_NUM_STREAMS;
        }

        for(int i= 0; i < MAX_NUM_STREAMS; i++)
        {
            mpOutputPin[i] = NULL;
        }
        memset(&mConfig, 0, sizeof(mConfig));
        memset(&mH264Config, 0, sizeof(mH264Config));
        memset(&mFifoInfo, 0 , sizeof(bs_fifo_info_t));
    }
    AM_ERR Construct();
    virtual ~CVideoEncoder();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
    {
        if (refiid == IID_IVideoEncoder)
            return (IVideoEncoder*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete() {
        if (mIavFd >= 0) {
            close(mIavFd);
            mIavFd = -1;
        }
        return inherited::Delete();
    }

    // IFilter
    virtual AM_ERR FlowControl(FlowControlType type);
    virtual void GetInfo(INFO& info);
    virtual IPin* GetOutputPin(AM_UINT index);
    // IActiveObject
    virtual void OnRun();

    // IVideoEncoder
    AM_ERR StopEncoding()
    {
        Stop();
        return ME_OK;
    }
    AM_ERR ExitPreview();
    //IParameters
    virtual AM_ERR SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index = 0);

#ifdef AM_DEBUG
    virtual void PrintState();
#endif

private:
    AM_ERR SetConfig();
    void DoStop();
    virtual bool ProcessCmd(CMD& cmd);
    AM_ERR ReadInputData();
    AM_UINT GetFrameNum(AM_UINT index);
    AM_UINT GetAliveFrameNum(AM_UINT index);
    AM_ERR HasIDRFrame(AM_INT index);
    AM_ERR CheckBufferPool();
    AM_ERR DropFrame();
    AM_ERR DropByPause();
    AM_ERR NotifyFlowControl();
    AM_ERR ProcessBuffer();
    AM_ERR ProcessEOS();
    //AM_ERR SendOutInfo();
    void PrintH264Config(iav_h264_config_t *config);
    AM_ERR setupVideoEncoder();
    void DoFlowControl(FlowControlType type);

    void CheckSourceInfo(void);
    AM_ERR InitVin(enum amba_video_mode mode);

private:
    EncoderConfig mConfig;
    CVideoEncoderOutput* mpOutputPin[MAX_NUM_STREAMS];
    int mIavFd;
    bool mbMemMapped;
    iav_h264_config_t mH264Config;
    bs_fifo_info_t mFifoInfo;
    bool mbStop;

    AM_UINT mStreamNum;

private:
    AM_UINT mStreamsMask;

private:
    AM_UINT mnReservedBufferNum;

//vin related should move to IAV encoder interface wrapper?
private:
    //vin parameters
    AM_INT mVinAntiFlicker;
    AM_INT mVinMirrorPattern;
    AM_INT mVinMirrorMode;
    AM_INT mVinFrameRate;
    AM_INT mVinSource;

    //Debug
    IFileWriter *mpVFile;
};

//-----------------------------------------------------------------------
//
// CVideoEncoderOutput
//
//-----------------------------------------------------------------------
class CVideoEncoderOutput: public COutputPin
{
    typedef COutputPin inherited;
    friend class CVideoEncoder;

    enum {
        VideoEncOPin_Runing,
        VideoEncOPin_Paused,
        VideoEncOPin_tobePaused,
        VideoEncOPin_tobeResumed
    };

public:
    static CVideoEncoderOutput* Create(CFilter *pFilter);

private:
    CVideoEncoderOutput(CFilter *pFilter):
        inherited(pFilter), mnFrames(0), mbNeedSendFinalizeFile(false)
    {
        mMediaFormat.pMediaType = &GUID_Video;
        mMediaFormat.pSubType = &GUID_AmbaVideoAVC;
        mMediaFormat.pFormatType = &GUID_NULL;

        mpBSB = NULL;
        mTotalDrop = mPauseDrop = 0;
        mbSkip = mbBlock = false;
        mIDRDrop = 0;
        msState = VideoEncOPin_Runing;
        mpFlowControlList = NULL;
    }
    AM_ERR Construct();
    virtual ~CVideoEncoderOutput()
    {
        if (mpFlowControlList) {
            delete mpFlowControlList;
            mpFlowControlList = NULL;
        }
    }

public:
    // IPin
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
    {
        pMediaFormat = &mMediaFormat;
        return ME_OK;
    }

    //Flow Control related
    void ProcessBuffer(bits_info_t* p_desc);
    AM_ERR SendVideoBuffer(bits_info_t* p_desc);
    AM_ERR SendFlowControlBuffer(IFilter::FlowControlType type, AM_U64 pts = 0);

private:
    CMediaFormat mMediaFormat;
    AM_UINT mnFrames;

protected:
    AM_UINT mTotalDrop;
    AM_UINT mPauseDrop;
    bool mbSkip;
    bool mbBlock;
    CSimpleBufferPool* mpBSB;
    AM_UINT mIDRDrop;

    AM_UINT msState;

#ifdef __print_time_info__
    struct timeval tv_last_buffer;
#endif

protected:
    CList* mpFlowControlList;

    bool mbNeedSendFinalizeFile;
};

#endif

