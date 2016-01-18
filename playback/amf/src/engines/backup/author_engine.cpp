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
#define LOG_TAG "Author_engine"
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
#include "author_engine.h"
#include "camera_if.h"


extern IRecordControl* CreateAuthorControl()
{
	return (IRecordControl*)CAuthorEngine::Create();
}

//-----------------------------------------------------------------------
//
// CAuthorEngine
//
//-----------------------------------------------------------------------
CAuthorEngine* CAuthorEngine::Create()
{
	CAuthorEngine *result = new CAuthorEngine();
	if (result && result->Construct() != ME_OK)  {
		delete result;
		result = NULL;
	}
	return result;
}

CAuthorEngine::CAuthorEngine():
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
	mOutputFormat(OUTPUT_MP4)
{
	mFileName[0] = '\0';
}

AM_ERR CAuthorEngine::Construct()
{
	AM_ERR err = inherited::Construct();
	mbDual = false;
	if (err != ME_OK)
		return err;
	mpEvtExit = CEvent::Create();
	return ME_OK;
}

CAuthorEngine::~CAuthorEngine()
{
	mpEvtExit->Delete();
}

void *CAuthorEngine::QueryEngineInterface(AM_REFIID refiid)
{
	// todo
	return NULL;
}

AM_ERR CAuthorEngine::SetCamera(ICamera *pCamera)
{
	if (mpIAV != NULL) {
		AM_ERROR(" CAuthorEngine::SetCamera mpIAV already setted\n");
		return ME_TOO_MANY;
	}
	mpCamera = pCamera;
	return ME_OK;
}

AM_ERR CAuthorEngine::SetIAV(AM_IAV *pIAV)
{
	if (mpCamera != NULL) {
		AM_ERROR(" CAuthorEngine::SetIAV mpCamera already setted\n");
		return ME_TOO_MANY;
	}
	mpIAV = pIAV;
	return ME_OK;
}

AM_ERR CAuthorEngine::SetVideoFormat(VIDEO_FORMAT format)
{
	if (format != mVideoFormat) {
		mVideoFormat = format;
		mbNeedRecreate = true;
	}
	return ME_OK;
}

AM_ERR CAuthorEngine::SetOutputFormat(OUTPUT_FORMAT format)
{
	if (format != mOutputFormat) {
		mOutputFormat = format;
		mbNeedRecreate = true;
	}
	return ME_OK;
}

AM_ERR CAuthorEngine::SetOutputFile(const char *pFileName)
{
	strcpy(mFileName, pFileName);
	return ME_OK;
}

AM_ERR CAuthorEngine::StartRecord()
{
	AM_ERR err;
	AUTO_LOCK(mpMutex);

	if (mState == STATE_RECORDING) {
		return ME_OK;
	}
	else if (mState == STATE_STOPPED) {

		if (mFileName[0] == '\0') {
			AM_ERROR("CAuthorEngine::StartRecord Call SetOutputFile first\n");
			return ME_BAD_STATE;
		}

		if (mbNeedRecreate) {
			AM_ERR err = CreateGraph();
			if (err != ME_OK) {
				ClearGraph();
				return err;
			}
			mbNeedRecreate = false;
		}

		err = mpMuxer->SetOutputFile(mFileName);
		if (err != ME_OK)
			return err;

		if(mbDual) {
			err = mpMuxer1->SetOutputFile("rec1.3gp");
			if (err != ME_OK)
				return err;
		}

		err = RunAllFilters();
		if (err != ME_OK) {
			AM_ERROR("CAuthorEngine::StartRecord RunAllFilters failed\n");
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

AM_ERR CAuthorEngine::StopRecord()
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

void CAuthorEngine::AbortRecord()
{
	AUTO_LOCK(mpMutex);
	if (mState != STATE_STOPPED) {
		StopAllFilters();
		NewSession();
		mState = STATE_STOPPED;
	}
}

void CAuthorEngine::MsgProc(AM_MSG& msg)
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
	default:
		break;
	}
}

AM_ERR CAuthorEngine::CreateGraph(void)
{
	AM_ERR err = ME_OK;

	//if (mpCamera == NULL) {
	//	AM_ERROR("Camera not set\n");
	//	return ME_BAD_STATE;
	//}

	//int fd = mpCamera->GetDevFd();

	if (mpIAV == NULL && mpCamera == NULL) {
		AM_ERROR("CAuthorEngine::CreateGraph Camera device not setted\n");
		return ME_BAD_STATE;
	}
	int fd = -1;
	if (mpIAV != NULL)
		fd = mpIAV->GetDevFd();
	else if (mpCamera != NULL)
		fd = mpCamera->GetDevFd();
	
	if (fd < 0) {
		AM_ERROR("CAuthorEngine::CreateGraph No device\n");
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
	if ((mpAudioInput = IEncoder::GetInterfaceFrom(mpAudioInputFilter)) == NULL) {
		return ME_ERROR;
	}
	if ((err = AddFilter(mpAudioInputFilter)) != ME_OK)
		return err;

	// audio encoder filter
	if ((mpAudioEncoderFilter = CreateAudioEncoder((IRecordEngine*)this)) == NULL) {
		return ME_ERROR;
	}
	if ((mpAudioEncoder = IEncoder::GetInterfaceFrom(mpAudioEncoderFilter)) == NULL) {
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


