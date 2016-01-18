/**
 * am_futif.h 
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

#ifndef __AM_FUTIF_H__
#define __AM_FUTIF_H__

extern const AM_IID IID_IFUTControl;
class filter_entry;
class IFilter;

typedef enum
{
	FFMPEG_DEMUXER = 0,
	FFMPEG_DECODER,
	AMBA_VDEC,
	VIDEO_RENDERER,
	AUDIO_RENDERER,
	AMBA_NOFILTER,
}AM_FILTER;

class IFUTControl : public IMediaControl
{
public:
	struct FUTINFO 
	{
		//TODO
	};
	/*
	struct FilterGraph
	{
		//TODO
	};
	*/
	struct FilterList
	{
		filter_entry* pEntry;
		IFilter* pFilter;
		FilterList* pNext;
	};


public:
	DECLARE_INTERFACE(IFUTControl, IID_IFUTControl);

	virtual AM_ERR PlayTest() = 0;
	virtual AM_ERR StopPlay() = 0;
	virtual AM_ERR GetFUTInfo(FUTINFO& info) = 0;
	virtual AM_ERR PrintfFUTGraph()=0;
	
	virtual AM_ERR UpConnect(AM_FILTER& UpFilter) = 0;
	virtual AM_ERR DownConnect(AM_FILTER& DownFilter) = 0;
	virtual AM_ERR ConnectDone(const char* pFile, const char* pOutFile, const char* pType)=0;
	virtual AM_ERR ReConnect()=0;
	virtual AM_ERR SaveStream(const char* pdistFile) = 0;
};

extern IFUTControl* CreateFUTControl(AM_FILTER& filter);


#endif







