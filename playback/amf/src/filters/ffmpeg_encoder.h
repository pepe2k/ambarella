/*
 * ffmpeg_encoder.h
 *
 * History:
 *    2011/7/27 - [Jay Zhang] created file
 *
 * Copyright (C) 2010-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __FFMPEG_ENCODER_H__
#define __FFMPEG_ENCODER_H__

class CFFMpegEncoderInput;
class CFFMpegEncoderOutput;

class CFFMpegEncoder: public CActiveFilter, public IAudioEncoder
{
	typedef CActiveFilter inherited ;
	friend class CFFMpegEncoderInput;
	friend class CFFMpegEncoderOutput;

public:
	static IFilter * Create(IEngine * pEngine);
protected:
	CFFMpegEncoder(IEngine * pEngine) :
		inherited(pEngine,"FFMpegAudioEncoder"),
		mpInput(NULL),
		mAudioFormat(StreamFormat_AAC),
		mSampleFmt(SampleFMT_S16),
		mSampleRate(0),
		mNumOfChannels(0),
		mBitrate(128000),
		mpAudioEncoder(NULL),
		mCodecId(CODEC_ID_NONE),
            mSpecifiedFrameSize(MAX_AUDIO_BUFFER_SIZE),
		mbNeedFadeIn(true),
		mFadeInBufferNumber(0),
		mFadeInMaxBufferNumber(4),//some default value
		mFadeInCount(0),
            mDumpFileIndex(0)
	{
            for (AM_UINT i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
                mpOutput[i] = NULL;
            }
            mpBufferPool = NULL;

            mpRecConfig = (SConsistentConfig*)pEngine->mpOpaque;
            AM_ASSERT(mpRecConfig);
        }
	AM_ERR Construct();
	virtual ~CFFMpegEncoder();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
    {
        if (refiid == IID_IAudioEncoder)
            return (IAudioEncoder*)this;
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

    // IParameters
    AM_ERR SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index = 0);

    // IFilter
    virtual AM_ERR AddOutputPin(AM_UINT& index, AM_UINT type);

    virtual bool ProcessCmd(CMD& cmd);

#ifdef AM_DEBUG
    virtual void PrintState();
#endif

private:
    void SendoutBuffer(CBuffer* pBuffer);
    AM_ERR OpenEncoder();
    AM_ERR GetParametersFromEngine();

private:
    enum {
        MAX_NUM_OUTPUT_PIN = 2,
    };

    CFFMpegEncoderInput * mpInput;
    CSimpleBufferPool *mpBufferPool;
    CFFMpegEncoderOutput * mpOutput[MAX_NUM_OUTPUT_PIN];

    StreamFormat mAudioFormat;
    AudioSampleFMT mSampleFmt;
    AM_UINT mSampleRate;
    AM_UINT mNumOfChannels;
    AM_UINT mBitrate;
    AVCodecContext *mpAudioEncoder;
    CodecID mCodecId;
    AM_UINT mSpecifiedFrameSize;

//debug use
private:
    SConsistentConfig* mpRecConfig;
    bool mbNeedFadeIn;
    AM_UINT mFadeInBufferNumber;
    AM_UINT mFadeInMaxBufferNumber;
    AM_UINT mFadeInCount;
    AM_UINT mDumpFileIndex;
};

//-----------------------------------------------------------------------
//
// CFFMpegEncoderInput
//
//-----------------------------------------------------------------------
class CFFMpegEncoderInput: CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CFFMpegEncoder;

public:
	static CFFMpegEncoderInput *Create(CFilter *pFilter);

protected:
	CFFMpegEncoderInput(CFilter *pFilter):
		inherited(pFilter)
	{}
	AM_ERR Construct();
	virtual ~CFFMpegEncoderInput(){}

public:
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat){return ME_OK;}
};
//-----------------------------------------------------------------------
//
// CFFMpegEncoderOutput
//
//-----------------------------------------------------------------------
class CFFMpegEncoderOutput: public COutputPin
{
	typedef COutputPin inherited;
	friend class CFFMpegEncoder;

public:
	static CFFMpegEncoderOutput* Create(CFilter *pFilter);

private:
	CFFMpegEncoderOutput(CFilter *pFilter):
		inherited(pFilter)
	{
		mMediaFormat.pMediaType = &GUID_Audio;
		mMediaFormat.pSubType = &GUID_Audio_COOK;
		mMediaFormat.pFormatType = &GUID_NULL;
	}
	AM_ERR Construct() { return ME_OK;}
	virtual ~CFFMpegEncoderOutput() {}

public:
	// IPin
	virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
	{
		pMediaFormat = &mMediaFormat;
		return ME_OK;
	}

private:
	CMediaFormat mMediaFormat;

//test feature, for audio fade in
private:
    AM_UINT mFadeLevel;

};


#endif
