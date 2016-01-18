/**
 * audio_muxer.cpp
 *
 * History:
 *    2010/6/21 - [Luo Fei] create file
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_TAG "audio_muxer"
#define AMDROID_DEBUG
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

#include "am_record_if.h"
#include "record_if.h"
#include "engine_guids.h"


extern "C" {
#define INT64_C(a) (a ## LL)
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

#include "audio_muxer.h"

#if !FFMPEG_VER_0_6
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_SUBTITLE AVMEDIA_TYPE_SUBTITLE
#define guess_format av_guess_format
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif

extern IFilter* CreateAudioMuxer(IEngine *pEngine)
{
	return CAudioMuxer::Create(pEngine);
}

//-----------------------------------------------------------------------
//
// CAudioMuxer
//
//-----------------------------------------------------------------------
IFilter* CAudioMuxer::Create(IEngine *pEngine)
{
	CAudioMuxer *result = new CAudioMuxer(pEngine);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAudioMuxer::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	mpInputPin = CAudioMuxerInput::Create(this);
	if (mpInputPin == NULL)
		return ME_ERROR;

	return ME_OK;
}

CAudioMuxer::~CAudioMuxer()
{
	AM_DELETE(mpInputPin);
}

void *CAudioMuxer::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IAndroidMuxer)
		return (IAndroidMuxer*)this;
	return inherited::GetInterface(refiid);
}

AM_ERR CAudioMuxer::Run()
{
	mpInputPin->mbEOS = false;
	return ME_OK;
}

AM_ERR CAudioMuxer::Stop()
{
	return ME_OK;
}

void CAudioMuxer::GetInfo(INFO& info)
{
	info.nInput = 1;
	info.nOutput = 0;
	info.pName = "AudioMuxer";
}

IPin* CAudioMuxer::GetInputPin(AM_UINT index)
{
	if (index == 0)
		return mpInputPin;
	return NULL;
}

static const char *get_state_str(int format)
{
	switch (format) {
	case 0:	return "mp4";//default
	case 1:	return "3gp";
	case 2:	return "mp4";
	case 3:	return "amr";
	case 4:	return "amr";
	case 5:	return "adif";//not supported
	case 6:	return "adts";
	default:	return "???";
	}
}
//interface from IAndroidMuxer for video
AM_ERR CAudioMuxer::SetVideoSize( AM_UINT width, AM_UINT height )
{
	return ME_OK;
}

AM_ERR CAudioMuxer::SetVideoFrameRate( AM_UINT framerate )
{
	return ME_OK;
}

AM_ERR CAudioMuxer::SetMaxDuration(AM_U64 limit)
{
	return ME_OK;
}

AM_ERR CAudioMuxer::SetMaxFileSize(AM_U64 limit)
{
	return ME_OK;
}
AM_ERR CAudioMuxer::SetAudioEncoder(AUDIO_ENCODER ae)
{
	mAe = ae;
	if (mAe == AUDIO_ENCODER_DEFAULT) {
		mCodecId = CODEC_ID_NONE;//fc->oformat->audio_codec;
		return ME_ERROR;
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
	AM_INFO("codec_id = %x\n",mCodecId);
	return ME_OK;
}

AM_ERR CAudioMuxer::SetOutputFormat( OUTPUT_FORMAT of )
{
	mOutputFormat = of;
	return ME_OK;
}

AM_ERR CAudioMuxer::SetAudioParameters(AM_UINT audioSource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate)
{
	mAudioSamplingRate = samplingRate;
	mAudioNumberOfChannels = numberOfChannels;
	mAudioBitrate = bitrate;
	AM_INFO("mAudioSamplingRate=%d,mAudioNumberOfChannels=%d,mAudioBitrate=%d\n",mAudioSamplingRate,mAudioNumberOfChannels,mAudioBitrate);
	return ME_OK;
}

AM_ERR CAudioMuxer::SetOutputFile(const char *pFileName)
{
	strcpy(mOutputFileName,pFileName);
	return ME_OK;
}

AM_ERR CAudioMuxer::WriteHeader()
{
	avcodec_init();
	av_register_all();

	AVCodec *codec ;
	AVCodecContext *cc;
	AVFormatContext *fc ;
	AVOutputFormat *fmt ;
	AVStream *audio_st = NULL;
	AVFormatParameters params, *ap = &params;
	enum  CodecID codec_id;

	char pFileNameSuffix[128];
	strcpy(pFileNameSuffix, mOutputFileName);
	strcat(pFileNameSuffix, ".");
	strcat(pFileNameSuffix, get_state_str(mOutputFormat));
	AM_INFO("CAudioMuxer::SetOutputFile  Filename %s  %s\n", pFileNameSuffix, mOutputFileName);

	/*guess format*/
	fmt = guess_format(NULL, pFileNameSuffix, NULL);
	if (!fmt) {
		AM_ERROR("Could not deduce output format from file extension: using mpeg.\n");
		fmt = guess_format("mpeg", NULL, NULL);
	}
	if (!fmt) {
		AM_ERROR("Could not find suitable output format\n");
		return ME_ERROR;
	}
	//AM_INFO("name:%s,codec id: %d\n",fmt->name, fmt->audio_codec);

	/* allocate the output media context */
	fc= avformat_alloc_context();
	if (!fc) {
		AM_ERROR("Memory error\n");
		return ME_ERROR;
	}
	fc->oformat = fmt;
	snprintf(fc->filename, sizeof(fc->filename), "%s", pFileNameSuffix);
	//AM_INFO("FC->filename:%s\n",fc->filename);
	memset(ap, 0, sizeof(*ap));
	if (av_set_parameters(fc, ap) < 0) {
		AM_ERROR("av_set_parameters error\n");
		return ME_ERROR;
	}

	/* add audio stream*/
	if (fmt->audio_codec != CODEC_ID_NONE) {
		audio_st = av_new_stream(fc, fc->nb_streams);
	}
	if(!audio_st) {
		AM_ERROR("Could not add new audio stream\n");
		return ME_ERROR;
	}
	#if FFMPEG_VER_0_6
	audio_st->filename = fc->filename;
	#else
	audio_st->probe_data.filename = fc->filename;
	#endif
	avcodec_get_context_defaults2(audio_st->codec, CODEC_TYPE_AUDIO);

	if(fc->oformat->flags & AVFMT_GLOBALHEADER)
		audio_st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

	/*prepare codec*/
	cc = audio_st->codec;
	cc->codec_id = mCodecId;
	cc->codec_type = CODEC_TYPE_AUDIO;
	cc->bit_rate = mAudioBitrate;
	cc->sample_rate = mAudioSamplingRate;
	cc->channels = mAudioNumberOfChannels;
	if (cc->codec_id == CODEC_ID_AC3) {
		if (cc->channels == 1)
			cc->channel_layout = CH_LAYOUT_MONO;
		else if (cc->channels == 2)
			cc->channel_layout = CH_LAYOUT_STEREO;
	}
	/* find codec*/
	codec = avcodec_find_encoder(cc->codec_id);
	if (!codec) {
		AM_ERROR("codec not found\n");
		return ME_ERROR;
	}
	/* open codec */
	if (avcodec_open(cc, codec) < 0) {
		AM_ERROR("could not open codec\n");
		return ME_ERROR;
	}
	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&fc->pb, pFileNameSuffix, AVIO_FLAG_WRITE) < 0) {
			AM_ERROR("Could not open '%s'\n", pFileNameSuffix);
			return ME_ERROR;
		}
		//AM_INFO("url_fopen done\n");
	}
	/* write the stream header, if any */
	if(av_write_header(fc)<0) {
		AM_ERROR("write header err\n");
		return ME_ERROR;
	}
	//AM_INFO("av_write_header done\n");
	mpFormat=fc;
	mpAs = audio_st;
	mpOFormat = fmt;
	return ME_OK;
}


void CAudioMuxer::ErrorStop()
{
	mbError = true;
	PostEngineMsg(IEngine::MSG_ERROR);
}

void CAudioMuxer::OnEOS()
{
	if (mpInputPin->mbEOS) {
		/*write trailer*/
		if (av_write_trailer(mpFormat)<0) {
			AM_ERROR(" av_write_trailer err\n");
			return;
		}
		mFirstFrameReceived = true;//for loop
		/*close codec*/
		avcodec_close(mpAs->codec);
		/* free codec and streams */
		unsigned int i;
		for (i = 0; i < mpFormat->nb_streams; i++) {
			av_freep(&mpFormat->streams[i]->codec);
			av_freep(&mpFormat->streams[i]);
		}
		/* close the output file */
		if (!(mpOFormat->flags & AVFMT_NOFILE)) {
			avio_close(mpFormat->pb);
		}
		/*free mpFormat*/
		av_free(mpFormat);
		PostEngineMsg(IEngine::MSG_EOS);
	}
}

void CAudioMuxer::OnAudioBuffer(CBuffer *pBuffer)
{
	if (mFirstFrameReceived) {
		WriteHeader();
		mFirstFrameReceived = false;
	}

	AVPacket pkt;
	av_init_packet(&pkt);

	pkt.size= pBuffer->GetDataSize();
	pkt.flags |= PKT_FLAG_KEY;
	pkt.stream_index= 0;
	pkt.data= pBuffer->GetDataPtr();
	pkt.pts= av_rescale_q(pBuffer->GetPTS(), (AVRational){1,1000}, mpFormat->streams[pkt.stream_index]->time_base);

	/* write the frame in the media file */
	if (av_write_frame(mpFormat, &pkt) != 0) {
		AM_ERROR("Error while writing audio frame\n");
		return;
	}
	AM_INFO("Aud ### Line %d,size:%d,pts:%d\n", __LINE__,pkt.size,(int)pkt.pts);

}
//-----------------------------------------------------------------------
//
// CAudioMuxerInput
//
//-----------------------------------------------------------------------
CAudioMuxerInput* CAudioMuxerInput::Create(CFilter *pFilter)
{
	CAudioMuxerInput* result = new CAudioMuxerInput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAudioMuxerInput::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;
	return ME_OK;
}

CAudioMuxerInput::~CAudioMuxerInput()
{
}

void CAudioMuxerInput::Receive(CBuffer *pBuffer)
{
	switch (pBuffer->GetType()) {
	case CBuffer::EOS:
		mbEOS = true;
		((CAudioMuxer*)mpFilter)->OnEOS();
		break;

	case CBuffer::DATA:
		((CAudioMuxer*)mpFilter)->OnAudioBuffer(pBuffer);
		break;

	default:
		break;
	}

	pBuffer->Release();
}

AM_ERR CAudioMuxerInput::CheckMediaFormat(CMediaFormat *pFormat)
{
	return ME_OK;
}

