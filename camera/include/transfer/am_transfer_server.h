/*
 * am_transfer_server.h
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
#ifndef __AM_TRANSFER_SERVER_H__
#define __AM_TRANSFER_SERVER_H__

#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#ifndef MAX_REQUEST_NUM
#define MAX_REQUEST_NUM 4
#endif

struct AmPacket;

class AmTransferServer {
public:
   AmTransferServer ();
   int Construct ();
   ~AmTransferServer ();
   static AmTransferServer *Create ();
   int SendPacket (AmTransferPacket *packet);
   int Start ();
   int Stop ();

private:
   static void *connection_accept (void *arg);

private:
   static struct sockaddr_un addr;
   pthread_t                 accept_thread_id;
   static int                socket_fd;
   static int                connect_fd[MAX_REQUEST_NUM];
   static int                connect_num;
   static pthread_mutex_t    mutex;
};

#endif
