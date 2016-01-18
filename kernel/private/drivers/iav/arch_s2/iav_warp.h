/*
 * iav_warp.h
 *
 * History:
 *	2012/10/10 - [Jian Tang] Created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_WARP_H__
#define __IAV_WARP_H__

typedef enum {
	IAV_BUF_WARP_CLEAR = 0,
	IAV_BUF_WARP_SET = 1,
	IAV_BUF_WARP_ACTION_NUM,
	IAV_BUF_WARP_ACTION_FIRST = 0,
	IAV_BUF_WARP_ACTION_LAST = IAV_BUF_WARP_ACTION_NUM,
} IAV_BUF_WARP_ACTION_FLAG;

extern iav_warp_control_ex_t G_warp_control;

int iav_warp_init(void);

int set_default_warp_dptz_param(iav_buffer_id_t update_buffer_id,
	IAV_BUF_WARP_ACTION_FLAG flag);

int set_default_warp_param(void);

int get_aspect_ratio_in_warp_mode(iav_encode_format_ex_t * format,
	u8 * aspect_ratio_idc, u16 * sar_width, u16 * sar_height);

int iav_set_warp_control_ex(iav_context_t *context,
	struct iav_warp_control_ex_s __user *arg);

int iav_get_warp_control_ex(iav_context_t * context,
	struct iav_warp_control_ex_s __user * arg);

int iav_set_warp_region_dptz_ex(iav_context_t * context,
	struct iav_warp_dptz_ex_s __user *arg);

int iav_get_warp_region_dptz_ex(iav_context_t * context,
	struct iav_warp_dptz_ex_s __user *arg);

int iav_get_warp_proc(iav_context_t * context, void __user * arg);
int iav_cfg_warp_proc(iav_context_t * context, void __user * arg);
int iav_apply_warp_proc(iav_context_t * context, void __user * arg);


/* Chroma Aberration Correction */
void cmd_set_ca_warp(void);
int get_ca_warp_param(iav_ca_warp_t* param);
int cfg_ca_warp_param(iav_ca_warp_t* param);


#endif	//__IAV_WARP_H__

