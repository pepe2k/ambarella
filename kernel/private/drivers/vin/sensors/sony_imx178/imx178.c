/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx178/imx178.c
 *
 * History:
 *    2012/12/24 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
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
#include <linux/bitrev.h>

#include "utils.h"
#include "imx178_pri.h"
/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_IMX178_ADDR
	#define CONFIG_I2C_AMBARELLA_IMX178_ADDR	(0x34>>1)
#endif

AMBA_VIN_BASIC_PARAM_CALL(imx178, CONFIG_I2C_AMBARELLA_IMX178_ADDR, 0, 0644);

static int spi = 0;
module_param(spi, int, 0644);
MODULE_PARM_DESC(spi, " Use SPI interface 0:I2C 1:SPI");
static int slave_mode = 0;	// 0 : master mode, 1 : slave mode
module_param(slave_mode, int, 0644);
MODULE_PARM_DESC(slave_mode, "slave mode, 0: master mode, 1: slave mode");
static int spi_bus = 0;
module_param(spi_bus, int, 0644);
MODULE_PARM_DESC(spi_bus, "SPI controller ID");
static int spi_cs = 0;
module_param(spi_cs, int, 0644);
MODULE_PARM_DESC(spi_cs, "SPI CS");
static int llp_mode = 0;
module_param(llp_mode, int, 0644);
MODULE_PARM_DESC(llp_mode, "Set mode 0:HLP mode(high light) 1:LLP mode(low light)");
static int bits = 14;
module_param(bits, int, 0644);
MODULE_PARM_DESC(bits, "Set ADC bit: 10: 10bits 12: 12bits 14: 14bits");
/* ========================================================================== */
struct imx178_info {
	struct i2c_client		client;
	struct spi_device		*spi;
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
	u16				old_shr_width_upper;
	int				current_slowshutter_mode;
	u16				xvs_num;
	u8				lane_num;
	u32				pixel_size;

	amba_vin_agc_info_t agc_info;
	amba_vin_shutter_info_t shutter_info;
};

/* ========================================================================== */

static const char imx178_name[] = "imx178";
static struct i2c_driver i2c_driver_imx178;
static struct imx178_info *pinfo = 0; /*memory handle */

static int imx178_write_reg( struct __amba_vin_source *src, u16 subaddr, u8 data)
{
	int	errCode = 0;
	u8 pbuf[3];

	vin_dbg("imx178 write reg - 0x%x, data - 0x%x\n", subaddr, data);

	if (spi) {
		amba_spi_cfg_t config;
		amba_spi_write_t write;
		u8	i;

		switch(subaddr >> 8) {
		case 0x30: /* cid: 0x02 */
			subaddr &= 0x00FF;
			subaddr |= 0x0200;
			break;
		case 0x31: /* cid: 0x03 */
			subaddr &= 0x00FF;
			subaddr |= 0x0300;
			break;
		case 0x32: /* cid: 0x04 */
			subaddr &= 0x00FF;
			subaddr |= 0x0400;
			break;
		case 0x33: /* cid: 0x05 */
			subaddr &= 0x00FF;
			subaddr |= 0x0500;
			break;
		}

		pbuf[0] = subaddr>>0x08;
		pbuf[1] = subaddr&0xFF;
		pbuf[2] = data;

		config.cfs_dfs = 8;//bits
		config.baud_rate = 1000000;
		config.cs_change = 0;
		config.spi_mode = SPI_MODE_3;

		write.buffer = pbuf;
		write.n_size = 3;
		write.cs_id = spi_cs;
		write.bus_id = spi_bus;

		/* reverse the bit, LSB first */
		for(i = 0; i < write.n_size; i++)
			pbuf[i] = bitrev8(pbuf[i]);

		ambarella_spi_write(&config, &write);
	} else { /* I2C */
		struct i2c_client *client;
		struct i2c_msg msgs[1];

		pinfo = (struct imx178_info *)src->pinfo;
		client = &pinfo->client;

		pbuf[0] = (subaddr >> 8);
		pbuf[1] = (subaddr & 0xff);
		pbuf[2] = data;

		msgs[0].addr = client->addr;
		if (unlikely(subaddr == IMX178_SWRESET))
			msgs[0].flags = client->flags | I2C_M_IGNORE_NAK;
		else
			msgs[0].flags = client->flags;
		msgs[0].len = 3;
		msgs[0].buf = pbuf;

		errCode = i2c_transfer(client->adapter, msgs, 1);
		if (errCode != 1) {
			vin_err("imx178_write_reg(error) %d [0x%x:0x%x]\n",
				errCode, subaddr, data);
			errCode = -EIO;
		} else {
			errCode = 0;
		}
	}

	return errCode;
}

static int imx178_read_reg( struct __amba_vin_source *src, u16 subaddr, u8 *pdata)
{
	int	errCode = 0;
	u8 pbuf[3];

	if (spi) {
		amba_spi_cfg_t config;
		amba_spi_write_then_read_t write;
		u8	tmp, i;

		switch(subaddr >> 8) {
		case 0x30: /* cid: 0x02 */
			subaddr &= 0x00FF;
			subaddr |= 0x0200;
			break;
		case 0x31: /* cid: 0x03 */
			subaddr &= 0x00FF;
			subaddr |= 0x0300;
			break;
		case 0x32: /* cid: 0x04 */
			subaddr &= 0x00FF;
			subaddr |= 0x0400;
			break;
		case 0x33: /* cid: 0x05 */
			subaddr &= 0x00FF;
			subaddr |= 0x0500;
			break;
		}

		pbuf[0] = (subaddr>>8)|0x80;
		pbuf[1] = subaddr & 0xFF;

		config.cfs_dfs = 8;//bits
		config.baud_rate = 1000000;
		config.cs_change = 0;
		config.spi_mode = SPI_MODE_3;

		write.w_buffer = pbuf;
		write.w_size = 2;
		write.r_buffer = &tmp;
		write.r_size = 1;
		write.cs_id = spi_cs;
		write.bus_id = spi_bus;

		/* reverse the bit, LSB first */
		for(i = 0; i < write.w_size; i++)
			pbuf[i] = bitrev8(pbuf[i]);

		ambarella_spi_write_then_read(&config, &write);

		/* reverse the bit, LSB first */
		*pdata = bitrev8(tmp);
	} else { /* I2C */
		struct i2c_client *client;
		struct i2c_msg msgs[2];
		u8 pbuf0[2];

		pinfo = (struct imx178_info *)src->pinfo;
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
			vin_err("imx178_read_reg(error) %d [0x%x]\n",
				errCode, subaddr);
			errCode = -EIO;
		} else {
			*pdata = pbuf[0];
			errCode = 0;
		}
	}
	return errCode;
}

/* ========================================================================== */

#include "imx178_arch_reg_tbl.c"
#include "imx178_reg_tbl.c"
#include "imx178_video_tbl.c"

static void imx178_print_info(struct __amba_vin_source *src){}

/* ========================================================================== */
#include "arch/imx178_arch.c"
#include "imx178_docmd.c"

/* ========================================================================== */
static int imx178_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	errCode = 0;
	struct __amba_vin_source *src;

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto imx178_exit;
	}

	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = IMX178_GAIN_ROWS - 1;
	pinfo->pixel_size = 0x00026666; /* 2.4um */

	if (llp_mode) {// low light mode: 3.0~51.0db/0.1db step
		pinfo->agc_info.db_max = 0x33000000;	// 51dB
		pinfo->agc_info.db_min = 0x03000000;	// 3dB
		pinfo->agc_info.db_step = 0x00199999;	// 0.1dB
	} else {// high light mode: 0.0~48.0db/0.1db step
		pinfo->agc_info.db_max = 0x30000000;	// 48dB
		pinfo->agc_info.db_min = 0x00000000;	// 0dB
		pinfo->agc_info.db_step = 0x00199999;	// 0.1dB
	}

	/* I2c Client */
	if (!spi){
		i2c_set_clientdata(client, pinfo);
		memcpy(&pinfo->client, client, sizeof(pinfo->client));
		pinfo->client.addr = imx178_addr;
		strlcpy(pinfo->client.name, imx178_name, sizeof(pinfo->client.name));
	}
	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, imx178_name, sizeof(src->name));

	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = imx178_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errCode = amba_vin_add_source(src);
	if (errCode)
		goto imx178_free_pinfo;
	errCode = imx178_init_hw(src);
	if (errCode)
		goto imx178_del_source;
	vin_info("IMX178 init(10-lane lvds)-with I2C interface\n");
	if (slave_mode)
		vin_info("slave mode!\n");

	goto imx178_exit;

imx178_del_source:
	amba_vin_del_source(src);
imx178_free_pinfo:
	kfree(pinfo);
imx178_exit:
	return errCode;
}

/************************ for SPI *****************************/
static int imx178_spi_probe(void)
{
	int	errCode = 0;

	struct __amba_vin_source *src;

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto imx178_exit;
	}

	pinfo->current_video_index = -1;
	pinfo->frame_rate = AMBA_VIDEO_FPS_29_97;	//default value.
	pinfo->pll_index = 0;				//default value.
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = IMX178_GAIN_ROWS - 1;
	pinfo->pixel_size = 0x00026666; /* 2.4um */

	if (llp_mode) {// low light mode: 3.0~51.0db/0.1db step
		pinfo->agc_info.db_max = 0x33000000;	// 51dB
		pinfo->agc_info.db_min = 0x03000000;	// 3dB
		pinfo->agc_info.db_step = 0x00199999;	// 0.1dB
	} else {// high light mode: 0.0~48.0db/0.1db step
		pinfo->agc_info.db_max = 0x30000000;	// 48dB
		pinfo->agc_info.db_min = 0x00000000;	// 0dB
		pinfo->agc_info.db_step = 0x00199999;	// 0.1dB
	}

	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, imx178_name, sizeof(src->name));

	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = imx178_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errCode = amba_vin_add_source(src);
	if (errCode)
		goto imx178_free_pinfo;
	errCode = imx178_init_hw(src);
	if (errCode)
		goto imx178_del_source;
	vin_info("IMX178 init(10-lane lvds)-with SPI interface\n");
	if (slave_mode)
		vin_info("slave mode!\n");

	goto imx178_exit;

imx178_del_source:
	amba_vin_del_source(src);
imx178_free_pinfo:
	kfree(pinfo);
imx178_exit:
	return errCode;
}

static int imx178_remove(struct i2c_client *client)
{
	int	errCode = 0;
	struct imx178_info *pinfo;
	struct __amba_vin_source *src;

	pinfo = (struct imx178_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		src = &pinfo->source;
		errCode |= amba_vin_del_source(src);
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", imx178_name);

	return errCode;
}

static struct i2c_device_id imx178_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx178_idtable);

static struct i2c_driver i2c_driver_imx178 = {
	.driver = {
		.name	= "amb_vin0",
	},
	.id_table	= imx178_idtable,
	.probe		= imx178_probe,
	.remove		= imx178_remove,
};

static int __init imx178_init(void)
{
	if (!spi)
		return i2c_add_driver(&i2c_driver_imx178);
	else {
		return imx178_spi_probe();
	}
}

static void __exit imx178_exit(void)
{
	if (!spi)
		i2c_del_driver(&i2c_driver_imx178);
	else {
		amba_vin_del_source(&pinfo->source);
		kfree((void*)pinfo);
	}
}

module_init(imx178_init);
module_exit(imx178_exit);

MODULE_DESCRIPTION("IMX178 1/1.8 -Inch, 3096x2080, 6.44-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

