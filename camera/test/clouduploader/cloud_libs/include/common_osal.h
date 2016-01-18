/**
 * common_osal.h
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
 *
 * Copyright (C) 2012, the ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the ambarella Inc.
 */

#ifndef __COMMON_OSAL_H__
#define __COMMON_OSAL_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CIMutex
//
//-----------------------------------------------------------------------
class CIMutex
{
    friend class CICondition;

public:
    static CIMutex* Create(bool bRecursive = true);
    void Delete();

public:
    void Lock();
    void Unlock();

private:
    CIMutex();
    EECode Construct(bool bRecursive);
    ~CIMutex();

private:
    pthread_mutex_t mMutex;
};

//-----------------------------------------------------------------------
//
// CICondition
//
//-----------------------------------------------------------------------
class CICondition
{
public:
    static CICondition* Create();
    void Delete();

public:
    void Wait(CIMutex *pMutex);
    void Signal();
    void SignalAll();

private:
    CICondition();
    ~CICondition();
    EECode Construct();

private:
    pthread_cond_t mCond;
};

//-----------------------------------------------------------------------
//
// CIAutoLock
//
//-----------------------------------------------------------------------
class CIAutoLock
{
public:
    CIAutoLock(CIMutex *pMutex);
    ~CIAutoLock();

private:
    CIMutex *_pMutex;
};

#define AUTO_LOCK(pMutex)	CIAutoLock __lock__(pMutex)
#define __LOCK(pMutex)		pMutex->Lock()
#define __UNLOCK(pMutex)	pMutex->Unlock()

//-----------------------------------------------------------------------
//
// CIEvent
//
//-----------------------------------------------------------------------
class CIEvent
{
public:
    static CIEvent* Create();
    void Delete();

public:
    EECode Wait(TInt ms = -1);
    void Signal();
    void Clear();

private:
    CIEvent();
    EECode Construct();
    ~CIEvent();

private:
    sem_t   mEvent;
};

//-----------------------------------------------------------------------
//
// CIThread
//
//-----------------------------------------------------------------------
enum {
    ESchedulePolicy_Other = 0,
    ESchedulePolicy_FIFO,
    ESchedulePolicy_RunRobin,
};

typedef EECode (*TF_THREAD_FUNC)(void*);
class CIThread
{
public:
    static CIThread* Create(const char *pName, TF_THREAD_FUNC entry, void *pParam, TUint schedule_policy = ESchedulePolicy_Other, TUint priority = 0, TUint affinity = 0);
    void Delete();

public:
    static void DumpStack();
    const char *Name() { return mpName;}

private:
    CIThread(const char *pName);
    EECode Construct(TF_THREAD_FUNC entry, void *pParam, TUint schedule_policy = ESchedulePolicy_Other, TUint priority = 0, TUint affinity = 0);
    ~CIThread();

private:
    static void *__Entry(void*);

private:
    TU8 mbThreadCreated, mReserved0, mReserved1, mReserved2;
    pthread_t   mThread;
    const char  *mpName;

    TF_THREAD_FUNC  mEntry;
    void    *mpParam;
};

//extern CIMutex* gpLogMutex;

extern void OSAL_Init();
extern void OSAL_Terminate();

extern TInt __atomic_inc(TAtomic* value);
extern TInt __atomic_dec(TAtomic* value);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

