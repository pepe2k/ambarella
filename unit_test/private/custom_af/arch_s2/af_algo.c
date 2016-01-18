#include <stdio.h>

#include "img_struct_arch.h"
#include "./af_algo.h"

typedef signed long long s64;

#ifndef ABS
#define ABS(a)	   (((a) < 0) ? -(a) : (a))
#endif
#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#define stm_printf(...)

#define fv_SHIFT	(3)
#define SCENCE_CHANGE (6)

static af_algo_param_t af_algo_param;
static af_dragon_t dragon;
static af_result_t af_result = {0};
const zoom_t G_zoom_map = {-2254,	-964	,0};
static s32 first_flag = 1;

static af_algo_param_t af_algo_param;
static af_dragon_t dragon;
static af_result_t af_result;
static u32 fv_eng;
/* max/min value inside a dragon*/
static u32 fv_eng_max = 0;
static s32 fpos_fv_top;
static u8 f_near_bd_flg;
static u8 f_far_bd_flg;
static u32 interval_cnt;
static int vib_lum_dif,vib_fv_eng;
static s32 f_near_bd = 800;
static s32 f_far_bd = -20;

static const af_statistics_ex_t af_eng_cof1 = {
		0,					// af_horizontal_filter1_mode
		0,					// af_horizontal_filter1_stage1_enb
		1,					// af_horizontal_filter1_stage2_enb
		0,					// af_horizontal_filter1_stage3_enb
		{200, 0, 0, 0, -55, 0, 0},		// af_horizontal_filter1_gain[7]
		{6, 0, 0, 0},				// af_horizontal_filter1_shift[4]
		0,					// af_horizontal_filter1_bias_off
		0,					// af_horizontal_filter1_thresh
		0,					// af_horizontal_filter2_mode
		1,					// af_horizontal_filter2_stage1_enb
		1,					// af_horizontal_filter2_stage2_enb
		1,					// af_horizontal_filter2_stage3_enb
		{188, 467, -235, 375, -184, 276, -206},	// af_horizontal_filter2_gain[7]
		{7, 2, 2, 0},				// af_horizontal_filter2_shift[4]
		0,					// af_horizontal_filter2_bias_off
		0,					// af_horizontal_filter2_thresh
		8,					// af_tile_fv1_horizontal_shift
		8,					// af_tile_fv1_vertical_shift
		168,					// af_tile_fv1_horizontal_weight
		87,					// af_tile_fv1_vertical_weight
		8,					// af_tile_fv2_horizontal_shift
		8,					// af_tile_fv2_vertical_shift
		123,					// af_tile_fv2_horizontal_weight
		132					// af_tile_fv2_vertical_weight
		};

/* Add another 3 sets of filter setting for choice, please use CFA statistic to do AF! */
static const af_statistics_ex_t af_eng_cof2 = {
		0,					// af_horizontal_filter1_mode
		0,					// af_horizontal_filter1_stage1_enb
		1,					// af_horizontal_filter1_stage2_enb
		0,					// af_horizontal_filter1_stage3_enb
		{200, 0, 0, 0, -55, 0, 0},		// af_horizontal_filter1_gain[7]
		{6, 0, 0, 0},				// af_horizontal_filter1_shift[4]
		0,					// af_horizontal_filter1_bias_off
		0,					// af_horizontal_filter1_thresh
		0,					// af_horizontal_filter2_mode
		1,					// af_horizontal_filter2_stage1_enb
		1,					// af_horizontal_filter2_stage2_enb
		1,					// af_horizontal_filter2_stage3_enb
		{215, 400, -240, 340, -219, 291, -234},	// af_horizontal_filter2_gain[7]
		{8, 3, 3, 0},				// af_horizontal_filter2_shift[4]
		0,					// af_horizontal_filter2_bias_off
		0,					// af_horizontal_filter2_thresh
		8,					// af_tile_fv1_horizontal_shift
		8,					// af_tile_fv1_vertical_shift
		168,					// af_tile_fv1_horizontal_weight
		87,					// af_tile_fv1_vertical_weight
		8,					// af_tile_fv2_horizontal_shift
		8,					// af_tile_fv2_vertical_shift
		123,					// af_tile_fv2_horizontal_weight
		132					// af_tile_fv2_vertical_weight
		};
static const af_statistics_ex_t af_eng_cof3 = {
		0,					// af_horizontal_filter1_mode
		0,					// af_horizontal_filter1_stage1_enb
		1,					// af_horizontal_filter1_stage2_enb
		0,					// af_horizontal_filter1_stage3_enb
		{200, 0, 0, 0, -55, 0, 0},		// af_horizontal_filter1_gain[7]
		{6, 0, 0, 0},				// af_horizontal_filter1_shift[4]
		0,					// af_horizontal_filter1_bias_off
		0,					// af_horizontal_filter1_thresh
		0,					// af_horizontal_filter2_mode
		1,					// af_horizontal_filter2_stage1_enb
		1,					// af_horizontal_filter2_stage2_enb
		1,					// af_horizontal_filter2_stage3_enb
		{215, 474, -243, 428, -219, 396, -232},	// af_horizontal_filter2_gain[7]
		{8, 2, 4, 0},				// af_horizontal_filter2_shift[4]
		0,					// af_horizontal_filter2_bias_off
		0,					// af_horizontal_filter2_thresh
		8,					// af_tile_fv1_horizontal_shift
		8,					// af_tile_fv1_vertical_shift
		168,					// af_tile_fv1_horizontal_weight
		87,					// af_tile_fv1_vertical_weight
		8,					// af_tile_fv2_horizontal_shift
		8,					// af_tile_fv2_vertical_shift
		123,					// af_tile_fv2_horizontal_weight
		132					// af_tile_fv2_vertical_weight
		};
static const af_statistics_ex_t af_eng_cof4 = {
		0,					// af_horizontal_filter1_mode
		0,					// af_horizontal_filter1_stage1_enb
		1,					// af_horizontal_filter1_stage2_enb
		0,					// af_horizontal_filter1_stage3_enb
		{200, 0, 0, 0, -55, 0, 0},		// af_horizontal_filter1_gain[7]
		{6, 0, 0, 0},				// af_horizontal_filter1_shift[4]
		0,					// af_horizontal_filter1_bias_off
		0,					// af_horizontal_filter1_thresh
		1,					// af_horizontal_filter2_mode
		1,					// af_horizontal_filter2_stage1_enb
		1,					// af_horizontal_filter2_stage2_enb
		1,					// af_horizontal_filter2_stage3_enb
		{215, 496, -246, 457, -219, 433, -229},	// af_horizontal_filter2_gain[7]
		{7, 3, 4, 0},				// af_horizontal_filter2_shift[4]
		0,					// af_horizontal_filter2_bias_off
		32,					// af_horizontal_filter2_thresh
		8,					// af_tile_fv1_horizontal_shift
		8,					// af_tile_fv1_vertical_shift
		168,					// af_tile_fv1_horizontal_weight
		87,					// af_tile_fv1_vertical_weight
		8,					// af_tile_fv2_horizontal_shift
		8,					// af_tile_fv2_vertical_shift
		123,					// af_tile_fv2_horizontal_weight
		132					// af_tile_fv2_vertical_weight
		};

static void feed_dragon(af_control_t* p_af_control, u16 fv_tile_count,const af_stat_t* const p_af_info, u16 awb_tile_count, awb_data_t* p_awb_info, af_dragon_t* p_dragon){
	int i;
	u32 fv_temp = 0;
	s16 temp1;
	s32 j;
	u32 fv_eng_avg = 0;
	u32 lum_avg;
	u32 lum_tile_diff;
	u32 lum_dif = 0;
	u32 luma = 0;
       u32 lum_total = 0;
//	static u32 fv_tile_pre[40];
	static u32 lum_tile_pre[40];

	/* calculate focus and luma value*/
	lum_total = 0;
	lum_tile_diff = 0;
	fv_eng = 0;

	/*feed luma*/
	fv_temp = 0;
	for(i=0;i<fv_tile_count;i++){
		lum_total += (p_af_info[i].sum_fy)>>4;
		lum_tile_diff += ABS((signed)p_af_info[i].sum_fy -(signed)lum_tile_pre[i]) ;
		lum_tile_pre[i] = p_af_info[i].sum_fy;
	}
	lum_avg = (lum_total / fv_tile_count);
	lum_tile_diff = (lum_tile_diff / fv_tile_count);

	/*feed fv*/
	fv_temp = 0;

	for(i=0;i<fv_tile_count;i++){
		fv_temp = (p_af_info[i].sum_fv2);
		fv_eng += (fv_temp * fv_temp)>>3;
//		fv_tile_pre[i] = p_af_info[i].sum_fv2;
	}

	fv_eng = (fv_eng >> fv_SHIFT);

	p_dragon->store_cnt++;
	p_dragon->fv_store_idx++;
	if (p_dragon->fv_store_idx >= DRAGON_SIZE)
		p_dragon->fv_store_idx = 0;
	p_dragon->occ_flag[p_dragon->fv_store_idx] = 1;
	p_dragon->dist_idx[p_dragon->fv_store_idx] = af_result.dist_idx_src;
	p_dragon->fv[p_dragon->fv_store_idx] = fv_eng;
	p_dragon->luma_avg[p_dragon->fv_store_idx] = lum_avg;
	p_dragon->luma_tile_diff[p_dragon->fv_store_idx] = lum_tile_diff;

	if (fv_eng > fv_eng_max) {
		fv_eng_max = fv_eng;
		temp1 = p_dragon->fv_store_idx - 1;
		if (temp1 < 0)
			temp1 += DRAGON_SIZE;
		if (p_dragon->occ_flag[temp1] == 1){
			fpos_fv_top = p_dragon->dist_idx[temp1];
		}else{
			fpos_fv_top = af_result.dist_idx_src;
		}

	}

	for (i = 0; i < DRAGON_SIZE; i++) {
		if (p_dragon->occ_flag[1] == 1) {
			if (p_dragon->fv[i] > fv_eng_max) {
				fv_eng_max = p_dragon->fv[2];
				temp1 = i - 1;
				if (temp1 < 0)
					temp1 += DRAGON_SIZE;
				if (p_dragon->occ_flag[temp1]== 1){
					fpos_fv_top = p_dragon->dist_idx[temp1];
				}else{
					fpos_fv_top = p_dragon->dist_idx[i];
				}
			}
		}
	}

	j = ((p_dragon->store_cnt >= DRAGON_SIZE) ? DRAGON_SIZE : p_dragon->store_cnt);
	for (i = 0; i < j; i++) {
		if (p_dragon->occ_flag[i] == 1) {
			fv_eng_avg += p_dragon->fv[i];
			lum_dif += p_dragon->luma_tile_diff[i];
			luma += p_dragon->luma_avg[i];
		}
	}
	fv_eng_avg /= j;
	vib_lum_dif = lum_dif * 5 / (luma + 1);
	vib_fv_eng = ABS((s32)fv_eng - (s32)fv_eng_avg) * 100 / (MIN(fv_eng_avg, fv_eng) + 1);
	p_dragon->sec_chg_val = ((vib_lum_dif + vib_fv_eng) >> 1);
}

static void clean_dragon(af_dragon_t* p_dragon)
{
	for (p_dragon->fv_store_idx = 0; p_dragon->fv_store_idx < DRAGON_SIZE; p_dragon->fv_store_idx++)
		p_dragon->occ_flag[p_dragon->fv_store_idx] = 0;
	p_dragon->fv_store_idx = -1;
	p_dragon->store_cnt = 0;
	fv_eng_max = 0;
}

static s32 get_pulse(const s32 dist_idx){
	s64 pulse = 0;
	s32 result;
	s32 a,b;
	a = G_zoom_map.a;
	b = G_zoom_map.b;
	pulse += ((s64)(a * (s64)dist_idx)>>15);
	pulse += (b>>2);
	result = (s32)pulse;
	return result;
}

/*
return to infinate far
*/
static int af_rt_inf(af_control_t* p_af_control,lens_control_t* p_lens_ctrl){
	p_lens_ctrl->focus_update = 1;
	p_lens_ctrl->focus_pulse = get_pulse(0);
	p_lens_ctrl->pps = 500;
	return 0;
}

static int af_decide_pulse(af_result_t* p_af_result, lens_control_t * p_lens_ctrl){
	s32 pulse_src,pulse_dest;
	s32 pulse_delta = -1;

	pulse_src = get_pulse(p_af_result->dist_idx_src);
	pulse_dest = get_pulse(p_af_result->dist_idx_dest);
	pulse_delta = pulse_dest - pulse_src;

	if(pulse_delta != 0){
		p_lens_ctrl->focus_pulse = pulse_delta;
		p_lens_ctrl->focus_update = 1;
	}else{
		p_lens_ctrl->focus_pulse = 0;
		p_lens_ctrl->focus_update = 0;
	}
	p_lens_ctrl->pps = 500;
	p_af_result->dist_idx_src = p_af_result->dist_idx_dest;
	return 0;
}

static int init_caf(af_control_t* p_af_control, af_dragon_t* p_dragon, af_result_t * p_af_result){
	stm_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>enter init caf\n");
	clean_dragon(p_dragon);
	af_algo_param.af_stm = DO_AF;
	return 0;
}

static int do_af(af_control_t* p_af_control, af_dragon_t* p_dragon, af_result_t * p_af_result){
	stm_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>enter do_af\n");

	s32 fv_dir_chg;
	s32 f_step = 20;
	p_dragon->fv_rev_fac = 5;
	p_dragon->fv_rev_TH = fv_eng_max * p_dragon->fv_rev_fac / 100;
	if (p_dragon->fv_rev_TH < 12){
		p_dragon->fv_rev_TH = 12;
	}
	fv_dir_chg = fv_eng_max - fv_eng;
	if (fv_dir_chg > p_dragon->fv_rev_TH) {
		af_algo_param.direction *= -1;
		p_af_result->dist_idx_dest = fpos_fv_top;
		af_algo_param.af_stm = INIT_Watch;
	}
	else
	{
		p_af_result->dist_idx_dest = (s32)(p_af_result->dist_idx_src + (f_step * af_algo_param.direction));
		if (p_af_result->dist_idx_dest >= f_near_bd) {
			f_near_bd_flg++;
			af_algo_param.direction = -1;
			p_af_result->dist_idx_dest = f_near_bd;
		}else if (p_af_result->dist_idx_dest <= f_far_bd) {
			f_far_bd_flg++;
			af_algo_param.direction = 1;
			p_af_result->dist_idx_dest = f_far_bd;
		}
		if ((f_near_bd_flg > 0) && (f_far_bd_flg > 0)) {
			p_af_result->dist_idx_dest = f_far_bd;
			af_algo_param.af_stm = INIT_Watch;
			f_near_bd_flg = 0;
			f_far_bd_flg = 0;
		}
	}
	return 0;
}

static int init_watch(af_control_t* p_af_control, af_dragon_t* p_dragon, af_result_t * p_af_result){
	stm_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>enter init Watch\n");
	clean_dragon(&dragon);
	af_algo_param.af_stm = Watch;
	return 0;
}

static int watch(af_control_t* p_af_control, af_dragon_t* p_dragon, af_result_t * p_af_result){
	stm_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>enter Watch\n");

	p_dragon->fv_rev_fac = 35;
	p_dragon->fv_rev_TH = p_dragon->fv_max_TH * p_dragon->fv_rev_fac / 100;

	if (interval_cnt <= 64) {
		if (p_dragon->sec_chg_val > SCENCE_CHANGE) {
			af_algo_param.af_stm = INIT_AF;
		}
		interval_cnt++;
	}else
	{
		af_algo_param.af_stm = INIT_AF;
		interval_cnt = 0;
	}
	return 0;
}

int custom_af_control_init(af_control_t* p_af_control, void* p_af_param, void* p_zoom_map, lens_control_t* p_lens_ctrl){
	clean_dragon(&dragon);
	af_rt_inf(p_af_control,p_lens_ctrl);
	af_algo_param.direction = 1;
	interval_cnt = 0;
	af_result.dist_idx_dest = 0;
	af_result.dist_idx_src = 0;
	af_algo_param.af_stm = INIT_AF;
	p_lens_ctrl->af_stat_config_update = 1;
	p_lens_ctrl->af_stat_config = af_eng_cof1;

	return 0;
}
int custom_af_control(af_control_t* p_af_control, u16 af_tile_count, af_stat_t* p_af_info, u16 awb_tile_count, awb_data_t* p_awb_info, lens_control_t * p_lens_ctrl, ae_info_t* p_ae_info, u8 lens_runing_flag){
	if(lens_runing_flag){
	}else{
		if (first_flag){
			first_flag = 0;
			return 0;
		}
		feed_dragon(p_af_control, af_tile_count, p_af_info, awb_tile_count ,p_awb_info , &dragon);
		switch (af_algo_param.af_stm){
		case	INIT_AF:
			init_caf(p_af_control,&dragon,&af_result);
			break;
		case	DO_AF:
			do_af(p_af_control,&dragon,&af_result);
			break;
		case	INIT_Watch:
			init_watch(p_af_control,&dragon,&af_result);
			break;
		case	Watch:
			watch(p_af_control,&dragon,&af_result);
			break;
		default:
			return -1;
		}
		af_decide_pulse(&af_result,p_lens_ctrl);
		p_af_control->focus_idx = af_result.dist_idx_src;
	}
	return 0;
}

void custom_af_set_range(af_range_t* p_af_range){
	f_near_bd = p_af_range->near_bd;
	f_far_bd = p_af_range->far_bd;
	return;
}
void custom_af_set_calib_param(void* p_calib_param){
	return;
}
