
/**
 * audio_rec_engine.h
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

#ifndef __AUDIO_REC_ENGINE_H__
#define __AUDIO_REC_ENGINE_H__

//-----------------------------------------------------------------------
//
// CAudioRecEngine
//
//-----------------------------------------------------------------------
class CAudioRecEngine: public CBaseEngine, public IRecordEngine, public IAndroidRecordControl
{
	typedef CBaseEngine inherited;

public:
	static CAudioRecEngine* Create();

private:
	CAudioRecEngine();
	AM_ERR Construct();
	virtual ~CAudioRecEngine();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid)
	{
		if (refiid == IID_IRecordEngine)
			return (IRecordControl*)this;
		if (refiid == IID_IAndroidRecordControl)
			return (IAndroidRecordControl*)this;
		return inherited::GetInterface(refiid);
	}
	virtual void Delete() { inherited::Delete(); }

	// IEngine
	virtual AM_ERR PostEngineMsg(AM_MSG& msg)
	{
		return inherited::PostEngineMsg(msg);
	}

	virtual void *QueryEngineInterface(AM_REFIID refiid);

	// IMediaControl
	virtual AM_ERR SetAppMsgSink(IMsgSink *pAppMsgSink)
	{
		return inherited::SetAppMsgSink(pAppMsgSink);
	}
	virtual AM_ERR SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&), void *context)
	{
		return inherited::SetAppMsgCallback(MsgProc, context);
	}

	// IAndroidRecordControl
	virtual AM_ERR SetCamera(ICamera *pCamera);
	AM_ERR SetIAV(AM_IAV *pIAV);
	virtual AM_ERR SetVideoFormat(VIDEO_FORMAT format);
	virtual AM_ERR SetOutputFormat(OUTPUT_FORMAT format);

	virtual AM_ERR SetOutputFile(const char *pFileName);
	virtual AM_ERR StartRecord();
	virtual AM_ERR StopRecord();
	virtual void AbortRecord();

	virtual AM_ERR SetVideoSize(AM_UINT width, AM_UINT height);
	virtual AM_ERR SetVideoFrameRate(AM_UINT framerate);
	virtual AM_ERR SetVideoQuality(AM_UINT quality);
	virtual AM_ERR SetAudioEncoder(AUDIO_ENCODER ae);
	virtual AM_ERR SetAudioParameters(AM_UINT audioSource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate);
	virtual AM_ERR SetMaxDuration(AM_U64 limit);
	virtual AM_ERR SetMaxFileSize(AM_U64 limit);

private:
	virtual void MsgProc(AM_MSG& msg);
	AM_ERR SetMuxerParameters(IAndroidMuxer *pMuxer);
	AM_ERR SetAudioEncoderParameters(IAndroidAudioEncoder *pEncoder);

private:
	AM_ERR CreateGraph(void);

private:
	CEvent *mpEvtExit;
	IFilter *mpAudioInputFilter;
	IFilter *mpAudioEncoderFilter;
	IFilter *mpMuxerFilter;

	IAndroidAudioEncoder *mpAudioInput;
	IAndroidAudioEncoder *mpAudioEncoder;
	IAndroidMuxer *mpMuxer;

	STATE mState;
	bool mbNeedRecreate;

	int mAudioSource;
	int mAudioSamplingRate;
	int mAudioNumOfChannels;
	int mAudioBitrate;
	AUDIO_ENCODER mAe;

	OUTPUT_FORMAT mOutputFormat;
	char mFileName[256];
};

#endif

