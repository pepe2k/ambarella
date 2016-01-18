#ifndef	IMG_API_H
#define	IMG_API_H

#ifdef __cplusplus
extern "C" {
#endif

//#include "config.h"
#include "basetypes.h"

// basic 3A related APIs
int img_lib_init(u32 enable_defblc, u32 enable_sharpen_b);
int img_lib_deinit(void);
void img_algo_get_lib_version(img_lib_version_t* p_img_algo_info );
int img_start_aaa(int fd_iav); //OK
int img_stop_aaa(void);

int img_set_work_mode(int mode); //0 -preview

void img_get_3a_cntl_status(aaa_cntl_t *cntl);
int img_enable_adj(int enable);
int img_enable_ae(int enable);
int img_enable_af(int enable);
int img_enable_awb(int enable);

// lens and sensor related APIs
int img_lens_init(void);
int img_config_lens_info(lens_ID lensID);
int img_load_lens_param(lens_param_t *p_lens_param);
int img_config_lens_cali(lens_cali_t *p_lens_cali);
int img_config_sensor_info( sensor_config_t* config);
int img_config_sensor_hdr_mode(int hdr_mode);

int img_load_image_sensor_param(image_sensor_param_t* p_app_param_image_sensor);
int img_dynamic_load_image_sensor_param(image_sensor_param_t* p_app_param_image_sensor);
int img_set_sensor_shutter_index(int fd_iav, u32 shutter_index);
int img_get_sensor_shutter_index();
int img_set_sensor_agc_index(int fd_iav, u32 agc_index, u32 step);
int img_get_sensor_agc_index();
int img_get_sensor_shutter_width(int fd_iav,int shutter_index, int* curr_width);

// adj related APIs
int img_config_working_status(int fd_iav,img_config_info_t * p_img_config);
int img_adj_load_cc_table(u32 matrix_addr, u32 index);

// ae related APIs
void img_register_iris_cntl(dc_iris_cntl p_cntl);
u32 img_ae_get_system_gain(void);
int img_ae_set_antiflicker(int mode);		//0-50Hz, 1-60Hz
int img_ae_set_backlight(int backlight_enable); //0-disable, 1-enable
int img_ae_set_meter_mode(ae_metering_mode mode);
int img_ae_load_exp_line(line_t* line, u16 line_num, u16 line_belt);	//lin_num <=10, line_belt <= lin_num, line_belt count from 1
int img_ae_set_belt(u16 line_belt);					//line_belt <= line_num, line_belt count from 1
s16 img_ae_get_cursor(void);			//-1: <= start point, 0: in control, 1: >= end point
u16 img_ae_get_target(void);
int img_ae_set_target_ratio(u16 ratio);		// unit = 1024
u16 img_ae_get_target_ratio(void);
int img_set_ae_roi(int * roi_table);
int img_get_ae_luma_value(void);
int img_get_ae_rgb_luma_value(void);
int img_ae_enable_subflicker(int enable, int diff);	// diff range is from 0 to 50 in luma domain
int img_set_ae_speed(u8 mode,u8 ae_speed_level); //default mode=0, only when mode =1,then speed_level =0~6, 0 is fastest
int img_set_hlc_enable(int enable);//0: disable or 1: enable
int img_ae_check_limited();
u8 img_ae_check_stable();
int img_ae_set_target_adj(u8 fixed_adj_enable,u8 adj);//adj 0-16, default: fixed_adj_enable =0,adj =auto.

// af related APIs
int img_af_set_range(af_range_t * p_af_range);
int img_af_set_mode(af_mode mode);
int img_af_set_roi(af_ROI_config_t* af_ROI);
int img_af_set_zoom_idx(u16 zoom_idx);
int img_af_set_focus_idx(s32 focus_idx);
int img_af_set_reset(void);
void img_af_load_param(void* p_af_param, void* p_zoom_map);
int img_wait_af_stable_signal(int fd);
int img_af_set_touch_point(int fd,int x,int y);

// awb related APIs
int img_awb_set_environment(awb_environment_t env);	// work only if AWB_METHOD = AWB_NORMAL, WB is automatic, fits different environment
int img_awb_set_mode(awb_control_mode_t mode);	// work only if AWB_METHOD =AWB_NORMAL; include automatic mode and single const wb gain mode
awb_control_mode_t img_awb_get_mode(void);
int img_awb_set_method(awb_work_method_t method);	// AWB_NORMAL(automatic), GREY_WORLD(automatic) and MANUAL(manually define)
awb_work_method_t img_awb_get_method(void);
int img_awb_set_wb_shift(wb_gain_t* p_wb_org_gain, wb_gain_t* p_wb_ref_gain);
void img_awb_get_wb_cal(wb_gain_t* p_wb_gain);
int img_awb_set_speed(u8 speed);
int img_awb_set_cali_diff_thr(u16 r_thr, u16 b_thr);
int img_awb_set_custom_gain(wb_gain_t* cus_gain);
int img_awb_set_failure_remedy(awb_failure_remedy_t remedy_mode);

// image property and color style related APIs
int img_set_color_hue(int hue);			//mode 0:-15 - +15: -30deg - +30 deg, mode 1:[-36, 36], -180 deg. <--> +180 deg
int img_set_color_hue_mode(int hue_mode); //0 normal, 1 enhance
int img_set_color_brightness(int brightness);	//-255 - +255
int img_set_color_contrast(int contrast);	//unit = 64, 0 ~ 128
int img_set_chroma_noise_filter_strength(int level);	//unit = 64, 0~256
int img_set_chroma_noise_filter_max_radius(int max_radius);//32,64,128
int img_set_auto_color_contrast(int enable); // 0 = disable, 1 = auto;
int img_set_auto_color_contrast_strength(int strength); // 0~128, 0: no effect; 128: full effect
int img_set_yuv_contrast_enable(int enable); // 0 = disable, 1 = enable
int img_set_yuv_contrast_strength(int strength);	// 0~128, default: 64
int img_set_wdr_enable(int enable);	// 0= disable, 1= enable
int img_set_wdr_strength(int strength);	// 0~128, default:64
int img_set_color_saturation(int saturation);	//unit = 64
int img_set_sharpness(int level);		//0 ~ 10; default = 6;
int img_set_mctf_strength(int level);		//0 ~ 10; default = 6;
int img_set_spacial_denoise(int level);	//0 ~ 10; default = 0;
int img_set_auto_local_exposure(int mode); //0 = disable, 1 = auto, 64 = 1X, 128 = 2X, 192 = 3X, 256 = 4X
void img_get_img_property(image_property_t *prop);
int img_set_color_style(img_color_style mode);
img_color_style img_get_color_style(void);
int img_set_bw_mode(u8 en); // 0: diable, 1: enable 2: auto
int img_set_extra_blc(int blc_level);
int img_get_extra_blc(void);
int img_set_mctf_property_ratio(mctf_property_t* p_mctf_property);//unit {64,64,64,64};
int img_set_sharpen_property_ratio(sharpen_property_t* p_shp_property);//unit {64,64,16,64,64,64,64,64};

// calibration related APIs
int img_cal_vignette(vignette_cal_t *vig_detect_setup);
int img_cali_bad_pixel(int fd_iav, cali_badpix_setup_t *pCali_badpix_setup);
int img_cal_gyro(cali_gyro_setup_t* in, gyro_calib_info_t* out);
int img_cal_warp(const warp_cal_info_t* p_cali_setup, warp_correction_t* p_warp_correct);
int img_warp_crop_region_table(s16 *p_normal_hori_table, s16* p_normal_ver_table, u16 full_width, u16 full_height);
int img_calc_warp_table(warp_correction_t *mw_cmd, warp_calib_map_t *ctrl, u16 main_width, u16 main_height);

// customerize 3A related APIs
int img_register_aaa_algorithm(aaa_api_t custom_aaa_api);
int img_register_lens_drv(lens_dev_drv_t custom_lens_drv);

#ifdef __cplusplus
}
#endif

#endif	// IMG_API_H

