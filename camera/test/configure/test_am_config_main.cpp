/*******************************************************************************
 * test_am_config_main.cpp
 *
 * Histroy:
 *  2012-3-5 2012 - [ypchang] created file
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
#include "am_configure.h"

int main(int argc, char *argv[])
{
  if (argc != 2) {
    ERROR("Usage: %s module[vin | vout | vdevice | stream | photo]", argv[0]);
    exit (0);
  } else {
    AmConfig *config = new AmConfig();
    if (is_str_equal(argv[1], "vin")) {
      if (config->load_vdev_config()) {
        config->print_vin_config();
      }
    } else if (is_str_equal(argv[1], "vout")) {
      if (config->load_vdev_config()) {
        config->print_vout_config();
      }
    } else if (is_str_equal(argv[1], "vdevice")) {
      if (config->load_vdev_config()) {
        VDeviceParameters *vdev = config->vdevice_config();
        if (vdev) {
          config->print_vdev_config();
        }
        delete config;
        config = NULL;
      }
    } else if (is_str_equal(argv[1], "stream")) {
      if (config->load_vdev_config() &&
          config->load_record_config()) {
        RecordParameters *record = config->record_config();
        if (record) {
          config->print_record_config();
        }
        delete config;
        config = NULL;
      }
    } else if (is_str_equal(argv[1], "photo")) {
      if (config->load_photo_config()) {
        PhotoParameters *photo = config->photo_config();
        if (photo) {
          config->print_photo_config();
        }
      }
    } else {
      ERROR("Unknown module type %s", argv[1]);
    }

    delete config;
    DEBUG("Exit!");
  }

  return 0;
}


