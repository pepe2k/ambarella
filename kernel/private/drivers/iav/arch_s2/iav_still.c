/*
 * iav_still.c
 *
 * History:
 *	2012/04/13 - [Jian Tang] created file
 *
 * Copyright (C) 2009-2016, Ambarella, Inc.
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


static int init_still_capture(iav_context_t *context, int quality)
{
#if 0
	struct iav_global_info *g_info = context->g_info;

	if (G_iav_obj.op_mode == DSP_UNKNOWN_MODE) {
		iav_printk("check_still: dsp has not been booted yet\n");
		return -EPERM;
	}

	if (!(g_info->pvoutinfo[0]->enabled || g_info->pvoutinfo[1]->enabled)) {
		iav_printk("check_still: Please init vout first!\n");
		return -EPERM;
	}

	if (!(g_info->pvininfo->enabled & IAV_VIN_ENABLED_FOR_STILL)) {
		iav_printk("check_still: Please init vin first!\n");
		return -EPERM;
	}

	jpeg_encode_setup(context, 0, quality);	//still capture only

#endif
	return 0;
}

int iav_init_still_capture(iav_context_t *context, struct iav_still_init_info __user *arg)
{
	struct iav_still_init_info still_info;
	struct iav_global_info *g_info = context->g_info;
	int rval;

	if (copy_from_user(&still_info,
			(struct iav_still_init_info __user *)arg,
			sizeof(still_info)))
		return -EFAULT;

	if ((rval = iav_disable_preview(context)) < 0) {
		iav_printk("iav_disable_preview: %d fail\n", rval);
		return rval;
	}
	DRV_PRINT("enter timer mode\n");
	if ((rval = init_still_capture(context,
		still_info.jpeg_quality)) < 0) {
		iav_printk("iav_init_still_capture: %d fail\n", rval);
		return rval;
	}

	DRV_PRINT("init still done\n");
	g_info->state = IAV_STATE_STILL_CAPTURE;
	return 0;
}

extern void encode_init(void);
int iav_start_still_capture(iav_context_t *context, struct iav_still_cap_info __user *arg)
{
#if 0
	struct iav_global_info *g_info = context->g_info;
	struct iav_still_cap_info cap_info;

	if (g_info->state != IAV_STATE_STILL_CAPTURE) {
		iav_printk("__iav_start_still_capture: bad state %d\n",
			g_info->state);
		return -EPERM;
	}

	if (copy_from_user(&cap_info,
			(struct iav_still_cap_info __user *)arg,
			sizeof(cap_info)))
		return -EFAULT;

	encode_init();

	DRV_PRINT("start capture\n");
	still_capture(context, &cap_info);
	DRV_PRINT("capture done\n");
	wait_dsp_state(DSP_ENCODE_MODE, ENC_BUSY_STATE, 0, "wait for ENC_BUSY_STATE");
	DRV_PRINT("mode ENC_BUSY\n");
#endif
	return 0;
}

int iav_stop_still_capture(iav_context_t *context)
{
#if 0
	/*STILLDEC_FREEZE_CMD dsp_cmd;
	memset(&dsp_cmd, 0, sizeof(dsp_cmd));

	dsp_cmd.cmd_code = CMD_STILL_FREEZE;

	DRV_PRINT("stop capture\n");
	dsp_issue_cmd(&dsp_cmd, sizeof(dsp_cmd));*/

	DRV_PRINT("waitting mode ENC_IDLE\n");
	wait_dsp_state(DSP_ENCODE_MODE, ENC_IDLE_STATE, 0, "wait for ENC_IDLE_STATE");

	DRV_PRINT("mode ENC_IDLE\n");
	enter_timer_mode(context);
	DRV_PRINT("enter timer mode 2\n");

	context->g_info->state = IAV_STATE_IDLE;

	DRV_PRINT("stop bs reader\n");
#endif
	return 0;
}

