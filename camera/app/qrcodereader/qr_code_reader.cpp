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
#include <zbar.h>

#define SIMPLE_CAM_VIN_CONFIG "/etc/camera/video_vin.conf"
using namespace zbar;


int main(int argc, char *argv[])
{
  if (argc < 2 || argc > 3) {
    NOTICE("Usage: %s <YUV filename>", argv[0]);
    return 0;
  }
  int status;
  FILE *fp = NULL;
  AmConfig *config = new AmConfig();
  char command[128];

  snprintf(command, sizeof(command),
           "ImageServerDaemon");
  status = system(command);
  if (WEXITSTATUS(status)) {
    ERROR("Failed to run ImageServerDaemon");
  }

  if (NULL == (fp = fopen(argv[1],"wb+"))) {
    ERROR("Failed to open %s: %s", argv[1], strerror(errno));
    return -1;
  }

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
      if (AM_LIKELY(simplecam.enter_preview())) {
        YBufferFormat yBuffer = {0};
        ImageScanner zbarScanner;
        int retVal = 0;
        int count = 0;

        do {
          if (simplecam.get_y_data(&yBuffer)) {
            Image image(yBuffer.y_width, yBuffer.y_height, "Y800",
                        yBuffer.y_addr, yBuffer.y_width * yBuffer.y_height);
            if (AM_LIKELY((retVal = zbarScanner.scan(image)) > 0)) {
              for (Image::SymbolIterator symbol = image.symbol_begin();
                   symbol != image.symbol_end(); ++ symbol) {
                fprintf(stdout, "\n%s\n\n", symbol->get_data().c_str());
              }
              snprintf(command, sizeof(command),
                       "aplay -vv -fdat ./OK.wav");
              status = system(command);
              if (WEXITSTATUS(status)) {
                ERROR("Failed to play wav file");
              }
              size_t written = fwrite(yBuffer.y_addr, sizeof(uint8_t),
                                      yBuffer.y_height * yBuffer.y_width, fp);

              NOTICE("written data: %d Bytes, width: %d, height: %d.",
                     written, yBuffer.y_width, yBuffer.y_height);
            } else if (retVal == 0) {
              NOTICE("No symbold found!");
            } else if (retVal < 0) {
              ERROR("Zbar scan error!");
            }
          } else {
            ERROR("Failed to get Y data!");
          }
          ++ count;
        } while ((retVal <= 0) && (count < 20));
        if (count >= 20) {
          snprintf(command, sizeof(command),
                   "aplay -vv -fdat ./ERROR.wav");
          status = system(command);
          if (WEXITSTATUS(status)) {
            ERROR("Failed to play wav file");
          }
          ERROR("Failed to resolve symbol!");
          size_t written = fwrite(yBuffer.y_addr, sizeof(uint8_t),
                                  yBuffer.y_height * yBuffer.y_width, fp);

          NOTICE("written data: %d Bytes, width: %d, height: %d.",
                 written, yBuffer.y_width, yBuffer.y_height);
        }
      } else {
        ERROR("Failed to enter preview!");
      }
    } else {
      ERROR("Faild to get VideoDevice's configurations!");
    }
    delete config;
  } else {
    ERROR("Failed to load configurations!");
  }
  snprintf(command, sizeof(command),
           "ImageServerDaemon -k");
  status = system(command);
  if (WEXITSTATUS(status)) {
    ERROR("Failed to kill ImageServerDaemon");
  }
  fclose(fp);
  return 0;
}

