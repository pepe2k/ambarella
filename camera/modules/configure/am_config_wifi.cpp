/*
 * am_config_wifi.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 23/11/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
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
#include "am_config_wifi.h"

WifiParameters* AmConfigWifi::get_wifi_config()
{
  WifiParameters *ret = NULL;
  if (init()) {
    if (!mWifiParameters) {
      mWifiParameters = new WifiParameters();
    }
    if (mWifiParameters) {
      mWifiParameters->\
      set_wifi_mode (get_string("WIFI:Mode", "DHCP"));

      mWifiParameters->\
      set_wifi_ssid (get_string("WIFI:Ssid", NULL));

      mWifiParameters->\
      set_wifi_key (get_string("WIFI:Key", NULL));

      if (mWifiParameters->config_changed) {
        set_wifi_config(mWifiParameters);
      }
    }
    ret = mWifiParameters;
  }

  return ret;
}

void AmConfigWifi::set_wifi_config(WifiParameters *config)
{
  if (AM_LIKELY(config)) {
    if (init()) {
      set_value("WIFI:Mode", config->wifi_mode);
      set_value("WIFI:Ssid", config->wifi_ssid);
      set_value("WIFI:Key", config->wifi_key);
      config->config_changed = 0;
      save_config();
    } else {
      WARN("Failed openint %s, wifi configuration NOT saved!", mConfigFile);
    }
  }
}
