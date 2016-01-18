/*
 * kernel/private/drivers/ambarella/vin/sensors/panasonic_mn34220pl/mn34220pl.c
 *
 * History:
 *    2013/06/08 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
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
#include "mn34220pl_pri.h"

#define GAIN_160_STEPS	(1) //use 160 steps gain table

/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_MN34220PL_ADDR
	#define CONFIG_I2C_AMBARELLA_MN34220PL_ADDR	(0x6C>>1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(mn34220pl, CONFIG_I2C_AMBARELLA_MN34220PL_ADDR, 0, 0644);

static int hdr_mode = 0;
module_param(hdr_mode, int, 0644);
MODULE_PARM_DESC(hdr_mode, "Set HDR mode 0:linear 1:2x HDR 2:3x HDR 3:4x HDR");

static int fps120_mode = 0;
module_param(fps120_mode, int, 0644);
MODULE_PARM_DESC(fps120_mode, "120fps mode , 0:off 1:on");
/* ========================================================================== */
struct mn34220pl_info {
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
	u32				frame_rate;
	u32				pll_index;
	u32				mode_type;
	u32				mode_index;
	struct amba_vin_black_level_compensation current_sw_blc;
	u32				current_shutter_time;
	u32				current_gain_db;
	u16				min_agc_index;
	u16				max_agc_index;
	u16				min_wdr_gain_index;
	u16				max_wdr_gain_index;
	u16				old_shr_width_upper;
	int				current_slowshutter_mode;
	u8				op_mode;
	u32				pixel_size;

	amba_vin_agc_info_t agc_info;
	amba_vin_agc_info_t wdr_gain_info;
	amba_vin_shutter_info_t shutter_info;
	u32 agc_call_cnt;
};

/* ========================================================================== */

static const char mn34220pl_name[] = "mn34220pl";
static struct i2c_driver i2c_driver_mn34220pl;

static int mn34220pl_write_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
{
	int	errCode = 0;
	struct mn34220pl_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	vin_dbg("mn34220pl write reg - 0x%x, data - 0x%x\n", subaddr, data);
	pinfo = (struct mn34220pl_info *)src->pinfo;
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
		DRV_PRINT("mn34220pl_write_reg(error) %d [0x%x:0x%x]\n",
			errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int mn34220pl_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
{
	int	errCode = 0;
	struct mn34220pl_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	pinfo = (struct mn34220pl_info *)src->pinfo;
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
	msgs[1].len = 1;

	errCode = i2c_transfer(client->adapter, msgs, 2);
	if ((errCode != 1) && (errCode != 2)){
		DRV_PRINT("mn34220pl_read_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = pbuf[0];
		errCode = 0;
	}

	return errCode;
}

/* ========================================================================== */

#include "mn34220pl_arch_reg_tbl.c"
#include "mn34220pl_reg_tbl.c"
#include "mn34220pl_video_tbl.c"

static void mn34220pl_print_info(struct __amba_vin_source *src){}

/* ========================================================================== */
#include "arch/mn34220pl_arch.c"
#include "mn34220pl_docmd.c"
/* ========================================================================== */
static int mn34220pl_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	errCode = 0;
	struct mn34220pl_info		*pinfo;
	struct __amba_vin_source *src;

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto mn34220pl_exit;
	}

	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = MN34220PL_GAIN_ROWS - 1;
	pinfo->pixel_size = 0x0002C000; /* 2.75um */

	pinfo->agc_info.db_max = 0x3C000000;	// 60dB
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
#if GAIN_160_STEPS
	pinfo->agc_info.db_step = 0x00600000;	// 0.375dB
#else
	pinfo->agc_info.db_step = 0x00180000;	// 0.09375dB
#endif

	/* AGAIN and DGAIN for WDR mode */
	pinfo->wdr_gain_info.db_max = 0x1E000000;	// 30dB
	pinfo->wdr_gain_info.db_min = 0x00000000;	// 0dB
#if GAIN_160_STEPS
	pinfo->wdr_gain_info.db_step = 0x00600000;	// 0.375dB
#else
	pinfo->wdr_gain_info.db_step = 0x00180000;	// 0.09375dB
#endif
	pinfo->min_wdr_gain_index = 0;
	pinfo->max_wdr_gain_index = MN34220PL_WDR_GAIN_30DB;

	switch(hdr_mode) {
		case MN34220PL_LINEAR_MODE:
			vin_info("Linear mode\n");
			break;
		case MN34220PL_2X_WDR_MODE:
			vin_info("2x WDR mode\n");
			break;
		case MN34220PL_3X_WDR_MODE:
			vin_info("3x WDR mode\n");
			break;
		case MN34220PL_4X_WDR_MODE:
			vin_info("4x WDR mode\n");
			break;
		default:
			vin_err("Unknown mode\n");
			errCode = -EPERM;
			goto mn34220pl_free_pinfo;
	}
	pinfo->op_mode = hdr_mode;

	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(pinfo->client));
	pinfo->client.addr = mn34220pl_addr;
	strlcpy(pinfo->client.name, mn34220pl_name, sizeof(pinfo->client.name));

	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, mn34220pl_name, sizeof(src->name));

	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = mn34220pl_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errCode = amba_vin_add_source(src);
	if (errCode)
		goto mn34220pl_free_pinfo;
	errCode = mn34220pl_init_hw(src);
	if (errCode)
		goto mn34220pl_del_source;

	vin_info("MN34220PL init(6-lane lvds)\n");

	goto mn34220pl_exit;

mn34220pl_del_source:
	amba_vin_del_source(src);
mn34220pl_free_pinfo:
	kfree(pinfo);
mn34220pl_exit:
	return errCode;
}

static int mn34220pl_remove(struct i2c_client *client)
{
	int	errCode = 0;
	struct mn34220pl_info *pinfo;
	struct __amba_vin_source *src;

	pinfo = (struct mn34220pl_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		errCode |= amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", mn34220pl_name);

	return errCode;
}

static struct i2c_device_id mn34220pl_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mn34220pl_idtable);

static struct i2c_driver i2c_driver_mn34220pl = {
	.driver = {
		.name	= "amb_vin0",
	},
	.id_table	= mn34220pl_idtable,
	.probe		= mn34220pl_probe,
	.remove		= mn34220pl_remove,
};

static int __init mn34220pl_init(void)
{
	return i2c_add_driver(&i2c_driver_mn34220pl);
}

static void __exit mn34220pl_exit(void)
{
	i2c_del_driver(&i2c_driver_mn34220pl);
}

module_init(mn34220pl_init);
module_exit(mn34220pl_exit);

MODULE_DESCRIPTION("MN34220PL 1/3 -Inch, 1944x1213, 2.4-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

