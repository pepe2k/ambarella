/*
 * test_audio_detect.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 09/01/2014 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_configure.h"
#include "amevent.h"
#include "am_audio_detect.h"

#ifndef PROPERTY_MAX_LEN
#define PROPERTY_MAX_LEN 128
#endif

#ifndef AUDIODETECT_CONFIG_PATH
#define AUDIODETECT_CONFIG_PATH "/etc/camera/audiodetect.conf"
#endif

static MODULE_CONFIG  *module_config;
static bool           g_isrunning;
static AmEvent        *event;

static void
show_menu ()
{
   printf ("\n");
   printf (" ======== Test AmAudioDetect ======== \n\n");
   printf (" b ===== Begin Audio Detect ======== \n");
   printf (" s ===== Stop  Audio Detect ======== \n");
   printf (" q ===== Quit  Audio Detect ======== \n\n");
   printf (" ====================================\n\n");
}

static void mainloop ()
{
   g_isrunning = true;

   while (g_isrunning) {
      show_menu();
      switch (getchar ()) {
      case 'b':
      case 'B': {
         if (event->start_event_monitor (AUDIO_DECT) < 0) {
            ERROR ("Failed start audio detect module!");
            g_isrunning = false;
         }
      } break;

      case 's':
      case 'S': {
         if (event->stop_event_monitor (AUDIO_DECT) < 0) {
            NOTICE ("Failed to stop audio detect module, maybe module doesn't start!");
         }
      } break;

      case 'q':
      case 'Q': {
         g_isrunning = false;
      } break;

      case '\n':
         continue;

      default:
         break;
      }
   }
}

static int audio_alert_callback (audio_detect_msg_t *message)
{
   NOTICE ("Audio alert: seqNum = %d", message->seq_num);
   return 0;
}

static void sigstop (int signo)
{
   if (event) {
      event->destroy_event_monitor (AUDIO_DECT);
      delete event;
   }

   NOTICE ("Stop Audio Detect.");
   exit (1);
}

static int init_audiodetect_config ()
{
   AmConfig *config = NULL;
   AudioDetectParameters *detectParams = NULL;
   char key[PROPERTY_MAX_LEN];
   int ret = 0;
   int call_value;
   uint32_t i;

   do {
      if ((config = new AmConfig ()) == NULL) {
         ERROR ("Failed to create an instance of AmConfig!");
         ret = -1;
         break;
      }

      config->set_audiodetect_config_path (AUDIODETECT_CONFIG_PATH);
      if (config->load_audiodetect_config ()) {
         detectParams = config->audiodetect_config ();
      } else {
         ERROR ("Failed to load audio detect config!");
         ret = -1;
         break;
      }

      if ((module_config = new MODULE_CONFIG ()) == NULL) {
         ERROR ("Failed to create an instance of module_config");
         ret = -1;
         break;
      } else {
         module_config->key = key;
      }

      if (detectParams) {
         DEBUG ("audio_channel_number: %d", detectParams->audio_channel_number);
         DEBUG ("audio_sample_rate: %d", detectParams->audio_sample_rate);
         DEBUG ("audio_chunk_bytes: %d", detectParams->audio_chunk_bytes);
         DEBUG ("enable_alert_detect: %d", detectParams->enable_alert_detect);
         DEBUG ("audio_alert_sensitivity: %d", detectParams->audio_alert_sensitivity);
         DEBUG ("audio_alert_direction: %d", detectParams->audio_alert_direction);
         DEBUG ("enable_analysis_detect: %d", detectParams->enable_analysis_detect);
         DEBUG ("audio_analysisdirection: %d", detectParams->audio_analysis_direction);
         DEBUG ("audio_analysis_module_num: %d", detectParams->audio_analysis_mod_num);
         DEBUG ("enable_analysis_detect: %d", detectParams->enable_analysis_detect);
         for (i = 0; i < detectParams->audio_analysis_mod_num; i++) {
             DEBUG ("audio_analysis_mod_num%d: %s", i + 1, detectParams->aa_param[i].aa_mod_names);
             DEBUG ("audio_analysis_sen%d: %d", i + 1, detectParams->aa_param[i].aa_mod_th);
         }
      } else {
         ERROR ("Failed to fetch auido detect parameters!");
         ret = -1;
         break;
      }

      /* Set properties for audio detect module */
      module_config->value = &detectParams->audio_channel_number;
      strcpy (module_config->key, "AudioChannelNumber");
      call_value = event->set_monitor_config (AUDIO_DECT, module_config);
      if (call_value == -1) {
         ERROR ("Failed to set audio channel number");
         ret = -1;
         break;
      } else if (call_value == 1) {
         NOTICE ("Audio Channel can not be changed dynamically");
      }

      module_config->value = &detectParams->audio_sample_rate;
      strcpy (module_config->key, "AudioSampleRate");
      call_value = event->set_monitor_config (AUDIO_DECT, module_config);
      if (call_value == -1) {
         ERROR ("Failed to set audio sample rate");
         ret = -1;
         break;
      } else if (call_value == 1) {
         NOTICE ("Audio sample rate can not be changed dynamically");
      } else {
         DEBUG ("Audio sample rate is set to %d", detectParams->audio_sample_rate);
      }

      module_config->value = &detectParams->audio_chunk_bytes;
      strcpy (module_config->key, "AudioChunkBytes");
      call_value = event->set_monitor_config (AUDIO_DECT, module_config);
      if (call_value == -1) {
         ERROR ("Failed to set audio chunk bytes");
         ret = -1;
         break;
      } else if (call_value == 1) {
         NOTICE ("Audio chunk bytes can not be changed dynamically");
      } else {
         DEBUG ("Audio chunk bytes is set to %d", detectParams->audio_chunk_bytes);
      }

      module_config->value = &detectParams->enable_alert_detect;
      strcpy (module_config->key, "EnableAlertDetect");
      call_value = event->set_monitor_config (AUDIO_DECT, module_config);
      if (call_value == -1) {
         ERROR ("Failed to enable or disable alert detect");
         ret = -1;
         break;
      } else if (call_value == 1) {
         NOTICE ("Enable or disable alert detect can not be changed dynamically");
      } else {
         DEBUG ("Enable alert detect or not: %d", detectParams->enable_alert_detect);
      }

      module_config->value = &detectParams->audio_alert_sensitivity;
      strcpy (module_config->key, "AudioAlertSensitivity");
      call_value = event->set_monitor_config (AUDIO_DECT, module_config);
      if (call_value == -1) {
         ERROR ("Failed to set audio alert sensitivity");
         ret = -1;
         break;
      } else if (call_value == 1) {
         NOTICE ("Audio alert sensitivity can not be changed dynamically");
      } else {
         DEBUG ("Audio alert sensitivity is set to %d", detectParams->audio_alert_sensitivity);
      }

      module_config->value = &detectParams->audio_alert_direction;
      strcpy (module_config->key, "AudioAlertDirection");
      call_value = event->set_monitor_config (AUDIO_DECT, module_config);
      if (call_value == -1) {
         ERROR ("Failed to set audio alert direction");
         ret = -1;
         break;
      } else if (call_value == 1) {
         NOTICE ("Audio alert direction can not be changed dynamically");
      } else {
         DEBUG ("Audio alert direction: %d", detectParams->audio_alert_direction);
      }

      module_config->value = &detectParams->enable_analysis_detect;
      strcpy (module_config->key, "EnableAnalysisDetect");
      call_value = event->set_monitor_config (AUDIO_DECT, module_config);
      if (call_value == -1) {
         ERROR ("Failed to enable or disable analysis detect");
         ret = -1;
         break;
      } else if (call_value == 1) {
         NOTICE ("Enable or disable analysis detect can not be changed dynamically");
      } else {
         DEBUG ("Enable analysis detect: %d", detectParams->enable_analysis_detect);
      }

      module_config->value = &detectParams->audio_analysis_direction;
      strcpy (module_config->key, "AudioAnalysisDirection");
      call_value = event->set_monitor_config (AUDIO_DECT, module_config);
      if (call_value == -1) {
         ERROR ("Failed to set AudioAnalysisDirection");
         ret = -1;
         break;
      } else if (call_value == 1) {
         NOTICE ("AudioAnalysisDirection can not be changed dynamically");
      } else {
         DEBUG ("AudioAnalysisDirection: %d", detectParams->audio_analysis_direction);
      }

      for (uint32_t i = 0; i < detectParams->audio_analysis_mod_num; i++) {
          module_config->value = &detectParams->aa_param[i];
          NOTICE("detectParams->aa_param[%d].aa_mod_names=%s\n", i, detectParams->aa_param[i].aa_mod_names);
          NOTICE("detectParams->aa_param[%d].aa_mod_th=%d\n", i, detectParams->aa_param[i].aa_mod_th);
          strcpy (module_config->key, "AudioAnalysisMod");
          call_value = event->set_monitor_config (AUDIO_DECT, module_config);
          if (call_value == -1) {
             ERROR ("Failed to set AudioAnalysisMod");
             ret = -1;
             break;
          } else if (call_value == 1) {
             NOTICE ("AudioAnalysisMod can not be changed dynamically");
          } else {
             DEBUG ("AudioAnalysisMod: %d", detectParams->aa_param[1].aa_mod_th);
          }

      }

      module_config->value = (void *)audio_alert_callback;
      strcpy (module_config->key, "AudioAlertDetectCallback");
      call_value = event->set_monitor_config (AUDIO_DECT, module_config);
      if (call_value == -1) {
         ERROR ("Failed to set audio alert callback");
         ret = -1;
         break;
      } else if (call_value == 1) {
         NOTICE ("Audio alert callback can not be changed dynamically");
      } else {
         DEBUG ("Alert callback has been set!");
      }
   } while (0);

   /* Release memory */
   if (config) {
      delete config;
   }

   if (module_config) {
      delete module_config;
   }

   return ret;
}

int
main (int argc, char **argv)
{
   signal (SIGINT,  sigstop);
   signal (SIGQUIT, sigstop);
   signal (SIGTERM, sigstop);

   if ((event = AmEvent::get_instance ()) == NULL) {
      ERROR ("Failed to get an instance of AmEvent!");
      return -1;
   }

   if (init_audiodetect_config () < 0) {
      ERROR ("Failed to initialize properties for audio detect");
      return -1;
   }

   mainloop ();

   event->stop_event_monitor (AUDIO_DECT);
   event->destroy_event_monitor (AUDIO_DECT);

   return 0;
}
