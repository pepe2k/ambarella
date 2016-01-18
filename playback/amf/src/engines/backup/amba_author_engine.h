/**
 * amba_author_engine.h
 *
 * History:
 *    2011/4/13 - [Luo Fei] created file
 *
 * Copyright (C) 2010-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBA_AUTHOR_ENGINE_H__
#define __AMBA_AUTHOR_ENGINE_H__

//-----------------------------------------------------------------------
//
// CAmbaAuthorEngine
//
//-----------------------------------------------------------------------
class CAmbaAuthorEngine: public CBaseEngine, public IRecordEngine, public IAmbaAuthorControl
{
	typedef CBaseEngine inherited;

public:
	static CAmbaAuthorEngine* Create();

private:
	CAmbaAuthorEngine();
	AM_ERR Construct();
	virtual ~CAmbaAuthorEngine();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid)
	{
		if (refiid == IID_IRecordEngine)
			return (IRecordControl*)this;
		if (refiid == IID_IAmbaAuthorControl)
			return (IAmbaAuthorControl*)this;
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

	// IAmbaAuthorControl
	virtual AM_ERR StartRecord();
	virtual AM_ERR StopRecord();
	virtual void AbortRecord();
	virtual AM_ERR SetOutputFile(const char *pFileName);
	virtual void GetAudioParam(int * samplingRate, int * numOfChannels, int * bitrate, int * sampleFmt, int * codecId);
	virtual void SetAudioParam(int samplingRate, int numOfChannels, int bitrate, int  sampleFmt, int codecId);
	virtual void GetVideoParam(int * width, int * height, int * framerate, int * quality);
	virtual void SetVideoParam(int width, int height, int framerate, int quality);

private:
	virtual void MsgProc(AM_MSG& msg);
	AM_ERR CreateGraph(void);

private:
	CEvent *mpEvtExit;
	IFilter *mpVideoEncoderFilter;
	IFilter *mpAudioInputFilter;
	IFilter *mpAudioEncoderFilter;
	IFilter *mpMuxerFilter;
	IEncoder *mpVideoEncoder;
	IEncoder *mpAudioInput;
	IEncoder *mpAudioEncoder;
	IMuxer *mpMuxer;
	IMp4Muxer *mpMp4Muxer;
	STATE mState;

	//parameters
	int mVideoWidth;
	int mVideoHeight;
	int mVideoFramerate;
	int mVideoQuality;
	int mAudioSamplingRate;
	int mAudioNumOfChannels;
	int mAudioBitrate;
	int mAudioSampleFmt;
	int mAudioCodecId;

	char mFileName[128];
	unsigned long long mLimitFileSize;
	unsigned long long mLimitDuration;
};

#endif

