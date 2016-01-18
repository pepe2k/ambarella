/*
 * simple_audio_recorder.cpp
 *
 * History:
 *    2013/11/11 - [SkyChen] create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <basetypes.h>
#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

#include "simple_audio_common.h"
#include "simple_audio_sink.h"
#include "resample_filter.h"
#include "simple_audio_recorder.h"

static void* audio_recorder_pthread_loop(void *argu)
{
    CSimpleAudioRecorder* pInstance = (CSimpleAudioRecorder*)argu;
    if (pInstance) {
        pInstance->OnRun();
    }
    pthread_exit(NULL);
    return NULL;
}
///////////////////////////////////////////////
//
//
//////////////////////////////////////////////
CSimpleAudioRecorder* CSimpleAudioRecorder::Create()
{
    CSimpleAudioRecorder* audioRecorder = new CSimpleAudioRecorder();
    if (audioRecorder && audioRecorder->Construct() != ME_OK) {
        delete audioRecorder;
        audioRecorder = NULL;
    }
    return audioRecorder;
}

CSimpleAudioRecorder::CSimpleAudioRecorder():
    inherited("CSimpleAudioRecorder"),
    mpAudioDriver(NULL),
    mpAudioEncoder(NULL),
    mbRun(AM_FALSE),
    mpNextFilter(NULL),
    mbIsSink(AM_FALSE)
{
    mAudioParam.numChannels = 1;
    mAudioParam.sampleFormat = IAudioHAL::FORMAT_S16_LE;
    mAudioParam.sampleRate = INPUT_SAMPLE_RATE;
    mAudioParam.stream = IAudioHAL::STREAM_CAPTURE;
    memset(mpBuffer, 0x0, sizeof(mpBuffer));
}

CSimpleAudioRecorder::~CSimpleAudioRecorder()
{
}

AM_ERR CSimpleAudioRecorder::Construct()
{
    AM_INFO("CSimpleAudioRecorder Construct!\n");
    AM_ERR err = inherited::Construct();
    if(err != ME_OK) {
        AM_ERROR("inherited::Construct() fail in CSimpleAudioRecorder::Construct().\n");
        return err;
    }

    for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
        mpBuffer[i] = (AM_U8*)malloc(MAX_AUDIO_BUFFER_SIZE);
        if (!mpBuffer[i]) {
            AM_ERROR("CSimpleAudioRecorder::Construct(): no mem!?\n");
            return ME_ERROR;
        }
        memset(mpBuffer[i], 0x0, MAX_AUDIO_BUFFER_SIZE);
    }

    mpAudioDriver = AM_CreateAudioHAL(NULL, 0, 0);
    if (NULL == mpAudioDriver) {
        AM_ERROR("AM_CreateAudioHAL failed!\n");
        return err;
    }
    err = ME_OK;
    return err;
}

AM_ERR CSimpleAudioRecorder::ConfigMe(SAudioInfo info)
{
    AM_ERR err = ME_ERROR;
    mAudioInfo = info;
    mAudioParam.numChannels = mAudioInfo.uChannel;
    mAudioParam.sampleRate = mAudioInfo.uSampleRate;
    mAudioParam.sampleFormat = IAudioHAL::FORMAT_S16_LE;
    if (mAudioInfo.uSampleRate == OUTPUT_SAMPLE_RATE) {
        AM_INFO("CSimpleAudioRecorder: same sample rate, so don't need to resample!\n");
        mbIsSink = AM_TRUE;
        mpNextFilter = (void*)CSimpleAudioSink::Create();
        if (!mpNextFilter) {
            AM_ERROR("CSimpleAudioRecorder: ConfigMe, failed to create CSimpleAudioSink!\n");
            return err;
        }
        err = ((CSimpleAudioSink*)mpNextFilter)->ConfigSink(info);
        if (err != ME_OK) {
            ((CSimpleAudioSink*)mpNextFilter)->Delete();
            mpNextFilter = NULL;
            return err;
        }
    } else {
        mbIsSink = AM_FALSE;
        mpNextFilter = (void*)CReSampleFilter::Create();
        if (!mpNextFilter) {
            AM_ERROR("CSimpleAudioRecorder: ConfigMe, failed to create CReSampleFilter!\n");
            return err;
        }
        err = ((CReSampleFilter*)mpNextFilter)->ConfigConvertor(info);
        if (err != ME_OK) {
            ((CReSampleFilter*)mpNextFilter)->Delete();
            mpNextFilter = NULL;
            return err;
        }
    }
    return err;
}

AM_ERR CSimpleAudioRecorder::Start()
{
    AM_ERR err = mpAudioDriver->OpenAudioHAL(&mAudioParam, NULL, NULL);
    if (err != ME_OK) {
        AM_ERROR("failed to OpenAudioHAL\n");
        return err;
    }
    mBitsPerFrame = mpAudioDriver->GetBitsPerFrame();
    mCurrentBufferSize = AUDIO_CHUNK_SIZE*(mBitsPerFrame>>3);
    SendCmd(CMD_RUN);// enter onRun
    return err;
}

void CSimpleAudioRecorder::OnRun()
{
    AM_INFO("CSimpleAudioRecorder::OnRun !\n");
    AM_ERR err = ME_OK;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    CBuffer *pBuffer = NULL;

    AM_UINT numOfFrames = 0;
    AM_U64 timeStamp = 0;
    AM_UINT bufferIndex = 0;
    mbRun = true;
    CmdAck(ME_OK);
    mpAudioDriver->Start();
    while (mbRun) {
        bool ret = false;
        ret = mpWorkQ->PeekCmd(cmd);
        if (ret == true) {
            ProcessCmd(cmd);
        } else {
            if (bufferIndex >= (AUDIO_BUFFER_COUNT -1)) bufferIndex = 0;
            if ((err = mpAudioDriver->ReadStream(mpBuffer[bufferIndex], mCurrentBufferSize, &numOfFrames, &timeStamp)) != ME_OK) {
                AM_ERROR("ReadStream error!\n");
                mbRun = false;
            }
#if 1
            FILE* file = ::fopen("./audio_pcm_n", "a+");
            if (file) {
                ::fwrite(mpBuffer[bufferIndex], 1, mCurrentBufferSize, (FILE*)file);
                ::fclose((FILE*)file);
            }
#endif
            mToTSample += numOfFrames;
            mCBuffer.mpData = mpBuffer[bufferIndex];
            mCBuffer.mBlockSize = mCBuffer.mDataSize = numOfFrames * (mBitsPerFrame>>3);
            mCBuffer.SetPTS(timeStamp);
            mCBuffer.mBufferId = bufferIndex;//for debug
            if (mbIsSink == AM_TRUE) {
                ((CSimpleAudioSink*)mpNextFilter)->FeedData(mCBuffer.mpData, mCBuffer.mBlockSize, timeStamp);
            } else {
                ((CReSampleFilter*)mpNextFilter)->FeedData(&mCBuffer);
            }
        }
    }
    return;
}

AM_ERR CSimpleAudioRecorder::PerformCmd(CMD& cmd, AM_BOOL isSend)
{
    AM_ERR err;
    if(isSend == AM_TRUE)
    {
        err = MsgQ()->SendMsg(&cmd, sizeof(cmd));
    }else{
        err = MsgQ()->PostMsg(&cmd, sizeof(cmd));
    }
    return err;
}

AM_ERR CSimpleAudioRecorder::ProcessCmd(CMD& cmd)
{
    switch(cmd.code) {
        case CMD_STOP:
            mbRun = false;
            mpAudioDriver->Stop();
            mpAudioDriver->CloseAudioHAL();
            CmdAck(ME_OK);
            break;
        default:
            AM_ERROR("wrong CMD: code %d\n", cmd.code);
            break;
    }
    return ME_OK;
}

void CSimpleAudioRecorder::Stop()
{
    if (mbIsSink == AM_TRUE) {
        ((CSimpleAudioSink*)mpNextFilter)->Stop();
    } else {
        ((CReSampleFilter*)mpNextFilter)->Stop();
    }
    CMD cmd(CMD_STOP);
    PerformCmd(cmd, AM_TRUE);
    return;
}

void CSimpleAudioRecorder::Delete()
{
    if (mpAudioDriver) {
        mpAudioDriver->Delete();
        mpAudioDriver = NULL;
    }
    for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
        if (mpBuffer[i]) {
            free(mpBuffer[i]);
            mpBuffer[i] = NULL;
        }
    }
    if (mbIsSink == AM_TRUE) {
        ((CSimpleAudioSink*)mpNextFilter)->Delete();
    } else {
        ((CReSampleFilter*)mpNextFilter)->Delete();
    }
    inherited::Delete();
    return;
}