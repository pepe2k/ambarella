/**
 * am_queue.h
 *
 * History:
 *	2007/11/5 - [Oliver Li] created file
 *	2009/12/2 - [Oliver Li] rewrite
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_QUEUE_H__
#define __AM_QUEUE_H__

#if PLATFORM_LINUX
#include <string.h>
#endif

class CMutex;
class CCondition;

class CQueue
{
public:
	struct WaitResult
	{
		CQueue *pDataQ;
		void *pOwner;
		AM_UINT blockSize;
	};

	enum QType
	{
		Q_MSG,
		Q_DATA,
		Q_NONE,
	};

public:
	static CQueue* Create(CQueue *pMainQ, void *pOwner, AM_UINT blockSize, AM_UINT nReservedSlots);
	void Delete();

private:
	CQueue(CQueue *pMainQ, void *pOwner);
	AM_ERR Construct(AM_UINT blockSize, AM_UINT nReservedSlots);
	~CQueue();

public:
	AM_ERR PostMsg(const void *pMsg, AM_UINT msgSize);

	// return receiver's Reply()
	AM_ERR SendMsg(const void *pMsg, AM_UINT msgSize);

	void GetMsg(void *pMsg, AM_UINT msgSize);
	bool PeekMsg(void *pMsg, AM_UINT msgSize);
	void Reply(AM_ERR result);

	bool GetMsgEx(void *pMsg, AM_UINT msgSize);
	void Enable(bool bEnabled = true);

	AM_ERR PutData(const void *pBuffer, AM_UINT size);

	QType WaitDataMsg(void *pMsg, AM_UINT msgSize, WaitResult *pResult);
    QType WaitDataMsgCircularly(void *pMsg, AM_UINT msgSize, WaitResult *pResult);
    QType WaitDataMsgWithSpecifiedQueue(void *pMsg, AM_UINT msgSize, const CQueue *pQueue);
	void WaitMsg(void *pMsg, AM_UINT msgSize);

	bool PeekData(void *pBuffer, AM_UINT size);

	AM_UINT GetDataCnt() const {AUTO_LOCK(mpMutex); return mnData;}

         AM_ERR Attach(CQueue* mainQ = NULL);
         AM_ERR Detach();
         AM_ERR Dump(const char* ch);
private:
    AM_ERR swicthToNextDataQueue(CQueue* pCurrent);
    AM_ERR AttachBack();
    AM_ERR AttachNew(CQueue* mainQ);

public:
	bool IsMain() { return mpMainQ == NULL; }
	bool IsSub() { return mpMainQ != NULL; }

private:
	struct List {
		List *pNext;
		bool bAllocated;
		void Delete();
	};

private:
	void *mpOwner;
	bool mbDisabled;
         bool mbDetach;

	CQueue *mpMainQ;
	CQueue *mpPrevQ;
	CQueue *mpNextQ;

	CMutex *mpMutex;
	CCondition *mpCondReply;
	CCondition *mpCondGet;
	CCondition *mpCondSendMsg;

	AM_UINT mnGet;
	AM_UINT mnSendMsg;

	AM_UINT mBlockSize;
	AM_UINT mnData;

	List *mpTail;
	List *mpFreeList;

	List mHead;

	List *mpSendBuffer;
	AM_U8 *mpReservedMemory;

	AM_ERR *mpMsgResult;

private:
    CQueue *mpCurrentCircularlyQueue;

private:
	static void Copy(void *to, const void *from, AM_UINT bytes)
	{
		if (bytes == sizeof(void*))
			*(void**)to = *(void**)from;
		else
			::memcpy(to, from, bytes);
	}

	List *AllocNode();

	void WriteData(List *pNode, const void *pBuffer, AM_UINT size);
	void ReadData(void *pBuffer, AM_UINT size);

	void WakeupAnyReader();
};

#endif
