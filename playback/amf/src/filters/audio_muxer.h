/*
 * audio_Muxer.h
 *
 * History:
 *    2010/6/21 - [Luo Fei] create file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AUDIO_MUXER_H__
#define __AUDIO_MUXER_H__


class CAudioMuxer;
class CAudioMuxerInput;

//-----------------------------------------------------------------------
//
// CAudioMuxer
//
//-----------------------------------------------------------------------
class CAudioMuxer: public CFilter, public IAndroidMuxer
{
	typedef CFilter inherited;
	friend class CAudioMuxerInput;

public:
	static IFilter* Create(IEngine* pEngine);

private:
	CAudioMuxer(IEngine *pEngine):
		inherited(pEngine),
		mpInputPin(NULL),
		mpFormat(NULL),
		mpAs(NULL),
		mpOFormat(NULL),
		mFirstFrameReceived(true),
		mAudioSamplingRate(48000),
		mAudioNumberOfChannels(2),
		mAudioBitrate(192000),
		mAe(AUDIO_ENCODER_DEFAULT),
		mOutputFormat(OUTPUT_DEF),
		mCodecId(CODEC_ID_NONE),
		mbError(false)
	{
		mOutputFileName[0]='\0';
	}
	AM_ERR Construct();
	virtual ~CAudioMuxer();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IFilter
	virtual AM_ERR Run();
	virtual AM_ERR Stop();

	virtual void GetInfo(INFO& info);
	virtual IPin* GetInputPin(AM_UINT index);

	// IAndroidMuxer
	virtual AM_ERR SetVideoSize( AM_UINT width, AM_UINT height );
	virtual AM_ERR SetVideoFrameRate( AM_UINT framerate );
	virtual AM_ERR SetOutputFormat( OUTPUT_FORMAT of );
	virtual AM_ERR SetOutputFile(const char *pFileName);
	virtual AM_ERR SetAudioEncoder(AUDIO_ENCODER ae);
	virtual AM_ERR SetAudioParameters(AM_UINT audioSource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate);
	virtual AM_ERR SetMaxDuration(AM_U64 limit);
	virtual AM_ERR SetMaxFileSize(AM_U64 limit);
private:
	void ErrorStop();
	void OnEOS();
	void OnAudioBuffer(CBuffer *pBuffer);
	AM_ERR WriteHeader();

private:
	CAudioMuxerInput *mpInputPin;
	AVFormatContext *mpFormat;
	AVStream *mpAs;
	AVOutputFormat *mpOFormat;
	volatile bool mFirstFrameReceived;
	AM_UINT mAudioSamplingRate;
	AM_UINT mAudioNumberOfChannels;
	AM_UINT mAudioBitrate;
	AUDIO_ENCODER  mAe;
	OUTPUT_FORMAT mOutputFormat;
	CodecID mCodecId;
	bool mbError;
	char mOutputFileName[256];
};

//-----------------------------------------------------------------------
//
// CAudioMuxerInput
//
//-----------------------------------------------------------------------
class CAudioMuxerInput: public CInputPin
{
	typedef CInputPin inherited;
	friend class CAudioMuxer;

public:
	static CAudioMuxerInput* Create(CFilter *pFilter);

private:
	CAudioMuxerInput(CFilter *pFilter):
		inherited(pFilter)
	{}
	AM_ERR Construct();
	virtual ~CAudioMuxerInput();

public:
	// IPin
	virtual void Receive(CBuffer *pBuffer);
	virtual void Purge() {}

	// CInputPin
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);

private:
	bool mbEOS;
};

#endif

