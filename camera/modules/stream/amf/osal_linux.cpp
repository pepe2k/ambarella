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

#include <signal.h>
#include "am_new.h"
#include "am_types.h"
#include "osal.h"

#include "unistd.h"
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)

#ifdef AM_DEBUG

#include <unwind.h>
#include <stdint.h>

static pthread_key_t g_thread_name_key;
static void osal_thread_name_dtor(void *value)
{
}
#endif

extern void OSAL_Init()
{
#ifdef AM_DEBUG
  pthread_key_create(&g_thread_name_key, osal_thread_name_dtor);
#endif
}

extern void OSAL_Terminate()
{
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
  CMutex *result = new CMutex();
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
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    ::pthread_mutex_init(&mMutex, &attr);
  } else {
    // a littler faster than recursive
    ::pthread_mutex_init(&mMutex, NULL);
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
  CEvent* result = new CEvent();
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

CThread::CThread(const char *pName) :
    mbThreadCreated(false),
    mbThreadRunning(false),
    mThread(0),
    mpName(pName),
    mEntry(NULL),
    mpParam(NULL)
{
}

AM_ERR CThread::Construct(AM_THREAD_FUNC entry, void *pParam)
{
  mEntry = entry;
  mpParam = pParam;
  mbThreadRunning = false;

  if (AM_UNLIKELY(0 != pthread_create(&mThread,
                                      NULL,
                                      __Entry,
                                      (void*) this))) {
    return ME_OS_ERROR;
  }
  if (AM_UNLIKELY(pthread_setname_np(mThread, mpName) < 0)) {
    PERROR("Failed to set thread name:");
  }

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
}stack_state_t;

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
  //char tmp[16*20];
  int i;

  //tmp[0] = 0;
  for (i = 0; i < c; i++) {
    snprintf(buf, sizeof buf, "%2d: %08x\n", i, addrs[i]);
    printf(buf);
  }
}

static void segfault_handler(int signo)
{
  CThread *pthis = (CThread*)pthread_getspecific(g_thread_name_key);
  DEBUG("thread %s caused a segmentation fault\n", pthis->Name());
  DEBUG("stack trace\n");
  DEBUG("-----------\n");
  dump_stack_trace();
  DEBUG("-----------\n");
  pthread_exit((void*)5);
}
#endif

void *CThread::__Entry(void *p)
{
  CThread *pthis = (CThread *) p;

#ifdef AM_DEBUG
  pthread_setspecific(g_thread_name_key, pthis);
  signal(SIGSEGV, segfault_handler);
#endif

  INFO("thread %s created, tid %ld\n", pthis->mpName, gettid());

  pthis->mbThreadRunning = true;
  pthis->mEntry(pthis->mpParam);
  pthis->mbThreadRunning = false;

  INFO("thread %s exits\n", pthis->mpName);
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
  AM_ERR ret = ME_OK;
  struct sched_param param;
  param.sched_priority = priority;
#if 0
  if (sched_setscheduler(0, SCHED_FIFO, &param) < 0)
  AM_PERROR("sched_setscheduler");
#else
  if (pthread_setschedparam(mThread, SCHED_RR, &param) < 0) {
    PERROR("sched_setscheduler");
    ret = ME_ERROR;
  }
#endif
  return ret;
}

