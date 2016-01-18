/*
 * audio_alert.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 24/01/2013 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_include.h"
#include "am_utility.h"
#include <asm/byteorder.h>
#include "am_audioalert.h"
#include "am_event.h"

static inline snd_pcm_format_t pa_fmt_to_pcm_fmt(pa_sample_format_t format)
{
  snd_pcm_format_t fmt = SND_PCM_FORMAT_UNKNOWN;
  switch(format) {
    case PA_SAMPLE_U8:
      fmt = SND_PCM_FORMAT_U8;
      break;
    case PA_SAMPLE_ALAW:
      fmt = SND_PCM_FORMAT_A_LAW;
      break;
    case PA_SAMPLE_ULAW:
      fmt = SND_PCM_FORMAT_MU_LAW;
      break;
    case PA_SAMPLE_S16LE:
      fmt = SND_PCM_FORMAT_S16_LE;
      break;
    case PA_SAMPLE_S16BE:
      fmt = SND_PCM_FORMAT_S16_BE;
      break;
    case PA_SAMPLE_FLOAT32LE:
      fmt = SND_PCM_FORMAT_FLOAT_LE;
      break;
    case PA_SAMPLE_FLOAT32BE:
      fmt = SND_PCM_FORMAT_FLOAT_BE;
      break;
    case PA_SAMPLE_S32LE:
      fmt = SND_PCM_FORMAT_S32_LE;
      break;
    case PA_SAMPLE_S32BE:
      fmt = SND_PCM_FORMAT_S32_BE;
      break;
    case PA_SAMPLE_S24LE:
      fmt = SND_PCM_FORMAT_S24_3LE;
      break;
    case PA_SAMPLE_S24BE:
      fmt = SND_PCM_FORMAT_S24_3BE;
      break;
    case PA_SAMPLE_S24_32LE:
      fmt = SND_PCM_FORMAT_S24_LE;
      break;
    case PA_SAMPLE_S24_32BE:
      fmt = SND_PCM_FORMAT_S24_BE;
      break;
    case PA_SAMPLE_INVALID:
    default:
      fmt = SND_PCM_FORMAT_UNKNOWN;
      break;
  }

  return fmt;
}

AmAudioAlert *AmAudioAlert::Create (snd_pcm_format_t audioFormat,
                                  AmAudioSensitivity audioSen)
{
   AmAudioAlert *result = new AmAudioAlert (audioFormat, audioSen);
   if (result != NULL && !result->Construct ()) {
      ERROR ("Failed to create an instance of AmAudioAlert");
      delete result;
      result = NULL;
   }

   return result;
}

AmAudioAlert::AmAudioAlert (snd_pcm_format_t audioFormat,
                          AmAudioSensitivity audioSen):
   mAudioSen (audioSen),
   mAlertDirection (0),
   mVolumeSum (0LL),
   mSampleNum (0),
   mBitsPerSample (0),
   mMaximumPeak (0),
   mMicroHistoryHead (0),
   mMacroHistoryHead (0),
   mMicroHistoryTail (0),
   mMacroHistoryTail (0),
   mpCallback (NULL),
   mAudioFormat (audioFormat)
{}

AmAudioAlert::~AmAudioAlert ()
{}

bool AmAudioAlert::Construct ()
{
   int i;

   /*
    * Initialize mMicroHistory array, each element is
    * assigned to minimum volume.
    */
   for (i = 0; i < REFERENCE_COUNT + 1; i++) {
      mMicroHistory[i] = 0;
      mMacroHistory[i] = 0;
   }

   mBitsPerSample = snd_pcm_format_physical_width (mAudioFormat);
   mMaximumPeak = (1 << (mBitsPerSample - 1)) - 1;
   return (mBitsPerSample > 0);
}

bool AmAudioAlert::SetAudioAlertSensitivity (int audioSen)
{
   bool ret = true;

   if (audioSen == 1) {
      mAudioSen = AM_AUDIO_SENSITIVITY_LOWE;
   } else if (audioSen == 2) {
      mAudioSen = AM_AUDIO_SENSITIVITY_LOW;
   } else if (audioSen == 3) {
      mAudioSen = AM_AUDIO_SENSITIVITY_MID;
   } else if (audioSen == 4) {
      mAudioSen = AM_AUDIO_SENSITIVITY_HIG;
   } else if (audioSen == 5) {
      mAudioSen = AM_AUDIO_SENSITIVITY_HIGE;
   } else {
      ret = false;
      ERROR ("Audio sensitivity should be 1, 2, 3, 4 or 5!");
   }

   if (audioSen >= 1 && audioSen <=5) {
      INFO ("Audio alert sensitivity is set to %d", audioSen);
   }

   return ret;
}

int AmAudioAlert::GetAudioAlertSensitivity ()
{
   return mAudioSen;
}

int AmAudioAlert::GetAudioAlertDirection ()
{
   return mAlertDirection;
}

snd_pcm_format_t AmAudioAlert::GetAudioFormat ()
{
   return mAudioFormat;
}

void AmAudioAlert::SetAudioAlertCallback (void(*callback)())
{
   mpCallback = callback;
}

void AmAudioAlert::SetAudioAlertDirection (int alertDirection)
{
   mAlertDirection = alertDirection;
}

void AmAudioAlert::SetAudioFormat(pa_sample_format_t format)
{
  mAudioFormat = pa_fmt_to_pcm_fmt(format);
}

bool AmAudioAlert::IsSilent (int percent)
{
   bool ret = true;

   if (percent < 0 || percent >= 100) {
      NOTICE ("Invalid argument, percent [0 - 100]: %d", percent);
      return false;
   }

   for (int i = 0; i < REFERENCE_COUNT + 1; i++) {
      if (mMacroHistory[i] * 100 >=
            mMaximumPeak * percent * MACRO_AVERAGE_SAMPLES) {
         ret = false;
         break;
      }
   }

   return ret;
}

void AmAudioAlert::StoreMacroHistoryData (long long historyData)
{
   mMacroHistory[mMacroHistoryHead] = historyData;
   if (mMacroHistoryHead == mMacroHistoryTail) {
      mMacroHistoryTail = (mMacroHistoryTail + 1) % (REFERENCE_COUNT + 1);
   }

   mMacroHistoryHead = (mMacroHistoryHead + 1) % (REFERENCE_COUNT + 1);
}

bool AmAudioAlert::StoreMicroHistoryData (int historyData)
{
   bool ret = true;
   historyData = historyData * 100 / mMaximumPeak;

   mMicroHistory[mMicroHistoryHead] = historyData;
   if (mMicroHistoryHead == mMicroHistoryTail) {
      mMicroHistoryTail = (mMicroHistoryTail + 1) % (REFERENCE_COUNT + 1);
   }

   mMicroHistoryHead = (mMicroHistoryHead + 1) % (REFERENCE_COUNT + 1);

   if (mAlertDirection) {
      if (historyData < (int)mAudioSen ||
          (mMicroHistory[mMicroHistoryTail] >= (int)mAudioSen)) {
         ret = false;
      } else {
         for (int i = 1; i < REFERENCE_COUNT; i++) {
            if (mMicroHistory[(mMicroHistoryTail + i) % (REFERENCE_COUNT + 1)] < (int)mAudioSen) {
               ret = false;
               break;
            }
         }
      }
   } else {
      if (historyData >= (int)mAudioSen ||
          (mMicroHistory[mMicroHistoryTail] < (int)mAudioSen)) {
         ret = false;
      } else {
         for (int i = 1; i < REFERENCE_COUNT; i++) {
            if (mMicroHistory[(mMicroHistoryTail + i) % (REFERENCE_COUNT + 1)] >= (int)mAudioSen) {
               ret = false;
               break;
            }
         }
      }
   }

   if (ret) {
      NOTICE ("historyData = %d", historyData);
      for (int i = 0; i < REFERENCE_COUNT + 1; i++)
         DEBUG ("historyData[%d] = %d", i,
              mMicroHistory[(mMicroHistoryTail + i) % (REFERENCE_COUNT + 1)]);
   }

   return ret;
}

void AmAudioAlert::AudioAlertDetect (void *pcmData,
      int len, uint64_t pts, AmAudioDetect *instance)
{
   int val, counter, count, averagePeak, sum;
   int format = snd_pcm_format_little_endian (mAudioFormat);
   audio_detect_msg_t msg;
   static int seq_num = 0;

   counter = sum = 0;
   averagePeak = 0;
   count = len;

   switch (mBitsPerSample) {
     case 8: {
       signed char *ptr = (signed char *)pcmData;
       signed char mask = snd_pcm_format_silence (mAudioFormat);

       /* Computer average volume in current pcm data. */
       counter = 0;
       while (count-- > 0) {
         val = abs (*ptr++ ^ mask);
         sum += val;
         mVolumeSum += val;
         mSampleNum++;
         counter++;
         if (counter == MICRO_AVERAGE_SAMPLES) {
           averagePeak = sum / counter;
           if (StoreMicroHistoryData (averagePeak)) {
             memset (&msg, 0, sizeof (audio_detect_msg_t));

             msg.seq_num  = ++seq_num;
             msg.msg_type = AM_AUDIO_DETECT_ALARM;
             msg.msg_pts  = pts;

             pthread_mutex_lock (&instance->mRingBufferMutex);
             AudioDetectRingBufferWrite (instance->mpRingBuffer, &msg);
             pthread_cond_signal (&instance->mRingBufferCond);
             pthread_mutex_unlock (&instance->mRingBufferMutex);
             DEBUG ("Write an alert message: seq_num = %d", seq_num);
           }

           counter = sum = 0;
         }

         if (mSampleNum == MACRO_AVERAGE_SAMPLES) {
            StoreMacroHistoryData (mVolumeSum);
            mVolumeSum = 0;
         }
       }
     } break;

     case 16: {
       signed short *ptr = (signed short *)pcmData;
       signed short mask = snd_pcm_format_silence_16 (mAudioFormat);

       count >>= 1; /* One sample takes up two bytes */
       while (count-- > 0) {
         val = format ? __le16_to_cpu(*ptr++) :
                        __be16_to_cpu(*ptr++) ;

         if (val & (1 << (mBitsPerSample - 1))) {
           val |= 0xffff << 16;
         }

         val = abs (val) ^ mask;
         sum += val;
         mVolumeSum += val;
         mSampleNum++;
         counter++;
         if (counter % MICRO_AVERAGE_SAMPLES == 0) {
           averagePeak = sum / counter;
           if (StoreMicroHistoryData (averagePeak)) {
             memset (&msg, 0, sizeof (audio_detect_msg_t));

             msg.seq_num  = ++seq_num;
             msg.msg_type = AM_AUDIO_DETECT_ALARM;
             msg.msg_pts  = pts;

             pthread_mutex_lock (&instance->mRingBufferMutex);
             AudioDetectRingBufferWrite (instance->mpRingBuffer, &msg);
             pthread_cond_signal (&instance->mRingBufferCond);
             pthread_mutex_unlock (&instance->mRingBufferMutex);
             DEBUG ("Write an alert message: seq_num = %d", seq_num);
           }

           counter = sum = 0;
         }

         if (mSampleNum == MACRO_AVERAGE_SAMPLES) {
            StoreMacroHistoryData (mVolumeSum);
            mVolumeSum = 0;
         }
       }
     } break;

     case 24: {
       signed char  *ptr = (signed char *)pcmData;
       int mask = snd_pcm_format_silence_32 (mAudioFormat);

       count /= 3; /* One sample takes up three bytes. */
       while (count-- > 0) {
         val = format ? (ptr[0] | (ptr[1] << 8) | (ptr[2] << 16)) :
                        (ptr[2] | (ptr[1] << 8) | (ptr[0] << 16));

         /* Correct signed bit in 32-bit value */
         if (val & (1 << (mBitsPerSample - 1))) {
           val |= 0xff << 24;
         }

         val = abs (val) ^ mask;
         sum += val;
         mVolumeSum += val;
         mSampleNum++;
         counter++;
         if (counter % MICRO_AVERAGE_SAMPLES == 0) {
           averagePeak = sum / counter;
           if (StoreMicroHistoryData (averagePeak)) {
             memset (&msg, 0, sizeof (audio_detect_msg_t));

             msg.seq_num  = ++seq_num;
             msg.msg_type = AM_AUDIO_DETECT_ALARM;
             msg.msg_pts  = pts;

             pthread_mutex_lock (&instance->mRingBufferMutex);
             AudioDetectRingBufferWrite (instance->mpRingBuffer, &msg);
             pthread_cond_signal (&instance->mRingBufferCond);
             pthread_mutex_unlock (&instance->mRingBufferMutex);
             DEBUG ("Write an alert message: seq_num = %d", seq_num);
           }

           counter = sum = 0;
         }

         if (mSampleNum == MACRO_AVERAGE_SAMPLES) {
            StoreMacroHistoryData (mVolumeSum);
            mVolumeSum = 0;
         }

         ptr += 3;
       }
     } break;

     case 32: {
       int *ptr = (int *)pcmData;
       int mask = snd_pcm_format_silence_32 (mAudioFormat);

       count >>= 2; /* One sample takes up four bytes. */
       while (count-- > 0) {
         val = abs(format ? __le32_to_cpu (*ptr++) :
                            __be32_to_cpu (*ptr++)) ^ mask;
         sum += val;
         mVolumeSum += val;
         mSampleNum++;
         counter++;
         if (counter % MICRO_AVERAGE_SAMPLES == 0) {
           averagePeak = sum / counter;
           if (StoreMicroHistoryData (averagePeak)) {
             memset (&msg, 0, sizeof (audio_detect_msg_t));

             msg.seq_num  = ++seq_num;
             msg.msg_type = AM_AUDIO_DETECT_ALARM;
             msg.msg_pts  = pts;

             pthread_mutex_lock (&instance->mRingBufferMutex);
             AudioDetectRingBufferWrite (instance->mpRingBuffer, &msg);
             pthread_cond_signal (&instance->mRingBufferCond);
             pthread_mutex_unlock (&instance->mRingBufferMutex);
             DEBUG ("Write an alert message: seq_num = %d", seq_num);
           }

           counter = sum = 0;
         }

         if (mSampleNum == MACRO_AVERAGE_SAMPLES) {
            StoreMacroHistoryData (mVolumeSum);
            mVolumeSum = 0;
         }
       }
     } break;

     default:
       ERROR ("No such sample");
       break;
   }
}
