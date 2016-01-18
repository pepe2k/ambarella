/*
 * resample_filter.h
 *
 * History:
 *    2013/11/11 - [SkyChen] create file
 *    Please refer audio_effecter.h
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef _RESAMPLE_FILTER_H
#define _RESAMPLE_FILTER_H

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/opt.h"
}

class CReSampleFilter : public CActiveObject
{
    typedef CActiveObject inherited;

public:
    static CReSampleFilter* Create();

public:
    AM_ERR ConfigConvertor(SAudioInfo info);
    AM_ERR FeedData(CBuffer* buffer);
    void Delete();
    void OnRun();
    void Stop();

private:
    CReSampleFilter();
    AM_ERR Construct();
    ~CReSampleFilter();

    AM_ERR PerformCmd(CMD& cmd, AM_BOOL isSend);
    AM_ERR ProcessCmd(CMD& cmd);
    AM_ERR ProcessData(CBuffer* buffer);

private:
    CQueue* mpAudioQ;

private:
    struct AVResampleContext * mpResampler;
    AM_UINT mReservedSamples[D_AUDIO_MAX_CHANNELS];
    AM_INT mChannel;

    short* mpInputBufferChannelInterleave[D_AUDIO_MAX_CHANNELS];
    short* mpOutputBufferChannelInterleave[D_AUDIO_MAX_CHANNELS];

    short* mpInputBufferAll[D_AUDIO_MAX_CHANNELS];
    AM_UINT mInputBufferAllSample[D_AUDIO_MAX_CHANNELS];
    short* mpOutputBuffer;

    AM_UINT mBufferIndex;

private:
    bool mbRun;
    CBuffer mBuffer;

    CSimpleAudioSink* mpAudioSink;
};
#endif
