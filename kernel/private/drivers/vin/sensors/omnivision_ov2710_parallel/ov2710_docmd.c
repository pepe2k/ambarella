/*
 * Filename : ov2710_docmd.c
 *
 * History:
 *    2009/06/19 - [Qiao Wang] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

static void ov2710_dump_reg(struct __amba_vin_source *src)
{
}

static void ov2710_fill_video_format_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 format_index;
	struct ov2710_info *pinfo;

	pinfo = (struct ov2710_info *) src->pinfo;

	vin_dbg("ov2710_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = ov2710_video_info_table[index].format_index;

	for (i = 0; i < OV2710_VIDEO_FORMAT_REG_NUM; i++) {
		if (ov2710_video_format_tbl.reg[i] == 0)
			break;

		ov2710_write_reg(src,
				  ov2710_video_format_tbl.reg[i],
				  ov2710_video_format_tbl.table[format_index].data[i]);
	}

	if (ov2710_video_format_tbl.table[format_index].ext_reg_fill)
		ov2710_video_format_tbl.table[format_index].ext_reg_fill(src);

}

static void ov2710_fill_share_regs(struct __amba_vin_source *src)
{
	int i, reg_tbl_size = 0;
	u32 index;
	u32 format_index;
	struct ov2710_info *pinfo;
	const struct ov2710_reg_table *reg_tbl = NULL;

	pinfo = (struct ov2710_info *) src->pinfo;
	index = pinfo->current_video_index;
	format_index = ov2710_video_info_table[index].format_index;

	if(format_index == 0) {//720P
		reg_tbl = ov2710_720p_share_regs;
		reg_tbl_size = OV2710_720P_SHARE_REG_SZIE;
	} else if(format_index == 1){//1080P
		reg_tbl = ov2710_1080p_share_regs;
		reg_tbl_size = OV2710_1080P_SHARE_REG_SZIE;
	}

	for (i = 0; i < reg_tbl_size; i++) {
		ov2710_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
#ifdef OV2710_SW_SAVE_HVTS
		/* use variable to save HTS/VTS */
		if(reg_tbl[i].reg == OV2710_TIMING_CONTROL_HTS_HIGHBYTE) {
			pinfo->line_length |= reg_tbl[i].data << 8;
		} else if (reg_tbl[i].reg == OV2710_TIMING_CONTROL_HTS_LOWBYTE) {
			pinfo->line_length |= reg_tbl[i].data;
		} else if (reg_tbl[i].reg == OV2710_TIMING_CONTROL_VTS_HIGHBYTE) {
			pinfo->frame_length_lines |= reg_tbl[i].data << 8;
		} else if (reg_tbl[i].reg == OV2710_TIMING_CONTROL_VTS_LOWBYTE) {
			pinfo->frame_length_lines |= reg_tbl[i].data;
		}
#endif
	}
}

static void ov2710_fill_pll_regs(struct __amba_vin_source *src)
{
	int i;
	struct ov2710_info 			*pinfo;
	const struct ov2710_pll_reg_table 	*pll_reg_table;
	const struct ov2710_reg_table 		*pll_tbl;

	pinfo = (struct ov2710_info *) src->pinfo;

	vin_dbg("ov2710_fill_pll_regs\n");
	pll_reg_table = &ov2710_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < OV2710_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			ov2710_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
	msleep(2);
}

static void ov2710_set_streaming(struct __amba_vin_source *src)
{
	u8 data;
	ov2710_read_reg(src, OV2710_SYSTEM_CONTROL00, &data);
	ov2710_write_reg(src, OV2710_SYSTEM_CONTROL00, (data & 0xBF));
}

static void ov2710_sw_reset(struct __amba_vin_source *src) {
	u8 data;
	ov2710_read_reg(src, OV2710_SYSTEM_CONTROL00, &data);
	ov2710_write_reg(src, OV2710_SYSTEM_CONTROL00, (data | 0x80));
}

static void ov2710_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	ov2710_sw_reset(src);
}

static int ov2710_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct ov2710_info  *pinfo;

	pinfo = (struct ov2710_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int ov2710_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errCode = 0;
	u32 index;
	u32 format_index;
	struct ov2710_info *pinfo;

	pinfo = (struct ov2710_info *) src->pinfo;

	index = pinfo->current_video_index;

	if (index >= OV2710_VIDEO_INFO_TABLE_SZIE) {
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
		format_index = ov2710_video_info_table[index].format_index;

		p_video_info->width = ov2710_video_info_table[index].def_width;
		p_video_info->height = ov2710_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = ov2710_video_format_tbl.table[format_index].format;
		p_video_info->type = ov2710_video_format_tbl.table[format_index].type;
		p_video_info->bits = ov2710_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = ov2710_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

/*
static int ov2710_get_vblank_time( struct __amba_vin_source *src,
	u32 *ptime)
{
	int				errCode		= 0;
	int				mode_index;
	u32				format_index;
	u64				v_btime;
	u32				active_lines, frame_length_lines;
	u32				line_length;
	u8				data_l, data_h;

	struct ov2710_info 			*pinfo;
	const struct ov2710_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct ov2710_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= ov2710_video_info_table[mode_index].format_index;
	pll_reg_table	= &ov2710_pll_tbl[pinfo->pll_index];

	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_HTS_HIGHBYTE, &data_h);
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_HTS_LOWBYTE, &data_l);
	line_length = (data_h<<8) + data_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_VTS_HIGHBYTE, &data_h);
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_VTS_LOWBYTE, &data_l);
	frame_length_lines = (data_h<<8) + data_l;

	active_lines = ov2710_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pll_reg_table->pixclk); //ns
	*ptime = v_btime;

	vin_dbg("ov2710_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return errCode;
}
*/

static int ov2710_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct ov2710_info *pinfo;

	pinfo = (struct ov2710_info *) src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}
static int ov2710_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));

	return 0;
}

static int ov2710_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < OV2710_VIDEO_MODE_TABLE_SZIE; i++) {
		if (ov2710_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = OV2710_VIDEO_MODE_TABLE_AUTO;
			p_mode_info->is_supported = 1;

			index = ov2710_video_mode_table[i].still_index;
			format_index = ov2710_video_info_table[index].format_index;

			p_mode_info->video_info.width = ov2710_video_info_table[index].def_width;
			p_mode_info->video_info.height = ov2710_video_info_table[index].def_height;
			p_mode_info->video_info.fps = ov2710_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = ov2710_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = ov2710_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = ov2710_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = ov2710_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;

			break;
		}
	}

	return errCode;
}

static int ov2710_query_sensor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	int errCode = 0;
	u8 id_h, id_l;

	errCode = ov2710_read_reg(src, OV2710_PIDH, &id_h);
	errCode = ov2710_read_reg(src, OV2710_PIDL, &id_l);
	*ss_id = (id_h<<8)|id_l;

	return errCode;

}

static int ov2710_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{
	return 0;
}

static int ov2710_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
	return 0;
}

static int ov2710_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	int 	errCode = 0;
	u64 	exposure_time_q9;
	u32 	line_length, frame_length_lines;
	int shutter_width;

	const struct ov2710_pll_reg_table *pll_table;
	struct ov2710_info		*pinfo;

	vin_dbg("ov2710_set_shutter: 0x%x\n", shutter_time);

	pinfo = (struct ov2710_info *)src->pinfo;
	pll_table = &ov2710_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time;

#ifndef OV2710_SW_SAVE_HVTS
	u8	data_l, data_h;
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_HTS_HIGHBYTE, &data_h);
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_HTS_LOWBYTE, &data_l);
	line_length = (data_h<<8) + data_l;
#else
	line_length = pinfo->line_length;
#endif
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

#ifndef OV2710_SW_SAVE_HVTS
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_VTS_HIGHBYTE, &data_h);
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_VTS_LOWBYTE, &data_l);
	frame_length_lines = (data_h<<8) + data_l;
#else
	frame_length_lines = pinfo->frame_length_lines;
#endif
	exposure_time_q9 = exposure_time_q9 * (u64)pll_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (u32)exposure_time_q9;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 3) */
	if(shutter_width < 1) {
		shutter_width = 1;
	} else if(shutter_width  > (frame_length_lines - 3)) {
		shutter_width = frame_length_lines - 3;
	}

	vin_dbg("V:%d, shutter_width: %d\n", frame_length_lines, shutter_width);
	shutter_width <<= 4; //the register value should be exposure time * 16

	errCode |= ov2710_write_reg(src, OV2710_AEC_PK_EXPO_H, (u8)((shutter_width & 0x0F0000) >> 16) );
	errCode |= ov2710_write_reg(src, OV2710_AEC_PK_EXPO_M, (u8)((shutter_width & 0x00FF00) >> 8 ) );
	errCode |= ov2710_write_reg(src, OV2710_AEC_PK_EXPO_L, (u8)((shutter_width & 0x0000FF) >> 0 ) );

	pinfo->current_shutter_time = shutter_time;
	return errCode;
}

static int ov2710_shutter_time2width(struct __amba_vin_source *src, u32* shutter_time)
{
	int 	errCode = 0;
	u64 	exposure_time_q9;
	u32 	line_length, frame_length_lines;
	int shutter_width;

	const struct ov2710_pll_reg_table *pll_table;
	struct ov2710_info		*pinfo;

//	vin_dbg("ov2710_set_shutter: 0x%x\n", shutter_time);

	pinfo = (struct ov2710_info *)src->pinfo;
	pll_table = &ov2710_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = *shutter_time;

#ifndef OV2710_SW_SAVE_HVTS
	u8	data_l, data_h;
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_HTS_HIGHBYTE, &data_h);
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_HTS_LOWBYTE, &data_l);
	line_length = (data_h<<8) + data_l;
#else
	line_length = pinfo->line_length;
#endif
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

#ifndef OV2710_SW_SAVE_HVTS
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_VTS_HIGHBYTE, &data_h);
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_VTS_LOWBYTE, &data_l);
	frame_length_lines = (data_h<<8) + data_l;
#else
	frame_length_lines = pinfo->frame_length_lines;
#endif

	exposure_time_q9 = exposure_time_q9 * (u64)pll_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (u32)exposure_time_q9;

	*shutter_time=shutter_width;
	return errCode;
}

static int ov2710_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	struct ov2710_info *pinfo;
	u32 gain_index;
	s32 db_max;
	s32 db_step;

	pinfo = (struct ov2710_info *) src->pinfo;

	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	vin_dbg("ov2710_set_agc: 0x%x\n", agc_db);
	gain_index = (db_max - agc_db) / db_step;

	if (gain_index > OV2710_GAIN_0DB)
		gain_index = OV2710_GAIN_0DB;
	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {

		ov2710_write_reg(src, OV2710_GROUP_ACCESS, 0x00);

		ov2710_write_reg(src, OV2710_AEC_AGC_ADJ_H, OV2710_GAIN_TABLE[gain_index][OV2710_GAIN_COL_REG300A]);
		ov2710_write_reg(src, OV2710_AEC_AGC_ADJ_L, OV2710_GAIN_TABLE[gain_index][OV2710_GAIN_COL_REG300B]);

		ov2710_write_reg(src, OV2710_GROUP_ACCESS, 0x10);
		ov2710_write_reg(src, OV2710_GROUP_ACCESS, 0xA0);

		pinfo->current_gain_db = agc_db;

		return 0;
	} else{
		return -1;
	}
}

static int ov2710_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int errCode = 0;
	uint readmode;
	struct ov2710_info *pinfo;
	u32 target_bayer_pattern;
	u8 tmp_reg;
	u8 ana_reg;
	u8 vstart_reg;

	pinfo = (struct ov2710_info *) src->pinfo;

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
	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = OV2710_MIRROR_ROW + OV2710_MIRROR_COLUMN;
		ana_reg = 0x14;
		vstart_reg = 0x09;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		break;
	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		readmode = OV2710_MIRROR_ROW;
		ana_reg = 0x14;
		vstart_reg = 0x0A;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		break;
	case AMBA_VIN_SRC_MIRROR_VERTICALLY:
		readmode = OV2710_MIRROR_COLUMN;
		ana_reg = 0x04;
		vstart_reg = 0x09;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		break;
	case AMBA_VIN_SRC_MIRROR_NONE:
		readmode = 0;
		ana_reg = 0x04;
		vstart_reg = 0x0A;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		break;
	default:
		vin_err("do not support cmd mirror mode\n");
		return -EINVAL;
		break;
	}
	pinfo->bayer_pattern = mirror_mode->bayer_pattern;
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL18, &tmp_reg);
	tmp_reg &= (~OV2710_MIRROR_MASK);
	tmp_reg |= readmode;
	errCode |= ov2710_write_reg(src, OV2710_TIMING_CONTROL18, tmp_reg);
	errCode |= ov2710_write_reg(src, OV2710_ANA_ARRAY_01, ana_reg);
	errCode |= ov2710_write_reg(src, OV2710_TIMING_CONTROL_VS_LOWBYTE, vstart_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}

static int ov2710_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
		/* >> TODO, for YUV output sensor only << */
	return errCode;
}

static int ov2710_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int 				errCode = 0;
	u32  				frame_time = 0;
	u32				index;
	u32 				format_index;
	u64 				frame_time_pclk;
	int 				vertical_lines = 0;
	u32 				line_length;
	struct amba_vin_irq_fix			vin_irq_fix;


	const struct ov2710_pll_reg_table *pll_table;
 	struct ov2710_info		*pinfo;
	pinfo = (struct ov2710_info *)src->pinfo;
	index = pinfo->current_video_index;

	format_index = ov2710_video_info_table[index].format_index;
	pll_table = &ov2710_pll_tbl[pinfo->pll_index];

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = ov2710_video_format_tbl.table[format_index].auto_fps;
	if(fps < ov2710_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, ov2710_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto ov2710_set_fps_exit;
	}

	frame_time = fps;

#ifndef OV2710_SW_SAVE_HVTS
	u8	data_l, data_h;
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_HTS_HIGHBYTE, &data_h);
	errCode |= ov2710_read_reg(src, OV2710_TIMING_CONTROL_HTS_LOWBYTE, &data_l);
	line_length = (data_h<<8) + data_l;
#else
	line_length = pinfo->line_length;
#endif
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		errCode =  -EIO;
		goto ov2710_set_fps_exit;
	}

	frame_time_pclk = frame_time * (u64)pll_table->pixclk;

	DO_DIV_ROUND(frame_time_pclk, line_length);
	DO_DIV_ROUND(frame_time_pclk, 512000000);
	vin_dbg("frame_time %d, line_length 0x%x, vertical_lines 0x%llx\n", frame_time, line_length, frame_time_pclk);

	vertical_lines = frame_time_pclk;

	errCode |= ov2710_write_reg(src, OV2710_TIMING_CONTROL_VTS_HIGHBYTE, (u8)((vertical_lines & 0x00FF00) >>8));
	errCode |= ov2710_write_reg(src, OV2710_TIMING_CONTROL_VTS_LOWBYTE, (u8)(vertical_lines & 0x0000FF));

	pinfo->frame_rate = fps;

	ov2710_set_shutter_time(src, pinfo->current_shutter_time);

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_irq_fix.delay = 0;
	/*
	errCode |= ov2710_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode)
		goto ov2710_set_fps_exit;
	*/
	errCode |= amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

ov2710_set_fps_exit:
	return errCode;
}

static int ov2710_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct ov2710_info *pinfo;
	u32 format_index;

	pinfo = (struct ov2710_info *) src->pinfo;

	if (index >= OV2710_VIDEO_INFO_TABLE_SZIE) {
		vin_err("ov2710_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto ov2710_set_mode_exit;
	}

	errCode |= ov2710_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = ov2710_video_info_table[index].format_index;

	pinfo->cap_start_x = ov2710_video_info_table[index].def_start_x;
	pinfo->cap_start_y = ov2710_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = ov2710_video_info_table[index].def_width;
	pinfo->cap_cap_h = ov2710_video_info_table[index].def_height;
	pinfo->bayer_pattern = ov2710_video_info_table[index].bayer_pattern;
	pinfo->pll_index = ov2710_video_format_tbl.table[format_index].pll_index;
	ov2710_print_info(src);

	//set clk_si
	errCode |= ov2710_init_vin_clock(src, &ov2710_pll_tbl[pinfo->pll_index]);

	ov2710_fill_share_regs(src);
	ov2710_fill_pll_regs(src);
	ov2710_fill_video_format_regs(src);
	ov2710_set_fps(src, AMBA_VIDEO_FPS_AUTO);

	ov2710_set_shutter_time(src, AMBA_VIDEO_FPS_60);
	ov2710_set_agc_db(src, 0);
	ov2710_set_streaming(src);	//start streaming

	errCode |= ov2710_set_vin_mode(src);

	errCode |= ov2710_post_set_vin_mode(src);

	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

ov2710_set_mode_exit:
	return errCode;
}

static int ov2710_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errCode = -EINVAL;
	int i;
	struct ov2710_info *pinfo;

	pinfo = (struct ov2710_info *) src->pinfo;

	errCode = ov2710_init_vin_clock(src, &ov2710_pll_tbl[0]);
	if (errCode)
		goto ov2710_set_video_mode_exit;
	msleep(10);
	ov2710_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	for (i = 0; i < OV2710_VIDEO_MODE_TABLE_SZIE; i++) {
		if (ov2710_video_mode_table[i].mode == mode) {
			errCode = ov2710_set_video_index(src, ov2710_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= OV2710_VIDEO_MODE_TABLE_SZIE) {
		vin_err("ov2710_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = ov2710_video_mode_table[i].mode;
		pinfo->mode_type = ov2710_video_mode_table[i].preview_mode_type;
	}

ov2710_set_video_mode_exit:
	return errCode;
}

static int ov2710_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	u16				sen_id = 0;

	errCode = ov2710_init_vin_clock(src, &ov2710_pll_tbl[0]);
	if (errCode)
		goto ov2710_init_hw_exit;
	msleep(10);
	ov2710_reset(src);

	errCode = ov2710_query_sensor_id(src, &sen_id);
	if (errCode)
		goto ov2710_init_hw_exit;
	/*
	if (sen_id != expected id) {
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		goto ov2710_init_hw_exit;
	}*/

	vin_info("OV2710 sensor ID is 0x%x\n", sen_id);

ov2710_init_hw_exit:
	return errCode;
}

static int ov2710_docmd(struct __amba_vin_source *src,
	enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct ov2710_info*pinfo;
	pinfo = (struct ov2710_info *) src->pinfo;

	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		ov2710_reset(src);
		break;

	case AMBA_VIN_SRC_SET_POWER:
		ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_power, (u32)args);
		break;

	case AMBA_VIN_SRC_SUSPEND:
		ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_reset, 1);
		ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_power, 0);
		break;

	case AMBA_VIN_SRC_RESUME:
		errCode = ov2710_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO:
		{
			struct amba_vin_source_info *pub_src;

			pub_src = (struct amba_vin_source_info *) args;
			pub_src->id = src->id;
			pub_src->adapter_id = adapter_id;
			pub_src->dev_type = src->dev_type;
			strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
			pub_src->sensor_id = SENSOR_OV2710;
			pub_src->default_mode = OV2710_VIDEO_MODE_TABLE_AUTO;
			pub_src->total_channel_num = src->total_channel_num;
			pub_src->active_channel_id = src->active_channel_id;
			pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = ov2710_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = ov2710_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = ov2710_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = ov2710_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = ov2710_get_capability(src, (struct amba_vin_src_capability *) args);

		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = ov2710_get_video_mode(src,(enum amba_video_mode *)args);
		break;
	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = ov2710_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = ov2710_set_still_mode(src, (struct amba_vin_src_still_info *) args);
		break;

	case AMBA_VIN_SRC_GET_BLC:
		errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_GET_SW_BLC, (void *) args);
		memcpy(&(pinfo->current_sw_blc), (void *) args, sizeof (pinfo->current_sw_blc));
		break;
	case AMBA_VIN_SRC_SET_BLC:
		memcpy(&(pinfo->current_sw_blc), (void *) args, sizeof (pinfo->current_sw_blc));
		errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_SW_BLC, (void *) args);
		break;

	case AMBA_VIN_SRC_GET_FRAME_RATE:
		*(u32 *)args = pinfo->frame_rate;
		break;
	case AMBA_VIN_SRC_SET_FRAME_RATE:
		errCode = ov2710_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;
	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = ov2710_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(u32 *)args = pinfo->current_gain_db;
		break;
	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = ov2710_set_agc_db(src, *(s32 *) args);
		break;
	case AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH:
		errCode=ov2710_shutter_time2width(src, (u32 *) args);
		break;
	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = ov2710_set_low_ligth_agc(src, *(u32 *) args);
		break;
	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = ov2710_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;
	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = ov2710_set_anti_flicker(src, *(u32 *) args);
		break;
	case AMBA_VIN_SRC_TEST_DUMP_REG:
		ov2710_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		{
			u16 sen_id = 0;
			u16 sen_ver= 0;
			u32 *pdata = (u32 *) args;

			errCode = ov2710_query_sensor_id(src, &sen_id);
			if (errCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;

			*pdata = (sen_id << 16) | sen_ver;
		}
	      exit_AMBA_VIN_SRC_TEST_GET_DEV_ID:
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u16  subaddr;
			u8  data = 0;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;

			errCode = ov2710_read_reg(src, subaddr, &data);

			reg_data->data = data;
		}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u16  subaddr;
			u8  data = 0;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;
			data = reg_data->data;

			errCode = ov2710_write_reg(src, subaddr, data);
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
