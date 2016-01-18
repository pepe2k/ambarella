/*
 * test_encode.c
 * the program can setup VIN , preview and start encoding/stop
 * encoding for flexible multi streaming encoding.
 * after setup ready or start encoding/stop encoding, this program
 * will exit
 *
 * History:
 *	2010/12/31 - [Louis Sun] create this file base on test2.c
 *	2011/10/31 - [Jian Tang] modified this file.
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "amba_usb.h"
#include <signal.h>

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif

#ifndef DIV_ROUND
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#endif

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			} 						\
		} while (0)
#endif

#define MAX_ENCODE_STREAM_NUM		(IAV_STREAM_MAX_NUM_IMPL)

typedef enum {
	MAIN_SOURCE_BUFFER = 0,
	SECOND_SOURCE_BUFFER = 1,
	THIRD_SOURCE_BUFFER = 2,
	FOURTH_SOURCE_BUFFER = 3,
	MAIN_SOURCE_DRAM = 4,
	MAX_SOURCE_BUFFER_NUM,

	PRE_MAIN_BUFFER = MAX_SOURCE_BUFFER_NUM,
	MAX_WARP_SOURCE_BUFFER_NUM = PRE_MAIN_BUFFER + 1, // source buffer + source dram + pre-main
	MAX_BUFFER_NUM = MAX_WARP_SOURCE_BUFFER_NUM,
	MAX_PREVIEW_BUFFER_NUM = 2,
} AMBA_SOURCE_BUFFER_ID;


//support up to 4 streams by this design
#define ALL_ENCODE_STREAMS	((1 << MAX_ENCODE_STREAM_NUM) - 1)

#define COLOR_PRINT0(msg)		printf("\033[34m"msg"\033[39m")
#define COLOR_PRINT(msg, arg...)		printf("\033[34m"msg"\033[39m", arg)
#define BOLD_PRINT0(msg, arg...)		printf("\033[1m"msg"\033[22m")
#define BOLD_PRINT(msg, arg...)		printf("\033[1m"msg"\033[22m", arg)


/* osd blending mixer selection */
#define OSD_BLENDING_OFF				0
#define OSD_BLENDING_FROM_MIXER_A		1
#define OSD_BLENDING_FROM_MIXER_B		2

//static struct timeval start_enc_time;

/*
 * use following commands to enable timer1
 *

amba_debug -w 0x7000b014 -d 0xffffffff
amba_debug -w 0x7000b030 -d 0x15
amba_debug -r 0x7000b000 -s 0x40

*/

// the device file handle
static int fd_iav = -1;

// QP Matrix buffer
static u8 * G_qp_matrix_addr = NULL;
static int G_qp_matrix_size = 0;

// vin
#include "../../vin_test/vin_init.c"
#include "../../vout_test/vout_init.c"

#define VERIFY_STREAMID(x)   do {		\
			if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
				printf ("stream id wrong %d \n", (x));			\
				return -1; 	\
			}	\
		} while (0)

#define VERIFY_BUFFERID(x)	do {		\
			if ((x) < 0 || ((x) >= MAX_BUFFER_NUM)) {	\
				printf ("buffer id wrong %d\n", (x));			\
				return -1;	\
			}	\
		} while (0)

#define VERIFY_SUB_BUFFERID(x)	do {		\
			if ((x) < 1 || ((x) >= MAX_SOURCE_BUFFER_NUM)) {	\
				printf ("sub buffer id wrong %d\n", (x));			\
				return -1;	\
			}	\
		} while (0)


typedef u32 id_map_t ;

//encode format
typedef struct stream_encode_format_s {
	int type;
	int type_changed;

	int width;
	int height;
	int resolution_changed;

	int hflip;
	int hflip_changed;

	int vflip;
	int vflip_changed;

	int rotate;
	int rotate_changed;

	int offset_x;
	int offset_y;
	int offset_changed;

	int source;
	int source_changed;

	int duration;
	int duration_changed;

	int negative_offset_disable_changed;
	int negative_offset_disable;
} stream_encode_format_t;


//source buffer format
typedef struct source_buffer_format_s {
	int width;
	int height;
	int resolution_changed;

	int input_width;
	int input_height;
	int input_size_changed;

	int input_x;
	int input_y;
	int input_offset_changed;
}  source_buffer_format_t;

//source buffer setup
typedef struct source_buffer_setup_s {
	iav_source_buffer_type_ex_t buffer_type[MAX_SOURCE_BUFFER_NUM];
	id_map_t buffer_type_changed_map;

	u32 unwarp[MAX_SOURCE_BUFFER_NUM];
	id_map_t buffer_unwarp_changed_map;
}  source_buffer_setup_t;

// system resource setup
typedef struct system_resource_setup_s {
	int encode_mode;
	int encode_mode_changed;

	int buffer_max_width[MAX_SOURCE_BUFFER_NUM];
	int buffer_max_height[MAX_SOURCE_BUFFER_NUM];
	id_map_t buffer_max_changed_map;

	int stream_max_width[MAX_ENCODE_STREAM_NUM];
	int stream_max_height[MAX_ENCODE_STREAM_NUM];
	id_map_t stream_max_changed_map;

	int GOP_max_M[MAX_ENCODE_STREAM_NUM];
	id_map_t GOP_max_M_changed_map;

	int stream_2x_search_range[MAX_ENCODE_STREAM_NUM];
	id_map_t stream_2x_search_range_changed_map;

	int sharpen_b;
	int sharpen_b_changed;

	int enc_from_raw;
	int enc_from_raw_changed;

	int rotate_possible;
	int rotate_possible_changed;

	int exposure_num;
	int exposure_num_changed;

	int vin_num;
	int vin_num_changed;

	int extra_dram_buf[MAX_SOURCE_BUFFER_NUM];
	int extra_dram_buf_changed_map;

	int hwarp_bypass_possible;
	int hwarp_bypass_possible_changed;

	int stream_max_num;
	int stream_max_num_changed;

	int raw_capture_enabled;
	int raw_capture_enabled_changed;

	int raw_stats_lines_top;
	int raw_stats_lines_bottom;
	int raw_stats_lines_changed;

	int warp_max_input_width;
	int warp_max_input_width_changed;

	int warp_max_output_width;
	int warp_max_output_width_changed;

	int max_chroma_noise_shift;
	int max_chroma_noise_shift_changed;

	int max_dram_frame[IAV_ENCODE_SOURCE_DRAM_TOTAL_NUM];
	int max_dram_frame_map;

	int vskip_before_encode;
	int vskip_before_encode_changed;

	int extra_2x_enable;
	int extra_2x_enable_changed;

	int vca_buffer_enable;
	int vca_buffer_enable_changed;

	int debug_max_ref_P[MAX_ENCODE_STREAM_NUM];

	int vout_b_letter_box_enable;
	int vout_b_letter_box_enable_changed;
	id_map_t debug_max_ref_P_changed_map;

	int map_dsp_partition;
	int map_dsp_partition_changed;

	int yuv_input_enhanced;
	int yuv_input_enhanced_changed;

	int debug_chip_id;
	int debug_chip_id_flag;
} system_resource_setup_t;

// system info setup
typedef struct system_info_setup_s {
	int osd_mixer;
	int osd_mixer_changed;

	int bits_read_mode;
	int bits_read_mode_changed;

	int cmd_read_mode;
	int cmd_read_mode_changed;

	int eis_delay_count;
	int eis_delay_count_changed;

	int vout_swap;
	int vout_swap_changed;

	int pm_type;
	int pm_type_changed;

	//DSP non-cached
	int dsp_noncache;
	int dsp_noncache_changed;

	int debug_enc_dummy_latency_count;
	int debug_enc_dummy_latency_flag;
} system_info_setup_t;

//qp limit
typedef struct qp_limit_params_s {
	u8	min_i;
	u8	max_i;
	u8	i_changed;

	u8	min_p;
	u8	max_p;
	u8	p_changed;

	u8	min_b;
	u8	max_b;
	u8	b_changed;

	u8	min_q;
	u8	max_q;
	u8	q_changed;
	u8	adapt_qp;
	u8	adapt_qp_changed;

	u8	i_qp_reduce;
	u8	i_qp_reduce_changed;

	u8	p_qp_reduce;
	u8	p_qp_reduce_changed;
	u8	q_qp_reduce;
	u8	q_qp_reduce_changed;
	u8	log_q_num;
	u8	log_q_num_changed;

	u8	skip_frame;
	u8	skip_frame_changed;
} qp_limit_params_t;

// qp matrix delta value
typedef struct qp_matrix_params_s {
	s8	delta[4];
	int	delta_changed;

	int	matrix_mode;
	int	matrix_mode_changed;
} qp_matrix_params_t;

// h.264 config
typedef struct h264_param_s {
	int h264_M;
	int h264_M_changed;

	int h264_N;
	int h264_N_changed;

	int h264_idr_interval;
	int h264_idr_interval_changed;

	int h264_gop_model;
	int h264_gop_model_changed;

	int debug_h264_long_p_interval;
	int debug_h264_long_p_interval_changed;

	int h264_bitrate_control;
	int h264_bitrate_control_changed;

	int h264_cbr_avg_bitrate;
	int h264_cbr_bitrate_changed;

	int h264_vbr_max_bitrate;
	int h264_vbr_min_bitrate;
	int h264_vbr_bitrate_changed;

	int h264_deblocking_filter_alpha;
	int h264_deblocking_filter_alpha_changed;

	int h264_deblocking_filter_beta;
	int h264_deblocking_filter_beta_changed;

	int h264_deblocking_filter_disable;
	int h264_deblocking_filter_disable_changed;

	int h264_chrome_format;			// 0: YUV420; 1: Mono
	int h264_chrome_format_changed;

	int h264_profile_level;
	int h264_profile_level_changed;

	int h264_qmatrix_4x4;
	int h264_qmatrix_4x4_changed;

	int force_intlc_tb_iframe;
	int force_intlc_tb_iframe_changed;

	int au_type;
	int au_type_changed;

	u8 panic_num;
	u8 panic_den;
	int panic_ratio_changed;

	int debug_h264_ref_P;
	int debug_h264_ref_P_changed;
} h264_param_t;

typedef struct jpeg_param_s {
	int quality;
	int quality_changed;

	int jpeg_chrome_format;			// 1: YUV420; 2: Mono
	int jpeg_chrome_format_changed;
} jpeg_param_t;

typedef struct h264_enc_param_s {
	u16	intrabias_p;
	u16	intrabias_p_changed;

	u16	intrabias_b;
	u16	intrabias_b_changed;

	u16	bias_p_skip;
	u16	bias_p_skip_changed;

	u16	cpb_underflow_num;
	u16	cpb_underflow_den;
	u16	cpb_underflow_changed;

	u8	zmv_threshold;
	u8	zmv_threshold_changed;
	s8	mode_bias_I4_add;
	u8	mode_bias_I4_add_changed;
	s8	mode_bias_I16_add;
	u8	mode_bias_I16_add_changed;
	s8	mode_bias_Inter8Add;
	u8	mode_bias_Inter8Add_changed;
	s8	mode_bias_Inter16Add;
	u8	mode_bias_Inter16Add_changed;
	s8	mode_bias_DirectAdd;
	u8	mode_bias_DirectAdd_changed;
} h264_enc_param_t;

typedef struct stream_encode_param_s {
	h264_param_t h264_param;
	jpeg_param_t jpeg_param;
} stream_encode_param_t;

typedef struct vcap_param_s {
	u8	video_freeze;
	u8	video_freeze_flag;
} vcap_param_t;

typedef struct debug_param_s {
	u8	check_disable;
	u8	check_disable_flag;

	u8	kernel_print_enable;
	u8	kernel_print_enable_flag;
} debug_param_t;

// state control
static int idle_cmd = 0;
static int nopreview_flag = 0;		// do not enter preview automatically

// vin capture window offset
static u32 vin_cap_start_x = 0;
static u32 vin_cap_start_y = 0;
static u32 vin_cap_offset_flag = 0;

//stream and source buffer identifier
static int current_stream = -1; 	// -1 is a invalid stream, for initialize data only
static int current_buffer = -1;	// -1 is a invalid buffer, for initialize data only

//encode start/stop/idr control variables
static iav_stream_id_t start_stream_id = 0;
static iav_stream_id_t stop_stream_id = 0;
static iav_stream_id_t force_idr_id = 0;

//encoding settings
static stream_encode_format_t stream_encode_format[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t  stream_encode_format_changed_map = 0;

static stream_encode_param_t stream_encode_param[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t stream_encode_param_changed_map  = 0;

//source buffer settings
static source_buffer_setup_t source_buffer_setup;
static id_map_t source_buffer_setup_changed;
static source_buffer_format_t source_buffer_format[MAX_BUFFER_NUM];
static id_map_t source_buffer_format_changed_map;

// system resource settings
static system_resource_setup_t system_resource_setup;
static int system_resource_setup_changed = 0;

// system info setting
static system_info_setup_t system_info_setup;
static int system_info_setup_changed;

// real time setting
static int framerate_factor[MAX_ENCODE_STREAM_NUM][3];
static iav_stream_id_t framerate_factor_changed_map;
static iav_stream_id_t keep_fps_in_ss_changed_map;
static iav_stream_id_t framerate_factor_sync_map = 0;

// frame drop setting
static iav_frame_drop_info_ex_t frame_drop_info[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t frame_drop_info_changed_map;

static qp_limit_params_t qp_limit[MAX_ENCODE_STREAM_NUM];
static iav_stream_id_t qp_limit_changed_map;

static int intra_mb_rows[MAX_ENCODE_STREAM_NUM];
static int intra_mb_rows_changed_map;

static qp_matrix_params_t qp_matrix[MAX_ENCODE_STREAM_NUM];
static int qp_matrix_changed_map;

static h264_enc_param_t h264_enc_param[MAX_ENCODE_STREAM_NUM];
static int h264_enc_param_changed_map;

//system information display
static int show_iav_state_flag = 0;
static int show_encode_stream_info_flag = 0;
static int show_encode_config_flag = 0;
static int show_source_buffer_info_flag = 0;
static int show_resource_limit_info_flag = 0;
static int show_encode_mode_cap_flag = 0;
static int show_iav_driver_info_flag = 0;
static int show_chip_info_flag = 0;

//misc settings
static iav_stream_id_t restart_stream_id = 0;
static int dump_idsp_bin_flag = 0;

//preview A framerate divisor
static u8 prev_a_framerate_div = 1;
static u8 prev_a_framerate_div_changed = 0;

static vcap_param_t vcap_param;
static u8 vcap_param_changed = 0;

//IAV debug flag
static debug_param_t iav_debug;
static u8 iav_debug_flag = 0;

static int dsp_stat_init_flag = 0;

struct encode_resolution_s {
	const char 	*name;
	int		width;
	int		height;
}
__encode_res[] =
{
	{"4kx3k",4016,3016},
	{"4kx2k",4096,2160},
	{"4k",4096,2160},
	{"qfhd",3840,2160},
	{"qhd",2560,1440},
	{"5m", 2592, 1944},
	{"5m_169", 2976, 1674},
	{"3m", 2048, 1536},
	{"3m_169", 2304, 1296},
	{"1080p", 1920, 1080},
	{"720p", 1280, 720},
	{"480p", 720, 480},
	{"576p", 720, 576},
	{"4sif", 704, 480},
	{"4cif", 704, 576},
	{"xga", 1024, 768},
	{"vga", 640, 480},
	{"wvga", 800, 480},
	{"fwvga", 854, 480},	//a 16:9 style
	{"cif", 352, 288},
	{"sif", 352, 240},
	{"qvga", 320, 240},
	{"qwvga", 400, 240},
	{"qcif", 176, 144},
	{"qsif", 176, 120},
	{"qqvga", 160, 120},
	{"svga", 800, 600},
	{"sxga", 1280, 1024},
	{"480i", 720, 480},
	{"576i", 720, 576},
	{"1080i", 1920, 1080},

	{"", 0, 0},

	{"1920x1080", 1920, 1080},
	{"1600x1200", 1600, 1200},
	{"1440x1080", 1440, 1080},
	{"1366x768", 1366, 768},
	{"1280x1024", 1280, 1024},
	{"1280x960", 1280, 960},
	{"1280x720", 1280, 720},
	{"1024x768", 1024, 768},
	{"720x480", 720, 480},
	{"720x576", 720, 576},

	{"", 0, 0},

	{"704x480", 704, 480},
	{"704x576", 704, 576},
	{"640x480", 640, 480},
	{"352x288", 352, 288},
	{"352x256", 352, 256},	//used for interlaced MJPEG 352x256 encoding ( crop to 352x240 by app)
	{"352x240", 352, 240},
	{"320x240", 320, 240},
	{"176x144", 176, 144},
	{"176x120", 176, 120},
	{"160x120", 160, 120},

	//vertical video resolution
	{"480x640", 480, 640},
	{"480x854", 480, 854},

	//for preview size only to keep aspect ratio in preview image for different VIN aspect ratio
	{"16_9_vin_ntsc_preview", 720, 360},
	{"16_9_vin_pal_preview", 720, 432},
	{"4_3_vin_ntsc_preview", 720, 480},
	{"4_3_vin_pal_preview", 720, 576},
	{"5_4_vin_ntsc_preview", 672, 480},
	{"5_4_vin_pal_preview", 672, 576 },
	{"ntsc_vin_ntsc_preview", 720, 480},
	{"pal_vin_pal_preview", 720, 576},
};

#define	NO_ARG		0
#define	HAS_ARG		1

enum numeric_short_options {
	// Option bases
	SYSTEM_OPTIONS_BASE = 1000,
	VIN_OPTIONS_BASE = 1100,
	VOUT_OPTIONS_BASE = 1300,
	PREVIEW_OPTIONS_BASE = 1500,
	ENCODING_OPTIONS_BASE = 1700,
	MISC_OPTIONS_BASE = 1900,
	DEBUG_OPTIONS_BASE = 2000,

	// System
	SYSTEM_IDLE = SYSTEM_OPTIONS_BASE,
	SPECIFY_SHARPEN_B,
	SPECIFY_ENCODE_MODE,
	SPECIFY_ROTATE_POSSIBLE,
	DUMP_IDSP_CONFIG,
	SPECIFY_HDR_EXPOSURE_NUM,
	SPECIFY_CMD_READ_DELAY,
	SPECIFY_EIS_DELAY_COUNT,
	SPECIFY_RAW_CAPTURE,
	SPECIFY_ENC_FROM_RAW,
	SPECIFY_YUV_INPUT_ENHANCED,

	// VIN
	VIN_NUMVERIC_SHORT_OPTIONS,

	// Vout
	VOUT_NUMERIC_SHORT_OPTIONS,

	// Preview
	NO_PREVIEW = PREVIEW_OPTIONS_BASE,

	// Encoding
	ENCODING_RESTART = ENCODING_OPTIONS_BASE,
	ENCODING_OFFSET,
	STREAM_MAX_SIZE,
	SPECIFY_FORCE_IDR,
	FRAME_FACTOR,
	FRAME_FACTOR_SYNC,
	PREV_A_FRAMERATE_DIV,
	SPECIFY_GOP_IDR,
	SPECIFY_GOP_MODEL,
	BITRATE_CONTROL,
	SPECIFY_CBR_BITRATE,
	SPECIFY_VBR_BITRATE,
	DEBLOCKING_ALPHA,
	DEBLOCKING_BETA,
	DEBLOCKING_ENABLE,
	SPECIFY_PROFILE_LEVEL,
	SPECIFY_QMATRIX_4X4,
	INTLC_IFRAME,
	SPECIFY_AU_TYPE,
	SPECIFY_HFLIP,
	SPECIFY_VFLIP,
	SPECIFY_ROTATE,
	SPECIFY_CHROME_FORMAT,
	SPECIFY_NEGATIVE_OFFSET_DISABLE,
	SPECIFY_SUB_BUFFER,
	SPECIFY_BUFFER_TYPE,
	SPECIFY_BUFFER_SIZE,
	SPECIFY_BUFFER_MAX_SIZE,
	SPECIFY_BUFFER_INPUT_SIZE,
	SPECIFY_BUFFER_INPUT_OFFSET,
	SPECIFY_BUFFER_UNWARP,
	SPECIFY_OSD_MIXER,
	CHANGE_QP_LIMIT_I,
	CHANGE_QP_LIMIT_P,
	CHANGE_QP_LIMIT_B,
	CHANGE_QP_LIMIT_Q,
	CHANGE_ADAPT_QP,
	CHANGE_I_QP_REDUCE,
	CHANGE_P_QP_REDUCE,
	CHANGE_Q_QP_REDUCE,
	CHANGE_LOG_Q_NUM,
	CHANGE_SKIP_FRAME_MODE,
	CHANGE_INTRA_MB_ROWS,
	CHANGE_QP_MATRIX_DELTA,
	CHANGE_QP_MATRIX_MODE,
	SPECIFY_PANIC_RATIO,
	SPECIFY_MAX_GOP_M,
	SPECIFY_READOUT_MODE,
	SPECIFY_2X_SEARCH_RANGE,
	SPECIFY_INTRABIAS_P,
	SPECIFY_INTRABIAS_B,
	SPECIFY_VOUT_SWAP,
	SPECIFY_PM_TYPE,
	SPECIFY_P_SKIP_BIAS,
	SPECIFY_MAX_ENC_NUM,
	SPECIFY_MAX_CFA_VIN_NUM,
	SPECIFY_EXTRA_DRAM_BUF,
	SPECIFY_HWARP_BYPASS_POSSIBLE,
	SPECIFY_DRAM_BUFFER_MAX_FRAME_NUM,
	SPECIFY_VSKIP_BEFORE_ENCODE,
	SPECIFY_FRAME_DROP,
	SPECIFY_EXTRA_2X_ZOOM,
	SPECIFY_CPB_UNDERFLOW_RATIO,
	SPECIFY_ZMV_THRESHOLD,

	// Misc
	SHOW_SYSTEM_STATE = MISC_OPTIONS_BASE,
	SHOW_ENCODE_CONFIG,
	SHOW_STREAM_INFO,
	SHOW_BUFFER_INFO,
	SHOW_RESOURCE_INFO,
	SHOW_DRIVER_INFO,
	SHOW_CHIP_INFO,
	SHOW_ENCODE_MODE_CAP,
	SHOW_ALL_INFO,

	// Debug
	SPECIFY_CHECK_DISABLE = DEBUG_OPTIONS_BASE,
	SPECIFY_DEBUG_CHIP_ID,
	SPECIFY_KEEP_FPS_IN_SS,
	SPECIFY_KERNEL_PRINT_ENABLE,
	SPECIFY_MAX_VIN_STAT_LINES_TOP,
	SPECIFY_MAX_VIN_STAT_LINES_BOT,
	SPECIFY_VIN_CAP_OFFSET,
	SPECIFY_VIDEO_FREEZE,
	SPECIFY_MAX_CHROMA_NOISE_SHIFT,
	SPECIFY_MULTISTREAMS_START,
	SPECIFY_MULTISTREAMS_STOP,
	SPECIFY_DSP_NONCACHED,
	SPECIFY_VCA_BUFFER,
	SPECIFY_MAP_DSP_PARTITION,
	SPECIFY_ENC_DUMMY_LATENCY,
	SPECIFY_MODE_BIAS_I4,
	SPECIFY_MODE_BIAS_I16,
	SPECIFY_MODE_BIAS_INTER8,
	SPECIFY_MODE_BIAS_INTER16,
	SPECIFY_MODE_BIAS_DIRECT,
	SPECIFY_LONG_P_INTERVAL,
	SPECIFY_GOP_REF_P,
	SPECIFY_VOUT_B_LETTER_BOXING_ENABLE,
};

static struct option long_options[] = {
	{"stream",	HAS_ARG,	0,	'S' },   // -Sx means stream no. X
	{"stream_A",	NO_ARG,		0,	'A' },   // -A xxxxx    means all following configs will be applied to stream A
	{"stream_B",	NO_ARG,		0,	'B' },
	{"stream_C",	NO_ARG,		0,	'C' },
	{"stream_D",	NO_ARG,		0,	'D' },
	{"stream_E",	NO_ARG,		0,	'E' },
	{"stream_F",	NO_ARG,		0,	'F' },
	{"stream_G",	NO_ARG,		0,	'G' },
	{"stream_H",	NO_ARG,		0,	'H' },

	{"h264", 		HAS_ARG,	0,	'h'},
	{"mjpeg",	HAS_ARG,	0,	'm'},
	{"none",		NO_ARG,		0,	'n'},
	{"src",	HAS_ARG,	0,	'b' },	//encode source buffer
	{"duration",	HAS_ARG,	0,	'd'},
	{"offset",	HAS_ARG,	0,	ENCODING_OFFSET },	//encoding offset
	{"smaxsize",	HAS_ARG,	0,	STREAM_MAX_SIZE},	//encoding max size

	//immediate action, configure encode stream on the fly
	{"encode",	NO_ARG,		0,	'e'},		//start encoding
	{"stop",		NO_ARG,		0,	's'},		//stop encoding
	{"start-multi",	HAS_ARG,	0,	SPECIFY_MULTISTREAMS_START},
	{"stop-multi",	HAS_ARG,	0,	SPECIFY_MULTISTREAMS_STOP},
	{"force-idr",	NO_ARG,		0,	SPECIFY_FORCE_IDR },
	{"restart",		NO_ARG,		0,	ENCODING_RESTART },			//immediate stop and start encoding
	{"frame-drop",		HAS_ARG,	0,	SPECIFY_FRAME_DROP},
	{"frame-factor",	HAS_ARG,	0,	FRAME_FACTOR },
	{"frame-factor-sync",	HAS_ARG,	0,	FRAME_FACTOR_SYNC },
	{"prev-a-frame-divisor",	HAS_ARG,	0,	PREV_A_FRAMERATE_DIV },
	{"intrabias-p",	HAS_ARG,	0,	SPECIFY_INTRABIAS_P },
	{"intrabias-b",	HAS_ARG,	0,	SPECIFY_INTRABIAS_B },
	{"p-skip-bias",	HAS_ARG,	0,	SPECIFY_P_SKIP_BIAS },
	{"cpb-ratio",		HAS_ARG,	0,	SPECIFY_CPB_UNDERFLOW_RATIO},
	{"zmv-threshold",	HAS_ARG,	0,	SPECIFY_ZMV_THRESHOLD},
	{"mode-bias-I4",	HAS_ARG,	0,	SPECIFY_MODE_BIAS_I4},
	{"mode-bias-I16",	HAS_ARG,	0,	SPECIFY_MODE_BIAS_I16},
	{"mode-bias-Inter8",	HAS_ARG,	0,	SPECIFY_MODE_BIAS_INTER8},
	{"mode-bias-Inter16",	HAS_ARG,	0,	SPECIFY_MODE_BIAS_INTER16},
	{"mode-bias-Direct",	HAS_ARG,	0,	SPECIFY_MODE_BIAS_DIRECT},

	//H.264 encode configurations
	{"M",		HAS_ARG,	0,	'M' },
	{"N",		HAS_ARG,	0,	'N'},
	{"idr",		HAS_ARG,	0,	SPECIFY_GOP_IDR},
	{"gop",		HAS_ARG,	0,	SPECIFY_GOP_MODEL},
	{"bc",		HAS_ARG,	0,	BITRATE_CONTROL},
	{"bitrate",	HAS_ARG,	0,	SPECIFY_CBR_BITRATE},
	{"vbr-bitrate",	HAS_ARG,	0,	SPECIFY_VBR_BITRATE},
	{"qp-limit-i", 	HAS_ARG, 0, CHANGE_QP_LIMIT_I},
	{"qp-limit-p", 	HAS_ARG, 0, CHANGE_QP_LIMIT_P},
	{"qp-limit-b", 	HAS_ARG, 0, CHANGE_QP_LIMIT_B},
	{"qp-limit-q",		HAS_ARG, 0, CHANGE_QP_LIMIT_Q},
	{"adapt-qp",		HAS_ARG, 0, CHANGE_ADAPT_QP},
	{"i-qp-reduce",	HAS_ARG, 0, CHANGE_I_QP_REDUCE},
	{"p-qp-reduce",	HAS_ARG, 0, CHANGE_P_QP_REDUCE},
	{"q-qp-reduce",	HAS_ARG, 0, CHANGE_Q_QP_REDUCE},
	{"log-q-num",	HAS_ARG, 0, CHANGE_LOG_Q_NUM},
	{"skip-frame-mode",	HAS_ARG, 0, CHANGE_SKIP_FRAME_MODE},
	{"intra-mb-rows",	HAS_ARG, 0, CHANGE_INTRA_MB_ROWS},
	{"qm-delta",	HAS_ARG,	0,	CHANGE_QP_MATRIX_DELTA},
	{"qm-mode",	HAS_ARG,	0,	CHANGE_QP_MATRIX_MODE},

	{"deblocking-alpha",	HAS_ARG,	0,	DEBLOCKING_ALPHA},
	{"deblocking-beta",	HAS_ARG,	0,	DEBLOCKING_BETA},
	{"deblocking-disable",	HAS_ARG,	0,	DEBLOCKING_ENABLE},
	{"profile",	HAS_ARG,	0,	SPECIFY_PROFILE_LEVEL},
	{"qmatrix-4x4",	HAS_ARG,	0,	SPECIFY_QMATRIX_4X4},
	{"search-range-2x",	HAS_ARG,	0,	SPECIFY_2X_SEARCH_RANGE},
	{"read-mode",	HAS_ARG,	0,	SPECIFY_READOUT_MODE},

	{"intlc_iframe",		HAS_ARG,	0,	INTLC_IFRAME},
	{"long-p-interval",		HAS_ARG,	0,	SPECIFY_LONG_P_INTERVAL},

	//panic mode
	{"panic-ratio", HAS_ARG, 0, SPECIFY_PANIC_RATIO},

	//H.264 syntax options
	{"au-type",		HAS_ARG,	0,	SPECIFY_AU_TYPE},

	//MJPEG encode configurations
	{"quality",	HAS_ARG,	0,	'q'}, //quality factor

	//common encode configurations
	{"hflip",		HAS_ARG,	0,	SPECIFY_HFLIP}, //horizontal flip
	{"vflip",		HAS_ARG,	0,	SPECIFY_VFLIP}, //vertical flip
	{"rotate",		HAS_ARG,	0,	SPECIFY_ROTATE}, //clockwise rotate
	{"chrome",	HAS_ARG,	0,	SPECIFY_CHROME_FORMAT}, // chrome format
	{"nega-offset-disable",  HAS_ARG, 0, SPECIFY_NEGATIVE_OFFSET_DISABLE},

	//vin configurations
	VIN_LONG_OPTIONS()

	{"vin-cap-offset",	HAS_ARG,	0,	SPECIFY_VIN_CAP_OFFSET},
	{"nopreview",	NO_ARG,		0,	NO_PREVIEW},
	{"video-freeze",	HAS_ARG,	0,	SPECIFY_VIDEO_FREEZE},
	{"yuv-input-enhanced",	HAS_ARG,	0,	SPECIFY_YUV_INPUT_ENHANCED},

	//system state
	{"idle",		NO_ARG,		0,	SYSTEM_IDLE},			//put system to IDLE  (turn off all encoding )
	{"sharpen-b",	HAS_ARG,	0,	SPECIFY_SHARPEN_B},
	{"enc-mode",	HAS_ARG,	0,	SPECIFY_ENCODE_MODE},
	{"max-enc-num",	HAS_ARG,	0,	SPECIFY_MAX_ENC_NUM},
	{"enc-rotate-possible",	HAS_ARG,	0,	SPECIFY_ROTATE_POSSIBLE},
	{"hdr-expo",	HAS_ARG,	0,	SPECIFY_HDR_EXPOSURE_NUM},
	{"cfa-vin",	HAS_ARG,	0,	SPECIFY_MAX_CFA_VIN_NUM},
	{"hwarp-bypass-possible",	HAS_ARG,	0,	SPECIFY_HWARP_BYPASS_POSSIBLE},
	{"cmd-read-dly",	HAS_ARG,	0,	SPECIFY_CMD_READ_DELAY},
	{"eis-delay-count",	HAS_ARG,	0,	SPECIFY_EIS_DELAY_COUNT},
	{"raw-capture",	HAS_ARG,	0,	SPECIFY_RAW_CAPTURE},
	{"enc-from-raw",	HAS_ARG,	0,	SPECIFY_ENC_FROM_RAW},
	{"max-vin-stats-top",	HAS_ARG,	0,	SPECIFY_MAX_VIN_STAT_LINES_TOP},
	{"max-vin-stats-bot",	HAS_ARG,	0,	SPECIFY_MAX_VIN_STAT_LINES_BOT},
	{"max-warp-in-width",	HAS_ARG,	0,	'w'},
	{"max-warp-out-width",	HAS_ARG,	0,	'W'},
	{"max-chroma-noise-shift",	HAS_ARG,	0,	SPECIFY_MAX_CHROMA_NOISE_SHIFT},
	{"vout-swap",	HAS_ARG,		0,	SPECIFY_VOUT_SWAP},
	{"vskip-before-enc", HAS_ARG,	0,	SPECIFY_VSKIP_BEFORE_ENCODE},
	{"pm-type", HAS_ARG,	0,	SPECIFY_PM_TYPE},
	{"extra-2x-zm",	HAS_ARG,	0,	SPECIFY_EXTRA_2X_ZOOM},
	{"vca-buf", HAS_ARG,	0,	SPECIFY_VCA_BUFFER},
	{"map-dsp-partition", HAS_ARG,	0,	SPECIFY_MAP_DSP_PARTITION},
	{"debug-enc-dummy-latency", HAS_ARG,	0,	SPECIFY_ENC_DUMMY_LATENCY},
	{"debug-gop-ref-p", HAS_ARG,	0,	SPECIFY_GOP_REF_P},

	//Vout
	VOUT_LONG_OPTIONS()

	//IDSP related
	{"dump-idsp-cfg",	NO_ARG,		0,	DUMP_IDSP_CONFIG},

	//source buffer param
	{"premain",	NO_ARG, 0,	'P'},
	{"main-buffer",	NO_ARG,	0,	'X'},	//main source buffer
	{"sub-buffer",	NO_ARG,		0,	SPECIFY_SUB_BUFFER },   // -Bx means sub buffer no. X
	{"second-buffer",	NO_ARG,	0,	'Y'},
	{"third-buffer",	NO_ARG,	0,	'J'},	//
	{"fourth-buffer",	NO_ARG,	0,	'K'},	//
	{"dram-buffer",	HAS_ARG,	0,	'Z'},
	{"btype",			HAS_ARG,	0,	SPECIFY_BUFFER_TYPE},
	{"bsize",			HAS_ARG,	0,	SPECIFY_BUFFER_SIZE},
	{"bmaxsize",		HAS_ARG,	0,	SPECIFY_BUFFER_MAX_SIZE},
	{"binsize",		HAS_ARG,	0,	SPECIFY_BUFFER_INPUT_SIZE},
	{"binoffset",	HAS_ARG,	0,	SPECIFY_BUFFER_INPUT_OFFSET},
	{"bunwarp",		HAS_ARG,	0,	SPECIFY_BUFFER_UNWARP},
	{"extra-buf",	HAS_ARG,	0,	SPECIFY_EXTRA_DRAM_BUF},

	{"dframe",	HAS_ARG,	0,	SPECIFY_DRAM_BUFFER_MAX_FRAME_NUM},

	//OSD blending
	{"osd-mixer",		HAS_ARG,	0,	SPECIFY_OSD_MIXER},
	{"vout-box-enable",	HAS_ARG,	0,	SPECIFY_VOUT_B_LETTER_BOXING_ENABLE},

	//show info options
	{"show-system-state",	NO_ARG,	0,	SHOW_SYSTEM_STATE},		//show system state
	{"show-encode-config",	NO_ARG,	0,	SHOW_ENCODE_CONFIG},
	{"show-stream-info",	NO_ARG,	0,	SHOW_STREAM_INFO},
	{"show-buffer-info",	NO_ARG,	0,	SHOW_BUFFER_INFO},
	{"show-resource-info",	NO_ARG,	0,	SHOW_RESOURCE_INFO},
	{"show-driver-info",	NO_ARG,	0,	SHOW_DRIVER_INFO},
	{"show-chip-info",		NO_ARG, 0,	SHOW_CHIP_INFO},
	{"show-enc-mode-cap",	NO_ARG,	0,	SHOW_ENCODE_MODE_CAP},
	{"show-all-info",		NO_ARG, 0,	SHOW_ALL_INFO},

	{"dsp-noncached",		HAS_ARG, 0,	SPECIFY_DSP_NONCACHED},
	{"check-disable",		HAS_ARG, 0,	SPECIFY_CHECK_DISABLE},
	{"debug-chip-id",	HAS_ARG,	0,	SPECIFY_DEBUG_CHIP_ID},
	{"keep-fps-in-ss",	HAS_ARG,	0,	SPECIFY_KEEP_FPS_IN_SS},
	{"kernel-print-enable",	HAS_ARG, 0,	SPECIFY_KERNEL_PRINT_ENABLE},

	{0, 0, 0, 0}
};

static const char *short_options = "ABb:Cc:Dd:eEFf:GHh:JKm:nM:N:q:i:PS:sV:v:W:w:XYZ:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"0~15", "\tspecify stream ID\n"},
	{"", "\t\tconfig for stream A"},
	{"", "\t\tconfig for stream B"},
	{"", "\t\tconfig for stream C"},
	{"", "\t\tconfig for stream D"},
	{"", "\t\tconfig for stream E"},
	{"", "\t\tconfig for stream F"},
	{"", "\t\tconfig for stream G"},
	{"", "\t\tconfig for stream H\n"},

	{"resolution", "\tenter H.264 encoding resolution"},
	{"resolution", "\tenter MJPEG encoding resolution"},
	{"", "\t\tset stream encode type to NONE"},
	{"0~3", "\t\tsource buffer 0~3" },
	{"0~1024", "\tencode duration. The number of frames will be encoded."},
	{"axb", "\tcut out encoding offset from source buffer"},
	{"resolution", "specify stream max size for system resouce limit\n"},

	//immediate action, configure encode stream on the fly
	{"", "\t\tstart encoding for current stream"},
	{"", "\t\tstop encoding for current stream"},
	{"m~n", "\tstart encoding for multiple streams. Maximum range is 0~15."},
	{"m~n", "\tstop encoding for multiple streams. Maximum range is 0~15."},
	{"", "\t\tforce IDR at once for current stream"},
	{"", "\t\trestart encoding for current stream"},
	{"0~300", "\thow many frame drops"},
	{"1~255/1~255", "change frame rate interval for current stream, numerator/denominator"},
	{"1~255/1~255", "Simutaneously change frame rate interval for current streams during encoding, numerator/denominator"},
	{"1~255", "set preview A framerate divisor, to match with DVOUT framerate"},
	{"1~4000", "set intrabias for current stream P frame"},
	{"1~4000", "set intrabias for current stream B frame"},
	{"1~4000", "set bias for P-skip of current stream, larger value means higher possibility to encode frame as P skip"},
	{"1~255/1~255", "set CPB buffer underflow ratio"},
	{"0~255", "set zmv threshold for current stream, value 0 means disable this"},
	{"-4~5", "set mode bias of I4."},
	{"-4~5", "set mode bias of I16.\n"},
	{"-4~5", "set mode bias of Inter8."},
	{"-4~5", "set mode bias of Inter16.\n"},
	{"-4~5", "set mode bias of Direct.\n"},

	//H.264 encode configurations
	{"1~8", "\t\tH.264 GOP parameter M"},
	{"1~255", "\t\tH.264 GOP parameter N, must be multiple of M, can be changed during encoding"},
	{"1~128", "\thow many GOP's, an IDR picture should happen, can be changed during encoding"},
	{"0~4", "\t\tGOP model, 0 for simple, 1 for advanced, 2 for 2 level SVCT, 3 for 3 level SVCT, 4 for 4 level SVCT"},
	{"cbr|vbr|cbr-quality|vbr-quality|cbr2", "\tbitrate control method"},
	{"value", "\tset cbr average bitrate, can be changed during encoding"},
	{"min~max", "set vbr bitrate range, can be changed during encoding"},
	{"0~51", "\tset I-frame qp limit range, 0:auto 1~51:qp limit range"},
	{"0~51", "\tset P-frame qp limit range, 0:auto 1~51:qp limit range"},
	{"0~51", "\tset B-frame qp limit range, 0:auto 1~51:qp limit range"},
	{"0~51", "\tset Q-frame qp limit range, 0:quto 1~51:qp limit range"},
	{"0~4", "\tset strength of adaptive qp"},
	{"1~10", "\tset diff of I QP less than P QP"},
	{"1~5", "\tset diff of P QP less than B QP"},
	{"1~10", "\tset diff of Q QP less than C QP"},
	{"0~10", "\tset log Q num minus 1"},
	{"0|1|2", "0: disable, 1: skip based on CPB size, 2: skip based on target bitrate and max QP"},
	{"0~n", "set intra refresh MB rows number, default value is 0, which means disable"},
	{"-50~50", "\tset QP Matrix delta value in format of 'd0,d1,d2,d3'"},
	{"0~4", "\tset QP Matrix mode, 0: default; 1: skip left region; 2: skip right; 3: skip top; 4: skip bottom"},

	{"-6~6", "deblocking-alpha"},
	{"-6~6", "deblocking-beta"},
	{"0|1", "deblocking-disable, 0 is default value and enable deblocking filter"},
	{"0|1|2", "\tH264 profile level, 0 is baseline, 1 is main, 2 is high, default is 1"},
	{"0|1", "\tDisable / Enable Q-matrix 4x4 for high profile only. Default is 0."},
	{"0|1", "Enable 2X vertical motion vector search range, default is 0"},
	{"1|0", "\tset read encode info protocol, 0:polling mode, 1:coded bits interrupt mode. Default is 1"},

	{"0|1", "\tInterlaced Encoding 0:default, 1: force two fields be I-picture\n"},

	{"0~63", "\tLong term p interval, 1.<=1 -- Every image predicts from the I picture; 2.The pictures that none multiple of the value, predict from the previous image.\n"},

	{"1~255/1~255", "change panic ratio for H264 stream, default is 30/1. The larger value is, the less frequence to trigger panic mode.\n"},

	//H.264 syntax options
	{"0~4", "\t0: No AUD, No SEI; 1: AUD before SPS, PPS, with SEI; " \
		"2: AUD after SPS, PPS, with SEI; 3: No AUD, with SEI; " \
		"4: AUD with PPS and SEI in every frame.\n"},

	//MJPEG encode configurations
	{"quality", "\tset jpeg/mjpeg encode quality"},

	//common encode configurations
	{"0|1", "\tdsp horizontal flip for current stream"},
	{"0|1", "\tdsp vertical flip for current stream"},
	{"0|1", "\tdsp clockwise rotate for current stream"},
	{"0|1", "\tset chrome format, 0 for YUV420 format, 1 for monochrome\n"},
	{"0|1", "\tdsp negative offset disable for current stream\n"},
	//vin configurations
	VIN_PARAMETER_HINTS()

	{"", "\tspecify the VIN capture window offset"},
	{"", "\t\tdo not enter preview"},
	{"0|1", "\tspecify video freeze enable. Default is 0.\n"},
	{"0|1", "\tEnhance encoding performance to QFHDP30 when yuv input in enc-mode 6. Default is 0.\n"},

	//system state
	{"", "\t\tput system to IDLE  (turn off all encoding)"},
	{"0|1", "\tEnable sharpen B on Non-run time, default is on"},
	{"0~10", "\tChoose encode mode, default is 0." \
			"\n\t\t\t\t0: full frame rate mode; 1: multi region warp mode;" \
			"\n\t\t\t\t2: high mega pixel mode; 3: calibration mode;" \
			"\n\t\t\t\t4: hdr frame interleaved mode; 5: hdr line interleaved mode;" \
			"\n\t\t\t\t6: high MP (low delay) mode; 7: full FPS (low delay) mode;" \
			"\n\t\t\t\t8: multiple CFA VIN mode; 9: Ultra Low Light (ULL) mode\n" \
			"\n\t\t\t\t10: high MP warp mode"},
	{"4~8", "\tSet maximum encode number. Default is 4 for all modes. Only mode 0 supports up to 8 streams."},
	{"0|1", "Disable/Enable rotate possibility on Non-run time. Default is 0. 0: disable, 1: enable"},
	{"1~4", "\tChoose exposure number in HDR line interleaved mode"},
	{"2~4", "\tChoose CFA VIN number in multiple VIN mode"},
	{"0|1", "\tSet H-warp bypass possibility."},
	{"", "\tSet command read delay for EIS mode"},
	{"0~2", "\tSet eis delay count for EIS mode"},
	{"0|1", "\tDisable/Enable raw capture on non-run time. Default is 0. 0: disable, 1: enable."},
	{"0|1", "\tDisable/Enable encoding from RAW on non-run time. Default is 0. 0: disable, 1: enable."},
	{"0~15", "Specify the max VIN statistics top lines. Default is 0. It only works in some modes."},
	{"0~15", "Specify the max VIN statistics bottome lines. Default is 0. It only works in some modes."},
	{"", "\tSet maximum input width of warp regions"},
	{"", "\tSet maximum output width of warp regions"},
	{"0~2", "\tSet maximum chroma noise shift. 0: 32, 1: 64, 2:128"},
	{"0|1", "\tSwap VOUT and preview. 0: preview A for VOUTA, preview B for VOUTB;"
			"\n\t\t\t\t1: preview A for VOUTB, preview B for VOUTA.\n"},
	{"3~5", "Set the skipped Vsync number before encoding"},
	{"0|1", "\tEnable MCTF privacy mask. 0 is HDR based PM, 1 is MCTF based PM."},
	{"0|1", "\tEnable / Disable extra 2X zoom for 2nd buffer. Default is enabled."},
	{"0|1", "\tEnable / Disable VCA buffer. Default is disabled.\n"},
	{"0|1", "\tEnable / Disable Map DSP Partition. Default is disabled.\n"},
	{"0~4", "\tSpecify the latancy for encode dummy"},
	{"0~2", "\tSpecify the reference P num. Not support now."},

	//VOUT
	VOUT_PARAMETER_HINTS()

	//IDSP related
	{"", "\tdump iDSP config for debug purpose\n"},

	//source buffer param
	{"", "\t\tconfig for Pre-Main source buffer"},
	{"", "\tconfig for Main source buffer\n"},
	{"1~3", "\tconfig for Sub source buffer"},
	{"", "\tconfig for Second source buffer"},
	{"", "\tconfig for Third source buffer"},
	{"", "\tconfig for Fourth source buffer"},
	{"0", "\tconfig for source DRAM buffer id"},
	{"enc|prev|off", "specify source buffer type"},
	{"resolution", "\tspecify source buffer resolution, set 0x0 to disable it"},
	{"resolution", "specify source buffer max resolution, set 0x0 to cleanly disable it"},
	{"resolution", "specify source buffer input window size, so as to crop before downscale"},
	{"resolution", "specify source buffer input window offset, so as to crop before downscale"},
	{"0|1", "\tspecify source buffer unwarp option in warp mode, 0: after warping; 1: before warping. Default is 0."},
	{"-4~20", "\tSet extra DRAM buffer number for extremely heavy load cases.\n"},

	{"1~5", "\tspecify max frame num for encode dram buffer pool.\n"},

	//OSD blending
	{"off|a|b", "OSD blending mixer, off: disable, a: select from VOUTA, b: select from VOUTB\n"},
	{"0|1", "Vout B letter box enable\n"},
	//show info options
	{"", "\tShow system state"},
	{"", "\tshow stream(H.264/MJPEG) encode config"},
	{"", "\tShow stream format , size, info & state"},
	{"", "\tShow source buffer info & state"},
	{"", "\tShow codec resource limit info"},
	{"", "\tShow IAV driver info"},
	{"", "\tShow chip info"},
	{"", "\tShow encode mode capability"},
	{"", "\tShow all info \n"},

	{"0|1", "Set DSP memory as non-cached type. Default is 0."},
	{"0|1", "Disable IAV kernel protection. Default is 0."},
	{"-1~5", "Specify Chip ID in debug mode. 0: S2_33, 1: S2_55, 2: S2_66,"\
			"\n\t\t\t\t3: S2_88, 4: S2_99, 5: S2_22  -1: disable this debug option"},
	{"0|1", "Keep stream fps fixed when slow shutter enable. Default is 0.\n"},
	{"0|1", "Enable IAV kernel print. Default is 1.\n"},

};


int get_arbitrary_resolution(const char *name, int *width, int *height)
{
	sscanf(name, "%dx%d", width, height);
	return 0;
}

int get_encode_resolution(const char *name, int *width, int *height)
{
	int i;

	for (i = 0; i < sizeof(__encode_res) / sizeof(__encode_res[0]); i++)
		if (strcmp(__encode_res[i].name, name) == 0) {
			*width = __encode_res[i].width;
			*height = __encode_res[i].height;
			printf("%s resolution is %dx%d\n", name, *width, *height);
			return 0;
		}
	get_arbitrary_resolution(name, width, height);
	printf("resolution %dx%d\n", *width, *height);
	return 0;
}

//first second value must in format "x~y" if delimiter is '~'
static int get_two_unsigned_int(const char *name, u32 *first, u32 *second, char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("range should be like a%cb \n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator - name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1,  name + strlen(name) -separator);
	*second = atoi(tmp_string);

//	printf("input string %s,  first value %d, second value %d \n",name, *first, *second);
	return 0;
}

static int get_multi_s8_arg(char * optarg, s8 *argarr, int argcnt)
{
	int i;
	char *delim = ",:";
	char *ptr;

	ptr = strtok(optarg, delim);
	argarr[0] = atoi(ptr);
	printf("[0], : %d]\n", argarr[0]);

	for (i = 1; i < argcnt; i++) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i] = atoi(ptr);
	}
	if (i < argcnt)
		return -1;

	return 0;
}

static const char *get_state_str(int state)
{
	switch (state) {
	case IAV_STATE_PREVIEW:	return "preview";
	case IAV_STATE_ENCODING: return "encoding";
	case IAV_STATE_STILL_CAPTURE: return "still capture";
	case IAV_STATE_DECODING: return "decoding";
	case IAV_STATE_IDLE: return "idle";
	case IAV_STATE_INIT: return "init";
	default: return "???";
	}
}

static const char *get_dsp_op_mode_str(int mode)
{
	switch (mode) {
	case 0:	return "encode mode";
	case 1:	return "decode mode";
	case 2:	return "reset mode";
	default: return "???";
	}
}

static const char *get_dsp_encode_state_str(int state)
{
	switch (state) {
	case 0:	return "idle";
	case 1:	return "busy";
	case 2:	return "pause";
	case 3:	return "flush";
	case 4:	return "unknown";
	default: return "???";
	}
}

static const char *get_dsp_encode_mode_str(int mode)
{
	switch (mode) {
	case 0:	return "stop";
	case 1:	return "video";
	case 2:	return "sjpeg";
	case 3:	return "mjpeg";
	case 4:	return "fast3a";
	case 5:	return "rjpeg";
	case 6:	return "timer";
	case 7:	return "unknown";
	default: return "???";
	}
}

static const char *get_dsp_decode_state_str(int state)
{
	switch (state) {
	case 0:	return "idle";
	case 1:	return "h264dec";
	case 2:	return "h264dec idle";
	case 3:	return "transit h264dec to idle";
	case 4:	return "transit h264dec to h264dec idle";
	case 5:	return "jpeg still";
	case 6:	return "transit jpeg still to idle";
	case 7:	return "multiscene";
	case 8:	return "transit multiscene to idle";
	case 9:	return "unknown";
	default: return "???";
	}
}

static const char *get_dsp_decode_mode_str(int mode)
{
	switch (mode) {
	case 0:	return "stopped";
	case 1:	return "idle";
	case 2:	return "jpeg";
	case 3:	return "h.264";
	case 4:	return "multi";
	default: return "???";
	}
}

int get_bitrate_control(const char *name)
{
	if (strcmp(name, "cbr") == 0)
		return IAV_CBR;
	else if (strcmp(name, "vbr") == 0)
		return IAV_VBR;
	else if (strcmp(name, "cbr-quality") == 0)
		return IAV_CBR_QUALITY_KEEPING;
	else if (strcmp(name, "vbr-quality") == 0)
		return IAV_VBR_QUALITY_KEEPING;
	else if (strcmp(name, "cbr2") == 0)
		return IAV_CBR2;
	else
		return -1;
}

int get_chrome_format(const char *format, int encode_type)
{
	int chrome = atoi(format);
	if (chrome == 0) {
		return (encode_type == IAV_ENCODE_H264) ?
			IAV_H264_CHROMA_FORMAT_YUV420 :
			IAV_JPEG_CHROMA_FORMAT_YUV420;
	} else if (chrome == 1) {
		return (encode_type == IAV_ENCODE_H264) ?
			IAV_H264_CHROMA_FORMAT_MONO :
			IAV_JPEG_CHROMA_FORMAT_MONO;
	} else {
		printf("invalid chrome format : %d.\n", chrome);
		return -1;
	}
}

int get_buffer_type(const char *name)
{
	if (strcmp(name, "enc") == 0)
		return IAV_SOURCE_BUFFER_TYPE_ENCODE;
	if (strcmp(name, "prev") == 0)
		return IAV_SOURCE_BUFFER_TYPE_PREVIEW;
	if (strcmp(name, "off") == 0)
		return IAV_SOURCE_BUFFER_TYPE_OFF;

	printf("invalid buffer type: %s\n", name);
	return -1;
}

int get_osd_mixer_selection(const char *name)
{
	if (strcmp(name, "off") == 0)
		return OSD_BLENDING_OFF;
	if (strcmp(name, "a") == 0)
		return OSD_BLENDING_FROM_MIXER_A;
	if (strcmp(name, "b") == 0)
		return OSD_BLENDING_FROM_MIXER_B;

	printf("invalid osd mixer selection: %s\n", name);
	return -1;
}

void usage(void)
{
	int i;

	printf("test_encode usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");

	printf("vin mode:  ");
	for (i = 0; i < sizeof(__vin_modes)/sizeof(__vin_modes[0]); i++) {
		if (__vin_modes[i].name[0] == '\0') {
			printf("\n");
			printf("           ");
		} else
			printf("%s  ", __vin_modes[i].name);
	}
	printf("\n");

	printf("vout mode:  ");
	for (i = 0; i < sizeof(vout_res) / sizeof(vout_res[0]); i++)
		printf("%s  ", vout_res[i].name);
	printf("\n");

	printf("resolution:  ");
	for (i = 0; i < sizeof(__encode_res)/sizeof(__encode_res[0]); i++) {
		if (__encode_res[i].name[0] == '\0') {
			printf("\n");
			printf("             ");
		} else
			printf("%s  ", __encode_res[i].name);
	}
	printf("\n");
}

static inline int check_intrabias(u32 intrabias)
{
	if (intrabias > INTRABIAS_MAX || intrabias < INTRABIAS_MIN) {
		printf("Invalid value:must be %d~%d\n", INTRABIAS_MIN, INTRABIAS_MAX);
		return -1;
	}

	return 0;
}

int verify_params(void)
{
#define MIN_EXPOSURE_NUM	1

	iav_system_resource_setup_ex_t  resource_setup;
	memset(&resource_setup, 0, sizeof(resource_setup));
	resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup);

	if (system_resource_setup.encode_mode_changed) {
		if (system_resource_setup.encode_mode != IAV_ENCODE_HDR_LINE_MODE) {
			if (resource_setup.exposure_num != MIN_EXPOSURE_NUM) {
				printf("Correct hdr expo [%d -> %d] in non-hdr_line_mode\n",
					system_resource_setup.exposure_num, MIN_EXPOSURE_NUM);
				system_resource_setup.exposure_num = MIN_EXPOSURE_NUM;
				system_resource_setup.exposure_num_changed = 1;
			} else {
				if (system_resource_setup.exposure_num_changed) {
					printf("Setting hdr expo only permit in hdr_line_mode\n");
					system_resource_setup.exposure_num = MIN_EXPOSURE_NUM;
					system_resource_setup.exposure_num_changed = 0;
				}
			}
		}
	} else {
		if (resource_setup.encode_mode != IAV_ENCODE_HDR_LINE_MODE) {
			if (system_resource_setup.exposure_num_changed) {
				printf("Setting hdr expo only permit in hdr_line_mode\n");
				system_resource_setup.exposure_num = MIN_EXPOSURE_NUM;
				system_resource_setup.exposure_num_changed = 0;
			}
		}
	}

	if (resource_setup.exposure_num == system_resource_setup.exposure_num) {
		system_resource_setup.exposure_num_changed = 0;
	}

	return 0;
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int width, height, value;
	u32 min_value, max_value;
	u32 numerator, denominator;

	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {

		// handle all other options
		switch (ch) {
			case 'A':
				current_stream = 0;
				break;
			case 'B':
				current_stream = 1;
				break;
			case 'C':
				current_stream = 2;
				break;
			case 'D':
				current_stream = 3;
				break;
			case 'E':
				current_stream = 4;
				break;
			case 'F':
				current_stream = 5;
				break;
			case 'G':
				current_stream = 6;
				break;
			case 'H':
				current_stream = 7;
				break;

			case 'S':
				min_value = atoi(optarg);
				VERIFY_STREAMID(min_value);
				current_stream = min_value;
				break;

			case 'h':
					VERIFY_STREAMID(current_stream);
					if (get_encode_resolution(optarg, &width, &height) < 0)
						return -1;
					stream_encode_format[current_stream].type = IAV_ENCODE_H264;
					stream_encode_format[current_stream].type_changed = 1;

					stream_encode_format[current_stream].width = width;
					stream_encode_format[current_stream].height = height;
					stream_encode_format[current_stream].resolution_changed = 1;
					stream_encode_format_changed_map |= (1 << current_stream);
					break;

			case 'm':
					VERIFY_STREAMID(current_stream);
					if (get_encode_resolution(optarg, &width, &height) < 0)
						return -1;
					stream_encode_format[current_stream].type = IAV_ENCODE_MJPEG;
					stream_encode_format[current_stream].type_changed = 1;

					stream_encode_format[current_stream].width = width;
					stream_encode_format[current_stream].height = height;
					stream_encode_format[current_stream].resolution_changed = 1;

					stream_encode_format_changed_map |= (1 << current_stream);
					break;

			case 'n':
					VERIFY_STREAMID(current_stream);
					stream_encode_format[current_stream].type = IAV_ENCODE_NONE;
					stream_encode_format[current_stream].type_changed = 1;

					stream_encode_format_changed_map |= (1 << current_stream);
					break;

			case 'b':
					VERIFY_STREAMID(current_stream);
					stream_encode_format[current_stream].source = atoi(optarg);
					stream_encode_format[current_stream].source_changed = 1;
					stream_encode_format_changed_map |= (1 << current_stream);
					break;

			case 'd':
					VERIFY_STREAMID(current_stream);
					value = atoi(optarg);
					if ((value != IAV_ENC_DURATION_FOREVER) &&
						(value < IAV_ENC_DURATION_MIN ||
						value > IAV_ENC_DURATION_MAX)) {
						printf("Invalid enocde duration value [%d], must be "
							"in the range of [0, %d~%d].\n", value,
							IAV_ENC_DURATION_MIN, IAV_ENC_DURATION_MAX);
						return -1;
					}
					stream_encode_format[current_stream].duration = value;
					stream_encode_format[current_stream].duration_changed = 1;
					stream_encode_format_changed_map |= (1 << current_stream);
					break;

			case ENCODING_OFFSET:
					VERIFY_STREAMID(current_stream);
					if (get_encode_resolution(optarg, &width, &height) < 0)
						return -1;
					stream_encode_format[current_stream].offset_x = width;
					stream_encode_format[current_stream].offset_y = height;
					stream_encode_format[current_stream].offset_changed = 1;
					stream_encode_format_changed_map |= (1 << current_stream);
					break;

			case STREAM_MAX_SIZE:
					VERIFY_STREAMID(current_stream);
					if (get_encode_resolution(optarg, &width, &height) < 0)
						return -1;
					system_resource_setup.stream_max_width[current_stream] = width;
					system_resource_setup.stream_max_height[current_stream] = height;
					system_resource_setup.stream_max_changed_map |= (1 << current_stream);
					system_resource_setup_changed = 1;
					break;

			case 'e':
					VERIFY_STREAMID(current_stream);
					start_stream_id |= (1 << current_stream);
					break;

			case 's':
					VERIFY_STREAMID(current_stream);
					stop_stream_id |= (1 << current_stream);
					break;

			case SPECIFY_MULTISTREAMS_START:
					if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
						return -1;
					}
					if (min_value < 0 || max_value >= MAX_ENCODE_STREAM_NUM || min_value > max_value) {
						printf("Invalid stream ID range [%d~%d], it must be in the range of [0~%d].\n",
							min_value, max_value, MAX_ENCODE_STREAM_NUM - 1);
						return -1;
					}
					start_stream_id |= ((1 << (max_value + 1)) - 1U);
					start_stream_id &= ~((1 << min_value) - 1U);
					break;

			case SPECIFY_MULTISTREAMS_STOP:
					if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
						return -1;
					}
					if (min_value < 0 || max_value >= MAX_ENCODE_STREAM_NUM || min_value > max_value) {
						printf("Invalid stream ID range [%d~%d], it must be in the range of [0~%d].\n",
							min_value, max_value, MAX_ENCODE_STREAM_NUM - 1);
						return -1;
					}
					stop_stream_id |= ((1 << (max_value + 1)) - 1U);
					stop_stream_id &= ~((1 << min_value) - 1U);
					break;

			case SPECIFY_FORCE_IDR:
					VERIFY_STREAMID(current_stream);
					//force idr
					force_idr_id |= (1 << current_stream);
					break;

			case ENCODING_RESTART:
					VERIFY_STREAMID(current_stream);
					//restart encoding for stream
					printf("restart.......... \n");
					restart_stream_id |= (1 << current_stream);
					break;

			case FRAME_FACTOR:
					VERIFY_STREAMID(current_stream);
					//change frame rate on the fly
					if (get_two_unsigned_int(optarg, &numerator, &denominator, '/') < 0) {
						return -1;
					}
					framerate_factor[current_stream][0] = numerator;
					framerate_factor[current_stream][1] = denominator;
					framerate_factor_changed_map |= (1 << current_stream);
					break;

			case FRAME_FACTOR_SYNC:
					VERIFY_STREAMID(current_stream);
					//change frame rate on the fly
					if (get_two_unsigned_int(optarg, &numerator, &denominator, '/')
					        < 0) {
						return -1;
					}
					framerate_factor_sync_map |= (1 << current_stream);
					framerate_factor[current_stream][0] = numerator;
					framerate_factor[current_stream][1] = denominator;
					break;

			case SPECIFY_KEEP_FPS_IN_SS:
					VERIFY_STREAMID(current_stream);
					value = atoi(optarg);
					if (value < 0 || value > 1) {
						printf("Invalid value [%d], must be in [0~1].\n", value);
						return -1;
					}
					framerate_factor[current_stream][2] = value;
					keep_fps_in_ss_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_INTRABIAS_P:
					VERIFY_STREAMID(current_stream);
					//change intrabias_p on the fly
					h264_enc_param[current_stream].intrabias_p = atoi(optarg);
					h264_enc_param[current_stream].intrabias_p_changed = 1;
					h264_enc_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_INTRABIAS_B:
					VERIFY_STREAMID(current_stream);
					h264_enc_param[current_stream].intrabias_b = atoi(optarg);
					h264_enc_param[current_stream].intrabias_b_changed = 1;
					h264_enc_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_P_SKIP_BIAS:
					VERIFY_STREAMID(current_stream);
					h264_enc_param[current_stream].bias_p_skip = atoi(optarg);
					h264_enc_param[current_stream].bias_p_skip_changed = 1;
					h264_enc_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_CPB_UNDERFLOW_RATIO:
					VERIFY_STREAMID(current_stream);
					if (get_two_unsigned_int(optarg, &min_value, &max_value, '/') < 0) {
						return -1;
					}
					h264_enc_param[current_stream].cpb_underflow_num = min_value;
					h264_enc_param[current_stream].cpb_underflow_den = max_value;
					h264_enc_param[current_stream].cpb_underflow_changed = 1;
					h264_enc_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_ZMV_THRESHOLD:
					VERIFY_STREAMID(current_stream);
					min_value = atoi(optarg);
					if (min_value > ZMV_TH_MAX || min_value < ZMV_TH_MIN) {
						printf("Invalid zmv threshold value [%d], please choose from [%d~%d].\n",
							min_value, ZMV_TH_MIN, ZMV_TH_MAX);
						return -1;
					}
					h264_enc_param[current_stream].zmv_threshold = min_value;
					h264_enc_param[current_stream].zmv_threshold_changed = 1;
					h264_enc_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_MODE_BIAS_I4:
					VERIFY_STREAMID(current_stream);
					value = atoi(optarg);
					if (value > MODE_BIAS_MAX || value < MODE_BIAS_MIN) {
						printf("Invalid mode bias I4 value [%d], please choose from [%d~%d].\n",
							value, MODE_BIAS_MIN, MODE_BIAS_MAX);
						return -1;
					}
					h264_enc_param[current_stream].mode_bias_I4_add = value;
					h264_enc_param[current_stream].mode_bias_I4_add_changed = 1;
					h264_enc_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_MODE_BIAS_I16:
					VERIFY_STREAMID(current_stream);
					value = atoi(optarg);
					if (value > MODE_BIAS_MAX || value < MODE_BIAS_MIN) {
						printf("Invalid mode bias I16 value [%d], please choose from [%d~%d].\n",
							value, MODE_BIAS_MIN, MODE_BIAS_MAX);
						return -1;
					}
					h264_enc_param[current_stream].mode_bias_I16_add = value;
					h264_enc_param[current_stream].mode_bias_I16_add_changed = 1;
					h264_enc_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_MODE_BIAS_INTER8:
					VERIFY_STREAMID(current_stream);
					value = atoi(optarg);
					if (value > MODE_BIAS_MAX || value < MODE_BIAS_MIN) {
						printf("Invalid mode bias Inter8 value [%d], please choose from [%d~%d].\n",
							value, MODE_BIAS_MIN, MODE_BIAS_MAX);
						return -1;
					}
					h264_enc_param[current_stream].mode_bias_Inter8Add = value;
					h264_enc_param[current_stream].mode_bias_Inter8Add_changed = 1;
					h264_enc_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_MODE_BIAS_INTER16:
					VERIFY_STREAMID(current_stream);
					value = atoi(optarg);
					if (value > MODE_BIAS_MAX || value < MODE_BIAS_MIN) {
						printf("Invalid mode bias Inter16 value [%d], please choose from [%d~%d].\n",
							value, MODE_BIAS_MIN, MODE_BIAS_MAX);
						return -1;
					}
					h264_enc_param[current_stream].mode_bias_Inter16Add = value;
					h264_enc_param[current_stream].mode_bias_Inter16Add_changed = 1;
					h264_enc_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_MODE_BIAS_DIRECT:
					VERIFY_STREAMID(current_stream);
					value = atoi(optarg);
					if (value > MODE_BIAS_MAX || value < MODE_BIAS_MIN) {
						printf("Invalid mode bias direct value [%d], please choose from [%d~%d].\n",
							value, MODE_BIAS_MIN, MODE_BIAS_MAX);
						return -1;
					}
					h264_enc_param[current_stream].mode_bias_DirectAdd = value;
					h264_enc_param[current_stream].mode_bias_DirectAdd_changed = 1;
					h264_enc_param_changed_map |= (1 << current_stream);
					break;

			case PREV_A_FRAMERATE_DIV:
					prev_a_framerate_div = atoi(optarg);
					if (prev_a_framerate_div == 0)	// divisor cannot be zero
						return -1;
					prev_a_framerate_div_changed = 1;
					break;

			case CHANGE_QP_LIMIT_I:
					VERIFY_STREAMID(current_stream);
					if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
						return -1;
					}
					qp_limit[current_stream].min_i = min_value;
					qp_limit[current_stream].max_i = max_value;
					qp_limit[current_stream].i_changed = 1;
					qp_limit_changed_map |= (1 << current_stream);
					break;

			case CHANGE_QP_LIMIT_P:
					VERIFY_STREAMID(current_stream);
					if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
						return -1;
					}
					qp_limit[current_stream].min_p = min_value;
					qp_limit[current_stream].max_p = max_value;
					qp_limit[current_stream].p_changed = 1;
					qp_limit_changed_map |= (1 << current_stream);
					break;

			case CHANGE_QP_LIMIT_B:
					VERIFY_STREAMID(current_stream);
					if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
						return -1;
					}
					qp_limit[current_stream].min_b = min_value;
					qp_limit[current_stream].max_b = max_value;
					qp_limit[current_stream].b_changed = 1;
					qp_limit_changed_map |= (1 << current_stream);
					break;

			case CHANGE_QP_LIMIT_Q:
					VERIFY_STREAMID(current_stream);
					if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
						return -1;
					}
					qp_limit[current_stream].min_q = min_value;
					qp_limit[current_stream].max_q = max_value;
					qp_limit[current_stream].q_changed = 1;
					qp_limit_changed_map |= (1 << current_stream);
					break;

			case CHANGE_ADAPT_QP:
					VERIFY_STREAMID(current_stream);
					qp_limit[current_stream].adapt_qp = atoi(optarg);
					qp_limit[current_stream].adapt_qp_changed = 1;
					qp_limit_changed_map |= (1 << current_stream);
					break;

			case CHANGE_I_QP_REDUCE:
					VERIFY_STREAMID(current_stream);
					qp_limit[current_stream].i_qp_reduce = atoi(optarg);
					qp_limit[current_stream].i_qp_reduce_changed = 1;
					qp_limit_changed_map |= (1 << current_stream);
					break;

			case CHANGE_P_QP_REDUCE:
					VERIFY_STREAMID(current_stream);
					qp_limit[current_stream].p_qp_reduce = atoi(optarg);
					qp_limit[current_stream].p_qp_reduce_changed = 1;
					qp_limit_changed_map |= (1 << current_stream);
					break;

			case CHANGE_Q_QP_REDUCE:
					VERIFY_STREAMID(current_stream);
					qp_limit[current_stream].q_qp_reduce = atoi(optarg);
					qp_limit[current_stream].q_qp_reduce_changed = 1;
					qp_limit_changed_map |= (1 << current_stream);
					break;

			case CHANGE_LOG_Q_NUM:
					VERIFY_STREAMID(current_stream);
					min_value = atoi(optarg);
					if (min_value < 0 || min_value > 10) {
						printf("Invalid value [%d] for 'log_q_num_minus_1' option.\n", min_value);
						return -1;
					}
					qp_limit[current_stream].log_q_num = min_value;
					qp_limit[current_stream].log_q_num_changed = 1;
					qp_limit_changed_map |= (1 << current_stream);
					break;

			case CHANGE_SKIP_FRAME_MODE:
					VERIFY_STREAMID(current_stream);
					qp_limit[current_stream].skip_frame = atoi(optarg);
					qp_limit[current_stream].skip_frame_changed = 1;
					qp_limit_changed_map |= (1 << current_stream);
					break;

			case CHANGE_INTRA_MB_ROWS:
					VERIFY_STREAMID(current_stream);
					intra_mb_rows[current_stream] = atoi(optarg);
					intra_mb_rows_changed_map |= (1 << current_stream);
					break;

			case CHANGE_QP_MATRIX_DELTA:
					VERIFY_STREAMID(current_stream);
					if (get_multi_s8_arg(optarg, qp_matrix[current_stream].delta, 4) < 0) {
						printf("need %d args for qp matrix delta array.\n", 4);
						return -1;
					}
					qp_matrix_changed_map |= (1 << current_stream);
					break;

			case CHANGE_QP_MATRIX_MODE:
					VERIFY_STREAMID(current_stream);
					min_value = atoi(optarg);
					if (min_value > 5) {
						printf("Invalid QP Matrix mode [%d], please choose from 0~5.\n", min_value);
						return -1;
					}
					qp_matrix[current_stream].matrix_mode = min_value;
					qp_matrix_changed_map |= (1 << current_stream);
					break;

			case 'M':
					VERIFY_STREAMID(current_stream);
					min_value = atoi(optarg);
					system_resource_setup.GOP_max_M[current_stream] = min_value;
					system_resource_setup.GOP_max_M_changed_map |= (1 << current_stream);
					system_resource_setup_changed = 1;
					stream_encode_param[current_stream].h264_param.h264_M = min_value;
					stream_encode_param[current_stream].h264_param.h264_M_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case 'N':
					VERIFY_STREAMID(current_stream);
					stream_encode_param[current_stream].h264_param.h264_N = atoi(optarg);
					stream_encode_param[current_stream].h264_param.h264_N_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_GOP_IDR:
					//idr
					VERIFY_STREAMID(current_stream);
					stream_encode_param[current_stream].h264_param.h264_idr_interval = atoi(optarg);
					stream_encode_param[current_stream].h264_param.h264_idr_interval_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_FRAME_DROP:
					VERIFY_STREAMID(current_stream);
					min_value = atoi(optarg);
					if (min_value > 300) {
						printf("Invalid frame drop num [%d], must be [0~300]\n", min_value);
						return -1;
					}
					frame_drop_info[current_stream].drop_frames_num = min_value;
					frame_drop_info_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_GOP_MODEL:
					//gop
					VERIFY_STREAMID(current_stream);
					min_value = atoi(optarg);
					if (min_value > IAV_GOP_LONGTERM_P1B0REF) {
						printf("Invalid GOP model [%d].\n", min_value);
						return -1;
					}
					stream_encode_param[current_stream].h264_param.h264_gop_model = min_value;
					stream_encode_param[current_stream].h264_param.h264_gop_model_changed  = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_LONG_P_INTERVAL:
					VERIFY_STREAMID(current_stream);
					stream_encode_param[current_stream].h264_param.debug_h264_long_p_interval = atoi(optarg);
					stream_encode_param[current_stream].h264_param.debug_h264_long_p_interval_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case BITRATE_CONTROL:
					//bitrate control
					VERIFY_STREAMID(current_stream);
					stream_encode_param[current_stream].h264_param.h264_bitrate_control = get_bitrate_control(optarg);
					stream_encode_param[current_stream].h264_param.h264_bitrate_control_changed  = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_CBR_BITRATE:
					VERIFY_STREAMID(current_stream);
					stream_encode_param[current_stream].h264_param.h264_cbr_avg_bitrate = atoi(optarg);
					stream_encode_param[current_stream].h264_param.h264_cbr_bitrate_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_VBR_BITRATE:
					VERIFY_STREAMID(current_stream);
					if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {
						return -1;
					}
					stream_encode_param[current_stream].h264_param.h264_vbr_min_bitrate = min_value;
					stream_encode_param[current_stream].h264_param.h264_vbr_max_bitrate = max_value;
					stream_encode_param[current_stream].h264_param.h264_vbr_bitrate_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case DEBLOCKING_ALPHA:
					//deblocking alpha
					VERIFY_STREAMID(current_stream);
					value = atoi(optarg);
					if((value < SLICE_ALPHA_VALUE_MIN) ||
						(value > SLICE_ALPHA_VALUE_MAX)) {
						printf("Invalid param [%d] for 'deblocking alpha'"
									"option [-6~6].\n", value);
						return -1;
					}
					stream_encode_param[current_stream].h264_param.h264_deblocking_filter_alpha = atoi(optarg);
					stream_encode_param[current_stream].h264_param.h264_deblocking_filter_alpha_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case DEBLOCKING_BETA:
					//deblocking beta
					VERIFY_STREAMID(current_stream);
					value = atoi(optarg);
					if((value < SLICE_BETA_VALUE_MIN) ||
						(value > SLICE_BETA_VALUE_MAX)) {
						printf("Invalid param [%d] for 'deblocking alpha'"
								"option [-6~6].\n", value);
						return -1;
					}
					stream_encode_param[current_stream].h264_param.h264_deblocking_filter_beta = atoi(optarg);
					stream_encode_param[current_stream].h264_param.h264_deblocking_filter_beta_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case DEBLOCKING_ENABLE:
					//deblocking enable
					VERIFY_STREAMID(current_stream);
					min_value = atoi(optarg);
					if (min_value > 1 ) {
						printf("Invalid param [%d] for 'deblocking disable'"
								"option [0|1].\n", min_value);
					}
					stream_encode_param[current_stream].h264_param.h264_deblocking_filter_disable = atoi(optarg);
					stream_encode_param[current_stream].h264_param.h264_deblocking_filter_disable_changed= 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_PROFILE_LEVEL:
					VERIFY_STREAMID(current_stream);
					min_value = atoi(optarg);
					if ((min_value < H264_PROFILE_FIRST) || (min_value >= H264_PROFILE_LAST)) {
						printf("Invalid param [%d] for 'H264 profile' option [0~2].\n", min_value);
						return -1;
					}
					stream_encode_param[current_stream].h264_param.h264_profile_level = min_value;
					stream_encode_param[current_stream].h264_param.h264_profile_level_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_QMATRIX_4X4:
					VERIFY_STREAMID(current_stream);
					min_value = atoi(optarg);
					if (min_value > 1) {
						printf("Invalid param [%d] for 'Q-matrix 4x4 enable' option [0|1].\n", min_value);
						return -1;
					}
					stream_encode_param[current_stream].h264_param.h264_qmatrix_4x4 = min_value;
					stream_encode_param[current_stream].h264_param.h264_qmatrix_4x4_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_2X_SEARCH_RANGE:
					VERIFY_STREAMID(current_stream);
					min_value = atoi(optarg);
					if (min_value > 1) {
						printf("Invalid param [%d] for 'search_range_2x' option (0|1)!\n", min_value);
						return -1;
					}
					system_resource_setup.stream_2x_search_range[current_stream] = min_value;
					system_resource_setup.stream_2x_search_range_changed_map |= (1 << current_stream);
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_READOUT_MODE:
					min_value = atoi(optarg);
					system_info_setup.bits_read_mode = atoi(optarg);
					system_info_setup_changed = 1;
					break;


			case INTLC_IFRAME:
					//interlaced iframe
					VERIFY_STREAMID(current_stream);
					stream_encode_param[current_stream].h264_param.force_intlc_tb_iframe = atoi(optarg);
					stream_encode_param[current_stream].h264_param.force_intlc_tb_iframe_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_PANIC_RATIO:
					VERIFY_STREAMID(current_stream);
					if (get_two_unsigned_int(optarg, &min_value, &max_value, '/') < 0) {
						return -1;
					}
					stream_encode_param[current_stream].h264_param.panic_num = min_value;
					stream_encode_param[current_stream].h264_param.panic_den = max_value;
					stream_encode_param[current_stream].h264_param.panic_ratio_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_AU_TYPE:
					//au type in h264 syntax
					VERIFY_STREAMID(current_stream);
					value = atoi(optarg);
					if ((value < IAV_AU_TYPE_FIRST) || (value >= IAV_AU_TYPE_LAST)) {
						printf("Invalid AU type value [%d]. It must be in the range of [%d~%d).\n",
							value, IAV_AU_TYPE_FIRST, IAV_AU_TYPE_LAST);
						return -1;
					}
					stream_encode_param[current_stream].h264_param.au_type = value;
					stream_encode_param[current_stream].h264_param.au_type_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case 'q':
					//mjpeg quality
					VERIFY_STREAMID(current_stream);
					stream_encode_param[current_stream].jpeg_param.quality = atoi(optarg);
					stream_encode_param[current_stream].jpeg_param.quality_changed  = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_HFLIP:
					//horizontal flip
					VERIFY_STREAMID(current_stream);
					stream_encode_format[current_stream].hflip = atoi(optarg);
					stream_encode_format[current_stream].hflip_changed = 1;
					stream_encode_format_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_VFLIP:
					//vertical flip
					VERIFY_STREAMID(current_stream);
					stream_encode_format[current_stream].vflip = atoi(optarg);
					stream_encode_format[current_stream].vflip_changed = 1;
					stream_encode_format_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_ROTATE:
					//clockwise rotate
					VERIFY_STREAMID(current_stream);
					min_value = atoi(optarg);
					stream_encode_format[current_stream].rotate = min_value;
					stream_encode_format[current_stream].rotate_changed = 1;
					stream_encode_format_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_CHROME_FORMAT:
					//chrome format
					VERIFY_STREAMID(current_stream);
					if ((min_value = get_chrome_format(optarg, IAV_ENCODE_H264)) < 0)
						return -1;
					if ((max_value = get_chrome_format(optarg, IAV_ENCODE_MJPEG)) < 0)
						return -1;
					stream_encode_param[current_stream].h264_param.h264_chrome_format = min_value;
					stream_encode_param[current_stream].h264_param.h264_chrome_format_changed = 1;
					stream_encode_param[current_stream].jpeg_param.jpeg_chrome_format = max_value;
					stream_encode_param[current_stream].jpeg_param.jpeg_chrome_format_changed = 1;
					stream_encode_param_changed_map |= (1 << current_stream);
					break;

			case SPECIFY_NEGATIVE_OFFSET_DISABLE:
					VERIFY_STREAMID(current_stream);
					value = atoi(optarg);
					if (value < 0 || value > 1) {
						printf("Invalid value [%d], valid range [0, 1].\n",value);
						return -1;
					}
					stream_encode_format[current_stream].negative_offset_disable = value;
					stream_encode_format[current_stream].negative_offset_disable_changed = 1;
					stream_encode_format_changed_map |= (1 << current_stream);
					break;

			//VIN
			VIN_INIT_PARAMETERS()

			case SPECIFY_VIN_CAP_OFFSET:
					if (get_two_unsigned_int(optarg, &min_value, &max_value, 'x') < 0) {
						return -1;
					}
					vin_cap_start_x = min_value;
					vin_cap_start_y = max_value;
					vin_cap_offset_flag = 1;
					break;

			case NO_PREVIEW:
					//nopreview
					nopreview_flag = 1;
					break;

			case SPECIFY_VIDEO_FREEZE:
					min_value = atoi(optarg);
					if (min_value > 1) {
						printf("Invalid parameter for VIDEO freeze option [0|1].\n");
						return -1;
					}
					vcap_param.video_freeze = min_value;
					vcap_param.video_freeze_flag = 1;
					vcap_param_changed = 1;
					break;

			case SYSTEM_IDLE:
					//system state, go to idle
					idle_cmd = 1;
					break;

			case SPECIFY_SHARPEN_B:
					min_value = atoi(optarg);
					if (min_value > 1) {
						printf("Invalid parameter for MCTF possible option [0|1].\n");
						return -1;
					}
					system_resource_setup.sharpen_b = min_value;
					system_resource_setup.sharpen_b_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_ENCODE_MODE:
					min_value = atoi(optarg);
					if (min_value >= IAV_ENCODE_MODE_TOTAL_NUM) {
						printf("Invalid param [%d] for encode mode option [0~%d].\n",
							min_value, IAV_ENCODE_MODE_TOTAL_NUM - 1);
						return -1;
					}
					system_resource_setup.encode_mode = min_value;
					system_resource_setup.encode_mode_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_MAX_ENC_NUM:
					min_value = atoi(optarg);
					if (min_value < 1 || min_value > MAX_ENCODE_STREAM_NUM) {
						printf("Invalid param for max encode number option"
							" [1~%d].\n", MAX_ENCODE_STREAM_NUM);
						return -1;
					}
					system_resource_setup.stream_max_num = min_value;
					system_resource_setup.stream_max_num_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_ROTATE_POSSIBLE:
					min_value = atoi(optarg);
					system_resource_setup.rotate_possible = min_value;
					system_resource_setup.rotate_possible_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_HDR_EXPOSURE_NUM:
					min_value = atoi(optarg);
					system_resource_setup.exposure_num = min_value;
					system_resource_setup.exposure_num_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_MAX_CFA_VIN_NUM:
					min_value = atoi(optarg);
					system_resource_setup.vin_num = min_value;
					system_resource_setup.vin_num_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_HWARP_BYPASS_POSSIBLE:
					value = atoi(optarg);
					if (value < 0 || value > 1) {
						printf("Invalid value [%d], valid range [0, 1].\n",
							value);
						return -1;
					}
					system_resource_setup.hwarp_bypass_possible = value;
					system_resource_setup.hwarp_bypass_possible_changed = value;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_CMD_READ_DELAY:
					min_value = atoi(optarg);
					system_info_setup.cmd_read_mode = min_value;
					system_info_setup.cmd_read_mode_changed = 1;
					system_info_setup_changed = 1;
					break;

			case SPECIFY_EIS_DELAY_COUNT:
					min_value = atoi(optarg);
					if (min_value > 2 || min_value < 0) {
						printf("Invalid value [%d] for eis-delay-count.\n", min_value);
						return -1;
					}
					system_info_setup.eis_delay_count = min_value;
					system_info_setup.eis_delay_count_changed = 1;
					system_info_setup_changed = 1;
					break;

			case SPECIFY_RAW_CAPTURE:
					min_value = atoi(optarg);
					system_resource_setup.raw_capture_enabled = !!min_value;
					system_resource_setup.raw_capture_enabled_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_ENC_FROM_RAW:
					min_value = atoi(optarg);
					system_resource_setup.enc_from_raw = !!min_value;
					system_resource_setup.enc_from_raw_changed = 1;
					if (min_value > 0) {
						/* Enable RAW capture if encoding from RAW is enabled. */
						system_resource_setup.raw_capture_enabled = 1;
						system_resource_setup.raw_capture_enabled_changed = 1;
					}
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_MAX_VIN_STAT_LINES_TOP:
					min_value = atoi(optarg);
					system_resource_setup.raw_stats_lines_top = min_value;
					system_resource_setup.raw_stats_lines_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_MAX_VIN_STAT_LINES_BOT:
					min_value = atoi(optarg);
					system_resource_setup.raw_stats_lines_bottom = min_value;
					system_resource_setup.raw_stats_lines_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case 'w':
					min_value = atoi(optarg);
					system_resource_setup.warp_max_input_width = min_value;
					system_resource_setup.warp_max_input_width_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case 'W':
					min_value = atoi(optarg);
					system_resource_setup.warp_max_output_width = min_value;
					system_resource_setup.warp_max_output_width_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_MAX_CHROMA_NOISE_SHIFT:
					min_value = atoi(optarg);
					system_resource_setup.max_chroma_noise_shift = min_value;
					system_resource_setup.max_chroma_noise_shift_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_VOUT_SWAP:
					min_value = atoi(optarg);
					if (min_value > 1) {
						printf("Invalid VOUT swap [%d], should be [0|1].\n",
							min_value);
						return -1;
					}
					system_info_setup.vout_swap = min_value;
					system_info_setup.vout_swap_changed = 1;
					system_info_setup_changed = 1;
					break;

			case SPECIFY_PM_TYPE:
					min_value = atoi(optarg);
					if (min_value > 1) {
						printf("Invalid MCTF pm [%d], should be [0|1].\n",
							min_value);
						return -1;
					}
					system_info_setup.pm_type = min_value;
					system_info_setup.pm_type_changed = 1;
					system_info_setup_changed = 1;
					break;

			case SPECIFY_EXTRA_2X_ZOOM:
					min_value = atoi(optarg);
					if (min_value < 0 || min_value > 1) {
						printf("Invalid value [%d] for 'extra 2X zoom' option.", min_value);
						return -1;
					}
					system_resource_setup.extra_2x_enable = min_value;
					system_resource_setup.extra_2x_enable_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_VCA_BUFFER:
					min_value = atoi(optarg);
					if (min_value < 0 || min_value > 1) {
						printf("Invalid value [%d] for 'VCA buffer' option.", min_value);
						return -1;
					}
					system_resource_setup.vca_buffer_enable = min_value;
					system_resource_setup.vca_buffer_enable_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_MAP_DSP_PARTITION:
					min_value = atoi(optarg);
					if (min_value < 0 || min_value > 1) {
						printf("Invalid value [%d] for 'VCA buffer' option.", min_value);
						return -1;
					}
					system_resource_setup.map_dsp_partition = min_value;
					system_resource_setup.map_dsp_partition_changed = 1;
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_VSKIP_BEFORE_ENCODE:
					min_value = atoi(optarg);
					if (min_value < 3 || min_value > 5) {
						printf("Invalid vsync skip number [%d], should be [3~5].\n",
							min_value);
						return -1;
					}
					system_resource_setup.vskip_before_encode = min_value;
					system_resource_setup.vskip_before_encode_changed = 1;
					system_resource_setup_changed = 1;
					break;

			//Vout
			VOUT_INIT_PARAMETERS()

			case DUMP_IDSP_CONFIG:
					dump_idsp_bin_flag = 1;
					break;

			//source buffer
			case 'X':
					current_buffer = MAIN_SOURCE_BUFFER;
					break;
			case 'P':
					current_buffer = PRE_MAIN_BUFFER;
					break;
			case SPECIFY_SUB_BUFFER:
					current_buffer = atoi(optarg);
					VERIFY_SUB_BUFFERID(current_buffer);
					break;
			case 'Y':
					current_buffer = SECOND_SOURCE_BUFFER;
					break;
			case 'J':
					current_buffer = THIRD_SOURCE_BUFFER;
					break;
			case 'K':
					current_buffer = FOURTH_SOURCE_BUFFER;
					break;

			case 'Z':
					current_buffer = atoi(optarg);
					if (current_buffer != 0) {
						printf("Invalid ID for DRAM buffer! It must be 0!\n");
						return -1;
					}
					current_buffer += MAIN_SOURCE_DRAM;
					break;

			case SPECIFY_BUFFER_TYPE:
					VERIFY_BUFFERID(current_buffer);
					int buffer_type = get_buffer_type(optarg);
					if (buffer_type < 0)
						return -1;
					source_buffer_setup.buffer_type[current_buffer] = buffer_type;
					source_buffer_setup.buffer_type_changed_map |= (1 << current_buffer);
					source_buffer_setup_changed = 1;
					break;

			case SPECIFY_BUFFER_SIZE:
					VERIFY_BUFFERID(current_buffer);
					if (get_encode_resolution(optarg, &width, &height) < 0)
						return -1;
					source_buffer_format[current_buffer].width = width;
					source_buffer_format[current_buffer].height = height;
					source_buffer_format[current_buffer].resolution_changed = 1;
					source_buffer_format_changed_map |= (1 << current_buffer);
					break;

			case SPECIFY_BUFFER_MAX_SIZE:
					VERIFY_BUFFERID(current_buffer);
					if (current_buffer == PRE_MAIN_BUFFER) {
						printf("cannot set bmaxsize for P buffer!\n");
						return -1;
					}
					if (get_encode_resolution(optarg, &width, &height) < 0)
						return -1;
					system_resource_setup.buffer_max_width[current_buffer] = width;
					system_resource_setup.buffer_max_height[current_buffer] = height;
					system_resource_setup.buffer_max_changed_map |= (1 << current_buffer);
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_EXTRA_DRAM_BUF:
					VERIFY_BUFFERID(current_buffer);
					value = atoi(optarg);
					if (value < -4 || value > 20) {
						printf("Invalid value [%d], valid range [-4~20].\n",
							value);
						return -1;
					}
					system_resource_setup.extra_dram_buf[current_buffer] = value;
					system_resource_setup.extra_dram_buf_changed_map |= (1 << current_buffer);
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_BUFFER_INPUT_SIZE:
					VERIFY_BUFFERID(current_buffer);
					if (get_encode_resolution(optarg, &width, &height) < 0)
						return -1;
					source_buffer_format[current_buffer].input_width = width;
					source_buffer_format[current_buffer].input_height = height;
					source_buffer_format[current_buffer].input_size_changed = 1;
					source_buffer_format_changed_map |= (1 << current_buffer);
					break;

			case SPECIFY_BUFFER_INPUT_OFFSET:
					VERIFY_BUFFERID(current_buffer);
					if (get_encode_resolution(optarg, &width, &height) < 0)
						return -1;
					source_buffer_format[current_buffer].input_x = width;
					source_buffer_format[current_buffer].input_y = height;
					source_buffer_format[current_buffer].input_offset_changed = 1;
					source_buffer_format_changed_map |= (1 << current_buffer);
					break;

			case SPECIFY_BUFFER_UNWARP:
					VERIFY_BUFFERID(current_buffer);
					min_value = atoi(optarg);
					source_buffer_setup.unwarp[current_buffer] = (min_value ? 1 : 0);
					source_buffer_setup.buffer_unwarp_changed_map |= (1 << current_buffer);
					source_buffer_setup_changed = 1;
					break;

			case SPECIFY_DRAM_BUFFER_MAX_FRAME_NUM:
					VERIFY_BUFFERID(current_buffer);
					max_value = atoi(optarg);
					if (max_value > 10) {
						printf("Cannot set max frame number [%d] greater than 10!\n", max_value);
						return -1;
					}
					system_resource_setup.max_dram_frame[current_buffer - MAIN_SOURCE_DRAM] = max_value;
					system_resource_setup.max_dram_frame_map |= (1 << current_buffer);
					system_resource_setup_changed = 1;
					break;

			case SPECIFY_OSD_MIXER:
					system_info_setup.osd_mixer = get_osd_mixer_selection(optarg);
					if (system_info_setup.osd_mixer < 0)
						return -1;
					system_info_setup.osd_mixer_changed = 1;
					system_info_setup_changed = 1;
					break;

			//Show status
			case SHOW_SYSTEM_STATE:
					show_iav_state_flag = 1;
					break;

			case SHOW_ENCODE_CONFIG:
					//show h264 or mjpeg encode config
					show_encode_config_flag = 1;
					break;

			case SHOW_STREAM_INFO:
					//show encode stream info
					show_encode_stream_info_flag = 1;
					break;

			case SHOW_BUFFER_INFO:
					//show source buffer info
					show_source_buffer_info_flag = 1;
					break;

			case SHOW_RESOURCE_INFO:
					//show resource limit info
					show_resource_limit_info_flag = 1;
					break;

			case SHOW_DRIVER_INFO:
					show_iav_driver_info_flag = 1;
					break;

			case SHOW_CHIP_INFO:
					show_chip_info_flag = 1;
					break;

			case SHOW_ENCODE_MODE_CAP:
					show_encode_mode_cap_flag = 1;
					break;

			case SHOW_ALL_INFO:
					show_iav_state_flag = 1;
					show_encode_config_flag = 1;
					show_encode_stream_info_flag = 1;
					show_source_buffer_info_flag = 1;
					show_resource_limit_info_flag = 1;
					show_iav_driver_info_flag = 1;
					show_chip_info_flag = 1;
					show_encode_mode_cap_flag = 1;
					break;

			case SPECIFY_DSP_NONCACHED:
					min_value = atoi(optarg);
					if (min_value > 1) {
						printf("Invalid value [%d] for 'dsp-noncached' option.\n",
							min_value);
						return -1;
					}
					system_info_setup.dsp_noncache = min_value;
					system_info_setup.dsp_noncache_changed = 1;
					system_info_setup_changed = 1;
					break;

			case SPECIFY_CHECK_DISABLE:
					min_value = atoi(optarg);
					if (min_value > 1) {
						printf("Invalid value [%d] for 'check-disable' option.\n", min_value);
						return -1;
					}
					iav_debug.check_disable = min_value;
					iav_debug.check_disable_flag = 1;
					iav_debug_flag = 1;
					break;
			case SPECIFY_DEBUG_CHIP_ID:
					value = atoi(optarg);
					if (value >= IAV_CHIP_ID_S2_LAST &&
							value < IAV_CHIP_ID_S2_UNKNOWN) {
						printf("Invalid value [%d] for 'debug-chip-id' option.\n", value);
						return -1;
					}
					system_resource_setup.debug_chip_id = value;
					system_resource_setup.debug_chip_id_flag = 1;
					system_resource_setup_changed = 1;
					break;
			case SPECIFY_KERNEL_PRINT_ENABLE:
					min_value = atoi(optarg);
					if (min_value > 1) {
						printf("Invalid value [%d] for 'kernel-print-enable' option.\n", min_value);
						return -1;
					}
					iav_debug.kernel_print_enable = min_value;
					iav_debug.kernel_print_enable_flag = 1;
					iav_debug_flag = 1;
					break;

			case SPECIFY_ENC_DUMMY_LATENCY:
				min_value = atoi(optarg);
				if (min_value < 0 || min_value > 4) {
					printf("Invalid value [%d], must be in [0~4].\n",	min_value);
					return -1;
				}
				system_info_setup.debug_enc_dummy_latency_count = min_value;
				system_info_setup.debug_enc_dummy_latency_flag = 1;
				system_info_setup_changed = 1;
				break;

			case SPECIFY_GOP_REF_P:
				min_value = atoi(optarg);
				if (min_value < GOP_REF_P_MIN || min_value > GOP_REF_P_MAX) {
					printf("Invalid value [%d], must be in [%d ~ %d].\n", min_value,
					GOP_REF_P_MIN, GOP_REF_P_MAX);
					return -1;
				}

				VERIFY_STREAMID(current_stream);
				system_resource_setup.debug_max_ref_P[current_stream] = min_value;
				system_resource_setup.debug_max_ref_P_changed_map |= (1 << current_stream);
				system_resource_setup_changed = 1;
				stream_encode_param[current_stream].h264_param.debug_h264_ref_P = min_value;
				stream_encode_param[current_stream].h264_param.debug_h264_ref_P_changed = 1;
				stream_encode_param_changed_map |= (1 << current_stream);
				break;
			case SPECIFY_VOUT_B_LETTER_BOXING_ENABLE:
				min_value = atoi(optarg);
				if (min_value < 0 || min_value > 1) {
					printf("Invalid value [%d], must be in [0~1].\n", min_value);
					return -1;
				}
				system_resource_setup.vout_b_letter_box_enable = min_value;
				system_resource_setup.vout_b_letter_box_enable_changed = 1;
				system_resource_setup_changed = 1;
				break;
			case SPECIFY_YUV_INPUT_ENHANCED:
				min_value = atoi(optarg);
				if (min_value < 0 || min_value > 1) {
					printf("Invalid value [%d], must be in [0~1].\n", min_value);
					return -1;
				}
				system_resource_setup.yuv_input_enhanced = min_value;
				system_resource_setup.yuv_input_enhanced_changed = 1;
				system_resource_setup_changed = 1;
				break;
			default:
				printf("unknown option found: %c\n", ch);
				return -1;
			}
	}

	if (verify_params() < 0) {
		printf("verify_params error\n");
		return -1;
	}

	return 0;
}

static void get_chip_id_str(u32 chip_id, char *chip_str)
{
	switch (chip_id) {
		case IAV_CHIP_ID_S2_22:
			strcpy(chip_str, "S222");
			break;
		case IAV_CHIP_ID_S2_33:
			strcpy(chip_str, "S233");
			break;
		case IAV_CHIP_ID_S2_55:
			strcpy(chip_str, "S255");
			break;
		case IAV_CHIP_ID_S2_66:
			strcpy(chip_str, "S266");
			break;
		case IAV_CHIP_ID_S2_88:
			strcpy(chip_str, "S288");
			break;
		case IAV_CHIP_ID_S2_99:
			strcpy(chip_str, "S299");
			break;
		default:
			strcpy(chip_str, "Unknown");
			break;
	}
}

static int show_encode_stream_info(void)
{
	iav_encode_stream_info_ex_t stream_info;
	iav_encode_format_ex_t stream_encode_format;
	iav_change_framerate_factor_ex_t frame_rate;
	int format_configured;
	char state_str[128];
	char type_str[128];
	char source_str[128];
	int i;

	printf("\n[Encode stream info]:\n");
	for (i = 0; i < MAX_ENCODE_STREAM_NUM ; i++) {
		memset(&stream_info, 0, sizeof(stream_info));
		stream_info.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info);
		switch (stream_info.state) {
		case IAV_STREAM_STATE_UNKNOWN:
			strcpy(state_str, "unknown");
			break;
		case IAV_STREAM_STATE_ERROR:
			strcpy(state_str, "error");
			break;
		case IAV_STREAM_STATE_READY_FOR_ENCODING:
			strcpy(state_str, "ready for encoding");
			break;
		case IAV_STREAM_STATE_ENCODING:
			strcpy(state_str, "encoding");
			break;
		case IAV_STREAM_STATE_STARTING:
			strcpy(state_str, "encode starting");
			break;
		case IAV_STREAM_STATE_STOPPING:
			strcpy(state_str, "encode stopping");
			break;
		default:
			printf("Invalid stream state [%d].\n", stream_info.state);
			return -1;
		}

		memset(&stream_encode_format, 0, sizeof(stream_encode_format));
		stream_encode_format.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &stream_encode_format);

		switch (stream_encode_format.encode_type) {
		case IAV_ENCODE_H264:
			strcpy(type_str, "H.264");
			format_configured = 1;
			break;
		case IAV_ENCODE_MJPEG:
			strcpy(type_str, "MJPEG");
			format_configured = 1;
			break;
		case IAV_ENCODE_NONE:
			strcpy(type_str, "None");
		default:
			format_configured = 0;
			break;
		}

		switch (stream_encode_format.source) {
		case IAV_ENCODE_SOURCE_MAIN_BUFFER:
			strcpy(source_str, "MAIN source buffer");
			break;
		case IAV_ENCODE_SOURCE_SECOND_BUFFER:
			strcpy(source_str, "SECOND source buffer");
			break;
		case IAV_ENCODE_SOURCE_THIRD_BUFFER:
			strcpy(source_str, "THIRD source buffer");
			break;
		case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
			strcpy(source_str, "FOURTH source buffer");
			break;
		}
		memset(&frame_rate, 0, sizeof(frame_rate));
		frame_rate.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_FRAMERATE_FACTOR_EX, &frame_rate);

		COLOR_PRINT("Stream %c \n", i + 'A');
		BOLD_PRINT("\t%s\n", type_str);
		if (format_configured) {
			printf("\t        state = %s\n", state_str);
			printf("\tencode source = %s\n", source_str);
			printf("\t     duration = %d%s\n", stream_encode_format.duration,
				stream_encode_format.duration ? "" : " (for ever)");
			printf("\t   resolution = (%dx%d) \n", stream_encode_format.encode_width,
				stream_encode_format.encode_height);
			printf("\tencode offset = (%d,%d)\n", stream_encode_format.encode_x,
				stream_encode_format.encode_y);
			printf("\t        hflip = %d\n", stream_encode_format.hflip);
			printf("\t        vflip = %d\n", stream_encode_format.vflip);
			printf("\t       rotate = %d\n", stream_encode_format.rotate_clockwise);
			printf("\t    fps ratio = %d/%d\n", frame_rate.ratio_numerator,
				frame_rate.ratio_denominator);
			printf("\tkeep_fps_in_ss = %d\n", frame_rate.keep_fps_in_ss);
		}
	}

	return 0;
}

static int show_source_buffer_info(void)
{
	iav_source_buffer_info_ex_t		buffer_info;
	iav_system_resource_setup_ex_t	resource_setup;
	iav_rect_ex_t	vin_cap;
	iav_source_buffer_setup_ex_t	buffer_setup;
	struct amba_video_info video_info;
	int preview_width, preview_height;
	char state_str[64];
	char buffer_type_str[64];
	char fps[32];
	u32 fps_hz;
	int i;

	AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &buffer_setup);

	resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup);
	memset(&vin_cap, 0, sizeof(vin_cap));
	AM_IOCTL(fd_iav, IAV_IOC_GET_VIN_CAPTURE_WINDOW, &vin_cap);
	memset(&video_info, 0, sizeof(video_info));
	AM_IOCTL(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info);
	change_fps_to_hz(video_info.fps, &fps_hz, fps);

	printf("\n[VIN Capture info]:\n");
	printf("  VIN Capture window : %dx%d\n", vin_cap.width, vin_cap.height);
	printf("  VIN Capture offset : %dx%d\n", vin_cap.x, vin_cap.y);
	printf("  VIN frame rate : %s\n", fps);

	printf("\n[Source buffer info]:\n");
	if (resource_setup.encode_mode == IAV_ENCODE_WARP_MODE ||
		resource_setup.encode_mode == IAV_ENCODE_HIGH_MP_WARP_MODE) {
		COLOR_PRINT0("  Pre Main buffer: \n");
		printf("\tformat : %dx%d\n", buffer_setup.pre_main.width,
		       buffer_setup.pre_main.height);
		printf("\tinput_format : %dx%d\n", buffer_setup.pre_main_input.width,
		       buffer_setup.pre_main_input.height);
		printf("\tinput_offset : %dx%d\n\n", buffer_setup.pre_main_input.x,
		       buffer_setup.pre_main_input.y);
	}

	for (i = 0; i < MAX_SOURCE_BUFFER_NUM; i++) {
		memset(&buffer_info, 0, sizeof(buffer_info));
		buffer_info.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_INFO_EX, &buffer_info);
		switch (buffer_info.state) {
		case IAV_SOURCE_BUFFER_STATE_UNKNOWN:
			strcpy(state_str, "unknown");
			break;
		case IAV_SOURCE_BUFFER_STATE_ERROR:
			strcpy(state_str, "error");
			break;
		case IAV_SOURCE_BUFFER_STATE_IDLE:
			strcpy(state_str, "idle");
			break;
		case IAV_SOURCE_BUFFER_STATE_BUSY:
			strcpy(state_str, "busy");
			break;
		default:
			return -1;
		}
		COLOR_PRINT("  Source buffer [%d]: \n", i);
		preview_width = 0;
		preview_height = 0;
		switch (buffer_setup.type[i]) {
		case IAV_SOURCE_BUFFER_TYPE_ENCODE:
			strcpy(buffer_type_str, "encode");
			break;
		case IAV_SOURCE_BUFFER_TYPE_PREVIEW:
			strcpy(buffer_type_str,"preview");
			if (i == IAV_ENCODE_SOURCE_THIRD_BUFFER) {
				preview_width = resource_setup.voutB.width;
				preview_height = resource_setup.voutB.height;
			} else if (i == IAV_ENCODE_SOURCE_FOURTH_BUFFER) {
				preview_width = resource_setup.voutA.width;
				preview_height = resource_setup.voutA.height;
			}
			break;
		case IAV_SOURCE_BUFFER_TYPE_OFF:
			strcpy(buffer_type_str,"off");
			break;
		default:
			sprintf(buffer_type_str,"%d", buffer_setup.type[i]);
			break;
		}

		printf("\ttype : %s \n", buffer_type_str);
		if (buffer_setup.type[i] == IAV_SOURCE_BUFFER_TYPE_ENCODE) {
			printf("\tformat : %dx%d\n", buffer_setup.size[i].width,
				buffer_setup.size[i].height);
			printf("\tinput_format : %dx%d\n",
			       buffer_setup.input[i].width, buffer_setup.input[i].height);
			printf("\tinput_offset : %dx%d\n",
			       buffer_setup.input[i].x, buffer_setup.input[i].y);
			printf("\tunwarp flag: %d\n", buffer_setup.unwarp[i]);
			printf("\tstate : %s \n\n", state_str);
		} else {
			printf("\tformat : %dx%d\n\n", preview_width, preview_height);
		}
	}

	return 0;
}

static int show_resource_limit_info(void)
{
	iav_system_resource_setup_ex_t  resource_setup;
	iav_system_setup_info_ex_t sys_setup;
	char temp[32];
	char chip_str[128];
	int i, dewarp_mode = 0;
	char flag[2][16] = { "Disabled", "Enabled" };

	memset(chip_str, 0, sizeof(chip_str));
	memset(&sys_setup, 0, sizeof(sys_setup));
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_SETUP_INFO_EX, &sys_setup);
	memset(&resource_setup, 0, sizeof(resource_setup));
	resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup);

	switch (resource_setup.encode_mode) {
	case IAV_ENCODE_FULL_FRAMERATE_MODE:
		strcpy(temp, "Full frame rate");
		break;
	case IAV_ENCODE_WARP_MODE:
		strcpy(temp, "Multi region warping");
		dewarp_mode = 1;
		break;
	case IAV_ENCODE_HIGH_MEGA_MODE:
		strcpy(temp, "High mega pixel");
		break;
	case IAV_ENCODE_CALIBRATION_MODE:
		strcpy(temp, "Calibration");
		break;
	case IAV_ENCODE_HDR_FRAME_MODE:
		strcpy(temp, "HDR frame");
		break;
	case IAV_ENCODE_HDR_LINE_MODE:
		strcpy(temp, "HDR line");
		break;
	case IAV_ENCODE_HIGH_MP_FULL_PERF_MODE:
		strcpy(temp, "High MP (Full performance)");
		break;
	case IAV_ENCODE_FULL_FPS_FULL_PERF_MODE:
		strcpy(temp, "Full FPS (Full performance)");
		break;
	case IAV_ENCODE_MULTI_VIN_MODE:
		strcpy(temp, "Multiple VIN");
		break;
	case IAV_ENCODE_HISO_VIDEO_MODE:
		strcpy(temp, "Ultra Low Light (ULL)");
		break;
	case IAV_ENCODE_HIGH_MP_WARP_MODE:
		strcpy(temp, "High MP Warping");
		dewarp_mode = 1;
		break;
	default :
		sprintf(temp, "Unknown mode [%d]", resource_setup.encode_mode);
		break;
	}

	printf("\n[System information]:\n");
	get_chip_id_str(resource_setup.debug_chip_id, chip_str);
	printf("\n  Debug Chip ID : [%s]\n", chip_str);
	printf("  Encode mode : [%s]\n", temp);
	printf("  Frame read out protocol : %s\n",
		sys_setup.coded_bits_interrupt_enable ? "Interrupt" : "Polling");
	printf("\n[Codec resource limit info]:\n");
	for (i = 0; i < IAV_ENCODE_SOURCE_TOTAL_NUM; ++ i) {
		printf("  Source buffer %d max size : %dx%d\n", i,
			resource_setup.buffer_max_size[i].width,
			resource_setup.buffer_max_size[i].height);
	}
	printf("  Extra DRAM buffer (0~3)  : %d, %d, %d, %d\n\n",
		resource_setup.extra_dram_buf[0], resource_setup.extra_dram_buf[1],
		resource_setup.extra_dram_buf[2], resource_setup.extra_dram_buf[3]);

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		printf("  Stream %c Max : size [%4dx%4d], M [%d], N [%3d], 2X V-search [%d].\n",
			'A' + i,
			resource_setup.stream_max_size[i].width,
			resource_setup.stream_max_size[i].height,
			resource_setup.stream_max_GOP_M[i],
			resource_setup.stream_max_GOP_N[i],
			resource_setup.stream_2x_search_range[i]);
	}

	printf("  Max number of encode streams (4~8)  : %d\n",
		resource_setup.max_num_encode_streams);
	printf("  Max number of capture sources (2~4) : %d\n\n",
		resource_setup.max_num_cap_sources);

	if (dewarp_mode) {
		printf("  Max warp input width : %d\n",
			resource_setup.max_warp_input_width);
		printf("  Max warp output width : %d\n",
			resource_setup.max_warp_output_width);
		printf("  H-warp bypass possible : %d\n",
		 	resource_setup.hwarp_bypass_possible);
	}

	printf("  Exposure number (1~4)       : %d\n",
		resource_setup.exposure_num);
	printf("  Multiple VIN number (1~4)   : %d\n",
		resource_setup.vin_num);

	printf("  Raw statistics lines (0~15) : Top [%d] Bottom [%d]\n",
		resource_setup.max_vin_stats_lines_top,
		resource_setup.max_vin_stats_lines_bottom);
	printf("  Vsync skip before encode    : %d\n",
		resource_setup.vskip_before_encode);
	printf("  2nd buf extra 2X  : %s\n", flag[resource_setup.extra_2x_zoom_enable]);
	printf("  Sharpen-B         : %s\n", flag[resource_setup.sharpen_b_enable]);
	printf("  Mixer-B           : %s\n", flag[resource_setup.mixer_b_enable]);
	printf("  Vout B Letter boxing  : %s\n", flag[resource_setup.vout_b_letter_box_enable]);
	printf("  Enhance YUV input     : %s\n", flag[resource_setup.yuv_input_enhanced]);
	printf("  Rotation possible : %s\n", flag[resource_setup.rotate_possible]);
	printf("  Raw Capture       : %s\n", flag[resource_setup.raw_capture_enable]);
	printf("  Encode from RAW   : %s\n", flag[resource_setup.enc_from_raw_enable]);
	printf("  VOUT swap         : %s\n", flag[sys_setup.vout_swap]);
	printf("  Privacy Mask Type : %s\n", sys_setup.mctf_privacy_mask ?
		"MCTF based (MB level)" : "HDR based (Pixel level)");
	printf("  Max chroma noise shift : %u\n",
		32 << resource_setup.max_chroma_noise_shift);
	printf("  DSP memory non-cached : %s\n", flag[sys_setup.dsp_noncached]);
	printf("  Total dram memory size (G bit) : %d\n",
		resource_setup.total_memory_size);

	return 0;
}

static int show_h264_encode_config(int stream_id,
	iav_encode_format_ex_t * format)
{
	iav_bitrate_info_ex_t bitrate_info;
	iav_h264_config_ex_t config;
	iav_stream_cfg_t qmatrix;
	iav_h264_q_matrix_ex_t * qm = NULL;
	char tmp[32];
	int i, qm_flag = 0;

	memset(&bitrate_info, 0, sizeof(bitrate_info));
	memset(&config, 0, sizeof(config));
	memset(&qmatrix, 0, sizeof(qmatrix));
	memset(tmp, 0, sizeof(tmp));

	config.id = (1 << stream_id);
	bitrate_info.id = (1 << stream_id);
	qmatrix.cid = IAV_H264CFG_Q_MATRIX;
	qm = &qmatrix.arg.h_qm;
	qm->id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG_EX, &config);
	AM_IOCTL(fd_iav, IAV_IOC_GET_BITRATE_EX, &bitrate_info);
	AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_CFG_EX, &qmatrix);

	switch (config.profile) {
	case H264_BASELINE_PROFILE:
		strcpy(tmp, "Baseline");
		break;
	case H264_MAIN_PROFILE:
		strcpy(tmp, "Main");
		break;
	case H264_HIGH_PROFILE:
		strcpy(tmp, "High");
		qm_flag = 1;
		break;
	default:
		strcpy(tmp, "Unknown");
		break;
	}
	BOLD_PRINT0("\tH.264\n");
	printf("\t        profile = %s\n", tmp);
	printf("\t              M = %d\n", config.M);
	printf("\t              N = %d\n", config.N);
	printf("\t   idr interval = %d\n", config.idr_interval);
	printf("\t      gop model = %s\n", (config.gop_model == 0)? "simple":"advanced");
	printf("\t deblock filter = %s\n",
		(config.deblocking_filter_disable == 0)? "enable":"disable");
	printf("\t          alpha = %d\n", config.slice_alpha_c0_offset_div2);
	printf("\t           beta = %d\n", config.slice_beta_offset_div2);
	printf("\t        bitrate = %d bps\n", config.average_bitrate);
	printf("\t  chrome format = %s\n",
		(config.chroma_format == IAV_H264_CHROMA_FORMAT_YUV420) ?
		"YUV420" : "MONO");
	printf("\t    intrabias_P = %d\n", config.intrabias_P);
	printf("\t    intrabias_B = %d\n", config.intrabias_B);
	printf("\t    bias_P_skip = %d\n", config.nonSkipCandidate_bias);
	printf("\tcpb reset ratio = %d/%d\n", config.cpb_underflow_num,
		config.cpb_underflow_den);
	printf("\t  zmv threshold = %d\n", config.zmv_threshold);
	printf("\t  mode bias I4  = %d\n", config.mode_bias_I4Add);
	printf("\t  mode bias I16 = %d\n", config.mode_bias_I16Add);

	// four kinds of bitrate control method
	switch (bitrate_info.rate_control_mode) {
	case IAV_CBR:
		strcpy(tmp, "cbr");
		break;
	case IAV_VBR:
		strcpy(tmp, "vbr");
		break;
	case IAV_CBR_QUALITY_KEEPING:
		strcpy(tmp, "cbr-quality");
		break;
	case IAV_VBR_QUALITY_KEEPING:
		strcpy(tmp, "vbr-quality");
		break;
	case IAV_CBR2:
		strcpy(tmp, "cbr2");
		break;
	case IAV_VBR2:
		strcpy(tmp, "vbr2");
		break;
	}
	printf("\t   bitrate ctrl = %s\n", tmp);
	printf("\t           ar_x = %d\n", config.pic_info.ar_x);
	printf("\t           ar_y = %d\n", config.pic_info.ar_y);
	printf("\t     frame mode = %d\n", config.pic_info.frame_mode);
	printf("\t           rate = %d\n", config.pic_info.rate);
	printf("\t          scale = %d\n", config.pic_info.scale);

	if (qm_flag) {
		printf("\n\t H264 Q Matrix:\n");
		printf("\t   Intra Y / U / V :\n");
		for (i = 0; i < 4; ++i) {
			printf("\t      %2d  %2d  %2d  %2d\n",
				qm->qm_4x4[0][i*4], qm->qm_4x4[0][i*4+1],
				qm->qm_4x4[0][i*4+2], qm->qm_4x4[0][i*4+3]);
		}
		printf("\t   Inter Y / U / V :\n");
		for (i = 0; i < 4; ++i) {
			printf("\t      %2d  %2d  %2d  %2d\n",
				qm->qm_4x4[3][i*4], qm->qm_4x4[3][i*4+1],
				qm->qm_4x4[3][i*4+2], qm->qm_4x4[3][i*4+3]);
		}
	}
	printf("\n");

	return 0;
}

static int show_mjpeg_encode_config(int stream_id,
	iav_encode_format_ex_t * format)
{
	iav_jpeg_config_ex_t config;
	memset(&config, 0, sizeof(config));
	config.id = (1 << stream_id);
	AM_IOCTL(fd_iav, IAV_IOC_GET_JPEG_CONFIG_EX, &config);
	BOLD_PRINT0("\tMJPEG\n");
	printf("\tQuality factor = %d\n", config.quality);
	printf("\t Chroma format = %s\n",
		(config.chroma_format == IAV_JPEG_CHROMA_FORMAT_YUV420) ?
		"YUV420" : "MONO");

	return 0;
}

static int show_encode_config(void)
{
	int i;
	iav_encode_format_ex_t  format;

	printf("\n[Encode stream config]:\n");
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		memset(&format, 0, sizeof(format));
		format.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);

		COLOR_PRINT(" Stream %c\n ", i + 'A');
		if (format.encode_type == IAV_ENCODE_H264) {
			if (show_h264_encode_config(i, &format) < 0)
				return -1;
		} else if (format.encode_type == IAV_ENCODE_MJPEG) {
			if (show_mjpeg_encode_config(i, &format) < 0)
				return -1;
		} else {
			printf("\tencoding not configured\n");
		}
	}
	return 0;
}

static int show_state(void)
{
	iav_state_info_t info;

	AM_IOCTL(fd_iav, IAV_IOC_GET_STATE_INFO, &info);

	printf("\n[System state]:\n");
	printf("    vout_irq_count = %d\n", info.vout_irq_count);
	printf("     vin_irq_count = %d\n", info.vin_irq_count);
	printf("    vdsp_irq_count = %d\n", info.vdsp_irq_count);
	printf("             state = %d [%s]\n", info.state,
		get_state_str(info.state));
	printf("       dsp_op_mode = %d [%s]\n", info.dsp_op_mode,
		get_dsp_op_mode_str(info.dsp_op_mode));
	printf("  dsp_encode_state = %d [%s]\n", info.dsp_encode_state,
		get_dsp_encode_state_str(info.dsp_encode_state));
	printf("   dsp_encode_mode = %d [%s]\n", info.dsp_encode_mode,
		get_dsp_encode_mode_str(info.dsp_encode_mode));
	printf("  dsp_decode_state = %d [%s]\n", info.dsp_decode_state,
		get_dsp_decode_state_str(info.dsp_decode_state));
	printf("      decode_state = %d [%s]\n", info.decode_state,
		get_dsp_decode_mode_str(info.decode_state));
	printf("   encode timecode = %d\n", info.encode_timecode);
	printf("        encode pts = %d\n", info.encode_pts);

	return 0;
}

static int show_driver_info(void)
{
	driver_version_t iav_driver_info;

	memset(&iav_driver_info, 0, sizeof(iav_driver_info));
	AM_IOCTL(fd_iav, IAV_IOC_GET_DRIVER_INFO, &iav_driver_info);

	printf("\n[IAV driver info]:\n");
	printf("   IAV Driver Version : %s-%d.%d.%d (Last updated: %x)\n"
		"   DSP Driver version : %d-%d\n",
		iav_driver_info.description, iav_driver_info.major,
		iav_driver_info.minor, iav_driver_info.patch,
		iav_driver_info.mod_time,
		iav_driver_info.api_version, iav_driver_info.idsp_version);

	return 0;
}

static int show_chip_info(void)
{
	iav_chip_s2_id chip_id;
	char chip_str[128];

	memset(&chip_id, 0, sizeof(chip_id));
	AM_IOCTL(fd_iav, IAV_IOC_GET_CHIP_ID_EX, &chip_id);

	memset(chip_str, 0, sizeof(chip_str));
	get_chip_id_str(chip_id, chip_str);

	printf("\n[Chip version]:\n");
	printf("   CHIP Version : %s\n", chip_str);

	return 0;
}

static int show_encode_mode_capability(void)
{
	iav_encmode_cap_ex_t cap;
	char temp[32];
	char flag[2][16] = {"Disabled", "Enabled"};

	memset(&cap, 0, sizeof(cap));
	cap.encode_mode = IAV_ENCODE_CURRENT_MODE;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_ENCMODE_CAP_EX, &cap);

	switch (cap.encode_mode) {
	case IAV_ENCODE_FULL_FRAMERATE_MODE:
		strcpy(temp, "Full frame rate");
		break;
	case IAV_ENCODE_WARP_MODE:
		strcpy(temp, "Multi region warping");
		break;
	case IAV_ENCODE_HIGH_MEGA_MODE:
		strcpy(temp, "High mega pixel");
		break;
	case IAV_ENCODE_CALIBRATION_MODE:
		strcpy(temp, "Calibration");
		break;
	case IAV_ENCODE_HDR_FRAME_MODE:
		strcpy(temp, "HDR frame");
		break;
	case IAV_ENCODE_HDR_LINE_MODE:
		strcpy(temp, "HDR line");
		break;
	case IAV_ENCODE_HIGH_MP_FULL_PERF_MODE:
		strcpy(temp, "High MP (Full performance)");
		break;
	case IAV_ENCODE_FULL_FPS_FULL_PERF_MODE:
		strcpy(temp, "Full FPS (Full performance)");
		break;
	case IAV_ENCODE_MULTI_VIN_MODE:
		strcpy(temp, "Multiple VIN");
		break;
	case IAV_ENCODE_HISO_VIDEO_MODE:
		strcpy(temp, "Ultra Low Light (ULL)");
		break;
	case IAV_ENCODE_HIGH_MP_WARP_MODE:
		strcpy(temp, "High MP Warp");
		break;
	default :
		sprintf(temp, "Unknown mode [%d]", cap.encode_mode);
		break;
	}

	printf("\n[Encode mode capability]:\n");
	printf("  Encode mode : [%s]\n", temp);
	printf("  Supported main buffer size : Min [%dx%d], Max [%dx%d]\n",
		cap.main_width_min, cap.main_height_min,
		cap.main_width_max, cap.main_height_max);
	printf("  Supported max encode streams : %d\n", cap.max_streams_num);
	printf("  Supported min encode stream size : %dx%d\n",
		cap.min_encode_width, cap.min_encode_height);
	printf("  Supported max chroma noise filter strength : %d\n",
		cap.max_chroma_noise_strength);
	printf("  Supported max encode macro blocks : [%d]\n", cap.max_encode_MB);
	printf("\n  Supported possible functions :\n");
	printf("                 Sharpen B : %s\n", flag[cap.sharpen_b_possible]);
	printf("                   Mixer B : %s\n", flag[cap.mixer_b_possible]);
	printf("      Vout B Letter boxing : %s\n", flag[cap.vout_b_letter_box_possible]);
	printf("         Enhance YUV Input : %s\n", flag[cap.yuv_input_enhanced_possible]);
	printf("           Stream Rotation : %s\n", flag[cap.rotate_possible]);
	printf("               Raw Capture : %s\n", flag[cap.raw_cap_possible]);
	printf("    Raw Statistics Capture : %s\n", flag[cap.raw_stat_cap_possible]);
	printf("        Digital PTZ Type I : %s\n", flag[cap.dptz_I_possible]);
	printf("       Digital PTZ Type II : %s\n", flag[cap.dptz_II_possible]);
	printf("    VIN Capture Win offset : %s\n",
		flag[cap.vin_cap_offset_possible]);
	printf("             H-warp bypass : %s\n", flag[cap.hwarp_bypass_possible]);
	printf("                     SVC-T : %s\n", flag[cap.svc_t_possible]);
	printf("           Encode From YUV : %s\n", flag[cap.enc_from_yuv_possible]);
	printf("           Encode From RAW : %s\n", flag[cap.enc_from_raw_possible]);
	printf("                 VOUT swap : %s\n", flag[cap.vout_swap_possible]);
	printf("    MCTF based Privacy Mask (MB level) : %s\n",
		flag[cap.mctf_pm_possible]);
	printf("  HDR based Privacy Mask (Pixel level) : %s\n",
		flag[cap.hdr_pm_possible]);
	printf("              Video Freeze : %s\n", flag[cap.video_freeze_possible]);
	printf("         Map DSP Partition : %s\n", flag[cap.map_dsp_partition]);

	printf("\n");

	return 0;
}

int set_h264_encode_param(int stream)
{
	iav_bitrate_info_ex_t bitrate_info;
	iav_change_gop_ex_t change_gop;
	iav_stream_cfg_t stream_cfg;
	iav_h264_config_ex_t config;
	iav_chroma_format_info_ex_t chroma_format_info;
	h264_param_t * param = &stream_encode_param[stream].h264_param;

	memset(&config, 0, sizeof(config));
	config.id =  (1 << stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG_EX, &config);
	memset(&bitrate_info, 0, sizeof(bitrate_info));
	bitrate_info.id = (1 << stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_BITRATE_EX, &bitrate_info);

	if (param->h264_M_changed)
		config.M = param->h264_M;

	if (param->h264_N_changed)
		config.N = param->h264_N;

	if (param->debug_h264_ref_P_changed)
		config.numRef_P = param->debug_h264_ref_P;

	if (param->h264_idr_interval_changed)
		config.idr_interval = param->h264_idr_interval;

	if (param->h264_gop_model_changed)
		config.gop_model = param->h264_gop_model;

	if (param->debug_h264_long_p_interval_changed)
		config.debug_long_p_interval = param->debug_h264_long_p_interval;

	// deblocking filter params
	if (param->h264_deblocking_filter_alpha_changed)
		config.slice_alpha_c0_offset_div2 = param->h264_deblocking_filter_alpha;

	if (param->h264_deblocking_filter_beta_changed)
		config.slice_beta_offset_div2 = param->h264_deblocking_filter_beta;

	if (param->h264_deblocking_filter_disable_changed)
		config.deblocking_filter_disable = param->h264_deblocking_filter_disable;

	if (param->h264_profile_level_changed)
		config.profile = param->h264_profile_level;

	if (param->h264_qmatrix_4x4_changed)
		config.qmatrix_4x4_enable = param->h264_qmatrix_4x4;

	// force first field or two fields to be I-frame setting
	if (param->force_intlc_tb_iframe_changed)
		config.force_intlc_tb_iframe = param->force_intlc_tb_iframe;

	if (param->h264_chrome_format_changed)
		config.chroma_format = param->h264_chrome_format;

	// bitrate control settings
	if (param->h264_bitrate_control_changed)
		bitrate_info.rate_control_mode = param->h264_bitrate_control;

	if (param->h264_cbr_bitrate_changed)
		bitrate_info.cbr_avg_bitrate = param->h264_cbr_avg_bitrate;

	if (param->h264_vbr_bitrate_changed) {
		bitrate_info.vbr_min_bitrate = param->h264_vbr_min_bitrate;
		bitrate_info.vbr_max_bitrate = param->h264_vbr_max_bitrate;
	}

	// panic ratio setting
	if (param->panic_ratio_changed) {
		config.panic_num = param->panic_num;
		config.panic_den = param->panic_den;
	}

	// h264 syntax settings
	if (param->au_type_changed) {
		config.au_type = param->au_type;
	}

	AM_IOCTL(fd_iav, IAV_IOC_SET_H264_CONFIG_EX, &config);

	// Following configurations can be changed during encoding
	if (param->h264_bitrate_control_changed || param->h264_cbr_bitrate_changed ||
		param->h264_vbr_bitrate_changed) {
		AM_IOCTL(fd_iav, IAV_IOC_SET_BITRATE_EX, &bitrate_info);
	}

	if (param->h264_N_changed || param->h264_idr_interval_changed) {
		change_gop.id = (1 << stream);
		change_gop.N = config.N;
		change_gop.idr_interval = config.idr_interval;
		AM_IOCTL(fd_iav, IAV_IOC_CHANGE_GOP_EX, &change_gop);
	}

	if (param->panic_ratio_changed) {
		stream_cfg.cid = IAV_H264CFG_PANIC;
		stream_cfg.arg.h_panic.id = (1 << stream);
		stream_cfg.arg.h_panic.panic_num = param->panic_num;
		stream_cfg.arg.h_panic.panic_den = param->panic_den;
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CFG_EX, &stream_cfg);
	}

	if (param->h264_chrome_format_changed) {
		memset(&chroma_format_info, 0, sizeof(chroma_format_info));
		chroma_format_info.id = (1 << stream);
		chroma_format_info.chroma_format = param->h264_chrome_format;
		AM_IOCTL(fd_iav,IAV_IOC_SET_CHROMA_FORMAT_EX,&chroma_format_info);
	}

	return 0;
}

static int set_mjpeg_encode_param(int stream)
{
	jpeg_param_t * param = &(stream_encode_param[stream].jpeg_param);
	iav_jpeg_config_ex_t config;
	iav_chroma_format_info_ex_t chroma_format_info;

	config.id = (1 << stream);

	AM_IOCTL(fd_iav, IAV_IOC_GET_JPEG_CONFIG_EX, &config);

	if (param->quality_changed)
		config.quality = param->quality;

	AM_IOCTL(fd_iav, IAV_IOC_SET_JPEG_CONFIG_EX, &config);

	// Following configurations can be changed during encoding
	if (param->jpeg_chrome_format_changed) {
		memset(&chroma_format_info, 0, sizeof(chroma_format_info));
		chroma_format_info.id = (1 << stream);
		chroma_format_info.chroma_format = param->jpeg_chrome_format;
		AM_IOCTL(fd_iav,IAV_IOC_SET_CHROMA_FORMAT_EX,&chroma_format_info);
	}

	return 0;
}

static int set_stream_encode_param(void)
{
	int i;
	iav_encode_format_ex_t  format;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (stream_encode_param_changed_map & (1 << i)) {
			format.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);

			if (format.encode_type == IAV_ENCODE_H264) {
				if (set_h264_encode_param(i) < 0)
					return -1;
			}
			else if (format.encode_type == IAV_ENCODE_MJPEG) {
				if (set_mjpeg_encode_param(i) < 0)
					return -1;
			}
			else
				return -1;
		}
	}
	return 0;
}

static int set_stream_encode_format(void)
{
	iav_encode_format_ex_t format;
	int i;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (stream_encode_format_changed_map & (1 << i)) {
			format.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);
			if (stream_encode_format[i].type_changed) {
				format.encode_type = stream_encode_format[i].type;
			}
			if (stream_encode_format[i].resolution_changed) {
				format.encode_width = stream_encode_format[i].width;
				format.encode_height = stream_encode_format[i].height;
			}
			if (stream_encode_format[i].offset_changed) {
				format.encode_x = stream_encode_format[i].offset_x;
				format.encode_y = stream_encode_format[i].offset_y;
			}
			if (stream_encode_format[i].source_changed) {
				format.source = stream_encode_format[i].source;
			}
			if (stream_encode_format[i].duration_changed) {
				format.duration = stream_encode_format[i].duration;
			}
			if (stream_encode_format[i].hflip_changed) {
				format.hflip = stream_encode_format[i].hflip;
			}
			if (stream_encode_format[i].vflip_changed) {
				format.vflip = stream_encode_format[i].vflip;
			}
			if (stream_encode_format[i].rotate_changed) {
				format.rotate_clockwise = stream_encode_format[i].rotate;
			}
			if (stream_encode_format[i].negative_offset_disable_changed) {
				format.negative_offset_disable = stream_encode_format[i].negative_offset_disable;
			}

			AM_IOCTL(fd_iav, IAV_IOC_SET_ENCODE_FORMAT_EX, &format);
		}
	}
	return 0;
}

static int set_vin_cap_offset(void)
{
	iav_rect_ex_t vin_cap;

	memset(&vin_cap, 0, sizeof(vin_cap));
	AM_IOCTL(fd_iav, IAV_IOC_GET_VIN_CAPTURE_WINDOW, &vin_cap);
	vin_cap.x = vin_cap_start_x;
	vin_cap.y = vin_cap_start_y;
	AM_IOCTL(fd_iav, IAV_IOC_SET_VIN_CAPTURE_WINDOW, &vin_cap);

	return 0;
}

static int set_frame_drop(void)
{
	iav_stream_cfg_t stream_cfg;
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (frame_drop_info_changed_map & (1 << i)) {
			stream_cfg.cid = IAV_STMCFG_FRAME_DROP;
			stream_cfg.arg.frame_drop.id = (1 << i);
			stream_cfg.arg.frame_drop.drop_frames_num = frame_drop_info[i].drop_frames_num;
			printf("Stream [%d] drops %d frames\n", i, frame_drop_info[i].drop_frames_num);
			AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CFG_EX, &stream_cfg);
		}
	}
	return 0;
}

static int set_vcap_param(void)
{
	iav_apply_flag_t flags[IAV_VCAP_PROC_NUM];
	iav_vcap_proc_t vcap_cfg;

	memset(flags, 0, sizeof(flags));
	if (vcap_param.video_freeze_flag) {
		vcap_cfg.vid = IAV_VCAP_FREEZE;
		vcap_cfg.arg.video_freeze = vcap_param.video_freeze;
		AM_IOCTL(fd_iav, IAV_IOC_CFG_VCAP_PROC, &vcap_cfg);
		flags[IAV_VCAP_FREEZE].apply = 1;
	}

	AM_IOCTL(fd_iav, IAV_IOC_APPLY_VCAP_PROC, &flags);
	return 0;
}

static int goto_idle(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_ENTER_IDLE, 0);
	printf("goto_idle done\n");
	return 0;
}

static int enable_preview(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_ENABLE_PREVIEW, 0);
	printf("enable_preview done\n");
	return 0;
}

static int start_encode(iav_stream_id_t  streamid)
{
	int i;
	iav_encode_stream_info_ex_t info;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		if (streamid & (1 << i)) {
			info.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &info);
			if (info.state == IAV_STREAM_STATE_ENCODING) {
				streamid &= ~(1 << i);
			}
		}
	}
	if (streamid == 0) {
		printf("already in encoding, nothing to do \n");
		return 0;
	}
	// start encode
	AM_IOCTL(fd_iav, IAV_IOC_START_ENCODE_EX, streamid);

	printf("Start encoding for stream 0x%x successfully\n", streamid);
	return 0;
}

//this function will get encode state, if it's encoding, then stop it, otherwise, return 0 and do nothing
static int stop_encode(iav_stream_id_t  streamid)
{
	int i;
	iav_state_info_t info;
	iav_encode_stream_info_ex_t stream_info;

	printf("Stop encoding for stream 0x%x \n", streamid);
	AM_IOCTL(fd_iav, IAV_IOC_GET_STATE_INFO, &info);
	if (info.state != IAV_STATE_ENCODING) {
		return 0;
	}
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		stream_info.id = (1 << i);
		AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info);
		if (stream_info.state != IAV_STREAM_STATE_ENCODING) {
			streamid &= ~(1 << i);
		}
	}
	if (streamid == 0)
		return 0;
	AM_IOCTL(fd_iav, IAV_IOC_STOP_ENCODE_EX, streamid);

	return 0;
}

/* some actions like dump bin may need file access, but that should be put into different unit tests */
static int dump_idsp_bin(void)
{
#if 0
	int fd;
	u8* buffer;
	iav_dump_idsp_info_t dump_info;
	buffer = malloc(1024*8);
	char full_filename[256];

	dump_info.mode = 1;
	dump_info.pBuffer = buffer;
	AM_IOCTL(fd_iav, IAV_IOC_DUMP_IDSP_INFO, &dump_info);
	sleep(1);
	dump_info.mode = 2;
	AM_IOCTL(fd_iav, IAV_IOC_DUMP_IDSP_INFO, &dump_info);
	if (filename[0] == '\0')
		strcpy(filename, default_filename);

	sprintf(full_filename, "%s.bin", filename);

	if ((fd = create_file(full_filename)) < 0)
		return -1;

	if (write_data(fd, buffer, 1024*8) < 0) {
		perror("write(5)");
		return -1;
	}
	dump_info.mode = 0;
	AM_IOCTL(fd_iav, IAV_IOC_DUMP_IDSP_INFO, &dump_info);

	free(buffer);
#endif

	return 0;
}

static int change_frame_rate(void)
{
	iav_change_framerate_factor_ex_t factor;
	int i, flag;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (framerate_factor_changed_map || keep_fps_in_ss_changed_map) {
			memset(&factor, 0, sizeof(factor));
			factor.id = (1 << i);
			flag = 0;
			AM_IOCTL(fd_iav, IAV_IOC_GET_FRAMERATE_FACTOR_EX, &factor);
			if (framerate_factor_changed_map & (1 << i)) {
				factor.ratio_numerator = framerate_factor[i][0];
				factor.ratio_denominator = framerate_factor[i][1];
				printf("Stream [%d] change frame interval %d/%d \n", i,
				framerate_factor[i][0], framerate_factor[i][1]);
				flag = 1;
			}
			if (keep_fps_in_ss_changed_map & (1 << i)) {
				factor.keep_fps_in_ss = framerate_factor[i][2];
				printf("Stream [%d] keep_fps_in_ss %s\n", i,framerate_factor[i][2] ? "enabled": "disabled");
				flag = 1;
			}
			if (flag == 1) {
				AM_IOCTL(fd_iav, IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX, &factor);
			}
		}
	}
	return 0;
}

static int sync_frame_rate(void)
{
	iav_sync_framerate_factor_ex_t sync_framerate;
	int i;
	memset(&sync_framerate, 0, sizeof(sync_framerate));
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (framerate_factor_sync_map & (1 << i)) {
			if (framerate_factor[i][0] > framerate_factor[i][1]) {
				printf("Invalid frame interval value : %d/%d should be less than 1/1\n",
					framerate_factor[i][0],
					framerate_factor[i][1]);
				return -1;
			}
			sync_framerate.enable[i] = 1;
			sync_framerate.framefactor[i].id = (1 << i);
			sync_framerate.framefactor[i].ratio_numerator = framerate_factor[i][0];
			sync_framerate.framefactor[i].ratio_denominator = framerate_factor[i][1];
			printf("Stream [%d] sync frame interval %d/%d \n", i,
				framerate_factor[i][0],
				framerate_factor[i][1]);
		}
	}
	AM_IOCTL(fd_iav, IAV_IOC_SYNC_FRAMERATE_FACTOR_EX, &sync_framerate);
	return 0;
}

static int set_h264_enc_param(void)
{
	iav_h264_enc_param_ex_t enc_param;
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (h264_enc_param_changed_map & (1 << i)) {
			enc_param.id = 1 << i;
			AM_IOCTL(fd_iav, IAV_IOC_GET_H264_ENC_PARAM_EX, &enc_param);

			if (h264_enc_param[i].intrabias_p_changed) {
				enc_param.intrabias_P= h264_enc_param[i].intrabias_p;
				printf("Stream [%d] change intrabias_P = %d.\n",
					i, h264_enc_param[i].intrabias_p);
			}
			if (h264_enc_param[i].intrabias_b_changed) {
				enc_param.intrabias_B = h264_enc_param[i].intrabias_b;
				printf("Stream [%d] change intrabias_B = %d.\n",
					i, h264_enc_param[i].intrabias_b);
			}
			if (h264_enc_param[i].bias_p_skip_changed) {
				enc_param.nonSkipCandidate_bias = h264_enc_param[i].bias_p_skip;
				enc_param.skipCandidate_threshold = h264_enc_param[i].bias_p_skip;
				printf("Stream [%d] P-skip bias = %d.\n",
					i, h264_enc_param[i].bias_p_skip);
			}
			if (h264_enc_param[i].cpb_underflow_changed) {
				enc_param.cpb_underflow_num = h264_enc_param[i].cpb_underflow_num;
				enc_param.cpb_underflow_den = h264_enc_param[i].cpb_underflow_den;
				printf("Stream [%d] CPB underflow ratio = %d/%d.\n", i,
					enc_param.cpb_underflow_num, enc_param.cpb_underflow_den);
			}
			if (h264_enc_param[i].zmv_threshold_changed) {
				enc_param.zmv_threshold = h264_enc_param[i].zmv_threshold;
				printf("Stream [%d] ZMV threshold = %d.\n", i,
					enc_param.zmv_threshold);
			}
			if (h264_enc_param[i].mode_bias_I4_add_changed) {
				enc_param.mode_bias_I4Add = h264_enc_param[i].mode_bias_I4_add;
				printf("Stream [%d] mode bias I4 = %d.\n", i,
					enc_param.mode_bias_I4Add);
			}
			if (h264_enc_param[i].mode_bias_I16_add_changed) {
				enc_param.mode_bias_I16Add = h264_enc_param[i].mode_bias_I16_add;
				printf("Stream [%d] mode bias I16 = %d.\n", i,
					enc_param.mode_bias_I16Add);
			}
			if (h264_enc_param[i].mode_bias_Inter8Add_changed) {
				enc_param.mode_bias_Inter8Add = h264_enc_param[i].mode_bias_Inter8Add;
				printf("Stream [%d] mode bias Inter8 = %d.\n", i,
					enc_param.mode_bias_Inter8Add);
			}
			if (h264_enc_param[i].mode_bias_Inter16Add_changed) {
				enc_param.mode_bias_Inter16Add = h264_enc_param[i].mode_bias_Inter16Add;
				printf("Stream [%d] mode bias Inter16 = %d.\n", i,
					enc_param.mode_bias_Inter16Add);
			}
			if (h264_enc_param[i].mode_bias_DirectAdd_changed) {
				enc_param.mode_bias_DirectAdd = h264_enc_param[i].mode_bias_DirectAdd;
				printf("Stream [%d] mode bias direct = %d.\n", i,
					enc_param.mode_bias_DirectAdd);
			}
			AM_IOCTL(fd_iav, IAV_IOC_SET_H264_ENC_PARAM_EX, &enc_param);
		}
	}
	return 0;
}

static int setup_system_info(void)
{
	iav_system_setup_info_ex_t info_setup;

	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_SETUP_INFO_EX, &info_setup);
	if (system_info_setup.bits_read_mode_changed) {
		info_setup.coded_bits_interrupt_enable = system_info_setup.bits_read_mode;
	}
	if (system_info_setup.cmd_read_mode_changed) {
		info_setup.cmd_read_delay = system_info_setup.cmd_read_mode;
	}
	if (system_info_setup.vout_swap_changed) {
		info_setup.vout_swap = system_info_setup.vout_swap;
	}
	if (system_info_setup.pm_type_changed) {
		info_setup.mctf_privacy_mask = system_info_setup.pm_type;
	}
	if (system_info_setup.eis_delay_count_changed) {
		info_setup.eis_delay_count = system_info_setup.eis_delay_count;
	}

	if (system_info_setup.osd_mixer_changed) {
		if (OSD_BLENDING_FROM_MIXER_A == system_info_setup.osd_mixer) {
			info_setup.voutA_osd_blend_enable = 1;
			info_setup.voutB_osd_blend_enable = 0;
		} else if (OSD_BLENDING_FROM_MIXER_B == system_info_setup.osd_mixer) {
			info_setup.voutA_osd_blend_enable = 0;
			info_setup.voutB_osd_blend_enable = 1;
		} else {
			info_setup.voutA_osd_blend_enable = 0;
			info_setup.voutB_osd_blend_enable = 0;
		}
	}
	if (system_info_setup.dsp_noncache_changed) {
		info_setup.dsp_noncached = system_info_setup.dsp_noncache;
	}

	if (system_info_setup.debug_enc_dummy_latency_flag) {
		info_setup.debug_enc_dummy_latency_count =
			system_info_setup.debug_enc_dummy_latency_count;
	}
	AM_IOCTL(fd_iav, IAV_IOC_SET_SYSTEM_SETUP_INFO_EX, &info_setup);

	return 0;
}

static int setup_system_resource(void)
{
	int i;
	iav_system_resource_setup_ex_t  resource_setup;

	memset(&resource_setup, 0, sizeof(resource_setup));
	if (system_resource_setup.encode_mode_changed) {
		resource_setup.encode_mode = system_resource_setup.encode_mode;
	} else {
		resource_setup.encode_mode = IAV_ENCODE_CURRENT_MODE;
	}
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup);

	for (i= 0; i < MAX_SOURCE_BUFFER_NUM; ++i) {
		if (system_resource_setup.buffer_max_changed_map & (1 << i)) {
			resource_setup.buffer_max_size[i].width =
				system_resource_setup.buffer_max_width[i];
			resource_setup.buffer_max_size[i].height =
				system_resource_setup.buffer_max_height[i];
		}
		if ((i >= MAIN_SOURCE_DRAM) &&
			(system_resource_setup.max_dram_frame_map & (1 << i))) {
			resource_setup.max_dram_frame[i - MAIN_SOURCE_DRAM] =
				system_resource_setup.max_dram_frame[i - MAIN_SOURCE_DRAM];
		}
		if ((i < IAV_ENCODE_SUB_SOURCE_BUFFER_LAST) &&
			(system_resource_setup.extra_dram_buf_changed_map & (1 << i))) {
			resource_setup.extra_dram_buf[i] = system_resource_setup.extra_dram_buf[i];
		}
	}

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		if (system_resource_setup.stream_max_changed_map & (1 << i)) {
			resource_setup.stream_max_size[i].width =
				system_resource_setup.stream_max_width[i];
			resource_setup.stream_max_size[i].height =
				system_resource_setup.stream_max_height[i];
		}

		if (system_resource_setup.GOP_max_M_changed_map & (1 << i)) {
			resource_setup.stream_max_GOP_M[i] =
				system_resource_setup.GOP_max_M[i];
		}

		if (system_resource_setup.stream_2x_search_range_changed_map & (1 << i)) {
			resource_setup.stream_2x_search_range[i] =
				system_resource_setup.stream_2x_search_range[i];
		}

		if (system_resource_setup.debug_max_ref_P_changed_map & (1 << i)) {
			resource_setup.debug_max_ref_P[i] =
				system_resource_setup.debug_max_ref_P[i];
		}

	}

	if (system_resource_setup.stream_max_num_changed) {
		resource_setup.max_num_encode_streams = system_resource_setup.stream_max_num;
	}

	if (system_resource_setup.sharpen_b_changed) {
		resource_setup.sharpen_b_enable = system_resource_setup.sharpen_b;
	}

	if (system_resource_setup.rotate_possible_changed) {
		resource_setup.rotate_possible = system_resource_setup.rotate_possible;
	}

	if (system_resource_setup.exposure_num_changed) {
		resource_setup.exposure_num = system_resource_setup.exposure_num;
	}

	if (system_resource_setup.vin_num_changed) {
		resource_setup.vin_num = system_resource_setup.vin_num;
	}

	if (system_resource_setup.hwarp_bypass_possible_changed) {
		resource_setup.hwarp_bypass_possible = system_resource_setup.hwarp_bypass_possible;
	}

	if (system_resource_setup.raw_capture_enabled_changed) {
		resource_setup.raw_capture_enable = system_resource_setup.raw_capture_enabled;
	}

	if (system_resource_setup.enc_from_raw_changed) {
		resource_setup.enc_from_raw_enable = system_resource_setup.enc_from_raw;
	}

	if (system_resource_setup.raw_stats_lines_changed) {
		resource_setup.max_vin_stats_lines_top = system_resource_setup.raw_stats_lines_top;
		resource_setup.max_vin_stats_lines_bottom = system_resource_setup.raw_stats_lines_bottom;
	}

	if (system_resource_setup.warp_max_input_width_changed) {
		resource_setup.max_warp_input_width = system_resource_setup.warp_max_input_width;
	}
	if (system_resource_setup.warp_max_output_width_changed) {
		resource_setup.max_warp_output_width = system_resource_setup.warp_max_output_width;
	}

	if (system_resource_setup.max_chroma_noise_shift_changed) {
		resource_setup.max_chroma_noise_shift = system_resource_setup.max_chroma_noise_shift;
	}

	if (system_resource_setup.vskip_before_encode_changed) {
		resource_setup.vskip_before_encode = system_resource_setup.vskip_before_encode;
	}

	if (system_resource_setup.extra_2x_enable_changed) {
		resource_setup.extra_2x_zoom_enable = system_resource_setup.extra_2x_enable;
	}

	if (system_resource_setup.vca_buffer_enable_changed) {
		resource_setup.vca_buf_src_id = system_resource_setup.vca_buffer_enable;
	}

	if (system_resource_setup.vout_b_letter_box_enable_changed) {
		resource_setup.vout_b_letter_box_enable = system_resource_setup.vout_b_letter_box_enable;
	}

	if (system_resource_setup.map_dsp_partition_changed) {
		resource_setup.map_dsp_partition = system_resource_setup.map_dsp_partition;
	}

	if (system_resource_setup.debug_chip_id_flag) {
		resource_setup.debug_chip_id = system_resource_setup.debug_chip_id;
	}

	if (system_resource_setup.yuv_input_enhanced_changed) {
		resource_setup.yuv_input_enhanced = system_resource_setup.yuv_input_enhanced;
	}
	resource_setup.mixer_b_enable = (vout_flag[VOUT_1] &&
		(vout_display_input[VOUT_1] == AMBA_VOUT_INPUT_FROM_MIXER));

	AM_IOCTL(fd_iav, IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX, &resource_setup);

	return 0;
}

static int setup_source_buffer(void)
{
	int i;
	iav_source_buffer_setup_ex_t buffer_setup;

	AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &buffer_setup);

	for (i = 0; i < MAX_SOURCE_BUFFER_NUM; ++ i) {
		if (source_buffer_setup.buffer_type_changed_map & (1 << i)) {
			buffer_setup.type[i] = source_buffer_setup.buffer_type[i];
		}

		if (source_buffer_setup.buffer_unwarp_changed_map & (1 << i)) {
			buffer_setup.unwarp[i] = source_buffer_setup.unwarp[i];
		}

		if (source_buffer_format_changed_map & (1 << i)) {
			if (source_buffer_format[i].resolution_changed) {
				buffer_setup.size[i].width = source_buffer_format[i].width;
				buffer_setup.size[i].height = source_buffer_format[i].height;
				if (i == MAIN_SOURCE_BUFFER) {
					memset(&buffer_setup.input, 0, sizeof(buffer_setup.input));
				} else {
					memset(&buffer_setup.input[i], 0, sizeof(buffer_setup.input[i]));
				}
			}
			if (source_buffer_format[i].input_size_changed) {
				buffer_setup.input[i].width = source_buffer_format[i].input_width;
				buffer_setup.input[i].height = source_buffer_format[i].input_height;
			}
			if (source_buffer_format[i].input_offset_changed) {
				buffer_setup.input[i].x = source_buffer_format[i].input_x;
				buffer_setup.input[i].y = source_buffer_format[i].input_y;
			}
		}
	}

	i = PRE_MAIN_BUFFER;
	if (source_buffer_format_changed_map & (1 << i)) {
		if (source_buffer_format[i].resolution_changed) {
			buffer_setup.pre_main.width = source_buffer_format[i].width;
			buffer_setup.pre_main.height = source_buffer_format[i].height;
			memset(&buffer_setup.pre_main_input, 0, sizeof(buffer_setup.pre_main_input));
		}
		if (source_buffer_format[i].input_size_changed) {
			buffer_setup.pre_main_input.width = source_buffer_format[i].input_width;
			buffer_setup.pre_main_input.height = source_buffer_format[i].input_height;
		}
		if (source_buffer_format[i].input_offset_changed) {
			buffer_setup.pre_main_input.x = source_buffer_format[i].input_x;
			buffer_setup.pre_main_input.y = source_buffer_format[i].input_y;
		}
	}

	AM_IOCTL(fd_iav, IAV_IOC_SET_SOURCE_BUFFER_SETUP_EX, &buffer_setup);

	source_buffer_format_changed_map = 0;
	return 0;
}

static int set_source_buffer_format(void)
{
	iav_source_buffer_format_ex_t buf_format;
	int i;

	for (i = SECOND_SOURCE_BUFFER; i < MAX_SOURCE_BUFFER_NUM; ++ i) {
		if (source_buffer_format_changed_map & (1 << i)) {
			buf_format.source = i;
			AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX, &buf_format);
			if (source_buffer_format[i].input_size_changed) {
				buf_format.input.width = source_buffer_format[i].input_width;
				buf_format.input.height = source_buffer_format[i].input_height;
			} else {
				buf_format.input.width = buf_format.input.height = 0;
			}

			if (source_buffer_format[i].input_offset_changed) {
				buf_format.input.x = source_buffer_format[i].input_x;
				buf_format.input.y = source_buffer_format[i].input_y;
			} else {
				buf_format.input.x = buf_format.input.y = 0;
			}

			if (source_buffer_format[i].resolution_changed) {
				buf_format.size.width = source_buffer_format[i].width;
				buf_format.size.height = source_buffer_format[i].height;
			}
			AM_IOCTL(fd_iav, IAV_IOC_SET_SOURCE_BUFFER_FORMAT_EX, &buf_format);
		}
	}

	return 0;
}

static int init_default_value(void)
{
	return 0;
}

static int force_idr_insertion(iav_stream_id_t  stream_id)
{
	if (ioctl(fd_iav, IAV_IOC_FORCE_IDR_EX, stream_id)  < 0) {
		perror("IAV_IOC_FORCE_IDR_EX");
		printf("force idr for stream id 0x%x Failed \n", stream_id);
		return -1;
	} else {
		printf("force idr for stream id 0x%x OK \n", stream_id);
		return 0;
	}
}

static int change_qp_limit(void)
{
	qp_limit_params_t * param;
	iav_change_qp_limit_ex_t limit;
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (qp_limit_changed_map & (1 << i)) {
			limit.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_GET_QP_LIMIT_EX, &limit);
			param = &qp_limit[i];
			if (param->i_changed) {
				limit.qp_min_on_I = param->min_i;
				limit.qp_max_on_I = param->max_i;
			}

			if (param->p_changed) {
				limit.qp_min_on_P = param->min_p;
				limit.qp_max_on_P = param->max_p;
			}

			if (param->b_changed) {
				limit.qp_min_on_B = param->min_b;
				limit.qp_max_on_B = param->max_b;
			}

			if (param->q_changed) {
				limit.qp_min_on_Q = param->min_q;
				limit.qp_max_on_Q = param->max_q;
			}
			if (param->adapt_qp_changed)
				limit.adapt_qp = param->adapt_qp;

			if (param->i_qp_reduce_changed)
				limit.i_qp_reduce = param->i_qp_reduce;

			if (param->p_qp_reduce_changed)
				limit.p_qp_reduce = param->p_qp_reduce;
			if (param->q_qp_reduce_changed)
				limit.q_qp_reduce = param->q_qp_reduce;
			if (param->log_q_num_changed)
				limit.log_q_num_minus_1 = param->log_q_num;

			if (param->skip_frame_changed)
				limit.skip_flag = param->skip_frame;

			AM_IOCTL(fd_iav, IAV_IOC_CHANGE_QP_LIMIT_EX, &limit);
		}
	}

	return 0;
}

static int change_intra_refresh_mb_rows(void)
{
	int i;
	iav_change_intra_mb_rows_ex_t intra_rows;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (intra_mb_rows_changed_map & (1 << i)) {
			intra_rows.id = (1 << i);
			intra_rows.intra_refresh_mb_rows = intra_mb_rows[i];
			AM_IOCTL(fd_iav, IAV_IOC_CHANGE_INTRA_MB_ROWS_EX, &intra_rows);
		}
	}
	return 0;
}

static int change_prev_a_framerate(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_SET_PREV_A_FRAMERATE_DIV_EX, prev_a_framerate_div);
	return 0;
}

static int set_qp_matrix_mode(int stream)
{
	u32 *addr = NULL;
	u32 width, buf_pitch, height, total;
	int i, j;
	iav_encode_format_ex_t format;

	format.id = (1 << stream);
	AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &format);
	addr = (u32 *) (G_qp_matrix_addr
	        + (G_qp_matrix_size / MAX_ENCODE_STREAM_NUM * stream));
	width = ROUND_UP(format.encode_width, 16) / 16;
	buf_pitch = ROUND_UP(width, 8);
	height = ROUND_UP(format.encode_height, 16) / 16;
	total = (buf_pitch * 4) * height; // (((width * 4) * height) + 31) & (~31);
	memset(addr, 0, total);
	printf("set_qp_matrix_mode : %d.\n", qp_matrix[stream].matrix_mode);
	printf("width (pitch) : %d (%d), height : %d, total : %d.\n", width,
	       buf_pitch, height, total);
	switch (qp_matrix[stream].matrix_mode) {
		case 0:
			break;
		case 1:
			for (i = 0; i < height; i++) {
				for (j = 0; j < width / 3; j++)
					addr[i * buf_pitch + j] = 1;
			}
			break;
		case 2:
			for (i = 0; i < height; i++) {
				for (j = width * 2 / 3; j < width; j++)
					addr[i * buf_pitch + j] = 1;
			}
			break;
		case 3:
			for (i = 0; i < height / 3; i++) {
				for (j = 0; j < width; j++)
					addr[i * buf_pitch + j] = 1;
			}
			break;
		case 4:
			for (i = height * 2 / 3; i < height; i++) {
				for (j = 0; j < width; j++)
					addr[i * buf_pitch + j] = 1;
			}
			break;
		case 5:
			for (i = 0; i < height / 2; i++) {
				for (j = 0; j < width / 3; j++)
					addr[i * buf_pitch + j] = 1;
				for (j = width / 3; j < width * 2 / 3; j++)
					addr[i * buf_pitch + j] = 2;
			}
			break;
		default:
			printf("unknow matrix mode %d\n",qp_matrix[stream].matrix_mode);
			break;
	}

	for (i = 0; i < height; i++) {
		printf("\n");
		for (j = 0; j < width; j++) {
			printf("%d ", addr[i * buf_pitch + j]);
		}
	}
	printf("\n");

	return 0;
}

static int change_qp_matrix(void)
{
	int i, j, k, delta_num, frame_type_num;
	iav_qp_roi_matrix_ex_t matrix;
	iav_mmap_info_t qp_info;
	delta_num = sizeof(matrix.delta[0]) / sizeof(matrix.delta[0][0]);
	frame_type_num = sizeof(matrix.delta) / sizeof(matrix.delta[0]);
	printf("delta_num %d, frame_type_num %d\n", delta_num, frame_type_num);

	AM_IOCTL(fd_iav, IAV_IOC_MAP_QP_ROI_MATRIX_EX, &qp_info);
	G_qp_matrix_addr = qp_info.addr;
	G_qp_matrix_size = qp_info.length;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (qp_matrix_changed_map & (1 << i)) {
			matrix.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_GET_QP_ROI_MATRIX_EX, &matrix);

			if (qp_matrix[i].delta_changed) {
				for (j = 0; j < delta_num; ++j) {
					for (k = 0; k < frame_type_num; ++k) {
						matrix.delta[k][j] = qp_matrix[i].delta[j];
					}
				}
			}
			if (qp_matrix[i].matrix_mode_changed) {
				matrix.enable = (qp_matrix[i].matrix_mode != 0);
				if (set_qp_matrix_mode(i) < 0) {
					printf("set_qp_matrix_mode for stream [%d] failed!\n", i);
					return -1;
				}
			}
			AM_IOCTL(fd_iav, IAV_IOC_SET_QP_ROI_MATRIX_EX, &matrix);
		}
	}
	return 0;
}

static int do_show_status(void)
{
	//show encode sream info
	if (show_encode_stream_info_flag) {
		if (show_encode_stream_info() < 0)
			return -1;
	}

	//show source buffer info
	if (show_source_buffer_info_flag) {
		if (show_source_buffer_info() < 0)
			return -1;
	}

	//show codec resource limit info
	if (show_resource_limit_info_flag) {
		if (show_resource_limit_info() < 0)
			return -1;
	}

	if (show_encode_config_flag) {
		if (show_encode_config() < 0)
			return -1;
	}

	if (show_iav_state_flag) {
		if (show_state() < 0)
			return -1;
	}

	if (show_iav_driver_info_flag) {
		if (show_driver_info() < 0)
			return -1;
	}

	if (show_chip_info_flag) {
		if (show_chip_info() < 0)
			return -1;
	}

	if (show_encode_mode_cap_flag) {
		if (show_encode_mode_capability() < 0)
			return -1;
	}

	return 0;
}

static int do_debug_dump(void)
{
	//dump Image DSP setting
	if (dump_idsp_bin_flag) {
		if (dump_idsp_bin() < 0) {
			perror("dump_idsp_bin failed");
			return -1;
		}
	}

	return 0;
}

static int do_stop_encoding(void)
{
	if (stop_encode(stop_stream_id) < 0)
		return -1;
	return 0;
}

static int do_goto_idle(void)
{
	if (goto_idle() < 0)
		return -1;
	return 0;
}

static int do_vout_setup(void)
{
	if (check_vout() < 0) {
		return -1;
	}

	if (dynamically_change_vout())
		return 0;

	if (vout_flag[VOUT_0] && init_vout(VOUT_0, 0) < 0)
		return -1;
	if (vout_flag[VOUT_1] && init_vout(VOUT_1, 0) < 0)
		return -1;

	return 0;
}

static int do_vin_setup(void)
{
	// select channel: for multi channel VIN (initialize)
	if (channel >= 0) {
		if (select_channel() < 0)
			return -1;
	}

	if (!dsp_stat_init_flag) {
		// switch vin operate mode
		if (system_resource_setup.exposure_num_changed) {
			if (set_vin_linear_wdr_mode(
				system_resource_setup.exposure_num - 1) < 0) {
				return -1;
			}
		} else {
			if (sensor_operate_mode_changed) {
				if (set_vin_linear_wdr_mode(sensor_operate_mode) < 0) {
					return -1;
				}
			}
		}
	}

	if (init_vin(vin_mode) < 0)
		return -1;

	return 0;
}

static int do_change_parameter_during_idle(void)
{
	if (system_resource_setup_changed || vout_flag[VOUT_1]) {
		if (setup_system_resource() < 0)
			return -1;
	}

	if (system_info_setup_changed) {
		if (setup_system_info() < 0)
			return -1;
	}

	if (source_buffer_setup_changed || source_buffer_format_changed_map) {
		if (setup_source_buffer() < 0)
			return -1;
	}

	return 0;
}

static int do_enable_preview(void)
{
	if (enable_preview() < 0)
		return -1;

	return 0;
}


static int do_change_encode_format(void)
{
	if (source_buffer_format_changed_map)  {
		if (set_source_buffer_format() < 0) {
			return -1;
		}
	}

	if (stream_encode_format_changed_map) {
		if (set_stream_encode_format() < 0)
			return -1;
	}
	return 0;
}


static int do_change_encode_config(void)
{
	if (set_stream_encode_param() < 0)
		return -1;

	return 0;
}

static int do_start_encoding(void)
{
	if (start_encode(start_stream_id) < 0)
		return -1;
	return 0;
}

static int do_real_time_change(void)
{
	if (mirror_pattern_flag) {
		if (set_vin_mirror_pattern() < 0)
			return -1;
	}

	if (framerate_flag) {
		if (set_vin_frame_rate() < 0)
			return -1;
	}

	if (framerate_factor_changed_map ||
			keep_fps_in_ss_changed_map)  {
		if (change_frame_rate() < 0)
			return -1;
	}

	if (framerate_factor_sync_map) {
		if (sync_frame_rate() < 0)
			return -1;
	}

	if (force_idr_id) {
		if (force_idr_insertion(force_idr_id) <0)
			return -1;
	}

	if (qp_limit_changed_map) {
		if (change_qp_limit() < 0)
			return -1;
	}

	if (intra_mb_rows_changed_map) {
		if (change_intra_refresh_mb_rows() < 0)
			return -1;
	}

	if (prev_a_framerate_div_changed) {
		if (change_prev_a_framerate() < 0) {
			return -1;
		}
	}

	if (qp_matrix_changed_map) {
		if (change_qp_matrix() < 0)
			return -1;
	}

	if (h264_enc_param_changed_map) {
		if(set_h264_enc_param() < 0 )
			return -1;
	}

	if (vin_cap_offset_flag) {
		if (set_vin_cap_offset() < 0) {
			return -1;
		}
	}
	if (frame_drop_info_changed_map) {
		if (set_frame_drop() < 0) {
			return -1;
		}
	}
	if (vcap_param_changed) {
		if (set_vcap_param() < 0) {
			return -1;
		}
	}
	return 0;
}

static int do_iav_debug(void)
{
	iav_debug_setup_t debug;

	if (iav_debug.kernel_print_enable_flag) {
		memset(&debug, 0, sizeof(debug));
		debug.enable = iav_debug.kernel_print_enable;
		debug.flag = IAV_DEBUG_PRINT_ENABLE;
		AM_IOCTL(fd_iav, IAV_IOC_DEBUG_SETUP, &debug);
	}
	if (iav_debug.check_disable_flag) {
		memset(&debug, 0, sizeof(debug));
		debug.enable = iav_debug.check_disable;
		debug.flag = IAV_DEBUG_CHECK_DISABLE;
		AM_IOCTL(fd_iav, IAV_IOC_DEBUG_SETUP, &debug);
	}
	return 0;
}

int main(int argc, char **argv)
{
	int do_show_status_flag = 0;
	int do_debug_dump_flag = 0;
	int do_stop_encoding_flag = 0;
	int do_goto_idle_flag = 0;
	int do_vout_setup_flag = 0;
	int do_vin_setup_flag = 0;
	int do_change_parameter_during_idle_flag  = 0;
	int do_enable_preview_flag = 0;
	int do_change_encode_format_flag = 0;
	int do_change_encode_config_flag = 0;
	int do_start_encoding_flag = 0;
	int do_real_time_change_flag = 0;
	int do_iav_debug_flag = 0;
//	int do_change_encode_mode_flag = 0;
	iav_state_info_t info;

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
		return -1;
	}

	AM_IOCTL(fd_iav, IAV_IOC_GET_STATE_INFO, &info);
	if (info.state == IAV_STATE_INIT) {
		dsp_stat_init_flag = 1;
	}

	if (init_default_value() < 0)
		return -1;

	if (init_param(argc, argv) < 0)
		return -1;

/********************************************************
 *  process parameters to get flag
 *******************************************************/
	//show message flag
	if (show_encode_stream_info_flag ||
		show_source_buffer_info_flag ||
		show_encode_config_flag ||
		show_resource_limit_info_flag ||
		show_iav_state_flag ||
		show_iav_driver_info_flag ||
		show_chip_info_flag ||
		show_encode_mode_cap_flag) {
		do_show_status_flag  = 1;
	}

	//debug dump flag
	if (dump_idsp_bin_flag)  {
		do_debug_dump_flag = 1;
	}

	//stop encoding flag
	stop_stream_id |= restart_stream_id;
	if (vin_flag || source_buffer_setup_changed || system_info_setup_changed
		|| system_resource_setup_changed ||
		(source_buffer_format_changed_map & (1 << MAIN_SOURCE_BUFFER)) ||
		(source_buffer_format_changed_map & (1 << MAIN_SOURCE_DRAM)) ||
		(source_buffer_format_changed_map &  (1 << PRE_MAIN_BUFFER))) {
		stop_stream_id = ALL_ENCODE_STREAMS;
	}
	if (stop_stream_id)  {
		do_stop_encoding_flag = 1;
	}

	//go to idle (disable preview) flag
	if (channel < 0) {
		//channel is -1 means VIN is single channel
		if (vout_flag[VOUT_0] || vout_flag[VOUT_1] || vin_flag ||
			(source_buffer_format_changed_map & (1 << MAIN_SOURCE_BUFFER)) ||
			(source_buffer_format_changed_map & (1 << MAIN_SOURCE_DRAM)) ||
			(source_buffer_format_changed_map &  (1 << PRE_MAIN_BUFFER))||
			source_buffer_setup_changed ||
			system_resource_setup_changed ||
			system_info_setup_changed || sensor_operate_mode_changed ||
			idle_cmd) {
			do_goto_idle_flag = 1;
		}
	}

	//vout setup flag
	if (vout_flag[VOUT_0] || vout_flag[VOUT_1]) {
		do_vout_setup_flag = 1;
	}

	//vin setup flag
	if (vin_flag) {
		do_vin_setup_flag = 1;
	}

	//source buffer flag config during idle flag
	if (do_goto_idle_flag) {
		do_change_parameter_during_idle_flag = 1;
	}

	//enable preview flag
	if (do_goto_idle_flag && !nopreview_flag) {
		do_enable_preview_flag = 1;
	}

	//set encode format flag
	if (stream_encode_format_changed_map || source_buffer_format_changed_map) {
		do_change_encode_format_flag = 1;
	}

	//set encode param flag
	if (stream_encode_param_changed_map) {
		do_change_encode_config_flag = 1;
	}

	//encode start flag
	start_stream_id |= restart_stream_id;
	if (start_stream_id) {
		do_start_encoding_flag = 1;
	}

	//real time change flag
	if (framerate_factor_changed_map ||
		framerate_factor_sync_map ||
		keep_fps_in_ss_changed_map ||
		force_idr_id ||
		qp_limit_changed_map ||
		intra_mb_rows_changed_map ||
		prev_a_framerate_div_changed ||
		qp_matrix_changed_map ||
		h264_enc_param_changed_map ||
		vin_cap_offset_flag ||
		frame_drop_info_changed_map ||
		vcap_param_changed) {
		do_real_time_change_flag = 1;
	}

	if (iav_debug_flag) {
		do_iav_debug_flag = 1;
	}

/********************************************************
 *  check dependency base on flag
 *******************************************************/
/*	if (do_change_encode_mode_flag && !do_vout_setup_flag) {
		printf("Please specify VOUT parameter when encode mode is changed!\n");
		printf("E.g.:\n  test_encode -V 480p --hdmi --enc-mode 0\n");
		printf("  test_encode -V 480p --hdmi --mixer 0 --enc-mode 1\n");
		printf("  test_encode -V 480p --hdmi --enc-mode 3\n");
		return -1;
	}
*/
/********************************************************
 *  execution base on flag
 *******************************************************/
	//show message
	if (do_show_status_flag) {
		if (do_show_status() < 0)
			return -1;
	}

	//debug dump
	if (do_debug_dump_flag) {
		if (do_debug_dump() < 0)
			return -1;
	}

	//stop encoding
	if (do_stop_encoding_flag) {
		if (do_stop_encoding() < 0)
			return -1;
	}

	//disable preview (goto idle)
	if (do_goto_idle_flag) {
		if (do_goto_idle() < 0)
			return -1;
	}

	//vout setup
	if (do_vout_setup_flag) {
		if (do_vout_setup() < 0)
			return -1;
	}

	//vin setup
	if (do_vin_setup_flag) {
		if (do_vin_setup() < 0)
			return -1;
	}

	//change source buffer paramter that needs idle state
	if (do_change_parameter_during_idle_flag) {
		if (do_change_parameter_during_idle() < 0)
			return -1;
	}

	//enable preview
	if (do_enable_preview_flag) {
		if (do_enable_preview() < 0)
			return -1;
	}

	//change encoding format
	if (do_change_encode_format_flag) {
		if (do_change_encode_format() < 0)
			return -1;
	}

	//change encoding param
	if (do_change_encode_config_flag) {
		if (do_change_encode_config() < 0)
			return -1;
	}

	//real time change encoding parameter
	if (do_real_time_change_flag) {
		if (do_real_time_change() < 0)
			return -1;
	}

	//start encoding
	if (do_start_encoding_flag) {
		if (do_start_encoding() < 0)
			return -1;
	}

	if (do_iav_debug_flag) {
		if (do_iav_debug() < 0)
			return -1;
	}

	if (fd_iav >= 0)
		close(fd_iav);

	return 0;
}

