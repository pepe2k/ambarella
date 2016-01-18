/*
 * Filename : ov5658.c
 *
 * History:
 *    2013/08/28 - [Johnson Diao] Create
 *    2013/09/18 - [Johnson Diao] support 2lane
 * Copyright (C) 2013-2017, Ambarella, Inc.
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
#include "ov5658_pri.h"

static int OV5658_LANE=4;
module_param(OV5658_LANE, int, S_IRUGO);


/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_OV5658_ADDR
	#define CONFIG_I2C_AMBARELLA_OV5658_ADDR	(0x6c>>1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(ov5658, CONFIG_I2C_AMBARELLA_OV5658_ADDR, 0, 0644);

/* ========================================================================== */
struct ov5658_info {
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
	u32						current_agc_db;
	u16						min_agc_index;
	u16						max_agc_index;

	int						current_slowshutter_mode;
	amba_vin_agc_info_t				agc_info;
	amba_vin_shutter_info_t				shutter_info;
};
/* ========================================================================== */

static const char ov5658_name[] = "ov5658";
static struct i2c_driver i2c_driver_ov5658;


static int ov5658_write_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
{
	int errCode = 0;
	struct ov5658_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

//	DRV_PRINT("cddiao write 0x%x=0x%x\n",subaddr,data)
	vin_dbg("ov5658 write reg - 0x%x, data - 0x%x\n", subaddr, data);
	pinfo = (struct ov5658_info *)src->pinfo;
	client = &pinfo->client;



	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = (data & 0xff);

	msgs[0].len = 3;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("ov5658_write_reg(error) %d [0x%x:0x%x]\n",
			errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int ov5658_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
{
	int	errCode = 0;
	struct ov5658_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[6];


	pinfo = (struct ov5658_info *)src->pinfo;
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
		DRV_PRINT("ov5658_read_reg(error) %d [0x%x]\n",
			errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = pbuf[0];
		errCode = 0;
	}

	return errCode;
}


#if 0
/*
 *	Write any register field
 *	Para:
 *   	@reg, register
 *		@bitfild, mask
 *      @value, value to set
 *	<registername>, [<bitfieldname>,] <value>
 */
static int ov5658_write_reg_field(struct __amba_vin_source *src, u16 add, u8 bitfield, u8 value)
{
	int errCode = 0;
	u8 regdata;
	int mask_bit_start = 0;
	errCode = ov5658_read_reg(src, add, &regdata);

	if (errCode != 0)
		return -1;
	regdata &= (~bitfield);
	while ((bitfield & 0x01) == 0) {
		mask_bit_start++;
		bitfield >>= 0x01;
	}
	value <<= mask_bit_start;
	regdata |= value;
	vin_dbg("regdata = 0x%04x, value_byshift = 0x%04x, mask_bit_start =  %04d\n", regdata, value,mask_bit_start);
	errCode = ov5658_write_reg(src, add, regdata);

	return errCode;
}
#endif
/* ========================================================================== */

#include "ov5658_arch_reg_tbl.c"
#include "ov5658_reg_tbl.c"
#include "ov5658_video_tbl.c"

static void ov5658_print_info(struct __amba_vin_source *src)
{

}

/* ========================================================================== */
#include "arch/ov5658_arch.c"
#include "ov5658_docmd.c"
/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ov5658_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int					errCode = 0;
	u16					sen_id = 0;
	struct ov5658_info			*pinfo;
	struct __amba_vin_source		*src;

	/* Platform Info */
	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto ov5658_probe_exit;
	}
	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = OV5658_GAIN_ROWS - 1;
	pinfo->agc_info.db_max = 0x2a000000;	// 42dB
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
	pinfo->agc_info.db_step = 0x0060c183;	// 0.377953 db
	pinfo->current_slowshutter_mode = 0;

	/* I2c Client */
	i2c_set_clientdata(client, pinfo);
	memcpy(&pinfo->client, client, sizeof(*client));
	pinfo->client.addr = ov5658_addr;
	strlcpy(pinfo->client.name, ov5658_name, sizeof(pinfo->client.name));


	/* Vin Source */
	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, ov5658_name, sizeof(src->name));
	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = ov5658_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;
	errCode = amba_vin_add_source(src);
	if (errCode)
		goto ov5658_free_pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	if((OV5658_LANE != 4) && (OV5658_LANE != 2)){
		vin_info("OV5658 only support 2/4 lane mipi, your setting is %d lanes\n",OV5658_LANE);
		vin_info("The driver will fix it to default setting");
		OV5658_LANE = 4;
	}
	if (OV5658_LANE == 4){
		errCode = ov5658_init_vin_clock(src, &ov5658_pll_4lane_tbl[pinfo->pll_index]);
	}else if (OV5658_LANE == 2){
		errCode = ov5658_init_vin_clock(src, &ov5658_pll_2lane_tbl[pinfo->pll_index]);
	}
	if (errCode)
		goto ov5658_del_vin_src;
	msleep(10);
	ov5658_reset(src);

	errCode = ov5658_query_sensor_id(src, &sen_id);
	if (errCode)
		goto ov5658_del_vin_src;
	vin_info("OV5658 sensor ID is 0x%x\n", sen_id);

	vin_info("OV5658 init(%d-lane mipi)\n",OV5658_LANE);

	goto ov5658_probe_exit;

ov5658_del_vin_src:
	amba_vin_del_source(src);

ov5658_free_pinfo:
	kfree(pinfo);

ov5658_probe_exit:
	return errCode;
}

static int ov5658_remove(struct i2c_client *client)
{
	int				errCode = 0;
	struct ov5658_info		*pinfo;
	struct __amba_vin_source	*src;

	pinfo = (struct ov5658_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		errCode = amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", ov5658_name);

	return errCode;
}

static struct i2c_device_id ov5658_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov5658_idtable);

static struct i2c_driver i2c_driver_ov5658 = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "amb_vin0",
	},
	.id_table	= ov5658_idtable,
	.probe		= ov5658_probe,
	.remove		= ov5658_remove,
};

static int __init ov5658_init(void)
{
	return i2c_add_driver(&i2c_driver_ov5658);
}

static void __exit ov5658_exit(void)
{
	i2c_del_driver(&i2c_driver_ov5658);
}

module_init(ov5658_init);
module_exit(ov5658_exit);

MODULE_DESCRIPTION("OV5658 1/3.2-Inch 5-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Diao Chengdong, <cddiao@ambarella.com>");
MODULE_LICENSE("Proprietary");

