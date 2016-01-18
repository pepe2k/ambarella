/*
 * amba_ffmpeg_muxer.cpp
 *
 * History:
 *    2011/4/13 - [Luo Fei] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_NDEBUG 0
#define LOG_TAG "amba_ffmpeg_muxer"
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

#include <basetypes.h>
#include "iav_drv.h"
#include "amba_ffmpeg_muxer.h"

#define SPS_PPS_LEN (23)
//#define SPS_PPS_LEN (22)
#define HEADER_SIZE_AROUND (8192)

#if ENABLE_AMBA_AAC==true
	#define AMBA_AAC 1
#else
	#define AMBA_AAC 0
#endif

#define RECORD_VID_FILE
#define RECORD_AUD_FILE

#if !FFMPEG_VER_0_6
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_SUBTITLE AVMEDIA_TYPE_SUBTITLE
#define guess_format av_guess_format
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#define URL_WRONLY AVIO_FLAG_WRITE
#endif

extern IFilter* CreateAmbaFFMpegMuxer(IEngine *pEngine)
{
	return CAmbaFFMpegMuxer::Create(pEngine);
}

//-----------------------------------------------------------------------
//
// CAmbaFFMpegMuxer
//
//-----------------------------------------------------------------------
IFilter* CAmbaFFMpegMuxer::Create(IEngine * pEngine)
{
	CAmbaFFMpegMuxer * result = new CAmbaFFMpegMuxer(pEngine);
	if (result && result->Construct() != ME_OK)
	{
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaFFMpegMuxer::Construct()
{
	AM_ERR err;
	if ((err = inherited::Construct()) != ME_OK)
		return err;

	if ((mpAudioInput = CAmbaFFMpegMuxerInput::Create(this,CAmbaFFMpegMuxerInput::AUDIO)) == NULL)
		return ME_NO_MEMORY;

	if ((mpVideoInput = CAmbaFFMpegMuxerInput::Create(this,CAmbaFFMpegMuxerInput::VIDEO)) == NULL)
		return ME_NO_MEMORY;

#ifdef RECORD_VID_FILE
	if ((mpVFile = CFileWriter::Create()) == NULL) return ME_ERROR;
#endif
#ifdef RECORD_AUD_FILE
	if ((mpAFile = CFileWriter::Create()) == NULL) return ME_ERROR;
#endif

	return ME_OK;
}

void CAmbaFFMpegMuxer::Clear()
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
CAmbaFFMpegMuxer::~CAmbaFFMpegMuxer()
{
	AM_DELETE(mpVideoInput);
	AM_DELETE(mpAudioInput);
	Clear();
#ifdef RECORD_VID_FILE
	AM_DELETE(mpVFile);
#endif
#ifdef RECORD_AUD_FILE
	AM_DELETE(mpAFile);
#endif
}

void *CAmbaFFMpegMuxer::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IMuxer) return (IMuxer*)this;
	return inherited::GetInterface(refiid);
}

void CAmbaFFMpegMuxer::GetInfo(INFO& info)
{
	info.nInput = 2;
	info.nOutput = 0;
	info.pName = "AmbaFfmpegMuxer";
}

IPin* CAmbaFFMpegMuxer::GetInputPin(AM_UINT index)
{
	if (index == 0)	return mpVideoInput;
	else if (index ==1) return mpAudioInput;
	return NULL;
}

void CAmbaFFMpegMuxer::DoStop()
{
	mbRunFlag = false;
}

AM_ERR CAmbaFFMpegMuxer::GetParametersFromEngine()
{
	IAmbaAuthorControl * p = NULL;
	if ((p = IAmbaAuthorControl::GetInterfaceFrom(mpEngine)) == NULL) {
		AM_INFO("Executing in if statement, returned value is NULL!\n");
		mSamplingRate = 48000;
		mNumberOfChannels = 2;
		mBitrate = 128000;
		mSampleFmt = SAMPLE_FMT_S16;
		mCodecId = CODEC_ID_AAC;
		mWidth = 1280;
		mHeight = 720;
		mFramerate = 30;
	}
	else {
		AM_INFO("Executing in else statement.");
		int fmt,codecId;
		p->GetAudioParam(&mSamplingRate,&mNumberOfChannels,&mBitrate,&fmt,&codecId);
		mSampleFmt = (SampleFormat)fmt;
		mCodecId = (CodecID)codecId;
		p->GetVideoParam(&mWidth,&mHeight,&mFramerate,NULL);
		AM_INFO("mWidth = %d, mHeight = %d\n", mWidth, mHeight);
	}

	AM_INFO("mSamplingRate=%d, mNumberOfChannels=%d,mBitrate=%d,mSampleFmt=%d,mCodecId=%d",
		mSamplingRate,mNumberOfChannels,mBitrate,mSampleFmt,mCodecId);
	AM_INFO("mWidth=%d, mHeight=%d,mFramerate=%d\n",
		mWidth,mHeight,mFramerate);

	return ME_OK;
}

AM_ERR CAmbaFFMpegMuxer::SetOutputFile(const char *pFileName)
{
	strcpy(mOutputFileName,pFileName);
	AM_INFO("OutputFile = %s",mOutputFileName);
	return ME_OK;
}

AM_ERR CAmbaFFMpegMuxer::WriteHeader()
{
	AM_INFO("WriteHeader ... ");
	AM_ERR err;
	AVFormatContext *pFormat;
	AVOutputFormat *file_oformat;
	AVFormatParameters params, *ap = &params;
	AVStream *st;
	AVCodecContext *video_enc;
	AVCodecContext *audio_enc;
	enum CodecID codec_id;
	AVCodec *codec;

	if (GetParametersFromEngine() != ME_OK) return ME_ERROR;

	char pFileNameSuffix[128];
	strcpy(pFileNameSuffix, mOutputFileName);
	strcat(pFileNameSuffix, ".");
	strcat(pFileNameSuffix,"mp4");

	avcodec_init();
	av_register_all();

	//guess format
	file_oformat = guess_format(NULL, pFileNameSuffix, NULL);
	if (!file_oformat) {
		AM_ERROR("Could not deduce output format from file extension: using MPEG.\n");
		file_oformat = guess_format("mp4", NULL, NULL);
	}
	if (!file_oformat) {
		return ME_ERROR;
	}

	//allocate the output media context
	if (!(pFormat = avformat_alloc_context())) {
		AM_ERROR("avformat_alloc_context error\n");
		return ME_ERROR;
	}
	pFormat->oformat = file_oformat;
	snprintf(pFormat->filename, sizeof(pFormat->filename), "%s", pFileNameSuffix);

	memset(ap, 0, sizeof(*ap));
	if (av_set_parameters(pFormat, ap) < 0) {
		AM_ERROR("av_set_parameters error\n");
		return ME_ERROR;
	}

	//add video stream
	AM_INFO("pFormat->nb_streams = %d\n", pFormat->nb_streams);
	if (!(st = av_new_stream(pFormat, pFormat->nb_streams))) {
		return ME_ERROR;
	}
	AM_INFO("pFormat->nb_streams = %d\n", pFormat->nb_streams);

	#if FFMPEG_VER_0_6
	st->filename = pFormat->filename;
	#else
	st->probe_data.filename = pFormat->filename;
	#endif
	avcodec_get_context_defaults2(st->codec, CODEC_TYPE_VIDEO);
	video_enc = st->codec;
	if (pFormat->oformat->flags & AVFMT_GLOBALHEADER)
		video_enc->flags |= CODEC_FLAG_GLOBAL_HEADER;
	video_enc->codec_id = CODEC_ID_H264;
	video_enc->time_base= (AVRational){3003,90000};
	video_enc->width = mWidth;
	video_enc->height = mHeight;
	video_enc->sample_aspect_ratio = st->sample_aspect_ratio = (AVRational) {1, 1};
	//video_enc->has_b_frames = 2;
	video_enc->extradata_size = SPS_PPS_LEN;
	video_enc->extradata = (uint8_t*)mVideoExtra;

	//add audio stream
	if (!(st = av_new_stream(pFormat, pFormat->nb_streams))) return ME_ERROR;
	#if FFMPEG_VER_0_6
	st->filename = pFormat->filename;
	#else
	st->probe_data.filename = pFormat->filename;
	#endif
	avcodec_get_context_defaults2(st->codec, CODEC_TYPE_AUDIO);
	audio_enc = st->codec;
	if (pFormat->oformat->flags & AVFMT_GLOBALHEADER)
		audio_enc->flags |= CODEC_FLAG_GLOBAL_HEADER;
	audio_enc->codec_id = mCodecId;
	audio_enc->bit_rate = mBitrate;
	audio_enc->sample_rate = mSamplingRate;
	audio_enc->channels = mNumberOfChannels;
	audio_enc->sample_fmt = mSampleFmt;
	if (audio_enc->codec_id == CODEC_ID_AC3) {
		if (audio_enc->channels == 1)
			audio_enc->channel_layout = CH_LAYOUT_MONO;
		else if (audio_enc->channels == 2)
			audio_enc->channel_layout = CH_LAYOUT_STEREO;
	}
	//
	if (mCodecId == CODEC_ID_AAC && AMBA_AAC ) {
		/*************************************************
		audioObjectType; 5 [2 AAC LC]
		samplingFrequencyIndex; 4 [0x3 48000,0x8 16000,0xb 8000]
		channelConfiguration; 4
		frameLengthFlag;1
		dependsOnCoreCoder; 1
		extensionFlag; 1
		**************************************************/
		audio_enc->extradata_size = 2;
		//Fix me, Hard code for AAC LC/48000/2/0/0/0
		mAudioExtra[0] = 0x11;//0001 0001
		mAudioExtra[1] = 0x90;//1001 0000
		audio_enc->extradata = (uint8_t*)mAudioExtra;
	}
	if (!(codec = avcodec_find_encoder(audio_enc->codec_id))) {
		AM_ERROR("Failed to find audio codec\n");
		return ME_ERROR;
	}
	if (avcodec_open(audio_enc, codec) < 0) {
		AM_ERROR("Failed to open audio codec\n");
		return ME_ERROR;
	}

	if (!(file_oformat->flags & AVFMT_NOFILE)) {
		if (url_fopen(&pFormat->pb, pFileNameSuffix, URL_WRONLY) < 0) {
			AM_ERROR("Failed to open '%s'\n", pFileNameSuffix);
			return ME_ERROR;
		}
	}

	if (av_write_header(pFormat) < 0) {
		AM_ERROR("Failed to write header\n");
		return ME_ERROR;
	}

	mpFormat = pFormat;
	AM_INFO("WriteHeader done ");

#ifdef RECORD_VID_FILE
	if ((err = mpVFile->CreateFile("/data/vid.264")) != ME_OK) return err;
#endif
#ifdef RECORD_AUD_FILE
	if ((err = mpAFile->CreateFile("/data/aud.aac")) != ME_OK) return err;
#endif

	return ME_OK;
}

AM_ERR CAmbaFFMpegMuxer::WriteTail(AM_UINT code)
{
	AM_INFO("WriteTail .... ");
	if (av_write_trailer(mpFormat)<0) {
		AM_ERROR(" av_write_trailer err\n");
		return ME_ERROR;
	}
	AM_INFO("WriteTail done");

#ifdef RECORD_VID_FILE
	mpVFile->CloseFile();
#endif
#ifdef RECORD_AUD_FILE
	mpAFile->CloseFile();
#endif

	Clear();
	mbTailWritten = true;
	PostEngineMsg(code);
	return ME_OK;
}

AM_UINT
CAmbaFFMpegMuxer::ReadBit(AM_U8 *pBuffer,
	AM_INT *value, AM_U8 *bit_pos = 0, AM_UINT num = 1)
{
	*value = 0;
	AM_UINT i = 0, j;
	for (j = 0; j < num; j++) {
		if (*bit_pos == 8) {
			*bit_pos = 0;
			i++;
		}

		if (*bit_pos == 0) {
			if (pBuffer[i] == 0x03  &&
				pBuffer[i - 1] == 0 &&
				pBuffer[i - 2] == 0)
				i++;
		}

		*value <<= 1;
		*value += pBuffer[i] >> (7 - (*bit_pos)++) &0x1;
	}
	return i;
}

AM_UINT
CAmbaFFMpegMuxer::GetSpsPpsLen(AM_U8 *pBuffer)
{
	AM_UINT start_code, times, pos;
	unsigned char tmp;

	times = pos = 0;
	for (start_code = 0xffffffff; times < 3; pos++) {
		tmp = (unsigned char)pBuffer[pos];
		start_code <<= 8;
		start_code |= (unsigned int)tmp;
		if (start_code == 0x00000001) {
			times = times + 1;
		}
	}
	AM_INFO("sps-pps length = %d\n", pos - 4);
	return pos - 4;
}

void CAmbaFFMpegMuxer::OnVideoBuffer(CBuffer *pBuffer)
{
	AM_INFO("OnVideoBuffer .... ");
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

	if(mVideoExtraLen == 0) {
		if(pBuffer->mFrameType == 1) {
			AM_INFO("Beginning to calculate sps-pps's length.\n");
			mVideoExtraLen = GetSpsPpsLen(pBuffer->GetDataPtr ());
			AM_INFO("Calculate sps-pps's length completely.\n");
			memcpy(mVideoExtra, packet.data, mVideoExtraLen + 8);
			// mpFormat->streams[0]->codec->extradata_size = mVideoExtraLen;
		}
	}

	if ((ret = av_write_frame(mpFormat, &packet)) != 0) {
		AM_ERROR("OnVideoBuffer error!\n");
		WriteTail(IEngine::MSG_ERROR);
		return;
	}
	AM_INFO("OnVideoBuffer done, size:%d,pts:%d\n",packet.size,(int)packet.pts);

#ifdef RECORD_VID_FILE
	mpVFile->WriteFile(pBuffer->GetDataPtr(), pBuffer->GetDataSize());
#endif
}

void CAmbaFFMpegMuxer::OnAudioBuffer(CBuffer *pBuffer)
{
	AM_INFO("OnAudioBuffer ...\n");
	AVPacket packet;
	int ret = 0;
	mDuration = pBuffer->GetPTS(),
	av_init_packet(&packet);
	AM_UINT header_length = 0;

	AM_INFO("AMBA_AAC = %d\n", AMBA_AAC);
	if (AMBA_AAC == 1) {
		AM_INT syncword;
		AM_U8 bit_pos = 0;
		AM_U8 *pAdts_header = pBuffer->GetDataPtr ();
		pAdts_header += ReadBit (pAdts_header, &syncword, &bit_pos, 12);
		if (syncword != 0xFFF) {
			AM_ERROR("syncword != 0xFFF [%s-%s-%s]", __FILE__, __FUNCTION__, __LINE__);
		}

		bit_pos += 3;
		AM_INT protection_absent;
		pAdts_header += ReadBit (pAdts_header, &protection_absent, &bit_pos);
		pAdts_header = pBuffer->GetDataPtr () + 6;
		bit_pos = 6;

		header_length = 7;
		AM_INT number_of_raw_data_blocks_in_frame;
		pAdts_header += ReadBit (pAdts_header,
			&number_of_raw_data_blocks_in_frame, &bit_pos, 2);
		if (number_of_raw_data_blocks_in_frame == 0) {
			if (protection_absent == 0) {
				header_length += 2;
			}
		}
	}

	AM_INFO("OnAudioBuffer, header_length = %d\n", header_length);

	packet.stream_index = 1;
	packet.size = pBuffer->GetDataSize() - header_length;
	packet.data = pBuffer->GetDataPtr() + header_length;
	packet.pts = av_rescale_q(mDuration, (AVRational){1,1000}, mpFormat->streams[packet.stream_index]->time_base);

	if ((ret = av_write_frame(mpFormat, &packet)) != 0) {
		AM_ERROR("OnAudioBuffer error!\n");
		WriteTail(IEngine::MSG_ERROR);
		return;
	}
	AM_INFO("OnAudioBuffer done,size:%d,pts:%d\n",packet.size,(int)packet.pts);

#ifdef RECORD_AUD_FILE
	mpAFile->WriteFile(pBuffer->GetDataPtr(), pBuffer->GetDataSize());
#endif

}

void CAmbaFFMpegMuxer::OnRun()
{
	AM_INFO("OnRun .... ");
	if (WriteHeader() != ME_OK) return CmdAck(ME_ERROR);
	else CmdAck(ME_OK);
	AM_INFO("OnRun OK ");

	CQueueInputPin *pPin;
	CBuffer *pBuffer;
	mbRunFlag = true;
	mbVideoEOS = mbAudioEOS = mbTailWritten = false;
	mDuration = mFileSize = 0;

	while(true) {
		if (!WaitInputBuffer(pPin, pBuffer)) {
			AM_ERROR("WaitInputBuffer failed when get MSG\n");
			break;
		}

		if (!mbRunFlag) {
			pBuffer->Release();
			break;
		}

		if (pBuffer->GetType() == CBuffer::DATA) {
			if (mbTailWritten)
				AM_INFO("Discarding DATA...");
			else {
				if (((CAmbaFFMpegMuxerInput*)pPin)->mType == CAmbaFFMpegMuxerInput::VIDEO)
					OnVideoBuffer(pBuffer);
				else if (((CAmbaFFMpegMuxerInput*)pPin)->mType == CAmbaFFMpegMuxerInput::AUDIO)
					OnAudioBuffer(pBuffer);
			}
		} else if (pBuffer->GetType() == CBuffer::EOS) {
			if (((CAmbaFFMpegMuxerInput*)pPin)->mType == CAmbaFFMpegMuxerInput::VIDEO)
				mbVideoEOS = true;
			else if (((CAmbaFFMpegMuxerInput*)pPin)->mType == CAmbaFFMpegMuxerInput::AUDIO)
				mbAudioEOS = true;

			if (mbVideoEOS && mbAudioEOS){ // end OnRun
				if (!mbTailWritten) WriteTail(IEngine::MSG_EOS);
				pBuffer->Release();
				break;
			}
		}

		pBuffer->Release();
	}
}

//-----------------------------------------------------------------------
//
// CAmbaAudioEncoderInput
//
//-----------------------------------------------------------------------
CAmbaFFMpegMuxerInput *CAmbaFFMpegMuxerInput::Create(CFilter *pFilter,PinType type)
{
	CAmbaFFMpegMuxerInput *result = new CAmbaFFMpegMuxerInput(pFilter);
	if (result && result->Construct(type) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaFFMpegMuxerInput::Construct(PinType type)
{
	AM_ERR err = inherited::Construct(((CAmbaFFMpegMuxer*)mpFilter)->MsgQ());
	if (err != ME_OK) return err;
	if (type>=0 && type < END) mType = type;
	else return ME_ERROR;
	return ME_OK;
}

