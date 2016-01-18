/*
 * playback.cpp
 *
 * History:
 *    2011/4/7 - [Yi Zhu] created file
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
#include <signal.h>
#include <getopt.h>
#include <sched.h>

#include "am_includes.h"
#include "record_if.h"

#include "iav_drv.h"
#include "ambas_common.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include <sys/resource.h>

static IamMediaController *g_pb_pController;
static IamTsDemuxer* g_pb_pTsDemuxer;
static IamMp4Demuxer* g_pb_pMp4Demuxer;
static IamFileExtract *g_pb_pFileExtract;
static IamFileDump* g_pb_pFiledump;
static IamVoutControl *g_pb_pVoutControl;
static IamVideoDecoder *g_pb_pVideoDecorder;
static IamAudioDecoder* g_pb_pAudioDecoder;
static IamAoutControl *g_pb_pAoutControl;
static IamPlaybackPipeline * g_pb_pPipeline;
static int exit_flag = 0;

static IamPlaybackPipeline::DEMUX_INPUT_TYPE input_type = IamPlaybackPipeline::TS;
static IamPlaybackPipeline::DEMUX_OUTPUT_TYPE output_type;
static bool specify_output_type = 0;
static bool specify_vout_mode_flag = 0;
static bool specify_vout_type_flag = 0;
static bool specify_savefile_flag = 0;
static int vout_mode = AMBA_VIDEO_MODE_D1_NTSC;
static int vout_type = AMBA_VOUT_SINK_TYPE_HDMI;
static char dst_file[256];
static char src_file[256]="/mnt/media/test.ts";

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
	SPECIFY_CVBS_VOUT,
	SPECIFY_HDMI_VOUT,
};

static struct option long_options[] = {
	{"input file",	HAS_ARG,	0,	'i'},
	{"input type",	HAS_ARG,	0,	't'},
	{"output type",	HAS_ARG,	0,	'o'},

	{"output file",		HAS_ARG,	0,	'f'},

	{"vout2",		HAS_ARG,	0,	'V'},
	{"cvbs",		NO_ARG,		0,	SPECIFY_CVBS_VOUT},
	{"hdmi",		NO_ARG,		0,	SPECIFY_HDMI_VOUT},
	{0, 0, 0, 0},
};

static const char *short_options = "t:i:V:o:f:";

struct hint_s {
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] = {
	{"","\t\tdefault input file is /mnt/media/test.ts"},
	{"ts|mp4|pcm","default is ts, pcm refer to S16_LE/stereo/48KHz"},

	{"disp|file", "playback or save as local file"},
	{"", "\tdefault output file is /mnt/media/test"},

	{"vout mode", "\tChange vout mode"},
	{"", "\t\tSelect cvbs output"},
	{"", "\t\tSelect hdmi output"},
};

void usage(void)
{
	int i;
	printf("playback usage:\n");
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
	printf("Example:\n");
	printf("  demux mp4 file and playback on TV:\n");
	printf("    playback -t mp4 -i /mnt/media/input.mp4 -o disp \n");
	printf("\n");
	printf("  demux ts file and save the element stream as local file:\n");
	printf("    playback -t ts -i /mnt/media/input.ts -o file -f /mnt/media/output  \n");
	printf("\n");
	printf("  playing an S16_LE format pcm audio file:\n");
	printf("    playback -t pcm -i /mnt/media/input.dat -o disp \n");

}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int mode = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
	{
		switch (ch) {
		case 't':
			if (strcmp(optarg,"ts") == 0)
				input_type = IamPlaybackPipeline::TS;
			else if (strcmp(optarg,"mp4") == 0)
				input_type = IamPlaybackPipeline::MP4;
			else if (strcmp(optarg,"pcm") == 0)
				input_type = IamPlaybackPipeline::PCM;
			else
				return -1;
			break;
		case 'i':
			strcpy(src_file,optarg);
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
		case 'o':
			if (strcmp(optarg,"file") == 0) {
				output_type = IamPlaybackPipeline::LOCAL_FILE;
				specify_output_type = 1;
			}
			else if (strcmp(optarg,"disp") == 0) {
				output_type = IamPlaybackPipeline::PLAY_BACK;
				specify_output_type = 1;
			}
			else
				return -1;
			break;
		case 'f':
			specify_savefile_flag = 1;
			strcpy(dst_file,optarg);
			break;
		default:
			break;
		}
	}
	if ( (specify_vout_mode_flag || specify_vout_type_flag)
		&& specify_savefile_flag)
	{
		printf("not support save playback and save file at the same time\n");
		return 1;
	}
	if (specify_output_type == 0)
	{
		return -1;
	}
	return 0;
}

int InitInterface(void)
{
	if (input_type == IamPlaybackPipeline::TS) {
		if ((g_pb_pTsDemuxer = IamTsDemuxer::GetInterfaceFrom(g_pb_pController)) == NULL)
		{
			printf("No IamTsDemuxer\n");
			return -1;
		}
	}else if (input_type == IamPlaybackPipeline::MP4) {
		if ((g_pb_pMp4Demuxer = IamMp4Demuxer::GetInterfaceFrom(g_pb_pController)) == NULL)
		{
			printf("No IamMp4Demuxer\n");
			return -1;
		}
	}
	if(output_type == IamPlaybackPipeline::PLAY_BACK) {
		if (input_type == IamPlaybackPipeline::TS ||
			input_type == IamPlaybackPipeline::MP4) {
			if ((g_pb_pVoutControl = IamVoutControl::GetInterfaceFrom(g_pb_pController)) == NULL)
			{
				printf("No IamVoutControl\n");
				return -1;
			}

			if ((g_pb_pVideoDecorder = IamVideoDecoder::GetInterfaceFrom(g_pb_pController)) == NULL)
			{
				printf("No IamVideoDecoder\n");
			}
			if ((g_pb_pAudioDecoder = IamAudioDecoder::GetInterfaceFrom(g_pb_pController)) == NULL)
			{
				printf("No IamAacDecoder\n");
			}

			if ((g_pb_pAoutControl = IamAoutControl::GetInterfaceFrom(g_pb_pController)) == NULL)
			{
				printf("No IamAoutControl\n");
				return -1;
			}
		} else if (input_type == IamPlaybackPipeline::PCM) {
			if ((g_pb_pFileExtract = IamFileExtract::GetInterfaceFrom(g_pb_pController)) == NULL)
			{
				printf("No IamFileExtract\n");
				return -1;
			}
			if ((g_pb_pAoutControl = IamAoutControl::GetInterfaceFrom(g_pb_pController)) == NULL)
			{
				printf("No IamAoutControl\n");
				return -1;
			}
		}

	}else if(output_type == IamPlaybackPipeline::LOCAL_FILE) {
		if ((g_pb_pFiledump= IamFileDump::GetInterfaceFrom(g_pb_pController)) == NULL)
		{
			printf("No IamFileDump\n");
			return -1;
		}
	}
	return 0;
}

int StartPrg()
{
	if (specify_vout_mode_flag || specify_vout_type_flag) {
		CamVoutMode vout = {vout_mode, vout_type};
		if (g_pb_pVoutControl->SetVoutMode(vout)){
			return -1;
		}
	}
	if (specify_savefile_flag) {
		if (g_pb_pFiledump->SetDstFile(dst_file)) {
			return -1;
		}
	}
	if (input_type == IamPlaybackPipeline::TS)  {
		g_pb_pTsDemuxer->SetSrcFile(src_file);
	} else if (input_type == IamPlaybackPipeline::MP4)  {
		g_pb_pMp4Demuxer->SetSrcFile(src_file);
	} else if (input_type == IamPlaybackPipeline::PCM) {
		g_pb_pFileExtract->SetSrcFile(src_file);
	}
	if (output_type == IamPlaybackPipeline::PLAY_BACK) {
		if (input_type == IamPlaybackPipeline::TS ||
			input_type == IamPlaybackPipeline::MP4) {
			if(g_pb_pVideoDecorder->ReadytoPlay()){
				return -1;
			}
			if (g_pb_pVideoDecorder->SetTrickMode(0)){
				return -1;
			}
		}
	}

	if (g_pb_pController->RunPipeline())
		return -1;

	return 0;
}

int StopPrg()
{
	if (g_pb_pController->StopPipeline())
		return -1;
	return 0;
}

int Restart()
{
	if (output_type == IamPlaybackPipeline::PLAY_BACK) {
		if (StopPrg())
			return -1;
		if (input_type == IamPlaybackPipeline::TS ||
			input_type == IamPlaybackPipeline::MP4) {
			if(g_pb_pVideoDecorder->ReadytoPlay()){
				return -1;
			}
			if (g_pb_pVideoDecorder->SetTrickMode(0)){
				return -1;
			}
		}
	}

	if (g_pb_pController->RunPipeline()) {
		return -1;
	}
	return 0;
}

int display_decode_h264_menu(void)
{
	printf("\n================ Playing control ================\n");
	if (input_type == IamPlaybackPipeline::TS ||
		input_type == IamPlaybackPipeline::MP4) {
		printf("  p -- Pause decoding\n");
		printf("  r -- Resume to decode\n");
		printf("  f -- Fast play forward\n");
	}
	printf("  q -- Quit decode control\n");
	printf("  R -- Restart decoding\n");
	printf("==============================================\n> ");
	return 0;
}

void RunIdle(void)
{
	char ch = 0;
	int flags = fcntl(STDIN_FILENO, F_GETFL);

	flags |= O_NONBLOCK;
	flags = fcntl(STDIN_FILENO, F_SETFL, flags);
	if (output_type == IamPlaybackPipeline::PLAY_BACK) {
		display_decode_h264_menu();
	}
	else {
		printf("  q -- Quit\n");
		printf("> ");
	}
	while (!exit_flag)
	{
		if (output_type == 0) {
			int i=0;
			if (read(STDIN_FILENO, &ch, 1) < 0) {
				usleep(100000);
				continue;
			}
			if (ch == 10) {//LF
				continue;
			}
			switch (ch) {
				case 'p':
					g_pb_pVideoDecorder->PausePlay();
					g_pb_pAoutControl->PausePlay();
					break;
				case 'f':
					printf("\n  Fast play forward (0 - 1X, 1 - 2X, 2 - 4X, 3 - 0.5X, 4 - 0.25X)\n");
					flags = fcntl(STDIN_FILENO, F_GETFL);
					flags &= ~O_NONBLOCK;
					fcntl(STDIN_FILENO, F_SETFL, flags);
					scanf("%d", &i);
					if (i == 0) {
						g_pb_pVideoDecorder->SetTrickMode(i);
						g_pb_pAoutControl->SkipPlay(false);
					} else {
						g_pb_pVideoDecorder->SetTrickMode(i);
						g_pb_pAoutControl->SkipPlay(true);
					}
					fcntl(STDIN_FILENO, F_GETFL);
					flags |= O_NONBLOCK;
					flags = fcntl(STDIN_FILENO, F_SETFL, flags);
					break;
				case 'r':
					g_pb_pVideoDecorder->ResumePlay();
					g_pb_pAoutControl->ResumePlay();
					break;
				case 'q':
					StopPrg();
					exit_flag = 1;
					break;
				case 'R':
					if (Restart() < 0)
						exit_flag = 1;
					break;
				default:
					break;
			}
			display_decode_h264_menu();
		}else {
			if (read(STDIN_FILENO, &ch, 1) < 0) {
				usleep(100000);
				continue;
			}
			switch (ch) {
				case 'q':
					StopPrg();
					exit_flag = 1;
					break;
				default:
					break;
			}
			printf("  q -- Quit\n");
			printf("> ");
		}
	}
	flags = fcntl(STDIN_FILENO, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(STDIN_FILENO, F_SETFL, flags);
}

static void sigstop(int a)
{
	StopPrg();
	g_pb_pController->Delete();
	exit(1);
}

static void ProcessAppMsg(AM_MSG *pMsg, void *pContext)
{
	//exit_flag = 1;
	if (output_type == 0) {
		if (input_type == IamPlaybackPipeline::PCM) {
			exit_flag = 1;
		}
		printf("Playing back is over\n");
	} else {
		exit_flag = 1;
	}
	return;
}

extern "C" int main(int argc, char **argv)
{
    int err;
    //register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);
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
	if ((g_pb_pController = AM_MediaControllerCreate()) == NULL)
		return -1;

	if ((g_pb_pController ->InstallMsgCallback(ProcessAppMsg, NULL)) != ME_OK)
		return -1;

	// create the tspb pipeline
	if( (err = g_pb_pController->CreatePipeline(CreatePlaybackPipeline)) != ME_OK)
	{
		printf("Playback--CreatePipeline() failed %d.Exit!!!\n",err);
		return -1;
	}
	if ((g_pb_pPipeline= IamPlaybackPipeline::GetInterfaceFrom(g_pb_pController)) == NULL)
	{
		printf("Playback--No IamPlaybackPipeline.Exit!!!\n");
		return -1;
	}
	g_pb_pPipeline->SetInputType(input_type);
	g_pb_pPipeline->SetOutputType(output_type);
	g_pb_pPipeline->BuildPipeline();

	if (InitInterface() < 0)
		return -1;

	if (StartPrg()) {
		return -1;
	}
	RunIdle();
	g_pb_pController->Delete();
	return 0;
}

