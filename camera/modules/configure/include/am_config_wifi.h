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
#ifndef __AM_CONFIG_WIFI_H__
#define __AM_CONFIG_WIFI_H__

class AmConfigWifi: public AmConfigBase
{
public:
   AmConfigWifi(const char *configFileName) :
      AmConfigBase(configFileName),
      mWifiParameters(NULL){}
   virtual ~AmConfigWifi()
   {
      delete mWifiParameters;
   }

public:
   WifiParameters *get_wifi_config();
   void set_wifi_config(WifiParameters *config);

private:
   WifiParameters *mWifiParameters;
};

#endif
