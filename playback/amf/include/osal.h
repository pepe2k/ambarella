/**
 * osal.h
 *
 * History:
 *	2007/11/5 - [Oliver Li] created file
 *	2009/12/2 - [Oliver Li] rewrite
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __OSAL_H__
#define __OSAL_H__

// we need to include these header files in this header file,
// or all users will have to include
// the header files to resolve pthread_mutex_t, etc.
#include "semaphore.h"
#include "pthread.h"

#if PLATFROM_NEED_ATOMICS
int __atomic_inc(am_atomic_t* value);
int __atomic_dec(am_atomic_t* value);
#endif

class CMutex;
class CAutoLock;
class CCondition;
class CEvent;
class CThread;

//-----------------------------------------------------------------------
//
// CMutex
//
//-----------------------------------------------------------------------
class CMutex
{
	friend class CCondition;

public:
	static CMutex* Create(bool bRecursive = true);
	void Delete() { delete this; }

public:
	void Lock() { pthread_mutex_lock(&mMutex); }
	void Unlock() { pthread_mutex_unlock(&mMutex); }

private:
	CMutex() {}
	AM_ERR Construct(bool bRecursive);
	~CMutex() { pthread_mutex_destroy(&mMutex); }

private:
	pthread_mutex_t mMutex;
};

//-----------------------------------------------------------------------
//
// CAutoLock
//
//-----------------------------------------------------------------------
class CAutoLock
{
public:
	CAutoLock(CMutex *pMutex): _pMutex(pMutex)
         {
            if(_pMutex)
                _pMutex->Lock();
         }
	~CAutoLock()
        {
            if(_pMutex)
                _pMutex->Unlock();
        }

private:
	CMutex *_pMutex;
};

#define AUTO_LOCK(pMutex)	CAutoLock __lock__(pMutex)
#define __LOCK(pMutex)		pMutex->Lock()
#define __UNLOCK(pMutex)	pMutex->Unlock()


//-----------------------------------------------------------------------
//
// CCondition
//
//-----------------------------------------------------------------------
class CCondition
{
	friend class CQueue;

public:
	static CCondition* Create() { return new CCondition(); }
	void Delete() { delete this; }

public:
	void Wait(CMutex *pMutex)
	{
		int err = pthread_cond_wait(&mCond, &pMutex->mMutex);
		AM_ASSERT(err == 0);
		err = err;
	}

	void Signal()
	{
		int err = pthread_cond_signal(&mCond);
		AM_ASSERT(err == 0);
		err = err;
	}

	void SignalAll()
	{
		int err = pthread_cond_broadcast(&mCond);
		AM_ASSERT(err == 0);
		err = err;
	}

private:
	CCondition()
	{
		int err = pthread_cond_init(&mCond, NULL);
		AM_ASSERT(err == 0);
		err = err;
	}
	~CCondition() { pthread_cond_destroy(&mCond); }

private:
	pthread_cond_t mCond;
};


//-----------------------------------------------------------------------
//
// CEvent
//
//-----------------------------------------------------------------------
class CEvent
{
public:
	static CEvent* Create();
	void Delete();

public:
	AM_ERR Wait(int ms = -1)
	{
		if (ms < 0) {
			sem_wait(&mEvent);
			return ME_OK;
		}
		else {
			struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += ms / 1000;
			ts.tv_nsec += (ms % 1000) * 1000000;
			return sem_timedwait(&mEvent, &ts) == 0 ?
				ME_OK : ME_ERROR;
		}
	}

	void Signal()
	{
		sem_trywait(&mEvent);
		sem_post(&mEvent);
	}

	void Clear()
	{
		sem_trywait(&mEvent);
	}

private:
	CEvent() {}
	AM_ERR Construct();
	~CEvent() { sem_destroy(&mEvent); }

private:
	sem_t   mEvent;
};

//-----------------------------------------------------------------------
//
// CThread
//
//-----------------------------------------------------------------------
typedef AM_ERR (*AM_THREAD_FUNC)(void*);
class CThread
{
public:
	static CThread* Create(const char *pName, AM_THREAD_FUNC entry, void *pParam);
	void Delete() { delete this; }

public:
	enum {
		PRIO_HIGH,
		PRIO_NORMAL,
		PRIO_LOW,
	};
         AM_ERR SetThreadPrio(AM_INT debug, AM_INT priority);
	static AM_ERR SetRTPriority(int priority = PRIO_NORMAL);
	static CThread* GetCurrent();
	static void DumpStack();
	const char *Name() { return mpName; }

private:
	CThread(const char *pName);
	AM_ERR Construct(AM_THREAD_FUNC entry, void *pParam);
	~CThread();

private:
	static void *__Entry(void*);

private:
	bool		mbThreadCreated;
	pthread_t	mThread;
	const char	*mpName;

	AM_THREAD_FUNC	mEntry;
	void 		*mpParam;
};

extern void OSAL_Init();
extern void OSAL_Terminate();

#endif

