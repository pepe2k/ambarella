/*
 * test_adevice.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 28/06/2013 [Created]
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
#include "am_adevice.h"

/*
 * This test program just checks whether APIs declared
 * in audio deivce works normally. Before running this
 * program, user needs to ensure that there is sound
 * in speaker device, so features in audio device, such
 * as increasing or decreasing volume, can be tested.
 *
 * User can play an audio media using test_amplayback.
 */
AmAudioDevice *adev;
bool           g_isrunning;
int            volume_before_mute;

static void
show_menu ()
{
   printf ("\n======== Test AmAudioDevice ========\n");
   printf ("  d -- decrease volume\n");
   printf ("  i -- increase volume\n");
   printf ("  f -- fetch current volume\n");
   printf ("  m -- mute\n");
   printf ("  r -- restore\n");
   printf ("  q -- quit\n");
   printf ("\n====================================\n\n");
}

static void
mainloop ()
{
   int cur_volume;
   int new_volume;
   g_isrunning = true;

   while (g_isrunning) {
      show_menu();
      switch (getchar ()) {
      case 'D':
      case 'd': {
         /*
          * When user want to test the feature of decreasing volume, current
          * volume of speaker device needs to read. And then decrease volume
          * of speaker will be decreased by 5%. If current volume is equal
          * to or less than 5%, volume will be set to 0.
          */
         if (adev->audio_device_volume_get (AM_AUDIO_DEVICE_SPEAKER, &cur_volume) < 0) {
            ERROR ("Failed to get volume of speaker!");
            g_isrunning = false;
         } else {
            if (cur_volume > 5) {
               new_volume = cur_volume - 5;
            } else {
               new_volume = 0;
            }

            if (adev->audio_device_volume_set (AM_AUDIO_DEVICE_SPEAKER, new_volume) < 0) {
               PERROR ("Failed to decrease volume of speaker!");
               g_isrunning = false;
            }
         }
      } break;

      case 'I':
      case 'i': {
         /* Like decreasing volume, speaker's volume will be increased by 5%. */
         if (adev->audio_device_volume_get (AM_AUDIO_DEVICE_SPEAKER, &cur_volume) < 0) {
            ERROR ("Failed to get volume of speaker!");
            g_isrunning = false;
         } else {
            if (cur_volume < 95) {
               new_volume = cur_volume + 5;
            } else {
               new_volume = 100;
            }

            if (adev->audio_device_volume_set (AM_AUDIO_DEVICE_SPEAKER, new_volume) < 0) {
               ERROR ("Failed to decrease volume of speaker!");
               g_isrunning = false;
            }
         }
      } break;

      case 'F':
      case 'f': {
          if (adev->audio_device_volume_get (AM_AUDIO_DEVICE_SPEAKER, &cur_volume) < 0) {
            ERROR ("Failed to get volume of speaker!");
            g_isrunning = false;
         } else {
            NOTICE ("current volume of speaker: %d\n", cur_volume);
         }
      } break;

      case 'M':
      case 'm': {
         if (adev->audio_device_volume_get (AM_AUDIO_DEVICE_SPEAKER, &cur_volume) < 0) {
            ERROR ("Failed to get volume of speaker!");
            g_isrunning = false;
         } else {
            volume_before_mute = cur_volume;
         }

         if (adev->audio_device_volume_mute (AM_AUDIO_DEVICE_SPEAKER) < 0) {
            ERROR ("Failed to decrease volume of speaker!");
            g_isrunning = false;
         }
      } break;

      case 'R':
      case 'r': {
         if (adev->audio_device_volume_unmute (AM_AUDIO_DEVICE_SPEAKER) < 0) {
            PERROR ("Failed to decrease volume of speaker!");
            g_isrunning = false;
         }
      } break;

      case 'Q':
      case 'q': {
         g_isrunning = false;
      } break;

      case '\n':
         continue;

      default:
         break;
      }
   }
}


int
main (int argc, char **argv)
{
   if ((adev = new AmAudioDevice ()) == NULL) {
      ERROR ("Failed to create an instance of AmAudioDevice!");
      return -1;
   }

   NOTICE ("Before audio_device_init");
   if (adev->audio_device_init () < 0) {
      ERROR ("Failed to init audio device!");
      delete adev;
      return -1;
   }

   mainloop ();
   delete adev;
}
