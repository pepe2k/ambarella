/*
 * build/include/ambas_vin.h
 *
 * History:
 *    2008/01/18 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBAS_VIN_H
#define __AMBAS_VIN_H

#define AMBA_VIN_NAME_LENGTH		32

#define AMBA_VIN_SRC_DEV_TYPE_CMOS	(1 << 0)
#define AMBA_VIN_SRC_DEV_TYPE_CCD	(1 << 1)
#define AMBA_VIN_SRC_DEV_TYPE_DECODER	(1 << 2)

#define AMBA_VIN_ADAPTER_STARTING_ID	0
#define AMBA_VIN_SOURCE_STARTING_ID	0

#include <ambas_common.h>

/* ========================= IAV VIN IO defines ============================= */
#include <linux/ioctl.h>

#define IAV_IOC_VIN_MAGIC			'i'

#define IAV_IOC_VIN_GET_SOURCE_NUM		_IOR(IAV_IOC_VIN_MAGIC, 1, int *)
#define IAV_IOC_VIN_GET_CURRENT_SRC		_IOR(IAV_IOC_VIN_MAGIC, 2, int *)
#define IAV_IOC_VIN_SET_CURRENT_SRC		_IOW(IAV_IOC_VIN_MAGIC, 2, int *)

#define IAV_IOC_VIN_SRC_MAGIC			's'

enum amba_vin_camera_location {
	AMBA_VIN_CAMERA_REAR			= 0,
	AMBA_VIN_CAMERA_FRONT,
};

enum amba_vin_capture_mode {
	AMBA_VIN_CAPTURE_MODE_ERS_CONTINUOUS	= 0,
	AMBA_VIN_CAPTURE_MODE_ERS_SNAPSHOT,
	AMBA_VIN_CAPTURE_MODE_ERS_BULB,
	AMBA_VIN_CAPTURE_MODE_GRR_SNAPSHOT,
	AMBA_VIN_CAPTURE_MODE_GRR_BULB
};

enum amba_vin_trigger_source {
	AMBA_VIN_TRIGGER_SOURCE_GPIO		= 0,
	AMBA_VIN_TRIGGER_SOURCE_EXT
};

enum amba_vin_flash_level {
	AMBA_VIN_FLASH_LEVEL_LOW		= 0,
	AMBA_VIN_FLASH_LEVEL_HIGH
};

enum amba_vin_flash_status {
	AMBA_VIN_FLASH_OFF			= 0,
	AMBA_VIN_FLASH_ON
};

enum amba_vin_cmos_source_type {
	AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO		= 0,
};

enum amba_vin_ccd_source_type {
	AMBA_VIN_CCD_CHANNEL_TYPE_AUTO		= 0,
};

enum amba_vin_decoder_source_type {
	AMBA_VIN_DECODER_CHANNEL_TYPE_CVBS	= 0,
	AMBA_VIN_DECODER_CHANNEL_TYPE_SVIDEO,
	AMBA_VIN_DECODER_CHANNEL_TYPE_YPBPR,
	AMBA_VIN_DECODER_CHANNEL_TYPE_HDMI,
	AMBA_VIN_DECODER_CHANNEL_TYPE_VGA,

	AMBA_VIN_DECODER_CHANNEL_TYPE_NUM
};

enum amba_vin_src_mirror_pattern {
	AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY	= 0,
	AMBA_VIN_SRC_MIRROR_HORRIZONTALLY,
	AMBA_VIN_SRC_MIRROR_VERTICALLY,
	AMBA_VIN_SRC_MIRROR_NONE,

	AMBA_VIN_SRC_MIRROR_AUTO			= 255,
};
enum amba_vin_src_bayer_pattern {
	AMBA_VIN_SRC_BAYER_PATTERN_RG		= 0,
	AMBA_VIN_SRC_BAYER_PATTERN_BG,
	AMBA_VIN_SRC_BAYER_PATTERN_GR,
	AMBA_VIN_SRC_BAYER_PATTERN_GB,

	AMBA_VIN_SRC_BAYER_PATTERN_AUTO		= 255,
};
enum amba_vin_anti_flicker {
	AMBA_VIN_SRC_NO_ANTI_FLICKER		= 0,
	AMBA_VIN_SRC_50HZ_ANTI_FLICKER,
	AMBA_VIN_SRC_60HZ_ANTI_FLICKER,
 };
enum amba_vin_sensor_id {
	//Micron Sensor
	SENSOR_MT9J001			= 0x00000000,
	SENSOR_MT9M033,
	SENSOR_MT9P001,
	SENSOR_MT9V136,
	SENSOR_MT9T002,
	SENSOR_AR0331,
	SENSOR_AR0130,
	SENSOR_AR0835HS,

	//OV Sensor
	SENSOR_OV10620			= 0x00001000,
	SENSOR_OV14810,
	SENSOR_OV2710,
	SENSOR_OV5653,
	SENSOR_OV5658,
	SENSOR_OV7720,
	SENSOR_OV7725,
	SENSOR_OV7740,
	SENSOR_OV9710,
	SENSOR_OV10630,
	SENSOR_OV9726,
	SENSOR_OV9718,
	SENSOR_OV4688,
	SENSOR_OV4689,

	//Samsung Sensor
	SENSOR_S5K3E2FX			= 0x00002000,
	SENSOR_S5K4AWFX,
	SENSOR_S5K5B3GX,

	//Sony Sensor
	SENSOR_IMX035			= 0x00003000,
	SENSOR_IMX036,
	SENSOR_IMX072,
	SENSOR_IMX122,
	SENSOR_IMX121,
	SENSOR_IMX104,
	SENSOR_IMX136,
	SENSOR_IMX105,
	SENSOR_IMX172,
	SENSOR_IMX178,
	SENSOR_IMX078,
	SENSOR_IMX185,
	SENSOR_IMX226,
	SENSOR_IMX123,
	SENSOR_IMX123_DCG,
	SENSOR_IMX124,
	SENSOR_IMX071,
	SENSOR_IMX224,
	SENSOR_IMX183,
	SENSOR_IMX290,

	//Panasonic Sensor
	SENSOR_MN34041PL			= 0x00004000,
	SENSOR_MN34031PL,
	SENSOR_MN34210PL,
	SENSOR_MN34220PL,
	SENSOR_MN34230PL,

	//Toshiba Sensor
	SENSOR_TCM5117			= 0x00005000,

	//fpga vin
	SENSOR_ALTERA_FPGA		= 0x00006000,
	//Dummy Sensor
	SENSOR_AMBDS			= 0x00008000,

	//ADI Decoder
	DECODER_ADV7403			= 0x80000000,
	DECODER_ADV7441A,
	DECODER_ADV7443,
	DECODER_ADV7619,
	//RICHNEX Decoder
	DECODER_RN6240			= 0x80001000,

	//TI Decoder
	DECODER_TVP5150			= 0x80002000,

	//Techwell Decoder
	DECODER_TW2864			= 0x80003000,
	DECODER_TW9910,

	DECODER_GS2970			= 0x80004000,

	//Dummy Decoder
	DECODER_AMBDD			= 0x8fffffff,
};

enum amba_vin_interface_type {
	SENSOR_IF_PARALLEL	=0,
	SENSOR_IF_LVDS,
	SENSOR_IF_MIPI,
	SENSOR_IF_PARALLEL_LVDS
};

struct amba_vin_source_info {
	int	id;		//Source ID
	int	adapter_id;
	u32	dev_type;	//AMBA_VIN_SRC_DEV_TYPE_*
	char	name[AMBA_VIN_NAME_LENGTH];
	union {
		enum amba_vin_cmos_source_type		cmos;
		enum amba_vin_ccd_source_type		ccd;
		enum amba_vin_decoder_source_type	decoder;
		u32					dummy;	//Make source_type u32
	} source_type;
	enum amba_vin_sensor_id		sensor_id;
	enum amba_video_mode		default_mode;
	u32				total_channel_num;
	u32				active_channel_id;
	u32				interface_type;
};
#define IAV_IOC_VIN_SRC_GET_INFO		_IOR(IAV_IOC_VIN_SRC_MAGIC, 1, struct amba_vin_source_info *)

struct amba_vin_source_mode_info {
	u32 mode;				//enum amba_video_mode mode;
	struct amba_video_info video_info;	//Default value of that mode.
	u32 is_supported;			//0: Not supported. 1: Supported
	u32 fps_table_size;			//sizeof(fps_table)
	u32 *fps_table;
};
#define IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE	_IOR(IAV_IOC_VIN_SRC_MAGIC, 2, struct amba_vin_source_mode_info *)
#define IAV_IOC_VIN_SRC_SET_VIDEO_MODE		_IOW(IAV_IOC_VIN_SRC_MAGIC, 3, int)
#define IAV_IOC_VIN_SRC_GET_VIDEO_MODE		_IOR(IAV_IOC_VIN_SRC_MAGIC, 3, int *)

#define IAV_IOC_VIN_SRC_SET_FRAME_RATE		_IOW(IAV_IOC_VIN_SRC_MAGIC, 4, int)	//AMBA_VIDEO_FPS
#define IAV_IOC_VIN_SRC_GET_FRAME_RATE		_IOR(IAV_IOC_VIN_SRC_MAGIC, 4, int *)	//AMBA_VIDEO_FPS

#define IAV_IOC_VIN_SRC_GET_VIDEO_INFO		_IOR(IAV_IOC_VIN_SRC_MAGIC, 5, struct amba_video_info *)

struct amba_vin_black_level_compensation {
	s16 bl_oo;
	s16 bl_oe;
	s16 bl_eo;
	s16 bl_ee;
};
#define IAV_IOC_VIN_SRC_SET_BLC				_IOW(IAV_IOC_VIN_SRC_MAGIC, 6, struct amba_vin_black_level_compensation *)

#define IAV_IOC_VIN_SRC_SET_SHUTTER_TIME		_IOW(IAV_IOC_VIN_SRC_MAGIC, 7, u32)
#define IAV_IOC_VIN_SRC_GET_SHUTTER_TIME		_IOR(IAV_IOC_VIN_SRC_MAGIC, 7, u32*)
#define IAV_IOC_VIN_SRC_SET_AGC_DB			_IOW(IAV_IOC_VIN_SRC_MAGIC, 8, int)
#define IAV_IOC_VIN_SRC_GET_AGC_DB			_IOR(IAV_IOC_VIN_SRC_MAGIC, 8, int*)
#define  IAV_IOC_VIN_SRC_GET_SHUTTER_WIDTH   _IOWR(IAV_IOC_VIN_SRC_MAGIC, 24, u32*)
struct amba_vin_dgain_info {
		u32 r_ratio;
		u32 gr_ratio;
		u32 gb_ratio;
		u32 b_ratio;
};
 struct sensor_cmd_info_t{
	u32		gain_tbl_index;
	u32		shutter_row;
};
#define IAV_IOC_VIN_SRC_SET_DGAIN_RATIO				_IOW(IAV_IOC_VIN_SRC_MAGIC, 9, struct amba_vin_dgain_info*)
#define IAV_IOC_VIN_SRC_GET_DGAIN_RATIO				_IOR(IAV_IOC_VIN_SRC_MAGIC, 9, struct amba_vin_dgain_info*)

#if 0
enum amba_vin_trigger_mode {
	AMBA_VIN_TRIGGER_MODE_OFF = 0,
	AMBA_VIN_TRIGGER_MODE_ON,
};

#define IAV_IOC_VIN_SET_TRIGGER_MODE		_IOW(IAV_IOC_VIN_SRC_MAGIC, 12, int)
#else
struct amba_vin_trigger_mode{
	u16	trigger_select;	//0:use trigger0; 1:use trigger 1
	u8	enable;
	u8	polariy;		//0: act_low; 1: act_high
	u16	start_line;
	u16	end_line;
};

#define IAV_IOC_VIN_SET_TRIGGER_MODE		_IOW(IAV_IOC_VIN_SRC_MAGIC, 10, struct amba_vin_trigger_mode*)
#endif
struct amba_vin_src_mirror_mode{
	u32 pattern;				//enum amba_vin_src_mirror_pattern pattern;
	u32 bayer_pattern;			//enum amba_vin_src_bayer_pattern bayer_pattern;
};
#define IAV_IOC_VIN_SRC_SET_MIRROR_MODE		_IOW(IAV_IOC_VIN_SRC_MAGIC, 11, struct amba_vin_src_mirror_mode*)
#define IAV_IOC_VIN_SRC_GET_MIRROR_MODE		_IOR(IAV_IOC_VIN_SRC_MAGIC, 11, struct amba_vin_src_mirror_mode*)

#define IAV_IOC_VIN_SRC_SET_ANTI_FLICKER	_IOW(IAV_IOC_VIN_SRC_MAGIC, 12, int)

/* FPS stat info */
struct amba_fps_report_s{
	u32 counter_freq;
	u32 avg_frame_diff;
	u32 max_frame_diff;
	u32 min_frame_diff;
};

/* agc info */
typedef struct amba_vin_agc_info_s {
	s32	db_max;
	s32	db_min;
	s32	db_step;
} amba_vin_agc_info_t;

#define IAV_IOC_VIN_SRC_GET_AGC_INFO		_IOW(IAV_IOC_VIN_SRC_MAGIC, 13, amba_vin_agc_info_t*)

/* exposure table info */
typedef struct amba_vin_shutter_info_s {
	u16	max_shutter_value;	// not used
	u16	min_shutter_value;	// not used
	u16	shutter_step;	// shutter_step/gain_step
} amba_vin_shutter_info_t;

#define IAV_IOC_VIN_SRC_GET_SHUTTER_INFO	_IOW(IAV_IOC_VIN_SRC_MAGIC, 14, amba_vin_shutter_info_t*)

typedef enum {
	AMBA_VIN_LINEAR_MODE = 0,
	AMBA_VIN_HDR_MODE,
} amba_vin_sensor_op_mode;

#define IAV_IOC_VIN_SET_OPERATION_MODE		_IOW(IAV_IOC_VIN_SRC_MAGIC , 15, amba_vin_sensor_op_mode)
#define IAV_IOC_VIN_GET_OPERATION_MODE		_IOR(IAV_IOC_VIN_SRC_MAGIC , 15, amba_vin_sensor_op_mode*)

struct amba_vin_eis_info {
		u16 cap_start_x;
		u16 cap_start_y;
		u16 cap_cap_w;
		u16 cap_cap_h;
		u16 source_width;
		u16 source_height;
		u32 current_fps;
		u32 main_fps;
		u32 current_shutter_time;
		u32 row_time;
		u16 vb_lines;
		u16 sensor_cell_width;
		u16 sensor_cell_height;
		u8 column_bin;
		u8 row_bin;
};

#define IAV_IOC_VIN_SRC_GET_EIS_INFO				_IOR(IAV_IOC_VIN_SRC_MAGIC, 16, struct amba_vin_eis_info*)

#define IAV_IOC_VIN_SRC_SET_WDR_AGAIN			_IOW(IAV_IOC_VIN_SRC_MAGIC, 17, int)
#define IAV_IOC_VIN_SRC_GET_WDR_AGAIN			_IOR(IAV_IOC_VIN_SRC_MAGIC, 17, int*)

struct amba_vin_wdr_gain_gp_info {
		u32 long_gain;
		u32 short1_gain;
		u32 short2_gain;
		u32 short3_gain;
};
#define IAV_IOC_VIN_SRC_SET_WDR_DGAIN_GP			_IOW(IAV_IOC_VIN_SRC_MAGIC, 18, struct amba_vin_wdr_gain_gp_info*)
#define IAV_IOC_VIN_SRC_GET_WDR_DGAIN_GP			_IOR(IAV_IOC_VIN_SRC_MAGIC, 18, struct amba_vin_wdr_gain_gp_info*)

struct amba_vin_wdr_shutter_gp_info {
		u32 long_shutter;
		u32 short1_shutter;
		u32 short2_shutter;
		u32 short3_shutter;
};
#define IAV_IOC_VIN_SRC_SET_WDR_SHUTTER_GP	_IOW(IAV_IOC_VIN_SRC_MAGIC, 19, struct amba_vin_wdr_shutter_gp_info*)
#define IAV_IOC_VIN_SRC_GET_WDR_SHUTTER_GP	_IOR(IAV_IOC_VIN_SRC_MAGIC, 19, struct amba_vin_wdr_shutter_gp_info*)
#define IAV_IOC_VIN_SRC_WDR_SHUTTER2ROW	_IOWR(IAV_IOC_VIN_SRC_MAGIC, 19, struct amba_vin_wdr_shutter_gp_info*)

#define IAV_IOC_VIN_SRC_GET_SENSOR_TEMP		_IOR(IAV_IOC_VIN_SRC_MAGIC, 20, u64 *)

#define IAV_IOC_VIN_SRC_SET_SHUTTER_TIME_ROW	_IOW(IAV_IOC_VIN_SRC_MAGIC, 21, u32)
#define IAV_IOC_VIN_SRC_SET_WDR_SHUTTER_ROW_GP	_IOW(IAV_IOC_VIN_SRC_MAGIC, 22, struct amba_vin_wdr_shutter_gp_info*)
#define IAV_IOC_VIN_SRC_SET_AGC_INDEX			_IOW(IAV_IOC_VIN_SRC_MAGIC, 23, int)

#define IAV_IOC_VIN_SRC_SET_WDR_AGAIN_INDEX		_IOW(IAV_IOC_VIN_SRC_MAGIC, 24, int)
#define IAV_IOC_VIN_SRC_SET_WDR_DGAIN_INDEX_GP	_IOW(IAV_IOC_VIN_SRC_MAGIC, 25, struct amba_vin_wdr_gain_gp_info*)
#define IAV_IOC_VIN_SRC_SET_WDR_AGAIN_INDEX_GP	_IOW(IAV_IOC_VIN_SRC_MAGIC, 26, struct amba_vin_wdr_gain_gp_info*)

#define IAV_IOC_VIN_SRC_SET_SHUTTER_TIME_ROW_SYNC _IOW(IAV_IOC_VIN_SRC_MAGIC, 27, u32)
#define IAV_IOC_VIN_SRC_SET_AGC_INDEX_SYNC _IOW(IAV_IOC_VIN_SRC_MAGIC, 28, u32)
#define IAV_IOC_VIN_SRC_SET_SHUTTER_AND_AGC_SYNC _IOW(IAV_IOC_VIN_SRC_MAGIC, 29, struct sensor_cmd_info_t*)

#define IAV_IOC_VIN_SRC_STOP_SENSOR_STREAM		_IOW(IAV_IOC_VIN_SRC_MAGIC, 30, u8)

#define IAV_IOC_VIN_SRC_SET_LOW_LIGHT_MODE		_IOW(IAV_IOC_VIN_SRC_MAGIC, 31, u8)

struct amba_vin_aaa_info {
		u32 pixel_size;
		u32 slow_shutter_support;
};
#define IAV_IOC_VIN_SRC_GET_AAAINFO		_IOR(IAV_IOC_VIN_SRC_MAGIC, 32, struct amba_vin_aaa_info*)

struct amba_vin_wdr_win_offset{
	u16	long_start_x;
	u16	long_start_y;
	u16	short1_start_x;
	u16	short1_start_y;
	u16	short2_start_x;
	u16	short2_start_y;
	u16	short3_start_x;
	u16	short3_start_y;
};
#define IAV_IOC_VIN_SRC_GET_WDR_WIN_OFFSET	_IOR(IAV_IOC_VIN_SRC_MAGIC, 33, struct amba_vin_wdr_win_offset*)
/* ========================= IAV IO defines ============================= */
struct iav_still_init_info {	//IAV_IOC_INIT_STILL_CAPTURE
	enum amba_vin_capture_mode capture_mode;
	enum amba_vin_trigger_source trigger_source;
	enum amba_video_mode still_mode;
	enum amba_vin_flash_level flash_level;
	enum amba_vin_flash_status flash_status;
	int jpeg_quality;
};

struct iav_still_cap_info {	//IAV_IOC_START_STILL_CAPTURE
	u32 continuous_frames;
	u32 width;		//JPEG width, 0: Auto
	u32 height;		//JPEG height, 0: Auto
	u32 thumbnail_w;	//Thumbnail width, 0: Auto
	u32 thumbnail_h;	//Thumbnail height, 0: Auto
	u32 vsyn_skip;
	u32 need_raw : 1;
	u32 keep_AR_flag : 1;	//Keep correct aspect ratio flag
	u32 reserved : 30;
};

struct iav_still_proc_mem_info{	//IAV_IOC_START_STILL_CAPTURE
	u32 width;		//JPEG width, 0: Auto
	u32 height;		//JPEG height, 0: Auto
};

struct iav_init_still_mem_info{
	u32 width;
	u32 height;
	u8* __user_raw_addr;
};
#endif

