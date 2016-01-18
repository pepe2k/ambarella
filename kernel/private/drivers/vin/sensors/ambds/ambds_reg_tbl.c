/*
 * kernel/private/drivers/ambarella/vin/sensors/ambds/ambds_reg_tbl.c
 *
 * History:
 *    2014/12/30 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct ambds_video_format_reg_table ambds_video_format_tbl = {
	.reg = {
	},
	.table[0] = {		// 1080p
		.ext_reg_fill = NULL,
		.data = {
		},
		.width = 1920,
		.height = 1080,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_YUV_656,
		.bits = AMBA_VIDEO_BITS_8,
		.ratio = AMBA_VIDEO_RATIO_4_3,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= 1920 * 2, /* EAV position */
		.slvs_sav2sav_dist = 2200 * 2, /* SAV + Effective Pixel + EAV + HB */
	},
	.table[1] = {		// 4K
		.ext_reg_fill = NULL,
		.data = {
		},
		.width = 3840,
		.height = 2160,
		.format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type = AMBA_VIDEO_TYPE_YUV_656,
		.bits = AMBA_VIDEO_BITS_8,
		.ratio = AMBA_VIDEO_RATIO_16_9,
		.srm = 0,
		.sw_limit = -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
		.slvs_eav_col	= 3840 * 2, /* EAV position */
		.slvs_sav2sav_dist = 4400 * 2, /* SAV + Effective Pixel + EAV + HB */
	},
	/* add video format table here, if necessary */
};

