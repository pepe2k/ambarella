/*******************************************************************************
 * sei_sender.c
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

#include "sei_sender_if.h"
#include "sei_sender.h"

#define HW_TIMER     ((const char*)"/proc/ambarella/ambarella_hwtimer")

IPacketFilter* create_sei_sender_filter(IEngine *pEngine)
{
  return CSeiSender::Create(pEngine);
}

IPacketFilter* CSeiSender::Create(IEngine *pEngine)
{
  CSeiSender *result = new CSeiSender(pEngine);
  if (result && result->Construct() != ME_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CSeiSender::Construct()
{
  AM_ERR err = inherited::Construct();
  if (err != ME_OK){
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

  mpBufferPool = CFixedPacketPool::Create("SeiSenderBufPool",
                                          BUFFER_POOL_SIZE,
                                          MAX_SEI_NALU_SIZE);
  if (mpBufferPool == NULL) {
    return ME_NO_MEMORY;
  }

  mpOutPin = CSeiSenderOutput::Create(this);
  if (mpOutPin == NULL) {
    return ME_NO_MEMORY;
  }

  mpOutPin->SetBufferPool(mpBufferPool);

  return ME_OK;
}

CSeiSender::~CSeiSender()
{
  if (AM_LIKELY(mHwTimerFd > 0)) {
    close(mHwTimerFd);
    mHwTimerFd = -1;
  }

  AM_DELETE(mpMutex);
  AM_DELETE(mpOutPin);
  AM_RELEASE(mpBufferPool);
}

void CSeiSender::GetInfo(INFO& info)
{
  info.nInput = 0;
  info.nOutput = 0;
  info.pName = "SeiSender";
}

IPacketPin* CSeiSender::GetOutputPin(AM_UINT index)
{
  if (index == 0){
    return mpOutPin;
  }

  return NULL;
}

AM_ERR CSeiSender::Run()
{
  mRun = true;
  return ME_OK;
}

AM_ERR CSeiSender::Start()
{
  AUTO_LOCK(mpMutex);
  mRun = true;
  return ME_OK;
}

AM_ERR CSeiSender::Stop()
{
  AUTO_LOCK(mpMutex);
  mRun = false;
  return ME_OK;
}

inline AM_PTS CSeiSender::GetCurrentPts()
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

AM_ERR CSeiSender::SendSEI(void *pUserData, AM_U32 len)
{
  static const AM_U32 seiStartCode = 0x00000001; // Little Eddian
  AM_U8 UUID[UUID_SIZE] = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0x50, 0xbe,
                           0xe1, 0x11, 0x9b, 0x11, 0x30, 0x93, 0xfe, 0x7b};
  CPacket *pOutBuffer;

  AUTO_LOCK(mpMutex);

  if (!mRun){
    ERROR("Current state is not runing");
    return ME_ERROR;
  }

  if (!mpOutPin->AllocBuffer(pOutBuffer)){
    ERROR("Alloc buffer failed!\n");
    return ME_ERROR;
  }

  pOutBuffer->SetType(CPacket::AM_PAYLOAD_TYPE_DATA);
  pOutBuffer->SetAttr(CPacket::AM_PAYLOAD_ATTR_SEI);

  AM_U8 *pBufPtr = pOutBuffer->GetDataPtr();
  *(AM_U32 *)pBufPtr = seiStartCode;
  pBufPtr += sizeof(seiStartCode);

  *pBufPtr++ = 0x06;        // set SEI Nal Header
  *pBufPtr++ = 0x05;        // set SEI payload type: user data unregistered
  *pBufPtr++ = (len + 16);  // set SEI payload size

  memcpy(pBufPtr, UUID, UUID_SIZE);
  pBufPtr += UUID_SIZE;
  memcpy(pBufPtr, pUserData, len);

  pOutBuffer->SetDataSize(4 + 1 + 1 + 1+ 16 + len);
  pOutBuffer->SetPTS(GetCurrentPts());
  mpOutPin->SendBuffer(pOutBuffer);
  return ME_OK;
}

//-----------------------------------------------------------------------
//
// CSeiSenderOutput
//
//-----------------------------------------------------------------------
CSeiSenderOutput* CSeiSenderOutput::Create(CPacketFilter *pFilter)
{
  CSeiSenderOutput *result = new CSeiSenderOutput(pFilter);
  if (result && result->Construct() != ME_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CSeiSenderOutput::Construct()
{
  return ME_OK;
}

