
/*
 * simple_muxer.h
 *
 * History:
 *    2010/1/8 - [Oliver Li] create file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __SIMPLE_MUXER_H__
#define __SIMPLE_MUXER_H__


class CSimpleMuxer;
class CSimpleMuxerInput;

//-----------------------------------------------------------------------
//
// CSimpleMuxer
//
//-----------------------------------------------------------------------
class CSimpleMuxer: public CFilter, public IMuxer
{
	typedef CFilter inherited;
	friend class CSimpleMuxerInput;

private:
	struct video_frame_t
	{
		AM_U32		size;
		AM_U32		pts;
		AM_U32		flags;
		AM_U32		seq;
	};

public:
	static IFilter* Create(IEngine* pEngine);

private:
	CSimpleMuxer(IEngine *pEngine):
		inherited(pEngine),
		mpInputPin(NULL),
		mpVideoFile(NULL),
		mpInfoFile(NULL),
		mbFileCreated(false)
	{}
	AM_ERR Construct();
	virtual ~CSimpleMuxer();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IFilter
	virtual AM_ERR Run();
	virtual AM_ERR Stop();

	virtual void GetInfo(INFO& info);
	virtual IPin* GetInputPin(AM_UINT index);

	// IMuxer
	virtual AM_ERR SetOutputFile(const char *pFileName);
	virtual AM_ERR SetThumbNailFile(const char *pThumbNailFileName);

private:
	void CloseFiles();
	void ErrorStop();

private:
	void OnEOS();
	void OnVideoInfo(CBuffer *pBuffer);
	void OnVideoBuffer(CBuffer *pBuffer);

private:
	CSimpleMuxerInput *mpInputPin;
	IFileWriter *mpVideoFile;
	IFileWriter *mpInfoFile;

	am_file_off_t mVideoFileSize;
	am_file_off_t mInfoFileSize;
	bool mbError;
	bool mbFileCreated;
};

//-----------------------------------------------------------------------
//
// CSimpleMuxerInput
//
//-----------------------------------------------------------------------
class CSimpleMuxerInput: public CInputPin
{
	typedef CInputPin inherited;
	friend class CSimpleMuxer;

public:
	static CSimpleMuxerInput* Create(CFilter *pFilter);

private:
	CSimpleMuxerInput(CFilter *pFilter):
		inherited(pFilter)
	{}
	AM_ERR Construct();
	virtual ~CSimpleMuxerInput();

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

