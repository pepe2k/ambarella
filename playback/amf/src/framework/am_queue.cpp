/**
 * queue.cpp
 *
 * History:
 *    2007/11/5 - [Oliver Li] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <string.h>
#if PLATFORM_ANDROID
#include "sys/atomics.h"
#endif

#include "am_new.h"
#include "am_types.h"
#include "osal.h"

#include "am_queue.h"


#define NODE_SIZE		(sizeof(List) + mBlockSize)
#define NODE_BUFFER(_pNode)	((AM_U8*)_pNode + sizeof(List))

// blockSize - bytes of each queue item
// nReservedSlots - number of reserved nodes
CQueue* CQueue::Create(CQueue *pMainQ, void *pOwner, AM_UINT blockSize, AM_UINT nReservedSlots)
{
	CQueue *result = new CQueue(pMainQ, pOwner);
	if (result && result->Construct(blockSize, nReservedSlots) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

void CQueue::Delete()
{
	delete this;
}

CQueue::CQueue(CQueue *pMainQ, void *pOwner):
	mpOwner(pOwner),
	mbDisabled(false),
	mbDetach(false),

	mpMainQ(pMainQ),
	mpPrevQ(this),
	mpNextQ(this),

	mpMutex(NULL),
	mpCondReply(NULL),
	mpCondGet(NULL),
	mpCondSendMsg(NULL),

	mnGet(0),
	mnSendMsg(0),

	mBlockSize(0),
	mnData(0),

	mpFreeList(NULL),

	mpSendBuffer(NULL),
	mpReservedMemory(NULL),

	mpMsgResult(NULL),
	mpCurrentCircularlyQueue(NULL)
{
    mHead.pNext = NULL;
    mHead.bAllocated = false;

    mpTail = (List*)&mHead;
}

AM_ERR CQueue::Construct(AM_UINT blockSize, AM_UINT nReservedSlots)
{
	mBlockSize = ROUND_UP(blockSize, 4);

	// mpSendBuffer + list_node + list_node + ... + list_node
	//mpReservedMemory = new AM_U8[NODE_SIZE * (nReservedSlots + 1)];
	mpReservedMemory = (AM_U8*)malloc(NODE_SIZE * (nReservedSlots + 1));
	if (mpReservedMemory == NULL)
		return ME_NO_MEMORY;

	// for SendMsg()
	mpSendBuffer = (List*)mpReservedMemory;
	mpSendBuffer->pNext = NULL;
	mpSendBuffer->bAllocated = false;

	// reserved nodes, keep in free-list
	List *pNode = (List*)(mpReservedMemory + NODE_SIZE);
	for (; nReservedSlots > 0; nReservedSlots--) {
		pNode->bAllocated = false;
		pNode->pNext = mpFreeList;
		mpFreeList = pNode;
		pNode = (List*)((AM_U8*)pNode + NODE_SIZE);
	}

	if (IsMain()) {
	        if ((mpMutex = CMutex::Create(false)) == NULL)
			return ME_OS_ERROR;

		if ((mpCondGet = CCondition::Create()) == NULL)
			return ME_OS_ERROR;

		if ((mpCondReply = CCondition::Create()) == NULL)
			return ME_OS_ERROR;

		if ((mpCondSendMsg = CCondition::Create()) == NULL)
			return ME_OS_ERROR;
	}
	else {
		mpMutex = mpMainQ->mpMutex;
		mpCondGet = mpMainQ->mpCondGet;
		mpCondReply = mpMainQ->mpCondReply;
		mpCondSendMsg = mpMainQ->mpCondSendMsg;

		// attach to main-Q
		AUTO_LOCK(mpMainQ->mpMutex);
		mpPrevQ = mpMainQ->mpPrevQ;
		mpNextQ = mpMainQ;
		mpPrevQ->mpNextQ = this;
		mpNextQ->mpPrevQ = this;
	}

	return ME_OK;
}

CQueue::~CQueue()
{
    if (mpMutex)
        __LOCK(mpMutex);

    AM_ASSERT(mnGet == 0);
    AM_ASSERT(mnSendMsg == 0);

    if (IsSub()) {
        // detach from main-Q
        mpPrevQ->mpNextQ = mpNextQ;
        mpNextQ->mpPrevQ = mpPrevQ;
        if(mpMainQ->mpCurrentCircularlyQueue == this){
            mpMainQ->mpCurrentCircularlyQueue = NULL;
        }
    } else {
        // all sub-Qs should be removed
        AM_ASSERT(mpPrevQ == this);
        AM_ASSERT(mpNextQ == this);
        AM_ASSERT(mpMsgResult == NULL);
    }

    AM_DESTRUCTOR("CQueue0x%p: before mHead.Delete(), mHead.pNext %p.\n", this, mHead.pNext);
    mHead.Delete();
    AM_DESTRUCTOR("CQueue0x%p: before AM_DELETE(mpFreeList), mpFreeList %p.\n", this, mpFreeList);
    AM_DELETE(mpFreeList);
    AM_DESTRUCTOR("after delete.\n");

    //delete[] mpReservedMemory;
    free(mpReservedMemory);
    AM_DESTRUCTOR("after delete mpReservedMemory.\n");
    if (mpMutex)
        __UNLOCK(mpMutex);

    if (IsMain()) {
        AM_DELETE(mpCondSendMsg);
        AM_DESTRUCTOR("after delete mpCondSendMsg.\n");
        AM_DELETE(mpCondReply);
        AM_DESTRUCTOR("after delete mpCondReply.\n");
        AM_DELETE(mpCondGet);
        AM_DESTRUCTOR("after delete mpCondGet.\n");
        AM_DELETE(mpMutex);
        AM_DESTRUCTOR("after delete mpMutex.\n");
    }
}

void CQueue::List::Delete()
{
	List *pNode = this;
	while (pNode) {
		List *pNext = pNode->pNext;
		if (pNode->bAllocated) {
          AM_DESTRUCTOR(" ----delete node %p.\n", pNode);
			//delete[] (AM_U8*)pNode;
			free(pNode);
		}
		pNode = pNext;
	}
}

AM_ERR CQueue::PostMsg(const void *pMsg, AM_UINT msgSize)
{
	AM_ASSERT(IsMain());
	AUTO_LOCK(mpMutex);

	List *pNode = AllocNode();
	if (pNode == NULL)
		return ME_NO_MEMORY;

	WriteData(pNode, pMsg, msgSize);

	if (mnGet > 0) {
		mnGet--;
		mpCondGet->Signal();
	}

	return ME_OK;
}

AM_ERR CQueue::SendMsg(const void *pMsg, AM_UINT msgSize)
{
	AM_ASSERT(IsMain());

	AUTO_LOCK(mpMutex);
	while (1) {
		if (mpMsgResult == NULL) {
			WriteData(mpSendBuffer, pMsg, msgSize);

			if (mnGet > 0) {
				mnGet--;
				mpCondGet->Signal();
			}

			AM_ERR result;
			mpMsgResult = &result;
			mpCondReply->Wait(mpMutex);
			mpMsgResult = NULL;

			if (mnSendMsg > 0) {
				mnSendMsg--;
				mpCondSendMsg->Signal();
			}

			return result;
		}

		mnSendMsg++;
		mpCondSendMsg->Wait(mpMutex);
	}
}

void CQueue::Reply(AM_ERR result)
{
	AUTO_LOCK(mpMutex);

	AM_ASSERT(IsMain());
	AM_ASSERT(mpMsgResult);

	*mpMsgResult = result;
	mpCondReply->Signal();
}

void CQueue::GetMsg(void *pMsg, AM_UINT msgSize)
{
	AM_ASSERT(IsMain());

	AUTO_LOCK(mpMutex);
	while (1) {
		if (mnData > 0) {
			ReadData(pMsg, msgSize);
			return;
		}

		mnGet++;
		mpCondGet->Wait(mpMutex);
	}
}

bool CQueue::GetMsgEx(void *pMsg, AM_UINT msgSize)
{
	AM_ASSERT(IsMain());

	AUTO_LOCK(mpMutex);
	while (1) {
		if (mbDisabled)
			return false;

		if (mnData > 0) {
			ReadData(pMsg, msgSize);
			return true;
		}

		mnGet++;
		mpCondGet->Wait(mpMutex);
	}
}

void CQueue::Enable(bool bEnabled)
{
	AM_ASSERT(IsMain());

	AUTO_LOCK(mpMutex);

	mbDisabled = !bEnabled;

	if (mnGet > 0) {
		mnGet = 0;
		mpCondGet->SignalAll();
	}
}

bool CQueue::PeekMsg(void *pMsg, AM_UINT msgSize)
{
	AM_ASSERT(IsMain());

	AUTO_LOCK(mpMutex);
	if (mnData > 0) {
		if (pMsg)
			ReadData(pMsg, msgSize);
		return true;
	}

	return false;
}

AM_ERR CQueue::PutData(const void *pBuffer, AM_UINT size)
{
	AM_ASSERT(IsSub());
	AUTO_LOCK(mpMutex);

	List *pNode = AllocNode();
	if (pNode == NULL)
		return ME_NO_MEMORY;

	WriteData(pNode, pBuffer, size);
	if (mpMainQ->mnGet > 0 && mbDetach == false) {
		mpMainQ->mnGet--;
		mpMainQ->mpCondGet->Signal();
	}

	return ME_OK;
}

// wait this main-Q and all its sub-Qs
CQueue::QType CQueue::WaitDataMsg(void *pMsg, AM_UINT msgSize, WaitResult *pResult)
{
	AM_ASSERT(IsMain());

	AUTO_LOCK(mpMutex);
	while (1) {
		if (mnData > 0) {
			ReadData(pMsg, msgSize);
			return Q_MSG;
		}
		for (CQueue *q = mpNextQ; q != this; q = q->mpNextQ) {
			if (q->mnData > 0) {
				pResult->pDataQ = q;
				pResult->pOwner = q->mpOwner;
				pResult->blockSize = q->mBlockSize;
				return Q_DATA;
			}
		}
		mnGet++;
		mpCondGet->Wait(mpMutex);
	}
}

// wait this main-Q and specified sub-Q
CQueue::QType CQueue::WaitDataMsgWithSpecifiedQueue(void *pMsg, AM_UINT msgSize, const CQueue *pQueue)
{
    AM_ASSERT(IsMain());

    if(pQueue->mpMainQ != this)
    {
        WaitMsg(pMsg, msgSize);
        return Q_MSG;
    }

    AUTO_LOCK(mpMutex);
    while (1) {
        if (mnData > 0) {
            ReadData(pMsg, msgSize);
            return Q_MSG;
        }

        if (pQueue->mnData > 0) {
            return Q_DATA;
        }

        mnGet++;
        mpCondGet->Wait(mpMutex);
    }
}

// wait only MSG queue(main queue)
void CQueue::WaitMsg(void *pMsg, AM_UINT msgSize)
{
	AM_ASSERT(IsMain());

	AUTO_LOCK(mpMutex);
	while (1) {
		if (mnData > 0) {
			ReadData(pMsg, msgSize);
			return;
		}

		mnGet++;
		mpCondGet->Wait(mpMutex);
	}
}

AM_ERR CQueue::swicthToNextDataQueue(CQueue* pCurrent)
{
    AM_ASSERT(IsMain());

    if (NULL == pCurrent) {
        AM_ASSERT(NULL == mpCurrentCircularlyQueue);
        pCurrent = mpNextQ;
        if (pCurrent == this || NULL == pCurrent) {
            //AM_ERROR("!!!There's no sub queue(%p)? fatal error here, no-sub queue, must not come here.\n", pCurrent);
            //need return something to notify error?
            return ME_NOT_EXIST;
        }
        mpCurrentCircularlyQueue = pCurrent;
        AM_INFO("first time, choose next queue(%p).\n", pCurrent);
        return ME_OK;
    } else if (this == pCurrent) {
        AM_INFO(" this == pCurrent? would have logical error before.\n");
        mpCurrentCircularlyQueue = mpNextQ;
        return ME_OK;
    }

    AM_ASSERT(pCurrent);
    AM_ASSERT(pCurrent != this);
    pCurrent = pCurrent->mpNextQ;
    AM_ASSERT(pCurrent->mpNextQ);
    if (pCurrent == this) {
        mpCurrentCircularlyQueue = mpNextQ;
        AM_ASSERT(mpCurrentCircularlyQueue != this);
        return ME_OK;
    }
    mpCurrentCircularlyQueue = pCurrent;
    AM_ASSERT(mpCurrentCircularlyQueue != this);
    return ME_OK;
}

//add here?
//except msg, make pins without priority
CQueue::QType CQueue::WaitDataMsgCircularly(void *pMsg, AM_UINT msgSize, WaitResult *pResult)
{
    AM_ERR err;
    AM_ASSERT(IsMain());

    AUTO_LOCK(mpMutex);
    while (1) {
        if (mnData > 0) {
            ReadData(pMsg, msgSize);
            return Q_MSG;
        }

        if(mpNextQ == this){
            mnGet++;
            mpCondGet->Wait(mpMutex);
            continue;
        }
        AM_ASSERT(mpCurrentCircularlyQueue != this);
        if (mpCurrentCircularlyQueue == this || NULL == mpCurrentCircularlyQueue) {
            err = swicthToNextDataQueue(mpCurrentCircularlyQueue);
            //AM_ASSERT(ME_OK == err);
            if (ME_OK != err) {
                AM_ASSERT(ME_NOT_EXIST == err);
                //AM_ERROR("!!!Internal error must not comes here.\n");
                //need return some error code? only mainQ, and no msg
                AM_ASSERT(0);
                return Q_NONE;
            }
        }

        //peek mpCurrentCircularlyQueue first
        AM_ASSERT(mpCurrentCircularlyQueue);
        AM_ASSERT(mpCurrentCircularlyQueue != this);
        if (mpCurrentCircularlyQueue->mnData > 0) {
            pResult->pDataQ = mpCurrentCircularlyQueue;
            pResult->pOwner = mpCurrentCircularlyQueue->mpOwner;
            pResult->blockSize = mpCurrentCircularlyQueue->mBlockSize;

            err = swicthToNextDataQueue(mpCurrentCircularlyQueue);
            AM_ASSERT(ME_OK == err);
            return Q_DATA;
        }else{
            //AM_INFO("Queue Warning Debug: Selected Queue has no Data\n");
        }

        for (CQueue *q = mpCurrentCircularlyQueue->mpNextQ; q != mpCurrentCircularlyQueue; q = q->mpNextQ) {
            if (q->mnData > 0) {
                pResult->pDataQ = q;
                pResult->pOwner = q->mpOwner;
                pResult->blockSize = q->mBlockSize;

                err = swicthToNextDataQueue(q);
                AM_ASSERT(ME_OK == err);
                return Q_DATA;
            }
        }

        mnGet++;
        mpCondGet->Wait(mpMutex);
    }
}

bool CQueue::PeekData(void *pBuffer, AM_UINT size)
{
	AM_ASSERT(IsSub());

	AUTO_LOCK(mpMutex);

	if (mnData > 0) {
		ReadData(pBuffer, size);
		return true;
	}
	return false;
}

CQueue::List* CQueue::AllocNode()
{
	if (mpFreeList) {
		List *pNode = mpFreeList;
		mpFreeList = mpFreeList->pNext;
		pNode->pNext = NULL;
		return pNode;
	}

	//List *pNode = (List*)(new AM_U8[NODE_SIZE]);
	List *pNode = (List*)malloc(NODE_SIZE);
	if (pNode == NULL)
		return NULL;

	pNode->pNext = NULL;
	pNode->bAllocated = true;
    AM_DESTRUCTOR("CQueue0x%p malloc new node %p.\n", this, pNode);
	return pNode;
}

void CQueue::WriteData(List *pNode, const void *pBuffer, AM_UINT size)
{
	Copy(NODE_BUFFER(pNode), pBuffer, AM_MIN(mBlockSize, size));
	mnData++;

#ifdef AM_DEBUG
    AM_ASSERT(mpTail->pNext == NULL);
#endif

	pNode->pNext = NULL;
	mpTail->pNext = pNode;
	mpTail = pNode;
}

void CQueue::ReadData(void *pBuffer, AM_UINT size)
{
	List *pNode = mHead.pNext;
	AM_ASSERT(pNode);

#ifdef AM_DEBUG
    AM_ASSERT(mpTail->pNext == NULL);
#endif

	mHead.pNext = mHead.pNext->pNext;

    //tail is read out, change tail
    if (mHead.pNext == NULL) {
        AM_ASSERT(mpTail == pNode);
        AM_ASSERT(mnData == 1);
        mpTail = (List*)&mHead;
    }

	if (pNode != mpSendBuffer) {
		pNode->pNext = mpFreeList;
		mpFreeList = pNode;
	}

	Copy(pBuffer, NODE_BUFFER(pNode), AM_MIN(mBlockSize, size));
	mnData--;
}

//attach
inline AM_ERR CQueue::AttachBack()
{
    if(mbDetach == false)
        return ME_OK;

    mpMutex = mpMainQ->mpMutex;
    if (mpMutex)
        __LOCK(mpMutex);

    //attach to main-Q
    mpPrevQ->mpNextQ = this;
    mpNextQ->mpPrevQ = this;
    mbDetach = false;

    if (mpMutex)
        __UNLOCK(mpMutex);

    return ME_OK;
}

inline AM_ERR CQueue::AttachNew(CQueue* mainQ)
{
     //AM_INFO("CC4 %p. %p\n", mpMutex, mainQ->mpMutex);

    CMutex* oldMutex = mpMutex;
    AM_ASSERT(oldMutex == NULL);
    if (oldMutex)
        __LOCK(oldMutex);

    //attach to main-Q
    mpPrevQ = mainQ->mpPrevQ;
    mpNextQ = mainQ;
    mpMutex = mainQ->mpMutex;
    mpCondGet = mainQ->mpCondGet;
    mpCondReply = mainQ->mpCondReply;
    mpCondSendMsg = mainQ->mpCondSendMsg;
    mbDetach = false;
    mpMainQ = mainQ;

    //set mainQ side
    if (mpMutex)
        __LOCK(mpMutex);
    mpPrevQ->mpNextQ = this;
    mpNextQ->mpPrevQ = this;
    if (mpMutex)
        __UNLOCK(mpMutex);

     //AM_INFO("CC6\n");

    if (oldMutex)
        __UNLOCK(oldMutex);
    return ME_OK;
}

AM_ERR CQueue::Attach(CQueue* mainQ)
{
    if(mpMainQ == NULL){
        AM_ASSERT(0);
        return ME_OK;
    }
    if(mainQ == mpMainQ)
        mainQ = NULL;
    if(mainQ == NULL)
    {
        AttachBack();
    }else{
        AttachNew(mainQ);
    }
    return ME_OK;
}

AM_ERR CQueue::Detach()
{
    if(mpMainQ == NULL){
        AM_ASSERT(0);
        return ME_OK;
    }
    if (mpMutex)
        __LOCK(mpMutex);

    // detach from main-Q
    mpPrevQ->mpNextQ = mpNextQ;
    mpNextQ->mpPrevQ = mpPrevQ;
    //note your mpPrevQ and mpNextQ still alive
    mbDetach = true;
    //AM_INFO("dEBUG:%p, %p\n", mpMainQ->mpCurrentCircularlyQueue, this);
    if(mpMainQ->mpCurrentCircularlyQueue == this){
        mpMainQ->mpCurrentCircularlyQueue = NULL;
    }

    if (mpMutex)
        __UNLOCK(mpMutex);

    mpMutex = NULL;
    return ME_OK;
}

AM_ERR CQueue::Dump(const char* info)
{
    AM_INFO("%s Call Dump Queue%p:  ", info, this);
    for (CQueue *q = mpNextQ; q != this; q = q->mpNextQ) {
        AM_INFO("A SubQ: %p, Has Data%d         ", q, q->mnData);
	}
    AM_INFO("\nCur Read Queue on Wait:%p", mpCurrentCircularlyQueue);
    AM_INFO("\n");
    return ME_OK;
}
