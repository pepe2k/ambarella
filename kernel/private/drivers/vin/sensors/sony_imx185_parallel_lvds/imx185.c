/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx185/imx185.c
 *
 * History:
 *    2013/04/02 - [Ken He] Create
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
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
#include "imx185_pri.h"
#include "shutter_table.h"

static int slave_mode = 0;	// 0 : master mode, 1 : slave mode
module_param(slave_mode, int, 0644);
MODULE_PARM_DESC(slave_mode, "slave mode, 0: master mode, 1: slave mode");
/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_IMX185_ADDR
	#define CONFIG_I2C_AMBARELLA_IMX185_ADDR	(0x34>>1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(imx185, CONFIG_I2C_AMBARELLA_IMX185_ADDR, 0, 0644);

/* ========================================================================== */
struct imx185_info {
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
	u16				xvs_num;

	amba_vin_agc_info_t agc_info;
	amba_vin_shutter_info_t shutter_info;
	u32 agc_call_cnt;
};

/* ========================================================================== */

static const char imx185_name[] = "imx185";
static struct i2c_driver i2c_driver_imx185;

static int imx185_write_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
{
	int	errCode = 0;
	struct imx185_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	pinfo = (struct imx185_info *)src->pinfo;
	client = &pinfo->client;

	pbuf[0] = subaddr>>0x08;
	pbuf[1] = subaddr&0xFF;
	pbuf[2] = data;

	msgs[0].addr = client->addr;
	if (unlikely(subaddr == IMX185_SWRESET))
		msgs[0].flags = client->flags | I2C_M_IGNORE_NAK;
	else
		msgs[0].flags = client->flags;
	msgs[0].len = 3;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("imx185_write_reg(error) %d [0x%x:0x%x]\n",
			errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int imx185_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
{
	int	errCode = 0;
	struct imx185_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	pinfo = (struct imx185_info *)src->pinfo;
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
		DRV_PRINT("imx185_read_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = pbuf[0];
		errCode = 0;
	}

	return errCode;
}

/* ========================================================================== */

#include "imx185_arch_reg_tbl.c"
#include "imx185_reg_tbl.c"
#include "imx185_video_tbl.c"

static void imx185_print_info(struct __amba_vin_source *src){}

/* ========================================================================== */
#include "arch/imx185_arch.c"
#include "imx185_docmd.c"
/* ========================================================================== */
static int imx185_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	errCode = 0;
	struct imx185_info		*pinfo;
	struct __amba_vin_source *src;

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto imx185_exit;
	}

	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = IMX185_GAIN_ROWS - 1;

	pinfo->agc_info.db_max = 0x30000000;	// 48dB
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
	pinfo->agc_info.db_step = 0x004CCCCC;	// 0.3dB

	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(pinfo->client));
	pinfo->client.addr = imx185_addr;
	strlcpy(pinfo->client.name, imx185_name, sizeof(pinfo->client.name));

	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, imx185_name, sizeof(src->name));

	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = imx185_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errCode = amba_vin_add_source(src);
	if (errCode)
		goto imx185_free_pinfo;
	errCode = imx185_init_hw(src);
	if (errCode)
		goto imx185_del_source;
	vin_info("IMX185 init(parallel lvds)\n");
	if (slave_mode)
		vin_info("slave mode!\n");

	goto imx185_exit;

imx185_del_source:
	amba_vin_del_source(src);
imx185_free_pinfo:
	kfree(pinfo);
imx185_exit:
	return errCode;
}

static int imx185_remove(struct i2c_client *client)
{
	int	errCode = 0;
	struct imx185_info *pinfo;
	struct __amba_vin_source *src;

	pinfo = (struct imx185_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		errCode |= amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", imx185_name);

	return errCode;
}

static struct i2c_device_id imx185_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx185_idtable);

static struct i2c_driver i2c_driver_imx185 = {
	.driver = {
		.name	= "amb_vin0",
	},
	.id_table	= imx185_idtable,
	.probe		= imx185_probe,
	.remove		= imx185_remove,
};
static int __init imx185_init(void)
{
	return i2c_add_driver(&i2c_driver_imx185);
}

static void __exit imx185_exit(void)
{
	i2c_del_driver(&i2c_driver_imx185);
}

module_init(imx185_init);
module_exit(imx185_exit);

MODULE_DESCRIPTION("IMX185 1/1.9 -Inch, 1920x1200, 2.31-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Ken He <jianhe@ambarella.com>");
MODULE_LICENSE("Proprietary");

