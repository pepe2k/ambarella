/*******************************************************************************
 * am_config_vin.cpp
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
#include "am_config_vin.h"

AmConfigVin::AmConfigVin(const char *configFileName)
  : AmConfigBase(configFileName),
    mVinParamsYuv(0),
    mVinParamsRgb(0)
{
}

AmConfigVin::~AmConfigVin()
{
  delete mVinParamsYuv;
  delete mVinParamsRgb;
  DEBUG("~AmConfigVin");
}

VinParameters* AmConfigVin::get_vin_config(VinType type)
{
  VinParameters* ret = NULL;

  if (init()) {
    switch(type) {
      case AM_VIN_TYPE_RGB: {
        ret = get_rgb_config();
        if (ret && ret->config_changed) {
          set_rgb_config(ret);
        }
      }break;
      case AM_VIN_TYPE_YUV: {
        ret = get_yuv_config();
        if (ret && ret->config_changed) {
          set_yuv_config(ret);
        }
      }break;
      default:break;
    }
  }

  return ret;
}

void AmConfigVin::set_vin_config(VinParameters *vinConfig, VinType type)
{
  if (init()) {
    switch(type) {
      case AM_VIN_TYPE_RGB:
        set_rgb_config(vinConfig);
        break;
      case AM_VIN_TYPE_YUV:
        set_yuv_config(vinConfig);
        break;
      default:break;
    }
    save_config();
    vinConfig->config_changed = 0;
  }
}

VinParameters* AmConfigVin::get_yuv_config()
{
  if (!mVinParamsYuv) {
    mVinParamsYuv = new VinParameters;
  }
  if (mVinParamsYuv) {
    memset(mVinParamsYuv, 0, sizeof(*mVinParamsYuv));
    mVinParamsYuv->source = (uint32_t)get_int("YUV:Source", 0);
    mVinParamsYuv->video_mode =
        str_to_video_mode(get_string("YUV:Mode", "auto"));
    mVinParamsYuv->framerate  =
        str_to_fps(get_string((char *)"YUV:Framerate", "auto"));
    mVinParamsYuv->vin_eshutter_time = get_int("YUV:EshutterTime", 60);
    mVinParamsYuv->vin_agc_db = get_int("YUV:AgcDB", 6);
    mVinParamsYuv->mirror_mode.bayer_pattern =
        (uint32_t)get_int("YUV:BayerPattern", -1);
    mVinParamsYuv->mirror_mode.pattern =
        (uint32_t)get_int("YUV:Pattern", -1);
    if (mVinParamsYuv->framerate < 15) {
      WARN("VIN Framerate should NOT be lower than 15, reset to 15!");
      mVinParamsYuv->framerate = 15;
      mVinParamsYuv->config_changed = 1;
    }
  }

  return mVinParamsYuv;
}

void AmConfigVin::set_yuv_config(VinParameters* vinConfig)
{
  if (vinConfig) {
    set_value("YUV:Source", vinConfig->source);
    set_value("YUV:Mode", video_mode_to_str(vinConfig->video_mode));
    set_value("YUV:Framerate", fps_to_str(vinConfig->framerate));
    set_value("YUV:AgcDB", vinConfig->vin_agc_db);
    set_value("YUV:Pattern", vinConfig->mirror_mode.pattern);
  }
}

VinParameters* AmConfigVin::get_rgb_config()
{
  if (!mVinParamsRgb) {
    mVinParamsRgb = new VinParameters;
  }
  if (mVinParamsRgb) {
    memset(mVinParamsRgb, 0, sizeof(*mVinParamsRgb));
    mVinParamsRgb->source = (uint32_t)get_int("RGB:Source", 0);
    mVinParamsRgb->video_mode =
        str_to_video_mode(get_string("RGB:Mode", "auto"));
    mVinParamsRgb->framerate  = str_to_fps(get_string("RGB:Framerate", "auto"));
    mVinParamsRgb->vin_eshutter_time = get_int("RGB:EshutterTime", 60);
    mVinParamsRgb->vin_agc_db = get_int("RGB:AgcDB", 6);
    mVinParamsRgb->mirror_mode.bayer_pattern =
        (uint32_t)get_int("RGB:BayerPattern", -1);
    mVinParamsRgb->mirror_mode.pattern =
        (uint32_t)get_int("RGB:Pattern", -1);
    if (mVinParamsRgb->framerate < 15) {
      WARN("VIN Framerate should NOT be lower than 15, reset to 15!");
      mVinParamsRgb->framerate = 15;
      mVinParamsRgb->config_changed = 1;
    }
  }

  return mVinParamsRgb;
}

void AmConfigVin::set_rgb_config(VinParameters* vinConfig)
{
  if (vinConfig) {
    set_value("RGB:Source", vinConfig->source);
    set_value("RGB:Mode", video_mode_to_str(vinConfig->video_mode));
    set_value("RGB:Framerate", fps_to_str(vinConfig->framerate));
    set_value("RGB:AgcDB", vinConfig->vin_agc_db);
    set_value("RGB:Pattern", vinConfig->mirror_mode.pattern);
  }
}

uint32_t AmConfigVin::str_to_fps(const char *fps)
{
  uint32_t videoFps = 0xffffffff;

  if (fps) {
    for (uint32_t i = 0; i < sizeof(gFpsList)/sizeof(CameraVinFPS); ++ i) {
      if (is_str_equal(fps, gFpsList[i].fpsName)) {
        videoFps = gFpsList[i].fpsValue;
        DEBUG("%s is converted to %d", fps, videoFps);
        break;
      }
    }

    if (AM_UNLIKELY(videoFps == 0xffffffff)) {
      if (TYPE_INT == check_digits_type(fps)) {
        videoFps = (int32_t)(512000000 / atoi(fps));
      } else if (TYPE_DOUBLE == check_digits_type(fps)){
        videoFps = (int32_t)(51200000000LL / (atof(fps)*100));
      } else {
        videoFps = gFpsList[0].fpsValue;
        WARN("Invalid fps %s, reset to auto!", fps);
      }
    }
  }

  return videoFps;
}

const char* AmConfigVin::fps_to_str(uint32_t fps)
{
  char *videoFps = NULL;
  char string[64] = {0};

  for (uint32_t i = 0; i < sizeof(gFpsList) / sizeof(CameraVinFPS); ++ i) {
    if (gFpsList[i].fpsValue == fps) {
      videoFps = (char *)gFpsList[i].fpsName;
      DEBUG("%d is converted to %s", fps, videoFps);
      break;
    }
  }

  if (AM_UNLIKELY(videoFps == NULL)) {
    if (fps != 0xffffffff) {
      sprintf(string, "%.2lf", (double)(512000000/fps));
      videoFps = string;
    } else {
      videoFps = (char *)gFpsList[0].fpsName;
      WARN("Invalid fps value %d, reset to auto!", fps);
    }
  }

  return videoFps;
}

AmConfigVin::DigitalType AmConfigVin::check_digits_type(const char *str)
{
  DigitalType ret = TYPE_NONE;
  enum {DIGIT, CHAR, DOT} type;
  enum {START, GETNUM, GETDOT, ACCEPT, REJECT} state;

  if (str) {
    state = START;
    for (uint32_t count = 0; count < strlen(str); ++ count) {
      if ((str[count] >= '0') && (str[count] <= '9')) {
        type = DIGIT;
      } else if (str[count] == '.') {
        type = DOT;
      } else {
        type = CHAR;
      }
      switch(state) {
        case START: {
          switch(type) {
            case DIGIT:
              state = GETNUM; break;
            case CHAR:
            case DOT:
              state = REJECT; break;
          }
        }break;
        case GETNUM: {
          switch(type) {
            case DIGIT:
              state = GETNUM; break;
            case CHAR:
              state = REJECT; break;
            case DOT:
              state = GETDOT; break;
          }
        }break;
        case GETDOT: {
          switch(type) {
            case DIGIT:
              state = ACCEPT; break;
            case CHAR:
            case DOT:
              state = REJECT; break;
          }
        }break;
        case ACCEPT: {
          switch(type) {
            case DIGIT:
              state = ACCEPT; break;
            case CHAR:
            case DOT:
              state = REJECT; break;
          }
        }break;
        case REJECT: break;
      }
      if (state == REJECT) {
        break;
      }
    }
    if (state == ACCEPT) {
      ret = TYPE_DOUBLE;
    } else if (state == GETNUM) {
      ret = TYPE_INT;
    }
  }

  return ret;
}
