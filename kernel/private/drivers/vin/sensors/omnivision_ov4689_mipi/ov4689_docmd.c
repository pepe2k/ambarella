/*
 * kernel/private/drivers/ambarella/vin/sensors/omnivision_ov4689/ov4689_docmd.c
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

static void ov4689_dump_reg(struct __amba_vin_source *src)
{
}

static void ov4689_fill_video_format_regs(struct __amba_vin_source *src)
{
	int			i, ov4689_format_regs_size;
	u32			index;
	u32			format_index;
	struct ov4689_info	*pinfo;
	const struct ov4689_reg_table *reg_tbl;

	pinfo = (struct ov4689_info *)src->pinfo;

	vin_dbg("ov4689_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = ov4689_video_info_table[index].format_index;

	for (i = 0; i < OV4689_VIDEO_FORMAT_REG_NUM; i++) {
		if (ov4689_video_format_tbl.reg[i] == 0)
			break;

		ov4689_write_reg(src,
			ov4689_video_format_tbl.reg[i],
			ov4689_video_format_tbl.table[format_index].data[i]);
	}

	if (ov4689_video_format_tbl.table[format_index].ext_reg_fill)
		ov4689_video_format_tbl.table[format_index].ext_reg_fill(src);

	switch(format_index) {
	case 0: // 4M
		if(lane == 4) {
			reg_tbl = ov4689_4lane_4m_regs;
			ov4689_format_regs_size = OV4689_4LANE_4M_REG_SIZE;
			vin_info("4 lane 4M\n");
		} else {
			reg_tbl = ov4689_2lane_4m_regs;
			ov4689_format_regs_size = OV4689_2LANE_4M_REG_SIZE;
			vin_info("2 lane 4M\n");
		}
		break;
	case 3: // 2lane, 1080p
		reg_tbl = ov4689_2lane_1080p_regs;
		ov4689_format_regs_size = OV4689_2LANE_1080P_REG_SIZE;
		vin_info("2 lane 1080p\n");
		break;
	case 4: // 2lane, 720p
		reg_tbl = ov4689_2lane_720p_regs;
		ov4689_format_regs_size = OV4689_2LANE_720P_REG_SIZE;
		vin_info("2 lane 720p\n");
		break;
	case 2: // 3x wdr
		reg_tbl = ov4689_3x_hdr_regs;
		ov4689_format_regs_size = OV4689_3X_HDR_REG_SIZE;
		break;
	case 5: // 3x 4M wdr
		reg_tbl = ov4689_3x_4m_hdr_regs;
		ov4689_format_regs_size = OV4689_3X_4M_HDR_REG_SIZE;
		break;
	default:
		reg_tbl = NULL;
		ov4689_format_regs_size = 0;
		break;
	}

	for (i = 0; i < ov4689_format_regs_size; i++){
		if(reg_tbl[i].reg == 0xFFFF) {
			msleep(reg_tbl[i].data);
		} else {
			ov4689_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
		}
	}

}

static void ov4689_fill_share_regs(struct __amba_vin_source *src)
{
	int i, ov4689_share_regs_size;
	const struct ov4689_reg_table *reg_tbl;
	struct ov4689_info	*pinfo = (struct ov4689_info *) src->pinfo;

	vin_dbg("OV4689 fill share regs\n");

	switch (pinfo->op_mode) {
	case OV4689_2X_WDR_MODE:
		ov4689_share_regs_size = OV4689_2X_HDR_REG_SIZE;
		reg_tbl = ov4689_2x_hdr_regs;
		vin_info("2x WDR mode\n");
		break;
	case OV4689_3X_WDR_MODE:
		ov4689_share_regs_size = 0;
		reg_tbl = NULL;
		vin_info("3x WDR mode\n");
		break;
	case OV4689_LINEAR_MODE:
	default:
		vin_info("Linear mode\n");
		ov4689_share_regs_size = 0;
		reg_tbl = NULL;
		break;
	}

	for (i = 0; i < ov4689_share_regs_size; i++){
		if(reg_tbl[i].reg == 0xFFFF) {
			msleep(reg_tbl[i].data);
		} else {
			ov4689_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
		}
	}
}

static void ov4689_sw_reset(struct __amba_vin_source *src)
{
	struct ov4689_info		*pinfo;

	pinfo = (struct ov4689_info *)src->pinfo;
	ov4689_write_reg(src, OV4689_SWRESET, 0x01);
	ov4689_write_reg(src, OV4689_STANDBY, 0x00);/* Standby */
}

static void ov4689_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	ov4689_sw_reset(src);
}

static void ov4689_fill_pll_regs(struct __amba_vin_source *src)
{
	int i;
	struct ov4689_info *pinfo;
	const struct ov4689_pll_reg_table 	*pll_reg_table;
	const struct ov4689_reg_table 		*pll_tbl;

	pinfo = (struct ov4689_info *) src->pinfo;

	vin_dbg("ov4689_fill_pll_regs\n");
	pll_reg_table = &ov4689_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < OV4689_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			ov4689_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
	msleep(2);
}

static int ov4689_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct ov4689_info  *pinfo;

	pinfo = (struct ov4689_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int ov4689_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int	errCode = 0;
	u32	index;
	u32	format_index;
	struct ov4689_info *pinfo;

	pinfo = (struct ov4689_info *)src->pinfo;

	index = pinfo->current_video_index;

	if (index >= OV4689_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = ov4689_video_info_table[index].format_index;

		p_video_info->width = ov4689_video_info_table[index].act_width;
		p_video_info->height = ov4689_video_info_table[index].act_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = ov4689_video_format_tbl.table[format_index].format;
		p_video_info->type = ov4689_video_format_tbl.table[format_index].type;
		p_video_info->bits = ov4689_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = ov4689_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;

}

static int ov4689_get_vblank_time( struct __amba_vin_source *src,
	u32 *ptime)
{
	int				errCode		= 0;
	int				mode_index;
	u32				format_index;
	u64				v_btime;
	u32				active_lines, frame_length_lines;
	u32				line_length;
	u8				data_l, data_h;

	struct ov4689_info 			*pinfo;
	const struct ov4689_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct ov4689_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= ov4689_video_info_table[mode_index].format_index;
	pll_reg_table	= &ov4689_pll_tbl[pinfo->pll_index];

	errCode |= ov4689_read_reg(src, OV4689_HTS_MSB, &data_h);
	errCode |= ov4689_read_reg(src, OV4689_HTS_LSB, &data_l);
	line_length = (data_h<<8) + data_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= ov4689_read_reg(src, OV4689_VTS_MSB, &data_h);
	errCode |= ov4689_read_reg(src, OV4689_VTS_LSB, &data_l);
	frame_length_lines = (data_h<<8) + data_l;

	if (pinfo->op_mode == OV4689_2X_WDR_MODE) {
		frame_length_lines = frame_length_lines * 2;
	} else if (pinfo->op_mode == OV4689_3X_WDR_MODE) {
		frame_length_lines = frame_length_lines * 3;
	}

	active_lines = ov4689_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pll_reg_table->pixclk); //ns
	*ptime = v_btime;

	vin_dbg("ov4689_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return errCode;
}

static int ov4689_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct ov4689_info *pinfo;

	pinfo = (struct ov4689_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}

static int ov4689_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));
	return 0;
}

static int ov4689_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;
	u32 video_tbl_size;
	struct ov4689_info	*pinfo = (struct ov4689_info *) src->pinfo;
	struct ov4689_video_mode *video_mode_tbl;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	switch(pinfo->op_mode) {
		case OV4689_LINEAR_MODE:
			if (lane == 2) {
				video_tbl_size = OV4689_VIDEO_MODE_2LANE_TABLE_SIZE;
				video_mode_tbl = ov4689_video_mode_2lane_table;
			} else {
				video_tbl_size = OV4689_VIDEO_MODE_4LANE_TABLE_SIZE;
				video_mode_tbl = ov4689_video_mode_4lane_table;
			}
			break;
		case OV4689_2X_WDR_MODE:
			video_tbl_size = OV4689_VIDEO_MODE_2X_TABLE_SIZE;
			video_mode_tbl = ov4689_video_mode_2x_table;
			break;
		case OV4689_3X_WDR_MODE:
			video_tbl_size = OV4689_VIDEO_MODE_3X_TABLE_SIZE;
			video_mode_tbl = ov4689_video_mode_3x_table;
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
				p_mode_info->mode = OV4689_VIDEO_MODE_TABLE_AUTO;

			p_mode_info->is_supported = 1;

			index = video_mode_tbl[i].still_index;
			format_index = ov4689_video_info_table[index].format_index;

			p_mode_info->video_info.width = ov4689_video_info_table[index].act_width;
			p_mode_info->video_info.height = ov4689_video_info_table[index].act_height;
			if(lane == 2)
				p_mode_info->video_info.fps = ov4689_video_format_tbl.table[format_index].max_fps << 1;
			else
				p_mode_info->video_info.fps = ov4689_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = ov4689_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = ov4689_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = ov4689_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = ov4689_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;
			break;
		}
	}

	return errCode;
}

static int ov4689_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length;
	int	shutter_width;
	u8	data_l, data_h;

	const struct ov4689_pll_reg_table *pll_table;
	struct ov4689_info		*pinfo;
	pinfo = (struct ov4689_info *)src->pinfo;

	pll_table = &ov4689_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time;

	ov4689_read_reg(src, OV4689_VTS_MSB, &data_h);
	ov4689_read_reg(src, OV4689_VTS_LSB, &data_l);
	frame_length = (data_h<<8) + data_l;

	ov4689_read_reg(src, OV4689_HTS_MSB, &data_h);
	ov4689_read_reg(src, OV4689_HTS_LSB, &data_l);
	line_length = ((data_h)<<8) + data_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	/* t(frame)/t(per line) = t(exposure, in lines)*/
	exposure_time_q9 = exposure_time_q9 * pll_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (u32)exposure_time_q9;

	/* FIXME: shutter width: 4 ~ (Frame format(V) - 4) */
	if(shutter_width < 4) {
		shutter_width = 4;
	} else if(shutter_width  > frame_length - 4) {
		shutter_width = frame_length - 4;
	}

	vin_dbg("V:%d, shutter:%d\n", frame_length, shutter_width);

	shutter_width <<= 4; //the register value should be exposure time * 16

	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x01); /* group latch */
	ov4689_write_reg(src, OV4689_L_EXPO_HSB, (u8)((shutter_width >> 16) & 0xF));
	ov4689_write_reg(src, OV4689_L_EXPO_MSB, (u8)(shutter_width >> 8));
	ov4689_write_reg(src, OV4689_L_EXPO_LSB, (u8)(shutter_width & 0xFF));
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x11); /* group latch */
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0xE1); /* group latch */

	pinfo->current_shutter_time = shutter_time;
	return 0;
}

static int ov4689_shutter_time2width(struct __amba_vin_source *src, u32* shutter_time)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length;
	int	shutter_width;
	u8	data_l, data_h;

	const struct ov4689_pll_reg_table *pll_table;
	struct ov4689_info		*pinfo;
	pinfo = (struct ov4689_info *)src->pinfo;

	pll_table = &ov4689_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = *shutter_time;

	ov4689_read_reg(src, OV4689_VTS_MSB, &data_h);
	ov4689_read_reg(src, OV4689_VTS_LSB, &data_l);
	frame_length = (data_h<<8) + data_l;

	ov4689_read_reg(src, OV4689_HTS_MSB, &data_h);
	ov4689_read_reg(src, OV4689_HTS_LSB, &data_l);
	line_length = ((data_h)<<8) + data_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	/* t(frame)/t(per line) = t(exposure, in lines)*/
	exposure_time_q9 = exposure_time_q9 * pll_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (u32)exposure_time_q9;

	/* FIXME: shutter width: 4 ~ (Frame format(V) - 4) */
	if(shutter_width < 4) {
		shutter_width = 4;
	} else if(shutter_width  > frame_length - 4) {
		shutter_width = frame_length - 4;
	}

	*shutter_time=shutter_width;
	return 0;
}

static int ov4689_set_shutter_time_row(struct __amba_vin_source *src, u32 row)
{
	u64 exposure_time_q9;
	u8 data_h, data_l;
	u32 line_length, frame_length;
	int shutter_width;
	int errCode = 0;

	struct ov4689_info *pinfo;
	const struct ov4689_pll_reg_table *pll_reg_table;

	pinfo		= (struct ov4689_info *)src->pinfo;
	pll_reg_table	= &ov4689_pll_tbl[pinfo->pll_index];

	ov4689_read_reg(src, OV4689_HTS_MSB, &data_h);
	ov4689_read_reg(src, OV4689_HTS_LSB, &data_l);
	line_length = ((data_h)<<8) + data_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	ov4689_read_reg(src, OV4689_VTS_MSB, &data_h);
	ov4689_read_reg(src, OV4689_VTS_LSB, &data_l);
	frame_length = (data_h<<8) + data_l;

	shutter_width = row;

	/* FIXME: shutter width: 4 ~(Frame format(V) - 4) */
	if((shutter_width < 4) ||(shutter_width  > frame_length - 4)) {
		vin_warn("row number %d err, valid range is %d - %d\n", shutter_width, 4, frame_length - 4);
		return -EPERM;
	}

	vin_dbg("V:%d, shutter:%d\n", frame_length, shutter_width);

	shutter_width <<= 4; //the register value should be exposure time * 16
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x01); /* group latch */
	ov4689_write_reg(src, OV4689_L_EXPO_HSB, (u8)((shutter_width >> 16) & 0xF));
	ov4689_write_reg(src, OV4689_L_EXPO_MSB, (u8)(shutter_width >> 8));
	ov4689_write_reg(src, OV4689_L_EXPO_LSB, (u8)(shutter_width & 0xFF));
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x11); /* group latch */
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0xE1); /* group latch */

	exposure_time_q9 = shutter_width >> 4;

	exposure_time_q9 = exposure_time_q9 * (u64)line_length * 512000000;
	DO_DIV_ROUND(exposure_time_q9, pll_reg_table->pixclk);

	pinfo->current_shutter_time = (u32)exposure_time_q9;

	vin_dbg("shutter_time:%d, row:%d\n", pinfo->current_shutter_time, shutter_width);

	return errCode;
}

static int ov4689_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	int errCode = 0;
	struct ov4689_info *pinfo;
	u32 gain_index;
	s32 db_max;
	s32 db_step;

	pinfo = (struct ov4689_info *) src->pinfo;

	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	gain_index = (db_max - agc_db) / db_step;

	if(gain_index < 0 ){
		gain_index = 0;
	}else if(gain_index >= OV4689_GAIN_ROWS){
		gain_index = OV4689_GAIN_ROWS - 1;
	}

	vin_dbg("ov4689_set_agc: 0x%x, index:%d\n", agc_db, gain_index);

	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		ov4689_write_reg(src, OV4689_L_GAIN_MSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_REG3508]);
		ov4689_write_reg(src, OV4689_L_GAIN_LSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_REG3509]);
		/* WB-R */
		ov4689_write_reg(src, OV4689_L_WB_R_GAIN_MSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_L_WB_R_GAIN_LSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_LSB]);
		/* WB-G */
		ov4689_write_reg(src, OV4689_L_WB_G_GAIN_MSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_L_WB_G_GAIN_LSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_LSB]);
		/* WB-B */
		ov4689_write_reg(src, OV4689_L_WB_B_GAIN_MSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_L_WB_B_GAIN_LSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_LSB]);

		pinfo->current_gain_db = agc_db;
	} else {
		errCode = -1;	/* Index is out of range */
	}
	return errCode;
}

static int ov4689_set_agc_index(struct __amba_vin_source *src, u32 index)
{
	struct ov4689_info *pinfo;
	u32 gain_index;

	vin_dbg("ov4689_set_agc_index: %d\n", index);

	pinfo = (struct ov4689_info *) src->pinfo;

	gain_index = OV4689_GAIN_0DB - index;

	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		ov4689_write_reg(src, OV4689_L_GAIN_MSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_REG3508]);
		ov4689_write_reg(src, OV4689_L_GAIN_LSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_REG3509]);
		/* WB-R */
		ov4689_write_reg(src, OV4689_L_WB_R_GAIN_MSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_L_WB_R_GAIN_LSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_LSB]);
		/* WB-G */
		ov4689_write_reg(src, OV4689_L_WB_G_GAIN_MSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_L_WB_G_GAIN_LSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_LSB]);
		/* WB-B */
		ov4689_write_reg(src, OV4689_L_WB_B_GAIN_MSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_L_WB_B_GAIN_LSB, OV4689_GAIN_TABLE[gain_index][OV4689_GAIN_COL_DGAIN_LSB]);

		return 0;
	} else {
		return -1;
	}

	return 0;
}

static int ov4689_set_wdr_dgain_index_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_gain_gp_info *p_dgain_gp)
{
	u32 dgain_index;
	struct ov4689_info *pinfo;

	pinfo = (struct ov4689_info *) src->pinfo;

	if (p_dgain_gp->long_gain > OV4689_DGAIN_0DB) {
		vin_warn("long dgain index %d exceeds maximum %d\n", p_dgain_gp->long_gain, OV4689_DGAIN_0DB);
		p_dgain_gp->long_gain = OV4689_DGAIN_0DB;
	}
	if (p_dgain_gp->short1_gain > OV4689_DGAIN_0DB) {
		vin_warn("short1 dgain index %d exceeds maximum %d\n", p_dgain_gp->short1_gain, OV4689_DGAIN_0DB);
		p_dgain_gp->short1_gain = OV4689_DGAIN_0DB;
	}
	if (p_dgain_gp->short2_gain > OV4689_DGAIN_0DB) {
		vin_warn("short2 dgain index %d exceeds maximum %d\n", p_dgain_gp->short2_gain, OV4689_DGAIN_0DB);
		p_dgain_gp->short2_gain = OV4689_DGAIN_0DB;
	}

	/* long frame */
	dgain_index = OV4689_DGAIN_0DB - p_dgain_gp->long_gain;
	if ((dgain_index >= pinfo->min_wdr_dgain_index) && (dgain_index <= pinfo->max_wdr_dgain_index)) {
		/* WB-R */
		ov4689_write_reg(src, OV4689_L_WB_R_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_L_WB_R_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
		/* WB-G */
		ov4689_write_reg(src, OV4689_L_WB_G_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_L_WB_G_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
		/* WB-B */
		ov4689_write_reg(src, OV4689_L_WB_B_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_L_WB_B_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
	} else {
		vin_err("long frame again exceeds limitation[%d, %d]: %d!\n",
			pinfo->min_wdr_dgain_index, pinfo->max_wdr_dgain_index, dgain_index);
		return -1;
	}

	/* short frame 1 */
	dgain_index = OV4689_DGAIN_0DB - p_dgain_gp->short1_gain;
	if ((dgain_index >= pinfo->min_wdr_dgain_index) && (dgain_index <= pinfo->max_wdr_dgain_index)) {
		/* WB-R */
		ov4689_write_reg(src, OV4689_M_WB_R_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_M_WB_R_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
		/* WB-G */
		ov4689_write_reg(src, OV4689_M_WB_G_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_M_WB_G_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
		/* WB-B */
		ov4689_write_reg(src, OV4689_M_WB_B_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_M_WB_B_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
	} else {
		vin_err("short frame 1 dgain exceeds limitation[%d, %d]: %d!\n",
			pinfo->min_wdr_dgain_index, pinfo->max_wdr_dgain_index, dgain_index);
		return -1;
	}

	/* short frame 2 */
	dgain_index = OV4689_DGAIN_0DB - p_dgain_gp->short2_gain;
	if ((dgain_index >= pinfo->min_wdr_dgain_index) && (dgain_index <= pinfo->max_wdr_dgain_index)) {
		/* WB-R */
		ov4689_write_reg(src, OV4689_S_WB_R_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_S_WB_R_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
		/* WB-G */
		ov4689_write_reg(src, OV4689_S_WB_G_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_S_WB_G_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
		/* WB-B */
		ov4689_write_reg(src, OV4689_S_WB_B_GAIN_MSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_MSB]);
		ov4689_write_reg(src, OV4689_S_WB_B_GAIN_LSB, OV4689_WDR_DGAIN_TABLE[dgain_index][OV4689_DGAIN_LSB]);
	} else {
		vin_err("short frame 2 dgain exceeds limitation[%d, %d]: %d!\n",
			pinfo->min_wdr_dgain_index, pinfo->max_wdr_dgain_index, dgain_index);
		return -1;
	}

	vin_dbg("long dgain index:%d, short1 dgain index:%d, short2 dgain index:%d\n",
		p_dgain_gp->long_gain, p_dgain_gp->short1_gain, p_dgain_gp->short2_gain);

	return 0;
}

static int ov4689_set_wdr_again_index_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_gain_gp_info *p_again_gp)
{
	u32 again_index;
	struct ov4689_info *pinfo;

	pinfo = (struct ov4689_info *) src->pinfo;

	if (p_again_gp->long_gain > OV4689_AGAIN_0DB) {
		vin_warn("long gain index %d exceeds maximum %d\n", p_again_gp->long_gain, OV4689_AGAIN_0DB);
		p_again_gp->long_gain = OV4689_AGAIN_0DB;
	}
	if (p_again_gp->short1_gain > OV4689_AGAIN_0DB) {
		vin_warn("short1 gain index %d exceeds maximum %d\n", p_again_gp->short1_gain, OV4689_AGAIN_0DB);
		p_again_gp->short1_gain = OV4689_AGAIN_0DB;
	}
	if (p_again_gp->short2_gain > OV4689_AGAIN_0DB) {
		vin_warn("short2 gain index %d exceeds maximum %d\n", p_again_gp->short2_gain, OV4689_AGAIN_0DB);
		p_again_gp->short2_gain = OV4689_AGAIN_0DB;
	}

	p_again_gp->long_gain = OV4689_AGAIN_0DB - p_again_gp->long_gain;
	p_again_gp->short1_gain = OV4689_AGAIN_0DB - p_again_gp->short1_gain;
	p_again_gp->short2_gain = OV4689_AGAIN_0DB - p_again_gp->short2_gain;

	/* long frame */
	again_index = p_again_gp->long_gain;
	if ((again_index >= pinfo->min_wdr_again_index) && (again_index <= pinfo->max_wdr_again_index)) {
		ov4689_write_reg(src, OV4689_L_GAIN_MSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_MSB]);
		ov4689_write_reg(src, OV4689_L_GAIN_LSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_LSB]);
	} else {
		vin_err("long frame again exceeds limitation[%d, %d]: %d!\n",
			pinfo->min_wdr_again_index, pinfo->max_wdr_again_index, again_index);
		return -1;
	}

	/* short frame 1 */
	again_index = p_again_gp->short1_gain;
	if ((again_index >= pinfo->min_wdr_again_index) && (again_index <= pinfo->max_wdr_again_index)) {
		ov4689_write_reg(src, OV4689_M_GAIN_MSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_MSB]);
		ov4689_write_reg(src, OV4689_M_GAIN_LSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_LSB]);
	} else {
		vin_err("short frame 1 again exceeds limitation[%d, %d]: %d!\n",
			pinfo->min_wdr_again_index, pinfo->max_wdr_again_index, again_index);
		return -1;
	}

	/* short frame 2 */
	again_index = p_again_gp->short2_gain;
	if ((again_index >= pinfo->min_wdr_again_index) && (again_index <= pinfo->max_wdr_again_index)) {
		ov4689_write_reg(src, OV4689_S_GAIN_MSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_MSB]);
		ov4689_write_reg(src, OV4689_S_GAIN_LSB, OV4689_WDR_AGAIN_TABLE[again_index][OV4689_AGAIN_LSB]);
	} else {
		vin_err("short frame 2 again exceeds limitation[%d, %d]: %d!\n",
			pinfo->min_wdr_again_index, pinfo->max_wdr_again_index, again_index);
		return -1;
	}

	vin_dbg("long again index:%d, short1 again index:%d, short2 again index:%d\n",
		p_again_gp->long_gain, p_again_gp->short1_gain, p_again_gp->short2_gain);

	return 0;
}

static int ov4689_set_wdr_again_index(struct __amba_vin_source *src, s32 agc_index)
{
	struct amba_vin_wdr_gain_gp_info again_gp;
	again_gp.long_gain = agc_index;
	again_gp.short1_gain = agc_index;
	again_gp.short2_gain = agc_index;
	ov4689_set_wdr_again_index_group(src, &again_gp);

	return 0;
}

static int ov4689_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int		errCode = 0;
	u32		vflip = 0,hflip = 0;
	struct	ov4689_info			*pinfo;
	u32		target_bayer_pattern;
	u8		tmp_reg;
	pinfo = (struct ov4689_info *) src->pinfo;
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
		vflip = OV4689_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		break;

	case AMBA_VIN_SRC_MIRROR_NONE:
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
		vflip = OV4689_V_FLIP;
		hflip = OV4689_H_MIRROR;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		hflip = OV4689_H_MIRROR;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		break;

	default:
		vin_err("do not support cmd mirror mode\n");
		return -EINVAL;
	}
	errCode |= ov4689_read_reg(src,OV4689_V_FORMAT,&tmp_reg);
	tmp_reg &= (~OV4689_V_FLIP);
	tmp_reg |= vflip;
	ov4689_write_reg(src, OV4689_V_FORMAT, tmp_reg);

	errCode |= ov4689_read_reg(src,OV4689_H_FORMAT,&tmp_reg);
	tmp_reg &= (~OV4689_H_MIRROR);
	tmp_reg |= hflip;
	ov4689_write_reg(src, OV4689_H_FORMAT, tmp_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}

static int ov4689_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int					errCode = 0;
	u32					frame_time = 0;
	u64					frame_time_pclk;
	u32					vertical_lines = 0;
	u32					line_length = 0;
	u32					index;
	u32					format_index;
	u8					data_val;
	u32					max_fps;
	const struct ov4689_pll_reg_table 	*pll_table;
	struct ov4689_info			*pinfo;
	struct amba_vin_irq_fix			vin_irq_fix;

	pinfo = (struct ov4689_info *)src->pinfo;
	index = pinfo->current_video_index;
	format_index = ov4689_video_info_table[index].format_index;

	if (index >= OV4689_VIDEO_INFO_TABLE_SIZE) {
		vin_err("ov4689_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto ov4689_set_fps_exit;
	}

	format_index = ov4689_video_info_table[index].format_index;
	pll_table = &ov4689_pll_tbl[pinfo->pll_index];

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = ov4689_video_format_tbl.table[format_index].auto_fps;

	max_fps = (lane ==4 || format_index != 0)?ov4689_video_format_tbl.table[format_index].max_fps:
		ov4689_video_format_tbl.table[format_index].max_fps << 1;

	if(fps < max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, max_fps));
		errCode = -EPERM;
		goto ov4689_set_fps_exit;
	}

	frame_time = fps;

	errCode |= ov4689_read_reg(src, OV4689_HTS_MSB, &data_val);
	line_length = (u32)(data_val) <<  8;
	errCode |= ov4689_read_reg(src, OV4689_HTS_LSB, &data_val);
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

	vertical_lines = frame_time_pclk;

	vin_dbg("vertical_lines is  0x%05x\n", vertical_lines);

	if(format_index == 4) {// 2 lane, 720p binning mode, VTS (R380e:R380f) has a limitation to be 4n+1
		if ((vertical_lines - 1) & 0x3) {
			vin_warn("binning mode, VTS(%d) should be 4n+1, fixed to %d\n",
				vertical_lines, ((vertical_lines - 1) & 0xFFFFFFFC) + 1);
			vertical_lines = ((vertical_lines - 1) & 0xFFFFFFFC) + 1;
		}
	}

	errCode |= ov4689_write_reg(src, OV4689_VTS_MSB, (u8)((vertical_lines & 0xFF00) >> 8));
	errCode |= ov4689_write_reg(src, OV4689_VTS_LSB, (u8)(vertical_lines & 0xFF));

	ov4689_set_shutter_time(src, pinfo->current_shutter_time);// keep the same shutter time

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	errCode |= ov4689_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode)
		goto ov4689_set_fps_exit;
	errCode |= amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

ov4689_set_fps_exit:
	return errCode;
}

static void ov4689_start_streaming(struct __amba_vin_source *src)
{
	ov4689_write_reg(src, OV4689_STANDBY, 0x01); //streaming
}

static int ov4689_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int				errCode = 0;
	struct ov4689_info		*pinfo;
	u32				format_index;

	pinfo = (struct ov4689_info *)src->pinfo;

	if (index >= OV4689_VIDEO_INFO_TABLE_SIZE) {
		vin_err("ov4689_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto ov4689_set_mode_exit;
	}

	errCode |= ov4689_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = ov4689_video_info_table[index].format_index;

	pinfo->cap_start_x 	= ov4689_video_info_table[index].def_start_x;
	pinfo->cap_start_y 	= ov4689_video_info_table[index].def_start_y;
	pinfo->cap_cap_w 	= ov4689_video_info_table[index].def_width;
	pinfo->cap_cap_h 	= ov4689_video_info_table[index].def_height;
	pinfo->act_start_x = ov4689_video_info_table[index].act_start_x;
	pinfo->act_start_y = ov4689_video_info_table[index].act_start_y;
	pinfo->act_width = ov4689_video_info_table[index].act_width;
	pinfo->act_height = ov4689_video_info_table[index].act_height;
	pinfo->bayer_pattern 	= ov4689_video_info_table[index].bayer_pattern;
	pinfo->pll_index 	= ov4689_video_format_tbl.table[format_index].pll_index;

	ov4689_print_info(src);
	//set clk_si
	errCode |= ov4689_init_vin_clock(src, &ov4689_pll_tbl[pinfo->pll_index]);

	//reset IDSP
	ov4689_vin_reset_idsp(src);
	//Enable MIPI
	if(lane == 4)
		ov4689_vin_mipi_phy_enable(src, MIPI_4LANE); //  4lanes
	else
		ov4689_vin_mipi_phy_enable(src, MIPI_2LANE); //  2lanes

	ov4689_fill_pll_regs(src);
	ov4689_fill_share_regs(src);
	ov4689_fill_video_format_regs(src);

	ov4689_write_reg(src, 0x4800, 0x24); //mipi clock gate run
	ov4689_start_streaming(src);
	ov4689_vin_mipi_reset(src);
	ov4689_write_reg(src, 0x4800, 0x04); //mipi clock free run

	ov4689_set_fps(src, AMBA_VIDEO_FPS_AUTO);

	errCode |= ov4689_set_vin_mode(src);
	errCode |= ov4689_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

ov4689_set_mode_exit:
	return errCode;
}

static int ov4689_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int				errCode = -EINVAL;
	int				i;
	struct ov4689_info		*pinfo;
	u32 video_tbl_size;
	struct ov4689_video_mode *video_mode_tbl;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	ov4689_reset(src);

	pinfo = (struct ov4689_info *)src->pinfo;

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	if (mode == AMBA_VIDEO_MODE_AUTO) {
		mode = OV4689_VIDEO_MODE_TABLE_AUTO;
	}

	switch(pinfo->op_mode) {
		case OV4689_LINEAR_MODE:
			if (lane == 2) {
				video_tbl_size = OV4689_VIDEO_MODE_2LANE_TABLE_SIZE;
				video_mode_tbl = ov4689_video_mode_2lane_table;
			} else {
				video_tbl_size = OV4689_VIDEO_MODE_4LANE_TABLE_SIZE;
				video_mode_tbl = ov4689_video_mode_4lane_table;
			}
			break;
		case OV4689_2X_WDR_MODE:
			video_tbl_size = OV4689_VIDEO_MODE_2X_TABLE_SIZE;
			video_mode_tbl = ov4689_video_mode_2x_table;
			break;
		case OV4689_3X_WDR_MODE:
			video_tbl_size = OV4689_VIDEO_MODE_3X_TABLE_SIZE;
			video_mode_tbl = ov4689_video_mode_3x_table;
			break;
		default:
			vin_err("Unsupported mode:%d\n", pinfo->op_mode);
			video_tbl_size = 0;
			video_mode_tbl = NULL;
			return -EPERM;
	}

	for (i = 0; i < video_tbl_size; i++) {
		if (video_mode_tbl[i].mode == mode) {
			errCode = ov4689_set_video_index(src, video_mode_tbl[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= video_tbl_size) {
		vin_err("ov4689_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = video_mode_tbl[i].mode;
		pinfo->mode_type = video_mode_tbl[i].preview_mode_type;
	}

	return errCode;
}

static int ov4689_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	struct ov4689_info		*pinfo;

	pinfo = (struct ov4689_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = ov4689_init_vin_clock(src, &ov4689_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto ov4689_init_hw_exit;

	msleep(10);
	ov4689_reset(src);

ov4689_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int ov4689_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{
	return 0;
}

static int ov4689_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
	/* >> TODO, for RGB mode only << */
	return 0;
}

static int ov4689_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;

	/* >> TODO, for YUV output mode only << */
	return errCode;
}

static int ov4689_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;

	/* >> TODO, for RGB raw output mode only << */
	/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}

static int ov4689_set_wdr_shutter_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_shutter_gp_info *p_shutter_gp)
{
	u64 exposure_time_q9;
	u8 data_h, data_l;
	u32 line_length, frame_length, active_lines;
	u32 index, format_index;
	int shutter_long, shutter_short1, shutter_short2 = 0, max_middle = 0, max_short = 0;

	struct ov4689_info *pinfo;
	const struct ov4689_pll_reg_table *pll_reg_table;

	pinfo		= (struct ov4689_info *)src->pinfo;
	index = pinfo->current_video_index;
	format_index = ov4689_video_info_table[index].format_index;
	active_lines = ov4689_video_format_tbl.table[format_index].height;
	pll_reg_table	= &ov4689_pll_tbl[pinfo->pll_index];

	ov4689_read_reg(src, OV4689_HTS_MSB, &data_h);
	ov4689_read_reg(src, OV4689_HTS_LSB, &data_l);
	line_length = ((data_h)<<8) + data_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	ov4689_read_reg(src, OV4689_VTS_MSB, &data_h);
	ov4689_read_reg(src, OV4689_VTS_LSB, &data_l);
	frame_length = (data_h<<8) + data_l;

	ov4689_read_reg(src, OV4689_M_MAX_EXPO_MSB, &data_h);
	ov4689_read_reg(src, OV4689_M_MAX_EXPO_LSB, &data_l);
	max_middle = ((data_h)<<8) + data_l;
	if(unlikely(!max_middle)) {
		vin_warn("max_middle is 0!\n");
	}

	if(pinfo->op_mode == OV4689_3X_WDR_MODE) {
		ov4689_read_reg(src, OV4689_S_MAX_EXPO_MSB, &data_h);
		ov4689_read_reg(src, OV4689_S_MAX_EXPO_LSB, &data_l);
		max_short = ((data_h)<<8) + data_l;
		if(unlikely(!max_short)) {
			vin_warn("max_short is 0!\n");
		}
	}

	/* long shutter */
	exposure_time_q9 = p_shutter_gp->long_shutter;
	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;
	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);
	shutter_long = (u32)exposure_time_q9;

	/* short shutter 1 */
	exposure_time_q9 = p_shutter_gp->short1_shutter;
	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;
	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);
	shutter_short1 = (u32)exposure_time_q9;

	if(pinfo->op_mode == OV4689_3X_WDR_MODE) {
		/* short shutter 2 */
		exposure_time_q9 = p_shutter_gp->short2_shutter;
		exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;
		DO_DIV_ROUND(exposure_time_q9, line_length);
		DO_DIV_ROUND(exposure_time_q9, 512000000);
		shutter_short2 = (u32)exposure_time_q9;
	}

	/* shutter limitation check */
	if(shutter_short1 > max_middle - 1) {
		vin_warn("middle shutter %d exceeds limitation %d\n", shutter_short1, max_middle - 1);
	}

	if(pinfo->op_mode == OV4689_3X_WDR_MODE) {
		if(shutter_short2 > max_short - 1) {
			vin_warn("short shutter %d exceeds limitation %d\n", shutter_short2 , max_short - 1);
		}
		if(shutter_long + shutter_short1 + shutter_short2 >= frame_length - 4){
			vin_err("shutter exceeds limitation! long:%d, short1:%d, short2:%d, V:%d\n",
				shutter_long, shutter_short1, shutter_short2, frame_length);
			return -1;
		}
	} else {
		if(shutter_long + shutter_short1 >= frame_length - 4){
			vin_err("shutter exceeds limitation! long:%d, short1:%d, V:%d\n",
				shutter_long, shutter_short1, frame_length);
			return -1;
		}
	}

	/* long shutter */
	shutter_long  = shutter_long << 4;
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x01); /* group latch */
	ov4689_write_reg(src, OV4689_L_EXPO_HSB, (u8)((shutter_long >> 16) & 0xF));
	ov4689_write_reg(src, OV4689_L_EXPO_MSB, (u8)(shutter_long >> 8));
	ov4689_write_reg(src, OV4689_L_EXPO_LSB, (u8)(shutter_long & 0xFF));
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x11); /* group latch */
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0xE1); /* group latch */


	/* short shutter 1 */
	shutter_short1  = shutter_short1 << 4;
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x01); /* group latch */
	ov4689_write_reg(src, OV4689_M_EXPO_HSB, (u8)((shutter_short1 >> 16) & 0xF));
	ov4689_write_reg(src, OV4689_M_EXPO_MSB, (u8)(shutter_short1 >> 8));
	ov4689_write_reg(src, OV4689_M_EXPO_LSB, (u8)(shutter_short1 & 0xFF));
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x11); /* group latch */
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0xE1); /* group latch */

	if(pinfo->op_mode == OV4689_3X_WDR_MODE) {
		/* short shutter 2 */
		shutter_short2  = shutter_short2 << 4;
		ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x01); /* group latch */
		ov4689_write_reg(src, OV4689_S_EXPO_HSB, (u8)((shutter_short2 >> 16) & 0xF));
		ov4689_write_reg(src, OV4689_S_EXPO_MSB, (u8)(shutter_short2 >> 8));
		ov4689_write_reg(src, OV4689_S_EXPO_LSB, (u8)(shutter_short2 & 0xFF));
		ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x11); /* group latch */
		ov4689_write_reg(src, OV4689_GRP_ACCESS, 0xE1); /* group latch */
	}

	vin_dbg("shutter long:%d, short1:%d, short2:%d\n", p_shutter_gp->long_shutter,
		p_shutter_gp->short1_shutter, p_shutter_gp->short2_shutter);

	return 0;
}

static int ov4689_set_wdr_shutter_row_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_shutter_gp_info *p_shutter_gp)
{
	u8 data_h, data_l;
	u32 frame_length;
	u32 index, format_index;
	int shutter_long, shutter_short1, shutter_short2, max_middle = 0, max_short = 0;

	struct ov4689_info *pinfo;

	pinfo = (struct ov4689_info *)src->pinfo;
	index = pinfo->current_video_index;
	format_index = ov4689_video_info_table[index].format_index;

	ov4689_read_reg(src, OV4689_VTS_MSB, &data_h);
	ov4689_read_reg(src, OV4689_VTS_LSB, &data_l);
	frame_length = (data_h<<8) + data_l;

	ov4689_read_reg(src, OV4689_M_MAX_EXPO_MSB, &data_h);
	ov4689_read_reg(src, OV4689_M_MAX_EXPO_LSB, &data_l);
	max_middle = ((data_h)<<8) + data_l;
	if(unlikely(!max_middle)) {
		vin_warn("max_middle is 0!\n");
	}

	if(pinfo->op_mode == OV4689_3X_WDR_MODE) {
		ov4689_read_reg(src, OV4689_S_MAX_EXPO_MSB, &data_h);
		ov4689_read_reg(src, OV4689_S_MAX_EXPO_LSB, &data_l);
		max_short = ((data_h)<<8) + data_l;
		if(unlikely(!max_short)) {
			vin_warn("max_short is 0!\n");
		}
	}

	/* long shutter */
	shutter_long = p_shutter_gp->long_shutter;

	/* short shutter 1 */
	shutter_short1 = p_shutter_gp->short1_shutter;

	/* short shutter 2 */
	shutter_short2 = p_shutter_gp->short2_shutter;

	/* shutter limitation check */
	if(shutter_short1 > max_middle - 1) {
		vin_warn("middle shutter %d exceeds limitation %d\n", shutter_short1, max_middle - 1);
	}

	if(pinfo->op_mode == OV4689_3X_WDR_MODE) {
		if(shutter_short2 > max_short - 1) {
			vin_warn("short shutter %d exceeds limitation %d\n", shutter_short2 , max_short - 1);
		}
		if(shutter_long + shutter_short1 + shutter_short2 >= frame_length - 4){
			vin_err("shutter exceeds limitation! long:%d, short1:%d, short2:%d, V:%d\n",
				shutter_long, shutter_short1, shutter_short2, frame_length);
			return -1;
		}
	} else {
		if(shutter_long + shutter_short1 >= frame_length - 4){
			vin_err("shutter exceeds limitation! long:%d, short1:%d, V:%d\n",
				shutter_long, shutter_short1, frame_length);
			return -1;
		}
	}

	/* long shutter */
	shutter_long  = shutter_long << 4;
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x01); /* group latch */
	ov4689_write_reg(src, OV4689_L_EXPO_HSB, (u8)((shutter_long >> 16) & 0xF));
	ov4689_write_reg(src, OV4689_L_EXPO_MSB, (u8)(shutter_long >> 8));
	ov4689_write_reg(src, OV4689_L_EXPO_LSB, (u8)(shutter_long & 0xFF));
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x11); /* group latch */
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0xE1); /* group latch */


	/* short shutter 1 */
	shutter_short1  = shutter_short1 << 4;
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x01); /* group latch */
	ov4689_write_reg(src, OV4689_M_EXPO_HSB, (u8)((shutter_short1 >> 16) & 0xF));
	ov4689_write_reg(src, OV4689_M_EXPO_MSB, (u8)(shutter_short1 >> 8));
	ov4689_write_reg(src, OV4689_M_EXPO_LSB, (u8)(shutter_short1 & 0xFF));
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x11); /* group latch */
	ov4689_write_reg(src, OV4689_GRP_ACCESS, 0xE1); /* group latch */

	if(pinfo->op_mode == OV4689_3X_WDR_MODE) {
		/* short shutter 2 */
		shutter_short2  = shutter_short2 << 4;
		ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x01); /* group latch */
		ov4689_write_reg(src, OV4689_S_EXPO_HSB, (u8)((shutter_short2 >> 16) & 0xF));
		ov4689_write_reg(src, OV4689_S_EXPO_MSB, (u8)(shutter_short2 >> 8));
		ov4689_write_reg(src, OV4689_S_EXPO_LSB, (u8)(shutter_short2 & 0xFF));
		ov4689_write_reg(src, OV4689_GRP_ACCESS, 0x11); /* group latch */
		ov4689_write_reg(src, OV4689_GRP_ACCESS, 0xE1); /* group latch */
	}

	vin_dbg("shutter long:%d, short1:%d, short2:%d\n", p_shutter_gp->long_shutter,
		p_shutter_gp->short1_shutter, p_shutter_gp->short2_shutter);

	return 0;
}

static int ov4689_set_operation_mode(struct __amba_vin_source *src, amba_vin_sensor_op_mode mode)
{
	int 			errCode = 0;
	struct ov4689_info	*pinfo = (struct ov4689_info *) src->pinfo;

	if((mode != OV4689_LINEAR_MODE) &&
		(mode != OV4689_2X_WDR_MODE) &&
			(mode != OV4689_3X_WDR_MODE) &&
				(mode != OV4689_4X_WDR_MODE)){
		vin_err("wrong opeartion mode, %d!\n", mode);
		errCode = -EPERM;
	} else {
		pinfo->op_mode = mode;
	}

	return errCode;
}

static int ov4689_wdr_shutter2row(struct __amba_vin_source *src,
	struct amba_vin_wdr_shutter_gp_info *p_shutter2row)
{
	u64 exposure_time_q9;
	u8 data_h, data_l;
	u32 line_length, frame_length;

	struct ov4689_info *pinfo;
	const struct ov4689_pll_reg_table *pll_reg_table;

	pinfo		= (struct ov4689_info *)src->pinfo;
	pll_reg_table	= &ov4689_pll_tbl[pinfo->pll_index];

	ov4689_read_reg(src, OV4689_HTS_MSB, &data_h);
	ov4689_read_reg(src, OV4689_HTS_LSB, &data_l);
	line_length = ((data_h)<<8) + data_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	ov4689_read_reg(src, OV4689_VTS_MSB, &data_h);
	ov4689_read_reg(src, OV4689_VTS_LSB, &data_l);
	frame_length = (data_h<<8) + data_l;

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

	return 0;
}

static int ov4689_wdr_get_win_offset(struct __amba_vin_source *src,
	struct amba_vin_wdr_win_offset *p_win_offset)
{
	int errCode = 0;
	u32 format_index;
	struct ov4689_info	*pinfo = (struct ov4689_info *) src->pinfo;

	if (pinfo->op_mode == OV4689_LINEAR_MODE) {
		vin_warn("%s should be called by hdr mode\n", __func__);
		return -EPERM;
	}

	format_index = ov4689_video_info_table[pinfo->current_video_index].format_index;
	memcpy(p_win_offset,
		&ov4689_video_format_tbl.table[format_index].hdr_win_offset,
		sizeof(struct amba_vin_wdr_win_offset));

	return errCode;
}

static int ov4689_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct ov4689_info		*pinfo;

	pinfo = (struct ov4689_info *) src->pinfo;
	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		ov4689_reset(src);
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
		errCode = ov4689_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO: {
		struct amba_vin_source_info *pub_src;

		pub_src = (struct amba_vin_source_info *) args;
		pub_src->id = src->id;
		pub_src->sensor_id = SENSOR_OV4689;
		pub_src->adapter_id = adapter_id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		pub_src->interface_type = SENSOR_IF_MIPI;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = ov4689_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = ov4689_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = ov4689_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = ov4689_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = ov4689_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = ov4689_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = ov4689_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = ov4689_set_still_mode(src, (struct amba_vin_src_still_info *) args);
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
		errCode = ov4689_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = ov4689_set_agc_db(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(s32*)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = ov4689_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = ov4689_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = ov4689_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = ov4689_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		ov4689_dump_reg(src);
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

		errCode = ov4689_read_reg(src, subaddr, &data);

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

		errCode = ov4689_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = ov4689_set_slowshutter_mode(src, *(int *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_SHUTTER_GROUP:
		errCode = ov4689_set_wdr_shutter_group(src,
			(struct amba_vin_wdr_shutter_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX_GROUP:
		errCode = ov4689_set_wdr_again_index_group(src,
			(struct amba_vin_wdr_gain_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_DGAIN_INDEX_GROUP:
		errCode = ov4689_set_wdr_dgain_index_group(src,
			(struct amba_vin_wdr_gain_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_SHUTTER_ROW_GROUP:
		errCode = ov4689_set_wdr_shutter_row_group(src,
			(struct amba_vin_wdr_shutter_gp_info *) args);
		break;

	case AMBA_VIN_SRC_WDR_SHUTTER2ROW:
		errCode = ov4689_wdr_shutter2row(src,
			(struct amba_vin_wdr_shutter_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW:
		errCode = ov4689_set_shutter_time_row(src, *(u32 *) args);
		break;
	case AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH:
		errCode=ov4689_shutter_time2width(src, (u32 *) args);
		break;
	case AMBA_VIN_SRC_SET_GAIN_INDEX:
		errCode = ov4689_set_agc_index(src, *(u32 *) args);
		break;
	case AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX:
		errCode = ov4689_set_wdr_again_index(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_SET_OPERATION_MODE:
		errCode = ov4689_set_operation_mode(src, *(amba_vin_sensor_op_mode *)args);
		break;

	case AMBA_VIN_SRC_GET_WDR_WIN_OFFSET:
		errCode = ov4689_wdr_get_win_offset(src,
			(struct amba_vin_wdr_win_offset *) args);
		break;

	case AMBA_VIN_SRC_GET_AAA_INFO: {
		struct amba_vin_aaa_info *aaa_info;

		aaa_info = (struct amba_vin_aaa_info *) args;
		aaa_info->pixel_size = pinfo->pixel_size;
		}
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
