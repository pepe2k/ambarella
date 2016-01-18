#ifndef __IMG_HDR_API_H__
#define __IMG_HDR_API_H__

#ifdef __cplusplus
extern "C" {
#endif

// start/stop 3A and HDR blending
int img_start_hdr(int fd_iav);
int img_stop_hdr(void);

// register hdr function cntl
int img_hdr_register_aaa_algorithm(hdr_cntl_fp_t * p_cntl_fp,const u8 frame_id);

// config sensor and coresponding sensor parameters
int img_hdr_config_sensor_info(hdr_sensor_cfg_t * p_sensor_config);
int img_hdr_load_sensor_param(image_sensor_param_t * p_image_param,hdr_sensor_param_t * p_hdr_param);
int img_hdr_dyn_load_sensor_param(image_sensor_param_t* p_image_param);

// get image statistics
// *** customer should use this one if they use AMBA main loop.
// *** otherwise img_dsp_get_statistics_hdr() should be used if customer generate their own main loop
int img_hdr_get_statis(img_aaa_stat_t * p_statis,const u8 num);

// get 3A auto-run status
int img_hdr_get_aaa_status(aaa_cntl_t *aaa_status);

// enable/disable 3A auto-run
void img_hdr_enable_awb(u8 enable);
void img_hdr_enable_ae(u8 enable);
void img_hdr_enable_af(u8 enable);
void img_hdr_enable_adj(u8 enable);

// AE related
int img_ae_hdr_set_speed(u32 * p_speeds, const u8 num);
int img_ae_hdr_get_speed(u32 * p_speeds, const u8 num);
int img_ae_hdr_set_shutter_ratio(u32 *p_ratios, const u8 num); // important. shutter ratio decide how HDR look, unit 128, default 64X shutter ratio is 8192
int img_ae_hdr_get_shutter_ratio(u32 *p_ratios, const u8 num);
void img_ae_hdr_set_target_ratio(u16 ratio, u8 frame_id);
u16 img_ae_hdr_get_target_ratio(u8 frame_id);
void img_ae_hdr_set_target(int target, u8 frame_id);
u16 img_ae_hdr_get_target(u8 frame_id);
int img_ae_hdr_get_luma_value(u8 frame_id);
int img_hdr_register_iris_cntl(dc_iris_cntl iris_cntl, u8 frame_id);
int img_ae_hdr_set_shutter_row(int * p_shutter_row,const u8 num);
int img_ae_hdr_get_shutter_row(int * p_shutter_row,const u8 num);
int img_ae_hdr_set_dgain(u32 * p_dgain,const u8 num);
int img_ae_hdr_get_dgain(u32 * p_dgain,const u8 num);
int img_ae_hdr_set_agc_index(int * p_agc_index,const u8 num);
int img_ae_hdr_get_agc_index(int * p_agc_index,const u8 num);
int img_ae_hdr_load_exp_line(line_t * p_line,u16 ln_num,u16 ln_belt,const u8 frame_id);
int img_ae_hdr_set_belts(u16 *p_line_belts, const u8 num);
int img_ae_hdr_get_cursors(s16 *p_cursors, const u8 num);
int img_ae_hdr_set_meter_modes(ae_metering_mode *p_modes, const u8 num);
int img_ae_hdr_set_roi(int * p_roi_tbl,const u8 frame_id);
int img_ae_hdr_set_orders(int *p_orders, const u8 num);

// AF
int img_hdr_register_lens_drv(lens_dev_drv_t custom_lens_drv);
int img_hdr_lens_init(void);
void img_af_hdr_load_param(void* p_af_param, void* p_zoom_map);
int img_hdr_config_lens_info(lens_ID lensID);
int img_af_hdr_set_range(af_range_t *p_af_range);
int img_af_hdr_set_mode(af_mode mode);
int img_af_hdr_set_roi(af_ROI_config_t* p_af_ROI);
int img_af_hdr_set_zoom_idx(u16 zoom_idx);
int img_af_hdr_set_focus_idx(s32 focus_idx);

// AWB
int img_hdr_awb_set_environment(awb_environment_t* p_env, const u8 num);
int img_hdr_awb_set_mode(awb_control_mode_t *p_mode, const u8 num);
int img_hdr_awb_get_mode(awb_control_mode_t *p_mode, const u8 num);
int img_hdr_awb_set_method(awb_work_method_t *p_method, const u8 num);
int img_hdr_awb_get_method(awb_work_method_t *p_method, const u8 num);
int img_hdr_awb_set_failure_remedy(awb_failure_remedy_t * p_remedy,const u8 num);
int img_hdr_awb_set_speed(u8 *p_speed, const u8 num);
int img_hdr_awb_set_cali_thre(u16 *p_r_th, u16 *p_b_th, const u8 num);
int img_hdr_awb_set_cali_shift(wb_gain_t* p_wb_org_gain, wb_gain_t* p_wb_ref_gain);
int img_hdr_awb_set_custom_gain(wb_gain_t* p_cus_gain, const u8 num);
int img_hdr_awb_get_manual_gain(wb_gain_t* p_wb_gain, const u8 num);
int img_hdr_set_wb_gain(wb_gain_t * p_wb_gain,const u8 num);
int img_hdr_get_wb_gain(wb_gain_t * p_wb_gain,const u8 num);

// adj filters related
// BLC
void img_adj_hdr_set_auto_blc(u8 enable);		// 0: manual mode; 1: auto mode
u8 img_adj_hdr_get_auto_blc();
int img_adj_hdr_set_blc(blc_level_t *p_blc, const u8 num);
int img_adj_hdr_get_blc(blc_level_t *p_blc, const u8 num);

// CFA
void img_adj_hdr_set_auto_cfa(u8 enable);		// 0: manual mode; 1: auto mode
u8 img_adj_hdr_get_auto_cfa();
int img_adj_hdr_set_cfa(cfa_noise_filter_info_t *p_cnf, const u8 num);
int img_adj_hdr_get_cfa(cfa_noise_filter_info_t *p_cnf, const u8 num);

// DBP
void img_adj_hdr_set_auto_dbp(u8 enable);
u8 img_adj_hdr_get_auto_dbp(void);
int img_adj_hdr_set_dbp(dbp_correction_t *p_dbp, const u8 num);
int img_adj_hdr_get_dbp(dbp_correction_t *p_dbp, const u8 num);

// ANTI-ALIASING
void img_adj_hdr_set_auto_anti_aliasing(u8 enable);
u8  img_adj_hdr_get_auto_anti_aliasing(void);
int  img_adj_hdr_set_anti_aliasing(u8 *p_anti_aliasing, const u8 num);
int  img_adj_hdr_get_anti_aliasing(u8 *p_anti_aliasing, const u8 num);

// LE
void img_hdr_set_auto_le(u8 enable);		// 0: manual mode; 1: auto mode
u8 img_hdr_get_auto_le(void);
int img_hdr_set_le_param(le_curve_config_t *p_le_config,local_exposure_t* p_le_info, const u8 num);	// if p_le_config == NULL, use p_le_info only
int img_hdr_get_le_param(le_curve_config_t *p_le_config,local_exposure_t* p_le_info, const u8 num);	// if p_le_config == NULL, use p_le_info only

// HDR ALPHA BLENDING
void img_hdr_set_auto_alpha_map(u8 enable);		// 0: manual mode; 1: auto mode
u8 img_hdr_get_auto_alpha_map(void);
int img_hdr_set_alpha_map_blend(alpha_map_config_t * p_alpha_blend,const u8 num);
int img_hdr_get_alpha_map_blend(alpha_map_config_t * p_alpha_blend,const u8 num);
int img_hdr_set_alpha_map_lut_offset(out_lut_offset_t * p_alpha_lut_offset,const u8 num);
int img_hdr_get_alpha_map_lut_offset(out_lut_offset_t * p_alpha_lut_offset,const u8 num);
int img_hdr_set_alpha_map_gap(int gap);	// gap: 0~64, default: 64
int img_hdr_get_alpha_map_gap(void);

// COLOR CORRECTION
int img_hdr_set_cc_matrix(color_correction_t * p_color_corr,const u8 num);
int img_hdr_set_cc_tone(tone_curve_t * p_tone,const u8 num);
int img_hdr_get_cc_tone(tone_curve_t * p_tone,const u8 num);

//COLOR CONTRAST
int img_hdr_set_color_contrast(int contrast);		// unit:64, 0~128
int img_hdr_set_auto_color_contrast(int enable); //0 : disable, 1 : enable
int img_hdr_set_auto_color_contrast_strength(int strength); //0~128, 0: no effect; 128: full effect

// YUV CONTRAST ENHANCEMENT
int img_hdr_set_yuv_contrast_low_pass_radius(u8 radius);	// radius: 0 ~ 2
int img_hdr_get_yuv_contrast_low_pass_radius(void);
int img_hdr_set_yuv_contrast_gain(u16 gain);		// gain: 0 ~ 4095, unit: 1024, default: 256
int img_hdr_get_yuv_contrast_gain(void);
int img_hdr_set_yuv_contrast_enable(u8 enable);	// enable : 1; disable : 0
int img_hdr_get_yuv_contrast_enable(void);

// Noise filter overall strength level adjustment
int img_hdr_get_img_property(image_property_t *p_img_prop);
int img_hdr_set_bw_mode(u8 enable);			// 0: disable; 1: enable
int img_hdr_set_sharpness(int level);			// 0~10: default is 6
int img_hdr_set_spacial_denoise(int level);		// 0~10: default is 0
int img_hdr_set_color_style(img_color_style mode);
int img_hdr_set_color_saturation(int saturation);	// 0 ~ 255, unit is 64
int img_hdr_set_color_hue(int hue);				// -15 ~ 15
int img_hdr_set_color_brightness(int brightness);	// -255 ~ 255
int img_hdr_set_yuv_contrast_strength(int strength);	// 0 ~ 128: unit is 32
int img_hdr_set_mctf_strength(int level);					// 0 ~ 10: default is 6
int img_hdr_set_chroma_noise_filter_strength(int level);		// 0 ~ 256, unit is 64
int img_hdr_set_chroma_noise_filter_max_radius(int max_radius);	// 32, 64 or 128

#ifdef __cplusplus
}
#endif

#endif //__IMG_HDR_API_H__

