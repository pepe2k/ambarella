/*
 * kernel/private/drivers/ambarella/vin/decoders/adv7441a/adv7441a.c
 *
 * History:
 *    2009/07/23 - [Qiao Wang] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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

#include "adv7441a_pri.h"
#include "arch/adv7441a_arch.h"
#include "adv7441a_reg_tbl.c"

/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_ADV7441A_USERMAP_ADDR
#define CONFIG_I2C_AMBARELLA_ADV7441A_USERMAP_ADDR	(0x42 >> 1)
#endif

#ifndef CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET
#define CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET \
	((CONFIG_I2C_AMBARELLA_ADV7441A_USERMAP_ADDR << 1) - 0x40)
#endif

AMBA_VIN_BASIC_PARAM_CALL(adv7441a,
	CONFIG_I2C_AMBARELLA_ADV7441A_USERMAP_ADDR, 0, 0644);

static int swap_cbcr = 0;
module_param(swap_cbcr, int, 0644);
MODULE_PARM_DESC(swap_cbcr, "Set true will try swap cbcr");

static int check_hdmi_infoframe = 0;
module_param(check_hdmi_infoframe, int, 0644);
MODULE_PARM_DESC(check_hdmi_infoframe, "Set true will check HDMI infoframe.");

static char *edid_data = NULL;
MODULE_PARM_DESC(edid_data, "EDID Bin file for ADV7441A");
module_param(edid_data, charp, 0444);

/* ========================================================================== */
struct adv7441a_src_info {
	struct __amba_vin_source cap_src;
	u16 cap_src_id;

	enum amba_video_mode cap_vin_mode;
	u16 cap_start_x;
	u16 cap_start_y;
	u16 cap_cap_w;
	u16 cap_cap_h;
	u16 sync_start;
	u8 video_system;
	u32 frame_rate;
	u8 aspect_ratio;
	u8 input_type;
	u8 video_format;
	u8 bit_resolution;
	u8 input_format;

	u16 line_width;
	u16 f0_height;
	u16 f1_height;
	u16 hs_front_porch;
	u16 hs_plus_width;
	u16 hs_back_porch;
	u16 f0_vs_front_porch;
	u16 f0_vs_plus_width;
	u16 f0_vs_back_porch;
	u16 f1_vs_front_porch;
	u16 f1_vs_plus_width;
	u16 f1_vs_back_porch;

	const struct adv7441a_reg_table *fix_reg_table;
	u32 so_freq_hz;
	u32 so_pclk_freq_hz;
};

struct adv7441a_info {
	struct i2c_client idc_usrmap;
	struct i2c_client idc_usrmap1;
	struct i2c_client idc_usrmap2;
	struct i2c_client idc_vdpmap;
	struct i2c_client idc_hdmimap;
	struct i2c_client idc_rksvmap;
	struct i2c_client idc_edidmap;
	struct i2c_client idc_revmap;

	struct adv7441a_src_info *active_vin_src;
	struct adv7441a_src_info sources[ADV7441A_SOURCE_NUM];

	u32 valid_edid;
	u8 edid[ADV7441A_EDID_TABLE_SIZE];
};

/* ========================================================================== */

static const char adv7441a_name[] = "adv7441a";
static struct i2c_driver i2c_driver_adv7441a;

static int adv7441a_write_reg(struct __amba_vin_source *src, u8 regmap, u8 subaddr, u8 data)
{
	int errCode = 0;
	struct adv7441a_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[2];

	pinfo = (struct adv7441a_info *) src->pinfo;

	switch (regmap) {
	case USER_MAP:
		client = &pinfo->idc_usrmap;
		break;

	case USER_MAP_1:
		client = &pinfo->idc_usrmap1;
		break;

	case USER_MAP_2:
		client = &pinfo->idc_usrmap2;
		break;

	case VDP_MAP:
		client = &pinfo->idc_vdpmap;
		break;

	case HDMI_MAP:
		client = &pinfo->idc_hdmimap;
		break;

	case KSV_MAP:
		client = &pinfo->idc_rksvmap;
		break;

	case EDID_MAP:
		client = &pinfo->idc_edidmap;
		break;

	case RESERVED_MAP:
		client = &pinfo->idc_revmap;
		break;

	default:
		errCode = -EIO;
		goto adv7441a_write_reg_exit;
		break;
	}

	pbuf[0] = subaddr;
	pbuf[1] = data;

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 2;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("adv7441a_write_reg(error) %d [0x%x:0x%x]\n", errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

      adv7441a_write_reg_exit:
	return errCode;
}

static int adv7441a_read_reg(struct __amba_vin_source *src, u8 regmap, u8 subaddr, u8 * pdata)
{
	int errCode = 0;
	struct adv7441a_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf[2];

	pinfo = (struct adv7441a_info *) src->pinfo;

	switch (regmap) {
	case USER_MAP:
		client = &pinfo->idc_usrmap;
		break;

	case USER_MAP_1:
		client = &pinfo->idc_usrmap1;
		break;

	case USER_MAP_2:
		client = &pinfo->idc_usrmap2;
		break;

	case VDP_MAP:
		client = &pinfo->idc_vdpmap;
		break;

	case HDMI_MAP:
		client = &pinfo->idc_hdmimap;
		break;

	case KSV_MAP:
		client = &pinfo->idc_rksvmap;
		break;

	case EDID_MAP:
		client = &pinfo->idc_edidmap;
		break;

	case RESERVED_MAP:
		client = &pinfo->idc_revmap;
		break;

	default:
		errCode = -EIO;
		goto adv7441a_read_reg_exit;
		break;
	}

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 1;
	msgs[0].buf = &subaddr;
	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].len = 2;
	msgs[1].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 2);
	if (errCode != 2) {
		DRV_PRINT("adv7441a_read_reg(error) %d [0x%x]\n", errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = pbuf[0];
		errCode = 0;
	}

      adv7441a_read_reg_exit:
	return errCode;
}

static int adv7441a_write_reset(struct __amba_vin_source *src)
{
	int errCode = 0;
	struct adv7441a_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[2];

	pinfo = (struct adv7441a_info *) src->pinfo;
	client = &pinfo->idc_usrmap;

	pbuf[0] = ADV7441A_REG_PWR;
	pbuf[1] = 0x80;

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags | I2C_M_IGNORE_NAK;
	msgs[0].len = 2;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		DRV_PRINT("adv7441a_write_reset(error) %d [0x%x:0x%x]\n", errCode, pbuf[0], pbuf[1]);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static void adv7441a_print_info(struct __amba_vin_source *src)
{
#ifdef CONFIG_AMBARELLA_VIN_DEBUG
	struct adv7441a_src_info		*psrcinfo;

	psrcinfo = (struct adv7441a_src_info *)src->psrcinfo;
#endif

	vin_dbg("cap_src_id = %d\n", psrcinfo->cap_src_id);
	vin_dbg("cap_vin_mode = %d\n", psrcinfo->cap_vin_mode);
	vin_dbg("cap_start_x = %d\n", psrcinfo->cap_start_x);
	vin_dbg("cap_start_y = %d\n", psrcinfo->cap_start_y);
	vin_dbg("cap_cap_w = %d\n", psrcinfo->cap_cap_w);
	vin_dbg("cap_cap_h = %d\n", psrcinfo->cap_cap_h);
	vin_dbg("sync_start = %d\n", psrcinfo->sync_start);
	vin_dbg("video_system = %d\n", psrcinfo->video_system);
	vin_dbg("frame_rate = %d\n", psrcinfo->frame_rate);
	vin_dbg("aspect_ratio = %d\n", psrcinfo->aspect_ratio);
	vin_dbg("input_type = %d\n", psrcinfo->input_type);
	vin_dbg("video_format = %d\n", psrcinfo->video_format);
	vin_dbg("bit_resolution = %d\n", psrcinfo->bit_resolution);
	vin_dbg("input_format = %d\n", psrcinfo->input_format);

	vin_dbg("line_width = %d\n", psrcinfo->line_width);
	vin_dbg("f0_height = %d\n", psrcinfo->f0_height);
	vin_dbg("f1_height = %d\n", psrcinfo->f1_height);
	vin_dbg("hs_front_porch = %d\n", psrcinfo->hs_front_porch);
	vin_dbg("hs_plus_width = %d\n", psrcinfo->hs_plus_width);
	vin_dbg("hs_back_porch = %d\n", psrcinfo->hs_back_porch);
	vin_dbg("f0_vs_front_porch = %d\n", psrcinfo->f0_vs_front_porch);
	vin_dbg("f0_vs_plus_width = %d\n", psrcinfo->f0_vs_plus_width);
	vin_dbg("f0_vs_back_porch = %d\n", psrcinfo->f0_vs_back_porch);
	vin_dbg("f1_vs_front_porch = %d\n", psrcinfo->f1_vs_front_porch);
	vin_dbg("f1_vs_plus_width = %d\n", psrcinfo->f1_vs_plus_width);
	vin_dbg("f1_vs_back_porch = %d\n", psrcinfo->f1_vs_back_porch);

	vin_dbg("fix_reg_table = 0x%x\n", (u32) (psrcinfo->fix_reg_table));
	vin_dbg("so_freq_hz = %d\n", psrcinfo->so_freq_hz);
	vin_dbg("so_pclk_freq_hz = %d\n", psrcinfo->so_pclk_freq_hz);
}

/* ========================================================================== */
#include "arch/adv7441a_arch.c"
#include "adv7441a_docmd.c"

/* ========================================================================== */
static int adv7441a_set_i2c_client(struct i2c_client *client,
	struct i2c_client *_client, struct adv7441a_info *pinfo)
{
	u32				addr;

	i2c_set_clientdata(_client, pinfo);
	addr = client->addr;
	memcpy(client, _client, sizeof(*client));
	client->addr = addr;
	strlcpy(client->name, adv7441a_name, sizeof (client->name));

	return 0;
}

static int adv7441a_probe(struct i2c_client *client,
	const struct i2c_device_id *_id)
{
	int					errCode = 0;
	int					i;
	u32					edid_size;
	u8					id;
	struct adv7441a_info			*pinfo;
	struct __amba_vin_source		*src = NULL;
	const struct firmware			*pedid;

	/* Platform Info */
	pinfo = kzalloc(sizeof (struct adv7441a_info), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto adv7441a_probe_exit;
	}

	/* EDID Data */
	if (edid_data != NULL) {
		errCode = request_firmware(&pedid, edid_data,
							&client->adapter->dev);
		if (errCode) {
			vin_err("Can't load EDID from %s!\n", edid_data);
			goto adv7441a_free_pinfo;
		}
		vin_notice("Load %s, size = %d\n", edid_data, pedid->size);
		if (pedid->size > ADV7441A_EDID_TABLE_SIZE)
			edid_size = ADV7441A_EDID_TABLE_SIZE;
		else
			edid_size = pedid->size;
		memcpy(pinfo->edid, pedid->data, edid_size);
		pinfo->valid_edid = 1;
	}

	/* Vin Source */
	for (i = 0; i < ADV7441A_SOURCE_NUM; i++) {
		pinfo->active_vin_src = &pinfo->sources[i];
		pinfo->active_vin_src->cap_vin_mode = AMBA_VIDEO_MODE_AUTO;
		pinfo->active_vin_src->cap_src_id = i;
		pinfo->active_vin_src->so_freq_hz = PLL_CLK_27MHZ;
		pinfo->active_vin_src->so_pclk_freq_hz = PLL_CLK_27MHZ;

		src = &pinfo->active_vin_src->cap_src;
		src->id = -1;
		src->adapid = adapter_id;
		src->dev_type = AMBA_VIN_SRC_DEV_TYPE_DECODER;
		snprintf(src->name, sizeof (src->name), "%s:%d<%s>", adv7441a_name, i, adv7441a_source_table[i].name);
		src->owner = THIS_MODULE;
		src->pinfo = pinfo;
		src->psrcinfo = pinfo->active_vin_src;
		src->docmd = adv7441a_docmd;
		src->active_channel_id = 0;
		src->total_channel_num = 1;

		errCode = amba_vin_add_source(src);
		if (errCode)
			goto adv7441a_del_src;
	}

	/* Hardware Pre-Initialization */
	errCode = adv7441a_init_vin_clock(src);
	if (errCode)
		goto adv7441a_del_src;
	msleep(10);

	/* I2c Client */
	pinfo->idc_usrmap.addr = adv7441a_addr;
	errCode = adv7441a_set_i2c_client(&pinfo->idc_usrmap, client, pinfo);
	if (errCode)
		goto adv7441a_del_src;

	errCode = adv7441a_update_idc_usrmap2(src);
	if (errCode)
		goto adv7441a_del_src;
	errCode = adv7441a_set_i2c_client(&pinfo->idc_usrmap2, client, pinfo);
	if (errCode)
		goto adv7441a_del_src;

	errCode = adv7441a_update_idc_maps(src);
	if (errCode)
		goto adv7441a_del_src;
	errCode = adv7441a_set_i2c_client(&pinfo->idc_usrmap1, client, pinfo);
	if (errCode)
		goto adv7441a_del_src;
	errCode = adv7441a_set_i2c_client(&pinfo->idc_vdpmap, client, pinfo);
	if (errCode)
		goto adv7441a_del_src;
	errCode = adv7441a_set_i2c_client(&pinfo->idc_hdmimap, client, pinfo);
	if (errCode)
		goto adv7441a_del_src;
	errCode = adv7441a_set_i2c_client(&pinfo->idc_rksvmap, client, pinfo);
	if (errCode)
		goto adv7441a_del_src;
	errCode = adv7441a_set_i2c_client(&pinfo->idc_edidmap, client, pinfo);
	if (errCode)
		goto adv7441a_del_src;
	errCode = adv7441a_set_i2c_client(&pinfo->idc_revmap, client, pinfo);
	if (errCode)
		goto adv7441a_del_src;

	/* Hardware Post-Initialization */
	adv7441a_reset(src);

	errCode = adv7441a_query_decoder_idrev(src, &id);
	if (errCode)
		goto adv7441a_del_src;
	adv7441a_select_source(src);
	vin_notice("%s:%d probed %d!\n", adv7441a_name, id, ADV7441A_SOURCE_NUM);

	goto adv7441a_probe_exit;

adv7441a_del_src:
	for (i = 0; i < ADV7441A_SOURCE_NUM; i++) {
		pinfo->active_vin_src = &pinfo->sources[i];
		src = &pinfo->active_vin_src->cap_src;
		amba_vin_del_source(src);
	}

adv7441a_free_pinfo:
	kfree(pinfo);

adv7441a_probe_exit:
	return errCode;

}

static int adv7441a_remove(struct i2c_client *client)
{
	int					errCode = 0;
	int					i;
	struct adv7441a_info			*pinfo;
	struct __amba_vin_source		*src;

	pinfo = (struct adv7441a_info *)i2c_get_clientdata(client);

	if (pinfo) {
		i2c_set_clientdata(client, NULL);
		for (i = 0; i < ADV7441A_SOURCE_NUM; i++) {
			pinfo->active_vin_src = &pinfo->sources[i];
			src = &pinfo->active_vin_src->cap_src;
			amba_vin_del_source(src);
		}
		kfree(pinfo);
	} else {
		vin_err("%s: pinfo of i2c client not found!\n", __func__);
	}

	vin_notice("%s removed!\n", adv7441a_name);

	return errCode;
}

static struct i2c_device_id adv7441a_idtable[] = {
	{ "amb_vin0", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, adv7441a_idtable);

static struct i2c_driver i2c_driver_adv7441a = {
	.driver = {
		   .name = "amb_vin0",
		   },
	.id_table	= adv7441a_idtable,
	.probe		= adv7441a_probe,
	.remove		= adv7441a_remove,
};

static int __init adv7441a_init(void)
{
	return i2c_add_driver(&i2c_driver_adv7441a);
}

static void __exit adv7441a_exit(void)
{
	i2c_del_driver(&i2c_driver_adv7441a);
}

module_init(adv7441a_init);
module_exit(adv7441a_exit);

MODULE_DESCRIPTION("ADV7441A video decoder");
MODULE_AUTHOR("Qiao Wang, <qwang@ambarella.com>");
MODULE_LICENSE("Proprietary");
