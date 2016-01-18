/*******************************************************************************
 * am_network_config.cpp
 *
 * History:
 *   2013-1-7 - [Tao Wu] created file
 *
 * Copyright (C) 2008-2013, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <net/if.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <dbus/dbus-glib.h>
#include <nm-utils.h>
#include <nm-client.h>
#include <nm-device-ethernet.h>
#include <nm-device-wifi.h>
#include <nm-device-wimax.h>

#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_network.h"

#ifdef ADD
#undef ADD
#endif
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

/* Paths */
#define PROC_NET_WIRELESS "/proc/net/wireless"
#define PROC_NET_DEV      "/proc/net/dev"
#define PROC_NET_ROUTE    "/proc/net/route"
#define LO_INTERFACE      "lo"

#define RTF_UP          0x0001 /* route usable */

int AmNetworkConfig::iw_sockets_open(void)
{
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (AM_UNLIKELY(sock < 0)) {
    PERROR("socket");
  }
  return sock;
}

char* AmNetworkConfig::iw_get_ifname(char *name, int nsize, char * buf)
{
  char *end;
  while (isspace(*buf)) {
    buf ++;
  }
  end = strrchr(buf, ':');
  if ((end == NULL) || (((end - buf) + 1) > nsize)) {
    return NULL;
  }
  memcpy(name, buf, (end - buf));
  name[end - buf] = '\0';

  return name;
}

void AmNetworkConfig::iw_enum_devices(int skfd)
{
  char buff[1024] = {0};
  FILE *fh = fopen(PROC_NET_DEV, "r");

  if (fh != NULL) {
    fgets(buff, sizeof(buff), fh);
    fgets(buff, sizeof(buff), fh);
    while (fgets(buff, sizeof(buff), fh)) {
      char name[IFNAMSIZ + 1] = {0};
      if (AM_LIKELY(!((buff[0] == '\0') || (buff[1] == '\0')))) {
        if (AM_UNLIKELY(!iw_get_ifname(name, sizeof(name), buff))) {
          ERROR("Cannot parse " PROC_NET_DEV " ");
        } else {
          if (AM_UNLIKELY(!is_str_same(name, LO_INTERFACE))) {
            strncpy(mInterface[mCount ++], name, strlen(name));
          }
        }
      }
      memset(buff, 0, sizeof(buff));
    }
    fclose(fh);
  } else {
    struct ifconf ifc;
    struct ifreq *ifr = NULL;

    memset(&ifc, 0, sizeof(ifc));
    ifc.ifc_len = sizeof(buff);
    ifc.ifc_buf = buff;
    if (AM_UNLIKELY(ioctl(skfd, SIOCGIFCONF, &ifc) < 0)) {
      ERROR("SIOCGIFCONF: %s");
    } else {
      ifr = ifc.ifc_req;
      for (int32_t i = ifc.ifc_len / sizeof(struct ifreq); -- i >= 0; ifr ++) {
        strncpy(mInterface[mCount ++], ifr->ifr_name, strlen(ifr->ifr_name));
      }
    }
  }
}

bool AmNetworkConfig::init(void)
{
  if (AM_LIKELY(false == mIsInited)) {
    mSock = iw_sockets_open();
    if (AM_LIKELY(mSock < 0)) {
      ERROR("init failed");
    } else {
      iw_enum_devices(mSock);
      mIsInited = true;
    }
  }
  return mIsInited;
}

void AmNetworkConfig::finish(void)
{
  if (AM_LIKELY(mSock >= 0)) {
    close(mSock);
  }
  mCount = 0;
  mIsInited = false;
}

AmNetworkConfig::AmNetworkConfig() :
    mSock(0),
    mCount(0),
    mIsInited(false)
{
  for (uint32_t i = 0; i < MAXIFACE; ++ i) {
    memset(&mInterface[i], 0, IFNAMSIZ + 1);
  }
}

AmNetworkConfig::~AmNetworkConfig()
{
  finish();
}

bool AmNetworkConfig::check_socket(void)
{
  if (AM_UNLIKELY(mSock < 0)) {
    ERROR("Socket is not initialized!");
  }
  return (mSock >= 0);
}

bool AmNetworkConfig::get_default_connection(NetDeviceInfo **info)
{
  bool found = false;

  if (AM_LIKELY(init() && mCount)) {
    NetDeviceInfo **devInfo = info;
    INFO("Found %u network %s!", mCount, (mCount > 1 ? "devices" : "device"));

    for (uint32_t i = 0; i < mCount; ++ i) {
      char mac[32] = {0};
      while (*devInfo != NULL) {
        devInfo = &((*devInfo)->info_next);
      }
      *devInfo = new NetDeviceInfo();
      (*devInfo)->set_netdev_name(mInterface[i]);
      (*devInfo)->is_default = is_default_connection(mInterface[i]);
      if (get_mac_address(mInterface[i], mac, sizeof(mac))) {
        (*devInfo)->set_netdev_mac(mac);
      }
      (*devInfo)->set_netdev_mtu(get_mtu_size(mInterface[i]));

      if (AM_UNLIKELY(!get_ipv4_details(mInterface[i], *(*devInfo)))) {
        WARN("Failed to get detailed IPv4 information of device %s, "
             "remove %s from device list!",
             mInterface[i], mInterface[i]);
        delete *devInfo;
        *devInfo = NULL;
      } else {
        found = true;
      }
    }
  }

  return found;
}

bool AmNetworkConfig::get_ipv4_details(char *iface, NetDeviceInfo &info)
{
  bool ret = false;
  uint32_t addr = (uint32_t)-1;
  uint32_t mask = (uint32_t)-1;
  if (AM_LIKELY(get_address(iface, &addr) &&
                get_netmask(iface, &mask))) {
    NetInfoIPv4 *ipv4 = new NetInfoIPv4();

    if (AM_LIKELY(ipv4)) {
      DnsIPv4 *dns = NULL;
      uint32_t gate = (uint32_t)-1;
      ipv4->set_address(addr);
      ipv4->set_netmask(mask);
      if (AM_LIKELY(get_gateway(iface, &gate))) {
        ipv4->set_gateway(gate);
      } else {
        ipv4->set_gateway((uint32_t)0);
      }
      if (AM_LIKELY(get_dns(&dns))){
        ipv4->add_dns(dns);
      }
      info.add_netdev_ipv4(ipv4);
      ret = true;
    }
  }

  return ret;
}

bool AmNetworkConfig::is_default_connection(char * iface)
{
  bool ret = false;
  FILE *fp = fopen(PROC_NET_ROUTE, "r");

  if (fp == NULL) {
    PERROR("fopen");
    return ret;
  }

  if (fscanf(fp, "%*[^\n]\n") < 0) { /* Skip the first line. */
    PERROR("fscanf");
    fclose(fp);
    return ret; /* Empty or missing line, or read error. */
  }
  while (1) {
    char devname[64] = {0};
    unsigned long d = 0;
    unsigned long g = 0;
    unsigned long m = 0;
    int flgs = 0;
    int ref = 0;
    int use = 0;
    int metric = 0;
    int mtu = 0;
    int win = 0;
    int ir = 0;

    int r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
               devname, &d, &g, &flgs, &ref, &use,
               &metric, &m, &mtu, &win, &ir);

    if (AM_UNLIKELY(r != 11)) {
      if (AM_UNLIKELY((r < 0) && feof(fp))) {
        /* EOF with no (nonspace) chars read. */
        break;
      }
      PERROR("fscanf");
    }
    if (!(flgs & RTF_UP)) {
      /* Skip interfaces that are down. */
      continue;
    }
    if (d == INADDR_ANY ) {
      if (is_str_same(devname, iface)) {
        INFO("%s is default", devname);
        ret = true;
      }
      break;
    }
  }
  fclose(fp);
  return ret;
}

bool AmNetworkConfig::get_mac_address(char * iface, char *mac, uint32_t size)
{
  bool ret = false;
  if (AM_LIKELY(iface && mac && (size >= 32) && check_socket())) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, strlen(iface));

    if (AM_LIKELY(ioctl(mSock, SIOCGIFHWADDR, &ifr) >= 0)) {
      sprintf(mac,
              "%02X:%02X:%02X:%02X:%02X:%02X",
              (unsigned char) ifr.ifr_hwaddr.sa_data[0],
              (unsigned char) ifr.ifr_hwaddr.sa_data[1],
              (unsigned char) ifr.ifr_hwaddr.sa_data[2],
              (unsigned char) ifr.ifr_hwaddr.sa_data[3],
              (unsigned char) ifr.ifr_hwaddr.sa_data[4],
              (unsigned char) ifr.ifr_hwaddr.sa_data[5]);
      ret = true;
      DEBUG("%s : MAC=%s", iface, mac);
    }
  }

  return ret;
}

uint32_t AmNetworkConfig::get_mtu_size(char *iface)
{
  uint32_t mtu = 1500;
  if (AM_LIKELY(check_socket())) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, strlen(iface));
    if (AM_LIKELY(ioctl(mSock, SIOCGIFMTU, &ifr) >= 0)) {
      mtu = ifr.ifr_ifru.ifru_mtu;
      DEBUG(" %s : MTU=%d", iface, mtu);
    }
  }
  return mtu;
}

bool AmNetworkConfig::get_address(char *iface, uint32_t *addr)
{
  bool ret = false;
  if (AM_LIKELY(iface && addr && check_socket())) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, strlen(iface));
    if (AM_LIKELY(ioctl(mSock, SIOCGIFADDR, &ifr) >= 0)) {
      *addr = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;
      memcpy(&addr, &ifr.ifr_addr, sizeof(addr));
      ret = true;
      DEBUG(" %s : IP Address=%s",
            iface, inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));
    }
  }

  return ret;
}

bool AmNetworkConfig::get_netmask(char *iface, uint32_t *mask)
{
  bool ret = false;

  if (iface && mask && check_socket()) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, iface, strlen(iface));
    if (AM_LIKELY(ioctl(mSock, SIOCGIFNETMASK, &ifr) >= 0)) {
      *mask = ((struct sockaddr_in*)&ifr.ifr_netmask)->sin_addr.s_addr;
      ret = true;
      DEBUG(" %s : Netmask=%s", iface,
            inet_ntoa(((struct sockaddr_in*)&ifr.ifr_netmask)->sin_addr));
    }
  }

  return ret;
}

bool AmNetworkConfig::get_gateway(char *iface, uint32_t *gw)
{
  bool ret = false;

  if (AM_LIKELY(iface && gw)) {
    FILE *fp = fopen(PROC_NET_ROUTE, "r");
    if (fp == NULL) {
      PERROR("fopen");
      return ret;
    }

    if (fscanf(fp, "%*[^\n]\n") < 0) { /* Skip the first line. */
      PERROR("fscanf");
      fclose(fp);
      return ret; /* Empty or missing line, or read error. */
    }
    while (1) {
      char devname[64] = {0};
      unsigned long d = 0;
      unsigned long g = 0;
      unsigned long m = 0;
      int flgs = 0;
      int ref = 0;
      int use = 0;
      int metric = 0;
      int mtu = 0;
      int win = 0;
      int ir = 0;

      int r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                     devname, &d, &g, &flgs, &ref, &use,
                     &metric, &m, &mtu, &win, &ir);
      if (AM_UNLIKELY(r != 11)) {
        if (AM_UNLIKELY((r < 0) && feof(fp))) {
          /* EOF with no (nonspace) chars read. */
          break;
        }
        PERROR("fscanf");
      }
      if (AM_UNLIKELY(!(flgs & RTF_UP))) {
        /* Skip interfaces that are down. */
        continue;
      }
      if (is_str_same(devname, iface) && (g > 0)) {
        *gw = g;
        ret = true;
        break;
      }
    }

    fclose(fp);
  }

  return ret;
}

bool AmNetworkConfig::get_dns(DnsIPv4 **dns)
{
  extern struct __res_state _res;
  bool ret = false;

  if (AM_LIKELY(dns)) {
    if (AM_UNLIKELY(res_init() != 0)) {
      PERROR("res_init");
    } else {
      DnsIPv4 **temp = dns;
      for (int32_t i = 0; i < _res.nscount; ++ i) {
        (*temp) = new DnsIPv4();
        (*temp)->dns_next = NULL;
        (*temp)->set_dns(_res.nsaddr_list[i].sin_addr.s_addr);
        temp = &((*temp)->dns_next);
      }
      ret = (*dns != NULL);
    }
  }

  return ret;
}

void AmNetworkConfig::show_info(void)
{
  DEBUG("mSock=%d, mCount=%d", mSock, mCount);
  for (uint32_t i = 0; i < mCount; i ++) {
    char mac[32] = {0};
    INFO("Interface[%d] = %s", i, mInterface[i]);
    if (get_mac_address(mInterface[i], mac, sizeof(mac))) {
      INFO("MAC = %s", mac);
    }
    INFO("MTU = %u", (get_mtu_size(mInterface[i])));
  }
}

