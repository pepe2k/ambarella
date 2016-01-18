/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx078/imx078_docmd.c
 *
 * History:
 *    2013/03/11 - [Bingliang Hu] Create
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

static void imx078_fill_video_format_regs(struct __amba_vin_source *src)
{
	int			i;
	u32			index;
	u32			format_index;
	struct imx078_info	*pinfo;

	pinfo = (struct imx078_info *)src->pinfo;

	vin_dbg("imx078_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = imx078_video_info_table[index].format_index;

	for (i = 0; i < imx078_VIDEO_FORMAT_REG_NUM; i++) {
		if (imx078_video_format_tbl.reg[i] == 0)
			break;
		imx078_write_reg(src,
			imx078_video_format_tbl.reg[i],
			imx078_video_format_tbl.table[format_index].data[i]);
	}

	if (imx078_video_format_tbl.table[format_index].ext_reg_fill)
		imx078_video_format_tbl.table[format_index].ext_reg_fill(src);
}

static void imx078_fill_share_regs(struct __amba_vin_source *src)
{
	int				i;
	const struct imx078_reg_table	*reg_tbl;

	reg_tbl = imx078_share_regs;
	vin_dbg("imx078 fill share regs\n");
	for (i = 0; i < imx078_SHARE_REG_SIZE; i++) {
		vin_dbg("imx078 write reg %d\n", i);
		imx078_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void imx078_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
}

static int imx078_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct imx078_info  *pinfo;

	pinfo = (struct imx078_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int imx078_get_video_info( struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int	errCode = 0;
	u32	index;
	u32	format_index;
	struct imx078_info *pinfo;

	pinfo = (struct imx078_info *)src->pinfo;

	index = pinfo->current_video_index;

	if (index >= imx078_VIDEO_INFO_TABLE_SIZE) {
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
		format_index = imx078_video_info_table[index].format_index;

		p_video_info->width = imx078_video_info_table[index].def_width;
		p_video_info->height = imx078_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = imx078_video_format_tbl.table[format_index].format;
		p_video_info->type = imx078_video_format_tbl.table[format_index].type;
		p_video_info->bits = imx078_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = imx078_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

static int imx078_get_agc_info( struct __amba_vin_source *src, amba_vin_agc_info_t *p_agc_info)
{
	int errCode = 0;
	struct imx078_info *pinfo;

	pinfo = (struct imx078_info *) src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}

static int imx078_get_shutter_info( struct __amba_vin_source *src, amba_vin_shutter_info_t *pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof(amba_vin_shutter_info_t));

	return 0;
}

static int imx078_check_video_mode( struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int				errCode = 0;
	int				i;
	u32				index;
	u32				format_index;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof(p_mode_info->video_info));


	for (i = 0; i < imx078_VIDEO_MODE_TABLE_SIZE; i++) {
		if (imx078_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = imx078_VIDEO_MODE_TABLE_AUTO;
			p_mode_info->is_supported = 1;

			index = imx078_video_mode_table[i].still_index;
			format_index = imx078_video_info_table[index].format_index;

			p_mode_info->video_info.width = imx078_video_info_table[index].def_width;
			p_mode_info->video_info.height = imx078_video_info_table[index].def_height;
			p_mode_info->video_info.fps = imx078_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format = imx078_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = imx078_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = imx078_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = imx078_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;

			break;
		}
	}

	return errCode;
}

static int imx078_set_still_mode( struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{
	return 0;
}

static int imx078_set_low_light_agc( struct __amba_vin_source *src, u32 agc_index)
{
	return 0;
}

static int imx078_set_shutter_time( struct __amba_vin_source *src, u32 shutter_time)
{
	return 0;
}

static int imx078_set_agc_db( struct __amba_vin_source *src, s32 agc_db)
{
	int errCode = 0;

	return errCode;
}

static int imx078_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int errCode = 0;
	struct imx078_info *pinfo;

	pinfo = (struct imx078_info *) src->pinfo;

	pinfo->bayer_pattern = mirror_mode->bayer_pattern;

	return errCode;
}

static int imx078_set_fps( struct __amba_vin_source *src, int fps)
{
	int errCode = 0;

	u32 index;
	u32 format_index;
	u64 vertical_lines;
	struct imx078_info *pinfo;
	struct amba_vin_irq_fix		vin_irq_fix;
	struct amba_vin_HV	master_HV;

	pinfo = (struct imx078_info *)src->pinfo;
	index = pinfo->current_video_index;

	if (index >= imx078_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx078_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto imx078_set_fps_exit;
	}

	format_index = imx078_video_info_table[index].format_index;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = imx078_video_format_tbl.table[format_index].auto_fps;
	if(fps < imx078_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, imx078_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto imx078_set_fps_exit;
	}

	/* Slave sensor, adjust VB time from DSP */
	master_HV.pel_clk_a_line = (imx078_video_format_tbl.table[format_index].xhs_clk * 4 * 2);
	if(master_HV.pel_clk_a_line == 0) {
		vin_err("line length is 0!\n");
		return -EIO;
	}
	vertical_lines = (u64)imx078_pll_tbl[pinfo->pll_index].pixclk * fps;

	DO_DIV_ROUND(vertical_lines, master_HV.pel_clk_a_line);
	DO_DIV_ROUND(vertical_lines, 512000000);
	master_HV.line_num_a_field = vertical_lines;
	pinfo->xvs_num[format_index] = master_HV.line_num_a_field;
	imx078_set_master_HV(src, master_HV);

	imx078_set_shutter_time(src, pinfo->current_shutter_time);// keep the same shutter time
	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	vin_irq_fix.delay = 0;// change to zero for all non Aptina Hi-SPI sensors (for A5s/A7/S2)

	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

imx078_set_fps_exit:
	return errCode;
}

static void imx078_start_streaming(struct __amba_vin_source *src)
{
	imx078_write_reg(src, imx078_REG_00, 0x00); //cancel standby, enter normal mode
}
static void imx078_stop_streaming(struct __amba_vin_source *src)
{
	imx078_write_reg(src, imx078_REG_00, 0x02); //standby except pll
}

static int imx078_set_video_index( struct __amba_vin_source *src, u32 index)
{
	int				errCode = 0;
	struct imx078_info		*pinfo;
	u32				format_index;

	pinfo = (struct imx078_info *)src->pinfo;

	if (index >= imx078_VIDEO_INFO_TABLE_SIZE) {
		vin_err("imx078_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto imx078_set_mode_exit;
	}

	errCode |= imx078_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = imx078_video_info_table[index].format_index;

	pinfo->cap_start_x 	= imx078_video_info_table[index].def_start_x;
	pinfo->cap_start_y 	= imx078_video_info_table[index].def_start_y;
	pinfo->cap_cap_w 	= imx078_video_info_table[index].def_width;
	pinfo->cap_cap_h 	= imx078_video_info_table[index].def_height;
	pinfo->slvs_eav_col = imx078_video_info_table[index].slvs_eav_col;
	pinfo->slvs_sav2sav_dist = imx078_video_info_table[index].slvs_sav2sav_dist;
	pinfo->bayer_pattern 	= imx078_video_info_table[index].bayer_pattern;
	pinfo->pll_index 	= imx078_video_format_tbl.table[format_index].pll_index;

	imx078_print_info(src);

	//set clk_si
	errCode |= imx078_init_vin_clock(src, &imx078_pll_tbl[pinfo->pll_index]);

	errCode |= imx078_set_vin_mode(src);
	imx078_fill_video_format_regs(src);
	imx078_set_fps(src, AMBA_VIDEO_FPS_AUTO);
	imx078_set_shutter_time(src, AMBA_VIDEO_FPS_60);

	errCode |= imx078_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

	/* TG reset release ( Enable Streaming )*/
	AMBA_VIN_HW_RESET();

	imx078_stop_streaming(src);
	msleep(10);
	imx078_start_streaming(src);
	imx078_fill_share_regs(src);


imx078_set_mode_exit:
	return errCode;
}

static int imx078_set_video_mode( struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int				errCode = -EINVAL;
	int				i;
	struct imx078_info		*pinfo;

	static int			first_set_video_mode = 1;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct imx078_info *)src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errCode = imx078_init_vin_clock(src, &imx078_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto imx078_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	imx078_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	if (mode == AMBA_VIDEO_MODE_AUTO) {
		mode = imx078_VIDEO_MODE_TABLE_AUTO;
	}

	for (i = 0; i < imx078_VIDEO_MODE_TABLE_SIZE; i++) {
		if (imx078_video_mode_table[i].mode == mode) {
			errCode = imx078_set_video_index(src, imx078_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= imx078_VIDEO_MODE_TABLE_SIZE) {
		vin_err("imx078_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = imx078_video_mode_table[i].mode;
		pinfo->mode_type = imx078_video_mode_table[i].preview_mode_type;
	}

imx078_set_video_mode_exit:
	return errCode;
}

static int imx078_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	struct imx078_info		*pinfo;

	pinfo = (struct imx078_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = imx078_init_vin_clock(src, &imx078_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto imx078_init_hw_exit;
	msleep(10);
	imx078_reset(src);

imx078_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int imx078_docmd( struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int				errCode = 0;
	struct imx078_info		*pinfo;

	pinfo = (struct imx078_info *) src->pinfo;

	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		imx078_reset(src);
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
		errCode = imx078_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO: {
		struct amba_vin_source_info	*pub_src;

		pub_src = (struct amba_vin_source_info *)args;
		pub_src->id = src->id;
		pub_src->adapter_id = adapter_id;
		pub_src->dev_type = src->dev_type;
		strlcpy(pub_src->name, src->name, sizeof(pub_src->name));
		pub_src->sensor_id = SENSOR_IMX078;
		pub_src->default_mode = imx078_VIDEO_MODE_TABLE_AUTO;
		pub_src->total_channel_num = src->total_channel_num;
		pub_src->active_channel_id = src->active_channel_id;
		pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = imx078_check_video_mode(src, (struct amba_vin_source_mode_info *)args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = imx078_get_video_info(src, (struct amba_video_info *)args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = imx078_get_agc_info(src, (amba_vin_agc_info_t *)args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = imx078_get_shutter_info(src, (amba_vin_shutter_info_t *)args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = imx078_get_capability(src, (struct amba_vin_src_capability *)args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = imx078_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = imx078_set_video_mode(src, *(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = imx078_set_still_mode(src, (struct amba_vin_src_still_info *)args);
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
		errCode = imx078_set_fps(src, *(u32 *)args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = imx078_set_shutter_time(src, *(u32 *)args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(u32 *)args = pinfo->current_gain_db;
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = imx078_set_agc_db(src, *(s32 *)args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = imx078_set_low_light_agc(src, *(u32 *)args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = imx078_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
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

		errCode = imx078_read_reg(src, subaddr, &data);

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

		errCode = imx078_write_reg(src, subaddr, data);
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