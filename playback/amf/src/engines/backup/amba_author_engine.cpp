/**
 *
 * History:
 *    2011/4/6 - [Luo Fei] modified file
 *
 * Copyright (C) 2010-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "Amba_Author_Engine"
#define AMDROID_DEBUG

#include <stdio.h>
#include "am_new.h"
#include "am_types.h"
#include "osal.h"
#include "am_queue.h"
#include "am_if.h"
#include "am_record_if.h"
#include "am_mw.h"
#include "am_base.h"
#include "record_if.h"
#include "am_iav.h"
#include "amba_author_engine.h"

#define LOG_FUNCTION_NAME AM_INFO("%s",  __FUNCTION__)

extern IAmbaAuthorControl* CreateAmbaAuthorControl()
{
	return (IAmbaAuthorControl*)CAmbaAuthorEngine::Create();
}

//-----------------------------------------------------------------------
//
// CAndroidAuthorEngine
//
//-----------------------------------------------------------------------
CAmbaAuthorEngine* CAmbaAuthorEngine::Create()
{
	CAmbaAuthorEngine *result = new CAmbaAuthorEngine();
	if (result && result->Construct() != ME_OK)  {
		delete result;
		result = NULL;
	}
	return result;
}

CAmbaAuthorEngine::CAmbaAuthorEngine():
	mpEvtExit(NULL),
	mpVideoEncoderFilter(NULL),
	mpAudioInputFilter(NULL),
	mpAudioEncoderFilter(NULL),
	mpMuxerFilter(NULL),
	mpVideoEncoder(NULL),
	mpAudioInput(NULL),
	mpAudioEncoder(NULL),
	mpMuxer(NULL),
	mpMp4Muxer(NULL),
	mState(STATE_STOPPED),
	mVideoWidth(1280),
	mVideoHeight(720),
	mVideoFramerate(30),
	mVideoQuality(0),
	mAudioSamplingRate(48000),
	mAudioNumOfChannels(2),
	mAudioBitrate(128000),
	mAudioSampleFmt(1),
	mAudioCodecId(0x15002), //CODEC_ID_MP2= 0x15000, CODEC_ID_MP3, CODEC_ID_AAC, CODEC_ID_AC3,
	mLimitFileSize(0),
	mLimitDuration(0)
{
	mFileName[0] = '\0';
}

AM_ERR CAmbaAuthorEngine::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;
	mpEvtExit = CEvent::Create();
	if (mpEvtExit == NULL)
		return ME_ERROR;
	return ME_OK;
}

CAmbaAuthorEngine::~CAmbaAuthorEngine()
{
	mpEvtExit->Delete();
}

void *CAmbaAuthorEngine::QueryEngineInterface(AM_REFIID refiid)
{
	// todo
	return NULL;
}

AM_ERR CAmbaAuthorEngine::SetOutputFile(const char *pFileName)
{
	LOG_FUNCTION_NAME;
	if (pFileName == NULL) return ME_ERROR;
	strcpy(mFileName, pFileName);
	AM_INFO("mFileName = %s",mFileName);
	return ME_OK;
}

void CAmbaAuthorEngine::GetAudioParam(int * samplingRate, int * numOfChannels, int * bitrate, int * sampleFmt, int *codecId)
{
	if (samplingRate != NULL) *samplingRate = mAudioSamplingRate;
	if (numOfChannels != NULL) *numOfChannels = mAudioNumOfChannels;
	if (bitrate != NULL) *bitrate = mAudioBitrate;
	if (sampleFmt != NULL) *sampleFmt = mAudioSampleFmt;
	if (codecId != NULL) *codecId = mAudioCodecId;
}

void CAmbaAuthorEngine::SetAudioParam(int samplingRate, int numOfChannels, int bitrate, int sampleFmt, int codecId)
{
	mAudioSamplingRate = samplingRate;
	mAudioNumOfChannels = numOfChannels;
	mAudioBitrate = bitrate;
	mAudioSampleFmt = sampleFmt;
	mAudioCodecId = codecId;
}

void CAmbaAuthorEngine::GetVideoParam(int * width, int * height, int * framerate, int * quality)
{
	if (width != NULL) *width = mVideoWidth;
	if (height != NULL) *height = mVideoHeight;
	if (framerate != NULL) *framerate = mVideoFramerate;
	if (quality != NULL) *quality = mVideoQuality;
}

void CAmbaAuthorEngine::SetVideoParam(int width, int height, int framerate, int quality)
{
	mVideoWidth = width;
	mVideoHeight = height;
	mVideoFramerate = framerate;
	mVideoQuality = quality;
}

AM_ERR CAmbaAuthorEngine::StartRecord()
{
	LOG_FUNCTION_NAME;
	AM_ERR err;
	AUTO_LOCK(mpMutex);

	if (mState == STATE_STOPPED) {
		if ((err = CreateGraph()) != ME_OK ||
#if ENABLE_AMBA_MP4==true
			mpMp4Muxer->SetOutputFile(mFileName) != ME_OK) {
#else
			mpMuxer->SetOutputFile(mFileName) !=ME_OK) {
#endif
			ClearGraph();
			return err;
		}

		if ((err = RunAllFilters()) != ME_OK) {
			AM_ERROR("Failed to RunAllFilters\n");
			StopAllFilters();
			PurgeAllFilters();
			return err;
		}
		mState = STATE_RECORDING;
	}
	else if (mState == STATE_RECORDING) {
		return ME_OK;
	}
	else {
		AM_ERROR("State is %d\n", mState);
		return ME_BAD_STATE;
	}

	return ME_OK;
}

AM_ERR CAmbaAuthorEngine::StopRecord()
{
	LOG_FUNCTION_NAME;
	AM_ERR err;
	{
		AUTO_LOCK(mpMutex);

		if (mState == STATE_RECORDING) {
			if ((err = mpVideoEncoder->StopEncoding()) != ME_OK) {
				AM_ERROR("VideoEncoder failed to StopEncoding\n");
				return err;
			}
			if ((err = mpAudioInput->StopEncoding()) != ME_OK) {
				AM_ERROR("AudioInput failed to StopEncoding\n");
				return err;
			}
			mState = STATE_STOPPING;
		}
		else {
			AM_ERROR("state is %d\n", mState);
			return ME_BAD_STATE;
		}
	}
	mpEvtExit->Wait();

	return ME_OK;
}

void CAmbaAuthorEngine::AbortRecord()
{
	AUTO_LOCK(mpMutex);

	if (mState != STATE_STOPPED) {
		StopAllFilters();
		NewSession();
		mState = STATE_STOPPED;
	}
}

void CAmbaAuthorEngine::MsgProc(AM_MSG& msg)
{
	LOG_FUNCTION_NAME;
	AUTO_LOCK(mpMutex);

	if (!IsSessionMsg(msg))
		return;

	switch (msg.code) {
	case MSG_EOS:
		if (mState ==  STATE_STOPPING) {
		 	mState = STATE_STOPPED;
			PostAppMsg(MSG_EOS);
			mpEvtExit->Signal();
		}
		break;
	case MSG_ERROR:
		break;
	case MSG_FILESIZE:
	case MSG_DURATION:
		if (mState != STATE_STOPPED) {
			mpVideoEncoder->StopEncoding();
			mpAudioInput->StopEncoding();
			mState = STATE_STOPPING;
			StopAllFilters();
			PurgeAllFilters();
			NewSession();
			mState = STATE_STOPPED;
		}
		PostAppMsg(msg.code);
		break;
	default:
		break;
	}
}

AM_ERR CAmbaAuthorEngine::CreateGraph(void)
{
	AM_ERR err = ME_OK;

	ClearGraph();

	// video encoder filter
	if ((mpVideoEncoderFilter = CreateAmbaVideoEncoderFilterEx((IRecordEngine*)this)) == NULL)
		return ME_ERROR;
	if ((mpVideoEncoder = IEncoder::GetInterfaceFrom(mpVideoEncoderFilter)) == NULL)
		return ME_ERROR;
	if ((err = AddFilter(mpVideoEncoderFilter)) != ME_OK)
		return err;

	// audio input filter
	if ((mpAudioInputFilter = CreateAmbaAudioInput((IRecordEngine*)this)) == NULL)
		return ME_ERROR;
	if ((mpAudioInput = IEncoder::GetInterfaceFrom(mpAudioInputFilter)) == NULL)
		return ME_ERROR;
	if ((err = AddFilter(mpAudioInputFilter)) != ME_OK)
		return err;

	// audio encoder filter
	if ((mpAudioEncoderFilter = CreateAmbaAudioEncoder((IRecordEngine*)this)) == NULL)
		return ME_ERROR;
	if ((mpAudioEncoder = IEncoder::GetInterfaceFrom(mpAudioEncoderFilter)) == NULL)
		return ME_ERROR;
	if ((err = AddFilter(mpAudioEncoderFilter)) != ME_OK)
		return err;
	
#if ENABLE_AMBA_MP4==true
	// Mp4Muxer muxer
	if ((mpMuxerFilter = CreateAmbaMp4Muxer((IRecordEngine*)this)) == NULL)
		return ME_ERROR;
	if ((mpMp4Muxer = IMp4Muxer::GetInterfaceFrom(mpMuxerFilter)) == NULL)
		return ME_ERROR;
	if ((err = AddFilter(mpMuxerFilter)) != ME_OK)
		return err;
#else
	// ffmpeg muxer
	if ((mpMuxerFilter = CreateAmbaFFMpegMuxer((IRecordEngine*)this)) == NULL)
		return ME_ERROR;
	if ((mpMuxer = IMuxer::GetInterfaceFrom(mpMuxerFilter)) == NULL)
		return ME_ERROR;
	if ((err = AddFilter(mpMuxerFilter)) != ME_OK)
		return err;
#endif
	

	// connect
	if ((err = Connect(mpVideoEncoderFilter, 0, mpMuxerFilter, 0)) != ME_OK)
		return err;
	if ((err = Connect(mpAudioInputFilter, 0, mpAudioEncoderFilter, 0)) != ME_OK)
		return err;
	if ((err = Connect(mpAudioEncoderFilter, 0, mpMuxerFilter, 1)) != ME_OK)
		return err;

	return ME_OK;

}
