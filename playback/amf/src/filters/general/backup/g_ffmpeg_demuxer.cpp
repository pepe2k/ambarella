
/*
 * g_ffmpeg_demuxer.cpp
 *
 * History:
 *    2012/3/27 - [QingXiong Z] created file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "g_ffmpeg_demux"
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

#include "g_ffmpeg_demuxer.h"

GDemuxerParser gFFMpegDemuxer = {
    "FFMpegDemuxer-G",
    CGFFMpegDemuxer::Create,
    CGFFMpegDemuxer::ParseFile,
    CGFFMpegDemuxer::ClearParse,
};

//todo take care when cbuffer(bp) reasele
//-----------------------------------------------------------------------
//
// CGFFMpegDemuxer
//
//-----------------------------------------------------------------------
//accept this file or not.
AVFormatContext* CGFFMpegDemuxer::mpAVArray[MDEC_SOURCE_MAX_NUM] = {NULL,};
AM_INT CGFFMpegDemuxer::mAVLable = 0;

AM_INT CGFFMpegDemuxer::ParseFile(const char* filename, CGConfig* pConfig)
{
    AM_INFO("CGFFMpegDemuxer::ParseFile\n");
    AM_INT i = 0, score = 0;
    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        if(mpAVArray[i] == NULL)
            break;
    }
    if(i == MDEC_SOURCE_MAX_NUM){
        score = -100;
        AM_INFO("CGFFMpegDemuxer ParseFile, Score:%d\n", score);
        return score;
    }
    if(!(pConfig->globalFlag & USING_FOR_NET_PB))
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
        AM_INFO("CGFFMpegDemuxer: %s is [%s] format\n", filename, pAV->iformat->name);
        mpAVArray[mAVLable] = pAV;
        score = 90;
    }

    AM_INFO("CGFFMpegDemuxer ParseFile, Score:%d\n", score);
    return score;
}

AM_ERR CGFFMpegDemuxer::ClearParse()
{
    //AM_INT lable = mAVLable -1;
    if (mpAVArray[mAVLable] != NULL)
    {
        av_close_input_file(mpAVArray[mAVLable]);
        mpAVArray[mAVLable] = NULL;
    }
    return ME_OK;
}

IGDemuxer* CGFFMpegDemuxer::Create(IFilter* pFilter, CGConfig* config)
{
    CGFFMpegDemuxer* result = new CGFFMpegDemuxer(pFilter, config);
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
AM_ERR CGFFMpegDemuxer::Construct()
{
    AM_INFO("CGFFMpegDemuxer Construct!\n");
    //set av first
    mAVIndex = mAVLable;
    mpAVFormat = mpAVArray[mAVLable];
    AM_ASSERT(mpAVFormat);
    //if failed during construct, reset av on delete
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    CQueue* mainQ = ((CActiveFilter* )mpFilter)->MsgQ();
    if ((mpBufferQV= CQueue::Create(mainQ, this, sizeof(CGBuffer), DEMUXER_BQUEUE_NUM_V)) == NULL)
        return ME_NO_MEMORY;
    if ((mpBufferQA= CQueue::Create(NULL, this, sizeof(CGBuffer), DEMUXER_BQUEUE_NUM_A)) == NULL)
        return ME_NO_MEMORY;

    if ((mpRetrieveQ = CQueue::Create(NULL, this, sizeof(CGBuffer), DEMUXER_RETRIEVE_Q_NUM)) == NULL)
        return ME_NO_MEMORY;
    //default av config
    if(mpConfig->disableAudio == AM_FALSE)
        mbEnAudio = true;
    if(mpConfig->disableVideo == AM_TRUE)
        mbEnVideo = false;


    mpWorkQ->SetThreadPrio(1, 1);
    return ME_OK;
}

void CGFFMpegDemuxer::Delete()
{
    AM_INFO("CGFFMpegDemuxer::Delete().\n");
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
    inherited::Delete();
}

void CGFFMpegDemuxer::ClearQueue(CQueue* queue)
{
    AM_BOOL rval;
    CGBuffer buffer;
    while(1)
    {
        if(queue == mpBufferQV){
            rval = queue->PeekData(&buffer, sizeof(CGBuffer));
        }else{
            rval = queue->PeekMsg(&buffer, sizeof(CGBuffer));
        }
        if(rval == AM_FALSE)
        {
            break;
        }
        //release this packet
        AVPacket* pkt = (AVPacket* )(buffer.GetExtraPtr());
        av_free_packet(pkt);
        delete pkt;
        continue;
    }
}

CGFFMpegDemuxer::~CGFFMpegDemuxer()
{
    AMLOG_DESTRUCTOR("~CGFFMpegDemuxer.\n");
    AMLOG_DESTRUCTOR("~CGFFMpegDemuxer done.\n");
}

AM_ERR CGFFMpegDemuxer::PerformCmd(CMD& cmd, AM_BOOL isSend)
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
AM_ERR CGFFMpegDemuxer::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGFFMpegDemuxer::ProcessCmd %d\n ", cmd.code);
    AM_ERR err;
    switch (cmd.code)
    {
    case CMD_STOP:
        DoStop();
        mbRun = false;
        CmdAck(ME_OK);
        break;

    case CMD_PAUSE:
        //DoPause();
        CmdAck(ME_OK);
        break;

    case CMD_RESUME:
        //DoResume();
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
            mState = STATE_IDLE;
        }
        //CmdAck(ME_OK);
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }

    return ME_OK;
}

AM_ERR CGFFMpegDemuxer::DoConfig()
{
    //process pause/resume
    if(mpConfig->paused == AM_TRUE){
        //mState = STATE_PAUSE;
        DoPause();
    }else{
        if(mState == STATE_PAUSE)
            DoResume();
    }

    //mbOnlyReceive = mpConfig->doRecieve;
    if(mbOnlyReceive == AM_TRUE){
        AM_ASSERT(mpConfig->disableAudio == AM_TRUE);
        AM_ASSERT(mpConfig->disableVideo == AM_TRUE);
    }
    //process disable a/v
    bool saveA = mbEnAudio, saveV = mbEnVideo;
    bool enA = true, enV = true;
    //bool continue_need = false;
    if(mpConfig->disableAudio == AM_TRUE)
        enA = false;
    if(mpConfig->disableVideo == AM_TRUE)
        enV = false;

    mbEnAudio = (mAudio != -1) ? enA : false;
    mbEnVideo = (mVideo != -1) ? enV : false;
    if(mbEnAudio == AM_FALSE && mpConfig->disableAudio == AM_FALSE){
        mpConfig->disableAudio = AM_TRUE;
    }
    //do other things;
    //you can let me wait here
    if(saveA == false && saveV == false){
        if(mbEnAudio || mbEnVideo){
            ContinueRun();
        }
    }
    if(saveA == true && mbEnAudio == false){
        ClearQueue(mpBufferQA);
        //mAudioConsumeCnt = mAudioSentCnt = mAudioCount = 0;
        mbAHandleSent = AM_FALSE;
    }

    if(saveA == false && mbEnAudio == true && mbEOS != true){
        mAudioConsumeCnt = mAudioSentCnt = mAudioCount = 0;
        if(mpConfig->sendHandleA == true){
            AM_INFO("SOURCE %d Audio Selected!\n", mConfigIndex);
            mAHandleBuffer.SetOwnerType(DEMUXER_FFMPEG);
            mAHandleBuffer.SetBufferType(HANDLE_BUFFER);
            mAHandleBuffer.SetStreamType(STREAM_AUDIO);
            mAHandleBuffer.SetIdentity(mConfigIndex);
            mAHandleBuffer.SetCount(0);
            mpBufferQA->PostMsg(&mAHandleBuffer, sizeof(CGBuffer));
            mbAHandleSent = AM_TRUE;
            //SendFilterMsg(GFilter::MSG_DEMUXER_A, NULL);
            mbSelected = AM_TRUE;
        }
    }

    return ME_OK;
}

AM_ERR CGFFMpegDemuxer::DoFlush()
{
    //need no during g flush;

    return ME_OK;
}

AM_ERR CGFFMpegDemuxer::DoStop()
{
    return ME_OK;
}

//do something if using a network-based format:RTSP
AM_ERR CGFFMpegDemuxer::DoPause()
{
    if(mState == STATE_PAUSE)
        return ME_OK;
    AM_INT ret;
    const char* name = mpAVFormat->iformat->name;
    if(strcmp(name, "rtsp") == 0)
    {
        ret = av_read_pause(mpAVFormat);
        if(ret != 0)
            AM_INFO("rtsp pause fail!\n");
    }

    //Explain: this num is exact for pause cnt.
    //though GetCGBuffer() will affect this Cnt, but for engine flow:
    //pasue GDM
    //config me
    //resume GDM (now even GDM call GetCGBuffer(), the state is paused.)
    //key point is pause GDM first, and then config me pause.
    mpConfig->pausedCnt = mVideoSentCnt;
    if(mVideoConsumeCnt == mpConfig->pausedCnt)
        mpConfig->pausedCnt = 0;
    mState = STATE_PAUSE;
    return ME_OK;
}

AM_ERR CGFFMpegDemuxer::DoResume()
{
    AM_INT ret;
    const char* name = mpAVFormat->iformat->name;
    if(strcmp(name, "rtsp") == 0)
    {
        ret = av_read_play(mpAVFormat);
        if(ret != 0)
            AM_INFO("rtsp resume fail!\n");
    }
    AM_ASSERT(mState == STATE_PAUSE);
    mState = STATE_IDLE;
    return ME_OK;
}

void CGFFMpegDemuxer::OnRun()
{
    AM_ERR err;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    AM_INT ret = 0;

    AM_INFO("CGFFMpegDemuxer OnRun Info: enA:%d, enV:%d\n", mbEnAudio, mbEnVideo);
    FillHandleBuffer();
    mbRun = true;
    CmdAck(ME_OK);
    mState = STATE_IDLE;
    while(mbRun)
    {
        //AM_INFO("D4, mState:%d, mbEna %d, v%d\n",mState, mbEnAudio, mbEnVideo);
        if(NeedGoPending() == AM_TRUE)
            mState = STATE_PENDING;

        switch(mState)
        {
        case STATE_IDLE:
            if(mpWorkQ->PeekCmd(cmd))
            {
                ProcessCmd(cmd);
            }else{
                err = FillBufferToQueue();
                if(err != ME_OK)
                    mState = STATE_PENDING;
            }
            break;

        case STATE_PENDING:
        case STATE_PAUSE:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        default:
            AM_ERROR("Check Me.\n");
            break;
        }
    }
    AM_INFO("GFFMpegDemuxer OnRun Exit.\n");
}

inline AM_BOOL CGFFMpegDemuxer::NeedGoPending()
{
    if(mbOnlyReceive == AM_TRUE)
    {
        return false;
    }
    if(mState == STATE_PAUSE)
        return false;
    if(mbEOS == AM_TRUE)
        return true;

    //Select performance support
    if(mbSelected == true){
        if(mpBufferQA->GetDataCnt() <= (DEMUXER_BQUEUE_NUM_A/2))
            return false;
        SendFilterMsg(GFilter::MSG_DEMUXER_A, 0);
        mbSelected = false;
    }

    if(mbEnAudio == false && mbEnVideo == false)
    {
        return true;
    }

    if(mpBufferQV->GetDataCnt() >= DEMUXER_BQUEUE_NUM_V)
    {
        if(!mbEnAudio)
            return true;

        if(mNoPending == true){
            if(mpBufferQA->GetDataCnt() >= (DEMUXER_BQUEUE_NUM_A /2))
                mNoPending = false;
            return false;
        }else{
            return true;
        }
    }
    return false;
}

//time diff -test this
inline AM_ERR CGFFMpegDemuxer::FillHandleBuffer()
{
    mVHandleBuffer.SetOwnerType(DEMUXER_FFMPEG);
    mVHandleBuffer.SetBufferType(HANDLE_BUFFER);
    mVHandleBuffer.SetStreamType(STREAM_VIDEO);
    mVHandleBuffer.SetIdentity(mConfigIndex);
    mVHandleBuffer.SetCount(0);
    mpBufferQV->PutData(&mVHandleBuffer, sizeof(CGBuffer));

    if(mbEnAudio)
    {
        mAHandleBuffer.SetOwnerType(DEMUXER_FFMPEG);
        mAHandleBuffer.SetBufferType(HANDLE_BUFFER);
        mAHandleBuffer.SetStreamType(STREAM_AUDIO);
        mAHandleBuffer.SetIdentity(mConfigIndex);
        mAHandleBuffer.SetCount(0);
        mpBufferQV->PutData(&mAHandleBuffer, sizeof(CGBuffer));
        //tirck here to enable audio at the first
    }
    return ME_OK;
}

AM_ERR CGFFMpegDemuxer::FillBufferToQueue()
{
    CGBuffer gBuffer;
    if(mpBufferQV->GetDataCnt() >= DEMUXER_BQUEUE_NUM_V ||
        mpBufferQA->GetDataCnt() >= DEMUXER_BQUEUE_NUM_A)
    {
        //AM_ASSERT(mpBufferQV->GetDataCnt() < DEMUXER_BQUEUE_NUM_V);
        //if(mpBufferQV->GetDataCnt() >= DEMUXER_BQUEUE_NUM_V)
            //AM_INFO("Video BufferQueue Data Num:%d(Alloc:%d)\n", mpBufferQV->GetDataCnt(), DEMUXER_BQUEUE_NUM_V);

        //AM_INFO("Audio BufferQueue Data Num:%d(Alloc:%d)\n", mpBufferQA->GetDataCnt(), DEMUXER_BQUEUE_NUM_A);
        //return ME_ERROR;
    }
    ReadData(gBuffer);
    if(mbOnlyReceive == AM_TRUE){
        FillOnlyReceive(gBuffer);
        return ME_OK;
    }
    STREAM_TYPE type = gBuffer.GetStreamType();
    if(type == STREAM_VIDEO){
        mVideoCount++;
        gBuffer.SetCount(mVideoCount);
        mpBufferQV->PutData(&gBuffer, sizeof(CGBuffer));
    }else if(type == STREAM_AUDIO){
        mAudioCount++;
        gBuffer.SetCount(mAudioCount);
        mpBufferQA->PostMsg(&gBuffer, sizeof(CGBuffer));
        //protect for case: decode cosume, and demuxer block(audio Q still be 0, and decode cosume empty)
        //this will block all the audio, note if audioQ be full, this may be time cosume.
        if(mpBufferQA->GetDataCnt() >= DEMUXER_BQUEUE_NUM_A /3){
            if((mAudioConsumeCnt == mAudioSentCnt) && mAudioConsumeCnt != 0){
                AM_INFO("Warning==>Audio Fill Queue/2 Full send, check the speed for Sync!\n");
                SendFilterMsg(GFilter::MSG_DEMUXER_A, 0);
            }
        }
        if(mpBufferQA->GetDataCnt() >= DEMUXER_BQUEUE_NUM_A){
            //AM_INFO("Warning==>Audio Fill Queue Full but not send, check the speed for Sync!\n");
            //SendFilterMsg(GFilter::MSG_DEMUXER_A, NULL);
        }
    }
    if(gBuffer.GetBufferType() == EOS_BUFFER)
    {
        mVideoCount++;
        gBuffer.SetCount(mVideoCount);
        gBuffer.SetStreamType(STREAM_VIDEO);
        mpBufferQV->PutData(&gBuffer, sizeof(CGBuffer));
        if(mbEnAudio){
            mAudioCount++;
            gBuffer.SetCount(mAudioCount);
            gBuffer.SetStreamType(STREAM_AUDIO);
            mpBufferQA->PostMsg(&gBuffer, sizeof(CGBuffer));
        }
        mbEOS = AM_TRUE;
        AM_INFO("Stream %d demuxer EOS reached:%d\n", mConfigIndex, mVideoCount);
        //mState = STATE_EOS;
    }

    return ME_OK;
}

AM_ERR CGFFMpegDemuxer::FillOnlyReceive(CGBuffer& buffer)
{
    AM_ASSERT(0);
    return ME_OK;
}

AM_ERR CGFFMpegDemuxer::ReadData(CGBuffer& buffer)
{
    AM_INT ret;
    AM_U64 curPts = 0;
    static int n = 0;
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

        //BUG HERE!!!assume stream_index is audio 1, and this if not do.(mAudio is -1)
        //than .....get out this packet..........................................funny...
        if((mPkt.stream_index != mVideo && mPkt.stream_index != mAudio) ||
                (mPkt.stream_index == mVideo && !mbEnVideo) ||
                (mPkt.stream_index == mAudio && !mbEnAudio))
        {
            //AM_INFO("Discard AVPacket :Index:%d(A:%d, V:%d), stream_index:%d\n", mConfigIndex, mAudio, mVideo, mPkt.stream_index);
            av_free_packet(&mPkt);
            continue;
        }
        //AM_INFO("Commit AVPacket :Index:%d(A:%d, V:%d), stream_index:%d\n", mConfigIndex, mAudio, mVideo, mPkt.stream_index);

        //AM_INFO("Demuxer:size:%d(%d, %d,%d, %d, %d)------loop: %d\n", mPkt.size,
        //mPkt.data[0], mPkt.data[1], mPkt.data[2],mPkt.data[3],mPkt.data[4], ++n);

        av_dup_packet(&mPkt);
        AVPacket* pPkt = new AVPacket;
        //*pPkt = mPkt;
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
        break;
    }
    return ME_OK;
}


AM_ERR CGFFMpegDemuxer::FillEOS(CGBuffer& buffer)
{
    //AM_INFO("Fill EOS To Buffer. Stream: %d\n", mConfigIndex);
    buffer.SetBufferType(EOS_BUFFER);
    buffer.SetOwnerType(DEMUXER_FFMPEG);
    buffer.SetIdentity(mConfigIndex);
    buffer.SetStreamType(STREAM_NULL);
    return ME_OK;
}

AM_BOOL CGFFMpegDemuxer::GetBufferPolicy(CGBuffer& gBuffer)
{
    return mpBufferQV->PeekData(&gBuffer, sizeof(CGBuffer));
}

AM_ERR CGFFMpegDemuxer::GetAudioBuffer(CGBuffer& oBuffer)//a-v spe
{
    //AM_INFO("DDDDDDDDDDD2, %d, %d, %d\n", mState, mbEnAudio, mbAHandleSent);

    if(mState == STATE_PAUSE)
        return ME_CLOSED;
    if(mbEnAudio == AM_FALSE)
        return ME_ERROR;
    if(mbAHandleSent == AM_FALSE)
        return ME_ERROR;

    AM_BOOL rval;
    //AM_INFO("Data Cnt: %d\n", mpBufferQA->GetDataCnt());
    rval = mpBufferQA->PeekMsg(&oBuffer, sizeof(CGBuffer));
    if(rval == AM_FALSE){
        //no data
        return ME_ERROR;
    }
    mAudioSentCnt++;
    return ME_OK;
}

AM_ERR CGFFMpegDemuxer::GetCGBuffer(CGBuffer& buffer)
{
    if(mbOnlyReceive == AM_TRUE)
        return ME_CLOSED;
    if(mState == STATE_PAUSE){
        return ME_CLOSED;
    }
    if(mpConfig->pausedCnt > 0){
        AM_INFO("%d Retrieve still Doing (%d) .\n", mConfigIndex, mpConfig->pausedCnt);
        return ME_CLOSED;
    }
    AM_BOOL rval;

    while(1)
    {
        //first send out retrieveQ
        if(mpRetrieveQ->GetDataCnt() > 0){
            //note this will always be true
            rval = mpRetrieveQ->PeekMsg(&buffer, sizeof(CGBuffer));
            AM_ASSERT(rval == AM_TRUE);
            AM_ASSERT(buffer.GetStreamType() == STREAM_VIDEO);
        }else{
            rval = GetBufferPolicy(buffer);
        }

        if(rval == AM_FALSE)
        {
            //no data(be freed), read from mpBufferQV. Never happen after a-v spe
            ContinueRun();
            return ME_ERROR;
        }

        BUFFER_TYPE btype = (BUFFER_TYPE)buffer.GetBufferType();
        if(btype == HANDLE_BUFFER){
            if(buffer.GetStreamType() == STREAM_AUDIO){
                mbAHandleSent = AM_TRUE;
            }
            break;
        }

        STREAM_TYPE type = (STREAM_TYPE)buffer.GetStreamType();
        AM_ASSERT(type == STREAM_VIDEO);
         if(!mbEnVideo){
              //TODO THIS
              AM_ASSERT(0);
             //release this packet
            AVPacket* pkt = (AVPacket* )(buffer.GetExtraPtr());
            av_free_packet(pkt);
            delete pkt;
            buffer.SetExtraPtr((AM_INTPTR)NULL);
            continue;
        }
        break;
    }
    //buffer.Dump("INSIDE!!");
    if(buffer.GetStreamType() == STREAM_VIDEO){
        mVideoSentCnt++;
    }
    return ME_OK;
}

inline AM_ERR CGFFMpegDemuxer::ContinueRun()
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

inline AM_BOOL CGFFMpegDemuxer::NeedSendAudio(CGBuffer* buffer)
{
    /*
    if(buffer->GetStreamType() == STREAM_AUDIO)
    AM_INFO(" NeedSendAudio(%d), streamtype%d, ena%d, aQcnt%d, state%d, run%d, buffer%d, sent%d, consume:%d\n", mConfigIndex,
        buffer->GetStreamType(), mbEnAudio, mpBufferQA->GetDataCnt(), mState, mbRun, buffer->GetCount(), mAudioSentCnt, mAudioConsumeCnt);
    */


    if(buffer->GetStreamType() != STREAM_AUDIO)
        return false;

    if(!mbEnAudio){
        if(mpBufferQA->GetDataCnt() > 0)
            ClearQueue(mpBufferQA);
        return false;
    }
    if((mState != STATE_IDLE && mState != STATE_PENDING) || mbRun == false){
        return false;
    }

    AM_INT sentDiff, diff;
    sentDiff = mAudioSentCnt - (buffer->GetCount());
    diff = mAudioCount - (buffer->GetCount());
    //AM_INFO("Audio BufferQueue Data Num:%d(Alloc:%d), (%d, %d) , sent:%d\n", mpBufferQA->GetDataCnt(), DEMUXER_BQUEUE_NUM_A
    //, mAudioCount, buffer->GetCount(), mAudioSentCnt);
    if(diff <= 96){
        mNoPending = true;
    }else{
        mNoPending = false;
    }

    //nopending must be assign before return this
    if(mpBufferQA->GetDataCnt() <= 0){
        return false;
    }

    if(sentDiff > 64 && mpBufferQA->GetDataCnt() <= 32)
        return false;

    return true;
}

AM_ERR CGFFMpegDemuxer::OnReleaseBuffer(CGBuffer* buffer)
{
    if(buffer->GetStreamType() == STREAM_VIDEO){
        //AM_ASSERT(mVideoConsumeCnt == buffer->GetCount());
        //buffer->Dump("OnReleaseBuffer");
        if(mVideoConsumeCnt != buffer->GetCount())
            AM_INFO("DIff(%d) : %d-->%d\n", mConfigIndex, mVideoConsumeCnt, buffer->GetCount());

        mVideoConsumeCnt++;
            //for case no video on bp when pause
        if(mVideoConsumeCnt == mpConfig->pausedCnt)
            mpConfig->pausedCnt = 0;
    }else{
        AM_ASSERT(mAudioConsumeCnt == buffer->GetCount());
        mAudioConsumeCnt++;
        //debug
        //AM_INFO(" CGFFMpegDemuxer OnReleaseBuffer %d, %d  %d-%d.\n", mConfigIndex, mAudioCount, buffer->GetCount(), mAudioSentCnt);
    }

    if(NeedSendAudio(buffer) == AM_TRUE)
    {
        //AM_INFO("DDDDDDDDDDD1\n");
        SendFilterMsg(GFilter::MSG_DEMUXER_A, 0);
    }
    if(buffer->GetBufferType() != DATA_BUFFER)
        return ME_OK;

    if(mpBufferQV->GetDataCnt() <= (DEMUXER_BQUEUE_NUM_V /3))
    {
        ContinueRun();//inline this function.
    }else if(mpBufferQA->GetDataCnt() <= (DEMUXER_BQUEUE_NUM_A /3)){
        ContinueRun();
    }

    AVPacket* pkt = (AVPacket* )(buffer->GetExtraPtr());
    if(pkt != NULL){
        av_free_packet(pkt);
        delete pkt;
        buffer->SetExtraPtr((AM_INTPTR)NULL);
    }else{
        AM_ASSERT(0);
    }
    //AM_INFO(" CGFFMpegDemuxer OnReleaseBuffer Done!!\n");
    return ME_OK;
}

AM_ERR CGFFMpegDemuxer::OnRetrieveBuffer(CGBuffer* buffer)
{
    //retrieve Q ensured small than GDM-GDE video bufferpool(audio only one way)
    AM_ASSERT(mpRetrieveQ->GetDataCnt() <= DEMUXER_RETRIEVE_Q_NUM);
    AM_ASSERT(buffer->GetStreamType() == STREAM_VIDEO);
    AM_UINT bufferCnt = buffer->GetCount() + 1;
    if(bufferCnt == mpConfig->pausedCnt)
        mpConfig->pausedCnt = 0;
    //AM_INFO("OnRetrieveBuffer:bufferCnt %d, paseCnt:%d, videoSent:%d, videoConsuem:%d ,videocnt:%d\n",
        //bufferCnt - 1, mpConfig->pausedCnt, mVideoSentCnt, mVideoConsumeCnt, mVideoCount);
    mVideoSentCnt--;
    mpRetrieveQ->PostMsg(buffer, sizeof(CGBuffer));
    return ME_OK;
}
//===========================================
//
//===========================================
AM_ERR CGFFMpegDemuxer::PostFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    msg.p1 = mConfigIndex;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->PostMsgToFilter(msg);
    return ME_OK;
}

AM_ERR CGFFMpegDemuxer::SendFilterMsg(AM_UINT code, AM_INTPTR ptr)
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
AM_ERR CGFFMpegDemuxer::LoadFile()
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
    AM_INFO("CGFFMpegDemuxer: FindMediaStream video[%d]audio[%d].\n", mVideo, mAudio);

    if (mVideo >= 0)
    {
        err = SetFormat(mVideo, AVMEDIA_TYPE_VIDEO);
        if(err == ME_BAD_FORMAT)
            return err;
        if (err != ME_OK) {
            AMLOG_PRINTF("InitStream video stream fail, disable it.\n");
            mbEnVideo = false;
        }
    }

    if (mAudio >= 0)
    {
        err = SetFormat(mAudio, AVMEDIA_TYPE_AUDIO);
        if(err == ME_BAD_FORMAT)
            return err;
        if (err != ME_OK) {
            AMLOG_ERROR("initStream audio stream fail, disable it.\n");
            mbEnAudio = false;
        }
    }else{
        if(mpConfig->disableAudio == false)
            mpConfig->disableAudio = true;
    }

    if(mpConfig->disableAudio == true)
        mbEnAudio = false;

    if(mpConfig->disableVideo == true)
        mbEnVideo = false;

    //mbOnlyReceive = mpConfig->doRecieve;

    UpdatePTSConvertor();
    //OR mVHandleBuffer.ffmpegStream = stream; think about this.
    CParam envPar(2);
    mpGConfig->DemuxerSetDecoderEnv(mConfigIndex, (void* )mpAVFormat, envPar);
    return ME_OK;
}

int CGFFMpegDemuxer::hasCodecParameters(AVCodecContext* enc)
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

AM_INT CGFFMpegDemuxer::FindMediaStream(AM_INT media)
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

void CGFFMpegDemuxer::UpdatePTSConvertor()
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

AM_U64 CGFFMpegDemuxer::ConvertVideoPts(am_pts_t pts)
{
    pts *= mPTSVideo_Num;
    if (mPTSVideo_Den != 1) {
        pts /=  mPTSVideo_Den;
    }
    return pts;
}

AM_U64 CGFFMpegDemuxer::ConvertAudioPts(am_pts_t pts)
{
    pts *= mPTSAudio_Num;
    if (mPTSAudio_Den != 1) {
        pts /=  mPTSAudio_Den;
    }
    return pts;
}
//
//
//===========================================================//
#if 1
AM_ERR CGFFMpegDemuxer::SeekTo(AM_U64 ms)
{
    AM_ASSERT(AV_TIME_BASE == 1000000);
    AM_S64 seek_target = ms * 1000;
    AM_INT ret = 0;

    //seek by time
    if ((ret = av_seek_frame(mpAVFormat, -1, seek_target, 0)) < 0) {
        AMLOG_ERROR("seek by time fail, ret =%d, try by bytes.\n", ret);
        //try seek by byte
        seek_target =seek_target *mpAVFormat->file_size/mpAVFormat->duration;
        if ((ret = av_seek_frame( mpAVFormat, -1, seek_target, AVSEEK_FLAG_BYTE )) < 0) {
            AMLOG_ERROR("seek by bytes fail, ret =%d, Seek return error.\n", ret);
            return ME_ERROR;
        }
    }
    return ME_OK;
}
#else
AM_ERR CGFFMpegDemuxer::Seek(AM_U64 ms)
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


AM_ERR CGFFMpegDemuxer::GetTotalLength(AM_U64& ms)
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
AM_ERR CGFFMpegDemuxer::SetFormat(int stream, int media)
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

void CGFFMpegDemuxer::Dump()
{
    AM_INFO("       {---FFMpeg Demuxer---}\n");
    AM_INFO("           Config->en audio:%d, en video:%d\n", mbEnAudio, mbEnVideo);
    AM_INFO("           State->%d, sent handleA:%d, pauseCnt:%d\n", mState, mbAHandleSent, mpConfig->pausedCnt);
    AM_INFO("           BufferPool->Audio Bp Cnt:%d, Video Bp Cnt:%d, Retrieve Q Cnt:%d\n",
        mpBufferQA->GetDataCnt(), mpBufferQV->GetDataCnt(), mpRetrieveQ->GetDataCnt());
}

