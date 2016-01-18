/*
 * audio_encoder.cpp
 *
 * History:
 *    2011/07/12 - [Jay Zhang] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_TAG "audio_encoder2"
#define AMDROID_DEBUG

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "record_if.h"
#include "engine_guids.h"
#include "am_hwtimer.h"
#include "am_record_if.h"

extern "C" {
#define INT64_C(a) (a ## LL)
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "audio_encoder2.h"
#define MAX_FRAME_SIZE 8192
//#define HW_TIMER_AUDIO_ENCODER


IFilter* CreateAudioEncoder2(IEngine * pEngine)
{
	return CAudioEncoder2::Create(pEngine);
}

IFilter* CAudioEncoder2::Create(IEngine * pEngine)
{
	CAudioEncoder2 * result = new CAudioEncoder2(pEngine);

	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAudioEncoder2::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
    	return err;

    if ((mpInput = CAudioEncoderInput::Create(this)) == NULL)
    	return ME_NO_MEMORY;

    if ((mpOutput = CAudioEncoderOutput::Create(this)) == NULL)
    	return ME_NO_MEMORY;

    if ((mpBufferPool = CSimpleBufferPool::Create("AudioEncoderBuffer", 16, sizeof(CBuffer) + MAX_FRAME_SIZE)) == NULL)
    	return ME_NO_MEMORY;

    mpOutput->SetBufferPool(mpBufferPool);

    return ME_OK;
}

CAudioEncoder2::~CAudioEncoder2()
{
    if (mpAudioEncoder != NULL) {
        avcodec_close(mpAudioEncoder);
        av_free(mpAudioEncoder);
        mpAudioEncoder = NULL;
    }
    AM_DELETE(mpInput);
    mpInput = NULL;
    AM_DELETE(mpOutput);
    mpOutput = NULL;
    AM_DELETE(mpBufferPool);
    mpBufferPool = NULL;
}

void CAudioEncoder2::GetInfo(INFO& info)
{
	info.nInput = 1;
	info.nOutput = 1;
	info.pName = "AudioEncoder";
}

IPin* CAudioEncoder2::GetInputPin(AM_UINT index)
{
	if (index == 0)
		return mpInput;
	return NULL;
}

IPin* CAudioEncoder2::GetOutputPin(AM_UINT index)
{
	if (index == 0)
		return mpOutput;
	return NULL;
}

AM_ERR CAudioEncoder2::Stop()
{
	if (mbEncoding) {/*Maybe a mutex is needed for mbEncoding*/
		mbEncoding = false;
	}
}

AM_ERR CAudioEncoder2::SetAudioEncoder(AUDIO_ENCODER ae)
{
	mAe = ae;
	if (mAe == AUDIO_ENCODER_UNKNOWN) {
		mCodecId = CODEC_ID_NONE;
		return ME_OK;
	} else if (mAe == AUDIO_ENCODER_AAC) {
		mCodecId = CODEC_ID_AAC;
	} else if (mAe == AUDIO_ENCODER_AC3) {
		mCodecId = CODEC_ID_AC3;
	} else if (mAe == AUDIO_ENCODER_ADPCM) {
		mCodecId = CODEC_ID_ADPCM_G726;
	} else {
		AM_ERROR("no codec id for %d\n", mAe);
		return ME_ERROR;
	}
	AM_INFO("codec id = %x\n", mCodecId);
	return ME_OK;
}

AM_ERR CAudioEncoder2::SetAudioParameters(PCM_FORMAT pcmFormat, AM_UINT sampleRate, AM_UINT numOfChannels)
{
    mSampleRate = sampleRate;
    mNumOfChannels = numOfChannels;
    if (pcmFormat == PCM_FORMAT_S16_LE)
        mBitsPerSample = 16;
    else
        mBitsPerSample = 8;

    printf("channel %d, samplerate  %d, bits %d\n", mNumOfChannels, mSampleRate, mBitsPerSample);
	return ME_OK;
}

AM_ERR CAudioEncoder2::OpenEncoder()
{
	AVCodec *codec;
	avcodec_init();
	avcodec_register_all();

	codec = avcodec_find_encoder(mCodecId);
	if (!codec) {
		AM_ERROR("could not find encoder:%x\n",mCodecId);
		return ME_ERROR;
	}
	mpAudioEncoder= avcodec_alloc_context();
	// set sample parameters
        mpAudioEncoder->sample_fmt = AV_SAMPLE_FMT_S16;
	mpAudioEncoder->bit_rate = 128000; // AAC: 128000; AAC_PLUS: 64000; AACPLUS_PS: 40000;
	mpAudioEncoder->sample_rate = mSampleRate;
	mpAudioEncoder->channels = mNumOfChannels;
	mpAudioEncoder->codec_id = codec->id;
	mpAudioEncoder->codec_type = codec->type;
	if (mpAudioEncoder->codec_id == CODEC_ID_AC3) {
		if (mpAudioEncoder->channels == 1)
			mpAudioEncoder->channel_layout = CH_LAYOUT_MONO;
		else if (mpAudioEncoder->channels == 2)
			mpAudioEncoder->channel_layout = CH_LAYOUT_STEREO;
	}
	// open it
	if (avcodec_open(mpAudioEncoder, codec) < 0) {
		AM_ERROR("could not open codec\n");
		return ME_ERROR;
	}
	return ME_OK;
}

void CAudioEncoder2::OnRun()
{
    AM_U32 totalTime = 0;
    int encodeFrame = 0;
    int droppedFrame = 0;
    AM_U32 time_before_enc = 0;
    AM_U32 time_after_enc = 0;
    mbEncoding = true;
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;

    AM_PRINTF("CamAin MainLoop start! \n");

    if (OpenEncoder() != ME_OK)
        return CmdAck(ME_ERROR);
    else CmdAck(ME_OK);


    while(true) {
        CBuffer *pInBuffer;
    	CBuffer *pOutBuffer;
    	int out_size = 0;
    	AM_U8* pOutData = NULL;

        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
        if(type == CQueue::Q_MSG) {
            ProcessCmd(cmd);
            continue;
        }
        AM_ASSERT((CQueueInputPin*)result.pOwner == (CQueueInputPin*)mpInput);
        if (!mpInput->PeekBuffer(pInBuffer)) {
            AM_ERROR("No buffer?\n");
            return;
        }

        if (pInBuffer->GetType() == CBuffer::EOS) {

            if (!mpOutput->AllocBuffer(pOutBuffer))
                AM_ASSERT(0);
            pOutBuffer->SetType(CBuffer::EOS);
            mpOutput->SendBuffer(pOutBuffer);
            return;
        }
        else if (pInBuffer->GetType() == CBuffer::DATA) {

        	if (!mpOutput->AllocBuffer(pOutBuffer))
        		AM_ASSERT(0);
        	pOutData = (AM_U8*)((AM_U8*)pOutBuffer+sizeof(CBuffer));

        	// encode data
        	if (mpAudioEncoder->codec_id == CODEC_ID_NONE) {
        		out_size = pInBuffer->mDataSize;
        		memcpy(pOutData, pInBuffer->GetDataPtr(), pInBuffer->mDataSize);
        	}
        	else {
        		short *pSamples = NULL;
        		pSamples = (short *)pInBuffer->GetDataPtr();

                        printf("avcodec_encode_audio...\n");
        		out_size = avcodec_encode_audio(mpAudioEncoder, pOutData, pInBuffer->mDataSize, pSamples);
                        printf("done: out_size %d\n", out_size);
        	}
        	//send data
        	if (out_size <= 0) {
        		AM_ERROR("%s, %s(), %d out_size %d <=0!!!\n", __FILE__, __FUNCTION__, __LINE__,out_size);

        		pInBuffer->Release();
        		pOutBuffer->Release();
        	}
        	else {
        		pOutBuffer->SetType(CBuffer::DATA);
        		pOutBuffer->mpData = pOutData;
        		pOutBuffer->mFlags = 0;
        		pOutBuffer->mDataSize = out_size;
        		pOutBuffer->SetPTS(pInBuffer->GetPTS());
        		//AM_INFO("CAudioEncoder2 send DATA\n");
        		mpOutput->SendBuffer(pOutBuffer);
        	}
        }
        else {
            AM_ASSERT(0);
        }
        pInBuffer->Release();
    }
}

//-----------------------------------------------------------------------
//
// CAudioEncoderInput
//
//-----------------------------------------------------------------------
CAudioEncoderInput *CAudioEncoderInput::Create(CFilter *pFilter)
{
	CAudioEncoderInput *result = new CAudioEncoderInput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAudioEncoderInput::Construct()
{
	AM_ERR err = inherited::Construct(((CAudioEncoder2*)mpFilter)->MsgQ());
	if (err != ME_OK)
		return err;
	return ME_OK;
}

CAudioEncoderInput::~CAudioEncoderInput()
{
}

AM_ERR CAudioEncoderInput::CheckMediaFormat(CMediaFormat *pFormat)
{
	//TODO
	return ME_OK;
}

//-----------------------------------------------------------------------
//
// CAudioEncoderOutput
//
//-----------------------------------------------------------------------
CAudioEncoderOutput* CAudioEncoderOutput::Create(CFilter *pFilter)
{
	CAudioEncoderOutput *result = new CAudioEncoderOutput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAudioEncoderOutput::Construct()
{
	return ME_OK;
}


