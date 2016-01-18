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

#include "am_new.h"
#include "am_types.h"
#include "osal.h"

#include "am_queue.h"

#define NODE_SIZE		(sizeof(List) + mBlockSize)
#define NODE_BUFFER(_pNode)	((AM_U8*)_pNode + sizeof(List))

// blockSize - bytes of each queue item
// nReservedSlots - number of reserved nodes
CQueue* CQueue::Create(CQueue *pMainQ,
                       void *pOwner,
                       AM_UINT blockSize,
                       AM_UINT nReservedSlots)
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

CQueue::CQueue(CQueue *pMainQ, void *pOwner) :
        mpOwner(pOwner),
        mbDisabled(false),
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
        mpHead(NULL),
        mpTail((List*) &mpHead),
        mpFreeList(NULL),
        mpSendBuffer(NULL),
        mpReservedMemory(NULL),
        mpMsgResult(NULL)
{
}

AM_ERR CQueue::Construct(AM_UINT blockSize, AM_UINT nReservedSlots)
{
  mBlockSize = ROUND_UP(blockSize, 4);

  // mpSendBuffer + list_node + list_node + ... + list_node
  mpReservedMemory = new AM_U8[NODE_SIZE * (nReservedSlots + 1)];
  if (mpReservedMemory == NULL) {
    return ME_NO_MEMORY;
  }

  // for SendMsg()
  mpSendBuffer = (List*) mpReservedMemory;
  mpSendBuffer->pNext = NULL;
  mpSendBuffer->bAllocated = false;

  // reserved nodes, keep in free-list
  List *pNode = (List*) (mpReservedMemory + NODE_SIZE);
  for (; nReservedSlots > 0; nReservedSlots --) {
    pNode->bAllocated = false;
    pNode->pNext = mpFreeList;
    mpFreeList = pNode;
    pNode = (List*) ((AM_U8*) pNode + NODE_SIZE);
  }

  if (IsMain()) {
    if ((mpMutex = CMutex::Create(false)) == NULL) {
      return ME_OS_ERROR;
    }

    if ((mpCondGet = CCondition::Create()) == NULL) {
      return ME_OS_ERROR;
    }

    if ((mpCondReply = CCondition::Create()) == NULL) {
      return ME_OS_ERROR;
    }

    if ((mpCondSendMsg = CCondition::Create()) == NULL) {
      return ME_OS_ERROR;
    }
  } else {
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
  if (mpMutex) {
    __LOCK(mpMutex);
  }

  AM_ASSERT(mnGet == 0);
  AM_ASSERT(mnSendMsg == 0);

  if (IsSub()) {
    // detach from main-Q
    mpPrevQ->mpNextQ = mpNextQ;
    mpNextQ->mpPrevQ = mpPrevQ;
  } else {
    // all sub-Qs should be removed
    AM_ASSERT(mpPrevQ == this);
    AM_ASSERT(mpNextQ == this);
    AM_ASSERT(mpMsgResult == NULL);
  }

  mpHead->Delete();
  mpFreeList->Delete();

  delete[] mpReservedMemory;

  if (mpMutex) {
    __UNLOCK(mpMutex);
  }

  if (IsMain()) {
    AM_DELETE(mpCondSendMsg);
    AM_DELETE(mpCondReply);
    AM_DELETE(mpCondGet);
    AM_DELETE(mpMutex);
  }
}

void CQueue::List::Delete()
{
  List *pNode = this;
  while (pNode) {
    List *pNext = pNode->pNext;
    if (pNode->bAllocated) {
      delete[] (AM_U8*) pNode;
    }
    pNode = pNext;
  }
}

AM_ERR CQueue::PostMsg(const void *pMsg, AM_UINT msgSize)
{
  AM_ASSERT(IsMain());
  AUTO_LOCK(mpMutex);

  List *pNode = AllocNode();
  if (pNode == NULL) {
    return ME_NO_MEMORY;
  }

  WriteData(pNode, pMsg, msgSize);

  if (mnGet > 0) {
    mnGet --;
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
        mnGet --;
        mpCondGet->Signal();
      }

      AM_ERR result;
      mpMsgResult = &result;
      mpCondReply->Wait(mpMutex);
      mpMsgResult = NULL;

      if (mnSendMsg > 0) {
        mnSendMsg --;
        mpCondSendMsg->Signal();
      }

      return result;
    }

    mnSendMsg ++;
    mpCondSendMsg->Wait(mpMutex);
  }
  return ME_ERROR;
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

    mnGet ++;
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

    mnGet ++;
    mpCondGet->Wait(mpMutex);
  }

  return false;
}

void CQueue::Enable(bool bEnabled)
{
//	AM_ASSERT(IsMain());

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
  if (mpMainQ->mnGet > 0) {
    mpMainQ->mnGet --;
    mpMainQ->mpCondGet->Signal();
  }

  return ME_OK;
}

// wait this main-Q and all its sub-Qs
CQueue::QType CQueue::WaitDataMsg(void *pMsg,
                                  AM_UINT msgSize,
                                  WaitResult *pResult)
{
  AM_ASSERT(IsMain());

  AUTO_LOCK(mpMutex);
  while (1) {
    if (mnData > 0) {
      ReadData(pMsg, msgSize);
      return Q_MSG;
    }

    for (CQueue *q = mpNextQ; q != this; q = q->mpNextQ) {
      if (q->mnData > 0 && !q->mbDisabled) {
        pResult->pDataQ = q;
        pResult->pOwner = q->mpOwner;
        pResult->blockSize = q->mBlockSize;

        /* move the hit Q to the tail, semi Round Robin
         if (q != mpPrevQ) {		// if q is not the tail
         q->mpPrevQ->mpNextQ = q->mpNextQ;
         q->mpNextQ->mpPrevQ = q->mpPrevQ;
         mpPrevQ->mpNextQ = q;
         q->mpPrevQ = mpPrevQ;
         mpPrevQ = q;
         q->mpNextQ = this;
         }	*/
        return Q_DATA;
      }
    }

    mnGet ++;
    mpCondGet->Wait(mpMutex);
  }

  return Q_MSG; // Add this line to shut up code analysis warning
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

  List *pNode = (List*) (new AM_U8[NODE_SIZE]);
  if (pNode == NULL)
    return NULL;

  pNode->pNext = NULL;
  pNode->bAllocated = true;

  return pNode;
}

void CQueue::WriteData(List *pNode, const void *pBuffer, AM_UINT size)
{
  Copy(NODE_BUFFER(pNode), pBuffer, AM_MIN(mBlockSize, size));
  mnData ++;

  pNode->pNext = NULL;
  mpTail->pNext = pNode;
  mpTail = pNode;
}

void CQueue::ReadData(void *pBuffer, AM_UINT size)
{
  List *pNode = mpHead;
  AM_ASSERT(pNode);

  mpHead = mpHead->pNext;
  if (mpHead == NULL)
    mpTail = (List*) &mpHead;

  if (pNode != mpSendBuffer) {
    pNode->pNext = mpFreeList;
    mpFreeList = pNode;
  }

  Copy(pBuffer, NODE_BUFFER(pNode), AM_MIN(mBlockSize, size));
  mnData --;
}

