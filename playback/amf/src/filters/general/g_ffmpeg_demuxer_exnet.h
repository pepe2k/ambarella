
/*
 * g_ffmpeg_demuxer_net.h
 *
 * History:
 *    2012/7/03 - [QingXiong Z] created file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __G_FFMPEG_DEMUXER_EX_NET_H__
#define __G_FFMPEG_DEMUXER_EX_NET_H__

#if NEW_RTSP_CLIENT
#include "amf_rtspclient.h"
#endif

//#define DEMUXER_BQUEUE_NUM_V 1024//to drain the jiffer
#define DEMUXER_BQUEUE_NUM_V 512
#define DEMUXER_BQUEUE_NUM_A 256
#define DEMUXER_RETRIEVE_Q_NUM 384 //1todo this num

#define CLOCKTIMEBASENUM 1
#define CLOCKTIMEBASEDEN 90000
//-----------------------------------------------------------------------
//
// CGDemuxerFFMpegNetEx
//-----------------------------------------------------------------------
class CGDemuxerFFMpegNetEx: public IGDemuxer, public CActiveObject
{
    typedef CActiveObject inherited;
    typedef CInterActiveFilter GFilter;
    enum{
        STATE_PAUSE = LAST_COMMON_STATE,//8
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
    CGDemuxerFFMpegNetEx(IFilter* pFilter, CGConfig* pConfig):
        inherited("G_FFMpegDemuxer_Net"),
#if NEW_RTSP_CLIENT
        m_rtsp_client(NULL),
#endif
        mpAVFormat(NULL),
        mpFilter(pFilter),
        mbEOS(AM_FALSE),
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
        mbRevived(AM_TRUE),
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
        mSeekCheck(-1),
        mLastSeek(0),
        mLastPts(0),
        mLastAudioPts(0),
        mDebug(AM_FALSE),
        mFirstPts(0)
    {
        mpGConfig = pConfig;
        mConfigIndex = pConfig->curIndex;
        mpConfig = &(pConfig->demuxerConfig[mConfigIndex]);
        AM_ASSERT(mpConfig->configIndex == mConfigIndex);
        mpGMuxer = mpGConfig->mainMuxer->generalMuxer;
        mReleaseRevive = 0;
        mpAudioMan = mpGConfig->audioManager;
        pthread_mutex_init(&mutex,NULL);
    }
    AM_ERR Construct();
    void ClearQueue(CQueue* queue);
    virtual ~CGDemuxerFFMpegNetEx();

private:
    AM_ERR ProcessCmd(CMD& cmd);
    AM_ERR DoAudioCtrl();
    AM_ERR DoEnableAV();
    AM_ERR DoConfig();
    AM_ERR DoFlush();
    AM_ERR DoStop();
    AM_ERR DoPause();
    AM_ERR DoResume();
    AM_ERR DoSeek(AM_U64 target);
    AM_ERR DoSeekCheck();
    AM_ERR DoHide(AM_BOOL hided);
    AM_ERR PostFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);
    AM_ERR SendFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);

    void Test(const char* chr);
    int Test2(AM_INT i);
private:
    AM_BOOL NeedGoPending();
    AM_BOOL NeedSendAudio(CGBuffer* pBuffer);
    AM_BOOL GetBufferPolicy(CGBuffer& gBuffer);
    AM_ERR ContinueRun();
    AM_ERR FillAudioHandleBuffer();
    AM_ERR FillHandleBuffer(AM_BOOL goon = AM_FALSE);
    AM_ERR FillBufferToQueue();
    AM_ERR FillOnlyReceive();
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
    AM_ERR StatVideoFrame(CGBuffer* gBuffer);

private:

#if NEW_RTSP_CLIENT
    static /*AmfRtspClient*/void * mpAVArray[MDEC_SOURCE_MAX_NUM];
    static  int mpAVArrayType[MDEC_SOURCE_MAX_NUM]; //0 -- AmfRtspClient, 1 -- ffmpeg rtspclient
    AmfRtspClient *m_rtsp_client;
#else
    static AVFormatContext* mpAVArray[MDEC_SOURCE_MAX_NUM];
#endif
     static AM_INT mAVLable;
    //static AM_INT mHDCur;

    AVFormatContext* mpAVFormat;
    AM_INT mAVIndex;
    IFilter* mpFilter;
    CGConfig* mpGConfig;
    CDemuxerConfig* mpConfig;
    AM_INT mConfigIndex;

    AM_BOOL mbEOS;
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
    //Pre_Buffer
    AM_BOOL mbRevived;

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

    AM_S64 mSeekCheck;
    AM_U64 mLastSeek;
    AM_U64 mLastPts;
    AM_U64 mLastAudioPts;
    AM_BOOL mDebug;//debug only
    AM_S64 mFirstPts; //debug only
    AM_UINT mReleaseRevive;//debug only

    class FrameStat
    {
        public:
            typedef struct VecInt{
                AM_INT Gop;
                AM_INT Frame;
            }VecInt;
        public:
        FrameStat()
        {
            PtsDiff = PtsLast = 0;
            ElemNum = GopNum = 0;
            FrameEachGop = 0;
            DataNum = 100;
            data = new VecInt[DataNum];
        }
        AM_ERR InsertElem(AM_INT frameNum, AM_INT gopIndex)
        {
            if(ElemNum >= DataNum){
                VecInt* temp = new VecInt[DataNum + 100];
                memcpy(temp, data, sizeof(VecInt) * DataNum);
                DataNum += 100;
                delete[] data;
                data = temp;
            }
            data[ElemNum].Frame = frameNum;
            data[ElemNum].Gop = gopIndex;
            ElemNum++;
            return ME_OK;
        }
        ~FrameStat()
        {
            delete[] data;
        }
        AM_INT GopNum;
        AM_INT ElemNum;
        AM_INT DataNum;
        AM_INT FrameEachGop;
        AM_INT PtsDiff;
        AM_S64 PtsLast;
        VecInt* data;
    };
    FrameStat mFrameStat;
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
    pthread_mutex_t mutex;
};

#endif



