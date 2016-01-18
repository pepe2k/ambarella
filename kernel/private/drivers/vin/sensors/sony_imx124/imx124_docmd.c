/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx124/imx124_docmd.c
 *
 * History:
 *    2014/07/23 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

static void imx124_dump_reg(struct __amba_vin_source *src)
{
	u32 i;
	u16 reg_to_dump_init[] = {};

	for(i=0; i<sizeof(reg_to_dump_init)/sizeof(reg_to_dump_init[0]); i++) {
		u16 reg_addr = reg_to_dump_init[i];
		u8 reg_val;

		imx124_read_reg(src, reg_addr, &reg_val);
		vin_dbg("REG= 0x%04X, 0x%04X\n", reg_addr, reg_val);
	}

}

static void imx124_fill_video_format_regs(struct __amba_vin_source *src)
{
	int			i;
	u32			index;
	u32			format_index;
	struct imx124_info	*pinfo;

	pinfo = (struct imx124_info *)src->pinfo;

	vin_dbg("imx124_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = imx124_video_info_table[index].format_index;

	for (i = 0; i < IMX124_VIDEO_FORMAT_REG_NUM; i++) {
		if (imx124_video_format_tbl.reg[i] == 0)
			break;

		imx124_write_reg(src,
			imx124_video_format_tbl.reg[i],
			imx124_video_format_tbl.table[format_index].data[i]);
	}

	if (imx124_video_format_tbl.table[format_index].ext_reg_fill)
		imx124_video_format_tbl.table[format_index].ext_reg_fill(src);

}

static void imx124_fill_share_regs(struct __amba_vin_source *src)
{
	int i;
	const struct imx124_reg_table	*reg_tbl;

	reg_tbl = imx124_share_regs;
	vin_dbg("imx124 fill share regs\n");
	for (i = 0; i < IMX124_SHARE_REG_SIZE; i++) {
		imx124_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void imx124_sw_reset(struct __amba_vin_source *src)
{
	imx124_write_reg(src, IMX124_STANDBY, 0x1);//STANDBY
	imx124_write_reg(src, IMX124_SWRESET, 0x1);
	msleep(30);
}

static void imx124_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	imx124_sw_reset(src);
}

static void imx124_fill_pll_regs(struct __amba_vin_source *src)
{
	int i = 0;
	struct imx124_info *pinfo;
	const struct imx124_pll_reg_table 	*pll_reg_table;
	const struct imx124_reg_table 		*pll_tbl;

	pinfo = (struct imx124_info *) src->pinfo;

	vin_dbg("imx124_fill_pll_regs\n");
	pll_reg_table = &imx124_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < IMX124_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			imx124_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
}

static int imx124_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct imx124_info  *pinfo;

	pinfo = (struct imx124_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

#if 0
static int imx124_get_vblank_time( struct __amba_vin_source *src, u32 *ptime)
{
	u32					frame_length_lines, active_lines, format_index;
	u64					v_btime;
	u32					line_length;
	int					errCode = 0;
	int					mode_index;
	u8					data_val;

	struct imx124_info 			*pinfo;
	const struct imx124_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct imx124_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= imx124_video_info_table[mode_index].format_index;
	pll_reg_table	= &imx124_pll_tbl[pinfo->pll_index];

	errCode |= imx124_read_reg(src, IMX124_VMAX_HSB, &data_val);
	frame_length_lines = (u32)(data_val & 0x01) <<  16;
	errCode |= imx124_read_reg(src, IMX124_VMAX_MSB, &data_val);
	frame_length_lines += (u32)(data_val) <<  8;
	errCode |= imx124_read_reg(src, IMX124_VMAX_LSB, &data_val);
	frame_length_lines += (u32)(data_val);
	BUG_ON(frame_length_lines == 0);

	errCode |= imx124_read_reg(src, IMX124_HMAX_MSB, &data_val);
	line_length = (u32)(data_val & 0x0f) <<  8;
	errCode |= imx124_read_reg(src, IMX124_HMAX_LSB, &data_val);
	line_length += (u32)data_val;
	BUG_ON(line_length == 0);

	active_lines = imx124_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pll_reg_table->pixclk); //ns
	*ptime = v_btime;

	vin_dbg("imx124_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return 0;
}
#endif

static int imx124_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errCode = 0;
	u32 index;
	u32 format_index;

	struct imx124_info *pinfo;

	pinfo = (struct imx124_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= IMX124_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = imx124_video_info_table[index].format_index;
		p_video_info->width = imx124_video_info_table[index].act_width;
		p_video_info->height = imx124_video_info_table[index].act_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = imx124_video_format_tbl.table[format_index].format;
		p_video_info->type = imx124_video_format_tbl.table[format_index].type;
		p_video_info->bits = imx124_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = imx124_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

static int imx124_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct imx124_info *pinfo;

	pinfo = (struct imx124_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}

static int imx124_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));
	return 0;
}

static int imx124_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;
	struct imx124_info *pinfo;

	pinfo = (struct imx124_info *) src->pinfo;
	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < IMX124_VIDEO_MODE_TABLE_SIZE; i++) {
		if (imx124_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = IMX124_VIDEO_MODE_TABLE_AUTO;

			p_mode_info->is_supported = 1;

			index = imx124_video_mode_table[i].still_index;
			format_index = imx124_video_info_table[index].format_index;

			p_mode_info->video_info.width = imx124_video_info_table[index].act_width;
			p_mode_info->video_info.height = imx124_video_info_table[index].act_height;
			p_mode_info->video_info.fps = imx124_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = imx124_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = imx124_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = imx124_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = imx124_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;

			break;
		}
	}

	return errCode;
}

static int imx124_query_sensor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	int 	errCode = 0;
	return errCode;
}

static int imx124_query_sensor_version(struct __amba_vin_source *src, u16 * ver)
{
	/* >> TODO << */
	return 0;
}

static int imx124_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length;
	u8 	shr_width_h, shr_width_m, shr_width_l;
	int	shutter_width;

	const struct imx124_pll_reg_table *pll_table;
	struct imx124_info		*pinfo;
	pinfo = (struct imx124_info *)src->pinfo;

	pll_table = &imx124_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time;

	imx124_read_reg(src, IMX124_VMAX_HSB, &shr_width_h);
	imx124_read_reg(src, IMX124_VMAX_MSB, &shr_width_m);
	imx124_read_reg(src, IMX124_VMAX_LSB, &shr_width_l);

	frame_length = ((shr_width_h&0x01)<<16) + (shr_width_m<<8) + shr_width_l;

	imx124_read_reg(src, IMX124_HMAX_MSB, &shr_width_m);
	imx124_read_reg(src, IMX124_HMAX_LSB, &shr_width_l);

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

	imx124_write_reg(src, IMX124_SHS1_HSB, shr_width_h);
	imx124_write_reg(src, IMX124_SHS1_MSB, shr_width_m);
	imx124_write_reg(src, IMX124_SHS1_LSB, shr_width_l);

	vin_dbg("frame_length: %d, sweep lines:%d\n", frame_length, frame_length - shutter_width);
	pinfo->current_shutter_time = shutter_time;
	return 0;
}

static int imx124_shutter_time2width(struct __amba_vin_source *src, u32* shutter_time)
{
	u64 	exposure_time_q9;
	u32 	frame_length, line_length;
	u8 	shr_width_h, shr_width_m, shr_width_l;
	int	shutter_width;

	const struct imx124_pll_reg_table *pll_table;
	struct imx124_info		*pinfo;
	pinfo = (struct imx124_info *)src->pinfo;

	pll_table = &imx124_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = *shutter_time;

	imx124_read_reg(src, IMX124_VMAX_HSB, &shr_width_h);
	imx124_read_reg(src, IMX124_VMAX_MSB, &shr_width_m);
	imx124_read_reg(src, IMX124_VMAX_LSB, &shr_width_l);

	frame_length = ((shr_width_h&0x01)<<16) + (shr_width_m<<8) + shr_width_l;

	imx124_read_reg(src, IMX124_HMAX_MSB, &shr_width_m);
	imx124_read_reg(src, IMX124_HMAX_LSB, &shr_width_l);

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

static int imx124_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int					errCode = 0;
	u32					frame_time = 0;
	u64					frame_time_pclk;
	u32					vertical_lines = 0;
	u32					line_length = 0;
	u32					index;
	u32					format_index;
	u8					data_val;
	const struct imx124_pll_reg_table 	*pll_table;
	struct imx124_info			*pinfo;
	struct amba_vin_irq_fix			vin_irq_fix;

	pinfo = (struct imx124_info *)src->pinfo;
	index = pinfo->current_video_index;

	if (index >= IMX124_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx124_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto imx124_set_fps_exit;
	}

	format_index = imx124_video_info_table[index].format_index;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = imx124_video_format_tbl.table[format_index].auto_fps;
	if(fps < imx124_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, imx124_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto imx124_set_fps_exit;
	}

	frame_time = fps;

	/* ToDo: Add specified PLL index*/

	pll_table = &imx124_pll_tbl[pinfo->pll_index];

	errCode |= imx124_read_reg(src, IMX124_HMAX_MSB, &data_val);
	line_length = (u32)(data_val) <<  8;
	errCode |= imx124_read_reg(src, IMX124_HMAX_LSB, &data_val);
	line_length += (u32)data_val;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	frame_time_pclk = frame_time * (u64)pll_table->pixclk;

	vin_dbg("pixclk %d, frame_time_pclk 0x%llx\n", pll_table->pixclk, frame_time_pclk);

	DO_DIV_ROUND(frame_time_pclk, line_length);
	DO_DIV_ROUND(frame_time_pclk, 512000000);

	vertical_lines = frame_time_pclk;

	vin_dbg("vertical_lines is  0x%05x\n", vertical_lines);
	errCode |= imx124_write_reg(src, IMX124_VMAX_HSB, (u8)((vertical_lines & 0x010000) >> 16));
	errCode |= imx124_write_reg(src, IMX124_VMAX_MSB, (u8)((vertical_lines & 0x00FF00) >> 8));
	errCode |= imx124_write_reg(src, IMX124_VMAX_LSB, (u8)(vertical_lines & 0x0000FF));

	imx124_set_shutter_time(src, pinfo->current_shutter_time);// keep the same shutter time

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_irq_fix.delay = 0;// change to zero for all non Aptina Hi-SPI sensors (for A5s/A7/S2)
	/*
	errCode = imx124_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode)
		goto imx124_set_fps_exit;
	*/
	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

imx124_set_fps_exit:
	return errCode;
}

static void imx124_start_streaming(struct __amba_vin_source *src)
{
	imx124_write_reg(src, IMX124_XMSTA, 0x00); //master mode start
	imx124_write_reg(src, IMX124_STANDBY, 0x00); //cancel standby
}

static int imx124_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct imx124_info *pinfo;
	u32 format_index;

	pinfo = (struct imx124_info *) src->pinfo;

	if (index >= IMX124_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx124_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto imx124_set_mode_exit;
	}

	errCode |= imx124_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = imx124_video_info_table[index].format_index;

	pinfo->cap_start_x = imx124_video_info_table[index].def_start_x;
	pinfo->cap_start_y = imx124_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = imx124_video_info_table[index].def_width;
	pinfo->cap_cap_h = imx124_video_info_table[index].def_height;
	pinfo->act_start_x = imx124_video_info_table[index].act_start_x;
	pinfo->act_start_y = imx124_video_info_table[index].act_start_y;
	pinfo->act_width = imx124_video_info_table[index].act_width;
	pinfo->act_height = imx124_video_info_table[index].act_height;
	pinfo->slvs_eav_col = imx124_video_info_table[index].slvs_eav_col;
	pinfo->slvs_sav2sav_dist = imx124_video_info_table[index].slvs_sav2sav_dist;
	pinfo->bayer_pattern = imx124_video_info_table[index].bayer_pattern;
	//set the specified PLL index for each sensor_mode.
	pinfo->pll_index = imx124_video_format_tbl.table[format_index].pll_index;
	imx124_print_info(src);

	//set clk_si
	errCode |= imx124_init_vin_clock(src, &imx124_pll_tbl[pinfo->pll_index]);
	errCode |= imx124_set_vin_mode(src);

	imx124_fill_pll_regs(src);
	imx124_fill_share_regs(src);
	imx124_fill_video_format_regs(src);

	imx124_set_fps(src, AMBA_VIDEO_FPS_AUTO);
	imx124_set_shutter_time(src, AMBA_VIDEO_FPS_60);

	errCode |= imx124_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

	imx124_start_streaming(src);

imx124_set_mode_exit:
	return errCode;
}

static int imx124_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errCode = -EINVAL;
	int i;
	struct imx124_info *pinfo;
	int errorCode = 0;
	static int 				first_set_video_mode = 1;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct imx124_info *) src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errorCode = imx124_init_vin_clock(src, &imx124_pll_tbl[pinfo->pll_index]);
		if (errorCode)
			goto imx124_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	imx124_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	for (i = 0; i < IMX124_VIDEO_MODE_TABLE_SIZE; i++) {
		if (imx124_video_mode_table[i].mode == mode) {
			errCode = imx124_set_video_index(src, imx124_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}

	if (i >= IMX124_VIDEO_MODE_TABLE_SIZE) {
		vin_err("imx124_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = imx124_video_mode_table[i].mode;
		pinfo->mode_type = imx124_video_mode_table[i].preview_mode_type;
	}

imx124_set_video_mode_exit:
	return errCode;
}

static int imx124_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	struct imx124_info		*pinfo;

	pinfo = (struct imx124_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = imx124_init_vin_clock(src, &imx124_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto imx124_init_hw_exit;

	msleep(10);
	imx124_reset(src);

imx124_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int imx124_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{
	return 0;
}

static int imx124_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
	/* >> TODO, for RGB mode only << */
	return 0;
}

static int imx124_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	int errCode = 0;
	u16 reg_d;
	struct imx124_info *pinfo;
	u32 gain_index;
	s32 db_max;
	s32 db_step;

	pinfo = (struct imx124_info *) src->pinfo;

	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	vin_dbg("imx124_set_agc: 0x%x\n", agc_db);

	gain_index = (db_max - agc_db) / db_step;

	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		reg_d = IMX124_GAIN_MAX_DB - gain_index;
		imx124_write_reg(src, IMX124_GAIN_LSB, (u8)(reg_d&0xFF));
		imx124_write_reg(src, IMX124_GAIN_MSB, (u8)(reg_d>>8));
		pinfo->current_gain_db = agc_db;
	} else {
		errCode = -1;	/* Index is out of range */
	}
	return errCode;
}

static int imx124_set_agc_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct imx124_info *pinfo;

	vin_dbg("imx124_set_agc_index: %d\n", index);

	pinfo = (struct imx124_info *) src->pinfo;

	if ((index >= pinfo->min_agc_index) && (index <= pinfo->max_agc_index)) {
		imx124_write_reg(src, IMX124_GAIN_LSB, (u8)(index&0xFF));
		imx124_write_reg(src, IMX124_GAIN_MSB, (u8)(index>>8));
	} else {
		errCode = -1;	/* Index is out of range */
	}
	return errCode;
}

static int imx124_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int		errCode = 0;
	u32		readmode;
	struct	imx124_info			*pinfo;
	u32		target_bayer_pattern;
	u16		tmp_reg;

	pinfo = (struct imx124_info *) src->pinfo;

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
		readmode = IMX124_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	case AMBA_VIN_SRC_MIRROR_NONE:
		readmode = 0;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX124_H_MIRROR | IMX124_V_FLIP;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		readmode = IMX124_H_MIRROR;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_RG;
		break;

	default:
		vin_err("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	errCode |= imx124_read_reg(src, IMX124_WINMODE, (u8 *)&tmp_reg);
	tmp_reg &= ~(IMX124_H_MIRROR | IMX124_V_FLIP);
	tmp_reg |= readmode;
	errCode |= imx124_write_reg(src, IMX124_WINMODE, tmp_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}

static int imx124_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
	/* >> TODO, for YUV output mode only << */
	return errCode;
}

static int imx124_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;
	/* >> TODO, for RGB raw output mode only << */
	/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}

static int imx124_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct imx124_info		*pinfo;

	pinfo = (struct imx124_info *) src->pinfo;

	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		imx124_reset(src);
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
		errCode = imx124_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO: {
		struct amba_vin_source_info *pub_src;

		pub_src = (struct amba_vin_source_info *) args;
		pub_src->id = src->id;
		pub_src->sensor_id = SENSOR_IMX124;
		pub_src->adapter_id = adapter_id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = imx124_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = imx124_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = imx124_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = imx124_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = imx124_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = imx124_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = imx124_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = imx124_set_still_mode(src, (struct amba_vin_src_still_info *) args);
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
		errCode = imx124_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH:
		errCode=imx124_shutter_time2width(src, (u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = imx124_set_agc_db(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(s32*)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = imx124_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = imx124_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = imx124_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = imx124_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		imx124_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID: {
		u16 sen_id = 0;
		u16 sen_ver = 0;
		u32 *pdata = (u32 *) args;

		errCode = imx124_query_sensor_id(src, &sen_id);
		if (errCode)
			goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;
		errCode = imx124_query_sensor_version(src, &sen_ver);
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

		errCode = imx124_read_reg(src, subaddr, &data);

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

		errCode = imx124_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = imx124_set_slowshutter_mode(src, *(int *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_INDEX:
		errCode = imx124_set_agc_index(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX:
		errCode = imx124_set_agc_index(src, *(u32 *) args);
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
