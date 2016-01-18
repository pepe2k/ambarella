

/**
 * active_veengine.cpp
 *
 * History:
 *    2011/08/15- [GangLiu] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "active_ve_engine"
//#define AMDROID_DEBUG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_queue.h"
#include "am_if.h"
#include "am_ve_if.h"
#include "am_mw.h"
#include "am_base.h"
#include "am_util.h"
#include "pbif.h"
#include "ve_if.h"
#include "record_if.h"
#include "active_ve_engine.h"
#include "filter_list.h"
#include "filter_registry.h"
#include "am_dsp_if.h"
#include "msgsys.h"

IVEControl* CreateActiveVEControl(void *audiosink)
{
    return (IVEControl*)CActiveVEEngine::Create(audiosink);
}

//-----------------------------------------------------------------------
//
// CActiveVEEngine
//
//-----------------------------------------------------------------------
CActiveVEEngine* CActiveVEEngine::Create(void *audiosink)
{
    CActiveVEEngine *result = new CActiveVEEngine(audiosink);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CActiveVEEngine::CActiveVEEngine(void *audiosink):
    inherited("active_ve_engine"),
    mbRun(true),
    mbPaused(false),
    mFatalErrorCnt(0),
    mTrySeekTimeGap(2000),
    mState(STATE_IDLE),
    mpVoutController(NULL),
    mpClockManager(NULL),
    mpAudioSink(audiosink),//no audio now
    mpMasterFilter(NULL),
    mMostPriority(0),
    mSeekTime(0),
    mTimeShaftLength(0),
    mTimeEncoded(0),
    mEncWidth(720),
    mEncHeight(480),
    mbEncKeepAspectRatio(true),
    mBitrate(2000000),
    mFpsBase(0),
    mbhasBfrm(false),
    mbRenderHDMI(true),
    mbRenderLCD(false),
    mbPreviewEnc(true),
    mbCtrlFps(true),
    mSavingStrategy(2),
    mbUseAbsoluteTime(true),
    mpMutex(NULL),
    mpCondStopfilters(NULL)
{
    AM_UINT i = 0;
    memset((void*)&mShared, 0, sizeof(mShared));

    memset(mpRecognizer, 0, sizeof(mpRecognizer));
    memset(mpFilterDemuxer, 0, sizeof(mpFilterDemuxer));
    memset(mpFilterVideoDecoder, 0, sizeof(mpFilterVideoDecoder));
    memset(mpFilterVideoRenderer, 0, sizeof(mpFilterVideoRenderer));
    memset(mpDemuxer, 0, sizeof(mpDemuxer));
    mpVideoEffecterControl = NULL;
    mpVideoTranscoder = NULL;
    mpVideoMemEncoder = NULL;
    mpMasterFilter = NULL;
    mpMuxerControl = NULL;
    mpFilterVideoEffecter = NULL;
    mpFilterVideoTranscoder = NULL;
    mpFilterVideoMemEncoder = NULL;
    mpFilterMuxer = NULL;
    //memset(&outFileName[0], 0,  256);
    char filenamebase[256] = "/dev/demo/files/transc_demo";
    strcpy(outFileName, filenamebase);

    AMLOG_INFO("mpDemuxer[1]=%p:\n",mpDemuxer[1]);
    //debug
    mShared.mEngineVersion = 1;
    mShared.mbIavInited = 0;
    mShared.mIavFd = -1;//init value

    AM_DefaultPBConfig(&mShared);
    AM_DefaultPBDspConfig(&mShared);

//read setting from pb.config
    char configfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};
    snprintf(configfilename, sizeof(configfilename), "%s/pb.config", AM_GetPath(AM_PathConfig));
    i = AM_LoadPBConfigFile(configfilename, &mShared);
    snprintf(configfilename, sizeof(configfilename), "%s/log.config", AM_GetPath(AM_PathConfig));
    AM_LoadLogConfigFile(configfilename);

    //read fail, for safe, set default value again
    if (i==0) {
        AM_DefaultPBConfig(&mShared);
        AM_DefaultPBDspConfig(&mShared);
    }

    AMLOG_INFO("Load config file, configNumber=%d:\n", i);

    pthread_mutex_init(&mShared.mMutex, NULL);

    //print current settings
    AMLOG_INFO("      disable audio? %d, disable video? %d, disable subtitle? %d, vout select %x, ppmod select 0x%x.\n", mShared.pbConfig.audio_disable, mShared.pbConfig.video_disable, mShared.pbConfig.subtitle_disable, mShared.pbConfig.vout_config, mShared.ppmode);
    for (i=0; i< CODEC_NUM; i++) {
        AMLOG_INFO("      video format %d, select dec type %d.\n", mShared.decSet[i].codecId, mShared.decSet[i].dectype);
    }
    AM_PrintLogConfig();
    AM_PrintPBDspConfig(&mShared);

    mpOpaque = (void*)&mShared;

    //deinterlace config
    mShared.bEnableDeinterlace = AM_FALSE;
}

AM_ERR CActiveVEEngine::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    if ((mpMutex = CMutex::Create(false)) == NULL)
        return ME_OS_ERROR;

    if ((mpCondStopfilters = CCondition::Create()) == NULL)
        return ME_OS_ERROR;

    mpMsgSys = CMsgSys::Init();
    if(mpMsgSys == NULL)
        return ME_NO_MEMORY;

    DSetModuleLogConfig(LogModuleVEEngine);

    if ((mpClockManager = CClockManager::Create()) == NULL)
        return ME_ERROR;
    AM_ASSERT(mpWorkQ);

    mpWorkQ->Run();
    return ME_OK;
}

CActiveVEEngine::~CActiveVEEngine()
{
    AM_INFO(" ====== Destroy ActiveVEEngine ====== \n");

    if (mpClockManager) {
        mpClockManager->StopClock();
        mpClockManager->SetSource(NULL);
    }
    __LOCK(mpMutex);

    __UNLOCK(mpMutex);
    for(int i=0; i<MAX_VIDEO_NUMS;i++)
       AM_DELETE(mpRecognizer[i]);
    AM_DELETE(mpClockManager);
    mpOpaque = NULL;
    pthread_mutex_destroy(&mShared.mMutex);
    AM_INFO(" ====== Destroy ActiveVEEngine done ====== \n");
}

void CActiveVEEngine::Delete()
{
    if (mpWorkQ) {
        mpWorkQ->SendCmd(CMD_STOP);
    }
    if (mpClockManager) {
        mpClockManager->StopClock();
        mpClockManager->SetSource(NULL);
    }

    for(int i=0; i<MAX_VIDEO_NUMS;i++){
        if(!mpRecognizer[i])
            continue;
        AM_INFO(" ====== AM_DELETE(mpRecognizer[%d]); ====== \n", i);
        AM_DELETE(mpRecognizer[i]);
        mpRecognizer[i] = NULL;
    }
    AM_DELETE(mpClockManager);
    mpClockManager = NULL;

    AM_DELETE(mpMsgSys);
    mpMsgSys = NULL;

    inherited::Delete();
}

void *CActiveVEEngine::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IVEControl)
        return (IVEControl*)this;
    if (refiid == IID_IVEEngine)
        return (IVEEngine*)this;
    if (refiid == IID_IMSGSYS)
        return mpMsgSys;
    return inherited::GetInterface(refiid);
}

void *CActiveVEEngine::QueryEngineInterface(AM_REFIID refiid)
{
    if (refiid == IID_IClockManager || refiid == IID_IClockManagerExt)
        return mpClockManager;

    if (refiid == IID_IAudioSink)
        return mpAudioSink;

    return NULL;
}

AM_ERR CActiveVEEngine::CreateRecognizer(AM_INT index)
{
    if (mpRecognizer[index])
        return ME_OK;

    if ((mpRecognizer[index] = CMediaRecognizer::Create((IEngine*)(inherited*)this)) == NULL)
        return ME_NO_MEMORY;

    return ME_OK;
}

AM_ERR CActiveVEEngine::CreateProject()
{
    return ME_OK;
}
AM_ERR CActiveVEEngine::OpenProject(const char *pProjectName)
{
    return ME_OK;
}
AM_ERR CActiveVEEngine::SaveProject()
{
    return ME_OK;
}

AM_ERR CActiveVEEngine::Stop()
{
    AM_ASSERT(mpCmdQueue);
    CMD cmd;
    cmd.code = VECMD_STOP_PREVIEW;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return ME_OK;
}

AM_ERR CActiveVEEngine::StopTransc(AM_INT stopcode)
{
    AUTO_LOCK(mpMutex);
    if((mState != STATE_STOPPING) && (mState != STATE_FINALIZE) && (mState != STATE_READY)){
        AMLOG_INFO("CActiveVEEngine is NOT running: state = %d.\n ", mState);
        return ME_OK;
    }

    AM_ASSERT(mpCmdQueue);
    CMD cmd;

    cmd.code = VECMD_STOP_FINALIZE;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    cmd.res32_1 = stopcode;
    mpCmdQueue->SendMsg(&cmd, sizeof(cmd));

    AMLOG_INFO("CActiveVEEngine Wait Cond...\n ");
    while (mState != STATE_STOPPED) {
        mpCondStopfilters->Wait(mpMutex);
    }

    return ME_OK;
}

AM_ERR CActiveVEEngine::AddSource(const char *pFileName, VE_TYPE type, AM_UINT &sourceID)
{
    AM_ERR err;
    AM_INT index;
    CMediaRecognizer* pRecognizer;
    if(!mpRecognizer[0]){
        err = CreateRecognizer(0);
        pRecognizer = mpRecognizer[0];
        index = 0;
        AMLOG_INFO("CreateRecognizer 0 done.\n");
    }else if(!mpRecognizer[1]){
        err = CreateRecognizer(1);
        pRecognizer = mpRecognizer[1];
        index = 1;
        AMLOG_INFO("CreateRecognizer 1 done.\n");
    }else{
        index = -1;
        return ME_BUSY;
    }
    if (err != ME_OK){
        AMLOG_ERROR("CreateRecognizer fail, ret %d.\n", err);
        return err;
    }

    err = pRecognizer->RecognizeFile(pFileName, true);
    if (err != ME_OK) {
        AMLOG_ERROR("RecognizeFile %s fail, ret %d.\n", pFileName, err);
        return err;
    }

    AMLOG_INFO("index = %d, %p.\n", index, pRecognizer->GetParser().context);
//    pRecognizer->ClearParser();

    mState = STATE_PREPARED;

    return ME_OK;
}

AM_ERR CActiveVEEngine::DelSource(AM_UINT sourceID)
{
    return ME_OK;
}
AM_ERR CActiveVEEngine::SeekTo(AM_U64 time)
{
    return ME_OK;
}

AM_ERR CActiveVEEngine::GetVEInfo(VE_INFO& info)
{
    info.state = mState;
    info.SelectedSourceID = mCurrentItem;
    info.CurTime = mCurTime;
    info.TimeShaftLength = mTimeShaftLength;

//    AMLOG_DEBUG("GetVEInfo CurTime = %llu, TimeShaftLength= %llu, SelectedSourceID=%u, mState %d.\n",
//        info.CurTime, info.TimeShaftLength, info.SelectedSourceID, mState);
    return ME_OK;
}

//operate of selected item
AM_ERR CActiveVEEngine::SelectID(AM_UINT sourceID)
{
    return ME_OK;
}
AM_ERR CActiveVEEngine::GetSourceInfo(VE_SRC_INFO& src_info)
{//todo
    src_info.ID = 0;
    src_info.type = Type_None;
    src_info.StartTime = 0;
    src_info.duration = 0;
    return ME_OK;
}
AM_ERR CActiveVEEngine::SetStartTime(AM_U64 time)
{
//    VE_SRC_INFO* srcinfo;
    //get current source's info here
//    srcinfo->StartTime = time;
    return ME_OK;
}

AM_ERR CActiveVEEngine::SplitSource(AM_U64 from, AM_U64 to, VE_TYPE src_type)
{
    return ME_OK;
}

//preview
AM_ERR CActiveVEEngine::AddEffect(VE_Effect_t* effect)
{
    return ME_OK;
}

AM_ERR CActiveVEEngine::StartPreview(AM_INT mode)
{
    AM_ERR err;
    AUTO_LOCK(mpMutex);

    if (mState == STATE_ERROR) {
        return ME_BAD_STATE;
    }
    if(mode == 0)//test 2 udec
        err = BuildTest2UdecFilterGragh(1);//transcode mode
    else if(mode == 2)//test 2 udec
        err = BuildTest2UdecFilterGragh(2);//udec mode
    else if(mode == 3)
        err = BuildTranscodeFilterGragh();
    else if(mode == 1)
        err = BuildPreviewFilterGragh();
    else
        return ME_ERROR;
    if (err != ME_OK) {
        AM_ERROR("CreateGraph failed, now clearing...\n");
        ClearGraph();
        return err;
    }

    err = RunAllFilters();
    if (err != ME_OK) {
        StopAllFilters();
        PurgeAllFilters();
        return err;
    }
    if(mode == 3){
        mState = STATE_FINALIZE;
        mpClockManager->StartClock();
    }else
        mState = STATE_PREVIEW_RUN;

    return ME_OK;
}


AM_ERR CActiveVEEngine::StartTranscode()
{
    AM_ERR err;
    AUTO_LOCK(mpMutex);

    if (mState == STATE_ERROR) {
        return ME_BAD_STATE;
    }

    mState = STATE_READY;

    err = BuildTranscodeFilterGragh();

    if (err != ME_OK) {
        AM_ERROR("CreateGraph failed, now clearing...\n");
        ClearGraph();
        return err;
    }

    err = RunAllFilters();
    if (err != ME_OK) {
        StopAllFilters();
        PurgeAllFilters();
        return err;
    }

    mState = STATE_FINALIZE;
    mpClockManager->StartClock();

    return ME_OK;
}

AM_ERR CActiveVEEngine::ResumePreview()
{
    return ME_OK;
}
AM_ERR CActiveVEEngine::PausePreview()
{
    return ME_OK;
}
AM_ERR CActiveVEEngine::StopPreview()
{
    StopAllFilters();
    PurgeAllFilters();
    //SaveDataBase();
    return ME_OK;
}

AM_ERR CActiveVEEngine::StopDecoder(AM_INT index)
{
    AMLOG_INFO("\n=== StopDecoder branch [%d]\n", index);
    IFilter::INFO info;
    for (AM_UINT i = 0; i < mnFilters; i++) {
        IFilter *pFilter = mFilters[i].pFilter;
        pFilter->GetInfo(info);
        if(info.mIndex == index){
            AMLOG_INFO("Stop %s (%d)\n", GetFilterName(pFilter), i+1);
            pFilter->Flush();
            PurgeFilter(pFilter);
//            if (info.mFlags & DEC_FLAG) {
//                pFilter->SendCmd(CMD_RELEASE_DECODER);
//                AMLOG_INFO("Stop %s (%d), CMD Release.\n", GetFilterName(pFilter), i+1);
//            }
        }
    }
    StopUDEC(mShared.mIavFd, index, STOPFLAG_STOP);

    AMLOG_INFO("StopDecoder [%d] OK\n", index);
    return ME_OK;
}

AM_ERR CActiveVEEngine::SetEncParamaters(AM_UINT en_w, AM_UINT en_h, bool keepAspectRatio, AM_UINT fpsBase, AM_UINT bitrate, bool hasBfrm)
{
    mShared.sTranscConfig.keepAspectRatio = mbEncKeepAspectRatio = keepAspectRatio;
    mShared.sTranscConfig.enc_w = mEncWidth = en_w;
    mShared.sTranscConfig.enc_h = mEncHeight = en_h;
    mFpsBase = (0 == fpsBase)?mShared.mVideoTicks:fpsBase;
    mBitrate = (0 == bitrate)?mBitrate:bitrate;
    mbhasBfrm = hasBfrm;
    AMLOG_INFO("CActiveVEEngine::SetEncParamaters: \n\tEnc size = %u*%u, %s keep aupectRatio, fpsBase: %u, bitrate: %u, %s B frames.\n",
        mEncWidth, mEncHeight, mbEncKeepAspectRatio?"do":"not", mFpsBase, mBitrate, mbhasBfrm?"has":"no");
    return  ME_OK;
}


AM_ERR CActiveVEEngine::SetTranscSettings(bool RenderHDMI, bool RenderLCD, bool PreviewEnc, bool ctrlfps)
{
    mbRenderHDMI = RenderHDMI;
    mbPreviewEnc = PreviewEnc;
    if(mbPreviewEnc){
        if(RenderLCD) AMLOG_WARN("RenderLCD and PreviewEnc can not be enabled at the same time, disable render to LCD.");
    }else{
        mbRenderLCD = RenderLCD;
    }
    mbCtrlFps = ctrlfps;
    AMLOG_INFO("CActiveVEEngine::SetTranscSettings: \n\t%s HDMI, %s LCD,%s control FPS.\n",
        mbRenderHDMI?"Render DEC0 to":"no", mbPreviewEnc?"preview":(mbRenderLCD?"Render DEC1 to":"no"), mbCtrlFps?"":" not");
    return  ME_OK;
}

AM_ERR CActiveVEEngine::SetMuxParamaters(const char *filename, AM_UINT cutSize)
{
    memcpy(&outFileName, filename, strlen(filename)+1);
    mSavingStrategy = cutSize;
    AMLOG_INFO("CActiveVEEngine::SetMuxParamaters: out name: %s,\n\tSaveStrategy(s): %d(0 for whole file).\n",
        outFileName, cutSize);
    return  ME_OK;
}

//finalize
AM_ERR CActiveVEEngine::StartFinalize()
{
    return  ME_OK;
}

AM_INT CActiveVEEngine::GetProcess()
{
    if(mState == STATE_FINALIZE)
        return 100*mTimeEncoded/mTimeShaftLength;
    if(mState == STATE_COMPLETED)
        return 100;

    return -1;
}

void CActiveVEEngine::setloop(AM_U8 loop)
{
    mShared.mbLoopPlay = loop;
}

void CActiveVEEngine::setTranscFrms(AM_INT frames)
{
    if(mpVideoTranscoder){
        mpVideoTranscoder->setTranscFrms(frames);
    }else{
        AMLOG_ERROR("setTranscFrms: No mpVideoTranscoder!\n");
    }
}

AM_ERR CActiveVEEngine::SetDecPPParamaters(AM_INT index){
    if(mEncHeight > 720){
        mShared.sTranscConfig.dec_w[index] = 1920;
        mShared.sTranscConfig.dec_h[index] = 1080;
    }else{
        if(mbRenderHDMI){
            mShared.sTranscConfig.dec_w[index] = 1280;
            mShared.sTranscConfig.dec_h[index] = 720;
        }else{
            mShared.sTranscConfig.dec_w[index] = 1280; // mEncWidth;//round up to 32?
            mShared.sTranscConfig.dec_h[index] = 720; // mEncHeight;
        }
    }
    return ME_OK;
}

AM_ERR CActiveVEEngine::setMuxParam()
{
    IMuxer*pMuxer;
    AM_ERR err;
    pMuxer = IMuxer::GetInterfaceFrom(mpFilterMuxer);
    pMuxer->SetOutputFile(outFileName);

    //hard code here
    mUFormatSpecific.video.pic_width = mEncWidth;
    mUFormatSpecific.video.pic_height = mEncHeight;
    mUFormatSpecific.video.framerate_num = 90000;
    mUFormatSpecific.video.framerate_den = mFpsBase;
    mUFormatSpecific.video.framerate = AM_VideoEncCalculateFrameRate(90000, mFpsBase);
    mUFormatSpecific.video.M = mbhasBfrm?3:1;
    mUFormatSpecific.video.N = 15;
    mUFormatSpecific.video.IDRInterval = 2;
    mUFormatSpecific.video.sample_aspect_ratio_num = 1;//hard code
    mUFormatSpecific.video.sample_aspect_ratio_den = 1;//hard code
    mUFormatSpecific.video.bitrate = mBitrate;
    mUFormatSpecific.video.lowdelay = 0;
    mUFormatSpecific.video.entropy_type = IParameters::EntropyType_H264_CABAC;
    err = mpMuxerControl->SetParameters(
        IParameters::StreamType_Video,
        IParameters::StreamFormat_H264,
        &mUFormatSpecific, 0);
    err = mpMuxerControl->SetSavingStrategy(
        mSavingStrategy?(IParameters::MuxerSavingFileStrategy_AutoSeparateFile):(IParameters::MuxerSavingFileStrategy_ToTalFile),
        IParameters::MuxerSavingCondition_FrameCount,
        IParameters::MuxerAutoFileNaming_ByNumber,
        mSavingStrategy*30);
    err = mpMuxerControl->SetMaxFileNumber(999999);
    AMLOG_INFO("setMuxParam.mFpsBase: %u, framerate: %u, outFileName: %s, err=%d\n",mFpsBase, mUFormatSpecific.video.framerate, outFileName, err);
    return ME_OK;
}

AM_ERR CActiveVEEngine::generateHLSPlaylist(AM_INTPTR index ,AM_INTPTR container)
{
    static AM_UINT seq_num = 0;
    AM_UINT start_index = 0;
    AM_INT save_file_nums = 20;
    AM_INT list_files_nums = 3;
    char full_file_name[256] = "";
    char name_base_for_playlist[256] = "files/transc_demo";
    char playlist[256] = "/dev/demo/AMBA_transc_demo.m3u8";

    AMLOG_INFO("==== New file generated, index %d, container %s, seq_num %d ====\n",
        index, AM_GetStringFromContainerType((IParameters::ContainerType)container), seq_num);

    if((start_index == 0)&&(index == 0)){
        AMLOG_DEBUG("The first file, start to generate HLS list from the 2nd file completed.\n");
        return ME_OK;
    }

    //calculate start index, not care wrapper case, todo
    if (index > list_files_nums) {
        start_index = index - list_files_nums;
    }

    if(mShared.mbLoopPlay && (index > save_file_nums)){
        sprintf(full_file_name, "%s_%06d.%s", outFileName, index -save_file_nums-1, AM_GetStringFromContainerType((IParameters::ContainerType)container));
        remove(full_file_name);
        AMLOG_INFO("==== remove file \"%s\" ====\n",full_file_name);
    }

    AM_WriteHLSConfigfile(playlist, name_base_for_playlist, start_index, index-1, AM_GetStringFromContainerType((IParameters::ContainerType)container), seq_num, mSavingStrategy);
    return ME_OK;
}

AM_ERR CActiveVEEngine::BuildTest2UdecFilterGragh(AM_INT mode)
{
    AM_ERR err;
    for(int i=0;i<2;i++){//here only 2 streams
        if(mpRecognizer[i]){
            filter_entry * sourceFilter = mpRecognizer[i]->GetFilterEntry();

            AMLOG_INFO("get sourceFilter = %s.\n",sourceFilter->pName);
            // create source filter
            IFilter *pFilter = sourceFilter->create((IEngine*)this);
            mpFilterDemuxer[i] = pFilter;
            if (pFilter == NULL) {
                AMLOG_ERROR("Create %s failed\n", sourceFilter->pName);
                return ME_NOT_EXIST;
            }
            AMLOG_INFO("Filter[%s] created.\n",sourceFilter->pName);

            // get IDemuxer
            mpDemuxer[i] = IDemuxer::GetInterfaceFrom(pFilter);
            if (mpDemuxer == NULL) {
                AMLOG_ERROR("No IDemuxer\n");
                AM_DELETE(pFilter);
                return ME_NO_INTERFACE;
            }

            // load file
            const char* filename = mpRecognizer[i]->GetFileName();
            AMLOG_INFO("Filter[%s] LoadFile %s, %p.\n",sourceFilter->pName,filename,mpRecognizer[i]->GetParser().context);

            err = mpDemuxer[i]->LoadFile(filename, mpRecognizer[i]->GetParser().context);
            if (err != ME_OK) {
                AMLOG_ERROR("LoadFile failed\n");
                AM_DELETE(pFilter);
                return err;
            }
            AMLOG_INFO("Filter[%s] LoadFile done.\n",sourceFilter->pName);
            mpRecognizer[i]->ClearParser();

            // get&check file type
            const char *pFileType = NULL;
            mpDemuxer[i]->GetFileType(pFileType);
            CheckFileType(pFileType);

            if ((err = AddFilter(mpFilterDemuxer[i])) != ME_OK)
                return err;
        }
    }

    if(mode == 1){
        //set ppmode=3
        mShared.ppmode = 3;
        mShared.mDSPmode = 3;
    }else{
        //set ppmode=1
        mShared.ppmode = 1;
        mShared.mDSPmode = 1;
    }

    if ((mpFilterVideoDecoder[0] = CreateGeneralVideoDecodeFilter((IVEEngine*)this)) == NULL)
        return ME_ERROR;
    if ((err = AddFilter(mpFilterVideoDecoder[0])) != ME_OK)
        return err;

    //here should build gpu filter, not renderer
    if ((mpFilterVideoRenderer[0] = CreateVideoRenderFilter((IVEEngine*)this)) == NULL)
        return ME_ERROR;
    if ((err = AddFilter(mpFilterVideoRenderer[0])) != ME_OK)
        return err;

    if(mpDemuxer[1]){
        if ((mpFilterVideoDecoder[1] = CreateGeneralVideoDecodeFilter((IVEEngine*)this)) == NULL)
            return ME_ERROR;
        if ((err = AddFilter(mpFilterVideoDecoder[1])) != ME_OK)
            return err;

        if ((mpFilterVideoRenderer[1] = CreateVideoRenderFilter((IVEEngine*)this)) == NULL)
            return ME_ERROR;
        if ((err = AddFilter(mpFilterVideoRenderer[1])) != ME_OK)
            return err;
    }

    //GetMediaFormat here, todo
    //connect video
    if ((err = Connect(mpFilterDemuxer[0], 0, mpFilterVideoDecoder[0], 0)) != ME_OK){
        AM_ERROR("Connect mpFilterDemuxer[0]<->mpFilterVideoDecoder[0] failed.\n");
        return err;
    }
    mpDemuxer[0]->SetIndex(0);
    if ((err = Connect(mpFilterVideoDecoder[0], 0, mpFilterVideoRenderer[0], 0)) != ME_OK){
        AM_ERROR("Connect mpFilterVideoDecoder[0]<->mpFilterVideoRenderer[0] failed.\n");
        return err;
    }
    if(mpDemuxer[1]){
        if ((err = Connect(mpFilterDemuxer[1], 0, mpFilterVideoDecoder[1], 0)) != ME_OK){
            AM_ERROR("Connect mpFilterDemuxer[1]<->mpFilterVideoDecoder[1] failed.\n");
            return err;
        }
        mpDemuxer[1]->SetIndex(1);
        if ((err = Connect(mpFilterVideoDecoder[1], 0, mpFilterVideoRenderer[1], 0)) != ME_OK){
            AM_ERROR("Connect mpFilterVideoDecoder[1]<->mpFilterVideoRenderer[1] failed.\n");
            return err;
        }
    }

    //set master renderer
    mpMasterFilter = mpFilterVideoRenderer[0];
    AM_ASSERT(mpMasterFilter);
    mpMasterFilter->SetMaster(true);
    mpMasterRenderer = IRender::GetInterfaceFrom(mpMasterFilter);
    AM_ASSERT(mpMasterRenderer);

    IFilter::INFO info;
    mpMasterFilter->GetInfo(info);
    AMLOG_INFO("master renderer is %s.\n",info.pName);

    //pb-config
    int nums = 1;
    if(mpDemuxer[1]){nums = 2;}
    for(int i=0; i < nums; i++){
        if(mpDemuxer[i]){
            mpDemuxer[i]->EnableAudio(false);//disable audio now
            mpDemuxer[i]->EnableVideo(true);
            mpDemuxer[i]->EnableSubtitle(false);
            mpDemuxer[i]->GetTotalLength(mDuration[i]);
            AMLOG_INFO("current item tot lenth %llu.\n", mDuration[i]);
        }
    }
    return ME_OK;
}

AM_ERR CActiveVEEngine::BuildPreviewFilterGragh()
{
    AM_ERR err;
    for(int i=0;i<2;i++){//here only 2 streams
        if(mpRecognizer[i]){
            filter_entry * sourceFilter = mpRecognizer[i]->GetFilterEntry();

            AMLOG_INFO("get sourceFilter = %s.\n",sourceFilter->pName);
            // create source filter
            IFilter *pFilter = sourceFilter->create((IEngine*)this);
            mpFilterDemuxer[i] = pFilter;
            if (pFilter == NULL) {
                AMLOG_ERROR("Create %s failed\n", sourceFilter->pName);
                return ME_NOT_EXIST;
            }
            AMLOG_INFO("Filter[%s] created.\n",sourceFilter->pName);

            // get IDemuxer
            mpDemuxer[i] = IDemuxer::GetInterfaceFrom(pFilter);
            if (mpDemuxer == NULL) {
                AMLOG_ERROR("No IDemuxer\n");
                AM_DELETE(pFilter);
                return ME_NO_INTERFACE;
            }

            // load file
            const char* filename = mpRecognizer[i]->GetFileName();
            AMLOG_INFO("Filter[%s] LoadFile %s, %p.\n",sourceFilter->pName,filename,mpRecognizer[i]->GetParser().context);

            err = mpDemuxer[i]->LoadFile(filename, mpRecognizer[i]->GetParser().context);
            if (err != ME_OK) {
                AMLOG_ERROR("LoadFile failed\n");
                AM_DELETE(pFilter);
                return err;
            }
            AMLOG_INFO("Filter[%s] LoadFile done.\n",sourceFilter->pName);
            mpRecognizer[i]->ClearParser();

            // get&check file type
            const char *pFileType = NULL;
            mpDemuxer[i]->GetFileType(pFileType);
            CheckFileType(pFileType);

            if ((err = AddFilter(mpFilterDemuxer[i])) != ME_OK)
                return err;
        }
    }

    //set ppmode=1
    mShared.ppmode = 3;
    mShared.mDSPmode = 3;
    mShared.sTranscConfig.chroma_fmt = UDEC_CFG_FRM_CHROMA_FMT_420;

    if ((mpFilterVideoDecoder[0] = CreateGeneralVideoDecodeFilter((IVEEngine*)this)) == NULL)
        return ME_ERROR;
    if ((err = AddFilter(mpFilterVideoDecoder[0])) != ME_OK)
        return err;

    //GetMediaFormat here, todo
    //connect video
    if ((err = Connect(mpFilterDemuxer[0], 0, mpFilterVideoDecoder[0], 0)) != ME_OK){
        AM_ERROR("Connect mpFilterDemuxer[0]<->mpFilterVideoDecoder[0] failed.\n");
        return err;
    }
    mpDemuxer[0]->SetIndex(0);

    if(mpDemuxer[1]){
        if ((mpFilterVideoDecoder[1] = CreateGeneralVideoDecodeFilter((IVEEngine*)this)) == NULL)
            return ME_ERROR;
        if ((err = AddFilter(mpFilterVideoDecoder[1])) != ME_OK)
            return err;

        //connect video
        if ((err = Connect(mpFilterDemuxer[1], 0, mpFilterVideoDecoder[1], 0)) != ME_OK){
            AM_ERROR("Connect mpFilterDemuxer[1]<->mpFilterVideoDecoder[1] failed.\n");
            return err;
        }
        mpDemuxer[1]->SetIndex(1);
    }

    if ((mpFilterVideoEffecter = CreateVideoEffecterPreviewFilter((IVEEngine*)this)) == NULL)
        return ME_ERROR;
    if ((err = AddFilter(mpFilterVideoEffecter)) != ME_OK)
        return err;
    //mpVideoEffecterControl = IVideoEffecterControl::GetInterfaceFrom(mpFilterVideoEffecter);
    //AM_ASSERT(mpVideoEffecterControl);

    AM_UINT inpin_index;
    if(ME_OK!=mpFilterVideoEffecter->AddInputPin(inpin_index, 0)){
        AM_ERROR("mpFilterVideoEffecter->AddInputPin[%d] failed.\n",inpin_index);
        return err;
    }
    if ((err = Connect(mpFilterVideoDecoder[0], 0, mpFilterVideoEffecter, inpin_index)) != ME_OK){
        AM_ERROR("Connect mpFilterVideoDecoder[0]<->mpFilterVideoEffecter[%d] failed.\n",inpin_index);
        return err;
    }

    if(mpDemuxer[1]){
        if(ME_OK!=mpFilterVideoEffecter->AddInputPin(inpin_index, 0)){
            AM_ERROR("mpFilterVideoEffecter->AddInputPin[%d] failed.\n",inpin_index);
            return err;
        }
        if ((err = Connect(mpFilterVideoDecoder[1], 0, mpFilterVideoEffecter, inpin_index)) != ME_OK){
            AM_ERROR("Connect mpFilterVideoDecoder[1]<->mpFilterVideoEffecter[%d] failed.\n",inpin_index);
            return err;
        }
    }

    //set master renderer
    mpMasterFilter = mpFilterVideoEffecter;
    AM_ASSERT(mpMasterFilter);
    mpMasterFilter->SetMaster(true);
    mpMasterRenderer = IRender::GetInterfaceFrom(mpMasterFilter);
    AM_ASSERT(mpMasterRenderer);

    IFilter::INFO info;
    mpMasterFilter->GetInfo(info);
    AMLOG_DEBUG("master renderer is %s.\n",info.pName);

    //pb-config
    int nums = 1;
    if(mpDemuxer[1]){nums = 2;}
    for(int i=0; i < nums; i++){
        if(mpDemuxer[i]){
            mpDemuxer[i]->EnableAudio(false);//disable audio now
            mpDemuxer[i]->EnableVideo(true);
            mpDemuxer[i]->EnableSubtitle(false);
            mpDemuxer[i]->GetTotalLength(mDuration[i]);
            AMLOG_INFO("current item tot lenth %llu.\n", mDuration[i]);
        }
    }
    AMLOG_INFO("BuildPreviewFilterGragh done.\n");

    return ME_OK;
}

AM_ERR CActiveVEEngine::BuildFinalizeFilterGragh()
{
    //todo
    return ME_OK;
}

AM_ERR CActiveVEEngine::BuildTranscodeFilterGragh()
{
    AM_ERR err;

    AMLOG_INFO("BuildTranscodeFilterGragh:\n");

    if(!mpRecognizer[0]){
        AM_ERROR("There has no mpRecognizer been created, buildgraph failed.\n");
        return ME_ERROR;
    }

    for(int i=0;i<2;i++){//here at most 2 streams
        if(mpRecognizer[i]){
            SetDecPPParamaters(i);

            filter_entry * sourceFilter = mpRecognizer[i]->GetFilterEntry();

            AMLOG_INFO("get sourceFilter = %s.\n",sourceFilter->pName);
            // create source filter
            IFilter *pFilter = sourceFilter->create((IEngine*)this);
            mpFilterDemuxer[i] = pFilter;
            if (pFilter == NULL) {
                AMLOG_ERROR("Create %s failed\n", sourceFilter->pName);
                return ME_NOT_EXIST;
            }
            AMLOG_INFO("Filter[%s] created.\n",sourceFilter->pName);

            // get IDemuxer
            mpDemuxer[i] = IDemuxer::GetInterfaceFrom(pFilter);
            if (mpDemuxer == NULL) {
                AMLOG_ERROR("No IDemuxer\n");
                AM_DELETE(pFilter);
                return ME_NO_INTERFACE;
            }
            mpDemuxer[i]->SetIndex(i);

            // load file
            const char* filename = mpRecognizer[i]->GetFileName();
            AMLOG_INFO("Filter[%s] LoadFile %s, %p.\n",sourceFilter->pName,filename,mpRecognizer[i]->GetParser().context);

            err = mpDemuxer[i]->LoadFile(filename, mpRecognizer[i]->GetParser().context);
            if (err != ME_OK) {
                AMLOG_ERROR("LoadFile failed\n");
                AM_DELETE(pFilter);
                return err;
            }
            AMLOG_INFO("Filter[%s] LoadFile done.\n",sourceFilter->pName);
            mpRecognizer[i]->ClearParser();

            mFpsBase = (0 == mFpsBase)?mShared.mVideoTicks:mFpsBase;//get VideoTicks from demuxer

            // get&check file type
            const char *pFileType = NULL;
            mpDemuxer[i]->GetFileType(pFileType);
            CheckFileType(pFileType);

            if ((err = AddFilter(mpFilterDemuxer[i])) != ME_OK)
                return err;
        }
    }

    //set ppmode=3
    mShared.ppmode = 3;// 0: no pp; 1: decpp; 2: voutpp; 3: transode
    mShared.mDSPmode = 3;//
    mShared.sTranscConfig.chroma_fmt = UDEC_CFG_FRM_CHROMA_FMT_422;
    mShared.mbLoopPlay = 1;

    if ((mpFilterVideoDecoder[0] = CreateGeneralVideoDecodeFilter((IVEEngine*)this)) == NULL)
        return ME_ERROR;
    if ((err = AddFilter(mpFilterVideoDecoder[0])) != ME_OK)
        return err;

    //GetMediaFormat here, todo
    //connect video
    if ((err = Connect(mpFilterDemuxer[0], 0, mpFilterVideoDecoder[0], 0)) != ME_OK){
        AM_ERROR("Connect mpFilterDemuxer[0]<->mpFilterVideoDecoder[0] failed.\n");
        return err;
    }

    if(mpDemuxer[1]){
        if ((mpFilterVideoDecoder[1] = CreateGeneralVideoDecodeFilter((IVEEngine*)this)) == NULL)
            return ME_ERROR;
        if ((err = AddFilter(mpFilterVideoDecoder[1])) != ME_OK)
            return err;

        //connect video
        if ((err = Connect(mpFilterDemuxer[1], 0, mpFilterVideoDecoder[1], 0)) != ME_OK){
            AM_ERROR("Connect mpFilterDemuxer[1]<->mpFilterVideoDecoder[1] failed.\n");
            return err;
        }
    }

    AM_INT nums = 1;
    if(mpDemuxer[1]){nums = 2;}
    for(AM_INT i=0; i < nums; i++){
        if(mpDemuxer[i]){
            mpDemuxer[i]->EnableAudio(false);//disable audio now
            mpDemuxer[i]->EnableVideo(true);
            mpDemuxer[i]->EnableSubtitle(false);
            mpDemuxer[i]->GetTotalLength(mDuration[i]);
            AMLOG_INFO("current item tot lenth %llu.\n", mDuration[i]);
        }
    }

    if ((mpFilterVideoTranscoder = CreateVideoTranscoderFilter((IVEEngine*)this)) == NULL)
        return ME_ERROR;
    if ((err = AddFilter(mpFilterVideoTranscoder)) != ME_OK)
        return err;
    AMLOG_INFO("CreateVideoTranscoderFilter done.\n");
    mpVideoTranscoder = IVideoTranscoder::GetInterfaceFrom(mpFilterVideoTranscoder);

    AM_UINT inpin_index;
    if(ME_OK!=mpFilterVideoTranscoder->AddInputPin(inpin_index, 0)){
        AM_ERROR("mpFilterVideoTranscoder->AddInputPin[%d] failed.\n",inpin_index);
        return err;
    }
    if ((err = Connect(mpFilterVideoDecoder[0], 0, mpFilterVideoTranscoder, inpin_index)) != ME_OK){
        AM_ERROR("Connect mpFilterVideoDecoder[0]<->mpFilterVideoTranscoder[%d] failed.\n", inpin_index);
        return err;
    }

    if(mpDemuxer[1]){
        if(ME_OK!=mpFilterVideoTranscoder->AddInputPin(inpin_index, 0)){
            AM_ERROR("mpFilterVideoTranscoder->AddInputPin[%d] failed.\n",inpin_index);
            return err;
        }
        if ((err = Connect(mpFilterVideoDecoder[1], 0, mpFilterVideoTranscoder, inpin_index)) != ME_OK){
            AM_ERROR("Connect mpFilterVideoDecoder[1]<->mpFilterVideoTranscoder[%d] failed.\n", inpin_index);
            return err;
        }
    }

    //hardcode here
    //mpVideoTranscoder->SetEncoderParamaters(1280, 720, 720, 480, true, true, false, 2000000);
    AMLOG_INFO("Transcode Engine: source size is %u*%u, enc size is %u*%u.",
        mShared.sTranscConfig.video_w[0], mShared.sTranscConfig.video_h[0], mEncWidth, mEncHeight);
    if(mbEncKeepAspectRatio && (mShared.sTranscConfig.video_w[0]*mEncHeight != mShared.sTranscConfig.video_h[0]*mEncWidth)){
    //if Keep AspectRatio, reset the enc size
        mEncWidth = (mShared.sTranscConfig.video_w[0]*mEncHeight)/mShared.sTranscConfig.video_h[0];
        if((mEncWidth/16)*16 != mEncWidth)
            mEncWidth = (mEncWidth + 0x0f) & ~0x0f;// roundup to 16
        mShared.sTranscConfig.enc_w = mEncWidth;
        AMLOG_INFO("Transcode Engine: reset enc_w to: %u.", mEncWidth);
    }

    mFpsBase = (0 == mFpsBase)?3000:mFpsBase;
    AMLOG_INFO("Transcode Engine: SetEncParamaters mVideoTicks: %u.", mFpsBase);
    mpVideoTranscoder->SetEncParamaters(mEncWidth, mEncHeight, mFpsBase, mBitrate, mbhasBfrm);
    mpVideoTranscoder->SetTranscSettings(mbRenderHDMI, mbRenderLCD, mbPreviewEnc, mbCtrlFps);


    if ((mpFilterVideoMemEncoder = CreateVideoMemEncoderFilter((IVEEngine*)this)) == NULL)
        return ME_ERROR;
    if ((err = AddFilter(mpFilterVideoMemEncoder)) != ME_OK)
        return err;
    AMLOG_INFO("CreateVideoMemEncoderFilter done.\n");
    mpVideoMemEncoder = IVideoMemEncoder::GetInterfaceFrom(mpFilterVideoMemEncoder);
    if ((err = Connect(mpFilterVideoTranscoder, 0, mpFilterVideoMemEncoder, 0)) != ME_OK){
        AM_ERROR("Connect mpFilterVideoTranscoder[0]<->mpFilterVideoMemEncoder[0] failed.\n");
        return err;
    }

    // ffmpeg muxer
    if ((mpFilterMuxer = CreateFFmpegMuxer((IEngine*)this)) == NULL) {
        AM_ERROR("CFFMpegMuxer failed.\n");
        return ME_ERROR;
    }
    if ((err = AddFilter(mpFilterMuxer)) != ME_OK)
        return err;

    mpMuxerControl = IMuxerControl::GetInterfaceFrom(mpFilterMuxer);
    AM_ASSERT(mpMuxerControl);

    mpMuxerControl->SetContainerType(IParameters::MuxerContainer_TS);

    //add muxer input pin for video
    mpFilterMuxer->AddInputPin(inpin_index, IParameters::StreamType_Video);

    //connect video
    if ((err = Connect(mpFilterVideoMemEncoder, 0, mpFilterMuxer, 0)) != ME_OK){
        AM_ERROR("Connect failed.\n");
        return err;
    }

    setMuxParam();
//Audio, Pridata

    AMLOG_INFO("BuildTranscodeFilterGragh done.\n");

    return ME_OK;
}

void CActiveVEEngine::HandleReadyMsg(AM_MSG& msg)
{
    IFilter *pFilter = (IFilter*)msg.p1;
    AM_UINT i;

    AM_ASSERT(pFilter != NULL);

    AM_UINT index;
    if (FindFilter(pFilter, index) != ME_OK)
        return;

    SetReady(index);
    if (AllRenderersReady()) {
        AM_PRINTF("---All renderers ready----\n");
        mpClockManager->StartClock();
        StartAllRenderers();
        SetState(STATE_PREVIEW_RUN);
        //AM_ASSERT(mState == STATE_PREVIEW_RUN);
        if (mbPaused) {
            AMLOG_INFO("PauseAllFilters\n");
            for(i=0; i<mnFilters; i++) {
                mFilters[i].pFilter->Pause();
            }
            SetState(STATE_PREVIEW_PAUSE);
        }
    }
}

void CActiveVEEngine::HandleEOSMsg(AM_MSG& msg)
{
    IFilter *pFilter = (IFilter*)msg.p1;
//    AM_ERR err;
//    AM_PlayItem* pnext;
    AM_ASSERT(pFilter != NULL);

    AM_UINT index;
    if (FindFilter(pFilter, index) != ME_OK) {
        AMLOG_ERROR("cannot find filter? %d\n", index);
        return;
    }
    SetEOS(index);

    if (AllRenderersEOS()) {
        AMLOG_INFO("All renderers eos\n");

        AMLOG_INFO("Sending eos to app:\n");
        SetState(STATE_COMPLETED);
        PostAppMsg(IMediaControl :: MSG_PLAYER_EOS);
        AMLOG_INFO("Sending eos to app done.\n");
    }
}

void CActiveVEEngine::CheckFileType(const char *pFileType)
{
    // default
    mbUseAbsoluteTime = true;

    if (!pFileType) {
        return ;
    }

    // rtsp protocol
    if (!strncmp(pFileType, "rtsp", 4)) {
        mbUseAbsoluteTime = false;
        AMLOG_INFO("CActiveVEEngine::CheckFileType: rtsp protocol.\n");
        return ;
    }

    // flac type
    if (!strncmp(pFileType, "flac", 4)) {
        mbUseAbsoluteTime = false;
        AMLOG_INFO("CActiveVEEngine::CheckFileType: flac type.\n");
        return ;
    }
}

void CActiveVEEngine::GetDecType(DECSETTING decSetDefault[])
{
    int i = 0;
    while(i != CODEC_NUM)
    {
        decSetDefault[i].codecId = (((SConsistentConfig *)mpOpaque)->decSet[i]).codecId;
        decSetDefault[i].dectype = (((SConsistentConfig *)mpOpaque)->decSet[i]).dectype;
        i++;
    }
    return;
}
void CActiveVEEngine::SetDecType(DECSETTING decSetCur[])
{
    int i = 0;
    while(i != CODEC_NUM)
    {
        (((SConsistentConfig *)mpOpaque)->decSet[i]).codecId = decSetCur[i].codecId;
        //Check if the setting within the ability of the decoder
        if((decSetCur[i].dectype & mdecCap[i]) != 0)
            (((SConsistentConfig *)mpOpaque)->decSet[i]).dectype = decSetCur[i].dectype;
        i++;
    }
    return;
}
void CActiveVEEngine::ResetParameters()
{
    memset(mpFilterDemuxer, 0, sizeof(mpFilterDemuxer));
    memset(mpFilterVideoDecoder, 0, sizeof(mpFilterVideoDecoder));
    memset(mpFilterVideoRenderer, 0, sizeof(mpFilterVideoRenderer));
    memset(mpDemuxer, 0, sizeof(mpDemuxer));
    mpVideoEffecterControl = NULL;
    mpVideoTranscoder = NULL;
    mpVideoMemEncoder = NULL;
    mpMasterFilter = NULL;
    mpMuxerControl = NULL;
    mpFilterVideoEffecter = NULL;
    mpFilterVideoTranscoder = NULL;
    mpFilterVideoMemEncoder = NULL;
    mpFilterMuxer = NULL;
    AMLOG_INFO("---reset active_ve_engine---\n");
}

#ifdef AM_DEBUG
//obsolete, would be removed later
AM_ERR CActiveVEEngine::SetLogConfig(AM_UINT index, AM_UINT level, AM_UINT option)
{
/*    if(mnFilters <= index || level > LogAll) {
        AMLOG_INFO("CActiveVEEngine::SetLogConfig, invalid arguments: index=%d,level=%d,option=%d.\n", index, level, option);
        return ME_BAD_PARAM;
    }

    if(!mFilters[index].pFilter) {
        AMLOG_INFO("CActiveVEEngine::SetLogConfig, mFilters[index].pFilter not ready, must have errors.\n");
        return ME_ERROR;
    }

    ((CFilter*)mFilters[index].pFilter)->SetLogLevelOption((ELogLevel)mShared.logConfig[index].log_level, mShared.logConfig[index].log_option, 0);
    */return ME_OK;
}

void CActiveVEEngine::PrintState()
{
    AM_UINT i = 0;
//    AM_UINT udec_state, vout_state, error_code;
    AMLOG_INFO(" CActiveVEEngine's state is %d.\n", mState);
    AMLOG_INFO(" is_paused %d, cmd cnt from app %d, msg from filters cnd %d.\n", mbPaused, mpCmdQueue->GetDataCnt(), mpFilterMsgQ->GetDataCnt());
    for (i = 0; i < mnFilters; i ++) {
        mFilters[i].pFilter->PrintState();
    }
    //GetUdecState(mShared.mIavFd, &udec_state, &vout_state, &error_code);
    //AMLOG_INFO(" Udec state %d, Vout state %d, error_code 0x%x.\n", udec_state, vout_state, error_code);
}
#endif

bool CActiveVEEngine::ProcessGenericCmd(CMD& cmd)
{
//    AM_PlayItem* item = NULL;
    AM_ERR err;
//    AM_UINT i;

    AMLOG_CMD("****CActiveVEEngine::ProcessGenericCmd, cmd.code %d, state %d.\n", cmd.code, mState);

    switch (cmd.code) {
        case CMD_STOP:
            AMLOG_INFO("****CActiveVEEngine STOP cmd, start.\n");
            mbRun = false;
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine STOP cmd, done.\n");
            break;

        case VECMD_ADD_SOURCE:
            AMLOG_INFO("****CActiveVEEngine VECMD_ADD_SOURCE cmd, start.\n");
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_ADD_SOURCE cmd, done.\n");
            break;

        case VECMD_PREPARE_SOURCE:
            AMLOG_INFO("****CActiveVEEngine VECMD_PREPARE_SOURCE cmd, start.\n");
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_PREPARE_SOURCE cmd, done.\n");
            break;

        case VECMD_DEL_SOURCE:
            AMLOG_INFO("****CActiveVEEngine VECMD_DEL_SOURCE cmd, start.\n");
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_DEL_SOURCE cmd, done.\n");
            break;

        case VECMD_SEEK:
            AMLOG_INFO("****CActiveVEEngine VECMD_SEEK cmd, start.\n");
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_SEEK cmd, done.\n");
            break;

        case VECMD_INIT_PREVIEW:
            AMLOG_INFO("****CActiveVEEngine VECMD_INIT_PREVIEW cmd, start.\n");
            mbRun = true;
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_INIT_PREVIEW cmd, done.\n");
            break;

        case VECMD_START_PREVIEW:
            AMLOG_INFO("****CActiveVEEngine VECMD_START_PREVIEW cmd, start.\n");
            err = RunAllFilters();
            if (err != ME_OK){
                AMLOG_ERROR("RunAllfilters fail %d.\n", err);
                SetState(STATE_ERROR);
            }
            SetState(STATE_PREVIEW_RUN);
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_START_PREVIEW cmd, done.\n");
            break;

        case VECMD_RESUME_PREVIEW:
            AMLOG_INFO("****CActiveVEEngine VECMD_RESUME_PREVIEW cmd, start.\n");
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_RESUME_PREVIEW cmd, done.\n");
            break;

        case VECMD_PAUSE_PREVIEW:
            AMLOG_INFO("****CActiveVEEngine VECMD_PAUSE_PREVIEW cmd, start.\n");
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_PAUSE_PREVIEW cmd, done.\n");
            break;

        case VECMD_STOP_PREVIEW:
            AMLOG_INFO("****CActiveVEEngine VECMD_STOP_PREVIEW cmd, start.\n");

            AM_ASSERT(mpClockManager);
            mpClockManager->StopClock();
            ClearGraph();
            ResetParameters();
            NewSession();
            SetState(STATE_STOPPED);
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_STOP_PREVIEW cmd, done.\n");
            break;

        case VECMD_START_FINALIZE:
            AMLOG_INFO("****CActiveVEEngine VECMD_START_FINALIZE cmd, start.\n");
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_START_FINALIZE cmd, done.\n");
            break;

        case VECMD_STOP_FINALIZE:
            AMLOG_INFO("****CActiveVEEngine VECMD_STOP_FINALIZE cmd, start.\n");
            AM_ASSERT(mpClockManager);
            if(mpVideoTranscoder){
                mpVideoTranscoder->StopEncoding(cmd.res32_1);
                AMLOG_INFO("mpVideoTranscoder->StopEncoding done.\n");
            }
            if(mState != STATE_FINALIZE){
                AM_ASSERT(mpClockManager);
                mpClockManager->StopClock();
                ClearGraph();
                ResetParameters();
                NewSession();
                SetState(STATE_STOPPED);
                AMLOG_PRINTF("** allMuxerEOS, before mpCondStopfilters->Signal();.\n");
                mpCondStopfilters->Signal();
                PostAppMsg(IMediaControl::MSG_PLAYER_EOS);
                AMLOG_PRINTF("** PostAppMsg(IMediaControl::MSG_PLAYER_EOS) done.\n");
            }
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_STOP_FINALIZE cmd, done.\n");
            break;

        case VECMD_COMPELETE_FINALIZE:
            AMLOG_INFO("****CActiveVEEngine VECMD_COMPELETE_FINALIZE cmd, start.\n");
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActiveVEEngine VECMD_COMPELETE_FINALIZE cmd, done.\n");
            break;

        default:
            return false;
    }
    return true;
}

bool CActiveVEEngine::ProcessGenericMsg(AM_MSG& msg)
{
//    AM_PlayItem* item = NULL;
//    AM_ERR err;

    AMLOG_CMD("****CActiveVEEngine::ProcessGenericMsg, msg.code %d, msg.sessionID %d, state %d.\n", msg.code, msg.sessionID, mState);

    //discard old sessionID msg first
    if (!IsSessionMsg(msg)) {
        AMLOG_WARN("got msg(code %d) with old sessionID %d, current sessionID %d.\n", msg.code, msg.sessionID, mSessionID);
        return true;
    }

    switch (msg.code) {
        case MSG_READY:
            AMLOG_INFO("-----MSG_READY-----\n");
            HandleReadyMsg(msg);
            break;

        case MSG_EOS:
            AMLOG_INFO("-----MSG_EOS-----\n");
            if(mState == STATE_FINALIZE){
                AM_ASSERT(mpClockManager);
                mpClockManager->StopClock();
                ClearGraph();
                ResetParameters();
                NewSession();
                SetState(STATE_STOPPED);
                AMLOG_PRINTF("** allMuxerEOS, before mpCondStopfilters->Signal();.\n");
                mpCondStopfilters->Signal();
                PostAppMsg(IMediaControl::MSG_PLAYER_EOS);
                AMLOG_PRINTF("** PostAppMsg(IMediaControl::MSG_PLAYER_EOS) done.\n");
            }else if (mState == STATE_STOPPING) {
                AMLOG_PRINTF("=== Engine received MSG_EOS.\n");
                SetState(STATE_STOPPED);
                PostAppMsg(IMediaControl::MSG_PLAYER_EOS);
                AMLOG_PRINTF("** PostAppMsg(IMediaControl::MSG_PLAYER_EOS) done.\n");
                NewSession();
            }else{
                HandleEOSMsg(msg);
            }
            AMLOG_INFO("-----MSG_EOS done-----\n");
            break;

        case MSG_AVSYNC:
            if(mState == STATE_FINALIZE){break;}
            if(mShared.mbAlreadySynced)
                break;
            AMLOG_INFO("-----MSG_AVSYNC-----\n");
            //HandleSyncMsg(msg);
            AllFiltersCmd(IActiveObject::CMD_AVSYNC);
            mShared.mbAlreadySynced = 1;
            break;

//encode related

        case MSG_DURATION:
        case MSG_FILESIZE:
            AMLOG_PRINTF("INTERNAL_ENDING msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
            AM_ASSERT(mState == STATE_FINALIZE);
            if (mState == STATE_FINALIZE) {
                SetState(STATE_STOPPING);
                AMLOG_PRINTF("=== Engine received MSG_INTERNAL_ENDING.\n");
                StopTransc(2);
            }
            break;

        case MSG_STORAGE_ERROR:
            AMLOG_PRINTF("STORAGE_ERROR msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
            AM_ASSERT(mState == STATE_FINALIZE);
            if (mState == STATE_FINALIZE) {

                if (IParameters::MuxerSavingFileStrategy_AutoSeparateFile == 1) {
                    mpFilterMuxer->PostMsg(CMD_DELETE_FILE);
                } else {
                    //abort recording, the muxed file maybe corrupted
                    AM_ERROR("NOT ENOUGH storage!!! need preset max file size or max duration according to storage status, or use auto-slip/auto-delete saving strategy.\n");
                    AMLOG_ERROR("Abort recording now.\n");
                    SetState(STATE_STOPPING);
                    StopTransc(2);
                }
            }
            break;

        case MSG_NEWFILE_GENERATED:
            AM_MSG msg0;
            AMLOG_PRINTF("MSG_NEWFILE_GENERATED msg comes, mState %d, code %d, msg.session id %d, session %d.\n", mState, msg.code, msg.sessionID, mSessionID);
            msg0.code = IMediaControl::MSG_NEWFILE_GENERATED_POST_TO_APP;
            msg0.sessionID = mSessionID;
            msg0.p2 = msg.p2;//current index
            msg0.p3 = msg.p3;
            msg0.p5 = msg.p5;//container type
            PostAppMsg(msg0);
            generateHLSPlaylist(msg0.p2, msg0.p5);
            break;

        case MSG_ERROR:
            //HandleErrorMsg(msg);
            break;

        default:
            break;
    }
    AMLOG_CMD("****CActiveVEEngine::ProcessGenericMsg, msg.code %d done, msg.sessionID %d, state %d.\n", msg.code, msg.sessionID, mState);

    return true;
}

void CActiveVEEngine::ProcessCMD(CMD& oricmd)
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
                //drag mode, to do
                ProcessGenericCmd(cmd1);
                return;
            }
        }
    } else {
        AMLOG_ERROR("must not come here.\n");
        return;
    }
}

void CActiveVEEngine::OnRun()
{
    CMD cmd, nextCmd;
    AM_MSG msg;
    CQueue::QType type;
//    CQueueInputPin* pPin;
    CQueue::WaitResult result;
    AM_ASSERT(mpFilterMsgQ);
    AM_ASSERT(mpCmdQueue);
    mpWorkQ->CmdAck(ME_OK);

    while (mbRun) {
        AMLOG_STATE("CActiveVEEngine: mState=%d, msg cnt from filters %d.\n", mState, mpFilterMsgQ->GetDataCnt());
        if (mState == STATE_ERROR) {
//            PostAppMsg(IMediaControl :: MSG_ERROR);
        }

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
}

