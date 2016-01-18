#ifndef	IMG_API_ADV_ARCH_H
#define	IMG_API_ADV_ARCH_H

#ifdef __cplusplus
extern "C" {
#endif

//#include "config.h"
#include "basetypes.h"

// basic 3A related APIs
int img_hiso_lib_init(int fd_iav, u32 enable_defblc, u32 enable_sharpen_b);
int img_hiso_lib_deinit(void);
void img_hiso_algo_get_lib_version(img_lib_version_t* p_img_algo_info );
int img_hiso_config_pipeline(IMG_PIPELINE_SEL pipeline, u32 exposure_num);
int img_hiso_start_aaa(int fd_iav);
int img_hiso_stop_aaa(void);
int img_hiso_prepare_idsp(int fd_iav);

int img_hiso_set_work_mode(int mode); //0 -preview
int img_hiso_config_stat_tiles(statistics_config_t *tile_cfg);
int img_hiso_adj_load_cc_binary(cc_binary_addr_t* addr);
int img_hiso_adj_init_containers_liso(fc_collection_t* src_fc);
int img_hiso_dynamic_load_containers(fc_collection_t* p_fc,fc_collection_hiso_t* p_hiso_fc);
int img_hiso_dynamic_load_cc_bin(cc_binary_addr_t* addr);
int img_hiso_adj_init_containers_hiso(fc_collection_hiso_t* src_fc_hiso);
int img_hiso_config_ae_tables(ae_cfg_tbl_t * ae_tbl,int tbl_num);
int img_hiso_config_sensor_info(sensor_config_t * config);
int img_hiso_config_stat_tiles(statistics_config_t * tile_cfg);
int img_hiso_get_sensor_agc();
int img_hiso_get_sensor_shutter();
int img_hiso_set_sensor_agc(int fd_iav,u32 gain_tbl_idx);
int img_hiso_set_sensor_shutter(int fd_iav,u32 shutter_row);
int img_hiso_enable_ae(int enable);
int img_hiso_enable_awb(int enable);
int img_hiso_enable_af(int enable);
int img_hiso_enable_adj(int enable);
void img_hiso_get_3a_cntl_status(aaa_cntl_t *cntl);
int img_hiso_config_awb_param(img_awb_param_t* p_awb_param);

//ae
int img_hiso_set_ae_speed(u8* speed_level);
int img_hiso_set_ae_exp_lines(line_t* line, u16* line_num, u16* line_belt);
int img_hiso_set_ae_target_ratio(u16* target_ratio);
int img_hiso_set_ae_loosen_belt(u16* belt);
int img_hiso_set_ae_meter_mode(u8* mode);
int img_hiso_set_ae_sync_delay(u8* shutter_delay, u8* agc_delay);
int img_hiso_set_ae_backlight(int* backlight_enable);
int img_hiso_register_custom_aeflow_func(ae_flow_func_t* custom_ae_flow_func);
int img_hiso_get_ae_cfa_luma(u16* ae_cfa_luma);
int img_hiso_get_ae_rgb_luma(u16* ae_rgb_luma);
int img_hiso_get_ae_system_gain(u32* ae_system_gain);
int img_hiso_get_ae_stable(u8* ae_stable_flag);
int img_hiso_get_ae_cursor(u8* ae_cursor);
int img_hiso_get_ae_target_ratio(u16* ae_target_ratio);
int img_hiso_format_ae_line(int fd_iav, line_t* ae_lines,int line_num, u32 db_step);//shutter_index to shutter_row used in ae algo

//awb
int img_hiso_get_awb_manual_gain(wb_gain_t* p_wb_gain);
int img_hiso_set_awb_custom_gain(wb_gain_t* p_cus_gain);
int img_hiso_set_awb_cali_shift(wb_gain_t* p_wb_org_gain, wb_gain_t* p_wb_ref_gain);
int img_hiso_set_awb_cali_thre(u16 *p_r_th, u16 *p_b_th);
int img_hiso_set_awb_speed(u8* p_awb_spd);
int img_hiso_set_awb_failure_remedy(awb_failure_remedy_t* awb_failure_remedy);
int img_hiso_get_awb_mode(awb_control_mode_t *p_awb_mode);
int img_hiso_set_awb_mode(awb_control_mode_t *p_awb_mode);
int img_hiso_get_awb_method(awb_work_method_t *p_awb_method);
int img_hiso_set_awb_method(awb_work_method_t *p_awb_method);
int img_hiso_get_awb_env(awb_environment_t *p_awb_env);
int img_hiso_set_awb_env(awb_environment_t *p_awb_env);
//af
int img_hiso_register_lens_drv(lens_dev_drv_t custom_lens_drv);
int img_hiso_lens_init(void);
int img_hiso_config_lens_info(lens_ID lensID);
int img_hiso_af_set_range(af_range_t * p_af_range);
int img_hiso_af_set_mode(af_mode mode);
int img_hiso_af_set_roi(af_ROI_config_t* af_ROI);
int img_hiso_af_set_zoom_idx(u16 zoom_idx);
int img_hiso_af_set_focus_idx(s32 focus_idx);
//int img_af_set_reset(void);
void img_hiso_af_load_param(void* p_af_param, void* p_zoom_map);

//calibartion
int img_hiso_cal_vignette(vignette_cal_t *vig_detect_setup);
int img_hiso_cali_bad_pixel(int fd_iav, cali_badpix_setup_t *pCali_badpix_setup);

//property
int img_hiso_set_color_hue(int hue);			//-15 - +15: -30deg - +30 deg
int img_hiso_set_color_brightness(int brightness);	//-255 - +255
int img_hiso_set_color_contrast(int contrast);	//unit = 64, 0 ~ 128
int img_hiso_set_color_saturation(int saturation);	//unit = 64
int img_hiso_set_bw_mode(u8 mode);// 0: diable, 1: enable
int img_hiso_set_sharpening_strength(u8 str_level);//unit =64, 0-128
int img_hiso_get_img_property(image_property_t *p_img_prop);
int img_hiso_set_mctf_strength(u8 str_level);//unit = 64 ,0 ~256
#ifdef __cplusplus
}
#endif

#endif	// IMG_API_H

