/*
 * am_net.h
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

#ifndef __AM_NET_H__
#define __AM_NET_H__

AM_INT AMNet_ConnectTo(const char *host_addr, AM_INT port, AM_INT type, AM_INT protocol);

AM_INT AMNet_Send(AM_INT fd, AM_U8* data, AM_INT len, AM_UINT flag);
AM_INT AMNet_Recv(AM_INT fd, AM_U8* data, AM_INT len, AM_UINT flag);
AM_INT AMNet_SendTo(AM_INT fd, AM_U8* data, AM_INT len, AM_UINT flag, const struct sockaddr *to, AM_INT tolen);
AM_INT AMNet_RecvFrom(AM_INT fd, AM_U8* data, AM_INT len, AM_UINT flag, const struct sockaddr *from, AM_INT fromlen);

#endif

