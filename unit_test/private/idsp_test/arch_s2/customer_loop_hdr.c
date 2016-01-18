#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <getopt.h>
#include <sched.h>
#include <signal.h>
#include "basetypes.h"

#include "iav_drv.h"
#include "iav_drv_ex.h"

#define CUSTOMER_MAINLOOP
#include "ambas_imgproc_arch.h"
#include "ambas_vin.h"
#include "img_struct_arch.h"
#include "img_dsp_interface_arch.h"
#include "img_api_arch.h"
#include "img_hdr_api_arch.h"
#include "img_customer_interface_arch.h"

#include "mn34220pl_adj_param_hdr.c"
#include "mn34220pl_aeb_param_hdr.c"

#ifndef IMGPROC_PARAM_PATH
#define	IMGPROC_PARAM_PATH	"/etc/idsp"
#endif

#ifndef AWB_UNIT_GAIN
#define AWB_UNIT_GAIN	1024
#endif

#ifndef ABS
#define ABS(a)	   (((a) < 0) ? -(a) : (a))
#endif

hdr_proc_data_t  hdr_data_proc = {0};

static img_color_style rgb2yuv_style = IMG_COLOR_PC;

static awb_gain_t awb_gain;
static pipeline_control_t adj_pipe_ctl;
static adj_aeawb_control_t adj_aeawb_ctl;
static adj_param_t is_adj_param;
static rgb_to_yuv_t is_rgb2yuv[4];
static chroma_scale_filter_t is_chroma_scale;
static statistics_config_t is_tile_config;
static img_awb_param_t is_awb_param;

bayer_pattern pat;
u32 max_sensor_gain, gain_step, timing_mode;

line_t ae_lines[MAX_HDR_EXPOSURE_NUM][4];
u16 ae_targets[MAX_HDR_EXPOSURE_NUM];

img_config_info_t img_config_cus;
hdr_sensor_cfg_t sensor_cfg;

static pthread_t id;
pthread_cond_t img_cond;

/* HFV2 BPF: 0.1 - 0.3 */
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
/* HFV2 BPF: 0.2 - 0.3 */
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
/* HFV2 BPF: 0.1 - 0.2 */
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
/* HFV2 BPF: 0.05 - 0.15 with 32 threshold */
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

extern void init_hdr_proc(hdr_proc_data_t * p_hdr_proc_pipe,pipeline_control_t * p_adj_pipe);
extern int yuv_contrast_control(hdr_proc_data_t *p_hdr_proc_pipe, img_aaa_stat_t *p_hdr_stat);

void  cus_awb(u16 tile_count, awb_data_t * p_tile_info, awb_gain_t * p_awb_gain, u8 frame_id)
{
	int	idx = 0;
	u64 r_sum, g_sum, b_sum;
	u32 cnt_rgb =0;
	wb_gain_t cur_wb_gain, next_wb_gain;
	u8 cus_awb_speed = 16;
	static u8 cus_awb_skip_frames = 0;

	if(cus_awb_skip_frames > 0){
		--cus_awb_skip_frames;
		return;
	}

	cur_wb_gain = next_wb_gain = p_awb_gain->video_wb_gain;
	r_sum = g_sum = b_sum = 0;
	for (idx = 0; idx < tile_count; idx++){
		if(p_tile_info[idx].lin_y > 8 && p_tile_info[idx].lin_y <56){		// lin_y is in 0~63 range
			r_sum += p_tile_info[idx].r_avg;
			g_sum += p_tile_info[idx].g_avg;
			b_sum += p_tile_info[idx].b_avg;
			cnt_rgb ++;
		}
	}

	if (cnt_rgb > 0 && r_sum > cnt_rgb && b_sum > cnt_rgb) {
		next_wb_gain.r_gain = g_sum * AWB_UNIT_GAIN/ r_sum;
		next_wb_gain.g_gain= AWB_UNIT_GAIN;
		next_wb_gain.b_gain= g_sum * AWB_UNIT_GAIN/ b_sum;
	}else{
		next_wb_gain.r_gain = AWB_UNIT_GAIN;
		next_wb_gain.g_gain = AWB_UNIT_GAIN;
		next_wb_gain.b_gain = AWB_UNIT_GAIN;
	}

	if(next_wb_gain.r_gain != cur_wb_gain.r_gain ||next_wb_gain.b_gain != cur_wb_gain.b_gain){
		if(ABS(next_wb_gain.r_gain - cur_wb_gain.r_gain) > cus_awb_speed){
			next_wb_gain.r_gain = (next_wb_gain.r_gain + cur_wb_gain.r_gain * (cus_awb_speed - 1))/cus_awb_speed;
		}else{
			if(next_wb_gain.r_gain > cur_wb_gain.r_gain){
				next_wb_gain.r_gain = cur_wb_gain.r_gain + 1;
			}else{
				next_wb_gain.r_gain = cur_wb_gain.r_gain - 1;
			}
		}

		if(ABS(next_wb_gain.b_gain - cur_wb_gain.b_gain) > cus_awb_speed){
			next_wb_gain.b_gain = (next_wb_gain.b_gain + cur_wb_gain.b_gain * (cus_awb_speed - 1))/cus_awb_speed;
		}else{
			if(next_wb_gain.b_gain > cur_wb_gain.b_gain){
				next_wb_gain.b_gain = cur_wb_gain.b_gain + 1;
			}else{
				next_wb_gain.b_gain = cur_wb_gain.b_gain - 1;
			}
		}
		memcpy(&p_awb_gain->video_wb_gain, &next_wb_gain, sizeof(wb_gain_t));
		p_awb_gain->video_gain_update = 1;
		cus_awb_skip_frames = 2;
	}
}

void cus_ae(u8 frame_id)
{
	return;
}

int cus_set_sht_row_group(int fd_iav, int shutter_row_gp[4])
{
	struct amba_vin_wdr_shutter_gp_info shutter_gp;
	memcpy(&shutter_gp, shutter_row_gp, sizeof(struct amba_vin_wdr_shutter_gp_info));

	if(ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_WDR_SHUTTER_ROW_GP, &shutter_gp) < 0){
		perror("IAV_IOC_VIN_SRC_SET_WDR_SHUTTER_ROW_GP error");
		return -1;
	}

	return 0;
}

int cus_set_agc_index(int fd_iav, int agc_index)
{
	if(ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_AGC_INDEX, agc_index)<0) {
		perror("IAV_IOC_VIN_SRC_SET_AGC_INDEX");
		return -1;
	}
	return 0;
}

void cus_set_sensor_raw_offset(int fd_iav, hdr_proc_data_t *p_hdr_proc)
{
	// different HDR sensor has different raw offsets setting, refer to sensor data sheet for more information
	// some sensors use y offset but no x offset; some sensors use both x offset and y offset
	p_hdr_proc->raw_offset.x_offset[0] = 0;
	p_hdr_proc->raw_offset.x_offset[1] = 0;
	p_hdr_proc->raw_offset.x_offset[2] = 0;
	p_hdr_proc->raw_offset.x_offset[3] = 0;

	p_hdr_proc->raw_offset.y_offset[0] = 0;
	p_hdr_proc->raw_offset.y_offset[1] = 0;
	p_hdr_proc->raw_offset.y_offset[2] = 0;
	p_hdr_proc->raw_offset.y_offset[3] = 0;

	p_hdr_proc->raw_offset_update = 1;
}

void cus_set_hdr_batch(int fd_iav, hdr_proc_data_t *p_hdr_proc)
{
	int idx = 0;
	u8 update_flag = 0;

	for(idx = 0; idx < img_config_cus.expo_num; idx ++){
		if((p_hdr_proc->blc_update[idx] == 1) ||
			(p_hdr_proc->anti_aliasing_update[idx] == 1) ||
			(p_hdr_proc->dbp_update[idx] == 1) ||
			(p_hdr_proc->cfa_update[idx] == 1) ||
			(p_hdr_proc->le_update[idx] == 1) ||
			(p_hdr_proc->alpha_map_update[idx] == 1)||
			(p_hdr_proc->cc_update[idx] == 1)||
			(p_hdr_proc->wb_update[idx] == 1)||
			(p_hdr_proc->dgain_update[idx] == 1) ||
			(p_hdr_proc->yuv_contrast_update == 1)){

			update_flag = 1;
		}
	}
	if(update_flag == 1){
		for(idx = 0; idx < img_config_cus.expo_num; idx ++){
			p_hdr_proc->expo_id = idx;
			img_dsp_set_hdr_batch(fd_iav, p_hdr_proc);

			p_hdr_proc->blc_update[idx] = 0;
			p_hdr_proc->anti_aliasing_update[idx] = 0;
			p_hdr_proc->dbp_update[idx] = 0;
			p_hdr_proc->cfa_update[idx] = 0;
			p_hdr_proc->le_update[idx] = 0;
			p_hdr_proc->alpha_map_update[idx] = 0;
			p_hdr_proc->cc_update[idx] = 0;
			p_hdr_proc->wb_update[idx] = 0;
			p_hdr_proc->dgain_update[idx] = 0;
			p_hdr_proc->yuv_contrast_update = 0;
		}
		img_dsp_set_yuv_contrast(fd_iav, &p_hdr_proc->yuv_contrast_info);
		img_dsp_set_hdr_video_proc(fd_iav, &p_hdr_proc->video_proc);
	}

	if(p_hdr_proc->raw_offset_update == 1){
		img_dsp_set_hdr_row_offset(fd_iav, &p_hdr_proc->raw_offset);
		p_hdr_proc->raw_offset_update = 0;
	}
}

void cus_main_loop(void* arg)
{
	aaa_tile_report_t act_tile;
	u32 table_index = 0;
	static img_aaa_stat_t hdr_line_stat[MAX_HDR_EXPOSURE_NUM];
	int fd_iav = (int)arg;
	u8 frame_id = 0;
	int idx = 0;

	adj_video_init(&is_adj_param, &adj_pipe_ctl);
	init_hdr_proc(&hdr_data_proc, &adj_pipe_ctl);

	while(1){
		if(img_dsp_get_statistics_hdr(fd_iav, hdr_line_stat, &act_tile) < 0){
			continue;
		}

		for(idx = 0; idx < img_config_cus.expo_num; ++idx){
			hdr_data_proc.expo_id = hdr_line_stat[idx].tile_info.exposure_index;
			frame_id = hdr_data_proc.expo_id;

			// AE API, and update shutter setting
			cus_ae(frame_id);
			if(frame_id == 0){
				hdr_data_proc.shutter_row[frame_id] = 640;
			}else{
				hdr_data_proc.shutter_row[frame_id] = 128;
			}
			hdr_data_proc.shutter_update[frame_id] = 1;
			hdr_data_proc.agc_idx[frame_id] = 0;	// output of cus_ae();
			hdr_data_proc.agc_update[frame_id] = 1;

			// AWB API
			cus_awb(act_tile.awb_tile, hdr_line_stat[idx].awb_info, &awb_gain, frame_id);

			// AF API
			// af_api();

			//ADJ API
			if(frame_id == 0){
				adj_video_aeawb_control(table_index, &(is_adj_param.awbae), &adj_aeawb_ctl);
				adj_video_black_level_control(table_index, awb_gain.video_wb_gain, &is_adj_param, &adj_pipe_ctl);
				adj_video_ev_img(table_index, awb_gain.video_wb_gain, &is_adj_param, &adj_pipe_ctl, &is_chroma_scale);
				adj_video_noise_control(table_index, awb_gain.video_wb_gain, 0, &is_adj_param, &adj_pipe_ctl);
				adj_video_color_contrast(&adj_pipe_ctl, &hdr_line_stat[frame_id].rgb_hist);
			}

			// HDR Alpha Blending control API, can't miss this
			// alpha_blending_api();

			// parameters update
			if(adj_pipe_ctl.black_corr_update) {
				for(idx = 0; idx < img_config_cus.expo_num; ++idx){
					memcpy(&hdr_data_proc.blc_info[idx], &adj_pipe_ctl.black_corr, sizeof(blc_level_t));
					hdr_data_proc.blc_update[idx] = 1;
				}
				adj_pipe_ctl.black_corr_update = 0;
			}
			if(adj_pipe_ctl.badpix_corr_update) {
				for(idx = 0; idx < img_config_cus.expo_num; ++idx){
					memcpy(&hdr_data_proc.dbp_info[idx], &adj_pipe_ctl.badpix_corr_strength, sizeof(dbp_correction_t));
					hdr_data_proc.dbp_update[idx] = 1;
				}
				adj_pipe_ctl.badpix_corr_update = 0;
			}
			if(adj_pipe_ctl.anti_aliasing_update) {
				for(idx = 0; idx < img_config_cus.expo_num; ++idx){
					hdr_data_proc.anti_aliasing_strength[idx] = adj_pipe_ctl.anti_aliasing;
					hdr_data_proc.anti_aliasing_update[idx] = 1;
				}
				adj_pipe_ctl.anti_aliasing_update = 0;
			}
			if(adj_pipe_ctl.cfa_filter_update) {
				for(idx = 0; idx < img_config_cus.expo_num; ++idx){
					memcpy(&hdr_data_proc.cfa_info[idx], &adj_pipe_ctl.cfa_filter, sizeof(cfa_noise_filter_info_t));
					hdr_data_proc.cfa_update[idx] = 1;
				}
				adj_pipe_ctl.cfa_filter_update = 0;
			}
			if(adj_pipe_ctl.cc_matrix_update) {
				for(idx = 0; idx < img_config_cus.expo_num; ++idx){
					memcpy((void*)hdr_data_proc.cc_info[idx].matrix_3d_table_addr,
						(void*)adj_pipe_ctl.color_corr.matrix_3d_table_addr, CC_3D_SIZE);
					hdr_data_proc.cc_update[idx] = 1;
				}
				adj_pipe_ctl.cc_matrix_update = 0;
			}
			if(adj_pipe_ctl.gamma_update == 1){
				for(idx = 0; idx < img_config_cus.expo_num; ++idx){
					memcpy(&hdr_data_proc.tone_info[idx], &adj_pipe_ctl.gamma_table, sizeof(tone_curve_t));
					hdr_data_proc.cc_update[idx] = 1;
				}
				adj_pipe_ctl.gamma_update = 0;
			}
			if(awb_gain.video_gain_update){
				memcpy(&hdr_data_proc.wb_gain[frame_id], &awb_gain.video_wb_gain, sizeof(wb_gain_t));
				hdr_data_proc.wb_update[frame_id] = 1;
				awb_gain.video_gain_update = 0;
			}
		}

		if(adj_pipe_ctl.chroma_median_filter_update){
			img_dsp_set_chroma_median_filter(fd_iav, &adj_pipe_ctl.chroma_median_filter);
			adj_pipe_ctl.chroma_median_filter_update = 0;
		}
		if(adj_pipe_ctl.chroma_noise_filter_update){
			img_dsp_set_chroma_noise_filter(fd_iav, IMG_VIDEO, &adj_pipe_ctl.chroma_noise_filter);
			adj_pipe_ctl.chroma_noise_filter_update = 0;
		}
		if(adj_pipe_ctl.chroma_scale_update) {
			img_dsp_set_chroma_scale(fd_iav, &adj_pipe_ctl.chroma_scale);
			adj_pipe_ctl.chroma_scale_update = 0;
		}
		if(adj_pipe_ctl.mctf_update) {
			img_dsp_set_video_mctf(fd_iav, &adj_pipe_ctl.mctf_param);
			adj_pipe_ctl.mctf_update = 0;
		}
		if(adj_pipe_ctl.asf_select_update){
			img_dsp_set_video_sharpen_a_or_spatial_filter(fd_iav, adj_pipe_ctl.asf_select);
			adj_pipe_ctl.asf_select_update = 0;
		}
		if(adj_pipe_ctl.cdnr_filter_update) {
			img_dsp_set_color_dependent_noise_reduction(fd_iav, 0, &adj_pipe_ctl.cdnr_filter);
			adj_pipe_ctl.cdnr_filter_update = 0;
		}
		if(adj_pipe_ctl.spatial_filter_update) {
			img_dsp_set_advance_spatial_filter(fd_iav, IMG_VIDEO, &adj_pipe_ctl.spatial_filter);
			adj_pipe_ctl.spatial_filter_update = 0;
		}
		if(adj_pipe_ctl.high_freq_noise_reduc_update) {
			img_dsp_set_luma_high_freq_noise_reduction(fd_iav, &adj_pipe_ctl.high_freq_noise_strength);
			adj_pipe_ctl.high_freq_noise_reduc_update = 0;
		}
		if(adj_pipe_ctl.sharp_a_level_minimum_update) {
			img_dsp_set_sharpen_a_level_minimum(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_a_level_minimum);
			adj_pipe_ctl.sharp_a_level_minimum_update = 0;
		}
		if(adj_pipe_ctl.sharp_a_level_overall_update) {
			img_dsp_set_sharpen_a_level_overall(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_a_level_overall);
			adj_pipe_ctl.sharp_a_level_overall_update = 0;
		}
		if(adj_pipe_ctl.sharp_a_retain_update) {
			img_dsp_set_sharpen_a_signal_retain_level(fd_iav, IMG_VIDEO, adj_pipe_ctl.sharp_a_retain);
			adj_pipe_ctl.sharp_a_retain_update = 0;
		}
		if(adj_pipe_ctl.sharp_a_max_change_update) {
			img_dsp_set_sharpen_a_max_change(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_a_max_change);
			adj_pipe_ctl.sharp_a_max_change_update = 0;
		}
		if(adj_pipe_ctl.sharp_a_fir_update) {
			img_dsp_set_sharpen_a_fir(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_a_fir);
			adj_pipe_ctl.sharp_a_fir_update = 0;
		}
		if(adj_pipe_ctl.sharp_a_coring_update) {
			img_dsp_set_sharpen_a_coring(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_a_coring);
			adj_pipe_ctl.sharp_a_coring_update = 0;
		}
		if(adj_pipe_ctl.sharp_b_level_minimum_update) {
			img_dsp_set_sharpen_b_level_minimum(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_b_level_minimum);
			adj_pipe_ctl.sharp_b_level_minimum_update = 0;
		}
		if(adj_pipe_ctl.sharp_b_level_overall_update) {
			img_dsp_set_sharpen_b_level_overall(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_b_level_overall);
			adj_pipe_ctl.sharp_b_level_overall_update = 0;
		}
		if(adj_pipe_ctl.sharp_b_retain_update){
			img_dsp_set_sharpen_b_signal_retain_level(fd_iav, IMG_VIDEO, adj_pipe_ctl.sharp_b_retain);
			adj_pipe_ctl.sharp_b_retain_update = 0;
		}
		if(adj_pipe_ctl.sharp_b_max_change_update) {
			img_dsp_set_sharpen_b_max_change(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_b_max_change);
			adj_pipe_ctl.sharp_b_max_change_update = 0;
		}
		if(adj_pipe_ctl.sharp_b_fir_update) {
			img_dsp_set_sharpen_b_fir(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_b_fir);
			adj_pipe_ctl.sharp_b_fir_update = 0;
		}
		if(adj_pipe_ctl.sharp_b_coring_update) {
			img_dsp_set_sharpen_b_coring(fd_iav, IMG_VIDEO, &adj_pipe_ctl.sharp_b_coring);
			adj_pipe_ctl.sharp_b_coring_update = 0;
		}
		if(adj_pipe_ctl.sharp_b_linearization_update){
			img_dsp_set_sharpen_b_linearization(fd_iav, IMG_VIDEO, adj_pipe_ctl.sharp_b_linearization_strength);
			adj_pipe_ctl.sharp_b_linearization_update = 0;
		}
		if(adj_pipe_ctl.rgb_yuv_matrix_update) {
			rgb_to_yuv_t r2y_matrix = is_rgb2yuv[rgb2yuv_style];
			adj_set_color_conversion(&r2y_matrix);
			img_dsp_set_rgb2yuv_matrix(fd_iav, &r2y_matrix);
			adj_pipe_ctl.rgb_yuv_matrix_update = 0;
		}

		// contrast setting
		yuv_contrast_control(&hdr_data_proc, hdr_line_stat);

		// set shutter for all frames
		cus_set_sht_row_group(fd_iav, hdr_data_proc.shutter_row);

		// set gain agc level
		cus_set_agc_index(fd_iav, hdr_data_proc.agc_idx[0]);	// set agc for frame 0 only, other short frames will take effect too

		// set set offset related to sensor shutter setting
		cus_set_sensor_raw_offset(fd_iav, &hdr_data_proc);

		// set HDR commands queue
		cus_set_hdr_batch(fd_iav, &hdr_data_proc);
	}
}

int load_dsp_cc_table(int fd_iav)
{
	int rval = 0;
	color_correction_reg_t color_corr_reg;
	color_correction_t color_corr;

	u8 *reg = NULL, *matrix = NULL, *sec_cc = NULL;
	char filename[128];
	int file, count;

	if(reg == NULL){
		reg = (u8*)malloc(CC_REG_SIZE);
		if(reg == NULL){
			rval = -1;
			goto load_dsp_cc_exit;
		}
	}
	if(matrix == NULL){
		matrix = (u8*)malloc(CC_3D_SIZE);
		if(matrix == NULL){
			rval = -1;
			goto load_dsp_cc_exit;
		}
	}
	if(sec_cc == NULL){
		sec_cc = (u8*)malloc(SEC_CC_SIZE);
		if(sec_cc == NULL){
			rval = -1;
			goto load_dsp_cc_exit;
		}
	}
	sprintf(filename, "%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("reg.bin cannot be opened\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if((count = read(file, reg, CC_REG_SIZE)) != CC_REG_SIZE) {
		printf("read reg.bin error\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if(file >= 0){
		close(file);
	}

	sprintf(filename, "%s/3D.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("3D.bin cannot be opened\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
		printf("read 3D.bin error\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if(file >= 0){
		close(file);
	}

	sprintf(filename, "%s/3D_sec.bin", IMGPROC_PARAM_PATH);
	if((file = open(filename, O_RDONLY, 0))<0) {
		printf("3D_sec.bin cannot be opened\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if((count = read(file, sec_cc, SEC_CC_SIZE)) != SEC_CC_SIZE) {
		printf("read 3D_sec error\n");
		rval = -1;
		goto load_dsp_cc_exit;
	}
	if(file >= 0){
		close(file);
	}

	color_corr_reg.reg_setting_addr = (u32)reg;
	color_corr.matrix_3d_table_addr = (u32)matrix;
	color_corr.sec_cc_addr = (u32)sec_cc;
	img_dsp_set_color_correction_reg(&color_corr_reg);
	img_dsp_set_color_correction(fd_iav, &color_corr);

load_dsp_cc_exit:
	if(reg != NULL){
		free(reg);
		reg = NULL;
	}
	if(matrix != NULL){
		free(matrix);
		matrix = NULL;
	}
	if(sec_cc != NULL){
		free(sec_cc);
		sec_cc = NULL;
	}
	return rval;
}

int load_adj_cc_table(char * sensor_name)
{
	int file = -1;
	int count = 0;
	char filename[128];
	u8 *matrix = NULL;
	u8 i, adj_mode = 4;
	int rval = 0;

	if(matrix == NULL){
		matrix = malloc(CC_3D_SIZE);
		if(matrix == NULL){
			rval = -1;
			printf("error: malloc no memory\n");
			goto load_adj_cc_exit;
		}
	}

	for (i = 0; i < adj_mode; i++) {
		sprintf(filename,"%s/sensors/%s_0%d_3D.bin", IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			printf("3D.bin cannot be opened\n");
			rval = -1;
			goto load_adj_cc_exit;
		}
		if((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
			printf("read %s error\n",filename);
			rval = -1;
			goto load_adj_cc_exit;
		}
		img_adj_load_cc_table((u32)matrix, i);
	}

load_adj_cc_exit:
	if(file >= 0){
		close(file);
		file = -1;
	}
	if(matrix != NULL){
		free(matrix);
		matrix = NULL;
	}
	return rval;
}

int main(int argc, char **argv)
{
	int fd_iav;
	sem_t sem;
	af_statistics_ex_t af_statistic_setup_ex;
	image_sensor_param_t sensor_app_param;
	hdr_sensor_param_t hdr_app_param;
	char sensor_name[32] = "img";

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	if(img_config_working_status(fd_iav, &img_config_cus) < 0){
		printf("error: img_config_working_status\n");
		return -1;
	}
	if(img_lib_init(img_config_cus.defblc_enable, img_config_cus.sharpen_b_enable) < 0){
		printf("error: img_lib_init\n");
		return -1;
	}

	memcpy(&sensor_cfg, &mn34220pl_sensor_config, sizeof(hdr_sensor_cfg_t));
	sensor_app_param.p_adj_param = &mn34220pl_adj_param;
	sensor_app_param.p_rgb2yuv = mn34220pl_rgb2yuv;
	sensor_app_param.p_chroma_scale = &mn34220pl_chroma_scale;
	sensor_app_param.p_awb_param = &mn34220pl_awb_param;
	sensor_app_param.p_50hz_lines = NULL;
	sensor_app_param.p_60hz_lines = NULL;
	sensor_app_param.p_tile_config = &mn34220pl_tile_config;
	sensor_app_param.p_ae_agc_dgain = mn34220pl_ae_agc_dgain;
	sensor_app_param.p_ae_sht_dgain = mn34220pl_ae_sht_dgain;
	sensor_app_param.p_dlight_range = mn34220pl_dlight;
	sensor_app_param.p_manual_LE = mn34220pl_manual_LE;
	if(img_config_cus.expo_num == 3){
		hdr_app_param.p_ae_target = mn34220pl_multifrm_ae_target_3x;
		hdr_app_param.p_sht_lines = (line_t**)&mn34220pl_multifrm_sht_lines_3x[0];
	}else if(img_config_cus.expo_num == 2){
		hdr_app_param.p_ae_target = mn34220pl_multifrm_ae_target_2x;
		hdr_app_param.p_sht_lines = (line_t**)&mn34220pl_multifrm_sht_lines_2x[0];
	}else{
		hdr_app_param.p_ae_target = NULL;
		hdr_app_param.p_sht_lines = NULL;
	}
	sprintf(sensor_name, "mn34220pl");


	load_dsp_cc_table(fd_iav);
	load_adj_cc_table(sensor_name);

	// load sensor parameters
	memcpy(&is_awb_param, sensor_app_param.p_awb_param, sizeof(is_awb_param));
	memcpy(&is_adj_param, sensor_app_param.p_adj_param, sizeof(is_adj_param));
	memcpy(is_rgb2yuv, sensor_app_param.p_rgb2yuv, sizeof(is_rgb2yuv));
	memcpy(&is_chroma_scale, sensor_app_param.p_chroma_scale, sizeof(is_chroma_scale));
	memcpy(&is_tile_config, sensor_app_param.p_tile_config, sizeof(is_tile_config));
	memcpy(ae_targets, hdr_app_param.p_ae_target, sizeof(ae_targets));
	memcpy(ae_lines, hdr_app_param.p_sht_lines, sizeof(ae_lines));

	//re-init idsp config start, use your own img_dsp_xxx and dsp driver instead of below ones.
	int i = 0;
	digital_sat_level_t dgain_sat = {16383, 16383, 16383, 16383};
	histogram_config_t hist_config;
	sensor_info_t sensor_info;
	sensor_info.sensor_pattern = pat;

	hist_config.mode = 2;
	hist_config.hist_select = 0;
	for (i = 0; i < MAX_AE_TILE_ROW; ++i) {
		hist_config.tile_mask[i] = 0xFFF;
	}

	img_dsp_set_dgain_saturation_level(fd_iav, &dgain_sat);
	img_dsp_config_statistics_info(fd_iav, &is_tile_config);
	img_dsp_set_config_histogram_info(fd_iav, &hist_config);
	img_dsp_set_rgb2yuv_matrix(fd_iav, &is_rgb2yuv[rgb2yuv_style]);	//PC RGB2YUV matrix by default
	img_dsp_enable_color_correction(fd_iav);
	af_statistic_setup_ex = af_eng_cof1;
	img_dsp_set_af_statistics_ex(fd_iav, &af_statistic_setup_ex,1);
	img_dsp_sensor_config(fd_iav, &sensor_info);
	img_dsp_set_video_sharpen_a_or_spatial_filter(fd_iav, 1);
	img_idsp_filters_init(fd_iav);
	//re-init idsp config end

	pthread_create(&id, NULL, (void*)cus_main_loop, (void*)fd_iav);
	sem_init(&sem, 0, 0);
	sem_wait(&sem);

	return 0;
}


