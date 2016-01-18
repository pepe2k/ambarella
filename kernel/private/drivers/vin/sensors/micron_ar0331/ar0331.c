/*
 * Filename : ar0331.c
 *
 * History:
 *    2011/07/11 - [Haowei Lo] Create
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

#include "utils.h"
#include "ar0331_pri.h"


/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_AR0331_ADDR
	#define CONFIG_I2C_AMBARELLA_AR0331_ADDR	(0x20>>1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(ar0331, CONFIG_I2C_AMBARELLA_AR0331_ADDR, 0, 0644);

static int hdr_mode = 0;
module_param(hdr_mode, int, 0644);
MODULE_PARM_DESC(hdr_mode, "Set HDR mode 0:pure linear 1:12bits HDR(ALTM) 2:16bits HDR(ALTM bypass)");

/* ========================================================================== */
struct ar0331_info {
	struct i2c_client				client;
	struct __amba_vin_source			source;
	int						current_video_index;
	enum amba_video_mode				current_vin_mode;
	u16						cap_start_x;
	u16						cap_start_y;
	u16						cap_cap_w;
	u16						cap_cap_h;
	u16						act_start_x;
	u16						act_start_y;
	u16						act_width;
	u16						act_height;
	u16						slvs_eav_col;
	u16						slvs_sav2sav_dist;
	u8						active_capinfo_num;
	u8						bayer_pattern;
	u32						pll_index;
	u32						frame_rate;
	u32						mode_type;
	u32						mode_index;
	struct amba_vin_black_level_compensation 	current_sw_blc;
	u32						current_shutter_time;
	u32						current_gain_db;
	u16						min_agc_index;
	u16						max_agc_index;
	u8						init_flag;
	u32						dgain_r_ratio;
	u32						dgain_b_ratio;
	u16						dgain_base;

	int						current_slowshutter_mode;
	amba_vin_agc_info_t				agc_info;
	amba_vin_shutter_info_t				shutter_info;
	u8						op_mode;
};
/* ========================================================================== */

static const char ar0331_name[] = "ar0331";
static struct i2c_driver i2c_driver_ar0331;


static int ar0331_write_reg( struct __amba_vin_source *src, u16 subaddr, u16 data)
{
	int errCode = 0;
	struct ar0331_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	vin_dbg("ar0331 write reg - 0x%x, data - 0x%x\n", subaddr, data);
	pinfo = (struct ar0331_info *)src->pinfo;
	client = &pinfo->client;

	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = (data >> 8);
	pbuf[3] = (data & 0xff);

	msgs[0].len = 4;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("ar0331_write_reg(error) %d [0x%x:0x%x]\n",
			errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int ar0331_read_reg( struct __amba_vin_source *src, u16 subaddr, u16 *pdata)
{
	int	errCode = 0;
	struct ar0331_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[6];

	pinfo = (struct ar0331_info *)src->pinfo;
	client = &pinfo->client;

	pbuf0[0] = (subaddr >> 8);
	pbuf0[1] = (subaddr & 0xff);

	msgs[0].len = 2;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf0;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].buf = pbuf;
	msgs[1].len = 2;

	errCode = i2c_transfer(client->adapter, msgs, 2);
	if ((errCode != 1) && (errCode != 2)){
		DRV_PRINT("ar0331_read_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = ((pbuf[0] << 8) | pbuf[1]);
		errCode = 0;
	}

	return errCode;
}

/* ========================================================================== */

#include "ar0331_arch_reg_tbl.c"
#include "ar0331_reg_tbl.c"
#include "ar0331_video_tbl.c"

static void ar0331_print_info(struct __amba_vin_source *src)
{
#ifdef CONFIG_AMBARELLA_VIN_DEBUG
	struct ar0331_info *pinfo = (struct ar0331_info *) src->pinfo;
	u32 index = pinfo->current_video_index;
	u32 format_index = ar0331_video_info_table[index].format_index;
#endif

	vin_dbg("ar0331 video info index is:%d\n", index);
	vin_dbg("ar0331 video format index is:%d\n", format_index);

	vin_dbg("current_video_index = %d\n", pinfo->current_video_index);
	vin_dbg("x = %d, y = %d, w = %d, h = %d\n",
		pinfo->cap_start_x, pinfo->cap_start_y, pinfo->cap_cap_w, pinfo->cap_cap_h);
	vin_dbg("bayer_pattern = %d\n", pinfo->bayer_pattern);
	vin_dbg("mode_type = %d, mode_index = %d\n", pinfo->mode_type, pinfo->mode_index);
	vin_dbg("current_shutter_time = %d, current_gain_db = %d\n",
		pinfo->current_shutter_time, pinfo->current_gain_db);
	vin_dbg("current sw_blc = %d, %d, %d, %d\n",
		pinfo->current_sw_blc.bl_oo,
		pinfo->current_sw_blc.bl_oe, pinfo->current_sw_blc.bl_eo, pinfo->current_sw_blc.bl_ee);
	vin_dbg("ar0331 HDR mode or not: %d\n", hdr_mode);
}

/* ========================================================================== */
#include "arch/ar0331_arch.c"
#include "ar0331_docmd.c"
/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ar0331_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int				errCode = 0;
	struct ar0331_info		*pinfo;
	struct __amba_vin_source	*src;


	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto ar0331_exit;
	}

	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
	pinfo->min_agc_index = 0;

	switch(hdr_mode) {
	case 0:
		pinfo->op_mode = AMBA_VIN_LINEAR_MODE;
		break;
	case 2:
		pinfo->op_mode = AR0331_16BIT_HDR_MODE;
		break;
	case 1:
	default:
		pinfo->op_mode = AMBA_VIN_HDR_MODE;
		break;
	}
	/* TODO, update the db info for each sensor */
	pinfo->agc_info.db_step = 0x000C0000;	// 0.046875dB

	if (pinfo->op_mode == AMBA_VIN_LINEAR_MODE) {
		pinfo->agc_info.db_min = 0x01CC506A;	// 1.798102dB(1.23x)
		pinfo->agc_info.db_max = 0x2A000000;	// 42dB
		pinfo->max_agc_index = AR0331_LINEAR_GAIN_42DB;
	} else {
#if 1
		pinfo->agc_info.db_min = 0x01CC506A;	// 1.798102dB(1.23x)
		pinfo->agc_info.db_max = 0x1E000000;	// 30dB
		pinfo->max_agc_index = AR0331_GAIN_1_23DB;
#else
		pinfo->agc_info.db_min = 0x0224EA39;	// 2.144199dB(1.28x)
		pinfo->agc_info.db_max = 0x18000000;	// 24dB
		pinfo->max_agc_index = AR0331_HDR_GAIN_24DB;
#endif
	}

	pinfo->dgain_r_ratio = 1024;// 1x
	pinfo->dgain_b_ratio = 1024;// 1x

	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(pinfo->client));
	pinfo->client.addr = ar0331_addr;
	strlcpy(pinfo->client.name, ar0331_name, sizeof(pinfo->client.name));

	/* Vin Source */
	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, ar0331_name, sizeof(src->name));
	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = ar0331_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;
	errCode = amba_vin_add_source(src);
	if (errCode)
		goto ar0331_free_pinfo;

	errCode = ar0331_init_hw(src);
	if (errCode)
		goto ar0331_del_source;

	vin_info("AR0331 init(4-lane lvds)\n");

	goto ar0331_exit;

ar0331_del_source:
	amba_vin_del_source(src);
ar0331_free_pinfo:
	kfree(pinfo);
ar0331_exit:
	return errCode;
}

static int ar0331_remove(struct i2c_client *client)
{
	int	errCode = 0;
	struct ar0331_info *pinfo;
	struct __amba_vin_source *src;

	pinfo = (struct ar0331_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		errCode |= amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", ar0331_name);

	return errCode;
}

static struct i2c_device_id ar0331_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ar0331_idtable);

static struct i2c_driver i2c_driver_ar0331 = {
	.driver = {
		.name	= "amb_vin0",
	},

	.id_table	= ar0331_idtable,
	.probe		= ar0331_probe,
	.remove		= ar0331_remove,

};

static int __init ar0331_init(void)
{
	return i2c_add_driver(&i2c_driver_ar0331);
}

static void __exit ar0331_exit(void)
{
	i2c_del_driver(&i2c_driver_ar0331);
}

module_init(ar0331_init);
module_exit(ar0331_exit);

MODULE_DESCRIPTION("AR0331 1/3-Inch 3.1-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Haowei Lo, <hwlo@ambarella.com>");
MODULE_LICENSE("Proprietary");

