

/**
 * pb_engine.cpp
 *
 * History:
 *    2009/12/22 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "pb_engine"
//#define AMDROID_DEBUG

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
#include "pb_engine.h"
#include "filter_list.h"
#include "filter_registry.h"

IPBControl* CreatePBControl(void *audiosink)
{
	return (IPBControl*)CPBEngine::Create(audiosink);
}

//-----------------------------------------------------------------------
//
// CPBEngine
//
//-----------------------------------------------------------------------
CPBEngine* CPBEngine::Create(void *audiosink)
{
	CPBEngine *result = new CPBEngine(audiosink);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

CPBEngine::CPBEngine(void *audiosink):
	mFatalErrorCnt(0),
	mTrySeekTimeGap(2000),
	mState(STATE_IDLE),
	mDir(DIR_FORWARD),
	mSpeed(SPEED_NORMAL),
	mpRecognizer(NULL),
	mpDemuxer(NULL),
	mpMasterRenderer(NULL),
	mpVoutController(NULL),
	mpClockManager(NULL),
	mpAudioSink(audiosink),
	mpMasterFilter(NULL),
	mMostPriority(0),
	mSeekTime(0),
	mLoop(0),
	mbUseAbsoluteTime(true)
{
    AM_UINT i = 0;
    char configfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};
    memset((void*)&mShared, 0, sizeof(mShared));

    //debug
    mShared.mEngineVersion = 0;
    mShared.mbIavInited = 0;
    mShared.mIavFd = -1;//init value

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
    for (i=0; i< CODEC_NUM; i++) {
        AMLOG_INFO("      video format %d, select dec type %d.\n", mShared.decSet[i].codecId, mShared.decSet[i].dectype);
    }
    AM_PrintLogConfig();
    AM_PrintPBDspConfig(&mShared);

    mpOpaque = (void*)&mShared;

#if PLATFORM_ANDROID
    //hard code: request disable osd on hdmi
    mShared.dspConfig.voutConfigs.voutConfig[eVoutHDMI].osd_disable = 1;
#endif

}

AM_ERR CPBEngine::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;
	DSetModuleLogConfig(LogModulePBEngine);

	if ((mpClockManager = CClockManager::Create()) == NULL)
		return ME_ERROR;

	return ME_OK;
}

CPBEngine::~CPBEngine()
{
    AM_INFO(" ====== Destroy PBEngine ====== \n");
    AM_DELETE(mpRecognizer);
    AM_DELETE(mpClockManager);
    mpOpaque = NULL;
    pthread_mutex_destroy(&mShared.mMutex);
    AM_INFO(" ====== Destroy PBEngine done ====== \n");
}

void CPBEngine::Delete()
{
	AM_DELETE(mpRecognizer);
	mpRecognizer = NULL;
	AM_DELETE(mpClockManager);
	mpClockManager = NULL;
	inherited::Delete();
}

void *CPBEngine::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IPBControl)
		return (IPBControl*)this;
	if (refiid == IID_IPBEngine)
		return (IPBEngine*)this;
	return inherited::GetInterface(refiid);
}

void *CPBEngine::QueryEngineInterface(AM_REFIID refiid)
{
	if (refiid == IID_IClockManager)
		return mpClockManager;

	if (refiid == IID_IAudioSink)
		return mpAudioSink;

	return NULL;
}

AM_ERR CPBEngine::CreateRecognizer()
{
	if (mpRecognizer)
		return ME_OK;

	if ((mpRecognizer = CMediaRecognizer::Create((IEngine*)(inherited*)this)) == NULL)
		return ME_NO_MEMORY;

	return ME_OK;
}

AM_ERR CPBEngine::PlayFile(const char *pFileName)
{
	AUTO_LOCK(mpMutex);
#if 0
	StopPlay();

	AM_ERR err = CreateRecognizer();
	if (err != ME_OK)
		return err;

	strncpy(mFileName, pFileName, sizeof(mFileName) - 1);	// todo
	err = mpRecognizer->RecognizeFile(mFileName);
	if (err != ME_OK)
		return err;

	SetState(STATE_OPENING);
#endif

	AM_ERR err = RunAllFilters();
	if (err != ME_OK){
		EnterErrorState(err);
		return err;
	}
	// set state
	SetState(STATE_STARTED);
	PostAppMsg(MSG_STATE_CHANGED);

	return ME_OK;
}

AM_ERR CPBEngine::PrepareFile(const char *pFileName)
{
    AUTO_LOCK(mpMutex);
    StopPlay();

    SetState(STATE_INITIALIZED);
    AM_ERR err = CreateRecognizer();
    if (err != ME_OK)
        return err;

    strncpy(mFileName, pFileName, sizeof(mFileName) - 1);	// todo
    err = mpRecognizer->RecognizeFile(mFileName, true);
    if (err != ME_OK)
        return err;

    HandleParseMediaDone();

    if (mState != STATE_PREPARED)
        return ME_ERROR;

    return ME_OK;
}

AM_ERR CPBEngine::StopPlay()
{
	AUTO_LOCK(mpMutex);


	if (mState == STATE_STOPPED)
		return ME_OK;

	if (mState == STATE_INITIALIZED){
		mpRecognizer->StopRecognize();
	}
	mpClockManager->StopClock();
	mpClockManager->SetSource(NULL);

	ClearGraph();
	Reset();
	NewSession();
	//mpClockManager->StopClock();
	//mpClockManager->SetSource(NULL);

	SetState(STATE_STOPPED);
	return ME_OK;
}

AM_ERR CPBEngine::GetPBInfo(PBINFO& info)
{
    AUTO_LOCK(mpMutex);

    info.state = mState;
    info.position = 0;
    if (STATE_STARTED== mState ||STATE_PAUSED == mState || STATE_PREPARED ==mState || STATE_STEP == mState)
    {
        AM_U64 absoluteTimeMs = 0, relativeTimeMs = 0;
        mpMasterRenderer->GetCurrentTime(absoluteTimeMs, relativeTimeMs);
        mpDemuxer->GetTotalLength(info.length);

        // adopt two different methods to calculate current time
        // 1. normal  case = (lastPts - firstPts) + delta, which is done by master renderer
        // 2. special case = seek time + delta
        // note:
        // delta represents the duration, in which master renderer have played samples or frames normally
        if (mbUseAbsoluteTime && !mShared.mbSeekFailed) {
            info.position = absoluteTimeMs;
            if(absoluteTimeMs == 0 && relativeTimeMs == 0)
            {
                info.position = mSeekTime;
            }
        }
        else {
            info.position = mSeekTime + relativeTimeMs;
        }

        if (info.position > info.length) {
            info.position = info.length;
        }
    } else if(STATE_COMPLETED == mState){
        mpDemuxer->GetTotalLength(info.length);
        info.position = info.length;
    } else {
        AMLOG_ERROR("CPBEngine::GetPBInfo error state %d.\n", mState);
        return ME_ERROR;
    }

    AMLOG_VERBOSE("CPBEngine::GetPBInfo info.position = %llu, info.length= %llu, mState %d.\n", info.position, info.length, mState);
    return ME_OK;
}

void CPBEngine::PauseAllFilters()
{
    AMLOG_INFO("PauseAllFilters\n");
    for(AM_UINT i=0; i<mnFilters; i++) {
    mFilters[i].pFilter->Pause();
    }
}

AM_ERR CPBEngine::PausePlay()
{
   AUTO_LOCK(mpMutex);

    if (mState == STATE_STOPPED || mState == STATE_STEP)
    {
        AMLOG_ERROR("PausePlay: bad state, %d\n", mState);
        return ME_BAD_STATE;
    }
    PauseAllFilters();
    mpClockManager->PauseClock();

    SetState(STATE_PAUSED);
    return ME_OK;

}

void  CPBEngine::ResumeAllFilters()
{
    AMLOG_INFO("ResumeAllFilters\n");
    for(AM_UINT i=0; i<mnFilters; i++) {
        mFilters[i].pFilter->Resume();
    }
}

AM_ERR CPBEngine::ResumePlay()
{
    AUTO_LOCK(mpMutex);
    AMLOG_INFO("ResumePlay---\n");
    // Add condition: mState != STATE_EOS
    // When Playback reaches the end of stream in REPEAT_CURRENT mode,
    // mediaplayer seek 0 and start again.
    // PBEngine is in STATE_EOS and all filters are on pending,
    // so we should call ResumePlay other than PlayFile, which blocks app thread
    if (mState != STATE_PAUSED && mState != STATE_COMPLETED)
    {
        AM_ERROR("ResumePlay: bad state\n");
        return ME_BAD_STATE;
    }
    mpClockManager->ResumeClock();
    ResumeAllFilters();
    SetState(STATE_STARTED);
    return ME_OK;
}

AM_ERR CPBEngine::Loopplay(AM_U32 loop_mode)
{
    mShared.mbLoopPlay = loop_mode;
    return ME_OK;
}

//#define _tmp_fix_ucode_cannot_seek_when_paused_

AM_ERR CPBEngine::Seek(AM_U64 ms)
{
    AM_ERR err;
//    AM_UINT i = 0;
    AM_U64 length = 0;
    AUTO_LOCK(mpMutex);
    AMLOG_INFO("**New Seek\n");

    if (mState == STATE_STOPPED || mState == STATE_STEP) {
        AMLOG_ERROR("Cannot Seek in state %d.\n", mState);
        return ME_OK;
    }

    mpDemuxer->GetTotalLength(length);
    if(ms>=length)
    {
        AMLOG_ERROR("seek time bigger than length, no seek. ms=%lld,length=%lld.\n", ms,length);
        return ME_OK;
    }

    PB_STATE oldState;
    oldState = mState;
#ifdef _tmp_fix_ucode_cannot_seek_when_paused_
    if(oldState == STATE_PAUSED)
        ResumePlay();
#endif
    //SetState(STATE_SEEKING);
    // clear ready flag for renderers
    ClearReadyEOS();

    //clear shared data
    mShared.mbAlreadySynced = 0;
    mShared.mbVideoFirstPTSVaild = false;
    mShared.mbAudioFirstPTSVaild = false;
    mShared.videoFirstPTS = 0;
    mShared.audioFirstPTS = 0;
#if 1
    if(mState != STATE_PREPARED)
    {
        mpClockManager->PauseClock();
#ifdef _tmp_fix_ucode_cannot_seek_when_paused_
        if(mpVoutController->CheckRunState())
        {
            AMLOG_ERROR("udec & vout state wrong!!");
        }
#endif
        FlushAllFilters();
        PurgeAllFilters();
    }else{
        mbUseAbsoluteTime = false;
    }
    AM_U64 seekTime = ms;
    err = mpDemuxer->Seek(seekTime);
    if (err != ME_OK) {
        SetState(STATE_ERROR);
        return err;
    }
    mSeekTime = seekTime;
    //if(oldState == STATE_STARTED)
    //{
        mpClockManager->ResumeClock();
        StopUDEC(mShared.mIavFd, 0, STOPFLAG_CLEAR);
        GetUdecState(mShared.mIavFd, &mShared.udec_state, &mShared.vout_state, &mShared.error_code);
        AllFiltersMsg(IActiveObject::CMD_BEGIN_PLAYBACK);
    //}

#ifdef _tmp_fix_ucode_cannot_seek_when_paused_
    if(oldState == STATE_PAUSED)
    {
        PauseAllFilters();
        mpClockManager->PauseClock();
    }
#endif

#else

    StopAllFilters();
    PurgeAllFilters();
    err = mpDemuxer->Seek(ms);
    if (err != ME_OK) {
        SetState(STATE_ERROR);
        return err;
    }
    RunAllFilters();
#endif

    SetState(oldState);
    return ME_OK;
}

AM_ERR CPBEngine::Step()
{
    if (!mpVoutController) {
        AMLOG_ERROR("No voutcontroller, cannot switch to setp mode.\n");
        return ME_ERROR;
    }

    if (STATE_STARTED == mState) {
        AMLOG_INFO("switch into STEP mode.\n");
        mState = STATE_STEP;
    } else if (mState != STATE_STEP) {
        AMLOG_ERROR("cannot switch to setp mode in state %d.\n", mState);
        return ME_ERROR;
    }

    mpVoutController->Step();
    return ME_OK;
}

AM_ERR CPBEngine::StartwithStepMode(AM_UINT startCnt)
{
    mShared.mbStartWithStepMode = 1;
    mShared.mStepCnt = startCnt;
    AMLOG_INFO("pb-engine, start with step mode, cnt = %d.\n", startCnt);
    return ME_OK;
}

AM_ERR CPBEngine::SetTrickMode(PB_DIR dir, PB_SPEED speed)
{
	return ME_OK;
}

AM_ERR CPBEngine::SetDeinterlaceMode(AM_INT bEnable)
{
    mShared.bEnableDeinterlace = (AM_BOOL)bEnable;
    return ME_OK;
}

AM_ERR CPBEngine::GetDeinterlaceMode(AM_INT *pbEnable)
{
    *pbEnable = (AM_INT)mShared.bEnableDeinterlace;
    return ME_OK;
}

AM_ERR CPBEngine::SetLooping(int loop)
{
    mLoop = loop;
    return ME_OK;
}

void CPBEngine::MsgProc(AM_MSG& msg)
{
	AMLOG_DEBUG("*** CPBEngine::MsgProc: msg.code %d start.\n", msg.code);
	AUTO_LOCK(mpMutex);

	if (!IsSessionMsg(msg))
		return;

	switch (msg.code) {
	case MSG_PARSE_MEDIA_DONE:
		AMLOG_INFO("MSG_PARSE_MEDIA_DONE\n");
		HandleParseMediaDone();
		break;
	case MSG_READY:
		AMLOG_INFO("-----MSG_READY-----\n");
		HandleReadyMsg(msg);
		break;
	case MSG_EOS:
		AMLOG_INFO("-----MSG_EOS-----\n");
		HandleEOSMsg(msg);
		break;
    case MSG_AVSYNC:
		AMLOG_INFO("-----MSG_AVSYNC-----\n");
		HandleSyncMsg(msg);
		break;

    case MSG_SOURCE_FILTER_BLOCKED:
        AMLOG_INFO("-----MSG_SOURCE_FILTER_BLOCKED-----\n");
        HandleSourceFilterBlockedMsg(msg);
        break;

    case MSG_ERROR:
        HandleErrorMsg(msg);
        break;
	default:
        break;
    }
	AMLOG_DEBUG("*** CPBEngine::MsgProc: msg.code %d done.\n", msg.code);

}

void CPBEngine::HandleSourceFilterBlockedMsg(AM_MSG& msg) {
    AllFiltersMsg(IActiveObject::CMD_SOURCE_FILTER_BLOCKED);
}

void CPBEngine::HandleSyncMsg(AM_MSG& msg)
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
        AllFiltersClearFlag(SYNC_FLAG);
        mShared.mbAlreadySynced = 1;
        AllFiltersCmd(IActiveObject::CMD_AVSYNC);
    }
}
void CPBEngine::HandleForceSyncMsg(AM_MSG& msg) {
    mShared.mbAlreadySynced = 1;
    AllFiltersCmd(IActiveObject::CMD_AVSYNC);
}

void CPBEngine::HandleErrorMsg(AM_MSG& msg) {
    AM_ERR err;
    AM_UINT e_code = mShared.error_code;
    AM_UINT mw_behavior;

    if ((AM_ERR)msg.p0 == ME_VIDEO_DATA_ERROR) {
        AMLOG_INFO("[ErrorHandling]: disable video\n");
        //mpDemuxer->EnableVideo(false);
        HandleForceSyncMsg(msg);
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

            //clear shared data
            mShared.mbAlreadySynced = 0;
            mShared.mbVideoFirstPTSVaild = false;
            mShared.mbAudioFirstPTSVaild = false;
            mShared.videoFirstPTS = 0;
            mShared.audioFirstPTS = 0;

            //try seek to skip the error place
            AM_ASSERT(mpMasterRenderer);
            AM_U64 absoluteTimeMs = 0, relativeTimeMs = 0;
            mpMasterRenderer->GetCurrentTime(absoluteTimeMs, relativeTimeMs);
            //flush all
            AMLOG_INFO("[ErrorHandling]: get current time %llu, start flush active_pb_engine.\n", absoluteTimeMs);
            mpClockManager->PauseClock();
            StopUDEC(mShared.mIavFd, 0, STOPFLAG_FLUSH);
            FlushAllFilters();
            PurgeAllFilters();
            absoluteTimeMs += mTrySeekTimeGap;
            //try skip error place
            AMLOG_INFO("[ErrorHandling]: flush done, seek to time %llu.\n", absoluteTimeMs);
            err = mpDemuxer->Seek(absoluteTimeMs);
            if (err != ME_OK) {
                AMLOG_ERROR("[ErrorHandling]: seek error, pb-engine go to error state.\n");
                SetState(STATE_ERROR);
                return;
            }
            //restart playing
            AMLOG_INFO("[ErrorHandling]: seek to time %llu done.\n", absoluteTimeMs);
            mSeekTime = absoluteTimeMs;
            mpClockManager->ResumeClock();
            StopUDEC(mShared.mIavFd, 0, STOPFLAG_CLEAR);
            GetUdecState(mShared.mIavFd, &mShared.udec_state, &mShared.vout_state, &mShared.error_code);
            AMLOG_INFO("[ErrorHandling]: Clear stop done, udec_state %d, vout_state %d, error_code %d.\n", mShared.udec_state, mShared.vout_state, mShared.error_code);
            AllFiltersMsg(IActiveObject::CMD_BEGIN_PLAYBACK);
            SetState(STATE_STARTED);
        } else if (mw_behavior == MW_Bahavior_ExitPlayback) {
            //clear shared data
            mShared.mbAlreadySynced = 0;
            mShared.mbVideoFirstPTSVaild = false;
            mShared.mbAudioFirstPTSVaild = false;
            mShared.videoFirstPTS = 0;
            mShared.audioFirstPTS = 0;

            AMLOG_INFO("[ErrorHandling]: Stop current playback.\n");
            StopUDEC(mShared.mIavFd, 0, STOPFLAG_STOP);
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

void CPBEngine::HandleReadyMsg(AM_MSG& msg)
{
    IFilter *pFilter = (IFilter*)msg.p1;

    AM_ASSERT(pFilter != NULL);

    AM_UINT index;
    if (FindFilter(pFilter, index) != ME_OK)
        return;

    SetReady(index);
    if (AllRenderersReady()) {
        AM_PRINTF("---All renderers ready----\n");
        mpClockManager->StartClock();
        StartAllRenderers();
    }
}


void CPBEngine::HandleEOSMsg(AM_MSG& msg)
{
    IFilter *pFilter = (IFilter*)msg.p1;

    AM_ASSERT(pFilter != NULL);

    AM_UINT index;
    if (FindFilter(pFilter, index) != ME_OK)
        return;

    SetEOS(index);
    if (AllRenderersEOS()) {
        mFatalErrorCnt = 0;
        mTrySeekTimeGap = 2000;
        AM_PRINTF("All renderers eos\n");
        if(mLoop)
        {
            AM_PRINTF("Looping~\n");
            PausePlay();
            Seek(0);
            ResumePlay();
        }else{
        mState = STATE_COMPLETED;
        PostAppMsg(MSG_EOS);
        AM_MSG msgEOS;
        msgEOS.code = IMediaControl :: MSG_PLAYER_EOS;
        mAppMsgProc(mAppMsgContext, msgEOS);
        }
    }

}

void CPBEngine::HandleParseMediaDone()
{
    AM_ERR err;

    AM_ASSERT(mState == STATE_INITIALIZED);

    if (mState != STATE_INITIALIZED)
        return;

    filter_entry *pf = mpRecognizer->GetFilterEntry();
    if (pf == NULL) {
        AM_PRINTF("No parser found\n");
        return EnterErrorState(ME_NOT_EXIST);
    }

    // build the graph

    // create source filter
   IFilter *pFilter = pf->create((IEngine*)(inherited*)this);
    if (pFilter == NULL) {
        AM_ERROR("Create %s failed\n", pf->pName);
        return EnterErrorState(ME_NOT_EXIST);
    }

    // get IDemuxer
    mpDemuxer = IDemuxer::GetInterfaceFrom(pFilter);
    if (mpDemuxer == NULL) {
        AM_ERROR("No IDemuxer\n");
        AM_DELETE(pFilter);
        return EnterErrorState(ME_NO_INTERFACE);
    }

    // load file
    err = mpDemuxer->LoadFile(mFileName, mpRecognizer->GetParser().context);
    if (err != ME_OK) {
        AM_ERROR("LoadFile failed\n");
        AM_DELETE(pFilter);
        return EnterErrorState(err);
    }
    mpRecognizer->ClearParser();

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
        return EnterErrorState(ME_TOO_MANY);
    }

    // render this filter (recursive)
    err = RenderFilter(pFilter);
    if (err != ME_OK) {
        return EnterErrorState(err);
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

    if (mShared.mbStartWithStepMode && mpVoutController) {
        AMLOG_INFO("Start with step mode.\n");
        mpVoutController->Step();
    }

/*#ifdef AM_DEBUG
    AM_ASSERT(mnFilters<DPB_MAX_AO_NUM);
    AMLOG_INFO("set filters' log level/log option.\n");
    //log-config
    AM_INT index = 1;
    for (; index <mnFilters+1; index++) {
        ((CFilter*)mFilters[index -1].pFilter)->SetLogLevelOption((ELogLevel)mShared.logConfig[index].log_level, mShared.logConfig[index].log_option, mShared.logConfig[index].log_output);
        AMLOG_INFO("filter %s, index=%d, log level = %d, log option = %x, log output = %x.\n",((CActiveFilter*)mFilters[index -1].pFilter)->GetName(), index-1, mShared.logConfig[index].log_level, mShared.logConfig[index].log_option, mShared.logConfig[index].log_output);
    }
    SetLogLevelOption((ELogLevel)mShared.logConfig[0].log_level, mShared.logConfig[0].log_option, mShared.logConfig[0].log_output);//set engine's self log config
#endif*///pre set log config in constructor of engine/filters for print func

    // set state
    SetState(STATE_PREPARED);
    PostAppMsg(MSG_STATE_CHANGED);

}

void CPBEngine::CheckFileType(const char *pFileType)
{
    // default
    mbUseAbsoluteTime = true;

    if (!pFileType) {
        return ;
    }

    // rtsp protocol
    if (!strncmp(pFileType, "rtsp", 4)) {
        mbUseAbsoluteTime = false;
        AMLOG_INFO("CPBEngine::CheckFileType: rtsp protocol.\n");
        return ;
    }

    // flac type
    if (!strncmp(pFileType, "flac", 4)) {
        mbUseAbsoluteTime = false;
        AMLOG_INFO("CPBEngine::CheckFileType: flac type.\n");
        return ;
    }
}

AM_ERR CPBEngine::RenderFilter(IFilter *pFilter)
{
    AMLOG_INFO("Render %s\n", GetFilterName(pFilter));

    IFilter::INFO info;
    pFilter->GetInfo(info);

    //find vout controller
    if (!mpVoutController) {
        mpVoutController = IVideoOutput::GetInterfaceFrom(pFilter);
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
			AM_ERROR("%s has no output pin %d\n", GetFilterName(pFilter), i);
			return ME_NO_INTERFACE;
		}

		// query the media format on the pin
		CMediaFormat *pMediaFormat = NULL;
		AM_ERR err = pOutput->GetMediaFormat(pMediaFormat);
		if (err != ME_OK) {
			AM_ERROR("%s[%d]::GetMediaFormat failed\n", GetFilterName(pFilter), i);
			return err;
		}

		// find a filter entry, which declares it can accept the media
		filter_entry *pf = CMediaRecognizer::FindMatch(*pMediaFormat);
		if (pf == NULL) {
			AM_ERROR("Cannot find filter to render %s[%d]\n", GetFilterName(pFilter), i);
			return ME_NO_INTERFACE;
		}

		// create the filter
		IFilter *pNextFilter = pf->create((IEngine*)(inherited*)this);
		if (pNextFilter == NULL) {
			AM_ERROR("Cannot create filter %s\n", pf->pName);
			return ME_NOT_EXIST;
		}

		// save the filter
		err = AddFilter(pNextFilter);
		if (err != ME_OK)
			return ME_TOO_MANY;

		// get the first input pin
		IPin *pInput = pNextFilter->GetInputPin(0);
		if (pInput == NULL) {
			AM_ERROR("Filter %s has no input pin\n", GetFilterName(pNextFilter));
			return ME_NOT_EXIST;
		}

		// connect with it
		AMLOG_INFO("try to connect %s with %s\n", GetFilterName(pFilter), GetFilterName(pNextFilter));
		err = CreateConnection(pOutput, pInput);
		if (err != ME_OK) {
			return err;
		}
		AMLOG_INFO("connected\n");

		// render the next filter
		err = RenderFilter(pNextFilter);
		if (err != ME_OK)
			return err;
	}

	return ME_OK;
}

void CPBEngine::EnterErrorState(AM_ERR err)
{
	AMLOG_INFO("PB engine enters ERROR state\n");
	mError = err;
	SetState(STATE_ERROR);
	PostAppMsg(MSG_STATE_CHANGED);
}

AM_ERR CPBEngine::ChangeInputCenter(AM_INT input_center_x, AM_INT input_center_y)
{
    if (mpVoutController) {
        return mpVoutController->ChangeInputCenter(input_center_x, input_center_y);
    }
    AMLOG_ERROR("CPBEngine::ChangeInputCenter, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::SetDisplayPositionSize(AM_INT vout, AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height)
{
    if (mpVoutController) {
        return mpVoutController->SetDisplayPositionSize(vout, pos_x, pos_y, width, height);
    }
    AMLOG_ERROR("CPBEngine::SetDisplayPositionSize, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::SetDisplayPosition(AM_INT vout, AM_INT pos_x, AM_INT pos_y)
{
    if (mpVoutController) {
        return mpVoutController->SetDisplayPosition(vout, pos_x, pos_y);
    }
    AMLOG_ERROR("CPBEngine::SetDisplayPosition, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::SetDisplaySize(AM_INT vout, AM_INT width, AM_INT height)
{
    if (mpVoutController) {
        return mpVoutController->SetDisplaySize(vout, width, height);
    }
    AMLOG_ERROR("CPBEngine::SetDisplaySize, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::GetDisplayPositionSize(AM_INT vout, AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height)
{
    if (mpVoutController) {
        return mpVoutController->GetDisplayPositionSize(vout, pos_x, pos_y, width, height);
    }
    AMLOG_ERROR("CPBEngine::GetDisplayPositionSize, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::GetDisplayDimension(AM_INT vout, AM_INT* width, AM_INT* height)
{
    if (mpVoutController) {
        return mpVoutController->GetDisplayDimension(vout, width, height);
    }
    AMLOG_ERROR("CPBEngine::GetDisplayDimension, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::GetVideoPictureSize(AM_INT* width, AM_INT* height)
{
    if (mpVoutController) {
        return mpVoutController->GetVideoPictureSize(width, height);
    }
    AMLOG_ERROR("CPBEngine::GetVideoPictureSize, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::GetCurrentVideoPictureSize(AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height)
{
    if (mpVoutController) {
        return mpVoutController->GetCurrentVideoPictureSize(pos_x, pos_y, width, height);
    }
    AMLOG_ERROR("CPBEngine::GetCurrentVideoPictureSize, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::VideoDisplayZoom(AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height)
{
    if (mpVoutController) {
        return mpVoutController->VideoDisplayZoom(pos_x, pos_y, width, height);
    }
    AMLOG_ERROR("CPBEngine::VideoDisplayZoom, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::SetDisplayRotation(AM_INT vout, AM_INT degree)
{
    if (mpVoutController) {
        return mpVoutController->SetDisplayRotation(vout, degree);
    }
    AMLOG_ERROR("CPBEngine::SetDisplayRotation, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::SetDisplayFlip(AM_INT vout, AM_INT flip)
{
    if (mpVoutController) {
        return mpVoutController->SetDisplayFlip(vout, flip);
    }
    AMLOG_ERROR("CPBEngine::SetDisplayFlip, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::SetDisplayMirror(AM_INT vout, AM_INT mirror)
{
    if (mpVoutController) {
        return mpVoutController->SetDisplayMirror(vout, mirror);
    }
    AMLOG_ERROR("CPBEngine::SetDisplayMirror, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::EnableVout(AM_INT vout, AM_INT enable)
{
    if (mpVoutController) {
        return mpVoutController->EnableVout(vout, enable);
    }
    AMLOG_ERROR("CPBEngine::EnableVout, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::EnableOSD(AM_INT vout, AM_INT enable)
{
    if (mpVoutController) {
        return mpVoutController->EnableOSD(vout, enable);
    }
    AMLOG_ERROR("CActivePBEngine::EnableOSD, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::EnableVoutAAR(AM_INT enable)
{
    return ME_NO_IMPL;
}

AM_ERR CPBEngine::SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h)
{
    if (mpVoutController) {
        return mpVoutController->SetVideoSourceRect(x, y, w, h);
    }
    AMLOG_ERROR("CPBEngine::SetVideoSourceRect, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h)
{
    if (mpVoutController) {
        return mpVoutController->SetVideoDestRect(vout_id, x, y, w, h);
    }
    AMLOG_ERROR("CPBEngine::SetVideoDestRect, NULL mpVoutController.\n");
    return ME_ERROR;
}

AM_ERR CPBEngine::SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y)
{
    if (mpVoutController) {
        return mpVoutController->SetVideoScaleMode(vout_id, mode_x, mode_y);
    }
    AMLOG_ERROR("CPBEngine::SetVideoScaleMode, NULL mpVoutController.\n");
    return ME_ERROR;
}
void CPBEngine::GetDecType(DECSETTING decSetDefault[])
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
void CPBEngine::SetDecType(DECSETTING decSetCur[])
{
    int i = 0;
    while(i != CODEC_NUM) {
        (((SConsistentConfig *)mpOpaque)->decSet[i]).codecId = decSetCur[i].codecId;
        //Check if the setting within the ability of the decoder
        if((decSetCur[i].dectype & mdecCap[i]) != 0)
            (((SConsistentConfig *)mpOpaque)->decSet[i]).dectype = decSetCur[i].dectype;
        i++;
    }
    return;
}
void CPBEngine::Reset()
{
    //reset the parameters
    ClearGraph();
    mpVoutController = NULL;
    mpDemuxer = NULL;
    mpMasterRenderer = NULL;
    mpMasterFilter = NULL;
    mMostPriority = 0;
    mSeekTime = 0;
    mLoop = 0;
    mbUseAbsoluteTime = true;
    mShared.mbAlreadySynced = 0;
    AMLOG_DEBUG("---reset pb_engine---\n");
}

#ifdef AM_DEBUG
//obsolete, remove later
AM_ERR CPBEngine::SetLogConfig(AM_UINT index, AM_UINT level, AM_UINT option)
{
/*
	if(mnFilters <= index || level > LogAll) {
		AM_INFO("CPBEngine::SetLogConfig, invalid arguments: index=%d,level=%d,option=%d.\n", index, level, option);
		return ME_BAD_PARAM;
	}

	if(!mFilters[index].pFilter) {
		AM_INFO("CPBEngine::SetLogConfig, mFilters[index].pFilter not ready, must have errors.\n");
		return ME_ERROR;
	}

	((CFilter*)mFilters[index].pFilter)->SetLogLevelOption((ELogLevel)mShared.logConfig[index].log_level, mShared.logConfig[index].log_option, 0);
	*/
	return ME_OK;
}

AM_ERR CPBEngine::SetPBProperty(AM_UINT prop, AM_UINT value)
{
    switch (prop) {
        case IPBControl::DEC_PROPERTY_SPEEDUP_REALTIME_PLAYBACK:
            AMLOG_INFO("post speedup cmd.\n");
            AllFiltersMsg(IActiveObject::CMD_REALTIME_SPEEDUP);
            break;

        default:
            AM_ERROR("BAD prop %d, value %d.\n", prop, value);
            break;
    }
    return ME_OK;
}

void CPBEngine::PrintState()
{
    AM_UINT i = 0;
    AM_UINT udec_state, vout_state, error_code;
    AMLOG_INFO(" pb-engine's state is %d.\n", mState);
    for (i = 0; i < mnFilters; i ++) {
        mFilters[i].pFilter->PrintState();
    }
    GetUdecState(mShared.mIavFd, &udec_state, &vout_state, &error_code);
    AMLOG_INFO(" Udec state %d, Vout state %d, error_code 0x%x.\n", udec_state, vout_state, error_code);
}
#endif


