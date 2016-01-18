/*
 * am_transfer_client.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 21/11/2013 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "am_include.h"
#include "am_utility.h"
#include "transfer/am_data_transfer.h"
#include "transfer/am_transfer_client.h"
#include "transfer/version.h"

AmTransferClient::AmTransferClient () :
   socket_fd (-1)
{}

AmTransferClient::~AmTransferClient ()
{
   if (socket_fd >= 0) {
      close (socket_fd);
   }
}

AmTransferClient *AmTransferClient::Create ()
{
   AmTransferClient *result = new AmTransferClient ();
   if (result && result->Construct () != 0) {
      delete result;
      result = NULL;
   }

   return result;
}

int AmTransferClient::Construct ()
{
   if (access (TRANSFER_COMMUNICATE_PATH, F_OK) != 0) {
      ERROR ("Server doesn't run: /tmp/am_datatransfer doesn't exist!");
      return -1;
   }

   if ((socket_fd = socket (PF_UNIX, SOCK_SEQPACKET, 0)) < 0) {
      ERROR ("Failed to create a socket!");
      return -1;
   }

   memset (&addr, 0, sizeof (sockaddr_un));

   addr.sun_family = AF_UNIX;
   snprintf (addr.sun_path, UNIX_PATH_MAX, TRANSFER_COMMUNICATE_PATH);

   if (connect (socket_fd,
               (struct sockaddr *)&addr,
                sizeof (struct sockaddr_un)) < 0) {
      ERROR ("Failed to connect server: %s", strerror (errno));
      return -1;
   }

   return 0;
}

int AmTransferClient::ReceivePacket (AmTransferPacket *packet)
{
   int len = 0;

   if (socket_fd == -1) {
      ERROR ("Socket file descriptor was not initialized!");
      return -1;
   }

   if (!packet) {
      ERROR ("Invalid argument: NULL pointer!");
      return -1;
   }

   len = read (socket_fd, (void *)packet, sizeof (AmTransferPacket));
   if (len != packet->header.dataLen + (int)sizeof (AmTransferPacketHeader)) {
      ERROR ("Error on read packet's header real data from socket!");
      return -1;
   }

   return 0;
}

int AmTransferClient::Close ()
{
   /* End reading operation from socket_fd */
   if (shutdown (socket_fd, 0) < 0) {
      ERROR ("Failed to end reading operation from socket_fd");
      return -1;
   }

   close (socket_fd);
   return 0;
}
