/*******************************************************************************
 * rtp_session.h
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

#ifndef RTP_SESSION_H_
#define RTP_SESSION_H_

#define TCP_HEADER_SIZE       40
#define UDP_HEADER_SIZE       28
#define NTP_OFFSET            2208988800ULL
#define NTP_OFFSET_US         (NTP_OFFSET * 1000000ULL)

enum RtpSessionError {
  AM_RTP_SESSION_ERROR_OK,
  AM_RTP_SESSION_ERROR_UNKNOWN,
  AM_RTP_SESSION_ERROR_NETWORK,
  AM_RTP_SESSION_ERROR_BITSTREAM,
  AM_RTP_SESSION_ERROR_NOSESSION,
  AM_RTP_SESSION_ERROR_INVALIDPKT
};

class CRtspServer;

class CRtpSession
{
  public:
    CRtpSession(AM_UINT streamid, AM_UINT oldStreamId);
    virtual ~CRtpSession();

  public:
    RtpSessionError SendPacket(CPacket *packet);
    bool CheckSessionByName(char* streamName);
    bool CheckSessionByTrackId(char* streamName);
    timespec* GetCreationTime(char* streamName);
    char* GetTrackId();
    char* GetStreamName();
    AM_UINT GetStreamId() {return mStreamId;}
    AM_U32 GetCurrentTimeStamp() {return mCurrentTimeStamp;}
    AM_U16 GetSeqNumber() {return mSeqNum;}
    char* GetSourceAddrString(struct sockaddr_in& clientAddr)
    {
      NetDeviceInfo* devinfo = mRtspServer->FindNetDevInfoByAddr(clientAddr);
      return devinfo ? devinfo->ipv4->get_address_string() : NULL;
    }
    bool AddDestination(Destination *dest, AM_UINT id);
    bool DelDestination(Destination *dest, AM_UINT id);
    bool EnableDesitnation(Destination &dest, AM_UINT id);

  public:
    virtual void GetSdp(char* buf,
                        AM_UINT size,
                        struct sockaddr_in& clientAddr,
                        AM_U16 clientPort = 0) = 0;

  protected:
    enum SessionType {
      AM_RTSP_SESSION_TYPE_NONE,
      AM_RTSP_SESSION_TYPE_H264,
      AM_RTSP_SESSION_TYPE_MJPEG,
      AM_RTSP_SESSION_TYPE_AAC,
      AM_RTSP_SESSION_TYPE_OPUS,
      AM_RTSP_SESSION_TYPE_G726,
    };

    struct SessionInfo {
      char* streamName;
      char* trackId;
      SessionInfo() :
        streamName(NULL),
        trackId(NULL){}
      ~SessionInfo()
      {
        delete[] streamName;
        delete[] trackId;
      }

      void set_stream_name(char* name)
      {
        delete[] streamName;
        streamName = NULL;
        if (AM_LIKELY(name && (strlen(name) > 0))) {
          streamName = amstrdup(name);
        }
      }

      void set_track_id(char* id)
      {
        delete[] trackId;
        trackId = NULL;
        if (AM_LIKELY(id && (strlen(id) > 0))) {
          trackId = amstrdup(id);
        }
      }
    };

  protected:
    AM_ERR Construct(const char* sessionName,
                     CRtspServer *server,
                     SessionType type);
    bool EndSession(AM_UINT id);
    bool HaveDestination(AM_UINT id);
    inline AM_U64 GetNtpTime()
    {
      return (AM_U64)((mRtspServer->GetFakeNtpTime() - mCreationFakeNtp)
          + mCreationTimeUs + NTP_OFFSET_US);
    }
    virtual RtpSessionError SendData(CPacket* packet) = 0;

  protected:
    AM_UINT        mFrameCount;
    AM_UINT        mStreamId;
    AM_UINT        mOldStreamId;
    AM_UINT        mDestNumber;
    AM_U32         mCurrentTimeStamp;
    AM_U16         mSeqNum;
    AM_U32         mMaxPacketSize;
    AM_U32         mSsrc;
    AM_U64         mOldPts;
    AM_U64         mCreationFakeNtp;
    AM_U64         mCreationTimeUs;
    CMutex        *mMutex;
    Destination   *mDestination;
    CRtspServer   *mRtspServer;
    CRtpPackager  *mPackager;
    SessionInfo    mSessionInfo;
    timespec       mCreationTime;
};

#endif /* RTP_SESSION_H_ */
