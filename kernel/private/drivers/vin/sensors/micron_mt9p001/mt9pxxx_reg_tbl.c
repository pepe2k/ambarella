/*
 * kernel/private/drivers/ambarella/vin/sensors/micron_mt9p001/mt9p001_reg_tbl.c
 *
 * History:
 *    2008/01/18 - [Anthony Ginger] Create
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
static void mt9p001_fill_0_ext_regs(struct __amba_vin_source *src)
{
	u16 tmp_reg;

	mt9p001_read_reg(src, MT9P001_BLACK_LEVEL_CAL, &tmp_reg);
	tmp_reg |= 0x0001;
	mt9p001_write_reg(src, MT9P001_BLACK_LEVEL_CAL, tmp_reg);

	tmp_reg = 0;
	mt9p001_write_reg(src, MT9P001_ROW_BLACK_DEF_OFFSET, tmp_reg);
}

static void mt9p001_fill_1_ext_regs(struct __amba_vin_source *src)
{
	u16 tmp_reg;

	mt9p001_read_reg(src, MT9P001_BLACK_LEVEL_CAL, &tmp_reg);
	tmp_reg &= (~0x0001);
	mt9p001_write_reg(src, MT9P001_BLACK_LEVEL_CAL, tmp_reg);

	tmp_reg = 0;
	mt9p001_write_reg(src, MT9P001_ROW_BLACK_DEF_OFFSET, tmp_reg);
}

static void mt9p001_tn09111_regs(struct __amba_vin_source *src)
{
	mt9p001_write_reg(src, MT9P001_REG_29, 0x0481);
	mt9p001_write_reg(src, MT9P001_REG_3E, 0x0087);
	mt9p001_write_reg(src, MT9P001_REG_3F, 0x0007);
	mt9p001_write_reg(src, MT9P001_REG_41, 0x0003);
	mt9p001_write_reg(src, MT9P001_REG_48, 0x0018);
	mt9p001_write_reg(src, MT9P001_REG_57, 0x0007);
	mt9p001_write_reg(src, MT9P001_BLC_TARGET_THR, 0x1C16);
	mt9p001_write_reg(src, MT9P001_REG_70, 0x005C);
	mt9p001_write_reg(src, MT9P001_REG_71, 0x5B00);
	mt9p001_write_reg(src, MT9P001_REG_72, 0x5900);
	mt9p001_write_reg(src, MT9P001_REG_73, 0x0200);
	mt9p001_write_reg(src, MT9P001_REG_74, 0x0200);
	mt9p001_write_reg(src, MT9P001_REG_75, 0x2800);
	mt9p001_write_reg(src, MT9P001_REG_76, 0x3E29);
	mt9p001_write_reg(src, MT9P001_REG_77, 0x3E29);
	mt9p001_write_reg(src, MT9P001_REG_78, 0x583F);
	mt9p001_write_reg(src, MT9P001_REG_79, 0x5B00);
	mt9p001_write_reg(src, MT9P001_REG_7A, 0x5A00);
	mt9p001_write_reg(src, MT9P001_REG_7B, 0x5900);
	mt9p001_write_reg(src, MT9P001_REG_7C, 0x5900);
	mt9p001_write_reg(src, MT9P001_REG_7E, 0x5900);
	mt9p001_write_reg(src, MT9P001_REG_7F, 0x5900);
}

/* ========================================================================== */
#define MT9PXXX_BAYER		AMBA_VIN_SRC_BAYER_PATTERN_RG
#define MT9PXXX_RM2_MASK	(0x0000)

static const struct mt9p001_video_format_reg_table mt9p001_video_format_tbl = {
	.reg = {
		MT9P001_ROW_START,
		MT9P001_COL_START,
		MT9P001_ROW_SIZE,
		MT9P001_COL_SIZE,
		MT9P001_READ_MODE_1,
		MT9P001_READ_MODE_2,
		MT9P001_ROW_ADDR_MODE,
		MT9P001_COL_ADDR_MODE,
		},
	.table[0] = {		//2752x2004 Full-array Readout, Manual BLC
		     .ext_reg_fill = mt9p001_fill_0_ext_regs,
		     .data = {
			      0x0000,	//MT9P001_ROW_START
			      0x0000,	//MT9P001_COL_START
			      0x07D3,	//MT9P001_ROW_SIZE
			      0x0ABF,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x1800 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0000,	//MT9P001_ROW_ADDR_MODE
			      0x0000,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_2752,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_4_3,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
	.table[1] = {		//2752x1992 Full-array Readout, Auto BLC
		     .ext_reg_fill = mt9p001_fill_1_ext_regs,
		     .data = {
			      0x000c,	//MT9P001_ROW_START
			      0x0000,	//MT9P001_COL_START
			      0x07c7,	//MT9P001_ROW_SIZE
			      0x0ABF,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x1800 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0000,	//MT9P001_ROW_ADDR_MODE
			      0x0000,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_2752,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_4_3,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
	.table[2] = {		//2592x1944
		     .ext_reg_fill = NULL,
		     .data = {
			      0x0036,	//MT9P001_ROW_START
			      0x0010,	//MT9P001_COL_START
			      0x0797,	//MT9P001_ROW_SIZE
			      0x0a1f,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0000,	//MT9P001_ROW_ADDR_MODE
			      0x0000,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_2592x1944,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_4_3,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
	.table[3] = {		//2560x1440
		     .ext_reg_fill = NULL,
		     .data = {
			      0x0130,	//MT9P001_ROW_START
			      0x0020,	//MT9P001_COL_START
			      0x05A1,	//MT9P001_ROW_SIZE
			      0x09FF,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0000,	//MT9P001_ROW_ADDR_MODE
			      0x0000,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_2560x1440,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_16_9,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
	.table[4] = {		//1920x1080
		     .ext_reg_fill = NULL,
		     .data = {
			      0x01e6,	//MT9P001_ROW_START
			      0x0160,	//MT9P001_COL_START
			      0x0437,	//MT9P001_ROW_SIZE
			      0x077f,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0000,	//MT9P001_ROW_ADDR_MODE
			      0x0000,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_1920x1080,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_16_9,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
	.table[5] = {		//1280x720 Bin=0, Skip=1
		     .ext_reg_fill = NULL,
		     .data = {
			      0x0132,	//MT9P001_ROW_START
			      0x0020,	//MT9P001_COL_START
			      0x059f,	//MT9P001_ROW_SIZE
			      0x09ff,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0001,	//MT9P001_ROW_ADDR_MODE
			      0x0001,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_1280x720_01,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_16_9,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
	.table[6] = {		//1280x720 Bin=1, Skip=1
		     .ext_reg_fill = NULL,
		     .data = {
			      0x0132,	//MT9P001_ROW_START
			      0x0020,	//MT9P001_COL_START
			      0x059f,	//MT9P001_ROW_SIZE
			      0x09ff,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0011,	//MT9P001_ROW_ADDR_MODE
			      0x0011,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_1280x720_11,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_16_9,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
	.table[7] = {		//640x480 Bin=0, Skip=3
		     .ext_reg_fill = NULL,
		     .data = {
			      0x0042,	//MT9P001_ROW_START
			      0x0020,	//MT9P001_COL_START
			      0x077f,	//MT9P001_ROW_SIZE
			      0x09ff,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0003,	//MT9P001_ROW_ADDR_MODE
			      0x0003,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_640x480_00,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_4_3,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
	.table[8] = {		//1280x960 Bin=1, Skip=1
		     .ext_reg_fill = NULL,
		     .data = {
			      0x0042,	//MT9P001_ROW_START
			      0x0020,	//MT9P001_COL_START
			      0x077f,	//MT9P001_ROW_SIZE
			      0x09ff,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0011,	//MT9P001_ROW_ADDR_MODE
			      0x0011,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_1280x960_11,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_4_3,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
	.table[9] = {		//1280x960 Bin=1, Skip=1
		     .ext_reg_fill = NULL,
		     .data = {
			      0x0042,	//MT9P001_ROW_START
			      0x0020,	//MT9P001_COL_START
			      0x077f,	//MT9P001_ROW_SIZE
			      0x09ff,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0011,	//MT9P001_ROW_ADDR_MODE
			      0x0011,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_1280x960_11,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_4_3,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
	.table[10] = {		//1280x720 Bin=1, Skip=1, P60
		      .ext_reg_fill = mt9p001_tn09111_regs,
		      .data = {
			       0x0132,	//MT9P001_ROW_START
			       0x0020,	//MT9P001_COL_START
			       0x059f,	//MT9P001_ROW_SIZE
			       0x09ff,	//MT9P001_COL_SIZE
			       0x0406,	//MT9P001_READ_MODE_1
			       (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			       0x0011,	//MT9P001_ROW_ADDR_MODE
			       0x0011,	//MT9P001_COL_ADDR_MODE
			       },
#if defined(CONFIG_SENSOR_MT9P401)
		      .fps_table = &mt9p401_video_fps_1280x720_11,
#else
		      .fps_table = &mt9p001_video_fps_1280x720_11,
#endif
		      .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		      .type = AMBA_VIDEO_TYPE_RGB_RAW,
		      .bits = AMBA_VIDEO_BITS_12,
		      .ratio = AMBA_VIDEO_RATIO_16_9,
		      .srm = 0,
		      .bayer_pattern = MT9PXXX_BAYER,
		      },
	.table[11] = {		//2048x1536 Bin=0, Skip=0
		      .ext_reg_fill = NULL,
		      .data = {
			       0x0102,	//MT9P001_ROW_START
			       0x0120,	//MT9P001_COL_START
			       0x05ff,	//MT9P001_ROW_SIZE
			       0x07ff,	//MT9P001_COL_SIZE
			       0x0406,	//MT9P001_READ_MODE_1
			       (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			       0x0000,	//MT9P001_ROW_ADDR_MODE
			       0x0000,	//MT9P001_COL_ADDR_MODE
			       },
		      .fps_table = &mt9p001_video_fps_2048x1536,
		      .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		      .type = AMBA_VIDEO_TYPE_RGB_RAW,
		      .bits = AMBA_VIDEO_BITS_12,
		      .ratio = AMBA_VIDEO_RATIO_4_3,
		      .srm = 0,
		      .bayer_pattern = MT9PXXX_BAYER,
		      },
	.table[12] = {		//640x480 Bin=0, Skip=0
		      .ext_reg_fill = NULL,
		      .data = {
			       0x0312,	//MT9P001_ROW_START
			       0x03E0,	//MT9P001_COL_START
			       0x01df,	//MT9P001_ROW_SIZE
			       0x027f,	//MT9P001_COL_SIZE
			       0x0406,	//MT9P001_READ_MODE_1
			       (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			       0x0000,	//MT9P001_ROW_ADDR_MODE
			       0x0000,	//MT9P001_COL_ADDR_MODE
			       },
		      .fps_table = &mt9p001_video_fps_640x480_00,
		      .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		      .type = AMBA_VIDEO_TYPE_RGB_RAW,
		      .bits = AMBA_VIDEO_BITS_12,
		      .ratio = AMBA_VIDEO_RATIO_4_3,
		      .srm = 0,
		      .bayer_pattern = MT9PXXX_BAYER,
		      },
	.table[13] = {		//1024x768 Bin=0, Skip=1
		      .ext_reg_fill = NULL,
		      .data = {
			       0x0102,	//MT9P001_ROW_START
			       0x0120,	//MT9P001_COL_START
			       0x05ff,	//MT9P001_ROW_SIZE
			       0x07ff,	//MT9P001_COL_SIZE
			       0x0406,	//MT9P001_READ_MODE_1
			       (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			       0x0001,	//MT9P001_ROW_ADDR_MODE
			       0x0001,	//MT9P001_COL_ADDR_MODE
			       },
		      .fps_table = &mt9p001_video_fps_1024x768_01,
		      .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		      .type = AMBA_VIDEO_TYPE_RGB_RAW,
		      .bits = AMBA_VIDEO_BITS_12,
		      .ratio = AMBA_VIDEO_RATIO_4_3,
		      .srm = 0,
		      .bayer_pattern = MT9PXXX_BAYER,
		      },
	.table[14] = {		//1024x768 Bin=1, Skip=1
		      .ext_reg_fill = NULL,
		      .data = {
			       0x0102,	//MT9P001_ROW_START
			       0x0120,	//MT9P001_COL_START
			       0x05ff,	//MT9P001_ROW_SIZE
			       0x07ff,	//MT9P001_COL_SIZE
			       0x0406,	//MT9P001_READ_MODE_1
			       (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			       0x0011,	//MT9P001_ROW_ADDR_MODE
			       0x0011,	//MT9P001_COL_ADDR_MODE
			       },
		      .fps_table = &mt9p001_video_fps_1024x768_11,
		      .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		      .type = AMBA_VIDEO_TYPE_RGB_RAW,
		      .bits = AMBA_VIDEO_BITS_12,
		      .ratio = AMBA_VIDEO_RATIO_4_3,
		      .srm = 0,
		      .bayer_pattern = MT9PXXX_BAYER,
		      },
	.table[15] = {		//1280x720 Bin=0, Skip=0
		      .ext_reg_fill = NULL,
		      .data = {
			       0x029A,	//MT9P001_ROW_START
			       0x02A0,	//MT9P001_COL_START
			       0x02cf,	//MT9P001_ROW_SIZE
			       0x04ff,	//MT9P001_COL_SIZE
			       0x0406,	//MT9P001_READ_MODE_1
			       (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			       0x0000,	//MT9P001_ROW_ADDR_MODE
			       0x0000,	//MT9P001_COL_ADDR_MODE
			       },
		      .fps_table = &mt9p001_video_fps_1280x720_00,
		      .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		      .type = AMBA_VIDEO_TYPE_RGB_RAW,
		      .bits = AMBA_VIDEO_BITS_12,
		      .ratio = AMBA_VIDEO_RATIO_16_9,
		      .srm = 0,
		      .bayer_pattern = MT9PXXX_BAYER,
		      },
	.table[16] = {		//1280x1024 qiao_ls
		      .ext_reg_fill = NULL,
		      .data = {
			       0x01e6,	//MT9P001_ROW_START
			       0x0160,	//MT9P001_COL_START
			       0x0437,	//MT9P001_ROW_SIZE
			       0x077f,	//MT9P001_COL_SIZE
			       0x0406,	//MT9P001_READ_MODE_1
			       (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			       0x0000,	//MT9P001_ROW_ADDR_MODE
			       0x0000,	//MT9P001_COL_ADDR_MODE
			       },
		      .fps_table = &mt9p001_video_fps_1280x1024,
		      .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		      .type = AMBA_VIDEO_TYPE_RGB_RAW,
		      .bits = AMBA_VIDEO_BITS_12,
		      .ratio = AMBA_VIDEO_RATIO_16_9,
		      .srm = 0,
		      .bayer_pattern = MT9PXXX_BAYER,
		      },
	.table[17] = {		//1280x720 Bin=1, Skip=1 @ 72M
		     .ext_reg_fill = NULL,
		     .data = {
			      0x0132,	//MT9P001_ROW_START
			      0x0020,	//MT9P001_COL_START
			      0x059f,	//MT9P001_ROW_SIZE
			      0x09ff,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0011,	//MT9P001_ROW_ADDR_MODE
			      0x0011,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_1280x720_11_74p5m,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_16_9,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
	.table[18] = {		//320x240 Bin=1, Skip=1
		     .ext_reg_fill = NULL,
		     .data = {
			      0x0312,	//MT9P001_ROW_START
			      0x0370,	//MT9P001_COL_START
			      	 479,	//MT9P001_ROW_SIZE
			      	 639,	//MT9P001_COL_SIZE
			      0x0406,	//MT9P001_READ_MODE_1
			      (0x0040 | MT9PXXX_RM2_MASK),	//MT9P001_READ_MODE_2
			      0x0001,	//MT9P001_ROW_ADDR_MODE
			      0x0001,	//MT9P001_COL_ADDR_MODE
			      },
		     .fps_table = &mt9p001_video_fps_320x240_11,
		     .format = AMBA_VIDEO_FORMAT_PROGRESSIVE,
		     .type = AMBA_VIDEO_TYPE_RGB_RAW,
		     .bits = AMBA_VIDEO_BITS_12,
		     .ratio = AMBA_VIDEO_RATIO_4_3,
		     .srm = 0,
		     .bayer_pattern = MT9PXXX_BAYER,
		     },
};

/** Agc Table */
#ifdef USE_32_STEP_GAIN_TABLE	//32-step
const s16 MT9PXXX_GLOBAL_GAIN_TABLE[MT9P001_GAIN_ROWS][MT9P001_GAIN_COLS] = {
	/* gain_value*256,  log2(gain)*1000,  register */
	/* error */
	{0x8000, 7000, 0x397f},	/* index 0   -0.002121     : 42dB */
	{0x7d41, 6968, 0x387f},	/* index 1   0.051357      */
	{0x7a92, 6937, 0x367f},	/* index 2   -0.036266     */
	{0x77f2, 6906, 0x357f},	/* index 3   0.010639      */
	{0x7560, 6875, 0x347f},	/* index 4   0.055210      */
	{0x72dc, 6843, 0x327f},	/* index 5   -0.051109     */
	{0x7066, 6812, 0x317f},	/* index 6   -0.014027     */
	{0x6dfd, 6781, 0x307f},	/* index 7   0.020378      */
	{0x6ba2, 6750, 0x2f7f},	/* index 8   0.052017      */
	{0x6954, 6718, 0x2e7f},	/* index 9   0.080780      */
	{0x6712, 6687, 0x2c7f},	/* index 10  -0.058884     */
	{0x64dc, 6656, 0x2b7f},	/* index 11  -0.039406     */
	{0x62b3, 6625, 0x2a7f},	/* index 12  -0.023262     */
	{0x6096, 6593, 0x297f},	/* index 13  -0.010597     */
	{0x5e84, 6562, 0x287f},	/* index 14  -0.001549     */
	{0x5c7d, 6531, 0x277f},	/* index 15  0.003723      */
	{0x5a82, 6500, 0x267f},	/* index 16  0.005066      */
	{0x5891, 6468, 0x257f},	/* index 17  0.002308      */
	{0x56ac, 6437, 0x247f},	/* index 18  -0.004745     */
	{0x54d0, 6406, 0x237f},	/* index 19  -0.016289     */
	{0x52ff, 6375, 0x227f},	/* index 20  -0.032528     */
	{0x5138, 6343, 0x217f},	/* index 21  -0.053692     */
	{0x4f7a, 6312, 0x207f},	/* index 22  -0.080025     */
	{0x4dc6, 6281, 0x207f},	/* index 23  0.108116      */
	{0x4c1b, 6250, 0x1f7f},	/* index 24  0.076355      */
	{0x4a7a, 6218, 0x1e7f},	/* index 25  0.038879      */
	{0x48e1, 6187, 0x1d7f},	/* index 26  -0.004616     */
	{0x4752, 6156, 0x1c7f},	/* index 27  -0.054459     */
	{0x45ca, 6125, 0x1b7f},	/* index 28  -0.111004     */
	{0x444c, 6093, 0x1b7f},	/* index 29  0.077141      */
	{0x42d5, 6062, 0x1a7f},	/* index 30  0.013504      */
	{0x4166, 6031, 0x197f},	/* index 31  -0.057655     */
	{0x4000, 6000, 0x197f},	/* index 32  0.130489     : 36dB */
	{0x3ea0, 5968, 0x187f},	/* index 33  0.051353      */
	{0x3d49, 5937, 0x177f},	/* index 34  -0.036266     */
	{0x3bf9, 5906, 0x167f},	/* index 35  -0.132935     */
	{0x3ab0, 5875, 0x167f},	/* index 36  0.055210      */
	{0x396e, 5843, 0x157f},	/* index 37  -0.051109     */
	{0x3833, 5812, 0x157f},	/* index 38  0.137035      */
	{0x36fe, 5781, 0x147f},	/* index 39  0.020378      */
	{0x35d1, 5750, 0x137f},	/* index 40  -0.107365     */
	{0x34aa, 5718, 0x137f},	/* index 41  0.080780      */
	{0x3389, 5687, 0x127f},	/* index 42  -0.058884     */
	{0x326e, 5656, 0x127f},	/* index 43  0.129257      */
	{0x3159, 5625, 0x117f},	/* index 44  -0.023266     */
	{0x304b, 5593, 0x117f},	/* index 45  0.164879      */
	{0x2f42, 5562, 0x107f},	/* index 46  -0.001549     */
	{0x2e3e, 5531, 0x0f7f},	/* index 47  -0.183079     */
	{0x2d41, 5500, 0x0f7f},	/* index 48  0.005066      */
	{0x2c48, 5468, 0x0e7f},	/* index 49  -0.192890     */
	{0x2b56, 5437, 0x0e7f},	/* index 50  -0.004745     */
	{0x2a68, 5406, 0x0e7f},	/* index 51  0.183395      */
	{0x297f, 5375, 0x0d7f},	/* index 52  -0.032528     */
	{0x289c, 5343, 0x0d7f},	/* index 53  0.155617      */
	{0x27bd, 5312, 0x0c7f},	/* index 54  -0.080027     */
	{0x26e3, 5281, 0x0c7f},	/* index 55  0.108118      */
	{0x260d, 5250, 0x0b7f},	/* index 56  -0.149267     */
	{0x253d, 5218, 0x0b7f},	/* index 57  0.038877      */
	{0x2470, 5187, 0x0b7f},	/* index 58  0.227022      */
	{0x23a9, 5156, 0x0a7f},	/* index 59  -0.054457     */
	{0x22e5, 5125, 0x0a7f},	/* index 60  0.133688      */
	{0x2226, 5093, 0x097f},	/* index 61  -0.174641     */
	{0x216a, 5062, 0x097f},	/* index 62  0.013504      */
	{0x20b3, 5031, 0x097f},	/* index 63  0.201647      */
	{0x2000, 5000, 0x087f},	/* index 64  -0.136789    : 30dB */
	{0x1f50, 4968, 0x087f},	/* index 65  0.051353      */
	{0x1ea4, 4937, 0x087f},	/* index 66  0.239498      */
	{0x1dfc, 4906, 0x077f},	/* index 67  -0.132933     */
	{0x1d58, 4875, 0x077f},	/* index 68  0.055212      */
	{0x1cb7, 4843, 0x077f},	/* index 69  0.243355      */
	{0x1c19, 4812, 0x067f},	/* index 70  -0.167765     */
	{0x1b7f, 4781, 0x067f},	/* index 71  0.020378      */
	{0x1ae8, 4750, 0x067f},	/* index 72  0.208523      */
	{0x1a55, 4718, 0x057f},	/* index 73  -0.247028     */
	{0x19c4, 4687, 0x057f},	/* index 74  -0.058884     */
	{0x1937, 4656, 0x057f},	/* index 75  0.129259      */
	{0x18ac, 4625, 0x057f},	/* index 76  0.317404      */
	{0x1825, 4593, 0x047f},	/* index 77  -0.189695     */
	{0x17a1, 4562, 0x047f},	/* index 78  -0.001551     */
	{0x171f, 4531, 0x047f},	/* index 79  0.186592      */
	{0x16a0, 4500, 0x047f},	/* index 80  0.374737      */
	{0x1624, 4468, 0x037f},	/* index 81  -0.192892     */
	{0x15ab, 4437, 0x037f},	/* index 82  -0.004747     */
	{0x1534, 4406, 0x037f},	/* index 83  0.183395      */
	{0x14bf, 4375, 0x037f},	/* index 84  0.371540      */
	{0x144e, 4343, 0x027f},	/* index 85  -0.268171     */
	{0x13de, 4312, 0x027f},	/* index 86  -0.080027     */
	{0x1371, 4281, 0x027f},	/* index 87  0.108116      */
	{0x1306, 4250, 0x027f},	/* index 88  0.296261      */
	{0x129e, 4218, 0x017f},	/* index 89  -0.430746     */
	{0x1238, 4187, 0x017f},	/* index 90  -0.242601     */
	{0x11d4, 4156, 0x017f},	/* index 91  -0.054459     */
	{0x1172, 4125, 0x017f},	/* index 92  0.133686      */
	{0x1113, 4093, 0x017f},	/* index 93  0.321829      */
	{0x10b5, 4062, 0x017f},	/* index 94  0.509974      */
	{0x1059, 4031, 0x007f},	/* index 95  -0.324934     */
	{0x1000, 4000, 0x0860},	/* index 96  0.000000     : 24dB */
	{0x0fa8, 3968, 0x007f},	/* index 97  0.051353      */
	{0x0f52, 3937, 0x007d},	/* index 98  -0.040716     */
	{0x0efe, 3906, 0x007c},	/* index 99  0.003855      */
	{0x0eac, 3875, 0x007b},	/* index 100 0.046015      */
	{0x0e5b, 3843, 0x0079},	/* index 101 -0.065386     */
	{0x0e0c, 3812, 0x0078},	/* index 102 -0.030977     */
	{0x0dbf, 3781, 0x0077},	/* index 103 0.000660      */
	{0x0d74, 3750, 0x0076},	/* index 104 0.029425      */
	{0x0d2a, 3718, 0x0075},	/* index 105 0.055210      */
	{0x0ce2, 3687, 0x0074},	/* index 106 0.077904      */
	{0x0c9b, 3656, 0x0072},	/* index 107 -0.074619     */
	{0x0c56, 3625, 0x0071},	/* index 108 -0.061953     */
	{0x0c12, 3593, 0x0070},	/* index 109 -0.052906     */
	{0x0bd0, 3562, 0x006f},	/* index 110 -0.047630     */
	{0x0b8f, 3531, 0x006e},	/* index 111 -0.046288     */
	{0x0b50, 3500, 0x006d},	/* index 112 -0.049049     */
	{0x0b12, 3468, 0x006c},	/* index 113 -0.056101     */
	{0x0ad5, 3437, 0x006b},	/* index 114 -0.067642     */
	{0x0a9a, 3406, 0x006a},	/* index 115 -0.083881     */
	{0x0a5f, 3375, 0x006a},	/* index 116 0.104261      */
	{0x0a27, 3343, 0x0069},	/* index 117 0.083097      */
	{0x09ef, 3312, 0x0068},	/* index 118 0.056763      */
	{0x09b8, 3281, 0x0067},	/* index 119 0.025000      */
	{0x0983, 3250, 0x0066},	/* index 120 -0.012478     */
	{0x094f, 3218, 0x0065},	/* index 121 -0.055971     */
	{0x091c, 3187, 0x0064},	/* index 122 -0.105812     */
	{0x08ea, 3156, 0x0064},	/* index 123 0.082333      */
	{0x08b9, 3125, 0x0063},	/* index 124 0.025785      */
	{0x0889, 3093, 0x0062},	/* index 125 -0.037851     */
	{0x085a, 3062, 0x0061},	/* index 126 -0.109009     */
	{0x082c, 3031, 0x0061},	/* index 127 0.079136      */
	{0x0800, 3000, 0x0060},	/* index 128 0.000000      : 18dB */
	{0x07d4, 2968, 0x003f},	/* index 129 0.051355      */
	{0x07a9, 2937, 0x003d},	/* index 130 -0.040716     */
	{0x077f, 2906, 0x003c},	/* index 131 0.003857      */
	{0x0756, 2875, 0x003b},	/* index 132 0.046015      */
	{0x072d, 2843, 0x0039},	/* index 133 -0.065384     */
	{0x0706, 2812, 0x0038},	/* index 134 -0.030977     */
	{0x06df, 2781, 0x0037},	/* index 135 0.000662      */
	{0x06ba, 2750, 0x0036},	/* index 136 0.029425      */
	{0x0695, 2718, 0x0035},	/* index 137 0.055212      */
	{0x0671, 2687, 0x0034},	/* index 138 0.077904      */
	{0x064d, 2656, 0x0032},	/* index 139 -0.074618     */
	{0x062b, 2625, 0x0031},	/* index 140 -0.061954     */
	{0x0609, 2593, 0x0030},	/* index 141 -0.052905     */
	{0x05e8, 2562, 0x002f},	/* index 142 -0.047629     */
	{0x05c7, 2531, 0x002e},	/* index 143 -0.046287     */
	{0x05a8, 2500, 0x002d},	/* index 144 -0.049048     */
	{0x0589, 2468, 0x002c},	/* index 145 -0.056102     */
	{0x056a, 2437, 0x002b},	/* index 146 -0.067642     */
	{0x054d, 2406, 0x002a},	/* index 147 -0.083882     */
	{0x052f, 2375, 0x002a},	/* index 148 0.104261      */
	{0x0513, 2343, 0x0029},	/* index 149 0.083097      */
	{0x04f7, 2312, 0x0028},	/* index 150 0.056763      */
	{0x04dc, 2281, 0x0027},	/* index 151 0.025000      */
	{0x04c1, 2250, 0x0026},	/* index 152 -0.012477     */
	{0x04a7, 2218, 0x0025},	/* index 153 -0.055971     */
	{0x048e, 2187, 0x0024},	/* index 154 -0.105812     */
	{0x0475, 2156, 0x0024},	/* index 155 0.082332      */
	{0x045c, 2125, 0x0023},	/* index 156 0.025786      */
	{0x0444, 2093, 0x0022},	/* index 157 -0.037852     */
	{0x042d, 2062, 0x0021},	/* index 158 -0.109008     */
	{0x0416, 2031, 0x0021},	/* index 159 0.079136      */
	{0x0400, 2000, 0x0020},	/* index 160 0.000000     : 12dB */
	{0x03ea, 1968, 0x0219},	/* index 161 -0.017856     */
	{0x03d4, 1937, 0x0513},	/* index 162 0.065427      */
	{0x03bf, 1906, 0x001e},	/* index 163 0.003857      */
	{0x03ab, 1875, 0x011a},	/* index 164 -0.027907     */
	{0x0396, 1843, 0x0217},	/* index 165 0.010476      */
	{0x0383, 1812, 0x0119},	/* index 166 0.007713      */
	{0x036f, 1781, 0x0216},	/* index 167 0.000660      */
	{0x035d, 1750, 0x001b},	/* index 168 0.029426      */
	{0x034a, 1718, 0x0215},	/* index 169 -0.027120     */
	{0x0338, 1687, 0x0117},	/* index 170 0.036045      */
	{0x0326, 1656, 0x0019},	/* index 171 -0.074618     */
	{0x0315, 1625, 0x0116},	/* index 172 0.026229      */
	{0x0304, 1593, 0x0018},	/* index 173 -0.052906     */
	{0x02f4, 1562, 0x0115},	/* index 174 -0.001551     */
	{0x02e3, 1531, 0x0017},	/* index 175 -0.046287     */
	{0x02d4, 1500, 0x050e},	/* index 176 0.046928      */
	{0x02c4, 1468, 0x0016},	/* index 177 -0.056103     */
	{0x02b5, 1437, 0x0113},	/* index 178 -0.118290     */
	{0x02a6, 1406, 0x0945},	/* index 179 0.018909      */
	{0x0297, 1375, 0x030f},	/* index 180 -0.052246     */
	{0x0289, 1343, 0x0112},	/* index 181 -0.023480     */
	{0x027b, 1312, 0x0845},	/* index 182 0.056762      */
	{0x026e, 1281, 0x040d},	/* index 183 0.024999      */
	{0x0260, 1250, 0x0013},	/* index 184 -0.012477     */
	{0x0253, 1218, 0x0745},	/* index 185 0.060619      */
	{0x0247, 1187, 0x0d07},	/* index 186 0.073286      */
	{0x023a, 1156, 0x030d},	/* index 187 0.021802      */
	{0x022e, 1125, 0x0247},	/* index 188 0.025786      */
	{0x0222, 1093, 0x0011},	/* index 189 -0.037852     */
	{0x0216, 1062, 0x0b07},	/* index 190 -0.043454     */
	{0x020b, 1031, 0x020d},	/* index 191 -0.053476     */
	{0x0200, 1000, 0x0010},	/* index 192 0.000000      : 6dB */
	{0x01f5, 968, 0x0a07},	/* index 193 0.051355      */
	{0x01ea, 937, 0x030b},	/* index 194 -0.112205     */
	{0x01df, 906, 0x0445},	/* index 195 0.003856      */
	{0x01d5, 875, 0x010d},	/* index 196 -0.027907     */
	{0x01cb, 843, 0x0f05},	/* index 197 0.010476      */
	{0x01c1, 812, 0x000e},	/* index 198 -0.030976     */
	{0x01b7, 781, 0x0e05},	/* index 199 0.000660      */
	{0x01ae, 750, 0x0a06},	/* index 200 0.029425      */
	{0x01a5, 718, 0x0d05},	/* index 201 -0.027120     */
	{0x019c, 687, 0x000d},	/* index 202 0.077905      */
	{0x0193, 656, 0x020a},	/* index 203 -0.074618     */
	{0x018a, 625, 0x010b},	/* index 204 0.026230      */
	{0x0182, 593, 0x000c},	/* index 205 -0.052906     */
	{0x017a, 562, 0x0b05},	/* index 206 0.044285      */
	{0x0171, 531, 0x0f04},	/* index 207 -0.046286     */
	{0x016a, 500, 0x0507},	/* index 208 0.046928      */
	{0x0162, 468, 0x000b},	/* index 209 -0.056102     */
	{0x015a, 437, 0x0e04},	/* index 210 0.132041      */
	{0x0153, 406, 0x0905},	/* index 211 0.018910      */
	{0x014b, 375, 0x0407},	/* index 212 0.104261      */
	{0x0144, 343, 0x0109},	/* index 213 -0.023480     */
	{0x013d, 312, 0x0805},	/* index 214 0.056763      */
	{0x0137, 281, 0x0506},	/* index 215 0.024999      */
	{0x0130, 250, 0x0b04},	/* index 216 -0.012478     */
	{0x0129, 218, 0x0705},	/* index 217 0.060619      */
	{0x0123, 187, 0x0009},	/* index 218 -0.105812     */
	{0x011d, 156, 0x0406},	/* index 219 0.082332      */
	{0x0117, 125, 0x0207},	/* index 220 0.025786      */
	{0x0111, 93, 0x0904},	/* index 221 -0.037852     */
	{0x010b, 62, 0x0306},	/* index 222 -0.109008     */
	{0x0105, 31, 0x0505},	/* index 223 -0.053476     */
	{0x0100, 0, 0x0008},	/* index 224 0.000000      : 0dB */
	{0x00fa, -31, 0x0107},	/* index 225  0.051355    */
	{0x00f5, -62, 0x0206},	/* index 226  -0.184287   */
	{0x00ef, -93, 0x0405},	/* index 227  0.003856    */
	{0x00ea, -125, 0x0405},	/* index 228  0.192000    */
	{0x00e5, -156, 0x0604},	/* index 229  -0.219120   */
	{0x00e0, -187, 0x0604},	/* index 230 -0.030976    */
	{0x00db, -218, 0x0305},	/* index 231 0.000660     */
	{0x00d7, -250, 0x0106},	/* index 232 0.029426     */
	{0x00d2, -281, 0x0504},	/* index 233 -0.110239    */
	{0x00ce, -312, 0x0504},	/* index 234 0.077905     */
	{0x00c9, -343, 0x0205},	/* index 235 -0.074618    */
	{0x00c5, -375, 0x0205},	/* index 236 0.113525     */
	{0x00c1, -406, 0x0006},	/* index 237 -0.052906    */
	{0x00bd, -437, 0x0006},	/* index 238 0.135238     */
	{0x00b8, -468, 0x0105},	/* index 239 -0.237193    */
	{0x00b5, -500, 0x0105},	/* index 240 -0.049050    :-3dB */
	{0x00b1, -531, 0x0304},	/* index 241 -0.056102    */
	{0x00ad, -562, 0x0304},	/* index 242 0.132041     */
	{0x00a9, -593, 0x0304},	/* index 243 0.320185     */
	{0x00a5, -625, 0x0204},	/* index 244 -0.319525    */
	{0x00a2, -656, 0x0204},	/* index 245 -0.131381    */
	{0x009e, -687, 0x0005},	/* index 246 0.056763     */
	{0x009b, -718, 0x0005},	/* index 247 0.244906     */
	{0x0098, -750, 0x0005},	/* index 248 0.433050     */
	{0x0094, -781, 0x0104},	/* index 249 -0.293956    */
	{0x0091, -812, 0x0104},	/* index 250 -0.105812    */
	{0x008e, -843, 0x0104},	/* index 251 0.082332     */
	{0x008b, -875, 0x0104},	/* index 252 0.270475     */
	{0x0088, -906, 0x0104},	/* index 253 0.458619     */
	{0x0085, -937, 0x0004},	/* index 254  -0.376287   */
	{0x0082, -968, 0x0004}	/* index 255  -0.188144   */

};

#else				//16-step
const s16 MT9PXXX_GLOBAL_GAIN_TABLE[MT9P001_GAIN_ROWS][MT9P001_GAIN_COLS] = {
	/* gain_value*256,  log2(gain)*1000,  register */
	/* error */
	{0x8000, 7000, 0x7860},	/* index 0   0.000000   : 42dB */
	{0x7a92, 6937, 0x7360},	/* index 1   0.426781   */
	{0x7560, 6875, 0x6d60},	/* index 2   -0.376518  */
	{0x7066, 6812, 0x6860},	/* index 3   -0.400139  */
	{0x6ba2, 6750, 0x6460},	/* index 4   0.365257   */
	{0x6712, 6687, 0x5f60},	/* index 5   -0.071381  */
	{0x62b3, 6625, 0x5b60},	/* index 6   0.298508   */
	{0x5e84, 6562, 0x5760},	/* index 7   0.483124   */
	{0x5a82, 6500, 0x5360},	/* index 8   0.490334   */
	{0x56ac, 6437, 0x4f60},	/* index 9   0.327644   */
	{0x52ff, 6375, 0x4b60},	/* index 10  0.002266   */
	{0x4f7a, 6312, 0x4760},	/* index 11  -0.478897  */
	{0x4c1b, 6250, 0x4460},	/* index 12  -0.109253  */
	{0x48e1, 6187, 0x4160},	/* index 13  0.117531   */
	{0x45ca, 6125, 0x3e60},	/* index 14  0.207504   */
	{0x42d5, 6062, 0x3b60},	/* index 15  0.166481   */
	{0x4000, 6000, 0x3860},	/* index 16  0.000000   : 36dB */
	{0x3d49, 5937, 0x3560},	/* index 17  -0.286610  */
	{0x3ab0, 5875, 0x3360},	/* index 18  0.311741   */
	{0x3833, 5812, 0x3060},	/* index 19  -0.200069  */
	{0x35d1, 5750, 0x2e60},	/* index 20  0.182629   */
	{0x3389, 5687, 0x2c60},	/* index 21  0.464310   */
	{0x3159, 5625, 0x2960},	/* index 22  -0.350746  */
	{0x2f42, 5562, 0x2760},	/* index 23  -0.258438  */
	{0x2d41, 5500, 0x2560},	/* index 24  -0.254833  */
	{0x2b56, 5437, 0x2360},	/* index 25  -0.336178  */
	{0x297f, 5375, 0x2160},	/* index 26  -0.498867  */
	{0x27bd, 5312, 0x2060},	/* index 27  0.260551   */
	{0x260d, 5250, 0x1e60},	/* index 28  -0.054626  */
	{0x2470, 5187, 0x1c60},	/* index 29  -0.441235  */
	{0x22e5, 5125, 0x1b60},	/* index 30  0.103752   */
	{0x216a, 5062, 0x1960},	/* index 31  -0.416759  */
	{0x2000, 5000, 0x1860},	/* index 32  0.000000   : 30dB */
	{0x1ea4, 4937, 0x1760},	/* index 33  0.356695   */
	{0x1d58, 4875, 0x1560},	/* index 34  -0.344130  */
	{0x1c19, 4812, 0x1460},	/* index 35  -0.100035  */
	{0x1ae8, 4750, 0x1360},	/* index 36  0.091314   */
	{0x19c4, 4687, 0x1260},	/* index 37  0.232155   */
	{0x18ac, 4625, 0x1160},	/* index 38  0.324627   */
	{0x17a1, 4562, 0x1060},	/* index 39  0.370781   */
	{0x16a0, 4500, 0x0f60},	/* index 40  0.372583   */
	{0x15ab, 4437, 0x0e60},	/* index 41  0.331911   */
	{0x14bf, 4375, 0x0d60},	/* index 42  0.250566   */
	{0x13de, 4312, 0x0c60},	/* index 43  0.130276   */
	{0x1306, 4250, 0x0b60},	/* index 44  -0.027313  */
	{0x1238, 4187, 0x0a60},	/* index 45  -0.220617  */
	{0x1172, 4125, 0x0960},	/* index 46  -0.448124  */
	{0x10b5, 4062, 0x0960},	/* index 47  0.291620   */
	{0x1000, 4000, 0x0860},	/* index 48  0.000000   : 24dB */
	{0x0f52, 3937, 0x085f},	/* index 49  -0.321652  */
	{0x0eac, 3875, 0x085d},	/* index 50  0.327935   */
	{0x0e0c, 3812, 0x085c},	/* index 51  -0.050017  */
	{0x0d74, 3750, 0x085b},	/* index 52  -0.454343  */
	{0x0ce2, 3687, 0x085a},	/* index 53  0.116077   */
	{0x0c56, 3625, 0x0859},	/* index 54  -0.337687  */
	{0x0bd0, 3562, 0x0858},	/* index 55  0.185390   */
	{0x0b50, 3500, 0x0857},	/* index 56  -0.313708  */
	{0x0ad5, 3437, 0x0856},	/* index 57  0.165956   */
	{0x0a5f, 3375, 0x0855},	/* index 58  -0.374717  */
	{0x09ef, 3312, 0x0854},	/* index 59  0.065138   */
	{0x0983, 3250, 0x0853},	/* index 60  0.486343   */
	{0x091c, 3187, 0x0852},	/* index 61  -0.110309  */
	{0x08b9, 3125, 0x0851},	/* index 62  0.275938   */
	{0x085a, 3062, 0x0851},	/* index 63  -0.354190  */
	{0x0800, 3000, 0x0820},	/* index 64  0.000000   : 18dB */
	{0x07a9, 2937, 0x005f},	/* index 65  0.356695   */
	{0x0756, 2875, 0x005e},	/* index 66  -0.344130  */
	{0x0706, 2812, 0x005c},	/* index 67  -0.100035  */
	{0x06ba, 2750, 0x005b},	/* index 68  0.091314   */
	{0x0671, 2687, 0x005a},	/* index 69  0.232155   */
	{0x062b, 2625, 0x0059},	/* index 70  0.324627   */
	{0x05e8, 2562, 0x0058},	/* index 71  0.370781   */
	{0x05a8, 2500, 0x0057},	/* index 72  0.372583   */
	{0x056a, 2437, 0x0056},	/* index 73  0.331911   */
	{0x052f, 2375, 0x0055},	/* index 74  0.250566   */
	{0x04f7, 2312, 0x0054},	/* index 75  0.130276   */
	{0x04c1, 2250, 0x0053},	/* index 76  -0.027313  */
	{0x048e, 2187, 0x0052},	/* index 77  -0.220617  */
	{0x045c, 2125, 0x011F},	/* index 78  -0.448124  */
	{0x042d, 2062, 0x0051},	/* index 79  0.291620   */
	{0x0400, 2000, 0x0020},	/* index 80  0.000000   : 12dB */
	{0x03d4, 1937, 0x001f},	/* index 81  0.356695   */
	{0x03ab, 1875, 0x001d},	/* index 82  -0.344130  */
	{0x0383, 1812, 0x001c},	/* index 83  -0.100035  */
	{0x035d, 1750, 0x001b},	/* index 84  0.091314   */
	{0x0338, 1687, 0x001a},	/* index 85  0.232155   */
	{0x0315, 1625, 0x0019},	/* index 86  0.324627   */
	{0x02f4, 1562, 0x0018},	/* index 87  0.370781   */
	{0x02d4, 1500, 0x0017},	/* index 88  0.372583   */
	{0x02b5, 1437, 0x0016},	/* index 89  0.331911   */
	{0x0297, 1375, 0x0015},	/* index 90  0.250566   */
	{0x027b, 1312, 0x0014},	/* index 91  0.130276   */
	{0x0260, 1250, 0x0013},	/* index 92  -0.027313  */
	{0x0247, 1187, 0x0012},	/* index 93  -0.220617  */
	{0x022e, 1125, 0x0011},	/*0x0C07}, *//* index 94  -0.448124  */
	{0x0216, 1062, /*0x0011}, */ 0x0011},	/* index 95  0.291620   */
	{0x0200, 1000, /*0x0010}, */ 0x0010},	/* index 96  0.000000   : 6dB */
	{0x01ea, 937, /*0x000f}, */ 0x1704},	/* index 97  -0.321652  */
	{0x01d5, 875, /*0x000f}, */ 0x000f},	/* index 98  0.327935   */
	{0x01c1, 812, /*0x000e}, */ 0x000e},	/* index 99  -0.050017  */
	{0x01ae, 750, /*0x000d}, */ 0x0146},	/* index 100 -0.454343  */
	{0x019c, 687, /*0x000d}, */ 0x000d},	/* index 101 0.116077   */
	{0x018a, 625, /*0x000c}, */ 0x010B},	/* index 102 -0.337687  */
	{0x017a, 562, /*0x000c}, */ 0x000c},	/* index 103 0.185390   */
	{0x016a, 500, /*0x000b}, */ 0x0507},	/* index 104 -0.313708  */
	{0x015a, 437, /*0x000b}, */ 0x000b},	/* index 105 0.165956   */
	{0x014b, 375, /*0x000a}, */ 0x0D04},	/* index 106 -0.374717  */
	{0x013d, 312, /*0x000a}, */ 0x0805},	/* index 107 0.065138   */
	{0x0130, 250, /*0x000a}, */ 0x000a},	/* index 108 0.486343   */
	{0x0123, 187, /*0x0009}, */ 0x0A04},	/* index 109 -0.110309  */
	{0x0117, 125, /*0x0009}, */ 0x0009},	/* index 110 0.275938   */
	{0x010b, 62, /*0x0008}, */ 0x0306},	/* index 111 -0.354190  */
	{0x0100, 0, 0x0008},	/* index 112 0.000000   : 0dB */
	{0x00f5, -62, 0x0008},	/* index 113  0.339174  */
	{0x00ea, -125, 0x0007},	/* index 114  -0.336032 */
	{0x00e0, -187, 0x0007},	/* index 115  -0.025009 */
	{0x00d7, -250, 0x0007},	/* index 116  0.272829  */
	{0x00ce, -312, 0x0006},	/* index 117  -0.441961 */
	{0x00c5, -375, 0x0006},	/* index 118  -0.168843 */
	{0x00bd, -437, 0x0006},	/* index 119  0.092695  */
	{0x00b5, -500, 0x0006},	/* index 120  0.343146  */
	{0x00ad, -562, 0x0005},	/* index 121  -0.417022 */
	{0x00a5, -625, 0x0005},	/* index 122  -0.187358 */
	{0x009e, -687, 0x0005},	/* index 123  0.032569  */
	{0x0098, -750, 0x0005},	/* index 124  0.243172  */
	{0x0091, -812, 0x0005},	/* index 125  0.444846  */
	{0x008b, -875, 0x0004},	/* index 126  -0.362031 */
	{0x0085, -937, 0x0004},	/* index 127  -0.177095 */
	{0x0080, -1000, 0x0004},	/* index 128  0.000000  */
	{0x007a, -1062, 0x0004},	/* index 129  0.169587  */
	{0x0075, -1125, 0x0004},	/* index 130  0.331984  */
	{0x0070, -1187, 0x0004},	/* index 131  0.487496  */
	{0x006b, -1250, 0x0003},	/* index 132  -0.363586 */
	{0x0067, -1312, 0x0003},	/* index 133  -0.220981 */
	{0x0062, -1375, 0x0003},	/* index 134  -0.084422 */
	{0x005e, -1437, 0x0003},	/* index 135  0.046348  */
	{0x005a, -1500, 0x0003},	/* index 136  0.171573  */
	{0x0056, -1562, 0x0003},	/* index 137  0.291489  */
	{0x0052, -1625, 0x0003},	/* index 138  0.406321  */
	{0x004f, -1687, 0x0002},	/* index 139  -0.483716 */
	{0x004c, -1750, 0x0002},	/* index 140  -0.378414 */
	{0x0048, -1812, 0x0002},	/* index 141  -0.277577 */
	{0x0045, -1875, 0x0002},	/* index 142  -0.181015 */
	{0x0042, -1937, 0x0002},	/* index 143  -0.088547 */
	{0x0040, -2000, 0x0002},	/* index 144  0.000000  */
	{0x003d, -2062, 0x0002},	/* index 145  0.084793  */
	{0x003a, -2125, 0x0002},	/* index 146  0.165992  */
	{0x0038, -2187, 0x0002},	/* index 147  0.243748  */
	{0x0035, -2250, 0x0002},	/* index 148  0.318207  */
	{0x0033, -2312, 0x0002},	/* index 149  0.389510  */
	{0x0031, -2375, 0x0002},	/* index 150  0.457789  */
	{0x002f, -2437, 0x0001},	/* index 151  -0.476826 */
	{0x002d, -2500, 0x0001},	/* index 152  -0.414214 */
	{0x002b, -2562, 0x0001},	/* index 153  -0.354256 */
	{0x0029, -2625, 0x0001},	/* index 154  -0.296840 */
	{0x0027, -2687, 0x0001},	/* index 155  -0.241858 */
	{0x0026, -2750, 0x0001},	/* index 156  -0.189207 */
	{0x0024, -2812, 0x0001},	/* index 157  -0.138789 */
	{0x0022, -2875, 0x0001},	/* index 158  -0.090508 */
	{0x0021, -2937, 0x0001},	/* index 159  -0.044274 */
	{0x0020, -3000, 0x0001},	/* index 160  0.000000  */
	{0x001f, -3000, 0x0001}	/* index 161  0.042397  */
};
#endif

/* ========================================================================== */
static const struct mt9p001_pll_reg_table mt9p001_pll_tbl[] = {
#ifdef CONFIG_SENSOR_MT9PXXX_ENABLE_PLL
	[0] = {
	       .pixclk = PLL_CLK_96MHZ,
	       .extclk = PLL_CLK_27MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x2002},
			{MT9P001_PLL_CONFIG2, 0x0002},
			}
	       },
	[1] = {
	       .pixclk = PLL_CLK_96D1001MHZ,
	       .extclk = PLL_CLK_27D1001MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x2002},
			{MT9P001_PLL_CONFIG2, 0x0002},
			}
	       },
	[2] = {
	       .pixclk = ((PLL_CLK_24MHZ * 0x48) / (9 * 10)),
	       .extclk = PLL_CLK_24MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x4808},
			{MT9P001_PLL_CONFIG2, 0x0009},
			}
	       },
	[3] = {
	       .pixclk = PLL_CLK_27MHZ,
	       .extclk = PLL_CLK_27MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x1802},
			{MT9P001_PLL_CONFIG2, 0x0007},
			}
	       },
	[4] = {
	       .pixclk = PLL_CLK_48MHZ,
	       .extclk = PLL_CLK_24MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x1001},
			{MT9P001_PLL_CONFIG2, 0x0003},
			}
	       },
	[5] = {
	       .pixclk = PLL_CLK_48D1001MHZ,
	       .extclk = PLL_CLK_24D1001MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x1001},
			{MT9P001_PLL_CONFIG2, 0x0003},
			}
	       },
	[6] = {
	       .pixclk = PLL_CLK_24MHZ,
	       .extclk = PLL_CLK_24MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x1001},
			{MT9P001_PLL_CONFIG2, 0x0007},
			}
	       },
	[7] = {
	       .pixclk = PLL_CLK_13_5MHZ,
	       .extclk = PLL_CLK_13_5MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x3001},
			{MT9P001_PLL_CONFIG2, 0x0017},
			}
	       },
	[8] = {
	       .pixclk = PLL_CLK_74_25MHZ,
	       .extclk = PLL_CLK_13_5MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x2c01},
			{MT9P001_PLL_CONFIG2, 0x0003},
			}
	       },
	[9] = {
	       .pixclk = PLL_CLK_74_25D1001MHZ,
	       .extclk = PLL_CLK_13_5D1001MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x2c01},
			{MT9P001_PLL_CONFIG2, 0x0003},
			}
	       },
	[10] = {
	       .pixclk = PLL_CLK_60MHZ,
	       .extclk = PLL_CLK_24MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x1401},
			{MT9P001_PLL_CONFIG2, 0x0003},
			}
	       },
#else
	[0] = {
	       .pixclk = PLL_CLK_96MHZ,
	       .extclk = PLL_CLK_96MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0050},
			{MT9P001_PLL_CONFIG1, 0x6404},
			{MT9P001_PLL_CONFIG2, 0x0000},
			}
	       },
	[1] = {
	       .pixclk = PLL_CLK_96D1001MHZ,
	       .extclk = PLL_CLK_96D1001MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0050},
			{MT9P001_PLL_CONFIG1, 0x6404},
			{MT9P001_PLL_CONFIG2, 0x0000},
			}
	       },
	[2] = {
	       .pixclk = ((PLL_CLK_24MHZ * 0x48) / (9 * 10)),
	       .extclk = PLL_CLK_24MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0053},
			{MT9P001_PLL_CONFIG1, 0x4808},
			{MT9P001_PLL_CONFIG2, 0x0009},
			}
	       },
	[3] = {
	       .pixclk = PLL_CLK_27MHZ,
	       .extclk = PLL_CLK_27MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0050},
			{MT9P001_PLL_CONFIG1, 0x6404},
			{MT9P001_PLL_CONFIG2, 0x0000},
			}
	       },
	[4] = {
	       .pixclk = PLL_CLK_48MHZ,
	       .extclk = PLL_CLK_48MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0050},
			{MT9P001_PLL_CONFIG1, 0x6404},
			{MT9P001_PLL_CONFIG2, 0x0000},
			}
	       },
	[5] = {
	       .pixclk = PLL_CLK_48D1001MHZ,
	       .extclk = PLL_CLK_48D1001MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0050},
			{MT9P001_PLL_CONFIG1, 0x6404},
			{MT9P001_PLL_CONFIG2, 0x0000},
			}
	       },
	[6] = {
	       .pixclk = PLL_CLK_24MHZ,
	       .extclk = PLL_CLK_24MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0050},
			{MT9P001_PLL_CONFIG1, 0x6404},
			{MT9P001_PLL_CONFIG2, 0x0000},
			}
	       },
	[7] = {
	       .pixclk = PLL_CLK_13_5MHZ,
	       .extclk = PLL_CLK_13_5MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0050},
			{MT9P001_PLL_CONFIG1, 0x6404},
			{MT9P001_PLL_CONFIG2, 0x0000},
			}
	       },
	[8] = {
	       .pixclk = PLL_CLK_74_25MHZ,
	       .extclk = PLL_CLK_74_25MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0050},
			{MT9P001_PLL_CONFIG1, 0x6404},
			{MT9P001_PLL_CONFIG2, 0x0000},
			}
	       },
	[9] = {
	       .pixclk = PLL_CLK_74_25D1001MHZ,
	       .extclk = PLL_CLK_74_25D1001MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0050},
			{MT9P001_PLL_CONFIG1, 0x6404},
			{MT9P001_PLL_CONFIG2, 0x0000},
			}
	       },
	[10] = {
	       .pixclk = PLL_CLK_60MHZ,
	       .extclk = PLL_CLK_60MHZ,
	       .regs = {
			{MT9P001_PLL_CTRL, 0x0050},
			{MT9P001_PLL_CONFIG1, 0x6404},
			{MT9P001_PLL_CONFIG2, 0x0000},
			}
	       },
#endif
};
