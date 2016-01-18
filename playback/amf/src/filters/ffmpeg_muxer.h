/*
 * ffmpeg_muxer.h
 *
 * History:
 *    2011/7/21 - [Zhi He] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FFMPEG_MUXER_H__
#define __FFMPEG_MUXER_H__

#include <unistd.h>
#include "fixed_mempool.h"
//#define __print_time_info__
//
class CFFMpegMuxer;
class CFFMpegMuxerInput;

#define MaxStreamNumber  8
#define DMaxFileExterntionLength 32
#define DMaxFileIndexLength 32

enum {
    RTP_VERSION = 2,

    RTP_PAYLOAD_TYPE_PRIVATE = 96,
    RTP_PT_H264 = RTP_PAYLOAD_TYPE_PRIVATE,
    RTP_PT_AAC,
};

//aligned to 16 byte, and less than 1500
#define DRecommandMaxUDPPayloadLength 1440
#define DRecommandMaxRTPPayloadLength (DRecommandMaxUDPPayloadLength - 32)

static const char *_getContainerString(IParameters::ContainerType type)
{
    switch (type) {
        case IParameters::MuxerContainer_MP4://default
            return "mp4";
        case IParameters::MuxerContainer_3GP:
            return "3gp";
        case IParameters::MuxerContainer_TS:
            return "ts";
        case IParameters::MuxerContainer_MOV:
            return "mov";
        case IParameters::MuxerContainer_AVI:
            return "avi";
        case IParameters::MuxerContainer_AMR:
            return "amr";
        case IParameters::MuxerContainer_MKV:
            return "mkv";
        default:
            AM_ERROR("unknown container type.\n");
            return "???";
    }
}

enum {
    eAudioObjectType_AAC_MAIN = 1,
    eAudioObjectType_AAC_LC = 2,
    eAudioObjectType_AAC_SSR = 3,
    eAudioObjectType_AAC_LTP = 4,
    eAudioObjectType_AAC_scalable = 6,
    //add others, todo

    eSamplingFrequencyIndex_96000 = 0,
    eSamplingFrequencyIndex_88200 = 1,
    eSamplingFrequencyIndex_64000 = 2,
    eSamplingFrequencyIndex_48000 = 3,
    eSamplingFrequencyIndex_44100 = 4,
    eSamplingFrequencyIndex_32000 = 5,
    eSamplingFrequencyIndex_24000 = 6,
    eSamplingFrequencyIndex_22050 = 7,
    eSamplingFrequencyIndex_16000 = 8,
    eSamplingFrequencyIndex_12000 = 9,
    eSamplingFrequencyIndex_11025 = 0xa,
    eSamplingFrequencyIndex_8000 = 0xb,
    eSamplingFrequencyIndex_7350 = 0xc,
    eSamplingFrequencyIndex_escape = 0xf,//should not be this value
};

//refer to iso14496-3
typedef struct
{
    AM_U8 samplingFrequencyIndex_high : 3;
    AM_U8 audioObjectType : 5;
    AM_U8 bitLeft : 3;
    AM_U8 channelConfiguration : 4;
    AM_U8 samplingFrequencyIndex_low : 1;
} __attribute__((packed))SSimpleAudioSpecificConfig;


//filename handling
enum {
    eFileNameHandling_noAppendExtention,//have '.'+'known externtion', like "xxx.mp4", "xxx.3gp", "xxx.ts"
    eFileNameHandling_appendExtention,//have no '.' + 'known externtion'
};

enum {
    eFileNameHandling_noInsert,
    eFileNameHandling_insertFileNumber,//have '%', and first is '%d' or '%06d', like "xxx_%d.mp4", "xxx_%06d.mp4"
    eFileNameHandling_insertDateTime,//have '%t', and first is '%t', will insert datetime, like "xxx_%t.mp4" ---> "xxx_20111223_115503.mp4"
};

//-----------------------------------------------------------------------
//
// CFFMpegMuxerInput
//
//-----------------------------------------------------------------------
class CFFMpegMuxerInput: public CQueueInputPin, public IParameters
{
    typedef CQueueInputPin inherited;
    friend class CFFMpegMuxer;

    enum {
        DefaultPTSDTSGapDivideDuration = 4,
    };

public:
    typedef struct {
        AM_UINT extradata_size;
        AM_U8*  p_extradata;

        AM_UINT format;
        AM_INT codec_id;

        AM_UINT bitrate;
        AM_UINT reserved;

        UFormatSpecific specific;
    } SCodecParams;

public:
    static CFFMpegMuxerInput* Create(CFilter *pFilter);

private:
    CFFMpegMuxerInput(CFilter *pFilter):
        inherited(pFilter),
        mDiscardPacket(0),
        mIndex(0),
        mpStream(NULL),
        mType(StreamType_Invalid),
        mEOSComes(0),
        mpPrivateData(NULL),
        mStartPTS(0),
        mSessionInputPTSStartPoint(0),
        mSessionPTSStartPoint(0),
        mCurrentPTS(0),
        mCurrentDTS(0),
        mLastPTS(0),
        mDuration(0),
        mInputSatrtPTS(0),
        mInputEndPTS(0),
        mTotalFrameCount(0LL),
        mCurrentFrameCount(0LL),
        mSession(0),
        mTimeUintNum(1),
        mTimeUintDen(IParameters::TimeUnitDen_90khz),
        mNeedModifyPTS(0),
        mbSkiped(false),
        mpDumper(NULL),
        mPTSDTSGap(0),
        mPTSDTSGapDivideDuration(DefaultPTSDTSGapDivideDuration),
        mExpectedDuration(0),
        mFrameCountFromLastBPicture(0),
        mLastBPicturePTS(0),
        mpCachedBuffer(NULL),
        mbAutoBoundaryReached(false),
        mbAutoBoundaryStarted(false)
    {
        memset(&mParams, 0, sizeof(mParams));
        mParams.format = StreamFormat_Invalid;
    }
    AM_ERR Construct();
    virtual ~CFFMpegMuxerInput();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
    {
        if (refiid == IID_IParameters)
            return (IParameters*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete() { return inherited::Delete(); }

    // IParameters
    AM_ERR SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index = 0);

    // CInputPin
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat) {return ME_OK;}

    // CFFMpegMuxerInput
    AM_UINT NeedDiscardPacket() {return mDiscardPacket;}
    void DiscardPacket(AM_UINT discard) {mDiscardPacket = discard;}
    void PrintState() {AMLOG_PRINTF("CFFMpegMuxerInput state: mDiscardPacket %d, mType %d, mStreamFormat %d, mStreamIndex %d.\n", mDiscardPacket, mType, mParams.format, mIndex);}
    void PrintTimeInfo() { AMLOG_PRINTF(" Time info: mStartPTS %llu, mCurrentPTS %llu, mCurrentDTS %llu.\n", mStartPTS, mCurrentPTS, mCurrentDTS);
            AMLOG_PRINTF("      mCurrentFrameCount %llu, mTotalFrameCount %llu, mSession %u, mDuration%u.\n", mCurrentFrameCount, mTotalFrameCount, mSession, mDuration);
    }

    void timeDiscontinue();

protected:
    AM_UINT mDiscardPacket;
    AM_UINT mIndex;

protected:
    AVStream* mpStream;

protected:
    AM_UINT mType;
    SCodecParams mParams;
    AM_UINT mEOSComes;

protected:
    AM_U8* mpPrivateData;

protected:
    AM_U64 mStartPTS;
    AM_U64 mSessionInputPTSStartPoint;//join two start points, make PTS continus when pause/resume
    AM_U64 mSessionPTSStartPoint;
    AM_U64 mCurrentPTS;
    AM_U64 mCurrentDTS;
    AM_U64 mLastPTS;//debug only
    AM_UINT mDuration;
    AM_UINT mAVNormalizedDuration;//use av's unit

    AM_U64 mInputSatrtPTS;
    AM_U64 mInputEndPTS;

    AM_U64 mTotalFrameCount;
    AM_U64 mCurrentFrameCount;
    AM_UINT mSession;

protected:
    AM_UINT mTimeUintNum;
    AM_UINT mTimeUintDen;

protected:
    AM_UINT mNeedModifyPTS;

protected:
    bool mbSkiped;

protected:
    IFileWriter* mpDumper;

//some error concealment code, expect: "PTS not very correct" will not cause recording fail, only generate warning msg
//muxer should be robust, code would be not beautiful
protected:
    //DTS generator logic: write pts = pts + ptsdts gap, dts = pts(sync B picture comes), dts = previous dts + duration
    AM_UINT mPTSDTSGap;
    AM_UINT mPTSDTSGapDivideDuration;
    //calculated from frame rate, debug only
    AM_UINT mExpectedDuration;

    AM_UINT mFrameCountFromLastBPicture;
    AM_U64 mLastBPicturePTS;

protected:
    CBuffer* mpCachedBuffer;//saving for Next File
    bool mbAutoBoundaryReached;
    bool mbAutoBoundaryStarted;
};

//-----------------------------------------------------------------------
//
// CFFMpegMuxer
//
//-----------------------------------------------------------------------
class CFFMpegMuxer: public CActiveFilter, public IMuxer, public IMuxerControl, public IStreammingDataTransmiter
{
    typedef CActiveFilter inherited;
    friend class CFFMpegMuxerInput;

    enum {
        MaxDTSDrift = 4,

        //check preset flag
        EPresetCheckFlag_videoframecount = 0x1,
        EPresetCheckFlag_audioframecount = 0x2,
        EPresetCheckFlag_filesize = 0x4,
    };

    enum {
        //cut file with according to master input, not very precise pts, but less latency for write file/streaming
        STATE_SAVING_PARTIAL_FILE = LAST_COMMON_STATE + 1,
        //with more precise pts, (sub-states), with much latency for write new file/streaming
        STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_MASTER_PIN,
        STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN,
        STATE_SAVING_PARTIAL_FILE_WAIT_MASTER_PIN,
        STATE_SAVING_PARTIAL_FILE_WAIT_NON_MASTER_PIN,

        STATE_SAVING_PRESET_TOTAL_FILE,

        STATE_FLUSH_EXPIRED_FRAME,
    };

public:
    static IFilter* Create(IEngine *pEngine);

protected:
    CFFMpegMuxer(IEngine *pEngine):
        inherited(pEngine, "CFFMpeg_Muxer"),
        mContainerType(MuxerContainer_MP4),
        mTotalInputPinNumber(0),
        mpFormat(NULL),
        mpOutFormat(NULL),
        mpFileWriter(NULL),
        mpMutex(NULL),
        mpOutputFileName(NULL),
        mpThumbNailFileName(NULL),
        mOutputFileNameLength(0),
        mThumbNailFileNameLength(0),
        mpInputBuffer(NULL),

        mnSession(0),
        mStreamStartPTS(0),

        //mIncommingPTSThreshHold(0),
        mpMasterInput(NULL),
        mpClockManager(NULL),
        mCurrentFileIndex(1),//start from 1, request from app
        mFirstFileIndexCanbeDeleted(0),
        mTotalFileNumber(0),
        mMaxTotalFileNumber(0),
        mpOutputFileNameBase(NULL),
        mpThumbNailFileNameBase(NULL),
        mOutputFileNameBaseLength(0),
        mThumbNailFileNameBaseLength(0),
        mSavingFileStrategy(IParameters::MuxerSavingFileStrategy_ToTalFile),
        mSavingCondition(IParameters::MuxerSavingCondition_InputPTS),
        mAutoFileNaming(IParameters::MuxerAutoFileNaming_ByNumber),
        mAutoSaveFrameCount(DefaultAutoSavingParam),
        mAutoSavingTimeDuration(DefaultAutoSavingParam* IParameters::TimeUnitDen_90khz),
        mNextFileTimeThreshold(DefaultAutoSavingParam* IParameters::TimeUnitDen_90khz),
        mAudioNextFileTimeThreshold(0),
        mIDRFrameInterval(0),
        mVideoFirstPTS(0),
        mbNextFileTimeThresholdSet(false),
        mbPTSStartFromZero(true),
        mPresetVideoMaxFrameCnt(DDefaultPresetMaxFrameCount),
        mPresetAudioMaxFrameCnt(DDefaultPresetMaxFrameCount),
        mPresetMaxFilesize(DDefaultPreSetMaxFileSize),
        mCurrentTotalFilesize(0LL),
        mPresetCheckFlag(0),
        mPresetReachType(0),
        isHD(false),
        mLastVideoStreammingTime(0LL),
        mLastAudioStreammingTime(0LL),
        mLastVideoRTPSeqNum(0),
        mLastAudioRTPSeqNum(0),
        mbStreammingEnabled(false),
        mbRTPSocketSetup(false),
        mbAudioRTPSocketSetup(false),
        mRTPSocket(-1),
        mRTCPSocket(-1),
        mRTPPort(DefaultRTPServerPortBase),
        mRTCPPort(DefaultRTPServerPortBase + 1),
        mAudioRTPSocket(-1),
        mAudioRTCPSocket(-1),
        mAudioRTPPort(DefaultRTPServerPortBase + 2),
        mAudioRTCPPort(DefaultRTPServerPortBase + 3),
        mpRTPBuffer(NULL),
        mRTPBufferLength(0),
        mRTPBufferTotalLength(0),
        mRTPAudioTimeStamp(0),
        mRtpTimestamp(0),
        mFileNameHanding1(eFileNameHandling_appendExtention),
        mFileNameHanding2(eFileNameHandling_noInsert),
        mFileDuration(0),
        mFileBitrate(0),
        mbMasterStarted(false),
        mSyncedStreamPTSDTSGap(0),
        mbMuxerPaused(false)
    {
        AM_UINT i = 0;
        for (; i < MaxStreamNumber; i++) {
            mpInput[i] = NULL;
        }
        mpConsistentConfig = (SConsistentConfig*)pEngine->mpOpaque;
        AM_ASSERT(mpConsistentConfig);
        memset(&mAVFormatParam, 0, sizeof(mAVFormatParam));

        mRTPPort = mpConsistentConfig->rtsp_server_config.rtp_rtcp_port_start;
        mRTCPPort = mpConsistentConfig->rtsp_server_config.rtp_rtcp_port_start + 1;
        mAudioRTPPort = mpConsistentConfig->rtsp_server_config.rtp_rtcp_port_start + 2;
        mAudioRTCPPort = mpConsistentConfig->rtsp_server_config.rtp_rtcp_port_start + 3;

        rtp_thread = -1;
        mRtpMemPool = NULL;

        mSyncAudioPTS.audioPTSoffset = 0;
        mSyncAudioPTS.lastVideoPTS = 0;
        mSyncAudioPTS.lastAudioPTS = 0;
        mSyncAudioPTS.needmodifyPTS = false;
    }
    AM_ERR Construct();
    virtual ~CFFMpegMuxer();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();

    // IFilter
    virtual void GetInfo(INFO& info);
    virtual IPin* GetInputPin(AM_UINT index);
    virtual AM_ERR AddInputPin(AM_UINT& index, AM_UINT type);

    // CActiveFilter
    virtual void Pause() {mpWorkQ->SendCmd(CMD_PAUSE);}
    virtual void Resume() {mpWorkQ->SendCmd(CMD_RESUME);}

    virtual AM_ERR SetOutputFile(const char *pFileName);
    virtual AM_ERR SetThumbNailFile(const char *pThumbaNailFileName);

    //IMuxerControl
    virtual AM_ERR SetContainerType(IParameters::ContainerType type);
    virtual AM_ERR SetSavingStrategy(IParameters::MuxerSavingFileStrategy strategy, IParameters::MuxerSavingCondition condition, IParameters::MuxerAutoFileNaming naming, AM_UINT param = DefaultAutoSavingParam, bool PTSstartFromZero = true);
    virtual AM_ERR SetMaxFileNumber(AM_UINT max_file_num);
    virtual AM_ERR SetTotalFileNumber(AM_UINT total_file_num);
    virtual AM_ERR SetPresetMaxFrameCount(AM_U64 max_frame_count, IParameters::StreamType type = IParameters::StreamType_Video);
    virtual AM_ERR SetPresetMaxFilesize(AM_U64 max_file_size);
    AM_ERR SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index = 0);
    virtual void OnRun();

    //IStreammingDataTransmiter
    virtual AM_ERR AddDstAddress(SDstAddr* p_addr, IParameters::StreamType type = IParameters::StreamType_Video);
    virtual AM_ERR RemoveDstAddress(SDstAddr* p_addr, IParameters::StreamType type = IParameters::StreamType_Video);
    virtual AM_ERR SetSrcPort(AM_U16 port, AM_U16 port_ext, IParameters::StreamType type = IParameters::StreamType_Video);
    virtual AM_ERR GetSrcPort(AM_U16& port, AM_U16& port_ext, IParameters::StreamType type = IParameters::StreamType_Video);
    virtual AM_ERR Enable(bool enable) { AUTO_LOCK(mpMutex); mbStreammingEnabled = enable; return ME_OK;}
    virtual AM_U16 GetRTPLastSeqNumber(IParameters::StreamType type);
    virtual void GetRTPLastTime(AM_U64& last_time, IParameters::StreamType type);

#ifdef AM_DEBUG
    virtual void PrintState();
#endif

public:
//new api for customized inputpin number
    AM_ERR Finalize();
    AM_ERR Initialize();
    AM_UINT GetSpsPpsLen(AM_U8 *pBuffer, AM_UINT* offset);
    void SendEOS();
    bool ReadInputData(CFFMpegMuxerInput* inputpin, CBuffer::Type& type);
    bool ProcessCmd(CMD& cmd);

//RTP related
protected:
    void rtpProcessData(AM_U8* srcbuf, AM_UINT srclen, AM_UINT timestamp,IParameters::StreamFormat format);
    AM_ERR setupUDPSocket(IParameters::StreamType type);
    //void sendPacketViaUDP(AM_U8* buf, AM_UINT len, AM_UINT is_last);
    void nalSend(const AM_U8 *buf, AM_UINT size, AM_UINT last,AM_UINT timestamp);
    void aacSend(const AM_U8 *buf, AM_UINT size,AM_UINT timestamp);

private:
    void analyseFileNameFormat(char* pstart);
    void updateFileName(AM_UINT file_index = 0, bool simpleautofilename = false);
    AM_ERR setParametersToMuxer(CFFMpegMuxerInput* pInput);
    AM_ERR setVideoParametersToMuxer(CFFMpegMuxerInput* pInput);
    AM_ERR setAudioParametersToMuxer(CFFMpegMuxerInput* pInput);
    AM_ERR setSubtitleParametersToMuxer(CFFMpegMuxerInput* pInput);
    AM_ERR setPrivateDataParametersToMuxer(CFFMpegMuxerInput* pInput);
    bool allInputEos();
    void writeVideoBuffer(CFFMpegMuxerInput* pInput, CBuffer *pBuffer);
    void writeAudioBuffer(CFFMpegMuxerInput* pInput, CBuffer *pBuffer);
    void writePridataBuffer(CFFMpegMuxerInput* pInput, CBuffer *pBuffer);

    void clearFFMpegContext();
    void checkVideoParameters();
    bool isCommingPTSNotContinuous(CFFMpegMuxerInput* pInput, CBuffer *pBuffer);
    bool isCommingBufferAutoFileBoundary(CFFMpegMuxerInput* pInputPin, CBuffer* pBuffer);

    bool isReachPresetConditions(CFFMpegMuxerInput* pinput);
    bool hasPinNeedReachBoundary(AM_UINT& i);

    void deletePartialFile(AM_UINT file_index);
    void getFileInformation();
    void checkParameters();

private:
    IParameters::ContainerType mContainerType;
    AM_UINT mTotalInputPinNumber;
    CFFMpegMuxerInput* mpInput[MaxStreamNumber];

    AVFormatContext* mpFormat;
    AVFormatParameters mAVFormatParam;
    AVOutputFormat* mpOutFormat;
    IFileWriter *mpFileWriter;
    CMutex *mpMutex;

private:
    char* mpOutputFileName;
    char* mpThumbNailFileName;
    AM_UINT mOutputFileNameLength;
    AM_UINT mThumbNailFileNameLength;

private:
    CBuffer* mpInputBuffer;

#ifdef __print_time_info__
protected:
    struct timeval tv_last_buffer;
#endif

private:
    AM_UINT mnSession;
    AM_U64 mStreamStartPTS;

private:
    SConsistentConfig* mpConsistentConfig;

//for separate file
private:
    //AM_U64 mIncommingPTSThreshHold;
    CFFMpegMuxerInput* mpMasterInput;//pin which will send finalize_control_buffer
    IClockManager*   mpClockManager;
    AM_UINT mCurrentFileIndex;//max
    AM_UINT mFirstFileIndexCanbeDeleted;//min
    AM_UINT mTotalFileNumber;
    AM_UINT mMaxTotalFileNumber;

    //skychen, 2012_7_17. mTotalRecFileNumber: the rec file num, when file num >= mTotalRecFileNumber, engine need to stop recording.
    AM_UINT mTotalRecFileNumber;
    char* mpOutputFileNameBase;
    char* mpThumbNailFileNameBase;
    AM_UINT mOutputFileNameBaseLength;
    AM_UINT mThumbNailFileNameBaseLength;

//use to modify audio pts after re-open audio HAL
private:
    struct {
	AM_U64 lastAudioPTS;
	AM_U64 lastVideoPTS;
	AM_U64 audioPTSoffset;
	bool needmodifyPTS;
    } mSyncAudioPTS;
    bool mbAudioHALClosed;

private:
    IParameters::MuxerSavingFileStrategy mSavingFileStrategy;
    IParameters::MuxerSavingCondition mSavingCondition;
    IParameters::MuxerAutoFileNaming mAutoFileNaming;
    AM_UINT mAutoSaveFrameCount;
    AM_U64 mAutoSavingTimeDuration;
    AM_U64 mNextFileTimeThreshold;
    //chuchen,2012_5_17
    AM_U64 mAudioNextFileTimeThreshold;
    AM_UINT mIDRFrameInterval;
    AM_U64 mVideoFirstPTS;//video stream first pts and audio stream first pts is 0
    AM_U64 mAudioLastPTS;//debug

    bool mbNextFileTimeThresholdSet;
    bool mbPTSStartFromZero;
//for preset
private:
    AM_U64 mPresetVideoMaxFrameCnt;
    AM_U64 mPresetAudioMaxFrameCnt;
    AM_U64 mPresetMaxFilesize;
    AM_U64 mCurrentTotalFilesize;
    AM_UINT mPresetCheckFlag;
    AM_UINT mPresetReachType;

private:
    //video is hd, tmp use
    bool isHD;

private:
    CDoubleLinkedList mVideoDstAddressList;
    CDoubleLinkedList mAudioDstAddressList;
    AM_U64 mLastVideoStreammingTime;
    AM_U64 mLastAudioStreammingTime;
    AM_U16 mLastVideoRTPSeqNum;
    AM_U16 mLastAudioRTPSeqNum;
    bool mbStreammingEnabled;

private:
    bool mbRTPSocketSetup;
    bool mbAudioRTPSocketSetup;
    AM_INT mRTPSocket;
    AM_INT mRTCPSocket;
    AM_U16 mRTPPort;
    AM_U16 mRTCPPort;

    AM_INT mAudioRTPSocket;
    AM_INT mAudioRTCPSocket;
    AM_U16 mAudioRTPPort;
    AM_U16 mAudioRTCPPort;

private:
    AM_U8* mpRTPBuffer;
    AM_UINT mRTPBufferLength;
    AM_UINT mRTPBufferTotalLength;
    AM_U32 mRTPAudioTimeStamp;
    AM_U32 mRtpTimestamp;
    bool mIsFirst;
private:
    //rtp/rtcp receive
    void ReceiveRtpRtcpPackets(int is_video);
    void notifyRtcpPacket(int is_video,unsigned char *buf, int len, struct sockaddr_in *from);
private:
    time_t mLastTime;
    AM_U16 mFileNameHanding1;
    AM_U16 mFileNameHanding2;

//file information
private:
    AM_U64 mFileDuration;
    AM_UINT mFileBitrate;

private:
    bool mbMasterStarted;

//av sync related
private:
    AM_UINT mSyncedStreamPTSDTSGap;

//for pause/resume muxer
private:
    bool mbMuxerPaused;//important param to control finalize/initialize file at pause/resume processing

//for rtp/rtcp-send optimization
private:
    CFixedMemPool *mRtpMemPool;
    enum {MAX_DESTADDR_NUM = 4};
    enum {RTPBUF_RESERVED_SIZE = 4};
    enum {TYPE_NONE,TYPE_VIDEO_RTP,TYPE_VIDEO_RTCP,TYPE_AUDIO_RTP,TYPE_AUDIO_RTCP};
    struct RtpDest{
        int sock_fd;
        int addr_count;
        int rtp_over_rtsp[MAX_DESTADDR_NUM];
        struct sockaddr_in addr[MAX_DESTADDR_NUM];
        struct {
           IRtspCallback *callback;
           int channel;
           int rtsp_fd;
        }rtsp[MAX_DESTADDR_NUM];
        unsigned int rtp_ts[MAX_DESTADDR_NUM];
        unsigned int rtp_seq[MAX_DESTADDR_NUM];
        unsigned int rtp_ssrc[MAX_DESTADDR_NUM];
    };

    enum {SEND_INTERVAL = 10000};//usec, 10ms
    enum {MAX_SEND_PACKET_COUNT = 16};
    enum {MAX_SEND_OCT_COUNT = 50 * 1024};
    struct RTPQueue {
         RTPQueue():head(NULL),tail(NULL),handler_(NULL){
             mNodePool = CFixedMemPool::Create(sizeof(RTP_NODE),512);
             if(!mNodePool){
                 AM_ERROR("RTPQueue() error, failed to init mempool\n");
                 return;
             }
             CFixedMemPool::MemBlock nodeBlock;
             if(mNodePool->mallocBlock(nodeBlock) < 0){
                 AM_ERROR("RTPQueue() error, failed to alloc node\n");
                 return;
             }
             RTP_NODE *node = (RTP_NODE*)nodeBlock.buf;
             memset(node,0,sizeof(RTP_NODE));
             node->node = nodeBlock;
             node->next = NULL;
             head = tail = node;
             pthread_mutex_init(&q_h_lock,NULL);
             pthread_mutex_init(&q_t_lock,NULL);
        }
        ~RTPQueue(){
            if(mNodePool){
                if(head) mNodePool->freeBlock(head->node);
                mNodePool->Delete();
                mNodePool = NULL;
            }
            pthread_mutex_destroy(&q_h_lock);
            pthread_mutex_destroy(&q_t_lock);
        }
        void enqueue(RtpDest *dest,CFixedMemPool::MemBlock  &block, int size,int data_type, unsigned int duration = 0){
            CFixedMemPool::MemBlock nodeBlock;
            if(!mNodePool || mNodePool->mallocBlock(nodeBlock) < 0){
                AM_ERROR("RTPQueue -- enqueue failed to alloc memory for node\n");
                return;
            }
            RTP_NODE *node = (RTP_NODE*)nodeBlock.buf;
            memset(node,0,sizeof(RTP_NODE));
            node->node = nodeBlock;
            if(size > 0){
                node->block = block;
                node->len = size;
                node->data_type = data_type;
                node->duration = duration;
                node->dest = *dest;
                node->next = NULL;
           }
           pthread_mutex_lock(&q_t_lock);
           tail->next = node;
           tail = node;
           pthread_mutex_unlock(&q_t_lock);
       }
      int dequeue(CFixedMemPool::MemBlock &block,int *len,int *data_type){
          RTP_NODE *node,*new_head;
          pthread_mutex_lock(&q_h_lock);
          node = head;
          new_head = node->next;
          if(new_head == NULL){
              pthread_mutex_unlock(&q_h_lock);
              return 0;
          }
          block = new_head->block;
          if(len) *len = new_head->len;
          if(data_type) *data_type = new_head->data_type;

          new_head->len = 0;
          head = new_head;
          pthread_mutex_unlock(&q_h_lock);
          if(mNodePool){
              mNodePool->freeBlock(node->node);
          }
          return 1;
      }
       void set_handler(CFFMpegMuxer *handler){
          handler_ = handler;
       }
       int dequeue_send(int &packet_count,int &oct_count){
          RTP_NODE *node,*new_head;
          pthread_mutex_lock(&q_h_lock);
          node = head;
          new_head = node->next;
          if(new_head == NULL){
              pthread_mutex_unlock(&q_h_lock);
              return 0;
          }
          if(new_head->len > 0){
               handler_->rtp_send_data(& new_head->dest,new_head->block.buf + RTPBUF_RESERVED_SIZE,new_head->len,new_head->data_type,&handler_->rtp_task_exit_flag);
               if(new_head->data_type == TYPE_VIDEO_RTP){
                   packet_count ++;
                   if(new_head->duration){
                       packet_count  = MAX_SEND_PACKET_COUNT;
                   }
                }
               oct_count += new_head->len;
               if(handler_->mRtpMemPool){
                   handler_->mRtpMemPool->freeBlock(new_head->block);
               }
          }else{
               AM_ERROR("RTPQueue -- dequeue  ERROR\n");
          }
          new_head->len = 0;
          new_head->data_type = TYPE_NONE;
          memset(&new_head->dest,0,sizeof(RtpDest));
          head = new_head;
          pthread_mutex_unlock(&q_h_lock);
          if(mNodePool){
              mNodePool->freeBlock(node->node);
          }
          return 1;
      }
     private:
        typedef struct node_t {
           CFixedMemPool::MemBlock block;
           int len;
           int data_type; //0 --none,  1 -- video_rtp, 2 --video_rtcp, 3 --audio_rtp,4 --audio_rtcp
           unsigned int duration;
           RtpDest dest;
           CFixedMemPool::MemBlock node;
           struct node_t *next;
       }RTP_NODE;

        CFixedMemPool *mNodePool;
        RTP_NODE *head;
        RTP_NODE *tail;
        pthread_mutex_t q_h_lock;
        pthread_mutex_t q_t_lock;
        CFFMpegMuxer *handler_;
    };
    RTPQueue mRtpQueue;
private:
    //pthread_mutex_t flag_mutex;
    volatile int rtp_task_exit_flag;
    int get_task_exit_flag(){
        /*
         int flag;
         pthread_mutex_lock(&flag_mutex);
         flag = rtp_task_exit_flag;
         pthread_mutex_unlock(&flag_mutex);
         return flag;
        */
        return rtp_task_exit_flag;
    }
    void set_task_exit_flag(int value){
        /*
         pthread_mutex_lock(&flag_mutex);
         rtp_task_exit_flag = value;
         pthread_mutex_unlock(&flag_mutex);
        */
        rtp_task_exit_flag = value;
    }
    void set_handler(){
        mRtpQueue.set_handler(this);
    }
    void flush_rtp_queue(){
         CFixedMemPool::MemBlock block;
         while(mRtpQueue.dequeue(block,NULL,NULL)){
            if(mRtpMemPool){
                mRtpMemPool->freeBlock(block);
            }
         }
    }
    pthread_t  rtp_thread;
    int startRTPSendTask(int  priority = 80){
        if((int)rtp_thread != -1){
            stopRTPSendTask();
        }
        flush_rtp_queue();
        mRtpMemPool = CFixedMemPool::Create(DRecommandMaxUDPPayloadLength, 512);
        if(!mRtpMemPool){
            return -1;
        }
        //pthread_mutex_init(&flag_mutex,NULL);
        set_task_exit_flag(0);
        pthread_attr_t attr;
        struct sched_param param;
        pthread_attr_init(&attr);
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        param.sched_priority = priority;
        pthread_attr_setschedparam(&attr, &param);
        if (pthread_create(&rtp_thread, &attr, rtp_send_task, (void*)this)) {
            pthread_attr_destroy(&attr);
            //pthread_mutex_destroy(&flag_mutex);
            if(mRtpMemPool){
                mRtpMemPool->Delete();
                mRtpMemPool = NULL;
            }
            return -1;
        }
        pthread_attr_destroy(&attr);
        mRtpTimestamp = 0;
        mIsFirst= true;
        return 0;
    }
    int stopRTPSendTask(){
        if((int)rtp_thread != -1){
            set_task_exit_flag(1);
            pthread_join(rtp_thread,NULL);
            //pthread_mutex_destroy(&flag_mutex);
            rtp_thread = -1;
        }
        if(mRtpMemPool){
            mRtpMemPool->Delete();
            mRtpMemPool = NULL;
        }
        return 0;
    }
    void debugRtpSendTask(int packet_count,int oct_count){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        AMLOG_WARN("rtp_send_task -- gettimeofday -- tv.tv_sec = %d, tv.tv_usec = %d,packet_count = %d,oct_count = %d\n",(int)tv.tv_sec,(int)tv.tv_usec,packet_count,oct_count);
    }
    int updateRtpInfo(RtpDest &dest, AM_UINT timestamp,AM_INT is_video = 1);
    int updateSendRtcp(RtpDest &dest, rtcp_stat_t *s,AM_UINT ssrc, AM_UINT timestamp,AM_INT is_video);
    AM_INT check_send_rtcp_sr(AM_UINT timestamp,AM_INT size,AM_INT is_video);
    static AM_INT  makeup_rtcp_sr(AM_U8 *buf, AM_INT len, AM_UINT ssrc, AM_S64 ntp_time,AM_UINT timestamp,AM_UINT packet_count,AM_UINT octet_count);
    AM_UINT getVideoPacketDuration(AM_UINT timestamp);
    void rtp_send_data(RtpDest *p_dest,unsigned char *buf,int len,int data_type,volatile int *force_exit_flag){
        for(int i = 0; i < p_dest->addr_count; i++){
            if(data_type == TYPE_VIDEO_RTP || data_type == TYPE_AUDIO_RTP){
                *(unsigned short *)&buf[2] = p_dest->rtp_seq[i];//seq
                *(unsigned int *)&buf[4] = p_dest->rtp_ts[i];//timestamp
                *(unsigned int *)&buf[8] = p_dest->rtp_ssrc[i];//ssrc
            }
            if(p_dest->rtp_over_rtsp[i]){
                unsigned char *data = &buf[-RTPBUF_RESERVED_SIZE];
                int size = len + RTPBUF_RESERVED_SIZE;
                data[0] = 0x24;
                data[1] = (unsigned char)p_dest->rtsp[i].channel;
                data[2] = (len >> 8) & 0xff;
                data[3] = (len) & 0xff;
                p_dest->rtsp[i].callback->OnRtpRtcpPackets(p_dest->rtsp[i].rtsp_fd,data,size,force_exit_flag);
                continue;
            }

        retry_send:
            if(*force_exit_flag) continue;
            int ret = sendto(p_dest->sock_fd, buf, len, 0, (struct sockaddr*)&p_dest->addr[i], sizeof(struct sockaddr));
            if(ret < 0){
                int err = errno;
                if(err == EINTR || err == EWOULDBLOCK || err == EAGAIN)
                    goto retry_send;
                AMLOG_ERROR("rtp_send_data failed --- errno = %d\n",err);
            }
        }
    }
    static void *rtp_send_task( void * arg){
        CFFMpegMuxer *pThis = (CFFMpegMuxer*)arg;
        int packet_count = 0;
        int oct_count = 0;
        pThis->set_handler();
        for(;;) {
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = SEND_INTERVAL;
            select(0, NULL, NULL, NULL, &timeout);

            if(pThis->get_task_exit_flag()){
                break;
            }

            packet_count = 0;
            oct_count = 0;
            while(pThis->mRtpQueue.dequeue_send(packet_count,oct_count)){
                if(pThis->get_task_exit_flag()){
                    break;
                }
                if(packet_count >= MAX_SEND_PACKET_COUNT)
                    break;
                if(oct_count > MAX_SEND_OCT_COUNT){
                    break;
                }
            }
            //pThis->debugRtpSendTask(packet_count,oct_count);
            if(pThis->get_task_exit_flag()){
                break;
            }
        }
        pThis->flush_rtp_queue();
        return (void*)NULL;
    }
};

#endif

