/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx185/imx185_docmd.c
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

static void imx185_dump_reg(struct __amba_vin_source *src)
{
}

static void imx185_fill_video_format_regs(struct __amba_vin_source *src)
{
	int			i;
	u32			index;
	u32			format_index;
	struct imx185_info	*pinfo;

	pinfo = (struct imx185_info *)src->pinfo;

	vin_dbg("imx185_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = imx185_video_info_table[index].format_index;

	for (i = 0; i < IMX185_VIDEO_FORMAT_REG_NUM; i++) {
		if (imx185_video_format_tbl.reg[i] == 0)
			break;

		imx185_write_reg(src,
			imx185_video_format_tbl.reg[i],
			imx185_video_format_tbl.table[format_index].data[i]);
	}

	if (imx185_video_format_tbl.table[format_index].ext_reg_fill)
		imx185_video_format_tbl.table[format_index].ext_reg_fill(src);

}

static void imx185_fill_share_regs(struct __amba_vin_source *src)
{
	int		i;
	const struct imx185_reg_table	*reg_tbl = imx185_share_regs;

	vin_dbg("IMX185 fill share regs\n");
	for (i = 0; i < IMX185_SHARE_REG_SIZE; i++){
		vin_dbg("IMX185 write reg %d\n", i);
		imx185_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void imx185_sw_reset(struct __amba_vin_source *src)
{
	imx185_write_reg(src, IMX185_STANDBY, 0x1);/* Standby */
	imx185_write_reg(src, IMX185_SWRESET, 0x1);
	msleep(30);
}

static void imx185_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	imx185_sw_reset(src);
}

static void imx185_fill_pll_regs(struct __amba_vin_source *src)
{
	int i;
	struct imx185_info *pinfo;
	const struct imx185_pll_reg_table 	*pll_reg_table;
	const struct imx185_reg_table 		*pll_tbl;

	pinfo = (struct imx185_info *) src->pinfo;

	vin_dbg("imx185_fill_pll_regs\n");
	pll_reg_table = &imx185_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < IMX185_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			imx185_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
	msleep(2);
}

static int imx185_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct imx185_info  *pinfo;

	pinfo = (struct imx185_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int imx185_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int	errCode = 0;
	u32	index;
	u32	format_index;
	struct imx185_info *pinfo;

	pinfo = (struct imx185_info *)src->pinfo;

	index = pinfo->current_video_index;

	if (index >= IMX185_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = imx185_video_info_table[index].format_index;

		p_video_info->width = imx185_video_info_table[index].def_width;
		p_video_info->height = imx185_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = imx185_video_format_tbl.table[format_index].format;
		p_video_info->type = imx185_video_format_tbl.table[format_index].type;
		p_video_info->bits = imx185_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = imx185_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;

}

static int imx185_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct imx185_info *pinfo;

	pinfo = (struct imx185_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}

static int imx185_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));
	return 0;
}

static int imx185_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < IMX185_VIDEO_MODE_TABLE_SIZE; i++) {
		if (imx185_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = IMX185_VIDEO_MODE_TABLE_AUTO;

			p_mode_info->is_supported = 1;

			index = imx185_video_mode_table[i].still_index;
			format_index = imx185_video_info_table[index].format_index;

			p_mode_info->video_info.width = imx185_video_info_table[index].def_width;
			p_mode_info->video_info.height = imx185_video_info_table[index].def_height;
			p_mode_info->video_info.fps = imx185_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = imx185_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = imx185_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = imx185_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = imx185_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;
			break;
		}
	}

	return errCode;
}

static int imx185_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length, format_index;
	u8 	shr_width_h, shr_width_m, shr_width_l;
	int	blank_lines;

	const struct imx185_pll_reg_table *pll_table;
	struct imx185_info		*pinfo;
	pinfo = (struct imx185_info *)src->pinfo;

	format_index = imx185_video_info_table[pinfo->current_video_index].format_index;
	pll_table = &imx185_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time;

	if (!slave_mode) {
		imx185_read_reg(src, IMX185_VMAX_HSB, &shr_width_h);
		imx185_read_reg(src, IMX185_VMAX_MSB, &shr_width_m);
		imx185_read_reg(src, IMX185_VMAX_LSB, &shr_width_l);

		frame_length = ((shr_width_h&0x01)<<16) + (shr_width_m<<8) + shr_width_l;

		imx185_read_reg(src, IMX185_HMAX_MSB, &shr_width_m);
		imx185_read_reg(src, IMX185_HMAX_LSB, &shr_width_l);

		line_length = ((shr_width_m)<<8) + shr_width_l;
		if(unlikely(!line_length)) {
			vin_err("line length is 0!\n");
			return -EIO;
		}
		/* t(frame)/t(per line) = t(exposure, in lines)*/
		exposure_time_q9 = exposure_time_q9 * pll_table->pixclk;
	} else {
		frame_length = pinfo->xvs_num;
		line_length = imx185_video_format_tbl.table[format_index].h_pixel;
		/* t(frame)/t(per line) = t(exposure, in lines)*/
		exposure_time_q9 = exposure_time_q9 * imx185_video_format_tbl.table[format_index].data_rate;
	}

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	blank_lines = frame_length - (u16)exposure_time_q9; /* get the shutter sweep time */

	if (unlikely(blank_lines < 0)) {
		vin_dbg("Exposure time is too long, exceed:%d\n", -blank_lines);
		blank_lines = 0;
	}

	shr_width_h = blank_lines >> 16;
	shr_width_m = blank_lines >> 8;
	shr_width_l = blank_lines & 0xff;

	imx185_write_reg(src, IMX185_SHS1_HSB, shr_width_h);
	imx185_write_reg(src, IMX185_SHS1_MSB, shr_width_m);
	imx185_write_reg(src, IMX185_SHS1_LSB, shr_width_l);

	vin_dbg("frame_length: %d, sweep lines:%d\n", frame_length, blank_lines);
	pinfo->current_shutter_time = shutter_time;
	return 0;
}
static int imx185_set_shutter_row_sync( struct __amba_vin_source *src, u32 shutter_row)
{
	u8 	shr_width_h, shr_width_m, shr_width_l;
	int	blank_lines;
	u32 	frame_length;
	struct imx185_info		*pinfo;

	pinfo = (struct imx185_info *)src->pinfo;

	if (!slave_mode) {
		imx185_read_reg(src, IMX185_VMAX_HSB, &shr_width_h);
		imx185_read_reg(src, IMX185_VMAX_MSB, &shr_width_m);
		imx185_read_reg(src, IMX185_VMAX_LSB, &shr_width_l);

		frame_length = ((shr_width_h&0x01)<<16) + (shr_width_m<<8) + shr_width_l;
	} else
		frame_length = pinfo->xvs_num;

	blank_lines = frame_length - shutter_row; /* get the shutter sweep time */

	if (unlikely(blank_lines < 0)) {
		vin_dbg("Exposure time is too long\n");
		blank_lines = 0;
	}

	shr_width_h = blank_lines >> 16;
	shr_width_m = blank_lines >> 8;
	shr_width_l = blank_lines & 0xff;

	imx185_write_reg(src, IMX185_SHS1_HSB, shr_width_h);
	imx185_write_reg(src, IMX185_SHS1_MSB, shr_width_m);
	imx185_write_reg(src, IMX185_SHS1_LSB, shr_width_l);

	vin_dbg("frame_length: %d, sweep lines:%d\n", frame_length, blank_lines);
	return 0;
}
static int imx185_set_shutter_time_row( struct __amba_vin_source *src, u32 row)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length, format_index;
	int 	shutter_width;
	u8 	shr_width_h, shr_width_m, shr_width_l;

	const struct imx185_pll_reg_table *pll_table;
	struct imx185_info		*pinfo;
	pinfo = (struct imx185_info *)src->pinfo;

	format_index = imx185_video_info_table[pinfo->current_video_index].format_index;
	pll_table = &imx185_pll_tbl[pinfo->pll_index];

	if (!slave_mode) {
		imx185_read_reg(src, IMX185_VMAX_HSB, &shr_width_h);
		imx185_read_reg(src, IMX185_VMAX_MSB, &shr_width_m);
		imx185_read_reg(src, IMX185_VMAX_LSB, &shr_width_l);
		frame_length = ((shr_width_h&0x01)<<16) + (shr_width_m<<8) + shr_width_l;

		imx185_read_reg(src, IMX185_HMAX_MSB, &shr_width_m);
		imx185_read_reg(src, IMX185_HMAX_LSB, &shr_width_l);
		line_length = ((shr_width_m)<<8) + shr_width_l;
		if(unlikely(!line_length)) {
			vin_err("line length is 0!\n");
			return -EIO;
		}
	} else {
		frame_length = pinfo->xvs_num;
		line_length = imx185_video_format_tbl.table[format_index].h_pixel;
	}

	shutter_width = row;

	/* FIXME: shutter width: 1 ~ Frame format(V) */
	if((shutter_width < 1) ||(shutter_width  > frame_length) ) {
		vin_warn("row number %d err, valid range is %d - %d\n", shutter_width, 1, frame_length);
		return -EPERM;
	}

	imx185_write_reg(src, IMX185_SHS1_HSB, (frame_length - shutter_width) >> 16);
	imx185_write_reg(src, IMX185_SHS1_MSB, (frame_length - shutter_width) >> 8);
	imx185_write_reg(src, IMX185_SHS1_LSB, (frame_length - shutter_width) & 0xFF);

	exposure_time_q9 = shutter_width;

	exposure_time_q9 = exposure_time_q9 * (u64)line_length * 512000000;
	DO_DIV_ROUND(exposure_time_q9, pll_table->pixclk);

	pinfo->current_shutter_time = (u32)exposure_time_q9;

	vin_dbg("shutter_time:%d, row:%d\n", pinfo->current_shutter_time, shutter_width);

	return 0;
}


static int imx185_shutter_time2width(struct __amba_vin_source *src, u32* shutter_time)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length, format_index;
	u8 	shr_width_h, shr_width_m, shr_width_l;

	const struct imx185_pll_reg_table *pll_table;
	struct imx185_info		*pinfo;
	pinfo = (struct imx185_info *)src->pinfo;

	format_index = imx185_video_info_table[pinfo->current_video_index].format_index;
	pll_table = &imx185_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = *shutter_time;

	if (!slave_mode) {
		imx185_read_reg(src, IMX185_VMAX_HSB, &shr_width_h);
		imx185_read_reg(src, IMX185_VMAX_MSB, &shr_width_m);
		imx185_read_reg(src, IMX185_VMAX_LSB, &shr_width_l);

		frame_length = ((shr_width_h&0x01)<<16) + (shr_width_m<<8) + shr_width_l;

		imx185_read_reg(src, IMX185_HMAX_MSB, &shr_width_m);
		imx185_read_reg(src, IMX185_HMAX_LSB, &shr_width_l);

		line_length = ((shr_width_m)<<8) + shr_width_l;
		if(unlikely(!line_length)) {
			vin_err("line length is 0!\n");
			return -EIO;
		}

		/* t(frame)/t(per line) = t(exposure, in lines)*/
		exposure_time_q9 = exposure_time_q9 * pll_table->pixclk;
	} else {
		frame_length = pinfo->xvs_num;
		line_length = imx185_video_format_tbl.table[format_index].h_pixel;
		/* t(frame)/t(per line) = t(exposure, in lines)*/
		exposure_time_q9 = exposure_time_q9 * imx185_video_format_tbl.table[format_index].data_rate;
	}

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	*shutter_time =exposure_time_q9;

	return 0;
}

static int imx185_set_shutter_and_agc_sync( struct __amba_vin_source *src, struct sensor_cmd_info_t * info)
{
	int errCode = 0;
	u64 	exposure_time_q9;
	struct imx185_info *pinfo;
	u32 vsync_irq;
	u32 	frame_length, line_length, format_index;
	int 	shutter_width;
	u8 	shr_width_h, shr_width_m, shr_width_l;
	const struct imx185_pll_reg_table *pll_table;
	pinfo = (struct imx185_info *) src->pinfo;
	vsync_irq= vsync_irq_flag;
	pinfo->agc_call_cnt++;

	format_index = imx185_video_info_table[pinfo->current_video_index].format_index;
	pll_table = &imx185_pll_tbl[pinfo->pll_index];

	if (!slave_mode) {
		imx185_read_reg(src, IMX185_VMAX_HSB, &shr_width_h);
		imx185_read_reg(src, IMX185_VMAX_MSB, &shr_width_m);
		imx185_read_reg(src, IMX185_VMAX_LSB, &shr_width_l);
		frame_length = ((shr_width_h&0x01)<<16) + (shr_width_m<<8) + shr_width_l;

		imx185_read_reg(src, IMX185_HMAX_MSB, &shr_width_m);
		imx185_read_reg(src, IMX185_HMAX_LSB, &shr_width_l);
		line_length = ((shr_width_m)<<8) + shr_width_l;
		if(unlikely(!line_length)) {
			vin_err("line length is 0!\n");
			return -EIO;
		}
	} else {
		frame_length = pinfo->xvs_num;
		line_length = imx185_video_format_tbl.table[format_index].h_pixel;
	}

	shutter_width = info->shutter_row;

	/* FIXME: shutter width: 1 ~ Frame format(V) */
	if((shutter_width < 1) ||(shutter_width  > frame_length) ) {
		vin_warn("row number %d err, valid range is %d - %d\n", shutter_width, 1, frame_length);
		return -EPERM;
	}


	wait_event_interruptible(vsync_irq_wait, vsync_irq < vsync_irq_flag || pinfo->agc_call_cnt < 7);//FIXME
	if ((info->gain_tbl_index>= pinfo->min_agc_index) && (info->gain_tbl_index <= pinfo->max_agc_index)) {
		imx185_write_reg(src, IMX185_GAIN_LSB, info->gain_tbl_index);
	} else {
		errCode = -1;	/* Index is out of range */
		vin_err("gain index %d exceeds limitation [%d~%d]\n", info->gain_tbl_index,
			pinfo->min_agc_index, pinfo->max_agc_index);
	}

	imx185_write_reg(src, IMX185_SHS1_HSB, (frame_length - shutter_width) >> 16);
	imx185_write_reg(src, IMX185_SHS1_MSB, (frame_length - shutter_width) >> 8);
	imx185_write_reg(src, IMX185_SHS1_LSB, (frame_length - shutter_width) & 0xFF);

	exposure_time_q9 = shutter_width;
	exposure_time_q9 = exposure_time_q9 * (u64)line_length * 512000000;
	if (!slave_mode)
		DO_DIV_ROUND(exposure_time_q9, pll_table->pixclk);
	else
		DO_DIV_ROUND(exposure_time_q9, imx185_video_format_tbl.table[format_index].data_rate);

	pinfo->current_shutter_time = (u32)exposure_time_q9;

	vin_dbg("shutter_time:%d, row:%d\n", pinfo->current_shutter_time, shutter_width);
	return errCode;
}

static int imx185_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	int errCode = 0;
	u8 reg_d;
	struct imx185_info *pinfo;
	s32 db_max;
	s32 db_step;

	pinfo = (struct imx185_info *) src->pinfo;

	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	vin_dbg("imx185_set_agc: 0x%x\n", agc_db);

	if (agc_db > db_max)
		agc_db = db_max;

	reg_d = agc_db / db_step;
	errCode = imx185_write_reg(src, IMX185_GAIN_LSB, reg_d&0xFF);

	if(errCode == 0)
		pinfo->current_gain_db = agc_db;

	return errCode;
}
static int imx185_set_agc_index( struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct imx185_info *pinfo;

	pinfo = (struct imx185_info *) src->pinfo;

	if ((index >= pinfo->min_agc_index) && (index <= pinfo->max_agc_index)) {
		imx185_write_reg(src, IMX185_GAIN_LSB, index);
	} else {
		errCode = -1;	/* Index is out of range */
		vin_err("gain index %d exceeds limitation [%d~%d]\n", index,
			pinfo->min_agc_index, pinfo->max_agc_index);
	}
	return errCode;
}
static int imx185_set_agc_index_sync( struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct imx185_info *pinfo;
	u32 vsync_irq;
	pinfo = (struct imx185_info *) src->pinfo;
	vsync_irq= vsync_irq_flag;
	pinfo->agc_call_cnt++;
	wait_event_interruptible(vsync_irq_wait, vsync_irq < vsync_irq_flag || pinfo->agc_call_cnt < 7);//FIXME
	if ((index >= pinfo->min_agc_index) && (index <= pinfo->max_agc_index)) {
		imx185_write_reg(src, IMX185_GAIN_LSB, index);
	} else {
		errCode = -1;	/* Index is out of range */
		vin_err("gain index %d exceeds limitation [%d~%d]\n", index,
			pinfo->min_agc_index, pinfo->max_agc_index);
	}
	return errCode;
}

static int imx185_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int		errCode = 0;
	u32		readmode;
	struct	imx185_info			*pinfo;
	u32		target_bayer_pattern;
	u16		tmp_reg;

	pinfo = (struct imx185_info *) src->pinfo;

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
		readmode = IMX185_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GB;
		break;

	case AMBA_VIN_SRC_MIRROR_NONE:
		readmode = 0;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX185_H_MIRROR | IMX185_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		readmode = IMX185_H_MIRROR;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
		break;

	default:
		vin_err("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	errCode |= imx185_read_reg(src, IMX185_WINMODE, (u8 *)&tmp_reg);
	tmp_reg &= ~(IMX185_H_MIRROR | IMX185_V_FLIP);
	tmp_reg |= readmode;
	errCode |= imx185_write_reg(src, IMX185_WINMODE, tmp_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}

static int imx185_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int					errCode = 0;
	u32					frame_time = 0;
	u64					frame_time_pclk;
	u32					vertical_lines = 0;
	u32					line_length = 0;
	u32					index;
	u32					format_index;
	u8					data_val;
	u8					current_pll_index = 0;
	const struct imx185_pll_reg_table 	*pll_table;
	struct imx185_info			*pinfo;
	struct amba_vin_irq_fix			vin_irq_fix;
	struct amba_vin_HV	master_HV;

	pinfo = (struct imx185_info *)src->pinfo;
	index = pinfo->current_video_index;

	if (index >= IMX185_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx185_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto imx185_set_fps_exit;
	}

	format_index = imx185_video_info_table[index].format_index;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = imx185_video_format_tbl.table[format_index].auto_fps;
	if(fps < imx185_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, imx185_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto imx185_set_fps_exit;
	}

	frame_time = fps;

	/* ToDo: Add specified PLL index*/
	current_pll_index = 0;

	if(pinfo->pll_index != current_pll_index){
		pinfo->pll_index = current_pll_index;
		imx185_fill_pll_regs(src);
		errCode = imx185_init_vin_clock(src, &imx185_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto imx185_set_fps_exit;
	}

	pll_table = &imx185_pll_tbl[pinfo->pll_index];

	if (!slave_mode) {
		errCode |= imx185_read_reg(src, IMX185_HMAX_MSB, &data_val);
		line_length = (u32)(data_val) <<  8;
		errCode |= imx185_read_reg(src, IMX185_HMAX_LSB, &data_val);
		line_length += (u32)data_val;
		if(unlikely(!line_length)) {
			vin_err("line length is 0!\n");
			return -EIO;
		}
	} else
		line_length = imx185_video_format_tbl.table[format_index].h_pixel;

	vin_dbg("line_length is  0x%0x\n", line_length);

	if (!slave_mode)
		frame_time_pclk = frame_time * (u64)pll_table->pixclk;
	else
		frame_time_pclk = frame_time * (u64)imx185_video_format_tbl.table[format_index].data_rate;


	vin_dbg("pixclk %d, frame_time_pclk 0x%llx\n", pll_table->pixclk, frame_time_pclk);

	DO_DIV_ROUND(frame_time_pclk, line_length);
	DO_DIV_ROUND(frame_time_pclk, 512000000);

	vertical_lines = frame_time_pclk;

	vin_dbg("vertical_lines is  0x%05x\n", vertical_lines);

	if (!slave_mode) {
		errCode |= imx185_write_reg(src, IMX185_VMAX_HSB, (u8)((vertical_lines & 0x030000) >> 16));
		errCode |= imx185_write_reg(src, IMX185_VMAX_MSB, (u8)((vertical_lines & 0x00FF00) >> 8));
		errCode |= imx185_write_reg(src, IMX185_VMAX_LSB, (u8)(vertical_lines & 0x0000FF));
	} else {
		/* master vsync width is only 14-bits, so we must increase frame-rate if it exceeds */
		if(vertical_lines > 0x3FFF) {
			vin_warn("vsync width is overflow, please increase the frame rate\n");
			return -EPERM;
		}
		master_HV.pel_clk_a_line = line_length;
		master_HV.line_num_a_field = vertical_lines;
		pinfo->xvs_num = master_HV.line_num_a_field;
		imx185_set_master_HV(src, master_HV);
		vin_dbg("H:%d, V:%d\n", line_length, vertical_lines);
	}

	imx185_set_shutter_time(src, pinfo->current_shutter_time);// keep the same shutter time

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_irq_fix.delay = 0;// change to zero for all non Aptina Hi-SPI sensors (for A5s/A7/S2)

	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

imx185_set_fps_exit:
	return errCode;
}

static void imx185_start_streaming(struct __amba_vin_source *src)
{
	imx185_write_reg(src, IMX185_STANDBY, 0x00); //cancel standby
	msleep(10);

	if (!slave_mode)
		imx185_write_reg(src, IMX185_XMSTA, 0x00); //master mode start
}

static int imx185_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int				errCode = 0;
	struct imx185_info		*pinfo;
	u32				format_index;

	pinfo = (struct imx185_info *)src->pinfo;

	if (index >= IMX185_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx185_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto imx185_set_mode_exit;
	}

	errCode |= imx185_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = imx185_video_info_table[index].format_index;

	pinfo->cap_start_x 	= imx185_video_info_table[index].def_start_x;
	pinfo->cap_start_y 	= imx185_video_info_table[index].def_start_y;
	pinfo->cap_cap_w 	= imx185_video_info_table[index].def_width;
	pinfo->cap_cap_h 	= imx185_video_info_table[index].def_height;
	pinfo->bayer_pattern 	= imx185_video_info_table[index].bayer_pattern;
	pinfo->pll_index 	= imx185_video_format_tbl.table[format_index].pll_index;

	imx185_print_info(src);

	//set clk_si
	errCode |= imx185_init_vin_clock(src, &imx185_pll_tbl[pinfo->pll_index]);

	errCode |= imx185_set_vin_mode(src);

	imx185_fill_pll_regs(src);

	imx185_fill_share_regs(src);

	imx185_fill_video_format_regs(src);
	imx185_set_fps(src, AMBA_VIDEO_FPS_AUTO);

	imx185_set_shutter_time(src, AMBA_VIDEO_FPS_60);
	imx185_set_agc_db(src, 0);

	errCode |= imx185_post_set_vin_mode(src);

	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

	/* TG reset release ( Enable Streaming )*/
	imx185_start_streaming(src);

	//imx185_dump_reg(src);

imx185_set_mode_exit:
	return errCode;
}

static int imx185_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int				errCode = -EINVAL;
	int				i;
	struct imx185_info		*pinfo;

	static int			first_set_video_mode = 1;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct imx185_info *)src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errCode = imx185_init_vin_clock(src, &imx185_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto imx185_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	imx185_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	if (mode == AMBA_VIDEO_MODE_AUTO) {
		mode = IMX185_VIDEO_MODE_TABLE_AUTO;
	}

	for (i = 0; i < IMX185_VIDEO_MODE_TABLE_SIZE; i++) {
		if (imx185_video_mode_table[i].mode == mode) {
			errCode = imx185_set_video_index(src, imx185_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= IMX185_VIDEO_MODE_TABLE_SIZE) {
		vin_err("imx185_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = imx185_video_mode_table[i].mode;
		pinfo->mode_type = imx185_video_mode_table[i].preview_mode_type;
	}

imx185_set_video_mode_exit:
	return errCode;
}

static int imx185_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	struct imx185_info		*pinfo;

	pinfo = (struct imx185_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = imx185_init_vin_clock(src, &imx185_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto imx185_init_hw_exit;

	msleep(10);
	imx185_reset(src);

imx185_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int imx185_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{
	return 0;
}

static int imx185_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
	/* >> TODO, for RGB mode only << */
	return 0;
}

static int imx185_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
	/* >> TODO, for YUV output mode only << */
	return errCode;
}

static int imx185_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;
	/* >> TODO, for RGB raw output mode only << */
	/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}

static int imx185_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct imx185_info		*pinfo;

	pinfo = (struct imx185_info *) src->pinfo;

	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		imx185_reset(src);
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
		errCode = imx185_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO: {
		struct amba_vin_source_info *pub_src;

		pub_src = (struct amba_vin_source_info *) args;
		pub_src->id = src->id;
		pub_src->sensor_id = SENSOR_IMX185;
		pub_src->adapter_id = adapter_id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = imx185_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = imx185_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = imx185_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = imx185_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = imx185_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = imx185_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = imx185_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = imx185_set_still_mode(src, (struct amba_vin_src_still_info *) args);
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
		errCode = imx185_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = imx185_set_agc_db(src, *(s32 *) args);
		break;
	case AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH:
		errCode=imx185_shutter_time2width(src, (u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(s32*)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = imx185_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = imx185_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = imx185_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = imx185_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		imx185_dump_reg(src);
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

		errCode = imx185_read_reg(src, subaddr, &data);

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

		errCode = imx185_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = imx185_set_slowshutter_mode(src, *(int *) args);
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW:
		errCode = imx185_set_shutter_time_row(src, *(u32 *) args);
		break;
	case AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW_SYNC:
		errCode = imx185_set_shutter_row_sync(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_INDEX:
		errCode = imx185_set_agc_index(src, *(u32 *) args);
		break;
	case AMBA_VIN_SRC_SET_GAIN_INDEX_SYNC:
		errCode = imx185_set_agc_index_sync(src, *(u32 *) args);
		break;
	case AMBA_VIN_SRC_SET_SHUTTER_AND_GAIN_SYNC:
		errCode = imx185_set_shutter_and_agc_sync(src,(struct sensor_cmd_info_t *) args);
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
