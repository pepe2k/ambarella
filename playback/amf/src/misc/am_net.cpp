/*
 * am_net.cpp
 *
 * History:
 *    2012/04/28 - [Zhi He] created file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

//depend on socket.h
#include <sys/socket.h>
#include <arpa/inet.h>
#if PLATFORM_LINUX
#include <netinet/in.h>
#include <netdb.h>
#endif

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#include "am_net.h"

AM_INT AMNet_ConnectTo(const char *host_addr, AM_INT port, AM_INT type, AM_INT protocol)
{
    AM_INT fd = -1;
    AM_INT ret = 0;
    struct sockaddr_in localhost_addr;
    struct sockaddr_in dest_addr;

    //only implement TCP
    AM_ASSERT(host_addr);
    AM_ASSERT(SOCK_STREAM == port);
    AM_ASSERT(IPPROTO_TCP == protocol);
    type = SOCK_STREAM;
    protocol = IPPROTO_TCP;

    //create socket
    fd = socket(AF_INET, type, protocol);
    if (fd == -1) {
        AM_ERROR("create socket fail, type %d, protocol %d.\n", type, protocol);
        return -1;
    }

    //bind to local host, random port
    memset(&localhost_addr, 0x0, sizeof(localhost_addr));
    localhost_addr.sin_family = AF_INET;
    localhost_addr.sin_port = 0;
    localhost_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(fd, (struct sockaddr *)&localhost_addr, sizeof(struct sockaddr));
    if (ret < 0) {
        AM_ERROR("bind() fail, ret %d.\n", ret);
        close(fd);
        return -2;
    }

    //connect to dest addr
    memset(&dest_addr, 0x0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = port;
    dest_addr.sin_addr.s_addr = inet_addr(host_addr);
    ret = connect(fd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
    if (ret < 0) {
        AM_ERROR("connect() fail, ret %d.\n", ret);
        close(fd);
        return -3;
    }

    return fd;
}

AM_INT AMNet_Send(AM_INT fd, AM_U8* data, AM_INT len, AM_UINT flag)
{
    AM_INT sent = 0, remain = len;
    do {
        sent = send(fd, (void*)data, len, flag);
        remain -= sent;
    } while (remain > 0);
    return len;
}

AM_INT AMNet_Recv(AM_INT fd, AM_U8* data, AM_INT len, AM_UINT flag)
{
#if 0
    AM_INT read = 0, remain = len;
    do {
        read = recv(fd, (void*)data, len, flag);
        remain -= read;
    } while (remain > 0);
    return len;
#endif
    return recv(fd, (void*)data, len, flag);
}

AM_INT AMNet_SendTo(AM_INT fd, AM_U8* data, AM_INT len, AM_UINT flag, const struct sockaddr *to, AM_INT tolen)
{
    AM_INT sent = 0, remain = len;
    do {
        sent = sendto(fd, (void*)data, len, flag, to, tolen);
        remain -= sent;
    } while (remain > 0);
    return len;
}

AM_INT AMNet_RecvFrom(AM_INT fd, AM_U8* data, AM_INT len, AM_UINT flag, const struct sockaddr *from, AM_INT fromlen)
{
#if 0
    AM_INT read = 0, remain = len;
    do {
        read = recvfrom(fd, (void*)data, len, flag, from, fromlen);
        remain -= read;
    } while (remain > 0);
    return len;
#endif
    return recvfrom(fd, (void*)data, len, flag, from, (socklen_t*)fromlen);
}

