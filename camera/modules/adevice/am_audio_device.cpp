/*
 * am_audio_device.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 25/06/2013 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_include.h"
#include "am_types.h"
#include "am_adevice.h"
#include <string.h>

#define CONTEXT_NAME ((const char *)"AmAudioDevice")

AmMutex AmAudioDevice::mMutex;
AmAudioDevice *AmAudioDevice::instance = NULL;

struct UserData {
   AmAudioDevice *adev;
   void          *data;
   UserData (AmAudioDevice *dev, void *userData):
      adev (dev),
      data (userData) {}
};

struct AmAudioVolumeInfo {
   uint8_t volume;
   uint8_t mute;
};

AmAudioDevice::AmAudioDevice ():
   mbCtxConnected (false),
   mbSinkMuted (false),
   mbSourceMuted (false),
   mSinkIndex (-1),
   mSourceIndex (-1),
   mContextState (PA_CONTEXT_UNCONNECTED),
   mpSinkVolume (NULL),
   mpSourceVolume (NULL),
   mpContext (NULL),
   mpThreadedMainloop (NULL)
{}

AmAudioDevice::~AmAudioDevice ()
{
   if (mbCtxConnected) {
      pa_context_set_state_callback (mpContext, NULL, NULL);
      pa_context_disconnect (mpContext);
      mbCtxConnected = false;
      DEBUG ("pa_context_disconnect\n");
   }

   if (mpContext) {
      pa_context_unref (mpContext);
      mpContext = NULL;
      DEBUG ("pa_context_unref (mpContext)\n");
   }

   if (mpThreadedMainloop) {
      pa_threaded_mainloop_free (mpThreadedMainloop);
      mpThreadedMainloop = NULL;
      DEBUG ("pa_threaded_mainloop_free (mpThreadedMainloop)\n");
   }

   DEBUG ("Before free mpSinkInfo");
   delete mpSinkVolume;
   DEBUG ("Before free mpSourceInfo");
   delete mpSourceVolume;
}

AmAudioDevice *AmAudioDevice::get_instance ()
{
   mMutex.Lock ();

   if (instance == NULL) {
      if ((instance = new AmAudioDevice ()) == NULL) {
         ERROR ("Failed to create an instance of AmAudioDevice!");
      } else {
         if (instance->audio_device_init () != 0) {
            ERROR ("Failed to initialize audio device!\n");
            delete instance;
            instance = NULL;
         }
      }
   }

   mMutex.Unlock ();
   return instance;
}

int AmAudioDevice::audio_device_init ()
{
   int ret = 0;
   pa_mainloop_api *mainloop_api;

   if (mpThreadedMainloop == NULL) {
      if ((mpThreadedMainloop = pa_threaded_mainloop_new ()) != NULL) {
         UserData userData (this, mpThreadedMainloop);
         mainloop_api = pa_threaded_mainloop_get_api (mpThreadedMainloop);
         mpContext = pa_context_new (mainloop_api, CONTEXT_NAME);

         if (mpContext == NULL) {
            ret = -1;
            INFO ("Failed to new context!\n");
         } else {
            pa_context_set_state_callback (mpContext, static_pa_state, &userData);
            pa_context_connect (mpContext, NULL, PA_CONTEXT_NOFLAGS, NULL);

            if (pa_threaded_mainloop_start (mpThreadedMainloop) != 0) {
               ret = -1;
               INFO ("Failed to start threaded mainloop!\n");
            } else {
               pa_threaded_mainloop_lock (mpThreadedMainloop);
               /* Timeout machanism will be added to detect no response of pulseaudio */
               while (mContextState != PA_CONTEXT_READY) {
                  if ((mContextState == PA_CONTEXT_FAILED) ||
                      (mContextState == PA_CONTEXT_TERMINATED)) {
                     break;
                  }
                  pa_threaded_mainloop_wait (mpThreadedMainloop);
               }

               pa_threaded_mainloop_unlock (mpThreadedMainloop);
               pa_context_set_state_callback (mpContext, NULL, NULL);

               mbCtxConnected = (mContextState == PA_CONTEXT_READY);
               if (!mbCtxConnected) {
                  if ((mContextState == PA_CONTEXT_TERMINATED) ||
                      (mContextState == PA_CONTEXT_FAILED)) {
                     pa_threaded_mainloop_lock (mpThreadedMainloop);
                     pa_context_disconnect (mpContext);
                     pa_threaded_mainloop_unlock (mpThreadedMainloop);
                  }

                  mbCtxConnected = false;
                  pa_threaded_mainloop_stop (mpThreadedMainloop);

                  ret = -1;
                  INFO ("pa_conext_connect failed: %u!\n", mContextState);
               }
            }
         }
      } else {
         ret = -1;
         INFO ("Failed to new threaded mainloop!\n");
      }
   } else {
      INFO ("mpThreadedMainloop already created!\n");
   }

   if ((mpSinkVolume = new pa_cvolume) != NULL) {
      if ((mpSourceVolume = new pa_cvolume) == NULL) {
         delete mpSinkVolume;
         mpSinkVolume = NULL;

         ret = -1;
         INFO ("Failed to create an instance of pa_cvolume!");
      }
   } else {
      ret = -1;
      INFO ("Failed to create an instance of pa_cvolume!");
   }

   return ret;
}

int AmAudioDevice::audio_device_volume_get (AmAudioDeviceType device, int *volume)
{
   int ret = 0;
   AmAudioVolumeInfo volume_info;

   if (!volume) {
      ERROR ("Invalid argument: null pointer! ");
      return -1;
   }

   if (get_audio_volume_info (device, &volume_info) != 0) {
      ERROR ("Failed to get volume of corresponding audio device");
      ret = -1;
   } else {
      *volume = volume_info.volume;
   }

   return ret;
}

int AmAudioDevice::audio_device_volume_set (AmAudioDeviceType device, int volume)
{
   AmAudioVolumeInfo volume_info;
   AmAudioDeviceOperation op;

   if (volume < 0 || volume > 100) {
      ERROR ("Invalid argument: volume = %d", volume);
      return -1;
   }

   volume_info.volume = volume;
   op = AM_AUDIO_DEVICE_OPERATION_VOLUME_SET;
   return set_audio_volume_info (device, &volume_info, op);
}

int AmAudioDevice::audio_device_volume_mute (AmAudioDeviceType device)
{
   AmAudioVolumeInfo volume_info;
   AmAudioDeviceOperation op;

   op = AM_AUDIO_DEVICE_OPERATION_VOLUME_MUTE;
   return set_audio_volume_info (device, &volume_info, op);
}

int AmAudioDevice::audio_device_volume_unmute (AmAudioDeviceType device)
{
   AmAudioVolumeInfo volume_info;
   AmAudioDeviceOperation op;

   op = AM_AUDIO_DEVICE_OPERATION_VOLUME_UNMUTE;
   return set_audio_volume_info (device, &volume_info, op);
}

int AmAudioDevice::audio_device_is_mute (AmAudioDeviceType device, bool *mute)
{
   int ret = 0;
   AmAudioVolumeInfo volume_info;

   if (get_audio_volume_info (device, &volume_info) != 0) {
      ERROR ("Failed to get volume info of corresponding audio device");
      ret = -1;
   } else {
      *mute = (volume_info.mute == 1);
   }

   return ret;
}

void AmAudioDevice::static_pa_state (pa_context *context, void *data)
{
   ((UserData *)data)->adev->pa_state (context,((UserData *)data)->data);
}

void AmAudioDevice::pa_state (pa_context *context, void *data)
{
   if (context) {
      mContextState = pa_context_get_state (context);
   }

   pa_threaded_mainloop_signal ((pa_threaded_mainloop *)data, 0);
   DEBUG ("pa_state is called!");
}

void AmAudioDevice::static_get_sink_info (pa_context  *c,
                                   const pa_sink_info *i,
                                   int                 eol,
                                   void               *user_data)
{
   ((UserData *)user_data)->adev->get_sink_info (i);
   pa_threaded_mainloop_signal ((pa_threaded_mainloop *)(((UserData *)user_data)->data), 0);
   DEBUG ("static_get_sink_info is called!");
}

void AmAudioDevice::get_sink_info (const pa_sink_info *sink)
{
   int i;

   if (sink != NULL) {
      mSinkIndex  = sink->index;
      mbSinkMuted = (sink->mute > 0);

      mpSinkVolume->channels = sink->volume.channels;
      for (i = 0; i < sink->volume.channels; i++) {
         mpSinkVolume->values[i] = sink->volume.values[i];
      }

      /*
       * As name, description, monitor_source_name and driver are declared as const
       * in prototype of pa_sink_info, coping operation is not allowed. So, don't
       * use these members in other place because parameter i will be destoried in
       * pulse audio library and contents of those varibles is uncertain.
       */
      DEBUG ("sink->name: %s", sink->name);
      DEBUG ("sink->description: %s", sink->description);
      DEBUG ("sink->monitor_source_name: %s", sink->monitor_source_name);
      DEBUG ("sink->driver: %s", sink->driver);
   }
}

void AmAudioDevice::static_get_source_info (pa_context *c,
                                  const pa_source_info *i,
                                  int                   eol,
                                  void                 *user_data)
{
   ((UserData *)user_data)->adev->get_source_info (i);
   pa_threaded_mainloop_signal ((pa_threaded_mainloop *)(((UserData *)user_data)->data), 0);
   NOTICE ("static_get_source_info is called!");
}

void AmAudioDevice::get_source_info (const pa_source_info *source)
{
   int i;

   if (source != NULL) {
      mSourceIndex  = source->index;
      mbSourceMuted = (source->mute > 0);

      mpSourceVolume->channels = source->volume.channels;
      for (i = 0; i < source->volume.channels; i++) {
         mpSourceVolume->values[i] = source->volume.values[i];
      }

      /*
       * As name, description, monitor_of_sink_name and driver are declared as const
       * in prototype of pa_sink_info, coping operation is not allowed. So, don't
       * use these members in other place because parameter i will be destoried in
       * pulse audio library and contents of those varibles is uncertain.
       */
      DEBUG ("source->name: %s", source->name);
      DEBUG ("source->description: %s", source->description);
      DEBUG ("source->monitor_of_sink_name: %s", source->monitor_of_sink_name);
      DEBUG ("source->driver: %s", source->driver);
   }
}

int AmAudioDevice::get_audio_volume_info (AmAudioDeviceType device,
                                          AmAudioVolumeInfo *volumeInfo)
{
   int ret = 0;
   bool has_get_volume_info = true;
   pa_operation *pa_op = NULL;
   pa_operation_state_t op_state;
   UserData userData (this, mpThreadedMainloop);

   if (!mpSourceVolume || !mpSinkVolume) {
      INFO ("audio_device_init should be called before!\n");
      return -1;
   }

   pa_threaded_mainloop_lock (mpThreadedMainloop);
   if (device == AM_AUDIO_DEVICE_MIC) {
      pa_op = pa_context_get_source_info_list (mpContext,
                                               static_get_source_info,
                                               &userData);
   } else if (device == AM_AUDIO_DEVICE_SPEAKER) {
      pa_op = pa_context_get_sink_info_list (mpContext,
                                             static_get_sink_info,
                                             &userData);
   } else {
      INFO ("No such audio device: device = %d", (int)device);
      return -1;
   }

   while ((op_state = pa_operation_get_state (pa_op)) != PA_OPERATION_DONE) {
      if (op_state == PA_OPERATION_CANCELLED) {
         INFO ("Get source info operation cancelled!\n");
         has_get_volume_info = false;
         break;
      }
      pa_threaded_mainloop_wait (mpThreadedMainloop);
   }

   pa_operation_unref (pa_op);
   pa_threaded_mainloop_unlock (mpThreadedMainloop);

   if (has_get_volume_info) {
      if (device == AM_AUDIO_DEVICE_MIC) {
         volumeInfo->volume = pa_cvolume_avg (mpSourceVolume) * 100 / PA_VOLUME_NORM;
         volumeInfo->mute = (mbSourceMuted) ? 1 : 0;
      } else {
         volumeInfo->volume = pa_cvolume_avg (mpSinkVolume) * 100 / PA_VOLUME_NORM;
         volumeInfo->mute = (mbSinkMuted) ? 1 : 0;
      }
   } else {
      INFO ("Failed to get volume info!\n");
      ret = -1;
   }

   return ret;
}

int AmAudioDevice::set_audio_volume_info (AmAudioDeviceType device,
                                          AmAudioVolumeInfo *volumeInfo,
                                          AmAudioDeviceOperation op)
{
   int i, ret = 0;
   AmAudioVolumeInfo temp;
   pa_operation *pa_op = NULL;

   if (!mpSourceVolume || !mpSinkVolume) {
      INFO ("audio_device_init should be called before!\n");
      return -1;
   }

   /*
    * It is not sure that get_audio_volume_info for corresponding
    * audio device has been called, we need to call it to get
    * index of corresponding audio device.
    */
   if (get_audio_volume_info (device, &temp) < 0) {
      INFO ("Failed to get index of audio device.");
      return -1;
   }

   pa_threaded_mainloop_lock (mpThreadedMainloop);
   switch (op) {
   case AM_AUDIO_DEVICE_OPERATION_VOLUME_SET: {
      if (device == AM_AUDIO_DEVICE_MIC) {
         for (i = 0; i < mpSourceVolume->channels; i++) {
            mpSourceVolume->values[i] = volumeInfo->volume * PA_VOLUME_NORM / 100;
         }

         pa_op = pa_context_set_source_volume_by_index (mpContext,
                                                        mSourceIndex,
                                                        mpSourceVolume,
                                                        NULL,
                                                        NULL);
      } else if (device == AM_AUDIO_DEVICE_SPEAKER) {
         for (i = 0; i < mpSinkVolume->channels; i++) {
            mpSinkVolume->values[i] = volumeInfo->volume * PA_VOLUME_NORM / 100;
         }

         pa_op = pa_context_set_sink_volume_by_index (mpContext,
                                                      mSinkIndex,
                                                      mpSinkVolume,
                                                      NULL,
                                                      NULL);
      } else {
         INFO ("No such audio device: device = %d", (int)device);
         return -1;
      }
   } break;

   case AM_AUDIO_DEVICE_OPERATION_VOLUME_MUTE: {
      if (device == AM_AUDIO_DEVICE_MIC) {
         pa_op = pa_context_set_source_mute_by_index (mpContext,
                                                      mSourceIndex,
                                                      1,
                                                      NULL,
                                                      NULL);
      } else if (device == AM_AUDIO_DEVICE_SPEAKER) {
         pa_op = pa_context_set_sink_mute_by_index (mpContext,
                                                    mSinkIndex,
                                                    1,
                                                    NULL,
                                                    NULL);
      } else {
         INFO ("No such audio device: device = %d", (int)device);
         return -1;
      }
   } break;

   case AM_AUDIO_DEVICE_OPERATION_VOLUME_UNMUTE: {
      if (device == AM_AUDIO_DEVICE_MIC) {
         pa_op = pa_context_set_source_mute_by_index (mpContext,
                                                      mSourceIndex,
                                                      0,
                                                      NULL,
                                                      NULL);
      } else if (device == AM_AUDIO_DEVICE_SPEAKER) {
         pa_op = pa_context_set_sink_mute_by_index (mpContext,
                                                    mSinkIndex,
                                                    0,
                                                    NULL,
                                                    NULL);
      } else {
         INFO ("No such audio device: device = %d", (int)device);
         return -1;
      }
   } break;

   default: {
      ERROR ("No such operation in set_audio_volume_info!");
   } break;

   }

   if (!pa_op) {
      ret = -1;
      WARN ("set_audio_volume_info failed for some reason!");
   }

   pa_operation_unref (pa_op);
   pa_threaded_mainloop_unlock (mpThreadedMainloop);

   return ret;
}

