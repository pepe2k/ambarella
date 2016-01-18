/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx104/imx104.c
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
#include "imx104_pri.h"
#include "shutter_table.h"

/* ========================================================================== */
AMBA_VIN_BASIC_PARAM_CALL(imx104, 0, 0, 0644);

static int spi_bus = 0;
module_param(spi_bus, int, 0644);
MODULE_PARM_DESC(spi_bus, "SPI controller ID");
static int spi_cs = 0;
module_param(spi_cs, int, 0644);
MODULE_PARM_DESC(spi_cs, "SPI CS");
static int wdr_mode = 0;
module_param(wdr_mode, int, 0644);
MODULE_PARM_DESC(wdr_mode, "WDR mode, 0:off 1:on");
static int fps120_mode = 0;
module_param(fps120_mode, int, 0644);
MODULE_PARM_DESC(fps120_mode, "120fps mode , 0:off 1:on");
static int llp_mode = 0;
module_param(llp_mode, int, 0644);
MODULE_PARM_DESC(llp_mode, "Set mode 0:HLP mode(high light) 1:LLP mode(low light)");
/* ========================================================================== */
struct imx104_info {
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
	u32 agc_call_cnt;
};

/* ========================================================================== */

static const char imx104_name[] = "spidev-imx104";

static int imx104_write_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
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

static int imx104_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
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

#include "imx104_arch_reg_tbl.c"
#include "imx104_reg_tbl.c"
#include "imx104_video_tbl.c"

static void imx104_print_info(struct __amba_vin_source *src){}

/* ========================================================================== */
#include "arch/imx104_arch.c"
#include "imx104_docmd.c"
static struct imx104_info *pinfo = 0; /*memory handle */
/* ========================================================================== */
static int imx104_probe(void)
{
	int	errCode = 0;

	struct __amba_vin_source *src;

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto imx104_free_client;
	}

	//pinfo->spi = spi;
	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
	pinfo->min_agc_index = 0;

	if(wdr_mode) {
		pinfo->max_agc_index = IMX104_WDR_GAIN_ROWS - 1;
		pinfo->agc_info.db_max = 0x0C000000;	// 12dB
	} else {
		pinfo->max_agc_index = IMX104_GAIN_ROWS - 1;
		pinfo->agc_info.db_max = 0x2A000000;	// 42dB
	}
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
	pinfo->agc_info.db_step = 0x004CCCCC;	// 0.3dB

	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, imx104_name, sizeof(src->name));

	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = imx104_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errCode = amba_vin_add_source(src);
	if (errCode)
		goto imx104_detach_client;
	errCode = imx104_init_hw(src);
	if (errCode)
		goto imx104_del_source;
	vin_info("IMX104 init(4-lane lvds)\n");

	goto imx104_exit;

imx104_del_source:
	amba_vin_del_source(src);
imx104_detach_client:
	//spi_set_drvdata(spi, NULL);
imx104_free_client:
	kfree(pinfo);
imx104_exit:
	return errCode;
}

static int __init imx104_init(void)
{
	return imx104_probe();
}

static void __exit imx104_exit(void)
{
	amba_vin_del_source(&pinfo->source);
	kfree((void*)pinfo);
}

module_init(imx104_init);
module_exit(imx104_exit);

MODULE_DESCRIPTION("IMX104 1/3 -Inch, 1280x1024, 1.31-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

