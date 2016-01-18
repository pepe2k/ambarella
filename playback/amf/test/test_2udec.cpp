
/**
 * test_2udec.cpp
 *
 * History:
 *    2011/8/18 - [GangLiu] created file
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

void ProcessAMFMsg(void *context, AM_MSG& msg)
{
    AM_INFO("AMF msg: %d\n", msg.code);

    if (msg.code == IMediaControl::MSG_PLAYER_EOS)
        AM_INFO("==== Playback end ====\n");
}

void PrintHelp()
{
    AM_INFO("test_2udec stream1 stream2\n");
    return;
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

        if(info.state == IVEControl::STATE_COMPLETED){
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

            if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
                AM_ERROR("stdin_fileno set error");
            return;

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

#define NO_ARG	0
#define HAS_ARG	1

char FileName1[256] = "";
char FileName2[256] = "";
AM_INT dspMode=0;

static struct option long_options[] = {
	{"TranscodeMode", NO_ARG, 0, 't'},
	{"filename", HAS_ARG, 0, 'f'},
	{0, 0, 0, 0}
};

static const char *short_options = "f:t";

int init_param(int argc, char **argv)
{
    int ch;
    int option_index = 0;
    char *file_suffix;
    opterr = 0;

    while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (ch) {
            case 't':
                dspMode = 1;
                AM_INFO("set dsp mode to Transc Mode.\n");
                break;
            case 'f': {
                if(FileName1[0]=='\0'){
                    strcpy(FileName1, optarg);
                    AM_INFO("get 1st file url=%s done.\n", FileName1);
                    break;
                }
                if(FileName2[0]=='\0'){
                    strcpy(FileName2, optarg);
                    AM_INFO("get 2nd file url=%s done.\n", FileName2);
                    break;
                }
                AM_ERROR("Only support 2 files now.\n");
            }
            break;

        default:
            AM_ERROR("unknown option found: %c\n", ch);
            return -1;
        }
    }

    if(FileName2[0]=='\0'){
        AM_ERROR("useage:\n\ttest_2udec -f stream1 -f stream2 -t\n\tin transcode mode with \"-t\", udec mode if not.\n");
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

    err = G_pVEControl->AddSource(FileName1,IVEControl::Type_Video,Source1);
    if( err == ME_OK){
        AM_INFO("PrepareFile url=%s done.\n", FileName1);
    }else{
    AM_ERROR("PrepareFile url=%s failed.\n", FileName1);
    return -1;
    }

    err = G_pVEControl->AddSource(FileName2,IVEControl::Type_Video,Source2);
    if( err == ME_OK){
        AM_INFO("PrepareFile url=%s done.\n", FileName2);
    }else{
        AM_ERROR("PrepareFile url=%s failed.\n", FileName2);
    return -1;
    }

    AM_INFO("StartPreview ...\n");
    err = G_pVEControl->StartPreview(dspMode?0:2);
    if( err == ME_OK){
        AM_INFO("StartPreview done.\n");
    }else{
        AM_INFO("StartPreview failed.\n");
        return -1;
    }

    RunMainLoop();

    G_pVEControl->Stop();
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

