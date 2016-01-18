
#ifndef __MW_API_H__
#define __MW_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "basetypes.h"
#include <config.h>


/********************** Parse configuration parameter **********************/
int mw_config_params(Mapping * pMap, char * pContent);
int mw_output_params(Mapping * pMap, char ** pContentOut);


/********************** Global configuration **********************/
int mw_init(void);
int mw_uninit(void);
int mw_get_driver_info(mw_driver_info *pDriverInfo);
int mw_get_version_info(mw_version_info *pVersion);
int mw_get_log_level(int * pLog);
int mw_set_log_level(mw_log_level log);
int mw_get_lib_version(mw_aaa_lib_version* pAlgo_lib_version,
	mw_aaa_lib_version* pDsp_lib_version);
int mw_get_iav_state(int fd, int *pState);
int mw_load_aaa_param_file(char *pFile_name, int type);


/********************** VIN configuration **********************/
int mw_get_vin_resolution(u32 * width, u32 * height);
int mw_get_vin_mode(mw_vin_mode * pVIN);
int mw_set_vin_mode(mw_vin_mode * pVIN);
int mw_enable_preview(int enable);


/********************** VOUT configuration **********************/
int mw_get_vout_mode(mw_vout_mode * pVOUT);
int mw_set_vout_mode(mw_vout_mode * pVOUT);


/********************** Video Streams configuration **********************/
int mw_get_encode_mode(u32 *pMode);
int mw_set_encode_mode(u32 encode_mode);
int mw_enable_stream(int stream);
int mw_disable_stream(int stream);
int mw_force_IDR(int stream);
int mw_change_br(mw_bitrate_range * pBR);
int mw_change_fr(mw_frame_factor * pFR);
int mw_get_buffer_info(mw_info *pInfo);
int mw_get_buffers_format(mw_source_buffer_format_all *pFormat);
int mw_set_buffers_format(mw_source_buffer_format_all *pFormat);
int mw_get_buffers_type(mw_source_buffer_type_all *pType);
int mw_set_buffers_type(mw_source_buffer_type_all *pType);
int mw_get_encode_info(mw_info *pInfo);
int mw_get_encode_format(mw_encode_format *pEncodeFormat);
int mw_set_encode_format(mw_encode_format *pEncodeFormat);
int mw_get_h264_config(mw_h264_config *pH264Config);
int mw_set_h264_config(mw_h264_config *pH264Config);
int mw_get_jpeg_config(mw_jpeg_config *pJpegConfig);
int mw_set_jpeg_config(mw_jpeg_config *pJpegConfig);
int mw_get_dptz_param(mw_dptz_param *pDPTZ);
int mw_set_dptz_param(mw_dptz_param *pDPTZ);
int mw_get_dptz_org_param(mw_dptz_org_param * pDPTZ, mw_dptz_param * dptz);
int mw_set_dptz_privacy_mask(mw_dptz_pm_param *pDZPM);
int mw_get_vin_linear_wdr_mode(int *pOpmode);
int mw_set_vin_linear_wdr_mode(int opmode);

/********************** Image configuration **********************/
int mw_start_aaa(int fd_iav);
int mw_stop_aaa(void);
int mw_get_iq_preference(int *pPrefer);
int mw_set_iq_preference(int prefer);
int mw_get_af_param(mw_af_param *pAF);
int mw_set_af_param(mw_af_param *pAF);
int mw_get_image_statistics(mw_image_stat_info *pInfo);
int mw_set_sharpen_filter(int sharpen_b);
int mw_get_sharpen_filter(void);
int mw_init_mctf(int fd, mw_mctf_info_t *mctf_info);

/********************** Exposure Control Settings **********************/
int mw_enable_ae(int enable);
int mw_get_ae_param(mw_ae_param *ae_param);
int mw_set_ae_param(mw_ae_param *ae_param);
int mw_get_exposure_level(int *pExposure_level);
int mw_get_shutter_time(int fd_iav, int *pShutter_time);
int mw_get_sensor_gain(int fd_iav, int *pGain_db);
int mw_get_ae_metering_mode(mw_ae_metering_mode *pMode);
int mw_set_ae_metering_mode(mw_ae_metering_mode mode);
int mw_get_ae_metering_table(mw_ae_metering_table *pAe_metering_table);
int mw_set_ae_metering_table(mw_ae_metering_table *pAe_metering_table);
int mw_set_iris_type(int type);
int mw_get_iris_type(void);
int mw_is_dc_iris_supported(void);
int mw_enable_dc_iris_control(int enable);
int mw_set_dc_iris_pid_coef(mw_dc_iris_pid_coef *pPid_coef);
int mw_get_dc_iris_pid_coef(mw_dc_iris_pid_coef *pPid_coef);
int mw_enable_p_iris_control(int enable);
int mw_set_piris_exit_step(int step);
int mw_get_piris_exit_step(void);
int mw_is_ir_led_supported(void);
int mw_set_ir_led_param(mw_ir_led_control_param *pParam);
int mw_get_ir_led_param(mw_ir_led_control_param *pParam);
int mw_set_ir_led_brightness(int brightness);
int mw_get_ae_luma_value(int *pLuma);
int mw_get_ae_lines(mw_ae_line *lines, u32 line_num);
int mw_set_ae_lines(mw_ae_line *lines, u32 line_num);
int mw_get_ae_points(mw_ae_point *point_arr, u32 point_num);
int mw_set_ae_points(mw_ae_point *point_arr, u32 point_num);
int mw_set_ae_area(int enable);

#ifdef CONFIG_ARCH_S2
int mw_set_sensor_gain(int fd_iav, int *gain_db);
int mw_set_shutter_time(int fd_iav, int *shutter_time);
int mw_set_exposure_level(int *exposure_level);
#else
int mw_set_exposure_level(int exposure_level);
int mw_set_shutter_time(int fd_iav, int shutter_time);
int mw_set_sensor_gain(int fd_iav, int gain_db);
#endif

/********************** White Balance Settings **********************/
int mw_enable_awb(int enable);
int mw_get_awb_param(mw_awb_param *pAWB);
int mw_set_awb_param(mw_awb_param *pAWB);
int mw_get_awb_method(mw_white_balance_method *wb_method);
int mw_set_awb_method(mw_white_balance_method wb_method);
int mw_set_white_balance_mode(mw_white_balance_mode wb_mode);
int mw_get_white_balance_mode(mw_white_balance_mode *pWb_mode);
int mw_get_wb_cal(mw_wb_gain *pGain);
int mw_get_rgb_gain(mw_wb_gain *pGain);
int mw_set_rgb_gain(int fd_iav, mw_wb_gain *pGain);
int mw_set_custom_gain(mw_wb_gain * pGain);


/********************** Image Adjustment Settings **********************/
int mw_get_image_param(mw_image_param *pImage);
int mw_set_image_param(mw_image_param *pImage);
int mw_get_saturation(int *pSaturation);
int mw_set_saturation(int saturation);
int mw_get_brightness(int *pBrightness);
int mw_set_brightness(int brightness);
int mw_get_contrast(int *pContrast);
int mw_set_contrast(int contrast);
int mw_get_sharpness(int *pSharpness);
int mw_set_sharpness(int sharpness);


/********************** Image Enhancement Settings **********************/
int mw_get_mctf_strength(u32 *pStrength);
int mw_set_mctf_strength(u32 strength);
int mw_get_auto_local_exposure_mode(u32 *pMode);
int mw_set_auto_local_exposure_mode(u32 mode);
int mw_set_local_exposure_curve(int fd_iav, mw_local_exposure_curve *pLocal_exposure);
int mw_enable_backlight_compensation(int enable);
int mw_enable_day_night_mode(int enable);
int mw_set_auto_color_contrast(u32 enable);
int mw_get_auto_color_contrast(u32 *pEnable);
int mw_set_auto_color_contrast_strength (u32 value);
int mw_get_adj_status(int *pEnable);
int mw_enable_adj(int enable);
int mw_get_chroma_noise_strength(int *pStrength);
int mw_set_chroma_noise_strength(int strength);
int mw_get_auto_wdr_strength(int *pStrength);
int mw_set_auto_wdr_strength(int strength);
int mw_set_video_freeze(int enable);

/********************** Overlay configuration **********************/
int mw_osd_set_bmp(mw_osd_bmp *pBMP);
int mw_osd_set_string(mw_osd_string *pStr);
int mw_osd_get_overlay(mw_osd_overlay *pOverlay);
int mw_osd_set_overlay(mw_osd_overlay *pOverlay);
int mw_add_privacy_mask(mw_privacy_mask *pPrivacyMask);
int mw_enable_privacy_mask(int enable);	// 0: clear all privacy masks; 1: draw privacy masks added before


/********************** Still capture configuration **********************/
int mw_still_capture(int fd_iav, mw_stilcap_param * pStil);


/********************** Calibration related APIs **********************/
int mw_load_calibration_file(mw_cali_file * pFile);


/********************** Motion detection related APIs **********************/
int mw_md_get_motion_event(int *p_motion_event);
int mw_md_callback_setup(alarm_handle_func alarm_handler);
int mw_md_get_roi_info(int roi_idx, mw_roi_info *roi_info);
int mw_md_set_roi_info(int roi_idx, const mw_roi_info *roi_info);
int mw_md_clear_roi(int roi_idx);
int mw_md_roi_region_display(void);
int mw_md_thread_start(void);
int mw_md_thread_stop(void);
void mw_md_print_event(int flag);


/********************** Audio configuration (not supported yet) **********************/
int mw_get_audio_param(mw_audio_param *pAudio);
int mw_set_audio_param(mw_audio_param *pAudio);


/********************** Face detection related APIs (not supported yet) **********************/
int mw_setup_fd_callback(mw_alarm_handle_func func, void *pData);
int mw_get_fd_facetile_event(u32 *pFace_tile_event);
int mw_set_fd_done(void);
int mw_display_adj_bin_version(void);

/*****************************************************************/
int mw_get_sensor_param_for_3A(mw_sensor_param *psensor_param);
int mw_get_wb_gain(mw_wb_gain *wb_gain);
int mw_set_wb_gain(mw_wb_gain *wb_gain);
int mw_set_lens_id(int lens_id);

#ifdef __cplusplus
}
#endif

#endif //  __MW_API_H__

