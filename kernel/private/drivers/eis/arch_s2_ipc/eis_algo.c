/*
 * kernel/private/drivers/eis/arch_s2_ipc/eis_algo.c
 *
 * History:
 *    2013/02/25 - [Zhenwu Xue] Ported from test_eis.c
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>
#include <amba_vin.h>
#include <utils.h>

/******************* should be moved to aaa param *******************/
static u8 min_frate = 15, max_eis_range_perct = 20, min_eis_range_perct = 20;
static u8 eis_strength_x = 100;
static u8 eis_strength_y = 100;
static u8 rsc_strength_x = 100;
static u8 rsc_strength_y = 100;
//static u8 flg_eis_dz_effect = 1;
static u8 video_sampling_rate = 1;
//static u8 still_sampling_rate = 5;

/******************* should be moved to aaa param *******************/
static s8 Eis_update_end_time_1;
static s8 Eis_update_end_time_2;
static s8 Def_eis_update_end_time;
static s8 Rsc_range_perct;
static u8 *eis_enhance_turbo_buf;
static u8 flg_eis_gyro_log = 0;
static u8 flg_is_non_support;
static s32 *gyro_buf2;
static s32 latest_eis_ptr[2];
static u32 sensor_active_time;
static u32 curr_ftime;
static u32 now_frate = 0;
//static u32 *dump;
//static u32 current_shutter_time = 0;
static u32 curr_eis_range_perct;
static u32 old_dsp_zf_x = 0;
static u32 old_dsp_zf_y = 0;
static u32 dsp_zf_x = 0;
static u32 dsp_zf_y = 0;
static u64 sec2_dsp_zf_y;
static u32 cfa_zf_x;
static u32 cfa_zf_y;
static u64 eis_dzoom_factor;
static u32 Eis_enable_dly_cnt;
static u32 bias_adjust_bit_shift;
static u32 active_h;
static u32 safe_h;
static u32 minus_h = 40;
static u32 act_win_start_y = 0;
static u32 vertical_blank_time;
//static u32 *log_index;
static u32 focal_value;
static u32 eis_gyro_log_cnt;
static eis_coeff_info_t* _eis_coeff[2];
static eis_update_info_t* _eis_update;
static iav_eis_info_ex_t iav_eis_info;
static eis_param_t eis_param, def_eis_param;
static gyro_param_t gyro_param, gyro_param_bak;
static gyro_info_t gyro_info;
static gyro_log_t gyro_log, *eis_gyro_log;
static eis_t G_eis_obj = { 0 };
static eis_move_t eis_offset_info = { 0 };
static eis_move_t rsc_skew_info = { 0 };
static eis_move_t eis_lim_p;
static eis_move_t eis_lim_n;
static eis_move_t rsc_lim_p;
static eis_move_t rsc_lim_n;
static eis_move_t act_win_zp;
static is_motion_t eis_mv;
static gyro_calib_info_t gyro_calib_info;
static eis_status_t eis_status;
static struct amba_vin_eis_info vin_eis_info;
static u32 apb_addr;
static struct timeval vcap_time;

#ifndef ABS
#define ABS(a)	   				(((a) < 0) ? -(a) : (a))
#endif
#define EIS_BIT_SHIFT				8
#define ZF_SFT					16
#define ZF_1X					(1 << ZF_SFT)
#define PI					(3.1415926f)

#define EIS_DIS_NON_ALIGN_DEF_DZOOM		2
#define EIS_UPDATE_ADV_TIME			4
#define EIS_MAX_ENABLE_DLY_TIM          	500 //ms
#define EIS_BIAS_ADJ_PRE_SET			5
#define EIS_BIAS_ADJ_POST_SET			9

//>>>
#define EIS_DBG_DSP_SYS_TIM

/******************* should be moved to gyro driver *******************/
static gyro_calib_info_t gyro_calib_info_main;
static void gyro_init_calib_params(void)
{
	gyro_calib_info_main.mean_x =
		((u32) gyro_info.min_bias + gyro_info.max_bias)
			<< (EIS_BIT_SHIFT - 1);
	gyro_calib_info_main.mean_y = gyro_calib_info_main.mean_x;
	gyro_calib_info_main.var_x = 0; //gyro_calib_param->var_x;
	gyro_calib_info_main.var_y = 0; //gyro_calib_param->var_y;

	gyro_calib_info_main.sense_x = (gyro_info.min_sense + gyro_info.max_sense)
		<< (EIS_BIT_SHIFT - 1);
	gyro_calib_info_main.sense_y = gyro_calib_info_main.sense_x;
} /* end gyro_init_params() */

static void gyro_get_params_info(gyro_calib_info_t *calib_param)
{
	gyro_init_calib_params();
	memcpy(calib_param, &gyro_calib_info_main, sizeof(gyro_calib_info_t));
}

/******************* should be moved to lens driver *******************/
u32 zp_focal = 800;

static int img_eis_calc_ssr(idsp_crop_info_t* ssr, const eis_move_t* eis,
	const eis_move_t* rsc)
{
	s32 lt_x, lt_y, compensate_x, compensate_y;
	u64 act_w, act_h;
	u32 raw_w = vin_eis_info.cap_cap_w;
	u32 raw_h = vin_eis_info.cap_cap_h;
	u32 main_w = iav_eis_info.main_width;
	u32 main_h = iav_eis_info.main_height;
	u32 zf_x_prime = iav_eis_info.zoom_x;
	u32 zf_y_prime = iav_eis_info.zoom_y;

	act_w = ((u64) main_w << (ZF_SFT << 1));
	DO_DIV_ROUND(act_w, zf_x_prime);
	act_h = ((u64) main_h << (ZF_SFT << 1));
	DO_DIV_ROUND(act_h, zf_y_prime);
	ssr->left_top_x = ((raw_w << ZF_SFT) - act_w) >> 1;
	ssr->left_top_y = ((raw_h << ZF_SFT) - act_h) >> 1;
	ssr->right_bot_x = (raw_w << ZF_SFT) - ssr->left_top_x;
	ssr->right_bot_y = (raw_h << ZF_SFT) - ssr->left_top_y;
	ssr->right_bot_y += (rsc->mov_y * (act_h >> ZF_SFT));

	lt_x = ssr->left_top_x;
	lt_x += eis->mov_x;
	lt_y = ssr->left_top_y;
	lt_y += eis->mov_y;
	compensate_x = 0;
	compensate_y = 0;

	if (lt_x < 0) {
		printk("ssr->left_top_x %d < 0\n", lt_x >> ZF_SFT);
		compensate_x = 0 - lt_x;
		lt_x = 0;
	}
	if (lt_y < 0) {
		printk("ssr->left_top_y %d < 0\n", lt_y >> ZF_SFT);
		compensate_y = 0 - lt_y;
		lt_y = 0;
	}
	ssr->left_top_x = lt_x;
	ssr->left_top_y = lt_y;
	ssr->right_bot_x += eis->mov_x + compensate_x;
	ssr->right_bot_y += eis->mov_y + compensate_y;
	compensate_x = 0;
	compensate_y = 0;
	if (ssr->right_bot_x >> ZF_SFT > raw_w) {
		printk("ssr->right_bot_x %d > cap_w %d\n",
			ssr->right_bot_x >> ZF_SFT, raw_w);
		compensate_x = ssr->right_bot_x - (raw_w << ZF_SFT);
		ssr->right_bot_x = raw_w << ZF_SFT;
	}
	if (ssr->right_bot_y >> ZF_SFT > raw_h) {
		printk("ssr->right_bot_y %d > height %d\n",
			ssr->right_bot_y >> ZF_SFT, raw_h);
		compensate_y = ssr->right_bot_y - (raw_h << ZF_SFT);
		ssr->right_bot_y = raw_h << ZF_SFT;
	}
	if (compensate_x) {
		if (ssr->left_top_x < compensate_x) {
			printk("ssr->left_top_x %d < compensate_x %d!!\n",
				ssr->left_top_x, compensate_x);
			ssr->left_top_x = 0;
		} else {
			ssr->left_top_x -= compensate_x;
		}
	}
	if (compensate_y) {
		if (ssr->left_top_y < compensate_y) {
			printk("ssr->left_top_y %d < compensate_y %d!!\n",
				ssr->left_top_y, compensate_y);
			ssr->left_top_y = 0;
		} else {
			ssr->left_top_y -= compensate_y;
		}
	}
	return 0;
}

static int img_eis_calc_dmy(idsp_crop_info_t* ssr, geometry_info_t* dmy,
	const eis_move_t* eis, const eis_move_t* rsc)
{
	s32 temp, temp2;
	s32 raw_w = vin_eis_info.cap_cap_w;
	s32 raw_h = vin_eis_info.cap_cap_h;
	s32 diff_x, diff_y;
	u32 range_x, dummy_range_x = eis_dzoom_factor;

	//------------------- y direction -------------------//
	dmy->start_y = ssr->left_top_y >> ZF_SFT;
	if (dmy->start_y & 0x1)
		dmy->start_y--;

	dmy->width = iav_eis_info.dummy_width;
	dmy->height = iav_eis_info.dummy_height;
	temp = raw_h - dmy->height;
	if (dmy->start_y > temp) {
		temp2 = rsc->mov_x * ((s32) dmy->start_y - temp);
		ssr->left_top_x += temp2;
		ssr->right_bot_x += temp2;
		dmy->start_y = temp;
	}

	//------------------- x direction -------------------//
	dmy->start_x = ssr->left_top_x >> ZF_SFT;
	range_x = raw_w - (raw_w << ZF_SFT) / dummy_range_x;
	DO_DIV_ROUND(range_x, dummy_range_x);
	range_x >>= 1;
	if (dmy->start_x < range_x)
		dmy->start_x = 0;
	else
		dmy->start_x -= range_x;

	// 2-pixel alignment
	if (dmy->start_x & 0x1)
		dmy->start_x--;
	temp = raw_w - dmy->width;
	if (dmy->start_x > temp)
		dmy->start_x = temp;

	diff_x = ((ssr->right_bot_x + 0xFFFF) >> ZF_SFT)
		- (dmy->start_x + dmy->width);
	if (diff_x > 0) {
		printk("X: dummy window < actual window. ssr(%d ~ %d), dmy(%d ~ %d)\n",
			((ssr->left_top_x + 0xFFFF) >> ZF_SFT),
			((ssr->right_bot_x + 0xFFFF) >> ZF_SFT),
			dmy->start_x, dmy->start_x + dmy->width);
		ssr->right_bot_x -= (diff_x << ZF_SFT);
	}

	diff_y = ((ssr->right_bot_y + 0xFFFF) >> ZF_SFT)
		- (dmy->start_y + dmy->height);
	if (diff_y > 0) {
		printk("Y: dummy window < actual window. ssr(%d ~ %d), dmy (%d ~ %d)\n",
			((ssr->left_top_y + 0xFFFF) >> ZF_SFT),
			((ssr->right_bot_y + 0xFFFF) >> ZF_SFT),
			dmy->start_y, dmy->start_y + dmy->height);
		ssr->right_bot_y -= (diff_y << ZF_SFT);
	}
	return 0;
}

static int img_eis_calc_win_pos(idsp_crop_info_t* ssr, geometry_info_t* dmy,
	const eis_move_t eis, const eis_move_t rsc)
{
	img_eis_calc_ssr(ssr, &eis, &rsc);
	img_eis_calc_dmy(ssr, dmy, &eis, &rsc);
	return 0;
}

static int img_eis_range_update(void)
{
	u64 act_w, act_h, sec2_out_h;
	u32 cap_w = vin_eis_info.cap_cap_w, cap_h = vin_eis_info.cap_cap_h;
	eis_move_t temp_p, temp_n, eis = { 0, 0 }, rsc = { 0, 0 };
	static idsp_crop_info_t ssr = { 0 };
	static geometry_info_t dmy = { 0 };
	static eis_move_t is_mov_lim = { 0 };

	// get dsp zf
	get_eis_info_ex(&iav_eis_info);
	dsp_zf_x = iav_eis_info.zoom_x;
	dsp_zf_y = iav_eis_info.zoom_y;
	if ((old_dsp_zf_x != dsp_zf_x) || (old_dsp_zf_y != dsp_zf_y)) {
		old_dsp_zf_x = dsp_zf_x;
		old_dsp_zf_y = dsp_zf_y;

		// get sensor binning
		amba_vin_source_cmd(0, 0, AMBA_VIN_SRC_GET_EIS_INFO, &vin_eis_info);

		//>>> FIXME
		eis_param.sensor_cell_size_x = def_eis_param.sensor_cell_size_x
			* vin_eis_info.column_bin / 100;
		eis_param.sensor_cell_size_y = def_eis_param.sensor_cell_size_y
			* vin_eis_info.row_bin / 100;

		img_eis_calc_win_pos(&ssr, &dmy, eis, rsc);
		act_win_zp.mov_x = ssr.left_top_x;
		act_win_zp.mov_y = ssr.left_top_y;
		active_h = (ssr.right_bot_y - ssr.left_top_y + 0xffff) >> ZF_SFT;
		act_win_start_y = ssr.left_top_y >> ZF_SFT;

		Rsc_range_perct = (vin_eis_info.source_height + vin_eis_info.vb_lines + active_h);
		active_h = 100 * active_h;
		DO_DIV_ROUND(active_h, Rsc_range_perct);
		Rsc_range_perct = active_h;

		//if (G_eis_obj.enable) {
		cfa_zf_x = (iav_eis_info.cfa_width << ZF_SFT)
		        / iav_eis_info.dummy_width;
		cfa_zf_y = (iav_eis_info.cfa_height << ZF_SFT)
		        / iav_eis_info.dummy_height;
		if (cfa_zf_y == ZF_1X) {
			sec2_dsp_zf_y = dsp_zf_y;
		} else {
			sec2_dsp_zf_y = ((u64) dsp_zf_y << ZF_SFT);
			DO_DIV_ROUND(sec2_dsp_zf_y, cfa_zf_y);
		}

		if (sec2_dsp_zf_y > ZF_1X) {
			sec2_out_h = (u64)(iav_eis_info.main_height << ZF_SFT);
			DO_DIV_ROUND(sec2_out_h, sec2_dsp_zf_y);
			sec2_dsp_zf_y = ZF_1X;
		} else {
			sec2_out_h = iav_eis_info.main_height;
		}
		safe_h = sec2_out_h > minus_h ? sec2_out_h - minus_h : 0;

		act_w = (cap_w << ZF_SFT);
		DO_DIV_ROUND(act_w, eis_dzoom_factor + 1);
		act_h = (cap_h << ZF_SFT);
		DO_DIV_ROUND(act_h, eis_dzoom_factor + 1);
		is_mov_lim.mov_x = (cap_w - act_w) >> 1;
		if (is_mov_lim.mov_x < 0) {
			is_mov_lim.mov_x = 0;
		}
		is_mov_lim.mov_y = (cap_h - act_h) >> 1;
		if (is_mov_lim.mov_y < 0) {
			is_mov_lim.mov_y = 0;
		}
		//} //if (G_eis_obj.enable) {
	} //if ((old_dsp_zf_x != dsp_zf_x) || (old_dsp_zf_y != dsp_zf_y)) {

	// x dir, left and right is symmetric
	temp_n.mov_x = is_mov_lim.mov_x * Rsc_range_perct / 100;
	temp_p.mov_x = (ssr.left_top_x >> ZF_SFT) - dmy.start_x - 1;
	if ((temp_p.mov_x > 0) && (temp_n.mov_x > temp_p.mov_x)) {
		temp_n.mov_x = temp_p.mov_x;
	} else if (temp_p.mov_x <= 0) {
		temp_n.mov_x = 0;
	}
	temp_p.mov_x = is_mov_lim.mov_x - temp_n.mov_x;
	rsc_lim_p.mov_x = rsc_lim_n.mov_x = temp_n.mov_x << EIS_BIT_SHIFT;
	eis_lim_p.mov_x = eis_lim_n.mov_x = temp_p.mov_x << EIS_BIT_SHIFT;

	// y dir, up and down is NOT symmetric
	temp_n.mov_y = is_mov_lim.mov_y * Rsc_range_perct / 100;
	temp_p.mov_y = dmy.start_y + dmy.height - (ssr.right_bot_y >> ZF_SFT) - 1;
	if ((temp_p.mov_y > 0) && (temp_n.mov_y > temp_p.mov_y)) {
		temp_n.mov_y = temp_p.mov_y;
	} else if (temp_p.mov_y <= 0) {
		temp_n.mov_y = 0;
	}
	temp_p.mov_y = is_mov_lim.mov_y - temp_n.mov_y;
	if (gyro_info.gyro_y_polar == -1) {
		eis_lim_n.mov_y = temp_p.mov_y << EIS_BIT_SHIFT;
		eis_lim_p.mov_y = (temp_p.mov_y + temp_n.mov_y) << EIS_BIT_SHIFT;
	} else {
		eis_lim_p.mov_y = temp_p.mov_y << EIS_BIT_SHIFT;
		eis_lim_n.mov_y = (temp_p.mov_y + temp_n.mov_y) << EIS_BIT_SHIFT;
	}
	rsc_lim_p.mov_y = rsc_lim_n.mov_y = temp_n.mov_y << EIS_BIT_SHIFT;

	temp_p.mov_x = eis_lim_p.mov_x << (ZF_SFT - EIS_BIT_SHIFT);
	temp_n.mov_x = eis_lim_n.mov_x << (ZF_SFT - EIS_BIT_SHIFT);
	if (gyro_info.gyro_y_polar == -1) {
		temp_n.mov_y = eis_lim_p.mov_y << (ZF_SFT - EIS_BIT_SHIFT);
		temp_p.mov_y = eis_lim_n.mov_y << (ZF_SFT - EIS_BIT_SHIFT);
	} else {
		temp_p.mov_y = eis_lim_p.mov_y << (ZF_SFT - EIS_BIT_SHIFT);
		temp_n.mov_y = eis_lim_n.mov_y << (ZF_SFT - EIS_BIT_SHIFT);
	}
	if (eis_offset_info.mov_x > temp_p.mov_x) {
		eis_offset_info.mov_x = temp_p.mov_x;
	} else if (eis_offset_info.mov_x < -temp_n.mov_x) {
		eis_offset_info.mov_x = -temp_n.mov_x;
	}

	if (eis_offset_info.mov_y > temp_p.mov_y) {
		eis_offset_info.mov_y = temp_p.mov_y;
	} else if (eis_offset_info.mov_y < -temp_n.mov_y) {
		eis_offset_info.mov_y = -temp_n.mov_y;
	}

	return 0;
} /* static void img_eis_range_update(void) */

static void img_eis_set_adc_sampling_rate(u32 adc_sample_rate)
{
	G_eis_obj.adc_sampling_rate = adc_sample_rate;

	Eis_update_end_time_1 =
		(G_eis_obj.adc_sampling_rate == 1) ?
		Def_eis_update_end_time - EIS_UPDATE_ADV_TIME :
		Def_eis_update_end_time - (G_eis_obj.adc_sampling_rate << 1);
	Eis_update_end_time_2 = Def_eis_update_end_time;
	Eis_update_end_time_1 += sensor_active_time;
	Eis_update_end_time_2 += sensor_active_time;

	if (Eis_update_end_time_1 > curr_ftime)
		Eis_update_end_time_1 -= curr_ftime;
	if (Eis_update_end_time_2 > curr_ftime)
		Eis_update_end_time_2 -= curr_ftime;

	if (G_eis_obj.enable || G_eis_obj.enable_cmd) {
		// Magic number 167 is based on bias_adjust_bit_shift = 5 IIR filter, and the time index
		//      for the step response of this filter reaches 0.995 is 167ms.
		Eis_enable_dly_cnt = 167 * (1 << bias_adjust_bit_shift)
		        * G_eis_obj.adc_sampling_rate / (1 << 5);
		if (Eis_enable_dly_cnt > EIS_MAX_ENABLE_DLY_TIM) {
			s32 i;

			for (i = bias_adjust_bit_shift; i > -1; i--) {
				Eis_enable_dly_cnt = 167 * (1 << i)
					* G_eis_obj.adc_sampling_rate / (1 << 5);
				if (Eis_enable_dly_cnt < EIS_MAX_ENABLE_DLY_TIM) {
					bias_adjust_bit_shift = i;
					break;
				}
			}
		}
		Eis_enable_dly_cnt = Eis_enable_dly_cnt / curr_ftime + 1;
	}
}

static void img_eis_gyro_param_init(void)
{
	// load calibration parameters
	gyro_get_params_info(&gyro_calib_info);
	gyro_param.Offset_x = gyro_calib_info_main.mean_x;
	gyro_param.Offset_y = gyro_calib_info_main.mean_y;
	gyro_param.Deadband_x = gyro_calib_info_main.var_x;
	gyro_param.Deadband_y = gyro_calib_info_main.var_y;
	// backup gyro calibration data
	memcpy(&gyro_param_bak, &gyro_param, sizeof(gyro_param_t));
}

static int img_eis_param_init(void)
{
	amba_vin_source_cmd(0, 0, AMBA_VIN_SRC_GET_EIS_INFO, &vin_eis_info);
	def_eis_param.sensor_cell_size_x = ((u32) vin_eis_info.sensor_cell_width
	        << EIS_BIT_SHIFT);
	def_eis_param.sensor_cell_size_y = ((u32) vin_eis_info.sensor_cell_height
	        << EIS_BIT_SHIFT);
	def_eis_param.eis_scale_factor_den_x = def_eis_param.sensor_cell_size_x
	        * 180;
	def_eis_param.eis_scale_factor_den_x *= gyro_info.vol_div_den;
	def_eis_param.eis_scale_factor_den_y = def_eis_param.sensor_cell_size_y
	        * 180;
	def_eis_param.eis_scale_factor_den_y *= gyro_info.vol_div_den;
	def_eis_param.eis_scale_factor_num = (1 << EIS_BIT_SHIFT) * PI;
	def_eis_param.eis_scale_factor_num *= gyro_info.vol_div_num;
	eis_param.eis_scale_factor_num = def_eis_param.eis_scale_factor_num;
	return 0;
}

static int img_eis_mode_init(void)
{
	static u8 flg_once = 0;
	static u8 *gyro_buf_addr = NULL;
	s32 i, buf_size;
	u64 temp;

	if (!flg_once) {
		flg_once = 1;
		// allocate memory for gyro logging buffer
		buf_size = 1000 / min_frate * 3;
		if ((gyro_buf_addr = kmalloc(buf_size * EIS_BUFFER_NUM_CHANNELS
			* sizeof(s32), GFP_KERNEL)) < 0) {
			printk("ERROR:can't malloc memory for gyro logging\n");
			return -1;
		}
		memset(gyro_buf_addr, 0, buf_size * EIS_BUFFER_NUM_CHANNELS
			* sizeof(s32));
		gyro_buf2 = (s32*) gyro_buf_addr;
		for (i = 0; i < EIS_BUFFER_NUM_CHANNELS; i++) {
			gyro_log.chan[i].data = &gyro_buf2[buf_size * i];
			gyro_log.chan[i].ndata = buf_size;
			gyro_log.chan[i].head = 0;
			gyro_log.chan[i].tail = 0;
			gyro_log.chan[i].lim_l = 0;
			gyro_log.chan[i].lim_h = 0;
		}

		img_eis_gyro_param_init();
		// Need to set a initial value before EIS is enabled. Since we compute
		// gyro offset from current gyro sample and use it for next sample.
		gyro_log.chan[EIS_BUFFER_OFFSET_X_CHAN].data[0] =
			gyro_param_bak.Offset_x;
		gyro_log.chan[EIS_BUFFER_OFFSET_Y_CHAN].data[0] =
			gyro_param_bak.Offset_y;
		img_eis_param_init();
	}

	if (max_eis_range_perct < min_eis_range_perct)
		max_eis_range_perct = min_eis_range_perct;
	//>>>FIXME
	curr_eis_range_perct = (u32) min_eis_range_perct << ZF_SFT;
	eis_dzoom_factor = ((u64) 100 << (ZF_SFT << 1));
	DO_DIV_ROUND(eis_dzoom_factor, ((100 << ZF_SFT) - curr_eis_range_perct));
	eis_status.eis_dzoom_factor = eis_dzoom_factor;
	focal_value = (zp_focal << EIS_BIT_SHIFT) / 100;

	amba_vin_source_cmd(0, 0, AMBA_VIN_SRC_GET_EIS_INFO, &vin_eis_info);
	get_eis_info_ex(&iav_eis_info);

	curr_ftime = (vin_eis_info.main_fps >> 9) / 1000 + 1;
	sensor_active_time = vin_eis_info.source_height * vin_eis_info.row_time
	        / 1000000 + 1;

	//>>> FIXME
	eis_param.sensor_cell_size_x = def_eis_param.sensor_cell_size_x
	        * vin_eis_info.column_bin / 100;
	eis_param.sensor_cell_size_y = def_eis_param.sensor_cell_size_y
	        * vin_eis_info.row_bin / 100;
	temp = (u64) def_eis_param.eis_scale_factor_den_x * vin_eis_info.column_bin
	        * gyro_calib_info.sense_x;
	DO_DIV_ROUND(temp, 100);
	eis_param.eis_scale_factor_den_x = temp >> EIS_BIT_SHIFT;
	temp = (u64) def_eis_param.eis_scale_factor_den_y * vin_eis_info.row_bin
	        * gyro_calib_info.sense_y;
	DO_DIV_ROUND(temp, 100);
	eis_param.eis_scale_factor_den_y = temp >> EIS_BIT_SHIFT;

	//>>>FIXME
	Def_eis_update_end_time = iav_eis_info.cmd_read_delay / 12288;
	old_dsp_zf_x = old_dsp_zf_y = 0;
	now_frate = 0;
	return 0;
}

static int hori_skew_dbg_port(s32 value, u32* sreg)
{
	static u8		init_flg = 0;
	static u32		vert_luma_reg1, vert_luma_reg2;
	static u32		debug_port_en = 0x00118000, debug_port_hor = 0x00110030;
	u32			v_addr, value1, value2;

	if (!init_flg) {
		invalidate_d_cache((void*)eis_enhance_turbo_buf, sizeof(eis_enhance_turbo_buf_t));
		vert_luma_reg1 = _eis_update->sec2_vert_out_luma_addr_1 & 0x0FFFFFFF;
		vert_luma_reg2 = _eis_update->sec2_vert_out_luma_addr_2 & 0x0FFFFFFF;
		if ((!vert_luma_reg1) || (!vert_luma_reg2)) {
			printk("ERROR: hori_skew_dbg_port sec2 output address is 0!!!\n");
			return -3;
		} else {
			init_flg = 1;
		}
	}

	v_addr = apb_addr + vert_luma_reg1;
	value1 = amba_readl(v_addr);
	v_addr = apb_addr + vert_luma_reg2;
	value2 = amba_readl(v_addr);

	value1 = (value1 & 0xFFF8000) >> 15;
	value2 = (value2 & 0xFFF8000) >> 15;

	*sreg = value1;
	*(sreg + 1) = value2;
	if (((value1 == 0) || (value1 > safe_h))
	        && ((value2 == 0) || (value2 > safe_h))) {
		return (-2);
	}

	value >>= 3;

	v_addr = apb_addr + debug_port_en;
	value1 = 0x2110;
	amba_writel(v_addr, value1);

	v_addr = apb_addr + debug_port_hor;
	value2 = value;
	amba_writel(v_addr, value2);

	// chroma
	v_addr = apb_addr + debug_port_en;
	value1 = 0x2120;
	amba_writel(v_addr, value1);

	v_addr = apb_addr + debug_port_hor;
	value2 = value;
	amba_writel(v_addr, value2);

	return 0;
}

static int vert_skew_dbg_port_2(s32 luma_val, s32 chroma_val, u8 idx)
{
#define USE_NEW_REG
	static u8		init_flg = 0;
	static u32		debug_port_en = 0x00118000, debug_port = 0x001120E4;
	static u32		vert_luma_reg[2];
	u32			v_addr, value1, value2;

	if (!init_flg) {
		invalidate_d_cache((void*)eis_enhance_turbo_buf, sizeof(eis_enhance_turbo_buf_t));
		vert_luma_reg[0] = _eis_update->sec2_vert_out_luma_addr_1 & 0x0FFFFFFF;
		vert_luma_reg[1] = _eis_update->sec2_vert_out_luma_addr_2 & 0x0FFFFFFF;
		if ((!vert_luma_reg[0]) || (!vert_luma_reg[1])) {
			printk("ERROR: vert_skew_dbg_port sec2 output address is 0!!!\n");
			return -3;
		} else {
			init_flg = 1;
		}
	}

	v_addr = apb_addr + vert_luma_reg[idx];
	value1 = amba_readl(v_addr);
	value1 = (value1 & 0x7FFC000) >> 14;
	if ((value1 == 0) || (value1 > safe_h))
		return -2;

	v_addr = apb_addr + debug_port_en;
	value1 = 0x2110;
	amba_writel(v_addr, value1);

	luma_val >>= 3;
#ifdef USE_NEW_REG
	v_addr = apb_addr + debug_port;
	value2 = luma_val & 0xFFFF;
	amba_writel(v_addr, value2);
#else
	v_addr = apb_addr + debug_port_l;
	value1 = luma_val & 0x1FFF;
	amba_writel(v_addr, value1);

	v_addr = apb_addr + debug_port_h;
	value2 = (luma_val >> 13) & 0x7;
	amba_writel(v_addr, value2);
#endif
	v_addr = apb_addr + debug_port_en;
	value1 = 0x2120;
	amba_writel(v_addr, value1);

	chroma_val >>= 3;
#ifdef USE_NEW_REG
	v_addr = apb_addr + debug_port;
	value2 = chroma_val & 0xFFFF;
	amba_writel(v_addr, value2);
#else
	v_addr = apb_addr + debug_port_l;
	value1 = chroma_val & 0x1FFF;
	amba_writel(v_addr, value1);

	v_addr = apb_addr + debug_port_h;
	value2 = (chroma_val >> 13) & 0x7;
	amba_writel(v_addr, value2);
#endif

	return 0;
}

/*static int vert_skew_dbg_port(s32 value, u8 spf, u8 idx)
{
	static u8		init_flg = 0;
	static u32		debug_port_en = 0x00118000, debug_port_h = 0x00110040;
	static u32		debug_port_l = 0x0011003C;
	static u32		vert_luma_reg[2], hori_luma_reg[2], hori_chroma_reg[2];
	s32			rval = 0;
	u32			retry = 0, G_retry_count = 200;
	u32			luma_high_limit = 0;
	u32			chroma_high_limit = 0;
	u32			v_addr, value1, value2, value3;

	if (!init_flg) {
		invalidate_d_cache((void*)eis_enhance_turbo_buf, sizeof(eis_enhance_turbo_buf_t));
		vert_luma_reg[0] = _eis_update->sec2_vert_out_luma_addr_1 & 0x0FFFFFFF;
		vert_luma_reg[1] = _eis_update->sec2_vert_out_luma_addr_2 & 0x0FFFFFFF;
		hori_luma_reg[0] = _eis_update->sec2_hori_out_luma_addr_1 & 0x0FFFFFFF;
		hori_luma_reg[1] = _eis_update->sec2_hori_out_luma_addr_2 & 0x0FFFFFFF;
		hori_chroma_reg[0] = _eis_update->sec2_hori_out_chroma_addr_1
		        & 0x0FFFFFFF;
		hori_chroma_reg[1] = _eis_update->sec2_hori_out_chroma_addr_2
		        & 0x0FFFFFFF;

		if ((!vert_luma_reg[0]) || (!vert_luma_reg[1]) || (!hori_luma_reg[0])
		        || (!hori_luma_reg[1]) || (!hori_chroma_reg[0])
		        || (!hori_chroma_reg[1])) {
			printk("ERROR: vert_skew_dbg_port sec2 output address is 0!!!\n");
			return (-3);
		} else {
			init_flg = 1;
		}
	}

	if (spf != 0xFF) {
		v_addr = apb_addr + vert_luma_reg[idx];
		value3 = amba_readl(v_addr);
		value3 = (value3 & 0xFFF8000) >> 15;

		if ((value3 == 0) || (value3 > safe_h)) {
			rval = -2;
		} else {
			retry = G_retry_count;

			v_addr = apb_addr + debug_port_en;
			value1 = 0x2110;
			amba_writel(v_addr, value1);

			while (rval == 0) {
				v_addr = apb_addr + hori_luma_reg[idx];
				value3 = amba_readl(v_addr);
				value3 = value3 & 0x3FFF;
				if (value3 <= luma_high_limit) {
					v_addr = apb_addr + debug_port_l;
					value1 = value & 0xFFFF;
					amba_writel(v_addr, value1);

					v_addr = apb_addr + debug_port_h;
					value2 = (value >> 16) & 0x7F;
					amba_writel(v_addr, value2);

					value <<= 1;

					v_addr = apb_addr + debug_port_en;
					value2 = 0x2120;
					amba_writel(v_addr, value2);

					retry = G_retry_count;
					do {
						v_addr = apb_addr + hori_chroma_reg[idx];
						value3 = amba_readl(v_addr);
						value3 = value3 & 0x3FFF;
						if (value3 <= chroma_high_limit) {
							v_addr = apb_addr + debug_port_l;
							value1 = value & 0xFFFF;
							amba_writel(v_addr, value1);

							v_addr = apb_addr + debug_port_h;
							value2 = (value >> 16) & 0x7F;
							amba_writel(v_addr, value2);

							break;
						}
					} while (--retry);

					if (retry == 0)
						rval = -1;

					break;
				}

				if (--retry == 0)
					rval = -1;
			} //while(rval == 0) {
		} //if ((reg_1 == 0) || (reg_1 > safe_h))
	} else { //if (spf != 0xFF) {
		v_addr = apb_addr + vert_luma_reg[0];
		value1 = amba_readl(v_addr);
		value1 = (value1 & 0xFFF8000) >> 15;

		v_addr = apb_addr + vert_luma_reg[1];
		value2 = amba_readl(v_addr);
		value2 = (value2 & 0xFFF8000) >> 15;

		if (((value1 != 0) && (value1 < safe_h))
			|| ((value2 != 0) || (value2 < safe_h))) {
			v_addr = apb_addr + debug_port_en;
			value1 = 0x2110;
			amba_writel(v_addr, value1);

			v_addr = apb_addr + debug_port_l;
			value1 = value & 0xFFFF;
			amba_writel(v_addr, value1);

			v_addr = apb_addr + debug_port_h;
			value2 = (value >> 16) & 0x7F;
			amba_writel(v_addr, value2);

			value <<= 1;

			v_addr = apb_addr + debug_port_en;
			value1 = 0x2120;
			amba_writel(v_addr, value1);

			v_addr = apb_addr + debug_port_l;
			value1 = value & 0xFFFF;
			amba_writel(v_addr, value1);

			v_addr = apb_addr + debug_port_h;
			value2 = (value >> 16) & 0x7F;
			amba_writel(v_addr, value2);
		}
	}

	return 0;
}*/

static s32 eis_algo_calc_gyro_phase_offset(int vb_time, int mov_y,
	u32 row_time, int p_offset)
{
	s32 def_gyro_phase_offset;

	def_gyro_phase_offset = vb_time + (row_time * mov_y / 1000000) + p_offset;
	def_gyro_phase_offset += sensor_active_time;
	return def_gyro_phase_offset;
}

static s32 eis_algo_calc_mv(u8 dir, s32 gyro_value, u32 ftime)
{
	s8 sign;
	s32 rval, deadband_thd;
	u64 g_val, scale_factor_den;

	if (!dir) {
		scale_factor_den = eis_param.eis_scale_factor_den_x;
		deadband_thd = gyro_param.Deadband_x;
	} else {
		scale_factor_den = eis_param.eis_scale_factor_den_y;
		deadband_thd = gyro_param.Deadband_y;
	}

	if (gyro_value >= 0) {
		sign = 1;
		g_val = gyro_value;
	} else {
		sign = -1;
		g_val = -gyro_value;
	}

	if (g_val >= deadband_thd) {
		g_val = (g_val * eis_param.eis_scale_factor_num) >> EIS_BIT_SHIFT;
		g_val = (g_val * ftime) >> EIS_BIT_SHIFT;
		g_val = g_val * focal_value;
		DO_DIV_ROUND(g_val, scale_factor_den);
	} else {
		g_val = 0;
		sign = 0;
	}

	rval = (s32) g_val;
	if (sign < 0)
		rval *= (-1);
	return (rval);
}

static void eis_algo_me(eis_move_t *mv_in, eis_move_t *mv_out, u32 ft)
{
	s8 sign_x, sign_y;

	if (mv_in->mov_x >= 0) {
		sign_x = 1;
	} else {
		sign_x = -1;
		mv_in->mov_x *= (-1);
	}
	if (mv_in->mov_y >= 0) {
		sign_y = 1;
	} else {
		sign_y = -1;
		mv_in->mov_y *= (-1);
	}

	mv_out->mov_x = eis_algo_calc_mv(0, mv_in->mov_x, ft);
	mv_out->mov_y = eis_algo_calc_mv(1, mv_in->mov_y, ft);

	// rounding
	if (!(mv_out->mov_x >> EIS_BIT_SHIFT))
		mv_out->mov_x = 0;
	if (!(mv_out->mov_y >> EIS_BIT_SHIFT))
		mv_out->mov_y = 0;

	if ((sign_x == -1) && (mv_out->mov_x > 0))
		mv_out->mov_x *= (-1);
	if ((sign_y == -1) && (mv_out->mov_y > 0))
		mv_out->mov_y *= (-1);
}

static s32 eis_algo_calc_mc(u8 dir, s32 fmv, s32* amv)
{
	u8 rsc_str, eis_str;
	s32 tmp, out, lim_pos, lim_neg;

	if (amv == NULL ) {
		// calculate rolling shutter
		if (!dir)
			rsc_str = rsc_strength_x;
		else
			rsc_str = rsc_strength_y;

		tmp = fmv * rsc_str / 100;
		out = tmp;
	} else {
		// calculate EIS
		if (!dir) {
			eis_str = eis_strength_x;
			lim_pos = eis_lim_p.mov_x;
			lim_neg = eis_lim_n.mov_x;
		} else {
			eis_str = eis_strength_y;
			lim_pos = eis_lim_p.mov_y;
			lim_neg = eis_lim_n.mov_y;
		}

		tmp = *amv * 99 / 100 + fmv * eis_str / 100;

		if (tmp > (lim_pos >> 1)) {
			tmp -= (tmp >> 5);
		} else if (tmp < -(lim_neg >> 1)) {
			tmp -= (tmp >> 5);
		} else {
			tmp -= (tmp >> 6);
		}
		out = tmp;

		// clipping
		if (out > lim_pos)
			out = lim_pos;
		else if (out < -lim_neg)
			out = -lim_neg;
	}

	return out;
}

static void eis_algo_mc(eis_move_t *mv_in, eis_move_t *mv_in2,
	eis_move_t *mv_out)
{
	if (mv_in2 == NULL ) {
		mv_out->mov_x = eis_algo_calc_mc(0, mv_in->mov_x, 0);
		mv_out->mov_y = eis_algo_calc_mc(1, mv_in->mov_y, 0);
	} else {
		mv_out->mov_x = eis_algo_calc_mc(0, mv_in->mov_x, &(mv_in2->mov_x));
		mv_out->mov_y = eis_algo_calc_mc(1, mv_in->mov_y, &(mv_in2->mov_y));
	}
}

//>>>
//#define TEST_INTERPOLATION
static void eis_algo_enhance_turbo(gyro_log_t* gyro_buf, u32 start_ptr,
	u32 last_ptr, u16 exp_time, eis_move_t *mv, u8 dbg_en,
	s32 gyro_sample_offset, s32 act_h, eis_move_t *eis_info,
	eis_move_t *rsc_info, u8 me_flag, u32 ftime)
{
	u8 def_spf;
	s32 adj_start_ptr, prev_frm_amv_ptr, spf, i, j;
	u32 buf_size = gyro_buf->chan[0].ndata;
	eis_move_t gyroraw_inter, gyroraw_intra, fmv_inter, fmv_intra, cmv_inter,
	        cmv_intra = { 0, 0 }, amv_inter;

	gyro_sample_offset -= (exp_time >> 1);
	def_spf = ftime >> EIS_BIT_SHIFT;
	if (G_eis_obj.adc_sampling_rate > 1) {
		gyro_sample_offset /= G_eis_obj.adc_sampling_rate;
		def_spf /= G_eis_obj.adc_sampling_rate;
	}

	// RSC
	gyroraw_intra.mov_x = gyro_buf->chan[GYRO_RAW_X].data[last_ptr]
	        - gyro_buf->chan[EIS_BUFFER_OFFSET_X_CHAN].data[last_ptr];
	if (dbg_en) {
		fmv_intra.mov_x = eis_algo_calc_mv(0, gyroraw_intra.mov_x, ftime);
		cmv_intra.mov_x = eis_algo_calc_mc(0, fmv_intra.mov_x, 0);
		cmv_intra.mov_y = 0;
	} else {
		gyroraw_intra.mov_y = gyro_buf->chan[GYRO_RAW_Y].data[last_ptr]
		        - gyro_buf->chan[EIS_BUFFER_OFFSET_Y_CHAN].data[last_ptr];
		eis_algo_me(&gyroraw_intra, &fmv_intra, ftime);
		eis_algo_mc(&fmv_intra, 0, &cmv_intra);
		cmv_intra.mov_y = (cmv_intra.mov_y * act_h)
		        / (vin_eis_info.source_height + vin_eis_info.vb_lines);
		// clipping
		if (cmv_intra.mov_y > rsc_lim_p.mov_y) {
			cmv_intra.mov_y = rsc_lim_p.mov_y;
		} else if (cmv_intra.mov_y < -rsc_lim_n.mov_y) {
			cmv_intra.mov_y = -rsc_lim_n.mov_y;
		}
		cmv_intra.mov_y <<= (ZF_SFT - EIS_BIT_SHIFT);
		cmv_intra.mov_y /= act_h;
	}
	cmv_intra.mov_x = (cmv_intra.mov_x * act_h)
	        / (vin_eis_info.source_height + vin_eis_info.vb_lines);
	// clipping
	if (cmv_intra.mov_x > rsc_lim_p.mov_x) {
		cmv_intra.mov_x = rsc_lim_p.mov_x;
	} else if (cmv_intra.mov_x < -rsc_lim_n.mov_x) {
		cmv_intra.mov_x = -rsc_lim_n.mov_x;
	}
	cmv_intra.mov_x <<= (ZF_SFT - EIS_BIT_SHIFT);
	cmv_intra.mov_x /= act_h;

	// EIS
	adj_start_ptr = (s32) start_ptr + gyro_sample_offset;
	if (adj_start_ptr < 0) {
		adj_start_ptr += buf_size;
	} else if (adj_start_ptr >= buf_size) {
		adj_start_ptr -= buf_size;
	}

	if (last_ptr < adj_start_ptr) {
		spf = last_ptr + buf_size - adj_start_ptr + 1;
	} else {
		spf = last_ptr - adj_start_ptr + 1;
	}

	if (spf > def_spf) {
		spf = def_spf;
	} else if (spf == 0) {
		// for protection
		spf = 1;
	}

#ifdef TEST_INTERPOLATION
{
	s32 k;

	gyroraw_inter.mov_x = gyroraw_inter.mov_y = 0;
	for (i = 0; i < spf - 1; i ++) {
		j = (adj_start_ptr + i);
		if (j >= buf_size)
		j -= buf_size;
		k = j + 1;
		if (k >= buf_size)
		k -= buf_size;
		gyroraw_inter.mov_x += ((gyro_buf->chan[GYRO_RAW_X].data[j] -
			gyro_buf->chan[EIS_BUFFER_OFFSET_X_CHAN].data[j] +
			gyro_buf->chan[GYRO_RAW_X].data[k] -
			gyro_buf->chan[EIS_BUFFER_OFFSET_X_CHAN].data[k]) >> 1);
		gyroraw_inter.mov_y += ((gyro_buf->chan[GYRO_RAW_Y].data[j] -
			gyro_buf->chan[EIS_BUFFER_OFFSET_Y_CHAN].data[j] +
			gyro_buf->chan[GYRO_RAW_Y].data[k] -
			gyro_buf->chan[EIS_BUFFER_OFFSET_Y_CHAN].data[k]) >> 1);
	}
	gyroraw_inter.mov_x /= (spf - 1);
	gyroraw_inter.mov_y /= (spf - 1);
}
#else
	gyroraw_inter.mov_x = gyroraw_inter.mov_y = 0;
	for (i = 0; i < spf; i++) {
		j = (adj_start_ptr + i);
		if (j >= buf_size)
			j -= buf_size;
		gyroraw_inter.mov_x += gyro_buf->chan[GYRO_RAW_X].data[j]
		        - gyro_buf->chan[EIS_BUFFER_OFFSET_X_CHAN].data[j];
		gyroraw_inter.mov_y += gyro_buf->chan[GYRO_RAW_Y].data[j]
		        - gyro_buf->chan[EIS_BUFFER_OFFSET_Y_CHAN].data[j];
	}
	gyroraw_inter.mov_x /= spf;
	gyroraw_inter.mov_y /= spf;
#endif

	// to avoid prev_frm_amv_ptr exceeds the limit by check the log of previous EIS update index,
	// due to potentially EIS update miss caused by heavy system loading.
	prev_frm_amv_ptr = last_ptr - def_spf;
	i = gyro_buf->chan[0].lim_l;
	j = gyro_buf->chan[0].lim_h;
	if (i <= j) {
		if (prev_frm_amv_ptr < 0)
			prev_frm_amv_ptr += buf_size;
	} else
		i -= buf_size;

	if (prev_frm_amv_ptr < i)
		prev_frm_amv_ptr = i;
	else if (prev_frm_amv_ptr > j)
		prev_frm_amv_ptr = j;
	if (prev_frm_amv_ptr < 0)
		prev_frm_amv_ptr += buf_size;
	amv_inter.mov_x =
	        gyro_buf->chan[EIS_BUFFER_AMV_X_CHAN].data[prev_frm_amv_ptr];
	amv_inter.mov_y =
	        gyro_buf->chan[EIS_BUFFER_AMV_Y_CHAN].data[prev_frm_amv_ptr];

	eis_algo_me(&gyroraw_inter, &fmv_inter, ftime);
	eis_algo_mc(&fmv_inter, &amv_inter, &cmv_inter);

	gyro_buf->chan[EIS_BUFFER_AMV_X_CHAN].data[last_ptr] = cmv_inter.mov_x;
	gyro_buf->chan[EIS_BUFFER_AMV_Y_CHAN].data[last_ptr] = cmv_inter.mov_y;

	cmv_inter.mov_x <<= (ZF_SFT - EIS_BIT_SHIFT);
	cmv_inter.mov_y <<= (ZF_SFT - EIS_BIT_SHIFT);

	eis_info->mov_x = cmv_inter.mov_x;
	eis_info->mov_y = cmv_inter.mov_y;
	rsc_info->mov_x = cmv_intra.mov_x;
	rsc_info->mov_y = cmv_intra.mov_y;

	mv->mov_x = fmv_inter.mov_x;
	mv->mov_y = fmv_inter.mov_y;
}

/*static int img_eis_disable(void)
{
	int rval = 0;

	G_eis_obj.enable_pref = G_eis_obj.rsc_enable_pref = 0;
	if (!G_eis_obj.enable && !G_eis_obj.disable_cmd && !G_eis_obj.enable_cmd) {
		rval = -1;
		goto done;
	}
	G_eis_obj.enable = eis_status.enable = 0;
	G_eis_obj.mc_enable = eis_status.mc_enable = 0;
	G_eis_obj.rsc_enable = eis_status.rsc_enable = 0;
	G_eis_obj.enable_cmd = G_eis_obj.disable_cmd = 0;
	memset(&eis_offset_info, 0, sizeof(eis_move_t));
	memset(&rsc_skew_info, 0, sizeof(eis_move_t));

	if (G_eis_obj.stabilizer_type == EIS) {
		//_eis_update->latest_eis_coeff_addr = 0;
		//img_eis_stop_gyro_task(0);
		{
			idsp_crop_info_t ssr = { 0 };
			geometry_info_t dmy = { 0 };

			//>>> FIXME
			img_eis_calc_win_pos(&ssr, &dmy, eis_offset_info, rsc_skew_info);
			_eis_coeff[0]->hor_skew_phase_inc = 0;
			_eis_coeff[0]->zoom_y = dsp_zf_y;
			_eis_coeff[0]->dummy_window_width = 0;
			_eis_coeff[0]->dummy_window_height = 0;
			_eis_coeff[0]->dummy_window_x_left = dmy.start_x;
			_eis_coeff[0]->dummy_window_y_top = dmy.start_y;
			_eis_coeff[0]->actual_left_top_x = ssr.left_top_x
			        - (dmy.start_x << ZF_SFT);
			_eis_coeff[0]->actual_left_top_y = ssr.left_top_y
			        - (dmy.start_y << ZF_SFT);
			_eis_coeff[0]->actual_right_bot_x = ssr.right_bot_x
			        - (dmy.start_x << ZF_SFT);
			_eis_coeff[0]->actual_right_bot_y = ssr.right_bot_y
			        - (dmy.start_y << ZF_SFT);
//>>>FIXME
			_eis_coeff[0]->arm_sys_tim = 0xDEADBEEF;
			_eis_update->latest_eis_coeff_addr = latest_eis_ptr[0];
			clean_d_cache((void*)eis_enhance_turbo_buf, sizeof(eis_enhance_turbo_buf_t));
			//sleep(1);
			msleep(1000);
			_eis_update->latest_eis_coeff_addr = 0;
			_eis_update->arm_sys_tim = 0xDEADBEEF;
			clean_d_cache((void*)eis_enhance_turbo_buf, sizeof(eis_enhance_turbo_buf_t));
		}
		memset(gyro_log.chan[GYRO_RAW_X].data, 0,
			gyro_log.chan[0].ndata * sizeof(s32));
		memset(gyro_log.chan[GYRO_RAW_Y].data, 0,
			gyro_log.chan[0].ndata * sizeof(s32));
		memset(gyro_log.chan[EIS_BUFFER_OFFSET_X_CHAN].data, 0,
			gyro_log.chan[0].ndata * sizeof(s32));
		memset(gyro_log.chan[EIS_BUFFER_OFFSET_Y_CHAN].data, 0,
			gyro_log.chan[0].ndata * sizeof(s32));
		gyro_log.chan[0].head = 0;
		gyro_param.Offset_x = gyro_log.chan[EIS_BUFFER_OFFSET_X_CHAN].data[0] =
		        gyro_param_bak.Offset_x;
		gyro_param.Offset_y = gyro_log.chan[EIS_BUFFER_OFFSET_Y_CHAN].data[0] =
		        gyro_param_bak.Offset_y;
		memset(gyro_log.chan[EIS_BUFFER_AMV_X_CHAN].data, 0,
			gyro_log.chan[0].ndata * sizeof(s32));
		memset(gyro_log.chan[EIS_BUFFER_AMV_Y_CHAN].data, 0,
			gyro_log.chan[0].ndata * sizeof(s32));
	}

	done: return rval;
}*/

static int img_eis_start_gyro_task(u8 rate)
{
	now_frate = 0;
	img_eis_set_adc_sampling_rate(rate);

	return 0;
}

static int img_eis_enable(void)
{
	s32 rval = 0;

	if (G_eis_obj.enable) {
		rval = -1;
		goto done;
	}

	if (!G_eis_obj.enable_cmd) {
		amba_vin_source_cmd(0, 0, AMBA_VIN_SRC_GET_EIS_INFO, &vin_eis_info);
		flg_is_non_support = 0;
		if (((vin_eis_info.main_fps >> 9) / 1000) > 60)
			flg_is_non_support = 1;

		if (flg_is_non_support) {
			if ((!G_eis_obj.enable_pref) && (!G_eis_obj.rsc_enable_pref)) {
				G_eis_obj.enable_pref = 1;
				G_eis_obj.rsc_enable_pref = 1;
			}
			rval = -1;
			goto done;
		}
	}

	if (G_eis_obj.stabilizer_type == EIS) {
		if (!G_eis_obj.enable_cmd) {
			G_eis_obj.enable_cmd = 1;
			bias_adjust_bit_shift = EIS_BIAS_ADJ_PRE_SET;
			//eis_pipe_vin_cnt = 0;
			img_eis_start_gyro_task(video_sampling_rate);
		}

		if (!Eis_enable_dly_cnt) {
			bias_adjust_bit_shift = EIS_BIAS_ADJ_POST_SET;
			G_eis_obj.enable = eis_status.enable = 1;
			img_eis_range_update();
			G_eis_obj.mc_enable = eis_status.mc_enable = 1;

			// default set RSC enabled
			G_eis_obj.rsc_enable = eis_status.rsc_enable = 1;
			G_eis_obj.dbg_port_enable = 1;
			G_eis_obj.enable_cmd = 0;
		} else {
			if ((!G_eis_obj.enable_pref) && (!G_eis_obj.rsc_enable_pref)) {
				G_eis_obj.enable_pref = 1;
				G_eis_obj.rsc_enable_pref = 1;
			}
		}
	}

	done: return rval;
}

static int img_eis_save_pref(u8 pref_1, u8 pref_2)
{
	G_eis_obj.enable_pref = pref_1;
	G_eis_obj.rsc_enable_pref = pref_2;
	return 0;
}

static int img_eis_set_mc_enable(u8 enable)
{
	G_eis_obj.mc_enable = enable;
	eis_status.mc_enable = enable;

	return 0;
}

static int img_eis_set_rsc_enable(u8 enable)
{
	G_eis_obj.rsc_enable = enable;
	eis_status.rsc_enable = enable;

	return 0;
}

static void eis_pipe_vin(void)
{
	static u32 eis_pipe_vin_cnt = 0;

	if ((!(G_eis_obj.disable_cmd || G_eis_obj.enable_cmd))
	        || flg_is_non_support)
		return;

	if (G_eis_obj.enable_cmd) {
		if (Eis_enable_dly_cnt > 0) {
			if (eis_pipe_vin_cnt <= Eis_enable_dly_cnt) {
				eis_pipe_vin_cnt++;
				return;
			}
			if (!G_eis_obj.proc) {
				eis_pipe_vin_cnt = Eis_enable_dly_cnt = 0;
				img_eis_enable();
				if (!G_eis_obj.enable_pref)
					img_eis_set_mc_enable(G_eis_obj.enable_pref);
				if (!G_eis_obj.rsc_enable_pref)
					img_eis_set_rsc_enable(G_eis_obj.rsc_enable_pref);
				img_eis_save_pref(0, 0);
			}
		}
	}
}

//#define TEST_DBG
static void eis_handler(gyro_data_t *data)
{
	static u8 Curr_eis_coeff_ptr = 0, update_eis_status_do = 0;
	static u8 update_eis_status_dont = 0;
	static u8 first_read_dsp_sys_tim = 0;
	static s32 prev_lim_l = 0, sample_offset = 0;
	static s32 skew_x = 0, skew_y = 0, gyro_value_x_old = 0;
	static s32 gyro_value_y_old = 0;
	static u32 first_gyro_point = 0, pre_first_gyro_point = 0;
	static u32 pre2_first_gyro_point = 0, gyro_point = 0;
	static u32 old_shutter_time = 0, index = 0, ftime_sft = 0, exposure_time;
	static u32 vin_isr_cnt = 0, eis_upd_buf[EIS_UPDATE_ADV_TIME + 1][3];
	static u32 dbg_wr_start_time = 0, dbg_wr_end_time = 0;
	static struct timeval eis_vin_time = { 0 };

	s8 sign;
	u8 Dbg_port_safe_zone_top = 0, Dbg_port_safe_zone_bottom = 0;
	u8 eis_en = 0, mc_en = 0, rsc_en = 0, dbg_port_en = 0, me_en = 0;
	u8 dbg_port_do = 0, enhance_eis_do;
	s32 gyro_value_x, gyro_value_y, diff_2, adj_last_ptr;
	u32 v_sreg[2], sreg_idx, pre_index, dsp_sys_tim = -1;
	u32 diff_time = 0, first_line_time, j, flg_PIV_status;
	eis_move_t motion_vector = { 0 }, rsc_mov, rsc_skew;
	eis_move_t eis_mov = {0}, rsc_lim_pos = {0}, rsc_lim_neg = {0};
	idsp_crop_info_t ssr = { 0 };
	geometry_info_t dmy = { 0 };
	struct timeval vin_time, sample_time;
	u64	tmp;
	static u8 preview = 0;

	if (!preview) {
		vin_time = vcap_time;

		if ((vin_time.tv_usec != eis_vin_time.tv_usec)
	        || (vin_time.tv_sec != eis_vin_time.tv_sec)) {
			eis_vin_time.tv_sec = vin_time.tv_sec;
			eis_vin_time.tv_usec = vin_time.tv_usec;
			preview = 1;

			img_eis_mode_init();
			img_eis_enable();
		}

		return;
	}

	pre_index = index;
	index = gyro_log.chan[0].head;
	vin_time = vcap_time;
	if ((vin_time.tv_usec != eis_vin_time.tv_usec)
	        || (vin_time.tv_sec != eis_vin_time.tv_sec)) {
		u8 upd_fl = 0, upd_ftime = 0;

		vin_isr_cnt++;
		eis_vin_time.tv_sec = vin_time.tv_sec;
		eis_vin_time.tv_usec = vin_time.tv_usec;
		pre2_first_gyro_point = pre_first_gyro_point;
		pre_first_gyro_point = first_gyro_point;
		first_gyro_point = index;

		eis_pipe_vin();

		// compute new exposure time if shutter index is changed.
		amba_vin_source_cmd(0, 0, AMBA_VIN_SRC_GET_EIS_INFO, &vin_eis_info);
		if (old_shutter_time != vin_eis_info.current_shutter_time) {
			exposure_time = (vin_eis_info.current_shutter_time >> 9) / 1000;
			old_shutter_time = vin_eis_info.current_shutter_time;
		}

		if (now_frate != vin_eis_info.current_fps) {
			now_frate = vin_eis_info.current_fps;
			vertical_blank_time = vin_eis_info.vb_lines * vin_eis_info.row_time
			        / 1000000;

			curr_ftime = (vin_eis_info.current_fps >> 9) / 1000 + 1;
			ftime_sft = (vin_eis_info.current_fps >> 9 << EIS_BIT_SHIFT) / 1000;
			Eis_update_end_time_1 =
			        G_eis_obj.adc_sampling_rate == 1 ?
			                Def_eis_update_end_time - EIS_UPDATE_ADV_TIME :
			                Def_eis_update_end_time
			                        - (G_eis_obj.adc_sampling_rate << 1);
			Eis_update_end_time_2 = Def_eis_update_end_time;
			Eis_update_end_time_1 += sensor_active_time;
			Eis_update_end_time_2 += sensor_active_time;

			if (Eis_update_end_time_1 > curr_ftime)
				Eis_update_end_time_1 -= curr_ftime;
			if (Eis_update_end_time_2 > curr_ftime)
				Eis_update_end_time_2 -= curr_ftime;
			upd_ftime = 1;
			dbg_wr_end_time = sensor_active_time - Dbg_port_safe_zone_bottom;
		}

		if (upd_ftime || upd_fl) {
			eis_mv.max_pix_x = eis_mv.max_ang_x * focal_value * curr_ftime
			        / eis_param.sensor_cell_size_x;
			eis_mv.max_pix_x >>= EIS_BIT_SHIFT;
			eis_mv.max_pix_y = eis_mv.max_ang_y * focal_value * curr_ftime
			        / eis_param.sensor_cell_size_y;
			eis_mv.max_pix_y >>= EIS_BIT_SHIFT;
		}
		dbg_port_do = 0;
		skew_x = skew_y = 0;
		v_sreg[0] = v_sreg[1] = 0;
	}

	memcpy(gyro_log.chan[GYRO_RAW_Z].data + index, &data->x, sizeof(s32));
	memcpy(gyro_log.chan[GYRO_RAW_Y].data + index, &data->y, sizeof(s32));
	memcpy(gyro_log.chan[GYRO_RAW_X].data + index, &data->z, sizeof(s32));
	do_gettimeofday(&sample_time);
	gyro_log.chan[0].head++;
	if (gyro_log.chan[0].head == gyro_log.chan[0].ndata)
		gyro_log.chan[0].head = 0;

	// calculate gyro bias
	gyro_log.chan[GYRO_RAW_X].data[index] <<= EIS_BIT_SHIFT;
	gyro_log.chan[GYRO_RAW_Y].data[index] <<= EIS_BIT_SHIFT;
	gyro_param.Offset_x = (((u64) gyro_param.Offset_x << bias_adjust_bit_shift)
	        - gyro_param.Offset_x + gyro_log.chan[GYRO_RAW_X].data[index])
	        >> bias_adjust_bit_shift;
	gyro_param.Offset_y = (((u64) gyro_param.Offset_y << bias_adjust_bit_shift)
	        - gyro_param.Offset_y + gyro_log.chan[GYRO_RAW_Y].data[index])
	        >> bias_adjust_bit_shift;
	gyro_log.chan[EIS_BUFFER_OFFSET_X_CHAN].data[gyro_log.chan[0].head] =
	        gyro_param.Offset_x;
	gyro_log.chan[EIS_BUFFER_OFFSET_Y_CHAN].data[gyro_log.chan[0].head] =
	        gyro_param.Offset_y;

////////////////////////////////////////////////////////////////////////////////
	eis_en = G_eis_obj.enable;
	mc_en = G_eis_obj.mc_enable;
	rsc_en = G_eis_obj.rsc_enable;
	me_en = G_eis_obj.me_enable;
	dbg_port_en = G_eis_obj.dbg_port_enable;
////////////////////////////////////////////////////////////////////////////////
	diff_time = (sample_time.tv_usec >= eis_vin_time.tv_usec) ?
		(sample_time.tv_usec - eis_vin_time.tv_usec) / 1000 :
		(sample_time.tv_usec + 1000000 - eis_vin_time.tv_usec) / 1000;

	// EIS correction starts
	enhance_eis_do = 0;
	if (!eis_en && me_en)
		enhance_eis_do = 2;
	if ((eis_en && mc_en) || enhance_eis_do) {
		if ((Eis_update_end_time_1 < Eis_update_end_time_2)
		        && (diff_time >= Eis_update_end_time_1)
		        && (diff_time <= Eis_update_end_time_2)) {
			enhance_eis_do |= 1;
			gyro_point = pre_first_gyro_point;
		} else if (Eis_update_end_time_1 > Eis_update_end_time_2) {
//>>>FIXME
			if (diff_time >= Eis_update_end_time_1) {
				enhance_eis_do |= 1;
				gyro_point = pre_first_gyro_point;
			} else if (diff_time <= Eis_update_end_time_2) {
				enhance_eis_do |= 1;
				gyro_point = pre2_first_gyro_point;
			}
		}
	} else {
		_eis_update->latest_eis_coeff_addr = 0;
		_eis_update->arm_sys_tim = 0xDEADBEEF;
		clean_d_cache((void*)eis_enhance_turbo_buf, sizeof(eis_enhance_turbo_buf_t));
	}

	if (enhance_eis_do & 0x1) {
		u8 flag_me_only = (enhance_eis_do & 0x2) ? 1 : 0;

		update_eis_status_do++;
		if (update_eis_status_do == 1) {
			update_eis_status_dont = first_read_dsp_sys_tim = 0;
			gyro_log.chan[0].lim_l = prev_lim_l;
			prev_lim_l = index;
			sample_offset =
			        eis_algo_calc_gyro_phase_offset(vertical_blank_time,
			        	vin_eis_info.cap_start_y + act_win_start_y
			        	+ (eis_offset_info.mov_y >> ZF_SFT),
			        	vin_eis_info.row_time, gyro_info.phs_dly_in_ms);

			diff_2 = (gyro_log.chan[0].lim_h > gyro_log.chan[0].lim_l) ?
				(gyro_log.chan[0].lim_h - gyro_log.chan[0].lim_l + 1) :
				(gyro_log.chan[0].lim_h + gyro_log.chan[0].ndata -
					gyro_log.chan[0].lim_l + 1);
			motion_vector.mov_x = gyro_info.gyro_x_polar *
				(eis_offset_info.mov_x >> (ZF_SFT - EIS_BIT_SHIFT));
			motion_vector.mov_y = gyro_info.gyro_y_polar *
				(eis_offset_info.mov_y >> (ZF_SFT - EIS_BIT_SHIFT));
			for (j = 0; j < diff_2; j ++) {
				gyro_value_x = gyro_log.chan[0].lim_l + j;
				if (gyro_value_x >= gyro_log.chan[0].ndata) {
					gyro_value_x -= gyro_log.chan[0].ndata;
				}
				gyro_log.chan[EIS_BUFFER_AMV_X_CHAN].data[gyro_value_x] = motion_vector.mov_x;
				gyro_log.chan[EIS_BUFFER_AMV_Y_CHAN].data[gyro_value_x] = motion_vector.mov_y;
			}
		}
		eis_algo_enhance_turbo(&gyro_log, gyro_point, index, exposure_time,
			&motion_vector, dbg_port_en, sample_offset, active_h,
			&eis_offset_info, &rsc_skew_info, flag_me_only, ftime_sft);

		if (gyro_info.gyro_x_polar == -1) {
			eis_offset_info.mov_x *= (-1);
			rsc_skew_info.mov_x *= (-1);
		}
		if (gyro_info.gyro_y_polar == -1) {
			eis_offset_info.mov_y *= (-1);
			if (!dbg_port_en)
				rsc_skew_info.mov_y *= (-1);
		}
		if (!rsc_en)
			rsc_skew_info.mov_x = rsc_skew_info.mov_y = 0;
		if (dbg_port_en)
			rsc_skew_info.mov_y = 0;

#ifdef TEST_DBG
{
		eis_move_t motion_vector2 = {0};

		eis_offset_info.mov_x = 0;
		if (eis_offset_info.mov_y < 0)
		eis_offset_info.mov_y = 0;
		img_eis_calc_win_pos(&ssr, &dmy, eis_offset_info, motion_vector2);
		_eis_coeff[Curr_eis_coeff_ptr]->hor_skew_phase_inc = motion_vector2.mov_x;
}
#else
		img_eis_calc_win_pos(&ssr, &dmy, eis_offset_info, rsc_skew_info);
		_eis_coeff[Curr_eis_coeff_ptr]->hor_skew_phase_inc =
		        rsc_skew_info.mov_x;
#endif
		if (dbg_port_en)
			_eis_coeff[Curr_eis_coeff_ptr]->zoom_y = dsp_zf_y;
		else {
#ifdef TEST_DBG
			_eis_coeff[Curr_eis_coeff_ptr]->zoom_y = dsp_zf_y;
#else
			tmp = ((u64) iav_eis_info.main_height << (ZF_SFT << 1));
			DO_DIV_ROUND(tmp, (ssr.right_bot_y - ssr.left_top_y));
			_eis_coeff[Curr_eis_coeff_ptr]->zoom_y = tmp;
#endif
		}
		_eis_coeff[Curr_eis_coeff_ptr]->dummy_window_width = 0;
		_eis_coeff[Curr_eis_coeff_ptr]->dummy_window_height = 0;
		_eis_coeff[Curr_eis_coeff_ptr]->dummy_window_x_left = dmy.start_x;
		_eis_coeff[Curr_eis_coeff_ptr]->dummy_window_y_top = dmy.start_y;
		_eis_coeff[Curr_eis_coeff_ptr]->arm_sys_tim = update_eis_status_do - 1;
		_eis_coeff[Curr_eis_coeff_ptr]->actual_left_top_x = ssr.left_top_x
		        - (dmy.start_x << ZF_SFT);
		_eis_coeff[Curr_eis_coeff_ptr]->actual_left_top_y = ssr.left_top_y
		        - (dmy.start_y << ZF_SFT);
		_eis_coeff[Curr_eis_coeff_ptr]->actual_right_bot_x = ssr.right_bot_x
		        - (dmy.start_x << ZF_SFT);
		_eis_coeff[Curr_eis_coeff_ptr]->actual_right_bot_y = ssr.right_bot_y
		        - (dmy.start_y << ZF_SFT);
		_eis_update->latest_eis_coeff_addr = latest_eis_ptr[Curr_eis_coeff_ptr];
		_eis_update->arm_sys_tim = 0xDEADBEEF;
		clean_d_cache((void*)eis_enhance_turbo_buf, sizeof(eis_enhance_turbo_buf_t));
		eis_upd_buf[update_eis_status_do - 1][0] = diff_time;
		eis_upd_buf[update_eis_status_do - 1][1] = ssr.left_top_x;
		eis_upd_buf[update_eis_status_do - 1][2] = ssr.left_top_y;

		Curr_eis_coeff_ptr++;
		if (Curr_eis_coeff_ptr == 2)
			Curr_eis_coeff_ptr = 0;
	} else {
		update_eis_status_dont++;
		if (update_eis_status_dont == 1) {
			update_eis_status_do = 0;
			gyro_log.chan[0].lim_h = pre_index;
		}
		if (eis_en) {
			invalidate_d_cache((void*)eis_enhance_turbo_buf, sizeof(eis_enhance_turbo_buf_t));
			dsp_sys_tim = _eis_update->arm_sys_tim;
			if ((dsp_sys_tim <= EIS_UPDATE_ADV_TIME)
			        && !first_read_dsp_sys_tim) {
				first_read_dsp_sys_tim = 1;
				eis_offset_info.mov_x = (s32) eis_upd_buf[dsp_sys_tim][1]
				        - act_win_zp.mov_x;
				eis_offset_info.mov_y = (s32) eis_upd_buf[dsp_sys_tim][2]
				        - act_win_zp.mov_y;
				first_line_time = vin_eis_info.row_time
				        * (vin_eis_info.cap_start_y + act_win_start_y
				                + (eis_offset_info.mov_y >> ZF_SFT)) / 1000000;
				if (first_line_time < Dbg_port_safe_zone_top)
					first_line_time = Dbg_port_safe_zone_top;
				dbg_wr_start_time = first_line_time + vertical_blank_time;
			}
		}
	}

	// RSC correction (debug port) starts
	if ((eis_en) && (rsc_en) && (dbg_port_en)) {
		dbg_port_do++;
		if ((diff_time < dbg_wr_start_time) || (diff_time > dbg_wr_end_time))
			dbg_port_do = 0;

		invalidate_d_cache((void*)eis_enhance_turbo_buf, sizeof(eis_enhance_turbo_buf_t));
		flg_PIV_status = _eis_update->flg_PIV_status;
		if (dbg_port_do && !flg_PIV_status) {
			if (dbg_port_do == 1)
				eis_mov.mov_x = eis_offset_info.mov_x >> (ZF_SFT - EIS_BIT_SHIFT);

			if (G_eis_obj.adc_sampling_rate > 1)
				adj_last_ptr = (s32) index
				        + (gyro_info.phs_dly_in_ms - (exposure_time >> 1))
				                / G_eis_obj.adc_sampling_rate;
			else
				adj_last_ptr = (s32) index + gyro_info.phs_dly_in_ms
				        - (exposure_time >> 1);

			if (adj_last_ptr > index)
				adj_last_ptr = index;
			else if (adj_last_ptr < 0)
				adj_last_ptr += gyro_log.chan[0].ndata;

			gyro_value_x =
			        gyro_log.chan[GYRO_RAW_X].data[adj_last_ptr]
			                - gyro_log.chan[EIS_BUFFER_OFFSET_X_CHAN].data[adj_last_ptr];
			if (skew_x && gyro_value_x_old) {
				sign = 1;
				if (gyro_value_x < 0)
					sign = -1;
				rsc_mov.mov_x = ABS(skew_x);
				tmp = ((u64)(rsc_mov.mov_x) * ABS(gyro_value_x));
				DO_DIV_ROUND(tmp, ABS(gyro_value_x_old));
				rsc_mov.mov_x = tmp;
				rsc_mov.mov_x *= sign;
			} else {
				rsc_mov.mov_x = eis_algo_calc_mv(0, gyro_value_x, ftime_sft);
				skew_x = rsc_mov.mov_x;
				gyro_value_x_old = gyro_value_x;
			}
			rsc_mov.mov_x = (rsc_mov.mov_x * rsc_strength_x) / 100;
			rsc_skew.mov_x =
			        (rsc_mov.mov_x * (int) active_h)
			                / (int) (vin_eis_info.source_height
			                        + vin_eis_info.vb_lines);

			if (eis_mov.mov_x >= 0) {
				rsc_lim_pos.mov_x = eis_lim_p.mov_x + rsc_lim_p.mov_x - eis_mov.mov_x;
				rsc_lim_neg.mov_x = eis_lim_n.mov_x + rsc_lim_n.mov_x;
			} else {
				rsc_lim_pos.mov_x = eis_lim_p.mov_x + rsc_lim_p.mov_x;
				rsc_lim_neg.mov_x = eis_lim_n.mov_x + rsc_lim_n.mov_x + eis_mov.mov_x;
			}
			if (rsc_skew.mov_x > rsc_lim_pos.mov_x)
				rsc_skew.mov_x = rsc_lim_pos.mov_x;
			else if (rsc_skew.mov_x < -rsc_lim_neg.mov_x)
				rsc_skew.mov_x = -rsc_lim_neg.mov_x;

			rsc_skew.mov_x = (rsc_skew.mov_x << (ZF_SFT - EIS_BIT_SHIFT))
			        / (int) active_h;
			// consider cfa zf
			sign = 1;
			if (rsc_skew.mov_x < 0) {
				rsc_skew.mov_x *= (-1);
				sign = -1;
			}
			tmp = (u64) rsc_skew.mov_x * cfa_zf_x;
			DO_DIV_ROUND(tmp, cfa_zf_y);
			rsc_skew.mov_x = tmp;
			if (sign == -1)
				rsc_skew.mov_x *= (-1);
			if (gyro_info.gyro_x_polar == -1)
				rsc_skew.mov_x *= (-1);

			if (hori_skew_dbg_port(rsc_skew.mov_x, v_sreg) == 0) {
				gyro_value_y =
				        gyro_log.chan[GYRO_RAW_Y].data[adj_last_ptr]
				                - gyro_log.chan[EIS_BUFFER_OFFSET_Y_CHAN].data[adj_last_ptr];
				if (skew_y && gyro_value_y_old) {
					sign = 1;
					if (gyro_value_y < 0)
						sign = -1;
					rsc_mov.mov_y = ABS(skew_y);
					tmp = ((u64)(rsc_mov.mov_y) * ABS(gyro_value_y));
					DO_DIV_ROUND(tmp, ABS(gyro_value_y_old));
					rsc_mov.mov_y = tmp;
					rsc_mov.mov_y *= sign;
				} else {
					rsc_mov.mov_y =
					        eis_algo_calc_mv(1, gyro_value_y, ftime_sft);
					skew_y = rsc_mov.mov_y;
					gyro_value_y_old = gyro_value_y;
				}
				rsc_mov.mov_y = (rsc_mov.mov_y * rsc_strength_y) / 100;
				rsc_skew.mov_y = (rsc_mov.mov_y * (int) active_h)
				        / (int) (vin_eis_info.source_height
				                + vin_eis_info.vb_lines);
				if (rsc_skew.mov_y > rsc_lim_p.mov_y)
					rsc_skew.mov_y = rsc_lim_p.mov_y;
				else if (rsc_skew.mov_y < -rsc_lim_n.mov_y)
					rsc_skew.mov_y = -rsc_lim_n.mov_y;

				rsc_skew.mov_y = (rsc_skew.mov_y << (ZF_SFT - EIS_BIT_SHIFT))
				        / (int) active_h;
				if (gyro_info.gyro_y_polar == -1)
					rsc_skew.mov_y *= (-1);

				sreg_idx = (v_sreg[0] == 0) ? 1 : 0;
				tmp = ((u64) 1 << (ZF_SFT << 1));
				DO_DIV_ROUND(tmp, sec2_dsp_zf_y);
				diff_2 = tmp;

				sign = 1;
				if (rsc_skew.mov_y < 0) {
					sign = -1;
					rsc_skew.mov_y *= (-1);
				}
				tmp = ((u64) rsc_skew.mov_y << ZF_SFT);
				DO_DIV_ROUND(tmp, sec2_dsp_zf_y);
				rsc_skew.mov_y = tmp;
				if (sign == -1)
					rsc_skew.mov_y *= (-1);

				vert_skew_dbg_port_2(rsc_skew.mov_y + diff_2, rsc_skew.mov_y
				        + diff_2, sreg_idx);
			}
		}
	}


	/** copy gyro sampling data to another memory for logging */
	if (flg_eis_gyro_log && (eis_gyro_log != NULL )) {
		u32 i;

		for (i = 0; i <= EIS_BUFFER_OFFSET_Y_CHAN; i++)
			eis_gyro_log->chan[i].data[eis_gyro_log->chan[0].head] =
			        gyro_log.chan[i].data[index];
		eis_gyro_log->chan[GYRO_RAW_X].data[eis_gyro_log->chan[0].head] >>=
		        EIS_BIT_SHIFT;
		eis_gyro_log->chan[GYRO_RAW_Y].data[eis_gyro_log->chan[0].head] >>=
		        EIS_BIT_SHIFT;
		if (flg_eis_gyro_log == 1) {
			eis_gyro_log->chan[GYRO_RAW_Z].data[eis_gyro_log->chan[0].head] =
			        sample_time.tv_usec / 1000;
			eis_gyro_log->chan[GYRO_RAW_T].data[eis_gyro_log->chan[0].head] =
			        vin_isr_cnt;
		} else {
			eis_gyro_log->chan[EIS_BUFFER_AMV_X_CHAN].data[eis_gyro_log->chan[0].head] =
			        sample_time.tv_usec / 1000;
			eis_gyro_log->chan[EIS_BUFFER_AMV_Y_CHAN].data[eis_gyro_log->chan[0].head] =
			        vin_isr_cnt;
		}
#ifdef EIS_DBG_DSP_SYS_TIM
		eis_gyro_log->chan[EIS_BUFFER_OFFSET_X_CHAN].data[eis_gyro_log->chan[0].head] = update_eis_status_do;
		eis_gyro_log->chan[EIS_BUFFER_OFFSET_Y_CHAN].data[eis_gyro_log->chan[0].head] = diff_time;
		eis_gyro_log->chan[EIS_BUFFER_AMV_X_CHAN].data[eis_gyro_log->chan[0].head] =
			(dsp_sys_tim <= EIS_UPDATE_ADV_TIME) ? eis_upd_buf[dsp_sys_tim][0] : -1;
#endif
		eis_gyro_log->chan[0].head++;
		if (eis_gyro_log->chan[0].head >= eis_gyro_log_cnt) {
			printk("--- GYRO LOG FINISH! ---\n");
			flg_eis_gyro_log = 0;
		}
	}
}

void init_eis_buf(void)
{
	eis_enhance_turbo_buf	= (u8 *)&pinfo->share_data->enhance_turbo_buf;
	_eis_coeff[0]		= &pinfo->share_data->enhance_turbo_buf.coeff_info[0];
	_eis_coeff[1]		= &pinfo->share_data->enhance_turbo_buf.coeff_info[1];
	_eis_update		= &pinfo->share_data->enhance_turbo_buf.update_info;
	latest_eis_ptr[0]	= ambarella_virt_to_phys((u32)_eis_coeff[0]);
	latest_eis_ptr[1]	= ambarella_virt_to_phys((u32)_eis_coeff[1]);
}

void eis_algo_init(void)
{
	apb_addr = get_ambarella_apb_virt();
	init_eis_buf();
	gyro_get_info(&gyro_info);
	G_eis_obj.stabilizer_type	= EIS;
	eis_status.stabilizer_type	= EIS;
}

