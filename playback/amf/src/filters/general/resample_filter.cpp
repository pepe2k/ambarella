/*
 * resample_filter.cpp
 *
 * History:
 *    2013/11/11 - [SkyChen] create file
 *    Please refer audio_effecter.cpp
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

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/opt.h"
}
#include "simple_audio_common.h"
#include "simple_audio_sink.h"
#include "resample_filter.h"


static const AVOption _toptions[] = {{NULL, NULL, 0, FF_OPT_TYPE_FLAGS, 0, 0, 0, 0, NULL}};
static const char *_tcontext_to_name(void *ptr)
{
    return "audioresample";
}
static const AVClass _taudioresample_context_class = { "ReSampleContext", _tcontext_to_name, (const struct AVOption *)_toptions, 0, 0, 0};

///////////////////////////////////////////////
//
//
//////////////////////////////////////////////
CReSampleFilter* CReSampleFilter::Create()
{
    CReSampleFilter* result = new CReSampleFilter();
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CReSampleFilter::CReSampleFilter():
    inherited("CReSampleFilter"),
    mpAudioQ(NULL),
    mpResampler(NULL),
    mChannel(0),
    mbRun(false),
    mpAudioSink(NULL)
{
    memset((void*)mReservedSamples, 0x0, sizeof(mReservedSamples));
    memset((void*)mpInputBufferChannelInterleave, 0x0, sizeof(mpInputBufferChannelInterleave));
    memset((void*)mpOutputBufferChannelInterleave, 0x0, sizeof(mpOutputBufferChannelInterleave));
    memset((void*)mpInputBufferAll, 0x0, sizeof(mpInputBufferAll));
    memset((void*)mInputBufferAllSample, 0x0, sizeof(mInputBufferAllSample));
}

CReSampleFilter::~CReSampleFilter()
{
    if (mpResampler) {
        av_resample_close(mpResampler);
    }
}

AM_ERR CReSampleFilter::Construct()
{
    AM_INFO("CReSampleFilter Construct!\n");
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    CQueue* mainQ = MsgQ();
    if ((mpAudioQ= CQueue::Create(mainQ, this, sizeof(CGBuffer), 32)) == NULL)
        return ME_NO_MEMORY;

    mpAudioSink = CSimpleAudioSink::Create();
    if (!mpAudioSink)
        return ME_ERROR;

    return ME_OK;
}

void CReSampleFilter::Delete()
{
    if (mpAudioSink) {
        mpAudioSink->Delete();
        mpAudioSink = NULL;
    }
    for (AM_INT i = 0; i < mChannel; i++) {
        if (mpInputBufferChannelInterleave[i])  free(mpInputBufferChannelInterleave[i]);
        if (mpOutputBufferChannelInterleave[i])  free(mpOutputBufferChannelInterleave[i]);
        if (mpInputBufferAll[i])  free(mpInputBufferAll[i]);
    }
    AM_DELETE(mpAudioQ);
    inherited::Delete();
}

AM_ERR CReSampleFilter::ConfigConvertor(SAudioInfo info)
{
    mpResampler = av_resample_init(OUTPUT_SAMPLE_RATE, info.uSampleRate, 16, 10, 0, 0.8);
    *(const AVClass**)mpResampler = &_taudioresample_context_class;

    if(!mpResampler) {
        AM_ERROR("CReSampleFilter av_resample_init fail, out rate %d, in rate %d.\n", OUTPUT_SAMPLE_RATE, info.uSampleRate);
        return ME_ERROR;
    }
    mChannel = info.uChannel;
    for (AM_INT i = 0; i < mChannel; i++) {
        mpInputBufferChannelInterleave[i] = (short*)malloc((AUDIO_CHUNK_SIZE/mChannel) * sizeof(AM_S16));
        mpOutputBufferChannelInterleave[i] = (short*)malloc(2 * AUDIO_CHUNK_SIZE * sizeof(AM_S16));//hardcode here
        mpInputBufferAll[i] = (short*)malloc(2 * AUDIO_CHUNK_SIZE * sizeof(AM_S16));
        if (!mpInputBufferChannelInterleave[i] || !mpOutputBufferChannelInterleave[i] || !mpInputBufferAll[i]) {
            AM_ERROR("CReSampleFilter: no mem!?\n");
            return ME_ERROR;
        }
        mInputBufferAllSample[i] = 2 * AUDIO_CHUNK_SIZE;
    }
    if (mpAudioSink) {
        AM_ERR ret = mpAudioSink->ConfigSink(info);
        if (ret != ME_OK) {
            AM_ERROR("CReSampleFilter: config audiosink failed!\n");
            return ME_ERROR;
        }
        mpAudioSink->Start();
    }
    SendCmd(CMD_RUN);// enter onRun
    return ME_OK;
}

AM_ERR CReSampleFilter::FeedData(CBuffer* buffer)
{
    if (mpAudioQ->GetDataCnt() < 32) {
        mpAudioQ->PutData(buffer, sizeof(CBuffer));
        mBufferIndex = buffer->mBufferId;
        return ME_OK;
    }
    AM_INFO("Resample audio too slowly!\n");
    return ME_OK;
}

void CReSampleFilter::Stop()
{
    if (mpAudioSink)
        mpAudioSink->Stop();
    CMD cmd(CMD_STOP);
    PerformCmd(cmd, AM_TRUE);
    return;
}

/////////////////////////////////////////////
void CReSampleFilter::OnRun()
{
    AM_INFO("CReSampleFilter::OnRun !\n");
    AM_ERR err = ME_OK;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    CBuffer *pBuffer = NULL;

    mbRun = true;
    CmdAck(ME_OK);

    while (mbRun) {
        bool ret = false;
        AM_U8* data = NULL;
        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),  &result);
        if (type == CQueue::Q_MSG) {
            ProcessCmd(cmd);
        } else {
            if (!mpAudioQ->PeekData(&mBuffer, sizeof(CBuffer))) {
                AM_ERROR("!!PeekData Failed!\n");
                break;
            }
            ProcessData(&mBuffer);
        }
    }
}

AM_ERR CReSampleFilter::ProcessData(CBuffer* buffer)
{
    AM_INT i;
    AM_INT consumed, ret, src_size, dst_size;

    AM_UINT currentBufferIndex = buffer->mBufferId;
    if ((mBufferIndex - currentBufferIndex) > 32) {
        AM_WARNING("ResampleFilter: ProcessData too slowly!!buffer index %d, current index %d\n", mBufferIndex, currentBufferIndex);
    }

    short* pInputBuffer = (short*)buffer->GetDataPtr();
    AM_UINT nSamples = buffer->GetDataSize() >> 1;//(sizeof(S16))=2;
    if (nSamples > AUDIO_CHUNK_SIZE) {
        AM_ERROR("CReSampleFilter: input sample count is %d, larger than [1024]!\n", nSamples);
        return ME_ERROR;
    }
    if (mChannel == 2) {
        for (i = 0; i < (nSamples>>1); i++) {
            *(mpInputBufferChannelInterleave[0] + i) = *(((short*)pInputBuffer) + 2*i);
            *(mpInputBufferChannelInterleave[1] + i) = *(((short*)pInputBuffer) + 2*i + 1);
        }
    } else {
        ::memcpy(mpInputBufferChannelInterleave[0], pInputBuffer, buffer->GetDataSize());
    }

    for (i = 0; i < mChannel; i++) {
        consumed = 0;
        src_size = dst_size = (nSamples/mChannel + mReservedSamples[i]) * sizeof(AM_S16);
        if (mInputBufferAllSample[i] < (nSamples/mChannel + mReservedSamples[i])) {
            short* pNewBuffer = (short*)malloc(src_size);
            if (!pNewBuffer) {
                AM_ERROR("CReSampleFilter::ProcessData: no mem!?\n");
                return ME_ERROR;
            }
            ::memcpy(pNewBuffer, mpInputBufferAll[i], mReservedSamples[i] * sizeof(AM_S16));
            ::memcpy(pNewBuffer + mReservedSamples[i], mpInputBufferChannelInterleave[i], nSamples/mChannel * sizeof(AM_S16));
            free(mpInputBufferAll[i]);
            mpInputBufferAll[i] = pNewBuffer;
            mInputBufferAllSample[i] = nSamples/mChannel + mReservedSamples[i];

            //update outputbuffer
            free(mpOutputBufferChannelInterleave[i]);
            mpOutputBufferChannelInterleave[i] = (short*)malloc(dst_size);
        } else {
            ::memcpy(mpInputBufferAll[i] + mReservedSamples[i], mpInputBufferChannelInterleave[i], nSamples/mChannel * sizeof(AM_S16));
        }
        ret = av_resample(mpResampler, mpOutputBufferChannelInterleave[i], mpInputBufferAll[i], &consumed, nSamples/mChannel + mReservedSamples[i], dst_size, (int)((i+1) == mChannel));
        //AM_INFO("channel %d, ret %d, consumed %d, srcsize %d, mReservedSamples %d\n", i, ret, consumed, src_size, mReservedSamples[i]);
        mReservedSamples[i] = nSamples/mChannel + mReservedSamples[i] - consumed;
        ::memcpy(mpInputBufferAll[i], mpInputBufferAll[i] + consumed, mReservedSamples[i] * sizeof(AM_S16));
    }

    if (mChannel == 2) {
        //no implement
    } else {
        if (mpAudioSink)
            mpAudioSink->FeedData((AM_U8*)mpOutputBufferChannelInterleave[0], ret * sizeof(AM_S16), buffer->GetPTS());
    }
    return ME_OK;
}

AM_ERR CReSampleFilter::ProcessCmd(CMD& cmd)
{
    switch(cmd.code) {
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

AM_ERR CReSampleFilter::PerformCmd(CMD& cmd, AM_BOOL isSend)
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