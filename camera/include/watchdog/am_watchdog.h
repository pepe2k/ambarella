/*
 * am_watchdog.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 27/12/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#define	WATCHDOG_IOCTL_BASE 'W'

struct watchdog_info {
    uint32_t options;          /* Options the card/driver supports */
    uint32_t firmware_version; /* Firmware version of the card */
    uint8_t  identity[32];     /* Identity of the board */
};

#define	WDIOC_GETSUPPORT    _IOR(WATCHDOG_IOCTL_BASE, 0, struct watchdog_info)
#define	WDIOC_GETSTATUS     _IOR(WATCHDOG_IOCTL_BASE, 1, int)
#define	WDIOC_GETBOOTSTATUS _IOR(WATCHDOG_IOCTL_BASE, 2, int)
#define	WDIOC_GETTEMP       _IOR(WATCHDOG_IOCTL_BASE, 3, int)
#define	WDIOC_SETOPTIONS    _IOR(WATCHDOG_IOCTL_BASE, 4, int)
#define	WDIOC_KEEPALIVE     _IOR(WATCHDOG_IOCTL_BASE, 5, int)
#define	WDIOC_SETTIMEOUT    _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define	WDIOC_GETTIMEOUT    _IOR(WATCHDOG_IOCTL_BASE, 7, int)
#define	WDIOC_SETPRETIMEOUT _IOWR(WATCHDOG_IOCTL_BASE, 8, int)
#define	WDIOC_GETPRETIMEOUT _IOR(WATCHDOG_IOCTL_BASE, 9, int)
#define	WDIOC_GETTIMELEFT   _IOR(WATCHDOG_IOCTL_BASE, 10, int)

#define ID_DIR   "/var/run/msg"
#define ID_PATH  "/var/run/msg/wdt_id"
#define MSG_TYPE 3

typedef enum {
   WATCHDOG_REQUEST_FEED   = 0,
   WATCHDOG_REQUEST_REBOOT = 1,
   WATCHDOG_REQUEST_UNKNOWN = 2,
} WATCHDOG_REQUEST_TYPE;

typedef struct watchdog_msg {
   int type;
   WATCHDOG_REQUEST_TYPE request;
} watchdog_msg_t;

extern int reboot_system_on_error ();

#endif

