/*
 * kernel/private/drivers/ambarella/vin/decoders/adv7619/adv7619_docmd.c
 *
 * History:
 *    2009/07/23 - [Qiao Wang] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
// #undef vin_dbg
//#define vin_dbg printk
#if 0
static void adv7619_dump_reg(struct __amba_vin_source *src, const struct adv7619_reg_table regtable[]){
	u32 i;
	u8 reg;
	for (i = 0;; i++) {
		if ((regtable[i].data == 0xff) && (regtable[i].reg == 0xff))
			break;
		adv7619_read_reg(src, regtable[i].regmap, regtable[i].reg, &reg);
		DRV_PRINT("reg 0x%x map %d = 0x%x\n",regtable[i].reg,regtable[i].regmap,reg);
	}

}

static int adv7619_check_status_analog(struct __amba_vin_source *src)
{
	int errCode = 0;

	return errCode;
}

static int adv7619_check_status_cp(struct __amba_vin_source *src)
{
	int errCode = 0;

	return errCode;
}

static int adv7619_check_status_hdmi_infoframe(struct __amba_vin_source *src)
{
	int errCode = 0;

	return errCode;
}

static int adv7619_check_status_hdmi_resolution(struct __amba_vin_source *src)
{
	int errCode = 0;

	return errCode;
}

static int adv7619_check_status_hdmi(struct __amba_vin_source *src)
{
	int errCode = 0;

	return errCode;
}

static int adv7619_init_interrupts(struct __amba_vin_source *src)
{
	int errCode = 0;
	u8 regdata;

	/*Initiate int mask1 */
	regdata = 0xff;
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_INT1MSK1, regdata);
	/*Reset int clear1 */
	regdata = 0xff;
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_INT1CLR1, regdata);
	regdata = 0x00;
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_INT1CLR1, regdata);

	/*Initiate int mask2 */
	regdata = 0xff;
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_INT1MSK2, regdata);
	/*Reset int clear2 */
	regdata = 0xff;
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_INT1CLR2, regdata);
	regdata = 0x00;
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_INT1CLR2, regdata);

	/*Initiate int mask3 */
	regdata = 0xff;
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_INT1MSK3, regdata);
	/*Reset int clear3 */
	regdata = 0xff;
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_INT1CLR3, regdata);
	regdata = 0x00;
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_INT1CLR3, regdata);

	/*HDMI interrupt */
	regdata = 0x10;
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_HDMIINT2MSK2, regdata);
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_HDMIINT2MSK1, regdata);
	regdata = 0xff;
	errCode = adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_HDMIINTSTUS2, regdata);

	return errCode;
}

static void adv7619_sw_reset(struct __amba_vin_source *src, int force_init)
{
	//int errCode = 0;
	//u32 i;
	struct adv7619_info *pinfo;
	//u8 reg;

	pinfo = (struct adv7619_info *) src->pinfo;

	if (force_init) {
		//Deassert HPD...
		ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_hdmi_hpd, 0);
		msleep(ADV7619_RETRY_SLEEP_MS);
	}

	adv7619_write_reg(src, USER_MAP, ADV7619_REG_I2C_RST, 0X80);


	if (force_init) {
		//Assert HPD...
		ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_hdmi_hpd, 1);
	}

	msleep(ADV7619_RETRY_SLEEP_MS);
}
#endif
static void adv7619_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	msleep(1000);
//	adv7619_sw_reset(src, 1);
}

static void adv7619_fill_share_regs(struct __amba_vin_source *src, const struct adv7619_reg_table regtable[])
{
	u32 i;

	for (i = 0;; i++) {
		if ((regtable[i].data == 0xff) && (regtable[i].reg == 0xff))
			break;
		adv7619_write_reg(src, regtable[i].regmap, regtable[i].reg, regtable[i].data);
		msleep(50);
		if((regtable[i].reg == ADV7619_REG_I2C_RST)
		&& (regtable[i].regmap == USER_MAP)
		&& (regtable[i].data == 0x80))
			msleep(1000);
	}
}

static int adv7619_select_source(struct __amba_vin_source *src)
{
	int errCode = 0;
#if 0
	u8 regdata;
	u32 source_id;
	struct adv7619_src_info *psrcinfo;

	psrcinfo = (struct adv7619_src_info *) src->psrcinfo;
	source_id = adv7619_source_table[psrcinfo->cap_src_id].source_id;

	adv7619_sw_reset(src, 0);

	if (source_id < ADV7619_HDMI_MIN_ID) {
		errCode = adv7619_read_reg(src, USER_MAP, ADV7619_REG_INPUTCTRL, &regdata);
		if (errCode)
			goto adv7619_select_channel_exit;

		regdata &= ~ADV7619_INPUTCTRL_INSEL_MASK;
		regdata |= source_id;
		adv7619_write_reg(src, USER_MAP, ADV7619_REG_INPUTCTRL, regdata);
	} else {
		errCode = adv7619_read_reg(src, HDMI_MAP, ADV7619_REG_HDMIPORTSEL, &regdata);
		if (errCode)
			goto adv7619_select_channel_exit;

		regdata &= ~ADV7619_HDMIPORTSEL_PORT_MASK;
		regdata |= (source_id - ADV7619_HDMI_MIN_ID);
		adv7619_write_reg(src, HDMI_MAP, ADV7619_REG_HDMIPORTSEL, regdata);
	}

	adv7619_fill_share_regs(src, adv7619_source_table[psrcinfo->cap_src_id].pregtable);
#endif
//adv7619_select_channel_exit:
	return errCode;
}

static int adv7619_set_pattern(struct __amba_vin_source *src,enum amba_video_mode mode)
{
	int errCode = 0;
	struct adv7619_src_info *psrcinfo = (struct adv7619_src_info *) src->psrcinfo;
	//u32 source_id = adv7619_source_table[psrcinfo->cap_src_id].source_id;
	if(mode == AMBA_VIDEO_MODE_1080P){
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_30;

		DRV_PRINT("1080p mode\n");
		psrcinfo->so_freq_hz = PLL_CLK_27MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_27MHZ;
		psrcinfo->cap_start_x = 0;
		psrcinfo->cap_start_y = 0;
		//1920x1080P30
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_30;
		psrcinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
	//	psrcinfo->so_freq_hz = PLL_CLK_148_5MHZ;
	//	psrcinfo->so_pclk_freq_hz = PLL_CLK_148_5MHZ;
		psrcinfo->sync_start = 0x8005;
		psrcinfo->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
		psrcinfo->bit_resolution = AMBA_VIDEO_BITS_8;
	 	psrcinfo->input_type = AMBA_VIDEO_TYPE_YUV_656;
	}

	if(mode == AMBA_VIDEO_MODE_1920x2160){
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_30;

		DRV_PRINT("4k2k mode\n");
		psrcinfo->so_freq_hz = PLL_CLK_24MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_150MHZ;
		psrcinfo->cap_start_x = 0;
		psrcinfo->cap_start_y = 0;
		//1920x1080P30
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1920x2160;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_30;
		psrcinfo->cap_cap_w = 1920;
		psrcinfo->cap_cap_h = 2160;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
	//	psrcinfo->so_freq_hz = PLL_CLK_148_5MHZ;
	//	psrcinfo->so_pclk_freq_hz = PLL_CLK_148_5MHZ;
		psrcinfo->sync_start = 0x8005;
		psrcinfo->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
		psrcinfo->bit_resolution = AMBA_VIDEO_BITS_8;
	 	psrcinfo->input_type = AMBA_VIDEO_TYPE_YUV_656;
	}
	return errCode;
}

static int adv7619_check_status(struct __amba_vin_source *src)
{
	int errCode = 0;
	struct adv7619_src_info *psrcinfo = (struct adv7619_src_info *) src->psrcinfo;
	//u32 source_id = adv7619_source_table[psrcinfo->cap_src_id].source_id;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_30;
#if 0
		DRV_PRINT("1080p mode 0x%x\n",psrcinfo->cap_vin_mode);
		psrcinfo->so_freq_hz = PLL_CLK_27MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_27MHZ;
		psrcinfo->cap_start_x = 0;
		psrcinfo->cap_start_y = 0;
		//1920x1080P30
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_30;
		psrcinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
	//	psrcinfo->so_freq_hz = PLL_CLK_148_5MHZ;
	//	psrcinfo->so_pclk_freq_hz = PLL_CLK_148_5MHZ;
		psrcinfo->sync_start = 0x8005;
		psrcinfo->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
		psrcinfo->bit_resolution = AMBA_VIDEO_BITS_8;
	 	psrcinfo->input_type = AMBA_VIDEO_TYPE_YUV_656;
#endif
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_30;

		DRV_PRINT("4k2k mode\n");
		psrcinfo->so_freq_hz = PLL_CLK_24MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_150MHZ;
		psrcinfo->cap_start_x = 0;
		psrcinfo->cap_start_y = 0;
		//1920x1080P30
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1920x2160;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_30;
		psrcinfo->cap_cap_w = 1920;
		psrcinfo->cap_cap_h = 2160;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
	//	psrcinfo->so_freq_hz = PLL_CLK_148_5MHZ;
	//	psrcinfo->so_pclk_freq_hz = PLL_CLK_148_5MHZ;
		psrcinfo->sync_start = 0x8005;
		psrcinfo->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
		psrcinfo->bit_resolution = AMBA_VIDEO_BITS_8;
	 	psrcinfo->input_type = AMBA_VIDEO_TYPE_YUV_656;
	return errCode;
}

static int adv7619_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errCode = 0;
	struct adv7619_src_info *psrcinfo;

	psrcinfo = (struct adv7619_src_info *) src->psrcinfo;
	memset(p_video_info, 0, sizeof(struct amba_video_info));

	p_video_info->width = psrcinfo->cap_cap_w;
	p_video_info->height = psrcinfo->cap_cap_h;
	p_video_info->fps = psrcinfo->frame_rate;
	p_video_info->format = psrcinfo->video_format;
	p_video_info->type = psrcinfo->input_type;
	p_video_info->bits = psrcinfo->bit_resolution;
	p_video_info->ratio = psrcinfo->aspect_ratio;
	p_video_info->system = psrcinfo->video_system;

	return errCode;
}

static int adv7619_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	struct adv7619_src_info *psrcinfo;

	psrcinfo = (struct adv7619_src_info *) src->psrcinfo;

	p_mode_info->is_supported = 0;
	if (p_mode_info->mode != AMBA_VIDEO_MODE_AUTO) {
		errCode = -EINVAL;
		goto adv7619_check_video_mode_exit;
	}

	errCode = adv7619_check_status(src);
	if (errCode) {
		errCode = 0;
		goto adv7619_check_video_mode_exit;
	}

	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	p_mode_info->mode = psrcinfo->cap_vin_mode;
	p_mode_info->is_supported = 1;
	p_mode_info->video_info.width = psrcinfo->cap_cap_w;
	p_mode_info->video_info.height = psrcinfo->cap_cap_h;
	p_mode_info->video_info.fps = psrcinfo->frame_rate;
	p_mode_info->video_info.format = psrcinfo->video_format;
	p_mode_info->video_info.type = psrcinfo->input_type;
	p_mode_info->video_info.bits = psrcinfo->bit_resolution;
	p_mode_info->video_info.ratio = psrcinfo->aspect_ratio;
	p_mode_info->video_info.system = psrcinfo->video_system;
	amba_vin_source_set_fps_flag(p_mode_info, psrcinfo->frame_rate);

adv7619_check_video_mode_exit:
	return errCode;
}

static int adv7619_query_decoder_idrev(struct __amba_vin_source *src, u8 * id)
{
	int errCode = 0;
	u8 idrev = 0;

	errCode = adv7619_read_reg(src, USER_MAP, ADV7619_REG_IDENT, &idrev);
	switch (idrev) {
	case 0x20:
		DRV_PRINT("ADV7619 is detected\n");
		break;
	default:
		DRV_PRINT("Can't detect ADV7619, idrev is 0x%x\n", idrev);
		break;
	}
	*id = idrev;

	return errCode;
}
static int adv7619_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct adv7619_src_info *psrcinfo;

	psrcinfo = (struct adv7619_src_info *) src->psrcinfo;

	*p_mode = psrcinfo->cap_vin_mode;

	return errCode;
}

static int adv7619_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errCode = 0;

	struct adv7619_src_info *psrcinfo;

	/* Hardware Initialization */
/*	errCode = adv7619_init_vin_clock(src);
	if (errCode)
		goto adv7619_set_video_mode_exit;
	msleep(10);
	adv7619_reset(src);
*/
	DRV_PRINT("*****************mode = 0x%x\n",mode);
	psrcinfo = (struct adv7619_src_info *) src->psrcinfo;

	if(mode == AMBA_VIDEO_MODE_1080P)
		psrcinfo->fix_reg_table = adv7619_ycbcr_1080p30_fix_regs;
	if(mode == AMBA_VIDEO_MODE_1920x2160)
		psrcinfo->fix_reg_table = adv7619_ycbcr_2160p30_fix_regs;

	errCode = adv7619_select_source(src);


	errCode = adv7619_set_pattern(src,mode);
	if (errCode)
		goto adv7619_set_video_mode_exit;
	
//	errCode = adv7619_check_status(src,mode);
//	if (errCode)
//		goto adv7619_set_video_mode_exit;

	if (psrcinfo->fix_reg_table)
		adv7619_fill_share_regs(src, psrcinfo->fix_reg_table);

	if ((mode != AMBA_VIDEO_MODE_AUTO) && (mode != psrcinfo->cap_vin_mode)) {
		errCode = -EINVAL;
		goto adv7619_set_video_mode_exit;
	}

//	adv7619_print_info(src);
//	adv7619_dump_reg(src,psrcinfo->fix_reg_table);
	
	errCode = adv7619_pre_set_vin_mode(src);
	if (errCode)
		goto adv7619_set_video_mode_exit;

	errCode = adv7619_set_vin_mode(src);
	if (errCode)
		goto adv7619_set_video_mode_exit;

	errCode |= adv7619_post_set_vin_mode(src);

	msleep(100);
adv7619_set_video_mode_exit:
	return errCode;
}

static int adv7619_update_idc_maps(struct __amba_vin_source *src)
{
	int errCode = 0;
	u8 regdata;
	struct adv7619_info *pinfo;

//	adv7619_write_reg(src, USER_MAP, ADV7619_REG_POWER_DOWN_REG, 0x42);//power up i2c first
	pinfo = (struct adv7619_info *) src->pinfo;

	adv7619_write_reg(src, USER_MAP, ADV7619_REG_CEC_SLAVE_ADDRESS, CONFIG_I2C_AMBARELLA_ADV7619_CECMAP_ADDR);
	errCode = adv7619_read_reg(src, USER_MAP, ADV7619_REG_CEC_SLAVE_ADDRESS, &regdata);
	vin_dbg("CEC addr is 0x%x\n", (regdata));
	pinfo->idc_cecmap.addr = CONFIG_I2C_AMBARELLA_ADV7619_CECMAP_ADDR >> 1;

	adv7619_write_reg(src, USER_MAP, ADV7619_REG_INFOFRAME_SLAVE_ADDRESS, CONFIG_I2C_AMBARELLA_ADV7619_INFOFRAMEMAP_ADDR);
	errCode = adv7619_read_reg(src, USER_MAP, ADV7619_REG_INFOFRAME_SLAVE_ADDRESS, &regdata);
	vin_dbg("INFOFRAME addr is 0x%x\n", (regdata));
	pinfo->idc_infoframemap.addr = CONFIG_I2C_AMBARELLA_ADV7619_INFOFRAMEMAP_ADDR >> 1;

	adv7619_write_reg(src, USER_MAP, ADV7619_REG_DPLL_SLAVE_ADDRESS, CONFIG_I2C_AMBARELLA_ADV7619_DPLLMAP_ADDR);
	errCode = adv7619_read_reg(src, USER_MAP, ADV7619_REG_DPLL_SLAVE_ADDRESS, &regdata);
	vin_dbg("DPLL addr is 0x%x\n", (regdata));
	pinfo->idc_dpllmap.addr = CONFIG_I2C_AMBARELLA_ADV7619_DPLLMAP_ADDR >> 1;

	adv7619_write_reg(src, USER_MAP, ADV7619_REG_KSV_SLAVE_ADDRESS, CONFIG_I2C_AMBARELLA_ADV7619_KSVMAP_ADDR);
	errCode = adv7619_read_reg(src, USER_MAP, ADV7619_REG_KSV_SLAVE_ADDRESS, &regdata);
	vin_dbg("KSV addr is 0x%x\n", (regdata));
	pinfo->idc_ksvmap.addr = CONFIG_I2C_AMBARELLA_ADV7619_KSVMAP_ADDR >> 1;

	adv7619_write_reg(src, USER_MAP, ADV7619_REG_EDID_SLAVE_ADDRESS, CONFIG_I2C_AMBARELLA_ADV7619_EDIDMAP_ADDR);
	errCode = adv7619_read_reg(src, USER_MAP, ADV7619_REG_EDID_SLAVE_ADDRESS, &regdata);
	vin_dbg("EDID addr is 0x%x\n", (regdata));
	pinfo->idc_edidmap.addr = CONFIG_I2C_AMBARELLA_ADV7619_EDIDMAP_ADDR >> 1;

	adv7619_write_reg(src, USER_MAP, ADV7619_REG_HDMI_SLAVE_ADDRESS, CONFIG_I2C_AMBARELLA_ADV7619_HDMIMAP_ADDR);
	errCode = adv7619_read_reg(src, USER_MAP, ADV7619_REG_HDMI_SLAVE_ADDRESS, &regdata);
	vin_dbg("HDMI addr is 0x%x\n", (regdata));
	pinfo->idc_hdmimap.addr = CONFIG_I2C_AMBARELLA_ADV7619_HDMIMAP_ADDR >> 1;

	adv7619_write_reg(src, USER_MAP, ADV7619_REG_CP_SLAVE_ADDRESS, CONFIG_I2C_AMBARELLA_ADV7619_CPMAP_ADDR);
	errCode = adv7619_read_reg(src, USER_MAP, ADV7619_REG_CP_SLAVE_ADDRESS, &regdata);
	vin_dbg("CP addr is 0x%x\n", (regdata));
	pinfo->idc_cpmap.addr = CONFIG_I2C_AMBARELLA_ADV7619_CPMAP_ADDR >> 1;

	pinfo->idc_50map.addr = 0x50 >> 1;
	return errCode;
}

static int adv7619_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct adv7619_info *pinfo;
	struct adv7619_src_info *psrcinfo;

	pinfo = (struct adv7619_info *) src->pinfo;
	psrcinfo = (struct adv7619_src_info *) src->psrcinfo;

	if (src != &pinfo->active_vin_src->cap_src) {
		pinfo->active_vin_src = psrcinfo;
		errCode = adv7619_select_source(src);
		if (errCode) {
			vin_err("Select source%d failed with %d!\n", psrcinfo->cap_src_id, errCode);
			goto adv7619_docmd_exit;
		}
	}

	switch (cmd) {
	case AMBA_VIN_SRC_IDLE:
		break;

	case AMBA_VIN_SRC_RESET:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_POWER:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_INFO:
		{
			struct amba_vin_source_info *pub_src;

			pub_src = (struct amba_vin_source_info *) args;
			pub_src->id = src->id;
			pub_src->adapter_id = adapter_id;
			pub_src->dev_type = src->dev_type;
			strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
			pub_src->sensor_id = DECODER_ADV7619;
			pub_src->default_mode = AMBA_VIDEO_MODE_AUTO;
			pub_src->total_channel_num = src->total_channel_num;
			pub_src->active_channel_id = src->active_channel_id;
			pub_src->source_type.decoder = adv7619_source_table[psrcinfo->cap_src_id].source_type;
		}
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = adv7619_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = adv7619_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = adv7619_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_SELECT_CHANNEL:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = adv7619_get_video_mode(src, (enum amba_video_mode *) args);
		break;
	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = adv7619_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_FPN:
	case AMBA_VIN_SRC_SET_FPN:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_BLC:
	case AMBA_VIN_SRC_SET_BLC:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_CAP_WINDOW:
	case AMBA_VIN_SRC_SET_CAP_WINDOW:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_FRAME_RATE:
		*(u32 *)args = psrcinfo->frame_rate;
		break;
	case AMBA_VIN_SRC_SET_FRAME_RATE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_BLANK:
	case AMBA_VIN_SRC_SET_BLANK:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_PIXEL_SKIP_BIN:
	case AMBA_VIN_SRC_SET_PIXEL_SKIP_BIN:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_CAPTURE_MODE:
	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_TRIGGER_MODE:
	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_SLOWSHUTTER_MODE:
	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		//adv7619_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		{
			u8 id;
			u32 *pdata = (u32 *) args;

			errCode = adv7619_query_decoder_idrev(src, &id);
			if (errCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;

			*pdata = id;
		}
	      exit_AMBA_VIN_SRC_TEST_GET_DEV_ID:
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u8 subaddr;
			u8 data;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;

			errCode = adv7619_read_reg(src, reg_data->regmap, subaddr, &data);

			reg_data->data = data;
		}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u8 subaddr;
			u16 data;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;
			data = reg_data->data;

			errCode = adv7619_write_reg(src, reg_data->regmap, subaddr, data);
		}
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

adv7619_docmd_exit:
	return errCode;
}

