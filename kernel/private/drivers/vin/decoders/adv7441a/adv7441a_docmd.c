/*
 * kernel/private/drivers/ambarella/vin/decoders/adv7441a/adv7441a_docmd.c
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

static void adv7441a_dump_reg(struct __amba_vin_source *src){}

static int adv7441a_check_status_analog(struct __amba_vin_source *src)
{
	int errCode = 0;
	u8 regdata = 0xff;
	u32 count = ADV7441A_RETRY_COUNT;
	u8 videostandard;
	//struct adv7441a_info *pinfo = (struct adv7441a_info *) src->pinfo;
	struct adv7441a_src_info *psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;

	adv7441a_write_reg(src, USER_MAP, ADV7441A_REG_AUTO_DETECT_REG, regdata);
	msleep(10);

	while (count) {	/*Check video detected */
		errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_STATUS1, &regdata);
		if (errCode)
			goto adv7441a_check_status_analog_exit;

		if (((regdata & ADV7441A_STATUS1_INLOCK_MASK)== ADV7441A_STATUS1_INLOCK_LOCK)
			&& ((regdata & ADV7441A_STATUS1_FSCLOCK_MASK) == ADV7441A_STATUS1_FSCLOCK_LOCK)) {
			vin_dbg("video detected\n");
			break;
		}
		msleep(ADV7441A_RETRY_SLEEP_MS);
		count--;
	}
	if (!count) {
		vin_err("Count time out, video not detected!\n");
		errCode = -EINVAL;
		goto adv7441a_check_status_analog_exit;
	}

	videostandard = (regdata & ADV7441A_STATUS1_ADRES_MASK);
	vin_dbg("video standard is 0x%x\n", videostandard);

	/*Check bits width */
#if 0
	errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_OUTCTRL, &regdata);
	vin_dbg("ADV7441A_REG_OUTCTRL 0x%x\n", regdata);
	switch (regdata & ADV7441A_OUTCTRL_OFSEL_MASK) {
	case ADV7441A_OUTCTRL_OFSEL_8BIT656:
		psrcinfo->bit_resolution = AMBA_VIDEO_BITS_8;
		psrcinfo->input_type = AMBA_VIDEO_TYPE_YUV_656;
		break;
	case ADV7441A_OUTCTRL_OFSEL_16BIT:
		psrcinfo->bit_resolution = AMBA_VIDEO_BITS_16;
		psrcinfo->input_type = AMBA_VIDEO_TYPE_YUV_601;
		break;
	default:
		vin_err("not support ouput format\n");
		break;
	}
#else
	psrcinfo->bit_resolution = AMBA_VIDEO_BITS_8;
	psrcinfo->input_type = AMBA_VIDEO_TYPE_YUV_656;
#endif

	switch (videostandard) {
	case ADV7441A_STATUS1_ADRES_NTSM:
	case ADV7441A_STATUS1_ADRES_NTSC443:
	case ADV7441A_STATUS1_ADRES_PALM:
	case ADV7441A_STATUS1_ADRES_PAL60:
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_480I;
		if (psrcinfo->input_type == AMBA_VIDEO_TYPE_YUV_601) {
			psrcinfo->cap_start_x = ADV7441A_480I_601_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_480I_601_CAP_START_Y;
			psrcinfo->sync_start = ADV7441A_480I_601_CAP_SYNC_START;
		} else {
			psrcinfo->cap_start_x = ADV7441A_480I_656_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_480I_656_CAP_START_Y;
			psrcinfo->sync_start = ADV7441A_480I_656_CAP_SYNC_START;
		}
		psrcinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		psrcinfo->fix_reg_table = adv7441a_sdp_480i_fix_regs;
		break;

	case ADV7441A_STATUS1_ADRES_SECAM:
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_576I;
		if (psrcinfo->input_type == AMBA_VIDEO_TYPE_YUV_601) {
			psrcinfo->cap_start_x = ADV7441A_576I_601_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_576I_601_CAP_START_Y;
			psrcinfo->sync_start = ADV7441A_576I_601_CAP_SYNC_START;
		} else {
			psrcinfo->cap_start_x = ADV7441A_576I_656_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_576I_656_CAP_START_Y;
			psrcinfo->sync_start = ADV7441A_576I_656_CAP_SYNC_START;
		}
		psrcinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_SECAM;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->fix_reg_table = adv7441a_sdp_576i_fix_regs;
		break;

	case ADV7441A_STATUS1_ADRES_PAL:
	case ADV7441A_STATUS1_ADRES_PALN:
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_576I;
		if (psrcinfo->input_type == AMBA_VIDEO_TYPE_YUV_601) {
			psrcinfo->cap_start_x = ADV7441A_576I_601_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_576I_601_CAP_START_Y;
			psrcinfo->sync_start = ADV7441A_576I_601_CAP_SYNC_START;
		} else {
			psrcinfo->cap_start_x = ADV7441A_576I_656_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_576I_656_CAP_START_Y;
			psrcinfo->sync_start = ADV7441A_576I_656_CAP_SYNC_START;
		}
		psrcinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->fix_reg_table = adv7441a_sdp_576i_fix_regs;
		break;

	case ADV7441A_STATUS1_ADRES_SECAM525:
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_480I;
		if (psrcinfo->input_type == AMBA_VIDEO_TYPE_YUV_601) {
			psrcinfo->cap_start_x = ADV7441A_480I_601_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_480I_601_CAP_START_Y;
			psrcinfo->sync_start = ADV7441A_480I_601_CAP_SYNC_START;
		} else {
			psrcinfo->cap_start_x = ADV7441A_480I_656_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_480I_656_CAP_START_Y;
			psrcinfo->sync_start = ADV7441A_480I_656_CAP_SYNC_START;
		}
		psrcinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_SECAM;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		psrcinfo->fix_reg_table = adv7441a_sdp_480i_fix_regs;
		break;

	default:
		vin_err("not valid video standard\n");
		errCode = -EINVAL;
		goto adv7441a_check_status_analog_exit;
		break;
	}

	/*Check Interlace mode */
#if 0
	errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_STATUS3, &regdata);
	vin_dbg("ADV7441A_REG_STATUS3 0x%x\n", regdata);
	if ((regdata & ADV7441A_STATUS3_INTL_MASK)
	    == ADV7441A_STATUS3_INTL_INTL) {
		psrcinfo->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
	} else {
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
	}
#else
	psrcinfo->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC;
	psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
#endif

	if (psrcinfo->input_type == AMBA_VIDEO_TYPE_YUV_656) {
		psrcinfo->so_freq_hz = PLL_CLK_27MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_27MHZ;
	} else {
		psrcinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
	}

adv7441a_check_status_analog_exit:
	return errCode;
}

static int adv7441a_check_status_cp(struct __amba_vin_source *src)
{
	int errCode = 0;
	u8 regdata;
	u8 regdata1;
	u16 bl;
	u16 scvs;
	u16 scf;
	u16 count = ADV7441A_RETRY_COUNT;
	u32 i;
	struct adv7441a_vid_stds *pstd;
	//struct adv7441a_info *pinfo = (struct adv7441a_info *) src->pinfo;
	struct adv7441a_src_info *psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;

	while (count) { /*Check video detected */
		errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_STDI_DVALID_REG, &regdata);
		if (errCode)
			goto adv7441a_check_status_cp_exit;

		if ((regdata & ADV7441A_STDI_DVALID_MSK) == ADV7441A_STDI_DVALID) {
			vin_dbg("ADV7441A_STDI_DVALID\n");
			break;
		}
		msleep(ADV7441A_RETRY_SLEEP_MS);
		count--;
	}
	if (!count) {
		vin_err("Count time out, ADV7441A_STDI_DVALID not detected!\n");
		errCode = -EINVAL;
		goto adv7441a_check_status_cp_exit;
	}

	errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_BL_REG, &regdata1);
	if (errCode)
		goto adv7441a_check_status_cp_exit;
	bl = regdata & ADV7441A_BL_H6B_MSK;
	bl <<= 8;
	bl += regdata1;
	bl = (bl * ADV7441A_BL_CLK_FREQ) / ADV7441A_OSC_CLK_FREQ;

	errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_SCVS_REG, &regdata);
	if (errCode)
		goto adv7441a_check_status_cp_exit;
	scvs = regdata >> 3;

	errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_SCF_REG, &regdata1);
	if (errCode)
		goto adv7441a_check_status_cp_exit;
	scf = regdata & ADV7441A_SCF_L3B_MSK;
	scf <<= 8;
	scf += regdata1;

	vin_dbg("bl = %d, scf = %d, scvs = %d, count = %d\n", bl, scf, scvs, count);

	for (i = 0; i < ADV7441A_VID_STD_NUM; i++) {
		pstd = &adv7441a_vid_std_table[i];
		if ((bl >= pstd->bl_low) &&
		    (bl <= pstd->bl_upper) &&
		    (scvs >= pstd->scvs_low) &&
		    (scvs <= pstd->scvs_upper) && (scf >= pstd->scf_low) && (scf <= pstd->scf_upper)) {
			psrcinfo->cap_vin_mode = pstd->cap_vin_mode;
			psrcinfo->cap_start_x = pstd->cap_start_x;
			psrcinfo->cap_start_y = pstd->cap_start_y;
			psrcinfo->cap_cap_w = pstd->cap_cap_w;
			psrcinfo->cap_cap_h = pstd->cap_cap_h;
			psrcinfo->sync_start = pstd->sync_start;
			psrcinfo->video_system = pstd->video_system;
			psrcinfo->frame_rate = pstd->frame_rate;
			psrcinfo->aspect_ratio = pstd->aspect_ratio;
			psrcinfo->input_type = pstd->input_type;
			psrcinfo->video_format = pstd->video_format;
			psrcinfo->bit_resolution = pstd->bit_resolution;
			psrcinfo->input_format = pstd->input_format;
			psrcinfo->so_freq_hz = pstd->so_freq_hz;
			psrcinfo->so_pclk_freq_hz = pstd->so_pclk_freq_hz;
			psrcinfo->fix_reg_table = pstd->fix_reg_table;
			break;
		}
	}
	if (i >= ADV7441A_VID_STD_NUM) {
		vin_warn("Can't support format %d, %d, %d!\n", bl, scvs, scf);
		errCode = -EINVAL;
		goto adv7441a_check_status_cp_exit;
	}

	errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_STDI_REG, &regdata);
	if (errCode)
		goto adv7441a_check_status_cp_exit;
	regdata = (regdata & 0x1c) | 0x02;
	errCode = adv7441a_write_reg(src, USER_MAP, ADV7441A_REG_STDI_REG, regdata);
	if (errCode)
		goto adv7441a_check_status_cp_exit;

adv7441a_check_status_cp_exit:
	return errCode;
}

static int adv7441a_check_status_hdmi_infoframe(struct __amba_vin_source *src)
{
	int errCode = 0;
	u8 regdata;
	u8 infoframever;
	//struct adv7441a_info *pinfo = (struct adv7441a_info *) src->pinfo;
	struct adv7441a_src_info *psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;
	int count = ADV7441A_RETRY_COUNT;

	errCode = adv7441a_read_reg(src, USER_MAP_1, ADV7441A_REG_HDMIRAWSTUS1, &regdata);
	if (errCode)
		goto adv7441a_check_status_hdmi_infoframe_exit;
	if ((regdata & ADV7441A_HDMIRAWSTUTS1_AVIINFO_MASK) == ADV7441A_HDMIRAWSTUTS1_AVIINFO_DET) {
		errCode = adv7441a_read_reg(src, USER_MAP_1, ADV7441A_REG_HDMIINTSTUS4, &regdata);
		if (errCode)
			goto adv7441a_check_status_hdmi_infoframe_exit;
	} else {
		vin_warn("ADV7441A_HDMIRAWSTUTS1_AVIINFO_NODET\n");
	}

	while (count) {
		errCode = adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_AVIINFOVER, &infoframever);
		if (errCode)
			goto adv7441a_check_status_hdmi_infoframe_exit;

		if (infoframever >= 2)
			break;

		count--;
		msleep(ADV7441A_RETRY_SLEEP_MS);
	}
	if (!count) {
		vin_err("HDMI infoframever = %d.\n", infoframever);
		errCode = -EINVAL;
		goto adv7441a_check_status_hdmi_infoframe_exit;
	}

	errCode = adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_AVIINFODATA4, &regdata);
	if (errCode)
		goto adv7441a_check_status_hdmi_infoframe_exit;
	vin_info("video identification code is 0x%x\n", regdata & ADV7441A_AVIINFODATA4_VIC_MASK);

	psrcinfo->cap_start_x = psrcinfo->hs_back_porch;
	psrcinfo->cap_start_y = psrcinfo->f0_vs_front_porch + psrcinfo->f1_vs_front_porch;
	psrcinfo->so_freq_hz = PLL_CLK_27MHZ;
	psrcinfo->so_pclk_freq_hz = PLL_CLK_27MHZ;

	switch (regdata & ADV7441A_AVIINFODATA4_VIC_MASK) {
	/*-----------------60 Hz Systems ----------------*/
	case 1:		/*640x480P60 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_VGA;
		psrcinfo->cap_cap_w = AMBA_VIDEO_VGA_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_VGA_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
		break;

	case 2:		/*720x480P60 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_NTSC;
		psrcinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;//AMBA_VIDEO_FPS_60;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->sync_start = ADV7441A_480P_601_CAP_SYNC_START;
		break;

	case 3:		/*720x480P60 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_NTSC;
		psrcinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->sync_start = ADV7441A_480P_601_CAP_SYNC_START;
		break;

	case 4:		/*1280x720P60 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_720P;
		psrcinfo->cap_cap_w = AMBA_VIDEO_720P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_720P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;//AMBA_VIDEO_FPS_60;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->sync_start = ADV7441A_720P60_601_CAP_SYNC_START;
		break;

	case 5:		/*1920x1080i60 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080I;
		psrcinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		psrcinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->sync_start = ADV7441A_1080I60_601_CAP_SYNC_START;
		break;

	case 6:		/*720x480i60 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_480I;
		psrcinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		psrcinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->sync_start = ADV7441A_480I_601_CAP_SYNC_START;
		if (psrcinfo->line_width == psrcinfo->cap_cap_w) {
			psrcinfo->cap_start_x = ADV7441A_480I_601_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_480I_601_CAP_START_Y;
		}
		break;

	case 7:		/*720x480i60 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_480I;
		psrcinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		psrcinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->sync_start = ADV7441A_480I_601_CAP_SYNC_START;
		if (psrcinfo->line_width == psrcinfo->cap_cap_w) {
			psrcinfo->cap_start_x = ADV7441A_480I_601_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_480I_601_CAP_START_Y;
		}
		break;

	case 16:		/*1920x1080P60 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		psrcinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->so_freq_hz = PLL_CLK_148_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_148_5MHZ;
		psrcinfo->sync_start = ADV7441A_1080P60_601_CAP_SYNC_START;
		break;

	/*-----------------50 Hz Systems ----------------*/
	case 17:		/*720x576p@50 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_PAL;
		psrcinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->sync_start = ADV7441A_576P_601_CAP_SYNC_START;
		break;

	case 18:		/*720x576p@50 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_PAL;
		psrcinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->sync_start = ADV7441A_576P_601_CAP_SYNC_START;
		break;

	case 19:		/*1280x720P50 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_720P;
		psrcinfo->cap_cap_w = AMBA_VIDEO_720P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_720P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->fix_reg_table = adv7441a_hdmi_720p50_fix_regs;
		psrcinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->sync_start = ADV7441A_720P50_601_CAP_SYNC_START;
		break;

	case 20:		/*1920x1080i50 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080I;
		psrcinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		psrcinfo->fix_reg_table = adv7441a_hdmi_1080i50_fix_regs;
		psrcinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->sync_start = ADV7441A_1080I50_601_CAP_SYNC_START;
		break;

	case 21:		/*720x576i@50 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_576I;
		psrcinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		psrcinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->sync_start = ADV7441A_576I_601_CAP_SYNC_START;
		if (psrcinfo->line_width == psrcinfo->cap_cap_w) {
			psrcinfo->cap_start_x = ADV7441A_480I_601_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_480I_601_CAP_START_Y;
		}
		break;

	case 22:		/*720x576i@50 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_576I;
		psrcinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		psrcinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->sync_start = ADV7441A_576I_601_CAP_SYNC_START;
		if (psrcinfo->line_width == psrcinfo->cap_cap_w) {
			psrcinfo->cap_start_x = ADV7441A_480I_601_CAP_START_X;
			psrcinfo->cap_start_y = ADV7441A_480I_601_CAP_START_Y;
		}
		break;

	case 31:		/*1920x1080P50 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		psrcinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->fix_reg_table = adv7441a_hdmi_1080p50_fix_regs;
		psrcinfo->so_freq_hz = PLL_CLK_148_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_148_5MHZ;
		psrcinfo->sync_start = ADV7441A_1080P50_601_CAP_SYNC_START;
		break;

	case 32:		/*1920x1080P24 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		psrcinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS(24);
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->fix_reg_table = adv7441a_hdmi_1080p24_fix_regs;
		psrcinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
		break;

	case 33:		/*1920x1080P25 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		psrcinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_25;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->fix_reg_table = adv7441a_hdmi_1080p25_fix_regs;
		psrcinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
		break;

	case 34:		/*1920x1080P30 */
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		psrcinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_30;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
#ifndef CONFIG_ADV7441A_BLACKMAGIC_YUV
		psrcinfo->fix_reg_table = adv7441a_hdmi_1080p30_fix_regs;
#endif
		psrcinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
		break;

	default:
		vin_warn("Unknown video identification code is 0x%x\n",
			regdata & ADV7441A_AVIINFODATA4_VIC_MASK);
		errCode = -1;
		goto adv7441a_check_status_hdmi_infoframe_exit;
	}

	if (psrcinfo->video_format == AMBA_VIDEO_FORMAT_PROGRESSIVE) {
		if (psrcinfo->line_width != psrcinfo->cap_cap_w) {
			vin_warn("Progressive Mismatch: line_width[%d] and cap_cap_w[%d].\n",
				psrcinfo->line_width, psrcinfo->cap_cap_w);
			errCode = -1;
			goto adv7441a_check_status_hdmi_infoframe_exit;
		}

		if ((psrcinfo->f0_height != psrcinfo->cap_cap_h) ||
			(psrcinfo->f1_height != psrcinfo->cap_cap_h)){
			vin_warn("Progressive Mismatch: f0_height[%d], f1_height[%d] and cap_cap_h[%d].\n",
				psrcinfo->f0_height, psrcinfo->f0_height, psrcinfo->cap_cap_h);
			errCode = -1;
			goto adv7441a_check_status_hdmi_infoframe_exit;
		}
	} else {
		if ((psrcinfo->line_width != psrcinfo->cap_cap_w) &&
			(psrcinfo->line_width != (psrcinfo->cap_cap_w * 2))){
			vin_warn("Interlace Mismatch: line_width[%d] and cap_cap_w[%d].\n",
				psrcinfo->line_width, psrcinfo->cap_cap_w);
			errCode = -1;
			goto adv7441a_check_status_hdmi_infoframe_exit;
		}

		if ((psrcinfo->f0_height + psrcinfo->f1_height) != psrcinfo->cap_cap_h) {
			vin_warn("Interlace Mismatch: f0_height[%d], f1_height[%d] and cap_cap_h[%d].\n",
				psrcinfo->f0_height, psrcinfo->f0_height, psrcinfo->cap_cap_h);
			errCode = -1;
			goto adv7441a_check_status_hdmi_infoframe_exit;
		}
	}

adv7441a_check_status_hdmi_infoframe_exit:
	return errCode;
}

static int adv7441a_check_status_hdmi_resolution(struct __amba_vin_source *src)
{
	int errCode = 0;
	u8 regdata1;
	u8 regdata2;
	int count = ADV7441A_RETRY_COUNT;
	struct adv7441a_src_info *psrcinfo;

	psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;

	/*Increase fix value for different source if necessary... */
	count *= ADV7441A_RETRY_FIX;
	/*PLL locked */
	while (count) {
		errCode = adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_REG04, &regdata1);
		if (errCode)
			goto adv7441a_check_status_hdmi_resolution_exit;
		if ((regdata1 & ADV7441A_REG04_LOCK_MASK) == ADV7441A_REG04_LOCK_LOCKED)
			break;
		else
			count--;
		msleep(ADV7441A_RETRY_SLEEP_MS);
	}
	if (!count) {
		vin_err("Count time out, HDMI PLL not locked 0x%x!\n", regdata1);
		errCode = -EINVAL;
		goto adv7441a_check_status_hdmi_resolution_exit;
	}
	/*DE regeneration locked */
	while (count) {
		errCode = adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_REG07, &regdata1);
		if (errCode)
			goto adv7441a_check_status_hdmi_resolution_exit;
		if ((regdata1 & ADV7441A_REG07_LOCK_MASK) == ADV7441A_REG07_LOCK_LOCKED)
			break;
		else
			count--;
		msleep(ADV7441A_RETRY_SLEEP_MS);
	}
	if (!count) {
		vin_err("Count time out, HDMI DE not locked 0x%x!\n", regdata1);
		errCode = -EINVAL;
		goto adv7441a_check_status_hdmi_resolution_exit;
	}

	/*Line width */
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_LW, &regdata2);
	psrcinfo->line_width = (regdata1 & ADV7441A_REG07_LW11TO8_MASK) << 8;
	psrcinfo->line_width |= regdata2;

	/*field 0 */
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F0HMSB, &regdata1);
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F0HLSB, &regdata2);
	psrcinfo->f0_height = (regdata1 & ADV7441A_F0HMSB_MSB_MASK) << 8;
	psrcinfo->f0_height |= regdata2;

	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F0VSFPMSB, &regdata1);
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F0VSFPLSB, &regdata2);
	psrcinfo->f0_vs_front_porch = (regdata1 & ADV7441A_F0VSFPMSB_MSB_MASK) << 8;
	psrcinfo->f0_vs_front_porch |= regdata2;

	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F0VSPWMSB, &regdata1);
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F0VSPWMSB, &regdata2);
	psrcinfo->f0_vs_plus_width = (regdata1 & ADV7441A_F0VSFPMSB_MSB_MASK) << 8;
	psrcinfo->f0_vs_plus_width |= regdata2;

	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F0VSBPMSB, &regdata1);
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F0VSBPLSB, &regdata2);
	psrcinfo->f0_vs_back_porch = (regdata1 & ADV7441A_F0VSBPMSB_MSB_MASK) << 8;
	psrcinfo->f0_vs_back_porch |= regdata2;

	/*field 1 */
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F1HMSB, &regdata1);
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F1HLSB, &regdata2);
	psrcinfo->f1_height = (regdata1 & ADV7441A_F1HMSB_MSB_MASK) << 8;
	psrcinfo->f1_height |= regdata2;

	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F1VSFPMSB, &regdata1);
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F1VSFPLSB, &regdata2);
	psrcinfo->f1_vs_front_porch = (regdata1 & ADV7441A_F1VSFPMSB_MSB_MASK) << 8;
	psrcinfo->f1_vs_front_porch |= regdata2;

	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F1VSPWMSB, &regdata1);
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F1VSPWLSB, &regdata2);
	psrcinfo->f1_vs_plus_width = (regdata1 & ADV7441A_F1VSFPMSB_MSB_MASK) << 8;
	psrcinfo->f1_vs_plus_width |= regdata2;

	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F1VSBPMSB, &regdata1);
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_F1VSBPLSB, &regdata2);
	psrcinfo->f1_vs_back_porch = (regdata1 & ADV7441A_F1VSBPMSB_MSB_MASK) << 8;
	psrcinfo->f1_vs_back_porch |= regdata2;

	/*HS*/
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_HSFPMSB, &regdata1);
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_HSFPLSB, &regdata2);
	psrcinfo->hs_front_porch = (regdata1 & ADV7441A_HSFPMSB_MSB_MASK) << 8;
	psrcinfo->hs_front_porch |= regdata2;

	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_HSPWMSB, &regdata1);
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_HSPWLSB, &regdata2);
	psrcinfo->hs_plus_width = (regdata1 & ADV7441A_HSPWMSB_MSB_MASK) << 8;
	psrcinfo->hs_plus_width |= regdata2;

	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_HSBPMSB, &regdata1);
	adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_HSBPLSB, &regdata2);
	psrcinfo->hs_back_porch = (regdata1 & ADV7441A_HSBPMSB_MSB_MASK) << 8;
	psrcinfo->hs_back_porch |= regdata2;

adv7441a_check_status_hdmi_resolution_exit:
	return errCode;
}

static int adv7441a_check_status_hdmi(struct __amba_vin_source *src)
{
	int errCode = 0;
	u8 regdata;
	u8 mask;
	int count = ADV7441A_RETRY_COUNT;
	//struct adv7441a_info *pinfo = (struct adv7441a_info *) src->pinfo;
	struct adv7441a_src_info *psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;
	u32 source_id = adv7441a_source_table[psrcinfo->cap_src_id].source_id;

	if (source_id == ADV7441A_HDMI_MIN_ID)
		mask = ADV7441A_HDMIRAWSTUTS3_TMDSCLKA_MASK;
	else
		mask = ADV7441A_HDMIRAWSTUTS3_TMDSCLKB_MASK;
	mask |= ADV7441A_HDMIRAWSTUTS3_CLKLOCK_MASK;
	while (count) {
		errCode = adv7441a_read_reg(src, USER_MAP_1, ADV7441A_REG_HDMIRAWSTUS3, &regdata);
		if (errCode)
			goto adv7441a_check_status_hdmi_exit;
		if ((regdata & mask) == mask)
			break;
		else
			count--;
		msleep(ADV7441A_RETRY_SLEEP_MS);
	}
	if (!count) {
		vin_err("Count time out, video not detected 0x%x!\n", regdata);
		errCode = -EINVAL;
		goto adv7441a_check_status_hdmi_exit;
	}

	errCode = adv7441a_check_status_hdmi_resolution(src);
	if (errCode)
		goto adv7441a_check_status_hdmi_exit;

	/*check hdmi mode */
	errCode = adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_REG05, &regdata);
	if (errCode)
		goto adv7441a_check_status_hdmi_exit;
	if ((regdata & ADV7441A_REG05_HDCP_MASK) == ADV7441A_REG05_HDCP_INUSE)
		vin_info("HDCP protected!\n");
	if ((regdata & ADV7441A_REG05_HDMIMODE_MASK) != ADV7441A_REG05_HDMIMODE_HDMI)
		vin_notice("Input mode is DVI!\n");

	if (((regdata & ADV7441A_REG05_HDMIMODE_MASK)== ADV7441A_REG05_HDMIMODE_HDMI) || check_hdmi_infoframe) {
		errCode = adv7441a_check_status_hdmi_infoframe(src);
		if (!errCode)
			goto adv7441a_check_status_check_bitwidth;
		else
			errCode = 0;
	}

#if 0
	vin_notice("Checking HDMI by CP...\n");
	errCode = adv7441a_check_status_cp(src);
	if (!errCode) {
		psrcinfo->fix_reg_table = NULL;
		goto adv7441a_check_status_check_bitwidth;
	} else
		errCode = 0;

	if (psrcinfo->line_width == 0) {
		errCode = -1;
		goto adv7441a_check_status_hdmi_exit;
	}
#endif

	vin_notice("Checking HDMI width and height...\n");
	psrcinfo->so_freq_hz = PLL_CLK_27MHZ;
	psrcinfo->so_pclk_freq_hz = PLL_CLK_27MHZ;
	psrcinfo->cap_start_x = psrcinfo->hs_back_porch;
	psrcinfo->cap_start_y = psrcinfo->f0_vs_front_porch +
		psrcinfo->f1_vs_front_porch;
	if ((psrcinfo->line_width == AMBA_VIDEO_1080P_W) &&
	    (psrcinfo->f0_height == AMBA_VIDEO_1080P_H) &&
	    (psrcinfo->f1_height == AMBA_VIDEO_1080P_H)) {
		//1920x1080P60
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->so_freq_hz = PLL_CLK_148_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_148_5MHZ;
		psrcinfo->sync_start = ADV7441A_1080P60_601_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_1080P_W) &&
		   (psrcinfo->f0_height == (AMBA_VIDEO_1080P_H / 2)) &&
		   (psrcinfo->f1_height == (AMBA_VIDEO_1080P_H / 2))) {
		//1920x1080I60
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080I;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		psrcinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->sync_start = ADV7441A_1080I60_601_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_720P_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_720P_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_720P_H)) {
		//1280x720P60
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_720P;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;//AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_720P_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_720P_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		psrcinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		psrcinfo->sync_start = ADV7441A_720P60_601_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_PAL_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_PAL_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_PAL_H)) {
		//576P
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_PAL;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->sync_start = ADV7441A_576P_601_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == (AMBA_VIDEO_PAL_W * 2)) &&
		   (psrcinfo->f0_height == (AMBA_VIDEO_PAL_H / 2)) &&
		   (psrcinfo->f1_height == (AMBA_VIDEO_PAL_H / 2))) {
		//576I
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_576I;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->cap_cap_w = AMBA_VIDEO_PAL_W * 2;
		psrcinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->sync_start = ADV7441A_576I_601_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_PAL_W) &&
		   (psrcinfo->f0_height == (AMBA_VIDEO_PAL_H / 2)) &&
		   (psrcinfo->f1_height == (AMBA_VIDEO_PAL_H / 2))) {
		//576I
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_576I;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_50;
		psrcinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->sync_start = ADV7441A_576I_601_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_NTSC_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_NTSC_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_NTSC_H)) {
		//480P
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_NTSC;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;//AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->sync_start = ADV7441A_480P_601_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == (AMBA_VIDEO_NTSC_W * 2)) &&
		   (psrcinfo->f0_height == (AMBA_VIDEO_NTSC_H / 2)) &&
		   (psrcinfo->f1_height == (AMBA_VIDEO_NTSC_H / 2))) {
		//480I
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_480I;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		psrcinfo->cap_cap_w = AMBA_VIDEO_NTSC_W * 2;
		psrcinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->sync_start = ADV7441A_480I_601_CAP_SYNC_START;
		psrcinfo->cap_start_x = psrcinfo->hs_back_porch +
			psrcinfo->hs_front_porch + psrcinfo->hs_plus_width;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_NTSC_W) &&
		   (psrcinfo->f0_height == (AMBA_VIDEO_NTSC_H / 2)) &&
		   (psrcinfo->f1_height == (AMBA_VIDEO_NTSC_H / 2))) {
		//480I
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_480I;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		psrcinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		psrcinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		psrcinfo->sync_start = ADV7441A_480I_601_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_VGA_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_VGA_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_VGA_H)) {
		//VGA
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_VGA;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_VGA_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_VGA_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_SVGA_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_SVGA_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_SVGA_H)) {
		//SVGA
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_SVGA;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_SVGA_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_SVGA_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		psrcinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_XGA_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_XGA_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_XGA_H)) {
		//XGA
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_XGA;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_XGA_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_XGA_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		psrcinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_SXGA_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_SXGA_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_SXGA_H)) {
		//SXGA
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_SXGA;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_SXGA_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_SXGA_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		psrcinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_UXGA_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_UXGA_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_UXGA_H)) {
		//UXGA
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_UXGA;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_UXGA_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_UXGA_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		psrcinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_WXGA_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_WXGA_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_WXGA_H)) {
		//WXGA
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_WXGA;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_WXGA_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_WXGA_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		psrcinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_WSXGA_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_WSXGA_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_WSXGA_H)) {
		//WSXGA
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_WSXGA;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_WSXGA_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_WSXGA_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		psrcinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_WSXGAP_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_WSXGAP_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_WSXGAP_H)) {
		//WSXGAP
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_WSXGAP;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_WSXGAP_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_WSXGAP_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		psrcinfo->so_freq_hz = PLL_CLK_148_5D1001MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_148_5D1001MHZ;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((psrcinfo->line_width == AMBA_VIDEO_WUXGA_W) &&
		   (psrcinfo->f0_height == AMBA_VIDEO_WUXGA_H) &&
		   (psrcinfo->f1_height == AMBA_VIDEO_WUXGA_H)) {
		//WUXGA
		psrcinfo->cap_vin_mode = AMBA_VIDEO_MODE_WUXGA;
		psrcinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		psrcinfo->frame_rate = AMBA_VIDEO_FPS_60;
		psrcinfo->cap_cap_w = AMBA_VIDEO_WUXGA_W;
		psrcinfo->cap_cap_h = AMBA_VIDEO_WUXGA_H;
		psrcinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		psrcinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		psrcinfo->so_freq_hz = PLL_CLK_148_5D1001MHZ;
		psrcinfo->so_pclk_freq_hz = PLL_CLK_148_5D1001MHZ;
		psrcinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else {
		vin_err("Not support video format %dx[%d/%d]\n",
			psrcinfo->line_width, psrcinfo->f0_height,
			psrcinfo->f1_height);
		adv7441a_print_info(src);
		errCode = -1;
		goto adv7441a_check_status_hdmi_exit;
	}

adv7441a_check_status_check_bitwidth:
	if (psrcinfo->video_format == AMBA_VIDEO_FORMAT_PROGRESSIVE)
		psrcinfo->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
	else
		psrcinfo->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC;

	errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_OUTCTRL, &regdata);
	switch (regdata & ADV7441A_OUTCTRL_OFSEL_MASK) {
	case ADV7441A_OUTCTRL_OFSEL_8BIT656:
		psrcinfo->bit_resolution = AMBA_VIDEO_BITS_8;
		psrcinfo->input_type = AMBA_VIDEO_TYPE_YUV_656;
		break;

	case ADV7441A_OUTCTRL_OFSEL_16BIT:
#if defined(ADV7441A_PREFER_EMBMODE)
		psrcinfo->cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
		psrcinfo->cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
		psrcinfo->sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
		psrcinfo->bit_resolution = AMBA_VIDEO_BITS_16;
		psrcinfo->input_type = AMBA_VIDEO_TYPE_YUV_656;
#else
		psrcinfo->bit_resolution = AMBA_VIDEO_BITS_16;
		psrcinfo->input_type = AMBA_VIDEO_TYPE_YUV_601;
#endif
		break;

	default:
		vin_err("not support ouput format\n");
		break;
	}

adv7441a_check_status_hdmi_exit:
	return errCode;
}

static int adv7441a_init_interrupts(struct __amba_vin_source *src)
{
	int errCode = 0;
	u8 regdata;

	/*Initiate int mask1 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_INT1MSK1, regdata);
	/*Reset int clear1 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_INT1CLR1, regdata);
	regdata = 0x00;
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_INT1CLR1, regdata);

	/*Initiate int mask2 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_INT1MSK2, regdata);
	/*Reset int clear2 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_INT1CLR2, regdata);
	regdata = 0x00;
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_INT1CLR2, regdata);

	/*Initiate int mask3 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_INT1MSK3, regdata);
	/*Reset int clear3 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_INT1CLR3, regdata);
	regdata = 0x00;
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_INT1CLR3, regdata);

	/*HDMI interrupt */
	regdata = 0x10;
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_HDMIINT2MSK2, regdata);
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_HDMIINT2MSK1, regdata);
	regdata = 0xff;
	errCode = adv7441a_write_reg(src, USER_MAP_1, ADV7441A_REG_HDMIINTSTUS2, regdata);

	return errCode;
}

static void adv7441a_sw_reset(struct __amba_vin_source *src, int force_init)
{
	//int errCode = 0;
	u32 i;
	struct adv7441a_info *pinfo;
	u8 reg;

	pinfo = (struct adv7441a_info *) src->pinfo;

	if (force_init) {
		//Deassert HPD...
		ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_hdmi_hpd, 0);
		msleep(ADV7441A_RETRY_SLEEP_MS);
	}

	adv7441a_write_reg(src, KSV_MAP, ADV7441A_REG_CTRL_BITS, 0);

	//errCode = adv7441a_write_reset(src);
	adv7441a_write_reset(src);
	do {
		msleep(5);
		//errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_PWR, &reg);
		adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_PWR, &reg);
	} while ((reg & 0x80) != 0);

	adv7441a_init_interrupts(src);

	adv7441a_write_reg(src, KSV_MAP, ADV7441A_REG_BCAPS,
		ADV7441A_BCAPS_DEFAULT | ADV7441A_BCAPS_HDMI_ENABLE);

	if (pinfo->valid_edid) {
		adv7441a_write_reg(src, KSV_MAP, ADV7441A_REG_CTRL_BITS, 0);
		for (i = 0; i < ADV7441A_EDID_TABLE_SIZE; i++)
			adv7441a_write_reg(src, EDID_MAP, i, pinfo->edid[i]);
		adv7441a_write_reg(src, KSV_MAP, ADV7441A_REG_CTRL_BITS,
			ADV7441A_EDID_A_ENABLE | ADV7441A_EDID_B_ENABLE);
		msleep(5);
		for (i = 0; i < ADV7441A_EDID_TABLE_SIZE; i++) {
			adv7441a_read_reg(src, EDID_MAP, i, &reg);
			vin_dbg("[%02d]: 0x%02x\n", i, reg);
		}
	}

	if (force_init) {
		//Assert HPD...
		ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_hdmi_hpd, 1);
	}

	msleep(ADV7441A_RETRY_SLEEP_MS);
}

static void adv7441a_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	adv7441a_sw_reset(src, 1);
}

static void adv7441a_fill_share_regs(struct __amba_vin_source *src, const struct adv7441a_reg_table regtable[])
{
	u32 i;

	for (i = 0;; i++) {
		if ((regtable[i].data == 0xff) && (regtable[i].reg == 0xff))
			break;
		adv7441a_write_reg(src, regtable[i].regmap, regtable[i].reg, regtable[i].data);
	}
}

static int adv7441a_select_source(struct __amba_vin_source *src)
{
	int errCode = 0;
	u8 regdata;
	u32 source_id;
	struct adv7441a_src_info *psrcinfo;

	psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;
	source_id = adv7441a_source_table[psrcinfo->cap_src_id].source_id;

	adv7441a_sw_reset(src, 0);

	if (source_id < ADV7441A_HDMI_MIN_ID) {
		errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_INPUTCTRL, &regdata);
		if (errCode)
			goto adv7441a_select_channel_exit;

		regdata &= ~ADV7441A_INPUTCTRL_INSEL_MASK;
		regdata |= source_id;
		adv7441a_write_reg(src, USER_MAP, ADV7441A_REG_INPUTCTRL, regdata);
	} else {
		errCode = adv7441a_read_reg(src, HDMI_MAP, ADV7441A_REG_HDMIPORTSEL, &regdata);
		if (errCode)
			goto adv7441a_select_channel_exit;

		regdata &= ~ADV7441A_HDMIPORTSEL_PORT_MASK;
		regdata |= (source_id - ADV7441A_HDMI_MIN_ID);
		adv7441a_write_reg(src, HDMI_MAP, ADV7441A_REG_HDMIPORTSEL, regdata);
	}

	adv7441a_fill_share_regs(src, adv7441a_source_table[psrcinfo->cap_src_id].pregtable);

adv7441a_select_channel_exit:
	return errCode;
}

static int adv7441a_check_status(struct __amba_vin_source *src)
{
	int errCode = 0;
	struct adv7441a_src_info *psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;
	//u32 source_id = adv7441a_source_table[psrcinfo->cap_src_id].source_id;

	psrcinfo->line_width = 0;
	psrcinfo->f0_height = 0;
	psrcinfo->f1_height = 0;
	psrcinfo->f0_vs_back_porch = 0;
	psrcinfo->f1_vs_back_porch = 1;

	switch (adv7441a_source_table[psrcinfo->cap_src_id].source_processor) {
	case ADV7441A_PROCESSOR_SDP:
		errCode = adv7441a_check_status_analog(src);
		break;

	case ADV7441A_PROCESSOR_CP:
		errCode = adv7441a_check_status_cp(src);
		break;

	case ADV7441A_PROCESSOR_HDMI:
		errCode = adv7441a_check_status_hdmi(src);
		break;

	default:
		vin_err("Unknown processor type\n");
		errCode = -EINVAL;
		break;
	}

	return errCode;
}

static int adv7441a_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errCode = 0;
	struct adv7441a_src_info *psrcinfo;

	psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;
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

static int adv7441a_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	struct adv7441a_src_info *psrcinfo;

	psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;

	p_mode_info->is_supported = 0;
	if (p_mode_info->mode != AMBA_VIDEO_MODE_AUTO) {
		errCode = -EINVAL;
		goto adv7441a_check_video_mode_exit;
	}

	errCode = adv7441a_check_status(src);
	if (errCode) {
		errCode = 0;
		goto adv7441a_check_video_mode_exit;
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

adv7441a_check_video_mode_exit:
	return errCode;
}

static int adv7441a_query_decoder_idrev(struct __amba_vin_source *src, u8 * id)
{
	int errCode = 0;
	u8 idrev = 0;

	errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_IDENT, &idrev);
	switch (idrev) {
	case 0x01:
		vin_dbg("ADV7441A-ES1 is detected\n");
		break;
	case 0x02:
		vin_dbg("ADV7441A-ES2 is detected\n");
		break;
	case 0x04:
		vin_dbg("ADV7441A-ES3 is detected\n");
		break;
	default:
		vin_err("Can't detect ADV7441A, idrev is 0x%x\n", idrev);
		break;
	}
	*id = idrev;

	return errCode;
}

static int adv7441a_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errCode = 0;

	struct adv7441a_src_info *psrcinfo;

	/* Hardware Initialization */
	errCode = adv7441a_init_vin_clock(src);
	if (errCode)
		goto adv7441a_set_video_mode_exit;
	msleep(10);
	adv7441a_reset(src);

	psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;

	psrcinfo->fix_reg_table = NULL;
	errCode = adv7441a_select_source(src);

	errCode = adv7441a_check_status(src);
	if (errCode)
		goto adv7441a_set_video_mode_exit;

	if (psrcinfo->fix_reg_table)
		adv7441a_fill_share_regs(src, psrcinfo->fix_reg_table);

	if ((mode != AMBA_VIDEO_MODE_AUTO) && (mode != psrcinfo->cap_vin_mode)) {
		errCode = -EINVAL;
		goto adv7441a_set_video_mode_exit;
	}

	adv7441a_print_info(src);

	errCode = adv7441a_pre_set_vin_mode(src);
	if (errCode)
		goto adv7441a_set_video_mode_exit;

	errCode = adv7441a_set_vin_mode(src);
	if (errCode)
		goto adv7441a_set_video_mode_exit;

	errCode |= adv7441a_post_set_vin_mode(src);

	msleep(100);
adv7441a_set_video_mode_exit:
	return errCode;
}

static int adv7441a_update_idc_usrmap2(struct __amba_vin_source *src)
{
	int errCode = 0;
	u8 regdata;
	struct adv7441a_info *pinfo;

	pinfo = (struct adv7441a_info *) src->pinfo;

	errCode = adv7441a_read_reg(src, USER_MAP, ADV7441A_REG_USS2, &regdata);
	pinfo->idc_usrmap2.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_dbg("User map2 addr is 0x%x\n", (pinfo->idc_usrmap2.addr << 1));

	return errCode;
}

static int adv7441a_update_idc_maps(struct __amba_vin_source *src)
{
	int errCode = 0;
	u8 regdata;
	struct adv7441a_info *pinfo;

	pinfo = (struct adv7441a_info *) src->pinfo;

	errCode = adv7441a_read_reg(src, USER_MAP_2, ADV7441A_REG_HID, &regdata);
	pinfo->idc_revmap.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_dbg("Hidden Map addr is 0x%x\n", (pinfo->idc_revmap.addr << 1));

	errCode = adv7441a_read_reg(src, USER_MAP_2, ADV7441A_REG_USS1, &regdata);
	pinfo->idc_usrmap1.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_dbg("User Sub Map 1 addr is 0x%x\n", (pinfo->idc_usrmap1.addr << 1));

	errCode = adv7441a_read_reg(src, USER_MAP_2, ADV7441A_REG_VDP, &regdata);
	pinfo->idc_vdpmap.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_dbg("VDP Map addr is 0x%x\n", (pinfo->idc_vdpmap.addr << 1));

	errCode = adv7441a_read_reg(src, USER_MAP_2, ADV7441A_REG_KSV, &regdata);
	pinfo->idc_rksvmap.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_dbg("KSV Map addr is 0x%x\n", (pinfo->idc_rksvmap.addr << 1));

	errCode = adv7441a_read_reg(src, USER_MAP_2, ADV7441A_REG_EDID, &regdata);
	pinfo->idc_edidmap.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_dbg("EDID Map addr is 0x%x\n", (pinfo->idc_edidmap.addr << 1));

	errCode = adv7441a_read_reg(src, USER_MAP_2, ADV7441A_REG_HDMI, &regdata);
	pinfo->idc_hdmimap.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_dbg("HDMI Map addr is 0x%x\n", (pinfo->idc_hdmimap.addr << 1));

	return errCode;
}

static int adv7441a_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct adv7441a_info *pinfo;
	struct adv7441a_src_info *psrcinfo;

	pinfo = (struct adv7441a_info *) src->pinfo;
	psrcinfo = (struct adv7441a_src_info *) src->psrcinfo;

	if (src != &pinfo->active_vin_src->cap_src) {
		pinfo->active_vin_src = psrcinfo;
		errCode = adv7441a_select_source(src);
		if (errCode) {
			vin_err("Select source%d failed with %d!\n", psrcinfo->cap_src_id, errCode);
			goto adv7441a_docmd_exit;
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
			pub_src->sensor_id = DECODER_ADV7441A;
			pub_src->default_mode = AMBA_VIDEO_MODE_AUTO;
			pub_src->total_channel_num = src->total_channel_num;
			pub_src->active_channel_id = src->active_channel_id;
			pub_src->source_type.decoder = adv7441a_source_table[psrcinfo->cap_src_id].source_type;
		}
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = adv7441a_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = adv7441a_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = adv7441a_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_SELECT_CHANNEL:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;
	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = adv7441a_set_video_mode(src, *(enum amba_video_mode *) args);
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
		adv7441a_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		{
			u8 id;
			u32 *pdata = (u32 *) args;

			errCode = adv7441a_query_decoder_idrev(src, &id);
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

			errCode = adv7441a_read_reg(src, reg_data->regmap, subaddr, &data);

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

			errCode = adv7441a_write_reg(src, reg_data->regmap, subaddr, data);
		}
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

adv7441a_docmd_exit:
	return errCode;
}

