
/**
 * am_base.cpp
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

#include <sys/time.h>
#include "string.h"
#if PLATFORM_ANDROID
#include "sys/atomics.h"
#endif


//#define LOG_NDEBUG 0
#define LOG_TAG "am_base"
//#define AMDROID_DEBUG

#include "am_new.h"
#include "am_types.h"
#include "am_if.h"
#include "am_mw.h"
#include "osal.h"
#include "am_queue.h"
#include "msgsys.h"

#include "am_base.h"

#ifdef AM_DEBUG
am_char_t CObject::mcLogTag[LogAll+1][16]= {"   ", "Error: ", "Warning: ", "Info: ", "Debug: ", "Verbose: ", "    "};
#endif

//-----------------------------------------------------------------------
//
//  CObject
//
//-----------------------------------------------------------------------
void *CObject::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IInterface)
		return (IInterface*)this;
	return NULL;
}

void CObject::Delete()
{
	delete this;
}

CSubtitleBuffer::CSubtitleBuffer()
{
    subType = SUBTYPE_NON;
    start_time = 0;
    end_time = 0;
    memset((void*)style, 0, sizeof(style));
    memset((void*)fontname, 0, sizeof(fontname));
    fontsize = 0;
    PrimaryColour = 0;
    BackColour = 0;
    Bold = 0;
    Italic = 0;
}

//-----------------------------------------------------------------------
//
//  CBufferPool
//
//-----------------------------------------------------------------------
AM_ERR CBufferPool::Construct(AM_UINT nMaxBuffers)
{
	if ((mpBufferQ = CQueue::Create(NULL, this, sizeof(CBuffer*), nMaxBuffers)) == NULL)
		return ME_NO_MEMORY;

	AM_PRINTF("buffer pool '%s' created\n", mpName);
	return ME_OK;
}

CBufferPool::~CBufferPool()
{
	AM_PRINTF("buffer pool '%s' destroyed\n", mpName);
	AM_DELETE(mpBufferQ);
}

void *CBufferPool::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IBufferPool)
		return (IBufferPool*)this;
	return inherited::GetInterface(refiid);
}

void CBufferPool::Enable(bool bEnabled)
{
	mpBufferQ->Enable(bEnabled);
}

bool CBufferPool::AllocBuffer(CBuffer*& pBuffer, AM_UINT size)
{
	if (mpBufferQ->GetMsgEx(&pBuffer, sizeof(pBuffer))) {
		pBuffer->mRefCount = 1;
		pBuffer->mpPool = this;
		pBuffer->mpNext = NULL;
		return true;
	}
	return false;
}

void CBufferPool::AddRef(CBuffer *pBuffer)
{
	__atomic_inc(&pBuffer->mRefCount);
}

void CBufferPool::Release(CBuffer *pBuffer)
{
	//printf("--mRefCount:%d\n",pBuffer->mRefCount);
	if (__atomic_dec(&pBuffer->mRefCount) == 1) {
		//printf("--in Release\n");
		// from 1 to 0
		CBuffer *pBuffer2 = pBuffer->mpNext;

		OnReleaseBuffer(pBuffer);
		AM_ERR err = mpBufferQ->PostMsg((void*)&pBuffer, sizeof(CBuffer*));
		AM_ASSERT_OK(err);

		if (pBuffer2) {
			OnReleaseBuffer(pBuffer2);
			AM_ERR err = mpBufferQ->PostMsg((void*)&pBuffer2, sizeof(CBuffer*));
			AM_ASSERT_OK(err);
		}
		CFilter* pFilter = (CFilter*)mpNotifyOwner;
		if(mpNotifyOwner)
			pFilter->OutputBufferNotify((IBufferPool*)this);

	}
	//printf("--after Release\n");
}

void CBufferPool::AddRef()
{
	__atomic_inc(&mRefCount);
}

void CBufferPool::Release()
{
	if (__atomic_dec(&mRefCount) == 1) {
		inherited::Delete();
	}
}

//-----------------------------------------------------------------------
//
//  CSimpleBufferPool
//
//-----------------------------------------------------------------------
CSimpleBufferPool* CSimpleBufferPool::Create(const char *pName, AM_UINT count, AM_UINT objectSize)
{
	CSimpleBufferPool* result = new CSimpleBufferPool(pName);
	if (result && result->Construct(count, objectSize) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CSimpleBufferPool::Construct(AM_UINT count, AM_UINT objectSize)
{
	AM_ERR err;

	if ((err = inherited::Construct(count)) != ME_OK)
		return err;

	AM_ASSERT(objectSize >= sizeof(CBuffer));
	objectSize = ROUND_UP(objectSize, 4);

	mpBufferMemory = new AM_U8[objectSize * count];
	if (mpBufferMemory == NULL)
		return ME_NO_MEMORY;

	CBuffer *pBuffer = (CBuffer*)mpBufferMemory;
	for (AM_UINT i = 0; i < count; i++) {
		pBuffer->mpData = ((AM_U8*)pBuffer) + sizeof(CBuffer);
		pBuffer->mBlockSize = objectSize - sizeof(CBuffer);
		err = mpBufferQ->PostMsg(&pBuffer, sizeof(pBuffer));
		AM_ASSERT_OK(err);
		pBuffer = (CBuffer*)((AM_U8*)pBuffer + objectSize);
	}

	return ME_OK;
}

CSimpleBufferPool::~CSimpleBufferPool()
{
	delete[] mpBufferMemory;
}

//-----------------------------------------------------------------------
//
// CFixedBufferPool
//
//-----------------------------------------------------------------------

CFixedBufferPool *CFixedBufferPool::Create(AM_UINT size, AM_UINT count)
{
    CFixedBufferPool *result = new CFixedBufferPool;
    if (result != NULL && result->Construct(size, count) != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CFixedBufferPool::Construct(AM_UINT size, AM_UINT count)
{
    AM_ERR err;

    if ((err = inherited::Construct(count)) != ME_OK)
        return err;

    if ((_pBuffers = new CBuffer[count]) == NULL)
    {
        AM_ERROR("CFixedBufferPool allocate CBuffers fail.\n");
        return ME_ERROR;
    }

    size = ROUND_UP(size, 4);
    if ((_pMemory = new AM_U8[count * size]) == NULL) {
        AM_ERROR("CFixedBufferPool allocate data buffer fail.\n");
        return ME_ERROR;
    }

    AM_U8 *ptr = _pMemory;
    CBuffer *pBuffer = _pBuffers;
    for (AM_UINT i = 0; i < count; i++, ptr += size, pBuffer++) {
        pBuffer->mpData = ptr;
        pBuffer->mpDataBase = ptr;
        pBuffer->mBlockSize = size;
        pBuffer->mpPool = this;

        err = mpBufferQ->PostMsg(&pBuffer, sizeof(pBuffer));
        AM_ASSERT_OK(err);
    }

    return ME_OK;
}

CFixedBufferPool::~CFixedBufferPool()
{
    if(_pMemory) {
        delete[] _pMemory;
        _pMemory = NULL;
    }
    if(_pBuffers) {
        delete _pBuffers;
        _pBuffers = NULL;
    }
}

void CFixedBufferPool::Delete()
{
    if(_pMemory) {
        delete[] _pMemory;
        _pMemory = NULL;
    }
    if(_pBuffers) {
        delete _pBuffers;
        _pBuffers = NULL;
    }
    inherited::Delete();
}

//-----------------------------------------------------------------------
//
//  CWorkQueue
//
//-----------------------------------------------------------------------
CWorkQueue* CWorkQueue::Create(AO *pAO)
{
	CWorkQueue *result = new CWorkQueue(pAO);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

void CWorkQueue::Delete()
{
	delete this;
}

AM_ERR CWorkQueue::Construct()
{
	if ((mpMsgQ = CQueue::Create(NULL, this, sizeof(AO::CMD), 16)) == NULL)
		return ME_NO_MEMORY;

	if ((mpThread = CThread::Create(mpAO->GetName(), ThreadEntry, this)) == NULL)
		return ME_OS_ERROR;

	return ME_OK;
}

CWorkQueue::~CWorkQueue()
{
         //AM_INFO("~CWorkQueue, name: %s\n", mpAO->GetName());
	if (mpThread) {
		Terminate();
		AM_DELETE(mpThread);
	}
	AM_DELETE(mpMsgQ);
         //AM_INFO("~CWorkQueue done\n");
}

AM_ERR CWorkQueue::ThreadEntry(void *p)
{
	AM_PRINTF("CWorkQueue::ThreadEntry 0x%08x->MainLoop() begin\n", (AM_U32)p);
	((CWorkQueue*)p)->MainLoop();
	AM_PRINTF("CWorkQueue::ThreadEntry 0x%08x->MainLoop() end\n", (AM_U32)p);
	return ME_OK;
}

void CWorkQueue::Terminate()
{
    AM_ERR err = SendCmd(AO::CMD_TERMINATE);
    AM_ASSERT_OK(err);
}

void CWorkQueue::MainLoop()
{
	AO::CMD cmd;
	while (1) {
		GetCmd(cmd);
                  //AM_INFO("cmd.code: %d, name:%s\n", cmd.code, mpAO->GetName());
		switch (cmd.code) {

		case AO::CMD_TERMINATE:
			AM_PRINTF("CWorkQueue::MainLoop this:0x%08x  AO::CMD_TERMINATE\n", (AM_U32)this);
			CmdAck(ME_OK);
			return;

		case AO::CMD_RUN:
			// CmdAck(ME_OK);
			AM_PRINTF("CWorkQueue::MainLoop this:0x%08x  mpAO:0x%08x->OnRun()\n", (AM_U32)this, (AM_U32)mpAO);
			mpAO->OnRun();
			AM_PRINTF("CWorkQueue::MainLoop this:0x%08x  mpAO:0x%08x->OnRun() end\n", (AM_U32)this, (AM_U32)mpAO);
			break;

		case AO::CMD_STOP:
			CmdAck(ME_OK);
			break;

		default:
			mpAO->OnCmd(cmd);
			break;
		}
	}
}

//-----------------------------------------------------------------------
//
//  CPin
//
//-----------------------------------------------------------------------
void *CPin::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IBufferPool)
		return mpBufferPool;
	if (refiid == IID_IPin)
		return (IPin*)this;
	return inherited::GetInterface(refiid);
}

CPin::~CPin()
{
	// force to release
	SetBufferPool(NULL);
}

void CPin::OnDisconnect()
{
	ReleaseBufferPool();
}

AM_ERR CPin::NoBufferPoolHandler()
{
	AM_ERROR("No buffer pool for pin of %s\n", FilterName());
	return ME_NO_INTERFACE;
}

void CPin::SetBufferPool(IBufferPool *pBufferPool)
{
	if (pBufferPool)
		pBufferPool->AddRef();

	if (mpBufferPool)
		mpBufferPool->Release();

	mpBufferPool = pBufferPool;
	mbSetBP = pBufferPool != NULL;
}

void CPin::ReleaseBufferPool()
{
	if (mpBufferPool && !mbSetBP) {
		mpBufferPool->SetNotifyOwner(NULL);
		mpBufferPool->Release();
		mpBufferPool = NULL;
	}
}


//-----------------------------------------------------------------------
//
// CInputPin
//
//-----------------------------------------------------------------------
AM_ERR CInputPin::Connect(IPin *pPeer)
{
	if (mpPeer) {
		AM_ERROR("Input pin of %s is already connected\n", FilterName());
		return ME_BAD_STATE;
	}

	AM_ERR err = OnConnect(pPeer);
	if (err != ME_OK)
		return err;

	mpPeer = pPeer;
	return ME_OK;
}

void CInputPin::Disconnect()
{
	if (mpPeer) {
		OnDisconnect();
		mpPeer = NULL;
	}
}

AM_ERR CInputPin::OnConnect(IPin *pPeer)
{
	CMediaFormat *pUpStreamMediaFormat = NULL;
	AM_ERR err;

	// check media format

	err = pPeer->GetMediaFormat(pUpStreamMediaFormat);
	if (err != ME_OK)
		return err;

	err = CheckMediaFormat(pUpStreamMediaFormat);
	if (err != ME_OK)
		return err;

	// check buffer pool

	if (mpBufferPool == NULL) {
		// if upstream can provide buffer pool
		if ((mpBufferPool = IBufferPool::GetInterfaceFrom(pPeer))) {
			mpBufferPool->AddRef();
		}
		else {
			err = NoBufferPoolHandler();
			if (err != ME_OK)
				return err;
		}
	}

	return ME_OK;
}

//-----------------------------------------------------------------------
//
//  COutputPin
//
//-----------------------------------------------------------------------
AM_ERR COutputPin::Connect(IPin *pPeer)
{
	if (mpPeer) {
		AM_ERROR("Output pin of %s is already connected\n", FilterName());
		return ME_BAD_STATE;
	}

	AM_ERR err = pPeer->Connect(this);
	if (err != ME_OK)
		return err;

	err = OnConnect(pPeer);
	if (err != ME_OK) {
		pPeer->Disconnect();
		return err;
	}

	mpBufferPool->SetNotifyOwner((void*)mpFilter);

	mpPeer = pPeer;
	return ME_OK;
}

void COutputPin::Disconnect()
{
	if (mpPeer == NULL)
		return;

	OnDisconnect();

	mpPeer->Disconnect();
	mpPeer = NULL;
}

AM_ERR COutputPin::OnConnect(IPin *pPeer)
{
	if (mpBufferPool == NULL) {
		if ((mpBufferPool = IBufferPool::GetInterfaceFrom(pPeer))) {
			mpBufferPool->AddRef();
		}
		else {
			AM_ERR err = NoBufferPoolHandler();
			if (err != ME_OK)
				return err;
		}
	}

	// check
	IBufferPool *pBufferPool = IBufferPool::GetInterfaceFrom(pPeer);
	if (mpBufferPool != pBufferPool) {
		AM_ERROR("Pin of %s uses different BP\n", FilterName());
		ReleaseBufferPool();
		return ME_ERROR;
	}

	return ME_OK;
}

//-----------------------------------------------------------------------
//
//  CActiveOutputPin
//
//-----------------------------------------------------------------------
void *CActiveOutputPin::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IActiveObject)
		return (IActiveObject*)this;
	return inherited::GetInterface(refiid);
}

AM_ERR CActiveOutputPin::Construct()
{
	if ((mpWorkQ = CWorkQueue::Create((IActiveObject*)this)) == NULL)
		return ME_NO_MEMORY;
	return ME_OK;
}

CActiveOutputPin::~CActiveOutputPin()
{
	AM_DELETE(mpWorkQ);
}

//-----------------------------------------------------------------------
//
//  CQueueInputPin
//
//-----------------------------------------------------------------------
AM_ERR CQueueInputPin::Construct(CQueue *pMsgQ)
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	if ((mpBufferQ = CQueue::Create(pMsgQ, this, sizeof(CBuffer*), 16)) == NULL)
		return ME_NO_MEMORY;

	return ME_OK;
}

CQueueInputPin::~CQueueInputPin()
{
	AM_DELETE(mpBufferQ);
}

void CQueueInputPin::Purge()
{
	if (mpBufferQ) {
		CBuffer *pBuffer;
		while (mpBufferQ->PeekData((void*)&pBuffer, sizeof(pBuffer))) {
			pBuffer->Release();
		}
	}
}
//-----------------------------------------------------------------------
//
// CGeneralInputPin
//
//-----------------------------------------------------------------------
/*
CGeneralInputPin* CGeneralInputPin::Create(CFilter* pFilter)
{
    CGeneralInputPin* result = new CGeneralInputPin(pFilter);
    if (result && result->Construct())
    {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CGeneralInputPin::Construct()
{
    AM_ERR err;
    CQueue* pQueue = ((CActiveFilter* )mpFilter)->MsgQ();
    err = inherited::Construct(pQueue);
    if (err != ME_OK)
    {
        AM_PRINTF("CGeneralInputPin::Construct Failed.\n");
        return err;
    }
    return ME_OK;
}

CGeneralInputPin::~CGeneralInputPin()
{
    AM_PRINTF("~CGeneralOutputPin\n");
}

AM_ERR CGeneralInputPin::CheckMediaFormat(CMediaFormat* pFormat)
{
    //add this setinputformat to CInterActiveFilter
    AM_ERR err;
    err = ((CInterActiveFilter* )mpFilter)->SetInputFormat(pFormat);
    if(err != ME_OK)
    {
        AM_PRINTF("CheckMediaFormat Failed!\n");
        return err;
    }
    return ME_OK;
}

//-----------------------------------------------------------------------
//
// CGeneralOutputPin
//
//-----------------------------------------------------------------------
CGeneralOutputPin* CGeneralOutputPin::Create(CFilter* pFilter)
{
    CGeneralOutputPin *result = new CGeneralOutputPin(pFilter);
    if (result && result->Construct() != ME_OK)
    {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CGeneralOutputPin::Construct()
{
    return ME_OK;
}

CGeneralOutputPin::~CGeneralOutputPin()
{
    AM_PRINTF("~CGeneralOutputPin\n");
}

AM_ERR CGeneralOutputPin::GetMediaFormat(CMediaFormat*& pMediaFormat)
{
    ((CInterActiveFilter* )mpFilter)->SetOutputFormat(&mMediaFormat);
    pMediaFormat = &mMediaFormat;
    return ME_OK;
}
*/
//-----------------------------------------------------------------------
//
//  CActiveInputPin
//
//-----------------------------------------------------------------------
AM_ERR CActiveInputPin::Construct()
{
	if ((mpWorkQ = CWorkQueue::Create((IActiveObject*)this)) == NULL)
		return ME_NO_MEMORY;

	AM_ERR err = inherited::Construct(mpWorkQ->MsgQ());
	if (err != ME_OK)
		return err;

	return ME_OK;
}

CActiveInputPin::~CActiveInputPin()
{
	AM_DELETE(mpBufferQ);
	mpBufferQ = NULL;
	AM_DELETE(mpWorkQ);
}

void *CActiveInputPin::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IActiveObject)
		return (IActiveObject*)this;
	return inherited::GetInterface(refiid);
}

// This is a example OnRun() and can be used in common-simple case.
// If the pin shall response to other commands besides CMD_STOP,
// it should override OnCmd().
void CActiveInputPin::OnRun()
{
	CmdAck(ME_OK);

	CQueue::WaitResult result;
	CBuffer *pBuffer;
	AO::CMD cmd;

	while (1) {
		CQueue::QType type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);

		if (type == CQueue::Q_MSG) {
			// running state, received a cmd
			if (cmd.code == AO::CMD_STOP) {
				CmdAck(ME_OK);
				break;
			}
		}
		else {
			// a buffer
			if (PeekBuffer(pBuffer)) {
				AM_ERR err = ProcessBuffer(pBuffer);
				if (err != ME_OK) {
					if (err != ME_CLOSED)
						PostEngineErrorMsg(err);
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------
//
//  CFilter
//
//-----------------------------------------------------------------------
AM_ERR CFilter::Construct()
{
	INFO info;
	info.pName = "Filter";
	GetInfo(info);
	mpName = info.pName;
	return ME_OK;
}

void *CFilter::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IFilter)
		return (IFilter*)this;
	return inherited::GetInterface(refiid);
}
//-----------------------------------------------------------------------
//
//  CActiveObject
//
//-----------------------------------------------------------------------
AM_ERR CActiveObject::Construct()
{
    if ((mpWorkQ = CWorkQueue::Create((IActiveObject*)this)) == NULL)
        return ME_NO_MEMORY;

    return ME_OK;
}

CActiveObject::~CActiveObject()
{
    AM_DELETE(mpWorkQ);
}

void *CActiveObject::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IActiveObject)
        return (IActiveObject*)this;
    return inherited::GetInterface(refiid);
}
//-----------------------------------------------------------------------
//
//  CActiveQueue
//
//-----------------------------------------------------------------------
AM_ERR CActiveQueue::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
    {
        AM_ERROR("CInterActiveFilter::Construct fail err %d .\n", err);
        return err;
    }
    if ((mpBufferQ = CQueue::Create(MsgQ(), this, sizeof(CBuffer*), mTotal)) == NULL)
        return ME_NO_MEMORY;


    return ME_OK;
}

//take care for CBuffer release
AM_ERR CActiveQueue::PushBuffer(CBuffer* pBuffer)
{
    if(mCount == mTotal)
    {
        //should not post
    }
    mpBufferQ->PutData(&pBuffer, sizeof(pBuffer));
    mCount++;
    return ME_OK;
}

AM_ERR CActiveQueue::PopBuffer(CBuffer* pBuffer)
{
    if(mpBufferQ->PeekData(&pBuffer, sizeof(pBuffer)))
        return ME_OK;
    return ME_BAD_STATE;
    mCount--;
}
AM_UINT CActiveQueue::QueueSpace()
{
    AM_ASSERT(mpBufferQ->GetDataCnt() == mCount);
    AM_UINT space = mTotal - mCount;
    return space;
}

bool CActiveQueue::IsQueueFull()
{
    AM_ASSERT(mpBufferQ->GetDataCnt() == mCount);
    return mCount == mTotal;
}
//-----------------------------------------------------------------------
//
//  CActiveFilter
//
//-----------------------------------------------------------------------
AM_ERR CActiveFilter::Construct()
{
	if ((mpWorkQ = CWorkQueue::Create((IActiveObject*)this)) == NULL)
		return ME_NO_MEMORY;

	return ME_OK;
}

CActiveFilter::~CActiveFilter()
{
    AM_PRINTF("~CActiveFilter\n");
    AM_DELETE(mpWorkQ);
    AM_PRINTF("~CActiveFilter DOne\n");

}

void *CActiveFilter::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IActiveObject)
        return (IActiveObject*)this;
    return inherited::GetInterface(refiid);
}

void CActiveFilter::GetInfo(INFO& info)
{
	info.nInput = 0;
	info.nOutput = 0;
	info.pName = mpName;
}

bool CActiveFilter::WaitInputBuffer(CQueueInputPin*& pPin, CBuffer*& pBuffer)
{
	CQueue::WaitResult result;
	AO::CMD cmd;

	while (true) {
		CQueue::QType type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
		if (type == CQueue::Q_MSG) {
			if (!ProcessCmd(cmd)) {
				if (cmd.code == AO::CMD_STOP) {
					OnStop();
					CmdAck(ME_OK);
					return false;
				}
			}
		}
		else {
			pPin = (CQueueInputPin*)result.pOwner;
			if (pPin->PeekBuffer(pBuffer))
				return true;
			AM_PRINTF("No buffer?\n");
		}
	}
}
//-----------------------------------------------------------------------
//
//  CInterActiveFilter
//
//-----------------------------------------------------------------------
AM_ERR CInterActiveFilter::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
    {
        AM_ERROR("CInterActiveFilter::Construct fail err %d .\n", err);
        return err;
    }

    //CMsgSys* pMsgsys;
    /* make compile don't support this   -_-||
    CBaseEngine* pEngine = dynamic_cast<CBaseEngine* >(mpEngine);
    if(pEngine == NULL)
    {
        pMsgsys = CMsgSys::Init();
    }else{
        pMsgsys = pEngine->GetMsgSys();
    }
    */
    AM_ASSERT(mpSharedRes->mEngineVersion);
    mpMsgsys = (CMsgSys* )mpEngine->GetInterface(IID_IMSGSYS);
    if(mpMsgsys == NULL)
        return ME_NO_MEMORY;

    mpFilterPort = CMsgPort::Create((IMsgSink*)this, mpMsgsys);
    if(mpFilterPort == NULL)
        return ME_NO_MEMORY;
    return ME_OK;
}

void CInterActiveFilter::PostMsgToFilter(AM_MSG & msg)
{
    mpFilterPort->PostMsg(msg);
}

void CInterActiveFilter::SendMsgToFilter(AM_MSG& msg)
{
    mpFilterPort->SendMsg(msg);//will behold utill be processed
}

void CInterActiveFilter::MsgProc(AM_MSG & msg)
{

}

AM_ERR CInterActiveFilter::Remove(AM_INT index, AM_U8 flag)
{
    AM_ERR err;
    CMD cmd(CMD_REMOVE);
    cmd.res32_1 = index;
    cmd.flag = flag;
    CQueue* q = mpWorkQ->MsgQ();
    err = q->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CInterActiveFilter::Resume(AM_INT target, AM_U8 flag)
{
    AM_ERR err;
    CMD cmd(CMD_RESUME);
    cmd.res32_1 = target;
    cmd.flag = flag;
    CQueue* q = mpWorkQ->MsgQ();
    err = q->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CInterActiveFilter::Pause(AM_INT target, AM_U8 flag)
{
    AM_ERR err;
    CMD cmd(CMD_PAUSE);
    cmd.res32_1 = target;
    cmd.flag = flag;
    CQueue* q = mpWorkQ->MsgQ();
    err = q->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CInterActiveFilter::Flush(AM_INT target, AM_U8 flag)
{
    AM_ERR err;
    CMD cmd(CMD_FLUSH);
    cmd.res32_1 = target;
    cmd.flag = flag;
    CQueue* q = mpWorkQ->MsgQ();
    err = q->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CInterActiveFilter::Config(AM_INT target, AM_U8 flag)
{
    AM_ERR err;
    CMD cmd(CMD_CONFIG);
    cmd.res32_1 = target;
    cmd.flag = flag;
    CQueue* q = mpWorkQ->MsgQ();
    err = q->SendMsg(&cmd, sizeof(cmd));
    return err;
}

void *CInterActiveFilter::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IActiveObject)
        return (IActiveObject*)this;
    return inherited::GetInterface(refiid);
}

CInterActiveFilter::~CInterActiveFilter()
{
    AM_DELETE(mpFilterPort);
    //AM_DELETE(mpMsgsys);
}

//-----------------------------------------------------------------------
//
//  CBaseEngine
//
//-----------------------------------------------------------------------

CEngineMsgProxy* CEngineMsgProxy::Create(CBaseEngine *pEngine,CMsgSys *pMsgSys)
{
	CEngineMsgProxy *result = new CEngineMsgProxy(pEngine,pMsgSys);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CEngineMsgProxy::Construct()
{
	mpMsgPort = CMsgPort::Create((IMsgSink*)this,mpMsgSys);
	if (mpMsgPort == NULL)
		return ME_NO_MEMORY;

	return ME_OK;
}

CEngineMsgProxy::~CEngineMsgProxy()
{
	AM_DELETE(mpMsgPort);
}

void *CEngineMsgProxy::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IMsgSink)
		return (IMsgSink*)this;
	return inherited::GetInterface(refiid);
}

void CEngineMsgProxy::MsgProc(AM_MSG& msg)
{
	mpEngine->OnAppMsg(msg);
}


AM_ERR CBaseEngine::Construct()
{
	if ((mpMutex = CMutex::Create(true)) == NULL)
		return ME_NO_MEMORY;
	if ((mpMsgsys = CMsgSys::Init()) == NULL)
		return ME_NO_MEMORY;

	mpPortFromFilters = CMsgPort::Create(this,mpMsgsys);
	if (mpPortFromFilters == NULL)
		return ME_NO_MEMORY;

	mpMsgProxy = CEngineMsgProxy::Create(this,mpMsgsys);
	if (mpMsgProxy == NULL)
		return ME_NO_MEMORY;

	return ME_OK;
}

CBaseEngine::~CBaseEngine()
{
	AM_DELETE(mpMsgProxy);
	AM_DELETE(mpPortFromFilters);
	AM_DELETE(mpMutex);
	AM_DELETE(mpMsgsys);
}

void *CBaseEngine::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IEngine)
		return (IEngine*)this;
	if (refiid == IID_IMsgSink)
		return (IMsgSink*)this;
	if (refiid == IID_IMediaControl)
		return (IMediaControl*)this;
	return inherited::GetInterface(refiid);
}

AM_ERR CBaseEngine::PostEngineMsg(AM_MSG& msg)
{
//	AUTO_LOCK(mpMutex);
	msg.sessionID = mSessionID;
	return mpPortFromFilters->PostMsg(msg);
}

AM_ERR CBaseEngine::SetAppMsgSink(IMsgSink *pAppMsgSink)
{
	AUTO_LOCK(mpMutex);
	mpAppMsgSink = pAppMsgSink;
	return ME_OK;
}

AM_ERR CBaseEngine::SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&), void *context)
{
	AUTO_LOCK(mpMutex);
	mAppMsgProc = MsgProc;
	mAppMsgContext = context;
	return ME_OK;
}

AM_ERR CBaseEngine::PostAppMsg(AM_MSG& msg)
{
//	AUTO_LOCK(mpMutex);
	msg.sessionID = mSessionID;
	return mpMsgProxy->MsgPort()->PostMsg(msg);
}

// called by proxy
void CBaseEngine::OnAppMsg(AM_MSG& msg)
{
	AUTO_LOCK(mpMutex);
	if (msg.sessionID == mSessionID) {
		if (mpAppMsgSink)
			mpAppMsgSink->MsgProc(msg);
		else if (mAppMsgProc)
			mAppMsgProc(mAppMsgContext, msg);
	}
}
//-----------------------------------------------------------------------
//
//  CActiveEngine
//
//-----------------------------------------------------------------------
AM_ERR CActiveEngine::Construct()
{
    if ((mpMutex = CMutex::Create(true)) == NULL)
        return ME_NO_MEMORY;

    if ((mpWorkQ = CWorkQueue::Create((IActiveObject*)this)) == NULL) {
        AM_DELETE(mpMutex);
        mpMutex = NULL;
        return ME_NO_MEMORY;
    }
    mpCmdQueue = mpWorkQ->MsgQ();
    if ((mpFilterMsgQ = CQueue::Create(mpCmdQueue, this, sizeof(AM_MSG), 1)) == NULL) {
        AM_DELETE(mpMutex);
        mpMutex = NULL;
        AM_DELETE(mpWorkQ);
        mpWorkQ = NULL;
        return ME_NO_MEMORY;
    }

    return ME_OK;
}

CActiveEngine::~CActiveEngine()
{
    AM_DELETE(mpFilterMsgQ);
    AM_DELETE(mpWorkQ);
    AM_DELETE(mpMutex);
}

void * CActiveEngine::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IActiveObject)
        return (IActiveObject*)this;
    if (refiid == IID_IEngine)
        return (IEngine*)this;
    if (refiid == IID_IMsgPort)
        return (IMsgPort*)this;
    if (refiid == IID_IMediaControl)
        return (IMediaControl*)this;
    return inherited::GetInterface(refiid);
}

AM_ERR CActiveEngine::PostEngineMsg(AM_MSG& msg)
{
    AM_ASSERT(mpFilterMsgQ);
    AM_ASSERT(mpWorkQ);
    {
        AUTO_LOCK(mpMutex);
        msg.sessionID = mSessionID;
    }
    return mpFilterMsgQ->PutData(&msg, sizeof(AM_MSG));
}

AM_ERR CActiveEngine::SetAppMsgSink(IMsgSink *pAppMsgSink)
{
    AUTO_LOCK(mpMutex);
    mpAppMsgSink = pAppMsgSink;
    return ME_OK;
}

AM_ERR CActiveEngine::SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&), void *context)
{
    AUTO_LOCK(mpMutex);
    mAppMsgProc = MsgProc;
    mAppMsgContext = context;
    return ME_OK;
}

AM_ERR CActiveEngine::PostAppMsg(AM_MSG& msg)
{
    AUTO_LOCK(mpMutex);
    if (msg.sessionID == mSessionID) {
        if (mpAppMsgSink) {
            mpAppMsgSink->MsgProc(msg);
        }
        else if (mAppMsgProc) {
            mAppMsgProc(mAppMsgContext, msg);
            }
        else {
            AMLOG_ERROR("no app msg sink or msg call back.\n");
            return ME_ERROR;
        }
    } else {
        AMLOG_WARN("should not comes here, not correct seesion id %d, engine's session id %d.\n", msg.sessionID, mSessionID);
    }
    return ME_OK;
}

//-----------------------------------------------------------------------
//
//  CFilterManager
//
//-----------------------------------------------------------------------
CFilterGraph::~CFilterGraph()
{
	ClearGraph();
}

void CFilterGraph::ClearGraph()
{
    if (mnFilters == 0)
        return;

    AM_INFO("=== Clear graph, filter cnt %d.\n", mnFilters);
    StopAllFilters();
    DisableAllOutput();
    PurgeAllFilters();
    DeleteAllConnections();
    RemoveAllFilters();
    AM_INFO("Clear graph done\n");
}

AM_ERR CFilterGraph::RunAllFilters()
{
	AMLOG_INFO("\n=== RunAllFilters\n");
	for (AM_UINT i = mnFilters; i > 0; i--) {
		IFilter *pFilter = mFilters[i-1].pFilter;
		AMLOG_INFO("Run %s (%d)\n", GetFilterName(pFilter), i);
		EnableOutputPins(pFilter);
		AM_ERR err = pFilter->Run();
		if (err != ME_OK) {
			AMLOG_INFO("Run %s (%d) returns %d\n", GetFilterName(pFilter), i, err);
			return err;
		}
	}
	AMLOG_INFO("RunAllFilters OK\n");
	return ME_OK;
}

void CFilterGraph::DisableAllOutput()
{
	//AM_INFO("\n=== DisableAllOutput\n");
	for (AM_UINT i = 0; i < mnFilters; i++) {
		IFilter *pFilter = mFilters[i].pFilter;
		AMLOG_INFO("Disable output %s (%d)\n", GetFilterName(pFilter), i+1);
		DisableOutputPins(pFilter);
	}
	//AM_INFO("DisableAllOutput OK\n");
}

void CFilterGraph::StopAllFilters()
{
	AMLOG_INFO("\n=== StopAllFilters\n");
	for (AM_UINT i = 0; i < mnFilters; i++) {
		IFilter *pFilter = mFilters[i].pFilter;
		AMLOG_INFO("Stop %s (%d)\n", GetFilterName(pFilter), i+1);
		AM_ERR err = pFilter->Stop();
		AM_ASSERT_OK(err);
	}
	AMLOG_INFO("StopAllFilters OK\n");
}

void CFilterGraph::PurgeAllFilters()
{
	//AM_INFO("\n=== PurgeAllFilters\n");
	for (AM_UINT i = 0; i < mnFilters; i++) {
		AM_INFO("Purge %s (%d)\n", GetFilterName(mFilters[i].pFilter), i + 1);
		PurgeFilter(mFilters[i].pFilter);
	}
	//AM_INFO("PurgeAllFilters done\n");
}

void CFilterGraph::FlushAllFilters()
{
    AMLOG_INFO("\n=== FlushAllFilters\n");
    for (AM_UINT i = 0; i < mnFilters; i++) {
    AMLOG_INFO("Flush %s (%d)\n", GetFilterName(mFilters[i].pFilter), i + 1);
        mFilters[i].pFilter->Flush();
    }
    AMLOG_INFO("FlushAllFilters done\n");

}

void CFilterGraph::DeleteAllConnections()
{
	AMLOG_INFO("\n=== DeleteAllConnections\n");
	for (AM_UINT i = mnConnections; i > 0; i--) {
		mConnections[i-1].pOutputPin->Disconnect();
	}
	mnConnections = 0;
	AMLOG_INFO("DeleteAllConnections done\n");
}

void CFilterGraph::RemoveAllFilters()
{
	AMLOG_INFO("\n=== RemoveAllFilters\n");
	for (AM_UINT i = mnFilters; i > 0; i--) {
		AMLOG_INFO("**Remove %s (%d)\n", GetFilterName(mFilters[i-1].pFilter), i);
		mFilters[i-1].pFilter->Delete();
	}
	mnFilters = 0;
	AMLOG_INFO("**RemoveAllFilters done\n");
}

void CFilterGraph::EnableOutputPins(IFilter *pFilter, bool bEnable)
{
	IFilter::INFO info;
	pFilter->GetInfo(info);
	for (AM_UINT j = 0; j < info.nOutput; j++) {
		IPin *pPin = pFilter->GetOutputPin(j);
		if (pPin != NULL)
			pPin->Enable(bEnable);
	}
}

AM_ERR CFilterGraph::AddFilter(IFilter *pFilter)
{
	if (mnFilters >= ARRAY_SIZE(mFilters)) {
		AMLOG_ERROR("too many filters\n");
		return ME_TOO_MANY;
	}


	mFilters[mnFilters].pFilter = pFilter;
	mFilters[mnFilters].flags = 0;
	mnFilters++;

	AMLOG_INFO("Added %s (%d)\n", GetFilterName(pFilter), mnFilters);

	return ME_OK;
}

IFilter* CFilterGraph::FindFilterbyIndex(AM_INT& index)
{
        if(index < 0 ||(AM_UINT)index >= mnFilters)
            return NULL;
        return mFilters[index].pFilter;
}

AM_ERR CFilterGraph::FindFilter(IFilter *pFilter, AM_UINT& index)
{
	for (AM_UINT i = 0; i < mnFilters; i++) {

		if (pFilter == mFilters[i].pFilter) {
			index = i;
			return ME_OK;
		}
	}

	AM_ERROR("Filter not found\n");
	return ME_ERROR;
}

bool CFilterGraph::AllRenderersReady()
{
    IFilter::INFO info;
    for (AM_UINT i = 0; i < mnFilters; i++) {
        IFilter *pFilter = mFilters[i].pFilter;
        pFilter->GetInfo(info);
        if(info.nOutput == 0) {
            if (!HasReady(i)) {
                AMLOG_INFO("renderer %d, flag 0x%x, not ready.\n", i, mFilters[i].flags);
                return false;
            }
        }
    }

    return true;
}

bool CFilterGraph::AllRenderersEOS()
{
	IFilter::INFO info;
	for (AM_UINT i = 0; i < mnFilters; i++) {
		IFilter *pFilter = mFilters[i].pFilter;
		pFilter->GetInfo(info);
		if (info.nOutput == 0) {
			if (!HasEOS(i)) {
                            //AMLOG_INFO("**all renderer eos false %d, 0x%x.\n", i,  mFilters[i].flags);
				return false;
			    }
		}
	}
	return true;
}

bool CFilterGraph::AllFiltersHasFlag(AM_UINT flag)
{
    IFilter::INFO info;
    for (AM_UINT i = 0; i < mnFilters; i++) {
        IFilter *pFilter = mFilters[i].pFilter;
        pFilter->GetInfo(info);
        AMLOG_PRINTF(" AllFiltersHasFlag: index %d, info.mFlags 0x%x, flag 0x%x.\n", i, info.mFlags, flag);
        if (info.mFlags & flag) {
            if (!(mFilters[i].flags & flag))
                return false;
        }
    }
    return true;
}

void CFilterGraph::AllFiltersCmd(AM_UINT cmd)
{
    AM_UINT i = 0;
    IFilter::INFO info;
    AMLOG_INFO("AllFiltersCmd %d.\n", cmd);

    if (cmd == IActiveObject::CMD_AVSYNC) {
        for (i=0; i<mnFilters; i++) {
            mFilters[i].pFilter->GetInfo(info);
            if (info.mFlags & SYNC_FLAG) {
                mFilters[i].pFilter->SendCmd(cmd);
                AMLOG_INFO("Filter[%u] Cmd[%u] SendCmd done.\n",i,cmd);
            }
        }
    } else {
        for (i=0; i<mnFilters; i++) {
            mFilters[i].pFilter->SendCmd(cmd);
            AMLOG_INFO("Filter[%u] Cmd[%u] SendCmd done.\n",i,cmd);
        }
    }
    AMLOG_INFO("AllFiltersCmd done.\n");
}

void CFilterGraph::AllFiltersMsg(AM_UINT msg)
{
    AM_UINT i = 0;
    IFilter::INFO info;
    AMLOG_INFO("AllFiltersMsg %d.\n", msg);
    if (msg == IActiveObject::CMD_UDEC_IN_RUNNING_STATE) {
        for (i=0; i<mnFilters; i++) {
            mFilters[i].pFilter->GetInfo(info);
            if (info.mFlags & RECIEVE_UDEC_RUNNING_FLAG) {
                AMLOG_INFO("Filter[%u] msg[%u] PostMsg start.\n",i,msg);
                mFilters[i].pFilter->PostMsg(msg);
                AMLOG_INFO("Filter[%u] msg[%u] PostMsg done.\n",i,msg);
            }
        }
    } else {
        for(i=0; i<mnFilters; i++) {
            mFilters[i].pFilter->PostMsg(msg);
            AMLOG_INFO("Filter[%u] Msg[%u] PostMsg done.\n",i,msg);
        }
    }
    AMLOG_INFO("AllFiltersMsg done.\n");
}

void CFilterGraph::AllFiltersClearFlag(AM_UINT flag)
{
    AMLOG_INFO("AllFiltersClearFlag %d.\n", flag);
    for(AM_UINT i=0; i<mnFilters; i++) {
        mFilters[i].flags &= ~flag;
    }
    AMLOG_INFO("AllFiltersClearFlag done.\n");
}

void CFilterGraph::ClearReadyEOS()
{
	for (AM_UINT i = 0; i < mnFilters; i++)
		mFilters[i].flags &= ~(EOS_FLAG | READY_FLAG);
}

void CFilterGraph::StartAllRenderers()
{
	IFilter::INFO info;
	for (AM_UINT i = 0; i < mnFilters; i++) {
		IFilter *pFilter = mFilters[i].pFilter;
		pFilter->GetInfo(info);
		if (info.nOutput == 0)
			pFilter->Start();
	}

}

AM_ERR CFilterGraph::Connect(IFilter *pUpstreamFilter, AM_UINT indexUp,
	IFilter *pDownstreamFilter, AM_UINT indexDown)
{
	IPin *pOutput = pUpstreamFilter->GetOutputPin(indexUp);
	if (pOutput == NULL) {
		AM_ERROR("No such pin: %s[%d]\n", GetFilterName(pUpstreamFilter), indexUp);
		return ME_ERROR;
	}

	IPin *pInput = pDownstreamFilter->GetInputPin(indexDown);
	if (pInput == NULL) {
		AM_ERROR("No such pin: %s[%d]\n", GetFilterName(pDownstreamFilter), indexDown);
		return ME_ERROR;
	}

	return CreateConnection(pOutput, pInput);
}

AM_ERR CFilterGraph::CreateConnection(IPin *pOutputPin, IPin *pInputPin)
{
	if (mnConnections >= ARRAY_SIZE(mConnections)) {
		AM_ERROR("too many connections\n");
		return ME_TOO_MANY;
	}

	AM_ERR err = pOutputPin->Connect(pInputPin);
	if (err != ME_OK)
		return err;

	mConnections[mnConnections].pOutputPin = pOutputPin;
	mConnections[mnConnections].pInputPin = pInputPin;
	mnConnections++;

	return ME_OK;
}

const char *CFilterGraph::GetFilterName(IFilter *pFilter)
{
	IFilter::INFO info;
	pFilter->GetInfo(info);
	return info.pName;
}

void CFilterGraph::PurgeFilter(IFilter *pFilter)
{
	IFilter::INFO info;
	pFilter->GetInfo(info);
	for (AM_UINT i = 0; i < info.nInput; i++) {
		IPin *pPin = pFilter->GetInputPin(i);
		if (pPin)
			pPin->Purge();
	}
}

//-----------------------------------------------------------------------
//
// CSystemClockSource
//
//-----------------------------------------------------------------------
CSystemClockSource* CSystemClockSource::Create()
{
	CSystemClockSource *result = new CSystemClockSource;
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

void *CSystemClockSource::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IClockSource)
		return (IClockSource*)this;
	return inherited::GetInterface(refiid);
}

am_pts_t CSystemClockSource::GetClockTime()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return ((am_pts_t)tv.tv_sec * mScaleSec + tv.tv_usec/mScaleUSec);
}


am_pts_t CSystemClockSource::GetClockBase()
{
	return GetClockTime();
}

void CSystemClockSource::SetClockState(CLOCK_STATE state)
{

}



//-----------------------------------------------------------------------
//
// CClockManager
//
//-----------------------------------------------------------------------
CClockManager* CClockManager::Create()
{
	CClockManager *result = new CClockManager;
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CClockManager::Construct()
{
	if ((mpMutex = CMutex::Create()) == NULL)
		return ME_ERROR;

	if ((mpTimerEvent = CEvent::Create()) == NULL)
		return ME_ERROR;

	if ((mpSystemClockSource = CSystemClockSource::Create()) == NULL)
		return ME_ERROR;

    mpSystemClockSource->SetClockUnit(1, 90000);//set 90khz here, move setting to engine?
    mpSource = mpSystemClockSource;

	if ((mpWorkQ = CWorkQueue::Create((IActiveObject*)this)) == NULL)
		return ME_ERROR;

	return ME_OK;
}

CClockManager::~CClockManager()
{
	AM_DELETE(mpWorkQ);
	AM_DELETE(mpSystemClockSource);
	AM_DELETE(mpTimerEvent);
	AM_DELETE(mpMutex);
}

void *CClockManager::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IClockManager)
		return (IClockManager*)this;
	return inherited::GetInterface(refiid);
}

AM_ERR CClockManager::SetTimer(IClockObserver *pObserver, am_pts_t pts)
{
	AUTO_LOCK(mpMutex);

	for (int i = 0; i < MAX_TIMERS; i++)
		if (mTimerNodes[i].pObserver == NULL) {
			mTimerNodes[i].pObserver = pObserver;
			mTimerNodes[i].pts = pts;
			return ME_OK;
		}

	return ME_TOO_MANY;
}


AM_ERR CClockManager::DeleteTimer(IClockObserver *pObserver)
{
	AUTO_LOCK(mpMutex);

	for (int i = 0; i < MAX_TIMERS; i++)
		if (mTimerNodes[i].pObserver == pObserver) {
			mTimerNodes[i].pObserver = NULL;
			return ME_OK;
		}

	return ME_ERROR;
}


am_pts_t CClockManager::GetCurrentTime()
{
	AUTO_LOCK(mpMutex);
	AMLOG_VERBOSE("mpSource->GetClockTime()=%llu, mStartTime=%llu, mStartPts=%llu.\n",mpSource->GetClockTime(), mStartTime, mStartPts);
	if(!mbPaused)
		return (mpSource->GetClockTime() - mStartTime) * mClockRate + mStartPts;
	else
		return (mPausedTime - mStartTime) * mClockRate + mStartPts;
}


AM_ERR CClockManager::StartClock()
{
	AUTO_LOCK(mpMutex);
	mStartTime = mpSource->GetClockBase();
	mpTimerEvent->Clear();
	mpSource->SetClockState(IClockSource::RUNNING);
	AMLOG_DEBUG("CClockManager::StartClock: mpSource->GetClockTime()=%llu, mStartTime=%llu, mStartPts=%llu.\n",mpSource->GetClockTime(), mStartTime, mStartPts);
	return mpWorkQ->Run();
}

void CClockManager::PauseClock()
{
	AUTO_LOCK(mpMutex);
	//store current time
	mPausedTime = mpSource->GetClockTime();
	mpSource->SetClockState(IClockSource::PAUSED);
	mpWorkQ->PostMsg(CMD_PAUSE);
	AMLOG_DEBUG("CClockManager::PauseClock: mStartTime=%llu, mStartPts=%llu, mPausedTime=%llu.\n", mStartTime, mStartPts, mPausedTime);
}

void CClockManager::ResumeClock()
{
	AUTO_LOCK(mpMutex);
	//update mStartTime
	mStartTime += mpSource->GetClockTime() - mPausedTime;
	mpSource->SetClockState(IClockSource::RUNNING);
	mpWorkQ->PostMsg(CMD_RESUME);
	AMLOG_DEBUG("CClockManager::PauseClock: mStartTime=%llu, mStartPts=%llu, mPausedTime=%llu.\n", mStartTime, mStartPts, mPausedTime);
}

void CClockManager::StopClock()
{
	//AUTO_LOCK(mpMutex);
	__LOCK(mpMutex);
	mpSource->SetClockState(IClockSource::STOPPED);

	//if (mpSource == mpSystemClockSource)
	//	mStartPts = GetCurrentTime();
	mpTimerEvent->Signal();
	__UNLOCK(mpMutex);

	AM_PRINTF("CClockManager::StopClock, call mpWorkQ->Stop().\n");
	mpWorkQ->Stop();
	AM_PRINTF("CClockManager::StopClock, call mpWorkQ->Stop() done.\n");

	__LOCK(mpMutex);
	mpTimerEvent->Clear();
	__UNLOCK(mpMutex);
}

void CClockManager::SetSource(IClockSource *pSource)
{
	AUTO_LOCK(mpMutex);
	if (pSource)
		mpSource = pSource;
	else
		mpSource = mpSystemClockSource;
}

void CClockManager::SetStartTime(am_pts_t time)
{
	AUTO_LOCK(mpMutex);
	mStartTime = time;
}

am_pts_t CClockManager::GetStartTime()
{
	AUTO_LOCK(mpMutex);
	return mStartTime;
}

void CClockManager::SetStartPts(am_pts_t time)
{
	AUTO_LOCK(mpMutex);
	mStartPts = time;
}

am_pts_t CClockManager::GetStartPts()
{
	AUTO_LOCK(mpMutex);
	return mStartPts;
}

void CClockManager::SetClockMgr(am_pts_t startPts)
{
	AUTO_LOCK(mpMutex);
	mStartPts = startPts;
	mStartTime = mpSource->GetClockTime();
}

void CClockManager::PurgeClock()
{
    AUTO_LOCK(mpMutex);

    for (AM_UINT i = 0; i < MAX_TIMERS; i++) {
        mTimerNodes[i].pObserver = NULL;
        mTimerNodes[i].pts = 0;
    }
}

bool CClockManager::ProcessCMD(AO::CMD& cmd)
{
	switch (cmd.code) {
		case CMD_STOP:
			mpWorkQ->CmdAck(ME_OK);
			AM_PRINTF("cmd.code == AO::CMD_STOP");
			return true;

		case CMD_PAUSE:
			mbPaused = true;
			break;

		case CMD_RESUME:
			mbPaused = false;
			break;

		default:
			AM_ERROR("error cmd %d.\n", cmd.code);
	}
	return false;
}

void CClockManager::OnRun()
{
	mbPaused = false;
	mpWorkQ->CmdAck(ME_OK);

	while (true) {
		AO::CMD cmd;

		//enter paused state
		if (mbPaused) {
			mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
			if(ProcessCMD(cmd))
				break;
			continue;
		}

		if (mpWorkQ->PeekCmd(cmd)) {
			if(ProcessCMD(cmd))
				break;
		}

		if (mpTimerEvent->Wait(10) == ME_OK)	// 10 ms
			break;

		CheckTimers();
	}
}

void CClockManager::CheckTimers()
{
	AUTO_LOCK(mpMutex);
	am_pts_t pts = GetCurrentTime();
	//am_pts_t pts = mpSource->GetClockPTS() - mBasePts;

	for (AM_UINT i = 0; i < MAX_TIMERS; i++) {
		if (mTimerNodes[i].pObserver) {
			if (pts >= mTimerNodes[i].pts) {
				mTimerNodes[i].pObserver->OnTimer(pts);
				mTimerNodes[i].pObserver = NULL;
			}
		}
	}
}
//-----------------------------------------------------------------------
//
// CFileReader
//
//-----------------------------------------------------------------------
CFileReader *CFileReader::Create()
{
	CFileReader *result = new CFileReader;
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CFileReader::Construct()
{
	if ((mpMutex = CMutex::Create(false)) == NULL)
		return ME_ERROR;
	return ME_OK;
}

CFileReader::~CFileReader()
{
	__CloseFile();
	AM_DELETE(mpMutex);
}

void *CFileReader::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IFileReader)
		return (IFileReader*)this;
	return inherited::GetInterface(refiid);
}

AM_ERR CFileReader::OpenFile(const char *pFileName)
{
	AUTO_LOCK(mpMutex);

	__CloseFile();

	mpFile = ::fopen(pFileName, "rb");
	if (mpFile == NULL) {
		AM_PERROR(pFileName);
		return ME_IO_ERROR;
	}

	return ME_OK;
}

AM_ERR CFileReader::CloseFile()
{
	AUTO_LOCK(mpMutex);
	__CloseFile();
	return ME_OK;
}

void CFileReader::__CloseFile()
{
	if (mpFile) {
		::fclose((FILE*)mpFile);
		mpFile = NULL;
	}
}

am_file_off_t CFileReader::GetFileSize()
{
	AUTO_LOCK(mpMutex);

	if (::fseeko((FILE*)mpFile, 0, SEEK_END) < 0) {
		AM_PERROR("fseeko");
		return 0;
	}

	unsigned long long size = ::ftello((FILE*)mpFile);
	return size;
}

AM_ERR CFileReader::ReadFile(am_file_off_t offset, void *pBuffer, AM_UINT size)
{
	AUTO_LOCK(mpMutex);

	if (::fseeko((FILE*)mpFile, offset, SEEK_SET) < 0) {
		AM_PERROR("fseeko");
		return ME_IO_ERROR;
	}

	if (::fread(pBuffer, 1, size, (FILE*)mpFile) != size) {
		AM_PERROR("fread");
		return ME_IO_ERROR;
	}

	return ME_OK;
}

//-----------------------------------------------------------------------
//
// globals
//
//-----------------------------------------------------------------------
extern AM_ERR AMF_Init()
{
//    AM_ERR err;

    OSAL_Init();
    AM_Util_Init();
    return ME_OK;
}

extern void AMF_Terminate()
{
	//CMsgSys::Delete();
	OSAL_Terminate();
}

