
/*
 * video_encoder.h
 *
 * History:
 *    2012/02/24 - [Zhi He] create file
 *
 * Copyright (C) 2012, Ambarella, Inc.
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

#define DMAX_OSD_AREA_NUM 3
#define DMAX_OSD_BUFFER_MAX_SIZE (320*180)
#define DMAX_OSD_BUFFER_MAX_SIZE_CLUT (1920*400)

typedef struct {
    AM_INT area_id;
    AM_INT offset_x, offset_y;
    AM_INT max_width, max_height;
    AM_INT buffer_size;

    AM_U8* p_addr_y;
    AM_U8* p_addr_uv;
    AM_U8* p_addr_alpha_y;
    AM_U8* p_addr_alpha_uv;

    AM_U8 inited;
    AM_U8 reserved0, reserved1, reserved2;
} SOSDArea;

typedef struct {
    AM_INT area_id;
    AM_INT offset_x, offset_y;
    AM_INT width, height, pitch;
    AM_INT buffer_size;

    AM_U8* p_dsp_data[IAV_BLEND_AREA_BUFFER_NUMBER];//Y Cb Cr Alpha color index
    AM_U8* p_input_data;

    AM_U8* p_dsp_clut;
    AM_U8* p_clut;
    //AM_U32 priv_clut[256];

    AM_U8 clut_need_update;
    AM_U8 cur_buf_id;
    AM_U8 input_data_format;
    AM_U8 cur_clut_type;//0, use default yCbCr, 1, use input

    AM_U8 inited;
    AM_U8 reserved0;
    AM_U8 reserved1;
    AM_U8 reserved2;
} SOSDArea_CLUT;

typedef struct
{
    AM_INT stream_format;
    AM_UINT offset_x, offset_y;
    AM_UINT width;
    AM_UINT height;
    AM_UINT framerate;
    AM_UINT entropy_type;
    AM_UINT M, N, IDRInterval;

    AM_U8 requested;
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
    AM_UINT main_width, main_height;
    AM_U8 profile, level;

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

    virtual void OnReleaseBuffer(CBuffer *pBuffer);

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
        mpSimpleEncAPI(NULL),
        mDSPIndex(0),
        mFrameLifeTime(270000),//hard code default value, about 3 seconds
        mLastFrameExpiredTime(0),
        mpClockManager(NULL),
        mIavFd(-1),
        mbIavFdOwner(false),
        mbStop(false),
        mnOsdAreaNumber(0),
        mnTotalOsdBufferSize(0),
        mnOsdAreaNumberCLUT(0),
        mnTotalOsdBufferSizeCLUT(0),
        mbEnableTransparentColor(1),
        mReservedTransparentIndex(1),
        mOSDMode(EOSD_MODE_INIT),
        mbGetFirstPTS(false),
        mMaxStartPTS(3003*4),//hard code here
        mStartPTSOffset(0),
        mVinAntiFlicker(-1),
        mVinMirrorPattern(-1),
        mVinMirrorMode(-1),
        mVinFrameRate(0),
        mVinSource(-1),
        mpVFile(NULL),
        mpMutex(NULL),
        mnDumpCLUTCount(0),
        mnDumpColorCount(0)
    {
        mnReservedBufferNum = 0;
        mStreamNum = request_stream_nuber;
        if (mStreamNum > MAX_NUM_STREAMS) {
            AM_ASSERT(0);
            AM_ERROR("request_stream_nuber > MAX_NUM_STREAMS in CVideoEncoder.\n");
            mStreamNum = MAX_NUM_STREAMS;
        }
        memset(&mConfig, 0, sizeof(mConfig));
        for(AM_INT i= 0; i < MAX_NUM_STREAMS; i++) {
            mpOutputPin[i] = NULL;

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

            mConfig.stream_info[i].entropy_type = IAV_ENTROPY_CAVLC;
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
            memset(&mOsdBlendInfo, 0, sizeof(iav_osd_blend_info_t));
        }


        memset(mJpegFileName, 0x0, sizeof(mJpegFileName));
        strcpy(mJpegFileName, "test.jpeg");

        memset(&mOsdBlendInfo, 0x0, sizeof(mOsdBlendInfo));
        memset(&mOsdArea, 0x0, sizeof(mOsdArea));

        memset(&mOsdBlendInfoCLUT, 0x0, sizeof(mOsdBlendInfoCLUT));
        memset(&mOsdAreaCLUT, 0x0, sizeof(mOsdAreaCLUT));
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
    AM_ERR SetModeConfig(SEncodingModeConfig* pconfig);

    AM_ERR StopEncoding()
    {
        Stop();
        return ME_OK;
    }
    AM_ERR ExitPreview();

    virtual AM_ERR UpdatePreviewDisplay(AM_UINT width, AM_UINT height, AM_UINT pos_x, AM_UINT pos_y, AM_UINT alpha);
    virtual AM_ERR DemandIDR(AM_UINT out_index);
    virtual AM_ERR UpdateGOPStructure(AM_UINT out_index, AM_INT M, AM_INT N, AM_INT idr_interval);
    virtual AM_ERR SetDisplayMode(AM_UINT display_preview, AM_UINT display_playback, AM_UINT preview_vout, AM_UINT pb_vout);
    virtual AM_ERR GetDisplayMode(AM_UINT& display_preview, AM_UINT& display_playback, AM_UINT& preview_vout, AM_UINT& pb_vout);

    virtual AM_ERR CaptureRawData(AM_U8*& p_raw, AM_UINT target_width, AM_UINT target_height, IParameters::PixFormat pix_format);
    virtual AM_ERR CaptureJpeg(char* jpeg_filename, AM_UINT target_width, AM_UINT target_height);
    virtual AM_ERR CaptureYUVdata(AM_INT fd, AM_UINT& pitch, AM_UINT& height, SYUVData* yuvdata = NULL);

    virtual AM_INT AddOsdBlendArea(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height);
    virtual AM_ERR UpdateOsdBlendArea(AM_INT index, AM_U8* data, AM_INT width, AM_INT height);
    virtual AM_ERR RemoveOsdBlendArea(AM_INT index);

    virtual AM_INT AddOsdBlendAreaCLUT(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height);
    virtual AM_ERR UpdateOsdBlendAreaCLUT(AM_INT index, AM_U8* data, AM_U32* p_input_clut, AM_INT width, AM_INT height, IParameters::OSDInputDataFormat data_format);
    virtual AM_ERR RemoveOsdBlendAreaCLUT(AM_INT index);

    virtual AM_ERR FreezeResumeHDMIPreview(AM_INT flag);

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
    AM_ERR ProcessBuffer();
    AM_ERR ProcessEOS();

    AM_ERR SetConfigInCameraRecordMode();
    AM_ERR SetConfigInDuplexMode();
    AM_ERR enterDuplexMode();
    AM_ERR initEncoder(AM_UINT dspmode);
    AM_ERR setupVideoEncoder();
    void DoFlowControl(FlowControlType type);

    void CheckSourceInfo(void);
    AM_ERR InitVin(enum amba_video_mode mode);
    bool isVideoSettingMatched(iav_enc_info_t& enc_info);
    void testPrintConfig(StreamDesc* desc);
    bool findAvaiableArea(AM_INT& i);
    bool findAvaiableAreaCLUT(AM_INT& i);
    AM_ERR zeroOsdBlendAreaCLUT(AM_INT index);

private:
    ISimpleEncAPI* mpSimpleEncAPI;
    AM_UINT mDSPIndex;
    SEncodingModeConfig mModeConfig;
    SBitDescs mBitstreamDesc;

//for discard data if downstream filter is blocked or too slow
private:
    AM_UINT mFrameLifeTime;//estimate from BSB buffer size and DSP's bit rate
    AM_U64 mLastFrameExpiredTime;

private:
    EncoderConfig mConfig;
    CVideoEncoderOutput* mpOutputPin[MAX_NUM_STREAMS];
    IClockManager* mpClockManager;
    AM_INT mIavFd;
    bool mbIavFdOwner;
    bool mbStop;

    AM_UINT mStreamNum;

private:
    AM_INT mnOsdAreaNumber;
    AM_UINT mnTotalOsdBufferSize;

    iav_osd_blend_info_t mOsdBlendInfo[DMAX_OSD_AREA_NUM];
    SOSDArea mOsdArea[DMAX_OSD_AREA_NUM];
//    bool mbOsdInited;

    //CLUT mode
    AM_INT mnOsdAreaNumberCLUT;
    AM_UINT mnTotalOsdBufferSizeCLUT;
    iav_osd_blend_info_ex_t mOsdBlendInfoCLUT[DMAX_OSD_AREA_NUM];
    SOSDArea_CLUT mOsdAreaCLUT[DMAX_OSD_AREA_NUM];

    AM_U8 mbEnableTransparentColor;
    AM_U8 mReservedTransparentIndex;
    AM_U8 mOSDMode, mReserved1;
    AM_U32 mPreSetY4Cb2Cr2CLUT[256];
    //AM_U32 mPreSetR3G3B2CLUT[256];

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

    char mJpegFileName[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN + 4];
    //Debug
    IFileWriter *mpVFile;

    CMutex* mpMutex;

    //debug dump
    AM_UINT mnDumpCLUTCount;
    AM_UINT mnDumpColorCount;
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
        mbSkip = false;
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
    void ProcessBuffer(SBitDesc* p_desc, AM_U64 expired_time);
    AM_ERR SendVideoBuffer(SBitDesc* p_desc, AM_U64 expired_time);
    AM_ERR SendFlowControlBuffer(IFilter::FlowControlType type, AM_U64 pts = 0);

private:
    CMediaFormat mMediaFormat;
    AM_UINT mnFrames;

private:
    CSimpleBufferPool* mpBSB;
    bool mbSkip;
    AM_UINT msState;

#ifdef __print_time_info__
    struct timeval tv_last_buffer;
#endif

protected:
    CList* mpFlowControlList;

    bool mbNeedSendFinalizeFile;

    am_pts_t mStartPTSOffset;//start near zero, from dsp.
    am_pts_t mLastPTS;
    AM_UINT mPTSLoop;
    am_pts_t mLoopPTSOffset;
};

#endif

