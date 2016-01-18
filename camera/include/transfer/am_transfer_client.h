/*
 * am_transfer_client.h
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
#ifndef __AM_TRANSFER_CLIENT_H__
#define __AM_TRANSFER_CLIENT_H__

#include <sys/socket.h>
#include <sys/un.h>

struct AmPacket;

class AmTransferClient {
public:
   AmTransferClient ();
   int Construct ();
   ~AmTransferClient ();
   static AmTransferClient *Create ();
   int ReceivePacket (AmTransferPacket *packet);
   int Close ();

private:
   struct sockaddr_un addr;
   int                socket_fd;
};

#endif
