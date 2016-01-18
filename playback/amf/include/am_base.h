
/**
 * am_base.h
 *
 * History:
 *    2009/12/7 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_BASE_H__
#define __AM_BASE_H__

class CMutex;
class CQueue;
class CMsgSys;

class CObject;
class CBufferPool;
class CSimpleBufferPool;
class CWorkQueue;

class CPin;
class CInputPin;
class COutputPin;
class CActiveOutputPin;
class CQueueInputPin;
class CActiveInputPin;
class CFilter;
class CActiveFilter;

class CFilterGraph;
class CEngineMsgProxy;
class CBaseEngine;

enum
{
    EOS_FLAG = 1,
    READY_FLAG = 4,
    SYNC_FLAG = 8,
    RECIEVE_UDEC_RUNNING_FLAG = 16,
};

//-----------------------------------------------------------------------
//
//  CObject
//
//-----------------------------------------------------------------------
class CObject: public IInterface
{
public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete();
	virtual ~CObject() {}

//log, for debug
#ifdef AM_DEBUG
public:
	CObject():mLogLevel(LogInfoLevel), mLogOption(LogBasicInfo | LogPerformance), mLogOutput(LogConsole | LogLogcat) {}
	void SetLogLevelOption(AM_UINT level, AM_UINT option, AM_UINT output) {mLogLevel = level; mLogOption = option; mLogOutput = output;}

protected:
	static am_char_t mcLogTag[LogAll+1][16];

	AM_UINT mLogLevel;
	AM_UINT mLogOption;
	//log output config, console or ddms
	AM_UINT mLogOutput;
#endif

};

//-----------------------------------------------------------------------
//
//  CBufferPool
//
//-----------------------------------------------------------------------
class CBufferPool: public CObject, public IBufferPool
{
	typedef CObject inherited;

protected:
	CBufferPool(const char *pName): mpBufferQ(NULL), mRefCount(1), mpName(pName), mpNotifyOwner(NULL) {}
	AM_ERR Construct(AM_UINT nMaxBuffers);
	virtual ~CBufferPool();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IBufferPool
	virtual const char *GetName() { return mpName; }

	virtual void Enable(bool bEnabled = true);
	virtual bool AllocBuffer(CBuffer*& pBuffer, AM_UINT size);
	virtual AM_UINT GetFreeBufferCnt() {return mpBufferQ->GetDataCnt();}
	virtual void SetNotifyOwner(void* pOwner) {mpNotifyOwner = pOwner;}

	virtual void AddRef(CBuffer *pBuffer);
	virtual void Release(CBuffer *pBuffer);

	virtual void AddRef();
	virtual void Release();

protected:
	virtual void OnReleaseBuffer(CBuffer *pBuffer) {}

protected:
	CQueue *mpBufferQ;
	am_atomic_t mRefCount;
	const char *mpName;
	void* mpNotifyOwner;
};

//-----------------------------------------------------------------------
//
//  CSimpleBufferPool
//
//-----------------------------------------------------------------------
class CSimpleBufferPool: public CBufferPool
{
	typedef CBufferPool inherited;

public:
	static CSimpleBufferPool* Create(const char *pName, AM_UINT count,
		AM_UINT objectSize = sizeof(CBuffer));

protected:
	CSimpleBufferPool(const char *pName):
		inherited(pName),
		mpBufferMemory(NULL) {}
	AM_ERR Construct(AM_UINT count, AM_UINT objectSize = sizeof(CBuffer));
	virtual ~CSimpleBufferPool();

private:
	AM_U8 *mpBufferMemory;
};

//-----------------------------------------------------------------------
//
// CFixedBufferPool
//
//-----------------------------------------------------------------------
class CFixedBufferPool: public CBufferPool
{
    typedef CBufferPool inherited;

public:
    static CFixedBufferPool* Create(AM_UINT size, AM_UINT count);
    virtual void Delete();

protected:
    CFixedBufferPool(): inherited("FixedBufferPool"), _pBuffers(NULL), _pMemory(NULL) {}
    AM_ERR Construct(AM_UINT size, AM_UINT count);
    virtual ~CFixedBufferPool();

private:
    CBuffer *_pBuffers;
    AM_U8 *_pMemory;
};


//-----------------------------------------------------------------------
//
//  CGeneralBufferPool
//
//-----------------------------------------------------------------------
/*
class CGeneralBufferPool: public CSimpleBufferPool
{
    typedef CSimpleBufferPool inherited;
public:
    static CGeneralBufferPool* Create(const char* name, AM_UINT count)
    {
        CGeneralBufferPool* result = new CGeneralBufferPool(name);
        if(result && result->Construct(count) != ME_OK)
        {
            delete result;
            result = NULL;
        }
        return result;
    }
protected:
    CGeneralBufferPool(const char* name):
        inherited(name)
        {}
    AM_ERR Construct(AM_UINT count)
    {
        //AM_UINT pSize = sizeof(AM_INTPTR);
        return inherited::Construct(count, sizeof(CGBuffer));
    }
    virtual ~CGeneralBufferPool() {}

//protected:
    //virtual void OnReleaseBuffer(CBuffer *pBuffer) {}
};
*/
//-----------------------------------------------------------------------
//
//  CWorkQueue
//
//-----------------------------------------------------------------------
class CWorkQueue
{
	typedef IActiveObject AO;

public:
	static CWorkQueue* Create(AO *pAO);
	void Delete();

private:
	CWorkQueue(AO *pAO):
		mpAO(pAO),
		mpMsgQ(NULL),
		mpThread(NULL)
	{}
	AM_ERR Construct();
	~CWorkQueue();

public:
         AM_ERR SetThreadPrio(AM_INT debug, AM_INT priority)
        {
            return mpThread->SetThreadPrio(debug, priority);
        }
	// return receive's reply
	AM_ERR SendCmd(AM_UINT cid, void *pExtra = NULL)
	{
		AO::CMD cmd(cid);
		cmd.pExtra = pExtra;
		return mpMsgQ->SendMsg(&cmd, sizeof(cmd));
	}

	void GetCmd(AO::CMD& cmd)
	{
		mpMsgQ->GetMsg(&cmd, sizeof(cmd));
	}

	bool PeekCmd(AO::CMD& cmd)
	{
		return mpMsgQ->PeekMsg(&cmd, sizeof(cmd));
	}

	AM_ERR PostMsg(AM_UINT cid, void *pExtra = NULL)
	{
		AO::CMD cmd(cid);
		cmd.pExtra = pExtra;
		return mpMsgQ->PostMsg(&cmd, sizeof(cmd));
	}

	AM_ERR Run() { return SendCmd(AO::CMD_RUN); }
	AM_ERR Stop() { return SendCmd(AO::CMD_STOP); }
	AM_ERR Start() { return SendCmd(AO::CMD_START); }

	void CmdAck(AM_ERR result) { mpMsgQ->Reply(result); }
	CQueue *MsgQ() { return mpMsgQ; }

	CQueue::QType WaitDataMsg(void *pMsg, AM_UINT msgSize, CQueue::WaitResult *pResult)
	{
		return mpMsgQ->WaitDataMsg(pMsg, msgSize, pResult);
	}

    CQueue::QType WaitDataMsgCircularly(void *pMsg, AM_UINT msgSize, CQueue::WaitResult *pResult)
    {
        return mpMsgQ->WaitDataMsgCircularly(pMsg, msgSize, pResult);
    }

    CQueue::QType WaitDataMsgWithSpecifiedQueue(void *pMsg, AM_UINT msgSize, const CQueue *pQueue)
    {
        return mpMsgQ->WaitDataMsgWithSpecifiedQueue(pMsg, msgSize, pQueue);
    }

	void WaitMsg(void *pMsg, AM_UINT msgSize)
	{
		mpMsgQ->WaitMsg(pMsg, msgSize);
	}

private:
	void MainLoop();
	static AM_ERR ThreadEntry(void *p);
	void Terminate();

private:
	AO *mpAO;
	CQueue *mpMsgQ;
	CThread *mpThread;
};


//-----------------------------------------------------------------------
//
//  CFilter
//
//-----------------------------------------------------------------------
class CFilter: public CObject, public IFilter
{
	typedef CObject inherited;

protected:
	CFilter(IEngine *pEngine):
		mpEngine(pEngine),
		mpName(NULL),
		mbIsMaster(false)
	{}
	AM_ERR Construct();
	virtual ~CFilter() {}

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

    // IFilter
    //virtual AM_ERR Run();
    //virtual AM_ERR Stop();
    virtual void Pause() {}
    virtual void Resume() {}
    virtual void Flush() {}

#ifdef AM_DEBUG
    virtual void PrintState() {};
#endif

	virtual bool IsMaster() { return mbIsMaster; }
	virtual void SetMaster( bool isMaster) { mbIsMaster = isMaster; }

    //virtual void GetInfo(INFO& info);
    virtual AM_ERR Start() { return ME_OK; }
    virtual void SendCmd(AM_UINT cmd) {}
    virtual void PostMsg(AM_UINT cmd) {}

    virtual AM_ERR FlowControl(FlowControlType type)
    {
        AMLOG_INFO("flow control comes, type %d.\n", type);
        return ME_OK;
    }

	virtual IPin* GetInputPin(AM_UINT index)
	{
		return NULL;
	}

	virtual IPin* GetOutputPin(AM_UINT index)
	{
		return NULL;
	}

    virtual AM_ERR AddOutputPin(AM_UINT& index, AM_UINT type)
    {
        AM_ERROR("!!!must not comes here, not implement.\n");
        return ME_NOT_SUPPORTED;
    }

    virtual AM_ERR AddInputPin(AM_UINT& index, AM_UINT type)
    {
        AM_ERROR("!!!must not comes here, not implement.\n");
        return ME_NOT_SUPPORTED;
    }

	virtual void OutputBufferNotify(IBufferPool* buffer_pool)
	{
		return;
	}

public:
	AM_ERR PostEngineMsg(AM_MSG& msg)
	{
		return mpEngine->PostEngineMsg(msg);
	}

	AM_ERR PostEngineMsg(AM_UINT code)
	{
		AM_MSG msg;
		msg.code = code;

		msg.p1  = (AM_INTPTR)static_cast<IFilter*>(this);

		return PostEngineMsg(msg);
	}

	AM_ERR PostEngineErrorMsg(AM_ERR err)
	{
		AM_MSG msg;
		msg.code = IEngine::MSG_ERROR;
		msg.p0 = err;
		msg.p1  = (AM_INTPTR)static_cast<IFilter*>(this);
		return PostEngineMsg(msg);
	}

        AM_ERR PostEngineSwitchMasterRenderMsg()//for bug1294
        {
                AM_MSG msg;
                msg.code = IEngine::MSG_SWITCH_MASTER_RENDER;
                return PostEngineMsg(msg);
        }

protected:
	IEngine *mpEngine;
	const char *mpName;
	bool mbIsMaster;
};


//-----------------------------------------------------------------------
//
//  CPin
//
//-----------------------------------------------------------------------
class CPin: public CObject, public IPin
{
	typedef CObject inherited;

protected:
	CPin(CFilter *pFilter):
		mpFilter(pFilter),
		mpPeer(NULL),
		mpBufferPool(NULL),
		mbSetBP(false)
	{}
	virtual ~CPin();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IPin
	//virtual AM_ERR Connect(IPin *pPeer) = 0;
	//virtual void Disconnect() = 0;

	//virtual void Receive(CBuffer *pBuffer) = 0;
	//virtual void Purge() = 0;
	virtual void Enable(bool bEnable)
	{
		if (mpBufferPool)
			mpBufferPool->Enable(bEnable);
	}

	virtual IPin* GetPeer() { return mpPeer; }
	virtual IFilter* GetFilter() { return mpFilter; }

protected:
	virtual AM_ERR OnConnect(IPin *pPeer) = 0;
	virtual void OnDisconnect();

protected:
	// When connecting, if there's no buffer pool,
	// this method is called.
	// If it returns an error (default), connecting fails.
	virtual AM_ERR NoBufferPoolHandler();

	// called by derived class to set a buffer pool for this pin
	// The buffer pool will be addref-ed.
	// If pBufferPool is NULL, previous buffer pool is released.
	void SetBufferPool(IBufferPool *pBufferPool);

	void ReleaseBufferPool();

protected:

	// Helpers

	AM_ERR PostEngineMsg(AM_MSG& msg)
	{
		return mpFilter->PostEngineMsg(msg);
	}

	AM_ERR PostEngineMsg(AM_UINT code)
	{
		AM_MSG msg;
		msg.code = code;
		msg.p1 = (AM_INTPTR)static_cast<IFilter*>(mpFilter);
		return PostEngineMsg(msg);
	}

	AM_ERR PostEngineErrorMsg(AM_ERR err)
	{
		AM_MSG msg;
		msg.code = IEngine::MSG_ERROR;
		msg.p0 = err;
		msg.p1 = (AM_INTPTR)mpFilter;
		return PostEngineMsg(msg);
	}

	const char *FilterName()
	{
		IFilter::INFO info;
		mpFilter->GetInfo(info);
		return info.pName;
	}

protected:
	CFilter *mpFilter;
	IPin *mpPeer;
	IBufferPool *mpBufferPool;
	bool mbSetBP;	// the buffer pool is specified by SetBufferPool()
};

//-----------------------------------------------------------------------
//
//  CInputPin
//
//-----------------------------------------------------------------------
class CInputPin: public CPin
{
	typedef CPin inherited;

protected:
	CInputPin(CFilter *pFilter): inherited(pFilter) {}
	AM_ERR Construct() { return ME_OK; }
	virtual ~CInputPin() {AMLOG_DESTRUCTOR("~CInputPin.\n");}

public:
	// IPin
	virtual AM_ERR Connect(IPin *pPeer);
	virtual void Disconnect();
	//virtual void Receive(CBuffer *pBuffer) = 0;
	virtual void Purge() {}

	virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
	{
		AM_ASSERT(0);
		return ME_NOT_EXIST;
	}

protected:
	virtual AM_ERR OnConnect(IPin *pPeer);
	//virtual void OnDisconnect();

	// Called when connecting to check upstream media format
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat) = 0;
};

//-----------------------------------------------------------------------
//
//  COutputPin
//
//-----------------------------------------------------------------------
class COutputPin: public CPin
{
	typedef CPin inherited;

protected:
	COutputPin(CFilter *pFilter): inherited(pFilter) {}
	virtual ~COutputPin() {}

public:
	// IPin
	virtual AM_ERR Connect(IPin *pPeer);
	virtual void Disconnect();
	virtual void Receive(CBuffer *pBuffer) { AM_ASSERT(0); }
	virtual void Purge() { AM_ASSERT(0); }
	//virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat) = 0;

public:
	bool AllocBuffer(CBuffer*& pBuffer, AM_UINT size = 0)
	{
		AM_ASSERT(mpBufferPool);
		return mpBufferPool->AllocBuffer(pBuffer, size);
	}

	void SendBuffer(CBuffer *pBuffer)
	{
		if (mpPeer)
			mpPeer->Receive(pBuffer);
		else {
			//AM_INFO("buffer dropped\n");
			pBuffer->Release();
		}
	}


protected:
	virtual AM_ERR OnConnect(IPin *pPeer);
	//virtual void OnDisconnect();
};

//-----------------------------------------------------------------------
//
//  CActiveOutputPin
//
//-----------------------------------------------------------------------
class CActiveOutputPin: public COutputPin, public IActiveObject
{
	typedef COutputPin inherited;

protected:
	typedef IActiveObject AO;

protected:
	CActiveOutputPin(CFilter *pFilter, const char *pName):
		inherited(pFilter),
		mpWorkQ(NULL),
		mpName(pName)
	{}
	AM_ERR Construct();
	virtual ~CActiveOutputPin();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IPin
	//virtual void GetInfo(INFO& info) = 0;
	//virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat) = 0;

	// IActiveObject
	virtual const char *GetName() { return mpName; }

public:
	virtual AM_ERR Run() { return mpWorkQ->Run(); }
	virtual AM_ERR Stop() { return mpWorkQ->Stop(); }

protected:
	// called when stopped, and received CMD_RUN
	//virtual void OnRun() = 0;

	// called by the thread when a cmd is received
	// should return false for CMD_TERMINATE
	virtual void OnCmd(CMD& cmd) {}

protected:
	CQueue *MsgQ() { return mpWorkQ->MsgQ(); }
	void GetCmd(AO::CMD& cmd) { mpWorkQ->GetCmd(cmd); }
	bool PeekCmd(AO::CMD& cmd) { return mpWorkQ->PeekCmd(cmd); }
	void CmdAck(AM_ERR err) { mpWorkQ->CmdAck(err); }

protected:
	CWorkQueue *mpWorkQ;
	const char *mpName;
};

//-----------------------------------------------------------------------
//
//  CQueueInputPin
//
//-----------------------------------------------------------------------
class CQueueInputPin: public CInputPin
{
	typedef CInputPin inherited;

protected:
	CQueueInputPin(CFilter *pFilter):
		inherited(pFilter),
		mpBufferQ(NULL)
	{}
	AM_ERR Construct(CQueue *pMsgQ);
	virtual ~CQueueInputPin();

public:
	// IPin
	virtual void Receive(CBuffer *pBuffer)
	{
		AM_ERR err = mpBufferQ->PutData(&pBuffer, sizeof(pBuffer));
		AM_ASSERT_OK(err);
	}
	virtual void Purge();

public:
/*
	bool ReceiveBuffer(CBuffer*& pBuffer, CQueue *pMsgQ, void *pCmd, AM_UINT cmdSize)
	{
		return mpBufferQ->GetMsg2((void*)&pBuffer, sizeof(pBuffer), pMsgQ, pCmd, cmdSize);
	}
*/
//	bool ReceiveBuffer(CBuffer*& pBuffer, void *pCmd, AM_UINT cmdSize);

	bool PeekBuffer(CBuffer*& pBuffer)
	{
		return mpBufferQ->PeekData((void*)&pBuffer, sizeof(pBuffer));
	}

protected:
	CQueue *mpBufferQ;
};

//-----------------------------------------------------------------------
//
//  CGeneralInputPin
//it is no clear for cqueueinputpin, maybe just aford a machine which can be waited by processmsgand data
//-----------------------------------------------------------------------
/*
class CGeneralInputPin: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
public:
    //just want this pin be created by CInterActiveFilter
    static CGeneralInputPin* Create(CFilter* pFilter);

private:
    CGeneralInputPin(CFilter* pFilter):
        inherited(pFilter)
    { }
    AM_ERR Construct();
    virtual ~CGeneralInputPin();

public:
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
    AM_UINT GetDataCnt() { return mpBufferQ->GetDataCnt();}
};

//-----------------------------------------------------------------------
//
//  CGeneralOutputPin
//
//-----------------------------------------------------------------------
class CGeneralOutputPin: public COutputPin
{
    typedef COutputPin inherited;
    friend class CGeneralDecoder;
    friend class CGeneralDemuxer;

public:
    static CGeneralOutputPin* Create(CFilter *pFilter);

protected:
    CGeneralOutputPin(CFilter *pFilter):
        inherited(pFilter)
     {}
    AM_ERR Construct();
    virtual ~CGeneralOutputPin();

public:
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat);
private:
    CMediaFormat mMediaFormat;
};
*/
//-----------------------------------------------------------------------
//
//  CActiveInputPin
//
//-----------------------------------------------------------------------
class CActiveInputPin: public CQueueInputPin, public IActiveObject
{
	typedef CQueueInputPin inherited;

protected:
	typedef IActiveObject AO;

protected:
	CActiveInputPin(CFilter *pFilter, const char *pName):
		inherited(pFilter),
		mpName(pName)
	{}
	AM_ERR Construct();
	virtual ~CActiveInputPin();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IActiveObject
	virtual const char *GetName() { return mpName; }

	// called when stopped, and received CMD_RUN
	virtual void OnRun();

	// called by the thread when a cmd is received
	// should return false for CMD_TERMINATE
	// It is called in STOP state
	virtual void OnCmd(CMD& cmd) {}

protected:
	// should release the buffer
	// if return an error, the pin will enter stopped,
	//	and the error is posted to engine
	// return ME_CLOSED means EOS
	virtual AM_ERR ProcessBuffer(CBuffer *pBuffer) = 0;

public:
	AM_ERR Run() { return mpWorkQ->Run(); }
	AM_ERR Stop() { return mpWorkQ->Stop(); }
	AM_ERR Start() { return mpWorkQ->Start(); }

protected:
	CQueue *MsgQ() { return mpWorkQ->MsgQ(); }
	void GetCmd(AO::CMD& cmd) { mpWorkQ->GetCmd(cmd); }
	bool PeekCmd(AO::CMD& cmd) { return mpWorkQ->PeekCmd(cmd); }
	void CmdAck(AM_ERR err) { mpWorkQ->CmdAck(err); }

protected:
	CWorkQueue *mpWorkQ;
	const char *mpName;
};
//-----------------------------------------------------------------------
//
//  CActiveObject
//
//-----------------------------------------------------------------------
class CActiveObject: public CObject, public IActiveObject
{
    typedef CObject inherited;
public:
    enum {
        STATE_IDLE = 0,
        STATE_HAS_INPUTDATA,
        STATE_WAIT_OUTPUT,
        STATE_READY,
        STATE_PENDING,
        STATE_ERROR,
        STATE_WAIT_EOS,
        STATE_EOS,
        LAST_COMMON_STATE,//this must be last element
    };

protected:
    typedef IActiveObject AO;
    CActiveObject(const char* pName):
        inherited(),
        mpName(pName),
        mpWorkQ(NULL),
        mbRun(true),
        mbPaused(false)
        { }

    AM_ERR Construct();
    virtual ~CActiveObject();
public:
    virtual void* GetInterface(AM_REFIID refiid);
    virtual void Delete() { inherited::Delete(); }

public:
    virtual AM_ERR SendCmd(AM_UINT cmd) { return mpWorkQ->SendCmd(cmd);}
    virtual AM_ERR PostCmd(AM_UINT cmd) {return mpWorkQ->PostMsg(cmd);}
    //PostCmd
    virtual void OnRun() {};
    virtual const char* GetName() { return mpName; }
    virtual void OnCmd(CMD& cmd) {}

protected:
    CQueue* MsgQ() { return mpWorkQ->MsgQ(); }
    void GetCmd(CMD& cmd) { mpWorkQ->GetCmd(cmd); }
    bool PeekCmd(CMD& cmd) { return mpWorkQ->PeekCmd(cmd); }
    void CmdAck(AM_ERR err) { mpWorkQ->CmdAck(err); }

protected:
    const char* mpName;
    CWorkQueue* mpWorkQ;
    bool mbRun;
    bool mbPaused;
    AM_UINT mState;
};
//-----------------------------------------------------------------------
//
//  CActiveQueue
//
//-----------------------------------------------------------------------
class CActiveQueue: public CActiveObject
{
    typedef CActiveObject inherited;
protected:
    CActiveQueue(AM_UINT num = 16, const char* name = NULL):
        inherited(name),
        mpBufferQ(NULL),
        mTotal(num),
        mCount(0)
    { }

    AM_ERR Construct();
    //wrap!
    AM_ERR PushBuffer(CBuffer* pBuffer);
    AM_ERR PopBuffer(CBuffer* pBuffer);
    AM_UINT QueueSize() { return mpBufferQ->GetDataCnt(); }
    AM_UINT QueueSpace();
    bool IsQueueFull();
//class CBuffferQueue;
private:
    //well, it may be strange for using postmsg and getmsg for data.
    CQueue* mpBufferQ;
    AM_UINT mTotal;
    AM_UINT mCount;
};
//-----------------------------------------------------------------------
//
//  CActiveFilter
//
//-----------------------------------------------------------------------
class CActiveFilter: public CFilter, public IActiveObject
{
	typedef CFilter inherited;

protected:
	typedef IActiveObject AO;

public:

	enum {
		CMD_OBUFNOTIFY = AO::CMD_LAST,
		CMD_TIMERNOTIFY,
	};

	enum {
		STATE_IDLE = 0,
		STATE_HAS_INPUTDATA,
		STATE_HAS_OUTPUTBUFFER,
		STATE_WAIT_OUTPUT,
		STATE_READY,
		STATE_PENDING,
		STATE_PROCCESS_EOS,
		STATE_ERROR,

        LAST_COMMON_STATE,//this must be last element
	};

	// abs(diff)>NOTSYNC_THRESHOLD: master renderer will sync clock manager
	// diff < (- WAIT_THRESHOLD): slave renderer will discard buffer
	// diff > (WAIT_THRESHOLD): slave renderer will wait proper time for rendering
	// current NOTSYNC_THRESHOLD 100ms, WAIT_THRESHOLD a frame tick, you can change it to more proper value
	enum {NOTSYNC_THRESHOLD = 9000, WAIT_THRESHOLD = 3003};

protected:
	CActiveFilter(IEngine *pEngine, const char *pName):
		inherited(pEngine),
		mpName(pName),
		mpWorkQ(NULL),
		mbRun(true),
		mbPaused(false),
		mbStreamStart(false),
		mNotSyncThreshold(NOTSYNC_THRESHOLD),
		mWaitThreshold(WAIT_THRESHOLD),
		msState(STATE_IDLE)
	{
//		AM_ASSERT(pEngine->mpOpaque);
		mpSharedRes = (SConsistentConfig*)pEngine->mpOpaque;
	}
	AM_ERR Construct();
	virtual ~CActiveFilter();

public:
	// IInterface
	virtual AM_ERR SetThreadPrio(AM_INT debug, AM_INT prio)
	{   return mpWorkQ->SetThreadPrio(debug, prio); }
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IFilter
	virtual AM_ERR Run() { return mpWorkQ->Run(); }
	virtual AM_ERR Stop() { return mpWorkQ->Stop(); }
	virtual void Pause() {mpWorkQ->PostMsg(CMD_PAUSE);}
	virtual void Resume() {mpWorkQ->PostMsg(CMD_RESUME);}
	virtual void Flush() {mpWorkQ->SendCmd(CMD_FLUSH);}

    virtual void SendCmd(AM_UINT cmd) {mpWorkQ->SendCmd(cmd);}
    virtual void PostMsg(AM_UINT cmd) {mpWorkQ->PostMsg(cmd);}

	virtual void GetInfo(INFO& info);
	//virtual IPin* GetInputPin(AM_UINT index);
	//virtual IPin* GetOutputPin(AM_UINT index);

	// IActiveObject
	virtual const char *GetName() { return mpName; }

	// called when stopped, and received CMD_RUN
	//virtual void OnRun() = 0;

	// called by the thread when a cmd is received
	// should return false for CMD_TERMINATE
	// It is called in STOP state
	virtual void OnCmd(CMD& cmd) {}
	virtual void OutputBufferNotify(IBufferPool* pool) {mpWorkQ->PostMsg(CMD_OBUFNOTIFY, (void*)pool);}
         CQueue *MsgQ() { return mpWorkQ->MsgQ(); }

protected:
	bool WaitInputBuffer(CQueueInputPin*& pPin, CBuffer*& pBuffer);
	virtual bool ProcessCmd(CMD& cmd) { return false; }
	virtual void OnStop() {}


protected:
	void GetCmd(AO::CMD& cmd) { mpWorkQ->GetCmd(cmd); }
	bool PeekCmd(AO::CMD& cmd) { return mpWorkQ->PeekCmd(cmd); }
	void CmdAck(AM_ERR err) { mpWorkQ->CmdAck(err); }

protected:
	const char *mpName;
	CWorkQueue *mpWorkQ;
	bool mbRun;
	bool mbPaused;
	bool mbStreamStart;//flag get first data buffer
	AM_S64 mNotSyncThreshold, mWaitThreshold;
	AM_UINT msState;

public:
    SConsistentConfig* mpSharedRes;
};

//-----------------------------------------------------------------------
//
//  CInterActiveFilter
//
//-----------------------------------------------------------------------
class CInterActiveFilter: public CActiveFilter, public IMsgSink
{
    typedef CActiveFilter inherited;
public:
    enum {
        MSG_READY = IEngine::MSG_LAST, // num 19
        MSG_DEMUXER_A,
        MSG_DECODED,
        MSG_UDEC_ERROR,
        MSG_DECODER_ERROR,
        MSG_DSP_GOTO_SHOW_PIC,
        MSG_DSP_GET_OUTPIC,//add for VE mode to get outpic

        MSG_SYNC_RENDER_V,
        MSG_SYNC_RENDER_A,
        MSG_RENDER_ADD_AUDIO,
        MSG_RENDER_END,
        MSG_SYNC_RENDER_DONE,

        MSG_SWITCH_TEST,
        MSG_SWITCHBACK_TEST,
        MSG_TEST,
    };
    enum{
        CMD_REMOVE = CMD_LAST + 100,
    };

    CInterActiveFilter(IEngine *pEngine, const char *pName):
        inherited(pEngine, pName),
        mpFilterPort(NULL),
        mpMsgsys(NULL)
    {}
    virtual ~CInterActiveFilter();
    AM_ERR Construct();
    virtual void* GetInterface(AM_REFIID refiid);

    //virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete() { inherited::Delete(); }
    virtual void MsgProc(AM_MSG& msg);
    virtual AM_ERR Remove(AM_INT index, AM_U8 flag = 0);
    virtual AM_ERR Config(AM_INT target, AM_U8 flag = 0);
    virtual AM_ERR Resume(AM_INT target, AM_U8 flag = 0);
    virtual AM_ERR Pause(AM_INT target, AM_U8 flag = 0);
    virtual AM_ERR Flush(AM_INT target, AM_U8 flag = 0);
    void PostMsgToFilter(AM_MSG& msg);
    void SendMsgToFilter(AM_MSG& msg);
    virtual AM_ERR SetInputFormat(CMediaFormat* pFormat) { return ME_OK;}
    virtual AM_ERR SetOutputFormat(CMediaFormat* pFormat) { return ME_OK;}
    virtual AM_ERR ReleaseBuffer(CBuffer* buffer) { return ME_OK;}
    virtual AM_ERR RetrieveBuffer(CBuffer* buffer) { return ME_OK;}

protected:
    IMsgPort* mpFilterPort;
    CMsgSys * mpMsgsys;
    AM_UINT msOldState;
};
//-----------------------------------------------------------------------
//
//  CFilterGraph
//
//-----------------------------------------------------------------------
class CFilterGraph: public CObject
{
	typedef CObject inherited;

protected:
	CFilterGraph():
		mnFilters(0),
		mnConnections(0)
	{}
	AM_ERR Construct() { return ME_OK; }
	virtual ~CFilterGraph();

protected:

	struct Filter {
		IFilter *pFilter;
		AM_U32 flags;
	};

	struct Connection {
		IPin *pOutputPin;
		IPin *pInputPin;
	};

protected:
	void ClearGraph();

	AM_ERR RunAllFilters();
	void DisableAllOutput();
	void StopAllFilters();
	void PurgeAllFilters();
	void FlushAllFilters();
	void DeleteAllConnections();
	void RemoveAllFilters();

	void EnableOutputPins(IFilter *pFilter, bool bEnable = true);
	void DisableOutputPins(IFilter *pFilter) { EnableOutputPins(pFilter, false); }

	AM_ERR AddFilter(IFilter *pFilter);
	AM_ERR FindFilter(IFilter *pFilter, AM_UINT& index);
         IFilter* FindFilterbyIndex(AM_INT& index);
	AM_ERR Connect(IFilter *pUpstreamFilter, AM_UINT indexUp,
		IFilter *pDownstreamFilter, AM_UINT indexDown);
	AM_ERR CreateConnection(IPin *pOutputPin, IPin *pInputPin);

	const char *GetFilterName(IFilter *pFilter);
	bool AllRenderersReady();
	bool AllRenderersEOS();
	void StartAllRenderers();
	//void ClearRenderersReady();
	void ClearReadyEOS();

    bool AllFiltersHasFlag(AM_UINT flag);
    void AllFiltersClearFlag(AM_UINT flag);
    void AllFiltersCmd(AM_UINT cmd);
    void AllFiltersMsg(AM_UINT msg);

protected:
	void PurgeFilter(IFilter *pFilter);

protected:

	bool HasEOS(AM_UINT index) { return (mFilters[index].flags & EOS_FLAG) != 0; }
	void SetEOS(AM_UINT index) { mFilters[index].flags |= EOS_FLAG; }
	void ClearEOS(AM_UINT index) { mFilters[index].flags &= ~EOS_FLAG; }

	bool HasReady(AM_UINT index) { return (mFilters[index].flags & READY_FLAG) != 0; }
	void SetReady(AM_UINT index) { mFilters[index].flags |= READY_FLAG; }

	void ClearReady(AM_UINT index) { mFilters[index].flags &= ~READY_FLAG; }


protected:
	AM_UINT mnFilters;
	Filter mFilters[16];

	AM_UINT mnConnections;
	Connection mConnections[32];
};

//-----------------------------------------------------------------------
//
//  CBaseEngine
//
//-----------------------------------------------------------------------

class CBaseEngine;

class CEngineMsgProxy: public CObject, public IMsgSink
{
	typedef CObject inherited;

public:
	static CEngineMsgProxy* Create(CBaseEngine *pEngine,CMsgSys *pMsgSys);

protected:
	CEngineMsgProxy(CBaseEngine *pEngine,CMsgSys *pMsgSys):
		mpEngine(pEngine),
		mpMsgSys(pMsgSys),
		mpMsgPort(NULL)
	{}
	AM_ERR Construct();
	virtual ~CEngineMsgProxy();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IMsgSink
	virtual void MsgProc(AM_MSG& msg);

	IMsgPort *MsgPort() { return mpMsgPort; }

private:
    CBaseEngine *mpEngine;
    CMsgSys *mpMsgSys;
    IMsgPort *mpMsgPort;
};

class CBaseEngine: public CFilterGraph,
	virtual public IEngine, public IMsgSink, public IMediaControl
{
	typedef CFilterGraph inherited;
	friend class CEngineMsgProxy;

protected:
	CBaseEngine():
		mSessionID(0),
		mpMutex(NULL),
		mpPortFromFilters(NULL),
		mpMsgProxy(NULL),
		mpAppMsgSink(NULL),
		mAppMsgProc(NULL),
		mpMsgsys(NULL)
	{
		mpOpaque = NULL;
	}
	AM_ERR Construct();
	virtual ~CBaseEngine();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IEngine
	virtual AM_ERR PostEngineMsg(AM_MSG& msg);
         //virtual CMsgSys* GetMsgSys() { return NULL;}
	// IMsgSink
	//virtual void MsgProc(AM_MSG& msg);

	// IMediaControl
	virtual AM_ERR SetAppMsgSink(IMsgSink *pAppMsgSink);
	virtual AM_ERR SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&), void *context);
         virtual CMsgSys* GetMsgSys(){ return mpMsgsys;}

protected:
	AM_ERR PostAppMsg(AM_MSG& msg);
	AM_ERR PostAppMsg(AM_UINT code)
	{
		AM_MSG msg;
		msg.code = code;
		msg.sessionID = mSessionID;
		return PostAppMsg(msg);
	}

	void NewSession() { mSessionID++; }
	bool IsSessionMsg(AM_MSG& msg) { return msg.sessionID == mSessionID; }
private:
	void OnAppMsg(AM_MSG& msg);

protected:
	AM_UINT mSessionID;
	CMutex *mpMutex;
	IMsgPort *mpPortFromFilters;
	CEngineMsgProxy *mpMsgProxy;
	IMsgSink *mpAppMsgSink;
	void (*mAppMsgProc)(void*, AM_MSG&);
	void *mAppMsgContext;
	CMsgSys * mpMsgsys;
};

//-----------------------------------------------------------------------
//
//  CActiveEngine
//
//-----------------------------------------------------------------------
#if 0
class CActiveEngine;

class CEngineMsgProxy: public CObject, public IMsgSink
{
	typedef CObject inherited;

public:
	static CEngineMsgProxy* Create(CActiveEngine *pEngine,CMsgSys *pMsgSys);

protected:
	CEngineMsgProxy(CActiveEngine *pEngine,CMsgSys *pMsgSys):
		mpEngine(pEngine),
		mpMsgSys(pMsgSys),
		mpMsgPort(NULL)
	{}
	AM_ERR Construct();
	virtual ~CEngineMsgProxy();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IMsgSink
	virtual void MsgProc(AM_MSG& msg);

	IMsgPort *MsgPort() { return mpMsgPort; }

private:
	CActiveEngine *mpEngine;
	IMsgPort *mpMsgPort;
	CMsgSys *mpMsgSys;
};
#endif

class CActiveEngine: public CFilterGraph, public IActiveObject,
    virtual public IEngine, public IMediaControl
{
    typedef CFilterGraph inherited;

protected:
    enum {
        CMD_TYPE_SINGLETON = 0,//need excute one by one
        CMD_TYPE_REPEAT_LAST,//next will replace previous
        CMD_TYPE_REPEAT_CNT,//cmd count
        CMD_TYPE_REPEAT_AVATOR,//need translate cmd
    };

protected:
    CActiveEngine(const char* pname):
        mSessionID(0),
        mpMutex(NULL),
        mpAppMsgSink(NULL),
        mAppMsgProc(NULL),
        mpWorkQ(NULL),
        mpCmdQueue(NULL),
        mpFilterMsgQ(NULL),
        mpName(pname)
    {
        mpOpaque = NULL;
    }
    AM_ERR Construct();
    virtual ~CActiveEngine();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete() { inherited::Delete(); }

    // IEngine
    virtual AM_ERR PostEngineMsg(AM_MSG& msg);

    // IMediaControl
    virtual AM_ERR SetAppMsgSink(IMsgSink *pAppMsgSink);
    virtual AM_ERR SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&), void *context);
    virtual CMsgSys* GetMsgSys(){ return NULL;}

    //IAO
    virtual const char *GetName() {return mpName;}
    virtual void OnRun() {AM_ASSERT(0); AMLOG_ERROR("should not come here.\n");}
    virtual void OnCmd(CMD& cmd) {}

protected:
    AM_ERR PostAppMsg(AM_MSG& msg);
    AM_ERR PostAppMsg(AM_UINT code)
    {
        AM_MSG msg;
        msg.code = code;
        msg.sessionID = mSessionID;
        return PostAppMsg(msg);
    }

    void NewSession() { AUTO_LOCK(mpMutex); mSessionID++; }
    bool IsSessionMsg(AM_MSG& msg) { return msg.sessionID == mSessionID; }

protected:
    AM_UINT mSessionID;
    CMutex *mpMutex;
    IMsgSink *mpAppMsgSink;
    void (*mAppMsgProc)(void*, AM_MSG&);
    void *mAppMsgContext;
    CWorkQueue * mpWorkQ;//mainq, cmd from user/app
    CQueue *mpCmdQueue;//msg from user/app
    CQueue *mpFilterMsgQ;//msg from filters
    const char* mpName;
    //CMsgSys * mpMsgsys;
};

//-----------------------------------------------------------------------
//
// CSystemClockSource
//
//-----------------------------------------------------------------------
class CSystemClockSource: public CObject, public IClockSource
{
	typedef CObject inherited;

public:
	static CSystemClockSource* Create();

public:
	CSystemClockSource() {}
	AM_ERR Construct() { mUnitNum = 1; mUnitDen = 1000; mScaleSec = 1000; mScaleUSec = 1000; return ME_OK; }
	virtual ~CSystemClockSource() {}

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IClockSource
    virtual  am_pts_t GetClockTime();
    virtual  am_pts_t GetClockBase();
    virtual void GetClockUnit(AM_UINT& num, AM_UINT& den) {num = mUnitNum; den = mUnitDen;}
    virtual void SetClockUnit(AM_UINT num, AM_UINT den)
    {
        mUnitNum = num;
        mUnitDen = den;
        mScaleSec = mUnitDen/mUnitNum;
        mScaleUSec = 1000000*mUnitNum/mUnitDen;
        AMLOG_INFO(" SetClockUint num%d, den%d, mScaleSec %d, mScaleUSec %d.\n", num, den, mScaleSec, mScaleUSec);
    }
    virtual void SetClockState(CLOCK_STATE state);
protected:
    AM_UINT mUnitNum;
    AM_UINT mUnitDen;

private:
    AM_UINT mScaleSec;
    AM_UINT mScaleUSec;
};

//-----------------------------------------------------------------------
//
// CClockManager
//
//-----------------------------------------------------------------------
class CClockManager: public CObject, public IClockManager, public IActiveObject
{
	typedef IActiveObject AO;
	typedef CObject inherited;

public:
	static CClockManager* Create();

protected:
	enum ClockState{

	};

protected:
	CClockManager():
		mpMutex(NULL),
		mpTimerEvent(NULL),
		mpSource(NULL),
		mpSystemClockSource(NULL),
		mStartTime(0),
		mPausedTime(0),
		mStartPts(0),
		mClockRate(1.0),
		mpWorkQ(NULL),
		mbPaused(true)
	{
		InitTimers();
	}
	AM_ERR Construct();
	virtual ~CClockManager();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IClockManager
	virtual AM_ERR SetTimer(IClockObserver *pObserver, am_pts_t pts);
	virtual AM_ERR DeleteTimer(IClockObserver *pObserver);
	virtual am_pts_t GetCurrentTime();
	virtual AM_ERR StartClock();
	virtual void StopClock();
	virtual void PauseClock();
	virtual void ResumeClock();
	virtual void SetSource(IClockSource *pSource);
	virtual void SetStartTime(am_pts_t time);
	virtual am_pts_t GetStartTime();
	virtual void SetStartPts(am_pts_t time);
	virtual am_pts_t GetStartPts();
	virtual void SetClockMgr(am_pts_t startPts);
	virtual void PurgeClock();

	// IActiveObject
	virtual const char *GetName() { return "ClockManager"; }
	virtual void OnRun();
	virtual void OnCmd(CMD& cmd) {}

protected:
	virtual bool ProcessCMD(CMD& cmd);


private:
	CMutex *mpMutex;
	CEvent *mpTimerEvent;
	IClockSource *mpSource;
	IClockSource *mpSystemClockSource;
	am_pts_t mStartTime;
	am_pts_t mPausedTime;
	am_pts_t mStartPts;
	double mClockRate;
	CWorkQueue *mpWorkQ;
	bool mbPaused;

private:
	struct TimerNode
	{
		IClockObserver *pObserver;
		am_pts_t pts;
	};

	enum {MAX_TIMERS = 8};
	TimerNode mTimerNodes[MAX_TIMERS];

	void InitTimers()
	{
		for (int i = 0; i < MAX_TIMERS; i++)
			mTimerNodes[i].pObserver = NULL;
	}

	void CheckTimers();
};
//-----------------------------------------------------------------------
//
// CFileReader
//
//-----------------------------------------------------------------------
class CFileReader: public CObject, public IFileReader
{
	typedef CObject inherited;

public:
	static CFileReader *Create();

private:
	CFileReader(): mpMutex(NULL), mpFile(NULL) {}
	AM_ERR Construct();
	virtual ~CFileReader();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { return inherited::Delete(); }

	// IFileReader
	virtual AM_ERR OpenFile(const char *pFileName);
	virtual AM_ERR CloseFile();
	virtual am_file_off_t GetFileSize();
	virtual AM_ERR ReadFile(am_file_off_t offset, void *pBuffer, AM_UINT size);

private:
	void __CloseFile();

private:
	CMutex *mpMutex;
	void *mpFile;
};

extern void AM_Util_Init();

#endif

