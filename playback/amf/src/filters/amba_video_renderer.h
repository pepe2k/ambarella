
/*
 * amba_video_renderer.h
 *
 * History:
 *    2010/10/15 - [Yu Jiankang] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBA_VIDEO_RENDERER_H__
#define __AMBA_VIDEO_RENDERER_H__

class CAmbaFrameBufferPool;
class CAmbaVideoRenderer;
class CAmbaVideoRendererInput;


//-----------------------------------------------------------------------
//
// CAmbaFrameBufferPool
//
//-----------------------------------------------------------------------
class CAmbaFrameBufferPool: public CSimpleBufferPool
{
	typedef CSimpleBufferPool inherited;

public:
	static CAmbaFrameBufferPool* Create(const char *name, AM_UINT count);

protected:
	CAmbaFrameBufferPool(const char *name):
		inherited(name)
	{}
	AM_ERR Construct(AM_UINT count)
	{
	    //for all mode, choose max size
	    AM_UINT buffersize = sizeof(iav_decoded_frame_t) >= sizeof(iav_udec_status_t)? sizeof(iav_decoded_frame_t):sizeof(iav_udec_status_t);
        buffersize += sizeof(CBuffer) + 4;//add 4 for safe
        AMLOG_PRINTF("CAmbaFrameBufferPool Construct, sizeof(iav_udec_status_t) %d, sizeof(iav_decoded_frame_t) %d, sizeof(CBuffer) %d, total %d.\n", sizeof(iav_udec_status_t), sizeof(iav_decoded_frame_t), sizeof(CBuffer), buffersize);
		return inherited::Construct(count, buffersize);
	}
	virtual ~CAmbaFrameBufferPool() {}

protected:
	//virtual void OnReleaseBuffer(CBuffer *pBuffer);
};

//-----------------------------------------------------------------------
//
// CAmbaVideoRendererInput
//
//-----------------------------------------------------------------------
class CAmbaVideoRendererInput: public CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CAmbaVideoRenderer;

public:
	static CAmbaVideoRendererInput *Create(CFilter *pFilter);

protected:
	CAmbaVideoRendererInput(CFilter *pFilter):
		inherited(pFilter)
		{}
	AM_ERR Construct();
	virtual ~CAmbaVideoRendererInput();

public:

	// CPin
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
};

//-----------------------------------------------------------------------
//
// CAmbaVideoRenderer
//
//-----------------------------------------------------------------------
class CAmbaVideoRenderer: public CActiveFilter, public IClockObserver, IVideoOutput, public IRender
{
	typedef CActiveFilter inherited;
	friend class CAmbaVideoRendererInput;

public:
	static IFilter* Create(IEngine *pEngine);
	static int AcceptMedia(CMediaFormat& format);

protected:
	CAmbaVideoRenderer(IEngine *pEngine):
		inherited(pEngine,"AmbaRenderer"),
		mpInput(NULL),
		mpAmbaFrameBufferPool(NULL),
		mIavFd(-1),
		mbVideoStarted(false),
		mpClockManager(NULL),
		mpBuffer(NULL),
		mbDisplayRectNeedUpdate(false),
		mTimeOffset(0),
		mFirstPTS(0xffffffff),
		mLastPTS(0),
		msOldState(STATE_IDLE),
		mAdjustInterval(0),
		mbFlushed(false),
		mbStepMode(false),
		mStepCnt(0)
		{}
	AM_ERR Construct();
	virtual ~CAmbaVideoRenderer();

public:
	// IFilter
	virtual AM_ERR Run();
	virtual AM_ERR Start();

#ifdef AM_DEBUG
    virtual void PrintState();
#endif

	virtual void GetInfo(INFO& info)
	{
		info.nInput = 1;
		info.nOutput = 0;
		info.mPriority = 5;
		info.mFlags = SYNC_FLAG;
		info.pName = "AmbaVideoRenderer";
	}
	virtual IPin* GetInputPin(AM_UINT index)
	{
		if (index == 0)
			return mpInput;
		return NULL;
	}

	virtual bool ProcessCmd(CMD& cmd);

	// IActiveObject
	virtual void OnRun();

	// IClockObserver
	virtual AM_ERR OnTimer(am_pts_t curr_pts);

	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete();

	// IRender
    virtual AM_ERR GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs);

    //IVideoOutput
    virtual AM_ERR Step();
    virtual AM_ERR SetDisplayPositionSize(AM_INT vout, AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height);
    virtual AM_ERR GetDisplayPositionSize(AM_INT vout, AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height);
    virtual AM_ERR SetDisplayPosition(AM_INT vout, AM_INT pos_x, AM_INT pos_y);
    virtual AM_ERR SetDisplaySize(AM_INT vout, AM_INT width, AM_INT height);
    virtual AM_ERR GetDisplayDimension(AM_INT vout, AM_INT* width, AM_INT* height);
    virtual AM_ERR GetVideoPictureSize(AM_INT* width, AM_INT* height);
    virtual AM_ERR SetDisplayFlip(AM_INT vout, AM_INT flip);
    virtual AM_ERR SetDisplayMirror(AM_INT vout, AM_INT mirror);
    virtual AM_ERR SetDisplayRotation(AM_INT vout, AM_INT degree);
    virtual AM_ERR EnableVout(AM_INT vout, AM_INT enable);

    virtual AM_ERR SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h);
    virtual AM_ERR SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h);
    virtual AM_ERR SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y);

private:
	bool ReadyWaitStart();

private:

	CAmbaVideoRendererInput *mpInput;
	CAmbaFrameBufferPool *mpAmbaFrameBufferPool;
	int mIavFd;

	bool mbVideoStarted;
	IClockManager *mpClockManager;
	CBuffer* mpBuffer;
	iav_decoded_frame_t *mpFrame;

protected:
	AM_ERR SetInputFormat(CMediaFormat *pFormat);
	void RenderBuffer(CBuffer*& pBuffer);
	void DiscardBuffer(CBuffer*& pBuffer);

private:
    AM_U32 mPicWidth, mPicHeight;
    SVoutConfig mVoutConfig[eVoutCnt];//0:lcd, 1:hdmi
    bool mbDisplayRectNeedUpdate;
    SDisplayRectMap mDisplayRectMap[eVoutCnt];

private:
    AM_U64 mTimeOffset;
    AM_U64 mFirstPTS;
    am_pts_t mLastPTS;
    AM_UINT mAdjustInterval;
    AM_UINT msOldState;
    bool mbFlushed;

    bool mbStepMode;
    AM_UINT mStepCnt;
};


#endif

