/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx078/imx078_video_tbl.c
 *
 * History:
 *    2013/03/11 - [Bingliang Hu] Create
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct imx078_video_info imx078_video_info_table[] = {
	[0] = {
		.format_index	= 0,
		.def_start_x	= 0x42,
		.def_start_y	= 0xe,
		.def_width	= 1984,
		.def_height	= 1116,
		.sync_start	= (0 + 0),
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_RG,
		.slvs_eav_col	= 0x824,
		.slvs_sav2sav_dist	= 0x99f,
	},
	/* note update imx078_VIDEO_FORMAT_REG_TABLE_SIZE */
};
#define imx078_VIDEO_INFO_TABLE_SIZE	ARRAY_SIZE(imx078_video_info_table)
#define imx078_DEFAULT_VIDEO_INDEX	(0)

struct imx078_video_mode imx078_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_1984x1116,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
	{AMBA_VIDEO_MODE_OFF,
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	1,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},
};

#define imx078_VIDEO_MODE_TABLE_SIZE	ARRAY_SIZE(imx078_video_mode_table)
#define imx078_VIDEO_MODE_TABLE_AUTO AMBA_VIDEO_MODE_1984x1116

