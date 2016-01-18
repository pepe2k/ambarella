/*
 * ffmpeg_muxer_ex.cpp
 *
 * History:
 *    2010/7/13 - [Luo Fei] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_TAG "ffmpeg_muxer_ex"
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
#include "ffmpeg_muxer_ex.h"

#if !FFMPEG_VER_0_6
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_SUBTITLE AVMEDIA_TYPE_SUBTITLE
#define guess_format av_guess_format
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif

#define SPS_PPS_LEN (22)
#define HEADER_SIZE_AROUND (8192)
extern IFilter* CreateFFmpegMuxerEx(IEngine *pEngine)
{
	return CFFMpegMuxerEx::Create(pEngine);
}

//-----------------------------------------------------------------------
//
// CFFMpegMuxerEx
//
//-----------------------------------------------------------------------
IFilter* CFFMpegMuxerEx::Create(IEngine * pEngine)
{
	CFFMpegMuxerEx * result = new CFFMpegMuxerEx(pEngine);

	if (result && result->Construct() != ME_OK)
	{
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CFFMpegMuxerEx::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;
	//if ((mpMutex = CMutex::Create(false)) == NULL)
	//	return ME_NO_MEMORY;

	if ((mpAudioInput = CFFMpegMuxerInputEx::Create(this,CFFMpegMuxerInputEx::AUDIO)) == NULL)
		return ME_NO_MEMORY;

	if ((mpVideoInput = CFFMpegMuxerInputEx::Create(this,CFFMpegMuxerInputEx::VIDEO)) == NULL)
		return ME_NO_MEMORY;

#ifdef RECORD_TEST_FILE
	mpVFile = CFileWriter::Create();
	if (mpVFile == NULL)
		return ME_ERROR;
#endif
	return ME_OK;
}

void CFFMpegMuxerEx::Clear()
{
	if (mpFormat) {
		unsigned int j;
		if (!(mpFormat->oformat->flags & AVFMT_NOFILE) && mpFormat->pb)
			url_fclose(mpFormat->pb);
		for (j=0;j<mpFormat->nb_streams;j++) {
			av_metadata_free(&mpFormat->streams[j]->metadata);
			av_free(mpFormat->streams[j]->codec);
			av_free(mpFormat->streams[j]);
		}
		for (j=0;j<mpFormat->nb_programs;j++) {
			av_metadata_free(&mpFormat->programs[j]->metadata);
		}
		for (j=0;j<mpFormat->nb_chapters;j++) {
			av_metadata_free(&mpFormat->chapters[j]->metadata);
		}
		av_metadata_free(&mpFormat->metadata);
		av_free(mpFormat);
		mpFormat = NULL;
	}
}
CFFMpegMuxerEx::~CFFMpegMuxerEx()
{
	AM_DELETE(mpVideoInput);
	AM_DELETE(mpAudioInput);
	Clear();
	//AM_DELETE(mpMutex);
#ifdef RECORD_TEST_FILE
	AM_DELETE(mpVFile);
#endif
}

void *CFFMpegMuxerEx::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IAndroidMuxer)
		return (IAndroidMuxer*)this;
	return inherited::GetInterface(refiid);
}

void CFFMpegMuxerEx::GetInfo(INFO& info)
{
	info.nInput = 2;
	info.nOutput = 0;
	info.pName = "FFMpegMuxerEx";
}

IPin* CFFMpegMuxerEx::GetInputPin(AM_UINT index)
{
	if (index == 0)
		return mpVideoInput;
	else if (index ==1)
		return mpAudioInput;

	return NULL;
}

void CFFMpegMuxerEx::DoStop()
{
	//AUTO_LOCK(mpMutex);
	if (mbRunning) {
		mbRunning = false;
	}
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
	default: 	return "???";
	}
}

AM_ERR CFFMpegMuxerEx::SetVideoSize( AM_UINT width, AM_UINT height )
{
	mWidth = width;
	mHeight = height;
	return ME_OK;
}

AM_ERR CFFMpegMuxerEx::SetVideoFrameRate( AM_UINT framerate )
{
	mFramerate = framerate;
	return ME_OK;
}

AM_ERR CFFMpegMuxerEx::SetMaxDuration(AM_U64 limit)
{
	mLimitDuration = limit;
	AM_INFO("mLimitDuration = %lld\n", mLimitDuration);
	return ME_OK;
}

AM_ERR CFFMpegMuxerEx::SetMaxFileSize(AM_U64 limit)
{
	mLimitFileSize = limit;
	AM_INFO("mLimitFileSize = %lld\n", mLimitFileSize);
	return ME_OK;
}

AM_ERR CFFMpegMuxerEx::SetOutputFormat( OUTPUT_FORMAT of )
{
	mOutputFormat = of;
	return ME_OK;
}

AM_ERR CFFMpegMuxerEx::SetAudioEncoder(AUDIO_ENCODER ae)
{
	mAe = ae;
	if (mAe == AUDIO_ENCODER_DEFAULT) {
		mCodecId = CODEC_ID_NONE;
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
	return ME_OK;
}

AM_ERR CFFMpegMuxerEx::SetOutputFile(const char *pFileName)
{
	strcpy(mOutputFileName,pFileName);
	return ME_OK;
}

AM_ERR CFFMpegMuxerEx::SetAudioParameters(AM_UINT audioSource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate)
{
	mAudioSamplingRate = samplingRate;
	mAudioNumberOfChannels = numberOfChannels;
	mAudioBitrate = bitrate;
	//AM_INFO("mAudioSamplingRate=%d,mAudioNumberOfChannels=%d,mAudioBitrate=%d\n",mAudioSamplingRate,mAudioNumberOfChannels,mAudioBitrate);
	return ME_OK;
}

AM_ERR CFFMpegMuxerEx::WriteHeader()
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
	char pFileNameSuffix[128];

	strcpy(pFileNameSuffix, mOutputFileName);

	strcat(pFileNameSuffix, ".");
	strcat(pFileNameSuffix, get_state_str(mOutputFormat));

	AM_INFO("CFFMpegMuxerEx::SetOutputFile  Filename %s  %s\n", pFileNameSuffix, mOutputFileName);

	avcodec_init();
	av_register_all();

	/*guess format*/
	file_oformat = guess_format(NULL, pFileNameSuffix, NULL);
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
	snprintf(pFormat->filename, sizeof(pFormat->filename), "%s", pFileNameSuffix);

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
	//Fix ME for framerate setting
	if (mFramerate == 30)//29.97
		video_enc->time_base= (AVRational){3003,90000};
	else if (mFramerate == 60)//59.94
		video_enc->time_base= (AVRational){3003,180000};
	else{
		AM_INFO("mFramerate = %d\n , it is not a standard 30 or 60 fps, we have to use 30fps instead of it",mFramerate);
		video_enc->time_base= (AVRational){3003,90000};
	}
	AM_INFO("mFramerate = %d\n",mFramerate);
	video_enc->width = mWidth;
	video_enc->height = mHeight;
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

	audio_enc->codec_id = mCodecId;
	audio_enc->bit_rate = mAudioBitrate;
	audio_enc->sample_rate = mAudioSamplingRate;
	audio_enc->channels = mAudioNumberOfChannels;
	if (audio_enc->codec_id == CODEC_ID_AC3) {
		if (audio_enc->channels == 1)
			audio_enc->channel_layout = CH_LAYOUT_MONO;
		else if (audio_enc->channels == 2)
			audio_enc->channel_layout = CH_LAYOUT_STEREO;
	}
	//AM_INFO("CFFMpegMuxerEx::mCodecId  %d mAudioBitrate %d mAudioSamplingRate %d  mAudioNumberOfChannels  %d\n",
	//		mCodecId ,mAudioBitrate,mAudioSamplingRate , mAudioNumberOfChannels );
	codec = avcodec_find_encoder(audio_enc->codec_id);
	if (!codec) {
		AM_ERROR("could not find audio codec\n");
		return ME_ERROR;
	}
	if (avcodec_open(audio_enc, codec) < 0) {
		AM_ERROR("could not open audio codec\n");
		return ME_ERROR;
	}

	/* open the output file, if needed */
	if (!(file_oformat->flags & AVFMT_NOFILE)) {
		if (url_fopen(&pFormat->pb, pFileNameSuffix, URL_WRONLY) < 0) {
			AM_ERROR("Could not open '%s'\n", pFileNameSuffix);
			return ME_ERROR;
		}
	}
	if (av_write_header(pFormat) < 0) {
		return ME_ERROR;
	}
	mpFormat = pFormat;
#ifdef RECORD_TEST_FILE
	err = mpVFile->CreateFile("/data/ffmpeg_muxer_ex.264");
	if (err != ME_OK)
		return err;
#endif
	return ME_OK;
}

AM_ERR CFFMpegMuxerEx::WriteTailer(AM_UINT code)
{
	if (av_write_trailer(mpFormat)<0) {
		AM_ERROR(" av_write_trailer err\n");
		return ME_ERROR;
	}

#ifdef RECORD_TEST_FILE
		mpVFile->CloseFile();
#endif

	Clear();
	mbWriteTail = false;
	PostEngineMsg(code);
	return ME_OK;
}

void CFFMpegMuxerEx::OnVideoBuffer(CBuffer *pBuffer)
{
	AVPacket packet;
	int ret = 0;

	av_init_packet(&packet);
	packet.stream_index = 0;
	packet.size = pBuffer->GetDataSize();
	packet.data = pBuffer->GetDataPtr();
	//AM_INFO("stream:%d,%d\n",mpFormat->streams[packet.stream_index]->time_base.num,mpFormat->streams[packet.stream_index]->time_base.den);
	//packet.pts = av_rescale_q(pBuffer->GetPTS(), mpFormat->streams[packet.stream_index]->codec->time_base, mpFormat->streams[packet.stream_index]->time_base);
	//AM_INFO("change PTS from %d to %d\n",(int)pBuffer->GetPTS(),(int)packet.pts);
	packet.pts = pBuffer->GetPTS();

	if(mExtraLen == 0) {
		if(pBuffer->mFrameType == 1) {
			mExtraLen = SPS_PPS_LEN;
			memcpy(mvideoExtra,packet.data,mExtraLen);
		}
	}

	if (mLimitFileSize !=0) {
		mFileSize += pBuffer->GetDataSize();
		if (mFileSize + HEADER_SIZE_AROUND >= mLimitFileSize) {
			AM_INFO("---mFileSize = %lld, pBuffer->GetDataSize() = %d,mLimitFileSize = %d\n",mFileSize-pBuffer->GetDataSize(),pBuffer->GetDataSize(),mLimitFileSize);
			WriteTailer(IEngine::MSG_FILESIZE);
			return;
		}
	}

	ret = av_write_frame(mpFormat, &packet);
	if (ret != 0) {
		AM_ERROR("OnVideoBuffer error!\n");
		WriteTailer(IEngine::MSG_ERROR);
		return;
	}

#ifdef RECORD_TEST_FILE
	mpVFile->WriteFile(pBuffer->GetDataPtr(), pBuffer->GetDataSize());
#endif
	AM_INFO("Vid ### Line %d,size:%d,pts:%d\n", __LINE__,packet.size,(int)packet.pts);
}

void CFFMpegMuxerEx::OnAudioBuffer(CBuffer *pBuffer)
{
	AVPacket packet;
	int ret = 0;
	mDuration = pBuffer->GetPTS(),
	av_init_packet(&packet);
	packet.stream_index = 1;
	packet.size = pBuffer->GetDataSize();
	packet.data = pBuffer->GetDataPtr();
	packet.pts = av_rescale_q(mDuration, (AVRational){1,1000}, mpFormat->streams[packet.stream_index]->time_base);

	if (mLimitFileSize != 0) {
		mFileSize += pBuffer->GetDataSize();
		if (mFileSize + HEADER_SIZE_AROUND >= mLimitFileSize) {
			AM_INFO("---mFileSize = %lld, pBuffer->GetDataSize() = %d,mLimitFileSize = %d\n",mFileSize-pBuffer->GetDataSize(),pBuffer->GetDataSize(),mLimitFileSize);
			WriteTailer(IEngine::MSG_FILESIZE);
			return;
		}
	}

	if (mLimitDuration != 0) {
		if (mDuration >= mLimitDuration) {
			WriteTailer(IEngine::MSG_DURATION);
			return;
		}
	}

	ret = av_write_frame(mpFormat, &packet);
	if (ret != 0) {
		AM_ERROR("OnAudioBuffer error!\n");
		WriteTailer(IEngine::MSG_ERROR);
		return;
	}

	AM_INFO("Aud ### Line %d,size:%d,pts:%d\n", __LINE__,packet.size,(int)packet.pts);
}

void CFFMpegMuxerEx::OnRun()
{
	if (WriteHeader() != ME_OK)
		return CmdAck(ME_ERROR);

	CmdAck(ME_OK);

	CQueueInputPin *pPin ;
	CBuffer *pBuffer;
	mbRunning = mbWriteTail = true;
	mbVideoEOS = mbAudioEOS = false;
	mDuration = mFileSize = 0;
	while(true) {
		if (!WaitInputBuffer(pPin, pBuffer)) {
			AM_ERROR("WaitInputBuffer failed when get MSG\n");
			return;
		}
		//AUTO_LOCK(mpMutex);

		if (!mbRunning) {
			pBuffer->Release();
			break;
		}

		if (pBuffer->GetType() == CBuffer::DATA) {
			if (!mbWriteTail)
				AM_INFO("Discarding DATA...");
			else if (((CFFMpegMuxerInputEx*)pPin)->mType == CFFMpegMuxerInputEx::VIDEO)
				OnVideoBuffer(pBuffer);
			else if (((CFFMpegMuxerInputEx*)pPin)->mType == CFFMpegMuxerInputEx::AUDIO)
				OnAudioBuffer(pBuffer);
		}
		else if (pBuffer->GetType() == CBuffer::EOS) {
			if (((CFFMpegMuxerInputEx*)pPin)->mType == CFFMpegMuxerInputEx::VIDEO)
				mbVideoEOS = true;
			else if (((CFFMpegMuxerInputEx*)pPin)->mType == CFFMpegMuxerInputEx::AUDIO)
				mbAudioEOS = true;

			if (mbVideoEOS && mbAudioEOS){ // end OnRun
				if (mbWriteTail)
					WriteTailer(IEngine::MSG_EOS);
				pBuffer->Release();
				break;
			}
		}
		pBuffer->Release();
	}
}

//-----------------------------------------------------------------------
//
// CAudioEncoderInput
//
//-----------------------------------------------------------------------
CFFMpegMuxerInputEx *CFFMpegMuxerInputEx::Create(CFilter *pFilter,PinType type)
{
	CFFMpegMuxerInputEx *result = new CFFMpegMuxerInputEx(pFilter);
	if (result && result->Construct(type) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CFFMpegMuxerInputEx::Construct(PinType type)
{
	AM_ERR err = inherited::Construct(((CFFMpegMuxerEx*)mpFilter)->MsgQ());
	if (err != ME_OK)
		return err;
	if (type>=0 && type < END)
		mType = type;
	else
		return ME_ERROR;
	return ME_OK;
}

CFFMpegMuxerInputEx::~CFFMpegMuxerInputEx()
{
}

AM_ERR CFFMpegMuxerInputEx::CheckMediaFormat(CMediaFormat *pFormat)
{
	//TODO
	return ME_OK;
}
