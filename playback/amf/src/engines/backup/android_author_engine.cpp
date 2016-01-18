/**
 * author_engine.cpp
 *
 * History:
 *    2010/1/6 - [Oliver Li] created file
 *    2010/4/1 - [Kaiming Xie] created file
 *    2010/7/6 - [Luo Fei] modified file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
 
#define LOG_NDEBUG 0
#define LOG_TAG "Android_Author_Engine"
#include <stdio.h>

#define AMDROID_DEBUG

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
#include "android_author_engine.h"
#include "camera_if.h"


extern IAndroidRecordControl* CreateAndroidAuthorControl()
{
	return (IAndroidRecordControl*)CAndroidAuthorEngine::Create();
}

//-----------------------------------------------------------------------
//
// CAndroidAuthorEngine
//
//-----------------------------------------------------------------------
CAndroidAuthorEngine* CAndroidAuthorEngine::Create()
{
	CAndroidAuthorEngine *result = new CAndroidAuthorEngine();
	if (result && result->Construct() != ME_OK)  {
		delete result;
		result = NULL;
	}
	return result;
}

CAndroidAuthorEngine::CAndroidAuthorEngine():
	mpCamera(NULL),
	mpIAV(NULL),
	mpEvtExit(NULL),

	mpVideoEncoderFilter(NULL),
	mpAudioInputFilter(NULL),
	mpAudioEncoderFilter(NULL),
	mpMuxerFilter(NULL),
	mpMuxerFilter1(NULL),

	mpVideoEncoder(NULL),
	mpAudioInput(NULL),
	mpAudioEncoder(NULL),
	mpMuxer(NULL),
	mpMuxer1(NULL),
	mbDual(false),

	mState(STATE_STOPPED),
	mbNeedRecreate(true),

	mVideoFormat(VIDEO_AVC),
	mOutputFormat(OUTPUT_MP4),

	mWidth(1920),
	mHeight(1080),
	mFramerate(30),//means default value 29.97
	mAudioSamplingRate(48000),
	mAudioNumOfChannels(2),
	mAudioSource(1),
	mAudioBitrate(192000),
	mAe(AUDIO_ENCODER_MP2),
	mLimitFileSize(0),
	mLimitDuration(0)
{
	mFileName[0] = '\0';
}

AM_ERR CAndroidAuthorEngine::Construct()
{
	AM_ERR err = inherited::Construct();
	mbDual = false;
	if (err != ME_OK)
		return err;
	mpEvtExit = CEvent::Create();
	mpIAV = AM_IAV::getInstance();
	return ME_OK;
}

CAndroidAuthorEngine::~CAndroidAuthorEngine()
{
	mpEvtExit->Delete();
}

void *CAndroidAuthorEngine::QueryEngineInterface(AM_REFIID refiid)
{
	// todo
	return NULL;
}

AM_ERR CAndroidAuthorEngine::SetCamera(ICamera *pCamera)
{
	if (mpIAV != NULL) {
		AM_ERROR(" CAndroidAuthorEngine::SetCamera mpIAV already setted\n");
		return ME_TOO_MANY;
	}
	mpCamera = pCamera;
	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetIAV(AM_IAV *pIAV)
{
	if (mpCamera != NULL) {
		AM_ERROR(" CAndroidAuthorEngine::SetIAV mpCamera already setted\n");
		return ME_TOO_MANY;
	}
	mpIAV = pIAV;
	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetVideoFormat(VIDEO_FORMAT format)
{
	if (format != mVideoFormat) {
		mVideoFormat = format;
		mbNeedRecreate = true;
	}
	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetOutputFormat(OUTPUT_FORMAT format)
{
	if (format != mOutputFormat) {
		mOutputFormat = format;
		mbNeedRecreate = true;
	}
	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetVideoSize(AM_UINT width, AM_UINT height) 
{
	if ( mpIAV == NULL )  {
		AM_ERROR("CAndroidAuthorEngine::SetVideoSize Camera device not setted\n");
		return ME_BAD_STATE;
	}

	if ( mpIAV->SetVideoSize( width, height ) < 0)
	{
		AM_ERROR("CAndroidAuthorEngine::SetVideoSize video size not supported  %dx%d\n", width, height);
		return ME_NOT_SUPPORTED;
	}

	mWidth = width;
	mHeight = height;

	return ME_OK;	
}

AM_ERR CAndroidAuthorEngine::SetVideoFrameRate(AM_UINT framerate)
{
	if ( mpIAV == NULL )  {
		AM_ERROR("CAndroidAuthorEngine::SetVideoSize Camera device not setted\n");
		return ME_BAD_STATE;
	}

	if ( mpIAV->SetVideoFrameRate( framerate ) < 0)
	{
		AM_ERROR("CAndroidAuthorEngine::SetVideoFrameRate not supported  %d\n", framerate);
		return ME_NOT_SUPPORTED;
	}

	mFramerate = framerate;

	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetVideoQuality(AM_UINT  quality)
{
	if ( mpIAV == NULL )  {
		AM_ERROR("CAndroidAuthorEngine::SetVideoQuality Camera device not setted\n");
		return ME_BAD_STATE;
	}

	if ( mpIAV->SetVideoQuality( quality ) < 0)
	{
		AM_ERROR("CAndroidAuthorEngine::SetVideoQuality not supported\n");
		return ME_NOT_SUPPORTED;
	}

	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetMaxDuration(AM_U64 limit)
{
	mLimitDuration = limit;
	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetMaxFileSize(AM_U64 limit)
{
	mLimitFileSize = limit;
	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetOutputFile(const char *pFileName)
{
	if (strcmp(mFileName,pFileName) != 0) {
		strcpy(mFileName, pFileName);
		mbNeedRecreate = true;
	}
	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetAudioEncoder(AUDIO_ENCODER ae)
{
	if (mAe != ae) {
		mAe = ae;
		mbNeedRecreate = true;
	}
	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetAudioParameters(AM_UINT audioSource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate)
{
	if (mAudioSource != (int)audioSource || mAudioSamplingRate != (int)samplingRate ||
		mAudioNumOfChannels != (int)numberOfChannels ||mAudioBitrate != (int)bitrate ) {
		mAudioSource = audioSource;
		mAudioSamplingRate = samplingRate;
		mAudioNumOfChannels = numberOfChannels;
		mAudioBitrate = bitrate;
		mbNeedRecreate = true;
	}
	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetAudioEncoderParameters(IAndroidAudioEncoder *pEncoder)
{
	AM_ERR err;

	err = pEncoder->SetAudioEncoder(IAndroidAudioEncoder::AUDIO_ENCODER(mAe));
	if (err != ME_OK)
		return err;

	err = pEncoder->SetAudioParameters(mAudioSource, mAudioSamplingRate,mAudioNumOfChannels,mAudioBitrate);
	if (err != ME_OK)
		return err;

	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::SetMuxerParameters(IAndroidMuxer *pMuxer)
{
	AM_ERR err;

	err = pMuxer->SetVideoSize( mWidth, mHeight );
	if (err != ME_OK)
		return err;

	err = pMuxer->SetOutputFormat(IAndroidMuxer::OUTPUT_FORMAT( mOutputFormat ) );
	if (err != ME_OK)
		return err;

	err = pMuxer->SetOutputFile(mFileName);
	if (err != ME_OK)
		return err;

	err = pMuxer->SetVideoFrameRate(mFramerate);
	if (err != ME_OK)
		return err;

	err = pMuxer->SetAudioEncoder(IAndroidMuxer::AUDIO_ENCODER(mAe));
	if (err != ME_OK)
		return err;

	err = pMuxer->SetAudioParameters(mAudioSource, mAudioSamplingRate,mAudioNumOfChannels,mAudioBitrate);
	if (err != ME_OK)
		return err;

	err = pMuxer->SetMaxDuration(mLimitDuration);
	if (err != ME_OK)
		return err;

	err = pMuxer->SetMaxFileSize(mLimitFileSize);
	if (err != ME_OK)
		return err;

	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::StartRecord()
{
	AM_ERR err;
	AUTO_LOCK(mpMutex);

	if (mState == STATE_RECORDING) {
		return ME_OK;
	}
	else if (mState == STATE_STOPPED) {

		err = mpIAV->TakeEffectAll();
		if (err != ME_OK) {
			AM_ERROR("CAndroidAuthorEngine::StartRecord mpIAV->TakeEffectAll failed\n");
			return err;
		}

		if (mFileName[0] == '\0') {
			AM_ERROR("CAndroidAuthorEngine::StartRecord Call SetOutputFile first\n");
			return ME_BAD_STATE;
		}

		if (mbNeedRecreate) {
			err = CreateGraph();
			if (err != ME_OK) {
				ClearGraph();
				return err;
			}
			mbNeedRecreate = false;
		}

		err = SetAudioEncoderParameters(mpAudioInput);
		if (err != ME_OK) {
			AM_ERROR("CAndroidAuthorEngine::StartRecord SetAudioEncoderParameters mpAudioInput failed\n");
			return err;
		}

		err = SetAudioEncoderParameters(mpAudioEncoder);
		if (err != ME_OK) {
			AM_ERROR("CAndroidAuthorEngine::StartRecord SetAudioEncoderParameters  mpAudioEncoder failed\n");
			return err;
		}

		err = SetMuxerParameters(mpMuxer);
		if (err != ME_OK) {
			AM_ERROR("CAndroidAuthorEngine::StartRecord setMuxerParameters failed\n");
			return err;
		}

		/*should be in preview state before encoding*/
		if (mpIAV->PreviewEnabled() != true){
			mpIAV->StartPreview();
			//AM_ERROR("CAndroidAuthorEngine::StartRecord not in preview state\n");
			//return ME_BAD_STATE;
		}

		err = RunAllFilters();
		if (err != ME_OK) {
			AM_ERROR("CAndroidAuthorEngine::StartRecord RunAllFilters failed\n");
			StopAllFilters();
			PurgeAllFilters();
			return err;
		}

		mState = STATE_RECORDING;
	}
	else {
		AM_ERROR("state is %d\n", mState);
		return ME_BAD_STATE;
	}
	return ME_OK;
}

AM_ERR CAndroidAuthorEngine::StopRecord()
{
	//mbNeedRecreate = true;
	{
		AUTO_LOCK(mpMutex);

		if (mState == STATE_STOPPED){
			return ME_OK;
		}
		else if (mState == STATE_RECORDING) {

			AM_ERR err;
			err = mpVideoEncoder->StopEncoding();
			AM_ASSERT_OK(err);

			err = mpAudioInput->StopEncoding();
			AM_ASSERT_OK(err);

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

void CAndroidAuthorEngine::AbortRecord()
{
	AUTO_LOCK(mpMutex);
	if (mState != STATE_STOPPED) {
		StopAllFilters();
		NewSession();
		mState = STATE_STOPPED;
	}
}

void CAndroidAuthorEngine::MsgProc(AM_MSG& msg)
{
	
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

AM_ERR CAndroidAuthorEngine::CreateGraph(void)
{
	AM_ERR err = ME_OK;

	if (mpIAV == NULL && mpCamera == NULL) {
		AM_ERROR("CAndroidAuthorEngine::CreateGraph Camera device not setted\n");
		return ME_BAD_STATE;
	}
	int fd = -1;
	if (mpIAV != NULL)
		fd = mpIAV->GetDevFd();
	else if (mpCamera != NULL)
		fd = mpCamera->GetDevFd();
	
	if (fd < 0) {
		AM_ERROR("CAndroidAuthorEngine::CreateGraph No device\n");
		return ME_ERROR;
	}

	ClearGraph();

	// video encoder filter
	if ((mpVideoEncoderFilter = CreateAmbaVideoEncoderFilter((IRecordEngine*)this, fd)) == NULL) {
		return ME_ERROR;
	}
	if ((mpVideoEncoder = IEncoder::GetInterfaceFrom(mpVideoEncoderFilter)) == NULL) {
		return ME_ERROR;
	}
	if ((err = AddFilter(mpVideoEncoderFilter)) != ME_OK)
		return err;

	// audio input filter
	if ((mpAudioInputFilter = CreateAudioInput((IRecordEngine*)this)) == NULL) {
		return ME_ERROR;
	}
	if ((mpAudioInput = IAndroidAudioEncoder::GetInterfaceFrom(mpAudioInputFilter)) == NULL) {
		return ME_ERROR;
	}
	if ((err = AddFilter(mpAudioInputFilter)) != ME_OK)
		return err;

	// audio encoder filter
	if ((mpAudioEncoderFilter = CreateAudioEncoder((IRecordEngine*)this)) == NULL) {
		return ME_ERROR;
	}
	if ((mpAudioEncoder = IAndroidAudioEncoder::GetInterfaceFrom(mpAudioEncoderFilter)) == NULL) {
		return ME_ERROR;
	}
	if ((err = AddFilter(mpAudioEncoderFilter)) != ME_OK)
		return err;

	// ffmpeg muxer
	if ((mpMuxerFilter = CreateFFmpegMuxerEx((IRecordEngine*)this)) == NULL) {
		return ME_ERROR;
	}

	if(mbDual) {
		if ((mpMuxerFilter1 = CreateFFmpegMuxerEx((IRecordEngine*)this)) == NULL) {
			return ME_ERROR;
		}
		if ((mpMuxer1 = IAndroidMuxer::GetInterfaceFrom(mpMuxerFilter1)) == NULL) {
			return ME_ERROR;
		}
		if ((err = AddFilter(mpMuxerFilter1)) != ME_OK)
			return err;
	}

	if ((mpMuxer = IAndroidMuxer::GetInterfaceFrom(mpMuxerFilter)) == NULL) {
		return ME_ERROR;
	}

	if ((err = AddFilter(mpMuxerFilter)) != ME_OK)
		return err;

	// connect
	if ((err = Connect(mpVideoEncoderFilter, 0, mpMuxerFilter, 0)) != ME_OK)
		return err;
	if ((err = Connect(mpAudioInputFilter, 0, mpAudioEncoderFilter, 0)) != ME_OK)
		return err;
	if ((err = Connect(mpAudioEncoderFilter, 0, mpMuxerFilter, 1)) != ME_OK)
		return err;
	if(mbDual) {
		if ((err = Connect(mpVideoEncoderFilter, 1, mpMuxerFilter1, 0)) != ME_OK)
			return err;
	}

	return ME_OK;

}


