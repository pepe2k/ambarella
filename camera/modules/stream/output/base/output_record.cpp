/*
 * output_record.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 21/09/2012 [Created]
 * @Time  : 2013/03/19 [Modified by ypchang]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "config.h"
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
#include "output_record.h"
#include "packet_distributor.h"

extern IPacketFilter *CreatePacketDistributor(IEngine *, AM_UINT);
extern IPacketFilter *CreateRawMuxer         (IEngine *);
extern IPacketFilter *CreateTsMuxer          (IEngine *);
extern IPacketFilter *CreateRtspServer       (IEngine *);
extern IPacketFilter *CreateMp4Muxer         (IEngine *);
extern IPacketFilter *CreateJpegMuxer        (IEngine *);

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

IOutputRecord* create_record_output(CPacketFilterGraph *pFilterGraph,
                                     IEngine *pEngine)
{
  return COutputRecord::Create(pFilterGraph, pEngine);
}

COutputRecord *COutputRecord::Create(CPacketFilterGraph *pFilterGraph,
                                     IEngine *pEngine)
{
  COutputRecord *result = new COutputRecord(pFilterGraph, pEngine);
  if (result && result->Construct() != ME_OK) {
    delete result;
    result = NULL;
  }

  return result;
}

COutputRecord::COutputRecord(CPacketFilterGraph *pFilterGraph,
                             IEngine *pEngine) :
    mpEngine(pEngine),
    mpFilterGraph(pFilterGraph),
    mpPacketDistributor(NULL),
    mRtspSendWait(false),
    mRtspNeedAuth(false)
{
  for (int streamId = 0; streamId < AM_STREAM_NUMBER_MAX; ++ streamId) {
    mMuxerMask[streamId] = 0;
    for (int muxerId = 0; muxerId < MUXER_TYPE_AMOUNT; ++ muxerId) {
      mpMuxerFilter[streamId][muxerId] = NULL;
      mpMediaSink[streamId][muxerId] = NULL;
      mpMuxerArray[streamId][muxerId] = NULL;
      mDuration[streamId][muxerId] = 0;
      mSourceMask[streamId][muxerId] = 0;
    }
  }
}

COutputRecord::~COutputRecord()
{
  for (int streamId = 0; streamId < AM_STREAM_NUMBER_MAX; ++ streamId) {
    for (int muxerId = 0; muxerId < MUXER_TYPE_AMOUNT; ++ muxerId) {
      delete[] mpMediaSink[streamId][muxerId];
      mpMediaSink[streamId][muxerId] = NULL;
      AM_DELETE(mpMuxerFilter[streamId][muxerId]);
    }
  }

  AM_DELETE(mpPacketDistributor);
  DEBUG("~COutputRecord");
}

AM_ERR COutputRecord::Construct()
{
  /* Initialize muxer creating function */
  mpMuxerCreatorArray[0] = CreateRawMuxer;
  mpMuxerCreatorArray[1] = CreateTsMuxer;
  mpMuxerCreatorArray[2] = CreateTsMuxer;
  mpMuxerCreatorArray[3] = CreateTsMuxer;
  // mpMuxerCreatorArray[4] = CreateTsMuxer;
  mpMuxerCreatorArray[5] = CreateRtspServer;
  mpMuxerCreatorArray[6] = CreateMp4Muxer;
  mpMuxerCreatorArray[7] = CreateJpegMuxer;

  return ME_OK;
}

void *COutputRecord::GetInterface(AM_REFIID refiid)
{
  if (AM_LIKELY(refiid == IID_IOutputRecord)) {
    return (IOutputRecord*) this;
  }

  return NULL;
}

AM_ERR COutputRecord::SetMuxerMask(AmStreamNumber streamNum, AM_UINT muxerMask)
{
  if (streamNum < AM_STREAM_NUMBER_1 || streamNum >= AM_STREAM_NUMBER_MAX) {
    ERROR("No such stream number.\n");
    return ME_ERROR;
  }

  mMuxerMask[streamNum] = muxerMask;
  return ME_OK;
}

AM_ERR COutputRecord::SetMediaSource(AmStreamNumber streamNum,
                                     AmMuxerType muxerType)
{
  int muxerIndex = GetMuxerIndex(muxerType);

  if (AM_UNLIKELY((streamNum < AM_STREAM_NUMBER_1) ||
                  (streamNum >= AM_STREAM_NUMBER_MAX))) {
    ERROR("No such stream number.\n");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(muxerIndex < 0)) {
    ERROR("No such muxer.\n");
    return ME_ERROR;
  }

  mSourceMask[streamNum][muxerIndex] |= 0x1 << (AM_UINT) streamNum;
  return ME_OK;
}

AM_ERR COutputRecord::SetMediaSink(AmStreamNumber streamNum,
                                   AmMuxerType muxerType,
                                   const char *mediaSink)
{
  int muxerIndex;

  if (streamNum < AM_STREAM_NUMBER_1 || streamNum >= AM_STREAM_NUMBER_MAX) {
    ERROR("No such stream number.\n");
    return ME_ERROR;
  }

  if (mediaSink == NULL) {
    ERROR("Media' sink should not be empty.\n");
    return ME_ERROR;
  }

  if ((muxerIndex = GetMuxerIndex(muxerType)) < 0) {
    ERROR("No such muxer.\n");
    return ME_ERROR;
  }

  delete[] mpMediaSink[streamNum][muxerIndex];
  mpMediaSink[streamNum][muxerIndex] = NULL;

  if (AM_UNLIKELY((mpMediaSink[streamNum][muxerIndex] = amstrdup(mediaSink))
      == NULL)) {
    ERROR("Failed to allocate memory to store file name of %dth muxer.\n");
    return ME_ERROR;
  }

  return ME_OK;
}

AM_ERR COutputRecord::SetMaxFileAmount(AmStreamNumber streamNum,
                                       AmMuxerType muxerType,
                                       AM_UINT maxFileAmount)
{
  int muxerIndex;

  if (streamNum < AM_STREAM_NUMBER_1 || streamNum >= AM_STREAM_NUMBER_MAX) {
    ERROR("No such stream number.\n");
    return ME_ERROR;
  }

  if (maxFileAmount == 0) {
    ERROR("maximum file amount should not be zero.\n");
    return ME_ERROR;
  }

  if ((muxerIndex = GetMuxerIndex(muxerType)) < 0) {
    ERROR("No such muxer.\n");
    return ME_ERROR;
  }

  mFileAmount[streamNum][muxerIndex] = maxFileAmount;
  return ME_OK;
}

AM_ERR COutputRecord::SetSplitDuration(AmStreamNumber streamNum,
                                       AmMuxerType muxerType,
                                       AM_UINT sec,
                                       bool alignGOP)
{
  int muxerIndex;

  if (streamNum < AM_STREAM_NUMBER_1 || streamNum >= AM_STREAM_NUMBER_MAX) {
    ERROR("No such stream number.\n");
    return ME_ERROR;
  }

  if ((muxerIndex = GetMuxerIndex(muxerType)) < 0) {
    ERROR("No such muxer.\n");
    return ME_ERROR;
  }

  if (alignGOP) {
    mDuration[streamNum][muxerIndex] = sec * 270000000LL / 2997;
  } else {
    mDuration[streamNum][muxerIndex] = sec * 90000LL;
  }

  INFO("Split duration in 90K base: %lld\n", mDuration[muxerIndex]);
  return ME_OK;
}

IPacketPin *COutputRecord::GetModuleInputPin ()
{
   return mpPacketDistributor->GetInputPin (0);
}

int COutputRecord::GetMuxerIndex (AmMuxerType muxerType)
{
   int muxerIndex = 0;
   AM_UINT muxerMask;

   if (muxerType == 0 || GetOneBitNum (muxerType) != 1) {
      ERROR ("No such muxer: %d\n", muxerType);
      return -1;
   }

   muxerMask = (AM_UINT)muxerType;
   while (muxerMask > 0) {
      muxerMask >>= 1;
      muxerIndex += 1;
   }

   return muxerIndex - 1;
}

AM_UINT COutputRecord::GetMuxerAmount()
{
   int i, muxerAmount;

   for (i = 0, muxerAmount = 0; i < AM_STREAM_NUMBER_MAX; i++) {
      muxerAmount += GetOneBitNum (mMuxerMask[i]);
   }

   return muxerAmount;
}

void COutputRecord::SetRtspAttribute(bool SendNeedWait, bool RtspNeedAuth)
{
  mRtspSendWait = SendNeedWait;
  mRtspNeedAuth = RtspNeedAuth;
}

AM_UINT COutputRecord::GetOneBitNum (AM_UINT num)
{
   AM_UINT oneBitNum = 0;

   /* Counting how many one bits in an integer. */
   while (num > 0) {
      oneBitNum++;
      num &= num - 1;
   }

   return oneBitNum;
}

void COutputRecord::SpecialProcessForRTSPAndRaw ()
{
  int minStreamId = -1;
  int muxerIndex = GetMuxerIndex (AM_MUXER_TYPE_RTSP);

  for (int i = 0; i < AM_STREAM_NUMBER_MAX; ++ i) {
    if ((mMuxerMask[i] & AM_MUXER_TYPE_RTSP) != 0) {
      if (minStreamId == -1) {
        minStreamId = i;
        continue;
      } else {
        mSourceMask[minStreamId][muxerIndex] |= (0x1 << i);
        mSourceMask[i][muxerIndex] = 0;
        mMuxerMask[i] &= ~(AM_MUXER_TYPE_RTSP);
      }
    }
  }

  minStreamId = -1;
  muxerIndex = GetMuxerIndex (AM_MUXER_TYPE_RAW);
  for (int i = 0; i < AM_STREAM_NUMBER_MAX; ++ i) {
    if ((mMuxerMask[i] & AM_MUXER_TYPE_RAW) != 0) {
      if (minStreamId == -1) {
        minStreamId = i;
        continue;
      } else {
        mSourceMask[minStreamId][muxerIndex] |= (0x1 << i);
        mSourceMask[i][muxerIndex] = 0;
        mMuxerMask[i] &= ~(AM_MUXER_TYPE_RAW);
      }
    }
  }
}

AM_ERR COutputRecord::CreateSubGraph()
{
  AM_UINT pinIndex = 0;
  AM_UINT muxerNum = GetMuxerAmount();

  if (muxerNum == 0) {
    ERROR("Muxer mask is not specified.\n");
    return ME_ERROR;
  }

  /* Create data adapter filter */
  if ((mpPacketDistributor = CreatePacketDistributor(mpEngine, muxerNum))
      == NULL) {
    ERROR("Failed to create data adapter.\n");
    return ME_ERROR;
  }

  if (mpFilterGraph->AddFilter(mpPacketDistributor) != ME_OK) {
    ERROR("Failed to add packet distributor to pipeline.\n");
    return ME_ERROR;
  }

  /* try to compress multiple rtsp streams into one. */
  SpecialProcessForRTSPAndRaw();

  /* Create multiple muxer filters based on mMuxerMask */
  for (AM_UINT streamId = 0; streamId < AM_STREAM_NUMBER_MAX; ++ streamId) {
    for (AM_UINT muxerId = 0; muxerId < MUXER_TYPE_AMOUNT; ++ muxerId) {
      if ((mMuxerMask[streamId] & (1 << muxerId)) != 0) {
        MuxerCreator &muxerCreator = mpMuxerCreatorArray[muxerId];
        AM_DELETE(mpMuxerFilter[streamId][muxerId]);
        if ((mpMuxerFilter[streamId][muxerId] = muxerCreator(mpEngine)) ==
            NULL) {
          ERROR("Failed to create %s muxer of %s stream\n",
                pinNames[muxerId], pinNames[streamId]);
          return ME_ERROR;
        }

        if ((mpMuxerArray[streamId][muxerId] =
            IMediaMuxer::GetInterfaceFrom(mpMuxerFilter[streamId][muxerId])) ==
                NULL) {
          ERROR("No muxer interface for %s muxer of %s stream.\n",
                pinNames[muxerId], pinNames[streamId]);
          return ME_ERROR;
        }

        if (AM_LIKELY(0 != (mMuxerMask[streamId] & AM_MUXER_TYPE_RTSP ))) {
          IRtspFilter* rtspFilter =
              IRtspFilter::GetInterfaceFrom(mpMuxerFilter[streamId][muxerId]);
          if (AM_LIKELY(rtspFilter)) {
            rtspFilter->SetRtspAttribute(mRtspSendWait, mRtspNeedAuth);
          }
        }

        if (mpFilterGraph->AddFilter(mpMuxerFilter[streamId][muxerId])
            != ME_OK) {
          ERROR("Failed to add a muxer to pipeline.\n");
          return ME_ERROR;
        }

        /* Connect data adapter and muxer */
        if (mpFilterGraph->Connect(mpPacketDistributor,
                                   pinIndex ++,
                                   mpMuxerFilter[streamId][muxerId],
                                   0) != ME_OK) {
          ERROR("Failed to connect packet distributor and muxer %s.\n",
                ((CPacketActiveFilter*)
                    mpMuxerFilter[streamId][muxerId])->GetName());
          return ME_OK;
        }

        ((CPacketDistributor *)mpPacketDistributor)->\
            SetSourceMask(pinIndex - 1, mSourceMask[streamId][muxerId]);
      }
    }
  }

  return ME_OK;
}

AM_ERR COutputRecord::PrepareToRun ()
{
  for (int streamId = 0; streamId < AM_STREAM_NUMBER_MAX; streamId++) {
    for (int muxerId = 0; muxerId < MUXER_TYPE_AMOUNT; muxerId++) {
      if ((mMuxerMask[streamId] & (1 << muxerId)) != 0) {
        if (mpMuxerArray[streamId][muxerId]->SetSplitDuration (
            mDuration[streamId][muxerId]) != ME_OK) {
          ERROR ("Failed to set split duation for %s muxer of %s stream.\n",
                 pinNames[muxerId], pinNames[streamId]);
          return ME_ERROR;
        }

        if (mpMuxerArray[streamId][muxerId]->SetMediaSourceMask (
            mSourceMask[streamId][muxerId]) != ME_OK) {
          ERROR ("Failed to set source mask for %s muxer of %s stream.\n",
                 pinNames[muxerId], pinNames[streamId]);
          return ME_ERROR;
        }

        if (mpMuxerArray[streamId][muxerId]->SetMaxFileAmount (
            mFileAmount[streamId][muxerId]) != ME_OK) {
          ERROR ("Failed to set maximum file amount for "
                 "%s muxer of %s stream.\n",
                 pinNames[muxerId], pinNames[streamId]);
          return ME_ERROR;
        }

        if (mpMuxerArray[streamId][muxerId]->SetMediaSink(
            (AmSinkType)(1 << muxerId),
            mpMediaSink[streamId][muxerId]) != ME_OK) {
          ERROR ("Failed to set media's sink for %s muxer of %s stream.\n",
                 pinNames[muxerId], pinNames[streamId]);
          return ME_ERROR;
        }
      }
    }
  }

  return ME_OK;
}
