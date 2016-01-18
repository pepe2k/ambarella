/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx224/imx224_docmd.c
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

static void imx224_dump_reg(struct __amba_vin_source *src)
{
}

static void imx224_fill_video_format_regs(struct __amba_vin_source *src)
{
	int			i;
	u32			index;
	u32			format_index;
	struct imx224_info	*pinfo;

	pinfo = (struct imx224_info *)src->pinfo;

	vin_dbg("imx224_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = imx224_video_info_table[index].format_index;

	switch(pinfo->op_mode) {
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
			vin_err("Unsupported mode\n");
			return;
	}

	for (i = 0; i < IMX224_VIDEO_FORMAT_REG_NUM; i++) {
		imx224_write_reg(src,
			imx224_video_format_tbl.reg[i],
			imx224_video_format_tbl.table[format_index].data[i]);
	}

	if (imx224_video_format_tbl.table[format_index].ext_reg_fill)
		imx224_video_format_tbl.table[format_index].ext_reg_fill(src);

}

static void imx224_fill_share_regs(struct __amba_vin_source *src)
{
	int		i;
	const struct imx224_reg_table	*reg_tbl = imx224_share_regs;

	vin_dbg("IMX224 fill share regs\n");
	for (i = 0; i < IMX224_SHARE_REG_SIZE; i++){
		vin_dbg("IMX224 write reg %d\n", i);
		imx224_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void imx224_sw_reset(struct __amba_vin_source *src)
{
	imx224_write_reg(src, IMX224_STANDBY, 0x1);/* Standby */
	imx224_write_reg(src, IMX224_SWRESET, 0x1);
	msleep(30);
}

static void imx224_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	imx224_sw_reset(src);
}

static void imx224_fill_pll_regs(struct __amba_vin_source *src)
{
	int i;
	struct imx224_info *pinfo;
	const struct imx224_pll_reg_table 	*pll_reg_table;
	const struct imx224_reg_table 		*pll_tbl;

	pinfo = (struct imx224_info *) src->pinfo;

	vin_dbg("imx224_fill_pll_regs\n");
	pll_reg_table = &imx224_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < IMX224_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			imx224_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
	msleep(2);
}

static int imx224_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct imx224_info  *pinfo;

	pinfo = (struct imx224_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int imx224_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int	errCode = 0;
	u32	index;
	u32	format_index;
	struct imx224_info *pinfo;

	pinfo = (struct imx224_info *)src->pinfo;

	index = pinfo->current_video_index;

	if (index >= IMX224_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = imx224_video_info_table[index].format_index;

		p_video_info->width = imx224_video_info_table[index].act_width;
		p_video_info->height = imx224_video_info_table[index].act_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = imx224_video_format_tbl.table[format_index].format;
		p_video_info->type = imx224_video_format_tbl.table[format_index].type;
		p_video_info->bits = imx224_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = imx224_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;

}

static int imx224_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct imx224_info *pinfo;

	pinfo = (struct imx224_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}

static int imx224_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));
	return 0;
}

static int imx224_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;
	u32 video_tbl_size;
	struct imx224_video_mode *video_mode_tbl;
	struct imx224_info *pinfo;

	pinfo = (struct imx224_info *) src->pinfo;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	switch(pinfo->op_mode) {
		case IMX224_LINEAR_MODE:
			video_tbl_size = IMX224_VIDEO_MODE_TABLE_SIZE;
			video_mode_tbl = imx224_video_mode_table;
			break;
		case IMX224_2X_WDR_MODE:
			video_tbl_size = IMX224_VIDEO_MODE_2XDOL_TABLE_SIZE;
			video_mode_tbl = imx224_video_mode_2xdol_table;
			break;
		case IMX224_3X_WDR_MODE:
			video_tbl_size = IMX224_VIDEO_MODE_3XDOL_TABLE_SIZE;
			video_mode_tbl = imx224_video_mode_3xdol_table;
			break;
		default:
			vin_err("Unsupported mode:%d\n", pinfo->op_mode);
			video_tbl_size = 0;
			video_mode_tbl = NULL;
			return -EPERM;
	}

	for (i = 0; i < video_tbl_size; i++) {
		if (video_mode_tbl[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = IMX224_VIDEO_MODE_TABLE_AUTO;

			p_mode_info->is_supported = 1;

			index = video_mode_tbl[i].still_index;
			format_index = imx224_video_info_table[index].format_index;

			p_mode_info->video_info.width = imx224_video_info_table[index].act_width;
			p_mode_info->video_info.height = imx224_video_info_table[index].act_height;
			p_mode_info->video_info.fps = imx224_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = imx224_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = imx224_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = imx224_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = imx224_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;
			break;
		}
	}

	return errCode;
}

static int imx224_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length;
	u8 	shr_width_m, shr_width_l;
	int	blank_lines, shutter_width;
	const struct imx224_pll_reg_table *pll_table;
	struct imx224_info		*pinfo;

	pinfo = (struct imx224_info *)src->pinfo;

	pll_table = &imx224_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time;

	imx224_read_reg(src, IMX224_VMAX_MSB, &shr_width_m);
	imx224_read_reg(src, IMX224_VMAX_LSB, &shr_width_l);
	frame_length = (shr_width_m<<8) + shr_width_l;

	imx224_read_reg(src, IMX224_HMAX_MSB, &shr_width_m);
	imx224_read_reg(src, IMX224_HMAX_LSB, &shr_width_l);
	line_length = ((shr_width_m)<<8) + shr_width_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	/* t(frame)/t(per line) = t(exposure, in lines)*/
	exposure_time_q9 = exposure_time_q9 * pll_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = exposure_time_q9;

	/* FIXME: shutter width: 2 ~ Frame format(V) */
	if(shutter_width < 2) {
		shutter_width = 2;
	} else if(shutter_width  > frame_length) {
		shutter_width = frame_length;
	}

	blank_lines = frame_length - shutter_width; /* get the shutter sweep time */

	shr_width_m = blank_lines >> 8;
	shr_width_l = blank_lines & 0xff;

	imx224_write_reg(src, IMX224_REGHOLD, 0x1);
	imx224_write_reg(src, IMX224_SHS1_MSB, shr_width_m);
	imx224_write_reg(src, IMX224_SHS1_LSB, shr_width_l);
	imx224_write_reg(src, IMX224_REGHOLD, 0x0);

	vin_dbg("frame_length: %d, sweep lines:%d\n", frame_length, blank_lines);
	pinfo->current_shutter_time = shutter_time;
	return 0;
}

static int imx224_set_shutter_row( struct __amba_vin_source *src, u32 row, bool sync)
{
	u64 	exposure_time_q9;
	u32 	blank_lines, shutter_width, frame_length, line_length, format_index;
	u8 	shr_width_m, shr_width_l;

	const struct imx224_pll_reg_table *pll_table;
	struct imx224_info		*pinfo;
	pinfo = (struct imx224_info *)src->pinfo;

	format_index = imx224_video_info_table[pinfo->current_video_index].format_index;
	pll_table = &imx224_pll_tbl[pinfo->pll_index];

	imx224_read_reg(src, IMX224_VMAX_MSB, &shr_width_m);
	imx224_read_reg(src, IMX224_VMAX_LSB, &shr_width_l);
	frame_length = (shr_width_m<<8) + shr_width_l;

	imx224_read_reg(src, IMX224_HMAX_MSB, &shr_width_m);
	imx224_read_reg(src, IMX224_HMAX_LSB, &shr_width_l);
	line_length = ((shr_width_m)<<8) + shr_width_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	shutter_width = row;

	/* FIXME: shutter width: 2 ~ Frame format(V) */
	if(shutter_width < 2) {
		shutter_width = 2;
	} else if(shutter_width  > frame_length) {
		shutter_width = frame_length;
	}

	blank_lines = frame_length - shutter_width; /* get the shutter sweep time */

	shr_width_m = blank_lines >> 8;
	shr_width_l = blank_lines & 0xff;

	if (sync) {
		u32 vsync_irq;

		vsync_irq= vsync_irq_flag;
		pinfo->sync_call_cnt++;
		wait_event_interruptible(vsync_irq_wait, vsync_irq < vsync_irq_flag || pinfo->sync_call_cnt < 7);//FIXME
	}
	imx224_write_reg(src, IMX224_REGHOLD, 0x1);
	imx224_write_reg(src, IMX224_SHS1_MSB, shr_width_m);
	imx224_write_reg(src, IMX224_SHS1_LSB, shr_width_l);
	imx224_write_reg(src, IMX224_REGHOLD, 0x0);

	exposure_time_q9 = shutter_width;
	exposure_time_q9 = exposure_time_q9 * (u64)line_length * 512000000;
	DO_DIV_ROUND(exposure_time_q9, pll_table->pixclk);

	pinfo->current_shutter_time = (u32)exposure_time_q9;

	vin_dbg("shutter_time:%d, row:%d\n", pinfo->current_shutter_time, shutter_width);

	return 0;
}

static int imx224_shutter_time2width(struct __amba_vin_source *src, u32* shutter_time)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length;
	u8 	shr_width_m, shr_width_l;

	const struct imx224_pll_reg_table *pll_table;
	struct imx224_info		*pinfo;
	pinfo = (struct imx224_info *)src->pinfo;

	pll_table = &imx224_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = *shutter_time;

	imx224_read_reg(src, IMX224_VMAX_MSB, &shr_width_m);
	imx224_read_reg(src, IMX224_VMAX_LSB, &shr_width_l);
	frame_length = (shr_width_m<<8) + shr_width_l;

	imx224_read_reg(src, IMX224_HMAX_MSB, &shr_width_m);
	imx224_read_reg(src, IMX224_HMAX_LSB, &shr_width_l);
	line_length = (shr_width_m<<8) + shr_width_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	/* t(frame)/t(per line) = t(exposure, in lines)*/
	exposure_time_q9 = exposure_time_q9 * pll_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	*shutter_time =exposure_time_q9;

	return 0;
}

static int imx224_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	int errCode = 0;
	struct imx224_info *pinfo;
	s32 db_max, db_step, gain_index;

	pinfo = (struct imx224_info *) src->pinfo;

	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	vin_dbg("imx224_set_agc: 0x%x\n", agc_db);

	/* if it's high gain mode, 6db <= agc <=  54db */
	if (pinfo->high_gain_mode) {
		if (agc_db < 0x6000000){
			vin_dbg("high gain mode, agc should >= 6db, (0x%x)\n", agc_db);
			agc_db = 0x6000000;
		}
		else if (agc_db > 0x36000000) {
			vin_dbg("high gain mode, agc should <= 54db, (0x%x)\n", agc_db);
			agc_db = 0x36000000;
		}

		agc_db -= 0x6000000;
	}

	gain_index = (db_max - agc_db) / db_step;

	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		gain_index = IMX224_GAIN_MAX_DB - gain_index;
		imx224_write_reg(src, IMX224_REGHOLD, 0x1);
		imx224_write_reg(src, IMX224_GAIN_LSB, (u8)(gain_index & 0xFF));
		imx224_write_reg(src, IMX224_GAIN_MSB, (u8)(gain_index >> 8));
		imx224_write_reg(src, IMX224_REGHOLD, 0x0);
		pinfo->current_gain_db = agc_db;
	} else {
		errCode = -1;	/* Index is out of range */
	}
	return errCode;
}

static int imx224_set_agc_index( struct __amba_vin_source *src, u32 index, bool sync)
{
	int errCode = 0;
	struct imx224_info *pinfo;
	pinfo = (struct imx224_info *) src->pinfo;

	/* if it's high gain mode, 6db <= agc <=  54db */
	if (pinfo->high_gain_mode) {
		if (index < 60){
			vin_dbg("high gain mode, agc should >= 6db, (%d)\n", index);
			index = 60;
		}
		else if (index > 540) {
			vin_dbg("high gain mode, agc should <= 54db, (%d)\n", index);
			index = 540;
		}

		index -= 60;
	}

	if ((index >= pinfo->min_agc_index) && (index <= pinfo->max_agc_index)) {
		if (sync) {
			u32 vsync_irq;

			vsync_irq= vsync_irq_flag;
			pinfo->sync_call_cnt++;
			wait_event_interruptible(vsync_irq_wait, vsync_irq < vsync_irq_flag || pinfo->sync_call_cnt < 7);//FIXME
		}
		imx224_write_reg(src, IMX224_REGHOLD, 0x1);
		imx224_write_reg(src, IMX224_GAIN_LSB, (u8)(index & 0xFF));
		imx224_write_reg(src, IMX224_GAIN_MSB, (u8)(index >> 8));
		imx224_write_reg(src, IMX224_REGHOLD, 0x0);
	} else {/* Index is out of range */
		errCode = -1;
		vin_err("gain index %d exceeds limitation [%d~%d]\n", index,
			pinfo->min_agc_index, pinfo->max_agc_index);
	}
	return errCode;
}

static int imx224_set_shutter_and_agc( struct __amba_vin_source *src, struct sensor_cmd_info_t * info, bool sync)
{
	int errCode = 0;
	u64 	exposure_time_q9;
	u32 	blank_lines, shutter_width, frame_length, line_length, format_index, gain_index;
	u8 	shr_width_m, shr_width_l;

	const struct imx224_pll_reg_table *pll_table;
	struct imx224_info		*pinfo;
	pinfo = (struct imx224_info *)src->pinfo;

	format_index = imx224_video_info_table[pinfo->current_video_index].format_index;
	pll_table = &imx224_pll_tbl[pinfo->pll_index];

	imx224_read_reg(src, IMX224_VMAX_MSB, &shr_width_m);
	imx224_read_reg(src, IMX224_VMAX_LSB, &shr_width_l);
	frame_length = (shr_width_m<<8) + shr_width_l;

	imx224_read_reg(src, IMX224_HMAX_MSB, &shr_width_m);
	imx224_read_reg(src, IMX224_HMAX_LSB, &shr_width_l);
	line_length = ((shr_width_m)<<8) + shr_width_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	shutter_width = info->shutter_row;

	/* FIXME: shutter width: 2 ~ Frame format(V) */
	if(shutter_width < 2) {
		shutter_width = 2;
	} else if(shutter_width  > frame_length) {
		shutter_width = frame_length;
	}

	blank_lines = frame_length - shutter_width; /* get the shutter sweep time */

	shr_width_m = blank_lines >> 8;
	shr_width_l = blank_lines & 0xff;

	if ((info->gain_tbl_index>= pinfo->min_agc_index) && (info->gain_tbl_index <= pinfo->max_agc_index)) {
		gain_index = info->gain_tbl_index;

		/* if it's high gain mode, 6db <= agc <=  54db */
		if (pinfo->high_gain_mode) {
			if (gain_index < 60){
				vin_dbg("high gain mode, agc should >= 6db, (%d)\n", gain_index);
				gain_index = 60;
			}
			else if (gain_index > 540) {
				vin_dbg("high gain mode, agc should <= 54db, (%d)\n", gain_index);
				gain_index = 540;
			}
			gain_index -= 60;
		}

		if (sync) {
			u32 vsync_irq;

			vsync_irq= vsync_irq_flag;
			pinfo->sync_call_cnt++;
			wait_event_interruptible(vsync_irq_wait, vsync_irq < vsync_irq_flag || pinfo->sync_call_cnt < 7);//FIXME
		}
		imx224_write_reg(src, IMX224_REGHOLD, 0x1);
		imx224_write_reg(src, IMX224_GAIN_LSB, (u8)(gain_index & 0xFF));
		imx224_write_reg(src, IMX224_GAIN_MSB, (u8)(gain_index >> 8));

		imx224_write_reg(src, IMX224_SHS1_MSB, shr_width_m);
		imx224_write_reg(src, IMX224_SHS1_LSB, shr_width_l);
		imx224_write_reg(src, IMX224_REGHOLD, 0x0);
	} else {
		vin_err("gain index %d exceeds limitation [%d~%d]\n", info->gain_tbl_index,
			pinfo->min_agc_index, pinfo->max_agc_index);
		return -EPERM;
	}

	exposure_time_q9 = shutter_width;
	exposure_time_q9 = exposure_time_q9 * (u64)line_length * 512000000;
	DO_DIV_ROUND(exposure_time_q9, pll_table->pixclk);

	pinfo->current_shutter_time = (u32)exposure_time_q9;

	vin_dbg("shutter_time:%d, row:%d\n", pinfo->current_shutter_time, shutter_width);

	return errCode;
}

static int imx224_set_wdr_again_index_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_gain_gp_info *p_again_gp)
{
	return 0;
}

static int imx224_set_wdr_dgain_index_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_gain_gp_info *p_dgain_gp)
{
	return 0;
}

static int imx224_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int		errCode = 0;
	u32		readmode;
	struct	imx224_info			*pinfo;
	u32		target_bayer_pattern;
	u16		tmp_reg;

	pinfo = (struct imx224_info *) src->pinfo;

	switch (mirror_mode->bayer_pattern) {
	case AMBA_VIN_SRC_BAYER_PATTERN_AUTO:
		break;

	case AMBA_VIN_SRC_BAYER_PATTERN_RG:
	case AMBA_VIN_SRC_BAYER_PATTERN_BG:
	case AMBA_VIN_SRC_BAYER_PATTERN_GR:
	case AMBA_VIN_SRC_BAYER_PATTERN_GB:
		pinfo->bayer_pattern = mirror_mode->bayer_pattern;
		break;

	default:
		vin_err("do not support bayer pattern: %d\n", mirror_mode->bayer_pattern);
		return -EINVAL;
	}

	switch (mirror_mode->pattern) {
	case AMBA_VIN_SRC_MIRROR_AUTO:
		return 0;

	case AMBA_VIN_SRC_MIRROR_VERTICALLY:
		readmode = IMX224_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GB;
		break;

	case AMBA_VIN_SRC_MIRROR_NONE:
		readmode = 0;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX224_H_MIRROR | IMX224_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		readmode = IMX224_H_MIRROR;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
		break;

	default:
		vin_err("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	errCode |= imx224_read_reg(src, IMX224_WINMODE, (u8 *)&tmp_reg);
	tmp_reg &= ~(IMX224_H_MIRROR | IMX224_V_FLIP);
	tmp_reg |= readmode;
	errCode |= imx224_write_reg(src, IMX224_WINMODE, tmp_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}

static int imx224_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int					errCode = 0;
	u32					frame_time = 0;
	u64					frame_time_pclk;
	u32					vertical_lines = 0, fsc;
	u32					line_length = 0;
	u32					index;
	u32					format_index;
	u8					data_val;
	u8					current_pll_index = 0;
	const struct imx224_pll_reg_table 	*pll_table;
	struct imx224_info			*pinfo;
	struct amba_vin_irq_fix			vin_irq_fix;

	pinfo = (struct imx224_info *)src->pinfo;
	index = pinfo->current_video_index;

	if (index >= IMX224_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx224_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto imx224_set_fps_exit;
	}

	format_index = imx224_video_info_table[index].format_index;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = imx224_video_format_tbl.table[format_index].auto_fps;
	if(fps < imx224_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, imx224_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto imx224_set_fps_exit;
	}

	frame_time = fps;

	/* ToDo: Add specified PLL index*/
	current_pll_index = 0;

	if(pinfo->pll_index != current_pll_index){
		pinfo->pll_index = current_pll_index;
		imx224_fill_pll_regs(src);
		errCode = imx224_init_vin_clock(src, &imx224_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto imx224_set_fps_exit;
	}

	pll_table = &imx224_pll_tbl[pinfo->pll_index];

	errCode |= imx224_read_reg(src, IMX224_HMAX_MSB, &data_val);
	line_length = (u32)(data_val) <<  8;
	errCode |= imx224_read_reg(src, IMX224_HMAX_LSB, &data_val);
	line_length += (u32)data_val;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}
	vin_dbg("line_length is  0x%0x\n", line_length);

	frame_time_pclk = frame_time * (u64)pll_table->pixclk;

	vin_dbg("pixclk %d, frame_time_pclk 0x%llx\n", pll_table->pixclk, frame_time_pclk);

	DO_DIV_ROUND(frame_time_pclk, line_length);
	DO_DIV_ROUND(frame_time_pclk, 512000000);

	fsc = frame_time_pclk;

	if (pinfo->op_mode == IMX224_LINEAR_MODE) {
		vertical_lines = fsc;/* FSC = VMAX */
	} else if(pinfo->op_mode == IMX224_2X_WDR_MODE) {
		vertical_lines = fsc >> 1; /* FSC = VMAX * 2 */
	} else if (pinfo->op_mode == IMX224_3X_WDR_MODE) {
		vertical_lines = fsc >> 2; /* FSC = VMAX * 4 */
	}

	vin_dbg("vertical_lines is  0x%05x\n", vertical_lines);
	errCode |= imx224_write_reg(src, IMX224_VMAX_MSB, (u8)((vertical_lines & 0xFF00) >> 8));
	errCode |= imx224_write_reg(src, IMX224_VMAX_LSB, (u8)(vertical_lines & 0xFF));

	imx224_set_shutter_time(src, pinfo->current_shutter_time);// keep the same shutter time

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_irq_fix.delay = 0;// change to zero for all non Aptina Hi-SPI sensors (for A5s/A7/S2)

	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

imx224_set_fps_exit:
	return errCode;
}

static void imx224_start_streaming(struct __amba_vin_source *src)
{
	imx224_write_reg(src, IMX224_STANDBY, 0x00); //cancel standby
	msleep(10);
	imx224_write_reg(src, IMX224_XMSTA, 0x00); //master mode start
}

static int imx224_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int				errCode = 0;
	struct imx224_info		*pinfo;
	u32				format_index;

	pinfo = (struct imx224_info *)src->pinfo;

	if (index >= IMX224_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx224_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto imx224_set_mode_exit;
	}

	errCode |= imx224_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = imx224_video_info_table[index].format_index;

	pinfo->cap_start_x 	= imx224_video_info_table[index].def_start_x;
	pinfo->cap_start_y 	= imx224_video_info_table[index].def_start_y;
	pinfo->cap_cap_w 	= imx224_video_info_table[index].def_width;
	pinfo->cap_cap_h 	= imx224_video_info_table[index].def_height;
	pinfo->act_start_x = imx224_video_info_table[index].act_start_x;
	pinfo->act_start_y = imx224_video_info_table[index].act_start_y;
	pinfo->act_width = imx224_video_info_table[index].act_width;
	pinfo->act_height = imx224_video_info_table[index].act_height;
	pinfo->slvs_eav_col = imx224_video_format_tbl.table[format_index].slvs_eav_col;
	pinfo->slvs_sav2sav_dist = imx224_video_format_tbl.table[format_index].slvs_sav2sav_dist;
	pinfo->bayer_pattern 	= imx224_video_info_table[index].bayer_pattern;
	pinfo->pll_index 	= imx224_video_format_tbl.table[format_index].pll_index;

	imx224_print_info(src);

	//set clk_si
	errCode |= imx224_init_vin_clock(src, &imx224_pll_tbl[pinfo->pll_index]);

	errCode |= imx224_set_vin_mode(src);

	imx224_fill_pll_regs(src);

	imx224_fill_share_regs(src);

	imx224_fill_video_format_regs(src);
	imx224_set_fps(src, AMBA_VIDEO_FPS_AUTO);

	imx224_set_shutter_time(src, AMBA_VIDEO_FPS_60);
	imx224_set_agc_db(src, 0);

	errCode |= imx224_post_set_vin_mode(src);

	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

	/* TG reset release ( Enable Streaming )*/
	imx224_start_streaming(src);

	//imx224_dump_reg(src);

imx224_set_mode_exit:
	return errCode;
}

static int imx224_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int				errCode = -EINVAL;
	int				i;
	struct imx224_info		*pinfo;
	u32 video_tbl_size;
	struct imx224_video_mode *video_mode_tbl;

	static int			first_set_video_mode = 1;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct imx224_info *)src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errCode = imx224_init_vin_clock(src, &imx224_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto imx224_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	imx224_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	if (mode == AMBA_VIDEO_MODE_AUTO) {
		mode = IMX224_VIDEO_MODE_TABLE_AUTO;
	}

	switch(pinfo->op_mode) {
		case IMX224_LINEAR_MODE:
			video_tbl_size = IMX224_VIDEO_MODE_TABLE_SIZE;
			video_mode_tbl = imx224_video_mode_table;
			break;
		case IMX224_2X_WDR_MODE:
			video_tbl_size = IMX224_VIDEO_MODE_2XDOL_TABLE_SIZE;
			video_mode_tbl = imx224_video_mode_2xdol_table;
			break;
		case IMX224_3X_WDR_MODE:
			video_tbl_size = IMX224_VIDEO_MODE_3XDOL_TABLE_SIZE;
			video_mode_tbl = imx224_video_mode_3xdol_table;
			break;
		default:
			vin_err("Unsupported mode:%d\n", pinfo->op_mode);
			video_tbl_size = 0;
			video_mode_tbl = NULL;
			return -EPERM;
	}

	for (i = 0; i < video_tbl_size; i++) {
		if (video_mode_tbl[i].mode == mode) {
			errCode = imx224_set_video_index(src, video_mode_tbl[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= video_tbl_size) {
		vin_err("imx224_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = video_mode_tbl[i].mode;
		pinfo->mode_type = video_mode_tbl[i].preview_mode_type;
	}

imx224_set_video_mode_exit:
	return errCode;
}

static int imx224_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	struct imx224_info		*pinfo;

	pinfo = (struct imx224_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = imx224_init_vin_clock(src, &imx224_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto imx224_init_hw_exit;

	msleep(10);
	imx224_reset(src);

imx224_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int imx224_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{
	return 0;
}

static int imx224_set_low_ligth_mode(struct __amba_vin_source *src, u8 ll_mode)
{
	int				errCode = 0;
	struct imx224_info		*pinfo;
	u8 tmp;

	pinfo = (struct imx224_info *) src->pinfo;

	if (ll_mode) {
		imx224_read_reg(src, IMX224_FRSEL, &tmp);
		tmp |= IMX224_HI_GAIN_MODE;
		imx224_write_reg(src, IMX224_FRSEL, tmp);
		pinfo->high_gain_mode = 1;
		vin_info("high gain mode\n");
	} else {
		imx224_read_reg(src, IMX224_FRSEL, &tmp);
		tmp &= ~IMX224_HI_GAIN_MODE;
		imx224_write_reg(src, IMX224_FRSEL, tmp);
		pinfo->high_gain_mode = 0;
		vin_info("low gain mode\n");
	}
	return errCode;
}

static int imx224_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
	/* >> TODO, for YUV output mode only << */
	return errCode;
}

static int imx224_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;
	/* >> TODO, for RGB raw output mode only << */
	/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}

static int imx224_set_wdr_shutter_row_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_shutter_gp_info *p_shutter_gp)
{
	u8 	data;
	u32 frame_length, fsc, rhs1, rhs2;
	u32 index, format_index;
	int shutter_long, shutter_short1, shutter_short2;
	int errCode = 0;
	struct imx224_info *pinfo;
	const struct imx224_pll_reg_table *pll_reg_table;

	pinfo		= (struct imx224_info *)src->pinfo;
	index = pinfo->current_video_index;
	format_index = imx224_video_info_table[index].format_index;
	pll_reg_table	= &imx224_pll_tbl[pinfo->pll_index];

	imx224_read_reg(src, IMX224_RHS1_HSB, &data);
	rhs1 = (data & 0x0F) << 16;
	imx224_read_reg(src, IMX224_RHS1_MSB, &data);
	rhs1 += data << 8;
	imx224_read_reg(src, IMX224_RHS1_LSB, &data);
	rhs1 += data;

	imx224_read_reg(src, IMX224_RHS2_HSB, &data);
	rhs2 = (data & 0x0F) << 16;
	imx224_read_reg(src, IMX224_RHS2_MSB, &data);
	rhs2 += data << 8;
	imx224_read_reg(src, IMX224_RHS2_LSB, &data);
	rhs2 += data;

	imx224_read_reg(src, IMX224_VMAX_HSB, &data);
	frame_length = (data & 0x0F) << 16;
	imx224_read_reg(src, IMX224_VMAX_MSB, &data);
	frame_length += data << 8;
	imx224_read_reg(src, IMX224_VMAX_LSB, &data);
	frame_length += data;

	/* long shutter */
	shutter_long = p_shutter_gp->long_shutter;
	/* short shutter 1 */
	shutter_short1 = p_shutter_gp->short1_shutter;
	/* short shutter 2 */
	shutter_short2 = p_shutter_gp->short2_shutter;

	/* shutter limitation check */
	if(pinfo->op_mode == IMX224_2X_WDR_MODE) {
		fsc = frame_length << 1; /* FSC = VMAX * 2 */
		if(fsc < shutter_long) {
			vin_err("long shutter row(%d) must be less than fsc(%d)!\n", shutter_long, fsc);
			return -EPERM;
		} else {
			shutter_long = fsc - shutter_long - 1;
		}
		if(rhs1 < shutter_short1) {
			vin_err("short shutter row(%d) must be less than rhs(%d)!\n", shutter_short1, rhs1);
			return -EPERM;
		} else {
			shutter_short1 = rhs1 - shutter_short1 - 1;
		}
		vin_dbg("fsc:%d, shs1:%d, shs2:%d, rhs1:%d\n",
			fsc, shutter_short1, shutter_long, rhs1);

		/* short shutter check */
		if((shutter_short1 >= 3) && (shutter_short1 <= rhs1 - 2)) {
			imx224_write_reg(src, IMX224_SHS1_LSB, (u8)(shutter_short1 & 0xFF));
			imx224_write_reg(src, IMX224_SHS1_MSB, (u8)(shutter_short1 >> 8));
			imx224_write_reg(src, IMX224_SHS1_HSB, (u8)((shutter_short1 >> 16) & 0xF));
		} else {
			vin_err("shs1 exceeds limitation! shs1:%d, rhs1:%d\n",
					shutter_short1, rhs1);
			return -EPERM;
		}
		/* long shutter check */
		if((shutter_long >= rhs1 + 3) && (shutter_long <= fsc  - 2)) {
			imx224_write_reg(src, IMX224_SHS2_LSB, (u8)(shutter_long & 0xFF));
			imx224_write_reg(src, IMX224_SHS2_MSB, (u8)(shutter_long >> 8));
			imx224_write_reg(src, IMX224_SHS2_HSB, (u8)((shutter_long >> 16) & 0xF));
		} else {
			vin_err("shs2 exceeds limitation! shs2:%d, shs1:%d, rhs1:%d, fsc:%d\n",
					shutter_long, shutter_short1, rhs1, fsc);
			return -EPERM;
		}

	} else if(pinfo->op_mode == IMX224_3X_WDR_MODE) {
		fsc = frame_length << 2; /* FSC = VMAX * 4 */
		if(fsc < shutter_long) {
			vin_err("long shutter row(%d) must be less than fsc(%d)!\n", shutter_long, fsc);
			return -EPERM;
		} else {
			shutter_long = fsc - shutter_long - 1;
		}
		if(rhs1 < shutter_short1) {
			vin_err("short shutter 1 row(%d) must be less than rhs1(%d)!\n", shutter_short1, rhs1);
			return -EPERM;
		} else {
			shutter_short1 = rhs1 - shutter_short1 - 1;
		}
		if(rhs2 < shutter_short2) {
			vin_err("short shutter 2 row(%d) must be less than rhs2(%d)!\n", shutter_short2, rhs2);
			return -EPERM;
		} else {
			shutter_short2 = rhs2 - shutter_short2 - 1;
		}
		vin_dbg("fsc:%d, shs1:%d, shs2:%d, shs3:%d, rhs1:%d, rhs2:%d\n",
			fsc, shutter_short1, shutter_short2, shutter_long, rhs1, rhs2);

		/* short shutter 1 check */
		if((shutter_short1 >= 4) && (shutter_short1 <= rhs1 - 2)) {
			imx224_write_reg(src, IMX224_SHS1_LSB, (u8)(shutter_short1 & 0xFF));
			imx224_write_reg(src, IMX224_SHS1_MSB, (u8)(shutter_short1 >> 8));
			imx224_write_reg(src, IMX224_SHS1_HSB, (u8)((shutter_short1 >> 16) & 0xF));
		} else {
			vin_err("shs1 exceeds limitation! shs1:%d, rhs1:%d\n",
					shutter_short1, rhs1);
			return -EPERM;
		}
		/* short shutter 2 check */
		if((shutter_short2 >= rhs1 + 4) && (shutter_short2 <= rhs2 - 2)) {
			imx224_write_reg(src, IMX224_SHS2_LSB, (u8)(shutter_short2 & 0xFF));
			imx224_write_reg(src, IMX224_SHS2_MSB, (u8)(shutter_short2 >> 8));
			imx224_write_reg(src, IMX224_SHS2_HSB, (u8)((shutter_short2 >> 16) & 0xF));
		} else {
			vin_err("shs2 exceeds limitation! shs1:%d, shs2:%d, shs3:%d, rhs1:%d, rhs2:%d\n",
					shutter_short1, shutter_short2, shutter_long, rhs1, rhs2);
			return -EPERM;
		}
		/* long shutter check */
		if((shutter_long >= rhs2 + 4) && (shutter_long <= fsc - 2)) {
			imx224_write_reg(src, IMX224_SHS3_LSB, (u8)(shutter_long & 0xFF));
			imx224_write_reg(src, IMX224_SHS3_MSB, (u8)(shutter_long >> 8));
			imx224_write_reg(src, IMX224_SHS3_HSB, (u8)((shutter_long >> 16) & 0xF));
		} else {
			vin_err("shs3 exceeds limitation! shs1:%d, shs2:%d, shs3:%d, rhs2:%d, fsc:%d\n",
					shutter_short1, shutter_short2, shutter_long, rhs2, fsc);
			return -EPERM;
		}
	} else {
		vin_err("Non WDR mode can't support this API: %s!\n", __func__);
	}

	return errCode;
}

static int imx224_wdr_shutter2row(struct __amba_vin_source *src,
	struct amba_vin_wdr_shutter_gp_info *p_shutter2row)
{
	u64 exposure_time_q9;
	u8 	data;
	u32 line_length;
	u32 index, format_index;
	int errCode = 0;
	struct imx224_info *pinfo;
	const struct imx224_pll_reg_table *pll_reg_table;

	pinfo		= (struct imx224_info *)src->pinfo;
	index = pinfo->current_video_index;
	format_index = imx224_video_info_table[index].format_index;
	pll_reg_table	= &imx224_pll_tbl[pinfo->pll_index];

	imx224_read_reg(src, IMX224_HMAX_MSB, &data);
	line_length = data << 8;
	imx224_read_reg(src, IMX224_HMAX_LSB, &data);
	line_length += data;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	/* long shutter */
	exposure_time_q9 = p_shutter2row->long_shutter;
	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;
	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);
	p_shutter2row->long_shutter = (u32)exposure_time_q9;

	/* short shutter 1 */
	exposure_time_q9 = p_shutter2row->short1_shutter;
	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;
	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);
	p_shutter2row->short1_shutter = (u32)exposure_time_q9;

	/* short shutter 2 */
	exposure_time_q9 = p_shutter2row->short2_shutter;
	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;
	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);
	p_shutter2row->short2_shutter = (u32)exposure_time_q9;

	return errCode;
}

static int imx224_set_operation_mode(struct __amba_vin_source *src, amba_vin_sensor_op_mode mode)
{
	int 			errCode = 0;
	struct imx224_info	*pinfo = (struct imx224_info *) src->pinfo;

	if((mode != IMX224_LINEAR_MODE) &&
		(mode != IMX224_2X_WDR_MODE) &&
			(mode != IMX224_3X_WDR_MODE)){
		vin_err("wrong opeartion mode, %d!\n", mode);
		errCode = -EPERM;
	} else {
		pinfo->op_mode = mode;
	}

	return errCode;
}

static int imx224_wdr_get_win_offset(struct __amba_vin_source *src,
	struct amba_vin_wdr_win_offset *p_win_offset)
{
	int errCode = 0;
	u32 format_index;
	struct imx224_info	*pinfo = (struct imx224_info *) src->pinfo;

	if (pinfo->op_mode == IMX224_LINEAR_MODE) {
		vin_warn("%s should be called by hdr mode\n", __func__);
		return -EPERM;
	}

	format_index = imx224_video_info_table[pinfo->current_video_index].format_index;
	memcpy(p_win_offset,
		&imx224_video_format_tbl.table[format_index].hdr_win_offset,
		sizeof(struct amba_vin_wdr_win_offset));

	return errCode;
}

static int imx224_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct imx224_info		*pinfo;

	pinfo = (struct imx224_info *) src->pinfo;

	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		imx224_reset(src);
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
		errCode = imx224_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO: {
		struct amba_vin_source_info *pub_src;

		pub_src = (struct amba_vin_source_info *) args;
		pub_src->id = src->id;
		pub_src->sensor_id = SENSOR_IMX224;
		pub_src->adapter_id = adapter_id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = imx224_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = imx224_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = imx224_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = imx224_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = imx224_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = imx224_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		pinfo->sync_call_cnt = 0;
		errCode = imx224_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = imx224_set_still_mode(src, (struct amba_vin_src_still_info *) args);
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
		errCode = imx224_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = imx224_set_agc_db(src, *(s32 *) args);
		break;
	case AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH:
		errCode=imx224_shutter_time2width(src, (u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(s32*)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = imx224_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = imx224_set_low_ligth_mode(src, *(u8 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = imx224_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = imx224_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		imx224_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA: {
		struct amba_vin_test_reg_data *reg_data;
		u16 subaddr;
		u8 data = 0;

		reg_data = (struct amba_vin_test_reg_data *) args;
		subaddr = reg_data->reg;

		errCode = imx224_read_reg(src, subaddr, &data);

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

		errCode = imx224_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = imx224_set_slowshutter_mode(src, *(int *) args);
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW:
		errCode = imx224_set_shutter_row(src, *(u32 *) args, false);
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW_SYNC:
		errCode = imx224_set_shutter_row(src, *(u32 *) args, true);
		break;

	case AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX:
	case AMBA_VIN_SRC_SET_GAIN_INDEX:
		errCode = imx224_set_agc_index(src, *(u32 *) args, false);
		break;

	case AMBA_VIN_SRC_SET_GAIN_INDEX_SYNC:
		errCode = imx224_set_agc_index(src, *(u32 *) args, true);
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_AND_GAIN_SYNC:
		errCode = imx224_set_shutter_and_agc(src,(struct sensor_cmd_info_t *) args, true);
		break;

	case AMBA_VIN_SRC_SET_WDR_DGAIN_INDEX_GROUP:
		errCode = imx224_set_wdr_dgain_index_group(src,
			(struct amba_vin_wdr_gain_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_OPERATION_MODE:
		errCode = imx224_set_operation_mode(src, *(amba_vin_sensor_op_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX_GROUP:
		errCode = imx224_set_wdr_again_index_group(src,
			(struct amba_vin_wdr_gain_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_SHUTTER_ROW_GROUP:
		errCode = imx224_set_wdr_shutter_row_group(src,
			(struct amba_vin_wdr_shutter_gp_info *) args);
		break;

	case AMBA_VIN_SRC_WDR_SHUTTER2ROW:
		errCode = imx224_wdr_shutter2row(src,
			(struct amba_vin_wdr_shutter_gp_info *) args);
		break;

	case AMBA_VIN_SRC_GET_WDR_WIN_OFFSET:
		errCode = imx224_wdr_get_win_offset(src,
			(struct amba_vin_wdr_win_offset *) args);
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
