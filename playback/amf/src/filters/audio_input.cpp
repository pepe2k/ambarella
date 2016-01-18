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
#define LOG_TAG "audio_input"
#define AMDROID_DEBUG
//#define RECORD_TEST_FILE

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "record_if.h"
#include "am_record_if.h"
#include "engine_guids.h"

#include <media/AudioRecord.h>
extern "C" {
#define INT64_C(a) (a ## LL)
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "audio_input.h"
#define MAX_FRAME_SIZE 8192
using namespace android;

extern IFilter* CreateAudioInput(IEngine * pEngine)
{
	return CAudioInput::Create(pEngine);
}

IFilter* CAudioInput::Create(IEngine * pEngine)
{
	CAudioInput * result = new CAudioInput(pEngine);

	if(result && result->Construct() != ME_OK)
	{
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAudioInput::Construct()
{
	AM_ERR err = inherited::Construct();
	if(err != ME_OK)
		return err;

	if ((mpBufferPool = CSimpleBufferPool::Create("AudioInputBuffer", 16, sizeof(CBuffer) + MAX_FRAME_SIZE) ) == NULL)
		return ME_NO_MEMORY;

	if ((mpOutput = CAudioInputOutput::Create(this)) == NULL)
		return ME_NO_MEMORY;

	mpOutput->SetBufferPool(mpBufferPool);

#ifdef RECORD_TEST_FILE
        mpAFile = CFileWriter::Create();
        if (mpAFile == NULL)
                return ME_ERROR;
#endif

	return ME_OK;
}


CAudioInput::~CAudioInput()
{
	if (mpAudioEncoder != NULL) {
		avcodec_close(mpAudioEncoder);
		av_free(mpAudioEncoder);
		mpAudioEncoder = NULL;
	}
	AM_DELETE(mpOutput);
	AM_DELETE(mpBufferPool);

#ifdef RECORD_TEST_FILE
        AM_DELETE(mpAFile);
#endif

}

void CAudioInput::GetInfo(INFO& info)
{
	info.nInput = 0;
	info.nOutput = 1;
	info.pName = "AudioInput";
}

IPin* CAudioInput::GetOutputPin(AM_UINT index)
{
	if (index == 0)
		return mpOutput;
	return NULL;
}


void CAudioInput::DoStop(STOP_ACTION action)
{
	if (mbInputting) {/*Maybe a mutex is needed for mbInputting*/
		mAction = action;
		mbInputting = false;
	}
}

AM_ERR CAudioInput::SetAudioEncoder(AUDIO_ENCODER ae)
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

//same as AM_ERR CAudioEncoder::OpenEncoder()
AM_ERR CAudioInput::OpenEncoder()
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

AM_ERR CAudioInput::GetFrameSize()
{
	if (OpenEncoder() != ME_OK)
		return ME_ERROR ;
	mFrameSizeInByte = (mpAudioEncoder->frame_size)*mAudioNumberOfChannels*sizeof(AM_U16);
	return ME_OK;
}

AM_ERR CAudioInput::SetAudioParameters(AM_UINT audioSource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate)
{
	mAudioSource = audioSource;
	mAudioSamplingRate = samplingRate;
	mAudioNumberOfChannels =numberOfChannels;
	mAudioBitrate = bitrate;
	return ME_OK;
}

void CAudioInput::OnRun()
{
	if (GetFrameSize() !=ME_OK) {
		AM_ERROR("Get FrameSize failed\n");
		return CmdAck(ME_ERROR);
	}
	//TODO: get buffer size from AudioFlinger
	const int kBufferSize = 2048;
	int iTimeStamp = 0;
	int dataDuration = 0;
	CBuffer *pBuffer;

	AM_UINT flags = AudioRecord::RECORD_AGC_ENABLE | AudioRecord::RECORD_NS_ENABLE | AudioRecord::RECORD_IIR_ENABLE;

	AudioRecord * record = new AudioRecord(
			mAudioSource, mAudioSamplingRate,
			AudioSystem::PCM_16_BIT,
			(mAudioNumberOfChannels > 1) ? AudioSystem::CHANNEL_IN_STEREO : AudioSystem::CHANNEL_IN_MONO,
			32*kBufferSize/mAudioNumberOfChannels/sizeof(AM_U16), flags);
	status_t res = record->initCheck();
	if (res == NO_ERROR)
		res = record->start();

	if (res == NO_ERROR) {

#ifdef RECORD_TEST_FILE
	AM_ERR er = mpAFile->CreateFile("/data/audio_input.pcm");
  	if (er != ME_OK) {
  		AM_ERROR("CreateFile failed %s", __FUNCTION__);
  		return ;
  	}
	AM_INFO("CreateFile /data/audio_input.pcm");
#endif

		mbInputting = true;
		CmdAck(ME_OK);

		while (1) {

			AM_U8*  data = NULL;
			int numOfBytes = 0;

			if (!mpOutput->AllocBuffer(pBuffer)) {
				AM_ERROR("AllocBuffer failed %s", __FUNCTION__);
				break;
			}

			if (!mbInputting) {
				if (mAction == SA_STOP) {
					//AM_INFO("CAudioInput send EOS\n");
					pBuffer->SetType(CBuffer::EOS);
					mpOutput->SendBuffer(pBuffer);
				}
				break;
			}

			data = (AM_U8*)((AM_U8*)pBuffer+sizeof(CBuffer));

			numOfBytes = record->read(data, mFrameSizeInByte);
			if (numOfBytes <= 0) {
				AM_ERROR("CAudioInput::OnRun() read bytes <= 0\n");
				return;
			}
			dataDuration = (numOfBytes/sizeof(AM_U16)/mAudioNumberOfChannels) * 1000 /mAudioSamplingRate; //ms
			pBuffer->SetType(CBuffer::DATA);
			pBuffer->mpData = data;
			pBuffer->mFlags = 0;
			pBuffer->mDataSize = numOfBytes;
			pBuffer->SetPTS(iTimeStamp);
			//AM_INFO("CAudioInput send data\n");
			mpOutput->SendBuffer(pBuffer);

			iTimeStamp += dataDuration;

#ifdef RECORD_TEST_FILE
			AM_INFO("WriteFile bytes:%d\n",numOfBytes);
			mpAFile->WriteFile(data, numOfBytes);
#endif

		}
		record->stop();

#ifdef RECORD_TEST_FILE
	AM_INFO("CloseFile\n");
	mpAFile->CloseFile();
#endif

	}
	else {
			AM_ERROR("AudioRecord Error!\n");
			CmdAck(ME_ERROR);
	}
	delete record;

}


//-----------------------------------------------------------------------
//
// CAudioInputOutput
//
//-----------------------------------------------------------------------
CAudioInputOutput* CAudioInputOutput::Create(CFilter *pFilter)
{
	CAudioInputOutput *result = new CAudioInputOutput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAudioInputOutput::Construct()
{
	return ME_OK;
}

