/*******************************************************************************
 * rtsp_server.h
 *
 * History:
 *   2012-11-12 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef RTSP_SERVER_H_
#define RTSP_SERVER_H_
#include "config.h"
#include <queue>
#include "rtcp_structs.h"

#define  RTSP_SERVER_PORT     554
#define  RTP_STREAM_PORT_BASE 50000

struct RtspTransHeader;
struct Destination;
struct ServerCtrlMsg;

class CRtpSession;
class CRtspFilter;
class CRtspClientSession;

enum RtspStreamMode {
  RTP_TCP,
  RTP_UDP,
  RAW_UDP
};

class CRtspServer
{
    friend class CRtspFilter;
    friend class CRtspClientSession;
    friend class CRtpSession;
    friend class CRtpSessionVideo;
    friend class CRtpSessionAudio;
    enum {
      MAX_CLIENT_NUMBER = 32
    };

  public:
    static CRtspServer* Create(CRtspFilter *filter,
#if defined(BUILD_AMBARELLA_CAMERA_ENABLE_RT) || \
    defined(BUILD_AMBARELLA_CAMERA_ENABLE_RTSP_RT)
                               bool RTPriority = true,
#else
                               bool RTPriority = false,
#endif
                               int priority = CThread::PRIO_LOW);

  public:
    bool start(AM_U16 tcpPort = RTSP_SERVER_PORT);
    void stop();
    bool SendPacket(Destination* &destination, RtspPacket* packet);
    bool SendRtcpSr(Destination* &destination);
    bool SendRtcpBye(Destination* &destination);
    void TrackEos(AM_UINT streamId);
    void SetRtspAttribute(bool needWait, bool needAuth)
    {
      mSendNeedWait = needWait;
      mRtspNeedAuth = needAuth;
    }

  public:
    bool AddSession(AM_UINT streamid,
                    AM_UINT oldstreamid,
                    CPacket::AmPayloadAttr type,
                    void* avInfo);

  public:
    AM_U16 GetServerPortTcp() {return mPortTcp;}
    AM_U32 GetRandomNumber();
    AM_U64 GetFakeNtpTime();

  private:
    static AM_ERR StaticServerThread(void* data)
    {
      return ((CRtspServer*)data)->ServerThread();
    }

  private:
    void ServerAbort();
    bool SendTcp(int *sock, uint8_t *data, uint32_t len);
    bool SendUdp(int *sock, uint8_t *data, uint32_t len, struct sockaddr *addr);

  private:
    bool StartServerThread();
    bool SetupServerSocketTcp(AM_U16 serverPort = RTSP_SERVER_PORT);
    AM_ERR ServerThread();
    void AbortClient(CRtspClientSession& client);
    bool FindClientByAddrAndKill(struct in_addr& addr);
    bool DeleteClientSession(CRtspClientSession** data);
    bool GetDefaultNetworkDevice();
    NetDeviceInfo* FindNetDevInfoByAddr(struct sockaddr_in& addr);

  private:
    bool CheckStreamExistence(char* streamName);
    CRtpSession* GetRtpSessionByName(char* streamName);
    CRtpSession* GetRtpSessionByTrackId(char* trackid);
    CRtpSession* GetRtpSessionById(AM_UINT id);
    bool GetSdp(char* streamName,
                char* buf,
                AM_UINT size,
                struct sockaddr_in& clientAddr,
                AM_U16 clientPort = 0,
                bool trackOnly = false);
    bool GetRtspUrl(char* streamName,
                    char* buf,
                    AM_UINT size,
                    struct sockaddr_in& clientAddr);

  private:
    CRtspServer(CRtspFilter* filter, bool RTPriority, int priority);
    virtual ~CRtspServer();
    AM_ERR Construct();

  private:
    int                 mPipe[2];
    int                 mSrvSockTcp;
    AM_U16              mPortTcp;
    bool                mRun;
    bool                mSendNeedWait;
    bool                mRtspNeedAuth;
    bool                mRTPriority;
    int                 mPriority;
    int                 mHwTimerFd;
    AM_UINT             mTcpSendCnt;
    AM_UINT             mUdpSendCnt;
    AM_U64              mTcpSendSize;
    AM_U64              mUdpSendSize;
    AM_U64              mTcpSendTime;
    AM_U64              mUdpSendTime;
    AM_U64              mTcpAvgSpeed;
    AM_U64              mTcpMaxSpeed;
    AM_U64              mTcpMinSpeed;
    AM_U64              mUdpAvgSpeed;
    AM_U64              mUdpMaxSpeed;
    AM_U64              mUdpMinSpeed;
    CThread            *mThread;
    CEvent             *mEvent;
    CMutex             *mMutex;
    CMutex             *mClientMutex;
    CRtspFilter        *mRtspFilter;
    AmNetworkConfig    *mNetworkConfig;
    NetDeviceInfo      *mNetDeviceInfo;
    NetDeviceInfo      *mDefaultNetdev;
    CRtspClientSession *mClientData[MAX_CLIENT_NUMBER];
    CRtpSession        *mRtpSession[MAX_ENCODE_STREAM_NUM + 1];
    AM_VIDEO_INFO       mVideoInfo[MAX_ENCODE_STREAM_NUM];
    AM_AUDIO_INFO       mAudioInfo;
};

struct Destination
{
  RtspStreamMode                     mode;
  struct in_addr                     address;
  AM_U16                             port;
  AM_U16                             channel; /* Used for TCP */
  AM_U32                             ssrc;
  AM_U8                              sr[sizeof(RtspTcpHeader) +
                                        sizeof(RtcpHeader)    +
                                        sizeof(RtcpSRPayload)];
  AM_U64                             lastsenttimestamp;
  AM_U64                             lastsentntptime;
  AM_U32                             lastsentbytecount;
  bool                               enable;
  bool                               sendsr;
  RtspTcpHeader                     *tcphdr;
  RtcpHeader                        *rtcpHdr;
  RtcpSRPayload                     *srpkt;
  int                               *clientTcpSock;
  int                               *serverUdpRtpSock;
  int                               *serverUdpRtcpSock;
  Destination                       *prev;
  Destination                       *next;

  Destination(struct in_addr& addr,
              AM_U16          tcpRtpPt,
              AM_U16          ch,
              AM_U32          destssrc,
              int            *tcpSock,
              int            *udpRtpSock,
              int            *udpRtcpSock,
              RtspStreamMode  md);
  void close_socket();
  void make_bye_packet();
  void update_sent_data_info(uint32_t timestamp,
                             uint64_t ntptime,
                             uint32_t pktcount,
                             uint32_t datasize);
    void update_rtcp_sr(uint32_t timestamp,
                        uint32_t samplefreq,
                        uint64_t ntptime);
};

bool operator==(Destination &dest1, Destination &dest2);
bool operator!=(Destination &dest1, Destination &dest2);

#endif /* RTSP_SERVER_H_ */
