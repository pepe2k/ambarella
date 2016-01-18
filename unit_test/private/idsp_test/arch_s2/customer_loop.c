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
#include "img_customer_interface_arch.h"

#include "imx172_adj_param.c"
#include "imx172_aeb_param.c"

#include "mn34041pl_adj_param.c"
#include "mn34041pl_aeb_param.c"

#define	IMGPROC_PARAM_PATH	"/etc/idsp"
#ifndef AWB_UNIT_GAIN
#define AWB_UNIT_GAIN	1024
#endif
#ifndef ABS
#define ABS(a)	   (((a) < 0) ? -(a) : (a))
#endif
static img_color_style rgb2yuv_style = IMG_COLOR_PC;

static img_aaa_stat_t aaa_stat_info;
static awb_gain_t awb_gain;
static pipeline_control_t adj_pipe_ctl;
static adj_aeawb_control_t adj_aeawb_ctl;
static adj_param_t is_adj_param;
static rgb_to_yuv_t is_rgb2yuv[4];
static chroma_scale_filter_t is_chroma_scale;
static statistics_config_t is_tile_config;
static img_awb_param_t is_awb_param;
static image_sensor_param_t app_param_image_sensor;

bayer_pattern pat;
u32 max_sensor_gain, gain_step, timing_mode;

img_config_info_t img_config_cus;
hdr_proc_data_t defog_pipe_ctl;

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

void  cus_awb(u16 tile_count, awb_data_t * p_tile_info, awb_gain_t * p_awb_gain)
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

void cus_main_loop(void* arg)
{
	int fd_iav;
	fd_iav = (int)arg;
	aaa_tile_report_t act_tile;
	u32 table_index = 0;

	adj_video_init(&is_adj_param, &adj_pipe_ctl);

	while(1){
		if(img_dsp_get_statistics(fd_iav, &aaa_stat_info, &act_tile) < 0)	continue;

		// register your own AWB/AE/AF by img_register_aaa_algorithm()
		// AE API
		table_index = 128;		//table_index = gain_db/6*128

		// AWB API
		cus_awb(act_tile.awb_tile, aaa_stat_info.awb_info, &awb_gain);

		// AF API

		adj_video_aeawb_control(table_index, &(is_adj_param.awbae), &adj_aeawb_ctl);
		adj_video_black_level_control(table_index, awb_gain.video_wb_gain, &is_adj_param, &adj_pipe_ctl);
		adj_video_ev_img(table_index, awb_gain.video_wb_gain, &is_adj_param, &adj_pipe_ctl, &is_chroma_scale);
		adj_video_noise_control(table_index, awb_gain.video_wb_gain, 0, &is_adj_param, &adj_pipe_ctl);
		adj_video_color_contrast(&adj_pipe_ctl, &aaa_stat_info.rgb_hist);

		if(adj_pipe_ctl.local_exposure_update) {
			img_dsp_set_local_exposure(fd_iav, &adj_pipe_ctl.local_exposure);
			adj_pipe_ctl.local_exposure_update = 0;
		}
		if(adj_pipe_ctl.black_corr_update) {
			img_dsp_set_global_blc(fd_iav, &adj_pipe_ctl.black_corr, pat);
			adj_pipe_ctl.black_corr_update = 0;
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
		if(adj_pipe_ctl.cc_matrix_update) {
			img_dsp_set_color_correction(fd_iav, &adj_pipe_ctl.color_corr);
			adj_pipe_ctl.cc_matrix_update = 0;
		}
		if(adj_pipe_ctl.badpix_corr_update) {
			img_dsp_set_dynamic_bad_pixel_correction(fd_iav, &adj_pipe_ctl.badpix_corr_strength);
			adj_pipe_ctl.badpix_corr_update = 0;
		}
		if(adj_pipe_ctl.cfa_filter_update) {
			img_dsp_set_cfa_noise_filter(fd_iav, &adj_pipe_ctl.cfa_filter);
			adj_pipe_ctl.cfa_filter_update = 0;
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
		if(adj_pipe_ctl.gamma_update) {
			img_dsp_set_tone_curve(fd_iav, &adj_pipe_ctl.gamma_table);
			adj_pipe_ctl.gamma_update = 0;
		}
		if(adj_pipe_ctl.anti_aliasing_update){
			img_dsp_set_anti_aliasing(fd_iav, adj_pipe_ctl.anti_aliasing);
			adj_pipe_ctl.anti_aliasing_update = 0;
		}
		if(awb_gain.video_gain_update==1){
			img_dsp_set_wb_gain(fd_iav,&awb_gain.video_wb_gain);
			awb_gain.video_gain_update = 0;
		}

	}
}

int load_adj_cc_table(char *sensor_name)
{
	int file, count;
	char filename[128];
	u8 matrix[CC_3D_SIZE], reg[CC_REG_SIZE];
	u8 i, adj_mode = 4;
	static color_correction_reg_t color_corr_reg;

	for (i = 0; i < adj_mode; i++) {
		sprintf(filename,"%s/sensors/%s_0%d_3D.bin",
			IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			printf("can not open 3D.bin\n");
			return -1;
		}
		if((count = read(file, matrix, CC_3D_SIZE)) != CC_3D_SIZE) {
			printf("read imx036_01_3D.bin error\n");
			return -1;
		}
		close(file);
		img_adj_load_cc_table((u32)matrix, i);
	}

	sprintf(filename, "%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		if ((file = open("reg.bin", O_RDONLY, 0)) < 0) {
			printf("reg.bin cannot be opened\n");
			return -1;
		}
	}
	if((count = read(file, reg, CC_REG_SIZE)) != CC_REG_SIZE) {
		printf("read reg.bin error\n");
		return -1;
	}
	close(file);
	color_corr_reg.reg_setting_addr=(u32)reg;
	img_dsp_set_color_correction_reg(&color_corr_reg);

	return 0;
}

int main(int argc, char **argv)
{
	int fd_iav;
	char sensor_name[32];
	struct amba_vin_source_info vin_info;
	sem_t sem;
	af_statistics_ex_t af_statistic_setup_ex;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
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

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}

	switch (vin_info.sensor_id) {
		case SENSOR_IMX172:
			pat = RG;
			app_param_image_sensor.p_adj_param = &imx172_adj_param;
			app_param_image_sensor.p_rgb2yuv = imx172_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &imx172_chroma_scale;
			app_param_image_sensor.p_awb_param = &imx172_awb_param;
			app_param_image_sensor.p_tile_config = &imx172_tile_config;
			sprintf(sensor_name, "imx172");
			break;
		case SENSOR_MN34041PL:
			pat = BG;
			app_param_image_sensor.p_adj_param = &mn34041pl_adj_param;
			app_param_image_sensor.p_rgb2yuv = mn34041pl_rgb2yuv;
			app_param_image_sensor.p_chroma_scale = &mn34041pl_chroma_scale;
			app_param_image_sensor.p_awb_param = &mn34041pl_awb_param;
			app_param_image_sensor.p_tile_config = &mn34041pl_tile_config;
			sprintf(sensor_name, "mn34041pl");
			break;
		default:
			return -1;
	}

	load_adj_cc_table(sensor_name);
	memcpy(&is_awb_param, app_param_image_sensor.p_awb_param, sizeof(is_awb_param));
	memcpy(&is_adj_param, app_param_image_sensor.p_adj_param, sizeof(is_adj_param));
	memcpy(is_rgb2yuv, app_param_image_sensor.p_rgb2yuv, sizeof(is_rgb2yuv));
	memcpy(&is_chroma_scale, app_param_image_sensor.p_chroma_scale, sizeof(is_chroma_scale));
	memcpy(&is_tile_config, app_param_image_sensor.p_tile_config, sizeof(is_tile_config));

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
	int max_radius = 128;
	iav_system_resource_setup_ex_t  resource_limit;
	memset(&resource_limit, 0, sizeof(resource_limit));
	resource_limit.encode_mode = IAV_ENCODE_CURRENT_MODE;
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_limit)<0){
		perror("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX");
		return -1;
	}
	max_radius = 1<<(resource_limit.max_chroma_noise_shift + 5);
	img_dsp_set_chroma_noise_filter_max_radius(max_radius);
	//re-init idsp config end

	pthread_create(&id, NULL, (void*)cus_main_loop, (void*)fd_iav);
	sem_init(&sem, 0, 0);
	sem_wait(&sem);

	return 0;
}

