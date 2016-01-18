/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx226/imx226_docmd.c
 *
 * History:
 *    2013/12/20 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

static void imx226_fill_video_format_regs(struct __amba_vin_source *src)
{
	int			i;
	u32			index;
	u32			format_index;
	struct imx226_info	*pinfo;

	pinfo = (struct imx226_info *)src->pinfo;

	index = pinfo->current_video_index;
	format_index = imx226_video_info_table[index].format_index;

	for (i = 0; i < IMX226_VIDEO_FORMAT_REG_NUM; i++) {
		if (imx226_video_format_tbl.reg[i] == 0)
			break;
		imx226_write_reg(src,
			imx226_video_format_tbl.reg[i],
			imx226_video_format_tbl.table[format_index].data[i]);
	}

	if (imx226_video_format_tbl.table[format_index].ext_reg_fill)
		imx226_video_format_tbl.table[format_index].ext_reg_fill(src);
}

static void imx226_fill_share_regs(struct __amba_vin_source *src)
{
	int				i;
	const struct imx226_reg_table	*reg_tbl;

	reg_tbl = imx226_share_regs;
	for (i = 0; i < IMX226_SHARE_REG_SIZE; i++) {
		imx226_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void imx226_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
}

static int imx226_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct imx226_info  *pinfo;

	pinfo = (struct imx226_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int imx226_get_video_info( struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int	errCode = 0;
	u32	index;
	u32	format_index;
	struct imx226_info *pinfo;

	pinfo = (struct imx226_info *)src->pinfo;

	index = pinfo->current_video_index;

	if (index >= IMX226_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = imx226_video_info_table[index].format_index;

		p_video_info->width = imx226_video_info_table[index].def_width;
		p_video_info->height = imx226_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = imx226_video_format_tbl.table[format_index].format;
		p_video_info->type = imx226_video_format_tbl.table[format_index].type;
		p_video_info->bits = imx226_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = imx226_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

static int imx226_get_agc_info( struct __amba_vin_source *src, amba_vin_agc_info_t *p_agc_info)
{
	int errCode = 0;
	struct imx226_info *pinfo;

	pinfo = (struct imx226_info *) src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}

static int imx226_get_shutter_info( struct __amba_vin_source *src, amba_vin_shutter_info_t *pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));

	return 0;
}

static int imx226_check_video_mode( struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int				errCode = 0;
	int				i;
	u32				index;
	u32				format_index;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof(p_mode_info->video_info));

	for (i = 0; i < IMX226_VIDEO_MODE_TABLE_SIZE; i++) {
		if (imx226_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = IMX226_VIDEO_MODE_TABLE_AUTO;
			p_mode_info->is_supported = 1;

			index = imx226_video_mode_table[i].still_index;
			format_index = imx226_video_info_table[index].format_index;

			p_mode_info->video_info.width = imx226_video_info_table[index].def_width;
			p_mode_info->video_info.height = imx226_video_info_table[index].def_height;
			p_mode_info->video_info.fps = imx226_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = imx226_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = imx226_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = imx226_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = imx226_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;

			break;
		}
	}

	return errCode;
}

static int imx226_set_still_mode( struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{
	return 0;
}

static int imx226_set_low_light_agc( struct __amba_vin_source *src, u32 agc_index)
{
	return 0;
}

static int imx226_shutter_time2row( struct __amba_vin_source *src, u32* shutter_time)
{
	u64 exposure_time_q9;
	u32 xhs_time,  offset;
	u8 format_index;
	struct imx226_info		*pinfo;

	pinfo = (struct imx226_info *)src->pinfo;

	format_index = imx226_video_info_table[pinfo->current_video_index].format_index;
	if (format_index == 0 || format_index == 2 || format_index == 3)
		offset = 201;
	else
		offset = 150;

	xhs_time = imx226_video_format_tbl.table[format_index].xhs_clk;/* number of INCK per XHS */
	if(xhs_time == 0) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	exposure_time_q9 = *shutter_time * (u64)pinfo->pll_table.extclk;/* convert to INCK */

	DO_DIV_ROUND(exposure_time_q9, 512000000);/* XHS */
	exposure_time_q9 -= offset;
	DO_DIV_ROUND(exposure_time_q9, xhs_time);/* convert to XHS */

	*shutter_time =exposure_time_q9;

	return 0;
}

static int imx226_set_shutter_time( struct __amba_vin_source *src, u32 shutter_time)
{
	u32 exposure_time_row, xvs_time_h, num_time_h;
	u8 	shr_width_m, shr_width_l, svr, format_index;
	struct imx226_info		*pinfo;

	exposure_time_row = shutter_time;
	imx226_shutter_time2row(src, &exposure_time_row);

	pinfo = (struct imx226_info *)src->pinfo;
	format_index = imx226_video_info_table[pinfo->current_video_index].format_index;
	xvs_time_h = pinfo->xvs_num[format_index];/* number of XHS per frame */

	svr = (exposure_time_row > xvs_time_h)?(exposure_time_row/xvs_time_h):0;
	pinfo->svr_value = svr;
	xvs_time_h *= svr + 1;

	num_time_h = xvs_time_h - exposure_time_row;

	/* the valid range of SHR is 8 ~ (the number of XHS per frame - 4)*/
	if ((format_index == 5) && (num_time_h < 10)){/* binning mode SHR is 10~xvs_time_h - 4 */
		num_time_h = 10;
	} else if (num_time_h < 8){
		num_time_h = 8;
	}
	if (num_time_h > (xvs_time_h - 4)){
		num_time_h = (xvs_time_h - 4);
	}

	shr_width_m = num_time_h >> 8;
	shr_width_l = num_time_h & 0xff;

	imx226_write_reg(src, IMX226_REG_SVR_LSB, svr);
	imx226_write_reg(src, IMX226_REG_SHR_LSB, shr_width_l);
	imx226_write_reg(src, IMX226_REG_SHR_MSB, shr_width_m);

	vin_dbg("shutter=%d, SHR=0x%x%x\n", shutter_time, shr_width_m, shr_width_l);
	pinfo->current_shutter_time = shutter_time;
	return 0;
}

static int imx226_set_shutter_row(struct __amba_vin_source *src, u32 row)
{
	u64 exposure_time_q9;
	u32 xvs_time_h, xhs_time, num_time_h, offset;
	u8 shr_width_m, shr_width_l, format_index, svr;
	struct imx226_info		*pinfo;

	pinfo = (struct imx226_info *)src->pinfo;
	format_index = imx226_video_info_table[pinfo->current_video_index].format_index;
	xvs_time_h = pinfo->xvs_num[format_index];/* number of XHS per frame */

	svr = row/xvs_time_h;
	pinfo->svr_value = svr;
	xvs_time_h *= svr + 1;

	num_time_h = xvs_time_h - row;

	/* the valid range of SHR is 8 ~ (the number of XHS per frame - 4)*/
	if ((format_index == 5) && (num_time_h < 10)){/* binning mode SHR is 10~xvs_time_h - 4 */
		num_time_h = 10;
	} else if (num_time_h < 8){
		num_time_h = 8;
	}
	if (num_time_h > (xvs_time_h - 4)){
		num_time_h = (xvs_time_h - 4);
	}

	shr_width_m = num_time_h >> 8;
	shr_width_l = num_time_h & 0xff;

	imx226_write_reg(src, IMX226_REG_SVR_LSB, svr);
	imx226_write_reg(src, IMX226_REG_SHR_LSB, shr_width_l);
	imx226_write_reg(src, IMX226_REG_SHR_MSB, shr_width_m);

	xhs_time = imx226_video_format_tbl.table[format_index].xhs_clk;/* number of INCK per XHS */
	if(xhs_time == 0) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	if (format_index == 0 || format_index == 2 || format_index == 3)
		offset = 201;
	else
		offset = 150;

	exposure_time_q9 = ((row * (u64)xhs_time) + offset) * 512000000;
	DO_DIV_ROUND(exposure_time_q9, pinfo->pll_table.extclk);

	vin_dbg("V:%d, shutter_row=%d, shutter_time=%d\n", xvs_time_h, row, (u32)exposure_time_q9);
	pinfo->current_shutter_time = (u32)exposure_time_q9;

	return 0;
}

static int imx226_set_agc_db( struct __amba_vin_source *src, s32 agc_db)
{
	int errCode = 0;
	s32 index, db_max, db_step;
	u16 gain_index, pgc_val, dgain_val, apgc01_val = 0, apgc02_val = 0;
	struct imx226_info *pinfo;

	pinfo = (struct imx226_info *) src->pinfo;
	index = pinfo->current_video_index;

	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;
	gain_index = (db_max - agc_db) / db_step;

	if (gain_index > IMX226_GAIN_0DB)
		gain_index = IMX226_GAIN_0DB;

	pgc_val = IMX226_GAIN_TABLE[gain_index][IMX226_GAIN_COL_AGC];
	dgain_val = IMX226_GAIN_TABLE[gain_index][IMX226_GAIN_COL_DGAIN];

	switch(imx226_video_info_table[index].format_index) {
	case 0:
	case 2:
	case 3:
		apgc01_val = 0x003c;
		apgc02_val = 0x003b;
		break;
	case 1:
	case 4:
	case 5:
	case 6:
		if (pgc_val <= 0x146) {
			apgc01_val = 0x001e;
			apgc02_val = 0x001d;
		} else if (pgc_val >= 0x147 && pgc_val <= 0x400) {
			apgc01_val = 0x0020;
			apgc02_val = 0x001f;
		} else if (pgc_val >= 0x401 && pgc_val <= 0x52c) {
			apgc01_val = 0x0025;
			apgc02_val = 0x0024;
		} else if (pgc_val >= 0x52d && pgc_val <= 0x696) {
			apgc01_val = 0x0027;
			apgc02_val = 0x0026;
		} else if (pgc_val >= 0x697 && pgc_val <= 0x74b) {
			apgc01_val = 0x002b;
			apgc02_val = 0x002a;
		} else if (pgc_val >= 0x74c && pgc_val <= 0x7a5) {
			apgc01_val = 0x002f;
			apgc02_val = 0x002e;
		}
		break;
	default:
		BUG();
		break;
	}

	imx226_write_reg(src, IMX226_REG_01, 0x10); //DCKRST:0, CLPSQRST:1
	imx226_write_reg(src, IMX226_REG_DGAIN, dgain_val);
	imx226_write_reg2(src, IMX226_REG_PGC_LSB, pgc_val);
	imx226_write_reg2(src, IMX226_REG_APGC01_LSB, apgc01_val);
	imx226_write_reg2(src, IMX226_REG_APGC02_LSB, apgc02_val);

	if(errCode == 0)
		pinfo->current_gain_db = agc_db;

	return errCode;
}

static int imx226_set_shutter_and_agc( struct __amba_vin_source *src, u32 shutter_time, s32 agc_db, u32 update_flag)
{
	s32 vsync_irq;
	struct imx226_info		*pinfo;

	pinfo = (struct imx226_info *)src->pinfo;

	if (update_flag & IMX226_SET_SHUTTER) {
		if (update_flag & IMX226_UPDATE_SYNC) {
			vsync_irq = vsync_irq_flag;
			pinfo->agc_call_cnt++;
			wait_event_interruptible(vsync_irq_wait, vsync_irq < vsync_irq_flag || pinfo->agc_call_cnt < 7);//FIXME
		}
		imx226_set_shutter_time(src, shutter_time);
	}

	if (update_flag & IMX226_SET_AGC) {
		if (update_flag & IMX226_UPDATE_SYNC) {
			vsync_irq = vsync_irq_flag;
			pinfo->agc_call_cnt++;
			wait_event_interruptible(vsync_irq_wait, vsync_irq < vsync_irq_flag || pinfo->agc_call_cnt < 7);//FIXME
		}
		imx226_set_agc_db(src, agc_db);
	}

	return 0;
}

static int imx226_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int errCode = 0;
	u32 readmode;
	struct imx226_info *pinfo;
	u32 target_bayer_pattern;
	u8 tmp_reg;

	pinfo = (struct imx226_info *) src->pinfo;

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
		readmode = IMX226_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	case AMBA_VIN_SRC_MIRROR_NONE:
		readmode = 0;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
	default:
		vin_err("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	errCode |= imx226_read_reg(src, IMX226_REG_MDVREV, &tmp_reg);
	tmp_reg &= ~IMX226_V_FLIP;
	tmp_reg |= readmode;
	errCode |= imx226_write_reg(src, IMX226_REG_MDVREV, tmp_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}

/*
static int imx226_get_vblank_time( struct __amba_vin_source *src, u32 *ptime)
{
	u32					frame_length_lines, active_lines, format_index;
	u64					v_btime;
	u32					line_length;
	int					mode_index;

	struct imx226_info 			*pinfo;

	pinfo		= (struct imx226_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= imx226_video_info_table[mode_index].format_index;

	frame_length_lines = pinfo->xvs_num[format_index];
	line_length = imx226_video_format_tbl.table[format_index].xhs_clk
		* imx226_pll_tbl[pinfo->pll_index].factor;

	active_lines = imx226_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pinfo->pll_table.pixclk); //ns
	*ptime = v_btime;

	vin_dbg("imx226_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return 0;
}
*/

static int imx226_set_fps( struct __amba_vin_source *src, int fps)
{
	int errCode = 0;
	u32 index;
	u32 format_index;
	u64 vertical_lines;
	u8	current_pll_index = 0;

	struct imx226_info *pinfo;
	struct amba_vin_irq_fix		vin_irq_fix;
	struct amba_vin_HV	master_HV;

	pinfo = (struct imx226_info *)src->pinfo;
	index = pinfo->current_video_index;

	if (index >= IMX226_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx226_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto imx226_set_fps_exit;
	}

	format_index = imx226_video_info_table[index].format_index;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = imx226_video_format_tbl.table[format_index].auto_fps;
	if(fps < imx226_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, imx226_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto imx226_set_fps_exit;
	}

	current_pll_index = imx226_video_format_tbl.table[format_index].pll_index;

	if(pinfo->pll_index != current_pll_index){
		pinfo->pll_index = current_pll_index;
		errCode = imx226_init_vin_clock(src, &imx226_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto imx226_set_fps_exit;
	}

	pinfo->pll_table.pixclk = imx226_pll_tbl[pinfo->pll_index].pixclk;
	pinfo->pll_table.extclk = imx226_pll_tbl[pinfo->pll_index].extclk;

	/* Slave sensor, adjust VB time from DSP */
	master_HV.pel_clk_a_line = (imx226_video_format_tbl.table[format_index].xhs_clk
		* imx226_pll_tbl[pinfo->pll_index].factor) - 1;
	if(master_HV.pel_clk_a_line == 0) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	vertical_lines = (u64)imx226_pll_tbl[pinfo->pll_index].pixclk * fps;

	DO_DIV_ROUND(vertical_lines, master_HV.pel_clk_a_line);
	DO_DIV_ROUND(vertical_lines, 512000000);

	/* FIXME: master vsync width is 14-bits, so need to adjust pll setting if it exceeds */
	if(vertical_lines > 0x3FFF) {
		/* FIXME: workaroud to reduce the framerate */
		u64 factor = 0;

		vin_dbg("vertical_lines %lld> 0x3FFF\n", vertical_lines);
		factor = vertical_lines;
		DO_DIV_ROUND(factor, 0x3FFF);
		factor++;

		if (factor) {
			DO_DIV_ROUND(vertical_lines, factor);
			pinfo->svr_value = factor - 1;
			imx226_write_reg(src, IMX226_REG_SVR_LSB, factor - 1);
		}
	} else {
		pinfo->svr_value = 0;
		imx226_write_reg(src, IMX226_REG_SVR_LSB, 0x0);
	}

	master_HV.line_num_a_field = vertical_lines;
	pinfo->xvs_num[format_index] = master_HV.line_num_a_field;
	imx226_set_master_HV(src, master_HV);

	imx226_set_shutter_time(src, pinfo->current_shutter_time);// keep the same shutter time

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_irq_fix.delay = 0;// change to zero for all non Aptina Hi-SPI sensors (for A5s/A7/S2)
	/*
	errCode = imx226_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode)
		goto imx226_set_fps_exit;
	*/
	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

imx226_set_fps_exit:
	return errCode;
}

static void imx226_start_streaming(struct __amba_vin_source *src)
{
	imx226_write_reg(src, IMX226_REG_00, 0x04); //STANDBY:0, STBLOGIC:0
	msleep(20);
	imx226_write_reg(src, IMX226_REG_01, 0x11); //DCKRST:1, CLPSQRST:1
	msleep(20);
}

static int imx226_set_video_index( struct __amba_vin_source *src, u32 index)
{
	int				errCode = 0;
	struct imx226_info		*pinfo;
	u32				format_index;

	pinfo = (struct imx226_info *)src->pinfo;

	if (index >= IMX226_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx226_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto imx226_set_mode_exit;
	}

	errCode |= imx226_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = imx226_video_info_table[index].format_index;

	pinfo->cap_start_x 	= imx226_video_info_table[index].def_start_x;
	pinfo->cap_start_y 	= imx226_video_info_table[index].def_start_y;
	pinfo->cap_cap_w 	= imx226_video_info_table[index].def_width;
	pinfo->cap_cap_h 	= imx226_video_info_table[index].def_height;
	pinfo->bayer_pattern 	= imx226_video_info_table[index].bayer_pattern;
	pinfo->pll_index 	= imx226_video_format_tbl.table[format_index].pll_index;
	pinfo->lane_num	= imx226_video_format_tbl.table[format_index].lane_num;
	pinfo->slvs_eav_col	= imx226_video_format_tbl.table[format_index].slvs_eav_col;
	pinfo->slvs_sav2sav_dist = (imx226_video_format_tbl.table[format_index].xhs_clk *
		imx226_pll_tbl[pinfo->pll_index].factor *
		imx226_video_format_tbl.table[format_index].h_xhs *
		pinfo->lane_num) / imx226_video_format_tbl.table[format_index].bits;

	imx226_print_info(src);

	//set clk_si
	errCode |= imx226_init_vin_clock(src, &imx226_pll_tbl[pinfo->pll_index]);

	errCode |= imx226_set_vin_mode(src);

	imx226_fill_share_regs(src);
	imx226_fill_video_format_regs(src);
	imx226_set_fps(src, AMBA_VIDEO_FPS_AUTO);
	imx226_set_shutter_time(src, AMBA_VIDEO_FPS_60);
	imx226_set_agc_db(src, 0);

	errCode |= imx226_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

	/* TG reset release ( Enable Streaming )*/
	imx226_start_streaming(src);

imx226_set_mode_exit:
	return errCode;
}

static int imx226_set_video_mode( struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int				errCode = -EINVAL;
	int				i;
	struct imx226_info		*pinfo;

	static int			first_set_video_mode = 1;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct imx226_info *)src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errCode = imx226_init_vin_clock(src, &imx226_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto imx226_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	imx226_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	if (mode == AMBA_VIDEO_MODE_AUTO) {
		mode = IMX226_VIDEO_MODE_TABLE_AUTO;
	}

	for (i = 0; i < IMX226_VIDEO_MODE_TABLE_SIZE; i++) {
		if (imx226_video_mode_table[i].mode == mode) {
			errCode = imx226_set_video_index(src, imx226_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= IMX226_VIDEO_MODE_TABLE_SIZE) {
		vin_err("imx226_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = imx226_video_mode_table[i].mode;
		pinfo->mode_type = imx226_video_mode_table[i].preview_mode_type;
	}

imx226_set_video_mode_exit:
	return errCode;
}

static int imx226_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	struct imx226_info		*pinfo;

	pinfo = (struct imx226_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = imx226_init_vin_clock(src, &imx226_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto imx226_init_hw_exit;
	msleep(10);
	imx226_reset(src);

imx226_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int imx226_docmd( struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int				errCode = 0;
	struct imx226_info		*pinfo;

	pinfo = (struct imx226_info *) src->pinfo;

	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		imx226_reset(src);
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
		errCode = imx226_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO: {
		struct amba_vin_source_info	*pub_src;

		pub_src = (struct amba_vin_source_info *)args;
		pub_src->id = src->id;
		pub_src->adapter_id = adapter_id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof(pub_src->name));
		pub_src->sensor_id = SENSOR_IMX226;
		pub_src->default_mode = IMX226_VIDEO_MODE_TABLE_AUTO;
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = imx226_check_video_mode(src, (struct amba_vin_source_mode_info *)args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = imx226_get_video_info(src, (struct amba_video_info *)args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = imx226_get_agc_info(src, (amba_vin_agc_info_t *)args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = imx226_get_shutter_info(src, (amba_vin_shutter_info_t *)args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = imx226_get_capability(src, (struct amba_vin_src_capability *)args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = imx226_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		pinfo->agc_call_cnt = 0;
		errCode = imx226_set_video_mode(src, *(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = imx226_set_still_mode(src, (struct amba_vin_src_still_info *)args);
		break;

	case AMBA_VIN_SRC_GET_BLC:
		errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_GET_SW_BLC, (void *)args);
		break;

	case AMBA_VIN_SRC_SET_BLC:
		errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_SW_BLC, (void *)args);
		break;

	case AMBA_VIN_SRC_GET_FRAME_RATE:
		*(u32 *)args = pinfo->frame_rate;
		break;

	case AMBA_VIN_SRC_SET_FRAME_RATE:
		errCode = imx226_set_fps(src, *(u32 *)args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = imx226_set_shutter_and_agc(src,
			*(u32 *)args, 0, IMX226_SET_SHUTTER | IMX226_UPDATE_SYNC);
		break;

	case  AMBA_VIN_SRC_SET_SHUTTER_AND_GAIN_SYNC: {
		struct sensor_cmd_info_t *p_shutter_agc;
		p_shutter_agc = (struct sensor_cmd_info_t *) args;

		errCode = imx226_set_shutter_and_agc(src,
			p_shutter_agc->shutter_row,
			p_shutter_agc->gain_tbl_index,
			IMX226_SET_SHUTTER | IMX226_SET_AGC | IMX226_UPDATE_SYNC);
		}
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(u32 *)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = imx226_set_shutter_and_agc(src,
			0, *(s32 *)args, IMX226_SET_AGC | IMX226_UPDATE_SYNC);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = imx226_set_low_light_agc(src, *(u32 *)args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = imx226_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA: {
		struct amba_vin_test_reg_data	*reg_data;
		u16	subaddr;
		u8	data;

		reg_data = (struct amba_vin_test_reg_data *)args;
		subaddr = reg_data->reg;

		errCode = imx226_read_reg(src, subaddr, &data);

		reg_data->data = data;
		}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA: {
		struct amba_vin_test_reg_data *reg_data;
		u16	subaddr;
		u8	data;

		reg_data = (struct amba_vin_test_reg_data *)args;
		subaddr = reg_data->reg;
		data = reg_data->data;

		errCode = imx226_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW:
		errCode = imx226_set_shutter_row(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH:
		errCode=imx226_shutter_time2row(src, (u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_AAA_INFO: {
		struct amba_vin_aaa_info *aaa_info;

		aaa_info = (struct amba_vin_aaa_info *) args;
		aaa_info->pixel_size = pinfo->pixel_size;
		aaa_info->slow_shutter_support = 1;/* do slow shutter by sensor-self */
		}
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	}

	return errCode;
}

