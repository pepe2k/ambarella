/*
 * g_coreavc_decoder.cpp
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
#define LOG_TAG "g_coreavc_decoder"
//#define AMDROID_DEBUG

//#define USE_LIBAACDEC

#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

extern "C" {
#define INT64_C
#include "coreavc/corec.h"
#include "coreavc/avc.h"
}
#include "audio_if.h"
#include "g_coreavc_decoder.h"

MEMALLOC_DEFAULT

//-----------------------------------------------------------------------
//
// CGCoreAVCDecoder
//
//-----------------------------------------------------------------------
AM_INT CGCoreAVCDecoder::mConfigIndex = 0;
STREAM_TYPE CGCoreAVCDecoder::mType = STREAM_VIDEO;
CQueue* CGCoreAVCDecoder::mpDataQ = NULL;

GDecoderParser gDecoderCoreAVC = {
    "PureSoft-CoreAVC-Decoder",
    CGCoreAVCDecoder::Create,
    CGCoreAVCDecoder::ParseBuffer,
    CGCoreAVCDecoder::ClearParse,
};

//
AM_INT CGCoreAVCDecoder::ParseBuffer(const CGBuffer* gBuffer)
{
    //(const_cast<CGBuffer*>(gBuffer))->Dump("ParseBuffer");
    AM_INT score = 0;

    STREAM_TYPE type = gBuffer->GetStreamType();
    if(type != STREAM_VIDEO)
        return -90;

    if(*(gBuffer->codecType) != GUID_Video_FF_OTHER_CODECS)
        return -90;

    mpDataQ = (CQueue* )(gBuffer->GetExtraPtr());
    if(mpDataQ == NULL)
        return -100;

    mConfigIndex  = gBuffer->GetIdentity();
    mType = type;
    score = 92;
    AM_INFO("CGCoreAVCDecoder, Score:%d, type(%d)\n", score, type);
    return score;
}

AM_ERR CGCoreAVCDecoder::ClearParse()
{
    return ME_OK;
}
//-----------------------------------------------------------------------
//
// CGCoreAVCDecoder
IGDecoder* CGCoreAVCDecoder::Create(IFilter* pFilter, CGConfig* pconfig)
{
    CGCoreAVCDecoder* result = new CGCoreAVCDecoder(pFilter, pconfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
        }
    return result;
}

CGCoreAVCDecoder::CGCoreAVCDecoder(IFilter* pFilter, CGConfig* pConfig):
    inherited("CGCoreAVCDecoder"),
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

AM_ERR CGCoreAVCDecoder::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    if(mStreamType == STREAM_VIDEO){
        if ((mpTempMianQ = CQueue::Create(NULL, this, sizeof(AM_INT), 1)) == NULL)
            return ME_NO_MEMORY;
        if ((mpVideoQ = CQueue::Create(mpTempMianQ, this, sizeof(CGBuffer), GDECODER_COREAVC_VIDEO_QNUM)) == NULL)
            return ME_NO_MEMORY;
    }

    err = ConstructCoreAVC();
    if (err != ME_OK)
    {
        AM_ERROR("CGCoreAVCDecoder ,ConstructCoreAVC fail err = %d .\n", err);
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

AM_ERR CGCoreAVCDecoder::ConstructCoreAVC()
{
    if(mStreamType != STREAM_VIDEO){
        AM_ASSERT(0);
        return ME_ERROR;
    }
    AM_INT ret;
    CUintMuxerConfig& curUnit = mpGConfig->mainMuxer->unitConfig[mConIndex];
    if(curUnit.configVideo == AM_FALSE){
        AM_ASSERT(0);
    }

    avc_init(&mCodec, CPUCaps(), &MemAlloc_Default, 0, NULL, NULL);

    ret = avc_extra(&mCodec, curUnit.videoInfo.extradata, curUnit.videoInfo.extrasize, -1);
    if(ret < 0){
        AM_ERROR("avc_extra add failed!\n");
        return ME_ERROR;
    }
    return ME_OK;
}

void CGCoreAVCDecoder::ClearQueue(CQueue* queue)
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

void CGCoreAVCDecoder::Delete()
{
    AM_INFO("CGCoreAVCDecoder::Delete().\n");
    ClearQueue(mpVideoQ);

    mpInputQ->Detach();
    mpVideoQ->Attach(mpTempMianQ);
    avc_done(&mCodec);

    AM_DELETE(mpVideoQ);
    AM_DELETE(mpTempMianQ);
    inherited::Delete();
}

CGCoreAVCDecoder::~CGCoreAVCDecoder()
{
    AM_INFO("~CGCoreAVCDecoder.\n");
    AM_INFO("~CGCoreAVCDecoder done.\n");
}
//--------------------------------------------------------------------
//
//
//--------------------------------------------------------------------
AM_ERR CGCoreAVCDecoder::PerformCmd(CMD& cmd, AM_BOOL isSend)
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

AM_ERR CGCoreAVCDecoder::DoPause()
{
    if(mState == STATE_PENDING || mState == STATE_ERROR){
        AM_ASSERT(0);
        return ME_OK;
    }
    mState = STATE_PENDING;
    return ME_OK;
}

AM_ERR CGCoreAVCDecoder::DoResume()
{
    AM_ASSERT(mState == STATE_PENDING);
    if(mBuffer.GetBufferType() == NOINITED_BUFFER){
        mState = STATE_IDLE;
    }else{
        mState = STATE_HAS_INPUTDATA;
    }
    return ME_OK;
}

AM_ERR CGCoreAVCDecoder::DoFlush()
{
    AM_ASSERT(mState == STATE_PENDING);

    ClearQueue(mpVideoQ);
    if(mBuffer.GetBufferType() != NOINITED_BUFFER){
        ((IGDemuxer*)mpQOwner)->OnReleaseBuffer(&mBuffer);
        mBuffer.Clear();
    }
    return ME_OK;
}

AM_ERR CGCoreAVCDecoder::DoStop()
{
    if(mBuffer.GetBufferType() != NOINITED_BUFFER){
        ((IGDemuxer*)mpQOwner)->OnReleaseBuffer(&mBuffer);
        mBuffer.Clear();
    }
    mbRun = false;
    return ME_OK;
}

AM_ERR CGCoreAVCDecoder::DoIsReady()
{
    //mBuffer.Dump("DoIsReady");
    if(mpVideoQ->GetDataCnt() >= (GDECODER_COREAVC_VIDEO_QNUM - 10))
        return ME_BUSY;
    return ME_OK;
}

AM_ERR CGCoreAVCDecoder::IsReady(CGBuffer* pBuffer)
{
    return ME_OK;
}

AM_ERR CGCoreAVCDecoder::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGCoreAVCDecoder::ProcessCmd %d.\n", cmd.code);
    AM_ERR err = ME_OK;
    switch (cmd.code)
    {
    case CMD_STOP:
        AM_INFO("CGCoreAVCDecoder Process CMD_STOP.\n");
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

void CGCoreAVCDecoder::OnRun()
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
    AM_INFO("CGCoreAVCDecoder OnRun Exit.\n");
}
//------------------------------------------------------
//
//
//------------------------------------------------------
inline AM_ERR CGCoreAVCDecoder::FillHandleBuffer()
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

AM_ERR CGCoreAVCDecoder::DoDecode()
{
    AM_ERR err=ME_OK;
    STREAM_TYPE type = mBuffer.GetStreamType();
    AM_ASSERT(type == mStreamType);
    err = DecodeVideo();

    ((IGDemuxer*)mpQOwner)->OnReleaseBuffer(&mBuffer);
    mBuffer.Clear();
    return err;
}

AM_ERR CGCoreAVCDecoder::DecodeVideo()
{
    AM_INT ret, flags = 0;
    static AM_INT test = 0;

    ret = avc_frame(&mCodec, mBuffer.PureDataPtr(), mBuffer.PureDataSize(), NULL, mFrame, NULL, NULL, flags);
    if(ret == ERR_NEED_MORE_DATA){
        //AM_ERROR("avc_frame don't finished a frame, need more data.\n");
        return ME_OK;
    }
    if(ret != ERR_NONE || !mFrame[0]) {
        AM_ERROR("avc_frame() fail, ret %d\n", ret);
        return ME_ERROR;
    }
    FillVideoFrameQueue();
    while(1)
    {
        avc_format(&mCodec, &mFormat);
        if(test == 0){
            AM_INFO("Decoded YUV Info: Height-%d, Width-%d, Pitch-%d, %s.\n", mFormat.Height, mFormat.Width, mFormat.Pitch,
               mFormat.Pixel.Flags & (PF_YUV420I|PF_YUV420I9|PF_YUV420I10) ? "YUV420I" : "YUV420P");
            test = 1;
        }
        ret = avc_frame(&mCodec, NULL,  0, NULL, mFrame, NULL, NULL, flags);
        if(ret == ERR_NEED_MORE_DATA){
            break;
        }
        if(ret != ERR_NONE || !mFrame[0]) {
            return ME_ERROR;
        }
        FillVideoFrameQueue();
        AM_INFO("More than one video frames decoded.!\n");
    }
    return ME_OK;
}

inline AM_ERR CGCoreAVCDecoder::FillVideoFrameQueue()
{
    UpdateVideoInfo();
    return ME_OK;
}

inline AM_ERR CGCoreAVCDecoder::UpdateVideoInfo()
{
    AM_INT sampleNum = 0;
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
AM_ERR CGCoreAVCDecoder::GetDecodedGBuffer(CGBuffer& oBuffer)
{
    AM_ASSERT(0);
    return ME_OK;
}

AM_ERR CGCoreAVCDecoder::OnReleaseBuffer(CGBuffer* buffer)
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

AM_ERR CGCoreAVCDecoder::FillEOS()
{
    AM_INFO("EOS Filling on CGCoreAVCDecoder\n");

    return ME_OK;
}
//===========================================
//
//===========================================
AM_ERR CGCoreAVCDecoder::PostFilterMsg(AM_UINT code, AM_INTPTR ptr)
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

AM_ERR CGCoreAVCDecoder::SendFilterMsg(AM_UINT code, AM_INTPTR ptr)
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

AM_ERR CGCoreAVCDecoder::ReSet()
{
    return ME_OK;
}

void CGCoreAVCDecoder::Dump()
{
    AM_INFO("       {---FFMpeg Decoder---}\n");
    AM_INFO("           State->%d, \n", mState);
    //AM_INFO("           BufferPool->Input Buffer Cnt:%d, Audio Decoded Bp(%p) Cnt:%d\n", mpInputQ->GetDataCnt(), mpAudioQ, mpAudioQ->GetDataCnt());
    AM_INFO("           Ptr Dump->Input Buffer Ptr:%p, Owner Ptr:%p\n", mpInputQ, mpQOwner);
    //CQueue* cmdQ = MsgQ();
    //cmdQ->Dump("           CGCoreAVCDecoder Q");
}
