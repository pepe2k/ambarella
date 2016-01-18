/**
 * simple_filesave.h
 *
 * History: 
 *    2010/8/20 - [QXiong Zheng] created file
 *    create this filter for saving demuxer's packet
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

 #ifndef  __SIMPLE_FILESAVE_FILTER_H__
 #define  __SIMPLE_FILESAVE_FILTER_H__

//-----------------------------------------------------------------------
//
// CFileSaveInput
//
//-----------------------------------------------------------------------
class CSimpleFileSaveInput : public CActiveInputPin
{
	typedef CActiveInputPin inherited;
	friend class CSimpleFilesSave;
public:
	static CSimpleFileSaveInput* Create(CFilter* pFilter, const char* pName);
	
protected:
	CSimpleFileSaveInput(CFilter *pFilter, const char* pName):inherited(pFilter, pName)
		{}
	AM_ERR Construct();
	virtual ~CSimpleFileSaveInput();

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
// CSimpleFileSave
//
//-----------------------------------------------------------------------
class CSimpleFileSave:public CFilter
{
	typedef CFilter inherited;
	friend class CSimpleFileSaveInput;
	
public:
	static IFilter* Create(IEngine *pEngine, const char* pFile);

private:
	CSimpleFileSave(IEngine *pEngine, const char* pFile):
		inherited(pEngine),	
		mbIsAudio(false),
		mbFirstBuffer(true),
		mpFileName(pFile),
		mpWriter(NULL),
		mpInput(NULL)
		{}
	AM_ERR Construct();
	virtual ~CSimpleFileSave();
	
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
	AM_ERR SaveBuffer(CBuffer * pBuffer);
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

	//CFFMpegMediaFormat* mpFormat;
	IFileWriter *mpWriter;
	CSimpleFileSaveInput* mpInput;
};

#endif

