/*
 * wdt_daemon.c
 *
 * History:
 *	2012/11/22 - [Zhong Xu] create this file
 *	2012/11/23 - [Bo YU] modify some type and clean
 * 2012/12/27 - [Hanbo Xiao] modified
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "am_watchdog.h"

int main(int argc, char **argv)
{
   int fd, ret, msg_id, tmpfd;
   watchdog_msg_t msgs;

#ifdef EXIT_WITH_DISABLE_WDT
   char *buf = "V";
#endif

   if ((msg_id = msgget (IPC_PRIVATE, 0666 | IPC_CREAT)) < 0) {
      printf ("Failed to create msgID %d, %s",
            msg_id,
            strerror (errno));
      return -1;
   }

   printf("id is %d\n", msg_id);

   if (access (ID_DIR, F_OK) != 0) {
      /* /var/run/msg doesn't exist, we need to create it. */
      if (mkdir (ID_DIR, 0666) < 0) {
         printf ("Failed to create %s: %s",
               ID_DIR,
               strerror (errno));
         return -1;
      }
   }

   if ((tmpfd = open (ID_PATH, O_RDWR | O_CREAT | O_TRUNC)) < 0) {
      printf ("Failed to open %s: %s",
            ID_PATH,
            strerror (errno));
      return -1;
   }

   if(write (tmpfd, &msg_id, 4) != 4) {
      perror ("Write failed\n");
      close (tmpfd);
      return -1;
   }

   close(tmpfd);

   if ((fd = open("/dev/watchdog", O_RDWR)) < 0) {
      printf("Open error\n");
      return -1;
   }

#ifdef EXIT_WITH_DISABLE_WDT
   write (fd, buf, sizeof(buf));
#endif

   while(1) {
      ret = msgrcv (msg_id, (struct msgbuf*)&msgs,
                 sizeof(watchdog_msg_t), MSG_TYPE, IPC_NOWAIT);
      if ((ret < 0) && (errno == ENOMSG)) {
         /*
          * No message received in message queue. That indicates
          * no errors happen and there is no need to reboot. So,
          * we have to feed watch dog
          */
         ioctl (fd, WDIOC_KEEPALIVE, 0);
      } else if (ret < 0) {
         printf ("Error happens on receiving msg: %s\n", strerror (errno));
      }

      if (msgs.request == WATCHDOG_REQUEST_REBOOT) {
         /* Fatal error happens, we need to reboot our system. */
         printf ("System is going to reboot ... ");
         break;
      } else {
         ioctl(fd, WDIOC_KEEPALIVE, 0);
      }

      sleep (1);
   }

   close(fd);
   return 0;
}
