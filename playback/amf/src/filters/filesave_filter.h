/**
 * filesave_filter.h
 *
 * History: 
 *    2010/6/1 - [QXiong Zheng] created file
 *    
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

 #ifndef  __FILESAVE_FILTER_H__
 #define  __FILESAVE_FILTER_H__

//-----------------------------------------------------------------------
//
// CVideoBufferPool
//
//-----------------------------------------------------------------------
class CVideoBufferPool: public CBufferPool
{
	typedef CBufferPool inherited;

public:
	static CVideoBufferPool* Create(AM_UINT size, AM_UINT count);

protected:
	CVideoBufferPool():
		inherited("FileSaveVedioBP"),
		mpBufferVB(NULL),	
		mpLumaMemory(NULL),
		mpChroMemory(NULL)
		{}
	AM_ERR Construct(AM_UINT size, AM_UINT count);
	virtual ~CVideoBufferPool();

public:
	AM_ERR CreateFrameBP(AM_UINT width, AM_UINT height, AM_UINT picWidth, AM_UINT picHeight);
	AM_ERR ReleaseFrameBP();

public:
	virtual bool AllocBuffer(CBuffer*& pBuffer, AM_UINT size);
	//virtual void CloseDevice();
	virtual AM_ERR OpenDevice(AM_UINT width, AM_UINT height, AM_UINT picWidth, AM_UINT picHeight);
protected:
	virtual void OnReleaseBuffer(CBuffer *pBuffer);
private:
	AM_UINT mPicWidth;
	AM_UINT mPicHeight;
	AM_UINT mFbWidth;
	AM_UINT mFbHeight;

	CVideoBuffer* mpBufferVB;
	AM_U8* mpLumaMemory;
	AM_U8* mpChroMemory;
};

//-----------------------------------------------------------------------
//
// CVideoBufferPool2
//
//-----------------------------------------------------------------------
/*
class CVideoBufferPool: public CFrameBufferPool
{
	typedef CFrameBufferPool inherited;

public:
	static CVideoBufferPool *Create(int iavFd);

protected:
	CVideoBufferPool(int iavFd):
		inherited(iavFd)
		{}
	AM_ERR Construct();
	virtual ~CVideoBufferPool();

public:
	//called by the filter->run(), set the framebufferqueue
	AM_ERR CreateFrameBP(AM_UINT width, AM_UINT height, AM_UINT picWidth, AM_UINT picHeight);
	AM_ERR ReleaseFrameBP();

public:
	virtual bool AllocBuffer(CBuffer*& pBuffer, AM_UINT size);
	virtual void CloseDevice();
	virtual AM_ERR OpenDevice(AM_UINT width, AM_UINT height, AM_UINT picWidth, AM_UINT picHeight);
protected:
	virtual void OnReleaseBuffer(CBuffer *pBuffer);

};
*/
//-----------------------------------------------------------------------
//
// CAudioBufferPool
//
//-----------------------------------------------------------------------

class CAudioBufferPool:public CBufferPool
{
	typedef CBufferPool inherited;
public:
	static CAudioBufferPool* Create(AM_UINT size, AM_UINT count);

protected:
	CAudioBufferPool():  inherited("FileSaveAudioBP"), mpBuffers(NULL), mpMemory(NULL) {}
	AM_ERR Construct(AM_UINT size, AM_UINT count);
	virtual ~CAudioBufferPool();
private:
	CBuffer* mpBuffers;
	AM_U8* mpMemory;
};

//-----------------------------------------------------------------------
//
// CFileSaveInput
//
//-----------------------------------------------------------------------
class CFileSaveInput:public CActiveInputPin
{
	typedef CActiveInputPin inherited;
	friend class CFileSaveFilter;
public:
	static CFileSaveInput* Create(CFilter* pFilter, const char* pName);
	
protected:
	CFileSaveInput(CFilter *pFilter, const char* pName):inherited(pFilter, pName)
		{}
	AM_ERR Construct();
	virtual ~CFileSaveInput();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	virtual AM_ERR CheckMediaFormat(CMediaFormat* pMediaFormat);
	virtual void OnRun();

	virtual AM_ERR ProcessBuffer(CBuffer *pBuffer);
private:
	void DoStop();
};

//-----------------------------------------------------------------------
//
// CAudioRendererInput
//
//-----------------------------------------------------------------------
class CFileSaveFilter:public CFilter
{
	typedef CFilter inherited;
	friend class CFileSaveInput;
	
public:
	static IFilter* Create(IEngine *pEngine, const char* pFile);

private:
	CFileSaveFilter(IEngine *pEngine, const char* pFile):
		inherited(pEngine),	
		mbIsAudio(false),
		mbFirstBuffer(true),
		mpFileName(pFile),
		mIavFd(-1),
		mpWriter(NULL),
		mpInput(NULL),
		mpAudioBP(NULL),
		mpVideoBP(NULL)
		{}
	AM_ERR Construct();
	virtual ~CFileSaveFilter();
	
public:
	// IFilter
	virtual AM_ERR Run();
	virtual AM_ERR Stop();
	virtual AM_ERR Start();
	virtual void GetInfo(INFO& info);
	virtual IPin* GetInputPin(AM_UINT index);
	
	virtual void* GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	virtual AM_ERR ProcessBuffer(CBuffer *pBuffer);
	
private:
	AM_ERR IavInit();
	void IavStop();
	AM_ERR ProcessVideoBuffer(CBuffer * pBuffer);
	AM_ERR ProcessAudioBuffer(CBuffer * pBuffer);
	void SetVideoBuffer(AVPicture& pic, CBuffer* pBuffer);
	AM_ERR SetInputFormat(CFFMpegMediaFormat *pFormat);
	AM_ERR SetOutputFile();
	void CloseFiles();
	void WriteEOS();

private:
	bool mbIsAudio;
	bool mbFirstBuffer;
	const char* mpFileName;
	//am_file_off_t mFileSize;

	CFFMpegMediaFormat* mpFormat;
	int mIavFd;
	AM_UINT mAudioBytesWritten;
	AM_UINT mVideoBytesWritten;
	IFileWriter *mpWriter;
	CFileSaveInput* mpInput;

	CAudioBufferPool* mpAudioBP;
	CVideoBufferPool* mpVideoBP;
};

#endif

