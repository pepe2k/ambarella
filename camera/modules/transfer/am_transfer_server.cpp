/*
 * am_transfer_server.cpp
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
#include "transfer/am_transfer_server.h"
#include "transfer/version.h"

#ifndef WRITE_RETRY_TIMES
#define WRITE_RETRY_TIMES 10
#endif

struct sockaddr_un AmTransferServer::addr;
int AmTransferServer::socket_fd = -1;
int AmTransferServer::connect_fd[] = {-1};
int AmTransferServer::connect_num  = 0;
pthread_mutex_t AmTransferServer::mutex;

static const char *client_index[] = {
   "first", "second", "third", "forth"
};

AmTransferServer::AmTransferServer() :
  accept_thread_id(0) {}
AmTransferServer::~AmTransferServer () {}

AmTransferServer *AmTransferServer::Create ()
{
   AmTransferServer *result = new AmTransferServer ();
   if (result && result->Construct () != 0) {
      delete result;
      result = NULL;
   }

   return result;
}

int AmTransferServer::Construct ()
{
   if ((socket_fd = socket (PF_UNIX, SOCK_SEQPACKET, 0)) < 0) {
      NOTICE ("Failed to create a socket!\n");
      return -1;
   }

   if (access (TRANSFER_COMMUNICATE_PATH, F_OK) == 0) {
      unlink (TRANSFER_COMMUNICATE_PATH);
   }

   memset (&addr, 0, sizeof (struct sockaddr_un));

   addr.sun_family = AF_UNIX;
   snprintf (addr.sun_path, UNIX_PATH_MAX, TRANSFER_COMMUNICATE_PATH);

   return 0;
}

int AmTransferServer::Start ()
{
   if (socket_fd == -1) {
      ERROR ("Socket file descriptor is not initialized!");
      return -1;
   }

   if (bind (socket_fd,
             (struct sockaddr *)&addr,
             sizeof (struct sockaddr_un)) != 0) {
      ERROR ("Failed to bind socket!");
      close (socket_fd);
      return -1;
   }

   if (listen (socket_fd, 5) != 0) {
      ERROR ("Failed to listen socket!");
      close (socket_fd);
      return -1;
   }

   pthread_mutex_init (&mutex, NULL);
   if (pthread_create (&accept_thread_id, NULL,
            &AmTransferServer::connection_accept, NULL) < 0) {
      ERROR ("Failed to create accept thread!");
      close (socket_fd);
      return -1;
   }

   return 0;
}

int AmTransferServer::Stop ()
{
   if (shutdown (socket_fd, SHUT_RD) < 0) {
      ERROR ("Failed to end reading operation on socket_fd: %s",
             strerror(errno));
      return -1;
   }

   if (accept_thread_id != 0) {
     pthread_join (accept_thread_id, NULL);
   }

   close (socket_fd);
   unlink (TRANSFER_COMMUNICATE_PATH);
   pthread_mutex_destroy (&mutex);

   return 0;
}

void *AmTransferServer::connection_accept (void *arg)
{
   int       connection_fd;
   socklen_t addr_len = sizeof (struct sockaddr);

   while (true) {
      if ((connection_fd = accept (socket_fd,
                                   (struct sockaddr *)&addr,
                                   &addr_len)) < 0) {
         NOTICE ("Server will be exit!");
         break;
      }

      pthread_mutex_lock (&mutex);
      if (connect_num == MAX_REQUEST_NUM) {
         NOTICE ("Clients has been reached limit, ignore this client!");
         pthread_mutex_unlock (&mutex);
         close (connection_fd);
      } else {
         connect_fd[connect_num++] = connection_fd;
         NOTICE ("%s client has been connected!", client_index[connect_num - 1]);
         pthread_mutex_unlock (&mutex);
      }
   }

   return NULL;
}

/*
 * Send packet to all clients.
 *
 * Maybe client has quited and data can't sent successfully.
 * To handle such situation, we will disable this connection.
 */
int AmTransferServer::SendPacket (AmTransferPacket *packet)
{
   int count = 0;
   int left  = 0;
   int sent  = 0;
   int retry_times = 0;
   int error_flag = 0;

   if (socket_fd == -1) {
      ERROR ("Socket file descriptor was not initialized!");
      return -1;
   }

   if (!packet) {
      ERROR ("Invalid argument: NULL pointer!");
      return -1;
   }



   pthread_mutex_lock (&mutex);
   for (int i = 0; i < connect_num; i++) {
      sent = retry_times = 0;
      left = sizeof (AmTransferPacketHeader) + packet->header.dataLen;

      /*
       * Try to send all data to a certain client. Sometimes, it
       * is very busy that write system call can not write data
       * successfully just one time, so we need to check return
       * value of write. If we encounter this situation, we need
       * to try to sent rest data in several times.
       */
      do {
         if ((count = write (connect_fd[i],
                            (void *)(packet + sent),
                            left)) <= 0) {
            if (count <= 0 && (errno == EINTR  ||
                              errno == EAGAIN ||
                              errno == EWOULDBLOCK)) {
               NOTICE ("System is busying now, trg again!");
               continue;
            } else {
               error_flag = 1;
               NOTICE ("Failed to send %d bytes to client: %s",
                       left, strerror (errno));
            }
         }

         if (++retry_times >= WRITE_RETRY_TIMES || error_flag) {
            if (!error_flag) {
               ERROR ("Data was't sent successfully in %d times!", retry_times);
            }

            for (int j = i; j < connect_num - 1; j++) {
               connect_fd[j] = connect_fd[j + 1];
            }

            connect_fd[connect_num] = -1;
            connect_num -= 1;
            break;
         }

         left -= count;
         sent += count;
      } while (left > 0);

      retry_times = 0;
   }

   pthread_mutex_unlock (&mutex);
   return 0;
}
