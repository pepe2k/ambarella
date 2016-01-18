/*
 * amba_audio_input.h
 *
 * History:
 *    2011/4/13 - [Luo Fei] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AMBA_AUDIO_INPUT_H__
#define __AMBA_AUDIO_INPUT_H__

class CAmbaAudioInput;
class CAmbaAudioInputOutput;

//-----------------------------------------------------------------------
//
// CAmbaAudioInputOutput
//
//-----------------------------------------------------------------------
class CAmbaAudioInputOutput: public COutputPin
{
	typedef COutputPin inherited;
	friend class CAmbaAudioInput;

public:
	static CAmbaAudioInputOutput* Create(CFilter *pFilter);

private:
	CAmbaAudioInputOutput(CFilter *pFilter):
		inherited(pFilter)
	{
		mMediaFormat.pMediaType = &GUID_Audio;
		mMediaFormat.pSubType = &GUID_Audio_COOK;
		mMediaFormat.pFormatType = &GUID_NULL;
	}
	AM_ERR Construct(){ return ME_OK;}
	virtual ~CAmbaAudioInputOutput() {}

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


class CAmbaAudioInput: public CActiveFilter, public IEncoder
{
	typedef CActiveFilter inherited ;
public:
	static IFilter * Create(IEngine * pEngine);
protected:
	CAmbaAudioInput(IEngine * pEngine) :
		inherited(pEngine,"AmbaAudInput"),
		mpBuf(NULL),
		mpOutput(NULL),
		mbRunFlag(false),
		mAction(SA_NONE),
		mSamplingRate(0),
		mNumberOfChannels(0),
		mSampleFmt(SAMPLE_FMT_NONE),
		mSampleSize(0),
		mCodecId(CODEC_ID_NONE),
		mFrameSizeInByte(0),
		mpAFile(NULL)
	{}
	AM_ERR Construct();
	virtual ~CAmbaAudioInput();

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
	virtual IPin* GetOutputPin(AM_UINT index);
	virtual AM_ERR Stop()
	{
		DoStop(SA_ABORT);
		inherited::Stop();
		return ME_OK;
	}

	// IActiveObject
	virtual void OnRun();

	// IAudioEncoder
	AM_ERR StopEncoding()
	{
		DoStop(SA_STOP);
		return ME_OK;
	}

private:
	void DoStop(STOP_ACTION action);
	AM_ERR GetParametersFromEngine();

private:
	CSimpleBufferPool *mpBuf;
	CAmbaAudioInputOutput * mpOutput;
	bool mbRunFlag;
	STOP_ACTION mAction;
	int mSamplingRate;
	int mNumberOfChannels;
	SampleFormat mSampleFmt;
	int mSampleSize;
	CodecID mCodecId;
	int mFrameSizeInByte;
	IFileWriter *mpAFile;
};

#endif
