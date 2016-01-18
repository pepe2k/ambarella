/**
 * fileload_filter.cpp
 *
 * History:
 *    2010/6/1 - [QXiong Zheng] created file
 *     
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <stdio.h>
#include <sys/time.h>


 
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

#include "am_futif.h"
//#include "am_pbif.h"
#include "pbif.h"	//IDemux, Cfileread
#include "engine_guids.h"
#include "filter_list.h"

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "fileload_filter.h"

#if !FFMPEG_VER_0_6
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_SUBTITLE AVMEDIA_TYPE_SUBTITLE
#define guess_format av_guess_format
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif

/*
int CFileLoadFilter::ParseMedia(const char* pFileName, AM_FILTER filterID, AM_UINT size)
{
	mpFileName = pFileName;
	mFilterID = filterID;
	mSize = size;
	//if(mSize != sizeof(AVPacket))
	//	CFileBufferPool::mObjectSize = mSize;
	return 1;
}*/

//-----------------------------------------------------------------------
//
// CFileBufferPool
//
//-----------------------------------------------------------------------
CFileBufferPool* CFileBufferPool::Create(const char *name, AM_UINT count, AM_UINT size)
{
	CFileBufferPool* result = new CFileBufferPool(name);
	if (result && result->Construct(count, size) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

void CFileBufferPool::OnReleaseBuffer(CBuffer *pBuffer)
{
	if ((pBuffer->GetType() == CBuffer::DATA) && mObjectSize == sizeof(AVPacket)) 
	{
		AVPacket *pPacket = (AVPacket*)((AM_U8*)pBuffer + sizeof(CBuffer));
		av_free_packet(pPacket);
//		static int counter = 0;
//		AM_INFO("release packet %d\n", ++counter);
	}
}

//-----------------------------------------------------------------------
//
// CFileLoadFilter
//
//-----------------------------------------------------------------------
IFilter* CFileLoadFilter::Create(IEngine* pEngine,  AM_FILTER filterID, AM_UINT size)
{
	CFileLoadFilter *result = new CFileLoadFilter(pEngine, filterID, size);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CFileLoadFilter::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;
	if ((mpOutput = CFileLoadOutput::Create(this)) == NULL)
		return ME_ERROR;
	
	if ((mpStreamBP = CFileBufferPool::Create("FileLoadBufferPool", 32, mSize)) == NULL)
			return ME_ERROR;
	
	mpOutput->SetBufferPool(mpStreamBP);		
	return ME_OK;
}

void CFileLoadFilter::Clear()
{
	if (mpFormat) {
		av_close_input_file(mpFormat);
		mpFormat = NULL;
	}
}

CFileLoadFilter::~CFileLoadFilter()
{
	AM_DELETE(mpOutput);
	AM_DELETE(mpStreamBP);
	Clear();
}

void* CFileLoadFilter::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IDemuxer)
		return (IDemuxer*)this;
	return inherited::GetInterface(refiid);
}

void CFileLoadFilter::GetInfo(INFO& info)
{
	inherited::GetInfo(info);
	info.nInput = 0;
	info.nOutput = 1;
//	if (mStream < 0)
		//info.nOutput++;
}

IPin* CFileLoadFilter::GetOutputPin(AM_UINT index)
{
	//if mStream == 0, then also return the OutputPin, in this Case, the LoadFile()
	//will return a ME_ERROR, So, the engine will STOP, then will no afect.
	if (index == 0) 
		return mpOutput;
	return NULL;
}


AM_ERR CFileLoadFilter::LoadFFMpegFile(int type)
{
	AVFormatContext* pFormat;
	av_register_all();
	int rval = av_open_input_file(&pFormat, mpFileName, NULL, 0, NULL);
	if (rval < 0) {
		AM_INFO("ffmpeg does not recognize this File:%s\n", mpFileName);
		return ME_ERROR;
	}
	
	rval = av_find_stream_info(pFormat);
	if (rval < 0) {
		AM_ERROR("av_find_stream_info failed\n");
		return ME_ERROR;
	}
	
#ifdef AM_DEBUG
	dump_format(pFormat, 0, mpFileName, 0);
#endif
	for (AM_UINT i = 0; i < pFormat->nb_streams; i++) 
	{
		if (pFormat->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO && type == 0)
		{
			mStream = i;
			break;
		}
		if (pFormat->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO && type == 1)
		{
			mStream = i;
			break;
		}
//		if (pFormat->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO && type == 2)
//		{
//			mStream = i;
//			break;
//		}
	}
	
	if(mStream<0){
		AM_ERROR("No video, no audio\n");
		return ME_ERROR;
	}
	AM_INFO("The stream of the media: %d\n", mStream);
	AM_ERR err;
	int media = pFormat->streams[mStream]->codec->codec_type;
	err = mpOutput->InitStream(pFormat, mStream, media);
	if (err != ME_OK)
		return err;
	
	mpFormat = pFormat;
	return ME_OK;
}

//TODO
AM_ERR CFileLoadFilter::LoadBitFile()
{
	return ME_OK;
}

AM_ERR CFileLoadFilter::LoadFile(const char *pFileName, void *context)
{
	AM_ERR err;
	mpFileName = pFileName;
	int* pType = (int*)context;
	int type = *pType;
	if(mFilterID == FFMPEG_DECODER)
		err = LoadFFMpegFile(type);
	else
		err = LoadBitFile();
	if(err != ME_OK){
		AM_ERROR("---Load File Error---\n");
		return err;
	}
	return ME_OK;
}


void CFileLoadFilter::OnRunFFMpeg()
{
	//AM_INFO("Case===FFMpeg_Decoder\n");
	AVPacket packet;
	CBuffer *pBuffer;
	AO::CMD cmd;
	int frameNum = 0;
	struct timeval time1;
	struct timeval time2;
	AM_U32 time_use = 0;
	gettimeofday(&time1, NULL);
	while (1)
	{		
		if (mpWorkQ->PeekCmd(cmd)) 
		{
			//CmdAck(ME_OK);
			if (cmd.code == AO::CMD_STOP) {
				//DoStop();
				AM_INFO("Stop FileLoadFilter!\n");
				return;
			}
		}
		//AM_PRINTF("1111111111");
		if(av_read_frame(mpFormat, &packet) >= 0)
		{	
			++frameNum;
			gettimeofday(&time2, NULL);
			time_use = (time2.tv_sec-time1.tv_sec)*1000000+(time2.tv_usec-time1.tv_usec);
			if(time_use >= 1000000)
			{
				AM_INFO("====>the frame nums in one second is : %d ", frameNum);
				time_use = 0;
				time1 = time2;
				frameNum = 0;
			}
			//AM_PRINTF("av_read_frame:get %d\n",frameNum);
			if (packet.stream_index == mStream) 
			{
				if (!mpOutput->AllocBuffer(pBuffer)) {
					AM_PRINTF("Output(FileLoad)->AllocBuffer Fails\n");
					return;
				}
				//AM_PRINTF("1122222111");
				pBuffer->SetType(CBuffer::DATA);
				pBuffer->SetDataSize(packet.size);
				pBuffer->SetDataPtr(NULL);

				::memcpy((AM_U8*)pBuffer + sizeof(CBuffer), &packet, sizeof(packet));
				//AM_PRINTF("Send out: %d\n",frameNum);
				mpOutput->SendBuffer(pBuffer);
				//
				
				//AM_PRINTF("1133111");
			}else{
				av_free_packet(&packet);
			}
		}else
			break;
	}
	if(mStream >= 0)
		if (!SendEOS(mpOutput))
			return;
}

//TODO
void CFileLoadFilter::OnRunBit()
{	
	//AM_INFO("CASE::::FFMPEG_DEMUXER\n");
}

void CFileLoadFilter::OnRun()
{
	CmdAck(ME_OK);
	switch (mFilterID)
	{
		case FFMPEG_DECODER:
			OnRunFFMpeg();
			break;
		//case 
		default:
			OnRunBit();
			break;
	}
}

bool CFileLoadFilter::SendEOS(CFileLoadOutput *pPin)
{
	CBuffer *pBuffer;
	AM_INFO("FileLoadFilter Send EOS\n");
	if (!pPin->AllocBuffer(pBuffer))
		return false;

	pBuffer->SetType(CBuffer::EOS);
	pPin->SendBuffer(pBuffer);
	return true;
}

AM_ERR CFileLoadFilter::Seek(AM_U64 &ms)
{
	return ME_NOT_EXIST;
}


AM_ERR CFileLoadFilter::GetTotalLength(AM_U64& ms)
{
	return ME_NOT_EXIST;
}

AM_ERR CFileLoadFilter::GetFileType(const char *&pFileType)
{
    return ME_NOT_EXIST;
}
//-----------------------------------------------------------------------
//
// CFileLoadOutput
//
//-----------------------------------------------------------------------
CFileLoadOutput* CFileLoadOutput::Create(CFilter *pFilter)
{
	CFileLoadOutput *result = new CFileLoadOutput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

CFileLoadOutput::CFileLoadOutput(CFilter *pFilter):
	inherited(pFilter)
{
}

AM_ERR CFileLoadOutput::Construct()
{
//	AM_ERR err = inherited::Construct();
//	if (err != ME_OK)
//		return err;
	return ME_OK;
}

CFileLoadOutput::~CFileLoadOutput()
{
}

AM_ERR CFileLoadOutput::OnConnect(IPin *pPeer)
{
	if (mpBufferPool == NULL) {
		if ((mpBufferPool = IBufferPool::GetInterfaceFrom(pPeer))) {
			mpBufferPool->AddRef();
		}
		else {
			AM_ERR err = NoBufferPoolHandler();
			if (err != ME_OK)
				return err;
		}
	}
	
	// check
	IBufferPool *pBufferPool = IBufferPool::GetInterfaceFrom(pPeer);
	if (mpBufferPool != pBufferPool) {
		//AM_ERROR("Pin of %s uses different BP\n", FilterName());
		ReleaseBufferPool();
		mpBufferPool = pBufferPool;
	}
	
	return ME_OK;
}

AM_ERR CFileLoadOutput::InitStream(AVFormatContext *pFormat, int stream, int media)
{
	//AVCodecContext *pCodec = pFormat->streams[stream]->codec;
	AVStream *pStream = pFormat->streams[stream];
	AVCodecContext *pCodec = pStream->codec;	

	if (media == CODEC_TYPE_VIDEO) {
		switch (pCodec->codec_id) {
		case CODEC_ID_MPEG1VIDEO:	mMediaFormat.pSubType = &GUID_Video_MPEG12; break;
		case CODEC_ID_MPEG2VIDEO:	mMediaFormat.pSubType = &GUID_Video_MPEG12; break;
		case CODEC_ID_MPEG4:		mMediaFormat.pSubType = &GUID_Video_MPEG4; break;
		case CODEC_ID_H264:		mMediaFormat.pSubType = &GUID_Video_H264; break;

		case CODEC_ID_RV10:		mMediaFormat.pSubType = &GUID_Video_RV10; break;
		case CODEC_ID_RV20:		mMediaFormat.pSubType = &GUID_Video_RV20; break;
		case CODEC_ID_RV30:		mMediaFormat.pSubType = &GUID_Video_RV30; break;
		case CODEC_ID_RV40:		mMediaFormat.pSubType = &GUID_Video_RV40; break;

		case CODEC_ID_MSMPEG4V1:	mMediaFormat.pSubType = &GUID_Video_MSMPEG4_V1; break;
		case CODEC_ID_MSMPEG4V2:	mMediaFormat.pSubType = &GUID_Video_MSMPEG4_V2; break;
		case CODEC_ID_MSMPEG4V3:	mMediaFormat.pSubType = &GUID_Video_MSMPEG4_V3; break;

		case CODEC_ID_WMV1:		mMediaFormat.pSubType = &GUID_Video_WMV1; break;
		case CODEC_ID_WMV2:		mMediaFormat.pSubType = &GUID_Video_WMV2; break;
		case CODEC_ID_WMV3:		mMediaFormat.pSubType = &GUID_Video_WMV3; break;

		case CODEC_ID_H263P:		mMediaFormat.pSubType = &GUID_Video_H263P; break;
		case CODEC_ID_H263I:		mMediaFormat.pSubType = &GUID_Video_H263I; break;

		case CODEC_ID_VC1:		mMediaFormat.pSubType = &GUID_Video_VC1; break;

		default:
			AM_ERROR("Unsupported video format, codec id = 0x%x\n", pCodec->codec_id);
			return ME_ERROR;
		}

		mMediaFormat.pMediaType = &GUID_Video;
		mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Stream;
		//mMediaFormat.format = (AM_INTPTR)pCodec;
		mMediaFormat.format = (AM_INTPTR)pStream;

		return ME_OK;
	}
	else if (media == CODEC_TYPE_AUDIO) {
		switch (pCodec->codec_id) {
		case CODEC_ID_MP2:		mMediaFormat.pSubType = &GUID_Audio_MP2; break;
#if PLATFORM_ANDROID
	     case CODEC_ID_MP3PV:
#endif
		case CODEC_ID_MP3:		mMediaFormat.pSubType = &GUID_Audio_MP3; break;
		case CODEC_ID_AAC:		mMediaFormat.pSubType = &GUID_Audio_AAC; break;
		case CODEC_ID_AC3:		mMediaFormat.pSubType = &GUID_Audio_AC3; break;
		case CODEC_ID_DTS:		mMediaFormat.pSubType = &GUID_Audio_DTS; break;

		case CODEC_ID_WMAV1:		mMediaFormat.pSubType = &GUID_Audio_WMAV1; break;
		case CODEC_ID_WMAV2:		mMediaFormat.pSubType = &GUID_Audio_WMAV2; break;

		case CODEC_ID_COOK:		mMediaFormat.pSubType = &GUID_Audio_COOK; break;

		default:
			AM_ERROR("Unsupported audio format, codec id = 0x%x\n", pCodec->codec_id);
			return ME_ERROR;
		}

		mMediaFormat.pMediaType = &GUID_Audio;
		mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Stream;
		//mMediaFormat.format = (AM_INTPTR)pCodec;
		mMediaFormat.format = (AM_INTPTR)pStream;

		return ME_OK;
	}
	return ME_ERROR;
}
