/*
 * audio_detect.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 23/12/2013 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, oi transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_include.h"
#include "am_utility.h"
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_event.h"
#include "am_media_info.h"
#include "audio_codec_info.h"
#include "am_audioalert.h"
#include "am_audioanalysis.h"

#define CONTEXT_NAME ((const char*)"AudioDetect")
#define STREAM_NAME  ((const char*)"AudioDetectStream")
#define HW_TIMER     ((const char*)"/proc/ambarella/ambarella_hwtimer")

AmAudioDetect *AmAudioDetect::mpAudioDetectInstance = NULL;

int module_start ()
{
   int ret = 0;
   AmAudioDetect *instance = NULL;

   if ((instance = AmAudioDetect::GetInstance ()) == NULL) {
      ERROR ("Failed to get an instance of AmAudioDetect!");
      ret = -1;
   } else {
      if (instance->AudioDetectStart () < 0) {
         ERROR ("Failed to start audio detect module!");
         ret = -1;
      }
   }

   return ret;
}

int module_stop ()
{
   int ret = 0;
   AmAudioDetect *instance = NULL;

   if ((instance = AmAudioDetect::GetInstance ()) == NULL) {
      ERROR ("Failed to get an instance of AmAudioDetect!");
      ret = -1;
   } else {
      instance->AudioDetectStop ();
   }

   return ret;
}

int module_destroy ()
{
   int ret = 0;
   AmAudioDetect *instance = NULL;

   if ((instance = AmAudioDetect::GetInstance ()) == NULL) {
      ERROR ("Failed to get an instance of AmAudioDetect!");
      ret = -1;
   } else {
      instance->AudioDetectFini ();
      delete instance;
   }

   return ret;
}

int module_get_config (MODULE_CONFIG *config)
{
   int ret = 0;
   AmAudioDetect *instance = NULL;

   if (!config) {
      ERROR ("Invalid argument: null pointer!");
      return -1;
   }

   if ((instance = AmAudioDetect::GetInstance ()) == NULL) {
      ERROR ("Failed to get an instance of AmAudioDetect!");
      ret = -1;
   } else {
      instance->GetAudioDetectProperty (config);
   }

   return ret;
}

MODULE_ID module_get_id(void)
{
   return AUDIO_DECT;
}

int module_set_config (MODULE_CONFIG *config)
{
   int ret = 0;
   int dynamic_change_support = 1;
   AmAudioDetect *instance = NULL;

   do {
      if (!config || !config->key) {
         ERROR ("Invalid argument: Null pointer");
         ret = -1;
         break;
      }

      if ((instance = AmAudioDetect::GetInstance ()) == NULL) {
         ERROR ("Failed to get an instance of AmAudioDetect!");
         ret = -1;
         break;
      }

      if (!strcmp (config->key, "AudioChannelNumber")) {
         if (instance->mbAudioDetectRun) {
            dynamic_change_support = 0;
         } else {
            instance->SetAudioChannel (*((uint32_t *)config->value));
         }
      } else if (!strcmp (config->key, "AudioSampleRate")) {
         if (instance->mbAudioDetectRun) {
            dynamic_change_support = 0;
         } else {
            instance->SetAudioSampleRate (*((uint32_t *)config->value));
         }
      } else if (!strcmp (config->key, "AudioChunkBytes")) {
         if (instance->mbAudioDetectRun) {
            dynamic_change_support = 0;
         } else {
            instance->SetAudioChunkBytes (*((uint32_t *)config->value));
         }
      } else if (!strcmp (config->key, "EnableAlertDetect")) {
         if (*((int *)config->value) > 0) {
            instance->mbEnableAlertDetect = true;
         } else {
            instance->mbEnableAlertDetect = false;
         }
      } else if (!strcmp (config->key, "AudioAlertDetectCallback")) {
         instance->mAudioDetectCallback[(uint32_t)AM_AUDIO_DETECT_ALARM \
                              - 1] = (audio_detect_callback)config->value;
      } else if (!strcmp (config->key, "AudioAlertSensitivity")) {
         if (!instance->mpAudioAlert->SetAudioAlertSensitivity (
                  *((int *)config->value))) {
            ERROR ("Failed to set sensitivity for audio alert module!");
            ret = -1;
            break;
         }
      } else if (!strcmp (config->key, "AudioAlertDirection")) {
         instance->mpAudioAlert->SetAudioAlertDirection (*((int *)config->value));
      } else if (!strcmp (config->key, "EnableAnalysisDetect")) {
         if (*((int *)config->value) > 0) {
            instance->mbEnableAnalysisDetect = true;
         } else {
            instance->mbEnableAnalysisDetect = false;
         }
      } else if (!strcmp (config->key, "AudioAnalysisDirection")) {
          instance->mpAudioAnalysis->SetAudioAnalysisDirection (*((int *)config->value));
      } else if (!strcmp (config->key, "AudioAnalysisMod")) {
          instance->mpAudioAnalysis->SetAudioAnalysisMod(
                  (audio_analy_param *)config->value);
      } else if (!strcmp (config->key, "AudioAnalysisDetectCallback")) {
              instance->mAudioDetectCallback[(uint32_t)AM_AUDIO_DETECT_ANALY \
                              - 1] = (audio_detect_callback)config->value;
      } else {
          ERROR ("No such config in audio detect module"
                   " needs to be configured: %s", config->key);
          ret = -1;
          break;
      }

      if (!dynamic_change_support) {
         NOTICE ("Audio detect module is running now and this property can not be changed dynamiclly!");
         NOTICE ("You need to decide what value should be assigned to this property before module start!");
         ret = 1;
      }
   }while (0);

   return ret;
}

AmAudioDetect::AmAudioDetect () :
   mpRingBuffer (NULL),
   mpAudioAlert (NULL),
   mpAudioAnalysis (NULL),
   mbEnableAlertDetect (false),
   mbEnableAnalysisDetect (false),
   mbAudioDetectRun (false),
   mpPaThreadedMainloop (NULL),
   mpPaMainloopApi (NULL),
   mpPaContext (NULL),
   mpPaStreamRecord (NULL),
   mpReadData (NULL),
   mpOverflow (NULL),
   mpAudioBuffer (NULL),
   mpAudioRdPtr (NULL),
   mpAudioWtPtr (NULL),
   mpDefSrcName (NULL),
   mpMutex (NULL),
   mPaContextState(PA_CONTEXT_UNCONNECTED),
   mAudioSampleRate (48000),
   mAudioChannel (2),
   mAudioCodecType (AM_AUDIO_CODEC_PCM),
   mAudioChunkBytes (2048),
   mAudioBufSize (0),
   mFrameBytes(0),
   mLastPts (0),
   mFragmentPts (0),
   mHwTimerFd (-1),
   mbCtxConnected (false),
   mHandlerThreadId(0)
{
   memset(&mPaSampleSpec, 0, sizeof(mPaSampleSpec));
   memset(&mPaChannelMap, 0, sizeof(mPaChannelMap));
   for (int i = 0; i < AUDIO_DETECT_TYPE_NUM; i++) {
      mAudioDetectCallback[i] = NULL;
   }
}

AmAudioDetect::~AmAudioDetect ()
{
   AudioDetectFini ();

   delete mpReadData;
   delete mpOverflow;
   delete mpAudioAnalysis;
   delete mpAudioAlert;
   AM_DELETE (mpMutex);

   if (mpRingBuffer) {
      AudioDetectRingBufferFree (mpRingBuffer);
      free (mpRingBuffer);
   }
}

AmAudioDetect *AmAudioDetect::GetInstance ()
{
   if (!mpAudioDetectInstance) {
      mpAudioDetectInstance = new AmAudioDetect ();
      if (mpAudioDetectInstance &&
         (mpAudioDetectInstance->Construct () < 0)) {
         ERROR ("Failed to initialize an instance of AmAudioDetect");
         delete mpAudioDetectInstance;
         mpAudioDetectInstance = NULL;
      }
   }

   return mpAudioDetectInstance;
}

int AmAudioDetect::Construct ()
{
   pthread_cond_init (&mRingBufferCond, NULL);
   pthread_mutex_init (&mRingBufferMutex, NULL);

   if ((mpMutex = CMutex::Create ()) == NULL) {
      ERROR ("Failed to create an instance of CMutex!");
      return -1;
   }

   if ((mpReadData = new PaData (this, NULL)) == NULL) {
      ERROR ("Failed to initialize mpReadData!");
      return -1;
   }

   if ((mpOverflow = new PaData (this, NULL)) == NULL) {
      ERROR ("Failed to initialize mpOverflow!");
      return -1;
   }

   if ((mpRingBuffer = (AudioDetectRingBuffer *)malloc (
                 sizeof (AudioDetectRingBuffer))) == NULL) {
      ERROR ("Failed to allocate memory for mpRingBuffer");
      return -1;
   }

   memset (mpRingBuffer, 0, sizeof (AudioDetectRingBuffer));
   AudioDetectRingBufferInit (mpRingBuffer, AUDIO_DETECT_RINGBUFFER_LEN);

   if (!mpRingBuffer->elems) {
      ERROR ("Failed to allocate memory for mpRingBuffer->elems");
      return -1;
   }

   if ((mpAudioAlert = AmAudioAlert::Create (SND_PCM_FORMAT_S16_LE,
               AM_AUDIO_SENSITIVITY_DEAF)) == NULL) {
      ERROR ("Failed to create an instance of AmAudioAlert");
      return -1;
   }

   if ((mpAudioAnalysis = AmAudioAnalysis::Create (SND_PCM_FORMAT_S16_LE)) == NULL) {
      ERROR ("Failed to create an instance of AmAudioAnalysis");
      return -1;
   }

   /*
    * Create a appropriative thread to handle event message
    * from audio alert, analysis detect and so on.
    */
   if (pthread_create (&mHandlerThreadId, NULL,
            &AmAudioDetect::AudioEventHandler, (void *)this) < 0) {
      ERROR ("Failed to create a thread for event handler!");
      return -1;
   }

   return 0;
}

void AmAudioDetect::CleanUp (void *arg)
{
   if (mpAudioDetectInstance != NULL) {
      pthread_mutex_unlock (&mpAudioDetectInstance->mRingBufferMutex);
   }
}

int AmAudioDetect::GetAudioDetectProperty (MODULE_CONFIG *config)
{
   int ret = 0;

   if (!strcmp (config->key, "AudioChannelNumber")) {
      *((int *)config->value) = mAudioChannel;
      INFO ("mAudioChannel = %d\n", mAudioChannel);
   } else if (!strcmp (config->key, "AudioSampleRate")) {
      *((int *)config->value) = mAudioSampleRate;
      INFO ("mAudioSampleRate = %d\n", mAudioSampleRate);
   } else if (!strcmp (config->key, "AudioChunkBytes")) {
      *((int *)config->value) = mAudioChunkBytes;
      INFO ("mAudioChunkBytes = %d\n", mAudioChunkBytes);
   } else if (!strcmp (config->key, "EnableAlertDetect")) {
      *((int *)config->value) = mbEnableAlertDetect ? 1 : 0;
      INFO ("Enable or disable alert detect: ", *((int *)config->value));
   } else if (!strcmp (config->key, "AudioAlertSensitivity")) {
      *((int *)config->value) = mpAudioAlert->GetAudioAlertSensitivity ();
      INFO ("Audio alert sensitivity: ", *((int *)config->value));
   } else if (!strcmp (config->key, "AudioAlertDirection")) {
      *((int *)config->value) = mpAudioAlert->GetAudioAlertDirection ();
      INFO ("Audio alert direction: ", *((int *)config->value));
   } else if (!strcmp (config->key, "EnableAnalysisDetect")) {
      *((int *)config->value) = mbEnableAnalysisDetect ? 1 : 0;
      INFO ("Enable or disable analysis detect: ", *((int *)config->value));
   } else if (!strcmp (config->key, "AudioAnalysisDirection")) {
      *((int *)config->value) = mpAudioAnalysis->GetAudioAnalysisDirection ();
      INFO ("Audio analysis direction: ", *((int *)config->value));
   } else {
      WARN ("No such property with key: %s needs to be get", config->key);
      ret = -1;
   }

   return ret;
}

void *AmAudioDetect::AudioEventHandler (void *arg)
{
   audio_detect_msg_t  message;
   audio_detect_callback callback = NULL;
   AmAudioDetect *instance = (AmAudioDetect *)arg;
   pthread_detach (pthread_self ());

   do {
      /* make sure when thread is cancelled it will not hold lock. */
      pthread_cleanup_push (AmAudioDetect::CleanUp, NULL);

      pthread_mutex_lock (&instance->mRingBufferMutex);
      pthread_cond_wait (&instance->mRingBufferCond,
                         &instance->mRingBufferMutex);

      /* Read out all the messages contained in ring buffer. */
      while (!AudioDetectRingBufferIsEmpty (instance->mpRingBuffer)) {
         AudioDetectRingBufferRead (instance->mpRingBuffer, &message);
         if (instance->mbEnableAlertDetect &&
            message.msg_type == AM_AUDIO_DETECT_ALARM) {
            NOTICE ("Fetch an alert message, seq: %d", message.seq_num);
            callback = instance->mAudioDetectCallback[AM_AUDIO_DETECT_ALARM - 1];
         } else if (instance->mbEnableAnalysisDetect &&
                    message.msg_type == AM_AUDIO_DETECT_ANALY) {
            NOTICE ("Fetch a analysis message, seq: %d", message.seq_num);
            callback = instance->mAudioDetectCallback[AM_AUDIO_DETECT_ANALY - 1];
         } else {
            NOTICE ("No such message needs to be processed!");
            continue;
         }

         (*callback) (&message);
      }

      pthread_mutex_unlock (&instance->mRingBufferMutex);
      pthread_cleanup_pop (0);
   } while (instance->mbAudioDetectRun);

   return NULL;
}


void AmAudioDetect::SetAudioChannel (uint32_t channel)
{
   INFO ("Audio channel in audio detect module is set to %d", channel);
   mAudioChannel = channel;
}

void AmAudioDetect::SetAudioSampleRate (uint32_t sampleRate)
{
   INFO ("Audio sample rate in audio detect module is set to %d", sampleRate);
   mAudioSampleRate = sampleRate;
}

void AmAudioDetect::SetAudioChunkBytes (uint32_t chunkBytes)
{
   INFO ("Audio chunk bytes in audio detect module is set to %d", chunkBytes);
   mAudioChunkBytes = chunkBytes;
}

int AmAudioDetect::AudioDetectInit ()
{
   int ret = 0;
   AUTO_LOCK (mpMutex);

   if (mHwTimerFd < 0) {
      if ((mHwTimerFd = open (HW_TIMER, O_RDONLY)) < 0) {
         ERROR ("Failed to open %s: %s", HW_TIMER, strerror (errno));
         return -1;
      }
   }

   if (mpAudioAnalysis->InitAudioAnalysis() < 0) {
       ERROR ("Failed to init Audio Analysis!\n");
       return -1;
   }

   if (mpPaThreadedMainloop == NULL) {
      if ((mpPaThreadedMainloop = pa_threaded_mainloop_new ()) != NULL) {
         PaData userData (this, mpPaThreadedMainloop);
         mpPaMainloopApi = pa_threaded_mainloop_get_api (mpPaThreadedMainloop);
         mpPaContext = pa_context_new (mpPaMainloopApi, CONTEXT_NAME);

         if (mpPaContext == NULL) {
            ret = -1;
            INFO ("Failed to new context!\n");
         } else {
            pa_context_set_state_callback (mpPaContext, StaticPaState, &userData);
            pa_context_connect (mpPaContext, NULL, PA_CONTEXT_NOFLAGS, NULL);

            if (pa_threaded_mainloop_start (mpPaThreadedMainloop) != 0) {
               ret = -1;
               INFO ("Failed to start threaded mainloop!\n");
            } else {
               pa_threaded_mainloop_lock (mpPaThreadedMainloop);
               /* Timeout machanism will be added to detect no response of pulseaudio */
               while (mPaContextState != PA_CONTEXT_READY) {
                  if ((mPaContextState == PA_CONTEXT_FAILED) ||
                      (mPaContextState == PA_CONTEXT_TERMINATED)) {
                     break;
                  }
                  pa_threaded_mainloop_wait (mpPaThreadedMainloop);
               }

               pa_threaded_mainloop_unlock (mpPaThreadedMainloop);
               pa_context_set_state_callback (mpPaContext, NULL, NULL);

               mbCtxConnected = (mPaContextState == PA_CONTEXT_READY);
               if (!mbCtxConnected) {
                  if ((mPaContextState == PA_CONTEXT_TERMINATED) ||
                      (mPaContextState == PA_CONTEXT_FAILED)) {
                     pa_threaded_mainloop_lock (mpPaThreadedMainloop);
                     pa_context_disconnect (mpPaContext);
                     pa_threaded_mainloop_unlock (mpPaThreadedMainloop);
                  }

                  mbCtxConnected = false;
                  pa_threaded_mainloop_stop (mpPaThreadedMainloop);

                  ret = -1;
                  INFO ("pa_conext_connect failed: %u!\n", mPaContextState);
               }
            }
         }
      } else {
         ret = -1;
         INFO ("Failed to new threaded mainloop!\n");
      }
   } else {
      INFO ("mpPaThreadedMainloop already created!\n");
   }

   return ret;
}

int AmAudioDetect::AudioDetectStart ()
{
   int ret = 0;
   AUTO_LOCK (mpMutex);

   if (mbAudioDetectRun) {
      NOTICE ("Audio detect is already running!");
      return ret;
   }

   if (AudioDetectInit () < 0) {
      ERROR ("Failed to initialize audio detect!");
      ret = -1;
   } else {
      pa_operation *paOp = NULL;
      pa_operation_state_t opState;
      PaData servInfo (this, mpPaThreadedMainloop);

      pa_threaded_mainloop_lock (mpPaThreadedMainloop);
      paOp = pa_context_get_server_info (mpPaContext,
                                         StaticPaServerInfo,
                                         &servInfo);

      while ((opState = pa_operation_get_state (paOp)) != PA_OPERATION_DONE) {
         if (opState == PA_OPERATION_CANCELLED) {
            WARN ("Operation for fetching server info is cancelled!");
            break;
         }

         pa_threaded_mainloop_wait (mpPaThreadedMainloop);
      }

      pa_operation_unref (paOp);
      pa_threaded_mainloop_unlock (mpPaThreadedMainloop);

      if (opState == PA_OPERATION_DONE) {
         pa_buffer_attr bufAttr = { (uint32_t) -1};
         mpPaStreamRecord = pa_stream_new (mpPaContext,
                                           "AudioDetect",
                                           &mPaSampleSpec,
                                           &mPaChannelMap);

         if (mpPaStreamRecord) {
             bufAttr.fragsize = mAudioChunkBytes * 5;
             mpReadData->data = mpPaThreadedMainloop;
             mpOverflow->data = mpPaThreadedMainloop;

             pa_stream_set_read_callback (mpPaStreamRecord,
                                          StaticPaRead,
                                          mpReadData);
             pa_stream_set_overflow_callback (mpPaStreamRecord,
                                              StaticPaOverflow,
                                              mpOverflow);

             if (pa_stream_connect_record (mpPaStreamRecord,
                                           mpDefSrcName,
                                           &bufAttr,
                                           (pa_stream_flags) \
                                           (PA_STREAM_INTERPOLATE_TIMING |
                                            PA_STREAM_ADJUST_LATENCY |
                                            PA_STREAM_AUTO_TIMING_UPDATE)) < 0) {
                ERROR ("Failed to connect with record stream!");
                ret = -1;
             } else {
                pa_stream_state_t streamState;
                while ((streamState = pa_stream_get_state (
                            mpPaStreamRecord)) != PA_STREAM_READY) {
                   if (streamState == PA_STREAM_FAILED ||
                       streamState == PA_STREAM_TERMINATED) {
                      ret = -1; break;
                   }
                }

                if (streamState == PA_STREAM_READY) {
                   const pa_buffer_attr *attr =
                         pa_stream_get_buffer_attr (mpPaStreamRecord);

                   if (attr) {
                      INFO ("Client requested fragment size: %u", bufAttr.fragsize);
                      INFO ("Server returned  fragment size: %u", attr->fragsize);
                   } else {
                      ret = -1;
                      ERROR ("Failed to get buffer's attribute!");
                   }
                } else {
                   ret = -1;
                   ERROR ("Failed to connect record stream to audio server!");
                }
             }
         } else {
            ERROR ("Failed to create record stream: %s!",
                  pa_strerror (pa_context_errno (mpPaContext)));
            ret = -1;
         }
      } else {
         ERROR ("Failed to get server info!");
         ret = -1;
      }
   }

   mbAudioDetectRun = (ret == 0);
   return ret;
}

void AmAudioDetect::AudioDetectStop ()
{
   AUTO_LOCK (mpMutex);

   if (mbAudioDetectRun) {
      mbAudioDetectRun = false;
      pa_threaded_mainloop_stop (mpPaThreadedMainloop);
      AudioDetectFini ();
   } else {
      NOTICE ("Audio detect is already stoppped!");
   }

}

void AmAudioDetect::PaState (pa_context *context, void *data)
{
  if (context) {
    mPaContextState = pa_context_get_state (context);
  }

  pa_threaded_mainloop_signal ((pa_threaded_mainloop*)data, 0);
  DEBUG ("PaState called!");
}

void AmAudioDetect::PaServerInfo (pa_context *context,
      const pa_server_info *info,
      void *data)
{
   INFO ("Audio Server Information:");
   INFO ("       Server Version: %s",   info->server_version);
   INFO ("          Server Name: %s",   info->server_name);
   INFO ("  Default Source Name: %s",   info->default_source_name);
   INFO ("    Default Sink Name: %s",   info->default_sink_name);
   INFO ("            Host Name: %s",   info->host_name);
   INFO ("            User Name: %s",   info->user_name);
   INFO ("             Channels: %hhu", info->sample_spec.channels);
   INFO ("                 Rate: %u",   info->sample_spec.rate);
   INFO ("           Frame Size: %u",   pa_frame_size(&info->sample_spec));
   INFO ("          Sample Size: %u",   pa_sample_size(&info->sample_spec));
   INFO ("  ChannelMap Channels: %hhu", info->channel_map.channels);

   memcpy(&mPaSampleSpec, &info->sample_spec, sizeof(info->sample_spec));
   memcpy(&mPaChannelMap, &info->channel_map, sizeof(info->channel_map));
   mPaSampleSpec.rate = mAudioSampleRate;
   mPaSampleSpec.channels = mAudioChannel;

   switch (mPaSampleSpec.channels) {
   case 1 :
      pa_channel_map_init_mono (&mPaChannelMap);
      break;
   case 2 :
      pa_channel_map_init_stereo (&mPaChannelMap);
      break;
   default:
      pa_channel_map_init_auto (&mPaChannelMap,
            mPaSampleSpec.channels,
            PA_CHANNEL_MAP_ALSA);
      break;
   }

   INFO (" Client Configuration:");
   INFO ("             Channels: %hhu", mPaSampleSpec.channels);
   INFO ("                 Rate: %u",   mPaSampleSpec.rate);
   INFO ("  ChannelMap Channels: %hhu", mPaChannelMap.channels);

   mpDefSrcName   = amstrdup (info->default_source_name);
   mFrameBytes   = pa_frame_size (&mPaSampleSpec) * mPaSampleSpec.rate;

   if (mAudioChunkBytes > 0) {
      mAudioBufSize = GetLcm (mAudioChunkBytes, mFrameBytes);
      mpAudioBuffer = new uint8_t[mAudioBufSize];
      mFragmentPts  = (90000 * mAudioChunkBytes) / mFrameBytes;
      INFO("         Fragment PTS: %llu", mFragmentPts);

      if (mbEnableAlertDetect) {
         mpAudioAlert->SetAudioFormat(info->sample_spec.format);
      }

      if (mpAudioBuffer == NULL) {
         ERROR ("Failed to allocate audio buffer!");
      } else {
         mpAudioRdPtr = mpAudioBuffer;
         mpAudioWtPtr = mpAudioBuffer;
         INFO ("Allocated %u bytes audio buffer, this will buffer %u seconds!",
               mAudioBufSize, mAudioBufSize / mFrameBytes);
      }
   } else {
      ERROR ("Invalid audio chunk size %u bytes", mAudioChunkBytes);
   }

   pa_threaded_mainloop_signal ((pa_threaded_mainloop*)data, 0);
}

void AmAudioDetect::PaRead (pa_stream *stream, size_t bytes, void *userData)
{
   const void *data = NULL;
   uint32_t availDataSize = 0;
   uint64_t currPts = GetCurrentPTS ();

   if (pa_stream_peek (stream, &data, &bytes) < 0) {
      ERROR("pa_stream_peek() failed: %s",
            pa_strerror (pa_context_errno (mpPaContext)));
   } else if (data && (bytes > 0)) {
      memcpy (mpAudioWtPtr, data, bytes);
      mpAudioWtPtr = mpAudioBuffer +
         (((mpAudioWtPtr - mpAudioBuffer) + bytes) % mAudioBufSize);
   }

   pa_stream_drop (stream);
   availDataSize = GetAvailDataSize ();
   mLastPts = ((mLastPts == 0) ?
         currPts - ((availDataSize * mFragmentPts) / mAudioChunkBytes) :
         mLastPts);

   if (availDataSize >= mAudioChunkBytes) {
      uint64_t realPtsIncr = currPts - mLastPts;
      uint64_t currPtsSeg = ((mAudioChunkBytes * realPtsIncr) / availDataSize);
      uint32_t pktNumber = availDataSize / mAudioChunkBytes;

      for (uint32_t i = 0; i < pktNumber; ++ i) {
         mLastPts += currPtsSeg;

         /* Adding audio detect functions here. */
         if (mbEnableAlertDetect) {
            mpAudioAlert->AudioAlertDetect (mpAudioRdPtr,
                                              mAudioChunkBytes,
                                              mLastPts,
                                              this);
         }

         if (mbEnableAnalysisDetect) {
            mpAudioAnalysis->AmAudioAnalysisDetect (mpAudioRdPtr,
                                              mAudioChunkBytes,
                                              mLastPts,
                                              this);
         }

         mpAudioRdPtr = mpAudioBuffer +
            (((mpAudioRdPtr - mpAudioBuffer) + mAudioChunkBytes) % mAudioBufSize);
      }
   }
}

void AmAudioDetect::AudioDetectFini ()
{
   if (mpPaStreamRecord) {
      pa_stream_disconnect (mpPaStreamRecord);
      DEBUG ("pa_stream_disconnect (mpPaStreamRecord)");
      pa_stream_unref (mpPaStreamRecord);
      mpPaStreamRecord = NULL;
      DEBUG ("pa_stream_unref (mpPaStreamRecord)");
   }

   if (mbCtxConnected) {
      pa_context_set_state_callback (mpPaContext, NULL, NULL);
      pa_context_disconnect (mpPaContext);
      mbCtxConnected = false;
      DEBUG ("pa_context_disconnect\n");
   }

   if (mpPaContext) {
      pa_context_unref (mpPaContext);
      mpPaContext = NULL;
      DEBUG ("pa_context_unref (mpPaContext)\n");
   }

   if (mpPaThreadedMainloop) {
      pa_threaded_mainloop_free (mpPaThreadedMainloop);
      mpPaThreadedMainloop = NULL;
      DEBUG ("pa_threaded_mainloop_free (mpThreadedMainloop)\n");
   }

   if (mHwTimerFd > 0) {
      close (mHwTimerFd);
      mHwTimerFd = -1;
   }

   if (mpDefSrcName) {
      delete[] mpDefSrcName;
      mpDefSrcName = NULL;
   }

   if (mpAudioBuffer) {
      delete[] mpAudioBuffer;
      mpAudioRdPtr = NULL;
   }

   if (mpAudioAnalysis) {
       mpAudioAnalysis->StopAudioAnalysis();
   }

   mpAudioWtPtr = NULL;
   mpAudioBuffer = NULL;
   mbAudioDetectRun = false;

   INFO ("AudioDetect's resource has been released!");
}

void AmAudioDetect::PaOverflow (pa_stream *stream, void *data)
{
   ERROR ("Data Overflow!");
}

void AmAudioDetect::StaticPaState (pa_context *context, void *data)
{
  ((PaData*)data)->adev->PaState (context, ((PaData*)data)->data);
}

void AmAudioDetect::StaticPaServerInfo (pa_context *context,
                                        const pa_server_info *info,
                                        void  *data)
{
  ((PaData*)data)->adev->PaServerInfo (context, info, ((PaData*)data)->data);
}

void AmAudioDetect::StaticPaRead (pa_stream *stream,
                                       size_t bytes,
                                       void *data)
{
  ((PaData*)data)->adev->PaRead (stream, bytes, ((PaData*)data)->data);
}

void AmAudioDetect::StaticPaOverflow (pa_stream *stream,
                                   void *data)
{
  ((PaData*)data)->adev->PaOverflow (stream, ((PaData*)data)->data);
}

inline uint64_t AmAudioDetect::GetCurrentPTS ()
{
   uint8_t pts[32] = {0};
   uint64_t currPts = mLastPts;

   if (mHwTimerFd >= 0) {
      if (read(mHwTimerFd, pts, sizeof(pts)) < 0) {
         PERROR("read");
      } else {
         currPts = strtoull((const char*)pts, (char **)NULL, 10);
      }
   }

   return currPts;
}

inline uint32_t AmAudioDetect::GetLcm (uint32_t a, uint32_t b)
{
   uint32_t c = a;
   uint32_t d = b;

   /* Compute the maximum divisor of a and b.*/
   while (((c > d) ? (c %= d) : (d %= c)));

   /* Return the least common multiple of a and b.*/
   return (a * b) / (c + d);
}

inline uint32_t AmAudioDetect::GetAvailDataSize()
{
  return (uint32_t)((mpAudioWtPtr >= mpAudioRdPtr) ? (mpAudioWtPtr - mpAudioRdPtr) :
      (mAudioBufSize + mpAudioWtPtr - mpAudioRdPtr));
}

