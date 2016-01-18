

#ifndef __MW_IMAGE_PRIV_H__
#define __MW_IMAGE_PRIV_H__

#define	MW_EXPOSURE_LEVEL_MIN		(25)
#define	MW_EXPOSURE_LEVEL_MAX		(400)

#define	MW_SATURATION_MIN			(0)
#define	MW_SATURATION_MAX			(255)
#define	MW_BRIGHTNESS_MIN			(-255)
#define	MW_BRIGHTNESS_MAX			(255)
#define	MW_CONTRAST_MIN				(0)
#define	MW_CONTRAST_MAX				(128)
#define	MW_SHARPNESS_MIN			(0)
#define	MW_SHARPNESS_MAX			(10)

#define	MW_MCTF_STRENGTH_MIN		(0)
#define	MW_MCTF_STRENGTH_MAX		(11)

#define	MW_DC_IRIS_BALANCE_DUTY_MAX	(800)
#define	MW_DC_IRIS_BALANCE_DUTY_MIN	(400)

#define	FOCUS_INFINITY		(50000)
#define	FOCUS_MACRO			(50)

#define	IMGPROC_PARAM_PATH	"/etc/idsp"

typedef enum {
	MW_CALIB_BPC = 1,
	MW_CALIB_WB,
	MW_CALIB_LENS_SHADING,
	MW_CONVERT_VIG_TO_AWB,
} MW_CALIBRATION_TYPE;

extern int fd_iav_aaa;
extern mw_ae_param			G_mw_ae_param;
extern mw_image_param		G_mw_image;
extern mw_awb_param			G_mw_awb;
extern _mw_af_info			G_mw_af;
extern sensor_model_t			G_mw_sensor_model;
extern u32					G_mctf_strength;
extern u32					G_auto_local_exposure_mode;
extern int					G_exposure_level[MAX_HDR_EXPOSURE_NUM];
extern mw_cali_file				G_mw_cali;
extern img_config_info_t		G_img_config_info;

inline int is_rgb_sensor_vin(void);
int get_sensor_aaa_params(int fd, sensor_model_t * sensor);
int config_sensor_lens_info(sensor_model_t * sensor);
int load_dsp_cc_table(void);
int load_adj_cc_table(sensor_model_t *sensor);
inline int get_vin_mode(u32 * vin_mode);
inline int get_vin_frame_rate(u32 * pFrame_time);
inline int get_shutter_time(u32 *pShutter_time);
int load_ae_exp_lines(mw_ae_param *ae);
int get_ae_exposure_lines(mw_ae_line *lines, u32 num);
int set_ae_exposure_lines(mw_ae_line *lines, u32 num);
int set_ae_switch_points(mw_ae_point *points, u32 num);
int set_day_night_mode(u32 dn_mode);
int create_slowshutter_task(void);
int destroy_slowshutter_task(void);
int load_calibration_data(mw_cali_file *file, MW_CALIBRATION_TYPE type);
int load_default_params(void);
int check_af_params(mw_af_param *pAF);
int reload_previous_params(void);
inline int check_state(void);
int set_chroma_noise_filter_max(int fd_iav);

int load_mctf_bin(void);

int get_sensor_aaa_params_from_bin(sensor_model_t * sensor);
int check_aaa_state(int fd_iav);
int load_binary_file(void);
int load_hdr_param_load_adj_aeb(void);
int get_hdr_sensor_adj_aeb_param(image_sensor_param_t* info,
	hdr_sensor_param_t* hdr_info, sensor_model_t * sensor,
	hdr_sensor_cfg_t *p_sensor_config);
u32 shutter_index_to_q9(int index);
int shutter_q9_to_index(u32 shutter_time);

int start_aaa_task(void);
int stop_aaa_task(void);
int send_image_msg_stop_aaa(void);

int init_netlink(void);
void * netlink_loop(void * data);

#endif // __MW_IMAGE_PRIV_H__

