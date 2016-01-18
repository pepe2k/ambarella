/*
 * Filename : mt9p006_reg_tbl.c
 *
 * History:
 *    2011/05/23 - [Haowei Lo] Create
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

	/*   < rember to update MT9P006_VIDEO_FORMAT_REG_TABLE_SIZE, once you add or remove table items */
	/*   < rember to update MT9P006_VIDEO_FORMAT_REG_NUM, once you add or remove register here*/
static const struct mt9p006_video_format_reg_table mt9p006_video_format_tbl = {
	.reg = {
		    0x01,     /*  0: MT9P401_ROW_START        */
		    0x02,     /*  1: MT9P401_COL_START        */
		    0x03,     /*  2: MT9P401_ROW_SIZE         */
		    0x04,     /*  3: MT9P401_COL_SIZE         */
		    0x05,     /*  4: MT9P401_HORI_BLANKING    */
		    0x06,     /*  5: MT9P401_VERT_BLANKING    */
		    0x07,     /*  6: MT9P401_OUTPUT_CTRL      */
		    0x08,     /*  7: MT9P401_SHR_WIDTH_UPPER  */
		    0x09,     /*  8: MT9P401_SHR_WIDTH        */
		    0x1E,     /*  9: MT9P401_READ_MODE_1      */
		    0x20,     /* 10: MT9P401_READ_MODE_2      */
		    0x22,     /* 11: MT9P401_ROW_ADDR_MODE    */
		    0x23,     /* 12: MT9P401_COL_ADDR_MODE    */
		    0x48,     /* 13: MT9P401_REG_48           */
		    0x70,     /* 14: MT9P401_REG_70           */
		    0x71,     /* 15: MT9P401_REG_71           */
		    0x72,     /* 16: MT9P401_REG_72           */
		    0x73,     /* 17: MT9P401_REG_73           */
		    0x74,     /* 18: MT9P401_REG_74           */
		    0x75,     /* 19: MT9P401_REG_75           */
		    0x76,     /* 20: MT9P401_REG_76           */
		    0x77,     /* 21: MT9P401_REG_77           */
		    0x78,     /* 22: MT9P401_REG_78           */
		    0x79,     /* 23: MT9P401_REG_79           */
		    0x7A,     /* 24: MT9P401_REG_7A           */
		    0x7B,     /* 25: MT9P401_REG_7B           */
		    0x7C,     /* 26: MT9P401_REG_7C           */
		    0x7D,     /* 27: MT9P401_REG_7D           */
		    0x7E,     /* 28: MT9P401_REG_7E           */
		    0x7F,     /* 29: MT9P401_REG_7F           */
		    0x80      /* 30: MT9P401_REG_80           */
		},     

	.table[0] = {		//1920x1080 @ 29.97fps
		.ext_reg_fill = NULL,
		.data = {
			        486,   /*  0: MT9P401_ROW_START        */
			        352,   /*  1: MT9P401_COL_START        */
			       1079,   /*  2: MT9P401_ROW_SIZE         */
			       1919,   /*  3: MT9P401_COL_SIZE         */
			        455,   /*  4: MT9P401_HORI_BLANKING    */
			         49,   /*  5: MT9P401_VERT_BLANKING    */
			     0x1F82,   /*  6: MT9P401_OUTPUT_CTRL      */
			          0,   /*  7: MT9P401_SHR_WIDTH_UPPER  */
			        718,   /*  8: MT9P401_SHR_WIDTH        */
			     0x4006,   /*  9: MT9P401_READ_MODE_1      */
			     0x0040,   /* 10: MT9P401_READ_MODE_2      */
			     0x0000,   /* 11: MT9P401_ROW_ADDR_MODE    */
			     0x0000,   /* 12: MT9P401_COL_ADDR_MODE    */
			     0x0018,   /* 13: MT9P401_REG_48           */
			     0x0079,   /* 14: MT9P401_REG_70           */
			     0x7800,   /* 15: MT9P401_REG_71           */
			     0x7800,   /* 16: MT9P401_REG_72           */
			     0x0300,   /* 17: MT9P401_REG_73           */
			     0x0300,   /* 18: MT9P401_REG_74           */
			     0x3C00,   /* 19: MT9P401_REG_75           */
			     0x4E3D,   /* 20: MT9P401_REG_76           */
			     0x4E3D,   /* 21: MT9P401_REG_77           */
			     0x774F,   /* 22: MT9P401_REG_78           */
			     0x7900,   /* 23: MT9P401_REG_79           */
			     0x7900,   /* 24: MT9P401_REG_7A           */
			     0x7800,   /* 25: MT9P401_REG_7B           */
			     0x7800,   /* 26: MT9P401_REG_7C           */
			     0xFF00,   /* 27: MT9P401_REG_7D           */
			     0x7800,   /* 28: MT9P401_REG_7E           */
			     0x7800,   /* 29: MT9P401_REG_7F           */
			     0x004E    /* 30: MT9P401_REG_80           */
		}, 
		.downsample = 1,
		.width	= 1920,
		.height	= 1080,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,//TODO update it if it is YUV mode
		.bits	= AMBA_VIDEO_BITS_12,//TODO
		.ratio	= AMBA_VIDEO_RATIO_16_9,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_30,
		.auto_fps = AMBA_VIDEO_FPS_29_97,
		.pll_index = 0,
	},

	.table[1] = {		//1280x720p @ 59.94fps
		.ext_reg_fill = NULL,
		.data = {
		         306,    /*  0: MT9P401_ROW_START        */
		          32,    /*  1: MT9P401_COL_START        */
		        1439,    /*  2: MT9P401_ROW_SIZE         */
		        2559,    /*  3: MT9P401_COL_SIZE         */
		         455,    /*  4: MT9P401_HORI_BLANKING    */
		           9,    /*  5: MT9P401_VERT_BLANKING    */
		      0x1F82,    /*  6: MT9P401_OUTPUT_CTRL      */
		           0,    /*  7: MT9P401_SHR_WIDTH_UPPER  */
		         718,    /*  8: MT9P401_SHR_WIDTH        */
		      0x4006,    /*  9: MT9P401_READ_MODE_1      */
		      0x0040,    /* 10: MT9P401_READ_MODE_2      */
		      0x0011,    /* 11: MT9P401_ROW_ADDR_MODE    */
		      0x0011,    /* 12: MT9P401_COL_ADDR_MODE    */
		      0x0018,    /* 13: MT9P401_REG_48           */
		      0x005C,    /* 14: MT9P401_REG_70           */
		      0x5B00,    /* 15: MT9P401_REG_71           */
		      0x5900,    /* 16: MT9P401_REG_72           */
		      0x0200,    /* 17: MT9P401_REG_73           */
		      0x0200,    /* 18: MT9P401_REG_74           */
		      0x2800,    /* 19: MT9P401_REG_75           */
		      0x3E29,    /* 20: MT9P401_REG_76           */
		      0x3E29,    /* 21: MT9P401_REG_77           */
		      0x583F,    /* 22: MT9P401_REG_78           */
		      0x5B00,    /* 23: MT9P401_REG_79           */
		      0x5A00,    /* 24: MT9P401_REG_7A           */
		      0x5900,    /* 25: MT9P401_REG_7B           */
		      0x5900,    /* 26: MT9P401_REG_7C           */
		      0xFF00,    /* 27: MT9P401_REG_7D           */
		      0x5900,    /* 28: MT9P401_REG_7E           */
		      0x5900,    /* 29: MT9P401_REG_7F           */
		      0x004E     /* 30: MT9P401_REG_80           */
		},       
		.downsample = 2,
		.width	= 1280,
		.height	= 720,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,//TODO update it if it is YUV mode
		.bits	= AMBA_VIDEO_BITS_12,//TODO
		.ratio	= AMBA_VIDEO_RATIO_16_9,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_60,
		.auto_fps = AMBA_VIDEO_FPS_59_94,
		.pll_index = 0,
	},

	.table[2] = {		//2560x1440p @ 15fps
		.ext_reg_fill = NULL,
		.data = {
		      306,     /*  0: MT9P401_ROW_START        */
		       32,     /*  1: MT9P401_COL_START        */
		     1439,     /*  2: MT9P401_ROW_SIZE         */
		     2559,     /*  3: MT9P401_COL_SIZE         */
		      471,     /*  4: MT9P401_HORI_BLANKING    */
		      271,     /*  5: MT9P401_VERT_BLANKING    */
		   0x1F82,     /*  6: MT9P401_OUTPUT_CTRL      */
			0,     /*  7: MT9P401_SHR_WIDTH_UPPER  */
		      718,     /*  8: MT9P401_SHR_WIDTH        */
		   0x0006,     /*  9: MT9P401_READ_MODE_1      */
		   0x0040,     /* 10: MT9P401_READ_MODE_2      */
		   0x0000,     /* 11: MT9P401_ROW_ADDR_MODE    */
		   0x0000,     /* 12: MT9P401_COL_ADDR_MODE    */
		   0x0010,     /* 13: MT9P401_REG_48           */
		   0x0079,     /* 14: MT9P401_REG_70           */
		   0x7800,     /* 15: MT9P401_REG_71           */
		   0x7800,     /* 16: MT9P401_REG_72           */
		   0x0300,     /* 17: MT9P401_REG_73           */
		   0x0300,     /* 18: MT9P401_REG_74           */
		   0x3C00,     /* 19: MT9P401_REG_75           */
		   0x4E3D,     /* 20: MT9P401_REG_76           */
		   0x4E3D,     /* 21: MT9P401_REG_77           */
		   0x774F,     /* 22: MT9P401_REG_78           */
		   0x7900,     /* 23: MT9P401_REG_79           */
		   0x7900,     /* 24: MT9P401_REG_7A           */
		   0x7800,     /* 25: MT9P401_REG_7B           */
		   0x7800,     /* 26: MT9P401_REG_7C           */
		   0xFF00,     /* 27: MT9P401_REG_7D           */
		   0x7800,     /* 28: MT9P401_REG_7E           */
		   0x7800,     /* 29: MT9P401_REG_7F           */
		   0x004E      /* 30: MT9P401_REG_80           */
		},
		.downsample = 1,
		.width	= 2560,
		.height	= 1440,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,//TODO update it if it is YUV mode
		.bits	= AMBA_VIDEO_BITS_12,//TODO
		.ratio	= AMBA_VIDEO_RATIO_16_9,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS(16),
		.auto_fps = AMBA_VIDEO_FPS_15,
		.pll_index = 0,
	},

	.table[3] = {		//2592x1944 @ 12fps
		.ext_reg_fill = NULL,
		.data = {
		          54,       /*  0: MT9P401_ROW_START        */
		          16,       /*  1: MT9P401_COL_START        */
		        1943,       /*  2: MT9P401_ROW_SIZE         */
		        2591,       /*  3: MT9P401_COL_SIZE         */
		         455,       /*  4: MT9P401_HORI_BLANKING    */
		         335,       /*  5: MT9P401_VERT_BLANKING    */
		      0x1F82,       /*  6: MT9P401_OUTPUT_CTRL      */
		           0,       /*  7: MT9P401_SHR_WIDTH_UPPER  */
		         718,       /*  8: MT9P401_SHR_WIDTH        */
		      0x0006,       /*  9: MT9P401_READ_MODE_1      */
		      0x0040,       /* 10: MT9P401_READ_MODE_2      */
		      0x0000,       /* 11: MT9P401_ROW_ADDR_MODE    */
		      0x0000,       /* 12: MT9P401_COL_ADDR_MODE    */
		      0x0010,       /* 13: MT9P401_REG_48           */
		      0x0079,       /* 14: MT9P401_REG_70           */
		      0x7800,       /* 15: MT9P401_REG_71           */
		      0x7800,       /* 16: MT9P401_REG_72           */
		      0x0300,       /* 17: MT9P401_REG_73           */
		      0x0300,       /* 18: MT9P401_REG_74           */
		      0x3C00,       /* 19: MT9P401_REG_75           */
		      0x4E3D,       /* 20: MT9P401_REG_76           */
		      0x4E3D,       /* 21: MT9P401_REG_77           */
		      0x774F,       /* 22: MT9P401_REG_78           */
		      0x7900,       /* 23: MT9P401_REG_79           */
		      0x7900,       /* 24: MT9P401_REG_7A           */
		      0x7800,       /* 25: MT9P401_REG_7B           */
		      0x7800,       /* 26: MT9P401_REG_7C           */
		      0xFF00,       /* 27: MT9P401_REG_7D           */
		      0x7800,       /* 28: MT9P401_REG_7E           */
		      0x7800,       /* 29: MT9P401_REG_7F           */
		      0x004E        /* 30: MT9P401_REG_80           */
		},
		.downsample = 1,
		.width	= 2592,
		.height	= 1944,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,//TODO update it if it is YUV mode
		.bits	= AMBA_VIDEO_BITS_12,//TODO
		.ratio	= AMBA_VIDEO_RATIO_4_3,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_12,
		.auto_fps = AMBA_VIDEO_FPS_12,
		.pll_index = 0,
	},

	.table[4] = {		//2048*1536 @ 15fps
		.ext_reg_fill = NULL,
		.data = {
			      306,     /*  0: MT9P401_ROW_START        */
			       32,     /*  1: MT9P401_COL_START        */
			     1535,     /*  2: MT9P401_ROW_SIZE         */
			     2047,     /*  3: MT9P401_COL_SIZE         */
			      471,     /*  4: MT9P401_HORI_BLANKING    */
			      271,     /*  5: MT9P401_VERT_BLANKING    */
			   0x1F82,     /*  6: MT9P401_OUTPUT_CTRL      */
				0,     /*  7: MT9P401_SHR_WIDTH_UPPER  */
			      718,     /*  8: MT9P401_SHR_WIDTH        */
			   0x0006,     /*  9: MT9P401_READ_MODE_1      */
			   0x0040,     /* 10: MT9P401_READ_MODE_2      */
			   0x0000,     /* 11: MT9P401_ROW_ADDR_MODE    */
			   0x0000,     /* 12: MT9P401_COL_ADDR_MODE    */
			   0x0010,     /* 13: MT9P401_REG_48           */
			   0x0079,     /* 14: MT9P401_REG_70           */
			   0x7800,     /* 15: MT9P401_REG_71           */
			   0x7800,     /* 16: MT9P401_REG_72           */
			   0x0300,     /* 17: MT9P401_REG_73           */
			   0x0300,     /* 18: MT9P401_REG_74           */
			   0x3C00,     /* 19: MT9P401_REG_75           */
			   0x4E3D,     /* 20: MT9P401_REG_76           */
			   0x4E3D,     /* 21: MT9P401_REG_77           */
			   0x774F,     /* 22: MT9P401_REG_78           */
			   0x7900,     /* 23: MT9P401_REG_79           */
			   0x7900,     /* 24: MT9P401_REG_7A           */
			   0x7800,     /* 25: MT9P401_REG_7B           */
			   0x7800,     /* 26: MT9P401_REG_7C           */
			   0xFF00,     /* 27: MT9P401_REG_7D           */
			   0x7800,     /* 28: MT9P401_REG_7E           */
			   0x7800,     /* 29: MT9P401_REG_7F           */
			   0x004E      /* 30: MT9P401_REG_80           */
		},
		.downsample = 1,
		.width	= 2048,
		.height	= 1536,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,//TODO update it if it is YUV mode
		.bits	= AMBA_VIDEO_BITS_12,//TODO
		.ratio	= AMBA_VIDEO_RATIO_4_3,
		.srm	= 0,
		.sw_limit	= -1,
		.max_fps = AMBA_VIDEO_FPS_20,
		.auto_fps = AMBA_VIDEO_FPS_15,
		.pll_index = 0,
	},
	/* add video format table here, if necessary */
};


/** MT9P006 global gain table row size */
#define MT9P006_GAIN_ROWS		(244)
#define MT9P006_GAIN_COLS		(2)
#define MT9P006_GAIN_DOUBLE		(32)
#define MT9P006_GAIN_0DB		(224)

#define MT9P006_GAIN_COL_AGC		(0)
#define MT9P006_GAIN_COL_REG		(1)


/* This is 32-step gain table, MT9P006_GAIN_ROWS = 81, MT9P006_GAIN_COLS = 3 */
const s16 MT9P006_GAIN_TABLE[MT9P006_GAIN_ROWS][MT9P006_GAIN_COLS] =
{
	/* gain_value*256,  log2(gain)*1024,  register */
	{0x8000, 0x783f},       /* index 000, gain = 42.007411 dB */ 
	{0x7d42, 0x773f},       /* index 001, gain = 41.939286 dB */ 
	{0x7a93, 0x773e},       /* index 002, gain = 41.800309 dB */ 
	{0x77f2, 0x773c},       /* index 003, gain = 41.515500 dB */ 
	{0x7560, 0x703f},       /* index 004, gain = 41.446836 dB */ 
	{0x72dd, 0x703d},       /* index 005, gain = 41.166622 dB */ 
	{0x7066, 0x703c},       /* index 006, gain = 41.023050 dB */ 
	{0x6dfe, 0x703b},       /* index 007, gain = 40.877066 dB */ 
	{0x6ba2, 0x7039},       /* index 008, gain = 40.577523 dB */ 
	{0x6954, 0x7038},       /* index 009, gain = 40.423786 dB */ 
	{0x6712, 0x603f},       /* index 010, gain = 40.203878 dB */ 
	{0x64dd, 0x603e},       /* index 011, gain = 40.064901 dB */ 
	{0x62b4, 0x603d},       /* index 012, gain = 39.923664 dB */ 
	{0x6096, 0x603b},       /* index 013, gain = 39.634108 dB */ 
	{0x5e84, 0x603a},       /* index 014, gain = 39.485627 dB */ 
	{0x5c7e, 0x6039},       /* index 015, gain = 39.334564 dB */ 
	{0x5a82, 0x6038},       /* index 016, gain = 39.180828 dB */ 
	{0x5892, 0x6037},       /* index 017, gain = 39.024321 dB */ 
	{0x56ac, 0x503f},       /* index 018, gain = 38.752865 dB */ 
	{0x54d1, 0x503e},       /* index 019, gain = 38.613888 dB */ 
	{0x52ff, 0x503c},       /* index 020, gain = 38.329079 dB */ 
	{0x5138, 0x503b},       /* index 021, gain = 38.183094 dB */ 
	{0x4f7b, 0x503a},       /* index 022, gain = 38.034614 dB */ 
	{0x4dc7, 0x5039},       /* index 023, gain = 37.883551 dB */ 
	{0x4c1c, 0x5037},       /* index 024, gain = 37.573308 dB */ 
	{0x4a7a, 0x443f},       /* index 025, gain = 37.479483 dB */ 
	{0x48e2, 0x443d},       /* index 026, gain = 37.199269 dB */ 
	{0x4752, 0x443c},       /* index 027, gain = 37.055697 dB */ 
	{0x45cb, 0x443b},       /* index 028, gain = 36.909713 dB */ 
	{0x444c, 0x443a},       /* index 029, gain = 36.761232 dB */ 
	{0x42d5, 0x3c3f},       /* index 030, gain = 36.513390 dB */ 
	{0x4167, 0x3c3e},       /* index 031, gain = 36.374413 dB */ 
	{0x4000, 0x3c3c},       /* index 032, gain = 36.089604 dB */ 
	{0x3ea1, 0x3c3b},       /* index 033, gain = 35.943619 dB */ 
	{0x3d49, 0x3c3a},       /* index 034, gain = 35.795139 dB */ 
	{0x3bf9, 0x3c38},       /* index 035, gain = 35.490339 dB */ 
	{0x3ab0, 0x3c37},       /* index 036, gain = 35.333833 dB */ 
	{0x396e, 0x3c36},       /* index 037, gain = 35.174454 dB */ 
	{0x3833, 0x3c35},       /* index 038, gain = 35.012096 dB */ 
	{0x36ff, 0x3c34},       /* index 039, gain = 34.846646 dB */ 
	{0x35d1, 0x3c33},       /* index 040, gain = 34.677982 dB */ 
	{0x34aa, 0x2d3f},       /* index 041, gain = 34.348729 dB */ 
	{0x3389, 0x2d3e},       /* index 042, gain = 34.209752 dB */ 
	{0x326e, 0x2d3d},       /* index 043, gain = 34.068515 dB */ 
	{0x315a, 0x2d3c},       /* index 044, gain = 33.924943 dB */ 
	{0x304b, 0x2d3a},       /* index 045, gain = 33.630478 dB */ 
	{0x2f42, 0x2d39},       /* index 046, gain = 33.479415 dB */ 
	{0x2e3f, 0x2d38},       /* index 047, gain = 33.325678 dB */ 
	{0x2d41, 0x2d37},       /* index 048, gain = 33.169172 dB */ 
	{0x2c49, 0x2d35},       /* index 049, gain = 32.847435 dB */ 
	{0x2b56, 0x243f},       /* index 050, gain = 32.732265 dB */ 
	{0x2a68, 0x243e},       /* index 051, gain = 32.593288 dB */ 
	{0x2980, 0x243c},       /* index 052, gain = 32.308479 dB */ 
	{0x289c, 0x243b},       /* index 053, gain = 32.162494 dB */ 
	{0x27bd, 0x243a},       /* index 054, gain = 32.014014 dB */ 
	{0x26e3, 0x2439},       /* index 055, gain = 31.862951 dB */ 
	{0x260e, 0x2437},       /* index 056, gain = 31.552708 dB */ 
	{0x253d, 0x2436},       /* index 057, gain = 31.393329 dB */ 
	{0x2471, 0x1d3f},       /* index 058, gain = 31.227246 dB */ 
	{0x23a9, 0x1d3e},       /* index 059, gain = 31.088269 dB */ 
	{0x22e5, 0x1d3c},       /* index 060, gain = 30.803460 dB */ 
	{0x2226, 0x1d3b},       /* index 061, gain = 30.657475 dB */ 
	{0x216b, 0x1d3a},       /* index 062, gain = 30.508995 dB */ 
	{0x20b3, 0x1d39},       /* index 063, gain = 30.357932 dB */ 
	{0x2000, 0x1d37},       /* index 064, gain = 30.047689 dB */    
	{0x1f50, 0x183f},       /* index 065, gain = 29.966211 dB */ 
	{0x1ea5, 0x183d},       /* index 066, gain = 29.685997 dB */ 
	{0x1dfd, 0x183c},       /* index 067, gain = 29.542425 dB */ 
	{0x1d58, 0x183b},       /* index 068, gain = 29.396440 dB */ 
	{0x1cb7, 0x1839},       /* index 069, gain = 29.096897 dB */ 
	{0x1c1a, 0x1838},       /* index 070, gain = 28.943161 dB */ 
	{0x1b7f, 0x1837},       /* index 071, gain = 28.786654 dB */ 
	{0x1ae9, 0x133f},       /* index 072, gain = 28.490487 dB */ 
	{0x1a55, 0x133e},       /* index 073, gain = 28.351510 dB */ 
	{0x19c5, 0x133d},       /* index 074, gain = 28.210273 dB */ 
	{0x1937, 0x133c},       /* index 075, gain = 28.066701 dB */ 
	{0x18ad, 0x133a},       /* index 076, gain = 27.772236 dB */ 
	{0x1826, 0x1339},       /* index 077, gain = 27.621173 dB */   
	{0x17a1, 0x103f},       /* index 078, gain = 27.467436 dB */ 
	{0x171f, 0x103e},       /* index 079, gain = 27.328459 dB */ 
	{0x16a1, 0x103c},       /* index 080, gain = 27.043650 dB */ 
	{0x1624, 0x103b},       /* index 081, gain = 26.897666 dB */ 
	{0x15ab, 0x103a},       /* index 082, gain = 26.749185 dB */ 
	{0x1534, 0x1039},       /* index 083, gain = 26.598122 dB */   
	{0x14c0, 0x0d3f},       /* index 084, gain = 26.307597 dB */ 
	{0x144e, 0x0d3e},       /* index 085, gain = 26.168620 dB */ 
	{0x13df, 0x0d3d},       /* index 086, gain = 26.027383 dB */ 
	{0x1372, 0x0d3b},       /* index 087, gain = 25.737827 dB */ 
	{0x1307, 0x0d3a},       /* index 088, gain = 25.589346 dB */ 
	{0x129f, 0x0d39},       /* index 089, gain = 25.438284 dB */ 
	{0x1238, 0x0d38},       /* index 090, gain = 25.284547 dB */ 
	{0x11d5, 0x0a3f},       /* index 091, gain = 24.968662 dB */ 
	{0x1173, 0x0a3e},       /* index 092, gain = 24.829684 dB */ 
	{0x1113, 0x0a3d},       /* index 093, gain = 24.688447 dB */ 
	{0x10b5, 0x0a3b},       /* index 094, gain = 24.398891 dB */ 
	{0x105a, 0x0a3a},       /* index 095, gain = 24.250410 dB */ 
	{0x1000, 0x0a39},       /* index 096, gain = 24.099348 dB */ 
	{0x0fa8, 0x083f},       /* index 097, gain = 23.945611 dB */ 
	{0x0f52, 0x083d},       /* index 098, gain = 23.665397 dB */ 
	{0x0efe, 0x083c},       /* index 099, gain = 23.521825 dB */ 
	{0x0eac, 0x083b},       /* index 100, gain = 23.375840 dB */ 
	{0x0e5c, 0x0839},       /* index 101, gain = 23.076297 dB */ 
	{0x0e0d, 0x0838},       /* index 102, gain = 22.922561 dB */ 
	{0x0dc0, 0x063f},       /* index 103, gain = 22.785772 dB */ 
	{0x0d74, 0x063e},       /* index 104, gain = 22.646795 dB */ 
	{0x0d2b, 0x063c},       /* index 105, gain = 22.361986 dB */ 
	{0x0ce2, 0x063b},       /* index 106, gain = 22.216001 dB */ 
	{0x0c9c, 0x063a},       /* index 107, gain = 22.067521 dB */ 
	{0x0c56, 0x0638},       /* index 108, gain = 21.762722 dB */ 
	{0x0c13, 0x0637},       /* index 109, gain = 21.606215 dB */ 
	{0x0bd1, 0x043f},       /* index 110, gain = 21.446836 dB */ 
	{0x0b90, 0x043e},       /* index 111, gain = 21.307859 dB */ 
	{0x0b50, 0x043c},       /* index 112, gain = 21.023050 dB */ 
	{0x0b12, 0x043b},       /* index 113, gain = 20.877066 dB */ 
	{0x0ad6, 0x043a},       /* index 114, gain = 20.728585 dB */ 
	{0x0a9a, 0x0439},       /* index 115, gain = 20.577523 dB */ 
	{0x0a60, 0x0437},       /* index 116, gain = 20.267279 dB */ 
	{0x0a27, 0x0436},       /* index 117, gain = 20.107901 dB */ 
	{0x09ef, 0x023f},       /* index 118, gain = 19.863212 dB */ 
	{0x09b9, 0x023e},       /* index 119, gain = 19.724234 dB */ 
	{0x0983, 0x023d},       /* index 120, gain = 19.582997 dB */ 
	{0x094f, 0x023c},       /* index 121, gain = 19.439426 dB */ 
	{0x091c, 0x023a},       /* index 122, gain = 19.144960 dB */ 
	{0x08ea, 0x013f},       /* index 123, gain = 18.948062 dB */ 
	{0x08b9, 0x013e},       /* index 124, gain = 18.809084 dB */ 
	{0x088a, 0x013d},       /* index 125, gain = 18.667847 dB */ 
	{0x085b, 0x013b},       /* index 126, gain = 18.378291 dB */ 
	{0x082d, 0x013a},       /* index 127, gain = 18.229811 dB */ 
	{0x0800, 0x0139},       /* index 128, gain = 18.078748 dB */ 
	{0x07d4, 0x003f},       /* index 129, gain = 17.925011 dB */ 
	{0x07a9, 0x003d},       /* index 130, gain = 17.644797 dB */ 
	{0x077f, 0x003c},       /* index 131, gain = 17.501225 dB */ 
	{0x0756, 0x003b},       /* index 132, gain = 17.355240 dB */ 
	{0x072e, 0x0039},       /* index 133, gain = 17.055697 dB */ 
	{0x0706, 0x0038},       /* index 134, gain = 16.901961 dB */ 
	{0x06e0, 0x0037},       /* index 135, gain = 16.745454 dB */ 
	{0x06ba, 0x0036},       /* index 136, gain = 16.586075 dB */ 
	{0x0695, 0x0035},       /* index 137, gain = 16.423718 dB */ 
	{0x0671, 0x0034},       /* index 138, gain = 16.258267 dB */ 
	{0x064e, 0x0032},       /* index 139, gain = 15.917600 dB */ 
	{0x062b, 0x0031},       /* index 140, gain = 15.742122 dB */ 
	{0x0609, 0x0030},       /* index 141, gain = 15.563025 dB */ 
	{0x05e8, 0x002f},       /* index 142, gain = 15.380157 dB */ 
	{0x05c8, 0x002e},       /* index 143, gain = 15.193357 dB */ 
	{0x05a8, 0x002d},       /* index 144, gain = 15.002451 dB */ 
	{0x0589, 0x002c},       /* index 145, gain = 14.807254 dB */ 
	{0x056b, 0x002b},       /* index 146, gain = 14.607569 dB */ 
	{0x054d, 0x002a},       /* index 147, gain = 14.403186 dB */ 
	{0x0530, 0x002a},       /* index 148, gain = 14.403186 dB */ 
	{0x0514, 0x0029},       /* index 149, gain = 14.193877 dB */ 
	{0x04f8, 0x0028},       /* index 150, gain = 13.979400 dB */ 
	{0x04dc, 0x0027},       /* index 151, gain = 13.759492 dB */ 
	{0x04c2, 0x0026},       /* index 152, gain = 13.533872 dB */ 
	{0x04a8, 0x0025},       /* index 153, gain = 13.302235 dB */ 
	{0x048e, 0x0024},       /* index 154, gain = 13.064250 dB */ 
	{0x0475, 0x0024},       /* index 155, gain = 13.064250 dB */ 
	{0x045d, 0x0023},       /* index 156, gain = 12.819561 dB */ 
	{0x0445, 0x0022},       /* index 157, gain = 12.567779 dB */ 
	{0x042d, 0x0021},       /* index 158, gain = 12.308479 dB */ 
	{0x0416, 0x0021},       /* index 159, gain = 12.308479 dB */ 
	{0x0400, 0x0020},       /* index 160, gain = 12.041200 dB */ 
	{0x03ea, 0x001f},       /* index 161, gain = 11.765434 dB */ 
	{0x03d5, 0x001f},       /* index 162, gain = 11.765434 dB */ 
	{0x03c0, 0x001e},       /* index 163, gain = 11.480625 dB */ 
	{0x03ab, 0x001d},       /* index 164, gain = 11.186160 dB */ 
	{0x0397, 0x001d},       /* index 165, gain = 11.186160 dB */ 
	{0x0383, 0x001c},       /* index 166, gain = 10.881361 dB */ 
	{0x0370, 0x001c},       /* index 167, gain = 10.881361 dB */ 
	{0x035d, 0x001b},       /* index 168, gain = 10.565476 dB */ 
	{0x034b, 0x001a},       /* index 169, gain = 10.237667 dB */ 
	{0x0339, 0x001a},       /* index 170, gain = 10.237667 dB */ 
	{0x0327, 0x0019},       /* index 171, gain = 9.897000 dB */ 
	{0x0316, 0x0019},       /* index 172, gain = 9.897000 dB */ 
	{0x0305, 0x0018},       /* index 173, gain = 9.542425 dB */ 
	{0x02f4, 0x0018},       /* index 174, gain = 9.542425 dB */ 
	{0x02e4, 0x0017},       /* index 175, gain = 9.172757 dB */ 
	{0x02d4, 0x0017},       /* index 176, gain = 9.172757 dB */ 
	{0x02c5, 0x0016},       /* index 177, gain = 8.786654 dB */ 
	{0x02b5, 0x0016},       /* index 178, gain = 8.786654 dB */ 
	{0x02a7, 0x0015},       /* index 179, gain = 8.382586 dB */ 
	{0x0298, 0x0015},       /* index 180, gain = 8.382586 dB */ 
	{0x028a, 0x0014},       /* index 181, gain = 7.958800 dB */ 
	{0x027c, 0x0014},       /* index 182, gain = 7.958800 dB */ 
	{0x026e, 0x0013},       /* index 183, gain = 7.513272 dB */ 
	{0x0261, 0x0013},       /* index 184, gain = 7.513272 dB */ 
	{0x0254, 0x0013},       /* index 185, gain = 7.513272 dB */ 
	{0x0247, 0x0012},       /* index 186, gain = 7.043650 dB */ 
	{0x023b, 0x0012},       /* index 187, gain = 7.043650 dB */ 
	{0x022e, 0x0011},       /* index 188, gain = 6.547179 dB */ 
	{0x0222, 0x0011},       /* index 189, gain = 6.547179 dB */ 
	{0x0217, 0x0011},       /* index 190, gain = 6.547179 dB */ 
	{0x020b, 0x0010},       /* index 191, gain = 6.020600 dB */ 
	{0x0200, 0x0010},       /* index 192, gain = 6.020600 dB */ 
	{0x01f5, 0x0010},       /* index 193, gain = 6.020600 dB */ 
	{0x01ea, 0x000f},       /* index 194, gain = 5.460025 dB */ 
	{0x01e0, 0x000f},       /* index 195, gain = 5.460025 dB */ 
	{0x01d6, 0x000f},       /* index 196, gain = 5.460025 dB */ 
	{0x01cb, 0x000e},       /* index 197, gain = 4.860761 dB */ 
	{0x01c2, 0x000e},       /* index 198, gain = 4.860761 dB */ 
	{0x01b8, 0x000e},       /* index 199, gain = 4.860761 dB */ 
	{0x01af, 0x000d},       /* index 200, gain = 4.217067 dB */ 
	{0x01a5, 0x000d},       /* index 201, gain = 4.217067 dB */ 
	{0x019c, 0x000d},       /* index 202, gain = 4.217067 dB */ 
	{0x0193, 0x000d},       /* index 203, gain = 4.217067 dB */ 
	{0x018b, 0x000c},       /* index 204, gain = 3.521825 dB */ 
	{0x0182, 0x000c},       /* index 205, gain = 3.521825 dB */ 
	{0x017a, 0x000c},       /* index 206, gain = 3.521825 dB */ 
	{0x0172, 0x000c},       /* index 207, gain = 3.521825 dB */ 
	{0x016a, 0x000b},       /* index 208, gain = 2.766054 dB */ 
	{0x0162, 0x000b},       /* index 209, gain = 2.766054 dB */ 
	{0x015b, 0x000b},       /* index 210, gain = 2.766054 dB */ 
	{0x0153, 0x000b},       /* index 211, gain = 2.766054 dB */ 
	{0x014c, 0x000a},       /* index 212, gain = 1.938200 dB */ 
	{0x0145, 0x000a},       /* index 213, gain = 1.938200 dB */ 
	{0x013e, 0x000a},       /* index 214, gain = 1.938200 dB */ 
	{0x0137, 0x000a},       /* index 215, gain = 1.938200 dB */ 
	{0x0130, 0x000a},       /* index 216, gain = 1.938200 dB */ 
	{0x012a, 0x0009},       /* index 217, gain = 1.023050 dB */ 
	{0x0124, 0x0009},       /* index 218, gain = 1.023050 dB */ 
	{0x011d, 0x0009},       /* index 219, gain = 1.023050 dB */ 
	{0x0117, 0x0009},       /* index 220, gain = 1.023050 dB */ 
	{0x0111, 0x0009},       /* index 221, gain = 1.023050 dB */ 
	{0x010b, 0x0008},       /* index 222, gain = 0.000000 dB */ 
	{0x0106, 0x0008},       /* index 223, gain = 0.000000 dB */ 
	{0x0100, 0x0008},       /* index 224, gain = 0.000000 dB */ 
	{0x00fb, 0x0008},       /* index 225, gain = 0.000000 dB */ 
	{0x00f5, 0x0008},       /* index 226, gain = 0.000000 dB */ 
	{0x00f0, 0x0008},       /* index 227, gain = 0.000000 dB */ 
	{0x00eb, 0x0007},       /* index 228, gain = -1.159839 dB */ 
	{0x00e6, 0x0007},       /* index 229, gain = -1.159839 dB */ 
	{0x00e1, 0x0007},       /* index 230, gain = -1.159839 dB */ 
	{0x00dc, 0x0007},       /* index 231, gain = -1.159839 dB */ 
	{0x00d7, 0x0007},       /* index 232, gain = -1.159839 dB */ 
	{0x00d3, 0x0007},       /* index 233, gain = -1.159839 dB */ 
	{0x00ce, 0x0006},       /* index 234, gain = -2.498775 dB */ 
	{0x00ca, 0x0006},       /* index 235, gain = -2.498775 dB */ 
	{0x00c5, 0x0006},       /* index 236, gain = -2.498775 dB */ 
	{0x00c1, 0x0006},       /* index 237, gain = -2.498775 dB */ 
	{0x00bd, 0x0006},       /* index 238, gain = -2.498775 dB */ 
	{0x00b9, 0x0006},       /* index 239, gain = -2.498775 dB */ 
	{0x00b5, 0x0006},       /* index 240, gain = -2.498775 dB */ 
	{0x00b1, 0x0006},       /* index 241, gain = -2.498775 dB */ 
	{0x00ad, 0x0005},       /* index 242, gain = -4.082400 dB */ 
	{0x00aa, 0x0005}        /* index 243, gain = -4.082400 dB */ 
};                            

