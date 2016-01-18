/*
 * Filename : ov2710.c
 *
 * History:
 *    2009/06/19 - [Qiao Wang] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
#include "ov2710_pri.h"

/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_OV2710_ADDR
	#define CONFIG_I2C_AMBARELLA_OV2710_ADDR	(0x6C>>1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(ov2710, CONFIG_I2C_AMBARELLA_OV2710_ADDR, 0, 0644);

/* ========================================================================== */
struct ov2710_info {
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
	u8				active_capinfo_num;
	u8				bayer_pattern;
	u32				frame_rate;
	int				pll_index;
	u32				mode_type;
	u32				mode_index;
	struct amba_vin_black_level_compensation current_sw_blc;
	u32				current_shutter_time;
	u32				current_gain_db;
	u16 				min_agc_index;
	u16 				max_agc_index;
	u16				old_shr_width_upper;
	u32 				line_length;
	u32				frame_length_lines;

	amba_vin_agc_info_t 		agc_info;
	amba_vin_shutter_info_t 	shutter_info;
};

/* ========================================================================== */

static const char ov2710_name[] = "ov2710";
static struct i2c_driver i2c_driver_ov2710;


static int ov2710_write_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
{
	int	errCode = 0;
	struct ov2710_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	vin_dbg("ov2710 write reg - 0x%x, data - 0x%x\n", subaddr, data);
	pinfo = (struct ov2710_info *)src->pinfo;
	client = &pinfo->client;

	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = data;

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 3;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("ov2710_write_reg(error) %d [0x%x:0x%x]\n",
			errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int ov2710_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
{
	int	errCode = 0;
	struct ov2710_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[1];
	u8 pbuf0[2];


	pinfo = (struct ov2710_info *)src->pinfo;
	client = &pinfo->client;

	pbuf0[1] = (subaddr & 0xff);
	pbuf0[0] = (subaddr >> 8);

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 2;
	msgs[0].buf = pbuf0;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("ov2710_read_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	}


	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags | I2C_M_RD;
	msgs[0].len = 1;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("ov2710_read_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = pbuf[0];
		errCode = 0;
	}

	return errCode;
}



/* ========================================================================== */

#include "ov2710_arch_reg_tbl.c"
#include "ov2710_reg_tbl.c"
#include "ov2710_video_tbl.c"

static void ov2710_print_info(struct __amba_vin_source *src)
{

}

/* ========================================================================== */
#include "arch/ov2710_arch.c"
#include "ov2710_docmd.c"
/* 	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ov2710_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int				errCode = 0;
	struct ov2710_info		*pinfo;
	struct __amba_vin_source	*src;

	/* Platform Info */
	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto ov2710_probe_exit;
	}
	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;
	pinfo->pll_index = -1;
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = OV2710_GAIN_ROWS - 1;
	pinfo->agc_info.db_max = 0x24000000;	// 36dB
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
	pinfo->agc_info.db_step = 0x00600000;	// 0.375dB

	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(pinfo->client));
	pinfo->client.addr = ov2710_addr;
	strlcpy(pinfo->client.name, ov2710_name, sizeof(pinfo->client.name));

	/* Vin Source */
	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, ov2710_name, sizeof(src->name));
	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = ov2710_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;
	errCode = amba_vin_add_source(src);
	if (errCode)
		goto ov2710_free_pinfo;

	errCode = ov2710_init_hw(src);
	if (errCode)
		goto ov2710_del_src;

	vin_info("OV2710 init(parallel)\n");

	goto ov2710_probe_exit;

ov2710_del_src:
	amba_vin_del_source(src);

ov2710_free_pinfo:
	kfree(pinfo);

ov2710_probe_exit:
	return errCode;
}

static int ov2710_remove(struct i2c_client *client)
{
	int				errCode = 0;
	struct ov2710_info		*pinfo;
	struct __amba_vin_source	*src;

	pinfo = (struct ov2710_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		ov2710_docmd(src, AMBA_VIN_SRC_SUSPEND, NULL);
		errCode = amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", ov2710_name);

	return errCode;
}

static struct i2c_device_id ov2710_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov2710_idtable);

static struct i2c_driver i2c_driver_ov2710 = {
	.driver = {
		   .name = "amb_vin0",
		   },
	.id_table	= ov2710_idtable,
	.probe		= ov2710_probe,
	.remove		= ov2710_remove,
};

static int __init ov2710_init(void)
{
	return i2c_add_driver(&i2c_driver_ov2710);
}

static void __exit ov2710_exit(void)
{
	i2c_del_driver(&i2c_driver_ov2710);
}

module_init(ov2710_init);
module_exit(ov2710_exit);

MODULE_DESCRIPTION("OV2710 1/3-Inch 2-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Qiao Wang, <qwang@ambarella.com>");
MODULE_LICENSE("Proprietary");

