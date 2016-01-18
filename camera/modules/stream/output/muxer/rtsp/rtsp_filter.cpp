/*******************************************************************************
 * rtsp_filter.cpp
 *
 * History:
 *   2012-11-9 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_data.h"
#include "am_network.h"

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
#include "am_media_info.h"
#include "output_record_if.h"

#include "rtp_packager.h"
#include "rtsp_server.h"
#include "rtsp_filter.h"
#include "rtp_session.h"
#include "rtp_session_audio.h"
#include "rtp_session_video.h"

IPacketFilter* CreateRtspServer(IEngine* engine)
{
  return CRtspFilter::Create(engine);
}

/*
 * CRtspFilter
 */
CRtspFilter::CRtspFilter(IEngine* engine, bool RTPriority, int priority) :
    inherited(engine, "RtspFilter"),
    mRun(false),
    mIsServerAlive(false),
    mRTPriority(RTPriority),
    mPriority(priority),
    mEosFlag(0),
    mSourceMask(0),
    mMinStream(0),
    mExitState(AM_FILTER_EXIT_STATE_NORMAL),
    mEvent(NULL),
    mMediaInput(NULL),
    mRtspServer(NULL)
{
}

CRtspFilter::~CRtspFilter()
{
  AM_DELETE(mEvent);
  AM_DELETE(mMediaInput);
  delete mRtspServer;
}

CRtspFilter* CRtspFilter::Create(IEngine* engine,
                                 bool RTPriority,
                                 int priority)
{
  CRtspFilter *result = new CRtspFilter(engine, RTPriority, priority);
  if (AM_UNLIKELY(result &&
                  (ME_OK != result->Construct(RTPriority, priority)))) {
    delete result;
    result = NULL;
  }

  return result;
}

AM_ERR CRtspFilter::Construct(bool RTPriority, int priority)
{
  if (AM_UNLIKELY(inherited::Construct (RTPriority, priority) != ME_OK)) {
     ERROR ("Failed to construct RTSP filter's parent.");
     return ME_ERROR;
  }

  if (AM_UNLIKELY(NULL == (mEvent = CEvent::Create()))) {
    ERROR("Failed to create event!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY((mMediaInput = CRtspFilterInput::Create (this)) == NULL)) {
     ERROR  ("Failed to create media input for RTSP filter.");
     return ME_ERROR;
  }

  if (AM_UNLIKELY((mRtspServer = CRtspServer::Create(this,
                                                     RTPriority,
                                                     priority)) == NULL)) {
    ERROR("Failed to create RTSP server thread!");
    return ME_ERROR;
  }

  return ME_OK;
}

void CRtspFilter::SetRtspAttribute(bool needWait, bool needAuth)
{
  mRtspServer->SetRtspAttribute(needWait, needAuth);
}

void* CRtspFilter::GetInterface (AM_REFIID refiid)
{
   if (refiid == IID_IMediaMuxer) {
      return (IMediaMuxer*)this;
   } else if (refiid == IID_IRtspFilter) {
     return (IRtspFilter*)this;
   }
   return inherited::GetInterface (refiid);
}

AM_ERR CRtspFilter::Stop ()
{
   if (mRun) {
      mRun = false;
      inherited::Stop ();
   }

   return ME_OK;
}

void CRtspFilter::GetInfo (INFO& info)
{
   info.nInput = 1;
   info.nOutput = 0;
   info.pName = "Rtsp Filter";
}

IPacketPin* CRtspFilter::GetInputPin (AM_UINT index)
{
  if (AM_UNLIKELY(index != 0)) {
    ERROR("No such input pin: pin[%u]", index);
  }
  return (index == 0) ? mMediaInput : NULL;
}

AM_ERR CRtspFilter::SetMediaSink (AmSinkType sinkType, const char *destStr)
{
   return ME_OK;
}

AM_ERR CRtspFilter::SetSplitDuration (AM_U64 duration)
{
   return ME_OK;
}

AM_ERR CRtspFilter::SetMediaSourceMask (AM_UINT mediaSourceMask)
{
   AM_UINT i = 0;

   if (mediaSourceMask == 0) {
      ERROR ("No video stream is specified.\n");
      return ME_ERROR;
   }

   mSourceMask = mediaSourceMask;
   while ((mediaSourceMask & 0x1) == 0) {
      ++ i;
      mediaSourceMask >>= 1;
   }

   mMinStream = i;
   return ME_OK;
}

inline AM_INT CRtspFilter::OnData(CPacket* packet)
{
  RtpSessionError ret = AM_RTP_SESSION_ERROR_OK;
  if (AM_LIKELY(mIsServerAlive && mRtspServer &&
                mRtspServer->mRtpSession[packet->GetStreamId()])) {
    ret = mRtspServer->mRtpSession[packet->GetStreamId()]->SendPacket(packet);
  } else {
    if (packet->GetAttr() == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
      packet->SetStreamId(mMinStream);
    }
    packet->Release();
  }

  return ((AM_INT)ret);
}

inline AM_ERR CRtspFilter::OnInfo(CPacket* packet)
{
  AM_ERR   ret = ME_OK;
  AM_U16    id = 0;
  AM_U16 oldId = 0;
  void*   info = NULL;

  switch (packet->GetAttr ()) {
    case CPacket::AM_PAYLOAD_ATTR_AUDIO: {
      id = MAX_ENCODE_STREAM_NUM;
      info = packet->GetDataPtr();
      oldId = mMinStream;
      NOTICE ("Audio Info has been received: sample rate %d, "
              "chunk bytes %d, channel %d\n",
              ((AM_AUDIO_INFO *)info)->sampleRate,
              ((AM_AUDIO_INFO *)info)->chunkSize,
              ((AM_AUDIO_INFO *)info)->channels);
    }break;
    case CPacket::AM_PAYLOAD_ATTR_VIDEO: {
      id = packet->GetStreamId();
      info = packet->GetDataPtr();
      oldId = id;
      NOTICE ("Video[%hu] Info has been received: size "
              "%hux%hu, M %hu, N %hu, rate %u, scale %u\n",
              id, ((AM_VIDEO_INFO*)info)->width,
              ((AM_VIDEO_INFO*)info)->height,
              ((AM_VIDEO_INFO*)info)->M,
              ((AM_VIDEO_INFO*)info)->N,
              ((AM_VIDEO_INFO*)info)->rate,
              ((AM_VIDEO_INFO*)info)->scale);
    }break;
    default: {
      ret = ME_ERROR;
      ERROR ("Unknown info");
    }break;
  }

  if (AM_LIKELY(info)) {
    ret = mRtspServer->AddSession(id, oldId, packet->GetAttr(), info) ?
        ME_OK : ME_ERROR;
    packet->SetStreamId(oldId);
  }
  packet->Release();

  return ret;
}

inline void CRtspFilter::OnEos(CPacket *packet)
{
  NOTICE("Received EOS, StreamID %u", packet->GetStreamId());
  if (AM_LIKELY(mIsServerAlive && mRtspServer &&
                mRtspServer->mRtpSession[packet->GetStreamId()])) {
    mRtspServer->mRtpSession[packet->GetStreamId()]->SendPacket(packet);
  } else {
    if (packet->GetAttr() == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
      packet->SetStreamId(mMinStream);
    }
    packet->Release();
  }
}

inline void CRtspFilter::OnRun()
{
  CPacket                *packet = NULL;
  CPacketQueueInputPin *inputPin = NULL;
  bool               isInfoReady = false;
  CmdAck(ME_OK);

  mRun = true;
  mIsServerAlive = false;
  mEosFlag = 0;
  while (mRun) {
    if (AM_UNLIKELY(!mIsServerAlive && isInfoReady)) {
      if (AM_UNLIKELY(false == (mIsServerAlive = mRtspServer->start()))) {
        ERROR("Failed to start RTSP server!");
        mExitState = AM_FILTER_EXIT_STATE_ABORT;
        break;
      }
    }
    packet   = NULL;
    inputPin = NULL;
    if (AM_UNLIKELY(!WaitInputBuffer(inputPin, packet))) {
      if (AM_LIKELY(false == mRun)) {
        NOTICE("All media streams have received EOS, need exit!");
      } else {
        NOTICE("Filter aborted!");
      }
      break;
    }

    if (AM_UNLIKELY(packet->GetPacketType() & CPacket::AM_PACKET_TYPE_EVENT)) {
      /* Filter out all event packet, RTSP doesn't support event */
      packet->Release();
      continue;
    }
    switch(packet->GetType()) {
      case CPacket::AM_PAYLOAD_TYPE_INFO: {
        switch(packet->GetAttr()) {
          case CPacket::AM_PAYLOAD_ATTR_VIDEO:
            mEosFlag |= (1 << packet->GetStreamId());
            break;
          case CPacket::AM_PAYLOAD_ATTR_AUDIO:
            mEosFlag |= (1 << (packet->GetStreamId() + MAX_ENCODE_STREAM_NUM));
            break;
          default:break;
        }
      }break;
      case CPacket::AM_PAYLOAD_TYPE_EOS: {
        switch(packet->GetAttr()) {
          case CPacket::AM_PAYLOAD_ATTR_VIDEO:
            mEosFlag &= ~(1 << packet->GetStreamId());
            break;
          case CPacket::AM_PAYLOAD_ATTR_AUDIO:
            mEosFlag &= ~(1 << (packet->GetStreamId() + MAX_ENCODE_STREAM_NUM));
            break;
          default:break;
        }
      }break;
      default:break;
    }

    mRun = (mEosFlag != 0);
    if (AM_UNLIKELY(!mRun)) {
      mExitState = AM_FILTER_EXIT_STATE_NORMAL;
    }

    if (packet->GetAttr() == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
      if (packet->GetStreamId() != mMinStream) {
        packet->Release();
        continue;
      } else {
        packet->SetStreamId(MAX_ENCODE_STREAM_NUM);
      }
    }

    switch(packet->GetType()) {
      case CPacket::AM_PAYLOAD_TYPE_INFO: {
        mRun = (ME_OK == OnInfo(packet)); /* packet is released in OnInfo */
        if (AM_UNLIKELY(!mRun)) {
          mExitState = AM_FILTER_EXIT_STATE_ABORT;
        }
        isInfoReady = mRun &&
            (packet->GetAttr() == CPacket::AM_PAYLOAD_ATTR_VIDEO);
      }break;
      case CPacket::AM_PAYLOAD_TYPE_DATA: {
        /* packet will be released in RtpSession */
        switch((RtpSessionError)OnData(packet)) {
          case AM_RTP_SESSION_ERROR_NETWORK: {
            /* todo: Post network error to engine */
            ERROR("Network error occurred!");
          }break;
          case AM_RTP_SESSION_ERROR_BITSTREAM: {
            mRun = false;
            mExitState = AM_FILTER_EXIT_STATE_ABORT;
            ERROR("Fatal error occurred! "
                  "need to stop engine to prevent further damage!");
          }break;
          case AM_RTP_SESSION_ERROR_NOSESSION:
          case AM_RTP_SESSION_ERROR_INVALIDPKT:
          case AM_RTP_SESSION_ERROR_OK:
          case AM_RTP_SESSION_ERROR_UNKNOWN:
          default:break;
        }
      }break;
      case CPacket::AM_PAYLOAD_TYPE_EOS: {
        OnEos(packet);  /* packet will be released in RtpSession */
      }break;
      default: {
        ERROR("Unknown packet type: %u", packet->GetType());
        packet->Release();
      }break;
    }
  }

  if (AM_LIKELY(!mRun && mIsServerAlive)) {
    mRtspServer->stop();
  }

  switch (mExitState) {
    case AM_FILTER_EXIT_STATE_NORMAL: {
      PostEngineMsg(IEngine::MSG_EOS);
    }break;
    case AM_FILTER_EXIT_STATE_ABORT: {
      PostEngineMsg(IEngine::MSG_ABORT);
    }
  }

  INFO("RTSP Server exit main loop!");
}

/*
 * CRtspFilterInput
 */

CRtspFilterInput* CRtspFilterInput::Create(CPacketFilter* filter)
{
  CRtspFilterInput *result = new CRtspFilterInput(filter);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }

  return result;
}

AM_ERR CRtspFilterInput::Construct()
{
  return inherited::Construct(((CRtspFilter*)mpFilter)->MsgQ());
}
