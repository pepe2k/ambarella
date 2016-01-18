
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
#define LOG_TAG "g_ffmpeg_demux_net_exqueue"
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

#include "g_ffmpeg_demuxer_exnet.h"

GDemuxerParser gFFMpegDemuxerNetEx = {
    "FFMpegDemuxer-G-Net-Ex",
    CGDemuxerFFMpegNetEx::Create,
    CGDemuxerFFMpegNetEx::ParseFile,
    CGDemuxerFFMpegNetEx::ClearParse,
};

#define SEEK_GAP (FRAME_TIME_DIFF(10))
//-----------------------------------------------------------------------
//
// CGDemuxerFFMpegNetEx
//
//-----------------------------------------------------------------------
//accept this file or not.
#if NEW_RTSP_CLIENT
/*AmfRtspClient*/void * CGDemuxerFFMpegNetEx::mpAVArray[MDEC_SOURCE_MAX_NUM] = {NULL,};
int CGDemuxerFFMpegNetEx::mpAVArrayType[MDEC_SOURCE_MAX_NUM] = {0};
#else
AVFormatContext* CGDemuxerFFMpegNetEx::mpAVArray[MDEC_SOURCE_MAX_NUM] = {NULL,};
#endif
AM_INT CGDemuxerFFMpegNetEx::mAVLable = 0;
//AM_INT CGDemuxerFFMpegNetEx::mHDCur = -1;

AM_INT CGDemuxerFFMpegNetEx::ParseFile(const char* filename, CGConfig* pConfig)
{
    AM_INFO("CGDemuxerFFMpegNetEx::ParseFile\n");
    AM_INT i = 0, score = 0;
    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        if(mpAVArray[i] == NULL)
            break;
    }
    if(i == MDEC_SOURCE_MAX_NUM){
        score = -100;
        AM_INFO("CGDemuxerFFMpegNetEx ParseFile, Score:%d\n", score);
        return score;
    }
    //if(pConfig->globalFlag & USING_FOR_NET_PB)
        //return -50;
    if(pConfig->demuxerConfig[pConfig->curIndex].netused == AM_FALSE)
        return -50;

    mAVLable = i;
    AM_INFO("CGDemuxerFFMpegNetEx ParseFile, Score:%d\n", score);
#if NEW_RTSP_CLIENT
    char const* extension = strrchr(filename, '.');
    if (extension && strcmp(extension, ".livets") == 0) {
        //So far, MPEGTS use ffmpeg rtsp client
        mpAVArrayType[mAVLable] = 1;//ffmpeg rtsp client

        AVFormatContext* pAV = (AVFormatContext*)mpAVArray[mAVLable];
        av_register_all();
        int rval = av_open_input_file(&pAV, filename, NULL, 0, NULL);
        if (rval < 0)
        {
            AM_INFO("ffmpeg does not recognize %s\n", filename);
            //av_close_input_file(mpAVFormat);
            score = -10;
        }else{
            AM_INFO("CGDemuxerFFMpegNetEx: %s is [%s] format\n", filename, pAV->iformat->name);
            mpAVArray[mAVLable] = (void*)pAV;
            score = 99;
        }
    }else{
        mpAVArrayType[mAVLable] = 0;//AmfRtspClient
        mpAVArray[mAVLable] = AmfRtspClientManager::instance()->create_rtspclient(filename);
        if(mpAVArray[mAVLable]){
            score = 99;
        }else{
            score = -10;
        }
    }
#else
    AVFormatContext* pAV = mpAVArray[mAVLable];
    av_register_all();

    int rval = av_open_input_file(&pAV, filename, NULL, 0, NULL);
    if (rval < 0)
    {
        AM_INFO("ffmpeg does not recognize %s\n", filename);
        //av_close_input_file(mpAVFormat);
        score = -10;
    }else{
        AM_INFO("CGDemuxerFFMpegNetEx: %s is [%s] format\n", filename, pAV->iformat->name);
        mpAVArray[mAVLable] = pAV;
        score = 99;
    }
#endif
    AM_INFO("CGDemuxerFFMpegNetEx ParseFile, Score:%d\n", score);
    return score;
}

AM_ERR CGDemuxerFFMpegNetEx::ClearParse()
{
#if NEW_RTSP_CLIENT
    if (mpAVArray[mAVLable] != NULL){
        if(!mpAVArrayType[mAVLable]){
            AmfRtspClientManager::instance()->destroy_rtspclient((AmfRtspClient*)mpAVArray[mAVLable]);
        }else{
            av_close_input_file((AVFormatContext*)mpAVArray[mAVLable]);
        }
        mpAVArray[mAVLable] = NULL;
    }
#else
    //AM_INT lable = mAVLable -1;
    if (mpAVArray[mAVLable] != NULL)
    {
        av_close_input_file(mpAVArray[mAVLable]);
        mpAVArray[mAVLable] = NULL;
    }
#endif
    return ME_OK;
}

IGDemuxer* CGDemuxerFFMpegNetEx::Create(IFilter* pFilter, CGConfig* config)
{
    CGDemuxerFFMpegNetEx* result = new CGDemuxerFFMpegNetEx(pFilter, config);
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
AM_ERR CGDemuxerFFMpegNetEx::Construct()
{
    AM_INFO("CGDemuxerFFMpegNetEx Construct!\n");
    //set av first
    mAVIndex = mAVLable;
#if NEW_RTSP_CLIENT
    if(!mpAVArrayType[mAVLable]){
        m_rtsp_client = (AmfRtspClient*)mpAVArray[mAVLable];
    }else{
        mpAVFormat = (AVFormatContext*)mpAVArray[mAVLable];
        AM_ASSERT(mpAVFormat);
    }
#else
    mpAVFormat = mpAVArray[mAVLable];
    AM_ASSERT(mpAVFormat);
#endif
    //if failed during construct, reset av on delete
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    CQueue* mainQ = ((CActiveFilter* )mpFilter)->MsgQ();
    CQueue* mainQA = mpAudioMan->AudioMainQ();
    if ((mpBufferQV= CQueue::Create(mainQ, this, sizeof(CGBuffer), DEMUXER_BQUEUE_NUM_V)) == NULL){
        AM_ASSERT(0);
        return ME_NO_MEMORY;
    }

    if ((mpBufferQA= CQueue::Create(mainQA, this, sizeof(CGBuffer), DEMUXER_BQUEUE_NUM_A)) == NULL){
        AM_ASSERT(0);
        return ME_NO_MEMORY;
    }
    if ((mpRetrieveQ = CQueue::Create(NULL, this, sizeof(CGBuffer), DEMUXER_RETRIEVE_Q_NUM)) == NULL){
        AM_ASSERT(0);
        return ME_NO_MEMORY;
    }
    //default av config

    //AM_INFO("XXX");
    //mainQ->Dump();
    mpWorkQ->SetThreadPrio(1, 1);
    return ME_OK;
}

void CGDemuxerFFMpegNetEx::Delete()
{
    AM_INFO("CGDemuxerFFMpegNetEx::Delete().\n");
    //attach to delete;
    CQueue* mainQ = ((CActiveFilter* )mpFilter)->MsgQ();
    mpBufferQV->Attach(mainQ);
    CQueue* mainQA = mpAudioMan->AudioMainQ();
    mpBufferQA->Attach(mainQA);

    ClearQueue(mpBufferQV);
    ClearQueue(mpBufferQA);
    ClearQueue(mpRetrieveQ);
#if NEW_RTSP_CLIENT
    if(m_rtsp_client)
    {
        AmfRtspClientManager::instance()->destroy_rtspclient(m_rtsp_client);
        m_rtsp_client = NULL;
        mpAVArray[mAVIndex] = NULL;
        mpAVFormat = NULL;
    }else{
        //ffmpeg rtspclient
        if(mpAVFormat){
            av_close_input_file(mpAVFormat);
            mpAVFormat = NULL;
            mpAVArray[mAVIndex] = NULL;
        }
    }
#else
    if(mpAVFormat)
    {
        av_close_input_file(mpAVFormat);
        mpAVFormat = NULL;
        mpAVArray[mAVIndex] = NULL;
    }
#endif
    AM_DELETE(mpBufferQV);
    AM_DELETE(mpBufferQA);
    AM_DELETE(mpRetrieveQ);
    inherited::Delete();
}

void CGDemuxerFFMpegNetEx::ClearQueue(CQueue* queue)
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
        pthread_mutex_lock(&mutex);
        //release this packet
        if(buffer.GetStreamType() == STREAM_VIDEO){
            AM_ASSERT(mVideoConsumeCnt == buffer.GetCount());
            //if(mVideoConsumeCnt != buffer->GetCount())
            //AM_INFO("DIff(%d) : %d-->%d\n", mConfigIndex, mVideoConsumeCnt, buffer->GetCount());
            mVideoConsumeCnt++;
        }else{
            AM_ASSERT(mAudioConsumeCnt == buffer.GetCount());
            //if(mAudioConsumeCnt != buffer.GetCount())
                //AM_INFO("DIff(%d) : %d-->%d\n", mConfigIndex, mAudioConsumeCnt, buffer.GetCount());
            mAudioConsumeCnt++;
        }
        FinalDataProcess(&buffer);
        pthread_mutex_unlock(&mutex);
        continue;
    }
}

CGDemuxerFFMpegNetEx::~CGDemuxerFFMpegNetEx()
{
    AMLOG_DESTRUCTOR("~CGDemuxerFFMpegNetEx.\n");
    pthread_mutex_destroy(&mutex);
    AMLOG_DESTRUCTOR("~CGDemuxerFFMpegNetEx done.\n");
}

AM_ERR CGDemuxerFFMpegNetEx::PerformCmd(CMD& cmd, AM_BOOL isSend)
{
    AM_ERR err;
    if(isSend == AM_TRUE)
    {
        err = SendCmd(cmd.code);
    }else{
        err = PostCmd(cmd.code);
    }
    return err;
}

//CMD FROM GENERAL DEMUXER
AM_ERR CGDemuxerFFMpegNetEx::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGDemuxerFFMpegNetEx %d ::ProcessCmd %d\n ", mConfigIndex, cmd.code);
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
        err = DoSeek(par);
        CmdAck(err);
        break;

    case CMD_AUDIO:
        err = DoAudioCtrl();
        CmdAck(err);
        break;

    case CMD_BW_PLAYBACK_DIRECTION:
    case CMD_FW_PLAYBACK_DIRECTION:
        //ignore
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }

    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::DoAudioCtrl()
{
    AM_INFO("DoAudioCtrl\n");
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
        mAudioCount++;
        SendFilterMsg(GFilter::MSG_DEMUXER_A, 0);
        mbSelected = AM_TRUE;
        //set audio system
        //mpAudioMan->SetAudioPosition(mConfigIndex, QueryVideoPts());
    }
    mbEnAudio = !mpConfig->disableAudio;
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::DoEnableAV()
{
    mbEnAudio = !mpConfig->disableAudio && mpConfig->hasAudio;
    mbEnVideo = !mpConfig->disableVideo && mpConfig->hasVideo;
    return ME_OK;
}

//Imp interactive fun.
AM_ERR CGDemuxerFFMpegNetEx::DoConfig()
{
    if(mpConfig->needStart == AM_TRUE){
        //let's go!
        AM_ASSERT(mState == STATE_PENDING);
        AM_ASSERT(mpConfig->paused == AM_FALSE);
        mState = STATE_READ_DATA;
        //flush for net stream
        AM_INFO("ff_read_frame_flush\n");
#if NEW_RTSP_CLIENT
        if(m_rtsp_client){
            m_rtsp_client->flush_queue();
        }else{
            ff_read_frame_flush(mpAVFormat);
        }
#else
        ff_read_frame_flush(mpAVFormat);
#endif
        mbEnAudio = !mpConfig->disableAudio && mpConfig->hasAudio;
        mbEnVideo = !mpConfig->disableVideo && mpConfig->hasVideo;
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

AM_ERR CGDemuxerFFMpegNetEx::DoFlush()
{
    AM_INFO("CGDemuxerFFMpegNetEx %d Flush All Data:", mConfigIndex);
    AM_INFO("   Video Cnt: %d, Consume Video Cnt:%d, SentVideo Cnt:%d, TotalVideo Cnt:%d\n",
        mpBufferQV->GetDataCnt(), mVideoConsumeCnt, mVideoSentCnt, mVideoCount);
    AM_INFO("CGDemuxerFFMpegExQueue %d Flush All Data:", mConfigIndex);
    AM_INFO("   Audio Cnt: %d, Consume Audio Cnt:%d, SentAudio Cnt:%d, TotalAudio Cnt:%d\n",
        mpBufferQA->GetDataCnt(), mAudioConsumeCnt, mAudioSentCnt, mAudioCount);
    ClearQueue(mpBufferQV);
    //mVideoConsumeCnt += mpBufferQV->GetDataCnt();
    //mVideoSentCnt += mpBufferQV->GetDataCnt();
    //AM_ASSERT(mVideoSentCnt == mVideoConsumeCnt);
    AM_ASSERT(mVideoCount == (mVideoConsumeCnt -1));//mVideoCnt no INCLUDE handle buffer
    ClearQueue(mpBufferQA);
    AM_ASSERT(mAudioCount == mAudioConsumeCnt);

    mbRevived = AM_TRUE;
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::DoStop()
{
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::DoHide(AM_BOOL hided)
{
    AM_INFO("Demuxer-Net-Queue DoHide(%d, %d)\n", mConfigIndex, hided);
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
AM_ERR CGDemuxerFFMpegNetEx::DoPause()
{
    if(mState == STATE_PAUSE)
        return ME_OK;
//    AM_INT ret;
    /*const char* name = mpAVFormat->iformat->name;
    if(strcmp(name, "rtsp") == 0)
    {
        ret = av_read_pause(mpAVFormat);
        if(ret != 0)
            AM_INFO("rtsp pause fail!\n");
    }
    */
    mState = STATE_PAUSE;
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::DoResume()
{
//    AM_INT ret;
    /*const char* name = mpAVFormat->iformat->name;
    if(strcmp(name, "rtsp") == 0)
    {
        ret = av_read_play(mpAVFormat);
        if(ret != 0)
            AM_INFO("rtsp resume fail!\n");
    }
    */
    //AM_ASSERT(mState == STATE_PAUSE);
    mState = STATE_READ_DATA;
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::DoSeekCheck()
{
    AM_INFO("DoSeekCheck %lld(%lld) for Stream:%d\n", mSeekCheck, mSeekCheck / 3003, mConfigIndex);
#if NEW_RTSP_CLIENT
    AM_WARNING("CGDemuxerFFMpegNetEx::DoSeekCheck not implemented yet!\n");
    return ME_OK;
#else
    AM_INT ret;
    AM_ERR err = ME_OK;
    while(1)
    {
        av_init_packet(&mPkt);
        ret = av_read_frame(mpAVFormat, &mPkt);
        if(ret < 0)
        {
            if(ret == AVERROR(EAGAIN)){
                av_free_packet(&mPkt);
                continue;//how to impl ME_TRYNEXT?
            }
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

        if(mSeekCheck != -1)
        {
            if(mPkt.pts > mSeekCheck){
                AM_INFO("Adjuct Demuxer Seek to: %lld, cur:%lld\n", mSeekCheck, mPkt.pts);
                mLastSeek -= SEEK_GAP;
                err = DoSeek(mLastSeek);
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
                if((mSeekCheck - mPkt.pts) >= FRAME_TIME_DIFF(100)){
                    AM_INFO("Too Long Diff , Seek again %lld!\n", mPkt.pts);
                    av_free_packet(&mPkt);
                    mLastSeek += FRAME_TIME_DIFF(100);
                    err = DoSeek(mLastSeek);
                    AM_ASSERT(err == ME_OK);
                    continue;
                }
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
    av_free_packet(&mPkt);
    av_init_packet(&mPkt);
    mDebug = AM_TRUE;
    return ME_OK;
#endif
}

AM_ERR CGDemuxerFFMpegNetEx::DoSeek(AM_U64 target)
{
#if NEW_RTSP_CLIENT
    AM_WARNING("CGDemuxerFFMpegNetEx::DoSeek not implemented yet!\n");
    return ME_OK;
#else
    AM_INT ret = 0;
    AM_U64 targeByte;
    AM_U64 timeMs;

    if((ret = av_seek_frame(mpAVFormat, -1, target, 0)) < 0)
    {
        timeMs = target * 1000 * (mpAVFormat->streams[mVideo]->time_base.num) / (mpAVFormat->streams[mVideo]->time_base.den);
        targeByte = target * mpAVFormat->file_size /mpAVFormat->duration;
        AM_INFO("seek by timestamp(%lld) fail, ret =%d, try by bytes(%lld), timeMs(%llu)--duration:%lld.\n", target, ret, targeByte, timeMs, mpAVFormat->duration);
        if((ret = av_seek_frame(mpAVFormat, -1, target, AVSEEK_FLAG_BYTE )) < 0) {
            AM_ERROR("seek by bytes fail, ret =%d, Seek return error.\n", ret);
            return ME_ERROR;
        }
    }
    mLastSeek = target;
#endif
    return ME_OK;
}

void CGDemuxerFFMpegNetEx::OnRun()
{
    AM_ERR err;
    CMD cmd;
//    CQueue::QType type;
//    CQueue::WaitResult result;
//    AM_INT ret = 0;

    DoEnableAV();
    AM_INFO("CGDemuxerFFMpegNetEx OnRun Info: enA:%d, enV:%d\n", mbEnAudio, mbEnVideo);
    FillAudioHandleBuffer();
    FillHandleBuffer();
    mbRun = true;

#if NEW_RTSP_CLIENT
    AM_INFO("CGDemuxerFFMpegNetEx Use AMF RTSP cliet```````````````````\n");
    if(m_rtsp_client){
        m_rtsp_client->play();
    }
#endif

    CmdAck(ME_OK);
    mState = STATE_PENDING;
    while(mbRun)
    {
        //AM_INFO("D%d, mState:%d, mbEna %d, v%d\n", mConfigIndex, mState, mbEnAudio, mbEnVideo);
        //if(mConfigIndex == 0)
            //Dump();
        if(NeedGoPending() == AM_TRUE)
            mState = STATE_PENDING;

        switch(mState)
        {
        case STATE_READ_DATA:
            if(mpWorkQ->PeekCmd(cmd))
            {
                ProcessCmd(cmd);
            }else{
                err = FillBufferToQueue();
                if(err != ME_OK && err != ME_TRYNEXT)
                    mState = STATE_PENDING;
            }
            break;

        case STATE_PENDING:
        case STATE_PAUSE:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        case STATE_HIDED:
            //still read data
            if(mpWorkQ->PeekCmd(cmd))
            {
                ProcessCmd(cmd);
            }else{
                err = FillOnlyReceive();
                AM_ASSERT(err == ME_OK || err == ME_TRYNEXT);
            }
            break;

        default:
            AM_ERROR("Check Me %d.\n", mConfigIndex);
            break;
        }
    }
    AM_INFO("CGDemuxerFFMpegNetEx OnRun Exit.\n");
}

inline AM_BOOL CGDemuxerFFMpegNetEx::NeedGoPending()
{
    if(mState == STATE_PAUSE || mState == STATE_HIDED)
        return AM_FALSE;

    if(mbEOS == AM_TRUE)
        return AM_TRUE;

    if(mpBufferQV->GetDataCnt() >= DEMUXER_BQUEUE_NUM_V)
    {
        //AM_INFO("Warning::!! Demuxer too fast for net stream playback!\n");
        return AM_TRUE;
    }
    return AM_FALSE;
}

//Fill audio handler buffer at the beginning
inline AM_ERR CGDemuxerFFMpegNetEx::FillAudioHandleBuffer()
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
        mAudioCount++;
        SendFilterMsg(GFilter::MSG_DEMUXER_A, 0);
    }
    return ME_OK;
}

//time diff -test this
inline AM_ERR CGDemuxerFFMpegNetEx::FillHandleBuffer(AM_BOOL stillGoOn)
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
                mVideoCount--;
                return ME_OK;
            }
        }
    }

    mpBufferQV->PutData(&mVHandleBuffer, sizeof(CGBuffer));
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::FillBufferToQueue()
{
    CGBuffer gBuffer;
    AM_ERR ret;
    if(mpBufferQV->GetDataCnt() >= DEMUXER_BQUEUE_NUM_V ||mpBufferQA->GetDataCnt() >= DEMUXER_BQUEUE_NUM_A)
    {
        AM_INFO("Debug. V %d, A %d\n",mpBufferQV->GetDataCnt(),mpBufferQA->GetDataCnt());
    }
    ret = ReadData(gBuffer) ;
    if(ret == ME_TRYNEXT){
        return ME_TRYNEXT;
    }

#if PLATFORM_LINUX
    AVPacket* pkt = (AVPacket *)gBuffer.GetExtraPtr();
    mpGMuxer->SendToInjector(mConfigIndex, &gBuffer,pkt->pts);
#endif

    //handle audio buffer
    if(gBuffer.GetStreamType() == STREAM_AUDIO && mbEnAudio == AM_FALSE){
        FinalDataProcess(&gBuffer);
        return ME_OK;
    }

    //release to IDR frame, revived after flush
    if(mbRevived == AM_TRUE && (mpGConfig->globalFlag & BUFFER_TO_IDR_OR_NOT_JUST_TEST_FOR_DEMUXER))
    {
        AVPacket* packet = (AVPacket*)(gBuffer.GetExtraPtr());
        if((packet->flags & AV_PKT_FLAG_KEY) && packet->stream_index == mVideo){
            mbRevived = AM_FALSE;
            AM_INFO("Skip %d frames to IDR frame on stream%d.\n", mReleaseRevive, mConfigIndex);
            mReleaseRevive = 0;
        }else{
            mReleaseRevive++;
            FinalDataProcess(&gBuffer);
            return ME_OK;
        }
    }

    STREAM_TYPE type = gBuffer.GetStreamType();
    if(type == STREAM_VIDEO){
        if(mVideoCount == 0)
            mFirstPts = gBuffer.GetPTS();
        mVideoCount++;
        gBuffer.SetCount(mVideoCount);
        mpBufferQV->PutData(&gBuffer, sizeof(CGBuffer));
    }else if(type == STREAM_AUDIO){
        gBuffer.SetCount(mAudioCount);
        mAudioCount++;
        mpBufferQA->PutData(&gBuffer, sizeof(CGBuffer));
    }

    if(gBuffer.GetBufferType() == EOS_BUFFER)
    {
        mVideoCount++;
        gBuffer.SetCount(mVideoCount);
        gBuffer.SetStreamType(STREAM_VIDEO);
        mpBufferQV->PutData(&gBuffer, sizeof(CGBuffer));
        mbEOS = AM_TRUE;
        AM_INFO("Stream %d demuxer EOS reached:%d\n", mConfigIndex, mVideoCount);
    }

    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::FillOnlyReceive()
{
    CGBuffer gBuffer;
    AM_ERR ret;
    if(mpBufferQV->GetDataCnt() > 0)
    {
        ClearQueue(mpBufferQV);
    }
    if(mpBufferQA->GetDataCnt() > 0)
    {
        ClearQueue(mpBufferQA);
    }

    ret = ReadData(gBuffer) ;
    if(ret == ME_TRYNEXT){
        return ME_TRYNEXT;
    }
    FinalDataProcess(&gBuffer);
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::ReadData(CGBuffer& buffer)
{
    AM_INT ret;
    AM_U64 curPts = 0;

    //ME_TRYNEXT will be return if data is not available or  expected.
    //   so that Cmd can be processed.
    while(1)
    {
#if NEW_RTSP_CLIENT
        if(m_rtsp_client){
            ret = m_rtsp_client->read_avframe(&mPkt);
        }else{
            ret = av_read_frame(mpAVFormat, &mPkt);
        }
        //AM_INFO("CGDemuxerFFMpegNetEx::ReadData() --- read_avframe[%d], index = %d, pts = %lld\n",ret,mPkt.stream_index,mPkt.pts);
#else
        ret = av_read_frame(mpAVFormat, &mPkt);
#endif
        if(ret < 0)
        {
            if(ret == AVERROR(EAGAIN)){
                av_free_packet(&mPkt);
                return ME_TRYNEXT;
            }
            AM_INFO("av_read_frame()<0, stream[%d] end? ret=%d.\n", mConfigIndex, ret);
            FillEOS(buffer);
            mbEOS = AM_TRUE;
            av_free_packet(&mPkt);
            break;
        }

        if(mPkt.stream_index != mVideo && mPkt.stream_index != mAudio)
        {
            av_free_packet(&mPkt);
            return ME_TRYNEXT;
        }

        if(mPkt.pts < 0 && mPkt.dts < 0){
            AM_INFO("invaild pts 0x%llx, invaild dts 0x%llx, free this packet\n", mPkt.pts, mPkt.dts);
            av_free_packet(&mPkt);
            return ME_TRYNEXT;
        }
        //if(mPkt.stream_index == mAudio && mConfigIndex == 1)
            //AM_INFO("Show For Audio:%d\n", mPkt.size);

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
            buffer.mFrameType = (mPkt.flags & AV_PKT_FLAG_KEY)? PredefinedPictureType_IDR : 0;
        }else if(mPkt.stream_index == mAudio){
            curPts = ConvertAudioPts(mPkt.pts);
            buffer.SetStreamType(STREAM_AUDIO);
        }
        buffer.SetPTS(curPts);
        av_init_packet(&mPkt);
        break;
    }
    StatVideoFrame(&buffer);
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::FillEOS(CGBuffer& buffer)
{
    //AM_INFO("Fill EOS To Buffer. Stream: %d\n", mConfigIndex);
    buffer.SetBufferType(EOS_BUFFER);
    buffer.SetOwnerType(DEMUXER_FFMPEG);
    buffer.SetIdentity(mConfigIndex);
    buffer.SetStreamType(STREAM_NULL);
    return ME_OK;
}

AM_BOOL CGDemuxerFFMpegNetEx::GetBufferPolicy(CGBuffer& gBuffer)
{
    return mpBufferQV->PeekData(&gBuffer, sizeof(CGBuffer));
}

AM_ERR CGDemuxerFFMpegNetEx::GetAudioBuffer(CGBuffer& oBuffer)//a-v spe
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

AM_ERR CGDemuxerFFMpegNetEx::GetCGBuffer(CGBuffer& buffer)
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

inline AM_ERR CGDemuxerFFMpegNetEx::ContinueRun()
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

inline AM_BOOL CGDemuxerFFMpegNetEx::NeedSendAudio(CGBuffer* buffer)
{
    AM_ASSERT(0);
    return false;
}

AM_ERR CGDemuxerFFMpegNetEx::OnReleaseBuffer(CGBuffer* buffer)
{
    pthread_mutex_lock(&mutex);
    //CGBuffer dumpBuffer = *buffer;
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

    //1 todo check buffer cnt on net condition
    if(mpBufferQV->GetDataCnt() <= (DEMUXER_BQUEUE_NUM_V /3))
    {
        ContinueRun();//inline this function.
    }
    if(mpBufferQA->GetDataCnt() <= 0)
    {
        ContinueRun();
    }

    FinalDataProcess(buffer);
    //AM_INFO(" CGDemuxerFFMpegNetEx OnReleaseBuffer Done!!\n");
    pthread_mutex_unlock(&mutex);
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::OnRetrieveBuffer(CGBuffer* buffer)
{
    AM_ASSERT(0);
    return ME_OK;
}

inline AM_ERR CGDemuxerFFMpegNetEx::FinalDataProcess(CGBuffer* buffer)
{
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
AM_ERR CGDemuxerFFMpegNetEx::PostFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    msg.p1 = mConfigIndex;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->PostMsgToFilter(msg);
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::SendFilterMsg(AM_UINT code, AM_INTPTR ptr)
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
AM_ERR CGDemuxerFFMpegNetEx::LoadFile()
{
    AM_ERR err;
#if NEW_RTSP_CLIENT
    if(m_rtsp_client){
        int rval = m_rtsp_client->start();
        if (rval < 0)
        {
            AM_ERROR("av_find_stream_info failed\n");
            return ME_ERROR;
        }
        mpAVFormat = m_rtsp_client->get_avformat();
    }else{
        //ffmpeg rtsp client
        int rval = av_find_stream_info(mpAVFormat);
        if (rval < 0)
        {
            AM_ERROR("av_find_stream_info failed\n");
            return ME_ERROR;
        }
    }
#else
    int rval = av_find_stream_info(mpAVFormat);
    if (rval < 0)
    {
        AM_ERROR("av_find_stream_info failed\n");
        return ME_ERROR;
    }
#endif
//#ifdef AM_DEBUG
    av_dump_format(mpAVFormat, 0, mpAVFormat->filename, 0);
//#endif

    // find video stream & audio stream
    mVideo = FindMediaStream(AVMEDIA_TYPE_VIDEO);
    mAudio = FindMediaStream(AVMEDIA_TYPE_AUDIO);
    if (mVideo < 0 && mAudio < 0)
    {
        AM_ERROR("No video, no audio, Check me!\n");
        return ME_ERROR;
    }
    AM_INFO("CGDemuxerFFMpegNetEx: FindMediaStream video[%d]audio[%d].\n", mVideo, mAudio);

    if (mVideo >= 0)
    {
        err = SetFormat(mVideo, AVMEDIA_TYPE_VIDEO);
        if(err == ME_BAD_FORMAT)
            return err;
        if (err != ME_OK) {
            AM_ERROR("InitStream video stream fail, disable it.\n");
            mpConfig->disableVideo = true;
            mpConfig->hasVideo = false;
            mbEnVideo = false;
        }
    }else{
        //disagree for novideo
        mpConfig->disableVideo = true;
        mpConfig->hasVideo = false;
        mbEnVideo = false;
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
            mbEnAudio = false;
        }
    }else{
        mpConfig->disableAudio = true;
        mpConfig->hasAudio = false;
        mbEnAudio = false;
    }

    mbEnAudio = !mpConfig->disableAudio;
    mbEnVideo = !mpConfig->disableVideo;

    UpdatePTSConvertor();
    //OR mVHandleBuffer.ffmpegStream = stream; think about this.
    CParam envPar(2);
    mpGConfig->DemuxerSetDecoderEnv(mConfigIndex, (void* )mpAVFormat, envPar);

    mpAudioMan->AddAudioSource(mConfigIndex, mAudio);
    //Set Muxer info here
    SetupMuxerInfo();

    if((mVideoWidth == 0 ||mVideoHeight == 0) && mbEnVideo){
        mVideoWidth = mpAVFormat->streams[mVideo]->codec->width;
        mVideoHeight = mpAVFormat->streams[mVideo]->codec->height;
        if(mVideoHeight==1088) mVideoHeight = 1080;
    }
    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::SetupMuxerInfo()
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

int CGDemuxerFFMpegNetEx::hasCodecParameters(AVCodecContext* enc)
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
            AM_INFO("Find Audio, sample_rate, channels, smaple_fmt:%d, %d, %d, val%d\n", enc->sample_rate, enc->channels, enc->sample_fmt, val);
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
        AM_INFO("Reject here\n");
        val = 0;
    }

    return val;
}

AM_INT CGDemuxerFFMpegNetEx::FindMediaStream(AM_INT media)
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

void CGDemuxerFFMpegNetEx::UpdatePTSConvertor()
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

AM_U64 CGDemuxerFFMpegNetEx::ConvertVideoPts(am_pts_t pts)
{
    pts *= mPTSVideo_Num;
    if (mPTSVideo_Den != 1) {
        pts /=  mPTSVideo_Den;
    }
    mLastPts = pts;
    return pts;
}

AM_U64 CGDemuxerFFMpegNetEx::ConvertAudioPts(am_pts_t pts)
{
    pts *= mPTSAudio_Num;
    if (mPTSAudio_Den != 1) {
        pts /=  mPTSAudio_Den;
    }
    mLastAudioPts = pts;
    return pts;
}
//
//
//===========================================================//
void CGDemuxerFFMpegNetEx::Test(const char* info)
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
int CGDemuxerFFMpegNetEx::Test2(AM_INT i)
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

#if 1
AM_ERR CGDemuxerFFMpegNetEx::QueryInfo(AM_INT type, CParam64& param)
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
        break;

    default:
        AM_ERROR("Demuxer(%d) Query Wrong Info Type.\n", mConfigIndex);
    };

    return ME_OK;
}

AM_ERR CGDemuxerFFMpegNetEx::SeekTo(AM_U64 timestamp, AM_INT flag)
{
    //ms is based on CLOCKTIMEBASEDEN
    AM_ASSERT(0);
    AM_ASSERT(AV_TIME_BASE == 1000000);
    AM_U64 ffTimestamp = timestamp * mPTSVideo_Den / mPTSVideo_Num;
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
    cmd.res64_1 = ffTimestamp;
    err = (MsgQ())->SendMsg(&cmd, sizeof(CMD));

    if(mSeekCheck != -1)
    {
        return DoSeekCheck();
    }
    return err;
}
#else
AM_ERR CGDemuxerFFMpegNetEx::Seek(AM_U64 ms)
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


AM_ERR CGDemuxerFFMpegNetEx::GetTotalLength(AM_U64& ms)
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
AM_ERR CGDemuxerFFMpegNetEx::SetFormat(int stream, int media)
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
                mVHandleBuffer.codecType = &GUID_AmbaVideoDecoder;
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

inline AM_INT CGDemuxerFFMpegNetEx::FRAME_TIME_DIFF(AM_INT diff)
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

inline AM_ERR CGDemuxerFFMpegNetEx::StatVideoFrame(CGBuffer* gBuffer)
{
    //static AM_INT num = 0;
    return ME_OK;
    if(gBuffer->GetStreamType() != STREAM_VIDEO)
        return ME_OK;

    AVPacket* packet = (AVPacket*)(gBuffer->GetExtraPtr());
    AM_INT oldFrameNum = mFrameStat.FrameEachGop;
    //mFrameStat.FrameEachGop++;
    if(packet->flags & AV_PKT_FLAG_KEY){
        if(oldFrameNum > 60){
            AM_INFO("Stream %d Miss I Frame on Gop%d, Two Gop Frame Num:%d.\n", mConfigIndex, mFrameStat.GopNum, oldFrameNum);
            mFrameStat.InsertElem(oldFrameNum, mFrameStat.GopNum);
        }
        if(oldFrameNum < 60 && oldFrameNum != 0){
            AM_INFO("Stream %d Miss Frame on Gop%d, Miss Frame Num:%d.\n", mConfigIndex, mFrameStat.GopNum, 60 -oldFrameNum);
            mFrameStat.InsertElem(oldFrameNum, mFrameStat.GopNum);
        }

        mFrameStat.GopNum++;
        mFrameStat.FrameEachGop = 1;

    }else{
        mFrameStat.FrameEachGop++;
    }
    mFrameStat.PtsDiff = gBuffer->GetPTS() - mFrameStat.PtsLast;
    mFrameStat.PtsLast = gBuffer->GetPTS();
    if(mFrameStat.GopNum < 3){
        //AM_INFO("Stream %d Debug PTS: %lld\n", mConfigIndex, mFrameStat.PtsLast);
        //num++;
    }
    //if(mFrameStat.PtsDiff > 3003 *5)
        //AM_INFO("Stream %d Pts Diff: %d(Gop%d)\n", mConfigIndex, mFrameStat.PtsDiff, mFrameStat.GopNum);
    return ME_OK;
}

void CGDemuxerFFMpegNetEx::Dump(AM_INT flag)
{
    AM_INFO("       {---FFMpeg Demuxer For Net(%d, %p)---}\n",  mConfigIndex, this);
    if(flag & DUMP_FLAG_FFMPEG){
       // av_dump(mpAVFormat);
        return;
    }
    AM_INFO("           Config->en audio:%d, en video:%d, FirstPts Filled:%lld\n", mbEnAudio, mbEnVideo, mFirstPts);
    AM_INFO("           State->%d, paused:%d, Hided:%d, needStart:%d.\n", mState, mpConfig->paused, mpConfig->hided, mpConfig->needStart);
    AM_INFO("           BufferPool->Audio Bp(%p) Cnt:%d, Video Bp Cnt:%d, Retrieve Q Cnt:%d\n", mpBufferQA,
        mpBufferQA->GetDataCnt(),  mpBufferQV->GetDataCnt(), mpRetrieveQ->GetDataCnt());
    /*AM_INFO("           Frame Stat(Press '-d framestat' To Dump All)->Gop Num:%d, Miss Gop Num:%d (%d-%d, %d-%d, %d-%d...)\n",
        mFrameStat.GopNum, mFrameStat.ElemNum, mFrameStat.data[0].Frame, mFrameStat.data[0].Gop,
        mFrameStat.data[1].Frame, mFrameStat.data[1].Gop, mFrameStat.data[2].Frame, mFrameStat.data[2].Gop);
    */
}




