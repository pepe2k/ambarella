
#ifndef _MW_PRI_STRUCT_H_
#define _MW_PRI_STRUCT_H_
//#include "mw_struct.h"
#include "mw_defines.h"

#define	MW_EXPOSURE_LEVEL_MIN		(25)
#define	MW_EXPOSURE_LEVEL_MAX		(400)

#define	MW_SATURATION_MIN			(0)
#define	MW_SATURATION_MAX			(255)
#define	MW_BRIGHTNESS_MIN			(-255)
#define	MW_BRIGHTNESS_MAX			(255)
#define	MW_CONTRAST_MIN				(0)
#define	MW_CONTRAST_MAX				(128)
#define	MW_HUE_MIN				(0)
#define	MW_HUE_MAX				(255)

#define	MW_SHARPENING_STR_MIN			(0)
#define	MW_SHARPENING_STR_MAX			(128)

#define	MW_MCTF_STRENGTH_MIN		(0)
#define	MW_MCTF_STRENGTH_MAX		(256)

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

typedef struct aaa_files_s {
	char		adj_file[FILE_NAME_LENGTH];
	char		aeb_file[FILE_NAME_LENGTH];
	char		lens_file[FILE_NAME_LENGTH];
} aaa_files_t;


//init
typedef struct {
	int	aaa_enable;
	int	mode;
	int	nl_enable;
} mw_init_param;

//Sensor
typedef struct sensor_model_s {
	u32		sensor_id;
	char		name[32];
	u32		default_fps;
	u32		current_fps;
	u32		vin_mode;
	u32		sensor_op_mode;
	u32		lens_id;
	u8		hdr_mode;
	u8		hdr_expo_num;
	u8		hdr_shutter_mode;
	u8		reserverd;
	u32		db_step;
	u16		max_g_db;
	u16		max_ag_db;
	u16		max_dg_db;
	u16		reserved1;
	aaa_files_t	load_files;
} mw_sensor_param;


#define	MW_NUM_EXPOSURE_CURVE	(256)
typedef struct {
	u16			gain_curve_table[MW_NUM_EXPOSURE_CURVE];
} mw_local_exposure_curve;

typedef struct {
	int				metering_weight[96];
} mw_ae_metering_table;


//AF
typedef struct {
	u32				lens_type;
	u16				af_mode;
	u16				af_tile_mode;
	u16				zm_idx;
	u16				fs_idx;
	u16				fs_near;
	u16				fs_far;
} _mw_af_info;

//AWB
typedef struct {
	int		enable;
	int		wb_mode;
	int		wb_method;
} mw_awb_param;

//IMAGE
typedef struct {
	int		saturation;
	int		brightness;
	int		hue;
	int		contrast;
} mw_image_param;

//ADJ
typedef struct {
	int		sharpen_strength;
	int		mctf_strength;
	int		dn_mode;
} mw_enhance_param;

//iris
enum iris_type_enum {
	DC_IRIS = 0,
	P_IRIS = 1,
};

typedef struct {
	int	iris_enable;
	int	iris_type;
} mw_iris_param;

typedef struct {
	int	fd;
	mw_init_param	init_params;
	mw_sensor_param	sensor_params;
	mw_ae_param		ae_params;
	mw_awb_param	awb_params;
	mw_image_param	image_params;
	mw_enhance_param	enh_params;
	mw_iris_param	iris_params;
} _mw_global_config;

#endif //  _MW_PRI_STRUCT_H_
