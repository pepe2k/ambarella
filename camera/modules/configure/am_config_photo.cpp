/*******************************************************************************
 * am_config_photo.cpp
 *
 * Histroy:
 *  2012-4-5 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
#include <iniparser.h>
#ifdef __cplusplus
}
#endif

#include "am_include.h"
#include "utilities/am_define.h"
#include "utilities/am_log.h"
#include "datastructure/am_structure.h"
#include "am_config_base.h"
#include "am_config_photo.h"

PhotoParameters* AmConfigPhoto::get_photo_config()
{
  PhotoParameters *ret = NULL;
  if (init()) {
    if (!mPhotoParameters) {
      mPhotoParameters = new PhotoParameters();
    }
    if (mPhotoParameters) {
      int32_t quality = get_int("PHOTO:JpegQuality", 75);
      if ((quality >= 5) && (quality <= 95)) {
        mPhotoParameters->quality = (uint32_t)quality;
      } else {
        WARN("Invalid JPEG quality value: %d, reset to 75!", quality);
        mPhotoParameters->quality = 75;
        mPhotoParameters->config_changed = 1;
      }

      mPhotoParameters->\
      set_file_name_prefix(get_string("PHOTO:FileNamePrefix", "A5s"));

      mPhotoParameters->\
      set_file_store_location(get_string("PHOTO:FileStoreLocation",
                                         "/media/mmcblk0p1/image/"));
      if (mPhotoParameters->config_changed) {
        set_photo_config(mPhotoParameters);
      }
    }
    ret = mPhotoParameters;
  }

  return ret;
}

void AmConfigPhoto::set_photo_config(PhotoParameters *config)
{
  if (AM_LIKELY(config)) {
    if (init()) {
      set_value("PHOTO:JpegQuality", config->quality);
      set_value("PHOTO:FileNamePrefix", config->file_name_prefix);
      set_value("PHOTO:FileStoreLocation", config->file_store_location);
      config->config_changed = 0;
      save_config();
    } else {
      WARN("Failed openint %s, photo configuration NOT saved!", mConfigFile);
    }
  }
}
