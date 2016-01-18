/*******************************************************************************
 * event_sender.c
 *
 * History:
 *    2012/11/20 - [Dongge Wu] create file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ******************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "am_include.h"
#include "am_data.h"

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_media_info.h"

#include "event_sender_if.h"
#include "event_sender.h"

#define HW_TIMER     ((const char*)"/proc/ambarella/ambarella_hwtimer")

IPacketFilter* create_event_sender_filter(IEngine *pEngine)
{
  return CEventSender::Create(pEngine);
}

IPacketFilter* CEventSender::Create(IEngine *pEngine)
{
  CEventSender *result = new CEventSender(pEngine);
  if (result && result->Construct() != ME_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CEventSender::Construct()
{
  AM_ERR err = inherited::Construct();
  if (err != ME_OK) {
    return err;
  }

  if (AM_LIKELY(mHwTimerFd < 0)) {
    if (AM_UNLIKELY((mHwTimerFd = open(HW_TIMER, O_RDONLY)) < 0)) {
      return ME_ERROR;
    }
  }
  if ((mpMutex = CMutex::Create(false)) == NULL) {
    return ME_OS_ERROR;
  }

  mpBufferPool = CFixedPacketPool::Create("EventSenderBufPool",
                                          BUFFER_POOL_SIZE,
                                          MAX_EVENT_NALU_SIZE);
  if (mpBufferPool == NULL) {
    return ME_NO_MEMORY;
  }

  mpOutPin = CEventSenderOutput::Create(this);
  if (mpOutPin == NULL) {
    return ME_NO_MEMORY;
  }

  mpOutPin->SetBufferPool(mpBufferPool);

  return ME_OK;
}

CEventSender::~CEventSender()
{
  if (AM_LIKELY(mHwTimerFd > 0)) {
    close(mHwTimerFd);
    mHwTimerFd = -1;
  }

  AM_DELETE(mpMutex);
  AM_DELETE(mpOutPin);
  AM_RELEASE(mpBufferPool);
}

void CEventSender::GetInfo(INFO& info)
{
  info.nInput = 0;
  info.nOutput = 0;
  info.pName = "EventSender";
}

IPacketPin* CEventSender::GetOutputPin(AM_UINT index)
{
  if (index == 0) {
    return mpOutPin;
  }
  return NULL;
}

AM_ERR CEventSender::Run()
{
  mRun = true;
  return ME_OK;
}

AM_ERR CEventSender::Start()
{
  AUTO_LOCK(mpMutex);
  mRun = true;
  return ME_OK;
}

AM_ERR CEventSender::Stop()
{
  AUTO_LOCK(mpMutex);
  mRun = false;
  return ME_OK;
}

inline AM_PTS CEventSender::GetCurrentPts()
{
  AM_U8 pts[32] = {0};
  AM_PTS currPts = mLastPts;
  if (AM_LIKELY(mHwTimerFd >= 0)) {
    if (AM_UNLIKELY(read(mHwTimerFd, pts, sizeof(pts)) < 0)) {
      PERROR("read");
    } else {
      currPts = strtoull((const char*)pts, (char **)NULL, 10);
      mLastPts = currPts;
    }
  }
  return currPts;
}

AM_ERR CEventSender::SendEvent(CPacket::AmPayloadAttr eventType)
{
  CPacket *pOutBuffer;

  AUTO_LOCK(mpMutex);

  if (!mRun) {
    return ME_ERROR;
  }

  if (!mpOutPin->AllocBuffer(pOutBuffer)) {
    return ME_ERROR;
  }

  pOutBuffer->SetType(CPacket::AM_PAYLOAD_TYPE_EVENT);
  pOutBuffer->SetAttr(eventType);
  pOutBuffer->SetDataSize(0);
  pOutBuffer->SetPTS(GetCurrentPts());
  mpOutPin->SendBuffer(pOutBuffer);
  return ME_OK;
}

//-----------------------------------------------------------------------
//
// CEventSenderOutput
//
//-----------------------------------------------------------------------
CEventSenderOutput* CEventSenderOutput::Create(CPacketFilter *pFilter)
{
  CEventSenderOutput *result = new CEventSenderOutput(pFilter);
  if (result && result->Construct() != ME_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CEventSenderOutput::Construct()
{
  return ME_OK;
}

