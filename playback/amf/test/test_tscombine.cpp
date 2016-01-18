/**
 * test_tscombine.cpp
 *
 * History:
 *    2012/4/13 - [Gang Liu] created file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
 #if PLATFORM_ANDROID
//#define LOG_NDEBUG 0
#define LOG_TAG "test_tscombine"
#include "utils/Log.h"
#endif

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "am_types.h"
#include "am_if.h"
#include "am_mw.h"
#include "osal.h"
#include "am_util.h"

#include "TSFileCombiner.h"

char Files[3][64] = {"", "", ""};
int StartTime[3] = {0, 0, 0};
int EndTime[3] = {0, 0, 0};
int files = 0;
char OutputFile[64]= "";


int ParseOption(int argc, char **argv)
{
    int i=0;
    //AM_ERROR("argc: %d\n", argc);
    if (argc < 5) {
        goto error;
    }

    for(i=1; i<argc;){
        //AM_ERROR("option: %s\n", argv[i]);
        if(!strcmp("-o",argv[i])) {
            if(i+1 > argc) goto error;
            strcpy(OutputFile, argv[i+1]);
            AM_ERROR("set output file: %s\n", OutputFile);
            i+=2;
            continue;
        }
        if(!strcmp("-f",argv[i])) {
            if(i+3 > argc) goto error;
            strcpy(Files[files], argv[i+1]);
            StartTime[files] = atoi(argv[i+2]);
            EndTime[files] = atoi(argv[i+3]);
            AM_ERROR("add input file: %s, time: %ds~%ds.\n", Files[files], StartTime[files], EndTime[files]);
            files++;
            i+=4;
            continue;
        }
        AM_ERROR("Unknown option: %s", argv[i]);
        goto error;
    }

    return 0;

error:
    AM_ERROR("\nusage:\n\ttest_tscombine -f file1 t0 t1 (-f file2 t0 t1 -f file3 t0 t1) -o outfile\n3 input files most.\n");
    return -1;
}

int main(int argc, char **argv)
{
    if (ParseOption(argc, argv)) {
        return -1;
    }

    char configfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};
    //read global cfg from rec.config, optional
    snprintf(configfilename, sizeof(configfilename), "%s/rec.config", AM_GetPath(AM_PathConfig));
    AM_LoadGlobalCfg(configfilename);
    snprintf(configfilename, sizeof(configfilename), "%s/log.config", AM_GetPath(AM_PathConfig));
    AM_LoadLogConfigFile(configfilename);
#if PLATFORM_ANDROID
    android::TSFileCombiner* mpFileCombiner = new android::TSFileCombiner();
#else
    TSFileCombiner* mpFileCombiner = new TSFileCombiner();
#endif
    if(mpFileCombiner==NULL){
        AM_ERROR("mpFileCombiner==NULL");
        return -1;
    }

    if(0 != mpFileCombiner->addFile(Files[0], StartTime[0], EndTime[0])){
        AM_ERROR("add file error %s.", Files[0]);
    }
    if(files>1 && 0 != mpFileCombiner->addFile(Files[1], StartTime[1], EndTime[1])){
        AM_ERROR("add file error %s.", Files[1]);
    }
    if(files>2 && 0 != mpFileCombiner->addFile(Files[2], StartTime[2], EndTime[2])){
        AM_ERROR("add file error %s.", Files[2]);
    }

    if(OutputFile[0]=='\0'){
        strcpy(OutputFile, "/sdcard/combine_output.ts");
    }
    if(0 != mpFileCombiner->doCombineFile(OutputFile, false, false, NULL)){
        AM_ERROR("doCombineFile error!");
    }
    AM_INFO("doCombineFile done.");
    return 0;
}

