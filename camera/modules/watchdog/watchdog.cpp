/*
 * watchdog.cpp
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
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/ioctl.h>

#include "am_watchdog.h"

int reboot_system_on_error ()
{
   int fd, msg_id;
   watchdog_msg_t msg;

   if ((fd = open (ID_PATH, O_RDONLY)) < 0) {
      printf ("Failed to open %s: %s",
            ID_PATH,
            strerror (errno));
   }

   if (read (fd, (void *)&msg_id,
            sizeof (int)) != sizeof (int)) {
      printf ("Failed to read %d bytes from %s: %s",
            sizeof (int),
            ID_PATH,
            strerror (errno));
      close(fd);
      return -1;
   }

   close (fd);

   msg.type = MSG_TYPE;
   msg.request = WATCHDOG_REQUEST_REBOOT;

   return msgsnd (msg_id, &msg, sizeof (msg), 0);
}
