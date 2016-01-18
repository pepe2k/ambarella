/*
 * ffmpeg_muxer_ori.h
 *
 * History:
 *    2010/3/4 - [Kaiming Xie] created file
 *    2010/7/6 - [Luo Fei] modified file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FFMPEG_MUXER_ORI_H__
#define __FFMPEG_MUXER_ORI_H__
class CFFMpegMuxerOri;
class CFFMpegMuxerInputOri;

//-----------------------------------------------------------------------
//
// CFFMpegMuxerOri
//
//-----------------------------------------------------------------------
class CFFMpegMuxerOri: public CFilter, public IMuxer
{
	typedef CFilter inherited;
	friend class CFFMpegMuxerInputOri;

public:
	static IFilter* Create(IEngine *pEngine);

protected:
	CFFMpegMuxerOri(IEngine *pEngine):
		inherited(pEngine),
		mpFormat(NULL),
		mVideo(-1),
		mAudio(-1),
		mpVideoInput(NULL),
		mpAudioInput(NULL),
		mpVFile(NULL),
		mExtraLen(0),
		mbRuning(false),
		mbWriteTail(false),
		mpMutex(NULL),
		mbEOS(false),
		mOutputFormat(2)
	{
		mvideoExtra[0] = '\0';
	}
	AM_ERR Construct();
	virtual ~CFFMpegMuxerOri();

protected:
	void Clear();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IFilter
	virtual void GetInfo(INFO& info);
	virtual IPin* GetInputPin(AM_UINT index);
	// IFilter
	virtual AM_ERR Run();
	virtual AM_ERR Stop();
	// IMuxer
	virtual AM_ERR SetOutputFile(const char *pFileName);

private:
	void OnVideoBuffer(CBuffer *pBuffer);
	void OnAudioBuffer(CBuffer *pBuffer);
	void OnEOS();


private:
	AVFormatContext *mpFormat;
	int mVideo;
	int mAudio;
	CFFMpegMuxerInputOri *mpVideoInput;
	CFFMpegMuxerInputOri *mpAudioInput;
	IFileWriter *mpVFile;
	AM_U8 mvideoExtra[64];
	int mExtraLen;
	bool mbRuning;
	bool mbWriteTail;
	CMutex *mpMutex;
	bool mbEOS;
	AM_UINT mOutputFormat;
};

//-----------------------------------------------------------------------
//
// CFFMpegMuxerInputOri
//
//-----------------------------------------------------------------------
class CFFMpegMuxerInputOri: public CInputPin
{
	typedef CInputPin inherited;
	friend class CFFMpegMuxerOri;

public:
	enum PinType {
		VIDEO,
		AUDIO,
		END,
	};

	static CFFMpegMuxerInputOri* Create(CFilter *pFilter, PinType type);

private:
	CFFMpegMuxerInputOri(CFilter *pFilter):
		inherited(pFilter)
	{}
	AM_ERR Construct(PinType type);
	virtual ~CFFMpegMuxerInputOri();

public:
	// IPin
	virtual void Receive(CBuffer *pBuffer);
	virtual void Purge() {}

	// CInputPin
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);

private:
	PinType mType;
};

#endif

