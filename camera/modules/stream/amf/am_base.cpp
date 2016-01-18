/*******************************************************************************
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
 ******************************************************************************/

#include "string.h"

#include "am_new.h"
#include "am_types.h"
#include "am_if.h"
#include "am_mw.h"
#include "osal.h"
#include "am_queue.h"
#include "msgsys.h"

#include "am_base.h"

//-----------------------------------------------------------------------
//
//  CObject
//
//-----------------------------------------------------------------------
void *CObject::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IInterface) {
    return (IInterface*) this;
  }
  return NULL;
}

void CObject::Delete()
{
  delete this;
}

//-----------------------------------------------------------------------
//
//  CWorkQueue
//
//-----------------------------------------------------------------------
CWorkQueue* CWorkQueue::Create(AO *pAO, bool RTPriority, int priority)
{
  CWorkQueue *result = new CWorkQueue(pAO);
  if (result && result->Construct(RTPriority, priority) != ME_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

void CWorkQueue::Delete()
{
  delete this;
}

AM_ERR CWorkQueue::Construct(bool RTPriority, int priority)
{
  if ((mpMsgQ = CQueue::Create(NULL, this, sizeof(AO::CMD), 0)) == NULL) {
    return ME_NO_MEMORY;
  }

  if ((mpThread = CThread::Create(mpAO->GetName(),
                                  ThreadEntry, this)) == NULL) {
    return ME_OS_ERROR;
  }

  if (RTPriority) {
    if (AM_LIKELY(ME_OK == mpThread->SetRTPriority(priority))) {
      NOTICE("%s Filter is set to real time thread, priority is %u",
             mpAO->GetName(), priority);
    } else {
      ERROR("Failed to set %s filter to real time thread!", mpAO->GetName());
    }
  }

  return ME_OK;
}

CWorkQueue::~CWorkQueue()
{
  if (mpThread) {
    Terminate();
    AM_DELETE(mpThread);
  }
  AM_DELETE(mpMsgQ);
}

AM_ERR CWorkQueue::ThreadEntry(void *p)
{
  ((CWorkQueue*) p)->MainLoop();
  return ME_OK;
}

void CWorkQueue::Terminate()
{
  AM_ENSURE_OK_( SendCmd(AO::CMD_TERMINATE));
}

void CWorkQueue::MainLoop()
{
  AO::CMD cmd;
  while (1) {
    GetCmd(cmd);
    switch (cmd.code) {
      case AO::CMD_TERMINATE:
        CmdAck(ME_OK);
        return;

      case AO::CMD_RUN:
        // CmdAck(ME_OK);
        mpAO->OnRun();
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
//  CBaseEngine
//
//-----------------------------------------------------------------------

CEngineMsgProxy* CEngineMsgProxy::Create(CBaseEngine *pEngine)
{
  CEngineMsgProxy *result = new CEngineMsgProxy(pEngine);
  if (result && result->Construct() != ME_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CEngineMsgProxy::Construct()
{
  mpMsgPort = CMsgPort::Create((IMsgSink*) this, mpEngine->mpMsgSys);
  if (mpMsgPort == NULL) {
    return ME_NO_MEMORY;
  }

  return ME_OK;
}

CEngineMsgProxy::~CEngineMsgProxy()
{
  AM_DELETE(mpMsgPort);
}

void *CEngineMsgProxy::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IMsgSink) {
    return (IMsgSink*) this;
  }
  return inherited::GetInterface(refiid);
}

void CEngineMsgProxy::MsgProc(AM_MSG& msg)
{
  // Proxy is a virtual MsgSink and delegate the work to engine
  mpEngine->OnAppMsg(msg);
}

AM_ERR CBaseEngine::Construct()
{
  if ((mpMsgSys = CMsgSys::Create()) == NULL) {
    return ME_NO_MEMORY;
  }

  if ((mpMutex = CMutex::Create(true)) == NULL) {
    return ME_NO_MEMORY;
  }

  /* Port always uses its container as MsgSink.
   Filter port uses the engine itself as MsgSink	*/
  mpPortFromFilters = CMsgPort::Create(this, mpMsgSys);
  if (mpPortFromFilters == NULL) {
    return ME_NO_MEMORY;
  }

  /* MsgProxy contains another port, which uses the Proxy as MsgSink,
   * which delegates msg proc to engine.
   * Thus, BaseEngine virtually has two ports and two msg proc */
  mpMsgProxy = CEngineMsgProxy::Create(this);
  if (mpMsgProxy == NULL) {
    return ME_NO_MEMORY;
  }

  return ME_OK;
}

CBaseEngine::~CBaseEngine()
{
  AM_DELETE(mpMsgProxy);
  AM_DELETE(mpPortFromFilters);
  AM_DELETE(mpMutex);
  AM_DELETE(mpMsgSys);
}

void *CBaseEngine::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IEngine) {
    return (IEngine*) this;
  }
  if (refiid == IID_IMsgSink) {
    return (IMsgSink*) this;
  }
  if (refiid == IID_IMediaControl) {
    return (IMediaControl*) this;
  }
  return inherited::GetInterface(refiid);
}

AM_ERR CBaseEngine::PostEngineMsg(AM_MSG& msg)
{
//	AUTO_LOCK(mpMutex);
  msg.sessionID = mSessionID;
  /* use filter port so that msg is handled by the engine itself as MsgSink,
   which use MsgProc to process */
  return mpPortFromFilters->PostMsg(msg);
}

AM_ERR CBaseEngine::SetAppMsgSink(IMsgSink *pAppMsgSink)
{
  AUTO_LOCK(mpMutex);
  mpAppMsgSink = pAppMsgSink;
  return ME_OK;
}

AM_ERR CBaseEngine::SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&),
                                      void *context)
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
  /* use the Proxy port so that msg is handled by Proxy as MsgSink,
   which delegate to engine's OnAppMsg to process eventually */
  return mpMsgProxy->MsgPort()->PostMsg(msg);
}

// called by proxy
void CBaseEngine::OnAppMsg(AM_MSG& msg)
{
  AUTO_LOCK(mpMutex);
  if (msg.sessionID == mSessionID) {
    if (mpAppMsgSink) {
      mpAppMsgSink->MsgProc(msg);
    } else if (mAppMsgProc) {
      mAppMsgProc(mAppMsgContext, msg);
    }
  }
}

//-----------------------------------------------------------------------
//
// globals
//
//-----------------------------------------------------------------------
extern AM_ERR AMF_Init()
{
  OSAL_Init();
  return ME_OK;
}

extern void AMF_Terminate()
{
  OSAL_Terminate();
}

