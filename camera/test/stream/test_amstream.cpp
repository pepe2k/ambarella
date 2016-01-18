/*******************************************************************************
 * test_amstream.cpp
 *
 * Histroy:
 *  2012-3-26 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <getopt.h>
#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_vdevice.h"
#include "am_network.h"
#include "am_audioalert.h"
#include "am_stream.h"
#include "am_configure.h"
#include "am_watchdog.h"
#include "am_wsdiscovery.h"
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
#include "frame_stat.h"
#endif

#define MAX_FILE_NAME_LEN 256
#define MAX_STREAM_NUMBER 4
#define DATE_FMT_LEN 128

#define SIMPLE_CAM_VIN_CONFIG BUILD_AMBARELLA_CAMERA_CONF_DIR"/video_vin.conf"

AmRecordStream *g_amstream;
AmSimpleCam    *g_cam;
char            filename[MAX_FILE_NAME_LEN];
bool            run;
int             ctrl[2];
int             raw_fd[MAX_STREAM_NUMBER + 1] = {-1, -1, -1, -1, -1};

#define CTRL_READ  ctrl[0]
#define CTRL_WRITE ctrl[1]

#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
static inline void DumpRcStatisticInfo(FrameRcStat *rcStat)
{
  DEBUG("\nFrame Statistics:"
        "\n     Session: 0x%08x"
        "\nInstant kbps: %u"
        "\n   Stream ID: %hu"
        "\n  I Frame QP: %hhu"
        "\n  P Frame QP: %hhu"
        "\n  B Frame QP: %hhu",
        rcStat->session,
        rcStat->kbps_instant,
        rcStat->streamid,
        rcStat->qp_i,
        rcStat->qp_p,
        rcStat->qp_b);
}

static void frame_statistics_callback(void *data)
{
  FrameRcStat *rcStat = (FrameRcStat*)data;
  if (AM_LIKELY(rcStat)) {
    DumpRcStatisticInfo(rcStat);
  }
}
#endif

static void app_engine_callback(AmRecordStream::AmMsg *msg)
{
  if (msg) {
    switch(msg->msg) {
      case AmRecordStream::AM_RECORD_STREAM_MSG_EOS:
        NOTICE("Engine stopped successfully!");
        break;
      case AmRecordStream::AM_RECORD_STREAM_MSG_ABORT:
        NOTICE("Engine abort due to unrecoverable error occurred in muxer!");
        break;
      case AmRecordStream::AM_RECORD_STREAM_MSG_ERR:
        NOTICE("Engine abort due to fatal error in filters!");
        break;
      case AmRecordStream::AM_RECORD_STREAM_MSG_OVFL:
        NOTICE("Engine stopped due to bad network condition!");
        break;
      case AmRecordStream::AM_RECORD_STREAM_MSG_NULL:
      default:
        break;
    }
  }
}

static void show_menu()
{
  printf("\n================ Test AmStream ================\n");
  printf("  r -- start recording\n");
  printf("  R -- reload configuration, then start recording\n");
  printf("  e -- send event\n");
  printf("  s -- stop\n");
  printf("\n================ Misc Settings ================\n");
  printf("  q -- Quit the program\n");
  printf("\n===============================================\n\n");
}

static bool load_config(AmConfig*       config,
                        AmRecordStream* stream,
                        AmSimpleCam*    simplecam,
                        bool            needApply)
{
  bool ret = false;
  if (AM_LIKELY(needApply && config && stream)) {
    if (AM_LIKELY(config->load_vdev_config())) {
      VDeviceParameters *vdevConfig = config->vdevice_config();
      for (uint32_t i = 0; i < vdevConfig->vin_number; ++ i) {
        simplecam->set_vin_config(config->vin_config(i), i);
      }
      for (uint32_t i = 0; i < vdevConfig->vout_number; ++ i) {
        simplecam->set_vout_config(config->vout_config(i), i);
      }
      simplecam->set_encoder_config(config->encoder_config());

      for (uint32_t i = 0; i < vdevConfig->stream_number; ++ i) {
        simplecam->set_stream_config(config->stream_config(i), i);
      }
      if (config->load_record_config()) {
        if (stream->init_record_stream(config->record_config())) {
          stream->set_app_msg_callback(
              (AmRecordStream::AmRecordStreamAppCb)app_engine_callback, NULL);
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
          stream->set_frame_statistics_callback(frame_statistics_callback);
#endif
          ret = true;
        } else {
          ERROR("Failed to initialize record engine!");
        }
      } else {
        ERROR("Failed to load record configuration!");
      }
    } else {
      ERROR("Failed to load vdev config!");
    }
  } else if (!config) {
    ERROR("Invalid configuration!");
  } else if (!stream) {
    ERROR("Invalid stream!");
  } else {
    ret = true;
  }

  return ret;
}

static void RunMainLoop(AmConfig* config)
{
  AmNetworkConfig    *nc            = new AmNetworkConfig();
  NetDeviceInfo      *netDev        = NULL;
  WSDiscoveryService *wsDiscoverSrv = NULL;
  int                         maxfd = -1;
  bool                     firstRun = true;
  fd_set allset;
  fd_set fdset;

  FD_ZERO(&allset);
  FD_SET(STDIN_FILENO, &allset);
  FD_SET(CTRL_READ,    &allset);
  maxfd = (STDIN_FILENO > CTRL_READ) ? STDIN_FILENO : CTRL_READ;

  run = true;
  show_menu();
  while (run) {
    char ch = '\n';
    fdset = allset;
    if (AM_LIKELY(select(maxfd + 1, &fdset, NULL, NULL, NULL) > 0)) {
      if (AM_LIKELY(FD_ISSET(STDIN_FILENO, &fdset))) {
        if (AM_LIKELY(read(STDIN_FILENO, &ch, 1)) < 0) {
          PERROR("read");
          run = false;
          continue;
        }
      } else if (AM_LIKELY(FD_ISSET(CTRL_READ, &fdset))) {
        char cmd[1] = {0};
        if (AM_LIKELY(read(CTRL_READ, cmd, sizeof(cmd))) < 0) {
          PERROR("read");
          run = false;
          continue;
        } else if (cmd[0] == 'e') {
          NOTICE("Exit mainloop!");
          run = false;
          continue;
        }
      }
    } else {
      if (AM_LIKELY(errno != EINTR)) {
        PERROR("select");
        run = false;
      }
      continue;
    }
    switch(ch) {
      case 'R':
        firstRun = true;
        /* no breaks */
      case 'r': {
        if (g_amstream->is_recording()) {
          NOTICE("Recording is already started!");
        } else if (!load_config(config, g_amstream, g_cam, firstRun) ||
                   !g_cam->ready_for_encode() ||
                   !g_amstream->record_start()) {
          ERROR("Failed to start recording!");
        } else {
          if (nc && nc->get_default_connection(&netDev)) {
            wsDiscoverSrv = WSDiscoveryService::GetInstance();
            if (wsDiscoverSrv) {
              wsDiscoverSrv->start(netDev->netdev_name);
            }
          }
        }
        firstRun = false;
      }break;
      case 'e': {
        if (g_amstream->is_recording()) {
          g_amstream->send_usr_event(AmRecordStream::AM_EVENT_TYPE_EMG);
        }
      }break;
      case 's':
      case 'S':
      case 'q':
      case 'Q':
        if (g_amstream->is_recording()) {
          g_amstream->record_stop();
          g_cam->goto_idle();
        } /* else if (stream.is_playing()) {
          stream.play_stop();
          cam.goto_idle();
        }*/
        if (wsDiscoverSrv) {
          wsDiscoverSrv->Delete();
          wsDiscoverSrv = NULL;
        }
        delete netDev;
        netDev = NULL;
        if ((ch != 's') && (ch != 'S')) {
          NOTICE("Quit!");
          delete nc;
          nc = NULL;
          run = false;
        }
        break;
      case '\n':
        continue;
      default:
        break;
    }
    show_menu();
  }

  for (int i = 0; i < 5; i++) {
     if (raw_fd[i] != -1) {
       close (raw_fd[i]);
     }
  }
}

//-------------------------------------------------------------------
//                      Param
//-------------------------------------------------------------------
#define NO_ARG                0
#define HAS_ARG               1
#define SYSTEM_OPTIONS_BASE   0
#define ENCODING_OPTIONS_BASE 130

enum numeric_short_options {
  // System
  SYSTEM_IDLE = SYSTEM_OPTIONS_BASE,

  // Encoding
  SPECIFY_FORCE_IDR = ENCODING_OPTIONS_BASE,
  NO_PREVIEW,

  // H264
  SPECIFY_GOP_IDR,
  SPECIFY_GOP_MODEL,
  BITRATE_CONTROL,
  SPECIFY_CAVLC,

  // misc
  HELP,
};

static struct option long_options[] =
{
 {"help",  NO_ARG,  0, HELP},
 {0, 0, 0, 0}
};


static const char *short_options = "";

struct hint_s {
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] =
{
 {"", "\t\t\t" "print the most commonly used option usage"},
};

void usage(void)
{
  uint32_t i;

  printf("\nUsage: test_amstream [OPTION]...\n\n");
  for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
    if (isalpha(long_options[i].val)) {
      printf("-%c, ", long_options[i].val);
    } else {
      printf("    ");
    }
    printf("--%s", long_options[i].name);
    if (hint[i].arg[0] != 0) {
      printf(" [%s]", hint[i].arg);
    }
    printf("\t%s\n", hint[i].str);
  }
  printf("\n");
}

int init_param(int argc, char **argv)
{
  int ch;
  int option_index = 0;
  opterr = 0;
  while ((ch = getopt_long(argc, argv,
                           short_options,
                           long_options,
                           &option_index)) != -1) {
    switch (ch) {
      case HELP:
        usage();
        return -1;
        break;
      default:
        printf("unknown option found: %d\n", ch);
        return -1;
    }
  }
  return 0;
}

static void sigstop(int sig_num)
{
  NOTICE("Quit!");
  if (g_amstream && g_cam && g_amstream->is_recording()) {
    g_amstream->record_stop();
    g_cam->goto_idle();
  }
  write(CTRL_WRITE, "e", 1);
}

int main(int argc, char **argv)
{
  signal(SIGINT,  sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  if (init_param(argc, argv) < 0) {
    return -1;
  }

  if (AM_UNLIKELY(pipe(ctrl) < 0)) {
    PERROR("pipe");
    return -1;
  }

  AmConfig *config = new AmConfig();
  if (config) {
    //If you have different config, please set it here
    config->set_vin_config_path     (SIMPLE_CAM_VIN_CONFIG);
    //config.set_vout_config_path    (vout_config_path);
    //config.set_vdevice_config_path (vdevice_config_path);
    //config.set_record_config_path  (record_config_path);
    if (config->load_vdev_config()) {
      VDeviceParameters *vdevConfig = config->vdevice_config();
      if (AM_LIKELY(vdevConfig)) {
        g_cam      = new AmSimpleCam(vdevConfig);
        g_amstream = new AmRecordStream();
        if (AM_LIKELY(g_cam && g_amstream)) {
          RunMainLoop(config); /* Block here */
          delete g_cam;
          delete g_amstream;
        } else if (!g_cam) {
          ERROR("Failed to create camera object!");
        } else {
          ERROR("Failed to create record stream!");
        }
      } else {
        ERROR("Failed to get video device configurations!");
      }
    } else {
      ERROR("Failed to load configurations!");
    }
    delete config;
  } else {
    ERROR("Out of memory!");
  }

  close(CTRL_READ);
  close(CTRL_WRITE);

  return 0;
}
