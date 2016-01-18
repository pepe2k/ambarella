/*
 * rtsp_client_session.cpp
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

#include "rtsp_filter.h"
#include "server_media_session.h"

#include "adts_rtp_sink.h"
#include "h264_rtp_sink.h"
#include "random.h"
#include "helper.h"
#include "rtsp_client_session.h"

// Generate a "Date:" header for use in a RTSP response:
static char const* dateHeader() {
	static char buf[200];
	time_t tt = time(NULL);
	strftime(buf, sizeof buf, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
	return buf;
}

static char* strDupSize(char const* str) {
	if (str == NULL) return NULL;
	size_t len = strlen(str) + 1;
	char* copy = new char[len];

	return copy;
}


//-----------------------------------------------------------------------
//
// CamRTSPClientSession
//
//-----------------------------------------------------------------------
CamRTSPClientSession* CamRTSPClientSession::Create(CamRTSPServer *ourServer,
	int clientSocket, AM_U32 clientAddr, AM_U16 serverRtpPort)
{
	CamRTSPClientSession* result = new CamRTSPClientSession(ourServer, clientSocket, clientAddr, serverRtpPort);
	if (result != NULL && result->Construct() != ME_OK)
	{
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CamRTSPClientSession::Construct()
{	
	_requestBuffer[0] = '\0';
	_responseBuffer[0] = '\0';
	_sessionId = (AM_U32)our_random();

	return ME_OK;
}

CamRTSPClientSession::~CamRTSPClientSession() 
{
	ReclaimStreamStates();
}

void CamRTSPClientSession::ReclaimStreamStates()
{
	for (AM_U32 i = 0; i < _numStreamStates; ++i) {
		CamServerMediaSubsession* subsession = _pStreamStates[i].subsession;
		if (subsession != NULL) {
			subsession->DeleteDestination(_sessionId);
			if (subsession->GetDestinationCount() == 0)
				subsession->RemoveRtpSink();
		}
	}
	delete[] _pStreamStates; _pStreamStates = NULL;
	_numStreamStates = 0;
}

AM_ERR CamRTSPClientSession::incomingRequestHandler()
{
	#define RTSP_PARAM_STRING_MAX	200
	int bytesRead;
	char cmdName[RTSP_PARAM_STRING_MAX];
	char urlPreSuffix[RTSP_PARAM_STRING_MAX];
	char urlSuffix[RTSP_PARAM_STRING_MAX];
	char cseq[RTSP_PARAM_STRING_MAX];
	
	if ( (bytesRead = read(_clientSocket, _requestBuffer, RTSP_BUFFER_SIZE)) <= 0) {
		/*4connection closed by client */
		AM_PRINTF("Client EOF\n");
		return ME_CLOSED;
	}
	_requestBuffer[bytesRead] = '\0';		// in case string is not terminated with '\0'
//	printf("receiving request (%d bytes):\n%s", bytesRead, _requestBuffer);
	if (parseRTSPRequestString((char*)_requestBuffer, bytesRead,
				 cmdName, sizeof cmdName,
				 urlPreSuffix, sizeof urlPreSuffix,
				 urlSuffix, sizeof urlSuffix,
				 cseq, sizeof cseq)) {

		if (strcmp(cmdName, "OPTIONS") == 0) {
			handleCmd_OPTIONS(cseq);
		} else if (strcmp(cmdName, "DESCRIBE") == 0) {
			handleCmd_DESCRIBE(cseq, urlSuffix, (char const*)_requestBuffer);
		} else if (strcmp(cmdName, "SETUP") == 0) {
			handleCmd_SETUP(cseq, urlPreSuffix, urlSuffix, (char const*)_requestBuffer);
		} else if (strcmp(cmdName, "TEARDOWN") == 0
			|| strcmp(cmdName, "PLAY") == 0
			|| strcmp(cmdName, "PAUSE") == 0
			|| strcmp(cmdName, "GET_PARAMETER") == 0
			|| strcmp(cmdName, "SET_PARAMETER") == 0) {
			handleCmd_withinSession(cmdName, urlPreSuffix, urlSuffix, cseq,
				(char const*)_requestBuffer);
		} else {
			handleCmd_notSupported(cseq);
		}
	} else {
		AM_ERROR("parseRTSPRequestString() failed\n");
		handleCmd_bad(cseq);
	}

//	printf("%s\n\n", _responseBuffer);

  	write(_clientSocket, (char const*)_responseBuffer, strlen(_responseBuffer));

	return ME_OK;
}

AM_BOOL CamRTSPClientSession::parseRTSPRequestString(char const* reqStr,
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

	// Read everything up to the first space as the command name:
	AM_BOOL parseSucceeded = AM_FALSE;
	AM_U32 i;
	for (i = 0; i < resultCmdNameMaxSize-1 && i < reqStrSize; ++i) {
		char c = reqStr[i];
		if (c == ' ' || c == '\t') {
			parseSucceeded = AM_TRUE;
			break;
		}
		resultCmdName[i] = c;
	}
	resultCmdName[i] = '\0';
	if (!parseSucceeded) return AM_FALSE;

	// Skip over the prefix of any "rtsp://" or "rtsp:/" URL that follows:
	AM_U32 j = i+1;
	while (j < reqStrSize && (reqStr[j] == ' ' || reqStr[j] == '\t')) ++j; // skip over any additional white space

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

	// Look for the URL suffix (before the following "RTSP/1.0"):
	parseSucceeded = AM_FALSE;
	for (AM_U32 k = i+1; (int)k < (int)(reqStrSize-5); ++k) {
		
		if (reqStr[k] == 'R' && reqStr[k+1] == 'T' &&
			reqStr[k+2] == 'S' && reqStr[k+3] == 'P' && reqStr[k+4] == '/') {
			
			while (--k >= i && reqStr[k] == ' ') {} // go back over all spaces before "RTSP/"
			AM_U32 k1 = k;
			while (k1 > i && reqStr[k1] != '/') --k1; // go back to the first '/'
			// the URL suffix comes from [k1+1,k]

			// Copy "resultURLSuffix":
			if (k - k1 + 1 > resultURLSuffixMaxSize) return AM_FALSE; // there's no room
			AM_U32 n = 0, k2 = k1+1;
			while (k2 <= k) resultURLSuffix[n++] = reqStr[k2++];
			resultURLSuffix[n] = '\0';

			// Also look for the URL 'pre-suffix' before this:
			unsigned k3 = (k1 == 0) ? 0 : --k1;
			while (k3 > i && reqStr[k3] != '/') --k3; // go back to the first '/'
			// the URL pre-suffix comes from [k3+1,k1]

			// Copy "resultURLPreSuffix":
			if (k1 - k3 + 1 > resultURLPreSuffixMaxSize) return AM_FALSE; // there's no room
			n = 0; k2 = k3+1;
			while (k2 <= k1) resultURLPreSuffix[n++] = reqStr[k2++];
			resultURLPreSuffix[n] = '\0';

			i = k + 7; // to go past " RTSP/"
			parseSucceeded = AM_TRUE;
			break;
		}
	}
	if (!parseSucceeded) return AM_FALSE;

	// Look for "CSeq:", skip whitespace,
	// then read everything up to the next \r or \n as 'CSeq':
	parseSucceeded = AM_FALSE;
	for (j = i; (int)j < (int)(reqStrSize-5); ++j) {
		
		if (reqStr[j] == 'C' && reqStr[j+1] == 'S' && reqStr[j+2] == 'e' &&
			reqStr[j+3] == 'q' && reqStr[j+4] == ':') {
			j += 5;
			unsigned n;
			while (j < reqStrSize && (reqStr[j] ==  ' ' || reqStr[j] == '\t')) ++j; // skip over any additional white space
			for (n = 0; n < resultCSeqMaxSize-1 && j < reqStrSize; ++n,++j) {
				char c = reqStr[j];
				if (c == '\r' || c == '\n') {
					parseSucceeded = AM_TRUE;
					break;
				}
				resultCSeq[n] = c;
			}
			resultCSeq[n] = '\0';
			break;
		}
	}
	if (!parseSucceeded) return AM_FALSE;

	return AM_TRUE;
}


AM_BOOL CamRTSPClientSession::parseTransportHeader(char const* buf,
				 StreamingMode *streamingMode,
				 char* streamingModeString,
				 char* destinationAddressStr,
				 AM_U8 *destinationTTL,
				 AM_U16 *clientRTPPortNum, // if UDP
				 AM_U16 *clientRTCPPortNum, // if UDP
				 AM_U8 *rtpChannelId, // if TCP
				 AM_U8 *rtcpChannelId // if TCP
				 ) {
	// Initialize the result parameters to default values:
	*streamingMode = RTP_UDP;
	*destinationTTL = 255;
	*clientRTPPortNum = 0;
	*clientRTCPPortNum = 1;
	*rtpChannelId = *rtcpChannelId = 0xFF;

	AM_U16 p1, p2;
	AM_U32 ttl, rtpCid, rtcpCid;

	// First, find "Transport:"
	while (1) {
		if (*buf == '\0') return AM_FALSE; // not found
		if (strncasecmp(buf, "Transport: ", 11) == 0) break;
		++buf;
	}

	// Then, run through each of the fields, looking for ones we handle:
	char const* fields = buf + 11;
	char* field = strDupSize(fields);

	while (sscanf(fields, "%[^;]", field) == 1) {
		if (strcmp(field, "RTP/AVP/TCP") == 0) {
			*streamingMode = RTP_TCP;
		} else if (strcmp(field, "RAW/RAW/UDP") == 0 ||
			strcmp(field, "MP2T/H2221/UDP") == 0) {
			*streamingMode = RAW_UDP;
			strncpy(streamingModeString, field, strlen(streamingModeString));
		} else if (strncasecmp(field, "destination=", 12) == 0) {
			delete[] destinationAddressStr;
			strncpy(destinationAddressStr, field+12, strlen(destinationAddressStr));
		} else if (sscanf(field, "ttl%u", &ttl) == 1) {
			*destinationTTL = (u_int8_t)ttl;
		} else if (sscanf(field, "client_port=%hu-%hu", &p1, &p2) == 2) {
			*clientRTPPortNum = p1;
			*clientRTCPPortNum = p2;
		} else if (sscanf(field, "client_port=%hu", &p1) == 1) {
			*clientRTPPortNum = p1;
			*clientRTCPPortNum = (*streamingMode == RAW_UDP) ? 0 : p1 + 1;
		} else if (sscanf(field, "interleaved=%u-%u", &rtpCid, &rtcpCid) == 2) {
			*rtpChannelId = (unsigned char)rtpCid;
			*rtcpChannelId = (unsigned char)rtcpCid;
		}

		fields += strlen(field);
		while (*fields == ';') ++fields; // skip over separating ';' chars
		if (*fields == '\0' || *fields == '\r' || *fields == '\n') break;
	}
 	delete[] field;
	return AM_TRUE;
}

// Handler routines for specific RTSP commands:

void CamRTSPClientSession::handleCmd_bad(char const* /*cseq*/) {
  // Don't do anything with "cseq", because it might be nonsense
  snprintf((char*)_responseBuffer, sizeof _responseBuffer,
	   "RTSP/1.0 400 Bad Request\r\n%sAllow: %s\r\n\r\n",
	   dateHeader(), getAllowedCommandNames());
}

void CamRTSPClientSession::handleCmd_notSupported(char const* cseq) {
  snprintf((char*)_responseBuffer, sizeof _responseBuffer,
	   "RTSP/1.0 405 Method Not Allowed\r\nCSeq: %s\r\n%sAllow: %s\r\n\r\n",
	   cseq, dateHeader(), getAllowedCommandNames());
}

void CamRTSPClientSession::handleCmd_notFound(char const* cseq) {
  snprintf((char*)_responseBuffer, sizeof _responseBuffer,
	   "RTSP/1.0 404 Stream Not Found\r\nCSeq: %s\r\n%s\r\n",
	   cseq, dateHeader());
}

void CamRTSPClientSession::handleCmd_unsupportedTransport(char const* cseq) {
  snprintf((char*)_responseBuffer, sizeof _responseBuffer,
	   "RTSP/1.0 461 Unsupported Transport\r\nCSeq: %s\r\n%s\r\n",
	   cseq, dateHeader());
}

void CamRTSPClientSession::handleCmd_OPTIONS(char const* cseq) {
  snprintf((char*)_responseBuffer, sizeof _responseBuffer,
	   "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sPublic: %s\r\n\r\n",
	   cseq, dateHeader(), getAllowedCommandNames());
}

void CamRTSPClientSession::handleCmd_DESCRIBE(char const* cseq,
    char const* urlSuffix, char const* fullRequestStr) {

    // We should really check that the request contains an "Accept:" #####
    // for "application/sdp", because that's what we're sending back #####

    // Begin by looking up the "ServerMediaSession" object for the
    // specified "urlSuffix":
    CamServerMediaSession *pMediaSession = _ourServer->LookupServerMediaSession(urlSuffix);
    if (pMediaSession == NULL) {
        handleCmd_notFound(cseq);
        return;
    }

    // Then, assemble a SDP description for this session:
    CAutoString sdpDescription = pMediaSession->GenerateSDPDescription();

    if (sdpDescription.StrLen() == 0) {
        // This usually means that a file name that was specified for a
        // "ServerMediaSubsession" does not exist.
        snprintf((char*)_responseBuffer, sizeof _responseBuffer,
            "RTSP/1.0 404 File Not Found, Or In Incorrect Format\r\n"
            "CSeq: %s\r\n"
            "%s\r\n",
            cseq,
            dateHeader());
        return;
    }
    AM_U32 sdpDescriptionSize = sdpDescription.StrLen();

    // Also, generate our RTSP URL, for the "Content-Base:" header
    // (which is necessary to ensure that the correct URL gets used in
    // subsequent "SETUP" requests).

    CAutoString rtspURL = _ourServer->rtspURL(pMediaSession, _clientSocket);

    snprintf((char*)_responseBuffer, sizeof _responseBuffer,
         "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
         "%s"
         "Content-Base: %s/\r\n"
         "Content-Type: application/sdp\r\n"
         "Content-Length: %d\r\n\r\n"
         "%s",
         cseq,
         dateHeader(),
         rtspURL.GetStr(),
         sdpDescriptionSize,
         sdpDescription.GetStr());
}


void CamRTSPClientSession::handleCmd_SETUP(char const* cseq,
		  char const* urlPreSuffix, char const* urlSuffix,
		  char const* fullRequestStr) {

	// "urlPreSuffix" should be the session (stream) name, and
	// "urlSuffix" should be the subsession (track) name.
	char const* pStreamName = urlPreSuffix;
	char const* pTrackId = urlSuffix;
	CamServerMediaSubsession* pSubsession = NULL;

	StreamingMode streamingMode;
	char streamingModeString[16]; // set when RAW_UDP streaming is specified
	char clientsDestinationAddressStr[16];
	AM_U8 clientsDestinationTTL;
	AM_U16 clientRTPPort, clientRTCPPort;
	unsigned char rtpChannelId, rtcpChannelId;


	// Check whether we have existing session state, and, if so, whether it's
	// for the session that's named in "streamName".  (Note that we don't
	// support more than one concurrent session on the same client connection.) #####
	if (_pMediaSession != NULL &&
		strcmp(pStreamName, _pMediaSession->GetStreamName()) != 0) {
		_pMediaSession = NULL;
	}
	if (_pMediaSession == NULL) {	// first time SETUP, create streamState
		// Associate a media session to this client session. Set up this session's state.
		// Look up the "ServerMediaSession" object for the specified stream
		_pMediaSession = _ourServer->LookupServerMediaSession(pStreamName);
		if (_pMediaSession == NULL) {
			handleCmd_notFound(cseq);
			return;
		}
		    // Set up our array of states for this session's subsessions (tracks):
		for (pSubsession = _pMediaSession->GetSubsessionsHead();
			pSubsession != NULL; pSubsession = pSubsession->GetNext())
			_numStreamStates++;
   		_pStreamStates = new struct streamState[_numStreamStates];
		pSubsession = _pMediaSession->GetSubsessionsHead();
		for (AM_U32 i = 0; i < _numStreamStates; ++i) {
			 _pStreamStates[i].subsession = pSubsession;
			 pSubsession = pSubsession->GetNext();
		}
	}
	  // Look up information for the specified subsession (track):
	if (pTrackId != NULL && pTrackId[0] != '\0') { // normal case
		pSubsession = _pMediaSession->LookupSubsessionByTrackId(pTrackId);
		if (pSubsession == NULL) {
			// The specified track id doesn't exist, so this request fails:
			handleCmd_notFound(cseq);
			return;
		}
	} else {
		// Weird case: there was no track id in the URL.
		// This works only if we have only one subsession:
		if (_numStreamStates != 1) {
			handleCmd_bad(cseq);
			return;
		}
		pSubsession = _pStreamStates[0].subsession;
	}

	AM_ASSERT(pSubsession != NULL);
// Look for a "Transport:" header in the request string,
// to extract client parameters:
	
	parseTransportHeader(fullRequestStr, &streamingMode, streamingModeString,
		clientsDestinationAddressStr, &clientsDestinationTTL,
		&clientRTPPort, &clientRTCPPort,
		&rtpChannelId, &rtcpChannelId);

  
	const char* destAddrStr = "10.0.0.1";
	const char* sourceAddrStr = "10.0.0.2";

	pSubsession->AddDestination(_sessionId, _ourServer->GetRtpServerPort(), _clientAddr, clientRTPPort);
//	inputPin->AddDestination(_sessionId, _ourServer->GetRtpServerPort(), _clientAddr, clientRTPPort);
  // Fill in the response:	
	snprintf((char*)_responseBuffer, sizeof _responseBuffer,
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %s\r\n"
		"%s"
		"Transport: RTP/AVP;unicast;destination=%s;source=%s;client_port=%d-%d;server_port=%d-%d\r\n"
		"Session: %08X\r\n\r\n",
		cseq,
		dateHeader(),
		destAddrStr, sourceAddrStr, clientRTPPort, clientRTCPPort,
		_serverPort, _serverPort+1,
		_sessionId);

	
}

void CamRTSPClientSession::handleCmd_PLAY(CamServerMediaSubsession* pSubsession, char const* cseq,
		   char const* fullRequestStr)
{
	CAutoString rtspURL = _ourServer->rtspURL(_pMediaSession, _clientSocket);

	const char* scaleHeader = "";
	const char* rangeHeader = "Range: npt=0.000-\r\n";

	// Create a "RTP-Info:" line.  It will get filled in from each subsession's state:
	const char* rtpInfoFmt =
		"%s" // comma separator, if needed
		"url=%s/%s"
		";seq=%d"
		";rtptime=%u";

	CAutoString rtpInfoAll("RTP-Info: ");
	AM_U32 numRTPInfoItems = 0;

	// Now, start streaming:
	for (AM_U32 i = 0; i < _numStreamStates; ++i) {
		if (pSubsession == NULL || /* means: aggregated operation */
			pSubsession == _pStreamStates[i].subsession) {

			CamRtpSink* pRtpSink = _pStreamStates[i].subsession->GetRtpSink();
			AM_U16 rtpSeqNum = pRtpSink->GetCurrentSeqNo();
			AM_U32 rtpTimestamp = pRtpSink->GetCurrentTimestamp() + 3003;
			CAutoString urlSuffix = _pStreamStates[i].subsession->GetTrackIdStr();

			AM_U32 rtpInfoSize = strlen(rtpInfoFmt)
				+ 1	//comma separator, if needed
				+ rtspURL.StrLen() + urlSuffix.StrLen()
				+ 5 /*max unsigned short len*/
				+ 10 /*max unsigned (32-bit) len*/
				+ 2 /*allows for trailing \r\n at final end of string*/;

			CAutoString rtpInfo(rtpInfoSize);
			sprintf(rtpInfo.GetStr(), rtpInfoFmt,
				numRTPInfoItems++ == 0 ? "" : ",",
				rtspURL.GetStr(), urlSuffix.GetStr(),
				rtpSeqNum,
				rtpTimestamp);

			rtpInfoAll += rtpInfo;
			rtpInfoAll += CAutoString("\r\n");
		}
	}


	if (numRTPInfoItems == 0) {
		rtpInfoAll = CAutoString();	// empty string
	}

	// Fill in the response:
	snprintf((char*)_responseBuffer, sizeof _responseBuffer,
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %s\r\n"
		"%s"
		"%s"
		"%s"
		"Session: %08X\r\n"
		"%s\r\n",
		cseq,
		dateHeader(),
		scaleHeader,
		rangeHeader,
		_sessionId,
		rtpInfoAll.GetStr());
}

void CamRTSPClientSession::handleCmd_GET_PARAMETER(char const* cseq,
	char const* fullRequestStr)
{
  snprintf((char*)_responseBuffer, sizeof _responseBuffer,
	   "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sSession: %08X\r\n\r\n",
	   cseq, dateHeader(), _sessionId);
}

void CamRTSPClientSession::handleCmd_SET_PARAMETER(char const* cseq,
	char const* /*fullRequestStr*/) {
  // By default, we implement "SET_PARAMETER" just as a 'keep alive', and send back an empty response.
  // (If you want to handle "SET_PARAMETER" properly, you can do so by defining a subclass of "RTSPServer"
  // and "RTSPServer::RTSPClientSession", and then reimplement this virtual function in your subclass.)
  snprintf((char*)_responseBuffer, sizeof _responseBuffer,
	   "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sSession: %08X\r\n\r\n",
	   cseq, dateHeader(), _sessionId);
}

void CamRTSPClientSession::handleCmd_withinSession(char const* cmdName,
			  char const* urlPreSuffix, char const* urlSuffix,
			  char const* cseq, char const* fullRequestStr) 
{
	CamServerMediaSubsession* subsession;
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
			handleCmd_GET_PARAMETER(cseq, fullRequestStr);
		} else if (strcmp(cmdName, "SET_PARAMETER") == 0) {
			handleCmd_SET_PARAMETER(cseq, fullRequestStr);
		} else {
			handleCmd_notSupported(cseq);
		}
		return;
	} else if (_pMediaSession == NULL) { // There wasn't a previous SETUP!
		handleCmd_notSupported(cseq);
		return;
	} else if (urlSuffix[0] != '\0' && strcmp(_pMediaSession->GetStreamName(), urlPreSuffix) == 0) {
		// Non-aggregated operation.
		// Look up the media subsession whose track id is "urlSuffix":
		subsession = _pMediaSession->LookupSubsessionByTrackId(urlSuffix);
		if (subsession == NULL) { // no such track!
			handleCmd_notFound(cseq);
			return;
		}
	} else if (strcmp(_pMediaSession->GetStreamName(), urlSuffix) == 0 ||
		strcmp(_pMediaSession->GetStreamName(), urlPreSuffix) == 0) {
		// Aggregated operation
		subsession = NULL;
	} else { // the request doesn't match a known stream and/or track at all!
		handleCmd_notFound(cseq);
		return;
	}

	if (strcmp(cmdName, "TEARDOWN") == 0) {
//		handleCmd_TEARDOWN(subsession, cseq);
	} else if (strcmp(cmdName, "PLAY") == 0) {
		handleCmd_PLAY(subsession, cseq, fullRequestStr);
	} else if (strcmp(cmdName, "PAUSE") == 0) {
//		handleCmd_PAUSE(subsession, cseq);
	} else if (strcmp(cmdName, "GET_PARAMETER") == 0) {
		handleCmd_GET_PARAMETER(cseq, fullRequestStr);
	} else if (strcmp(cmdName, "SET_PARAMETER") == 0) {
//		handleCmd_SET_PARAMETER(subsession, cseq, fullRequestStr);
	}
}

