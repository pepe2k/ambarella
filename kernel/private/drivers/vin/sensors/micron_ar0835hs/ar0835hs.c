/*
 * Filename : ar0835hs.c
 *
 * History:
 *    2012/12/26 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
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
#include "ar0835hs_pri.h"
/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_AR0835HS_ADDR
	#define CONFIG_I2C_AMBARELLA_AR0835HS_ADDR	(0x6C>> 1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(ar0835hs, CONFIG_I2C_AMBARELLA_AR0835HS_ADDR, 0, 0644);
/* ========================================================================== */
struct ar0835hs_info {
	struct i2c_client		client;
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
	u8				mirror_pattern;
	u8				reserved;
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
	u8				init_flag;

	amba_vin_agc_info_t agc_info;
	amba_vin_shutter_info_t shutter_info;
};

/* ========================================================================== */

static const char ar0835hs_name[] = "ar0835hs";
static struct i2c_driver i2c_driver_ar0835hs;

static int ar0835hs_write_reg(struct __amba_vin_source *src, u16 subaddr, u16 data)
{
	int errCode = 0;
	struct ar0835hs_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	pinfo = (struct ar0835hs_info *) src->pinfo;
	client = &pinfo->client;

	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = (data >> 8);
	pbuf[3] = (data & 0xff);
	vin_dbg("reg:0x%x, value:0x%x\n", subaddr, data);
	//DRV_PRINT("reg:0x%x, value:0x%x\n", subaddr, data);

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 4;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("ar0835hs_write_reg(error) %d [0x%x:0x%x]\n", errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int ar0835hs_read_reg(struct __amba_vin_source *src, u16 subaddr, u16 * pdata)
{
	int errCode = 0;
	struct ar0835hs_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[6];

	pinfo = (struct ar0835hs_info *) src->pinfo;
	client = &pinfo->client;

	pbuf0[1] = (subaddr & 0xff);
	pbuf0[0] = (subaddr >> 8);
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 2;
	msgs[0].buf = pbuf0;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].len = 2;
	msgs[1].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 2);
	if ((errCode != 1) && (errCode != 2)) {
		DRV_PRINT("ar0835hs_read_reg(error) %d [0x%x]\n", errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = ((pbuf[0] << 8) | pbuf[1]);
		errCode = 0;
	}

	return errCode;
}

static int ar0835hs_writeb_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
{
	int	errCode = 0;
	struct ar0835hs_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf0[3];

	pinfo = (struct ar0835hs_info *)src->pinfo;
	client = &pinfo->client;

	pbuf0[0] = (subaddr >> 8);
	pbuf0[1] = (subaddr & 0xff);
	pbuf0[2] = data;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf0;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1){
		DRV_PRINT("ar0835hs_writeb_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

/* ========================================================================== */

#include "ar0835hs_arch_reg_tbl.c"
#include "ar0835hs_reg_tbl.c"
#include "ar0835hs_video_tbl.c"

static void ar0835hs_print_info(struct __amba_vin_source *src)
{

}

/* ========================================================================== */
#include "arch/ar0835hs_arch.c"
#include "ar0835hs_docmd.c"
/* 	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ar0835hs_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int					errCode = 0;
	struct ar0835hs_info			*pinfo;
	struct __amba_vin_source		*src;

	/* Platform Info */
	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto ar0835hs_probe_exit;
	}
	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = AR0835HS_GAIN_ROWS - 1;

	pinfo->agc_info.db_max = 0x2A000000;	// 42dB
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
	pinfo->agc_info.db_step = 0x00180000;	// 0.09375dB

	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(pinfo->client));
	pinfo->client.addr = ar0835hs_addr;
	strlcpy(pinfo->client.name, ar0835hs_name, sizeof(pinfo->client.name));

	/* Vin Source */
	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, ar0835hs_name, sizeof(src->name));
	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = ar0835hs_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;
	errCode = amba_vin_add_source(src);
	if (errCode)
		goto ar0835hs_free_pinfo;

	errCode = ar0835hs_init_hw(src);
	if (errCode)
		goto ar0835hs_del_src;

	vin_info("AR0835HS init(4-lane lvds)\n");

	goto ar0835hs_probe_exit;

ar0835hs_del_src:
	amba_vin_del_source(src);

ar0835hs_free_pinfo:
	kfree(pinfo);

ar0835hs_probe_exit:
	return errCode;
}

static int ar0835hs_remove(struct i2c_client *client)
{
	int				errCode = 0;
	struct ar0835hs_info		*pinfo;
	struct __amba_vin_source	*src;

	pinfo = (struct ar0835hs_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		errCode = amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", ar0835hs_name);

	return errCode;
}

static struct i2c_device_id ar0835hs_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ar0835hs_idtable);

static struct i2c_driver i2c_driver_ar0835hs = {
	.driver = {
		   .name = "amb_vin0",
		   },
	.id_table	= ar0835hs_idtable,
	.probe		= ar0835hs_probe,
	.remove		= ar0835hs_remove,
};

static int __init ar0835hs_init(void)
{
	return i2c_add_driver(&i2c_driver_ar0835hs);
}

static void __exit ar0835hs_exit(void)
{
	i2c_del_driver(&i2c_driver_ar0835hs);
}

module_init(ar0835hs_init);
module_exit(ar0835hs_exit);

MODULE_DESCRIPTION("AR0835HS 1/3.2-Inch 8-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

