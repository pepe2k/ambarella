/*******************************************************************************
 * test_encode_device_main.cpp
 *
 * History:
 *  Dec 4, 2012 2012 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_data.h"
#include "am_configure.h"
#include "am_utility.h"
#include "am_vdevice.h"

#define VIN_CONFIG BUILD_AMBARELLA_CAMERA_CONF_DIR"/video_vin.conf"

static int get_three_int(char *name, int *first, int *second, int *third,
                         char delim1, char delim2)
{
  char tmp_string[16];
  char * separator1 = NULL, *separator2 = NULL;

  separator1 = strchr(name, delim1);
  if (!separator1)
    return -1;
  separator2 = strchr(separator1, delim2);
  if (!separator2)
    return -1;

  strncpy(tmp_string, name, separator1 - name);
  tmp_string[separator1-name] = '\0';
  *first = atoi(tmp_string);
  strncpy(tmp_string, separator1 + 1, separator1 - name);
  *second = atoi(tmp_string);
  strncpy(tmp_string, separator2 + 1, separator2 - separator1);
  *third = atoi(tmp_string);

  return 0;
}

static void usage()
{
  ERROR("Usage: test_encdev "
            "[encode 1280x720p30 720x480p30 ... ]"
            "| [stop] | [framerate 20 10 ...]"
            "| [bitrate 2000000 1000000 ...]");
}

int main(int argc, char *argv[])
{
  bool error = false;
  if ((argc < 2)
      || (!is_str_equal("encode", argv[1])
          && !is_str_equal("stop", argv[1])
          && !is_str_equal("bitrate", argv[1])
          && !is_str_equal("framerate", argv[1]))) {
    usage();
    exit(0);
  }

  AmConfig *config = new AmConfig();
  //If you have different config, please set it here
  config->set_vin_config_path(VIN_CONFIG);
  //config->set_vout_config_path    (vout_config_path);
  //config->set_vdevice_config_path (vdevice_config_path);
  //config->set_record_config_path  (record_config_path);

  if (!config || !config->load_vdev_config()) {
    ERROR("Faild to get VideoDevice's configurations!");
    return -1;
  }

  VDeviceParameters *vdevConfig = config->vdevice_config();
  if (! vdevConfig) {
    ERROR("Faild to get VideoDevice's configurations!");
    return -1;
  }
  AmEncodeDevice encode_device(vdevConfig);

  for (uint32_t i = 0; i < vdevConfig->vin_number; ++i) {
    encode_device.set_vin_config(config->vin_config(i), i);
  }

  for (uint32_t i = 0; i < vdevConfig->vout_number; ++i) {
    encode_device.set_vout_config(config->vout_config(i), i);
  }

  encode_device.set_encoder_config(config->encoder_config());

  for (uint32_t i = 0; i < vdevConfig->stream_number; ++i) {
    encode_device.set_stream_config(config->stream_config(i), i);
  }

  if (is_str_equal("encode", argv[1])) {
    if (argc > 2) {
      EncodeSize *pSize = new EncodeSize[vdevConfig->stream_number];
      uint32_t stream_num = 0;
      for (uint32_t i = 0; i < vdevConfig->stream_number; ++i) {
        EncodeType type = AM_ENCODE_TYPE_NONE;
        if (i < (uint32_t)argc - 2) {
        int width = 0, height = 0, framerate = 0;
          switch (argv[i + 2][0]) {
            case 'm':
            case 'M':
              type = AM_ENCODE_TYPE_MJPEG;
              break;
            case 'h':
            case 'H':
              type = AM_ENCODE_TYPE_H264;
              break;
            default:
              break;
          }
          if (get_three_int(&argv[i + 2][1], &width, &height, &framerate, 'x', 'p')
              < 0) {
            ERROR("Invalid resolution %s. must be [width]x[height]p[framerate].",
                  &argv[i][1]);
            error = true;
            break;
          } else {
            pSize[i].width = (uint16_t) width;
            pSize[i].height = (uint16_t) height;
            stream_num++;
          }

          if (encode_device.set_stream_framerate(i, framerate)) {
            INFO("set stream %d framerate %d successfully!", i, framerate);
          } else {
            ERROR("Failed to set stream %d framerate %d!", i, framerate);
            error = true;
          }
        }

        if (encode_device.set_stream_type(i, type)) {
          INFO("set stream %d type %d successfully!", i, type);
        } else {
          ERROR("Failed to set stream %d type %d!", i, type);
          error = true;
        }
      }

      if (!error) {
        if (encode_device.set_stream_size_all(stream_num, pSize)) {
          INFO("set_stream_size_all successfully!");
        } else {
          ERROR("Failed to set all streams size!");
          error = true;
        }
      }
      delete[] pSize;
    }


    if (encode_device.start_encode()) {
      INFO("Start all streams successfully!");
    } else {
      ERROR("Failed to start all streams!");
      error = true;
    }
  } else if (is_str_equal("stop", argv[1])) {
     if (encode_device.stop_encode()) {
       INFO("Stop all streams successfully!");
     } else {
       ERROR("Failed to stop all streams!");
       error = true;
     }
  } else if (is_str_equal("framerate", argv[1])) {
    for (int i = 2; i < argc; ++i) {
      uint32_t framerate = atoi(argv[i]);
      if (encode_device.set_stream_framerate(i - 2, framerate)) {
        INFO("Change stream %u frame rate %d.", i - 2, framerate);
      } else {
        ERROR("Failed to Change stream%u frame rate %d.", i - 2, framerate);
        error = true;
      }
    }
  } else if (is_str_equal("bitrate", argv[1])) {
    for (int i = 2; i < argc; ++i) {
      uint32_t bitrate = atoi(argv[i]);
      if (encode_device.set_cbr_bitrate(i - 2, bitrate)) {
        INFO("Change stream %u bitrate %dbps.", i - 2, bitrate);
      } else {
        ERROR("Failed to Change stream%u bitrate %dbps.", i - 2, bitrate);
        error = true;
      }
    }
  } else {
    ERROR("Unrecognized parameter: %s", argv[1]);
    error = true;
  }

  /* Save stream configuration */
  if (!error) {
    for (uint32_t i = 0; i < vdevConfig->stream_number; ++i) {
      config->set_stream_config(config->stream_config(i), i);
    }
  }

  delete config;

  return 0;
}
