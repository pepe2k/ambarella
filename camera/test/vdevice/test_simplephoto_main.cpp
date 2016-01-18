/*******************************************************************************
 * test_simplephoto_main.cpp
 *
 * Histroy:
 *  2012-4-9 2012 - [ypchang] created file
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
#include "am_configure.h"
#include "am_utility.h"
#include "am_vdevice.h"

#define SIMPLE_PHOTO_VIN_CONFIG BUILD_AMBARELLA_CAMERA_CONF_DIR"/photo_vin.conf"

int main(int argc, char *argv[])
{
  if ((argc != 3) ||
      (!is_str_equal("jpeg",   argv[1]) &&
      !is_str_equal("raw",    argv[1])) ||
      (atoi(argv[2]) <= 0)) {
    ERROR("Usage: test_simplephoto operation [jpeg | raw] num [>=1]");
    exit (0);
  } else {
    AmConfig *config = new AmConfig();
    //If you have different config, please set it here
    config->set_vin_config_path(SIMPLE_PHOTO_VIN_CONFIG);
    //config.set_vout_config_path    (vout_config_path);
    //config.set_vdevice_config_path (vdevice_config_path);
    //config.set_record_config_path  (record_config_path);
    if (config && config->load_vdev_config()) {
      VDeviceParameters *vdevConfig = config->vdevice_config();
      if (vdevConfig) {
        AmSimplePhoto simplephoto(vdevConfig);
        for (uint32_t i = 0; i < vdevConfig->vin_number; ++ i) {
          simplephoto.set_vin_config(config->vin_config(i), i);
        }
        for (uint32_t i = 0; i < vdevConfig->vout_number; ++ i) {
          simplephoto.set_vout_config(config->vout_config(i), i);
        }
        simplephoto.set_encoder_config(config->encoder_config());
        for (uint32_t i = 0; i < vdevConfig->stream_number; ++ i) {
          simplephoto.set_stream_config(config->stream_config(i), i);
        }
        if (is_str_equal("jpeg", argv[1])) {
          if (config->load_photo_config()) {
            simplephoto.set_photo_config(config->photo_config());
            simplephoto.take_photo(AM_PHOTO_JPEG, (uint32_t)atoi(argv[2])) ?
                INFO("Successfully!") : ERROR("Failed!");
          } else {
            ERROR("Failed to load photo's configuration!");
          }
        } else if (is_str_equal("raw", argv[1])) {
          if (config->load_photo_config()) {
            simplephoto.set_photo_config(config->photo_config());
            simplephoto.take_photo(AM_PHOTO_RAW, (uint32_t)atoi(argv[2])) ?
                INFO("Successfully!") : ERROR("Failed!");
          } else {
            ERROR("Failed to load photo's configuration!");
          }
        } else {
          ERROR("Unrecognized parameter: %s", argv[1]);
        }
      } else {
        ERROR("Faild to get VideoDevice's configurations!");
      }
      delete config;
    } else {
      ERROR("Failed to load configurations!");
    }
  }
  return 0;
}

