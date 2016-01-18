/*
 * general_audio_sink.cpp
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
#define AUDIO_SINK_DEBUG
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <basetypes.h>
#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

extern "C" {
#define INT64_C
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

#include "simple_audio_common.h"
#include "simple_audio_sink.h"
////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////
CSimpleAudioSink* CSimpleAudioSink::Create()
{
    CSimpleAudioSink* result = new CSimpleAudioSink();
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CSimpleAudioSink::CSimpleAudioSink():
    inherited("CSimpleAudioSink"),
    mpBuffer(NULL),
    mpOutputBuffer(NULL),
    mBufferIndex(0),
    mpDataQ(NULL),
    mpG711Encoder(NULL),
    mprtspInjector(NULL),
    mpDumpFile(NULL)
{
    memset(mpInputBuffer, 0x0, sizeof(mpInputBuffer));
}


AM_ERR CSimpleAudioSink::Construct()
{
    AM_INFO("CSimpleAudioSink Construct!\n");
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    CQueue* mainQ = MsgQ();
    if ((mpDataQ= CQueue::Create(mainQ, this, sizeof(CGBuffer), 32)) == NULL)
        return ME_NO_MEMORY;

    mpBuffer = new CGBuffer();

    for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
        mpInputBuffer[i] = (AM_U8*)malloc(MAX_AUDIO_BUFFER_SIZE);
        if (!mpInputBuffer[i]) {
            AM_ERROR("CSimpleAudioSink::Construct: no mem!?\n");
            return ME_NO_MEMORY;
        }
    }
    mpOutputBuffer = (AM_U8*)malloc(SINGLE_CHANNEL_MAX_AUDIO_BUFFER_SIZE);
    if (!mpOutputBuffer) {
        AM_ERROR("CSimpleAudioSink::Construct: failed to malloc output buffer!\n");
        return ME_NO_MEMORY;
    }

    mpG711Encoder = CG711Codec::CreateG711Codec();
    if (!mpG711Encoder) {
        AM_ERROR("CSimpleAudioSink::Construct: failed to create g711!\n");
        return ME_NO_MEMORY;
    }

    return ME_OK;
}

CSimpleAudioSink::~CSimpleAudioSink()
{
}

void CSimpleAudioSink::Delete()
{
    if (mpG711Encoder) {
        delete mpG711Encoder;
        mpG711Encoder = NULL;
    }

    if (mprtspInjector) {
        delete mprtspInjector;
    }
    mprtspInjector = NULL;

    for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
        if (mpInputBuffer[i]) {
            free(mpInputBuffer[i]);
            mpInputBuffer[i] = NULL;
        }
    }
    if (mpOutputBuffer) {
        free(mpOutputBuffer);
        mpOutputBuffer = NULL;
    }

    if (mpDumpFile) {
        ::fclose((FILE*)mpDumpFile);
        mpDumpFile = NULL;
    }
    AM_DELETE(mpDataQ);
    inherited::Delete();
    AM_INFO("CSimpleAudioSink Delete done!\n");
}

void CSimpleAudioSink::OnRun()
{
    AM_INFO("CSimpleAudioSink::OnRun !\n");
    AM_ERR err;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;

    mbRun = true;
    CmdAck(ME_OK);
    mState = STATE_IDLE;

    while(mbRun) {
        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),  &result);
        if (type == CQueue::Q_MSG) {
            ProcessCmd(cmd);
        } else {
            if (!mpDataQ->PeekData(&mBuffer, sizeof(CBuffer))) {
                AM_ERROR("!!PeekData Failed!\n");
                break;
            }
            err = ProcessData(&mBuffer);
        }
    }
    return;
}

AM_ERR CSimpleAudioSink::ConfigSink(SAudioInfo info)
{
    AM_ERR ret = ME_OK;
    AM_INFO("CSimpleAudioSink::ConfigSink IN\n");

    mAudioInfo = info;
    if (mAudioInfo.bEnableRTSP == AM_TRUE) {
        mprtspInjector = new RtspMediaSession();
        if (mprtspInjector == NULL) {
            AM_ERROR("Create CGInjectorRtsp Failed!\n");
            ret = ME_ERROR;
            return ret;
        }
        mprtspInjector->setDataSourceType(RtspMediaSession::DS_TYPE_LIVE);
        if (mAudioInfo.eFormateType == AUDIO_ALAW) {
            mprtspInjector->addAudioG711Stream(1);
        } else if (mAudioInfo.eFormateType == AUDIO_ULAW) {
            mprtspInjector->addAudioG711Stream(0);
        } else {
            AM_ERROR("CSimpleAudioSink::ConfigSink don't support!\n");
            ret  = ME_ERROR;
            return ret;
        }
        mprtspInjector->setDestination((char*)mAudioInfo.cURL);
    }

    if (mAudioInfo.bSave == AM_TRUE) {
        mpDumpFile = ::fopen((char*)mAudioInfo.cFileName, "a+");
        if (!mpDumpFile) {
            mAudioInfo.bSave = AM_FALSE;
            AM_ERROR("CSimpleAudioSink::ConfigSink failed to open file %s\n", mAudioInfo.cFileName);
        }
    }

    return ret;
}

void CSimpleAudioSink::Start()
{
    SendCmd(CMD_RUN);
    return;
}

void CSimpleAudioSink::Stop()
{
    CMD cmd(CMD_STOP);
    PerformCmd(cmd, AM_TRUE);
    return;
}

AM_ERR CSimpleAudioSink::FeedData(AM_U8* data, AM_UINT size, AM_U64 pts)
{
    AM_ASSERT(size < MAX_AUDIO_BUFFER_SIZE);
    ::memcpy(mpInputBuffer[mBufferIndex%AUDIO_BUFFER_COUNT], data, size);
    mpBuffer->SetDataPtr(mpInputBuffer[mBufferIndex%AUDIO_BUFFER_COUNT]);
    mpBuffer->SetDataSize(size);
    mpBuffer->mBufferId = mBufferIndex;
    mpBuffer->SetPTS(pts);
    if (mpDataQ->GetDataCnt() < 32) {
        mpDataQ->PutData(mpBuffer, sizeof(CBuffer));
        mBufferIndex++;
        return ME_OK;
    }
    AM_WARNING("Resample audio too slowly!\n");
    return ME_OK;
}
////////////////////////////////////////////////////////
AM_ERR CSimpleAudioSink::PerformCmd(CMD& cmd, AM_BOOL isSend)
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

AM_ERR CSimpleAudioSink::ProcessCmd(CMD& cmd)
{
    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            CmdAck(ME_OK);
            break;
        default:
            AM_ERROR("wrong CMD: code %d\n", cmd.code);
            break;
    }
    return ME_OK;
}

AM_ERR CSimpleAudioSink::ProcessData(CGBuffer* pBuffer)
{
    AM_ERR err = ME_OK;
    AM_U8* input = pBuffer->GetDataPtr();
    AM_UINT length = pBuffer->GetDataSize();
    AM_U64 pts = pBuffer->GetPTS();

    AM_UINT currentBufferIndex = pBuffer->mBufferId;
    if ((mBufferIndex - currentBufferIndex) > AUDIO_BUFFER_COUNT) {
        AM_WARNING("CSimpleAudioSink: ProcessData too slowly!!buffer index %d, current index %d\n", mBufferIndex, currentBufferIndex);
    }

    if (mpG711Encoder) {
        AM_INT ret = mpG711Encoder->Encoder(input, mpOutputBuffer, length);
        if (mAudioInfo.bSave == AM_TRUE) {
            ::fwrite(mpOutputBuffer, 1, length>>1, (FILE*)mpDumpFile);
        }
    }

    if (mAudioInfo.bEnableRTSP == AM_TRUE) {
        mprtspInjector->sendData(RtspMediaSession::TYPE_G711_A, mpOutputBuffer, length>>1, -1);
    }

    return err;
}

