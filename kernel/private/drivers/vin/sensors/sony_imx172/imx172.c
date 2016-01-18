/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx172/imx172.c
 *
 * History:
 *    2012/08/22 - [Cao Rongrong] Create
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

#include <dsp_cmd.h>
#include <vin_pri.h>
#include <linux/bitrev.h>

#include "utils.h"
#include "imx172_pri.h"
#include "shutter_table.h"

/* ========================================================================== */
AMBA_VIN_BASIC_PARAM_CALL(imx172, 0x82, 0, 0644);

#define IMX172_WRITE_ADDR 0x81
#define IMX172_READ_ADDR  0x82

static int fps120_mode = 0;
module_param(fps120_mode, int, 0644);
MODULE_PARM_DESC(fps120_mode, "120fps mode , 0:off 1:on");
static int spi_bus = 0;
module_param(spi_bus, int, 0644);
MODULE_PARM_DESC(spi_bus, "SPI controller ID");
static int spi_cs = 0;
module_param(spi_cs, int, 0644);
MODULE_PARM_DESC(spi_cs, "SPI CS");
/* ========================================================================== */
struct imx172_info {
	struct spi_device *spi;
	struct __amba_vin_source	source;
	int				current_video_index;
	enum amba_video_mode 		current_vin_mode;
	u16				cap_start_x;
	u16				cap_start_y;
	u16				cap_cap_w;
	u16				cap_cap_h;
	u16				act_start_x;
	u16				act_start_y;
	u16				act_width;
	u16				act_height;
	u16				slvs_eav_col;
	u16				slvs_sav2sav_dist;
	u8				active_capinfo_num;
	u8				bayer_pattern;
	u32				frame_rate;
	u32				pll_index;
	u32				mode_type;
	u32				mode_index;
	struct amba_vin_black_level_compensation current_sw_blc;
	u32				current_shutter_time;
	u32				current_gain_db;
	u16				min_agc_index;
	u16				max_agc_index;
	u16				old_shr_width_upper;
	int				current_slowshutter_mode;
	u16				xvs_num[IMX172_VIDEO_FORMAT_REG_TABLE_SIZE];
	u16				xhs_clk;
	s32				agc_call_cnt;
	u8				svr_value;
	u8				lane_num;
	u32				pixel_size;

	amba_vin_agc_info_t agc_info;
	amba_vin_shutter_info_t shutter_info;
	struct imx172_pll_reg_table pll_table;
};

/* ========================================================================== */

static const char imx172_name[] = "spidev-imx172";

static int imx172_write_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
{
	int	errCode = 0;
	u8	pbuf[4], i;
	amba_spi_cfg_t config;
	amba_spi_write_t write;

	pbuf[0] = IMX172_WRITE_ADDR;
	pbuf[1] = subaddr>>8;
	pbuf[2] = subaddr&0xFF;
	pbuf[3] = data;

	config.cfs_dfs = 8;//bits
	config.baud_rate = 3000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3;

	write.buffer = pbuf;
	write.n_size = 4;
	write.cs_id = spi_cs;
	write.bus_id = spi_bus;
	//reverse the bit, LSB first
	for(i = 0; i < write.n_size; i++){
		pbuf[i] = bitrev8(pbuf[i]);
	}
	ambarella_spi_write(&config, &write);

	return errCode;
}

static int imx172_write_reg2( struct __amba_vin_source *src, u16 subaddr, u16 data)
{
	int	errCode = 0;
	u8	pbuf[5], i;
	amba_spi_cfg_t config;
	amba_spi_write_t write;

	pbuf[0] = IMX172_WRITE_ADDR;
	pbuf[1] = subaddr >> 8;
	pbuf[2] = subaddr & 0xFF;
	pbuf[3] = data & 0xff;
	pbuf[4] = data >> 8;

	config.cfs_dfs = 8;//bits
	config.baud_rate = 3000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3;

	write.buffer = pbuf;
	write.n_size = 5;
	write.cs_id = spi_cs;
	write.bus_id = spi_bus;
	//reverse the bit, LSB first
	for(i = 0; i < write.n_size; i++){
		pbuf[i] = bitrev8(pbuf[i]);
	}
	ambarella_spi_write(&config, &write);

	return errCode;
}

static int imx172_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
{
	int				errCode = 0;
	u8	pbuf[4];
	u8	tmp, i;
	amba_spi_cfg_t config;
	amba_spi_write_then_read_t write;

	pbuf[0] = IMX172_READ_ADDR;
	pbuf[1] = subaddr>>8;
	pbuf[2] = subaddr&0xFF;

	config.cfs_dfs = 8;//bits
	config.baud_rate = 3000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3;

	write.w_buffer = pbuf;
	write.w_size = 3;
	write.r_buffer = &tmp;
	write.r_size = 1;
	write.cs_id = spi_cs;
	write.bus_id = spi_bus;

	//reverse the bit, LSB first
	for(i = 0; i < write.w_size; i++){
		pbuf[i] = bitrev8(pbuf[i]);
	}
	ambarella_spi_write_then_read(&config, &write);
	//reverse the bit, LSB first
	*pdata = bitrev8(tmp);

	return errCode;
}

/* ========================================================================== */

#include "imx172_arch_reg_tbl.c"
#include "imx172_reg_tbl.c"
#include "imx172_video_tbl.c"

static void imx172_print_info(struct __amba_vin_source *src)
{
	struct imx172_info	*pinfo;
	u32			format_index;
	pinfo = (struct imx172_info *)src->pinfo;
	format_index = imx172_video_info_table[pinfo->current_video_index].format_index;

	DRV_PRINT("format_index=%d\n", format_index);
}

/* ========================================================================== */
#include "arch/imx172_arch.c"
#include "imx172_docmd.c"
static struct imx172_info *pinfo = 0; /*memory handle */
/* ========================================================================== */
static int imx172_probe(void)
{
	int	errCode = 0;

	struct __amba_vin_source *src;

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto imx172_free_client;
	}

	//pinfo->spi = spi;
	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = IMX172_GAIN_ROWS - 1;
	pinfo->pixel_size = 0x00018CCD; /* 1.55um */

	pinfo->agc_info.db_max = 0x2D000000;	// 45dB
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
	pinfo->agc_info.db_step = 0x001AFA9A;	// 0.10538dB

	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, imx172_name, sizeof(src->name));

	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = imx172_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errCode = amba_vin_add_source(src);
	if (errCode)
		goto imx172_detach_client;
	errCode = imx172_init_hw(src);
	if (errCode)
		goto imx172_del_source;
	vin_info("IMX172 init(4/8/10-lane lvds)\n");

	goto imx172_exit;

imx172_del_source:
	amba_vin_del_source(src);
imx172_detach_client:
	//spi_set_drvdata(spi, NULL);
imx172_free_client:
	kfree(pinfo);
imx172_exit:
	return errCode;
}

static int __init imx172_init(void)
{
	return imx172_probe();
}

static void __exit imx172_exit(void)
{
	amba_vin_del_source(&pinfo->source);
	kfree((void*)pinfo);
}

module_init(imx172_init);
module_exit(imx172_exit);

MODULE_DESCRIPTION("IMX172 1/2.5 -Inch, 4096x2160, 8.97-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Cao Rongrong, <rrcao@ambarella.com>");
MODULE_LICENSE("Proprietary");
