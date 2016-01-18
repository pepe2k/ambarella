/*
 * kernel/private/drivers/ambarella/vin/sensors/ambds/ambds.c
 *
 * History:
 *    2014/12/30 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
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
#include "ambds_pri.h"
/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_AMBDS_ADDR
	#define CONFIG_I2C_AMBARELLA_AMBDS_ADDR	(0x34>>1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(ambds, CONFIG_I2C_AMBARELLA_AMBDS_ADDR, 0, 0644);

static int lane = 4;
module_param(lane, int, 0644);
MODULE_PARM_DESC(lane, "lvds lane number: 1~12");

static int input_mode = VIN_YUV_LVDS_2PELS_DDR_CRY0_CBY1_LVCMOS;
module_param(input_mode, int, 0644);
MODULE_PARM_DESC(input_mode, "input_mode");

static int sync_duplicate = 1;
module_param(sync_duplicate, int, 0644);
MODULE_PARM_DESC(sync_duplicate, "sync code sytle: duplicate on each lane");
/* ========================================================================== */
struct ambds_info {
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
	u32				frame_rate;
	u32				pll_index;
	u32				mode_type;
	u32				mode_index;
	struct amba_vin_black_level_compensation current_sw_blc;
	u32				current_shutter_time;
	u32				current_gain_db;
	u16				min_agc_index;
	u16				max_agc_index;
	u16				old_shr_width_upper;
	int				current_slowshutter_mode;

	amba_vin_agc_info_t agc_info;
	amba_vin_shutter_info_t shutter_info;
};

/* ========================================================================== */

static const char ambds_name[] = "ambds";
static struct i2c_driver i2c_driver_ambds;

static int ambds_write_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
{
	int	errCode = 0;
	struct ambds_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	vin_dbg("ambds write reg - 0x%x, data - 0x%x\n", subaddr, data);
	pinfo = (struct ambds_info *)src->pinfo;
	client = &pinfo->client;

	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = data;

	msgs[0].addr = client->addr;
	if (unlikely(subaddr == AMBDS_SWRESET))
		msgs[0].flags = client->flags | I2C_M_IGNORE_NAK;
	else
		msgs[0].flags = client->flags;
	msgs[0].len = 3;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		vin_err("ambds_write_reg(error) %d [0x%x:0x%x]\n",
			errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int ambds_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
{
	int	errCode = 0;
	struct ambds_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	pinfo = (struct ambds_info *)src->pinfo;
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
		vin_err("ambds_read_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = pbuf[0];
		errCode = 0;
	}

	return errCode;
}

/* ========================================================================== */

#include "ambds_arch_reg_tbl.c"
#include "ambds_reg_tbl.c"
#include "ambds_video_tbl.c"

static void ambds_print_info(struct __amba_vin_source *src){}

/* ========================================================================== */
#include "arch/ambds_arch.c"
#include "ambds_docmd.c"
/* ========================================================================== */
static int ambds_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	errCode = 0;
	struct ambds_info		*pinfo;
	struct __amba_vin_source *src;

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto ambds_exit;
	}

	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->min_agc_index = 0;

	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(pinfo->client));
	pinfo->client.addr = ambds_addr;
	strlcpy(pinfo->client.name, ambds_name, sizeof(pinfo->client.name));

	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, ambds_name, sizeof(src->name));

	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = ambds_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errCode = amba_vin_add_source(src);
	if (errCode)
		goto ambds_free_pinfo;
	errCode = ambds_init_hw(src);
	if (errCode)
		goto ambds_del_source;

	vin_info("AMBDS init(%d-lane lvds)\n", lane);

	goto ambds_exit;

ambds_del_source:
	amba_vin_del_source(src);
ambds_free_pinfo:
	kfree(pinfo);
ambds_exit:
	return errCode;
}

static int ambds_remove(struct i2c_client *client)
{
	int	errCode = 0;
	struct ambds_info *pinfo;
	struct __amba_vin_source *src;

	pinfo = (struct ambds_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		errCode |= amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", ambds_name);

	return errCode;
}

static struct i2c_device_id ambds_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ambds_idtable);

static struct i2c_driver i2c_driver_ambds = {
	.driver = {
		.name	= "amb_vin0",
	},
	.id_table	= ambds_idtable,
	.probe		= ambds_probe,
	.remove		= ambds_remove,
};

static int __init ambds_init(void)
{
	return i2c_add_driver(&i2c_driver_ambds);
}

static void __exit ambds_exit(void)
{
	i2c_del_driver(&i2c_driver_ambds);
}

module_init(ambds_init);
module_exit(ambds_exit);

MODULE_DESCRIPTION("AMBDS, 4096x2160, AMBA DUMMY SENSOR");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

