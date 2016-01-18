
/*
 * android_audio_in.h
 *
 * History:
 *    2010/3/18 - [Kaiming Xie] created file
 *    2010/7/6 - [Luo Fei] modified file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AUDIO_INPUT_H__
#define __AUDIO_INPUT_H__

class CAudioInput;

//-----------------------------------------------------------------------
//
// CAudioInputOutput
//
//-----------------------------------------------------------------------
class CAudioInputOutput: public COutputPin
{
	typedef COutputPin inherited;
	friend class CAudioInput;

public:
	static CAudioInputOutput* Create(CFilter *pFilter);

private:
	CAudioInputOutput(CFilter *pFilter):
		inherited(pFilter)
	{
		mMediaFormat.pMediaType = &GUID_Audio;
		mMediaFormat.pSubType = &GUID_Audio_COOK;
		mMediaFormat.pFormatType = &GUID_NULL;
	}
	AM_ERR Construct();
	virtual ~CAudioInputOutput() {}

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


class CAudioInput: public CActiveFilter, public IAndroidAudioEncoder
{
	typedef CActiveFilter inherited ;
public:
	static IFilter * Create(IEngine * pEngine);
protected:
	CAudioInput(IEngine * pEngine) :
		inherited(pEngine,"AudioInput"),
		mbInputting(false),
		iFirstFrameReceived(false),
		mAudioSource(1),
		mAudioSamplingRate(48000),
		mAudioNumberOfChannels(2),
		mAudioBitrate(192000),
		mFrameSizeInByte(0),
		mpAFile(NULL),
		mpBufferPool(NULL),
		mpOutput(NULL),
		mAction(SA_NONE),
		mpAudioEncoder(NULL),
		mCodecId(CODEC_ID_NONE),
		mAe(AUDIO_ENCODER_AAC)
	{}
	AM_ERR Construct();
	virtual ~CAudioInput();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid)
	{
		if (refiid == IID_IAndroidAudioEncoder)
			return (IAndroidAudioEncoder*)this;
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
		//inherited::Stop();
		return ME_OK;
	}
	virtual AM_ERR SetAudioParameters(AM_UINT audiosource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitrate);
	virtual AM_ERR SetAudioEncoder(AUDIO_ENCODER ae);
private:
	void DoStop(STOP_ACTION action);
	AM_ERR OpenEncoder();
	AM_ERR GetFrameSize();

private:
	volatile bool mbInputting;
	volatile bool iFirstFrameReceived;

	AM_UINT mAudioSource;
	AM_UINT mAudioSamplingRate;
	AM_UINT mAudioNumberOfChannels;
	AM_UINT mAudioBitrate;
	int mFrameSizeInByte;
	IFileWriter *mpAFile;

	CSimpleBufferPool *mpBufferPool;
	CAudioInputOutput * mpOutput;

	STOP_ACTION mAction;
	AVCodecContext *mpAudioEncoder;
	CodecID mCodecId;
	AUDIO_ENCODER  mAe;

};

#endif
