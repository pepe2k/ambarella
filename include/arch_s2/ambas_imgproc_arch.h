#ifndef __AMBAS_IMGPROC_ARCH_H__
#define __AMBAS_IMGPROC_ARCH_H__

#define	RGB_AAA_DATA_BLOCK			(5120)
#define	CFA_AAA_DATA_BLOCK			(15872)
#define	RGB_AAA_DATA_BLOCK_SIZE		(RGB_AAA_DATA_BLOCK*(4))
#define	CFA_AAA_DATA_BLOCK_SIZE		(CFA_AAA_DATA_BLOCK*(4))
#define 	SENSOR_HIST_DATA_BLOCK		(2080*2*4)
#define	INPUT_LOOK_UP_TABLE_SIZE		(192*3*4)
#define	MATRIX_TABLE_SIZE			(16*16*16*4)

/*input lookup tabel for color correction 192*3 */
#define	NUM_IN_LOOKUP				(192*3)
#define	SIZE_IN  	(NUM_IN_LOOKUP*sizeof(u32))	/* 2112 bytes */	// NUM_IN_LOOKUP @ iav/iav_struct.h
#define	SIZE_3D  	(NUM_MATRIX*sizeof(u32))	/* 16384 bytes */	// NUM_MATRIX @ iav/iav_struct.h
#define	SIZE_OUT 	(NUM_OUT_LOOKUP*sizeof(u32))	/* 1024 bytes */	// NUM_OUT_LOOKUP @ iav/iav_struct.h
#define	SIZE_TONE_CURVE	(256)
#define LS_THREE_D_TABLE_SIZE (4096)


/*matrix tabel for color correction 16*16*16 */
#define	NUM_MATRIX				(4096)
#define	NUM_OUT_LOOKUP			(256)
#define	NUM_CHROMA_GAIN_CURVE		(128)
#define	RGB_TO_YUV_MATRIX_SIZE		(9)
#define	K0123_ARRAY_SIZE			(24)
#define	MCTF_CFG_SIZE				(27600)
#define	CC_CFG_SIZE 				(20608)

#define	NUM_BAD_PIXEL_THD			(384)


#define	MAX_TILES_H				(33)
#define	MAX_TILES_V				(33)
#define	MAX_VIGNETTE_NUM			(MAX_TILES_H * MAX_TILES_V * (2))
#define	DYN_BPC_THD_TABLE_SIZE		((3)*(128)*(2))

struct aaa_tile_info {
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
	u8  total_exposure;
	u8  exposure_index;
	u8 total_channel_num;
	u8 channel_idx;
	u16 reserved[30];
};

struct af_stat {
	u16 sum_fy;
	u16 sum_fv1;
	u16 sum_fv2;
};

struct float_rgb_af_stat{
	u32 sum_fv1_h;
	u32 sum_fv2_h;
	u32 sum_fv1_v;
	u32 sum_fv2_v;
};

struct rgb_histogram_stat {
	u32 his_bin_y[64];
	u32 his_bin_r[64];
	u32 his_bin_g[64];
	u32 his_bin_b[64];
};

struct rgb_aaa_stat {
	struct aaa_tile_info	aaa_tile_info;
	u16			 frame_id;
	struct af_stat		 af_stat[256];
	u16		 	 ae_sum_y[96];
	struct rgb_histogram_stat histogram_stat;
	struct float_rgb_af_stat	 float_rgb_af_stat[32];
	u32			 float_ae_sum_y[32];
	u8		 	 reserved[62];
	u8			 reserved_add[1532];
};

struct cfa_ae_stat{
	u16 lin_y;
	u16 count_min;
	u16 count_max;
};

struct cfa_awb_stat {
	u16	sum_r;
	u16	sum_g;
	u16	sum_b;
	u16	count_min;
	u16	count_max;
};

struct cfa_af_stat {
	u16	sum_fy;
	u16	sum_fv1;
	u16	sum_fv2;
};

struct float_cfa_awb_stat{
	u32 sum_r;
	u32 sum_g;
	u32 sum_b;
	u32 count_min;
	u32 count_max;
};

struct float_cfa_ae_stat{
	u32 lin_y;
	u32 count_min;
	u32 count_max;
};

struct float_cfa_af_stat{
	u32 sum_fv1;
	u32 sum_fv2;
};

struct cfa_histogram_stat {
	u32	his_bin_r[64];
	u32	his_bin_g[64];
	u32	his_bin_b[64];
	u32	his_bin_y[64];
};

/* for 13696 = 128 + 13568 */
struct cfa_aaa_stat {
	struct aaa_tile_info		aaa_tile_info;
	u16						frame_id;
	struct cfa_awb_stat		awb_stat[1024];
	struct cfa_ae_stat			ae_stat[96];
	struct af_stat				af_stat[256];
	struct cfa_histogram_stat	histogram_stat;
	struct float_cfa_awb_stat	float_awb_stat[32];
	struct float_cfa_ae_stat	float_ae_stat[32];
	struct float_cfa_af_stat		float_af_stat[32];
	u8		reserved[190];
	u8		reserved_add[892];
};

struct img_statistics {
	u8	*rgb_data_valid;
	u8	*cfa_data_valid;
	u8	*hist_data_valid;
	void	*rgb_statis;
	void	*cfa_statis;
	void  *hist_statis;
};

struct black_level_global_offset {
	s32	global_offset_ee;
	s32	global_offset_eo;
	s32	global_offset_oe;
	s32	global_offset_oo;
	// For A5S
	u16	black_level_offset_red;
	u16	black_level_offset_green;
	u16	black_level_offset_blue;
	u16 gain_depedent_offset_red;
	u16 gain_depedent_offset_green;
	u16 gain_depedent_offset_blue;
};

struct color_correction_info {
	u8	enable;
	u8	no_interpolation;
	u8	yuv422_foramt;
	u8	uv_center;
	u32	multi_red;		/* 16:0 */
	u32	multi_green;		/* 16:0 */
	u32	multi_blue;		/* 16:0 */
	u32	*in_lut_addr;		/* r[2k] 11:0, r[2k+1] 23:12 */
	u32	*matrix_addr;		/* r[k] 9:0, g[k] 19:10, b[k] 29:20 */
	u32	output_lookup_bypass;
	u32	*out_lut_addr;		/* r[k] 9:0, g[k] 19:10 b[k] 29:20 */
	u32	group_index;
};

#define NUM_ALPHA_TABLE	512
/*
struct luma_sharp_edge_info {
	u32	group_index;
	u16	edge_thresh;
	u8	edge_thresh_multi;
	u8 	wide_weight;
	u8	narrow_weight;
};*/

struct luma_sharp_misc_info {
	u32	group_index;
	u8	coring_control;
	u8	add_in_low_pass;
	u8	second_input_enable;
	u8	second_input_signed;
	u8	second_input_shift;
	u8	output_signed;
	u8	output_shift;
	u8	abs;
	u8	yuv;
};

struct mctf_gmv_info {
	u32 channel_id;
	u32 stream_type;
	u32 reserved_0;
	u32 enable_external_gmv;
	u32 reserved_1;
	u32 external_gmv;
};

struct aaa_statistics_ex {
	u8 af_horizontal_filter1_mode;
	u8 af_horizontal_filter1_stage1_enb;
	u8 af_horizontal_filter1_stage2_enb;
	u8 af_horizontal_filter1_stage3_enb;
	s16 af_horizontal_filter1_gain[7];
	u16 af_horizontal_filter1_shift[4];
	u16 af_horizontal_filter1_bias_off;
	u16 af_horizontal_filter1_thresh;
	u16 af_vertical_filter1_thresh;
	u16 af_tile_fv1_horizontal_shift;
	u16 af_tile_fv1_vertical_shift;
	u16 af_tile_fv1_horizontal_weight;
	u16 af_tile_fv1_vertical_weight;

	u8 af_horizontal_filter2_mode;
	u8 af_horizontal_filter2_stage1_enb;
	u8 af_horizontal_filter2_stage2_enb;
	u8 af_horizontal_filter2_stage3_enb;
	s16 af_horizontal_filter2_gain[7];
	u16 af_horizontal_filter2_shift[4];
	u16 af_horizontal_filter2_bias_off;
	u16 af_horizontal_filter2_thresh;
	u16 af_vertical_filter2_thresh;
	u16 af_tile_fv2_horizontal_shift;
	u16 af_tile_fv2_vertical_shift;
	u16 af_tile_fv2_horizontal_weight;
	u16 af_tile_fv2_vertical_weight;
};

struct anti_aliasing_info {
	u32 enable;
	u32 threshold;
	u32 shift;
};

struct zoom_factor_info {
	u32	zoom_x;
	u32	zoom_y;
	u32	x_center_offset;
	u32	y_center_offset;
};

////////////////////////////////////////

struct vignette_compensation_info{
	u8	enable;
	u8	gain_shift;
	u16	group_index;
	u32	vignette_red_gain_ptr;
	u32	vignette_green_even_gain_ptr;
	u32	vignette_green_odd_gain_ptr;
	u32	vignette_blue_gain_ptr;
};

#define NUM_EXPOSURE_CURVE	(256)

struct local_exposure_info {
	u8  group_index;
	u8  enable;
	u16 radius;					/* 0-7 */
	u8  luma_weight_red;				/* 4:0 */
	u8  luma_weight_green;				/* 4:0 */
	u8  luma_weight_blue;				/* 4:0 */
	u8  luma_weight_sum_shift;			/* 4:0 */
	u32	gain_curve_table_addr;	/* 11:0 */
	u16 luma_offset;
	u16 reserved;
};

struct chroma_scale_info {
	u16 group_index;
	u16 enable;
	u32 make_legal;
	s16 u_weight_0;
	s16 u_weight_1;
	s16 u_weight_2;
	s16 v_weight_0;
	s16 v_weight_1;
	s16 v_weight_2;
	u32	gain_curver_addr;	/* 11:0 */
};

struct luma_sharpening_info {
	u8  group_index;
	u8  enable;
	u8  use_generated_low_pass;

	u8  input_B_enable;
	u8  input_C_enable;
	u8  FIRs_input_from_B_minus_C;
	u8  coring1_input_from_B_minus_C;
	u8  abs;
	u8  yuv;
	u8  reserved;

	u8  clip_low;
	u8  clip_high;

	u8  max_change_down;
	u8  max_change_up;
	u8  max_change_down_center;
	u8  max_change_up_center;

	// alpha control
	u8 grad_thresh_0;
	u8 grad_thresh_1;
	u8 smooth_shift;
	u8 edge_shift;
	u32 alpha_table_addr;

	// edge control
	u8  wide_weight;
	u8  narrow_weight;
	u8  edge_threshold_multiplier;
	u16 edge_thresh;
};

struct luma_sharp_fir_info {
	u8  group_index;
	u8  enable_fir1;
	u8  enable_fir2;
	u8  add_in_non_alpha;
	u8  add_in_alpha1;
	u8  add_in_alpha2;
	u16 fir1_clip_low;
	u16 fir1_clip_high;
	u16 fir2_clip_low;
	u16 fir2_clip_high;
	u32 coeff_fir1_addr;
	u32 coeff_fir2_addr;
	u32 coring_table_addr;
};

struct luma_sharp_lnl_info {
	u8  group_index;
	u8  enable;
	u8  output_normal_luma_size_select;
	u8  output_low_noise_luma_size_select;
	u8  reserved;
	u8  input_weight_red;
	u8  input_weight_green;
	u8  input_weight_blue;
	u8  input_shift_red;
	u8  input_shift_green;
	u8  input_shift_blue;
	u16 input_clip_red;
	u16 input_clip_green;
	u16 input_clip_blue;
	u16 input_offset_red;
	u16 input_offset_green;
	u16 input_offset_blue;
	u8  output_normal_luma_weight_a;
	u8  output_normal_luma_weight_b;
	u8  output_normal_luma_weight_c;
	u8  output_low_noise_luma_weight_a;
	u8  output_low_noise_luma_weight_b;
	u8  output_low_noise_luma_weight_c;
	u8  output_combination_min;
	u8  output_combination_max;
	u32 tone_curve_addr;
};

struct luma_sharp_tone_ctrl{
	u8  group_index;
	u8  shift_y;
	u8  shift_u;
	u8  shift_v;
	u8  offset_y;
	u8  offset_u;
	u8  offset_v;
	u8  bits_u;
	u8  bits_v;
	u16 reserved;
	u32 tone_based_3d_level_table_addr;
};

struct luma_sharp_blend_info {
	u32 group_index;
	u16 enable;
	u8  edge_threshold_multiplier;
	u8  iso_threshold_multiplier;
	u16 edge_threshold0;
	u16 edge_threshold1;
	u16 dir_threshold0;
	u16 dir_threshold1;
	u16 iso_threshold0;
	u16 iso_threshold1;
};

struct luma_sharp_level_info {
	u32 group_index;
	u32 select;
	u8  low;
	u8  low_0;
	u8  low_delta;
	u8  low_val;
	u8  high;
	u8  high_0;
	u8  high_delta;
	u8  high_val;
	u8  base_val;
	u8  area;
	u16 level_control_clip_low;
	u16 level_control_clip_low2;
	u16 level_control_clip_high;
	u16 level_control_clip_high2;
};

struct demoasic_filter_info {
	u8  group_index;
	u8  enable;
	u8  clamp_directional_candidates;
	u8  activity_thresh;
	u16 activity_difference_thresh;
	u16 grad_clip_thresh;
	u16 grad_noise_thresh;
	u16 grad_noise_difference_thresh;
	u16 zipper_noise_difference_add_thresh;
	u8  zipper_noise_difference_mult_thresh;
	u8  max_const_hue_factor;
};

 struct rgb_to_yuv_info {
	u32 group_index;
	s16 matrix_values[RGB_TO_YUV_MATRIX_SIZE];
	s16 y_offset;
	s16 u_offset;
	s16 v_offset;
} ;

struct cfa_noise_filter_info{
	u8  enable:2;
	u8  mode:2;
	u8  shift_coarse_ring1:2;
	u8  shift_coarse_ring2:2;
	u8  shift_fine_ring1:2;
	u8  shift_fine_ring2:2;
	u8  shift_center_red:4;
	u8  shift_center_green:4;
	u8  shift_center_blue:4;
	u8  target_coarse_red;
	u8  target_coarse_green;
	u8  target_coarse_blue;
	u8  target_fine_red;
	u8  target_fine_green;
	u8  target_fine_blue;
	u8  cutoff_red;
	u8  cutoff_green;
	u8  cutoff_blue;
	u16 thresh_coarse_red;
	u16 thresh_coarse_green;
	u16 thresh_coarse_blue;
	u16 thresh_fine_red;
	u16 thresh_fine_green;
	u16 thresh_fine_blue;
} ;

struct bad_pixel_correct_info{
	u8  dynamic_bad_pixel_detection_mode;
	u8  dynamic_bad_pixel_correction_method;
	u16 correction_mode;
	u32 hot_pixel_thresh_addr;
	u32 dark_pixel_thresh_addr;
	u16 hot_shift0_4;
	u16 hot_shift5;
	u16 dark_shift0_4;
	u16 dark_shift5;
 };

struct cfa_leakage_filter_info{
	u32 enable;
	s8 alpha_rr;
	s8 alpha_rb;
	s8 alpha_br;
	s8 alpha_bb;
	u16 saturation_level;
	//u16 threshold;
	//u8 shift;
};

struct rgb_gain_info {
	u32 r_gain;
	u32 g_even_gain;
	u32 g_odd_gain;
	u32 b_gain;
	u32 group_index;
};

struct digital_gain_level {
	u32 level_red;
	u32 level_green_even;
	u32 level_green_odd;
	u32 level_blue;
	u32 group_index;
};

struct chroma_median_filter {
	u32	enable;
	u32	group_index;
	u32	k0123_table_addr;
	u16	u_sat_t0;
	u16	u_sat_t1;
	u16	v_sat_t0;
	u16	v_sat_t1;
	u16	u_act_t0;
	u16	u_act_t1;
	u16	v_act_t0;
	u16	v_act_t1;
};

struct chroma_noise_filter_info{
	u8 enable;
	u8 radius;
	u16 mode;
	u16 thresh_u;
	u16 thresh_v;
	u16 shift_center_u;
	u16 shift_center_v;
	u16 shift_ring1;
	u16 shift_ring2;
	u16 target_u;
	u16 target_v;
};

struct aaa_statistics_config {
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
};

struct aaa_histogram_config {
	u16 mode;
	u16 hist_select;
	u16 tile_mask[8];	/* 12 * 8 tiles */
};

struct warp_info{
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

	/*
                This one is used for ARM to calcuate the
                dummy window for Ucode, these fields should be
                zero for turbo command in case of EIS. could be
                non-zero valid value only when this warp command is send
                in non-turbo command way.
	*/
	u16 dummy_window_x_left;
	u16 dummy_window_y_top;
	u16 dummy_window_width;
	u16 dummy_window_height;

	/*
                This field is used for ARM to calculate the
                cfa prescaler zoom factor which will affect
                the warping table value. this should also be zeor
                during the turbo command sending.Only valid on the
                non-turbo command time.
	*/
	u16 cfa_output_width;
	u16 cfa_output_height;
	u32 extra_sec2_vert_out_vid_mode;
};

struct chroma_aberration_warp_ctrl_info{
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
};

struct aaa_early_wb_gain_info{
	u32 red_multiplier;
	u32 green_multiplier_even;
	u32 green_multiplier_odd;
	u32 blue_multiplier;
	u8  enable_ae_wb_gain;
	u8  enable_af_wb_gain;
	u8  enable_histogram_wb_gain;
	u8  reserved;
	u32 red_wb_multiplier;
	u32 green_wb_multiplier_even;
	u32 green_wb_multiplier_odd;
	u32 blue_wb_multiplier;
};

struct aaa_floating_tile_config_info{
	u16 frame_sync_id;
	u16 number_of_tiles;
	u32 floating_tile_config_addr;
};

struct early_wb_gain_info{
	u32 cfa_early_red_multiplier;
	u32 cfa_early_green_multiplier_even;
	u32 cfa_early_green_multiplier_odd;
	u32 cfa_early_blue_multiplier;
	u32 aaa_early_red_multiplier;
	u32 aaa_early_green_multiplier_even;
	u32 aaa_early_green_multiplier_odd;
	u32 aaa_early_blue_multiplier;
};

struct dump_idsp_config_info{
	u32 dram_addr;
	u32 dram_size;
	u32 mode;
} ;

struct send_idsp_debug_cmd_info{
	u32 mode;
	u32 param1;
	u32 param2;
	u32 param3;
	u32 param4;
	u32 param5;
	u32 param6;
	u32 param7;
	u32 param8;
};

struct update_idsp_config_info{
	u16 section_id;
	u8  mode;
	u8  table_sel;
	u32 dram_addr;
	u32 data_size;
};

////////////////////////////////////////

struct sensor_info {
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
};

/**
 * pic_structure: 1: Top field, 2: Bottom field, 3: Progressive frame
 */
struct capture_video_pic_info{
	u8  decoder_id;
	u8  reserved[3];

	u32 coded_pic_base;
	u32 coded_pic_limit;
	u32 thumbnail_pic_base;
	u32 thumbnail_pic_limit;

	u16 thumbnail_width;
	u16 thumbnail_height;

	u16 thumb_letterbox_strip_width;
	u16 thumb_letterbox_strip_height;

	u8  thumb_letterbox_strip_y;
	u8  thumb_letterbox_strip_cb;
	u8  thumb_letterbox_strip_cr;
	u8  reserved0;

	u32 reserved3;

	u16 target_pic_width;
	u16 target_pic_height;

	u32 pic_structure : 8;
	u32 reserved1 : 24;

	u32 screennail_pic_base;
	u32 screennail_pic_limit;

	u16 screennail_width;
	u16 screennail_height;

	u16 screennail_letterbox_strip_width;
	u16 screennail_letterbox_strip_height;

	u8  screennail_letterbox_strip_y;
	u8  screennail_letterbox_strip_cb;
	u8  screennail_letterbox_strip_cr;
	u8  reserved2;

	u32 reserved4;
	u32 reserved5;

	int id;
};

struct mctf_load_update
{
	u32 mctf_config_update;	// load# 5-7
	u32 mctf_hist_update;	// load# 8-11
	u32 mctf_curves_update;	// load# 12-15
	u32 mcts_config_update;	// load# 16
	u32 shpb_config_update;	// load# 17
	u32 shpc_config_update;	// load# 18
	u32 shpb_fir1_update;	// load# 19
	u32 shpb_fir2_update;	// load# 20
	u32 shpc_fir1_update;	// load# 21
	u32 shpc_fir2_update;	// load# 22
	u32 shpb_alphas_update;	// load# 23
	u32 shpc_alphas_update;	// load# 24
	u32 tone_hist_update;	// load# 25-26
	u32 shpb_coring1_update;	// load# 27
	u32 shpc_coring1_update;	// load# 28
	u32 shpb_coring2_update;	// load# 29
	u32 shpb_linear_update;	// load# 30
	u32 shpc_linear_update;	// load# 31
	u32 shpb_inv_linear_update;	// load# 32
	u32 shpc_inv_linear_update;	// load# 33
	u32 mctf_3d_level_update;	// load# 34,35
	u32 shpb_3d_level_update;	// load# 36,37
	u32 cc_config_update;	// load# 48
	u32 cc_input_table_update;	// load# 49
	u32 cc_output_table_update;	// load# 50
	u32 cc_3d_table_update;	// load# 51-58
	u32 cc_matrix_update;	// load# 59
	u32 cc_blend_input_update;	// load# 60
	u32 cmpr_all_update;	// load# 62-63
	u32 mctf_mcts_all_update;
	u32 cc_all_update;
	u32 reserved;
};

struct mctf_mv_stab_info{
	u32 noise_filter_strength;
	u32 mctf_chan;
	u32 sharpen_b_chan;
	u32 cc_en;
	u32 cmpr_en;
	u32 cmpr_dither;
	u32 mode;
	u32 image_stabilize_strength;
	u32 bitrate_y;
	u32 bitrate_uv;
	u32 use_zmv_as_predictor;
	u32 reserved;
	u32 mctf_cfg_dram_addr;
	u32 mctf_cc_cfg_dram_addr;
	u32 mctf_compr_cfg_dram_addr;
	union {
		struct mctf_load_update cmds;
		u32 word;
	}loadcfg_type;
};

struct mctf_blackbar_info{
	u32 channel_id                 :8;
	u32 stream_type                :8;
	u32 padded_mbs                 :8;
};

//
//      POST PROCESSING
//
struct rescale_postproc_info{
	u8  decode_cat_id;
	u8  decode_id;
	u16 reserved_0;

	u8  voutA_enable;
	u8  voutB_enable;
	u8  pipA_enable;
	u8  pipB_enable;

	u32 fir_pts_low;
	u32 fir_pts_high;

	u16 input_center_x;
	u16 input_center_y;

	// voutA window
	u16 voutA_target_win_offset_x;
	u16 voutA_target_win_offset_y;
	u16 voutA_target_win_width;
	u16 voutA_target_win_height;

	u32 voutA_zoom_factor_x;
	u32 voutA_zoom_factor_y;

	u16 voutB_target_win_offset_x;
	u16 voutB_target_win_offset_y;
	u16 voutB_target_win_width;
	u16 voutB_target_win_height;

	u32 voutB_zoom_factor_x;
	u32 voutB_zoom_factor_y;

	// PIP display
	u16 pipA_target_offset_x;
	u16 pipA_target_offset_y;
	u16 pipA_target_x_size;
	u16 pipA_target_y_size;

	u32 pipA_zoom_factor_x;
	u32 pipA_zoom_factor_y;

	u16 pipB_target_offset_x;
	u16 pipB_target_offset_y;
	u16 pipB_target_x_size;
	u16 pipB_target_y_size;

	u32 pipB_zoom_factor_x;
	u32 pipB_zoom_factor_y;

	u8  vout0_flip;
	u8  vout0_rotate;
	u16 vout0_win_offset_x;
	u16 vout0_win_offset_y;
	u16 vout0_win_width;
	u16 vout0_win_height;

	u8  vout1_flip;
	u8  vout1_rotate;
	u16 vout1_win_offset_x;
	u16 vout1_win_offset_y;
	u16 vout1_win_width;
	u16 vout1_win_height;
};

struct black_level_state{
	u32 num_columns;				/* <= 256 */
	u32 column_frame_acc_addr;	/* 16:0 */
	u32 column_average_acc_addr;/* 19:0 */
	u32 num_rows;					/* <= 128 */
	u32 row_frame_offset_addr;	/* 16:0 */
	u32 row_average_acc_addr;	/* 19:0 */
};

struct fixed_pattern_correct {
	u32 fpn_pixel_mode;
	u32 row_gain_enable;
	u32 column_gain_enable;
	u32 num_of_rows;
	u16 num_of_cols;
	u16 fpn_pitch;
	u32 fpn_pixels_addr;
	u32 fpn_pixels_buf_size;
	u32 intercept_shift;
	u32 intercepts_and_slopes_addr;
	u32 row_gain_addr;
	u32 column_gain_addr;
};

struct still_capture_start_info{
	u8  channel_id;
	u8  stream_type;
	u8  still_process_mode;
	u8  reserved;
	u8  output_select;
	u8  input_format;
	u8  vsync_skip;
	u8  reserved_0;
	u32 number_frames_to_capture;
	u16 vidcap_w;
	u16 vidcap_h;
	u16 main_w;
	u16 main_h;
	u16 encode_w;
	u16 encode_h;
	u16 preview_w_A;
	u16 preview_h_A;
	u16 thumbnail_w;
	u16 thumbnail_h;
	u16 thumbnail_dram_w;
	u16 thumbnail_dram_h;
	u32 jpeg_bits_fifo_start;
	u32 jpeg_info_fifo_start;
	u32 sensor_readout_mode;
	u16 sensor_id;
	u8  sensor_resolution;
	u8  sensor_pattern;
	u16 preview_w_B;
	u16 preview_h_B;
	u32 raw_cap_cntl;
	u16 yuv_proc_mode; 		//raw2yuv_proc_mode
	u16 jpg_enc_cntrl; 		//jpeg_encode_cntl
	u32 raw_cap_hw_rsc_ptr; 	//raw_capture_resource_ptr
	u32 hiLowISO_proc_cfg_ptr;
	u32 preview_control;
	u32 jpeg_enc_mode;
	u32 hdr_processing;
	u32 reserved_2;
	u32 screen_thumbnail_active_w;
	u32 screen_thumbnail_active_h;
	u32 screen_thumbnail_dram_w;
	u32 screen_thumbnail_dram_h;
};

struct jpeg_encode_info{
	u32 channel_id;
	u32 stream_type;
	u32 reserved1;
	u32 chroma_format;
	u32 bits_fifo_base;
	u32 bits_fifo_limit;
	u32 info_fifo_base;
	u32 info_fifo_limit;
	u32 reserved2;
	u32 frame_rate;
	u32 process_mode;
	u32 reserved3;
	u32 targ_bits_pp; 	//target bit per pixel
	u8  initial_ql;
	u8  tolerance;
	u8  max_recode_lp;
	u8  total_sample_pts; 	//rate_curve_points
	u32 rate_curve_dram_addr;
	u32 quant_matrix_addr;
};

struct still_capture_adv_info
{
	u32 adv_yuv2jpeg;
	u32 adv_raw2yuv;
	u32 adv_raw_cap;
	u32 terminate_raw_cap;
	u32 discard_raw;
	u32 discard_yuv;
	u32 rawcap_preview;
	u32 adv_cfa_3a;
	u32 adv_hdr_out;
	u32 adv_hdr_yuv	;
	u32 disable_quickview;
	u32 raw_is_compressed;	// App supplied raw buffer is compressed.
	u32 reserved;
	u8  vsync_skip;
	u8  reserve2;
	u16 reserve3;
};

struct interval_cap_info{
	u8  channel_id;
	u8  stream_type;
	u16 mode;
	u32 action; 		//interval_cap_act;
	u32 num_of_frame; 	//frames_to_cap;
};

struct still_proc_mem_info
{
	u8  channel_id;
	u8  stream_type;
	u8  still_process_mode;
	u8  reserved;
	u8  output_select;
	u8  input_format;
	u8  bayer_pattern;
	u8  resolution;
	u32 input_address;
	u32 input_chroma_addr;
	u16 input_pitch;
	u16 input_chroma_pitch;
	u16 input_w;
	u16 input_h;
	u16 main_w;
	u16 main_h;
	u16 encode_w;
	u16 encode_h;
	s16 encode_x; 		//encode_x_ctr_offset
	s16 encode_y; 		//encode_y_ctr_offset
	u16 thumbnail_w; 	//thumbnail_active_w
	u16 thumbnail_h; 	//thumbnail_active_h
	u16 thumbnail_dram_w;
	u16 thumbnail_dram_h;
	u16 preview_w_A;
	u16 preview_h_A;
	u16 preview_w_B;
	u16 preview_h_B;
	u32 hiLowISO_proc_cfg_ptr;
	u32 preview_control;
	u32 raw2yuv_proc_mode;
	u32 raw_cap_cntl;
	u32 screen_thumbnail_active_w;
	u32 screen_thumbnail_active_h;
	u32 screen_thumbnail_dram_w;
	u32 screen_thumbnail_dram_h;
};

struct still_capture_low_iso_addr{
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
};

struct still_capture_high_iso_addr{
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
};

struct hdr_mixer_info {
	u32 mixer_mode;
	u8 radius;
	u8 luma_weight_red;
	u8 luma_weight_green;
	u8 luma_weight_blue;
	u16 threshold;
	u8 thresh_delta;
	u8 long_exposure_shift;
};

struct video_hdr_proc_control {
	u32 raw_start_x_offset[4];
	u32 raw_start_y_offset[4];
};

struct vcap_no_op{
	u32 reserved;
};

struct video_hdr_yuv_cntl_param{
	u16 low_pass_radius;
	u16 contrast_enhance_gain;
};

struct raw2enc_raw_feed_info{
	u32 sensor_raw_start_daddr; /* frm 0 start address */
	u32 daddr_offset ; /* offset from sensor_raw_start_daddr to the start address of the next frame in DRAM */
	u16 raw_width ; /* image width in pixels */
	u16 raw_height ; /* image height in pixels */
	u32 dpitch : 16 ; /* buffer pitch in bytes */
	u32 raw_compressed : 1 ; /* whether raw compression is done */
	u32 num_frames : 8 ; /* number of frames stored in DRAM starting from sensor_raw_start_daddr */
	u32 reserved : 7 ;
};

#endif
