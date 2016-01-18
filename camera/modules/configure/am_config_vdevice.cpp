/*******************************************************************************
 * am_config_vdevice.cpp
 *
 * Histroy:
 *  2012-3-8 2012 - [ypchang] created file
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
#include "am_config_vdevice.h"

static const char* vout_type_to_str[] = {
  "None", "Lcd", "Hdmi", "Cvbs"
};

static const char* vin_type_to_str[] = {
  "None", "RGB", "YUV"
};

AmConfigVDevice::AmConfigVDevice(const char *configFileName)
  : AmConfigBase(configFileName),
    mVdevParameters(NULL)
{
}

AmConfigVDevice::~AmConfigVDevice()
{
  delete mVdevParameters;
  DEBUG("AmConfigVDevice deleted!");
}

VDeviceParameters* AmConfigVDevice::get_vdev_config()
{
  VDeviceParameters* ret = NULL;
  if (init()) {
    if (!mVdevParameters) {
      mVdevParameters = new VDeviceParameters;
    }
    if (mVdevParameters) {
      mVdevParameters->vout_number =
        (uint32_t)get_int("AMVDEVICE:VoutNumber", 2);
        if (mVdevParameters->vout_number != 2) {
        WARN("VOUT devices must be set to 2, reset!");
        mVdevParameters->vout_number = 2;
      }
      mVdevParameters->vout_list = new VoutType[mVdevParameters->vout_number];
      for (uint32_t i = 0; i < mVdevParameters->vout_number; ++ i) {
        char voutDev[32] = {0};
        char *str = NULL;
        sprintf(voutDev, "AMVDEVICE:Vout%u", i);
        str = get_string(voutDev, "None");
        if (is_str_equal(str, "None")) {
          mVdevParameters->vout_list[i] = AM_VOUT_TYPE_NONE;
        } else {
          if (i == 0) {
            mVdevParameters->vout_list[i] = (is_str_equal(str, "None") ?
            AM_VOUT_TYPE_NONE : (is_str_equal(str, "Lcd") ?
            AM_VOUT_TYPE_LCD :
            (WARN("%s not supported by Vout0! Disabled", str),
             AM_VOUT_TYPE_NONE)));
          } else if (i == 1) {
            mVdevParameters->vout_list[i] = (is_str_equal(str, "None") ?
            AM_VOUT_TYPE_NONE : (is_str_equal(str, "Hdmi") ?
            AM_VOUT_TYPE_HDMI : (is_str_equal(str, "Cvbs") ?
            AM_VOUT_TYPE_CVBS :
            (WARN("%s is not supported by Vout1! Disabled", str),
             AM_VOUT_TYPE_NONE))));
          } else {
            ERROR("No such device!");
            mVdevParameters->vout_list[i] = AM_VOUT_TYPE_NONE;
          }
        }
      }
      mVdevParameters->vin_number =
        (uint32_t)get_int("AMVDEVICE:VinNumber", 1);
      if (mVdevParameters->vin_number <= 0) {
        WARN("Invalid VIN number: %d, at least one VIN device must exist, "
            "reset VinNumber to 1!", mVdevParameters->vin_number);
        mVdevParameters->vin_number = 1;
      }
      mVdevParameters->vin_list = new VinType[mVdevParameters->vin_number];
      for (uint32_t i = 0; i < mVdevParameters->vin_number; ++ i) {
        char vinDev[32] = {0};
        char *str = NULL;
        sprintf(vinDev, "AMVDEVICE:Vin%u", i);
        str = get_string(vinDev, "RGB");
        mVdevParameters->vin_list[i] = (is_str_equal(str, "RGB") ?
          AM_VIN_TYPE_RGB : (is_str_equal(str, "YUV") ? AM_VIN_TYPE_YUV :
                    (WARN("Unknown VIN type %s, reset to RGB"), AM_VIN_TYPE_RGB)));

        mVdevParameters->stream_number =
            (uint32_t) get_int("AMVDEVICE:StreamNumber", MAX_ENCODE_STREAM_NUM);
        if (mVdevParameters->stream_number > MAX_ENCODE_STREAM_NUM) {
          WARN("Invalid Stream number: %d.  Max number is %d. "
              "Reset StreamNumber to %d!",
              mVdevParameters->stream_number, MAX_ENCODE_STREAM_NUM,
              MAX_ENCODE_STREAM_NUM);
          mVdevParameters->stream_number = MAX_ENCODE_STREAM_NUM;
        }
      }
    }
    ret = mVdevParameters;
  }

  return ret;
}

void AmConfigVDevice::set_vdev_config(VDeviceParameters* vdevConfig)
{
  if (init()) {
    if (vdevConfig) {
      set_value("AMVDEVICE:VoutNumber", vdevConfig->vout_number);
      for (uint32_t i = 0; i < vdevConfig->vout_number; ++ i) {
        char dev[32] = {0};
        sprintf(dev, "AMVDEVICE:Vout%u", i);
        set_value(dev, vout_type_to_str[vdevConfig->vout_list[i]]);
      }

      set_value("AMVDEVICE:VinNumber", vdevConfig->vin_number);
      for (uint32_t i = 0; i < vdevConfig->vin_number; ++ i) {
        char dev[32] = {0};
        sprintf(dev, "AMVDEVICE:Vin%u", i);
        set_value(dev, vin_type_to_str[vdevConfig->vin_list[i]]);
      }
    }
    save_config();
  }
}

