/*******************************************************************************
 * am_config_fisheye.cpp
 *
 * History:
 *  Mar 20, 2013 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
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
#include "am_config_fisheye.h"

FisheyeParameters* AmConfigFisheye::get_fisheye_config()
{
  FisheyeParameters *ret = NULL;
  if (init()) {
    if (!mFisheyeParameters) {
      mFisheyeParameters = new FisheyeParameters();
    }
    if (mFisheyeParameters) {
      int32_t value = get_int("FISHEYE:Mount", 0);
      switch (value) {
        case AM_FISHEYE_MOUNT_WALL:
        case AM_FISHEYE_MOUNT_CEILING:
        case AM_FISHEYE_MOUNT_DESKTOP:
          mFisheyeParameters->mount = (FisheyeMount)value;
          break;
        default:
          WARN("Invalid Mount value: %d, reset to %d (Wall Mount).",
              value, AM_FISHEYE_MOUNT_WALL);
          mFisheyeParameters->mount = AM_FISHEYE_MOUNT_WALL;
          mFisheyeParameters->config_changed = 1;
          break;
      }

      value = get_int("FISHEYE:Projection", 0);
      switch (value) {
        case AM_FISHEYE_PROJECTION_FTHETA:
        case AM_FISHEYE_PROJECTION_FTAN:
          mFisheyeParameters->projection = (FisheyeProjection)value;
          break;
        default:
          WARN("Invalid Projection value: %d, reset to %d (F-theta).",
              value, AM_FISHEYE_PROJECTION_FTHETA);
          mFisheyeParameters->projection = AM_FISHEYE_PROJECTION_FTHETA;
          mFisheyeParameters->config_changed = 1;
          break;
      }

      value = get_int("FISHEYE:MaxFOV", 0);
      if (value < 0) {
        mFisheyeParameters->max_fov = 180;
        WARN("Invalid MaxFOV value: %d < 0, reset to %d.", value,
             mFisheyeParameters->max_fov);
        mFisheyeParameters->config_changed = 1;
      } else {
        mFisheyeParameters->max_fov = value;
      }

      value = get_int("FISHEYE:MaxCircle", 0);
      if (value < 0) {
        mFisheyeParameters->max_circle = 1024;
        WARN("Invalid MaxFOV value: %d < 0, reset to %d.", value,
             mFisheyeParameters->max_circle);
        mFisheyeParameters->config_changed = 1;
      } else {
        mFisheyeParameters->max_circle = value;
      }

      get_warp_config(mFisheyeParameters->layout);

      if (mFisheyeParameters->config_changed) {
        set_fisheye_config(mFisheyeParameters);
      }
    }
    ret = mFisheyeParameters;
  }
  return ret;
}

void AmConfigFisheye::set_fisheye_config(FisheyeParameters *config)
{
  if (AM_LIKELY(config)) {
    if (init()) {
      set_value("FISHEYE:Mount", config->mount);
      set_value("FISHEYE:Projection", config->projection);
      set_value("FISHEYE:MaxFOV", config->max_fov);
      set_value("FISHEYE:MaxCircle", config->max_circle);
      set_warp_config(config->layout);
      config->config_changed = 0;
      save_config();
    } else {
      WARN("Failed to open %s, fisheye configuration NOT saved!", mConfigFile);
    }
  }
}

void AmConfigFisheye::get_warp_config(WarpParameters &config)
{
  int  value;

  value = get_int("LAYOUT:UnwarpWidth", 0);
  if (value <= 0) {
   config.unwarp.width = 1920;
    WARN("Invalid UnwarpWidth value: %d < 0, reset to %d.", value,
         config.unwarp.width);
    mFisheyeParameters->config_changed = 1;
  } else {
    config.unwarp.width = value;
  }

  value = get_int("LAYOUT:UnwarpHeight", 0);
  if (value <= 0) {
    config.unwarp.height = 1080;
    WARN("Invalid UnwarpHeight value: %d < 0, reset to %d.", value,
         config.unwarp.height);
    mFisheyeParameters->config_changed = 1;
  } else {
    config.unwarp.height = value;
  }

  value = get_int("LAYOUT:WarpWidth", -1);
  if (value <= 0) {
    config.warp.width = config.unwarp.width;
    WARN("Invalid WarpWidth value: %d < 0, reset to %d (UnwarpWidth).",
         value, config.warp.width);
    mFisheyeParameters->config_changed = 1;
  } else {
    config.warp.width = value;
  }

  value = get_int("LAYOUT:WarpHeight", -1);
  if (value <= 0) {
    config.warp.height = config.unwarp.height;
    WARN("Invalid WarpHeight value: %d < 0, reset to %d (UnwarpHeight).",
         value, config.warp.height);
    mFisheyeParameters->config_changed = 1;
  } else {
    config.warp.height = value;
  }

  value = get_int("LAYOUT:UnwarpWindowWidth", -1);
  if (value <= 0) {
    config.unwarp_window.width = config.unwarp.width;
    WARN("Invalid UnwarpWindowWidth value: %d < 0, reset to %d (UnwarpWidth).",
         value, config.unwarp_window.width);
    mFisheyeParameters->config_changed = 1;
  } else {
    config.unwarp_window.width = value;
  }

  value = get_int("LAYOUT:UnwarpWindowHeight", -1);
  if (value <= 0) {
    config.unwarp_window.height = config.unwarp.height;
    WARN("Invalid UnwarpWindowHeight value: %d < 0, reset to %d (UnwarpWidth).",
         value, config.unwarp_window.height);
    mFisheyeParameters->config_changed = 1;
  } else {
    config.unwarp_window.height = value;
  }

  value = get_int("LAYOUT:UnwarpWindowOffsetX", -1);
  if (value < 0) {
    config.unwarp_window.x = 0;
    WARN("Invalid UnwarpWindowOffsetX value: %d < 0, reset to %d.", value,
         config.unwarp_window.x);
    mFisheyeParameters->config_changed = 1;
  } else {
    config.unwarp_window.x = value;
  }

  value = get_int("LAYOUT:UnwarpWindowOffsetY", -1);
  if (value < 0) {
    config.unwarp_window.y = 0;
    WARN("Invalid UnwarpWindowOffsetY value: %d < 0, reset to %d.", value,
         config.unwarp_window.y);
    mFisheyeParameters->config_changed = 1;
  } else {
    config.unwarp_window.y = value;
  }
}

void AmConfigFisheye::set_warp_config(WarpParameters &config)
{
  set_value("LAYOUT:UnwarpWidth", config.unwarp.width);
  set_value("LAYOUT:UnwarpHeight", config.unwarp.height);
  set_value("LAYOUT:WarpWidth", config.warp.width);
  set_value("LAYOUT:WarpHeight", config.warp.height);

  set_value("LAYOUT:UnwarpWindowWidth", config.unwarp_window.width);
  set_value("LAYOUT:UnwarpWindowHeight", config.unwarp_window.height);
  set_value("LAYOUT:UnwarpWindowOffsetX", config.unwarp_window.x);
  set_value("LAYOUT:UnwarpWindowOffsetY", config.unwarp_window.y);
}

