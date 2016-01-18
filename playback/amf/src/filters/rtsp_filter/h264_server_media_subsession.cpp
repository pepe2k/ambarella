/*
 * h264_server_media_subsession.cpp
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

 #include "am_types.h"
#include "h264_rtp_sink.h"
#include "h264_server_media_subsession.h"

CamRtpSink* CamH264ServerMediaSubsession::CreateNewRtpSink()
{
	return CamH264RtpSink::Create();
}

CamH264ServerMediaSubsession* CamH264ServerMediaSubsession::Create(const char* pMediaTypeStr)
{
	CamH264ServerMediaSubsession* result = new CamH264ServerMediaSubsession();
	if (result != NULL && result->Construct(pMediaTypeStr) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

