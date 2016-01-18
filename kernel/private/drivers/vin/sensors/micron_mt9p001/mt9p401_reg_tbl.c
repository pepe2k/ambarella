/*
 * kernel/private/drivers/ambarella/vin/sensors/micron_mt9p001/arch_a2/mt9p401_arch_reg_tbl.c
 *
 * History:
 *    2008/03/18 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
#undef MT9P401_FILL_ALL
static const struct mt9p001_reg_table mt9pxxx_share_regs[] = {
	{MT9P001_OUTPUT_CTRL, 0x1f82},
	{MT9P001_SHR_WIDTH_UPPER, 0x0000},
	{MT9P001_SHR_WIDTH, 0x0177},
	{MT9P001_PCLK_CTRL, 0x0000},
	{MT9P001_RESTART, 0x0000},
	{MT9P001_SHR_DELAY, 0x0000},
	{MT9P001_RESET, 0x0000},
#ifdef MT9P401_FILL_ALL
	{MT9P001_REG_0F, 0x0000},
	{MT9P001_REG_14, 0x0036},
	{MT9P001_REG_15, 0x0010},
	{MT9P001_REG_24, 0x0002},
	{MT9P001_REG_27, 0x000B},
#endif
	{MT9P001_REG_29, 0x0481},
#ifdef MT9P401_FILL_ALL
	{MT9P001_REG_2a, 0x1086},
#endif
	{MT9P001_GREEN1_GAIN, 0x0008},
	{MT9P001_BLUE_GAIN, 0x0008},
	{MT9P001_RED_GAIN, 0x0008},
	{MT9P001_GREEN2_GAIN, 0x0008},
#ifdef MT9P401_FILL_ALL
	{MT9P001_REG_30, 0x0000},
	{MT9P001_REG_32, 0x0000},
#endif
	{MT9P001_GLOBAL_GAIN, 0x0008},
#ifdef MT9P401_FILL_ALL
	{MT9P001_REG_3C, 0x1010},
	{MT9P001_REG_3D, 0x0005},
#endif
	{MT9P001_REG_3E, 0x80C7},
	{MT9P001_REG_3F, 0x0004},
#ifdef MT9P401_FILL_ALL
	{MT9P001_REG_40, 0x0007},
#endif
	{MT9P001_REG_41, 0x0003},
#ifdef MT9P401_FILL_ALL
	{MT9P001_REG_42, 0x0005},
	{MT9P001_REG_43, 0x0001},
	{MT9P001_REG_44, 0x0203},
	{MT9P001_REG_45, 0x1010},
	{MT9P001_REG_46, 0x1010},
	{MT9P001_REG_47, 0x1010},
#endif
	{MT9P001_REG_48, 0x0010},
	{MT9P001_ROW_BLACK_TARGET, 0x00a8},
#ifdef MT9P401_FILL_ALL
	{MT9P001_REG_4A, 0x0010},
#endif
	{MT9P001_ROW_BLACK_DEF_OFFSET, 0x0028},
#ifdef MT9P401_FILL_ALL
	{MT9P001_REG_4C, 0x0010},
	{MT9P001_REG_4D, 0x2020},
	{MT9P001_REG_4E, 0x1010},
	{MT9P001_REG_4F, 0x0014},
	{MT9P001_REG_50, 0x8000},
	{MT9P001_REG_51, 0x0007},
	{MT9P001_REG_52, 0x8000},
	{MT9P001_REG_53, 0x0007},
	{MT9P001_REG_54, 0x0008},
	{MT9P001_REG_56, 0x0020},
#endif
	{MT9P001_REG_57, 0x0004},
#ifdef MT9P401_FILL_ALL
	{MT9P001_REG_58, 0x8000},
	{MT9P001_REG_59, 0x0007},
	{MT9P001_REG_5A, 0x0004},
	{MT9P001_BLC_SAMPLE_SIZE, 0x0001},
	{MT9P001_BLC_TUNE1, 0x005a},
	{MT9P001_BLC_DELTA_THR, 0x2d13},
	{MT9P001_BLC_TUNE2, 0x41ff},
#endif
	{MT9P001_BLC_TARGET_THR, 0x231D},	//0x1C16
#ifdef MT9P401_FILL_ALL
	{MT9P001_GREEN1_OFFSET, 0x0020},
	{MT9P001_GREEN2_OFFSET, 0x0020},
	{MT9P001_BLACK_LEVEL_CAL, 0x0000},
	{MT9P001_RED_OFFSET, 0x0020},
	{MT9P001_BLUE_OFFSET, 0x0020},
	{MT9P001_REG_65, 0x0000},
	{MT9P001_REG_68, 0x0000},
	{MT9P001_REG_69, 0x0000},
	{MT9P001_REG_6A, 0x0000},
	{MT9P001_REG_6B, 0x0000},
	{MT9P001_REG_6C, 0x0000},
	{MT9P001_REG_6D, 0x0000},
#endif
	{MT9P001_REG_70, 0x00AC},
	{MT9P001_REG_71, 0xA700},
	{MT9P001_REG_72, 0xA700},
	{MT9P001_REG_73, 0x0C00},
	{MT9P001_REG_74, 0x0600},
	{MT9P001_REG_75, 0x5617},
	{MT9P001_REG_76, 0x6B57},
	{MT9P001_REG_77, 0x6B57},
	{MT9P001_REG_78, 0xA500},
	{MT9P001_REG_79, 0xAB00},
	{MT9P001_REG_7A, 0xA904},
	{MT9P001_REG_7B, 0xA700},
	{MT9P001_REG_7C, 0xA700},
	{MT9P001_REG_7D, 0xFF00},
	{MT9P001_REG_7E, 0xA900},
	{MT9P001_REG_7F, 0x6404},	//0xa900
//      {MT9P001_REG_80                 ,0x0022},
//      {MT9P001_CHIP_VER_ALT           ,0x1800}
};

#define MT9P001_SHARE_REG_SZIE		ARRAY_SIZE(mt9pxxx_share_regs)

/* ========================================================================== */
static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_2752 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(14),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_14,
		   .data = {
			    0x0000,	//MT9P001_HORI_BLANKING
			    0x0008,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_2592x1944 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(10),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_10,
		   .data = {
			    0x02DD,	//MT9P001_HORI_BLANKING
			    0x01a3,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(5),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_5,
		   .data = {
			    0x0800,	//MT9P001_HORI_BLANKING
			    0x039D,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_2560x1440 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(16),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_16,
		   .data = {
			    0x02ED,	//MT9P001_HORI_BLANKING
			    0x0017,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(10),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_10,
		   .data = {
			    0x0750,	//MT9P001_HORI_BLANKING
			    0x004f,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(5),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_5,
		   .data = {
			    0x0750,	//MT9P001_HORI_BLANKING
			    0x0641,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_2048x1536 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(20),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_20,
		   .data = {
			    0x0200,	//MT9P001_HORI_BLANKING
			    0x0018,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(15),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_15,
		   .data = {
			    0x0200,	//MT9P001_HORI_BLANKING
			    0x0220,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(10),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_10,
		   .data = {
			    0x0800,	//MT9P001_HORI_BLANKING
			    0x0019,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(5),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_5,
		   .data = {
			    0x0200,	//MT9P001_HORI_BLANKING
			    0x0633,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_1920x1080 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[1],
		   .fps = AMBA_VIDEO_FPS_29_97,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x01da,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS_30,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x01da,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS_25,
		   .system = AMBA_VIDEO_SYSTEM_PAL,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_25,
		   .data = {
			    0x02f9,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(20),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_20,
		   .data = {
			    0x04a7,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS(15),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_15,
		   .data = {
			    0x01da,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS(10),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_10,
		   .data = {
			    0x04a7,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[6],
		   .fps = AMBA_VIDEO_FPS(5),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_5,
		   .data = {
			    0x04a7,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_1280x720_00 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[5],
		   .fps = AMBA_VIDEO_FPS_29_97,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x01c4,	//MT9P001_HORI_BLANKING
			    0x000b,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS_60,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_60,
		   .data = {
			    0x01c4,	//MT9P001_HORI_BLANKING
			    0x000b,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[1],
		   .fps = AMBA_VIDEO_FPS_59_94,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_60,
		   .data = {
			    0x01c4,	//MT9P001_HORI_BLANKING
			    0x000b,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS_30,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x01c4,	//MT9P001_HORI_BLANKING
			    0x000b,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS_25,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_25,
		   .data = {
			    0x027a,	//MT9P001_HORI_BLANKING
			    0x0020,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_1280x720_01 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[5],
		   .fps = AMBA_VIDEO_FPS_29_97,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x01c0,	//MT9P001_HORI_BLANKING
			    0x000d,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS_30,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x01c0,	//MT9P001_HORI_BLANKING
			    0x000d,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p401_video_fps_1280x720_11 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[1],
		   .fps = AMBA_VIDEO_FPS_59_94,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .data = {
			    0x0000,	//MT9P001_HORI_BLANKING
			    0x0000,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_1280x720_11 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[1],
		   .fps = AMBA_VIDEO_FPS_29_97,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x054f,	//MT9P001_HORI_BLANKING
			    0x004f,	//MT9P001_VERT_BLANKING
			    },
		   .slow_shutter = {
				    0x0d1f,	//MT9P001_HORI_BLANKING
				    0x004f,	//MT9P001_VERT_BLANKING
				    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS_30,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x054f,	//MT9P001_HORI_BLANKING
			    0x004f,	//MT9P001_VERT_BLANKING
			    },
		   .slow_shutter = {
				    0x0d1f,	//MT9P001_HORI_BLANKING
				    0x004f,	//MT9P001_VERT_BLANKING
				    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS_25,
		   .system = AMBA_VIDEO_SYSTEM_PAL,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_25,
		   .data = {
			    0x06e2,	//MT9P001_HORI_BLANKING
			    0x004e,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(20),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_20,
		   .data = {
			    0x0970,	//MT9P001_HORI_BLANKING
			    0x0040,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS(15),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_15,
		   .data = {
			    0x054f,	//MT9P001_HORI_BLANKING
			    0x004f,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS(10),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_10,
		   .data = {
			    0x0937,	//MT9P001_HORI_BLANKING
			    0x004f,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[6],
		   .fps = AMBA_VIDEO_FPS(5),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_5,
		   .data = {
			    0x0937,	//MT9P001_HORI_BLANKING
			    0x004f,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_1280x720_11_74p5m = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[9],
		   .fps = AMBA_VIDEO_FPS_29_97,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x0384,	//MT9P001_HORI_BLANKING
			    0x0052,	//MT9P001_VERT_BLANKING
			    },
		   .slow_shutter = {
				    0x0384,	//MT9P001_HORI_BLANKING
				    0x0052,	//MT9P001_VERT_BLANKING
				    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[8],
		   .fps = AMBA_VIDEO_FPS_30,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x0384,	//MT9P001_HORI_BLANKING
			    0x0052,	//MT9P001_VERT_BLANKING
			    },
		   .slow_shutter = {
				    0x0384,	//MT9P001_HORI_BLANKING
				    0x0052,	//MT9P001_VERT_BLANKING
				    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_640x480_00 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[1],
		   .fps = AMBA_VIDEO_FPS(120),
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_120,
		   .data = {
			    0x01ea,	//MT9P001_HORI_BLANKING
			    0x000c,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_1280x960_11 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[1],
		   .fps = AMBA_VIDEO_FPS_29_97,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x03ca,	//MT9P001_HORI_BLANKING
			    0x0020,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS_30,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x03ca,	//MT9P001_HORI_BLANKING
			    0x0020,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS_25,
		   .system = AMBA_VIDEO_SYSTEM_PAL,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_25,
		   .data = {
			    0x050c,	//MT9P001_HORI_BLANKING
			    0x0020,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(20),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_20,
		   .data = {
			    0x06ef,	//MT9P001_HORI_BLANKING
			    0x0020,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS(15),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_15,
		   .data = {
			    0x03ca,	//MT9P001_HORI_BLANKING
			    0x0020,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS(10),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_10,
		   .data = {
			    0x06ef,	//MT9P001_HORI_BLANKING
			    0x0020,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_1024x768_01 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[5],
		   .fps = AMBA_VIDEO_FPS_29_97,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x01d2,	//MT9P001_HORI_BLANKING
			    0x0030,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS_30,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x01d2,	//MT9P001_HORI_BLANKING
			    0x0030,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS_25,
		   .system = AMBA_VIDEO_SYSTEM_PAL,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_25,
		   .data = {
			    0x01d2,	//MT9P001_HORI_BLANKING
			    0x00d3,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};

static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_1024x768_11 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[1],
		   .fps = AMBA_VIDEO_FPS_29_97,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x0582,	//MT9P001_HORI_BLANKING
			    0x003f,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS_30,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x0582,	//MT9P001_HORI_BLANKING
			    0x003f,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS_25,
		   .system = AMBA_VIDEO_SYSTEM_PAL,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_25,
		   .data = {
			    0x0702,	//MT9P001_HORI_BLANKING
			    0x003f,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};
//qiao_ls
static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_1280x1024 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[1],
		   .fps = AMBA_VIDEO_FPS_29_97,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x01da,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    },
		   .slow_shutter = {
				    0x0776,	//MT9P001_HORI_BLANKING
				    0x0022,	//MT9P001_VERT_BLANKING
				    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS_30,
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_30,
		   .data = {
			    0x01da,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS_25,
		   .system = AMBA_VIDEO_SYSTEM_PAL,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_25,
		   .data = {
			    0x02f9,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[0],
		   .fps = AMBA_VIDEO_FPS(20),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_20,
		   .data = {
			    0x04a7,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS(15),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_15,
		   .data = {
			    0x01da,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[4],
		   .fps = AMBA_VIDEO_FPS(10),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_10,
		   .data = {
			    0x04a7,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[6],
		   .fps = AMBA_VIDEO_FPS(5),
		   .system = AMBA_VIDEO_SYSTEM_AUTO,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_5,
		   .data = {
			    0x04a7,	//MT9P001_HORI_BLANKING
			    0x0022,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};
static const struct mt9p001_video_fps_reg_table mt9p001_video_fps_320x240_11 = {
	.reg = {
		MT9P001_HORI_BLANKING,
		MT9P001_VERT_BLANKING,
		},
	.table = {
		  {
		   .pll_reg_table = &mt9p001_pll_tbl[1],
		   .fps = AMBA_VIDEO_FPS(120),
		   .system = AMBA_VIDEO_SYSTEM_NTSC,
		   .eshutter_limit = AMBA_VIDEO_ESHUTTER_LIMIT_FPS_120,
		   .data = {
			      0x0402,	//MT9P001_HORI_BLANKING
			      0x0060,	//MT9P001_VERT_BLANKING
			    }},
		  {		//End
		   .pll_reg_table = NULL,
		   },
		  },
};


