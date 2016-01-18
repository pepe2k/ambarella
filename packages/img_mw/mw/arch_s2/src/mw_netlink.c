
/*******************************************************************************
 * mw_netlink.c
 *
 * Histroy:
 *  2014/09/02 2014 - [jingyang qiu] created file
 *
 * Copyright (C) 2012 - 2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <config.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <getopt.h>
#include <sched.h>
#include <basetypes.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/netlink.h>

#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "ambas_imgproc_arch.h"

#include "img_struct_arch.h"
#include "img_api_arch.h"
#include "img_dsp_interface_arch.h"

#include "img_hdr_api_arch.h"
#include "mw_aaa_params.h"
#include "mw_api.h"
#include "mw_pri_struct.h"
#include "mw_image_priv.h"

#ifdef CONFIG_IAV_CONTROL_AAA

#define NETLINK_IMAGE_PROTOCAL	20
#define MAX_NL_MSG_LEN 1024

typedef enum {
	IMAGE_CMD_START_AAA = 0,
	IMAGE_CMD_STOP_AAA,
} NL_IMAGE_AAA_CMD;

typedef enum {
	IMAGE_STATUS_START_AAA_SUCCESS = 0,
	IMAGE_STATUS_START_AAA_FAIL,
	IMAGE_STATUS_STOP_AAA_SUCCESS,
	IMAGE_STATUS_STOP_AAA_FAIL,
} NL_IMAGE_AAA_STATUS;

typedef enum {
	IMAGE_SESSION_CMD_CONNECT = 0,
	IMAGE_SESSION_CMD_DISCONNECT,
}NL_IMAGE_SESSION_CMD;

typedef enum {
	IMAGE_SESSION_STATUS_CONNECT_SUCCESS = 0,
	IMAGE_SESSION_STATUS_CONNECT_FAIL,
	IMAGE_SESSION_STATUS_DISCONNECT_SUCCESS,
	IMAGE_SESSION_STATUS_DISCONNECT_FAIL,
}NL_IMAGE_SESSION_STATUS;

typedef enum {
	IMAGE_MSG_INDEX_SESSION_CMD = 0,
	IMAGE_MSG_INDEX_SESSION_STATUS,
	IMAGE_MSG_INDEX_AAA_CMD,
	IMAGE_MSG_INDEX_AAA_STATUS,
}NL_IMAGE_MSG_INDEX;

typedef struct nl_image_msg_s {
	u32 pid;
	s32 index;
	union {
		s32 session_cmd;
		s32 session_status;
		s32 image_cmd;
		s32 image_status;
	} msg;
} nl_image_msg_t;

typedef struct nl_image_config_s {
	s32 fd_nl;
	s32 image_init;
	s32 nl_connected;
	nl_image_msg_t nl_msg;
	char nl_send_buf[MAX_NL_MSG_LEN];
	char nl_recv_buf[MAX_NL_MSG_LEN];
} nl_image_config_t;

static nl_image_config_t nl_image_config;
extern int fd_iav_aaa;
extern int netlink_exit_flag;

int init_netlink(void)
{
	u32 pid;
	struct sockaddr_nl saddr;

	nl_image_config.fd_nl = socket(AF_NETLINK,
		SOCK_RAW, NETLINK_IMAGE_PROTOCAL);
	memset(&saddr, 0, sizeof(saddr));
	pid = getpid();
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = pid;
	saddr.nl_groups = 0;
	saddr.nl_pad = 0;
	bind(nl_image_config.fd_nl, (struct sockaddr *)&saddr, sizeof(saddr));

	nl_image_config.nl_connected = 0;
	nl_image_config.image_init = 0;

	return 0;
}

static int send_image_msg_to_kernel(nl_image_msg_t nl_msg)
{
	struct sockaddr_nl daddr;
	struct msghdr msg;
	struct nlmsghdr *nlhdr = NULL;
	struct iovec iov;

	memset(&daddr, 0, sizeof(daddr));
	daddr.nl_family = AF_NETLINK;
	daddr.nl_pid = 0;
	daddr.nl_groups = 0;
	daddr.nl_pad = 0;

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_send_buf;
	nlhdr->nlmsg_pid = getpid();
	nlhdr->nlmsg_len = NLMSG_LENGTH(sizeof(nl_msg));
	nlhdr->nlmsg_flags = 0;
	memcpy(NLMSG_DATA(nlhdr), &nl_msg, sizeof(nl_msg));

	memset(&msg, 0, sizeof(struct msghdr));
	iov.iov_base = (void *)nlhdr;
	iov.iov_len = nlhdr->nlmsg_len;
	msg.msg_name = (void *)&daddr;
	msg.msg_namelen = sizeof(daddr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(nl_image_config.fd_nl, &msg, 0);

	return 0;
}

static int recv_image_msg_from_kernel()
{
	struct sockaddr_nl sa;
	struct nlmsghdr *nlhdr = NULL;
	struct msghdr msg;
	struct iovec iov;

	int ret = 0;

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_recv_buf;
	iov.iov_base = (void *)nlhdr;
	iov.iov_len = MAX_NL_MSG_LEN;

	memset(&sa, 0, sizeof(sa));
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&(sa);
	msg.msg_namelen = sizeof(sa);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (nl_image_config.fd_nl > 0) {
		ret = recvmsg(nl_image_config.fd_nl, &msg, 0);
	} else {
		printf("Netlink socket is not opened for receive message!\n");
		ret = -1;
	}

	return ret;
}

static int process_image_cmd(int image_cmd)
{
	int ret = 0;
	if (image_cmd == IMAGE_CMD_START_AAA) {
		ret = start_aaa_task();
		nl_image_config.nl_msg.pid = getpid();
		nl_image_config.nl_msg.index = IMAGE_MSG_INDEX_AAA_STATUS;
		if (ret < 0) {
			printf("Start AAA failed!\n");
			nl_image_config.nl_msg.msg.image_status =
				IMAGE_STATUS_START_AAA_FAIL;
			send_image_msg_to_kernel(nl_image_config.nl_msg);
		} else {
			nl_image_config.nl_msg.msg.image_status =
				IMAGE_STATUS_START_AAA_SUCCESS;
			send_image_msg_to_kernel(nl_image_config.nl_msg);
		}
	} else if (image_cmd == IMAGE_CMD_STOP_AAA) {
		ret = stop_aaa_task();
		nl_image_config.nl_msg.pid = getpid();
		nl_image_config.nl_msg.index = IMAGE_MSG_INDEX_AAA_STATUS;
		if (ret < 0) {
			printf("Stop AAA failed!\n");
			nl_image_config.nl_msg.msg.image_status =
				IMAGE_STATUS_STOP_AAA_FAIL;
			send_image_msg_to_kernel(nl_image_config.nl_msg);
		} else {
			nl_image_config.nl_msg.msg.image_status =
				IMAGE_STATUS_STOP_AAA_SUCCESS;
			send_image_msg_to_kernel(nl_image_config.nl_msg);
		}
	} else {
		printf("Unrecognized kernel message!\n");
		ret = -1;
	}

	return ret;
}

static int process_image_session_status(int session_status)
{
	int ret = 0;

	switch (session_status) {
	case IMAGE_SESSION_STATUS_CONNECT_SUCCESS:
		nl_image_config.nl_connected = 1;
		printf("Connection established with kernel.\n");
		break;
	case IMAGE_SESSION_STATUS_CONNECT_FAIL:
		nl_image_config.nl_connected = 0;
		printf("Failed to establish connection with kernel!\n");
		break;
	case IMAGE_SESSION_STATUS_DISCONNECT_SUCCESS:
		nl_image_config.nl_connected = 0;
		printf("Connection removed with kernel.\n");
		break;
	case IMAGE_SESSION_STATUS_DISCONNECT_FAIL:
		nl_image_config.nl_connected = 0;
		printf("Failed to remove connection with kernel!\n");
		break;
	default:
		printf("Unrecognized session status from kernel!\n");
		ret = -1;
		break;
	}

	return ret;
}

static int check_recv_image_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	int msg_len;

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_recv_buf;
	if (nlhdr->nlmsg_len <  sizeof(struct nlmsghdr)) {
		printf("Corruptted kernel message!\n");
		return -1;
	}
	msg_len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
	if (msg_len < sizeof(nl_image_msg_t)) {
		printf("Unknown kernel message!!\n");
		return -1;
	}

	return 0;
}

static int process_image_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	nl_image_msg_t *kernel_msg;
	int ret = 0;

	if (check_recv_image_msg() < 0) {
		return -1;
	}

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_recv_buf;
	kernel_msg = (nl_image_msg_t *)NLMSG_DATA(nlhdr);

	if(kernel_msg->index == IMAGE_MSG_INDEX_AAA_CMD) {
		if (process_image_cmd(kernel_msg->msg.image_cmd) < 0) {
			ret = -1;
		}
	} else if (kernel_msg->index == IMAGE_MSG_INDEX_SESSION_STATUS) {
		if (process_image_session_status(kernel_msg->msg.session_status) < 0) {
			ret = -1;
		}
	} else {
		printf("Incorrect message from kernel!\n");
		ret = -1;
	}

	return ret;
}

static int nl_send_image_session_cmd(int cmd)
{
	int ret = 0;

	if (cmd == IMAGE_SESSION_CMD_DISCONNECT) {
		stop_aaa_task();
	}

	nl_image_config.nl_msg.pid = getpid();
	nl_image_config.nl_msg.index = IMAGE_MSG_INDEX_SESSION_CMD;
	nl_image_config.nl_msg.msg.session_cmd = cmd;
	send_image_msg_to_kernel(nl_image_config.nl_msg);

	if (cmd == IMAGE_SESSION_CMD_CONNECT) {
		ret = recv_image_msg_from_kernel();

		if (ret > 0) {
			ret = process_image_msg();
			if (ret < 0) {
				printf("Failed to process session status!\n");
			}
		} else {
			printf("Error for getting session status!\n");
		}
	}

	return ret;
}

void * netlink_loop(void * data)
{
	int ret;

	if (nl_send_image_session_cmd(IMAGE_SESSION_CMD_CONNECT) < 0) {
		printf("Failed to establish connection with kernel!\n");
	}

	if (!nl_image_config.nl_connected) {
		return NULL;
	}

	while (nl_image_config.nl_connected || (netlink_exit_flag != 1)) {
		ret = recv_image_msg_from_kernel();
		if (ret > 0) {
			ret = process_image_msg();
			if (ret < 0) {
				printf("Failed to process the msg from kernel!\n");
			}
		}
		else {
			printf("Error for getting msg from kernel!\n");
		}
	}
	nl_image_config.nl_connected = 0;
	return NULL;
}

int send_image_msg_stop_aaa(void)
{
	if (nl_image_config.nl_connected) {
		process_image_cmd(IMAGE_CMD_STOP_AAA);
		nl_send_image_session_cmd(IMAGE_SESSION_CMD_DISCONNECT);
	} else {
		if(stop_aaa_task() < 0) {
			printf("stop_aaa_task\n");
			return -1;
		}
	}
	return 0;
}

#else

int init_netlink(void)
{
	return 0;
}

int send_image_msg_stop_aaa(void)
{
	if(stop_aaa_task() < 0) {
		printf("stop_aaa_task\n");
		return -1;
	}
	return 0;
}

void * netlink_loop(void * data)
{
	return 0;
}
#endif
