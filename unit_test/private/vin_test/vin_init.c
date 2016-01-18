/*
 * test_vin.c
 *
 * History:
 *	2009/11/17 - [Qiao Wang] create
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include "ambas_vin.h"

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )

extern int fd_iav;

int vin_flag		= 0;
int vin_mode		= 0;

int framerate_flag	= 0;
int framerate		= 0;

int mirror_pattern_flag = 0;
int mirror_pattern	= AMBA_VIN_SRC_MIRROR_AUTO;
int bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_AUTO;

int anti_flicker	= -1;

int channel = -1;
int source = -1; //not specify source

int check_input = -1;

int sensor_operate_mode = 0;
int sensor_operate_mode_changed = 0;

struct amba_vin_src_mirror_mode mirror_mode;

struct vin_mode_s {
	const char *name;
	enum amba_video_mode mode;
} ;

struct vin_mode_s __vin_modes[] = {
	{"0",		AMBA_VIDEO_MODE_AUTO	},
	{"auto",	AMBA_VIDEO_MODE_AUTO	},

	{"480i",	AMBA_VIDEO_MODE_480I	},
	{"576i",	AMBA_VIDEO_MODE_576I	},
	{"480p",	AMBA_VIDEO_MODE_D1_NTSC	},
	{"576p",	AMBA_VIDEO_MODE_D1_PAL	},
	{"720p",	AMBA_VIDEO_MODE_720P	},
	{"1080i",	AMBA_VIDEO_MODE_1080I	},
	{"1080p",	AMBA_VIDEO_MODE_1080P	},	//1080p, max 60fps mode
	{"1080p30", AMBA_VIDEO_MODE_1080P30	},	//1080p, max 30fps mode
	{"1080p60", AMBA_VIDEO_MODE_1080P60	},	//1080p60,add for mt9t002 60fps on A5S

	{"d1",		AMBA_VIDEO_MODE_D1_NTSC	},
	{"d1_pal",	AMBA_VIDEO_MODE_D1_PAL	},

	{"qvga",	AMBA_VIDEO_MODE_320_240	},
	{"vga",		AMBA_VIDEO_MODE_VGA	},
	{"wvga",	AMBA_VIDEO_MODE_WVGA	},
	{"wsvga",	AMBA_VIDEO_MODE_WSVGA	},

	{"qxga",	AMBA_VIDEO_MODE_QXGA	},
	{"xga",		AMBA_VIDEO_MODE_XGA	},
	{"wxga",	AMBA_VIDEO_MODE_WXGA	},
	{"uxga",	AMBA_VIDEO_MODE_UXGA	},
	{"qsxga",	AMBA_VIDEO_MODE_QSXGA	},

	{"3m",		AMBA_VIDEO_MODE_QXGA	},
	{"3m_169",	AMBA_VIDEO_MODE_2304x1296 },//2304x1296, 30fps mode
	{"3m_p30",	AMBA_VIDEO_MODE_2304x1296 },//same as above
	{"3m_p60",	AMBA_VIDEO_MODE_1296P60 },	//2304x1296, 60fps mode

	{"4m",		AMBA_VIDEO_MODE_4M_4_3	},
	{"4m_169",	AMBA_VIDEO_MODE_4M_16_9	},
	{"5m",		AMBA_VIDEO_MODE_QSXGA	},
	{"5m_169",	AMBA_VIDEO_MODE_5M_16_9	},

	{"1024x768",	AMBA_VIDEO_MODE_XGA	},
	{"1152x648",	AMBA_VIDEO_MODE_1152x648	},
	{"1280x720",	AMBA_VIDEO_MODE_720P	},
	{"1280x960",	AMBA_VIDEO_MODE_1280_960},
	{"1280x1024",	AMBA_VIDEO_MODE_SXGA	},
	{"1600x1200",	AMBA_VIDEO_MODE_UXGA	},
	{"1920x1440",	AMBA_VIDEO_MODE_1920_1440},
	{"1920x1080",	AMBA_VIDEO_MODE_1080P	},
	{"2048x1080",	AMBA_VIDEO_MODE_2048_1080},
	{"2048x1152",	AMBA_VIDEO_MODE_2048_1152},
	{"2048x1536",	AMBA_VIDEO_MODE_QXGA	},
	{"2560x1440",	AMBA_VIDEO_MODE_2560x1440	},
	{"2592x1944",	AMBA_VIDEO_MODE_QSXGA	},
	{"2208x1242",	AMBA_VIDEO_MODE_2208x1242	},
	{"2304x1296",	AMBA_VIDEO_MODE_2304x1296	},
	{"2304x1536",	AMBA_VIDEO_MODE_2304x1536	},
	{"3280x2464",	AMBA_VIDEO_MODE_3280_2464	},
	{"3280x1852",	AMBA_VIDEO_MODE_3280_1852	},
	{"2520x1424",	AMBA_VIDEO_MODE_2520_1424	},
	{"1640x1232",	AMBA_VIDEO_MODE_1640_1232	},
	{"4096x2160",	AMBA_VIDEO_MODE_4096x2160	},
	{"4016x3016",	AMBA_VIDEO_MODE_4016x3016	},
	{"4000x3000",	AMBA_VIDEO_MODE_4000x3000	},
	{"3840x2160",	AMBA_VIDEO_MODE_3840x2160	},
	{"640x400",		AMBA_VIDEO_MODE_640_400		},
	{"1296x1032",	AMBA_VIDEO_MODE_1296_1032	},
	{"3264x2448",	AMBA_VIDEO_MODE_3264_2448	},
	{"2304x1836",	AMBA_VIDEO_MODE_2304_1836	},
	{"3264x1836",	AMBA_VIDEO_MODE_3264_1836	},
	{"2208x1836",	AMBA_VIDEO_MODE_2208_1836	},
	{"1280x918",	AMBA_VIDEO_MODE_1280_918	},
	{"816x612",		AMBA_VIDEO_MODE_816_612		},
	{"320x306",		AMBA_VIDEO_MODE_320_306		},
	{"3072x2048",	AMBA_VIDEO_MODE_3072_2048	},
	{"1536x1024",	AMBA_VIDEO_MODE_1536_1024	},
	{"2560x2048",	AMBA_VIDEO_MODE_2560_2048	},
	{"1920x2160",	AMBA_VIDEO_MODE_1920x2160	},
	{"1920x1200",	AMBA_VIDEO_MODE_WUXGA	},
	{"1296x1032",	AMBA_VIDEO_MODE_1296_1032	},
	{"7680x1080",	AMBA_VIDEO_MODE_7680x1080	},
	{"5472x3648",	AMBA_VIDEO_MODE_5472_3648	},
	{"2736x1536",	AMBA_VIDEO_MODE_2736_1536	},
	{"2736x1824",	AMBA_VIDEO_MODE_2736_1824	},
	{"1824x1216",	AMBA_VIDEO_MODE_1824_1216	},
	{"2320x1352",	AMBA_VIDEO_MODE_2320_1352	},
	{"1544x1168",	AMBA_VIDEO_MODE_1544_1168	},
	{"1544x692",	AMBA_VIDEO_MODE_1544_692	},
	{"2320x1748",	AMBA_VIDEO_MODE_2320_1748	},
	{"4644x2332",	AMBA_VIDEO_MODE_4644_2332	},
	{"4644x2612",	AMBA_VIDEO_MODE_4644_2612	},
	{"4644x2160",	AMBA_VIDEO_MODE_4644_2160	},
	{"poweroff",	AMBA_VIDEO_MODE_OFF	},


	{"",		AMBA_VIDEO_MODE_AUTO	},
};




/***************************************************************
	VIN command line options
****************************************************************/
#define	VIN_NUMVERIC_SHORT_OPTIONS				\
	SPECIFY_NO_VIN_CHECK	=	VIN_OPTIONS_BASE,	\
	SPECIFY_ANTI_FLICKER,							\
	SPECIFY_MIRROR_PATTERN,							\
	SPECIFY_BAYER_PATTERN,							\
	SPECIFY_VIN_SOURCE,								\
	GET_VIN_OP_MODE,							\
	SET_VIN_OP_MODE						\

#define	NO_VIN_CHECK_OPTIONS	\
	{"no-vin-check",		NO_ARG,		0,	SPECIFY_NO_VIN_CHECK	},
#define	NO_VIN_CHECK_HINTS	\
	{"",	"\tdo not check all vin format" },

#define	ANTI_FLICKER_OPTIONS	\
	{"anti-flicker",		HAS_ARG,	0,	SPECIFY_ANTI_FLICKER,	},
#define	ANTI_FLICKER_HINTS	\
	{"0~2",	"\tset anti-flicker only for YUV sensor only, 0:no-anti; 1:50Hz; 2:60Hz" },

#define	MIRROR_PATTERN_OPTIONS	\
	{"mirror-pattern",		HAS_ARG,	0,	SPECIFY_MIRROR_PATTERN,	},
#define	MIRROR_PATTERN_HINTS	\
	{"0~3",	"set vin mirror pattern" },

#define	BAYER_PATTERN_OPTIONS	\
	{"bayer-pattern",		HAS_ARG,	0,	SPECIFY_BAYER_PATTERN,	},
#define	BAYER_PATTERN_HINTS		\
	{"0~3",	"set vin bayer pattern" },

#define GET_VIN_OP_MODE_OPTIONS \
	{"get-vin-opmode",		NO_ARG,	0,	GET_VIN_OP_MODE,	},
#define GET_LINEAR_WDR_HINTS \
	{"", "\tcheck vin is linear mode(0) or WDR mode(1)" },

#define SET_VIN_OP_MODE_OPTIONS \
	{"set-vin-opmode",		HAS_ARG,	0,	SET_VIN_OP_MODE,	},
#define SET_LINEAR_WDR_HINTS \
	{"0|1", "set vin to linear mode(0) or WDR mode(1), if it supports" },
#define	VIN_LONG_OPTIONS()		\
	{"vin",			HAS_ARG,	0,	'i' },		\
	{"ch",			HAS_ARG,	0,	'c' },		\
	{"frame-rate",	HAS_ARG,	0,	'f' },		\
	{"vin-src",		HAS_ARG,	0,	SPECIFY_VIN_SOURCE },		\
	NO_VIN_CHECK_OPTIONS		\
	ANTI_FLICKER_OPTIONS		\
	MIRROR_PATTERN_OPTIONS		\
	BAYER_PATTERN_OPTIONS		\
	GET_VIN_OP_MODE_OPTIONS	\
	SET_VIN_OP_MODE_OPTIONS

#define	VIN_PARAMETER_HINTS()		\
	{"vin mode",		"\tchange vin mode" },		\
	{"channel",		"\tselect channel for YUV input" },		\
	{"frate",			"\tset VIN frame rate" },		\
	{"source",		"\tselect source for YUV input" },		\
	NO_VIN_CHECK_HINTS			\
	ANTI_FLICKER_HINTS			\
	MIRROR_PATTERN_HINTS		\
	BAYER_PATTERN_HINTS			\
	GET_LINEAR_WDR_HINTS		\
	SET_LINEAR_WDR_HINTS

#define	VIN_INIT_PARAMETERS()		\
	case 'i':		\
		vin_flag = 1;				\
		vin_mode = get_vin_mode(optarg);	\
		if (vin_mode < 0)			\
			return -1;			\
		break;					\
	case 'c':		\
		channel = atoi(optarg);	\
		break;					\
	case 'f':		\
		framerate_flag = 1;		\
		framerate = atoi(optarg);	\
		switch (framerate) {		\
		case 0:					\
			framerate = AMBA_VIDEO_FPS_AUTO;	\
			break;				\
		case 0x10000:			\
			framerate = AMBA_VIDEO_FPS_29_97;	\
			break;				\
		case 0x10001:			\
			framerate = AMBA_VIDEO_FPS_59_94;	\
			break;				\
		case 0x10003:			\
			framerate = AMBA_VIDEO_FPS_12_5;	\
			break;				\
		case 0x10006:			\
			framerate = AMBA_VIDEO_FPS_7_5;		\
			break;				\
		default:					\
			framerate = DIV_ROUND(512000000, framerate);	\
			break;				\
		}						\
		break;					\
	case SPECIFY_VIN_SOURCE:		\
		source = atoi(optarg);		\
		break;					\
	case SPECIFY_NO_VIN_CHECK:	\
		check_input = 0;			\
		break;					\
	case SPECIFY_ANTI_FLICKER:	\
		anti_flicker = atoi(optarg);	\
		if (anti_flicker < 0 || anti_flicker > 2)	\
			return -1;			\
		break;					\
	case SPECIFY_MIRROR_PATTERN:	\
		mirror_pattern_flag = 1;    \
		mirror_pattern = atoi(optarg);	\
		if (mirror_pattern < 0)	\
			return -1;			\
		mirror_mode.pattern = mirror_pattern;		\
		break;					\
	case SPECIFY_BAYER_PATTERN:		\
		bayer_pattern = atoi(optarg);	\
		if (bayer_pattern < 0)	\
			return -1;			\
		mirror_mode.bayer_pattern = bayer_pattern;	\
		break;					\
	case GET_VIN_OP_MODE:	\
		if (get_vin_linear_wdr_mode() < 0) {	\
			return -1;	\
		}	\
		break;					\
	case SET_VIN_OP_MODE:	\
		sensor_operate_mode = atoi(optarg);	\
		sensor_operate_mode_changed = 1;	\
		break;


/*************************************************************************
	Initialize VIN
*************************************************************************/

#define CHECK_AMBA_VIDEO_FPS_LIST_SIZE		(256)

int select_channel(void)
{
	if (ioctl(fd_iav, IAV_IOC_SELECT_CHANNEL, channel) < 0) {
		perror("IAV_IOC_SELECT_CHANNEL");
		return -1;
	}

	return 0;
}

static inline u32 get_fps_list_count(struct amba_vin_source_mode_info *pinfo)
{
	u32	i;
	for (i = 0; i < pinfo->fps_table_size; i++) {
		if (pinfo->fps_table[i] == AMBA_VIDEO_FPS_AUTO)
			break;
	}
	return i;
}
enum amba_video_mode get_vin_mode(const char *mode)
{
	int i;

	for (i = 0; i < sizeof (__vin_modes) / sizeof (__vin_modes[0]); i++)
		if (strcmp(__vin_modes[i].name, mode) == 0)
			return __vin_modes[i].mode;

	printf("vin mode '%s' not found\n", mode);
	return -1;
}

int set_vin_mirror_pattern(void)
{
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_MIRROR_MODE, &mirror_mode) < 0) {
		perror("IAV_IOC_VIN_SRC_SET_MIRROR_MODE\n");
		return -1;
	} else {
		return 0;
	}
}

int get_vin_linear_wdr_mode(void)
{
	int op_mode;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_OPERATION_MODE, &op_mode) < 0) {
			perror("IAV_IOC_VIN_GET_OPERATION_MODE");
			return -1;
	}
	if(op_mode == AMBA_VIN_LINEAR_MODE)
		printf("sensor mode: Linear mode\n");
	else
		printf("sensor mode: Hdr mode\n");

	return op_mode;
}

int	set_vin_linear_wdr_mode(int  mode)
{
	if (mode < 0) mode = 0;
	if (ioctl(fd_iav, IAV_IOC_VIN_SET_OPERATION_MODE, mode) < 0) {
		perror("IAV_IOC_VIN_SET_OPERATION_MODE\n");
		return -1;
	} else {
		return 0;
	}
}

/*
 * The function is used to set 3A data, only for sensor that output RGB raw data
 */
static int set_vin_param(void)
{
	int vin_eshutter_time = 60;	// 1/60 sec
	int vin_agc_db = 0;	// 0dB

	u32 shutter_time_q9;
	s32 agc_db;

	shutter_time_q9 = 512000000/vin_eshutter_time;
	agc_db = vin_agc_db<<24;

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_SHUTTER_TIME, shutter_time_q9) < 0) {
		perror("IAV_IOC_VIN_SRC_SET_SHUTTER_TIME");
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_AGC_DB, agc_db) < 0) {
		perror("IAV_IOC_VIN_SRC_SET_AGC_DB");
		return -1;
	}

	return 0;
}

static int set_vin_frame_rate(void)
{
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_FRAME_RATE, framerate) < 0) {
		perror("IAV_IOC_VIN_SRC_SET_FRAME_RATE");
		return -1;
	}
	return 0;
}

/* (0 < index < AMBA_VIDEO_MODE_NUM) will change, use AMBA_VIDEO_MODE_XXX when
   you know the format, index2mode is mainly for the app to scan all
   possiable mode in a simple for/while loop.*/
static inline enum amba_video_mode amba_video_mode_index2mode(int index)
{
	enum amba_video_mode			mode = AMBA_VIDEO_MODE_MAX;

	if (index < 0)
		goto amba_video_mode_index2mode_exit;


	if ((index >= 0) && (index < AMBA_VIDEO_MODE_MISC_NUM)) {
		mode = (enum amba_video_mode)index;
		goto amba_video_mode_index2mode_exit;
	}

	if ((index >= AMBA_VIDEO_MODE_MISC_NUM) && (index < (AMBA_VIDEO_MODE_MISC_NUM + AMBA_VIDEO_MODE_STILL_NUM))) {
		mode = (enum amba_video_mode)(AMBA_VIDEO_MODE_3M_4_3 + (index - AMBA_VIDEO_MODE_MISC_NUM));
		goto amba_video_mode_index2mode_exit;
	}

	if ((index >= (AMBA_VIDEO_MODE_MISC_NUM + AMBA_VIDEO_MODE_STILL_NUM)) && (index < AMBA_VIDEO_MODE_NUM)) {
		mode = (enum amba_video_mode)(AMBA_VIDEO_MODE_480I + (index - (AMBA_VIDEO_MODE_MISC_NUM + AMBA_VIDEO_MODE_STILL_NUM)));
		goto amba_video_mode_index2mode_exit;
	}

amba_video_mode_index2mode_exit:
	return mode;
}

static int change_fps_to_hz(u32 fps_q9, u32 *fps_hz, char *fps)
{
	u32				hz = 0;

	switch(fps_q9) {
	case AMBA_VIDEO_FPS_AUTO:
		snprintf(fps, 32, "%s", "AUTO");
		break;
	case AMBA_VIDEO_FPS_29_97:
		snprintf(fps, 32, "%s", "29.97");
		break;
	case AMBA_VIDEO_FPS_59_94:
		snprintf(fps, 32, "%s", "59.94");
		break;
	default:
		hz = DIV_ROUND(512000000, fps_q9);
		snprintf(fps, 32, "%d", hz);
		break;
	}

	*fps_hz = hz;

	return 0;
}

void test_print_video_info( const struct amba_video_info *pvinfo, u32 fps_list_size, u32 *fps_list)
{
	char				format[32];
	char				fps[32];
	char				type[32];
	char				bits[32];
	char				ratio[32];
	char				system[32];
	u32				i;
	u32				fps_hz = 0;

	switch(pvinfo->format) {

	case AMBA_VIDEO_FORMAT_PROGRESSIVE:
		snprintf(format, 32, "%s", "P");
		break;
	case AMBA_VIDEO_FORMAT_INTERLACE:
		snprintf(format, 32, "%s", "I");
		break;
	case AMBA_VIDEO_FORMAT_AUTO:
		snprintf(format, 32, "%s", "Auto");
		break;
	default:
		snprintf(format, 32, "format?%d", pvinfo->format);
		break;
	}

	change_fps_to_hz(pvinfo->fps, &fps_hz, fps);

	switch(pvinfo->type) {
	case AMBA_VIDEO_TYPE_RGB_RAW:
		snprintf(type, 32, "%s", "RGB");
		break;
	case AMBA_VIDEO_TYPE_YUV_601:
		snprintf(type, 32, "%s", "YUV BT601");
		break;
	case AMBA_VIDEO_TYPE_YUV_656:
		snprintf(type, 32, "%s", "YUV BT656");
		break;
	case AMBA_VIDEO_TYPE_YUV_BT1120:
		snprintf(type, 32, "%s", "YUV BT1120");
		break;
	case AMBA_VIDEO_TYPE_RGB_601:
		snprintf(type, 32, "%s", "RGB BT601");
		break;
	case AMBA_VIDEO_TYPE_RGB_656:
		snprintf(type, 32, "%s", "RGB BT656");
		break;
	case AMBA_VIDEO_TYPE_RGB_BT1120:
		snprintf(type, 32, "%s", "RGB BT1120");
		break;
	default:
		snprintf(type, 32, "type?%d", pvinfo->type);
		break;
	}

	switch(pvinfo->bits) {
	case AMBA_VIDEO_BITS_AUTO:
		snprintf(bits, 32, "%s", "Bits Not Availiable");
		break;
	default:
		snprintf(bits, 32, "%dbits", pvinfo->bits);
		break;
	}

	switch(pvinfo->ratio) {
	case AMBA_VIDEO_RATIO_AUTO:
		snprintf(ratio, 32, "%s", "AUTO");
		break;
	case AMBA_VIDEO_RATIO_4_3:
		snprintf(ratio, 32, "%s", "4:3");
		break;
	case AMBA_VIDEO_RATIO_16_9:
		snprintf(ratio, 32, "%s", "16:9");
		break;
	default:
		snprintf(ratio, 32, "ratio?%d", pvinfo->ratio);
		break;
	}

	switch(pvinfo->system) {
	case AMBA_VIDEO_SYSTEM_AUTO:
		snprintf(system, 32, "%s", "AUTO");
		break;
	case AMBA_VIDEO_SYSTEM_NTSC:
		snprintf(system, 32, "%s", "NTSC");
		break;
	case AMBA_VIDEO_SYSTEM_PAL:
		snprintf(system, 32, "%s", "PAL");
		break;
	case AMBA_VIDEO_SYSTEM_SECAM:
		snprintf(system, 32, "%s", "SECAM");
		break;
	case AMBA_VIDEO_SYSTEM_ALL:
		snprintf(system, 32, "%s", "ALL");
		break;
	default:
		snprintf(system, 32, "system?%d", pvinfo->system);
		break;
	}

	printf("\t%dx%d%s\t%s\t%s\t%s\t%s\t%s\trev[%d]\n",
		pvinfo->width,
		pvinfo->height,
		format,
		fps,
		type,
		bits,
		ratio,
		system,
		pvinfo->rev
		);

	for (i = 0; i < fps_list_size; i++) {
		if (fps_list[i] == pvinfo->fps)
			continue;

		change_fps_to_hz(fps_list[i], &fps_hz, fps);
		printf("\t\t\t%s\n", fps);
	}
}

void check_source_info(void)
{
	char					format[32];
	u32					i;
	struct amba_vin_source_info		src_info;
	struct amba_vin_source_mode_info	mode_info;
	u32					fps_list[CHECK_AMBA_VIDEO_FPS_LIST_SIZE];
	u32					fps_list_size;

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &src_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO");
		return;
	}
	printf("\nFind Vin Source %s ", src_info.name);

	if (src_info.dev_type != AMBA_VIN_SRC_DEV_TYPE_DECODER) {
		printf("it supports:\n");

		for (i = 0; i < AMBA_VIDEO_MODE_NUM; i++) {
			mode_info.mode = amba_video_mode_index2mode(i);
			mode_info.fps_table_size = CHECK_AMBA_VIDEO_FPS_LIST_SIZE;
			mode_info.fps_table = fps_list;

			if (ioctl(fd_iav, IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE, &mode_info) < 0) {
				perror("IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE");
				continue;
			}
			if (mode_info.is_supported == 1) {
				fps_list_size = get_fps_list_count(&mode_info);
				test_print_video_info(&mode_info.video_info,
					fps_list_size, fps_list);
			}
		}
		if (source < 0) {
			source = 0;
		}
	} else {
		u32 active_channel_id = src_info.active_channel_id;

		printf("it supports %d channel.\n", src_info.total_channel_num);
		for (i = 0; i < src_info.total_channel_num; i++) {
			if (ioctl(fd_iav, IAV_IOC_SELECT_CHANNEL, i) < 0) {
				perror("IAV_IOC_SELECT_CHANNEL");
				continue;
			}

			if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &src_info) < 0) {
				perror("IAV_IOC_VIN_SRC_GET_INFO");
				continue;
			}

			switch(src_info.source_type.decoder) {
			case AMBA_VIN_DECODER_CHANNEL_TYPE_CVBS:
				snprintf(format, 32, "%s", "CVBS");
				break;
			case AMBA_VIN_DECODER_CHANNEL_TYPE_SVIDEO:
				snprintf(format, 32, "%s", "S-Video");
				break;
			case AMBA_VIN_DECODER_CHANNEL_TYPE_YPBPR:
				snprintf(format, 32, "%s", "YPbPr");
				break;
			case AMBA_VIN_DECODER_CHANNEL_TYPE_HDMI:
				snprintf(format, 32, "%s", "HDMI");
				break;
			case AMBA_VIN_DECODER_CHANNEL_TYPE_VGA:
				snprintf(format, 32, "%s", "VGA");
				break;
			default:
				snprintf(format, 32, "format?%d", src_info.source_type.decoder);
				break;
			}
			printf("Channel[%s] %d's type is %s, ", src_info.name, src_info.active_channel_id, format);

			mode_info.mode = AMBA_VIDEO_MODE_AUTO;
			mode_info.fps_table_size = CHECK_AMBA_VIDEO_FPS_LIST_SIZE;
			mode_info.fps_table = fps_list;
			if (ioctl(fd_iav, IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE, &mode_info) < 0) {
				perror("IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE");
				continue;
			}

			if (mode_info.is_supported == 1) {
				printf("The signal is:\n");
				fps_list_size = get_fps_list_count(&mode_info);
				test_print_video_info(&mode_info.video_info,
					fps_list_size, fps_list);
				if (source == -1) {
					source = src_info.id;
				}
			} else
				printf("No signal yet.\n");
		}

		if (ioctl(fd_iav, IAV_IOC_SELECT_CHANNEL, active_channel_id) < 0) {
			perror("IAV_IOC_SELECT_CHANNEL");
			return;
		}
	}
}

int init_vin(enum amba_video_mode mode)
{
	u32 num;
	int i;
	struct amba_video_info video_info;
	struct amba_vin_source_info src_info;

	num = 0;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_SOURCE_NUM, &num) < 0) {
		perror("IAV_IOC_VIN_GET_SOURCE_NUM");
		return -1;
	}
	if (num < 1) {
		printf("Please load source driver!\n");
		return -1;
	}

	for (i=0; i<num; i++) {
		if (ioctl(fd_iav, IAV_IOC_VIN_SET_CURRENT_SRC, &i) < 0) {
			perror("IAV_IOC_VIN_SET_CURRENT_SRC");
			return -1;
		}

		check_source_info();
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_SET_CURRENT_SRC, &source) < 0) {
		perror("IAV_IOC_VIN_SET_CURRENT_SRC");
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_VIDEO_MODE, mode) < 0) {
		perror("IAV_IOC_VIN_SRC_SET_VIDEO_MODE");
		return -1;
	}

	if (mirror_pattern != AMBA_VIN_SRC_MIRROR_AUTO || bayer_pattern != AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		if (ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_MIRROR_MODE, &mirror_mode) < 0) {
			perror("IAV_IOC_VIN_SRC_SET_MIRROR_MODE");
			return -1;
		}
	}

	if (anti_flicker >= 0) {
		if (ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_ANTI_FLICKER, anti_flicker) < 0) {
			perror("IAV_IOC_VIN_SRC_SET_ANTI_FLICKER");
			return -1;
		}
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &src_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO");
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
		return -1;
	}

	if (src_info.dev_type != AMBA_VIN_SRC_DEV_TYPE_DECODER) {
		if (video_info.type == AMBA_VIDEO_TYPE_RGB_RAW) {
			if (set_vin_param() < 0)	//set aaa here
				return -1;
		}
	}

	if (src_info.dev_type != AMBA_VIN_SRC_DEV_TYPE_DECODER) {
		if (framerate_flag) {
			if (set_vin_frame_rate()< 0)
				return -1;
			if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
				perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
				return -1;
			}
		}
	}

	printf("Active src %d's mode is:\n", source);
	test_print_video_info(&video_info, 0, NULL);

	printf("init_vin done\n");

	return 0;
}

