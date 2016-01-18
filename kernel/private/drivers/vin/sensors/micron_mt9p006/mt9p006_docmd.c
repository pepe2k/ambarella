/*
 * Filename : mt9p006_docmd.c
 *
 * History:
 *    2011/05/23 - [Haowei Lo] Create
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

static void mt9p006_dump_reg(struct __amba_vin_source *src)
{

	u32 i;
	u16 reg_to_dump_init[] = {
	};


	for(i=0; i<sizeof(reg_to_dump_init)/sizeof(reg_to_dump_init[0]); i++) {
		u16 reg_addr = reg_to_dump_init[i];
		u16 reg_val;

		mt9p006_read_reg(src, reg_addr, &reg_val);    // Read sensor revision number
		vin_dbg("REG= 0x%04X, 0x%04X\n", reg_addr, reg_val);
	}

}


static void mt9p006_fill_video_format_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 format_index;
	struct mt9p006_info *pinfo;

	pinfo = (struct mt9p006_info *) src->pinfo;

	vin_dbg("mt9p006_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = mt9p006_video_info_table[index].format_index;

	for (i = 0; i < MT9P006_VIDEO_FORMAT_REG_NUM; i++) {
		if (mt9p006_video_format_tbl.reg[i] == 0)
			break;

		mt9p006_write_reg(src,
				mt9p006_video_format_tbl.reg[i],
				mt9p006_video_format_tbl.table[format_index].data[i]);
	}

	if (mt9p006_video_format_tbl.table[format_index].ext_reg_fill)
		mt9p006_video_format_tbl.table[format_index].ext_reg_fill(src);

}


static void mt9p006_fill_share_regs(struct __amba_vin_source *src)
{
	int i;
	const struct mt9p006_reg_table *reg_tbl = mt9p006_share_regs;
	//struct mt9p006_info *pinfo = (struct mt9p006_info *) src->pinfo;

	for (i = 0; i < MT9P006_SHARE_REG_SZIE; i++) {
		mt9p006_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}
static void mt9p006_start_streaming(struct __amba_vin_source *src)
{
	// GO (resume streaming)
	mt9p006_write_reg(src, MT9P006_RESTART, 0x0001);
}

static void mt9p006_sw_reset(struct __amba_vin_source *src)
{
		/* >> TODO << */
	mt9p006_write_reg(src, MT9P006_RESET, 0x0001);
	msleep(3);
	mt9p006_write_reg(src, MT9P006_RESET, 0x0000);
	msleep(10);
}

static void mt9p006_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	mt9p006_sw_reset(src);
}

static void mt9p006_fill_pll_regs(struct __amba_vin_source *src)
{

	int i;
	struct mt9p006_info *pinfo;
	const struct mt9p006_pll_reg_table 	*pll_reg_table;
	const struct mt9p006_reg_table 	*pll_tbl;

	pinfo = (struct mt9p006_info *) src->pinfo;

	vin_dbg("mt9p006_fill_pll_regs\n");
	pll_reg_table = &mt9p006_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < MT9P006_VIDEO_PLL_REG_TABLE_SIZE; i++) {

			if(pll_tbl[i].reg == MT9P006_PLL_CTRL) {
				msleep(2);	// Ensures VCO has locked
			}
			mt9p006_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
	msleep(2);
}

static int mt9p006_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct mt9p006_info  *pinfo;

	pinfo = (struct mt9p006_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int mt9p006_get_vblank_time( struct __amba_vin_source *src,
	u32 *ptime)
{
	int				errCode = 0;
	int				mode_index;
	u32				format_index;
	u64				v_btime;
	u32				active_lines, frame_length_lines;
	u32				line_length;
	u16				data_val, row_size, col_size;

	struct mt9p006_info 			*pinfo;

	pinfo		= (struct mt9p006_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= mt9p006_video_info_table[mode_index].format_index;

	//READ the row and col size
	errCode |= mt9p006_read_reg(src, MT9P006_COL_SIZE, &col_size);
	errCode |= mt9p006_read_reg(src, MT9P006_ROW_SIZE, &row_size);
	col_size = col_size + 1;
	row_size = row_size + 1;

	if (pinfo->downsample == 2) {
		col_size = (col_size >> 1);
		row_size = (row_size >> 1);
	}

	errCode |= mt9p006_read_reg(src, MT9P006_HORI_BLANKING, &data_val);
	line_length = ((u32)(data_val + 1) << 1);
	line_length += (u32) (col_size);		//line_length = col_size + HB_data *2
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mt9p006_read_reg(src, MT9P006_VERT_BLANKING, &data_val);
	frame_length_lines = (u32) (data_val + 1);
	frame_length_lines += (u32) (row_size);		// frame_length_lines = row_size + vb

	active_lines = mt9p006_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pinfo->pll_table.pixclk); //ns
	*ptime = v_btime;

	vin_dbg("mt9p006_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return errCode;
}

static int mt9p006_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int 				errCode = 0;
	u32 				index;
	u32 				format_index;
	struct mt9p006_info 		*pinfo;

	pinfo = (struct mt9p006_info *) src->pinfo;

	index = pinfo->current_video_index;

	if (index >= MT9P006_VIDEO_INFO_TABLE_SZIE) {
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
		format_index = mt9p006_video_info_table[index].format_index;

		p_video_info->width = mt9p006_video_info_table[index].def_width;
		p_video_info->height = mt9p006_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = mt9p006_video_format_tbl.table[format_index].format;
		p_video_info->type = mt9p006_video_format_tbl.table[format_index].type;
		p_video_info->bits = mt9p006_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = mt9p006_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

static int mt9p006_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct mt9p006_info *pinfo;

	pinfo = (struct mt9p006_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}
static int mt9p006_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));
	return 0;
}

static int mt9p006_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	//struct mt9p006_info *pinfo = (struct mt9p006_info *) src->pinfo;
	u32 index;
	u32 format_index;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < MT9P006_VIDEO_MODE_TABLE_SZIE; i++) {
		if (mt9p006_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = MT9P006_VIDEO_MODE_TABLE_AUTO;
			p_mode_info->is_supported = 1;

			index = mt9p006_video_mode_table[i].still_index;
			format_index = mt9p006_video_info_table[index].format_index;

			p_mode_info->video_info.width =
					mt9p006_video_info_table[index].def_width;
			p_mode_info->video_info.height =
					mt9p006_video_info_table[index].def_height;
			p_mode_info->video_info.fps =
					mt9p006_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format =
					mt9p006_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type =
					mt9p006_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits =
					mt9p006_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio =
					mt9p006_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system =
					AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;
			break;
		}
	}

	return errCode;
}

static int mt9p006_query_sensor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	int errCode = 0;

	errCode = mt9p006_read_reg(src, MT9P006_CHIP_ID, ss_id);

	return errCode;
}

static int mt9p006_query_sensor_version(struct __amba_vin_source *src, u16 * ver)
{
	int errCode = 0;

	return errCode;
}

static int mt9p006_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{

	return 0;

}

static int mt9p006_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
		/* >> TODO, for RGB mode only << */
	return 0;

}

static int mt9p006_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{

	u64 			exposure_time_q9;
	u16 			data_val;
	u32 			line_length, frame_length_lines;
	u32 			shutter_width;
	u16			row_size, col_size;
	int 			errCode = 0;

	struct mt9p006_info *pinfo;

	pinfo		= (struct mt9p006_info *)src->pinfo;

	exposure_time_q9 = shutter_time; 		// time*512*1000000

	//READ the row and col size
	errCode |= mt9p006_read_reg(src, MT9P006_COL_SIZE, &col_size);
	errCode |= mt9p006_read_reg(src, MT9P006_ROW_SIZE, &row_size);
	col_size = col_size + 1;
	row_size = row_size + 1;

	if (pinfo->downsample == 2) {
		col_size = (col_size >> 1);
		row_size = (row_size >> 1);
	}

	errCode |= mt9p006_read_reg(src, MT9P006_HORI_BLANKING, &data_val);
	line_length = ((u32)(data_val + 1) << 1);
	line_length += (u32) (col_size);		//line_length = col_size + HB_data *2
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mt9p006_read_reg(src, MT9P006_VERT_BLANKING, &data_val);
	frame_length_lines = (u32) (data_val + 1);
	frame_length_lines += (u32) (row_size);		// frame_length_lines = row_size + vb

	exposure_time_q9 = exposure_time_q9 * (u64)pinfo->pll_table.pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (u16) exposure_time_q9;

	/* FIXME: shutter width: 1 ~(Frame format(V) - 4) */
	if(shutter_width < 1) {
		shutter_width = 1;
	} else if(shutter_width  > frame_length_lines - 4) {
		shutter_width = frame_length_lines - 4;
	}

	errCode |= mt9p006_write_reg(src, MT9P006_SHR_WIDTH_UPPER, (shutter_width >> 16) & 0x0000ffff );
	errCode |= mt9p006_write_reg(src, MT9P006_SHR_WIDTH, (shutter_width) & 0x0000ffff );
	pinfo->current_shutter_time = shutter_time;

	vin_dbg("shutter_width:%d\n", shutter_width);

	return errCode;

}

static int mt9p006_agc_parse_virtual_index(struct __amba_vin_source *src, u32 virtual_index, u32 *agc_index, u32 *summing_gain)
{
	struct  mt9p006_info *pinfo;
	u32 downsample;

	pinfo = (struct  mt9p006_info *) src->pinfo;
	downsample = pinfo->downsample;

	if(downsample == 1) {
		*agc_index = virtual_index;
		*summing_gain = 0;
	} else if(downsample == 2) {

		u32 index_0dB  = MT9P006_GAIN_0DB;
		u32 index_12dB = MT9P006_GAIN_0DB - MT9P006_GAIN_DOUBLE*2;

		if(index_0dB >= virtual_index && virtual_index > index_12dB) { // 0dB <= gain < 12dB
			*agc_index = virtual_index;
			*summing_gain = 1;
		} else {
			*agc_index = virtual_index + (MT9P006_GAIN_DOUBLE*1);
			*summing_gain = 2;
		}
	} else if (downsample == 4) {
		*agc_index = virtual_index;
		*summing_gain = 1;
	} else {
		return -1;
	}
	vin_dbg("mt9p006_agc_index: %d, downsample : %dx\n", *agc_index, downsample);
	return 0;
}

static void mt9p006_set_binning_summing(struct __amba_vin_source *src, u32 bin_sum_config) // toggle between 1x, 2x, 4x sum
{

	switch(bin_sum_config) {
	case 0:
		/* do nothing */
		break;
	case 1:
		mt9p006_write_reg(src, MT9P006_READ_MODE_2, 0x0040); // MT9P006_READ_MODE_2
		break;
	case 2:
		mt9p006_write_reg(src, MT9P006_READ_MODE_2, 0x0060); // MT9P006_READ_MODE_2
		break;
	default:
		break;
	}
}

static int mt9p006_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	struct mt9p006_info *pinfo;
	u32 gain_index = 0;
	u32 virtual_gain_index;
	s32 db_max;
	s32 db_step;
	u16 data_val;
	u32 summing_gain = 0;

	vin_dbg("mt9p006_set_agc_db: 0x%x\n", agc_db);

	pinfo = (struct mt9p006_info *) src->pinfo;
	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	virtual_gain_index = (db_max - agc_db) / db_step;

	if (virtual_gain_index > MT9P006_GAIN_ROWS)
		virtual_gain_index = (MT9P006_GAIN_ROWS - 1);
	if ((virtual_gain_index >= pinfo->min_agc_index) && (virtual_gain_index <= pinfo->max_agc_index)) {

		mt9p006_agc_parse_virtual_index(src, virtual_gain_index, &gain_index, &summing_gain);

		/* TODO, add binning case here */
		mt9p006_set_binning_summing(src, summing_gain);

		// For Micron sensor defect, Blue strap under light condition
		if(gain_index >= (MT9P006_GAIN_0DB - MT9P006_GAIN_DOUBLE)) {
			mt9p006_write_reg(src, MT9P006_REG_7F, 0x0000);
		} else {
			if (pinfo->downsample == 2) {
				mt9p006_write_reg(src, MT9P006_REG_7F, 0x5900);
			} else {
				mt9p006_write_reg(src, MT9P006_REG_7F, 0x7800);
			}
		}

		mt9p006_read_reg(src, MT9P006_READ_MODE_1, &data_val);

		if(gain_index <= (MT9P006_GAIN_0DB - 3*MT9P006_GAIN_DOUBLE)) {
				/* disable anti-blooming for reducing black dots */
			mt9p006_write_reg(src, MT9P006_READ_MODE_1, data_val & 0xBFFF);  // MT9P401_READ_MODE_1
		} else {
				/* enable anti-blooming */
			mt9p006_write_reg(src, MT9P006_READ_MODE_1, data_val | 0x4000);  // MT9P401_READ_MODE_1
		}

		mt9p006_write_reg(src, MT9P006_GLOBAL_GAIN,  MT9P006_GAIN_TABLE[gain_index][MT9P006_GAIN_COL_REG]);  // MT9P401_GLOBAL_GAIN

		pinfo->current_gain_db = agc_db;

		return 0;
	} else{
		return -1;
	}
}
static int mt9p006_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int errCode = 0;
	uint readmode;
	uint bayer_pattern;
	struct mt9p006_info *pinfo;
	u16 tmp_reg;

	pinfo = (struct mt9p006_info *) src->pinfo;
	switch (mirror_mode->pattern) {
	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
		bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GB;
		readmode = MT9P006_READMODE2_REG_MIRROR_ROW + MT9P006_READMODE2_REG_MIRROR_COLUMN;
		break;
	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		readmode = MT9P006_READMODE2_REG_MIRROR_COLUMN;
		break;
	case AMBA_VIN_SRC_MIRROR_VERTICALLY:
		bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_BG;
		readmode = MT9P006_READMODE2_REG_MIRROR_ROW;
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

	pinfo->bayer_pattern = bayer_pattern;
	errCode |= mt9p006_read_reg(src, MT9P006_READ_MODE_2, &tmp_reg);
	tmp_reg &= (~MT9P006_MIRROR_MASK);
	tmp_reg |= readmode;
	errCode |= mt9p006_write_reg(src, MT9P006_READ_MODE_2, tmp_reg);
	return errCode;
}
static int mt9p006_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
		/* >> TODO, for YUV output mode only << */
	return errCode;
}
static int mt9p006_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;
		/* >> TODO, for RGB raw output mode only << */
		/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}
static int mt9p006_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int 					errCode = 0;
	struct mt9p006_info 	*pinfo;
	u32					index;
	u32					format_index;
	u64					frame_time_pclk;
	u32					frame_time = 0;
	u32					frame_rate = 0;
	u32					line_length;
	u16					data_val;
	u32					max_fps;
	u16					col_size;
	u16					row_size;
	const struct mt9p006_pll_reg_table 	*pll_reg_table;
	struct amba_vin_irq_fix		vin_irq_fix;

	pinfo = (struct mt9p006_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= MT9P006_VIDEO_INFO_TABLE_SZIE) {
		vin_err("mt9p006_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto mt9p006_set_fps_exit;
	}

	/* ToDo: Add specified PLL index to lower fps */

	format_index = mt9p006_video_info_table[index].format_index;
	pll_reg_table = &mt9p006_pll_tbl[pinfo->pll_index];
	max_fps = mt9p006_video_format_tbl.table[format_index].max_fps;

	if (fps == AMBA_VIDEO_FPS_AUTO) {
		fps = mt9p006_video_format_tbl.table[format_index].auto_fps;
	}
	if(fps < mt9p006_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, mt9p006_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto mt9p006_set_fps_exit;
	}

	frame_time = fps;

	vin_dbg("mt9p006_set_fps %d max : %d\n", frame_time, max_fps);

	errCode |= mt9p006_init_vin_clock(src, pll_reg_table);
	if (errCode)
		goto mt9p006_set_fps_exit;
	pinfo->pll_table.pixclk = pll_reg_table->pixclk;
	pinfo->pll_table.extclk = pll_reg_table->extclk;

	//READ the row and col size
	errCode |= mt9p006_read_reg(src, MT9P006_COL_SIZE, &col_size);
	errCode |= mt9p006_read_reg(src, MT9P006_ROW_SIZE, &row_size);
	col_size = col_size + 1;
	row_size = row_size + 1;

	if (pinfo->downsample == 2) {
		col_size = (col_size >> 1);
		row_size = (row_size >> 1);
	}

	frame_rate = DIV_ROUND(512000000, frame_time);
	max_fps = DIV_ROUND(512000000, max_fps);

	if (frame_rate <= (max_fps >> 1)) {
		/*
			If fps is smaller than the half of max fps.
			We need to tune the hori_blnaking first (double the
			line length first).
			New hori_blanking = col_size/2 + 2*old_hori_blanking.
		*/

		data_val = ((mt9p006_video_format_tbl.table[format_index].data[4] + 1) << 1);  //hori_blanking

		data_val += (col_size >> 1);
		if (data_val >= (4094 - col_size) / 2) {
			data_val = (4094 - col_size) / 2;
		}
		mt9p006_write_reg(src, MT9P006_HORI_BLANKING, data_val);
	} else {
		data_val = mt9p006_video_format_tbl.table[format_index].data[4] + 1;  //hori_blanking
		mt9p006_write_reg(src, MT9P006_HORI_BLANKING, data_val);
	}

	errCode |= mt9p006_read_reg(src, MT9P006_HORI_BLANKING, &data_val);
	line_length =  (u32) ((data_val + 1)) << 1;
	line_length += (u32) (col_size);	//line_length = col_size + HB_data *2
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	frame_time_pclk =  frame_time * (u64)(pll_reg_table->pixclk);

	DO_DIV_ROUND(frame_time_pclk, line_length);
	DO_DIV_ROUND(frame_time_pclk, 512000000);

	frame_time_pclk = (u32)frame_time_pclk - (u32)(row_size);

	if(frame_time_pclk > 0x800) {//workround: register 0x06 limitation-0x7FF
		frame_time_pclk = 0x800 + row_size;
		pinfo->pll_table.pixclk = line_length * frame_time_pclk * frame_rate;
		pinfo->pll_table.extclk = pinfo->pll_table.pixclk >> 2;//EXT_CLK = PIX_CLK/4
		errCode |= mt9p006_init_vin_clock(src, &pinfo->pll_table);
		frame_time_pclk = 0x800;
		if (errCode)
			goto mt9p006_set_fps_exit;
	}

	mt9p006_write_reg(src, MT9P006_VERT_BLANKING, (u16)(frame_time_pclk -1));

	mt9p006_set_shutter_time(src, pinfo->current_shutter_time);

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	errCode = mt9p006_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode) {
		goto mt9p006_set_fps_exit;
	}
	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

mt9p006_set_fps_exit:
	return errCode;
}

static int mt9p006_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct mt9p006_info *pinfo;
	u32 format_index;

	pinfo = (struct mt9p006_info *) src->pinfo;

	if (index >= MT9P006_VIDEO_INFO_TABLE_SZIE) {
		vin_err("mt9p006_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto mt9p006_set_mode_exit;
	}

	errCode |= mt9p006_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = mt9p006_video_info_table[index].format_index;

	pinfo->cap_start_x = mt9p006_video_info_table[index].def_start_x;
	pinfo->cap_start_y = mt9p006_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = mt9p006_video_info_table[index].def_width;
	pinfo->cap_cap_h = mt9p006_video_info_table[index].def_height;
	pinfo->bayer_pattern = mt9p006_video_info_table[index].bayer_pattern;
	pinfo->downsample = mt9p006_video_format_tbl.table[format_index].downsample;
	//set the specified PLL index for each sensor_mode.
	pinfo->pll_index = mt9p006_video_format_tbl.table[format_index].pll_index;

	//set clk_si
	errCode |= mt9p006_init_vin_clock(src, &mt9p006_pll_tbl[pinfo->pll_index]);

	errCode |= mt9p006_set_vin_mode(src);

	mt9p006_fill_share_regs(src);

	mt9p006_fill_pll_regs(src);

	mt9p006_fill_video_format_regs(src);

	mt9p006_set_fps(src, AMBA_VIDEO_FPS_AUTO);
	mt9p006_set_shutter_time(src, AMBA_VIDEO_FPS_60);	//the default fps = 60.
	mt9p006_set_agc_db(src, 0);

	errCode |= mt9p006_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

	/* Enable Streaming */
	mt9p006_start_streaming(src);

	mt9p006_print_info(src);

mt9p006_set_mode_exit:
	return errCode;
}

static int mt9p006_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int 					errCode = -EINVAL;
	int 					i;
	struct mt9p006_info 			*pinfo;
	int 					errorCode = 0;
	static int 				first_set_video_mode = 1;

	pinfo = (struct mt9p006_info *) src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errorCode = mt9p006_init_vin_clock(src, &mt9p006_pll_tbl[pinfo->pll_index]);
		if (errorCode)
			goto mt9p006_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	mt9p006_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	for (i = 0; i < MT9P006_VIDEO_MODE_TABLE_SZIE; i++) {
		if (mt9p006_video_mode_table[i].mode == mode) {
			errCode = mt9p006_set_video_index(src, mt9p006_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= MT9P006_VIDEO_MODE_TABLE_SZIE) {
		vin_err("mt9p006_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = mt9p006_video_mode_table[i].mode;
		pinfo->mode_type = mt9p006_video_mode_table[i].preview_mode_type;
	}

mt9p006_set_video_mode_exit:
	return errCode;
}

static int mt9p006_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	u16				sen_id = 0;
	struct mt9p006_info	*pinfo;

	pinfo = (struct mt9p006_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = mt9p006_init_vin_clock(src, &mt9p006_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto mt9p006_init_hw_exit;
	msleep(10);
	mt9p006_reset(src);

	errCode = mt9p006_query_sensor_id(src, &sen_id);
	if (errCode)
		goto mt9p006_init_hw_exit;
	/*
	if (sen_id != expected id) {
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		goto mt9p006_init_hw_exit;
	}*/

	DRV_PRINT("MT9P006 sensor ID is 0x%x\n", sen_id);

mt9p006_init_hw_exit:
	return errCode;
}

static int mt9p006_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int 			errCode = 0;
	struct mt9p006_info	*pinfo;
	pinfo = (struct mt9p006_info *) src->pinfo;

	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		mt9p006_reset(src);
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
		errCode = mt9p006_init_hw(src);
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
			pub_src->default_mode = MT9P006_VIDEO_MODE_TABLE_AUTO;
			pub_src->total_channel_num = src->total_channel_num;
			pub_src->active_channel_id = src->active_channel_id;
			pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = mt9p006_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = mt9p006_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = mt9p006_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = mt9p006_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = mt9p006_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = mt9p006_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = mt9p006_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = mt9p006_set_still_mode(src, (struct amba_vin_src_still_info *) args);
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
		errCode = mt9p006_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(u32 *)args = pinfo->current_gain_db;
		break;
	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = mt9p006_set_agc_db(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = mt9p006_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = mt9p006_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = mt9p006_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = mt9p006_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		mt9p006_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		{
			u16 sen_id = 0;
			u16 sen_ver = 0;
			u32 *pdata = (u32 *) args;

			errCode = mt9p006_query_sensor_id(src, &sen_id);
			if (errCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;
			errCode = mt9p006_query_sensor_version(src, &sen_ver);
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
			u16 data = 0;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;

			errCode = mt9p006_read_reg(src, subaddr, &data);

			reg_data->data = data;
		}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u16 subaddr;
			u16 data = 0;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;
			data = reg_data->data;

			errCode = mt9p006_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = mt9p006_set_slowshutter_mode(src, *(int *) args);
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
