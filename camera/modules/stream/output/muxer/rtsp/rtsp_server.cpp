/*******************************************************************************
 * rtsp_server.cpp
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

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <queue>

#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
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
#include "rtsp_requests.h"
#include "rtsp_client_session.h"
#include "rtp_session.h"
#include "rtp_session_audio.h"
#include "rtp_session_video.h"

#define  HW_TIMER ((const char*)"/proc/ambarella/ambarella_hwtimer")
#define  SERVER_CTRL_READ          mPipe[0]
#define  SERVER_CTRL_WRITE         mPipe[1]
#define  SOCK_TIMEOUT_SECOND       5
#define  DATA_SEND_RETRY_TIMES     100
#define  SEND_FAIL_USLEEP_TIME     5000
#define  SEND_STATISTICS_THRESHOLD 20000

#define  SERVER_MAX_LISTEN_NUM     20
#define  RTSP_PARAM_STRING_MAX     2000

#define  CMD_DEL_CLIENT             'd'
#define  CMD_ABRT_ALL               'a'
#define  CMD_STOP_ALL               'e'

struct ServerCtrlMsg
{
    uint32_t code;
    void*    data;
};

CRtspServer* CRtspServer::Create(CRtspFilter *filter,
                                 bool RTPriority,
                                 int priority)
{
  CRtspServer* result = new CRtspServer(filter, RTPriority, priority);
  if (AM_UNLIKELY(result &&
                  (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }

  return result;
}

bool CRtspServer::start(AM_U16 tcpPort)
{
  if (AM_LIKELY(mRun == false)) {
    mRun = StartServerThread() && SetupServerSocketTcp(tcpPort);
    mEvent->Signal();
  }

  return mRun;
}

void CRtspServer::stop()
{
  if (AM_UNLIKELY(false == mRun)) {
    mEvent->Signal();
  }

  while (mRun) {
    if (AM_LIKELY(SERVER_CTRL_WRITE >= 0)) {
      ServerCtrlMsg msg;
      msg.code = CMD_STOP_ALL;
      msg.data = NULL;
      write(SERVER_CTRL_WRITE, &msg, sizeof(msg));
      usleep(30000);
    }
  }

  close(mSrvSockTcp);
  mSrvSockTcp = -1;
  AM_DELETE(mThread);
  for (AM_UINT i = 0; i <= MAX_ENCODE_STREAM_NUM; ++ i) {
    delete mRtpSession[i];
    mRtpSession[i] = NULL;
  }
}

bool CRtspServer::SendPacket(Destination* &destination, RtspPacket* packet)
{
  bool ret = false;

  signal(SIGPIPE, SIG_IGN);
  if (AM_LIKELY(destination)) {
    Destination* dest = NULL;
    for (dest = destination; dest; dest = dest->next) {
      if (AM_LIKELY(dest->enable)) {
        switch(dest->mode) {
          case RTP_TCP: {
            uint8_t *data   = packet->rtsp_tcp_data();
            uint32_t  len   = packet->rtsp_tcp_data_size();
            uint8_t *rtpHdr = packet->rtsp_udp_data();

            data[1]    = dest->channel;
            rtpHdr[8]  = (dest->ssrc & 0xff000000) >> 24;
            rtpHdr[9]  = (dest->ssrc & 0x00ff0000) >> 16;
            rtpHdr[10] = (dest->ssrc & 0x0000ff00) >> 8;
            rtpHdr[11] = (dest->ssrc & 0x000000ff);
            ret = SendTcp(dest->clientTcpSock, data, len);
            if (AM_UNLIKELY(!ret && (*dest->clientTcpSock < 0))) {
              WARN("Connection closed while sending data to %s, channel %hu",
                   inet_ntoa(dest->address), dest->channel);
              dest->enable = false;
              FindClientByAddrAndKill(dest->address);
            }
          }break;
          case RTP_UDP: {
            struct sockaddr_in addr;
            uint8_t *data   = packet->rtsp_udp_data();
            uint32_t  len   = packet->rtsp_udp_data_size();

            data[8]  = (dest->ssrc & 0xff000000) >> 24;
            data[9]  = (dest->ssrc & 0x00ff0000) >> 16;
            data[10] = (dest->ssrc & 0x0000ff00) >> 8;
            data[11] = (dest->ssrc & 0x000000ff);

            memset(&addr, 0, sizeof(addr));
            addr.sin_family      = AF_INET;
            addr.sin_addr.s_addr = dest->address.s_addr;
            addr.sin_port        = htons(dest->port);

            ret = SendUdp(dest->serverUdpRtpSock, data, len,
                          (struct sockaddr*)&addr);
            if (AM_UNLIKELY(!ret && (*dest->serverUdpRtpSock < 0))) {
              WARN("Connection closed while sending data to %s:%hu",
                   inet_ntoa(dest->address), dest->port);
              dest->enable = false;
              FindClientByAddrAndKill(dest->address);
            }
          }break;
          default: {
            ERROR("Not supported!");
          }break;
        }
      } else {
        /* Destination is not enabled, ignore */
        ret = true;
      }
    }
  } else {
    /* No destination, ignore */
    ret = true;
  }

  return ret;
}

bool CRtspServer::SendRtcpSr(Destination* &dest)
{
  bool ret = false;
  signal(SIGPIPE, SIG_IGN);

  if (AM_LIKELY(dest)) {
    switch(dest->mode) {
      case RTP_TCP: {
        ret = SendTcp(dest->clientTcpSock, dest->sr, sizeof(dest->sr));
        if (AM_UNLIKELY(!ret && (*dest->clientTcpSock < 0))) {
          WARN("Connection closed while sending Sender Report to "
               "%s, channel %hu", inet_ntoa(dest->address), dest->channel + 1);
          dest->enable = false;
          FindClientByAddrAndKill(dest->address);
        } else {
          INFO("Send RTCP SR packet %u bytes to %s, channel %hu",
               sizeof(dest->sr), inet_ntoa(dest->address), dest->channel + 1);
        }
      }break;
      case RTP_UDP: {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = dest->address.s_addr;
        addr.sin_port        = htons(dest->port + 1);
        ret = SendUdp(dest->serverUdpRtcpSock,
                      (uint8_t*)dest->rtcpHdr,
                      (sizeof(RtcpHeader) + sizeof(RtcpSRPayload)),
                      (struct sockaddr*)&addr);
        if (AM_UNLIKELY(!ret && (*dest->serverUdpRtcpSock < 0))) {
          WARN("Connection closed while sending Sender Report to %s:%hu",
               inet_ntoa(dest->address), dest->port + 1);
          dest->enable = false;
          FindClientByAddrAndKill(dest->address);
        } else {
          INFO("Send RTCP SR packet %u bytes to %s:%hu",
               (ntohs(dest->rtcpHdr->length) + 1) * sizeof(uint32_t),
               inet_ntoa(dest->address), dest->port + 1);
        }
      }break;
      default: {
        ERROR("Not supported!");
      }break;
    }
    dest->sendsr = false;
  } else {
    /* No destination, ignore */
    ret = true;
  }

  return ret;
}

bool CRtspServer::SendRtcpBye(Destination* &dest)
{
  bool ret = false;
  signal(SIGPIPE, SIG_IGN);

  if (AM_LIKELY(dest)) {
    dest->make_bye_packet();
    switch(dest->mode) {
      case RTP_TCP: {
        if (AM_LIKELY(*dest->clientTcpSock >= 0)) {
          ret = SendTcp(dest->clientTcpSock, dest->sr,
                        sizeof(RtspTcpHeader) + sizeof(RtcpHeader));
          if (AM_UNLIKELY(!ret && (*dest->clientTcpSock < 0))) {
            WARN("Connection closed while sending Sender Report to "
                "%s, channel %hu", inet_ntoa(dest->address), dest->channel + 1);
          } else {
            INFO("Send RTCP BYE packet %u bytes to %s, channel %hu",
                 sizeof(dest->sr), inet_ntoa(dest->address), dest->channel + 1);
          }
        }
      }break;
      case RTP_UDP: {
        if (AM_LIKELY(*dest->serverUdpRtcpSock >= 0)) {
          struct sockaddr_in addr;
          memset(&addr, 0, sizeof(addr));
          addr.sin_family      = AF_INET;
          addr.sin_addr.s_addr = dest->address.s_addr;
          addr.sin_port        = htons(dest->port + 1);
          ret = SendUdp(dest->serverUdpRtcpSock,
                        (uint8_t*)dest->rtcpHdr,
                        sizeof(RtcpHeader),
                        (struct sockaddr*)&addr);
          if (AM_UNLIKELY(!ret && (*dest->serverUdpRtcpSock < 0))) {
            WARN("Connection closed while sending Sender Report to "
                "%s:%hu", inet_ntoa(dest->address), dest->port + 1);
          } else {
            INFO("Send RTCP BYE packet %u bytes to %s:%hu",
                 (ntohs(dest->rtcpHdr->length) + 1) * sizeof(uint32_t),
                 inet_ntoa(dest->address), dest->port + 1);
          }
        }
      }break;
      default: {
        ERROR("Not supported!");
      }break;
    }
    dest->sendsr = false;
  } else {
    /* No destination, ignore */
    ret = true;
  }

  return ret;
}

void CRtspServer::TrackEos(AM_UINT streamId)
{
  AUTO_LOCK(mClientMutex);
  for (AM_UINT i = 0; i < MAX_CLIENT_NUMBER; ++ i) {
    if (AM_LIKELY(mClientData[i])) {
      mClientData[i]->mTrackMap &= ~(1 << streamId);
      NOTICE("Client %s: received track%u EOS, trackmap: %u!",
             mClientData[i]->mClientName,
             streamId + 1, mClientData[i]->mTrackMap);
      if (AM_LIKELY(mClientData[i]->mTrackMap == 0)) {
        if (!mClientData[i]->KillClient()) {
          ERROR("Failed to kill client: %s!", mClientData[i]->mClientName);
        } else {
          while(mClientData[i]->mClientThread->IsThreadRunning()) {
            INFO("Wait for thread of client %s to exit!",
                 mClientData[i]->mClientName);
            usleep(100000);
          }
          NOTICE("Sent stop command to client: %s",
                 mClientData[i]->mClientName);
        }
      }
    }
  }
}

bool CRtspServer::AddSession(AM_UINT streamid,
                             AM_UINT oldstreamid,
                             CPacket::AmPayloadAttr attr,
                             void* avInfo)
{
  bool ret = false;
  if (AM_LIKELY((streamid < (sizeof(mRtpSession) / sizeof(CRtpSession*))) &&
                avInfo)) {
    CRtpSession*& rtpSession = mRtpSession[streamid];
    delete rtpSession;
    rtpSession = NULL;
    switch(attr) {
      case CPacket::AM_PAYLOAD_ATTR_AUDIO: {
        memcpy(&mAudioInfo, avInfo, sizeof(mAudioInfo));
        rtpSession = CRtpSessionAudio::Create(streamid,
                                              oldstreamid,
                                              this,
                                              &mAudioInfo);
      }break;
      case CPacket::AM_PAYLOAD_ATTR_VIDEO: {
        memcpy(&mVideoInfo[streamid], avInfo, sizeof(mVideoInfo[streamid]));
        rtpSession = CRtpSessionVideo::Create(streamid,
                                              oldstreamid,
                                              this,
                                              &mVideoInfo[streamid]);
      }break;
      default: {
        ERROR("Unknown session type!");
      }break;
    }

    if (AM_UNLIKELY(!rtpSession)) {
      ERROR("Failed to create rtp session for stream%u", streamid);
    } else {
      ret = true;
    }

  } else if (AM_LIKELY(!avInfo)) {
    ERROR("Invalid INFO packet!");
  } else {
    ERROR("Invalid stream ID: %u", streamid);
  }

  return ret;
}

AM_U32 CRtspServer::GetRandomNumber()
{
  struct timeval current = {0};
  gettimeofday(&current, NULL);
  srand(current.tv_usec);
  return (rand() % ((AM_U32)-1));
}

AM_U64 CRtspServer::GetFakeNtpTime()
{
  AM_U64 ntptime = 0;
  AM_U8 pts[32] = {0};
  if (AM_LIKELY(mHwTimerFd >= 0)) {
    if (AM_UNLIKELY(read(mHwTimerFd, pts, sizeof(pts)) < 0)) {
      PERROR("read");
    } else {
      ntptime = strtoull((const char*)pts, (char **)NULL, 10) * 1000000 / 90000;
    }
  }

  return ntptime;
}

void CRtspServer::ServerAbort()
{
  if (AM_UNLIKELY(!mRun)) {
    mEvent->Signal();
  }

  while (mRun) {
    if (AM_LIKELY(SERVER_CTRL_WRITE >= 0)) {
      ServerCtrlMsg msg;
      msg.code = CMD_ABRT_ALL;
      msg.data = NULL;
      write(SERVER_CTRL_WRITE, &msg, sizeof(msg));
      usleep(30000);
    }
  }
}

bool CRtspServer::SendTcp(int *sock, uint8_t *data, uint32_t len)
{
  bool        ret = false;
  int     sendRet = -1;
  int   totalSent = 0;
  int reSendCount = 0;
  struct timeval start = {0, 0};
  struct timeval end   = {0, 0};

  if (AM_UNLIKELY(gettimeofday(&start, NULL)) < 0) {
    PERROR("gettimeofday");
  }
  do {
    if (AM_LIKELY(*sock >= 0)) {
      sendRet = send(*sock, data + totalSent,
                     len - totalSent, (mSendNeedWait ? 0 : MSG_DONTWAIT));
      if (AM_UNLIKELY(sendRet < 0)) {
        if (AM_LIKELY((errno != EAGAIN) &&
                      (errno != EWOULDBLOCK) &&
                      (errno != EINTR))) {
          close(*sock);
          *sock = -1;
          ret = false;
          break;
        }
      } else {
        totalSent += sendRet;
      }
      ret = ((len - totalSent) == 0);
      if (AM_UNLIKELY(!ret)) {
        if (AM_LIKELY(++ reSendCount >= DATA_SEND_RETRY_TIMES)) {
          ERROR("Network condition is bad, tried %d times, "
                "%d bytes sent, %u bytes dropped!",
                reSendCount, totalSent, (len - totalSent));
#if 0
          close(*sock);
          *sock = -1;
#endif
          break;
        }
        usleep(SEND_FAIL_USLEEP_TIME);
      }
    } else {
      ERROR("Invalid socket!");
      break;
    }
  }while(!ret);

  if (AM_UNLIKELY(gettimeofday(&end, NULL)) < 0) {
    PERROR("gettimeofday");
  } else if (AM_LIKELY(*sock >= 0)) {
    AM_U64 timediff = (end.tv_sec * 1000000 + end.tv_usec) -
                      (start.tv_sec * 1000000 + start.tv_usec) -
                      (SEND_FAIL_USLEEP_TIME *
                          ((reSendCount >= DATA_SEND_RETRY_TIMES) ?
                          (reSendCount - 1) : reSendCount));
    ++ mTcpSendCnt;
    mTcpSendSize += totalSent;
    mTcpSendTime += timediff;
    if (AM_UNLIKELY(timediff >= 200000)) {
      WARN("Send %d bytes takes %llu ms!", totalSent, timediff / 1000);
    }
    if (AM_UNLIKELY(mTcpSendCnt >= SEND_STATISTICS_THRESHOLD)) {
      mTcpAvgSpeed = mTcpSendSize * 1000000 / mTcpSendTime / 1024;
      mTcpMaxSpeed = (mTcpAvgSpeed > mTcpMaxSpeed) ?
          mTcpAvgSpeed : mTcpMaxSpeed;
      mTcpMinSpeed = (mTcpAvgSpeed < mTcpMinSpeed) ?
          mTcpAvgSpeed : mTcpMinSpeed;
      STAT("\nSend %u times, %llu bytes data sent through TCP, takes %llu ms"
           "\n       Average speed %llu KB/sec"
           "\nHistorical min speed %llu KB/sec"
           "\nHistorical max speed %llu KB/sec",
           mTcpSendCnt, mTcpSendSize, mTcpSendTime / 1000,
           mTcpAvgSpeed, mTcpMinSpeed, mTcpMaxSpeed);
      mTcpSendCnt = 0;
      mTcpSendSize = 0;
      mTcpSendTime = 0;
    }
  }

  return ret;
}

bool CRtspServer::SendUdp(int *sock, uint8_t *data,
                          uint32_t len, struct sockaddr *addr)
{
  bool        ret = false;
  int     sendRet = -1;
  int   totalSent = 0;
  int reSendCount = 0;
  struct timeval start = {0, 0};
  struct timeval end   = {0, 0};

  if (AM_UNLIKELY(gettimeofday(&start, NULL)) < 0) {
    PERROR("gettimeofday");
  }

  do {
    if (AM_LIKELY(*sock >= 0)) {
      sendRet = sendto(*sock, data + totalSent, len - totalSent,
                       (mSendNeedWait ? 0 : MSG_DONTWAIT),
                       addr, sizeof(struct sockaddr));
      if (AM_UNLIKELY(sendRet < 0)) {
        if (AM_LIKELY((errno != EAGAIN) &&
                      (errno != EWOULDBLOCK) &&
                      (errno != EINTR))) {
          close(*sock);
          *sock = -1;
          ret = false;
          break;
        }
      } else {
        totalSent += sendRet;
      }
      ret = ((len - totalSent) == 0);
      if (AM_UNLIKELY(!ret)) {
        if (AM_LIKELY(++ reSendCount >= DATA_SEND_RETRY_TIMES)) {
          ERROR("Network condition is bad, tried %d times, "
                "%d bytes sent, %u bytes dropped!",
                reSendCount, totalSent, (len - totalSent));
#if 0
          close(*sock);
          *sock = -1;
#endif
          break;
        }
        usleep(SEND_FAIL_USLEEP_TIME);
      }
    } else {
      ERROR("Invalid socket!");
      break;
    }
  }while (!ret);

  if (AM_UNLIKELY(gettimeofday(&end, NULL)) < 0) {
    PERROR("gettimeofday");
  } else if (AM_LIKELY(*sock >= 0)) {
    AM_U64 timediff = (end.tv_sec * 1000000 + end.tv_usec) -
                      (start.tv_sec * 1000000 + start.tv_usec) -
                      (SEND_FAIL_USLEEP_TIME *
                          ((reSendCount >= DATA_SEND_RETRY_TIMES) ?
                          (reSendCount - 1) : reSendCount));
    ++ mUdpSendCnt;
    mUdpSendSize += totalSent;
    mUdpSendTime += timediff;
    if (AM_UNLIKELY(timediff >= 200000)) {
      WARN("Send %d bytes takes %llu ms!", totalSent, timediff / 1000);
    }
    if (AM_UNLIKELY(mUdpSendCnt >= SEND_STATISTICS_THRESHOLD)) {
      mUdpAvgSpeed = mUdpSendSize * 1000000 / mUdpSendTime / 1024;
      mUdpMaxSpeed = (mUdpAvgSpeed > mUdpMaxSpeed) ?
          mUdpAvgSpeed : mUdpMaxSpeed;
      mUdpMinSpeed = (mUdpAvgSpeed < mUdpMinSpeed) ?
          mUdpAvgSpeed : mUdpMinSpeed;
      STAT("\nSend %u times, %llu bytes data sent through UDP, takes %llu ms"
           "\n       Average speed %llu KB/sec"
           "\nHistorical min speed %llu KB/sec"
           "\nHistorical max speed %llu KB/sec",
           mUdpSendCnt, mUdpSendSize, mUdpSendTime / 1000,
           mUdpAvgSpeed, mUdpMinSpeed, mUdpMaxSpeed);
      mUdpSendCnt = 0;
      mUdpSendSize = 0;
      mUdpSendTime = 0;
    }
  }

  return ret;
}

bool CRtspServer::StartServerThread()
{
  bool ret = true;

  AM_DELETE(mThread);
  if (AM_UNLIKELY(NULL == (mThread = CThread::Create("RtspServer",
                                                     StaticServerThread,
                                                     this)))) {
    ERROR("Failed to create RTSP Server thread!");
    ret = false;
  } else if (AM_UNLIKELY(mRTPriority &&
                         (ME_OK != mThread->SetRTPriority(mPriority)))) {
    ERROR("Failed to set RTSP server to real-time thread!");
    ret = false;
  }

  return ret;
}

bool CRtspServer::SetupServerSocketTcp(AM_U16 port)
{
  bool ret  = false;
  int reuse = 1;
  struct sockaddr_in serverAddr;

  mPortTcp = port;
  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family      = AF_INET;
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddr.sin_port        = htons(mPortTcp);

  if (AM_UNLIKELY((mSrvSockTcp = socket(AF_INET, SOCK_STREAM,
                                        IPPROTO_TCP)) < 0)) {
    PERROR("socket");
  } else {
    if (AM_LIKELY(setsockopt(mSrvSockTcp, SOL_SOCKET,
                             SO_REUSEADDR, &reuse, sizeof(reuse)) == 0)) {
      AM_INT flag = fcntl(mSrvSockTcp, F_GETFL, 0);
      flag |= O_NONBLOCK;

      if (AM_UNLIKELY(fcntl(mSrvSockTcp, F_SETFL, flag) != 0)) {
        PERROR("fcntl");
      } else if (AM_UNLIKELY(bind(mSrvSockTcp,
                                  (struct sockaddr*)&serverAddr,
                                  sizeof(serverAddr)) != 0)){
        PERROR("bind");
      } else if (AM_UNLIKELY(listen(mSrvSockTcp, SERVER_MAX_LISTEN_NUM) < 0)) {
        PERROR("listen");
      } else {
        ret = true;
      }
    } else {
      PERROR("setsockopt");
    }
  }

  return ret;
}

AM_ERR CRtspServer::ServerThread()
{
  AM_ERR ret = ME_OK;
  int maxfd = -1;
  fd_set allset;
  fd_set fdset;
  mEvent->Wait();

  FD_ZERO(&allset);
  FD_SET(mSrvSockTcp, &allset);
  FD_SET(SERVER_CTRL_READ, &allset);

  maxfd = AM_MAX(mSrvSockTcp, SERVER_CTRL_READ);

  signal(SIGPIPE, SIG_IGN);
  while(mRun) {
    fdset = allset;

    if (AM_LIKELY(select(maxfd + 1, &fdset, NULL, NULL, NULL) > 0)) {
      if (FD_ISSET(SERVER_CTRL_READ, &fdset)) {
        ServerCtrlMsg msg = {0};
        AM_UINT   readCnt = 0;
        ssize_t   readRet = 0;
        do {
          readRet = read(SERVER_CTRL_READ, &msg, sizeof(msg));
        } while ((++ readCnt < 5) && ((readRet == 0) || ((readRet < 0) &&
                 ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
                  (errno == EINTR)))));
        if (AM_UNLIKELY(readRet < 0)) {
          PERROR("read");
        } else {
          if (msg.code == CMD_DEL_CLIENT) {
            AUTO_LOCK(mClientMutex);
            CRtspClientSession** client = (CRtspClientSession**)msg.data;
            if (AM_LIKELY(*client)) {
              NOTICE("RTSP server is asked to delete client %s",
                     (*client)->mClientName);
              delete *client;
            }
          } else if ((msg.code == CMD_ABRT_ALL) || (msg.code == CMD_STOP_ALL)) {
            AUTO_LOCK(mClientMutex);
            mRun = false;
            close(mSrvSockTcp);
            mSrvSockTcp = -1;
            for (AM_UINT i = 0; i < MAX_CLIENT_NUMBER; ++ i) {
              if (AM_LIKELY(mClientData[i])) {
                AbortClient(*mClientData[i]);
                delete mClientData[i];
              }
            }
            switch(msg.code) {
              case CMD_ABRT_ALL :
                ERROR("Fatal error occurred, RTSP server abort!");
              break;
              case CMD_STOP_ALL :
                NOTICE("RTSP server received stop signal!");
              break;
              default: break;
            }
            mRtspFilter->Abort();
            break;
          }
        }
      } else if (FD_ISSET(mSrvSockTcp, &fdset)) {
        AUTO_LOCK(mClientMutex);
        CRtspClientSession** self = NULL;
        for (AM_UINT i = 0; i < MAX_CLIENT_NUMBER; ++ i) {
          if (AM_LIKELY(!mClientData[i])) {
            self = &mClientData[i];
            *self = new CRtspClientSession(this, self,
                                           (RTP_STREAM_PORT_BASE + i * 2));
            break;
          }
        }
        if (AM_UNLIKELY(!(*self))) {
          ERROR("Maximum client number has reached!");
        } else if (AM_UNLIKELY(!(*self)->InitClientSession(mSrvSockTcp))) {
          ERROR("Failed to initialize client session");
          if (AM_LIKELY(*self)) {
            AbortClient(**self);
            delete *self;
          }
        }
      }
    } else {
      PERROR("select");
      break;
    }
  }

  if (AM_UNLIKELY(mRun)) {
    ERROR("RTSP server exited abnormally!");
    ret = ME_ERROR;
  } else {
    INFO("RTSP server exit mainloop!");
  }

  return ret;
}

void CRtspServer::AbortClient(CRtspClientSession& client)
{
  RtspRequest rtspReq;
  rtspReq.set_command((char*)"TEARDOWN");

  client.HandleClientRequest(rtspReq);
  client.ClientAbort();
  while(client.mClientThread->IsThreadRunning()) {
    NOTICE("Wait for client %s to exit!", client.mClientName);
    usleep(10000);
  }
}

bool CRtspServer::FindClientByAddrAndKill(struct in_addr& addr)
{
  AUTO_LOCK(mClientMutex);
  CRtspClientSession* client = NULL;
  for (AM_UINT i = 0; i < MAX_CLIENT_NUMBER; ++ i) {
    client = mClientData[i];
    if (AM_LIKELY(client && (addr.s_addr ==
        client->mClientAddr.sin_addr.s_addr))) {
      break;
    }
  }

  return (client ? client->KillClient() : true);
}

bool CRtspServer::DeleteClientSession(CRtspClientSession** data)
{
  ServerCtrlMsg msg;
  msg.code = CMD_DEL_CLIENT;
  msg.data = ((void*)data);
  return ((ssize_t)sizeof(msg) == write(SERVER_CTRL_WRITE, &msg, sizeof(msg)));
}

bool CRtspServer::GetDefaultNetworkDevice()
{
  AUTO_LOCK(mMutex);
  bool ret = false;

  if (AM_LIKELY(mNetworkConfig)) {
    delete mNetDeviceInfo;
    mNetDeviceInfo = NULL;
    mDefaultNetdev = NULL;

    if (AM_LIKELY(mNetworkConfig->get_default_connection(&mNetDeviceInfo))) {
      mDefaultNetdev = mNetDeviceInfo;

      while (!ret && mDefaultNetdev) {
        ret = mDefaultNetdev->is_default;
        if (AM_LIKELY(!ret)) {
          mDefaultNetdev = mDefaultNetdev->info_next;
        }
      }
      if (AM_UNLIKELY(!ret)) {
        mDefaultNetdev = mNetDeviceInfo;
      }
      if (AM_UNLIKELY(!mDefaultNetdev)) {
        ERROR("Default network device is not found!");
        ret = false;
      } else {
        ret = true;
      }
    }
  }

  return ret;
}

NetDeviceInfo* CRtspServer::FindNetDevInfoByAddr(struct sockaddr_in& address)
{
  NetDeviceInfo* devinfo = NULL;
  if (AM_LIKELY(GetDefaultNetworkDevice())) {
    devinfo = mNetDeviceInfo;

    while (devinfo) {
      const uint32_t& mask = devinfo->ipv4->get_netmask();
      const uint32_t& addr = devinfo->ipv4->get_address();

      if (AM_LIKELY((mask & addr) == (mask & address.sin_addr.s_addr))) {
        break;
      }
      devinfo = devinfo->info_next;
    }

    if (AM_UNLIKELY(!devinfo)) {
      devinfo = mDefaultNetdev;
    }
  } else {
    ERROR("Network is down!");
  }

  return devinfo;
}

bool CRtspServer::CheckStreamExistence(char* streamName)
{
  bool ret = false;
  for (AM_UINT i = 0; i < (MAX_ENCODE_STREAM_NUM + 1); ++ i) {
    ret = (mRtpSession[i] && mRtpSession[i]->CheckSessionByName(streamName));
    if (AM_LIKELY(ret)) {
      break;
    }
  }

  return ret;
}

CRtpSession* CRtspServer::GetRtpSessionByName(char* streamName)
{
  CRtpSession* session = NULL;

  for (AM_UINT i = 0; i < (MAX_ENCODE_STREAM_NUM + 1); ++ i) {

    if (AM_LIKELY(mRtpSession[i] &&
                  mRtpSession[i]->CheckSessionByName(streamName))) {
      session = mRtpSession[i];
      break;
    }
  }

  return session;
}

CRtpSession* CRtspServer::GetRtpSessionByTrackId(char* trackid)
{
  CRtpSession* session = NULL;

  for (AM_UINT i = 0; i < (MAX_ENCODE_STREAM_NUM + 1); ++ i) {

    if (AM_LIKELY(mRtpSession[i] &&
                  mRtpSession[i]->CheckSessionByTrackId(trackid))) {
      session = mRtpSession[i];
      break;
    }
  }

  return session;
}

CRtpSession* CRtspServer::GetRtpSessionById(AM_UINT id)
{
  CRtpSession* session = NULL;

  for (AM_UINT i = 0; i < (MAX_ENCODE_STREAM_NUM + 1); ++ i) {
    session = mRtpSession[i];
    if (AM_LIKELY(session && (id == session->GetStreamId()))) {
      break;
    }
  }

  return session;
}

bool CRtspServer::GetSdp(char* streamName,
                         char* buf,
                         AM_UINT size,
                         struct sockaddr_in& clientAddr,
                         AM_U16 clientPort,
                         bool trackOnly)
{
  bool ret = false;

  CRtpSession* session = GetRtpSessionByName(streamName);
  if (AM_LIKELY(buf && (size > 0) && session)) {
    CRtpSession* audio = trackOnly ? NULL : mRtpSession[MAX_ENCODE_STREAM_NUM];
    const char* serverString = "Streamed by \"Ambarella RTSP Server\"";
    const char* sourceFilterLine = "";
    const char* fMiscSdpLines = "";
    timespec* creationTime = session->GetCreationTime(streamName);
    NetDeviceInfo* netdev = FindNetDevInfoByAddr(clientAddr);
    const char *hostAddr = netdev ?
        netdev->ipv4->get_address_string() : "0.0.0.0";
    char sessionSdp[4096] = {0};
    session->GetSdp(sessionSdp, sizeof(sessionSdp), clientAddr, clientPort);

    snprintf(buf, size,
             "v=0\r\n"
             "o=- %ld%06ld %d IN IP4 %s\r\n"
             "s=%s\r\n"
             "i=%s\r\n"
             "t=0 0\r\n"
             "a=tool:Ambarella RTSP Server: %s - %s\r\n"
             "a=type:broadcast\r\n"
             "a=control:*\r\n"
             "a=range:npt=0-\r\n"
             "%s"
             "a=x-qt-text-nam:%s\r\n"
             "a=x-qt-text-inf:%s\r\n"
             "%s"
             "%s",
             creationTime->tv_sec,
             ((creationTime->tv_nsec + 500) / 1000), /* o= <session id> */
             1,                                      /* o= <version>    */
             hostAddr,
             serverString,
             streamName,
             __DATE__, __TIME__,
             sourceFilterLine,
             serverString,
             streamName,
             fMiscSdpLines,
             sessionSdp);
    if (AM_LIKELY((session != audio) && audio &&
                  ((mAudioInfo.format == MF_AAC)     ||
                   (mAudioInfo.format == MF_OPUS)    ||
                   (mAudioInfo.format == MF_G726_40) ||
                   (mAudioInfo.format == MF_G726_32) ||
                   (mAudioInfo.format == MF_G726_24) ||
                   (mAudioInfo.format == MF_G726_16)))) {
      char audioSdp[4096] = {0};
      audio->GetSdp(audioSdp, sizeof(audioSdp), clientAddr, 0);
      snprintf(&buf[strlen(buf)], size - strlen(buf), "%s\r\n", audioSdp);
    }
    ret = true;
  }

  return ret;
}

bool CRtspServer::GetRtspUrl(char* streamName,
                             char* buf,
                             AM_UINT size,
                             struct sockaddr_in& clientAddr)
{
  bool ret = false;
  NetDeviceInfo *devinfo = FindNetDevInfoByAddr(clientAddr);
  const char* hostAddr = devinfo ?
      devinfo->ipv4->get_address_string() : "0.0.0.0";
  if (AM_LIKELY(buf && (size > 0))) {
    if (AM_LIKELY(GetServerPortTcp() == 554)) {
      snprintf(buf, size, "rtsp://%s/%s", hostAddr, streamName);
    } else {
      snprintf(buf, size, "rtsp://%s:%hu/%s",
               hostAddr, GetServerPortTcp(), streamName);
    }
    INFO("RTSP URL: %s", buf);
    ret = true;
  }
  return ret;
}

CRtspServer::CRtspServer(CRtspFilter *filter, bool RTPriority, int priority) :
    mSrvSockTcp(-1),
    mPortTcp(RTSP_SERVER_PORT),
    mRun(false),
    mSendNeedWait(false),
    mRtspNeedAuth(false),
    mRTPriority(RTPriority),
    mPriority(priority),
    mHwTimerFd(-1),
    mTcpSendCnt(0),
    mUdpSendCnt(0),
    mTcpSendSize(0),
    mUdpSendSize(0),
    mTcpSendTime(0),
    mUdpSendTime(0),
    mTcpAvgSpeed(0),
    mTcpMaxSpeed(0),
    mTcpMinSpeed((AM_U64)-1),
    mUdpAvgSpeed(0),
    mUdpMaxSpeed(0),
    mUdpMinSpeed((AM_U64)-1),
    mThread(NULL),
    mEvent(NULL),
    mMutex(NULL),
    mClientMutex(NULL),
    mRtspFilter(filter),
    mNetworkConfig(NULL),
    mNetDeviceInfo(NULL),
    mDefaultNetdev(NULL)
{
  memset(mPipe, -1, sizeof(mPipe));
  memset(mClientData, 0, sizeof(CRtspClientSession*) * MAX_CLIENT_NUMBER);
  memset(mRtpSession, 0, sizeof(mRtpSession));
  memset(&mAudioInfo, 0, sizeof(AM_AUDIO_INFO));
  memset(mVideoInfo, 0, sizeof(AM_VIDEO_INFO)*MAX_ENCODE_STREAM_NUM);
}

CRtspServer::~CRtspServer()
{
  if (AM_LIKELY(mSrvSockTcp >= 0)) {
    close(mSrvSockTcp);
  }
  AM_DELETE(mThread);
  AM_DELETE(mEvent);
  AM_DELETE(mMutex);
  AM_DELETE(mClientMutex);
  if (AM_LIKELY(mPipe[0] >= 0)) {
    close(mPipe[0]);
  }
  if (AM_LIKELY(mPipe[1] >= 0)) {
    close(mPipe[1]);
  }
  if (AM_LIKELY(mHwTimerFd >= 0)) {
    close(mHwTimerFd);
  }
  delete mNetworkConfig;
  delete mNetDeviceInfo;
  for (AM_UINT i = 0; i < MAX_CLIENT_NUMBER; ++ i) {
    delete mClientData[i];
  }
  for (AM_UINT i = 0; i <= MAX_ENCODE_STREAM_NUM; ++ i) {
    delete mRtpSession[i];
  }
}

AM_ERR CRtspServer::Construct()
{
  if (AM_UNLIKELY(NULL == mRtspFilter)) {
    ERROR("Invalid RTSP filter!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(NULL == (mEvent = CEvent::Create()))) {
    ERROR("Failed to create event!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(NULL == (mMutex = CMutex::Create()))) {
    ERROR("Failed to create mutex!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(NULL == (mClientMutex = CMutex::Create()))) {
    ERROR("Failed to create mutex!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(pipe(mPipe) < 0)) {
    PERROR("pipe");
    return ME_ERROR;
  }

  if (AM_UNLIKELY((mHwTimerFd = open(HW_TIMER, O_RDONLY)) < 0)) {
    PERROR("open hardware timer");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(NULL == (mNetworkConfig = new AmNetworkConfig()))) {
    ERROR("Failed to create network config client!");
    return ME_ERROR;
  }

  return ME_OK;
}

/*
 * Struct Destination
 */
Destination::Destination(struct in_addr& addr,
                         AM_U16          tcpRtpPt,
                         AM_U16          ch,
                         AM_U32          destssrc,
                         int            *tcpSock,
                         int            *udpRtpSock,
                         int            *udpRtcpSock,
                         RtspStreamMode  md)
{
  address           = addr;
  port              = tcpRtpPt;
  channel           = ch;
  ssrc              = destssrc;
  lastsenttimestamp = 0;
  lastsentbytecount = 0;
  lastsentntptime   = 0;
  enable            = false;
  sendsr            = true;
  clientTcpSock     = tcpSock;
  serverUdpRtpSock  = udpRtpSock;
  serverUdpRtcpSock = udpRtcpSock;
  mode              = md;
  prev              = NULL;
  next              = NULL;
  memset(sr, 0, sizeof(sr));
  tcphdr            = (RtspTcpHeader*)sr;
  rtcpHdr           = (RtcpHeader*)&sr[sizeof(RtspTcpHeader)];
  srpkt             = (RtcpSRPayload*)&sr[sizeof(RtspTcpHeader) +
                                          sizeof(RtcpHeader)];
  tcphdr->set_magic_num();
  tcphdr->channel     = channel + 1;
  rtcpHdr->padding    = 0;
  rtcpHdr->version    = 2;
  rtcpHdr->set_ssrc(destssrc);
}

void Destination::close_socket()
{
  if (AM_LIKELY(clientTcpSock && (*clientTcpSock >= 0))) {
    close(*clientTcpSock);
    *clientTcpSock = -1;
  }
  if (AM_LIKELY(serverUdpRtpSock && (*serverUdpRtpSock >= 0))) {
    close(*serverUdpRtpSock);
    *serverUdpRtpSock = -1;
  }
  if (AM_LIKELY(serverUdpRtcpSock && (*serverUdpRtcpSock >= 0))) {
    close(*serverUdpRtcpSock);
    *serverUdpRtcpSock = -1;
  }
}

void Destination::make_bye_packet()
{
  tcphdr->set_data_length(sizeof(RtcpHeader));
  rtcpHdr->rrcount = 1;
  rtcpHdr->set_packet_type(203);
  rtcpHdr->set_packet_length(sizeof(RtcpHeader) / sizeof(uint32_t) - 1);
}

void Destination::update_sent_data_info(uint32_t timestamp,
                                        uint64_t ntptime,
                                        uint32_t pktcount,
                                        uint32_t datasize)
{
  uint32_t rtcpbytes = 0;
  srpkt->set_packet_bytes(ntohl(srpkt->packetbytes) + datasize);
  srpkt->set_packet_count(ntohl(srpkt->packetcount) + pktcount);
  rtcpbytes = (ntohl(srpkt->packetbytes) - lastsentbytecount) * 5 / 1000;
  sendsr = (((0 == lastsentntptime) && (0 == lastsenttimestamp)) ||
            ((rtcpbytes >= (sizeof(RtcpHeader) + sizeof(RtcpSRPayload))) &&
             (timestamp != (uint32_t)lastsenttimestamp) &&
             ((ntptime - lastsentntptime) >= 5000000)));
}

void Destination::update_rtcp_sr(uint32_t timestamp,
                                 uint32_t samplefreq,
                                 uint64_t ntptime)
{
  if (AM_UNLIKELY(0 == lastsenttimestamp)) {
    lastsenttimestamp = timestamp;
  }
  if (AM_UNLIKELY(0 == lastsentntptime)) {
    lastsentntptime = ntptime;
  }
  if (AM_UNLIKELY(ntptime < lastsentntptime)) {
    ERROR("NTP-time error!");
    sendsr = false;
  } else {
    lastsenttimestamp += (((ntptime - lastsentntptime) * samplefreq + 500000)
                         / 1000000);
    lastsentntptime = ntptime;
    tcphdr->set_data_length((AM_U16)(sizeof(RtcpHeader)+sizeof(RtcpSRPayload)));
    rtcpHdr->rrcount = 0;
    rtcpHdr->set_packet_type(200);
    rtcpHdr->set_packet_length((AM_U16) ((sizeof(RtcpHeader)
        + sizeof(RtcpSRPayload)) / sizeof(uint32_t) - 1));
    srpkt->set_time_stamp((AM_U32)lastsenttimestamp);
    srpkt->set_ntp_time_h32((AM_U32) (ntptime / 1000000));
    srpkt->set_ntp_time_l32((AM_U32) (((ntptime % 1000000) << 32) / 1000000));

    lastsentbytecount = ntohl(srpkt->packetbytes);
  }
}

bool operator==(Destination &dest1, Destination &dest2)
{
  bool ret = false;
  if (&dest1 == &dest2) {
    ret = true;
  } else {
    ret = ((dest1.mode              == dest2.mode)              &&
           (dest1.port              == dest2.port)              &&
           (dest1.channel           == dest2.channel)           &&
           (dest1.ssrc              == dest2.ssrc)              &&
           (dest1.clientTcpSock     == dest2.clientTcpSock)     &&
           (dest1.serverUdpRtpSock  == dest2.serverUdpRtpSock)  &&
           (dest1.serverUdpRtcpSock == dest2.serverUdpRtcpSock) &&
           (dest1.address.s_addr    == dest2.address.s_addr));
  }

  return ret;
}

bool operator!=(Destination &dest1, Destination &dest2)
{
  return !(dest1 == dest2);
}
