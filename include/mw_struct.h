
#ifndef __MW_STRUCT_H__
#define __MW_STRUCT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "basetypes.h"

typedef u32			mw_id;
typedef u32			mw_stream;

#define	MW_MAX_STREAM_NUM		(4)
#define	MW_MAX_BUFFER_NUM		(4)
#define	MW_NUM_EXPOSURE_CURVE	(256)
#define	MW_MAX_ROI_NUM			(4)
#define	MW_MAX_MD_THRESHOLD		(200)

#define	MW_TRANSPARENT			(0)
#define	MW_NONTRANSPARENT		(255)
#define	MW_OSD_AREA_NUM			(3)
#define	MW_OSD_CLUT_NUM			(MW_MAX_STREAM_NUM * MW_OSD_AREA_NUM)
#define	MW_OSD_CLUT_SIZE			(1024)
#define	MW_OSD_CLUT_OFFSET		(0)
#define	MW_OSD_YUV_OFFSET		(MW_OSD_CLUT_NUM * MW_OSD_CLUT_SIZE)


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
#define SHUTTER_1BY33_SEC				15360000		//(512000000 / 33.3)
#define SHUTTER_1BY40_SEC				12800000		//(512000000 / 40)
#define SHUTTER_1BY50_SEC				10240000		//(512000000 / 50)
#define SHUTTER_1BY60_SEC				8533333		//(512000000 / 60)
#define SHUTTER_1BY100_SEC				5120000		//(512000000 / 100)
#define SHUTTER_1BY120_SEC				4266667		//(512000000 / 120)
#define SHUTTER_1BY240_SEC				2133333		//(512000000 / 240)
#define SHUTTER_1BY480_SEC				1066667		//(512000000 / 480)
#define SHUTTER_1BY960_SEC				533333		//(512000000 / 960)
#define SHUTTER_1BY1024_SEC				500000		//(512000000 / 1024)
#define SHUTTER_1BY8000_SEC				64000		//(512000000 / 8000)

#define LENS_FNO_UNIT		65536

#define	MW_ERROR_LEVEL      0
#define 	MW_MSG_LEVEL          1
#define 	MW_INFO_LEVEL         2
#define 	MW_DEBUG_LEVEL      3
#define 	MW_LOG_LEVEL_NUM  4
typedef int mw_log_level;

typedef enum {
	MW_MANUAL_IRIS = 0,
	MW_P_IRIS,
	MW_DC_IRIS,
	MW_LENS_TYPE_NUM,
	MW_LENS_TYPE_FIRST = 0,
	MW_LENS_TYPE_LAST = MW_LENS_TYPE_NUM,
} MW_LENS_TYPE;

typedef enum {
	MW_IAV_STATE_IDLE = 0,
	MW_IAV_STATE_PREVIEW = 1,
	MW_IAV_STATE_ENCODING = 2,
	MW_IAV_STATE_STILL_CAPTURE = 3,
	MW_IAV_STATE_DECODING = 4,
	MW_IAV_STATE_TRANSCODING = 5,
	MW_IAV_STATE_DUPLEX = 6,
	MW_IAV_STATE_INIT = 0xFF,
} MW_IAV_STATE;

typedef enum {
	MW_ID_1ST		= (1 << 0),
	MW_ID_2ND		= (1 << 1),
	MW_ID_3RD		= (1 << 2),
	MW_ID_4TH		= (1 << 3),
	MW_ID_5TH		= (1 << 4),
	MW_ID_6TH		= (1 << 5),
	MW_ID_7TH		= (1 << 6),
	MW_ID_8TH		= (1 << 7),
} MW_ID;


typedef enum {
	MW_BUFFER_1	= 0,
	MW_BUFFER_2,
	MW_BUFFER_3,
	MW_BUFFER_4,
	MW_BUFFER_TOTAL_NUM,
} MW_BUFFER_ID;


typedef enum {
	MW_ENCODE_NORMAL_MODE = 0,
	MW_ENCODE_HIGH_MEGA_MODE,
	MW_ENCODE_LOW_DELAY_MODE,
	MW_ENCODE_TOTAL_NUM_MODE,
} MW_ENCODE_MODE;


typedef enum {
	MW_ENCODE_OFF,
	MW_ENCODE_H264,
	MW_ENCODE_MJPEG,
} MW_ENCODE_TYPE;


typedef enum {
	MW_LE_STOP = 0,
	MW_LE_AUTO,
	MW_LE_2X,
	MW_LE_3X,
	MW_LE_4X,
	MW_LE_TOTAL_NUM,
} mw_local_exposure_mode;


typedef enum {
	MW_ANTI_FLICKER_50HZ			= 0,
	MW_ANTI_FLICKER_60HZ			= 1,
} mw_anti_flicker_mode;


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


typedef enum {
	MW_CBR_MODE = 0,
	MW_VBR_MODE,
	MW_CBR_QUALITY_MODE,
	MW_VBR_QUALITY_MODE,
	MW_BRC_MODE_NUM,
} mw_brc_mode;


typedef enum {
	MW_FR_NORMAL = 0,
	MW_FR_HORI_FLIP = (1 << 0),
	MW_FR_VERT_FLIP = (1 << 1),
	MW_FR_CLOCKWISEROTATE_90 = (1 << 2),
	MW_FR_ROTATE_180 = ((1 << 1) | (1 << 0)),
	MW_FR_CLOCKWISEROTATE_270 = ((1 << 2) | (1 << 1) | (1 << 0)),
} mw_flip_rotate_mode;


typedef enum {
	MW_PM_ADD_INC = 0,	// add include region
	MW_PM_ADD_EXC,		// add exclude region
	MW_PM_REPLACE,		// replace with new region
	MW_PM_REMOVE_ALL,	// remove all regions
	MW_PM_ACTIONS_NUM,
} mw_privacy_mask_action;


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

typedef struct {
	u32				mirror_pattern;
	u32				bayer_pattern;
} mw_mirror_mode;


typedef struct {
	u32				enable;
	u32				mode;
	u32				frame_rate;
	mw_mirror_mode	mirror_mode;
} mw_vin_mode;


typedef struct {
	u32				mode;
	u32				type;
} mw_vout_mode;


typedef struct {
	mw_id			id;
	u32				state;
} mw_info;


typedef struct {
	u32				buffer;
	u16				width;
	u16				height;
	u8				deintlc_for_intlc_vin;
	u8				type;
	u8				reserved[2];
} mw_source_buffer_format;


typedef struct {
	u16				main_width;
	u16				main_height;
	u16				main_deintlc_for_intlc_vin;
	u16				second_width;
	u16				second_height;
	u16				second_deintlc_for_intlc_vin;
	u16				third_width;
	u16				third_height;
	u16				third_deintlc_for_intlc_vin;
	u16				fourth_width;
	u16				fourth_height;
	u16				fourth_deintlc_for_intlc_vin;

	u16				intlc_scan;
	u16				reserved;
} mw_source_buffer_format_all;


typedef struct {
	u32				main_buffer_type;
	u32				second_buffer_type;
	u32				third_buffer_type;
	u32				fourth_buffer_type;
} mw_source_buffer_type_all;


typedef struct {
	mw_info			stream[MW_MAX_STREAM_NUM];
	mw_info			buffer[MW_MAX_BUFFER_NUM];
	mw_source_buffer_format	format[MW_MAX_BUFFER_NUM];
} mw_encode_info;


typedef struct mw_encode_format_s {
	mw_stream		stream;
	u8				encode_type;		// 0: none, 1: H.264, 2: MJPEG
	u8				source;
	u8				flip_rotate;
	u8				reserved;
	u16				encode_width;
	u16				encode_height;
	u16				encode_x;
	u16				encode_y;
	u32				encode_fps;
} mw_encode_format;


typedef struct mw_h264_config_s {
	mw_stream		stream;

	u8				M;
	u8				N;
	u8				idr_interval;
	u8				gop_model;
	u8				profile;
	u8				brc_mode;	// 0: CBR; 1: VBR; 2: CBR keep quality; 3: VBR keep quality
	u16				reserved;

	u32				cbr_avg_bps;
	u32				vbr_min_bps;
	u32				vbr_max_bps;
} mw_h264_config;


typedef struct {
	mw_stream		stream;
	u8				chroma_format;		// 0: YUV 422, 1: YUV 420
	u8				quality;				// 1 ~ 100, 100 is best quality
	u16				reserved;
} mw_jpeg_config;


typedef struct {
	mw_encode_format	format;
	mw_h264_config		h264;
	mw_jpeg_config		mjpeg;
	u8					dptz;
	u8					reserved[3];
} mw_encode_stream;


typedef struct mw_bitrate_range_s {
	mw_id			id;
	u32				min_bps;
	u32				max_bps;
} mw_bitrate_range;


typedef struct mw_frame_factor_s {
	mw_id			id;
	u32				enc_fps;
} mw_frame_factor;


typedef struct {
} mw_qp_param;


typedef struct {
	u32				source_buffer;
	u32				zoom_factor;
	int				offset_x;
	int				offset_y;
} mw_dptz_param;

typedef struct {
	u32				source_buffer;
	u32				zoom_factor_x;
	u32				zoom_factor_y;
	int				offset_x;
	int				offset_y;
} mw_dptz_org_param;


typedef struct {
	u32		FNO_min;
	u32		FNO_max;
	u32		aperture_min;
	u32		aperture_max;
} mw_aperture_param;

typedef struct {
	mw_anti_flicker_mode	anti_flicker_mode;
	u32					shutter_time_min;
	u32					shutter_time_max;
	u32					sensor_gain_max;
	u32					slow_shutter_enable;
	u32					ir_led_mode;
	u32					current_vin_fps;
	mw_aperture_param	lens_aperture;
} mw_ae_param;

typedef struct {
	int					p_coef;
	int					i_coef;
	int					d_coef;
} mw_dc_iris_pid_coef;

typedef struct {
	int					open_threshold;
	int					close_threshold;
	int					open_delay;
	int					close_delay;
	int					threshold_1X;
	int					threshold_2X;
	int					threshold_3X;
} mw_ir_led_control_param;

typedef enum {
	MW_IR_LED_MODE_OFF = 0,
	MW_IR_LED_MODE_ON = 1,
	MW_IR_LED_MODE_AUTO = 2
} mw_ir_led_mode;

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
	MW_AE_SPOT_METERING = 0,
	MW_AE_CENTER_METERING,
	MW_AE_AVERAGE_METERING,
	MW_AE_CUSTOM_METERING,
	MW_AE_METERING_TYPE_NUMBER,
} mw_ae_metering_mode;


typedef struct {
	int				metering_weight[96];
} mw_ae_metering_table;


typedef struct {
	s32				saturation;
	s32				brightness;
	s32				hue;
	s32				contrast;
	s32				sharpness;
} mw_image_param;


typedef struct {
	char				lect_filename[64];		// local exposure curve table filename
	u32				lect_reload;
	char				gct_filename[64];		// gamma curve table filename
	u32				gct_reload;
} mw_iq_param;

#define	MW_WB_MODE_HOLD			(100)

typedef struct {
	u32				wb_mode;
} mw_awb_param;


typedef struct {
	u32				lens_type;
	u16				af_mode;
	u16				af_tile_mode;
	u16				zm_dist;
	u16				fs_dist;
	u16				fs_near;
	u16				fs_far;
} mw_af_param;


typedef struct {
	u32			agc;
	u32			shutter;
} mw_image_stat_info;


typedef struct {
	u32			r_gain;
	u32			g_gain;
	u32			b_gain;
	u16			d_gain;
	u16			reserved;
} mw_wb_gain;


typedef struct {
	u16			gain_curve_table[MW_NUM_EXPOSURE_CURVE];
} mw_local_exposure_curve;


// MUST keep the byte order in this struct
typedef struct {
	u8				v;
	u8				u;
	u8				y;
	u8				a;
} mw_clut;


typedef struct {
	mw_stream		stream;
	u8				area;
	u8				enable;
	u16				reserved;
	u16				offset_x;
	u16				offset_y;
	char *			bmp_filename;
} mw_osd_bmp;


typedef struct {
	mw_stream		stream;
	u8				area;
	u8				enable;
	u16				reserved;
	u16				offset_x;
	u16				offset_y;
	u32				size;
	char *			text_string;
} mw_osd_string;


typedef struct {
	mw_stream		stream;
	u8				area;
	u8				enable;
	u16				width;
	u16				height;
	u16				offset_x;
	u16				offset_y;
	u16				clut_num;
	mw_clut *		clut;
	u8 *				data;
	u32				data_len;
} mw_osd_overlay;


typedef struct {
	int				unit; //0- percent ,1-pixel
	int				left;
	int				top;
	int				width;
	int				height;
	u32				color;		// 0:Black, 1:Red, 2:Blue, 3:Green, 4:Yellow, 5:Magenta, 6:Cyan, 7:White
	u32				action;
} mw_privacy_mask;


typedef struct {
	mw_dptz_param	dptz;
	mw_privacy_mask	pm;
} mw_dptz_pm_param;


typedef struct {
	char *			jpeg_filename;
	u32				jpeg_w;
	u32				jpeg_h;
	u32				thumbnail_w;
	u32				thumbnail_h;
	u32				pic_num;
	u16				quality;
	u16				cap_raw : 1;
	u16				cap_jpeg : 1;
	u16				cap_thumbnail : 1;
	u16				keep_AR_flag : 1;
	u16				reserved : 12;
} mw_stilcap_param;


typedef struct {
} mw_audio_param;


typedef struct {
} mw_alarm_handle_func;


typedef enum {
	NO_MOTION = 0,
	IN_MOTION,
} mw_motion_status_type;


typedef enum {
	EVENT_NO_MOTION = 0,
	EVENT_MOTION_START,
	EVENT_MOTION_END,
} mw_motion_event_type;


typedef int (*alarm_handle_func)(const int *p_motion_event);


typedef struct {
	u16	x;
	u16	y;
	u16	width;
	u16	height;
	u16	threshold;
	u16	sensitivity;
	u16	valid;
} mw_roi_info;


typedef struct {
	char bad_pixel[64];		// for bad pixel calibration data
	char wb[64];			// for white balance calibration data
	char shading[64];		// for lens shading calibration data
} mw_cali_file;


typedef struct {
	u8 hdr_mode;
	u8 hdr_expo_num;
	u8 hdr_shutter_mode;
	u8 reserverd;
	u16 step;
	u16 max_g_db;
	u16 max_ag_db;
	u16 max_dg_db;
} mw_sensor_param;


/*******************************************************
 *
 *	Parse Configuration Parameters
 *
 *******************************************************/
typedef enum {
	MAP_TO_U32 = 0,
	MAP_TO_U16,
	MAP_TO_U8,
	MAP_TO_S32,
	MAP_TO_S16,
	MAP_TO_S8,
	MAP_TO_DOUBLE,
	MAP_TO_STRING,
	MAP_TO_UNKNOWN = -1,
} mapping_data_type;


typedef enum {
	NO_LIMIT = 0,
	MIN_MAX_LIMIT,
	MIN_LIMIT,
	MAX_LIMIT,
} mapping_data_constraint;


typedef struct {
	const char * 				TokenName;		// name
	void * 					Address;		// address
	mapping_data_type		Type;			// type: 0 - u32, 1 - u16, 2 - u8, 3 - int(s32), 4 - double, 5 - char[]>
	double					Default;			// default value>
	mapping_data_constraint	ParamLimits;	// 0 - no limits, 1 - min and max limits, 2 - only min limit, 3 - only max limit
	double					MinLimit;		// Minimun value
	double					MaxLimit;		// Maximun value
	int						StringLengthLimit;	// Dimension of type char[]
} Mapping;

#define	FILE_NAME_LENGTH		64

typedef enum {
	FILE_TYPE_ADJ = 0,
	FILE_TYPE_AEB = 1,
	FILE_TYPE_PIRIS = 2,
	FILE_TYPE_TOTAL_NUM,
	FILE_TYPE_FIRST = FILE_TYPE_ADJ,
	FILE_TYPE_LAST = FILE_TYPE_TOTAL_NUM,
} MW_AAA_FILE_TYPE;


typedef struct mw_mctf_one_chan_s {
	u8  temporal_alpha;
	u8  temporal_alpha1;
	u8  temporal_alpha2;
	u8  temporal_alpha3;
	u8  temporal_t0;
	u8  temporal_t1;
	u8  temporal_t2;
	u8  temporal_t3;
	u8  temporal_maxchange;
	u16 radius;        //0-256
	u16 str_3d;        //0-256
	u16 str_spatial;   //0-256
	u16 level_adjust;  //0-256
} mw_mctf_one_chan_t;

typedef struct mw_mctf_info_s {
	mw_mctf_one_chan_t chan_info[3]; //YCbCr
	u16 combined_str_y; //0-256
} mw_mctf_info_t;

#ifdef __cplusplus
}
#endif

#endif //  __MW_STRUCT_H__

