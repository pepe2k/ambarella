/*
 * adts_rtp_sink.cpp
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
#include <arpa/inet.h>

#include "am_types.h"
#include "helper.h"
#include "adts_rtp_sink.h"

CamAdtsRtpSink *CamAdtsRtpSink::Create()
{
	CamAdtsRtpSink *result = new CamAdtsRtpSink();
	if (result != NULL && result->Construct() != ME_OK) {
		delete result;
		result = 0;
	}
	return result;
}

AM_ERR CamAdtsRtpSink::Construct()
{
	if (inherited::Construct("MPEG4-GENERIC") != ME_OK)
	        return ME_ERROR;

	return ME_OK;
}

CAutoString CamAdtsRtpSink::GetAuxSDPLine() const
{
	// Set up the "a=fmtp:" SDP line for this stream:
	char const* const fmtpFmt =
	"a=fmtp:%d "
	"streamtype=%d;profile-level-id=1;"
	"mode=%s;sizelength=13;indexlength=3;indexdeltalength=3;"
	"config=%s\r\n";
	
	AM_U32 fmtpFmtSize = strlen(fmtpFmt)
	+ 3 /* max char len */
	+ 3 /* max char len */
	+ strlen("AAC-hbr")
	+ strlen("1190");
	
	CAutoString fmtpSDPLine(fmtpFmtSize);
	
	sprintf(fmtpSDPLine.GetStr(), fmtpFmt,
		_rtpPayloadType,	//fmtp
		5,	//streamtype: audio
		"AAC-hbr",
		"1190");		// AudioSpecificConfig ISO/IEC 14496-3

	return fmtpSDPLine;
}

AM_BOOL CamAdtsRtpSink::GetOneFrame(AM_U8 **ppFrameStart, AM_U32 *pFrameSize, AM_BOOL *pLastFragment)
{
	if (_currBufSize <= 0)
		return AM_FALSE;

	*ppFrameStart = _pCurrBuf;
	*pFrameSize = _currBufSize;
	_currBufSize = 0;
	*pLastFragment = AM_TRUE;
	return AM_TRUE;
}

// make sure outBuf is large enough to accomendate the frame
AM_U32 CamAdtsRtpSink::DoSpecialFrameHandling(AM_U8 *outBuf, AM_U8 *pFrameStart,
		AM_U32 frameSize, AM_BOOL lastFragment)
{
	int header_size = 16;		// rtp header + special header

	AM_ASSERT(outBuf != NULL && pFrameStart != NULL);

	// Set up the RTP header (12 bytes):
	AM_U32 rtpHdr = 0x80000000; // RTP version 2
	int fRTPPayloadType = 98;

	AM_U8 adts_headers[7];
	memcpy(adts_headers, pFrameStart, sizeof adts_headers);
	
	// Extract important fields from the headers:
//	AM_BOOL protection_absent = headers[1]&0x01;
	AM_U32 frame_length = ((adts_headers[3]&0x03)<<11) |
		(adts_headers[4]<<3) | ((adts_headers[5]&0xE0)>>5);

	AM_ASSERT(frameSize == frame_length);

	rtpHdr |= (fRTPPayloadType<<16);
	rtpHdr |= _seqNo; // sequence number
	if (lastFragment)
		rtpHdr |= 0x00800000;	//set Marker Bit
	*((AM_U32 *)&outBuf[0]) = htonl(rtpHdr);
	*((AM_U32 *)&outBuf[4]) = htonl(_timeStamp);
	*((AM_U32 *)&outBuf[8]) = htonl(_ssrc);
//	printf("rtpHdr: 0x%x, 0x%x, 0x%x, 0x%x,\n", _outBuf[0], _outBuf[1], _outBuf[2], _outBuf[3]);

	// Set up special header (4 bytes):
	AM_U8 auHeaders[4];
	auHeaders[0] = 0; auHeaders[1] = 16 /* bits */; // AU-headers-length
	auHeaders[2] = frame_length >> 5; auHeaders[3] = (frame_length&0x1F)<<3;

	memcpy(&outBuf[12], auHeaders, sizeof auHeaders);
	memcpy(&outBuf[header_size], pFrameStart + sizeof adts_headers, frame_length - sizeof adts_headers);

	_timeStamp += 1024;
	_seqNo++;
	return frame_length - sizeof adts_headers + header_size;
}


