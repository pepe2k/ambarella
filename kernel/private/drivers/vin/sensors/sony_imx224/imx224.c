/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx224/imx224.c
 *
 * History:
 *    2014/07/08 - [Long Zhao] Create
 *
 * Copyright (C) 2012-2014, Ambarella, Inc.
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
#include "imx224_pri.h"
#include "shutter_table.h"

/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_IMX224_ADDR
	#define CONFIG_I2C_AMBARELLA_IMX224_ADDR	(0x34>>1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(imx224, CONFIG_I2C_AMBARELLA_IMX224_ADDR, 0, 0644);

static int hdr_mode = 0;
module_param(hdr_mode, int, 0644);
MODULE_PARM_DESC(hdr_mode, "Set HDR mode 0:linear 1:2x HDR 2:3x HDR");

#define USE_960P_2X_30FPS

/* ========================================================================== */
struct imx224_info {
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
	u16 				min_agc_index;
	u16 				max_agc_index;
	u16 				min_wdr_agc_index;
	u16 				max_wdr_agc_index;
	u8				op_mode;
	u32 				sync_call_cnt;
	u8				high_gain_mode;

	amba_vin_agc_info_t agc_info;
	amba_vin_shutter_info_t shutter_info;
};

/* ========================================================================== */

static const char imx224_name[] = "imx224";
static struct i2c_driver i2c_driver_imx224;

static int imx224_write_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
{
	int	errCode = 0;
	struct imx224_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	pinfo = (struct imx224_info *)src->pinfo;
	client = &pinfo->client;

	pbuf[0] = subaddr>>0x08;
	pbuf[1] = subaddr&0xFF;
	pbuf[2] = data;

	msgs[0].addr = client->addr;
	if (unlikely(subaddr == IMX224_SWRESET))
		msgs[0].flags = client->flags | I2C_M_IGNORE_NAK;
	else
		msgs[0].flags = client->flags;
	msgs[0].len = 3;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("imx224_write_reg(error) %d [0x%x:0x%x]\n",
			errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int imx224_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
{
	int	errCode = 0;
	struct imx224_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	pinfo = (struct imx224_info *)src->pinfo;
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
		DRV_PRINT("imx224_read_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = pbuf[0];
		errCode = 0;
	}

	return errCode;
}

/* ========================================================================== */

#include "imx224_arch_reg_tbl.c"
#include "imx224_reg_tbl.c"
#include "imx224_video_tbl.c"

static void imx224_print_info(struct __amba_vin_source *src){}

/* ========================================================================== */
#include "arch/imx224_arch.c"
#include "imx224_docmd.c"
/* ========================================================================== */
static int imx224_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	errCode = 0;
	struct imx224_info		*pinfo;
	struct __amba_vin_source *src;

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto imx224_exit;
	}

	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = IMX224_GAIN_ROWS - 1;

	pinfo->agc_info.db_max = 0x30000000;	// 48dB
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
	pinfo->agc_info.db_step = 0x00199999;	// 0.1dB

	switch(hdr_mode) {
		case IMX224_LINEAR_MODE:
			vin_info("Linear mode\n");
			break;
		case IMX224_2X_WDR_MODE:
			vin_info("2x WDR mode\n");
			break;
		case IMX224_3X_WDR_MODE:
			vin_info("3x WDR mode\n");
			break;
		default:
			vin_err("Unsupported mode:%d\n", pinfo->op_mode);
			errCode = -EPERM;
			goto imx224_free_pinfo;
	}
	pinfo->op_mode = hdr_mode;

	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(pinfo->client));
	pinfo->client.addr = imx224_addr;
	strlcpy(pinfo->client.name, imx224_name, sizeof(pinfo->client.name));

	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, imx224_name, sizeof(src->name));

	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = imx224_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errCode = amba_vin_add_source(src);
	if (errCode)
		goto imx224_free_pinfo;
	errCode = imx224_init_hw(src);
	if (errCode)
		goto imx224_del_source;
	vin_info("IMX224 init(4-lane lvds)\n");

	goto imx224_exit;

imx224_del_source:
	amba_vin_del_source(src);
imx224_free_pinfo:
	kfree(pinfo);
imx224_exit:
	return errCode;
}

static int imx224_remove(struct i2c_client *client)
{
	int	errCode = 0;
	struct imx224_info *pinfo;
	struct __amba_vin_source *src;

	pinfo = (struct imx224_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		errCode |= amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", imx224_name);

	return errCode;
}

static struct i2c_device_id imx224_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx224_idtable);

static struct i2c_driver i2c_driver_imx224 = {
	.driver = {
		.name	= "amb_vin0",
	},
	.id_table	= imx224_idtable,
	.probe		= imx224_probe,
	.remove		= imx224_remove,
};
static int __init imx224_init(void)
{
	return i2c_add_driver(&i2c_driver_imx224);
}

static void __exit imx224_exit(void)
{
	i2c_del_driver(&i2c_driver_imx224);
}

module_init(imx224_init);
module_exit(imx224_exit);

MODULE_DESCRIPTION("IMX224 1/3 -Inch, 1280x960, 1.23-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

