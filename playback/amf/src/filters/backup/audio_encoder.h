/*
 * audio_encoder.h
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
#ifndef __AUDIO_ENCODER_H__
#define __AUDIO_ENCODER_H__

class CAudioEncoder;

//-----------------------------------------------------------------------
//
// CAudioEncoderInput
//
//-----------------------------------------------------------------------
class CAudioEncoderInput: CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CAudioEncoder;

public:
	static CAudioEncoderInput *Create(CFilter *pFilter);

protected:
	CAudioEncoderInput(CFilter *pFilter):
		inherited(pFilter)
	{}
	AM_ERR Construct();
	virtual ~CAudioEncoderInput();

public:
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
};
//-----------------------------------------------------------------------
//
// CAudioEncoderOutput
//
//-----------------------------------------------------------------------
class CAudioEncoderOutput: public COutputPin
{
	typedef COutputPin inherited;
	friend class CAudioEncoder;

public:
	static CAudioEncoderOutput* Create(CFilter *pFilter);

private:
	CAudioEncoderOutput(CFilter *pFilter):
		inherited(pFilter)
	{
		mMediaFormat.pMediaType = &GUID_Audio;
		mMediaFormat.pSubType = &GUID_Audio_COOK;
		mMediaFormat.pFormatType = &GUID_NULL;
	}
	AM_ERR Construct();
	virtual ~CAudioEncoderOutput() {}

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


class CAudioEncoder: public CActiveFilter, public IAndroidAudioEncoder
{
	typedef CActiveFilter inherited ;
	friend class CAudioEncoderInput;
	friend class CAudioEncoderOutput;
public:
	static IFilter * Create(IEngine * pEngine);
	enum CODEC_TYPE {
		NO_CODEC,
		AMBA_AAC,
		FFMPEG_AAC,
	};
protected:
	CAudioEncoder(IEngine * pEngine) :
		inherited(pEngine,"AudioEncoder"),
		mbEncoding(false),
		iFirstFrameReceived(false),
		mAudioSamplingRate(48000),
		mAudioNumberOfChannels(2),
		mAudioBitrate(192000),
		mpInput(NULL),
		mpBufferPool(NULL),
		mpOutput(NULL),
		mAction(SA_NONE),
		mpAudioEncoder(NULL),
		mCodecId(CODEC_ID_NONE),
		mAe(AUDIO_ENCODER_AAC)
	{}
	AM_ERR Construct();
	virtual ~CAudioEncoder();

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

	// IAudioEncoder
	//OnRun() should be stopped by EOS but not this function.
	//so this function will not be called by engine or something else.
	AM_ERR StopEncoding()
	{
		DoStop(SA_STOP);
		inherited::Stop();
		return ME_OK;
	}
	virtual AM_ERR SetAudioParameters(AM_UINT audioSource, AM_UINT samplingRate, AM_UINT numberOfChannels, AM_UINT bitRate);
	virtual AM_ERR SetAudioEncoder(AUDIO_ENCODER ae);
private:
	void DoStop(STOP_ACTION action);

	AM_ERR OpenEncoder();

private:
	volatile bool mbEncoding;
	volatile bool iFirstFrameReceived;

	AM_UINT mAudioSamplingRate;
	AM_UINT mAudioNumberOfChannels;
	AM_UINT mAudioBitrate;

	CAudioEncoderInput * mpInput;
	CSimpleBufferPool *mpBufferPool;
	CAudioEncoderOutput * mpOutput;

	STOP_ACTION mAction;

	AVCodecContext *mpAudioEncoder;
	CodecID mCodecId;
	AUDIO_ENCODER  mAe;


};
#endif
