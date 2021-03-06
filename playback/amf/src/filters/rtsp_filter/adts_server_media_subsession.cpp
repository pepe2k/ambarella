/*
 * adts_server_media_subsession.cpp
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

#include "rtp_sink.h"
#include "adts_rtp_sink.h"
#include "adts_server_media_subsession.h"

CamRtpSink* CamAdtsServerMediaSubsession::CreateNewRtpSink()
{
	return CamAdtsRtpSink::Create();
}

CamAdtsServerMediaSubsession* CamAdtsServerMediaSubsession::Create(const char* pMediaTypeStr)
{
	CamAdtsServerMediaSubsession* result = new CamAdtsServerMediaSubsession();
	if (result != NULL && result->Construct(pMediaTypeStr) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

