
#ifndef _MW_PRI_STRUCT_H_
#define _MW_PRI_STRUCT_H_

#include "ambas_common.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "mw_defines.h"


typedef enum {
	MW_IDLE				= 0,
	MW_PREVIEW,
	MW_ENCODING,
	MW_STILL_CAPTURE,
	MW_DECODING,
} MW_STATE;


typedef enum {
	MW_MAIN				= 0,
	MW_SECOND,
	MW_THIRD,
	MW_FORTH,
	MW_STREAM_NUM,
} MW_STREAM;


typedef enum {
	MW_MAIN_BUFFER		= 0,
	MW_SECOND_BUFFER,
	MW_THIRD_BUFFER,
	MW_FOURTH_BUFFER,
	MW_BUFFER_NUM,
} MW_BUFFER;


typedef struct {
	const char			*name;
	u32					mode;
	s32					width;
	s32					height;
} _mw_resolution;


typedef struct {
	s32					mirror_pattern;
	s32					bayer_pattern;
} _mw_mirror_mode;


typedef struct {
	u32					state;
	u32					mode;
	u32					framerate;
	s32					anti_flicker;
	s32					src_dev_type;
	_mw_mirror_mode		mirror_mode;
} _mw_vin_info;


typedef struct {
	struct amba_vin_source_info info;
	u32					width;
	u32					height;
} _mw_vin_source_info;


typedef struct {
	u32					mode;
	u32					frame_rate;
	u32					dev_flag;
	u32					sink_type;
	u8					csc_enable;
	u8					video_enable;
	u8					rotate_enable;
	u8					fb_id;
	struct amba_vout_video_size		video_size;
	struct amba_vout_video_offset	video_offset;
} _mw_vout_info;


typedef struct {
	iav_source_buffer_info_ex_t		info;
	iav_source_buffer_format_ex_t	format;
	iav_source_buffer_type_ex_t		type;
} _mw_source_buffer;


typedef struct {
	iav_encode_stream_info_ex_t		info;
	iav_encode_format_ex_t			format;
	iav_h264_config_ex_t			h264;
	iav_jpeg_config_ex_t			mjpeg;
	u32							max_width;
	u32							max_height;
} _mw_encode_stream;


typedef struct {
	u32				lens_type;
	u16				af_mode;
	u16				af_tile_mode;
	u16				zm_idx;
	u16				fs_idx;
	u16				fs_near;
	u16				fs_far;
} _mw_af_info;


typedef struct {
	u8					* start;
	u8					* end;
	u32					size;
} _mw_mmap_info;


typedef struct {
	u16				start_x;
	u16				start_y;
	u16				width;
	u16				height;
	u32				color;		// 0:Black, 1:Red, 2:Blue, 3:Green, 4:Yellow, 5:Magenta, 6:Cyan, 7:White
	u32				action;
} _mw_privacy_mask_rect;


extern _mw_vin_info					G_mw_vin;

extern _mw_vin_source_info				G_mw_vin_source;

extern _mw_vout_info					G_mw_vout;

extern _mw_source_buffer				G_mw_buffer[MW_BUFFER_NUM];

extern iav_source_buffer_format_all_ex_t	G_mw_buffers_format;

extern iav_source_buffer_type_all_ex_t	G_mw_buffers_type;

extern iav_digital_zoom_ex_t			G_mw_dptz_I;

extern iav_digital_zoom_ex_t			G_mw_dptz[MW_BUFFER_NUM];

extern _mw_encode_stream				G_mw_stream[MW_STREAM_NUM];

extern u32							G_mw_encode_mode;

extern int								fd_iav;

#endif //  _MW_PRI_STRUCT_H_

