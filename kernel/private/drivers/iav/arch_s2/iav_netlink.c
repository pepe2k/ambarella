/*
 * iav_netlink.c
 *
 * History:
 *	2013/05/09 [Jian Tang] created file
 *
 * Copyright (C) 2013-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>

#include "amba_vin.h"
#include "amba_vout.h"
#include "amba_iav.h"

#include "iav_common.h"
#include "iav_drv.h"
#include "dsp_cmd.h"
#include "dsp_api.h"
#include "iav_api.h"
#include "iav_priv.h"
#include "iav_mem.h"
#include "utils.h"

#include <linux/netlink.h>
#include <net/sock.h>
#include <asm/siginfo.h>
#include <linux/signal.h>

#include "iav_netlink.h"

#ifdef CONFIG_GUARD_VSYNC_LOSS
static iav_nl_vsync_config_t G_nl_vsync_config;
#endif

#ifdef CONFIG_IAV_CONTROL_AAA
iav_nl_image_config_t G_nl_image_config;
#endif

#ifdef CONFIG_GUARD_VSYNC_LOSS
static int nl_send_vsync_msg(nl_vsync_msg_t vsync_msg)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlhdr = NULL;
	void *data;

	if (vsync_msg.index != VSYNC_MSG_INDEX_VSYNC_REQ&&
		vsync_msg.index != VSYNC_MSG_INDEX_SESSION_STATUS) {
		iav_printk("Incorrect vsync netlink msg to app!\n");
		return -1;
	}

	skb = nlmsg_new(sizeof(vsync_msg), GFP_KERNEL);
	if (!skb) {
		iav_printk("Function nlmsg_new failed!\n");
		return -1;
	}
	nlhdr = __nlmsg_put(skb, G_nl_vsync_config.nl_user_pid, 0, NLMSG_NOOP,
		sizeof(vsync_msg), 0);

	data = NLMSG_DATA(nlhdr);
	memcpy(data, (void *)&vsync_msg, sizeof(vsync_msg));

	netlink_unicast(G_nl_vsync_config.nl_sock,
		skb, G_nl_vsync_config.nl_user_pid, 0);

	if (vsync_msg.index == VSYNC_MSG_INDEX_VSYNC_REQ) {
		// wait for the response of the message
		if (vsync_msg.msg.vsync_req == VSYNC_REQ_VYSNC_LOSS) {
			iav_printk("Send vsync loss request to app %d.\n",
				G_nl_vsync_config.nl_user_pid);
			iav_unlock();
			if (wait_for_completion_interruptible(
					&G_nl_vsync_config.vsync_loss_complete)) {
				iav_lock();
				return -EINTR;
			}
			iav_lock();
		}
	} else if (vsync_msg.index == VSYNC_MSG_INDEX_SESSION_STATUS) {
		iav_printk("Send vsync session status to app %d.\n",
			G_nl_vsync_config.nl_user_pid);
	}

	return 0;
}

static int nl_process_vsync_session_cmd(nl_vsync_msg_t nl_recv_msg)
{
	int ret = 0;
	nl_vsync_msg_t nl_send_msg;

	switch (nl_recv_msg.msg.session_cmd) {
	case VSYNC_SESSION_CMD_CONNECT:
		G_nl_vsync_config.nl_connected = 1;
		G_nl_vsync_config.nl_user_pid = nl_recv_msg.pid;
		iav_printk("Correct vsync connect cmd from app %u.\n",
			nl_recv_msg.pid);
		nl_send_msg.pid = 0;
		nl_send_msg.index = VSYNC_MSG_INDEX_SESSION_STATUS;
		nl_send_msg.msg.session_status =
			VSYNC_SESSION_STATUS_CONNECT_SUCCESS;
		nl_send_vsync_msg(nl_send_msg);
		break;
	case VSYNC_SESSION_CMD_DISCONNECT:
		G_nl_vsync_config.nl_connected = 0;
		iav_printk("Correct vsync disconnect cmd from app %u.\n",
			nl_recv_msg.pid);
		nl_send_msg.pid = 0;
		nl_send_msg.index = VSYNC_MSG_INDEX_SESSION_STATUS;
		nl_send_msg.msg.session_status =
			VSYNC_SESSION_STATUS_DISCONNECT_SUCCESS;
		nl_send_vsync_msg(nl_send_msg);
		G_nl_vsync_config.nl_user_pid = -1;
		break;
	default:
		iav_printk("Unknown vsync session cmd from app %d!\n",
			nl_recv_msg.pid);
		ret = -1;
		break;
	}

	return ret;
}

static int nl_process_vsync_response(nl_vsync_msg_t nl_recv_msg)
{
	int ret = 0;

	switch (nl_recv_msg.msg.vsync_resp) {
	case VSYNC_RESP_VSYNC_SUCCEED:
		iav_printk("Vsync loss responded from app %u by netlink succeeded.\n",
			nl_recv_msg.pid);
		complete(&G_nl_vsync_config.vsync_loss_complete);
		break;
	case VSYNC_RESP_VSYNC_FAIL:
		iav_printk("Failed to respond vsync loss from app %uby netlink!\n",
			nl_recv_msg.pid);
		complete(&G_nl_vsync_config.vsync_loss_complete);
		break;
	default:
		iav_printk("Unknown vysnc netlink message from app %u!\n",
			nl_recv_msg.pid);
		ret = -1;
		break;
	}

	return ret;
}

static void nl_recv_vsync_msg_handler(struct sk_buff * skb)
{
	struct nlmsghdr * nlhdr = NULL;
	int len;
	int msg_len;
	nl_vsync_msg_t nl_recv_msg;

	nlhdr = nlmsg_hdr(skb);
	len = skb->len;

	for(; NLMSG_OK(nlhdr, len); nlhdr = NLMSG_NEXT(nlhdr, len)) {
		if (nlhdr->nlmsg_len < sizeof(struct nlmsghdr)) {
			iav_printk("Corruptted vsync netlink msg!\n");
			continue;
		}
		msg_len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
		if (msg_len < sizeof(nl_recv_msg)) {
			iav_printk("Unknown vsync netlink msg!\n");
			continue;
		}
		memcpy(&nl_recv_msg, NLMSG_DATA(nlhdr), sizeof(nl_recv_msg));
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
		if (NETLINK_CB(skb).pid == nl_recv_msg.pid) {
#else
		if (NETLINK_CB(skb).portid == nl_recv_msg.pid) {
#endif
			switch(nl_recv_msg.index) {
			case VSYNC_MSG_INDEX_SESSION_CMD:
				nl_process_vsync_session_cmd(nl_recv_msg);
				break;
			case VSYNC_MSG_INDEX_VSYNC_RESP:
				nl_process_vsync_response(nl_recv_msg);
				break;
			case VSYNC_MSG_INDEX_SESSION_STATUS:
			case VSYNC_MSG_INDEX_VSYNC_REQ:
			default:
				iav_printk("Error vsync msg from app %u, index %d, msg %d!\n",
					nl_recv_msg.pid, nl_recv_msg.index,
					nl_recv_msg.msg.vsync_resp);
				break;
			}
		}
	}
}

int nl_restore_vsync_loss(void)
{
	int ret = 0;
	if (G_nl_vsync_config.nl_connected) {
		G_nl_vsync_config.nl_msg.pid = 0;
		G_nl_vsync_config.nl_msg.index = VSYNC_MSG_INDEX_VSYNC_REQ;
		G_nl_vsync_config.nl_msg.msg.vsync_req = VSYNC_REQ_VYSNC_LOSS;

		ret = nl_send_vsync_msg(G_nl_vsync_config.nl_msg);
	}

	return ret;
}
#endif


#ifdef CONFIG_IAV_CONTROL_AAA
static int nl_send_image_msg(nl_image_msg_t image_msg)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlhdr = NULL;
	void *data;

	if (image_msg.index != IMAGE_MSG_INDEX_AAA_CMD &&
		image_msg.index != IMAGE_MSG_INDEX_SESSION_STATUS) {
		iav_printk("Incorrect netlink msg to app!\n");
		return -1;
	}

	skb = nlmsg_new(sizeof(image_msg), GFP_KERNEL);
	if (!skb) {
		iav_printk("Function nlmsg_new failed!\n");
		return -1;
	}
	nlhdr = __nlmsg_put(skb, G_nl_image_config.nl_user_pid, 0, NLMSG_NOOP,
		sizeof(image_msg), 0);

	data = NLMSG_DATA(nlhdr);
	memcpy(data, (void *)&image_msg, sizeof(image_msg));

	netlink_unicast(G_nl_image_config.nl_sock, skb, G_nl_image_config.nl_user_pid, 0);

	if (image_msg.index == IMAGE_MSG_INDEX_AAA_CMD) {
		// wait for the response of the command
		iav_unlock();
		if (image_msg.msg.image_cmd == IMAGE_CMD_START_AAA) {
			iav_printk("Send start AAA cmd to app %d.\n",
				G_nl_image_config.nl_user_pid);
			if (wait_for_completion_interruptible(
					&G_nl_image_config.aaa_start_complete)) {
				iav_lock();
				return -EINTR;
			}
		} else if (image_msg.msg.image_cmd == IMAGE_CMD_STOP_AAA) {
			iav_printk("Send stop AAA cmd to app %d.\n",
				G_nl_image_config.nl_user_pid);
			if (wait_for_completion_interruptible(
					&G_nl_image_config.aaa_stop_complete)) {
				iav_lock();
				return -EINTR;
			}
		} else if (image_msg.msg.image_cmd == IMAGE_CMD_PREPARE_AAA) {
			iav_printk("Send prepare AAA cmd to app %d.\n",
				G_nl_image_config.nl_user_pid);
			if (wait_for_completion_interruptible(
					&G_nl_image_config.aaa_prepare_complete)) {
				iav_lock();
				return -EINTR;
			}
		}
		iav_lock();
	} else if (image_msg.index == IMAGE_MSG_INDEX_SESSION_STATUS) {
		iav_printk("Send session status to app %d.\n",
			G_nl_image_config.nl_user_pid);
	}

	return 0;
}

static int nl_process_image_session_cmd(nl_image_msg_t nl_recv_msg)
{
	int ret = 0;
	nl_image_msg_t nl_send_msg;

	switch (nl_recv_msg.msg.session_cmd) {
	case IMAGE_SESSION_CMD_CONNECT:
		G_nl_image_config.nl_connected = 1;
		G_nl_image_config.nl_user_pid = nl_recv_msg.pid;
		iav_printk("Correct connect cmd from app %u.\n",
			nl_recv_msg.pid);
		nl_send_msg.pid = 0;
		nl_send_msg.index = IMAGE_MSG_INDEX_SESSION_STATUS;
		nl_send_msg.msg.session_status =
			IMAGE_SESSION_STATUS_CONNECT_SUCCESS;
		nl_send_image_msg(nl_send_msg);
		break;
	case IMAGE_SESSION_CMD_DISCONNECT:
		G_nl_image_config.nl_connected = 0;
		iav_printk("Correct disconnect cmd from app %u.\n",
			nl_recv_msg.pid);
		nl_send_msg.pid = 0;
		nl_send_msg.index = IMAGE_MSG_INDEX_SESSION_STATUS;
		nl_send_msg.msg.session_status =
			IMAGE_SESSION_STATUS_DISCONNECT_SUCCESS;
		nl_send_image_msg(nl_send_msg);
		G_nl_image_config.nl_user_pid = -1;
		break;
	default:
		iav_printk("Unknown session cmd from app %d!\n",
			nl_recv_msg.pid);
		ret = -1;
		break;
	}

	return ret;
}

static int nl_process_image_aaa_status(nl_image_msg_t nl_recv_msg)
{
	int ret = 0;

	switch (nl_recv_msg.msg.image_status) {
	case IMAGE_STATUS_START_AAA_SUCCESS:
		iav_printk("Start AAA by netlink succeeded.\n");
		complete(&G_nl_image_config.aaa_start_complete);
		break;
	case IMAGE_STATUS_START_AAA_FAIL:
		iav_printk("Failed to start AAA by netlink!\n");
		complete(&G_nl_image_config.aaa_start_complete);
		break;
	case IMAGE_STATUS_STOP_AAA_SUCCESS:
		iav_printk("Stop AAA by netlink succeeded.\n");
		complete(&G_nl_image_config.aaa_stop_complete);
		break;
	case IMAGE_STATUS_STOP_AAA_FAIL:
		iav_printk("Failed to stop AAA by netlink!\n");
		complete(&G_nl_image_config.aaa_stop_complete);
		break;
	case IMAGE_STATUS_PREPARE_AAA_SUCCESS:
		iav_printk("Prepare AAA by netlink succeeded.\n");
		complete(&G_nl_image_config.aaa_prepare_complete);
		break;
	case IMAGE_STATUS_PREPARE_AAA_FAIL:
		iav_printk("Failed to prepare AAA by netlink!\n");
		complete(&G_nl_image_config.aaa_prepare_complete);
		break;
	default:
		iav_printk("Unknown netlink message from app!\n");
		ret = -1;
		break;
	}

	return ret;
}

static void nl_recv_image_msg_handler(struct sk_buff * skb)
{
	struct nlmsghdr * nlhdr = NULL;
	int len;
	int msg_len;
	nl_image_msg_t nl_recv_msg;

	nlhdr = nlmsg_hdr(skb);
	len = skb->len;

	for(; NLMSG_OK(nlhdr, len); nlhdr = NLMSG_NEXT(nlhdr, len)) {
		if (nlhdr->nlmsg_len < sizeof(struct nlmsghdr)) {
			iav_printk("Corruptted netlink msg!\n");
			continue;
		}
		msg_len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
		if (msg_len < sizeof(nl_recv_msg)) {
			iav_printk("Unknown netlink msg!\n");
			continue;
		}
		memcpy(&nl_recv_msg, NLMSG_DATA(nlhdr), sizeof(nl_recv_msg));
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
		if (NETLINK_CB(skb).pid == nl_recv_msg.pid) {
#else
		if (NETLINK_CB(skb).portid == nl_recv_msg.pid) {
#endif
			switch(nl_recv_msg.index) {
			case IMAGE_MSG_INDEX_SESSION_CMD:
				nl_process_image_session_cmd(nl_recv_msg);
				break;
			case IMAGE_MSG_INDEX_AAA_STATUS:
				nl_process_image_aaa_status(nl_recv_msg);
				break;
			case IMAGE_MSG_INDEX_SESSION_STATUS:
			case IMAGE_MSG_INDEX_AAA_CMD:
			default:
				iav_printk("Incorrect msg from app %u, index %d, msg %d!\n",
					nl_recv_msg.pid, nl_recv_msg.index,
					nl_recv_msg.msg.image_status);
				break;
			}
		}
	}
}

int nl_image_prepare_aaa(void)
{
	int ret = 0;
	if (G_nl_image_config.nl_connected) {
		G_nl_image_config.nl_msg.pid = 0;
		G_nl_image_config.nl_msg.index = IMAGE_MSG_INDEX_AAA_CMD;
		G_nl_image_config.nl_msg.msg.image_cmd = IMAGE_CMD_PREPARE_AAA;
		nl_send_image_msg(G_nl_image_config.nl_msg);
	}
	return ret;
}

int nl_image_start_aaa(void)
{
	int ret = 0;
	if (G_nl_image_config.nl_connected) {
		G_nl_image_config.nl_msg.pid = 0;
		G_nl_image_config.nl_msg.index = IMAGE_MSG_INDEX_AAA_CMD;
		G_nl_image_config.nl_msg.msg.image_cmd = IMAGE_CMD_START_AAA;
		nl_send_image_msg(G_nl_image_config.nl_msg);
	}
	return ret;
}
int nl_image_stop_aaa(void)
{
	int ret = 0;
	if (G_nl_image_config.nl_connected) {
		G_nl_image_config.nl_msg.pid = 0;
		G_nl_image_config.nl_msg.index = IMAGE_MSG_INDEX_AAA_CMD;
		G_nl_image_config.nl_msg.msg.image_cmd = IMAGE_CMD_STOP_AAA;
		nl_send_image_msg(G_nl_image_config.nl_msg);
	}
	return ret;
}
#endif

#ifdef CONFIG_GUARD_VSYNC_LOSS
static int nl_vsync_init(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
	G_nl_vsync_config.nl_sock = netlink_kernel_create(&init_net,
		NETLINK_VSYNC_PROTOCAL, 0, nl_recv_vsync_msg_handler,
		NULL, THIS_MODULE);
#else
	struct netlink_kernel_cfg cfg = {
		.groups = 0,
		.input	= nl_recv_vsync_msg_handler,
	};
	G_nl_vsync_config.nl_sock = netlink_kernel_create(&init_net,
		NETLINK_VSYNC_PROTOCAL, &cfg);
#endif
	init_completion(&G_nl_vsync_config.vsync_loss_complete);
	G_nl_vsync_config.nl_user_pid = -1;
	G_nl_vsync_config.nl_connected = 0;
	return 0;
}
#else
static int nl_vsync_init(void)
{
	return 0;
}
#endif


#ifdef CONFIG_IAV_CONTROL_AAA
static int nl_image_init(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
	G_nl_image_config.nl_sock = netlink_kernel_create(&init_net,
		NETLINK_IMAGE_PROTOCAL, 0, nl_recv_image_msg_handler,
		NULL, THIS_MODULE);
#else
	struct netlink_kernel_cfg cfg = {
		.groups = 0,
		.input	= nl_recv_image_msg_handler,
	};
	G_nl_image_config.nl_sock = netlink_kernel_create(&init_net,
		NETLINK_IMAGE_PROTOCAL, &cfg);
#endif
	init_completion(&G_nl_image_config.aaa_start_complete);
	init_completion(&G_nl_image_config.aaa_stop_complete);
	init_completion(&G_nl_image_config.aaa_prepare_complete);

	G_nl_image_config.nl_user_pid = -1;
	G_nl_image_config.nl_connected = 0;
	return 0;
}
#else
static int nl_image_init(void)
{
	return 0;
}
#endif

int nl_init(void)
{
	int ret = 0;

	nl_vsync_init();
	nl_image_init();

	return ret;
}


