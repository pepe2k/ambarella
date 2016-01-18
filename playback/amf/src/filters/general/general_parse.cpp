/*
 * general_parse.cpp
 *
 * History:
 *    2012/4/6 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

GDemuxerParser* gDemuxerList[] =
{
    //&gFFMpegDemuxer,
    //&gFFMpegDemuxerNet,
    &gFFMpegDemuxerQueEx,
    &gFFMpegDemuxerNetEx,
    NULL,
};


GDecoderParser* gDecoderList[] =
{
    &gDecoderFFMpeg,
    &gVideoDspDecoderQue,
    //&gDecoderCoreAVC,
    &gDecoderFFMpegVideo,
    NULL,
};

GRendererParser* gRendererList[] =
{
    &gSyncRenderer,
    NULL,
};

