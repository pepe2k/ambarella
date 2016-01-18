/*
 * packet_distributor.cpp
 *
 * @Author: Hanbo Xiao
 * @Time  : 10/09/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
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
#include "am_muxer_info.h"
#include "output_record_if.h"
#include "packet_distributor.h"

#ifndef POOL_NAME_LEN
#define POOL_NAME_LEN 256
#endif

#ifndef ONE_WAY_PACKET_NUM
#define ONE_WAY_PACKET_NUM 4
#endif

static AM_PTS gLastPts[MAX_ENCODE_STREAM_NUM][2];

IPacketFilter *CreatePacketDistributor (IEngine *pEngine, AM_UINT muxerAmount)
{
   return CPacketDistributor::Create (pEngine, muxerAmount);
}

CPacketDistributor *CPacketDistributor::Create (IEngine *pEngine,
                                                AM_UINT  muxerAmount,
                                                bool     RTPriority,
                                                int      priority)
{
   CPacketDistributor *result = new CPacketDistributor (pEngine, muxerAmount);
   if (result && result->Construct (RTPriority, priority) != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

CPacketDistributor::CPacketDistributor (IEngine *pEngine, AM_UINT muxerAmount):
   inherited (pEngine, "Output"),
   mRun (false),
   mMuxerNum (muxerAmount),
   mEosNum (0),
   mSourceMask (NULL),
   mpMediaInput (NULL),
   mppMediaOutput (NULL),
   mppMediaPacketPool (NULL),
   mppPacketPoolName (NULL)
{
  memset(gLastPts, 0, sizeof(AM_PTS)*MAX_ENCODE_STREAM_NUM*2);
}

AM_ERR CPacketDistributor::Construct (bool RTPriority, int priority)
{
   AM_ERR err;

   /* Check whether mMuxerNum is equal to zero or not. */
   if (mMuxerNum == 0) {
      ERROR ("No muxer is specified.\n");
      return ME_ERROR;
   }

   if ((err = inherited::Construct (RTPriority, priority)) != ME_OK) {
      ERROR ("Error occurs on inherited::Construct.\n");
      return ME_ERROR;
   }

   if ((mpMediaInput = CPacketDistributorInput::Create (this)) == NULL) {
      ERROR ("Failed to create media input.\n");
      return ME_ERROR;
   }

   mpMediaInput->Enable (true);

   if ((mppMediaOutput = new CPacketDistributorOutput *[mMuxerNum]) == NULL) {
      ERROR ("Failed to allocate memory for video output pointer\n");
      return ME_ERROR;
   }

   if ((mppMediaPacketPool = new CPacketPool *[mMuxerNum]) == NULL) {
      ERROR ("Failed to allocate memory for video buffer pool pointer.\n");
      return ME_ERROR;
   }

   if ((mppPacketPoolName = new AM_U8* [mMuxerNum]) == NULL) {
     ERROR("Failed to allocate memory for video buffer pool name pointer.");
     return ME_ERROR;
   }
   memset(mppPacketPoolName, 0, mMuxerNum*sizeof(AM_U8*));

   for (AM_UINT i = 0; i < mMuxerNum; i++) {
      mppMediaOutput[i] = NULL;
      mppMediaPacketPool[i] = NULL;
      mppPacketPoolName[i] = new AM_U8[POOL_NAME_LEN];

      if ((mppMediaOutput[i] = CPacketDistributorOutput::Create (this))
          == NULL) {
         ERROR ("Failed to create %dth video output\n", i);
         return ME_ERROR;
      }

      if (AM_UNLIKELY(NULL == mppPacketPoolName[i])) {
        ERROR("Failed to allocate memory for A/V packet pool: %d's name", i);
        return ME_ERROR;
      }
      memset(mppPacketPoolName[i], 0, POOL_NAME_LEN);
      snprintf ((char*)mppPacketPoolName[i],
                POOL_NAME_LEN, "A/V packet pool: %d", i);
      if ((mppMediaPacketPool[i] = CPacketPool::Create (
          (const char*)mppPacketPoolName[i],
          (mMuxerNum + 1) * ONE_WAY_PACKET_NUM)) == NULL) {
         ERROR ("Failed to create %dth video buffer pool.\n", i);
         return ME_ERROR;
      }

      mppMediaOutput[i]->SetBufferPool (mppMediaPacketPool[i]);
   }

   if ((mSourceMask = new AM_UINT[mMuxerNum]) == NULL) {
      ERROR ("Failed to allocate memory for enum array.\n");
      return ME_ERROR;
   }

   memset (mSourceMask, 0, mMuxerNum * sizeof (AM_UINT));
   return ME_OK;
}

CPacketDistributor::~CPacketDistributor ()
{
   AM_DELETE (mpMediaInput);

   for (AM_UINT i = 0; i < mMuxerNum; i++) {
      AM_DELETE (mppMediaOutput[i]);
      AM_RELEASE (mppMediaPacketPool[i]);
      delete[] mppPacketPoolName[i];
   }

   delete[] mppMediaOutput;
   delete[] mppMediaPacketPool;
   delete[] mppPacketPoolName;

   if (mSourceMask) {
      delete[] mSourceMask;
      mSourceMask = NULL;
   }
}

AM_ERR CPacketDistributor::SetSourceMask (AM_UINT muxerIndex,
                                          AM_UINT sourceMask)
{
   if (muxerIndex > mMuxerNum - 1) {
      ERROR ("No such output pin.\n");
      return ME_ERROR;
   }

   mSourceMask[muxerIndex] = sourceMask;
   return ME_OK;
}

void *CPacketDistributor::GetInterface (AM_REFIID refiid)
{
   return inherited::GetInterface (refiid);
}

void CPacketDistributor::Delete ()
{
  inherited::Delete();
}

AM_ERR CPacketDistributor::Stop ()
{
   AM_ERR ret = ME_OK;

   if (mRun) {
      mRun = false;
      ret = inherited::Stop ();
   }

   return ret;
}

void CPacketDistributor::GetInfo (IPacketFilter::INFO &info)
{
   info.nInput = 1;
   info.nOutput = mMuxerNum;
   info.pName = "PacketDistributor";
}

IPacketPin *CPacketDistributor::GetInputPin (AM_UINT index)
{
   if (index == 0) {
      return mpMediaInput;
   }

   return NULL;
}

IPacketPin *CPacketDistributor::GetOutputPin (AM_UINT index)
{
   if (index > mMuxerNum) {
      ERROR ("No such output pin: %d\n", index);
      return NULL;
   }

   return mppMediaOutput[index];
}

void CPacketDistributor::OnRun ()
{
  CPacket     *packetIncome = NULL;
  CPacketQueueInputPin *pin = NULL;

  mRun = true;
  CmdAck (ME_OK);

  while (mRun) {
    if (!WaitInputBuffer (pin, packetIncome)) {
      if (AM_LIKELY(!mRun)) {
        NOTICE("Stop is called!");
      } else {
        ERROR ("Failed to wait data!");
      }
      break;
    }

    switch(packetIncome->GetType()) {
      case CPacket::AM_PAYLOAD_TYPE_INFO:
      case CPacket::AM_PAYLOAD_TYPE_EOS: {
        if (AM_UNLIKELY(packetIncome->GetType() ==
            CPacket::AM_PAYLOAD_TYPE_INFO)) {
          ++ mEosNum;
        } else {
          if (AM_LIKELY(false == (mRun = ((-- mEosNum) != 0)))) {
            NOTICE("All EOS packet is sent to muxers, "
                "data adapter needs to exit!");
          }
        }
      }
      /* no break */
      default: {
        CPacket::AmPayloadAttr attr = packetIncome->GetAttr();
        AM_U16 streamId = packetIncome->GetStreamId();
        AM_U16 refCount = 0;

        for (AM_UINT i = 0; i < mMuxerNum; ++ i) {
          if (AM_LIKELY((mSourceMask[i] & (1 << streamId)) &&
                        ((attr == CPacket::AM_PAYLOAD_ATTR_AUDIO)     ||
                         (attr == CPacket::AM_PAYLOAD_ATTR_VIDEO)     ||
                         (attr == CPacket::AM_PAYLOAD_ATTR_SEI)       ||
                         (attr == CPacket::AM_PAYLOAD_ATTR_EVENT_EMG) ||
                         (attr == CPacket::AM_PAYLOAD_ATTR_EVENT_MD)))) {
            if (AM_LIKELY((++ refCount) > 1)) {
              packetIncome->AddRef();
            }
            switch(attr) {
              case CPacket::AM_PAYLOAD_ATTR_AUDIO: {
                DEBUG("Get Audio Data: %8u bytes, PTS: %10llu, Audio Type: %u "
                      "Stream: %u, PTS Diff: %lld", packetIncome->GetDataSize(),
                      packetIncome->GetPTS(), packetIncome->GetFrameType(),
                      streamId, packetIncome->GetPTS() - gLastPts[streamId][1]);
                gLastPts[streamId][1] = packetIncome->GetPTS();
              }break;
              case CPacket::AM_PAYLOAD_ATTR_VIDEO: {
                DEBUG("Get Video Data: %8u bytes, PTS: %10llu, Video Type: %u "
                      "Stream: %u, PTS Diff: %lld", packetIncome->GetDataSize(),
                      packetIncome->GetPTS(), packetIncome->GetFrameType(),
                      streamId, packetIncome->GetPTS() - gLastPts[streamId][0]);
                gLastPts[streamId][0] = packetIncome->GetPTS();
              }break;
              default:break;
            }
            mppMediaOutput[i]->SendBuffer(packetIncome);
          } else if (AM_UNLIKELY((mSourceMask[i] & (1 << streamId)))) {
            ERROR("Only Video/Audio/SEI/EVENT packets are supported!");
            packetIncome->Release();
          }
        }
      }break;
    }
  }
  INFO("PacketDistributor exit mainloop!");
}

CPacketDistributorInput *CPacketDistributorInput::Create(CPacketFilter *pFilter)
{
  CPacketDistributorInput *result = new CPacketDistributorInput(pFilter);
  if (result && result->Construct() != ME_OK) {
    delete result;
    result = NULL;
  }

  return result;
}

AM_ERR CPacketDistributorInput::Construct()
{
  if (inherited::Construct(((CPacketDistributor *) mpFilter)->MsgQ())
      != ME_OK) {
    return ME_ERROR;
  }

  return ME_OK;
}

CPacketDistributorOutput *CPacketDistributorOutput::Create(
    CPacketFilter *pFilter)
{
  CPacketDistributorOutput *result = new CPacketDistributorOutput(pFilter);
  if (result && result->Construct() != ME_OK) {
    delete result;
    result = NULL;
  }

  return result;
}
