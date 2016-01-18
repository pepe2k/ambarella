/*
 * kernel/private/drivers/ambarella/vin/sensors/micron_mt9p001/mt9p001.c
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

#include <amba_common.h>

#include <dsp_cmd.h>
#include <vin_pri.h>

#include "utils.h"
#include "mt9p001_pri.h"
#include "shutter_table.h"

/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_MT9P001_ADDR
#define CONFIG_I2C_AMBARELLA_MT9P001_ADDR	(0xBA >> 1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(mt9p001, CONFIG_I2C_AMBARELLA_MT9P001_ADDR, 0, 0644);

/* ========================================================================== */
static const u8 mt9p001_sync_reg_tbl[] = {
	MT9P001_SHR_WIDTH_UPPER,
	MT9P001_SHR_WIDTH,
	MT9P001_GREEN1_GAIN,
	MT9P001_BLUE_GAIN,
	MT9P001_RED_GAIN,
	MT9P001_GREEN2_GAIN,
	MT9P001_GLOBAL_GAIN
};

#define MT9P001_SYNC_REG_TBL_SZIE		ARRAY_SIZE(mt9p001_sync_reg_tbl)

struct mt9p001_info {
	struct i2c_client client;
	struct __amba_vin_source source;
	int current_video_index;
	u16 cap_start_x;
	u16 cap_start_y;
	u16 cap_cap_w;
	u16 cap_cap_h;
	u16 act_start_x;
	u16 act_start_y;
	u16 act_width;
	u16 act_height;
	u8 active_capinfo_num;
	u8 bayer_pattern;
	u32 fps_index;
	u32 frame_rate;
	u16 old_shr_width_upper;
	u32 mode_type;
	u32 mode_index;
	struct amba_vin_black_level_compensation current_sw_blc;
	u32 current_shutter_time;
	s32 current_gain_db;
	u16 read_mode_1;
	u16 restart;
	enum amba_vin_trigger_source trigger_source;
	enum amba_vin_flash_level flash_level;
	enum amba_vin_flash_status flash_status;
	enum amba_vin_capture_mode capture_mode;
	enum amba_video_mode current_vin_mode;

	struct mutex sync_reg_lock;
	struct mt9p001_sync_reg sync_reg_data[MT9P001_SYNC_REG_TBL_SZIE];
	amba_vin_agc_info_t agc_info;
	amba_vin_shutter_info_t shutter_info;
	u32 need_sync;

	int current_low_light_mode;
	int current_slowshutter_mode;
};

/* ========================================================================== */
static const struct mt9p001_pll_reg_table mt9p001_pll_tbl[];

#if defined(CONFIG_SENSOR_MT9P031)
static const char mt9p001_name[] = "mt9p031";
#elif defined(CONFIG_SENSOR_MT9P401)
static const char mt9p001_name[] = "mt9p401";
#else
static const char mt9p001_name[] = "mt9p001";
#endif
static struct i2c_driver i2c_driver_mt9p001;

static int mt9p001_write_reg(struct __amba_vin_source *src, u8 subaddr, u16 data)
{
	int errorCode = 0;
	struct mt9p001_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	pinfo = (struct mt9p001_info *) src->pinfo;
	client = &pinfo->client;

	pbuf[0] = subaddr;
	pbuf[1] = (data >> 8);
	pbuf[2] = (data & 0xFF);

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 3;
	msgs[0].buf = pbuf;

	errorCode = i2c_transfer(client->adapter, msgs, 1);
	if (errorCode != 1) {
		vin_err("mt9p001_write_reg %d [0x%x:0x%x]\n", errorCode, subaddr, data);
		errorCode = -EIO;
	} else {
		errorCode = 0;
	}

	return errorCode;
}

static int mt9p001_read_reg(struct __amba_vin_source *src, u8 subaddr, u16 * pdata)
{
	int errorCode = 0;
	struct mt9p001_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf[2];

	pinfo = (struct mt9p001_info *) src->pinfo;
	client = &pinfo->client;

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 1;
	msgs[0].buf = &subaddr;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].len = 2;
	msgs[1].buf = pbuf;

	errorCode = i2c_transfer(client->adapter, msgs, 2);
	if (errorCode != 2) {
		vin_err("mt9p001_read_reg %d [0x%x]\n", errorCode, subaddr);
		errorCode = -EIO;
	} else {
		*pdata = pbuf[0];
		*pdata <<= 8;
		*pdata |= pbuf[1];
		errorCode = 0;
	}

	return errorCode;
}

static void mt9p001_get_bin_skip(struct __amba_vin_source *src,
				 u8 * column_bin, u8 * row_bin, u8 * column_skip, u8 * row_skip)
{
	int errorCode = 0;
	u16 data;

	errorCode = mt9p001_read_reg(src, MT9P001_ROW_ADDR_MODE, &data);
	if (!errorCode) {
		*row_skip = (data >> 0) & 0x000F;
		*row_bin = (data >> 4) & 0x000F;
	}

	errorCode = mt9p001_read_reg(src, MT9P001_COL_ADDR_MODE, &data);
	if (!errorCode) {
		*column_skip = (data >> 0) & 0x000F;
		*column_bin = (data >> 4) & 0x000F;
	}
}

/* ========================================================================== */
#if defined(CONFIG_SENSOR_MT9P031)
#include "mt9p031_reg_tbl.c"
#elif defined(CONFIG_SENSOR_MT9P401)
#include "mt9p401_reg_tbl.c"
#else
#include "mt9p001_reg_tbl.c"
#endif
#include "mt9pxxx_reg_tbl.c"
#include "mt9p001_video_tbl.c"

static void mt9p001_print_info(struct __amba_vin_source *src)
{
	struct mt9p001_info *pinfo;
	u32 index;
	u32 format_index;

	pinfo = (struct mt9p001_info *) src->pinfo;
	index = pinfo->current_video_index;
	format_index = mt9p001_video_info_table[index].format_index;

	vin_dbg("mt9p001 video info index is:%d\n", index);
	vin_dbg("mt9p001 video format index is:%d\n", format_index);

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
	vin_dbg("current_vin_mode = %d, read_mode_1 = 0x%x, restart = 0x%x\n",
		pinfo->current_vin_mode, pinfo->read_mode_1, pinfo->restart);
	vin_dbg("capture_mode = %d, flash_level = %d, flash_status = %d\n",
		pinfo->capture_mode, pinfo->flash_level, pinfo->flash_status);
	vin_dbg("current_low_light_mode = %d\n", pinfo->current_low_light_mode);
}

/* ========================================================================== */
#include "arch/mt9p001_arch.c"
#include "mt9p001_docmd.c"

/* ========================================================================== */
static int mt9p001_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int				errorCode = 0;
	u16				sen_id, sen_ver;
	struct mt9p001_info		*pinfo;
	struct __amba_vin_source	*src;

	/* Platform Info */
	pinfo = kzalloc(sizeof (*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errorCode = -ENOMEM;
		goto mt9p001_probe_exit;
	}
	pinfo->current_video_index = -1;
	pinfo->fps_index = 0;
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
	mutex_init(&pinfo->sync_reg_lock);
	pinfo->agc_info.db_max = 0x2a000000;	// 42dB
	pinfo->agc_info.db_min = 0xfa000000;	// -6dB
	pinfo->agc_info.db_step = 0x00300000;	// 0.1875dB

	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(pinfo->client));
	pinfo->client.addr = mt9p001_addr;
	strlcpy(pinfo->client.name, mt9p001_name, sizeof(pinfo->client.name));

	/* Vin Source */
	src = &pinfo->source;
	src->id = -1;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, mt9p001_name, sizeof (src->name));
	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = mt9p001_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;
	errorCode = amba_vin_add_source(src);
	if (errorCode)
		goto mt9p001_free_pinfo;

	/* Hardware Initialization */
	errorCode = mt9p001_init_vin_clock(src, 0);
	if (errorCode)
		goto mt9p001_del_src;
	msleep(10);
	mt9p001_reset(src);

	errorCode = mt9p001_query_snesor_id(src, &sen_id);
	if (errorCode)
		goto mt9p001_del_src;
	if ((sen_id >> 8) != MT9P001_CHIP_VER_PART_ID) {
		vin_err("MT9P001_CHIP_ID[0x%x] is wrong!\n", sen_id);
		errorCode = -ENXIO;
		goto mt9p001_del_src;
	}
	errorCode = mt9p001_query_snesor_version(src, &sen_ver);
	if (errorCode)
		goto mt9p001_del_src;
	vin_notice("%s:%d ver#%d probed!\n", src->name, src->id, sen_ver);

	vin_info("MT9P001 init(parallel)\n");

	goto mt9p001_probe_exit;

mt9p001_del_src:
	amba_vin_del_source(src);

mt9p001_free_pinfo:
	kfree(pinfo);

mt9p001_probe_exit:
	return errorCode;
}

static int mt9p001_remove(struct i2c_client *client)
{
	int				errorCode = 0;
	struct mt9p001_info		*pinfo;
	struct __amba_vin_source	*src;

	pinfo = (struct mt9p001_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		errorCode = amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", mt9p001_name);

	return errorCode;
}

static struct i2c_device_id mt9p001_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mt9p001_idtable);

static struct i2c_driver i2c_driver_mt9p001 = {
	.driver = {
		   .name = "amb_vin0",
		   },
	.id_table	= mt9p001_idtable,
	.probe		= mt9p001_probe,
	.remove		= mt9p001_remove,
};

static int __init mt9p001_init(void)
{
	return i2c_add_driver(&i2c_driver_mt9p001);
}

static void __exit mt9p001_exit(void)
{
	i2c_del_driver(&i2c_driver_mt9p001);
}

module_init(mt9p001_init);
module_exit(mt9p001_exit);

MODULE_DESCRIPTION("MT9P001 1/2.5-Inch 5-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("Proprietary");
