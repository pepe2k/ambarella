/*
 * am_audio_device.h
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
#ifndef __AM_AUDIO_DEVICE_H__
#define __AM_AUDIO_DEVICE_H__

#include <pulse/pulseaudio.h>
#include <pthread.h>

enum AmAudioDeviceType {
   AM_AUDIO_DEVICE_MIC = 1,
   AM_AUDIO_DEVICE_SPEAKER = 2,
};

enum AmAudioDeviceOperation {
   AM_AUDIO_DEVICE_OPERATION_NONE          = 0,
   AM_AUDIO_DEVICE_OPERATION_VOLUME_GET    = 1,
   AM_AUDIO_DEVICE_OPERATION_VOLUME_SET    = 2,
   AM_AUDIO_DEVICE_OPERATION_VOLUME_MUTE   = 3,
   AM_AUDIO_DEVICE_OPERATION_VOLUME_UNMUTE = 4,
};

struct AmAudioVolumeInfo;

class AmMutex {
public:
   AmMutex (bool bRecursive = false)
   {
      if (bRecursive) {
         pthread_mutexattr_t attr;
         ::pthread_mutexattr_init (&attr);
         ::pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
         ::pthread_mutex_init (&mutex, &attr);
      } else {
         ::pthread_mutex_init (&mutex, NULL);
      }
   }

   void Lock ()
   {
      pthread_mutex_lock (&mutex);
   }

   void Unlock ()
   {
      pthread_mutex_unlock (&mutex);
   }

   bool Trylock ()
   {
      return (0 == pthread_mutex_trylock (&mutex));
   }

private:
   pthread_mutex_t mutex;
};

class AmAudioDevice {
public:
   AmAudioDevice ();
   virtual ~AmAudioDevice ();

public:
   static AmAudioDevice *get_instance ();

public:
   int audio_device_init ();
   int audio_device_volume_get (AmAudioDeviceType device, int *volume);
   int audio_device_volume_set (AmAudioDeviceType device, int volume);
   int audio_device_volume_mute (AmAudioDeviceType device);
   int audio_device_volume_unmute (AmAudioDeviceType device);
   int audio_device_is_mute (AmAudioDeviceType device, bool *mute);

private:
   void pa_state (pa_context *context, void *data);
   void get_source_info (const pa_source_info *source);
   void get_sink_info (const pa_sink_info *sink);
   int get_audio_volume_info (AmAudioDeviceType device, AmAudioVolumeInfo *volumeInfo);
   int set_audio_volume_info (AmAudioDeviceType device, AmAudioVolumeInfo *volumeInfo,
                              AmAudioDeviceOperation op);

private:
   static void static_pa_state (pa_context *context, void *data);
   static void static_get_source_info (pa_context *c, const pa_source_info *i, int eol, void *user_data);
   static void static_get_sink_info (pa_context *c, const pa_sink_info *i, int eol, void *user_data);

private:
   bool                  mbCtxConnected;
   bool                  mbSinkMuted;
   bool                  mbSourceMuted;
   int                   mSinkIndex;
   int                   mSourceIndex;
   pa_context_state      mContextState;
   pa_cvolume           *mpSinkVolume;
   pa_cvolume           *mpSourceVolume;
   pa_context           *mpContext;
   pa_threaded_mainloop *mpThreadedMainloop;

private:
   static AmAudioDevice *instance;
   static AmMutex mMutex;
};

#endif
