/*
 * av_queue.cpp
 *
 * History:
 *	2012/8/6 - [Shupeng Ren] created file
 * Copyright (C) 2007-2012, Ambarella, Inc.
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
#include "am_queue.h"
#include "am_mw.h"
#include "am_base.h"
#include "am_mw_packet.h"
#include "am_base_packet.h"
#include "am_media_info.h"
#include "av_queue_if.h"
#include "av_queue_builder.h"
#include "av_queue.h"

#define PACKET_POOL_SIZE 32
/*
 * CSimpeRingPool
 */
CSimpleRingPool* CSimpleRingPool::Create(const char *pName,
                                         AM_UINT     count,
                                         AM_UINT     dataSize,
                                         CAVQueue   *queue)
{
  CSimpleRingPool *result = new CSimpleRingPool(pName, queue);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct(count, dataSize)))) {
    delete result;
    result = NULL;
  }
  return result;
}

void CSimpleRingPool::OnReleaseBuffer(CPacket *pPacket)
{
  if (pPacket->GetType() == CPacket::AM_PAYLOAD_TYPE_INFO) {
    return;
  }
  mpQueue->ReleasePacket(pPacket);
}

/*
 * CAVQueue
 */
IPacketFilter* CreateAVQueue(IEngine *pEngine)
{
  return CAVQueue::Create(pEngine);
}

CAVQueue *CAVQueue::Create(IEngine *pEngine, bool RTPriority, int priority)
{
  CAVQueue *result = new CAVQueue(pEngine);
  if (AM_UNLIKELY(result && (result->Construct(RTPriority,
                                               priority) != ME_OK))) {
    delete result;
    result = NULL;
  }
  return result;
}

CAVQueue::CAVQueue(IEngine *pEngine) :
    inherited(pEngine, "AVQueue"),
    mStop(false),
    mIsVideoBlock(0),
    mIsAudioBlock(0),
    mIsVideoEventBlock(0),
    mIsAudioEventBlock(0),
    mVideoEventEOS(false),
    mIsInEvent(false),
    mFirstVideoPTS(0),
    mFirstAudioPTS(0),
    mEventEndPTS(0),
    mEventSyncFlag(0),
    mRun(0),
    mRunRef(~0),
    mEOSFlag(0),
    mEOSRef(~0),
    mVideoStreamCount(0),
    mAudioStreamCount(0),
    mEventStreamId(0),
    mEventHistoryDuration(10),
    mEventFutureDuration(20),
    mVideoBufferCount(512),
    mAudioBufferCount(512),
    mMaxVideoPacketSize(36*1024),
    mMaxAudioPacketSize(512),
    mVideoQEvt(NULL),
    mAudioQEvt(NULL),
    mVideoEventQEvt(NULL),
    mAudioEventQEvt(NULL),
    mEventWakeup(NULL),
    mEventThread(NULL),
    mpStreamInput(NULL),
    mpAVOutput(NULL),
    mpAVPacketPool(NULL)
{
  for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
    mIsVideoCome[i] = false;
    mpVideoQueue[i] = NULL;
    mpAudioQueue[i] = NULL;
    mVideoEOS[i] = false;
    mAudioEOS[i] = false;
    mVideoInfo[i] = NULL;
    mAudioInfo[i] = NULL;
  }
}

AM_ERR CAVQueue::Construct(bool RTPriority, int priority)
{
  AM_ERR err = inherited::Construct(RTPriority, priority);
  if(err != ME_OK) {
    ERROR("Failed to initialize CPacketActiveFilter!");
    return err;
  }

  if (AM_UNLIKELY((mpAVPacketPool = CSimpleRingPool::Create(
      "buffer_AV_pool", PACKET_POOL_SIZE, sizeof(CPacket), this)) == NULL)) {
    ERROR("Failed to create CSimpleRingPool!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY((mpStreamInput = CAVQueueInput::Create(
      this, "StreamInput")) == NULL)) {
    ERROR("Failed to create StreamInput!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY((mpAVOutput = CAVQueueOutput::Create(this, false)) == NULL)) {
    ERROR("Failed to create AVOutput!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(NULL == (mVideoQEvt = CEvent::Create()))) {
    ERROR("Failed to create video queue event!");
    return ME_ERROR;
  }
  if (AM_UNLIKELY(NULL == (mAudioQEvt = CEvent::Create()))) {
    ERROR("Failed to create audio queue event!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(NULL == (mVideoEventQEvt = CEvent::Create()))) {
    ERROR("Failed to create video event queue event!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(NULL == (mAudioEventQEvt = CEvent::Create()))) {
    ERROR("Failed to create audio event queue event!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(NULL == (mEventWakeup = CEvent::Create()))) {
    ERROR("Failed to create event wake up event!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY((mEventThread = CThread::Create("Process Event",
                                                  EventThread,
                                                  this)) == NULL)) {
    ERROR("Failed to create event thread!");
    return ME_ERROR;
  }

  for (AM_UINT i = 0; i < MUTEX_NUM; ++i) {
    if (AM_UNLIKELY(NULL == (mpMutex[i] = CMutex::Create()))) {
      ERROR("Failed to create mutex[%d]!", i);
      return ME_ERROR;
    }
  }
  mpStreamInput->Enable(true);
  mpAVOutput->SetBufferPool(mpAVPacketPool);
  return ME_OK;
}

void CAVQueue::InitVar()
{
  mStop = false;
  mFirstVideoPTS = 0;
  mRun = 0;
  mRunRef = ~0;
  mEOSFlag = 0;
  mEOSRef = ~0;
  mVideoStreamCount = 0;
  mAudioStreamCount = 0;
  mIsVideoBlock = 0;
  mIsAudioBlock = 0;
  mIsVideoEventBlock = 0;
  mIsAudioEventBlock = 0;
  mVideoEventEOS = false;
  mEventSyncFlag = 0;
  mIsInEvent = false;
  mFirstVideoPTS = 0;
  mEventEndPTS = 0;
  mpStreamInput->Enable(true);
  for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
    mIsVideoCome[i] = false;
    mVideoEOS[i] = false;
    mAudioEOS[i] = false;
  }
}

CAVQueue::~CAVQueue()
{
  AM_DELETE(mpAVOutput);
  AM_DELETE(mpStreamInput);
  AM_DELETE(mVideoQEvt);
  AM_DELETE(mAudioQEvt);
  AM_DELETE(mVideoEventQEvt);
  AM_DELETE(mAudioEventQEvt);
  AM_RELEASE(mpAVPacketPool);
  mEventWakeup->Signal();
  AM_DELETE(mEventThread);
  AM_DELETE(mEventWakeup);
  for (AM_UINT i = 0; i < MUTEX_NUM; ++i) {
    AM_DELETE(mpMutex[i]);
  }
  for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
    AM_DELETE(mpVideoQueue[i]);
    AM_DELETE(mpAudioQueue[i]);
    delete mVideoInfo[i];
    delete mAudioInfo[i];
  }
}

void *CAVQueue::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IAVQueue) {
    return (IAVQueue*)this;
  }
  return inherited::GetInterface(refiid);
}

bool CAVQueue::IsReadyForEvent()
{
  return (!mIsInEvent && mIsVideoCome[mEventStreamId]);
}

void CAVQueue::SetEventStreamId(AM_UINT StreamId)
{
  mEventStreamId = StreamId;
  NOTICE("EventStreamId: %d", mEventStreamId);
}

void CAVQueue::SetEventDuration(AM_UINT history_duration,
                                AM_UINT future_duration)
{
  mEventHistoryDuration = history_duration;
  mEventFutureDuration = future_duration;
  mVideoBufferCount = mEventHistoryDuration * 30 + 300;
  mAudioBufferCount = mEventHistoryDuration * 12 + 120;
  NOTICE("Event History Duration: %d, Future Duration: %d",
         mEventHistoryDuration, mEventFutureDuration);
}

void CAVQueue::GetInfo(INFO& info)
{
  info.nInput = 1;
  info.nOutput = 1;
  info.pName = "AVQueue";
}

IPacketPin* CAVQueue::GetInputPin(AM_UINT index)
{
  if (index == 0) {
    return mpStreamInput;
  } else {
    ERROR("No such input pin %d\n", index);
    return NULL;
  }
}

IPacketPin* CAVQueue::GetOutputPin(AM_UINT index)
{
  if (index == 0) {
    return mpAVOutput;
  } else {
    ERROR("No such output pin %d\n", index);
    return NULL;
  }
}

AM_ERR CAVQueue::Stop()
{
  //mRun = mRunRef;
  mStop = true;
  mpStreamInput->Enable(false);
  inherited::Stop();
  return ME_OK;
}

void CAVQueue::ReleasePacket(CPacket *packet)
{
  AM_U16 StreamId = packet->GetStreamId();
  if ((packet->GetAttr() == CPacket::AM_PAYLOAD_ATTR_AUDIO) &&
      (packet->GetType() != CPacket::AM_PAYLOAD_TYPE_INFO)) {
    mpAudioQueue[StreamId]->AVQRelease((ExPayload*)packet->mPayload);
    if (AM_UNLIKELY(mIsAudioBlock &&
                    mpAudioQueue[StreamId]->IsAVQAboutToEmpty(false))) {
      INFO("Audio normal queue is enough, start reading!");
      __atomic_dec((am_atomic_t*)&mIsAudioBlock);
      mAudioQEvt->Signal();
    }
    if (AM_UNLIKELY(mIsAudioEventBlock)) {
      if (mpAudioQueue[StreamId]->IsAVQAboutToEmpty(true)) {
        INFO("Audio event queue is enough, start reading!");
        __atomic_dec((am_atomic_t*)&mIsAudioEventBlock);
        mAudioEventQEvt->Signal();
      }
      if (!mIsInEvent) {
        INFO("Audio event queue is released, start reading!");
        __atomic_dec((am_atomic_t*)&mIsAudioEventBlock);
        mAudioEventQEvt->Signal();
        mpAudioQueue[mEventStreamId]->InitEvent();
      }
    }
  } else if ((packet->GetAttr() == CPacket::AM_PAYLOAD_ATTR_VIDEO) &&
             (packet->GetType() != CPacket::AM_PAYLOAD_TYPE_INFO)) {
    mpVideoQueue[StreamId]->AVQRelease((ExPayload*)packet->mPayload);
    if (AM_UNLIKELY(mIsVideoBlock &&
                    mpVideoQueue[StreamId]->IsAVQAboutToEmpty(false))) {
      INFO("Video queue is enough, start reading!");
      mVideoQEvt->Signal();
      __atomic_dec((am_atomic_t*)&mIsVideoBlock);
    }
    if (AM_UNLIKELY(mIsVideoEventBlock)) {
      if (mpVideoQueue[StreamId]->IsAVQAboutToEmpty(true)) {
        INFO("Video event queue is enough, start reading!");
        __atomic_dec((am_atomic_t*)&mIsVideoEventBlock);
        mVideoEventQEvt->Signal();
      }
      if (!mIsInEvent) {
        INFO("Video queue is released, start reading!");
        __atomic_dec((am_atomic_t*)&mIsVideoEventBlock);
        mVideoEventQEvt->Signal();
        mpVideoQueue[mEventStreamId]->InitEvent();
      }
    }
  }
}

inline AM_ERR CAVQueue::SendPacket(CPacket *pInPacket)
{
  CPacket *pOutPacket = NULL;
  if (!mpAVOutput->AllocBuffer(pOutPacket)) {
    ERROR("Failed to allocate buffer, data is dropped!");
    return ME_ERROR;
  }
  pOutPacket->mPayload = pInPacket->mPayload;
  mpAVOutput->SendBuffer(pOutPacket);

  return ME_OK;
}

inline AM_ERR CAVQueue::OnInfo(CPacket *pInPacket)
{
  AM_U16 StreamId = pInPacket->GetStreamId();

  switch(pInPacket->GetAttr()) {
    case CPacket::AM_PAYLOAD_ATTR_VIDEO: { /* Video Info */
      AM_VIDEO_INFO *pInfo = (AM_VIDEO_INFO*)(pInPacket->GetDataPtr());

      if (AM_UNLIKELY(!mVideoInfo[StreamId])) {
        mVideoInfo[StreamId] = new AM_VIDEO_INFO;
        if (AM_UNLIKELY(!mVideoInfo[StreamId])) {
          ERROR("Failed to allocate memory for AM_VIDEO_INFO!");
          return ME_ERROR;
        }
      }
      memcpy(mVideoInfo[StreamId], pInfo, sizeof(AM_VIDEO_INFO));
      if (AM_LIKELY(mVideoInfo[StreamId]->needsync)) {
        if (AM_LIKELY(mpVideoQueue[StreamId] == NULL)) {
          if (AM_UNLIKELY((mpVideoQueue[StreamId] =
              CRingPacketPool::Create(mVideoBufferCount,
                                      mMaxVideoPacketSize)) == NULL)) {
            ERROR("Failed to create VideoQueue!");
            return ME_ERROR;
          }
        } else {
          mpVideoQueue[StreamId]->InitVar();
        }
        ++ mVideoStreamCount;
        {
          AUTO_LOCK(mpMutex[0]);
          mRunRef = (mRunRef == (AM_UINT)(~0)) ?
              (1 << StreamId) : (mRunRef | (1 << StreamId));
          mEOSRef = (mEOSRef == (AM_UINT)(~0)) ?
              (1 << StreamId) : (mEOSRef | (1 << StreamId));
        }
        switch(mVideoInfo[StreamId]->type) {
          case 1: { /* H.264 */
            INFO("AVQueue[%hu] received H.264 INFO: size %hux%hu, "
                 "M: %hu, N: %hu",
                 StreamId, pInfo->width, pInfo->height,
                 pInfo->M, pInfo->N);
          }break;
          case 2: { /* MJPEG */
            INFO("AVQueue[%hu] received MJPEG INFO: size %hux%hu",
                 StreamId, pInfo->width, pInfo->height);
          }break;
          default: {
            ERROR("Unknown video type!");
          }break;
        }
      } else {
        AUTO_LOCK(mpMutex[0]);
        mEOSRef = (mEOSRef == (AM_UINT)(~0)) ?
            (1 << StreamId) : (mEOSRef | (1 << StreamId));
        switch(mVideoInfo[StreamId]->type) {
          case 1: { /* H.264 */
            INFO("Stream%u H264 size %ux%u is sent directly down stream!",
                 StreamId, pInfo->width, pInfo->height);
          }break;
          case 2: { /* MJPEG */
            INFO("Stream%u MJPEG size %ux%u is sent directly down stream!",
                 StreamId, pInfo->width, pInfo->height);
          }break;
          default: {
            ERROR("Unknown video type!");
          }
        }
      }
    }break;
    case CPacket::AM_PAYLOAD_ATTR_AUDIO: { /* Audio Info */
      AM_AUDIO_INFO *pInfo = (AM_AUDIO_INFO*)(pInPacket->GetDataPtr());

      if (AM_LIKELY(!mAudioInfo[StreamId])) {
        mAudioInfo[StreamId] = new AM_AUDIO_INFO;
        if (AM_UNLIKELY(!mAudioInfo[StreamId])) {
          ERROR("Failed to allocate memory for AM_AUDIO_INFO");
          return ME_ERROR;
        }
      }
      memcpy(mAudioInfo[StreamId], pInfo, sizeof(AM_AUDIO_INFO));
      if (AM_LIKELY(mAudioInfo[StreamId]->needsync)) {
        switch (mAudioInfo[StreamId]->format) {
          case MF_G726_40:
            mMaxAudioPacketSize = 410;
            break;
          case MF_G726_32:
            mMaxAudioPacketSize = 330;
            break;
          case MF_G726_24:
            mMaxAudioPacketSize = 250;
            break;
          case MF_G726_16:
            mMaxAudioPacketSize = 170;
            break;
          case MF_AAC:
            mMaxAudioPacketSize = 300;
            break;
          case MF_OPUS:
            mMaxAudioPacketSize = 384;
            break;
          case MF_BPCM:
            mMaxAudioPacketSize = 4096;
            break;
          default:
            break;
        }
        if (AM_LIKELY(mpAudioQueue[StreamId] == NULL)) {
          if (AM_UNLIKELY((mpAudioQueue[StreamId] =
              CRingPacketPool::Create(mAudioBufferCount,
                                      mMaxAudioPacketSize)) == NULL)) {
            ERROR("Failed to create AudioQueue!");
            return ME_ERROR;
          }
        } else {
          mpAudioQueue[StreamId]->InitVar();
        }
        ++ mAudioStreamCount;
        {
          AUTO_LOCK(mpMutex[0]);
          mRunRef = (mRunRef == (AM_UINT)(~0)) ?
              (1 << (StreamId + MAX_ENCODE_STREAM_NUM)) :
              (mRunRef | (1 << (StreamId + MAX_ENCODE_STREAM_NUM)));
          mEOSRef = (mEOSRef == (AM_UINT)(~0)) ?
              (1 << (StreamId + MAX_ENCODE_STREAM_NUM)) :
              (mEOSRef | (1 << (StreamId + MAX_ENCODE_STREAM_NUM)));
        }
        INFO("AVQueue[%d] received Audio INFO: SampleRate: %u, Channels %u",
             StreamId, pInfo->sampleRate, pInfo->channels);
      } else {
        AUTO_LOCK(mpMutex[0]);
        mEOSRef = (mEOSRef == (AM_UINT)(~0)) ?
            (1 << (StreamId + MAX_ENCODE_STREAM_NUM)) :
            (mEOSRef | (1 << (StreamId + MAX_ENCODE_STREAM_NUM)));
        INFO("Audio Stream%u is sent directly down stream!", StreamId);
      }
    }break;
    default: {
      ERROR("Invalid Info packet!");
    }break;
  }

  pInPacket->AddRef();
  mpAVOutput->SendBuffer(pInPacket);
  return ME_OK;
}

inline AM_ERR CAVQueue::OnData(CPacket *pInPacket)
{
  AM_ERR ret = ME_OK;
  AM_U16 StreamId = pInPacket->GetStreamId();
  CPacket::Payload *OutAVQInfo = pInPacket->mPayload;
  bool EventFlag = (StreamId == mEventStreamId) ? mIsInEvent : false;

  switch(pInPacket->GetAttr()) {
    case CPacket::AM_PAYLOAD_ATTR_SEI : {
      pInPacket->AddRef();
      mpAVOutput->SendBuffer(pInPacket);
    }break;
    case CPacket::AM_PAYLOAD_ATTR_VIDEO : {

      if (AM_UNLIKELY(mVideoInfo[StreamId]->needsync)) { /* Put into AVQueue */
        /*video input*/
        if (AM_UNLIKELY(pInPacket->GetType() == CPacket::AM_PAYLOAD_TYPE_EOS)) {
          if (AM_LIKELY(mpVideoQueue[StreamId]->IsAVQPayloadFull(false))) {
            WARN("VideoQueue[%hu] is not enough, "
                 "wait for a packet to send EOS!", StreamId);
            __atomic_inc((am_atomic_t*)&mIsVideoBlock);
            mVideoQEvt->Wait();
          }
          if (AM_LIKELY(EventFlag &&
                        mpVideoQueue[StreamId]->IsAVQPayloadFull(true))) {
            WARN("VideoQueue[%hu] is not enough, "
                 "wait for a packet to send EOS!", StreamId);
            __atomic_inc((am_atomic_t*)&mIsVideoEventBlock);
            mVideoEventQEvt->Wait();
          }
          mpVideoQueue[StreamId]->AVQWrite(OutAVQInfo, EventFlag);
        } else if (AM_UNLIKELY(
            mpVideoQueue[StreamId]->IsAVQPayloadFull(false) ||
            (EventFlag && mpVideoQueue[StreamId]->IsAVQPayloadFull(true)))) {
#if 0
          WARN("Video ReadablePayloadcount: %d, ReadPos: %d, WritePos: %d, "
               "InUsed: %d!",
               mpVideoQueue[StreamId]->mReadablePayloadCount_e,
               mpVideoQueue[StreamId]->mReadPos_e,
               mpVideoQueue[StreamId]->mWritePos,
               mpVideoQueue[StreamId]->mInUsedPayload_e);
#endif
          WARN("VideoQueue[%hu] is not enough, drop data %u bytes!",
               StreamId, pInPacket->GetDataSize());
        } else {
          mpVideoQueue[StreamId]->AVQWrite(OutAVQInfo, EventFlag);
        }
      } else { /* Send directly to down stream */
        pInPacket->AddRef();
        mpAVOutput->SendBuffer(pInPacket);
      }
    }break;
    case CPacket::AM_PAYLOAD_ATTR_AUDIO : {
      if (pInPacket->GetType() == CPacket::AM_PAYLOAD_TYPE_EOS) {
        if (mAudioInfo[StreamId]->needsync) {
          if (AM_UNLIKELY(mpAudioQueue[StreamId]->IsAVQPayloadFull(false))) {
            WARN("AudioQueue[%hu] is not enough, "
                "wait for a packet to send EOS!", StreamId);
            __atomic_inc((am_atomic_t*)&mIsAudioBlock);
            mAudioQEvt->Wait();
          }
          if (AM_UNLIKELY(EventFlag &&
                          mpAudioQueue[StreamId]->IsAVQPayloadFull(true))) {
            WARN("AudioQueue[%hu] is not enough, "
                "wait for a packet to send EOS!", StreamId);
            __atomic_inc((am_atomic_t*)&mIsAudioEventBlock);
            mAudioEventQEvt->Wait();
          }
          mpAudioQueue[StreamId]->AVQWrite(OutAVQInfo, EventFlag);
        } else {
          pInPacket->AddRef();
          mpAVOutput->SendBuffer(pInPacket);
        }
      } else {
        for (AM_UINT stream = 0; stream < MAX_ENCODE_STREAM_NUM; ++ stream) {
          bool AEventFlag = (stream == mEventStreamId) ? mIsInEvent : false;
          if (AM_UNLIKELY(!mAudioInfo[stream])) {
            continue;
          }
          if (AM_UNLIKELY(mAudioInfo[stream]->needsync)) {
            /* Put into AVQueue */
            /* audio input */
            if (AM_LIKELY((1 << (stream + MAX_ENCODE_STREAM_NUM)) & mRunRef)) {
              if (AM_UNLIKELY(mpAudioQueue[stream]->IsAVQPayloadFull(false) ||
                              (AEventFlag && mpAudioQueue[stream]->\
                                  IsAVQPayloadFull(true)))) {
#if 0
                WARN("Audio ReadablePayloadcount: %d, ReadPos: %d, "
                     "WritePos: %d, InUsed: %d!",
                     mpAudioQueue[stream]->mReadablePayloadCount,
                     mpAudioQueue[stream]->mReadPos,
                     mpAudioQueue[stream]->mWritePos,
                     mpAudioQueue[stream]->mInUsedPayload);
#endif
                WARN("AudioQueue[%d] is not enough, drop data %u bytes!",
                     stream, pInPacket->GetDataSize());
                continue;
              } else {
                OutAVQInfo->mData.mStreamId = stream;
                mpAudioQueue[stream]->AVQWrite(OutAVQInfo, AEventFlag);
              }
            }
          } else {
            pInPacket->AddRef();
            pInPacket->SetStreamId(stream);
            mpAVOutput->SendBuffer(pInPacket);
          }
        }
      }
    }break;
    default: {
      ERROR("Invalid data type!");
      ret = ME_ERROR;
    }break;
  }

  return ret;
}

inline AM_ERR CAVQueue::OnEvent(CPacket *pInPacket)
{
  CPacket *pOutPacket = NULL;

  if (!mVideoInfo[mEventStreamId]->needsync) {
    ERROR("Stream%d cannot support event!", mEventStreamId);
    return ME_ERROR;
  }
  if (!mpAVOutput->AllocBuffer(pOutPacket)) {
    ERROR("Failed to get buffer!");
    return ME_ERROR;
  }
  pInPacket->SetStreamId(mEventStreamId);
  pOutPacket->mPayload = pInPacket->mPayload;
  mpAVOutput->SendBuffer(pOutPacket);

  if (!mIsVideoCome[mEventStreamId]) {
    return ME_OK;
  }
  mpVideoQueue[mEventStreamId]->InitEvent();
  mpAudioQueue[mEventStreamId]->InitEvent();
  AM_PTS ePTS = pInPacket->GetPTS();
  INFO("EVENT PTS: %lld.", ePTS);
  AM_PTS vPTS_begin = mFirstVideoPTS;
  INFO("First video pts: %lld.", vPTS_begin);
  if (ePTS > (vPTS_begin + mEventHistoryDuration *
              mVideoInfo[mEventStreamId]->scale)) {
    vPTS_begin = ePTS - mEventHistoryDuration *
        mVideoInfo[mEventStreamId]->scale;
  }
  mEventEndPTS = ePTS + mEventFutureDuration *
      mVideoInfo[mEventStreamId]->scale;
  INFO("Event begin PTS: %lld, end PTS: %lld.", vPTS_begin, mEventEndPTS);

  ExPayload *pInVideoQueue = mpVideoQueue[mEventStreamId]->GetAVQPayload(true);
  ExPayload *pInAudioQueue = mpAudioQueue[mEventStreamId]->GetAVQPayload(true);
  if ((pInVideoQueue == NULL) || (pInAudioQueue == NULL)) {
    return ME_OK;
  }
  AM_PTS vPTS = pInVideoQueue->mData.mPayloadPts;
  AM_PTS aPTS = pInAudioQueue->mData.mPayloadPts;
  INFO("Event: current aPTS: %lld, vPTS: %lld.", aPTS, vPTS);

  if (vPTS_begin >= vPTS) {
    return ME_OK;
  }
  //set video ReadPos
  while (true) {
    if ((vPTS > vPTS_begin) ||
        ((vPTS <= vPTS_begin) && (pInVideoQueue->mData.mFrameType != 1))) {
      if (mpVideoQueue[mEventStreamId]->IsAVQPayloadFull(true) ||
          (mpVideoQueue[mEventStreamId]->GetPrevAVQPayload() == NULL)) {
        break;
      }
      pInVideoQueue = mpVideoQueue[mEventStreamId]->GetPrevAVQPayload();
      vPTS = pInVideoQueue->mData.mPayloadPts;
      mpVideoQueue[mEventStreamId]->AVQPayloadBack();
      //set audio ReadPos
      while (true) {
        if ((pInAudioQueue = mpAudioQueue[mEventStreamId]->GetPrevAVQPayload())
            == NULL) {
          break;
        }
        aPTS = pInAudioQueue->mData.mPayloadPts;
        if ((aPTS > vPTS) && (aPTS > mFirstAudioPTS) &&
            !mpAudioQueue[mEventStreamId]->IsAVQPayloadFull(true)) {
          mpAudioQueue[mEventStreamId]->AVQPayloadBack();
        } else {
          break;
        }
      }
      if (aPTS > vPTS) {
        break;
      }
    } else {
      break;
    }
  }
  INFO("Event start: aPTS: %lld, vPTS: %lld, frametype: %d.",
       pInAudioQueue->mData.mPayloadPts,
       pInVideoQueue->mData.mPayloadPts,
       pInVideoQueue->mData.mFrameType);
  mIsInEvent = true;
  mEventWakeup->Signal();

  return ME_OK;
}

AM_ERR CAVQueue::EventThread(void *p)
{
  while (!(((CAVQueue*)p)->mStop)) {
    ((CAVQueue*)p)->mEventWakeup->Wait();
    ((CAVQueue*)p)->ProcessEvent();
  }
  return ME_OK;
}

void CAVQueue::ProcessEvent()
{
  while (true) {
    SendEventPacket();
    if (mVideoEventEOS && (mEventSyncFlag == 0)) {
      mEventSyncFlag = 1;
    }
    if ((mEventSyncFlag == 2) || mStop || !mIsInEvent) {
      mIsInEvent = false;
      mEventSyncFlag = 0;
      mVideoEventEOS = false;
      break;
    }
  }
}

inline AM_ERR CAVQueue::OnEOS(CPacket *pInPacket)
{
  AM_ERR ret = ME_OK;

  NOTICE("Received Stream%u EOS, Attribute: %u, FrameType: %u",
         pInPacket->GetStreamId(),
         pInPacket->GetAttr(),
         pInPacket->GetFrameType());
  if (AM_LIKELY(mRun != mRunRef)) {
    ret = OnData(pInPacket);
    if (pInPacket->GetAttr() == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
      /* video EOS */
      mEOSFlag |= (1 << (pInPacket->GetStreamId()));
    } else if (pInPacket->GetAttr() == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
      /* audio EOS */
      mEOSFlag |= (1 << (pInPacket->GetStreamId() + MAX_ENCODE_STREAM_NUM));
    }
    mStop = ((mEOSFlag == mEOSRef) &&
             (mRun != mRunRef) &&
             (mRunRef == (AM_UINT)(~0)));
  }

  return ret;
}

inline void CAVQueue::SendEventPacket()
{
  ExPayload         *pInVideoQueue = NULL;
  ExPayload         *pInAudioQueue = NULL;
  ExPayload         *pInQueue = NULL;

  if (!mIsVideoCome[mEventStreamId]) {
    mIsInEvent = false;
    return;
  }
  pInVideoQueue = mpVideoQueue[mEventStreamId]->GetAVQPayload(true);
  pInAudioQueue = mpAudioQueue[mEventStreamId]->GetAVQPayload(true);
  if ((pInVideoQueue == NULL) || (pInAudioQueue == NULL)) {
    return;
  }

  if (!mpVideoQueue[mEventStreamId]->IsAVQPayloadEmpty(true) &&
      !mpAudioQueue[mEventStreamId]->IsAVQPayloadEmpty(true)) {
    pInQueue = (pInVideoQueue->mData.mPayloadPts <
                pInAudioQueue->mData.mPayloadPts) ?
                    pInVideoQueue : pInAudioQueue;
    if (pInQueue->mHeader.mPayloadType == CPacket::AM_PAYLOAD_TYPE_EOS) {
      mEventSyncFlag = 2;
      return;
    }
    if ((pInQueue == pInVideoQueue) && !mVideoEventEOS &&
         (pInQueue->mData.mPayloadPts >= mEventEndPTS) &&
           (pInQueue->mData.mFrameType == IDR_FRAME)) {
      mVideoEventEOS = true;
    }
  }
  if (AM_LIKELY(pInQueue)) {
    CPacket *pOutPacket = NULL;
    if (AM_UNLIKELY(!mpAVOutput->AllocBuffer(pOutPacket))) {
      return;
    }
    pOutPacket->mPayload = pInQueue;
    pOutPacket->SetPacketType(CPacket::AM_PACKET_TYPE_EVENT);
    if (mVideoEventEOS && (mEventSyncFlag == 0)) {
      pOutPacket->SetPacketType(CPacket::AM_PACKET_TYPE_EVENT |
                                CPacket::AM_PACKET_TYPE_STOP);
    }
    if (pInQueue == pInVideoQueue) {
      mpVideoQueue[mEventStreamId]->AVQPayloadAdvance(true);
    } else if (pInQueue == pInAudioQueue) {
      mpAudioQueue[mEventStreamId]->AVQPayloadAdvance(true);
    }
    mpAVOutput->SendBuffer(pOutPacket);
  }
}

inline AM_ERR CAVQueue::SortAndSend()
{
  if (AM_UNLIKELY((mVideoStreamCount == 0) ||
                  (mAudioStreamCount == 0) ||
                  (mVideoStreamCount != mAudioStreamCount))) {
    usleep(1000);
    return ME_OK;
  }
  for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
    if (AM_UNLIKELY((((1 << i) & mRunRef) != 0) &&
                    mpVideoQueue[i] &&
                    mpAudioQueue[i])) {
      ExPayload *pInVideoQueue = mpVideoQueue[i]->GetAVQPayload(false);
      ExPayload *pInAudioQueue = mpAudioQueue[i]->GetAVQPayload(false);
      ExPayload *pInQueue      = NULL;

      if (!mpVideoQueue[i]->IsAVQPayloadEmpty(false) &&
          !mpAudioQueue[i]->IsAVQPayloadEmpty(false)) {
        pInQueue = (pInVideoQueue->mData.mPayloadPts <
                    pInAudioQueue->mData.mPayloadPts) ?
                        pInVideoQueue : pInAudioQueue;
        if (AM_UNLIKELY(pInQueue->mHeader.mPayloadType ==
                        CPacket::AM_PAYLOAD_TYPE_EOS)) {
          if (pInQueue == pInVideoQueue) {
            mVideoEOS[i] = true;
            mRun |= (1 << i);
          } else if (pInQueue == pInAudioQueue) {
            mAudioEOS[i] = true;
            mRun |= 1 << (i + MAX_ENCODE_STREAM_NUM);
          }
        }
      } else if (!mpVideoQueue[i]->IsAVQPayloadEmpty(false) && mAudioEOS[i]) {
        pInQueue = pInVideoQueue;
        if (AM_LIKELY(pInVideoQueue->mHeader.mPayloadType ==
                      CPacket::AM_PAYLOAD_TYPE_EOS)) {
          mVideoEOS[i] = true;
          mRun |= (1 << i);
        }
      } else if (!mpAudioQueue[i]->IsAVQPayloadEmpty(false) && mVideoEOS[i]) {
        pInQueue = pInAudioQueue;
        if (AM_LIKELY(pInAudioQueue->mHeader.mPayloadType ==
                      CPacket::AM_PAYLOAD_TYPE_EOS)) {
          mAudioEOS[i] = true;
          mRun |= 1 << (i + MAX_ENCODE_STREAM_NUM);
        }
      }
      if (AM_UNLIKELY(!mIsVideoCome[i])) {
        if (pInQueue == pInVideoQueue) {
          mIsVideoCome[i] = true;
          mFirstVideoPTS = ((mEventStreamId == i) ?
              pInQueue->mData.mPayloadPts : mFirstVideoPTS);
        } else if (pInQueue == pInAudioQueue) {
          mpAudioQueue[i]->AVQPayloadDrop();
          pInQueue = NULL;
        }
      }
      if (!mIsAudioCome[i] && mIsVideoCome[i] && (pInQueue == pInAudioQueue)) {
        mIsAudioCome[i] = true;
        mFirstAudioPTS = ((mEventStreamId == i) ?
            pInQueue->mData.mPayloadPts : mFirstAudioPTS);
        NOTICE("First Audio PTS: %lld", mFirstAudioPTS);
      }
      if (AM_LIKELY(pInQueue)) {
        CPacket *pOutPacket = NULL;
        if (AM_UNLIKELY(!mpAVOutput->AllocBuffer(pOutPacket))) {
          ERROR("Failed to allocate buffer!");
          return ME_ERROR;
        } else {
          pOutPacket->mPayload = pInQueue;
          if ((i == mEventStreamId) &&
              (mEventSyncFlag == 1) &&
              mpVideoQueue[mEventStreamId]->IsAVQEventSync() &&
              (pInQueue->mData.mFrameType == IDR_FRAME)) {
            pOutPacket->SetPacketType(CPacket::AM_PACKET_TYPE_NORMAL |
                                      CPacket::AM_PACKET_TYPE_SYNC);
            mEventSyncFlag = 2;
            NOTICE("Send event stop Packet!");
          }
          AUTO_LOCK(mpMutex[0]);
          if (pInQueue == pInVideoQueue) {
            mpVideoQueue[i]->AVQPayloadAdvance(false);
          } else if (pInQueue == pInAudioQueue) {
            mpAudioQueue[i]->AVQPayloadAdvance(false);
          }
          mpAVOutput->SendBuffer(pOutPacket);
        }
      } else {
        usleep(10000); /* Sleep for 10 ms */
      }
    }
  }

  return ME_OK;
}

AM_ERR CAVQueue::ProcessBuffer(CPacket *pInPacket)
{
  AM_ERR ret = ME_ERROR;
  if (AM_LIKELY(mEOSFlag != mEOSRef)) {
    switch (pInPacket->GetType()) {
      case CPacket::AM_PAYLOAD_TYPE_INFO:
        ret = OnInfo(pInPacket);
        break;

      case CPacket::AM_PAYLOAD_TYPE_DATA:
        ret = OnData(pInPacket);
        break;

      case CPacket::AM_PAYLOAD_TYPE_EOS:
        ret = OnEOS(pInPacket);
        break;

      case CPacket::AM_PAYLOAD_TYPE_EVENT:
        if (pInPacket->GetAttr() == CPacket::AM_PAYLOAD_ATTR_EVENT_EMG &&
            !mIsInEvent) {
          ret = OnEvent(pInPacket);
        }
        break;

      default: {
        ret = ME_OK;
      } break;
    }
  }
  pInPacket->Release();

  return (ret == ME_OK) ? ((mEOSFlag != mEOSRef) ? ME_OK : ME_CLOSED) : ret;
}

void CAVQueue::OnRun()
{
  CmdAck(ME_OK);
  InitVar();
  mpStreamInput->Enable(true);
  mpStreamInput->Run();
  AM_UINT sumcount[MAX_ENCODE_STREAM_NUM] = {0};
  AM_UINT max_video_payload_free_num[MAX_ENCODE_STREAM_NUM] = {0};
  AM_UINT max_audio_payload_free_num[MAX_ENCODE_STREAM_NUM] = {0};
  AM_UINT min_video_payload_free_num[MAX_ENCODE_STREAM_NUM];
  AM_UINT min_audio_payload_free_num[MAX_ENCODE_STREAM_NUM];
  AM_U64  sum_of_video_payload_free_num[MAX_ENCODE_STREAM_NUM] = {0};
  AM_U64  sum_of_audio_payload_free_num[MAX_ENCODE_STREAM_NUM] = {0};

  AM_UINT total_video_mem_size = mMaxVideoPacketSize*mVideoBufferCount;
  AM_UINT total_audio_mem_size = mMaxAudioPacketSize*mAudioBufferCount;
  AM_UINT max_video_mem_free_size[MAX_ENCODE_STREAM_NUM] = {0};
  AM_UINT max_audio_mem_free_size[MAX_ENCODE_STREAM_NUM] = {0};
  AM_UINT min_video_mem_free_size[MAX_ENCODE_STREAM_NUM];
  AM_UINT min_audio_mem_free_size[MAX_ENCODE_STREAM_NUM];
  AM_U64  sum_of_video_mem_free_size[MAX_ENCODE_STREAM_NUM] = {0};
  AM_U64  sum_of_audio_mem_free_size[MAX_ENCODE_STREAM_NUM] = {0};

  for (int i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
    min_video_payload_free_num[i] = -1;
    min_audio_payload_free_num[i] = -1;

    min_video_mem_free_size[i] = -1;
    min_audio_mem_free_size[i] = -1;
  }

  while(!mStop) {
    if (mRun != mRunRef) {
      if (AM_UNLIKELY(ME_OK != SortAndSend())) {
        break;
      }
    } else if ((mRun == mRunRef) && !mIsInEvent && (mEOSFlag == mEOSRef)) {
      assert(mVideoStreamCount == mAudioStreamCount);
      AM_UINT count = 0;
      for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
        if (((1 << i) & mRunRef) == 0) {
          continue;
        }
        if (mpVideoQueue[i] && mpVideoQueue[i]->IsAVQPayloadUsed() &&
            mpAudioQueue[i] && mpAudioQueue[i]->IsAVQPayloadUsed()) {
          ++count;
          continue;
        } else {
          break;
        }
      }
      if (count >= mVideoStreamCount) {
        mStop = true;
        break;
      }
      usleep(1000);
    }

    AM_UINT tempsize = 0;
    for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
      if (((1 << i) & mRunRef) == 0 ||
          (mVideoStreamCount == 0) ||
          (mAudioStreamCount == 0)) {
        continue;
      }
      tempsize = mpVideoQueue[i]->GetFreePayloadCount();
      if (tempsize > max_video_payload_free_num[i]) {
        max_video_payload_free_num[i] = tempsize;
      }
      if (tempsize < min_video_payload_free_num[i]) {
        min_video_payload_free_num[i] = tempsize;
      }
      sum_of_video_payload_free_num[i] += tempsize;

      tempsize = mpAudioQueue[i]->GetFreePayloadCount();
      if (tempsize > max_audio_payload_free_num[i]) {
        max_audio_payload_free_num[i] = tempsize;
      }
      if (tempsize < min_audio_payload_free_num[i]) {
        min_audio_payload_free_num[i] = tempsize;
      }
      sum_of_audio_payload_free_num[i] += tempsize;

      tempsize = mpVideoQueue[i]->GetFreeMemSize();
      if (tempsize > max_video_mem_free_size[i]) {
        max_video_mem_free_size[i] = tempsize;
      }
      if (tempsize < min_video_mem_free_size[i]) {
        min_video_mem_free_size[i] = tempsize;
      }
      sum_of_video_mem_free_size[i] += tempsize;

      tempsize = mpAudioQueue[i]->GetFreeMemSize();
      if (tempsize > max_audio_mem_free_size[i]) {
        max_audio_mem_free_size[i] = tempsize;
      }
      if (tempsize < min_audio_mem_free_size[i]) {
        min_audio_mem_free_size[i] = tempsize;
      }
      sum_of_audio_mem_free_size[i] += tempsize;
      ++sumcount[i];
    }

    for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
      if (((1 << i) & mRunRef) == 0 ||
          sumcount[i] < 10000) {
        continue;
      }
      total_video_mem_size = ROUND_UP(mMaxVideoPacketSize, 4)*mVideoBufferCount;
      total_audio_mem_size = ROUND_UP(mMaxAudioPacketSize, 4)*mAudioBufferCount;
      STAT(
          "\n\n"
          "===============================================================================\n"
          "                                   Stream %d\n"
          "===============================================================================\n"
          "Average data(stat %d times):\n"
          "===============================================================================\n"
          "|       | Total Payload| Free Payload|  free%%| * | Total Mem| Free Mem|  free%%|\n"
          "|Video: |          %4u|         %4llu|%6.2f%%| * |  %8u| %8llu|%6.2f%%|\n"
          "|Audio: |          %4u|         %4llu|%6.2f%%| * |  %8u| %8llu|%6.2f%%|\n"
          "===============================================================================\n"
          "Historical Min Free data:\n"
          "===============================================================================\n"
          "|       | Total Payload| Free Payload|  free%%| * | Total Mem| Free Mem|  free%%|\n"
          "|Video: |          %4u|         %4u|%6.2f%%| * |  %8u| %8u|%6.2f%%|\n"
          "|Audio: |          %4u|         %4u|%6.2f%%| * |  %8u| %8u|%6.2f%%|\n"
          "===============================================================================\n"
          "Historical Max Free data:\n"
          "===============================================================================\n"
          "|       | Total Payload| Free Payload|  free%%| * | Total Mem| Free Mem|  free%%|\n"
          "|Video: |          %4u|         %4u|%6.2f%%| * |  %8u| %8u|%6.2f%%|\n"
          "|Audio: |          %4u|         %4u|%6.2f%%| * |  %8u| %8u|%6.2f%%|\n"
          "===============================================================================\n\n",
          i,
          sumcount[i],
          mVideoBufferCount,
          sum_of_video_payload_free_num[i]/sumcount[i],
          (double)sum_of_video_payload_free_num[i]/sumcount[i]*100/mVideoBufferCount,
          total_video_mem_size,
          sum_of_video_mem_free_size[i]/sumcount[i],
          (double)sum_of_video_mem_free_size[i]/sumcount[i]*100/total_video_mem_size,
          mAudioBufferCount,
          sum_of_audio_payload_free_num[i]/sumcount[i],
          (double)sum_of_audio_payload_free_num[i]/sumcount[i]*100/mAudioBufferCount,
          total_audio_mem_size,
          sum_of_audio_mem_free_size[i]/sumcount[i],
          (double)sum_of_audio_mem_free_size[i]/sumcount[i]*100/total_audio_mem_size,

          //Min
          mVideoBufferCount,
          min_video_payload_free_num[i],
          (float)min_video_payload_free_num[i]*100/mVideoBufferCount,
          total_video_mem_size,
          min_video_mem_free_size[i],
          (float)min_video_mem_free_size[i]*100/total_video_mem_size,
          mAudioBufferCount,
          min_audio_payload_free_num[i],
          (float)min_audio_payload_free_num[i]*100/mAudioBufferCount,
          total_audio_mem_size,
          min_audio_mem_free_size[i],
          (float)min_audio_mem_free_size[i]*100/total_audio_mem_size,

          //Max
          mVideoBufferCount,
          max_video_payload_free_num[i],
          (float)max_video_payload_free_num[i]*100/mVideoBufferCount,
          total_video_mem_size,
          max_video_mem_free_size[i],
          (float)max_video_mem_free_size[i]*100/total_video_mem_size,
          mAudioBufferCount,
          max_audio_payload_free_num[i],
          (float)max_audio_payload_free_num[i]*100/mAudioBufferCount,
          total_audio_mem_size,
          max_audio_mem_free_size[i],
          (float)max_audio_mem_free_size[i]*100/total_audio_mem_size
      );
      sumcount[i] = 0;
      sum_of_video_payload_free_num[i] = 0;
      sum_of_audio_payload_free_num[i] = 0;
      sum_of_video_mem_free_size[i] = 0;
      sum_of_audio_mem_free_size[i] = 0;
    }
  }

  if (AM_LIKELY(mIsVideoBlock)) {
    __atomic_dec((am_atomic_t*)&mIsVideoBlock);
    mVideoQEvt->Signal();
  }
  if (AM_LIKELY(mIsVideoEventBlock)) {
    __atomic_dec((am_atomic_t*)&mIsVideoEventBlock);
    mVideoEventQEvt->Signal();
  }
  if (AM_LIKELY(mIsAudioBlock)) {
    __atomic_dec((am_atomic_t*)&mIsAudioBlock);
    mAudioQEvt->Signal();
  }
  if (AM_LIKELY(mIsAudioEventBlock)) {
    __atomic_dec((am_atomic_t*)&mIsAudioEventBlock);
    mAudioEventQEvt->Signal();
  }
  mpStreamInput->Stop();
  for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
    if (((1 << i) & mRunRef) == 0) {
      continue;
    }
    if (AM_LIKELY(mpAudioQueue[i])) {
      mpAudioQueue[i]->InitVar();
    }
    if (AM_LIKELY(mpVideoQueue[i])) {
      mpVideoQueue[i]->InitVar();
    }
  }
  if (AM_UNLIKELY(!mStop)) {
    ERROR("Fatal error occured in AVQueue!");
    PostEngineMsg(IEngine::MSG_ABORT);
  }
  INFO("AVQueue exit mainloop\n");
}

//-----------------------------------------------------------------------
//
// CAVQueueInput
//
//-----------------------------------------------------------------------
CAVQueueInput* CAVQueueInput::Create(CPacketFilter *pFilter, const char *pName)
{
  CAVQueueInput* result = new CAVQueueInput(pFilter, pName);
  if (result && result->Construct() != ME_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CAVQueueInput::Construct()
{
  return inherited::Construct();
}

inline AM_ERR CAVQueueInput::ProcessBuffer(CPacket *pBuffer)
{
  return ((CAVQueue*)mpFilter)->ProcessBuffer(pBuffer);
}
