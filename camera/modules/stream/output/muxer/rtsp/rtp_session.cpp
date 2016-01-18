/*******************************************************************************
 * rtp_session.cpp
 *
 * History:
 *   2012-11-14 - [ypchang] created file
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
#include "rtp_packager_h264.h"
#include "rtp_packager_mjpeg.h"
#include "rtp_packager_aac.h"
#include "rtp_packager_opus.h"
#include "rtp_packager_g726.h"
#include "rtsp_server.h"
#include "rtp_session.h"
#include "rtsp_filter.h"

CRtpSession::CRtpSession(AM_UINT streamid, AM_UINT oldStreamId) :
  mFrameCount(0),
  mStreamId(streamid),
  mOldStreamId(oldStreamId),
  mDestNumber(0),
  mCurrentTimeStamp(0),
  mSeqNum(0),
  mMaxPacketSize(1448),
  mSsrc(0),
  mOldPts(0),
  mCreationFakeNtp(0),
  mCreationTimeUs(0),
  mMutex(NULL),
  mDestination(NULL),
  mRtspServer(NULL),
  mPackager(NULL)
{
}

CRtpSession::~CRtpSession()
{
  delete mPackager;
  AM_DELETE(mMutex);
  for (Destination *dest = mDestination; dest; dest = dest->next) {
    delete dest;
  }
  DEBUG("~CRtpSession");
}

RtpSessionError CRtpSession::SendPacket(CPacket *packet)
{
  RtpSessionError ret = AM_RTP_SESSION_ERROR_OK;
  switch(packet->GetType()) {
    case CPacket::AM_PAYLOAD_TYPE_EOS: {
      EndSession(packet->GetStreamId());
      mRtspServer->TrackEos(packet->GetStreamId());
      packet->SetStreamId(mOldStreamId);
      packet->Release();
      mOldPts = 0;
    }break;
    case CPacket::AM_PAYLOAD_TYPE_DATA: {
      AM_UINT size = packet->GetDataSize();
      AM_UINT id   = packet->GetStreamId();

      mFrameCount = packet->GetFrameCount();
      /* Packet will be released in SendData() */
      if (AM_UNLIKELY(AM_RTP_SESSION_ERROR_OK != (ret = SendData(packet)))) {
        ERROR("Failed to send data %u bytes, stream%u", size, id);
      }
    }break;
    default: {
      packet->SetStreamId(mOldStreamId);
      packet->Release();
      ERROR("Unknown packet type!");
    }break;
  }

  return ret;
}

bool CRtpSession::CheckSessionByName(char* streamName)
{
  return (streamName && mSessionInfo.streamName &&
      is_str_equal((const char*)streamName,
                   (const char*)mSessionInfo.streamName));
}

bool CRtpSession::CheckSessionByTrackId(char* trackId)
{
  return (trackId && mSessionInfo.trackId &&
      is_str_equal((const char*)trackId, (const char*)mSessionInfo.trackId));
}

timespec* CRtpSession::GetCreationTime(char* streamName)
{
  return (CheckSessionByName(streamName) ? &mCreationTime : NULL);
}

char* CRtpSession::GetTrackId()
{
  return (mSessionInfo.trackId);
}

char* CRtpSession::GetStreamName()
{
  return (mSessionInfo.streamName);
}

bool CRtpSession::AddDestination(Destination *addr, AM_UINT id)
{
  bool ret = false;

  if (AM_LIKELY(addr && (mStreamId == id))) {
    Destination  *prev = NULL;
    Destination **dest = &mDestination;
    while (*dest) {
      if (**dest == *addr) {
        NOTICE("%s:%hu mode %u ssrc %u is already added!",
               inet_ntoa(addr->address), addr->port, addr->mode, addr->ssrc);
        addr->prev = (*dest)->prev;
        addr->next = (*dest)->next;
        (*dest)->next = NULL;
        delete *dest;
        return true;
      } else {
        prev = *dest;
        dest = &((*dest)->next);
      }
    }
    NOTICE("%s:%hu mode %u ssrc 0x%X is added!",
           inet_ntoa(addr->address), addr->port, addr->mode, addr->ssrc);
    *dest = addr;
    addr->prev = prev;
    ret = true;
    ++ mDestNumber;
  } else if (AM_UNLIKELY(addr)) {
    ERROR("No such stream: %u", id);
    delete addr;
  }

  return ret;
}

bool CRtpSession::DelDestination(Destination *target, AM_UINT id)
{
  bool isFound = false;

  AUTO_LOCK(mMutex);
  if (AM_LIKELY(mStreamId == id)) {
    Destination *dest = mDestination;
    while (!isFound && dest) {
      isFound = (*dest == *target);
      if (AM_LIKELY(isFound)) {
        break;
      } else {
        dest = dest->next;
      }
    }

    if (AM_LIKELY(isFound && dest)) {
      -- mDestNumber;
      if (AM_LIKELY(dest->prev)) {
        dest->prev->next = dest->next;
      }
      if (AM_LIKELY(dest->next)) {
        dest->next->prev = dest->prev;
      }
      if (AM_LIKELY(dest == mDestination)) {
        mDestination = dest->next;
      }
      dest->next = NULL;
      /* Send BYE packet to destination */
      mRtspServer->SendRtcpBye(dest);
      dest->close_socket();
      NOTICE("%s:%hu mode %u is deleted!",
             inet_ntoa(dest->address), dest->port, dest->mode);
      delete dest;
    } else if (!isFound) {
      ERROR("Destination %s:%hu mode %u is not added",
            inet_ntoa(target->address), target->port, target->mode);
    }
  }

  return isFound;
}

bool CRtpSession::EnableDesitnation(Destination &dest, AM_UINT id)
{
  bool isFound = false;

  AUTO_LOCK(mMutex);
  if (AM_LIKELY(mStreamId == id)) {
    Destination **addr = &mDestination;
    while (!isFound && *addr) {
      isFound = (**addr == dest);
      if (AM_LIKELY(isFound)) {
        (**addr).enable = true;
        NOTICE("%s:%u mode %u is enabled!",
               inet_ntoa(dest.address), dest.port, dest.mode);
        break;
      } else {
        addr = &((*addr)->next);
      }
    }
  }

  return isFound;
}

AM_ERR CRtpSession::Construct(const char*  sessionName,
                              CRtspServer* server,
                              SessionType  type)
{
  mRtspServer = server;
  if (AM_UNLIKELY(!mRtspServer)) {
    ERROR("Invalid RTSP server!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(NULL == (mMutex = CMutex::Create()))) {
    ERROR("Failed to create mutex!");
    return ME_NO_MEMORY;
  }
  switch(type) {
    case AM_RTSP_SESSION_TYPE_H264:
      mPackager = new CRtpPackagerH264();
      break;
    case AM_RTSP_SESSION_TYPE_MJPEG:
      mPackager = new CRtpPackagerMjpeg();
      break;
    case AM_RTSP_SESSION_TYPE_AAC:
      mPackager = new CRtpPackagerAac();
      break;
    case AM_RTSP_SESSION_TYPE_OPUS:
      mPackager = new CRtpPackagerOpus();
      break;
    case AM_RTSP_SESSION_TYPE_G726:
      mPackager = new CRtpPackagerG726();
      break;
    default:
      break;
  }
  if (AM_UNLIKELY(!mPackager)) {
    ERROR("Failed to create rtp packager!");
    return ME_NO_MEMORY;
  }

  mSsrc = mRtspServer->GetRandomNumber();
  mCurrentTimeStamp = mRtspServer->GetRandomNumber();
  mCreationFakeNtp = mRtspServer->GetFakeNtpTime();
  clock_gettime(CLOCK_REALTIME, &mCreationTime);
  mCreationTimeUs = (((AM_U64)mCreationTime.tv_sec * 1000000000ULL
                     + (AM_U64)mCreationTime.tv_nsec) + 500ULL) / 1000ULL;

  return ME_OK;
}

bool CRtpSession::EndSession(AM_UINT id)
{
  bool ret = false;
  if (AM_LIKELY(id == mStreamId)) {
    /* Disable all the destination attached to this session */
    NOTICE("Track%u received EOS!", id+1);
    AUTO_LOCK(mMutex);
    for (Destination *dest = mDestination; dest; dest = dest->next) {
      dest->enable = false;
      /* Send BYE packet to destination */
      mRtspServer->SendRtcpBye(dest);
      NOTICE("Destination %s:%u is disabled!",
             inet_ntoa(dest->address), dest->port);
    }
    ret = true;
  } else {
    ERROR("No such session %u", id);
  }

  return ret;
}

bool CRtpSession::HaveDestination(AM_UINT id)
{
  return (id == mStreamId) ? (NULL != mDestination) : false;
}
