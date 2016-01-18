/*
 * buffer_builder.cpp
 *
 * History:
 *    2012/8/10 - [Shupeng Ren] created file
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <stdio.h>
#include <string.h>

#include "am_new.h"
#include "am_types.h"
#include "am_if.h"
#include "osal.h"
#include "am_queue.h"
#include "msgsys.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_base.h"
#include "am_base_packet.h"

#include "av_queue_builder.h"

CRingPacketPool* CRingPacketPool::Create(AM_UINT count, AM_UINT dataSize)
{
  CRingPacketPool* result = new CRingPacketPool();
  if (AM_UNLIKELY(result && (result->Construct(count, dataSize) != ME_OK))) {
    delete result;
    result = NULL;
  }

  return result;
}

AM_ERR CRingPacketPool::Construct(AM_UINT count, AM_UINT dataSize)
{
  AM_ERR err = ME_ERROR;
  if (count > 0 && dataSize > 0) {
    mpPayloadPool = new ExPayload[count];
    dataSize = ROUND_UP(dataSize, 4);
    mMaxDataSize = dataSize;
    mpRingBufferMem = new AM_U8[dataSize * (count+2)];
    if (AM_UNLIKELY((mpPayloadPool == NULL)) || (mpRingBufferMem == NULL)) {
      err = ME_NO_MEMORY;
      delete[] mpPayloadPool;
      delete[] mpRingBufferMem;
    } else {
      mpCurrent = mpRingBufferMem;
      mpRingBufferMemEnd = mpRingBufferMem + (count+2) * dataSize;
      mPayloadCount = count;
      mFreeMem = count * dataSize;
      err = ME_OK;
    }
    if ((mpMutex = CMutex::Create(false)) == NULL) {
      err =  ME_OS_ERROR;
    }
  }
  return err;
}

ExPayload* CRingPacketPool::GetAVQPayload(bool isEvent)
{
  //AUTO_LOCK(mpMutex);
  return (isEvent ? (&mpPayloadPool[mReadPos_e]) : (&mpPayloadPool[mReadPos]));
}

void CRingPacketPool::AVQPayloadAdvance(bool isEvent)
{
  //AUTO_LOCK(mpMutex);
  if (!isEvent) {
    __atomic_inc(&(mpPayloadPool[mReadPos].mRefCount));
    mpPayloadPool[mReadPos].mIsNormalUse = true;
    mReadPos = (mReadPos + 1) % mPayloadCount;
    __atomic_dec((am_atomic_t*)&mReadablePayloadCount);
    __atomic_inc((am_atomic_t*)&mInUsedPayload);
  } else {
    __atomic_inc(&(mpPayloadPool[mReadPos_e].mRefCount));
    mpPayloadPool[mReadPos_e].mIsEventUse = true;
    mReadPos_e = (mReadPos_e + 1) % mPayloadCount;
    __atomic_dec((am_atomic_t*)&mReadablePayloadCount_e);
    __atomic_inc((am_atomic_t*)&mInUsedPayload_e);
  }
}

void CRingPacketPool::AVQPayloadDrop()
{
  //AUTO_LOCK(mpMutex);
  mFreeMem += mpPayloadPool[mReadPos].mData.mSize;
  mpPayloadPool[mReadPos].mData.mSize = 0;
  mpPayloadPool[mReadPos].mData.mBuffer = NULL;
  mReadPos = (mReadPos + 1) % mPayloadCount;
  __atomic_dec((am_atomic_t*)&mReadablePayloadCount);
}

void CRingPacketPool::AVQRelease(ExPayload* payload)
{
  //AUTO_LOCK(mpMutex);
  if (__atomic_dec(&(payload->mRefCount)) == 1) {
    if (payload->mIsNormalUse) {
      payload->mIsNormalUse = false;
      mFreeMem += payload->mData.mSize;
      __atomic_dec((am_atomic_t*)&mInUsedPayload);
    }
    if (payload->mIsEventUse) {
      payload->mIsEventUse = false;
      __atomic_dec((am_atomic_t*)&mInUsedPayload_e);
    }
  }
}

void CRingPacketPool::AVQPayloadBack()
{
  //AUTO_LOCK(mpMutex);
  if (mReadPos_e == 0) {
    mReadPos_e = mPayloadCount - 1;
  } else {
    mReadPos_e -= 1;
  }
  __atomic_inc((am_atomic_t*)&mReadablePayloadCount_e);
}

void CRingPacketPool::AVQWrite(CPacket::Payload* payload, bool isEvent)
{
  AUTO_LOCK(mpMutex);
  mpPayloadPool[mWritePos] = *payload;
  if (AM_LIKELY(payload->mData.mBuffer && (payload->mData.mSize > 0))) {
    if (mpCurrent + payload->mData.mSize > mpRingBufferMemEnd) {
      mpCurrent = mpRingBufferMem;
    }
    memcpy(mpCurrent, payload->mData.mBuffer, payload->mData.mSize);
    mpPayloadPool[mWritePos].mData.mBuffer = mpCurrent;
    mpCurrent += payload->mData.mSize;
  } else {
    mpPayloadPool[mWritePos].mData.mSize   = 0;
    mpPayloadPool[mWritePos].mData.mBuffer = NULL;
  }
  mWritePos = (mWritePos + 1) % mPayloadCount;
  mFreeMem -= payload->mData.mSize;
  __atomic_inc((am_atomic_t*)&mReadablePayloadCount);
  if (isEvent) {
    __atomic_inc((am_atomic_t*)&mReadablePayloadCount_e);
  }
}

ExPayload* CRingPacketPool::GetPrevAVQPayload()
{
  return (mReadPos_e == 0) ? &mpPayloadPool[mPayloadCount - 1] :
                             &mpPayloadPool[mReadPos_e - 1];
}

bool CRingPacketPool::IsAVQPayloadFull(bool isEvent)
{
  AUTO_LOCK(mpMutex);
  return isEvent ?
      ((mReadablePayloadCount_e + mInUsedPayload_e) >= mPayloadCount):
      ((mReadablePayloadCount + mInUsedPayload) >= mPayloadCount);
}

bool CRingPacketPool::IsAVQPayloadEmpty(bool isEvent)
{
  AUTO_LOCK(mpMutex);
  return isEvent ? (mReadablePayloadCount_e == 0) :
                   (mReadablePayloadCount == 0);
}

bool CRingPacketPool::IsAVQPayloadUsed()
{
  AUTO_LOCK(mpMutex);
  return (mInUsedPayload + mInUsedPayload_e) == 0;
}

bool CRingPacketPool::IsAVQAboutToFull(bool isEvent)
{
  AUTO_LOCK(mpMutex);
  if (isEvent) {
    return (mReadablePayloadCount_e + mInUsedPayload_e) >=
           (mPayloadCount * 4 / 5);
  }

  return (mReadablePayloadCount + mInUsedPayload) >= (mPayloadCount * 4 / 5);
}

bool CRingPacketPool::IsAVQAboutToEmpty(bool isEvent)
{
  AUTO_LOCK(mpMutex);
  if (isEvent) {
    return (mReadablePayloadCount_e + mInUsedPayload_e) <=
           (mPayloadCount / 5);
  }

  return (mReadablePayloadCount + mInUsedPayload) <= (mPayloadCount / 5);
}

bool CRingPacketPool::IsAVQEventSync()
{
  return mReadablePayloadCount >= mReadablePayloadCount_e;
}

void CRingPacketPool::InitVar()
{
  mpCurrent = mpRingBufferMem;
  mFreeMem = mPayloadCount * mMaxDataSize;
  mReadablePayloadCount = 0;
  mReadablePayloadCount_e = 0;
  mReadPos = 0;
  mReadPos_e = 0;
  mWritePos = 0;
  mInUsedPayload = 0;
  mInUsedPayload_e = 0;
}

void CRingPacketPool::InitEvent()
{
  mReadPos_e = mWritePos;
  if (mReadPos_e > 0) {
    --mReadPos_e;
  }
  mReadablePayloadCount_e = 1;
  mInUsedPayload_e = 0;
}

AM_INT CRingPacketPool::GetFreeMemSize()
{
  return mFreeMem;
}

AM_INT CRingPacketPool::GetFreePayloadCount()
{
  return mPayloadCount - mReadablePayloadCount - mInUsedPayload;
}
