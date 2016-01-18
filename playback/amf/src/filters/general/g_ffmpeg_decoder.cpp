/*
 * gaudio_decoder_ffmpeg.cpp
 *
 * History:
 *    2012/4/6 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "g_ffmpeg_decoder"
//#define AMDROID_DEBUG

//#define USE_LIBAACDEC

#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

extern "C" {
#define INT64_C
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "aac_audio_dec.h"
}
#include "audio_if.h"
#include "g_ffmpeg_decoder.h"


//-----------------------------------------------------------------------
//
// CGDecoderFFMpeg
//
//-----------------------------------------------------------------------

#ifdef USE_LIBAACDEC
static au_aacdec_config_t nvr_aacdec_config;
AM_U32 CGDecoderFFMpeg::mpDecMem[106000];
AM_U8 CGDecoderFFMpeg::mpDecBackUp[252];
AM_U8 CGDecoderFFMpeg::mpInputBuf[16384];
AM_U32 CGDecoderFFMpeg::mpOutBuf[8192];
#endif

AM_INT CGDecoderFFMpeg::mConfigIndex = 0;
AM_INT CGDecoderFFMpeg::mStreamIndex = 0;
STREAM_TYPE CGDecoderFFMpeg::mType = STREAM_AUDIO;
CQueue* CGDecoderFFMpeg::mpDataQ = NULL;

GDecoderParser gDecoderFFMpeg = {
    "PureSoft-FFMpeg-Decoder",
    CGDecoderFFMpeg::Create,
    CGDecoderFFMpeg::ParseBuffer,
    CGDecoderFFMpeg::ClearParse,
};

//
AM_INT CGDecoderFFMpeg::ParseBuffer(const CGBuffer* gBuffer)
{
    //(const_cast<CGBuffer*>(gBuffer))->Dump("ParseBuffer");
    AM_INT score = 0;

    STREAM_TYPE type = gBuffer->GetStreamType();
    if(type == STREAM_VIDEO)
        return -90;

    //if(type == STREAM_AUDIO && *(gBuffer->codecType) == GUID_AmbaAudioHW)
       //return -90;
    OWNER_TYPE otype = gBuffer->GetOwnerType();
    AM_ASSERT(otype == DEMUXER_FFMPEG);
    if(otype != DEMUXER_FFMPEG)
        return -70;


    mpDataQ = (CQueue* )(gBuffer->GetExtraPtr());
    if(mpDataQ == NULL)
        return -100;

    mConfigIndex  = gBuffer->GetIdentity();
    mStreamIndex = gBuffer->ffmpegStream;
    mType = type;
    score = 92;
    AM_INFO("CGDecoderFFMpeg, Score:%d, type(%d)\n", score, type);
    return score;
}

AM_ERR CGDecoderFFMpeg::ClearParse()
{
    return ME_OK;
}
//-----------------------------------------------------------------------
//
// CGDecoderFFMpeg
IGDecoder* CGDecoderFFMpeg::Create(IFilter* pFilter, CGConfig* pconfig)
{
    CGDecoderFFMpeg* result = new CGDecoderFFMpeg(pFilter, pconfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
        }
    return result;
}

CGDecoderFFMpeg::CGDecoderFFMpeg(IFilter* pFilter, CGConfig* pConfig):
    inherited("CGDecoderFFMpeg"),
    mpFilter(pFilter),
    mpGConfig(pConfig),
    mpAVFormat(NULL),
    mpStream(NULL),
    mpCodec(NULL),
    mpDecoder(NULL),
    mSentBufferCnt(0),
    mBufferCnt(0),
    mConsumeCnt(0),
    mbIsEOS(AM_FALSE),
    mpAudioQ(NULL),
    mpVideoQ(NULL),
    mpInputQ(NULL),
    mpQOwner(NULL),
    mpAudioConvert(NULL)
{
    mbRun = true;
    mpConfig = &(pConfig->decoderConfig[mConfigIndex]);
    mConIndex = mConfigIndex;
    mStreamType = mType;
}

AM_ERR CGDecoderFFMpeg::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    if(mStreamType == STREAM_AUDIO){
        if ((mpTempMianQ = CQueue::Create(NULL, this, sizeof(AM_INT), 1)) == NULL)
            return ME_NO_MEMORY;
        if ((mpAudioQ = CQueue::Create(mpTempMianQ, this, sizeof(CGBuffer), GDECODER_FFMPEG_AUDIO_QNUM)) == NULL)
            return ME_NO_MEMORY;
    }

    mpAVFormat = (AVFormatContext* )mpConfig->decoderEnv;
    mpStream = mpAVFormat->streams[mStreamIndex];
    mpCodec = mpStream->codec;
#ifdef USE_LIBAACDEC
    AM_INFO("use libaacdec to decoder aac audio!!\n");
    err = ConstructAACdec();
#else
    err = ConstructFFMpeg();
#endif
    if (err != ME_OK)
    {
        AM_ERROR("CGDecoderFFMpeg ,ConstructFFMpeg fail err = %d .\n", err);
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

AM_ERR CGDecoderFFMpeg::ConstructFFMpeg()
{
    if(mStreamType == STREAM_AUDIO){
        if(!(mpGConfig->globalFlag & NO_AUTO_RESAMPLE_AUDIO) && (mpCodec->channels != G_A_CHANNEL || mpCodec->sample_rate != G_A_SAMPLERATE ||
            mpCodec->sample_fmt != G_A_SAMPLEFMT))
        {
            AM_INFO("Audio will be Convert to %d, %d, %d. Source:%d, %d, %d\n", G_A_CHANNEL, G_A_SAMPLEFMT, G_A_SAMPLERATE,
                mpCodec->channels, mpCodec->sample_fmt, mpCodec->sample_rate);
            mpAudioConvert = av_audio_resample_init(G_A_CHANNEL, mpCodec->channels, G_A_SAMPLERATE, mpCodec->sample_rate,
                G_A_SAMPLEFMT, mpCodec->sample_fmt, 16, 10, 0, 0.8);
            if(mpAudioConvert == NULL) {
                AM_ERROR("Malloc AudioConvert Fialed!\n");
                return ME_ERROR;
            }
        }
        //only for time consuming.
        mSampleRate = (mpCodec->sample_rate <= 0) ? 48000 : mpCodec->sample_rate;
        mpDecoder = avcodec_find_decoder(mpCodec->codec_id);
        if(mpDecoder == NULL) {
            AM_ERROR("ffmpeg audio decoder not found, mpCodec->codec_id=%x. \n",mpCodec->codec_id);
            return ME_ERROR;
        }
    }else if(mStreamType == STREAM_VIDEO){
        //TODO
        AM_ASSERT(0);
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

void CGDecoderFFMpeg::ClearQueue(CQueue* queue)
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
        if(mStreamType == STREAM_AUDIO){
            AM_ASSERT(queue == mpAudioQ);
            AM_ASSERT(mConsumeCnt == buffer.GetCount());
            if(mConsumeCnt != buffer.GetCount())
                AM_INFO("Diff: %d, %d\n",mConsumeCnt, buffer.GetCount());
            mConsumeCnt++;
            AM_U16* ptr = (AM_U16* )(buffer.GetExtraPtr());
            delete[] ptr;
        }else{
        }
        continue;
    }
}

void CGDecoderFFMpeg::Delete()
{
    AM_INFO("CGDecoderFFMpeg::Delete().\n");
    ClearQueue(mpAudioQ);
    ClearQueue(mpVideoQ);

    mpInputQ->Detach();
    mpAudioQ->Attach(mpTempMianQ);
    if(mpCodec){
        avcodec_close(mpCodec);
    }
    if(mpAudioConvert){
        audio_resample_close(mpAudioConvert);
    }

#ifdef USE_LIBAACDEC
     aacdec_close();
#endif
    AM_DELETE(mpAudioQ);
    AM_DELETE(mpVideoQ);
    AM_DELETE(mpTempMianQ);
    inherited::Delete();
}

CGDecoderFFMpeg::~CGDecoderFFMpeg()
{
    AM_INFO("~CGDecoderFFMpeg.\n");
    AM_INFO("~CGDecoderFFMpeg done.\n");
}
//--------------------------------------------------------------------
//
//
//--------------------------------------------------------------------
AM_ERR CGDecoderFFMpeg::PerformCmd(CMD& cmd, AM_BOOL isSend)
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

AM_ERR CGDecoderFFMpeg::DoPause()
{
    if(mState == STATE_PENDING || mState == STATE_ERROR){
        AM_ASSERT(0);
        return ME_OK;
    }
    mState = STATE_PENDING;
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::DoResume()
{
    AM_ASSERT(mState == STATE_PENDING);
    if(mBuffer.GetBufferType() == NOINITED_BUFFER){
        mState = STATE_IDLE;
    }else{
        mState = STATE_HAS_INPUTDATA;
    }
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::DoFlush()
{
    AM_ASSERT(mState == STATE_PENDING);

    ClearQueue(mpAudioQ);
    ClearQueue(mpVideoQ);
    if(mBuffer.GetBufferType() != NOINITED_BUFFER){
        ((IGDemuxer*)mpQOwner)->OnReleaseBuffer(&mBuffer);
        mBuffer.Clear();
    }
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::DoStop()
{
    if(mBuffer.GetBufferType() != NOINITED_BUFFER){
        ((IGDemuxer*)mpQOwner)->OnReleaseBuffer(&mBuffer);
        mBuffer.Clear();
    }
    mbRun = false;
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::DoIsReady()
{
    //mBuffer.Dump("DoIsReady");
    if(mBuffer.GetStreamType() == STREAM_AUDIO)
    {
        if(mpAudioQ->GetDataCnt() >= (GDECODER_FFMPEG_AUDIO_QNUM - 10))
            return ME_BUSY;
    }
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::IsReady(CGBuffer* pBuffer)
{
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGDecoderFFMpeg::ProcessCmd %d.\n", cmd.code);
    AM_ERR err = ME_OK;
    switch (cmd.code)
    {
    case CMD_STOP:
        AM_INFO("CVideoDecoderDsp Process CMD_STOP.\n");
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
        err = DoDecode();
        mState = STATE_IDLE;
        if(err == ME_OK && mStreamType == STREAM_VIDEO){
            SendFilterMsg(GFilter::MSG_DECODED, (AM_INTPTR)&mOBuffer);//wakeup isready and send cbuffer. make sure sended and then ack
        }else{
            //all send here:1. general will wakeup and test isready and to decode. 2. note this decode is done by dodecode which will block until here return.
            //AM_INFO("show: %d, %u\n", mSentBufferCnt - mConsumeCnt , mSentBufferCnt - mConsumeCnt);
            if((mSentBufferCnt - mConsumeCnt) < 100){
                    //AM_INFO("11111\n");
                SendFilterMsg(GFilter::MSG_DECODED, 0);
            }
        }
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

void CGDecoderFFMpeg::OnRun()
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
    AM_INFO("CGDecoderFFMpeg OnRun Exit.\n");
}
//------------------------------------------------------
//
//
//------------------------------------------------------
inline AM_ERR CGDecoderFFMpeg::FillHandleBuffer()
{
    mOBuffer.SetOwnerType(DECODER_FFMEPG);
    mOBuffer.SetBufferType(HANDLE_BUFFER);
    mOBuffer.SetIdentity(mConIndex);
    if(mStreamType == STREAM_AUDIO)
    {
        if(mpAudioConvert != NULL){
            mOBuffer.audioInfo.numChannels = G_A_CHANNEL;
            mOBuffer.audioInfo.sampleFormat = IAudioHAL::FORMAT_S16_LE;
            mOBuffer.audioInfo.sampleRate = G_A_SAMPLERATE;
        }else{
            mOBuffer.audioInfo.numChannels = mpCodec->channels;
            mOBuffer.audioInfo.sampleFormat = IAudioHAL::FORMAT_S16_LE;
            mOBuffer.audioInfo.sampleRate = mpCodec->sample_rate;
        }
        mOBuffer.SetStreamType(STREAM_AUDIO);
    }else{
        mOBuffer.SetStreamType(STREAM_VIDEO);
    }
    mOBuffer.SetCount(0);
    mOBuffer.SetExtraPtr((AM_INTPTR)mpAudioQ);
    mOBuffer.SetGBufferPtr((AM_INTPTR)this);
    //AM_INFO("111111111111111:%d\n", (AM_INTPTR)this);
    mpAudioQ->Detach();
    SendFilterMsg(GFilter::MSG_DECODED, (AM_INTPTR)&mOBuffer);//wakeup isready and send cbuffer. make sure sended and then ack
    mOBuffer.Clear();
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::DoDecode()
{
    //mpBuffer->Dump("AudioDecode");
    AM_ERR err=ME_OK;
    STREAM_TYPE type = mBuffer.GetStreamType();
    AM_ASSERT(type == mStreamType);
    switch(mStreamType)
    {
    case STREAM_VIDEO:
        err = DecodeVideo();
        break;

    case STREAM_AUDIO:

#ifdef USE_LIBAACDEC
        err = DecodeAudioAACdec();
#else
        err = DecodeAudio();
#endif
        break;

    default:
        AM_ASSERT(0);
    }
    AM_VERBOSE("err=%d\n", err);
    ((IGDemuxer*)mpQOwner)->OnReleaseBuffer(&mBuffer);
    mBuffer.Clear();
    GenerateGBuffer();
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::DecodeVideo()
{
    AM_ASSERT(0);
    return ME_OK;
}

//One Packet may contains more than one sample. Q these.
AM_ERR CGDecoderFFMpeg::DecodeAudio()
{
    //AM_INFO("DecodeAudio\n");
//    AM_ERR err;
    AM_INT sampleSize = MAX_AUDIO_SAMPLE_SIZE_8, usedSize = 0;
    AM_U8* saveData;
    AM_S16* pSample;
    AM_INT saveSize;
    AVPacket* pPacket = (AVPacket* )(mBuffer.GetExtraPtr());
    AM_ASSERT(pPacket != NULL);
    saveData = pPacket->data;
    saveSize = pPacket->size;

    if(mpAudioConvert == NULL){
        pSample = new AM_S16[MAX_AUDIO_SAMPLE_SIZE_16];
    }else{
        pSample = mAudioSample;
    }

    while(1)
    {
        usedSize = avcodec_decode_audio3(mpCodec, pSample, &sampleSize, pPacket);
        if(usedSize <= 0 || usedSize > pPacket->size || sampleSize <= 0){
            AM_ERROR("DecodeAudio Failed\n");
            if(mpAudioConvert == NULL)
                delete[] pSample;
            break;
        }
        //AM_INFO("Done usedSize:%d, packet size:%d, samplesize:%d\n", usedSize, pPacket->size, sampleSize);
        AM_ASSERT(sampleSize < MAX_AUDIO_SAMPLE_SIZE_8);

        UpdateAudioDuration(sampleSize);
        FillAudioSampleQueue(pSample, sampleSize);
        if(usedSize < pPacket->size)
        {
            pPacket->data += usedSize;
            pPacket->size -= usedSize;
            //pSample = NULL;
            sampleSize = usedSize = 0;
            continue;
        }
        AM_ASSERT(usedSize == pPacket->size);
        break;
    }
    pPacket->data = saveData;
    pPacket->size = saveSize;
    return ME_OK;
}

inline AM_ERR CGDecoderFFMpeg::FillAudioSampleQueue(AM_S16* sample, AM_INT size)
{
    AM_S16* pSample;
    AM_INT sampleSize;
    AM_INT sampleNum;
    if(mpAudioConvert != NULL){
        pSample = new AM_S16[MAX_AUDIO_SAMPLE_SIZE_16];
        sampleNum = audio_resample(mpAudioConvert, pSample, mAudioSample, GetAudioSampleNum(size));
        if(sampleNum == 0)
            AM_ASSERT(0);
        sampleSize = sampleNum * G_A_CHANNEL * 2;
    }else{
        pSample = sample;
        sampleSize = size;
    }

    if(mpAudioQ->GetDataCnt() >= GDECODER_FFMPEG_AUDIO_QNUM){
        AM_ASSERT(0);
        //return ME_TOO_MANY;
     }

    CGBuffer gBuffer;
    gBuffer.SetOwnerType(DECODER_FFMEPG);
    gBuffer.SetBufferType(DATA_BUFFER);
    gBuffer.SetIdentity(mConIndex);
    gBuffer.SetStreamType(STREAM_AUDIO);

    gBuffer.audioInfo.numChannels = G_A_CHANNEL;
    gBuffer.audioInfo.sampleFormat = G_A_SAMPLEFMT;
    gBuffer.audioInfo.sampleRate = G_A_SAMPLERATE;
    gBuffer.SetExtraPtr((AM_INTPTR)pSample);
    //gBuffer.SetGBufferPtr((AM_INTPTR)this);
    gBuffer.audioInfo.sampleSize = sampleSize;
    gBuffer.SetPTS(mCurPtsA);

    mBufferCnt++;
    gBuffer.SetCount(mBufferCnt);
    mpAudioQ->PutData(&gBuffer, sizeof(CGBuffer));
    return ME_OK;
}

inline AM_INT CGDecoderFFMpeg::GetAudioSampleNum(AM_INT size)
{
    AM_INT sampleNum = 0;
    switch(mpCodec->sample_fmt)
    {
    case SAMPLE_FMT_U8:
        sampleNum = size / mpCodec->channels;
        break;

    case SAMPLE_FMT_S16:
        sampleNum = (size / mpCodec->channels) >> 1;
        break;

    case SAMPLE_FMT_S32:
    case SAMPLE_FMT_FLT:
        sampleNum = (size / mpCodec->channels) >> 2;
        break;

    case SAMPLE_FMT_DBL:
        sampleNum = (size / mpCodec->channels) >> 3;
        break;

    default:
        AM_ERROR("Handle me\n");
        sampleNum = 0;
    }
    return sampleNum;
}

inline AM_ERR CGDecoderFFMpeg::UpdateAudioDuration(AM_INT size)
{
    AM_INT sampleNum = GetAudioSampleNum(size);

    mTotalSampleA += sampleNum;
    mDurationA += (sampleNum * TICK_PER_SECOND) /mSampleRate;
    //DIFF mAudioDruation ----pPacket->pts
    mCurPtsA = mDurationA;
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::GenerateGBuffer()
{
    //no send out since queue the sample
    if(mStreamType == STREAM_AUDIO)
        return ME_OK;
    AM_ASSERT(0);
    mOBuffer.Clear();
    mOBuffer.SetOwnerType(DECODER_FFMEPG);
    mOBuffer.SetBufferType(DATA_BUFFER);
    mOBuffer.SetStreamType(STREAM_VIDEO);
    mOBuffer.SetIdentity(mConIndex);
    mOBuffer.SetPTS(mBuffer.GetPTS());
    //get_time_ticks_from_dsp
    //mOBuffer.general();
    //mpDsp->GetSendBuffer(&mOBuffer);
    //mpVideoOutputPin->SendBuffer(pBuffer);
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::GetDecodedGBuffer(CGBuffer& oBuffer)
{
    AM_ASSERT(mStreamType == STREAM_AUDIO);
    AM_BOOL rval;
    //AM_INFO("Data Cnt: %d\n", mpAudioQ->GetDataCnt());
    rval = mpAudioQ->PeekData(&oBuffer, sizeof(CGBuffer));
    if(rval == AM_FALSE){
        //no data
        return ME_ERROR;
    }
    mSentBufferCnt++;
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::OnReleaseBuffer(CGBuffer* buffer)
{
    //AM_INFO("CC1   OnReleaseBuffer\n");
    AM_ASSERT(mConsumeCnt == buffer->GetCount());
    if(mConsumeCnt != buffer->GetCount())
        AM_INFO("Diff: %d, %d\n",mConsumeCnt, buffer->GetCount());
    mConsumeCnt++;

    if(buffer->GetBufferType() != DATA_BUFFER)
        return ME_OK;

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
        //NO input to drive the sending
        //if(mpAudioQ->GetDataCnt() > 0)
            //SendFilterMsg(GFilter::MSG_DECODED, 0);
    }
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::FillEOS()
{
    AM_INFO("EOS Filling on CGDecoderFFMpeg\n");
    if(mpAudioQ->GetDataCnt() >= GDECODER_FFMPEG_AUDIO_QNUM){
        AM_ASSERT(0);
        return ME_TOO_MANY;
     }
    CGBuffer gBuffer;
    gBuffer.SetBufferType(EOS_BUFFER);
    gBuffer.SetOwnerType(DECODER_FFMEPG);
    gBuffer.SetIdentity(mConIndex);
    gBuffer.SetStreamType(STREAM_AUDIO);

    mBufferCnt++;
    gBuffer.SetCount(mBufferCnt);
    //SendFilterMsg(GFilter::MSG_DECODED, (AM_INTPTR)&gBuffer); //two way's eos notify is nonecessary and complex.
    mpAudioQ->PutData(&gBuffer, sizeof(CGBuffer));
    mbIsEOS = AM_TRUE;
    ((IGDemuxer*)mpQOwner)->OnReleaseBuffer(&mBuffer);
    mBuffer.Clear();

    return ME_OK;
}

void CGDecoderFFMpeg::ProcessEOS()
{
    /*
    //if video format with CODEC_CAP_DELAY , get out all frames left then send eos out
    if (mpDecoder->type == CODEC_TYPE_VIDEO && (mpCodec->codec->capabilities & CODEC_CAP_DELAY))
    {
        AM_INT frame_finished, ret;
        AVPacket packet;
        packet.data = NULL;
        packet.size = 0;
        packet.flags = 0;

            AMLOG_INFO("CGDecoderFFMpeg[video] eos wait left frames out start.\n");
            while(1)
            {
                AMLOG_INFO("CGDecoderFFMpeg[video] eos wait left frams out......\n");

            ret = avcodec_decode_video2(mpCodec, &mFrame, &frame_finished, &packet);

            if(0>=ret)//all frames out
                break;

            if (frame_finished) {
                CBuffer *pOutputBuffer;
                if (mbNV12) {
                    pOutputBuffer = (CBuffer *)(mFrame.opaque);
                    AM_ASSERT(pOutputBuffer);
                    if(pOutputBuffer)
                    {
                        if (mbParallelDecoder == true) {
                            pOutputBuffer->mPTS = mFrame.pts;
                        }
                        mpOutput->SendBuffer(pOutputBuffer);
                     }
                } else {
                    if (!mpBufferPool->AllocBuffer(pOutputBuffer, 0)) {
                        AMLOG_INFO("%p, get output buffer fail, exit.\n", this);
                        //mbRun = false;
                        msState = STATE_PENDING;
                        return;
                    }
                    pOutputBuffer->mPTS = mFrame.reordered_opaque;
                    CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pOutputBuffer;
                    ConvertFrame(pVideoBuffer);
                    mpOutput->SendBuffer(pOutputBuffer);
                }
                decodedFrame ++;
                AMLOG_VERBOSE("CGDecoderFFMpeg[video] eos send a decoded frame done.\n");
            }
        }
        AMLOG_INFO("CGDecoderFFMpeg[video] eos wait left frames out done.\n");
    }

    //if not video format, send eos out directly
    SendEOS(mpOutput);
    mpBuffer->Release();
    mpBuffer = NULL;
    AMLOG_INFO("CGDecoderFFMpeg send EOS done.\n");
    */
}

/*
bool CGDecoderFFMpeg::SendEOS()
{
    return true;
}
*/
//===========================================
//
//===========================================
AM_ERR CGDecoderFFMpeg::PostFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    AM_INT size = 0;
    if(mStreamType == STREAM_AUDIO){
        size = mpAudioQ->GetDataCnt();
    }
    msg.p1 = size;
    msg.p2 = mConIndex;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->PostMsgToFilter(msg);
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::SendFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    AM_INT size = 0;
    if(mStreamType == STREAM_AUDIO){
        size = mpAudioQ->GetDataCnt();
    }
    msg.p1 = size;
    msg.p2 = mConIndex;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->SendMsgToFilter(msg);
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::ReSet()
{
    return ME_OK;
}

void CGDecoderFFMpeg::Dump()
{
    AM_INFO("       {---FFMpeg Decoder---}\n");
    AM_INFO("           State->%d, mConIndex: %d.\n", mState, mConIndex);
    AM_INFO("           BufferPool->Input Buffer Cnt:%d, Audio Decoded Bp(%p) Cnt:%d\n", mpInputQ->GetDataCnt(), mpAudioQ, mpAudioQ->GetDataCnt());
    AM_INFO("           Ptr Dump->Input Buffer Ptr:%p, Owner Ptr:%p\n", mpInputQ, mpQOwner);
    //CQueue* cmdQ = MsgQ();
    //cmdQ->Dump("           CGDecoderFFMpeg Q");
}
//-----------------------------------------------------------------------
//
// Video decoder related
//
//-----------------------------------------------------------------------
/*
void CGDecoderFFMpeg::ProcessVideo()
{
     AM_ASSERT(msState == STATE_READY);

    AM_INT frame_finished, ret;

    //get from AV_NOPTS_VALUE(avcodec.h), use (mpPacket->pts!=AV_NOPTS_VALUE) will have warnings, use (mpPacket->pts >= 0) judgement instead
    //convert to unit ms
    if (!mbParallelDecoder) {
        if (mpPacket->pts >= 0) {
            mCurrInputTimestamp = mpPacket->pts;
            AMLOG_PTS("CGDecoderFFMpeg[video] recieve a PTS, mpPacket->pts=%llu.\n",mpPacket->pts);
        }
        else {
            mCurrInputTimestamp = 0;
            AMLOG_WARN("CGDecoderFFMpeg[video] has no pts.\n");
        }
        mpCodec->reordered_opaque = mCurrInputTimestamp;
    }

    //for bug917/921, hy pts issue
    if(mpAccelerator && mpSharedRes)
    {
        if(mpAccelerator->video_ticks!=mpSharedRes->mVideoTicks)
        {
            mpAccelerator->video_ticks = mpSharedRes->mVideoTicks;
            AMLOG_DEBUG("now mpAccelerator->video_ticks = %d.\n", mpAccelerator->video_ticks);
        }
    }

    AMLOG_VERBOSE("CGDecoderFFMpeg[video] decoding start.\n");

#ifdef __use_hardware_timmer__
    time_count_before_dec = AM_get_hw_timer_count();
    ret = avcodec_decode_video2(mpCodec, &mFrame, &frame_finished, mpPacket);
    time_count_after_dec = AM_get_hw_timer_count();
    totalTime += AM_hwtimer2us(time_count_before_dec - time_count_after_dec);
#else
    struct timeval tvbefore, tvafter;
    gettimeofday(&tvbefore,NULL);
    ret = avcodec_decode_video2(mpCodec, &mFrame, &frame_finished, mpPacket);
    gettimeofday(&tvafter,NULL);
    totalTime += (tvafter.tv_sec -tvbefore.tv_sec)*1000000 + tvafter.tv_usec - tvbefore.tv_usec;
#endif

    AMLOG_VERBOSE("CGDecoderFFMpeg[video] decoding done.\n");

    if (!mbStreamStart) {
        pthread_mutex_lock(&mpSharedRes->mMutex);
        GetUdecState(mpDSPHandler->mIavFd, &mpSharedRes->udec_state, &mpSharedRes->vout_state, &mpSharedRes->error_code);
        if (IAV_UDEC_STATE_RUN == mpSharedRes->udec_state) {
            PostEngineMsg(IEngine::MSG_NOTIFY_UDEC_IS_RUNNING);
            mbStreamStart = true;
        }
        pthread_mutex_unlock(&mpSharedRes->mMutex);
    }

    if (!(decodedFrame & 0x1f) && decodedFrame)
        AMLOG_PERFORMANCE("video [%d] decoded [%d] dropped frames' average = %d uS  \n", decodedFrame, droppedFrame, totalTime/(decodedFrame+droppedFrame));

    if (ret < 0) {
        AMLOG_ERROR("--- while decoding video---, err=%d.\n",ret);
        droppedFrame ++;
    } else if (frame_finished) {
        CBuffer *pOutputBuffer;
        if (mbNV12) {
            pOutputBuffer = (CBuffer *)(mFrame.opaque);
            AM_ASSERT(pOutputBuffer);
            if(pOutputBuffer)
            {
                if (mbParallelDecoder == true) {
                    pOutputBuffer->mPTS = mFrame.pts;
                }
                mpOutput->SendBuffer(pOutputBuffer);
            }
        } else {

            //chech valid, for sw decoding and convert from yuv420p to nv12 format
            //if (!mFrame.data[0] || !mFrame.data[1] || !mFrame.data[2] *|| !mFrame.linesize[0] || !mFrame.linesize[1] || !mFrame.linesize[2] ) {
            if (!mFrame.data[0] || !mFrame.linesize[0] ) {
                AM_ASSERT(0);
                AM_ERROR("get not valid frame, not enough memory? or not supported pix_fmt %d?.\n", mpCodec->pix_fmt);
                msState = STATE_HAS_OUTPUTBUFFER;
                mpBuffer->Release();
                mpBuffer = NULL;
                mpPacket = NULL;
                return;
            }

            if (!mpBufferPool->AllocBuffer(pOutputBuffer, 0)) {
                AMLOG_INFO("%p, get output buffer fail, exit.\n", this);
                //mbRun = false;
                msState = STATE_PENDING;
                return;
            }
            pOutputBuffer->mPTS = mFrame.reordered_opaque;
            CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pOutputBuffer;
            ConvertFrame(pVideoBuffer);
            mpOutput->SendBuffer(pOutputBuffer);
        }
        decodedFrame ++;
        AMLOG_VERBOSE("CGDecoderFFMpeg[video] send a decoded frame done.\n");
    }

    AM_ASSERT(mpBuffer);
    mpBuffer->Release();
    mpBuffer = NULL;
    mpPacket = NULL;
    msState = STATE_IDLE;
}
*/

/*
AM_ERR CGDecoderFFMpegOutput::SetOutputFormat()
{
    AVCodec *pDecoder = ((CGDecoderFFMpeg*)mpFilter)->mpDecoder;
    AVCodecContext *pCodec = ((CGDecoderFFMpeg*)mpFilter)->mpCodec;

    if (pDecoder->type == CODEC_TYPE_VIDEO) {
        mMediaFormat.pMediaType = &GUID_Decoded_Video;
        mMediaFormat.pSubType = &GUID_Video_YUV420NV12;
        mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Media;
        mMediaFormat.mDspIndex = ((CGDecoderFFMpeg*)mpFilter)->mDspIndex;
        CalcVideoDimension(pCodec);
        return ME_OK;
    }
    if (pDecoder->type == CODEC_TYPE_AUDIO) {
        mMediaFormat.pMediaType = &GUID_Decoded_Audio;
        //mMediaFormat.pSubType = &GUID_Video_YUV420NV12;
        mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Media;
        mMediaFormat.auSamplerate = pCodec->sample_rate;
        mMediaFormat.auChannels = pCodec->channels;
        mMediaFormat.auSampleFormat = pCodec->sample_fmt;
        mMediaFormat.isChannelInterleave = 0;
        return ME_OK;
    }
    //subtitle stream need new structure. added by cx
    if (pDecoder->type == CODEC_TYPE_SUBTITLE) {
        mMediaFormat.pMediaType = &GUID_Decoded_Subtitle;
        mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Media;
        return ME_OK;
    }

    else {
        AM_ERROR("Cannot decoder other format yet\n");
        return ME_ERROR;
    }
}

void CGDecoderFFMpegOutput::CalcVideoDimension(AVCodecContext *pCodec)
{
    int w = pCodec->width;
    int h = pCodec->height;

    mMediaFormat.picWidth = w;
    mMediaFormat.picHeight = h;

    if (pCodec->codec_id == CODEC_ID_VP8) {
        w += 64;
        h += 64;
        mMediaFormat.picXoff = 32;
        mMediaFormat.picYoff = 32;
        AMLOG_INFO("CODEC_ID_VP8\n");
    }
    else if (!(pCodec->flags & CODEC_FLAG_EMU_EDGE)) {
        w += 32;
        h += 32;
        mMediaFormat.picXoff = 16;
        mMediaFormat.picYoff = 16;
        AMLOG_INFO("CODEC_FLAG_EMU_EDGE\n");
    }
    else {
        mMediaFormat.picXoff = 0;
        mMediaFormat.picYoff = 0;
    }

    mMediaFormat.picWidthWithEdge = w;
    mMediaFormat.picHeightWithEdge = h;

    w=(w+(CGDecoderFFMpeg::VIDEO_BUFFER_ALIGNMENT_WIDTH-1))&(~(CGDecoderFFMpeg::VIDEO_BUFFER_ALIGNMENT_WIDTH-1));
    h=(h+(CGDecoderFFMpeg::VIDEO_BUFFER_ALIGNMENT_HEIGHT-1))&(~(CGDecoderFFMpeg::VIDEO_BUFFER_ALIGNMENT_HEIGHT-1));

    mMediaFormat.bufWidth = w;
    mMediaFormat.bufHeight = h;
}

int CGDecoderFFMpeg::DecoderGetBuffer(AVCodecContext *s, AVFrame *pic)
{
    CBuffer *pBuffer=NULL;
    pic->opaque=NULL;
    CGDecoderFFMpeg *pFilter = (CGDecoderFFMpeg*)s->opaque;
    if(NULL==pFilter)
    {
        AM_WARNING("opaque warning!! %s,%d\n",__FILE__,__LINE__);
        return -EPERM;
    }
    IBufferPool *pBufferPool = pFilter->mpOutput->mpBufferPool;

    //AM_INFO("*****CGDecoderFFMpeg[video] DecoderGetBuffer callback.\n");

    if (!pBufferPool->AllocBuffer(pBuffer, 0)) {
        AM_ASSERT(0);
        return -1;
    }
    pBuffer->AddRef();
    if (!pFilter->mbParallelDecoder)
        pBuffer->mPTS = pFilter->mCurrInputTimestamp;

    CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pBuffer;
    pVideoBuffer->picXoff = pFilter->mpOutput->mMediaFormat.picXoff;
    pVideoBuffer->picYoff = pFilter->mpOutput->mMediaFormat.picYoff;

    //only nv12 format comes here
    if (pVideoBuffer->picXoff > 0) {
        pic->data[0] = (uint8_t*)pVideoBuffer->pLumaAddr +
            pVideoBuffer->picYoff * pVideoBuffer->fbWidth +
            pVideoBuffer->picXoff;
        pic->linesize[0] = pVideoBuffer->fbWidth;

        pic->data[1] = (uint8_t*)pVideoBuffer->pChromaAddr +
            (pVideoBuffer->picYoff / 2) * pVideoBuffer->fbWidth +
            pVideoBuffer->picXoff;
        pic->linesize[1] = pVideoBuffer->fbWidth;

        pic->data[2] = pic->data[1] + 1;
        pic->linesize[2] = pVideoBuffer->fbWidth;
    }
    else {
        AM_ASSERT(!pVideoBuffer->picYoff);
        pic->data[0] = (uint8_t*)pVideoBuffer->pLumaAddr;
        pic->linesize[0] = pVideoBuffer->fbWidth;

        pic->data[1] = (uint8_t*)pVideoBuffer->pChromaAddr;
        pic->linesize[1] = pVideoBuffer->fbWidth;

        pic->data[2] = pic->data[1] + 1;
        pic->linesize[2] = pVideoBuffer->fbWidth;
    }

    pic->age = 256*256*256*64;
    pic->type = FF_BUFFER_TYPE_USER;
    pic->opaque = (void*)pBuffer;
    pic->user_buffer_id = pVideoBuffer->buffer_id;
    //pic->fbp_id = pVideoBuffer->fbp_id;
    return 0;

}

void CGDecoderFFMpeg::DecoderReleaseBuffer(AVCodecContext *s, AVFrame *pic)
{
    CBuffer *pBuffer = (CBuffer*)pic->opaque;
    if(NULL==pBuffer)
    {
        AM_WARNING("opaque warning!! %s,%d\n",__FILE__,__LINE__);
        return;
    }
    pBuffer->Release();
    if (pic->type == DFlushing_frame) {
        //flushing, CBuffer will not send to video_renderer for render/release, so release here
        pBuffer->Release();
    }

    pic->data[0] = NULL;
    pic->type = 0;
}
*/

#ifdef USE_LIBAACDEC
///////////////////////////////////////////////////////
static void Audio32iTo16i(AM_S32* bufin, AM_S16* bufout, AM_S32 ch, AM_S32 proc_size)
{
  AM_S32 i,j;
  AM_S32* bufin_ptr;
  AM_S16* bufout_ptr;

  bufin_ptr = bufin;
  bufout_ptr = bufout;
  for( i = 0 ; i < proc_size ; i++ ) {
    for( j = 0 ; j < ch ; j++ ) {
      *bufout_ptr = (*bufin_ptr)>>16;
      bufin_ptr++;
      bufout_ptr++;
    }
  }
}

///////////////////////////////////////////////////////
AM_ERR CGDecoderFFMpeg::ConstructAACdec()
{
    if(mStreamType == STREAM_AUDIO){
        if(!(mpGConfig->globalFlag & NO_AUTO_RESAMPLE_AUDIO) && (mpCodec->channels != G_A_CHANNEL || mpCodec->sample_rate != G_A_SAMPLERATE ||
            mpCodec->sample_fmt != G_A_SAMPLEFMT))
        {
            AM_INFO("Audio will be Convert to %d, %d, %d. Source:%d, %d, %d\n", G_A_CHANNEL, G_A_SAMPLEFMT, G_A_SAMPLERATE,
                mpCodec->channels, mpCodec->sample_fmt, mpCodec->sample_rate);
            mpAudioConvert = av_audio_resample_init(G_A_CHANNEL, mpCodec->channels, G_A_SAMPLERATE, mpCodec->sample_rate,
                G_A_SAMPLEFMT, mpCodec->sample_fmt, 16, 10, 0, 0.8);
            if(mpAudioConvert == NULL) {
                AM_ERROR("Malloc AudioConvert Fialed!\n");
                return ME_ERROR;
            }
        }
        //only for time consuming.
        mSampleRate = (mpCodec->sample_rate <= 0) ? 48000 : mpCodec->sample_rate;

        InitHWAttr();

        SetupHWDecoder();
    }else if(mStreamType == STREAM_VIDEO){
        //TODO
        AM_ASSERT(0);
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::InitHWAttr()
{
    nvr_aacdec_config.externalSamplingRate = 48000;         /* !< external sampling rate for raw decoding */
    nvr_aacdec_config.bsFormat	= ADTS_BSFORMAT; /* !< ADTS_BSFORMAT : default Bitstream Type */
    nvr_aacdec_config.outNumCh            = 2;
    nvr_aacdec_config.externalBitrate      = 0;             /* !< external bitrate for loading the input buffer */
    nvr_aacdec_config.frameCounter         = 0;
    nvr_aacdec_config.errorCounter         = 0;
    nvr_aacdec_config.interBufSize         = 0;
    nvr_aacdec_config.codec_lib_mem_addr = (AM_U32* )mpDecMem; //libaac work mem.
    nvr_aacdec_config.codec_lib_backup_addr = (AM_U8* )mpDecBackUp; //multi way decoder support
    nvr_aacdec_config.codec_lib_backup_size = 0;
    nvr_aacdec_config.consumedByte = 0;
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::SetupHWDecoder()
{
    aacdec_setup(&nvr_aacdec_config);
    aacdec_open(&nvr_aacdec_config);

    if (nvr_aacdec_config.ErrorStatus != 0)
    {
        AM_INFO("SetupHWDecoder Failed!!!\n\n");
    }
    nvr_aacdec_config.dec_rptr =  (AM_U8 *)mpInputBuf;
    return ME_OK;
}

//One Packet may contains more than one sample. Q these.
AM_ERR CGDecoderFFMpeg::DecodeAudioAACdec()
{
    //AM_INFO("DecodeAudioAACdec\n");
    AM_ERR err;
    AM_INT sampleSize = MAX_AUDIO_SAMPLE_SIZE_8, usedSize = 0;
    AM_U8* saveData;
    AM_S16* pSample;
    AM_INT saveSize;

    AM_UINT inSize, outSize;
    AVPacket* pPacket = (AVPacket* )(mBuffer.GetExtraPtr());
    AM_ASSERT(pPacket != NULL);
    saveData = pPacket->data;
    saveSize = pPacket->size;

    if(mpAudioConvert == NULL){
        pSample = new AM_S16[MAX_AUDIO_SAMPLE_SIZE_16];
    }else{
        pSample = mAudioSample;
    }

    nvr_aacdec_config.dec_wptr = (AM_S32 *)mpOutBuf;

    if(pPacket->data[0] != 0xff || (pPacket->data[1] |0x0f) != 0xff){
        AddAdtsHeader(pPacket);
        nvr_aacdec_config.interBufSize = inSize = pPacket->size + 7;
    }else{
        WriteBitBuf(pPacket, 0);
        nvr_aacdec_config.interBufSize = inSize = pPacket->size;
    }

    //while(1)
    //{
        aacdec_set_bitstream_rp(&nvr_aacdec_config);
        aacdec_decode(&nvr_aacdec_config);

        if(nvr_aacdec_config.ErrorStatus != 0){
            AM_ERROR("DecodeAudio Failed\n");
            pPacket->data = saveData;
            pPacket->size = saveSize;
            return ME_ERROR;
        }
        AM_ASSERT(nvr_aacdec_config.consumedByte == inSize);
        AM_ASSERT((nvr_aacdec_config.frameCounter - mFrameNum) == 1);
        mFrameNum = nvr_aacdec_config.frameCounter;

        //length donot change, just 32 high 16bits to 16.
        Audio32iTo16i(nvr_aacdec_config.dec_wptr, pSample, nvr_aacdec_config.srcNumCh, nvr_aacdec_config.frameSize);
        sampleSize = nvr_aacdec_config.frameSize * nvr_aacdec_config.srcNumCh * 2; //THIS size is bytes!!!!
        //AM_INFO("sampleSize %d, inSize %d, pPacket->size  %d\n", sampleSize, inSize, pPacket->size);

        UpdateAudioDuration(sampleSize);
        FillAudioSampleQueue(pSample, sampleSize);
    //}
    pPacket->data = saveData;
    pPacket->size = saveSize;
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::WriteBitBuf(AVPacket* pPacket, int offset)
{
    memcpy(mpInputBuf+offset, pPacket->data, pPacket->size);
    return ME_OK;
}

AM_ERR CGDecoderFFMpeg::AddAdtsHeader(AVPacket* pPacket)
{
    AM_UINT size = pPacket->size +7;
    AM_U8 adtsHeader[7];
    adtsHeader[0] = 0xff;
    adtsHeader[1] = 0xf9;
    adtsHeader[2] = 0x4c;
    adtsHeader[3] = 0x80;          //fixed header end, the last two bits is the length .
    adtsHeader[4] = (size>>3)&0xff;
    adtsHeader[5] = (size&0x07)<<5;   //todo
    adtsHeader[6] = 0xfc;  //todo
    for (AM_UINT i=0; i < 7; i++)
    {
        mpInputBuf[i] = adtsHeader[i];
        //hBitBuf->cntBits += 8;
    }
    WriteBitBuf(pPacket, 7);
    return ME_OK;
}
#endif
