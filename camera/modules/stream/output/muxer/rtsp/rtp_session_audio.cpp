/*******************************************************************************
 * rtp_session_audio.cpp
 *
 * History:
 *   2012-11-19 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <arpa/inet.h>
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
#include "rtsp_filter.h"
#include "rtsp_server.h"
#include "rtp_session.h"
#include "rtp_session_audio.h"

CRtpSessionAudio* CRtpSessionAudio::Create(AM_UINT        streamid,
                                           AM_UINT        oldstreamid,
                                           CRtspServer   *server,
                                           AM_AUDIO_INFO *audioInfo)
{
  CRtpSessionAudio* result = new CRtpSessionAudio(streamid, oldstreamid);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct(server, audioInfo)))) {
    delete result;
    result = NULL;
  }

  return result;
}

void CRtpSessionAudio::GetSdp(char* buf, AM_UINT size,
                              struct sockaddr_in& clientAddr,
                              AM_U16 clientPort)
{
  if (AM_LIKELY(buf && size > 0)) {
    NetDeviceInfo *devinfo = mRtspServer->FindNetDevInfoByAddr(clientAddr);
    const char *hostAddr = devinfo ?
        devinfo->ipv4->get_address_string() : "0.0.0.0";
    switch(mAudioInfo.format) {
      case MF_AAC: {
        /* We use AAC-LC with max sampling frequency 48000, max 2 channels
         * from IEC-ISO-14496-3 Page34, we can tell this is AAC profile level2,
         * in Page35, table 1.12, we can get that profile-level-id is 0x29(41)
         */
        snprintf(buf, size,
                 "m=audio %hu RTP/AVP %u\r\n"
                 "c=IN IP4 %s\r\n"
                 "a=rtpmap:%u mpeg4-generic/%u/%u\r\n"
                 "a=fmtp:%u streamtype=5; profile-level-id=41; mode=AAC-hbr; "
                 "config=%X; SizeLength=13; IndexLength=3; IndexDeltaLength=3; "
                 "Profile=%u;\r\n"
                 "a=control:%s\r\n",
                 clientPort,            /* SDP port           */
                 RTP_PAYLOAD_TYPE_AAC,  /* RTP payload type   */
                 hostAddr,              /* Host Address       */
                 RTP_PAYLOAD_TYPE_AAC,  /* RTP payload type   */
                 mAudioInfo.sampleRate, /* Sampling Frequency */
                 mAudioInfo.channels,   /* Audio Channel Num  */
                 RTP_PAYLOAD_TYPE_AAC,
                 mPackager->getAacConfigNumber(),
                 (mPackager->getAacConfigNumber() >> 11) - 1,
                 mSessionInfo.trackId);
      }break;
      case MF_OPUS: {
        snprintf(buf, size,
                 "m=audio %hu RTP/AVP %u\r\n"
                 "c=IN IP4 %s\r\n"
                 "a=rtpmap:%u opus/48000/2\r\n"
                 "a=fmtp:%u maxplaybackrate=%u; "
                 "sprop-maxcapturerate=%u; "
                 "stereo=%u; sprop-stereo=%u\r\n"
                 "a=ptime:20\r\n"
                 "a=maxptime:20\r\n"
                 "a=control:%s\r\n",
                 clientPort,            /* SDP port           */
                 RTP_PAYLOAD_TYPE_OPUS, /* RTP payload type   */
                 hostAddr,              /* Host Address       */
                 RTP_PAYLOAD_TYPE_OPUS, /* RTP payload type   */
                 RTP_PAYLOAD_TYPE_OPUS,
                 mAudioInfo.sampleRate, /* Sampling Frequency */
                 mAudioInfo.sampleRate, /* Sampling Frequency */
                 ((mAudioInfo.channels == 1) ? 0 : 1),
                 ((mAudioInfo.channels == 1) ? 0 : 1),
                 mSessionInfo.trackId);
      }break;
      case MF_G726_40:
      case MF_G726_32:
      case MF_G726_24:
      case MF_G726_16: {
        AM_U8 bits = 40;
        switch(mAudioInfo.format) {
          case MF_G726_40:
            bits = 40;
            break;
          case MF_G726_32:
            bits = 32;
            break;
          case MF_G726_24:
            bits = 24;
            break;
          case MF_G726_16:
            bits = 16;
            break;
          default:
            break;
        }
        snprintf(buf, size,
                 "m=audio %hu RTP/AVP %u\r\n"
                 "c=IN IP4 %s\r\n"
                 "a=rtpmap:%u G726-%u/8000/1\r\n"
                 "a=control:%s\r\n",
                 clientPort,            /* SDP port           */
                 RTP_PAYLOAD_TYPE_G726, /* RTP payload type   */
                 hostAddr,              /* Host Address       */
                 RTP_PAYLOAD_TYPE_G726, /* RTP payload type   */
                 bits,
                 mSessionInfo.trackId);
      }break;
      default:
        sprintf(buf, "%s", "");
        break;
    }
  }
}

RtpSessionError CRtpSessionAudio::SendData(CPacket* packet)
{
  RtpSessionError ret = AM_RTP_SESSION_ERROR_OK;
  AM_U32           id = packet->GetStreamId();

  if (AM_LIKELY((id == mStreamId) &&
                ((packet->GetFrameType() == 1 /*AM_AUDIO_CODEC_AAC*/)  ||
                 (packet->GetFrameType() == 2 /*AM_AUDIO_CODEC_OPUS*/) ||
                 (packet->GetFrameType() == 5 /*AM_AUDIO_CODEC_G726*/)))) {
    AM_U8   *data = packet->GetDataPtr();
    AM_U32   size = packet->GetDataSize();
    AM_U32 packetNum = 0;
    RtspPacket *packets = NULL;

    switch(packet->GetFrameType()) {
      case 1 : {
        packets = mPackager->assemblePacketAdts(data, size, mFrameCount,
                                                packetNum, mSeqNum,
                                                mCurrentTimeStamp, mSsrc,
                                                mPktPtsIncr);
      }break;
      case 2 : {
        packets = mPackager->assemblePacketOpus(data, size, mFrameCount,
                                                packetNum, mSeqNum,
                                                mCurrentTimeStamp, mSsrc,
                                                mPktPtsIncr);
      }break;
      case 5 : {
        packets = mPackager->assemblePacketG726(data, size, mFrameCount,
                                                packetNum, mSeqNum,
                                                mCurrentTimeStamp, mSsrc,
                                                mPktPtsIncr);
      }break;
      default:
        break;
    }

    /* Audio stream's id is set to MAX_ENCODE_STREAM_NUM to distinguish it
     * from video stream, before release it we need to reset its stream id
     * to its original one, AVQueue need its original stream id to release
     * memory
     */
    packet->SetStreamId(mOldStreamId);
    packet->Release();

    if (AM_LIKELY(packets)) {
      AUTO_LOCK(mMutex);

      for (AM_UINT i = 0; i < packetNum; ++ i) {

        if (AM_LIKELY(HaveDestination(id))) {

          if (AM_UNLIKELY(!mRtspServer->SendPacket(mDestination,
                                                   &packets[i]))) {
            ERROR("Failed to send audio data");
            ret = AM_RTP_SESSION_ERROR_NETWORK;
            break;
          }
        }
        /* No destination, just ignore */
        ret = AM_RTP_SESSION_ERROR_OK;
      }
      for (Destination *dest = mDestination; dest; dest = dest->next) {
        if (dest->enable) {
          dest->update_sent_data_info(mCurrentTimeStamp,
                                      GetNtpTime(),
                                      packetNum,
                                      size);
          if (AM_UNLIKELY(dest->sendsr)) {
            dest->update_rtcp_sr(mCurrentTimeStamp,
                                 mAudioInfo.sampleRate,
                                 GetNtpTime());
            if (AM_LIKELY(dest->sendsr)) {
              mRtspServer->SendRtcpSr(dest);
              dest->sendsr = false;
            }
          }
        }
      }
    } else {
      ERROR("Failed to build RTP packet!");
      ret = AM_RTP_SESSION_ERROR_BITSTREAM;
    }
  } else {
    if (AM_LIKELY(id != mStreamId)) {
      ERROR("No such session %u", packet->GetStreamId());
      ret = AM_RTP_SESSION_ERROR_NOSESSION;
    } else {
      ret = AM_RTP_SESSION_ERROR_OK;
    }
    /* Audio stream's id is set to MAX_ENCODE_STREAM_NUM to distinguish it
     * from video stream, before release it we need to reset its stream id
     * to its original one, AVQueue need its original stream id to release
     * memory
     */
    packet->SetStreamId(mOldStreamId);
    packet->Release();
  }

  return ret;
}

CRtpSessionAudio::CRtpSessionAudio(AM_UINT streamid, AM_UINT oldstreamid) :
    inherited(streamid, oldstreamid),
    mPktPtsIncr(0)
{
  memset(&mAudioInfo, 0, sizeof(AM_AUDIO_INFO));
}

CRtpSessionAudio::~CRtpSessionAudio()
{
  DEBUG("~CRtpSessionAudio");
}

AM_ERR CRtpSessionAudio::Construct(CRtspServer* server,
                                   AM_AUDIO_INFO* audioInfo)
{
  AM_ERR ret = ME_ERROR;

  if (AM_LIKELY(server && audioInfo)) {
    char trackid[32] = {0};
    SessionType type = AM_RTSP_SESSION_TYPE_NONE;

    memcpy(&mAudioInfo, audioInfo, sizeof(mAudioInfo));
    mPktPtsIncr = (2 * (mAudioInfo.pktPtsIncr * mAudioInfo.sampleRate) + 90000)
        / (2 * 90000);

    snprintf(trackid, sizeof(trackid), "track%u", mStreamId + 1);
    mSessionInfo.set_stream_name((char*)"audio");
    mSessionInfo.set_track_id(trackid);
    switch(mAudioInfo.format) {
      case MF_AAC:  type = AM_RTSP_SESSION_TYPE_AAC; break;
      case MF_OPUS: type = AM_RTSP_SESSION_TYPE_OPUS; break;
      case MF_G726_40:
      case MF_G726_32:
      case MF_G726_24:
      case MF_G726_16: type = AM_RTSP_SESSION_TYPE_G726; break;
      default: ERROR("Invalid audio type: %u", mAudioInfo.format); break;
    }

    ret = inherited::Construct(mSessionInfo.streamName, server, type);
  } else {
    if (AM_LIKELY(!server)) {
      ERROR("Invalid RTSP server!");
    }

    if (AM_LIKELY(!audioInfo)) {
      ERROR("Invalid audio info!");
    }
  }

  return ret;
}
