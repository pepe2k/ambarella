/**
 * osal_linux.cpp
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
#define LOG_TAG "osal_linux"

#include "am_new.h"
#include "am_types.h"
#include "osal.h"

#ifdef AM_DEBUG
#include <linux/sched.h>
#include <unwind.h>
#include <stdint.h>
#if PLATFORM_LINUX
#include <sys/signal.h>
#endif

static pthread_key_t g_thread_name_key;

//implement simple atomics
#if PLATFROM_NEED_ATOMICS

#if NEW_RTSP_CLIENT
static pthread_spinlock_t g_impl_atomic_mutex;

int __atomic_inc(am_atomic_t* value)
{
    int ret;
    pthread_spin_lock(&g_impl_atomic_mutex);
    ret = (int)(*value);
    *value = ret + 1;
    pthread_spin_unlock(&g_impl_atomic_mutex);
    return ret;
}

int __atomic_dec(am_atomic_t* value)
{
    int ret;
    pthread_spin_lock(&g_impl_atomic_mutex);
    ret = (int)(*value);
    *value = ret -1;
    pthread_spin_unlock(&g_impl_atomic_mutex);
    return ret;
}

#else
static pthread_mutex_t g_impl_atomic_mutex = PTHREAD_MUTEX_INITIALIZER;

int __atomic_inc(am_atomic_t* value)
{
    int ret;
    pthread_mutex_lock(&g_impl_atomic_mutex);
    ret = (int)(*value);
    *value = ret + 1;
    pthread_mutex_unlock(&g_impl_atomic_mutex);
    return ret;
}

int __atomic_dec(am_atomic_t* value)
{
    int ret;
    pthread_mutex_lock(&g_impl_atomic_mutex);
    ret = (int)(*value);
    *value = ret -1;
    pthread_mutex_unlock(&g_impl_atomic_mutex);
    return ret;

}

#endif

#endif


static void osal_thread_name_dtor(void *value)
{
}
#endif

extern void OSAL_Init()
{
#ifdef AM_DEBUG
	pthread_key_create(&g_thread_name_key, osal_thread_name_dtor);
#endif

#if PLATFROM_NEED_ATOMICS
#if NEW_RTSP_CLIENT
    pthread_spin_init(&g_impl_atomic_mutex, PTHREAD_PROCESS_PRIVATE );
#endif
#endif
/*
#if PLATFROM_NEED_ATOMICS
    pthread_mutex_init(&g_impl_atomic_mutex, NULL);
#endif
*/
}

extern void OSAL_Terminate()
{
#if PLATFROM_NEED_ATOMICS
#if NEW_RTSP_CLIENT
    pthread_spin_destroy(&g_impl_atomic_mutex);
#endif
#endif
/*
#if PLATFROM_NEED_ATOMICS
    pthread_mutex_destroy(&g_impl_atomic_mutex);
#endif
*/
#ifdef AM_DEBUG
	pthread_key_delete(g_thread_name_key);
#endif
}

//-----------------------------------------------------------------------
//
// CMutex
//
//-----------------------------------------------------------------------
CMutex* CMutex::Create(bool bRecursive)
{
	CMutex *result = new CMutex;
	if (result && result->Construct(bRecursive) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CMutex::Construct(bool bRecursive)
{
	if (bRecursive) {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&mMutex, &attr);
	}
	else {
		// a littler faster than recursive
		pthread_mutex_init(&mMutex, NULL);
	}
	return ME_OK;
}

//-----------------------------------------------------------------------
//
// CEvent
//
//-----------------------------------------------------------------------
CEvent* CEvent::Create()
{
	CEvent* result = new CEvent;
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CEvent::Construct()
{
	sem_init(&mEvent, 0, 0);
	return ME_OK;
}

void CEvent::Delete()
{
	delete this;
}

//-----------------------------------------------------------------------
//
// CThread
//
//-----------------------------------------------------------------------
CThread* CThread::Create(const char *pName, AM_THREAD_FUNC entry, void *pParam)
{
	CThread *result = new CThread(pName);
	if (result && result->Construct(entry, pParam) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

CThread::CThread(const char *pName):
	mbThreadCreated(false),
	mpName(pName)
{
}


AM_ERR CThread::Construct(AM_THREAD_FUNC entry, void *pParam)
{
	mEntry = entry;
	mpParam = pParam;
#if 1
       pthread_attr_t attr;
       struct sched_param param;
       pthread_attr_init(&attr);
       pthread_attr_setschedpolicy(&attr, SCHED_RR);
       param.sched_priority = 80;  //todo, hardcode now
       pthread_attr_setschedparam(&attr, &param);


       int err = pthread_create(&mThread, &attr, __Entry, (void*)this);
	if (err) {
	    pthread_attr_destroy(&attr);
	    return ME_OS_ERROR;
	}
	pthread_attr_destroy(&attr);
#else
	int err = pthread_create(&mThread, NULL, __Entry, (void*)this);
	if (err) return ME_OS_ERROR;
#endif
	AM_PRINTF("thread %s created\n", mpName);
	if(mpName) pthread_setname_np(mThread, mpName);
	mbThreadCreated = true;
	return ME_OK;
}

CThread::~CThread()
{
	if (mbThreadCreated) {
		pthread_join(mThread, NULL);
	}
}

#ifdef AM_DEBUG

typedef struct {
	size_t count;
	intptr_t *addrs;
} stack_state_t;

static _Unwind_Reason_Code trace_function(_Unwind_Context *context, void *arg)
{
	stack_state_t *state = (stack_state_t*)arg;
	if (state->count) {
		intptr_t ip = (intptr_t)_Unwind_GetIP(context);
		if (ip) {
			state->addrs[0] = ip;
			state->addrs++;
			state->count--;
			return _URC_NO_REASON;
		}
	}
	return _URC_END_OF_STACK;
}

static int get_backtrace(intptr_t *addrs, size_t max_entries)
{
	stack_state_t state;
	state.count = max_entries;
	state.addrs = (intptr_t*)addrs;
	_Unwind_Backtrace(trace_function, (void*)&state);
	return max_entries - state.count;
}

void dump_stack_trace()
{
	intptr_t addrs[20];
	char c = get_backtrace(addrs, 20);
	char buf[16];
	char tmp[16*20];
	int i;

	tmp[0] = 0;
        AM_VERBOSE("tmp[0]=%d\n", tmp[0]);
	for (i = 0; i < c; i++) {
		snprintf(buf, sizeof buf, "%2d: %08x\n", i, addrs[i]);
		AM_WARNING("%s", buf);
	}
}

static void segfault_handler(int signo)
{
	CThread *pthis = (CThread*)pthread_getspecific(g_thread_name_key);
	LOGE("!!!thread %s caused a SEG-Fault!!!\n", pthis->Name());
	AM_WARNING("thread %s caused a segmentation fault\n", pthis->Name());
	AM_WARNING("stack trace\n");
	AM_WARNING("-----------\n");
	dump_stack_trace();
	AM_WARNING("-----------\n");
	pthread_exit((void*)5);
}
#endif

void *CThread::__Entry(void *p)
{
	CThread *pthis = (CThread *)p;

#ifdef AM_DEBUG
	pthread_setspecific(g_thread_name_key, pthis);

if (g_GlobalCfg.mUseNativeSEGFaultHandler) {
	//android logcat need SIGSEGV signal
	signal(SIGSEGV, segfault_handler);
}

#endif

	pthis->mEntry(pthis->mpParam);

	AM_PRINTF("thread %s exits\n", pthis->mpName);
	return NULL;
}

CThread* CThread::GetCurrent()
{
#ifdef AM_DEBUG
	return (CThread*)pthread_getspecific(g_thread_name_key);
#else
	return NULL;
#endif
}

void CThread::DumpStack()
{
#ifdef AM_DEBUG
	dump_stack_trace();
#endif
}

AM_ERR CThread::SetRTPriority(int priority)
{
	struct sched_param param;
	param.sched_priority = 99;
	if (sched_setscheduler(0, SCHED_FIFO, &param) < 0)
		AM_PERROR("sched_setscheduler");
	return ME_OK;
}

AM_ERR CThread::SetThreadPrio(AM_INT debug, AM_INT priority)
{
    //return ME_OK;//DEBUG
    AM_INFO("Cur thread %s..", mpName);
    const char* string[4];
    string[SCHED_FIFO] = "SCHED_FIFO";
    string[SCHED_RR] = "SCHED_RR";
    string[SCHED_OTHER] = "SCHED_OTHER";
    string[SCHED_BATCH] = "SCHED_BATCH";


    AM_INT curPolicy;
    struct sched_param curParam;
    if (pthread_getschedparam(mThread, &curPolicy, &curParam) < 0){
        AM_PERROR("SetThreadPrio");
        return ME_ERROR;
    }
    if(curPolicy != SCHED_FIFO && curPolicy !=  SCHED_RR && curPolicy !=  SCHED_OTHER && curPolicy !=  SCHED_BATCH)
        AM_ERROR("==>%d", curPolicy);

    AM_INFO("Policy::%s,  priority:%d\n", string[curPolicy], curParam.sched_priority);

    AM_INT setPolicy;
    struct sched_param setParam;
    setParam.sched_priority = priority;
    if(debug == 1)
        setPolicy = SCHED_RR;
    else if(debug ==3)
        setPolicy = SCHED_FIFO;
    else
        setPolicy = SCHED_OTHER;

    if (pthread_setschedparam(mThread, setPolicy, &setParam) < 0)
        AM_PERROR("SetThreadPrio");

    AM_INFO("Cur thread %s.. AFTER SET\n", mpName);

        if (pthread_getschedparam(mThread, &curPolicy, &curParam) < 0){
        AM_PERROR("SetThreadPrio");
        return ME_ERROR;
    }
    if(curPolicy != SCHED_FIFO && curPolicy !=  SCHED_RR && curPolicy !=  SCHED_OTHER && curPolicy !=  SCHED_BATCH)
        AM_ERROR("==>%d", curPolicy);

    AM_INFO("Policy::%s,  priority:%d\n", string[curPolicy], curParam.sched_priority);

    return ME_OK;
}

