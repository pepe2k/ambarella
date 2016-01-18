/*******************************************************************************
 * rtsp_video_session.cpp
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
#include "rtp_session_video.h"

CRtpSessionVideo* CRtpSessionVideo::Create(AM_UINT        streamid,
                                           AM_UINT        oldstreamid,
                                           CRtspServer   *server,
                                           AM_VIDEO_INFO *videoInfo)
{
  CRtpSessionVideo* result = new CRtpSessionVideo(streamid, oldstreamid);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct(server, videoInfo)))) {
    delete result;
    result = NULL;
  }

  return result;
}

void CRtpSessionVideo::GetSdp(char* buf, AM_UINT size,
                              struct sockaddr_in& clientAddr,
                              AM_U16 clientPort)
{
  if (AM_LIKELY(buf && size > 0)) {
    AM_U32& fps    = mVideoInfo.fps; /* VIN FPS */
    AM_U32& scale  = mVideoInfo.scale;
    AM_U32& rate   = mVideoInfo.rate;
    AM_U16& mul    = mVideoInfo.mul;
    AM_U16& div    = mVideoInfo.div;
    AM_U32 bitrate = 1000; /* estimate bit rate */
    NetDeviceInfo *devinfo = mRtspServer->FindNetDevInfoByAddr(clientAddr);
    const char *hostAddr = devinfo ?
            devinfo->ipv4->get_address_string() : "0.0.0.0";
    switch(mVideoInfo.type) {
      case 1 : { /* H.264 */
        snprintf(buf, size,
                 "m=video %hu RTP/AVP %u\r\n"
                 "c=IN IP4 %s\r\n"
                 "b=AS:%u\r\n"
                 "a=rtpmap:%u H264/%u\r\n"
                 "a=fmtp:%u packetization-mode=1; profile-level-id=%X; "
                 "sprop-parameter-sets=%s,%s;\r\n"
                 "a=framerate:%s\r\n"
                 "a=cliprect:0,0,%hu,%hu\r\n"
                 "a=control:%s\r\n",
                 clientPort,                     /* SDP port           */
                 RTP_PAYLOAD_TYPE_H264,          /* RTP payload type   */
                 hostAddr,                       /* Host Address       */
                 bitrate,                        /* Estimated bit rate */
                 RTP_PAYLOAD_TYPE_H264,          /* RTP payload type   */
                 90000,                          /* Sampling Frequency */
                 RTP_PAYLOAD_TYPE_H264,          /* RTP payload type   */
                 mPackager->getProfileLevelId(), /* Profile level ID   */
                 mPackager->getSpsString(),      /* Base64 encoded SPS */
                 mPackager->getPpsString(),      /* Base64 encoded PPS */
                 GetVideoFps(rate, scale,
                             mul, div, fps),     /* Video Frame Rate   */
                 mVideoInfo.height,              /* Video Height       */
                 mVideoInfo.width,               /* Video Width        */
                 mSessionInfo.trackId);
      }break;
      case 2: { /* MJPEG */
        snprintf(buf, size,
                 "m=video %hu RTP/AVP %u\r\n"
                 "c=IN IP4 %s\r\n"
                 "b=AS:%u\r\n"
                 "a=rtpmap:%u JPEG/%u\r\n"
                 "a=framerate:%s\r\n"
                 "a=cliprect:0,0,%hu,%hu\r\n"
                 "a=control:%s\r\n",
                 clientPort,                  /* SDP port           */
                 RTP_PAYLOAD_TYPE_JPEG,       /* RTP payload type   */
                 hostAddr,                    /* Host Address       */
                 bitrate,                     /* Estimated bit rate */
                 RTP_PAYLOAD_TYPE_JPEG,       /* RTP payload type   */
                 90000,                       /* Sampling Frequency */
                 GetVideoFps(rate, scale,
                             mul, div, fps),  /* Video Frame Rate   */
                 mVideoInfo.height,           /* Video Height       */
                 mVideoInfo.width,            /* Video Width        */
                 mSessionInfo.trackId);
      }break;
      default: {
        memset(buf, 0, size);
      }break;
    }
  }
}

RtpSessionError CRtpSessionVideo::SendData(CPacket* packet)
{
  RtpSessionError ret = AM_RTP_SESSION_ERROR_OK;
  AM_U32           id = packet->GetStreamId();

  if (AM_LIKELY(id == mStreamId)) {
    AM_U32     packetNum = 0;
    AM_U32          size = packet->GetDataSize();
    AM_U8          *data = packet->GetDataPtr();
    RtspPacket  *packets = NULL;
    AM_S32 pktPtsIncr90k = (mOldPts > 0) ? (AM_S32)(packet->GetPTS()-mOldPts) :
        ((mVideoInfo.type == 1) ? (AM_S32)((AM_U64)mVideoInfo.rate *
            90000LLU / (AM_U64)mVideoInfo.scale) :
            (AM_S32)((AM_U64)mVideoInfo.fps * 90000LLU / 512000000LLU));

    mOldPts = packet->GetPTS();

    switch(mVideoInfo.type) {
      case 1: { /* H.264 */
        packets = mPackager->assemblePacketH264(data, size, mMaxPacketSize,
                                                packetNum, mSeqNum,
                                                mCurrentTimeStamp,
                                                mSsrc, pktPtsIncr90k);
      }break;
      case 2: { /* MJPEG */
        AM_U8 qfactor = (0xff & packet->mPayload->mData.mFrameAttr);
        packets = mPackager->assemblePacketJpeg(data, qfactor, size,
                                                mMaxPacketSize, packetNum,
                                                mSeqNum, mCurrentTimeStamp,
                                                mSsrc, pktPtsIncr90k);
      }break;
      default: {
        ERROR("Invalid video type!");
      }break;
    }
    packet->SetStreamId(mOldStreamId);
    packet->Release();
    if (AM_LIKELY(packets)) {
      AUTO_LOCK(mMutex);

      for (AM_UINT i = 0; i < packetNum; ++ i) {

        if (AM_LIKELY(HaveDestination(id))) {

          if (AM_UNLIKELY(!mRtspServer->SendPacket(mDestination,
                                                   &packets[i]))) {
            ERROR("Failed to send video NALU");
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
                                 90000,
                                 GetNtpTime());
            if (AM_LIKELY(dest->sendsr)) {
              mRtspServer->SendRtcpSr(dest);
              dest->sendsr = false;
            }
          }
        }
      }
    } else {
      ret = AM_RTP_SESSION_ERROR_BITSTREAM;
      ERROR("Failed to build RTP packet!");
    }
  } else {
    ERROR("No such session %u", packet->GetStreamId());
    ret = AM_RTP_SESSION_ERROR_NOSESSION;
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

CRtpSessionVideo::CRtpSessionVideo(AM_UINT streamid, AM_UINT oldstreamid) :
    inherited(streamid, oldstreamid)
{
  memset(mFrameRateStr, 0, sizeof(mFrameRateStr));
}

CRtpSessionVideo::~CRtpSessionVideo()
{
  DEBUG("~CRtpsessionVideo");
}

AM_ERR CRtpSessionVideo::Construct(CRtspServer *server,
                                   AM_VIDEO_INFO *videoInfo)
{
  AM_ERR ret = ME_ERROR;

  if (AM_LIKELY(server && videoInfo)) {
    char streamname[32] = {0};
    char trackid[32] = {0};
    SessionType type = AM_RTSP_SESSION_TYPE_NONE;

    memcpy(&mVideoInfo, videoInfo, sizeof(mVideoInfo));

    snprintf(streamname, sizeof(streamname), "stream%u", mStreamId + 1);
    snprintf(trackid, sizeof(trackid), "track%u", mStreamId + 1);
    mSessionInfo.set_stream_name(streamname);
    mSessionInfo.set_track_id(trackid);
    switch(mVideoInfo.type) {
      case 1: type = AM_RTSP_SESSION_TYPE_H264; break;
      case 2: type = AM_RTSP_SESSION_TYPE_MJPEG; break;
      default: ERROR("Invalid video type: %u", mVideoInfo.type); break;
    }

    ret = inherited::Construct(mSessionInfo.streamName, server, type);
  } else {
    if (AM_LIKELY(!server)) {
      ERROR("Invalid RTSP server!");
    }
    if (AM_LIKELY(!videoInfo)) {
      ERROR("Invalid video info!");
    }
  }

  return ret;
}

const char* CRtpSessionVideo::GetVideoFps(AM_U32& fps)
{
  char *videoFps = NULL;

  memset(mFrameRateStr, 0, sizeof(mFrameRateStr));
  for (uint32_t i = 0; i < sizeof(gFpsList) / sizeof(CameraVinFPS); ++ i) {
    if (gFpsList[i].fpsValue == fps) {
      videoFps = (char *)gFpsList[i].fpsName;
      DEBUG("%u is converted to %s", fps, videoFps);
      break;
    }
  }

  if (AM_UNLIKELY(videoFps == NULL)) {
    if (fps != 0xffffffff) {
      snprintf(mFrameRateStr, sizeof(mFrameRateStr),
               "%.2lf", (double)(512000000/fps));
      videoFps = mFrameRateStr;
    } else {
      videoFps = (char *)gFpsList[0].fpsName;
      WARN("Invalid fps value %u, reset to auto!", fps);
    }
  }

  return videoFps;
}

const char* CRtpSessionVideo::GetVideoFps(AM_U32& rate, AM_U32& scale,
                                          AM_U16& mul,  AM_U16& div,
                                          AM_U32& fps)
{
  char* videoFps = NULL;
  if (AM_LIKELY(mul == div)) {
    videoFps = (char*)GetVideoFps(fps);
  } else if (AM_LIKELY((rate == 0 ) || (scale == 0))) {
    memset(mFrameRateStr, 0, sizeof(mFrameRateStr));
    snprintf(mFrameRateStr, sizeof(mFrameRateStr),
             "%.3lf", ((double)(512000000 * mul) / (double)(fps * div)));
    videoFps = mFrameRateStr;
  } else {
    memset(mFrameRateStr, 0, sizeof(mFrameRateStr));
    snprintf(mFrameRateStr, sizeof(mFrameRateStr), "%.3lf",
             ((double)(scale * mul) / (double)(rate * div)));
    videoFps = mFrameRateStr;
  }

  return videoFps;
}
