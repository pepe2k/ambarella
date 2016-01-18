/**
 * audio_analysis.cpp
 *
 *  History:
 *		Jan 10, 2014 - [binwang] created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_include.h"
#include "am_utility.h"
#include <asm/byteorder.h>
#include "am_audioanalysis.h"
#include "am_event.h"

AmAudioAnalysis *AmAudioAnalysis::Create(snd_pcm_format_t audioFormat)
{
    AmAudioAnalysis *result = new AmAudioAnalysis(audioFormat);
    if (result !=NULL && !result->Construct()) {
        ERROR("Failed to create an instance of AmAudioAnalysis\n");
        delete result;
        result = NULL;
    }

    return result;
}

AmAudioAnalysis::AmAudioAnalysis(snd_pcm_format_t audioFormat):

    mAnalysisDirection (0),
    mAAModNum (0),
    id (NULL),
    frame_buffer (NULL),
    frame_length (256),
    soundpack_number(NULL),
    usable_models (0),
    usable_model_names (NULL),
    mpCallback (NULL),
    mAudioFormat (audioFormat)
    {}

AmAudioAnalysis::~AmAudioAnalysis ()
{
    free(soundpack_number);
    free(usable_model_names);
    free(frame_buffer);
    free(results->score);
    free(results);
}

bool AmAudioAnalysis::Construct ()
{
    if ((results = (aa_results_t *)calloc(1, sizeof(aa_results_t))) == NULL) {
        ERROR("Failed to allocate mem for results!\n");
        return false;
    }
    if ((results->score = (int *)calloc(1, sizeof(int) * usable_models)) == NULL) {
        ERROR("Failed to allocate mem for results->score!\n");
        return false;
    }
    if ((soundpack_number = (int **)malloc(sizeof(int *) * mAAModNum)) == NULL) {
        ERROR("Failed to allocate mem for soundpack_number!\n");
        return false;
    }
    if ((usable_model_names = (char **)malloc(sizeof(char *) * mAAModNum)) == NULL) {
        ERROR("Failed to allocate mem for usable names!\n");
        return false;
    }
    if ((frame_buffer = (short *)malloc(sizeof(short) * frame_length)) == NULL) {
        ERROR("Failed to allocate mem for frame buffer!\n");
        return false;
    }

    return true;
}

void AmAudioAnalysis::SetAudioAnalysisDirection(int analysis_direction)
{
    mAnalysisDirection = analysis_direction;
}

int AmAudioAnalysis::GetAudioAnalysisDirection()
{
    return mAnalysisDirection;
}

uint32_t AmAudioAnalysis::GetAudioAnalysisModNum()
{
    return mAAModNum;
}

void AmAudioAnalysis::SetAudioAnalysisMod(audio_analy_param *mod)
{
        mAAParam[mAAModNum++] = *mod;
}

audio_analy_param *AmAudioAnalysis::GetAudioAnalysisMod()
{
    return mAAParam;
}

void AmAudioAnalysis::SetAudioAnalysisCallback(void(*callback)())
{
    mpCallback = callback;
}

int32_t AmAudioAnalysis::InitAudioAnalysis()
{
    uint32_t i, j = 0;
    int threshold[16] = {0};
    id = aa_init(DEFAULT_RATE, &frame_length);
    aa_soundpack_info_t *info = (aa_soundpack_info_t *)malloc(sizeof(aa_soundpack_info_t));
    info->name_length = 100;
    info->name = (char *)malloc(sizeof(char) * info->name_length);
    if (id == NULL){
        ERROR("Could not perform initialization of corelogger. "
                "Note that sample rate must be 8000, 16000 or 44100\n");
        free(info->name);
        free(info);
        return -1;
    }
    usable_models = mAAModNum;
    for (i = 0; i < mAAModNum; i++){
        printf("model_names[%d] is %s\n", i, mAAParam[i].aa_mod_names);
        if (aa_load_soundpack(id, mAAParam[i].aa_mod_names, soundpack_number + j) < 0) {
            WARN("Unable to load soundpack %s \n", mAAParam[i].aa_mod_names);
            usable_models--;
        } else {
            aa_get_soundpack_info(mAAParam[i].aa_mod_names, info);
            threshold[j] = info->threshold;
            NOTICE("Recommend threshold: %.2f,\"%s\"\n", threshold[j] / 65536.0f, info->name);
            usable_model_names[j] = mAAParam[i].aa_mod_names;
            j++;
        }
    }

    free(info->name);
    free(info);

    if (usable_models == 0) {
        WARN("No usable sound pack!\n");
    }

    return 0;
}

void AmAudioAnalysis::StopAudioAnalysis()
{
    aa_close(id);
    id = NULL;
}

void AmAudioAnalysis::AmAudioAnalysisDetect(void *pcmData, int len, uint64_t pts, AmAudioDetect *instance)
{
    int i, score = 0;
    audio_detect_msg_t msg;
    static int seq_num = 0;

    if (usable_models > 0) {
        results->score_length = usable_models;
        memcpy(frame_buffer, pcmData, frame_length);
        if((aa_classify_s(id, frame_buffer, results)) < 0){
            ERROR("ERROR: classifying frame %lld\n", pts);
            return;
        }
    } else {
        WARN("No usabel models for analysis detecting, please check /etc/camera/audiodetect.conf!\n");
        sleep (1);
        return;
    }

    for(i = 0; i < usable_models; ++i) {
        score = results->score[*soundpack_number[i]];
        if (mAAParam[i].aa_mod_th > ((float)score/ 65536.0f) && ((float)score/ 65536.0f) > 0) {

            NOTICE("Model:%s  pts:%lld, score:%.6f\n", usable_model_names[i], pts,
                    (float)results->score[*soundpack_number[i]] / 65536.0f);

            /*NOTICE("average level(dB): %.6f, Max level(dB): %.6f, Max Freq(Hz): %d\n",
                   (float)results->average_level_db / 65536.0f,
                   (float)results->max_level_db / 65536.0f, results->max_level_hz);*/

            memset (&msg, 0, sizeof (audio_detect_msg_t));
            msg.seq_num  = ++seq_num;
            msg.msg_type = AM_AUDIO_DETECT_ANALY;
            msg.msg_pts  = pts;

            pthread_mutex_lock (&instance->mRingBufferMutex);
            AudioDetectRingBufferWrite (instance->mpRingBuffer, &msg);
            pthread_cond_signal (&instance->mRingBufferCond);
            pthread_mutex_unlock (&instance->mRingBufferMutex);
            DEBUG ("Write an audio analysis message: seq_num = %d", seq_num);
        }
    }
}
