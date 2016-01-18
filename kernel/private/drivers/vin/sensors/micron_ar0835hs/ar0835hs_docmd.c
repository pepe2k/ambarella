/*
 * Filename : ar0835hs_docmd.c
 *
 * History:
 *    2012/12/26 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

static void ar0835hs_dump_reg(struct __amba_vin_source *src)
{
}

static void _ar0835_set_reg_0x301A(struct __amba_vin_source *src, u32 restart_frame, u32 streaming)
{
	u16 reg_0x301A;

	reg_0x301A =
		0 << 15                  |  // grouped parameter hold control
		1 << 14                  |  // gain values will always take effect in next frame indep of the integration time
		0 << 13                  |  // fast integration time update
		0 << 12                  |  // 1: disable serial interface (HiSPi)
		0 << 11                  |  // Set 1 can make PLL always be on
		0 << 10                  |  // DONT restart if bad frame is detected
		0 << 9                   |  // The sensor will produce bad frames as some register changed
		0 << 8                   |  // input buffer related to GPI0/1/2/3 inputs are powered down & disabled
		0 << 5                   |  // 0: disable signal to allow read from fuse ID registers
		1 << 4                   |  // reserved, set to 1 by default
		1 << 3                   |  // Forbids to change value of SMIA registers
		(streaming > 0) << 2     |  // Put the sensor in streaming mode
		(restart_frame > 0) << 1 |  // Causes sensor to truncate frame at the end of current row and start integrating next frame
		0 << 0;                     // Set the bit initiates a reset sequence

	// Typically, in normal streamming mode (restart_frame=0, streaming=1), the value is 0x401c
	ar0835hs_write_reg(src, 0x301a, reg_0x301A);
}

static void ar0835hs_fill_video_format_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 format_index;
	struct ar0835hs_info *pinfo;

	pinfo = (struct ar0835hs_info *) src->pinfo;

	vin_dbg("ar0835hs_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = ar0835hs_video_info_table[index].format_index;

	for (i = 0; i < AR0835HS_VIDEO_FORMAT_REG_NUM; i++) {
		if (ar0835hs_video_format_tbl.reg[i] == 0)
			break;

		ar0835hs_write_reg(src,
				ar0835hs_video_format_tbl.reg[i],
				ar0835hs_video_format_tbl.table[format_index].data[i]);
	}

	if (ar0835hs_video_format_tbl.table[format_index].ext_reg_fill)
		ar0835hs_video_format_tbl.table[format_index].ext_reg_fill(src);
}

static void ar0835hs_fill_share_regs(struct __amba_vin_source *src,
	const struct ar0835hs_reg_table *reg_tbl, int reg_tbl_size)
{
	int i;
	for (i = 0; i < reg_tbl_size; i++) {
		if(reg_tbl[i].reg == 0xffff) {
			msleep(reg_tbl[i].data);
		} else {
			ar0835hs_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
		}
	}
}

static void ar0835hs_fill_sequencer_v13p13(struct __amba_vin_source *src)
{
	int i;
	for (i = 0; i < AR0835HS_SEQUENCER_V13P13_SIZE; i++) {
		if(ar0835hs_sequencer_v13p13[i].reg == 0xffff) {
			msleep(ar0835hs_sequencer_v13p13[i].data);
		} else {
			ar0835hs_writeb_reg(src, ar0835hs_sequencer_v13p13[i].reg,
				(u8)ar0835hs_sequencer_v13p13[i].data);
		}
	}
}

static void ar0835hs_fill_setting_v7(struct __amba_vin_source *src)
{
	ar0835hs_fill_sequencer_v13p13(src);
	ar0835hs_fill_share_regs(src, ar0835hs_common_setting, AR0835HS_COMMON_SETTING_SIZE);
}

static void  ar0835hs_start_streaming(struct __amba_vin_source *src)
{
	_ar0835_set_reg_0x301A(src, 0/*restart_frame*/, 1/*streaming*/);
}

static void ar0835hs_sw_reset(struct __amba_vin_source *src)
{
	ar0835hs_write_reg(src, 0x301A, 0x0019);
	msleep(100);
}

static void ar0835hs_reset(struct __amba_vin_source *src)
{
	struct ar0835hs_info *pinfo;
	pinfo = (struct ar0835hs_info *) src->pinfo;

	AMBA_VIN_HW_RESET();
	ar0835hs_sw_reset(src);
	pinfo->init_flag = 1;
}

static int ar0835hs_get_vblank_time( struct __amba_vin_source *src,
	u32 *ptime)
{
	int				errCode		= 0;
	int				mode_index;
	u32				format_index;
	u64				v_btime;
	u32				active_lines, frame_length_lines;
	u32				line_length;
	u16				data_val;

	struct ar0835hs_info 			*pinfo;
	const struct ar0835hs_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct ar0835hs_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= ar0835hs_video_info_table[mode_index].format_index;
	pll_reg_table	= &ar0835hs_pll_tbl[pinfo->pll_index];

	errCode |= ar0835hs_read_reg(src, 0x300C, &data_val);
	line_length = (u32) data_val;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= ar0835hs_read_reg(src, 0x300A, &data_val);
	frame_length_lines = (u32) data_val;

	active_lines = ar0835hs_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pll_reg_table->pixclk); //ns
	*ptime = v_btime;

	vin_dbg("ar0835hs_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return errCode;
}

static void ar0835hs_fill_pll_regs(struct __amba_vin_source *src)
{
	int i;
	struct ar0835hs_info *pinfo;
	const struct ar0835hs_pll_reg_table 	*pll_reg_table;
	const struct ar0835hs_reg_table 		*pll_tbl;

	pinfo = (struct ar0835hs_info *) src->pinfo;

	vin_dbg("ar0835hs_fill_video_fps_regs\n");
	pll_reg_table = &ar0835hs_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < AR0835HS_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			ar0835hs_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
}

static int ar0835hs_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct ar0835hs_info  *pinfo;

	pinfo = (struct ar0835hs_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int ar0835hs_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errCode = 0;
	u32 index;
	u32 format_index;
	struct ar0835hs_info *pinfo;

	pinfo = (struct ar0835hs_info *) src->pinfo;

	index = pinfo->current_video_index;

	if (index >= AR0835HS_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = ar0835hs_video_info_table[index].format_index;

		p_video_info->width =ar0835hs_video_info_table[index].def_width;
		p_video_info->height = ar0835hs_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = ar0835hs_video_format_tbl.table[format_index].format;
		p_video_info->type = ar0835hs_video_format_tbl.table[format_index].type;
		p_video_info->bits = ar0835hs_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = ar0835hs_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

static int ar0835hs_get_agc_info( struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct ar0835hs_info *pinfo;

	pinfo = (struct ar0835hs_info *) src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}

static int ar0835hs_get_shutter_info( struct __amba_vin_source *src, amba_vin_shutter_info_t *pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));

	return 0;
}

static int ar0835hs_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < AR0835HS_VIDEO_MODE_TABLE_SIZE; i++) {
		if (ar0835hs_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = AR0835HS_VIDEO_MODE_TABLE_AUTO;
			p_mode_info->is_supported = 1;

			index = ar0835hs_video_mode_table[i].still_index;
			format_index = ar0835hs_video_info_table[index].format_index;

			p_mode_info->video_info.width = ar0835hs_video_info_table[index].def_width;
			p_mode_info->video_info.height = ar0835hs_video_info_table[index].def_height;
			p_mode_info->video_info.fps = ar0835hs_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = ar0835hs_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = ar0835hs_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = ar0835hs_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = ar0835hs_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;

			break;
		}
	}

	return errCode;
}

static int ar0835hs_query_sensor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	int errCode = 0;

	errCode |= ar0835hs_read_reg(src, 0x3000, ss_id);
	return errCode;
}

static int ar0835hs_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	int errCode = 0;
	u64 exposure_time_q9;
	u16 line_length;
	int num_time_h;
	u16 frame_lines;
	const struct ar0835hs_pll_reg_table *pll_table;
	struct ar0835hs_info * pinfo;

	pinfo = (struct ar0835hs_info *)src->pinfo;
	pll_table = &ar0835hs_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time;

	errCode |= ar0835hs_read_reg(src, 0x300C, &line_length);
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}
	errCode |= ar0835hs_read_reg(src, 0x300A, &frame_lines);

	/* t(frame)/t(per line) = t(exposure, in lines)*/
	exposure_time_q9 = exposure_time_q9 * (u64)pll_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	num_time_h = (u16)exposure_time_q9;

	if (unlikely((frame_lines) < (u16)exposure_time_q9)) {
		num_time_h = frame_lines;
		vin_dbg("warning: Exposure time is too long\n");
	}

	errCode |= ar0835hs_write_reg(src, 0x3012, num_time_h);

	pinfo->current_shutter_time = shutter_time;

	vin_dbg("shutter_width:%d\n", num_time_h);

	return errCode;
}

static int ar0835hs_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int	errCode = 0;
	u32	frame_time = 0;
	u64	frame_time_pclk;
	u16	line_length;
	u32	index;
	u32	format_index;
	u8	current_pll_index = 0;
	const struct ar0835hs_pll_reg_table *pll_reg_table;
	struct ar0835hs_info		*pinfo;
	struct amba_vin_irq_fix			vin_irq_fix;

	pinfo = (struct ar0835hs_info *)src->pinfo;
	index = pinfo->current_video_index;

	if (index >= AR0835HS_VIDEO_INFO_TABLE_SIZE) {
		vin_err("mt9p006_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto ar0835hs_set_fps_exit;
	}

	format_index = ar0835hs_video_info_table[index].format_index;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = ar0835hs_video_format_tbl.table[format_index].auto_fps;
	if(fps < ar0835hs_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, ar0835hs_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto ar0835hs_set_fps_exit;
	}

	frame_time = fps;

	/* ToDo: Add specified PLL index */
	current_pll_index = ar0835hs_video_format_tbl.table[format_index].pll_index;

	if(pinfo->pll_index != current_pll_index){
		pinfo->pll_index = current_pll_index;
		ar0835hs_fill_pll_regs(src);
		errCode = ar0835hs_init_vin_clock(src, &ar0835hs_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto ar0835hs_set_fps_exit;
	}

	pll_reg_table = &ar0835hs_pll_tbl[pinfo->pll_index];

	vin_dbg("ar0835hs_set_fps %d\n", frame_time);

	errCode = ar0835hs_read_reg(src, 0x300C, &line_length);
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	frame_time_pclk = frame_time * (u64)pll_reg_table->pixclk;

	DO_DIV_ROUND(frame_time_pclk, (u32)line_length);
	DO_DIV_ROUND(frame_time_pclk, 512000000);

	errCode |= ar0835hs_write_reg(src, 0x300a, (u16)frame_time_pclk);
	vin_dbg("frame_time %d, line_length 0x%x, vertical_lines 0x%x\n",
		frame_time, line_length, (u16)frame_time_pclk);

	ar0835hs_set_shutter_time(src, pinfo->current_shutter_time);// keep the same shutter time

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	errCode = ar0835hs_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode)
		goto ar0835hs_set_fps_exit;
	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

ar0835hs_set_fps_exit:
	return errCode;
}

static int ar0835hs_set_agc_db( struct __amba_vin_source *src, s32 agc_db)
{
	int errCode = 0;
	struct ar0835hs_info *pinfo;
	u32 gain_index;
	s32 db_max;
	s32 db_step;

	pinfo = (struct ar0835hs_info *) src->pinfo;

	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	gain_index = (db_max - agc_db) / db_step;

	if (gain_index >= AR0835HS_GAIN_ROWS) {
		vin_err("index of gain table out of range! %d\n", gain_index);
		errCode = -EINVAL;
		goto ar0835hs_set_agc_exit;
	}

	ar0835hs_write_reg(src, 0x305E, AR0835HS_GLOBAL_GAIN_TABLE[gain_index][AR0835_GAIN_COL_REG]);

	vin_dbg("db 0x%x, gain_index %d, regdata 0x%x\n",
		agc_db, gain_index, AR0835HS_GLOBAL_GAIN_TABLE[gain_index][AR0835_GAIN_COL_REG]);
	pinfo->current_gain_db = agc_db;

ar0835hs_set_agc_exit:
	return errCode;
}

static int ar0835hs_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct ar0835hs_info *pinfo;
	u32 format_index;

	pinfo = (struct ar0835hs_info *) src->pinfo;

	if (index >= AR0835HS_VIDEO_INFO_TABLE_SIZE) {
		vin_err("ar0835hs_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto ar0835hs_set_mode_exit;
	}

	errCode |= ar0835hs_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = ar0835hs_video_info_table[index].format_index;

	pinfo->cap_start_x = ar0835hs_video_info_table[index].def_start_x;
	pinfo->cap_start_y = ar0835hs_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = ar0835hs_video_info_table[index].def_width;
	pinfo->cap_cap_h = ar0835hs_video_info_table[index].def_height;
	pinfo->slvs_eav_col = ar0835hs_video_info_table[index].slvs_eav_col;
	pinfo->slvs_sav2sav_dist = ar0835hs_video_info_table[index].slvs_sav2sav_dist;
	pinfo->bayer_pattern = ar0835hs_video_info_table[index].bayer_pattern;
	pinfo->pll_index = ar0835hs_video_format_tbl.table[format_index].pll_index;

	ar0835hs_print_info(src);

	//set clk_si
	errCode |= ar0835hs_init_vin_clock(src, &ar0835hs_pll_tbl[pinfo->pll_index]);

	errCode |= ar0835hs_set_vin_mode(src);

	/* Only initialize after reset */
	if (pinfo->init_flag) {
		ar0835hs_fill_share_regs(src, ar0835hs_share_reg, AR0835HS_SHARE_REG_SIZE);
		ar0835hs_fill_pll_regs(src);
		ar0835hs_fill_video_format_regs(src);
		/* sequencer V7 */
		ar0835hs_fill_setting_v7(src);
		pinfo->init_flag = 0;
	} else {
		ar0835hs_fill_pll_regs(src);
		ar0835hs_fill_video_format_regs(src);
	}

	ar0835hs_set_fps(src, AMBA_VIDEO_FPS_AUTO);
	ar0835hs_set_agc_db(src, 0);

	errCode |= ar0835hs_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

	/* Enable Streaming */
	ar0835hs_start_streaming(src);

ar0835hs_set_mode_exit:
	return errCode;
}

static int ar0835hs_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errCode = -EINVAL;
	int i;
	struct ar0835hs_info *pinfo;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct ar0835hs_info *) src->pinfo;

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	for (i = 0; i < AR0835HS_VIDEO_MODE_TABLE_SIZE; i++) {
		if (ar0835hs_video_mode_table[i].mode == mode) {
			errCode = ar0835hs_set_video_index(src, ar0835hs_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= AR0835HS_VIDEO_MODE_TABLE_SIZE) {
		vin_err("ar0835hs_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = ar0835hs_video_mode_table[i].mode;
		pinfo->mode_type = ar0835hs_video_mode_table[i].preview_mode_type;
	}

	return errCode;
}

static int ar0835hs_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{
	return 0;
}

static int ar0835hs_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
		/* >> for RGB output sensor only << */
	return 0;
}

static int ar0835hs_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int errCode = 0;
	return errCode;
}

static int ar0835hs_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
		/* >> TODO, for YUV output sensor only << */
	return errCode;
}

static int ar0835hs_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	u16				sen_id = 0;
	struct ar0835hs_info	*pinfo;

	pinfo = (struct ar0835hs_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = ar0835hs_init_vin_clock(src, &ar0835hs_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto ar0835hs_init_hw_exit;
	msleep(10);
	ar0835hs_reset(src);

	errCode = ar0835hs_query_sensor_id(src, &sen_id);
	if (errCode)
		goto ar0835hs_init_hw_exit;

	vin_info("AR0835HS sensor ID is 0x%x\n", sen_id);

ar0835hs_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int ar0835hs_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct ar0835hs_info *pinfo;

	pinfo = (struct ar0835hs_info *) src->pinfo;

	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		ar0835hs_reset(src);
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
		errCode = ar0835hs_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO:
		{
			struct amba_vin_source_info *pub_src;
			pub_src = (struct amba_vin_source_info *) args;
			pub_src->id = src->id;
			pub_src->adapter_id = adapter_id;
			pub_src->dev_type = src->dev_type;
			strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
			pub_src->sensor_id = SENSOR_AR0835HS;
			pub_src->default_mode = AR0835HS_VIDEO_MODE_TABLE_AUTO;
			pub_src->total_channel_num = src->total_channel_num;
			pub_src->active_channel_id = src->active_channel_id;
			pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = ar0835hs_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = ar0835hs_get_video_info(src, (struct amba_video_info *) args);
		break;
	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = ar0835hs_get_agc_info(src, (amba_vin_agc_info_t *)args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = ar0835hs_get_shutter_info(src, (amba_vin_shutter_info_t *)args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = ar0835hs_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = ar0835hs_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = ar0835hs_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = ar0835hs_set_still_mode(src, (struct amba_vin_src_still_info *) args);
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
		errCode = ar0835hs_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(u32 *)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = ar0835hs_set_agc_db(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = ar0835hs_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = ar0835hs_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = ar0835hs_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = ar0835hs_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		ar0835hs_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		{
			u16 sen_id = 0;
			u32 *pdata = (u32 *) args;

			errCode = ar0835hs_query_sensor_id(src, &sen_id);
			if (errCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;

			*pdata = (sen_id << 16);
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

			errCode = ar0835hs_read_reg(src, subaddr, &data);

			reg_data->data = data;
		}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u16 subaddr;
			u16 data;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;
			data = reg_data->data;

			errCode = ar0835hs_write_reg(src, subaddr, data);
		}
		break;
	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;
	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;
	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		break;
	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}

