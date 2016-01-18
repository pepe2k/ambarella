
/**
 * test_transcode.cpp
 *
 * History:
 *    2011/9/20 - [GangLiu] created file
 *
 * Copyright (C) 2011-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>

#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_ve_if.h"
#include "am_mw.h"
#include "am_util.h"
#if PLATFORM_ANDROID
#include <binder/MemoryHeapBase.h>
#include <media/AudioTrack.h>
#include "MediaPlayerService.h"
#include "AMPlayer.h"
#endif

extern "C" {
//#include "libavcodec/avcodec.h"
}

IVEControl *G_pVEControl;
CEvent *G_pTimerEvent;
CThread *G_pTimerThread;


struct encode_resolution_s {
    const char *name;
    AM_UINT width;
    AM_UINT height;
}
__encode_res[] = {
    {"1080p", 1920, 1080},
    {"720p", 1280, 720},
    {"480p", 720, 480},
    {"576p", 720, 576},
    {"4sif", 704, 480},
    {"4cif", 704, 576},
    {"xga", 1024, 768},
    {"vga", 640, 480},
    {"cif", 352, 288},
    {"sif", 352, 240},
    {"wqvga", 432, 240},
    {"qvga", 320, 240},
    {"qcif", 176, 144},
    {"qsif", 176, 120},
    {"qqvga", 160, 120},
    {"svga", 800, 600},
    {"sxga", 1280, 1024},

    {"", 0, 0},

    //{"2048x1536", 2048, 1536},
    //{"qxga", 2048, 1536},
    //{"2048x1152", 2048, 1152},
    {"1920x1080", 1920, 1080},
    {"1440x1080", 1440, 1080},
    {"1366x768", 1366, 768},
    {"1280x1024", 1280, 1024},
    {"1280x960", 1280, 960},
    {"1280x720", 1280, 720},
    {"1024x768", 1024, 768},
    {"800x480", 800, 480},
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

char SourceFileName1[256] = "";
char SourceFileName2[256] = "";
char OutputFilenamebase[256] = "transcdemo";

AM_UINT enc_width = 720;
AM_UINT enc_height = 480;
bool keepAspectRatio = false;
AM_UINT bitrate = 2000000;
bool hasBfrm = false;
bool fpsCtrl = true;
AM_INT stopMode = 2;
AM_INT frm_nums = 0;
AM_U8 loop_live = 0;//demo use
bool render1 = true;
bool render2 = false;
bool enable_preview = true;
AM_UINT mSavingStrategy = 0;//whole file

AM_INT save_file_nums = 50;

#define NO_ARG	0
#define HAS_ARG 1

enum numeric_short_options {
	FRM_NUMS,
       FAST,
};

static struct option long_options[] = {
	{"filename", HAS_ARG, 0, 'f'},
	{"stopmode", HAS_ARG, 0, 's'},
	{"outputname", HAS_ARG, 0, 'o'},
	{"render1",	HAS_ARG, 0,	'r'},
	{"render2",	HAS_ARG, 0,	'R'},
	{"preview",	HAS_ARG, 0,	'p'},
	{"h264",		HAS_ARG, 0,	'h'},
	{"keepAR",	NO_ARG, 0,	'k'},
	{"bitrate",	HAS_ARG,	0,	'b'},
	{"cut",	HAS_ARG,	0,	'c'},
	{"hasBfrm",	NO_ARG,	0,	'B'},
	{"loop", NO_ARG, 0, 'l'},
	{"frames", HAS_ARG, 0, FRM_NUMS},
	{"fast", NO_ARG, 0, FAST},
	{0, 0, 0, 0}
};

static const char *short_options = "f:s:o:r:R:p:h:kb:c:Bl";

void ProcessAMFMsg(void *context, AM_MSG& msg)
{
    static unsigned int seq_num[2] = {0};
    unsigned int start_index = 0;
    char full_file_name[256] = "";
    char playlist[256] = "transc_demo.m3u8";

    AM_INFO("AMF msg: %d\n", msg.code);

    if (msg.code == IMediaControl::MSG_PLAYER_EOS)
        AM_INFO("==== Playback end ====\n");

    printf("AMF msg: %d\n", msg.code);
    if (msg.code == IMediaControl::MSG_RECORDER_EOS) {
        printf("==== Record end ====\n");
    } else if (msg.code == IMediaControl::MSG_NEWFILE_GENERATED_POST_TO_APP) {
        return;//deal with this MSG in engine, because java app also need to generate this playlist
        //update file
        printf("==== New file generated, p2 %d, p4 %d, p5 %d, seq_num[0] %d, seq_num[1] %d ====\n", msg.p2, msg.p4, msg.p5, seq_num[0], seq_num[1]);

        //calculate start index, not care wrapper case, todo
        if (msg.p2 > 4) {
            start_index = msg.p2 - 4;
        }
        if(loop_live && (msg.p2 > save_file_nums)){
            sprintf(full_file_name, "%s_%06d.%s", OutputFilenamebase, msg.p2 -save_file_nums-1, AM_GetStringFromContainerType((IParameters::ContainerType)msg.p5));
            remove(full_file_name);
            printf("==== remove file \"%s\" ====\n",full_file_name);
        }

        AM_WriteHLSConfigfile(playlist, OutputFilenamebase, start_index, msg.p2, AM_GetStringFromContainerType((IParameters::ContainerType)msg.p5), seq_num[msg.p4]);
    }
}

void PrintHelp()
{
    printf("test_transcode usage:\n");
    printf("-f --filename [stream url]:\tselect source file, support 2 streams now.\n");
    printf("-h --h264 [resolution]:\t\tset stream (h264) resolution.\n");
    printf("-o --outputname [stream url]:\tset output file name/patch.\n");
    printf("-s --stopmode [1 | 2]:\t\tdebug option, stopmode 1 is not recommended.\n");
    printf("-r --render1 [0 | 1]:\t\trender stream1 to HDMI, 0: disable, 1: enable(default).\n");
    printf("-R --render2 [0 | 1]:\t\trender stream2 to LCD, 0: disable(default), 1: enable.\n");
    printf("-p --preview [0 | 1]:\t\tenable preview on LCD, 0: disable, 1: enable(default), if enabled, ignore '-R'.\n");
    printf("-k --keepAR:\t\t\tkeep Aspect Ratio as input stream.\n");
    printf("-b --bitrate [value]:\t\tset stream (h264) bitrate.\n");
    printf("-c --cut [value]:\t\tset savestrategy: 0 for whole file, or value*30 frms per file.\n");
    printf("-B --hasBfrm:\t\t\tset stream (h264) has B frames.\n");
    printf("-l --loop:\t\t\tloop transcoding, default is false.\n");
    printf("   --frames [num]:\t\tonly transcode num frames(if loop==true, will transcode these frames cyclically).\n");
    printf("   --fast:\t\t\tDo not control fps.\n");

    printf("\nSupport resolutions:\n");
    for(int i=0;i<sizeof(__encode_res)/sizeof(encode_resolution_s);i++){
        if(__encode_res[i].name[0]=='\0') continue;
        printf("\t%s:  %u x %u,\n", ((__encode_res[i])).name, ((__encode_res[i])).width, ((__encode_res[i])).height);
    }
    printf("\n\n");
}

AM_ERR TimerThread(void *context)
{
    while (1) {
        if (G_pTimerEvent->Wait(1000) == ME_OK)
            break;
        //
    }
    return ME_OK;
}

int GetVEInfo(IVEControl::VE_INFO &info)
{
    AM_ERR err;
    if ((err = G_pVEControl->GetVEInfo(info)) != ME_OK) {
        AM_ERROR("GetPBInfo failed\n");
        return -1;
    }
    return 0;
}

void DoStop(AM_INT index)
{
    AM_ERR err;
    IVEControl::VE_INFO info;
    if(index > 3)
        return;
    if (GetVEInfo(info) < 0)
        return;
/*    if(info.InstanceState[index] == IVEControl::INSTANCE_STATE_NOT_EXIST ||
        info.InstanceState[index] == IVEControl::INSTANCE_STATE_IDLE)
        return;
*/
    G_pVEControl->StopDecoder(index);
    return;
}

void DoPause()
{
    AM_ERR err;
    IVEControl::VE_INFO info;

    if (GetVEInfo(info) < 0){}
        return;
}

void DoPrintState()
{
#ifdef AM_DEBUG
    G_pVEControl->PrintState();
#endif
}

void DoSetLogConfig(AM_UINT index, AM_UINT level, AM_UINT option)
{
#ifdef AM_DEBUG
//	G_pVEControl->SetLogConfig(index, level, option);
#endif
}

void RunMainLoop()
{
    char buffer_old[128] = {0};
    char buffer[128];
    IVEControl::VE_INFO info;
    static int flag_stdin = 0;

    flag_stdin = fcntl(STDIN_FILENO,F_GETFL);
    if(fcntl(STDIN_FILENO,F_SETFL,fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1)
        AM_ERROR("stdin_fileno set error");

    while (1) {
        GetVEInfo(info);

        if(info.state == IVEControl::STATE_ERROR){
            AM_ERROR("======Play Error!======\n");
            return;
        }

        if(info.state == IVEControl::STATE_STOPPED){
           if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
                AM_ERROR("stdin_fileno set error");
            AM_INFO("======Play Completed!======\n");
            return;
        }

        //add sleep to avoid affecting the performance
        usleep(10000);

        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
            continue;

        if (buffer[0] == '\n')
            buffer[0] = buffer_old[0];

        switch (buffer[0]) {
        case 'q':    // exit
            printf("Quit\n");

            G_pVEControl->StopTransc(stopMode);
            break;

        case 's':    // stop 1 decoder
            AM_INT DecoderIndex;
            buffer_old[0] = buffer[0];
            if(sizeof(buffer)>2)
                DecoderIndex = buffer[1]-'0';
            else{
                printf("Unknown CMD %s.\n", buffer);
                break;
            }
            printf("DoStop %d.\n", DecoderIndex);
            DoStop(DecoderIndex);
            break;

        case ' ':    // pause
            buffer_old[0] = buffer[0];
            printf("DoPause\n");
            DoPause();
            break;

        case 'p':   //print state, debug only
            printf("DoPrintState.\n");
            DoPrintState();
            break;

        default:
            break;
        }
    }

    if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
        AM_ERROR("stdin_fileno set error");
}

int get_encode_resolution(const char *name, AM_UINT *width, AM_UINT *height)
{
     int i;

     for (i = 0; i < ARRAY_SIZE(__encode_res); i++)
         if (strcmp(__encode_res[i].name, name) == 0) {
             *width = __encode_res[i].width;
             *height = __encode_res[i].height;
             printf("get_encode_resolution: %u, %u.\n", *width, *height);
             return 0;
         }

     printf("resolution '%s' not found.\n", name);
     return -1;
}

int init_param(int argc, char **argv)
{
    int ch, tmp;
    int option_index = 0;
    char *file_suffix;
    opterr = 0;

    while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (ch) {
            case 's':
                stopMode = atoi(optarg);
                AM_INFO("set stopMode %d.\n", stopMode);
                break;

            case 'f': {
                if(SourceFileName1[0]=='\0'){
                    strcpy(SourceFileName1, optarg);
                    AM_INFO("get 1st file url=%s done.\n", SourceFileName1);
                    break;
                }
                if(SourceFileName2[0]=='\0'){
                    strcpy(SourceFileName2, optarg);
                    AM_INFO("get 2nd file url=%s done.\n", SourceFileName2);
                    break;
                }
                AM_ERROR("Only support 2 files now.\n");

            }

            case 'o':{
                strcpy(OutputFilenamebase, optarg);
                AM_INFO("get output file url=%s done.\n", OutputFilenamebase);
                break;
            }

            case FRM_NUMS: {
                frm_nums  = atoi(optarg);
                AM_INFO("set frm_nums %d.\n", frm_nums);
                break;
            }

            case FAST: {
                fpsCtrl  = false;
                AM_INFO("Don not ctrl fps.\n");
                break;
            }

            case 'h':{
                if (get_encode_resolution(optarg, &enc_width, &enc_height) < 0){
                    AM_ERROR("get_encode_resolution error, set resolution to 720X480.\n");
                }
                break;
            }

            case 'k':{
                keepAspectRatio = true;
                break;
            }

            case 'b': {
                bitrate  = atoi(optarg);
                AM_INFO("set bitrate %d.\n", bitrate);
                break;
            }

            case 't': {
                mSavingStrategy  = atoi(optarg);
                AM_INFO("set mSavingStrategy %d.\n", mSavingStrategy);
                break;
            }

            case 'B':{
                hasBfrm = true;
                break;
            }

            case 'l':{
                loop_live = 1;
                break;
            }

            case 'r':{
                tmp = atoi(optarg);
                if(tmp) render1 = true;
                else render1 = false;
                break;
            }

            case 'R':{
                tmp = atoi(optarg);
                if(tmp) render2 = true;
                else render2 = false;
                break;
            }

            case 'p':{
                tmp = atoi(optarg);
                if(tmp) enable_preview = true;
                else enable_preview = false;
                break;
            }

            default:
                AM_ERROR("unknown option found: %c\n", ch);
                PrintHelp();
                return -1;
        }
    }

    if(SourceFileName1[0]=='\0'){
        PrintHelp();
        return -1;
    }
    return 0;
}

extern "C" int main(int argc, char **argv)
{

    AM_ERR err;
    AM_UINT Source1, Source2;
    char configfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};

    if (init_param(argc, argv)) {
        return 1;
    }
    if (ME_OK != AMF_Init()) {
        AM_ERROR("AMF_Init Error.\n");
        return -2;
    }

    //read global cfg from pb.config, optional
    snprintf(configfilename, sizeof(configfilename), "%s/pb.config", AM_GetPath(AM_PathConfig));
    AM_LoadGlobalCfg(configfilename);
    snprintf(configfilename, sizeof(configfilename), "%s/log.config", AM_GetPath(AM_PathConfig));
    AM_LoadLogConfigFile(configfilename);
    AM_OpenGlobalFiles();
    //AM_GlobalFilesSaveBegin(argv[1]);


    if (AMF_Init() != ME_OK) {
        AM_ERROR("AMF_Init() failed\n");
        return -1;
    }

    if ((G_pTimerEvent = CEvent::Create()) == NULL) {
        AM_ERROR("Cannot create event\n");
        return -1;
    }

    G_pVEControl = CreateActiveVEControl(NULL);

    if ((G_pVEControl->SetAppMsgCallback(ProcessAMFMsg, NULL)) != ME_OK) {
        AM_ERROR("SetAppMsgCallback failed\n");
        return -1;
    }
    if ((G_pTimerThread = CThread::Create("timer", TimerThread, NULL)) == NULL) {
        AM_ERROR("Create timer thread failed\n");
        return -1;
    }

    err = G_pVEControl->AddSource(SourceFileName1,IVEControl::Type_Video,Source1);
    if( err == ME_OK){
        AM_INFO("PrepareFile url=%s done.\n", SourceFileName1);
    }else{
    AM_ERROR("PrepareFile url=%s failed.\n", SourceFileName1);
    return -1;
    }
    if(strcmp(SourceFileName2,"\0")){
        err = G_pVEControl->AddSource(SourceFileName2,IVEControl::Type_Video,Source2);
        if( err == ME_OK){
            AM_INFO("PrepareFile url=%s done.\n", SourceFileName2);
        }else{
            AM_ERROR("PrepareFile url=%s failed.\n", SourceFileName2);
        return -1;
        }
    }

    G_pVEControl->SetEncParamaters(enc_width, enc_height, keepAspectRatio, 0, bitrate, hasBfrm);
    G_pVEControl->SetTranscSettings(render1, (enable_preview ? false : render2), enable_preview, fpsCtrl);
    G_pVEControl->SetMuxParamaters(OutputFilenamebase, mSavingStrategy);
    AM_INFO("StartTranscode ...\n");
    err = G_pVEControl->StartTranscode();
    if( err == ME_OK){
        AM_INFO("StartTranscode done.\n");
    }else{
        AM_INFO("StartTranscode failed.\n");
        return -1;
    }

    G_pVEControl->setloop(loop_live);
    G_pVEControl->setTranscFrms(frm_nums);

    RunMainLoop();

    G_pTimerEvent->Signal();
    G_pTimerThread->Delete();
    G_pVEControl->Delete();
    AM_INFO("G_pPBControl->Delete() done.\n");
    G_pTimerEvent->Delete();
    AM_INFO("G_pTimerEvent->Delete() done.\n");

    //todo - cause crash
    AMF_Terminate();
    AM_INFO("AMF_Terminate() done.\n");

    return 0;
}

