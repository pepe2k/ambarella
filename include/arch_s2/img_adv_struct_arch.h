#ifndef	IMG_ADV_STRUCT_ARCH_H
#define	IMG_ADV_STRUCT_ARCH_H

#include "basetypes.h"
#include "AmbaDataType.h"
#include "img_abs_filter.h"
#include "AmbaDSP_ImgDef.h"
#include "AmbaDSP_ImgFilter.h"

#define  VIN0_VSYNC		"/proc/ambarella/vin0_vsync"

#define MAX_AWB_TILE_COL		(32)
#define MAX_AWB_TILE_ROW		(32)
#define MAX_AWB_TILE_NUM		(MAX_AWB_TILE_COL * MAX_AWB_TILE_ROW)
#define MAX_AE_TILE_COL		(12)
#define MAX_AE_TILE_ROW		(8)
#define MAX_AE_TILE_NUM		(MAX_AE_TILE_COL * MAX_AE_TILE_ROW)
#define MAX_AF_TILE_COL		(16)
#define MAX_AF_TILE_ROW		(16)
#define MAX_AF_TILE_NUM		(MAX_AF_TILE_COL * MAX_AF_TILE_ROW)
#define SHT_NL_TABLE_SIZE 		(64)
#define STEP16_GAIN_TABLE_SIZE 	(161)
#define STEP20_GAIN_TABLE_SIZE		(201)
#define STEP57_GAIN_TABLE_SIZE 	(571)
#define STEP60_GAIN_TABLE_SIZE		(601)
#define STEP64_GAIN_TABLE_SIZE		(641)
#define	SHUTTER_TIME_TABLE_LENGTH	(2304)
#define	SHUTTER_TIME_TABLE_LENGTH_1	(SHUTTER_TIME_TABLE_LENGTH-1)
#define IK_MEM_SIZE		(4<<20)

#define	SHUTTER_TIME_TABLE_LENGTH	(2304)
#define	SHUTTER_TIME_TABLE_LENGTH_1	(SHUTTER_TIME_TABLE_LENGTH-1)
#define	SHUTTER_DGAIN_TABLE_LENGTH	(1292)//1012=>2303
#define	AGC_DGAIN_TABLE_LENGTH	(769)//0=>768
#define	MIN_HDR_EXPOSURE_NUM		1
#define	MAX_HDR_EXPOSURE_NUM		4
#define   MAX_AE_LINES_NUM				32


typedef enum{
	SHUTTER_AUTO = 0,
	SHUTTER_2S = SHUTTER_TIME_TABLE_LENGTH_1-256,
	SHUTTER_1S = SHUTTER_TIME_TABLE_LENGTH_1-384,
	SHUTTER_1BY2 = SHUTTER_TIME_TABLE_LENGTH_1-512,
	SHUTTER_1BY4 = SHUTTER_TIME_TABLE_LENGTH_1-640,
	SHUTTER_1BY7P5 = SHUTTER_TIME_TABLE_LENGTH_1 -756,
	SHUTTER_1BY8 = SHUTTER_TIME_TABLE_LENGTH_1-768,
	SHUTTER_1BY10 = SHUTTER_TIME_TABLE_LENGTH_1-809,
	SHUTTER_1BY12P5 = SHUTTER_TIME_TABLE_LENGTH_1-850,
	SHUTTER_1BY15 = SHUTTER_TIME_TABLE_LENGTH_1-884,
	SHUTTER_1BY20 = SHUTTER_TIME_TABLE_LENGTH_1-937,
	SHUTTER_1BY25 = SHUTTER_TIME_TABLE_LENGTH_1-978,
	SHUTTER_1BY30 = SHUTTER_TIME_TABLE_LENGTH_1-1012,
	SHUTTER_1BY50 = SHUTTER_TIME_TABLE_LENGTH_1-1106,
	SHUTTER_1BY60 = SHUTTER_TIME_TABLE_LENGTH_1-1140,
	SHUTTER_1BY100 = SHUTTER_TIME_TABLE_LENGTH_1-1234,
	SHUTTER_1BY120 = SHUTTER_TIME_TABLE_LENGTH_1-1268,
	SHUTTER_1BY200 =SHUTTER_TIME_TABLE_LENGTH_1-1362,
	SHUTTER_1BY240 = SHUTTER_TIME_TABLE_LENGTH_1-1396,
	SHUTTER_1BY480 = SHUTTER_TIME_TABLE_LENGTH_1-1524,
	SHUTTER_1BY960 = SHUTTER_TIME_TABLE_LENGTH_1-1652,
	SHUTTER_1BY1024 = SHUTTER_TIME_TABLE_LENGTH_1-1664,
	SHUTTER_1BY8000 = SHUTTER_TIME_TABLE_LENGTH_1-2046,
	SHUTTER_1BY16000 = SHUTTER_TIME_TABLE_LENGTH_1 -2174,
	SHUTTER_1BY32000 =SHUTTER_TIME_TABLE_LENGTH_1 -2302,
}ae_shutter_mode;
typedef enum {
	ISO_AUTO = 0,
	ISO_100	 = 0,	//0db
	ISO_150 = 3,
	ISO_200 = 6,	//6db
	ISO_300 = 9,
	ISO_400 = 12,	//12db
	ISO_600 = 15,
	ISO_800 = 18,	//18db
	ISO_1600 = 24,	//24db
	ISO_3200 = 30,	//30db
	ISO_6400 = 36,	//36db
	ISO_12800 = 42,	//42db
	ISO_25600 = 48,	//48db
	ISO_51200 = 54,	//54db
	ISO_102400 = 60,//60db
	ISO_204800 = 66,//66db
	ISO_409600 = 72,//72db
	ISO_819200 = 78,//78db
}ae_iso_mode;

#define	IRIS_FNO_TABLE_LENGTH	(2522)
#define	IRIS_FNO_TABLE_LENGTH_1	(IRIS_FNO_TABLE_LENGTH-1)
typedef enum {
	APERTURE_AUTO = 0,
	APERTURE_F64 = IRIS_FNO_TABLE_LENGTH_1-1881,
	APERTURE_F45 = IRIS_FNO_TABLE_LENGTH_1-1752,
	APERTURE_F32 = IRIS_FNO_TABLE_LENGTH_1-1624,
	APERTURE_F22 = IRIS_FNO_TABLE_LENGTH_1-1496,
	APERTURE_F16 = IRIS_FNO_TABLE_LENGTH_1-1367,
	APERTURE_F11 = IRIS_FNO_TABLE_LENGTH_1-1239,
	APERTURE_F8 = IRIS_FNO_TABLE_LENGTH_1-1110,
	APERTURE_F5P6 = IRIS_FNO_TABLE_LENGTH_1-982,
	APERTURE_F4 = IRIS_FNO_TABLE_LENGTH_1-853,
	APERTURE_F2P8 = IRIS_FNO_TABLE_LENGTH_1-725,
	APERTURE_F2 = IRIS_FNO_TABLE_LENGTH_1-596,
	APERTURE_F1P4 = IRIS_FNO_TABLE_LENGTH_1-468,
	APERTURE_F1P2 = IRIS_FNO_TABLE_LENGTH_1-407,
}ae_aperture_mode;

typedef struct joint_s {
/*	s16	shutter;		// shall be values in ae_shutter_mode
	s16	gain;		//shall be values in ae_iso_mode
	s16	aperture;	//shall be values in ae_aperture_mode
*/
	s32	factor[3];	//0-shutter, 1-gain, 2-iris
}joint_t;

typedef struct line_s {
	joint_t	start;
	joint_t	end;
}line_t;
typedef struct ae_info_s {
	u16	dgain_update;
	u16	shutter_update;
	u16	agc_update;
	u16	iris_update;
	u16	shutter_index;
	u16  agc_index;
	u16	iris_index;
	u16	dgain;
}ae_info_t;

typedef struct aaa_tile_report_s {
	u16 awb_tile;
	u16 ae_tile;
	u16 af_tile;
}aaa_tile_report_t;
typedef struct aaa_tile_info_s {
	u16 awb_tile_col_start;
	u16 awb_tile_row_start;
	u16 awb_tile_width;
	u16 awb_tile_height;
	u16 awb_tile_active_width;
	u16 awb_tile_active_height;
	u16 awb_rgb_shift;
	u16 awb_y_shift;
	u16 awb_min_max_shift;
	u16 ae_tile_col_start;
	u16 ae_tile_row_start;
	u16 ae_tile_width;
	u16 ae_tile_height;
	u16 ae_y_shift;
	u16 ae_linear_y_shift;
	u16 af_tile_col_start;
	u16 af_tile_row_start;
	u16 af_tile_width;
	u16 af_tile_height;
	u16 af_tile_active_width;
	u16 af_tile_active_height;
	u16 af_y_shift;
	u16 af_cfa_y_shift;
	u8  awb_tile_num_col;
	u8  awb_tile_num_row;
	u8  ae_tile_num_col;
	u8  ae_tile_num_row;
	u8  af_tile_num_col;
	u8  af_tile_num_row;
	u8 total_slices_x;
	u8 total_slices_y;
	u8 slice_index_x;
	u8 slice_index_y;
	u16 slice_width;
	u16 slice_height;
	u16 slice_start_x;
	u16 slice_start_y;
	u8   total_exposure;
	u8   exposure_index;
	u16 reserved[31];
}aaa_tile_info_t;

struct af_statistics_info{
	u16 af_tile_num_col;
	u16 af_tile_num_row;
	u16 af_tile_col_start;
	u16 af_tile_row_start;
	u16 af_tile_width;
	u16 af_tile_height;
	u16 af_tile_active_width;
	u16 af_tile_active_height;
	int id_1;
	int id_2;
};

typedef struct af_statistics_ex_s {
	u8 af_horizontal_filter1_mode;
	u8 af_horizontal_filter1_stage1_enb;
	u8 af_horizontal_filter1_stage2_enb;
	u8 af_horizontal_filter1_stage3_enb;
	s16 af_horizontal_filter1_gain[7];
	u16 af_horizontal_filter1_shift[4];
	u16 af_horizontal_filter1_bias_off;
	u16 af_vertical_filter1_thresh;
	u8 af_horizontal_filter2_mode;
	u8 af_horizontal_filter2_stage1_enb;
	u8 af_horizontal_filter2_stage2_enb;
	u8 af_horizontal_filter2_stage3_enb;
	s16 af_horizontal_filter2_gain[7];
	u16 af_horizontal_filter2_shift[4];
	u16 af_horizontal_filter2_bias_off;
	u16 af_vertical_filter2_thresh;
	u16 af_tile_fv1_horizontal_shift;
	u16 af_tile_fv1_vertical_shift;
	u16 af_tile_fv1_horizontal_weight;
	u16 af_tile_fv1_vertical_weight;
	u16 af_tile_fv2_horizontal_shift;
	u16 af_tile_fv2_vertical_shift;
	u16 af_tile_fv2_horizontal_weight;
	u16 af_tile_fv2_vertical_weight;
} af_statistics_ex_t;

typedef struct sensor_info_s {
	u8 sensor_id;
	u8 field_format;          /**< Field */
	u8 sensor_resolution;     /**< Number of bits for data representation */
	u8 sensor_pattern;        /**< Bayer patterns RG, BG, GR, GB */
	u8 first_line_field_0;
	u8 first_line_field_1;
	u8 first_line_field_2;
	u8 first_line_field_3;
	u8 first_line_field_4;
	u8 first_line_field_5;
	u8 first_line_field_6;
	u8 first_line_field_7;
	u32 sensor_readout_mode;
} sensor_info_t;

typedef struct statistics_config_s {
	u8	enable;
	u8	auto_shift;
	u16	awb_tile_num_col;
	u16	awb_tile_num_row;
	u16	awb_tile_col_start;
	u16	awb_tile_row_start;
	u16	awb_tile_width;
	u16	awb_tile_height;
	u16	awb_tile_active_width;
	u16	awb_tile_active_height;
	u16	awb_pix_min_value;
	u16	awb_pix_max_value;
	u16	ae_tile_num_col;
	u16	ae_tile_num_row;
	u16	ae_tile_col_start;
	u16	ae_tile_row_start;
	u16	ae_tile_width;
	u16	ae_tile_height;
	u16	af_tile_num_col;
	u16	af_tile_num_row;
	u16	af_tile_col_start;
	u16	af_tile_row_start;
	u16	af_tile_width;
	u16	af_tile_height;
	u16	af_tile_active_width;
	u16	af_tile_active_height;
	u16	ae_pix_min_value;
	u16	ae_pix_max_value;
}statistics_config_t;

typedef struct float_tile_config_s{
	u16 tile_col_start;
	u16 tile_row_start;
	u16 tile_width;
	u16 tile_height;
	u16 tile_shift;
}float_tile_config_t;

typedef struct float_statistics_config_s{
	u16 number_of_tiles;
	float_tile_config_t float_tiles_config[32];
}float_statistics_config_t;

typedef struct awb_data_s {
	u32 r_avg;
	u32 g_avg;
	u32 b_avg;
	u32 lin_y;
	u32 cr_avg;
	u32 cb_avg;
	u32 non_lin_y;
}awb_data_t;

typedef struct ae_data_s {
	u32 lin_y;
	u32 non_lin_y;
}ae_data_t;


//RAW statistics for customer 3A
typedef struct af_stat_s {
	u16 sum_fy;
	u16 sum_fv1;
	u16 sum_fv2;
}af_stat_t;

#define HIST_BIN_NUM	64

typedef struct rgb_histogram_stat_s {
	u32 his_bin_y[HIST_BIN_NUM];
	u32 his_bin_r[HIST_BIN_NUM];
	u32 his_bin_g[HIST_BIN_NUM];
	u32 his_bin_b[HIST_BIN_NUM];
}rgb_histogram_stat_t;

typedef struct float_rgb_af_stat_s{
	u32 sum_fv1;
	u32 sum_fv2;
}float_rgb_af_stat_t;

typedef struct rgb_aaa_stat_s { // 3712
	aaa_tile_info_t	tile_info;
	u16				frame_id;
	af_stat_t			af_stat[MAX_AF_TILE_NUM];
	u16				ae_sum_y[MAX_AE_TILE_NUM];
	rgb_histogram_stat_t	histogram_stat;
	float_rgb_af_stat_t		float_rgb_af_stat[32];
	u32					float_ae_sum_y[32];
	u8				reserved[190];
}rgb_aaa_stat_t;

typedef struct cfa_awb_stat_s {
	u16	sum_r;
	u16	sum_g;
	u16	sum_b;
	u16	count_min;
	u16	count_max;
}cfa_awb_stat_t;

typedef struct cfa_ae_stat_s {
	u16	lin_y;
	u16	count_min;
	u16	count_max;
}cfa_ae_stat_t;

typedef struct cfa_histogram_stat_s {
	u32	his_bin_r[HIST_BIN_NUM];
	u32	his_bin_g[HIST_BIN_NUM];
	u32	his_bin_b[HIST_BIN_NUM];
	u32	his_bin_y[HIST_BIN_NUM];
}cfa_histogram_stat_t;
typedef struct histogram_stat_s
{
	cfa_histogram_stat_t cfa_histogram;
	rgb_histogram_stat_t rgb_histogram;
}histogram_stat_t;
typedef  struct float_cfa_awb_stat_s {
	u32 sum_r;
	u32 sum_g;
	u32 sum_b;
	u32 count_min;
	u32 count_max;
}  float_cfa_awb_stat_t;

typedef  struct float_cfa_ae_stat_s {
	u32 lin_y;
	u32 count_min;
	u32 count_max;
}  float_cfa_ae_stat_t;

typedef  struct float_cfa_af_stat_s {
	u32 sum_fv1;
	u32 sum_fv2;
}  float_cfa_af_stat_t;

typedef struct cfa_aaa_stat_s {
	aaa_tile_info_t	tile_info;
	u16				frame_id;
	cfa_awb_stat_t	awb_stat[MAX_AWB_TILE_NUM];
	cfa_ae_stat_t		ae_stat[MAX_AE_TILE_NUM];
	af_stat_t			af_stat[MAX_AF_TILE_NUM];
	cfa_histogram_stat_t	histogram_stat;
	float_cfa_awb_stat_t	float_awb_stat[32];
	float_cfa_ae_stat_t	float_ae_stat[32];
	float_cfa_af_stat_t	float_af_stat[32];
	u8				reserved[190];
}cfa_aaa_stat_t;
typedef struct embed_hist_stat_s{
	u8   valid;
	u16 frame_cnt;
	u16 frame_id;
	u32 mean;
	u32 hist_begin;
	u32 hist_end;
	u32 mean_low_end;
	u32 perc_low_end;
	u32 norm_abs_dev;
	u32 hist_bin_data[256];
}embed_hist_stat_t;
typedef struct img_aaa_stat_s {
	aaa_tile_info_t 	tile_info;
	awb_data_t		awb_info[MAX_AWB_TILE_NUM];
	ae_data_t 		ae_info[MAX_AE_TILE_NUM];
	af_stat_t			af_info[MAX_AF_TILE_NUM];
	cfa_histogram_stat_t	cfa_hist;
	rgb_histogram_stat_t	rgb_hist;
	embed_hist_stat_t 		sensor_hist_info;
}img_aaa_stat_t;
typedef struct adj_aeawb_control_s {
	u8	low_temp_r_target;
	u8	low_temp_b_target;
	u8	d50_r_target;
	u8	d50_b_target;
	u8	high_temp_r_target;
	u8	high_temp_b_target;
	u16	ae_target;
//	u8	auto_knee;
} adj_aeawb_control_t;

/////////awb
#define	AWB_UNIT_GAIN		(1024)
#define	AWB_UNIT_RATIO	(128)

typedef enum{
	WB_ENV_INDOOR = 0,
	WB_ENV_INCANDESCENT,
	WB_ENV_D4000,
	WB_ENV_D5000,
	WB_ENV_SUNNY,
	WB_ENV_CLOUDY,
	WB_ENV_FLASH,
	WB_ENV_FLUORESCENT,
	WB_ENV_FLUORESCENT_H,
	WB_ENV_UNDERWATER,
	WB_ENV_CUSTOM,
	WB_ENV_OUTDOOR,
	WB_ENV_AUTOMATIC,
	WB_ENV_NUMBER,
}awb_environment_t;

typedef enum {
	WB_AUTOMATIC = 0,
	WB_INCANDESCENT,	// 2800K
	WB_D4000,
	WB_D5000,
	WB_SUNNY,		// 6500K
	WB_CLOUDY,		// 7500K
	WB_FLASH,
	WB_FLUORESCENT,
	WB_FLUORESCENT_H,
	WB_UNDERWATER,
	WB_CUSTOM,
	WB_OUTDOOR,
	WB_MODE_NUMBER,
}awb_control_mode_t;

typedef enum {
	AWB_NORMAL = 0,
	AWB_MANUAL,
	AWB_GREY_WORLD,
	AWB_METHOD_NUMBER,
}awb_work_method_t;

typedef enum{
	AWB_FAIL_DO_NOTHING = 0,
	AWB_FAIL_CONST_GAIN,
	AWB_FAIL_NUMBER,
}awb_failure_remedy_t;

typedef enum {
	AE_FULL_AUTO = 0,
	AE_SHUTTER_PRIORITY,
	AE_APERTURE_PRIORITY,
	AE_PROGRAM,
	AE_MANUAL,
}ae_work_mode;

typedef	struct wb_gain_s {
	u32	r_gain;
	u32	g_gain;
	u32	b_gain;
} wb_gain_t;

typedef struct awb_gain_s {
	u16		video_gain_update;
	u16		still_gain_update;
	wb_gain_t	video_wb_gain;
	wb_gain_t	still_wb_gain;
}awb_gain_t;

typedef struct awb_diff_data_s{
	wb_gain_t _pre_raw_wb_gain;
	u32 _d_50_a_diff;
	u32 _d_40_a_diff;
	u8 _spd_flg;
	u8 _diff_first_flg;
	u8 _d_a_diff_flg;
	u8 _diff_flg;
	s32 _diff_count;
	u32 _diff;
	u32 _raw_diff;
	u32 _diff_same;
}awb_diff_data_t;

typedef struct awb_lut_unit_s {
	u16 gr_min;
	u16 gr_max;
	u16 gb_min;
	u16 gb_max;
	s16 y_a_min_slope;
	s16 y_a_min;
	s16 y_a_max_slope;
	s16 y_a_max;
	s16 y_b_min_slope;
	s16 y_b_min;
	s16 y_b_max_slope;
	s16 y_b_max;
	s8  weight;
} awb_lut_unit_t;

#define MAX_AWB_WR_NUM (20)
typedef struct awb_lut_s {
	u8		lut_no;
	awb_lut_unit_t	awb_lut[MAX_AWB_WR_NUM];
} awb_lut_t;
typedef struct awb_lut_idx_s{
	u8 start;
	u8 num;
}awb_lut_idx_t;
typedef enum{
	COMP_FWD = 0,
	COMP_BWD,
}wb_cali_comp_t;
typedef struct white_adj_s {
	s32 r_ratio;
	s32 b_ratio;
}white_adj_t;
typedef struct wb_cali_s{
	wb_gain_t org_gain[2];
	wb_gain_t ref_gain[2];
	wb_gain_t comp_gain[2];
	u16 thre_r;
	u16 thre_b;
}wb_cali_t;
typedef struct img_awb_param_s {
	wb_gain_t	menu_gain[WB_MODE_NUMBER];
	awb_lut_t		wr_table;
	awb_lut_idx_t awb_lut_idx[MAX_AWB_WR_NUM];
}img_awb_param_t;

////////////////////AE
typedef enum {
	AE_SPOT_METERING = 0,
	AE_CENTER_METERING,
	AE_AVERAGE_METERING,
	AE_CUSTOM_METERING,
	AE_TILE_METERING,
	AE_Y_ORDER_METERING,
	AE_EXTERN_DESIGN_METERING,
	AE_METERING_TYPE_NUMBER,
}ae_metering_mode;

typedef struct ae_output_s {

	u32					shutter_update;
	u32					shutter_index;
	u32					agc_update;
	u32					agc_index;
	u32					dgain_update;
	u32					dgain;
	u32					iris_update;
	u32					iris_index;

}ae_output_t;
typedef struct ae_param_s{
	u16 *	p_ae_target;
	line_t **	p_expo_lines;
	u32* p_sht_table;
	u32* p_gain_table;
	u32 sht_nolinear_index;
	u32 gain_table_size;
}ae_param_t;

typedef struct ae_cfg_tbl_s {
	line_t*	p_expo_lines;
	u32	line_num;
	u32	belt;
	u32*	p_gain_tbl;
	u32	db_step;
	u32	gain_tbl_size;
	u32*	p_sht_nl_tbl;
	u32	sht_nl_tbl_size;
}ae_cfg_tbl_t;
//AF/////////////////////////////////////////////////////
/**
 * The supported Lens ID
 */
typedef enum{
/* fixed lens ID list */
	LENS_CMOUNT_ID = 0,
/*p-iris lens ID list*/
	LENS_M13VP288IR_ID = 2,
/*single focus lens ID list*/
	LENS_CM8137_ID = 10,
	LENS_SY3510A_ID = 11,
	LENS_IU072F_ID = 12,
/*zoom lens ID list*/
	LENS_TAMRON18X_ID = 101,
	LENS_JCD661_ID = 102,
	LENS_SY6310_ID = 103,
/*custom lens ID list*/
	LENS_CUSTOM_ID = 900,
}lens_ID;

typedef struct lens_dev_drv_s {
	int (*set_IRCut)(u8 enable);
	int (*set_shutter_speed)(u8 ex_mode, u16 ex_time);
	int (*lens_init)(void);
	int (*lens_park)(void);
	int (*zoom_stop)(void);
	int (*focus_near)(u16 pps, u32 distance);
	int (*focus_far)(u16 pps, u32 distance);
	int (*zoom_in)(u16 pps, u32 distance);
	int (*zoom_out)(u16 pps, u32 distance);
	int (*focus_stop)(void);
	int (*set_aperture)(u16 aperture_idx);
	int (*set_mechanical_shutter)(u8 me_shutter);
	void (*set_zoom_pi)(u8 pi_n);
	void (*set_focus_pi)(u8 pi_n);
	int (*lens_standby)(u8 en);
	int (*isFocusRuning)(void);
	int (*isZoomRuning)(void);
} lens_dev_drv_t;
typedef struct lens_control_s{ // for algo internal use
	u8 focus_update;
	u8 zoom_update;
	s32 focus_pulse;
	s32 zoom_pulse;
	s16 pps;
	af_statistics_ex_t af_stat_config;
	u8 af_stat_config_update;
}lens_control_t;
typedef struct af_range_s{
	s32 far_bd;
	s32 near_bd;
}af_range_t;
typedef enum{
	CAF = 0,
	SAF = 1,
	MANUAL = 3,
	CALIB = 4,
	DEBUG = 5,
}af_mode;
typedef struct af_ROI_config_s{
	u32	tiles[MAX_AF_TILE_NUM];
}af_ROI_config_t;

typedef struct af_control_s{
	af_mode workingMode;
	af_ROI_config_t af_window;
	u8 zoom_idx;
	s32 focus_idx; //only avilable in MANUAL mode
}af_control_t;

/////3A


typedef struct aaa_cntl_s {
	u8	awb_enable;
	u8	ae_enable;
	u8	af_enable;
	u8	adj_enable;
}aaa_cntl_t;
///adj////////////////////////////////////////////////////////
typedef enum {
	OV_9715 = 0,
	OV_2710,
	OV_5653,
	OV_14810,
	MT_9J001,
	MT_9P031,
	MT_9M033,
	MT_9T002,
	IMX_035,
	IMX_036,
	IMX_072,
	SAM_S5K5B3,
	AR_0331,
	MN_34041PL,
	MN_34210PL,
	MN_34220PL,
	IMX_121,
	IMX_172,
	IMX_178,
	IMX_104,
	IMX_136,
	IMX_185,
	IMX_226,
	IMX_224,
	OV_5658,
	OV_4689,
}sensor_label;

typedef enum bayer_pattern {
	RG = 0,
	BG = 1,
	GR = 2,
	GB = 3
} bayer_pattern;
typedef struct sensor_config_s{

	bayer_pattern pattern;//BG//GB//GR//RG
	u16 max_gain_db;//<60
	u16 gain_step;//16,20,64...
	u8 shutter_delay;//1,2,3
	u8 agc_delay;//1,2
	sensor_label sensor_lb;
}sensor_config_t;

///hdr///////////////

#define	VIDEO_FPS_UNIT		(100)

typedef struct hdr_shutter_gp_s{
	u32 shutter_info[MAX_HDR_EXPOSURE_NUM];
}hdr_shutter_gp_t;

typedef struct hdr_gain_gp_s{
	u32 gain_info[MAX_HDR_EXPOSURE_NUM];
}hdr_gain_gp_t;


//#### new start here




typedef struct img_lib_version_s {
	int	major;
	int	minor;
	int	patch;
	u32	mod_time;
	char	description[64];
}  img_lib_version_t;
//new img algo start here
typedef struct cc_binary_addr_s{
	void* cc_0;
	void* cc_1;
	void* cc_2;
	void* cc_3;
}cc_binary_addr_t;

typedef struct fc_collection_s{
	filter_container_p4_t *fc_ae_target;
	filter_container_p8_t *fc_wb_ratio;
	filter_container_p8_t *fc_blc;
	filter_container_p2_t *fc_antialiasing;
	filter_container_p4_t *fc_grgbmismatch;
	filter_container_p4_t *fc_dpc;
	filter_container_p32_t *fc_cfanf_low;
	filter_container_p32_t *fc_cfanf_high;
	filter_container_p8_t1_t *fc_le;
	filter_container_p8_t *fc_demosaic;
	filter_container_p4_t *fc_cc;
	filter_container_p0_t3_t *fc_tone;
	filter_container_p4_t3_t *fc_rgb2yuv;
	filter_container_p8_t1_t *fc_chroma_scale;
	filter_container_p8_t *fc_chroma_median;
	filter_container_p8_t *fc_chroma_nf;
	filter_container_p2_t *fc_cdnr;
	filter_container_p2_t *fc_1stmode_sel;
	filter_container_p64_t4_t *fc_asf;
	filter_container_p8_t *fc_1st_shpboth;
	filter_container_p16_t4_t *fc_1st_shpnoise;
	filter_container_p16_t4_t *fc_1st_shpfir;
	filter_container_p0_t1_t *fc_1st_shpcoring;
	filter_container_p8_t *fc_1st_shpcoring_idx_scale;
	filter_container_p8_t *fc_1st_shpcoring_min;
	filter_container_p8_t *fc_1st_shpcoring_scale_coring;
	filter_container_p8_t *fc_final_shpboth;
	filter_container_p16_t4_t *fc_final_shpnoise;
	filter_container_p16_t4_t *fc_final_shpfir;
	filter_container_p0_t1_t *fc_final_shpcoring;
	filter_container_p8_t *fc_final_shpcoring_idx_scale;
	filter_container_p8_t *fc_final_shpcoring_min;
	filter_container_p8_t *fc_final_shpcoring_scale_coring;
	filter_container_p64_t *fc_video_mctf;
}fc_collection_t;
typedef   struct image_property_s {
	int	brightness;
	int 	contrast;
	int 	saturation;
	int 	hue;
}  image_property_t;
typedef enum {
	IMG_COLOR_TV = 0,
	IMG_COLOR_PC,
	IMG_COLOR_STILL,
	IMG_COLOR_CUSTOM,
	IMG_COLOR_NUMBER,
}img_color_style;

///calibration//////
typedef struct vignette_cal_s {
#define VIGNETTE_MAX_WIDTH (33)
#define VIGNETTE_MAX_HEIGHT (33)
#define VIGNETTE_MAX_SIZE (VIGNETTE_MAX_WIDTH*VIGNETTE_MAX_HEIGHT)
/*input*/
	u16* raw_addr;
	u16 raw_w;
	u16 raw_h;
	bayer_pattern bp;
	u32 threshold; // if ((max/min)<<10 > threshold) => NG
	u16 compensate_ratio;  //1024 for fully compensated , 0 for no compensation
	u8 lookup_shift; // 0 for 3.7, 1 for 2.8, 2 for 1.9, 3 for 0.10, 255 for auto select
	u8 reserved;
	AMBA_DSP_IMG_BLACK_CORRECTION_s blc;
/*output*/
	u16* r_tab;
	u16* ge_tab;
	u16* go_tab;
	u16* b_tab;
} vignette_cal_t;

typedef struct cali_badpix_setup_s {
	u8* badpixmap_buf;
	u32 cap_width;
	u32 cap_height;
	u32 cali_mode; // video or still
	u32 badpix_type; // hot pixel for cold pixel
	u32 block_h;
	u32 block_w;
	u32 upper_thres;
	u32 lower_thres;
	u32 detect_times;
	u32 shutter_idx;
	u32 agc_idx;
	u8 save_raw_flag;
	u8 reserved[3];
} cali_badpix_setup_t;
typedef struct fc_collection_hiso_s{
	//HISO filters
	filter_container_p2_t *fc_antialiasing_hiso;
	filter_container_p4_t *fc_grgbmismatch_hiso;
	filter_container_p4_t *fc_dpc_hiso;
	filter_container_p32_t *fc_cfanf_hiso;
	filter_container_p8_t *fc_demosaic_hiso;
	filter_container_p8_t *fc_chroma_median_hiso;
	filter_container_p2_t *fc_cdnr_hiso;

	filter_container_p64_t4_t *fc_asf_hiso;
	filter_container_p64_t4_t *fc_asf_hiso_high;
	filter_container_p64_t4_t *fc_asf_hiso_low;
	filter_container_p64_t4_t *fc_asf_hiso_med1;
	filter_container_p64_t4_t *fc_asf_hiso_med2;

	filter_container_p8_t *fc_shpboth_hihigh;
	filter_container_p16_t4_t *fc_shpnoise_hihigh;
	filter_container_p16_t4_t *fc_shpfir_hihigh;
	filter_container_p0_t1_t *fc_shpcoring_hihigh;
	filter_container_p8_t *fc_shpcoring_idx_scale_hihigh;
	filter_container_p8_t *fc_shpcoring_min_hihigh;
	filter_container_p8_t *fc_shpcoring_scale_coring_hihigh;

	filter_container_p8_t *fc_shpboth_himed;
	filter_container_p16_t4_t *fc_shpnoise_himed;
	filter_container_p16_t4_t *fc_shpfir_himed;
	filter_container_p0_t1_t *fc_shpcoring_himed;
	filter_container_p8_t *fc_shpcoring_idx_scale_himed;
	filter_container_p8_t *fc_shpcoring_min_himed;
	filter_container_p8_t *fc_shpcoring_scale_coring_himed;

	filter_container_p8_t *fc_shpboth_hili;
	filter_container_p16_t4_t *fc_shpnoise_hili;
	filter_container_p16_t4_t *fc_shpfir_hili;
	filter_container_p0_t1_t *fc_shpcoring_hili;
	filter_container_p8_t *fc_shpcoring_idx_scale_hili;
	filter_container_p8_t *fc_shpcoring_min_hili;
	filter_container_p8_t *fc_shpcoring_scale_coring_hili;

	filter_container_p8_t *fc_chroma_nf_hihigh;
	filter_container_p4_t *fc_chroma_nf_hilowlow;
	filter_container_p8_t *fc_chroma_nf_hipre;
	filter_container_p8_t *fc_chroma_nf_himed;
	filter_container_p8_t *fc_chroma_nf_hilow;
	filter_container_p8_t *fc_chroma_nf_hiverylow;

	filter_container_p32_t *fc_chroma_combine_med;
	filter_container_p32_t *fc_chroma_combine_low;
	filter_container_p32_t *fc_chroma_combine_verylow;
	filter_container_p16_t *fc_luma_combine_noise;
	filter_container_p16_t *fc_combine_lowasf;
	filter_container_p64_t *fc_combine_iso;
	filter_container_p16_t4_t *fc_freq_recover;
	filter_container_p16_t	*fc_ta;

}fc_collection_hiso_t;

typedef enum{
	IMG_PIPELINE_LISO,
	IMG_PIPELINE_HDR,
	IMG_PIPELINE_HISO,
	IMG_PIPELINE_NUM,
}IMG_PIPELINE_SEL;
typedef struct ae_target_s{
	u16 target[MAX_HDR_EXPOSURE_NUM];
}ae_target_t;
typedef struct aaa_api_s{
	int (*p_ae_flow_control_init)();
	int (*p_ae_flow_control)(img_aaa_stat_t* p_aaa_data,ae_output_t *p_ae_output);
	int (*p_awb_flow_control_init)();
	int (*p_awb_flow_control)(img_aaa_stat_t* p_aaa_data, awb_gain_t *p_awb_gain);
	int (*p_af_control_init)(af_control_t* p_af_control, void* p_af_param, void* p_zoom_map, lens_control_t* p_lens_ctrl);
	int (*p_af_control)(af_control_t* p_af_control,img_aaa_stat_t* p_aaa_data, lens_control_t * p_lens_ctrl, ae_output_t* p_ae_info, u8 lens_runing_flag);
	void (*p_af_set_range)(af_range_t* p_af_range);
	void (*p_af_set_calib_param)(void* p_calib_param);
}aaa_api_t;
typedef struct ae_flow_func_s{

	u32 (*p_ae_flow_calc_luma_stat)(void* thisp,img_aaa_stat_t* p_aaa_data);
	u16 (*p_ae_flow_calc_target)(void* thisp,img_aaa_stat_t* p_aaa_data);
}ae_flow_func_t;
#endif
