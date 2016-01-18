/**
 * audio_rec_engine.cpp
 *
 * History:
 *    2010/6/18 - [Luo Fei] created file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#include <stdio.h>

#define AMDROID_DEBUG
#define LOG_TAG "audio_rec_engine"
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
#include "camera_if.h"
#include "audio_rec_engine.h"



extern IAndroidRecordControl* CreateAudioRecControl()
{
	return (IAndroidRecordControl*)CAudioRecEngine::Create();
}

//-----------------------------------------------------------------------
//
// CAudioRecEngine
//
//-----------------------------------------------------------------------
CAudioRecEngine* CAudioRecEngine::Create()
{
	CAudioRecEngine *result = new CAudioRecEngine();
	if (result && result->Construct() != ME_OK)  {
		delete result;
		result = NULL;
	}
	return result;
}

CAudioRecEngine::CAudioRecEngine():
	mpEvtExit(NULL),
	mpAudioInputFilter(NULL),
	mpAudioEncoderFilter(NULL),
	mpMuxerFilter(NULL),

	mpAudioInput(NULL),
	mpAudioEncoder(NULL),
	mpMuxer(NULL),

	mState(STATE_STOPPED),
	mbNeedRecreate(true),
	mAudioSource(1),
	mAudioSamplingRate(48000),
	mAudioNumOfChannels(2),
	mAudioBitrate(192000),
	mAe(AUDIO_ENCODER_MP2),
	mOutputFormat(OUTPUT_MP4)
{
	mFileName[0] = '\0';
}

AM_ERR CAudioRecEngine::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;
	mpEvtExit = CEvent::Create();
	return ME_OK;
}

CAudioRecEngine::~CAudioRecEngine()
{
}

void *CAudioRecEngine::QueryEngineInterface(AM_REFIID refiid)
{
	return NULL;
}

AM_ERR CAudioRecEngine::SetCamera(ICamera *pCamera)
{
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetIAV(AM_IAV *pIAV)
{
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetVideoFormat(VIDEO_FORMAT format)
{
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetOutputFormat(OUTPUT_FORMAT format)
{
	if (format != mOutputFormat) {
		mOutputFormat = format;
		mbNeedRecreate = true;
	}
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetVideoSize(AM_UINT width, AM_UINT height)
{
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetVideoFrameRate(AM_UINT framerate)
{
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetVideoQuality(AM_UINT  quality)
{
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetMaxDuration(AM_U64 limit)
{
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetMaxFileSize(AM_U64 limit)
{
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetAudioEncoder(AUDIO_ENCODER ae)
{
	mAe =ae;
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetOutputFile(const char *pFileName)
{
	strcpy(mFileName, pFileName);
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetAudioParameters(AM_UINT audioSource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate)
{
	mAudioSamplingRate = samplingRate;
	mAudioNumOfChannels = numberOfChannels;
	mAudioSource = audioSource;
	mAudioBitrate = bitrate;
	return ME_OK;
}

AM_ERR CAudioRecEngine::SetAudioEncoderParameters(IAndroidAudioEncoder *pEncoder)
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

AM_ERR CAudioRecEngine::SetMuxerParameters(IAndroidMuxer *pMuxer)
{
	AM_ERR err;
	err = pMuxer->SetOutputFormat(IAndroidMuxer::OUTPUT_FORMAT( mOutputFormat ) );
	if (err != ME_OK)
		return err;

	err = pMuxer->SetOutputFile(mFileName);
	if (err != ME_OK)
		return err;

	err = pMuxer->SetAudioEncoder(IAndroidMuxer::AUDIO_ENCODER(mAe));
	if (err != ME_OK)
		return err;

	err = pMuxer->SetAudioParameters(mAudioSource, mAudioSamplingRate,mAudioNumOfChannels,mAudioBitrate);
	if (err != ME_OK)
		return err;
	return ME_OK;
}

AM_ERR CAudioRecEngine::StartRecord()
{
	AM_ERR err;
	AUTO_LOCK(mpMutex);

	if (mState == STATE_RECORDING) {
		return ME_OK;
	} else if (mState == STATE_STOPPED) {

		if (mFileName[0] == '\0') {
			AM_ERROR("CAudioRecEngine::StartRecord Call SetOutputFile first\n");
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

		err = SetAudioEncoderParameters(mpAudioInput);
		if (err != ME_OK) {
			AM_ERROR("CAudioRecEngine::StartRecord SetAudioEncoderParameters mpAudioInput failed\n");
			return err;
		}

		err = SetAudioEncoderParameters(mpAudioEncoder);
		if (err != ME_OK) {
			AM_ERROR("CAudioRecEngine::StartRecord SetAudioEncoderParameters  mpAudioEncoder failed\n");
			return err;
		}

		err = SetMuxerParameters(mpMuxer);
		if (err != ME_OK) {
			AM_ERROR("CAudioRecEngine::StartRecord setMuxerParameters failed\n");
			return err;
		}

		err = RunAllFilters();
		if (err != ME_OK) {
			AM_ERROR("CAudioRecEngine::StartRecord RunAllFilters failed\n");
			StopAllFilters();
			PurgeAllFilters();
			return err;
		}

		mState = STATE_RECORDING;
	} else {
		AM_ERROR("state is %d\n", mState);
		return ME_BAD_STATE;
	}
	return ME_OK;
}

AM_ERR CAudioRecEngine::StopRecord()
{
	{
		AUTO_LOCK(mpMutex);

		if (mState == STATE_STOPPED) {
			return ME_OK;
		} else if (mState == STATE_RECORDING) {

			AM_ERR err;
			err = mpAudioInput->StopEncoding();
			AM_ASSERT_OK(err);

			mState = STATE_STOPPING;
		} else {
			AM_ERROR("state is %d\n", mState);
			return ME_BAD_STATE;
		}
	}

	mpEvtExit->Wait();

	return ME_OK;
}

void CAudioRecEngine::AbortRecord()
{
	AUTO_LOCK(mpMutex);
	if (mState != STATE_STOPPED) {
		StopAllFilters();
		NewSession();
		mState = STATE_STOPPED;
	}
}

void CAudioRecEngine::MsgProc(AM_MSG& msg)
{
	AUTO_LOCK(mpMutex);
	if (!IsSessionMsg(msg))
		return;

	switch (msg.code) {
	case MSG_EOS:
		if (mState == STATE_STOPPING) {
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

AM_ERR CAudioRecEngine::CreateGraph(void)
{
	AM_ERR err = ME_OK;

	ClearGraph();

	// audio input filter
	if ((mpAudioInputFilter = CreateAudioInput((IRecordEngine*)this)) == NULL) {
		return ME_ERROR;
	}
	if ((mpAudioInput = IAndroidAudioEncoder::GetInterfaceFrom(mpAudioInputFilter)) == NULL) {
		AM_ERROR("No audio input interface");
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

	//  muxer
	if ((mpMuxerFilter = CreateAudioMuxer((IRecordEngine*)this)) == NULL) {
		return ME_ERROR;
	}
	if ((mpMuxer = IAndroidMuxer::GetInterfaceFrom(mpMuxerFilter)) == NULL) {
		AM_ERROR("No interface\n");
		return ME_ERROR;
	}
	if ((err = AddFilter(mpMuxerFilter)) != ME_OK)
		return err;

	// connect
	if ((err = Connect(mpAudioInputFilter, 0, mpAudioEncoderFilter, 0)) != ME_OK)
		return err;
	if ((err = Connect(mpAudioEncoderFilter, 0, mpMuxerFilter, 0)) != ME_OK)
		return err;

	return ME_OK;
}

