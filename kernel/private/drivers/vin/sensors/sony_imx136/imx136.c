/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx136/imx136.c
 *
 * History:
 *    2012/02/21 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
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
#include "imx136_pri.h"
#include "shutter_table.h"

/* ========================================================================== */

AMBA_VIN_BASIC_PARAM_CALL(imx136, 0, 0, 0644);

static int spi_bus = 0;
module_param(spi_bus, int, 0644);
MODULE_PARM_DESC(spi_bus, "SPI controller ID");
static int spi_cs = 0;
module_param(spi_cs, int, 0644);
MODULE_PARM_DESC(spi_cs, "SPI CS");
/* ========================================================================== */
struct imx136_info {
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
	u16 				min_agc_index;
	u16 				max_agc_index;
	u16				old_shr_width_upper;
	int				current_slowshutter_mode;

	amba_vin_agc_info_t agc_info;
	amba_vin_shutter_info_t shutter_info;
};

/* ========================================================================== */

static const char imx136_name[] = "spidev-imx136";

static int imx136_write_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
{
	int	errCode = 0;
	u8	pbuf[3], i;
	amba_spi_cfg_t config;
	amba_spi_write_t write;

	pbuf[0] = subaddr>>0x08;;
	pbuf[1] = subaddr&0xFF;
	pbuf[2] = data;

	config.cfs_dfs = 8;//bits
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3;

	write.buffer = pbuf;
	write.n_size = 3;
	write.cs_id = spi_cs;
	write.bus_id = spi_bus;
	//reverse the bit, LSB first
	for(i = 0; i < write.n_size; i++){
		pbuf[i] = bitrev8(pbuf[i]);
	}
	ambarella_spi_write(&config, &write);

	return errCode;
}

static int imx136_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
{
	int				errCode = 0;
	u8	pbuf[3];
	u8	tmp, i;
	amba_spi_cfg_t config;
	amba_spi_write_then_read_t write;

	pbuf[0] = (subaddr>>8)|0x80;
	pbuf[1] = subaddr&0xFF;

	config.cfs_dfs = 8;//bits
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3;

	write.w_buffer = pbuf;
	write.w_size = 2;
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

#include "imx136_arch_reg_tbl.c"
#include "imx136_reg_tbl.c"
#include "imx136_video_tbl.c"

static void imx136_print_info(struct __amba_vin_source *src){}

/* ========================================================================== */
#include "arch/imx136_arch.c"
#include "imx136_docmd.c"
static struct imx136_info *pinfo = 0; /*memory handle */
/* ========================================================================== */
static int imx136_probe(void)
{
	int	errCode = 0;

	struct __amba_vin_source *src;

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto imx136_free_client;
	}

	//pinfo->spi = spi;
	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = IMX136_GAIN_ROWS - 1;

	pinfo->agc_info.db_max = 0x2A000000;	// 42dB
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
	pinfo->agc_info.db_step = 0x00199999;	// 0.1dB

	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, imx136_name, sizeof(src->name));

	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = imx136_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errCode = amba_vin_add_source(src);
	if (errCode)
		goto imx136_detach_client;
	errCode = imx136_init_hw(src);
	if (errCode)
		goto imx136_del_source;
	vin_info("IMX136 init(4-lane lvds)\n");

	goto imx136_exit;

imx136_del_source:
	amba_vin_del_source(src);
imx136_detach_client:
	//spi_set_drvdata(spi, NULL);
imx136_free_client:
	kfree(pinfo);
imx136_exit:
	return errCode;
}

static int __init imx136_init(void)
{
	return imx136_probe();
}

static void __exit imx136_exit(void)
{
	amba_vin_del_source(&pinfo->source);
	kfree((void*)pinfo);
}

module_init(imx136_init);
module_exit(imx136_exit);

MODULE_DESCRIPTION("IMX136 1/2.8 -Inch, 1920x1200, 2.38-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

