/*
 * audio_encoder2.h
 *
 * History:
 *    2010/07/12 - [Jay Zhang] created file
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

class CAudioEncoder2;
class CAudioEncoderInput;
class CAudioEncoderOutput;

class CAudioEncoder2: public CActiveFilter, public IAudioEncoder, public IAudioControl
{
    typedef CActiveFilter inherited ;
    friend class CAudioEncoderOutput;
public:
    static IFilter * Create(IEngine * pEngine);
protected:
	CAudioEncoder2(IEngine * pEngine) :
		inherited(pEngine,"AudioEncoder2"),
		mbEncoding(false),
		mSampleRate(48000),
		mNumOfChannels(1),
		mBitsPerSample(16),
		mpBufferPool(NULL),
		mpOutput(NULL),
		mpAudioEncoder(NULL),
		mCodecId(CODEC_ID_AAC),
		mAe(AUDIO_ENCODER_AAC)
	{}
	AM_ERR Construct();
	virtual ~CAudioEncoder2();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid)
	{
		if (refiid == IID_IAudioEncoder)
			return (IAndroidAudioEncoder*)this;
		return inherited::GetInterface(refiid);
	}
	virtual void Delete() { return inherited::Delete(); }

	// IFilter
	virtual void GetInfo(INFO& info);
	virtual IPin* GetInputPin(AM_UINT index);
	virtual IPin* GetOutputPin(AM_UINT index);
	virtual AM_ERR Stop();

	// IActiveObject
	virtual void OnRun();

        virtual AM_ERR SetAudioParameters(PCM_FORMAT pcmFormat,
            AM_UINT sampleRate, AM_UINT numOfChannels);
	virtual AM_ERR SetAudioEncoder(AUDIO_ENCODER ae);
private:

	AM_ERR OpenEncoder();
    AM_ERR StartRecord();

private:
    volatile bool mbEncoding;
    volatile bool iFirstFrameReceived;

    AM_UINT mSampleRate;
    AM_UINT mNumOfChannels;
    AM_UINT mBitsPerSample;
    AM_UINT mChunkSize;
    AM_UINT mPts;
    AM_UINT mPtsStep;
    AM_UINT mTimeOut;

    CAudioEncoderInput * mpInput;
    CSimpleBufferPool *mpBufferPool;
    CAudioEncoderOutput * mpOutput;

    AVCodecContext *mpAudioEncoder;
    CodecID mCodecId;
    AUDIO_ENCODER  mAe;
};

//-----------------------------------------------------------------------
//
// CAudioEncoderInput
//
//-----------------------------------------------------------------------
class CAudioEncoderInput: CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CAudioEncoder2;

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
	friend class CAudioEncoder2;

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

#endif

