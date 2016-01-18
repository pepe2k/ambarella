/*
 * amba_audio_encoder.h
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
#ifndef __AMBA_AUDIO_ENCODER_H__
#define __AMBA_AUDIO_ENCODER_H__

class CAmbaAudioEncoder;
class CAmbaAudioEncoderInput;
class CAmbaAudioEncoderOutput;

//-----------------------------------------------------------------------
//
// CAmbaAudioEncoderInput
//
//-----------------------------------------------------------------------
class CAmbaAudioEncoderInput: CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CAmbaAudioEncoder;

public:
	static CAmbaAudioEncoderInput *Create(CFilter *pFilter);

protected:
	CAmbaAudioEncoderInput(CFilter *pFilter):
		inherited(pFilter)
	{}
	AM_ERR Construct();
	virtual ~CAmbaAudioEncoderInput(){}

public:
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat){return ME_OK;}
};
//-----------------------------------------------------------------------
//
// CAmbaAudioEncoderOutput
//
//-----------------------------------------------------------------------
class CAmbaAudioEncoderOutput: public COutputPin
{
	typedef COutputPin inherited;
	friend class CAmbaAudioEncoder;

public:
	static CAmbaAudioEncoderOutput* Create(CFilter *pFilter);

private:
	CAmbaAudioEncoderOutput(CFilter *pFilter):
		inherited(pFilter)
	{
		mMediaFormat.pMediaType = &GUID_Audio;
		mMediaFormat.pSubType = &GUID_Audio_COOK;
		mMediaFormat.pFormatType = &GUID_NULL;
	}
	AM_ERR Construct() { return ME_OK;}
	virtual ~CAmbaAudioEncoderOutput() {}

public:
	// IPin
	virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
	{
		pMediaFormat = &mMediaFormat;
		return ME_OK;
	}

private:
	CMediaFormat mMediaFormat;

};


class CAmbaAudioEncoder: public CActiveFilter, public IEncoder
{
	typedef CActiveFilter inherited ;
	friend class CAmbaAudioEncoderInput;
	friend class CAmbaAudioEncoderOutput;
public:
	static IFilter * Create(IEngine * pEngine);
protected:
	CAmbaAudioEncoder(IEngine * pEngine) :
		inherited(pEngine,"AmbaAudEnc"),
		mpBuf(NULL),
		mpOutput(NULL),
		mpInput(NULL),
		mbRunFlag(false),
		mAction(SA_NONE),
		mSamplingRate(0),
		mNumberOfChannels(0),
		mSampleFmt(SAMPLE_FMT_NONE),
		mBitrate(0),
		mpAudioEncoder(NULL),
		mCodecId(CODEC_ID_NONE)
	{}
	AM_ERR Construct();
	virtual ~CAmbaAudioEncoder();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid)
	{
		if (refiid == IID_IEncoder)
			return (IEncoder*)this;
		return inherited::GetInterface(refiid);
	}
	virtual void Delete() { return inherited::Delete(); }

	// IFilter
	virtual void GetInfo(INFO& info);
	virtual IPin* GetInputPin(AM_UINT index);
	virtual IPin* GetOutputPin(AM_UINT index);
	virtual AM_ERR Stop()
	{
		DoStop(SA_ABORT);
		inherited::Stop();
		return ME_OK;
	}

	// IActiveObject
	virtual void OnRun();

	//IEncoder
	AM_ERR StopEncoding()
	{
		DoStop(SA_STOP);
		return ME_OK;
	}

private:
	void DoStop(STOP_ACTION action);
	AM_ERR OpenEncoder();
	AM_ERR GetParametersFromEngine();

private:
	CSimpleBufferPool *mpBuf;
	CAmbaAudioEncoderOutput * mpOutput;
	CAmbaAudioEncoderInput * mpInput;
	bool mbRunFlag;
	STOP_ACTION mAction;
	int mSamplingRate;
	int mNumberOfChannels;
	SampleFormat mSampleFmt;
	int mBitrate;
	AVCodecContext *mpAudioEncoder;
	CodecID mCodecId;
};
#endif
