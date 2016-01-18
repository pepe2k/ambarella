
/**
 * streamming_server.cpp
 *
 * History:
 *    2011/09/06 - [Zhi He] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"

#include "streamming_if.h"

#include "streamming_server.h"
#include "random.h"
//#include "ws_discovery.h"

static void _getDateString(char* pbuffer, AM_UINT size) {
    time_t tt = time(NULL);
    strftime(pbuffer, size, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
}

static char _RTSPallowedCommandNames[] = "OPTIONS, DESCRIBE, SETUP, PLAY,TEARDOWN";

void retrieveHostString(AM_U8* buf, AM_U8* dest, AM_UINT max_len) {
    AM_U32 part1, part2, part3, part4;
    char* ptmp;
    //find "rtsp://" prefix
    ptmp = strchr((char*)buf, ':');
    if (ptmp) {
        AM_ASSERT(((AM_UINT)ptmp) > ((AM_UINT)(buf + 4)));
        if (((AM_UINT)ptmp) > ((AM_UINT)(buf + 4))) {
            AM_ASSERT(ptmp[-4] == 'r');
            AM_ASSERT(ptmp[-3] == 't');
            AM_ASSERT(ptmp[-2] == 's');
            AM_ASSERT(ptmp[-1] == 'p');
            AM_ASSERT(ptmp[1] == '/');
            AM_ASSERT(ptmp[2] == '/');
            ptmp += 3;
            if (4 == sscanf((const char*)ptmp, "%u.%u.%u.%u", &part1, &part2, &part3, &part4)) {
                AM_INFO("Get host string: %u.%u.%u.%u.\n", part1, part2, part3, part4);
                snprintf((char*)dest, max_len, "%u.%u.%u.%u", part1, part2, part3, part4);
            } else {
                AM_ERROR(" get host string(%s, %s) fail, use default 10.0.0.2.\n", buf, ptmp);
                strncpy((char*)dest, "10.0.0.2", max_len);
            }
            return;
        }
    }

    AM_ERROR("may be not valid request string, please check, use 10.0.0.2 as default.\n");
    strncpy((char*)dest, "10.0.0.2", max_len);
}

IStreammingServerManager* CreateStreamingServerManager(SConsistentConfig* pconfig)
{
    return (IStreammingServerManager*)CStreammingServerManager::Create(pconfig);
}

static int  set_sock_nonblock(int sock){
    int flags = fcntl(sock, F_GETFL, 0);
    if(flags < 0){
        return -1;
    }
    if (fcntl(sock, F_SETFL, flags|O_NONBLOCK) != 0) {
        return -1;
    }
    return 0;
}

#include <netinet/in.h>
#include <netinet/tcp.h>
static int set_rtsp_socket_options(int fd){
    int tmp = 64 * 1024;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &tmp, sizeof(tmp)) < 0) {
        return -1;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &tmp, sizeof(tmp)) < 0) {
        return -1;
    }
    //disable Nagle
    tmp =1;
    if( setsockopt(fd,  IPPROTO_TCP, TCP_NODELAY,   &tmp,   sizeof(tmp)) < 0){
        return -1;
    }
    struct linger linger_c;
    linger_c.l_onoff = 1;
    linger_c.l_linger = 0 ;
    if(setsockopt(fd,SOL_SOCKET,SO_LINGER,(const char *)&linger_c,sizeof(linger_c)) < 0){
        return -1;
    }
    tmp = 20; //20ms,TCP_DELAY_ACK
    if(setsockopt(fd,IPPROTO_TCP,0x4002,&tmp,sizeof(tmp)) < 0){
        return -1;
    }
    return 0;
}

AM_ERR CStreammingServerManager::HandleServerRequest(SStreammingServerInfo* server, EServerAction& action)
{
    SClientSessionInfo* client_session;
    CDoubleLinkedList::SNode* p_node;
    SStreamContext* p_context;
    socklen_t   clilen;
    AM_UINT i;

    AMLOG_INFO("CStreammingServerManager::HandleServerRequest server type %d, server mode %d.\n", server->type, server->mode);
    if (server->type == IParameters::StreammingServerType_RTSP) {
        switch (server->mode) {
            case IParameters::StreammingServerMode_Unicast:
            case IParameters::StreammingServerMode_MulticastSetAddr:
                //new client
                client_session = (SClientSessionInfo*)malloc(sizeof(SClientSessionInfo));
                if (NULL == client_session) {
                    AM_ERROR("NOT enough memory.\n");
                    return ME_NO_MEMORY;
                }
                memset(client_session, 0x0, sizeof(SClientSessionInfo));
                pthread_mutex_init(&client_session->lock,NULL);
                client_session->p_server = server;
                clilen = sizeof(struct sockaddr_in);
             do_retry:
                client_session->socket = accept(server->socket, (struct sockaddr*)&client_session->client_addr, &clilen);
                if (client_session->socket < 0) {
                    int err = errno;
                    if(err == EINTR || err == EAGAIN || err == EWOULDBLOCK){
                        goto do_retry;
                    }
                    AM_ERROR("accept fail, return %d.\n", client_session->socket);
                    free(client_session);
                    client_session = NULL;
                } else if(server->mClientSessionList.NumberOfNodes() >= 4){
                    AMLOG_WARN(" Reject NEW client: %s, port %d. Too many clients\n", inet_ntoa(client_session->client_addr.sin_addr), ntohs(client_session->client_addr.sin_port));
                    close(client_session->socket);
                    pthread_mutex_destroy(&client_session->lock);
                    free(client_session);
                    client_session = NULL;
                }else{
                    AMLOG_WARN(" NEW client: %s, port %d.\n", inet_ntoa(client_session->client_addr.sin_addr), ntohs(client_session->client_addr.sin_port));
                    AM_ASSERT(client_session->socket >= 0);
                    AMLOG_WARN(" NEW client socket %d.\n", client_session->socket);

                    set_sock_nonblock(client_session->socket);
                    set_rtsp_socket_options(client_session->socket);
                    //add socket to listenning list
                    FD_SET(client_session->socket, &mAllSet);
                    if (client_session->socket > mMaxFd) {
                        AMLOG_INFO(" update mMaxFd(ori %d) socket %d.\n", mMaxFd, client_session->socket);
                        mMaxFd = client_session->socket;
                    }

                    client_session->enable_video = server->enable_video;
                    client_session->enable_audio = server->enable_audio;
                    //todo, send client info to record engine, record engine would notify muxer to send data via RTP/UDP
                    //todotodotodo!!!

                    //initial sub sessions
                    for (i = 0; i < ESubSession_tot_count; i++) {
                        client_session->sub_session[i].parent = client_session;
                        client_session->sub_session[i].state = ESessionServerState_Init;//wait SETUP, to READY

                        client_session->sub_session[i].track_id = i;
                        client_session->sub_session[i].dst_addr.addr = (AM_U32)ntohl(client_session->client_addr.sin_addr.s_addr);
                        client_session->sub_session[i].dst_addr.socket = client_session->socket;
                        client_session->sub_session[i].dst_addr.protocol_type = IParameters::ProtocolType_UDP;//hard code
                    }

                    client_session->session_id = 0; //hard code

                    //choose first content, hard code here
                    p_node = server->mContentList.FirstNode();
                    AM_ASSERT(p_node);
                    if (p_node) {
                        p_context = (SStreamContext*)p_node->p_context;
                        AM_ASSERT(p_context);
                        if (p_context) {
                            AM_ASSERT(p_context->content.mUrl);
                            client_session->p_session_name = &p_context->content.mUrl[0];
                            //AMLOG_INFO("{{{server %p}}}.\n", p_context);
                            //AMLOG_INFO("{{{server}}}, p_content->content.mUrl %p, %s.\n", p_context->content.mUrl, p_context->content.mUrl);
                            //AMLOG_INFO(" 1: client_session(%p)->p_session_name %s.\n", client_session, client_session->p_session_name);
                            //client_session->p_sdp_info = generate_sdp_description(p_context->content.mUrl, p_context->content.mUrl, "Session streamed by \"Amba IOne RTSP Server\"", 0, 0);//hard code here
                            //client_session->sdp_info_len = strlen(client_session->p_sdp_info);
                            //AMLOG_INFO(" 2: client_session(%p)->p_session_name %s.\n", client_session, client_session->p_session_name);
                            client_session->port = server->listenning_port;
                            client_session->p_data_transmiter = p_context->p_data_transmeter;
                            client_session->p_context = (void*)p_context;
                            AM_ASSERT(client_session->p_data_transmiter);
                            if (client_session->p_data_transmiter) {
                                AM_ASSERT(client_session->enable_video);
                                if (client_session->enable_video) {
                                    AMLOG_INFO("server set client's source port(video) %hu, %hu.\n", server->current_data_port_base, server->current_data_port_base + 1);
                                    client_session->p_data_transmiter->SetSrcPort(server->current_data_port_base, server->current_data_port_base + 1, IParameters::StreamType_Video);
                                }
                                if (client_session->enable_audio) {
                                    AMLOG_INFO("server set client's source port(audio) %hu, %hu.\n", server->current_data_port_base + 2, server->current_data_port_base + 3);
                                    client_session->p_data_transmiter->SetSrcPort(server->current_data_port_base + 2, server->current_data_port_base + 3, IParameters::StreamType_Audio);
                                }
                                server->current_data_port_base += 8;//update
                            }
                        }
                        AM_ASSERT(p_context->p_data_transmeter);
                    }
                    pthread_mutex_lock(&client_session->lock);
                    client_session->last_cmd_time = get_time();
                    pthread_mutex_unlock(&client_session->lock);
                    server->mClientSessionList.InsertContent(NULL, (void*)client_session, 1);
                }
                break;

            case IParameters::StreammingServerMode_MultiCastPickAddr:
                AMLOG_INFO("conference request? server pickup multicast addr.\n");
                //todo
                break;

            default:
                AM_ERROR("BAD server->mode %d.\n", server->mode);
                break;
        }
    }
    return ME_OK;
}

bool CStreammingServerManager::parseRTSPRequestString(char const* reqStr,
                AM_U32 reqStrSize,
                char* resultCmdName,
                AM_U32 resultCmdNameMaxSize,
                char* resultURLPreSuffix,
                AM_U32 resultURLPreSuffixMaxSize,
                char* resultURLSuffix,
                AM_U32 resultURLSuffixMaxSize,
                char* resultCSeq,
                AM_U32 resultCSeqMaxSize) {
    // This parser is currently rather dumb; it should be made smarter #####
    //AMLOG_DEBUG("parse start.\n");
    // Read everything up to the first space as the command name:
    bool parseSucceeded = false;
    AM_U32 i, j , k;
    for (i = 0; i < resultCmdNameMaxSize-1 && i < reqStrSize; ++i) {
        char c = reqStr[i];
        if (c == ' ' || c == '\t') {
            parseSucceeded = true;
            break;
        }
        resultCmdName[i] = c;
    }
    resultCmdName[i] = '\0';
    //AMLOG_DEBUG(" get resultCmd %s, i %d.\n", resultCmdName, i);
    if (!parseSucceeded) {
        AM_ERROR("parer cmd Error.\n");
        return false;
    }

    // Skip over the prefix of any "rtsp://" or "rtsp:/" URL that follows:
    j = i+1;
    while (j < reqStrSize && (reqStr[j] == ' ' || reqStr[j] == '\t')) ++j; // skip over any additional white space
    //AMLOG_DEBUG("parse after stage 1, j %d.\n", j);

    for (; (int)j < (int)(reqStrSize-8); ++j) {
        if ((reqStr[j] == 'r' || reqStr[j] == 'R')
            && (reqStr[j+1] == 't' || reqStr[j+1] == 'T')
            && (reqStr[j+2] == 's' || reqStr[j+2] == 'S')
            && (reqStr[j+3] == 'p' || reqStr[j+3] == 'P')
            && reqStr[j+4] == ':' && reqStr[j+5] == '/') {
            j += 6;	// skip over "RTSP:/" denotation
            if (reqStr[j] == '/') {
                // This is a "rtsp://" URL; skip over the host:port part that follows:
                ++j;
                while (j < reqStrSize && reqStr[j] != '/' && reqStr[j] != ' ') ++j;
            } else {
                // This is a "rtsp:/" URL; back up to the "/":
                --j;
            }
            i = j;
            break;
        }
    }
    //AMLOG_DEBUG("parse after stage 2, j %d, i %d.\n", j, i);

    // Look for the URL suffix (before the following "RTSP/1.0"):
    parseSucceeded = false;
    for (k = i+1; (int)k < (int)(reqStrSize-5); ++k) {

        if (reqStr[k] == 'R' && reqStr[k+1] == 'T' && reqStr[k+2] == 'S' && reqStr[k+3] == 'P' && reqStr[k+4] == '/') {

            while (--k >= i && reqStr[k] == ' ') {} // go back over all spaces before "RTSP/"
            AM_U32 k1 = k;
            while (k1 > i && reqStr[k1] != '/') --k1; // go back to the first '/'
            // the URL suffix comes from [k1+1,k]

            // Copy "resultURLSuffix":
            if (k - k1 + 1 > resultURLSuffixMaxSize) return false; // there's no room
            AM_U32 n = 0, k2 = k1+1;
            while (k2 <= k) resultURLSuffix[n++] = reqStr[k2++];
            resultURLSuffix[n] = '\0';

            // Also look for the URL 'pre-suffix' before this:
            unsigned k3 = (k1 == 0) ? 0 : --k1;
            while (k3 > i && reqStr[k3] != '/') --k3; // go back to the first '/'
            // the URL pre-suffix comes from [k3+1,k1]

            // Copy "resultURLPreSuffix":
            if (k1 - k3 + 1 > resultURLPreSuffixMaxSize) return false; // there's no room
            n = 0; k2 = k3+1;
            while (k2 <= k1) resultURLPreSuffix[n++] = reqStr[k2++];
            resultURLPreSuffix[n] = '\0';

            i = k + 7; // to go past " RTSP/"
            parseSucceeded = true;
            break;
        }
    }
    //AMLOG_DEBUG("parse after stage 3, k %d, i %d.\n", k, i);
    if (!parseSucceeded) {
        AM_ERROR("parer url Error.\n");
        return false;
    }

    // Look for "CSeq:", skip whitespace,
    // then read everything up to the next \r or \n as 'CSeq':
    parseSucceeded = false;
    for (j = i; (int)j < (int)(reqStrSize-5); ++j) {

        if (reqStr[j] == 'C' && reqStr[j+1] == 'S' && reqStr[j+2] == 'e' &&
            reqStr[j+3] == 'q' && reqStr[j+4] == ':') {
            j += 5;
            unsigned n;
            while (j < reqStrSize && (reqStr[j] ==  ' ' || reqStr[j] == '\t')) ++j; // skip over any additional white space
            for (n = 0; n < resultCSeqMaxSize-1 && j < reqStrSize; ++n,++j) {
                char c = reqStr[j];
                if (c == '\r' || c == '\n') {
                    parseSucceeded = true;
                    break;
                }
                resultCSeq[n] = c;
            }
            resultCSeq[n] = '\0';
            break;
        }
    }
    //AMLOG_DEBUG("parse after stage 4, j %d, i %d.\n", j , i);
    if (!parseSucceeded) {
        AM_ERROR("parer CSeq Error.\n");
        return false;
    }

    return true;
}

AM_ERR CStreammingServerManager::HandleClientRequest(SStreammingServerInfo* server_info, SClientSessionInfo* p_client, ESessionMethod& method)
{
    //todo
    #define RTSP_PARAM_STRING_MAX	256
//    int bytesRead;
    char cmdName[RTSP_PARAM_STRING_MAX];
    char urlPreSuffix[RTSP_PARAM_STRING_MAX];
    char urlSuffix[RTSP_PARAM_STRING_MAX];
    char cseq[RTSP_PARAM_STRING_MAX];

    AMLOG_DEBUG("CStreammingServerManager::HandleClientRequest server %p, client %p.\n", server_info, p_client);
    AMLOG_INFO("get Request:len[%d]%s\n", mRequestBufferLen,mRequestBuffer);

    if (parseRTSPRequestString((char*)mRequestBuffer, mRequestBufferLen,
        cmdName, sizeof cmdName,
        urlPreSuffix, sizeof urlPreSuffix,
        urlSuffix, sizeof urlSuffix,
        cseq, sizeof cseq)) {

        AMLOG_VERBOSE("cmdName %s.\n", cmdName);
        AMLOG_VERBOSE("urlPreSuffix %s.\n", urlPreSuffix);
        AMLOG_VERBOSE("urlSuffix %s.\n", urlSuffix);
        AMLOG_VERBOSE("cseq %s.\n", cseq);
        AMLOG_VERBOSE("mRequestBuffer %s.\n", mRequestBuffer);

        if (strcmp(cmdName, "OPTIONS") == 0) {
            handleCmd_OPTIONS(cseq);
        } else if (strcmp(cmdName, "DESCRIBE") == 0) {
            AMLOG_VERBOSE("before handleCmd_DESCRIBE.\n");
            handleCmd_DESCRIBE(p_client, cseq, urlSuffix, (char const*)mRequestBuffer);
        } else if (strcmp(cmdName, "SETUP") == 0) {
            handleCmd_SETUP(server_info, p_client, cseq, urlPreSuffix, urlSuffix, (char const*)mRequestBuffer);
        } else if (strcmp(cmdName, "TEARDOWN") == 0
                || strcmp(cmdName, "PLAY") == 0
                || strcmp(cmdName, "PAUSE") == 0
                || strcmp(cmdName, "GET_PARAMETER") == 0
                || strcmp(cmdName, "SET_PARAMETER") == 0) {
            //for withSession case, response int handleCmd
            return handleCmd_withinSession(server_info, p_client, cmdName, urlPreSuffix, urlSuffix, cseq, (char const*)mRequestBuffer);
        } else {
            handleCmd_notSupported(cseq);
        }
        pthread_mutex_lock(&p_client->lock);
        p_client->last_cmd_time = get_time();
        pthread_mutex_unlock(&p_client->lock);
    } else {
        AM_ERROR("parseRTSPRequestString() failed\n");
        handleCmd_bad(cseq);
    }

    AMLOG_INFO("response(%d):%s\n", strlen(mResponseBuffer), mResponseBuffer);

    if(tcp_write(p_client->socket, (char const*)mResponseBuffer, strlen(mResponseBuffer)) < 0){
        return ME_CLOSED;
    }

    return ME_OK;
}

CStreammingServerManager::CStreammingServerManager(SConsistentConfig* pconfig):
    mpWorkQueue(NULL),
    mpName(NULL),
    mbRun(true),
    mpContentProvider(NULL),
    mMaxFd(-1),
    mnServerNumber(0),
    mpMutex(NULL),
    mRtspSessionId(0),
    mRtspSSRC(0x124365),
    mpConfig(pconfig)
{
    mPipeFd[0] = mPipeFd[1] = -1;
    pthread_mutex_init(&mTcpSendLock,NULL);
}

void *CStreammingServerManager::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IStreammingServerManager)
        return (IStreammingServerManager*)this;
    else if (refiid == IID_IActiveObject)
        return (IActiveObject*)this;

    return inherited::GetInterface(refiid);
}

IStreammingServerManager* CStreammingServerManager::Create(SConsistentConfig* pconfig)
{
    CStreammingServerManager *result = new CStreammingServerManager(pconfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CStreammingServerManager::Construct()
{
    mpMutex = CMutex::Create();
    if (NULL == mpMutex) {
        AM_ERROR("Create mutex fail.\n");
        return ME_OS_ERROR;
    }

    DSetModuleLogConfig(LogModuleStreamingServerManager);

    if ((mpWorkQueue = CWorkQueue::Create((IActiveObject*)this)) == NULL) {
        AM_ERROR("Create CWorkQueue fail.\n");
        return ME_NO_MEMORY;
    }

    pipe(mPipeFd);

    AMLOG_INFO("before CStreammingServerManager:: mpWorkQueue->Run().\n");
    AM_ASSERT(mpWorkQueue);
    mpWorkQueue->Run();
    AMLOG_INFO("after CStreammingServerManager:: mpWorkQueue->Run().\n");

    return ME_OK;
}

void CStreammingServerManager::deleteClientSession(SClientSessionInfo* p_session)
{
    AM_ASSERT(p_session);
    AM_ASSERT(p_session->p_data_transmiter);
    AM_ASSERT(p_session->p_server);
    AM_ASSERT(p_session->p_sdp_info);
    if (!p_session) {
        return;
    }

    if (p_session->p_data_transmiter) {
        p_session->p_data_transmiter->RemoveDstAddress(&p_session->sub_session[ESubSession_audio].dst_addr, IParameters::StreamType_Audio);
        p_session->p_data_transmiter->RemoveDstAddress(&p_session->sub_session[ESubSession_video].dst_addr, IParameters::StreamType_Video);
    }

    if (p_session->p_server) {
        p_session->p_server->mClientSessionList.RemoveContent(p_session);
    }

    if (p_session->p_sdp_info) {
        free(p_session->p_sdp_info);
    }

    close(p_session->socket);
    pthread_mutex_destroy(&p_session->lock);

    free(p_session);
}

void CStreammingServerManager::deleteSDPSession(void* )
{
    //todo
    AM_ASSERT(0);
}

void CStreammingServerManager::deleteServer(SStreammingServerInfo* p_server)
{
    CDoubleLinkedList::SNode* pnode;

    AMLOG_INFO("CStreammingServerManager::deleteServer, %p start.\n", p_server);
    AM_ASSERT(p_server);
    if (!p_server) {
        AM_ERROR("NULL p_server pointer.\n");
        return;
    }

    AMLOG_INFO("CStreammingServerManager::deleteServer, start clean client list.\n");

    //clean each client session
    SClientSessionInfo* p_client;
    pnode = p_server->mClientSessionList.FirstNode();
    while (pnode) {
        p_client = (SClientSessionInfo*)(pnode->p_context);
        pnode = p_server->mClientSessionList.NextNode(pnode);
        AM_ASSERT(p_client);
        if (p_client) {
            deleteClientSession(p_client);
        } else {
            AM_ASSERT("NULL pointer here, something would be wrong.\n");
        }
    }

    //clean each SDP, todo
    AM_WARNING(" TODO: need clean SDP list.\n");

    AMLOG_INFO("CStreammingServerManager::deleteServer, before close socket %d.\n", p_server->socket);
    //close server socket
    if (p_server->socket >= 0) {
        close(p_server->socket);
        p_server->socket = -1;
    }

    AMLOG_INFO("CStreammingServerManager::deleteServer, before free url %p.\n", p_server->p_rtsp_url);
    if (p_server->p_rtsp_url) {
        AMLOG_DEBUG("CStreammingServerManager::deleteServer, url: %s.\n", p_server->p_rtsp_url);
        free(p_server->p_rtsp_url);
        p_server->p_rtsp_url = NULL;
    }

    AMLOG_INFO("CStreammingServerManager::deleteServer, before delete p_server.\n");
    delete p_server;

    AMLOG_INFO("CStreammingServerManager::deleteServer, after delete p_server.\n");
}

void CStreammingServerManager::Delete()
{
    //todo
    CDoubleLinkedList::SNode* pnode;
    AMLOG_DEBUG("CStreammingServerManager::deleteServer, after delete p_server.\n");

    //clean servers
    SStreammingServerInfo* p_server;
    pnode = mServerList.FirstNode();
    while (pnode) {
        p_server = (SStreammingServerInfo*)(pnode->p_context);
        pnode = mServerList.NextNode(pnode);
        AM_ASSERT(p_server);
        if (p_server) {
            deleteServer(p_server);
        } else {
            AM_ASSERT("NULL pointer here, something would be wrong.\n");
        }
    }

}

bool CStreammingServerManager::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("****CStreammingServerManager::ProcessCmd, cmd.code %d.\n", cmd.code);
    AM_ASSERT(mpWorkQueue);
    SStreammingServerInfo* server_info;
    char char_buffer;

    //bind with pipe fd
    read(mPipeFd[0], &char_buffer, sizeof(char_buffer));
    //AMLOG_INFO("****CStreammingServerManager::ProcessCmd, cmd.code %d, char %c.\n", cmd.code, char_buffer);

    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            //clear all running server, todo
            msState = EServerManagerState_halt;
            mpWorkQueue->CmdAck(ME_OK);
            break;

        case CMD_START:
            msState = EServerManagerState_noserver_alive;
            mpWorkQueue->CmdAck(ME_OK);
            break;

        case CMD_ADD_SERVER:
            AM_ASSERT(cmd.pExtra);
            server_info = (SStreammingServerInfo*)cmd.pExtra;
            if (server_info) {
                AMLOG_INFO("CStreammingServerManager::ProcessCmd, CMD_ADD_SERVER, %p, type %d, mode %d, socket %d, port %d.\n", server_info, server_info->type, server_info->mode, server_info->socket, server_info->listenning_port);

                //need add socket into set
                AM_ASSERT(server_info->socket >= 0);
                FD_SET(server_info->socket, &mAllSet);

                server_info->state = EServerState_running;

                mServerList.InsertContent(NULL, (void*)server_info, 0);
                mpWorkQueue->CmdAck(ME_OK);
            } else {
                AM_ERROR("NULL pointer here, must have errors.\n");
                mpWorkQueue->CmdAck(ME_BAD_COMMAND);
            }
            break;

        case CMD_REMOVE_SERVER:
            AM_ASSERT(cmd.pExtra);
            server_info = (SStreammingServerInfo*)cmd.pExtra;
            if (server_info) {
                AMLOG_INFO("CStreammingServerManager::ProcessCmd, CMD_REMOVE_SERVER, %p, type %d, mode %d, socket %d, port %d.\n", server_info, server_info->type, server_info->mode, server_info->socket, server_info->listenning_port);

                //need add socket into set
                AM_ASSERT(server_info->socket >= 0);
                FD_CLR(server_info->socket, &mAllSet);

                server_info->state = EServerState_closed;

                mServerList.RemoveContent((void*)server_info);
                mpWorkQueue->CmdAck(ME_OK);
            } else {
                AM_ERROR("NULL pointer here, must have errors.\n");
                mpWorkQueue->CmdAck(ME_BAD_COMMAND);
            }
            break;

        default:
            AM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }
    return false;
}

AM_ERR CStreammingServerManager::PrintState()
{
    AMLOG_WARN("CStreammingServerManager state %d.\n", msState);
    return ME_OK;
}

#include <signal.h>
void CStreammingServerManager::OnRun()
{
    CMD cmd;
    AM_UINT alive_server_num = 0;
    AM_INT nready = 0;
    SStreammingServerInfo* p_server;
    CDoubleLinkedList::SNode* pnode;
    EServerAction server_action = EServerAction_Invalid;
    mpWorkQueue->CmdAck(ME_OK);

    signal(SIGPIPE,SIG_IGN);
    mLastCheckTime = get_time();
    //init
    FD_ZERO(&mAllSet);
    FD_SET(mPipeFd[0], &mAllSet);
    mMaxFd = mPipeFd[0];

    //init state
    msState = EServerManagerState_idle;

    while (mbRun) {

        AMLOG_STATE("CStreammingServerManager::OnRun start switch state %d.\n", msState);

        switch (msState) {

            case EServerManagerState_idle:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case EServerManagerState_noserver_alive:
                AM_ASSERT(!alive_server_num);
                //clear FD set here?
                //FD_ZERO(&mAllSet);
                //FD_SET(mPipeFd[0], &mAllSet);
                //alive_server_num = 0;

                //start server needed running, todo
                pnode = mServerList.FirstNode();
                while (pnode) {
                    p_server = (SStreammingServerInfo*)pnode->p_context;
                    AM_ASSERT(p_server);
                    if (NULL == p_server) {
                        AM_ERROR("Fatal error, No server? must not get here.\n");
                        break;
                    }

                    //add to FD set
                    if (p_server->state == EServerState_running) {
                        AM_ASSERT(p_server->socket != -1);
                        if (p_server->socket >= 0) {
                            FD_SET(p_server->socket, &mAllSet);
                            mMaxFd = (mMaxFd < p_server->socket) ? p_server->socket : mMaxFd;
                            alive_server_num ++;
                        }
                    }

                    pnode = mServerList.NextNode(pnode);
                }
                if (alive_server_num) {
                    AMLOG_INFO("There's some(%d) server alive, transit to running state.\n", alive_server_num);
                    msState = EServerManagerState_running;
                    break;
                }
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case EServerManagerState_running:
                AM_ASSERT(alive_server_num > 0);
                mReadSet = mAllSet;
                AMLOG_DEBUG("[ServerManager]: before select.\n");
                //
                struct timeval timeout;
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                while( (nready = select(mMaxFd+1, &mReadSet, NULL, NULL, &timeout)) < 0 && errno == EINTR);
                checkClientsTimeout();
                if(nready < 0){
                    msState = EServerManagerState_error;
                    break;
                }else if(nready == 0){
                    break;
                }
                AMLOG_DEBUG("[ServerManager]: after select.\n");
                //process cmd
                if (FD_ISSET(mPipeFd[0], &mReadSet)) {
                    AMLOG_DEBUG("[ServerManager]: from pipe fd.\n");
                    //some input from engine, process cmd first
                    while (mpWorkQueue->MsgQ()->PeekMsg(&cmd,sizeof(cmd))) {
                        ProcessCmd(cmd);
                    }
                    nready --;
                    if (EServerManagerState_running != msState) {
                        AMLOG_INFO(" transit from EServerManagerState_running to state %d.\n", msState);
                        break;
                    }
                    if (nready <= 0) {
                        //read done
                        break;
                    }
                }

                //is request to server?
                pnode = mServerList.FirstNode();

                while (pnode) {
                    p_server = (SStreammingServerInfo*)pnode->p_context;
                    AM_ASSERT(p_server);
                    if (NULL == p_server) {
                        AM_ERROR("Fatal error(NULL == p_server), must not get here.\n");
                        break;
                    }

                    AM_ASSERT(p_server->socket >= 0);
                    if (p_server->socket >= 0) {
                        if (FD_ISSET(p_server->socket, &mReadSet)) {
                            nready --;
                            //new client's request comes
                            HandleServerRequest(p_server, server_action);
                            AMLOG_INFO("HandleServerRequest p_server %p, action %d, socket %d.\n", p_server, server_action, p_server->socket);
                            //choose action according to server_action
                        } else {
                            //search in current client list
                            ScanClientList(p_server, nready);
                        }
                    }

                    if (nready <= 0) {
                        //read done
                        break;
                    }
                    pnode = mServerList.NextNode(pnode);
                }
                break;

            case EServerManagerState_halt:
                //todo
                AM_ERROR("NEED implement this case.\n");
                break;

            case EServerManagerState_error:
                //todo
                AM_ERROR("NEED implement this case.\n");
                break;

        }
    }
}

void CStreammingServerManager::checkClientsTimeout()
{
    AM_S64 curr_time = get_time();
    if(curr_time > mLastCheckTime){
        if(curr_time - mLastCheckTime < 1000000){
            return;
        }
    }
    mLastCheckTime = curr_time;

    CDoubleLinkedList::SNode* pnode = mServerList.FirstNode();
    while (pnode) {
        SStreammingServerInfo *p_server = (SStreammingServerInfo*)pnode->p_context;
        if (NULL == p_server) {
            AM_ERROR("Fatal error(NULL == p_server), must not get here.\n");
            break;
        }
        doCheckClientsTimeout(p_server);
        pnode = mServerList.NextNode(pnode);
    }
}

void CStreammingServerManager::doCheckClientsTimeout(SStreammingServerInfo* server_info)
{
    CDoubleLinkedList::SNode* pnode;
    SClientSessionInfo* p_client;

    if (server_info) {
        pnode = server_info->mClientSessionList.FirstNode();
        while (pnode) {
            p_client = (SClientSessionInfo*)(pnode->p_context);
            pnode = server_info->mClientSessionList.NextNode(pnode);
            AM_ASSERT(p_client);
            if (p_client) {
                 /* check TCP connection alive or not,  current timeout is 40 seconds,TODO*/
                 AM_S64 last_cmd_time;
                 pthread_mutex_lock(&p_client->lock);
                 last_cmd_time = p_client->last_cmd_time;
                 pthread_mutex_unlock(&p_client->lock);
                 if ((get_time() - last_cmd_time) / 1000000 >= 40) {
                     AMLOG_WARN(" Client Session Timeout, client: %s, port %d.\n", inet_ntoa(p_client->client_addr.sin_addr), ntohs(p_client->client_addr.sin_port));
                     FD_CLR(p_client->socket, &mReadSet);
                     FD_CLR(p_client->socket, &mAllSet);
                     deleteClientSession(p_client);
                 }
            } else {
                AM_ASSERT("NULL pointer here, something would be wrong.\n");
            }
        }
    } else {
        AM_ERROR("NULL pointer here, something would be wrong.\n");
    }
}

void CStreammingServerManager::ScanClientList(SStreammingServerInfo* server_info, AM_INT& nready)
{
    CDoubleLinkedList::SNode* pnode;
    SClientSessionInfo* p_client;
    ESessionMethod session_method = ESessionMethod_INVALID;

    if (server_info) {
        pnode = server_info->mClientSessionList.FirstNode();
        while (pnode) {
            p_client = (SClientSessionInfo*)(pnode->p_context);
            pnode = server_info->mClientSessionList.NextNode(pnode);
            AM_ASSERT(p_client);
            if (p_client) {
                //process client
                if (FD_ISSET(p_client->socket, &mReadSet)) {
                    if (ME_CLOSED == handle_rtsp_request(server_info, p_client, session_method)) {
                        FD_CLR(p_client->socket, &mReadSet);
                        FD_CLR(p_client->socket, &mAllSet);
                        deleteClientSession(p_client);
                    }
                    AMLOG_INFO(" Get Client Session method %d, socket %d, p_client %p.\n", p_client->socket, session_method, p_client);
                    nready--;
                }
            } else {
                AM_ASSERT("NULL pointer here, something would be wrong.\n");
            }

            if (nready <= 0) {
                //read done
                break;
            }
        }
    } else {
        AM_ERROR("NULL pointer here, something would be wrong.\n");
    }
}

AM_ERR CStreammingServerManager::StartServerManager()
{
    AM_ERR err;
    AM_ASSERT(mpWorkQueue);

    AMLOG_INFO("CStreammingServerManager::StartServerManager start.\n");
    //wake up server manager, post cmd to it
    char wake_char = 'a';
    write(mPipeFd[1], &wake_char, 1);

    err = mpWorkQueue->SendCmd(CMD_START, NULL);
    AMLOG_INFO("CStreammingServerManager::StartServerManager end, ret %d.\n", err);
    return err;
}

AM_ERR CStreammingServerManager::StopServerManager()
{
    AM_ERR err;
    AM_ASSERT(mpWorkQueue);

    AMLOG_INFO("CStreammingServerManager::StopServerManager start.\n");
    //wake up server manager, post cmd to it
    char wake_char = 'b';
    write(mPipeFd[1], &wake_char, 1);

    err = mpWorkQueue->SendCmd(CMD_STOP, NULL);
    AMLOG_INFO("CStreammingServerManager::StopServerManager end, ret %d.\n", err);
    return err;
}

SStreammingServerInfo* CStreammingServerManager::AddServer(IParameters::StreammingServerType type, IParameters::StreammingServerMode mode, AM_UINT server_port, AM_U8 enable_video, AM_U8 enable_audio)
{
    AM_ASSERT(mpWorkQueue);
    SStreammingServerInfo* p_server = new SStreammingServerInfo();
    if (NULL == p_server) {
        AM_ERROR("!!!NOT enough memory? in CStreammingServerManager::AddServer.\n");
        return NULL;
    }
    AM_INT socket = SetupStreamSocket(INADDR_ANY, server_port, 1);
    if (socket < 0) {
        AM_ERROR("setup socket fail, ret %d.\n", socket);
        return NULL;
    }
    AMLOG_INFO("AddServer, socket %d.\n", socket);

    int val = 2; /**/
    setsockopt(socket,IPPROTO_TCP, TCP_DEFER_ACCEPT, &val, sizeof(val));

    p_server->type = type;
    p_server->mode = mode;
    p_server->listenning_port = server_port;
    if (mpConfig) {
        p_server->current_data_port_base = mpConfig->rtsp_server_config.rtp_rtcp_port_start;
    } else {
        p_server->current_data_port_base = DefaultRTPServerPortBase;
    }
    p_server->socket = socket;
    p_server->state = EServerState_running;
    AMLOG_INFO("AddServer type %d, mode %d, port %d, return socket %d, enable video %d, enable audio %d.\n", type, mode, server_port, socket, enable_video, enable_audio);

    p_server->enable_video = enable_video;
    p_server->enable_audio = enable_audio;

    //wake up server manager, send cmd to it
    char wake_char = 'c';
    write(mPipeFd[1], &wake_char, 1);

    mpWorkQueue->SendCmd(CMD_ADD_SERVER, (void*)p_server);

    //TODO, now only support  main-stream streaming
    //WSDiscoveryService::GetInstance()->setRtspInfo(server_port, "stream_0");
    //WSDiscoveryService::GetInstance()->start();
    return p_server;
}

AM_ERR CStreammingServerManager::CloseServer(SStreammingServerInfo* server)
{
    AUTO_LOCK(mpMutex);
    server->state = EServerState_closed;
    return ME_OK;
}

AM_ERR CStreammingServerManager::RemoveServer(SStreammingServerInfo* server)
{
    AM_ASSERT(mpWorkQueue);
    //wake up server manager, post cmd to it
    char wake_char = 'd';
    write(mPipeFd[1], &wake_char, 1);

    mpWorkQueue->SendCmd(CMD_REMOVE_SERVER, (void*)server);
    //WSDiscoveryService::GetInstance()->Delete();
    return ME_OK;
}

AM_ERR CStreammingServerManager::AddStreammingContent(SStreammingServerInfo* server_info, SStreamContext* content)
{
    //AMLOG_INFO("AddStreamingContent, this %p, %s.\n", content, content->content.mUrl);
    AMLOG_INFO("AddStreamingContent, content index %d, p_data_transmeter %p.\n", content->index, content->p_data_transmeter);
    server_info->mContentList.InsertContent(NULL, (void *) content, 0);
    return ME_OK;
}

void CStreammingServerManager::handleCmd_bad(char const* /*cseq*/) {
    _getDateString(mDateBuffer, sizeof(mDateBuffer));
    // Don't do anything with "cseq", because it might be nonsense
    snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
        "RTSP/1.0 400 Bad Request\r\n%sAllow: %s\r\n\r\n",
        mDateBuffer, _RTSPallowedCommandNames);
}

void CStreammingServerManager::handleCmd_notSupported(char const* cseq) {
    _getDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
        "RTSP/1.0 405 Method Not Allowed\r\nCSeq: %s\r\n%sAllow: %s\r\n\r\n",
        cseq, mDateBuffer, _RTSPallowedCommandNames);
}

void CStreammingServerManager::handleCmd_notFound(char const* cseq) {
    _getDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
        "RTSP/1.0 404 Stream Not Found\r\nCSeq: %s\r\n%s\r\n",
        cseq, mDateBuffer);
}

void CStreammingServerManager::handleCmd_unsupportedTransport(char const* cseq) {
    _getDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
        "RTSP/1.0 461 Unsupported Transport\r\nCSeq: %s\r\n%s\r\n",
        cseq, mDateBuffer);
}

void CStreammingServerManager::handleCmd_OPTIONS(char const* cseq) {
    _getDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sPublic: %s\r\n\r\n",
        cseq, mDateBuffer, _RTSPallowedCommandNames);
}

void CStreammingServerManager::handleCmd_DESCRIBE(SClientSessionInfo* p_client, char const* cseq,
        char const* urlSuffix, char const* fullRequestStr) {
    char rtspURL[128];

    AM_ASSERT(p_client);
    AM_ASSERT(p_client->p_session_name);
    //AMLOG_INFO("p_client->p_session_name %p.\n", p_client->p_session_name);
    //AMLOG_INFO(" 11111111111p_client->p_session_name %s.\n", p_client->p_session_name);
    _getDateString(mDateBuffer, sizeof(mDateBuffer));

    if (!p_client->get_host_string) {
        retrieveHostString((AM_U8*)fullRequestStr, p_client->host_addr, sizeof(p_client->host_addr));
        p_client->get_host_string = 1;
    }

    snprintf(rtspURL, sizeof rtspURL, "rtsp://%s:%d/%s", p_client->host_addr, p_client->port, p_client->p_session_name);

    if (!p_client->p_sdp_info) {
        if (p_client->enable_video && p_client->enable_audio) {
            p_client->p_sdp_info = generate_sdp_description_h264_aac(p_client->p_context, p_client->p_session_name, p_client->p_session_name, "Session streamed by \"Amba RTSP Server\"", (const char*)p_client->host_addr, rtspURL, 0, 0);
        } else if (p_client->enable_video) {
            p_client->p_sdp_info = generate_sdp_description_h264(p_client->p_context, p_client->p_session_name, p_client->p_session_name, "Session streamed by \"Amba RTSP Server\"", (const char*)p_client->host_addr, rtspURL, 0, 0);
        }else if(p_client->enable_audio) {
            p_client->p_sdp_info = generate_sdp_description_aac(p_client->p_context, p_client->p_session_name, p_client->p_session_name, "Session streamed by \"Amba RTSP Server\"", (const char*)p_client->host_addr, rtspURL, 0, 0);
        } else {
            AM_ERROR("!!!video is not enabled? please check code, should not comes here.\n");
            return;
        }
        p_client->sdp_info_len = strlen(p_client->p_sdp_info);
    } else {
        AMLOG_WARN("Already have SDP string.\n");
    }

    AMLOG_INFO(" SDP %d:%s\n", p_client->sdp_info_len, p_client->p_sdp_info);
    AMLOG_INFO(" p_client->p_session_name %s.\n", p_client->p_session_name);

    //AMLOG_INFO(" CStreammingServerManager::handleCmd_DESCRIBE 1.\n");
    // Also, generate our RTSP URL, for the "Content-Base:" header
    // (which is necessary to ensure that the correct URL gets used in
    // subsequent "SETUP" requests).

    //AMLOG_INFO(" CStreammingServerManager::handleCmd_DESCRIBE 2.\n");
    snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
        "%s"
        "Content-Base: %s/\r\n"
        "Content-Type: application/sdp\r\n"
        "Content-Length: %d\r\n\r\n"
        "%s",
        cseq,
        mDateBuffer,
        rtspURL,
        p_client->sdp_info_len,
        p_client->p_sdp_info);
}


void CStreammingServerManager::handleCmd_SETUP(SStreammingServerInfo* server_info, SClientSessionInfo* p_client, char const* cseq,
        char const* urlPreSuffix, char const* urlSuffix,
        char const* fullRequestStr) {

    // "urlPreSuffix" should be the session (stream) name, and
    // "urlSuffix" should be the subsession (track) name.
//    char const* pStreamName = urlPreSuffix;
    AM_UINT track_id = 0;
    IParameters::StreamType track_type;


    _getDateString(mDateBuffer, sizeof(mDateBuffer));
    IParameters::ProtocolType streamingMode;
    char streamingModeString[16]; // set when RAW_UDP streaming is specified
    char clientsDestinationAddressStr[16];
    AM_U8 clientsDestinationTTL;
    AM_U16 clientRTPPort, clientRTCPPort;
    unsigned char rtpChannelId, rtcpChannelId;

    parseTransportHeader(fullRequestStr, streamingMode, streamingModeString,
    clientsDestinationAddressStr, &clientsDestinationTTL,
    &clientRTPPort, &clientRTCPPort,
    &rtpChannelId, &rtcpChannelId);

    if (urlSuffix) {
        AMLOG_INFO("before getTrackID, url %s.\n", urlSuffix);
        track_id = getTrackID((char*)urlSuffix, (char*)urlSuffix + (strlen(urlSuffix) -4));
        AMLOG_INFO("after getTrackID, get track id %d.\n", track_id);
    } else {
        AM_ERROR("NULL urlSuffix, please check code.\n");
    }

    //check track id
    if (track_id >= ESubSession_tot_count) {
        AM_ERROR("BAD track id %d.\n", track_id);
        if (urlSuffix) {
            AM_ERROR("urlSuffix: %s.\n", urlSuffix);
        }
        track_id = 0;
    }

    if (ESubSession_video == track_id) {
        track_type = IParameters::StreamType_Video;
        AM_ASSERT(p_client->enable_video && server_info->enable_video);
    } else if (ESubSession_audio == track_id) {
        track_type = IParameters::StreamType_Audio;
        AM_ASSERT(p_client->enable_audio && server_info->enable_audio);
    } else {
        AM_ERROR("must not comes here.\n");
        track_type = IParameters::StreamType_Video;
    }

    AMLOG_INFO("[rtsp setup]: track id %d, track type %d.\n", track_id, track_type);
    if (!p_client->get_host_string) {
        retrieveHostString((AM_U8*)fullRequestStr, p_client->host_addr, sizeof(p_client->host_addr));
        p_client->get_host_string = 1;
    }
    AMLOG_INFO("get host string %s.\n", p_client->host_addr);

    unsigned short sessionId;
    if(!parseSessionHeader(fullRequestStr,sessionId)){
        p_client->session_id = mRtspSessionId++;
    }else{
        p_client->session_id = sessionId;
    }

    AM_S64 curr_time = get_time();
    AM_UINT seed = (AM_UINT)((curr_time >> 10)  & 0xffffffff);
    our_srandom(seed);
    p_client->sub_session[track_id].rtp_ssrc = our_random32();
    p_client->rtp_over_rtsp = (streamingMode == IParameters::ProtocolType_TCP);
    if(streamingMode == IParameters::ProtocolType_UDP){
        p_client->sub_session[track_id].rtp_over_rtsp  = 0;
        p_client->sub_session[track_id].dst_addr.port = clientRTPPort;
        p_client->sub_session[track_id].dst_addr.port_ext = clientRTCPPort;
        if (p_client->p_data_transmiter) {
            p_client->p_data_transmiter->GetSrcPort(p_client->sub_session[track_id].data_port, p_client->sub_session[track_id].data_port_ext, track_type);
        }
        // Fill in the response:
        snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %s\r\n"
            "%s"
            "Transport: RTP/AVP/UDP;unicast;destination=%s;source=%s;client_port=%d-%d;server_port=%d-%d;ssrc=%08X\r\n"
            "Session: %08X\r\n\r\n",
            cseq,
            mDateBuffer,
            inet_ntoa(p_client->client_addr.sin_addr), p_client->host_addr, clientRTPPort, clientRTCPPort,
            p_client->sub_session[track_id].data_port, p_client->sub_session[track_id].data_port_ext,
            p_client->sub_session[track_id].rtp_ssrc,
            p_client->session_id);
    }else{
        //rtp over rtsp
        p_client->sub_session[track_id].rtp_over_rtsp  = 1;
        p_client->sub_session[track_id].rtp_channel = rtpChannelId;
        p_client->sub_session[track_id].rtcp_channel = rtcpChannelId;
        p_client->sub_session[track_id].rtsp_fd = p_client->socket;
        // Fill in the response:
        snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %s\r\n"
            "%s"
            "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d;ssrc=%08X\r\n"
            "Session: %08X\r\n\r\n",
            cseq,
            mDateBuffer,
            rtpChannelId, rtcpChannelId,
            p_client->sub_session[track_id].rtp_ssrc,
            p_client->session_id);
    }
    p_client->sub_session[track_id].state = ESessionServerState_Ready;
}

int  CStreammingServerManager::handleCmd_PLAY(SStreammingServerInfo* server_info, SClientSessionInfo* p_client, char const* cseq,
           char const* fullRequestStr)
{
    AM_UINT i;
    AM_ASSERT(server_info);
    //AM_ASSERT(server_info->p_rtsp_url);

    if (!p_client->get_host_string) {
        retrieveHostString((AM_U8*)fullRequestStr, p_client->host_addr, sizeof(p_client->host_addr));
        p_client->get_host_string = 1;
    }

    if (!server_info->p_rtsp_url) {
        server_info->rtsp_url_len = 128;
        server_info->p_rtsp_url = (char*)malloc(server_info->rtsp_url_len);
        snprintf(server_info->p_rtsp_url, server_info->rtsp_url_len, "rtsp://%s:%hu", p_client->host_addr, server_info->listenning_port);
    } else {
        AMLOG_INFO("Already have rtsp url.\n");
    }

    AMLOG_INFO("server_info->p_rtsp_url(%d):%s.\n", server_info->rtsp_url_len, server_info->p_rtsp_url);

    _getDateString(mDateBuffer, sizeof(mDateBuffer));
    //const char* scaleHeader = "";
    const char* rangeHeader = "Range: npt=0.000-\r\n";

    const char* rtpInfoTrackFmt =
        "url=%s/%s/trackID=%d"
        ";seq=%d"
        ";rtptime=%u";

    AM_U32 rtpTrackInfoSize = strlen(rtpInfoTrackFmt)
        + strlen(server_info->p_rtsp_url) + p_client->session_name_len
        + 5
        + 10
        + 2
        + 32;

    char* rtpVideoTrackInfo = NULL;
    char* rtpAudioTrackInfo = NULL;

    AM_S64 curr_time = get_time();
    AM_UINT seed = (AM_UINT)((curr_time >> 10)  & 0xffffffff);
    our_srandom(seed);
    if (p_client->enable_video && (p_client->sub_session[ESubSession_video].state == ESessionServerState_Ready)) {
        p_client->sub_session[ESubSession_video].rtp_ts = our_random32();
        p_client->sub_session[ESubSession_video].rtp_seq = (our_random32() >> 16) & 0xffff;

        rtpVideoTrackInfo = (char*) malloc(rtpTrackInfoSize);
        AM_ASSERT(rtpVideoTrackInfo);

        memset(rtpVideoTrackInfo, 0x0, rtpTrackInfoSize);
        snprintf(rtpVideoTrackInfo, rtpTrackInfoSize, rtpInfoTrackFmt,
            server_info->p_rtsp_url, p_client->p_session_name,
            ESubSession_video,
            p_client->sub_session[ESubSession_video].rtp_seq,
            p_client->sub_session[ESubSession_video].rtp_ts);
        p_client->sub_session[ESubSession_video].state = ESessionServerState_Playing;
    }

    if (p_client->enable_audio && (p_client->sub_session[ESubSession_audio].state == ESessionServerState_Ready)) {
        p_client->sub_session[ESubSession_audio].rtp_ts = our_random32();
        p_client->sub_session[ESubSession_audio].rtp_seq = (our_random32() >> 16) & 0xffff;
        rtpAudioTrackInfo = (char*) malloc(rtpTrackInfoSize);
        AM_ASSERT(rtpAudioTrackInfo);

        memset(rtpAudioTrackInfo, 0x0, rtpTrackInfoSize);
        snprintf(rtpAudioTrackInfo, rtpTrackInfoSize, rtpInfoTrackFmt,
            server_info->p_rtsp_url, p_client->p_session_name,
            ESubSession_audio,
            p_client->sub_session[ESubSession_audio].rtp_seq,
            p_client->sub_session[ESubSession_audio].rtp_ts);
        p_client->sub_session[ESubSession_audio].state = ESessionServerState_Playing;
    }

    if((p_client->sub_session[ESubSession_video].state != ESessionServerState_Playing)\
        && (p_client->sub_session[ESubSession_audio].state != ESessionServerState_Playing)){
        AM_ERROR("Both video and audio tracks have not setup yet\n");
        return -1;
    }

    int enable_video = 0;
    int enable_audio = 0;
    if( (p_client->sub_session[ESubSession_video].state == ESessionServerState_Playing) && p_client->enable_video ){
        enable_video = 1;
    }
    if( (p_client->sub_session[ESubSession_audio].state == ESessionServerState_Playing) && p_client->enable_audio ){
        enable_audio = 1;
    }

    AM_U32 rtpInfoSize = 32 + rtpTrackInfoSize*(enable_video + enable_audio);

    char* rtpinfoall = (char*) malloc(rtpInfoSize);
    AM_ASSERT(rtpinfoall);

    if (enable_video && enable_audio) {
        snprintf(rtpinfoall, rtpInfoSize, "RTP-Info: %s,%s\r\n", rtpVideoTrackInfo, rtpAudioTrackInfo);
    } else if (enable_video) {
        snprintf(rtpinfoall, rtpInfoSize, "RTP-Info: %s\r\n", rtpVideoTrackInfo);
    } else if (enable_audio) {
        snprintf(rtpinfoall, rtpInfoSize, "RTP-Info: %s\r\n", rtpAudioTrackInfo);
    } else {
        AM_ERROR("why audio/video are all disabled?\n");
    }

    memset(mResponseBuffer, 0, sizeof(mResponseBuffer));

    // Fill in the response:
    snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %s\r\n"
        "%s"
        //"%s"
        "%s"
        "Session: %08X\r\n"
        "%s\r\n",
        cseq,
        mDateBuffer,
        //scaleHeader,
        rangeHeader,
        p_client->session_id,
        rtpinfoall);

    free(rtpinfoall);
    if (rtpAudioTrackInfo) {
        free(rtpAudioTrackInfo);
    }
    if (rtpVideoTrackInfo) {
        free(rtpVideoTrackInfo);
    }
    //RTSP PLAY response
    //tcp_write(p_client->socket, (char const*)mResponseBuffer, strlen(mResponseBuffer));

    if(p_client->rtp_over_rtsp){
        if (p_client->p_data_transmiter) {
            if (enable_video) {
                p_client->p_data_transmiter->RemoveDstAddress(&p_client->sub_session[ESubSession_video].dst_addr, IParameters::StreamType_Video);
            }
            if (enable_audio) {
                p_client->p_data_transmiter->RemoveDstAddress(&p_client->sub_session[ESubSession_audio].dst_addr, IParameters::StreamType_Audio);
            }

            for (i = 0; i < ESubSession_tot_count; i++) {
                p_client->sub_session[i].dst_addr.rtp_time_base = p_client->sub_session[i].rtp_ts;
                p_client->sub_session[i].dst_addr.rtp_seq_base = p_client->sub_session[i].rtp_seq;
                p_client->sub_session[i].dst_addr.rtp_ssrc = p_client->sub_session[i].rtp_ssrc;
                p_client->sub_session[i].dst_addr.stream_started = 0;
                p_client->sub_session[i].dst_addr.parent = &p_client->sub_session[i];
                p_client->sub_session[i].rtsp_callback = this;
            }
            if (enable_video) {
                p_client->p_data_transmiter->AddDstAddress(&p_client->sub_session[ESubSession_video].dst_addr, IParameters::StreamType_Video);
            }

            if (enable_audio) {
                p_client->p_data_transmiter->AddDstAddress(&p_client->sub_session[ESubSession_audio].dst_addr, IParameters::StreamType_Audio);
            }
            int ret = tcp_write(p_client->socket, (char const*)mResponseBuffer, strlen(mResponseBuffer));
            if(ret < 0) return ret;
            p_client->p_data_transmiter->Enable(true);
            return 0;
        }
        return -1;
    }
    //add addr to dst list in data transmiter
    AM_ASSERT(p_client->p_data_transmiter);
    if (p_client->p_data_transmiter) {
        if (enable_video) {
             p_client->p_data_transmiter->RemoveDstAddress(&p_client->sub_session[ESubSession_video].dst_addr, IParameters::StreamType_Video);
        }
        if (enable_audio) {
             p_client->p_data_transmiter->RemoveDstAddress(&p_client->sub_session[ESubSession_audio].dst_addr, IParameters::StreamType_Audio);
        }
        for (i = 0; i < ESubSession_tot_count; i++) {
            //for RTP
            memset(&p_client->sub_session[i].dst_addr.addr_port,0,sizeof(struct sockaddr_in));
            p_client->sub_session[i].dst_addr.addr_port.sin_family      = AF_INET;
            p_client->sub_session[i].dst_addr.addr_port.sin_addr.s_addr = htonl(p_client->sub_session[i].dst_addr.addr);
            p_client->sub_session[i].dst_addr.addr_port.sin_port        = htons(p_client->sub_session[i].dst_addr.port);
            //for RTCP
            memset(&p_client->sub_session[i].dst_addr.addr_port_ext,0,sizeof(struct sockaddr_in));
            p_client->sub_session[i].dst_addr.addr_port_ext.sin_family      = AF_INET;
            p_client->sub_session[i].dst_addr.addr_port_ext.sin_addr.s_addr = htonl(p_client->sub_session[i].dst_addr.addr);
            p_client->sub_session[i].dst_addr.addr_port_ext.sin_port        = htons(p_client->sub_session[i].dst_addr.port_ext);
            //
            p_client->sub_session[i].dst_addr.rtp_time_base = p_client->sub_session[i].rtp_ts;
            p_client->sub_session[i].dst_addr.rtp_seq_base = p_client->sub_session[i].rtp_seq;
            p_client->sub_session[i].dst_addr.rtp_ssrc = p_client->sub_session[i].rtp_ssrc;
            p_client->sub_session[i].dst_addr.stream_started = 0;
            p_client->sub_session[i].dst_addr.parent = &p_client->sub_session[i];
            p_client->sub_session[i].rtsp_callback = NULL;
        }

        if (enable_video) {
            AMLOG_INFO("Add dst addr(video) 0x%u(%s), RTP port %hu, RTCP port %hu.\n", \
                                    p_client->sub_session[ESubSession_video].dst_addr.addr, \
                                    inet_ntoa(p_client->client_addr.sin_addr), \
                                    p_client->sub_session[ESubSession_video].dst_addr.port, \
                                    p_client->sub_session[ESubSession_video].dst_addr.port_ext);
            p_client->p_data_transmiter->AddDstAddress(&p_client->sub_session[ESubSession_video].dst_addr, IParameters::StreamType_Video);
        }

        if (enable_audio) {
            AMLOG_INFO("Add dst addr(audio) 0x%u(%s), RTP port %hu, RTCP port %hu.\n", \
                                    p_client->sub_session[ESubSession_audio].dst_addr.addr, \
                                    inet_ntoa(p_client->client_addr.sin_addr), \
                                    p_client->sub_session[ESubSession_audio].dst_addr.port, \
                                    p_client->sub_session[ESubSession_audio].dst_addr.port_ext);
            p_client->p_data_transmiter->AddDstAddress(&p_client->sub_session[ESubSession_audio].dst_addr, IParameters::StreamType_Audio);
        }
        int ret = tcp_write(p_client->socket, (char const*)mResponseBuffer, strlen(mResponseBuffer));
        if(ret < 0) return ret;
        p_client->p_data_transmiter->Enable(true);
        return 0;
    } else {
        AM_ERROR("NULL p_client->p_data_transmiter.\n");
        return -1;
    }
}

void CStreammingServerManager::handleCmd_GET_PARAMETER(SClientSessionInfo* p_client, char const* cseq,
	char const* fullRequestStr)
{
    _getDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sSession: %08X\r\n\r\n",
        cseq, mDateBuffer, p_client->session_id);
}

void CStreammingServerManager::handleCmd_SET_PARAMETER(SClientSessionInfo* p_client, char const* cseq,
	char const* /*fullRequestStr*/) {
    // By default, we implement "SET_PARAMETER" just as a 'keep alive', and send back an empty response.
    // (If you want to handle "SET_PARAMETER" properly, you can do so by defining a subclass of "RTSPServer"
    // and "RTSPServer::RTSPClientSession", and then reimplement this virtual function in your subclass.)
    _getDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%s\r\nSession: %08X\r\n\r\n",
        cseq, mDateBuffer, p_client->session_id);
}
void CStreammingServerManager::handleCmd_TEARDOWN(SClientSessionInfo* p_client, char const* cseq,
	char const* fullRequestStr)
{
    _getDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((char*)mResponseBuffer, sizeof(mResponseBuffer),
        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sSession: %08X\r\n\r\n",
        cseq, mDateBuffer, p_client->session_id);
}

AM_ERR CStreammingServerManager::handleCmd_withinSession(SStreammingServerInfo* server_info, SClientSessionInfo* p_client, char const* cmdName,
			  char const* urlPreSuffix, char const* urlSuffix,
			  char const* cseq, char const* fullRequestStr)
{
   int ret=0;
    _getDateString(mDateBuffer, sizeof(mDateBuffer));
    // This will either be:
    // - an operation on the entire server, if "urlPreSuffix" is "", and "urlSuffix" is "*" (i.e., the special "*" URL), or
    // - a non-aggregated operation, if "urlPreSuffix" is the session (stream)
    //	 name and "urlSuffix" is the subsession (track) name, or
    // - an aggregated operation, if "urlSuffix" is the session (stream) name,
    //	 or "urlPreSuffix" is the session (stream) name, and "urlSuffix" is empty.
    // Begin by figuring out which of these it is:
    if (urlPreSuffix[0] == '\0' && urlSuffix[0] == '*' && urlSuffix[1] == '\0') {
        // An operation on the entire server.  This works only for GET_PARAMETER and SET_PARAMETER:
        if (strcmp(cmdName, "GET_PARAMETER") == 0) {
            handleCmd_GET_PARAMETER(p_client, cseq, fullRequestStr);
        } else if (strcmp(cmdName, "SET_PARAMETER") == 0) {
            handleCmd_SET_PARAMETER(p_client, cseq, fullRequestStr);
        } else {
            handleCmd_notSupported(cseq);
        }
        ret = tcp_write(p_client->socket, (char const*)mResponseBuffer, strlen(mResponseBuffer));
        if(ret < 0) return ME_CLOSED;
        return ME_OK;
    } else if (p_client == NULL) { // There wasn't a previous SETUP!
        handleCmd_notSupported(cseq);
        ret = tcp_write(p_client->socket, (char const*)mResponseBuffer, strlen(mResponseBuffer));
        if(ret < 0) return ME_CLOSED;
        return ME_OK;
    }

    if (strcmp(cmdName, "TEARDOWN") == 0) {
        handleCmd_TEARDOWN(p_client, cseq,fullRequestStr);
        ret = tcp_write(p_client->socket, (char const*)mResponseBuffer, strlen(mResponseBuffer));
    } else if (strcmp(cmdName, "PLAY") == 0) {
        ret = handleCmd_PLAY(server_info, p_client, cseq, fullRequestStr);
    } else if (strcmp(cmdName, "PAUSE") == 0) {
    //  handleCmd_PAUSE(subsession, cseq);
    } else if (strcmp(cmdName, "GET_PARAMETER") == 0) {
        handleCmd_GET_PARAMETER(p_client, cseq, fullRequestStr);
        ret = tcp_write(p_client->socket, (char const*)mResponseBuffer, strlen(mResponseBuffer));
    } else if (strcmp(cmdName, "SET_PARAMETER") == 0) {
    //  handleCmd_SET_PARAMETER(subsession, cseq, fullRequestStr);
    }
    if(ret < 0){
        return ME_CLOSED;
    }
    return ME_OK;
}


