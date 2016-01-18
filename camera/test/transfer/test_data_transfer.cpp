/*
 * test_datatransfer.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 24/07/2013 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_include.h"
#include "am_utility.h"
#include "transfer/am_data_transfer.h"
#include "transfer/am_transfer_client.h"

#include <sys/time.h>
#include <signal.h>

#define FILE_NAME_LEN_MAX 256
#define STREAM_NUMBER_MAX 4
#define DATE_FMT_LEN_MAX  128

char   filename[FILE_NAME_LEN_MAX];
int    raw_fd[2 * STREAM_NUMBER_MAX + 2];
int    g_running = true;
static const char *suffix[] = {
   NULL,
   "h264",
   "jpeg",
   "pcm",
   "g711",
   "g726-40",
   "g726-32",
   "g726-24",
   "g726-16",
   "aac",
   "opus",
   "bpcm",
};

static int fetch_file_name (int streamId, int dataType)
{
   char date[DATE_FMT_LEN_MAX] = {'\0'};
   time_t current = time (NULL);

   if (strftime (date, DATE_FMT_LEN_MAX,
            "%Y_%m_%d_%H_%M_%S", localtime (&current)) == 0) {
      ERROR ("Date string format error");
      return -1;
   }

   snprintf (filename, sizeof (filename), "/sdcard/video/File_stream%d_%s.%s",
         streamId, date, suffix[dataType]);
   NOTICE ("filename: %s", filename);
   return 0;
}


void receive_data_from_server (AmTransferClient* client)
{
   int raw_fd_index;
   AmTransferPacket packet;

   while (client->ReceivePacket (&packet) == 0 && g_running) {
      DEBUG ("streamId: %d, dataLen: %d, dataType: %d\n",
            packet.header.streamId,
            packet.header.dataLen,
            (int)(packet.header.dataType));

      if (packet.header.dataType == AM_TRANSFER_PACKET_TYPE_MJPEG) {
         /* stream is jpeg, we need to create new file for each jpeg. */
         if (fetch_file_name (packet.header.streamId,
                  (int)(packet.header.dataType)) < 0) {
            ERROR ("Failed to get a new file name to store jpeg.");
            return;
         }

         if ((raw_fd[packet.header.streamId] = open (filename, O_WRONLY | O_CREAT)) < 0) {
            ERROR ("Failed to open %s: %s", filename, strerror (errno));
            return;
         }

         if (write (raw_fd[packet.header.streamId], packet.data,
                  packet.header.dataLen) != packet.header.dataLen) {
            ERROR ("Failed to write %d bytes into %s: %s",
                  packet.header.dataLen,
                  filename,
                  strerror (errno));
         }

         close (raw_fd[packet.header.streamId]);
         raw_fd[packet.header.streamId] = -1;
      } else {
         if (packet.header.dataType == AM_TRANSFER_PACKET_TYPE_H264) {
            raw_fd_index = packet.header.streamId << 1;
         } else {
            raw_fd_index = (packet.header.streamId << 1) + 1;
         }

         if (raw_fd[raw_fd_index] == -1) {
            /* First time to Receive data from certain stream */
            if (fetch_file_name (packet.header.streamId,
                     (int)(packet.header.dataType)) < 0) {
               ERROR ("Failed to get a new file name to store audio or h264 stream.");
               return;
            }

            if ((raw_fd[raw_fd_index] = open (filename, O_WRONLY | O_CREAT)) < 0) {
               ERROR ("Failed to open %s: %s", filename, strerror (errno));
               return;
            }
         }

         if (write (raw_fd[raw_fd_index], packet.data,
                  packet.header.dataLen) != packet.header.dataLen) {
            ERROR ("Failed to write %d bytes into %s: %s",
                  packet.header.dataLen,
                  filename,
                  strerror (errno));
         }
      }
   }
}

static void sigstop (int sig_num)
{
   NOTICE ("Quit");
   g_running = false;
}

   int
main (int argc, char **argv)
{
   int ret = -1;
   AmTransferClient *client = NULL;

   signal (SIGINT, sigstop);
   signal (SIGQUIT, sigstop);
   signal (SIGTERM, sigstop);

   if ((client = AmTransferClient::Create ()) == NULL) {
      ERROR ("Failed to create an instance of AmTransferClient!");
   } else {
      for (int i = 0; i < STREAM_NUMBER_MAX * 2 + 2; i++) {
         raw_fd[i] = -1;
      }

      receive_data_from_server (client);
      client->Close ();
      delete client;
      ret = 0;
   }

   return ret;
}
