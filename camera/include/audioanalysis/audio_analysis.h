/**
 * audio_analysis.h
 *
 *  History:
 *		Jan 3, 2014 - [binwang] created file
 *
 * Copyright (C) 2014-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef AUDIO_ANALYSIS_H_
#define AUDIO_ANALYSIS_H_

#include <pulse/pulseaudio.h>
#include <alsa/asoundlib.h>
extern "C" {
    #include "audioanalytic.h"
}

#define DEFAULT_RATE 16000

#ifndef AUDIO_ANALY_MOD_MAX_NUM
#define AUDIO_ANALY_MOD_MAX_NUM (6)
#endif

#ifndef AUDIO_ANALY_MOD_MAX_NAME_LEN
#define AUDIO_ANALY_MOD_MAX_NAME_LEN (128)
#endif

struct audio_analy_param {
    char aa_mod_names[AUDIO_ANALY_MOD_MAX_NAME_LEN];
    uint32_t aa_mod_th;
    audio_analy_param(uint32_t mod_th) :
        aa_mod_th(mod_th){}
    audio_analy_param() :
        aa_mod_th(0){}
};

class AmAudioDetect;

class AmAudioAnalysis
{
  public:
    static AmAudioAnalysis *Create(snd_pcm_format_t);
    int32_t InitAudioAnalysis();
    void StopAudioAnalysis();
    void SetAudioAnalysisDirection(int);
    int  GetAudioAnalysisDirection();
    void SetAudioAnalysisModNum(uint32_t);
    uint32_t GetAudioAnalysisModNum();
    void SetAudioAnalysisMod(audio_analy_param *);
    audio_analy_param *GetAudioAnalysisMod();
    void SetAudioAnalysisCallback(void(*callback)());

  public:
    AmAudioAnalysis(snd_pcm_format_t audioFormat);
    void AmAudioAnalysisDetect(void *, int, uint64_t, AmAudioDetect *);
    bool Construct();
    virtual ~AmAudioAnalysis();

  private:
    bool StoreResult();

  private:
    int mAnalysisDirection;
    audio_analy_param      mAAParam[AUDIO_ANALY_MOD_MAX_NUM];
    uint32_t               mAAModNum;
    void *id;
    short *frame_buffer;
    int frame_length;
    int **soundpack_number;
    int usable_models;
    char **usable_model_names;
    void (*mpCallback)();
    snd_pcm_format_t mAudioFormat;
    aa_results_t *results;
};

#endif /* AUDIO_ANALYSIS_H_ */
