#ifndef __CUSTOM_AF_ALGO_H__
#define __CUSTOM_AF_ALGO_H__

#define DRAGON_SIZE 32

typedef struct af_result_s{
	s32 dist_idx_src;
	s32 dist_idx_dest;
	u16 pps;
}af_result_t;

typedef enum{
	INIT_AF =	0,
	DO_AF,
	INIT_Watch,
	Watch,
}af_stm_t;

/*
* calculate the statictis data
*/
typedef struct dragon_s{
	u32 occ_flag[DRAGON_SIZE];
	u32 fv[DRAGON_SIZE];
	u32 luma_tile_diff[DRAGON_SIZE];
	u32 luma_avg[DRAGON_SIZE];
	u32 dist_idx[DRAGON_SIZE];
	s16 fv_store_idx;
	u32 store_cnt;
	s32 fv_rev_fac;
	s32 fv_rev_TH;
	s32 fv_max_TH;
	u16 sec_chg_val;
}af_dragon_t;

typedef struct af_algo_param_s{
	af_stm_t af_stm;
	s8 direction;
}af_algo_param_t;

typedef struct zoom_s {
	s32 a;
	s32 b;
	u32 zoom_pluse;
} zoom_t;

int custom_af_control_init(af_control_t* p_af_control, void* p_af_param, void* p_zoom_map, lens_control_t* p_lens_ctrl);
int custom_af_control(af_control_t* p_af_control, u16 af_tile_count, af_stat_t* p_af_info, u16 awb_tile_count, awb_data_t* p_awb_info, lens_control_t * p_lens_ctrl, ae_info_t* p_ae_info, u8 lens_runing_flag);
void custom_af_set_range(af_range_t* p_af_range);
void custom_af_set_calib_param(void* p_calib_param);
#endif
