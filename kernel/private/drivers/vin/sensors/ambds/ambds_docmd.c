/*
 * kernel/private/drivers/ambarella/vin/sensors/ambds/ambds_docmd.c
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

static void ambds_dump_reg(struct __amba_vin_source *src)
{
	u32 i;
	u16 reg_to_dump_init[] = {};

	for(i=0; i<sizeof(reg_to_dump_init)/sizeof(reg_to_dump_init[0]); i++) {
		u16 reg_addr = reg_to_dump_init[i];
		u8 reg_val;

		ambds_read_reg(src, reg_addr, &reg_val);
		vin_dbg("REG= 0x%04X, 0x%04X\n", reg_addr, reg_val);
	}

}

static void ambds_fill_video_format_regs(struct __amba_vin_source *src)
{
	int			i;
	u32			index;
	u32			format_index;
	struct ambds_info	*pinfo;

	pinfo = (struct ambds_info *)src->pinfo;

	vin_dbg("ambds_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = ambds_video_info_table[index].format_index;

	for (i = 0; i < AMBDS_VIDEO_FORMAT_REG_NUM; i++) {
		if (ambds_video_format_tbl.reg[i] == 0)
			break;

		ambds_write_reg(src,
			ambds_video_format_tbl.reg[i],
			ambds_video_format_tbl.table[format_index].data[i]);
	}

	if (ambds_video_format_tbl.table[format_index].ext_reg_fill)
		ambds_video_format_tbl.table[format_index].ext_reg_fill(src);

}

static void ambds_fill_share_regs(struct __amba_vin_source *src)
{
	int i;
	const struct ambds_reg_table	*reg_tbl;

	reg_tbl = ambds_share_regs;
	vin_dbg("ambds fill share regs\n");
	for (i = 0; i < AMBDS_SHARE_REG_SIZE; i++) {
		ambds_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void ambds_sw_reset(struct __amba_vin_source *src)
{
	msleep(30);
}

static void ambds_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	ambds_sw_reset(src);
}

static void ambds_fill_pll_regs(struct __amba_vin_source *src)
{
	int i = 0;
	struct ambds_info *pinfo;
	const struct ambds_pll_reg_table 	*pll_reg_table;
	const struct ambds_reg_table 		*pll_tbl;

	pinfo = (struct ambds_info *) src->pinfo;

	vin_dbg("ambds_fill_pll_regs\n");
	pll_reg_table = &ambds_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < AMBDS_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			ambds_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
}

static int ambds_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct ambds_info  *pinfo;

	pinfo = (struct ambds_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int ambds_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errCode = 0;
	u32 index;
	u32 format_index;

	struct ambds_info *pinfo;

	pinfo = (struct ambds_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= AMBDS_VIDEO_INFO_TABLE_SIZE) {
		p_video_info->width = 0;
		p_video_info->height = 0;
		p_video_info->fps = 0;
		p_video_info->format = 0;
		p_video_info->type = 0;
		p_video_info->bits = 0;
		p_video_info->ratio = 0;
		p_video_info->system = 0;
		p_video_info->rev = 0;
		p_video_info->pattern= 0;

		errCode = -EPERM;
	} else {
		format_index = ambds_video_info_table[index].format_index;
		p_video_info->width = ambds_video_info_table[index].def_width;
		p_video_info->height = ambds_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = ambds_video_format_tbl.table[format_index].format;
		p_video_info->type = ambds_video_format_tbl.table[format_index].type;
		p_video_info->bits = ambds_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = ambds_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
	}

	return errCode;
}

static int ambds_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct ambds_info *pinfo;

	pinfo = (struct ambds_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}

static int ambds_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));
	return 0;
}

static int ambds_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;
	struct ambds_info *pinfo;

	pinfo = (struct ambds_info *) src->pinfo;
	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < AMBDS_VIDEO_MODE_TABLE_SIZE; i++) {
		if (ambds_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = AMBDS_VIDEO_MODE_TABLE_AUTO;

			p_mode_info->is_supported = 1;

			index = ambds_video_mode_table[i].still_index;
			format_index = ambds_video_info_table[index].format_index;

			p_mode_info->video_info.width = ambds_video_info_table[index].def_width;
			p_mode_info->video_info.height = ambds_video_info_table[index].def_height;
			p_mode_info->video_info.fps = ambds_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = ambds_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = ambds_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = ambds_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = ambds_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;

			break;
		}
	}

	return errCode;
}


static void ambds_start_streaming(struct __amba_vin_source *src)
{
}

static int ambds_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct ambds_info *pinfo;
	u32 format_index;

	pinfo = (struct ambds_info *) src->pinfo;

	if (index >= AMBDS_VIDEO_INFO_TABLE_SIZE) {
		vin_err("ambds_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto ambds_set_mode_exit;
	}

	errCode |= ambds_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = ambds_video_info_table[index].format_index;

	pinfo->cap_start_x = ambds_video_info_table[index].def_start_x;
	pinfo->cap_start_y = ambds_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = ambds_video_info_table[index].def_width;
	pinfo->cap_cap_h = ambds_video_info_table[index].def_height;
	pinfo->slvs_eav_col = ambds_video_format_tbl.table[format_index].slvs_eav_col;
	pinfo->slvs_sav2sav_dist = ambds_video_format_tbl.table[format_index].slvs_sav2sav_dist;
	//set the specified PLL index for each sensor_mode.
	pinfo->pll_index = ambds_video_format_tbl.table[format_index].pll_index;
	ambds_print_info(src);

	//set clk_si
	errCode |= ambds_init_vin_clock(src, &ambds_pll_tbl[pinfo->pll_index]);
	errCode |= ambds_set_vin_mode(src);

	ambds_fill_pll_regs(src);
	ambds_fill_share_regs(src);
	ambds_fill_video_format_regs(src);

	errCode |= ambds_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

	ambds_start_streaming(src);

ambds_set_mode_exit:
	return errCode;
}

static int ambds_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errCode = -EINVAL;
	int i;
	struct ambds_info *pinfo;
	int errorCode = 0;
	static int 				first_set_video_mode = 1;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct ambds_info *) src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errorCode = ambds_init_vin_clock(src, &ambds_pll_tbl[pinfo->pll_index]);
		if (errorCode)
			goto ambds_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	ambds_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	for (i = 0; i < AMBDS_VIDEO_MODE_TABLE_SIZE; i++) {
		if (ambds_video_mode_table[i].mode == mode) {
			errCode = ambds_set_video_index(src, ambds_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}

	if (i >= AMBDS_VIDEO_MODE_TABLE_SIZE) {
		vin_err("ambds_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = ambds_video_mode_table[i].mode;
		pinfo->mode_type = ambds_video_mode_table[i].preview_mode_type;
	}

ambds_set_video_mode_exit:
	return errCode;
}

static int ambds_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	struct ambds_info		*pinfo;

	pinfo = (struct ambds_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = ambds_init_vin_clock(src, &ambds_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto ambds_init_hw_exit;

	msleep(10);
	ambds_reset(src);

ambds_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int ambds_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct ambds_info		*pinfo;

	pinfo = (struct ambds_info *) src->pinfo;

	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		ambds_reset(src);
		break;

	case AMBA_VIN_SRC_SET_POWER:
		ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_power, (u32)args);
		break;

	case AMBA_VIN_SRC_SUSPEND:
		if (!adapter_id) {
			ambarella_set_gpio_output(
				&ambarella_board_generic.vin0_reset, 1);
			ambarella_set_gpio_output(
				&ambarella_board_generic.vin0_power, 0);
		} else {
			ambarella_set_gpio_output(
				&ambarella_board_generic.vin1_reset, 1);
			ambarella_set_gpio_output(
				&ambarella_board_generic.vin1_power, 0);
		}
		break;

	case AMBA_VIN_SRC_RESUME:
		errCode = ambds_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO: {
		struct amba_vin_source_info *pub_src;

		pub_src = (struct amba_vin_source_info *) args;
		pub_src->id = src->id;
		pub_src->sensor_id = SENSOR_AMBDS;
		pub_src->adapter_id = adapter_id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = ambds_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = ambds_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = ambds_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = ambds_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = ambds_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = ambds_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = ambds_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_BLC:
		errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_GET_SW_BLC, (void *) args);
		break;

	case AMBA_VIN_SRC_SET_BLC:
		errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_SW_BLC, (void *) args);
		break;

	case AMBA_VIN_SRC_GET_FRAME_RATE:
		*(u32 *)args = pinfo->frame_rate;
		break;

	case AMBA_VIN_SRC_SET_FRAME_RATE:
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(s32*)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		ambds_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA: {
		struct amba_vin_test_reg_data *reg_data;
		u16 subaddr;
		u8 data = 0;

		reg_data = (struct amba_vin_test_reg_data *) args;
		subaddr = reg_data->reg;

		errCode = ambds_read_reg(src, subaddr, &data);

		reg_data->data = data;
		}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA: {
		struct amba_vin_test_reg_data *reg_data;
		u16 subaddr;
		u8 data = 0;

		reg_data = (struct amba_vin_test_reg_data *) args;
		subaddr = reg_data->reg;
		data = reg_data->data;

		errCode = ambds_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
