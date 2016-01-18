/*
 * kernel/private/drivers/ambarella/vin/sensors/micron_mt9p001/mt9p001_docmd.c
 *
 * History:
 *    2008/01/18 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

static int mt9p001_set_fps(struct __amba_vin_source *src, u32 fps);
static int mt9p001_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time);
static int mt9p001_set_agc_db(struct __amba_vin_source *src, s32 agc_db);

static void mt9p001_dump_reg(struct __amba_vin_source *src)
{
	int i;
	u16 data;

	vin_dbg("mt9p001_dump_reg:\n");
	for (i = 0; i < 256; i++) {
		mt9p001_read_reg(src, i, &data);
		vin_dbg("0x%x=0x%x \n", i, data);
	}
}

static int mt9p001_check_reg(struct __amba_vin_source *src, const struct mt9p001_reg_table *reg_tbl)
{
	int retVal = 0;
	u16 data;

	vin_dbg("mt9p001_check_reg: 0x%x\n", reg_tbl->reg);
	mt9p001_read_reg(src, reg_tbl->reg, &data);

	if (data == reg_tbl->data) {
		vin_dbg("0x%x == 0x%x\n", reg_tbl->reg, data);
	} else {
		vin_dbg("0x%x != 0x%x\n", reg_tbl->reg, data);
		retVal = -1;
	}

	return retVal;
}

static void mt9p001_fill_video_format_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 format_index;
	struct mt9p001_info *pinfo;

	pinfo = (struct mt9p001_info *) src->pinfo;

	index = pinfo->current_video_index;
	format_index = mt9p001_video_info_table[index].format_index;

	for (i = 0; i < MT9P001_VIDEO_FORMAT_REG_NUM; i++) {
		if (mt9p001_video_format_tbl.reg[i] == 0)
			break;

		if (mt9p001_video_format_tbl.reg[i] == MT9P001_READ_MODE_1) {
			mt9p001_write_reg(src,
					  mt9p001_video_format_tbl.reg[i],
					  (mt9p001_video_format_tbl.table[format_index].data[i] | pinfo->read_mode_1));
		} else {
			mt9p001_write_reg(src,
					  mt9p001_video_format_tbl.reg[i],
					  mt9p001_video_format_tbl.table[format_index].data[i]);
		}
	}

	if (mt9p001_video_format_tbl.table[format_index].ext_reg_fill)
		mt9p001_video_format_tbl.table[format_index].ext_reg_fill(src);

}

static void mt9p001_fill_share_regs(struct __amba_vin_source *src)
{
	int i;
	const struct mt9p001_reg_table *reg_tbl;
	struct mt9p001_info *pinfo;

	pinfo = (struct mt9p001_info *) src->pinfo;
	reg_tbl = mt9pxxx_share_regs;

	for (i = 0; i < MT9P001_SHARE_REG_SZIE; i++) {
		if (reg_tbl[i].reg == MT9P001_SHR_WIDTH_UPPER)
			pinfo->old_shr_width_upper = reg_tbl[i].data;

		if (reg_tbl[i].reg == MT9P001_RESTART) {
			mt9p001_write_reg(src, reg_tbl[i].reg, (reg_tbl[i].data | pinfo->restart));
		} else {
			mt9p001_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
		}
	}
}

static void mt9p001_fill_video_fps_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 fps_index;
	u32 format_index;
	struct mt9p001_info *pinfo;
	const struct mt9p001_reg_table *pll_tbl;
	const struct mt9p001_video_fps_reg_table *fps_table;

	pinfo = (struct mt9p001_info *) src->pinfo;

	index = pinfo->current_video_index;
	format_index = mt9p001_video_info_table[index].format_index;
	fps_index = pinfo->fps_index;
	fps_table = mt9p001_video_format_tbl.table[format_index].fps_table;
	pll_tbl = fps_table->table[fps_index].pll_reg_table->regs;

	if (mt9p001_check_reg(src, &pll_tbl[0]) ||
	    mt9p001_check_reg(src, &pll_tbl[1]) || mt9p001_check_reg(src, &pll_tbl[2])) {
		mt9p001_write_reg(src, pll_tbl[0].reg, (pll_tbl[0].data & (~MT9P001_USE_PLL_REG_BIT)));
		mt9p001_write_reg(src, pll_tbl[1].reg, pll_tbl[1].data);
		mt9p001_write_reg(src, pll_tbl[2].reg, pll_tbl[2].data);
		if ((pll_tbl[0].data & MT9P001_USE_PLL_REG_BIT)) {
			msleep(2);
			mt9p001_write_reg(src, pll_tbl[0].reg, pll_tbl[0].data);
			msleep(100);
		}
	}

	for (i = 0; i < MT9P001_VIDEO_FPS_REG_NUM; i++) {
		mt9p001_write_reg(src, fps_table->reg[i], fps_table->table[fps_index].data[i]);
	}
}

static void mt9p001_sw_reset(struct __amba_vin_source *src)
{
	mt9p001_write_reg(src, MT9P001_RESET, 0x0001);
	msleep(1);
	mt9p001_write_reg(src, MT9P001_RESET, 0x0000);
	msleep(2);
}

static void mt9p001_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	mt9p001_sw_reset(src);
}

static int mt9p001_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errorCode = 0;
	u32 index;
	u32 fps_index;
	u32 format_index;
	const struct mt9p001_video_fps_reg_table *fps_table;
	struct mt9p001_info *pinfo;

	pinfo = (struct mt9p001_info *) src->pinfo;

	index = pinfo->current_video_index;

	if (index >= MT9P001_VIDEO_INFO_TABLE_SZIE) {
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

		errorCode = -EPERM;
	} else {
		format_index = mt9p001_video_info_table[index].format_index;
		fps_index = pinfo->fps_index;
		fps_table = mt9p001_video_format_tbl.table[format_index].fps_table;

		p_video_info->width = pinfo->cap_cap_w;
		p_video_info->height = pinfo->cap_cap_h;
		p_video_info->fps = fps_table->table[fps_index].fps;
		p_video_info->format = mt9p001_video_format_tbl.table[format_index].format;
		p_video_info->type = mt9p001_video_format_tbl.table[format_index].type;
		p_video_info->bits = mt9p001_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = mt9p001_video_format_tbl.table[format_index].ratio;
		p_video_info->system = fps_table->table[fps_index].system;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errorCode;
}

static int mt9p001_check_vidoe_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errorCode = 0;
	int i, j;
	struct mt9p001_info *pinfo;
	u32 index;
	u32 fps_index;
	u32 format_index;
	const struct mt9p001_video_fps_reg_table *fps_table;

	pinfo = (struct mt9p001_info *) src->pinfo;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < MT9P001_VIDEO_MODE_TABLE_SZIE; i++) {
		if (mt9p001_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = MT9P001_VIDEO_MODE_TABLE_AUTO;
			p_mode_info->is_supported = 1;

			index = mt9p001_video_mode_table[i].still_index;
			format_index = mt9p001_video_info_table[index].format_index;
			fps_index = mt9p001_video_info_table[index].fps_index;
			fps_table = mt9p001_video_format_tbl.table[format_index].fps_table;

			p_mode_info->video_info.width = mt9p001_video_info_table[index].def_width;
			p_mode_info->video_info.height = mt9p001_video_info_table[index].def_height;
			p_mode_info->video_info.fps = fps_table->table[fps_index].fps;
			p_mode_info->video_info.format = mt9p001_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = mt9p001_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = mt9p001_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = mt9p001_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = fps_table->table[fps_index].system;
			p_mode_info->video_info.rev = 0;

			for (j = 0; j < AMBA_VIN_MAX_FPS_TABLE_SIZE; j++) {
				if (fps_table->table[j].pll_reg_table == NULL)
					break;

				amba_vin_source_set_fps_flag(p_mode_info, fps_table->table[j].fps);
			}

			break;
		}
	}

	return errorCode;
}

static int mt9p001_query_snesor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	struct mt9p001_info *pinfo;

	pinfo = (struct mt9p001_info *) src->pinfo;

	return mt9p001_read_reg(src, MT9P001_CHIP_ID, ss_id);
}

static int mt9p001_query_snesor_version(struct __amba_vin_source *src, u16 * ver)
{
	int errorCode = 0;
	struct mt9p001_info *pinfo;
	u16 ss_ver;

	pinfo = (struct mt9p001_info *) src->pinfo;

	errorCode = mt9p001_read_reg(src, MT9P001_REG_F9, &ss_ver);
	if (errorCode)
		goto mt9p001_query_snesor_version_exit;

	ss_ver |= 0x0080;
	errorCode = mt9p001_write_reg(src, MT9P001_REG_F9, ss_ver);
	if (errorCode)
		goto mt9p001_query_snesor_version_exit;

	errorCode = mt9p001_read_reg(src, MT9P001_REG_FD, &ss_ver);
	if (errorCode)
		goto mt9p001_query_snesor_version_exit;

	ss_ver = ((ss_ver & 0x0FE0) >> 5);
	*ver = ss_ver;

      mt9p001_query_snesor_version_exit:
	return errorCode;
}

static int mt9p001_prepare_trigger(struct __amba_vin_source *src,
	enum amba_vin_trigger_source trigger_source,
	enum amba_vin_flash_level flash_level,
	enum amba_vin_flash_status flash_status, u16 mode_flag)
{
	int errorCode = 0;
	struct mt9p001_info *pinfo;
	u16 restart = 0;
	u16 read_mode_1 = 0;

	pinfo = (struct mt9p001_info *) src->pinfo;

	pinfo->trigger_source = trigger_source;
	pinfo->flash_status = flash_status;

	if (trigger_source == AMBA_VIN_TRIGGER_SOURCE_EXT) {
		restart |= MT9P001_TRIGGER;
	} else {	//AMBA_VIN_TRIGGER_SOURCE_GPIO
		if (ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_trigger, 0) < 0) {
			restart |= MT9P001_TRIGGER;
			if (ambarella_board_generic.vin0_trigger.active_level == 1)
				read_mode_1 |= MT9P001_INVERT_TRIGGER;
		} else {
			restart &= ~MT9P001_TRIGGER;
		}
	}

	if (flash_status == AMBA_VIN_FLASH_ON)
		read_mode_1 |= MT9P001_STROBE_ENABLE;

	if (flash_level == AMBA_VIN_FLASH_LEVEL_LOW)
		read_mode_1 |= MT9P001_STROBE_INVERT;

	pinfo->read_mode_1 = (read_mode_1 | mode_flag);
	pinfo->restart = restart;

	return errorCode;
}

#if 0
static int mt9p001_set_trigger_mode(struct __amba_vin_source *src, enum amba_vin_trigger_mode trigger_mode)
{
	int errorCode = 0;
	struct mt9p001_info *pinfo;
	u16 tmp_reg;

	pinfo = (struct mt9p001_info *) src->pinfo;

	if (pinfo->trigger_source == AMBA_VIN_TRIGGER_SOURCE_GPIO) {
		switch (trigger_mode) {
		case AMBA_VIN_TRIGGER_MODE_OFF:
			if ((pinfo->trigger_gpio >= 0) && (pinfo->trigger_gpio < 32 * GPIO_INSTANCES)) {
				AMBA_VIN_HW_TRIGGER(0);
			} else {
				errorCode |= mt9p001_read_reg(src, MT9P001_RESTART, &tmp_reg);
				tmp_reg &= ~MT9P001_TRIGGER;
				errorCode |= mt9p001_write_reg(src, MT9P001_RESTART, tmp_reg);
			}
			break;

		case AMBA_VIN_TRIGGER_MODE_ON:
			if ((pinfo->trigger_gpio >= 0) && (pinfo->trigger_gpio < 32 * GPIO_INSTANCES)) {
				AMBA_VIN_HW_TRIGGER(1);
			} else {
				errorCode |= mt9p001_read_reg(src, MT9P001_RESTART, &tmp_reg);
				tmp_reg |= MT9P001_TRIGGER;
				errorCode |= mt9p001_write_reg(src, MT9P001_RESTART, tmp_reg);
			}
			break;

		default:
			vin_err("mt9p001_set_trigger_mode do not support %d!\n", trigger_mode);
			errorCode = -EINVAL;
			break;;
		}
	}

	return errorCode;
}
#endif

static int mt9p001_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errorCode = 0;
	struct mt9p001_info *pinfo;
	u32 format_index;

	pinfo = (struct mt9p001_info *) src->pinfo;

	if (index >= MT9P001_VIDEO_INFO_TABLE_SZIE) {
		vin_err("mt9p001_set_video_index do not support mode %d!\n", index);
		errorCode = -EINVAL;
		goto mt9p001_set_video_index_exit;
	}

	errorCode |= mt9p001_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = mt9p001_video_info_table[index].format_index;

	pinfo->cap_start_x = mt9p001_video_info_table[index].def_start_x;
	pinfo->cap_start_y = mt9p001_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = mt9p001_video_info_table[index].def_width;
	pinfo->cap_cap_h = mt9p001_video_info_table[index].def_height;
	pinfo->bayer_pattern = mt9p001_video_format_tbl.table[format_index].bayer_pattern;
	pinfo->fps_index = mt9p001_video_info_table[index].fps_index;
	mt9p001_print_info(src);

	errorCode |= mt9p001_set_vin_mode(src);

	mt9p001_fill_share_regs(src);

	mt9p001_fill_video_fps_regs(src);

	mt9p001_fill_video_format_regs(src);

	mt9p001_set_fps(src, AMBA_VIDEO_FPS_AUTO);
	mt9p001_set_shutter_time(src, AMBA_VIDEO_FPS_60);	//the default fps = 60.
	mt9p001_set_agc_db(src, 0);

	errorCode |= mt9p001_write_reg(src, MT9P001_RESTART, MT9P001_RESTART_FRAME);

	errorCode |= mt9p001_post_set_vin_mode(src);

      mt9p001_set_video_index_exit:
	return errorCode;
}

static int mt9p001_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errorCode = -EINVAL;
	int i;
	struct mt9p001_info *pinfo;

	/* Hardware Initialization */
	errorCode = mt9p001_init_vin_clock(src, 0);
	if (errorCode)
		goto mt9p001_set_video_mode_exit;
	msleep(10);
	mt9p001_reset(src);

	pinfo = (struct mt9p001_info *) src->pinfo;

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->capture_mode = AMBA_VIN_CAPTURE_MODE_ERS_CONTINUOUS;

	errorCode = mt9p001_prepare_trigger(src, AMBA_VIN_TRIGGER_SOURCE_GPIO,
		AMBA_VIN_FLASH_LEVEL_HIGH, AMBA_VIN_FLASH_OFF, 0);
	if (errorCode)
		goto mt9p001_set_video_mode_exit;

	for (i = 0; i < MT9P001_VIDEO_MODE_TABLE_SZIE; i++) {
		if (mt9p001_video_mode_table[i].mode == mode) {
			errorCode = mt9p001_set_video_index(src, mt9p001_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= MT9P001_VIDEO_MODE_TABLE_SZIE) {
		vin_err("mt9p001_set_video_mode do not support %d, %d!\n", mode, i);
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errorCode) {
		pinfo->current_vin_mode = mt9p001_video_mode_table[i].mode;
		pinfo->mode_type = mt9p001_video_mode_table[i].preview_mode_type;
	}

mt9p001_set_video_mode_exit:
	return errorCode;
}

static int mt9p001_get_video_mode(struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errorCode = 0;
	struct mt9p001_info *pinfo;

	pinfo = (struct mt9p001_info *) src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errorCode;
}

static int mt9p001_set_capture_mode(struct __amba_vin_source *src, enum amba_vin_capture_mode cap_mode)
{
	int errorCode = 0;
	u16 tmp_read_mode1;
	u16 tmp_restart;
	struct mt9p001_info *pinfo;

	pinfo = (struct mt9p001_info *) src->pinfo;

	pinfo->capture_mode = cap_mode;

	errorCode = mt9p001_read_reg(src, MT9P001_READ_MODE_1, &tmp_read_mode1);
	if (errorCode)
		goto mt9p001_set_capture_mode_exit;
	tmp_read_mode1 &= ~(MT9P001_SNAPSHOT | MT9P001_GLOBAL_RESET | MT9P001_BULB_EXPOSURE);

	errorCode = mt9p001_read_reg(src, MT9P001_RESTART, &tmp_restart);
	if (errorCode)
		goto mt9p001_set_capture_mode_exit;

	switch (pinfo->capture_mode) {
	case AMBA_VIN_CAPTURE_MODE_ERS_CONTINUOUS:
		errorCode |= mt9p001_write_reg(src, MT9P001_REG_30, 0x0000);
		break;

	case AMBA_VIN_CAPTURE_MODE_ERS_SNAPSHOT:
		tmp_read_mode1 |= MT9P001_SNAPSHOT;
		errorCode |= mt9p001_write_reg(src, MT9P001_REG_30, 0x0000);
		break;

	case AMBA_VIN_CAPTURE_MODE_ERS_BULB:
		tmp_read_mode1 |= (MT9P001_SNAPSHOT | MT9P001_BULB_EXPOSURE);
		errorCode |= mt9p001_write_reg(src, MT9P001_REG_30, 0x0000);
		break;

	case AMBA_VIN_CAPTURE_MODE_GRR_SNAPSHOT:
		errorCode |= mt9p001_write_reg(src, MT9P001_REG_30, 0x0001);
		tmp_read_mode1 |= (MT9P001_SNAPSHOT | MT9P001_GLOBAL_RESET);
		break;

	case AMBA_VIN_CAPTURE_MODE_GRR_BULB:
		errorCode |= mt9p001_write_reg(src, MT9P001_REG_30, 0x0001);
		tmp_read_mode1 |= (MT9P001_SNAPSHOT | MT9P001_GLOBAL_RESET | MT9P001_BULB_EXPOSURE);
		break;

	default:
		break;;
	}

	errorCode = mt9p001_write_reg(src, MT9P001_READ_MODE_1, tmp_read_mode1);
	errorCode |= mt9p001_write_reg(src, MT9P001_RESTART, (tmp_restart | MT9P001_RESTART_FRAME));

mt9p001_set_capture_mode_exit:
	return errorCode;
}

static int mt9p001_set_low_ligth_mode(struct __amba_vin_source *src, int mode)
{
	int errorCode = 0;
	struct mt9p001_info *pinfo;
	u16 tmp_reg;
	u32 index;
	u32 fps_index;
	u32 format_index;
	const struct mt9p001_video_fps_reg_table *fps_table;

	pinfo = (struct mt9p001_info *) src->pinfo;

	index = pinfo->current_video_index;
	format_index = mt9p001_video_info_table[index].format_index;
	fps_index = pinfo->fps_index;
	fps_table = mt9p001_video_format_tbl.table[format_index].fps_table;

	if (mode == 1) {
		mt9p001_read_reg(src, MT9P001_READ_MODE_2, &tmp_reg);
		tmp_reg |= 0x0020;
		errorCode = mt9p001_write_reg(src, MT9P001_READ_MODE_2, tmp_reg);
	} else {
		mt9p001_read_reg(src, MT9P001_READ_MODE_2, &tmp_reg);
		tmp_reg &= (~0x0020);
		errorCode = mt9p001_write_reg(src, MT9P001_READ_MODE_2, tmp_reg);
	}

	pinfo->current_low_light_mode = mode;

	return errorCode;
}

static inline int mt9p001_ceil(int n, int base)
{
	int ret;

	ret = n / base;
	if ((n % base) != 0)
		ret++;

	return ret;
}

static int mt9p001_set_shr_width(struct __amba_vin_source *src, u32 shutter_width)
{
	int errorCode = 0;
	u16 shr_width_upper;
	u16 shr_width;
	struct mt9p001_info *pinfo;
	u16 tmp_reg = 0x1f82;

	pinfo = (struct mt9p001_info *) src->pinfo;

	shr_width = shutter_width & 0x0000FFFF;
	shr_width_upper = (shutter_width & 0xFFFF0000) >> 16;

	vin_dbg("shr_width_upper = %d,shr_width = %d\n", shr_width_upper, shr_width);

	if (unlikely(shr_width_upper != pinfo->old_shr_width_upper)) {
		errorCode |= mt9p001_read_reg(src, MT9P001_OUTPUT_CTRL, &tmp_reg);
		tmp_reg |= MT9P001_SYNCHRONIZE_CHANGES;
		errorCode |= mt9p001_write_reg(src, MT9P001_OUTPUT_CTRL, tmp_reg);

		errorCode |= mt9p001_write_reg(src, MT9P001_SHR_WIDTH_UPPER, shr_width_upper);
	}

	errorCode |= mt9p001_write_reg(src, MT9P001_SHR_WIDTH, shr_width);

	if (unlikely(shr_width_upper != pinfo->old_shr_width_upper)) {
		tmp_reg &= ~MT9P001_SYNCHRONIZE_CHANGES;
		errorCode |= mt9p001_write_reg(src, MT9P001_OUTPUT_CTRL, tmp_reg);
		pinfo->old_shr_width_upper = shr_width_upper;
	}

	return errorCode;
}

/**
 * Set AD gain of the selected color channel
 *
 * @Params color - MT9P001_GREEN1
 *		 - MT9P001_BLUE
 *		 - MT9P001_RED
 *		 - MT9P001_GREEN2
 *		 - MT9P001_GLOBAL
 * @Params gain - gain value to set
 */
static void mt9p001_set_gain_settings(struct __amba_vin_source *src, u16 color, u16 gain)
{
	u16 idc_reg = gain;
	u16 dgain = gain >> 8;
	u16 again = gain & 0x7f;

	vin_dbg("mt9p001_set_gain_settings = %d\n", idc_reg);
	if (dgain > 120) {
		vin_err("Digital gain = %d shall be less than 121\n", dgain);
	}

	if (again > 0x7f) {
		vin_err("Analog gain = %d shall be less than 128\n", again);
	}

	/* Set the gain by setting the register */
	if (color < MT9P001_GLOBAL) {
		/* Reg0x2B ~ 0x2E */
		mt9p001_write_reg(src, MT9P001_GREEN1_GAIN + color, idc_reg);
	} else {
		/* 0x35, Global Gain */
		mt9p001_write_reg(src, MT9P001_GLOBAL_GAIN, idc_reg);
	}
}

static int mt9p001_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	int errorCode = 0;
	u16 shutter_delay;
	u16 sensor_HB_reg;
	u16 sensor_VB_reg;
	u16 col_size;
	u16 row_addr_mode, col_addr_mode;
	u32 SO, SD, SD_max;
	u32 mode_index;
	u32 row_length;
	u32 HB, HB_min;
	u32 W, W_dc;
	u32 row_bin, col_bin, col_skip;
	u32 shutter_width;
	u64 exposure_time_q9;
	u32 fps_index;
	u32 format_index;
	const struct mt9p001_video_fps_reg_table *fps_table;
	struct mt9p001_info *pinfo;

	vin_dbg("mt9p001_set_shutter_time: 0x%x\n", shutter_time);

	pinfo = (struct mt9p001_info *) src->pinfo;
	pinfo->current_shutter_time = shutter_time;

	mode_index = pinfo->current_video_index;

	format_index = mt9p001_video_info_table[mode_index].format_index;
	fps_index = pinfo->fps_index;
	fps_table = mt9p001_video_format_tbl.table[format_index].fps_table;

	exposure_time_q9 = shutter_time;
	exposure_time_q9 *= fps_table->table[fps_index].pll_reg_table->pixclk;
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	mt9p001_read_reg(src, MT9P001_SHR_DELAY, &shutter_delay);
	mt9p001_read_reg(src, MT9P001_HORI_BLANKING, &sensor_HB_reg);
	mt9p001_read_reg(src, MT9P001_VERT_BLANKING, &sensor_VB_reg);
	mt9p001_read_reg(src, MT9P001_COL_SIZE, &col_size);
	mt9p001_read_reg(src, MT9P001_COL_ADDR_MODE, &row_addr_mode);
	mt9p001_read_reg(src, MT9P001_COL_ADDR_MODE, &col_addr_mode);

	row_bin = (row_addr_mode & (0x30)) >> 4;
	col_bin = (col_addr_mode & (0x30)) >> 4;
	col_skip = col_addr_mode & (0x07);

	HB = sensor_HB_reg + 1;
	W = 2 * mt9p001_ceil((col_size + 1), (2 * (col_skip + 1)));

	if (col_bin == 1)
		W_dc = 40;
	else if (col_bin == 3)
		W_dc = 20;
	else
		W_dc = 80;

#if defined(CONFIG_SENSOR_MT9P031)
	HB_min = 346 * (row_bin + 1) + 64 + W_dc / 2;
	row_length = 2 * max(W / 2 + max(HB, HB_min), (41 + 346 * (row_bin + 1) + 99));

	SD = shutter_delay + 1;
	SD_max = 1604;
	SO = 346 * (row_bin + 1) + 98 + min(SD, SD_max) + 166;
	shutter_width = exposure_time_q9 + SO * 2;
	shutter_width /= row_length;
	if (unlikely(shutter_width < 3)) {
		SD_max = 1315;
		SO = 346 * (row_bin + 1) + 98 + min(SD, SD_max) + 166;
		shutter_width = exposure_time_q9 + SO * 2;
		shutter_width /= row_length;
	}
#elif defined(CONFIG_SENSOR_MT9P401)
	HB_min = 346 * (row_bin + 1) + 64 + W_dc / 2;

	if (format_index == 10) {	//1280x720 Bin=1, Skip=1, P60
		row_length = 2 * max(W / 2 + max(HB, HB_min), (41 + 186 * (row_bin + 1) + 99));
	} else {
		row_length = 2 * max(W / 2 + max(HB, HB_min), (41 + 346 * (row_bin + 1) + 99));
	}

	SD = shutter_delay + 1;
	SD_max = 1504;
	SO = 208 * (row_bin + 1) + 98 + min(SD, SD_max) - 94;
	shutter_width = exposure_time_q9 + SO * 2;
	shutter_width /= row_length;
	if (unlikely(shutter_width < 3)) {
		SD_max = 1232;
		SO = 208 * (row_bin + 1) + 98 + min(SD, SD_max) - 94;
		shutter_width = exposure_time_q9 + SO * 2;
		shutter_width /= row_length;
	}
#else
	HB_min = 208 * (row_bin + 1) + 64 + W_dc / 2;
	row_length = 2 * max(W / 2 + max(HB, HB_min), (41 + 208 * (row_bin + 1) + 99));

	SD = shutter_delay + 1;
	SD_max = 1504;
	SO = 208 * (row_bin + 1) + 98 + min(SD, SD_max) - 94;
	shutter_width = exposure_time_q9 + SO * 2;
	shutter_width /= row_length;
	if (unlikely(shutter_width < 3)) {
		SD_max = 1232;
		SO = 208 * (row_bin + 1) + 98 + min(SD, SD_max) - 94;
		shutter_width = exposure_time_q9 + SO * 2;
		shutter_width /= row_length;
	}
#endif
	if (shutter_width > 3)
		shutter_width -= 3;

	if ((pinfo->capture_mode == AMBA_VIN_CAPTURE_MODE_GRR_SNAPSHOT) ||
	    (pinfo->capture_mode == AMBA_VIN_CAPTURE_MODE_GRR_BULB)) {
		shutter_width = exposure_time_q9;
		shutter_width /= row_length;
		shutter_width += sensor_VB_reg;
		vin_dbg("exposure_time_q9 = %lld\n", exposure_time_q9);
		vin_dbg("row_length = %d\n", row_length);
		vin_dbg("sensor_VB_reg = %d\n", sensor_VB_reg);
	}

	errorCode = mt9p001_set_shr_width(src, shutter_width);

	vin_dbg("shutter_time = 0x%x, shutter_width = 0x%x\n", shutter_time, shutter_width);
	return errorCode;
}

static int mt9p001_get_shutter_time(struct __amba_vin_source *src, u32 *pShutter_time)
{
	struct mt9p001_info *pinfo;

	pinfo = (struct mt9p001_info *) src->pinfo;
	*pShutter_time = pinfo->current_shutter_time;

	return 0;
}

static int mt9p001_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	int errorCode = 0;
	u16 idc_reg;
	struct mt9p001_info *pinfo;
	u32 gain_index;
	s32 db_max;
	s32 db_step;

	vin_dbg("mt9p001_set_agc_db: 0x%x\n", agc_db);

	pinfo = (struct mt9p001_info *) src->pinfo;

	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	gain_index = (db_max - agc_db) / db_step;
//	DRV_PRINT("gain_index 0x%x, %d\n", agc_db, gain_index);

	if (gain_index >= MT9P001_GAIN_ROWS) {
		vin_err("index of gain table out of range!\n");
		errorCode = -EINVAL;
		goto mt9p001_set_agc_exit;
	}

	idc_reg = MT9PXXX_GLOBAL_GAIN_TABLE[gain_index][MT9P001_GAIN_COL_REG];

	//For Micron sensor defect, Blue strap,under bright light condition
	if (gain_index > (MT9P001_GAIN_0DB - MT9P001_GAIN_DOUBLE)) {
		mt9p001_write_reg(src, 0x7f, 0x0000);
	} else {
		mt9p001_write_reg(src, 0x7f, 0xa900);
	}

	mt9p001_set_gain_settings(src, MT9P001_GLOBAL, idc_reg);

	pinfo->current_gain_db = agc_db;

mt9p001_set_agc_exit:
	return errorCode;
}

static int mt9p001_get_agc_db(struct __amba_vin_source *src, s32 *agc_db)
{
	struct mt9p001_info *pinfo;

	pinfo = (struct mt9p001_info *) src->pinfo;
	*agc_db = pinfo->current_gain_db;

	return 0;
}

static int mt9p001_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int errorCode = 0;
	struct mt9p001_info *pinfo;
	u32 i;
	u32 index;
	u32 fps_index = -1;
	u32 format_index;
	const struct mt9p001_video_fps_reg_table *fps_table;

	vin_dbg("mt9p001_set_fps: %d", fps);

	pinfo = (struct mt9p001_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= MT9P001_VIDEO_INFO_TABLE_SZIE) {
		vin_err("mt9p001_set_fps index = %d!\n", index);
		errorCode = -EPERM;
		goto mt9p001_set_fps_exit;
	}

	format_index = mt9p001_video_info_table[index].format_index;
	fps_table = mt9p001_video_format_tbl.table[format_index].fps_table;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps_index = 0;

	for (i = 0; i < AMBA_VIN_MAX_FPS_TABLE_SIZE; i++) {
		if (fps_table->table[i].pll_reg_table == NULL)
			break;

		if (fps_table->table[i].fps == fps) {
			fps_index = i;
			break;
		}
	}

	if (fps_index != -1) {
		pinfo->fps_index = fps_index;
		pinfo->frame_rate = fps_table->table[fps_index].fps;
		mt9p001_print_info(src);

		mt9p001_fill_video_fps_regs(src);

		errorCode |= mt9p001_init_vin_clock(src, index);
		errorCode |= mt9p001_set_shutter_time(src, fps_table->table[fps_index].eshutter_limit);
		msleep(10);
		errorCode |= mt9p001_write_reg(src, MT9P001_RESTART, MT9P001_RESTART_FRAME);
		msleep(50);
	} else
		errorCode = -EINVAL;

	mt9p001_print_info(src);

      mt9p001_set_fps_exit:
	return errorCode;
}

static int mt9p001_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errorCode = 0;
	struct mt9p001_info *pinfo;
//	u16 tmp_reg;
	int i;
	u32 index;
	u32 fps_index;
	u32 format_index;
	const struct mt9p001_video_fps_reg_table *fps_table;

	pinfo = (struct mt9p001_info *) src->pinfo;

	index = pinfo->current_video_index;
	format_index = mt9p001_video_info_table[index].format_index;
	fps_index = pinfo->fps_index;
	fps_table = mt9p001_video_format_tbl.table[format_index].fps_table;

	if (mode > 0) {
//		mt9p001_read_reg(src, MT9P001_OUTPUT_CTRL, &tmp_reg);
//		tmp_reg |= MT9P001_SYNCHRONIZE_CHANGES;
//		mt9p001_write_reg(src, MT9P001_OUTPUT_CTRL, tmp_reg);
		for (i = 0; i < MT9P001_VIDEO_FPS_REG_NUM; i++) {
			if (fps_table->table[fps_index].slow_shutter[i] != 0) {
				mt9p001_write_reg(src, fps_table->reg[i], fps_table->table[fps_index].slow_shutter[i]);
			} else {
				errorCode = -EINVAL;
				vin_err("%dfps doesn't support slow_shutter.\n", fps_table->table[fps_index].fps);
				break;
			}
		}

//		tmp_reg &= ~MT9P001_SYNCHRONIZE_CHANGES;
//		mt9p001_write_reg(src, MT9P001_OUTPUT_CTRL, tmp_reg);
	} else {
//		mt9p001_read_reg(src, MT9P001_OUTPUT_CTRL, &tmp_reg);
//		tmp_reg |= MT9P001_SYNCHRONIZE_CHANGES;
//		mt9p001_write_reg(src, MT9P001_OUTPUT_CTRL, tmp_reg);
		for (i = 0; i < MT9P001_VIDEO_FPS_REG_NUM; i++) {

			mt9p001_write_reg(src, fps_table->reg[i], fps_table->table[fps_index].data[i]);
		}
//		tmp_reg &= ~MT9P001_SYNCHRONIZE_CHANGES;
//		mt9p001_write_reg(src, MT9P001_OUTPUT_CTRL, tmp_reg);
	}
	mt9p001_set_shutter_time(src, pinfo->current_shutter_time); //must update shutter width after HB and VB has changed

	pinfo->current_slowshutter_mode = mode;

	return errorCode;
}

static int mt9p001_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *still_info)
{
	int errorCode = -EINVAL;
	struct mt9p001_info *pinfo;
	int i;

	pinfo = (struct mt9p001_info *) src->pinfo;

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->capture_mode = still_info->capture_mode;

	switch (still_info->capture_mode) {
	case AMBA_VIN_CAPTURE_MODE_ERS_CONTINUOUS:
		errorCode |= mt9p001_prepare_trigger(src,
			still_info->trigger_source,
			still_info->flash_level,
			still_info->flash_status, 0);

		errorCode |= mt9p001_write_reg(src, MT9P001_REG_30, 0x0000);
		break;

	case AMBA_VIN_CAPTURE_MODE_ERS_SNAPSHOT:
		errorCode |= mt9p001_prepare_trigger(src,
			still_info->trigger_source,
			still_info->flash_level,
			still_info->flash_status, (MT9P001_SNAPSHOT));

		errorCode |= mt9p001_write_reg(src, MT9P001_REG_30, 0x0000);
		break;

	case AMBA_VIN_CAPTURE_MODE_ERS_BULB:
		errorCode |= mt9p001_prepare_trigger(src,
			still_info->trigger_source,
			still_info->flash_level,
			still_info->flash_status,
			(MT9P001_SNAPSHOT | MT9P001_BULB_EXPOSURE));

		errorCode |= mt9p001_write_reg(src, MT9P001_REG_30, 0x0000);
		break;

	case AMBA_VIN_CAPTURE_MODE_GRR_SNAPSHOT:
		errorCode |= mt9p001_write_reg(src, MT9P001_REG_30, 0x0001);

		errorCode |= mt9p001_prepare_trigger(src,
			still_info->trigger_source,
			still_info->flash_level,
			still_info->flash_status,
			(MT9P001_SNAPSHOT | MT9P001_GLOBAL_RESET));
		break;

	case AMBA_VIN_CAPTURE_MODE_GRR_BULB:
		errorCode |= mt9p001_write_reg(src, MT9P001_REG_30, 0x0001);

		errorCode |= mt9p001_prepare_trigger(src,
			still_info->trigger_source,
			still_info->flash_level,
			still_info->flash_status,
			(MT9P001_SNAPSHOT | MT9P001_GLOBAL_RESET | MT9P001_BULB_EXPOSURE));
		break;

	default:
		vin_err("mt9p001_set_operating_modes do not support %d!\n", still_info->capture_mode);
		errorCode = -EINVAL;
		break;;
	}

	for (i = 0; i < MT9P001_VIDEO_MODE_TABLE_SZIE; i++) {
		if (mt9p001_video_mode_table[i].mode == still_info->still_mode) {
			errorCode = mt9p001_set_video_index(src, mt9p001_video_mode_table[i].still_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= MT9P001_VIDEO_MODE_TABLE_SZIE)
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

	if (!errorCode) {
		pinfo->current_vin_mode = mt9p001_video_mode_table[i].mode;
		pinfo->mode_type = mt9p001_video_mode_table[i].still_mode_type;
	}

	if(still_info->capture_mode==AMBA_VIN_CAPTURE_MODE_GRR_SNAPSHOT)
		mt9p001_write_reg(src, MT9P001_RESTART, 0x0004);
	return errorCode;
}

static int mt9p001_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int errorCode = 0;
	uint readmode;
	uint bayer_pattern;
	struct mt9p001_info *pinfo;
	u16 tmp_reg;

	pinfo = (struct mt9p001_info *) src->pinfo;
	switch (mirror_mode->pattern) {
	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
		bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GB;
		readmode = MT9P001_READMODE2_REG_MIRROR_ROW + MT9P001_READMODE2_REG_MIRROR_COLUMN;
		break;
	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		readmode = MT9P001_READMODE2_REG_MIRROR_ROW;
		break;
	case AMBA_VIN_SRC_MIRROR_VERTICALLY:
		bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		readmode = MT9P001_READMODE2_REG_MIRROR_COLUMN;
		break;
	case AMBA_VIN_SRC_MIRROR_NONE:
		bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
		readmode = 0;
		break;
	default:
		vin_err("do not support cmd mirror mode\n");
		return -EINVAL;
		break;
	}
	/* here, we adjust the MT9P001_COL_START register, we deal with 720p ,xga and 1080p at this time. */
	if (readmode & MT9P001_READMODE2_REG_MIRROR_COLUMN) {
		switch (pinfo->current_vin_mode) {
		case AMBA_VIDEO_MODE_AUTO:
		case AMBA_VIDEO_MODE_720P:
			tmp_reg = 0x24;
			break;
		case AMBA_VIDEO_MODE_1080P:
			tmp_reg = 0x162;
			break;
		case AMBA_VIDEO_MODE_XGA:
			tmp_reg = 0x122;
			break;
		default:
			tmp_reg = 0;
		}
	} else {
		switch (pinfo->current_vin_mode) {
		case AMBA_VIDEO_MODE_720P:
			tmp_reg = 0x20;
			break;
		case AMBA_VIDEO_MODE_1080P:
			tmp_reg = 0x160;
			break;
		case AMBA_VIDEO_MODE_XGA:
			tmp_reg = 0x120;
			break;
		default:
			tmp_reg = 0;
		}
	}
	if (tmp_reg) {
		errorCode |= mt9p001_write_reg(src, MT9P001_COL_START, tmp_reg);
	}

	pinfo->bayer_pattern = bayer_pattern;
	errorCode |= mt9p001_read_reg(src, MT9P001_READ_MODE_2, &tmp_reg);
	tmp_reg &= (~MT9P001_MIRROR_MASK);
	tmp_reg |= readmode;
	errorCode |= mt9p001_write_reg(src, MT9P001_READ_MODE_2, tmp_reg);

	return errorCode;
}

static int mt9p001_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct mt9p001_info *pinfo;

	pinfo = (struct mt9p001_info *) src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}
static int mt9p001_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));

	return 0;
}

static int mt9p001_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errorCode = 0;
	struct mt9p001_info *pinfo;

	pinfo = (struct mt9p001_info *) src->pinfo;

	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		{
			u32 cmd = *(u32 *) args;

			if (cmd & AMBA_VIN_SRC_RESET_HW)
				AMBA_VIN_HW_RESET();
			else if (cmd & AMBA_VIN_SRC_RESET_SW)
				mt9p001_sw_reset(src);
			else
				mt9p001_sw_reset(src);
		}
		break;

	case AMBA_VIN_SRC_SET_POWER:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_INFO:
		{
			struct amba_vin_source_info *pub_src;

			pub_src = (struct amba_vin_source_info *) args;
			pub_src->id = src->id;
			pub_src->adapter_id = adapter_id;
			pub_src->dev_type = src->dev_type;
			strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
			pub_src->sensor_id = SENSOR_MT9P001;
			pub_src->default_mode = MT9P001_VIDEO_MODE_TABLE_AUTO;
			pub_src->total_channel_num = src->total_channel_num;
			pub_src->active_channel_id = src->active_channel_id;
			pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errorCode = mt9p001_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errorCode = mt9p001_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errorCode = mt9p001_check_vidoe_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errorCode = mt9p001_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errorCode = mt9p001_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errorCode = mt9p001_get_video_mode(src, (enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errorCode = mt9p001_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_SRC_SET_STILL_MODE:
		errorCode = mt9p001_set_still_mode(src, (struct amba_vin_src_still_info *) args);
		break;

	case AMBA_VIN_SRC_GET_BLC:
		errorCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_GET_SW_BLC, (void *) args);
		memcpy(&(pinfo->current_sw_blc), (void *) args, sizeof (pinfo->current_sw_blc));
		break;
	case AMBA_VIN_SRC_SET_BLC:
		memcpy(&(pinfo->current_sw_blc), (void *) args, sizeof (pinfo->current_sw_blc));
		errorCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_SW_BLC, (void *) args);
		break;

	case AMBA_VIN_SRC_GET_FRAME_RATE:
		*(u32 *)args = pinfo->frame_rate;
		break;
	case AMBA_VIN_SRC_SET_FRAME_RATE:
		errorCode = mt9p001_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		errorCode = mt9p001_get_agc_db(src, (s32 *) args);
		break;
	case AMBA_VIN_SRC_SET_GAIN_DB:
		errorCode = mt9p001_set_agc_db(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		errorCode = mt9p001_get_shutter_time(src, (u32 *) args);
		break;
	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errorCode = mt9p001_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPTURE_MODE:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		errorCode = mt9p001_set_capture_mode(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_TRIGGER_MODE:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
//		errorCode = mt9p001_set_trigger_mode(src, *(enum amba_vin_trigger_mode *) args);
		{
			struct amba_vin_trigger_pin_info trigger_info;
			struct amba_vin_trigger_mode* trigger_mode = (struct amba_vin_trigger_mode*)args;

			memcpy(&trigger_info, &trigger_mode->enable, sizeof(struct amba_vin_trigger_pin_info));
			if(trigger_mode->trigger_select==0)
				errorCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_TRIGGER0_PIN_INFO, (void *) &trigger_info);
			else if(trigger_mode->trigger_select==1)
				errorCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_TRIGGER1_PIN_INFO, (void *) &trigger_info);
		}
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errorCode = mt9p001_set_low_ligth_mode(src, *(int *) args);
		break;

	case AMBA_VIN_SRC_GET_SLOWSHUTTER_MODE:
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errorCode = mt9p001_set_slowshutter_mode(src, *(int *) args);
		break;
	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errorCode = mt9p001_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;
	case AMBA_VIN_SRC_TEST_DUMP_REG:
		mt9p001_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		{
			u16 sen_id;
			u16 sen_ver;
			u32 *pdata = (u32 *) args;

			errorCode = mt9p001_query_snesor_id(src, &sen_id);
			if (errorCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;
			errorCode = mt9p001_query_snesor_version(src, &sen_ver);
			if (errorCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;

			*pdata = (sen_id << 16) | sen_ver;
		}
	      exit_AMBA_VIN_SRC_TEST_GET_DEV_ID:
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u8 subaddr;
			u16 data;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;

			errorCode = mt9p001_read_reg(src, subaddr, &data);

			reg_data->data = data;
		}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u8 subaddr;
			u16 data;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;
			data = reg_data->data;

			errorCode = mt9p001_write_reg(src, subaddr, data);
		}
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errorCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errorCode;
}
