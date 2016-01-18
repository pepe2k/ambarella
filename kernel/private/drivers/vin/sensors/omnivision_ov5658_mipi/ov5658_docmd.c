/*
 * Filename : ov5658_docmd.c
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
 *
 */


static void ov5658_fill_video_format_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 format_index;
	struct ov5658_info *pinfo;

	pinfo = (struct ov5658_info *) src->pinfo;

	vin_dbg("ov5658_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = ov5658_video_info_table[index].format_index;

	for (i = 0; i < OV5658_VIDEO_FORMAT_REG_NUM; i++) {
		if (ov5658_video_format_tbl.reg[i] == 0)
			break;

		ov5658_write_reg(src,
				ov5658_video_format_tbl.reg[i],
				ov5658_video_format_tbl.table[format_index].data[i]);
	}

	if (ov5658_video_format_tbl.table[format_index].ext_reg_fill)
		ov5658_video_format_tbl.table[format_index].ext_reg_fill(src);

}

static void ov5658_fill_share_regs(struct __amba_vin_source *src)
{
	int i;
	int size;
	const struct ov5658_reg_table *reg_tbl=NULL;
	if(OV5658_LANE == 4){
		reg_tbl = ov5658_share_regs_4lane;
		size = OV5658_SHARE_REG_SIZE_4LANE;
	}else if(OV5658_LANE == 2){
		reg_tbl = ov5658_share_regs_2lane;
		size = OV5658_SHARE_REG_SIZE_2LANE;
	}else{
		reg_tbl = ov5658_share_regs_4lane;
		size = OV5658_SHARE_REG_SIZE_4LANE;
	}
	for (i = 0; i < size; i++) {
		ov5658_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void ov5658_set_standby(struct __amba_vin_source *src, u32 enable)
{
	u32 standby = (enable)? 0x0: 0x1;
	ov5658_write_reg(src, ov5658_mode_select, standby);  // Register power down
}

static void ov5658_sw_reset(struct __amba_vin_source *src)
{
	ov5658_write_reg(src,ov5658_soft_reset,0x1);
	msleep(10);
}

static void ov5658_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	ov5658_sw_reset(src);
}

static void ov5658_fill_pll_regs(struct __amba_vin_source *src)
{

	int i;
	struct ov5658_info *pinfo;
	const struct ov5658_pll_reg_table 	*pll_reg_table;
	const struct ov5658_reg_table 		*pll_tbl;

	pinfo = (struct ov5658_info *) src->pinfo;

	vin_dbg("ov5658_fill_pll_regs\n");
	if(OV5658_LANE == 4){
		pll_reg_table = &ov5658_pll_4lane_tbl[pinfo->pll_index];
	}else if(OV5658_LANE == 2){
		pll_reg_table = &ov5658_pll_2lane_tbl[pinfo->pll_index];
	}else{
		pll_reg_table = &ov5658_pll_4lane_tbl[pinfo->pll_index];
	}
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < OV5658_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			ov5658_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
}

static int ov5658_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct ov5658_info  *pinfo;

	pinfo = (struct ov5658_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int ov5658_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errCode = 0;
	u32 index;
	u32 format_index;
	struct ov5658_info *pinfo;

	pinfo = (struct ov5658_info *) src->pinfo;

	index = pinfo->current_video_index;

	if (index >= OV5658_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = ov5658_video_info_table[index].format_index;

		p_video_info->width = ov5658_video_info_table[index].def_width;
		p_video_info->height = ov5658_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = ov5658_video_format_tbl.table[format_index].format;
		p_video_info->type = ov5658_video_format_tbl.table[format_index].type;
		p_video_info->bits = ov5658_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = ov5658_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

static int ov5658_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct ov5658_info *pinfo;

	pinfo = (struct ov5658_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}
static int ov5658_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));
	return 0;
}

static int ov5658_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < OV5658_VIDEO_MODE_TABLE_SIZE; i++) {
		if (ov5658_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = OV5658_VIDEO_MODE_TABLE_AUTO;
			p_mode_info->is_supported = 1;

			index = ov5658_video_mode_table[i].still_index;
			format_index = ov5658_video_info_table[index].format_index;

			p_mode_info->video_info.width =
				ov5658_video_info_table[index].def_width;
			p_mode_info->video_info.height =
				ov5658_video_info_table[index].def_height;
			p_mode_info->video_info.fps =
				ov5658_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format =
				ov5658_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type =
				ov5658_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits =
				ov5658_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio =
				ov5658_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;
			break;
		}
	}

	return errCode;
}

static int ov5658_query_sensor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	int errCode = 0;
	u8 id_h, id_l;

	errCode = ov5658_read_reg(src, OV5658_CHIP_ID_H, &id_h);
	errCode = ov5658_read_reg(src, OV5658_CHIP_ID_L, &id_l);
	*ss_id = (id_h<<8)|id_l;

	return errCode;

}

static int ov5658_query_sensor_version(struct __amba_vin_source *src, u16 * ver)
{
		/* >> TODO << */
	return 0;
}

static int ov5658_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{

	return 0;

}

static int ov5658_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
		/* >> TODO, for RGB mode only << */
	return 0;

}

static int ov5658_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	int 	errCode = 0;
	u64 	exposure_time_q9;
	u32 	line_length;
	int	shutter_width;
	u32	frame_length;
	u8	data_l, data_h;

	struct ov5658_info			*pinfo;
	const struct ov5658_pll_reg_table 	*pll_reg_table;

	vin_dbg("ov5658_set_shutter: %d\n", shutter_time);

	pinfo = (struct ov5658_info *)src->pinfo;
	if(OV5658_LANE == 4){
		pll_reg_table 	= &ov5658_pll_4lane_tbl[pinfo->pll_index];
	}else if (OV5658_LANE == 2){
		pll_reg_table 	= &ov5658_pll_2lane_tbl[pinfo->pll_index];
	}else{
		pll_reg_table 	= &ov5658_pll_4lane_tbl[pinfo->pll_index];
	}

	exposure_time_q9 =	shutter_time;

	errCode |= ov5658_read_reg(src, OV5658_TIMING_HTS_H, &data_h);
	errCode |= ov5658_read_reg(src, OV5658_TIMING_HTS_L, &data_l);

	line_length = (data_h<<8) + data_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}
	vin_dbg("line_length is [0x%04x]\n", line_length);

	errCode |= ov5658_read_reg(src, OV5658_TIMING_VTS_H, &data_h);
	errCode |= ov5658_read_reg(src, OV5658_TIMING_VTS_L, &data_l);

	frame_length = (data_h<<8) + data_l;

	/* t(frame)/t(per line) = t(exposure, in lines)*/
	vin_dbg("exp %d, pix %d\n", (u32)exposure_time_q9, pll_reg_table->pixclk);
	exposure_time_q9 = exposure_time_q9 * pll_reg_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = exposure_time_q9;

	if(shutter_width < 1) {
		shutter_width = 1;
	} else if(shutter_width  > frame_length - 3) {
		shutter_width = frame_length - 3;
	}

	vin_dbg("V:%d, shutter:%d\n", frame_length, shutter_width);

	shutter_width <<= 4; //the register value should be exposure time * 16
	errCode |= ov5658_write_reg(src, OV5658_LONG_EXPO_H, (u16)((shutter_width & 0x0F0000) >> 16) );
	errCode |= ov5658_write_reg(src, OV5658_LONG_EXPO_M, (u16)((shutter_width & 0x00FF00) >> 8 ) );
	errCode |= ov5658_write_reg(src, OV5658_LONG_EXPO_L, (u16)((shutter_width & 0x0000FF) >> 0 ) );
	pinfo->current_shutter_time = shutter_time;
	return errCode;
}

static int ov5658_shutter_time2width(struct __amba_vin_source *src, u32* shutter_time)
{
	int 	errCode = 0;
	u64 	exposure_time_q9;
	u32 	line_length;
	int	shutter_width;
	u32	frame_length;
	u8	data_l, data_h;

	struct ov5658_info			*pinfo;
	const struct ov5658_pll_reg_table 	*pll_reg_table;

//	vin_dbg("ov5658_set_shutter: %d\n", shutter_time);

	pinfo = (struct ov5658_info *)src->pinfo;
	if(OV5658_LANE == 4){
		pll_reg_table 	= &ov5658_pll_4lane_tbl[pinfo->pll_index];
	}else if (OV5658_LANE == 2){
		pll_reg_table 	= &ov5658_pll_2lane_tbl[pinfo->pll_index];
	}else{
		pll_reg_table 	= &ov5658_pll_4lane_tbl[pinfo->pll_index];
	}

	exposure_time_q9 =	*shutter_time;

	errCode |= ov5658_read_reg(src, OV5658_TIMING_HTS_H, &data_h);
	errCode |= ov5658_read_reg(src, OV5658_TIMING_HTS_L, &data_l);

	line_length = (data_h<<8) + data_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}
	vin_dbg("line_length is [0x%04x]\n", line_length);

	errCode |= ov5658_read_reg(src, OV5658_TIMING_VTS_H, &data_h);
	errCode |= ov5658_read_reg(src, OV5658_TIMING_VTS_L, &data_l);

	frame_length = (data_h<<8) + data_l;

	/* t(frame)/t(per line) = t(exposure, in lines)*/
	vin_dbg("exp %d, pix %d\n", (u32)exposure_time_q9, pll_reg_table->pixclk);
	exposure_time_q9 = exposure_time_q9 * pll_reg_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = exposure_time_q9;

	if(shutter_width < 1) {
		shutter_width = 1;
	} else if(shutter_width  > frame_length - 3) {
		shutter_width = frame_length - 3;
	}

	*shutter_time =shutter_width;
	return errCode;
}

static int ov5658_agc_parse_virtual_index(struct __amba_vin_source *src, u32 virtual_index, u32 *agc_index, u32 *summing_gain)
{
	struct ov5658_info *pinfo;
	u32 downsample;

	pinfo = (struct ov5658_info *) src->pinfo;
	downsample = pinfo->downsample;

	if(downsample == 1) {
		*agc_index = virtual_index;
		*summing_gain = 0;
	} else if(downsample == 2 || downsample == 4) {

		u32 index_0dB  = OV5658_GAIN_0DB;
		u32 index_6dB  = OV5658_GAIN_0DB - OV5658_GAIN_DOUBLE*1;
		u32 index_12dB = OV5658_GAIN_0DB - OV5658_GAIN_DOUBLE*2;

		if(index_0dB >= virtual_index && virtual_index > index_6dB) { // 0dB <= gain < 6dB
			*agc_index = virtual_index;
			*summing_gain = 1;
		} else if(index_6dB >= virtual_index && virtual_index > index_12dB) { // 6dB <= gain < 12dB
			*agc_index = virtual_index + (OV5658_GAIN_DOUBLE*1);	// AGC(dB) = Virtual(dB) - 6dB
			*summing_gain = 2;
		} else { // 12 dB <= gain
			*agc_index = virtual_index + (OV5658_GAIN_DOUBLE*2);	// AGC(dB) = Virtual(dB) - 12dB
			*summing_gain = 4;
		}
	} else {
		return -1;
	}
	vin_dbg("ov5658_agc_index: %d, downsample : %dx\n", *agc_index, downsample);
	return 0;
}
#if 0
static void ov5658_set_binning_summing(struct __amba_vin_source *src, u32 bin_sum_config) // toggle between 1x, 2x, 4x sum
{
	u8 reg_0x3613;
	u8 reg_0x3621;

//	ov5658_read_reg(src, 0x3613, &reg_0x3613);
//	ov5658_read_reg(src, 0x3621, &reg_0x3621);

	switch(bin_sum_config) {
	case 0:
		/* do nothing */
		break;
	case 1:
		reg_0x3621 |= 0x40;  // Reg0x3621[6]=1 for H-binning off: 1x <H-skip>
		reg_0x3613 = 0x44;   // 1x-gain
		break;
	case 2:
		reg_0x3621 &= ~0x40; // Reg0x3621[6]=0 for H-binning sum on: 2x <H-bin sum>
		reg_0x3613 = 0x44;   // 1x-gain
		break;
	case 4:
		reg_0x3621 &= ~0x40; // Reg0x3621[6]=0 for H-binning sum on: 2x <H-bin sum>
		reg_0x3613 = 0xC4;   // 2x-gain
		break;
	default:
		break;
	}

	//ov5658_write_reg(src, 0x3613, reg_0x3613);
	//ov5658_write_reg(src, 0x3621, reg_0x3621);
}
#endif
static int ov5658_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	struct ov5658_info *pinfo;
	u32 virtual_gain_index;
	u32 gain_index=0, summing_gain=0;
	s32 db_min;
	s32 db_step;

	pinfo = (struct ov5658_info *) src->pinfo;

	db_min = pinfo->agc_info.db_min;
	db_step = pinfo->agc_info.db_step;

	virtual_gain_index = (agc_db - db_min) / db_step;
	virtual_gain_index =  OV5658_GAIN_ROWS - virtual_gain_index - 1;
	if(virtual_gain_index < 0 ){
		virtual_gain_index = 0;
	}else if(virtual_gain_index >= OV5658_GAIN_ROWS){
		virtual_gain_index = OV5658_GAIN_ROWS - 1;
	}

	vin_dbg("ov5658_set_agc: 0x%x\n", agc_db);

	if (virtual_gain_index > OV5658_GAIN_0DB)
		virtual_gain_index = OV5658_GAIN_0DB;

	if ((virtual_gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		ov5658_write_reg(src, OV5658_SRM_GRUP_ACCESS, 0x00);

		ov5658_agc_parse_virtual_index(src, virtual_gain_index, &gain_index, &summing_gain);
		/* TODO, add binning case here */
		//ov5658_set_binning_summing(src, summing_gain);

		ov5658_write_reg(src, OV5658_AGC_ADJ_H, OV5658_GAIN_TABLE[gain_index][OV5658_GAIN_COL_REG350A]);
		ov5658_write_reg(src, OV5658_AGC_ADJ_L, OV5658_GAIN_TABLE[gain_index][OV5658_GAIN_COL_REG350B]);

		ov5658_write_reg(src, OV5658_SRM_GRUP_ACCESS, 0x10);
		ov5658_write_reg(src, OV5658_SRM_GRUP_ACCESS, 0xA0);
		pinfo->current_agc_db = agc_db;

		return 0;
	} else{
		return -1;
	}
}

static int ov5658_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
		int 	errCode = 0;
		u32 	vflip = 0,hflip = 0;
		struct	ov5658_info 		*pinfo;
		u32 	target_bayer_pattern;
		u8		tmp_reg;
		pinfo = (struct ov5658_info *) src->pinfo;
		switch (mirror_mode->bayer_pattern) {
		case AMBA_VIN_SRC_BAYER_PATTERN_AUTO:
			break;

		case AMBA_VIN_SRC_BAYER_PATTERN_RG:// 0
		case AMBA_VIN_SRC_BAYER_PATTERN_BG:// 1
		case AMBA_VIN_SRC_BAYER_PATTERN_GR:// 2
		case AMBA_VIN_SRC_BAYER_PATTERN_GB:// 3
			pinfo->bayer_pattern = mirror_mode->bayer_pattern;
			break;

		default:
			vin_err("do not support bayer pattern: %d\n", mirror_mode->bayer_pattern);
			return -EINVAL;
		}
		switch (mirror_mode->pattern) {
		case AMBA_VIN_SRC_MIRROR_AUTO:// 255
			return 0;

		case AMBA_VIN_SRC_MIRROR_VERTICALLY: // 2
			vflip = MIRROR_FLIP_ENABLE;
			target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
			break;

		case AMBA_VIN_SRC_MIRROR_NONE: // 3
			target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
			break;

		case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY: // 0
			vflip = MIRROR_FLIP_ENABLE;
			hflip = MIRROR_FLIP_ENABLE;
			target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
			break;

		case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:// 1
			hflip = MIRROR_FLIP_ENABLE;
			target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
			break;

		default:
			vin_err("do not support cmd mirror mode\n");
			return -EINVAL;
		}
		errCode |= ov5658_read_reg(src,OV5658_V_FLIP_REG,&tmp_reg);
		tmp_reg ^= vflip;
		ov5658_write_reg(src, OV5658_V_FLIP_REG, tmp_reg);

		tmp_reg = 0;
		errCode |= ov5658_read_reg(src,OV5658_H_MIRROR_REG,&tmp_reg);
		tmp_reg ^= hflip;
		ov5658_write_reg(src, OV5658_H_MIRROR_REG, tmp_reg);

		if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
			pinfo->bayer_pattern = target_bayer_pattern;
		}

		return errCode;
}


static int ov5658_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
		/* >> TODO, for YUV output mode only << */
	return errCode;
}
static int ov5658_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;
		/* >> TODO, for RGB raw output mode only << */
		/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}


static int ov5658_get_vblank_time( struct __amba_vin_source *src,
	u32 *ptime)
{
	int				errCode		= 0;
	int				mode_index;
	u32				format_index;
	u64				v_btime;
	u32				active_lines, frame_length_lines;
	u32				line_length;
	u8				data_h, data_l;

	struct ov5658_info 			*pinfo;
	const struct ov5658_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct ov5658_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= ov5658_video_info_table[mode_index].format_index;
	if(OV5658_LANE == 4){
		pll_reg_table	= &ov5658_pll_4lane_tbl[pinfo->pll_index];
	}else if(OV5658_LANE == 2){
		pll_reg_table	= &ov5658_pll_2lane_tbl[pinfo->pll_index];
	}else{
		pll_reg_table	= &ov5658_pll_4lane_tbl[pinfo->pll_index];
	}

	errCode |= ov5658_read_reg(src, OV5658_TIMING_VTS_H, &data_h);
	errCode |= ov5658_read_reg(src, OV5658_TIMING_VTS_L, &data_l);
	frame_length_lines = data_h&0x0F;
	frame_length_lines = (frame_length_lines<<8) + data_l;
	BUG_ON(frame_length_lines == 0);

	errCode |= ov5658_read_reg(src, OV5658_TIMING_HTS_H, &data_h);
	errCode |= ov5658_read_reg(src, OV5658_TIMING_HTS_L, &data_l);
	line_length = data_h&0x1F;
	line_length = (line_length<<8) + data_l;
	BUG_ON(line_length == 0);

	active_lines = ov5658_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pll_reg_table->pixclk); //ns
	*ptime = v_btime;

	vin_dbg("ov5658_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return errCode;
}


static int ov5658_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int errCode = 0;
	struct ov5658_info *pinfo;
	u32 index;
	u32 format_index;
	u64 frame_time_pclk;
	u32 frame_time = 0;
	u16 line_length;
	u8  data_l, data_h;
	u8  current_pll_index = 0;
	u32 max_fps;

	const struct ov5658_pll_reg_table *pll_reg_table;
	struct amba_vin_irq_fix			vin_irq_fix;

	pinfo = (struct ov5658_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= OV5658_VIDEO_INFO_TABLE_SIZE) {
		vin_err("ov5658_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto ov5658_set_fps_exit;
	}

	/* ToDo: Add specified PLL index to lower fps */
	format_index = ov5658_video_info_table[index].format_index;
	max_fps = ov5658_video_format_tbl.table[format_index].max_fps;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = ov5658_video_format_tbl.table[format_index].auto_fps;
	if(fps < ov5658_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, ov5658_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto ov5658_set_fps_exit;
	}

	frame_time = fps;

	vin_dbg("ov5658_set_fps %d max : %d\n", frame_time, max_fps);

	/* ToDo: Add specified PLL index*/

	current_pll_index = ov5658_video_format_tbl.table[format_index].pll_index;

	if(pinfo->pll_index != current_pll_index){
		pinfo->pll_index = current_pll_index;
		ov5658_fill_pll_regs(src);
		if(OV5658_LANE == 4){
			errCode = ov5658_init_vin_clock(src, &ov5658_pll_4lane_tbl[pinfo->pll_index]);
		}else if (OV5658_LANE == 2){
			errCode = ov5658_init_vin_clock(src, &ov5658_pll_2lane_tbl[pinfo->pll_index]);
		}
		if (errCode)
			goto ov5658_set_fps_exit;
	}
	if(OV5658_LANE == 4){
		pll_reg_table = &ov5658_pll_4lane_tbl[pinfo->pll_index];
	}else if (OV5658_LANE == 2){
		pll_reg_table = &ov5658_pll_2lane_tbl[pinfo->pll_index];
	}else{
		pll_reg_table = &ov5658_pll_4lane_tbl[pinfo->pll_index];
	}
	ov5658_read_reg(src, OV5658_TIMING_HTS_H, &data_h);
	ov5658_read_reg(src, OV5658_TIMING_HTS_L, &data_l);
	line_length = (data_h << 8) + data_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}


	frame_time_pclk =  frame_time * (u64)(pll_reg_table->pixclk);
	DO_DIV_ROUND(frame_time_pclk, (u32) line_length);
	DO_DIV_ROUND(frame_time_pclk, 512000000);
	ov5658_write_reg(src, OV5658_TIMING_VTS_H, (u8)((frame_time_pclk >> 8) & 0xff));
	ov5658_write_reg(src, OV5658_TIMING_VTS_L, (u8)(frame_time_pclk & 0xff));
	pinfo->frame_rate = fps;
	ov5658_set_shutter_time(src, pinfo->current_shutter_time);

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_irq_fix.delay = 0;

	errCode = ov5658_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode)
		goto ov5658_set_fps_exit;

	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

ov5658_set_fps_exit:
	return errCode;
}

static int ov5658_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct ov5658_info *pinfo;
	u32 format_index;

	pinfo = (struct ov5658_info *) src->pinfo;
	if (index >= OV5658_VIDEO_INFO_TABLE_SIZE) {
		vin_err("ov5658_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto ov5658_set_mode_exit;
	}

	errCode |= ov5658_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = ov5658_video_info_table[index].format_index;

	pinfo->cap_start_x = ov5658_video_info_table[index].def_start_x;
	pinfo->cap_start_y = ov5658_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = ov5658_video_info_table[index].def_width;
	pinfo->cap_cap_h = ov5658_video_info_table[index].def_height;
	pinfo->bayer_pattern = ov5658_video_info_table[index].bayer_pattern;
	pinfo->downsample = ov5658_video_format_tbl.table[format_index].downsample;
	//set the specified PLL index for each sensor_mode.
	pinfo->pll_index = ov5658_video_format_tbl.table[format_index].pll_index;

	ov5658_print_info(src);
	//set clk_si
	if(OV5658_LANE == 4)
		errCode |= ov5658_init_vin_clock(src, &ov5658_pll_4lane_tbl[pinfo->pll_index]);
	else if (OV5658_LANE == 2)
		errCode |= ov5658_init_vin_clock(src, &ov5658_pll_2lane_tbl[pinfo->pll_index]);

	errCode |= ov5658_set_vin_mode(src);

	//reset IDSP
	ov5658_vin_reset_idsp(src);
	//Enable MIPI
	if(OV5658_LANE == 4)
		ov5658_vin_mipi_phy_enable(src, MIPI_4LANE); //  4 lanes
	else if(OV5658_LANE == 2)
		ov5658_vin_mipi_phy_enable(src, MIPI_2LANE); //  2 lanes

	ov5658_fill_share_regs(src);
	ov5658_fill_pll_regs(src);
	ov5658_fill_video_format_regs(src);

	ov5658_set_fps(src, AMBA_VIDEO_FPS_AUTO);

	/* Enable Streaming */
	ov5658_write_reg(src, 0x4800, 0x24); //mipi clock gate run
	ov5658_set_standby(src, 0);
	ov5658_vin_mipi_reset(src);
	ov5658_write_reg(src, 0x4800, 0x04); //mipi clock free run

	errCode |= ov5658_post_set_vin_mode(src);

	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

ov5658_set_mode_exit:
	return errCode;
}

static int ov5658_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errCode = -EINVAL;
	int i;
	struct ov5658_info *pinfo;
	static int first_set_video_mode = 1;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct ov5658_info *) src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		if(OV5658_LANE == 4){
			errCode = ov5658_init_vin_clock(src, &ov5658_pll_4lane_tbl[pinfo->pll_index]);
		}else{
			errCode = ov5658_init_vin_clock(src, &ov5658_pll_2lane_tbl[pinfo->pll_index]);
		}
		if (errCode)
			goto ov5658_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	ov5658_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	for (i = 0; i < OV5658_VIDEO_MODE_TABLE_SIZE; i++) {
		if (ov5658_video_mode_table[i].mode == mode) {
			errCode = ov5658_set_video_index(src, ov5658_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= OV5658_VIDEO_MODE_TABLE_SIZE) {
		vin_err("ov5658_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = ov5658_video_mode_table[i].mode;
		pinfo->mode_type = ov5658_video_mode_table[i].preview_mode_type;
	}

ov5658_set_video_mode_exit:
	return errCode;
}

static int ov5658_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int				errCode = 0;
	struct ov5658_info		*pinfo;

	pinfo = (struct ov5658_info *) src->pinfo;
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		ov5658_reset(src);
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

	case AMBA_VIN_SRC_GET_INFO:
		{
			struct amba_vin_source_info *pub_src;

			pub_src = (struct amba_vin_source_info *) args;
			pub_src->id = src->id;
			pub_src->adapter_id = adapter_id;
			pub_src->dev_type = src->dev_type;
			strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
			pub_src->sensor_id = SENSOR_OV5658;
			pub_src->default_mode = OV5658_VIDEO_MODE_TABLE_AUTO;
			pub_src->total_channel_num = src->total_channel_num;
			pub_src->active_channel_id = src->active_channel_id;
			pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = ov5658_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = ov5658_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = ov5658_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = ov5658_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = ov5658_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = ov5658_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = ov5658_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = ov5658_set_still_mode(src, (struct amba_vin_src_still_info *) args);
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
		errCode = ov5658_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(u32 *)args = pinfo->current_agc_db;
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = ov5658_set_agc_db(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = ov5658_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = ov5658_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = ov5658_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;
	case AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH:
		errCode=ov5658_shutter_time2width(src, (u32 *) args);
		break;
	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = ov5658_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		{
			u16 sen_id = 0;
			u16 sen_ver = 0;
			u32 *pdata = (u32 *) args;

			errCode = ov5658_query_sensor_id(src, &sen_id);
			if (errCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;
			errCode = ov5658_query_sensor_version(src, &sen_ver);
			if (errCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;

			*pdata = (sen_id << 16) | sen_ver;
		}
		exit_AMBA_VIN_SRC_TEST_GET_DEV_ID:
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u16 subaddr;
			u8 data = 0;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;

			errCode = ov5658_read_reg(src, subaddr, &data);

			reg_data->data = data;
		}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u16 subaddr;
			u8 data = 0;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;
			data = reg_data->data;

			errCode = ov5658_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = ov5658_set_slowshutter_mode(src, *(int *) args);
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
