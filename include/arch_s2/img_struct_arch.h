#ifndef	IMG_STRUCT_ARCH_H
#define	IMG_STRUCT_ARCH_H

#include "basetypes.h"
typedef signed long long s64;

#define	RGB_AAA_DATA_BLOCK		(5120)
#define	CFA_AAA_DATA_BLOCK		(15872)
#define	RGB_AAA_DATA_BLOCK_SIZE	(RGB_AAA_DATA_BLOCK*(4))
#define	CFA_AAA_DATA_BLOCK_SIZE	(CFA_AAA_DATA_BLOCK*(4))
#define	MAX_TILES_H				(33)
#define	MAX_TILES_V				(33)
#define	MAX_VIGNETTE_NUM			(MAX_TILES_H * MAX_TILES_V * (2))
#define	NUM_BAD_PIXEL_THD		(384)

/* MCTF configuration layout
	config_hdr	 256
	curves_hdr	 768
	sharp_conf_hdr	 112
	lpfs_hdr	 144
	sharp_alpha_hdr	2048
	tone_conf_hdr	 256
	tone_adjust_hdr	4096
	tone_hist_hdr	2048
	--------------------
	MCTF_CFG_SIZE:	9728 = 0x2600
*/
#define MCTF_CFG_SIZE			(27600)
#define SEC_CC_SIZE				(20608)
#define CMPR_CFG_SIZE 			(544)
#define NUM_EXPOSURE_CURVE	(256)

#define INPUT_LOOK_UP_TABLE_SIZE	(192*3*4)
#define MATRIX_TABLE_SIZE			(16*16*16*4)
#define OUTPUT_LOOK_UP_TABLE_SIZE	(256*4)

/*input lookup tabel for color correction 192*3 */
//#define NUM_IN_LOOKUP			(192*3)
/*matrix tabel for color correction 16*16*16 */
//#define NUM_MATRIX			(16*16*16)
#define NUM_OUT_LOOKUP		(256)
#define NUM_CHROMA_GAIN_CURVE	(128)
#define RGB_TO_YUV_MATRIX_SIZE	(9)
#define K0123_ARRAY_SIZE			(24)
#define DYN_BPC_THD_TABLE_SIZE	((3)*(128)*(2))

#define MAX_AE_LINE_NUM		(12)
#define MAX_AWB_TILE_COL		(32)
#define MAX_AWB_TILE_ROW		(32)
#define MAX_AWB_TILE_NUM		(MAX_AWB_TILE_COL * MAX_AWB_TILE_ROW)
#define MAX_AE_TILE_COL		(12)
#define MAX_AE_TILE_ROW		(8)
#define MAX_AE_TILE_NUM		(MAX_AE_TILE_COL * MAX_AE_TILE_ROW)
#define MAX_AF_TILE_COL		(16)
#define MAX_AF_TILE_ROW		(16)
#define MAX_AF_TILE_NUM		(MAX_AF_TILE_COL * MAX_AF_TILE_ROW)
#define NUM_LOOKUP				(32 * 354)
#define NUM_EXPONENT			(64)
#define NUM_BALCK_LEVEL		(32)

#define MAX_BLACK_LEVEL_COLUMNS	(256)
#define MAX_BLACK_LEVEL_ROWS		(128)

#define NUM_INTERCEPT_GAIN		(256)
#define NUM_ROW_GAIN				(4096)  /* unlimited up to sensor size */
#define NUM_COLUM_GAIN			(4096)

#define NUM_CHROMA_GAIN_CURVE	(128)
//#define NUM_ALPHA_TABLE			(512)

#define LS_LINEAR_TABLE_SIZE		(512)
#define LS_INV_LINEAR_TABLE_SIZE	(256)
#define LS_LNL_LINEAR_TABLE_SIZE	(512)

#define LS_FIR0_COEFF_SIZE		(256)
#define LS_FIR1_COEFF_SIZE		(256)
#define LS_FIR2_COEFF_SIZE		(256)
#define LS_CORING_TABLE_SIZE	(256)

#define LS_LNL_TONE_CURVE_SIZE		(256)
#define LS_TONE_CTRL_CONFIG_SIZE	(4096)

#define	VIDEO_FPS_UNIT		(100)

typedef enum {
	IMG_VIDEO = 0,
	IMG_FAST_STILL,
	IMG_LOWISO_STILL,
	IMG_HIGHISO_STILL,
	IMG_MODE_NUMBER,
}image_mode;

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

typedef enum {
	COMP_STEP_SIZE_1BY3 = 0,
	COMP_STEP_SIZE_1BY2,
	COMP_STEP_SIZE_1,
}ae_compensation_step_size;

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

typedef enum{
	HDR_OP_MODE_HDR = 0,
	HDR_OP_MODE_COMBI_LINEAR,
	HDR_OP_MODE_AUTO,
	HDR_OP_MODE_NUMBER
}hdr_operation_mode_t;

typedef struct ae_info_s {
	u32	dgain_update;
	u32	shutter_update;
	u32	agc_update;
	u32	iris_update;
	u32	shutter_index;
	u32  	agc_index;
	u32	iris_index;
	u32	dgain;
}ae_info_t;

typedef enum {
	FLASH_OFF =0,
	FLASH_AUTO,
	FLASH_ON,
}FLASH_MODE;

typedef enum bayer_pattern {
	RG = 0,
	BG = 1,
	GR = 2,
	GB = 3
} bayer_pattern;

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

typedef struct awb_lut_s {
	u8		lut_no;
	awb_lut_unit_t	awb_lut[20];
} awb_lut_t;

typedef struct awb_lut_idx_s{
	u8 start;
	u8 num;
}awb_lut_idx_t;

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
	ISO_204800 =66,
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

#define	SHUTTER_TIME_TABLE_LENGTH	(2304)
#define	SHUTTER_TIME_TABLE_LENGTH_1	(SHUTTER_TIME_TABLE_LENGTH-1)
#define	SHUTTER_DGAIN_TABLE_LENGTH	(1292)//1012=>2303
#define	AGC_DGAIN_TABLE_LENGTH	(769)//0=>768
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

typedef enum{
	DEF_VIDEO_MCTF_BIN,
	DEF_MCTF_CMPR_BIN,
	DEF_LOWISO_MCTF_BIN,
	DEF_MCTF_LEVEL_BASED_BIN,
	DEF_MCTF_VLF_ADAPT_BIN,
	DEF_MCTF_L4C_EDGE_BIN,
	DEF_MCTF_MIX_124X_BIN,
	DEF_MCTF_MIX_MAIN_BIN,
	DEF_MCTF_C4_BIN,
	DEF_MCTF_C8_BIN,
	DEF_MCTF_CA_BIN,
	DEF_MCTF_VVLF_ADAPT_BIN,
	DEF_SEC_CC_L4C_EDGE_BIN,
	DEF_CC_REG_NORMAL_BIN,
	DEF_CC_3D_NORMAL_BIN,
	DEF_CC_OUT_NORMAL_BIN,
	DEF_THRESH_DARK_NORMAL_BIN,
	DEF_THRESH_HOT_NORMAL_BIN,
	DEF_FIR1_NORMAL_BIN,
	DEF_FIR2_NORMAL_BIN,
	DEF_CORING_NORMAL_BIN,
	DEF_LNL_TONE_NORMAL_BIN,
	DEF_CHROMA_SCALE_GAIN_NORMAL_BIN,
	DEF_LOCAL_EXPOSURE_GAIN_NORMAL_BIN,
	DEF_CMF_TABLE_NORMAL_BIN,
	DEF_ALPHA_LUMA_BIN,
	DEF_ALPHA_CHROMA_BIN,
	DEF_CC_REG_BIN,
	DEF_CC_3D_BIN,
	DEF_FIR1_SPAT_DEP_BIN,
	DEF_FIR2_SPAT_DEP_BIN,
	DEF_CORING_SPAT_DEP_BIN,
	DEF_FIR1_VLF_BIN,
	DEF_FIR2_VLF_BIN,
	DEF_CORING_VLF_BIN,
	DEF_FIR1_NRL_BIN,
	DEF_FIR2_NRL_BIN,
	DEF_CORING_NRL_BIN,
	DEF_FIR1_124X_BIN,
	DEF_CORING_CORING4X_BIN,
	DEF_CORING_CORING2X_BIN,
	DEF_CORING_CORING1X_BIN,
	DEF_FIR1_HIGH_NOISE_BIN,
	DEF_CORING_HIGH_NOISE_BIN,
	DEF_ALPHA_UNITY_BIN,
	DEF_ZERO_BIN,
	MF_HISO_MCTF_BIN1,
	MF_HISO_MCTF_BIN2,
	MF_HISO_MCTF_BIN3,
	MF_HISO_MCTF_BIN4,
	MF_HISO_MCTF_BIN5,
	MF_HISO_MCTF_BIN6,
	MF_HISO_MCTF_BIN7,
	MF_HISO_MCTF_BIN8
}idsp_def_bin_type;

typedef struct{
	idsp_def_bin_type type;
	u32 size;
}idsp_one_def_bin_t;

typedef struct{
	u8 num;
	idsp_one_def_bin_t one_bin[64];
}idsp_def_bin_t;

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

typedef  struct adj_black_level_control_s {
	s16	low_temperature_r;
	s16	low_temperature_g;
	s16	low_temperature_b;
	s16	high_temperature_r;
	s16	high_temperature_g;
	s16	high_temperature_b;
} adj_black_level_control_t;

typedef struct adj_awb_control_s {
	u8	low_temp_r_target;
	u8	low_temp_b_target;
	u8	d50_r_target;
	u8	d50_b_target;
	u8	high_temp_r_target;
	u8	high_temp_b_target;
} adj_awb_control_t;

typedef  struct adj_ev_color_s {
	u8	y_offset;
	u8	uv_saturation_ratio;
	u8	color_ratio;
	u8	gamma_ratio;
	u8	local_exposure_ratio;
	u8	chroma_scale_ratio;
} adj_ev_color_t;

typedef struct adj_ev_img_s {
	u8		y_offset_enable;
	u8		uv_saturation_ratio_enable;
	u8		color_ratio_enable;
	u8		gamma_ratio_enable;
	u8		local_exposure_ratio_enable;
	u8		chroma_scale_ratio_enable;
	u8		max_table_count;
	adj_ev_color_t	ev_color[20];
} adj_ev_img_t;

typedef  struct adj_noise_enable_s {
	u8	bad_pixel_enable;
	u8	chroma_median_filter_enable;
} adj_noise_enable_t;

typedef  struct adj_noise_control_s {
	u8	hot_pixel_strength;
	u8	dark_pixel_strength;
	u16	cb_mfilter_str;
	u16	cr_mfilter_str;
} adj_noise_control_t;

typedef  struct adj_cfa_filter_control_s {
	u8	dir_center_weight_red;
	u8	dir_center_weight_green;
	u8	dir_center_weight_blue;
	u16	dir_threshold_k0_red;
	u16	dir_threshold_k0_green;
	u16	dir_threshold_k0_blue;
	u8	non_dir_center_weight_red;
	u8	non_dir_center_weight_green;
	u8	non_dir_center_weight_blue;
	u16	non_dir_threshold_k0_red;
	u16	non_dir_threshold_k0_green;
	u16	non_dir_threshold_k0_blue;
	u16	non_dir_threshold_k0_close;
} adj_cfa_filter_control_t;


/////////////////////////////////////////////////////////////////

typedef struct blc_level_s {
	s32	r_offset;
	s32	gr_offset;
	s32	gb_offset;
	s32	b_offset;
}blc_level_t;

typedef struct fpn_correction_s {
	u8	enable;
	u16	pixel_map_width;
	u16	pixel_map_height;
	u16	fpn_pitch;
	u32	pixel_map_addr;
	u32	pixel_map_size;
} fpn_correction_t;

typedef struct fpn_col_gain_s {
	u8	enable;
	u32	gain_tab_addr;
	u32	offset_tab_addr;
	// Temp add for testing, should use fpn_correction_t in the end
	int	width;
	int	height;
}fpn_col_gain_t;

typedef struct dbp_correction_s {
	u8  enable; //0: disable
	            //1: hot 1st order, dark 2nd order
	            //2: hot 2nd order, dark 1st order
	            //3: hot 2nd order, dark 2nd order
	            //4: hot 1st order, dark 1st order
	u8  hot_pixel_strength;
	u8  dark_pixel_strength;
	u8  correction_method;  // 0: video, 1:still
}dbp_correction_t;

typedef struct cfa_leakage_filter_s {
	u8 enable;
	s8 alpha_rr;
	s8 alpha_rb;
	s8 alpha_br;
	s8 alpha_bb;
	u16 saturation_level;
}cfa_leakage_filter_t;

typedef struct cfa_noise_filter_info_s {
	u8  enable;
	u16 noise_level[3];        // R/G/B, 0-8192
	u16 original_blend_str[3]; // R/G/B, 0-256
	u16 extent_regular[3];     // R/G/B, 0-256
	u16 extent_fine[3];        // R/G/B, 0-256
	u16 strength_fine[3];      // R/G/B, 0-256
	u16 selectivity_regular;   // 0-256
	u16 selectivity_fine;      // 0-256
}cfa_noise_filter_info_t;



typedef struct chroma_filter_s {
	u8  enable;
	u16 noise_level[2];        // Cb/Cr, 0-255
	u16 original_blend_str[2]; // Cb/Cr, 0-256
//	u16 extent_regular[2];     // Cb/Cr, 0-256
	u16 extent_fine[2];        // Cb/Cr, 0-256
//	u16 strength_fine[2];      // Cb/Cr, 0-256
//	u16 selectivity_regular;   // 0-256
	u16 selectivity_fine;      // 0,50,100,150,200,250
	u16 radius;                // 32,64,128
}chroma_filter_t;

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
	u8 total_channel_num;
	u8 channel_idx;
	u16 reserved[30];
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
	u16 af_horizontal_filter1_thresh;
	u8 af_horizontal_filter2_mode;
	u8 af_horizontal_filter2_stage1_enb;
	u8 af_horizontal_filter2_stage2_enb;
	u8 af_horizontal_filter2_stage3_enb;
	s16 af_horizontal_filter2_gain[7];
	u16 af_horizontal_filter2_shift[4];
	u16 af_horizontal_filter2_bias_off;
	u16 af_horizontal_filter2_thresh;
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

typedef struct histogram_config_s {
	u16 mode;
	u16 hist_select;
	u16 tile_mask[8];	/* 12 * 8 tiles */
}histogram_config_t;

#define	AWB_UNIT_SHIFT (12)
typedef	struct wb_gain_s {
	u32	r_gain;
	u32	g_gain;
	u32	b_gain;
} wb_gain_t;

typedef struct digital_sat_level_s {
	u32 level_red;
	u32 level_green_even;
	u32 level_green_odd;
	u32 level_blue;
}digital_sat_level_t;

typedef struct local_exposure_s {
	u8 enable;
	u8 radius;
	u8 luma_weight_red;
	u8 luma_weight_green;
	u8 luma_weight_blue;
	u8 luma_weight_shift;
	u16 gain_curve_table[NUM_EXPOSURE_CURVE];
} local_exposure_t;

typedef struct rgb_noise_filter_s {
	u8 enable;
	u8 strength_directional_original_red;
	u8 strength_directional_original_green;
	u8 strength_directional_original_blue;
	u8 strength_directional_interpolated_red;
	u8 strength_directional_interpolated_green;
	u8 strength_directional_interpolated_blue;
	u8 strength_isotropic_original_red;
	u8 strength_isotropic_original_green;
	u8 strength_isotropic_original_blue;
	u8 strength_isotropic_interpolated_red;
	u8 strength_isotropic_interpolated_green;
	u8 strength_isotropic_interpolated_blue;
}rgb_noise_filter_t;

#define CC_3D_SIZE (17536)  //A7 CC 3D file size
#define SEC_CC_SIZE (20608) //A7 secondary CC file size
typedef struct color_correction_s {
	u32 matrix_3d_table_addr;
	u32 sec_cc_addr;
}color_correction_t;

#define CC_REG_SIZE (18752)  //A7 CC REG file size
typedef struct color_correction_reg_s {
	u32 reg_setting_addr;
}color_correction_reg_t;

#define	TONE_CURVE_SIZE	(256)
typedef struct tone_curve_s {
	u16 tone_curve_red[TONE_CURVE_SIZE];
	u16 tone_curve_green[TONE_CURVE_SIZE];
	u16 tone_curve_blue[TONE_CURVE_SIZE];
}tone_curve_t;

typedef struct rgb_to_yuv_s {
	s16	matrix_values[9];
	s16	y_offset;
	s16	u_offset;
	s16	v_offset;
}rgb_to_yuv_t;

#define NUM_CHROMA_GAIN_CURVE	(128)
typedef struct chroma_scale_filter_s {
	u16 enable; //0:disable 1:PC style VOUT 2:HDTV VOUT
	u16 gain_curve[NUM_CHROMA_GAIN_CURVE];
} chroma_scale_filter_t;

typedef struct chroma_median_filter_s {
	int	enable;
	u16	cb_str;
	u16	cr_str;
}chroma_median_filter_t;

typedef struct grid_point_s
{
	s16 x;
	s16 y;
} grid_point_t;

typedef struct grid_line_s{
	u16 grid_point_num;
	grid_point_t grid_point_pos[100];
	int coefficient[2];
	int err_variance;
	int err_straight;
}grid_line_t;

typedef struct grid_space_s{
	s16 tile_w;
	s16 tile_h;
	s16 grid_w;
	s16 grid_h;
	s16 tile_w_log2;
	s16 tile_h_log2;
}grid_space_t;

typedef struct geometry_info_s {
	u32 start_x;	// The location in sensor of output pixel (0, 0)
	u32 start_y;
	u32 width;
	u32 height;
	u32 aspect_ratio;
	u32 downsample;	// 1, 2, 4
} geometry_info_t;

#define	S3D_GEO_LEFT	1
#define	S3D_GEO_RIGHT	0
typedef struct s3d_geometry_info_s {
	u32 vin_output_sel;
	u32 prescale_dwnsmp;
	geometry_info_t geometry[2];
} s3d_geometry_info_t;

#define WARP_COORDINATE_VIRTUAL_2_CFA	0
#define WARP_COORDINATE_SSR_2_CFA		1

#define	WARP_VER_1_0	0x20100209
typedef struct warp_grid_info_s
{
	u32 version;		// WARP TABLE VERSION
	int grid_w;		// Horizontal grid point number
	int grid_h;		// Vertical grid point number
	int tile_w;		// Tile width in pixels. MUST be power of 2.
	int tile_h;		// Tile height in pixels. MUST be power of 2.
	int img_w;		// Image width
	int img_h;		// Image width
	geometry_info_t	img_geo;	// Image geometry
	u32 reserved[3];	// reserved
	grid_point_t *warp;	// Warp grid vector arrey.
} warp_grid_info_t;

typedef struct rsc_info_s
{
	int grid_w;		// Horizontal grid point number
	int grid_h;		// Vertical grid point number
	int tile_w;		// Tile width in pixels. MUST be power of 2.
	int tile_h;		// Tile height in pixels. MUST be power of 2.
	int img_w;		// Image width
	int img_h;		// Image width
	geometry_info_t	img_geo;	// Image geometry
} rsc_info_t;

typedef struct crop_info_s {
	u32 left_top_x;		// 16.16 format
	u32 left_top_y;		// 16.16 format
	u32 right_bot_x;	// 16.16 format
	u32 right_bot_y;	// 16.16 format
} crop_info_t;

typedef struct warp_control_info_s {
	// Warp table
	warp_grid_info_t* warp1;
	warp_grid_info_t* warp2;
	u32 blendRatio;
	u32 decayRatio;
	// Warp function control
	u8 ver_warp_en;
	u8 ssr_dwnsmp_h;
	u8 ssr_dwnsmp_v;
	u8 force_v4tap_disable;
	u8 tbl_upsmp_h;
	u8 tbl_dwnsmp_h;
	u8 tbl_upsmp_v;
	u8 tbl_dwnsmp_v;
	s32 hor_skew_phase_inc;
	u32 ext_ver_range;
	// Warp window info
	geometry_info_t *dummy;
	crop_info_t *tbl_crop;
	crop_info_t *ssr_crop;

	//RSC info
	rsc_info_t* rsc_info;
} warp_control_info_t;

typedef struct warp_output_info_s {
	u32 width;
	u32 height;
	u32 warp_grid_width;
	u32 warp_grid_height;
	u32 cfa_output_width;
	u32 cfa_output_height;
} warp_output_info_t;

#define	CA_VER_1_0	0x20101015
typedef struct ca_grid_info_s
{
	u32 version;		// CA TABLE VERSION
	int grid_w;		// Horizontal grid point number
	int grid_h;		// Vertical grid point number
	int tile_w;		// Tile width in pixels. MUST be power of 2.
	int tile_h;		// Tile height in pixels. MUST be power of 2.
	int img_w;		// Image width
	int img_h;		// Image width
	geometry_info_t	img_geo;	// Image geometry
	int red_scale_factor;
	int blue_scale_factor;
	u32 reserved;	// reserved
	grid_point_t *ca;	// CA grid vector arrey.
} ca_grid_info_t;

typedef struct ca_control_info_s {
	// ca table
	ca_grid_info_t* ca1;
	ca_grid_info_t* ca2;
	u32 blendRatio;
	u32 decayRatio;
	// CA function control
	u8 ver_ca_en;
	u8 reserved;
	u16 reserved1;
	u8 tbl_upsmp_h;
	u8 tbl_dwnsmp_h;
	u8 tbl_upsmp_v;
	u8 tbl_dwnsmp_v;
	// CA window info
	crop_info_t *tbl_crop;
	//crop_info_t *ssr_crop;
} ca_control_info_t;

typedef struct ca_output_info_s {
	u32 width;
	u32 height;
	u32 ca_grid_width;
	u32 ca_grid_height;
}ca_output_info_t;

typedef struct gmv_info_s {
	u8 enable;
	s16 mv_x;
	s16 mv_y;
}gmv_info_t;

typedef struct vignette_info_s {
	u8 enable;
	u8 gain_shift;
	u32 vignette_red_gain_addr;
	u32 vignette_green_even_gain_addr;
	u32 vignette_green_odd_gain_addr;
	u32 vignette_blue_gain_addr;
}vignette_info_t;

typedef struct color_noise_reduction_s {
        u8 enable;
        u8 tone_curve[TONE_CURVE_SIZE];
}color_noise_reduction_t;

typedef struct luma_high_freq_nr_s {
	u16 strength_coarse;
	u16 strength_fine;
}luma_high_freq_nr_t;

#define FIR_COEFF_SIZE	(10)
typedef struct fir_s {
	int fir_strength;
	s16 fir_coeff[FIR_COEFF_SIZE];
}fir_t;

#define CORING_TABLE_SIZE	(256)
typedef struct coring_table_s {
	u8 coring[CORING_TABLE_SIZE];
} coring_table_t;

typedef struct sharpen_level_s {
	u8 low;
	u8 low_delta;
	u8 low_strength;
	u8 mid_strength;
	u8 high;
	u8 high_delta;
	u8 high_strength;
} sharpen_level_t;
/* spatial filter is removed from A7
typedef struct spatial_filter_s {
	u8  isotropic_strength;
	u8  directional_strength;
	u16 edge_threshold; //11 bits only
	u16 max_change;
	u16 adaptation;
}spatial_filter_t;
*/
typedef struct dir_shp_s {
	u8  enable;
	u16 dir_strength[9];
	u16 iso_strength[9];
	u16 dir_amounts[9];
}dir_shp_t;

typedef struct chroma_noise_reduction_s {
	u16 chroma_strength;
	u16 chroma_radius;
	u8  chroma_adaptation;
	u8  mode;
}chroma_noise_reduction_t;

typedef struct video_mctf_one_chan_s {
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
}video_mctf_one_chan_t;

typedef struct mf_hiso_mctf_one_chan_s {
	u8  temporal_t0;
	u8  temporal_t1;
	u8  temporal_maxchange;
	u16 radius;        //0-256
	u16 str_3d;        //0-256
	u16 str_spatial;   //0-256
	u16 level_adjust;  //0-256
}mf_hiso_mctf_one_chan_t;

typedef struct video_mctf_info_s {
	video_mctf_one_chan_t chan_info[3]; //YCbCr
	u16 combined_str_y; //0-256
}video_mctf_info_t;

typedef struct mf_hiso_mctf_info_s {
	int num_of_frames;
	mf_hiso_mctf_one_chan_t chan_info[3]; //YCbCr
	u16 combined_str_y; //0-256
}mf_hiso_mctf_info_t;

typedef struct video_mctf_ghost_prv_info_s {
	u8 y;
	u8 cb;
	u8 cr;
}video_mctf_ghost_prv_info_t;

typedef struct cdnr_info_s {
	int cdnr_mode;
	int cdnr_strength;
}cdnr_info_t;

typedef struct still_size_info_s {
	u16 width_in;
	u16 height_in;
	u16 width_main;
	u16 height_main;
	u16 width_prev_A;
	u16 height_prev_A;
	u16 width_prev_B;
	u16 height_prev_B;
	u16 reserved[4]; // reserved for possible later usage
}still_size_info_t;

typedef struct max_change_s {
	u8 max_change_up;
	u8 max_change_down;
} max_change_t;

typedef struct asf_info_s {
	u16 iso_strength;			// 0-256
	u16 dir_strength;			// 0-256
	u8  directional_decide_t0;	// 0-255
	u8  directional_decide_t1;	// 0-255
	u16 adaptation;			// 0-256
	u8  max_change;			// 0-255
	sharpen_level_t level_adapt;	// 16 unit and range 0-255 for low/mid/high strength
	sharpen_level_t level_str_adjust;	// 32 unit and range 0-64 for low/mid/high strength
}asf_info_t;

typedef struct alpha_s {
	u8 alpha_plus;
	u8 alpha_minus;
	u8 smooth_adaptation;
	u8 smooth_edge_adaptation;
	u8 t0;
	u8 t1;
}alpha_t;

typedef struct cc_desat_s {
	u8 desat_str[3]; // YUV
}cc_desat_t;

/* data structure for A7 high-iso */

typedef struct reduce_cc_one_channel_s {
	u16 strength;
	u16 sat0;
	u16 sat1;
	u16 padding;
} reduce_cc_one_channel_t;

typedef struct reduced_color_correction_s {
	reduce_cc_one_channel_t reduce_cc_info[2];
}reduced_color_correction_t;

typedef struct luma_control_s {
	u16 global_strength;
}luma_control_t;

typedef struct luma_level_based_noise_reduction_s {
	u16 strength;
	sharpen_level_t level_str_adjust;
} luma_level_based_noise_reduction_t;

typedef struct luma_cntl_1_band_s {
	u8 max_change_up;
	u8 max_change_down;
	u16 noise_reduce;
	u16 sharpen_strength_up;
	u16 sharpen_strength_down;
} luma_cntl_1_band_t;

typedef struct luma_freq_seperation_s {
	luma_cntl_1_band_t luma_cntl[3]; //low, mid, high
	u32 low_band_noise_reduce_adaptation;
	u32 low_band_noise_reduce_max_change;
} luma_freq_separation_t;

typedef struct luma_very_low_freq_filter_s {
	u16 strength;
	u16 adaptation;
} luma_very_low_freq_filter_t;

typedef struct luma_detail_preservation_s {
	u32 method;
	u32 bp;
	u32 max_down;
	u32 max_up;
	u32 contrast0;
	u32 contrast1;
	u32 smooth_mul;
	sharpen_level_t level;
} luma_detail_preservation_t;

#define ALPHA_L_MASK_MAX_SIZE  (16384)
typedef struct luma_filt_spatial_dependent_s {
	u32 enable;
	u32 width;
	u32 height;
	u32 adaptation;
	u32 maxchange;
	u32 strength;
	u32 mask_addr;
}luma_filt_spatial_dependent_t;

typedef struct high_iso_chroma_filt_s {
	u32 edge_start_cb;
	u32 edge_start_cr;
	u32 edge_end_cb;
	u32 edge_end_cr;
} high_iso_chroma_filt_t;

typedef struct high_iso_color_correction_s {
	u32 reduce_cc_strength_cb;
	u32 reduce_cc_strength_cr;
	u32 reduce_cc_sat0_cb;
	u32 reduce_cc_sat0_cr;
	u32 reduce_cc_sat1_cb;
	u32 reduce_cc_sat1_cr;
} high_iso_color_correction_t;

#define ALPHA_C_MASK_MAX_SIZE  (32768)
typedef struct chroma_filter_spatial_dependent_s {
	u32 radius;
	u32 width;
	u32 height;
	u32 noise_level_cb;
	u32 noise_level_cr;
	u32 mask_addr;
} chroma_filter_spatial_dependent_t;

typedef struct chroma_filter_pre_s {
	u32 noise_level_cb;
	u32 noise_level_cr;
} chroma_filter_pre_t;

typedef struct chroma_filter_high_s {
	u32 noise_level_cb;
	u32 noise_level_cr;
} chroma_filter_high_t;

typedef struct chroma_filter_med_s {
	u32 noise_level_cb;
	u32 noise_level_cr;
	u32 adaptation_cb;
	u32 adaptation_cr;
	u32 maxchange_cb;
	u32 maxchange_cr;
} chroma_filter_med_t;

typedef struct chroma_filter_low_s {
	u32 noise_level_cb;
	u32 noise_level_cr;
	u32 adaptation_cb;
	u32 adaptation_cr;
	u32 maxchange_cb;
	u32 maxchange_cr;
} chroma_filter_low_t;

typedef struct chroma_filter_very_low_s {
	u32 noise_level_cb;
	u32 noise_level_cr;
	u32 adaptation_cb;
	u32 adaptation_cr;
	u32 maxchange_cb;
	u32 maxchange_cr;
} chroma_filter_very_low_t;

typedef struct chroma_filter_very_very_low_s {
	u32 noise_level_cb;
	u32 noise_level_cr;
	u32 adaptation_cb;
	u32 adaptation_cr;
	u32 maxchange_cb;
	u32 maxchange_cr;
} chroma_filter_very_very_low_t;

typedef struct wide_median_s {
	u32 adaptation_cb;
	u32 adaptation_cr;
	u32 maxchange_cb;
	u32 maxchange_cr;
} wide_median_t;

typedef struct chroma_level_based_noise_reduction_s {
	u8 cb_start;
	u8 cb_end;
	u8 cr_start;
	u8 cr_end;
}chroma_level_based_noise_reduction_t;

typedef struct warp_correction_s{
	u32 warp_control;
	u32 warp_horizontal_table_address;
	u32 warp_vertical_table_address;
	u32 actual_left_top_x;
	u32 actual_left_top_y;
	u32 actual_right_bot_x;
	u32 actual_right_bot_y;
	u32 zoom_x;
	u32 zoom_y;
	u32 x_center_offset;
	u32 y_center_offset;
	u8  grid_array_width;
	u8  grid_array_height;
	u8  horz_grid_spacing_exponent;
	u8  vert_grid_spacing_exponent;
	u8  vert_warp_enable;
	u8  vert_warp_grid_array_width;
	u8  vert_warp_grid_array_height;
	u8  vert_warp_horz_grid_spacing_exponent;
	u8  vert_warp_vert_grid_spacing_exponent;
	u8  force_v4tap_disable;
	u16 reserved_2;
	int hor_skew_phase_inc;
	u16 dummy_window_x_left;
	u16 dummy_window_y_top;
	u16 dummy_window_width;
	u16 dummy_window_height;
	u16 cfa_output_width;
	u16 cfa_output_height;
	u32 extra_sec2_vert_out_vid_mode;
}warp_correction_t;

#define WARP_MAX_WIDTH  (32)
#define WARP_MAX_HEIGHT (48)
#define WARP_MAX_SIZE (WARP_MAX_WIDTH*WARP_MAX_HEIGHT)
typedef struct warp_calib_map_s{
	u8  tile_size_h; // 0 for 64 and 1 for 128
	u8  tile_size_v; // 0 for 64 and 1 for 128
	u8  num_tiles_horizontal;
	u8  num_tiles_vertical;
	grid_point_t warp_vector[WARP_MAX_SIZE]; // WARP vectors
}warp_calib_map_t;

typedef struct chroma_aberration_s {
	u8  enable;
	u16 horz_warp_enable;
	u16 vert_warp_enable;

	u8  horz_pass_grid_array_width;
	u8  horz_pass_grid_array_height;
	u8  horz_pass_horz_grid_spacing_exponent;
	u8  horz_pass_vert_grid_spacing_exponent;
	u8  vert_pass_grid_array_width;
	u8  vert_pass_grid_array_height;
	u8  vert_pass_horz_grid_spacing_exponent;
	u8  vert_pass_vert_grid_spacing_exponent;

	u16 red_scale_factor;
	u16 blue_scale_factor;
	u32 warp_horizontal_table_address;
	u32 warp_vertical_table_address;
}chroma_aberration_t;

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

typedef struct awb_gain_s {
	u16		video_gain_update;
	u16		still_gain_update;
	wb_gain_t	video_wb_gain;
	wb_gain_t	still_wb_gain;
}awb_gain_t;

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

typedef struct img_aaa_stat_s {
	aaa_tile_info_t 	tile_info;
	awb_data_t		awb_info[MAX_AWB_TILE_NUM];
	ae_data_t 		ae_info[MAX_AE_TILE_NUM];
	af_stat_t			af_info[MAX_AF_TILE_NUM];
	cfa_histogram_stat_t	cfa_hist;
	rgb_histogram_stat_t	rgb_hist;
	embed_hist_stat_t 		sensor_hist_info;
}img_aaa_stat_t;

typedef struct retain_max_fir_s {
	u8				retain_level;
	max_change_t		max_change;
	fir_t			fir;
} retain_max_fir_t;
/*
typedef struct def_sharp_info_s {
	u8		max_table_count;
	u8                        high_freq_noise_enable;
	u8				high_freq_noise_reduc[15];

	u8                              spatial_enable;
	spatial_filter_t		spatial_filter[15];

	u8                              sharpen_enable;
	level_t			level_min[15];
	level_t			level_overall[15];
	retain_max_fir_t		rmf[15];
	coring_a5s_t		coring[15];
} def_sharp_info_t;

typedef struct adj_def_s {
	u8			max_table_count;
	color_3d_t		color;
	u8			black_level_enable;
	adj_black_level_control_t	black_level[15];
	adj_noise_enable_t              noise_enable;
	adj_noise_control_t	noise_table[15];
	u8                              	cfa_filter_enable;
	adj_cfa_filter_control_t	cfa_table[15];

	tone_curve_t	ratio_255_gamma;
	tone_curve_t	ratio_0_gamma;

} adj_def_t;
*/

typedef struct zoom_factor_info_s {
	u32	zoom_x;
	u32	zoom_y;
	u32	x_center_offset;
	u32	y_center_offset;
}zoom_factor_info_t;


typedef struct video_mctf_ext_info_s {
	u8 enable;
	u8 mctf_mode;	//0: modeA 1: modeB
	u8 alpha[4];
	u8 threshold_1[4];
	u8 threshold_2[4];
	u8 edge[4];   //0-63, 6 bits
	u8 y_max_change;
	u8 u_max_change;
	u8 v_max_change;
}video_mctf_ext_info_t;

typedef struct gbgr_mismatch_s {
	u8  narrow_enable;
	u8  wide_enable;
	u16 wide_safety;
	u16 wide_thresh;
}gbgr_mismatch_t;

#define	NUM_BAD_PIXEL_THD	(384)

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
	IMX_123,
	IMX_172,
	IMX_178,
	IMX_104,
	IMX_136,
	IMX_185,
	IMX_183,
	IMX_226,
	IMX_224,
	OV_5658,
	OV_4689,
}sensor_label;

typedef struct img_adj_param_s {
	blc_level_t	blc[3*6];//(FA+D50+D75)*6
	u16		ae_target[6];
	rgb_to_yuv_t	rgb2yuv_matrix;
	tone_curve_t	tone_curve;
}img_adj_param_t;

typedef struct still_cap_info_s {
	u8 qp;
	u8 capture_num;
	u8 vsync_skip;
	u16 jpeg_w;
	u16 jpeg_h;
	u16 preview_w_A;
	u16 preview_h_A;
	u16 preview_w_B;
	u16 preview_h_B;
	u16 thumbnail_w;
	u16 thumbnail_h;
	u16 thumbnail_dram_w;
	u16 thumbnail_dram_h;
	u8 sensor_resolution;
	u8 sensor_pattern;
	u8 preview_control;
	u8 hdr_processing;
	u16 screen_thumbnail_active_w;
	u16 screen_thumbnail_active_h;
	u16 screen_thumbnail_dram_w;
	u16 screen_thumbnail_dram_h;
	u8 need_raw;
	u8 bayer_pattern;
	u8 capture_mode;
	u32 hiLowISO_proc_cfg_ptr;
}still_cap_info_t;

typedef struct still_proc_mem_init_info_s{
	u32 width;
	u32 height;
	u8* __user_raw_addr;
}still_proc_mem_init_info_t;

typedef struct still_proc_mem_info_s{
	u32 width;
	u32 height;
}still_proc_mem_info_t;

/********************************************************
*	Auto-Focus related data structs							*
*********************************************************/

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

#define MAX_PIRIS_TABLE_SIZE	IRIS_FNO_TABLE_LENGTH

typedef enum{
    PIRIS_DGAIN_TABLE = 0x0,
    PIRIS_FNO_SCOPE = 0X1,
    PIRIS_STEP_TABLE = 0x2,
    PIRIS_ID_NUM,
}lens_piris_id;

typedef struct lens_piris_header_s{
    int lens_piris_id;
    int array_size;
}lens_piris_header_t;

typedef struct lens_piris_dgain_table_s{
    lens_piris_header_t header;
    u16 table[MAX_PIRIS_TABLE_SIZE];
}lens_piris_dgain_table_t;

typedef struct lens_piris_fno_scope_s{
    lens_piris_header_t header;
    int table[2];
    u32 reserved[4];
}lens_piris_fno_scope_t;

typedef struct lens_piris_step_table_s{
    lens_piris_header_t header;
    u16 table[MAX_PIRIS_TABLE_SIZE];
}lens_piris_step_table_t;

typedef struct piris_param_s{
	const int *scope;
	const u16 *table;
	int tbl_size;
	const lens_piris_dgain_table_t *dgain;
	//add here
}piris_param_t;

typedef struct lens_param_s{
	piris_param_t piris_std;
	//add here
}lens_param_t;

typedef enum{
	NORMAL_RUN = 0x0,		//default
	PIRIS_CALI = 0x1,		//bit0
	BACK_FOCUS_CALI = 0x2,	//bit1
	//add here
}lens_cali_mode;

typedef struct piris_cali_s{
	u16 step_size;
	//add here
}piris_cali_t;

typedef struct lens_cali_s{
	lens_cali_mode act_mode;
	piris_cali_t piris_cfg;
	//add here
}lens_cali_t;

typedef enum{
	CAF = 0,
	SAF = 1,
	MANUAL = 3,
	CALIB = 4,
	DEBUG = 5,
}af_mode;

typedef struct af_range_s{
	s32 far_bd;
	s32 near_bd;
}af_range_t;

typedef struct awb_weight_window_s{
	u32	tiles[384];
}awb_weight_window_t;

typedef struct af_ROI_config_s{
	u32	tiles[40];
}af_ROI_config_t;

typedef struct af_control_s{
	af_mode workingMode;
	af_ROI_config_t af_window;
	u8 zoom_idx;
	s32 focus_idx; //only avilable in MANUAL mode
}af_control_t;

typedef struct lens_control_s{ // for algo internal use
	u8 focus_update;
	u8 zoom_update;
	s32 focus_pulse;
	s32 zoom_pulse;
	s16 pps;
	af_statistics_ex_t af_stat_config;
	u8 af_stat_config_update;
}lens_control_t;

typedef struct aaa_api_s{
	int (*p_ae_control_init)(line_t* line, u16 line_num, u16 max_sensor_gain, u16 double_inc, u32* agc_dgain,u32* sht_dgain, u8* dlight_range);
	int (*p_ae_control)(ae_data_t *p_tile_info, u16 tile_num, ae_info_t *p_video_ae_info, ae_info_t *p_still_ae_info);
	int (*p_awb_control_init)(wb_gain_t * menu_gain, awb_lut_t * lut_table, awb_lut_idx_t * lut_table_idx);
	void (*p_awb_control)(u16 tile_count, awb_data_t * p_tile_info, awb_gain_t * p_awb_gain);
	int (*p_af_control_init)(af_control_t* p_af_control, void* p_af_param, void* p_zoom_map, lens_control_t* p_lens_ctrl);
	int (*p_af_control)(af_control_t* p_af_control, u16 af_tile_count, af_stat_t* p_af_info, u16 awb_tile_count, awb_data_t* p_awb_info, lens_control_t * p_lens_ctrl, ae_info_t* p_ae_info, u8 lens_runing_flag);
	void (*p_af_set_range)(af_range_t* p_af_range);
	void (*p_af_set_calib_param)(void* p_calib_param);
}aaa_api_t;

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
	int (*lens_cali)(lens_cali_t *p_lens_cali);
	int (*load_param)(lens_param_t *p_lens_param);
	int (*get_state)(int *lens_state);
} lens_dev_drv_t;
/********************************************************
*	End of Auto-Focus related data structs						*
*********************************************************/
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
	awb_lut_t	wr_table;
	awb_lut_idx_t awb_lut_idx[WB_ENV_NUMBER];
}img_awb_param_t;

/*******adjust***********/
typedef struct adj_color_control_ev_s{
	u16 ev_thre_low;
	u16 ev_thre_hi;
}adj_color_control_ev_t;

typedef struct adj_color_control_s {
	u16 r_gain;
	u16 b_gain;
	u32 matrix_3d_table_addr;
	s16 uv_matrix[4];
	s16 cc_matrix[9];
} adj_color_control_t;

typedef struct adj_lut_s {
	s16		value[32];
} adj_lut_t;

typedef struct color_3d_s {
	u8			type;
	u8			control;
	u8			table_count;
	adj_color_control_t		table[5];
	adj_color_control_ev_t	cc_interpo_ev;
} color_3d_t;

typedef struct adj_ae_awb_s {
	u8				max_table_count;
	adj_lut_t			table[25];
} adj_awb_ae_t;

typedef struct mctf_info_s {
	u8		max_table_count;
	u8		enable;
	adj_lut_t		mctf1[15];
	adj_lut_t		mctf2[15];
} mctf_info_t;

typedef struct adj_filter_info_s {
	u8			table_count;
	adj_lut_t		enable;
	adj_lut_t		table[25];
} adj_filter_info_t;

typedef struct adj_def_s {
	u8				max_table_count;
	color_3d_t		color;
	u8				black_level_enable;
	adj_lut_t			black_level[25];
	adj_lut_t			noise_enable;
	adj_lut_t			noise_table[25];
	u8				cfa_filter_enable;
	adj_lut_t			low_cfa_table[25];
	adj_lut_t			high_cfa_table[25];
	u8				chroma_noise_filter_enable;
	adj_lut_t			chroma_noise_filter_table[25];
	tone_curve_t              ratio_255_gamma;
	tone_curve_t              ratio_0_gamma;
	u16 ratio_255_local_exposure_table[NUM_EXPOSURE_CURVE];
	u16 ratio_0_local_exposure_table[NUM_EXPOSURE_CURVE];
} adj_def_t;

typedef struct def_sharp_info_s {
	u8				max_table_count;
	u8				cdnr_enable;
	adj_lut_t			cdnr_filter[25];
	u8				spatial_enable;
	adj_lut_t			spatial_filter1[25];
	adj_lut_t			spatial_filter2[25];
	adj_lut_t			spatial_filter_level_adapt[25];
	adj_lut_t			spatial_filter_level_str_adjust[25];
	u8				sharpen_a_enable;
	adj_lut_t			sharpen_a_level_min[25];
	adj_lut_t			sharpen_a_level_overall[25];
	adj_lut_t			sharpen_a_rmf[25];
	coring_table_t		sharpen_a_coring[25];
	u8				sharpen_b_enable;
	adj_lut_t			sharpen_b_level_min[25];
	adj_lut_t			sharpen_b_level_overall[25];
	adj_lut_t			sharpen_b_rmf[25];
	coring_table_t		sharpen_b_coring[25];
} def_sharp_info_t;

typedef struct str_info_s {
	s16             str;
	u16             offset;
	u16             adapt;
} str_info_t;

#define STR_MAX_VALUE		19
typedef struct str_lut_s {
	str_info_t	info[20];
} str_lut_t;

typedef struct adj_str_control_s {
	u8		table_count;
	u8		enable;
	s16		start;
	s16		end;
	str_lut_t 	lut[12];
}adj_str_control_t;

typedef struct adj_param_s {
	adj_awb_ae_t		awbae;
	adj_filter_info_t	ev_img;
	adj_def_t			def;
	mctf_info_t		mctf_info;
	def_sharp_info_t	sharp_info;
//	adj_str_control_t	dzoom_control;
} adj_param_t;

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

typedef struct pipeline_control_s {
	u8				black_corr_update;
	blc_level_t		black_corr;

	u8				rgb_yuv_matrix_update;
	rgb_to_yuv_t		rgb_yuv_matrix;

	u8				gamma_update;
	tone_curve_t		gamma_table;

	u8				local_exposure_update;
	local_exposure_t	local_exposure;

	u8					chroma_scale_update;
	chroma_scale_filter_t	chroma_scale;

	u8				anti_aliasing_update;
	u8				anti_aliasing;

	u8						chroma_median_filter_update;
	chroma_median_filter_t		chroma_median_filter;

	u8				chroma_noise_filter_update;
	chroma_filter_t	chroma_noise_filter;

	u8				high_freq_noise_reduc_update;
	luma_high_freq_nr_t high_freq_noise_strength;

	u8				cc_matrix_update;
	color_correction_t	color_corr;

	u8				badpix_corr_update;
	dbp_correction_t	badpix_corr_strength;

	u8					cfa_filter_update;
	cfa_noise_filter_info_t	cfa_filter;

	u8				mctf_update;
	video_mctf_info_t	mctf_param;

	u8				asf_select_update;
	u8				asf_select;

	u8				cdnr_filter_update;
	cdnr_info_t		cdnr_filter;

	u8				spatial_filter_update;
	asf_info_t		spatial_filter;

	u8				sharp_a_coring_update;
	coring_table_t		sharp_a_coring;

	u8				sharp_a_retain_update;
	u8				sharp_a_retain;

	u8				sharp_a_max_change_update;
	max_change_t	sharp_a_max_change;

	u8				sharp_a_fir_update;
	fir_t				sharp_a_fir;

	u8				sharp_a_level_minimum_update;
	sharpen_level_t	sharp_a_level_minimum;

	u8				sharp_a_level_overall_update;
	sharpen_level_t	sharp_a_level_overall;

	u8				sharp_b_coring_update;
	coring_table_t		sharp_b_coring;

	u8				sharp_b_retain_update;
	u8				sharp_b_retain;

	u8				sharp_b_max_change_update;
	max_change_t	sharp_b_max_change;

	u8				sharp_b_fir_update;
	fir_t				sharp_b_fir;

	u8				sharp_b_level_minimum_update;
	sharpen_level_t	sharp_b_level_minimum;

	u8				sharp_b_level_overall_update;
	sharpen_level_t	sharp_b_level_overall;

	u8				sharp_b_linearization_update;
	u16				sharp_b_linearization_strength;
} pipeline_control_t;

/*******adjust***********/

typedef struct aaa_cntl_s {
	u8	awb_enable;
	u8	ae_enable;
	u8	af_enable;
	u8	adj_enable;
}aaa_cntl_t;

typedef   struct image_property_s {
	int	brightness;
	int 	contrast;
	int 	saturation;
	int 	hue;
	int	sharpness;
	int	mctf;
	int	local_exp;
}  image_property_t;

typedef struct image_sensor_param_s{
	adj_param_t *		p_adj_param;
	adj_param_t *		p_adj_param_ir;
	rgb_to_yuv_t *		p_rgb2yuv;
	chroma_scale_filter_t *	p_chroma_scale;
	img_awb_param_t *	p_awb_param;
	line_t *				p_50hz_lines;
	line_t *				p_60hz_lines;
	statistics_config_t *	p_tile_config;
	u32* 				p_ae_agc_dgain;
	u32* 				p_ae_sht_dgain;
	u32*				p_shutter_table;
	u8*					p_dlight_range;
	local_exposure_t*		p_manual_LE;
}image_sensor_param_t;

typedef struct hdr_sensor_param_s{
	u16 *	p_ae_target;
	line_t **	p_sht_lines;
}hdr_sensor_param_t;

typedef struct gamma_curve_info_s {
	u16 tone_curve_red[256];
	u16 tone_curve_green[256];
	u16 tone_curve_blue[256];
}gamma_curve_info_t;

typedef int (*dc_iris_cntl)(int, int, int*);
typedef u32 (*luma_stat_calc)(void*, u16, img_aaa_stat_t*);

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


typedef struct point_pos_s{
	int x;
	int y;
}point_pos_t;

////////////////////////////////////////////////////////////
// HDR S2
#define	MIN_HDR_EXPOSURE_NUM		1
#define	MAX_HDR_EXPOSURE_NUM		4

typedef struct hdr_mixer_config_s {
	u32 mixer_mode;
	u8 radius;
	u8 luma_weight_red;
	u8 luma_weight_green;
	u8 luma_weight_blue;
	u16 threshold;
	u8 thresh_delta;
	s8 long_exposure_shift;
}hdr_mixer_config_t;

typedef struct le_curve_config_s{
	u16 strength;
	u16 width;
}le_curve_config_t;

#define OUT_LUT_TABLE_SIZE	256

typedef struct out_lut_offset_s{
	s16	entry_offset_t0;	// -255 ~ 255
	s16	entry_offset_t1;	// -255 ~ 255
	s16	ratio_offset_t0;	// -1023 ~ 1023
	s16	ratio_offset_t1;	// -1023 ~ 1023
}out_lut_offset_t;

typedef struct out_lut_level_s{
	u8	luma_entry_t0;	// 0 ~ 255
	u8	luma_entry_t1;	// 0 ~ 255
	u16	blend_ratio_t0;	// 0 ~ 1023
	u16	blend_ratio_t1;	// 0 ~ 1023
}out_lut_level_t;

typedef struct out_lut_table_s{
	u16 table[OUT_LUT_TABLE_SIZE];
}out_lut_table_t;

typedef struct alpha_map_config_s{
	u8	mode;						// 0: level; 1: table
	out_lut_level_t		out_lut_level;
	out_lut_table_t	out_lut_table;
}alpha_map_config_t;

typedef struct yuv_contrast_config_s{
	u8 enable;
	u8 low_pass_radius;
	u16 contrast_enhance_gain;	// 2.10 format
}yuv_contrast_config_t;

typedef struct video_proc_config_s{
	u32 reserved0[MAX_HDR_EXPOSURE_NUM];
	u32 reserved1[MAX_HDR_EXPOSURE_NUM];
}video_proc_config_t;

typedef struct raw_offset_config_s{
	u32 x_offset[MAX_HDR_EXPOSURE_NUM];
	u32 y_offset[MAX_HDR_EXPOSURE_NUM];
}raw_offset_config_t;

typedef struct hdr_proc_data_s {
	u8					expo_id;

	u8					blc_update[MAX_HDR_EXPOSURE_NUM];
	blc_level_t			blc_info[MAX_HDR_EXPOSURE_NUM];

	u8					dbp_update[MAX_HDR_EXPOSURE_NUM];
	dbp_correction_t		dbp_info[MAX_HDR_EXPOSURE_NUM];

	u8					anti_aliasing_update[MAX_HDR_EXPOSURE_NUM];
	u8					anti_aliasing_strength[MAX_HDR_EXPOSURE_NUM];

	u8					le_update[MAX_HDR_EXPOSURE_NUM];
	local_exposure_t		le_info[MAX_HDR_EXPOSURE_NUM];
	le_curve_config_t		le_config[MAX_HDR_EXPOSURE_NUM];

	u8					cfa_update[MAX_HDR_EXPOSURE_NUM];
	cfa_noise_filter_info_t	cfa_info[MAX_HDR_EXPOSURE_NUM];

	u8					wb_update[MAX_HDR_EXPOSURE_NUM];
	wb_gain_t			wb_gain[MAX_HDR_EXPOSURE_NUM];
	u8					dgain_update[MAX_HDR_EXPOSURE_NUM];
	u32					dgain[MAX_HDR_EXPOSURE_NUM];

	u8					alpha_map_update[MAX_HDR_EXPOSURE_NUM];
	alpha_map_config_t	alpha_map_info[MAX_HDR_EXPOSURE_NUM];

	u8					cc_update[MAX_HDR_EXPOSURE_NUM];
	color_correction_t		cc_info[MAX_HDR_EXPOSURE_NUM];
	tone_curve_t			tone_info[MAX_HDR_EXPOSURE_NUM];

	u8					shutter_update[MAX_HDR_EXPOSURE_NUM];
	int					shutter_row[MAX_HDR_EXPOSURE_NUM];

	u8					agc_update[MAX_HDR_EXPOSURE_NUM];
	int					agc_idx[MAX_HDR_EXPOSURE_NUM];

	u8					yuv_contrast_update;
	yuv_contrast_config_t	yuv_contrast_info;

	u8					raw_offset_update;
	raw_offset_config_t	raw_offset;

	video_proc_config_t	video_proc;
}hdr_proc_data_t;

typedef struct hdr_sensor_cfg_s{
	u16 chs_cfg;		// 0: img algo will assign one default setting; 1: config by customer
	sensor_label label;
	bayer_pattern pat;
	u16 step;
	u16 max_g_db;
	u16 max_ag_db;
	u16 max_dg_db;
	u16 max_g_idx;
	u16 max_ag_idx;
	u16 max_dg_idx;
	u16 s_delay;		// frame number delay for shutter setting compared with DSP timing
	u16 g_delay;		// frame number delay for gain setting compared with DSP timing
}hdr_sensor_cfg_t;

typedef struct hdr_shutter_gp_s{
	u32 shutter_info[MAX_HDR_EXPOSURE_NUM];
}hdr_shutter_gp_t;

typedef struct hdr_gain_gp_s{
	u32 gain_info[MAX_HDR_EXPOSURE_NUM];
}hdr_gain_gp_t;

typedef enum {
	NOT_HDR = 0,
	FRAME_HDR,
	LINE_HDR,
}img_work_mode;

typedef struct img_config_info_s{
	img_work_mode mode;
	u8 expo_num;
	u8 sharpen_b_enable;
	u8 defblc_enable;
	u8 defog_pipe_enable;
	u16 fps;
}img_config_info_t;

typedef struct hdr_cntl_fp_s{
	int (*p_ae_init)(line_t* p_line, u16 line_num, u16 luma_target, u32* p_gain_tbl, u32 gain_tbl_size, u32 dbl_step, u8 expo_id);
	int (*p_ae_cntl)(ae_info_t *p_video_ae, u16 tile_num, img_aaa_stat_t *p_aaa_stat, u8 expo_id);

	int (*p_awb_init)(wb_gain_t *p_menu_gain, awb_lut_t *p_lut_table, awb_lut_idx_t *p_lut_idx, u8 expo_id);
	int (*p_awb_cntl)(awb_gain_t *p_awb_gain, u16 tile_num, awb_data_t *p_awb_info, u8 expo_id);

	int (*p_af_init)(af_control_t* p_af_control, void* p_af_param, void* p_zoom_map, lens_control_t* p_lens_ctrl);
	int (*p_af_cntl)(af_control_t* p_af_control, u16 af_tile_count, af_stat_t* p_af_info, u16 awb_tile_count, awb_data_t* p_awb_info, lens_control_t * p_lens_ctrl, ae_info_t* p_ae_info, u8 lens_runing_flag);
	void (*p_af_set_range)(af_range_t* p_af_range);
	void (*p_af_set_calib_param)(void* p_calib_param);

	int (*p_alp_bld_cntl)(hdr_proc_data_t *p_hdr_proc_pipe, img_aaa_stat_t *p_hdr_stat);
}hdr_cntl_fp_t;

/////////////////////////////////////////////////////////////////

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
	blc_level_t blc;
/*output*/
	u16* r_tab;
	u16* ge_tab;
	u16* go_tab;
	u16* b_tab;
} vignette_cal_t;

typedef struct cali_gyro_setup_s{ //gyro_cal_info_s
	u8 mode;
#define GYRO_FUNC_CALIB	(0)
#define GYRO_BIAS_CALIB		(1)
	u32 time; //how many seconds to do calibration
#define SAMPLE_NUM_MAX (10240) // 10s, 1 sample/ms
	u32 freq; //the freq of shaker used
	u32 amp; // the full-range angle of shaker and multiply 10
	u16 buf_size;
	u16* buf_x;
	u16* buf_y;
	u8 sampling_rate;
}cali_gyro_setup_t;//gyro_cal_info_t;

typedef struct gyro_calib_info_s {
	u32 mean_x;
	u32 mean_y;
	u32 sense_x;
	u32 sense_y;
} gyro_calib_info_t;

typedef struct warp_cal_info_s {
	//Raw data info
	u16 *raw_addr;
	u16 width;
	u16 height;
	u8  bayer;
	//Calibration configurations
	u8 mode;// 0 for express mode, 1 for slow mode.
	u8  luma_avg_tile_size; // 0-2. default is 2.
	u8  tile_size_x;      // 0 for 64 tile, 1 for 128 tile
	u8  tile_size_y;      // 0 for 64 tile, 1 for 128 tile
	u8  save_file_flg;    // save warp calibration debug dump
	u16 threshold_x;      // if (max warp vector > thres) => NG
	u16 threshold_y;      // if (max warp vector > thres) => NG
	//Output address
	warp_calib_map_t *storage;
} warp_cal_info_t;

typedef enum {
	IMG_COLOR_TV = 0,
	IMG_COLOR_PC,
	IMG_COLOR_STILL,
	IMG_COLOR_CUSTOM,
	IMG_COLOR_NUMBER,
}img_color_style;

typedef struct still_hdr_proc_s{
	u32 height;
	u32 width;
	u16* short_exp_raw;
	u16* long_exp_raw;
	u32* raw_buff;
	u32 short_shutter_idx;
	u32 long_shutter_idx;
	u32 r_gain;
	u32 b_gain;
}still_hdr_proc_t;

typedef struct aaa_tile_report_s {
	u16 awb_tile;
	u16 ae_tile;
	u16 af_tile;
}aaa_tile_report_t;

typedef struct low_iso_addr_s{
	u32 cc_reg_user;           // 2304 bytes
	u32 cc_3d_user;            // 16384 bytes
	u32 cc_out_user;           // 1024 bytes
	u32 dark_threshold_user;   // 768 bytes
	u32 hot_threshold_user;    // 768 bytes
	u32 chroma_scale_user;     // 256 bytes
	u32 fir1_user;             // 256 bytes
	u32 fir2_user;             // 256 bytes
	u32 coring_user;           // 256 bytes
	u32 alpha_user;            // 512 bytes
	u32 local_exposure_user;   // 512 bytes
	u32 chroma_median_K_user;  // 48 bytes
	u32 vignette_user;         // 2178*4 bytes
}low_iso_addr_t;

typedef struct high_iso_addr_s{
	u32 cc_reg_user;           // 2304 bytes
	u32 cc_3d_user;            // 16384 bytes
	u32 cc_out_user;           // 1024 bytes
	u32 dark_threshold_user;   // 768 bytes
	u32 hot_threshold_user;    // 768 bytes
	u32 fir1_user;             // 256 bytes
	u32 fir2_user;             // 256 bytes
	u32 coring_user;           // 256 bytes
	u32 lnl_tone_curve_user;   // 256 bytes
	u32 chroma_scale_user;     // 256 bytes
	u32 local_exposure_user;   // 512 bytes
	u32 chroma_median_K_user;  // 48 bytes
}high_iso_addr_t;

typedef struct img_snapshot_config_s {
	u8 cap_mode; //0- fast, 1- high iso , 2 - low iso, 4 auto
	u8 cap_burst_num;
//	u8 art_effect;
	u8 save_raw;
	u8 flash_cntl;
	u8 hdr_cntl;
	u16 vidcap_w;
	u16 vidcap_h;
	u16 main_w;
	u16 main_h;
	u16 encode_w;
	u16 encode_h;
	u16 preview_w_A;
	u16 preview_h_A;
	u16 preview_w_B;
	u16 preview_h_B;
	u16 thumbnail_w;
	u16 thumbnail_h;
	u16 thumbnail_dram_w;
	u16 thumbnail_dram_h;
	u8 qp;
}img_snapshot_config_t;

typedef struct img_lib_version_s {
	int	major;
	int	minor;
	int	patch;
	u32	mod_time;
	char	description[64];
}  img_lib_version_t;

typedef struct sensor_config_s{
	bayer_pattern pattern;//BG//GB//GR//RG
	u16 max_gain_db;//<60
	u16 gain_step;//16,20,64...
	u8 shutter_delay;//1,2,3
	u8 agc_delay;//1,2
	sensor_label sensor_lb;
}sensor_config_t;
typedef struct mctf_property_s{
	int mctf_alpha_ratio;
	int mctf_threshold_ratio;
	int mctf_max_change_ratio;
	int mctf_3d_strength_ratio;
}mctf_property_t;

typedef struct sharpen_property_s{
	int cfa_noise_level_ratio;
	int cfa_extent_regular_ratio;
	int sharpen_fir_ratio;
	int sharpen_linear_ratio;
	int sharpen_max_change_ratio;
	int sharpen_overall_ratio;
	int asf_adaptation_ratio;
	int asf_level_adj_str_ratio;
}sharpen_property_t;

#define	VIN0_VSYNC		"/proc/ambarella/vin0_vsync"

#endif


