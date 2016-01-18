/*******************************************************************************
 * rtsp_requests.h
 *
 * History:
 *   2013-7-8 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef RTSP_REQUESTS_H_
#define RTSP_REQUESTS_H_

struct RtspTransHeader
{
    RtspStreamMode mode;
    char* modeString;
    char* clientDstAddrString;
    AM_U16 clientRtpPort;
    AM_U16 clientRtcpPort;
    AM_U8  rtpChannelId;
    AM_U8  rtcpChannelId;
    AM_U8  clientDstTTL;
    AM_U8  reserved;
    RtspTransHeader() :
      mode(RTP_UDP),
      modeString(NULL),
      clientDstAddrString(NULL),
      clientRtpPort(0),
      clientRtcpPort(0),
      rtpChannelId(0),
      rtcpChannelId(0),
      clientDstTTL(0),
      reserved(0xff){}
    ~RtspTransHeader()
    {
      delete[] modeString;
      delete[] clientDstAddrString;
    }

    RtspTransHeader(RtspTransHeader& hdr) :
      mode(hdr.mode),
      clientRtpPort(hdr.clientRtpPort),
      clientRtcpPort(hdr.clientRtcpPort),
      rtpChannelId(hdr.rtpChannelId),
      rtcpChannelId(hdr.rtcpChannelId),
      clientDstTTL(hdr.clientDstTTL),
      reserved(0xff)
    {
      modeString = amstrdup(hdr.modeString);
      clientDstAddrString = amstrdup(hdr.clientDstAddrString);
    }

    void print_info()
    {
      INFO("         Mode: %s", modeString);
      INFO("      RtpPort: %hu", clientRtpPort);
      INFO("     RtcpPort: %hu", clientRtcpPort);
      INFO(" RtpChannelID: %hhu", rtpChannelId);
      INFO("RtcpChannelID: %hhu", rtcpChannelId);
    }

    void set_streaming_mode_str(char* str)
    {
      delete[] modeString;
      modeString = (str && (strlen(str) > 0)) ? amstrdup(str) : NULL;
    }

    void set_client_dst_addr_str(char* str)
    {
      delete[] clientDstAddrString;
      clientDstAddrString = (str && (strlen(str) > 0)) ? amstrdup(str) : NULL;
    }
};

struct RtspAuthHeader
{
    char* realm;
    char* nonce;
    char* uri;
    char* username;
    char* response;

    RtspAuthHeader() :
      realm(NULL),
      nonce(NULL),
      uri(NULL),
      username(NULL),
      response(NULL){}
    ~RtspAuthHeader()
    {
      reset();
    }

    bool is_ok() {
      return (realm && nonce && uri && username && response);
    }

    void reset() {
      delete[] realm;
      realm = NULL;
      delete[] nonce;
      nonce = NULL;
      delete[] uri;
      uri = NULL;
      delete[] username;
      username = NULL;
      delete[] response;
      response = NULL;
    }

    void set_realm(char* rlm)
    {
      delete[] realm;
      realm = amstrdup(rlm);
    }

    void set_nonce(char* nce)
    {
      delete[] nonce;
      nonce = amstrdup(nce);
    }

    void set_uri(char* i)
    {
      delete[] uri;
      uri = amstrdup(i);
    }

    void set_username(char* name)
    {
      delete[] username;
      username = amstrdup(name);
    }

    void set_response(char* pass)
    {
      delete[] response;
      response = amstrdup(pass);
    }

    void print_info()
    {
      INFO("    Realm: %s", realm);
      INFO("    Nonce: %s", nonce);
      INFO("      Uri: %s", uri);
      INFO(" UserName: %s", username);
      INFO(" Response: %s", response);
    }
};

struct RtspRequest
{
    char* command;
    char* urlPreSuffix;
    char* urlSuffix;
    char* cseq;
    RtspRequest() :
      command(NULL),
      urlPreSuffix(NULL),
      urlSuffix(NULL),
      cseq(NULL){}
    ~RtspRequest()
    {
      delete[] command;
      delete[] urlPreSuffix;
      delete[] urlSuffix;
      delete[] cseq;
    }

    void set_command(char* cmd)
    {
      delete[] command;
      command = amstrdup(cmd);
    }

    void set_url_pre_suffix(char* presuffix)
    {
      delete[] urlPreSuffix;
      urlPreSuffix = amstrdup(presuffix);
    }

    void set_url_suffix(char* suffix)
    {
      delete[] urlSuffix;
      urlSuffix = amstrdup(suffix);
    }

    void set_cseq(char* str)
    {
      delete[] cseq;
      cseq = amstrdup(str);
    }

    void print_info()
    {
      INFO("  Command: %s", command);
      INFO("PreSuffix: %s", urlPreSuffix);
      INFO("   Suffix: %s", urlSuffix);
      INFO("     CSeq: %s", cseq);
    }
};

#endif /* RTSP_REQUESTS_H_ */
