/*
 * amba_ffmepg_muxer_ex.h
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
#ifndef __AMBA_FFMPEG_MUXER_H__
#define __AMBA_FFMPEG_MUXER_H__

class CAmbaFFMpegMuxer;
class CAmbaFFMpegMuxerInput;


//-----------------------------------------------------------------------
//
// CAmbaFFMpegMuxerInput
//
//-----------------------------------------------------------------------
class CAmbaFFMpegMuxerInput: CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CAmbaFFMpegMuxer;

public:
	enum PinType {
		VIDEO,
		AUDIO,
		END,
	};
	static CAmbaFFMpegMuxerInput *Create(CFilter *pFilter,PinType type);

protected:
	CAmbaFFMpegMuxerInput(CFilter *pFilter):
		inherited(pFilter)
	{}
	AM_ERR Construct(PinType type);
	virtual ~CAmbaFFMpegMuxerInput(){}
	PinType mType;
public:
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat){ return ME_OK;}
};

//-----------------------------------------------------------------------
//
// CAmbaFFMpegMuxer
//
//-----------------------------------------------------------------------
class CAmbaFFMpegMuxer: public CActiveFilter, public IMuxer
{
	typedef CActiveFilter inherited ;
	friend class CAmbaFFMpegMuxerInput;
public:
	static IFilter * Create(IEngine * pEngine);

protected:
	CAmbaFFMpegMuxer(IEngine * pEngine) :
		inherited(pEngine,"AmbaFfmpegMuxer"),
		mpVideoInput(NULL),
		mpAudioInput(NULL),
		mbRunFlag(false),
		mbTailWritten(false),
		mbVideoEOS(false),
		mbAudioEOS(false),
		mVideoExtraLen(0),
		mAudioExtraLen(0),
		mpVFile(NULL),
		mpAFile(NULL),
		mpFormat(NULL),
		mCodecId(CODEC_ID_NONE),
		mWidth(0),
		mHeight(0),
		mFramerate(0),
		mSamplingRate(0),
		mNumberOfChannels(0),
		mBitrate(0),
		mSampleFmt(SAMPLE_FMT_NONE),
		mLimitDuration(0),
		mDuration(0),
		mLimitFileSize(0),
		mFileSize(0)
	{
		mVideoExtra[0] = '\0';
		mAudioExtra[0] = '\0';
		mOutputFileName[0]='\0';
	}
	AM_ERR Construct();
	virtual ~CAmbaFFMpegMuxer();
public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IFilter
	virtual void GetInfo(INFO& info);
	virtual IPin* GetInputPin(AM_UINT index);

	// IMuxer
	virtual AM_ERR SetOutputFile(const char *pFileName);
	virtual AM_ERR Stop()
	{
		DoStop();
		inherited::Stop();
		return ME_OK;
	}

	// IActiveObject
	virtual void OnRun();

protected:
	void Clear();

private:
	void OnVideoBuffer(CBuffer *pBuffer);
	void OnAudioBuffer(CBuffer *pBuffer);
	void DoStop();
	AM_ERR WriteHeader();
	AM_ERR WriteTail(AM_UINT code);
	AM_ERR GetParametersFromEngine();
	AM_UINT ReadBit(AM_U8 *pBuffer, AM_INT *value, AM_U8 *bit_pos, AM_UINT type);
	AM_UINT GetSpsPpsLen (AM_U8 *pBuffer);

private:
	CAmbaFFMpegMuxerInput * mpVideoInput;
	CAmbaFFMpegMuxerInput * mpAudioInput;
	bool mbRunFlag;

	bool mbTailWritten;
	bool mbVideoEOS;
	bool mbAudioEOS;

	int mVideoExtraLen;
	AM_U8 mVideoExtra[64];
	int mAudioExtraLen;
	AM_U8 mAudioExtra[2];
	IFileWriter *mpVFile;
	IFileWriter *mpAFile;
	AVFormatContext *mpFormat;
	CodecID mCodecId;

	int mWidth;
	int mHeight;
	int mFramerate;

	int mSamplingRate;
	int mNumberOfChannels;
	int mBitrate;
	SampleFormat mSampleFmt;

	AM_U64 mLimitDuration;
	AM_U64 mDuration;
	AM_U64 mLimitFileSize;
	AM_U64 mFileSize;
	char mOutputFileName[128];
};
#endif