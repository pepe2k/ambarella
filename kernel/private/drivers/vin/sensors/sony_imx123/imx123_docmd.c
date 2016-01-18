/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx123/imx123_docmd.c
 *
 * History:
 *    2013/12/27 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

static void imx123_dump_reg(struct __amba_vin_source *src)
{
	u32 i;
	u16 reg_to_dump_init[] = {};

	for(i=0; i<sizeof(reg_to_dump_init)/sizeof(reg_to_dump_init[0]); i++) {
		u16 reg_addr = reg_to_dump_init[i];
		u8 reg_val;

		imx123_read_reg(src, reg_addr, &reg_val);
		vin_dbg("REG= 0x%04X, 0x%04X\n", reg_addr, reg_val);
	}

}

static void imx123_fill_video_format_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 format_index;
	struct imx123_info *pinfo;

	pinfo = (struct imx123_info *) src->pinfo;

	vin_dbg("imx123_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = imx123_video_info_table[index].format_index;

	for (i = 0; i < IMX123_VIDEO_FORMAT_REG_NUM; i++) {
		if (imx123_video_format_tbl.table[format_index].regs[i].reg == 0xFFFF)
			break;

		imx123_write_reg(src,
			imx123_video_format_tbl.table[format_index].regs[i].reg,
			imx123_video_format_tbl.table[format_index].regs[i].data);
	}

	if (imx123_video_format_tbl.table[format_index].ext_reg_fill)
		imx123_video_format_tbl.table[format_index].ext_reg_fill(src);
}

static void imx123_fill_share_regs(struct __amba_vin_source *src)
{
	int i;
	const struct imx123_reg_table	*reg_tbl;
	struct imx123_info *pinfo;

	pinfo = (struct imx123_info *) src->pinfo;

	switch(pinfo->op_mode) {
		case IMX123_LINEAR_MODE:
			vin_info("Linear mode\n");
			break;
		case IMX123_2X_WDR_MODE:
			vin_info("2x WDR mode\n");
			break;
		case IMX123_3X_WDR_MODE:
			vin_info("3x WDR mode\n");
			break;
		default:
			vin_err("Unsupported mode\n");
			return;
	}

	reg_tbl = imx123_share_regs;
	vin_dbg("imx123 fill share regs\n");
	for (i = 0; i < IMX123_SHARE_REG_SIZE; i++) {
		imx123_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}

	if (dual_gain) {
		reg_tbl = imx123_dual_gain_share_regs;
		for (i = 0; i < IMX123_DUAL_GAIN_SHARE_REG_SIZE; i++)
			imx123_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);

		vin_info("Dual gain wdr mode\n");
	}
}

static void imx123_sw_reset(struct __amba_vin_source *src)
{
	imx123_write_reg(src, IMX123_STANDBY, 0x1);//STANDBY
	imx123_write_reg(src, IMX123_SWRESET, 0x1);
	msleep(30);
}

static void imx123_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	imx123_sw_reset(src);
}

static void imx123_fill_pll_regs(struct __amba_vin_source *src)
{
	int i = 0;
	struct imx123_info *pinfo;
	const struct imx123_pll_reg_table 	*pll_reg_table;
	const struct imx123_reg_table 		*pll_tbl;

	pinfo = (struct imx123_info *) src->pinfo;

	vin_dbg("imx123_fill_pll_regs\n");
	pll_reg_table = &imx123_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < IMX123_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			imx123_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
}

static int imx123_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct imx123_info  *pinfo;

	pinfo = (struct imx123_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

#if 0
static int imx123_get_vblank_time( struct __amba_vin_source *src, u32 *ptime)
{
	u32					frame_length_lines, active_lines, format_index;
	u64					v_btime;
	u32					line_length;
	int					errCode = 0;
	int					mode_index;
	u8					data_val;

	struct imx123_info 			*pinfo;
	const struct imx123_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct imx123_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= imx123_video_info_table[mode_index].format_index;
	pll_reg_table	= &imx123_pll_tbl[pinfo->pll_index];

	errCode |= imx123_read_reg(src, IMX123_VMAX_HSB, &data_val);
	frame_length_lines = (u32)(data_val & 0x01) <<  16;
	errCode |= imx123_read_reg(src, IMX123_VMAX_MSB, &data_val);
	frame_length_lines += (u32)(data_val) <<  8;
	errCode |= imx123_read_reg(src, IMX123_VMAX_LSB, &data_val);
	frame_length_lines += (u32)(data_val);
	BUG_ON(frame_length_lines == 0);

	errCode |= imx123_read_reg(src, IMX123_HMAX_MSB, &data_val);
	line_length = (u32)(data_val & 0x0f) <<  8;
	errCode |= imx123_read_reg(src, IMX123_HMAX_LSB, &data_val);
	line_length += (u32)data_val;
	BUG_ON(line_length == 0);

	active_lines = imx123_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pll_reg_table->pixclk); //ns
	*ptime = v_btime;

	vin_dbg("imx123_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return 0;
}
#endif

static int imx123_get_vb_lines( struct __amba_vin_source *src, u16 *vblank_lines)
{
	int				errCode = 0;
	int				mode_index;
	u32				format_index;
	u32				frame_length_lines;
	u8				data_val_h, data_val_m, data_val_l;

	struct imx123_info 			*pinfo;

	pinfo = (struct imx123_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index = imx123_video_info_table[mode_index].format_index;

	errCode |= imx123_read_reg(src, IMX123_VMAX_HSB, &data_val_h);
	errCode |= imx123_read_reg(src, IMX123_VMAX_MSB, &data_val_m);
	errCode |= imx123_read_reg(src, IMX123_VMAX_LSB, &data_val_l);

	frame_length_lines = ((data_val_h&0x01)<<16) + (data_val_m<<8) + data_val_l;

	*vblank_lines = frame_length_lines - imx123_video_format_tbl.table[format_index].height;

	return errCode;
}

static int imx123_get_row_time( struct __amba_vin_source *src, u32 *row_time)
{
	int				errCode = 0;
	u64				h_time;
	u32				line_length;
	u8				data_val_m, data_val_l;

	struct imx123_info 			*pinfo;
	const struct imx123_pll_reg_table 	*pll_reg_table;

	pinfo = (struct imx123_info *)src->pinfo;
	pll_reg_table	= &imx123_pll_tbl[pinfo->pll_index];

	errCode |= imx123_read_reg(src, IMX123_HMAX_MSB, &data_val_m);
	errCode |= imx123_read_reg(src, IMX123_HMAX_LSB, &data_val_l);

	line_length = ((data_val_m)<<8) + data_val_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	h_time = (u64)line_length * 1000000000;
	DO_DIV_ROUND(h_time, (u64)pll_reg_table->pixclk); /* ns */

	*row_time = h_time;

	return errCode;
}

static int imx123_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errCode = 0;
	u32 index;
	u32 format_index;

	struct imx123_info *pinfo;

	pinfo = (struct imx123_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= IMX123_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = imx123_video_info_table[index].format_index;
		p_video_info->width = imx123_video_info_table[index].act_width;
		p_video_info->height = imx123_video_info_table[index].act_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = imx123_video_format_tbl.table[format_index].format;
		p_video_info->type = imx123_video_format_tbl.table[format_index].type;
		p_video_info->bits = imx123_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = imx123_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

static int imx123_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct imx123_info *pinfo;

	pinfo = (struct imx123_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}

static int imx123_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));
	return 0;
}

static int imx123_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;
	u32 video_tbl_size;
	struct imx123_info *pinfo;
	struct imx123_video_mode *video_mode_tbl;

	pinfo = (struct imx123_info *) src->pinfo;
	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	switch(pinfo->op_mode) {
		case IMX123_LINEAR_MODE:
			if (fps120_mode) {
				video_tbl_size = IMX123_VIDEO_MODE_120FPS_TABLE_SIZE;
				video_mode_tbl = imx123_video_mode_120fps_table;
			} else {
				video_tbl_size = IMX123_VIDEO_MODE_TABLE_SIZE;
				video_mode_tbl = imx123_video_mode_table;
			}
			break;
		case IMX123_2X_WDR_MODE:
			if (dual_gain) {
				video_tbl_size = IMX123_VIDEO_MODE_DUALGAIN_TABLE_SIZE;
				video_mode_tbl = imx123_video_mode_dualgain_table;
			} else {
				video_tbl_size = IMX123_VIDEO_MODE_2XDOL_TABLE_SIZE;
				video_mode_tbl = imx123_video_mode_2xdol_table;
			}
			break;
		case IMX123_3X_WDR_MODE:
			video_tbl_size = IMX123_VIDEO_MODE_3XDOL_TABLE_SIZE;
			video_mode_tbl = imx123_video_mode_3xdol_table;
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
				p_mode_info->mode = IMX123_VIDEO_MODE_TABLE_AUTO;

			p_mode_info->is_supported = 1;

			index = video_mode_tbl[i].still_index;
			format_index = imx123_video_info_table[index].format_index;

			p_mode_info->video_info.width = imx123_video_info_table[index].act_width;
			p_mode_info->video_info.height = imx123_video_info_table[index].act_height;
			p_mode_info->video_info.fps = imx123_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = imx123_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = imx123_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = imx123_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = imx123_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;

			break;
		}
	}

	return errCode;
}

static int imx123_query_sensor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	int 	errCode = 0;
	return errCode;
}

static int imx123_query_sensor_version(struct __amba_vin_source *src, u16 * ver)
{
	/* >> TODO << */
	return 0;
}

static int imx123_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length;
	u8 	shr_width_h, shr_width_m, shr_width_l;
	int	shutter_width;

	const struct imx123_pll_reg_table *pll_table;
	struct imx123_info		*pinfo;
	pinfo = (struct imx123_info *)src->pinfo;

	pll_table = &imx123_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time;

	imx123_read_reg(src, IMX123_VMAX_HSB, &shr_width_h);
	imx123_read_reg(src, IMX123_VMAX_MSB, &shr_width_m);
	imx123_read_reg(src, IMX123_VMAX_LSB, &shr_width_l);

	frame_length = ((shr_width_h&0x01)<<16) + (shr_width_m<<8) + shr_width_l;

	imx123_read_reg(src, IMX123_HMAX_MSB, &shr_width_m);
	imx123_read_reg(src, IMX123_HMAX_LSB, &shr_width_l);

	line_length = ((shr_width_m)<<8) + shr_width_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	/* t(frame)/t(per line) = t(exposure, in lines)*/
	exposure_time_q9 = exposure_time_q9 * pll_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (u32)exposure_time_q9;

	/* FIXME: shutter width: 3 ~ (Frame format(V) - 4) */
	if(shutter_width < 3) {
		shutter_width = 3;
	} else if(shutter_width  > frame_length - 4) {
		shutter_width = frame_length - 4;
	}

	shr_width_h = (frame_length - shutter_width) >> 16;
	shr_width_m = (frame_length - shutter_width) >> 8;
	shr_width_l = (frame_length - shutter_width) & 0xff;

	imx123_write_reg(src, IMX123_SHS1_HSB, shr_width_h);
	imx123_write_reg(src, IMX123_SHS1_MSB, shr_width_m);
	imx123_write_reg(src, IMX123_SHS1_LSB, shr_width_l);

	vin_dbg("frame_length: %d, sweep lines:%d\n", frame_length, frame_length - shutter_width);
	pinfo->current_shutter_time = shutter_time;
	return 0;
}

static int imx123_shutter_time2width(struct __amba_vin_source *src, u32* shutter_time)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length;
	u8 	shr_width_h, shr_width_m, shr_width_l;
	int	shutter_width;

	const struct imx123_pll_reg_table *pll_table;
	struct imx123_info		*pinfo;
	pinfo = (struct imx123_info *)src->pinfo;

	pll_table = &imx123_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = *shutter_time;

	imx123_read_reg(src, IMX123_VMAX_HSB, &shr_width_h);
	imx123_read_reg(src, IMX123_VMAX_MSB, &shr_width_m);
	imx123_read_reg(src, IMX123_VMAX_LSB, &shr_width_l);

	frame_length = ((shr_width_h&0x01)<<16) + (shr_width_m<<8) + shr_width_l;

	imx123_read_reg(src, IMX123_HMAX_MSB, &shr_width_m);
	imx123_read_reg(src, IMX123_HMAX_LSB, &shr_width_l);

	line_length = ((shr_width_m)<<8) + shr_width_l;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	/* t(frame)/t(per line) = t(exposure, in lines)*/
	exposure_time_q9 = exposure_time_q9 * pll_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (u32)exposure_time_q9;

	/* FIXME: shutter width: 3 ~ (Frame format(V) - 4) */
	if(shutter_width < 3) {
		shutter_width = 3;
	} else if(shutter_width  > frame_length - 4) {
		shutter_width = frame_length - 4;
	}

	*shutter_time =shutter_width;
	return 0;
}

static int imx123_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int					errCode = 0;
	u32					frame_time = 0;
	u64					frame_time_pclk;
	u32					vertical_lines = 0, fsc;
	u32					line_length = 0;
	u32					index;
	u32					format_index;
	u8					data_val;
	const struct imx123_pll_reg_table 	*pll_table;
	struct imx123_info			*pinfo;
	struct amba_vin_irq_fix			vin_irq_fix;

	pinfo = (struct imx123_info *)src->pinfo;
	index = pinfo->current_video_index;

	if (index >= IMX123_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx123_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto imx123_set_fps_exit;
	}

	format_index = imx123_video_info_table[index].format_index;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = imx123_video_format_tbl.table[format_index].auto_fps;
	if(fps < imx123_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, imx123_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto imx123_set_fps_exit;
	}

	frame_time = fps;

	/* ToDo: Add specified PLL index*/

	pll_table = &imx123_pll_tbl[pinfo->pll_index];

	errCode |= imx123_read_reg(src, IMX123_HMAX_MSB, &data_val);
	line_length = (u32)(data_val) <<  8;
	errCode |= imx123_read_reg(src, IMX123_HMAX_LSB, &data_val);
	line_length += (u32)data_val;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	frame_time_pclk = frame_time * (u64)pll_table->pixclk;

	vin_dbg("pixclk %d, frame_time_pclk 0x%llx\n", pll_table->pixclk, frame_time_pclk);

	DO_DIV_ROUND(frame_time_pclk, line_length);
	DO_DIV_ROUND(frame_time_pclk, 512000000);

	fsc = frame_time_pclk;

	if (pinfo->op_mode == IMX123_LINEAR_MODE || dual_gain) {
		vertical_lines = fsc;/* FSC = VMAX */
	} else if(pinfo->op_mode == IMX123_2X_WDR_MODE) {
		vertical_lines = fsc >> 1; /* FSC = VMAX * 2 */
	} else if (pinfo->op_mode == IMX123_3X_WDR_MODE) {
		vertical_lines = fsc >> 2; /* FSC = VMAX * 4 */
	}

	vin_dbg("vertical_lines is  0x%05x\n", vertical_lines);
	errCode |= imx123_write_reg(src, IMX123_VMAX_HSB, (u8)((vertical_lines & 0x010000) >> 16));
	errCode |= imx123_write_reg(src, IMX123_VMAX_MSB, (u8)((vertical_lines & 0x00FF00) >> 8));
	errCode |= imx123_write_reg(src, IMX123_VMAX_LSB, (u8)(vertical_lines & 0x0000FF));

	imx123_set_shutter_time(src, pinfo->current_shutter_time);// keep the same shutter time

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_irq_fix.delay = 0;// change to zero for all non Aptina Hi-SPI sensors (for A5s/A7/S2)
	/*
	errCode = imx123_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode)
		goto imx123_set_fps_exit;
	*/
	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

imx123_set_fps_exit:
	return errCode;
}

static void imx123_start_streaming(struct __amba_vin_source *src)
{
	imx123_write_reg(src, IMX123_XMSTA, 0x00); //master mode start
	imx123_write_reg(src, IMX123_STANDBY, 0x00); //cancel standby
}

static int imx123_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct imx123_info *pinfo;
	u32 format_index;

	pinfo = (struct imx123_info *) src->pinfo;

	if (index >= IMX123_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx123_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto imx123_set_mode_exit;
	}

	errCode |= imx123_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = imx123_video_info_table[index].format_index;

	pinfo->cap_start_x = imx123_video_info_table[index].def_start_x;
	pinfo->cap_start_y = imx123_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = imx123_video_info_table[index].def_width;
	pinfo->cap_cap_h = imx123_video_info_table[index].def_height;
	pinfo->act_start_x = imx123_video_info_table[index].act_start_x;
	pinfo->act_start_y = imx123_video_info_table[index].act_start_y;
	pinfo->act_width = imx123_video_info_table[index].act_width;
	pinfo->act_height = imx123_video_info_table[index].act_height;
	pinfo->slvs_eav_col = imx123_video_format_tbl.table[format_index].slvs_eav_col;
	pinfo->slvs_sav2sav_dist = imx123_video_format_tbl.table[format_index].slvs_sav2sav_dist;
	pinfo->bayer_pattern = imx123_video_info_table[index].bayer_pattern;
	//set the specified PLL index for each sensor_mode.
	pinfo->pll_index = imx123_video_format_tbl.table[format_index].pll_index;
	imx123_print_info(src);

	//set clk_si
	errCode |= imx123_init_vin_clock(src, &imx123_pll_tbl[pinfo->pll_index]);
	errCode |= imx123_set_vin_mode(src);

	imx123_fill_pll_regs(src);
	imx123_fill_share_regs(src);
	imx123_fill_video_format_regs(src);

	imx123_set_fps(src, AMBA_VIDEO_FPS_AUTO);
	imx123_set_shutter_time(src, AMBA_VIDEO_FPS_60);

	errCode |= imx123_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

	imx123_start_streaming(src);

imx123_set_mode_exit:
	return errCode;
}

static int imx123_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errCode = -EINVAL;
	int i;
	struct imx123_info *pinfo;
	int errorCode = 0;
	static int 	first_set_video_mode = 1;
	u32 video_tbl_size;
	struct imx123_video_mode *video_mode_tbl;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct imx123_info *) src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errorCode = imx123_init_vin_clock(src, &imx123_pll_tbl[pinfo->pll_index]);
		if (errorCode)
			goto imx123_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	imx123_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	if (mode == AMBA_VIDEO_MODE_AUTO) {
		mode = IMX123_VIDEO_MODE_TABLE_AUTO;
	}

	switch(pinfo->op_mode) {
		case IMX123_LINEAR_MODE:
			if (fps120_mode) {
				video_tbl_size = IMX123_VIDEO_MODE_120FPS_TABLE_SIZE;
				video_mode_tbl = imx123_video_mode_120fps_table;
			} else {
				video_tbl_size = IMX123_VIDEO_MODE_TABLE_SIZE;
				video_mode_tbl = imx123_video_mode_table;
			}
			break;
		case IMX123_2X_WDR_MODE:
			if (dual_gain) {
				video_tbl_size = IMX123_VIDEO_MODE_DUALGAIN_TABLE_SIZE;
				video_mode_tbl = imx123_video_mode_dualgain_table;
			} else {
				video_tbl_size = IMX123_VIDEO_MODE_2XDOL_TABLE_SIZE;
				video_mode_tbl = imx123_video_mode_2xdol_table;
			}
			break;
		case IMX123_3X_WDR_MODE:
			video_tbl_size = IMX123_VIDEO_MODE_3XDOL_TABLE_SIZE;
			video_mode_tbl = imx123_video_mode_3xdol_table;
			break;
		default:
			vin_err("Unsupported mode:%d\n", pinfo->op_mode);
			video_tbl_size = 0;
			video_mode_tbl = NULL;
			return -EPERM;
	}

	for (i = 0; i < video_tbl_size; i++) {
		if (video_mode_tbl[i].mode == mode) {
			errCode = imx123_set_video_index(src, video_mode_tbl[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}

	if (i >= video_tbl_size) {
		vin_err("imx123_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = video_mode_tbl[i].mode;
		pinfo->mode_type = video_mode_tbl[i].preview_mode_type;
	}

imx123_set_video_mode_exit:
	return errCode;
}

static int imx123_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	struct imx123_info		*pinfo;

	pinfo = (struct imx123_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = imx123_init_vin_clock(src, &imx123_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto imx123_init_hw_exit;

	msleep(10);
	imx123_reset(src);

imx123_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int imx123_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{
	return 0;
}

static int imx123_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
	/* >> TODO, for RGB mode only << */
	return 0;
}

static int imx123_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	int errCode = 0;
	u16 reg_d;
	struct imx123_info *pinfo;
	u32 gain_index;
	s32 db_max;
	s32 db_step;

	pinfo = (struct imx123_info *) src->pinfo;

	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	vin_dbg("imx123_set_agc: 0x%x\n", agc_db);

	gain_index = (db_max - agc_db) / db_step;

	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		reg_d = IMX123_GAIN_MAX_IDX - gain_index;
		imx123_write_reg(src, IMX123_GAIN_LSB, (u8)(reg_d&0xFF));
		imx123_write_reg(src, IMX123_GAIN_MSB, (u8)(reg_d>>8));
		pinfo->current_gain_db = agc_db;
	} else {
		errCode = -1;	/* Index is out of range */
	}
	return errCode;
}

static int imx123_set_agc_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct imx123_info *pinfo;

	vin_dbg("imx123_set_agc_index: %d\n", index);

	pinfo = (struct imx123_info *) src->pinfo;

	if ((index >= pinfo->min_agc_index) && (index <= pinfo->max_agc_index)) {
		imx123_write_reg(src, IMX123_GAIN_LSB, (u8)(index&0xFF));
		imx123_write_reg(src, IMX123_GAIN_MSB, (u8)(index>>8));
	} else {
		errCode = -1;	/* Index is out of range */
	}
	return errCode;
}

static int imx123_set_wdr_again_index_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_gain_gp_info *p_again_gp)
{
	struct imx123_info *pinfo;
	u32 again_index;

	pinfo = (struct imx123_info *) src->pinfo;

	if (!dual_gain) {
		vin_err("%s can only be supported by dual gain mode\n", __func__);
		return -EPERM;
	}

	/* long frame */
	again_index = p_again_gp->long_gain;
	if ((again_index >= pinfo->min_wdr_agc_index) && (again_index <= pinfo->max_wdr_agc_index)) {
		imx123_write_reg(src, IMX123_GAIN_LSB, (u8)(again_index&0xFF));
		imx123_write_reg(src, IMX123_GAIN_MSB, (u8)(again_index>>8)&0x3);
	} else {
		vin_err("dual gain long frame gain index %d exceeds [%d~%d]\n",
			again_index,
			again_index <= pinfo->min_wdr_agc_index,
			again_index <= pinfo->max_wdr_agc_index);
		return -EPERM;
	}

	/* short frame 1 */
	again_index = p_again_gp->short1_gain;
	if ((again_index >= pinfo->min_wdr_agc_index) && (again_index <= pinfo->max_wdr_agc_index)) {
		imx123_write_reg(src, IMX123_GAIN2_LSB, (u8)(again_index&0xFF));
		imx123_write_reg(src, IMX123_GAIN2_MSB, (u8)(again_index>>8)&0x3);
	} else {
		vin_err("dual gain short frame gain index %d exceeds [%d~%d]\n",
			again_index,
			again_index <= pinfo->min_wdr_agc_index,
			again_index <= pinfo->max_wdr_agc_index);
		return -EPERM;
	}

	vin_dbg("long again index:%d, short1 again index:%d, short2 again index:%d\n",
		p_again_gp->long_gain, p_again_gp->short1_gain, p_again_gp->short2_gain);

	return 0;
}

static int imx123_set_wdr_dgain_index_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_gain_gp_info *p_dgain_gp)
{
	return 0;
}

static int imx123_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int		errCode = 0;
	u32		readmode;
	struct	imx123_info			*pinfo;
	u32		target_bayer_pattern;
	u16		tmp_reg;

	pinfo = (struct imx123_info *) src->pinfo;

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
		readmode = IMX123_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	case AMBA_VIN_SRC_MIRROR_NONE:
		readmode = 0;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX123_H_MIRROR | IMX123_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		readmode = IMX123_H_MIRROR;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	default:
		vin_err("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	errCode |= imx123_read_reg(src, IMX123_WINMODE, (u8 *)&tmp_reg);
	tmp_reg &= ~(IMX123_H_MIRROR | IMX123_V_FLIP);
	tmp_reg |= readmode;
	errCode |= imx123_write_reg(src, IMX123_WINMODE, tmp_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}

static int imx123_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
	/* >> TODO, for YUV output mode only << */
	return errCode;
}

static int imx123_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;
	/* >> TODO, for RGB raw output mode only << */
	/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}

static int imx123_set_wdr_shutter_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_shutter_gp_info *p_shutter_gp)
{
	u64 exposure_time_q9;
	u8 	data;
	u32 line_length, frame_length, fsc, rhs1, rhs2;
	u32 index, format_index;
	int shutter_long, shutter_short1, shutter_short2;
	int errCode = 0;
	struct imx123_info *pinfo;
	const struct imx123_pll_reg_table *pll_reg_table;

	pinfo		= (struct imx123_info *)src->pinfo;
	index = pinfo->current_video_index;
	format_index = imx123_video_info_table[index].format_index;
	pll_reg_table	= &imx123_pll_tbl[pinfo->pll_index];

	imx123_read_reg(src, IMX123_HMAX_MSB, &data);
	line_length = data << 8;
	imx123_read_reg(src, IMX123_HMAX_LSB, &data);
	line_length += data;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	imx123_read_reg(src, IMX123_RHS1_HSB, &data);
	rhs1 = (data & 0x0F) << 16;
	imx123_read_reg(src, IMX123_RHS1_MSB, &data);
	rhs1 += data << 8;
	imx123_read_reg(src, IMX123_RHS1_LSB, &data);
	rhs1 += data;

	imx123_read_reg(src, IMX123_RHS2_HSB, &data);
	rhs2 = (data & 0x0F) << 16;
	imx123_read_reg(src, IMX123_RHS2_MSB, &data);
	rhs2 += data << 8;
	imx123_read_reg(src, IMX123_RHS2_LSB, &data);
	rhs2 += data;

	imx123_read_reg(src, IMX123_VMAX_HSB, &data);
	frame_length = (data & 0x0F) << 16;
	imx123_read_reg(src, IMX123_VMAX_MSB, &data);
	frame_length += data << 8;
	imx123_read_reg(src, IMX123_VMAX_LSB, &data);
	frame_length += data;

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

	/* shutter limitation check */
	if(pinfo->op_mode == IMX123_2X_WDR_MODE) {
		fsc = frame_length << 1; /* FSC = VMAX * 2 */
		if(fsc < shutter_long) {
			vin_err("long shutter row(%d) must be less than fsc(%d)!\n", shutter_long, fsc);
			return -EPERM;
		} else {
			shutter_long = fsc - shutter_long;
		}
		if(rhs1 < shutter_short1) {
			vin_err("short shutter row(%d) must be less than rhs(%d)!\n", shutter_short1, rhs1);
			return -EPERM;
		} else {
			shutter_short1 = rhs1 - shutter_short1;
		}
		vin_dbg("fsc:%d, shs1:%d, shs2:%d, rhs1:%d\n",
			fsc, shutter_short1, shutter_long, rhs1);

		/* short shutter check */
		if((shutter_short1 >= 7) && (shutter_short1 != 9) && (shutter_short1 != 10) &&
			(shutter_short1 < rhs1)  && (shutter_short1 != rhs1 - 3) && (shutter_short1 != rhs1 - 4)) {
			imx123_write_reg(src, IMX123_SHS1_LSB, (u8)(shutter_short1 & 0xFF));
			imx123_write_reg(src, IMX123_SHS1_MSB, (u8)(shutter_short1 >> 8));
			imx123_write_reg(src, IMX123_SHS1_HSB, (u8)((shutter_short1 >> 16) & 0xF));
		} else {
			vin_err("shs1 exceeds limitation! shs1:%d, rhs1:%d\n",
					shutter_short1, rhs1);
			return -EPERM;
		}
		/* long shutter check */
		if((shutter_long >= rhs1 + 7) && (shutter_long != rhs1 + 9) && (shutter_long != rhs1 + 10) &&
			(shutter_long < fsc)  && (shutter_long != fsc - 3) && (shutter_long != fsc - 4) &&
			(shutter_long != shutter_short1 + 9) && (shutter_long != fsc + shutter_short1 - 9)) {
			imx123_write_reg(src, IMX123_SHS2_LSB, (u8)(shutter_long & 0xFF));
			imx123_write_reg(src, IMX123_SHS2_MSB, (u8)(shutter_long >> 8));
			imx123_write_reg(src, IMX123_SHS2_HSB, (u8)((shutter_long >> 16) & 0xF));
		} else {
			vin_err("shs2 exceeds limitation! shs2:%d, shs1:%d, rhs1:%d, fsc:%d\n",
					shutter_long, shutter_short1, rhs1, fsc);
			return -EPERM;
		}

	} else if(pinfo->op_mode == IMX123_3X_WDR_MODE) {
		fsc = frame_length << 2; /* FSC = VMAX * 4 */
		if(fsc < shutter_long) {
			vin_err("long shutter row(%d) must be less than fsc(%d)!\n", shutter_long, fsc);
			return -EPERM;
		} else {
			shutter_long = fsc - shutter_long;
		}
		if(rhs1 < shutter_short1) {
			vin_err("short shutter 1 row(%d) must be less than rhs1(%d)!\n", shutter_short1, rhs1);
			return -EPERM;
		} else {
			shutter_short1 = rhs1 - shutter_short1;
		}
		if(rhs2 < shutter_short2) {
			vin_err("short shutter 2 row(%d) must be less than rhs2(%d)!\n", shutter_short2, rhs2);
			return -EPERM;
		} else {
			shutter_short2 = rhs2 - shutter_short2;
		}
		vin_dbg("fsc:%d, shs1:%d, shs2:%d, shs3:%d, rhs1:%d, rhs2:%d\n",
			fsc, shutter_short1, shutter_short2, shutter_long, rhs1, rhs2);

		/* short shutter 1 check */
		if((shutter_short1 >= 9) && (shutter_short1 != 13) && (shutter_short1 != 14) &&
			(shutter_short1 < rhs1)  && (shutter_short1 != rhs1 - 5) && (shutter_short1 != rhs1 - 6)) {
			imx123_write_reg(src, IMX123_SHS1_LSB, (u8)(shutter_short1 & 0xFF));
			imx123_write_reg(src, IMX123_SHS1_MSB, (u8)(shutter_short1 >> 8));
			imx123_write_reg(src, IMX123_SHS1_HSB, (u8)((shutter_short1 >> 16) & 0xF));
		} else {
			vin_err("shs1 exceeds limitation! shs1:%d, rhs1:%d\n",
					shutter_short1, rhs1);
			return -EPERM;
		}
		/* short shutter 2 check */
		if((shutter_short2 >= rhs1 + 9) && (shutter_short2 != rhs1 + 13) && (shutter_short2 != rhs1 + 14) &&
			(shutter_short2 < rhs2) && (shutter_short2 != rhs2 - 5) && (shutter_short2 != rhs2 - 6) &&
			(shutter_short2 != shutter_short1 + 13) && (shutter_short2 != shutter_long - 13)) {
			imx123_write_reg(src, IMX123_SHS2_LSB, (u8)(shutter_short2 & 0xFF));
			imx123_write_reg(src, IMX123_SHS2_MSB, (u8)(shutter_short2 >> 8));
			imx123_write_reg(src, IMX123_SHS2_HSB, (u8)((shutter_short2 >> 16) & 0xF));
		} else {
			vin_err("shs2 exceeds limitation! shs1:%d, shs2:%d, shs3:%d, rhs1:%d, rhs2:%d\n",
					shutter_short1, shutter_short2, shutter_long, rhs1, rhs2);
			return -EPERM;
		}
		/* long shutter check */
		if((shutter_long >= rhs2 + 9) && (shutter_long != rhs2 + 13) && (shutter_long != rhs2 + 14) &&
			(shutter_long < fsc) && (shutter_long != fsc - 5) && (shutter_long != fsc - 6) &&
			(shutter_long != shutter_short2 + 13) && (shutter_long != fsc + shutter_short1 - 13)) {
			imx123_write_reg(src, IMX123_SHS3_LSB, (u8)(shutter_long & 0xFF));
			imx123_write_reg(src, IMX123_SHS3_MSB, (u8)(shutter_long >> 8));
			imx123_write_reg(src, IMX123_SHS3_HSB, (u8)((shutter_long >> 16) & 0xF));
		} else {
			vin_err("shs3 exceeds limitation! shs1:%d, shs2:%d, shs3:%d, rhs2:%d, fsc:%d\n",
					shutter_short1, shutter_short2, shutter_long, rhs2, fsc);
			return -EPERM;
		}
	} else {
		vin_err("Non WDR mode can't support this API: %s!\n", __func__);
	}

	return errCode;
}

static int imx123_set_wdr_shutter_row_group(struct __amba_vin_source *src,
	struct amba_vin_wdr_shutter_gp_info *p_shutter_gp)
{
	u8 	data;
	u32 frame_length, fsc, rhs1, rhs2;
	u32 index, format_index;
	int shutter_long, shutter_short1, shutter_short2;
	int errCode = 0;
	struct imx123_info *pinfo;
	const struct imx123_pll_reg_table *pll_reg_table;

	pinfo		= (struct imx123_info *)src->pinfo;
	index = pinfo->current_video_index;
	format_index = imx123_video_info_table[index].format_index;
	pll_reg_table	= &imx123_pll_tbl[pinfo->pll_index];

	imx123_read_reg(src, IMX123_RHS1_HSB, &data);
	rhs1 = (data & 0x0F) << 16;
	imx123_read_reg(src, IMX123_RHS1_MSB, &data);
	rhs1 += data << 8;
	imx123_read_reg(src, IMX123_RHS1_LSB, &data);
	rhs1 += data;

	imx123_read_reg(src, IMX123_RHS2_HSB, &data);
	rhs2 = (data & 0x0F) << 16;
	imx123_read_reg(src, IMX123_RHS2_MSB, &data);
	rhs2 += data << 8;
	imx123_read_reg(src, IMX123_RHS2_LSB, &data);
	rhs2 += data;

	imx123_read_reg(src, IMX123_VMAX_HSB, &data);
	frame_length = (data & 0x0F) << 16;
	imx123_read_reg(src, IMX123_VMAX_MSB, &data);
	frame_length += data << 8;
	imx123_read_reg(src, IMX123_VMAX_LSB, &data);
	frame_length += data;

	/* long shutter */
	shutter_long = p_shutter_gp->long_shutter;
	/* short shutter 1 */
	shutter_short1 = p_shutter_gp->short1_shutter;
	/* short shutter 2 */
	shutter_short2 = p_shutter_gp->short2_shutter;

	/* shutter limitation check */
	if(pinfo->op_mode == IMX123_2X_WDR_MODE) {
		fsc = frame_length << 1; /* FSC = VMAX * 2 */
		if(fsc < shutter_long) {
			vin_err("long shutter row(%d) must be less than fsc(%d)!\n", shutter_long, fsc);
			return -EPERM;
		} else {
			shutter_long = fsc - shutter_long;
		}
		if(rhs1 < shutter_short1) {
			vin_err("short shutter row(%d) must be less than rhs(%d)!\n", shutter_short1, rhs1);
			return -EPERM;
		} else {
			shutter_short1 = rhs1 - shutter_short1;
		}
		vin_dbg("fsc:%d, shs1:%d, shs2:%d, rhs1:%d\n",
			fsc, shutter_short1, shutter_long, rhs1);

		/* short shutter check */
		if((shutter_short1 >= 7) && (shutter_short1 != 9) && (shutter_short1 != 10) &&
			(shutter_short1 < rhs1)  && (shutter_short1 != rhs1 - 3) && (shutter_short1 != rhs1 - 4)) {
			imx123_write_reg(src, IMX123_SHS1_LSB, (u8)(shutter_short1 & 0xFF));
			imx123_write_reg(src, IMX123_SHS1_MSB, (u8)(shutter_short1 >> 8));
			imx123_write_reg(src, IMX123_SHS1_HSB, (u8)((shutter_short1 >> 16) & 0xF));
		} else {
			vin_err("shs1 exceeds limitation! shs1:%d, rhs1:%d\n",
					shutter_short1, rhs1);
			return -EPERM;
		}
		/* long shutter check */
		if((shutter_long >= rhs1 + 7) && (shutter_long != rhs1 + 9) && (shutter_long != rhs1 + 10) &&
			(shutter_long < fsc)  && (shutter_long != fsc - 3) && (shutter_long != fsc - 4) &&
			(shutter_long != shutter_short1 + 9) && (shutter_long != fsc + shutter_short1 - 9)) {
			imx123_write_reg(src, IMX123_SHS2_LSB, (u8)(shutter_long & 0xFF));
			imx123_write_reg(src, IMX123_SHS2_MSB, (u8)(shutter_long >> 8));
			imx123_write_reg(src, IMX123_SHS2_HSB, (u8)((shutter_long >> 16) & 0xF));
		} else {
			vin_err("shs2 exceeds limitation! shs2:%d, shs1:%d, rhs1:%d, fsc:%d\n",
					shutter_long, shutter_short1, rhs1, fsc);
			return -EPERM;
		}

	} else if(pinfo->op_mode == IMX123_3X_WDR_MODE) {
		fsc = frame_length << 2; /* FSC = VMAX * 4 */
		if(fsc < shutter_long) {
			vin_err("long shutter row(%d) must be less than fsc(%d)!\n", shutter_long, fsc);
			return -EPERM;
		} else {
			shutter_long = fsc - shutter_long;
		}
		if(rhs1 < shutter_short1) {
			vin_err("short shutter 1 row(%d) must be less than rhs1(%d)!\n", shutter_short1, rhs1);
			return -EPERM;
		} else {
			shutter_short1 = rhs1 - shutter_short1;
		}
		if(rhs2 < shutter_short2) {
			vin_err("short shutter 2 row(%d) must be less than rhs2(%d)!\n", shutter_short2, rhs2);
			return -EPERM;
		} else {
			shutter_short2 = rhs2 - shutter_short2;
		}
		vin_dbg("fsc:%d, shs1:%d, shs2:%d, shs3:%d, rhs1:%d, rhs2:%d\n",
			fsc, shutter_short1, shutter_short2, shutter_long, rhs1, rhs2);

		/* short shutter 1 check */
		if((shutter_short1 >= 9) && (shutter_short1 != 13) && (shutter_short1 != 14) &&
			(shutter_short1 < rhs1)  && (shutter_short1 != rhs1 - 5) && (shutter_short1 != rhs1 - 6)) {
			imx123_write_reg(src, IMX123_SHS1_LSB, (u8)(shutter_short1 & 0xFF));
			imx123_write_reg(src, IMX123_SHS1_MSB, (u8)(shutter_short1 >> 8));
			imx123_write_reg(src, IMX123_SHS1_HSB, (u8)((shutter_short1 >> 16) & 0xF));
		} else {
			vin_err("shs1 exceeds limitation! shs1:%d, rhs1:%d\n",
					shutter_short1, rhs1);
			return -EPERM;
		}
		/* short shutter 2 check */
		if((shutter_short2 >= rhs1 + 9) && (shutter_short2 != rhs1 + 13) && (shutter_short2 != rhs1 + 14) &&
			(shutter_short2 < rhs2) && (shutter_short2 != rhs2 - 5) && (shutter_short2 != rhs2 - 6) &&
			(shutter_short2 != shutter_short1 + 13) && (shutter_short2 != shutter_long - 13)) {
			imx123_write_reg(src, IMX123_SHS2_LSB, (u8)(shutter_short2 & 0xFF));
			imx123_write_reg(src, IMX123_SHS2_MSB, (u8)(shutter_short2 >> 8));
			imx123_write_reg(src, IMX123_SHS2_HSB, (u8)((shutter_short2 >> 16) & 0xF));
		} else {
			vin_err("shs2 exceeds limitation! shs1:%d, shs2:%d, shs3:%d, rhs1:%d, rhs2:%d\n",
					shutter_short1, shutter_short2, shutter_long, rhs1, rhs2);
			return -EPERM;
		}
		/* long shutter check */
		if((shutter_long >= rhs2 + 9) && (shutter_long != rhs2 + 13) && (shutter_long != rhs2 + 14) &&
			(shutter_long < fsc) && (shutter_long != fsc - 5) && (shutter_long != fsc - 6) &&
			(shutter_long != shutter_short2 + 13) && (shutter_long != fsc + shutter_short1 - 13)) {
			imx123_write_reg(src, IMX123_SHS3_LSB, (u8)(shutter_long & 0xFF));
			imx123_write_reg(src, IMX123_SHS3_MSB, (u8)(shutter_long >> 8));
			imx123_write_reg(src, IMX123_SHS3_HSB, (u8)((shutter_long >> 16) & 0xF));
		} else {
			vin_err("shs3 exceeds limitation! shs1:%d, shs2:%d, shs3:%d, rhs2:%d, fsc:%d\n",
					shutter_short1, shutter_short2, shutter_long, rhs2, fsc);
			return -EPERM;
		}
	} else {
		vin_err("Non WDR mode can't support this API: %s!\n", __func__);
	}

	return errCode;
}

static int imx123_wdr_shutter2row(struct __amba_vin_source *src,
	struct amba_vin_wdr_shutter_gp_info *p_shutter2row)
{
	u64 exposure_time_q9;
	u8 	data;
	u32 line_length;
	u32 index, format_index;
	int errCode = 0;
	struct imx123_info *pinfo;
	const struct imx123_pll_reg_table *pll_reg_table;

	pinfo		= (struct imx123_info *)src->pinfo;
	index = pinfo->current_video_index;
	format_index = imx123_video_info_table[index].format_index;
	pll_reg_table	= &imx123_pll_tbl[pinfo->pll_index];

	imx123_read_reg(src, IMX123_HMAX_MSB, &data);
	line_length = data << 8;
	imx123_read_reg(src, IMX123_HMAX_LSB, &data);
	line_length += data;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

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

	return errCode;
}

static int imx123_set_operation_mode(struct __amba_vin_source *src, amba_vin_sensor_op_mode mode)
{
	int 			errCode = 0;
	struct imx123_info	*pinfo = (struct imx123_info *) src->pinfo;

	if((mode != IMX123_LINEAR_MODE) &&
		(mode != IMX123_2X_WDR_MODE) &&
			(mode != IMX123_3X_WDR_MODE)){
		vin_err("wrong opeartion mode, %d!\n", mode);
		errCode = -EPERM;
	} else {
		pinfo->op_mode = mode;
	}

	return errCode;
}

static int imx123_get_eis_info(struct __amba_vin_source *src, struct amba_vin_eis_info *eis_info)
{
	int errCode = 0;
	u32 index = -1;
	u32 format_index;
	struct imx123_info *pinfo;

	pinfo = (struct imx123_info *) src->pinfo;

	index = pinfo->current_video_index;
	if(index == -1){
		errCode = -EPERM;
		goto imx123_get_eis_info_exit;
	}
	format_index = imx123_video_info_table[index].format_index;

	memset(eis_info, 0, sizeof (struct amba_vin_eis_info));

	eis_info->cap_start_x = pinfo->cap_start_x;
	eis_info->cap_start_y = pinfo->cap_start_y;
	eis_info->cap_cap_w = pinfo->cap_cap_w;
	eis_info->cap_cap_h = pinfo->cap_cap_h;
	eis_info->source_width = imx123_video_format_tbl.table[format_index].width;
	eis_info->source_height = imx123_video_format_tbl.table[format_index].height;
	eis_info->current_fps = pinfo->frame_rate;
	eis_info->main_fps = imx123_video_format_tbl.table[format_index].auto_fps;
	eis_info->current_shutter_time = pinfo->current_shutter_time;
	eis_info->sensor_cell_width = 250;/* 2.5 um */
	eis_info->sensor_cell_height = 250;/* 2.5 um */
	eis_info->column_bin = 1;
	eis_info->row_bin = 1;

	imx123_get_vb_lines(&(pinfo->source), &(eis_info->vb_lines));
	imx123_get_row_time(&(pinfo->source), &(eis_info->row_time));

imx123_get_eis_info_exit:
	return errCode;
}

int imx123_get_eis_info_ex(struct amba_vin_eis_info * eis_info)
{
	int errCode = 0;
	errCode = imx123_get_eis_info(NULL, eis_info);
	return errCode;
}
EXPORT_SYMBOL(imx123_get_eis_info_ex);

static int imx123_wdr_get_win_offset(struct __amba_vin_source *src,
	struct amba_vin_wdr_win_offset *p_win_offset)
{
	int errCode = 0;
	u32 format_index;
	struct imx123_info	*pinfo = (struct imx123_info *) src->pinfo;

	if (pinfo->op_mode == IMX123_LINEAR_MODE) {
		vin_warn("%s should be called by hdr mode\n", __func__);
		return -EPERM;
	}

	format_index = imx123_video_info_table[pinfo->current_video_index].format_index;
	memcpy(p_win_offset,
		&imx123_video_format_tbl.table[format_index].hdr_win_offset,
		sizeof(struct amba_vin_wdr_win_offset));

	return errCode;
}

static int imx123_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct imx123_info		*pinfo;

	pinfo = (struct imx123_info *) src->pinfo;

	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		imx123_reset(src);
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
		errCode = imx123_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO: {
		struct amba_vin_source_info *pub_src;

		pub_src = (struct amba_vin_source_info *) args;
		pub_src->id = src->id;
		if (dual_gain)
			pub_src->sensor_id = SENSOR_IMX123_DCG;
		else
			pub_src->sensor_id = SENSOR_IMX123;
		pub_src->adapter_id = adapter_id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = imx123_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = imx123_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = imx123_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = imx123_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = imx123_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = imx123_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = imx123_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = imx123_set_still_mode(src, (struct amba_vin_src_still_info *) args);
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
		errCode = imx123_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH:
		errCode=imx123_shutter_time2width(src, (u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = imx123_set_agc_db(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(s32*)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = imx123_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = imx123_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = imx123_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = imx123_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		imx123_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID: {
		u16 sen_id = 0;
		u16 sen_ver = 0;
		u32 *pdata = (u32 *) args;

		errCode = imx123_query_sensor_id(src, &sen_id);
		if (errCode)
			goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;
		errCode = imx123_query_sensor_version(src, &sen_ver);
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

		errCode = imx123_read_reg(src, subaddr, &data);

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

		errCode = imx123_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = imx123_set_slowshutter_mode(src, *(int *) args);
		break;

	case AMBA_VIN_SRC_GET_EIS_INFO:
		errCode = imx123_get_eis_info(src, (struct amba_vin_eis_info *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_SHUTTER_GROUP:
		errCode = imx123_set_wdr_shutter_group(src,
			(struct amba_vin_wdr_shutter_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_SHUTTER_ROW_GROUP:
		errCode = imx123_set_wdr_shutter_row_group(src,
			(struct amba_vin_wdr_shutter_gp_info *) args);
		break;

	case AMBA_VIN_SRC_WDR_SHUTTER2ROW:
		errCode = imx123_wdr_shutter2row(src,
			(struct amba_vin_wdr_shutter_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_INDEX:
		errCode = imx123_set_agc_index(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX:
		errCode = imx123_set_agc_index(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_DGAIN_INDEX_GROUP:
		errCode = imx123_set_wdr_dgain_index_group(src,
			(struct amba_vin_wdr_gain_gp_info *) args);
		break;

	case AMBA_VIN_SRC_SET_OPERATION_MODE:
		errCode = imx123_set_operation_mode(src, *(amba_vin_sensor_op_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX_GROUP:
		errCode = imx123_set_wdr_again_index_group(src,
			(struct amba_vin_wdr_gain_gp_info *) args);
		break;

	case AMBA_VIN_SRC_GET_WDR_WIN_OFFSET:
		errCode = imx123_wdr_get_win_offset(src,
			(struct amba_vin_wdr_win_offset *) args);
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
