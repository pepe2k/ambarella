/**
 * author_engine.h
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

#ifndef __AUTHOR_ENGINE_H__
#define __AUTHOR_ENGINE_H__

//-----------------------------------------------------------------------
//
// CAuthorEngine
//
//-----------------------------------------------------------------------
class CAuthorEngine: public CBaseEngine, public IRecordEngine, public IRecordControl
{
	typedef CBaseEngine inherited;

public:
	static CAuthorEngine* Create();

private:
	CAuthorEngine();
	AM_ERR Construct();
	virtual ~CAuthorEngine();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid)
	{
		if (refiid == IID_IRecordEngine)
			return (IRecordControl*)this;
		if (refiid == IID_IRecordControl)
			return (IRecordControl*)this;
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

	// IRecordControl
	virtual AM_ERR SetCamera(ICamera *pCamera);
	AM_ERR SetIAV(AM_IAV *pIAV);
	virtual AM_ERR SetVideoFormat(VIDEO_FORMAT format);
	virtual AM_ERR SetOutputFormat(OUTPUT_FORMAT format);

	virtual AM_ERR SetOutputFile(const char *pFileName);
	virtual AM_ERR StartRecord();
	virtual AM_ERR StopRecord();
	virtual void AbortRecord();

private:
	virtual void MsgProc(AM_MSG& msg);

private:
	AM_ERR CreateGraph(void);

private:
	ICamera *mpCamera;
	AM_IAV *mpIAV;
	CEvent *mpEvtExit;

	IFilter *mpVideoEncoderFilter;
	IFilter *mpAudioInputFilter;
	IFilter *mpAudioEncoderFilter;
	IFilter *mpMuxerFilter;
	IFilter *mpMuxerFilter1;

	IEncoder *mpVideoEncoder;
	IEncoder *mpAudioInput;
	IEncoder *mpAudioEncoder;
	IAndroidMuxer *mpMuxer;
	IAndroidMuxer *mpMuxer1; //for dual stream
	bool mbDual;

	STATE mState;
	bool mbNeedRecreate;

	char mFileName[260];
	VIDEO_FORMAT mVideoFormat;
	OUTPUT_FORMAT mOutputFormat;
};

#endif

