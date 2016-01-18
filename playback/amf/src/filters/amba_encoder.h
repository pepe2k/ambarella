
/*
 * amba_encoder.h
 *
 * History:
 *    2010/1/6 - [Oliver Li] create file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

class CAmbaVideoEncoder;
class CAmbaVideoOutput;

//-----------------------------------------------------------------------
//
// CAmbaVideoOutput
//
//-----------------------------------------------------------------------
class CAmbaVideoOutput: public COutputPin
{
	typedef COutputPin inherited;
	friend class CAmbaVideoEncoder;

public:
	static CAmbaVideoOutput* Create(CFilter *pFilter);

private:
	CAmbaVideoOutput(CFilter *pFilter):
		inherited(pFilter)
	{
		mMediaFormat.pMediaType = &GUID_Video;
		mMediaFormat.pSubType = &GUID_AmbaVideoAVC;
		mMediaFormat.pFormatType = &GUID_NULL;
	}
	AM_ERR Construct();
	virtual ~CAmbaVideoOutput() {}

public:
	// IPin
	virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
	{
		pMediaFormat = &mMediaFormat;
		return ME_OK;
	}

private:
	void ResetForEncoding();
	am_pts_t FixPTS(AM_U32 dsp_pts);

private:
	CMediaFormat mMediaFormat;
	bool mbEncodingStarted;
	AM_UINT mnFrames;
	AM_U32 mLastDspPts;
	am_pts_t mLastPts;
};

//-----------------------------------------------------------------------
//
// CAmbaVideoEncoder
//
//-----------------------------------------------------------------------
class CAmbaVideoEncoder: public CActiveFilter, public IEncoder
{
	typedef CActiveFilter inherited;

public:
	static IFilter* Create(IEngine *pEngine, int fd);

private:
	CAmbaVideoEncoder(IEngine *pEngine, int fd):
		inherited(pEngine, "AmbaVEnc"),
		mpBSB(NULL),
		mpOutput(NULL),
		mFd(fd),
		mbMemMapped(false),
		mbEncoding(false),
		mAction(SA_NONE)
	{}
	AM_ERR Construct();
	virtual ~CAmbaVideoEncoder();

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

	// IVideoEncoder
	AM_ERR StopEncoding()
	{
		DoStop(SA_STOP);
		//inherited::Stop();
		return ME_OK;
	}

    	AM_ERR ExitPreview()
	{
		return ME_OK;
	}

private:
	void DoStop(STOP_ACTION action);
	void PrintH264Config(iav_h264_config_t *config);

private:
	CSimpleBufferPool *mpBSB;
	CAmbaVideoOutput *mpOutput;
	CAmbaVideoOutput *mpOutput1;
	int mFd;
	bool mbMemMapped;
	bool mbEncoding;
	STOP_ACTION mAction;
	iav_h264_config_t mH264Config;

	bool mbDual;
};

