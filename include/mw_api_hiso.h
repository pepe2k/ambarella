
#ifndef __MW_API_H__
#define __MW_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "basetypes.h"
#include <config.h>


/********************** Global configuration **********************/

int mw_get_driver_info(mw_driver_info *pDriverInfo);
int mw_get_version_info(mw_version_info *pVersion);
int mw_get_log_level(int * pLog);
int mw_set_log_level(mw_log_level log);
int mw_get_lib_version(mw_aaa_lib_version* pAlgo_lib_version,
	mw_aaa_lib_version* pDsp_lib_version);


/********************** Image configuration **********************/
int mw_init_aaa(int fd, int mode);
int mw_start_aaa(void);
int mw_stop_aaa(void);
int mw_deinit_aaa(void);


/********************** Exposure Control Settings **********************/
int mw_enable_ae(int enable);
int mw_get_ae_param(mw_ae_param *ae_param);
int mw_set_ae_param(mw_ae_param *ae_param);
int mw_get_exposure_level(int *pExposure_level);
int mw_set_exposure_level(int *exposure_level);

int mw_get_shutter_time(int *pShutter_time);
int mw_get_sensor_gain(int *pGain_db);
int mw_set_sensor_gain(int *gain_db);
int mw_set_shutter_time(int *shutter_time);
int mw_get_ae_luma_value(int *pLuma);
int mw_get_ae_lines(mw_ae_line *lines, u32 line_num);
int mw_set_ae_lines(mw_ae_line *lines, u32 line_num);
#if 0

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

int mw_get_ae_points(mw_ae_point *point_arr, u32 point_num);
int mw_set_ae_points(mw_ae_point *point_arr, u32 point_num);
int mw_set_ae_area(int enable);

#ifdef CONFIG_ARCH_S2
int mw_set_sensor_gain(int fd_iav, int *gain_db);
int mw_set_shutter_time(int fd_iav, int *shutter_time);
int mw_set_exposure_level(int *exposure_level);
#else
int mw_set_sensor_gain(int fd_iav, int gain_db);
int mw_set_shutter_time(int fd_iav, int shutter_time);
int mw_set_exposure_level(int exposure_level);
#endif
#endif

/********************** White Balance Settings **********************/
int mw_enable_awb(int enable);

int mw_get_awb_method(awb_work_method_t *wb_method);
int mw_set_awb_method(awb_work_method_t wb_method);
int mw_set_awb_mode(awb_control_mode_t wb_mode);
int mw_get_awb_mode(awb_control_mode_t *pWb_mode);
int mw_get_wb_cal(wb_gain_t *pGain);
int mw_set_custom_gain(wb_gain_t * pGain);

/********************** Image Adjustment Settings **********************/
int mw_get_saturation(int *pSaturation);
int mw_set_saturation(int saturation);
int mw_get_brightness(int *pBrightness);
int mw_set_brightness(int brightness);
int mw_get_contrast(int *pContrast);
int mw_set_contrast(int contrast);



/********************** Image Enhancement Settings **********************/
int mw_enable_adj(int enable);
int mw_get_sharpening_strength(int *pStrength);
int mw_set_sharpening_strength(int strength);
int mw_get_mctf_strength(int *pStrength);
int mw_set_mctf_strength(int strength);
int mw_get_night_mode(int *pDn_mode);
int mw_set_night_mode(int dn_mode);


/********************** Calibration related APIs **********************/
int mw_load_calibration_file(mw_cali_file * pFile);


/*****************************************************************/


#ifdef __cplusplus
}
#endif

#endif //  __MW_API_H__

