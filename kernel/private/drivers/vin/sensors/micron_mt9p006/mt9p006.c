/*
 * Filename : mt9p006.c
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

#include <amba_common.h>

#include <dsp_cmd.h>
#include <vin_pri.h>

#include "utils.h"
#include "mt9p006_pri.h"


/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_MT9P006_ADDR

/* IDC slave type3: 8-bit addr,  8-bit subaddr/regaddr, 16-bit data */
#define MT9P006_SADDR_LVL               (1)       /* 1 High, 0 Low */
#define CONFIG_I2C_AMBARELLA_MT9P006_ADDR	((MT9P006_SADDR_LVL == 1) ? ((0XBA)>>1) : ((0X90))>>1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(mt9p006, CONFIG_I2C_AMBARELLA_MT9P006_ADDR, 0, 0644);

/* ========================================================================== */
struct mt9p006_info {
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
	u8						active_capinfo_num;
	u8						bayer_pattern;
	u8						downsample;
	u32						pll_index;
	u32						frame_rate;
	u32						mode_type;
	u32						mode_index;
	struct amba_vin_black_level_compensation 	current_sw_blc;
	u32						current_shutter_time;
	u32						current_gain_db;
	u16						min_agc_index;
	u16						max_agc_index;

	int						current_slowshutter_mode;
	amba_vin_agc_info_t				agc_info;
	amba_vin_shutter_info_t				shutter_info;
	struct mt9p006_pll_reg_table pll_table;
};
/* ========================================================================== */

static const char mt9p006_name[] = "mt9p006";
static struct i2c_driver i2c_driver_mt9p006;


static int mt9p006_write_reg( struct __amba_vin_source *src, u8 subaddr, u16 data)
{
	int errCode = 0;
	struct mt9p006_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	vin_dbg("mt9p006 write reg - 0x%x, data - 0x%x\n", subaddr, data);
	pinfo = (struct mt9p006_info *)src->pinfo;
	client = &pinfo->client;

	pbuf[0] = subaddr;
	pbuf[1] = (data >> 8);
	pbuf[2] = (data & 0xff);

	msgs[0].len = 3;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("mt9p006_write_reg(error) %d [0x%x:0x%x]\n",
			errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int mt9p006_read_reg( struct __amba_vin_source *src, u8 subaddr, u16 *pdata)
{
	int	errCode = 0;
	struct mt9p006_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf[2];

	pinfo = (struct mt9p006_info *)src->pinfo;
	client = &pinfo->client;

	msgs[0].len = 1;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = &subaddr;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].buf = pbuf;
	msgs[1].len = 2;

	errCode = i2c_transfer(client->adapter, msgs, 2);
	if ((errCode != 1) && (errCode != 2)){
		DRV_PRINT("mt9p006_read_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = ((pbuf[0] << 8) | pbuf[1]);
		errCode = 0;
	}

	return errCode;
}

/* ========================================================================== */

#include "mt9p006_arch_reg_tbl.c"
#include "mt9p006_reg_tbl.c"
#include "mt9p006_video_tbl.c"

static void mt9p006_print_info(struct __amba_vin_source *src)
{
#ifdef CONFIG_AMBARELLA_VIN_DEBUG
	struct mt9p006_info *pinfo = (struct mt9p006_info *) src->pinfo;
	u32 index = pinfo->current_video_index;
	u32 format_index = mt9p006_video_info_table[index].format_index;
#endif

	vin_dbg("mt9p006 video info index is:%d\n", index);
	vin_dbg("mt9p006 video format index is:%d\n", format_index);

	vin_dbg("current_video_index = %d\n", pinfo->current_video_index);
	vin_dbg("x = %d, y = %d, w = %d, h = %d\n",
		pinfo->cap_start_x, pinfo->cap_start_y, pinfo->cap_cap_w, pinfo->cap_cap_h);
	vin_dbg("bayer_pattern = %d, pll_index = %d\n", pinfo->bayer_pattern, pinfo->pll_index);
	vin_dbg("mode_type = %d, mode_index = %d\n", pinfo->mode_type, pinfo->mode_index);
	vin_dbg("current_shutter_time = %d, current_gain_db = %d\n",
		pinfo->current_shutter_time, pinfo->current_gain_db);
	vin_dbg("current sw_blc = %d, %d, %d, %d\n",
		pinfo->current_sw_blc.bl_oo,
		pinfo->current_sw_blc.bl_oe, pinfo->current_sw_blc.bl_eo, pinfo->current_sw_blc.bl_ee);
}

/* ========================================================================== */
#include "arch/mt9p006_arch.c"
#include "mt9p006_docmd.c"
/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int mt9p006_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int				errCode = 0;
	struct mt9p006_info		*pinfo;
	struct __amba_vin_source	*src;


	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto mt9p006_exit;
	}

	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = MT9P006_GAIN_ROWS - 1;
	/* TODO, update the db info for each sensor */
	pinfo->agc_info.db_max = 0x2a000000;	// 42dB
	pinfo->agc_info.db_min = 0xfc000000;	// -4dB
	pinfo->agc_info.db_step = 0x00304325;	// 0.1885245dB

	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(pinfo->client));
	pinfo->client.addr = mt9p006_addr;
	strlcpy(pinfo->client.name, mt9p006_name, sizeof(pinfo->client.name));

	/* Vin Source */
	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, mt9p006_name, sizeof(src->name));
	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = mt9p006_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;
	errCode = amba_vin_add_source(src);
	if (errCode)
		goto mt9p006_free_pinfo;

	errCode = mt9p006_init_hw(src);
	if (errCode)
		goto mt9p006_del_source;

	vin_info("MT9P006 init(parallel)\n");

	goto mt9p006_exit;

mt9p006_del_source:
	amba_vin_del_source(src);
mt9p006_free_pinfo:
	kfree(pinfo);
mt9p006_exit:
	return errCode;
}

static int mt9p006_remove(struct i2c_client *client)
{
	int	errCode = 0;
	struct mt9p006_info *pinfo;
	struct __amba_vin_source *src;

	pinfo = (struct mt9p006_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		mt9p006_docmd(src, AMBA_VIN_SRC_SUSPEND, NULL);
		errCode |= amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", mt9p006_name);

	return errCode;
}

static struct i2c_device_id mt9p006_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mt9p006_idtable);

static struct i2c_driver i2c_driver_mt9p006 = {
	.driver = {
		.name	= "amb_vin0",
	},

	.id_table	= mt9p006_idtable,
	.probe		= mt9p006_probe,
	.remove		= mt9p006_remove,

};

static int __init mt9p006_init(void)
{
	return i2c_add_driver(&i2c_driver_mt9p006);
}

static void __exit mt9p006_exit(void)
{
	i2c_del_driver(&i2c_driver_mt9p006);
}

module_init(mt9p006_init);
module_exit(mt9p006_exit);

MODULE_DESCRIPTION("MT9P006 1/3.2-Inch 3.4-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Haowei Lo, <hwlo@ambarella.com>");
MODULE_LICENSE("Proprietary");

