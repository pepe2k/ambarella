
/*
 * dualreadstream.h
 *
 * History:
 *    2011/8/3 - [Chong Xing] create file
 *    2011/8/22- [Qiongxiong Z] modify file
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __DUAL_READSTREAM_H__
#define __DUAL_READSTREAM_H__

#include "mqueue.h"
#include "aic.h"
#include "aic_message.h"
//#include "ambastreamer/Ambastream_handler.hh"


#define MAX_NUM_STREAMS     4
#define STREAM_VIDEO_1ST     0
#define STREAM_VIDEO_2ND   2
#define STREAM_AUDIO_1ST    1
#define STREAM_AUDIO_2ND  3
#define DBGMSG AM_DBG

#define ENABLE_VIDEOSTREAM 0x00000001
#define ENABLE_AUDIOSTREAM 0x00000002
#define ENABLE_VIDEOSTREAM_2nd 0x00000004
#define ENABLE_AUDIOSTREAM_2nd 0x00000008

#define	VIDEO_MODE_DUAL			(0)
#define	VIDEO_MODE_NORMAL		(1)
#define	VIDEO_MODE_VIDEOONLY		(6)
#define	VIDEO_MODE_STREAMING		(7)

#define REC_STREAMING_OUT_PRI		(1)
#define REC_STREAMING_OUT_SEC		(2)
#define REC_STREAMING_OUT_TER		(3)
#define REC_STREAMING_OUT_DUAL		(4)

#define SAVE_ALLOC_MEM_NUM 64
#define CBUFFER_ALLOCED 0x08

typedef enum {
	DSP_ENC_29_97_I =   0,
	DSP_ENC_29_97_P =   1,
	DSP_ENC_59_94_P =   3,
	DSP_ENC_15      =  15,
	DSP_ENC_24      =  24,
	DSP_ENC_25      =  25,
	DSP_ENC_30      =  30,
	DSP_ENC_50      =  50,
	DSP_ENC_60      =  60,
	DSP_ENC_100     = 100,
	DSP_ENC_119_88  = 119,
	DSP_ENC_120     = 120,
	DSP_ENC_180     = 180,
	DSP_ENC_200     = 200,
	DSP_ENC_240     = 240,
	DSP_ENC_300     = 300
} dsp_enc_frame_rate;

enum sensor_video_res_id_e {
	SENSOR_VIDEO_RES_TRUE_1080P_FULL = 0,	/** 00 */
	SENSOR_VIDEO_RES_TRUE_1080P_HALF,
	SENSOR_VIDEO_RES_TRUE_1080I,
	SENSOR_VIDEO_RES_COMP_1080P_FULL,
	SENSOR_VIDEO_RES_COMP_1080P_HALF,
	SENSOR_VIDEO_RES_COMP_1080I,			/** 05 */
	SENSOR_VIDEO_RES_HD_FULL,
	SENSOR_VIDEO_RES_HD_HALF,

	SENSOR_VIDEO_RES_SDWIDE,
	SENSOR_VIDEO_RES_SD,
	SENSOR_VIDEO_RES_CIF,					/** 10 */
	SENSOR_VIDEO_RES_WVGA_FULL,
	SENSOR_VIDEO_RES_WVGA_HALF,
	SENSOR_VIDEO_RES_VGA,
	SENSOR_VIDEO_RES_WQVGA,
	SENSOR_VIDEO_RES_QVGA,					/** 15 */

	SENSOR_VIDEO_RES_PRESET_0,

	SENSOR_VIDEO_RES_PHOTO,

	SENSOR_VIDEO_RES_QVGA_P120,
	SENSOR_VIDEO_RES_QVGA_P180,
	SENSOR_VIDEO_RES_QVGA_P240,				/** 20 */
	SENSOR_VIDEO_RES_WQVGA_P100,
	SENSOR_VIDEO_RES_WQVGA_P120,
	SENSOR_VIDEO_RES_WQVGA_P180,
	SENSOR_VIDEO_RES_WQVGA_P200,
	SENSOR_VIDEO_RES_WQVGA_P240,			/** 25 */

	SENSOR_VIDEO_RES_WQVGA_HFR_1,
	SENSOR_VIDEO_RES_WQVGA_HFR_2,

	SENSOR_VIDEO_RES_NUM
};

typedef struct
{
    AM_INT stream_format;
    AM_UINT width;
    AM_UINT height;
    AM_UINT framerate;
    AM_UINT channel_number;
    AM_UINT sample_rate;
    AM_UINT a_bitrate;
} StreamParams;

typedef struct
{
    StreamParams smParam[MAX_NUM_STREAMS];
    AM_UINT aIndex;
    AM_UINT vIndex;
} EncoderConfig;

using namespace android;
class CEncoderITronOutput;
//-----------------------------------------------------------------------
//
// AmbaIPCStreamer, Get data form itron with amba ipc RPCL
//
//-----------------------------------------------------------------------
class AmbaIPCStreamer
{
private:
    Aic *mAic;
    am_msg_t *msg;
    am_res_t *res;
    int mAic_token;
    int encode_status;

public:
    static AmbaIPCStreamer *create();
    int init(void);
    ~AmbaIPCStreamer();

    int createToken(void);
    int releaseToken(void);
    int init_AmbaStreamer(void);
    int release_AmbaStreamer(void);
    int enable_AmbaStreamer(int type);
    int disable_AmbaStreamer(int type);
    int is_EncFrameValid(int type);
    int get_EncFrameDataInfo(int type, AmbaStream_frameinfo_t *frameInfo);
    int get_EncFrameData(int type, u8 **Framedata, unsigned int *Datalength);
    int notify_read_done(int type);
    int get_EncSPSPPS(int type, u8 *SPS, int *SPS_len, u8 *PPS, int *PPS_len, unsigned int *ProfileID);
    int config_video_only(void);
    int config_for_Streamout(int mode);
    int start_encode(void);
    int stop_encode(void);
    int switch_to_VideoMode(void);

    int config_for_Streamin(int Streams);
    int switch_to_PBStream(void);
    int switch_to_DUPLEXStream(void);
    int get_DecRemainFrameBufSize(int type);
    int set_DecFrameDataInfo(int type, AmbaStream_frameinfo_t *frameInfo);
    int set_DecFrameType(int type, AmbaStream_frameinfo_t *frameInfo);

    int set_video_wh(int v_resid);
    int set_video_framerate(int frames_per_second);
    int set_audio_channel_num(int numberOfChannels);
    int set_audio_bitrate(int64_t bitrate);
    int set_audio_samplerate(int64_t samplingRate);

    int notify_write_done(int type);
    int get_DecFrameDataWP(int type, u8 **FrameData, u8 **base, u8 **limit);
    int canfeedDecFrame(int type);
private:
    AmbaIPCStreamer();
    int wait_for_reply(unsigned int reply_msg);

};

//-----------------------------------------------------------------------
//
//BufferPool For CVideoEncoderITron
//
//-----------------------------------------------------------------------
class CVideoEncoderITronBP: public CSimpleBufferPool
{
    typedef CSimpleBufferPool inherited;

public:
    static CVideoEncoderITronBP* Create(const char *name, AM_UINT count);

protected:
    CVideoEncoderITronBP(const char *name):
        inherited(name)
    {}
    AM_ERR Construct(AM_UINT count)
    {
        return inherited::Construct(count, sizeof(CBuffer));
    }
    virtual ~CVideoEncoderITronBP() {}

protected:
    virtual void OnReleaseBuffer(CBuffer *pBuffer);
};
//-----------------------------------------------------------------------
//
// CVideoEncoderITron, encode by itron dual way video/audio
//
//-----------------------------------------------------------------------
class CVideoEncoderITron: public CActiveFilter, public IVideoEncoder
{
    typedef CActiveFilter inherited;
    friend class CEncoderITronOutput;

    enum {
        VideoEncoderITron_bufferCountInPool = 64,
    };

public:
    static IFilter* Create(IEngine* pEngine, bool bDual);

private:
    CVideoEncoderITron(IEngine *pEngine, bool bDual):
        inherited(pEngine, "VideoEncoderITron"),
        mStreamer(NULL),
        mMaxStreamIndexPlus1(0),
        mbReadDual(false),
        mCheckVaild(0)
    {
        AM_INT i;
        for(i= 0; i < MAX_NUM_STREAMS; i++)
        {
            memset(&mFrameInfo[i], 0, sizeof(AmbaStream_frameinfo_t));
            mDataPtr[i] = NULL;
            mDataLen[i] = 0;
            mpBSB[i] = NULL;
            mpOutputPin[i] = NULL;
            mTotalDrop[i] = mPauseDrop[i] = mIDRDrop[i] = 0;
            mbSkip[i] = false;
            mbBlock[i] = false;
            mGetData[i] = 0;
        }

        mConfig.vIndex = 0;
        mConfig.aIndex = 1;
    }
    AM_ERR Construct();
    virtual ~CVideoEncoderITron();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete() { inherited::Delete();}

    // IFilter
    virtual void GetInfo(INFO& info);
    virtual IPin* GetOutputPin(AM_UINT index);
    // IVideoEncoder
    AM_ERR StopEncoding()
    {
        Stop();
        return ME_OK;
    }
    AM_ERR ExitPreview()
    {
        AMLOG_WARN(" CVideoEncoderITron::ExitPreview, not implement here.\n");
        return ME_OK;
    }
    virtual AM_ERR SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index = 0);

    // IActiveObject
    virtual void OnRun();
protected:
    virtual bool ProcessCmd(CMD& cmd);
    //void DoPause();
public:
    AM_ERR Init();
    AM_ERR SetOutput();
private:
    void DoStop();
    AM_ERR ReadInputData();
    AM_ERR ProcessEOS();
    AM_ERR ProcessBuffer();
    AM_ERR IsIDRFrame(AM_UINT index);
    AM_ERR GenerateOutBuffer(AM_UINT index);

    AM_ERR ReadStreamerData();
    AM_ERR HasIDRFrame(AM_INT index);
    AM_ERR CheckBufferPool();
    AM_ERR DropFrame();
    AM_ERR DropByPause();
    AM_ERR NotifyFlowControl(FlowControlType);

    AM_ERR ConfigVideoParams(int index);
    AM_ERR ConfigITronEnc();

public:
    AM_ERR AddOutputPin(AM_UINT& index, AM_UINT type);

private:
    AmbaIPCStreamer* mStreamer;
    AmbaStream_frameinfo_t mFrameInfo[MAX_NUM_STREAMS];
    AM_U8* mDataPtr[MAX_NUM_STREAMS];
    AM_UINT mDataLen[MAX_NUM_STREAMS];
    CBuffer* mpBuffer;

    CSimpleBufferPool* mpBSB[MAX_NUM_STREAMS];
    CEncoderITronOutput* mpOutputPin[MAX_NUM_STREAMS];

    //AM_UINT mStreamNum;
    AM_UINT mMaxStreamIndexPlus1;
    bool mbReadDual;
    AM_UINT mGetData[MAX_NUM_STREAMS];
    bool mbSkip[MAX_NUM_STREAMS];
    bool mbBlock[MAX_NUM_STREAMS];
    //Debug
    AM_UINT mIDRDrop[MAX_NUM_STREAMS];
    AM_UINT mTotalDrop[MAX_NUM_STREAMS];
    AM_UINT mPauseDrop[MAX_NUM_STREAMS];

    AM_UINT mCheckVaild;


    AM_U64 mVideoTime_Curr;
    AM_U64 mVideoTime_Prev;
    AM_U64 mAudioTime_Curr;
    AM_U64 mAudioTime_Prev;
    AM_INT mEncode_status;

    EncoderConfig mConfig;
    //AM_UINT mConfigIndex;
};
//-----------------------------------------------------------------------
//
// CEncoderITronOutput
//
//-----------------------------------------------------------------------
class CEncoderITronOutput: public COutputPin
{
	typedef COutputPin inherited;
	friend class CVideoEncoderITron;

public:
	static CEncoderITronOutput* Create(CFilter *pFilter);
public:
    // IPin
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
    {
        pMediaFormat = &mMediaFormat;
        return ME_OK;
    }

protected:
	CEncoderITronOutput(CFilter *pFilter);
	AM_ERR Construct();
	virtual ~CEncoderITronOutput();
private:
    CMediaFormat mMediaFormat;
    AM_UINT mnFrames;

protected:
    AM_UINT mPreDefIndex;
};


#endif

