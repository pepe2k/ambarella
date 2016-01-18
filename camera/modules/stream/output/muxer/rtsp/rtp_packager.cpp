/*******************************************************************************
 * rtp_packager.cpp
 *
 * History:
 *   2012-11-25 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_utility.h"
#include "am_data.h"

#include "rtp_packager.h"

CRtpPackager::CRtpPackager() :
    mBuffer(0),
    mPacket(0),
    mBufMaxSize(0),
    mPktMaxNum(0)
{
}

CRtpPackager::~CRtpPackager()
{
  delete[] mBuffer;
  delete[] mPacket;
  DEBUG("~CRtpPackager");
}
