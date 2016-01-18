/*
 * server_media_session.cpp
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

 #include <stdio.h>
#include <string.h>

#include "am_new.h"
#include "sock_helper.h"
#include "rtp_sink.h"
#include "helper.h"
#include "server_media_session.h"


//-----------------------------------------------------------------------
//
// CamServerMediaSession
//
//-----------------------------------------------------------------------

CamServerMediaSession *CamServerMediaSession::Create(char const *pStreamName,
	char const *pInfo, char const *pDescription)
{
	CamServerMediaSession* result = new CamServerMediaSession();
	if (result != NULL && result->Construct(pStreamName, pInfo, pDescription) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;

}

AM_ERR CamServerMediaSession::Construct(char const* pStreamName, char const* pInfo,
	char const* pDescription)
{
	if (strlen(pStreamName) >= rtsp_string_max ||
		strlen(pInfo) >= rtsp_string_max ||
		strlen(pDescription) >= rtsp_string_max)
		return ME_ERROR;
	
	strcpy(_streamName, pStreamName);
	strcpy(_infoSDPString, pInfo);
	strcpy(_descriptionSDPString, pDescription);
	
	gettimeofday(&_creationTime, NULL);

	return ME_OK;
}

void CamServerMediaSession::Delete()
{
    delete this;
}

CamServerMediaSession::~CamServerMediaSession()
{
	for (CamServerMediaSubsession *pSubsession = _pSubsessionsHead, *tmp;
		pSubsession != NULL; pSubsession = tmp) {
		tmp = pSubsession->_pNext;
		if (pSubsession->DecRefCount() == 0)
			delete pSubsession;
	}
}

AM_ERR CamServerMediaSession::AddSubsession(CamServerMediaSubsession* pSubsession) {
//	if (pSubsession->_pParentSession != NULL)		// remove this check would be dangeous, but we must reuse subsession
//		return ME_ERROR; // it's already used

	if (_pSubsessionsTail == NULL) {
		_pSubsessionsHead = pSubsession;
	} else {
		_pSubsessionsTail->_pNext = pSubsession;
	}
	_pSubsessionsTail = pSubsession;

	pSubsession->_pParentSession = this;
	pSubsession->_trackNumber = ++_subsessionCounter;
	pSubsession->AddRefCount();
	return ME_OK;
}

CamServerMediaSubsession* CamServerMediaSession::LookupSubsessionByTrackId(const char* pTrackId)
{
	for (CamServerMediaSubsession* pSubsession = _pSubsessionsHead;
		pSubsession != NULL; pSubsession = pSubsession->_pNext) {
		if (strcmp(pSubsession->GetTrackIdStr().GetStr(), pTrackId) == 0)	
			return pSubsession;
	}
	return NULL;
}

float CamServerMediaSession::duration() const {
	float minSubsessionDuration = 0.0;
	float maxSubsessionDuration = 0.0;

	for (CamServerMediaSubsession* subsession = _pSubsessionsHead; subsession != NULL;
		subsession = subsession->_pNext) {

		float ssduration = subsession->duration();
		if (subsession == _pSubsessionsHead) { // this is the first subsession
			minSubsessionDuration = maxSubsessionDuration = ssduration;
		} else if (ssduration < minSubsessionDuration) {
			minSubsessionDuration = ssduration;
		} else if (ssduration > maxSubsessionDuration) {
			maxSubsessionDuration = ssduration;
		}
	}

	if (maxSubsessionDuration != minSubsessionDuration) {
		return -maxSubsessionDuration; // because subsession durations differ
	} else {
		return maxSubsessionDuration; // all subsession durations are the same
	}
}

CAutoString CamServerMediaSession::GenerateSDPDescription()
{
	AM_U32 OurIPAddress;
	const char* const libNameStr = "Ambarella Streaming Media v";
	const char* const libVersionStr = "2011.4.12";
	const char* const sourceFilterLine = "";
	const char* const rangeLine = "a=range:npt=0-\r\n";
	const char* const fMiscSDPLines = "";
	
	GetOurIPAddress(&OurIPAddress);
	char* const ipAddressStr = HostAddressToString(OurIPAddress);

	// Count the lengths of each subsession's media-level SDP lines.
	// (We do this first, because the call to "subsession->sdpLines()"
	// causes correct subsession 'duration()'s to be calculated later.)

#if 0
	sdpDescription = "v=0\r\n"
	"o=- 1073934419791938 1 IN IP4 10.0.0.2\r\n"
	"s=Session streamed by \"testOnDemandRTSPServer\"\r\n"
	"i=aacAudioTest\r\n"
	"t=0 0\r\n"
	"a=tool:LIVE555 Streaming Media v2011.03.05\r\n"
	"a=type:broadcast\r\n"
	"a=control:*\r\n"
	"a=range:npt=0-\r\n"
	"a=x-qt-text-nam:Session streamed by \"testOnDemandRTSPServer\"\r\n"
	"a=x-qt-text-inf:aacAudioTest\r\n"
	"m=audio 0 RTP/AVP 96\r\n"
	"c=IN IP4 0.0.0.0\r\n"
	"b=AS:96\r\n"
	"a=rtpmap:96 MPEG4-GENERIC/48000/2\r\n"
	"a=fmtp:96 streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1190\r\n"
	"a=control:track1\r\n\r\n";
#endif
	char const* const sdpPrefixFmt =
	"v=0\r\n"
	"o=- %ld%06ld %d IN IP4 %s\r\n"
	"s=%s\r\n"
	"i=%s\r\n"
	"t=0 0\r\n"
	"a=tool:%s%s\r\n"
	"a=type:broadcast\r\n"
	"a=control:*\r\n"
	"%s"
	"%s"
	"a=x-qt-text-nam:%s\r\n"
	"a=x-qt-text-inf:%s\r\n"
	"%s";

	AM_U32 sdpLength = strlen(sdpPrefixFmt)
	+ 20 + 6 + 20 + strlen(ipAddressStr)
	+ strlen(_descriptionSDPString)
	+ strlen(_infoSDPString)
	+ strlen(libNameStr) + strlen(libVersionStr)
	+ strlen(sourceFilterLine)
	+ strlen(rangeLine)
	+ strlen(_descriptionSDPString)
	+ strlen(_infoSDPString)
	+ strlen(fMiscSDPLines);

	CAutoString sdpDesc(sdpLength);

	// Generate the SDP prefix (session-level lines):
	sprintf(sdpDesc.GetStr(), sdpPrefixFmt,
		_creationTime.tv_sec, _creationTime.tv_usec, // o= <session id>
		1, // o= <version> // (needs to change if params are modified)
		ipAddressStr, // o= <address>
		_descriptionSDPString, // s= <description>
		_infoSDPString, // i= <info>
		libNameStr, libVersionStr, // a=tool:
		sourceFilterLine, // a=source-filter: incl (if a SSM session)
		rangeLine, // a=range: line
		_descriptionSDPString, // a=x-qt-text-nam: line
		_infoSDPString, // a=x-qt-text-inf: line
		fMiscSDPLines); // miscellaneous session SDP lines (if any)

	// Then, add the (media-level) lines for each subsession:

	for (CamServerMediaSubsession* pSubsession = _pSubsessionsHead;
		pSubsession != NULL; pSubsession = pSubsession->_pNext) {
		CAutoString mediaSdpLines = pSubsession->GetSdpLines();
		sdpDesc += mediaSdpLines;
	}

	return sdpDesc;
};


//-----------------------------------------------------------------------
//
// CamServerMediaSubsession
//
//-----------------------------------------------------------------------

AM_ERR CamServerMediaSubsession::Construct(const char* pMediaTypeStr)
{
	_pMediaTypeStr = new char[strlen(pMediaTypeStr) + 1];
	strncpy(_pMediaTypeStr, pMediaTypeStr, strlen(pMediaTypeStr) + 1);

	if ((_pRtpSinkMutex = CMutex::Create()) == NULL)
		return ME_OS_ERROR;

	for (int i = 0; i < max_client_num; i++) {
		_pDestinations[i] = NULL;
	}
	return ME_OK;
}

CamServerMediaSubsession::~CamServerMediaSubsession()
{
	if (_pMediaTypeStr != NULL)
		delete[] _pMediaTypeStr;

	AM_DELETE(_pRtpSink);
	__LOCK(_pRtpSinkMutex);
	__UNLOCK(_pRtpSinkMutex);
	AM_DELETE(_pRtpSinkMutex);
}

void CamServerMediaSubsession::Delete()
{
    delete this;
}

CAutoString CamServerMediaSubsession::GetTrackIdStr() const
{
	if (_trackNumber == 0) return CAutoString(); // not yet in a ServerMediaSession

	CAutoString trackIdStr(8);
	sprintf(trackIdStr.GetStr(), "track%d", _trackNumber);
	return trackIdStr;
}

CAutoString CamServerMediaSubsession::GetSdpLines()
{
	if (_pRtpSink == NULL)
		_pRtpSink = CreateNewRtpSink();

	char* pIpAddressStr = HostAddressToString(_serverAddressForSDP);
	CAutoString rtpmapLine = _pRtpSink->RtpmapLine();
	CAutoString rangeLine = GetRangeSdpLine();
	CAutoString auxSDPLine = _pRtpSink->GetAuxSDPLine();
	CAutoString trackIdStr = GetTrackIdStr();

	AM_U32 estBitrate = 500; // kbps, estimate

	const char* const sdpFmt =
	"m=%s %u RTP/AVP %d\r\n"
	"c=IN IP4 %s\r\n"
	"b=AS:%u\r\n"
	"%s"
	"%s"
	"%s"
	"a=control:%s\r\n";
	AM_U32 sdpFmtSize = strlen(sdpFmt)
	+ strlen(_pMediaTypeStr) + 5 /* max short len */ + 3 /* max char len */
	+ strlen(pIpAddressStr)
	+ 20 /* max int len */
	+ strlen(rtpmapLine.GetStr())
	+ strlen(rangeLine.GetStr())
	+ strlen(auxSDPLine.GetStr())
	+ strlen(trackIdStr.GetStr());

	CAutoString sdpLines(sdpFmtSize);
	
	sprintf(sdpLines.GetStr(), sdpFmt,
		_pMediaTypeStr, // m= <media>
		_portNumForSDP, // m= <port>
		_pRtpSink->GetRtpPayloadType(), // m= <fmt list>
		pIpAddressStr, // c= address
		estBitrate, // b=AS:<bandwidth>
		rtpmapLine.GetStr(), // a=rtpmap:... (if present)
		rangeLine.GetStr(), // a=range:... (if present)
		auxSDPLine.GetStr(), // optional extra SDP line
		trackIdStr.GetStr()); // a=control:<track-id>

	return sdpLines;
}

float CamServerMediaSubsession::duration() const {
	// default implementation: assume an unbounded session:
	return 0.0;
}

CAutoString CamServerMediaSubsession::GetRangeSdpLine() const {
	if (_pParentSession == NULL) return CAutoString();

	// If all of our parent's subsessions have the same duration
	// (as indicated by "fParentSession->duration() >= 0"), there's no "a=range:" line:
	if (_pParentSession->duration() >= 0.0) return CAutoString();

	// Use our own duration for a "a=range:" line:
	float ourDuration = duration();
	if (ourDuration == 0.0) {
		return CAutoString("a=range:npt=0-\r\n");
	} else {
		CAutoString rangeLine(100);
		sprintf(rangeLine.GetStr(), "a=range:npt=0-%.3f\r\n", ourDuration);
		return rangeLine;
	}
}

AM_ERR CamServerMediaSubsession::AddDestination(AM_U32 clientSessionID, int sock, AM_U32 addr, AM_U16 port)
{
	int i;
//	printf("AddDestination: this %p, session %d, sock %d, addr 0x%x, port %d\n", this, clientSessionID, sock, addr, port);

	for (i = 0; i < max_client_num; i++) {
		if (_pDestinations[i] == NULL)
			break;
	}
	if (i == max_client_num)	// has already reached maxiumn client number
		return ME_ERROR;

	if (addr != 0 && port != 0)	// UDP
		_pDestinations[i] = new CamDestination(clientSessionID, sock, addr, port);
	else						// TCP
		_pDestinations[i] = new CamDestination(clientSessionID, sock);

	_destinationCount++;
	return ME_OK;
}

AM_ERR CamServerMediaSubsession::DeleteDestination(AM_U32 clientSessionID)
{
	int i;

	for (i = 0; i < max_client_num; i++) {
		if (_pDestinations[i] != NULL && _pDestinations[i]->_clientSessionID == clientSessionID)
			break;
	}
	if (i == max_client_num)	// session ID not found
		return ME_ERROR;

	delete _pDestinations[i];
	_pDestinations[i] = NULL;
	_destinationCount--;
	return ME_OK;
}

CamDestination* CamServerMediaSubsession::GetNextDestination() const
{
	static int i = -1;
	while (++i < max_client_num) {
		if (_pDestinations[i] != NULL)
			return _pDestinations[i];
	}
	i = -1;
	return NULL;
}

void CamServerMediaSubsession::RemoveRtpSink()
{
	AUTO_LOCK(_pRtpSinkMutex);
	AM_DELETE(_pRtpSink);
	_pRtpSink = NULL;
}

