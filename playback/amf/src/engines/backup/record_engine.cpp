
/**
 * record_engine.cpp
 *
 * History:
 *    2010/1/6 - [Oliver Li] created file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

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
#include "record_engine.h"
#include "camera_if.h"

extern IRecordControl* CreateRecordControl()
{
	return (IRecordControl*)CRecordEngine::Create();
}

//-----------------------------------------------------------------------
//
// CRecordEngine
//
//-----------------------------------------------------------------------
CRecordEngine* CRecordEngine::Create()
{
	CRecordEngine *result = new CRecordEngine();
	if (result && result->Construct() != ME_OK)  {
		delete result;
		result = NULL;
	}
	return result;
}

CRecordEngine::CRecordEngine():
	mpCamera(NULL),

	mpVideoEncoderFilter(NULL),
	mpAudioEncoderFilter(NULL),
	mpMuxerFilter(NULL),

	mpVideoEncoder(NULL),
	mpAudioEncoder(NULL),
	mpMuxer(NULL),

	mState(STATE_STOPPED),
	mbNeedRecreate(true),

	mVideoFormat(VIDEO_AVC),
	mOutputFormat(OUTPUT_MP4)
{
	mFileName[0] = '\0';
}

AM_ERR CRecordEngine::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	return ME_OK;
}

CRecordEngine::~CRecordEngine()
{
}

void *CRecordEngine::QueryEngineInterface(AM_REFIID refiid)
{
	// todo
	return NULL;
}

AM_ERR CRecordEngine::SetCamera(ICamera *pCamera)
{
	mpCamera = pCamera;
	return ME_OK;
}

AM_ERR CRecordEngine::SetVideoFormat(VIDEO_FORMAT format)
{
	if (format != mVideoFormat) {
		mVideoFormat = format;
		mbNeedRecreate = true;
	}
	return ME_OK;
}

AM_ERR CRecordEngine::SetOutputFormat(OUTPUT_FORMAT format)
{
	if (format != mOutputFormat) {
		mOutputFormat = format;
		mbNeedRecreate = true;
	}
	return ME_OK;
}

AM_ERR CRecordEngine::SetOutputFile(const char *pFileName)
{
	strcpy(mFileName, pFileName);
	return ME_OK;
}

AM_ERR CRecordEngine::StartRecord()
{
	AUTO_LOCK(mpMutex);

	if (mState != STATE_STOPPED) {
		AM_ERROR("Bad state %d\n", mState);
		return ME_BAD_STATE;
	}

	if (mFileName[0] == '\0') {
		AM_ERROR("Call SetOutputFile first\n");
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

	AM_ERR err;

	err = mpMuxer->SetOutputFile(mFileName);
	if (err != ME_OK)
		return err;

	err = RunAllFilters();
	if (err != ME_OK) {
		StopAllFilters();
		PurgeAllFilters();
		return err;
	}

	mState = STATE_RECORDING;

	return ME_OK;
}

AM_ERR CRecordEngine::StopRecord()
{
	AUTO_LOCK(mpMutex);

	if (mState != STATE_RECORDING) {
		AM_ERROR("state is %d\n", mState);
		return ME_BAD_STATE;
	}

	AM_ERR err;
	err = mpVideoEncoder->StopEncoding();
	AM_ASSERT_OK(err);

	mState = STATE_STOPPING;

	return ME_OK;
}

void CRecordEngine::AbortRecord()
{
	AUTO_LOCK(mpMutex);
	if (mState != STATE_STOPPED) {
		StopAllFilters();
		NewSession();
		mState = STATE_STOPPED;
	}
}

void CRecordEngine::MsgProc(AM_MSG& msg)
{
	AUTO_LOCK(mpMutex);

	if (!IsSessionMsg(msg))
		return;

	switch (msg.code) {
	case MSG_EOS:
		if (mState == STATE_STOPPING) {
			mState = STATE_STOPPED;
			PostAppMsg(MSG_EOS);
		}
		break;

	case MSG_ERROR:
		break;

	default:
		break;
	}
}

AM_ERR CRecordEngine::CreateGraph(void)
{
	if (mpCamera == NULL) {
		AM_ERROR("Camera not set\n");
		return ME_BAD_STATE;
	}

	int fd = mpCamera->GetDevFd();
	if (fd < 0) {
		AM_ERROR("No device\n");
		return ME_ERROR;
	}

	ClearGraph();

	// video encoder filter

	if ((mpVideoEncoderFilter = CreateAmbaVideoEncoderFilter((IRecordEngine*)this, fd)) == NULL) {
		return ME_ERROR;
	}
	if ((mpVideoEncoder = IVideoEncoder::GetInterfaceFrom(mpVideoEncoderFilter)) == NULL) {
		AM_ERROR("No interface\n");
		return ME_ERROR;
	}

	AM_ERR err = AddFilter(mpVideoEncoderFilter);
	if (err != ME_OK)
		return err;

	// simple muxer

	if ((mpMuxerFilter = CreateSimpleMuxer((IRecordEngine*)this)) == NULL) {
		return ME_ERROR;
	}
	if ((mpMuxer = IMuxer::GetInterfaceFrom(mpMuxerFilter)) == NULL) {
		AM_ERROR("No interface\n");
		return ME_ERROR;
	}

	if ((err = AddFilter(mpMuxerFilter)) != ME_OK)
		return err;

	// connect

	if ((err = Connect(mpVideoEncoderFilter, 0, mpMuxerFilter, 0)) != ME_OK)
		return err;

	return ME_OK;
}

