/*
 * simple_audio_common.h
 *
 * History:
 *    2013/11/11 - [SkyChen] create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef _SIMPLE_AUDIO_COMMON_H_
#define _SIMPLE_AUDIO_COMMON_H_

#include "am_types.h"

#define INPUT_SAMPLE_RATE (48000)
#define OUTPUT_SAMPLE_RATE (8000)
#define AUDIO_BUFFER_COUNT (10)
#define D_AUDIO_MAX_CHANNELS  8

enum AudioFormateType
{
    AUDIO_ALAW = 0,
    AUDIO_ULAW = 1,
};

typedef struct SAudioInfo
{
    AM_UINT uChannel;
    AM_UINT uSampleRate;
    AM_BOOL bSave;
    char cFileName[32];
    AM_BOOL bEnableRTSP;
    char cURL[32];

    AudioFormateType eFormateType;

} SAudioInfo;
#endif
