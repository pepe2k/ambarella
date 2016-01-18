/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx122/imx122_docmd.c
 *
 * History:
 *    2011/09/23 - [Bingliang Hu] Create
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

static void imx122_fill_video_format_regs(struct __amba_vin_source *src)
{
	int			i;
	u32			index;
	u32			format_index;
	struct imx122_info	*pinfo;
	u8			video_mode;	//read from reg200h

	pinfo = (struct imx122_info *)src->pinfo;

	vin_dbg("imx122_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = imx122_video_info_table[index].format_index;

	for (i = 0; i < IMX122_VIDEO_FORMAT_REG_NUM; i++) {
		imx122_write_reg(src,
			imx122_video_format_tbl.reg[i],
			imx122_video_format_tbl.table[format_index].data[i]);
	}

	imx122_read_reg(src, IMX122_REG00, &video_mode);

	if((video_mode & 0x30) == 0x30) {
		// readout 1080pHD
		for(i = 0; i < IMX122_SHARE_1080P_CLASS_REG_SIZE; i ++) {
			imx122_write_reg(src, imx122_1080p_class_regs[i].reg, imx122_1080p_class_regs[i].data);
		}
	} else {
		// readout all pixel
		for(i = 0; i < IMX122_SHARE_3M_CLASS_REG_SIZE; i ++) {
			imx122_write_reg(src, imx122_3M_class_regs[i].reg, imx122_3M_class_regs[i].data);
		}
	}

	if (imx122_video_format_tbl.table[format_index].ext_reg_fill)
		imx122_video_format_tbl.table[format_index].ext_reg_fill(src);

}

static void imx122_fill_share_regs(struct __amba_vin_source *src)
{
	int				i;
	const struct imx122_reg_table	*reg_tbl = imx122_share_regs;

	vin_dbg("IMX122 fill share regs\n");
	for (i = 0; i < IMX122_SHARE_REG_SIZE; i++){
		vin_dbg("IMX122 write reg %d\n", i);
		imx122_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void imx122_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
}

static void imx122_fill_pll_regs(struct __amba_vin_source *src)
{

	int i;
	struct imx122_info *pinfo;
	const struct imx122_pll_reg_table 	*pll_reg_table;
	const struct imx122_reg_table 		*pll_tbl;

	pinfo = (struct imx122_info *) src->pinfo;

	vin_dbg("imx122_fill_pll_regs\n");
	pll_reg_table = &imx122_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < IMX122_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			imx122_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
	msleep(2);
}

static int imx122_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct imx122_info  *pinfo;

	pinfo = (struct imx122_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int imx122_get_video_info( struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int	errCode = 0;
	u32	index;
	u32	format_index;
	struct imx122_info *pinfo;

	pinfo = (struct imx122_info *)src->pinfo;

	index = pinfo->current_video_index;

	if (index >= IMX122_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = imx122_video_info_table[index].format_index;

		p_video_info->width = imx122_video_info_table[index].def_width;
		p_video_info->height = imx122_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = imx122_video_format_tbl.table[format_index].format;
		p_video_info->type = imx122_video_format_tbl.table[format_index].type;
		p_video_info->bits = imx122_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = imx122_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}
static int imx122_get_agc_info( struct __amba_vin_source *src, amba_vin_agc_info_t *p_agc_info)
{
	int errCode = 0;
	struct imx122_info *pinfo;

	pinfo = (struct imx122_info *) src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}
static int imx122_get_shutter_info( struct __amba_vin_source *src, amba_vin_shutter_info_t *pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));

	return 0;
}
static int imx122_check_video_mode( struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int				errCode = 0;
	int				i;
	u32				index;
	u32				format_index;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof(p_mode_info->video_info));

	for (i = 0; i < IMX122_VIDEO_MODE_TABLE_SIZE; i++) {
		if (imx122_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = IMX122_VIDEO_MODE_TABLE_AUTO;
			p_mode_info->is_supported = 1;

			index = imx122_video_mode_table[i].still_index;
			format_index = imx122_video_info_table[index].format_index;

			p_mode_info->video_info.width = imx122_video_info_table[index].def_width;
			p_mode_info->video_info.height = imx122_video_info_table[index].def_height;
			p_mode_info->video_info.fps = imx122_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = imx122_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = imx122_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = imx122_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = imx122_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;

			break;
		}
	}

	return errCode;
}

static int imx122_set_still_mode( struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{ return 0; }

static int imx122_set_low_light_agc( struct __amba_vin_source *src, u32 agc_index)
{ return 0; }

static int imx122_set_shutter_time( struct __amba_vin_source *src, u32 shutter_time)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length;
	u8 	shr_width_m, shr_width_l;
	int	blank_lines;

	const struct imx122_pll_reg_table *pll_table;
	struct imx122_info		*pinfo;
	pinfo = (struct imx122_info *)src->pinfo;

	pll_table = &imx122_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time;

	imx122_read_reg(src, IMX122_VMAX_MSB, &shr_width_m);
	imx122_read_reg(src, IMX122_VMAX_LSB, &shr_width_l);
	frame_length = (shr_width_m<<8) + shr_width_l;
	BUG_ON(frame_length == 0);

	imx122_read_reg(src, IMX122_HMAX_MSB, &shr_width_m);
	imx122_read_reg(src, IMX122_HMAX_LSB, &shr_width_l);
	line_length = ((shr_width_m&0x3F)<<8) + shr_width_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	/* t(frame)/t(per line) = t(exposure, in lines)*/
	exposure_time_q9 = exposure_time_q9 * (u64)pll_table->extclk;// imx122 spec: Hmax unit : INCK

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	blank_lines = frame_length - (u16)exposure_time_q9; /* get the shutter sweep time */

	if (unlikely(blank_lines < 0)) {
		blank_lines = 0;
	}

	shr_width_m = blank_lines >> 8;
	shr_width_l = blank_lines & 0xff;

	imx122_write_reg(src, IMX122_SHS1_MSB, shr_width_m);// SHS1 is for sweep time, should we reverse it for AEC?
	imx122_write_reg(src, IMX122_SHS1_LSB, shr_width_l);

	vin_dbg("frame_length: %d, sweep lines:%d\n", frame_length, blank_lines);
	pinfo->current_shutter_time = shutter_time;
	return 0;
}

static int imx122_set_agc_db( struct __amba_vin_source *src, s32 agc_db)
{
	int errCode = 0;
	u16 reg;
	struct imx122_info *pinfo;
	s32 db_max,db_set;
	s32 db_step;

	pinfo = (struct imx122_info *) src->pinfo;

	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;
	db_set = agc_db;

	vin_dbg("imx122_set_agc: 0x%x\n", db_set);

	if (db_set > db_max)
		db_set = db_max;

	reg = db_set / db_step;
	errCode = imx122_write_reg(src, IMX122_REG_GAIN, reg&0xFF);

	if(errCode == 0)
		pinfo->current_gain_db = agc_db;

	return errCode;
}

static int imx122_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int		errCode = 0;
	u32		readmode;
	struct	imx122_info			*pinfo;
	u32		target_bayer_pattern;
	u16		tmp_reg;

	pinfo = (struct imx122_info *) src->pinfo;

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
		readmode = IMX122_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GB;
		break;

	case AMBA_VIN_SRC_MIRROR_NONE:
		readmode = 0;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GB;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX122_H_MIRROR | IMX122_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GB;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		readmode = IMX122_H_MIRROR;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GB;
		break;

	default:
		vin_err("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	errCode |= imx122_read_reg(src, IMX122_REG01, (u8 *)&tmp_reg);
	tmp_reg &= ~(IMX122_H_MIRROR | IMX122_V_FLIP);
	tmp_reg |= readmode;
	errCode |= imx122_write_reg(src, IMX122_REG01, tmp_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}

/*
static int imx122_get_vblank_time( struct __amba_vin_source *src, u32 *ptime)
{
	u32					frame_length_lines, active_lines, format_index;
	u64					v_btime;
	u32					line_length;
	int					errCode = 0;
	int					mode_index;
	u8					data_val;

	struct imx122_info 			*pinfo;
	const struct imx122_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct imx122_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= imx122_video_info_table[mode_index].format_index;
	pll_reg_table	= &imx122_pll_tbl[pinfo->pll_index];
	frame_length_lines  = 0;

	errCode |= imx122_read_reg(src, IMX122_VMAX_MSB, &data_val);
	frame_length_lines += (u32)(data_val) <<  8;
	errCode |= imx122_read_reg(src, IMX122_VMAX_LSB, &data_val);
	frame_length_lines += (u32)(data_val);
	BUG_ON(frame_length_lines == 0);

	errCode |= imx122_read_reg(src, IMX122_HMAX_MSB, &data_val);
	line_length = (u32)(data_val & 0x0f) <<  8;
	errCode |= imx122_read_reg(src, IMX122_HMAX_LSB, &data_val);
	line_length += (u32)data_val;
	BUG_ON(line_length == 0);

	active_lines = imx122_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pll_reg_table->pixclk); //ns
	*ptime = v_btime;

	vin_dbg("imx122_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return errCode;
}
*/

static int imx122_set_fps( struct __amba_vin_source *src, int fps)
{
	int					errCode = 0;
	u32					frame_time = 0;
	u64					frame_time_pclk;
	u32					vertical_lines = 0;
	u32					line_length = 0;
	u32					storage_time = 0;
	u32					shs1 = 0;
	u32					index;
	u32					format_index;
	u8					data_val;
	u8					current_pll_index = 0;
	const struct imx122_pll_reg_table 	*pll_table;
	struct imx122_info			*pinfo;
	struct amba_vin_irq_fix			vin_irq_fix;

	pinfo = (struct imx122_info *)src->pinfo;
	index = pinfo->current_video_index;

	if (index >= IMX122_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx122_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto imx122_set_fps_exit;
	}

	format_index = imx122_video_info_table[index].format_index;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = imx122_video_format_tbl.table[format_index].auto_fps;
	if(fps < imx122_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, imx122_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto imx122_set_fps_exit;
	}

	frame_time = fps;

	/* ToDo: Add specified PLL index*/
	switch(fps) {
	case AMBA_VIDEO_FPS_29_97:
	case AMBA_VIDEO_FPS_59_94:
		current_pll_index = 1;
		break;
	default:
		current_pll_index = 0;
		break;
	}

	if(pinfo->pll_index != current_pll_index){
		pinfo->pll_index = current_pll_index;
		imx122_fill_pll_regs(src);
		errCode = imx122_init_vin_clock(src, &imx122_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto imx122_set_fps_exit;
	}

	pll_table = &imx122_pll_tbl[pinfo->pll_index];

	errCode |= imx122_read_reg(src, IMX122_VMAX_MSB, &data_val);
	vertical_lines += (u32)(data_val) <<  8;
	errCode |= imx122_read_reg(src, IMX122_VMAX_LSB, &data_val);
	vertical_lines += (u32)(data_val);
	BUG_ON(vertical_lines == 0);

	errCode |= imx122_read_reg(src, IMX122_SHS1_MSB, &data_val);
	shs1 = (u32)(data_val) <<  8;
	errCode |= imx122_read_reg(src, IMX122_SHS1_LSB, &data_val);
	shs1 += (u32)data_val;

	storage_time = vertical_lines - shs1;	//FIXME: ignore the toffset

	errCode |= imx122_read_reg(src, IMX122_HMAX_MSB, &data_val);
	line_length = (u32)(data_val & 0x0f) <<  8;
	errCode |= imx122_read_reg(src, IMX122_HMAX_LSB, &data_val);
	line_length += (u32)data_val;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	frame_time_pclk = frame_time * (u64)pll_table->pixclk;
	vin_dbg("pixclk %d, frame_time_pclk 0x%llx\n", pll_table->pixclk, frame_time_pclk);

	DO_DIV_ROUND(frame_time_pclk, line_length*2);
	DO_DIV_ROUND(frame_time_pclk, 512000000);
	vin_dbg("frame_time %d, vertical_lines 0x%llx\n", frame_time, frame_time_pclk);

	vertical_lines = frame_time_pclk;
	shs1 = vertical_lines - storage_time;		//to keep the same shutter time.

	vin_dbg("vertical_lines is  0x%05x\n", vertical_lines);
	errCode |= imx122_write_reg(src, IMX122_VMAX_MSB, (u8)((vertical_lines & 0x00FF00) >> 8));
	errCode |= imx122_write_reg(src, IMX122_VMAX_LSB, (u8)(vertical_lines & 0x0000FF));

	errCode |= imx122_write_reg(src, IMX122_SHS1_MSB, (u8)((shs1 & 0x00FF00) >> 8));
	errCode |= imx122_write_reg(src, IMX122_SHS1_LSB, (u8)(shs1 & 0x0000FF));

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_irq_fix.delay = 0;
	/*
	errCode = imx122_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode)
		goto imx122_set_fps_exit;
	*/
	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

imx122_set_fps_exit:
	return errCode;
}

static int imx122_set_video_index( struct __amba_vin_source *src, u32 index)
{
	int				errCode = 0;
	struct imx122_info		*pinfo;
	u32				format_index;

	pinfo = (struct imx122_info *)src->pinfo;

	if (index >= IMX122_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx122_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto imx122_set_mode_exit;
	}

	errCode |= imx122_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = imx122_video_info_table[index].format_index;

	pinfo->cap_start_x 	= imx122_video_info_table[index].def_start_x;
	pinfo->cap_start_y 	= imx122_video_info_table[index].def_start_y;
	pinfo->cap_cap_w 	= imx122_video_info_table[index].def_width;
	pinfo->cap_cap_h 	= imx122_video_info_table[index].def_height;
	pinfo->bayer_pattern 	= imx122_video_info_table[index].bayer_pattern;
	pinfo->pll_index 	= imx122_video_format_tbl.table[format_index].pll_index;

	imx122_print_info(src);

	//set clk_si
	errCode |= imx122_init_vin_clock(src, &imx122_pll_tbl[pinfo->pll_index]);

	errCode |= imx122_set_vin_mode(src);

	imx122_fill_pll_regs(src);

	imx122_fill_share_regs(src);

	imx122_fill_video_format_regs(src);
	imx122_set_fps(src, AMBA_VIDEO_FPS_AUTO);

	imx122_set_shutter_time(src, AMBA_VIDEO_FPS_60);
	imx122_set_agc_db(src, 0);

	errCode |= imx122_post_set_vin_mode(src);

	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

imx122_set_mode_exit:
	return errCode;
}

static int imx122_set_video_mode( struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int				errCode = -EINVAL;
	int				i;
	struct imx122_info		*pinfo;

	static int			first_set_video_mode = 1;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct imx122_info *)src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errCode = imx122_init_vin_clock(src, &imx122_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto imx122_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	imx122_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	if (mode == AMBA_VIDEO_MODE_AUTO) {
		mode = IMX122_VIDEO_MODE_TABLE_AUTO;
	}

	for (i = 0; i < IMX122_VIDEO_MODE_TABLE_SIZE; i++) {
		if (imx122_video_mode_table[i].mode == mode) {
			errCode = imx122_set_video_index(src, imx122_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= IMX122_VIDEO_MODE_TABLE_SIZE) {
		vin_err("imx122_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = imx122_video_mode_table[i].mode;
		pinfo->mode_type = imx122_video_mode_table[i].preview_mode_type;
	}

imx122_set_video_mode_exit:
	return errCode;
}

static int imx122_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	struct imx122_info		*pinfo;

	pinfo = (struct imx122_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = imx122_init_vin_clock(src, &imx122_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto imx122_init_hw_exit;
	msleep(10);
	imx122_reset(src);

imx122_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int imx122_docmd( struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int				errCode = 0;
	struct imx122_info		*pinfo;

	pinfo = (struct imx122_info *) src->pinfo;

	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		imx122_reset(src);
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
		errCode = imx122_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO:
	{
		struct amba_vin_source_info	*pub_src;

		pub_src = (struct amba_vin_source_info *)args;
		pub_src->id = src->id;
		pub_src->adapter_id = adapter_id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof(pub_src->name));
		pub_src->sensor_id = SENSOR_IMX122;
		pub_src->default_mode = IMX122_VIDEO_MODE_TABLE_AUTO;
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
	}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = imx122_check_video_mode(src, (struct amba_vin_source_mode_info *)args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = imx122_get_video_info(src, (struct amba_video_info *)args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = imx122_get_agc_info(src, (amba_vin_agc_info_t *)args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = imx122_get_shutter_info(src, (amba_vin_shutter_info_t *)args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = imx122_get_capability(src, (struct amba_vin_src_capability *)args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = imx122_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = imx122_set_video_mode(src, *(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = imx122_set_still_mode(src, (struct amba_vin_src_still_info *)args);
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
		errCode = imx122_set_fps(src, *(u32 *)args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = imx122_set_shutter_time(src, *(u32 *)args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(u32 *)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = imx122_set_agc_db(src, *(s32 *)args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = imx122_set_low_light_agc(src, *(u32 *)args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = imx122_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA:
	{
		struct amba_vin_test_reg_data	*reg_data;
		u16	subaddr;
		u8	data;

		reg_data = (struct amba_vin_test_reg_data *)args;
		subaddr = reg_data->reg;

		errCode = imx122_read_reg(src, subaddr, &data);

		reg_data->data = data;
	}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA:
	{
		struct amba_vin_test_reg_data *reg_data;
		u16	subaddr;
		u8	data;

		reg_data = (struct amba_vin_test_reg_data *)args;
		subaddr = reg_data->reg;
		data = reg_data->data;

		errCode = imx122_write_reg(src, subaddr, data);
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


