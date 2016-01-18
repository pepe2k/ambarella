/*
 * ustream_demo.cpp
 *
 * History:
 *    2011/01/12 - [Yi Zhu] created file
 *	  2011/11/01 - [Hanbo Xiao] modified file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
#include <signal.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include "am_includes.h"
#include "record_if.h"

#include "iav_drv.h"
#include "ambas_common.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "list_queue.h"

#include "broadcaster_api.h"
#include "compat_api.h"
#include "compat_driver.h"

#include "img_struct.h"
#include "img_api_arch.h"

#include "mw_struct.h"
#include "mw_api.h"

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#define VERSION_DATE "ustream-demo.1.3 [2012.03.30-16:30]"

static IamMediaController *g_stream_pController;
static IamVinControl *g_stream_pVinControl;
static IamVoutControl *g_stream_pVoutControl;
static IamVencoderControl *g_stream_pVencControl;
static IamVideoEncoder *g_stream_pVideoEncoder;
static IamAinControl * g_stream_pAinControl;
static IamAudioEncoder *g_stream_pAudioEncoder;
static IamMp4Muxer *g_stream_pMp4muxer;
static IamStreamDump *g_stream_pUploadStream;
static IamTsMuxer* g_stream_pTsMuxer;
static IamStreamPipeline * g_stream_pPipeline;

//global variable
static char filename[256];
static const char * default_filename = "/mnt/test";
static int audioType = 0; //0:no Audio, 1:AAC, 2:BPCM
static int recordType = 0; //0:ES, 1:TS, 2:mp4

bool stop_flag = 0;
static bool exit_flag = 0;
static bool start_flag = 0;
static bool specify_filename = 0;
static bool specify_enc_format_flag[4]={0,0,0,0};
static bool specify_h264_config_flag[4]={0,0,0,0};
static bool specify_vin_mode_flag = 0;
static bool specify_vout_mode_flag = 0;
static bool specify_vout_type_flag = 0;
static bool specify_source_flag = 0;
static bool specify_framerate_flag = 0;
static int encode_stream = 0;
static int current_stream = -1;
static int vout_mode = 0;
static int vout_type = 0;
static int default_vout_mode = AMBA_VIDEO_MODE_D1_NTSC;
static int default_vout_type = AMBA_VOUT_SINK_TYPE_HDMI;
static int source = 0;
static int framerate = 0;
static int vin_mode = 0;
static unsigned int max_file_size = 0;
static unsigned int max_frame_cnt = 0;

static bool low_delay_flag = 0;

static bool change_sample_freq_flag = 0;
static int audio_sample_freq = 48000;
static int audio_channel_num = 2;
static int aac_bitrate = 128000;

static char mac_id[128];
static char mac_secret[128];
static char mac_issue_time[128];
static char save_stream[256];
static int video_width;
static int video_height;

static int mac_id_flag = 0;
static int mac_secret_flag = 0;
static int mac_issue_time_flag = 0;
static int save_stream_flag = 0;

static int bitrate;
static int default_bitrate = 1000000;
static int bitrate_flag = 0;

static int min_bitrate;
static int default_min_bitrate = 128000;
static int min_bitrate_flag = 0;

static int min_fps;
static int default_min_fps = 10;
static int min_fps_flag = 0;

static int log_level;
static int log_level_flag = 0;
static int default_log_level = 5;

static int buffer_size;
static int buffer_size_flag = 0;
static int default_buffer_size = 4 * 1024 * 1024;

static int fps_adjust_control_flag = 0;
static int low_bitrate_control_flag = 0;

static pthread_t consumer_id;

static Encode_Format* encode_format[4] = {
	new Encode_Format(0),
	new Encode_Format(1),
	new Encode_Format(2),
	new Encode_Format(3)
};

static H264_Config* h264_config[4] = {
	new H264_Config(0),
	new H264_Config(1),
	new H264_Config(2),
	new H264_Config(3)
};

AM_UINT g_iav_bitrate = default_bitrate;
AM_UINT g_buffer_size = default_buffer_size;

struct encode_resolution_s {
	const char 	*name;
	int		width;
	int		height;
}
__encode_res[] =
{
	{"1080p", 1920, 1080},
	{"1080i", 1920, 1080},
	{"720p", 1280, 720},
	{"480p", 720, 480},
	{"576p", 720, 576},
	{"480i", 720, 480},
	{"576i", 720, 576},
	{"cif", 352, 288},
	{"sif", 352, 240},

	{"1920x1080", 1920, 1080},
	{"1280x720", 1280, 720},
	{"720x480", 720, 480},
	{"720x576", 720, 576},

	{"352x288", 352, 288},
	{"352x256", 352, 256},	//used for interlaced MJPEG 352x256 encoding ( crop to 352x240 by app)
	{"352x240", 352, 240},
	{"320x240", 320, 240},
};

struct vout_res_param {
    const char 	*name;
    int		mode;
}

__vout_res[] = {
    {"480i",	AMBA_VIDEO_MODE_480I},
    {"576i",	AMBA_VIDEO_MODE_576I},

    {"480p",	AMBA_VIDEO_MODE_D1_NTSC},
    {"576p",	AMBA_VIDEO_MODE_D1_PAL,},

    {"720p",	AMBA_VIDEO_MODE_720P},
    {"720p50",	AMBA_VIDEO_MODE_720P_PAL},

    {"1080i",	AMBA_VIDEO_MODE_1080I},
};

static int get_encode_resolution(const char *name, u16 *width, u16 *height)
{
	for (u32 i = 0; i < sizeof(__encode_res) / sizeof(__encode_res[0]); i++)
		if (strcmp(__encode_res[i].name, name) == 0) {
			*width = __encode_res[i].width;
			*height = __encode_res[i].height;
			return 0;
		}

	printf("resolution %dx%d\n", *width, *height);
	return 0;
}

static int get_vout_mode(const char *name, int *mode)
{
	for (u32 i = 0; i < sizeof(__vout_res) / sizeof(__vout_res[0]); i++)
		if (strcmp(__vout_res[i].name, name) == 0) {
			*mode = __vout_res[i].mode;
			return 0;
		}
	return 0;
}

#define NO_ARG  0
#define HAS_ARG 1
enum numeric_short_options {
	SPECIFY_GOP_IDR = 130,
	SPECIFY_CAVLC,
	SPECIFY_CBR_BITRATE,
	SPECIFY_CVBS_VOUT,
	SPECIFY_HDMI_VOUT,
	SPECIFY_FILE_SIZE,
	SPECIFY_FRAME_CNT,
	SPECIFY_SAMPLE_FREQ,
	SPECIFY_CHANNEL_NUM,
	SPECIFY_AAC_BITRATE,

	SPECIFY_MAC_ID,
	SPECIFY_MAC_SECRET,
	SPECIFY_MAC_ISSUE_TIME,
	SPECIFY_LOG_LEVEL,
	SPECIFY_SAVE_STREAM,
	SPECIFY_MIN_BITRATE,
	SPECIFY_MIN_FPS,
	SPECIFY_BUFFER_SIZE,
	SPECIFY_AU_TYPE,
	SPECIFY_FPS_ADJUST_CONTROL,
	SPECIFY_LOW_BITRATE_CONTROL,
};

static struct option long_options[] = {
	{"type",	HAS_ARG,	0,	't'},
	{"filename",		HAS_ARG,	0,	'f'},
	{"audio",		HAS_ARG,	0,	'a'},
	{"sample-freq",	HAS_ARG,	0,	SPECIFY_SAMPLE_FREQ},
	{"channel",	HAS_ARG,	0,	SPECIFY_CHANNEL_NUM},
	{"aac-bitrate",	HAS_ARG,	0, SPECIFY_AAC_BITRATE},
	{"filesize",	HAS_ARG,	0,	SPECIFY_FILE_SIZE},
	{"framecnt",		HAS_ARG,		0,	SPECIFY_FRAME_CNT},

	{"stream_A",	NO_ARG,		0,	'A' },   // -A xxxxx    means all following configs will be applied to stream A
	{"stream_B",	NO_ARG,		0,	'B' },
	{"stream_C",	NO_ARG,		0,	'C' },
	{"stream_D",	NO_ARG,		0,	'D' },

	{"h264", 		HAS_ARG,	0,	'h'},

	//immediate action, configure encode stream on the fly
	{"encode",	NO_ARG,		0,	'e'},		//start encoding

	//H.264 encode configurations
	{"M",		HAS_ARG,	0,	'M' },
	{"N",		HAS_ARG,	0,	'N'},
	{"idr",		HAS_ARG,	0,	SPECIFY_GOP_IDR},
	{"cavlc",		HAS_ARG,	0,	SPECIFY_CAVLC},
	{"bitrate",	HAS_ARG,	0,	SPECIFY_CBR_BITRATE},

	{"vout2",		HAS_ARG,	0,	'V'},
	{"cvbs",		NO_ARG,		0,	SPECIFY_CVBS_VOUT},
	{"hdmi",		NO_ARG,		0,	SPECIFY_HDMI_VOUT},
	{"vin",		HAS_ARG,	0,	'i'},
	{"src",		HAS_ARG,	0,	'S' },
	{"frame-rate",	HAS_ARG,	0,	'F'},

	{"lowdelay",		NO_ARG,		0,	'l'},

	{  "log-level",     HAS_ARG,  0,   SPECIFY_LOG_LEVEL},
	{    "mac-id",      HAS_ARG,  0,   SPECIFY_MAC_ID},
	{"mac-secret",      HAS_ARG,  0,   SPECIFY_MAC_SECRET},
	{"mac-issue-time",  HAS_ARG,  0,   SPECIFY_MAC_ISSUE_TIME},
	{   "save-stream",  HAS_ARG,  0,   SPECIFY_SAVE_STREAM},
	{   "min-bitrate",  HAS_ARG,  0,   SPECIFY_MIN_BITRATE},
	{   "min-fps",      HAS_ARG,  0,   SPECIFY_MIN_FPS},
	{   "buffer-size",  HAS_ARG,  0,   SPECIFY_BUFFER_SIZE},
	{       "au-type",  HAS_ARG,  0,   SPECIFY_AU_TYPE},
	{"fps-adjust-control" , HAS_ARG, 0, SPECIFY_FPS_ADJUST_CONTROL},
	{"low-bitrate-control", HAS_ARG, 0, SPECIFY_LOW_BITRATE_CONTROL},
	{0, 0, 0, 0},
};

static const char *short_options = "t:f:a:ABCDh:eM:N:i:S:F:V:l";

struct hint_s {
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] = {
	{"es|ts|mp4","\trecord as elemet stream(es), TS(ts), MP4(mp4)"},
	{"","\t\tspecify filename to save video frames coming from IAV to debug"},
	{"aac|bpcm|no", "include audio or not, the default is aac"},
	{"","\tsupport 48000(default), 44100, 32000, 22050, 16000, 12000, 11025, 8000"},
	{"1|2","\t1 for mono, 2 for stero"},
	{"","\taac encoding's bit rate,default is 128000 bps"},
	{"1~2047", "\tmax size(Mbyte) of one mp4 file, default is 2G"},
	{"number", "\tmax frame count in one mp4 file"},

	{"", "\t\tconfig for stream A"},
	{"", "\t\tconfig for stream B"},
	{"", "\t\tconfig for stream C"},
	{"", "\t\tconfig for stream D"},

	{"resolution", "\tenter H.264 encoding resolution"},
	{"", "\t\tstart encoding for current stream"},

	//H.264 encode configurations
	{"1~8", "\t\tH.264 GOP parameter M"},
	{"1~255", "\t\tH.264 GOP parameter N, must be multiple of M"},
	{"1~128", "\thow many GOP's, an IDR picture should happen"},
	{"1|0","\t1:Baseline profile(CAVLC), 0:Main profile(CABAC)"},
	{"value", "\tset cbr average bitrate"},

	{"vout mode", "\tChange vout mode"},
	{"", "\t\tSelect cvbs output"},
	{"", "\t\tSelect hdmi output"},
	{"vin mode", "\tchange vin mode" },
	{"0~3","\t\tselect source:YPbPr(0), HDMI(1), CVBS(2), VGA(3) "},
	{"frate", "\tset VIN frame rate"},

	{"","\t\tlow delay mode"},

	{"", "\t\tset log level"},
	{"", "\t\tset mac id"},
	{"", "\t\tset secret secret"},
	{"", "\tset mac issue time"},
	{"", "\tSpecify filename to save stream to debug"},

	{"", "\tSpecify the minimum bitrate to transifer [default is 128000]"},
	{"", "\t\tSpecify the minmum fps of video [default is 10]"},
	{"", "\tSpecify the size of buffer which is used to store video frames."},
	{"0~3", "Specify whether h264 frames contain AUD and SEI nal unit or not."},
	{"0|1", "Specify whether ustream library can slow down bitrate unlimited."},
	{"0|1", "Specify whether ustream library can adjust fps."},
};

int device_set_audio_bitrate (unsigned int bitrate)
{
	return 0;
}

int device_set_video_bitrate (int video_stream, unsigned int l_bitrate)
{
	if (low_bitrate_control_flag) {
		AM_PRINTF ("\n%s is called, new_bitrate = 0x%x\n", __FUNCTION__, l_bitrate);

		if (l_bitrate < (unsigned int) min_bitrate) {
			AM_PRINTF ("New bitrate should not be smaller that minimum bitrate\n\n");
			return -1;
		} else {
			AM_PRINTF ("\n");
		}
	}

	specify_h264_config_flag[current_stream] = 1;
	h264_config[current_stream]->brc_mode = 0;
	h264_config[current_stream]->cbr_avg_bps = l_bitrate;
	return g_stream_pVencControl->SetH264Config(h264_config[current_stream]);
}

int device_set_video_framerate (int video_stream, int numerator, int denominator)
{
	int iav_fd, quotient;
	iav_change_framerate_factor_ex_t change_framerate;

	if (fps_adjust_control_flag) {
		AM_PRINTF ("\n\n%s is called, numerator = %d, denominator = %d.\n\n",
			__FUNCTION__, numerator, denominator);

		if ((quotient = numerator / denominator) > 30) {
			AM_PRINTF ("Invalid frame interval value: %d / %d should be less than 30.\n",
					numerator, denominator);
			return -1;
		}

		change_framerate.ratio_numerator = (u8)quotient;
		change_framerate.ratio_denominator = 30;

		AM_PRINTF ("\n\n%s is called, numerator = %d, denominator = %d.\n\n",
				__FUNCTION__, change_framerate.ratio_numerator, change_framerate.ratio_denominator);

		change_framerate.id = (1 << current_stream);
		iav_fd = g_stream_pVideoEncoder->GetIAVFd ();
		if (ioctl (iav_fd, IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX, &change_framerate) < 0) {
			AM_PERROR("Error in IAV_IOC_CHANGE_FRAME_RATE , frame_interval2 must be integer multiple " \
				"of frame_interval");
			return -1;
		}
	}

	return -1;
}

void usage(void)
{
	int i;
	printf("ustream_demo usage:\n");
	for (i = 0; i <(int) (sizeof(long_options) / sizeof(long_options[0]) - 1); i++) {
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
}

int init_param(int argc, char **argv)
{
	int ch, au_type;
	int option_index = 0;
	u16 width =0;
	u16 height = 0;
	int mode = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
	{
		switch (ch) {
		case 't':
			if (strcmp(optarg,"es") == 0){
				recordType = 0;
			}else if (strcmp(optarg,"ts") == 0){
				recordType = 1;
			}else if (strcmp(optarg,"mp4") == 0){
				recordType = 2;
			} else{
				return -1;
			}
			break;
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
		case 'h':
			if (get_encode_resolution(optarg, &width, &height) < 0)
				return -1;
			if (current_stream == -1) {
				return -1;
			}
			video_width = width;
			video_height = height;
			specify_enc_format_flag[current_stream] = 1;
			encode_format[current_stream]->type = IAV_ENCODE_H264;
			encode_format[current_stream]->width = width;
			encode_format[current_stream]->height = height;

			break;
		case 'e':
			if (current_stream == -1) {
				return -1;
			}
			encode_stream |= 1<<current_stream;
			break;
		case 'M':
			if (current_stream == -1) {
				return -1;
			}
			specify_h264_config_flag[current_stream] = 1;
			h264_config[current_stream]->M = atoi(optarg);
			break;
		case 'N':
			if (current_stream == -1) {
				return -1;
			}
			specify_h264_config_flag[current_stream] = 1;
			h264_config[current_stream]->N = atoi(optarg);
			break;
		case SPECIFY_GOP_IDR:
			if (current_stream == -1) {
				return -1;
			}
			specify_h264_config_flag[current_stream] = 1;
			h264_config[current_stream]->idr_interval = atoi(optarg);
			break;
		case SPECIFY_CAVLC:
			if (current_stream == -1) {
				return -1;
			}
			h264_config[current_stream]->profile = atoi(optarg);
			if (h264_config[current_stream]->profile != 0 &&
				h264_config[current_stream]->profile != 1) {
				return -1;
			}
			break;
		case SPECIFY_CBR_BITRATE:
			bitrate_flag = 1;
			bitrate = atoi (optarg);
			specify_h264_config_flag[current_stream] = 1;
			h264_config[current_stream]->brc_mode = 0;
			h264_config[current_stream]->cbr_avg_bps = atoi(optarg);
			break;
		case 'V':
			if (get_vout_mode(optarg, &mode) < 0)
				return -1;
			specify_vout_mode_flag = 1;
			vout_mode = mode;
			break;
		case SPECIFY_CVBS_VOUT:
			specify_vout_type_flag = 1;
			vout_type = AMBA_VOUT_SINK_TYPE_CVBS;
			break;
		case SPECIFY_HDMI_VOUT:
			specify_vout_type_flag = 1;
			vout_type = AMBA_VOUT_SINK_TYPE_HDMI;
			break;
		case 'i':
			if (get_vout_mode(optarg, &mode) < 0)
				return -1;
			specify_vin_mode_flag = 1;
			vin_mode = mode;
			break;
		case 'S':
			specify_source_flag = 1;
			source = atoi(optarg);
			break;
		case 'F':
			specify_framerate_flag = 1;
			framerate = atoi(optarg);
			switch (framerate) {
			case 0:
				framerate = AMBA_VIDEO_FPS_AUTO;
				break;
			case 0x10000:
				framerate = AMBA_VIDEO_FPS_29_97;
				break;
			case 0x10001:
				framerate = AMBA_VIDEO_FPS_59_94;
				break;
			case 0x10003:
				framerate = AMBA_VIDEO_FPS_12_5;
				break;
			case 0x10006:
				framerate = AMBA_VIDEO_FPS_7_5;
				break;
			default:
				framerate = DIV_ROUND(512000000, framerate);
				break;
			}
			break;
		case 'f':
			specify_filename = 1;
			strcpy(filename,optarg);
			break;
		case 'a':
			if(strcmp(optarg,"aac") == 0)
				audioType = 1;
			else if(strcmp(optarg, "bpcm") == 0)
				audioType = 2;
			else if(strcmp(optarg,"no") == 0)
				audioType =0; //no audio
			break;
		case SPECIFY_FILE_SIZE:
			max_file_size = atoi(optarg);
			if (max_file_size > 2048 || max_file_size < 1) {
				return -1;
			}
			break;
		case SPECIFY_FRAME_CNT:
			max_frame_cnt = atoi(optarg);
			break;
		case SPECIFY_SAMPLE_FREQ:
			change_sample_freq_flag = 1;
			audio_sample_freq = atoi(optarg);
			break;
		case SPECIFY_CHANNEL_NUM:
			audio_channel_num = atoi(optarg);
			break;
		case SPECIFY_AAC_BITRATE:
			aac_bitrate = atoi(optarg);
			if (aac_bitrate <= 0)
				return -1;
			break;
		case 'l':
			low_delay_flag = true;
			break;

		case SPECIFY_MAC_ID:
			mac_id_flag = 1;
			strcpy (mac_id, optarg);
			break;
		case SPECIFY_MAC_SECRET:
			mac_secret_flag = 1;
			strcpy (mac_secret, optarg);
			break;
		case SPECIFY_MAC_ISSUE_TIME:
			mac_issue_time_flag = 1;
			strcpy (mac_issue_time, optarg);
			break;
		case SPECIFY_SAVE_STREAM:
			save_stream_flag = 1;
			strcpy (save_stream, optarg);
			break;
		case SPECIFY_MIN_BITRATE:
			min_bitrate_flag = 1;
			min_bitrate = atoi (optarg);
			break;
		case SPECIFY_MIN_FPS:
			min_fps_flag = 1;
			min_fps = atoi (optarg);
			break;
		case SPECIFY_LOG_LEVEL:
			log_level_flag = 1;
			log_level = atoi (optarg);
			break;
		case SPECIFY_BUFFER_SIZE:
			buffer_size_flag = 1;
			buffer_size = atoi (optarg);
			break;
		case SPECIFY_AU_TYPE:
			au_type = (unsigned char) atoi (optarg);
			if (au_type > 3) {
				printf ("Value of au_type should not be greater than 3.\n");
				return -1;
			}
			h264_config[current_stream]->au_type = au_type;
			break;
		case SPECIFY_LOW_BITRATE_CONTROL:
			low_bitrate_control_flag = atoi (optarg);
			if (low_bitrate_control_flag < 0 || low_bitrate_control_flag > 1) {
				printf ("Flag for low bitrate control should be 0 or 1.\n");
				return -1;
			}
			break;
		case SPECIFY_FPS_ADJUST_CONTROL:
			fps_adjust_control_flag = atoi (optarg);
			if (fps_adjust_control_flag < 0 || fps_adjust_control_flag > 1) {
				printf ("Flag for fps adjusting control should be 0 or 1.\n");
				return -1;
			}
			break;
		default:
			return -1;
		}
	}

	if ((specify_vin_mode_flag == 0) &&
		(specify_vout_mode_flag == 0) &&
		(specify_vout_type_flag == 0) &&
		(current_stream == -1)){
		specify_vout_mode_flag = 1;
		vout_mode = default_vout_mode;
		specify_vout_type_flag = 1;
		vout_type = default_vout_type;
		specify_vin_mode_flag = 1;
		encode_stream = 0x1;
		specify_enc_format_flag[0] = 1;
		encode_format[0]->type = IAV_ENCODE_H264;
	}

	if (!specify_filename) {
		strcpy (filename,default_filename);
	}

	if (!mac_id_flag) {
		AM_PRINTF ("mac id is not specified.\n");
		return -1;
	}

	if (!mac_secret_flag) {
		AM_PRINTF ("mac secret is not specified.\n");
		return -1;
	}

	if (!mac_issue_time_flag) {
		AM_PRINTF ("mac issue time is not specified.\n");
		return -1;
	}

	if (!bitrate_flag) {
		bitrate = default_bitrate;
	}

	if (!min_bitrate_flag) {
		min_bitrate = default_min_bitrate;
	}

	if (min_bitrate > bitrate) {
		AM_PRINTF("Warning: minimum bitrate is greater than bitrate.\n");
	}

	if (!min_fps_flag) {
		min_fps = default_min_fps;
	}

	if (min_fps > 30) {
		AM_PRINTF ("Warning: minimum fps is greater than 30.\n");
		min_fps = 30;
	}

	if (!log_level) {
		log_level = default_log_level;
	}

	if (bitrate_flag) {
		g_iav_bitrate = bitrate;
	}

	if (buffer_size_flag) {
		if (buffer_size < 1000000) {
			AM_PRINTF ("Buffer's size should be equal to or greater than 1MB");
			return -1;
		} else {
			g_buffer_size = buffer_size;
		}
	}

	return 0;
}

int ConfigAudio()
{
	if(audioType == 0) {
		return 0;
	} else if ((audioType == 1) || (audioType == 2)) {
		if ((g_stream_pAinControl = IamAinControl::GetInterfaceFrom(g_stream_pController)) == NULL)
		{
			printf("No IamAinControl\n");
			return -1;
		}

		if ((g_stream_pAudioEncoder = IamAudioEncoder::GetInterfaceFrom(g_stream_pController)) == NULL)
		{
			printf("No IamAudioEncoder\n");
			return -1;
		}

		IamAinControl::AUDIO_ATTR audioAttr;
		audioAttr.pcmFormat = IamAinControl::PCM;
		audioAttr.samplesPerSec = audio_sample_freq;
		audioAttr.nchannels = audio_channel_num;
		if (g_stream_pAinControl->SetAudioAttr(&audioAttr) != ME_OK)
			return -1;

		if (audioType == 1) { //AAC
			IamAudioEncoder::AAC_ATTR aacAttr;
			aacAttr.sample_freq = audio_sample_freq;
			aacAttr.src_numCh = audio_channel_num;
			aacAttr.out_numCh = audio_channel_num;
			aacAttr.bit_rate =  aac_bitrate;
			if (g_stream_pAudioEncoder->SetAacAttr(&aacAttr) != ME_OK) {
				printf("Audio paramters not supported\n");
				return -1;
			}
		}
	}
	return 0;
}

int ConfigPrg()
{
	if (recordType == 2) {
		if ((g_stream_pMp4muxer = IamMp4Muxer::GetInterfaceFrom(g_stream_pController)) == NULL)
		{
			printf("No IamMp4Muxer\n");
			return -1;
		}
		CMP4MUX_RECORD_INFO record_info;
		record_info.dest_name = filename;
		if (max_file_size || max_frame_cnt) {
			record_info.max_filesize = max_file_size << 20;
			record_info.max_videocnt = max_frame_cnt;
		}else {
			record_info.max_filesize = 0;
			record_info.max_videocnt = 0;
		}
		if (g_stream_pMp4muxer->Config(&record_info)) {
				return -1;
		}
	}else if (recordType == 1){
		if ((g_stream_pTsMuxer = IamTsMuxer::GetInterfaceFrom(g_stream_pController)) == NULL)
		{
			printf("No IamTsMuxer\n");
			return -1;
		}
		CTSMUX_CONFIG ts_config;
		ts_config.pid_info = NULL;
		ts_config.dest_name = filename;
		if (max_file_size || max_frame_cnt) {
			ts_config.max_filesize = max_file_size << 20;
			ts_config.max_videocnt = max_frame_cnt;
		}else {
			ts_config.max_filesize = 0;
			ts_config.max_videocnt = 0;
		}

		if (g_stream_pTsMuxer->Config(&ts_config)) {
			return -1;
		}
	} else if (recordType == 0) {
		if ((g_stream_pUploadStream = IamStreamDump::GetInterfaceFrom(g_stream_pController)) == NULL)
		{
			printf("No IamUploadStream");
			return -1;
		}

		if (specify_filename) {
			if(g_stream_pUploadStream->SetDstFile(filename)){
				return -1;
			}
		} else {
			if (g_stream_pUploadStream->SetDstFile (NULL)) {
				return -1;
			}
		}
	}
	return 0;
};

int StartPrg()
{
	if (specify_source_flag) {
		if (g_stream_pVinControl->SetVinSource(source)) {
			return -1;
		}
	}

	if (specify_vin_mode_flag || specify_vout_mode_flag ||
		specify_vout_type_flag ||
		specify_enc_format_flag[0] ||
		specify_enc_format_flag[1] ||
		specify_enc_format_flag[2] ||
		specify_enc_format_flag[3] ||
		h264_config[0]->M)
	{
		if (g_stream_pVencControl->GotoIdle()) {
			return -1;
		}
		if (g_stream_pVencControl->SetSourceBufferLimit(low_delay_flag)) {
			return -1;
		}
	}

	if (specify_vin_mode_flag) {
		if (g_stream_pVinControl->InitVin(vin_mode)) {
			return -1;
		}
		if ((encode_format[0]->width == 0) &&
			(encode_format[0]->height == 0 )) {
			if (g_stream_pVinControl->GetVinSize(
				&(encode_format[0]->width),
				&(encode_format[0]->height)) ) {
				return -1;
			}
		}
	}

	if (specify_framerate_flag) {
		if (g_stream_pVinControl->SetVinFramerate(framerate)) {
			return -1;
		}
	}

	if (!low_delay_flag && (specify_vout_mode_flag || specify_vout_type_flag)) {
		CamVoutMode vout = {vout_mode, vout_type};
		if (g_stream_pVoutControl->SetVoutMode(vout)){
			return -1;
		}
		if (g_stream_pVencControl->SetPreviewBuffer(g_stream_pVoutControl)){
			return -1;
		}
	}

	for (int i=0;i<4;i++) {
		if (specify_enc_format_flag[i]) {
			if (g_stream_pVencControl->SetEncodeFormat(encode_format[i], low_delay_flag)){
				return -1;
			}
		}
		if (specify_h264_config_flag[i]) {
			if (g_stream_pVencControl->SetH264Config(h264_config[i])) {
				return -1;
			}
		}
	}

	if (g_stream_pVencControl->EnablePreview()) {
			return -1;
	}

	if (change_sample_freq_flag)
	{
		if (g_stream_pVencControl->SetAudioClock(audio_sample_freq) != ME_OK)
			return -1;
	}

	if (encode_stream) {
		if (g_stream_pVencControl->StartEncode(encode_stream)){
			return -1;
		}
	}

	if (g_stream_pController->RunPipeline()) {
		return -1;
	}

	return 0;
}

void StopPrg()
{
	mw_stop_aaa ();
	g_stream_pController->StopPipeline();
}


int InitInterface(void)
{
	if ((g_stream_pVinControl = IamVinControl::GetInterfaceFrom(g_stream_pController)) == NULL)
	{
		printf("No IamVinControl\n");
		return -1;
	}

	if ((g_stream_pVoutControl = IamVoutControl::GetInterfaceFrom(g_stream_pController)) == NULL)
	{
		printf("No IamVinControl\n");
		return -1;
	}

	if ((g_stream_pVencControl = IamVencoderControl::GetInterfaceFrom(g_stream_pController)) == NULL)
	{
		printf("No IamVencodeControl\n");
		return -1;
	}

	if ((g_stream_pVideoEncoder = IamVideoEncoder::GetInterfaceFrom(g_stream_pController)) == NULL)
	{
		printf("No IamVideoEncoder\n");
		return -1;
	}

	return 0;
}

int recordRunIdle(void)
{
	//printf("\n>>wait for input:\n");
	//printf("	s--stop and exit.\n");
	//printf("\n>>");
	//char ch = 0;
	int flags = fcntl(STDIN_FILENO, F_GETFL);

	flags |= O_NONBLOCK;
	flags = fcntl(STDIN_FILENO, F_SETFL, flags);
	while (!exit_flag);
	/* {
		 if (read(STDIN_FILENO, &ch, 1) < 0) {
			usleep(100000);
			continue;
		}

		if (ch == 's') {
			StopPrg();
			break;
		}
	} */
	StopPrg ();
	flags = fcntl(STDIN_FILENO, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(STDIN_FILENO, F_SETFL, flags);
	stop_flag = 1;

	return 0;
}

static void sigrecordstop(int a)
{
	StopPrg();
	stop_flag = 1;
	g_stream_pController->Delete();

	int flags = fcntl(STDIN_FILENO, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(STDIN_FILENO, F_SETFL, flags);

	// pthread_join (consumer_id, NULL);
	exit(1);
}

static void ProcessAppMsg(AM_MSG *pMsg, void *pContext)
{
	exit_flag = 1;
	stop_flag = 1;
}

static void on_error (UstreamBroadcasterError error)
{
	printf ("[ustream] Callback - error: %d\n", error);
	switch (error) {
	case UstreamBroadcasterAuthenticationError:
		AM_PRINTF ("[ustream] Error occurs for authentication.\n");
		break;
	case UstreamBroadcasterInvalidChannelError:
		AM_PRINTF ("[ustream] Error occurs for channel.\n");
		break;
	case UstreamBroadcasterInvalidMACAgeError:
		AM_PRINTF ("[ustream] Error occurs for mac age.\n");
		break;
	case UstreamBroadcasterRecordingError:
		AM_PRINTF ("[ustream] Error occurs for recording.\n");
		break;
	case UstreamBroadcasterRecordLimitExceeded:
		AM_PRINTF ("[ustream] Error occurs for exceeding record limit.");
		break;
	case UstreamBroadcasterConnectionError:
		AM_PRINTF ("[ustream] Connection error.\n");
		break;
	case UstreamBroadcasterCannotSaveVideoDetails:
		AM_PRINTF ("[ustream] Can not save video details.\n");
		break;
	case UstreamBroadcasterCannotDropVideo:
		AM_PRINTF ("[ustream] Can not drop video.\n");
		break;
	case UstreamBroadcasterLowBandwidthError:
		AM_PRINTF ("[ustream] Low bandwidth error.\n");
		break;
	case UstreamBroadcasterVideoWasTooShort:
		AM_PRINTF ("[ustream] Video was too short.\n");
		break;
	}

	exit_flag = 1;
}

static void onConnectionStatus (UstreamConnectionStatus state)
{
	AM_PRINTF ("[ustream] Callback - connection status: %d\n", state);
}

static void onRecordTimerWarning (int remainingSecs)
{
	AM_PRINTF ("[ustream] Callback - record timer warning, remaining: %d seconds\n",
		remainingSecs);
}

static void onSessionState (UstreamSessionStatus state)
{
	AM_PRINTF ("[ustream] Callback - session state: %d\n", state);
}

static void onCaptureState (UstreamCaptureStatus state)
{
	AM_PRINTF ("[ustream] Callback - capture state: %d\n", state);
}

static void viewer_count_listener (int viewerCount)
{
	AM_PRINTF ("[ustream] Callback - viewer_count_listener, count: %d\n", viewerCount);
}

static void all_viewer_count_listener (int allViewerCount)
{
	AM_PRINTF ("[ustream] Callback - all_viewer_count_listener: count: %d\n", allViewerCount);
}

static int broadcast ()
{
	char device_id[20];
	char platform[20];
	char device_name[20];
	long l_mac_issue_time = 0;

	if (!sscanf (mac_issue_time, "%ld", &l_mac_issue_time)) {
		AM_PRINTF ("[ustream] Error on parsing mac_issue_time.\n");
		return -1;
	}

	sprintf (device_id, "%d", 12345);
	sprintf (platform, "%s", "Linux");
	sprintf (device_name, "%s", "Demo Device");

	if (ustream_init (platform, device_name, device_id)) {
		AM_PRINTF ("[ustream] Error on initializing Ustream library");
		return -1;
	}

	// ustream_set_recording_mode (0);
	ustream_set_config (UstreamRecordMode, 0);
	ustream_set_error_callback (on_error);
	ustream_set_connection_status_callback (onConnectionStatus);
	ustream_set_record_timer_warning_callback (onRecordTimerWarning);
	ustream_set_session_state_callback (onSessionState);
	ustream_set_capture_state_callback (onCaptureState);
	ustream_set_viewer_count_listener(viewer_count_listener);
	ustream_set_all_viewer_count_listener(all_viewer_count_listener);

	//ustream_set_auth_oauth2_no_http ((unsigned char *)ustream_channel,
	//	(unsigned char *)mac_id, (unsigned char *)mac_secret, l_mac_issue_time);
	ustream_set_auth_oauth2 ((unsigned char *)mac_id,
		(unsigned char *)mac_secret, l_mac_issue_time);

	/* getting channel list
	if (!ustream_get_channels (NULL)) {
		AM_PRINTF ("[ustream] Error occurs on getting channels\n");
		return -1;
	}

	// user has at least one channel
	if (ustream_select_channel_idx (0)) {
		AM_PRINTF ("[ustream] Error occurs on selecting channel\n");
		return -1;
	} */
	ustream_set_channel (0);

	if (save_stream_flag) {
		ustream_set_config(UstreamLocalRecordMode, 1);
		// ustream_save_stream (NULL, save_stream);
		ustream_set_output_filename(NULL, save_stream);
	}

	// ustream_set_video (0, video_width, video_height, 30, 1, bitrate, 30, 1, min_bitrate);
	// ustream_set_audio (48000, 2, 16, 128000);

	/* Set up ustream driver. */
	ustream_set_device_driver (&ustream_driver);

	/* Configure audio. */
	ustream_set_config(UstreamAudioSampleRate, 48000);
	ustream_set_config(UstreamAudioSampleSize, 16);
	ustream_set_config(UstreamAudioChannels, 2);
	ustream_set_config(UstreamAudioBitrate, 128000);

	/* Configure Qos. */
	ustream_set_config(UstreamQoSAlgorithm, UstreamQoSAdjustByLag);
	ustream_set_config(UstreamQoSVideoMaxBitrate, bitrate);
	ustream_set_config(UstreamQoSVideoMinBitrate, min_bitrate);
	ustream_set_config(UstreamQoSVideoFramerateMaxNum, 30);
	ustream_set_config(UstreamQoSVideoFramerateMaxDenom, 1);
	ustream_set_config(UstreamQoSVideoFramerateMinNum, min_fps);
	ustream_set_config(UstreamQoSVideoFramerateMinDenom, 1);

	/* Configure video. */
	ustream_set_config(UstreamVideoWidth, video_width);
	ustream_set_config(UstreamVideoHeight, video_height);
	ustream_set_config(UstreamVideoFramerateNum, 30);
	ustream_set_config(UstreamVideoFramerateDenom, 1);
	ustream_set_config(UstreamVideoBitrate, bitrate);

	ustream_start ();
	ustream_start_session ();

	start_flag = 1;

	while (!stop_flag) {};

	ustream_stop_session ();
	ustream_stop (2000);
	ustream_free ();

	return 0;
}

static void *consumer_entry (void *arg)
{
	if (broadcast () < 0)
		AM_PRINTF ("[ustream_demo]: Failed to call broadcast.\n");

	/* unsigned long timestamp;
	unsigned char *frame;
	int frame_len, frame_num;
	DeviceAVCNalUnitType frame_type;

	start_flag = 1;
	while (!stop_flag)
		device_get_video_frame (0, &timestamp,
			&frame, &frame_len, &frame_num, &frame_type);

	AM_PRINTF ("The thread to call device_get_video_frame is over.\n"); */
	return NULL;
}

extern "C" int main(int argc, char **argv)
{
    int err;

	printf ("Latest binary's Version: %s\n", VERSION_DATE);
    //register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigrecordstop);
	signal(SIGQUIT,	sigrecordstop);
	signal(SIGTERM,	sigrecordstop);

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	// create the media controller
	if ((g_stream_pController = AM_MediaControllerCreate()) == NULL)
		return -1;

	if ((err =g_stream_pController ->InstallMsgCallback(ProcessAppMsg, NULL)) != ME_OK)
		return -1;

	// create the stream pipeline
	if((err = g_stream_pController->CreatePipeline(CreateStreamPipeline)) != ME_OK)
	{
		printf("ustream_demo--CreatePipeline() failed %d.Exit!!!\n",err);
		return -1;
	}

	if ((g_stream_pPipeline= IamStreamPipeline::GetInterfaceFrom(g_stream_pController)) == NULL)
	{
		printf("ustream_demo--No IamStreamPipeline.Exit!!!\n");
		return -1;
	}

	IamStreamPipeline::FILE_TYPE file_type;
	IamStreamPipeline::AUDIO_TYPE audio_type;
	switch(recordType){
		case 0:
			file_type = IamStreamPipeline::ES;
			break;
		case 1:
			file_type = IamStreamPipeline::TS;
			break;
		case 2:
			file_type = IamStreamPipeline::MP4;
			break;
		default:
			return -1;
	}
	switch(audioType){
		case 0:
			audio_type = IamStreamPipeline::NONE;
			break;
		case 1:
			audio_type = IamStreamPipeline::AAC;
			break;
		case 2:
			audio_type = IamStreamPipeline::BPCM;
			break;
		default:
			return -1;
	}
	g_stream_pPipeline->SetRecordType(file_type, audio_type);
	if (g_stream_pPipeline->BuildPipeline())
		return -1;

	if (InitInterface() < 0)
		return -1;
	if (ConfigAudio() < 0)
		return -1;
	if (ConfigPrg() < 0)
		return -1;

	c_log_set_level (log_level);
	if ((pthread_create (&consumer_id,
		NULL, &consumer_entry, NULL)) < 0) {
		AM_PRINTF ("Failed to create consumer thread to call device_get_video_frame.\n");
		return -1;
	}

	while (!start_flag);
	// sleep (1);
	if (StartPrg()) {
		return -1;
	}

	mw_start_aaa (g_stream_pVideoEncoder->GetIAVFd ());
	recordRunIdle();
	g_stream_pController->Delete();

	if (exit_flag) {
		return -1;
	} else {
		return 0;
	}
}
