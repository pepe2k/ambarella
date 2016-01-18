/*
 * qrcode.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 21/09/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <zbar.h>

#include "am_include.h"
#include "am_data.h"
#include "am_configure.h"
#include "am_utility.h"
#include "am_vdevice.h"
#include "am_qrcode.h"

using namespace zbar;

AmQrcodeReader::AmQrcodeReader ():
   vin_config_path (NULL)
{}

AmQrcodeReader::~AmQrcodeReader ()
{
  delete[] vin_config_path;
}

bool AmQrcodeReader::set_vin_config_path (const char *config_path)
{
  delete[] vin_config_path;
  vin_config_path = (config_path ? amstrdup(config_path) : NULL);

   return (vin_config_path != NULL);
}

bool AmQrcodeReader::qrcode_read(const char *result_path)
{
  FILE *fp = NULL;
  bool ret = true;
  AmConfig *config = NULL;
  VDeviceParameters *vdev_config = NULL;
  AmSimpleCam *simple_cam = NULL;

  do {
    if (AM_UNLIKELY(NULL == (fp = fopen(result_path, "w+")))) {
      ERROR("Failed to open %s: %s", result_path, strerror(errno));
      ret = false;
      break;
    }

    if (AM_UNLIKELY(NULL == (config = new AmConfig()))) {
      ERROR("Failed to create an instance of AmConfig!");
      ret = false;
      break;
    }

    config->set_vin_config_path(vin_config_path);
    if (AM_UNLIKELY(!config->load_vdev_config ())) {
      ERROR("Failed to load vdev configurations!");
      ret = false;
      break;
    }

    if (AM_UNLIKELY(NULL == (vdev_config = config->vdevice_config()))) {
      ERROR("Failed to get vdevice configurations!");
      ret = false;
      break;
    }

    if (AM_UNLIKELY(NULL == (simple_cam = new AmSimpleCam(vdev_config)))) {
      ERROR("Failed to create simplecam instance!");
      ret = false;
      break;
    } else {
      for (uint32_t i = 0; i < vdev_config->vin_number; i ++) {
        simple_cam->set_vin_config(config->vin_config(i), i);
      }

      for (uint32_t i = 0; i < vdev_config->vout_number; i ++) {
        simple_cam->set_vout_config(config->vout_config(i), i);
      }

      simple_cam->set_encoder_config(config->encoder_config());
      for (uint32_t i = 0; i < vdev_config->stream_number; ++ i) {
        simple_cam->set_stream_config(config->stream_config(i), i);
      }
      if (!simple_cam->enter_preview()) {
        ERROR("Simple camera failed to enter preview!");
        ret = false;
        break;
      } else {
        int ret_val = 0;
        int count = 0;
        YBufferFormat y_buffer = { 0 };
        ImageScanner zbar_scanner;
        do {
          if (simple_cam->get_y_data(&y_buffer)) {
            Image image(y_buffer.y_width,
                        y_buffer.y_height,
                        "Y800",
                        y_buffer.y_addr,
                        y_buffer.y_width * y_buffer.y_height);
            if ((ret_val = zbar_scanner.scan(image)) > 0) {
              for (Image::SymbolIterator symbol = image.symbol_begin();
                  symbol != image.symbol_end(); ++ symbol) {
                fputs(symbol->get_data().c_str(), fp);
              }
            } else if (ret_val == 0) {
              NOTICE("No symbol found");
            } else {
              ERROR("Zbar scan error!");
            }
          } else {
            ERROR("Failed to get Y data!");
          }
        } while ((ret_val <= 0) && (++ count < 10));
        if ((ret_val <= 0) && (count >= 10)) {
          ERROR("Failed to resolve symbol!");
          ret = false;
          break;
        }
      }
    }
  } while (0);

  if (AM_LIKELY(fp)) {
    fputs("\n", fp);
    fclose(fp);
  }

  if (AM_UNLIKELY(!ret)) {
    /* delete wifi.conf when qr code fails to read wifi configuration */
    if (AM_UNLIKELY(result_path && (remove(result_path) < 0))) {
      ERROR("Failed to remove %s: %s", result_path, strerror (errno));
    }
  }
  delete config;
  delete simple_cam;

  return ret;
}
