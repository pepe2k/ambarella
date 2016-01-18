/*
 * audio_encoder.cpp
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
#define LOG_TAG "audio_encoder"
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
#include <media/AudioRecord.h>

extern "C" {
#define INT64_C(a) (a ## LL)
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "audio_encoder.h"
#define MAX_FRAME_SIZE 8192
//#define HW_TIMER_AUDIO_ENCODER
using namespace android;

extern IFilter* CreateAudioEncoder(IEngine * pEngine)
{
	return CAudioEncoder::Create(pEngine);
}

IFilter* CAudioEncoder::Create(IEngine * pEngine)
{
	CAudioEncoder * result = new CAudioEncoder(pEngine);

	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAudioEncoder::Construct()
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

CAudioEncoder::~CAudioEncoder()
{
	if (mpAudioEncoder != NULL) {
		avcodec_close(mpAudioEncoder);
		av_free(mpAudioEncoder);
		mpAudioEncoder = NULL;
	}
	AM_DELETE(mpInput);
	AM_DELETE(mpOutput);
	AM_DELETE(mpBufferPool);
}

void CAudioEncoder::GetInfo(INFO& info)
{
	info.nInput = 1;
	info.nOutput = 1;
	info.pName = "AudioEncoder";
}

IPin* CAudioEncoder::GetInputPin(AM_UINT index)
{
	if (index == 0)
		return mpInput;
	return NULL;
}

IPin* CAudioEncoder::GetOutputPin(AM_UINT index)
{
	if (index == 0)
		return mpOutput;
	return NULL;
}

void CAudioEncoder::DoStop(STOP_ACTION action)
{
	if (mbEncoding) {/*Maybe a mutex is needed for mbEncoding*/
		mAction = action;
		mbEncoding = false;
	}
}

AM_ERR CAudioEncoder::SetAudioEncoder(AUDIO_ENCODER ae)
{
	mAe = ae;
	if (mAe == AUDIO_ENCODER_DEFAULT) {
		mCodecId = CODEC_ID_NONE;
		return ME_OK;
	} else if (mAe == AUDIO_ENCODER_AAC) {
		mCodecId = CODEC_ID_AAC;
	} else if (mAe == AUDIO_ENCODER_AC3) {
		mCodecId = CODEC_ID_AC3;
	} else if (mAe == AUDIO_ENCODER_MP2) {
		mCodecId = CODEC_ID_MP2;
	} else if (mAe == AUDIO_ENCODER_AMR_NB) {
		mCodecId = CODEC_ID_AMR_NB;
	} else {
		AM_ERROR("no codec id for %d\n",mAe);
		return ME_ERROR;
	}
	AM_INFO("codec id = %x\n",mCodecId);
	return ME_OK;
}

AM_ERR CAudioEncoder::SetAudioParameters(AM_UINT audioSource,AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate)
{
	mAudioSamplingRate = samplingRate;
	mAudioNumberOfChannels = numberOfChannels;
	mAudioBitrate = bitrate;
	//AM_INFO("mAudioSamplingRate=%d,mAudioNumberOfChannels=%d,mAudioBitrate=%d\n",mAudioSamplingRate,mAudioNumberOfChannels,mAudioBitrate);
	return ME_OK;
}

AM_ERR CAudioEncoder::OpenEncoder()
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
	mpAudioEncoder->bit_rate = mAudioBitrate;
	mpAudioEncoder->sample_rate = mAudioSamplingRate;
	mpAudioEncoder->channels = mAudioNumberOfChannels;
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

void CAudioEncoder::OnRun()
{
	if (OpenEncoder() != ME_OK)
		return CmdAck(ME_ERROR);
	else CmdAck(ME_OK);

	int hw_timer_flag = -1;
#ifdef HW_TIMER_AUDIO_ENCODER
	hw_timer_flag = AM_open_hw_timer();
	AM_INFO("hw_timer_flag = %d",hw_timer_flag);
#endif

	AM_U32 totalTime = 0;
	int encodeFrame = 0;
	int droppedFrame = 0;
	AM_U32 time_before_enc = 0;
	AM_U32 time_after_enc = 0;
	mbEncoding = true;

	while(true) {
		CQueueInputPin *pPin ;
		CBuffer *pBuffer;
		if (!WaitInputBuffer(pPin, pBuffer)) {
			AM_ERROR("WaitInputBuffer failed");
			return;
		}

		if (!mbEncoding) {
			pBuffer->Release();
			break;
		}

		if (pBuffer->GetType() == CBuffer::EOS) {
			//AM_INFO("CAudioEncoder send EOS\n");
#ifdef HW_TIMER_AUDIO_ENCODER
			if (hw_timer_flag == 0) {
				if (encodeFrame+droppedFrame > 0)
					AM_INFO("encodeFrame=%d,droppedFrame=%d,totalTime=%u,average=%u\n",encodeFrame,droppedFrame,totalTime,totalTime/(encodeFrame+droppedFrame));
			}
#endif
			CBuffer *pOutBuffer;
			if (!mpOutput->AllocBuffer(pOutBuffer))
				return;
			pBuffer->Release();
			pOutBuffer->SetType(CBuffer::EOS);
			mpOutput->SendBuffer(pOutBuffer);
			break;
		}
		else if (pBuffer->GetType() == CBuffer::DATA) {
			CBuffer *pOutBuffer;
			int out_size = 0;
			AM_U8* data = NULL;

			if (!mpOutput->AllocBuffer(pOutBuffer))
				return;
			data = (AM_U8*)((AM_U8*)pOutBuffer+sizeof(CBuffer));

			// encode data
			if (mpAudioEncoder->codec_id == CODEC_ID_NONE) {
				out_size = pBuffer->mDataSize;
				memcpy(data,pBuffer->GetDataPtr(),pBuffer->mDataSize);
			}
			else {
				short *pSamples = NULL;
				pSamples = (short *)pBuffer->GetDataPtr();
#ifdef HW_TIMER_AUDIO_ENCODER
				if (hw_timer_flag == 0)
					time_before_enc = AM_get_hw_timer_count();
#endif
				out_size = avcodec_encode_audio(mpAudioEncoder, data, pBuffer->mDataSize, pSamples);
#ifdef HW_TIMER_AUDIO_ENCODER
				if (hw_timer_flag == 0)
					time_after_enc = AM_get_hw_timer_count();
#endif
			}
			//send data
			if (out_size <= 0) {
				AM_ERROR("%s, %s(), %d out_size %d <=0!!!\n", __FILE__, __FUNCTION__, __LINE__,out_size);
#ifdef HW_TIMER_AUDIO_ENCODER
				if (hw_timer_flag == 0) {
					droppedFrame++;
					totalTime += AM_hwtimer2us(time_before_enc - time_after_enc);
				}
#endif
				pBuffer->Release();
				pOutBuffer->Release();
				continue;
			}
			else {
#ifdef HW_TIMER_AUDIO_ENCODER
				if (hw_timer_flag == 0) {
					encodeFrame++;
					totalTime += AM_hwtimer2us(time_before_enc - time_after_enc);
				}
#endif
				pOutBuffer->SetType(CBuffer::DATA);
				pOutBuffer->mpData = data;
				pOutBuffer->mFlags = 0;
				pOutBuffer->mDataSize = out_size;
				pOutBuffer->SetPTS(pBuffer->GetPTS());
				//AM_INFO("CAudioEncoder send DATA\n");
				mpOutput->SendBuffer(pOutBuffer);
				pBuffer->Release();
			}
		}
		else
			pBuffer->Release();

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
	AM_ERR err = inherited::Construct(((CAudioEncoder*)mpFilter)->MsgQ());
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

