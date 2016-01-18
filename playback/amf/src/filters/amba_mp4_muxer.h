/*
 * amba_mp4_muxer.h
 *
 * History:
 *    2009/5/26 - [Jacky Zhang] created file
 *	  2011/2/24 - [Yi Zhu] modified
 *    2011/5/16 - [Hanbo Xiao] modified
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef AMBA__MP4_MUXER_H__
#define AMBA__MP4_MUXER_H__

class CQueueInputPin;
class CMp4MuxerInput;
class CCreateMp4;

class CMp4Muxer: public CActiveFilter, public IMp4Muxer
{
	typedef CActiveFilter inherited;
	friend class CMp4MuxerInput;

public:
	static CMp4Muxer *Create(IEngine *pEngine);
protected:
	CMp4Muxer(IEngine *pEngine);
	AM_ERR Construct();
	virtual ~CMp4Muxer();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID ifID);
	virtual void Delete();

	// IamFilter
	virtual AM_ERR Stop()
	{
		DoStop();
		inherited::Stop();
		return ME_OK;
	}
	virtual void GetInfo(INFO& info);
	virtual IPin* GetInputPin(AM_UINT index);

protected:
	// CActiveFilter
	virtual void OnRun();
	// IMp4Muxer
	virtual AM_ERR Config(CMP4MUX_CONFIG* pConfig);
	virtual AM_ERR SetOutputFile (const char *pFileName);

private:
	void DoStop ();

protected:
	CMp4MuxerInput	*_pVideo;
	CMp4MuxerInput	*_pAudio;
	CCreateMp4	*_pCreateMp4;
	CMP4MUX_CONFIG	 _Config;
	bool mbRunFlag;

	int mWidth;
	int mHeight;
	int mFramerate;
	

};

//-----------------------------------------------------------------------
//
// CamMp4MuxerInput
//
//-----------------------------------------------------------------------
class CMp4MuxerInput: public CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CMp4Muxer;

public:
	static CMp4MuxerInput* Create(CFilter *pFilter)	{
		CMp4MuxerInput* result = new CMp4MuxerInput(pFilter);
		if (result != NULL && result->Construct() != ME_OK)
		{
			delete result;
			result = NULL;
		}
		return result;
	}
	virtual ~CMp4MuxerInput(){}

protected:
	CMp4MuxerInput(CFilter *pFilter)
		:inherited(pFilter) {
	}
	
	AM_ERR Construct ()
	{
		AM_ERR ret = inherited::Construct(((CMp4Muxer *)mpFilter)->MsgQ());
		if (ret != ME_OK) return ret;
		return ME_OK;
	}

public:
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat) {return ME_OK;}
};

#endif //__MP4_MUXER_H__

