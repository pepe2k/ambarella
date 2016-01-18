/**
 * fileload_filter.h
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

 #ifndef  __FILELOAD_FILTER_H__
 #define  __FILELOAD_FILTER_H__

class CFFMpegBufferPool;

//-----------------------------------------------------------------------
//
// CFileBufferPool
//
//-----------------------------------------------------------------------
class CFileBufferPool:public CSimpleBufferPool
{
	typedef CSimpleBufferPool inherited;

public:
	static CFileBufferPool* Create(const char *name, AM_UINT count, AM_UINT size);

protected:
	CFileBufferPool(const char *name):
		inherited(name)
	{}
	AM_ERR Construct(AM_UINT count, AM_UINT size)
	{
		mObjectSize = size;
		return inherited::Construct(count, sizeof(CBuffer) +size);
	}
	virtual ~CFileBufferPool() {}
	virtual void OnReleaseBuffer(CBuffer *pBuffer);

private:
	 AM_UINT mObjectSize;
};

//-----------------------------------------------------------------------
//
// CFileLoadOutput
//
//-----------------------------------------------------------------------
class CFileLoadOutput:public COutputPin
{
	typedef COutputPin inherited;
	friend class CFileLoadFilter;
public:
	static CFileLoadOutput* Create(CFilter* pFilter);
protected:
	CFileLoadOutput(CFilter *pFilter);
	AM_ERR Construct();
	virtual ~CFileLoadOutput();

public:
	AM_ERR InitStream(AVFormatContext *pFormat, int stream, int media);
	virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
	{
		pMediaFormat = &mMediaFormat;
		return ME_OK;
	}
private:
	virtual AM_ERR OnConnect(IPin *pPeer);
private:
	CFFMpegStreamFormat mMediaFormat;
};

//-----------------------------------------------------------------------
//
// CFileLoadFilter
//
//-----------------------------------------------------------------------
class CFileLoadFilter:public CActiveFilter, public IDemuxer
{
	typedef CActiveFilter inherited;
	friend class CFileLoadOutput;
public:
	//static int ParseMedia(const char* pFileName, A M_FILTER filterID, AM_UINT size);
	static IFilter* Create(IEngine* pEngine, AM_FILTER filterID, AM_UINT size);

protected:
	CFileLoadFilter(IEngine* pEngine,  AM_FILTER filterID, AM_UINT size):
		inherited(pEngine, "FileLoadFilter"),
		mpFormat(NULL),
		mpFileName(NULL),
		mFilterID(filterID),
		mSize(size),
		mStream(-1),
		mpOutput(NULL),
		mpStreamBP(NULL)
		{}
	AM_ERR Construct();
	virtual ~CFileLoadFilter();
	void Clear();

public:
	virtual void* GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }
	virtual void GetInfo(INFO& info);
	virtual IPin* GetOutputPin(AM_UINT index);

	virtual void OnRun();
	virtual AM_ERR LoadFile(const char *pFileName, void *pParserObj);
       /*For duplex mode*/
       virtual bool isSupportedByDuplex(){return false;}
	AM_ERR Seek(AM_U64 &ms);
	AM_ERR GetTotalLength(AM_U64& ms);
	AM_ERR GetFileType(const char *&pFileType);
	virtual void EnableAudio(bool enable) {}
	virtual void EnableVideo(bool enable) {}
    virtual void EnableSubtitle(bool enable) {}
        /*A5S DV Playback Test */
    virtual AM_ERR  A5SPlayMode(AM_UINT mode){return ME_OK;}
    virtual AM_ERR  A5SPlayNM(AM_INT start_n, AM_INT end_m){return ME_OK;}

private:
	AM_ERR LoadFFMpegFile(int type);
	AM_ERR LoadBitFile();
	void OnRunFFMpeg();
	void OnRunBit();
	bool SendEOS(CFileLoadOutput * pPin);

private:
	AVFormatContext* mpFormat;
	const char* mpFileName;
 	AM_FILTER mFilterID;
	AM_UINT mSize;
	int mStream;

	CFileLoadOutput* mpOutput;
	CFileBufferPool* mpStreamBP;

	IFileReader* mpReader;
};

#endif

