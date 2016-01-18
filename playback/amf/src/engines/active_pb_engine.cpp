
/**
 * active_pb_engine.cpp
 *
 * History:
 *    2011/05/09 - [Zhi He] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "active_pb_engine"
//#define AMDROID_DEBUG

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if PLATFORM_ANDROID
#include "sys/atomics.h"
#endif

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_queue.h"
#include "am_if.h"
#include "am_pbif.h"
#include "am_mw.h"
#include "am_base.h"
#include "am_util.h"
#include "pbif.h"
#include "active_pb_engine.h"
#include "filter_list.h"
#include "filter_registry.h"
#include "am_dsp_if.h"

//#define _debug_old_flow_
#define _debug_use_stable_strategy_for_seek_

#define MAX_CMD_COUNT 64

IPBControl* CreateActivePBControl(void *audiosink)
{
    return (IPBControl*)CActivePBEngine::Create(audiosink);
}

//-----------------------------------------------------------------------
//
// CActivePBEngine
//
//-----------------------------------------------------------------------
CActivePBEngine* CActivePBEngine::Create(void *audiosink)
{
    CActivePBEngine *result = new CActivePBEngine(audiosink);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CActivePBEngine::CActivePBEngine(void *audiosink):
    inherited("active_pb_engine"),
    mbRun(true),
    mbPaused(false),
    mLoopPlayList(0),
    mLoopCurrentItem(0),
    mRandonSelectNextItem(0),
    mNeedReConfig(0),
    mpCurrentItem(NULL),
    mpFreeList(NULL),
    mnFreedCnt(0),
    mbSeeking(false),
    mpFilterDemuxer(NULL),
    mpFilterAudioDecoder(NULL),
    mpFilterVideoDecoder(NULL),
    mpFilterAudioRenderer(NULL),
    mpFilterVideoRenderer(NULL),
    mpFilterAudioEffecter(NULL),
    mpFilterVideoEffecter(NULL),
    mFatalErrorCnt(0),
    mTrySeekTimeGap(2000),
    mState(STATE_IDLE),
    mpRecognizer(NULL),
    mpDemuxer(NULL),
    mpMasterRenderer(NULL),
    mpVoutController(NULL),
    mpDecodeController(NULL),
    mpDataRetriever(NULL),
    mpClockManager(NULL),
    mpAudioSink(audiosink),
    mpMasterFilter(NULL),
    mMostPriority(0),
    mSeekTime(0),
    mbCurTimeValid(false),
    mCurTime(0),
    mbUseAbsoluteTime(true),
    mPrivateDataCallbackCookie(NULL),
    mPrivatedataCallback(NULL),
    mpPrivateDataBufferForCallback(NULL),
    mPrivateDataBufferToTSize(0),
    mRequestVoutMask((1<<eVoutLCD)|(1<<eVoutHDMI))
{
    AM_UINT i = 0;
    char configfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};
    memset((void*)&mShared, 0, sizeof(mShared));

    //debug
    mShared.mEngineVersion = 1;
    mShared.mbIavInited = 0;
    mShared.mIavFd = -1;//init value

    //deinterlace config
    mShared.bEnableDeinterlace = true;

    //init playlist
    mPlayListHead.type = PlayItemType_None;
    mPlayListHead.pPre = mPlayListHead.pNext = &mPlayListHead;
    mToBePlayedItem.type = PlayItemType_None;
    mToBePlayedItem.pPre = mToBePlayedItem.pNext = &mToBePlayedItem;

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

    AMLOG_INFO("Load config file, configNumber=%d:\n", i);

    pthread_mutex_init(&mShared.mMutex, NULL);

    //print current settings
    AMLOG_INFO("      disable audio? %d, disable video? %d, disable subtitle? %d, vout select %x, ppmod select 0x%x.\n", mShared.pbConfig.audio_disable, mShared.pbConfig.video_disable, mShared.pbConfig.subtitle_disable, mShared.pbConfig.vout_config, mShared.ppmode);
    AMLOG_INFO("      mPridataParsingMethod %d.\n", mShared.mPridataParsingMethod);
    for (i=0; i< CODEC_NUM; i++) {
        AMLOG_INFO("      video format %d, select dec type %d.\n", mShared.decSet[i].codecId, mShared.decSet[i].dectype);
    }
    AM_PrintLogConfig();
    AM_PrintPBDspConfig(&mShared);
    mpOpaque = (void*)&mShared;

#if PLATFORM_ANDROID
    //hard code: request disable osd on hdmi
#ifdef ENABLE_OSD_ON_HDMI
    mShared.dspConfig.voutConfigs.voutConfig[eVoutHDMI].osd_disable = 0;
#else
    mShared.dspConfig.voutConfigs.voutConfig[eVoutHDMI].osd_disable = 1;
#endif
#endif

    mRequestVoutMask = mShared.pbConfig.vout_config;
}

AM_ERR CActivePBEngine::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    DSetModuleLogConfig(LogModulePBEngine);

    if ((mpClockManager = CClockManager::Create()) == NULL)
        return ME_ERROR;

    AM_ASSERT(mpWorkQ);
    mpWorkQ->Run();
    return ME_OK;
}

CActivePBEngine::~CActivePBEngine()
{
    AM_PlayItem* tmp = mPlayListHead.pNext, *tmp1= NULL;
    AM_INFO(" ====== Destroy ActivePBEngine ====== \n");

    if (mpClockManager) {
        mpClockManager->StopClock();
        mpClockManager->SetSource(NULL);
    }
    __LOCK(mpMutex);
    while (tmp != &mPlayListHead) {
        tmp1 = tmp;
        tmp = tmp->pNext;
        AM_DeletePlayItem(tmp1);
    }
    mPlayListHead.pNext = mPlayListHead.pPre = &mPlayListHead;
    tmp = mToBePlayedItem.pNext;
    while (tmp != &mToBePlayedItem) {
        tmp1 = tmp;
        tmp = tmp->pNext;
        AM_DeletePlayItem(tmp1);
    }
    mToBePlayedItem.pNext = mToBePlayedItem.pPre = &mToBePlayedItem;
    tmp = mpFreeList;
    while (tmp) {
        tmp1 = tmp;
        tmp = tmp->pNext;
        AM_DeletePlayItem(tmp1);
    }
    mpFreeList = NULL;
    __UNLOCK(mpMutex);
    AM_DELETE(mpRecognizer);
    AM_DELETE(mpClockManager);
    mpOpaque = NULL;

    if (mpPrivateDataBufferForCallback) {
        free(mpPrivateDataBufferForCallback);
        mpPrivateDataBufferForCallback = NULL;
    }
    pthread_mutex_destroy(&mShared.mMutex);
    AM_INFO(" ====== Destroy ActivePBEngine done ====== \n");
}

void CActivePBEngine::Delete()
{
    AM_PlayItem* tmp = mPlayListHead.pNext, *tmp1= NULL;
    if (mpWorkQ) {
        mpWorkQ->SendCmd(CMD_STOP);
    }
    if (mpClockManager) {
        mpClockManager->StopClock();
        mpClockManager->SetSource(NULL);
    }

    __LOCK(mpMutex);
    while (tmp != &mPlayListHead) {
        tmp1 = tmp;
        tmp = tmp->pNext;
        AM_DeletePlayItem(tmp1);
    }
    mPlayListHead.pNext = mPlayListHead.pPre = &mPlayListHead;
    tmp = mToBePlayedItem.pNext;
    while (tmp != &mToBePlayedItem) {
        tmp1 = tmp;
        tmp = tmp->pNext;
        AM_DeletePlayItem(tmp1);
    }
    mToBePlayedItem.pNext = mToBePlayedItem.pPre = &mToBePlayedItem;
    tmp = mpFreeList;
    while (tmp) {
        tmp1 = tmp;
        tmp = tmp->pNext;
        AM_DeletePlayItem(tmp1);
    }
    mpFreeList = NULL;
    __UNLOCK(mpMutex);
    AM_DELETE(mpRecognizer);
    mpRecognizer = NULL;
    AM_DELETE(mpClockManager);
    mpClockManager = NULL;
    inherited::Delete();
}

void *CActivePBEngine::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IPBControl)
        return (IPBControl*)this;
    if (refiid == IID_IPBEngine)
        return (IPBEngine*)this;
    if (refiid == IID_IDataRetriever)
        return (IDataRetriever*)this;
    return inherited::GetInterface(refiid);
}

void *CActivePBEngine::QueryEngineInterface(AM_REFIID refiid)
{
    if (refiid == IID_IClockManager)
        return mpClockManager;

    if (refiid == IID_IAudioSink)
        return mpAudioSink;

    return NULL;
}

AM_ERR CActivePBEngine::CreateRecognizer()
{
    if (mpRecognizer)
        return ME_OK;

    if ((mpRecognizer = CMediaRecognizer::Create((IEngine*)(inherited*)this)) == NULL)
        return ME_NO_MEMORY;

    return ME_OK;
}

/*A5S DV Playback Test */
AM_ERR  CActivePBEngine::A5SPlayMode(AM_UINT mode)
{
    if(mode == FB_1X_SPEED || mOldMode == FB_1X_SPEED)
    {
        FlushAllFilters();
        PurgeAllFilters();
        AMLOG_INFO("FlushDone!\n\n");
    }
    mpDemuxer->A5SPlayMode(mode);
    if(mode == FB_1X_SPEED || mOldMode == FB_1X_SPEED)
        AllFiltersMsg(IActiveObject::CMD_BEGIN_PLAYBACK);
    mOldMode = mode;
    return ME_OK;
}

AM_ERR  CActivePBEngine::A5SPlayNM(AM_INT start_n, AM_INT end_m)
{
    FlushAllFilters();
    PurgeAllFilters();

    mpDemuxer->A5SPlayNM(start_n, end_m);
    AllFiltersMsg(IActiveObject::CMD_BEGIN_PLAYBACK);
    mOldMode = PLAY_N_TO_M;
    return ME_OK;
}
/*end A5S DV Test*/

AM_ERR CActivePBEngine::PrepareFile(const char *pFileName)
{
    CMD cmd;
    AM_PlayItem* item = NULL;
    char* filename = (char*)pFileName;
    AM_ASSERT(mpCmdQueue);

    AM_ERR err = CreateRecognizer();
    if (err != ME_OK){
        AMLOG_ERROR("CreateRecognizer fail, ret %d.\n", err);
        return err;
    }

    if (mShared.use_force_play_item) {
        filename = mShared.force_play_url;
        AMLOG_WARN("!!!force play specified file!!! %s.\n", filename);
    }

    //strncpy(mFileName, pFileName, sizeof(mFileName) - 1);	// todo
    err = mpRecognizer->RecognizeFile(filename, true);
    if (err != ME_OK) {
        AMLOG_ERROR("RecognizeFile %s fail, ret %d.\n", filename, err);
        return err;
    }

    item = RequestNewPlayItem();
    AM_FillPlayItem(item, (char *)filename, PlayItemType_LocalFile);
    cmd.code = PBCMD_PREPARE;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    cmd.pExtra = (void*) item;
    item->pSourceFilter = (void*)mpRecognizer->GetFilterEntry();
    item->pSourceFilterContext = mpRecognizer->GetParser().context;
    mpRecognizer->ClearParser();
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    if (err != ME_OK) {
        AMLOG_ERROR("PBCMD_PREPARE failure, ret %d.\n", err);
        return err;
    }
    return ME_OK;
}

AM_ERR CActivePBEngine::PlayFile(const char *pFileName)
{
    CMD cmd;
    AM_ASSERT(mpCmdQueue);
    cmd.code = PBCMD_START_PLAY;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    // will wait until file is really started
    mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return ME_OK;
}

AM_ERR CActivePBEngine::StopPlay()
{
    AM_ASSERT(mpCmdQueue);
    CMD cmd;
    cmd.code = PBCMD_STOP_PLAY;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return ME_OK;
}

void CActivePBEngine::StopEngine(AM_UINT stop_flag)
{
//#ifdef _debug_old_flow_
#if 1//STOP(1) should always sent to DSP, no depend on flush cmd flow
    AMLOG_INFO("CActivePBEngine::StopEngine, 1\n");
    AM_INT udec_stoped = 0;

    if (mShared.mbIavInited && (mShared.mIavFd>=0)) {
        //tmp fix, only for hybird rv40:
        //stop udec interrupt a transaction(hand shake) between arm and dsp, decoding would have some problem,
        //hybird rv40: arm(decode) --->dsp--->arm(wait decode done), will use new protocol in future, new protocol would remove this break atomic transaction issue
        if (!((mShared.decCurrType == DEC_HYBRID) && (mShared.get_outpic == 1)))
        {
            StopUDEC(mShared.mIavFd, 0, stop_flag);
            udec_stoped = 1;
        }
    }

    AMLOG_INFO("CActivePBEngine::StopEngine, 2\n");
    //mpClockManager->PauseClock();
    FlushAllFilters();
    AMLOG_INFO("CActivePBEngine::StopEngine, 3\n");
    PurgeAllFilters();
    AMLOG_INFO("CActivePBEngine::StopEngine, 4\n");
    if (mShared.mbIavInited && (mShared.mIavFd>=0)) {
        if (udec_stoped == 0) {
            StopUDEC(mShared.mIavFd, 0, stop_flag);
        }
    }
    AMLOG_INFO("CActivePBEngine::StopEngine, 5\n");
#else
    AMLOG_INFO("CActivePBEngine::StopEngine, before FlushAllFilters.\n");
    FlushAllFilters();
    AMLOG_INFO("CActivePBEngine::StopEngine, before PurgeAllFilters.\n");
    PurgeAllFilters();
    AMLOG_INFO("CActivePBEngine::StopEngine done.\n");
#endif
}

AM_ERR CActivePBEngine::GetPBInfo(PBINFO& info)
{
    info.state = mState;
    info.position = 0;
    info.dir = mDir;
    info.speed = mSpeed;
    info.length = mCurrentItemTotLength;

    if (0 == info.length) {
        info.position = 0;
        return ME_OK;
    }

    if (mbSeeking) {
        info.position = mSeekTime;
        AMLOG_PTS("CActivePBEngine::GetPBInfo is seeking, info.position = %llu, info.length= %llu, mState %d.\n", info.position, info.length, mState);
        return ME_OK;
    }

    if (mState == STATE_COMPLETED || mState == STATE_STOPPED || mState == STATE_END) {
        info.position = mCurrentItemTotLength;
    } else if (STATE_STARTED== mState ||STATE_PAUSED == mState || STATE_PREPARED ==mState || STATE_STEP == mState) {
        AM_U64 absoluteTimeMs = 0, relativeTimeMs = 0;
        if (mpMasterRenderer) {
            mpMasterRenderer->GetCurrentTime(absoluteTimeMs, relativeTimeMs);
        } else {
            AMLOG_WARN("NO master renderer's case, todo.\n");
            absoluteTimeMs = relativeTimeMs = mpClockManager->GetCurrentTime();
        }
        AMLOG_PTS("CActivePBEngine::GetPBInfo absoluteTimeMs = %llu, relativeTimeMs = %llu, mState %d.\n", absoluteTimeMs, relativeTimeMs, mState);

        // adopt two different methods to calculate current time
        // 1. normal  case = (lastPts - firstPts) + delta, which is done by master renderer
        // 2. special case = seek time + delta
        // note:
        // delta represents the duration, in which master renderer have played samples or frames normally
        if (mbUseAbsoluteTime && !mShared.mbSeekFailed) {
           info.position = absoluteTimeMs;
        } else {
           info.position = mSeekTime + relativeTimeMs;
        }

        // fix bug2330: assure time moves forward
        if (mbCurTimeValid) {
            if (info.position >= mCurTime) {
                mCurTime = info.position;
            } else {
                info.position = mCurTime;
            }
        } else {
            mCurTime = info.position;
            mbCurTimeValid = true;
        }

        if (info.position > info.length) {
            AM_ERROR("why info.position %llu > info.length %llu\n", info.position, info.length);
            info.position = info.length;
        }
    } else if (mState == STATE_ERROR) {
        AM_ERROR("CActivePBEngine::GetPBInfo error state %d.\n", mState);
        info.position = mCurrentItemTotLength;
        return ME_ERROR;
    } else {
        AM_ERROR("CActivePBEngine::GetPBInfo unknown state %d.\n", mState);
        info.position = 0;
        return ME_ERROR;
    }

    AMLOG_PTS("CActivePBEngine::GetPBInfo info.position = %llu, info.length= %llu, mState %d.\n", info.position, info.length, mState);
    return ME_OK;
}

AM_ERR CActivePBEngine::PausePlay()
{
    AM_ASSERT(mpCmdQueue);
    CMD cmd;
    cmd.code = PBCMD_PAUSE_RESUME;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    cmd.flag = 1;//pause
    mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return ME_OK;
}

AM_ERR CActivePBEngine::ResumePlay()
{
    AM_ASSERT(mpCmdQueue);
    CMD cmd;
    cmd.code = PBCMD_PAUSE_RESUME;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    cmd.flag = 0;//resume
    mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return ME_OK;
}

AM_ERR CActivePBEngine::Loopplay(AM_U32 loop_mode)
{
    mShared.mbLoopPlay = loop_mode;
    return ME_OK;
}

AM_ERR CActivePBEngine::Seek(AM_U64 ms)
{
    AM_ASSERT(mpCmdQueue);
    if (isCmdQueueFull()) {
        AMLOG_WARN("pb-engine's cmd queue is full now, please send seek not so frequently.\n");
        return ME_OK;
    }
    CMD cmd;
    cmd.code = PBCMD_SEEK;

#ifdef _debug_use_stable_strategy_for_seek_
    cmd.repeatType = CMD_TYPE_SINGLETON;
    cmd.res64_1 = ms;
    mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
#else
    cmd.repeatType = CMD_TYPE_REPEAT_AVATOR;
    cmd.res64_1 = ms;
    mpCmdQueue->PostMsg(&cmd, sizeof(cmd));
#endif

    return ME_OK;
}

AM_ERR CActivePBEngine::Step()
{
    if (!mpVoutController) {
        AMLOG_ERROR("No voutcontroller, cannot switch to setp mode.\n");
        return ME_ERROR;
    }

    if (STATE_STARTED == mState) {
        AMLOG_INFO("switch into STEP mode.\n");
        SetState(STATE_STEP);
    } else if (mState != STATE_STEP) {
        AMLOG_ERROR("cannot switch to setp mode in state %d.\n", mState);
        return ME_ERROR;
    }

    mpVoutController->Step();
    return ME_OK;
}

AM_ERR CActivePBEngine::StartwithStepMode(AM_UINT startCnt)
{
    mShared.mbStartWithStepMode = 1;
    mShared.mStepCnt = startCnt;
    AMLOG_INFO("pb-engine, start with step mode, cnt = %d.\n", startCnt);
    return ME_OK;
}

AM_ERR CActivePBEngine::StartwithPlayVideoESMode(AM_INT also_demuxfile, AM_INT param_0, AM_INT param_1)
{
    mShared.also_demux_file = also_demuxfile;
    mShared.play_video_es = 1;
    return ME_OK;
}

AM_ERR CActivePBEngine::SpecifyVideoFrameRate(AM_UINT framerate_num, AM_UINT framerate_den)
{
    mShared.specified_framerate_num = framerate_num;
    mShared.specified_framerate_den = framerate_den;
    return ME_OK;
}

AM_ERR CActivePBEngine::SetTrickMode(PB_DIR dir, PB_SPEED speed)
{
    return ME_OK;
}

AM_ERR CActivePBEngine::SetDeinterlaceMode(AM_INT bEnable)
{
    mShared.bEnableDeinterlace = (AM_BOOL)bEnable;
    return ME_OK;
}

AM_ERR CActivePBEngine::GetDeinterlaceMode(AM_INT *pbEnable)
{
    *pbEnable = (AM_INT)mShared.bEnableDeinterlace;
    return ME_OK;
}

AM_ERR CActivePBEngine::SetLooping(int loop)
{
    mLoopCurrentItem = loop;
    return ME_OK;
}

void CActivePBEngine::ClearCurrentSession()
{
    AMLOG_INFO("ClearCurrentSession start, purge cmd in queue\n");
    PurgeMsgInQueue();

    //clear shared data
    mShared.mbVideoFirstPTSVaild = false;
    mShared.mbAudioFirstPTSVaild = false;
    mShared.videoFirstPTS = 0;
    mShared.audioFirstPTS = 0;
    mShared.mbAlreadySynced = 0;
    AllFiltersClearFlag(EOS_FLAG | READY_FLAG | SYNC_FLAG);

    if (mShared.mbIavInited && (mShared.mIavFd>=0)) {
        AMLOG_INFO("ClearCurrentSession, purge driver\n");
        StopUDEC(mShared.mIavFd, 0, STOPFLAG_CLEAR);
        GetUdecState(mShared.mIavFd, &mShared.udec_state, &mShared.vout_state, &mShared.error_code);
        AMLOG_INFO("ClearCurrentSession, end, udec_state %d, vout_state %d, error_code %d.\n", mShared.udec_state, mShared.vout_state, mShared.error_code);
    }
}

AM_ERR CActivePBEngine::HandleSeek(AM_U64 ms)
{
    AM_ERR err = ME_OK;
//    AM_UINT i = 0;
//    AM_U64 length = 0;

    AMLOG_INFO("HandleSeek start\n");

    if(ms >= mCurrentItemTotLength) {
        AMLOG_ERROR("seek time bigger than length, no seek. ms=%lld,length=%lld.\n", ms,mCurrentItemTotLength);
        return ME_ERROR;
    }

    AMLOG_INFO("HandleSeek StopEngine start\n");
    mbSeeking = true;

    //flush clock
    AM_ASSERT(mpClockManager);
    mpClockManager->PurgeClock();

    //flush engine
    StopEngine(STOPFLAG_FLUSH);

    ClearCurrentSession();

    NewSession();

    err = mpDemuxer->Seek(ms);
    if (err == ME_OK) {
        mSeekTime = ms;
        mShared.mPlaybackStartTime = 90 * mSeekTime;
        mbCurTimeValid = false;
    } else {
        AMLOG_ERROR("seek error %d.\n", err);
    }

    AllFiltersMsg(IActiveObject::CMD_BEGIN_PLAYBACK);

    return err;
}

void CActivePBEngine::PurgeMsgInQueue()
{
    AM_MSG msg;
    AM_ASSERT(mpFilterMsgQ);
    if (mpFilterMsgQ) {
        while (mpFilterMsgQ->PeekData((void*)&msg, sizeof(msg))) {
            AMLOG_INFO("discard msg %d, after flush engine.\n", msg.code);
        }
    }
}

void CActivePBEngine::HandleSyncMsg(AM_MSG& msg)
{
    IFilter *pFilter = (IFilter*)msg.p1;

    AM_ASSERT(pFilter != NULL);

    AM_UINT index;
    if (FindFilter(pFilter, index) != ME_OK) {
        AMLOG_ERROR("engine corrupt?.\n");
        return;
    }

    //already synced(must be force synced),
    if (mShared.mbAlreadySynced) {
        mFilters[index].pFilter->SendCmd(IActiveObject::CMD_AVSYNC);
        return;
    }

    AMLOG_INFO("**sync msg index %d.\n", index);

    mFilters[index].flags |= SYNC_FLAG;
    if (AllFiltersHasFlag(SYNC_FLAG)) {
        AMLOG_INFO("---All filters sync ready----\n");
        mbSeeking = false;
        AllFiltersClearFlag(SYNC_FLAG);
        mShared.mbAlreadySynced = 1;

        //start play clock
        if (mShared.videoEnabled && mShared.audioEnabled) {
            AM_ASSERT(mShared.mbVideoFirstPTSVaild);
            AM_ASSERT(mShared.mbAudioFirstPTSVaild);
            if (mShared.mbVideoFirstPTSVaild && mShared.mbAudioFirstPTSVaild) {
                AMLOG_INFO("[AV sync flow]: from demuxer, start video pts %llu, start audio pts %llu.\n", mShared.videoFirstPTS, mShared.audioFirstPTS);
                if (mShared.videoFirstPTS < mShared.audioFirstPTS) {
                    AMLOG_INFO("video pts is less than audio, set pts start point as video pts %llu, start playback... diff is %lld.\n", mShared.videoFirstPTS, mShared.audioFirstPTS - mShared.videoFirstPTS);
                    mpClockManager->SetClockMgr(mShared.videoFirstPTS);
                } else {
                    AMLOG_INFO("audio pts is less than video, set pts start point as audio pts %llu, start playback... diff is %lld.\n", mShared.audioFirstPTS, mShared.videoFirstPTS - mShared.audioFirstPTS);
                    mpClockManager->SetClockMgr(mShared.audioFirstPTS);
                }
            } else if (mShared.mbVideoFirstPTSVaild) {
                AMLOG_WARN("[AV sync flow]: NO valid audio start pts from demuxer, please check code, video start pts %llu.\n", mShared.videoFirstPTS);
                mpClockManager->SetClockMgr(mShared.videoFirstPTS);
            } else if (mShared.mbAudioFirstPTSVaild) {
                AMLOG_WARN("[AV sync flow]: NO valid video start pts from demuxer, please check code, audio start pts %llu.\n", mShared.audioFirstPTS);
                mpClockManager->SetClockMgr(mShared.audioFirstPTS);
            } else {
                AM_ERROR("NO valid audio and video start pts, must not comes here, please check code.\n");
                mpClockManager->SetClockMgr(0);
            }
        }else if (mShared.videoEnabled) {
            if (mShared.mbVideoFirstPTSVaild) {
                AMLOG_WARN("[AV sync flow]: no audio, video start pts %llu.\n", mShared.videoFirstPTS);
                mpClockManager->SetClockMgr(mShared.videoFirstPTS);
            } else {
                AM_ERROR("NO valid video start pts, must not comes here, please check code.\n");
                mpClockManager->SetClockMgr(0);
            }
        } else if (mShared.audioEnabled) {
            if (mShared.mbAudioFirstPTSVaild) {
                AMLOG_WARN("[AV sync flow]: no video, audio start pts %llu.\n", mShared.audioFirstPTS);
                mpClockManager->SetClockMgr(mShared.audioFirstPTS);
            } else {
                AM_ERROR("NO valid audio start pts, must not comes here, please check code.\n");
                mpClockManager->SetClockMgr(0);
            }
        } else {
            AM_ERROR("both audio and video are disabled?\n");
            mpClockManager->SetClockMgr(0);
        }

        //send cmd to filters, start playback
        AllFiltersCmd(IActiveObject::CMD_AVSYNC);

        if (mShared.auto_purge_buffers_before_play_rtsp) {
            const char* pFileType = NULL;
            mpDemuxer->GetFileType(pFileType);
            if (pFileType && (!strncmp(pFileType, "rtsp", 4))) {
                AMLOG_INFO("RTSP: auto speedup...\n");
                AllFiltersMsg(IActiveObject::CMD_REALTIME_SPEEDUP);
            }
        }
    }
}

void CActivePBEngine::HandleNotifyUDECIsRunning() {
    AllFiltersMsg(IActiveObject::CMD_UDEC_IN_RUNNING_STATE);
}

void CActivePBEngine::HandleSourceFilterBlockedMsg(AM_MSG& msg) {
    AllFiltersMsg(IActiveObject::CMD_SOURCE_FILTER_BLOCKED);
}

void CActivePBEngine::HandleErrorMsg(AM_MSG& msg) {
//    AM_ERR err;
    AM_UINT e_code = mShared.error_code;
    AM_UINT mw_behavior;

    if ((AM_ERR)msg.p0 == ME_VIDEO_DATA_ERROR) {
        AMLOG_INFO("[ErrorHandling]: disable video\n");
        //mpDemuxer->EnableVideo(false);
        HandleSourceFilterBlockedMsg(msg);
    } else if ((AM_ERR)msg.p0 == ME_UDEC_ERROR) {
        AMLOG_INFO("[ErrorHandling]: start handle udec error, error_code 0x%x.\n", e_code);
        mw_behavior = AnalyseUdecErrorCode(e_code, mShared.dspConfig.errorHandlingConfig[0].error_handling_app_behavior);

        if (mw_behavior == MW_Bahavior_Ignore) {
            //just ignore it
            AMLOG_INFO("[ErrorHandling]: done, ignore error_code 0x%x.\n", e_code);
        } else if (mw_behavior == MW_Bahavior_Ignore_AndPostAppMsg) {
            //need notify app, udec encounter error
            AMLOG_INFO("[ErrorHandling]: done, post warning msg to app. error_code 0x%x.\n", e_code);
        } else if (mw_behavior == MW_Bahavior_TrySeekToSkipError) {

            mFatalErrorCnt ++;
            mTrySeekTimeGap += mFatalErrorCnt*2000;

            //try seek to skip the error place
            AM_ASSERT(mpMasterRenderer);
            AM_U64 absoluteTimeMs = 0, relativeTimeMs = 0;
            mpMasterRenderer->GetCurrentTime(absoluteTimeMs, relativeTimeMs);
            //flush all
            AMLOG_INFO("[ErrorHandling]: get current time %llu, start flush active_pb_engine.\n", absoluteTimeMs);
            //mpClockManager->PauseClock();

            mSeekTime = absoluteTimeMs;
            mShared.mPlaybackStartTime = absoluteTimeMs*90;
            mbSeeking = true;

            //purge clock
            mpClockManager->PurgeClock();

            //flush engine
            StopEngine(STOPFLAG_FLUSH);
            AMLOG_INFO("[ErrorHandling]: flush done.\n");

            ClearCurrentSession();
            NewSession();
            /*
            absoluteTimeMs += mTrySeekTimeGap;
            if(mCurrentItemTotLength != 0 && absoluteTimeMs >= mCurrentItemTotLength){
                AMLOG_ERROR("[ErrorHandling]: seek time [%llu] is larger than duration [%llu].\n",absoluteTimeMs,mCurrentItemTotLength);
                SetState(STATE_ERROR);
                return;
            }
            //try skip error place
            AMLOG_INFO("[ErrorHandling]: flush done, seek to time %llu.\n", absoluteTimeMs);

            // ddr 0927: won't seek if err happens
            // 1. err data is passed to DSP already, no need to skip following data. DSP should be able to handle non-key frame case
            // 2. for realtime streaming case, there's no data for seek
            err = mpDemuxer->Seek(absoluteTimeMs);
            if (err != ME_OK) {
                AMLOG_ERROR("[ErrorHandling]: seek error, pb-engine go to error state.\n");
                SetState(STATE_ERROR);
                return;
            }

            //restart playing
            AMLOG_INFO("[ErrorHandling]: seek to time %llu done.\n", absoluteTimeMs);
            mSeekTime = absoluteTimeMs;
            */
            //mpClockManager->ResumeClock();
            AllFiltersMsg(IActiveObject::CMD_BEGIN_PLAYBACK);
            AMLOG_INFO("[ErrorHandling]: begin next playback.\n");
            SetState(STATE_STARTED);
        } else if (mw_behavior == MW_Bahavior_ExitPlayback) {
            AMLOG_INFO("[ErrorHandling]: Stop current playback.\n");
            StopEngine(STOPFLAG_STOP);
            ClearCurrentSession();
            NewSession();
            SetState(STATE_COMPLETED);
            PostAppMsg(IMediaControl :: MSG_PLAYER_EOS);
            AMLOG_INFO("[ErrorHandling]: Stop done.\n");
        } else if (mw_behavior == MW_Bahavior_HaltForDebug) {
            AMLOG_WARN("[ErrorHandling]: Just Halt, do not trigger error handling flow, for debug...\n");
        } else {
            AM_ASSERT(0);
            AM_ERROR("Must not comes here.\n");
        }
        AMLOG_INFO("[ErrorHandling]: handle udec error done.\n");
    }
}

void CActivePBEngine::HandleReadyMsg(AM_MSG& msg)
{
    IFilter *pFilter = (IFilter*)msg.p1;
    AM_UINT i;

    AM_ASSERT(pFilter != NULL);

    AM_UINT index;
    if (FindFilter(pFilter, index) != ME_OK)
        return;

    AMLOG_INFO("before flag %d, 0x%x.\n", index, mFilters[index].flags);
    SetReady(index);
    AMLOG_INFO("after flag %d, 0x%x.\n", index, mFilters[index].flags);
    if (AllRenderersReady()) {
        AMLOG_INFO("---All renderers ready----\n");
        mpClockManager->StartClock();
        StartAllRenderers();
        SetState(STATE_STARTED);//for bug#2308 case 3: all format decoders, quick seek at file beginning, "CVideoRenderer::requestRun" blocked issue
        AM_ASSERT(mState == STATE_STARTED);
        if (mbPaused) {
            AMLOG_INFO("PauseAllFilters\n");
            for(i=0; i<mnFilters; i++) {
                mFilters[i].pFilter->Pause();
            }
            SetState(STATE_PAUSED);
        }
    }
}

void CActivePBEngine::HandleEOSMsg(AM_MSG& msg)
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

        mFatalErrorCnt = 0;
        mTrySeekTimeGap = 2000;

        if(mLoopCurrentItem)
        {
            //repeat play current item
            AMLOG_INFO("[PlayList]: repeat play current item~\n");
            //seek to 0
            HandleSeek(0);
        } else if (mLoopPlayList) {
            if (TryPlayNextItem() == false) {
                //play next item fail
                AMLOG_WARN("should have a accident, no items canbe played now, should report a error to app, or just notify a EOS(COMPLETED)?\n");
                if(mNeedReConfig == 0){
                    SetState(STATE_COMPLETED);
                    PostAppMsg(IMediaControl :: MSG_PLAYER_EOS);
                }else{
                    AMLOG_ERROR("mNeedReConfig %d, need to re-config decoder?\n",mNeedReConfig);
                }
            }
        } else {
            AMLOG_INFO("Sending eos to app start.\n");
            if(mNeedReConfig == 0){
                SetState(STATE_COMPLETED);
                PostAppMsg(IMediaControl :: MSG_PLAYER_EOS);
            }else{
                AMLOG_ERROR("mNeedReConfig %d, need to re-config decoder?\n",mNeedReConfig);
            }
            AMLOG_INFO("Sending eos to app done.\n");
        }
    } else {
        // if audio renderer eos, switch video as master
        // if video renderer eos, switch audio as master

        IFilter::INFO info;

        // if renderer to set is master now, do nothing
        pFilter->GetInfo(info);
        if (!strcmp(info.pName, "AudioRenderer")) {
            SetMasterRender("VideoRenderer");
        } else if (!strcmp(info.pName, "VideoRenderer")) {
            SetMasterRender("AudioRenderer");
        } else {
            AM_ERROR("HandleEOSMsg: can't switch master renderer\n");
        }
    }
}

void CActivePBEngine::HandleSwitchMasterRenderMsg(AM_MSG& msg)//for bug1294
{
    switch (msg.p0) {
        case 0:             // Audio Renderer As master
            SetMasterRender("AudioRenderer");
            break;
        case 1:             // Video Renderer As master
            SetMasterRender("VideoRenderer");
            break;
        default:
            AM_ERROR("HandleSwitchMasterRenderMsg: invalid parameters\n");
            break;
    }
}

void CActivePBEngine::SetMasterRender(const char *pFilterName)
{
    IFilter::INFO info;

    // if renderer to set is master now, do nothing
    AM_ASSERT(mpMasterFilter);
    mpMasterFilter->GetInfo(info);
    if (!strcmp(info.pName, pFilterName)) {
        AMLOG_INFO("SetMasterRender: current master is %s indeed, needn't do nothing.\n", pFilterName);
        return ;
    }

    // find filter according to filter name
    IFilter *pFilter = NULL;
    for (AM_UINT i = 0; i < mnFilters; i++) {
        //mFilters[i].pFilter->SetMaster(false);
        mFilters[i].pFilter->GetInfo(info);
        if (!strcmp(info.pName, pFilterName)) {
              pFilter = mFilters[i].pFilter;
              break;
        }
    }

    if (pFilter) {
        mpMasterFilter->SetMaster(false);
        mpMasterFilter = pFilter;
        mpMasterFilter->SetMaster(true);
        mpMasterRenderer = IRender::GetInterfaceFrom(mpMasterFilter);
        AMLOG_INFO("SetMasterRender: switch %s as master render done.\n", pFilterName);
    } else {
        AM_ERROR("SetMasterRender: can't find filter %s\n", pFilterName);
    }
    AM_ASSERT(mpMasterFilter);
    AM_ASSERT(mpMasterRenderer);
}

void CActivePBEngine::CheckFileType(const char *pFileType)
{
    // default
    mbUseAbsoluteTime = true;

    if (!pFileType) {
        return ;
    }

    // rtsp protocol
    if (!strncmp(pFileType, "rtsp", 4)) {
        mbUseAbsoluteTime = false;
        AMLOG_INFO("CActivePBEngine::CheckFileType: rtsp protocol.\n");
        return ;
    }

    // flac type
    if (!strncmp(pFileType, "flac", 4)) {
        mbUseAbsoluteTime = false;
        AMLOG_INFO("CActivePBEngine::CheckFileType: flac type.\n");
        return ;
    }

    // bitstream in which seek by byte
    if (mShared.mbDurationEstimated == AM_TRUE) {
        mbUseAbsoluteTime = false;
        AMLOG_INFO("CActivePBEngine::CheckFileType: bitstream in which seek by byte.\n");
        return ;
    }
}

AM_ERR CActivePBEngine::RetieveData(AM_U8* pdata, AM_UINT max_len, IParameters::DataCategory data_category)
{
    AM_ASSERT(IParameters::DataCategory_PrivateData == data_category);
    if (IParameters::DataCategory_PrivateData == data_category && mpDataRetriever) {
        return mpDataRetriever->RetieveData(pdata, max_len, data_category);
    }

    //bad param or not supported
    if (!mpDataRetriever) {
        AM_ERROR("NULL mpDataRetriever.\n");
        return ME_ERROR;
    }

    AM_ERROR("NOT supported data_category %d.\n", data_category);
    return ME_BAD_PARAM;
}

AM_ERR CActivePBEngine::RetieveDataByType(AM_U8* pdata, AM_UINT max_len, AM_U16 type, AM_U16 sub_type, IParameters::DataCategory data_category)
{
    AM_ASSERT(IParameters::DataCategory_PrivateData == data_category);
    if (IParameters::DataCategory_PrivateData == data_category && mpDataRetriever) {
        return mpDataRetriever->RetieveDataByType(pdata, max_len, type, sub_type, data_category);
    }

    //bad param or not supported
    if (!mpDataRetriever) {
        AM_ERROR("NULL mpDataRetriever.\n");
        return ME_ERROR;
    }

    AM_ERROR("NOT supported data_category %d.\n", data_category);
    return ME_BAD_PARAM;
}

AM_ERR CActivePBEngine::RenderFilter(IFilter *pFilter)
{
    AMLOG_INFO("Render %s\n", GetFilterName(pFilter));

    IFilter::INFO info;
    pFilter->GetInfo(info);

    //find vout controller
    if (!mpVoutController) {
        mpVoutController = IVideoOutput::GetInterfaceFrom(pFilter);
    }

    //find video decoder controller filter
    if(!mpDecodeController){
        mpDecodeController = IDecoderControl::GetInterfaceFrom(pFilter);
    }

    //find data retriever for pridata
    if(!mpDataRetriever){
        mpDataRetriever = IDataRetriever::GetInterfaceFrom(pFilter);
    }

    if (info.nOutput == 0) {
        // this is a renderer
        //IClockSource *pClockSource = IClockSource::GetInterfaceFrom(pFilter);
        //if (pClockSource) {
        //	mpClockManager->SetSource(pClockSource);
        //}
        //find master renderer
        if (mMostPriority < info.mPriority) {
            mpMasterFilter = pFilter;
            mMostPriority = info.mPriority;
        }

        AMLOG_INFO("Renderer (no output pins): %s\n", GetFilterName(pFilter));
        return ME_OK;
    }

    for (AM_UINT i = 0; i < info.nOutput; i++) {
        // get one pin
        IPin *pOutput = pFilter->GetOutputPin(i);
        if (pOutput == NULL) {
            AMLOG_ERROR("%s has no output pin %d\n", GetFilterName(pFilter), i);
            return ME_NO_INTERFACE;
        }
        // query the media format on the pin
        CMediaFormat *pMediaFormat = NULL;
        AM_ERR err = pOutput->GetMediaFormat(pMediaFormat);
        if (err != ME_OK) {
            AMLOG_ERROR("%s[%d]::GetMediaFormat failed\n", GetFilterName(pFilter), i);
            return err;
        }

        // find a filter entry, which declares it can accept the media
        filter_entry *pf = CMediaRecognizer::FindMatch(*pMediaFormat);
        if (pf == NULL) {
            AMLOG_ERROR("Cannot find filter to render %s[%d]\n", GetFilterName(pFilter), i);
            return ME_NO_INTERFACE;
        }

        // create the filter
        IFilter *pNextFilter = pf->create((IEngine*)(inherited*)this);
        if (pNextFilter == NULL) {
            AMLOG_ERROR("Cannot create filter %s\n", pf->pName);
            return ME_NOT_EXIST;
        }

        // get the first input pin
        IPin *pInput = pNextFilter->GetInputPin(0);
        if (pInput == NULL) {
            pNextFilter->Delete();
            pNextFilter = NULL;
            AMLOG_ERROR("Filter %s has no input pin\n", GetFilterName(pNextFilter));
            return ME_NOT_EXIST;
        }

        // connect with it
        AMLOG_INFO("try to connect %s with %s\n", GetFilterName(pFilter), GetFilterName(pNextFilter));
        err = CreateConnection(pOutput, pInput);
        if (err != ME_OK) {
            AMLOG_ERROR("connect fail, try new filter, maybe condition(preferWorkMode %d) is too strict.\n", pMediaFormat->preferWorkMode);
            if (PreferWorkMode_Duplex == pMediaFormat->preferWorkMode) {
                AM_ASSERT(DSPMode_DuplexLowdelay == mShared.encoding_mode_config.dsp_mode);
                pMediaFormat->preferWorkMode = PreferWorkMode_UDEC;
            } else if (PreferWorkMode_UDEC == pMediaFormat->preferWorkMode) {
                AM_ASSERT(DSPMode_UDEC == mShared.encoding_mode_config.dsp_mode);
                pMediaFormat->preferWorkMode = PreferWorkMode_Duplex;
            } else if (PreferWorkMode_None == pMediaFormat->preferWorkMode) {
                //need guess next mode
                if (DSPMode_DuplexLowdelay == mShared.encoding_mode_config.dsp_mode) {
                    AMLOG_WARN("[dsp mode]: guess next is UDEC mode, try UDEC mode.\n");
                    pMediaFormat->preferWorkMode = PreferWorkMode_UDEC;
                    mShared.encoding_mode_config.dsp_mode = DSPMode_UDEC;
                } else {
                    AMLOG_WARN("[dsp mode]: guess next is Duplex mode, try Duplex mode.\n");
                    pMediaFormat->preferWorkMode = PreferWorkMode_Duplex;
                    mShared.encoding_mode_config.dsp_mode = DSPMode_DuplexLowdelay;
                }
            }
            AMLOG_INFO("[dsp mode]: pMediaFormat->preferWorkMode %d, %p.\n", pMediaFormat->preferWorkMode, pMediaFormat);

            pNextFilter->Delete();
            pNextFilter = NULL;

            //try find next filter
            pf = CMediaRecognizer::FindNextMatch(*pMediaFormat, pf);
            if (!pf) {
                //not found, exit
                AMLOG_ERROR("cannot find next filter.\n");
                return err;
            } else {
                //try new filter
                pNextFilter = pf->create((IEngine*)(inherited*)this);
                if (pNextFilter == NULL) {
                    AMLOG_ERROR("Cannot create filter %s\n", pf->pName);
                    return ME_NOT_EXIST;
                }

                // get the first input pin
                pInput = pNextFilter->GetInputPin(0);
                if (pInput == NULL) {
                    pNextFilter->Delete();
                    pNextFilter = NULL;
                    AMLOG_ERROR("Filter %s has no input pin\n", GetFilterName(pNextFilter));
                    return ME_NOT_EXIST;
                }

                // connect with it
                AMLOG_INFO("try to connect %s with %s\n", GetFilterName(pFilter), GetFilterName(pNextFilter));
                err = CreateConnection(pOutput, pInput);
                if (err != ME_OK) {
                    AMLOG_ERROR("connect fail, try new filter.\n");
                    pNextFilter->Delete();
                    pNextFilter = NULL;
                    return err;
                }
            }
        }
        AMLOG_INFO("connected\n");

        // save the filter
        err = AddFilter(pNextFilter);
        if (err != ME_OK) {
            AMLOG_ERROR("Too many filter %s, tot %d.\n", pf->pName, mnFilters);
            return ME_TOO_MANY;
        }

        // render the next filter
        err = RenderFilter(pNextFilter);
        if (err != ME_OK) {
            AMLOG_ERROR("render filter fail.\n");
            return err;
        }
    }

    return ME_OK;
}

AM_ERR CActivePBEngine::ChangeInputCenter(AM_INT input_center_x, AM_INT input_center_y)
{
    if (mpVoutController) {
        return mpVoutController->ChangeInputCenter( input_center_x, input_center_y);
    }
    AMLOG_ERROR("CActivePBEngine::ChangeInputCenter, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::SetDisplayPositionSize(AM_INT vout, AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->ChangeSizePosition(vout, pos_x, pos_y, width, height);
    }

    if (mpVoutController) {
        return mpVoutController->SetDisplayPositionSize(vout, pos_x, pos_y, width, height);
    }
    AMLOG_ERROR("CActivePBEngine::SetDisplayPositionSize, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::SetDisplayPosition(AM_INT vout, AM_INT pos_x, AM_INT pos_y)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->ChangePosition(vout, pos_x, pos_y);
    }

    if (mpVoutController) {
        return mpVoutController->SetDisplayPosition(vout, pos_x, pos_y);
    }
    AMLOG_ERROR("CActivePBEngine::SetDisplayPosition, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::SetDisplaySize(AM_INT vout, AM_INT width, AM_INT height)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->ChangeSize(vout, width, height);
    }

    if (mpVoutController) {
        return mpVoutController->SetDisplaySize(vout, width, height);
    }
    AMLOG_ERROR("CActivePBEngine::SetDisplaySize, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::GetDisplayPositionSize(AM_INT vout, AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->GetSizePosition(vout, pos_x, pos_y, width, height);
    }

    if (mpVoutController) {
        return mpVoutController->GetDisplayPositionSize(vout, pos_x, pos_y, width, height);
    }
    AMLOG_ERROR("CActivePBEngine::GetDisplayPositionSize, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::GetDisplayDimension(AM_INT vout, AM_INT* width, AM_INT* height)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->GetDimension(vout, width, height);
    }

    if (mpVoutController) {
        return mpVoutController->GetDisplayDimension(vout, width, height);
    }
    AMLOG_ERROR("CActivePBEngine::GetDisplayDimension, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::GetVideoPictureSize(AM_INT* width, AM_INT* height)
{
    AM_INT offset_x, offset_y;
    if (mShared.voutHandler) {
        return mShared.voutHandler->GetPirtureSizeOffset(width, height, &offset_x, &offset_y);
    }

    if (mpVoutController) {
        return mpVoutController->GetVideoPictureSize(width, height);
    }
    AMLOG_ERROR("CActivePBEngine::GetVideoPictureSize, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::GetCurrentVideoPictureSize(AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height)
{
    AM_INT offset_x, offset_y;
    if (mShared.voutHandler) {
        return mShared.voutHandler->GetPirtureSizeOffset(width, height, &offset_x, &offset_y);
    }

    if (mpVoutController) {
        return mpVoutController->GetCurrentVideoPictureSize(pos_x, pos_y, width, height);
    }
    AMLOG_ERROR("CActivePBEngine::GetCurrentVideoPictureSize, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::VideoDisplayZoom(AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height)
{
    if (mpVoutController) {
        return mpVoutController->VideoDisplayZoom(pos_x, pos_y, width, height);
    }
    AMLOG_ERROR("CActivePBEngine::VideoDisplayZoom, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::SetDisplayRotation(AM_INT vout, AM_INT degree)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->Rotate(vout, degree);
    }

    if (mpVoutController) {
        return mpVoutController->SetDisplayRotation(vout, degree);
    }
    AMLOG_ERROR("CActivePBEngine::SetDisplayRotation, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::SetDisplayFlip(AM_INT vout, AM_INT flip)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->Flip(vout, flip);
    }

    if (mpVoutController) {
        return mpVoutController->SetDisplayFlip(vout, flip);
    }
    AMLOG_ERROR("CActivePBEngine::SetDisplayFlip, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::SetDisplayMirror(AM_INT vout, AM_INT mirror)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->Mirror(vout, mirror);
    }

    if (mpVoutController) {
        return mpVoutController->SetDisplayMirror(vout, mirror);
    }
    AMLOG_ERROR("CActivePBEngine::SetDisplayMirror, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::EnableVout(AM_INT vout, AM_INT enable)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->Enable(vout, enable);
    }

    if (mpVoutController) {
        return mpVoutController->EnableVout(vout, enable);
    }
    AMLOG_ERROR("CActivePBEngine::EnableVout, NULL mpVoutController.\n");
    return ME_ERROR;
}

//run time change
AM_ERR CActivePBEngine::EnableOSD(AM_INT vout, AM_INT enable)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->Enable(vout, enable);
    }

    if (mpVoutController) {
        return mpVoutController->EnableOSD(vout, enable);
    }
    AMLOG_ERROR("CActivePBEngine::EnableOSD, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::EnableVoutAAR(AM_INT enable)
{
    if (mpVoutController) {
        return mpVoutController->EnableVoutAAR(enable);
    }
    AMLOG_ERROR("CActivePBEngine::EnableVoutAAR, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->ChangeSourceRect(x, y, w, h);
    }

    if (mpVoutController) {
        return mpVoutController->SetVideoSourceRect(x, y, w, h);
    }
    AMLOG_ERROR("CActivePBEngine::SetVideoSourceRect, NULL mpVoutController.\n");
    return ME_ERROR;
}

//same with SetDisplayPisitionSize()
AM_ERR CActivePBEngine::SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h)
{
    if (mShared.voutHandler) {
        return mShared.voutHandler->ChangeSizePosition(vout_id, x, y, w, h);
    }

    if (mpVoutController) {
        return mpVoutController->SetVideoDestRect(vout_id, x, y, w, h);
    }
    AMLOG_ERROR("CActivePBEngine::SetVideoDestRect, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y)
{
    if (mpVoutController) {
        return mpVoutController->SetVideoScaleMode(vout_id, mode_x, mode_y);
    }
    AMLOG_ERROR("CActivePBEngine::SetVideoScaleMode, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CActivePBEngine::EnableDewarpFeature(AM_UINT enable, AM_UINT dewarp_param)
{
    mShared.enable_horz_dewarp = 1;
    return ME_OK;
}

AM_ERR CActivePBEngine::SetDeWarpControlWidth(AM_UINT enable, AM_UINT width_top, AM_UINT width_bottom)
{
    AM_ASSERT(1 == mShared.enable_horz_dewarp);
    if (mpVoutController) {
        return mpVoutController->SetDeWarpControlWidth(enable, width_top, width_bottom);
    }
    AMLOG_ERROR("CActivePBEngine::SetDeWarpControlWidth, NULL mpVoutController.\n");
    return ME_ERROR;
}

void CActivePBEngine::GetDecType(DECSETTING decSetDefault[])
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
void CActivePBEngine::SetDecType(DECSETTING decSetCur[])
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
void CActivePBEngine::ResetParameters()
{
    //reset the parameters
    mbRun = true;
    mbPaused = false;
    mpVoutController = NULL;
    mpDemuxer = NULL;
    mpMasterRenderer = NULL;
    mpMasterFilter = NULL;
    mMostPriority = 0;
    mSeekTime = 0;
    mLoopCurrentItem = 0;
    mbUseAbsoluteTime = true;
    mShared.mbAlreadySynced = 0;
    mShared.mbAudioFirstPTSVaild = false;
    mShared.mbVideoFirstPTSVaild = false;
    mShared.mbVideoStreamStartTimeValid = false;
    mShared.mbAudioStreamStartTimeValid = false;
    AMLOG_INFO("---reset active_pb_engine---\n");
}

#ifdef AM_DEBUG
//obsolete, would be removed later
AM_ERR CActivePBEngine::SetLogConfig(AM_UINT index, AM_UINT level, AM_UINT option)
{
/*    if(mnFilters <= index || level > LogAll) {
        AMLOG_INFO("CActivePBEngine::SetLogConfig, invalid arguments: index=%d,level=%d,option=%d.\n", index, level, option);
        return ME_BAD_PARAM;
    }

    if(!mFilters[index].pFilter) {
        AMLOG_INFO("CActivePBEngine::SetLogConfig, mFilters[index].pFilter not ready, must have errors.\n");
        return ME_ERROR;
    }

    ((CFilter*)mFilters[index].pFilter)->SetLogLevelOption((ELogLevel)mShared.logConfig[index].log_level, mShared.logConfig[index].log_option, 0);
    */return ME_OK;
}

void CActivePBEngine::PrintState()
{
    AM_UINT i = 0;
    AM_UINT udec_state, vout_state, error_code;
    AMLOG_INFO(" CActivePBEngine's state is %d.\n", mState);
    AMLOG_INFO(" is_paused %d, cmd cnt from app %d, msg from filters cnd %d.\n", mbPaused, mpCmdQueue->GetDataCnt(), mpFilterMsgQ->GetDataCnt());
    for (i = 0; i < mnFilters; i ++) {
        mFilters[i].pFilter->PrintState();
    }
    GetUdecState(mShared.mIavFd, &udec_state, &vout_state, &error_code);
    AMLOG_INFO(" Udec state %d, Vout state %d, error_code 0x%x.\n", udec_state, vout_state, error_code);
}
#endif

bool CActivePBEngine::TryPlayNextItem()
{
    AM_PlayItem* pnext;
    AM_ERR err;

    pnext = FindNextPlayItem(mpCurrentItem);
    if (pnext == mpCurrentItem) {
        //play same item
        AMLOG_INFO("[PlayList]: select the same item, play again.\n");
        err = Seek(0);
        AM_ASSERT(err == ME_OK);
        if (err != ME_OK) {
            AMLOG_ERROR("[PlayList]:  Seek(0) return error, this is not-expected, how to handle it?.\n");
            return false;
        }
        return true;
    } else if (pnext) {
        //clear current Context;
        AMLOG_INFO("[PlayList]: select another same item, play new item, clear current engine first.\n");
        mpCurrentItem->pSourceFilterContext = NULL;
        mpCurrentItem = pnext;
        ClearGraph();
        ResetParameters();
        NewSession();

        AMLOG_INFO("[PlayList]: clear current engine done, start play new item.\n");
        err = BuildFilterGragh(mpCurrentItem);
        if (err != ME_OK) {
            AMLOG_ERROR("[PlayList]: new item cannot play, would be currupt, remove it from play list, try another one.\n");
            GetoutPlayItem(&mPlayListHead, mpCurrentItem);
            ReleasePlayItem(mpCurrentItem);

retry_NextItem:
            //find another play item
            mpCurrentItem = FindNextPlayItem(NULL);
            while (mpCurrentItem) {
                err = BuildFilterGragh(mpCurrentItem);
                if (err != ME_OK) {
                    AMLOG_ERROR("[PlayList]: new item cannot play, would be currupt, remove it from play list, try another one.\n");
                    GetoutPlayItem(&mPlayListHead, mpCurrentItem);
                    ReleasePlayItem(mpCurrentItem);
                    ClearGraph();
                    ResetParameters();
                    NewSession();
                    goto retry_NextItem;
                } else {
                    AMLOG_INFO("[PlayList]: new item can play, start RunAllFilters().\n");
                    err = RunAllFilters();
                    if (err != ME_OK) {
                        AMLOG_ERROR("[PlayList]: RunAllFilters return err %d, remove it, and try another one.\n", err);
                        GetoutPlayItem(&mPlayListHead, mpCurrentItem);
                        ReleasePlayItem(mpCurrentItem);
                        ClearGraph();
                        ResetParameters();
                        NewSession();
                        goto retry_NextItem;
                    }
                    AMLOG_INFO("[PlayList]: RunAllFilters() done.\n");
                    return true;
                }
            }
        } else {
            AMLOG_INFO("[PlayList]: new item can play, start playing.\n");
            err = RunAllFilters();
            if (err != ME_OK) {
                AMLOG_ERROR("[PlayList]: RunAllFilters return err %d, remove it, and try another one.\n", err);
                GetoutPlayItem(&mPlayListHead, mpCurrentItem);
                ReleasePlayItem(mpCurrentItem);
                ClearGraph();
                ResetParameters();
                NewSession();
                goto retry_NextItem;
            }
        }
    } else {
        AM_ASSERT(!pnext);
        AM_ASSERT(!mPlayListHead.CntOrTag);
        AMLOG_ERROR("[PlayList]: no items, should not come here.\n");
    }
    return false;
}

void CActivePBEngine::checkDupexParameters(void)
{
    if (mRequestVoutMask & (1 << eVoutHDMI)) {
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

    } else if (mRequestVoutMask & (1 << eVoutLCD)) {
        AMLOG_INFO("[vout]: duplex mode request preview on HDMI.\n");
        if (mShared.encoding_mode_config.preview_vout_index != eVoutLCD) {
            AMLOG_WARN("[correct parameters]: preview's display on HDMI, change it to LCD.\n");
            mShared.encoding_mode_config.preview_vout_index = eVoutLCD;
        }

        if (mShared.encoding_mode_config.playback_vout_index!= eVoutLCD) {
            AMLOG_WARN("[correct parameters]: preview's display on HDMI, change it to LCD.\n");
            mShared.encoding_mode_config.playback_vout_index = eVoutLCD;
        }

        AM_ASSERT(mShared.encoding_mode_config.pb_display_enabled);
        if (1 == mShared.encoding_mode_config.pb_display_enabled) {
            AMLOG_WARN("[correct parameters]: enable playback on LCD, should disable preview display on LCD.\n");
            mShared.encoding_mode_config.preview_enabled = 0;
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

AM_ERR CActivePBEngine::BuildFilterGragh(AM_PlayItem* item)
{
    AM_ERR err;
    if (!item || !item->pSourceFilter) {
        AMLOG_ERROR("Fatal Error: BuildFilterGragh NULL input.\n");
        return ME_ERROR;
    }

    //check duplex parameters
    mShared.pbConfig.vout_config = mRequestVoutMask;
    AM_ASSERT(mRequestVoutMask);
    checkDupexParameters();

    AMLOG_INFO("[vout related info]: (pb engine) mRequestVoutMask 0x%x, dsp mode %d.\n", mRequestVoutMask, mShared.encoding_mode_config.dsp_mode);
    AMLOG_INFO("[vout related info]: pb display enabled %d, vout index %d.\n", mShared.encoding_mode_config.pb_display_enabled, mShared.encoding_mode_config.playback_vout_index);
    AMLOG_INFO("[vout related info]: preview display enabled %d, vout index %d.\n", mShared.encoding_mode_config.preview_enabled, mShared.encoding_mode_config.preview_vout_index);
    AMLOG_INFO("[vout related info]: playback display w %d, h %d, offset_x %d, offset_y %d.\n", mShared.encoding_mode_config.pb_display_width, mShared.encoding_mode_config.pb_display_height, \
        mShared.encoding_mode_config.pb_display_left, mShared.encoding_mode_config.pb_display_top);
    AMLOG_INFO("[vout related info]: preview display w %d, h %d, offset_x %d, offset_y %d, alpha %d.\n", mShared.encoding_mode_config.preview_width, mShared.encoding_mode_config.preview_height, \
        mShared.encoding_mode_config.preview_left, mShared.encoding_mode_config.preview_top, mShared.encoding_mode_config.preview_alpha);

    filter_entry * sourceFilter = (filter_entry *)item->pSourceFilter;

    // build the graph
    if (!item->pSourceFilterContext) {
        //get context
        parser_obj_s parser;
        parse_file_s parseFile;
        AMLOG_INFO("not first time, need create source filter context.\n");
        parseFile.pFileName = item->pDataSource;
        sourceFilter->parse(&parseFile, &parser);
        item->pSourceFilterContext = parser.context;
    } else {
        AMLOG_INFO("first time, get source filter context from recognizer.\n");
    }

    // create source filter
    IFilter *pFilter = sourceFilter->create((IEngine*)(inherited*)this);
    if (pFilter == NULL) {
        AMLOG_ERROR("Create %s failed\n", sourceFilter->pName);
        return ME_NOT_EXIST;
    }

    // get IDemuxer
    mpDemuxer = IDemuxer::GetInterfaceFrom(pFilter);
    if (mpDemuxer == NULL) {
        AMLOG_ERROR("No IDemuxer\n");
        AM_DELETE(pFilter);
        return ME_NO_INTERFACE;
    }

    // load file
    err = mpDemuxer->LoadFile((const char*)item->pDataSource, item->pSourceFilterContext);
    if (err != ME_OK) {
        AMLOG_ERROR("LoadFile failed\n");
        AM_DELETE(pFilter);
        return err;
    }

    // get&check file type
    const char *pFileType = NULL;
    mpDemuxer->GetFileType(pFileType);
    CheckFileType(pFileType);

    // check stream type
    //CheckStreamType();

    // save
    err = AddFilter(pFilter);
    if (err != ME_OK) {
        AM_DELETE(pFilter);
        AMLOG_ERROR("Too many filters(%d).\n", mnFilters);
        return ME_TOO_MANY;
    }

    // render this filter (recursive)
    err = RenderFilter(pFilter);
    if (err != ME_OK) {
        AMLOG_ERROR("RenderFilter fail err = %d.\n", err);
        return err;
    }

    //AM_ASSERT(mpDataRetriever);
    if (mpDataRetriever) {
        AM_ASSERT(mPrivateDataCallbackCookie && mPrivatedataCallback);
        if (mPrivateDataCallbackCookie && mPrivatedataCallback) {
            mpDataRetriever->SetDataRetrieverCallBack(mPrivateDataCallbackCookie, mPrivatedataCallback);
        }
    }

    //set master renderer
    AM_ASSERT(mpMasterFilter);
    mpMasterFilter->SetMaster(true);
    mpMasterRenderer = IRender::GetInterfaceFrom(mpMasterFilter);
    AM_ASSERT(mpMasterRenderer);

    IFilter::INFO info;
    mpMasterFilter->GetInfo(info);
    AMLOG_INFO("master renderer is %s.\n",info.pName);
    //pb-config
    if (mShared.pbConfig.audio_disable) {
        AMLOG_INFO("Disable audio.\n");
        mpDemuxer->EnableAudio(false);
    }
    if (mShared.pbConfig.video_disable) {
        AMLOG_INFO("Disable video.\n");
        mpDemuxer->EnableVideo(false);
    }
    if (mShared.pbConfig.subtitle_disable) {
        AMLOG_INFO("Disable subtitle.\n");
        mpDemuxer->EnableSubtitle(false);
    }
    if (mShared.mbStartWithStepMode && mpVoutController) {
        AMLOG_INFO("Start with step mode.\n");
        mpVoutController->Step();
    }

    AM_ASSERT(mpDemuxer);
    mpDemuxer->GetTotalLength(mCurrentItemTotLength);
    AMLOG_INFO("current item tot lenth %llu.\n", mCurrentItemTotLength);
    mpCurrentItem = item;
    mpCurrentItem->pSourceFilterContext = NULL;

    return ME_OK;
}

bool CActivePBEngine::ProcessGenericCmd(CMD& cmd)
{
    AM_PlayItem* item = NULL;
    AM_ERR err;
    AM_UINT i;

    AMLOG_CMD("****CActivePBEngine::ProcessGenericCmd, cmd.code %d, state %d.\n", cmd.code, mState);

    switch (cmd.code) {
        case CMD_STOP:
            AMLOG_INFO("****CActivePBEngine STOP cmd, start.\n");
            mbRun = false;
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActivePBEngine STOP cmd, done.\n");
            break;

        case CMD_START:
            AMLOG_INFO("****CActivePBEngine START cmd, start.\n");
            mbRun = true;
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActivePBEngine START cmd, done.\n");
            break;

        case PBCMD_STOP_PLAY:
            AMLOG_INFO("****CActivePBEngine PBCMD_STOP_PLAY cmd, start.\n");
            AM_ASSERT(mpClockManager);
            mpClockManager->StopClock();
            ClearGraph();
            ResetParameters();
            NewSession();
            SetState(STATE_STOPPED);
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActivePBEngine PBCMD_STOP_PLAY cmd, done.\n");
            break;

        case PBCMD_START_PLAY:
            AMLOG_INFO("****CActivePBEngine PBCMD_START_PLAY cmd, start.\n");

            err = RunAllFilters();
            if (err != ME_OK){
                AMLOG_ERROR("RunAllfilters fail %d.\n", err);
                SetState(STATE_ERROR);
            }
            //SetState(STATE_STARTED);//for bug#2308 case 3: all format decoders, quick seek at file beginning, "CVideoRenderer::requestRun" blocked issue
            mpWorkQ->CmdAck(ME_OK);
            AMLOG_INFO("****CActivePBEngine PBCMD_START_PLAY cmd, done.\n");
            break;

        case PBCMD_ADD_PLAYITEM:
            item = (AM_PlayItem*)cmd.pExtra;
            AM_ASSERT(item);
            if (item && item->pDataSource && item->type != PlayItemType_None) {
                AMLOG_INFO("****CActivePBEngine add new data source %s, type %d.\n", item->pDataSource, item->type);
                AppendPlayItem(&mPlayListHead, item);
                AMLOG_INFO("****CActivePBEngine add new data source, done.\n");
            } else {
                AMLOG_ERROR("PBCMD_ADD_PLAYITEM error.\n");
            }
            break;

        case PBCMD_PREPARE:
            err = ME_OK;
            item = (AM_PlayItem*)cmd.pExtra;
            AM_ASSERT(item);
            if (item && item->pDataSource && item->type != PlayItemType_None) {
                AMLOG_INFO("****CActivePBEngine add new data source %s, type %d.\n", item->pDataSource, item->type);
                AppendPlayItem(&mPlayListHead, item);
                AMLOG_INFO("****CActivePBEngine add new data source, done.\n");
            } else {
                AMLOG_ERROR("PBCMD_ADD_PLAYITEM error.\n");
            }

            AM_ASSERT(mState == STATE_IDLE);
            //if in ilde state, play the first item first
            if (mState == STATE_IDLE) {
                err = BuildFilterGragh(item);
                if (err != ME_OK) {
                    //handle error here
                    SetState(STATE_ERROR);
                    AMLOG_ERROR("BuildFilterGragh fail with first play item.\n");
                } else {
                    AMLOG_INFO("BuildFilterGragh success with first play item.\n");
                    SetState(STATE_PREPARED);
                }
            }
            mpWorkQ->CmdAck(err);
            break;

        case PBCMD_SEEK:
            AMLOG_INFO("Seek cmd comes, mState %d.\n", mState);
#ifdef _debug_use_stable_strategy_for_seek_
            if (mbSeeking && mSeekTime == cmd.res64_1) {
                AMLOG_WARN("Discard duplicated seek cmd.\n");
                mpWorkQ->CmdAck(ME_OK);
                break;
            }
#endif
            if (mState == STATE_STARTED || mState == STATE_PAUSED || mState == STATE_COMPLETED) {
                err = HandleSeek(cmd.res64_1);
                //After handle seek successfully, state of engine should turn to STATE_STARTED. chcuhen 2012_3_27
                if(err == ME_OK && mState == STATE_COMPLETED)
                    mState = STATE_STARTED;
            }

            //Apps may send seek cmd before PlayFile(), Gallery, etc.
            //All filters' state machines have not run up, so just seek in demuxer.
            if(mState == STATE_PREPARED){
                if(cmd.res64_1 >= mCurrentItemTotLength){
                    AMLOG_WARN("seek time ms=%lld,length=%lld.\n", cmd.res64_1, mCurrentItemTotLength);
                }else{
                    AMLOG_INFO("Start playing from %llu ms.\n",cmd.res64_1);
                    mSeekTime = cmd.res64_1;
                    mShared.mPlaybackStartTime = 90 * mSeekTime;
                    mbSeeking = true;
                    if (mpDemuxer->Seek(cmd.res64_1) != ME_OK) {
                        AMLOG_ERROR("seek error!.\n");
#ifdef _debug_use_stable_strategy_for_seek_
                        mpWorkQ->CmdAck(ME_OK);
#endif
                        break;
                    }
                }
            }
            AMLOG_INFO("Seek cmd comes process end, mState %d.\n", mState);
#ifdef _debug_use_stable_strategy_for_seek_
            mpWorkQ->CmdAck(ME_OK);
#endif
            break;

        case PBCMD_PAUSE_RESUME:
            AMLOG_INFO("pause-resume cmd comes, mState %d, cmd.flag %d, mbPaused %d.\n", mState, cmd.flag, mbPaused);
            if (mState == STATE_STARTED) {
                AM_ASSERT(!mbPaused && (cmd.flag == 1));
                if (!mbPaused && (cmd.flag == 1)) {
#ifdef _debug_old_flow_
                    if (!mShared.get_outpic) {
                        if (!mShared.udecHandler) {
                            PauseUdec(mShared.mIavFd, 0);
                        } else {
                            mShared.udecHandler->PauseUDEC(0);
                        }
                    }
#endif
                    //pause
                    mpClockManager->PauseClock();
                    AMLOG_INFO("PauseAllFilters\n");
                    for(i=0; i<mnFilters; i++) {
                        mFilters[i].pFilter->Pause();
                    }
                    mbPaused = true;
                    SetState(STATE_PAUSED);
                }
            } else if (mState == STATE_PAUSED) {
                AM_ASSERT(mbPaused && (cmd.flag == 0));
                if (mbPaused && (cmd.flag == 0)) {
#ifdef _debug_old_flow_
                    if (!mShared.get_outpic) {
                        if (!mShared.udecHandler) {
                            ResumeUdec(mShared.mIavFd, 0);
                        } else {
                            mShared.udecHandler->ResumeUDEC(0);
                        }
                    }
#endif
                    //resume
                    mpClockManager->ResumeClock();
                    AMLOG_INFO("ResumeAllFilters\n");
                    for(i=0; i<mnFilters; i++) {
                        mFilters[i].pFilter->Resume();
                    }
                    mbPaused = false;
                    SetState(STATE_STARTED);
                }
            } else if (mState == STATE_STEP) {
#ifdef _debug_old_flow_
                if (!mShared.get_outpic) {
                    if (!mShared.udecHandler) {
                        ResumeUdec(mShared.mIavFd, 0);
                    } else {
                        mShared.udecHandler->ResumeUDEC(0);
                    }
                }
#endif
                //resume
                mpClockManager->ResumeClock();
                AMLOG_INFO("ResumeAllFilters\n");
                for(i=0; i<mnFilters; i++) {
                    mFilters[i].pFilter->Resume();
                }
                SetState(STATE_STARTED);
                mShared.mbStartWithStepMode = 1;
            } else {
                AMLOG_INFO("pause/resume when state is not STATE_STARTED and STATE_PAUSED, state %d, cmd.flag %d.\n", mState, cmd.flag);
                if (!mbPaused && (cmd.flag == 1)) {
                    //pause
                    mbPaused = true;
                } else if (mbPaused && (cmd.flag == 0)) {
                    //resume
                    mbPaused = false;
                }
            }
            mpWorkQ->CmdAck(ME_OK);
            break;

        default:
            return false;
    }
    return true;
}

bool CActivePBEngine::ProcessGenericMsg(AM_MSG& msg)
{
//    AM_PlayItem* item = NULL;
//    AM_ERR err;
    AM_UINT data_len;

    AMLOG_CMD("****CActivePBEngine::ProcessGenericMsg, msg.code %d, msg.sessionID %d, state %d.\n", msg.code, msg.sessionID, mState);

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
            HandleEOSMsg(msg);
            if(mNeedReConfig == 1){
                AM_ASSERT(mpDemuxer);
                AM_ASSERT(mpDecodeController);

                AM_INT flag = mShared.mReConfigMode;
                AMLOG_INFO("reconfig mode is %d\n",flag);

                mpDecodeController->ReConfigDecoder(flag);
                if(flag == 0){
                    AM_INT mWidth;
                    AM_INT mHeight;
                    mpDemuxer->GetVideoSize(&mWidth, &mHeight);
                    ChangeInputCenter(mWidth/2, mHeight/2);
                }
                AllFiltersMsg(IActiveObject::CMD_BEGIN_PLAYBACK);
                SetState(STATE_STARTED);
                mNeedReConfig = 0;
            }
            AMLOG_INFO("-----MSG_EOS done-----\n");
            break;

        case MSG_AVSYNC:
            AMLOG_INFO("-----MSG_AVSYNC-----\n");
            HandleSyncMsg(msg);
            break;

        case MSG_NOTIFY_UDEC_IS_RUNNING:
            AMLOG_INFO("-----MSG_NOTIFY_UDEC_IS_RUNNING-----\n");
            HandleNotifyUDECIsRunning();
            break;

        case MSG_SOURCE_FILTER_BLOCKED:
            AMLOG_INFO("-----MSG_SOURCE_FILTER_BLOCKED-----\n");
            HandleSourceFilterBlockedMsg(msg);
            break;

        case MSG_ERROR:
            HandleErrorMsg(msg);
            break;
        case MSG_SUBERROR:
            AMLOG_INFO("----MSG_SUBERROR---- disablesustile---\n");
            mpDemuxer->EnableSubtitle(false);
            break;
        case MSG_SWITCH_MASTER_RENDER://for bug1294
            AMLOG_INFO("-----MSG_SWITCH_MASTER_RENDER-----\n");
            HandleSwitchMasterRenderMsg(msg);
            break;
        case MSG_VIDEO_RESOLUTION_CHANGED:
            AMLOG_INFO("-----MSG_VIDEO_RESOLUTION_CHANGED-----\n");
            mNeedReConfig = 1;
            break;

        case MSG_PRIDATA_GENERATED:
            AMLOG_DEBUG("-----MSG_PRIDATA_GENERATED-----, %d, %d, %d.\n", (AM_UINT)msg.p0, msg.p2, msg.p3);
            AM_ASSERT(mpDataRetriever);
            if (!mpDataRetriever || !mPrivateDataCallbackCookie || !mPrivatedataCallback) {
                AMLOG_WARN("NULL pointer in process MSG_PRIDATA_GENERATED, mpDataRetriever %p, cookie %p, callback %p.\n", mpDataRetriever, mPrivateDataCallbackCookie, mPrivatedataCallback);
                __atomic_dec(&mShared.mnPrivateDataCountNeedSent);
                break;
            }

            //get data
            data_len = (AM_UINT)msg.p0;
            if (data_len >= mPrivateDataBufferToTSize) {
                mPrivateDataBufferToTSize = data_len + 16;
                if (mpPrivateDataBufferForCallback) {
                    free(mpPrivateDataBufferForCallback);
                }
                mpPrivateDataBufferForCallback = (AM_U8*)malloc(mPrivateDataBufferToTSize);
                AM_ASSERT(mpPrivateDataBufferForCallback);
            }

            if (mpPrivateDataBufferForCallback) {
                mpDataRetriever->RetieveDataByType(mpPrivateDataBufferForCallback, data_len, msg.p2, msg.p3);
            }

            //invoke callback
            AMLOG_DEBUG("***before engine mPrivatedataCallback.\n");
            mPrivatedataCallback(mPrivateDataCallbackCookie, mpPrivateDataBufferForCallback, data_len, (AM_U16)msg.p2, (AM_U16)msg.p3);
            AMLOG_DEBUG("***after engine mPrivatedataCallback.\n");
            __atomic_dec(&mShared.mnPrivateDataCountNeedSent);
            break;

        case MSG_AUDIO_PTS_LEAPED:
            AMLOG_INFO("-----MSG_AUDIO_PTS_LEAPED-----\n");
            AM_ASSERT(mpDecodeController);
            if(mpDecodeController){
                mpDecodeController->AudioPtsLeaped();
            }
            break;

        default:
            break;
    }
    AMLOG_CMD("****CActivePBEngine::ProcessGenericMsg, msg.code %d done, msg.sessionID %d, state %d.\n", msg.code, msg.sessionID, mState);

    return true;
}

void CActivePBEngine::ProcessCMD(CMD& oricmd)
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

bool CActivePBEngine::isCmdQueueFull()
{
    AM_ASSERT(mpCmdQueue);
    if (mpCmdQueue->GetDataCnt() > MAX_CMD_COUNT) {
        return true;
    }
    return false;
}

AM_ERR CActivePBEngine::SetPBProperty(AM_UINT prop, AM_UINT value)
{
    switch (prop) {
        case IPBControl::DEC_PROPERTY_SPEEDUP_REALTIME_PLAYBACK:
            AMLOG_INFO("post speedup cmd.\n");
            AllFiltersMsg(IActiveObject::CMD_REALTIME_SPEEDUP);
            break;

        case IPBControl::DEC_DEBUG_PROPERTY_DISCARD_HALF_AUDIO_PACKET:
            AMLOG_WARN("set discard audio packet property to audio renderer.\n");
            mShared.discard_half_audio_packet = 1;
            break;

        case IPBControl::DEC_DEBUG_PROPERTY_COLOR_TEST:
            AMLOG_WARN("enable color test.\n");
            mShared.enable_color_test = 1;
            break;

        case IPBControl::DSP_TILE_MODE:
            AMLOG_WARN("tilemode %d.\n", value);
            mShared.dspConfig.preset_tilemode = value;
            break;

        case IPBControl::DSP_PREFETCH_COUNT:
            AMLOG_WARN("prefetch_count %d.\n", value);
            mShared.dspConfig.preset_prefetch_count= value;
            mShared.dspConfig.preset_enable_prefetch = mShared.dspConfig.preset_prefetch_count ? 1 :0;
            break;

        case IPBControl::DSP_BITS_FIFO_SIZE:
            AMLOG_WARN("bits_fifo_size %d.\n", value);
            mShared.dspConfig.preset_bits_fifo_size= value;
            break;

        case IPBControl::DSP_REF_CACHE_SIZE:
            AMLOG_WARN("ref_cache_size %d.\n", value);
            mShared.dspConfig.preset_ref_cache_size= value;
            break;

        case IPBControl::DEC_DEBUG_PROPERTY_AUTO_VOUT:
            AMLOG_WARN("%s auto vout.\n", value?"enable":"disable");
            mShared.pbConfig.auto_vout_enable = value;
            break;

        default:
            AM_ERROR("BAD prop %d, value %d.\n", prop, value);
            break;
    }
    return ME_OK;
}

void CActivePBEngine::OnRun()
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
        AMLOG_STATE("CActivePBEngine: mState=%d, msg cnt from filters %d.\n", mState, mpFilterMsgQ->GetDataCnt());
        if (mState == STATE_ERROR) {
            SetState(STATE_COMPLETED);
            PostAppMsg(IMediaControl :: MSG_PLAYER_EOS);
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

    AMLOG_STATE("CActivePBEngine: start Clear gragh for safe.\n");
    ClearGraph();
    ResetParameters();
    NewSession();
}

