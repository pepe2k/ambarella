/*
 * ffmepg_muxer_ex.h
 *
 * History:
 *    2010/7/13 - [Luo Fei] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __FFMPEG_MUXER_EX_H__
#define __FFMPEG_MUXER_EX_H__

class CFFMpegMuxerEx;

//-----------------------------------------------------------------------
//
// CFFMpegMuxerInputEx
//
//-----------------------------------------------------------------------
class CFFMpegMuxerInputEx: CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CFFMpegMuxerEx;

public:
	enum PinType {
		VIDEO,
		AUDIO,
		END,
	};
	static CFFMpegMuxerInputEx *Create(CFilter *pFilter,PinType type);

protected:
	CFFMpegMuxerInputEx(CFilter *pFilter):
		inherited(pFilter)
	{}
	AM_ERR Construct(PinType type);
	virtual ~CFFMpegMuxerInputEx();
	PinType mType;
public:
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
};

//-----------------------------------------------------------------------
//
// CFFMpegMuxerEx
//
//-----------------------------------------------------------------------
class CFFMpegMuxerEx: public CActiveFilter, public IAndroidMuxer
{
	typedef CActiveFilter inherited ;
	friend class CFFMpegMuxerInputEx;
public:
	static IFilter * Create(IEngine * pEngine);

protected:
	CFFMpegMuxerEx(IEngine * pEngine) :
		inherited(pEngine,"FfmpegMuxerEx"),
		mpFormat(NULL),
		mpVideoInput(NULL),
		mpAudioInput(NULL),
		//mpMutex(NULL),
		mbRunning(false),
		mbWriteTail(false),
		mbVideoEOS(false),
		mbAudioEOS(false),
		mExtraLen(0),
		mpVFile(NULL),
		mAudioSamplingRate(48000),
		mAudioNumberOfChannels(2),
		mAudioBitrate(192000),
		mCodecId(CODEC_ID_NONE),
		mAe(AUDIO_ENCODER_DEFAULT),
		mWidth(1920),
		mHeight(1080),
		mFramerate(30),//30->29.97,  60->59.94
		mOutputFormat(OUTPUT_MP4), //mp4
		mLimitDuration(0),
		mDuration(0),
		mLimitFileSize(0),
		mFileSize(0)
	{
		mvideoExtra[0] = '\0';
		mOutputFileName[0]='\0';
	}
	AM_ERR Construct();
	virtual ~CFFMpegMuxerEx();
protected:
	void Clear();
public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IFilter
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
	virtual AM_ERR Stop()
	{
		//at this moment, the OnRun()should have been stopped by EOS.
		DoStop();
		inherited::Stop();
		return ME_OK;
	}

	// IActiveObject
	virtual void OnRun();

private:
	void OnVideoBuffer(CBuffer *pBuffer);
	void OnAudioBuffer(CBuffer *pBuffer);
	void DoStop();
	AM_ERR WriteHeader();
	AM_ERR WriteTailer(AM_UINT code);

private:
	AVFormatContext *mpFormat;
	CFFMpegMuxerInputEx * mpVideoInput;
	CFFMpegMuxerInputEx * mpAudioInput;
	//CMutex *mpMutex;

	bool mbRunning;
	bool mbWriteTail;
	bool mbVideoEOS;
	bool mbAudioEOS;

	int mExtraLen;
	AM_U8 mvideoExtra[64];
	IFileWriter *mpVFile;

	// for ffmpeg write header info
	AM_UINT mAudioSamplingRate;
	AM_UINT mAudioNumberOfChannels;
	AM_UINT mAudioBitrate;
	CodecID mCodecId;
	AUDIO_ENCODER  mAe;
	AM_UINT mWidth;
	AM_UINT mHeight;
	AM_UINT mFramerate;
	OUTPUT_FORMAT mOutputFormat;
	AM_U64 mLimitDuration;
	AM_U64 mDuration;
	AM_U64 mLimitFileSize;
	AM_U64 mFileSize;
	char mOutputFileName[256];
};
#endif
