/*
 * record.cpp
 *
 * History:
 *    2011/1/12 - [Yi Zhu] created file
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

#include "am_includes.h"
#include "record_if.h"

#include "iav_drv.h"
#include "ambas_common.h"
#include "ambas_vin.h"
#include "ambas_vout.h"

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )


static IamMediaController *g_record_pController;
static IamVinControl *g_record_pVinControl;
static IamVoutControl *g_record_pVoutControl;
static IamVencoderControl *g_record_pVencControl;
static IamVideoEncoder *g_record_pVideoEncoder;
static IamAinControl * g_record_pAinControl;
static IamAudioEncoder *g_record_pAudioEncoder;
static IamMp4Muxer *g_record_pMp4muxer;
static IamFileDump *g_record_pFileDump;
static IamTsMuxer* g_record_pTsMuxer;
static IamRecordPipeline * g_record_pPipeline;

//global variable
static char filename[256];
static const char * default_filename = "/mnt/media/test";
static int audioType = 1; //0:no Audio, 1:AAC, 2:BPCM
static int recordType =2; //0:ES, 1:TS, 2:mp4

static bool exit_flag = 0;
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
};

static struct option long_options[] = {
	{"type",	HAS_ARG,	0,	't'},
	{"filename",		HAS_ARG,	0,	'f'},
	{"Audio",		HAS_ARG,	0,	'a'},
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
	{0, 0, 0, 0},
};

static const char *short_options = "t:f:a:ABCDh:eM:N:i:S:F:V:l";

struct hint_s {
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] = {
	{"es|ts|mp4","\trecord as elemet stream(es), TS(ts), MP4(mp4)"},
	{"","\t\tspecify filename"},
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
};

void usage(void)
{
	int i;
	printf("record usage:\n");
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
	int ch;
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
			current_stream =0;
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
		strcpy(filename,default_filename);
	}
	return 0;
}

int ConfigAudio()
{
	if(audioType == 0) {
		return 0;
	} else if ((audioType == 1) || (audioType == 2)) {
		if ((g_record_pAinControl = IamAinControl::GetInterfaceFrom(g_record_pController)) == NULL)
		{
			printf("No IamAinControl\n");
			return -1;
		}

		if ((g_record_pAudioEncoder = IamAudioEncoder::GetInterfaceFrom(g_record_pController)) == NULL)
		{
			printf("No IamAudioEncoder\n");
			return -1;
		}

		IamAinControl::AUDIO_ATTR audioAttr;
		audioAttr.pcmFormat = IamAinControl::PCM;
		audioAttr.samplesPerSec = audio_sample_freq;
		audioAttr.nchannels = audio_channel_num;
		if (g_record_pAinControl->SetAudioAttr(&audioAttr) != ME_OK)
			return -1;

		if (audioType == 1) { //AAC
			IamAudioEncoder::AAC_ATTR aacAttr;
			aacAttr.sample_freq = audio_sample_freq;
			aacAttr.src_numCh = audio_channel_num;
			aacAttr.out_numCh = audio_channel_num;
			aacAttr.bit_rate =  aac_bitrate;
			if (g_record_pAudioEncoder->SetAacAttr(&aacAttr) != ME_OK) {
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
		if ((g_record_pMp4muxer = IamMp4Muxer::GetInterfaceFrom(g_record_pController)) == NULL)
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
		if (g_record_pMp4muxer->Config(&record_info)) {
				return -1;
		}
	}else if (recordType == 1){
		if ((g_record_pTsMuxer = IamTsMuxer::GetInterfaceFrom(g_record_pController)) == NULL)
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
		if (g_record_pTsMuxer->Config(&ts_config)) {
			return -1;
		}
	} else if (recordType == 0) {
		if ((g_record_pFileDump = IamFileDump::GetInterfaceFrom(g_record_pController)) == NULL)
		{
			printf("No IamFileDump");
			return -1;
		}
		if(g_record_pFileDump->SetDstFile(filename)){
			return -1;
		}
	}
	return 0;
};

int StartPrg()
{
	if (specify_source_flag) {
		if (g_record_pVinControl->SetVinSource(source)) {
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
		if (g_record_pVencControl->GotoIdle()) {
			return -1;
		}
		if (g_record_pVencControl->SetSourceBufferLimit(low_delay_flag)) {
			return -1;
		}
	}

	if (specify_vin_mode_flag) {
		if (g_record_pVinControl->InitVin(vin_mode)) {
			return -1;
		}
		if ((encode_format[0]->width == 0) &&
			(encode_format[0]->height == 0 )) {
			if (g_record_pVinControl->GetVinSize(
				&(encode_format[0]->width),
				&(encode_format[0]->height)) ) {
				return -1;
			}
		}
	}

	if (specify_framerate_flag) {
		if (g_record_pVinControl->SetVinFramerate(framerate)) {
			return -1;
		}
	}

	if (!low_delay_flag && (specify_vout_mode_flag || specify_vout_type_flag)) {
		CamVoutMode vout = {vout_mode, vout_type};
		if (g_record_pVoutControl->SetVoutMode(vout)){
			return -1;
		}
		if (g_record_pVencControl->SetPreviewBuffer(g_record_pVoutControl)){
			return -1;
		}
	}

	for (int i=0;i<4;i++) {
		if (specify_enc_format_flag[i]) {
			if (g_record_pVencControl->SetEncodeFormat(encode_format[i], low_delay_flag)){
				return -1;
			}
		}
		if (specify_h264_config_flag[i]) {
			if (g_record_pVencControl->SetH264Config(h264_config[i])) {
				return -1;
			}
		}
	}

	if (g_record_pVencControl->EnablePreview()) {
			return -1;
	}

	if (change_sample_freq_flag)
	{
		if (g_record_pVencControl->SetAudioClock(audio_sample_freq) != ME_OK)
			return -1;
	}

	if (encode_stream) {
		if (g_record_pVencControl->StartEncode(encode_stream)){
			return -1;
		}
	}
	if (g_record_pController->RunPipeline()) {
		return -1;
	}
	return 0;
}

void StopPrg()
{
	g_record_pController->StopPipeline();
}


int InitInterface(void)
{
	if ((g_record_pVinControl = IamVinControl::GetInterfaceFrom(g_record_pController)) == NULL)
	{
		printf("No IamVinControl\n");
		return -1;
	}

	if ((g_record_pVoutControl = IamVoutControl::GetInterfaceFrom(g_record_pController)) == NULL)
	{
		printf("No IamVinControl\n");
		return -1;
	}

	if ((g_record_pVencControl = IamVencoderControl::GetInterfaceFrom(g_record_pController)) == NULL)
	{
		printf("No IamVencodeControl\n");
		return -1;
	}

	if ((g_record_pVideoEncoder = IamVideoEncoder::GetInterfaceFrom(g_record_pController)) == NULL)
	{
		printf("No IamVideoEncoder\n");
		return -1;
	}

	return 0;
}

int recordRunIdle(void)
{
	printf("\n>>wait for input:\n");
	printf("	s--stop and exit.\n");
	printf("\n>>");
	char ch = 0;
	int flags = fcntl(STDIN_FILENO, F_GETFL);

	flags |= O_NONBLOCK;
	flags = fcntl(STDIN_FILENO, F_SETFL, flags);
	while (!exit_flag)
	{
		if (read(STDIN_FILENO, &ch, 1) < 0) {
			usleep(100000);
			continue;
		}

		if (ch == 's') {
			StopPrg();
			break;
		}
	}
	flags = fcntl(STDIN_FILENO, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(STDIN_FILENO, F_SETFL, flags);

	return 0;
}

static void sigrecordstop(int a)
{
	StopPrg();
	g_record_pController->Delete();

	int flags = fcntl(STDIN_FILENO, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(STDIN_FILENO, F_SETFL, flags);
	exit(1);
}

static void ProcessAppMsg(AM_MSG *pMsg, void *pContext)
{
	exit_flag = 1;
}


extern "C" int main(int argc, char **argv)
{
    int err;

    //register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigrecordstop);
	signal(SIGQUIT,	sigrecordstop);
	signal(SIGTERM,	sigrecordstop);
	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
	{
		usage();
		return -1;
	}

	// create the media controller
	if ((g_record_pController = AM_MediaControllerCreate()) == NULL)
		return -1;

	if ((err =g_record_pController ->InstallMsgCallback(ProcessAppMsg, NULL)) != ME_OK)
		return -1;

	// create the record pipeline
	if((err = g_record_pController->CreatePipeline(CreateRecordPipeline)) != ME_OK)
	{
		printf("record--CreatePipeline() failed %d.Exit!!!\n",err);
		return -1;
	}

	if ((g_record_pPipeline= IamRecordPipeline::GetInterfaceFrom(g_record_pController)) == NULL)
	{
		printf("record--No IamrecordPipeline.Exit!!!\n");
		return -1;
	}

	IamRecordPipeline::FILE_TYPE file_type;
	IamRecordPipeline::AUDIO_TYPE audio_type;
	switch(recordType){
		case 0:
			file_type = IamRecordPipeline::ES;
			break;
		case 1:
			file_type = IamRecordPipeline::TS;
			break;
		case 2:
			file_type = IamRecordPipeline::MP4;
			break;
		default:
			return -1;
	}
	switch(audioType){
		case 0:
			audio_type = IamRecordPipeline::NONE;
			break;
		case 1:
			audio_type = IamRecordPipeline::AAC;
			break;
		case 2:
			audio_type = IamRecordPipeline::BPCM;
			break;
		default:
			return -1;
	}
	g_record_pPipeline->SetRecordType(file_type, audio_type);
	if (g_record_pPipeline->BuildPipeline())
		return -1;

	if (InitInterface() < 0)
		return -1;
	if (ConfigAudio() < 0)
		return -1;
	if (ConfigPrg() < 0)
		return -1;

	if (StartPrg()) {
		return -1;
	}
	recordRunIdle();
	g_record_pController->Delete();
	if (exit_flag) {
		return -1;
	} else {
		return 0;
	}
}

