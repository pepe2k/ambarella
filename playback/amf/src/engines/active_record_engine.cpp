
/**
 * active_record_engine.cpp
 *
 * History:
 *    2011/07/11 - [Jay Zhang] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "active_record_engine"
//#define AMDROID_DEBUG

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <basetypes.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_queue.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_record_if.h"
#include "am_base.h"
#include "am_util.h"
#include "record_if.h"

#include "streamming_if.h"

#include "active_record_engine.h"
#include "filter_list.h"
#include "filter_registry.h"

//#define USE_RTSP_MUXER

//test alloc/free small memory in android
//#define __test_alloc_small_memory__

IRecordControl2* CreateRecordControl2(int streaming_only /*= 0*/)
{
    return (IRecordControl2*)CRecordEngine2::Create(streaming_only);
}

#ifdef __test_alloc_small_memory__
void test_malloc()
{
    AM_UINT check_alignment_minus1 = 3;
    AM_U8* malloc_pointer[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    AM_UINT ran_seed[8] = {3, 7, 11, 19, 23, 29, 31, 37};
    AM_UINT size_minus1[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    AM_UINT size_range[8] = {4, 8, 16, 32, 64, 128, 256, 11};
    AM_UINT index = 0, tot_cnt = 10000;
    AM_UINT i = 0;
    AM_WARNING("test_malloc start.\n");
    for (index = 0; index < tot_cnt; index ++) {
        //malloc
        for (i = 0; i < 8; i ++) {
            malloc_pointer[i] = (AM_U8*) malloc(size_minus1[i] + 1);
            AM_ASSERT(!(check_alignment_minus1 & ((AM_UINT)malloc_pointer[i])));
        }

        //free
        for (i = 0; i < 8; i ++) {
            free(malloc_pointer[i]);
            malloc_pointer[i] = NULL;
        }

        //new
        for (i = 0; i < 8; i ++) {
            malloc_pointer[i] = new AM_U8[size_minus1[i] + 1];
            AM_ASSERT(!(check_alignment_minus1 & ((AM_UINT)malloc_pointer[i])));
        }

        //delete
        for (i = 0; i < 8; i ++) {
            delete [](malloc_pointer[i]);
            malloc_pointer[i] = NULL;
        }

        //update malloc size
        for (i = 0; i < 8; i ++) {
            size_minus1[i] += ran_seed[i];
            if (size_minus1[i] >= size_range[i]) {
                size_minus1[i] -= size_range[i];
            }
        }
    }
    AM_WARNING("test_malloc end.\n");
}
#endif

//-----------------------------------------------------------------------
//
// CRecordEngine2
//
//-----------------------------------------------------------------------
CRecordEngine2* CRecordEngine2::Create(int streaming_only)
{
    CRecordEngine2 *result = new CRecordEngine2(streaming_only);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CRecordEngine2::CRecordEngine2(int streaming_only /*= 0*/):
    inherited("record_engine2"),
    mpVideoEncoderFilter(NULL),
    mpAudioInputFilter(NULL),
    mpAudioEncoderFilter(NULL),
    mpPridataComposerFilter(NULL),
    mpVideoEncoder(NULL),
    mpAudioInput(NULL),
    mpAudioEncoder(NULL),
    mpPridataComposer(NULL),
    mpClockManager(NULL),
    mTotalMuxerNumber(0),
    mState(STATE_NOT_INITED),
    mbRun(true),
    mbNeedCreateGragh(true),
    mbStopCmdSent(false),

    mpMutex(NULL),
    mpCondStopfilters(NULL),
//platform related code
    mTotalVideoStreamNumber(2),
    //mMaxFileNumber(DefaultMaxFileNumber),
    //mPridataDuration(DefaultPridataDuration),

    //mSavingFileStrategy(IParameters::MuxerSavingFileStrategy_ToTalFile),
    //mSavingCondition(IParameters::MuxerSavingCondition_InputPTS),
    //mAutoSavingParam(DefaultAutoSavingParam),
    mpPrivateDataConfigList(NULL),
    mbAllVideoDisabled(false),
    mbAllAudioDisabled(false),
    mbAllPridataDisabled(true),
    mbAllStreamingDisabled(true),
    //streamming related
    mpStreammingServerManager(NULL),
    mnStreammingServerNumber(0),
    mnStreammingContentNumber(0),
    mServerMagicNumber(0),
    mbOnlyEncAudio(false),
    mbOnlyEncVideo(false)
{
    AM_UINT i;
    char configfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};
    memset((void*)&mShared, 0, sizeof(mShared));

    for (i=0; i<DMaxMuxerNumber; i++) {
        mpMuxerFilters[i] = NULL;
        mpMuxerControl[i] = NULL;
        mpStreamingDataTransmiter[i] = NULL;
        mbEOSComes[i] = 0;
        mFilename[i] = NULL;

        memset(&mMuxerConfig[i], 0, sizeof(SMuxerConfig));

        //some default value
        mMuxerConfig[i].mSavingStrategy = IParameters::MuxerSavingFileStrategy_ToTalFile;//no auto cut
        mMuxerConfig[i].mSavingCondition = IParameters::MuxerSavingCondition_InputPTS;
        mMuxerConfig[i].mConditionParam = DefaultAutoSavingParam;
        mMuxerConfig[i].mAutoNaming = IParameters::MuxerAutoFileNaming_ByNumber;//by number

        mMuxerConfig[i].mTotalFileNumber = DefaultTotalFileNumber;
    }

    //read setting from rec.config
    AM_DefaultRecConfig(&mShared);
    snprintf(configfilename, sizeof(configfilename), "%s/rec.config", AM_GetPath(AM_PathConfig));
    AM_LoadRecConfigFile(configfilename, &mShared);
    AM_PrintRecConfig(&mShared);

    AM_DefaultPBConfig(&mShared);
    AM_DefaultPBDspConfig(&mShared);

    //read setting from pb.config
    snprintf(configfilename, sizeof(configfilename), "%s/pb.config", AM_GetPath(AM_PathConfig));
    i = AM_LoadPBConfigFile(configfilename, &mShared);

    //read fail, for safe, set default value again
    if (i==0) {
        AM_DefaultPBConfig(&mShared);
        AM_DefaultPBDspConfig(&mShared);
    }

    if(streaming_only){
        mShared.disable_save_files = 1;
    }
    mRequestVoutMask = mShared.pbConfig.vout_config;
    mpOpaque = (void*)&mShared;

    //thumbnail related
    mpThumbNailFilename = NULL;
    mpThumbNailRawBuffer = NULL;
    mThumbNailRawBufferSize = 0;
    mThumbNailWidth = mShared.encoding_mode_config.thumbnail_width;
    mThumbNailHeight = mShared.encoding_mode_config.thumbnail_height;
}

AM_ERR CRecordEngine2::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    DSetModuleLogConfig(LogModuleRecordEngine);

    if ((mpMutex = CMutex::Create(false)) == NULL)
        return ME_OS_ERROR;

    if ((mpCondStopfilters = CCondition::Create()) == NULL)
        return ME_OS_ERROR;

    if ((mpClockManager = CClockManager::Create()) == NULL)
        return ME_ERROR;

    AM_ASSERT(mpWorkQ);
    mpWorkQ->Run();

    return ME_OK;
}

CRecordEngine2::~CRecordEngine2()
{
    AM_UINT i = 0;

    AMLOG_DESTRUCTOR("CRecordEngine2::~CRecordEngine2 start.\n");

    for (i=0; i<DMaxMuxerNumber; i++) {
        if (mFilename[i]) {
            free(mFilename[i]);
            mFilename[i] = NULL;
        }
    }

    if(mpThumbNailFilename){
        free((void*)mpThumbNailFilename);
        mpThumbNailFilename = NULL;
    }

    AMLOG_DESTRUCTOR("CRecordEngine2::~CRecordEngine2 before delete mpClockManager %p.\n", mpClockManager);

    if (mpClockManager) {
        mpClockManager->StopClock();
        mpClockManager->SetSource(NULL);
        AM_DELETE(mpClockManager);
        mpClockManager = NULL;
    }

    AMLOG_DESTRUCTOR("CRecordEngine2::Delete, before delete mpPrivateDataConfigList.\n");

    //set private durations
    SPrivateDataConfig* p_config = mpPrivateDataConfigList;
    SPrivateDataConfig* p_tmp;
    while (p_config) {
        p_tmp = p_config;
        p_config = p_config->p_next;
        free(p_tmp);
    }

    if (mpStreammingServerManager) {
        mpStreammingServerManager->StopServerManager();
        AM_DELETE(mpStreammingServerManager);
        mpStreammingServerManager = NULL;
    }

    if (mpThumbNailRawBuffer) {
        free(mpThumbNailRawBuffer);
        mpThumbNailRawBuffer = NULL;
    }

    AMLOG_DESTRUCTOR("CRecordEngine2::~CRecordEngine2 exit.\n");
}

void CRecordEngine2::Delete()
{
    AM_UINT i = 0;

    AMLOG_DESTRUCTOR("CRecordEngine2::Delete start.\n");

    for (i=0; i<DMaxMuxerNumber; i++) {
        if (mFilename[i]) {
            free(mFilename[i]);
            mFilename[i] = NULL;
        }
    }

    AMLOG_DESTRUCTOR("CRecordEngine2::Delete, before mpWorkQ->SendCmd(CMD_STOP);.\n");

    if (mpWorkQ) {
        AMLOG_INFO("before send stop.\n");
        mpWorkQ->SendCmd(CMD_STOP);
        AMLOG_INFO("after send stop.\n");
    }

    AMLOG_DESTRUCTOR("CRecordEngine2::Delete, before delete mpClockManager %p.\n", mpClockManager);

    if (mpClockManager) {
        mpClockManager->StopClock();
        mpClockManager->SetSource(NULL);
        AM_DELETE(mpClockManager);
        mpClockManager = NULL;
    }

    AMLOG_DESTRUCTOR("CRecordEngine2::Delete, before delete mpPrivateDataConfigList.\n");

    //set private durations
    SPrivateDataConfig* p_config = mpPrivateDataConfigList;
    SPrivateDataConfig* p_tmp;
    while (p_config) {
        p_tmp = p_config;
        p_config = p_config->p_next;
        free(p_tmp);
    }

    if (mpStreammingServerManager) {
        mpStreammingServerManager->StopServerManager();
        AM_DELETE(mpStreammingServerManager);
        mpStreammingServerManager = NULL;
    }

    AMLOG_DESTRUCTOR("CRecordEngine2::Delete, before inherited::Delete().\n");

    inherited::Delete();

    AMLOG_DESTRUCTOR("CRecordEngine2::Delete, exit.\n");

    if (mpThumbNailRawBuffer) {
        free(mpThumbNailRawBuffer);
        mpThumbNailRawBuffer = NULL;
    }

}

void *CRecordEngine2::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IRecordControl2)
        return (IRecordControl2*)this;
    if (refiid == IID_IStreamingControl)
        return (IStreamingControl*)this;
    return inherited::GetInterface(refiid);
}

void *CRecordEngine2::QueryEngineInterface(AM_REFIID refiid)
{
    if (refiid == IID_IClockManager)
        return mpClockManager;
    if (refiid == IID_IClockManagerExt)
        return mpClockManager;
    return NULL;
}

AM_UINT CRecordEngine2::allMuxerEOS()
{
    AM_UINT i;
    AMLOG_PRINTF("** allMuxerEOS start.\n");
    for (i=0; i<mTotalMuxerNumber; i++) {
        if (mbEOSComes[i] == 0) {
            AMLOG_PRINTF("** allMuxerEOS muxer %d not eos flag.\n", i);
            return 0;
        }
    }
    AMLOG_PRINTF("** allMuxerEOS verified.\n");
    return 1;
}

void CRecordEngine2::clearAllEOS()
{
    AM_UINT i;
    for (i=0; i<mTotalMuxerNumber; i++) {
        mbEOSComes[i] = 0;
    }
}

void CRecordEngine2::setMuxerEOS(IFilter* pfilter)
{
    AM_UINT i;
    for (i=0; i<mTotalMuxerNumber; i++) {
        if (pfilter == mpMuxerFilters[i]) {
            mbEOSComes[i] = 1;
            break;
        }
    }
}

AM_ERR CRecordEngine2::findNextStreamIndex(AM_UINT muxer_index, AM_UINT& index, IParameters::StreamType type)
{
    AMLOG_DEBUG("CRecordEngine2::findNextStreamIndex start, muxer_index %d, index %d, type %d.\n", muxer_index, index, type);
    //safe check
    AM_ASSERT(muxer_index < mTotalMuxerNumber);
    if (muxer_index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::findNextStreamIndex, muxer_index %d, mTotalMuxerNumber %d, type %d.\n", muxer_index, mTotalMuxerNumber, type);
        return ME_BAD_PARAM;
    }

    for (;index < mMuxerConfig[muxer_index].stream_number; index ++) {
        if (mMuxerConfig[muxer_index].stream_info[index].stream_type == type) {
            AMLOG_DEBUG("CRecordEngine2::findNextStreamIndex done, return index %d.\n", index);
            return ME_OK;
        }
    }
    AM_ERROR("CRecordEngine2::findNextStreamIndex not found, mMuxerConfig[muxer_index].stream_number %d.\n", mMuxerConfig[muxer_index].stream_number);
    return ME_ERROR;
}

AM_ERR CRecordEngine2::StartRecord()
{

#ifdef __test_alloc_small_memory__
    test_malloc();
    return ME_OK;
#endif

    //reset
    mbStopCmdSent = false;

    AM_ERR err;
    AUTO_LOCK(mpMutex);

    SetState(STATE_STOPPED);

    if (mState != STATE_STOPPED) {
        AM_ERROR("Bad state %d\n", mState);
        return ME_BAD_STATE;
    }

    if (mbNeedCreateGragh) {
#if PLATFORM_ANDROID_ITRON
if (mShared.use_itron_filter && mShared.video_from_itron && mShared.audio_from_itron) {
    err = CreateGraph_ITron();
} else {
    if (g_GlobalCfg.mUseFFMpegMuxer2 == 0) {
        err = CreateGraph();
    } else {
        err = CreateGraph_muxer2();
    }
}
#else
if (g_GlobalCfg.mUseFFMpegMuxer2 == 0) {
        err = CreateGraph();
} else {
        err = CreateGraph_muxer2();
}
#endif
        if (err != ME_OK) {
            AM_ERROR("CreateGraph failed, now clearing...\n");
            ClearGraph();
            return err;
        }
        mbNeedCreateGragh = false;
    }
    if (mpClockManager) {
        mpClockManager->SetClockMgr(0);//hard code here, dsp's pts start from 0
        mpClockManager->StartClock();
    }
    err = RunAllFilters();
    if (err != ME_OK) {
        StopAllFilters();
        PurgeAllFilters();
        return err;
    }

    SetState(STATE_RECORDING);

    return ME_OK;
}

//need mutex protection
void CRecordEngine2::sendStopRecord()
{
    if (true == mbStopCmdSent) {
        return;
    }

    mbStopCmdSent = true;
    if (mpClockManager) {
        mpClockManager->StopClock();
    }

    if (!mbAllVideoDisabled && !mbOnlyEncAudio) {
        AMLOG_INFO("before mpVideoEncoderFilter->FlowControl(IFilter::FlowControl_eos).\n");
        mpVideoEncoderFilter->FlowControl(IFilter::FlowControl_eos);
        AMLOG_INFO("mpVideoEncoderFilter->FlowControl(IFilter::FlowControl_eos) done.\n");
    }
    if (!mbAllAudioDisabled && !mbOnlyEncVideo) {
        //AM_ASSERT(mpAudioInputFilter);
        if (mpAudioInputFilter) {
            AMLOG_INFO("before mpAudioInputFilter->FlowControl(IFilter::FlowControl_eos).\n");
            mpAudioInputFilter->FlowControl(IFilter::FlowControl_eos);
            AMLOG_INFO("mpAudioInputFilter->FlowControl(IFilter::FlowControl_eos) done.\n");
        }
    }

    if (!mbAllPridataDisabled) {
        AM_ASSERT(mpPridataComposerFilter);
        if (mpPridataComposerFilter) {
            AMLOG_INFO("before mpPridataComposerFilter->FlowControl(IFilter::FlowControl_eos).\n");
            mpPridataComposerFilter->FlowControl(IFilter::FlowControl_eos);
            AMLOG_INFO("mpPridataComposerFilter->FlowControl(IFilter::FlowControl_eos) done.\n");
        }
    }
}

AM_ERR CRecordEngine2::StopRecord()
{

#ifdef __test_alloc_small_memory__
    return ME_OK;
#endif

    //bool need_wait = true;
    AUTO_LOCK(mpMutex);
    AMLOG_INFO("CRecordEngine2::StopRecord start, mState %d, STATE_STOPPED %d.\n", mState, STATE_STOPPED);
    if (mState == STATE_RECORDING || mState == STATE_STOPPING) {

        SetState(STATE_STOPPING);
        if (false == mbStopCmdSent) {
            sendStopRecord();
        }

        AMLOG_INFO("CRecordEngine2 Wait Cond...\n ");
        while (mState != STATE_STOPPED) {
            mpCondStopfilters->Wait(mpMutex);
        }
        AMLOG_INFO("CRecordEngine2 Wait Cond done\n ");
        AM_ASSERT(mState == STATE_STOPPED);
    }

    return ME_OK;
}

void CRecordEngine2::AbortRecord()
{
    AUTO_LOCK(mpMutex);
    if (mState != STATE_STOPPED) {
        StopAllFilters();
        NewSession();
        SetState(STATE_STOPPED);
    }
}

AM_ERR CRecordEngine2::PauseRecord()
{
    AM_ASSERT(mpCmdQueue);
    CMD cmd;
    AMLOG_INFO("CRecordEngine2::PauseRecord.\n");
    cmd.code = RECCMD_PAUSE_RESUME;
    cmd.repeatType = CMD_TYPE_REPEAT_CNT;
    cmd.flag = 0;
    mpCmdQueue->PostMsg(&cmd, sizeof(cmd));
    return ME_OK;
}
AM_ERR CRecordEngine2::ResumeRecord()
{
    AM_ASSERT(mpCmdQueue);
    CMD cmd;
    AMLOG_INFO("CRecordEngine2::ResumeRecord.\n");
    cmd.code = RECCMD_PAUSE_RESUME;
    cmd.repeatType = CMD_TYPE_REPEAT_CNT;
    cmd.flag = 1;
    mpCmdQueue->PostMsg(&cmd, sizeof(cmd));
    return ME_OK;
}

AM_ERR CRecordEngine2::CloseAudioHAL()
{
    AM_ASSERT(mpCmdQueue);
    CMD cmd;
    AMLOG_INFO("CRecordEngine2::CLOSE_OPEN_AUDIO.\n");
    cmd.code = RECCMD_CLOSE_OPEN_AUDIO_HAL;
    cmd.repeatType = CMD_TYPE_REPEAT_CNT;
    cmd.flag = 0;
    mpCmdQueue->PostMsg(&cmd, sizeof(cmd));
    return ME_OK;
}

AM_ERR CRecordEngine2::ReopenAudioHAL()
{
    AM_ASSERT(mpCmdQueue);
    CMD cmd;
    AMLOG_INFO("CRecordEngine2::CLOSE_OPEN_AUDIO.\n");
    cmd.code = RECCMD_CLOSE_OPEN_AUDIO_HAL;
    cmd.repeatType = CMD_TYPE_REPEAT_CNT;
    cmd.flag = 1;
    mpCmdQueue->PostMsg(&cmd, sizeof(cmd));
    return ME_OK;
}

bool CRecordEngine2::ProcessGenericMsg(AM_MSG& msg)
{
    AM_UINT i;
    AM_ERR err;
    AM_MSG msg0;
    AUTO_LOCK(mpMutex);
    if (!IsSessionMsg(msg)) {
        AMLOG_PRINTF("!!!!not session msg, code %d, msg.session id %d, session %d.\n", msg.code, msg.sessionID, mSessionID);
        return true;
    }

    switch (msg.code) {
        case MSG_EOS:
            AMLOG_PRINTF("EOS msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
            AM_ASSERT(mState == STATE_STOPPING);
            if (mState == STATE_STOPPING) {
                AMLOG_PRINTF("=== Engine received MSG_EOS.\n");
                setMuxerEOS((IFilter *)msg.p1);
                if (allMuxerEOS() == 1) {
                    SetState(STATE_STOPPED);
                    AMLOG_PRINTF("** allMuxerEOS, before mpCondStopfilters->Signal();.\n");
                    mpCondStopfilters->Signal();
                    AMLOG_PRINTF("** before PostAppMsg(IMediaControl::MSG_RECORDER_EOS).\n");
                    PostAppMsg(IMediaControl::MSG_RECORDER_EOS);
                    AMLOG_PRINTF("** PostAppMsg(IMediaControl::MSG_RECORDER_EOS) done.\n");
                    NewSession();
                }
            }
            break;

        case MSG_DURATION:
        case MSG_FILESIZE:
            AMLOG_PRINTF("INTERNAL_ENDING msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
            AM_ASSERT(mState == STATE_RECORDING);
            {
                int muxerIndex = -1;
                for (i = 0; i < DMaxMuxerNumber; i++) {
                    if (((IFilter *)msg.p0) == mpMuxerFilters[i]) {
                        muxerIndex = i;//muxer's index
                    }
                }
                AM_ASSERT(muxerIndex >=0 && muxerIndex < DMaxMuxerNumber);
                AM_MSG msg0;
                memset(&msg0,0,sizeof(AM_MSG));
                msg0.p4 = muxerIndex;
                if(msg.code == MSG_DURATION){
                    msg0.code = IMediaControl::MSG_RECORDER_REACH_DURATION;
                }else /*if(mPresetMsgCode == MSG_FILESIZE)*/{
                    AM_ASSERT(msg.code == MSG_FILESIZE);
                    msg0.code = IMediaControl::MSG_RECORDER_REACH_FILESIZE;
                }
                PostAppMsg(msg0);
            }
            break;

        case MSG_GENERATE_THUMBNAIL:
            AMLOG_PRINTF("MSG_GENERATE_THUMBNAIL msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
            AM_ASSERT(mState == STATE_RECORDING);

            //only do it for full resolution stream
            if (((IFilter *)msg.p1) != mpMuxerFilters[0]) {
                break;
            }

            if (mState == STATE_RECORDING) {
                AM_ASSERT(mpVideoEncoder);
                if (mpVideoEncoder) {

                    //check if size changed, todo...
                    if ((mThumbNailWidth != mShared.encoding_mode_config.thumbnail_width) || (mThumbNailHeight != mShared.encoding_mode_config.thumbnail_height)) {
                        if (mpThumbNailRawBuffer) {
                            free(mpThumbNailRawBuffer);
                            mpThumbNailRawBuffer = NULL;
                        }
                        AMLOG_INFO("free previous buffer, thumbnail jpeg size changes: %dx%d --> %dx%d.\n", mThumbNailWidth, mThumbNailHeight, mShared.encoding_mode_config.thumbnail_width, mShared.encoding_mode_config.thumbnail_height);
                        //update thumbnail width/height
                        mThumbNailWidth = mShared.encoding_mode_config.thumbnail_width;
                        mThumbNailHeight = mShared.encoding_mode_config.thumbnail_height;
                    }

                    //alloc buffer if needed
                    if (!mpThumbNailRawBuffer) {
                        mThumbNailRawBufferSize = mThumbNailWidth*mThumbNailHeight*4 + 32;
                        mpThumbNailRawBuffer = (AM_U8*)malloc(mThumbNailRawBufferSize);
                        AMLOG_PRINTF("malloc thumbnail buffer, tot size %d, width %d, height %d.\n", mThumbNailRawBufferSize, mThumbNailWidth, mThumbNailHeight);
                    }

                    AM_ASSERT(mpThumbNailRawBuffer);
                    if (mpThumbNailRawBuffer) {
                        AM_U8* src[3];
                        AM_U8* des[3];
                        AM_INT src_stride[3];
                        AM_INT des_stride[3];

                        //capture raw data
                        err = mpVideoEncoder->CaptureRawData(mpThumbNailRawBuffer, mThumbNailWidth, mThumbNailHeight, IParameters::PixFormat_NV12);
                        if (ME_OK != err) {
                            AM_ERROR("mpVideoEncoder->CaptureRawData() fail, ret %d.\n", err);
                            break;
                        }
                        //convert to yuv420p
                        src[0] = des[0] = mpThumbNailRawBuffer;
                        src[1] = mpThumbNailRawBuffer + mThumbNailWidth*mThumbNailHeight;
                        src_stride[0] = des_stride[0] = mThumbNailWidth;
                        src_stride[1] = mThumbNailWidth;

                        des[1] = mpThumbNailRawBuffer + (mThumbNailWidth*mThumbNailHeight)*3/2;
                        des[2] = mpThumbNailRawBuffer + (mThumbNailWidth*mThumbNailHeight)*7/4;
                        des_stride[1] = des_stride[2] = mThumbNailWidth/2;
                        AMLOG_INFO("before convert pix format.\n");
                        AM_ConvertPixFormat(IParameters::PixFormat_NV12, IParameters::PixFormat_YUV420P, src, des, src_stride, des_stride, mThumbNailWidth, mThumbNailHeight);

                        AMLOG_INFO("before FF_GenerateJpegFile, filename %s.\n", (char*)msg.p3);
                        //encoded to jpeg file
                        err = FF_GenerateJpegFile((char*)msg.p3, des, mThumbNailWidth, mThumbNailHeight, IParameters::PixFormat_YUV420P, 0, mpThumbNailRawBuffer + (mThumbNailWidth*mThumbNailHeight*2), mThumbNailWidth*mThumbNailHeight*2);
                        AM_ASSERT(ME_OK == err);
                    }
                }
            }
            break;

        case MSG_STORAGE_ERROR:
            AMLOG_ERROR("STORAGE_ERROR msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
            AM_ASSERT(mState == STATE_RECORDING);
            if (mState == STATE_RECORDING) {

                //do not delete file automatically, abort recording
                /*
                if (IParameters::MuxerSavingFileStrategy_AutoSeparateFile == mMuxerConfig[0].mSavingStrategy) {
                    //delete the most earliest file
                    for (i = 0; i < DMaxMuxerNumber; i++) {
                        //send delete exclude itself
                        if ((NULL == mpMuxerFilters[i]) || ((IFilter *)msg.p1) == mpMuxerFilters[i]) {
                            continue;
                        }
                        mpMuxerFilters[i]->PostMsg(CMD_DELETE_FILE);
                    }
                } else */{
                    //post msg to app
                    AMLOG_ERROR("MSG_STORAGE_ERROR msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
                    msg0.code = IMediaControl::MSG_STORAGE_ERROR_POST_TO_APP;
                    msg0.sessionID = mSessionID;
                    msg0.p2 = msg.p2;//current index
                    msg0.p3 = msg.p3;
                    for (i = 0; i < DMaxMuxerNumber; i++) {
                        if (((IFilter *)msg.p1) == mpMuxerFilters[i]) {
                            msg0.p4 = i;//muxer's index
                        }
                    }
                    msg0.p5 = 0;
                    PostAppMsg(msg0);

                    //abort recording, the muxed file maybe corrupted
                    AM_ERROR("NOT ENOUGH storage!!! need preset max file size or max duration according to storage status, or use auto-slip/auto-delete saving strategy.\n");
                    AMLOG_ERROR("Abort recording now.\n");
                    SetState(STATE_STOPPING);
                    AMLOG_INFO("sendStopRecord start.\n");
                    sendStopRecord();
                    AMLOG_INFO("sendStopRecord end.\n");
                }
            }
            break;

        case MSG_OS_ERROR:
            AMLOG_ERROR("MSG_OS_ERROR msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);

            //post msg to app
            msg0.code = IMediaControl::MSG_OS_ERROR_POST_TO_APP;
            msg0.sessionID = mSessionID;
            msg0.p2 = msg.p2;//current index
            msg0.p3 = msg.p3;
            for (i = 0; i < DMaxMuxerNumber; i++) {
                if (((IFilter *)msg.p1) == mpMuxerFilters[i]) {
                    msg0.p4 = i;//muxer's index
                }
            }
            msg0.p5 = 0;
            PostAppMsg(msg0);

            AM_ASSERT(mState == STATE_RECORDING);
            if (mState == STATE_RECORDING) {
                //abort recording
                AM_ERROR("IAV Error!!!Abort recording now, need stop muxer first? if bits-fifo is unmapped, it will cause some data pointer invalid, access them will cause SEG-Fault.\n");

#if 0
                //need stop muxer first, stop writing file
                for (i = 0; i < DMaxMuxerNumber; i++) {
                    if (mpMuxerFilters[i]) {
                        AMLOG_INFO("mpMuxerFilters[%d]->Stop() start.\n", i);
                        mpMuxerFilters[i]->Stop();
                        AMLOG_INFO("mpMuxerFilters[%d]->Stop() end.\n", i);
                    }
                }
#endif

                SetState(STATE_STOPPING);

                //stop recording
                AMLOG_INFO("sendStopRecord start.\n");
                sendStopRecord();
                AMLOG_INFO("sendStopRecord end.\n");
            }
            break;

        case MSG_NEWFILE_GENERATED:
            AMLOG_PRINTF("MSG_NEWFILE_GENERATED msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
            msg0.code = IMediaControl::MSG_NEWFILE_GENERATED_POST_TO_APP;
            msg0.sessionID = mSessionID;
            msg0.p2 = msg.p2;//current index
            msg0.p3 = msg.p3;
            for (i = 0; i < DMaxMuxerNumber; i++) {
                if (((IFilter *)msg.p1) == mpMuxerFilters[i]) {
                    msg0.p4 = i;//muxer's index
                }
            }
            msg0.p5 = msg.p5;//container type
            PostAppMsg(msg0);
            break;

        case MSG_FILE_NUM_REACHED_LIMIT:
            AMLOG_PRINTF("MSG_FILE_NUM_REACHED_LIMIT msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
            msg0.code = IMediaControl::MSG_RECORDER_REACH_FILENUM;
            msg0.sessionID = mSessionID;
            msg0.p2 = msg.p2;//current index
            msg0.p3 = msg.p3;
            for (i = 0; i < DMaxMuxerNumber; i++) {
                if (((IFilter *)msg.p1) == mpMuxerFilters[i]) {
                    msg0.p4 = i;//muxer's index
                }
            }
            msg0.p5 = 0;
            PostAppMsg(msg0);
            AM_ASSERT(mState == STATE_RECORDING);
            if (mState == STATE_RECORDING) {
                AMLOG_ERROR("Abort recording now.\n");
                SetState(STATE_STOPPING);
                //stop recording
                AMLOG_INFO("sendStopRecord start.\n");
                sendStopRecord();
                AMLOG_INFO("sendStopRecord end.\n");
            }
            break;

        case MSG_EVENT_NOTIFICATION:
            AMLOG_PRINTF("MSG_IDR_EVENT_NOTIFICATION msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
            msg0.code = IMediaControl::MSG_EVENT_NOTIFICATION_TO_APP;
            msg0.sessionID = mSessionID;
            msg0.p2 = msg.p2;//stream id
            msg0.p3 = msg.p3;//event type
            PostAppMsg(msg0);
            break;

        case MSG_ERROR:
            AMLOG_ERROR("MSG_ERROR msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
            msg0.code = IMediaControl::MSG_ERROR_POST_TO_APP;
            msg0.sessionID = mSessionID;
            msg0.p2 = msg.p2;//current index
            msg0.p3 = msg.p3;
            for (i = 0; i < DMaxMuxerNumber; i++) {
                if (((IFilter *)msg.p1) == mpMuxerFilters[i]) {
                    msg0.p4 = i;//muxer's index
                }
            }
            msg0.p5 = msg.p5;//container type
            PostAppMsg(msg0);
            if (mState == STATE_RECORDING) {
                AMLOG_ERROR("Abort recording now.\n");
                SetState(STATE_STOPPING);
                //stop recording
                AMLOG_INFO("sendStopRecord start.\n");
                sendStopRecord();
                AMLOG_INFO("sendStopRecord end.\n");
            }
            break;

        default:
            break;
    }
    return true;
}

bool CRecordEngine2::ProcessGenericCmd(CMD& cmd)
{
    AM_ERR err;
    AM_UINT i;

    AMLOG_CMD("****CRecordEngine2::ProcessGenericCmd, cmd.code %d, state %d.\n", cmd.code, mState);

    switch (cmd.code) {

        case CMD_STOP:
            AMLOG_INFO("****CRecordEngine2 STOP cmd, start.\n");
            mbRun = false;
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CRecordEngine2 STOP cmd, done.\n");
            break;

        case RECCMD_PAUSE_RESUME:
            AMLOG_INFO("****CRecordEngine2 PAUSE_RESUME cmd, flag %d, start.\n", cmd.flag);
            if (cmd.flag == 0) {
                if (mpVideoEncoderFilter) {
                    mpVideoEncoderFilter->FlowControl(IFilter::FlowControl_pause);
                }
                if (mpAudioInputFilter) {
                    mpAudioInputFilter->FlowControl(IFilter::FlowControl_pause);
                }
            } else if (cmd.flag == 1) {
                if (mpVideoEncoderFilter) {
                    mpVideoEncoderFilter->FlowControl(IFilter::FlowControl_resume);
                }
                if (mpAudioInputFilter) {
                    mpAudioInputFilter->FlowControl(IFilter::FlowControl_resume);
                }
            } else {
                AM_ASSERT(0);
            }
            AMLOG_INFO("****CRecordEngine2 PAUSE_RESUME cmd, done.\n");
            break;

        case RECCMD_CLOSE_OPEN_AUDIO_HAL:
            AMLOG_INFO("****CRecordEngine2 CLOSE_OPEN_AUDIO_HAL cmd, flag %d, start.\n", cmd.flag);
            if (cmd.flag == 0) {
                if (mpAudioInputFilter) {
                    mpAudioInputFilter->FlowControl(IFilter::FlowControl_close_audioHAL);
                }
            } else if (cmd.flag == 1) {
                if (mpAudioInputFilter) {
                    mpAudioInputFilter->FlowControl(IFilter::FlowControl_reopen_audioHAL);
                }
            } else {
                AM_ASSERT(0);
            }
            AMLOG_INFO("****CRecordEngine2 CLOSE_OPEN_AUDIO_HAL cmd, done.\n");
            break;

        default:
            AM_ERROR("unknown cmd %d.\n", cmd.code);
            return false;
    }
    return true;
}

void CRecordEngine2::ProcessCMD(CMD& oricmd)
{
    AM_UINT remaingCmd;
    CMD cmd1;
    AM_ASSERT(mpCmdQueue);

    if (oricmd.repeatType == CMD_TYPE_SINGLETON) {
        ProcessGenericCmd(oricmd);
        return;
    }

    remaingCmd = mpCmdQueue->GetDataCnt();
    if (remaingCmd == 0) {
        ProcessGenericCmd(oricmd);
        return;
    } else if (remaingCmd == 1) {
        ProcessGenericCmd(oricmd);
        mpCmdQueue->PeekMsg(&cmd1,sizeof(cmd1));
        ProcessGenericCmd(cmd1);
        return;
    }

    //process multi cmd
    if (oricmd.repeatType == CMD_TYPE_REPEAT_LAST) {
        while (mpCmdQueue->PeekMsg(&cmd1, sizeof(cmd1))) {
            if (cmd1.code != oricmd.code) {
                ProcessGenericCmd(oricmd);
                ProcessGenericCmd(cmd1);
                return;
            }
        }
        ProcessGenericCmd(cmd1);
        return;
    } else if (oricmd.repeatType == CMD_TYPE_REPEAT_CNT) {
        //to do
        ProcessGenericCmd(oricmd);
        return;
    } else if (oricmd.repeatType == CMD_TYPE_REPEAT_AVATOR) {
        while (mpCmdQueue->PeekMsg(&cmd1, sizeof(cmd1))) {
            if (cmd1.code != oricmd.code) {
                ProcessGenericCmd(oricmd);
                ProcessGenericCmd(cmd1);
                return;
            } else {
                //to do
                ProcessGenericCmd(cmd1);
                return;
            }
        }
    } else {
        AMLOG_ERROR("must not come here.\n");
        return;
    }
}

void CRecordEngine2::PrintState()
{
#ifdef AM_DEBUG
    AM_UINT i = 0;
    AMLOG_INFO(" CRecordEngine2's state is %d.\n", mState);
    for (i=0; i<mnFilters; i++) {
        mFilters[i].pFilter->PrintState();
    }

    if (mpStreammingServerManager) {
        mpStreammingServerManager->PrintState();
    }
#endif
}

void CRecordEngine2::OnRun()
{
    CMD cmd, nextCmd;
    AM_MSG msg;
    CQueue::QType type;
    CQueueInputPin* pPin;
    CQueue::WaitResult result;
    AM_ASSERT(mpFilterMsgQ);
    AM_ASSERT(mpCmdQueue);
    mpWorkQ->CmdAck(ME_OK);

    while (mbRun) {
        AMLOG_STATE("CRecordEngine2: mState=%d, msg cnt from filters %d.\n", mState, mpFilterMsgQ->GetDataCnt());

        //wait user/app set data source
        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
        if(type == CQueue::Q_MSG) {
            ProcessCMD(cmd);
        } else if (type == CQueue::Q_DATA) {
            //msg from filters
            AM_ASSERT(mpFilterMsgQ == result.pDataQ);
            if (result.pDataQ->PeekData(&msg, sizeof(msg))) {
                if (!ProcessGenericMsg(msg)) {
                    //need implement, process the msg
                    AMLOG_ERROR("not-processed cmd %d.\n", msg.code);
                }
            }
        } else {
            AMLOG_ERROR("Fatal error here.\n");
        }
    }

    AMLOG_STATE("CRecordEngine2: start Clear gragh for safe.\n");
    ClearGraph();
    NewSession();
}

#if PLATFORM_ANDROID_ITRON
AM_ERR CRecordEngine2::CreateGraph_ITron(void)
{
    AM_ERR err;
    AM_UINT i;
    AM_UINT video_index = 0, audio_index = 0;
    AM_UINT inpin_index, outpin_index;

    checkParameters();

    if ((mpVideoEncoderFilter = CreateITronEncoderFilter((IEngine*)this, false)) == NULL) {
        return ME_ERROR;
    }
    if ((err = AddFilter(mpVideoEncoderFilter)) != ME_OK)
        return err;
    mpVideoEncoder = IVideoEncoder::GetInterfaceFrom(mpVideoEncoderFilter);
    AM_ASSERT(mpVideoEncoder);

    //set muxer...............
    for (i=0; i<mTotalMuxerNumber; i++) {
        video_index = 0;
        audio_index = 0;
        // ffmpeg muxer
        if ((mpMuxerFilters[i] = CreateFFmpegMuxer((IEngine*)this)) == NULL) {
            AM_ERROR("CFFMpegMuxer failed");
            return ME_ERROR;
        }
        if ((err = AddFilter(mpMuxerFilters[i])) != ME_OK)
            return err;

        mpMuxerControl[i] = IMuxerControl::GetInterfaceFrom(mpMuxerFilters[i]);
        AM_ASSERT(mpMuxerControl[i]);

        mpMuxerControl[i]->SetContainerType(mMuxerConfig[i].out_format);
        setOutputFile(i);

        err = mpVideoEncoderFilter->AddOutputPin(outpin_index, IParameters::StreamType_Video);
        err = mpMuxerFilters[i]->AddInputPin(inpin_index, IParameters::StreamType_Video);

        //connect video
        if ((err = Connect(mpVideoEncoderFilter, outpin_index, mpMuxerFilters[i], inpin_index)) != ME_OK){
            AM_ERROR("Connect failed");
            return err;
        }

        err = findNextStreamIndex(i, video_index, IParameters::StreamType_Video);
        AM_ASSERT(mMuxerConfig[i].stream_info[video_index].stream_type == IParameters::StreamType_Video);
        AM_ASSERT(mMuxerConfig[i].stream_info[video_index].stream_format == IParameters::StreamFormat_H264);
        err = mpMuxerControl[i]->SetParameters(mMuxerConfig[i].stream_info[video_index].stream_type, mMuxerConfig[i].stream_info[video_index].stream_format, &mMuxerConfig[i].stream_info[video_index].spec, inpin_index);
        err = mpVideoEncoder->SetParameters(mMuxerConfig[i].stream_info[video_index].stream_type, mMuxerConfig[i].stream_info[video_index].stream_format, &mMuxerConfig[i].stream_info[video_index].spec, outpin_index);
        AMLOG_DEBUG("find video index %d, muxer %d.\n", video_index, i);
        video_index++;//next's start index

        err = mpVideoEncoderFilter->AddOutputPin(outpin_index, IParameters::StreamType_Audio);
        err = mpMuxerFilters[i]->AddInputPin(inpin_index, IParameters::StreamType_Audio);
            //connect audio
        if ((err = Connect(mpVideoEncoderFilter, outpin_index, mpMuxerFilters[i], inpin_index)) != ME_OK){
            AM_ERROR("Connect failed");
            return err;
        }

        err = findNextStreamIndex(i, audio_index, IParameters::StreamType_Audio);
        AM_ASSERT(mMuxerConfig[i].stream_info[audio_index].stream_type == IParameters::StreamType_Audio);
        AMLOG_DEBUG("find audio index %d, i %d, input_index %d, stream format %d, sr %d.\n", audio_index, i, inpin_index, mMuxerConfig[i].stream_info[audio_index].stream_format, mMuxerConfig[i].stream_info[audio_index].spec.audio.sample_rate);

        //mMuxerConfig[i].stream_info[audio_index].spec.audio.need_skip_adts_header = 1;

        err = mpVideoEncoder->SetParameters(mMuxerConfig[i].stream_info[video_index].stream_type, mMuxerConfig[i].stream_info[video_index].stream_format, &mMuxerConfig[i].stream_info[video_index].spec, outpin_index);
        err = mpMuxerControl[i]->SetParameters(mMuxerConfig[i].stream_info[audio_index].stream_type,
            mMuxerConfig[i].stream_info[audio_index].stream_format, &mMuxerConfig[i].stream_info[audio_index].spec, inpin_index);
        audio_index ++;//next's start index

    }

    return ME_OK;
}
#endif

AM_ERR CRecordEngine2::CreateGraph_muxer2(void)
{
    AM_ERR err;
    AM_UINT i;
    AM_UINT video_index = 0, audio_index = 0;

    checkParameters();

    AMLOG_PRINTF("*****1 mTotalVideoStreamNumber %d.\n", mTotalVideoStreamNumber);
    if (!mbOnlyEncAudio) {
        //debug assert
        AM_ASSERT(mTotalVideoStreamNumber == 2);
        if ((mpVideoEncoderFilter = CreateVideoEncoderFilter((IEngine*)this, mTotalVideoStreamNumber)) == NULL) {
            return ME_ERROR;
        }
        if ((err = AddFilter(mpVideoEncoderFilter)) != ME_OK)
            return err;

        mpVideoEncoder = IVideoEncoder::GetInterfaceFrom(mpVideoEncoderFilter);
        AM_ASSERT(mpVideoEncoder);
    }

    if (mShared.audio_enable) {
        // audio input filter
        if ((mpAudioInputFilter = CreateAudioInput2((IEngine*)this)) == NULL) {
            AM_ERROR("CreateAudioInput2 failed");
            return ME_ERROR;
        }
        if ((err = AddFilter(mpAudioInputFilter)) != ME_OK)
            return err;

        // audio encoder filter
        if (g_GlobalCfg.mUsePrivateAudioEncLib == 1) {
            if ((mpAudioEncoderFilter = CreateAACEncoder((IEngine*)this)) == NULL) {
                AM_ERROR("CreateAudioEncoder failed");
                return ME_ERROR;
            }
        } else {
            //default use this one
            if ((mpAudioEncoderFilter = CreateFFmpegAudioEncoder((IEngine*)this)) == NULL) {
                AM_ERROR("CreateAudioEncoder failed");
                return ME_ERROR;
            }
        }

        if ((err = AddFilter(mpAudioEncoderFilter)) != ME_OK)
            return err;
    }

#if 1
    // ffmpeg muxer
    if ((mpMuxerFilters[0] = CreateFFmpegMuxer2((IEngine*)this)) == NULL) {
        AM_ERROR("CreateFFmpegMuxer2 failed");
        return ME_ERROR;
    }
#else
    if ((mpMuxerFilters[0] = CreateSimpleMuxer((IRecordEngine*)this)) == NULL) {
        AM_ERROR("CreateSimpleMuxer failed");
        return ME_ERROR;
    }
#endif
    if ((err = AddFilter(mpMuxerFilters[0])) != ME_OK)
        return err;
    setOutputFile(0);
    if (!mbOnlyEncAudio) {
        // connect
        if ((err = Connect(mpVideoEncoderFilter, 0, mpMuxerFilters[0], 0)) != ME_OK){
            AM_ERROR("Connect failed");
            return err;
        }

        err = findNextStreamIndex(0, video_index, IParameters::StreamType_Video);
        AM_ASSERT(mMuxerConfig[0].stream_info[video_index].stream_type == IParameters::StreamType_Video);
        AM_ASSERT(mMuxerConfig[0].stream_info[video_index].stream_format == IParameters::StreamFormat_H264);
        err = mpVideoEncoder->SetParameters(mMuxerConfig[0].stream_info[video_index].stream_type, mMuxerConfig[0].stream_info[video_index].stream_format, &mMuxerConfig[0].stream_info[video_index].spec, 0);
        AMLOG_DEBUG("find video index %d, muxer2 %d.\n", video_index, 0);
        video_index++;//next's start index
    }

    if (mShared.audio_enable) {
        if ((err = Connect(mpAudioInputFilter, 0, mpAudioEncoderFilter, 0)) != ME_OK)
            return err;

        if ((err = Connect(mpAudioEncoderFilter, 0, mpMuxerFilters[0], 1)) != ME_OK)
            return err;

        err = findNextStreamIndex(0, audio_index, IParameters::StreamType_Audio);
        AM_ASSERT(mMuxerConfig[0].stream_info[audio_index].stream_type == IParameters::StreamType_Audio);
        AMLOG_DEBUG("find audio index %d, i %d, stream format %d, sr %d.\n", audio_index, 0, mMuxerConfig[0].stream_info[audio_index].stream_format, mMuxerConfig[0].stream_info[audio_index].spec.audio.sample_rate);

        err = mpAudioInput->SetParameters(mMuxerConfig[0].stream_info[audio_index].stream_type,
            mMuxerConfig[0].stream_info[audio_index].stream_format, &mMuxerConfig[0].stream_info[audio_index].spec);

        mpAudioEncoder->SetParameters(mMuxerConfig[0].stream_info[audio_index].stream_type,
            mMuxerConfig[0].stream_info[audio_index].stream_format, &mMuxerConfig[0].stream_info[audio_index].spec);
        audio_index ++;//next's start index
    }

    mTotalMuxerNumber = 1;
    return ME_OK;
}

bool CRecordEngine2::allvideoDisabled()
{
    AM_UINT total_number = 0, i = 0;
    if (!mShared.video_enable)
        return true;

    for (i = 0; i < DMaxMuxerNumber; i++) {
        total_number += mMuxerConfig[i].video_number;
    }
    if (total_number) {
        return false;
    }
    return true;
}

bool CRecordEngine2::allaudioDisabled()
{
    AM_UINT total_number = 0, i = 0;
    if (!mShared.audio_enable)
        return true;

    for (i = 0; i < DMaxMuxerNumber; i++) {
        total_number += mMuxerConfig[i].audio_number;
    }
    if (total_number) {
        return false;
    }
    return true;
}

bool CRecordEngine2::allpridataDisabled()
{
    AM_UINT total_number = 0, i = 0;
    if (!mShared.private_data_enable)
        return true;

    for (i = 0; i < DMaxMuxerNumber; i++) {
        total_number += mMuxerConfig[i].pridata_number;
    }
    if (total_number) {
        return false;
    }
    return true;
}

bool CRecordEngine2::allstreamingDisabled()
{
    AM_UINT i = 0;

    if (!mShared.streaming_enable)
        return true;

    for (i = 0; i < DMaxMuxerNumber; i++) {
        if (mMuxerConfig[i].video_streamming || mMuxerConfig[i].audio_streamming) {
            return false;
        }
    }

    return true;
}


CRecordEngine2::SPrivateDataConfig* CRecordEngine2::findConfigFromPrivateDataType(AM_U16 data_type)
{
    SPrivateDataConfig* p_config = mpPrivateDataConfigList;
    while (p_config) {
        if (p_config->data_type == data_type) {
            return p_config;
        }
        p_config = p_config->p_next;
    }

    //not found
    return NULL;
}

AM_ERR CRecordEngine2::SetPrivateDataDuration(AM_UINT duration, AM_U16 data_type)
{
    AUTO_LOCK(mpMutex);
    SPrivateDataConfig* p_config = findConfigFromPrivateDataType(data_type);

    if (!p_config) {
        p_config = (SPrivateDataConfig*)malloc(sizeof(SPrivateDataConfig));
        if (!p_config) {
            AM_ERROR("NO Memory.\n");
            return ME_NO_MEMORY;
        }
        p_config->duration = duration;
        p_config->data_type = data_type;

        p_config->p_next = mpPrivateDataConfigList;
        mpPrivateDataConfigList = p_config;
    } else {
        p_config->duration = duration;
    }
    return ME_OK;
}


AM_ERR CRecordEngine2::CreateGraph(void)
{
    AM_ERR err;
    AM_UINT i = 0;
    AM_UINT video_index = 0, audio_index = 0, pri_index = 0;
    AM_UINT inpin_index = 0, outpin_index = 0;
    //IParameters::UFormatSpecific config;
    IFilter* pFilter;
    bool need_add_output_pin = false;

    mbAllVideoDisabled = allvideoDisabled();
    mbAllAudioDisabled = allaudioDisabled();
    mbAllPridataDisabled = allpridataDisabled();
    mbAllStreamingDisabled = allstreamingDisabled();

    AMLOG_INFO("*****2 mbAllVideoDisabled %d, mbAllAudioDisabled %d, mbAllPridataDisabled %d, mbAllStreamingDisabled %d.\n", mbAllVideoDisabled, mbAllAudioDisabled, mbAllPridataDisabled, mbAllStreamingDisabled);

    //check if needed
    if (mbAllVideoDisabled && mbAllAudioDisabled) {
        AMLOG_ERROR("both audio and video are disabled, not start CreateGraph.\n");
        return ME_ERROR;
    }

//tmp
    AM_INT audio_fmt = 0;//0:aac, 1:mp2
    AMLOG_PRINTF("*****2 mTotalVideoStreamNumber %d.\n", mTotalVideoStreamNumber);

    mShared.pbConfig.vout_config = mRequestVoutMask;
    AM_ASSERT(mRequestVoutMask);
    checkParameters();

    AMLOG_INFO("[vout related info]: (rec engine) mRequestVoutMask 0x%x, dsp mode %d.\n", mRequestVoutMask, mShared.encoding_mode_config.dsp_mode);
    AMLOG_INFO("[vout related info]: pb display enabled %d, vout index %d.\n", mShared.encoding_mode_config.pb_display_enabled, mShared.encoding_mode_config.playback_vout_index);
    AMLOG_INFO("[vout related info]: preview display enabled %d, vout index %d.\n", mShared.encoding_mode_config.preview_enabled, mShared.encoding_mode_config.preview_vout_index);
    AMLOG_INFO("[vout related info]: playback display w %d, h %d, offset_x %d, offset_y %d.\n", mShared.encoding_mode_config.pb_display_width, mShared.encoding_mode_config.pb_display_height, \
        mShared.encoding_mode_config.pb_display_left, mShared.encoding_mode_config.pb_display_top);
    AMLOG_INFO("[vout related info]: preview display w %d, h %d, offset_x %d, offset_y %d, alpha %d.\n", mShared.encoding_mode_config.preview_width, mShared.encoding_mode_config.preview_height, \
        mShared.encoding_mode_config.preview_left, mShared.encoding_mode_config.preview_top, mShared.encoding_mode_config.preview_alpha);


    //debug assert
    //AM_ASSERT(mTotalVideoStreamNumber == 2);

    if (!mbAllVideoDisabled && !mbOnlyEncAudio) {
#if PLATFORM_ANDROID_ITRON
        if (mShared.use_itron_filter && mShared.video_from_itron) {
            AM_ASSERT(0 == mShared.audio_from_itron);
            if ((mpVideoEncoderFilter = CreateITronEncoderFilter((IEngine*)this, true)) == NULL) {
                return ME_ERROR;
            }
            need_add_output_pin = true;
        } else {
            if ((mpVideoEncoderFilter = CreateVideoEncoderFilter((IEngine*)this, mTotalVideoStreamNumber)) == NULL) {
                return ME_ERROR;
            }
        }
#else
        if ((mpVideoEncoderFilter = CreateVideoEncoderFilter((IEngine*)this, mTotalVideoStreamNumber)) == NULL) {
            return ME_ERROR;
        }
#endif
        if ((err = AddFilter(mpVideoEncoderFilter)) != ME_OK)
            return err;

        mpVideoEncoder = IVideoEncoder::GetInterfaceFrom(mpVideoEncoderFilter);
        AM_ASSERT(mpVideoEncoder);
        if (mpVideoEncoder) {
            err = mpVideoEncoder->SetModeConfig(&mShared.encoding_mode_config);
        }
    }

    if (!mbAllAudioDisabled && !mbOnlyEncVideo) {
        // audio input filter
        if ((mpAudioInputFilter = CreateAudioInput2((IEngine*)this)) == NULL) {
            AM_ERROR("CreateAudioInput2 failed.\n");
            return ME_ERROR;
        }
        if ((err = AddFilter(mpAudioInputFilter)) != ME_OK)
            return err;

        // audio encoder filter
        if (g_GlobalCfg.mUsePrivateAudioEncLib == 1) {
            if ((mpAudioEncoderFilter = CreateAACEncoder((IEngine*)this)) == NULL) {
                AM_ERROR("CreateAudioEncoder failed.\n");
                return ME_ERROR;
            }
            audio_fmt = 0;
        } else {
            //default use this one
            if ((mpAudioEncoderFilter = CreateFFmpegAudioEncoder((IEngine*)this)) == NULL) {
                AM_ERROR("CreateAudioEncoder failed.\n");
                return ME_ERROR;
            }
            audio_fmt = 1;
        }

        if ((err = AddFilter(mpAudioEncoderFilter)) != ME_OK)
            return err;

        if ((err = Connect(mpAudioInputFilter, 0, mpAudioEncoderFilter, 0)) != ME_OK)
            return err;
    }

    if (!mbAllPridataDisabled) {
        // pridata input filter
        AMLOG_INFO("before CreatePridataComposer.\n");
        if ((mpPridataComposerFilter = CreatePridataComposer((IEngine*)this)) != NULL) {
            if ((err = AddFilter(mpPridataComposerFilter)) != ME_OK) {
                AM_ERROR("!!!AddFilter Error? err %d.\n", err);
                return err;
            }
            mpPridataComposer = IPridataComposer::GetInterfaceFrom(mpPridataComposerFilter);
            AM_ASSERT(mpPridataComposer);

            //set private durations
            SPrivateDataConfig* p_config = mpPrivateDataConfigList;
            while (p_config) {
                mpPridataComposer->SetPridataDuration(p_config->duration, p_config->data_type);
                p_config = p_config->p_next;
            }

        } else {
            mbAllPridataDisabled = true;
            AMLOG_WARN("CreatePridataComposer failed.\n");
        }
        AMLOG_INFO("after CreatePridataComposer, mpPridataComposerFilter %p, mpPridataComposer %p.\n", mpPridataComposerFilter, mpPridataComposer);
    }

    //debug only
    //AM_ASSERT(mTotalMuxerNumber == 2);

#ifndef USE_RTSP_MUXER
//debug logic
//if (mMuxerConfig[0].out_format != IParameters::MuxerContainer_RTSP_LiveStreamming) {
    for (i=0; i<mTotalMuxerNumber; i++) {
        audio_index = 0;
        video_index = 0;
        // ffmpeg muxer
        if ((mpMuxerFilters[i] = CreateFFmpegMuxer((IEngine*)this)) == NULL) {
            AM_ERROR("CFFMpegMuxer failed.\n");
            return ME_ERROR;
        }
        if ((err = AddFilter(mpMuxerFilters[i])) != ME_OK)
            return err;

        mpMuxerControl[i] = IMuxerControl::GetInterfaceFrom(mpMuxerFilters[i]);
        AM_ASSERT(mpMuxerControl[i]);

        mpMuxerControl[i]->SetContainerType(mMuxerConfig[i].out_format);
        setOutputFile(i);

        mpStreamingDataTransmiter[i] = IStreammingDataTransmiter::GetInterfaceFrom(mpMuxerFilters[i]);
        AMLOG_INFO("mpStreamingDataTransmiter[i] %p, %d.\n", mpStreamingDataTransmiter[i], i);
        AM_ASSERT(mpStreamingDataTransmiter[i]);

        if(mMuxerConfig[i].mMaxFileSizeBytes){
            AM_U64 max_file_size = mMuxerConfig[i].mMaxFileSizeBytes * 85/100; //TODO, muxer->maxFileSize means payload size
            mpMuxerControl[i]->SetPresetMaxFilesize(max_file_size);
        }

        if (!mbAllVideoDisabled && !mbOnlyEncAudio) {
            /******************/
            /* Video Path Settings */
            /******************/
            //add muxer input pin for video
            mpMuxerFilters[i]->AddInputPin(inpin_index, IParameters::StreamType_Video);
            if (need_add_output_pin) {
                err = mpVideoEncoderFilter->AddOutputPin(outpin_index, IParameters::StreamType_Video);
            } else {
                outpin_index = i;//hard code here, remove later
            }
            //connect video
            if ((err = Connect(mpVideoEncoderFilter, outpin_index, mpMuxerFilters[i], inpin_index)) != ME_OK){
                AM_ERROR("Connect failed.\n");
                return err;
            }

            err = findNextStreamIndex(i, video_index, IParameters::StreamType_Video);
            AM_ASSERT(mMuxerConfig[i].stream_info[video_index].stream_type == IParameters::StreamType_Video);
            AM_ASSERT(mMuxerConfig[i].stream_info[video_index].stream_format == IParameters::StreamFormat_H264);
            //AMLOG_INFO(" *****!!!!111 w %d, h %d, entropy_type %d.\n", mMuxerConfig[i].stream_info[video_index].spec.video.pic_width, mMuxerConfig[i].stream_info[video_index].spec.video.pic_height, mMuxerConfig[i].stream_info[video_index].spec.video.entropy_type);
            err = mpMuxerControl[i]->SetParameters(mMuxerConfig[i].stream_info[video_index].stream_type, mMuxerConfig[i].stream_info[video_index].stream_format, &mMuxerConfig[i].stream_info[video_index].spec, inpin_index);
            err = mpMuxerControl[i]->SetSavingStrategy(mMuxerConfig[i].mSavingStrategy,mMuxerConfig[i].mSavingCondition, mMuxerConfig[i].mAutoNaming, mMuxerConfig[i].mConditionParam, false);
            if (mMuxerConfig[i].mMaxFileNumber) {
                err = mpMuxerControl[i]->SetMaxFileNumber(mMuxerConfig[i].mMaxFileNumber);
            }

            if(mMuxerConfig[i].mTotalFileNumber != 0){
                err = mpMuxerControl[i]->SetTotalFileNumber(mMuxerConfig[i].mTotalFileNumber);
            }
            //AMLOG_INFO(" *****!!!!222 w %d, h %d, entropy_type %d.\n", mMuxerConfig[i].stream_info[video_index].spec.video.pic_width, mMuxerConfig[i].stream_info[video_index].spec.video.pic_height, mMuxerConfig[i].stream_info[video_index].spec.video.entropy_type);
            err = mpVideoEncoder->SetParameters(mMuxerConfig[i].stream_info[video_index].stream_type, mMuxerConfig[i].stream_info[video_index].stream_format, &mMuxerConfig[i].stream_info[video_index].spec, outpin_index);

            if(mMuxerConfig[i].mMaxFileDurationUs){
                AM_UINT framerate_num = mMuxerConfig[i].stream_info[video_index].spec.video.framerate_num;
                AM_UINT framerate_den = mMuxerConfig[i].stream_info[video_index].spec.video.framerate_den;
                AM_U64 max_frame_count = mMuxerConfig[i].mMaxFileDurationUs * framerate_num /1000000/framerate_den;
                mpMuxerControl[i]->SetPresetMaxFrameCount(max_frame_count, IParameters::StreamType_Video);
            }

            AMLOG_INFO("find video index %d, muxer %d.\n", video_index, i);
            video_index++;//next's start index

        }


        if (!mbAllAudioDisabled && !mbOnlyEncVideo) {
            /******************/
            /* Audio Path Settings */
            /******************/
            mpAudioInput = IAudioInput::GetInterfaceFrom(mpAudioInputFilter);
            //add muxer input pin for audio
            mpMuxerFilters[i]->AddInputPin(inpin_index, IParameters::StreamType_Audio);

            //add audio encoder output pin
            mpAudioEncoder = IAudioEncoder::GetInterfaceFrom(mpAudioEncoderFilter);
            AM_ASSERT(mpAudioEncoder);
            mpAudioEncoderFilter->AddOutputPin(outpin_index, IParameters::StreamType_Audio);

            //connect audio
            if ((err = Connect(mpAudioEncoderFilter, outpin_index, mpMuxerFilters[i], inpin_index)) != ME_OK){
                AM_ERROR("Connect failed.\n");
                return err;
            }

            err = findNextStreamIndex(i, audio_index, IParameters::StreamType_Audio);
            AM_ASSERT(mMuxerConfig[i].stream_info[audio_index].stream_type == IParameters::StreamType_Audio);
            AMLOG_DEBUG("find audio index %d, i %d, input_index %d, stream format %d, sr %d.\n", audio_index, i, inpin_index, mMuxerConfig[i].stream_info[audio_index].stream_format, mMuxerConfig[i].stream_info[audio_index].spec.audio.sample_rate);

            if (audio_fmt == 0) {
                mMuxerConfig[i].stream_info[audio_index].spec.audio.need_skip_adts_header = 1;
            } else if (audio_fmt == 1){
                mMuxerConfig[i].stream_info[audio_index].spec.audio.need_skip_adts_header = 0;
            }

            err = mpAudioInput->SetParameters(mMuxerConfig[i].stream_info[audio_index].stream_type,
                mMuxerConfig[i].stream_info[audio_index].stream_format, &mMuxerConfig[i].stream_info[audio_index].spec);

            mpAudioEncoder->SetParameters(mMuxerConfig[i].stream_info[audio_index].stream_type,
                mMuxerConfig[i].stream_info[audio_index].stream_format, &mMuxerConfig[i].stream_info[audio_index].spec);

            mpMuxerControl[i]->SetParameters(mMuxerConfig[i].stream_info[audio_index].stream_type,
                mMuxerConfig[i].stream_info[audio_index].stream_format, &mMuxerConfig[i].stream_info[audio_index].spec, inpin_index);

            if(mMuxerConfig[i].mMaxFileDurationUs){
                AM_UINT frame_size = mMuxerConfig[i].stream_info[audio_index].spec.audio.frame_size;
                AM_U64 max_frame_count = mMuxerConfig[i].mMaxFileDurationUs * mMuxerConfig[i].stream_info[audio_index].spec.audio.sample_rate/1000000/frame_size;
                mpMuxerControl[i]->SetPresetMaxFrameCount(max_frame_count, IParameters::StreamType_Audio);
            }

            audio_index ++;//next's start index
        }

        if (!mbAllPridataDisabled && (mMuxerConfig[i].out_format == IParameters::MuxerContainer_TS)) {
            /******************/
            /* Pridata Path Settings */
            /******************/
            //add muxer input pin for private data
            mpMuxerFilters[i]->AddInputPin(inpin_index, IParameters::StreamType_PrivateData);

            //add audio encoder output pin
            mpPridataComposerFilter->AddOutputPin(outpin_index, IParameters::StreamType_PrivateData);

            //connect private data
            if ((err = Connect(mpPridataComposerFilter, outpin_index, mpMuxerFilters[i], inpin_index)) != ME_OK){
                AM_ERROR("Connect failed.\n");
                return err;
            }
            err = findNextStreamIndex(i, pri_index, IParameters::StreamType_PrivateData);
            AM_ASSERT(ME_OK == err);
            if (ME_OK == err) {
                mMuxerConfig[i].stream_info[pri_index].spec.pridata.duration = IParameters::TimeUnitDen_90khz;//default 1 second
                //get first duration
                if (mpPrivateDataConfigList) {
                    mMuxerConfig[i].stream_info[pri_index].spec.pridata.duration = mpPrivateDataConfigList->duration;
                }
                AMLOG_INFO("mMuxerConfig[i].stream_info[pri_index].spec.pridata.duration %d.\n", mMuxerConfig[i].stream_info[pri_index].spec.pridata.duration);
                mpMuxerControl[i]->SetParameters(mMuxerConfig[i].stream_info[pri_index].stream_type,
                    mMuxerConfig[i].stream_info[pri_index].stream_format, &mMuxerConfig[i].stream_info[pri_index].spec, inpin_index);
            }
        }

    }

#else
//}else {
        if(mbOnlyEncAudio || mbOnlyEncVideo){
            AM_ERROR("Only one stream mode is not supported under 'USE_RTSP_MUXER'!\n");
            return ME_ERROR;
        }

        // RTSP muxer
        if ((mpMuxerFilters[0] = CreateRTSPServer((IEngine*)this)) == NULL) {
            AM_ERROR("Create RTSP Server failed.\n");
            return ME_ERROR;
        }
        if ((err = AddFilter(mpMuxerFilters[0])) != ME_OK)
            return err;

        /******************/
        /* Video Path Settings */
        /******************/
        //connect video
        if ((err = Connect(mpVideoEncoderFilter, 0, mpMuxerFilters[0], 0)) != ME_OK){
            AM_ERROR("Connect failed.\n");
            return err;
        }

        err = findNextStreamIndex(0, video_index, IParameters::StreamType_Video);
        AM_ASSERT(mMuxerConfig[0].stream_info[video_index].stream_type == IParameters::StreamType_Video);
        AM_ASSERT(mMuxerConfig[0].stream_info[video_index].stream_format == IParameters::StreamFormat_H264);
        err = mpVideoEncoder->SetParameters(mMuxerConfig[0].stream_info[video_index].stream_type, mMuxerConfig[0].stream_info[video_index].stream_format, &mMuxerConfig[0].stream_info[video_index].spec, 0);
        AMLOG_DEBUG("find video index %d, rtsp muxer %d.\n", video_index, 0);
        video_index++;//next's start index

        if (mShared.audio_enable) {
            /******************/
            /* Audio Path Settings */
            /******************/
            mpAudioInput = IAudioInput::GetInterfaceFrom(mpAudioInputFilter);

            //add audio encoder output pin
            mpAudioEncoder = IAudioEncoder::GetInterfaceFrom(mpAudioEncoderFilter);
            AM_ASSERT(mpAudioEncoder);
            mpAudioEncoderFilter->AddOutputPin(outpin_index, IParameters::StreamType_Audio);

            //connect audio
            if ((err = Connect(mpAudioEncoderFilter, 0, mpMuxerFilters[0], 4)) != ME_OK){
                AM_ERROR("Connect failed.\n");
                return err;
            }

            inpin_index = 1;
            err = findNextStreamIndex(0, audio_index, IParameters::StreamType_Audio);
            AM_ASSERT(mMuxerConfig[0].stream_info[audio_index].stream_type == IParameters::StreamType_Audio);
            AMLOG_DEBUG("find audio index %d, rtps i %d, input_index %d, stream format %d, sr %d.\n", audio_index, 0, inpin_index, mMuxerConfig[0].stream_info[audio_index].stream_format, mMuxerConfig[0].stream_info[audio_index].spec.audio.sample_rate);

            mpAudioInput->SetParameters(mMuxerConfig[0].stream_info[audio_index].stream_type,
                mMuxerConfig[0].stream_info[audio_index].stream_format, &mMuxerConfig[0].stream_info[audio_index].spec);

            mpAudioEncoder->SetParameters(mMuxerConfig[0].stream_info[audio_index].stream_type,
                mMuxerConfig[0].stream_info[audio_index].stream_format, &mMuxerConfig[0].stream_info[audio_index].spec);
        }

#endif
//}

    //streaming server manager
    if ((!mbAllStreamingDisabled) || (mShared.auto_start_rtsp_steaming)) {
        AM_ASSERT(mpStreammingServerManager);
        if (!mpStreammingServerManager) {
            setupStreamingServerManger();
            if (!mpStreammingServerManager) {
                AM_ERROR("setupStreamingServerManger fail.\n");
            }
        }

        //hard code here
        SStreammingServerInfo* p_server = findStreamingServerByIndex(0);
        SStreamContext* p_content = findStreamingContentByIndex(0);
        AM_ASSERT(p_server && p_content);
        AMLOG_INFO("p_server %p, p_content %p.\n", p_server, p_content);

        if (!p_server) {
            AMLOG_WARN("no server here, create one.\n");
            //add new server
            p_server = mpStreammingServerManager->AddServer(IParameters::StreammingServerType_RTSP, IParameters::StreammingServerMode_MulticastSetAddr, mShared.rtsp_server_config.rtsp_listen_port, mShared.streaming_video_enable, mShared.streaming_audio_enable);

            AM_ASSERT(p_server);
            if (p_server) {
                //choose a not used server number
                while (findStreamingServerByIndex(mServerMagicNumber)) {
                    mServerMagicNumber ++;
                }
                p_server->index = mServerMagicNumber;
                mStreammingServerList.InsertContent(NULL, (void*)p_server, 0);
            }
        }

        if (!p_content) {
            p_content = generateStreamingContent(0);//hard code here
            AM_ASSERT(p_content);
            mStreamContextList.InsertContent(NULL, p_content, 0);
        }

        //add streaming content
        if (mpStreammingServerManager && p_content && p_server) {
            mpStreammingServerManager->AddStreammingContent(p_server, p_content);
        } else {
            AM_ERROR("NULL pointer in this case.\n");
        }
    }

    //set thumbnail file name,chuchen, 2012_5_18
    if(mpThumbNailFilename){
        setThumbNailFile(0);
    }
    return ME_OK;
}

AM_ERR CRecordEngine2::ExitPreview()
{
    AM_ERR err = ME_ERROR;
    AM_ASSERT(mState == STATE_STOPPED);
    AMLOG_INFO("CRecordEngine2::ExitPreview() start, mState %d, mpVideoEncoder %p.\n", mState, mpVideoEncoder);
    if (mState == STATE_STOPPED) {
        AM_ASSERT(mpVideoEncoder);
        if (mpVideoEncoder) {
            AMLOG_INFO("start mpVideoEncoder->ExitPreview.\n");
            err = mpVideoEncoder->ExitPreview();
            AMLOG_INFO("mpVideoEncoder->ExitPreview done, ret %d.\n", err);
        }
    }
    AMLOG_INFO("CRecordEngine2::ExitPreview() end.\n");
    return err;
}

AM_ERR CRecordEngine2::DisableVideo()
{
    mShared.video_enable = false;
    return ME_OK;
}

AM_ERR CRecordEngine2::DisableAudio()
{
    mShared.audio_enable = false;
    return ME_OK;
}

AM_ERR CRecordEngine2::SetMaxFileNumber(AM_UINT max_file_num, AM_UINT channel)
{
    if (DInvalidGeneralIntParam == channel) {
        AM_UINT index = 0;
        //set to all muxer
        for (index = 0; index < DMaxMuxerNumber; index ++) {
            mMuxerConfig[index].mMaxFileNumber = max_file_num;
        }
    } else {
        if (channel < DMaxMuxerNumber) {
            mMuxerConfig[channel].mMaxFileNumber = max_file_num;
        } else {
            AM_ERROR("BAD channel %d, DMaxMuxerNumber %d.\n", channel, DMaxMuxerNumber);
            return ME_BAD_PARAM;
        }
    }
    AMLOG_INFO("CRecordEngine2::SetMaxFileNumber %d, channel %d.\n", max_file_num, channel);
    return ME_OK;
}

AM_ERR CRecordEngine2::SetTotalFileNumber(AM_UINT total_file_num, AM_UINT channel)
{
    if (DInvalidGeneralIntParam == channel) {
        AM_UINT index = 0;
        //set to all muxer
        for (index = 0; index < DMaxMuxerNumber; index ++) {
            mMuxerConfig[index].mTotalFileNumber = total_file_num;
        }
    } else {
        if (channel < DMaxMuxerNumber) {
            mMuxerConfig[channel].mTotalFileNumber = total_file_num;
        } else {
            AM_ERROR("BAD channel %d, DMaxMuxerNumber %d.\n", channel, DMaxMuxerNumber);
            return ME_BAD_PARAM;
        }
    }
    AMLOG_INFO("CRecordEngine2::SetTotalFileNumber %d, channel %d.\n", total_file_num, channel);
    return ME_OK;
}

AM_ERR CRecordEngine2::SetTotalOutputNumber(AM_UINT& tot_number)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot set SetTotalOutputNumber.\n");
        return ME_BAD_STATE;
    }

    //default value
    mTotalMuxerNumber = DMaxMuxerNumber;

    if (tot_number <= mTotalMuxerNumber && tot_number != 0) {
        mTotalMuxerNumber = tot_number;
        mTotalVideoStreamNumber = mTotalMuxerNumber;
        return ME_OK;
    }

    AMLOG_WARN("Output number %d not supported, use default value: %d.\n", tot_number, mTotalMuxerNumber);
    tot_number = mTotalMuxerNumber;
    mTotalVideoStreamNumber = mTotalMuxerNumber;
    return ME_OK;
}


AM_ERR CRecordEngine2::SetupOutput(AM_UINT out_index, const char *pFileName, IParameters::ContainerType out_format)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot set SetupOutput.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    AM_ASSERT(pFileName);
    AM_ASSERT(out_format < IParameters::MuxerContainer_TotolNum);
    if (out_index >= mTotalMuxerNumber || !pFileName || out_format >= IParameters::MuxerContainer_TotolNum) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::SetupOutput, out_index %d, pFileName %p, out_format %d.\n", out_index, pFileName, out_format);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(mFilename[out_index] == NULL);
    if (mFilename[out_index]) {
        free((void*)mFilename[out_index]);
    }
    mFilename[out_index] = (char*)malloc(strlen(pFileName) + 4);
    AM_ASSERT(mFilename[out_index]);
    if (mFilename[out_index] == NULL) {
        return ME_NO_MEMORY;
    }
    memset(mFilename[out_index], 0, strlen(pFileName) + 4);
    memcpy(mFilename[out_index], pFileName, strlen(pFileName));

    if ((out_format == IParameters::MuxerContainer_AUTO) || (out_format >=IParameters::MuxerContainer_TotolNum) || (out_format == IParameters::MuxerContainer_Invalid)) {
        if (mShared.target_recorder_config[out_index].container_format == 0) {
            AM_ASSERT(0);
            mMuxerConfig[out_index].out_format = IParameters::MuxerContainer_MP4;
        } else {
            mMuxerConfig[out_index].out_format = (IParameters::ContainerType) mShared.target_recorder_config[out_index].container_format;
        }
    } else {
        mMuxerConfig[out_index].out_format = out_format;
    }
    return ME_OK;
}

AM_ERR CRecordEngine2::SetupThumbNailFile(const char *pThumbNailFileName)
{
    AM_ASSERT(mpThumbNailFilename == NULL);
    if(mpThumbNailFilename){
        free((void*)mpThumbNailFilename);
        mpThumbNailFilename = NULL;
    }

    AM_UINT filenamelen = strlen(pThumbNailFileName) + 4;
    mpThumbNailFilename = (char*)malloc(filenamelen);
    if(mpThumbNailFilename == NULL){
        return ME_NO_MEMORY;
    }
    memset(mpThumbNailFilename, 0, filenamelen);
    memcpy(mpThumbNailFilename, pThumbNailFileName, strlen(pThumbNailFileName));
    return ME_OK;
}

AM_ERR CRecordEngine2::SetupThumbNailParam(AM_UINT out_index,AM_INT enabled, AM_INT width, AM_INT height)
{
    mThumbNailWidth = width;
    mThumbNailHeight = height;

    //only support main stream now
    AM_ASSERT(0 == out_index);
    return ME_OK;
}

AM_ERR CRecordEngine2::SetSavingStrategy(IParameters::MuxerSavingFileStrategy strategy, IParameters::MuxerSavingCondition condition, IParameters::MuxerAutoFileNaming naming, AM_UINT param, AM_UINT channel)
{
    if (DInvalidGeneralIntParam == channel) {
        AM_UINT index = 0;
        //set to all muxer
        for (index = 0; index < DMaxMuxerNumber; index ++) {
            mMuxerConfig[index].mSavingStrategy = strategy;
            mMuxerConfig[index].mSavingCondition = condition;
            mMuxerConfig[index].mAutoNaming = naming;
            mMuxerConfig[index].mConditionParam = param;
        }
    } else {
        if (channel <DMaxMuxerNumber) {
            mMuxerConfig[channel].mSavingStrategy = strategy;
            mMuxerConfig[channel].mSavingCondition = condition;
            mMuxerConfig[channel].mAutoNaming = naming;
            mMuxerConfig[channel].mConditionParam = param;
        } else {
            AM_ERROR("BAD channel %d, DMaxMuxerNumber %d.\n", channel, DMaxMuxerNumber);
            return ME_BAD_PARAM;
        }
    }
    AMLOG_INFO("CRecordEngine2::SetSavingStrategy strategy %d, mSavingCondition %d, mAutoNaming %d, mAutoSavingParam %d, index %d.\n", mMuxerConfig[channel].mSavingStrategy, mMuxerConfig[channel].mSavingCondition, mMuxerConfig[channel].mAutoNaming, mMuxerConfig[channel].mConditionParam, channel);
    return ME_OK;
}

AM_ERR CRecordEngine2::NewStream(AM_UINT out_index, AM_UINT& stream_index, IParameters::StreamType type, IParameters::StreamFormat format)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot NewStream now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber || type >= IParameters::StreamType_TotalNum) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::NewStream, out_index %d, type %d, format %d.\n", out_index, type, format);
        return ME_BAD_PARAM;
    }
    stream_index = mMuxerConfig[out_index].stream_number;

    //get default parameters
    switch (type) {
        case IParameters::StreamType_Video:
            defaultVideoParameters(out_index, stream_index, out_index);
            mMuxerConfig[out_index].video_number ++;
            break;
        case IParameters::StreamType_Audio:
            defaultAudioParameters(out_index, stream_index);
            mMuxerConfig[out_index].audio_number ++;
            break;
        case IParameters::StreamType_Subtitle:
            AM_ASSERT(0);
            AMLOG_ERROR("not support here, need implement.\n");
            mMuxerConfig[out_index].subtitle_number ++;
            break;
        case IParameters::StreamType_PrivateData:
            if (mMuxerConfig[out_index].out_format != IParameters::MuxerContainer_TS)
                return ME_BAD_PARAM;
            mMuxerConfig[out_index].pridata_number ++;
            break;
        default:
            AM_ASSERT(0);
            AMLOG_ERROR("Wrong stream type %d.\n", type);
            return ME_BAD_PARAM;
    }
    mMuxerConfig[out_index].stream_info[stream_index].stream_type = type;

    if (format != IParameters::StreamFormat_Invalid) {
        //set by app
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = format;
    } else {
        //app not specify
    }

    mMuxerConfig[out_index].stream_number ++;
    return ME_OK;
}

AM_ERR CRecordEngine2::SetVideoStreamEntropyType(AM_UINT out_index, AM_UINT stream_index, IParameters::EntropyType entropy_type)
{
    //AMLOG_INFO("!!!comes here.\n");
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot SetVideoStreamEntropyType now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::SetVideoStreamEntropyType, out_index %d, stream index %d, entropy_type %d.\n", out_index, stream_index, entropy_type);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(stream_index < mMuxerConfig[out_index].stream_number);
    if (stream_index >= mMuxerConfig[out_index].stream_number) {
        AMLOG_ERROR("BAD stream_index in CRecordEngine2::SetVideoStreamEntropyType, out_index %d, stream index %d.\n", out_index, stream_index);
        return ME_BAD_PARAM;
    }

    if (entropy_type == IParameters::EntropyType_NOTSet) {
        AMLOG_WARN("Parameters not set, need invoke this api?\n");
    }
    mMuxerConfig[out_index].stream_info[stream_index].spec.video.entropy_type = entropy_type;
    //AMLOG_INFO("!!!!set entropy_type %d, result %d.\n", entropy_type, mMuxerConfig[out_index].stream_info[stream_index].spec.video.entropy_type);

    return ME_OK;
}

AM_ERR CRecordEngine2::SetVideoStreamDimention(AM_UINT out_index, AM_UINT stream_index, AM_UINT width, AM_UINT height)
{
    AMLOG_INFO(" CRecordEngine2::SetVideoStreamDimention, (out index %d, stream index %d), %d, %d.\n", out_index, stream_index, width, height);
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot SetVideoStreamDimention now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber || !width || !height) {
        AM_ERROR("BAD parameters in CRecordEngine2::SetVideoStreamDimention, out_index %d, stream index %d, width %d, height %d.\n", out_index, stream_index, width, height);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(stream_index < mMuxerConfig[out_index].stream_number);
    if (stream_index >= mMuxerConfig[out_index].stream_number) {
        AM_ERROR("BAD stream index %d, stream_number %d.\n", stream_index, mMuxerConfig[out_index].stream_number);
        return ME_BAD_PARAM;
    }

    if (!mShared.not_check_video_res) {
        AM_VideoEncCheckDimention(width, height, stream_index);
    } else {
        AM_INFO("not check video dimention [%d x %d].\n", width, height);
    }

    mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_width = width;
    mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_height = height;
    AMLOG_INFO(" CRecordEngine2::SetVideoStreamDimention done, (out index %d, stream index %d), %d, %d.\n", out_index, stream_index, mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_width, mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_height);

    return ME_OK;
}

AM_ERR CRecordEngine2::SetVideoStreamFramerate(AM_UINT out_index, AM_UINT stream_index, AM_UINT framerate_num, AM_UINT framerate_den)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot SetVideoStreamFramerate now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber || !framerate_num || !framerate_den) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::SetVideoStreamFramerate, out_index %d, stream index %d, framerate num %d, den %d.\n", out_index, stream_index, framerate_num, framerate_den);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(stream_index < mMuxerConfig[out_index].stream_number);
    if (stream_index >= mMuxerConfig[out_index].stream_number) {
        AMLOG_ERROR("Wrong stream index %d in CRecordEngine2::SetVideoStreamFramerate.\n", stream_index);
        return ME_BAD_PARAM;
    }

    mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_num = framerate_num;
    mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_den = framerate_den;

    return ME_OK;
}

AM_ERR CRecordEngine2::SetVideoStreamBitrate(AM_UINT out_index, AM_UINT stream_index, AM_UINT bitrate)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot SetVideoStreamBitrate now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber || !bitrate) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::SetVideoStreamBitrate, out_index %d, stream index %d, bitrate %d.\n", out_index, stream_index, bitrate);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(stream_index < mMuxerConfig[out_index].stream_number);
    if (stream_index >= mMuxerConfig[out_index].stream_number) {
        AMLOG_ERROR("Wrong stream index %d in CRecordEngine2::SetVideoStreamBitrate.\n", stream_index);
        return ME_BAD_PARAM;
    }

    mMuxerConfig[out_index].stream_info[stream_index].spec.video.bitrate = bitrate;
    AMLOG_INFO("SetVideoStreamBitrate: %d, %d, %d\n", out_index, stream_index, bitrate);
    return ME_OK;
}


AM_ERR CRecordEngine2::SetVideoStreamLowdelay(AM_UINT out_index, AM_UINT stream_index, AM_UINT lowdelay)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot SetVideoStreamLowdelay now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::SetVideoStreamLowdelay, out_index %d, stream index %d, lowdelay %d.\n", out_index, stream_index, lowdelay);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(stream_index < mMuxerConfig[out_index].stream_number);
    if (stream_index >= mMuxerConfig[out_index].stream_number) {
        AMLOG_ERROR("Wrong stream index %d in CRecordEngine2::SetVideoStreamLowdelay.\n", stream_index);
        return ME_BAD_PARAM;
    }

    mMuxerConfig[out_index].stream_info[stream_index].spec.video.lowdelay = lowdelay;
    return ME_OK;
}

AM_ERR CRecordEngine2::SetAudioStreamChannelNumber(AM_UINT out_index, AM_UINT stream_index, AM_UINT channel)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot SetAudioStreamChannelNumber now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::SetAudioStreamChannelNumber, out_index %d, stream index %d, channel %d.\n", out_index, stream_index, channel);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(stream_index < mMuxerConfig[out_index].stream_number);
    if (stream_index >= mMuxerConfig[out_index].stream_number) {
        AMLOG_ERROR("Wrong stream index %d in CRecordEngine2::SetAudioStreamChannelNumber.\n", stream_index);
        return ME_BAD_PARAM;
    }

    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_number = channel;
    return ME_OK;
}

AM_ERR CRecordEngine2::SetAudioStreamSampleFormat(AM_UINT out_index, AM_UINT stream_index, IParameters::AudioSampleFMT sample_fmt)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot SetAudioStreamSampleFormat now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::SetAudioStreamSampleFormat, out_index %d, stream index %d, channel %d.\n", out_index, stream_index, sample_fmt);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(stream_index < mMuxerConfig[out_index].stream_number);
    if (stream_index >= mMuxerConfig[out_index].stream_number) {
        AMLOG_ERROR("Wrong stream index %d in CRecordEngine2::SetAudioStreamSampleFormat.\n", stream_index);
        return ME_BAD_PARAM;
    }

    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_format = sample_fmt;
    return ME_OK;
}

AM_ERR CRecordEngine2::SetAudioStreamSampleRate(AM_UINT out_index, AM_UINT stream_index, AM_UINT samplerate)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot SetAudioStreamSampleRate now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::SetAudioStreamSampleRate, out_index %d, stream index %d, samplerate %d.\n", out_index, stream_index, samplerate);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(stream_index < mMuxerConfig[out_index].stream_number);
    if (stream_index >= mMuxerConfig[out_index].stream_number) {
        AMLOG_ERROR("Wrong stream index %d in CRecordEngine2::SetAudioStreamSampleRate.\n", stream_index);
        return ME_BAD_PARAM;
    }

    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_rate = samplerate;
    return ME_OK;
}

AM_ERR CRecordEngine2::SetAudioStreamBitrate(AM_UINT out_index, AM_UINT stream_index, AM_UINT bitrate)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot SetAudioStreamBitrate now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::SetAudioStreamBitrate, out_index %d, stream index %d, bitrate %d.\n", out_index, stream_index, bitrate);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(stream_index < mMuxerConfig[out_index].stream_number);
    if (stream_index >= mMuxerConfig[out_index].stream_number) {
        AMLOG_ERROR("Wrong stream index %d in CRecordEngine2::SetAudioStreamBitrate.\n", stream_index);
        return ME_BAD_PARAM;
    }

    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.bitrate = bitrate;
    return ME_OK;
}


AM_ERR CRecordEngine2::SetMaxFileDuration(AM_UINT out_index, AM_U64 timeUs)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot SetMaxFileDuration now.\n");
        return ME_BAD_STATE;
    }
    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::SetMaxFileDuration, out_index %d, timeUs %lld.\n", out_index, timeUs);
        return ME_BAD_PARAM;
    }
    mMuxerConfig[out_index].mMaxFileDurationUs = timeUs;
    AM_INFO("record engine's SetMaxFileDuration[%d] = %llu.\n", out_index,timeUs);
    return ME_OK;
}

AM_ERR CRecordEngine2::SetMaxFileSize(AM_UINT out_index, AM_U64 bytes)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot SetMaxFileSize now.\n");
        return ME_BAD_STATE;
    }
    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::SetMaxFileSize, out_index %d, bytes %lld.\n", out_index, bytes);
        return ME_BAD_PARAM;
    }
    mMuxerConfig[out_index].mMaxFileSizeBytes = bytes;
    AM_INFO("record engine's SetMaxFileSize[%d] = %llu.\n", out_index,bytes);
    return ME_OK;
}

AM_UINT CRecordEngine2::GetStreamContentNumber()
{
    return mnStreammingContentNumber;
}

SStreamContext* CRecordEngine2::GetStreamContent(AM_UINT index)
{
    CDoubleLinkedList::SNode* pnode;
    SStreamContext* p_content;
    AUTO_LOCK(mpMutex);

    pnode = mStreamContextList.FirstNode();
    while (pnode) {
        p_content = (SStreamContext*) pnode->p_context;
        if (p_content->index == index) {
            return p_content;
        }
        pnode = mStreamContextList.NextNode(pnode);
    }
    AM_ERROR("Can not find SStreamContext with index %d.\n", index);
    return NULL;
}

IStreammingDataTransmiter* CRecordEngine2::GetDataTransmiter(SStreamContext*content, AM_UINT index)
{
    CDoubleLinkedList::SNode* pnode;
    SStreamContext* p_content;
    AUTO_LOCK(mpMutex);

    pnode = mStreamContextList.FirstNode();
    while (pnode) {
        p_content = (SStreamContext*) pnode->p_context;
        if (p_content == content) {
            return p_content->p_data_transmeter;
        }
        pnode = mStreamContextList.NextNode(pnode);
    }
    AM_ERROR("Can not find SStreamContext with index %d, %p.\n", index, content);
    return NULL;
}

AM_ERR CRecordEngine2::getParametersFromRecConfig()
{
    AM_UINT out_index = 0, num = mShared.tot_muxer_number;
    AM_UINT stream_index;
    STargetRecorderConfig* pConfig;
    if (num == 0) {
        AM_ERROR("Nothing read from rec.config? must not comes here.\n");
        return ME_ERROR;
    }

    AMLOG_PRINTF("Basic info: number of muxer %d; enable video %d, audio %d, subtitle %d, gps sub info %d, amba trickplay info %d.\n", mShared.tot_muxer_number, mShared.video_enable, mShared.audio_enable, mShared.subtitle_enable, mShared.private_gps_sub_info_enable, mShared.private_am_trickplay_enable);

    for (out_index = 0; out_index < num; out_index++) {

        pConfig = &mShared.target_recorder_config[out_index];

        //video related, stream 0
        stream_index = 0;
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = IParameters::StreamType_Video;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = (IParameters::StreamFormat) pConfig->video_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_width = pConfig->pic_width;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_height = pConfig->pic_height;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_num = pConfig->video_framerate_num;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_den = pConfig->video_framerate_den;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_num = 1;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_den = 1;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.bitrate = pConfig->video_bitrate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.lowdelay = pConfig->video_lowdelay;

        //audio related, stream 1
        stream_index = 1;
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = IParameters::StreamType_Audio;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = (IParameters::StreamFormat)pConfig->audio_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_layout = 0;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_number = pConfig->channel_number;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_rate = pConfig->sample_rate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_format = (IParameters::AudioSampleFMT) pConfig->sample_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.frame_size = AUDIO_CHUNK_SIZE;//need set here?
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.bitrate = pConfig->audio_bitrate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.need_skip_adts_header = 0;

        //others streams, todo
    }

    return ME_OK;
}

AM_ERR CRecordEngine2::defaultVideoParameters(AM_UINT out_index, AM_UINT stream_index, AM_UINT ucode_stream_index)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot defaultVideoParameters now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::defaultVideoParameters, out_index %d, stream index %d.\n", out_index, stream_index);
        return ME_BAD_PARAM;
    }

    //if have input from external(rec.config, etc)
    if (out_index < mShared.tot_muxer_number) {
        STargetRecorderConfig* pConfig = &mShared.target_recorder_config[out_index];
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = IParameters::StreamType_Video;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = (IParameters::StreamFormat) pConfig->video_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_width = pConfig->pic_width;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_height = pConfig->pic_height;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_num = pConfig->video_framerate_num;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_den = pConfig->video_framerate_den;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_num = 1;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_den = 1;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.bitrate = pConfig->video_bitrate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.lowdelay = pConfig->video_lowdelay;
        //AMLOG_INFO("****!!!!pConfig->entropy_type %d.\n", pConfig->entropy_type);
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.entropy_type = pConfig->entropy_type;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.M = pConfig->M;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.N = pConfig->N;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.IDRInterval = pConfig->IDRInterval;
        return ME_OK;
    }

    AM_ASSERT(0);
    AMLOG_WARN("should not comes here.\n");
    //if have no input from external(rec.config, etc), use some hard coded default value
    if (ucode_stream_index == 0) {
        //a5s main stream
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = IParameters::StreamType_Video;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = IParameters::StreamFormat_H264;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_width = DefaultMainVideoWidth;//1080p
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_height = DefaultMainVideoHeight;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_num = 90000;//29.97 fps
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_den = 3003;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_num = 1;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_den = 1;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.bitrate = 8000000;//8M
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.lowdelay = 0;//have B picture
    } else {
        //a5s sub stream
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = IParameters::StreamType_Video;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = IParameters::StreamFormat_H264;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_width = DefaultPreviewCWidth;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.pic_height = DefaultPreviewCHeight;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_num = 90000;//29.97 fps
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.framerate_den = 3003;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_num = 1;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.sample_aspect_ratio_den = 1;
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.bitrate = 2000000;// 2M
        mMuxerConfig[out_index].stream_info[stream_index].spec.video.lowdelay = 0;//have B picture
    }

    mMuxerConfig[out_index].stream_info[stream_index].spec.video.M = DefaultH264M;
    mMuxerConfig[out_index].stream_info[stream_index].spec.video.N = DefaultH264N;
    mMuxerConfig[out_index].stream_info[stream_index].spec.video.IDRInterval = DefaultH264IDRInterval;
    mMuxerConfig[out_index].stream_info[stream_index].spec.video.entropy_type = IParameters::EntropyType_NOTSet;
    //AMLOG_INFO("****!!!!3333 pConfig->entropy_type %d.\n", mMuxerConfig[out_index].stream_info[stream_index].spec.video.entropy_type);
    return ME_OK;
}

AM_ERR CRecordEngine2::defaultAudioParameters(AM_UINT out_index, AM_UINT stream_index)
{
    //safe check
    AM_ASSERT(mState == STATE_NOT_INITED);
    AM_ASSERT(mbNeedCreateGragh);
    if (!mbNeedCreateGragh || mState != STATE_NOT_INITED) {
        AMLOG_ERROR("record engine's graph has been built yet, cannot defaultAudioParameters now.\n");
        return ME_BAD_STATE;
    }

    AM_ASSERT(out_index < mTotalMuxerNumber);
    if (out_index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::defaultAudioParameters, out_index %d, stream index %d.\n", out_index, stream_index);
        return ME_BAD_PARAM;
    }

    //hard code here, should configed by audio pts's generator
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.pts_unit_num = 1;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.pts_unit_den = IParameters::TimeUnitDen_90khz;

    //if have input from external(rec.config, etc)
    if (out_index < mShared.tot_muxer_number) {
        AMLOG_DEBUG("&&&&& mMuxerConfig[out_index %d].stream_info[stream_index %d].spec.audio.sample_rate %d.\n", out_index, stream_index, mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_rate);

        STargetRecorderConfig* pConfig = &mShared.target_recorder_config[out_index];
        mMuxerConfig[out_index].stream_info[stream_index].stream_type = IParameters::StreamType_Audio;
        mMuxerConfig[out_index].stream_info[stream_index].stream_format = (IParameters::StreamFormat)pConfig->audio_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_layout = 0;//hard code
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_number = pConfig->channel_number;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_rate = pConfig->sample_rate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_format = (IParameters::AudioSampleFMT) pConfig->sample_format;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.frame_size = AUDIO_CHUNK_SIZE;//need set here?
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.bitrate = pConfig->audio_bitrate;
        mMuxerConfig[out_index].stream_info[stream_index].spec.audio.need_skip_adts_header = 0;
        return ME_OK;
    }

    AM_ASSERT(0);
    AMLOG_WARN("&&&&& should not comes here.\n");
    //if have no input from external(rec.config, etc), use some hard coded default value
    mMuxerConfig[out_index].stream_info[stream_index].stream_type = IParameters::StreamType_Audio;
    mMuxerConfig[out_index].stream_info[stream_index].stream_format = IParameters::StreamFormat_AAC;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_layout = 0;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.channel_number = 2;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_rate = 48000;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.sample_format = IParameters::SampleFMT_S16;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.frame_size = AUDIO_CHUNK_SIZE;//need set here?
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.bitrate = 128000;
    mMuxerConfig[out_index].stream_info[stream_index].spec.audio.need_skip_adts_header = 0;

    return ME_OK;
}

AM_ERR CRecordEngine2::AddPrivateDataPacket(AM_U8* data_ptr, AM_UINT len, AM_U16 data_type, AM_U16 sub_type)
{
    if (mpPridataComposer) {
        mpPridataComposer->SetPridata(data_ptr, len, data_type, sub_type);
    }
    return ME_OK;
}

void CRecordEngine2::checkDupexParameters(void)
{
    if (mRequestVoutMask & (1<<eVoutHDMI)) {
        AMLOG_INFO("[vout]: duplex mode request preview on HDMI.\n");
        if (mShared.encoding_mode_config.preview_vout_index!= eVoutHDMI) {
            AMLOG_WARN("[correct parameters]: preview's display on LCD, change it to HDMI.\n");
            mShared.encoding_mode_config.preview_vout_index = eVoutHDMI;
        }

        if (mShared.encoding_mode_config.playback_vout_index != eVoutHDMI) {
            AMLOG_WARN("[correct parameters]: playback's display on LCD, change it to HDMI.\n");
            mShared.encoding_mode_config.playback_vout_index = eVoutHDMI;
        }

        AM_ASSERT(mShared.encoding_mode_config.preview_enabled);
        mShared.encoding_mode_config.preview_enabled = 1;

        AM_ASSERT(!mShared.encoding_mode_config.playback_in_pip);
        mShared.encoding_mode_config.playback_in_pip = 0;

        if (!mShared.encoding_mode_config.preview_in_pip) {
            AMLOG_WARN("[correct parameters]: preview shpuld in PIP on HDMI now.\n");
            mShared.encoding_mode_config.preview_in_pip = 1;
        }

    } else if (mRequestVoutMask & (1<<eVoutLCD)) {
        AMLOG_INFO("[vout]: duplex mode request preview on HDMI.\n");
        if (mShared.encoding_mode_config.preview_vout_index != eVoutLCD) {
            AMLOG_WARN("[correct parameters]: preview's display on HDMI, change it to LCD.\n");
            mShared.encoding_mode_config.preview_vout_index = eVoutLCD;
        }

        if (mShared.encoding_mode_config.playback_vout_index!= eVoutLCD) {
            AMLOG_WARN("[correct parameters]: preview's display on HDMI, change it to LCD.\n");
            mShared.encoding_mode_config.playback_vout_index = eVoutLCD;
        }

        AM_ASSERT(mShared.encoding_mode_config.preview_enabled);
        //mShared.encoding_mode_config.preview_enabled = 1;
        if (1 == mShared.encoding_mode_config.preview_enabled) {
            AMLOG_WARN("[correct parameters]: enable preview on LCD, should disable playback display on LCD.\n");
            mShared.encoding_mode_config.pb_display_enabled = 0;
        }

        AM_ASSERT(!mShared.encoding_mode_config.playback_in_pip);
        mShared.encoding_mode_config.playback_in_pip = 0;

        //hard code to initial vout setting to 480x800, fix me
        AM_ERROR("[vout]: hard code playback display range to full LCD.\n");
        mShared.encoding_mode_config.pb_display_left = 0;
        mShared.encoding_mode_config.pb_display_top = 0;
        mShared.encoding_mode_config.pb_display_width = 480;
        mShared.encoding_mode_config.pb_display_height = 800;

        AM_ERROR("[vout]: hard code preview display range to full LCD.\n");
        mShared.encoding_mode_config.preview_left = 0;
        mShared.encoding_mode_config.preview_top = 0;
        mShared.encoding_mode_config.preview_width = 480;
        mShared.encoding_mode_config.preview_height = 800;

    } else {
        //initial no preview
        AMLOG_WARN("[vout]: duplex mode request no preview.\n");
        mShared.encoding_mode_config.preview_enabled = 0;
    }
}

AM_ERR CRecordEngine2::checkParameters(void)
{
    AM_UINT i = 0, inpin_index = 0;
    AM_UINT have_amr_encoding = 0;
    AM_INT first_audio_muxer_index = -1;
    AM_INT first_audio_stream_index = -1;
    IParameters::StreamFormat audio_format = IParameters::StreamFormat_Invalid;

//#if PLATFORM_ANDROID
    //android need set to 44.1khz input, 48khz input will fail
    //if audio codec is amr, input need change to 8khz, 1 channel, for android's amr encoding library
    //need uniform audio parameters, first audio stream's parameters will take effect
    for (i = 0; i < mTotalMuxerNumber; i++) {
        for (inpin_index = 0; inpin_index < mMuxerConfig[i].stream_number; inpin_index++) {
            if (mMuxerConfig[i].stream_info[inpin_index].stream_type != IParameters::StreamType_Audio) {
                continue;
            }

            //Now amf can't support 8Khz AMR encoding, so using AAC encoder default
            if(mMuxerConfig[i].stream_info[inpin_index].stream_format == IParameters::StreamFormat_AMR_NB ||
                       mMuxerConfig[i].stream_info[inpin_index].stream_format == IParameters::StreamFormat_AMR_WB){
                AMLOG_WARN("audio stream formate hard code to AAC\n");
                mMuxerConfig[i].stream_info[inpin_index].stream_format = IParameters::StreamFormat_AAC;
            }

#if PLATFORM_ANDROID
            if (have_amr_encoding || mMuxerConfig[i].stream_info[inpin_index].stream_format == IParameters::StreamFormat_AMR_NB || mMuxerConfig[i].stream_info[inpin_index].stream_format == IParameters::StreamFormat_AMR_WB) {
                AM_ASSERT(mMuxerConfig[i].stream_info[inpin_index].stream_type == IParameters::StreamType_Audio);
                have_amr_encoding = 1;
                //hard code here
                AMLOG_WARN("android's amr library only support 1channel 8khz encoding, change audio parameters now.\n");
                mMuxerConfig[i].stream_info[inpin_index].stream_format = IParameters::StreamFormat_AMR_NB;
                mMuxerConfig[i].stream_info[inpin_index].spec.audio.sample_rate = 8000;
                mMuxerConfig[i].stream_info[inpin_index].spec.audio.sample_format = IParameters::SampleFMT_S16;
                mMuxerConfig[i].stream_info[inpin_index].spec.audio.channel_number = 1;
                mMuxerConfig[i].stream_info[inpin_index].spec.audio.frame_size = 160;//lib opencore amr hard code?
                mMuxerConfig[i].stream_info[inpin_index].spec.audio.bitrate = 12200;//AMR must use some specified bit-rate
            } else if (mMuxerConfig[i].stream_info[inpin_index].stream_format == IParameters::StreamFormat_AAC) {
                //aac hard code to 44.1khz, not support 8khz
                if(mMuxerConfig[i].stream_info[inpin_index].spec.audio.sample_rate != 44100){
                    AMLOG_WARN("aac set to 44.1khz.\n");
                    mMuxerConfig[i].stream_info[inpin_index].spec.audio.sample_rate = 44100;
                }
                //aac stream's bitrate hard code to 128kbps
                mMuxerConfig[i].stream_info[inpin_index].spec.audio.bitrate = 128000;
            } else if (mMuxerConfig[i].stream_info[inpin_index].spec.audio.sample_rate > 44100) {
                AMLOG_WARN("android's audio input's sample rate restrict to 44100.\n");
                mMuxerConfig[i].stream_info[inpin_index].spec.audio.sample_rate = 44100;
            }
#endif
            break;
        }

        //uniform audio parameters:
        if (first_audio_muxer_index >=0 && (mMuxerConfig[i].stream_info[inpin_index].stream_type == IParameters::StreamType_Audio)) {
            //debug assert
            AM_ASSERT(first_audio_muxer_index == 0);
            AM_ASSERT(first_audio_stream_index >= 0);
            mMuxerConfig[i].stream_info[inpin_index].spec = mMuxerConfig[first_audio_muxer_index].stream_info[first_audio_stream_index].spec;
        } else if (first_audio_muxer_index == -1 && (mMuxerConfig[i].stream_info[inpin_index].stream_type == IParameters::StreamType_Audio)) {
            first_audio_muxer_index = i;
            first_audio_stream_index = inpin_index;
            audio_format = mMuxerConfig[i].stream_info[inpin_index].stream_format;
        }

        AMLOG_WARN("after check, [%d].[%d]'s parameters: codec format %d, channel number %d, sample format %d, sampe rate %d, audio_format %d.\n", i, inpin_index, mMuxerConfig[i].stream_info[inpin_index].stream_format, mMuxerConfig[i].stream_info[inpin_index].spec.audio.channel_number, mMuxerConfig[i].stream_info[inpin_index].spec.audio.sample_format, mMuxerConfig[i].stream_info[inpin_index].spec.audio.sample_rate, audio_format);

    }
//#endif

    //change M,N,IDR Interval when need frame count cutting, main stream's
    AM_INT need_modify_M_N_IDRInterval = 0;
    if (mMuxerConfig[0].mSavingStrategy == IParameters::MuxerSavingFileStrategy_AutoSeparateFile && mMuxerConfig[0].mSavingCondition == IParameters::MuxerSavingCondition_FrameCount && mMuxerConfig[0].mConditionParam < 30) {
        for (i = 0; i < mTotalMuxerNumber; i++) {
            if ((mShared.target_recorder_config[i].N*mShared.target_recorder_config[i].IDRInterval) != mMuxerConfig[0].mConditionParam || 1 != mShared.target_recorder_config[i].M) {
                need_modify_M_N_IDRInterval = 1;
                AMLOG_WARN("framecount %d less than 30, need change M N IDR Interval (%d, %d, %d) to (1, %d, 1).\n", mMuxerConfig[0].mConditionParam, mShared.target_recorder_config[i].M, mShared.target_recorder_config[i].N, mShared.target_recorder_config[i].IDRInterval, mMuxerConfig[0].mConditionParam);
                break;
            }
        }
    } else if (mMuxerConfig[0].mSavingStrategy == IParameters::MuxerSavingFileStrategy_ToTalFile) {
        //enable this feature when recording to total file
        AMLOG_INFO(" enable audio fade in when muxing to total file.\n");
        mShared.enable_fade_in = 1;
    }

    //calculate video framerate
    for (i = 0; i < mTotalMuxerNumber; i++) {
        for (inpin_index = 0; inpin_index < mMuxerConfig[i].stream_number; inpin_index++) {
            if (mMuxerConfig[i].stream_info[inpin_index].stream_type == IParameters::StreamType_Video) {
                mMuxerConfig[i].stream_info[inpin_index].spec.video.framerate = AM_VideoEncCalculateFrameRate(mMuxerConfig[i].stream_info[inpin_index].spec.video.framerate_num, mMuxerConfig[i].stream_info[inpin_index].spec.video.framerate_den);
                if (need_modify_M_N_IDRInterval) {
                    //change M, N, IDR Interval
                    mMuxerConfig[i].stream_info[inpin_index].spec.video.M = 1;
                    mMuxerConfig[i].stream_info[inpin_index].spec.video.N = mMuxerConfig[0].mConditionParam;
                    mMuxerConfig[i].stream_info[inpin_index].spec.video.IDRInterval = 1;
                }
            }
        }
    }

    //check private lib?
    switch (audio_format) {
        //enc lib supported
        case IParameters::StreamFormat_AAC:
            break;
        //enc lib not supported
        case IParameters::StreamFormat_MP2:
        case IParameters::StreamFormat_AC3:
        case IParameters::StreamFormat_ADPCM:
        case IParameters::StreamFormat_AMR_NB:
        case IParameters::StreamFormat_AMR_WB:
        default:
            //private lib not support format
            if (g_GlobalCfg.mUsePrivateAudioEncLib) {
                AMLOG_WARN("current audio enc lib not support format %d, disable it.\n", audio_format);
                g_GlobalCfg.mUsePrivateAudioEncLib = 0;
            }
            break;
    }

    //if enable private data, can use only TS format now
    if (!mbAllPridataDisabled) {
        if (IParameters::MuxerContainer_TS != mMuxerConfig[0].out_format) {
            AMLOG_WARN(" Have Private data request, need change main stream container type to [TS].\n", i);
            mMuxerConfig[0].out_format = IParameters::MuxerContainer_TS;
        }
    }

    //check duplex related parameters
    checkDupexParameters();

    return ME_OK;
}

AM_ERR CRecordEngine2::setOutputFile(AM_UINT index)
{
    IMuxer*pMuxer;

    AM_ASSERT(index < mTotalMuxerNumber);
    if (index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::defaultAudioParameters, out_index %d.\n", index);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(mpMuxerFilters[index]);
    if (!mpMuxerFilters[index]) {
        AMLOG_ERROR("ERROR mpMuxerFilters[%d] NULL.\n", index);
        return ME_BAD_PARAM;
    }

    if ((pMuxer = IMuxer::GetInterfaceFrom(mpMuxerFilters[index])) == NULL) {
        AM_ERROR("No interface\n");
        return ME_ERROR;
    }

    AM_ASSERT(mFilename[index]);

    if (mFilename) {
        pMuxer->SetOutputFile(mFilename[index]);
    } else {
        if (index == 0)
            pMuxer->SetOutputFile("no_file_name0");
        else
            pMuxer->SetOutputFile("no_file_name1");
    }

    return ME_OK;
}

AM_ERR CRecordEngine2::setThumbNailFile(AM_UINT index)
{
     IMuxer*pMuxer;

    AM_ASSERT(index < mTotalMuxerNumber);
    AM_ASSERT(index == 0);//only main muxer need to generate thumbnail
    if (index >= mTotalMuxerNumber) {
        AMLOG_ERROR("BAD parameters in CRecordEngine2::defaultAudioParameters, out_index %d.\n", index);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(mpMuxerFilters[index]);
    if (!mpMuxerFilters[index]) {
        AMLOG_ERROR("ERROR mpMuxerFilters[%d] NULL.\n", index);
        return ME_BAD_PARAM;
    }

    if ((pMuxer = IMuxer::GetInterfaceFrom(mpMuxerFilters[index])) == NULL) {
        AM_ERROR("No interface\n");
        return ME_ERROR;
    }

    AM_ASSERT(mpThumbNailFilename);
    pMuxer->SetThumbNailFile(mpThumbNailFilename);
    return ME_OK;
}

void CRecordEngine2::setupStreamingServerManger()
{
    AM_ASSERT(!mpStreammingServerManager);
    if (mpStreammingServerManager) {
        return;
    }
    mpStreammingServerManager = CreateStreamingServerManager(&mShared);
    if (!mpStreammingServerManager) {
        AM_ERROR("Create CStreammingServerManager fail.\n");
        return;
    }

    mpStreammingServerManager->StartServerManager();
}

AM_ERR CRecordEngine2::AddStreamingServer(AM_UINT& server_index, IParameters::StreammingServerType type, IParameters::StreammingServerMode mode)
{
    AUTO_LOCK(mpMutex);
    if (!mShared.streaming_enable) {
        AMLOG_INFO(" CRecordEngine2::AddStreamingServer, streaming not enabled, return ME_OK.\n");
        return ME_OK;
    }

    if (!mpStreammingServerManager) {
        setupStreamingServerManger();
        if (!mpStreammingServerManager) {
            return ME_ERROR;
        }
    }

    //find if the server is exist
    CDoubleLinkedList::SNode* pnode;
    SStreammingServerInfo* p_server;

    pnode = mStreammingServerList.FirstNode();
    while (pnode) {
        p_server = (SStreammingServerInfo*) pnode->p_context;
        if ((p_server->type == type) && (p_server->mode == mode)) {
            AMLOG_WARN("server exist(type %d, mode %d).\n", type, mode);
            server_index = p_server->index;
            return ME_OK;
        }
        pnode = mStreammingServerList.NextNode(pnode);
    }

    //add new server
    p_server = mpStreammingServerManager->AddServer(type, mode, mShared.rtsp_server_config.rtsp_listen_port, mShared.streaming_video_enable, mShared.streaming_audio_enable);

    AM_ASSERT(p_server);
    if (p_server) {
        //choose a not used server number
        while (findStreamingServerByIndex(mServerMagicNumber)) {
            mServerMagicNumber ++;
        }
        p_server->index = mServerMagicNumber;
        mStreammingServerList.InsertContent(NULL, (void*)p_server, 0);
        return ME_OK;
    }

    return ME_ERROR;
}

AM_ERR CRecordEngine2::GetStreamingPortUrl(AM_UINT server_index, AM_UINT &server_port, AM_S8 *url, AM_UINT url_size){
    server_port = mShared.rtsp_server_config.rtsp_listen_port;
    snprintf((char*)url,url_size,"%s","stream_0");//hard code,todo
    return ME_OK;
}

AM_ERR CRecordEngine2::RemoveStreamingServer(AM_UINT server_index)
{
    AUTO_LOCK(mpMutex);
    if (!mShared.streaming_enable) {
        AMLOG_INFO(" CRecordEngine2::RemoveStreamingServer, streaming not enabled, return ME_OK.\n");
        return ME_OK;
    }

    if (!mpStreammingServerManager) {
        AM_ERROR("NULL mpStreammingServerManager, but someone invoke CRecordEngine2::RemoveStreamingServer?\n");
        return ME_ERROR;
    }

    //find if the server is exist
    CDoubleLinkedList::SNode* pnode;
    SStreammingServerInfo* p_server;

    pnode = mStreammingServerList.FirstNode();
    while (pnode) {
        p_server = (SStreammingServerInfo*) pnode->p_context;
        if (p_server->index == server_index) {
            AMLOG_INFO("remove server (ndex %d, type %d, mode %d).\n", p_server->index, p_server->type, p_server->mode);
            mStreammingServerList.RemoveContent((void*)p_server);
            mpStreammingServerManager->RemoveServer(p_server);
            return ME_OK;
        }
        pnode = mStreammingServerList.NextNode(pnode);
    }

    //cannot find the server
    AMLOG_ERROR("NO Server's index is %d.\n", server_index);
    return ME_ERROR;
}

SStreamContext* CRecordEngine2::findStreamingContentByIndex(AM_UINT output_index)
{
    AM_UINT i = 0;
    //find if the server is exist
    CDoubleLinkedList::SNode* pnode;
    SStreamContext* p_content;

    pnode = mStreammingServerList.FirstNode();
    while (pnode) {
        p_content = (SStreamContext*) pnode->p_context;
        if (p_content->index == output_index) {
            AMLOG_INFO("find streaming content in list(index %d).\n", output_index);
            return p_content;
        }
        pnode = mStreammingServerList.NextNode(pnode);
    }

    AMLOG_INFO("cannot find streaming content in list(index %d).\n", output_index);
    return NULL;
}

SStreammingServerInfo* CRecordEngine2::findStreamingServerByIndex(AM_UINT server_index)
{
    AM_UINT i = 0;
    //find if the server is exist
    CDoubleLinkedList::SNode* pnode;
    SStreammingServerInfo* p_server;

    pnode = mStreammingServerList.FirstNode();
    while (pnode) {
        p_server = (SStreammingServerInfo*) pnode->p_context;
        if (p_server->index == server_index) {
            AMLOG_INFO("find streaming content in list(index %d).\n", server_index);
            return p_server;
        }
        pnode = mStreammingServerList.NextNode(pnode);
    }

    AMLOG_INFO("cannot find streaming content in list(index %d).\n", server_index);
    return NULL;
}

SStreamContext* CRecordEngine2::generateStreamingContent(AM_UINT output_index)
{
    AM_ASSERT(output_index < mTotalMuxerNumber);
    if (output_index < mTotalMuxerNumber) {
        SStreamContext* p_content = (SStreamContext*)malloc(sizeof(SStreamContext));
        AM_UINT stream_index = 0;
        if (p_content) {
            memset(p_content, 0, sizeof(SStreamContext));
            p_content->index = output_index;
            p_content->p_data_transmeter = mpStreamingDataTransmiter[output_index];

            //content
            snprintf(p_content->content.mUrl, DMaxStreamContentUrlLength, "stream_%d", output_index);
            p_content->content.mType = IParameters::StreammingContentType_Live;
            AMLOG_INFO("{{{engine}}}, p_content->content.mUrl %p, %s, this %p, mpStreamingDataTransmiter[output_index] %p, %p.\n", p_content->content.mUrl, p_content->content.mUrl, p_content, mpStreamingDataTransmiter[output_index], p_content->p_data_transmeter);
            //video
            stream_index = 0;
            findNextStreamIndex(output_index, stream_index, IParameters::StreamType_Video);//hard code here, use find first video stream's parameters
            AM_ASSERT(IParameters::StreamType_Video == mMuxerConfig[output_index].stream_info[stream_index].stream_type);
            AM_ASSERT(IParameters::StreamFormat_H264 == mMuxerConfig[output_index].stream_info[stream_index].stream_format);
            if (IParameters::StreamType_Video == mMuxerConfig[output_index].stream_info[stream_index].stream_type) {
                p_content->content.video_format = mMuxerConfig[output_index].stream_info[stream_index].stream_format;
                p_content->content.video_bit_rate = mMuxerConfig[output_index].stream_info[stream_index].spec.video.bitrate;
                p_content->content.video_framerate = mMuxerConfig[output_index].stream_info[stream_index].spec.video.framerate;
                p_content->content.video_width = mMuxerConfig[output_index].stream_info[stream_index].spec.video.pic_width;
                p_content->content.video_height = mMuxerConfig[output_index].stream_info[stream_index].spec.video.pic_height;
            } else {
                AM_ASSERT("BAD request for video, no video for this case, please check.\n");
            }
            //audio
            stream_index = 0;//hard code here, video index == 0
            findNextStreamIndex(output_index, stream_index, IParameters::StreamType_Audio);//hard code here, use find first audio stream's parameters
            AM_ASSERT(IParameters::StreamType_Audio == mMuxerConfig[output_index].stream_info[stream_index].stream_type);
            AM_ASSERT(IParameters::StreamFormat_AAC == mMuxerConfig[output_index].stream_info[stream_index].stream_format);
            if (IParameters::StreamType_Audio == mMuxerConfig[output_index].stream_info[stream_index].stream_type) {
                p_content->content.audio_format = mMuxerConfig[output_index].stream_info[stream_index].stream_format;
                p_content->content.audio_bit_rate = mMuxerConfig[output_index].stream_info[stream_index].spec.audio.bitrate;
                p_content->content.audio_channel_number = mMuxerConfig[output_index].stream_info[stream_index].spec.audio.channel_number;
                p_content->content.audio_sample_rate = mMuxerConfig[output_index].stream_info[stream_index].spec.audio.sample_rate;
                p_content->content.audio_sample_format = mMuxerConfig[output_index].stream_info[stream_index].spec.audio.sample_format;
            } else {
                AM_ASSERT("BAD request for video, no video for this case, please check.\n");
            }
        }
        return p_content;
    }else {
        AM_ERROR("BAD outputindex %d, mTotalMuxerNumber %d.\n", output_index, mTotalMuxerNumber);
        return NULL;
    }
}

AM_ERR CRecordEngine2::EnableStreamming(AM_UINT server_index, AM_UINT output_index, AM_UINT streaming_index)
{
    //todo
    AUTO_LOCK(mpMutex);
    SStreamContext* p_content;
    SStreammingServerInfo* p_server;

    if (!mShared.streaming_enable) {
        AMLOG_INFO(" CRecordEngine2::EnableStreamming, streaming not enabled, return ME_OK.\n");
        return ME_OK;
    }

    if (output_index >= DMaxMuxerNumber) {
        AM_ERROR("CRecordEngine2::EnableStreamming BAD index %d.\n", output_index);
        return ME_BAD_PARAM;
    }

    mMuxerConfig[output_index].video_streamming = 1;
    mMuxerConfig[output_index].audio_streamming = 1;

    //not on the fly
    if (STATE_NOT_INITED == mState || STATE_STOPPED == mState) {

    } else if (STATE_RECORDING == mState) {
        p_content = findStreamingContentByIndex(output_index);
        if (!p_content) {
            p_content = generateStreamingContent(output_index);
            AM_ASSERT(p_content);
            mStreamContextList.InsertContent(NULL, p_content, 0);
        }

        p_server = findStreamingServerByIndex(server_index);
        if (!p_server) {
            //add new server
            p_server = mpStreammingServerManager->AddServer(IParameters::StreammingServerType_RTSP, IParameters::StreammingServerMode_MulticastSetAddr, DefaultRTSPServerPort, mShared.streaming_video_enable, mShared.streaming_audio_enable);//hard code here

            AM_ASSERT(p_server);
            if (p_server) {
                //choose a not used server number
                while (findStreamingServerByIndex(mServerMagicNumber)) {
                    mServerMagicNumber ++;
                }
                p_server->index = mServerMagicNumber;
                mStreammingServerList.InsertContent(NULL, (void*)p_server, 0);
                return ME_OK;
            }

        }
    } else {
        //no need do this here
        AM_ASSERT(0);
    }
    return ME_OK;
}

AM_ERR CRecordEngine2::DisableStreamming(AM_UINT server_index, AM_UINT output_index, AM_UINT streaming_index)
{
    //todo
    AUTO_LOCK(mpMutex);
    if (!mShared.streaming_enable) {
        AMLOG_INFO(" CRecordEngine2::DisableStreamming, streaming not enabled, return ME_OK.\n");
        return ME_OK;
    }

    if (output_index >= DMaxMuxerNumber) {
        AM_ERROR("CRecordEngine2::DisableStreamming BAD index %d.\n", output_index);
        return ME_BAD_PARAM;
    }

    mMuxerConfig[output_index].video_streamming = 0;
    mMuxerConfig[output_index].audio_streamming = 0;
    //todo
    return ME_OK;
}

AM_INT CRecordEngine2::AddOsdBlendArea(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->AddOsdBlendArea(xPos, yPos, width, height);
    }
    return ME_BAD_STATE;
}

AM_ERR CRecordEngine2::UpdateOsdBlendArea(AM_INT index, AM_U8* data, AM_INT width, AM_INT height)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->UpdateOsdBlendArea(index, data, width, height);
    }
    return ME_BAD_STATE;
}

AM_ERR CRecordEngine2::RemoveOsdBlendArea(AM_INT index)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->RemoveOsdBlendArea(index);
    }
    return ME_BAD_STATE;
}

AM_ERR CRecordEngine2::UpdatePreviewDisplay(AM_UINT width, AM_UINT height, AM_UINT pos_x, AM_UINT pos_y, AM_UINT alpha)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->UpdatePreviewDisplay(width, height, pos_x, pos_y, alpha);
    }
    return ME_NO_IMPL;
}

AM_INT CRecordEngine2::AddOsdBlendAreaCLUT(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->AddOsdBlendAreaCLUT(xPos, yPos, width, height);
    }
    return (-1);
}

AM_ERR CRecordEngine2::UpdateOsdBlendAreaCLUT(AM_INT index, AM_U8* data, AM_U32* p_input_clut, AM_INT width, AM_INT height, IParameters::OSDInputDataFormat data_format)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->UpdateOsdBlendAreaCLUT(index, data, p_input_clut, width, height, data_format);
    }
    return ME_BAD_STATE;
}

AM_ERR CRecordEngine2::RemoveOsdBlendAreaCLUT(AM_INT index)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->RemoveOsdBlendAreaCLUT(index);
    }
    return ME_BAD_STATE;
}

AM_ERR CRecordEngine2::FreezeResumeHDMIPreview(AM_INT flag)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->FreezeResumeHDMIPreview(flag);
    }
    return ME_BAD_STATE;
}

AM_ERR CRecordEngine2::SetDisplayMode(AM_UINT display_preview, AM_UINT display_playback, AM_UINT preview_vout, AM_UINT pb_vout)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->SetDisplayMode(display_preview, display_playback, preview_vout, pb_vout);
    }
    return ME_NO_IMPL;
}

AM_ERR CRecordEngine2::GetDisplayMode(AM_UINT& display_preview, AM_UINT& display_playback, AM_UINT& preview_vout, AM_UINT& pb_vout)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->GetDisplayMode(display_preview, display_playback, preview_vout, pb_vout);
    }
    return ME_NO_IMPL;
}

AM_ERR CRecordEngine2::DemandIDR(AM_UINT out_index)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->DemandIDR(out_index);
    }
    return ME_NO_IMPL;
}

AM_ERR CRecordEngine2::UpdateGOPStructure(AM_UINT out_index, int M, int N, int idr_interval)
{
    AM_ASSERT(mpVideoEncoder);
    if(mpVideoEncoder){
        return mpVideoEncoder->UpdateGOPStructure(out_index, M, N, idr_interval);
    }
    return ME_NO_IMPL;
}

AM_ERR CRecordEngine2::SetProperty(AM_UINT prop, AM_UINT value)
{
    switch (prop) {
        case REC_PROPERTY_NOT_START_ENCODING:
            AMLOG_INFO("[debug]: request not start encoding(%d).\n", value);
            mShared.not_start_encoding = value;
            break;
        case REC_PROPERTY_CAPTURE_JPEG:
            AMLOG_INFO("[debug]: request capture jpeg, size(%d*%d).\n", value>>16, value&0xffff);
            if (mpVideoEncoder) {
                mpVideoEncoder->CaptureJpeg("test0.jpeg", value>>16, value&0xffff);
            }
            break;
        case REC_PROPERTY_SELECT_CAMERA:
            AMLOG_INFO("[debug]: select camera, id %d.\n", value);
            mShared.select_camera_index = value & 0xff;
            break;
        case REC_PROPERTY_STREAM_ONLY_AUDIO:
            AMLOG_INFO("[debug]: ony audio.\n");
            mbOnlyEncAudio = true;
            mShared.streaming_video_enable = 0;
            mbOnlyEncVideo = !mbOnlyEncAudio;
            break;
        case REC_PROPERTY_STREAM_ONLY_VIDEO:
            AMLOG_INFO("[debug]: ony video.\n");
            mbOnlyEncVideo = true;
            mShared.streaming_audio_enable = 0;
            mbOnlyEncAudio = !mbOnlyEncVideo;
            break;
        default:
            AM_ERROR("BAD property %d.\n", prop);
            return ME_NOT_SUPPORTED;
    }
    return ME_OK;
}

AM_ERR CRecordEngine2::CaptureYUV(char* name, SYUVData* yuv)
{
    AM_ERR err;
    int fd;
    AM_UINT pitch = 0;
    AM_UINT height = 0;

    if(name == NULL){
        fd = 0;
        err = mpVideoEncoder->CaptureYUVdata(fd, pitch, height, yuv);
        if(ME_OK != err){
            AM_ERROR("mpVideoEncoder->CaptureRawData() fail, ret %d.\n", err);
        }else{
            AMLOG_WARN("CaptureYUVdata, %u*%u.\n", pitch, height);
        }
        return err;
    }

    fd = open(name, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if(fd<=0){
        AM_ERROR("can NOT create file %s!\n", name);
        return ME_ERROR;
    }

    //capture raw data
    err = mpVideoEncoder->CaptureYUVdata(fd, pitch, height);
    if (ME_OK != err) {
        AM_ERROR("mpVideoEncoder->CaptureRawData() fail, ret %d.\n", err);
    }else{
        AMLOG_WARN("CaptureYUVdata, %u*%u.\n", pitch, height);
    }

    close(fd);
    AMLOG_INFO("CaptureYUVdata done, %u*%u.\n", pitch, height);
    return err;
}

AM_ERR CRecordEngine2::CaptureJPEG(const char* name)
{
    AM_ERR err = ME_ERROR;
    if (mState != STATE_RECORDING) {
        AM_ERROR("CaptureJPEG: error state: %d\n", mState);
        return err;
    }

    AM_UINT width = mMuxerConfig[0].stream_info[0].spec.video.pic_width;
    AM_UINT height = mMuxerConfig[0].stream_info[0].spec.video.pic_height;
    AM_UINT iNailRawBufferSize = width * height * 4 + 32;
    AM_U8* pNailRawBuffer = (AM_U8*)malloc(iNailRawBufferSize);

    AMLOG_WARN("CaptureJPEG IN: size %d x %d\n", width, height);
    if (pNailRawBuffer && mpVideoEncoder) {
        AM_U8* src[3];
        AM_U8* des[3];
        AM_INT src_stride[3];
        AM_INT des_stride[3];

        //capture raw data
        err = mpVideoEncoder->CaptureRawData(pNailRawBuffer, width, height, IParameters::PixFormat_NV12);
        if (ME_OK != err) {
            AM_ERROR("CaptureJPEG: mpVideoEncoder->CaptureRawData() fail, ret %d.\n", err);
            return err;
        }
        //convert to yuv420p
        src[0] = des[0] = pNailRawBuffer;
        src[1] = pNailRawBuffer + width*height;
        src_stride[0] = des_stride[0] = width;
        src_stride[1] = width;

        des[1] = pNailRawBuffer + (width*height)*3/2;
        des[2] = pNailRawBuffer + (width*height)*7/4;
        des_stride[1] = des_stride[2] = width/2;
        AMLOG_INFO("before convert pix format.\n");
        AM_ConvertPixFormat(IParameters::PixFormat_NV12, IParameters::PixFormat_YUV420P, src, des, src_stride, des_stride, width, height);

        AMLOG_INFO("before FF_GenerateJpegFile, filename %s.\n", name);
        //encoded to jpeg file
        err = FF_GenerateJpegFile((char*)name, des, width, height, IParameters::PixFormat_YUV420P, 0, pNailRawBuffer + (width*height*2), width*height*2);
        AM_ASSERT(ME_OK == err);
        free(pNailRawBuffer);
    }
    AMLOG_WARN("CaptureJPEG done\n");

    return err;
}