/*
 * amba_audio_encoder.cpp
 *
 * History:
 *    2011/4/12 - [Luo Fei] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_NDEBUG 0
#define LOG_TAG "amba_audio_encoder"
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
#include "am_record_if.h"
extern "C" {
#define INT64_C(a) (a ## LL)
#include "aac_audio_enc.h"
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "amba_audio_encoder.h"

#define MAX_OUTPUT_FRAME_SIZE 512

#if ENABLE_AMBA_AAC==true
	#define AMBA_AAC 1
#else
	#define AMBA_AAC 0
#endif

#undef stderr
FILE *stderr =	(&__sF[2]);
au_aacenc_config_t au_aacenc_config;
AM_U8  temp_enc_buf[106000];


extern IFilter* CreateAmbaAudioEncoder(IEngine * pEngine)
{
	return CAmbaAudioEncoder::Create(pEngine);
}

IFilter* CAmbaAudioEncoder::Create(IEngine * pEngine)
{
	CAmbaAudioEncoder * result = new CAmbaAudioEncoder(pEngine);

	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaAudioEncoder::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	if ((mpInput = CAmbaAudioEncoderInput::Create(this)) == NULL)
		return ME_NO_MEMORY;

	if ((mpOutput = CAmbaAudioEncoderOutput::Create(this)) == NULL)
		return ME_NO_MEMORY;

	if ((mpBuf = CSimpleBufferPool::Create("AudioEncodeBuffer", 128, sizeof(CBuffer) + MAX_OUTPUT_FRAME_SIZE)) == NULL)
		return ME_NO_MEMORY;

	mpOutput->SetBufferPool(mpBuf);

	return ME_OK;
}

CAmbaAudioEncoder::~CAmbaAudioEncoder()
{
	if (mpAudioEncoder != NULL) {
		avcodec_close(mpAudioEncoder);
		av_free(mpAudioEncoder);
		mpAudioEncoder = NULL;
	}
	AM_DELETE(mpInput);
	AM_DELETE(mpOutput);
	AM_DELETE(mpBuf);
}

void CAmbaAudioEncoder::GetInfo(INFO& info)
{
	info.nInput = 1;
	info.nOutput = 1;
	info.pName = "AmbaAudEnc";
}

IPin* CAmbaAudioEncoder::GetInputPin(AM_UINT index)
{
	if (index == 0) return mpInput;
	return NULL;
}

IPin* CAmbaAudioEncoder::GetOutputPin(AM_UINT index)
{
	if (index == 0) return mpOutput;
	return NULL;
}

void CAmbaAudioEncoder::DoStop(STOP_ACTION action)
{
	mAction = action;
	mbRunFlag = false;
}

AM_ERR CAmbaAudioEncoder::GetParametersFromEngine()
{
	IAmbaAuthorControl * p = NULL;
	if ((p = IAmbaAuthorControl::GetInterfaceFrom(mpEngine)) == NULL) {
		mSamplingRate = 48000;
		mNumberOfChannels = 2;
		mBitrate = 128000;
		mSampleFmt = SAMPLE_FMT_S16;
		mCodecId = CODEC_ID_AAC;
	}
	else {
		int fmt,codecId;
		p->GetAudioParam(&mSamplingRate,&mNumberOfChannels,&mBitrate,&fmt,&codecId);
		mSampleFmt = (SampleFormat)fmt;
		mCodecId = (CodecID)codecId;
	}

	AM_INFO("mSamplingRate=%d, mNumberOfChannels=%d,mBitrate=%d,mSampleFmt=%d,mCodecId=%d",
		mSamplingRate,mNumberOfChannels,mBitrate,mSampleFmt,mCodecId);

	return ME_OK;
}

AM_ERR CAmbaAudioEncoder::OpenEncoder()
{
	if ((mCodecId == CODEC_ID_AAC) && AMBA_AAC) {
		au_aacenc_config.enc_mode	= 0;      // 0: AAC; 1: AAC_PLUS; 3: AACPLUS_PS;
		au_aacenc_config.sample_freq = mSamplingRate;
		au_aacenc_config.coreSampleRate = mSamplingRate;
		au_aacenc_config.Src_numCh = mNumberOfChannels;
		au_aacenc_config.Out_numCh = mNumberOfChannels;
		au_aacenc_config.tns  = 1;
		au_aacenc_config.ffType = 't';
		au_aacenc_config.bitRate = mBitrate; // AAC: 128000; AAC_PLUS: 64000; AACPLUS_PS: 40000;
		au_aacenc_config.quantizerQuality  = 2;
		au_aacenc_config.codec_lib_mem_adr = (u32*)temp_enc_buf;
		aacenc_setup(&au_aacenc_config);
		aacenc_open(&au_aacenc_config);
	}
	else {
		AVCodec *codec;
		avcodec_init();
		avcodec_register_all();

		codec = avcodec_find_encoder(mCodecId);
		if (!codec) {
			AM_ERROR("Failed to find encoder:%x\n",mCodecId);
			return ME_ERROR;
		}
		mpAudioEncoder= avcodec_alloc_context();
		mpAudioEncoder->bit_rate = mBitrate;
		mpAudioEncoder->sample_rate = mSamplingRate;
		mpAudioEncoder->channels = mNumberOfChannels;
		mpAudioEncoder->codec_id = mCodecId;
		mpAudioEncoder->codec_type = codec->type;
		mpAudioEncoder->sample_fmt = mSampleFmt;
		if (mpAudioEncoder->codec_id == CODEC_ID_AC3) {
			if (mpAudioEncoder->channels == 1)
				mpAudioEncoder->channel_layout = CH_LAYOUT_MONO;
			else if (mpAudioEncoder->channels == 2)
				mpAudioEncoder->channel_layout = CH_LAYOUT_STEREO;
		}
		if (avcodec_open(mpAudioEncoder, codec) < 0) {
			AM_ERROR("Failed to open codec\n");
			return ME_ERROR;
		}
	}
	return ME_OK;
}

void CAmbaAudioEncoder::OnRun()
{
	CQueueInputPin *pPin = NULL;
	CBuffer *pBuffer = NULL;
	CBuffer *pOutBuffer = NULL;
	int outSize =0;
	AM_U8* outData = NULL;
	short *inData = NULL;
	int frameNum = 0;

	AM_INFO("OnRun ...");
	if (GetParametersFromEngine() != ME_OK) {
		AM_ERROR("Failed to get parameters");
		return CmdAck(ME_ERROR);
	}
	if (OpenEncoder() != ME_OK)
		return CmdAck(ME_ERROR);
	else CmdAck(ME_OK);
	AM_INFO("OnRun OK");

	mbRunFlag = true;
	while(true) {
		if (!WaitInputBuffer(pPin, pBuffer)) {
			AM_ERROR("Failed to WaitInputBuffer");
			break;
		}

		if (!mbRunFlag) {
			pBuffer->Release();
			break;
		}

		if (pBuffer->GetType() == CBuffer::DATA) {
			if (!mpOutput->AllocBuffer(pOutBuffer)) {
				AM_ERROR("Failed to AllocBuffer");
				break;
			}
			AM_INFO("AllocBuffer done");
			outData = (AM_U8*)((AM_U8*)pOutBuffer+sizeof(CBuffer));
			inData = (short *)pBuffer->GetDataPtr();
			if (mCodecId == CODEC_ID_AAC && AMBA_AAC) {
				AM_INFO("In TW libacc.\n");
				au_aacenc_config.enc_rptr = (s32 *)inData;
				au_aacenc_config.enc_wptr = (u8 *)outData;
				aacenc_encode(&au_aacenc_config);
				outSize = (au_aacenc_config.nBitsInRawDataBlock + 7) >> 3;
				// outData += 7;//skip adts header
			}
			else {
				AM_INFO("In ffmpeg aac.\n");
				outSize = avcodec_encode_audio(mpAudioEncoder, outData, pBuffer->mDataSize, inData);
			}
			AM_INFO("encode audio done");
			if (outSize <= 0) {
				AM_ERROR("Failed to encode data");
				pBuffer->Release();
				pOutBuffer->Release();
				continue;
			} else {
				pOutBuffer->SetType(CBuffer::DATA);
				pOutBuffer->mpData = outData;
				pOutBuffer->mFlags = 0;
				pOutBuffer->mDataSize = outSize;
				pOutBuffer->mBlockSize = outSize;
				// pOutBuffer->mDataSize = (au_aacenc_config.nBitsInRawDataBlock + 7) >> 3;
				// pOutBuffer->mBlockSize = (au_aacenc_config.nBitsInRawDataBlock + 7) >> 3;
				pOutBuffer->SetPTS(pBuffer->GetPTS());
				AM_INFO("CAudioEncoder send DATA, frameNum %d, size %d", frameNum++, outSize);
				mpOutput->SendBuffer(pOutBuffer);
				pBuffer->Release();
			}
		}
		else if (pBuffer->GetType() == CBuffer::EOS) {
			AM_INFO("CAudioEncoder will send EOS\n");
			if (!mpOutput->AllocBuffer(pOutBuffer)) {
				AM_ERROR("Failed to AllocBuffer");
				break;
			}
			else {
				pBuffer->Release();
				pOutBuffer->SetType(CBuffer::EOS);
				AM_INFO("CAudioEncoder send EOS\n");
				mpOutput->SendBuffer(pOutBuffer);
				break;
			}
		}
		else
			pBuffer->Release();
	}

	AM_INFO("OnRun end");
}

//-----------------------------------------------------------------------
//
// CAmbaAudioEncoderInput
//
//-----------------------------------------------------------------------
CAmbaAudioEncoderInput *CAmbaAudioEncoderInput::Create(CFilter *pFilter)
{
	CAmbaAudioEncoderInput *result = new CAmbaAudioEncoderInput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaAudioEncoderInput::Construct()
{
	AM_ERR err = inherited::Construct(((CAmbaAudioEncoder*)mpFilter)->MsgQ());
	if (err != ME_OK)
		return err;
	return ME_OK;
}

//-----------------------------------------------------------------------
//
// CAmbaAudioEncoderOutput
//
//-----------------------------------------------------------------------
CAmbaAudioEncoderOutput* CAmbaAudioEncoderOutput::Create(CFilter *pFilter)
{
	CAmbaAudioEncoderOutput *result = new CAmbaAudioEncoderOutput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

