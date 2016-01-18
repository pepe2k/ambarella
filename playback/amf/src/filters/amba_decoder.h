
/*
 * amba_decoder.h
 *
 * History:
 *    2009/12/18 - [Oliver Li] create file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBA_RENDERER_H__
#define __AMBA_RENDERER_H__

class CAmbaBSB;
class CAmbaVideoDecoder;
class CAmbaVideoInput;


//-----------------------------------------------------------------------
//
// CAmbaBSB
//
//-----------------------------------------------------------------------
class CAmbaBSB: public CSimpleBufferPool
{
	typedef CSimpleBufferPool inherited;
	friend class CAmbaVideoInput;

public:
	static CAmbaBSB* Create(int iavFd);

protected:
	CAmbaBSB(int iavFd):
		inherited("DecBSB"),
		mIavFd(iavFd), mpMutex(NULL) {}
	AM_ERR Construct();
	virtual ~CAmbaBSB();

public:
	virtual bool AllocBuffer(CBuffer*& pBuffer, AM_UINT size);

public:
	AM_ERR AllocBSB(AM_UINT size);
	void Reset()
	{
		mpCurrAddr = mpStartAddr;
		mSpace = mpEndAddr - mpStartAddr;
	}

private:
	int mIavFd;
	CMutex *mpMutex;

	AM_U8 *mpStartAddr;	// BSB start address
	AM_U8 *mpEndAddr;	// BSB end address
	AM_U8 *mpCurrAddr;	// current pointer to BSB
	AM_U32 mSpace;		// free space in BSB
};

//-----------------------------------------------------------------------
//
// CAmbaVideoDecoder
//
//-----------------------------------------------------------------------
class CAmbaVideoDecoder: public CFilter
{
	typedef CFilter inherited;
	friend class CAmbaVideoInput;

public:
	static IFilter* Create(IEngine *pEngine);
	static int AcceptMedia(CMediaFormat& format);

private:
	CAmbaVideoDecoder(IEngine *pEngine):
		inherited(pEngine),
		mpVideoInputPin(NULL),
		mpBSB(NULL),
		mIavFd(-1)
	{}
	AM_ERR Construct();
	virtual ~CAmbaVideoDecoder();

public:
	// IFilter
	virtual AM_ERR Run();
	virtual AM_ERR Stop();

	virtual void GetInfo(INFO& info);
	virtual IPin* GetInputPin(AM_UINT index);
	//virtual IamPin* GetOutputPin(AM_UINT index);

private:
	CAmbaVideoInput *mpVideoInputPin;
	CAmbaBSB *mpBSB;
	int mIavFd;
};

//-----------------------------------------------------------------------
//
// CAmbaVideoInput
//
//-----------------------------------------------------------------------
class CAmbaVideoInput: public CActiveInputPin
{
	typedef CActiveInputPin inherited;
	friend class CAmbaVideoDecoder;

public:
	static CAmbaVideoInput* Create(CFilter *pFilter, int iavFd);

private:
	CAmbaVideoInput(CFilter *pFilter, int iavFd): 
		inherited(pFilter, "AmbaVDecIn"),
		mIavFd(iavFd)
	{}
	AM_ERR Construct();
	virtual ~CAmbaVideoInput();

public:
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
	virtual AM_ERR ProcessBuffer(CBuffer *pBuffer);

//	virtual void OnRun();

private:
	AM_ERR DecodeBuffer(CBuffer *pBuffer, int numPics);
	void FillEOS(CBuffer *pBuffer);
	AM_ERR WaitEOS(am_pts_t pts);

private:
	int mIavFd;
	bool mbConfigDecoder;
};

#endif

