/*
 * kernel/private/drivers/ambarella/vin/decoders/adv7619/adv7619_reg_tbl.c
 *
 * History:
 *    2009/07/23 - [Qiao Wang] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */

static const struct adv7619_reg_table adv7619_cvbs_share_regs[] = {


	{0xff, 0xff, 0xff},
};

static const struct adv7619_reg_table adv7619_svideo_share_regs[] = {


	{USER_MAP, 0xff, 0xff},
};


static const struct adv7619_reg_table adv7619_ypbpr_share_regs[] = {

	{0xff, 0xff, 0xff},
};

static const struct adv7619_reg_table adv7619_ycbcr_2160p30_fix_regs[] = {
	{USER_MAP, 0xff, 0x80},
	{USER_MAP, 0xff, 0x00},
	{USER_MAP, 0x0c, 0x42},
	{USER_MAP, 0x15, 0x80},
//	{USER_MAP, 0x14, 0x7e},

	{USER_MAP, 0xf4, 0x80},
	{USER_MAP, 0xf5, 0x7c},
	{USER_MAP, 0xf8, 0x4c},
	{USER_MAP, 0xf9, 0x64},
	{USER_MAP, 0xfa, 0x6c},
	{USER_MAP, 0xfb, 0x68},
	{USER_MAP, 0xfd, 0x44},

	{HDMI_MAP, 0xc0, 0x03},

	{USER_MAP, 0x01, 0x06},
	{USER_MAP, 0x02, 0xf7},
	{USER_MAP, 0x03, 0x94},
	{USER_MAP, 0x05, 0x28},
	{USER_MAP, 0x06, 0xa6},
//	{USER_MAP, 0x0c, 0x42},
	{USER_MAP, 0x15, 0x80},
	{USER_MAP, 0x19, 0x83},
	{USER_MAP, 0xdd, 0xa0},
	{USER_MAP, 0x33, 0x40},
//	{CP_MAP, 0xba, 0x01},

//	{KSV_MAP, 0x40, 0x81},

	{DPLL_MAP, 0xb5, 0x00},
	{DPLL_MAP, 0xc3, 0x80},
	{DPLL_MAP, 0xcf, 0x03},
//	{HDMI_MAP, 0x00, 0x08},

	{HDMI_MAP, 0x3e, 0x69},
	{HDMI_MAP, 0x3f, 0x46},
	{HDMI_MAP, 0x4e, 0x7e},
	{HDMI_MAP, 0x4f, 0x42},
	{HDMI_MAP, 0x02, 0x03},
	
	{HDMI_MAP, 0x57, 0xa3},
	{HDMI_MAP, 0x58, 0x07},
	{HDMI_MAP, 0x83, 0xfc},
	{HDMI_MAP, 0x84, 0x03},
	{HDMI_MAP, 0x85, 0x10},
	{HDMI_MAP, 0x86, 0x9b},
	{HDMI_MAP, 0x89, 0x03},
	{HDMI_MAP, 0x9b, 0x03},
	
	{0xff, 0xff, 0xff},
};

static const struct adv7619_reg_table adv7619_ycbcr_1080p30_fix_regs[] = {
	{USER_MAP, 0xff, 0x80},
	{USER_MAP, 0xff, 0x00},
	{USER_MAP, 0x0c, 0x42},
	{USER_MAP, 0x15, 0x80},
//	{USER_MAP, 0x14, 0x7e},

	{USER_MAP, 0xf4, 0x80},
	{USER_MAP, 0xf5, 0x7c},
	{USER_MAP, 0xf8, 0x4c},
	{USER_MAP, 0xf9, 0x64},
	{USER_MAP, 0xfa, 0x6c},
	{USER_MAP, 0xfb, 0x68},
	{USER_MAP, 0xfd, 0x44},
	{HDMI_MAP, 0xc0, 0x03},

	{USER_MAP, 0x01, 0x06},
	{USER_MAP, 0x02, 0xf7},
	{USER_MAP, 0x03, 0x94},
	{USER_MAP, 0x05, 0x28},
	{USER_MAP, 0x06, 0xa6},
	{USER_MAP, 0x0c, 0x42},
	{USER_MAP, 0x15, 0x80},
	{USER_MAP, 0x19, 0x83},
	{USER_MAP, 0x33, 0x40},

	{CP_MAP, 0xba, 0x01},

	{KSV_MAP, 0x40, 0x81},

	{DPLL_MAP, 0xb5, 0x01},


	{HDMI_MAP, 0x00, 0x08},
	{HDMI_MAP, 0x02, 0x03},
	{HDMI_MAP, 0x3e, 0x69},
	{HDMI_MAP, 0x3f, 0x46},
	{HDMI_MAP, 0x4e, 0x7e},
	{HDMI_MAP, 0x4f, 0x42},
	{HDMI_MAP, 0x57, 0xa3},
	{HDMI_MAP, 0x58, 0x07},
	{HDMI_MAP, 0x83, 0xfc},
	{HDMI_MAP, 0x84, 0x03},
	{HDMI_MAP, 0x85, 0x10},
	{HDMI_MAP, 0x86, 0x9b},
	{HDMI_MAP, 0x89, 0x03},
	{HDMI_MAP, 0x9b, 0x03},
	
	{0xff, 0xff, 0xff},
};


static const struct adv7619_reg_table adv7619_hdmi_share_regs[] = {

	{0xff, 0xff, 0xff},
};


static const struct adv7619_reg_table adv7619_vga_share_regs[] = {

	{0xff, 0xff, 0xff},
};


struct adv7619_vid_stds adv7619_vid_std_table[] = {
	{//YUV_MODE_1080P30_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_1080P,
	.cap_cap_w = AMBA_VIDEO_1080P_W,
	.cap_cap_h = AMBA_VIDEO_1080P_H,
#if defined(ADV7619_PREFER_EMBMODE)
	.cap_start_x = ADV7619_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7619_EMBMODE_CAP_START_Y,
	.sync_start = ADV7619_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7619_1080P60_601_CAP_START_X,
	.cap_start_y = ADV7619_1080P60_601_CAP_START_Y,
	.sync_start = ADV7619_1080P60_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_NTSC,
	.frame_rate = AMBA_VIDEO_FPS_30,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_148_5MHZ,
	.so_pclk_freq_hz = PLL_CLK_148_5MHZ,
	.fix_reg_table = adv7619_ycbcr_1080p30_fix_regs,

	.bl_low = 3180,
	.bl_upper = 3223,
	.scf_low = 1075,
	.scf_upper = 1175,
	.scvs_low = 4,
	.scvs_upper = 8,
	},
};

#define ADV7619_VID_STD_NUM 			ARRAY_SIZE(adv7619_vid_std_table)

struct adv7619_source_info adv7619_source_table[] = {
//	{"YPbPr",	0x00,	ADV7619_PROCESSOR_CP,	AMBA_VIN_DECODER_CHANNEL_TYPE_YPBPR,	&adv7619_ypbpr_share_regs[0]},
	{"HDMI-A",	0x10,	ADV7619_PROCESSOR_HDMI,AMBA_VIN_DECODER_CHANNEL_TYPE_HDMI,	&adv7619_hdmi_share_regs[0]},
//	{"CVBS-Red",	0x03,	ADV7619_PROCESSOR_SDP,	AMBA_VIN_DECODER_CHANNEL_TYPE_CVBS,	&adv7619_cvbs_share_regs[0]},
//	{"VGA",		0x00,	ADV7619_PROCESSOR_CP,	AMBA_VIN_DECODER_CHANNEL_TYPE_VGA,	&adv7619_vga_share_regs[0]},
//	{"SVideo",	0x00,	ADV7403_PROCESSOR_SDP,	AMBA_VIN_DECODER_CHANNEL_TYPE_SVIDEO,	&adv7619_svideo_share_regs[0]},
};

#define ADV7619_SOURCE_NUM 			ARRAY_SIZE(adv7619_source_table)

