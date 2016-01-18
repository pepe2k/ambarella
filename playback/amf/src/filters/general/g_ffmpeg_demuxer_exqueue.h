
/*
 * g_ffmpeg_demuxer_exqueue.h
 *
 * History:
 *    2012/6/01 - [QingXiong Z] created file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __G_FFMPEG_DEMUXER_EX_QUEUE_H__
#define __G_FFMPEG_DEMUXER_EX_QUEUE_H__

#define DEMUXER_BQUEUE_NUM_V 64
#define DEMUXER_BQUEUE_NUM_A 300
#define DEMUXER_RETRIEVE_Q_NUM 384 //1todo this num

#define CLOCKTIMEBASENUM 1
#define CLOCKTIMEBASEDEN 90000
//-----------------------------------------------------------------------
//
// CGDemuxerFFMpegNet
//-----------------------------------------------------------------------
class CGDemuxerFFMpegExQueue: public IGDemuxer, public CActiveObject
{
    typedef CActiveObject inherited;
    typedef CInterActiveFilter GFilter;
    enum{
        STATE_PAUSE = LAST_COMMON_STATE,
        STATE_READ_DATA,
        STATE_HIDED,
    };

public:
    static IGDemuxer* Create(IFilter* pFilter, CGConfig* pConfig);
    static AM_INT ParseFile(const char* filename, CGConfig* pConfig);
    static AM_ERR ClearParse();

public:
    // IInterface
    void* GetInterface(AM_REFIID refiid)
    {
        //if (refiid == IID_IGDemuxer)
            //return (IGDemuxer*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete();

    // IGDemuxer
    virtual AM_ERR PerformCmd(CMD& cmd, AM_BOOL isSend);
    virtual AM_ERR QueryInfo(AM_INT type, CParam64& param);

    virtual AM_ERR LoadFile();
    virtual AM_ERR SeekTo(AM_U64 ms, AM_INT flag);
    virtual AM_ERR GetTotalLength(AM_U64& ms);
    virtual AM_ERR GetCGBuffer(CGBuffer& buffer);
    virtual AM_ERR GetAudioBuffer(CGBuffer& oBuffer);//a-v spe
    virtual AM_ERR OnRetrieveBuffer(CGBuffer* buffer);
    virtual AM_ERR OnReleaseBuffer(CGBuffer* buffer);
    virtual const CQueue* GetBufferQ() { return mpBufferQV;}

    virtual void OnRun();
    virtual void Dump(AM_INT flag);

private:
    CGDemuxerFFMpegExQueue(IFilter* pFilter, CGConfig* pConfig):
        inherited("G_FFMpegDemuxer_Queue"),
        mpAVFormat(NULL),
        mpFilter(pFilter),
        mbEOS(AM_FALSE),
        mbEosV(AM_FALSE),
        mbEosA(AM_FALSE),
        mVideo(-1),
        mAudio(-1),
        mbEnVideo(AM_TRUE),
        mbEnAudio(AM_FALSE),
        mVideoWidth(0),
        mVideoHeight(0),
        mbAHandleSent(AM_FALSE),
        mNoPending(AM_FALSE),
        mbSelected(AM_FALSE),
        mbHideOnPause(AM_FALSE),
        mAudioCount(0),
        mAudioSentCnt(0),
        mAudioConsumeCnt(0),
        mVideoCount(-1),
        mVideoSentCnt(0),
        mVideoConsumeCnt(0),
        mPTSVideo_Num(1),
        mPTSVideo_Den(1),
        mPTSAudio_Num(1),
        mPTSAudio_Den(1),
        mLastPts(0),
        mLastLoopPts(0),
        mLastAudioPts(0),
        mSeekCheck(-1),
        mLastSeek(0),
        bFirstAudioData(AM_TRUE),
        mFirstAudioPts(0),
        mDebug(AM_FALSE),
        mBWLastPTS(0),
        mEstimatedKeyFrameInterval(0),
        mbBWIOnlyMode(false),
        mbBWPlaybackFinished(false),
        mbGetEstimatedKeyFrameInterval(false)
    {
        mpGConfig = pConfig;
        mConfigIndex = pConfig->curIndex;
        mpConfig = &(pConfig->demuxerConfig[mConfigIndex]);
        AM_ASSERT(mpConfig->configIndex == mConfigIndex);
        mpGMuxer = mpGConfig->mainMuxer->generalMuxer;
        mpAudioMan = mpGConfig->audioManager;
        pthread_mutex_init(&mutex,NULL);
    }
    AM_ERR Construct();
    void ClearQueue(CQueue* queue);
    virtual ~CGDemuxerFFMpegExQueue();

private:
    AM_ERR ProcessCmd(CMD& cmd);
    AM_ERR DoAudioCtrl(AM_INT flag);
    AM_ERR DoEnableAV();
    AM_ERR DoConfig();
    AM_ERR DoFlush();
    AM_ERR DoStop();
    AM_ERR DoPause();
    AM_ERR DoResume();
    AM_ERR DoSeek(AM_U64 target);
    AM_ERR DoSeek2(AM_U64 target);
    AM_ERR DoSeekCheck();
    AM_ERR DoHide(AM_BOOL hided);
    AM_ERR PostFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);
    AM_ERR SendFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);

    void Test(const char* chr);
    int Test2(AM_INT i);
    AM_U64 QueryVideoPts();

private:
    AM_BOOL NeedGoPending();
    AM_BOOL NeedSendAudio(CGBuffer* pBuffer);
    AM_BOOL GetBufferPolicy(CGBuffer& gBuffer);
    AM_ERR ContinueRun();
    AM_ERR FillAudioHandleBuffer();
    AM_ERR FillHandleBuffer(AM_BOOL goon = AM_FALSE);
    AM_ERR FillBufferToQueue();
    AM_ERR FillFullAudioQueue();
    AM_ERR FillOnlyReceive(CGBuffer& buffer);
    AM_ERR ReadData(CGBuffer& buffer);
    AM_ERR FillEOS(CGBuffer& buffer);
    void UpdatePTSConvertor();
    AM_U64 ConvertVideoPts(am_pts_t pts);
    AM_U64 ConvertAudioPts(am_pts_t pts);
    int FindMediaStream(int media);
    int hasCodecParameters(AVCodecContext *enc);
    AM_ERR SetFormat(int stream, int media);
    AM_ERR FinalDataProcess(CGBuffer* buffer);
    AM_ERR SetupMuxerInfo();
    AM_INT FRAME_TIME_DIFF(AM_INT diff);
    AM_ERR FillVBufferOnlyKeyframeOnlyBW();
    AM_ERR ReadVideoKeyframe(CGBuffer& buffer);

private:
    static AVFormatContext* mpAVArray[MDEC_SOURCE_MAX_NUM];
    static AM_INT mAVLable;
    //static AM_INT mHDCur;

    AVFormatContext* mpAVFormat;
    AM_INT mAVIndex;
    IFilter* mpFilter;
    CGConfig* mpGConfig;
    CDemuxerConfig* mpConfig;
    AM_INT mConfigIndex;

    AM_BOOL mbEOS;
    AM_BOOL mbEosV;
    AM_BOOL mbEosA;
    AM_INT mVideo;
    AM_INT mAudio;
    AM_BOOL mbEnVideo;
    AM_BOOL mbEnAudio;
    AM_INT mVideoWidth;
    AM_INT mVideoHeight;
    //TODO
    AM_BOOL mbAHandleSent;
    AM_BOOL mNoPending;
    AM_BOOL mbSelected;
    AM_BOOL mbHideOnPause;

    AVPacket mPkt;

    //IMP flag for pause/flush/seek
    AM_UINT mAudioCount;
    AM_UINT mAudioSentCnt;
    AM_UINT mAudioConsumeCnt;
    AM_UINT mVideoCount;
    AM_INT mVideoSentCnt; //purge when pause
    AM_UINT mVideoConsumeCnt;
private:
    AM_UINT mPTSVideo_Num;
    AM_UINT mPTSVideo_Den;
    AM_UINT mPTSAudio_Num;
    AM_UINT mPTSAudio_Den;

    AM_U64 mLastPts;
    AM_U64 mLastLoopPts;
    AM_U64 mLastAudioPts;
    AM_S64 mSeekCheck;
    AM_S64 mLastSeek;
    AM_BOOL bFirstAudioData;
    AM_S64 mFirstAudioPts;
    AM_BOOL mDebug;//debug only
private:
    //subQ of data, spe a/v on 4/17
    CQueue* mpBufferQV;
    CQueue* mpBufferQA;
    //enhance Q for pause/policy read
    CQueue* mpRetrieveQ;

    CGBuffer mVHandleBuffer;
    CGBuffer mAHandleBuffer;

private:
    //Muxer Control
    CGeneralMuxer* mpGMuxer;
    //aduio system
    CGeneralAudioManager* mpAudioMan;

private:
    am_pts_t mBWLastPTS;
    am_pts_t mEstimatedKeyFrameInterval;
    bool mbBWIOnlyMode;
    bool mbBWPlaybackFinished;
    bool mbGetEstimatedKeyFrameInterval;

    pthread_mutex_t mutex;
};

#endif



