
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
#define IAV_ENC_BUFFER 0x08
#define MAX_NUM_STREAMS 4

typedef struct
{
    AM_INT stream_format;
    AM_UINT width;
    AM_UINT height;
    AM_UINT framerate;
    AM_UINT entropy_type;
    AM_UINT M, N, IDRInterval;

    AM_U8 gop_model;
    AM_U8 bitrate_control;
    AM_U8 calibration;

    AM_U8 vbr_ness;
    AM_U8 min_vbr_rate_factor;
    AM_U16 max_vbr_rate_factor;

    AM_U32 average_bitrate;
} StreamDesc;

typedef struct
{
    StreamDesc stream_info[MAX_NUM_STREAMS];
} EncoderConfig;


//-----------------------------------------------------------------------
//
//  CVideoEncoderBufferPool
//
//-----------------------------------------------------------------------
class CVideoEncoderBufferPool: public CSimpleBufferPool
{
    typedef CSimpleBufferPool inherited;
    friend class CVideoEncoder;
public:
    static CVideoEncoderBufferPool* Create(const char* name, AM_UINT count, AM_INT fd)
    {
        CVideoEncoderBufferPool* result = new CVideoEncoderBufferPool(name, fd);
        if(result && result->Construct(count) != ME_OK)
        {
            delete result;
            result = NULL;
        }
        return result;
    }
protected:
    CVideoEncoderBufferPool(const char* name, AM_INT fd):
        inherited(name),
        iavfd(fd)
        {}
    AM_ERR Construct(AM_UINT count)
    {
        return inherited::Construct(count);
    }
    virtual ~CVideoEncoderBufferPool() {}

protected:
    virtual void OnReleaseBuffer(CBuffer *pBuffer);
private:
    AM_INT iavfd;
};

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
        mFrameLifeTime(270000),//hard code default value, about 3 seconds
        mLastFrameExpiredTime(0),
        mpClockManager(NULL),
        mIavFd(-1),
        mbMemMapped(false),
        mbStop(false),
        mEos(0),
        mStreamsMask(0x0),
        mbOsdInited(false),
        mbGetFirstPTS(false),
        mMaxStartPTS(3003*4),//hard code here
        mStartPTSOffset(0),
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
        memset(&mConfig, 0, sizeof(mConfig));
        for(int i= 0; i < MAX_NUM_STREAMS; i++) {
            mpOutputPin[i] = NULL;
            memset(&mEncConfig[i], 0, sizeof(iav_enc_config_t));

            //default parameters
            mConfig.stream_info[i].stream_format = IAV_ENCODE_H264;
            if (i == 0) {
                //main stream
                mConfig.stream_info[i].width = DefaultMainVideoWidth;
                mConfig.stream_info[i].height = DefaultMainVideoHeight;
            } else {
                //preview c?
                mConfig.stream_info[i].width = DefaultPreviewCWidth;
                mConfig.stream_info[i].height = DefaultPreviewCHeight;
            }
            mConfig.stream_info[i].framerate = 30;

            mConfig.stream_info[i].entropy_type = IAV_ENCODE_H264;
            mConfig.stream_info[i].M = DefaultH264M;
            mConfig.stream_info[i].N = DefaultH264N;
            mConfig.stream_info[i].IDRInterval = DefaultH264IDRInterval;
            mConfig.stream_info[i].gop_model = IAV_GOP_SIMPLE;
            mConfig.stream_info[i].bitrate_control = IAV_BRC_CBR;

            mConfig.stream_info[i].calibration = 100;
            mConfig.stream_info[i].vbr_ness = 50;
            mConfig.stream_info[i].min_vbr_rate_factor = 50;
            mConfig.stream_info[i].max_vbr_rate_factor = 300;

            if (i == 0) {
                mConfig.stream_info[i].average_bitrate = 4000000; // 4M
            } else {
                mConfig.stream_info[i].average_bitrate = 2000000; // 2M
            }
        }
        memset(&mOsdBlendInfo, 0, sizeof(iav_osd_blend_info_t));

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
    virtual void Delete();

    // IFilter
    virtual AM_ERR FlowControl(FlowControlType type);
    virtual void GetInfo(INFO& info);
    virtual IPin* GetOutputPin(AM_UINT index);
    // IActiveObject
    virtual void OnRun();
    // IVideoEncoder
    AM_ERR SetModeConfig(SEncodingModeConfig* pconfig) {return ME_OK;}
    // IVideoEncoder
    AM_ERR StopEncoding()
    {
        Stop();
        return ME_OK;
    }
    AM_ERR ExitPreview();

    virtual AM_ERR UpdatePreviewDisplay(AM_UINT width, AM_UINT height, AM_UINT pos_x, AM_UINT pos_y, AM_UINT alpha) {AM_ERROR("add implement.\n"); return ME_NO_IMPL;}
    virtual AM_ERR DemandIDR(AM_UINT out_index) {AM_ERROR("add implement.\n"); return ME_NO_IMPL;}
    virtual AM_ERR UpdateGOPStructure(AM_UINT out_index, int M, int N, int idr_interval) {AM_ERROR("add implement.\n"); return ME_NO_IMPL;}

    //OSD blending, only 1 area here
    virtual AM_ERR addOsdBlendArea(int xPos, int yPos, int width, int height, void** addr_y, void** addr_uv, void** a_addr_y, void** a_addr_uv);
    virtual AM_ERR updateOsdBlendArea();
    virtual AM_ERR removeOsdBlendArea();
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
    AM_ERR NotifyFlowControl();
    AM_ERR ProcessBuffer();
    AM_ERR ProcessEOS();
    //AM_ERR SendOutInfo();
    void PrintH264Config(iav_enc_config_t *config);
    AM_ERR setupVideoEncoder();
    void DoFlowControl(FlowControlType type);

    void CheckSourceInfo(void);
    AM_ERR InitVin(enum amba_video_mode mode);
    bool isVideoSettingMatched(iav_enc_info_t& enc_info);
    void testPrintConfig(StreamDesc* desc);

//for discard data if downstream filter is blocked or too slow
protected:
    AM_UINT mFrameLifeTime;//estimate from BSB buffer size and DSP's bit rate
    AM_U64 mLastFrameExpiredTime;

private:
    EncoderConfig mConfig;
    CVideoEncoderOutput* mpOutputPin[MAX_NUM_STREAMS];
    IClockManager* mpClockManager;
    AM_INT mIavFd;
    bool mbMemMapped;
    iav_enc_config_t mEncConfig[MAX_NUM_STREAMS];
    iav_frame_desc_t mFrame;
    bool mbStop;

    AM_UINT mStreamNum;
    AM_UINT mEos;
private:
    AM_UINT mStreamsMask;
private:
    iav_osd_blend_info_t mOsdBlendInfo;//only 1 area now
    bool mbOsdInited;

private:
    AM_UINT mnReservedBufferNum;

private:
    bool mbGetFirstPTS;
    am_pts_t mMaxStartPTS;
    am_pts_t mStartPTSOffset;//start near zero, from dsp.

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
        inherited(pFilter), mnFrames(0), mbNeedSendFinalizeFile(false), mStartPTSOffset(0)
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
    void ProcessBuffer(iav_frame_desc_t* p_desc, AM_U64 expired_time);
    AM_ERR SendVideoBuffer(iav_frame_desc_t* p_desc, AM_U64 expired_time);
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

    am_pts_t mStartPTSOffset;//start near zero, from dsp.
};

#endif

