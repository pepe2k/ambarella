/*
 * simple_audio_sink.h
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


#ifndef _SIMPLE_AUDIO_SINK_H_
#define _SIMPLE_AUDIO_SINK_H_

#include "rtsp_vod.h"
#include "g711_codec.h"

class CSimpleAudioSink : public CActiveObject
{
    typedef CActiveObject inherited;

public:
    static CSimpleAudioSink* Create();
public:
    void Delete();
    void OnRun();

    AM_ERR ConfigSink(SAudioInfo info);
    void Start();
    void Stop();
    AM_ERR FeedData(AM_U8* data, AM_UINT size, AM_U64 pts);

private:
    CSimpleAudioSink();
    AM_ERR Construct();
    ~CSimpleAudioSink();

    AM_ERR PerformCmd(CMD& cmd, AM_BOOL isSend);
    AM_ERR ProcessCmd(CMD& cmd);
    AM_ERR ProcessData(CGBuffer* pBuffer);

private:
    CGBuffer mBuffer;
    CGBuffer* mpBuffer;

    AM_U8* mpInputBuffer[AUDIO_BUFFER_COUNT];
    AM_U8* mpOutputBuffer;
    AM_UINT mBufferIndex;

    CQueue* mpDataQ;
    bool mbRun;

    CG711Codec* mpG711Encoder;

private:
    RtspMediaSession* mprtspInjector;
    SAudioInfo mAudioInfo;
    FILE* mpDumpFile;
};
#endif
