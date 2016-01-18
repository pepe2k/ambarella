
#ifndef __MW_STRUCT_H__
#define __MW_STRUCT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "basetypes.h"


//AE
#define SHUTTER_INVALID				0
#define SHUTTER_1_SEC					512000000		//(512000000 / 1)
#define SHUTTER_1BY2_SEC				256000000		//(512000000 / 2)
#define SHUTTER_1BY3_SEC				170666667		//(512000000 / 3)
#define SHUTTER_1BY3P125_SEC			163840000		//(512000000 / 25 * 8)
#define SHUTTER_1BY4_SEC				128000000		//(512000000 / 4)
#define SHUTTER_1BY5_SEC				102400000		//(512000000 / 5)
#define SHUTTER_1BY6_SEC				85333333		//(512000000 / 6)
#define SHUTTER_1BY6P25_SEC				81920000		//(512000000 / 25 * 4)
#define SHUTTER_1BY7P5_SEC				68266667		//(512000000 / 15 * 2)
#define SHUTTER_1BY10_SEC				51200000		//(512000000 / 10)
#define SHUTTER_1BY12P5_SEC				40960000		//(512000000 / 25 * 2)
#define SHUTTER_1BY15_SEC				34133333		//(512000000 / 15)
#define SHUTTER_1BY20_SEC				25600000		//(512000000 / 20)
#define SHUTTER_1BY24_SEC				21333333		//(512000000 / 24)
#define SHUTTER_1BY25_SEC				20480000		//(512000000 / 25)
#define SHUTTER_1BY30_SEC				17066667		//(512000000 / 30)
#define SHUTTER_1BY50_SEC				10240000		//(512000000 / 50)
#define SHUTTER_1BY60_SEC				8533333		//(512000000 / 60)
#define SHUTTER_1BY100_SEC				5120000		//(512000000 / 100)
#define SHUTTER_1BY120_SEC				4266667		//(512000000 / 120)
#define SHUTTER_1BY240_SEC				2133333		//(512000000 / 240)
#define SHUTTER_1BY480_SEC				1066667		//(512000000 / 480)
#define SHUTTER_1BY960_SEC				533333		//(512000000 / 960)
#define SHUTTER_1BY1024_SEC				500000		//(512000000 / 1024)
#define SHUTTER_1BY8000_SEC				64000		//(512000000 / 8000)
#define SHUTTER_1BY16000_SEC				32000		//(512000000 / 16000)
#define SHUTTER_1BY32000_SEC				16000		//(512000000 / 32000)

typedef enum {
	MW_SHUTTER = 0,
	MW_DGAIN,
	MW_IRIS,
	MW_TOTAL_DIMENSION
} mw_ae_point_dimension;


typedef enum {
	MW_AE_START_POINT = 0,
	MW_AE_END_POINT,
} mw_ae_point_position;


typedef struct {
	s32			factor[3];	// 0:shutter, 1:gain, 2:iris
	u32			pos;		// 0:start, 1:end
} mw_ae_point;


typedef struct {
	mw_ae_point	start;
	mw_ae_point	end;
} mw_ae_line;

typedef enum {
	MW_ANTI_FLICKER_50HZ			= 0,
	MW_ANTI_FLICKER_60HZ			= 1,
} mw_anti_flicker_mode;

typedef struct {
	u32		FNO_min;
	u32		FNO_max;
	u32		aperture_min;
	u32		aperture_max;
} mw_aperture_param;

typedef struct {
	int		ae_enable;
	int		ae_exp_level;
	mw_anti_flicker_mode		anti_flicker_mode;
	u32		shutter_time_min;
	u32		shutter_time_max;
	u32		sensor_gain_max;
	u32		slow_shutter_enable;
	u32		ir_led_mode;
	u32		current_vin_fps;
	mw_aperture_param	lens_aperture;
} mw_ae_param;

//AWB

typedef struct {
	u32			r_gain;
	u32			g_gain;
	u32			b_gain;
	u16			d_gain;
	u16			reserved;
} mw_wb_gain;

#define	MW_WB_MODE_HOLD			(100)

typedef enum {
	MW_WB_NORMAL_METHOD = 0,
	MW_WB_CUSTOM_METHOD,
	MW_WB_GREY_WORLD_METHOD,
	MW_WB_METHOD_NUMBER,
} mw_white_balance_method;


typedef enum {
	MW_WB_AUTO 					= 0,
	MW_WB_INCANDESCENT,			// 2800K
	MW_WB_D4000,
	MW_WB_D5000,
	MW_WB_SUNNY,					// 6500K
	MW_WB_CLOUDY,				// 7500K
	MW_WB_FLASH,
	MW_WB_FLUORESCENT,
	MW_WB_FLUORESCENT_H,
	MW_WB_UNDERWATER,
	MW_WB_CUSTOM,				// custom
	MW_WB_MODE_NUMBER,
} mw_white_balance_mode;


//debug
#define	MW_ERROR_LEVEL      0
#define 	MW_MSG_LEVEL          1
#define 	MW_INFO_LEVEL         2
#define 	MW_DEBUG_LEVEL      3
#define 	MW_LOG_LEVEL_NUM  4
typedef int mw_log_level;

#define	FILE_NAME_LENGTH		64


//version
typedef struct {
	int				arch;
	int				model;
	int				major;
	int				minor;
	int				patch;
	char				description[64];
} mw_driver_info;


typedef struct {
	u32 				major;
	u32 				minor;
	u32 				patch;
	u32				update_time;
} mw_version_info;

typedef struct  {
	int	major;
	int	minor;
	int	patch;
	u32	mod_time;
	char	description[64];
} mw_aaa_lib_version;

typedef enum {
	FILE_TYPE_ADJ = 0,
	FILE_TYPE_AEB = 1,
	FILE_TYPE_PIRIS = 2,
	FILE_TYPE_TOTAL_NUM,
	FILE_TYPE_FIRST = FILE_TYPE_ADJ,
	FILE_TYPE_LAST = FILE_TYPE_TOTAL_NUM,
} MW_AAA_FILE_TYPE;

typedef enum {
	MW_AAA_MODE_LISO = 0,
	MW_AAA_MODE_HISO,
	MW_AAA_MODE_NUM,
	MW_AAA_MODE_FIRST = MW_AAA_MODE_LISO,
	MW_AAA_MODE_LAST = MW_AAA_MODE_NUM - 1,
} MW_AAA_MODE;

//Other
typedef struct {
	char bad_pixel[64];		// for bad pixel calibration data
	char wb[64];			// for white balance calibration data
	char shading[64];		// for lens shading calibration data
} mw_cali_file;

#ifdef __cplusplus
}
#endif

#endif //  __MW_STRUCT_H__

