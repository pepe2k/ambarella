/*
 * rtsp_filter.cpp
 *
 * History:
 *    2011/8/31 - [Jay Zhang] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

#include "random.h"
#include "rtp_sink.h"
#include "helper.h"
#include "sock_helper.h"
#include "rtsp_client_session.h"
#include "adts_server_media_subsession.h"
#include "h264_server_media_subsession.h"

#include "rtsp_filter.h"

//#define RECORD_TEST_FILE

//-----------------------------------------------------------------------
//
// CamRTSPServer
//
//-----------------------------------------------------------------------
IFilter* CreateRTSPServer(IEngine * pEngine)
{
    return CamRTSPServer::Create(pEngine);
}

CamRTSPServer::CamRTSPServer(IEngine * pEngine):
    inherited(pEngine,"CRtspServer"),
    _rtspServerPort(554),
    _rtpServerPort(50000)
{
}
    
CamRTSPServer *CamRTSPServer::Create(IEngine * pEngine)
{
    CamRTSPServer *result = new CamRTSPServer(pEngine);
    if (result != NULL && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CamRTSPServer::Construct()
{
    CamServerMediaSession* pSession;
    CamServerMediaSubsession* pSubsession;

    if (inherited::Construct() != ME_OK)
        return ME_ERROR;

    pipe(_pipeFd);

    for (int i = 0; i < MAX_NUM_CLIENTS; i++) {
        _clientSock[i] = -1;			/* -1 indicates available entry */
        _pClientSession[i] = NULL;
    }
    for (int i = 0; i < MAX_NUM_MEDIA_SESSION; i++) {
        _pMediaSession[i] = NULL;
    }

    for(int i = 0; i < mux_input_pin_range; i++)
    {
        if((_pInput[i] = CamRtspServerInput::Create(this)) == NULL)
        return ME_ERROR;
//	printf("_pInput[%d] %p\n", i, _pInput[i]);
    }

    _rtspServerSocket= SetupStreamSocket(INADDR_ANY, _rtspServerPort, 1);
    _rtpUDPSocket = SetupDatagramSocket(INADDR_ANY, _rtpServerPort, 1);

    const char* descriptionString = "Session streamed by \"Amba RTSP Server\"";


    CamServerMediaSubsession* pAudioSubsession = CamAdtsServerMediaSubsession::Create("audio");
    _pInput[mux_input_pin_audio]->SetMediaSubsession(pAudioSubsession);
    pSession = CamServerMediaSession::Create("aac", "aac", descriptionString);
    pSession->AddSubsession(pAudioSubsession);
    AddServerMediaSession(pSession);

    pSubsession = CamH264ServerMediaSubsession::Create("video");
    _pInput[mux_input_pin_0]->SetMediaSubsession(pSubsession);
    pSession = CamServerMediaSession::Create("stream1", "stream1", descriptionString);
    pSession->AddSubsession(pSubsession);
    pSession->AddSubsession(pAudioSubsession);
    AddServerMediaSession(pSession);

    CAutoString rtspURL = this->rtspURL(pSession);
    AM_WARNING("Play this stream using the URL \"%s\"\n", rtspURL.GetStr());

    pSubsession = CamH264ServerMediaSubsession::Create("video");
    _pInput[mux_input_pin_1]->SetMediaSubsession(pSubsession);
    pSession = CamServerMediaSession::Create("stream2", "stream2", descriptionString);
    pSession->AddSubsession(pSubsession);
    pSession->AddSubsession(pAudioSubsession);
    AddServerMediaSession(pSession);

    pSubsession = CamH264ServerMediaSubsession::Create("video");
    _pInput[mux_input_pin_2]->SetMediaSubsession(pSubsession);
    pSession = CamServerMediaSession::Create("stream3", "stream3", descriptionString);
    pSession->AddSubsession(pSubsession);
    pSession->AddSubsession(pAudioSubsession);
    AddServerMediaSession(pSession);

    pSubsession = CamH264ServerMediaSubsession::Create("video");
    _pInput[mux_input_pin_3]->SetMediaSubsession(pSubsession);
    pSession = CamServerMediaSession::Create("stream4", "stream4", descriptionString);
    pSession->AddSubsession(pSubsession);
    pSession->AddSubsession(pAudioSubsession);
    AddServerMediaSession(pSession);

    if ((_pServerThread = CThread::Create("Connection Server", ServerThread, this)) == NULL)
        return ME_ERROR;

    return ME_OK;
}

AM_ERR CamRTSPServer::ServerThread(void *p)
{
	((CamRTSPServer*)p)->ServerThreadLoop();

	return ME_OK;
}

CamRTSPServer::~CamRTSPServer()
{
    if (_pServerThread != NULL) {
        AM_DELETE(_pServerThread);
    }

    for (int i = 0; i < mux_input_pin_range; i++)
        AM_DELETE(_pInput[i]);

    for (int i = 0; i < MAX_NUM_MEDIA_SESSION; i++)
        AM_DELETE(_pMediaSession[i]);
}

AM_ERR CamRTSPServer::Stop()
{
    AM_ERR err = ME_OK;

    char endStr = 'e';

    write(_pipeFd[1], &endStr, 1);	//awaken the select function call

    err =  inherited::Stop();

    return err;
}

void CamRTSPServer::GetInfo(INFO& info)
{
    info.nInput = mux_input_pin_range;
    info.nOutput = 0;
    info.pName = "RTSP Server";
}


IPin* CamRTSPServer::GetInputPin(AM_UINT index)
{
    if (index < mux_input_pin_range)
        return _pInput[index];

    AM_ERROR("no such pin %d\n", index);
    return NULL;
}

void CamRTSPServer::ServerThreadLoop()
{
    int					i, maxi, maxfd, connfd, sockfd;
    int					nready;
    fd_set				rset, allset;
    socklen_t				clilen;
    struct sockaddr_in		cliaddr;

    maxfd = AM_MAX(_rtspServerSocket, _pipeFd[0]);			/* initialize */
    maxi = -1;					/* index into client[] array */

    FD_ZERO(&allset);
    FD_SET(_rtspServerSocket, &allset);
    FD_SET(_pipeFd[0], &allset);

    for ( ; ; ) {
        rset = allset;		/* structure assignment */
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(_pipeFd[0], &rset)) {	/* filter stoped */
            break;
        }

        if (FD_ISSET(_rtspServerSocket, &rset)) {	/* new client connection */
            clilen = sizeof(cliaddr);
            connfd = accept(_rtspServerSocket, (struct sockaddr*) &cliaddr, &clilen);

            AM_WARNING("new client: %s, port %d\n\n", inet_ntoa(cliaddr.sin_addr), 
            ntohs(cliaddr.sin_port));

            for (i = 0; i < MAX_NUM_CLIENTS; i++) {
                if (_clientSock[i] < 0) {
                    _clientSock[i] = connfd;	/* save descriptor */
                    _pClientSession[i] = CamRTSPClientSession:: Create(this, _clientSock[i],
                    ntohl(cliaddr.sin_addr.s_addr), _rtpServerPort);
                    break;
                }
            }

            if (i == MAX_NUM_CLIENTS)
                AM_ERROR("too many clients\n");

            FD_SET(connfd, &allset);	/* add new descriptor to set */
            if (connfd > maxfd)
                maxfd = connfd;			/* for select */
            if (i > maxi)
                maxi = i;				/* max index in client[] array */

            if (--nready <= 0)
                continue;				/* no more readable descriptors */
        }

        for (i = 0; i <= maxi; i++) {	/* check all clients for data */
            if ( (sockfd = _clientSock[i]) < 0)
                continue;
            if (FD_ISSET(sockfd, &rset)) {
                if (_pClientSession[i]->incomingRequestHandler() == ME_CLOSED) {
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    _clientSock[i] = -1;
                    delete _pClientSession[i];
                    _pClientSession[i] = NULL;
                }
                if (--nready <= 0)
                    break;				/* no more readable descriptors */
            }
        }
    }
    AM_PRINTF("Connection Server exit main loop\n");
}

void CamRTSPServer::OnRun()
{
    CBuffer *pBuffer;
    CQueueInputPin *pInputPin;
    CamDestination *pDestination;
    CamServerMediaSubsession *pMediaSubsession;
    CamRtpSink *pRtpSink;
    AM_U32 bytes_count;
    AM_U8 *pFrameStart;
    AM_U32 frameSize;
    AM_BOOL lastFragment;
    int i;

    AM_ERR err = ME_OK;
    CmdAck(ME_OK);

#ifdef RECORD_TEST_FILE
    int fd = open("video.h264", O_CREAT | O_TRUNC | O_WRONLY, 0777);
#endif
    while (1) {
//      printf("WaitInput\n");
            if (!WaitInputBuffer(pInputPin, pBuffer)) {
                AM_ERROR("Failed to WaitInputBuffer\n");
                break;
            }

        for (i = 0; i < mux_input_pin_range; i++) {
            if (pInputPin == _pInput[i]) {
                break;
            }
        }

        if (i == mux_input_pin_range) {
            //printf("Error input pin go out.\n");
            err = ME_NOT_EXIST;
            break;
        }

//    printf("pin %d has data\n", i);
        if (pBuffer->GetType() == CBuffer::DATA) {

            pMediaSubsession = (static_cast<CamRtspServerInput *>(pInputPin))->GetMediaSubsession();

            pMediaSubsession->LockRtpSink();
            pRtpSink = pMediaSubsession->GetRtpSink();

            if (pRtpSink == NULL) {
                pBuffer->Release();
                pMediaSubsession->UnlockRtpSink();
                continue;
            }

#ifdef RECORD_TEST_FILE
            if (i == 0) {
                AM_WARNING("write %p, size %d\n", pBuffer->GetDataPtr(), pBuffer->GetDataSize());
                write(fd, pBuffer->GetDataPtr(), pBuffer->GetDataSize());
            }
#endif
            pRtpSink->FeedStreamData(pBuffer->GetDataPtr(), pBuffer->GetDataSize());

            while (pRtpSink->GetOneFrame(&pFrameStart, &frameSize, &lastFragment)) {

                bytes_count = pRtpSink->DoSpecialFrameHandling(_outBuf,
                    pFrameStart, frameSize, lastFragment);

                while ((pDestination = pMediaSubsession->GetNextDestination()) != NULL) {   /* check all destination */

                    AM_PRINTF("destination: %d, 0x%x : %d\n", pDestination->_socket,
                        pDestination->_addr, pDestination->_rtpPort);

                    SendDatagram(_rtpUDPSocket, pDestination->_addr, pDestination->_rtpPort,
                        _outBuf, bytes_count);
                }
            }
            pMediaSubsession->UnlockRtpSink();  // has finished using Rtp Sink
        } else if (pBuffer->GetType() == CBuffer::EOS) {

            char endStr = 'e';
            write(_pipeFd[1], &endStr, 1);  //awaken the select function call
            AM_INFO("RTSP received EOS\n!");
            PostEngineMsg(IEngine::MSG_EOS);
            break;
        }
//    printf("Release\n");
        pBuffer->Release();
    }
    AM_PRINTF("Streaming Server exit main loop\n");
}

AM_ERR CamRTSPServer::AddServerMediaSession(CamServerMediaSession *pMediaSession)
{
    int i;
//printf("AddDestination: session %d, sock %d, addr 0x%x, port %d\n", clientSessionID, sock, addr, port);

    for (i = 0; i < MAX_NUM_MEDIA_SESSION; i++) {
        if (_pMediaSession[i] == NULL)
            break;
    }
    if (i == MAX_NUM_MEDIA_SESSION)	// No free slot for new media session
        return ME_ERROR;

    _pMediaSession[i] = pMediaSession;

    return ME_OK;
}

CamServerMediaSession *CamRTSPServer::LookupServerMediaSession(char const* pStreamName) const
{
    for (int i = 0; i < MAX_NUM_MEDIA_SESSION; i++) {
        if (strcmp(_pMediaSession[i]->GetStreamName(), pStreamName) == 0)
            return _pMediaSession[i];
    }

    return NULL;
}

CamRTSPClientSession *CamRTSPServer::LookupClientSession(AM_U32 clientSessionID) const
{
    for (int i = 0; i < MAX_NUM_CLIENTS; i++) {
        if (_pClientSession[i]->GetSessionID() == clientSessionID)
            return _pClientSession[i];
    }
    return NULL;
}

CAutoString CamRTSPServer::rtspURL(CamServerMediaSession const* pMediaSession,
    int clientSocket) const {
    AM_ASSERT(pMediaSession);
    CAutoString urlPrefix = rtspURLPrefix(clientSocket);
    char const* sessionName = pMediaSession->GetStreamName();

    CAutoString resultURL(urlPrefix.StrLen() + strlen(sessionName));

    sprintf(resultURL.GetStr(), "%s%s", urlPrefix.GetStr(), sessionName);

    return resultURL;
}

CAutoString CamRTSPServer::rtspURLPrefix(int clientSocket) const
{
    AM_U32 IPAddress;
    if (clientSocket < 0) {
        // Use our default IP address in the URL:
        GetOurIPAddress(&IPAddress);
    } else {
        GetIPAddressBySock(clientSocket, &IPAddress);
    }
    char* const IPAddrStr = HostAddressToString(IPAddress);
    CAutoString urlBuffer(100); // more than big enough for "rtsp://<ip-address>:<port>/"

    if (_rtspServerPort == 554 /* the default port number */) {
        sprintf(urlBuffer.GetStr(), "rtsp://%s/", IPAddrStr);
    } else {
        sprintf(urlBuffer.GetStr(), "rtsp://%s:%hu/", IPAddrStr, _rtspServerPort);
    }

    return urlBuffer;
}
//-----------------------------------------------------------------------
//
// CamRtspServerInput
//
//-----------------------------------------------------------------------
CamRtspServerInput* CamRtspServerInput::Create(CFilter *pFilter)
{
    CamRtspServerInput* result = new CamRtspServerInput(pFilter);
    if (result && result->Construct() != ME_OK)
    {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CamRtspServerInput::Construct()
{
    AM_ERR err = inherited::Construct(((CamRTSPServer*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

AM_ERR CamRtspServerInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    //TODO
    return ME_OK;
}

