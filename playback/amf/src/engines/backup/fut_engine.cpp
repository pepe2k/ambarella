/**
 * fut_engine.cpp
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

#include <string.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_queue.h"
#include "am_if.h"  //IInterface IMsgPort
#include "am_futif.h" //for IFUTControl (Include)
#include "am_mw.h"
#include "am_base.h"
#include "pbif.h"   //IDEMUX CFileReader and the Ipbengine
#include "fut_if.h"
#include "fut_engine.h"  //for CFUTEngine
#include "filter_list.h"
#include "record_if.h" 

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "filter_registry.h"
#include "fileload_filter.h"
#include "filesave_filter.h"
#include "simple_filesave.h"


IFUTControl* CreateFUTControl(AM_FILTER& filter)
{
	return (IFUTControl*)CFUTEngine::Create(filter);
}

//-----------
//CFUTEngine
//
//--------------
CFUTEngine* CFUTEngine::Create(AM_FILTER& filter)
{
	CFUTEngine *result = new CFUTEngine(filter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CFUTEngine::Construct()
{
	AM_ERR err = inherited::Construct();
	if(err != ME_OK)
		return err;

	if ((mpClockManager = CClockManager::Create()) == NULL)
		return ME_ERROR;
	if(mFilterID != AMBA_NOFILTER)
	{
		filter_entry* pEntry = GetFilterFromList(mFilterID);
		mpFilter = pEntry->create((IEngine*)(inherited*)this);
		if(mpFilter == NULL)
		{
			AM_ERROR("Cannot create filter %s\n", pEntry->pName);
			return ME_NOT_EXIST;
		}
		mpHead = new FilterList;
		if(mpHead ==NULL)
			return ME_NO_MEMORY;
		mpHead->pEntry = pEntry;
		mpHead->pFilter = mpFilter;
		mpHead->pNext = NULL;
	}else{
		mpHead = new FilterList;
		mpHead->pEntry = NULL;
		mpHead->pFilter = NULL;
		mpHead->pNext = NULL;
	}

	mpTail = mpHead;
	mpCurUp = mpCurDown = mpHead;
	return ME_OK;
}

CFUTEngine::~CFUTEngine()
{
	AM_INFO(" ====== Destroy FUTEngine ====== \n");
	ClearGraph();
	DeleteList();
	AM_INFO(" ====== Destroy FUTEngine done ====== \n");
}

void *CFUTEngine::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IFUTControl)
		return (IFUTControl*)this;
	if (refiid == IID_IFUTEngine)
		return (IFUTEngine*)this;
	return inherited::GetInterface(refiid);
}
void* CFUTEngine::QueryEngineInterface(AM_REFIID refiid)
{	
	//must need for the renderer.
	if (refiid == IID_IClockManager)
		return mpClockManager;
	return NULL;
}


filter_entry* CFUTEngine::GetFilterFromList(AM_FILTER& filter)
{
	filter_entry** pListEntry = g_filter_table;
	return *(pListEntry+filter);
}

//AM_ERR CFUTEngine::UpConnect()
//{

//}
//---------------------
//
//maybe more complex.
//-----------------------
void CFUTEngine::UpInsert(FilterList* UpNode)
{
	if(mpHead->pFilter == NULL)
	{
		mpHead = UpNode;
		mpTail = mpHead;
		return;
	}
	UpNode->pNext = mpHead;
	mpHead = UpNode;
	mpCurUp = mpHead;
}
	
void CFUTEngine::DownInsert(FilterList* DownNode)
{
	mpTail->pNext = DownNode;
	mpTail = DownNode;
	mpCurDown = mpTail;
}

void CFUTEngine::DeleteList()
{
	FilterList* pNode = mpHead;
	while(pNode)
	{
		mpHead = mpHead->pNext;
		pNode->pEntry = NULL;
		pNode->pFilter = NULL;
		pNode->pNext = NULL;
		pNode = mpHead;
	}
	mpTail = mpHead;
}

bool CFUTEngine::IsDemuxFilter(FilterList* pHead)
{
	if(pHead->pFilter == NULL) return false;
	IFilter::INFO filterINFO;
	pHead->pFilter->GetInfo(filterINFO);
	return (filterINFO.nInput == 0);
}

bool CFUTEngine::IsRanderFilter(FilterList* pTail)
{
	if(pTail->pFilter == NULL) return false;
	IFilter::INFO filterINFO;
	pTail->pFilter->GetInfo(filterINFO);
	return (filterINFO.nOutput == 0);
}


//--------------------
//
//------------------	
AM_ERR CFUTEngine::UpConnect(AM_FILTER& UpFilter)
{
	if(IsDemuxFilter(mpCurUp))
		return	ME_NOT_EXIST;
	AM_ERR err;

	filter_entry *pf = GetFilterFromList(UpFilter); 
	IFilter* pUpFilter = pf->create((IEngine*)(inherited*)this);
	if(pUpFilter == NULL)
	{
		AM_ERROR("Cannot create filter %s\n", pf->pName);
		return ME_NOT_EXIST;
	}
	FilterList* pNode = new FilterList;
	pNode->pEntry = pf;
	pNode->pFilter = pUpFilter;
	pNode->pNext = NULL;
	
	UpInsert(pNode);
	return ME_OK;
}

AM_ERR CFUTEngine::DownConnect(AM_FILTER& DownFilter)
{
	if(IsRanderFilter(mpCurDown))
		return	ME_NOT_EXIST;
	AM_ERR err;
	
	filter_entry *pf = GetFilterFromList(DownFilter); 
	IFilter* pDownFilter = pf->create((IEngine*)(inherited*)this);
	if(pDownFilter == NULL)
	{
		AM_ERROR("Cannot create filter %s\n", pf->pName);
		return ME_NOT_EXIST;
	}
	FilterList* pNode = new FilterList;
	pNode->pEntry = pf;
	pNode->pFilter = pDownFilter;
	pNode->pNext = NULL;
	
	DownInsert(pNode);
	return ME_OK;
}

AM_ERR CFUTEngine::CheckConnect(const char* pOutFile)
{
	if(!IsDemuxFilter(mpHead))
	{
		IFilter* pUpFilter = CFileLoadFilter::Create((IEngine*)(inherited*) this, FFMPEG_DECODER, sizeof(AVPacket));
		if(pUpFilter == NULL)
		{
			AM_ERROR("Cannot create filter: CFileLoadFilter\n");
			return ME_NOT_EXIST;
		}
		FilterList* pNode = new FilterList;
		pNode->pEntry = NULL;
		pNode->pFilter = pUpFilter;
		pNode->pNext = NULL;	
		UpInsert(pNode);
	}
	if(!IsRanderFilter(mpTail))
	{
		IFilter* pDownFilter = CSimpleFileSave::Create((IEngine*)(inherited*) this, pOutFile);
		if(pDownFilter == NULL)
		{
			AM_ERROR("Cannot create filter: CFileSaveFilter\n");
			return ME_NOT_EXIST;
		}
		FilterList* pNode = new FilterList;
		pNode->pEntry = NULL;
		pNode->pFilter = pDownFilter;
		pNode->pNext = NULL;	
		DownInsert(pNode);
	}
	return ME_OK;
}


AM_ERR CFUTEngine::ConnectDone(const char* pFile, const char* pOutFile, const char* pType)
{
	AM_ERR err;
	err = CheckConnect(pOutFile);
	if(err != ME_OK)
		return ME_NOT_EXIST;
	FilterList* pNode = mpHead;
	err = SetType(pType);
	if(err != ME_OK){
		AM_INFO("==>The type must be one of: audio, video, subtitle\n<==");
		return ME_NOT_EXIST;
	}
		
	while(pNode != NULL)
	{
		err = AddFilter(pNode->pFilter);
		if(err != ME_OK)
			return ME_TOO_MANY;
		pNode = pNode->pNext;
	}

	err = DoConnect(pFile);
	if(err != ME_OK)
		return err;
	
	return ME_OK;
}

AM_ERR CFUTEngine::DoConnect(const char* pFile)
{
	 AM_ERR err;
	 strcpy(mFileName, pFile);
	 err = LoadFile();
	 if(err != ME_OK)
	 	return err;

	 AM_INFO("Renderer (no output pins): %s\n", GetFilterName(mpTail->pFilter));

	 FilterList* pFirst = mpHead;
	 FilterList* pSecond = mpHead->pNext;
	 while(pSecond)
	 {
	 	err = Connect(pFirst->pFilter, 0, pSecond->pFilter, 0);
		if(err != ME_OK)
		{
			AM_ERROR("Can not connect the Pins: %s[0], %s[0]",
				GetFilterName(pFirst->pFilter),GetFilterName(pSecond->pFilter));
			return ME_NOT_EXIST;
		}
		AM_INFO("Connected the Pins:%s[0], %s[0]\n",
				GetFilterName(pFirst->pFilter),GetFilterName(pSecond->pFilter));
	 	pFirst = pSecond;
		pSecond = pSecond->pNext;
	 }
	
	
	return ME_OK;
}

AM_ERR CFUTEngine::ReConnect()
{
	ClearGraph();
	DeleteList();
	
	filter_entry* pEntry = GetFilterFromList(mFilterID);
	mpFilter = pEntry->create((IEngine*)(inherited*)this);
	if(mpFilter == NULL)
	{
		AM_ERROR("Cannot create filter %s\n", pEntry->pName);
		return ME_NOT_EXIST;
	}
	mpHead = new FilterList;
	if(mpHead ==NULL)
		return ME_NO_MEMORY;
	mpHead->pEntry = pEntry;
	mpHead->pFilter = mpFilter;
	mpHead->pNext = NULL;

	mpTail = mpHead;
	mpCurUp = mpCurDown = mpHead;
	return ME_OK;
}

AM_ERR CFUTEngine::ConnectLoadFilter()
{
	return ME_OK;
}

AM_ERR CFUTEngine::ConnectSaveFilter()
{
	return ME_OK;
}

//------------------
//
//
//--------------------
AM_ERR CFUTEngine::LoadDemuxerInput()
{
	AM_ERR err;
	IDemuxer* pDemuxer;
	pDemuxer = IDemuxer::GetInterfaceFrom(mpHead->pFilter);
	if (pDemuxer == NULL) 
	{
		AM_ERROR("Filter with 0 inputPin must privode the IDemuxer.\n");
		//AM_DELETE(pFilter);
		return ME_NO_INTERFACE;
	}
	IFileReader *pFileReader = CFileReader::Create();
	if (pFileReader == NULL) 
		return ME_IO_ERROR;
	parse_file_s parseFile = {mFileName, pFileReader, 0};
	parser_obj_s parser;

	int rel = mpHead->pEntry->parse(&parseFile, &parser);
	if(rel == 0)
	{
		AM_ERROR("The Demuxer cannot parse this file.\n");
		//AM_DELETE(pFilter);
		return ME_NO_INTERFACE;
	}

	err = pDemuxer->LoadFile(mFileName, parser.context);
	if (err != ME_OK) {
		AM_ERROR("LoadFile failed\n");
		//AM_DELETE(pFilter);
		return EnterErrorState(err);
	}
	AM_DELETE(pFileReader);
	return ME_OK;
}


AM_ERR CFUTEngine::LoadFileInput()
{
	AM_ERR err;
	IDemuxer* pDemuxer;
	pDemuxer = IDemuxer::GetInterfaceFrom(mpHead->pFilter);
	if (pDemuxer == NULL) 
	{
		AM_ERROR("Filter with 0 inputPin must privode the IDemuxer.\n");
		return ME_NO_INTERFACE;
	}
	err = pDemuxer->LoadFile(mFileName, &mType);
	if (err != ME_OK) {
		AM_ERROR("LoadFile failed\n");
		return EnterErrorState(err);
	}
	return ME_OK;
}


AM_ERR CFUTEngine::LoadFile()
{
	AM_ERR err;	
	if(mpHead->pEntry)//no the CFileLoadFilter
	{
		err = LoadDemuxerInput();
		if(err != ME_OK)
			return ME_NOT_EXIST;
	}else{				
		err = LoadFileInput();			
		if(err != ME_OK)
			return ME_NOT_EXIST;
	}		
	return ME_OK;
}

AM_ERR CFUTEngine::SetType(const char* pType)
{
	if(strcmp(pType, "video") ==0)
		mType = 0;
	else if (strcmp(pType, "audio") ==0)
		mType =1;
		else if(strcmp(pType, "subtitle")==0)
			mType =2;
			else return ME_NOT_EXIST;
			
	return ME_OK;
}

//-------------------------
//
//
//--------------------------
AM_ERR CFUTEngine::SaveStream(const char* pdistFile)
{
	AM_ERR err;
	ConnectSaveFilter();
	err = LoadFile();
	err = RunAllFilters();
	return err;
}

AM_ERR CFUTEngine::PlayTest()
{
	//todo:test pEntry
	if(!IsRanderFilter(mpTail))
		return	ME_NOT_EXIST;

	AM_ERR err;
	err = RunAllFilters();
	if (err != ME_OK)
		return EnterErrorState(err);
	return ME_OK;	
}

AM_ERR CFUTEngine::StopPlay()
{
	//quit directly;
	return ME_OK;
}

//-----------
//Msg
//
//----------------------
void CFUTEngine::MsgProc(AM_MSG& msg)
{
	//if(!IsSessionMsg(msg))
	//	return;
	switch (msg.code) 
	{
		case MSG_READY:
			AM_INFO("-----MSG_READY-----\n");
			HandleReadyMsg(msg);
			break;
		case MSG_EOS:
			AM_INFO("-----MSG_EOS-----\n");
			HandleEOSMsg(msg);
			break;
		default:
			break;
	}
}

void CFUTEngine::HandleReadyMsg(AM_MSG& msg)
{
	IFilter *pFilter = (IFilter*)msg.p1;

	AM_ASSERT(pFilter != NULL);
	
	AM_UINT index;
	if (FindFilter(pFilter, index) != ME_OK)
		return;
	
	SetReady(index);
	if (AllRenderersReady()) {
		AM_INFO("---All Renderers Ready---\n");
		StartAllRenderers();
	}
}


void CFUTEngine::HandleEOSMsg(AM_MSG& msg)
{
	IFilter *pFilter = (IFilter*)msg.p1;

	AM_ASSERT(pFilter != NULL);
	
	AM_UINT index;
	if (FindFilter(pFilter, index) != ME_OK)
		return;
	
	SetEOS(index);
	if (AllRenderersEOS()) {
		AM_INFO("---All Renderers EOS---\n");
		PostAppMsg(MSG_EOS);
	}
}

AM_ERR CFUTEngine::EnterErrorState(AM_ERR err)
{
	AM_INFO("FUT engine enters ERROR state!\n");
	mError = err;
	PostAppMsg(MSG_STATE_CHANGED);
	return err;
}

//TODO
AM_ERR CFUTEngine::GetFUTInfo(FUTINFO& info)
{
	return ME_OK;
}

AM_ERR CFUTEngine::PrintfFUTGraph()
{
	return ME_OK;
}
