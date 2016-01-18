/*
 * am_audioalert.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 31/01/2013 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AUDIOALERT_H__
#define __AUDIOALERT_H__

#include <pulse/pulseaudio.h>
#include <alsa/asoundlib.h>

#define REFERENCE_COUNT 3
#define MICRO_AVERAGE_SAMPLES 30
#define MACRO_AVERAGE_SAMPLES 48000

class AmAudioDetect;

/*
 * Constants defined in this enum are threshold.
 * It means that an emergency recording will be
 * started when current audio volume is greater
 * than the threshold.
 */
enum AmAudioSensitivity {
   AM_AUDIO_SENSITIVITY_HIGE = 5,
   AM_AUDIO_SENSITIVITY_HIG  = 20,
   AM_AUDIO_SENSITIVITY_MID  = 30,
   AM_AUDIO_SENSITIVITY_LOW  = 50,
   AM_AUDIO_SENSITIVITY_LOWE = 80,
   AM_AUDIO_SENSITIVITY_DEAF = 100
};

class AmAudioAlert
{
  public:
    static AmAudioAlert *Create(snd_pcm_format_t, AmAudioSensitivity);
    bool SetAudioAlertSensitivity(int);
    int  GetAudioAlertSensitivity();
    void SetAudioFormat(pa_sample_format_t);
    snd_pcm_format_t GetAudioFormat();
    void SetAudioAlertDirection(int);
    int  GetAudioAlertDirection();
    void SetAudioAlertCallback(void(*callback)());
    void AudioAlertDetect(void *, int, uint64_t, AmAudioDetect *);
    bool IsSilent(int percent = 5);

  public:
    AmAudioAlert(snd_pcm_format_t, AmAudioSensitivity);
    bool Construct();
    virtual ~AmAudioAlert();

  private:
    bool StoreMicroHistoryData(int);
    void StoreMacroHistoryData(long long);

  private:
    AmAudioSensitivity mAudioSen;
    int mAlertDirection;
    long long mVolumeSum;
    int mSampleNum;
    int mBitsPerSample;
    int mMaximumPeak;
    int mMicroHistoryHead;
    int mMacroHistoryHead;
    int mMicroHistoryTail;
    int mMacroHistoryTail;
    int mMicroHistory[REFERENCE_COUNT + 1];
    long long mMacroHistory[REFERENCE_COUNT + 1];
    void (*mpCallback)();
    snd_pcm_format_t mAudioFormat;
};

#endif
