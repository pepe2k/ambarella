/*******************************************************************************
 * packet_aggregator.cpp
 *
 * @Author: Hanbo Xiao
 * @Time  : 13/11/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ******************************************************************************/
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"

#include "packet_aggregator_if.h"
#include "packet_aggregator.h"

#ifndef INPUT_NAME_LEN
#define INPUT_NAME_LEN 256
#endif

#ifndef ONE_WAY_PACKET_NUM
#define ONE_WAY_PACKET_NUM 4
#endif

static const char* pinNames[] =
{
  "1st",
  "2nd",
  "3rd",
  "4th",
  "5th",
  "6th",
  "7th",
  "8th"
};

IPacketFilter* create_packet_aggregator_filter(IEngine *pEngine)
{
  return CPacketAggregator::Create (pEngine);
}

CPacketAggregator *CPacketAggregator::Create (IEngine *pEngine,
                                              bool RTPriority,
                                              int priority)
{
  CPacketAggregator *result = new CPacketAggregator (pEngine);
  if (result && result->Construct (RTPriority, priority) != ME_OK) {
    delete result;
    result = NULL;
  }

  return result;
}

CPacketAggregator::CPacketAggregator (IEngine *pEngine):
       inherited (pEngine, "PacketAggregator"),
       mRun (false),
       mEOSNum (0),
       mInputNum (0),
       mStreamNum (0),
       mInputName (NULL),
       mpMutex (NULL),
       mpEvent (NULL),
       mppMediaInput (NULL),
       mpMediaOutput (NULL),
       mpMediaPacketPool (NULL)
{}

AM_ERR CPacketAggregator::SetStreamNumber(AM_UINT streamNum)
{
  /* mStreamNum == VideoStreamNumber + AudioStreamNumber */
  mStreamNum = streamNum;
  return ME_OK;
}

AM_ERR CPacketAggregator::SetInputNumber(AM_UINT inputNum)
{
  if (AM_UNLIKELY(mInputNum > 0)) {
    for (int i = 0; i < mInputNum; ++ i) {
      AM_DELETE(mppMediaInput[i]);
      delete[] mInputName[i];
    }
    delete[] mInputName;
    delete[] mppMediaInput;
  }
  mInputNum = inputNum;
  if (AM_LIKELY(mpMediaOutput)) {
    mpMediaOutput->SetBufferPool(NULL);
  }
  AM_RELEASE (mpMediaPacketPool);
  mpMediaPacketPool = NULL;

  /* Check whether mInputNum is less than or equal to zero or not. */
  if (AM_UNLIKELY(mInputNum <= 0)) {
    ERROR ("No input pin need to be created, mInputNum = %d", mInputNum);
    return ME_ERROR;
  }

  if (AM_UNLIKELY((mInputName = new char *[mInputNum]) == NULL)) {
    ERROR ("Failed to allocate memory to pointers.");
    return ME_ERROR;
  }

  if (AM_UNLIKELY((mppMediaInput = new CPacketAggregatorInput *[mInputNum])
                  == NULL)) {
    ERROR ("Failed to allocate memory for pointers.");
    return ME_NO_MEMORY;
  }

  memset(mInputName, 0, sizeof(char*) * mInputNum);

  for (int i = 0; i < mInputNum; ++ i) {
    mppMediaInput[i] = NULL;

    if ((mInputName[i] = new char[INPUT_NAME_LEN]) == NULL) {
      ERROR ("Failed to allocate memory to store input name.");
      return ME_ERROR;
    }

    snprintf (mInputName[i], INPUT_NAME_LEN, "%s input", pinNames[i]);
    if ((mppMediaInput[i] = CPacketAggregatorInput::Create (
        this, mInputName[i])) == NULL) {
      ERROR ("Failed to create %dth input for packet aggregator", i);
      return ME_ERROR;
    }

    mppMediaInput[i]->Enable (true);
  }

  AM_RELEASE (mpMediaPacketPool);
  mpMediaPacketPool = NULL;
  if (AM_UNLIKELY((mpMediaPacketPool = CPacketPool::Create (
      "AggregatorPacketPool", mInputNum * ONE_WAY_PACKET_NUM)) == NULL)) {
    ERROR ("Failed to create packet pool for packet aggregator");
    return ME_ERROR;
  }

  mpMediaOutput->SetBufferPool (mpMediaPacketPool);

  return ME_OK;
}

AM_ERR CPacketAggregator::Construct (bool RTPriority, int priority)
{
  if (AM_UNLIKELY(inherited::Construct (RTPriority, priority) != ME_OK)) {
    ERROR ("Error occurs on inherited::Construct.");
    return ME_ERROR;
  }

  if (AM_UNLIKELY((mpMutex = CMutex::Create ()) == NULL)) {
    ERROR ("Failed to create mutex");
    return ME_ERROR;
  }

  if (AM_UNLIKELY((mpEvent = CEvent::Create ()) == NULL)) {
    ERROR ("Failed to create event");
    return ME_ERROR;
  }

  if (AM_UNLIKELY((mpMediaOutput = CPacketAggregatorOutput::Create (this))
                  == NULL)) {
    ERROR ("Failed to create media output.");
    return ME_ERROR;
  }

  return ME_OK;
}

CPacketAggregator::~CPacketAggregator ()
{
  AM_DELETE(mpMutex);
  AM_DELETE(mpEvent);
  for (int i = 0; i < mInputNum; i++) {
    AM_DELETE (mppMediaInput[i]);
    delete[] mInputName[i];
  }
  AM_DELETE (mpMediaOutput);
  AM_RELEASE (mpMediaPacketPool);
  delete[] mppMediaInput;
  delete[] mInputName;
}

void *CPacketAggregator::GetInterface (AM_REFIID refiid)
{
  if (AM_LIKELY(refiid == IID_IPacketAggregator)) {
    return (IPacketAggregator*)this;
  }
  return inherited::GetInterface (refiid);
}

void CPacketAggregator::Delete ()
{
  inherited::Delete();
}

AM_ERR CPacketAggregator::Stop ()
{
  AM_ERR ret = ME_OK;

  if (mRun) {
    mRun = false;
    mpEvent->Signal();
    ret = inherited::Stop ();
  }

  return ret;
}

void CPacketAggregator::GetInfo (IPacketFilter::INFO &info)
{
  info.nInput = mInputNum;
  info.nOutput = 1;
  info.pName = "PacketAggregator";
}

IPacketPin *CPacketAggregator::GetInputPin (AM_UINT index)
{
  if (index >= (AM_UINT)mInputNum) {
    ERROR ("No such input pin: %d", index);
    return NULL;
  }

  return mppMediaInput[index];
}

IPacketPin *CPacketAggregator::GetOutputPin (AM_UINT index)
{
  if (index > 0) {
    ERROR ("No more than one output pin: %d", index);
    return NULL;
  }

  return mpMediaOutput;
}

void CPacketAggregator::OnRun ()
{
  for (int i = 0; i < mInputNum; ++ i) {
    mppMediaInput[i]->Run ();
  }

  CmdAck(ME_OK);

  mRun = true;
  mpEvent->Wait ();
  mRun = false;

  for (int i = 0; i < mInputNum; ++ i) {
    mppMediaInput[i]->Stop();
  }

  INFO("PacketAggregator exit mainloop!");
}

void CPacketAggregator::ProcessBuffer (CPacket *packet)
{
  if (AM_UNLIKELY((packet->GetType() == CPacket::AM_PAYLOAD_TYPE_EOS) &&
                  ((packet->GetAttr() == CPacket::AM_PAYLOAD_ATTR_AUDIO) ||
                   (packet->GetAttr() == CPacket::AM_PAYLOAD_ATTR_VIDEO)))) {
    /* Fixme: EOS packet of SEI and EVENT are not processed by all the filters
     * in output and PacketAggregator also doesn't count these EOS packets
     */
    __atomic_inc((am_atomic_t*)&mEOSNum);
  }

  mpMediaOutput->SendBuffer (packet);

  AUTO_LOCK (mpMutex);
  if (AM_UNLIKELY(mEOSNum == mStreamNum)) {
    INFO("PacketAggregator received all the EOS, exit!");
    mEOSNum = 0;
    mpEvent->Signal ();
  }
}

CPacketAggregatorInput *CPacketAggregatorInput::Create (
    CPacketFilter *pFilter, const char *pName)
{
  CPacketAggregatorInput *result = new CPacketAggregatorInput (pFilter, pName);
  if (result && result->Construct () != ME_OK) {
    delete result;
    result = NULL;
  }

  return result;
}

AM_ERR CPacketAggregatorInput::Construct ()
{
  return inherited::Construct ();
}

inline AM_ERR CPacketAggregatorInput::ProcessBuffer (CPacket *packet)
{
  ((CPacketAggregator *)mpFilter)->ProcessBuffer (packet);
  return ME_OK;
}

