/*******************************************************************************
 * test_webcam_main.cpp
 *
 * History:
 *  Dec 26, 2012 2012 - [qianshen] created file
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
#include "am_middleware.h"

#define VIN_CONFIG BUILD_AMBARELLA_CAMERA_CONF_DIR"/video_vin.conf"
char text[][128] = {
  "Ambarella Flexible Linux SDK",
  "True Type Support",
  "安霸中国(Chinese)",
  "여보세요(Korean)",
};

char ttfFile[][128] = {
  "/usr/share/fonts/Vera.ttf",
  "/usr/share/fonts/Lucida.ttf",
  "/usr/share/fonts/gbsn00lp.ttf",
  "/usr/share/fonts/UnPen.ttf",
};

static void usage()
{
  ERROR("Usage: test_encdev "
            "[encode] | [stop] | [overlay]");
}

int main(int argc, char *argv[])
{
  if (argc < 2 || (!is_str_equal("encode", argv[1])
      && !is_str_equal("overlay", argv[1])
          && !is_str_equal("stop", argv[1]))) {
    usage();
    exit(0);
  }

  AmConfig *config = new AmConfig();
  config->set_vin_config_path(VIN_CONFIG);

  AmWebCam webcam(config);

  if (!webcam.init()) {
    ERROR("Failed to init webcam.");
    return -1;
  }

  if (is_str_equal("encode", argv[1])) {
    webcam.start_encode() ? INFO("Start all streams successfully!"):
    ERROR("Failed to start all streams!");
  } else if (is_str_equal("stop", argv[1])) {
    webcam.stop_encode() ? INFO("Stop all streams successfully!"):
    ERROR("Failed to start all streams!");
  } else if (is_str_equal("overlay", argv[1])) {
    TextBox box;
    Point offset[MAX_OVERLAY_AREA_NUM] = { 0};
    memset(&box, 0, sizeof(TextBox));

    box.width = 700;
    box.height = 80;
    webcam.add_overlay_time(0, 0, offset[0], &box);
    webcam.add_overlay_bitmap(0, 1, offset[1],
        "/usr/local/bin/Ambarella-256x128-8bit.bmp");
    box.width = 380;
    box.height = 120;
    webcam.add_overlay_text(0, 2, offset[2], &box, text[0]);


    while (1) {
      sleep(10);
    }
  }
  delete config;
  return 0;
}
