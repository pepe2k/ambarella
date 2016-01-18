
/*
 * g_ffmpeg_demuxer_net.h
 *
 * History:
 *    2012/5/11 - [QingXiong Z] created file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __G_FFMPEG_DEMUXER_H__
#define __G_FFMPEG_DEMUXER_H__

#define DEMUXER_BQUEUE_NUM_V 96
#define DEMUXER_BQUEUE_NUM_A 256
#define DEMUXER_RETRIEVE_Q_NUM 384 //1todo this num

#define CLOCKTIMEBASENUM 1
#define CLOCKTIMEBASEDEN 90000
#define FUCK_D1_NUM 4
//-----------------------------------------------------------------------
//
// CGDemuxerFFMpegNetTest
//-----------------------------------------------------------------------
class CGDemuxerFFMpegNetTest: public IGDemuxer, public CActiveObject
{
    typedef CActiveObject inherited;
    typedef CInterActiveFilter GFilter;
    enum{
        STATE_PAUSE = LAST_COMMON_STATE, //8
        STATE_READ_DATA,
        STATE_HIDED,
        STATE_TEST,
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
    virtual AM_ERR QueryInfo(AM_INT type, CParam& param){ return ME_OK;}

    virtual AM_ERR LoadFile();
    virtual AM_ERR SeekTo(AM_U64 ms);
    virtual AM_ERR GetTotalLength(AM_U64& ms);
    virtual AM_ERR GetCGBuffer(CGBuffer& buffer);
    virtual AM_ERR GetAudioBuffer(CGBuffer& oBuffer);//a-v spe
    virtual AM_ERR OnRetrieveBuffer(CGBuffer* buffer);
    virtual AM_ERR OnReleaseBuffer(CGBuffer* buffer);
    virtual const CQueue* GetBufferQ() { return mpBufferQV;}

    virtual void OnRun();
    virtual void Dump();

private:
    CGDemuxerFFMpegNetTest(IFilter* pFilter, CGConfig* pConfig):
        inherited("G_FFMpegDemuxer"),
        mpAVFormat(NULL),
        mpFilter(pFilter),
        mbEOS(AM_FALSE),
        mVideo(-1),
        mAudio(-1),
        mbEnVideo(AM_TRUE),
        mbEnAudio(AM_FALSE),
        mbAHandleSent(AM_FALSE),
        mNoPending(AM_FALSE),
        mbSelected(AM_FALSE),
        mbHideOnPause(AM_FALSE),
        mAudioCount(0),
        mAudioSentCnt(0),
        mAudioConsumeCnt(0),
        mVideoCount(0),
        mVideoSentCnt(0),
        mVideoConsumeCnt(0),
        mPTSVideo_Num(1),
        mPTSVideo_Den(1),
        mPTSAudio_Num(1),
        mPTSAudio_Den(1)
    {
        mpGConfig = pConfig;
        mConfigIndex = pConfig->curIndex;
        mpConfig = &(pConfig->demuxerConfig[mConfigIndex]);
        AM_ASSERT(mpConfig->configIndex == mConfigIndex);
    }
    AM_ERR Construct();
    void ClearQueue(CQueue* queue);
    virtual ~CGDemuxerFFMpegNetTest();

private:
    AM_ERR ProcessCmd(CMD& cmd);
    AM_ERR DoConfig();
    AM_ERR DoFlush();
    AM_ERR DoStop();
    AM_ERR DoPause();
    AM_ERR DoResume();
    AM_ERR DoSeek();
    AM_ERR DoHide(AM_BOOL hided);
    AM_ERR PostFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);
    AM_ERR SendFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);

private:
    AM_BOOL NeedGoPending();
    AM_BOOL NeedSendAudio(CGBuffer* pBuffer);
    AM_BOOL GetBufferPolicy(CGBuffer& gBuffer);
    AM_ERR ContinueRun();
    AM_ERR FillHandleBuffer();
    AM_ERR FillBufferToQueue();
    AM_ERR FillOnlyReceive(CGBuffer& buffer);
    AM_ERR ReadData(CGBuffer& buffer);
    AM_ERR FillEOS(CGBuffer& buffer);
    void UpdatePTSConvertor();
    AM_U64 ConvertVideoPts(am_pts_t pts);
    AM_U64 ConvertAudioPts(am_pts_t pts);
    int FindMediaStream(int media);
    int hasCodecParameters(AVCodecContext *enc);
    AM_ERR SetFormat(int stream, int media);

private:
    static AVFormatContext* mpAVArray[MDEC_SOURCE_MAX_NUM];
    static AM_INT mAVLable;

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
    //TODO
    AM_BOOL mbAHandleSent;
    AM_BOOL mNoPending;
    AM_BOOL mbSelected;
    AM_BOOL mbHideOnPause;

    AVPacket mPkt;

    //IMP flag for pause/flush/seek
    AM_INT mAudioCount;
    AM_UINT mAudioSentCnt;
    AM_UINT mAudioConsumeCnt;
    AM_INT mVideoCount;
    AM_INT mVideoSentCnt; //purge when pause
    AM_UINT mVideoConsumeCnt;
private:
    AM_UINT mPTSVideo_Num;
    AM_UINT mPTSVideo_Den;
    AM_UINT mPTSAudio_Num;
    AM_UINT mPTSAudio_Den;

private:
    //subQ of data, spe a/v on 4/17
    CQueue* mpBufferQV;
    CQueue* mpBufferQA;
    //enhance Q for pause/policy read
    CQueue* mpRetrieveQ;

    CGBuffer mVHandleBuffer;
    CGBuffer mAHandleBuffer;
};

#endif


