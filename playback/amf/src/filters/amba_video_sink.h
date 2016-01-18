
/*
 * amba_video_sink.h
 *
 * History:
 *    2012/02/22 - [Zhi He] create file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBA_VIDEO_SINK_H__
#define __AMBA_VIDEO_SINK_H__

class CAmbaVideoSink;
class CAmbaVideoSinkInput;

//-----------------------------------------------------------------------
//
// CAmbaVideoSink
//
//-----------------------------------------------------------------------
class CAmbaVideoSink: public CActiveFilter, public IClockObserver, IVideoOutput, IRender
{
    typedef CActiveFilter inherited;
    friend class CAmbaVideoSinkInput;

    //filter related sub state
    enum {
        STATE_WAIT_PROPER_START_TIME = LAST_COMMON_STATE + 1,
    };

public:
    static IFilter* Create(IEngine *pEngine);
    static AM_INT AcceptMedia(CMediaFormat& format);

private:
    CAmbaVideoSink(IEngine *pEngine):
        inherited(pEngine, "AmbaVideoSink"),
        mpClockManager(NULL),
        mpSimpleDecAPI(NULL),
        mbDecAPIStopped(false),
        mpVideoInputPin(NULL),
        mbFillEOS(false),
        mIavFd(-1),
        mbIavFdOwner(false),
        mDspIndex(0),
        mbEnterDuplexMode(false),
        mbConfigData(false),
        mpBuffer(NULL),
        mVoutConfigMask(((1<<eVoutHDMI)|(1<<eVoutLCD))),
        mbAddUdecWarpper(true),
        mSeqConfigDataLen(0),
        mPicConfigDataLen(0),
        mSeqConfigDataSize(0),
        mpSeqConfigData(NULL),
        mH264DataFmt(H264_FMT_INVALID),
        mH264AVCCNaluLen(0),
        mCurrentPbPTS(0),
        mBeginPbPTS(0),
        mbCurrentPbPTSValid(0),
        mDSPErrorLevel(0),
        mDSPErrorType(0),
        mbWaitKeyFrame(0),
        mbWaitFirstValidPTS(1),
        mbRecievedSyncCmd(0),
        mbAlreadySendSyncMsg(0),
        mEstimatedLatency(0),
        mbVideoStarted(false),
        mTimeOffset(0),
        mbDoAvSync(0),
        mbNeedSpeedUp(0)
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
        mbVoutForPB[eVoutLCD] = 0;
        mbVoutForPB[eVoutHDMI] = 0;

        ::memset((void*)mDisplayRectMap, 0x0, sizeof(mDisplayRectMap));

        get_dump_config(&mDumpStartFrame, &mDumpEndFrame);
        mDumpIndex = 0;
    }
    AM_ERR Construct();
    AM_ERR SetInputFormat(CMediaFormat *pFormat);
    AM_ERR ConfigDecoder();
    void FeedConfigData(AVPacket *mpPacket);
    void FeedConfigDataWithUDECWrapper(AVPacket *mpPacket);
    u8 *CopyToBSB(AM_U8 *ptr, AM_U8 *buffer, AM_UINT size);
    AM_U8 *FillEOS(AM_U8 *ptr);

    AM_ERR ProcessBuffer(CBuffer *pBuffer);
    AM_UINT _NextESDatapacket(AM_U8 * start, AM_U8 * end, AM_UINT * totFrames);
    AM_ERR ProcessTestESBuffer();
    AM_ERR DecodeBuffer(CBuffer *pBuffer, AM_INT numPics, AM_U8* pStart, AM_U8* pEnd);

    bool ReadInputData();

    void PrintBitstremBuffer(AM_U8* p, AM_UINT size);
    void GenerateConfigData();
    bool AdjustSeqConfigDataSize(AM_INT size);

    virtual ~CAmbaVideoSink();

public:

    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);

    // IActiveObject
    virtual void OnRun();

    // IClockObserver
    virtual AM_ERR OnTimer(am_pts_t curr_pts);

    // IFilter
    virtual AM_ERR Start() {mpWorkQ->Start(); return ME_OK;};
    virtual AM_ERR Stop();

    virtual void GetInfo(INFO& info);
    virtual IPin* GetInputPin(AM_UINT index);
    virtual bool ProcessCmd(CMD& cmd);
    //virtual IamPin* GetOutputPin(AM_UINT index);
    virtual void Delete();
#ifdef AM_DEBUG
    virtual void PrintState();
#endif

    //IVideoOutput
    virtual AM_ERR Step() { return ME_NO_IMPL; }
    //vout config
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
    //source/dest rect and scale mode config
    virtual AM_ERR SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h);
    virtual AM_ERR SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h);
    virtual AM_ERR SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y);
    virtual AM_ERR SetDeWarpControlWidth(AM_UINT enable, AM_UINT width_top, AM_UINT width_bottom) {return ME_NO_IMPL;}

    virtual AM_ERR GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs);

private:
    AM_INT configVout(AM_INT vout, AM_INT video_rotate, AM_INT video_flip, AM_INT target_pos_x, AM_INT target_pos_y, AM_INT target_width, AM_INT target_height);
    AM_INT SetInputCenter(AM_INT pic_x, AM_INT pic_y);

private:
    AM_ERR enterDuplexMode(void);
    void updateStatus();
    void clearDecoder();
    void onStateWaitProperStartTime();
    void speedUp();
    void dumpEsData(AM_U8* pStart, AM_U8* pEnd);

private:
    IClockManager* mpClockManager;
    ISimpleDecAPI* mpSimpleDecAPI;
    bool mbDecAPIStopped;

private:
    CAmbaVideoSinkInput *mpVideoInputPin;
    bool mbFillEOS;
    //bool mbVideoStarted;
    AM_INT mIavFd;
    bool mbIavFdOwner;
    AM_UINT mDspIndex;
    bool mbEnterDuplexMode;

    AVCodecContext *mpCodec;
    AVStream *mpStream;
    bool mbConfigData;

    CMutex *mpMutex;

    AM_U8 *mpStartAddr;	// BSB start address
    AM_U8 *mpEndAddr;	// BSB end address
    AM_U8 *mpCurrAddr;	// current pointer to BSB
    AM_U32 mSpace;	// free space in BSB
    CBuffer *mpBuffer;
    AM_INT muDecType;

private:
    iav_udec_mode_config_t mUdecModeConfig;
    iav_udec_deint_config_t mUdecDeintConfig;
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
    //AM_U8 mpSeqConfigData[DUDEC_MAX_SEQ_CONFIGDATA_LEN];
    AM_INT mSeqConfigDataSize;
    AM_U8 *mpSeqConfigData;
    AM_U8 mPicConfigData[DUDEC_MAX_PIC_CONFIGDATA_LEN];
    H264_DATA_FORMAT mH264DataFmt;
    AM_INT mH264AVCCNaluLen;
    AM_UINT mbESMode;

private:
    SVoutConfig mVoutConfig[eVoutCnt];//0:lcd, 1:hdmi
    SDisplayRectMap mDisplayRectMap[eVoutCnt];
    AM_UINT mbVoutForPB[eVoutCnt];
    SDecoderParam mDecParam;

private:
    am_pts_t mCurrentPbPTS;
    am_pts_t mBeginPbPTS;
    AM_U8 mbCurrentPbPTSValid;
    AM_U8 mDSPErrorLevel;
    AM_U16 mDSPErrorType;

private:
    AM_U8 mbWaitKeyFrame;

    //avsync related
    AM_U8 mbWaitFirstValidPTS;
    AM_U8 mbRecievedSyncCmd;
    AM_U8 mbAlreadySendSyncMsg;

    AM_UINT mEstimatedLatency;

    bool        mbVideoStarted;
    am_pts_t    mTimeOffset;//time offset of seek pointer

    AM_U8 mbDoAvSync;
    AM_U8 mbNeedSpeedUp;
    AM_U8 mReserved1[2];

private:
    FILE* mpDumpFile;
    FILE* mpDumpFileSeparate;
    char mDumpFilename[DAMF_MAX_FILENAME_LEN + 1];

    AM_INT mDumpIndex;
    AM_INT mDumpStartFrame;
    AM_INT mDumpEndFrame;
};

//-----------------------------------------------------------------------
//
// CAmbaVideoSinkInput
//
//-----------------------------------------------------------------------
class CAmbaVideoSinkInput: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
    friend class CAmbaVideoSink;

public:
    static CAmbaVideoSinkInput* Create(CFilter *pFilter);

private:
    CAmbaVideoSinkInput(CFilter *pFilter):
        inherited(pFilter)
    {}
    AM_ERR Construct();
    virtual ~CAmbaVideoSinkInput();

public:
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
};

#endif

