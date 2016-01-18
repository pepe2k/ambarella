
/*
 * amba_video_decoder.h
 *
 * History:
 *    2010/9/30 - [Yu Jiankang] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBA_VIDEO_DECODER_H__
#define __AMBA_VIDEO_DECODER_H__

//debug check dram data(bit-stream buffer) is trashed
//#define DDEBUG_CHECK_DRAM_DATA_TRASH

class CAmbaVideoDecoder;
class CAmbaVideoInput;
class CAmbaVideoOutput;

//-----------------------------------------------------------------------
//
// CAmbaFrameBufferPool
//
//-----------------------------------------------------------------------
class CAmbaFrameBufferPool: public CSimpleBufferPool
{
    typedef CSimpleBufferPool inherited;

public:
    static CAmbaFrameBufferPool* Create(const char *name, AM_UINT count)
    {
        CAmbaFrameBufferPool *result = new CAmbaFrameBufferPool(name);
        if (result && result->Construct(count) != ME_OK) {
            delete result;
            result = NULL;
        }
        return result;
    }

protected:
    CAmbaFrameBufferPool(const char *name):
        inherited(name)
    {}
    AM_ERR Construct(AM_UINT count)
    {
        //for all mode, choose max size
        AM_UINT buffersize = sizeof(iav_decoded_frame_t) >= sizeof(iav_udec_status_t)? sizeof(iav_decoded_frame_t):sizeof(iav_udec_status_t);
        buffersize += sizeof(CVideoBuffer) + 4;//add 4 for safe
        AMLOG_INFO("CAmbaFrameBufferPool Construct, sizeof(iav_udec_status_t) %d, sizeof(iav_decoded_frame_t) %d, sizeof(CBuffer) %d, total %d.\n", sizeof(iav_udec_status_t), sizeof(iav_decoded_frame_t), sizeof(CBuffer), buffersize);
        return inherited::Construct(count, buffersize);
    }
    virtual ~CAmbaFrameBufferPool() {}

protected:
    //virtual void OnReleaseBuffer(CBuffer *pBuffer);
};

//-----------------------------------------------------------------------
//
// CAmbaVideoDecoder
//
//-----------------------------------------------------------------------
class CAmbaVideoDecoder: public CActiveFilter, IDecoderControl
{
    typedef CActiveFilter inherited;
    friend class CAmbaVideoInput;
    friend class CAmbaVideoOutput;
    enum{
            CMD_RECONFIG_DECODER = CMD_LAST + 200,
    };

public:
    static IFilter* Create(IEngine *pEngine);
    static int AcceptMedia(CMediaFormat& format);

private:
    CAmbaVideoDecoder(IEngine *pEngine):
        inherited(pEngine, "AmbaVideoDecoder"),
        mpVideoInputPin(NULL),
        mpVideoOutputPin(NULL),
        mbFillEOS(false),
        mIavFd(-1),
        mDspIndex(0),
        mbEnterUDECMode(false),
        mbConfigData(false),
        mpBuffer(NULL),
        mpBufferPool(NULL),
        mVoutConfigMask(((1<<eVoutHDMI)|(1<<eVoutLCD))),
        mbAddUdecWarpper(true),
        mSeqConfigDataLen(0),
        mPicConfigDataLen(0),
        mSeqConfigDataSize(0),
        mSeqConfigData(NULL),
        mH264DataFmt(H264_FMT_INVALID),
        mH264AVCCNaluLen(0),
        mbMP4WV1F(false),
        mbNeedFindVOLHead(false),
        last_dts(AV_NOPTS_VALUE),
        isAudPTSLeaped(0),
        mpDumpFile(NULL),
        mpDumpFileSeparate(NULL),
        mSpecifiedFrameRateTick(0),
        mInPrefetching(mpSharedRes->dspConfig.preset_enable_prefetch),
        mCurrentPrefetchCount(0),
        mpPrefetching(NULL),
        mH264SEIFound(false)
    {
        AM_ASSERT(mpSharedRes);
        if (mpSharedRes) {
            mVoutConfigMask = mpSharedRes->pbConfig.vout_config;
            AM_ASSERT(mVoutConfigMask == (1<<eVoutLCD) || mVoutConfigMask == (1<<eVoutHDMI) || (mVoutConfigMask == ((1<<eVoutHDMI)|(1<<eVoutLCD))));
        }

        //parse voutconfig
        mVoutNumber = 0;
        mVoutStartIndex = 0;
        mbESMode = 0;
#if 0
        if (mVoutConfigMask & (1<<eVoutLCD)) {
            mVoutNumber ++;
            mVoutStartIndex = eVoutLCD;
        } else {
            mVoutStartIndex = eVoutHDMI;
        }
        if (mVoutConfigMask & (1<<eVoutHDMI)) {
            mVoutNumber ++;
        }
        AMLOG_PRINTF("mVoutStartIndex %d, mVoutNumber %d, mVoutConfigMask 0x%x.\n", mVoutStartIndex, mVoutNumber, mVoutConfigMask);
#endif

        ::memset(mDumpFilename, 0x0, DAMF_MAX_FILENAME_LEN + 1);
#ifdef AM_DEBUG
        mdWriteSize = 0;
        mdRequestSize = 0;
        mdLastEndPointer = NULL;
        mdDSPCurrentPointer = NULL;
        get_dump_config(&mDumpStartFrame, &mDumpEndFrame);
        mDumpIndex = 0;
#endif
    }
    AM_ERR Construct();
    AM_ERR SetInputFormat(CMediaFormat *pFormat);
    AM_ERR ConfigVOUTVideo(AM_INT vout_id);
    AM_ERR ConfigVOUTMode(AM_INT vout_id);
    AM_ERR ConfigDecoder();
    AM_ERR HandleReConfigDecoder();
    void FeedConfigData(AVPacket *mpPacket);
    void FeedConfigDataWithUDECWrapper(AVPacket *mpPacket);
    //AM_ERR AllocBSB(AM_UINT size);
    u8 *CopyToBSB(AM_U8 *ptr, AM_U8 *buffer, AM_UINT size);
    AM_U8 *FillEOS(AM_U8 *ptr);
    //int RenderBuffer(AM_INT i);
    AM_ERR ProcessBuffer(CBuffer *pBuffer);
    AM_UINT _NextESDatapacket(AM_U8 * start, AM_U8 * end, AM_UINT * totFrames);
    AM_ERR ProcessTestESBuffer(CBuffer *pBuffer);
    AM_ERR DecodeBuffer(CBuffer *pBuffer, AM_INT numPics, AM_U8* pStart, AM_U8* pEnd);
    //AM_ERR WaitEOS(am_pts_t pts);
    bool ReadInputData();

    //AM_INT udec_create(void);
    void PrintBitstremBuffer(AM_U8* p, AM_UINT size);
    void GenerateConfigData();
    void UpdatePicConfigData(AVPacket *mpPacket);
    bool AdjustSeqConfigDataSize(AM_INT size);
    inline AM_U8* Find_StartCode(AM_U8* extradata, AM_INT size);
    inline AM_U8* Get_StartCode_info(AM_U8* porgaddr, AM_INT orgsize, AM_INT* pmdfsize);

    void SetUdecModeConfig();
    void SetUdecConfig(AM_INT index);
    AM_INT EnterUdecMode();
    AM_INT InitUdec(AM_INT index);
    AM_INT ReleaseUdec(AM_INT index);

    virtual ~CAmbaVideoDecoder();

public:

    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);

    // IActiveObject
    virtual void OnRun();

    // IFilter
    virtual AM_ERR Stop();

    // IDecoderControl
    virtual AM_ERR ReConfigDecoder(AM_INT flag);
    virtual AM_ERR AudioPtsLeaped();

    virtual void GetInfo(INFO& info);
    virtual IPin* GetInputPin(AM_UINT index);
    IPin* GetOutputPin(AM_UINT index);
    virtual bool ProcessCmd(CMD& cmd);
    //virtual IamPin* GetOutputPin(AM_UINT index);
    virtual void Delete();
#ifdef AM_DEBUG
    virtual void PrintState();
#endif

private:
    //AM_INT SendBufferMode1(iav_decoded_frame_t& frame);
    //AM_ERR AllocBSBMode1(AM_UINT size);
    AM_ERR AllocBSBMode(AM_UINT size);
    //AM_ERR MapDSP();
    //AM_ERR UnMapDSP();
    void dumpEsData(AM_U8* pStart, AM_U8* pEnd);

    bool CheckH264SPSExist(AM_U8 *pData, AM_INT iDataLen, AM_INT *pSPSPos, AM_INT *pSPSLen);
    bool Find_H264_SEI(AM_U8* data_base, AM_INT data_size);

    bool isVidResLargerThan1080P(){return (mpCodec->width>1920 && mpCodec->height>1080);}

#ifdef DDEBUG_CHECK_DRAM_DATA_TRASH
    void storeDramData(AM_UINT i);
    void checkDramDataTrashed();
#endif

private:
    CAmbaVideoInput *mpVideoInputPin;
    CAmbaVideoOutput *mpVideoOutputPin;
    bool mbFillEOS;
    bool mbVideoStarted;
    int mIavFd;
    AM_INT mDspIndex;
    bool mbEnterUDECMode;

    AVCodecContext *mpCodec;
    AVStream *mpStream;
    bool mbConfigData;

    AM_U8 *mpStartAddr;	// BSB start address
    AM_U8 *mpEndAddr;	// BSB end address
    AM_U8 *mpCurrAddr;	// current pointer to BSB
    AM_U32 mSpace;	// free space in BSB
    CBuffer *mpBuffer;
    IBufferPool *mpBufferPool;
    AM_INT muDecType;

private:
    iav_udec_mode_config_t mUdecModeConfig;
    iav_udec_deint_config_t mUdecDeintConfig;
    iav_udec_config_t mUdecConfig[DMAX_UDEC_INSTANCE_NUM];
//#if _use_iav_udec_info_t_
#if 0
    iav_udec_info_t mUdecInfo[DMAX_UDEC_INSTANCE_NUM];
#else
    iav_udec_info_ex_t mUdecInfo[DMAX_UDEC_INSTANCE_NUM];
#endif
    iav_udec_vout_config_t mUdecVoutConfig[eVoutCnt];
    AM_INT mMaxVoutWidth;
    AM_INT mMaxVoutHeight;

    AM_UINT mVoutConfigMask;    // vout mask bits: 1<<0 LCD; 1<<1 HDMI
    AM_UINT mVoutStartIndex;
    AM_UINT mVoutNumber;

private:
    bool mbAddUdecWarpper;
    AM_U8 mUSEQHeader[DUDEC_SEQ_HEADER_EX_LENGTH];
    AM_U8 mUPESHeader[DUDEC_PES_HEADER_LENGTH];
    //config data for each format, need re send seq data after seek
    AM_INT mSeqConfigDataLen;
    AM_UINT mPicConfigDataLen;
    //AM_U8 mSeqConfigData[DUDEC_MAX_SEQ_CONFIGDATA_LEN];
    AM_INT mSeqConfigDataSize;
    AM_U8 *mSeqConfigData;
    AM_U8 mPicConfigData[DUDEC_MAX_PIC_CONFIGDATA_LEN];
    H264_DATA_FORMAT mH264DataFmt;
    AM_INT mH264AVCCNaluLen;
    AM_UINT mbESMode;
    bool mbMP4WV1F;//for codec tag "WV1F"
    bool mbNeedFindVOLHead;//for extra data invalid issue, need find first vol header(0x00000120) in chunk
    AM_S64 last_dts;//for bug#1883 #1915,save last dts to catch DTS leap
    AM_UINT isAudPTSLeaped;
private:
    FILE* mpDumpFile;
    FILE* mpDumpFileSeparate;
    char mDumpFilename[DAMF_MAX_FILENAME_LEN + 1];
    AM_INT mRet;
    SVoutConfig mVoutConfig[eVoutCnt];//0:lcd, 1:hdmi
    amba_video_sink_mode sink_mode[eVoutCnt];//add for CMD_VOUT_VIDEO_SETUP

#ifdef AM_DEBUG
//check wrapper around
    AM_UINT mdWriteSize;//write size
    AM_UINT mdRequestSize;//write size
    AM_U8* mdLastEndPointer;
    AM_U8* mdDSPCurrentPointer;//dsp's read address
    AM_INT mDumpIndex;
    AM_INT mDumpStartFrame;
    AM_INT mDumpEndFrame;
#endif

//debug use
    AM_UINT mSpecifiedFrameRateTick;

#ifdef DDEBUG_CHECK_DRAM_DATA_TRASH
    AM_U8 *mpCheckDram[2];
    AM_UINT mNumberInt;
    AM_UINT mCheckCount;
#endif

private:
    AM_U8 mInPrefetching;
    AM_UINT mCurrentPrefetchCount;
    AM_U8* mpPrefetching;
    bool mH264SEIFound;

};

//-----------------------------------------------------------------------
//
// CAmbaVideoInput
//
//-----------------------------------------------------------------------
class CAmbaVideoInput: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
    friend class CAmbaVideoDecoder;

public:
    static CAmbaVideoInput* Create(CFilter *pFilter);

private:
    CAmbaVideoInput(CFilter *pFilter):
        inherited(pFilter)
    {}
    AM_ERR Construct();
    virtual ~CAmbaVideoInput();

public:
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
//    virtual AM_ERR ProcessBuffer(CBuffer *pBuffer);

//    virtual void OnRun();
};


//-----------------------------------------------------------------------
//
// CAmbaVideoOutput
//
//-----------------------------------------------------------------------
class CAmbaVideoOutput: COutputPin
{
    typedef COutputPin inherited;
    friend class CAmbaVideoDecoder;

public:
    static CAmbaVideoOutput *Create(CFilter *pFilter);

protected:
    CAmbaVideoOutput(CFilter *pFilter):
        inherited(pFilter)
        {}
    AM_ERR Construct();
    virtual ~CAmbaVideoOutput();

public:
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
    {
        pMediaFormat = &mMediaFormat;
        return ME_OK;
    }

private:
    AM_ERR SetOutputFormat();
    //void CalcVideoDimension(AVCodecContext *pCodec);

private:
    CFFMpegMediaFormat mMediaFormat;
};


#endif

