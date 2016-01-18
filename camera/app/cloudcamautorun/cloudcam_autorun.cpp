/*******************************************************************************
 * auto_set_cloudcam.cpp
 *
 * History:
 *   2012-12-25 - [Shupeng Ren] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_new.h"
#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_include.h"
#include "am_data.h"
#include "am_configure.h"
#include "am_utility.h"
#include "am_vdevice.h"
#include "am_audioalert.h"
#include "am_stream.h"
#include "am_qrcode.h"

#include <linux/input.h>
#include <sched.h>

//for gpio control
#define POWER_GPIO_ID 35
#define WIFI_GPIO_ID  12

#define LED_STATE_ON  0
#define LED_STATE_OFF 1

enum config_state {
  STATE_OK = 0,
  STATE_WAIT,
  STATE_BUSY,
  STATE_ERROR,
  STATE_RETRY
};

#define MAX_FILE_NAME_LEN 256

struct key_info {
    uint16_t input_key;
    char  name[32];
};

key_info INPUT_KEY_INFO[] = {
                             {KEY_WLAN,  "GPIOKEY_WLAN" },
                             {KEY_POWER, "GPIOKEY_POWER"},
};

#define INPUT_KEY_INFO_TABLE_SIZE \
    (sizeof(INPUT_KEY_INFO) / sizeof((INPUT_KEY_INFO)[0]))

const char *simple_camera_vin_config    = "/etc/camera/video_vin.conf";
const char *wifi_config_path            = "/etc/camera/wifi.conf";
const char *event_dev_path              = "/dev/input/event0";

int                  event_fd       = -1;
bool                 stop           = false;
bool                 is_stream_init = false;
bool                 is_wifi_setup  = false;
sigset_t             signal_mask;
FILE                *config_file    = NULL;
CThread             *led_flash      = NULL;
CThread             *signal_thread  = NULL;
AmQrcodeReader      *qrcode_reader         = NULL;
AmConfig            *config         = NULL;
AmRecordStream      *stream         = NULL;
AmSimpleCam         *simplecam      = NULL;
VDeviceParameters   *vdevConfig     = NULL;

config_state state = STATE_WAIT;

char auth_type[MAX_FILE_NAME_LEN] = {0};
char wifi_ssid[MAX_FILE_NAME_LEN] = {0};
char wifi_key[MAX_FILE_NAME_LEN]  = {0};

AM_ERR block_signals()
{
  if (sigemptyset(&signal_mask) != 0) {
    ERROR("Failed to initialize signal mask!");
    return ME_ERROR;
  }

  if (sigaddset(&signal_mask, SIGINT) != 0) {
    ERROR("Failed to add SIGINT to signal mask!");
    return ME_ERROR;
  }

  if (sigaddset(&signal_mask, SIGTERM) != 0) {
    ERROR("Failed to add SIGTERM to signal mask!");
    return ME_ERROR;
  }

  if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
    ERROR("Failed to block SIGINT and SIGTERM!");
    return ME_ERROR;
  }

  return ME_OK;
}

int deamon_init()
{
  pid_t pid, sid;

  pid = fork();
  if (pid < 0) {
    ERROR("fork: %s", strerror(errno));
    exit(EXIT_FAILURE);
  } else if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  if ((sid = setsid()) < 0) {
    ERROR("setid: %s", strerror(errno));
    return ME_ERROR;
  }

  if ((chdir("/")) < 0) {
    ERROR("chdir: %s", strerror(errno));
    return ME_ERROR;
  }

  umask(0);

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  return ME_OK;
}

AM_ERR do_export(int led_id, int state)
{
  int fd;
  char buf[MAX_FILE_NAME_LEN];
  char vbuf[4];

  if (state) {
    sprintf(buf, "/sys/class/gpio/export");
  } else {
    sprintf(buf, "/sys/class/gpio/unexport");
  }
  fd = open(buf, O_WRONLY);
  if (fd < 0) {
    ERROR("Open %s failed", buf);
    return ME_ERROR;
  }
  sprintf(vbuf, "%d", led_id);
  if (write(fd, vbuf, 4) != 4) {
    ERROR("Write %s failed", buf);
    close(fd);
    return ME_ERROR;
  }
  close(fd);
  return ME_OK;
}

int set_led_enable(int led_id, int enable)
{
  int ret = -1;

  if (led_id >= 0) {
    char gpio_addr[128] = { 0 };
    sprintf(gpio_addr, "/sys/devices/virtual/gpio/gpio%d", led_id);
    if (enable) {
      if (0 != access(gpio_addr, F_OK)) {
        ret = do_export(led_id, 1);
      }
    } else {
      if (0 == access(gpio_addr, F_OK)) {
        ret = do_export(led_id, 0);
      }
    }
  }

  return ret;
}

AM_ERR set_led_state(int led_id, int state)
{
  int fd;
  char buf[MAX_FILE_NAME_LEN];

  //set dir
  sprintf(buf, "/sys/class/gpio/gpio%d/direction", led_id);
  fd = open(buf, O_RDWR);
  if (fd < 0) {
    ERROR("Open gpio%d direction failed", led_id);
    return ME_ERROR;
  }
  if (write(fd, "out", 4) != 4) {
    close(fd);
    ERROR("Write gpio%d direction failed", led_id);
    return ME_ERROR;
  }
  close(fd);

  //set gpio output
  sprintf(buf, "/sys/class/gpio/gpio%d/value", led_id);
  fd = open(buf, O_RDWR);
  if (fd < 0) {
    ERROR("Open gpio%d value failed", led_id);
    return ME_ERROR;
  }
  sprintf(buf, "%d", state);
  if (write(fd, buf, 4) != 4) {
    ERROR("Write gpio%d value failed", led_id);
    return ME_ERROR;
  }
  close(fd);

  return ME_OK;
}

AM_ERR led_control_thread(void *p)
{
  unsigned int delay1 = 500000;
  unsigned int delay2 = 500000;
  while(!stop) {
    switch (state) {
      case STATE_OK:
        break;
      case STATE_WAIT:
        delay1 = 500000;
        delay2 = 500000;
        break;
      case STATE_BUSY:
        delay1 = 100000;
        delay2 = 100000;
        break;
      case STATE_ERROR:
        delay1 = 50000;
        delay2 = 500000;
        break;
      case STATE_RETRY:
        break;
      default:
        break;
    }
    set_led_state(WIFI_GPIO_ID, LED_STATE_ON);
    if (state == STATE_OK) {
      break;
    }
    usleep(delay1);
    set_led_state(WIFI_GPIO_ID, LED_STATE_OFF);
    usleep(delay2);
  }
  led_flash = NULL;
  return ME_OK;
}

AM_ERR setup_wifi()
{
  int status;
  char command[MAX_FILE_NAME_LEN];

  if (wifi_ssid[0] == 0) {
    ERROR("Fail to setup wifi");
    return ME_ERROR;
  }
  snprintf(command, sizeof(command),
           "nmcli dev wifi connect %s password %s",
           wifi_ssid, wifi_key);
  status = system(command);
  if (WIFEXITED(status)) {
    if (WEXITSTATUS(status)) {
      ERROR("Fail to setup wifi");
      return ME_ERROR;
    } else {
      is_wifi_setup = true;
      INFO("Setup wifi done!");
    }
  } else {
    ERROR("Fail to setup wifi");
    return ME_ERROR;
  }
  return ME_OK;
}

AM_ERR delete_wifi_setup()
{
  AM_INT status;
  char command[MAX_FILE_NAME_LEN];
  char wifi_config[MAX_FILE_NAME_LEN];

  if (!is_wifi_setup) {
    NOTICE("Wifi is not setuped!");
    return ME_OK;
  }
  snprintf(wifi_config, sizeof (wifi_config),
           "/etc/NetworkManager/system-connections/%s", wifi_ssid);

  if (access(wifi_config, F_OK) == 0) {
    snprintf(command, sizeof (command),
             "nmcli con delete id %s", wifi_ssid);

    status = system(command);
    if (WEXITSTATUS(status)) {
      ERROR("Failed to delete wifi connection");
      return ME_ERROR;
    }
    is_wifi_setup = false;
  }
  return ME_OK;
}

AM_ERR start_record()
{
  if (!stream->init_record_stream(config->record_config())) {
    ERROR("Failed to init record stream!");
    return ME_ERROR;
  }
  is_stream_init = true;

  if (stream->is_recording()) {
    NOTICE("Recording is already started!");
  } else if (!simplecam->ready_for_encode() || !stream->record_start()) {
    ERROR("Failed to start recording!");
    return ME_ERROR;
  }
  return ME_OK;
}

void stop_record()
{
  if (is_stream_init && stream->is_recording ()) {
    stream->record_stop();
    simplecam->goto_idle();
  }
}

void finit()
{
  set_led_enable(WIFI_GPIO_ID, 0);
  close(event_fd);
  delete qrcode_reader;
  delete config;
  delete stream;
  delete simplecam;
  AM_DELETE(led_flash);
  AM_DELETE(signal_thread);
}

AM_ERR signal_handle_thread (void *context)
{
  int sig_caught;
  sigwait(&signal_mask, &sig_caught);

  switch(sig_caught) {
    case SIGINT:
    case SIGTERM:
      INFO("signal captured!");
      stop = true;
      stop_record();
      delete_wifi_setup();
      finit();
      exit(0);
      break;

    default:
      ERROR("Unexpected signal: %d!", sig_caught);
      break;
  }

  return ME_OK;
}

AM_ERR init()
{
  set_led_enable(WIFI_GPIO_ID, 1);
  set_led_state(WIFI_GPIO_ID, LED_STATE_OFF);
  //open event0
  event_fd = open(event_dev_path, O_RDONLY);
  if (event_fd < 0) {
    printf("open %s error", event_dev_path);
    return ME_ERROR;
  }

  //init camera
  if ((config = new AmConfig ()) == NULL) {
    ERROR ("Failed to create an instance of AmConfig");
    return ME_NO_MEMORY;
  }

  config->set_vin_config_path(simple_camera_vin_config);
  if (!config->load_vdev_config()) {
    ERROR("Failed to load configurations!");
    return ME_ERROR;
  }
  vdevConfig = config->vdevice_config();

  if ((stream = new AmRecordStream ()) == NULL) {
    ERROR ("Failed to create an instance of AmStream");
    return ME_NO_MEMORY;
  }

  if ((simplecam = new AmSimpleCam (vdevConfig)) == NULL) {
    ERROR ("Failed to create an instance of AmSimpleCam");
    return ME_NO_MEMORY;
  }

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
  if (!config->load_record_config()) {
    ERROR("Failed to load configurations!");
    return ME_ERROR;
  }

  //init qrcode_reader
  if ((qrcode_reader = new AmQrcodeReader ()) == NULL) {
    ERROR ("Failed to create an instance of qrcode_reader!");
    return ME_ERROR;
  } else {
    qrcode_reader->set_vin_config_path(simple_camera_vin_config);
  }

  if ((signal_thread = CThread::Create ("signalHandler",
                                        signal_handle_thread, NULL)) == NULL) {
    ERROR ("Failed to signal handler!");
    return ME_ERROR;
  }

  return ME_OK;
}

AM_ERR parse_wifi_config_file()
{
  char wifi_content[MAX_FILE_NAME_LEN];
  char *p = wifi_content;
  char *key_point;

  if ((config_file = fopen(wifi_config_path, "r")) == NULL) {
    ERROR("%s", strerror(errno));
    return ME_ERROR;
  }
  fscanf(config_file, "wifi:%s", wifi_content);
  fclose(config_file);
  INFO("wifi content: %s\n", wifi_content);
  do {
    while ((key_point = strsep(&p, ";")) != NULL) {
      if (*key_point == 0) {
        continue;
      } else {
        break;
      }
    }
    if (key_point) {
      switch (*key_point) {
        case 'T':
          sscanf(key_point, "T:%s", auth_type);
          break;
        case 'S':
          sscanf(key_point, "S:%s", wifi_ssid);
          break;
        case 'P':
          sscanf(key_point, "P:%s", wifi_key);
          break;
        default:
          break;
      }
    }
  } while (key_point);
  return ME_OK;
}

AM_ERR key_cmd_process(input_event *cmd)
{
  unsigned int i;

  if (cmd->type != EV_KEY)
    return ME_OK;

  for (i = 0; i < INPUT_KEY_INFO_TABLE_SIZE; ++i) {
    if (cmd->code == (uint16_t)INPUT_KEY_INFO[i].input_key) {
      break;
    }
  }
  if (i == INPUT_KEY_INFO_TABLE_SIZE) {
    ERROR(" can't find the key map for event.code %d", cmd->code);
    state = STATE_OK;
    return ME_ERROR;
  }

  if ((cmd->value == 0) && (cmd->code == INPUT_KEY_INFO[0].input_key)) {
    if (is_stream_init && stream->is_recording()) {
      NOTICE("Recording is already started!");
      return ME_OK;
    }
    state = STATE_WAIT;
    if (led_flash == NULL) {
      //create led control thread
      if ((led_flash = CThread::Create("Flash led",
                                       led_control_thread, NULL)) == NULL) {
        ERROR("Create led control thread failed");
        return ME_ERROR;
      }
    }

    if (qrcode_reader->qrcode_read (wifi_config_path)) {
      state = STATE_BUSY;
      NOTICE ("Please see reading result from: %s", wifi_config_path);
      if (parse_wifi_config_file() != ME_OK) {
        ERROR("parse wifi config file error");
        return ME_ERROR;
      }
      if (setup_wifi() != ME_OK) {
        ERROR("Setup wifi error");
        return ME_ERROR;
      }
      if (start_record() != ME_OK) {
        ERROR("Start record error");
        return ME_ERROR;
      }
      state = STATE_OK;
    } else {
      ERROR ("Failed to read qrcode!");
      return ME_ERROR;
    }
  }
  return ME_OK;
}

AM_ERR main_loop()
{
  input_event event;

  while (true) {
    if (read(event_fd, &event, sizeof(event)) < 0) {
      state = STATE_ERROR;
      ERROR("read event_fd error\n");
      continue;
    }
    if (key_cmd_process(&event) != ME_OK) {
      state = STATE_ERROR;
    }
  }
  return ME_OK;
}

int main()
{
  //deamon_init();
  block_signals();
  if (init()) {
    ERROR("Initialize failed!");
    return ME_ERROR;
  }
  main_loop();
  stop_record();
  delete_wifi_setup();
  finit();

  return 0;
}
