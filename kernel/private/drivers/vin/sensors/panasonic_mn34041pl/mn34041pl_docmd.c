/*
 * Filename : mn34041pl_docmd.c
 *
 * History:
 *    2011/01/12 - [Haowei Lo] Create
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

static void mn34041pl_dump_reg(struct __amba_vin_source *src)
{
	u32 i;
	u16 reg_to_dump_init[] = {};

	for(i=0; i<sizeof(reg_to_dump_init)/sizeof(reg_to_dump_init[0]); i++) {
		u16 reg_addr = reg_to_dump_init[i];
		u8 reg_val[2];

		mn34041pl_read_reg(src, reg_addr, reg_val);    // Read sensor revision number
		vin_dbg("REG= 0x%04X, 0x%04X, 0x%04X\n", reg_addr, reg_val[1], reg_val[0]);
	}

}

static void mn34041pl_fill_video_format_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 format_index;
	struct mn34041pl_info *pinfo;

	pinfo = (struct mn34041pl_info *) src->pinfo;

	vin_dbg("mn34041pl_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = mn34041pl_video_info_table[index].format_index;

	for (i = 0; i < MN34041PL_VIDEO_FORMAT_REG_NUM; i++) {
		if (mn34041pl_video_format_tbl.reg[i] == 0)
			break;

		mn34041pl_write_reg(src,
			mn34041pl_video_format_tbl.reg[i],
			mn34041pl_video_format_tbl.table[format_index].data[i]);
	}

	if (mn34041pl_video_format_tbl.table[format_index].ext_reg_fill)
		mn34041pl_video_format_tbl.table[format_index].ext_reg_fill(src);
}

static void mn34041pl_fill_share_regs(struct __amba_vin_source *src)
{
	int i, mn34041pl_share_regs_size;
	const struct mn34041pl_reg_table *reg_tbl;

	if(fps120_mode) {
		vin_info("120fps mode!\n");
		reg_tbl = mn34041pl_120fps_share_regs;
		mn34041pl_share_regs_size = MN34041PL_120FPS_SHARE_REG_SIZE;
	} else {
		reg_tbl = mn34041pl_share_regs;
		mn34041pl_share_regs_size = MN34041PL_SHARE_REG_SIZE;
	}
	for (i = 0; i < mn34041pl_share_regs_size; i++) {
		mn34041pl_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void mn34041pl_sw_reset(struct __amba_vin_source *src)
{
	mn34041pl_write_reg(src, 0x0000, 0x0000);	//TG reset release
}

static void mn34041pl_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	mn34041pl_sw_reset(src);
}

static void mn34041pl_fill_pll_regs(struct __amba_vin_source *src)
{

	int i;
	struct mn34041pl_info *pinfo;
	const struct mn34041pl_pll_reg_table 	*pll_reg_table;
	const struct mn34041pl_reg_table 		*pll_tbl;

	pinfo = (struct mn34041pl_info *) src->pinfo;

	vin_dbg("mn34041pl_fill_pll_regs\n");
	pll_reg_table = &mn34041pl_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < MN34041PL_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			mn34041pl_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
}

static int mn34041pl_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct mn34041pl_info  *pinfo;

	pinfo = (struct mn34041pl_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

/*
static int mn34041pl_get_vblank_time( struct __amba_vin_source *src,
	u32 *ptime)
{
	int				errCode = 0;
	int				mode_index;
	u32				format_index;
	u64				v_btime;
	u32				active_lines, frame_length_lines;
	u32				line_length;
	u8				data_val[2];

	struct mn34041pl_info 			*pinfo;
	const struct mn34041pl_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct mn34041pl_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= mn34041pl_video_info_table[mode_index].format_index;
	pll_reg_table	= &mn34041pl_pll_tbl[pinfo->pll_index];

	line_length = 2400;

	errCode |= mn34041pl_read_reg(src, 0x01A0, data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = (data_val[1]<<8) + data_val[0];
	errCode |= mn34041pl_read_reg(src, 0x01A1, data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= (data_val[0]&0x01)<<16;

	active_lines = mn34041pl_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pll_reg_table->pixclk); //ns
	*ptime = v_btime;

	vin_dbg("mn34041pl_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return errCode;
}
*/

static int mn34041pl_get_vb_lines( struct __amba_vin_source *src, u16 *vblank_lines)
{
	int				errCode = 0;
	int				mode_index;
	u32				format_index;
	u32				frame_length_lines;
	u8				data_val[2];

	struct mn34041pl_info 			*pinfo;

	pinfo		= (struct mn34041pl_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= mn34041pl_video_info_table[mode_index].format_index;

	errCode |= mn34041pl_read_reg(src, 0x01A0, data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = (data_val[1]<<8) + data_val[0];
	errCode |= mn34041pl_read_reg(src, 0x01A1, data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= ((data_val[0]&0x01)<<16) + 1;

	*vblank_lines = frame_length_lines - mn34041pl_video_format_tbl.table[format_index].height;

	return errCode;
}

static int mn34041pl_get_row_time( struct __amba_vin_source *src, u32 *row_time)
{
	int				errCode = 0;
	u64				h_time;
	u32				line_length;

	struct mn34041pl_info 			*pinfo;
	const struct mn34041pl_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct mn34041pl_info *)src->pinfo;
	pll_reg_table	= &mn34041pl_pll_tbl[pinfo->pll_index];

	if(fps120_mode) {
		line_length = 2160;
	} else {
		line_length = 2400;
	}
	h_time = (u64)line_length * 1000000000;
	DO_DIV_ROUND(h_time, (u64)pll_reg_table->pixclk); //ns

	*row_time = h_time;

	return errCode;
}

static int mn34041pl_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errCode = 0;
	u32 index;
	u32 format_index;

	struct mn34041pl_info *pinfo;

	pinfo = (struct mn34041pl_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= MN34041PL_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = mn34041pl_video_info_table[index].format_index;
		p_video_info->width = mn34041pl_video_info_table[index].def_width;
		p_video_info->height = mn34041pl_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = mn34041pl_video_format_tbl.table[format_index].format;
		p_video_info->type = mn34041pl_video_format_tbl.table[format_index].type;
		p_video_info->bits = mn34041pl_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = mn34041pl_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

static int mn34041pl_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct mn34041pl_info *pinfo;

	pinfo = (struct mn34041pl_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}

static int mn34041pl_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));
	return 0;
}

static int mn34041pl_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < MN34041PL_VIDEO_MODE_TABLE_SIZE; i++) {
		if (mn34041pl_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = MN34041PL_VIDEO_MODE_TABLE_AUTO;
			//FIX ME
			if(fps120_mode) {
				mn34041pl_video_mode_table[0].preview_index = 1;
				mn34041pl_video_mode_table[0].still_index = 1;
				mn34041pl_video_mode_table[1].preview_index = 1;
				mn34041pl_video_mode_table[1].still_index = 1;
			}

			p_mode_info->is_supported = 1;

			index = mn34041pl_video_mode_table[i].still_index;
			format_index = mn34041pl_video_info_table[index].format_index;

			p_mode_info->video_info.width = mn34041pl_video_info_table[index].def_width;
			p_mode_info->video_info.height = mn34041pl_video_info_table[index].def_height;
			p_mode_info->video_info.fps = mn34041pl_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = mn34041pl_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = mn34041pl_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = mn34041pl_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = mn34041pl_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;
			break;
		}
	}

	return errCode;
}

static int mn34041pl_query_sensor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	int 	errCode = 0;
	return errCode;
}

static int mn34041pl_query_sensor_version(struct __amba_vin_source *src, u16 * ver)
{
	/* >> TODO << */
	return 0;
}

static int mn34041pl_shutter_time2width(struct __amba_vin_source *src, u32* shutter_time)
{
	u64 exposure_time_q9;
	u8 data_val[2];
	u32 line_length, frame_length_lines;
	int shutter_width;
	int errCode = 0;

	struct mn34041pl_info *pinfo;
	const struct mn34041pl_pll_reg_table *pll_reg_table;

	pinfo		= (struct mn34041pl_info *)src->pinfo;
	pll_reg_table	= &mn34041pl_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = *shutter_time; 		// time*512*1000000

	if(fps120_mode) {
		line_length = 2160;
	} else {
		line_length = 2400;
	}

	errCode |= mn34041pl_read_reg(src, 0x01A0, data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = (data_val[1]<<8) + data_val[0];
	errCode |= mn34041pl_read_reg(src, 0x01A1, data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= (data_val[0]&0x01)<<16;

	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk - (line_length * 6 / 10);// a = 0.6
	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (int)exposure_time_q9;
	/* FIXME: shutter width: 1 ~(Frame format(V) - 3 + 0.6) */
	if(shutter_width < 1) {
		shutter_width = 1;
	} else if(shutter_width  > frame_length_lines - 3 + 1) {
		shutter_width = frame_length_lines - 3 + 1;
	}

	*shutter_time =shutter_width;

	return errCode;
}

static int mn34041pl_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	u64 exposure_time_q9;
	u8 data_val[2];
	u32 line_length, frame_length_lines;
	int shutter_reg;
	int errCode = 0;

	struct mn34041pl_info *pinfo;
	const struct mn34041pl_pll_reg_table *pll_reg_table;

	pinfo		= (struct mn34041pl_info *)src->pinfo;
	pll_reg_table	= &mn34041pl_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time; 		// time*512*1000000

	if(fps120_mode) {
		line_length = 2160;
	} else {
		line_length = 2400;
	}

	errCode |= mn34041pl_read_reg(src, 0x01A0, data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = (data_val[1]<<8) + data_val[0];
	errCode |= mn34041pl_read_reg(src, 0x01A1, data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= (data_val[0]&0x01)<<16;

	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk - (line_length * 6 / 10);// a = 0.6
	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_reg = frame_length_lines - (u32)exposure_time_q9 - 3;// reg(H) = 1(V)-3+0.6-exposure(H)
	vin_dbg("V:%d, shutter:%lld\n", frame_length_lines, exposure_time_q9);

	if(shutter_reg < 0) {
		shutter_reg = 0;
	}

	errCode |= mn34041pl_write_reg(src, 0x00A1, (u16)(shutter_reg & 0xFFFF) );
	if(errCode != 0)
		return errCode;

	errCode |= mn34041pl_write_reg(src, 0x00A2, (u16)((shutter_reg >> 16) & 0x1) );
	if(errCode != 0)
		return errCode;

	pinfo->current_shutter_time = exposure_time_q9;

	return errCode;
}

static int mn34041pl_set_shutter_time_row(struct __amba_vin_source *src, u32 row)
{
	u64 exposure_time_q9;
	u8 data_val[2];
	u32 line_length, frame_length_lines;
	int shutter_reg;
	int errCode = 0;

	struct mn34041pl_info *pinfo;
	const struct mn34041pl_pll_reg_table *pll_reg_table;

	pinfo		= (struct mn34041pl_info *)src->pinfo;
	pll_reg_table	= &mn34041pl_pll_tbl[pinfo->pll_index];

	if(fps120_mode) {
		line_length = 2160;
	} else {
		line_length = 2400;
	}

	errCode |= mn34041pl_read_reg(src, 0x01A0, data_val);//VCYCLE_LSB
	if(errCode != 0)
		return errCode;
	frame_length_lines = (data_val[1]<<8) + data_val[0];
	errCode |= mn34041pl_read_reg(src, 0x01A1, data_val);//VCYCLE_MSB
	if(errCode != 0)
		return errCode;
	frame_length_lines |= (data_val[0]&0x01)<<16;

	shutter_reg = frame_length_lines - row - 3;// reg(H) = 1(V)-3+0.6-exposure(H)
	vin_dbg("V:%d, shutter:%lld\n", frame_length_lines, exposure_time_q9);

	if(shutter_reg < 0) {
		shutter_reg = 0;
	}

	errCode |= mn34041pl_write_reg(src, 0x00A1, (u16)(shutter_reg & 0xFFFF) );
	if(errCode != 0)
		return errCode;

	errCode |= mn34041pl_write_reg(src, 0x00A2, (u16)((shutter_reg >> 16) & 0x1) );
	if(errCode != 0)
		return errCode;

	exposure_time_q9 = row;

	exposure_time_q9 = exposure_time_q9 * (u64)line_length * 512000000;
	DO_DIV_ROUND(exposure_time_q9, pll_reg_table->pixclk);

	pinfo->current_shutter_time = (u32)exposure_time_q9;

	vin_dbg("shutter_time:%d, row:%d\n", pinfo->current_shutter_time, row);

	return errCode;
}

static int mn34041pl_set_fps(struct __amba_vin_source *src, u32 fps)
{
	struct amba_vin_irq_fix		vin_irq_fix;
	struct mn34041pl_info 		*pinfo;
	u32 	index;
	u32 	format_index;
	u64		frame_time_pclk;
	u32  	frame_time = 0;
	u16 	line_length, reg_msb, reg_lsb;
	u8	current_pll_index = 0;
	const struct mn34041pl_pll_reg_table 	*pll_reg_table;
	int		errCode = 0;

	pinfo = (struct mn34041pl_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= MN34041PL_VIDEO_INFO_TABLE_SIZE) {
		vin_err("mn34041pl_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto mn34041pl_set_fps_exit;
	}

	/* ToDo: Add specified PLL index to lower fps */

	format_index = mn34041pl_video_info_table[index].format_index;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = mn34041pl_video_format_tbl.table[format_index].auto_fps;
	if(fps < mn34041pl_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, mn34041pl_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto mn34041pl_set_fps_exit;
	}

	frame_time = fps;

	/* FIXME: Add specified PLL index*/
	current_pll_index = mn34041pl_video_format_tbl.table[format_index].pll_index;

	if(fps > AMBA_VIDEO_FPS_30) {//fps < 30, use low power pll table
		if(!fps120_mode){
			current_pll_index = 2;
		}
	}

	if(pinfo->pll_index != current_pll_index){
		pinfo->pll_index = current_pll_index;
		mn34041pl_fill_pll_regs(src);
		errCode = mn34041pl_init_vin_clock(src, &mn34041pl_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto mn34041pl_set_fps_exit;
	}

	pll_reg_table = &mn34041pl_pll_tbl[pinfo->pll_index];

	vin_dbg("mn34041pl_set_fps %d\n", frame_time);

	if(fps120_mode) {
		line_length = 2160;
	} else {
		line_length = 2400;
	}

	frame_time_pclk =  frame_time * (u64)(pll_reg_table->pixclk);

	DO_DIV_ROUND(frame_time_pclk, (u32) line_length);
	DO_DIV_ROUND(frame_time_pclk, 512000000);
	reg_msb = (u16)(((frame_time_pclk - 1) >> 16) & 0x1);
	reg_lsb = (u16)frame_time_pclk - 1;

	/* mode 1 */
	mn34041pl_write_reg(src, 0x01A0, reg_lsb);
	mn34041pl_write_reg(src, 0x01A1, reg_msb);
	mn34041pl_write_reg(src, 0x01A3, reg_lsb);
	mn34041pl_write_reg(src, 0x01A4, reg_msb);
	mn34041pl_write_reg(src, 0x01A7, reg_lsb);
	mn34041pl_write_reg(src, 0x01A8, reg_msb);
	/* mode 2 */
	mn34041pl_write_reg(src, 0x02A0, reg_lsb);
	mn34041pl_write_reg(src, 0x02A1, reg_msb);
	mn34041pl_write_reg(src, 0x02A3, reg_lsb);
	mn34041pl_write_reg(src, 0x02A4, reg_msb);
	mn34041pl_write_reg(src, 0x02A7, reg_lsb);
	mn34041pl_write_reg(src, 0x02A8, reg_msb);

	mn34041pl_set_shutter_time(src, pinfo->current_shutter_time);//keep same exposure time

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_irq_fix.delay = 0;
	/*
	errCode = mn34041pl_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode) {
		goto mn34041pl_set_fps_exit;
	}
	*/
	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

mn34041pl_set_fps_exit:
	return errCode;
}

static int mn34041pl_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct mn34041pl_info *pinfo;
	u32 format_index;

	pinfo = (struct mn34041pl_info *) src->pinfo;

	if (index >= MN34041PL_VIDEO_INFO_TABLE_SIZE) {
		vin_err("mn34041pl_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto mn34041pl_set_mode_exit;
	}

	errCode |= mn34041pl_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = mn34041pl_video_info_table[index].format_index;

	pinfo->cap_start_x = mn34041pl_video_info_table[index].def_start_x;
	pinfo->cap_start_y = mn34041pl_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = mn34041pl_video_info_table[index].def_width;
	pinfo->cap_cap_h = mn34041pl_video_info_table[index].def_height;
	pinfo->slvs_eav_col = mn34041pl_video_info_table[index].slvs_eav_col;
	pinfo->slvs_sav2sav_dist = mn34041pl_video_info_table[index].slvs_sav2sav_dist;
	pinfo->bayer_pattern = mn34041pl_video_info_table[index].bayer_pattern;
	//set the specified PLL index for each sensor_mode.
	pinfo->pll_index = mn34041pl_video_format_tbl.table[format_index].pll_index;
	mn34041pl_print_info(src);

	//set clk_si
	errCode |= mn34041pl_init_vin_clock(src, &mn34041pl_pll_tbl[pinfo->pll_index]);

	errCode |= mn34041pl_set_vin_mode(src);

	mn34041pl_fill_pll_regs(src);

	mn34041pl_fill_share_regs(src);

	msleep(30);

	mn34041pl_fill_video_format_regs(src);

	mn34041pl_set_fps(src, AMBA_VIDEO_FPS_AUTO);

	mn34041pl_set_shutter_time(src, AMBA_VIDEO_FPS_120);

	errCode |= mn34041pl_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

mn34041pl_set_mode_exit:
	return errCode;
}

static int mn34041pl_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errCode = -EINVAL;
	int i;
	struct mn34041pl_info *pinfo;
	int errorCode = 0;
	static int 				first_set_video_mode = 1;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct mn34041pl_info *) src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errorCode = mn34041pl_init_vin_clock(src, &mn34041pl_pll_tbl[pinfo->pll_index]);
		if (errorCode)
			goto mn34041pl_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	mn34041pl_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;


	for (i = 0; i < MN34041PL_VIDEO_MODE_TABLE_SIZE; i++) {
		if (mn34041pl_video_mode_table[i].mode == mode) {
			errCode = mn34041pl_set_video_index(src, mn34041pl_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}

	if (i >= MN34041PL_VIDEO_MODE_TABLE_SIZE) {
		vin_err("mn34041pl_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = mn34041pl_video_mode_table[i].mode;
		pinfo->mode_type = mn34041pl_video_mode_table[i].preview_mode_type;
	}

mn34041pl_set_video_mode_exit:
	return errCode;
}

static int mn34041pl_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	struct mn34041pl_info		*pinfo;
	u16 			sensor_ver;

	pinfo = (struct mn34041pl_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = mn34041pl_init_vin_clock(src, &mn34041pl_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto mn34041pl_init_hw_exit;

	errCode = mn34041pl_read_reg(src, 0x0036, (u8 *) &sensor_ver);
	if (errCode || sensor_ver == 0xFFFF) {
		vin_err("Query sensor version failed, please check SPI communication!\n");
		errCode = -EIO;
		goto mn34041pl_init_hw_exit;
	}
	vin_info("MN34041PL sensor version is 0x%x\n", sensor_ver);

	msleep(10);
	mn34041pl_reset(src);

mn34041pl_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();

	return errCode;
}

static int mn34041pl_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{
	return 0;
}

static int mn34041pl_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
	/* >> TODO, for RGB mode only << */
	return 0;
}

static int mn34041pl_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	u16 idc_reg;
	struct mn34041pl_info *pinfo;
	u32 gain_index;
	s32 db_max;
	s32 db_step;

	vin_dbg("mn34041pl_set_agc_db: 0x%x\n", agc_db);

	pinfo = (struct mn34041pl_info *) src->pinfo;
	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	gain_index = (db_max - agc_db) / db_step;

	if (gain_index > MN34041PL_GAIN_0DB)
		gain_index = MN34041PL_GAIN_0DB;
	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		idc_reg = MN34041PL_GAIN_TABLE[gain_index][MN34041PL_GAIN_COL_REG_CGAIN]
			+ MN34041PL_GAIN_TABLE[gain_index][MN34041PL_GAIN_COL_REG_AGAIN];
		mn34041pl_write_reg(src, 0x0020, idc_reg);

		idc_reg = MN34041PL_GAIN_TABLE[gain_index][MN34041PL_GAIN_COL_REG_DGAIN];
		mn34041pl_write_reg(src, 0x0021, idc_reg);

		pinfo->current_gain_db = agc_db;

		return 0;
	} else {
		return -1;
	}

	return 0;
}
static int mn34041pl_set_agc_index(struct __amba_vin_source *src, u32 index)
{
	u16 idc_reg;
	struct mn34041pl_info *pinfo;
	u32 gain_index;

	vin_dbg("mn34041pl_set_agc_index: %d\n", index);

	pinfo = (struct mn34041pl_info *) src->pinfo;

	gain_index = MN34041PL_GAIN_0DB - index;

	if (gain_index > MN34041PL_GAIN_0DB)
		gain_index = MN34041PL_GAIN_0DB;
	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		idc_reg = MN34041PL_GAIN_TABLE[gain_index][MN34041PL_GAIN_COL_REG_CGAIN]
			+ MN34041PL_GAIN_TABLE[gain_index][MN34041PL_GAIN_COL_REG_AGAIN];
		mn34041pl_write_reg(src, 0x0020, idc_reg);

		idc_reg = MN34041PL_GAIN_TABLE[gain_index][MN34041PL_GAIN_COL_REG_DGAIN];
		mn34041pl_write_reg(src, 0x0021, idc_reg);

		return 0;
	} else {
		return -1;
	}

	return 0;
}
static int mn34041pl_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int					errCode = 0;
	u32					readmode;
	struct			mn34041pl_info			*pinfo;
	u32					target_bayer_pattern;
	u16					tmp_reg;

	pinfo = (struct mn34041pl_info *) src->pinfo;

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
		readmode = MN34041PL_MIRROR_FLIP;
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

	errCode |= mn34041pl_read_reg(src, 0x00A0, (u8 *)&tmp_reg);
	tmp_reg &= (~MN34041PL_MIRROR_FLIP);
	tmp_reg |= readmode;
	errCode |= mn34041pl_write_reg(src, 0x00A0, tmp_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}

static int mn34041pl_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
	/* >> TODO, for YUV output mode only << */
	return errCode;
}

static int mn34041pl_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;
	/* >> TODO, for RGB raw output mode only << */
	/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}

static int mn34041pl_get_eis_info(struct __amba_vin_source *src, struct amba_vin_eis_info *eis_info)
{
	int errCode = 0;
	u32 index = -1;
	u32 format_index;
	//struct mn34041pl_info *pinfo;

	//pinfo = (struct mn34041pl_info *) src->pinfo;
	index = pinfo->current_video_index;
	if(index == -1){
		errCode = -EPERM;
		goto mn34041pl_get_eis_info_exit;
	}
	format_index = mn34041pl_video_info_table[index].format_index;

	memset(eis_info, 0, sizeof (struct amba_vin_eis_info));

	eis_info->cap_start_x = pinfo->cap_start_x;
	eis_info->cap_start_y = pinfo->cap_start_y;
	eis_info->cap_cap_w = pinfo->cap_cap_w;
	eis_info->cap_cap_h = pinfo->cap_cap_h;
	eis_info->source_width = mn34041pl_video_format_tbl.table[format_index].width;
	eis_info->source_height = mn34041pl_video_format_tbl.table[format_index].height;
	eis_info->current_fps = pinfo->frame_rate;
	eis_info->main_fps = mn34041pl_video_format_tbl.table[format_index].auto_fps;
	eis_info->current_shutter_time = pinfo->current_shutter_time;
	eis_info->sensor_cell_width = 275;// 2.75 um
	eis_info->sensor_cell_height = 275;// 2.75 um
	eis_info->column_bin = 1;
	eis_info->row_bin = 1;

	//mn34041pl_get_vb_lines(src, &(eis_info->vb_lines));
	//mn34041pl_get_row_time(src, &(eis_info->row_time));
	mn34041pl_get_vb_lines(&(pinfo->source), &(eis_info->vb_lines));
	mn34041pl_get_row_time(&(pinfo->source), &(eis_info->row_time));

mn34041pl_get_eis_info_exit:
	return errCode;
}

int mn34041pl_get_eis_info_ex(struct amba_vin_eis_info * eis_info)
{
	int errCode = 0;
	errCode = mn34041pl_get_eis_info(NULL, eis_info);
	return errCode;
}
EXPORT_SYMBOL(mn34041pl_get_eis_info_ex);

static int mn34041pl_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct mn34041pl_info		*pinfo;

	pinfo = (struct mn34041pl_info *) src->pinfo;

	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		mn34041pl_reset(src);
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
		errCode = mn34041pl_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO: {
		struct amba_vin_source_info *pub_src;

		pub_src = (struct amba_vin_source_info *) args;
		pub_src->id = src->id;
		pub_src->sensor_id = SENSOR_MN34041PL;
		pub_src->adapter_id = adapter_id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = mn34041pl_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = mn34041pl_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = mn34041pl_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = mn34041pl_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = mn34041pl_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = mn34041pl_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = mn34041pl_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = mn34041pl_set_still_mode(src, (struct amba_vin_src_still_info *) args);
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
		errCode = mn34041pl_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = mn34041pl_set_agc_db(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(s32*)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = mn34041pl_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = mn34041pl_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = mn34041pl_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = mn34041pl_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		mn34041pl_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID: {
		u16 sen_id = 0;
		u16 sen_ver = 0;
		u32 *pdata = (u32 *) args;

		errCode = mn34041pl_query_sensor_id(src, &sen_id);
		if (errCode)
			goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;
		errCode = mn34041pl_query_sensor_version(src, &sen_ver);
		if (errCode)
			goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;

		*pdata = (sen_id << 16) | sen_ver;
		}
exit_AMBA_VIN_SRC_TEST_GET_DEV_ID:
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA: {
		struct amba_vin_test_reg_data *reg_data;
		u16 subaddr;
		u8 data[2];

		reg_data = (struct amba_vin_test_reg_data *) args;
		subaddr = reg_data->reg;

		errCode = mn34041pl_read_reg(src, subaddr, data);

		reg_data->data = (data[1]<<8) + data[0];
		}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA: {
		struct amba_vin_test_reg_data *reg_data;
		u16 subaddr;
		u16 data = 0;

		reg_data = (struct amba_vin_test_reg_data *) args;
		subaddr = reg_data->reg;
		data = reg_data->data;

		errCode = mn34041pl_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = mn34041pl_set_slowshutter_mode(src, *(int *) args);
		break;

	case AMBA_VIN_SRC_GET_EIS_INFO:
		errCode = mn34041pl_get_eis_info(src, (struct amba_vin_eis_info *) args);
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW:
		errCode = mn34041pl_set_shutter_time_row(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_INDEX:
		errCode = mn34041pl_set_agc_index(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH:
		errCode=mn34041pl_shutter_time2width(src, (u32 *) args);
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
