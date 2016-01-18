/*
 * kernel/private/drivers/ambarella/vin/decoders/adv7441a/adv7441a_reg_tbl.c
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

static const struct adv7441a_reg_table adv7441a_cvbs_share_regs[] = {
	{USER_MAP, 0x00, 0x03},
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x3c, 0xad},
	{USER_MAP, 0x04, 0x47},
	{USER_MAP, 0x17, 0x41},
	{USER_MAP, 0x1d, 0x40},

	{USER_MAP, 0x31, 0x1a},
	{USER_MAP, 0x32, 0x81},
	{USER_MAP, 0x33, 0x84},
	{USER_MAP, 0x34, 0x00},
	{USER_MAP, 0x35, 0x00},
	{USER_MAP, 0x36, 0x7d},
	{USER_MAP, 0x37, 0xa1},

	{USER_MAP, 0x3a, 0x07},
	{USER_MAP, 0x3c, 0xa8},
	{USER_MAP, 0x47, 0x0a},
	{USER_MAP, 0xf3, 0x07},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_svideo_share_regs[] = {
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x04, 0x57},
	{USER_MAP, 0x1d, 0x47},
	{USER_MAP, 0x0f, 0x40},
	{USER_MAP, 0xe8, 0x65},
	{USER_MAP, 0xea, 0x63},
	{USER_MAP, 0x3a, 0x13},
	{USER_MAP, 0x3d, 0xa2},
	{USER_MAP, 0x3e, 0x64},
	{USER_MAP, 0x3f, 0xe4},	/* 10 */
	{USER_MAP, 0x69, 0x03},	/* Y = AIN11, CVBS = AIN11 */
	{USER_MAP, 0xf3, 0x03},
	{USER_MAP, 0xf9, 0x03},
	{USER_MAP, 0x0e, 0x80},
	{USER_MAP, 0x81, 0x30},
	{USER_MAP, 0x90, 0xc9},
	{USER_MAP, 0x91, 0x40},
	{USER_MAP, 0x92, 0x3c},
	{USER_MAP, 0x93, 0xca},
	{USER_MAP, 0x94, 0xd5},	/* 20 */
	{USER_MAP, 0xb1, 0xff},
	{USER_MAP, 0xb6, 0x08},
	{USER_MAP, 0xc0, 0x9a},
	{USER_MAP, 0xcf, 0x50},
	{USER_MAP, 0xd0, 0x4e},
	{USER_MAP, 0xd6, 0xdd},
	{USER_MAP, 0xd7, 0xe2},
	{USER_MAP, 0xe5, 0x51},
	{USER_MAP, 0x0e, 0x00},	/* 30 */
	{USER_MAP, 0x10, 0xff},

	{USER_MAP, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_sdp_480i_fix_regs[] = {
	{USER_MAP, 0xe5, 0x41},
	{USER_MAP, 0xe6, 0x84},
	{USER_MAP, 0xe7, 0x06},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_sdp_576i_fix_regs[] = {
	{USER_MAP, 0xe8, 0x41},
	{USER_MAP, 0xe9, 0x84},
	{USER_MAP, 0xea, 0x06},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_share_regs[] = {
	{USER_MAP, 0x00, 0x09},
	{USER_MAP, 0x1d, 0x40},
	{USER_MAP, 0x3c, 0xa8},
	{USER_MAP, 0x47, 0x0a},
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x6b, 0xd3},
#else
	{USER_MAP, 0x6b, 0xc3},
#endif
	{USER_MAP, 0x85, 0x19},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_480i_fix_regs[] = {
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x05, 0x00},
	{USER_MAP, 0x06, 0x0e},
	{USER_MAP, 0xc9, 0x0c},
#else
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x00},
	{USER_MAP, 0x06, 0x0c},
	{USER_MAP, 0xc9, 0x04},
#endif
	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_576i_fix_regs[] = {
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x05, 0x00},
	{USER_MAP, 0x06, 0x0f},
	{USER_MAP, 0xc9, 0x0c},
#else
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x00},
	{USER_MAP, 0x06, 0x0d},
	{USER_MAP, 0xc9, 0x04},
#endif
	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_480p_fix_regs[] = {
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x06},
	{USER_MAP, 0xc9, 0x0c},
#else
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x06},
	{USER_MAP, 0xc9, 0x04},
#endif
	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_576p_fix_regs[] = {
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x03, 0x0c},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x07},
	{USER_MAP, 0xc9, 0x0c},
#else
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x07},
	{USER_MAP, 0xc9, 0x04},
#endif
	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_720p60_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x0a},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_720p50_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x2a},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_1080i60_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x0c},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_1080i50_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x2c},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_1080p60_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x0b},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_ypbpr_1080p50_fix_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x2b},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_share_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x06},
	{USER_MAP, 0x1d, 0x40},
	{USER_MAP, 0x68, 0xf0},
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x6b, 0xd3},
#else
	{USER_MAP, 0x6b, 0xc3},
#endif
	{USER_MAP, 0xba, 0xa0},
	{USER_MAP, 0xc8, 0x08},
	{USER_MAP, 0xf4, 0x3f},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_720p50_fix_regs[] = {
	{USER_MAP, 0x06, 0x0a},
	{USER_MAP, 0x1d, 0x47},
	{USER_MAP, 0x3a, 0x20},
	{USER_MAP, 0x3c, 0x5c},
	{USER_MAP, 0x6b, 0xc1},
	{USER_MAP, 0x85, 0x19},
	{USER_MAP, 0x87, 0xe7},
	{USER_MAP, 0x88, 0xbc},
	{USER_MAP, 0x8f, 0x02},
	{USER_MAP, 0x90, 0xfc},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_1080i50_fix_regs[] = {
	{USER_MAP, 0x06, 0x0c},
	{USER_MAP, 0x1d, 0x47},
	{USER_MAP, 0x3a, 0x20},
	{USER_MAP, 0x3c, 0x5d},
	{USER_MAP, 0x6b, 0xc1},
	{USER_MAP, 0x85, 0x19},
	{USER_MAP, 0x87, 0xea},
	{USER_MAP, 0x88, 0x50},
	{USER_MAP, 0x8f, 0x03},
	{USER_MAP, 0x90, 0xfa},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_1080p50_fix_regs[] = {
	{USER_MAP, 0x06, 0x2b},
	{USER_MAP, 0x1d, 0x47},
	{USER_MAP, 0x3a, 0x20},
	{USER_MAP, 0x3c, 0x5d},
	{USER_MAP, 0x6b, 0xc1},
	{USER_MAP, 0x85, 0x19},
	{USER_MAP, 0x87, 0xea},
	{USER_MAP, 0x88, 0x50},
	{USER_MAP, 0x8f, 0x03},
	{USER_MAP, 0x90, 0xfa},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_1080p30_fix_regs[] = {
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x4C},
	{USER_MAP, 0x91, 0x10},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_1080p25_fix_regs[] = {
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x6C},
	{USER_MAP, 0x91, 0x10},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_hdmi_1080p24_fix_regs[] = {
	{USER_MAP, 0x05, 0x01},
	{USER_MAP, 0x06, 0x8C},
	{USER_MAP, 0x91, 0x10},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_vga_share_regs[] = {
	{USER_MAP, 0x03, 0x08},
	{USER_MAP, 0x05, 0x02},
	{USER_MAP, 0x1d, 0x47},

	{USER_MAP, 0x68, 0xFC},
	{USER_MAP, 0x6A, 0x00},
#if defined(ADV7441A_PREFER_EMBMODE)
	{USER_MAP, 0x6b, 0xd3},
#else
	{USER_MAP, 0x6b, 0xc3},
#endif
	{USER_MAP, 0x73, 0x90},
	{USER_MAP, 0xF4, 0x3F},
	{USER_MAP, 0xFA, 0x84},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_vga_vga_fix_regs[] = {
	{USER_MAP, 0x06, 0x08},
	{USER_MAP, 0x3A, 0x11},
	{USER_MAP, 0x3C, 0x5C},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_vga_svga_fix_regs[] = {
	{USER_MAP, 0x06, 0x01},
	{USER_MAP, 0x3A, 0x11},
	{USER_MAP, 0x3C, 0x5D},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_vga_xga_fix_regs[] = {
	{USER_MAP, 0x06, 0x0C},
	{USER_MAP, 0x3A, 0x21},
	{USER_MAP, 0x3C, 0x5D},

	{0xff, 0xff, 0xff},
};

static const struct adv7441a_reg_table adv7441a_vga_sxga_fix_regs[] = {
	{USER_MAP, 0x06, 0x05},
	{USER_MAP, 0x3A, 0x31},
	{USER_MAP, 0x3C, 0x5D},

	{0xff, 0xff, 0xff},
};

struct adv7441a_vid_stds adv7441a_vid_std_table[] = {
	{//AMBA_VIDEO_MODE_480I
	.cap_vin_mode = AMBA_VIDEO_MODE_480I,
	.cap_cap_w = AMBA_VIDEO_NTSC_W,
	.cap_cap_h = AMBA_VIDEO_NTSC_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_8,
#else
	.cap_start_x = ADV7441A_480I_601_CAP_START_X,
	.cap_start_y = ADV7441A_480I_601_CAP_START_Y,
	.sync_start = ADV7441A_480I_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.aspect_ratio = AMBA_VIDEO_RATIO_4_3,
	.frame_rate = AMBA_VIDEO_FPS_59_94,
	.video_system = AMBA_VIDEO_SYSTEM_NTSC,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC,
	.so_freq_hz = PLL_CLK_13_5MHZ,
	.so_pclk_freq_hz = PLL_CLK_13_5MHZ,
	.fix_reg_table = adv7441a_ypbpr_480i_fix_regs,

	.bl_low = 13640,
	.bl_upper = 13800,
	.scf_low = 261,
	.scf_upper = 311,
	.scvs_low = 2,
	.scvs_upper = 4,
	},

	{//AMBA_VIDEO_MODE_576I
	.cap_vin_mode = AMBA_VIDEO_MODE_576I,
	.cap_cap_w = AMBA_VIDEO_PAL_W,
	.cap_cap_h = AMBA_VIDEO_PAL_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_8,
#else
	.cap_start_x = ADV7441A_576I_601_CAP_START_X,
	.cap_start_y = ADV7441A_576I_601_CAP_START_Y,
	.sync_start = ADV7441A_576I_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.aspect_ratio = AMBA_VIDEO_RATIO_4_3,
	.frame_rate = AMBA_VIDEO_FPS_50,
	.video_system = AMBA_VIDEO_SYSTEM_PAL,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC,
	.so_freq_hz = PLL_CLK_13_5MHZ,
	.so_pclk_freq_hz = PLL_CLK_13_5MHZ,
	.fix_reg_table = adv7441a_ypbpr_576i_fix_regs,

	.bl_low = 13744,
	.bl_upper = 13904,
	.scf_low = 310,
	.scf_upper = 384,
	.scvs_low = 2,
	.scvs_upper = 4,
	},

	{//AMBA_VIDEO_MODE_D1_NTSC
	.cap_vin_mode = AMBA_VIDEO_MODE_D1_NTSC,
	.cap_cap_w = AMBA_VIDEO_NTSC_W,
	.cap_cap_h = AMBA_VIDEO_NTSC_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_8,
#else
	.cap_start_x = ADV7441A_480P_601_CAP_START_X,
	.cap_start_y = ADV7441A_480P_601_CAP_START_Y,
	.sync_start = ADV7441A_480P_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.aspect_ratio = AMBA_VIDEO_RATIO_4_3,
	.frame_rate = AMBA_VIDEO_FPS_59_94,//AMBA_VIDEO_FPS_60,
	.video_system = AMBA_VIDEO_SYSTEM_NTSC,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_27MHZ,
	.so_pclk_freq_hz = PLL_CLK_27MHZ,
	.fix_reg_table = adv7441a_ypbpr_480p_fix_regs,

	.bl_low = 6800,
	.bl_upper = 6880,
	.scf_low = 524,
	.scf_upper = 580,
	.scvs_low = 4,
	.scvs_upper = 8,
	},

	{//AMBA_VIDEO_MODE_D1_PAL
	.cap_vin_mode = AMBA_VIDEO_MODE_D1_PAL,
	.cap_cap_w = AMBA_VIDEO_PAL_W,
	.cap_cap_h = AMBA_VIDEO_PAL_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_8,
#else
	.cap_start_x = ADV7441A_576P_601_CAP_START_X,
	.cap_start_y = ADV7441A_576P_601_CAP_START_Y,
	.sync_start = ADV7441A_576P_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_PAL,
	.frame_rate = AMBA_VIDEO_FPS_50,
	.aspect_ratio = AMBA_VIDEO_RATIO_4_3,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_27MHZ,
	.so_pclk_freq_hz = PLL_CLK_27MHZ,
	.fix_reg_table = adv7441a_ypbpr_576p_fix_regs,

	.bl_low = 6872,
	.bl_upper = 6952,
	.scf_low = 624,
	.scf_upper = 690,
	.scvs_low = 3,
	.scvs_upper = 7,
	},

	{//YUV_MODE_720P60_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_720P,
	.cap_cap_w = AMBA_VIDEO_720P_W,
	.cap_cap_h = AMBA_VIDEO_720P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_720P60_601_CAP_START_X,
	.cap_start_y = ADV7441A_720P60_601_CAP_START_Y,
	.sync_start = ADV7441A_720P60_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_NTSC,
	.frame_rate = AMBA_VIDEO_FPS_59_94, //AMBA_VIDEO_FPS_60,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_74_25MHZ,
	.so_pclk_freq_hz = PLL_CLK_74_25MHZ,
	.fix_reg_table = adv7441a_ypbpr_720p60_fix_regs,

	.bl_low = 4760,
	.bl_upper = 4840,
	.scf_low = 724,
	.scf_upper = 776,
	.scvs_low = 3,
	.scvs_upper = 7,
	},

	{//YUV_MODE_720P60_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_720P,
	.cap_cap_w = AMBA_VIDEO_720P_W,
	.cap_cap_h = AMBA_VIDEO_720P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_720P60_601_CAP_START_X,
	.cap_start_y = ADV7441A_720P60_601_CAP_START_Y,
	.sync_start = ADV7441A_720P60_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_NTSC,
	.frame_rate = AMBA_VIDEO_FPS_60,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_74_25MHZ,
	.so_pclk_freq_hz = PLL_CLK_74_25MHZ,
	.fix_reg_table = adv7441a_ypbpr_720p60_fix_regs,

	.bl_low = 4760,
	.bl_upper = 4840,
	.scf_low = 729,
	.scf_upper = 781,
	.scvs_low = 7,
	.scvs_upper = 11,
	},

	{//YUV_MODE_720P50_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_720P,
	.cap_cap_w = AMBA_VIDEO_720P_W,
	.cap_cap_h = AMBA_VIDEO_720P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_720P60_601_CAP_START_X,
	.cap_start_y = ADV7441A_720P60_601_CAP_START_Y,
	.sync_start = ADV7441A_720P60_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_PAL,
	.frame_rate = AMBA_VIDEO_FPS_50,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_74_25MHZ,
	.so_pclk_freq_hz = PLL_CLK_74_25MHZ,
	.fix_reg_table = adv7441a_ypbpr_720p50_fix_regs,

	.bl_low = 5710,
	.bl_upper = 5790,
	.scf_low = 724,
	.scf_upper = 776,
	.scvs_low = 3,
	.scvs_upper = 7,
	},

	{//YUV_MODE_720P50_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_720P,
	.cap_cap_w = AMBA_VIDEO_720P_W,
	.cap_cap_h = AMBA_VIDEO_720P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_720P60_601_CAP_START_X,
	.cap_start_y = ADV7441A_720P60_601_CAP_START_Y,
	.sync_start = ADV7441A_720P60_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_PAL,
	.frame_rate = AMBA_VIDEO_FPS_50,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_74_25MHZ,
	.so_pclk_freq_hz = PLL_CLK_74_25MHZ,
	.fix_reg_table = adv7441a_ypbpr_720p50_fix_regs,

	.bl_low = 5710,
	.bl_upper = 5790,
	.scf_low = 729,
	.scf_upper = 781,
	.scvs_low = 7,
	.scvs_upper = 11,
	},

	{//YUV_MODE_1080I60_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_1080P,
	.cap_cap_w = AMBA_VIDEO_1080P_W,
	.cap_cap_h = AMBA_VIDEO_1080P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_1080I60_601_CAP_START_X,
	.cap_start_y = ADV7441A_1080I60_601_CAP_START_Y,
	.sync_start = ADV7441A_1080I60_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_NTSC,
	.frame_rate = AMBA_VIDEO_FPS_59_94,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC,
	.so_freq_hz = PLL_CLK_74_25D1001MHZ,
	.so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ,
	.fix_reg_table = adv7441a_ypbpr_1080i60_fix_regs,

	.bl_low = 6360,
	.bl_upper = 6440,
	.scf_low = 553,
	.scf_upper = 605,
	.scvs_low = 18,
	.scvs_upper = 22,
	},

	{//YUV_MODE_1080I60_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_1080P,
	.cap_cap_w = AMBA_VIDEO_1080P_W,
	.cap_cap_h = AMBA_VIDEO_1080P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_1080I60_601_CAP_START_X,
	.cap_start_y = ADV7441A_1080I60_601_CAP_START_Y,
	.sync_start = ADV7441A_1080I60_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_NTSC,
	.frame_rate = AMBA_VIDEO_FPS_59_94,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC,
	.so_freq_hz = PLL_CLK_74_25D1001MHZ,
	.so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ,
	.fix_reg_table = adv7441a_ypbpr_1080i60_fix_regs,

	.bl_low = 6360,
	.bl_upper = 6440,
	.scf_low = 536,
	.scf_upper = 588,
	.scvs_low = 2,
	.scvs_upper = 8,
	},

	{//YUV_MODE_1080I50_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_1080P,
	.cap_cap_w = AMBA_VIDEO_1080P_W,
	.cap_cap_h = AMBA_VIDEO_1080P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_1080I50_601_CAP_START_X,
	.cap_start_y = ADV7441A_1080I50_601_CAP_START_Y,
	.sync_start = ADV7441A_1080I50_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_PAL,
	.frame_rate = AMBA_VIDEO_FPS_50,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC,
	.so_freq_hz = PLL_CLK_74_25MHZ,
	.so_pclk_freq_hz = PLL_CLK_74_25MHZ,
	.fix_reg_table = adv7441a_ypbpr_1080i50_fix_regs,

	.bl_low = 7632,
	.bl_upper = 7712,
	.scf_low = 553,
	.scf_upper = 605,
	.scvs_low = 18,
	.scvs_upper = 22,
	},

	{//YUV_MODE_1080I50_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_1080P,
	.cap_cap_w = AMBA_VIDEO_1080P_W,
	.cap_cap_h = AMBA_VIDEO_1080P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_1080I50_601_CAP_START_X,
	.cap_start_y = ADV7441A_1080I50_601_CAP_START_Y,
	.sync_start = ADV7441A_1080I50_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_INTERLACE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_PAL,
	.frame_rate = AMBA_VIDEO_FPS_50,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC,
	.so_freq_hz = PLL_CLK_74_25MHZ,
	.so_pclk_freq_hz = PLL_CLK_74_25MHZ,
	.fix_reg_table = adv7441a_ypbpr_1080i50_fix_regs,

	.bl_low = 7632,
	.bl_upper = 7712,
	.scf_low = 536,
	.scf_upper = 588,
	.scvs_low = 3,
	.scvs_upper = 7,
	},

	{//YUV_MODE_1080P60_601_W16	TBD...
	.cap_vin_mode = AMBA_VIDEO_MODE_1080P,
	.cap_cap_w = AMBA_VIDEO_1080P_W,
	.cap_cap_h = AMBA_VIDEO_1080P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_1080P60_601_CAP_START_X,
	.cap_start_y = ADV7441A_1080P60_601_CAP_START_Y,
	.sync_start = ADV7441A_1080P60_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_NTSC,
	.frame_rate = AMBA_VIDEO_FPS_60,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_148_5MHZ,
	.so_pclk_freq_hz = PLL_CLK_148_5MHZ,
	.fix_reg_table = adv7441a_ypbpr_1080p60_fix_regs,

	.bl_low = 3183,
	.bl_upper = 3223,
	.scf_low = 1080,
	.scf_upper = 1180,
	.scvs_low = 8,
	.scvs_upper = 12,
	},

	{//YUV_MODE_1080P60_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_1080P,
	.cap_cap_w = AMBA_VIDEO_1080P_W,
	.cap_cap_h = AMBA_VIDEO_1080P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_1080P60_601_CAP_START_X,
	.cap_start_y = ADV7441A_1080P60_601_CAP_START_Y,
	.sync_start = ADV7441A_1080P60_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_NTSC,
	.frame_rate = AMBA_VIDEO_FPS_60,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_148_5MHZ,
	.so_pclk_freq_hz = PLL_CLK_148_5MHZ,
	.fix_reg_table = adv7441a_ypbpr_1080p60_fix_regs,

	.bl_low = 3180,
	.bl_upper = 3223,
	.scf_low = 1075,
	.scf_upper = 1175,
	.scvs_low = 4,
	.scvs_upper = 8,
	},

	{//YUV_MODE_1080P50_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_1080P,
	.cap_cap_w = AMBA_VIDEO_1080P_W,
	.cap_cap_h = AMBA_VIDEO_1080P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_1080P50_601_CAP_START_X,
	.cap_start_y = ADV7441A_1080P50_601_CAP_START_Y,
	.sync_start = ADV7441A_1080P50_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_PAL,
	.frame_rate = AMBA_VIDEO_FPS_50,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_148_5MHZ,
	.so_pclk_freq_hz = PLL_CLK_148_5MHZ,
	.fix_reg_table = adv7441a_ypbpr_1080p50_fix_regs,

	.bl_low = 3428,
	.bl_upper = 3468,
	.scf_low = 1080,
	.scf_upper = 1180,
	.scvs_low = 7,
	.scvs_upper = 11,
	},

	{//YUV_MODE_1080P50_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_1080P,
	.cap_cap_w = AMBA_VIDEO_1080P_W,
	.cap_cap_h = AMBA_VIDEO_1080P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_1080P50_601_CAP_START_X,
	.cap_start_y = ADV7441A_1080P50_601_CAP_START_Y,
	.sync_start = ADV7441A_1080P50_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_PAL,
	.frame_rate = AMBA_VIDEO_FPS_50,
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_148_5MHZ,
	.so_pclk_freq_hz = PLL_CLK_148_5MHZ,
	.fix_reg_table = adv7441a_ypbpr_1080p50_fix_regs,

	.bl_low = 3428,
	.bl_upper = 3468,
	.scf_low = 1075,
	.scf_upper = 1175,
	.scvs_low = 4,
	.scvs_upper = 8,
	},

	{//YUV_MODE_1080P24_601_W16
	.cap_vin_mode = AMBA_VIDEO_MODE_1080P,
	.cap_cap_w = AMBA_VIDEO_1080P_W,
	.cap_cap_h = AMBA_VIDEO_1080P_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_1080P60_601_CAP_START_X,
	.cap_start_y = ADV7441A_1080P60_601_CAP_START_Y,
	.sync_start = ADV7441A_1080P60_601_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_NTSC,
	.frame_rate = AMBA_VIDEO_FPS(24),
	.aspect_ratio = AMBA_VIDEO_RATIO_16_9,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_148_5MHZ,
	.so_pclk_freq_hz = PLL_CLK_148_5MHZ,
	.fix_reg_table = adv7441a_ypbpr_1080p60_fix_regs,

	.bl_low = 7958,
	.bl_upper = 8038,
	.scf_low = 1075,
	.scf_upper = 1175,
	.scvs_low = 3,
	.scvs_upper = 7,
	},

/* ========================================================================== */
	{//CP VGA 640x480@60
	.cap_vin_mode = AMBA_VIDEO_MODE_VGA,
	.cap_cap_w = AMBA_VIDEO_VGA_W,
	.cap_cap_h = AMBA_VIDEO_VGA_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_VGA_CAP_START_X,
	.cap_start_y = ADV7441A_VGA_CAP_START_Y,
	.sync_start = ADV7441A_VGA_CAP_START_X,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_AUTO,
	.frame_rate = AMBA_VIDEO_FPS_60,
	.aspect_ratio = AMBA_VIDEO_RATIO_AUTO,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_27MHZ,
	.so_pclk_freq_hz = PLL_CLK_27MHZ,
	.fix_reg_table = adv7441a_vga_vga_fix_regs,

	.bl_low = 6834,
	.bl_upper = 6874,
	.scf_low = 474,
	.scf_upper = 574,
	.scvs_low = 0,
	.scvs_upper = 4,
	},

	{//CP SVGA 800x600@60
	.cap_vin_mode = AMBA_VIDEO_MODE_SVGA,
	.cap_cap_w = AMBA_VIDEO_SVGA_W,
	.cap_cap_h = AMBA_VIDEO_SVGA_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_SVGA_CAP_START_X,
	.cap_start_y = ADV7441A_SVGA_CAP_START_Y,
	.sync_start = ADV7441A_SVGA_CAP_START_X,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_AUTO,
	.frame_rate = AMBA_VIDEO_FPS_60,
	.aspect_ratio = AMBA_VIDEO_RATIO_AUTO,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_54MHZ,
	.so_pclk_freq_hz = PLL_CLK_54MHZ,
	.fix_reg_table = adv7441a_vga_svga_fix_regs,

	.bl_low = 5687,
	.bl_upper = 5727,
	.scf_low = 577,
	.scf_upper = 677,
	.scvs_low = 2,
	.scvs_upper = 6,
	},

	{//CP XGA 1024x768@60
	.cap_vin_mode = AMBA_VIDEO_MODE_XGA,
	.cap_cap_w = AMBA_VIDEO_XGA_W,
	.cap_cap_h = AMBA_VIDEO_XGA_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_XGA_CAP_START_X,
	.cap_start_y = ADV7441A_XGA_CAP_START_Y,
	.sync_start = ADV7441A_XGA_CAP_START_X,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_AUTO,
	.frame_rate = AMBA_VIDEO_FPS_60,
	.aspect_ratio = AMBA_VIDEO_RATIO_AUTO,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_74_25MHZ,
	.so_pclk_freq_hz = PLL_CLK_74_25MHZ,
	.fix_reg_table = adv7441a_vga_xga_fix_regs,

	.bl_low = 4439,
	.bl_upper = 4479,
	.scf_low = 755,
	.scf_upper = 855,
	.scvs_low = 4,
	.scvs_upper = 8,
	},

	{//CP SXGA 1280x1024@60
	.cap_vin_mode = AMBA_VIDEO_MODE_SXGA,
	.cap_cap_w = AMBA_VIDEO_SXGA_W,
	.cap_cap_h = AMBA_VIDEO_SXGA_H,
#if defined(ADV7441A_PREFER_EMBMODE)
	.cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
	.cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
	.sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
	.input_type = AMBA_VIDEO_TYPE_YUV_656,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#else
	.cap_start_x = ADV7441A_SXGA_CAP_START_X,
	.cap_start_y = ADV7441A_SXGA_CAP_START_Y,
	.sync_start = ADV7441A_SXGA_CAP_START_X,
	.input_type = AMBA_VIDEO_TYPE_YUV_601,
	.video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
	.bit_resolution = AMBA_VIDEO_BITS_16,
#endif
	.video_system = AMBA_VIDEO_SYSTEM_AUTO,
	.frame_rate = AMBA_VIDEO_FPS_60,
	.aspect_ratio = AMBA_VIDEO_RATIO_AUTO,
	.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG,
	.so_freq_hz = PLL_CLK_108MHZ,
	.so_pclk_freq_hz = PLL_CLK_108MHZ,
	.fix_reg_table = adv7441a_vga_sxga_fix_regs,

	.bl_low = 3346,
	.bl_upper = 3386,
	.scf_low = 1015,
	.scf_upper = 1115,
	.scvs_low = 1,
	.scvs_upper = 5,
	},
};

#define ADV7441A_VID_STD_NUM 			ARRAY_SIZE(adv7441a_vid_std_table)

struct adv7441a_source_info adv7441a_source_table[] = {
	{"YPbPr",	0x00,	ADV7441A_PROCESSOR_CP,	AMBA_VIN_DECODER_CHANNEL_TYPE_YPBPR,	&adv7441a_ypbpr_share_regs[0]},
	{"HDMI-A",	0x10,	ADV7441A_PROCESSOR_HDMI,AMBA_VIN_DECODER_CHANNEL_TYPE_HDMI,	&adv7441a_hdmi_share_regs[0]},
	{"CVBS-Red",	0x03,	ADV7441A_PROCESSOR_SDP,	AMBA_VIN_DECODER_CHANNEL_TYPE_CVBS,	&adv7441a_cvbs_share_regs[0]},
	{"VGA",		0x00,	ADV7441A_PROCESSOR_CP,	AMBA_VIN_DECODER_CHANNEL_TYPE_VGA,	&adv7441a_vga_share_regs[0]},
//	{"SVideo",	0x00,	ADV7403_PROCESSOR_SDP,	AMBA_VIN_DECODER_CHANNEL_TYPE_SVIDEO,	&adv7441a_svideo_share_regs[0]},
};

#define ADV7441A_SOURCE_NUM 			ARRAY_SIZE(adv7441a_source_table)

