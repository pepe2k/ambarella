/*
 * eis_drv.h
 *
 * History:
 *	2012/08/22 - [Park Luo] created file
 *	2012/10/22 - [Jian Tang] modified file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef EIS_DRV_H
#define EIS_DRV_H

#define	EIS_IOC_MAGIC		's'

enum {
	IOC_GET_BUF_INFO = 0x1,
	IOC_DUMP_BUF = 0x2,
	IOC_UPDATE_BUF = 0x3,
	IOC_CLEAN_BUF = 0x4,
	IOC_RW_PORT = 0x5,
	IOC_EIS_GET_VIN = 0x6,
};

#define	EIS_IOC_GET_BUF_INFO			_IOR(EIS_IOC_MAGIC, IOC_GET_BUF_INFO, struct eis_buf_info_s*)
#define	EIS_IOC_DUMP_BUF				_IOR(EIS_IOC_MAGIC, IOC_DUMP_BUF, u8*)
#define	EIS_IOC_UPDATE_BUF				_IO(EIS_IOC_MAGIC, IOC_UPDATE_BUF)
#define	EIS_IOC_CLEAN_BUF				_IO(EIS_IOC_MAGIC, IOC_CLEAN_BUF)
#define	EIS_IOC_RW_PORT					_IOWR(EIS_IOC_MAGIC, IOC_RW_PORT, struct port_value_s *)
#define	EIS_IOC_GET_VIN					_IOR(EIS_IOC_MAGIC, IOC_EIS_GET_VIN, u32 *)


typedef struct eis_coeff_info_s{
	//this structure is updated by ARM
	u32 actual_left_top_x;
	u32 actual_left_top_y;
	u32 actual_right_bot_x;
	u32 actual_right_bot_y;
	s32 hor_skew_phase_inc;
	u32 zoom_y;
	u16 dummy_window_x_left;
	u16 dummy_window_y_top;
	u16 dummy_window_width;
	u16 dummy_window_height;
	u32 arm_sys_tim;
	u32 reserved[7];
} eis_coeff_info_t;

typedef struct eis_buf_info_s {
	int buf_size;
	u32 block0_addr;
	u32 block1_addr;
} eis_buf_info_t;

typedef	struct flg_piv_info_s {
	//this structure is updated by DSP
	u32 flg_PIV_status;
	u32 reserved_2[7];
} flag_piv_info_t;

typedef struct eis_update_info_s {
	u32 latest_eis_coeff_addr;
	u32 reserved[7];
	u32 sec2_hori_out_luma_addr_1;
	u32 sec2_hori_out_luma_addr_2;
	u32 sec2_hori_out_chroma_addr_1;
	u32 sec2_hori_out_chroma_addr_2;
	u32 sec2_vert_out_luma_addr_1;
	u32 sec2_vert_out_luma_addr_2;
	u32 flg_PIV_status;
	u32 arm_sys_tim;
} eis_update_info_t;

typedef struct gyro_param_s {
	s32 Offset_x;
	s32 Offset_y;
	s32 Deadband_x;
	s32 Deadband_y;
} gyro_param_t;

typedef struct eis_param_s {
	u32 eis_scale_factor_num;
	u32 eis_scale_factor_den_x;
	u32 eis_scale_factor_den_y;
	u32 sensor_cell_size_x;
	u32 sensor_cell_size_y;
} eis_param_t;

typedef enum {
	GYRO_RAW_X = 0,
	GYRO_RAW_Y = 1,
	GYRO_RAW_Z = 2,
	GYRO_RAW_T = 3,
	EIS_BUFFER_OFFSET_X_CHAN = 4,
	EIS_BUFFER_OFFSET_Y_CHAN = 5,
	EIS_BUFFER_AMV_X_CHAN = 6,
	EIS_BUFFER_AMV_Y_CHAN = 7,
	EIS_BUFFER_NUM_CHANNELS = 8,
} EIS_BUFFER_CHANNELS;

typedef struct gyro_log_s {
	struct {
		s32 *data;		/**< Data (circular) buffer */
		unsigned int ndata;	/**< Number of data elements */
		unsigned int head;	/**< Head element */
		unsigned int tail;	/**< Tail element */
		int lim_l;
		int lim_h;
	} chan[EIS_BUFFER_NUM_CHANNELS];
} gyro_log_t;

typedef struct eis_move_s {
	s32 mov_x;
	s32 mov_y;
} eis_move_t;

typedef struct eis_s {
#define NO_IS   0
#define DIS     1
#define EIS     2
	u8 enable;
	u8 mc_enable;
	u8 rsc_enable;
	u8 dbg_port_enable;
	u8 me_enable;
	u8 stabilizer_type;
	u8 proc;
	u8 enable_cmd;
	u8 disable_cmd;
	u8 adc_sampling_rate; // Bit[5:0]=sampling rate in ms, Bit[7:6]=additional scale factor(2's exponential)
	u8 enable_pref;
	u8 rsc_enable_pref;
} eis_t;

typedef struct eis_status_s {
	u8 enable;
	u8 me_enable;
	u8 mc_enable;
	u8 rsc_enable;
	u8 stabilizer_type;
	u8 reserved[3];
	u32 eis_dzoom_factor;
} eis_status_t;

typedef struct idsp_crop_info_s {
	u32 left_top_x;		// 16.16 format
	u32 left_top_y;		// 16.16 format
	u32 right_bot_x;	// 16.16 format
	u32 right_bot_y;	// 16.16 format
} idsp_crop_info_t;

typedef struct geometry_info_s {
	u32 start_x;	// The location in sensor of output pixel (0, 0)
	u32 start_y;
	u32 width;
	u32 height;
} geometry_info_t;

typedef struct is_motion_s {
	s32 pix_x;
	s32 pix_y;
	u32 max_pix_x;
	u32 max_pix_y;
	s32 ang_x;
	s32 ang_y;
	u32 max_ang_x;
	u32 max_ang_y;
} is_motion_t;

typedef struct port_value_s {
	u32 rw;
	u32 addr;
	u32 value;
} port_value_t;

typedef struct {
	s32		x;
	s32		y;
	s32		z;
} gyro_data_t;

typedef struct gyro_calib_info_s {
	u32	param_num;
	u32	mean_x;
	u32	mean_y;
	u32	var_x;
	u32	var_y;
	u32	sense_x;
	u32	sense_y;
	u32	sense_calib_freq_err;
	u32	check_sum;
} gyro_calib_info_t;

typedef struct gyro_info_s {
	u8	gyro_id;                // gyro sensor id

	u8	gyro_pwr_gpio;		// GPIO number controls gyro sensor power
	u8	gyro_hps_gpio;		// GPIO number controls gyro sensor hps
	u8	gyro_int_gpio;		// GPIO number connect to gyro sensor interrupt pin
	u8	gyro_x_chan;		// gyro sensor x axis channel
	u8	gyro_y_chan;		// gyro sensor y axis channel
	u8	gyro_z_chan;		// gyro sensor z axis channel
	u8	gyro_t_chan;		// gyro sensor t axis channel
	u8	gyro_x_reg;			// gyro sensor x axis reg
	u8	gyro_y_reg;			// gyro sensor y axis reg
	u8	gyro_z_reg;			// gyro sensor z axis reg
	s8	gyro_x_polar;		// gyro sensor x polarity
	s8	gyro_y_polar;		// gyro sensor y polarity
	s8	gyro_z_polar;		// gyro sensor z polarity
	u8	vol_div_num;		// numerator of voltage divider
	u8	vol_div_den;		// denominator of voltage divider

	u8	sensor_interface;	// gyro sensor interface
	u8	sensor_axis;		// gyro sensor axis
	u8	max_rms_noise;		// gyro sensor rms noise level
	u8	adc_resolution;		// gyro internal adc resolution, unit in bit(s)
	s8	phs_dly_in_ms;		// gyro sensor phase delay, unit in ms
	u8	reserved;
	u16	sampling_rate;		// digital gyro internal sampling rate, unit in samples / sec
	u16	max_sampling_rate;	// max digital gyro internal sampling rate, unit in samples / sec
	u16	max_bias;			// max gyro sensor bias
	u16	min_bias;			// min gyro sensor bias
	u16	max_sense;			// max gyro sensor sensitivity
	u16	min_sense;			// min gyro sensor sensitivity
	u16	start_up_time;		// gyro sensor start-up time
	u16	full_scale_range;	// gyro full scale range
	u16	max_sclk;			// max serial clock for digital interface, unit in 100khz
} gyro_info_t;

void gyro_get_info(gyro_info_t *);
typedef void (*gyro_eis_callback_t)(gyro_data_t *, void *);
void gyro_register_eis_callback(gyro_eis_callback_t cb, void *arg);
void gyro_unregister_eis_callback(void);


#endif	// EIS_DRV_H

