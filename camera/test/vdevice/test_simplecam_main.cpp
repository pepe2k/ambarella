/*******************************************************************************
 * test_simplecam_main.cpp
 *
 * Histroy:
 *  2012-3-10 2012 - [ypchang] created file
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

#define SIMPLE_CAM_VIN_CONFIG BUILD_AMBARELLA_CAMERA_CONF_DIR"/video_vin.conf"
int main(int argc, char *argv[])
{
  if ((argc != 2) || (!is_str_equal("idle",   argv[1]) &&
                      !is_str_equal("prev",   argv[1]) &&
                      !is_str_equal("encode", argv[1]) &&
                      !is_str_equal("decode", argv[1]) &&
                      !is_str_equal("stop",   argv[1]) &&
                      !is_str_equal("ready",  argv[1]) &&
                      !is_str_equal("reset",  argv[1]))) {
    ERROR("Usage: test_simplecam operation "
          "[idle | prev | encode | stop | ready | reset]");
    exit (0);
  } else {
    AmConfig *config = new AmConfig();
    //If you have different config, please set it here
    config->set_vin_config_path(SIMPLE_CAM_VIN_CONFIG);
    //config->set_vout_config_path    (vout_config_path);
    //config->set_vdevice_config_path (vdevice_config_path);
    //config->set_record_config_path  (record_config_path);
    if (config && config->load_vdev_config()) {
      VDeviceParameters *vdevConfig = config->vdevice_config();
      if (vdevConfig) {
        AmSimpleCam simplecam(vdevConfig);
        for (uint32_t i = 0; i < vdevConfig->vin_number; ++ i) {
          simplecam.set_vin_config(config->vin_config(i), i);
        }
        for (uint32_t i = 0; i < vdevConfig->vout_number; ++ i) {
          simplecam.set_vout_config(config->vout_config(i), i);
        }
        simplecam.set_encoder_config(config->encoder_config());
        for (uint32_t i = 0; i < vdevConfig->stream_number; ++ i) {
          simplecam.set_stream_config(config->stream_config(i), i);
        }

        if (is_str_equal("idle", argv[1]) ||
                   is_str_equal("stop", argv[1])) {
          simplecam.goto_idle() ?
              INFO("Goto Idle successfully!") :
              ERROR("Goto Idle failed!");
        } else if (is_str_equal("prev", argv[1])) {
          simplecam.enter_preview() ?
              INFO("Enter preview successfully!") :
              ERROR("Enter preview failed!");
        } else if (is_str_equal("encode", argv[1])) {
          simplecam.start_encode() ?
              INFO("Start encoding successfully!") :
              ERROR("Start encoding failed!");
        } else if (is_str_equal("decode", argv[1])) {
          simplecam.enter_decode_mode() ?
              INFO("Enter decoding mode successfully!") :
              ERROR("Start decoding mode failed!");
        } else if (is_str_equal("ready", argv[1])) {
          simplecam.ready_for_encode() ?
              INFO("Camera is ready for encoding!") :
              ERROR("Failed preparing camera for encoding!");
        } else if (is_str_equal("reset", argv[1])) {
          simplecam.reset_device() ?
              INFO("Reset camera successfully!") :
              ERROR("Reset camera failed!");
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

