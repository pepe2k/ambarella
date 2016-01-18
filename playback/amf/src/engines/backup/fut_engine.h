/**
 * fut_engine.h 
 * FUT stand for Filter Unit Test
 * History:
 *    2010/6/1 - [QXiong Zheng] created file
 *    
 *
 * Copyright (C) 2010-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef  __FUT_ENGINE_H__
#define __FUT_ENGINE_H__

//-----------------
//
//
//------------------
class CFUTEngine: public CBaseEngine, public IFUTEngine, public IFUTControl
{
	typedef CBaseEngine inherited;

public:
	static CFUTEngine* Create(AM_FILTER& filter);
private:
	CFUTEngine(AM_FILTER& filter):
		mFilterID(filter),
		mType(-1),	
		mpLoadFilter(NULL),		
		mpSaveFilter(NULL),		
		mpClockManager(NULL)
		{  mFileName[0]=0; }	
	AM_ERR Construct();
	virtual ~CFUTEngine();

public:
	virtual void* GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	virtual AM_ERR PostEngineMsg(AM_MSG& msg)
	{
		return inherited::PostEngineMsg(msg);
	}
	virtual void* QueryEngineInterface(AM_REFIID refiid);

	virtual AM_ERR SetAppMsgSink(IMsgSink *pAppMsgSink)
	{
		return inherited::SetAppMsgSink(pAppMsgSink);
	}
	virtual AM_ERR SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&), void *context)
	{
		return inherited::SetAppMsgCallback(MsgProc, context);
	}

	//IFUTControl
	virtual AM_ERR UpConnect(AM_FILTER& UpFilter);
	virtual AM_ERR DownConnect(AM_FILTER& DownFilter);
	virtual AM_ERR ConnectDone(const char* pFile, const char* pOutFile, const char* pTypes);
	virtual AM_ERR ReConnect();
	virtual AM_ERR SaveStream(const char* pdistFile);
	virtual AM_ERR PlayTest();
	virtual AM_ERR StopPlay();
	virtual AM_ERR GetFUTInfo(FUTINFO& info);
	virtual AM_ERR PrintfFUTGraph();
private:
	AM_ERR DoConnect(const char* pFile);
	AM_ERR ConnectSaveFilter();
	AM_ERR ConnectLoadFilter();	
	AM_ERR CheckConnect(const char* pOutFile);
	
	AM_ERR LoadFile();
	AM_ERR LoadDemuxerInput();
	AM_ERR LoadFileInput();
	AM_ERR SetType(const char * pType);
	
	void UpInsert(FilterList* UpNode);
	void DownInsert(FilterList* DownNode);
	void DeleteList();
	bool IsDemuxFilter(FilterList* pNode);
	bool IsRanderFilter(FilterList* pNode);
	filter_entry* GetFilterFromList(AM_FILTER&);	
private:
	virtual void MsgProc(AM_MSG& msg);
	void HandleEOSMsg(AM_MSG& msg);
	void HandleReadyMsg(AM_MSG& msg);
	AM_ERR EnterErrorState(AM_ERR err);
private:
	char mFileName[260];
	AM_ERR mError;  

	AM_FILTER mFilterID;
	IFilter* mpFilter;
	int mType;
	
	FilterList* mpCurUp;
	FilterList* mpCurDown;
	FilterList* mpHead;
	FilterList* mpTail;

	IFilter* mpLoadFilter;
	IFilter* mpSaveFilter;

	IClockManager	*mpClockManager;

};

#endif

	
	
