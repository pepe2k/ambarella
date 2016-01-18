/*
 * kernel/private/drivers/ambarella/vin/sensors/omnivision_ov4689/ov4689.c
 *
 * History:
 *    2013/05/29 - [Long Zhao] Create
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
#include "ov4689_pri.h"
#include "shutter_table.h"

/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_OV4689_ADDR
#define CONFIG_I2C_AMBARELLA_OV4689_ADDR	(0x6C >> 1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(ov4689, CONFIG_I2C_AMBARELLA_OV4689_ADDR, 0, 0644);

static int hdr_mode = 0;
module_param(hdr_mode, int, 0644);
MODULE_PARM_DESC(hdr_mode, "Set HDR mode 0:linear 1:2x HDR 2:3x HDR");

static int lane = 4;
module_param(lane, int, 0644);
MODULE_PARM_DESC(lane, "Set MIPI lane number 2:2 lane 4:4 lane");
/* ========================================================================== */
struct ov4689_info {
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
	u16				min_agc_index;
	u16				max_agc_index;
	u16				min_wdr_again_index;
	u16				max_wdr_again_index;
	u16				min_wdr_dgain_index;
	u16				max_wdr_dgain_index;
	int				current_slowshutter_mode;
	u8				op_mode;
	u32				pixel_size;

	amba_vin_agc_info_t agc_info;
	amba_vin_shutter_info_t shutter_info;
};

/* ========================================================================== */

static const char ov4689_name[] = "ov4689";
static struct i2c_driver i2c_driver_ov4689;

static int ov4689_write_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
{
	int	errCode = 0;
	struct ov4689_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	pinfo = (struct ov4689_info *)src->pinfo;
	client = &pinfo->client;
	vin_dbg("0x%x, 0x%x\n", subaddr, data);

	pbuf[0] = subaddr>>0x08;
	pbuf[1] = subaddr&0xFF;
	pbuf[2] = data;

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 3;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("ov4689_write_reg(error) %d [0x%x:0x%x]\n",
			errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int ov4689_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
{
	int	errCode = 0;
	struct ov4689_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	pinfo = (struct ov4689_info *)src->pinfo;
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
		DRV_PRINT("ov4689_read_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = pbuf[0];
		errCode = 0;
	}

	return errCode;
}

/* ========================================================================== */

#include "ov4689_arch_reg_tbl.c"
#include "ov4689_reg_tbl.c"
#include "ov4689_video_tbl.c"

static void ov4689_print_info(struct __amba_vin_source *src){}

/* ========================================================================== */
#include "arch/ov4689_arch.c"
#include "ov4689_docmd.c"
/* ========================================================================== */
static int ov4689_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	errCode = 0;
	struct ov4689_info		*pinfo;
	struct __amba_vin_source *src;

	if(lane !=2 && lane !=4) {
		vin_err("Current driver can only support 2 or 4 lane!\n");
		goto ov4689_exit;
	}

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto ov4689_exit;
	}

	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_59_94;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
	pinfo->pixel_size = 0x00020000; /* 2.0um */

	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = OV4689_GAIN_ROWS - 1;

	pinfo->min_wdr_again_index = 0;
	pinfo->max_wdr_again_index = OV4689_AGAIN_ROWS - 1;

	pinfo->min_wdr_dgain_index = 0;
	pinfo->max_wdr_dgain_index = OV4689_DGAIN_ROWS - 1;

	pinfo->agc_info.db_max = 0x24000000;	// 36dB
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
	pinfo->agc_info.db_step = 0x00180000;	// 0.09375dB

	switch(hdr_mode) {
		case OV4689_LINEAR_MODE:
			vin_info("Linear mode\n");
			break;
		case OV4689_2X_WDR_MODE:
			vin_info("2x WDR mode\n");
			break;
		case OV4689_3X_WDR_MODE:
			vin_info("3x WDR mode\n");
			break;
		default:
			vin_err("Unknown mode\n");
			errCode = -EPERM;
			goto ov4689_free_pinfo;
	}
	pinfo->op_mode = hdr_mode;

	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(pinfo->client));
	pinfo->client.addr = ov4689_addr;
	strlcpy(pinfo->client.name, ov4689_name, sizeof(pinfo->client.name));

	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, ov4689_name, sizeof(src->name));

	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = ov4689_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errCode = amba_vin_add_source(src);
	if (errCode)
		goto ov4689_free_pinfo;
	errCode = ov4689_init_hw(src);
	if (errCode)
		goto ov4689_del_source;
	vin_info("OV4689 init(%d-lane mipi)\n", lane);

	goto ov4689_exit;

ov4689_del_source:
	amba_vin_del_source(src);
ov4689_free_pinfo:
	kfree(pinfo);
ov4689_exit:
	return errCode;
}

static int ov4689_remove(struct i2c_client *client)
{
	int	errCode = 0;
	struct ov4689_info *pinfo;
	struct __amba_vin_source *src;

	pinfo = (struct ov4689_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		errCode |= amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", ov4689_name);

	return errCode;
}

static struct i2c_device_id ov4689_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov4689_idtable);

static struct i2c_driver i2c_driver_ov4689 = {
	.driver = {
		.name	= "amb_vin0",
	},
	.id_table	= ov4689_idtable,
	.probe		= ov4689_probe,
	.remove		= ov4689_remove,
};
static int __init ov4689_init(void)
{
	return i2c_add_driver(&i2c_driver_ov4689);
}

static void __exit ov4689_exit(void)
{
	i2c_del_driver(&i2c_driver_ov4689);
}

module_init(ov4689_init);
module_exit(ov4689_exit);

MODULE_DESCRIPTION("OV4689 1/3 -Inch, 2688x1520, 4-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

