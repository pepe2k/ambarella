/*
 * am_config_audiodetect.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 14/02/2014 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include <iniparser.h>
#ifdef __cplusplus
}
#endif

#include "am_include.h"
#include "utilities/am_define.h"
#include "utilities/am_log.h"
#include "datastructure/am_structure.h"
#include "am_config_base.h"
#include "am_config_audiodetect.h"

AudioDetectParameters* AmConfigAudioDetect::get_audiodetect_config()
{
   AudioDetectParameters *ret = NULL;
   if (init()) {
      if (!mAudioDetectParameters) {
         mAudioDetectParameters = new AudioDetectParameters();
      }
      if (mAudioDetectParameters) {
         int32_t audio_channel = get_int("GENERAL:AudioChannelNumber", 2);
         if ((audio_channel == 1) || (audio_channel == 2)) {
            mAudioDetectParameters->audio_channel_number = (uint32_t)audio_channel;
         } else {
            WARN("Invalid audio channel value: %d, reset to 2!", audio_channel);
            mAudioDetectParameters->audio_channel_number = 2;
            mAudioDetectParameters->config_changed = 1;
         }

         mAudioDetectParameters->audio_sample_rate = \
         (uint32_t)get_int ("GENERAL:AudioSampleRate", 48000);

         mAudioDetectParameters->audio_chunk_bytes = \
         (uint32_t)get_int ("GENERAL:AudioChunkBytes", 256);

         int32_t enable_alert = get_int ("ALERT_DETECT:EnableAlertDetect", 0);
         if (enable_alert == 0 || enable_alert == 1) {
            mAudioDetectParameters->enable_alert_detect = (uint32_t)enable_alert;
         } else {
            WARN("Invalid enable or disable audio alert[0 | 1]: %d, reset to 0!", enable_alert);
            mAudioDetectParameters->enable_alert_detect = 0;
            mAudioDetectParameters->config_changed = 1;
         }

         int32_t alert_sen = get_int ("ALERT_DETECT:AudioAlertSensitivity", 2);
         if (alert_sen >= 1 && alert_sen <= 5) {
            mAudioDetectParameters->audio_alert_sensitivity = (uint32_t)alert_sen;
         } else {
            WARN("Invalid audio alert sensitivity[1 - 5]: %d, reset to 2!", alert_sen);
            mAudioDetectParameters->audio_alert_sensitivity = 2;
            mAudioDetectParameters->config_changed = 1;
         }

         int32_t alert_direction = get_int ("ALERT_DETECT:AudioAlertDirection", 1);
         mAudioDetectParameters->audio_alert_direction = alert_direction;
         if (alert_direction == 0 || alert_direction == 1) {
            mAudioDetectParameters->audio_alert_direction = (uint32_t)alert_direction;
         } else {
            WARN("Invalid enable or disable audio alert[0 | 1]: %d, reset to 0!", alert_direction);
            mAudioDetectParameters->audio_alert_direction = 1;
            mAudioDetectParameters->config_changed = 1;
         }

         int32_t enable_analysis = get_int ("ANALYSIS_DETECT:EnableAnalysisDetect", 0);
         if (enable_analysis == 0 || enable_analysis == 1) {
            mAudioDetectParameters->enable_analysis_detect = (uint32_t)enable_analysis;
         } else {
            WARN("Invalid enable or disable audio analysis[0 | 1]: %d, reset to 0!", enable_analysis);
            mAudioDetectParameters->enable_analysis_detect = 0;
            mAudioDetectParameters->config_changed = 1;
         }

         int32_t analysis_direction = get_int ("ANALYSIS_DETECT:AudioAnalysisDirection", 1);
         if (analysis_direction == 0 || analysis_direction == 1) {
            mAudioDetectParameters->audio_analysis_direction = (uint32_t)analysis_direction;
         } else {
            WARN("Invalid enable or disable audio analysis[0 | 1]: %d, reset to 0!", analysis_direction);
            mAudioDetectParameters->audio_analysis_direction = 1;
            mAudioDetectParameters->config_changed = 1;
         }

         int32_t analysis_mod_num = get_int ("ANALYSIS_DETECT:AudioAnalysisModNum", 1);
         if (analysis_mod_num > 0) {
             mAudioDetectParameters->audio_analysis_mod_num = analysis_mod_num;
         } else {
             WARN("Invalid module number of audio analysis : %d, reset to 0!", analysis_mod_num);
             mAudioDetectParameters->audio_analysis_mod_num = 0;
             mAudioDetectParameters->config_changed = 1;
         }

         uint32_t i;
         char* string = (char *)malloc(sizeof(char) * 128);
         char* analysis_mod = NULL;
         uint32_t analysis_sen;
         for (i = 0; i < mAudioDetectParameters->audio_analysis_mod_num; i++) {
             sprintf(string, "%s%d", "ANALYSIS_DETECT:AudioAnalysisMod", i + 1);
             analysis_mod = get_string(string, "/etc/GlassBreak.bsm");
             if (analysis_mod != NULL) {
                 strcpy(mAudioDetectParameters->aa_param[i].aa_mod_names, analysis_mod);
             } else {
                 WARN("Invalid module name of audio analysis!");
                 mAudioDetectParameters->config_changed = 1;
             }

             sprintf(string, "%s%d", "ANALYSIS_DETECT:AudioAnalysisSensitivityMod", i + 1);
             analysis_sen = get_int (string, 4);
             if (analysis_sen > 0) {
                 mAudioDetectParameters->aa_param[i].aa_mod_th = analysis_sen;
             } else {
                 WARN("Invalid threshold of audio analysis!");
                 mAudioDetectParameters->aa_param[i].aa_mod_th = 0;
                 mAudioDetectParameters->config_changed = 1;
             }
         }
         if (mAudioDetectParameters->config_changed) {
            set_audiodetect_config(mAudioDetectParameters);
         }
         free(string);
      }

      ret = mAudioDetectParameters;
   }

   return ret;
}

void AmConfigAudioDetect::set_audiodetect_config(AudioDetectParameters *config)
{
    uint32_t i;
    char* string = (char *)malloc(sizeof(char) * 128);
    if (AM_LIKELY(config)) {
        if (init()) {
            set_value("GENERAL:AudioChannelNumber", config->audio_channel_number);
            set_value("GENERAL:AudioSampleRate", config->audio_sample_rate);
            set_value("GENERAL:AudioChunkBytes", config->audio_chunk_bytes);
            set_value("ALERT_DETECT:EnableAlertDetect", config->enable_alert_detect);
            set_value("ALERT_DETECT:AudioAlertSensitivity", config->audio_alert_sensitivity);
            set_value("ALERT_DETECT:AudioAlertDirection", config->audio_alert_direction);
            set_value("ANALYSIS_DETECT:EnableAnalysisDetect", config->enable_analysis_detect);
            set_value("ANALYSIS_DETECT:AudioAnalysisDirection", config->audio_analysis_direction);
            set_value("ANALYSIS_DETECT:AudioAnalysisModNum", config->audio_analysis_mod_num);
            for (i = 0; i < config->audio_analysis_mod_num; i++) {
                sprintf(string, "%s%d", "ANALYSISDETECT:AudioAnalysisMod", i + 1);
                set_value(string, config->aa_param[i].aa_mod_names);
                sprintf(string, "%s%d", "ANALYSIS_DETECT:AudioAnalysisSensitivityMod", i + 1);
                set_value(string, config->aa_param[i].aa_mod_th);
            }
            config->config_changed = 0;
            save_config();
        } else {
            WARN("Failed openint %s, audio detect configuration NOT saved!", mConfigFile);
        }
    }
    free(string);
}
