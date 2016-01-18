/*
 * ffmpeg_muxer_ori.cpp
 *
 * History:
 *    2010/3/4 - [Kaiming Xie] created file
 *    2010/7/6 - [Luo Fei] modified file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_TAG "ffmpeg_muxer_ori"
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
#include "am_mw.h"

extern "C" {
#define INT64_C(a) (a ## LL)
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

//#define RECORD_TEST_FILE
#include "ffmpeg_muxer_ori.h"

#if !FFMPEG_VER_0_6
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_SUBTITLE AVMEDIA_TYPE_SUBTITLE
#define guess_format av_guess_format
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif

#define SPS_PPS_LEN (22)
extern IFilter* CreateFFmpegMuxer(IEngine *pEngine)
{
	return CFFMpegMuxerOri::Create(pEngine);
}

//-----------------------------------------------------------------------
//
// CFFMpegMuxerOri
//
//-----------------------------------------------------------------------
IFilter* CFFMpegMuxerOri::Create(IEngine *pEngine)
{
	CFFMpegMuxerOri *result = new CFFMpegMuxerOri(pEngine);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CFFMpegMuxerOri::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	if ((mpMutex = CMutex::Create(false)) == NULL)
		return ME_NO_MEMORY;

	// video input pin & bp
	if ((mpVideoInput = CFFMpegMuxerInputOri::Create(this, CFFMpegMuxerInputOri::VIDEO)) == NULL)
		return ME_ERROR;

	// audio input pin & bp
	if ((mpAudioInput = CFFMpegMuxerInputOri::Create(this, CFFMpegMuxerInputOri::AUDIO)) == NULL)
		return ME_ERROR;

#ifdef RECORD_TEST_FILE
	mpVFile = CFileWriter::Create();
	if (mpVFile == NULL)
		return ME_ERROR;
#endif

	return ME_OK;
}

void CFFMpegMuxerOri::Clear()
{
	if (mpFormat) {
		unsigned int j;
		if (!(mpFormat->oformat->flags & AVFMT_NOFILE) && mpFormat->pb)
			url_fclose(mpFormat->pb);
		for(j=0;j<mpFormat->nb_streams;j++) {
			av_metadata_free(&mpFormat->streams[j]->metadata);
			av_free(mpFormat->streams[j]->codec);
			av_free(mpFormat->streams[j]);
		}
		for(j=0;j<mpFormat->nb_programs;j++) {
			av_metadata_free(&mpFormat->programs[j]->metadata);
		}
		for(j=0;j<mpFormat->nb_chapters;j++) {
			av_metadata_free(&mpFormat->chapters[j]->metadata);
		}
		av_metadata_free(&mpFormat->metadata);
		av_free(mpFormat);
		mpFormat = NULL;
	}
}

CFFMpegMuxerOri::~CFFMpegMuxerOri()
{
	AM_DELETE(mpVideoInput);
	AM_DELETE(mpAudioInput);
#ifdef RECORD_TEST_FILE
	AM_DELETE(mpVFile);
#endif
	Clear();
	AM_DELETE(mpMutex);
}

void *CFFMpegMuxerOri::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IMuxer)
		return (IMuxer*)this;
	return inherited::GetInterface(refiid);
}

void CFFMpegMuxerOri::GetInfo(INFO& info)
{
	info.nInput = 2;
	info.nOutput = 0;
	info.pName = "FFMpegMuxer";
//	if (mAudio >= 0)
//		info.nInput++;
}

IPin* CFFMpegMuxerOri::GetInputPin(AM_UINT index)
{
	if (index == 0) {
		return mpVideoInput;
	}
	else if (index == 1) {
		return mpAudioInput;
	}

	return NULL;
}

void CFFMpegMuxerOri::OnEOS()
{
	AUTO_LOCK(mpMutex);
	if (mbWriteTail) {
		AM_INFO("CFFMpegMuxerOri:receive the first EOS\n");
		mbWriteTail = false;
		av_write_trailer(mpFormat);
		return;
	} else {
		AM_INFO("CFFMpegMuxerOri:receive the second EOS\n");
		mbEOS = false;
		PostEngineMsg(IEngine::MSG_EOS);

#ifdef RECORD_TEST_FILE
		mpVFile->CloseFile();
#endif

		Clear();
	}
}

AM_ERR CFFMpegMuxerOri::SetOutputFile(const char *pFileName)
{
	AM_ERR err;
	AVFormatContext *pFormat;
	AVOutputFormat *file_oformat;
	AVFormatParameters params, *ap = &params;
	AVStream *st;
	AVCodecContext *video_enc;
	AVCodecContext *audio_enc;
	enum CodecID codec_id;
	AVCodec *codec;

	avcodec_init();
	av_register_all();

	char  pOutputFormat[128];

	switch (mOutputFormat) {
		case 0:
			strcpy (pOutputFormat, "mp4");
			break;
		case 1:
			strcpy (pOutputFormat, "3gp");
			break;
		case 2:
			AM_INFO("Could not deduce output format from file extension: using MPEG.\n");
			break;
		case 3:
			AM_INFO("Could not deduce output format from file extension: using MPEG.\n");
			break;
		default:
			AM_INFO("CFFMpegMuxerOri unknown output format.\n");
			break;
	}

	/*guess format*/
	file_oformat = guess_format(NULL, pFileName, NULL);
	if (!file_oformat) {
		AM_ERROR("Could not deduce output format from file extension: using MPEG.\n");
		file_oformat = guess_format("mp4", NULL, NULL);
	}
	if (!file_oformat) {
		return ME_ERROR;
	}

	/* allocate the output media context */
	pFormat=avformat_alloc_context();
	if (!pFormat) {
		AM_ERROR("Memory error\n");
		return ME_ERROR;
	}
	pFormat->oformat = file_oformat;
	snprintf(pFormat->filename, sizeof(pFormat->filename), "%s", pFileName);

	memset(ap, 0, sizeof(*ap));
	if (av_set_parameters(pFormat, ap) < 0) {
		AM_ERROR("av_set_parameters error\n");
		return ME_ERROR;
	}

	/*add video stream*/
	st = av_new_stream(pFormat, pFormat->nb_streams);
	if (!st) {
		return ME_ERROR;
	}
	#if FFMPEG_VER_0_6
	st->filename = pFormat->filename;
	#else
	st->probe_data.filename = pFormat->filename;
	#endif
	avcodec_get_context_defaults2(st->codec, CODEC_TYPE_VIDEO);

	/*Customizing video_enc*/
	video_enc = st->codec;
	if(pFormat->oformat->flags & AVFMT_GLOBALHEADER)
		video_enc->flags |= CODEC_FLAG_GLOBAL_HEADER;
	//codec_id = av_guess_codec(pFormat->oformat, NULL, pFormat->filename, NULL, CODEC_TYPE_VIDEO);
	//codec = avcodec_find_encoder(codec_id);
	codec_id = CODEC_ID_H264;
	video_enc->codec_id = codec_id;
	video_enc->time_base.den = 90000;
	video_enc->time_base.num = 3003;
	video_enc->width = 1280;
	video_enc->height = 720;
	video_enc->sample_aspect_ratio = st->sample_aspect_ratio = (AVRational) {1, 1};
	video_enc->has_b_frames = 2;
	video_enc->extradata_size = SPS_PPS_LEN;
	video_enc->extradata = (uint8_t*)mvideoExtra;
	//AM_INFO("Line %d,video flags:0x%x,codec_id:0x%x\n", __LINE__,video_enc->flags,video_enc->codec_id);

	/*add audio stream*/
	st = av_new_stream(pFormat, pFormat->nb_streams);
	if (!st) {
		return ME_ERROR;
	}
	#if FFMPEG_VER_0_6
	st->filename = pFormat->filename;
	#else
	st->probe_data.filename = pFormat->filename;
	#endif
	avcodec_get_context_defaults2(st->codec, CODEC_TYPE_AUDIO);

	/*Customizing audio_enc*/
	audio_enc = st->codec;
	if(pFormat->oformat->flags & AVFMT_GLOBALHEADER)
		audio_enc->flags |= CODEC_FLAG_GLOBAL_HEADER;
	//codec_id = av_guess_codec(pFormat->oformat, NULL, pFormat->filename, NULL, CODEC_TYPE_AUDIO);
	codec_id = CODEC_ID_AAC;
	audio_enc->codec_id = codec_id;
	audio_enc->bit_rate = 8000;
	audio_enc->sample_rate = 8000;
	audio_enc->channels = 1;
	codec = avcodec_find_encoder(audio_enc->codec_id);
	if (!codec) {
		AM_ERROR("could not find audio codec\n");
		return ME_ERROR;
	}
	if (avcodec_open(audio_enc, codec) < 0) {
		AM_ERROR("could not open audio codec\n");
		return ME_ERROR;
	}
	//AM_INFO("Line %d,audio flags:0x%x,codec_id:0x%x\n", __LINE__,audio_enc->flags,audio_enc->codec_id);

	/* open the output file, if needed */
	if (!(file_oformat->flags & AVFMT_NOFILE)) {
		if (url_fopen(&pFormat->pb, pFileName, URL_WRONLY) < 0) {
			AM_ERROR("Could not open '%s'\n", pFileName);
			return ME_ERROR;
		}
	//AM_INFO("url_fopen done\n");
	}
	if (av_write_header(pFormat) < 0) {
		return ME_ERROR;
	}

	mpFormat = pFormat;

#ifdef RECORD_TEST_FILE
	err = mpVFile->CreateFile("/data/ffmpeg_muxer.264");
	if (err != ME_OK)
		return err;
#endif

	return ME_OK;
}

void CFFMpegMuxerOri::OnVideoBuffer(CBuffer *pBuffer)
{
	//AM_INFO("OnVideoBuffer\n");
	AVPacket packet;
	int ret;

	AUTO_LOCK(mpMutex);

	if(!mbRuning)
	{
		AM_ERROR("%s(), %d --not running state!\n", __FUNCTION__, __LINE__);
		return;
	}
        av_init_packet(&packet);

	packet.size = pBuffer->GetDataSize();
	packet.pts = (AM_U32)pBuffer->GetPTS();
	packet.data = pBuffer->GetDataPtr();
	packet.stream_index = 0;

	AM_INFO("Vid ### Line %d,size:%d,pts:%d\n", __LINE__,packet.size,(int)packet.pts);

	if(mExtraLen == 0) {
		if(pBuffer->mFrameType == 1) {
			mExtraLen = SPS_PPS_LEN;
			memcpy(mvideoExtra,packet.data,mExtraLen);
		}
	}

	ret = av_write_frame(mpFormat, &packet);
	if(ret != 0)
		AM_ERROR("OnVideoBuffer error!\n");

#ifdef RECORD_TEST_FILE
	mpVFile->WriteFile(pBuffer->GetDataPtr(), pBuffer->GetDataSize());
#endif

}

void CFFMpegMuxerOri::OnAudioBuffer(CBuffer *pBuffer)
{
	//AM_INFO("OnAudioBuffer\n");
	AVPacket packet;
	int ret = 0;
	int out_size = 0;
	AUTO_LOCK(mpMutex);

	if(!mbRuning)
	{
		AM_ERROR("%s(), %d --not running state!\n", __FUNCTION__, __LINE__);
		return;
	}

	av_init_packet(&packet);

	out_size = pBuffer->GetDataSize();

	packet.data = pBuffer->GetDataPtr();
	packet.stream_index = 1;
	packet.pts = pBuffer->GetPTS();
	packet.dts = packet.pts;
	packet.size = out_size;

	ret = av_write_frame(mpFormat, &packet);
	if(ret != 0)
		AM_ERROR("OnAudioBuffer error!\n");

	AM_INFO("Aud ### Line %d,size:%d,pts:%d\n", __LINE__,out_size,(int)packet.pts);
}

AM_ERR CFFMpegMuxerOri::Run()
{
	mbRuning = true;
	mbWriteTail = true;
	return ME_OK;
}

AM_ERR CFFMpegMuxerOri::Stop()
{
	mbRuning = false;
	return ME_OK;
}
//-----------------------------------------------------------------------
//
// CFFMpegMuxerInputOri
//
//-----------------------------------------------------------------------
CFFMpegMuxerInputOri* CFFMpegMuxerInputOri::Create(CFilter *pFilter, PinType type)
{
	CFFMpegMuxerInputOri* result = new CFFMpegMuxerInputOri(pFilter);
	if (result && result->Construct(type) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CFFMpegMuxerInputOri::Construct(PinType type)
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	if(type>=0 && type < END)
		mType = type;
	else
		return ME_ERROR;

	return ME_OK;
}

CFFMpegMuxerInputOri::~CFFMpegMuxerInputOri()
{
}

void CFFMpegMuxerInputOri::Receive(CBuffer *pBuffer)
{
	switch (pBuffer->GetType()) {
	case CBuffer::EOS:
		((CFFMpegMuxerOri*)mpFilter)->mbEOS = true;
		((CFFMpegMuxerOri*)mpFilter)->OnEOS();
		break;
	case CBuffer::DATA:
		if( ((CFFMpegMuxerOri*)mpFilter)->mbEOS  == true){
			break;
		}
		if(mType == VIDEO)
			((CFFMpegMuxerOri*)mpFilter)->OnVideoBuffer(pBuffer);
		else if(mType == AUDIO)
			((CFFMpegMuxerOri*)mpFilter)->OnAudioBuffer(pBuffer);
		break;
	default:
		break;
	}

	pBuffer->Release();
}

AM_ERR CFFMpegMuxerInputOri::CheckMediaFormat(CMediaFormat *pFormat)
{
	return ME_OK;
}

