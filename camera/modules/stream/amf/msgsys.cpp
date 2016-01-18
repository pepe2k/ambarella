/*******************************************************************************
 * msgsys.cpp
 *
 * History:
 *    2009/12/8 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 ******************************************************************************/

#include <string.h>
#include "am_new.h"
#include "am_types.h"
#include "osal.h"
#include "am_queue.h"
#include "am_if.h"
#include "msgsys.h"

//-----------------------------------------------------------------------
//
// CMsgPort
//
//-----------------------------------------------------------------------
CMsgPort* CMsgPort::Create(IMsgSink *pMsgSink, CMsgSys *pMsgSys)
{
  // attach MsgSink to this MsgPort
  CMsgPort *result = new CMsgPort(pMsgSink);
  if (result && result->Construct(pMsgSys) != ME_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CMsgPort::Construct(CMsgSys *pMsgSys)
{
  // Port always attaches to MsgSys by attaching its Q to MsgSys mainQ
  if ((mpQ = CQueue::Create(pMsgSys->mpMainQ,
                            this,
                            sizeof(AM_MSG),
                            1)) == NULL) {
    return ME_NO_MEMORY;
  }

  return ME_OK;
}

CMsgPort::~CMsgPort()
{
  // todo
  AM_DELETE(mpQ);
}

void *CMsgPort::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IMsgPort)
    return (IMsgPort*) this;
  if (refiid == IID_IInterface)
    return (IInterface*) this;
  return NULL;
}

void CMsgPort::Delete()
{
  // todo
  delete this;
}

AM_ERR CMsgPort::PostMsg(AM_MSG& msg)
{
  return mpQ->PutData(&msg, sizeof(msg));
}

//-----------------------------------------------------------------------
//
// CMsgSys
//
//-----------------------------------------------------------------------
CMsgSys* CMsgSys::Create()
{
  CMsgSys* result = new CMsgSys();
  if (result && (ME_OK != result->Construct())) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CMsgSys::Construct()
{
  if ((mpMainQ = CQueue::Create(NULL, this, sizeof(AM_UINT), 1)) == NULL) {
    return ME_OS_ERROR;
  }

  if ((mpThread = CThread::Create("AMFMsgSys", ThreadEntry, this)) == NULL) {
    return ME_OS_ERROR;
  }

  return ME_OK;
}

CMsgSys::~CMsgSys()
{
  // stop messaging thread
  // todo - if this is called from our thread...

  AM_UINT msg = 0;
  AM_ENSURE_OK_( mpMainQ->SendMsg(&msg, sizeof(msg)));

  AM_DELETE(mpThread);

  // todo - delete all sub-Qs
  AM_DELETE(mpMainQ);
}

void CMsgSys::Delete()
{
  delete this;
}

AM_ERR CMsgSys::ThreadEntry(void *p)
{
  return ((CMsgSys*) p)->MainLoop();
}

AM_ERR CMsgSys::MainLoop()
{
  AM_MSG msg;

  while (true) {
    AM_UINT dummy;
    CQueue::QType qtype;
    CQueue::WaitResult result;

    qtype = mpMainQ->WaitDataMsg(&dummy, sizeof(dummy), &result);
    if (qtype == CQueue::Q_MSG) {
      mpMainQ->Reply(ME_OK);
      break;
    }

    if (qtype == CQueue::Q_DATA) {
      if (result.pDataQ->PeekData(&msg, sizeof(msg))) {
        // retrieve MsgSink from the msg owner, which is a MsgPort
        IMsgSink *pMsgSink = ((CMsgPort*) result.pOwner)->mpMsgSink;
        if (pMsgSink) {
          pMsgSink->MsgProc(msg);
        }
      }
    }
  }

  return ME_OK;
}

