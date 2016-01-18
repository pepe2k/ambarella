
/*
 * g_ffmpeg_demuxer_exqueue.cpp
 *
 * History:
 *    2012/6/01 - [QingXiong Z] created file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "g_ffmpeg_demux_exqueue"
//#define AMDROID_DEBUG
#include <unistd.h>

#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

extern "C" {
#define INT64_C
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

#include "general_muxer_save.h"
#include "general_audio_manager.h"
#include "g_ffmpeg_demuxer_exqueue.h"

GDemuxerParser gFFMpegDemuxerQueEx = {
    "FFMpegDemuxer-G-ExQueue",
    CGDemuxerFFMpegExQueue::Create,
    CGDemuxerFFMpegExQueue::ParseFile,
    CGDemuxerFFMpegExQueue::ClearParse,
};

#define SEEK_GAP (FRAME_TIME_DIFF(10))
//-----------------------------------------------------------------------
//
// CGDemuxerFFMpegExQueue
//
//-----------------------------------------------------------------------
//accept this file or not.
AVFormatContext* CGDemuxerFFMpegExQueue::mpAVArray[MDEC_SOURCE_MAX_NUM] = {NULL,};
AM_INT CGDemuxerFFMpegExQueue::mAVLable = 0;
//AM_INT CGDemuxerFFMpegExQueue::mHDCur = -1;

AM_INT CGDemuxerFFMpegExQueue::ParseFile(const char* filename, CGConfig* pConfig)
{
    AM_INFO("CGDemuxerFFMpegExQueue::ParseFile\n");
    AM_INT i = 0, score = 0;
    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        if(mpAVArray[i] == NULL)
            break;
    }
    if(i == MDEC_SOURCE_MAX_NUM){
        score = -100;
        AM_INFO("CGDemuxerFFMpegExQueue ParseFile, Score:%d\n", score);
        return score;
    }
    //if(pConfig->globalFlag & USING_FOR_NET_PB)
        //return -50;
    if(pConfig->demuxerConfig[pConfig->curIndex].netused == AM_TRUE)
        return -50;

    mAVLable = i;
    AVFormatContext* pAV = mpAVArray[mAVLable];
    av_register_all();

    int rval = av_open_input_file(&pAV, filename, NULL, 0, NULL);
    if (rval < 0)
    {
        AM_INFO("ffmpeg does not recognize %s\n", filename);
        //av_close_input_file(mpAVFormat);
        score = -10;
    }else{
        AM_INFO("CGDemuxerFFMpegExQueue: %s is [%s] format\n", filename, pAV->iformat->name);
        mpAVArray[mAVLable] = pAV;
        score = 99;
    }

    AM_INFO("CGDemuxerFFMpegExQueue ParseFile, Score:%d\n", score);
    return score;
}

AM_ERR CGDemuxerFFMpegExQueue::ClearParse()
{
    //AM_INT lable = mAVLable -1;
    if (mpAVArray[mAVLable] != NULL)
    {
        av_close_input_file(mpAVArray[mAVLable]);
        mpAVArray[mAVLable] = NULL;
    }
    return ME_OK;
}

IGDemuxer* CGDemuxerFFMpegExQueue::Create(IFilter* pFilter, CGConfig* config)
{
    CGDemuxerFFMpegExQueue* result = new CGDemuxerFFMpegExQueue(pFilter, config);
    if (result && result->Construct() != ME_OK)
    {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

//----------------------------------------------
//
//
AM_ERR CGDemuxerFFMpegExQueue::Construct()
{
    AM_INFO("CGDemuxerFFMpegExQueue Construct!\n");
    //set av first
    mAVIndex = mAVLable;
    mpAVFormat = mpAVArray[mAVLable];
    AM_ASSERT(mpAVFormat);
    //if failed during construct, reset av on delete
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    CQueue* mainQ = ((CActiveFilter* )mpFilter)->MsgQ();
    CQueue* mainQA = mpAudioMan->AudioMainQ();
    if ((mpBufferQV= CQueue::Create(mainQ, this, sizeof(CGBuffer), DEMUXER_BQUEUE_NUM_V)) == NULL)
        return ME_NO_MEMORY;

    if ((mpBufferQA= CQueue::Create(mainQA, this, sizeof(CGBuffer), DEMUXER_BQUEUE_NUM_A)) == NULL)
        return ME_NO_MEMORY;

    if ((mpRetrieveQ = CQueue::Create(NULL, this, sizeof(CGBuffer), DEMUXER_RETRIEVE_Q_NUM)) == NULL)
        return ME_NO_MEMORY;
    //default av config on LoadFile();

    //mainQ->Dump();
    mpWorkQ->SetThreadPrio(1, 1);
    return ME_OK;
}

void CGDemuxerFFMpegExQueue::Delete()
{
    AM_INFO("CGDemuxerFFMpegExQueue::Delete().\n");
    //attach to delete; has some logic here:
    // normal delete, gd will call detach, dynamic case, gd will not call detach.
    // add a remove config to gd to ensure gd call detach
    CQueue* mainQ = ((CActiveFilter* )mpFilter)->MsgQ();
    mpBufferQV->Attach(mainQ);
    CQueue* mainQA = mpAudioMan->AudioMainQ();
    mpBufferQA->Attach(mainQA);

    ClearQueue(mpBufferQV);
    ClearQueue(mpBufferQA);
    ClearQueue(mpRetrieveQ);
    if(mpAVFormat)
    {
        av_close_input_file(mpAVFormat);
        mpAVFormat = NULL;
        mpAVArray[mAVIndex] = NULL;
    }
    AM_DELETE(mpBufferQV);
    AM_DELETE(mpBufferQA);
    AM_DELETE(mpRetrieveQ);
    mpAudioMan->DeleteAudioSource(mConfigIndex);
    inherited::Delete();
}

void CGDemuxerFFMpegExQueue::ClearQueue(CQueue* queue)
{
    AM_BOOL rval;
    CGBuffer buffer;
    while(1)
    {
        if(queue == mpRetrieveQ){
            rval = queue->PeekMsg(&buffer, sizeof(CGBuffer));
        }else{
            rval = queue->PeekData(&buffer, sizeof(CGBuffer));
        }
        if(rval == AM_FALSE)
        {
            break;
        }
        //release this packet
        pthread_mutex_lock(&mutex);
        if(buffer.GetStreamType() == STREAM_VIDEO){
            AM_ASSERT(mVideoConsumeCnt == buffer.GetCount());
            if(mVideoConsumeCnt != buffer.GetCount())
                AM_INFO("DIff(%d) : %d-->%d\n", mConfigIndex, mVideoConsumeCnt, buffer.GetCount());
            mVideoConsumeCnt++;
        }else{
            AM_ASSERT(mAudioConsumeCnt == buffer.GetCount());
            if(mAudioConsumeCnt != buffer.GetCount())
                AM_INFO("DIff(%d) : %d-->%d\n", mConfigIndex, mAudioConsumeCnt, buffer.GetCount());
            mAudioConsumeCnt++;
        }

        FinalDataProcess(&buffer);
        pthread_mutex_unlock(&mutex);
        continue;
    }
}

CGDemuxerFFMpegExQueue::~CGDemuxerFFMpegExQueue()
{
    AM_INFO("~CGDemuxerFFMpegExQueue done.\n");
    AMLOG_DESTRUCTOR("~CGDemuxerFFMpegExQueue.\n");
    pthread_mutex_destroy(&mutex);
    AMLOG_DESTRUCTOR("~CGDemuxerFFMpegExQueue done.\n");
}

AM_ERR CGDemuxerFFMpegExQueue::PerformCmd(CMD& cmd, AM_BOOL isSend)
{
    AM_ERR err;
    if(isSend == AM_TRUE)
    {
        err = (MsgQ())->SendMsg(&cmd, sizeof(CMD));
    }else{
        err = (MsgQ())->PostMsg(&cmd, sizeof(CMD));
    }
    return err;
}

//CMD FROM GENERAL DEMUXER
AM_ERR CGDemuxerFFMpegExQueue::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGDemuxerFFMpegExQueue %d ::ProcessCmd %d\n ", mConfigIndex, cmd.code);
    AM_ERR err;
    AM_U64 par;
    switch (cmd.code)
    {
    case CMD_STOP:
        DoStop();
        mbRun = false;
        CmdAck(ME_OK);
        break;

    case CMD_PAUSE:
        DoPause();
        CmdAck(ME_OK);
        break;

    case CMD_RESUME:
        DoResume();
        CmdAck(ME_OK);
        break;

    case CMD_FLUSH:
        DoFlush();
        CmdAck(ME_OK);
        break;

    case CMD_CONFIG:
        err = DoConfig();
        CmdAck(err);
        break;

    case CMD_ACK:
        if(mState == STATE_PENDING)
        {
            mState = STATE_READ_DATA;
        }
        //CmdAck(ME_OK);
        break;

    case CMD_SEEK:
        par = cmd.res64_1;
        err = DoSeek2(par);
        if(mState == STATE_PENDING){
            mState = STATE_READ_DATA;
        }
        if(mbEOS){
            mbEOS = AM_FALSE;
            mbEosA = AM_FALSE;
            mbEosV = AM_FALSE;
        }
        //err = DoSeek(par);
        CmdAck(err);
        break;

    case CMD_AUDIO:
        err = DoAudioCtrl(cmd.flag);
        CmdAck(err);
        break;

    case CMD_BW_PLAYBACK_DIRECTION:
        mbBWIOnlyMode = true;
        break;

    case CMD_FW_PLAYBACK_DIRECTION:
        if ((STATE_PENDING != mState) && (STATE_READ_DATA != mState) && (STATE_HIDED!= mState)) {
            mState = STATE_READ_DATA;
        }
        mbBWIOnlyMode = false;

        //clear bw related field
        mbBWPlaybackFinished = false;
        mBWLastPTS = 0;
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }

    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::DoAudioCtrl(AM_INT flag)
{
    AM_INFO("DoAudioCtrl flag:%d, %s, %d.\n", flag, mpConfig->disableAudio == AM_FALSE ? "Add" : "Remove", mbEnAudio);
    AM_ASSERT(mpConfig->disableVideo == AM_FALSE);
    if(mbEnAudio != mpConfig->disableAudio)
    {
        AM_INFO("Check on Engine Plz(%d), %d, %d.\n", mConfigIndex, mbEnAudio, mpConfig->disableAudio);
        return ME_OK;
    }
    if(mbEnAudio == AM_TRUE){
        ClearQueue(mpBufferQA);
        CQueue* mainQA = mpAudioMan->AudioMainQ();
        mpBufferQA->Attach(mainQA);
    }else{
        pthread_mutex_lock(&mutex);
        mAudioConsumeCnt = mAudioSentCnt = mAudioCount = 0;
        pthread_mutex_unlock(&mutex);
        mAHandleBuffer.SetOwnerType(DEMUXER_FFMPEG);
        mAHandleBuffer.SetBufferType(HANDLE_BUFFER);
        mAHandleBuffer.SetStreamType(STREAM_AUDIO);
        mAHandleBuffer.SetIdentity(mConfigIndex);
        mAHandleBuffer.SetCount(0);
        mAHandleBuffer.SetExtraPtr((AM_INTPTR)mpBufferQA);
        mpBufferQA->PutData(&mAHandleBuffer, sizeof(CGBuffer));
        mbAHandleSent = AM_TRUE;
        SendFilterMsg(GFilter::MSG_DEMUXER_A, 0);
        mbSelected = AM_TRUE;
        //set audio system, some case, like replace/edit playback need not keep sync with video.
        if(flag & SYNC_AUDIO_LASTPTS)
            mpAudioMan->SetAudioPosition(mConfigIndex, QueryVideoPts());
    }
    mbEnAudio = !mpConfig->disableAudio;
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::DoEnableAV()
{
    mbEnAudio = !mpConfig->disableAudio && mpConfig->hasAudio;
    mbEnVideo = !mpConfig->disableVideo && mpConfig->hasVideo;
    return ME_OK;
}

//Imp interactive fun.
AM_ERR CGDemuxerFFMpegExQueue::DoConfig()
{
    if(mpConfig->needStart == AM_TRUE){
        //let's go!
        AM_ASSERT(mState == STATE_PENDING);
        AM_ASSERT(mpConfig->paused == AM_FALSE);
        mbEnAudio = !mpConfig->disableAudio && mpConfig->hasAudio;
        mbEnVideo = !mpConfig->disableVideo && mpConfig->hasVideo;
        mState = STATE_READ_DATA;
        DoHide(mpConfig->hided);
        mpConfig->needStart = AM_FALSE;
        return ME_OK;
    }
    //AM_INFO("De\n");
    if((mState == STATE_HIDED) != mpConfig->hided)
        DoHide(mpConfig->hided);
    //process pause/resume by cmd

    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::DoFlush()
{
    AM_INFO("CGDemuxerFFMpegExQueue %d Flush All Data:", mConfigIndex);
    AM_INFO("   Video Cnt: %d, Consume Video Cnt:%d, SentVideo Cnt:%d, TotalVideo Cnt:%d\n",
        mpBufferQV->GetDataCnt(), mVideoConsumeCnt, mVideoSentCnt, mVideoCount);
    AM_INFO("CGDemuxerFFMpegExQueue %d Flush All Data:", mConfigIndex);
    AM_INFO("   Audio Cnt: %d, Consume Audio Cnt:%d, SentAudio Cnt:%d, TotalAudio Cnt:%d\n",
        mpBufferQA->GetDataCnt(), mAudioConsumeCnt, mAudioSentCnt, mAudioCount);
    ClearQueue(mpBufferQV);
    //mVideoSentCnt += mpBufferQV->GetDataCnt();
    //AM_ASSERT(mVideoSentCnt == mVideoConsumeCnt);
    AM_ASSERT(mVideoCount == (mVideoConsumeCnt -1));//mVideoCnt no INCLUDE handle buffer

    ClearQueue(mpBufferQA);
    if(mAudioCount != 0)
        AM_ASSERT(mAudioCount == (mAudioConsumeCnt -1));
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::DoStop()
{
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::DoHide(AM_BOOL hided)
{
    AM_INFO("Demuxer-ExQueue DoHide(%d, %d)\n", mConfigIndex, hided);
    if(hided == AM_TRUE){
        if(mState == STATE_PAUSE && mpConfig->paused == AM_TRUE){
            //hided after paused.
            mbHideOnPause = AM_TRUE;
        }
        mState = STATE_HIDED;
    }

    if(hided == AM_FALSE){
        if(mbHideOnPause == AM_TRUE){
            mState = STATE_PAUSE;
            mbHideOnPause = AM_FALSE;
        }else{
            mState = STATE_READ_DATA;
        }
        AM_INFO("mpConfig->hd: %d, mpGConfig->curHdIndex: %d, mConfigIndex %d.\n",
            mpConfig->hd, mpGConfig->curHdIndex, mConfigIndex);
        if((mpConfig->hd == AM_TRUE && mpGConfig->curHdIndex != mConfigIndex)
            || mpConfig->sendHandleV == AM_TRUE){
            //a other HD will be show
            AM_ASSERT(mpBufferQV->GetDataCnt() <= 0);
            CQueue* mainQ = ((CActiveFilter* )mpFilter)->MsgQ();
            mpBufferQV->Attach(mainQ);
            FillHandleBuffer(AM_TRUE);
            if(mpConfig->hd == AM_TRUE)
                mpGConfig->curHdIndex = mConfigIndex;
            mpConfig->sendHandleV = AM_FALSE;
            //mainQ->Dump();
        }
    }
    return ME_OK;
}

//do something if using a network-based format:RTSP
AM_ERR CGDemuxerFFMpegExQueue::DoPause()
{
    if(mState == STATE_PAUSE)
        return ME_OK;

    mState = STATE_PAUSE;
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::DoResume()
{
//    AM_INT ret;
    //AM_ASSERT(mState == STATE_PAUSE);
    mState = STATE_READ_DATA;
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::DoSeekCheck()
{
    //AM_INFO("??????????????????%d\n", mConfigIndex);
    AM_INFO("DoSeekCheck %lld(%lld) for Stream: %d\n", mSeekCheck, mSeekCheck / 3003, mConfigIndex);
    AM_INT ret;
    AM_ERR err = ME_OK;
    //we need a self-directed mode to check the seek jump.
    bool bigFindBack = false;
    bool smallFindCloser = false;
    bool findCloser = true;
    bool checkDone = false;
    AM_INT smallCloser = 0;//need fast cross the small gap

    while(1)
    {
        av_init_packet(&mPkt);
        ret = av_read_frame(mpAVFormat, &mPkt);
        if(ret < 0)
        {
            AM_INFO("seek check meet eos! check me!ret=%d.\n", ret);
            AM_ASSERT(0);
            av_free_packet(&mPkt);
            return ME_CLOSED;
        }
        if(mPkt.stream_index != mVideo)
        {
            av_free_packet(&mPkt);
            continue;
        }

        if(bigFindBack == true && smallFindCloser == true){
            findCloser = false;
        }

        if(mSeekCheck != -1)
        {
            if(mPkt.pts > mSeekCheck && checkDone != true){
                AM_INFO("Adjuct Demuxer Seek to: %lld, cur:%lld\n", mSeekCheck, mPkt.pts);
                if(mLastSeek == 0){
                    //we set to 0, and still can not find a smaller position
                    break;
                }
                mLastSeek -= SEEK_GAP;
                if(mLastSeek < 0)
                    mLastSeek = 0;
                err = DoSeek(mLastSeek);
                bigFindBack = true;
                if(err != ME_OK){
                    AM_ERROR("Seek Failed, PTS adjuct failed\n");
                    AM_ASSERT(0);
                    av_free_packet(&mPkt);
                    return ME_ERROR;
                }else{
                    av_free_packet(&mPkt);
                    continue;
                }
            }else{
                if(((mSeekCheck - mPkt.pts) >= FRAME_TIME_DIFF(50)) && findCloser == true){
                    AM_INFO("Too Long Diff , Seek again %lld, Want(%lld).\n", mPkt.pts, mSeekCheck);
                    av_free_packet(&mPkt);
                    mLastSeek += (smallCloser/3 + 1) * SEEK_GAP;
                    smallFindCloser = true;
                    smallCloser++;
                    err = DoSeek(mLastSeek);
                    AM_ASSERT(err == ME_OK);
                    continue;
                }
                checkDone = true;
                AM_INFO("SeekCheck Done:%lld.\n", mPkt.pts);
                if((mSeekCheck - mPkt.pts) > FRAME_TIME_DIFF(3)){
                    //AM_INFO("Min Adjuct Read: %lld, cur:%lld\n", mSeekCheck, mPkt.pts);
                    //Test(0);
                    av_free_packet(&mPkt);
                    continue;
                }
            }
        }
        break;
    }
    /*
    if(mpGConfig->globalFlag & SWITCH_ARM_SEEK_FEED_DSP_DEBUG ||
        mpGConfig->globalFlag & SWITCH_JUST_FOR_WIN_ARM ){
        if(mConfigIndex == 0)
            Test("************End Seek for Stream 0 Right Now PTS Info: ");
        else
            Test("************End Seek for Stream 4, Right Now PTS Info: ");
    }
    */
    //AM_INFO("Adjuct Seek Done: Find pts:%lld, Pts on dsp:%lld, frame num:%d\n", mPkt.pts, ConvertVideoPts(mPkt.pts), mPkt.pts/3003);
    mSeekCheck = -1;
    //av_free_packet(&mPkt);
    //av_init_packet(&mPkt);
    mDebug = AM_TRUE;
    return ME_OK;
}

#ifndef INT64_MIN
#define INT64_MIN (-0x7fffffffffffffffLL - 1)
#endif
#ifndef INT64_MAX
#define INT64_MAX (0x7FFFFFFFFFFFFFFFLL)
#endif
AM_ERR CGDemuxerFFMpegExQueue::DoSeek2(AM_U64 target)
{
    AM_ERR err = ME_OK;
    AM_INT ret;
    AM_U64 starttime = 0;

    if((AM_U64)mpAVFormat->start_time != AV_NOPTS_VALUE){
        starttime = mpAVFormat->start_time;
    }
    /*if((AM_U64)mpAVFormat->start_time >= target){
        if ((ret = avformat_seek_file( mpAVFormat, -1,INT64_MIN, 0, INT64_MAX, AVSEEK_FLAG_BYTE))<0){
            AM_ERROR("seek by bytes fail, ret =%d, Seek return error.\n", ret);
            return ME_ERROR;
        }
    }else*/{

        AM_INFO("DoSeek2[%d], Seek, seek_target = %lld.\n", mConfigIndex, target);
        if ((ret = avformat_seek_file(mpAVFormat, mVideo,INT64_MIN, target,INT64_MAX, 0)) < 0) {
            AM_ERROR("seek by time fail, err =%d, try by bytes.\n", ret);

            if(mpAVFormat->duration<=0) {
                AM_ERROR("duration unknow, cannot seek!!\n");
                return ME_ERROR;
            }
            //try seek by byte
            AM_S64 ms = target * 1000
                *mpAVFormat->streams[mVideo]->time_base.num
                /mpAVFormat->streams[mVideo]->time_base.den;
            AM_S64 seek_target = (ms-starttime)
                *mpAVFormat->file_size/mpAVFormat->duration;
            if ((ret = avformat_seek_file( mpAVFormat, -1,INT64_MIN, seek_target,INT64_MAX, AVSEEK_FLAG_BYTE))<0){
                AM_ERROR("seek by bytes fail, err =%d, Seek return error.\n", ret);
                return ME_ERROR;
            }
        }
    }
    AM_INFO("DoSeek2[%d], Seek done, pos = %lld.\n", mConfigIndex, mpAVFormat->pb->pos);
    return err;
}

AM_ERR CGDemuxerFFMpegExQueue::DoSeek(AM_U64 target)
{
    AM_INT ret = 0;
    AM_U64 targeByte;
    AM_U64 timeMs;

    if((ret = av_seek_frame(mpAVFormat, -1, target, 0)) < 0)
    {
        timeMs = target * 1000 * (mpAVFormat->streams[mVideo]->time_base.num) / (mpAVFormat->streams[mVideo]->time_base.den);
        targeByte = target * mpAVFormat->file_size /mpAVFormat->duration;
        AM_INFO("seek by timestamp(%llu) fail, ret =%d, try by bytes(%lld), timeMs(%llu)--duration:%lld.\n", target, ret, targeByte, timeMs, mpAVFormat->duration);
        if((ret = av_seek_frame(mpAVFormat, -1, target, AVSEEK_FLAG_BYTE )) < 0) {
            AM_ERROR("seek by bytes fail, ret =%d, Seek return error.\n", ret);
            return ME_ERROR;
        }
    }else
        AM_INFO("CGDemuxerFFMpegExQueue::DoSeek, seek by timestamp(%llu), ret =%d, duration:%lld, pos=%lld.\n",
        target, ret, mpAVFormat->duration, mpAVFormat->data_offset);
    mLastSeek = target;
    mBWLastPTS = target;

    return ME_OK;
}

void CGDemuxerFFMpegExQueue::OnRun()
{
    AM_ERR err;
    CMD cmd;
//    CQueue::QType type;
//    CQueue::WaitResult result;
//    AM_INT ret = 0;

    DoEnableAV();
    AM_INFO("CGDemuxerFFMpegExQueue OnRun Info: enA:%d, enV:%d\n", mbEnAudio, mbEnVideo);
    FillAudioHandleBuffer();
    FillHandleBuffer();
    mbRun = true;
    CmdAck(ME_OK);
    mState = STATE_PENDING;
    while(mbRun)
    {
        //AM_INFO("D4, mState:%d, mbEna %d, v%d\n",mState, mbEnAudio, mbEnVideo);
        if(NeedGoPending() == AM_TRUE)
            mState = STATE_PENDING;

        switch(mState)
        {
        case STATE_READ_DATA:
            if(mpWorkQ->PeekCmd(cmd))
            {
                ProcessCmd(cmd);
            }else{
                if (!mbBWIOnlyMode) {
                    err = FillBufferToQueue();
                } else {
                    err = FillVBufferOnlyKeyframeOnlyBW();
                }
                if(err != ME_OK) {
                    mState = STATE_PENDING;
                }
            }
            break;

        case STATE_PENDING:
        case STATE_PAUSE:
        case STATE_HIDED:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        default:
            AM_ERROR("Check Me %d.\n", mConfigIndex);
            break;
        }
    }
    AM_INFO("CGDemuxerFFMpegExQueue OnRun Exit.\n");
}

inline AM_BOOL CGDemuxerFFMpegExQueue::NeedGoPending()
{
    if(mState == STATE_PAUSE || mState == STATE_HIDED)
        return AM_FALSE;

    if(mbEOS == AM_TRUE)
        return AM_TRUE;
    if(mpBufferQV->GetDataCnt() >= DEMUXER_BQUEUE_NUM_V)
    {
        //AM_INFO("Warning::!! Demuxer(%d) too fast for net stream playback!\n", mConfigIndex);
        return AM_TRUE;
    }
    //not care the audio buffer info

    if (mbBWIOnlyMode && mbBWPlaybackFinished) {
        return AM_TRUE;
    }
    return AM_FALSE;
}

//Fill audio handler buffer at the beginning
inline AM_ERR CGDemuxerFFMpegExQueue::FillAudioHandleBuffer()
{
    if(mbEnAudio == AM_TRUE)
    {
        AM_INFO("FillAudioHandleBuffer\n");
        pthread_mutex_lock(&mutex);
        mAudioConsumeCnt = mAudioSentCnt = mAudioCount = 0;
        pthread_mutex_unlock(&mutex);
        mAHandleBuffer.SetOwnerType(DEMUXER_FFMPEG);
        mAHandleBuffer.SetBufferType(HANDLE_BUFFER);
        mAHandleBuffer.SetStreamType(STREAM_AUDIO);
        mAHandleBuffer.SetIdentity(mConfigIndex);
        mAHandleBuffer.SetCount(0);
        mAHandleBuffer.SetExtraPtr((AM_INTPTR)mpBufferQA);
        //mAHandleBuffer.Dump("mAHandleBuffer");
        mpBufferQA->PutData(&mAHandleBuffer, sizeof(CGBuffer));
        SendFilterMsg(GFilter::MSG_DEMUXER_A, 0);
    }
    return ME_OK;
}

//time diff -test this
inline AM_ERR CGDemuxerFFMpegExQueue::FillHandleBuffer(AM_BOOL stillGoOn)
{
    if(mbEnVideo == AM_FALSE)
        return ME_OK;

    mVHandleBuffer.SetOwnerType(DEMUXER_FFMPEG);
    mVHandleBuffer.SetBufferType(HANDLE_BUFFER);
    mVHandleBuffer.SetStreamType(STREAM_VIDEO);
    mVHandleBuffer.SetIdentity(mConfigIndex);
    mVideoCount++;
    mVHandleBuffer.SetCount(mVideoCount);
    //AM_INFO("BUFFER ON DEMUXER %p\n", mpBufferQV);
    mVHandleBuffer.SetExtraPtr((AM_INTPTR)mpBufferQV);
    if(mpConfig->hd == AM_TRUE)
    {
        mVHandleBuffer.SetFlags(HD_STREAM_HANDLE);
        if(mpGConfig->curHdIndex < 0)
        {
            //yes, we first meet hd
            mpGConfig->curHdIndex = mConfigIndex;
        }else{
            //mpBufferQV->Detach(); //detach or not fill buffer?
            if(stillGoOn == AM_FALSE){
                mVideoCount--;//counteract upper ++
                return ME_OK;
            }
        }
    }

    mpBufferQV->PutData(&mVHandleBuffer, sizeof(CGBuffer));
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::FillBufferToQueue()
{
//    static int num = 0;

    CGBuffer gBuffer;

    if(mpBufferQV->GetDataCnt() >= DEMUXER_BQUEUE_NUM_V)
    {
    }
    if(mpBufferQV->GetDataCnt() == 0)
    {
    }
    //AM_INFO("====>%d\n", mpBufferQV->GetDataCnt());
    if(mpBufferQA->GetDataCnt() <= (DEMUXER_BQUEUE_NUM_A/3) &&
        (mpBufferQV->GetDataCnt() >= (DEMUXER_BQUEUE_NUM_V/2) || mbEosV == AM_TRUE))
    {
        if(mbEnAudio && mbEosA == AM_FALSE){
            FillFullAudioQueue();
        }
    }
    if(mbEosV == AM_TRUE)
        return ME_OK;

    ReadData(gBuffer);
    if(gBuffer.GetBufferType() == EOS_BUFFER)
    {
        AM_ASSERT(gBuffer.GetStreamType() == STREAM_VIDEO);
        mVideoCount++;
        gBuffer.SetCount(mVideoCount);
        //gBuffer.SetStreamType(STREAM_VIDEO);
        mpBufferQV->PutData(&gBuffer, sizeof(CGBuffer));
        if(mbEnAudio && 0){
            mAudioCount++;
            gBuffer.SetCount(mAudioCount);
            gBuffer.SetStreamType(STREAM_AUDIO);
            mpBufferQA->PostMsg(&gBuffer, sizeof(CGBuffer));
        }
        mbEosV = AM_TRUE;
        if(mbEosA == AM_TRUE || mpConfig->disableAudio == AM_TRUE)
            mbEOS = AM_TRUE;
        //AM_INFO("Stream %d demuxer video EOS reached:%d\n", mConfigIndex, mVideoCount);
        return ME_OK;
    }

    STREAM_TYPE type = gBuffer.GetStreamType();
    if(type == STREAM_VIDEO){
        if(mbEnVideo == AM_FALSE){
            AM_ASSERT(mpBufferQV->GetDataCnt() == 0);
            FinalDataProcess(&gBuffer);
        }else{
            mVideoCount++;
            gBuffer.SetCount(mVideoCount);
            mpBufferQV->PutData(&gBuffer, sizeof(CGBuffer));
        }
    }else if(type == STREAM_AUDIO){
        AM_ASSERT(0);
        if(mbEnAudio == AM_FALSE){
            AM_ASSERT(mpBufferQA->GetDataCnt() == 0);
            FinalDataProcess(&gBuffer);
        }else{
            mAudioCount++;
            gBuffer.SetCount(mAudioCount);
            if(mpBufferQA->GetDataCnt() >= DEMUXER_BQUEUE_NUM_A){
                AM_INFO("Warning==>Audio Fill Queue Full, Check For Sync!\n");
            }
            mpBufferQA->PutData(&gBuffer, sizeof(CGBuffer));
        }
    }

#if PLATFORM_LINUX
    mpGMuxer->SendToInjector(mConfigIndex, &gBuffer,-1,-1); //TODO, fill pts/dts
#endif
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::FillVBufferOnlyKeyframeOnlyBW()
{
    AM_ERR err;
    AM_S64 seekpts;

    CGBuffer gBuffer;
    if(mbEosV == AM_TRUE)
        return ME_OK;

    if (!mbGetEstimatedKeyFrameInterval) {
        mEstimatedKeyFrameInterval = 90000 + 10000;
        mbGetEstimatedKeyFrameInterval = true;
    }

    //seek
    if ((mBWLastPTS) && (mBWLastPTS >= mEstimatedKeyFrameInterval)) {
        AMLOG_WARN("do seek %llu, %llu\n", mBWLastPTS, mEstimatedKeyFrameInterval);
//        seekpts = av_rescale_q(mBWLastPTS - mEstimatedKeyFrameInterval, (AVRational){1,IParameters::TimeUnitDen_90khz}, {1, AV_TIME_BASE});
        /*AVRational bq={0,0}, cq={0,0};
        bq.num = 1;
        bq.den = IParameters::TimeUnitDen_90khz;
        cq.num = 1;
        cq.den = AV_TIME_BASE;*/
        seekpts = mBWLastPTS - mEstimatedKeyFrameInterval;//
        //seekpts = av_rescale_q(mBWLastPTS - mEstimatedKeyFrameInterval, cq, bq);
        err = DoSeek2(seekpts);
        AM_ASSERT(ME_OK == err);
    }else{
        AMLOG_DEBUG("do not seek %llu, %llu\n", mBWLastPTS, mEstimatedKeyFrameInterval);
    }
    err = ReadVideoKeyframe(gBuffer);
    AM_ASSERT(ME_OK == err);
    //AMLOG_WARN("get key frame, pts %llu\n", gBuffer.GetPTS());

    if (ME_OK == err) {
        if (mBWLastPTS) {
            if (gBuffer.GetPTS() < mBWLastPTS) {
                AMLOG_WARN("GetPTS[%d] %llu %llu\n", mConfigIndex, mBWLastPTS, gBuffer.GetPTS());
                mBWLastPTS = gBuffer.GetPTS();
            } else if (gBuffer.GetPTS() == mBWLastPTS && (mEstimatedKeyFrameInterval<1000000)) {
                AMLOG_WARN("increase interval %llu\n", mEstimatedKeyFrameInterval);
                mEstimatedKeyFrameInterval += 3000;
            } else {
                AMLOG_WARN("bw playback to end(to file header?) bw pts %llu greater than %llu, stop bw playback\n", gBuffer.GetPTS(), mBWLastPTS);
                mbBWPlaybackFinished = true;
                mEstimatedKeyFrameInterval = 1000;
                return ME_ERROR;
            }
        } else {
            mBWLastPTS = gBuffer.GetPTS();
        }
    } else {
        AM_ERROR("read key frame fail, disable bw playback\n");
        mbBWPlaybackFinished = true;
        return ME_OK;
    }

    if(gBuffer.GetBufferType() == EOS_BUFFER)
    {
        AM_ASSERT(gBuffer.GetStreamType() == STREAM_VIDEO);
        mVideoCount++;
        gBuffer.SetCount(mVideoCount);
        //gBuffer.SetStreamType(STREAM_VIDEO);
        mpBufferQV->PutData(&gBuffer, sizeof(CGBuffer));
        if(mbEnAudio && 0){
            mAudioCount++;
            gBuffer.SetCount(mAudioCount);
            gBuffer.SetStreamType(STREAM_AUDIO);
            mpBufferQA->PostMsg(&gBuffer, sizeof(CGBuffer));
        }
        mbEosV = AM_TRUE;
        if(mbEosA == AM_TRUE || mpConfig->disableAudio == AM_TRUE)
            mbEOS = AM_TRUE;
        //AM_INFO("Stream %d demuxer video EOS reached:%d\n", mConfigIndex, mVideoCount);
        return ME_OK;
    }

    STREAM_TYPE type = gBuffer.GetStreamType();
    if(type == STREAM_VIDEO){
        if(mbEnVideo == AM_FALSE){
            AM_ASSERT(mpBufferQV->GetDataCnt() == 0);
            FinalDataProcess(&gBuffer);
        }else{
            mVideoCount++;
            gBuffer.SetCount(mVideoCount);
            mpBufferQV->PutData(&gBuffer, sizeof(CGBuffer));
        }
    }else if(type == STREAM_AUDIO){
        AM_ASSERT(0);
    }

    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::FillFullAudioQueue()
{
    CGBuffer aBuffer;
    AM_ERR err = ME_OK;
    AM_U64 curPts;
    while(1)
    {
        aBuffer.Clear();
        if(mpBufferQA->GetDataCnt() >= DEMUXER_BQUEUE_NUM_A){
            break;
        }
        err = mpAudioMan->ReadAudioData(mConfigIndex, aBuffer);
        if(err == ME_CLOSED){
            //eos
            aBuffer.SetPTS(0);
            mbEosA = AM_TRUE;
            mbEOS = (mbEosV == AM_TRUE);
        }else{
            curPts = ConvertAudioPts(aBuffer.GetPTS());
            aBuffer.SetPTS(curPts);
        }
        mAudioCount++;
        aBuffer.SetCount(mAudioCount);
        mpBufferQA->PutData(&aBuffer, sizeof(CGBuffer));
        if(mbEosA == AM_TRUE)
            break;
    }
    if(mpBufferQV->GetDataCnt() <= 0){
        //AM_ASSERT(0);
    }
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::FillOnlyReceive(CGBuffer& buffer)
{
    AM_ASSERT(0);
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::ReadData(CGBuffer& buffer)
{
    AM_INT ret;
    AM_U64 curPts = 0;
    while(1)
    {
        ret = av_read_frame(mpAVFormat, &mPkt);
        if(ret < 0)
        {
            AM_INFO("Stream %d av_read_frame()<0(ret=%d).\n",mConfigIndex, ret);
            FillEOS(buffer);
            av_free_packet(&mPkt);
            break;
        }

        if(mPkt.stream_index != mVideo)//intro AudioManager && mPkt.stream_index != mAudio)
        {
            //AM_INFO("Discard AVPacket :Index:%d(A:%d, V:%d), stream_index:%d\n", mConfigIndex, mAudio, mVideo, mPkt.stream_index);
            av_free_packet(&mPkt);
            continue;
        }

        av_dup_packet(&mPkt);
        AVPacket* pPkt = new AVPacket;
        ::memcpy(pPkt, &mPkt, sizeof(mPkt));

        buffer.SetExtraPtr((AM_INTPTR)pPkt);
        buffer.SetOwnerType(DEMUXER_FFMPEG);
        buffer.SetBufferType(DATA_BUFFER);
        buffer.SetIdentity(mConfigIndex);
        if(mPkt.stream_index == mVideo)
        {
            buffer.SetStreamType(STREAM_VIDEO);
            if(mPkt.pts >= 0){
                curPts = ConvertVideoPts(mPkt.pts);
            }else{
                curPts = ConvertVideoPts(mPkt.dts);
            }
        }else if(mPkt.stream_index == mAudio){
            curPts = ConvertAudioPts(mPkt.pts);
            buffer.SetStreamType(STREAM_AUDIO);
        }
        buffer.SetPTS(mLastLoopPts + curPts);//for loop playback
        av_init_packet(&mPkt);
        break;
    }
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::ReadVideoKeyframe(CGBuffer& buffer)
{
    AM_INT ret;
    AM_U64 curPts = 0;
    while(1)
    {
        ret = av_read_frame(mpAVFormat, &mPkt);
        if(ret < 0)
        {
            AM_INFO("av_read_frame()<0, stream end? ret=%d.\n", ret);
            FillEOS(buffer);
            av_free_packet(&mPkt);
            break;
        }

        if(mPkt.stream_index != mVideo)//intro AudioManager && mPkt.stream_index != mAudio)
        {
            //AM_INFO("Discard AVPacket :Index:%d(A:%d, V:%d), stream_index:%d\n", mConfigIndex, mAudio, mVideo, mPkt.stream_index);
            av_free_packet(&mPkt);
            continue;
        }

        if(!(mPkt.flags & AV_PKT_FLAG_KEY))//discard non-key frame
        {
            //AM_INFO("Discard non-key AVPacket :Index:%d(A:%d, V:%d), stream_index:%d\n", mConfigIndex, mAudio, mVideo, mPkt.stream_index);
            av_free_packet(&mPkt);
            continue;
        }

        av_dup_packet(&mPkt);
        AVPacket* pPkt = new AVPacket;
        ::memcpy(pPkt, &mPkt, sizeof(mPkt));

        buffer.SetExtraPtr((AM_INTPTR)pPkt);
        buffer.SetOwnerType(DEMUXER_FFMPEG);
        buffer.SetBufferType(DATA_BUFFER);
        buffer.SetIdentity(mConfigIndex);
        if(mPkt.stream_index == mVideo)
        {
            buffer.SetStreamType(STREAM_VIDEO);
            if(mPkt.pts >= 0){
                curPts = ConvertVideoPts(mPkt.pts);
            }else{
                curPts = ConvertVideoPts(mPkt.dts);
            }
        }else if(mPkt.stream_index == mAudio){
            curPts = ConvertAudioPts(mPkt.pts);
            buffer.SetStreamType(STREAM_AUDIO);
        }
        buffer.SetPTS(curPts);
        av_init_packet(&mPkt);
        return ME_OK;
    }
    return ME_ERROR;
}

AM_ERR CGDemuxerFFMpegExQueue::FillEOS(CGBuffer& buffer)
{
    //AM_INFO("Fill EOS To Buffer. Stream: %d\n", mConfigIndex);
    buffer.SetBufferType(EOS_BUFFER);
    buffer.SetOwnerType(DEMUXER_FFMPEG);
    buffer.SetIdentity(mConfigIndex);
    buffer.SetStreamType(STREAM_VIDEO);
    return ME_OK;
}

AM_BOOL CGDemuxerFFMpegExQueue::GetBufferPolicy(CGBuffer& gBuffer)
{
    return mpBufferQV->PeekData(&gBuffer, sizeof(CGBuffer));
}

AM_ERR CGDemuxerFFMpegExQueue::GetAudioBuffer(CGBuffer& oBuffer)//a-v spe
{
    //AM_INFO("DDDDDDDDDDD2, %d, %d, %d\n", mState, mbEnAudio, mbAHandleSent);
    //AM_INFO("Audio NUm:%d\n", mpBufferQA->GetDataCnt());
    if(mpBufferQA->GetDataCnt() <= 0)
        return ME_CLOSED;

    AM_ASSERT(mpBufferQA->GetDataCnt() == 1);
    mpBufferQA->PeekData(&oBuffer, sizeof(CGBuffer));
    //oBuffer.Dump("GetAudioBuffer");
    //detach
//    CQueue* mainQA = mpAudioMan->AudioMainQ();
    //mainQA->Dump("Before on Demuxer");
    mpBufferQA->Detach();
    mAudioSentCnt++;
    //mainQA->Dump("After on Demuxer");
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::GetCGBuffer(CGBuffer& buffer)
{
    if(mState == STATE_PAUSE){
        return ME_CLOSED;
    }
    if(mState == STATE_HIDED){
        return ME_CLOSED;
    }
    AM_BOOL rval = AM_FALSE;

    while(1)
    {
        //first send out retrieveQ
        if(mpRetrieveQ->GetDataCnt() > 0){
            //note this will always be true
            AM_ASSERT(0);
        }else{
            rval = GetBufferPolicy(buffer);
        }

        if(rval == AM_FALSE)
        {
            //no data(be freed), read from mpBufferQV. Never happen after a-v spe
            AM_ASSERT(0);
            return ME_ERROR;
        }

        BUFFER_TYPE btype = (BUFFER_TYPE)buffer.GetBufferType();
        STREAM_TYPE type = (STREAM_TYPE)buffer.GetStreamType();
        AM_ASSERT(type == STREAM_VIDEO);
        AM_ASSERT(btype == HANDLE_BUFFER);
        mpBufferQV->Detach();
        break;
    }
    //buffer.Dump("INSIDE!!");
    mVideoSentCnt++;
    return ME_OK;
}

inline AM_ERR CGDemuxerFFMpegExQueue::ContinueRun()
{
    if(mState != STATE_PENDING)
    {
        //AM_INFO("ContinueRun on state_runing, Data Cnt: %d\n", mpBufferQ->GetDataCnt());
        return ME_OK;
    }
    //or send msg and oncmd handle.
    if(mbRun == true)
        mpWorkQ->PostMsg(CMD_ACK);
    return ME_OK;
}

inline AM_BOOL CGDemuxerFFMpegExQueue::NeedSendAudio(CGBuffer* buffer)
{
    AM_ASSERT(0);
    return false;
}

AM_ERR CGDemuxerFFMpegExQueue::OnReleaseBuffer(CGBuffer* buffer)
{
    //CGBuffer dumpBuffer = *buffer;
    pthread_mutex_lock(&mutex);
    if(buffer->GetStreamType() == STREAM_VIDEO){
        AM_ASSERT(mVideoConsumeCnt == buffer->GetCount());
        //buffer->Dump("OnReleaseBuffer");
        if(mVideoConsumeCnt != buffer->GetCount())
            AM_INFO("DIff(%d) : %d-->%d\n", mConfigIndex, mVideoConsumeCnt, buffer->GetCount());

        mVideoConsumeCnt++;
    }else{
        AM_ASSERT(mAudioConsumeCnt == buffer->GetCount());
        if(mAudioConsumeCnt != buffer->GetCount())
            AM_INFO("DIff(%d) : %d-->%d\n", mConfigIndex, mAudioConsumeCnt, buffer->GetCount());
        mAudioConsumeCnt++;
    }

    if(buffer->GetBufferType() != DATA_BUFFER){
        pthread_mutex_unlock(&mutex);
        return ME_OK;
    }

    //1change to handContinueRun()
    if(mpBufferQV->GetDataCnt() <= (DEMUXER_BQUEUE_NUM_V /3))
    {
        ContinueRun();//inline this function.
    }
    if(mpBufferQA->GetDataCnt() <= 0)
    {
        ContinueRun();
    }

    FinalDataProcess(buffer);
    pthread_mutex_unlock(&mutex);
    //AM_INFO(" CGDemuxerFFMpegExQueue OnReleaseBuffer Done!!\n");
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::OnRetrieveBuffer(CGBuffer* buffer)
{
    AM_ASSERT(0);
    return ME_OK;
}

inline AM_ERR CGDemuxerFFMpegExQueue::FinalDataProcess(CGBuffer* buffer)
{
    if(buffer->GetBufferType()!=DATA_BUFFER){
        AM_INFO("GetBuffertype()=%d.\n", buffer->GetBufferType());
        return ME_NOT_EXIST;
    }
    //you cando something skip this
    //if(globalFlag
    //AM_INFO("degbegege buffer %d\n", buffer->GetIdentity());
    if(1){
        mpGMuxer->AutoMuxer(mConfigIndex, buffer);
    }else{
        AVPacket* pkt = (AVPacket* )(buffer->GetExtraPtr());
        if(pkt != NULL){
            av_free_packet(pkt);
            delete pkt;
            buffer->SetExtraPtr((AM_INTPTR)NULL);
        }else{
            AM_ASSERT(0);
        }
    }
    return ME_OK;
}
//===========================================
//
//===========================================
AM_ERR CGDemuxerFFMpegExQueue::PostFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    msg.p1 = mConfigIndex;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->PostMsgToFilter(msg);
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::SendFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    //AM_INFO("SendFilterMsg\n");
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    msg.p1 = mConfigIndex;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->SendMsgToFilter(msg);
    return ME_OK;
}

//========================================================================
//
//
//after accept this file, configure env.
AM_ERR CGDemuxerFFMpegExQueue::LoadFile()
{
    AM_ERR err;
    int rval = av_find_stream_info(mpAVFormat);
    if (rval < 0)
    {
        AM_ERROR("av_find_stream_info failed\n");
        return ME_ERROR;
    }
#ifdef AM_DEBUG
    //dump_format(mpAVFormat, 0, mpAVFormat->filename, 0);
#endif

    // find video stream & audio stream
    mVideo = FindMediaStream(AVMEDIA_TYPE_VIDEO);
    mAudio = FindMediaStream(AVMEDIA_TYPE_AUDIO);
    if (mVideo < 0 && mAudio < 0)
    {
        AM_ERROR("No video, no audio, Check me!\n");
        return ME_ERROR;
    }
    AM_INFO("CGDemuxerFFMpegExQueue: FindMediaStream video[%d]audio[%d].\n", mVideo, mAudio);

    if (mVideo >= 0)
    {
        err = SetFormat(mVideo, AVMEDIA_TYPE_VIDEO);
        if(err == ME_BAD_FORMAT)
            return err;
        if (err != ME_OK) {
            AM_ERROR("InitStream video stream fail, disable it.\n");
            mpConfig->disableVideo = true;
            mpConfig->hasVideo = false;
            //mbEnVideo = false;
        }
    }else{
        //disagree for novideo
        //return ME_BAD_FORMAT;
        mpConfig->disableVideo = true;
        mpConfig->hasVideo = false;
        //mbEnVideo = false;
    }

    if (mAudio >= 0)
    {
        err = SetFormat(mAudio, AVMEDIA_TYPE_AUDIO);
        if(err == ME_BAD_FORMAT)
            return err;
        if (err != ME_OK) {
            AM_ERROR("initStream audio stream fail, disable it.\n");
            mpConfig->disableAudio = true;
            mpConfig->hasAudio = false;
            //mbEnAudio = false;
        }
    }else{
        //AM_ERROR("initStream audio stream fail, disable it.\n");
        mpConfig->disableAudio = true;
        mpConfig->hasAudio = false;
        //mbEnAudio = false;
    }

    //mbEnAudio = !mpConfig->disableAudio;
    //mbEnVideo = !mpConfig->disableVideo;

    UpdatePTSConvertor();
    //OR mVHandleBuffer.ffmpegStream = stream; think about this.
    CParam envPar(2);
    mpGConfig->DemuxerSetDecoderEnv(mConfigIndex, (void* )mpAVFormat, envPar);

    mpAudioMan->AddAudioSource(mConfigIndex, mAudio);
    //Set Muxer info here
    SetupMuxerInfo();

    if((mVideoWidth == 0 ||mVideoHeight == 0) && mVideo >= 0){
        mVideoWidth = mpAVFormat->streams[mVideo]->codec->width;
        mVideoHeight = mpAVFormat->streams[mVideo]->codec->height;
    }
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::SetupMuxerInfo()
{
    AVStream* videoStream = NULL;
    AVStream* audioStream = NULL;
    AVCodecContext* curCodec = NULL;
    CParam videoPar(8);
    CParam audioPar(5);
    if(mVideo >= 0)
    {
        videoStream = mpAVFormat->streams[mVideo];
        curCodec = videoStream->codec;
        videoPar[0] = curCodec->width;
        videoPar[1] = curCodec->height;
        videoPar[2] = curCodec->bit_rate;
        videoPar[3] = curCodec->codec_id;
        videoPar[4] = curCodec->extradata_size;
        videoPar[5] = (AM_INTPTR)curCodec->extradata;
        videoPar[6] = videoStream->r_frame_rate.den;
        videoPar[7] = videoStream->r_frame_rate.num;
        mpGConfig->mainMuxer->SetupMuxerInfo(mConfigIndex, AM_TRUE, videoPar);
    }

    if(mAudio >= 0)
    {
        audioStream = mpAVFormat->streams[mAudio];
        curCodec = audioStream->codec;
        audioPar[0] = curCodec->channels;
        audioPar[1] = curCodec->sample_rate;
        audioPar[2] = curCodec->bit_rate;
        audioPar[3] = curCodec->sample_fmt;
        audioPar[4] = curCodec->codec_id;
        mpGConfig->mainMuxer->SetupMuxerInfo(mConfigIndex, AM_FALSE, audioPar);
    }
    return ME_OK;
}

int CGDemuxerFFMpegExQueue::hasCodecParameters(AVCodecContext* enc)
{
    int val = 1;
    switch(enc->codec_type)
    {
        case AVMEDIA_TYPE_AUDIO:
            if(!enc->frame_size
                  && (enc->codec_id == CODEC_ID_VORBIS ||enc->codec_id == CODEC_ID_AAC ||enc->codec_id == CODEC_ID_SPEEX))
            {
                val  = 0;
            }
            else
            {
                val = enc->sample_rate && enc->channels && enc->sample_fmt != SAMPLE_FMT_NONE;
            }

            break;

        case AVMEDIA_TYPE_VIDEO:
            val = enc->width && enc->pix_fmt != PIX_FMT_NONE;
            break;

        case AVMEDIA_TYPE_SUBTITLE:
            val = 1;
            break;

        default:
            val = 1;
            break;
    }

    if(avcodec_find_decoder(enc->codec_id) == NULL || enc->codec_id == CODEC_ID_NONE)
    {
        val = 0;
    }

    return val;
}

AM_INT CGDemuxerFFMpegExQueue::FindMediaStream(AM_INT media)
{
    for (AM_UINT i = 0; i < mpAVFormat->nb_streams; i++)
    {
        AVCodecContext* avctx = mpAVFormat->streams[i]->codec;
        if (avctx->codec_type == media)
        {
            if (hasCodecParameters(avctx))
            {
                return i;
            }

        }
    }
    return -1;
}

void CGDemuxerFFMpegExQueue::UpdatePTSConvertor()
{
    AMLOG_PRINTF("..mpAVFormat %p, mVideo %d, mAudio %d.\n", mpAVFormat, mVideo, mAudio);
    AMLOG_PRINTF(" [PTS] FFDemuxer: UpdatePTSConvertor system num %d, den %d.\n", CLOCKTIMEBASENUM, CLOCKTIMEBASEDEN);
    if (mVideo >= 0) {
        AM_ASSERT(mpAVFormat->streams[mVideo]->time_base.num ==1);
        AMLOG_PRINTF(" [PTS] FFDemuxer: UpdatePTSConvertor video(%d) stream num %d, den %d.\n", mVideo, mpAVFormat->streams[mVideo]->time_base.num, mpAVFormat->streams[mVideo]->time_base.den);
        mPTSVideo_Num = mpAVFormat->streams[mVideo]->time_base.num * CLOCKTIMEBASEDEN;
        mPTSVideo_Den = mpAVFormat->streams[mVideo]->time_base.den * CLOCKTIMEBASENUM;
        if ((mPTSVideo_Num%mPTSVideo_Den) == 0) {
            mPTSVideo_Num /= mPTSVideo_Den;
            mPTSVideo_Den = 1;
        }
        AMLOG_PRINTF("    video convertor num %d, den %d.\n", mPTSVideo_Num, mPTSVideo_Den);
    }
    if (mAudio >= 0) {
        AM_ASSERT(mpAVFormat->streams[mAudio]->time_base.num ==1);
        AMLOG_PRINTF(" [PTS] FFDemuxer: UpdatePTSConvertor mAudio(%d) stream num %d, den %d.\n", mAudio, mpAVFormat->streams[mAudio]->time_base.num, mpAVFormat->streams[mAudio]->time_base.den);
        mPTSAudio_Num = mpAVFormat->streams[mAudio]->time_base.num * CLOCKTIMEBASEDEN;
        mPTSAudio_Den = mpAVFormat->streams[mAudio]->time_base.den * CLOCKTIMEBASENUM;
        if ((mPTSAudio_Num%mPTSAudio_Den) == 0) {
            mPTSAudio_Num /= mPTSAudio_Den;
            mPTSAudio_Den = 1;
        }
        AMLOG_PRINTF("    audio convertor num %d, den %d.\n", mPTSAudio_Num, mPTSAudio_Den);
    }
}

AM_U64 CGDemuxerFFMpegExQueue::ConvertVideoPts(am_pts_t pts)
{
    pts *= mPTSVideo_Num;
    if (mPTSVideo_Den != 1) {
        pts /=  mPTSVideo_Den;
    }
    mLastPts = pts;
    return pts;
}

AM_U64 CGDemuxerFFMpegExQueue::ConvertAudioPts(am_pts_t pts)
{
    pts *= mPTSAudio_Num;
    if (mPTSAudio_Den != 1) {
        pts /=  mPTSAudio_Den;
    }
    mLastAudioPts = pts;
    if(bFirstAudioData){
        bFirstAudioData = AM_FALSE;
        mFirstAudioPts = pts;
    }
    return pts;
}
//
//
//===========================================================//
void CGDemuxerFFMpegExQueue::Test(const char* info)
{
    AM_INFO("%s", info);
    IUDECHandler* mpDsp = mpGConfig->dspConfig.udecHandler;

    AM_UINT eos;
    AM_U64 pts;

    AM_INT i = 0;
    if(mConfigIndex == 0)
        i = 4;

    mpDsp->GetUdecTimeEos(i, eos, pts, 1);
    AM_INFO("Udec %d, Show LastPts:%lld, Change to frame Cnt:%lld.\n", i, pts, pts/3003);
}
int CGDemuxerFFMpegExQueue::Test2(AM_INT i)
{
    IUDECHandler* mpDsp = mpGConfig->dspConfig.udecHandler;

    AM_UINT eos;
    AM_U64 pts;

    mpDsp->GetUdecTimeEos(i, eos, pts, 1);
    if((AM_S64)pts == -1){
            AM_INFO("Udec %d, LastPts :%lld\n", i, pts);
        return 0;
    }else{
    AM_INFO("Udec %d, LastPts:%lld, Change to frame Cnt:%lld.\n", i, pts, pts/3003);
        return 1;
    }
}

AM_U64 CGDemuxerFFMpegExQueue::QueryVideoPts()
{
    IUDECHandler* mpDsp = mpGConfig->dspConfig.udecHandler;
    AM_UINT eos;
    AM_U64 pts;
    AM_ERR err;

    err = mpDsp->GetUdecTimeEos(mpGConfig->indexTable[mConfigIndex].dspIndex, eos, pts, 1);
    if(err != ME_OK){
        AM_INFO("Udec %d, LastPts :%lld\n", mConfigIndex, pts);
        return 0;
    }else{
        AM_INFO("Udec %d, LastPts:%lld, Change to ff time:%lld.\n", mConfigIndex, pts, pts* mPTSVideo_Den / mPTSVideo_Num);
        return pts* mPTSVideo_Den / mPTSVideo_Num;
    }
}
#if 1
AM_ERR CGDemuxerFFMpegExQueue::QueryInfo(AM_INT type, CParam64& param)
{
    AM_U64 length=0;
    AM_ERR err=ME_OK;
    switch(type)
    {
    case VIDEO_WIDTH_HEIGHT:
        param[0] = mVideoWidth;
        param[1] = mVideoHeight;
        break;

    case INFO_TIME_PTS:
        param[0] = mLastAudioPts;//mLastPts;
        err=GetTotalLength(length);
        if(ME_OK!=err)
        {
            length = 0;
        }
        param[1] = length;
        param[2] = mpAVFormat->start_time*1000/AV_TIME_BASE;
        break;

    default:
        AM_ERROR("Demuxer(%d) Query Wrong Info Type.\n", mConfigIndex);
    };

    return ME_OK;
}

AM_ERR CGDemuxerFFMpegExQueue::SeekTo(AM_U64 timestamp, AM_INT flag)
{
    //ms is based on CLOCKTIMEBASEDEN
    AM_ASSERT(AV_TIME_BASE == 1000000);
    AM_U64 ffTimestamp;
    if(flag & SEEK_BY_USER){
        if((mpAVFormat->duration >0) && ((AM_S64)timestamp > (mpAVFormat->duration)*1000/AV_TIME_BASE)){
            AM_ERROR("SeekTo: seek point is out of this file!: duration: %lldms, seek to %lldms.\n",
                mpAVFormat->duration*1000/AV_TIME_BASE, timestamp);
            return ME_BAD_PARAM;
        }
        ffTimestamp = timestamp;
    }else{
        ffTimestamp = timestamp * mPTSVideo_Den / mPTSVideo_Num;//pts
    }
    AM_INFO("CGDemuxerFFMpegExQueue::SeekTo: %u, %u; %llu->%llu\n",
        mPTSVideo_Den, mPTSVideo_Num, timestamp, ffTimestamp);
    //Note All of ms and ffmpegms is a timetick of based den
    //AM_U64 seek_time_second = ms * CLOCKTIMEBASENUM / CLOCKTIMEBASEDEN;
    //AM_U64 seek_time_ms = seek_timestamp_second * 1000;
    AM_ERR err;

    //Test(mConfigIndex);

    if(flag & SEEK_BY_SWITCH){
        ffTimestamp += FRAME_TIME_DIFF(2);//last pts is still goon, this is a exp value.
        mSeekCheck = ffTimestamp;
    }else{
        mSeekCheck = -1;
    }
    //AM_INFO("Demuxer(%d), Seek time for ffmpeg:%lld->%lld, Shown Frame:%d.\n", mConfigIndex, timestamp, ffTimestamp
        //, timestamp /3003);
    //debug
    /*
    if(mpGConfig->globalFlag & SWITCH_ARM_SEEK_FEED_DSP_DEBUG ||
       mpGConfig->globalFlag & SWITCH_JUST_FOR_WIN_ARM ){
        if(mConfigIndex == 0)
            Test("************Begin Switch Back (4->0) PTS Info: ");
        else
            Test("************Begin Switch (0->4) PTS Info: ");
    }
    */

    CMD cmd(CMD_SEEK);
    if((flag & SEEK_BY_USER)){
        AM_U64 starttime = 0;
        if((AM_U64)mpAVFormat->start_time != AV_NOPTS_VALUE){
            starttime = mpAVFormat->start_time;
        }
        /*ffTimestamp = (ffTimestamp+starttime)
            *mpAVFormat->streams[mVideo]->time_base.den
            /mpAVFormat->streams[mVideo]->time_base.num/1000;*/
        ffTimestamp += starttime*1000/AV_TIME_BASE;
        ffTimestamp = ffTimestamp * mpAVFormat->streams[mVideo]->time_base.den
            /mpAVFormat->streams[mVideo]->time_base.num*1000/AV_TIME_BASE;
        AM_INFO("CGDemuxerFFMpegExQueue::starttime: %llu, ffTimestamp: %llu\n",
        starttime, ffTimestamp);
    }
    cmd.res64_1 = ffTimestamp;
    err = (MsgQ())->SendMsg(&cmd, sizeof(CMD));

    if((mAudio>0) && (flag & SEEK_BY_USER)){
        //Fixme: convert pts from video 2 audio?
        //audio & video pts here have same timebase
        mpAudioMan->SetAudioPosition(mConfigIndex, ffTimestamp);
        //mpWorkQ->PostMsg(CMD_ACK);
    }

    if(flag & SEEK_FOR_LOOP){
        AM_ASSERT(mbEOS == AM_TRUE);
        AM_ASSERT(mpBufferQA->GetDataCnt() == 0);
        AM_ASSERT(mpBufferQV->GetDataCnt() == 0);
        mbEosA = mbEosV = mbEOS = AM_FALSE;
        mLastLoopPts += mLastPts;
        mpAudioMan->SetAudioPosition(mConfigIndex, ffTimestamp);
        mpWorkQ->PostMsg(CMD_ACK);
    }

    if(mSeekCheck != -1)
    {
        //return DoSeekCheck();
    }
    return err;
}
#else
AM_ERR CGDemuxerFFMpegExQueue::Seek(AM_U64 ms)
{
	int stream_index,seek_flags;
	seek_flags  = AVSEEK_FLAG_BACKWARD;//0;
	stream_index = -1;
#if 1
	// Audio first
	if (mAudio >= 0)
		stream_index = mAudio;
	else if (mVideo >= 0)
		stream_index = mVideo;
#else
	stream_index = mVideo;
#endif
	AM_U64 seek_target = (ms * AV_TIME_BASE) / 1000;
	seek_target= av_rescale_q(seek_target, AV_TIME_BASE_Q, mpAVFormat->streams[stream_index]->time_base);

	if (av_seek_frame(mpAVFormat, stream_index, seek_target, seek_flags) < 0) {
		AM_ERROR("error while seeking\n");
		return ME_ERROR;
	}
	return ME_OK;
}
#endif


AM_ERR CGDemuxerFFMpegExQueue::GetTotalLength(AM_U64& ms)
{
    if(!mpAVFormat)
        return ME_ERROR;
    if(mpAVFormat->duration < 0)
    {
        mpAVFormat->duration = 0;
    }
    ms = (mpAVFormat->duration * 1000) / AV_TIME_BASE;
    return ME_OK;
}

//todo , let this tobe demuxer's set format
AM_ERR CGDemuxerFFMpegExQueue::SetFormat(int stream, int media)
{
    AVStream *pStream = mpAVFormat->streams[stream];
    AVCodecContext *pCodec = pStream->codec;
    AM_ERR err = ME_OK;
    if (media == AVMEDIA_TYPE_VIDEO) {
        //AMLOG_INFO("FFDemuxer get video stream, pCodec->codec_id=%x, %p.\n", (AM_INT)pCodec->codec_id, &mMediaFormat);
        switch (pCodec->codec_id)
        {
            case CODEC_ID_MPEG1VIDEO:
                mVHandleBuffer.codecType = &GUID_AmbaVideoDecoder;
                break;
            case CODEC_ID_MPEG2VIDEO:
                //mpFilter->mpSharedRes->decCurrType = mpFilter->mpSharedRes->decSet[SCODEC_MPEG12].dectype;
                //if(mpFilter->mpSharedRes->decCurrType == DEC_HARDWARE)
                //mpGConfig->DecoderChoose[codec_id]
                mVHandleBuffer.codecType = &GUID_AmbaVideoDecoder;
                break;
            case CODEC_ID_MPEG4:
            //case CODEC_ID_XVID:
                mVHandleBuffer.codecType = &GUID_AmbaVideoDecoder;
                break;
            case CODEC_ID_H264:
                if(mpGConfig->mDecoderCap & DECODER_CAP_DSP)
                    mVHandleBuffer.codecType = &GUID_AmbaVideoDecoder;
                else if(mpGConfig->mDecoderCap & DECODER_CAP_COREAVC)
                    mVHandleBuffer.codecType = &GUID_Video_FF_OTHER_CODECS;
                else
                    mVHandleBuffer.codecType = &GUID_Video_H264;
                break;
            case CODEC_ID_RV10:             mVHandleBuffer.codecType = &GUID_Video_RV10; break;
            case CODEC_ID_RV20:             mVHandleBuffer.codecType = &GUID_Video_RV20; break;
            case CODEC_ID_RV30:             mVHandleBuffer.codecType = &GUID_Video_RV30; break;
            case CODEC_ID_RV40:
                mVHandleBuffer.codecType = &GUID_Video_RV40;
                break;
            case CODEC_ID_MSMPEG4V1:
                mVHandleBuffer.codecType = &GUID_Video_MSMPEG4_V1;
                break;
            case CODEC_ID_MSMPEG4V2:
                mVHandleBuffer.codecType = &GUID_Video_MSMPEG4_V2;
                break;
            case CODEC_ID_MSMPEG4V3:
                mVHandleBuffer.codecType = &GUID_Video_MSMPEG4_V3;
                break;
            case CODEC_ID_WMV1:
                mVHandleBuffer.codecType = &GUID_Video_WMV1;
                break;

            case CODEC_ID_WMV2:           mVHandleBuffer.codecType = &GUID_Video_WMV2; break;
            case CODEC_ID_WMV3:
                mVHandleBuffer.codecType = &GUID_AmbaVideoDecoder;
                break;
            case CODEC_ID_H263P:
                mVHandleBuffer.codecType = &GUID_Video_H263P;
                break;
            case CODEC_ID_H263I:
                mVHandleBuffer.codecType = &GUID_Video_H263I;
                break;
            case CODEC_ID_FLV1:
                mVHandleBuffer.codecType = &GUID_Video_FLV1;
                break;
            case CODEC_ID_VC1:
                mVHandleBuffer.codecType = &GUID_AmbaVideoDecoder;
                break;
            case CODEC_ID_VP8:              mVHandleBuffer.codecType = &GUID_Video_VP8; break;

            case CODEC_ID_NONE:
                AM_ERROR("ffmpeg not recognized stream. Check me\n");
                err = ME_BAD_FORMAT;

            default:
                mVHandleBuffer.codecType = &GUID_Video_FF_OTHER_CODECS;
                AM_INFO("Other video format, codec id = 0x%x\n", pCodec->codec_id);
                break;
        }
        mVHandleBuffer.ffmpegStream = stream;
    } else if (media == AVMEDIA_TYPE_AUDIO) {
        //AMLOG_INFO("FFDemuxer get audio stream, pCodec->codec_id=%x, %p.\n", (AM_INT)pCodec->codec_id, &mMediaFormat);
        switch (pCodec->codec_id)
        {
            case CODEC_ID_MP2:              mAHandleBuffer.codecType = &GUID_Audio_MP2; break;
#if PLATFORM_ANDROID
            case CODEC_ID_MP3PV:
#endif
            case CODEC_ID_MP3:              mAHandleBuffer.codecType = &GUID_Audio_MP3; break;
            case CODEC_ID_AAC:              mAHandleBuffer.codecType = &GUID_Audio_AAC; break;
            case CODEC_ID_AC3:              mAHandleBuffer.codecType = &GUID_Audio_AC3; break;
            case CODEC_ID_DTS:              mAHandleBuffer.codecType = &GUID_Audio_DTS; break;

            case CODEC_ID_WMAV1:        mAHandleBuffer.codecType = &GUID_Audio_WMAV1; break;
            case CODEC_ID_WMAV2:        mAHandleBuffer.codecType = &GUID_Audio_WMAV2; break;

            case CODEC_ID_COOK:          mAHandleBuffer.codecType = &GUID_Audio_COOK; break;
            case CODEC_ID_VORBIS:       mAHandleBuffer.codecType = &GUID_Audio_VORBIS; break;

            case CODEC_ID_PCM_S16LE:
            //case CODEC_ID_PCM_S16BE:
            case CODEC_ID_PCM_U16LE:
            //case CODEC_ID_PCM_U16BE:
            //case CODEC_ID_PCM_S8:
            //case CODEC_ID_PCM_U8:
            //case CODEC_ID_PCM_MULAW:
            //case CODEC_ID_PCM_ALAW:
            case CODEC_ID_PCM_S32LE:
            //case CODEC_ID_PCM_S32BE:
            case CODEC_ID_PCM_U32LE:
            //case CODEC_ID_PCM_U32BE:
            case CODEC_ID_PCM_S24LE:
            //case CODEC_ID_PCM_S24BE:
            case CODEC_ID_PCM_U24LE:
            //case CODEC_ID_PCM_U24BE:
            //case CODEC_ID_PCM_S24DAUD:
            //case CODEC_ID_PCM_ZORK:
            //case CODEC_ID_PCM_S16LE_PLANAR:
            //case CODEC_ID_PCM_DVD:
            //case CODEC_ID_PCM_F32BE:
            //case CODEC_ID_PCM_F32LE:
            //case CODEC_ID_PCM_F64BE:
            //case CODEC_ID_PCM_F64LE:
            //case CODEC_ID_PCM_BLURAY:
                AM_PRINTF("will use amba_audio_decoder.\n");
                mAHandleBuffer.codecType = &GUID_Audio_PCM;
                //mMediaFormat.reserved1 = (AM_UINT)pCodec->codec_id;
                break;

            case CODEC_ID_NONE:
                AM_ERROR("ffmpeg not recognized stream. Check me\n");
                err = ME_BAD_FORMAT;

            default:
                mAHandleBuffer.codecType = &GUID_Audio_FF_OTHER_CODECS;
                AM_INFO("Other audio format, codec id = 0x%x\n", pCodec->codec_id);
                break;
        }
        mAHandleBuffer.ffmpegStream = stream;
    }else{
        err = ME_ERROR;
    }
    return err;
}

inline AM_INT CGDemuxerFFMpegExQueue::FRAME_TIME_DIFF(AM_INT diff)
{
    AM_INT ptsDiff;
    AM_INT temp, temp1;
    AM_INT eachDiff;
    temp = mpAVFormat->streams[mVideo]->time_base.den * mpAVFormat->streams[mVideo]->r_frame_rate.den;
    temp1 = mpAVFormat->streams[mVideo]->time_base.num * mpAVFormat->streams[mVideo]->r_frame_rate.num;
    eachDiff = temp /temp1;
    //AM_INFO("Debug: %d, %d, %d, %d-->$&%d / %d  = %d\n", mpAVFormat->streams[mVideo]->time_base.num, mpAVFormat->streams[mVideo]->time_base.den,
        //mpAVFormat->streams[mVideo]->r_frame_rate.num, mpAVFormat->streams[mVideo]->r_frame_rate.den, temp, temp1, eachDiff);
    //AM_INFO("Debug:%d, %d\n", mpAVFormat->streams[mVideo]->avg_frame_rate.num, mpAVFormat->streams[mVideo]->avg_frame_rate.den
            //);
    ptsDiff = eachDiff * diff;
    return ptsDiff;
}

void CGDemuxerFFMpegExQueue::Dump(AM_INT flag)
{
    AM_INFO("       {---FFMpeg Demuxer ExQueue(%p)---}\n", this);
    AM_INFO("           Config->en audio:%d, en video:%d\n", mbEnAudio, mbEnVideo);
    AM_INFO("           State->%d, paused:%d, Hided:%d\n", mState, mpConfig->paused, mpConfig->hided);
    AM_INFO("           BufferPool->Audio Bp(%p) Cnt:%d, Video Bp Cnt:%d, Retrieve Q Cnt:%d\n", mpBufferQA,
        mpBufferQA->GetDataCnt(), mpBufferQV->GetDataCnt(), mpRetrieveQ->GetDataCnt());
}



