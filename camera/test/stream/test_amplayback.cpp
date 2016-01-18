/*******************************************************************************
 * test_amplayback.cpp
 *
 * History:
 *   2013-4-8 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_stream.h"

AmPlaybackStream *g_amplayback;
bool              g_isrunning;
int               ctrl[2];

#define CTRL_READ  ctrl[0]
#define CTRL_WRITE ctrl[1]

static void show_menu()
{
  printf("\n================ Test AmPlayback===============\n");
  printf("  r -- start/resume playing\n");
  printf("  p -- pause\n");
  printf("  s -- stop\n");
  printf("  q -- Quit the program\n");
  printf("\n===============================================\n\n");
}

static void app_engine_callback(AmPlaybackStream::AmMsg *msg)
{
  if (AM_LIKELY(msg)) {
    switch(msg->msg) {
      case AmPlaybackStream::AM_PLAYBACK_STREAM_MSG_START_OK:
        NOTICE("Playback engine started successfully!");
        break;
      case AmPlaybackStream::AM_PLAYBACK_STREAM_MSG_PAUSE_OK:
        NOTICE("Playback engine paused successfully!");
        break;
      case AmPlaybackStream::AM_PLAYBACK_STREAM_MSG_EOS:
        NOTICE("Playback finished successfully!");
        break;
      case AmPlaybackStream::AM_PLAYBACK_STREAM_MSG_ABORT:
        NOTICE("Playback engine abort!");
        break;
      case AmPlaybackStream::AM_PLAYBACK_STREAM_MSG_ERR:
        NOTICE("Playback engine abort due to fatal error occurred!");
        break;
      case AmPlaybackStream::AM_PLAYBACK_STREAM_MSG_NULL:
      default:break;
    }
    show_menu();
  }
}

static void sigstop(int sig_num)
{
  if (g_amplayback &&
      (g_amplayback->is_playing() || g_amplayback->is_paused())) {
    NOTICE("Quit!");
    g_amplayback->stop();
  }
  write(CTRL_WRITE, "e", 1);
}

static void mainloop(const char *file[], int count)
{
  fd_set allset;
  fd_set fdset;
  int maxfd = -1;

  FD_ZERO(&allset);
  FD_SET(STDIN_FILENO, &allset);
  FD_SET(CTRL_READ,    &allset);
  maxfd = (STDIN_FILENO > CTRL_READ) ? STDIN_FILENO : CTRL_READ;

  g_isrunning = true;
  show_menu();
  while(g_isrunning) {
    char ch = '\n';
    fdset = allset;
    if (AM_LIKELY(select(maxfd + 1, &fdset, NULL, NULL, NULL) > 0)) {
      if (AM_LIKELY(FD_ISSET(STDIN_FILENO, &fdset))) {
        if (AM_LIKELY(read(STDIN_FILENO, &ch, 1)) < 0) {
          PERROR("read");
          g_isrunning = false;
          continue;
        }
      } else if (AM_LIKELY(FD_ISSET(CTRL_READ, &fdset))) {
        char cmd[1] = {0};
        if (AM_LIKELY(read(CTRL_READ, cmd, sizeof(cmd))) < 0) {
          PERROR("read");
          g_isrunning = false;
          continue;
        } else if (cmd[0] == 'e') {
          NOTICE("Exit mainloop!");
          g_isrunning = false;
          continue;
        }
      }
    } else {
      if (AM_LIKELY(errno != EINTR)) {
        PERROR("select");
        g_isrunning = false;
      }
      continue;
    }

    switch(ch) {
      case 'R' :
      case 'r' : {
        if (g_amplayback->is_paused()) {
          if (!g_amplayback->pause(false)) {
            ERROR("Failed to resume!");
          }
        } else if (g_amplayback->is_playing()) {
          NOTICE("Playing is already started!");
        } else {
          bool enablePlay = false;
          for (int i = 0; i < count; ++ i) {
            if (!g_amplayback->add_uri(file[i])) {
              ERROR("Failed to add %s to playlist!", file[i]);
            } else {
              enablePlay = true;
            }
          }
          if (enablePlay && !g_amplayback->play()) {
            ERROR("Failed to start playing!");
          }
        }
      }break;
      case 'p' : {
        if (!g_amplayback->pause(true)) {
          ERROR("Failed to pause!");
        }
      }break;
      case 'q' :
      case 'Q' :
      case 'S' :
      case 's' : {
        if (!g_amplayback->stop()) {
          ERROR("Failed to stop playing!");
        }
        if ((ch == 'q') || (ch == 'Q')) {
          NOTICE("Quit!");
          g_isrunning = false;
        }
      }break;
      case '\n' :
        continue;
      default:
        break;
    }
  }
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    ERROR("Usage: test_amplayback mediafile1 [mediafile2 | ...]");
  } else {
    if (AM_UNLIKELY(pipe(ctrl) < 0)) {
      PERROR("pipe");
    } else {
      signal(SIGINT,  sigstop);
      signal(SIGQUIT, sigstop);
      signal(SIGTERM, sigstop);
      g_isrunning  = false;
      g_amplayback = new AmPlaybackStream();
      if (g_amplayback && g_amplayback->init_playback_stream()) {
        g_amplayback->set_app_msg_callback(app_engine_callback, NULL);
        mainloop((const char**)&argv[1], argc - 1);
      }
      delete g_amplayback;
      close(CTRL_READ);
      close(CTRL_WRITE);
    }
  }

  return 0;
}
