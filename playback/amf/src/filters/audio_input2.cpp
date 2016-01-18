/*
 * audio_input.cpp
 *
 * History:
 *    2010/3/18 - [Kaiming Xie] created file
 *    2010/7/6 - [Luo Fei] modified file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_TAG "audio_input2"
//#define AMDROID_DEBUG
//#define RECORD_TEST_FILE

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "record_if.h"
#include "engine_guids.h"

#include "audio_input2.h"

#define DMAX_FADE_CNT 8
static AM_UINT fade_level[DMAX_FADE_CNT] = {6,5,4,3,2,2,1,1};

static void _fade_audio_input_short(AM_S16* pdata, AM_UINT data_size, AM_UINT shift)
{
    while (data_size) {
        data_size --;
        *pdata >>= shift;
        pdata ++;
    }
}


IFilter* CreateAudioInput2(IEngine * pEngine)
{
    return CAudioInput2::Create(pEngine);
}

IFilter* CAudioInput2::Create(IEngine * pEngine)
{
    CAudioInput2 * result = new CAudioInput2(pEngine);

    if(result && result->Construct() != ME_OK)
    {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CAudioInput2::Construct()
{
    AM_ERR err = inherited::Construct();
    if(err != ME_OK) {
        AM_ERROR("inherited::Construct() fail in CAudioInput2::Construct().\n");
        return err;
    }
    DSetModuleLogConfig(LogModuleAudioInputFilter);

    if ((mpBufferPool = CSimpleBufferPool::Create("AudioInputBuffer", 96, sizeof(CBuffer) + MAX_AUDIO_BUFFER_SIZE) ) == NULL)
        return ME_NO_MEMORY;

    if ((mpOutput = CAudioInput2Output::Create(this)) == NULL)
        return ME_NO_MEMORY;

    mpOutput->SetBufferPool(mpBufferPool);

    mpAudioDriver = AM_CreateAudioHAL(mpEngine, 0, 0);

#ifdef RECORD_TEST_FILE
    mpAFile = CFileWriter::Create();
    if (mpAFile == NULL)
        return ME_ERROR;
    err = mpAFile->CreateFile("/mnt/media/audio_input2.pcm");
    if (err != ME_OK)
    return err;
#endif

    mpReserveBuffer = new AM_U8[MAX_AUDIO_BUFFER_SIZE];
    return ME_OK;
}

CAudioInput2::~CAudioInput2()
{
    AMLOG_DESTRUCTOR("CAudioInput2::~CAudioInput2 start.\n");
    AM_DELETE(mpOutput);
    mpOutput = NULL;
    AMLOG_DESTRUCTOR("CAudioInput2::~CAudioInput2 after AM_DELETE(mpOutput).\n");
    AM_RELEASE(mpBufferPool);
    mpBufferPool = NULL;
    AMLOG_DESTRUCTOR("CAudioInput2::~CAudioInput2 after AM_RELEASE(mpBufferPool).\n");
#ifdef RECORD_TEST_FILE
    AM_DELETE(mpAFile);
    mpAFile = NULL;
#endif
    delete[] mpReserveBuffer;
    AMLOG_DESTRUCTOR("CAudioInput2::~CAudioInput2 start mpAudioDriver->CloseAudioHAL().\n");
    //mpAudioDriver->CloseAudioHAL();
    //mpAudioDriver = NULL;
    AMLOG_DESTRUCTOR("CAudioInput2::~CAudioInput2 end.\n");
}

void CAudioInput2::GetInfo(INFO& info)
{
    info.nInput = 0;
    info.nOutput = 1;
    info.pName = "AudioInput";
}

IPin* CAudioInput2::GetOutputPin(AM_UINT index)
{
    if (index == 0)
        return mpOutput;
    return NULL;
}

AM_ERR CAudioInput2::FlowControl(FlowControlType type)
{
    CMD cmd0;
    AMLOG_WARN("CAudioInput2::FlowControl(%d) start.\n", type);
    cmd0.code = CMD_FLOW_CONTROL;
    cmd0.flag = (AM_U8)type;

    mpWorkQ->MsgQ()->SendMsg((void*)&cmd0, sizeof(cmd0));
    AMLOG_WARN("CAudioInput2::FlowControl(%d) end.\n", type);
    return ME_OK;
}

AM_ERR CAudioInput2::Stop()
{
    AMLOG_INFO("=== CAudioInput2::Stop()\n");
    inherited::Stop();
    return ME_OK;
}

AM_ERR CAudioInput2::SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index)
{
    AM_ASSERT(type == StreamType_Audio);
    AMLOG_DEBUG("&&&& CAudioInput2::SetParameters: mSampleRate %d, param->audio.sample_rate %d.\n", mSampleRate, param->audio.sample_rate);
    mSampleRate = param->audio.sample_rate;
    mNumOfChannels = param->audio.channel_number;
    if (param->audio.sample_format == SampleFMT_S16)
        mPcmFormat = IAudioHAL::FORMAT_S16_LE;
    else
        mPcmFormat = IAudioHAL::FORMAT_U8_LE;

    mSpecifiedFrameSize = param->audio.frame_size;

    AMLOG_INFO("channel %d, samplerate  %d, pcmFmt %d, framesize %d.\n", mNumOfChannels, mSampleRate, mPcmFormat, mSpecifiedFrameSize);
    return ME_OK;
}

bool CAudioInput2::ProcessCmd(CMD& cmd)
{
    CBuffer* pBuffer;
    AMLOG_CMD("CAudioInput2::ProcessCMD %d.\n", cmd.code);
    switch(cmd.code) {
        case CMD_STOP:
            if (msState != STATE_PENDING) {
                //engine not send EOS before, need check
                AMLOG_WARN("CAudioInput2: engine not send EOS, so send EOS here.\n");
                if (!mpOutput->AllocBuffer(pBuffer)) {
                    AM_ERROR("AllocBuffer failed %s", __FUNCTION__);
                    CmdAck(ME_OK);
                    return false;
                }
                AMLOG_INFO("CAudioInput2 send EOS.\n");
                pBuffer->SetType(CBuffer::EOS);
                mpOutput->SendBuffer(pBuffer);
            }
            CmdAck(ME_OK);
            mbRun = false;
            break;

        case CMD_RUN:
            //re-run
            AMLOG_INFO("CAudioInput2 re-Run, state %d.\n", msState);
            AM_ASSERT(STATE_PENDING == msState || STATE_ERROR == msState);
            msState = STATE_IDLE;
            CmdAck(ME_OK);
            break;

        case CMD_FLOW_CONTROL:
            AMLOG_WARN("Flow control here.\n");
            if (!mpOutput->AllocBuffer(pBuffer)) {
                AM_ERROR("AllocBuffer failed %s", __FUNCTION__);
                return false;
            }

            //EOS is special
            if (((IFilter::FlowControlType)cmd.flag) == FlowControl_eos) {
                AMLOG_WARN("CAudioInput2 send EOS.\n");
                AM_ASSERT(mbRun == true);
                pBuffer->SetType(CBuffer::EOS);
                mpOutput->SendBuffer(pBuffer);
                AMLOG_WARN("CAudioInput2 send EOS done.\n");
                msState = STATE_PENDING;
                mpAudioDriver->Stop();
                CmdAck(ME_OK);
                break;
            } else if (((IFilter::FlowControlType)cmd.flag) == FlowControl_close_audioHAL) {
                handleCloseAudioHAL();
            }

            AMLOG_INFO("CAudioInput2 send flow control.\n");
            pBuffer->SetType(CBuffer::FLOW_CONTROL);
            pBuffer->SetFlags((AM_INT)cmd.flag);
            mpOutput->SendBuffer(pBuffer);

            if (((IFilter::FlowControlType)cmd.flag) == FlowControl_pause) {
                AM_ASSERT(mbSkip == false);
                mbSkip = true;
                msState = STATE_PENDING;
            } else if (((IFilter::FlowControlType)cmd.flag) == FlowControl_resume) {
                AM_ASSERT(mbSkip == true);
                mbSkip = false;
                msState = STATE_IDLE;
            } else if (((IFilter::FlowControlType)cmd.flag) == FlowControl_reopen_audioHAL) {
                handleReopenAudioHAL();
            }
            CmdAck(ME_OK);
            break;

        case CMD_OBUFNOTIFY:
            break;

        default:
            AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return true;
}

void CAudioInput2::OnRun()
{
    AM_U64 timeStamp = 0;
    CBuffer *pBuffer = NULL;
    CmdAck(ME_OK);
    int ret = ME_ERROR;

    IAudioHAL::AudioParam audioParam;
    audioParam.sampleRate = mSampleRate;
    audioParam.numChannels = mNumOfChannels;
    audioParam.sampleFormat = mPcmFormat;
    audioParam.stream = IAudioHAL::STREAM_CAPTURE;
    AMLOG_DEBUG("&&&& CAudioInput2::OnRun: audioParam.sampleRate %d, mSampleRate %d.\n", audioParam.sampleRate, mSampleRate);
    ret = mpAudioDriver->OpenAudioHAL(&audioParam, NULL, NULL);
    if(ret == ME_ERROR){
        AM_ERROR("mpAudioDriver->OpenAudioHAL() failed, ret %d\n",ret);
        msState = STATE_ERROR;
    }else{
        mBitsPerFrame = mpAudioDriver->GetBitsPerFrame();

        mpAudioDriver->Start();
        msState = STATE_IDLE;
    }
    mbRun = true;

    if (!mSpecifiedFrameSize) {
        mCurrentBufferSize = AUDIO_CHUNK_SIZE*(mBitsPerFrame>>3);
        AMLOG_INFO(" CHECK buffer: AUDIO_CHUNK_SIZE %u, mBitsPerFrame %u, mNumOfChannels %u, mCurrentBufferSize %u.\n", AUDIO_CHUNK_SIZE, mBitsPerFrame, mNumOfChannels, mCurrentBufferSize);
    } else {
        mCurrentBufferSize = mSpecifiedFrameSize*(mBitsPerFrame>>3);
        mFadeInMaxBufferNumber = (1*mSampleRate)/mSpecifiedFrameSize/DMAX_FADE_CNT;
        AMLOG_INFO(" CHECK buffer: mSpecifiedFrameSize %u, mBitsPerFrame %u, mNumOfChannels %u, mCurrentBufferSize %u.\n", mSpecifiedFrameSize, mBitsPerFrame, mNumOfChannels, mCurrentBufferSize);
    }

    mAudioTimeInterval = (mCurrentBufferSize/mNumOfChannels/2)*1000*1000/mSampleRate + 1;
    AMLOG_WARN("CAudioInput2 mAudioTimeInterval %u\n", mAudioTimeInterval);

    while (mbRun) {
        CMD cmd;
        AM_U8*  data = NULL;
        AM_UINT numOfFrames = 0;

        AMLOG_STATE("CAudioInput2::OnRun: begin loop, state %d.\n", msState);

        switch (msState) {

            case STATE_IDLE:
                if (mpWorkQ->PeekCmd(cmd)) {
                    ProcessCmd(cmd);
                    if (mbRun == false || msState != STATE_IDLE) {
                        break;
                    }
                }

                while(mpBufferPool->GetFreeBufferCnt() == 0){
                    AMLOG_WARN("There is no free buffer in buffer pool, use the reserve buffer!!\n");
                    if ((mpAudioDriver->ReadStream(mpReserveBuffer, mCurrentBufferSize, &numOfFrames, &timeStamp)) != ME_OK){
                        AMLOG_ERROR(" read fail. CAudioInput2 should send EOS\n");
                        mbBufferFlag = false;
                        break;
                    }
                    mbBufferFlag = true;//use the ReserveBuffer
                }

                AM_ASSERT(mpBufferPool->GetFreeBufferCnt() > 0);
                if (!mpOutput->AllocBuffer(pBuffer)) {
                    AM_ERROR("AllocBuffer failed %s", __FUNCTION__);
                    msState = STATE_ERROR;
                    break;
                }

                data = (AM_U8*)((AM_U8*)pBuffer+sizeof(CBuffer));

                if(mbBufferFlag == true){
                    //copy audio data from mpReserveBuffer to CBuffer
                    ::memcpy(data, mpReserveBuffer, mCurrentBufferSize);
                    mbBufferFlag = false;
                }else{
                    if(true == mbAudioHALClosed){
                        fakeReadStream(data, mCurrentBufferSize, &numOfFrames, &timeStamp);
                    }else {
                        if ((mpAudioDriver->ReadStream(data, mCurrentBufferSize, &numOfFrames, &timeStamp)) != ME_OK) {
                            AMLOG_ERROR(" read fail. CAudioInput2 send EOS\n");
                            pBuffer->SetType(CBuffer::EOS);
                            mpOutput->SendBuffer(pBuffer);
                            pBuffer = NULL;
                            msState = STATE_ERROR;
                            break;
                        }
                        if (mbNeedFadeinAudio) {
                            _fade_audio_input_short((AM_S16*) data, mCurrentBufferSize/(sizeof(AM_S16)), fade_level[mFadeInCount]);
                            mFadeInBufferNumber++;
                            if (mFadeInBufferNumber >= mFadeInMaxBufferNumber) {
                                mFadeInCount ++;
                                mFadeInBufferNumber = 0;
                            }
                            if (mFadeInCount > DMAX_FADE_CNT) {
                                mFadeInCount = 0;
                                mbNeedFadeinAudio = false;
                            }
                        }
                    }
                }

                mToTSample += numOfFrames;

#ifdef RECORD_TEST_FILE
                mpAFile->WriteFile((void*)data, numOfFrames * mBitsPerFrame / 8);
#endif
                //AM_ASSERT(numOfFrames == AUDIO_CHUNK_SIZE);
                pBuffer->SetType(CBuffer::DATA);
                pBuffer->mpData = data;
                pBuffer->mFlags = 0;
                pBuffer->mDataSize = (numOfFrames * mBitsPerFrame / 8);
                AM_ASSERT(pBuffer->mDataSize == mCurrentBufferSize);
                pBuffer->SetPTS(timeStamp);
                static AM_U64 pre_pts = 0;
                AMLOG_DEBUG("CAudioInput2, send data %d, PTS %lld, diff %lld, numOfFrames %d, mBitsPerFrame %d, pBuffer->mDataSize %d.\n", pBuffer->mDataSize, timeStamp, timeStamp - pre_pts, numOfFrames, mBitsPerFrame, pBuffer->mDataSize);
                pre_pts = timeStamp;
                mpOutput->SendBuffer(pBuffer);
                pBuffer = NULL;
                break;

            case STATE_PENDING:
                if (mbSkip == false) {
                    //need read here?
                }
            case STATE_ERROR:
                AM_ASSERT(!pBuffer);
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            default:
                AM_ASSERT(0);
                AM_ERROR("BAD state(%d) in CAudioInput2::OnRun().\n", msState);
                break;
        }
    }

    AMLOG_INFO("CAudioInput2::OnRun end");
}


#ifdef AM_DEBUG
void CAudioInput2::PrintState()
{
    AMLOG_WARN("CAudioInput2: msState=%d.\n", msState);
    AM_ASSERT(mpBufferPool);
    if (mpBufferPool) {
        AMLOG_WARN(" buffer pool have %d free buffers.\n", mpBufferPool->GetFreeBufferCnt());
    }
}
#endif

void CAudioInput2::handleCloseAudioHAL()
{
    mpAudioDriver->Stop();
    mpAudioDriver->CloseAudioHAL();
    mbAudioHALClosed = true;
    AMLOG_WARN("close audio HAL done!\n");
    return;
}

void CAudioInput2::handleReopenAudioHAL()
{
    if(false == mbAudioHALClosed)     return;
    int ret = ME_ERROR;

    IAudioHAL::AudioParam audioParam;
    audioParam.sampleRate = mSampleRate;
    audioParam.numChannels = mNumOfChannels;
    audioParam.sampleFormat = mPcmFormat;
    audioParam.stream = IAudioHAL::STREAM_CAPTURE;
    ret = mpAudioDriver->OpenAudioHAL(&audioParam, NULL, NULL);
    if(ret == ME_OK){
        mbAudioHALClosed = false;
        mBitsPerFrame = mpAudioDriver->GetBitsPerFrame();
        mpAudioDriver->Start();
        msState = STATE_IDLE;
        mbNeedFadeinAudio = true;
        AMLOG_WARN("re-open audio HAL done!\n");
    }
    return;
}

void CAudioInput2::fakeReadStream(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames, AM_U64 *pTimeStamp)
{
    ::memset(pData, 0, dataSize);
    *pNumFrames = dataSize/mNumOfChannels/2;
    *pTimeStamp = ((mToTSample - 200)*IParameters::TimeUnitDen_90khz)/mSampleRate;
    struct timeval delay;
    delay.tv_sec = 0;
    delay.tv_usec = mAudioTimeInterval;
    select(0, NULL, NULL, NULL, &delay);
    return;
}
//-----------------------------------------------------------------------
//
// CAudioInput2Output
//
//-----------------------------------------------------------------------
CAudioInput2Output* CAudioInput2Output::Create(CFilter *pFilter)
{
    CAudioInput2Output *result = new CAudioInput2Output(pFilter);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CAudioInput2Output::Construct()
{
    return ME_OK;
}

