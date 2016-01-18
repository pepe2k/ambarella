/*
 * file_mux.cpp
 *
 * History:
 *    2011/6/15 - [Yi Zhu] created file
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

static IamMediaController *g_mux_pController;
static IamMp4Muxer *g_mux_pMp4muxer;
static IamFileExtract *g_mux_pFileExtract;
static IamMuxPipeline * g_mux_pPipeline;

//global variable
static char video_filename[256];
static char audio_filename[256];
static char output_filename[256];
static const char * default_filename = "/mnt/media/test";

static bool exit_flag = 0;
static bool specify_videofile = 0;
static bool specify_audiofile = 0;
static bool specify_outputfile = 0;

#define NO_ARG  0
#define HAS_ARG 1

static struct option long_options[] = {
	{"h264",	HAS_ARG,	0,	'h'},
	//{"aac",	HAS_ARG,	0,	'a'},
	//{"output-format",	HAS_ARG,	0,	'o'},
	{"output-file",		HAS_ARG,	0,	'f'},
	{0, 0, 0, 0},
};

static const char *short_options = "h:a:o:f:";

struct hint_s {
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] = {
	{"filename","\tinput video file"},
	//{"filename","\tinput audio file"},
	//{"mp4|ts", "\toutput's format"},
	{"filename", "\toutput file"},
};

void usage(void)
{
	int i;
	printf("usage:\n");
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
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
	{
		switch (ch) {
		case 'h':
			specify_videofile = 1;
			strcpy(video_filename,optarg);
			break;
		case 'a':
			specify_audiofile = 1;
			strcpy(audio_filename,optarg);
			break;
		case 'f':
			specify_outputfile = 1;
			strcpy(output_filename,optarg);
			break;
		default:
			break;
		}
	}
	if (!specify_videofile || !specify_videofile)
		return -1;
	if (!specify_outputfile) {
		strcpy(output_filename,default_filename);
	}
	return 0;
}

int StartPrg()
{
	g_mux_pFileExtract->SetSrcFile(video_filename);
	CMP4MUX_RECORD_INFO record_info;
	record_info.dest_name = output_filename;
	record_info.max_filesize = 0;
	record_info.max_videocnt = 0;

	if (g_mux_pMp4muxer->Config(&record_info)) {
		return -1;
	}

	if (g_mux_pController->RunPipeline())
		return -1;
	return 0;
}

void StopPrg()
{
	g_mux_pController->StopPipeline();
}

int InitInterface(void)
{
	if ((g_mux_pMp4muxer = IamMp4Muxer::GetInterfaceFrom(g_mux_pController)) == NULL) {
		printf("No IamMp4Muxer\n");
		return -1;
	}
	if ((g_mux_pFileExtract = IamFileExtract::GetInterfaceFrom(g_mux_pController)) == NULL) {
		printf("No IamFileExtract");
		return -1;
	}
	return 0;
}

int muxRunIdle(void)
{
	while (!exit_flag)
	{
		usleep(100000);
	}
	return 0;
}

static void sigrecordstop(int a)
{
	StopPrg();
	g_mux_pController->Delete();
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
	if ((g_mux_pController = AM_MediaControllerCreate()) == NULL)
		return -1;

	if ((err =g_mux_pController ->InstallMsgCallback(ProcessAppMsg, NULL)) != ME_OK)
		return -1;

	// create the record pipeline
	if((err = g_mux_pController->CreatePipeline(CreateMuxPipeline)) != ME_OK)
	{
		printf("mux--CreatePipeline() failed %d.Exit!!!\n",err);
		return -1;
	}

	if ((g_mux_pPipeline= IamMuxPipeline::GetInterfaceFrom(g_mux_pController)) == NULL)
	{
		printf("mux--No IamrecordPipeline.Exit!!!\n");
		return -1;
	}

	if (g_mux_pPipeline->BuildPipeline())
		return -1;

	if (InitInterface() < 0)
		return -1;

	if (StartPrg()) {
		return -1;
	}
	muxRunIdle();
	g_mux_pController->Delete();
	if (exit_flag) {
		return -1;
	} else {
		return 0;
	}
}

