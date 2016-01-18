/*
 * kernel/private/drivers/ambarella/vin/decoders/ambdd/ambdd.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>
#include <amba_sensors.h>
#include <vin_pri.h>

#include "arch/ambdd_arch.h"

/* ========================================================================== */
struct ambdd_info {
	struct i2c_client		client;
	struct __amba_vin_source	cap_src;
	u32				input_mode;
};

/* ========================================================================== */
AMBA_VIN_BASIC_PARAM_CALL(ambdd, 0, 0, 0644);

static int video_type = AMBA_VIDEO_TYPE_YUV_656;
module_param(video_type, int, 0644);
MODULE_PARM_DESC(video_type, "Video Input Type");

static int video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
module_param(video_format, int, 0644);
MODULE_PARM_DESC(video_format, "Video Input Format");

static int video_fps = AMBA_VIDEO_FPS_60;
module_param(video_fps, int, 0644);
MODULE_PARM_DESC(video_fps, "Video Input Frame Rate");

static int video_bits = AMBA_VIDEO_BITS_16;
module_param(video_bits, int, 0644);
MODULE_PARM_DESC(video_bits, "Video Input Width");

static int video_mode = AMBA_VIDEO_MODE_1080P;
module_param(video_mode, int, 0644);
MODULE_PARM_DESC(video_mode, "Video Input Mode");

static int video_phase = AMBA_VIDEO_ADAP_VD_HIGH_HD_HIGH;
module_param(video_phase, int, 0644);
MODULE_PARM_DESC(video_phase, "Video Input Sync Phase");

static int video_data_edge = PCLK_FALLING_EDGE;
module_param(video_data_edge, int, 0644);
MODULE_PARM_DESC(video_data_edge, "Video Input Data Edge");

static int video_yuv_mode = 0;
module_param(video_yuv_mode, int, 0644);
MODULE_PARM_DESC(video_yuv_mode, "0 ~ 3 (CrY0CbY1 ~ Y0CbY1Cr)");

static int video_clkout = PLL_CLK_27MHZ;
module_param(video_clkout, int, 0644);
MODULE_PARM_DESC(video_clkout, "Video Clock, Output");

static int video_pixclk = PLL_CLK_74_25MHZ;
module_param(video_pixclk, int, 0644);
MODULE_PARM_DESC(video_pixclk, "Video Pixel Clock");

static int cap_start_x = AMBDD_TBD_CAP_START_X;
static int cap_start_y = AMBDD_TBD_CAP_START_Y;
static int cap_cap_w = 1920;
static int cap_cap_h = 1080;
static int sync_start = AMBDD_TBD_CAP_SYNC_START;

module_param(cap_start_x, int, 0644);
module_param(cap_start_y, int, 0644);
module_param(cap_cap_w, int, 0644);
module_param(cap_cap_h, int, 0644);
module_param(sync_start, int, 0644);

/* ========================================================================== */
static const char ambdd_name[] = "ambdd";
static struct ambdd_info *pinfo = NULL;

/* ========================================================================== */
static void ambdd_print_info(struct __amba_vin_source *src)
{
	struct ambdd_info			*pinfo;

	pinfo = (struct ambdd_info *)src->pinfo;

	vin_notice("video_type = %d\n", video_type);
	vin_notice("video_format = %d\n", video_format);
	vin_notice("video_fps = %d\n", video_fps);
	vin_notice("video_bits = %d\n", video_bits);
	vin_notice("video_mode = %d\n", video_mode);
	vin_notice("video_phase = %d\n", video_phase);
	vin_notice("video_data_edge = %d\n", video_data_edge);
	vin_notice("video_yuv_mode = %d\n", video_yuv_mode);
	vin_notice("video_clkout = %d\n", video_clkout);
	vin_notice("video_pixclk = %d\n", video_pixclk);
	vin_notice("cap_start_x = %d\n", cap_start_x);
	vin_notice("cap_start_y = %d\n", cap_start_y);
	vin_notice("cap_cap_w = %d\n", cap_cap_w);
	vin_notice("cap_cap_h = %d\n", cap_cap_h);
	vin_notice("sync_start = %d\n", sync_start);
	vin_notice("input_mode = 0x%04x\n", pinfo->input_mode);
}

/* ========================================================================== */
#include "arch/ambdd_arch.c"
#include "ambdd_docmd.c"

/* ========================================================================== */
static int __init ambdd_init(void)
{
	int					errorcode = 0;
	struct __amba_vin_source		*src;

	/* Platform Info */
	pinfo = kzalloc(sizeof(struct ambdd_info), GFP_KERNEL);
	if (!pinfo) {
		errorcode = -ENOMEM;
		goto ambdd_init_exit;
	}

	src = &pinfo->cap_src;
	src->id = -1;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_DECODER;
	snprintf(src->name, sizeof(src->name), "%s", ambdd_name);
	src->pinfo = pinfo;
	src->psrcinfo = pinfo;
	src->docmd = ambdd_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errorcode = ambdd_init_vin_clock(src);
	if (errorcode)
		return errorcode;

	errorcode = amba_vin_add_source(src);
	if (errorcode)
		goto ambdd_init_free_pinfo;

	vin_notice("%s[%d@%d] probed!\n", ambdd_name, src->id, src->adapid);
	goto ambdd_init_exit;

ambdd_init_free_pinfo:
	kfree(pinfo);
	pinfo = NULL;

ambdd_init_exit:
	return errorcode;
}

static void __exit ambdd_exit(void)
{
	struct __amba_vin_source		*src;

	if (pinfo) {
		src = &pinfo->cap_src;
		amba_vin_del_source(src);
		kfree(pinfo);
		pinfo = NULL;
		vin_notice("%s[%d@%d] removed!\n",
			ambdd_name, src->id, src->adapid);
	}
}

module_init(ambdd_init);
module_exit(ambdd_exit);

MODULE_DESCRIPTION("Ambarella Dummy Decoder");
MODULE_AUTHOR("Anthony Ginger <hfjiang@ambarella.com>");
MODULE_LICENSE("Proprietary");

