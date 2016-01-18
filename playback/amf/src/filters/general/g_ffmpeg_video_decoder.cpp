/*
 * g_ffmpeg_video_decoder.cpp
 *
 * History:
 *    2013/4/6 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "g_ffmpeg_video_decoder"
//#define AMDROID_DEBUG

//#define USE_LIBAACDEC
#include <sys/time.h>

#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

extern "C" {
#define INT64_C
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "aac_audio_dec.h"
}
#include "g_ffmpeg_video_decoder.h"

//-----------------------------------------------------------------------
//
// CGFFMpegVideoDecoder
//
//-----------------------------------------------------------------------
AM_INT CGFFMpegVideoDecoder::mConfigIndex = 0;
STREAM_TYPE CGFFMpegVideoDecoder::mType = STREAM_VIDEO;
AM_INT CGFFMpegVideoDecoder::mStreamIndex = 0;
CQueue* CGFFMpegVideoDecoder::mpDataQ = NULL;

GDecoderParser gDecoderFFMpegVideo = {
    "PureSoft-FFMpeg-Video-Decoder",
    CGFFMpegVideoDecoder::Create,
    CGFFMpegVideoDecoder::ParseBuffer,
    CGFFMpegVideoDecoder::ClearParse,
};

//
AM_INT CGFFMpegVideoDecoder::ParseBuffer(const CGBuffer* gBuffer)
{
    //(const_cast<CGBuffer*>(gBuffer))->Dump("ParseBuffer");
    AM_INT score = 0;

    STREAM_TYPE type = gBuffer->GetStreamType();
    if(type != STREAM_VIDEO)
        return -90;

    if(*(gBuffer->codecType) != GUID_Video_H264)
        return -90;

    mpDataQ = (CQueue* )(gBuffer->GetExtraPtr());
    if(mpDataQ == NULL)
        return -100;

    mConfigIndex  = gBuffer->GetIdentity();
    mStreamIndex = gBuffer->ffmpegStream;
    mType = type;
    score = 92;
    AM_INFO("CGFFMpegVideoDecoder, Score:%d, type(%d)\n", score, type);
    return score;
}

AM_ERR CGFFMpegVideoDecoder::ClearParse()
{
    return ME_OK;
}
//-----------------------------------------------------------------------
//
// CGFFMpegVideoDecoder
IGDecoder* CGFFMpegVideoDecoder::Create(IFilter* pFilter, CGConfig* pconfig)
{
    CGFFMpegVideoDecoder* result = new CGFFMpegVideoDecoder(pFilter, pconfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
        }
    return result;
}

CGFFMpegVideoDecoder::CGFFMpegVideoDecoder(IFilter* pFilter, CGConfig* pConfig):
    inherited("CGFFMpegVideoDecoder"),
    mpFilter(pFilter),
    mpGConfig(pConfig),
    mSentBufferCnt(0),
    mBufferCnt(0),
    mConsumeCnt(0),
    mbIsEOS(AM_FALSE),
    mpVideoQ(NULL),
    mpInputQ(NULL),
    mpQOwner(NULL),
    mTotalFrame(0)
{
    mbRun = true;
    mpConfig = &(pConfig->decoderConfig[mConfigIndex]);
    mConIndex = mConfigIndex;
    mStreamType = mType;
}

AM_ERR CGFFMpegVideoDecoder::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    if(mStreamType == STREAM_VIDEO){
        if ((mpTempMianQ = CQueue::Create(NULL, this, sizeof(AM_INT), 1)) == NULL)
            return ME_NO_MEMORY;
        if ((mpVideoQ = CQueue::Create(mpTempMianQ, this, sizeof(CGBuffer), GDECODER_FFMPEG_VIDEO_QNUM)) == NULL)
            return ME_NO_MEMORY;
    }
    mpAVFormat = (AVFormatContext* )mpConfig->decoderEnv;
    mpStream = mpAVFormat->streams[mStreamIndex];
    mpCodec = mpStream->codec;

    err = ConstructFFMpeg();
    if (err != ME_OK)
    {
        AM_ERROR("CGFFMpegVideoDecoder ,ConstructCoreAVC fail err = %d .\n", err);
        return err;
    }
    //attach data q to cmd q
    CQueue* cmdQ = MsgQ();
    mpDataQ->Attach(cmdQ);
    mpInputQ = mpDataQ;
    //cmdQ->Dump("mpDataQ");

    mpWorkQ->SetThreadPrio(1, 1);
    DSetModuleLogConfig(LogModuleFFMpegDecoder);
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::ConstructFFMpeg()
{
    if(mStreamType != STREAM_VIDEO){
        AM_ASSERT(0);
        return ME_ERROR;
    }
    CUintMuxerConfig& curUnit = mpGConfig->mainMuxer->unitConfig[mConIndex];
    if(curUnit.configVideo == AM_FALSE){
        AM_ASSERT(0);
    }

    mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
    if(mpDecoder == NULL) {
        AM_ERROR("ffmpeg video decoder not found, mpCodec->codec_id=%x. \n",mpCodec->codec_id);
        return ME_ERROR;
    }
    int rval = avcodec_open(mpCodec, mpDecoder);
    if (rval < 0) {
        AM_ERROR("avcodec_open failed %d\n", rval);
        mpCodec = NULL;
        return ME_ERROR;
    }

    return ME_OK;
}

void CGFFMpegVideoDecoder::ClearQueue(CQueue* queue)
{
    if(queue == NULL)
        return;

    AM_BOOL rval;
    CGBuffer buffer;
    while(1)
    {
        rval = queue->PeekData(&buffer, sizeof(CGBuffer));
        if(rval == AM_FALSE)
        {
            break;
        }
        //release this buffer
        AM_ASSERT(queue == mpVideoQ);
        AM_ASSERT(mConsumeCnt == buffer.GetCount());
        if(mConsumeCnt != buffer.GetCount())
            AM_INFO("Diff: %d, %d\n",mConsumeCnt, buffer.GetCount());
        mConsumeCnt++;
        AM_U16* ptr = (AM_U16* )(buffer.GetExtraPtr());
        delete[] ptr;
        continue;
    }
}

void CGFFMpegVideoDecoder::Delete()
{
    AM_INFO("CGFFMpegVideoDecoder::Delete().\n");
    ClearQueue(mpVideoQ);

    mpInputQ->Detach();
    mpVideoQ->Attach(mpTempMianQ);
    if(mpCodec){
        avcodec_close(mpCodec);
    }

    AM_DELETE(mpVideoQ);
    AM_DELETE(mpTempMianQ);
    inherited::Delete();
}

CGFFMpegVideoDecoder::~CGFFMpegVideoDecoder()
{
    AM_INFO("~CGFFMpegVideoDecoder.\n");
    AM_INFO("~CGFFMpegVideoDecoder done.\n");
}
//--------------------------------------------------------------------
//
//
//--------------------------------------------------------------------
AM_ERR CGFFMpegVideoDecoder::PerformCmd(CMD& cmd, AM_BOOL isSend)
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

AM_ERR CGFFMpegVideoDecoder::DoPause()
{
    if(mState == STATE_PENDING || mState == STATE_ERROR){
        AM_ASSERT(0);
        return ME_OK;
    }
    mState = STATE_PENDING;
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::DoResume()
{
    AM_ASSERT(mState == STATE_PENDING);
    if(mBuffer.GetBufferType() == NOINITED_BUFFER){
        mState = STATE_IDLE;
    }else{
        mState = STATE_HAS_INPUTDATA;
    }
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::DoFlush()
{
    AM_ASSERT(mState == STATE_PENDING);

    ClearQueue(mpVideoQ);
    if(mBuffer.GetBufferType() != NOINITED_BUFFER){
        ((IGDemuxer*)mpQOwner)->OnReleaseBuffer(&mBuffer);
        mBuffer.Clear();
    }
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::DoStop()
{
    if(mBuffer.GetBufferType() != NOINITED_BUFFER){
        ((IGDemuxer*)mpQOwner)->OnReleaseBuffer(&mBuffer);
        mBuffer.Clear();
    }
    mbRun = false;
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::DoIsReady()
{
    //mBuffer.Dump("DoIsReady");
    if(mpVideoQ->GetDataCnt() >= (GDECODER_FFMPEG_VIDEO_QNUM - 10))
        return ME_BUSY;
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::IsReady(CGBuffer* pBuffer)
{
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGFFMpegVideoDecoder::ProcessCmd %d.\n", cmd.code);
    AM_ERR err = ME_OK;
    switch (cmd.code)
    {
    case CMD_STOP:
        AM_INFO("CGFFMpegVideoDecoder Process CMD_STOP.\n");
        DoStop();
        CmdAck(ME_OK);
        break;

     //todo check this
    case CMD_PAUSE:
        DoPause();
        CmdAck(ME_OK);
        break;

    case CMD_RESUME:
        DoResume();
        CmdAck(ME_OK);
        break;

    //TODO, seek
    case CMD_FLUSH:
        //avcodec_flush_buffers
        DoFlush();
        CmdAck(ME_OK);
        break;

    case CMD_DECODE:
        AM_ASSERT(0);
        break;

    case CMD_ISREADY:
        AM_ASSERT(0);
        break;

    case CMD_CLEAR:
        CmdAck(ME_OK);
        break;

    case CMD_ACK:
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return err;
}

void CGFFMpegVideoDecoder::OnRun()
{
    AM_ERR err;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    AM_INT ret = 0;

    FillHandleBuffer();
    mbRun = true;
    mState = STATE_IDLE;
    CmdAck(ME_OK);

    while(mbRun)
    {
        switch(mState)
        {
        case STATE_IDLE:
            type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
            if(type == CQueue::Q_MSG)
            {
                ProcessCmd(cmd);
            }else{
                AM_ASSERT(result.pDataQ == mpInputQ);
                ret = mpInputQ->PeekData(&mBuffer, sizeof(CGBuffer));
                AM_ASSERT(ret == true);
                if(mpQOwner == NULL)
                    mpQOwner = result.pOwner;

                mState = STATE_HAS_INPUTDATA;
            }
            break;

        case STATE_HAS_INPUTDATA:
            err = DoIsReady();
            if(err != ME_OK){
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
            }else{
                mState = STATE_READY;
            }
            break;

        case STATE_READY:
            if(mBuffer.GetBufferType() == EOS_BUFFER)
            {
                err = FillEOS();
                mState = STATE_IDLE;
                break;
            }
            err = DoDecode();
            if(err != ME_OK){
                AM_ASSERT(ME_OK);
                mState = STATE_ERROR;
            }else{
                mState = STATE_IDLE;
            }
            break;

        case STATE_PENDING:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        case STATE_ERROR:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            //filter some cmd
            ProcessCmd(cmd);
            break;

        default:
            break;
        }
    }
    AM_ASSERT(mBuffer.GetBufferType() == NOINITED_BUFFER);
    AM_INFO("CGFFMpegVideoDecoder OnRun Exit.\n");
}
//------------------------------------------------------
//
//
//------------------------------------------------------
inline AM_ERR CGFFMpegVideoDecoder::FillHandleBuffer()
{
    //FIXME TO RENDER VIDEO FRAMES
    mOBuffer.SetOwnerType(DECODER_FFMEPG);
    mOBuffer.SetBufferType(HANDLE_BUFFER);
    mOBuffer.SetIdentity(mConIndex);
    mOBuffer.SetStreamType(STREAM_VIDEO);
    mOBuffer.SetCount(0);
    mOBuffer.SetExtraPtr((AM_INTPTR)mpVideoQ);
    mOBuffer.SetGBufferPtr((AM_INTPTR)this);
    //AM_INFO("111111111111111:%d\n", (AM_INTPTR)this);
    mpVideoQ->Detach();
    SendFilterMsg(GFilter::MSG_DECODED, (AM_INTPTR)&mOBuffer);//wakeup isready and send cbuffer. make sure sended and then ack
    mOBuffer.Clear();
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::DoDecode()
{
    AM_ERR err=ME_OK;
    STREAM_TYPE type = mBuffer.GetStreamType();
    AM_ASSERT(type == mStreamType);
    err = DecodeVideo();

    ((IGDemuxer*)mpQOwner)->OnReleaseBuffer(&mBuffer);
    mBuffer.Clear();
    return err;
}

AM_ERR CGFFMpegVideoDecoder::DecodeVideo()
{
    AM_ERR err = ME_OK;
    AM_INT usedSize = 0;
    AM_INT finished;
    AVPacket* pPacket = (AVPacket* )(mBuffer.GetExtraPtr());
    AM_ASSERT(pPacket != NULL);
    AM_U8* saveData = pPacket->data;
    AM_INT saveSize = pPacket->size;

    while(1)
    {
        usedSize = avcodec_decode_video2(mpCodec, &mFrame, &finished, pPacket);
        if(usedSize < 0 || usedSize > pPacket->size){
            AM_ERROR("avcodec_decode_video2 Failed\n");
            err = ME_ERROR;
            break;
        }
        if(usedSize == 0){
            AM_ERROR("avcodec_decode_video2() don't finished a frame, finished %d\n", finished);
            err = ME_OK;
            break;
        }
        AM_ASSERT(finished);

        FillVideoFrameQueue();
        if(usedSize < pPacket->size)
        {
            AM_INFO("Decoder Again!!!!!!\n");
            pPacket->data += usedSize;
            pPacket->size -= usedSize;
            usedSize = 0;
            continue;
        }
        AM_ASSERT(usedSize == pPacket->size);
        break;
    }
    pPacket->data = saveData;
    pPacket->size = saveSize;
    return err;
}

inline AM_ERR CGFFMpegVideoDecoder::FillVideoFrameQueue()
{
    UpdateVideoInfo();
    return ME_OK;
}

inline AM_ERR CGFFMpegVideoDecoder::UpdateVideoInfo()
{
    //AM_INT sampleNum = 0;
    if(mTotalFrame == 0){
        gettimeofday(&mTimeStart,NULL);
    }
    mTotalFrame++;
    if((mTotalFrame % 100)==0){
        struct timeval tvTime;
        gettimeofday(&tvTime,NULL);
        AM_INT time = (tvTime.tv_sec -mTimeStart.tv_sec)*1000000 + tvTime.tv_usec - mTimeStart.tv_usec;
        AM_INFO("=======After %d frames, each frame's average time consume:%fus.============\n",
            mTotalFrame, (float)time/(float)mTotalFrame);
    }
    return ME_OK;
}

//Abandonment
AM_ERR CGFFMpegVideoDecoder::GetDecodedGBuffer(CGBuffer& oBuffer)
{
    AM_ASSERT(0);
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::OnReleaseBuffer(CGBuffer* buffer)
{
    //AM_INFO("CC1   OnReleaseBuffer\n");
    AM_ASSERT(mConsumeCnt == buffer->GetCount());
    if(mConsumeCnt != buffer->GetCount())
        AM_INFO("Diff: %d, %d\n",mConsumeCnt, buffer->GetCount());
    mConsumeCnt++;

    if(buffer->GetBufferType() != DATA_BUFFER)
        return ME_OK;
    AM_ASSERT(0);
    STREAM_TYPE type = buffer->GetStreamType();
    AM_ASSERT(type == mStreamType);
    if(mStreamType == STREAM_AUDIO){
        AM_U16* ptr = (AM_U16* )(buffer->GetExtraPtr());
        delete[] ptr;
    }else{
        AM_ASSERT(0);
    }
    if(mState == STATE_HAS_INPUTDATA)
        mpWorkQ->PostMsg(CMD_ACK);

    if(mbIsEOS == AM_TRUE){
    }
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::FillEOS()
{
    AM_INFO("EOS Filling on CGFFMpegVideoDecoder\n");

    return ME_OK;
}
//===========================================
//
//===========================================
AM_ERR CGFFMpegVideoDecoder::PostFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    AM_INT size = 0;
    if(mStreamType == STREAM_AUDIO){
        size = mpVideoQ->GetDataCnt();
    }
    msg.p1 = size;
    msg.p2 = mConIndex;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->PostMsgToFilter(msg);
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::SendFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    AM_INT size = 0;
    if(mStreamType == STREAM_AUDIO){
        size = mpVideoQ->GetDataCnt();
    }
    msg.p1 = size;
    msg.p2 = mConIndex;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->SendMsgToFilter(msg);
    return ME_OK;
}

AM_ERR CGFFMpegVideoDecoder::ReSet()
{
    return ME_OK;
}

void CGFFMpegVideoDecoder::Dump()
{
    AM_INFO("       {---FFMpeg Vide Decoder---}\n");
    AM_INFO("           State->%d, \n", mState);
    //AM_INFO("           BufferPool->Input Buffer Cnt:%d, Audio Decoded Bp(%p) Cnt:%d\n", mpInputQ->GetDataCnt(), mpAudioQ, mpAudioQ->GetDataCnt());
    AM_INFO("           Ptr Dump->Input Buffer Ptr:%p, Owner Ptr:%p\n", mpInputQ, mpQOwner);
    //CQueue* cmdQ = MsgQ();
    //cmdQ->Dump("           CGFFMpegVideoDecoder Q");
    struct timeval tvTime;
    gettimeofday(&tvTime,NULL);
    AM_INT time = (tvTime.tv_sec -mTimeStart.tv_sec)*1000000 + tvTime.tv_usec - mTimeStart.tv_usec;

    AM_INFO("           Total Frame:%d, each frame's average time consume:%fus.", mTotalFrame, (float)time/(float)mTotalFrame);
}
