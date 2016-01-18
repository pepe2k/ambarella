/*******************************************************************************
 * am_network_manager.cpp
 *
 * History:
 *   2012-11-6 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <dbus/dbus-glib.h>
#include <nm-utils.h>
#include <nm-client.h>
#include <nm-access-point.h>
#include <nm-remote-settings.h>

#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_network.h"
#include "am_network_manager_priv.h"


static const char *netstate_to_string[] =
{
  "Unknown",
  "Asleep",
  "Connecting",
  "Connected (Local Only)",
  "Connected (Site Only)",
  "Connected (Global)",
  "Disconnected"
};

static const char *devstate_to_string[] =
{
  "Unknown",
  "Unmanaged",
  "Unavailable",
  "Disconnected",
  "Connecting (Prepare)",
  "Connecting (Configuring)",
  "Connecting (Need Authentication)",
  "Connecting (Getting IP Configuration)",
  "Connecting (Checking IP Connectivity)",
  "Connecting (Starting Dependent Connections)",
  "Connected",
  "Disconnecting",
  "Failed"
};

AmNetworkManager::AmNetworkManager() :
  mNmPriv(NULL)
{
  mNmPriv = new AmNetworkManager_priv();
}

AmNetworkManager::~AmNetworkManager()
{
  delete mNmPriv;
}

const char* AmNetworkManager::net_state_to_string(
    AmNetworkManager::NetworkState state)
{
  return netstate_to_string[state];
}

const char* AmNetworkManager::device_state_to_string(
    AmNetworkManager::DeviceState state)
{
  return devstate_to_string[state];
}

AmNetworkManager::NetworkState AmNetworkManager::get_network_state()
{
  return (mNmPriv ? mNmPriv->get_network_state() :
                    AmNetworkManager::AM_NM_STATE_UNKNOWN);
}

AmNetworkManager::DeviceState AmNetworkManager::get_device_state(
    const char *iface)
{
  return (mNmPriv ? mNmPriv->get_device_state(iface) :
                    AmNetworkManager::AM_DEV_STATE_UNAVAILABLE);
}

uint32_t AmNetworkManager::get_current_device_speed(const char *iface)
{
  return mNmPriv ? mNmPriv->get_current_device_speed(iface) : 0;
}

int AmNetworkManager::wifi_list_ap(APInfo **infos)
{
  return mNmPriv ? mNmPriv->wifi_list_ap(infos) : 0;
}

int AmNetworkManager::wifi_connect_ap(const char *apName, const char* password)
{
  return mNmPriv ? mNmPriv->wifi_connect_ap(apName, password) :  (-1);
}

int AmNetworkManager::wifi_connect_ap_static(const char *apName, const char* password, NetInfoIPv4 *ipv4)
{
  return mNmPriv ? mNmPriv->wifi_connect_ap_static(apName, password, ipv4) :  (-1);
}

int AmNetworkManager::wifi_connect_hidden_ap(const char *apName, const char* password,
     const AmNetworkManager::APSecurityFlags security, uint32_t wep_index)
{
  return mNmPriv ? mNmPriv->wifi_connect_hidden_ap(apName, password, security, wep_index) : (-1);
}

int AmNetworkManager::wifi_connect_hidden_ap_static(const char *apName, const char* password,
     const AmNetworkManager::APSecurityFlags security, uint32_t wep_index, NetInfoIPv4 *ipv4)
{
  return mNmPriv ? mNmPriv->wifi_connect_hidden_ap_static(apName, password, security, wep_index, ipv4) : (-1);
}

int AmNetworkManager::wifi_setup_adhoc(const char *name, const char*password, const AmNetworkManager::APSecurityFlags security)
{
  return mNmPriv ? mNmPriv->wifi_setup_adhoc(name, password, security) : (-1);
}

int AmNetworkManager::wifi_setup_ap(const char *name, const char*password, const AmNetworkManager::APSecurityFlags security, int channel)
{
  return mNmPriv ? mNmPriv->wifi_setup_ap(name, password, security, channel) : (-1);
}

int AmNetworkManager::wifi_connection_up(const char* ap_name)
{
  return mNmPriv ? mNmPriv->wifi_connection_up(ap_name) : (-1);
}

int AmNetworkManager::wifi_connection_delete(const char* ap_name)
{
  return mNmPriv ? mNmPriv->wifi_connection_delete(ap_name) : false;
}

bool AmNetworkManager::wifi_get_active_ap(APInfo **infos)
{
  return mNmPriv ? mNmPriv->wifi_get_active_ap(infos) : false;
}

bool AmNetworkManager::disconnect_device(const char *iface)
{
  return mNmPriv ? (mNmPriv->disconnect_device(iface) == 0) : false;
}

bool AmNetworkManager::wifi_set_enabled(bool enable)
{
  return mNmPriv ? mNmPriv->wifi_set_enabled(enable) : false;
}

bool AmNetworkManager::network_set_enabled(bool enable)
{
  return mNmPriv ? mNmPriv->network_set_enabled(enable) : false;
}
