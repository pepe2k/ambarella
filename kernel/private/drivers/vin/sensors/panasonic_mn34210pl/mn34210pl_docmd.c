/*
 * kernel/private/drivers/ambarella/vin/sensors/panasonic_mn34210pl/mn34210pl_docmd.c
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

static void mn34210pl_dump_reg(struct __amba_vin_source *src)
{
	u32 i;
	u16 reg_to_dump_init[] = {};

	for(i=0; i<sizeof(reg_to_dump_init)/sizeof(reg_to_dump_init[0]); i++) {
		u16 reg_addr = reg_to_dump_init[i];
		u8 reg_val;

		mn34210pl_read_reg(src, reg_addr, &reg_val);    // Read sensor revision number
		vin_dbg("REG= 0x%04X, 0x%04X\n", reg_addr, reg_val);
	}

}

static void mn34210pl_fill_video_format_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 format_index;
	struct mn34210pl_info *pinfo;
	int mn34210pl_format_regs_size = 0;
	const struct mn34210pl_reg_table *reg_tbl = NULL;

	pinfo = (struct mn34210pl_info *) src->pinfo;

	vin_dbg("mn34210pl_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = mn34210pl_video_info_table[index].format_index;

	if (format_index == 0) {/* 1296x1032@12bits */
		reg_tbl = mn34210pl_linear_share_regs;
		mn34210pl_format_regs_size = MN34210PL_LINEAR_SHARE_REG_SIZE;
	} else if (format_index == 4) {/* 1296x1032@12bits */
		reg_tbl = mn34210pl_720p_share_regs;
		mn34210pl_format_regs_size = MN34210PL_720P_SHARE_REG_SIZE;
	}

	for (i = 0; i < mn34210pl_format_regs_size; i++) {
		mn34210pl_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}

	for (i = 0; i < MN34210PL_VIDEO_FORMAT_REG_NUM; i++) {
		if (mn34210pl_video_format_tbl.reg[i] == 0)
			break;

		mn34210pl_write_reg(src,
			mn34210pl_video_format_tbl.reg[i],
			mn34210pl_video_format_tbl.table[format_index].data[i]);
	}

	if (mn34210pl_video_format_tbl.table[format_index].ext_reg_fill)
		mn34210pl_video_format_tbl.table[format_index].ext_reg_fill(src);
}

static void mn34210pl_fill_share_regs(struct __amba_vin_source *src)
{
	int i, mn34210pl_share_regs_size;
	const struct mn34210pl_reg_table *reg_tbl;
	struct mn34210pl_info *pinfo;

	pinfo = (struct mn34210pl_info *) src->pinfo;

	switch(pinfo->op_mode) {
	case MN34210PL_LINEAR_MODE:
		reg_tbl = NULL;
		mn34210pl_share_regs_size = 0;
		vin_info("Linear mode\n");
		break;
	case MN34210PL_2X_WDR_MODE:
		reg_tbl = mn34210pl_2x_wdr_share_regs;
		mn34210pl_share_regs_size = MN34210PL_2X_WDR_SHARE_REG_SIZE;
		vin_info("2x WDR mode\n");
		break;
	case MN34210PL_3X_WDR_MODE:
		reg_tbl = mn34210pl_3x_wdr_share_regs;
		mn34210pl_share_regs_size = MN34210PL_3X_WDR_SHARE_REG_SIZE;
		vin_info("3x WDR mode\n");
		break;
	case MN34210PL_4X_WDR_MODE:
		reg_tbl = mn34210pl_4x_wdr_share_regs;
		mn34210pl_share_regs_size = MN34210PL_4X_WDR_SHARE_REG_SIZE;
		vin_info("4x WDR mode\n");
		break;
	default:
		reg_tbl = NULL;
		mn34210pl_share_regs_size = 0;
		vin_err("Unknown mode\n");
		break;
	}

	for (i = 0; i < mn34210pl_share_regs_size; i++) {
		mn34210pl_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void mn34210pl_sw_reset(struct __amba_vin_source *src)
{
	mn34210pl_write_reg(src, 0x3001, 0x0000);
	msleep(20);
}

static void mn34210pl_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	mn34210pl_sw_reset(src);
}

static void mn34210pl_fill_pll_regs(struct __amba_vin_source *src)
{

	int i;
	struct mn34210pl_info *pinfo;
	const struct mn34210pl_pll_reg_table 	*pll_reg_table;
	const struct mn34210pl_reg_table 		*pll_tbl;

	pinfo = (struct mn34210pl_info *) src->pinfo;

	vin_dbg("mn34210pl_fill_pll_regs\n");
	pll_reg_table = &mn34210pl_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < MN34210PL_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			mn34210pl_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
}

static int mn34210pl_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct mn34210pl_info  *pinfo;

	pinfo = (struct mn34210pl_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

/*
static int mn34210pl_get_vblank_time( struct __amba_vin_source *src,
	u32 *ptime)
{
	int				errCode = 0;
	int				mode_index;
	u32				format_index;
	u64				v_btime;
	u32				active_lines, frame_length_lines;
	u32				line_length;
	u8				data_val;

	struct mn34210pl_info 			*pinfo;
	const struct mn34210pl_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct mn34210pl_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= mn34210pl_video_info_table[mode_index].format_index;
	pll_reg_table	= &mn34210pl_pll_tbl[pinfo->pll_index];

	errCode |= mn34210pl_read_reg(src, 0x0343, &data_val);//HCYCLE_LSB
	if(errCode != 0)
		return errCode;
	line_length = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0342, &data_val);//HCYCLE_MSB
	if(errCode != 0)
		return errCode;
	line_length |= data_val<<8;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mn34210pl_read_reg(src, 0x0341, &data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0340, &data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= data_val<<8;

	active_lines = mn34210pl_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pll_reg_table->pixclk); //ns
	*ptime = v_btime;

	vin_dbg("mn34210pl_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return errCode;
}
*/

static int mn34210pl_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errCode = 0;
	u32 index;
	u32 format_index;

	struct mn34210pl_info *pinfo;

	pinfo = (struct mn34210pl_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= MN34210PL_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = mn34210pl_video_info_table[index].format_index;
		p_video_info->width = mn34210pl_video_info_table[index].act_width;
		p_video_info->height = mn34210pl_video_info_table[index].act_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = mn34210pl_video_format_tbl.table[format_index].format;
		p_video_info->type = mn34210pl_video_format_tbl.table[format_index].type;
		p_video_info->bits = mn34210pl_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = mn34210pl_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

static int mn34210pl_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct mn34210pl_info *pinfo;

	pinfo = (struct mn34210pl_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}

static int mn34210pl_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));
	return 0;
}

static int mn34210pl_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;
	struct mn34210pl_info *pinfo;

	pinfo = (struct mn34210pl_info *) src->pinfo;
	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < MN34210PL_VIDEO_MODE_TABLE_SIZE; i++) {
		if (mn34210pl_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = MN34210PL_VIDEO_MODE_TABLE_AUTO;

			p_mode_info->is_supported = 1;

			/* TODO */
			if(p_mode_info->mode == MN34210PL_VIDEO_MODE_TABLE_AUTO) {
				switch(pinfo->op_mode) {
					case MN34210PL_LINEAR_MODE:
						mn34210pl_video_mode_table[i].preview_index = 1;
						mn34210pl_video_mode_table[i].still_index = 1;
						break;
					case MN34210PL_2X_WDR_MODE:
						mn34210pl_video_mode_table[i].preview_index = 2;
						mn34210pl_video_mode_table[i].still_index = 2;
						break;
					case MN34210PL_3X_WDR_MODE:
						mn34210pl_video_mode_table[i].preview_index = 3;
						mn34210pl_video_mode_table[i].still_index = 3;
						break;
					case MN34210PL_4X_WDR_MODE:
						mn34210pl_video_mode_table[i].preview_index = 4;
						mn34210pl_video_mode_table[i].still_index = 4;
						break;
					default:
						break;
				}
			}

			index = mn34210pl_video_mode_table[i].still_index;
			format_index = mn34210pl_video_info_table[index].format_index;

			p_mode_info->video_info.width = mn34210pl_video_info_table[index].act_width;
			p_mode_info->video_info.height = mn34210pl_video_info_table[index].act_height;
			p_mode_info->video_info.fps = mn34210pl_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = mn34210pl_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = mn34210pl_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = mn34210pl_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = mn34210pl_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;
			break;
		}
	}

	return errCode;
}

static int mn34210pl_query_sensor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	int 	errCode = 0;
	return errCode;
}

static int mn34210pl_query_sensor_version(struct __amba_vin_source *src, u16 * ver)
{
	/* >> TODO << */
	return 0;
}

static int mn34210pl_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	u64 exposure_time_q9;
	u8 data_val;
	u32 line_length, frame_length_lines;
	int shutter_width;
	int errCode = 0;

	struct mn34210pl_info *pinfo;
	const struct mn34210pl_pll_reg_table *pll_reg_table;

	pinfo		= (struct mn34210pl_info *)src->pinfo;
	pll_reg_table	= &mn34210pl_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time; 		// time*512*1000000

	errCode |= mn34210pl_read_reg(src, 0x0343, &data_val);//HCYCLE_LSB
	if(errCode != 0)
		return errCode;
	line_length = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0342, &data_val);//HCYCLE_MSB
	if(errCode != 0)
		return errCode;
	line_length |= data_val<<8;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mn34210pl_read_reg(src, 0x0341, &data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0340, &data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= data_val<<8;

	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (u32)exposure_time_q9;

	/* FIXME: shutter width: 2 ~(Frame format(V) - 4) */
	if(shutter_width < 2) {
		shutter_width = 2;
	} else if(shutter_width  > frame_length_lines - 4) {
		shutter_width = frame_length_lines - 4;
	}

	errCode |= mn34210pl_write_reg(src, 0x0203, (u8)(shutter_width & 0xFF));
	if(errCode != 0)
		return errCode;

	errCode |= mn34210pl_write_reg(src, 0x0202, (u8)(shutter_width >> 8));
	if(errCode != 0)
		return errCode;
	vin_dbg("V:%d, shutter:%d\n", frame_length_lines, shutter_width);

	pinfo->current_shutter_time = shutter_time;

	return errCode;
}
static int mn34210pl_shutter_time2width(struct __amba_vin_source *src, u32* shutter_time)
{
	u64 exposure_time_q9;
	u8 data_val;
	u32 line_length, frame_length_lines;
	int shutter_width;
	int errCode = 0;

	struct mn34210pl_info *pinfo;
	const struct mn34210pl_pll_reg_table *pll_reg_table;

	pinfo		= (struct mn34210pl_info *)src->pinfo;
	pll_reg_table	= &mn34210pl_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = *shutter_time; 		// time*512*1000000

	errCode |= mn34210pl_read_reg(src, 0x0343, &data_val);//HCYCLE_LSB
	if(errCode != 0)
		return errCode;
	line_length = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0342, &data_val);//HCYCLE_MSB
	if(errCode != 0)
		return errCode;
	line_length |= data_val<<8;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mn34210pl_read_reg(src, 0x0341, &data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0340, &data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= data_val<<8;

	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (u32)exposure_time_q9;

	/* FIXME: shutter width: 2 ~(Frame format(V) - 4) */
	if(shutter_width < 2) {
		shutter_width = 2;
	} else if(shutter_width  > frame_length_lines - 4) {
		shutter_width = frame_length_lines - 4;
	}
	*shutter_time =shutter_width;

	return errCode;
}

static int mn34210pl_set_shutter_time_row(struct __amba_vin_source *src, u32 row)
{
	u64 exposure_time_q9;
	u8 data_val;
	u32 line_length, frame_length_lines;
	int shutter_width;
	int errCode = 0;

	struct mn34210pl_info *pinfo;
	const struct mn34210pl_pll_reg_table *pll_reg_table;

	pinfo		= (struct mn34210pl_info *)src->pinfo;
	pll_reg_table	= &mn34210pl_pll_tbl[pinfo->pll_index];

	errCode |= mn34210pl_read_reg(src, 0x0343, &data_val);//HCYCLE_LSB
	if(errCode != 0)
		return errCode;
	line_length = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0342, &data_val);//HCYCLE_MSB
	if(errCode != 0)
		return errCode;
	line_length |= data_val<<8;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mn34210pl_read_reg(src, 0x0341, &data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0340, &data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= data_val<<8;

	shutter_width = row;

	/* FIXME: shutter width: 2 ~(Frame format(V) - 4) */
	if((shutter_width < 2) ||(shutter_width  > frame_length_lines - 4)) {
		vin_warn("row number %d err, valid range is %d - %d\n", shutter_width, 2, frame_length_lines - 4);
		return -EPERM;
	}

	errCode |= mn34210pl_write_reg(src, 0x0203, (u8)(shutter_width & 0xFF));
	if(errCode != 0)
		return errCode;

	errCode |= mn34210pl_write_reg(src, 0x0202, (u8)(shutter_width >> 8));
	if(errCode != 0)
		return errCode;

	exposure_time_q9 = shutter_width;

	exposure_time_q9 = exposure_time_q9 * (u64)line_length * 512000000;
	DO_DIV_ROUND(exposure_time_q9, pll_reg_table->pixclk);

	pinfo->current_shutter_time = (u32)exposure_time_q9;

	vin_dbg("shutter_time:%d, row:%d\n", pinfo->current_shutter_time, shutter_width);

	return errCode;
}
static int mn34210pl_set_shutter_time_row_sync(struct __amba_vin_source *src, u32 row)
{
	u64 exposure_time_q9;
	u8 data_val;
	u32 line_length, frame_length_lines;
	int shutter_width;
	int errCode = 0;
	u32 vsync_irq;

	struct mn34210pl_info *pinfo;
	const struct mn34210pl_pll_reg_table *pll_reg_table;

	pinfo		= (struct mn34210pl_info *)src->pinfo;
	pll_reg_table	= &mn34210pl_pll_tbl[pinfo->pll_index];

	errCode |= mn34210pl_read_reg(src, 0x0343, &data_val);//HCYCLE_LSB
	if(errCode != 0)
		return errCode;
	line_length = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0342, &data_val);//HCYCLE_MSB
	if(errCode != 0)
		return errCode;
	line_length |= data_val<<8;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mn34210pl_read_reg(src, 0x0341, &data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0340, &data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= data_val<<8;

	shutter_width = row;

	/* FIXME: shutter width: 2 ~(Frame format(V) - 4) */
	if((shutter_width < 2) ||(shutter_width  > frame_length_lines - 4)) {
		vin_warn("row number %d err, valid range is %d - %d\n", shutter_width, 2, frame_length_lines - 4);
		return -EPERM;
	}
	vsync_irq = vsync_irq_flag;
	pinfo->agc_call_cnt++;
	wait_event_interruptible(vsync_irq_wait, vsync_irq < vsync_irq_flag || pinfo->agc_call_cnt < 7);//FIXME
	errCode |= mn34210pl_write_reg(src, 0x0203, (u8)(shutter_width & 0xFF));
	if(errCode != 0)
		return errCode;

	errCode |= mn34210pl_write_reg(src, 0x0202, (u8)(shutter_width >> 8));
	if(errCode != 0)
		return errCode;

	exposure_time_q9 = shutter_width;

	exposure_time_q9 = exposure_time_q9 * (u64)line_length * 512000000;
	DO_DIV_ROUND(exposure_time_q9, pll_reg_table->pixclk);

	pinfo->current_shutter_time = (u32)exposure_time_q9;

	vin_dbg("shutter_time:%d, row:%d\n", pinfo->current_shutter_time, shutter_width);

	return errCode;
}

static int mn34210pl_set_fps(struct __amba_vin_source *src, u32 fps)
{
	struct amba_vin_irq_fix		vin_irq_fix;
	struct mn34210pl_info 		*pinfo;
	u32 	index;
	u32 	format_index;
	u64		frame_time_pclk;
	u32  	frame_time = 0;
	u16 	line_length;
	u8 		data_val, current_pll_index;
	const struct mn34210pl_pll_reg_table 	*pll_reg_table;
	int		errCode = 0;

	pinfo = (struct mn34210pl_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= MN34210PL_VIDEO_INFO_TABLE_SIZE) {
		vin_err("mn34210pl_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto mn34210pl_set_fps_exit;
	}

	/* ToDo: Add specified PLL index to lower fps */

	format_index = mn34210pl_video_info_table[index].format_index;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = mn34210pl_video_format_tbl.table[format_index].auto_fps;
	if(fps < mn34210pl_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, mn34210pl_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto mn34210pl_set_fps_exit;
	}

	frame_time = fps;

	vin_dbg("mn34210pl_set_fps %d\n", frame_time);

	current_pll_index = mn34210pl_video_format_tbl.table[format_index].pll_index;
	/* FIXME */
	if ((format_index == 0) && (fps == AMBA_VIDEO_FPS_29_97)) {//linear mode, 29.97
		current_pll_index = 4;
	}

	if(pinfo->pll_index != current_pll_index){
		pinfo->pll_index = current_pll_index;
		errCode = mn34210pl_init_vin_clock(src, &mn34210pl_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto mn34210pl_set_fps_exit;
	}

	errCode |= mn34210pl_read_reg(src, 0x0343, &data_val);//HCYCLE_LSB
	if(errCode != 0)
		return errCode;
	line_length = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0342, &data_val);//HCYCLE_MSB
	if(errCode != 0)
		return errCode;
	line_length |= data_val<<8;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	pll_reg_table = &mn34210pl_pll_tbl[pinfo->pll_index];

	frame_time_pclk =  frame_time * (u64)(pll_reg_table->pixclk);

	DO_DIV_ROUND(frame_time_pclk, (u32) line_length);
	DO_DIV_ROUND(frame_time_pclk, 512000000);

	mn34210pl_write_reg(src, 0x0340, (u8)(frame_time_pclk >> 8));
	mn34210pl_write_reg(src, 0x0341, (u8)frame_time_pclk);

	mn34210pl_set_shutter_time(src, pinfo->current_shutter_time);//keep same exposure time

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_irq_fix.delay = 0;// change to zero for all non Aptina Hi-SPI sensors (for A5s/A7/S2)
	/*
	errCode = mn34210pl_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode) {
		goto mn34210pl_set_fps_exit;
	}
	*/
	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

mn34210pl_set_fps_exit:
	return errCode;
}

static int mn34210pl_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct mn34210pl_info *pinfo;
	u32 format_index;

	pinfo = (struct mn34210pl_info *) src->pinfo;

	if (index >= MN34210PL_VIDEO_INFO_TABLE_SIZE) {
		vin_err("mn34210pl_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto mn34210pl_set_mode_exit;
	}

	errCode |= mn34210pl_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = mn34210pl_video_info_table[index].format_index;

	pinfo->cap_start_x = mn34210pl_video_info_table[index].def_start_x;
	pinfo->cap_start_y = mn34210pl_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = mn34210pl_video_info_table[index].def_width;
	pinfo->cap_cap_h = mn34210pl_video_info_table[index].def_height;
	pinfo->act_start_x = mn34210pl_video_info_table[index].act_start_x;
	pinfo->act_start_y = mn34210pl_video_info_table[index].act_start_y;
	pinfo->act_width = mn34210pl_video_info_table[index].act_width;
	pinfo->act_height = mn34210pl_video_info_table[index].act_height;
	pinfo->slvs_eav_col = mn34210pl_video_info_table[index].slvs_eav_col;
	pinfo->slvs_sav2sav_dist = mn34210pl_video_info_table[index].slvs_sav2sav_dist;
	pinfo->bayer_pattern = mn34210pl_video_info_table[index].bayer_pattern;
	//set the specified PLL index for each sensor_mode.
	pinfo->pll_index = mn34210pl_video_format_tbl.table[format_index].pll_index;
	mn34210pl_print_info(src);

	//set clk_si
	errCode |= mn34210pl_init_vin_clock(src, &mn34210pl_pll_tbl[pinfo->pll_index]);

	errCode |= mn34210pl_set_vin_mode(src);

	mn34210pl_fill_pll_regs(src);

	mn34210pl_fill_share_regs(src);

	mn34210pl_fill_video_format_regs(src);

	mn34210pl_set_fps(src, AMBA_VIDEO_FPS_AUTO);

	mn34210pl_set_shutter_time(src, AMBA_VIDEO_FPS_60);

	errCode |= mn34210pl_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

mn34210pl_set_mode_exit:
	return errCode;
}

static int mn34210pl_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errCode = -EINVAL;
	int i;
	struct mn34210pl_info *pinfo;
	int errorCode = 0;
	static int 				first_set_video_mode = 1;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct mn34210pl_info *) src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errorCode = mn34210pl_init_vin_clock(src, &mn34210pl_pll_tbl[pinfo->pll_index]);
		if (errorCode)
			goto mn34210pl_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	mn34210pl_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	for (i = 0; i < MN34210PL_VIDEO_MODE_TABLE_SIZE; i++) {
		if (mn34210pl_video_mode_table[i].mode == mode) {
			errCode = mn34210pl_set_video_index(src, mn34210pl_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}

	if (i >= MN34210PL_VIDEO_MODE_TABLE_SIZE) {
		vin_err("mn34210pl_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = mn34210pl_video_mode_table[i].mode;
		pinfo->mode_type = mn34210pl_video_mode_table[i].preview_mode_type;
	}

mn34210pl_set_video_mode_exit:
	return errCode;
}

static int mn34210pl_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	struct mn34210pl_info		*pinfo;
	//u8 			sensor_ver;

	pinfo = (struct mn34210pl_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = mn34210pl_init_vin_clock(src, &mn34210pl_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto mn34210pl_init_hw_exit;
#if 0
	errCode = mn34210pl_read_reg(src, 0x300E, &sensor_ver);/* FIXME, in fact, 0x300e is not version */
	if (errCode) {
		vin_err("Query sensor version failed!\n");
		errCode = -EIO;
		goto mn34210pl_init_hw_exit;
	}
	vin_info("MN34210PL sensor version is 0x%x\n", sensor_ver);
 #endif
	msleep(10);
	mn34210pl_reset(src);

mn34210pl_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int mn34210pl_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{
	return 0;
}

static int mn34210pl_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
	/* >> TODO, for RGB mode only << */
	return 0;
}

static int mn34210pl_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	struct mn34210pl_info *pinfo;
	u32 gain_index, format_index;
	s32 db_max;
	s32 db_step;

	vin_dbg("mn34210pl_set_agc_db: 0x%x\n", agc_db);

	pinfo = (struct mn34210pl_info *) src->pinfo;

	format_index = mn34210pl_video_info_table[pinfo->current_video_index].format_index;

	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	gain_index = (db_max - agc_db) / db_step;

	if (gain_index > MN34210PL_GAIN_0DB)
		gain_index = MN34210PL_GAIN_0DB;
	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		if (mn34210pl_video_format_tbl.table[format_index].bits == AMBA_VIDEO_BITS_10) {
			/*  for 10bits, if gain >=16.5dB, set 0x3097 and 0x3098 according to Pana FAE's suggestion */
			if(MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN] >= 0x1B0) {
				mn34210pl_write_reg(src, 0x3097,  0x10);
				mn34210pl_write_reg(src, 0x3098,  0x00);
			} else {
				mn34210pl_write_reg(src, 0x3097,  0xD1);
				mn34210pl_write_reg(src, 0x3098,  0x80);
			}
		} else {
			mn34210pl_write_reg(src, 0x3097,  0x00);
			mn34210pl_write_reg(src, 0x3098,  0x00);
		}

		/* AGAIN */
		mn34210pl_write_reg(src, 0x0204, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0205, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN]);
		/* DGAIN-Gr */
		mn34210pl_write_reg(src, 0x020E, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x020F, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-R */
		mn34210pl_write_reg(src, 0x0210, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0211, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-B */
		mn34210pl_write_reg(src, 0x0212, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0213, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-Gb */
		mn34210pl_write_reg(src, 0x0214, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0215, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);

		pinfo->current_gain_db = agc_db;

		return 0;
	} else {
		return -1;
	}

	return 0;
}

static int mn34210pl_set_agc_index(struct __amba_vin_source *src, u32 index)
{
	struct mn34210pl_info *pinfo;
	u32 gain_index, format_index;

	vin_dbg("mn34210pl_set_agc_index: %d\n", index);

	pinfo = (struct mn34210pl_info *) src->pinfo;

	format_index = mn34210pl_video_info_table[pinfo->current_video_index].format_index;

	gain_index = MN34210PL_GAIN_0DB - index;

	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		if (mn34210pl_video_format_tbl.table[format_index].bits == AMBA_VIDEO_BITS_10) {
			/*  for 10bits, if gain >=16.5dB, set 0x3097 and 0x3098 according to Pana FAE's suggestion */
			if(MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN] >= 0x1B0) {
				mn34210pl_write_reg(src, 0x3097,  0x10);
				mn34210pl_write_reg(src, 0x3098,  0x00);
			} else {
				mn34210pl_write_reg(src, 0x3097,  0xD1);
				mn34210pl_write_reg(src, 0x3098,  0x80);
			}
		} else {
			mn34210pl_write_reg(src, 0x3097,  0x00);
			mn34210pl_write_reg(src, 0x3098,  0x00);
		}

		/* AGAIN */
		mn34210pl_write_reg(src, 0x0204, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0205, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN]);
		/* DGAIN-Gr */
		mn34210pl_write_reg(src, 0x020E, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x020F, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-R */
		mn34210pl_write_reg(src, 0x0210, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0211, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-B */
		mn34210pl_write_reg(src, 0x0212, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0213, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-Gb */
		mn34210pl_write_reg(src, 0x0214, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0215, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);

		return 0;
	} else {
		return -1;
	}

	return 0;
}

static int mn34210pl_set_agc_index_sync(struct __amba_vin_source *src, u32 index)
{
	struct mn34210pl_info *pinfo;
	u32 gain_index, format_index;
	u32 vsync_irq;
	vin_dbg("mn34210pl_set_agc_index: %d\n", index);

	pinfo = (struct mn34210pl_info *) src->pinfo;

	format_index = mn34210pl_video_info_table[pinfo->current_video_index].format_index;

	gain_index = MN34210PL_GAIN_0DB - index;
	vsync_irq = vsync_irq_flag;
	pinfo->agc_call_cnt++;
	wait_event_interruptible(vsync_irq_wait, vsync_irq < vsync_irq_flag || pinfo->agc_call_cnt < 7);//FIXME
	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		if (mn34210pl_video_format_tbl.table[format_index].bits == AMBA_VIDEO_BITS_10) {
			/*  for 10bits, if gain >=16.5dB, set 0x3097 and 0x3098 according to Pana FAE's suggestion */
			if(MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN] >= 0x1B0) {
				mn34210pl_write_reg(src, 0x3097,  0x10);
				mn34210pl_write_reg(src, 0x3098,  0x00);
			} else {
				mn34210pl_write_reg(src, 0x3097,  0xD1);
				mn34210pl_write_reg(src, 0x3098,  0x80);
			}
		} else {
			mn34210pl_write_reg(src, 0x3097,  0x00);
			mn34210pl_write_reg(src, 0x3098,  0x00);
		}

		/* AGAIN */
		mn34210pl_write_reg(src, 0x0204, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0205, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN]);
		/* DGAIN-Gr */
		mn34210pl_write_reg(src, 0x020E, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x020F, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-R */
		mn34210pl_write_reg(src, 0x0210, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0211, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-B */
		mn34210pl_write_reg(src, 0x0212, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0213, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-Gb */
		mn34210pl_write_reg(src, 0x0214, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0215, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);

		return 0;
	} else {
		return -1;
	}

	return 0;
}

static int mn34210pl_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int					errCode = 0;
	u32					readmode;
	struct			mn34210pl_info			*pinfo;
	u32					target_bayer_pattern;
	u8					tmp_reg;

	pinfo = (struct mn34210pl_info *) src->pinfo;

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
		readmode = MN34210PL_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
		break;

	case AMBA_VIN_SRC_MIRROR_NONE:
		readmode = 0;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
	default:
		vin_err("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	errCode |= mn34210pl_read_reg(src, 0x0101, &tmp_reg);
	tmp_reg &= (~MN34210PL_V_FLIP);
	tmp_reg |= readmode;
	errCode |= mn34210pl_write_reg(src, 0x0101, tmp_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}

static int mn34210pl_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
	/* >> TODO, for YUV output mode only << */
	return errCode;
}

static int mn34210pl_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;
	/* >> TODO, for RGB raw output mode only << */
	/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}

static int mn34210pl_set_wdr_again(struct __amba_vin_source *src, s32 agc_db)
{
	struct mn34210pl_info *pinfo;
	u32 gain_index;
	s32 db_max;
	s32 db_step;
	s16 again;

	pinfo = (struct mn34210pl_info *) src->pinfo;
	db_max = pinfo->wdr_gain_info.db_max;
	db_step = pinfo->wdr_gain_info.db_step;

	gain_index = (db_max - agc_db) / db_step;

	if ((gain_index >= pinfo->min_wdr_gain_index) && (gain_index <= pinfo->max_wdr_gain_index)) {
		again = 0x100 + (MN34210PL_WDR_GAIN_30DB - gain_index);
		/* for 10bits, if gain >=16.5dB, set 0x3097 and 0x3098 according to Pana FAE's suggestion */
		if(again >= 0x1B0) {
			mn34210pl_write_reg(src, 0x3097,  0x10);
			mn34210pl_write_reg(src, 0x3098,  0x00);
		} else {
			mn34210pl_write_reg(src, 0x3097,  0xD1);
			mn34210pl_write_reg(src, 0x3098,  0x80);
		}
		/* AGAIN */
		mn34210pl_write_reg(src, 0x0204, (again >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0205, (u8)again);
	} else {
		return -1;
	}

	//DRV_PRINT("gain_index:%d, again:0x%x\n", gain_index, again);

	return 0;
}

static int mn34210pl_set_wdr_dgain_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_gain_gp_info *p_dgain_gp)
{
	struct mn34210pl_info *pinfo;
	u32 gain_index;
	s32 db_max;
	s32 db_step;
	s16 dgain;

	pinfo = (struct mn34210pl_info *) src->pinfo;
	db_max = pinfo->wdr_gain_info.db_max;
	db_step = pinfo->wdr_gain_info.db_step;

	/* long frame */
	gain_index = (db_max - p_dgain_gp->long_gain) / db_step;
	if ((gain_index >= pinfo->min_wdr_gain_index) && (gain_index <= pinfo->max_wdr_gain_index)) {
		dgain = 0x100 +  (MN34210PL_WDR_GAIN_30DB - gain_index);
		/* DGAIN-Gr */
		mn34210pl_write_reg(src, 0x020E, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x020F, (u8)dgain);
		/* DGAIN-R */
		mn34210pl_write_reg(src, 0x0210, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0211, (u8)dgain);
		/* DGAIN-B */
		mn34210pl_write_reg(src, 0x0212, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0213, (u8)dgain);
		/* DGAIN-Gb */
		mn34210pl_write_reg(src, 0x0214, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0215, (u8)dgain);
	} else {
		vin_err("long frame dgain exceeds limitation!\n");
		return -1;
	}

	/* short frame 1 */
	gain_index = (db_max - p_dgain_gp->short1_gain) / db_step;
	if ((gain_index >= pinfo->min_wdr_gain_index) && (gain_index <= pinfo->max_wdr_gain_index)) {
		dgain = 0x100 +  (MN34210PL_WDR_GAIN_30DB - gain_index);
		/* DGAIN-Gr */
		mn34210pl_write_reg(src, 0x310A, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x310B, (u8)dgain);
		/* DGAIN-R */
		mn34210pl_write_reg(src, 0x310C, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x310D, (u8)dgain);
		/* DGAIN-B */
		mn34210pl_write_reg(src, 0x310E, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x310F, (u8)dgain);
		/* DGAIN-Gb */
		mn34210pl_write_reg(src, 0x3110, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3111, (u8)dgain);
	} else {
		vin_err("short frame 1 dgain exceeds limitation!\n");
		return -1;
	}

	/* short frame 2 */
	gain_index = (db_max - p_dgain_gp->short2_gain) / db_step;
	if ((gain_index >= pinfo->min_wdr_gain_index) && (gain_index <= pinfo->max_wdr_gain_index)) {
		dgain = 0x100 +  (MN34210PL_WDR_GAIN_30DB - gain_index);
		/* DGAIN-Gr */
		mn34210pl_write_reg(src, 0x3112, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3113, (u8)dgain);
		/* DGAIN-R */
		mn34210pl_write_reg(src, 0x3114, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3115, (u8)dgain);
		/* DGAIN-B */
		mn34210pl_write_reg(src, 0x3116, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3117, (u8)dgain);
		/* DGAIN-Gb */
		mn34210pl_write_reg(src, 0x3118, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3119, (u8)dgain);
	} else {
		vin_err("short frame 2 dgain exceeds limitation!\n");
		return -1;
	}

	/* short frame 3 */
	gain_index = (db_max - p_dgain_gp->short3_gain) / db_step;
	if ((gain_index >= pinfo->min_wdr_gain_index) && (gain_index <= pinfo->max_wdr_gain_index)) {
		dgain = 0x100 +  (MN34210PL_WDR_GAIN_30DB - gain_index);
		/* DGAIN-Gr */
		mn34210pl_write_reg(src, 0x311A, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x311B, (u8)dgain);
		/* DGAIN-R */
		mn34210pl_write_reg(src, 0x311C, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x311D, (u8)dgain);
		/* DGAIN-B */
		mn34210pl_write_reg(src, 0x311E, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x311F, (u8)dgain);
		/* DGAIN-Gb */
		mn34210pl_write_reg(src, 0x3120, (dgain >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3121, (u8)dgain);
	} else {
		vin_err("short frame 3 dgain exceeds limitation!\n");
		return -1;
	}

	//DRV_PRINT("long dgain:%d, short1 dgain:%d, short2 dgain:%d, short3 dgain:%d\n",
	//	p_dgain_gp->long_gain, p_dgain_gp->short1_gain, p_dgain_gp->short2_gain, p_dgain_gp->short3_gain);

	return 0;
}

static int mn34210pl_set_wdr_again_index(struct __amba_vin_source *src, s32 agc_index)
{
	struct mn34210pl_info *pinfo;
	s32 gain_index;

	pinfo = (struct mn34210pl_info *) src->pinfo;

	if ((agc_index >= pinfo->min_wdr_gain_index) && (agc_index <= pinfo->max_wdr_gain_index)) {
		gain_index = MN34210PL_GAIN_0DB - agc_index;
		/* for 10bits, if gain >=16.5dB, set 0x3097 and 0x3098 according to Pana FAE's suggestion */
		if(MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN] >= 0x1B0) {
			mn34210pl_write_reg(src, 0x3097,  0x10);
			mn34210pl_write_reg(src, 0x3098,  0x00);
		} else {
			mn34210pl_write_reg(src, 0x3097,  0xD1);
			mn34210pl_write_reg(src, 0x3098,  0x80);
		}
		/* AGAIN */
		mn34210pl_write_reg(src, 0x0204, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0205, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_AGAIN]);
	} else {
		return -1;
	}

	return 0;
}

static int mn34210pl_set_wdr_dgain_index_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_gain_gp_info *p_dgain_gp)
{
	struct mn34210pl_info *pinfo;
	s32 gain_index;
	s32 dgain_index;

	pinfo = (struct mn34210pl_info *) src->pinfo;

	/* long frame */
	dgain_index = p_dgain_gp->long_gain;
	if ((dgain_index >= pinfo->min_wdr_gain_index) && (dgain_index <= pinfo->max_wdr_gain_index)) {
		gain_index = MN34210PL_WDR_GAIN_30DB - dgain_index;
		/* DGAIN-Gr */
		mn34210pl_write_reg(src, 0x020E, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x020F, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-R */
		mn34210pl_write_reg(src, 0x0210, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0211, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-B */
		mn34210pl_write_reg(src, 0x0212, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0213, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-Gb */
		mn34210pl_write_reg(src, 0x0214, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x0215, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	} else {
		vin_err("long frame dgain %d exceeds limitation!\n", dgain_index);
		return -1;
	}

	/* short frame 1 */
	dgain_index = p_dgain_gp->short1_gain;
	if ((dgain_index >= pinfo->min_wdr_gain_index) && (dgain_index <= pinfo->max_wdr_gain_index)) {
		gain_index = MN34210PL_WDR_GAIN_30DB - dgain_index;
		/* DGAIN-Gr */
		mn34210pl_write_reg(src, 0x310A, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x310B, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-R */
		mn34210pl_write_reg(src, 0x310C, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x310D, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-B */
		mn34210pl_write_reg(src, 0x310E, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x310F, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-Gb */
		mn34210pl_write_reg(src, 0x3110, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3111, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	} else {
		vin_err("short1 frame dgain %d exceeds limitation!\n", dgain_index);
		return -1;
	}

	/* short frame 2 */
	dgain_index = p_dgain_gp->short2_gain;
	if ((dgain_index >= pinfo->min_wdr_gain_index) && (dgain_index <= pinfo->max_wdr_gain_index)) {
		gain_index = MN34210PL_WDR_GAIN_30DB - dgain_index;
		/* DGAIN-Gr */
		mn34210pl_write_reg(src, 0x3112, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3113, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-R */
		mn34210pl_write_reg(src, 0x3114, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3115, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-B */
		mn34210pl_write_reg(src, 0x3116, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3117, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-Gb */
		mn34210pl_write_reg(src, 0x3118, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3119, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	} else {
		vin_err("short2 frame dgain %d exceeds limitation!\n", dgain_index);
		return -1;
	}

	/* short frame 3 */
	dgain_index = p_dgain_gp->short3_gain;
	if ((dgain_index >= pinfo->min_wdr_gain_index) && (dgain_index <= pinfo->max_wdr_gain_index)) {
		gain_index = MN34210PL_WDR_GAIN_30DB - dgain_index;
		/* DGAIN-Gr */
		mn34210pl_write_reg(src, 0x311A, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x311B, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-R */
		mn34210pl_write_reg(src, 0x311C, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x311D, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-B */
		mn34210pl_write_reg(src, 0x311E, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x311F, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
		/* DGAIN-Gb */
		mn34210pl_write_reg(src, 0x3120, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
		mn34210pl_write_reg(src, 0x3121, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	} else {
		vin_err("short3 frame dgain %d exceeds limitation!\n", dgain_index);
		return -1;
	}

	return 0;
}

static int mn34210pl_set_wdr_shutter_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_shutter_gp_info *p_shutter_gp)
{
	u64 exposure_time_q9;
	u8 data_val;
	u32 line_length, frame_length_lines, active_lines;
	u32 index, format_index;
	int shutter_long, shutter_short1, shutter_short2, shutter_short3;
	int errCode = 0;

	struct mn34210pl_info *pinfo;
	const struct mn34210pl_pll_reg_table *pll_reg_table;

	pinfo		= (struct mn34210pl_info *)src->pinfo;
	pinfo		= (struct mn34210pl_info *)src->pinfo;
	index = pinfo->current_video_index;
	format_index = mn34210pl_video_info_table[index].format_index;
	active_lines = mn34210pl_video_format_tbl.table[format_index].height;
	pll_reg_table	= &mn34210pl_pll_tbl[pinfo->pll_index];

	errCode |= mn34210pl_read_reg(src, 0x0343, &data_val);//HCYCLE_LSB
	if(errCode != 0)
		return errCode;
	line_length = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0342, &data_val);//HCYCLE_MSB
	if(errCode != 0)
		return errCode;
	line_length |= data_val<<8;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mn34210pl_read_reg(src, 0x0341, &data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0340, &data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= data_val<<8;

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

	/* short shutter 2 */
	exposure_time_q9 = p_shutter_gp->short2_shutter;
	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;
	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);
	shutter_short2 = (u32)exposure_time_q9;

	/* short shutter 3 */
	exposure_time_q9 = p_shutter_gp->short3_shutter;
	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;
	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);
	shutter_short3 = (u32)exposure_time_q9;

	/* shutter limitation check */
	switch(pinfo->op_mode){
		case MN34210PL_2X_WDR_MODE:
			if(shutter_long + shutter_short1 + 5 > frame_length_lines){
				vin_err("shutter exceeds limitation! long:%d, short:%d, V:%d\n",
					shutter_long, shutter_short1, frame_length_lines);
				return -1;
			}else if(shutter_short1 + 2 > frame_length_lines - active_lines) {
				vin_err("short frame offset exceeds limitation! short:%d, VB:%d\n",
					shutter_short1, frame_length_lines - active_lines);
			}
			break;
		case MN34210PL_3X_WDR_MODE:
			if(shutter_long + shutter_short1 + shutter_short2 + 7 > frame_length_lines){
				vin_err("shutter exceeds limitation! long:%d, short1:%d, short2:%d, V:%d\n",
					shutter_long, shutter_short1, shutter_short2, frame_length_lines);
				return -1;
			}else if(shutter_short1 + shutter_short2 + 4 > frame_length_lines - active_lines) {
				vin_err("short frame offset exceeds limitation! short1:%d, short2:%d, VB:%d\n",
					shutter_short1, shutter_short2, frame_length_lines - active_lines);
			}
			break;
		case MN34210PL_4X_WDR_MODE:
			if(shutter_long + shutter_short1 + shutter_short2 + 9 > frame_length_lines){
				vin_err("shutter exceeds limitation! long:%d, short1:%d, short2:%d, short3:%d, V:%d\n",
					shutter_long, shutter_short1, shutter_short2, shutter_short3, frame_length_lines);
				return -1;
			}else if(shutter_short1 + shutter_short2 + shutter_short3 + 6 > frame_length_lines - active_lines) {
				vin_err("short frame offset exceeds limitation! short1:%d, short2:%d, short3:%d, VB:%d\n",
					shutter_short1, shutter_short2, shutter_short3, frame_length_lines - active_lines);
			}
			break;
		case MN34210PL_LINEAR_MODE:
		default:
			vin_err("Unsupported mode\n");
			return -1;
			break;
	}

	/* long shutter */
	errCode |= mn34210pl_write_reg(src, 0x0203, (u8)(shutter_long & 0xFF));
	if(errCode != 0)
		return errCode;
	errCode |= mn34210pl_write_reg(src, 0x0202, (u8)(shutter_long >> 8));
	if(errCode != 0)
		return errCode;

	/* short shutter 1 */
	errCode |= mn34210pl_write_reg(src, 0x3127, (u8)(shutter_short1 & 0xFF));
	if(errCode != 0)
		return errCode;
	errCode |= mn34210pl_write_reg(src, 0x3126, (u8)(shutter_short1 >> 8));
	if(errCode != 0)
		return errCode;

	/* short shutter 2 */
	errCode |= mn34210pl_write_reg(src, 0x312B, (u8)(shutter_short2 & 0xFF));
	if(errCode != 0)
		return errCode;
	errCode |= mn34210pl_write_reg(src, 0x312A, (u8)(shutter_short2 >> 8));
	if(errCode != 0)
		return errCode;

	/* short shutter 3 */
	errCode |= mn34210pl_write_reg(src, 0x312F, (u8)(shutter_short3 & 0xFF));
	if(errCode != 0)
		return errCode;
	errCode |= mn34210pl_write_reg(src, 0x312E, (u8)(shutter_short3 >> 8));
	if(errCode != 0)
		return errCode;

	//DRV_PRINT("shutter long:%d, short1:%d, short2:%d, short3:%d\n", p_shutter_gp->long_shutter,
	//	p_shutter_gp->short1_shutter, p_shutter_gp->short2_shutter, p_shutter_gp->short3_shutter);

	return 0;
}

static int mn34210pl_set_wdr_shutter_row_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_shutter_gp_info *p_shutter_gp)
{
	u8 data_val;
	u32 frame_length_lines, active_lines;
	u32 index, format_index;
	int shutter_long, shutter_short1, shutter_short2, shutter_short3;
	int errCode = 0;

	struct mn34210pl_info *pinfo;

	pinfo = (struct mn34210pl_info *)src->pinfo;
	index = pinfo->current_video_index;
	format_index = mn34210pl_video_info_table[index].format_index;
	active_lines = mn34210pl_video_format_tbl.table[format_index].height;

	errCode |= mn34210pl_read_reg(src, 0x0341, &data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0340, &data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= data_val<<8;

	/* long shutter */
	shutter_long = p_shutter_gp->long_shutter;

	/* short shutter 1 */
	shutter_short1 = p_shutter_gp->short1_shutter;

	/* short shutter 2 */
	shutter_short2 = p_shutter_gp->short2_shutter;

	/* short shutter 3 */
	shutter_short3 = p_shutter_gp->short3_shutter;

	/* shutter limitation check */
	switch(pinfo->op_mode){
		case MN34210PL_2X_WDR_MODE:
			if(shutter_long + shutter_short1 + 5 > frame_length_lines){
				vin_err("shutter exceeds limitation! long:%d, short:%d, V:%d\n",
					shutter_long, shutter_short1, frame_length_lines);
				return -1;
			}else if(shutter_short1 + 2 > frame_length_lines - active_lines) {
				vin_err("short frame offset exceeds limitation! short:%d, VB:%d\n",
					shutter_short1, frame_length_lines - active_lines);
			}
			break;
		case MN34210PL_3X_WDR_MODE:
			if(shutter_long + shutter_short1 + shutter_short2 + 7 > frame_length_lines){
				vin_err("shutter exceeds limitation! long:%d, short1:%d, short2:%d, V:%d\n",
					shutter_long, shutter_short1, shutter_short2, frame_length_lines);
				return -1;
			}else if(shutter_short1 + shutter_short2 + 4 > frame_length_lines - active_lines) {
				vin_err("short frame offset exceeds limitation! short1:%d, short2:%d, VB:%d\n",
					shutter_short1, shutter_short2, frame_length_lines - active_lines);
			}
			break;
		case MN34210PL_4X_WDR_MODE:
			if(shutter_long + shutter_short1 + shutter_short2 + 9 > frame_length_lines){
				vin_err("shutter exceeds limitation! long:%d, short1:%d, short2:%d, short3:%d, V:%d\n",
					shutter_long, shutter_short1, shutter_short2, shutter_short3, frame_length_lines);
				return -1;
			}else if(shutter_short1 + shutter_short2 + shutter_short3 + 6 > frame_length_lines - active_lines) {
				vin_err("short frame offset exceeds limitation! short1:%d, short2:%d, short3:%d, VB:%d\n",
					shutter_short1, shutter_short2, shutter_short3, frame_length_lines - active_lines);
			}
			break;
		case MN34210PL_LINEAR_MODE:
		default:
			vin_err("Unsupported mode\n");
			return -1;
			break;
	}

	/* long shutter */
	errCode |= mn34210pl_write_reg(src, 0x0203, (u8)(shutter_long & 0xFF));
	if(errCode != 0)
		return errCode;
	errCode |= mn34210pl_write_reg(src, 0x0202, (u8)(shutter_long >> 8));
	if(errCode != 0)
		return errCode;

	/* short shutter 1 */
	errCode |= mn34210pl_write_reg(src, 0x3127, (u8)(shutter_short1 & 0xFF));
	if(errCode != 0)
		return errCode;
	errCode |= mn34210pl_write_reg(src, 0x3126, (u8)(shutter_short1 >> 8));
	if(errCode != 0)
		return errCode;

	/* short shutter 2 */
	errCode |= mn34210pl_write_reg(src, 0x312B, (u8)(shutter_short2 & 0xFF));
	if(errCode != 0)
		return errCode;
	errCode |= mn34210pl_write_reg(src, 0x312A, (u8)(shutter_short2 >> 8));
	if(errCode != 0)
		return errCode;

	/* short shutter 3 */
	errCode |= mn34210pl_write_reg(src, 0x312F, (u8)(shutter_short3 & 0xFF));
	if(errCode != 0)
		return errCode;
	errCode |= mn34210pl_write_reg(src, 0x312E, (u8)(shutter_short3 >> 8));
	if(errCode != 0)
		return errCode;

	//DRV_PRINT("shutter long:%d, short1:%d, short2:%d, short3:%d\n", p_shutter_gp->long_shutter,
	//	p_shutter_gp->short1_shutter, p_shutter_gp->short2_shutter, p_shutter_gp->short3_shutter);

	return 0;
}

static int mn34210pl_wdr_shutter2row(struct __amba_vin_source *src,
	struct amba_vin_wdr_shutter_gp_info *p_shutter2row)
{
	u64 exposure_time_q9;
	u8 data_val;
	u32 line_length, frame_length_lines;
	int errCode = 0;

	struct mn34210pl_info *pinfo;
	const struct mn34210pl_pll_reg_table *pll_reg_table;

	pinfo		= (struct mn34210pl_info *)src->pinfo;
	pll_reg_table	= &mn34210pl_pll_tbl[pinfo->pll_index];

	errCode |= mn34210pl_read_reg(src, 0x0343, &data_val);//HCYCLE_LSB
	if(errCode != 0)
		return errCode;
	line_length = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0342, &data_val);//HCYCLE_MSB
	if(errCode != 0)
		return errCode;
	line_length |= data_val<<8;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mn34210pl_read_reg(src, 0x0341, &data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = data_val;
	errCode |= mn34210pl_read_reg(src, 0x0340, &data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= data_val<<8;

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

	/* short shutter 3 */
	exposure_time_q9 = p_shutter2row->short3_shutter;
	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;
	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);
	p_shutter2row->short3_shutter = (u32)exposure_time_q9;

	return 0;
}

static int mn34210pl_set_operation_mode(struct __amba_vin_source *src, amba_vin_sensor_op_mode mode)
{
	int 			errCode = 0;
	struct mn34210pl_info	*pinfo = (struct mn34210pl_info *) src->pinfo;

	if((mode != MN34210PL_LINEAR_MODE) &&
		(mode != MN34210PL_2X_WDR_MODE) &&
			(mode != MN34210PL_3X_WDR_MODE) &&
				(mode != MN34210PL_4X_WDR_MODE)){
		vin_err("wrong opeartion mode, %d!\n", mode);
		errCode = -EPERM;
	} else {
		pinfo->op_mode = mode;
	}

	return errCode;
}

static int mn34210pl_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct mn34210pl_info		*pinfo;

	pinfo = (struct mn34210pl_info *) src->pinfo;

	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		mn34210pl_reset(src);
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
		errCode = mn34210pl_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO: {
		struct amba_vin_source_info *pub_src;

		pub_src = (struct amba_vin_source_info *) args;
		pub_src->id = src->id;
		pub_src->sensor_id = SENSOR_MN34210PL;
		pub_src->adapter_id = adapter_id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = mn34210pl_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = mn34210pl_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = mn34210pl_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = mn34210pl_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = mn34210pl_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = mn34210pl_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = mn34210pl_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = mn34210pl_set_still_mode(src, (struct amba_vin_src_still_info *) args);
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
		errCode = mn34210pl_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = mn34210pl_set_agc_db(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(s32*)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = mn34210pl_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = mn34210pl_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = mn34210pl_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = mn34210pl_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		mn34210pl_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID: {
		u16 sen_id = 0;
		u16 sen_ver = 0;
		u32 *pdata = (u32 *) args;

		errCode = mn34210pl_query_sensor_id(src, &sen_id);
		if (errCode)
			goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;
		errCode = mn34210pl_query_sensor_version(src, &sen_ver);
		if (errCode)
			goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;

		*pdata = (sen_id << 16) | sen_ver;
		}
exit_AMBA_VIN_SRC_TEST_GET_DEV_ID:
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA: {
		struct amba_vin_test_reg_data *reg_data;
		u16 subaddr;
		u8 data = 0;

		reg_data = (struct amba_vin_test_reg_data *) args;
		subaddr = reg_data->reg;

		errCode = mn34210pl_read_reg(src, subaddr, &data);

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

		errCode = mn34210pl_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = mn34210pl_set_slowshutter_mode(src, *(int *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_AGAIN:
		errCode = mn34210pl_set_wdr_again(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_DGAIN_GROUP:
		errCode = mn34210pl_set_wdr_dgain_group(src,
			(struct amba_vin_wdr_gain_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX:
		errCode = mn34210pl_set_wdr_again_index(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_DGAIN_INDEX_GROUP:
		errCode = mn34210pl_set_wdr_dgain_index_group(src,
			(struct amba_vin_wdr_gain_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_SHUTTER_GROUP:
		errCode = mn34210pl_set_wdr_shutter_group(src,
			(struct amba_vin_wdr_shutter_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_SHUTTER_ROW_GROUP:
		errCode = mn34210pl_set_wdr_shutter_row_group(src,
			(struct amba_vin_wdr_shutter_gp_info *) args);
		break;

	case AMBA_VIN_SRC_WDR_SHUTTER2ROW:
		errCode = mn34210pl_wdr_shutter2row(src,
			(struct amba_vin_wdr_shutter_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW:
		errCode = mn34210pl_set_shutter_time_row(src, *(u32 *) args);
		break;
	case AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW_SYNC:
		errCode = mn34210pl_set_shutter_time_row_sync(src, *(u32 *) args);
		break;
	case AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH:
		errCode=mn34210pl_shutter_time2width(src, (u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_INDEX:
		errCode = mn34210pl_set_agc_index(src, *(u32 *) args);
		break;
	case AMBA_VIN_SRC_SET_GAIN_INDEX_SYNC:
		errCode = mn34210pl_set_agc_index_sync(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_OPERATION_MODE:
		*(amba_vin_sensor_op_mode *)args = pinfo->op_mode;
		break;

	case AMBA_VIN_SRC_SET_OPERATION_MODE:
		errCode = mn34210pl_set_operation_mode(src, *(amba_vin_sensor_op_mode *)args);
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
