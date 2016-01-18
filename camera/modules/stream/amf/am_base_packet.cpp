/*******************************************************************************
 * am_base_packet.cpp
 *
 * Histroy:
 *   2012-9-11 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

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

/*
 * CPacketPool
 */
CPacketPool *CPacketPool::Create (const char *pName, AM_UINT count)
{
   CPacketPool *result = new CPacketPool (pName);
   if (result && result->Construct (count) != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

AM_ERR CPacketPool::Construct(AM_UINT nMaxBuffers)
{
  if ((mpBufferQ = CQueue::Create(NULL, this, sizeof(CPacket*), nMaxBuffers))
      == NULL) {
    return ME_NO_MEMORY;
  }

  DEBUG("Packet pool '%s' created\n", mpName);
  return ME_OK;
}

CPacketPool::~CPacketPool()
{
  DEBUG("Packet pool '%s' destroyed\n", mpName);
  AM_DELETE(mpBufferQ);
}

void *CPacketPool::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IPacketPool) {
    return (IPacketPool*) this;
  }
  return inherited::GetInterface(refiid);
}

void CPacketPool::Enable(bool bEnabled)
{
  mpBufferQ->Enable(bEnabled);
}

bool CPacketPool::AllocBuffer(CPacket*& pBuffer, AM_UINT size)
{
  if (mpBufferQ->GetMsgEx(&pBuffer, sizeof(pBuffer))) {
    pBuffer->mRefCount = 1;
    pBuffer->mpPool = this;
    pBuffer->mpNext = NULL;
    pBuffer->SetPacketType(CPacket::AM_PACKET_TYPE_NORMAL);
    return true;
  }
  return false;
}

void CPacketPool::AddRef(CPacket *pBuffer)
{
  __atomic_inc(&pBuffer->mRefCount);
}

void CPacketPool::Release(CPacket *pBuffer)
{
  if (__atomic_dec(&pBuffer->mRefCount) == 1) {
    // from 1 to 0
    CPacket *pBuffer2 = pBuffer->mpNext;

    OnReleaseBuffer(pBuffer);
    AM_ENSURE_OK_( mpBufferQ->PostMsg((void*)&pBuffer, sizeof(CPacket*)));

    if (pBuffer2) {
      OnReleaseBuffer(pBuffer2);
      AM_ENSURE_OK_( mpBufferQ->PostMsg((void*)&pBuffer2, sizeof(CPacket*)));
    }
  }
}

void CPacketPool::AddRef()
{
  __atomic_inc(&mRefCount);
}

void CPacketPool::Release()
{
  if (__atomic_dec(&mRefCount) == 1) {
    inherited::Delete();
  }
}

/*
 * CSimplePacketPool
 */
CSimplePacketPool* CSimplePacketPool::Create(const char *pName,
                                             AM_UINT     count,
                                             AM_UINT     objectSize)
{
  CSimplePacketPool* result = new CSimplePacketPool(pName);
  if (AM_UNLIKELY(result && result->Construct(count, objectSize) != ME_OK)) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CSimplePacketPool::Construct(AM_UINT count, AM_UINT objectSize)
{
  AM_ERR err = ME_ERROR;

  if (AM_UNLIKELY((count > 0) &&
                  (err = inherited::Construct(count)) != ME_OK)) {
    return err;
  }

  AM_ASSERT(objectSize == sizeof(CPacket));
  mpPacketMemory = new CPacket[count];
  if (AM_UNLIKELY(mpPacketMemory == NULL)) {
    return ME_NO_MEMORY;
  } else {
    for (AM_UINT i = 0; i < count; ++ i) {
      CPacket *pPacket = &mpPacketMemory[i];
      AM_ENSURE_OK_( mpBufferQ->PostMsg(&pPacket, sizeof(pPacket)));
    }
  }

  return ME_OK;
}

CSimplePacketPool::~CSimplePacketPool()
{
  delete[] mpPacketMemory;
}

/*
 * CFixedPacketPool
 */
CFixedPacketPool* CFixedPacketPool::Create(const char *pName,
                                           AM_UINT     count,
                                           AM_UINT     dataSize)
{
  CFixedPacketPool* result = new CFixedPacketPool(pName);
  if (AM_UNLIKELY(result && result->Construct(count, dataSize) != ME_OK)) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CFixedPacketPool::Construct(AM_UINT count, AM_UINT dataSize)
{
  AM_ERR err;
  if (AM_LIKELY(((err = inherited::Construct(count)) == ME_OK))) {
    AM_UINT payloadSize = ROUND_UP((dataSize + sizeof(CPacket::Payload)), 4);
    mpPayloadMemory = new AM_U8[payloadSize * count];
    if (AM_UNLIKELY(mpPayloadMemory == NULL)) {
      err = ME_NO_MEMORY;
    } else {
      for (AM_UINT i = 0; i < count; ++ i) {
        mpPacketMemory[i].SetPayload(mpPayloadMemory + i * payloadSize);
      }
      err = ME_OK;
    }
  }

  return err;
}

CFixedPacketPool::~CFixedPacketPool()
{
  delete[] mpPayloadMemory;
}

/*
 * CPacketPin
 */
void *CPacketPin::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IPacketPool) {
    return mpPacketPool;
  }
  if (refiid == IID_IPacketPin) {
    return (IPacketPin*) this;
  }
  return inherited::GetInterface(refiid);
}

CPacketPin::~CPacketPin()
{
  // force to release
  SetBufferPool(NULL);
}

void CPacketPin::OnDisconnect()
{
  ReleaseBufferPool();
}

AM_ERR CPacketPin::NoBufferPoolHandler()
{
  ERROR("No buffer pool for pin of %s\n", FilterName());
  return ME_NO_INTERFACE;
}

void CPacketPin::SetBufferPool(IPacketPool *pPacketPool)
{
  if (pPacketPool) {
    pPacketPool->AddRef();
  }

  if (mpPacketPool) {
    mpPacketPool->Release();
  }

  mpPacketPool = pPacketPool;
  mbSetBP = (pPacketPool != NULL);
}

void CPacketPin::ReleaseBufferPool()
{
  if (mpPacketPool && !mbSetBP) {
    mpPacketPool->Release();
    mpPacketPool = NULL;
  }
}

/*
 * CPacketInputPin
 */
AM_ERR CPacketInputPin::Connect(IPacketPin *pPeer)
{
  AM_ERR ret = ME_ERROR;
  if (NULL == mpPeer) {
    if (ME_OK == (ret = OnConnect(pPeer))) {
      mpPeer = pPeer;
    } else {
      ERROR("Failed to connect this pin to peer!");
    }
  } else {
    ERROR("Input pin of %s is already connected\n", FilterName());
    ret = ME_BAD_STATE;
  }
  DEBUG("CPacketInputPin::Connect");

  return ret;
}

void CPacketInputPin::Disconnect()
{
  if (mpPeer) {
    OnDisconnect();
    mpPeer = NULL;
  }
}

AM_ERR CPacketInputPin::OnConnect(IPacketPin *pPeer)
{
  // check buffer pool

  if (mpPacketPool == NULL) {
    // if upstream can provide buffer pool
    if ((mpPacketPool = IPacketPool::GetInterfaceFrom(pPeer))) {
      mpPacketPool->AddRef();
    } else {
      return NoBufferPoolHandler();
    }
  }

  return ME_OK;
}

/*
 * CPacketOutputPin
 */
AM_ERR CPacketOutputPin::Connect(IPacketPin *pPeer)
{
  AM_ERR ret = ME_ERROR;

  if (!mpPeer) {
    if (ME_OK == (ret = pPeer->Connect(this))) {
      if (ME_OK == (ret = OnConnect(pPeer))) {
        mpPeer = pPeer;
      } else {
        ERROR("Failed to connect this pin to peer!");
        pPeer->Disconnect();
      }
    } else {
      ERROR("Failed to connect peer to this pin!");
    }
  } else {
    ERROR("Output pin of %s is already connected\n", FilterName());
    ret = ME_BAD_STATE;
  }
  DEBUG("CPacketOutputPin::Connect is called!");

  return ret;
}

void CPacketOutputPin::Disconnect()
{
  if (mpPeer == NULL) {
    return;
  }

  OnDisconnect();

  mpPeer->Disconnect();
  mpPeer = NULL;
}

AM_ERR CPacketOutputPin::OnConnect(IPacketPin *pPeer)
{
  if (mpPacketPool == NULL) {
    if ((mpPacketPool = IPacketPool::GetInterfaceFrom(pPeer))) {
      mpPacketPool->AddRef();
    } else {
      AM_ERR err = NoBufferPoolHandler();
      return (err != ME_OK) ? err : ME_OK;
    }
  }

  // check
  IPacketPool *pBufferPool = IPacketPool::GetInterfaceFrom(pPeer);
  if (mpPacketPool != pBufferPool) {
    ERROR("Pin of %s uses different BP\n", FilterName());
    ReleaseBufferPool();
    return ME_ERROR;
  }

  return ME_OK;
}

/*
 * CPacketActiveOutputPin
 */
void *CPacketActiveOutputPin::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IActiveObject) {
    return (IActiveObject*) this;
  }
  return inherited::GetInterface(refiid);
}

AM_ERR CPacketActiveOutputPin::Construct()
{
  if ((mpWorkQ = CWorkQueue::Create((IActiveObject*) this)) == NULL) {
    return ME_NO_MEMORY;
  }
  return ME_OK;
}

CPacketActiveOutputPin::~CPacketActiveOutputPin()
{
  AM_DELETE(mpWorkQ);
}

/*
 * CPacketQueueInputPin
 */
AM_ERR CPacketQueueInputPin::Construct(CQueue *pMsgQ)
{
  AM_ERR err = inherited::Construct();
  if (err != ME_OK) {
    return err;
  }

  if ((mpBufferQ = CQueue::Create(pMsgQ,
                                  this,
                                  sizeof(CPacket*),
                                  INITIAL_QUEUE_LENGTH)) == NULL) {
    return ME_NO_MEMORY;
  }

  return ME_OK;
}

CPacketQueueInputPin::~CPacketQueueInputPin()
{
  AM_DELETE(mpBufferQ);
}

void CPacketQueueInputPin::Purge()
{
  if (mpBufferQ) {
    CPacket *pBuffer;
    while (mpBufferQ->PeekData((void*) &pBuffer, sizeof(pBuffer))) {
      pBuffer->Release();
    }
  }
}

/*
 * CPacketActiveInputPin
 */
AM_ERR CPacketActiveInputPin::Construct()
{
  if ((mpWorkQ = CWorkQueue::Create((IActiveObject*) this)) == NULL)
    return ME_NO_MEMORY;

  AM_ERR err = inherited::Construct(mpWorkQ->MsgQ());
  if (err != ME_OK) {
    return err;
  }

  return ME_OK;
}

CPacketActiveInputPin::~CPacketActiveInputPin()
{
  AM_DELETE(mpBufferQ);
  mpBufferQ = NULL;
  AM_DELETE(mpWorkQ);
}

void *CPacketActiveInputPin::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IActiveObject) {
    return (IActiveObject*) this;
  }
  return inherited::GetInterface(refiid);
}

// This is a example OnRun() and can be used in common-simple case.
// If the pin shall response to other commands besides CMD_STOP,
// it should override OnCmd().
void CPacketActiveInputPin::OnRun()
{
  CmdAck(ME_OK);

  CQueue::WaitResult result;
  CPacket *pPacket;
  CMD cmd;

  while (true) {
    CQueue::QType type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);

    if (type == CQueue::Q_MSG) {
      // running state, received a cmd
      if (cmd.code == AO::CMD_STOP) {
        CmdAck(ME_OK);
        INFO("%s pin received stop command!", GetName());
        break;
      }
    } else {
      // a buffer
      if (PeekBuffer(pPacket)) {
        AM_ERR err = ProcessBuffer(pPacket);
        if (err != ME_OK) {
          if (err != ME_CLOSED)
            PostEngineErrorMsg(err);
          break;
        }
      }
    }
  }
}

/*
 * CPacketFilter
 */
AM_ERR CPacketFilter::Construct()
{
  INFO info;
  info.pName = "Filter";
  GetInfo(info);
  mpName = info.pName;
  return ME_OK;
}

void *CPacketFilter::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IPacketFilter) {
    return (IPacketFilter*) this;
  }
  return inherited::GetInterface(refiid);
}

/*
 * CPacketActiveFilter
 */
AM_ERR CPacketActiveFilter::Construct(bool RTPriority, int priority)
{
  if ((mpWorkQ = CWorkQueue::Create((IActiveObject*) this,
                                    RTPriority, priority))
      == NULL) {
    return ME_NO_MEMORY;
  }

  return ME_OK;
}

CPacketActiveFilter::~CPacketActiveFilter()
{
  AM_DELETE(mpWorkQ);
}

void *CPacketActiveFilter::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IActiveObject) {
    return (IActiveObject*) this;
  }
  return inherited::GetInterface(refiid);
}

void CPacketActiveFilter::GetInfo(INFO& info)
{
  info.nInput = 0;
  info.nOutput = 0;
  info.pName = mpName;
}

bool CPacketActiveFilter::WaitInputBuffer(CPacketQueueInputPin*& pPin,
                                          CPacket*&              pBuffer)
{
  CQueue::WaitResult result;
  CMD cmd;

  while (true) {
    CQueue::QType type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
    if (type == CQueue::Q_MSG) {
      if (!ProcessCmd(cmd)) {
        if (cmd.code == AO::CMD_STOP) {
          OnStop();
          CmdAck(ME_OK);
          return false;
        }
      }
    } else {
      pPin = (CPacketQueueInputPin*) result.pOwner;
      if (pPin->PeekBuffer(pBuffer)) {
        return true;
      }
      INFO("No buffer?\n");
    }
  }

  return false;
}

/*
 * CPacketFilterGraph
 */
void CPacketFilterGraph::ClearGraph()
{
  DEBUG("=== Clear graph");
  if (mnFilters > 0) {
    StopAllFilters();
    PurgeAllFilters();
    DeleteAllConnections();
    RemoveAllFilters();
  }
  DEBUG("Clear graph done");
}

CPacketFilterGraph::~CPacketFilterGraph()
{
  ClearGraph();
}

AM_ERR CPacketFilterGraph::RunAllFilters()
{
  INFO("=== RunAllFilters");
  for (AM_UINT i = 0; i < mnFilters; i ++) {
    IPacketFilter *pFilter = mFilters[i].pFilter;
    INFO("Run %s (%d)", GetFilterName(pFilter), i + 1);
    EnableOutputPins(pFilter);
    AM_ERR err = pFilter->Run();
    if (err != ME_OK) {
      INFO("Run %s (%d) returns %d", GetFilterName(pFilter), i + 1, err);
      return err;
    }
  }
  INFO("RunAllFilters done");
  return ME_OK;
}

void CPacketFilterGraph::StopAllFilters()
{
  INFO("=== StopAllFilters");
  for (AM_UINT i = mnFilters; i > 0; i --) {
    IPacketFilter *pFilter = mFilters[i - 1].pFilter;
    INFO("Stop %s (%d)\n", GetFilterName(pFilter), i);
    DisableOutputPins(pFilter);
    AM_ENSURE_OK_( pFilter->Stop());
  }
  INFO("StopAllFilters done");
}

void CPacketFilterGraph::PurgeAllFilters()
{
  INFO("=== PurgeAllFilters");
  for (AM_UINT i = 0; i < mnFilters; i ++) {
    INFO("Purge %s (%d)\n", GetFilterName(mFilters[i].pFilter), i + 1);
    PurgeFilter(mFilters[i].pFilter);
  }
  INFO("PurgeAllFilters done");
}

void CPacketFilterGraph::DeleteAllConnections()
{
  INFO("=== DeleteAllConnections");
  for (AM_UINT i = mnConnections; i > 0; -- i) {
    mConnections[i - 1].pOutputPin->Disconnect();
  }
  mnConnections = 0;
  INFO("DeleteAllConnections done");
}

void CPacketFilterGraph::RemoveAllFilters()
{
  INFO("=== RemoveAllFilters");
  for (AM_UINT i = mnFilters; i > 0; i --) {
    INFO("Remove %s (%d)", GetFilterName(mFilters[i-1].pFilter), i);
    mFilters[i - 1].pFilter->Delete();
  }
  mnFilters = 0;
  INFO("RemoveAllFilters done");
}

void CPacketFilterGraph::EnableOutputPins(IPacketFilter *pFilter, bool bEnable)
{
  IPacketFilter::INFO info;
  pFilter->GetInfo(info);
  for (AM_UINT j = 0; j < info.nOutput; j ++) {
    IPacketPin *pPin = pFilter->GetOutputPin(j);
    if (pPin != NULL) {
      pPin->Enable(bEnable);
    }
  }
}

AM_ERR CPacketFilterGraph::AddFilter(IPacketFilter *pFilter)
{
  if (mnFilters >= ARRAY_SIZE(mFilters)) {
    AM_ERROR("too many filters\n");
    return ME_TOO_MANY;
  }

  mFilters[mnFilters].pFilter = pFilter;
  mFilters[mnFilters].flags = 0;
  mnFilters ++;

  INFO("Added %s (%d)\n", GetFilterName(pFilter), mnFilters);

  return ME_OK;
}

AM_ERR CPacketFilterGraph::Connect(IPacketFilter *pUpstreamFilter,
                                   AM_UINT indexUp,
                                   IPacketFilter *pDownstreamFilter,
                                   AM_UINT indexDown)
{
  IPacketPin *pOutput = pUpstreamFilter->GetOutputPin(indexUp);
  if (pOutput == NULL) {
    WARN("No such pin: %s[%d]\n", GetFilterName(pUpstreamFilter), indexUp);
    return ME_ERROR;
  }

  IPacketPin *pInput = pDownstreamFilter->GetInputPin(indexDown);
  if (pInput == NULL) {
    WARN("No such pin: %s[%d]\n", GetFilterName(pDownstreamFilter), indexDown);
    return ME_ERROR;
  }

  return CreateConnection(pOutput, pInput);
}

AM_ERR CPacketFilterGraph::CreateConnection(IPacketPin *pOutputPin,
                                            IPacketPin *pInputPin)
{
  AM_ERR ret = ME_ERROR;
  if (mnConnections >= ARRAY_SIZE(mConnections)) {
    AM_ERROR("too many connections\n");
    ret = ME_TOO_MANY;
  } else {
    if (ME_OK == (ret = pOutputPin->Connect(pInputPin))) {
      mConnections[mnConnections].pOutputPin = pOutputPin;
      mConnections[mnConnections].pInputPin = pInputPin;
      mnConnections ++;
    } else {
      ERROR("Failed to connect output pin %p to input pin %p!",
            pOutputPin, pInputPin);
    }
  }
  DEBUG("Returned AM_ERR value is %u, 0x%08x", ret, ret);

  return ret;
}

const char *CPacketFilterGraph::GetFilterName(IPacketFilter *pFilter)
{
  IPacketFilter::INFO info;
  pFilter->GetInfo(info);
  return info.pName;
}

void CPacketFilterGraph::PurgeFilter(IPacketFilter *pFilter)
{
  IPacketFilter::INFO info;
  pFilter->GetInfo(info);
  for (AM_UINT i = 0; i < info.nInput; i ++) {
    IPacketPin *pPin = pFilter->GetInputPin(i);
    if (pPin) {
      pPin->Purge();
    }
  }
}
