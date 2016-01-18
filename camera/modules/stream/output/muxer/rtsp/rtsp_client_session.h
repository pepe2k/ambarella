/*******************************************************************************
 * rtsp_client_session.h
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

#ifndef RTSP_CLIENT_SESSION_H_
#define RTSP_CLIENT_SESSION_H_

#define  CLIENT_TIMEOUT_THRESHOLD  30

struct RtspRequest;
struct RtspAuthHeader;
struct RtspTransHeader;
struct ClientTrackInfo;

typedef std::queue<ClientTrackInfo*> TrackInfoQueue;

class CRtspAuthenticator;
class CRtspClientSession
{
    friend class CRtspServer;
    enum ParseState {
      PARSE_INCOMPLETE,
      PARSE_COMPLETE,
      PARSE_RTCP
    };

    enum ClientSessionState {
      CLIENT_SESSION_INIT,
      CLIENT_SESSION_THREAD_RUN,
      CLIENT_SESSION_OK,
      CLIENT_SESSION_FAILED,
      CLIENT_SESSION_STOPPED
    };

  public:
    enum RtcpPacketType {
      RTCP_SR   = 200,
      RTCP_RR   = 201,
      RTCP_SDES = 202,
      RTCP_BYE  = 203,
      RTCP_APP  = 204
    };
  public:
    CRtspClientSession(CRtspServer* server,
                       CRtspClientSession** location,
                       AM_U16 udpRtpPort);
    virtual ~CRtspClientSession();
    bool InitClientSession(AM_INT serverTcpSock);
    void ClientAbort();
    bool KillClient();
    void SetClientName(const char* nm)
    {
      delete[] mClientName;
      mClientName = NULL;
      if (AM_LIKELY(nm)) {
        mClientName = amstrdup(nm);
      }
    }

  private:
    static AM_ERR StaticClientThread(void* data)
    {
      return ((CRtspClientSession*)data)->ClientThread();
    }
    void CloseAllSocket();
    int  SetupServerSocketUdp(AM_U16 streamPort);
    AM_ERR ClientThread();
    bool ParseTransportHeader(RtspTransHeader& header,
                              char*            reqStr,
                              AM_UINT          reqLen);
    bool ParseAuthenticationHeader(RtspAuthHeader& header,
                                   char*           reqStr,
                                   AM_UINT         reqLen);
    void HandleRtcpPacket(char* RTCP);
    bool ParseClientRequest(RtspRequest&     request,
                            ParseState&      state,
                            char*            reqStr,
                            AM_UINT          reqLen);
    bool HandleClientRequest(RtspRequest& request);
    inline bool CmdOptions     (RtspRequest& req, char* buf, AM_UINT size);
    inline bool CmdDescribe    (RtspRequest& req, char* buf, AM_UINT size);
    inline bool CmdSetup       (RtspRequest& req, char* buf, AM_UINT size);
    inline bool CmdTeardown    (RtspRequest& req, char* buf, AM_UINT size);
    inline bool CmdPlay        (RtspRequest& req, char* buf, AM_UINT size);
    inline bool CmdGetParameter(RtspRequest& req, char* buf, AM_UINT size);
    inline bool CmdSetParameter(RtspRequest& req, char* buf, AM_UINT size);
    inline bool CmdNotSupported(RtspRequest& req, char* buf, AM_UINT size);
    inline bool CmdBadRequests (RtspRequest& req, char* buf, AM_UINT size);
    inline bool AuthenticateOK (RtspRequest& req, char* buf, AM_UINT size);
    inline void NotFound       (RtspRequest& req, char* buf, AM_UINT size);
    inline char* GetDateString (char* buf, AM_UINT size);
    inline void TearDown();
    inline bool EnableDestination();

  private:
    CRtspAuthenticator*  mAuthenticator;
    CRtspClientSession** mSelfLocation;
    CRtspServer*         mRtspServer;
    CThread*             mClientThread;
    char*                mClientName;
    TrackInfoQueue       mTrackInfoQ;
    ClientSessionState   mSessionState;
    AM_INT               mTcpSock;
    AM_INT               mUdpRtpSock;
    AM_INT               mUdpRtcpSock;
    AM_U16               mUdpRtpPort;
    AM_U16               mUdpRtcpPort;
    sockaddr_in          mClientAddr;
    AM_U32               mClientSessionId;
    AM_U32               mTrackMap;
    AM_INT               mClientCtrlSock[2];
    RtspTransHeader      mRtspTransHdr;
    RtspAuthHeader       mRtspAuthHdr;
    /* Used for RTCP RR packet handle */
    AM_U64               mRtcpRRInterval;
    AM_U64               mRtcpRRLastRecvTime;
    AM_INT               mDynamicTimeOutSec;
    AM_U32               mRtcpRRCnt;
    AM_U32               mRtcpRRCntThreshold;
    AM_U32               mRtcpRRSsrc;
#define CLIENT_CTRL_READ  mClientCtrlSock[0]
#define CLIENT_CTRL_WRITE mClientCtrlSock[1]
};

#endif /* RTSP_CLIENT_SESSION_H_ */
