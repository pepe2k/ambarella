/*
 * Filename : mn34041pl.c
 *
 * History:
 *    2011/01/12 - [Haowei Lo] Create
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

#include <amba_common.h>

#include <dsp_cmd.h>
#include <vin_pri.h>
#include <linux/bitrev.h>

#include "utils.h"
#include "mn34041pl_pri.h"


/* ========================================================================== */
//SPI

AMBA_VIN_BASIC_PARAM_CALL(mn34041pl, 0, 0, 0644);

static int spi_bus = 0;
module_param(spi_bus, int, 0644);
MODULE_PARM_DESC(spi_bus, "SPI controller ID");
static int spi_cs = 0;
module_param(spi_cs, int, 0644);
MODULE_PARM_DESC(spi_cs, "SPI CS");

static int sync_mode = 1;	//1 : slave mode, 2 : master mode 3 : slave + master mode
module_param(sync_mode, int, 0644);
MODULE_PARM_DESC(sync_mode, "Sensor sync mode");

static int act_lanes = 2;	//1 : 2lanes, 2 : 4 lanes 3 : 8 lanes
module_param(act_lanes, int, 0644);
MODULE_PARM_DESC(act_lanes, "Active lanes");

static int fps120_mode = 0;
module_param(fps120_mode, int, 0644);
MODULE_PARM_DESC(fps120_mode, "120fps mode , 0:off 1:on");
/* ========================================================================== */
struct mn34041pl_info {

	struct spi_device 		*spi;
	struct __amba_vin_source	source;
	int				current_video_index;
	enum amba_video_mode		current_vin_mode;
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
	u32				pll_index;
	u32				frame_rate;
	u32				mode_type;
	u32				mode_index;
	struct amba_vin_black_level_compensation current_sw_blc;
	u32				current_shutter_time;
	u32				current_gain_db;
	u16				min_agc_index;
	u16				max_agc_index;

	int				current_slowshutter_mode;
	amba_vin_agc_info_t		agc_info;
	amba_vin_shutter_info_t		shutter_info;
};
/* ========================================================================== */

static const char mn34041pl_name[] = "mn34041pl";

static int mn34041pl_write_reg( struct __amba_vin_source *src, u16 subaddr, u16 data)
{
	int	errCode = 0;
	u8	pbuf[4], i;
	//struct imx036_info	*pinfo = (struct imx036_info *)src->pinfo;
	amba_spi_cfg_t config;
	amba_spi_write_t write;

	pbuf[0] = subaddr&0xFF;
	pbuf[1] = subaddr>>0x08;
	pbuf[2] = data&0xFF;
	pbuf[3] = data>>0x08;

	config.cfs_dfs = 16;//bits
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3;

	write.buffer = pbuf;
	write.n_size = 4;
	write.cs_id = spi_cs;
	write.bus_id = spi_bus;
	//reverse the bit, LSB first
	for(i = 0; i < write.n_size/2; i++){
		((u16 *)pbuf)[i] = bitrev16(((u16 *)pbuf)[i]);
	}
	ambarella_spi_write(&config, &write);

	return errCode;
}

static int mn34041pl_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
{
	//SPI
	int				errCode = 0;
	u8	pbuf[2];
	u8	tmp[2], i;
	amba_spi_cfg_t config;
	amba_spi_write_then_read_t write;

	pbuf[0] = subaddr&0xFF;
	pbuf[1] = (subaddr>>8)|0x80;

	config.cfs_dfs = 16;//bits
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3;

	write.w_buffer = pbuf;
	write.w_size = 2;
	write.r_buffer = tmp;
	write.r_size = 2;
	write.cs_id = spi_cs;
	write.bus_id = spi_bus;

	//reverse the bit, LSB first
	for(i = 0; i < write.w_size/2; i++){
		((u16 *)pbuf)[i] = bitrev16(((u16 *)pbuf)[i]);
	}
	ambarella_spi_write_then_read(&config, &write);
	//reverse the bit, LSB first
	for(i = 0; i < write.r_size/2; i++){
		((u16 *)pdata)[i] = bitrev16(((u16 *)tmp)[i]);
	}

	return errCode;
}


#if 0
/*
 *	Write any register field
 *	Para:
 *   	@reg, register
 *		@bitfild, mask
 *      @value, value to set
 *	<registername>, [<bitfieldname>,] <value>
 */
static int mn34041pl_write_reg_field(struct __amba_vin_source *src, u16 add, u16 bitfield, u16 value)
{
	int errCode = 0;
	u16 regdata;
	int mask_bit_start = 0;
	errCode = mn34041pl_read_reg(src, add, &regdata);

	if (errCode != 0)
		return -1;
	regdata &= (~bitfield);
	while ((bitfield & 0x01) == 0) {
		mask_bit_start++;
		bitfield >>= 0x01;
	}
	value <<= mask_bit_start;
	regdata |= value;
	vin_dbg("regdata = 0x%04x, value_byshift = 0x%04x, mask_bit_start =  %04d\n", regdata, value,mask_bit_start);
	errCode = mn34041pl_write_reg(src, add, regdata);

	return errCode;
}
#endif
/* ========================================================================== */

#include "mn34041pl_arch_reg_tbl.c"
#include "mn34041pl_reg_tbl.c"
#include "mn34041pl_video_tbl.c"

static void mn34041pl_print_info(struct __amba_vin_source *src)
{
#if 0
#ifdef CONFIG_AMBARELLA_VIN_DEBUG
	struct mn34041pl_info *pinfo = (struct mn34041pl_info *) src->pinfo;
	u32 index = pinfo->current_video_index;
	u32 format_index = mn34041pl_video_info_table[index].format_index;
#endif

	vin_dbg("mn34041pl video info index is:%d\n", index);
	vin_dbg("mn34041pl video format index is:%d\n", format_index);

	vin_dbg("current_video_index = %d\n", pinfo->current_video_index);
	vin_dbg("x = %d, y = %d, w = %d, h = %d\n",
		pinfo->cap_start_x, pinfo->cap_start_y, pinfo->cap_cap_w, pinfo->cap_cap_h);
	vin_dbg("bayer_pattern = %d, fps_index = %d\n", pinfo->bayer_pattern, pinfo->fps_index);
	vin_dbg("mode_type = %d, mode_index = %d\n", pinfo->mode_type, pinfo->mode_index);
	vin_dbg("current_shutter_time = %d, current_gain_db = %d\n",
		pinfo->current_shutter_time, pinfo->current_gain_db);
	vin_dbg("current sw_blc = %d, %d, %d, %d\n",
		pinfo->current_sw_blc.bl_oo,
		pinfo->current_sw_blc.bl_oe, pinfo->current_sw_blc.bl_eo, pinfo->current_sw_blc.bl_ee);
#else
	//struct mn34041pl_info			*pinfo = (struct mn34041pl_info *)src->pinfo;

	vin_dbg("sync_mode = 0x%x\n", sync_mode);
	vin_dbg("act_lanes = 0x%x\n", act_lanes);
#endif
}

/* ========================================================================== */
struct mn34041pl_info		*pinfo;
#include "arch/mn34041pl_arch.c"
#include "mn34041pl_docmd.c"
/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */

static int mn34041pl_probe(void)
{
	int				errCode = 0;
	struct __amba_vin_source	*src;

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto mn34041pl_free_pinfo;
	}

	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = MN34041PL_GAIN_ROWS - 1;
	/* TODO, update the db info for each sensor */
	pinfo->agc_info.db_max = 0x2a000000;	// 42dB		//change from old 30db(datasheet) to 42db and checked with Panasonic FAE
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
	pinfo->agc_info.db_step = 0x00180000;	// 0.09375dB

	/* Vin Source */
	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, mn34041pl_name, sizeof(src->name));
	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = mn34041pl_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;
	errCode = amba_vin_add_source(src);
	if (errCode)
		goto mn34041pl_free_pinfo;
	errCode = mn34041pl_init_hw(src);
	if (errCode)
		goto mn34041pl_del_source;

	if(fps120_mode) {
		vin_info("MN34041PL init(6-lane lvds)\n");
	} else {
		vin_info("MN34041PL init(4-lane lvds)\n");
	}

	goto mn34041pl_exit;

mn34041pl_del_source:
	amba_vin_del_source(src);
mn34041pl_free_pinfo:
	kfree(pinfo);
mn34041pl_exit:
	return errCode;
}

static int __init mn34041pl_init(void)
{
	return mn34041pl_probe();
}

static void __exit mn34041pl_exit(void)
{
	amba_vin_del_source(&pinfo->source);
	kfree((void*)pinfo);
}

module_init(mn34041pl_init);
module_exit(mn34041pl_exit);

MODULE_DESCRIPTION("MN34041PL 1/3-Inch 2.1-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Haowei Lo, <hwlo@ambarella.com>");
MODULE_LICENSE("Proprietary");
