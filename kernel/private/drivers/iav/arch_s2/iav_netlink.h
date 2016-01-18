 /*
 * iav_netlink.h
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
#ifndef __IAV_NETLINK_H
#define __IAV_NETLINK_H

#ifdef CONFIG_GUARD_VSYNC_LOSS
#define NETLINK_VSYNC_PROTOCAL	21

typedef enum {
	VSYNC_REQ_VYSNC_LOSS = 0,
} NL_VSYNC_REQ;
typedef enum {
	VSYNC_RESP_VSYNC_SUCCEED = 0,
	VSYNC_RESP_VSYNC_FAIL,
} NL_VSYNC_RESP;
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
typedef struct iav_nl_vsync_config_s {
	u32 				nl_user_pid;
	u32 				nl_connected;
	struct sock 		*nl_sock;
	nl_vsync_msg_t 		nl_msg;
	struct completion 	vsync_loss_complete;
} iav_nl_vsync_config_t;

// method called from iav_api.c
int nl_restore_vsync_loss(void);
#endif

#ifdef CONFIG_IAV_CONTROL_AAA
#define NETLINK_IMAGE_PROTOCAL	20
typedef enum {
	IMAGE_CMD_START_AAA = 0,
	IMAGE_CMD_STOP_AAA,
	IMAGE_CMD_PREPARE_AAA,
} NL_IMAGE_AAA_CMD;
typedef enum {
	IMAGE_STATUS_START_AAA_SUCCESS = 0,
	IMAGE_STATUS_START_AAA_FAIL,
	IMAGE_STATUS_STOP_AAA_SUCCESS,
	IMAGE_STATUS_STOP_AAA_FAIL,
	IMAGE_STATUS_PREPARE_AAA_SUCCESS,
	IMAGE_STATUS_PREPARE_AAA_FAIL,
} NL_IMAGE_AAA_STATUS;
typedef enum {
	IMAGE_SESSION_CMD_CONNECT = 0,
	IMAGE_SESSION_CMD_DISCONNECT,
} NL_IMAGE_SESSION_CMD;
typedef enum {
	IMAGE_SESSION_STATUS_CONNECT_SUCCESS = 0,
	IMAGE_SESSION_STATUS_CONNECT_FAIL,
	IMAGE_SESSION_STATUS_DISCONNECT_SUCCESS,
	IMAGE_SESSION_STATUS_DISCONNECT_FAIL,
} NL_IMAGE_SESSION_STATUS;
typedef enum {
	IMAGE_MSG_INDEX_SESSION_CMD = 0,
	IMAGE_MSG_INDEX_SESSION_STATUS,
	IMAGE_MSG_INDEX_AAA_CMD,
	IMAGE_MSG_INDEX_AAA_STATUS,
} NL_IMAGE_MSG_INDEX;
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
typedef struct iav_nl_image_config_s {
	u32 nl_user_pid;
	u32 nl_connected;
	struct sock *nl_sock;
	nl_image_msg_t nl_msg;
	struct completion aaa_start_complete;
	struct completion aaa_stop_complete;
	struct completion aaa_prepare_complete;

} iav_nl_image_config_t;

int nl_image_prepare_aaa(void);
int nl_image_start_aaa(void);
int nl_image_stop_aaa(void);
#endif

int nl_init(void);

#endif

