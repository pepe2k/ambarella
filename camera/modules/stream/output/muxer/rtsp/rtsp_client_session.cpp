/*******************************************************************************
 * rtsp_client_session.cpp
 *
 * History:
 *   2013年10月15日 - [ypchang] created file
 *
 * Copyright (C) 2008-2013, Ambarella ShangHai Co,Ltd
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
#include "rtsp_authenticator.h"
#include "rtp_session.h"
#include "rtp_session_audio.h"
#include "rtp_session_video.h"

#define  SEND_BUF_SIZE             (8*1024*1024)
#define  SOCK_TIMEOUT_SECOND       5
#define  RTSP_PARAM_STRING_MAX     4096

#define  CMD_CLIENT_ABORT          'a'
#define  CMD_CLIENT_STOP           'e'

#define  ALLOWED_COMMAND \
  "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, GET_PARAMETER, SET_PARAMETER"

struct ClientTrackInfo
{
    AM_UINT      id;
    Destination *dest;
    ClientTrackInfo():
      id(-1),
      dest(NULL){}
    ClientTrackInfo(AM_UINT trackId, Destination *destination) :
      id(trackId),
      dest(destination){}
};

struct ClientSessionInfo
{
    CThread*            thread;
    CRtspClientSession* client;
    ClientSessionInfo(CThread* th, CRtspClientSession* cl) :
      thread(th),
      client(cl){}
};

CRtspClientSession::CRtspClientSession(CRtspServer* server,
                                       CRtspClientSession** location,
                                       AM_U16 udpRtpPort) :
        mAuthenticator(NULL),
        mSelfLocation(location),
        mRtspServer(server),
        mClientThread(NULL),
        mClientName(NULL),
        mSessionState(CLIENT_SESSION_INIT),
        mTcpSock(-1),
        mUdpRtpSock(-1),
        mUdpRtcpSock(-1),
        mUdpRtpPort(udpRtpPort),
        mUdpRtcpPort(udpRtpPort + 1),
        mClientSessionId(0),
        mTrackMap(0),
        mRtcpRRInterval(0),
        mRtcpRRLastRecvTime(0),
        mDynamicTimeOutSec(CLIENT_TIMEOUT_THRESHOLD),
        mRtcpRRCnt(0),
        mRtcpRRCntThreshold(3),
        mRtcpRRSsrc(0)
{
  memset(&mClientAddr, 0, sizeof(mClientAddr));
  memset(mClientCtrlSock, 0xff, sizeof(mClientCtrlSock));
}

CRtspClientSession::~CRtspClientSession()
{
  *mSelfLocation = NULL;
  CloseAllSocket();
  while(!mTrackInfoQ.empty()) {
    delete mTrackInfoQ.front();
    mTrackInfoQ.pop();
  }
  AM_DELETE(mClientThread);
  delete[] mClientName;
  delete mAuthenticator;
}

bool CRtspClientSession::InitClientSession(AM_INT serverTcpSock)
{
  bool ret = false;

  mSessionState = CLIENT_SESSION_INIT;
  if (AM_UNLIKELY(pipe(mClientCtrlSock) < 0)) {
    PERROR("pipe");
    mSessionState = CLIENT_SESSION_FAILED;
  } else if (AM_LIKELY(mRtspServer && mSelfLocation && (serverTcpSock >= 0))) {
    AM_UINT acceptCnt = 0;
    socklen_t sockLen = sizeof(mClientAddr);

    mClientSessionId = mRtspServer->GetRandomNumber();
    do {
      mTcpSock = accept(serverTcpSock, (sockaddr*)&(mClientAddr), &sockLen);
    } while((++ acceptCnt < 5) && (mTcpSock < 0) &&
            ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
             (errno == EINTR)));
    if (AM_LIKELY(mTcpSock >= 0)) {
      mUdpRtpSock  = SetupServerSocketUdp(mUdpRtpPort);
      mUdpRtcpSock = SetupServerSocketUdp(mUdpRtcpPort);
    } else {
      PERROR("accept");
    }

    if (AM_LIKELY((mTcpSock >= 0) &&
                  (mUdpRtpSock >= 0) &&
                  (mUdpRtcpSock >= 0))) {
      AM_INT sendBuf = SEND_BUF_SIZE;
      AM_INT noDelay = 1;
      timeval timeout = {SOCK_TIMEOUT_SECOND, 0};
      if (AM_UNLIKELY((setsockopt(mTcpSock,
                                  IPPROTO_TCP, TCP_NODELAY,
                                  &noDelay, sizeof(noDelay)) != 0))) {
        PERROR("setsockopt");
        mSessionState = CLIENT_SESSION_FAILED;
      } else if (AM_UNLIKELY((setsockopt(mTcpSock,
                                         SOL_SOCKET, SO_SNDBUF,
                                         &sendBuf, sizeof(sendBuf)) != 0))) {
        PERROR("setsockopt");
        mSessionState = CLIENT_SESSION_FAILED;
      } else if (AM_UNLIKELY((setsockopt(mTcpSock,
                                         SOL_SOCKET, SO_RCVTIMEO,
                                         (char*)&timeout,
                                         sizeof(timeout)) != 0))) {
        PERROR("setsockopt");
        mSessionState = CLIENT_SESSION_FAILED;
      } else if (AM_UNLIKELY((setsockopt(mTcpSock,
                                         SOL_SOCKET, SO_SNDTIMEO,
                                         (char*)&timeout,
                                         sizeof(timeout)) != 0))) {
        PERROR("setsockopt");
        mSessionState = CLIENT_SESSION_FAILED;
      } else {
        char clientName[128] = {0};
        sprintf(clientName, "%s-%hu", inet_ntoa(mClientAddr.sin_addr),
                ntohs(mClientAddr.sin_port));
        SetClientName(clientName);
        mClientThread = CThread::Create(mClientName, StaticClientThread, this);
        if (AM_UNLIKELY(!mClientThread)) {
          ERROR("Failed to create client thread for %s", clientName);
          mSessionState = CLIENT_SESSION_FAILED;
        } else if (mRtspServer->mRTPriority) {
          if (AM_UNLIKELY(ME_OK !=
              mClientThread->SetRTPriority(mRtspServer->mPriority))) {
            ERROR("Failed to set %s to RT thread!", mClientName);
          } else {
            NOTICE("%s is set to real time thread, priority is %u",
                   mClientName, mRtspServer->mRTPriority);
          }
          mSessionState = CLIENT_SESSION_THREAD_RUN;
          mAuthenticator = new CRtspAuthenticator();
          ret = (NULL != mAuthenticator);
          if (AM_UNLIKELY(!mAuthenticator)) {
            ERROR("Failed to new authenticator!");
          } else {
            mAuthenticator->Init();
          }
          mSessionState = ret ? CLIENT_SESSION_OK : mSessionState;
        }
      }
    } else {
      if (AM_LIKELY(mUdpRtpSock < 0)) {
        ERROR("Failed to create RTP UDP socket!");
      }

      if (AM_LIKELY(mUdpRtcpSock < 0)) {
        ERROR("Failed to create RTCP UDP socket!");
      }
      mSessionState = CLIENT_SESSION_FAILED;
    }
  } else {
    if (AM_LIKELY(!mRtspServer)) {
      ERROR("Invalid RTSP server!");
    }
    if (AM_LIKELY(!mSelfLocation)) {
      ERROR("Invalid location!");
    }
    if (AM_LIKELY(serverTcpSock < 0)) {
      ERROR("Invalid RTSP server socket!");
    }
    mSessionState = CLIENT_SESSION_FAILED;
  }

  return ret;
}

void CRtspClientSession::ClientAbort()
{
  while (mSessionState == CLIENT_SESSION_INIT) {
    usleep(100000);
  }
  if (AM_LIKELY((mSessionState == CLIENT_SESSION_OK) ||
                (mSessionState == CLIENT_SESSION_THREAD_RUN))) {
    char cmd[1] = {CMD_CLIENT_ABORT};
    int count = 0;
    while ((++ count < 5) && (1 != write(CLIENT_CTRL_WRITE, cmd, 1))) {
      if (AM_LIKELY((errno != EAGAIN) &&
                    (errno != EWOULDBLOCK) &&
                    (errno != EINTR))) {
        PERROR("write");
        break;
      }
    }
    NOTICE("Sent abort command to client: %s", mClientName);
  } else {
    ERROR("This client is not initialized successfully!");
  }
}

bool CRtspClientSession::KillClient()
{
  bool ret = true;
  while (mSessionState == CLIENT_SESSION_INIT) {
    usleep(100000);
  }
  switch(mSessionState) {
    case CLIENT_SESSION_OK: {
      char cmd[1] = {CMD_CLIENT_STOP};
      int count = 0;
      while ((++count < 5) && (1 != write(CLIENT_CTRL_WRITE, cmd, 1))) {
        ret = false;
        if (AM_LIKELY((errno != EAGAIN) &&
                      (errno != EWOULDBLOCK) &&
                      (errno != EINTR))) {
          PERROR("write");
          break;
        }
      }
      if (AM_LIKELY(ret && (ME_OK != mClientThread->\
          SetRTPriority(CThread::PRIO_HIGHEST)))) {
        /* We need the client session to handle this as soon as possible */
        WARN("Failed to change thread %s to highest priority!");
      }
    }break;
    case CLIENT_SESSION_FAILED: {
      WARN("This client is not initialized successfully!");
    }break;
    case CLIENT_SESSION_STOPPED: {
      NOTICE("This client is already stopped, no need to kill again!");
      ret = true;
    }break;
    default:break;
  }

  return ret;
}

void CRtspClientSession::CloseAllSocket()
{
  if (AM_LIKELY(mTcpSock >= 0)) {
    close(mTcpSock);
    mTcpSock = -1;
  }

  if (AM_LIKELY(mUdpRtpSock >= 0)) {
    close(mUdpRtpSock);
    mUdpRtpSock = -1;
  }

  if (AM_LIKELY(mUdpRtcpSock >= 0)) {
    close(mUdpRtcpSock);
    mUdpRtcpSock = -1;
  }

  for (AM_UINT i = 0; i < (sizeof(mClientCtrlSock)/sizeof(AM_INT)); ++ i) {
    if (AM_LIKELY(mClientCtrlSock[i] >= 0)) {
      close(mClientCtrlSock[i]);
      mClientCtrlSock[i] = -1;
    }
  }
}

int CRtspClientSession::SetupServerSocketUdp(AM_U16 streamPort)
{
  int reuse = 1;
  int  sock = -1;
  struct sockaddr_in streamAddr;

  memset(&streamAddr, 0, sizeof(streamAddr));
  streamAddr.sin_family      = AF_INET;
  streamAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  streamAddr.sin_port        = htons(streamPort);

  if (AM_UNLIKELY((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)) {
    PERROR("socket");
  } else {
    AM_INT sendBuf = SEND_BUF_SIZE;
    if (AM_LIKELY((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                              &reuse, sizeof(reuse)) == 0) &&
                  (setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
                              &sendBuf, sizeof(sendBuf)) == 0))) {
      if (AM_UNLIKELY(bind(sock, (struct sockaddr*)&streamAddr,
                           sizeof(streamAddr)) != 0)) {
        close(sock);
        sock = -1;
        PERROR("bind");
      } else {
        if (AM_LIKELY(mRtspServer->mSendNeedWait)) {
          timeval timeout = {SOCK_TIMEOUT_SECOND * 3, 0};
          if (AM_UNLIKELY((setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                                      (char*)&timeout,
                                      sizeof(timeout)) != 0))) {
            close(sock);
            sock = -1;
            PERROR("setsockopt");
          } else if (AM_UNLIKELY((setsockopt(sock,
                                             SOL_SOCKET, SO_SNDTIMEO,
                                             (char*)&timeout,
                                             sizeof(timeout)) != 0))) {
            close(sock);
            sock = -1;
            PERROR("setsockopt");
          }
        } else {
          AM_INT flag = fcntl(sock, F_GETFL, 0);
          flag |= O_NONBLOCK;
          if (AM_UNLIKELY(fcntl(sock, F_SETFL, flag) != 0)) {
            close(sock);
            sock = -1;
            PERROR("fcntl");
          }
        }
      }
    } else {
      close(sock);
      sock = -1;
      PERROR("setsockopt");
    }
  }

  return sock;
}

AM_ERR CRtspClientSession::ClientThread()
{
  bool run = true;
  int maxfd = -1;
  fd_set allset;
  fd_set fdset;

  char rtspReqStr[8192] = {0};
  ParseState state = PARSE_COMPLETE;
  bool ret = true;
  AM_INT received = 0;
  AM_INT readRet = 0;
  timeval timeout = {CLIENT_TIMEOUT_THRESHOLD, 0};

  FD_ZERO(&allset);
  FD_SET(mTcpSock, &allset);
  FD_SET(mUdpRtcpSock, &allset);
  FD_SET(CLIENT_CTRL_READ, &allset);
  maxfd = AM_MAX(mTcpSock, CLIENT_CTRL_READ);
  maxfd = AM_MAX(maxfd, mUdpRtcpSock);

  INFO("New Client: %s, port %hu\n", inet_ntoa(mClientAddr.sin_addr),
       ntohs(mClientAddr.sin_port));
  signal(SIGPIPE, SIG_IGN);

  while (run) {
    int retval = -1;
    timeval* tm = &timeout;
    timeout.tv_sec = mDynamicTimeOutSec;

    fdset = allset;
    if (AM_LIKELY((retval = select(maxfd + 1, &fdset, NULL, NULL, tm)) > 0)) {
      if (FD_ISSET(CLIENT_CTRL_READ, &fdset)) {
        char     cmd[1] = {0};
        AM_UINT readCnt = 0;
        ssize_t readRet = 0;
        do {
          readRet = read(CLIENT_CTRL_READ, &cmd, sizeof(cmd));
        } while ((++ readCnt < 5) && ((readRet == 0) || ((readRet < 0) &&
                 ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
                 (errno == EINTR)))));
        if (AM_UNLIKELY(readRet <= 0)) {
          PERROR("read");
        } else if (cmd[0] == CMD_CLIENT_STOP) {
          NOTICE("Client:%s received stop signal!", mClientName);
          run = false;
          TearDown();
          CloseAllSocket();
          break;
        } else if (cmd[0] == CMD_CLIENT_ABORT) {
          NOTICE("Client:%s received abort signal!", mClientName);
          break;
        }
      } else if (FD_ISSET(mUdpRtcpSock, &fdset)) {
        char RTCP[128] = {0};
        int ret = -1;
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        memcpy(&clientAddr, &mClientAddr, sizeof(clientAddr));
        clientAddr.sin_port = htons(mUdpRtcpPort);

        if (AM_UNLIKELY((ret = recvfrom(mUdpRtcpSock,
                                        RTCP, sizeof(RTCP), 0,
                                        (struct sockaddr*)&clientAddr,
                                        &addrLen))) < 0) {
          PERROR("recvfrom");
        } else { /* Receiver Report */
          HandleRtcpPacket(RTCP);
        }
      } else if (FD_ISSET(mTcpSock, &fdset)) {
        RtspRequest rtspReq;
        AM_UINT readCnt = 0;
        if (AM_LIKELY(ret && ((state == PARSE_COMPLETE) ||
                              (state == PARSE_RTCP)))) {
          received = 0;
          readRet  = 0;
          memset(rtspReqStr, 0, sizeof(rtspReqStr));
        }
        do {
          /* mTcpSock is always in blocking mode */
          readRet = recv(mTcpSock, &rtspReqStr[received],
                         sizeof(rtspReqStr) - received, 0);
        } while ((++ readCnt < 5) &&
                 ((readRet == 0) || ((readRet < 0) && (errno == EINTR))));
        if (AM_LIKELY(readRet > 0)) {
          received += readRet;
          ret = ParseClientRequest(rtspReq, state,
                                   rtspReqStr, received);
          if (AM_LIKELY(ret && (state == PARSE_COMPLETE))) {
            INFO("\nReceived request from client %s, port %hu, "
                 "length %d, request:\n%s",
                 inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port),
                 received, rtspReqStr);
            rtspReq.print_info();
          } else if (AM_LIKELY(ret && (state == PARSE_RTCP))) {
            received = 0;
          } else if (AM_UNLIKELY(!ret)) {
            NOTICE("\nReceived unknown request from client %s, port %hu, "
                   "length %d, request:\n%s",
                   inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port),
                   received, rtspReqStr);
          }
        } else if (AM_UNLIKELY(readRet <= 0)) {
          if (AM_LIKELY((errno == ECONNRESET) || (readRet == 0))) {
            if (AM_LIKELY(mTcpSock >= 0)) {
              /* Socket may be closed in SendPacket */
              close(mTcpSock);
              mTcpSock = -1;
            }
            WARN("Client closed socket!");
          } else {
            PERROR("recv");
            ERROR("Server is going to close this socket!");
          }
          run = false;
          TearDown();
          continue;
        }
        if (AM_UNLIKELY(ret && ((state == PARSE_INCOMPLETE) ||
                                (state == PARSE_RTCP)))) {
          continue;
        }

        /* Handle requests from client */
        if (AM_LIKELY(ret && (PARSE_COMPLETE == state))) {
          if (AM_LIKELY(rtspReq.command)) {
            if (AM_LIKELY(mRtspServer->mRtspNeedAuth)) {
              if (AM_LIKELY(ParseAuthenticationHeader(mRtspAuthHdr,
                                                      rtspReqStr,
                                                      received))) {
                mRtspAuthHdr.print_info();
              }
            }
            if (AM_LIKELY(is_str_equal(rtspReq.command, "SETUP"))) {
              if (AM_LIKELY(ParseTransportHeader(mRtspTransHdr,
                                                 rtspReqStr,
                                                 received))) {
                mRtspTransHdr.print_info();
              }
            }
          } else {
            /* Received NULL command, possible junk data */
            WARN("Received NULL request from client!");
            for (AM_INT i = 0;
                (i < received) && ((size_t)i < sizeof(rtspReqStr));
                ++ i) {
              printf("%c", rtspReqStr[i]);
            }
            printf("\n");
            rtspReq.set_command((char*)"BAD_REQUEST");
          }
        } else {
          /* Fake a BAD_REQUEST command */
          rtspReq.set_command((char*)"BAD_REQUEST");
        }
        run = HandleClientRequest(rtspReq);
      }
    } else if (retval == 0) {
      WARN("Client: %s is not responding within %d seconds, shutdown!",
           mClientName, mDynamicTimeOutSec);
      run = false;
      TearDown();
      CloseAllSocket();
    } else {
      PERROR("select");
      run = false;
    }
  }
  mSessionState = CLIENT_SESSION_STOPPED;
  if (AM_LIKELY(!run)) {
    /* if run is still true, client is abort from server,
     * so there's no need to ask server to delete this session
     */
    int count = 0;
    bool val = false;
    while ((++count < 5) &&
           !(val = mRtspServer->DeleteClientSession(mSelfLocation))) {
      usleep(5000);
    }
    if (AM_UNLIKELY((count >= 5) && !val)) {
      WARN("RtspServer is dead before client session exit!");
    }
  }

  return ME_OK;
}

bool CRtspClientSession::ParseTransportHeader(RtspTransHeader& header,
                                              char*            reqStr,
                                              AM_UINT          reqLen)
{
  bool ret = false;

  if (AM_LIKELY(reqStr && (reqLen > 0))) {
    char* req = reqStr;
    char* field = NULL;
    AM_INT len = reqLen;

    while (len >= 11) {
      if (AM_UNLIKELY(strncasecmp(req, "Transport: ", 11) == 0)) {
        break;
      }
      ++ req;
      -- len;
    }
    req += 11;
    len -= 11;
    field = ((len > 0) && (strlen(req) > 0)) ? new char[strlen(req) + 1] : NULL;
    if (AM_LIKELY(field)) {
      AM_INT rtpPort, rtcpPort;
      AM_INT rtpChannel, rtcpChannel;
      AM_INT ttl;
      header.rtpChannelId  = 0xff;
      header.rtcpChannelId = 0xff;
      header.clientDstTTL  = 255;
      while(sscanf(req, "%[^;]", field) == 1) {
        INFO("FIELD: %s", field);
        if (is_str_same(field, "RTP/AVP/TCP")) {
          header.mode = RTP_TCP;
          header.set_streaming_mode_str(field);
        } else if (is_str_same(field, "RAW/RAW/UDP") ||
            is_str_same(field, "MP2T/H2221/UDP")) {
          header.mode = RAW_UDP;
          header.set_streaming_mode_str(field);
        } else if (is_str_same(field, "RTP/AVP") ||
            is_str_same(field, "RTP/AVP/UDP")) {
          header.mode = RTP_UDP;
          header.set_streaming_mode_str(field);
        } else if (strncasecmp(field, "destination=", 12) == 0) {
          header.set_client_dst_addr_str(&field[12]);
        } else if (sscanf(field, "ttl%d", &ttl) == 1) {
          header.clientDstTTL = ttl;
        } else if (sscanf(field, "client_port=%d-%d",
                          &rtpPort, &rtcpPort) == 2) {
          header.clientRtpPort  = (AM_U16)rtpPort;
          header.clientRtcpPort = (AM_U16)rtcpPort;
        } else if (sscanf(field, "client_port=%d", &rtpPort) == 1) {
          header.clientRtpPort = (AM_U16)rtpPort;
          header.clientRtcpPort = (header.mode == RAW_UDP) ?
              0 : header.clientRtpPort + 1;
        } else if (sscanf(field, "interleaved=%d-%d",
                          &rtpChannel, &rtcpChannel) == 2) {
          header.rtpChannelId = rtpChannel;
          header.rtcpChannelId = rtcpChannel;
        }

        req += strlen(field);
        len -= strlen(field);
        while ((len > 0) && (*req == ';')) {
          ++ req;
          -- len;
        }
        if ((*req == '\0') || (*req == '\r') || (*req == '\n')) {
          break;
        }
      }
      delete[] field;
      ret = true;
    }
  }

  return ret;
}

bool CRtspClientSession::ParseAuthenticationHeader(RtspAuthHeader& header,
                                                   char*           reqStr,
                                                   AM_UINT         reqLen)
{
  bool ret = false;

  if (AM_LIKELY(reqStr && (reqLen > 0))) {
    int len = reqLen;
    char* req = reqStr;
    char parameter[128] = {0};
    char value[128] = {0};

    while(len >= 22) {
      if (AM_UNLIKELY(strncasecmp(req, "Authorization: Digest ", 22) == 0)) {
        break;
      }
      ++ req;
      -- len;
    }
    req += 22;
    len -= 22;
    /* Skip all white space */
    while ((len > 0) && ((*req == ' ') || (*req == '\t'))) {
      ++ req;
      -- len;
    }
    while (len > 0) {
      memset(parameter, 0, sizeof(parameter));
      memset(value, 0, sizeof(value));
      if (AM_UNLIKELY((sscanf(req, "%[^=]=\"%[^\"]\"", parameter, value) != 2)&&
                      (sscanf(req, "%[^=]=\"\"", parameter) != 1))) {
        break;
      }
      if (AM_LIKELY(is_str_equal(parameter, "username"))) {
        header.set_username(value);
      } else if (AM_LIKELY(is_str_equal(parameter, "realm"))) {
        header.set_realm(value);
      } else if (AM_LIKELY(is_str_equal(parameter, "nonce"))) {
        header.set_nonce(value);
      } else if (AM_LIKELY(is_str_equal(parameter, "uri"))) {
        header.set_uri(value);
      } else if (AM_LIKELY(is_str_equal(parameter, "response"))) {
        header.set_response(value);
      }
      req += strlen(parameter) + 2 /* =" */ + strlen(value) + 1 /* " */;
      len -= strlen(parameter) + 2 /* =" */ + strlen(value) + 1 /* " */;
      while ((len > 0) && ((*req == ',') || (*req == ' ') || (*req == '\t'))) {
        ++ req;
        -- len;
      }
    }

    ret = (header.realm && header.nonce &&
           header.username && header.uri && header.response);
  }

  return ret;
}

void CRtspClientSession::HandleRtcpPacket(char* RTCP)
{
  if (AM_LIKELY(RTCP)) {
    RtcpHeader*   rtcpHdr = (RtcpHeader*)RTCP;
    switch(RTCP[1]) {
      case RTCP_RR: { /* Receiver Report */
        AM_U64 rtcpRRRecvTime = mRtspServer->GetFakeNtpTime();
        AM_U32 rtcpCurSSRC = rtcpHdr->get_ssrc();
#if 0
        RtcpRRPayload* rtcpRR = (RtcpRRPayload*)(&RTCP[sizeof(RtcpHeader)]);
        AM_U8 rc = rtcpHdr->get_rr_count();
        for (AM_UINT i = 0; i < rc; ++ i) {
          INFO("\nReceived RR from %08x:"
               "\n              Fraction Lost: %hhu"
               "\n         Total Packets Lost: %u"
               "\nReceived Packets Seq Number: %u"
               "\n        Interarrival Jitter: %u"
               "\n                    Last SR: %u"
               "\n        Delay Since Last SR: %u",
               rtcpRR->get_source_ssrc(),
               rtcpRR->get_fraction_lost(),
               rtcpRR->get_packet_lost(),
               rtcpRR->get_sequence_number(),
               rtcpRR->get_jitter(),
               rtcpRR->get_lsr(),
               rtcpRR->get_dlsr());
          ++ rtcpRR;
        }
#endif
        if (AM_UNLIKELY(mRtcpRRSsrc == 0)) {
          /* Only need one RR report to calculate time interval */
          mRtcpRRSsrc = rtcpCurSSRC;
        }
        if (AM_LIKELY(mRtcpRRSsrc == rtcpCurSSRC)) {
          if (AM_LIKELY((mRtcpRRLastRecvTime > 0) &&
                        (rtcpRRRecvTime > mRtcpRRLastRecvTime))) {
            mRtcpRRInterval += (rtcpRRRecvTime - mRtcpRRLastRecvTime);
          }
          mRtcpRRLastRecvTime = rtcpRRRecvTime;
          mRtcpRRCnt += mRtcpRRInterval ? 1 : 0;
          if (AM_UNLIKELY(mRtcpRRCnt >= mRtcpRRCntThreshold)) {
            /* (A/B + 1/2) == ((2A + B) / 2B) */
            AM_U64 avgInterval = (2 * mRtcpRRInterval + mRtcpRRCnt) /
                (2 * mRtcpRRCnt);
            mRtcpRRCntThreshold =
                (AM_INT)((2 * 15000000 + avgInterval) / (2*avgInterval));
            mDynamicTimeOutSec =
                (AM_INT)((2*mRtcpRRInterval + (mRtcpRRCnt*1000000)) /
                    (2 * (mRtcpRRCnt*1000000))) + 2;
            if (mDynamicTimeOutSec <= 2) {
              NOTICE("Too much RR packet!");
              mDynamicTimeOutSec *= 4;
            } else {
              mDynamicTimeOutSec *= 2;
            }
            NOTICE("Got %u RR packets within %llu microseconds!",
                   mRtcpRRCnt, mRtcpRRInterval);
            NOTICE("Client timeout is changed to %d seconds, "
                "RR packet statistics threshold changed to %d",
                mDynamicTimeOutSec, mRtcpRRCntThreshold);
            mRtcpRRCnt = 0;
            mRtcpRRInterval = 0;
          }
        }
      }break;
      default:
        /* todo: handle other RTCP packet */
        break;
    }
  }
}

bool CRtspClientSession::ParseClientRequest(RtspRequest&     request,
                                            ParseState&      state,
                                            char*            reqStr,
                                            AM_UINT          reqLen)
{
  char result[RTSP_PARAM_STRING_MAX] = {0};
  AM_UINT cnt = 0;
  bool succeed = false;
  state = PARSE_INCOMPLETE;

  while ((cnt < reqLen) && (reqStr[cnt] == '$')) {
    /* This is a RTCP packet over TCP */
    if ((reqLen - cnt) <= sizeof(RtspTcpHeader)) {
      /* This RTCP packet is in-complete */
      return true;
    } else {
      uint16_t rtcpPktSize = (reqStr[cnt + 2] << 8) | reqStr[cnt + 3];
      if ((reqLen - cnt - sizeof(RtspTcpHeader)) < rtcpPktSize) {
        /* RTCP packet is in-complete */
        return true;
      } else {
        char RTCP[rtcpPktSize];
        memset(RTCP, 0, sizeof(RTCP));
        memcpy(RTCP, &reqStr[cnt + 4], rtcpPktSize);
        HandleRtcpPacket(RTCP);
        /* Skip over RTCP packet */
        cnt += (sizeof(RtspTcpHeader) + rtcpPktSize);
      }
    }
  }

  if (cnt >= reqLen) {
    /* Only RTCP packet is received */
    state = PARSE_RTCP;
    return true;
  }

  for (; cnt < reqLen && cnt < RTSP_PARAM_STRING_MAX; ++ cnt) {
    if (AM_UNLIKELY((reqStr[cnt] == ' ') || (reqStr[cnt] == '\t'))) {
      succeed = true;
      break;
    }
    result[cnt] = reqStr[cnt];
  }
  result[cnt] = '\0';
  if (AM_LIKELY(succeed)) {
    request.set_command(result);
  } else {
    return false;
  }

  /* skip over any additional white space */
  while ((cnt < reqLen) && ((reqStr[cnt] == ' ') || (reqStr[cnt] == '\t'))) {
    ++ cnt;
  }

  /* Skip over rtsp://url:port/ */
  for (; cnt < (reqLen - 8); ++ cnt) {
    if (AM_LIKELY((0 == strncasecmp(&reqStr[cnt], "rtsp://", 7)) ||
                  (0 == strncasecmp(&reqStr[cnt], "*", 1)))) {
      if (AM_LIKELY(0 == strncasecmp(&reqStr[cnt], "rtsp://", 7))) {
        cnt += 7;
        while ((cnt < reqLen) && (reqStr[cnt] != '/') &&
               (reqStr[cnt] != ' ') && (reqStr[cnt] != '\t')) {
          ++ cnt;
        }
      } else {
        cnt += 1;
      }
      if (AM_UNLIKELY(cnt == reqLen)) {
        return false;
      }
      break;
    }
  }
  succeed = false;

  for (AM_UINT i = cnt; i < (reqLen - 5); ++ i) {
    if (AM_LIKELY(0 == strncmp(&reqStr[i], "RTSP/", 5))) {
      AM_UINT lineEnd = i + strlen("RTSP/"); /* Skip RTSP/ */
      AM_UINT endMark = 0;
      char    endChar = 0;
      while ((lineEnd < reqLen) &&
             (reqStr[lineEnd] != '\n')) { /* Find line end */
        ++ lineEnd;
      }
      /* Found "RTSP/", back skip all white spaces */
      -- i;
      while ((i > cnt) && ((reqStr[i] == ' ') || (reqStr[i] == '\t') ||
                           (reqStr[i] == '/'))) {
        -- i;
      }
      if (AM_UNLIKELY(i <= cnt)) {
        /* No Suffix found */
        succeed = true;
        break;
      } else {
        endMark = ++ i;
        endChar = reqStr[endMark];
        reqStr[endMark] = '\0';
      }

      while ((i > cnt) && (reqStr[i] != '/')) {
        -- i;
      }
      request.set_url_suffix(&reqStr[i + 1]);
      reqStr[endMark] = endChar; /* Reset the string end */
      if (i > cnt) {
        /* Go on looking for pre-suffix */
        endMark = i;
        endChar = reqStr[endMark];
        reqStr[i] = '\0';
        while ((i > cnt) && (reqStr[i] != '/')) {
          -- i;
        }
        request.set_url_pre_suffix(&reqStr[i + 1]);
        reqStr[endMark] = endChar; /* Reset the string end */
      }
      succeed = true;
      cnt = lineEnd + 1;
      break;
    }
  }

  if (AM_UNLIKELY(!succeed)) {
    return false;
  }

  /* Find CSeq */
  succeed = false;
  for (; cnt < (reqLen - 5); ++ cnt) {
    if (AM_LIKELY(0 == strncasecmp(&reqStr[cnt], "CSeq:", 5))) {
      cnt += 5;

      /* Skip all additional white spaces */
      while ((cnt < reqLen) &&
             ((reqStr[cnt] == ' ') || (reqStr[cnt] == '\t'))) {
        ++ cnt;
      }

      for (AM_UINT j = cnt; j < reqLen; ++ j) {
        if (AM_LIKELY((reqStr[j] == '\r') || (reqStr[j] == '\n'))) {
          char temp = reqStr[j];
          reqStr[j] = '\0';
          if (AM_LIKELY(strlen(&reqStr[cnt]) > 0)) {
            request.set_cseq(&reqStr[cnt]);
            reqStr[j] = temp;
            succeed = true;
            state = PARSE_COMPLETE;
          }
          break;
        }
      }
      break;
    }
  }

  if (AM_UNLIKELY(!succeed)) {
    NOTICE("Received incomplete client request!");
  }

  return true;
}

bool CRtspClientSession::HandleClientRequest(RtspRequest& request)
{
  char responseBuf[8192] = {0};
  bool ret = false;
  if (is_str_same((const char*)request.command, "OPTIONS")) {
    ret = CmdOptions(request, responseBuf, sizeof(responseBuf));
  } else if (is_str_same((const char*)request.command, "DESCRIBE")) {
    ret = CmdDescribe(request, responseBuf, sizeof(responseBuf));
  } else if (is_str_same((const char*)request.command, "SETUP")) {
    ret = CmdSetup(request, responseBuf, sizeof(responseBuf));
  } else if (is_str_same((const char*)request.command, "TEARDOWN")) {
    ret = CmdTeardown(request, responseBuf, sizeof(responseBuf));
  } else if (is_str_same((const char*)request.command, "PLAY")) {
    ret = CmdPlay(request, responseBuf, sizeof(responseBuf));
  } else if (is_str_same((const char*)request.command, "GET_PARAMETER")) {
    ret = CmdGetParameter(request, responseBuf, sizeof(responseBuf));
  } else if (is_str_same((const char*)request.command, "SET_PARAMETER")) {
    ret = CmdSetParameter(request, responseBuf, sizeof(responseBuf));
  } else if (is_str_same((const char*)request.command, "BAD_REQUEST")) {
    ret = CmdBadRequests(request, responseBuf, sizeof(responseBuf));
  } else {
    ret = CmdNotSupported(request, responseBuf, sizeof(responseBuf));
  }

  INFO("\nServer respond to %s:\n%s", mClientName, responseBuf);
  if (AM_UNLIKELY((mTcpSock >= 0) && ((ssize_t)strlen(responseBuf) !=
      write(mTcpSock, responseBuf, strlen(responseBuf))))) {
    if (AM_LIKELY(errno == ECONNRESET)) {
      WARN("Client closed socket!");
    } else {
      PERROR("write");
    }
    ret = false;
  } else {
    if (is_str_same((const char*)request.command, "PLAY") && ret) {
      ret = EnableDestination();
    }
  }

  return ret;
}

inline bool CRtspClientSession::CmdOptions(RtspRequest& req,
                                           char* buf,
                                           AM_UINT size)
{
  char date[200] = {0};
  snprintf(buf, size, "RTSP/1.0 200 OK\r\n"
                      "CSeq: %s\r\n%s"
                      "Public: %s\r\n\r\n",
           req.cseq, GetDateString(date, sizeof(date)), ALLOWED_COMMAND);
  return true;
}

inline bool CRtspClientSession::CmdDescribe(RtspRequest& req,
                                            char* buf,
                                            AM_UINT size)
{
  bool ret = true;
  if (AM_LIKELY(AuthenticateOK(req, buf, size))) {
    char sdpstr[4096]  = {0};
    char rtspurl[1024] = {0};
    bool trackOnly = false;
    CRtpSession *session = mRtspServer->GetRtpSessionByName(req.urlSuffix);

    ret = false;
    if (AM_UNLIKELY(NULL == session)) {
      session = mRtspServer->GetRtpSessionByTrackId(req.urlSuffix);
      trackOnly = (NULL != session);
      trackOnly ? NOTICE("StreamName is %s", session->GetStreamName()) :
          NOTICE("No such stream %s", req.urlSuffix);
    }
    if (AM_LIKELY(session && mRtspServer->GetRtspUrl(session->GetStreamName(),
                                                     rtspurl, sizeof(rtspurl),
                                                     mClientAddr) &&
                  mRtspServer->GetSdp(session->GetStreamName(),
                                      sdpstr, sizeof(sdpstr),
                                      mClientAddr, 0, trackOnly))) {
      char date[200] = {0};
      snprintf(buf, size,
               "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
               "%s"
               "Content-Base: %s/\r\n"
               "Content-Type: application/sdp\r\n"
               "Content-Length: %u\r\n\r\n"
               "%s",
               req.cseq,
               GetDateString(date, sizeof(date)),
               rtspurl,
               strlen(sdpstr),
               sdpstr);
      ret = true;
    } else {
      NotFound(req, buf, size);
    }
  }

  return ret;
}

inline bool CRtspClientSession::CmdSetup(RtspRequest& req,
                                         char* buf,
                                         AM_UINT size)
{
  bool ret = true;
  if (AM_LIKELY(AuthenticateOK(req, buf, size))) {
    ret = false;
    if (AM_LIKELY(buf && (size > 0))) {
      CRtpSession* track   = mRtspServer->GetRtpSessionByTrackId(req.urlSuffix);
      if (AM_LIKELY(track)) {
        char date[200] = {0};
        if (mRtspTransHdr.mode == RTP_UDP) {
          /* Newly created destination will be deleted in session */
          Destination *dest = new Destination(mClientAddr.sin_addr,
                                              mRtspTransHdr.clientRtpPort,
                                              mRtspTransHdr.rtpChannelId,
                                              mRtspServer->GetRandomNumber(),
                                              &mTcpSock,
                                              &mUdpRtpSock,
                                              &mUdpRtcpSock,
                                              mRtspTransHdr.mode);
          ClientTrackInfo *trackInfo = new ClientTrackInfo(track->GetStreamId(),
                                                           dest);
          snprintf(buf, size,
                   "RTSP/1.0 200 OK\r\n"
                   "CSeq: %s\r\n"
                   "%s"
                   "Transport: %s;unicast;destination=%s;source=%s;"
                   "client_port=%hu-%hu;server_port=%hu-%hu\r\n"
                   "Session: %08X\r\n\r\n",
                   req.cseq,
                   GetDateString(date, sizeof(date)),
                   mRtspTransHdr.modeString,
                   inet_ntoa(mClientAddr.sin_addr),
                   track->GetSourceAddrString(mClientAddr),
                   mRtspTransHdr.clientRtpPort,
                   mRtspTransHdr.clientRtcpPort,
                   mUdpRtpPort, mUdpRtcpPort,
                   mClientSessionId);
          if (AM_LIKELY(track->AddDestination(dest, track->GetStreamId()))) {
            mTrackInfoQ.push(trackInfo);
            ret = true;
          } else {
            delete trackInfo;
          }
        } else if (mRtspTransHdr.mode == RTP_TCP) {
          /* Newly created destination will be deleted in session */
          Destination *dest = new Destination(mClientAddr.sin_addr,
                                              mRtspTransHdr.clientRtpPort,
                                              mRtspTransHdr.rtpChannelId,
                                              mRtspServer->GetRandomNumber(),
                                              &mTcpSock,
                                              &mUdpRtpSock,
                                              &mUdpRtcpSock,
                                              mRtspTransHdr.mode);
          ClientTrackInfo *trackInfo = new ClientTrackInfo(track->GetStreamId(),
                                                           dest);
          snprintf(buf, size,
                   "RTSP/1.0 200 OK\r\n"
                   "CSeq: %s\r\n"
                   "%s"
                   "Transport: %s;interleaved=%hhu-%hhu\r\n"
                   "Session: %08X\r\n\r\n",
                   req.cseq,
                   GetDateString(date, sizeof(date)),
                   mRtspTransHdr.modeString,
                   mRtspTransHdr.rtpChannelId,
                   mRtspTransHdr.rtcpChannelId,
                   mClientSessionId);
          if (AM_LIKELY(track->AddDestination(dest, track->GetStreamId()))) {
            mTrackInfoQ.push(trackInfo);
            ret = true;
          } else {
            delete trackInfo;
          }
        } else {
          /* Currently we don't support RAW_UDP */
          CmdBadRequests(req, buf, size);
        }
      } else {
        NotFound(req, buf, size);
      }
    }
  }

  return ret;
}

inline bool CRtspClientSession::CmdTeardown(RtspRequest& req,
                                            char* buf,
                                            AM_UINT size)
{
  bool ret = AuthenticateOK(req, buf, size);
  if (AM_LIKELY(ret)) {
    if (AM_LIKELY(buf && (size > 0))) {
      INFO("Client: %s teardown!", mClientName);
      snprintf(buf, size, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n\r\n",
               req.cseq);
      TearDown();
    }
  }

  return !ret;
}

inline bool CRtspClientSession::CmdPlay(RtspRequest& req,
                                        char* buf,
                                        AM_UINT size)
{
  bool ret = true;
  if (AM_LIKELY(AuthenticateOK(req, buf, size))) {
    ret = false;
    if (AM_LIKELY(buf && (size > 0))) {
      if (AM_LIKELY(!mTrackInfoQ.empty())) {
        char date[200] = {0};
        const char* scaleHeader = "";
        AM_UINT queueSize = mTrackInfoQ.size();
        snprintf(buf, size,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "%s"
                 "Range: npt=0.000-\r\n"
                 "Session: %08X\r\n"
                 "RTP-Info: ",
                 req.cseq,
                 GetDateString(date, sizeof(date)),
                 scaleHeader,
                 mClientSessionId);
        for (AM_UINT i = 0; i < queueSize; ++ i) {
          char rtspurl[1024] = {0};
          ClientTrackInfo* trackInfo = mTrackInfoQ.front();
          CRtpSession* track = mRtspServer->GetRtpSessionById(trackInfo->id);
          mTrackInfoQ.pop();
          mTrackInfoQ.push(trackInfo);

          if (AM_LIKELY(track && mRtspServer->GetRtspUrl(track->GetStreamName(),
                                                         rtspurl,
                                                         sizeof(rtspurl),
                                                         mClientAddr))) {
            snprintf(&buf[strlen(buf)], size - strlen(buf),
                     "url=%s/%s"
                     ";seq=%hu"
                     ";rtptime=%u,",
                     rtspurl,
                     track->GetTrackId(),
                     track->GetSeqNumber(),
                     track->GetCurrentTimeStamp());
            ret = true;
          }
        }
        snprintf(&buf[strlen(buf) - 1], size - strlen(buf), "\r\n\r\n");
      } else {
        ret = CmdNotSupported(req, buf, size);
      }
    }
  }

  return ret;
}

inline bool CRtspClientSession::CmdGetParameter(RtspRequest& req,
                                                char* buf,
                                                AM_UINT size)
{
  bool ret = true;
  if (AM_LIKELY(AuthenticateOK(req, buf, size))) {
    ret = false;
    if (AM_LIKELY(buf && (size > 0))) {
      if (AM_LIKELY(mRtspServer->CheckStreamExistence(req.urlSuffix) ||
                    ((req.urlPreSuffix == NULL) &&
                        (is_str_same(req.urlSuffix, "*"))))) {
        char date[200] = {0};
        snprintf(buf, size,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Session: %08X\r\n\r\n",
                 req.cseq,
                 GetDateString(date, sizeof(date)),
                 mClientSessionId);
        ret = true;
      } else if (AM_LIKELY(false == mRtspServer->\
                           CheckStreamExistence(req.urlSuffix))) {
        NotFound(req, buf, size);
      } else {
        ret = CmdNotSupported(req, buf, size);
      }
    }
  }

  return ret;
}

inline bool CRtspClientSession::CmdSetParameter(RtspRequest& req,
                                                char* buf,
                                                AM_UINT size)
{
  /* This "SET_PARAMETER" just send back an empty response as 'keep alive',
   * re-implement this method if you want to handle this
   */
  bool ret = true;
  if (AM_LIKELY(AuthenticateOK(req, buf, size))) {
    ret = false;
    if (AM_LIKELY(buf && (size > 0))) {
      if (AM_LIKELY(mRtspServer->CheckStreamExistence(req.urlPreSuffix) ||
                    ((req.urlPreSuffix == NULL) &&
                        (is_str_same(req.urlSuffix, "*"))))) {
        char date[200] = {0};
        snprintf(buf, size,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Session: %08X\r\n\r\n",
                 req.cseq,
                 GetDateString(date, sizeof(date)),
                 mClientSessionId);
        ret = true;
      } else if (AM_LIKELY(false == mRtspServer->\
                           CheckStreamExistence(req.urlPreSuffix))) {
        NotFound(req, buf, size);
      } else {
        ret = CmdNotSupported(req, buf, size);
      }
    }
  }

  return ret;
}

inline bool CRtspClientSession::CmdNotSupported(RtspRequest& req,
                                                char* buf,
                                                AM_UINT size)
{
  char date[200] = {0};
  snprintf(buf, size, "RTSP/1.0 405 Method Not Allowed\r\n"
                      "CSeq: %s\r\n%s"
                      "Allow: %s\r\n\r\n",
           req.cseq, GetDateString(date, sizeof(date)), ALLOWED_COMMAND);
  return true;
}

inline bool CRtspClientSession::CmdBadRequests(RtspRequest& req,
                                               char* buf,
                                               AM_UINT size)
{
  char date[200] = {0};
  snprintf(buf, size, "RTSP/1.0 400 Bad Request\r\n%sAllow: %s\r\n\r\n",
           GetDateString(date, sizeof(date)), ALLOWED_COMMAND);
  return true;
}

inline bool CRtspClientSession::AuthenticateOK(RtspRequest& req,
                                               char* buf,
                                               AM_UINT size)
{
  bool authOK = true;
  if (AM_LIKELY(mAuthenticator && mRtspServer->mRtspNeedAuth)) {
    if (AM_LIKELY(mRtspAuthHdr.is_ok() &&
                  is_str_same(mRtspAuthHdr.realm, mAuthenticator->GetRealm()) &&
                  is_str_same(mRtspAuthHdr.nonce, mAuthenticator->GetNonce()))){
      authOK = mAuthenticator->Authenticate(req, mRtspAuthHdr);
    } else {
      authOK = false;
    }

    if (AM_UNLIKELY(!authOK)) {
      char nonce[128] = { 0 };
      char date[200] = { 0 };
      sprintf(nonce,
              "%x%x",
              mRtspServer->GetRandomNumber(),
              mRtspServer->GetRandomNumber());
      mAuthenticator->SetNonce(nonce);
      snprintf(buf,
               size,
               "RTSP/1.0 401 Unauthorized\r\n"
               "CSeq: %s\r\n%s"
               "WWW-Authenticate: Digest realm=\"%s\", "
               "nonce=\"%s\"\r\n\r\n",
               req.cseq,
               GetDateString(date, sizeof(date)),
               mAuthenticator->GetRealm(),
               mAuthenticator->GetNonce());
    }
    mRtspAuthHdr.reset();
  }

  return authOK;
}

inline void CRtspClientSession::NotFound(RtspRequest& req,
                                         char* buf,
                                         AM_UINT size)
{
  char date[200] = {0};
  snprintf(buf, size, "RTSP/1.0 404 Stream Not Found\r\nCSeq: %s\r\n%s\r\n",
           req.cseq, GetDateString(date, sizeof(date)));
}

inline char* CRtspClientSession::GetDateString(char* buf, AM_UINT size)
{
  time_t curTime = time(NULL);
  strftime(buf, size, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&curTime));

  return buf;
}

inline void CRtspClientSession::TearDown()
{
  while (!mTrackInfoQ.empty()) {
    ClientTrackInfo *trackInfo = mTrackInfoQ.front();
    CRtpSession *track = mRtspServer->GetRtpSessionById(trackInfo->id);
    if (AM_LIKELY(track)) {
      track->DelDestination(trackInfo->dest, track->GetStreamId());
    }
    delete trackInfo;
    mTrackInfoQ.pop();
  }
}

inline bool CRtspClientSession::EnableDestination()
{
  bool ret = false;
  AM_UINT size = mTrackInfoQ.size();
  for (AM_UINT i = 0; i < size; ++ i) {
    ClientTrackInfo* trackInfo = mTrackInfoQ.front();
    CRtpSession* track = mRtspServer->GetRtpSessionById(trackInfo->id);

    mTrackInfoQ.pop();
    if (AM_UNLIKELY(track && trackInfo->dest && !track->EnableDesitnation(
        *(trackInfo->dest), track->GetStreamId()))) {
      track->DelDestination(trackInfo->dest, track->GetStreamId());
      ERROR("Failed to enable destination %s:%hu mode %u",
            inet_ntoa(mClientAddr.sin_addr),
            mRtspTransHdr.clientRtpPort,
            mRtspTransHdr.mode);
      delete trackInfo;
    } else if (AM_UNLIKELY(!track)) {
      ERROR("Can not find track%u", trackInfo->id);
      mTrackInfoQ.push(trackInfo);
    } else if (AM_UNLIKELY(!trackInfo->dest)) {
      ERROR("Invalid destination!");
      delete trackInfo;
    } else {
      /* Push this id back to track queue, will be used when tear down */
      mTrackInfoQ.push(trackInfo);
      mTrackMap |= (1 << trackInfo->id);
      ret = true;
    }
  }

  return ret;
}
