/*
 * rtp_sink.cpp
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

 #include <string.h>

#include "am_types.h"
#include "random.h"
#include "helper.h"
#include "rtp_sink.h"


//-----------------------------------------------------------------------
//
// CamRtpSink
//
//-----------------------------------------------------------------------

AM_ERR CamRtpSink::Construct(char const *pRtpPayloadFmtName)
{
	strncpy(_rtpPayloadFormatName, pRtpPayloadFmtName, sizeof _rtpPayloadFormatName);
	_seqNo = (AM_U16)our_random();
	_timeStamp = our_random32();
	_ssrc = our_random32();
	return ME_OK;
}

void CamRtpSink::Delete()
{
    delete this;
}

CAutoString CamRtpSink::RtpmapLine() const
{
	if (_rtpPayloadType >= 96) { // the payload format type is dynamic
		char *encodingParamsPart = new char[1 + 20 /* max int len */];
		if (_numChannels != 1) {
			sprintf(encodingParamsPart, "/%d", _numChannels);
		} else {
			encodingParamsPart[0] = '\0';
		}
		char const* const rtpmapFmt = "a=rtpmap:%d %s/%d%s\r\n";
		AM_U32 rtpmapFmtSize = strlen(rtpmapFmt) 
			+ 3 /* max char len */ + strlen(_rtpPayloadFormatName)
			+ 20 /* max int len */ + strlen(encodingParamsPart);

		CAutoString rtpmapSDPLine(rtpmapFmtSize);

		sprintf(rtpmapSDPLine.GetStr(), rtpmapFmt,
			_rtpPayloadType, _rtpPayloadFormatName,
			_rtpTimestampFrequency, encodingParamsPart);
		
		delete[] encodingParamsPart;
		return rtpmapSDPLine;
	} else {
		// The payload format is staic, so there's no "a=rtpmap:" line:
		return CAutoString();
	}
}

void CamRtpSink::FeedStreamData(AM_U8 *inputBuf, AM_U32 inputBufSize)
{
	_pCurrBuf = inputBuf;
	_currBufSize = inputBufSize;
}


