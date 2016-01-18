/*
 * guard_vsync_loss.c
 * this app will guard vsync loss issue automatically together with IAV
 *
 * History:
 *	2012/11/09 - [Zhaoyang Chen] create this file
 *
 * Copyright (C) 2012-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "amba_usb.h"
#include <signal.h>

// vin
#include "../../vin_test/vin_init.c"


#define NETLINK_VSYNC_PROTOCAL	21
#define MAX_NL_MSG_LEN 			1024

#define MAX_ENCODE_STREAM_NUM	(IAV_STREAM_MAX_NUM_IMPL)
#define ALL_ENCODE_STREAMS		((1<<MAX_ENCODE_STREAM_NUM) - 1)

typedef enum {
	VSYNC_REQ_VYSNC_LOSS = 0,
} NL_VSYNC_REQ;
typedef enum {
	VSYNC_RESP_VSYNC_SUCCEED = 0,
	VSYNC_RESP_VSYNC_FAIL,
} NL_VSYNC_RESPONSE;
typedef enum {
	VSYNC_SESSION_CMD_CONNECT = 0,
	VSYNC_SESSION_CMD_DISCONNECT,
} NL_VSYNC_SESSION_CMD;
typedef enum {
	VSYNC_SESSION_STATUS_CONNECT_SUCCESS = 0,
	VSYNC_SESSION_STATUS_CONNECT_FAIL,
	VSYNC_SESSION_STATUS_DISCONNECT_SUCCESS,
	VSYNC_SESSION_STATUS_DISCONNECT_FAIL,
} NL_VSYNC_SESSION_STATUS;
typedef enum {
	VSYNC_MSG_INDEX_SESSION_CMD = 0,
	VSYNC_MSG_INDEX_SESSION_STATUS,
	VSYNC_MSG_INDEX_VSYNC_REQ,
	VSYNC_MSG_INDEX_VSYNC_RESP,
} NL_VSYNC_MSG_INDEX;
typedef struct nl_vsync_msg_s {
	u32 pid;
	s32 index;
	union {
		s32 session_cmd;
		s32 session_status;
		s32 vsync_req;
		s32 vsync_resp;
	} msg;
} nl_vsync_msg_t;
typedef struct nl_vsync_config_s {
	s32 fd_nl;
	s32 nl_connected;
	nl_vsync_msg_t nl_msg;
	char nl_send_buf[MAX_NL_MSG_LEN];
	char nl_recv_buf[MAX_NL_MSG_LEN];
} nl_vsync_config_t;


int fd_iav;
static nl_vsync_config_t nl_vsync_config;
iav_stream_id_t	encode_streams;

static int init_netlink()
{
	u32 pid;
	struct sockaddr_nl saddr;

	nl_vsync_config.fd_nl = socket(AF_NETLINK,
		SOCK_RAW, NETLINK_VSYNC_PROTOCAL);
	memset(&saddr, 0, sizeof(saddr));
	pid = getpid();
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = pid;
	saddr.nl_groups = 0;
	saddr.nl_pad = 0;
	bind(nl_vsync_config.fd_nl, (struct sockaddr *)&saddr, sizeof(saddr));

	nl_vsync_config.nl_connected = 0;

	return 0;
}

static int send_vsync_msg_to_kernel(nl_vsync_msg_t nl_msg)
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

	nlhdr = (struct nlmsghdr *)nl_vsync_config.nl_send_buf;
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

	sendmsg(nl_vsync_config.fd_nl, &msg, 0);

	return 0;
}

static int recv_vsync_msg_from_kernel()
{
	struct sockaddr_nl sa;
	struct nlmsghdr *nlhdr = NULL;
	struct msghdr msg;
	struct iovec iov;

	int ret = 0;

	nlhdr = (struct nlmsghdr *)nl_vsync_config.nl_recv_buf;
	iov.iov_base = (void *)nlhdr;
	iov.iov_len = MAX_NL_MSG_LEN;

	memset(&sa, 0, sizeof(sa));
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&(sa);
	msg.msg_namelen = sizeof(sa);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (nl_vsync_config.fd_nl > 0) {
		ret = recvmsg(nl_vsync_config.fd_nl, &msg, 0);
	} else {
		printf("Netlink socket is not opened to receive message!\n");
		ret = -1;
	}

	return ret;
}

static int check_recv_vsync_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	int msg_len;

	nlhdr = (struct nlmsghdr *)nl_vsync_config.nl_recv_buf;
	if (nlhdr->nlmsg_len <  sizeof(struct nlmsghdr)) {
		printf("Corruptted kernel message!\n");
		return -1;
	}
	msg_len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
	if (msg_len < sizeof(nl_vsync_msg_t)) {
		printf("Unknown kernel message!!\n");
		return -1;
	}

	return 0;
}

int stop_all_encode_streams(void)
{
	int i;
	iav_state_info_t info;
	iav_encode_stream_info_ex_t stream_info;

	iav_stream_id_t  streamid = ALL_ENCODE_STREAMS;

	ioctl(fd_iav, IAV_IOC_GET_STATE_INFO, &info);
	if (info.state != IAV_STATE_ENCODING) {
		return 0;
	}
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		stream_info.id = (1 << i);
		ioctl(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info);
		if (stream_info.state != IAV_STREAM_STATE_ENCODING) {
			streamid &= ~(1 << i);
		}
	}
	encode_streams = streamid;
	if (streamid == 0)
		return 0;
	ioctl(fd_iav, IAV_IOC_STOP_ENCODE_EX, streamid);
	printf("Stop encoding for stream 0x%x done.\n", streamid);
	return 0;
}

static int goto_idle(void)
{
	int ret = 0;

	printf("Start to enter idle.\n");
	ret = ioctl(fd_iav, IAV_IOC_ENTER_IDLE, 0);
	if (!ret) {
		printf("Succeed to enter idle.\n");
	} else {
		printf("Failed to enter idle.\n");
	}

	return ret;
}

static int enter_preview(void)
{
	int ret = 0;

	printf("Start to enter preview.\n");
	ret = ioctl(fd_iav, IAV_IOC_ENABLE_PREVIEW, 0);
	if (!ret) {
		printf("Succeed to enter preview.\n");
	}
	else {
		printf("Failed to enter preview.\n");
	}

	return ret;
}

static int restart_encoded_streams(void)
{
	int i;
	iav_encode_stream_info_ex_t info;

	int ret = 0;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		info.id = (1 << i);
		ret = ioctl(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &info);
		if (ret) {
			printf("Failed to get stream info for stream %d.\n", i);
			return ret;
		}

		if (info.state == IAV_STREAM_STATE_ENCODING) {
			encode_streams &= ~(1 << i);
		}
	}
	if (encode_streams == 0) {
		printf("No encode streams to recover.\n");
		return 0;
	}
	printf("Start recovering encoding for stream 0x%x.\n", encode_streams);
	// start encode
	ret = ioctl(fd_iav, IAV_IOC_START_ENCODE_EX, encode_streams);
	if (!ret) {
		printf("Succeed to recover encoding for stream 0x%x.\n", encode_streams);
	} else {
		printf("Failed to recover encoding for stream 0x%x.\n", encode_streams);
	}

	return ret;
}

static int reset_vin(void)
{
	// select channel: for multi channel VIN (initialize)
	if (channel >= 0) {
		if (select_channel() < 0)
			return -1;
	}

	if(ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_MODE, &vin_mode)){
		return -1;
	} else {
		printf("Start to restore vin mode %d.\n", vin_mode);
	}

	if (init_vin(vin_mode) < 0)
		return -1;

	return 0;
}


static int recover_vsync_loss()
{
	int ret = 0;

	ret = stop_all_encode_streams();
	if (ret) {
		return ret;
	}
	ret = goto_idle();
	if (ret) {
		return ret;
	}
	reset_vin();
	ret = enter_preview();
	if (ret) {
		return ret;
	}
	ret = restart_encoded_streams();

	return ret;
}

static int process_vsync_req(int vsync_req)
{
	int ret = 0;

	if (vsync_req == VSYNC_REQ_VYSNC_LOSS) {
		ret = recover_vsync_loss();
		nl_vsync_config.nl_msg.pid = getpid();
		nl_vsync_config.nl_msg.index = VSYNC_MSG_INDEX_VSYNC_RESP;
		if (ret < 0) {
			nl_vsync_config.nl_msg.msg.vsync_resp =
				VSYNC_RESP_VSYNC_FAIL;
			send_vsync_msg_to_kernel(nl_vsync_config.nl_msg);
		} else {
			nl_vsync_config.nl_msg.msg.vsync_resp =
				VSYNC_RESP_VSYNC_SUCCEED;
			send_vsync_msg_to_kernel(nl_vsync_config.nl_msg);
		}
	} else {
		printf("Unrecognized kernel message!\n");
		ret = -1;
	}

	return ret;
}

static int process_vsync_session_status(int session_status)
{
	int ret = 0;

	switch (session_status) {
	case VSYNC_SESSION_STATUS_CONNECT_SUCCESS:
		nl_vsync_config.nl_connected = 1;
		printf("Connection established with kernel.\n");
		break;
	case VSYNC_SESSION_STATUS_CONNECT_FAIL:
		nl_vsync_config.nl_connected = 0;
		printf("Failed to establish connection with kernel!\n");
		break;
	case VSYNC_SESSION_STATUS_DISCONNECT_SUCCESS:
		nl_vsync_config.nl_connected = 0;
		printf("Connection removed with kernel.\n");
		break;
	case VSYNC_SESSION_STATUS_DISCONNECT_FAIL:
		nl_vsync_config.nl_connected = 0;
		printf("Failed to remove connection with kernel!\n");
		break;
	default:
		printf("Unrecognized session status from kernel!\n");
		ret = -1;
		break;
	}

	return ret;
}

static int process_vsync_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	nl_vsync_msg_t *kernel_msg;
	int ret = 0;

	if (check_recv_vsync_msg() < 0) {
		return -1;
	}

	nlhdr = (struct nlmsghdr *)nl_vsync_config.nl_recv_buf;
	kernel_msg = (nl_vsync_msg_t *)NLMSG_DATA(nlhdr);

	if(kernel_msg->index == VSYNC_MSG_INDEX_VSYNC_REQ) {
		if (process_vsync_req(kernel_msg->msg.vsync_req) < 0) {
			ret = -1;
		}
	} else if (kernel_msg->index == VSYNC_MSG_INDEX_SESSION_STATUS) {
		if (process_vsync_session_status(kernel_msg->msg.session_status) < 0) {
			ret = -1;
		}
	} else {
		printf("Incorrect message from kernel!\n");
		ret = -1;
	}

	return ret;
}

static int nl_send_vsync_session_cmd(int cmd)
{
	int ret = 0;

	nl_vsync_config.nl_msg.pid = getpid();
	nl_vsync_config.nl_msg.index = VSYNC_MSG_INDEX_SESSION_CMD;
	nl_vsync_config.nl_msg.msg.session_cmd = cmd;
	send_vsync_msg_to_kernel(nl_vsync_config.nl_msg);

	ret = recv_vsync_msg_from_kernel();

	if (ret > 0) {
		ret = process_vsync_msg();
		if (ret < 0) {
			printf("Failed to process session status!\n");
		}
	} else {
		printf("Error for getting session status!\n");
	}

	return ret;
}

static void * netlink_loop(void * data)
{
	int ret;
	int count = 100;

	while (count && !nl_vsync_config.nl_connected) {
		if (nl_send_vsync_session_cmd(VSYNC_SESSION_CMD_CONNECT) < 0) {
			printf("Failed to establish connection with kernel!\n");
		}
		sleep(1);
		count--;
	}

	if (!nl_vsync_config.nl_connected) {
		printf("Please enable kernel vsync loss guard mechanism!!!\n");
		return NULL;
	}

	while (nl_vsync_config.nl_connected) {
		ret = recv_vsync_msg_from_kernel();
		if (ret > 0) {
			ret = process_vsync_msg();
			if (ret < 0) {
				printf("Failed to process the msg from kernel!\n");
			}
		}
		else {
			printf("Error for getting msg from kernel!\n");
		}
	}

	return NULL;
}

static void sigstop()
{
	nl_send_vsync_session_cmd(VSYNC_SESSION_CMD_DISCONNECT);
	exit(1);
}

int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C, Ctrl+'\', and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	init_netlink();
	netlink_loop(NULL);

	return 0;
}

