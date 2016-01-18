
/*
 * audio_renderer.h
 *
 * History:
 *    2010/1/19 - [Yu Jiankang] create file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AUDIO_RENDERER_H__
#define __AUDIO_RENDERER_H__

class CAudioFixedBufferPool;
class CAudioRenderer;
class CAudioRendererInput;



//-----------------------------------------------------------------------
//
// CAudioFixedBufferPool
//
//-----------------------------------------------------------------------
class CAudioFixedBufferPool: public CBufferPool
{
	typedef CBufferPool inherited;
	friend class CAudioRendererInput;

public:
	static CAudioFixedBufferPool* Create(AM_UINT size, AM_UINT count);
	virtual void Delete();

protected:
	CAudioFixedBufferPool(): inherited("AudioPCMBuffer"), _pBuffers(NULL), _pMemory(NULL) {}
	AM_ERR Construct(AM_UINT size, AM_UINT count);
	virtual ~CAudioFixedBufferPool();

private:
	CAudioBuffer *_pBuffers;
	AM_U8 *_pMemory;
};


//-----------------------------------------------------------------------
//
// CAudioRenderer
//
//-----------------------------------------------------------------------
class CAudioRenderer: public CActiveFilter, public IClockObserver, IRender
{
    typedef CActiveFilter inherited;
    friend class CAudioRendererInput;

    enum AudioSinkState{
        AUDIO_SINK_CLOSED = 0,
        AUDIO_SINK_OPEN_GOOD,
        AUDIO_SINK_OPEN_FAIL,
    };

    enum {
        STATE_WAIT_RENDERER_READY = LAST_COMMON_STATE + 1,
        STATE_WAIT_PROPER_START_TIME,
    };

public:
	static IFilter* Create(IEngine *pEngine);
	static int AcceptMedia(CMediaFormat& format);

public:
    //IFilter
    virtual AM_ERR 		Start();
    virtual AM_ERR 		Run();
	virtual void 		GetInfo(INFO& info);
	virtual IPin* 		GetInputPin(AM_UINT index);
	//virtual IamPin* 	GetOutputPin(AM_UINT index);
#ifdef AM_DEBUG
    virtual void PrintState();
#endif

	//IActiveObject
	virtual void OnRun();

	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete();

	//IClockObserver
    virtual AM_ERR OnTimer(am_pts_t curr_pts);

	//IAudioRender
    virtual AM_ERR GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs);

	// inherited from CActiveFilter
	virtual bool ProcessCmd(CMD& cmd);

protected:
	AM_ERR Construct();

	CAudioRenderer(IEngine *pEngine);
	virtual ~CAudioRenderer();

private:
	void 	init();
	void 	deinit();
	void 	reset();
    bool 	requestRun();
    bool 	waitAVSyncCmd();
    void sendSyncInErrorCase();

	void 	renderBuffer( );
	void 	discardBuffer(CBuffer*& pBuffer);
    AM_ERR    openAudioSink(CAudioBuffer* pBuffer);
    AM_ERR switchVideoRendererAsMaster();

    void    handleEOS();
    void    onStateIdle();
    void    onStateHasInputData();
    void    onStatePending();
    void    onStateReady();
    void    onStateWaitRendererReady();
    void    onStateWaitProperTime();
    void    onStateError();

    void    speedUp();
private:
	CAudioRendererInput *mpAudioInputPin;
	CAudioFixedBufferPool 	*mpABP;

	IClockManager		*mpClockManager;
	CBuffer				*mpBuffer;

    IAudioHAL			*mpAudioOut;
    bool    mbAudioOutPaused;//"pause after flush, resume after begin playback" is not very perfect logic, feed latency should be not cause it overflow, overflow will cause some data discarded? todo
    //IFileWriter		*mpWriter;

	bool mbAudioStarted;//when renderer starts to work or resumes, it is true, and when render is stopped or is flushed, it is false;
	AM_UINT mAudioBufferSize;
	AM_INT mAudioSampleFormat;
	AM_UINT mAudioSamplerate;
	AM_UINT mAudioChannels;

	AM_U64 mAudioTotalSamples;//audio samples rendered after latest seek
	AM_U64 mAudioTotalSamplesTime;//audio samples rendered after latest seek
	AM_U64 mTimeOffset;//latest seek's position

	AM_U64 mFirstPTS;
	AM_U64 mLastPTS;
	AM_U32 mLatency;

    //for avsync parameters check, maybe ugly here, todo
    AM_U64 mEstimatedDurationForAVSync;
    bool mbGetEstimatedDuration;

	AudioSinkState mbAudioSinkOpened;

    bool mbRecievedSyncCmd;
    bool mbRecievedSourceFilterBlockedCmd;

    am_pts_t mCheckNoDataTime;

    AM_U8 mbNeedSpeedUp;
    AM_U8 mReserved1[3];
};

//-----------------------------------------------------------------------
//
// CAudioRendererInput
//
//-----------------------------------------------------------------------
class CAudioRendererInput: public CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CAudioRenderer;

public:
	static CAudioRendererInput* Create(CFilter *pFilter);

private:
	CAudioRendererInput(CFilter *pFilter):
		inherited(pFilter)
	{}
	AM_ERR Construct();
	virtual ~CAudioRendererInput();

public:
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
};


#endif
