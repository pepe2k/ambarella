/*
 * simple_audio_recorder.h
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
#ifndef _SIMPLE_AUDIO_RECORDER_H_
#define _SIMPLE_AUDIO_RECORDER_H_

class CSimpleAudioRecorder : public CActiveObject
{
    typedef CActiveObject inherited;

public:
    static CSimpleAudioRecorder* Create();

public:
    void Delete();
    void OnRun();
    AM_ERR ConfigMe(SAudioInfo info);
    AM_ERR Start();
    void Stop();
private:
    CSimpleAudioRecorder();
    AM_ERR Construct();
    ~CSimpleAudioRecorder();
    AM_ERR PerformCmd(CMD& cmd, AM_BOOL isSend);
    AM_ERR ProcessCmd(CMD& cmd);

private:
    CBuffer mCBuffer;

    AM_U8* mpBuffer[AUDIO_BUFFER_COUNT];

private:
    //audio driver
    //int mAudioFormate;
    AM_UINT mCurrentBufferSize;
    AM_UINT mBitsPerFrame;
    IAudioHAL* mpAudioDriver;
    void* mpAudioEncoder;
    IAudioHAL::AudioParam mAudioParam;

    bool mbRun;
    AM_U64 mToTSample;

    void* mpNextFilter;//re-sample filter or audio sink filter
    AM_BOOL mbIsSink;
    SAudioInfo mAudioInfo;
};
#endif
