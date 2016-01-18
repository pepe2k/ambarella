/**
 * active_mdec_engine.cpp
 *
 * History:
 *    2011/12/08- [GangLiu] created file
 *    2012/3/30- [Qingxiong Z] modify file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "active_mdec_engine"
//#define AMDROID_DEBUG
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>

#include "general_header.h"
#include "general_interface.h"
#include "general_demuxer_filter.h"
#include "general_decoder_filter.h"
#include "general_transc_audiosink_filter.h"
#include "general_renderer_filter.h"
#include "general_transcoder_filter.h"
#include "general_muxer_save.h"
#include "general_audio_manager.h"
#include "g_render_out.h"

#include "am_external_define.h"
#include "am_mdec_if.h"
#include "mdec_if.h"
#include "general_layout_manager.h"
#include "general_pipeline.h"
#include "active_mdec_engine.h"

#include "motiondetectreceiver.h"

#if NEW_RTSP_CLIENT
#include "amf_rtspclient.h"
#endif

#if PLATFORM_LINUX
#include "rtsp_vod.h"
#include "rtsp_audioproxy.h"
#endif

IMDecControl* CreateActiveMDecControl(void *audiosink, CParam& param)
{
    //return NULL;
    return (IMDecControl*)CActiveMDecEngine::Create(audiosink, param);
}
//-----------------------------------------------------------------------
//
// CActiveMDecEngine
//
//-----------------------------------------------------------------------
CActiveMDecEngine* CActiveMDecEngine::Create(void *audiosink, CParam& par)
{
    CActiveMDecEngine* result = new CActiveMDecEngine(audiosink);
    if (result && result->Construct(par) != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CActiveMDecEngine::CActiveMDecEngine(void *audiosink):
    inherited("active_mdec_engine"),
    mbRun(true),
    mFatalErrorCnt(0),
    mpCurSource(NULL),
    mpDemuxer(NULL),
    mpVideoDecoder(NULL),
    mpAudioDecoder(NULL),
    mpRenderer(NULL),
    mpTranscoder(NULL),
    mpTansAudioSink(NULL),
    mpAudioSinkFilter(NULL),
    mpMuxer(NULL),
    mpAudioMan(NULL),
    mpClockManager(NULL),
    mpAudioSink(audiosink),//no audio now
    mpMDReceiver(NULL),
    mEditPlayAudio(-1),
    mHdInfoIndex(-1),
    mbNohd(AM_FALSE),
    mbOnNVR(AM_TRUE),
    mbNetStream(AM_FALSE),
    mbDisconnect(AM_FALSE),
    mSwitchWait(SWITCH_WAIT_NOON),
    mAudioWait(AUDIO_WAIT_NOON),
    mBufferNum(0),
    mNvrPbMode(-1),
    mbSeeking(AM_FALSE),
    mbTalkBackStreamOn(AM_FALSE),
    mTalkBackIndexMask(0),
    mbEnableTanscode(AM_FALSE),
    mStartIndex(-1),
    mbTrickPlayDisabledAudio(0),
    mbRTSPServerStarted(0)
{
#if PLATFORM_LINUX
    pthread_mutex_init(&ts_mutex_,NULL);
    hash_table_ =(void*) new MyHashTable();
    pthread_mutex_init(&audio_mutex_,NULL);
    hash_table_audio_ =(void*) new MyHashTable();
#endif
    strcpy(mTalkBackURL, "");
    strcpy(mTalkBackName, "talk.back");
}

AM_ERR CActiveMDecEngine::Construct(CParam& par)
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

#if NEW_RTSP_CLIENT
    AmfRtspClientManager::instance()->start_service();
#endif

    mpMsgSys = CMsgSys::Init();
    if(mpMsgSys == NULL)
        return ME_NO_MEMORY;

//    AM_INT i = 0;
    mpConfig = new CGConfig();
    if(mpConfig == NULL)
        return ME_NO_MEMORY;

    mpMdecInfo = new MdecInfo();
    if(mpMdecInfo == NULL)
        return ME_NO_MEMORY;

    err = SetGConfig(par);
    if (err != ME_OK)
        return err;
    if ((mpLayout = CGeneralLayoutManager::Create(mpConfig, mpMdecInfo)) == NULL)
        return ME_ERROR;

    if ((mpPipeLine = CPipeLineManager::Create(this, AM_TRUE)) == NULL)
        return ME_ERROR;

    if ((mpClockManager = CClockManager::Create()) == NULL)
        return ME_ERROR;

    err = InitDspAudio(par);
    if (err != ME_OK)
        return err;

    err = BuildMDecGragh();
    if (err != ME_OK)
        return err;

#if PLATFORM_LINUX
    RtspMediaSession::start_rtsp_server();
    mbRTSPServerStarted = 1;
#endif
    mpWorkQ->SetThreadPrio(1, 55);
    mpWorkQ->Run();
    DSetModuleLogConfig(LogModuleVEEngine);
    return ME_OK;
}

void CActiveMDecEngine::Delete()
{
    AM_INFO("CActiveMDecEngine Delete\n");
//    AM_INT i = 0;
    AM_DELETE(mpMsgSys);
    mpMsgSys = NULL;

    if(mpMDReceiver) {
        RemoveMotionDetectReceiver();
    }


    AM_DELETE(mpMuxer);
    AM_DELETE(mpAudioMan);
    AM_DELETE(mpLayout);
    AM_DELETE(mpPipeLine);
    if (mpWorkQ) {
        mpWorkQ->SendCmd(CMD_STOP);
    }
    if (mpClockManager) {
        mpClockManager->StopClock();
        mpClockManager->SetSource(NULL);
    }
    AM_DELETE(mpClockManager);
    mpClockManager = NULL;
    DeInitDspAudio();
#if PLATFORM_LINUX
    if(hash_table_audio_){//free hash_table_audio_
        pthread_mutex_lock(&audio_mutex_);
        while(1){
            RtspAudioProxySession *session = (RtspAudioProxySession *)(((MyHashTable*)hash_table_audio_)->RemoveNext());
            if(!session){
                break;
            }
            delete session;
        }
        pthread_mutex_unlock(&audio_mutex_);
        pthread_mutex_destroy(&audio_mutex_);
        delete (MyHashTable*)hash_table_audio_,hash_table_audio_ = NULL;
    }
#endif

#if NEW_RTSP_CLIENT
    AmfRtspClientManager::instance()->stop_service();
#endif

    if(mpConfig)
        delete mpConfig;

    if(mpMdecInfo)
        delete mpMdecInfo;

    inherited::Delete();
    //AM_INFO("CActiveMDecEngine Delete Done\n");
}

AM_ERR CActiveMDecEngine::DeInitDspAudio()
{
    AM_DeleteDSPHandler(&mShared);
    mShared.udecHandler = NULL;
    mShared.mIavFd = -1;
    mShared.mbIavInited = 0;
    mShared.voutHandler = NULL;
    CVideoRenderOut::Clear();

    AM_DELETE(mpConfig->audioConfig.audioHandle);
    mpConfig->audioConfig.audioHandle = NULL;
    CAudioRenderOut::Clear();
    return ME_OK;
}

CActiveMDecEngine::~CActiveMDecEngine()
{
    AM_INFO("~CActiveMDecEngine Start.\n");
#if PLATFORM_LINUX
    if(hash_table_audio_){//free hash_table_audio_
        pthread_mutex_lock(&audio_mutex_);
        while(1){
            RtspAudioProxySession *session = (RtspAudioProxySession *)(((MyHashTable*)hash_table_audio_)->RemoveNext());
            if(!session){
                break;
            }
            delete session;
        }
        pthread_mutex_unlock(&audio_mutex_);
        pthread_mutex_destroy(&audio_mutex_);
        delete (MyHashTable*)hash_table_audio_,hash_table_audio_ = NULL;
    }
    if (mbRTSPServerStarted) {
        RtspMediaSession::stop_rtsp_server();
        mbRTSPServerStarted = 0;
    }
    pthread_mutex_lock(&ts_mutex_);
    //TODO
    delete (MyHashTable*)hash_table_,hash_table_ = NULL;
    pthread_mutex_unlock(&ts_mutex_);
    pthread_mutex_destroy(&ts_mutex_);
#endif
    AM_INFO("~CActiveMDecEngine Done.\n");
}

void *CActiveMDecEngine::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IMDecControl)
        return (IMDecControl*)this;
    if (refiid == IID_IMDECEngine)
        return (IMDECEngine*)this;
    if (refiid == IID_IMSGSYS)
        return mpMsgSys;
    return inherited::GetInterface(refiid);
}


void *CActiveMDecEngine::QueryEngineInterface(AM_REFIID refiid)
{
    if (refiid == IID_IClockManager)
        return mpClockManager;

    if (refiid == IID_IAudioSink)
        return mpAudioSink;

    return NULL;
}

AM_ERR CActiveMDecEngine::SetGConfig(CParam& par)
{
    //par[0] for dspinstance num
    //par[1] for gobal flag
    AM_INFO("SetGConfig %d\n", par[DISPLAY_LAYOUT]);
    AM_INT flag = par[GLOBAL_FLAG];
    if(flag & USING_FOR_NET_PB){
        mbNetStream = AM_TRUE;
    }
    mpConfig->dspConfig.dspNumRequested = par[REQUEST_DSP_NUM];
    if(par[DSP_MAX_FRAME_NUM] <= 0){
        AM_ASSERT(0);
        return ME_ERROR;
    }
    mpConfig->dspConfig.eachDecBufferNum = par[DSP_MAX_FRAME_NUM];
    mpConfig->dspConfig.preBufferLen = par[DSP_PRE_BUFFER_LEN];
    if(mpConfig->dspConfig.preBufferLen > 0){
        if(mpConfig->dspConfig.preBufferLen >= (par[DSP_MAX_FRAME_NUM]-5))
            mpConfig->dspConfig.preBufferLen = (par[DSP_MAX_FRAME_NUM]-5);
        mpConfig->dspConfig.enDspBufferCtrl = 1;
    }else if(mpConfig->dspConfig.preBufferLen < 0){
        //default disable this feature
        mpConfig->dspConfig.preBufferLen = (par[DSP_MAX_FRAME_NUM]-5);
        mpConfig->dspConfig.enDspBufferCtrl = 0;
    }else if(mpConfig->dspConfig.preBufferLen == 0){
        mpConfig->dspConfig.enDspBufferCtrl = 0;
    }
    //flag |= FILL_PES_SSP_HEADER;
    mpConfig->globalFlag = flag;

    mpConfig->mInitialLayout = par[DISPLAY_LAYOUT];
    if (par[DISPLAY_LAYOUT] > 0) {
        mpConfig->mbUseCustomizedDisplayLayout = 1;
        mpConfig->mCustomizedDisplayLayout = par[DISPLAY_LAYOUT];
    } else {
        mpConfig->mbUseCustomizedDisplayLayout = 0;
        mpConfig->mCustomizedDisplayLayout = 0;
    }

    //set muxer config
    mpMuxer = CGeneralMuxer::Create(mpConfig);
    if(mpMuxer == NULL){
        AM_ASSERT(0);
        //mpConfig->globalFlag |= DISABLE_ALL_SAVE_STREAM;
    }else{
        //AM_INFO("%p, %p, cc1\n", mpMuxer, mpConfig->mainMuxer);
        //skychen, 2012_10_10
        mpMuxer->mDuration = (AM_UINT)par[AUTO_SEPARATE_FILE_DURATION];
        mpMuxer->mMaxFileCount = (AM_UINT)par[AUTO_SEPARATE_FILE_MAXCOUNT];
        mpConfig->mainMuxer->SetGeneralMuxer(mpMuxer);
        //AM_INFO("dONE\n");
    }
    //audio system
    mpAudioMan = CGeneralAudioManager::Create(mpConfig);
    if(mpAudioMan == NULL){
        AM_ASSERT(0);
    }
    mpConfig->audioManager = mpAudioMan;

    if(par[NET_BUFFER_BEGIN_NUM] < 0){
        AM_ASSERT(0);
        return ME_ERROR;
    }
    mBufferNum = par[NET_BUFFER_BEGIN_NUM];
    if(mBufferNum > 0)
        mpConfig->globalFlag |= BUFFER_TO_IDR_OR_NOT_JUST_TEST_FOR_DEMUXER;

    if(mpConfig->globalFlag & NO_HD_WIN_NVR_PB){
        mbNohd = AM_TRUE;
    }else{
        mbNohd = AM_FALSE;
    }
    if(par[VOUT_MASK] == 0x1){
        mpConfig->globalFlag |= NVR_ONLY_SHOW_ON_LCD;
    }
    //todo
    mpConfig->engineptr = (IEngine*)this;
    //mpConfig->globalFlag |= NOTHING_DONOTHING;
    //mpConfig->globalFlag |= NOTHING_NOFLUSH;
    mpConfig->mDecoderCap = par[DECODER_CAP];
    if(mpConfig->globalFlag & HAVE_TRANSCODE){
        mbEnableTanscode = AM_TRUE;
    }
    return ME_OK;
}

/*
bool CActiveMDecEngine::allState(MD_DEC_STATE state)
{
    AM_INT i = 0;
    for(;i<StreamNums;i++){
        if(mInfo.decstate[i]!= state)
            return false;
    }
    return true;
}

void CActiveMDecEngine::SetdecState(MD_DEC_STATE state, AM_INT index)
{
    AMLOG_INFO("Dec[%d] Enter state %d\n",index, state);
    switch(state){
        case STATE_IDLE:{
            mInfo.decstate[index] = STATE_IDLE;
            if(allState(STATE_IDLE)){
                SetState(STATE_ALL_IDLE);
            }
        }break;
        case STATE_PREPARED:{
            AM_ASSERT(mInfo.decstate[index] == STATE_IDLE);
            mInfo.decstate[index] = STATE_PREPARED;
            SetState(STATE_INITIALIZED);
        }break;
        case STATE_STARTED:{
            AM_ASSERT(mInfo.decstate[index] == STATE_PREPARED);
            mInfo.decstate[index] = STATE_STARTED;
            SetState(STATE_RUN);
        }break;
        case STATE_STOPPED:{
            mInfo.decstate[index] = STATE_STOPPED;
            if(allState(STATE_STOPPED)){
                SetState(STATE_ALL_STOPPED);
            }
        }break;
        case STATE_ERROR:{
            mInfo.decstate[index] = STATE_ERROR;
            SetState(STATE_HAS_ERROR);
        }break;
        default:
            AM_ERROR("SetdecState %d.\n", state);
            break;
    }
}
*/

void CActiveMDecEngine::SetState(MDENGINE_STATE state)
{
    AMLOG_INFO("Engine enter state %d\n", state);
    mState = state;
    /*
    switch(state){
        case STATE_ALL_IDLE:{
            //AM_ASSERT(mInfo.state == STATE_ALL_STOPPED);
            mState = STATE_ALL_IDLE;
        }break;
        case STATE_INITIALIZED:{
            //AM_ASSERT(mInfo.state == STATE_ALL_IDLE);
            if(mInfo.state == STATE_ALL_IDLE) mInfo.state = STATE_INITIALIZED;
        }break;
        case STATE_RUN:{
            AM_ASSERT(mInfo.state == STATE_INITIALIZED || mInfo.state == STATE_RUN);
            mInfo.state = STATE_RUN;
        }break;
        case STATE_ALL_STOPPED:{
            AM_ASSERT(mInfo.state == STATE_RUN);
            mInfo.state = STATE_ALL_STOPPED;
        }break;
        case STATE_COMPLETED:{
            AM_ASSERT(mInfo.state == STATE_ALL_STOPPED);
            mInfo.state = STATE_COMPLETED;
        }break;
        case STATE_END:{
            AM_ASSERT(mInfo.state == STATE_COMPLETED);
            mInfo.state = STATE_END;
        }break;
        case STATE_HAS_ERROR:{
            mInfo.state = STATE_HAS_ERROR;
        }break;
        default:
            AM_ERROR("SetState %d.\n", state);
            break;
    }
    */
}
//----------------------------------------------------------
//
//Out Apies
//----------------------------------------------------------
AM_ERR CActiveMDecEngine::EditSourceDone()
{
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_EDIT_SOURCE_DONE;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    //double check for dynamic add, source is likely the winIndex
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::EditSource(AM_INT sourceGroup, const char* pFileName, AM_INT flag)
{
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_EDIT_SOURCE;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    //double check for dynamic add, source is likely the winIndex
    cmd.res32_1 = flag;
    cmd.res64_1 = sourceGroup;
    mpCurSource = pFileName;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

//need a api PerformSourceEdit
AM_ERR CActiveMDecEngine::BackSource(AM_INT sourceGroup, AM_INT orderLevel, AM_INT flag)
{
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_BACK_SOURCE;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    //double check for dynamic add, source is likely the winIndex
    cmd.res32_1 = flag;
    cmd.res64_1 = sourceGroup;
    cmd.res64_2 = orderLevel;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::DeleteSource(AM_INT sourceGroup, AM_INT flag)
{
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_DELETE_SOURCE;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    //double check for dynamic add, source is likely the winIndex
    cmd.res32_1 = flag;
    cmd.res64_1 = sourceGroup;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::AddSource(const char *pFileName, AM_INT sourceGroup, AM_INT flag)
{
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_ADD_SOURCE;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    //double check for dynamic add, source is likely the winIndex
    cmd.res32_1 = flag;
    cmd.res64_1 = sourceGroup;
    mpCurSource = pFileName;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

//SaveInputSource()/StopInputSource()  added for TsOverUdp/Rtp as inputSource
AM_ERR CActiveMDecEngine::SaveInputSource(char *streamName,char * inputAddress,int inputPort,int isRawUdp){
    AM_ERR err = ME_OK;
    StopInputSource(streamName);
#if PLATFORM_LINUX
    RtspMediaSession *ms = new RtspMediaSession();
    ms->setDestination(streamName, inputAddress,inputPort,isRawUdp);
    pthread_mutex_lock(&ts_mutex_);
    ((MyHashTable*)hash_table_)->Add(streamName,(void*)ms);
    pthread_mutex_unlock(&ts_mutex_);
#endif
    return err;
}

AM_ERR CActiveMDecEngine::StopInputSource(char *streamName){
    AM_ERR err = ME_OK;
#if PLATFORM_LINUX
    RtspMediaSession *ms;
    pthread_mutex_lock(&ts_mutex_);
    ms = (RtspMediaSession*)((MyHashTable*)hash_table_)->Lookup(streamName);
    if(ms){
        delete ms;
    }
    ((MyHashTable*)hash_table_)->Remove(streamName);
    pthread_mutex_unlock(&ts_mutex_);
#endif
    return err;
}

//added for forward audio, for bi-direction audio feature
AM_ERR CActiveMDecEngine::StartAudioProxy(const char *rtspUrl, const char* streamName){
    AM_ERR err = ME_OK;
    StopAudioProxy(rtspUrl);
#if PLATFORM_LINUX
    RtspAudioProxySession *ms = new RtspAudioProxySession(rtspUrl,streamName);
    pthread_mutex_lock(&audio_mutex_);
    ((MyHashTable*)hash_table_audio_)->Add(rtspUrl,(void*)ms);
    pthread_mutex_unlock(&audio_mutex_);
#endif
    return err;
}
AM_ERR CActiveMDecEngine::StopAudioProxy(const char *rtspUrl){
    AM_ERR err = ME_OK;
#if PLATFORM_LINUX
    RtspAudioProxySession *ms;
    pthread_mutex_lock(&audio_mutex_);
    ms = (RtspAudioProxySession*)((MyHashTable*)hash_table_audio_)->Lookup(rtspUrl);
    if(ms){
        delete ms;
    }
    ((MyHashTable*)hash_table_audio_)->Remove(rtspUrl);
    pthread_mutex_unlock(&audio_mutex_);
#endif
    return err;
}
//end

//this index should be same with the order your addsource.
//update to mdecInfo ndex on 11/28
AM_ERR CActiveMDecEngine::SaveSource(AM_INT index, const char* saveName, AM_INT flag)
{
    AM_INT infoIndex = -1, num = -1, i = 0;
    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        if(mpMdecInfo->unitInfo[i].isUsed == AM_TRUE)
            num++;
        if(num == index){
            infoIndex = i;
            break;
        }
    }
    if(infoIndex < 0){
        AM_INFO("Index failed on SaveSource\n");
        return ME_ERROR;
    }

    CGMuxerConfig* pConfig = mpConfig->mainMuxer;
    if(pConfig->CheckAction(infoIndex, AM_TRUE, flag) != ME_OK){
        AM_INFO("Setup %d Stream Rejected!\n", index);
        return ME_ERROR;
    }
    pConfig->InitUnitConfig(infoIndex, saveName, flag);
    //AM_INFO("CActiveMDecEngine::SaveSource Done\n");
    return ME_OK;
}

AM_ERR CActiveMDecEngine::StopSaveSource(AM_INT index, AM_INT flag)
{
    AM_INT infoIndex = -1, num = -1, i = 0;
    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        if(mpMdecInfo->unitInfo[i].isUsed == AM_TRUE)
            num++;
        if(num == index){
            infoIndex = i;
            break;
        }
    }
    if(infoIndex < 0){
        AM_INFO("Index failed on StopSaveSource\n");
        return ME_ERROR;
    }

    CGMuxerConfig* pConfig = mpConfig->mainMuxer;
    if(pConfig->CheckAction(infoIndex, AM_FALSE, flag) != ME_OK){
        AM_INFO("The %d Stream not save before!\n", index);
        return ME_ERROR;
    }
    pConfig->DeInitUnitConfig(infoIndex, flag);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::SelectAudio(AM_INT index)
{
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_SELECT_AUDIO;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    cmd.res32_1 = index;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::TalkBack(AM_UINT enable, AM_UINT winIndexMask, const char* url)
{
    AM_ERR err;
    CMD cmd;

    if(enable==1) {
        if(!strcmp(mTalkBackURL, url))
            AM_WARNING("TalkBack: mTalkBackURL has been set: %s!\n", mTalkBackURL);
        strcpy(mTalkBackURL, url);
    }
    cmd.code = MDCMD_AUDIO_TALKBACK;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    cmd.res32_1 = winIndexMask;
    cmd.flag2 = enable;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

//no change immediately
AM_ERR CActiveMDecEngine::ConfigWindow(AM_INT winIndex, CParam& param)
{
    AM_ERR err;
    err = param.CheckNum(4);
    if(err != ME_OK)
        return ME_BAD_PARAM;
    CMD cmd;
    cmd.code = MDCMD_CONFIG_WIN;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    cmd.pExtra = &param;
    cmd.res32_1 = winIndex;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::ConfigWinMap(AM_INT sourceIndex, AM_INT win, AM_INT zOrder)
{
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_CONFIG_WINMAP;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    cmd.res64_1 = sourceIndex;
    cmd.res64_2 = win;
    cmd.res32_1 = zOrder;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::PerformWinConfig()
{
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_PERFORM_WIN;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

inline AM_ERR CActiveMDecEngine::CheckActionByLayout(ACTION_TYPE type, AM_INT& index)
{
//    AM_ERR err;
//    AM_INT reIndex;
    if(mpLayout->CheckActionByLayout(type, index) != ME_OK){
        return ME_CLOSED;
    }
    return ME_OK;
}

//index = -1 for 4D1
AM_ERR CActiveMDecEngine::AutoSwitch(AM_INT index)
{
    AM_ERR err;
    CMD cmd;
    if(CheckActionByLayout((index < 0) ? ACTION_BACK : ACTION_SWITCH, index) != ME_OK){
        return ME_CLOSED;
    }
    cmd.code = MDCMD_AUTOSWITCH;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    cmd.flag = 0;
    if(index < 0){
        cmd.flag |= REVERT_TO_NVR;
    }
    cmd.res32_1 = index;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::SwitchSet(AM_INT index)
{
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_SWITCHSET;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    cmd.flag = 0;
    cmd.res32_1 = index;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::Prepare()
{
    CMD cmd;
    AM_ASSERT(mpCmdQueue);
    cmd.code = MDCMD_PREPARE;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return ME_OK;
}

AM_ERR CActiveMDecEngine::Start()
{
    AM_ERR err;
    CMD cmd;
    AM_ASSERT(mpCmdQueue);
    cmd.code = MDCMD_START;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::Stop()
{
    AM_ASSERT(mpCmdQueue);
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_STOP;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::DisconnectHw(AM_INT flag)
{
    AM_ASSERT(mpCmdQueue);
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_DISCONNECT;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::ReconnectHw(AM_INT flag)
{
    AM_ASSERT(mpCmdQueue);
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_RECONNECT;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::SetTranscode(AM_UINT w, AM_UINT h, AM_UINT br)
{
    AM_ERR err = ME_NO_INTERFACE;

    if(mbEnableTanscode && mpTranscoder){
        err = mpTranscoder->SetEncParam(w, h, br);
    }
    mShared.sTranscConfig.enc_w = w;
    mShared.sTranscConfig.enc_h = h;
    mShared.sTranscConfig.bitrate = br;
    return err;
}

AM_ERR CActiveMDecEngine::SetTranscodeBitrate(AM_INT kbps)
{
    AM_ERR err = ME_NO_INTERFACE;

    if(mbEnableTanscode && mpTranscoder){
        err = mpTranscoder->SetBitrate(kbps);
    }
    return err;
}

AM_ERR CActiveMDecEngine::SetTranscodeFramerate(AM_INT fps, AM_INT reduction)
{
    AM_ERR err = ME_NO_INTERFACE;

    if(mbEnableTanscode && mpTranscoder){
        err = mpTranscoder->SetFramerate(fps, reduction);
    }
    return err;
}

AM_ERR CActiveMDecEngine::SetTranscodeGOP(AM_INT M, AM_INT N, AM_INT interval, AM_INT structure)
{
    AM_ERR err = ME_NO_INTERFACE;

    if(mbEnableTanscode && mpTranscoder){
        err = mpTranscoder->SetGOP(M, N, interval, structure);
    }
    return err;
}

AM_ERR CActiveMDecEngine::TranscodeDemandIDR(AM_BOOL now)
{
    AM_ERR err = ME_NO_INTERFACE;

    if(mbEnableTanscode && mpTranscoder){
        err = mpTranscoder->DemandIDR(now);
    }
    return err;
}
AM_ERR CActiveMDecEngine::RecordToFile(AM_BOOL en, char* name)
{
    AM_ERR err = ME_NO_INTERFACE;

    if(mbEnableTanscode && mpTranscoder){
        err = mpTranscoder->RecordToFile(en, name);
    }
    return err;
}
AM_ERR CActiveMDecEngine::UploadNVRTranscode2Cloud(char* url, AM_INT flag)
{
    AM_ERR err = ME_NO_INTERFACE;

    if(mbEnableTanscode && mpTranscoder){
        err = mpTranscoder->UploadTranscode2Cloud(url, (UPLOAD_NVR_TRANSOCDE_FLAG)flag);
    }
    return err;
}

AM_ERR CActiveMDecEngine::StopUploadNVRTranscode2Cloud(AM_INT flag)
{
    AM_ERR err = ME_NO_INTERFACE;

    if(mbEnableTanscode && mpTranscoder){
        err = mpTranscoder->StopUploadTranscode2Cloud((UPLOAD_NVR_TRANSOCDE_FLAG)flag);
    }
    return err;
}

//AM_ERR CActiveMDecEngine::ClearGraph()

AM_ERR CActiveMDecEngine::GetMdecInfo(MdecInfo& info)
{
    AM_INT i = 0;
    AM_INT source;
    AM_ERR err = ME_OK;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    MdecInfo::MdecWinInfo* curWin = NULL;
    CParam64 pts(3);

    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        curWin = &(mpMdecInfo->winInfo[i]);
        if(curInfo->isUsed == AM_TRUE){
            err = mpDemuxer->QueryInfo(i, INFO_TIME_PTS, pts);
            if(err != ME_OK){
                AM_DBG("mpDemuxer->QueryInfo(%d) return %d, ", i, err);
                pts[0] = 0;
                pts[1] = 0;
                pts[2] = 0;
                }
            curInfo->curPts = pts[0];
            curInfo->length= pts[1];
            {
            //add for nvr app
                curInfo->isHided = mpConfig->demuxerConfig[i].hided;
                if(DEC_MAP(i)<0){
                    curInfo->progress = 0;
                }else{
                    AM_S64 starttime = pts[2];
                    err = mpVideoDecoder->QueryInfo(DEC_MAP(i), INFO_TIME_PTS, pts);
                    if(err != ME_OK){
                        AM_DBG("mpVideoDecoder->QueryInfo(%d->%d) return %d, ", i, DEC_MAP(i), err);
                        err = ME_OK;
                        pts[0] = 0;
                        pts[1] = 0;
                        pts[2] = 0;
                    }
                    //AM_INFO("%d: %lld, %lld\n", i, pts[2], starttime);
                    if(pts[2]<starttime) curInfo->progress = 0;
                    else curInfo->progress = pts[2]-starttime;
                }
                if(curInfo->is1080P==false){curInfo->sd2HDIdx = InfoHDBySD(i);}
                else{ curInfo->sd2HDIdx = -1;}
                //AM_INFO("%d: sd2HDIdx: %d %d.\n", i, curInfo->sd2HDIdx, curInfo->isHided);
            }
        }
        if(mpConfig->dspWinConfig.winConfig[i].winOffsetX >= 0){
            curWin->offsetX = mpConfig->dspWinConfig.winConfig[i].winOffsetX;
            curWin->offsetY = mpConfig->dspWinConfig.winConfig[i].winOffsetY;
            curWin->width = mpConfig->dspWinConfig.winConfig[i].winWidth;
            curWin->height = mpConfig->dspWinConfig.winConfig[i].winHeight;
            source = InfoChangeWinIndex(mpLayout->GetSourceGroupByWindow(i));
            curWin->dspRenIndex = mpConfig->indexTable[source].dsprenIndex;
            curWin->curIndex = mpConfig->indexTable[source].index;
        }
    }

    memcpy(&info, mpMdecInfo, sizeof(MdecInfo));
    return err;
}

AM_ERR CActiveMDecEngine::SeekTo(AM_U64 time, AM_INT index)
{
    AM_ASSERT(mpCmdQueue);
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_SEEKTO;
    cmd.flag = 0;
    cmd.res32_1 = index;
    cmd.res64_1 = time;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::PausePlay(AM_INT index)
{
    AM_ASSERT(mpCmdQueue);
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_PAUSEPLAY;
    cmd.flag = 0;
    if(index < 0)
        cmd.flag |= PAUSE_RESUME_STEP_HD;
    cmd.res32_1 = index;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::ResumePlay(AM_INT index)
{
    AM_ASSERT(mpCmdQueue);
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_RESUMEPLAY;
    cmd.flag = 0;
    if(index < 0)
        cmd.flag |= PAUSE_RESUME_STEP_HD;
    cmd.res32_1 = index;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::StepPlay(AM_INT index)
{
    AM_ASSERT(mpCmdQueue);
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_STEPPLAY;
    cmd.flag = 0;
    if(index < 0)
        cmd.flag |= PAUSE_RESUME_STEP_HD;

    cmd.res32_1 = index;
    cmd.repeatType = CMD_TYPE_SINGLETON;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::UpdatePBSpeed(AM_INT target, AM_U8 pb_speed, AM_U8 pb_speed_frac, IParameters::DecoderFeedingRule feeding_rule, AM_UINT flush)
{
    AM_ASSERT(mpCmdQueue);
    AM_ERR err;
    CMD cmd;
    cmd.code = MDCMD_SPEED_FEEDING_RULE;

    cmd.flag2 = target;
    cmd.res32_1 = (pb_speed << 8) | (pb_speed_frac);
    cmd.res64_1 = feeding_rule;
    cmd.res64_2 = flush;

    cmd.repeatType = CMD_TYPE_SINGLETON;
    err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::ReLayout(AM_INT layout, AM_INT winIndex)
{
    AM_ASSERT(mpCmdQueue);

    CMD cmd;
    cmd.code = MDCMD_RELAYOUT;
    cmd.res32_1 = layout;
    cmd.flag2 = (AM_U32)winIndex;
    cmd.repeatType = CMD_TYPE_SINGLETON;

    AM_ERR err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::ExchangeWindow(AM_INT winIndex0, AM_INT winIndex1)
{
    AM_ASSERT(mpCmdQueue);

    CMD cmd;
    cmd.code = MDCMD_EXCHANGE_WINDOW;
    cmd.res64_1 = (winIndex0<<16) | winIndex1;
    cmd.repeatType = CMD_TYPE_SINGLETON;

    AM_ERR err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}

AM_ERR CActiveMDecEngine::ConfigRender(AM_INT render, AM_INT window, AM_INT dsp)
{
    mpLayout->ConfigRender(render,window,dsp);
    return ME_OK;
}
AM_ERR CActiveMDecEngine::ConfigRenderDone()
{
    mpLayout->UpdateLayout();
    return ME_OK;
}
AM_ERR CActiveMDecEngine::ConfigTargetWindow(AM_INT target, CParam& param)
{
    AM_ASSERT(mpCmdQueue);

    CMD cmd;
    cmd.code = MDCMD_CONFIG_WINDOW;
    cmd.flag = target;
    cmd.pExtra = (void*)&param;
    cmd.repeatType = CMD_TYPE_SINGLETON;

    AM_ERR err = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
    return err;
}


AM_ERR CActiveMDecEngine::CreateMotionDetectReceiver()
{
    if(mpMDReceiver != NULL) return ME_TOO_MANY;
    mpMDReceiver = (void*)CMotionDetectReceiver::Create((void*)this);
    if(mpMDReceiver==NULL){
        return ME_ERROR;
    }
    ((CMotionDetectReceiver*)mpMDReceiver)->SetMDMsgHandler(GetInterface(IID_IMDecControl), (void*)this);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::RemoveMotionDetectReceiver()
{
    if(mpMDReceiver == NULL) return ME_OK;
    ((CMotionDetectReceiver*)mpMDReceiver)->Stop();
    ((CMotionDetectReceiver*)mpMDReceiver)->Delete();
    mpMDReceiver = NULL;
    return ME_OK;
}

void CActiveMDecEngine::MotionDetecthandler(const char* ip, void* event)
{
    AM_UINT srcIP;
    AM_UINT tmp[4];
    AM_INFO("CActiveMDecEngine::MotionDetectHandler: %s, %u, %s\n",
        ip, ((MD_EVENT_ST*)event)->ulMsgID, ((MD_EVENT_ST*)event)->acEventTime);
    if(4!=sscanf(ip, "%u.%u.%u.%u", &(tmp[0]), &(tmp[1]), &(tmp[2]), &(tmp[3]))){
        AM_ERROR("CActiveMDecEngine::MotionDetecthandler: unknow ip: %s!\n", ip);
    }
    srcIP = (tmp[0] & 0xff) << 24 | (tmp[1] & 0xff) << 16 | (tmp[2] & 0xff) << 8 | (tmp[3] & 0xff);
    int streamID = InfoFindIndexBySource(srcIP);
    if(streamID==-1) {
        AM_ERROR("CActiveMDecEngine::MotionDetecthandler: ip: %s??\n", ip);
    }else{
        mpMuxer->processMDMsg(streamID, ((MD_EVENT_ST*)event)->ulMsgID);
        AM_MSG msg;
        msg.code = MSG_EVENT_NVR_MOTION_DETECT_NOTIFY;
        msg.p4 = streamID;
        msg.p5 = ((MD_EVENT_ST*)event)->ulMsgID;
        msg.sessionID = mSessionID;
        PostAppMsg(msg);
    }
    return;
}

AM_ERR CActiveMDecEngine::ConfigLoader(AM_BOOL en, char* path, char* m3u8name, char* host, AM_INT count, AM_UINT duration)
{
    AM_ERR err = ME_NO_INTERFACE;

    if (mbEnableTanscode && mpTranscoder) {
        if (en == AM_TRUE) mpTranscoder->SetSavingTimeDuration(duration);
        err = mpTranscoder->ConfigLoader(en, path, m3u8name, host, count);
    }
    return err;
}
//well test thing//
/*
AM_ERR CActiveMDecEngine::Test(AM_INT flag)
{
    AM_INFO("Test\n");
    if(flag < 0)
        return ME_BAD_PARAM;

    mpVideoDecoder->Pause(flag);
    mpRenderer->Flush(flag);
    AM_INFO("Flush %d Done!\n", flag);
    mpVideoDecoder->Flush(flag);
    mpVideoDecoder->Resume(flag);
    AM_INFO("-------------^^ Test. :) @_@ #_# #_^  ^_^  :-) \n");
    static int testnum = 0;
    CDemuxerConfig* curConfig;

    if(testnum == 0){

        curConfig = &(mpConfig->demuxerConfig[3]);
        curConfig->hided = AM_FALSE;
        mpConfig->generalCmd |= DEMUXER_SPECIFY_GETBUFFER;
        mpConfig->specifyIndex = -1; //no specify read
        mpDemuxer->Config(3);
        mpConfig->generalCmd &= ~DEMUXER_SPECIFY_GETBUFFER;
        testnum = 100;
        return ME_OK;
    }

    curConfig = &(mpConfig->demuxerConfig[3]);
    curConfig->hided = AM_TRUE;
    mpConfig->generalCmd |= DEMUXER_SPECIFY_GETBUFFER;
    mpConfig->specifyIndex = -1; //no specify read
    mpDemuxer->Config(3);
    mpConfig->generalCmd &= ~DEMUXER_SPECIFY_GETBUFFER;

    curConfig = &(mpConfig->demuxerConfig[4]);
    curConfig->hided = AM_FALSE;
    mpDemuxer->Config(4);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleAutoSwitchSeek(AM_INT winIndex)
{
    AM_INT i, sIndex, hdIndex;
    AM_INT flag = 0;
    AM_ERR err;
    CDemuxerConfig* curConfig = NULL;

    sIndex = InfoChangeWinIndex(winIndex);
    if(sIndex < 0)
        return ME_BAD_PARAM;
    //find the connect HD source
    hdIndex = InfoFindHDIndex(sIndex);
    if(hdIndex < 0){
        AM_ERROR("HD Source for source %d donot seted!\n", winIndex);
        return ME_CLOSED;
    }
    mNvrAudioIndex = mpConfig->GetAudioSource();
    AM_ASSERT(mNvrAudioIndex < 0);

    //hide
    AM_INFO("Seek SW, Hide Small.\n");
    InfoHideAllSmall(AM_TRUE);
    mpDemuxer->Config(CMD_EACH_COMPONENT);
    //pause decoder, flush small
    mpVideoDecoder->Pause(CMD_EACH_COMPONENT);
    mpDemuxer->Flush(sIndex);
    mpVideoDecoder->Resume(hdIndex);
    //get seek info from sIndex
    CParam64 par(3);
    err = mpVideoDecoder->QueryInfo(sIndex, INFO_TIME_PTS, par);
    AM_INFO("Info Dump for Stream %d:Feed Pts:%lld, Render Pts:%lld\n", sIndex, par[0], par[1]);
    AM_U64 targetMs = par[1];
    mpDemuxer->SeekTo(hdIndex, targetMs);
    curConfig = &(mpConfig->demuxerConfig[hdIndex]);
    curConfig->hided = AM_FALSE;
    mpDemuxer->Config(hdIndex);
    //render?
    CParam par2(5);
    par2[0] = hdIndex;
    par2[1] = -1;
    HandleConfigWindow(&par2, 0);
    HandlePerformWinConfig();

    mFullWinIndex = hdIndex;
    mSmallWin = sIndex;
    mbOnNVR = AM_FALSE;
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleRevertNVRSeek()
{
    AM_ERR err;
    CDemuxerConfig* curConfig;
    //hide
    curConfig = &(mpConfig->demuxerConfig[mFullWinIndex]);
    curConfig->hided = AM_TRUE;
    mpDemuxer->Config(mFullWinIndex);
    //resume, pause decoder
    mpVideoDecoder->Pause(mFullWinIndex);
    mpDemuxer->Flush(mFullWinIndex);
    //seek
    CParam64 par(3);
    err = mpVideoDecoder->QueryInfo(mFullWinIndex, INFO_TIME_PTS, par);
    AM_INFO("Info Dump for Stream %d:Feed Pts:%lld, Render Pts:%lld\n", mFullWinIndex, par[0], par[1]);
    AM_U64 targetMs = par[1];
    mpDemuxer->SeekTo(mSmallWin, targetMs);
    InfoHideAllSmall(AM_FALSE);
    mpDemuxer->Config(CMD_EACH_COMPONENT);

    mpVideoDecoder->Resume(CMD_EACH_COMPONENT);

    AM_INFO("Re-Config Windows?\n");
    //RENDER?
    CParam par2(5);
    AM_INT i;
    for(i = 0; i < STAGE1_STREAM_NUM; i++){
        par2[0] = i;
        par2[1] = -1;
        HandleConfigWindow(&par2, i);
    }
    HandlePerformWinConfig();

    mbOnNVR = AM_TRUE;
    return ME_OK;
}
*/

//well test thing//
AM_ERR CActiveMDecEngine::Test(AM_INT flag)
{
//    AM_INT i;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
#if 0
/*    //-------------DELETE_SOURCE_TEST-------
    HandleDeleteSource(flag, 0);
    return ME_OK;

    //--------SOURCE_EDIT_ADDBACKGROUND-----
    mpCurSource = "3.mp4";
    HandleEditSource(0, SOURCE_EDIT_ADDBACKGROUND | SOURCE_ENABLE_VIDEO);
    return ME_OK;
    //------------------------Loop Test------------


    if(flag == 0)
        mpConfig->globalFlag |= LOOP_FOR_LOCAL_FILE;
    if(flag == 1)
        mpConfig->globalFlag &= ~LOOP_FOR_LOCAL_FILE;


    return ME_OK;


    //----------------Replace Test---------------

    AM_ERR err;
    if(flag == 0)
        HandleDisconnectHw();
    if(flag == 1)
        HandleReconnectHw();

    return ME_OK;

    /*//----------------Replace Test---------------
    AM_ERR err;
    mpCurSource = "1.mp4";
    if(flag == 0){
        //err = HandleEditSource(2, SOURCE_EDIT_PLAY | SOURCE_ENABLE_VIDEO);
        err = HandleEditSource(0, SOURCE_EDIT_PLAY | SOURCE_ENABLE_VIDEO);
        err = HandleEditSource(1, SOURCE_EDIT_PLAY | SOURCE_ENABLE_VIDEO);
        err = HandleEditSource(2, SOURCE_EDIT_PLAY | SOURCE_ENABLE_VIDEO);
        err = HandleEditSource(3, SOURCE_EDIT_PLAY | SOURCE_ENABLE_VIDEO);
    }
    if(flag == 1){
        err = HandleBackSource(0, 0, 0);
        err = HandleBackSource(1, 0, 0);
        err = HandleBackSource(2, 0, 0);
        err = HandleBackSource(3, 0, 0);
    }
    if(flag == 2){
        err = HandleEditSource(2, SOURCE_EDIT_REPLACE | SOURCE_ENABLE_VIDEO);
    }
    AM_INFO("Test return :%d\n", err);
    return ME_OK;



    //*/



    /*//-------Vout State Test---------------
    CDspGConfig* pDsp = &mpConfig->dspConfig;
    AM_UINT udecState, voutState, error;
    AM_ERR err;
    err = pDsp->udecHandler->GetUDECState(flag, udecState, voutState, error);
    AM_ASSERT(err == ME_OK);
    AM_INFO("State Info(%d)::::udecState(%d), voutState(%d), error(%d)\n", flag, udecState, voutState, error);
    */




  /*  //-----------------------WIN_REN Test----------------
    if(mbOnNVR == AM_TRUE){
        AM_INFO("Config Windows to Show HD.\n");
        HandleConfigWinMap(4, mUdecNum - 1, 0);
        HandlePerformWinConfig();
        DoPauseSmallDspMw(SWITCH_ACTION);
        //flush every thing
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE)
                DoFlushSpecifyDspMw(i);
        }
        mbOnNVR = AM_FALSE;
        AM_INFO("Well.\n");
    }else{
        AM_INFO("Config Windows to Show NVR!\n");
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE)
                DoFlushSpecifyDspMw(i);
        }
        usleep(500);
        HandleConfigWinMap(0, 0, 0);
        HandleConfigWinMap(1, 1, 1);
        HandleConfigWinMap(2, 2, 2);
        HandleConfigWinMap(3, 3, 3);
        HandlePerformWinConfig();
        mpVideoDecoder->Pause(DEC_MAP(4));

        if(mpMdecInfo->unitInfo[mHdInfoIndex].isPaused == AM_TRUE){
            AM_INFO("Paused will be resumed by switch back for Hd %d.\n", mHdInfoIndex);
            DoFlushSpecifyDspMw(4);
        }else{
            DoFlushSpecifyDspMw(4);
            mpVideoDecoder->Resume(DEC_MAP(4));
        }

        mbOnNVR = AM_TRUE;
        AM_INFO("Well!\n");
    }
    */
#endif
///*-----------------------Flush Test
    AM_INFO("Test\n");
    if(flag < 0)
        return ME_BAD_PARAM;

    curInfo = &(mpMdecInfo->unitInfo[flag]);
    mpVideoDecoder->Pause(flag);
    if(curInfo->playStream & TYPE_AUDIO){
        AM_ASSERT(mpConfig->GetAudioSource() == flag);
        AM_INFO("Pause the Audio Stream(%d).\n", flag);
        mpAudioSinkFilter->Pause(flag);//ADEC_MAP(I) == I;
    }
    mpRenderer->Pause(flag);
    mpDemuxer->Pause(flag);
    AM_INFO("Pause Done.\n");
    //Dump();
    mpRenderer->Flush(flag);
    AM_INFO("Dump after Render Flush.\n");
    //Dump();
    mpVideoDecoder->Flush(flag);
    if(curInfo->playStream & TYPE_AUDIO){
        mpAudioSinkFilter->Flush(flag);
    }
    AM_INFO("Dump after AVDec Flush.\n");
    //Dump();
    mpDemuxer->Flush(flag);
    AM_INFO("Flush Done.\n");
    Dump();

    //sleep(4);
    //mpRenderer->Resume(flag);
    AM_INFO("Resume Begin...\n");
    mpRenderer->Resume(flag);
    mpVideoDecoder->Resume(flag);
    if(curInfo->playStream & TYPE_AUDIO){
        mpAudioSinkFilter->Resume(flag);
    }
    mpDemuxer->Resume(flag);
    AM_INFO("Well Done!\n");
//*/

/*
    AM_INFO("-------------^^ Test. :) @_@ #_# #_^  ^_^  :-) \n");
    static int testnum = 0;
    CDemuxerConfig* curConfig;

    if(testnum == 0){

        curConfig = &(mpConfig->demuxerConfig[3]);
        curConfig->hided = AM_FALSE;
        mpConfig->generalCmd |= DEMUXER_SPECIFY_GETBUFFER;
        mpConfig->specifyIndex = -1; //no specify read
        mpDemuxer->Config(3);
        mpConfig->generalCmd &= ~DEMUXER_SPECIFY_GETBUFFER;
        testnum = 100;
        return ME_OK;
    }

    curConfig = &(mpConfig->demuxerConfig[3]);
    curConfig->hided = AM_TRUE;
    mpConfig->generalCmd |= DEMUXER_SPECIFY_GETBUFFER;
    mpConfig->specifyIndex = -1; //no specify read
    mpDemuxer->Config(3);
    mpConfig->generalCmd &= ~DEMUXER_SPECIFY_GETBUFFER;

    curConfig = &(mpConfig->demuxerConfig[4]);
    curConfig->hided = AM_FALSE;
    mpDemuxer->Config(4);
*/
    return ME_OK;
}

AM_ERR CActiveMDecEngine::SetNvrPlayBackMode(AM_INT mode)
{
    mNvrPbMode = mode;
    return ME_OK;
}

AM_ERR CActiveMDecEngine::ChangeSavingTimeDuration(AM_INT duration)
{
    AM_ASSERT(mpMuxer);
    mpMuxer->SetSavingTimeDuration((AM_UINT)duration);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::GetStreamInfo(AM_INT index, AM_INT* width, AM_INT* height)
{
    AM_ERR err;
    AM_INT DemIndex;
    CParam param(2);
    if(mpMdecInfo->isNvr == AM_TRUE){
        DemIndex = InfoChangeWinIndex(index);
    }else{
        AM_INT sdIndex;
        sdIndex = InfoChangeWinIndex(index);
        DemIndex = InfoFindHDIndex(sdIndex);
    }

    err = mpDemuxer->GetStreamInfo(DemIndex, param);
    if(err != ME_OK){
        AM_ERROR("engine: GetStreamInfo error!\n");
        return err;
    }
    *width = param[0];
    *height = param[1];
    return err;
}

AM_ERR CActiveMDecEngine::ActiveStream(AM_INT streamIndex, bool activeAudio)
{
    // todo: check whether stream has been active
    AM_ERR err;

    err = ForegroundStream(streamIndex, activeAudio);
    if (err != ME_OK) {
        AM_ERROR("ActiveStream: ForegroundStream failure, err:%d streamIndex:%d\n", err, streamIndex);
        return err;
    }

    err = ResumeStream(streamIndex);
    if (err != ME_OK) {
        AM_ERROR("ActiveStream: ResumeStream failure, err:%d streamIndex:%d\n", err, streamIndex);
        return err;
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::InactiveStream(AM_INT streamIndex)
{
    // todo: check whether stream has been inactive
    AM_ERR err;

    err = PauseStream(streamIndex);
    if (err != ME_OK) {
        AM_ERROR("InactiveStream: PauseStream failure, err:%d streamIndex:%d\n", err, streamIndex);
        return err;
    }

    err = BackgroundStream(streamIndex);
    if (err != ME_OK) {
        AM_ERROR("InactiveStream: BackgroundStream failure, err:%d streamIndex:%d\n", err, streamIndex);
        return err;
    }

    err = FlushStream(streamIndex);
    if (err != ME_OK) {
        AM_ERROR("InactiveStream: FlushStream failure, err:%d streamIndex:%d\n", err, streamIndex);
        return err;
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::ForegroundSDStreams()
{
    AM_ERR err;
    err = InfoHideAllSmall(AM_FALSE);
    if (err != ME_OK) {
        AM_ERROR("ForegroundSDStreams: InfoHideAllSmall show failure, err:%d\n", err);
        return err;
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::BackgroundSDStreams()
{
    AM_ERR err;
    err = InfoHideAllSmall(AM_TRUE);
    if (err != ME_OK) {
        AM_ERROR("ForegroundSDStreams: InfoHideAllSmall hide failure, err:%d\n", err);
        return err;
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::PauseStream(AM_INT streamIndex)
{
    AM_ERR err = mpVideoDecoder->Pause(DEC_MAP(streamIndex));
    if (err != ME_OK) {
        AM_ERROR("PauseStream: Pause failure, err:%d streamIndex:%d\n", err, streamIndex);
        return err;
    }
    mpMdecInfo->unitInfo[streamIndex].isPaused = AM_TRUE;

    return ME_OK;
}

AM_ERR CActiveMDecEngine::ResumeStream(AM_INT streamIndex)
{
    AM_ERR err = mpVideoDecoder->Resume(DEC_MAP(streamIndex));
    if (err != ME_OK) {
        AM_ERROR("ResumeStream: Resume failure, err:%d streamIndex:%d\n", err, streamIndex);
        return err;
    }
    mpMdecInfo->unitInfo[streamIndex].isPaused = AM_FALSE;

    return ME_OK;
}

AM_ERR CActiveMDecEngine::FlushStream(AM_INT streamIndex)
{
    AM_ERR err = DoFlushSpecifyDspMw(streamIndex, AM_TRUE);
    if (err != ME_OK) {
        AM_ERROR("FlushStream: DoFlushSpecifyDspMw failure, err:%d streamIndex:%d\n", err, streamIndex);
        return err;
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::ForegroundStream(AM_INT streamIndex, bool activeAudio)
{
    AM_ERR err;
    mpConfig->demuxerConfig[streamIndex].hided = AM_FALSE;
    err = mpDemuxer->Config(streamIndex);
    if (err != ME_OK) {
        AM_ERROR("ForegroundStream: config show failure, err:%d streamIndex:%d\n", err, streamIndex);
        return err;
    }

    mpMdecInfo->unitInfo[streamIndex].playStream |= TYPE_VIDEO;
    if(activeAudio)  mpMdecInfo->unitInfo[streamIndex].playStream |= TYPE_AUDIO;

    return ME_OK;
}

AM_ERR CActiveMDecEngine::BackgroundStream(AM_INT streamIndex)
{
    AM_ERR err;
    mpConfig->demuxerConfig[streamIndex].hided = AM_TRUE;
    err = mpDemuxer->Config(streamIndex);
    if (err != ME_OK) {
        AM_ERROR("BackgroundStream: config hide failure, err:%d streamIndex:%d\n", err, streamIndex);
        return err;
    }

    mpMdecInfo->unitInfo[streamIndex].playStream &= ~(TYPE_AUDIO | TYPE_VIDEO);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::ActiveSDStreams()
{
    return ME_OK;
}

AM_ERR CActiveMDecEngine::InactiveSDStreams()
{
    return ME_OK;
}

AM_ERR CActiveMDecEngine::PauseSDStreams()
{
    AM_ERR err;
    err = DoPauseSmallDspMw(SWITCH_ACTION);
    if (err != ME_OK) {
        AM_ERROR("PauseSDStreams: DoPauseSmallDspMw failure, err:%d\n", err);
        return err;
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::ResumeSDStreams()
{
    AM_ERR err;
    err = DoResumeSmallDspMw(SWITCH_ACTION);
    if (err != ME_OK) {
        AM_ERROR("PauseSDStreams: DoResumeSmallDspMw failure, err:%d\n", err);
        return err;
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::FlushSDStreams()
{
    AM_ERR err;
    err = DoFlushSmallDspMw();
    if (err != ME_OK) {
        AM_ERROR("PauseSDStreams: DoResumeSmallDspMw failure, err:%d\n", err);
        return err;
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::SwitchAudio(AM_INT audioSrcIndex, AM_INT audioTargetIndex)
{
    if (audioSrcIndex == audioTargetIndex) {
        AM_INFO("SwitchAudio: it has been audioTargetIndex:%d\n", audioTargetIndex);
        return ME_OK;
    }

    AM_ERR err;
    err = DoRemoveAudioStream(audioSrcIndex);
    if (err != ME_OK) {
        AM_ERROR("SwitchAudio: DoRemoveAudioStream failure, err:%d audioSrcIndex:%d audioTargetIndex:%d\n", err, audioSrcIndex, audioTargetIndex);
        return err;
    }

    err = DoStartSpecifyAudio(audioTargetIndex);
    if (err != ME_OK) {
        AM_ERROR("SwitchAudio: DoStartSpecifyAudio failure, err:%d audioSrcIndex:%d audioTargetIndex:%d\n", err, audioSrcIndex, audioTargetIndex);
        return err;
    }

    return ME_OK;
}

AM_BOOL CActiveMDecEngine::IsShowOnLCD()
{
    AM_ASSERT(mpConfig);

    return (AM_BOOL)(mpConfig->globalFlag & NVR_ONLY_SHOW_ON_LCD);
}

AM_ERR CActiveMDecEngine::HandleExchangeWindow(AM_INT winIndex0, AM_INT winIndex1)
{
    return ME_NO_IMPL;
}
//---------------------------end test thing---------------------------------//


//----------------------------------------------------------
//
//
//
//----------------------------------------------------------
inline AM_INT CActiveMDecEngine::InfoChooseIndex(AM_BOOL isNet, AM_INT flag, AM_INT order)
{
    //
    AM_INFO("InfoChooseIndex: isNet:%d, flag:%d, order:%d\n", isNet, flag, order);
    if(mpConfig->globalFlag & NO_HD_WIN_NVR_PB){
        if(mStartIndex == -1){
            mStartIndex = mpConfig->sourceNum;
            AM_INFO("InfoChooseIndex: demuxer start index %d\n", mStartIndex);
        }
        //select a noused to use
        AM_INT i = 0;
        for(; i < MDEC_SOURCE_MAX_NUM; i++){
                if(mpMdecInfo->unitInfo[i].isUsed == AM_FALSE)
                break;
        }
        if(i == MDEC_SOURCE_MAX_NUM){
            AM_ERROR("Not useable index on NoHd Case\n");
            return -1;
        }
        return i;
    }

    AM_INT winNum = mpConfig->dspConfig.dspNumRequested - 1;
    AM_INT netSize = winNum + winNum;
    AM_INT isHD = (flag & SOURCE_FULL_HD) ? 1 : 0;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    AM_INT start, end;
    AM_INT i;
    AM_INT orderGap = order * netSize;

    if(isNet == AM_TRUE){
        start = isHD * winNum + orderGap;
        end = start + winNum;

        if(mStartIndex == -1){
            //get start index at the first time to choose index
            mStartIndex = start;
            AM_INFO("InfoChooseIndex: demuxer start index %d\n", mStartIndex);
        }
        for(i = start; i < end; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_FALSE)
                return i;
        }
    }else{
        start = isHD * winNum + netSize + orderGap;
        end = start + winNum;
        if(mStartIndex == -1){
            mStartIndex = start;
            AM_INFO("InfoChooseIndex: demuxer start index %d\n", mStartIndex);
        }
        AM_INFO("%d, %d, %d, %d\n", start, end, netSize, orderGap);
        for(i = start; i < end; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_FALSE)
                return i;
        }
    }
    return -1;
}

inline AM_INT CActiveMDecEngine::InfoChangeWinIndex(AM_INT winIndex)
{
    return mpMdecInfo->FindSdBySourceGroup(winIndex);
}

inline AM_INT CActiveMDecEngine::InfoSDByWindow(AM_INT winIndex)
{
    return InfoChangeWinIndex(winIndex);
}

inline AM_INT CActiveMDecEngine::InfoHDByWindow(AM_INT winIndex)
{
    return mpMdecInfo->FindHdBySourceGroup(winIndex);
}

inline AM_INT CActiveMDecEngine::InfoFindHDIndex(AM_INT sIndex)
{
    return mpMdecInfo->FindHdBySd(sIndex);
}

inline AM_INT CActiveMDecEngine::InfoHDBySD(AM_INT sdIndex)
{
    return InfoFindHDIndex(sdIndex);
}

inline AM_INT CActiveMDecEngine::InfoSDByHD(AM_INT hdIndex)
{
    AM_INT sourceGroup, order, i, maxOrder = -1, max=0;
    AM_INT ret;
    MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[hdIndex]);
    sourceGroup = curInfo->sourceGroup;
    order = curInfo->addOrder;
    AM_VERBOSE("InfoSDByHD, order=%d.\n", order);

    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->sourceGroup == sourceGroup && curInfo->is1080P == AM_FALSE){
                //find a sd
                if(curInfo->addOrder > maxOrder){
                    maxOrder = curInfo->addOrder;
                    max = i;
                    AM_ASSERT(i != hdIndex);
                }
            }
    }
    ret = (maxOrder == -1) ? -1 : max;
    return ret;
}

inline AM_INT CActiveMDecEngine::InfoFindIndexBySource(AM_UINT ip)
{
    return mpMdecInfo->FindIndexBySource(ip);
}

inline AM_ERR CActiveMDecEngine::InfoPauseAllSmall()
{
    AM_INT i;
    MdecInfo::MdecUnitInfo* curInfo = NULL;

    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
            curInfo->isPaused = AM_TRUE;
            mpConfig->demuxerConfig[i].paused = AM_TRUE;
        }
    }
    return ME_OK;
}

inline AM_ERR CActiveMDecEngine::InfoResumeAllSmall()
{
    AM_INT i;
    MdecInfo::MdecUnitInfo* curInfo = NULL;

    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
            curInfo->isPaused = AM_FALSE;
            mpConfig->demuxerConfig[i].paused = AM_FALSE;
        }
    }
    return ME_OK;
}

inline AM_ERR CActiveMDecEngine::InfoHideAllSmall(AM_BOOL hided)
{
    AM_INT i;
    MdecInfo::MdecUnitInfo* curInfo = NULL;

    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
            mpConfig->demuxerConfig[i].hided = hided;
        }
    }
    return ME_OK;
}
//----------------------------------------------------------
//
//
//
//----------------------------------------------------------
AM_ERR CActiveMDecEngine::DoAutoChooseAudioStream()
{
    AM_INFO("DoAutoChooseAudioStream, State:%d.\n", mState);
    //CHOOSE AUDIO STREAM
    //ONCASE: very begining, dynamic add and soon.
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    CDemuxerConfig* pDemConfig = NULL;
    AM_INT index;

    if(mpConfig->globalFlag & NO_AUDIO_ON_GMF || mpConfig->GetAudioSource() >= 0){
        return ME_OK;
    }
    if(mState != STATE_IDLE){
        //Dynamic call AddSource
        index = mpConfig->curIndex;
        curInfo = &(mpMdecInfo->unitInfo[index]);
        pDemConfig = &(mpConfig->demuxerConfig[index]);
        if(mpConfig->globalFlag & NO_HD_WIN_NVR_PB){
            //choose this anyway
            curInfo->playStream |= TYPE_AUDIO;
            pDemConfig->disableAudio = AM_FALSE;
            return ME_OK;
        }
        switch(mpLayout->GetLayoutType())
        {
        case LAYOUT_TYPE_TABLE:
            if(curInfo->is1080P == AM_FALSE){
                if(mEditPlayAudio>=0){
                }else
                {
                    curInfo->playStream |= TYPE_AUDIO;
                    pDemConfig->disableAudio = AM_FALSE;
                }
            }
            break;

        case LAYOUT_TYPE_TELECONFERENCE:
            if(curInfo->sourceGroup == 0 && curInfo->is1080P == AM_FALSE)//no choose this
                break;
            if(curInfo->sourceGroup == 0 || curInfo->is1080P == AM_FALSE){
                curInfo->playStream |= TYPE_AUDIO;
                pDemConfig->disableAudio = AM_FALSE;
            }
            break;

        default:
            AM_ASSERT(0);
            break;
        }
        return ME_OK;
    }

    //this point is only reach by handlePrepare() function
    if(mpConfig->globalFlag & NO_HD_WIN_NVR_PB){
        //choose the first one
        for(index = 0; index < MDEC_SOURCE_MAX_NUM; index++)
        {
            curInfo = &(mpMdecInfo->unitInfo[index]);
            pDemConfig = &(mpConfig->demuxerConfig[index]);
            if(curInfo->isUsed == AM_TRUE && pDemConfig->hasAudio == AM_TRUE){
                curInfo->playStream |= TYPE_AUDIO;
                pDemConfig->disableAudio = AM_FALSE;
                return ME_OK;
            }
        }
    }
    AM_INFO("d1\n");
    switch(mpLayout->GetLayoutType())
    {
    case LAYOUT_TYPE_TABLE:
        for(index = 0; index < MDEC_SOURCE_MAX_NUM; index++)
        {
            curInfo = &(mpMdecInfo->unitInfo[index]);
            pDemConfig = &(mpConfig->demuxerConfig[index]);
            //THE FIRST NO HD SOURCE.
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE && pDemConfig->hasAudio == AM_TRUE){
                curInfo->playStream |= TYPE_AUDIO;
                pDemConfig->disableAudio = AM_FALSE;
                AM_INFO("d2\n");
                break;
            }
        }
        break;

    case LAYOUT_TYPE_TELECONFERENCE:
        //FIND THE HD ON WINDOW 0
        for(index = 0; index < MDEC_SOURCE_MAX_NUM; index++)
        {
            curInfo = &(mpMdecInfo->unitInfo[index]);
            pDemConfig = &(mpConfig->demuxerConfig[index]);
            if(curInfo->isUsed == AM_TRUE && curInfo->sourceGroup == 0 && curInfo->is1080P == AM_TRUE){
                curInfo->playStream |= TYPE_AUDIO;
                pDemConfig->disableAudio = AM_FALSE;
                break;
            }
        }
        //NO HD SETED
        if(mpConfig->GetAudioSource() < 0)
        {
            for(index = 0; index < MDEC_SOURCE_MAX_NUM; index++){
                curInfo = &(mpMdecInfo->unitInfo[index]);
                pDemConfig = &(mpConfig->demuxerConfig[index]);
                if(curInfo->isUsed == AM_TRUE && curInfo->sourceGroup != 0 && curInfo->is1080P != AM_TRUE){
                    curInfo->playStream |= TYPE_AUDIO;
                    pDemConfig->disableAudio = AM_FALSE;
                    break;
                }
            }
        }
        break;

    default:
        AM_ASSERT(0);
        break;
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoInterPausePlay()
{
    DoPauseAudioStream();
    mpRenderer->Pause(CMD_GENERAL_ONLY);
    mpVideoDecoder->Pause(CMD_GENERAL_ONLY);
    mpDemuxer->Pause(CMD_GENERAL_ONLY);
    return ME_OK;
}

//can be discard
AM_ERR CActiveMDecEngine::DoResumeAudioStream(AM_INT aIndex)
{
    //mpConfig->generalCmd |= RENDERER_ENABLE_AUDIO;
    //mpRenderer->Config(CMD_EACH_COMPONENT);
    //mpConfig->generalCmd &= ~RENDERER_ENABLE_AUDIO;
    mpAudioSinkFilter->Resume(CMD_GENERAL_ONLY);
    return ME_OK;
}

//can be moved
AM_ERR CActiveMDecEngine::DoPauseAudioStream(AM_INT aIndex)
{
    //Pause audio only on render side is non-reasonable.
    mpAudioSinkFilter->Pause(CMD_GENERAL_ONLY);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoRemoveAudioStream(AM_INT aIndex)
{
    AM_INFO("===DoRemoveAudioStream, %d\n", aIndex);
    if(mpConfig->globalFlag & NO_AUDIO_ON_GMF) {
        AM_INFO("DoRemoveAudioStream: audio is disabled!\n");
        return ME_OK;
    }
//    AM_INT i;
    if(aIndex == -1){
        aIndex = mpConfig->GetAudioSource();
        if(aIndex == -1){
            AM_INFO("The AudioStream has removed already!\n");
            return ME_OK;
        }
    }else{
        if(mpConfig->demuxerConfig[aIndex].disableAudio != AM_FALSE){
            AM_INFO("The specify source index %d on RemoveAudioStream is wrong!\n", aIndex);
            return ME_BAD_PARAM;
        }
    }

    //DoPauseAudioStream();
    mpConfig->generalCmd |= RENDEER_SPECIFY_CONFIG;
    mpConfig->specifyIndex = aIndex;
    mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = REMOVE_RENDER_AOUT;
    mpRenderer->Config(aIndex);
    mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = -1;
    mpConfig->generalCmd &= ~RENDEER_SPECIFY_CONFIG;

    mpAudioSinkFilter->Pause(CMD_GENERAL_ONLY);
    mpAudioSinkFilter->Remove(aIndex);
    if(mpAudioSinkFilter==mpAudioDecoder) mpAudioSinkFilter->Resume(CMD_GENERAL_ONLY);

    mpConfig->demuxerConfig[aIndex].disableAudio = AM_TRUE;
    mpDemuxer->AudioCtrl(aIndex);

    mpConfig->DisableAllAudio();
    mpMdecInfo->unitInfo[aIndex].playStream &= ~TYPE_AUDIO;
    return ME_OK;
}

//after remove audio stream, can be discard
inline AM_ERR CActiveMDecEngine::DoStartSpecifyAudio(AM_INT index, AM_INT flag)
{
    AM_INFO("===DoStartSpecifyAudio, %d\n", index);
    if(mpConfig->globalFlag & NO_AUDIO_ON_GMF) {
        AM_INFO("DoStartSpecifyAudio: audio is disabled!\n");
        return ME_OK;
    }
    CDemuxerConfig* curDemConfig = &(mpConfig->demuxerConfig[index]);
    if(curDemConfig->hasAudio == AM_FALSE){
        AM_INFO("Want to start a source which don't have audio!Do nothing.\n");
        mAudioWait = AUDIO_WAIT_NOON;
        return ME_OK;
    }
    curDemConfig->sendHandleA = AM_TRUE;
    curDemConfig->disableAudio = AM_FALSE;
    mpDemuxer->AudioCtrl(index, flag);
    mpMdecInfo->unitInfo[index].playStream |= TYPE_AUDIO;

    if(mpAudioSinkFilter==mpTansAudioSink) mpAudioSinkFilter->Resume(CMD_GENERAL_ONLY);

    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoFlushAllStream()
{
    AM_INFO("===DoFlushAllStream\n");
    DoRemoveAudioStream();
    IPin* pOutPin = mpDemuxer->GetOutputPin(0);
    if(pOutPin != NULL)
        pOutPin->Enable(false);
    pOutPin = mpVideoDecoder->GetOutputPin(0);
    if(pOutPin != NULL)
        pOutPin->Enable(false);
    //flush dsp
    mpRenderer->Flush(CMD_GENERAL_ONLY);
    mpVideoDecoder->Flush(CMD_GENERAL_ONLY);

    //purge pin
    IPin* pInPin = mpRenderer->GetInputPin(0);
    if(pInPin != NULL)
        pInPin->Purge();
    pInPin = mpVideoDecoder->GetInputPin(0);
    if(pInPin != NULL)
        pInPin->Purge();
    //for release diff cnt
    mpDemuxer->Flush(CMD_GENERAL_ONLY);

    pOutPin = mpDemuxer->GetOutputPin(0);
    if(pOutPin != NULL)
        pOutPin->Enable(true);
    pOutPin = mpVideoDecoder->GetOutputPin(0);
    if(pOutPin != NULL)
        pOutPin->Enable(true);

    //mpRenderer->Flush(CMD_EACH_COMPONENT);
    mpVideoDecoder->Flush(CMD_EACH_COMPONENT);
    mpDemuxer->Flush(CMD_EACH_COMPONENT);
    return ME_OK;
}
//---------------------------------------------------------
//audio switch handler funcs
//---------------------------------------------------------
AM_ERR CActiveMDecEngine::DoSwitchAudioLocal(AM_INT sIndex, AM_INT hdIndex)
{
    AM_INFO("DoSwitchAudioLocal\n");
    if(sIndex == mNvrAudioIndex){
        DoRemoveAudioStream(sIndex);
    }else{
        //need nothing?
    }
    //enable hd audio
    DoStartSpecifyAudio(hdIndex);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoSwitchBackAudioLocal()
{
    AM_INFO("DoSwitchBackAudioLocal\n");
    DoRemoveAudioStream(mHdInfoIndex);

    if(mSdInfoIndex != mNvrAudioIndex){
    }else{
        DoStartSpecifyAudio(mNvrAudioIndex);
        //render donot has a resume because the flush mNvrAudioIndex, this resume will be protected by v-render-out state check
        mpRenderer->Resume(mNvrAudioIndex);
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoSwitchAudioNet(AM_INT sIndex, AM_INT hdIndex)
{
    AM_INFO("DoSwitchAudioNet\n");
    //if(sIndex == mNvrAudioIndex){
    DoRemoveAudioStream(mNvrAudioIndex);
    //enable hd audio
    DoStartSpecifyAudio(hdIndex);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoSwitchBackAudioNet()
{
    AM_INFO("DoSwitchBackAudioNet\n");
    DoRemoveAudioStream(mHdInfoIndex);

    DoStartSpecifyAudio(mNvrAudioIndex);
    //render donot has a resume because the flush mNvrAudioIndex, this resume will be protected by v-render-out state check
    mpRenderer->Resume(mNvrAudioIndex);
    return ME_OK;
}
//---------------------------------------------------------
//only for video stream, these 3 func
//---------------------------------------------------------
AM_ERR CActiveMDecEngine::DoFlushSpecifyDspMw(AM_INT index, AM_U8 flag)
{
    MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[index]);
    if(mpConfig->globalFlag & NOTHING_DONOTHING && mpConfig->globalFlag & NOTHING_NOFLUSH){
        if(curInfo->isPaused == AM_TRUE){
            curInfo->isPaused = AM_FALSE;//not need resume dsp after flush
            mpVideoDecoder->Resume(DEC_MAP(index));
            mpRenderer->Resume(REN_MAP(index));
        }
        return ME_OK;
    }
    //no a winIndex
    mpRenderer->Flush(REN_MAP(index), flag);
    mpVideoDecoder->Flush(DEC_MAP(index));
    mpDemuxer->Flush(index);
    if(curInfo->isPaused == AM_TRUE){
        curInfo->isPaused = AM_FALSE;//not need resume dsp after flush
        mpVideoDecoder->Resume(DEC_MAP(index));
        mpRenderer->Resume(REN_MAP(index));
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoPauseSmallDspMw(AM_INT action)
{
    AM_INT i;
//    AM_INT indexRen, indexDec;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE && curInfo->isPaused == AM_FALSE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
            curInfo->isPaused = AM_TRUE;
            curInfo->pausedBy = action;
            mpVideoDecoder->Pause(DEC_MAP(i));
            mpRenderer->Pause(REN_MAP(i));
        }
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoResumeSmallDspMw(AM_INT action)
{
    AM_INT i;
//    AM_INT indexRen, indexDec;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->pausedBy != action)
            continue;//info paused by other one, resume should do by him.

        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE && curInfo->isPaused == AM_TRUE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
            curInfo->isPaused = AM_FALSE;
            curInfo->pausedBy = -1;
            mpVideoDecoder->Resume(DEC_MAP(i));
            mpRenderer->Resume(REN_MAP(i));
        }
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoFlushSmallDspMw()
{
    AM_INT i;
    MdecInfo::MdecUnitInfo* curInfo = NULL;

    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]) {

            mpRenderer->Flush(REN_MAP(i));
            mpVideoDecoder->Flush(DEC_MAP(i));
            mpDemuxer->Flush(i);

            // todo: this maybe cause issues
            if (curInfo->isPaused == AM_TRUE) {
                curInfo->isPaused = AM_FALSE; //not need resume dsp after flush
                mpVideoDecoder->Resume(DEC_MAP(i));
            }
        }
    }

    return ME_OK;
}

AM_BOOL CActiveMDecEngine::DoDetectSeamless(AM_INT sdIndex, AM_INT hdIndex)
{
    AM_BOOL seamless = AM_FALSE;
    if(mpConfig->globalFlag & SWITCH_ARM_SEEK_FEED_DSP_DEBUG){
        //seamless
        if(mpConfig->demuxerConfig[sdIndex].netused == AM_TRUE)
            seamless = AM_FALSE;
        else
            seamless = AM_TRUE;
    }
    if(mpConfig->globalFlag & SWITCH_ARM_SEEK_FEED_DSP_DEBUG2){
        //no seamless
        if(mpConfig->demuxerConfig[sdIndex].netused == AM_TRUE)
            seamless = AM_FALSE;
        else
            seamless = AM_FALSE;
    }
    AM_INFO("DoDetectSeamless, result:%d\n", seamless);
    return seamless;
}

AM_ERR CActiveMDecEngine::DoSyncSDStreamBySource(AM_INT source)
{
    AM_INT i;
    AM_ERR err;
    CParam64 parVec(3);
    AM_S64 targetTime;
    CDemuxerConfig* curConfig = NULL;
    MdecInfo::MdecUnitInfo* curInfo = NULL;

    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE && i != mSdInfoIndex && curInfo->isNet == AM_FALSE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
            err = mpVideoDecoder->QueryInfo(DEC_MAP(mHdInfoIndex), INFO_TIME_PTS, parVec);
            if(err != ME_OK){
                AM_ASSERT(0);
                parVec[0] = parVec[1] = 0;
            }
            targetTime = parVec[1];
            if(parVec[1] == -1)
                targetTime = parVec[0] - 2102100;
            if(targetTime < targetTime)//sd still has not frame out
                targetTime = targetTime;

            mpDemuxer->SeekTo(i, targetTime, SEEK_BY_SWITCH);
            curConfig = &(mpConfig->demuxerConfig[i]);
            curConfig->hided = AM_FALSE;
            mpDemuxer->Config(i);
        }
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoDspSwitchForSource(AM_INT oriSource, AM_INT destSource, AM_BOOL seamless)
{
    AM_INFO("DoDspSwitchForSource, ori:%d, dest:%d\n", oriSource, destSource);
//    CDemuxerConfig* oriConfig = &(mpConfig->demuxerConfig[oriSource]);
    MdecInfo::MdecUnitInfo* oriInfo = &(mpMdecInfo->unitInfo[oriSource]);
    CDemuxerConfig* curConfig = NULL;
//    MdecInfo::MdecUnitInfo* curInfo = NULL;
    AM_ERR err;
    CParam64 parVec(3);
    AM_S64 targetTime;

    if(oriInfo->isNet == AM_FALSE)
    {
        err = mpVideoDecoder->QueryInfo(DEC_MAP(oriSource), INFO_TIME_PTS, parVec);
        if(err != ME_OK){
            AM_ASSERT(0);
            parVec[0] = parVec[1] = 0;
        }
        targetTime = parVec[1];
        if(parVec[1] == -1)
            targetTime = parVec[0] - 2102100;

        AM_INFO("Info Dump for Stream %d:Feed Pts:%lld, Render Pts:%lld, Target:%lld\n", oriSource, parVec[0], parVec[1], targetTime);
        mpDemuxer->SeekTo(destSource, targetTime, SEEK_BY_SWITCH);
        curConfig = &(mpConfig->demuxerConfig[destSource]);
        curConfig->hided = AM_FALSE;
        mpDemuxer->Config(destSource);
    }else{
        curConfig = &(mpConfig->demuxerConfig[destSource]);
        curConfig->hided = AM_FALSE;
        mpDemuxer->Config(destSource);
    }
    //wait the decoder constructer.
    while(DSP_MAP(destSource) < 0)
        usleep(500);

    AM_INFO("Switch to Dsp %d<->%d, Wait!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", oriSource, destSource);
    mpLayout->SwitchStream(oriSource, destSource, seamless);
    AM_INFO("Well. Now Handle Layout and Audio Stream.\n");
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoFindStreamIndex(AM_INT sourceGroup, AM_INT& sIndex, AM_INT& hdIndex)
{
    AM_INFO("DoFindStreamIndex, sourceGroup:%d\n", sourceGroup);
    sIndex = InfoChangeWinIndex(sourceGroup);
    if(sIndex < 0)
        return ME_BAD_PARAM;
    //find the connect HD source
    hdIndex = InfoFindHDIndex(sIndex);
    if(hdIndex < 0){
        AM_ERROR("HD Source for source %d donot seted!\n", sourceGroup);
        return ME_CLOSED;
    }
    AM_INFO("DoFindStreamIndex, Find sIndex:%d, hdIndex:%d\n", sIndex, hdIndex);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoCreatePipeLine(AM_INT streamIndex)
{
    AM_INFO("Crteate pipleline for %d\n", streamIndex);
    AM_INT pipe = mpPipeLine->CreatePipeLine((char*)mpConfig->indexTable[streamIndex].file, IPipeLineManager::PIPELINE_PLAYBACK);
    mpPipeLine->AddPipeNode(pipe, streamIndex, IPipeLineManager::DEMUXER_FFMPEG);
    mpPipeLine->AddPipeNode(pipe, DEC_MAP(streamIndex), IPipeLineManager::DECODER_DSP);
    mpPipeLine->AddPipeNode(pipe, REN_MAP(streamIndex), IPipeLineManager::RENDER_SYNC);
    return ME_OK;
}
//---------------------------------------------------------
//
//
//---------------------------------------------------------
AM_ERR CActiveMDecEngine::InitSourceConfig(AM_INT source, AM_INT flag, AM_BOOL isNet)
{
    AM_INT index = mpConfig->curIndex;
//    AM_INT i = 0;
    MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[index]);
    CDemuxerConfig* pDemConfig = &(mpConfig->demuxerConfig[index]);
    if(curInfo->isUsed == AM_TRUE){
        AM_ERROR("Already Used, Source:%d.!\n", source);
        return ME_BUSY;
    }

    curInfo->isUsed = AM_TRUE;
    curInfo->sourceGroup = source;
    curInfo->playStream |= TYPE_VIDEO;
    curInfo->isPaused = AM_FALSE;

    pDemConfig->disableVideo = false;
    pDemConfig->hided = AM_FALSE;
    pDemConfig->hd = AM_FALSE;
    pDemConfig->disableAudio = AM_TRUE; //choose one handlestart()

    //flag handle
    if((flag & SOURCE_ENABLE_AUDIO) == 0 && (flag & SOURCE_ENABLE_VIDEO) == 0)
        AM_ASSERT(flag & SOURCE_FULL_HD);
    if(flag & SOURCE_ENABLE_AUDIO && !(flag & SOURCE_ENABLE_VIDEO)){
        AM_INFO("Only Audio not supported!\n");
        AM_ASSERT(0);
    }
    if(flag & SOURCE_FULL_HD)
    {
        curInfo->is1080P = AM_TRUE;
        pDemConfig->hd = AM_TRUE;
        //if(mHdInfoIndex < 0)
        //    mHdInfoIndex = index;
    }else{
    }

    //layout and add order handle
    if(mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE){
        //hide
        if(curInfo->is1080P == AM_TRUE){
            pDemConfig->hided = AM_TRUE;
        }
    }else if(mpLayout->GetLayoutType() == LAYOUT_TYPE_TELECONFERENCE){
        //hide
        if(curInfo->is1080P == AM_TRUE){
            if(curInfo->sourceGroup == 0){
            }else{
                pDemConfig->hided = AM_TRUE;
            }
        }else{
            if(curInfo->sourceGroup == 0)
                pDemConfig->hided = AM_TRUE;
        }
    }else{
        AM_ASSERT(0);
    }
    pDemConfig->configIndex = index;
    mpConfig->InitMapTable(index, mpCurSource, source);
    mpLayout->InitWinRenMap(index, source, flag);

    //some for dynamicl add
    if(mState != STATE_IDLE){
        DoAutoChooseAudioStream();
        pDemConfig->needStart = AM_TRUE;
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::CheckBeforeAddSource(AM_INT source, AM_INT flag)
{
    AM_INT dspNum = mpConfig->dspConfig.dspNumRequested;
    AM_INT i;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    if(mpConfig->globalFlag & NO_HD_WIN_NVR_PB){
        if(mpConfig->sourceNum >= dspNum){
            AM_INFO("Too many source\n");
            return ME_TOO_MANY;
        }
        if(flag & SOURCE_FULL_HD){
            AM_INFO("No HD on all small playback\n");
            return ME_BAD_PARAM;
        }
    }else{
        if(flag & SOURCE_FULL_HD){
            if(mpMdecInfo->HdWindowNum() >= (dspNum -1)){
                AM_INFO("Too Many HD source!\n");
                return ME_TOO_MANY;
            }
            if(source >= (dspNum -1)){
                AM_INFO("HD souce group Failed should be %d~%d!\n", 0, dspNum - 2);
                return ME_BAD_PARAM;
            }
            //check hd source group
            for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
                curInfo = &(mpMdecInfo->unitInfo[i]);
                if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_TRUE){
                    if(curInfo->sourceGroup == source){
                        AM_INFO("HD source group:%d has alread assgined!\n", source);
                        return ME_BAD_PARAM;
                    }
                }
            }
        }else{
            if((mpConfig->sourceNum - mpMdecInfo->HdStreamNum()) >= dspNum -1){
                AM_INFO("Too Many Sd Source, %d, %d!\n", mpConfig->sourceNum, mpMdecInfo->HdStreamNum());
                return ME_TOO_MANY;
            }
            //check sd's sourceGroup(winIndex)
            if(source >= (dspNum -1)){
                AM_INFO("SD souce group Failed, should be %d~%d!\n", 0, dspNum - 2);
                return ME_BAD_PARAM;
            }
            for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
                if(mpConfig->indexTable[i].isHd == AM_FALSE)
                    if(mpConfig->indexTable[i].sourceGroup == source){
                        AM_INFO("SD source group has been used!\n");
                        return ME_BAD_PARAM;
                    }
            }
        }
    }
    return ME_OK;
}
//---------------------------------------------------------
//
//
//
//---------------------------------------------------------
AM_ERR CActiveMDecEngine::HandleEditSourceDone()
{
    AM_INFO("HandleEditSourceDone.\n");
    AM_INT i = 0;
    CDemuxerConfig* curConfig = NULL;
    MdecInfo::MdecUnitInfo* curInfo = NULL;

    //sleep for last editsource
    if(mEditPlayAudio >= 0){
        DoStartSpecifyAudio(mEditPlayAudio, 0);
        mEditPlayAudio = -1;
    }
    usleep(2000);
    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        curInfo = &(mpMdecInfo->unitInfo[i]);
        curConfig = &(mpConfig->demuxerConfig[i]);

        if(curInfo->isUsed == AM_TRUE){
            if(curConfig->edited == AM_TRUE){
                mpVideoDecoder->Resume(DEC_MAP(i));
                curConfig->edited = AM_FALSE;
            }
        }
    }
    return ME_OK;
}


AM_ERR CActiveMDecEngine::HandleEditSource(AM_INT source, AM_INT flag)
{
    AM_INFO("HandleEditSource, source:%d\n", source);
    //we require this for sample. exp for audio handle.
    if(mpMdecInfo->isNvr == AM_FALSE){
        AM_INFO("Please edit source during NVR mode. Use '-b' back to NVR.\n");
        return ME_CLOSED;
    }

    AM_ERR err = ME_OK;
    AM_INT aIndex, index, order;
    AM_INT hdIndex;
    AM_BOOL isNet = AM_FALSE, hasAudio = AM_FALSE;
    CDemuxerConfig* curConfig = NULL;

    aIndex = mpConfig->GetAudioSource();
    index = InfoChangeWinIndex(source);
    if(flag & SOURCE_FULL_HD)
        index = InfoFindHDIndex(index);
    if(index < 0){
        AM_INFO("Edit a inexistent source! Use AddSource() api.\n");
        HandleAddSource(source, flag);
        return ME_OK;
    }
    if(strstr(mpCurSource, "rtsp://") != NULL){
        AM_INFO("NetPlayback!\n");
        isNet = AM_TRUE;
    }

    AM_INFO("Debug:aIndex %d, index %d, isNet %d\n", aIndex, index, isNet);
    curConfig = &(mpConfig->demuxerConfig[index]);

    if(flag & SOURCE_FULL_HD){
        AM_ASSERT(curConfig->hided == AM_TRUE);
        hdIndex = mpConfig->curHdIndex;
        order = mpMdecInfo->unitInfo[index].addOrder;
        AM_INFO("Cur HD:%d.\n", hdIndex);

        if(index != hdIndex){
            if(flag & SOURCE_EDIT_REPLACE){
                if(REN_MAP(index) != -1)
                    mpRenderer->Remove(REN_MAP(index));
                mpDemuxer->Remove(index);
                mpConfig->curIndex = index;
                mpMdecInfo->ClearUnitInfo(index);
                mpConfig->DeInitMap(index);
            }else if(flag & SOURCE_EDIT_PLAY){
                mpConfig->curIndex = InfoChooseIndex(isNet, flag, order + 1);
            }
        }else{
            if(flag & SOURCE_EDIT_REPLACE){
                mpVideoDecoder->Pause(DEC_MAP(index));
                mpVideoDecoder->Remove(DEC_MAP(index), DEC_REMOVE_JUST_NOTIFY);
                mpVideoDecoder->Resume(DEC_MAP(index));
                AM_ASSERT(REN_MAP(index) != -1);
                mpRenderer->Remove(REN_MAP(index));
                mpDemuxer->Remove(index);
                mpConfig->curIndex = index;
                mpMdecInfo->ClearUnitInfo(index);
                mpConfig->DeInitMap(index);
                //init HD index
                mpConfig->curHdIndex = -1;
            }else if(flag & SOURCE_EDIT_PLAY){
                mpConfig->curIndex = InfoChooseIndex(isNet, flag, order + 1);
            }
        }

        mbNetStream = mpConfig->demuxerConfig[mpConfig->curIndex].netused = isNet;
        mpMdecInfo->unitInfo[mpConfig->curIndex].isNet = isNet;
        err = InitSourceConfig(source, flag, isNet);
        err = mpDemuxer->AcceptMedia(mpCurSource);
        if(err != ME_OK){
            mpMdecInfo->unitInfo[mpConfig->curIndex].isUsed = AM_FALSE;
            mpConfig->DeInitMap(mpConfig->curIndex);
            AM_ERROR("Replace Source :%s Failed!\n", mpCurSource);
            return err;
        }

        if(flag & SOURCE_EDIT_ADDBACKGROUND){
            //this case apply edit_play for simplie, i tied to change it. but the addorder must be handled.
            mpMdecInfo->unitInfo[mpConfig->curIndex].addOrder = order;
            mpMdecInfo->unitInfo[index].addOrder = order + 1;
        }else if(flag & SOURCE_EDIT_REPLACE){
            mpMdecInfo->unitInfo[mpConfig->curIndex].addOrder = order;
        }else if(flag & SOURCE_EDIT_PLAY){
            mpMdecInfo->unitInfo[mpConfig->curIndex].addOrder = order + 1;
        }
        mpDemuxer->Config(mpConfig->curIndex);//let the new goto hided
        //curConfig->sendHandleV = AM_TRUE; hd will apdat by curHDindex.
        //we detach curhdindex
        //mpConfig->demuxerConfig[hdIndex].sendHandleV = AM_TRUE;
        return ME_OK;
    }

    //edit sd source
    if(flag & SOURCE_EDIT_REPLACE){
        //remove
        if(aIndex == index){
            DoRemoveAudioStream(aIndex);
            hasAudio = AM_TRUE;
        }
        order = mpMdecInfo->unitInfo[index].addOrder;
        AM_ASSERT(order == mpMdecInfo->winOrder[source]);

        mpVideoDecoder->Pause(DEC_MAP(index));
        mpRenderer->Pause(REN_MAP(index));
        curConfig->hided = AM_TRUE;
        mpDemuxer->Config(index);
        DoFlushSpecifyDspMw(index);
        mpVideoDecoder->Remove(DEC_MAP(index), DEC_REMOVE_JUST_NOTIFY);
        mpVideoDecoder->Resume(DEC_MAP(index));
        mpRenderer->Remove(REN_MAP(index));
        mpDemuxer->Remove(index);
        mpMdecInfo->ClearUnitInfo(index);
        mpConfig->DeInitMap(index);

        //assign
        mpConfig->curIndex = index;
        mbNetStream = mpConfig->demuxerConfig[mpConfig->curIndex].netused = isNet;
        mpMdecInfo->unitInfo[mpConfig->curIndex].isNet = isNet;
        err = InitSourceConfig(source, flag, isNet);
        err = mpDemuxer->AcceptMedia(mpCurSource);
        if(err != ME_OK){
            mpMdecInfo->unitInfo[mpConfig->curIndex].isUsed = AM_FALSE;
            mpConfig->DeInitMap(mpConfig->curIndex);
            AM_ERROR("Replace Source :%s Failed!\n", mpCurSource);
            return err;
        }
        //wait some to wakeup
        mpMdecInfo->unitInfo[mpConfig->curIndex].addOrder = order;
        mpMdecInfo->winOrder[source] = order;
        if(hasAudio)
            DoStartSpecifyAudio(mpConfig->curIndex, 0);

        //mpMdecInfo->ClearUnitInfo(index);---?
        return err;
    }

    if(flag & SOURCE_EDIT_PLAY){
        //assign
        order = mpMdecInfo->unitInfo[index].addOrder;
        mpConfig->curIndex = InfoChooseIndex(isNet, flag, order + 1);
        mbNetStream = mpConfig->demuxerConfig[mpConfig->curIndex].netused = isNet;
        mpMdecInfo->unitInfo[mpConfig->curIndex].isNet = isNet;
        err = InitSourceConfig(source, flag, isNet);
        err = mpDemuxer->AcceptMedia(mpCurSource, AM_FALSE);
        if(err != ME_OK){
            mpMdecInfo->unitInfo[mpConfig->curIndex].isUsed = AM_FALSE;
            mpConfig->DeInitMap(mpConfig->curIndex);
            AM_ERROR("Add Play Source :%s Failed!\n", mpCurSource);
            return err;
        }

        if(aIndex == index){
            DoRemoveAudioStream(aIndex);
            hasAudio = AM_TRUE;
        }
        mpVideoDecoder->Pause(DEC_MAP(index));
        //mpRenderer->Pause(REN_MAP(index));
        curConfig->hided = AM_TRUE;
        mpDemuxer->Config(index);
        DoFlushSpecifyDspMw(index, 1);
        mpVideoDecoder->Resume(DEC_MAP(index));
        /////mpDemuxer->Remove(index);

        mpDemuxer->Config(mpConfig->curIndex, CONFIG_DEMUXER_GO_RUN);
        mpMdecInfo->unitInfo[mpConfig->curIndex].addOrder = order + 1;
        mpMdecInfo->winOrder[source] = order + 1;

        if(hasAudio){
            mEditPlayAudio = mpConfig->curIndex;
        }
        //Enable demuxer here, and Resume Decoder on EditDone
        //AM_ASSERT(DEC_MAP(mpConfig->curIndex) == DEC_MAP(index)); may be no processed.
        mpVideoDecoder->Pause(DEC_MAP(index));
        mpConfig->demuxerConfig[mpConfig->curIndex].edited = AM_TRUE;
        mpDemuxer->Config(mpConfig->curIndex);
        //wait some to wakeup
        return err;
    }
    if(flag & SOURCE_EDIT_ADDBACKGROUND)
    {
        CDemuxerConfig* addConfig = NULL;
        MdecInfo::MdecUnitInfo* addInfo = NULL;

        order = mpMdecInfo->unitInfo[index].addOrder;
        mpConfig->curIndex = InfoChooseIndex(isNet, flag, order + 1);
        addConfig = &(mpConfig->demuxerConfig[mpConfig->curIndex]);
        addInfo = &(mpMdecInfo->unitInfo[mpConfig->curIndex]);
        mbNetStream = addConfig->netused = addInfo->isNet = isNet;
        err = InitSourceConfig(source, flag, isNet);
        //disable audio and video
        addConfig->disableAudio = addConfig->disableVideo = AM_TRUE;
        addConfig->hided = AM_TRUE;
        err = mpDemuxer->AcceptMedia(mpCurSource);
        if(err != ME_OK){
            mpMdecInfo->unitInfo[mpConfig->curIndex].isUsed = AM_FALSE;
            AM_ERROR("Add Background Source :%s Failed!\n", mpCurSource);
            return err;
        }
        addConfig->disableVideo = AM_FALSE;
        //increase curIndex
        addInfo->addOrder = order;
        mpMdecInfo->unitInfo[index].addOrder = order + 1;
        mpMdecInfo->winOrder[source] = order + 1;
        mpDemuxer->Config(mpConfig->curIndex);//net need to keep live, for local, well as same.
        return err;
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleBackSource(AM_INT sourceGroup, AM_INT orderLevel, AM_INT flag)
{
    if(mbOnNVR == AM_FALSE){
        AM_INFO("Please edit source during NVR mode. Use '-b' back to NVR.\n");
        return ME_CLOSED;
    }
    AM_ERR err = ME_OK;
    AM_INT aIndex, index, order;
    CDemuxerConfig* curConfig = NULL;
    AM_BOOL hasAudio = AM_FALSE;

    aIndex = mpConfig->GetAudioSource();
    index = InfoChangeWinIndex(sourceGroup);
    //handle HD source
    if(flag & SOURCE_FULL_HD){
        AM_INT hdIndex = InfoFindHDIndex(index);
        AM_INT hdOrder = mpMdecInfo->unitInfo[hdIndex].addOrder;
        if(hdOrder <= orderLevel || orderLevel < 0){
            AM_INFO("Back HD source(cur order:%d), the specify orderLevel(%d) wrong!\n", hdOrder, orderLevel);
            return ME_BAD_PARAM;
        }
        while(hdOrder > orderLevel)
        {
            AM_INFO("Back From HD source order:%d, index:%d, curHd:%d.\n", hdOrder, hdIndex, mpConfig->curHdIndex);
            if(hdIndex == mpConfig->curHdIndex){
                if(DEC_MAP(hdIndex)!=-1){
                    mpVideoDecoder->Pause(DEC_MAP(hdIndex));
                    mpVideoDecoder->Remove(DEC_MAP(hdIndex), DEC_REMOVE_JUST_NOTIFY);
                    mpVideoDecoder->Resume(DEC_MAP(hdIndex));
                    AM_ASSERT(REN_MAP(hdIndex) != -1);
                    mpRenderer->Remove(REN_MAP(hdIndex));
                }else{
                    AM_INFO("curHD[%d] is not configed!!\n", hdIndex);
                }
                mpDemuxer->Remove(hdIndex);
                mpConfig->curHdIndex = -1;
            }else{
                if(REN_MAP(hdIndex) != -1)
                    mpRenderer->Remove(REN_MAP(hdIndex));
                mpDemuxer->Remove(hdIndex);
            }
            mpMdecInfo->ClearUnitInfo(hdIndex);
            mpConfig->DeInitMap(hdIndex);
            hdIndex = InfoFindHDIndex(index);
            hdOrder = mpMdecInfo->unitInfo[hdIndex].addOrder;
        }
        AM_ASSERT(hdOrder == orderLevel);
        AM_INFO("Cur HD source for group:%d is %d, the order:%d.\n", sourceGroup, hdIndex, hdOrder);
        return ME_OK;
    }
    //SD
    order = mpMdecInfo->unitInfo[index].addOrder;
    if(order <= orderLevel || orderLevel < 0){
        AM_INFO("Back source(cur order:%d), the specify orderLevel(%d) wrong!\n", order, orderLevel);
        return ME_BAD_PARAM;
    }

    if(aIndex == index){
        DoRemoveAudioStream(aIndex);
        hasAudio = AM_TRUE;
    }

    while(order > orderLevel)
    {
        AM_INFO("Back From source order:%d, index:%d.\n", order, index);
        //remove cur order
        curConfig = &(mpConfig->demuxerConfig[index]);
        mpVideoDecoder->Pause(DEC_MAP(index));
        mpRenderer->Pause(REN_MAP(index));
        curConfig->hided = AM_TRUE;
        mpDemuxer->Config(index);
        DoFlushSpecifyDspMw(index, 1);
        mpVideoDecoder->Remove(DEC_MAP(index), DEC_REMOVE_JUST_NOTIFY);
        mpVideoDecoder->Resume(DEC_MAP(index));
        mpRenderer->Remove(REN_MAP(index));
        mpDemuxer->Remove(index);
        mpMdecInfo->ClearUnitInfo(index);
        mpConfig->DeInitMap(index);

        index = InfoChangeWinIndex(sourceGroup);
        order = mpMdecInfo->unitInfo[index].addOrder;
    }
    AM_ASSERT(order == orderLevel);
    //show this order
    AM_INFO("Begin show source:%d, order:%d.\n", index, order);
    curConfig = &(mpConfig->demuxerConfig[index]);
    curConfig->hided = AM_FALSE;
    curConfig->sendHandleV = AM_TRUE;
    mpDemuxer->Config(index);
    if(hasAudio)
        DoStartSpecifyAudio(index);

    mpMdecInfo->winOrder[sourceGroup] = order;

    return err;
}

AM_ERR CActiveMDecEngine::HandleDeleteSource(AM_INT sourceGroup, AM_INT flag)
{
    //delete all the streams on sourceGroup
    AM_INFO("HandleDeleteSource for %d.\n", sourceGroup);
    if(mbOnNVR == AM_FALSE){
        AM_INFO("Please delete source during NVR mode. Use '-b' back to NVR.\n");
        return ME_CLOSED;
    }
    if(mAudioWait == AUDIO_WAIT_DONE){
        AM_INFO("Wait audio stream construction done.\n");
        return ME_BUSY;
    }
//    AM_ERR err = ME_OK;
    AM_INT aIndex, index, order;
    AM_INT hdIndex, hdOrder;
    CDemuxerConfig* curConfig = NULL;
    AM_BOOL hasAudio = AM_FALSE;

    aIndex = mpConfig->GetAudioSource();
    index = InfoChangeWinIndex(sourceGroup);
    if(index < 0){
        AM_ERROR("Sources already be deleated, check for the HD source.\n");
        return ME_BAD_STATE;
    }
    //delete hd sources
    hdIndex = InfoFindHDIndex(index);
    if(hdIndex >= 0){
        hdOrder = mpMdecInfo->unitInfo[hdIndex].addOrder;
        while(1)
        {
            AM_INFO("Delete the HD sources cur_order:%d, index:%d, curHd:%d.\n", hdOrder, hdIndex, mpConfig->curHdIndex);
            if(hdIndex == mpConfig->curHdIndex){
                mpVideoDecoder->Pause(DEC_MAP(hdIndex));
                mpVideoDecoder->Remove(DEC_MAP(hdIndex), DEC_REMOVE_JUST_NOTIFY);
                mpVideoDecoder->Resume(DEC_MAP(hdIndex));
                AM_ASSERT(REN_MAP(hdIndex) != -1);
                mpRenderer->Remove(REN_MAP(hdIndex));
                mpDemuxer->Remove(hdIndex);
                mpConfig->curHdIndex = -1;
            }else{
                if(REN_MAP(hdIndex) != -1)
                    mpRenderer->Remove(REN_MAP(hdIndex));
                mpDemuxer->Remove(hdIndex);
            }
            mpMdecInfo->ClearUnitInfo(hdIndex);
            mpConfig->DeInitMap(hdIndex);
            mpConfig->sourceNum--;
            hdIndex = InfoFindHDIndex(index);
            if(hdIndex < 0){
                break;
            }
            hdOrder = mpMdecInfo->unitInfo[hdIndex].addOrder;
        }
        AM_INFO("Delete the HD sources done.\n");
    }
    //delete sd sources
    if(aIndex == index){
        DoRemoveAudioStream(aIndex);
        hasAudio = AM_TRUE;
    }
    order = mpMdecInfo->unitInfo[index].addOrder;
    while(1)
    {
        AM_INFO("Delete the SD sources cur_order:%d, index:%d.\n", order, index);
        //remove cur order
        curConfig = &(mpConfig->demuxerConfig[index]);
        mpVideoDecoder->Pause(DEC_MAP(index));
        mpRenderer->Pause(REN_MAP(index));
        curConfig->hided = AM_TRUE;
        mpDemuxer->Config(index);
        DoFlushSpecifyDspMw(index);
        mpVideoDecoder->Remove(DEC_MAP(index), DEC_REMOVE_JUST_NOTIFY);
        mpVideoDecoder->Resume(DEC_MAP(index));
        mpRenderer->Remove(REN_MAP(index));
        mpDemuxer->Remove(index);
        mpMdecInfo->ClearUnitInfo(index);
        mpConfig->DeInitMap(index);
        mpConfig->sourceNum--;
        index = InfoChangeWinIndex(sourceGroup);
        if(index < 0)
            break;
        order = mpMdecInfo->unitInfo[index].addOrder;
    }
    AM_INFO("Delete the SD sources done.\n");
    if(hasAudio){
        //Enable other one audio
        AM_INT i = 0;
        MdecInfo::MdecUnitInfo* curInfo = NULL;
        AM_INT index = -1;
        for(; i < MDEC_SOURCE_MAX_NUM; i++)
        {
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE &&
                curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
                index = i;
                break;
            }
        }
        if(index >= 0){
            AM_INFO("Select new audio index:%d.\n", index);
            DoStartSpecifyAudio(index);
            mNvrAudioIndex = index;
            if(0 == (mpConfig->globalFlag & NO_AUDIO_ON_GMF)) {mAudioWait = AUDIO_WAIT_DONE;}
        }
    }
    mpMdecInfo->winOrder[sourceGroup] = 0;
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleAddSource(AM_INT source, AM_INT flag)
{
    AM_INFO("HandleAddSource.\n");
    AM_ERR err;
    AM_BOOL isNet = AM_FALSE;
    err = CheckBeforeAddSource(source, flag);
    if(err != ME_OK)
        return err;

    if(mpConfig->sourceNum >= MDEC_SOURCE_MAX_NUM){
        AM_ERROR("Too Many Sources, Total max source num is :%d.\n", MDEC_SOURCE_MAX_NUM);
        return ME_TOO_MANY;
    }
    //mpConfig->curIndex = mpConfig->sourceNum;
    if(strstr(mpCurSource, "rtsp://") != NULL){
        AM_INFO("NetPlayback!\n");
        isNet = AM_TRUE;
    }
    mpConfig->curIndex = InfoChooseIndex(isNet, flag);
    AM_ASSERT(mpConfig->curIndex >=0);

    mbNetStream = mpConfig->demuxerConfig[mpConfig->curIndex].netused = isNet;
    mpMdecInfo->unitInfo[mpConfig->curIndex].isNet = isNet;
    err = InitSourceConfig(source, flag, isNet);
    if(err != ME_OK)
        return err;

    err = mpDemuxer->AcceptMedia(mpCurSource, mState != STATE_IDLE);
    if(err != ME_OK){
        //revert info&config
        mpMdecInfo->unitInfo[mpConfig->curIndex].isUsed = AM_FALSE;
        mpConfig->DeInitMap(mpConfig->curIndex);
        mpConfig->curIndex = -1;
        AM_ERROR("Accept Source :%s Failed!\n", mpCurSource);
        return err;
    }

    //Add Success
    mpConfig->sourceNum++;
    mpMdecInfo->unitInfo[mpConfig->curIndex].addOrder = 0;

    //parse source ip
    AM_UINT tmp[4];
    char str[128];
    if(5==sscanf(mpCurSource, "rtsp://%u.%u.%u.%u/%s", &(tmp[0]), &(tmp[1]), &(tmp[2]), &(tmp[3]), str)){
        AM_INFO("Get source IP: %u.%u.%u.%u\n", tmp[0], tmp[1], tmp[2], tmp[3]);
        mpMdecInfo->unitInfo[mpConfig->curIndex].sourceIP = (tmp[0] & 0xff) << 24 | (tmp[1] & 0xff) << 16 | (tmp[2] & 0xff) << 8 | (tmp[3] & 0xff);
    }
    AM_ASSERT(1);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleSelectAudio(AM_INT winIndex)
{
//    AM_ERR err;
    AM_INT index;
    AM_INT i = 0;
    if(mbOnNVR == AM_FALSE)
    {
        AM_ERROR("Cannot Select a Audio when play one FHD stream, Switch to Nvr First!\n");
        return ME_BAD_STATE;
    }
    if(mpConfig->globalFlag & NO_AUDIO_ON_GMF){
        AM_ERROR("HandleSelectAudio: audio is disabled!\n");
        return ME_OK;
    }

    AM_INT srcgroup = mpLayout->GetSourceGroupByWindow(winIndex);
    LAYOUT_TYPE layout = mpLayout->GetLayoutType();
    if(((layout==LAYOUT_TYPE_TELECONFERENCE) ||
        (layout==LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN)) &&
        (mbNohd == AM_FALSE) &&
        (winIndex == 0)){
        index = InfoHDByWindow(srcgroup);
    }else index = InfoSDByWindow(srcgroup);
    if(index < 0)
        return ME_BAD_PARAM;

    CDemuxerConfig* curConfig = &(mpConfig->demuxerConfig[index]);
    if(curConfig->hasAudio == AM_FALSE){
        AM_INFO("Selected stream(%d) has no audio!\n", winIndex);
        return ME_CLOSED;
    }

    MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[index]);

    AM_ASSERT(curInfo->isUsed != AM_FALSE);
    //AM_ASSERT(curInfo->is1080P != AM_TRUE);
    if(curInfo->playStream & TYPE_AUDIO){
        AM_INFO("Selected audio is already Played.\n");
        return ME_OK;
    }

    DoRemoveAudioStream();
    AM_INFO("Change Audio to Source:(%d)......\n", index);
    //Dump();
    curInfo = &(mpMdecInfo->unitInfo[0]);
    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        if(curInfo->isUsed == AM_TRUE){
            CDemuxerConfig* pDemConfig = &(mpConfig->demuxerConfig[i]);
            if(i == index)
            {
                curInfo->playStream |= TYPE_AUDIO;
                pDemConfig->disableAudio = AM_FALSE;
            }else{
                curInfo->playStream &= ~TYPE_AUDIO;
                pDemConfig->disableAudio = AM_TRUE;
            }
        }
        curInfo = &(mpMdecInfo->unitInfo[i+1]);
    }
    DoStartSpecifyAudio(index);
    mNvrAudioIndex = InfoChangeWinIndex(srcgroup);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleTalkBack(AM_INT enable, AM_UINT winIndexMask)
{
    AM_ERR err = ME_OK;

    if((enable == 1) && (mbTalkBackStreamOn == AM_FALSE)){
        AM_ASSERT(mTalkBackIndexMask == 0);
        err = StartAudioProxy(mTalkBackURL, mTalkBackName);
        if(err == ME_OK){
            mbTalkBackStreamOn = AM_TRUE;
        }else{
            AM_ERROR("StartAudioProxy failed: %d\n", err);
            return err;
        }
    }

    for(int i = 0; i<MDEC_SOURCE_MAX_NUM; i++){
        if((winIndexMask & (0x01 << i)) == 0) continue;
        AM_INT srcgroup = mpLayout->GetSourceGroupByWindow(i);

        {
            //send msg to camera
            char cameraIP[64] = "";
            int port = 20123;//hardcode here, should be same with camera side
            //int max_len = 32;

            int sockfd;
            char buffer[128];
            struct sockaddr_in server_addr;
            int len;
            struct MSGs{
                char type[16];
                char stream[16];
                char url[64];
            };
            MSGs msg;

            if(enable==1) {strcpy(msg.type, "start");}
            else {strcpy(msg.type, "stop");}
            strcpy(msg.stream, mTalkBackName);
            strcpy(msg.url, mTalkBackURL);

            //get camera ip
            AM_UINT sCamIP = mpMdecInfo->FindIPByIndex(srcgroup);
            if(sCamIP != 0) {
                do{
                    sprintf(cameraIP, "%u.%u.%u.%u", sCamIP>>24, (sCamIP>>16) &0x0ff, (sCamIP>>8) &0x0ff, sCamIP &0x0ff);
                    sockfd = socket(AF_INET,SOCK_STREAM,0);
                    if(sockfd < 0) {
                        AM_ERROR("HandleTalkBack create sockfd failed!\n");
                        break;
                    }

                    bzero(&server_addr,sizeof(server_addr));
                    server_addr.sin_family=AF_INET;
                    server_addr.sin_port=htons(port);
                    server_addr.sin_addr.s_addr = inet_addr(cameraIP);

                    if(connect(sockfd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr))==-1) {
                        AM_ERROR("HandleTalkBack connect %s:%d failed! error: %d, %s\n",
                            cameraIP, port, errno, strerror(errno));
                        break;
                    }

                    memset(buffer, 0, sizeof(buffer));
                    memcpy(buffer,&msg,sizeof(MSGs));

                    len=send(sockfd,buffer,sizeof(buffer),0);
                    if(len < 0){
                        AM_ERROR("HandleTalkBack send failed! error: %d, %s\n" ,errno, strerror(errno));
                        break;
                    }else{
                        AM_INFO("msg Send Success[%d(%d)], to %s:%d, %s %s!!\n", i, winIndexMask, cameraIP, port, msg.type, msg.stream);
                    }
                }while(0);
                if(sockfd>0) close(sockfd);
            }

            if(enable == 1) mTalkBackIndexMask |= (0x01<< i);
            else mTalkBackIndexMask &= ~(0x01<< i);
        }
    }

    if((enable != 1) && (mbTalkBackStreamOn == AM_TRUE) && (mTalkBackIndexMask==0)){
        err = StopAudioProxy(mTalkBackURL);
        if(err == ME_OK){
            mbTalkBackStreamOn = AM_FALSE;
            strcpy(mTalkBackURL, "");
        }else{
            AM_ERROR("StopAudioProxy failed: %d\n", err);
            return err;
        }
    }
    AM_INFO("HandleTalkBack: %s talkback to window %x done.\n", (enable==1)?"enable":"disable", winIndexMask);
    return err;
}

AM_ERR CActiveMDecEngine::HandleConfigWindow(AM_INT index, CParam* pParam)
{
    CParam param = *pParam;
    CDspWinConfig* winConfig = &(mpConfig->dspWinConfig);
    AM_INT dspIndex = DSP_MAP(index);

    if(dspIndex < 0 || dspIndex >= winConfig->winNumConfiged)
        return ME_BAD_PARAM;

    if(param[0] >= 0 && param[1] >= 0 && param[2] > 0 && param[3] > 0){
        winConfig->winConfig[dspIndex].winOffsetX = param[0];
        winConfig->winConfig[dspIndex].winOffsetY = param[1];
        winConfig->winConfig[dspIndex].winWidth= param[2];
        winConfig->winConfig[dspIndex].winHeight= param[3];
        winConfig->winChanged = AM_TRUE;
    }else{
        return ME_ERROR;
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleConfigWinMap(AM_INT sourceIndex, AM_INT win, AM_INT zOrder)
{
    AM_INT dspIndex = DSP_MAP(sourceIndex);
    //maybe dsp instance is not mapped
    if(dspIndex < 0)
        dspIndex = win;

    CDspRenConfig* renConfig = &(mpConfig->dspRenConfig);

    if(renConfig->renChanged == AM_FALSE)
        renConfig->renNumNeedConfig = 0;

    AM_ASSERT(renConfig->renConfig[zOrder].renIndex == zOrder);
    renConfig->renConfig[zOrder + 1].dspIndex = dspIndex;
    renConfig->renConfig[zOrder + 1].winIndex = win;
    renConfig->renNumNeedConfig++;
    renConfig->renChanged = AM_TRUE;
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandlePerformWinConfig()
{
    AM_INT index = 0; // any one which can carry this cmd is ok

    CDspWinConfig* winConfig = &(mpConfig->dspWinConfig);
    CDspRenConfig* renConfig = &(mpConfig->dspRenConfig);
    if(renConfig->renNumNeedConfig > 0 && !mbNohd)
        renConfig->renNumNeedConfig++;//for hd

    mpConfig->generalCmd |= RENDEER_SPECIFY_CONFIG;
    mpConfig->specifyIndex = index;
    mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = CONFIG_WINDOW_RENDER;
    mpRenderer->Config(index);
    mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = 0;
    mpConfig->generalCmd &= ~RENDEER_SPECIFY_CONFIG;

    winConfig->winChanged = renConfig->renChanged = AM_FALSE;
    return ME_OK;
}

AM_ERR CActiveMDecEngine::PlaybackZoom(AM_INT index, AM_U16 w, AM_U16 h, AM_U16 x, AM_U16 y)
{
    AM_ASSERT(mpRenderer);
    if (mpRenderer) {
        return mpRenderer->PlaybackZoom(index, w, h, x, y);
    }
    return ME_BAD_STATE;
}

AM_ERR CActiveMDecEngine::HandleSpeedFeedingRule(AM_INT target, AM_UINT speed, IParameters::DecoderFeedingRule feeding_rule, AM_UINT flush)
{
    AM_INT dec_target = 0;
    AM_INT demuxer_target = 0;
    AM_ERR err = ME_ERROR;
    am_pts_t last_pb_time = 0;
    CParam64 parVec(3);
    AM_INT aIndex;

    if (0 > target) {
        if (mbOnNVR) {
            AM_ERROR("NOT in hd's case\n");
            return ME_BAD_PARAM;
        } else {
            demuxer_target = mHdInfoIndex;
            dec_target = DEC_MAP(mHdInfoIndex);
         }
    } else if (AllTargetTag == target) {
        demuxer_target = CMD_EACH_COMPONENT;
        dec_target = AllTargetTag;
    } else if (target < MDEC_SOURCE_MAX_NUM) {
        demuxer_target = InfoChangeWinIndex(target);
        dec_target = DEC_MAP(InfoChangeWinIndex(target));
    } else {
        AM_ERROR("target(%d) excced max value(%d)\n", target, MDEC_SOURCE_MAX_NUM);
        return ME_BAD_PARAM;
    }

    if(speed == 0){
        AM_INFO("HandleSpeedFeedingRule: just reset pb speed!!\n\n");
        mpDemuxer->UpdatePBDirection(CMD_EACH_COMPONENT, 0);
        err = mpVideoDecoder->UpdatePBSpeed(AllTargetTag, 1, 0, IParameters::DecoderFeedingRule_AllFrames);
        return ME_OK;
    }

    //disable audio
    aIndex = mpConfig->GetAudioSource();
    if ((0x0100 != speed) && (!mbTrickPlayDisabledAudio)
        &&((demuxer_target == aIndex) || (demuxer_target == CMD_EACH_COMPONENT))) {
        AMLOG_WARN("not fw 1x speed, disable audio\n");
        DoRemoveAudioStream(aIndex);
        mbTrickPlayDisabledAudio = 1;
        mNvrAudioIndex = aIndex;//use to remember the audio stream to resume
    }

    //flush if specified
    if (flush) {
        if (AllTargetTag != dec_target) {
            err = mpVideoDecoder->QueryInfo(dec_target, INFO_TIME_PTS, parVec);
            if(err != ME_OK){
                AM_ERROR("seek fail\n");
                last_pb_time = 0;
            } else {
                last_pb_time = parVec[1];
            }

            mpDemuxer->Pause(demuxer_target);
            mpVideoDecoder->Pause(dec_target);
            /*if (mbTrickPlayDisabledAudio) {
                mpAudioSinkFilter->Pause(demuxer_target);
            }*/
            DoFlushSpecifyDspMw(demuxer_target, 1);

            mpDemuxer->SeekTo(demuxer_target, last_pb_time);
        } else {
            for(int idx=0; idx<mUdecNum; idx++){
                demuxer_target = InfoChangeWinIndex(idx);
                dec_target = DEC_MAP(demuxer_target);
                AM_INFO("demuxer_target %d, dec_target %d (%d).\n", demuxer_target, dec_target, idx);
                if((demuxer_target<0)||(dec_target<0)) continue;
                if(mpConfig->demuxerConfig[demuxer_target].hided == AM_FALSE)
                    mpVideoDecoder->Pause(dec_target);
            }
            if(mHdInfoIndex!=-1){
                mpVideoDecoder->Pause(DEC_MAP(mHdInfoIndex));
            }
            for(int idx=0; idx<mUdecNum; idx++){
                demuxer_target = InfoChangeWinIndex(idx);
                dec_target = DEC_MAP(demuxer_target);
                AM_INFO("demuxer_target %d, dec_target %d (%d).\n", demuxer_target, dec_target, idx);
                if((demuxer_target<0)||(dec_target<0)) continue;
                if(mpConfig->demuxerConfig[demuxer_target].hided != AM_FALSE) continue;
                if(last_pb_time==0){
                    err = mpVideoDecoder->QueryInfo(dec_target, INFO_TIME_PTS, parVec);
                    if(err != ME_OK){
                        AM_ERROR("seek fail\n");
                        last_pb_time = 0;
                    } else {
                        last_pb_time = parVec[1];
                    }
                }
                mpDemuxer->Pause(demuxer_target);
                //mpVideoDecoder->Pause(dec_target);
                DoFlushSpecifyDspMw(demuxer_target, 1);
                mpDemuxer->SeekTo(demuxer_target, last_pb_time);
            }
            if(mHdInfoIndex!=-1){
                demuxer_target = mHdInfoIndex;
                dec_target = DEC_MAP(demuxer_target);
                AM_INFO("demuxer_target[HD] %d, dec_target %d.\n", demuxer_target, dec_target);
                if(last_pb_time==0){
                    err = mpVideoDecoder->QueryInfo(dec_target, INFO_TIME_PTS, parVec);
                    if(err != ME_OK){
                        AM_ERROR("seek fail\n");
                        last_pb_time = 0;
                    } else {
                        last_pb_time = parVec[1];
                    }
                }
                mpDemuxer->Pause(mHdInfoIndex);
                DoFlushSpecifyDspMw(mHdInfoIndex, 1);
                mpDemuxer->SeekTo(mHdInfoIndex, last_pb_time);
            }
            demuxer_target = CMD_EACH_COMPONENT;
            dec_target = AllTargetTag;
        }
    }

    AMLOG_WARN("target 0x%08x, demuxer %08x, dec %08x, speed 0x%04x\n", target, demuxer_target, dec_target, speed);
    if (mpDemuxer) {
        if (AllTargetTag != dec_target) {
            if (speed & 0x8000) {
                err = mpDemuxer->UpdatePBDirection(demuxer_target, 1);
            } else {
                err = mpDemuxer->UpdatePBDirection(demuxer_target, 0);
            }
            AM_ASSERT(ME_OK == err);
        }else{
        /*
            for(int idx=0; idx<mUdecNum; idx++){
                demuxer_target = InfoChangeWinIndex(idx);
                dec_target = DEC_MAP(demuxer_target);
                AM_INFO("demuxer_target %d, dec_target %d (%d).\n", demuxer_target, dec_target, idx);
                if((demuxer_target<0)) continue;
                if (speed & 0x8000) {
                    err = mpDemuxer->UpdatePBDirection(demuxer_target, 1);
                } else {
                    err = mpDemuxer->UpdatePBDirection(demuxer_target, 0);
                }
                AM_ASSERT(ME_OK == err);
            }*/
            if (speed & 0x8000) {
                err = mpDemuxer->UpdatePBDirection(demuxer_target, 1);
            } else {
                err = mpDemuxer->UpdatePBDirection(demuxer_target, 0);
            }
            demuxer_target = CMD_EACH_COMPONENT;
            dec_target = AllTargetTag;
        }
    }
    if (mpVideoDecoder) {
        if (AllTargetTag != dec_target)
            err = mpVideoDecoder->UpdatePBSpeed(dec_target, speed >> 8, (speed & 0xff), feeding_rule);
        else{
            /*
            for(int idx=0; idx<mUdecNum; idx++){
                demuxer_target = InfoChangeWinIndex(idx);
                dec_target = DEC_MAP(demuxer_target);
                AM_INFO("demuxer_target %d, dec_target %d (%d).\n", demuxer_target, dec_target, idx);
                if((dec_target<0)) continue;
                err = mpVideoDecoder->UpdatePBSpeed(dec_target, speed >> 8, (speed & 0xff), feeding_rule);
            }*/
            err = mpVideoDecoder->UpdatePBSpeed(dec_target, speed >> 8, (speed & 0xff), feeding_rule);
            demuxer_target = CMD_EACH_COMPONENT;
            dec_target = AllTargetTag;
        }
    } else {
        AM_ERROR("NULL mpVideoDecoder\n");
        return ME_ERROR;
    }

    //resume
    if (flush) {
        if (AllTargetTag != dec_target) {
            mpDemuxer->Resume(demuxer_target);
            mpVideoDecoder->Resume(dec_target);
            /*if (mbTrickPlayDisabledAudio) {
                mpAudioSinkFilter->Resume(demuxer_target);
            }*/
        }else{
            for(int idx=0; idx<mUdecNum; idx++){
                demuxer_target = InfoChangeWinIndex(idx);
                dec_target = DEC_MAP(demuxer_target);
                AM_INFO("demuxer_target %d, dec_target %d (%d).\n", demuxer_target, dec_target, idx);
                if((demuxer_target<0)||(dec_target<0)) continue;
                if(mpConfig->demuxerConfig[demuxer_target].hided != AM_FALSE) continue;
                mpDemuxer->Resume(demuxer_target);
                AM_INFO("mpDemuxer->Resume %d.\n", demuxer_target);
                mpVideoDecoder->Resume(dec_target);
                AM_INFO("mpVideoDecoder->Resume %d.\n", dec_target);
            }
            if(mHdInfoIndex!=-1){
                dec_target = DEC_MAP(mHdInfoIndex);
                mpDemuxer->Resume(mHdInfoIndex);
                AM_INFO("mpDemuxer->Resume %d.\n", mHdInfoIndex);
                mpVideoDecoder->Resume(dec_target);
                AM_INFO("mpVideoDecoder->Resume %d.\n", dec_target);
            }
            demuxer_target = CMD_EACH_COMPONENT;
            dec_target = AllTargetTag;
        }
    }

    //enable audio, do this after videoDec resumed, because audio need to get pts
    //usleep(10000);//FixME: ugly, wait decoder flush&resume done
    if ((0x0100 == speed) && mbTrickPlayDisabledAudio && ((mNvrAudioIndex == demuxer_target) || (demuxer_target == CMD_EACH_COMPONENT))) {
        AMLOG_WARN("change to fw 1x speed, enable audio automatically(correct here?)\n");
        DoStartSpecifyAudio(mNvrAudioIndex);
        mbTrickPlayDisabledAudio = 0;
    }

    return err;
}

AM_ERR CActiveMDecEngine::HandlePrepare()
{
//    AM_ERR err;
    //CHOOSE AUDIO
    DoAutoChooseAudioStream();

    //HANDE MDINFO TO DO SOMETHING.
    UpdateUdecConfigInfo();
    SetState(STATE_PREPARED);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleStop()
{
    AM_ASSERT(mpClockManager);
    //mpClockManager->StopClock();
    ClearGraph();
    //ResetParameters();
    NewSession();
    //DeInitDspAudio();
    AMLOG_INFO("****CActiveVEEngine MDCMD_STOP cmd, done.\n");
    SetState(STATE_STOPPED);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleSeekTo(AM_U64 ms, AM_INT index)
{
    AM_INT aIndex = 0, sdindex, hdIndex, curindex = -1;
    AM_INT curIdx[10];

    AM_ERR err = ME_OK;
   // AM_INT aIndex, index, order;
    CDemuxerConfig* curConfig = NULL;
    //AM_BOOL hasAudio = AM_FALSE;
    AM_BOOL seekAll = AM_FALSE;
    AM_INT num = 0;

    if(index>=MDEC_SOURCE_MAX_NUM+1){
        AM_INFO("Seek all streams that are not hided!\n");
        seekAll = AM_TRUE;
    }else{
        AM_INFO("Seek window index: %d!\n", index);
    }
    if(mbNohd ==  AM_TRUE){
        int i = (seekAll ? 0 : index);
        for(; i<(seekAll ? MDEC_SOURCE_MAX_NUM : index+1); i++){
            sdindex = InfoChangeWinIndex(i);
            if((sdindex!=-1) && (mpConfig->demuxerConfig[sdindex].hided==AM_FALSE)){
                if(num>mUdecNum){
                    AM_ERROR("Too many index are not hided!\n");
                    for(int ii=0; ii<mUdecNum; ii++){
                        AM_INFO("\t>>%d: %d.\n", ii, curIdx[ii]);
                    }
                    break;
                }
                curIdx[num] = sdindex;
                num++;
            }
        }
    }else{
        int i = (seekAll ? 0 : index);
        for(; i<(seekAll ? MDEC_SOURCE_MAX_NUM : index+1); i++){
            sdindex = InfoSDByWindow(i);
            hdIndex = InfoHDByWindow(i);
            if((sdindex!=-1) && (mpConfig->demuxerConfig[sdindex].hided==AM_FALSE)){
                curIdx[num] = sdindex;
            }else if((hdIndex!=-1) && (mpConfig->demuxerConfig[hdIndex].hided==AM_FALSE)){
                curIdx[num] = hdIndex;
            }else{
                continue;
            }
            if(num>mUdecNum){
                AM_ERROR("Too many index are not hided!\n");
                for(int ii=0; ii<mUdecNum; ii++){
                    AM_ERROR("\t>>%d: %d.\n", ii, curIdx[ii]);
                }
                break;
            }
            //curindex[i] = sdindex;
            num++;
        }
    }
    if(num==0){
        AM_ERROR("All sources are hided!\n");
        return ME_BAD_STATE;
    }

    for(int i=0; i<num; i++){
        if(mpConfig->demuxerConfig[curIdx[i]].netused){
                continue;
        }

        AM_INFO("HandleSeekTo: source index: [%d].\n", curIdx[i]);
        curConfig = &(mpConfig->demuxerConfig[curIdx[i]]);
        mpDemuxer->Pause(curIdx[i]);
        mpVideoDecoder->Pause(DEC_MAP(curIdx[i]));

        if(curConfig->edited != AM_TRUE){
            aIndex = mpConfig->GetAudioSource();
            if(aIndex == curIdx[i]){
                DoRemoveAudioStream(aIndex);
                //hasAudio = AM_TRUE;
            }
        }
        mpDemuxer->SeekTo(curIdx[i], ms, SEEK_BY_USER);
        curConfig->hided = AM_FALSE;
        mpDemuxer->Config(curIdx[i]);
        err = DoFlushSpecifyDspMw(curIdx[i], 1);

    }
    for(int i=0; i<num; i++){
        if(mpConfig->demuxerConfig[curIdx[i]].netused){
                continue;
        }
        if(curConfig->edited != AM_TRUE){
            mpVideoDecoder->Resume(DEC_MAP(curIdx[i]));
        }
        mpDemuxer->Resume(curIdx[i]);
        if(curConfig->edited != AM_TRUE){
            if(aIndex == curIdx[i]){
                DoStartSpecifyAudio(aIndex, 0);
            }
        }
    }
    mbSeeking = AM_TRUE;
    return err;

//not used
    if(mbNohd ==  AM_TRUE){
        curindex = InfoChangeWinIndex(index);
        if(mpConfig->demuxerConfig[curindex].hided){
            AM_ERROR("NoHD, but this source[%d] is hided!\n", curindex);
            return ME_BAD_STATE;
        }
    }else{
        sdindex = InfoSDByWindow(index);
        hdIndex = InfoHDByWindow(index);
        if(!mpConfig->demuxerConfig[sdindex].hided){
            curindex = sdindex;
        }else if(!mpConfig->demuxerConfig[hdIndex].hided){
            curindex = hdIndex;
        }else{
            AM_ERROR("Both HD[%d] & SD[%d] of this source are hided!\n", hdIndex, sdindex);
            return ME_TRYNEXT;
        }
    }
    if(mpConfig->demuxerConfig[curindex].netused){
        AM_ERROR("NET source[%d], cannot do seek!\n", curindex);
        return ME_BAD_STATE;
    }

    AM_INFO("HandleSeekTo: source index: [%d].\n", curindex);
    curConfig = &(mpConfig->demuxerConfig[curindex]);

    {
        aIndex = mpConfig->GetAudioSource();

        mpDemuxer->Pause(curindex);
        mpVideoDecoder->Pause(DEC_MAP(curindex));

        if(curConfig->edited != AM_TRUE){
            if(aIndex == curindex){
                DoRemoveAudioStream(aIndex);
                //hasAudio = AM_TRUE;
            }
        }
        mpDemuxer->SeekTo(curindex, ms, SEEK_BY_USER);
        curConfig->hided = AM_FALSE;
        mpDemuxer->Config(curindex);
        err = DoFlushSpecifyDspMw(curindex, 1);
        if(curConfig->edited != AM_TRUE){
            mpVideoDecoder->Resume(DEC_MAP(curindex));
        }
        mpDemuxer->Resume(curindex);
        if(curConfig->edited != AM_TRUE){
            //usleep(50000);
            if(aIndex == curindex){
                DoStartSpecifyAudio(aIndex, 0);
            }
        }
    }

    return err;
}

AM_ERR CActiveMDecEngine::HandleDisconnectHw()
{
    if(mbDisconnect == AM_TRUE){
        AM_ERROR("Nvr already disconnectted!");
        return ME_BAD_STATE;
    }
    if(mbOnNVR == AM_FALSE){
        AM_INFO("Please disconnect during NVR mode. Use '-b' back to NVR.\n");
        return ME_CLOSED;
    }

    AM_INFO("HandleDisconnectDsp\n");
    AM_ERR err = ME_OK;
    AM_INT i;//, audio;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
//    CDemuxerConfig* curConfig = NULL;

    //TEST
    if(mbOnNVR == AM_TRUE){
        //Audio
        mNvrAudioIndex = mpConfig->GetAudioSource();
        AM_ASSERT(mNvrAudioIndex > 0);
        AM_INFO("Remove audio:%d on Disconnect Dsp\n", mNvrAudioIndex);
        DoRemoveAudioStream(mNvrAudioIndex);

        InfoHideAllSmall(AM_TRUE);
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE
                && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
                AM_INFO("Handle %d, dec:%d\n", i, DEC_MAP(i));
                mpVideoDecoder->Pause(DEC_MAP(i));
                mpDemuxer->Config(i);
                mpDemuxer->Flush(i);
                mpVideoDecoder->Remove(DEC_MAP(i));
            }
        }
        //handle hdInfoIndex
        if(mHdInfoIndex >= 0)
            mpVideoDecoder->Remove(DEC_MAP(mHdInfoIndex));
    }else{
        /*AM_ASSERT(mHdInfoIndex > 0);
        DoRemoveAudioStream(mHdInfoIndex);

        //remove sd, sd on hided, flushed
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE
                && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
                mpVideoDecoder->Remove(DEC_MAP(i));
            }
        }
        //remove hd
        mpVideoDecoder->Pause(DEC_MAP(mHdInfoIndex));
        curConfig = &(mpConfig->demuxerConfig[mHdInfoIndex]);
        curConfig->hided = AM_TRUE;
        mpDemuxer->Config(mHdInfoIndex);
        mpDemuxer->Flush(mHdInfoIndex);
        mpVideoDecoder->Remove(DEC_MAP(mHdInfoIndex));
        */
    }

    mpConfig->generalCmd |= RENDERER_DISCONNECT_HW;
    mpRenderer->Config(CMD_EACH_COMPONENT);
    mpConfig->generalCmd &= ~RENDERER_DISCONNECT_HW;

    DeInitDspAudio();
    mbDisconnect = AM_TRUE;
    return err;
}

AM_ERR CActiveMDecEngine::HandleReconnectHw()
{
    if(mbDisconnect == AM_FALSE){
        AM_ERROR("Nvr already connectted!");
        return ME_BAD_STATE;
    }
    if(mbOnNVR == AM_FALSE){
        AM_ASSERT(0);
        return ME_CLOSED;
    }

    AM_ERR err = ME_OK;
    AM_INT i;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    CDemuxerConfig* curConfig = NULL;

    if (mShared.udecHandler == NULL)
    {
        err = AM_CreateDSPHandler(&mShared);
        if(err != ME_OK)
            return err;
        //todo
    }
    mpMdecInfo->nvrIavFd = mShared.mIavFd;
    CDspGConfig* pDsp = &mpConfig->dspConfig;
    pDsp->iavHandle = mShared.mIavFd;
    pDsp->udecHandler = mShared.udecHandler;
    pDsp->udecHandler->SetUdecNums(mUdecNum);
    pDsp->udecHandler->InitWindowRender(mpConfig);
    if (IsShowOnLCD()) {
        mpMdecInfo->displayWidth = mShared.dspConfig.voutConfigs.voutConfig[eVoutLCD].height;
        mpMdecInfo->displayHeight = mShared.dspConfig.voutConfigs.voutConfig[eVoutLCD].width;
    } else {
        mpMdecInfo->displayWidth = mShared.dspConfig.voutConfigs.voutConfig[eVoutHDMI].width;
        mpMdecInfo->displayHeight = mShared.dspConfig.voutConfigs.voutConfig[eVoutHDMI].height;
    }
    //Audio handle
    if(!mbEnableTanscode) {
        mpConfig->audioConfig.audioHandle = AM_CreateAudioHAL(this, 1, 0);
        if(mpConfig->audioConfig.audioHandle == NULL)
            return ME_ERROR;
    }
    mpConfig->audioConfig.isOpened = AM_FALSE;

    AM_ASSERT(mNvrAudioIndex >= 0);
    if(mbOnNVR == AM_TRUE){
        InfoHideAllSmall(AM_FALSE);
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            curConfig = &(mpConfig->demuxerConfig[i]);

            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE
                && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
                //re_create
                curConfig->sendHandleV = AM_TRUE;
                mpDemuxer->Config(i);
            }
        }
        //handle hdInfoIndex
        if(mHdInfoIndex >= 0){
            curConfig = &(mpConfig->demuxerConfig[mHdInfoIndex]);
            curConfig->sendHandleV = AM_TRUE;
        }
        DoStartSpecifyAudio(mNvrAudioIndex, 0);
    }else{
        /*AM_ASSERT(mHdInfoIndex >= 0);
        //config sd
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            curConfig = &(mpConfig->demuxerConfig[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE
                && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
                //re_create
                curConfig->sendHandleV = AM_TRUE;
            }
        }
        //enable hd
        curConfig = &(mpConfig->demuxerConfig[mHdInfoIndex]);
        curConfig->hided = AM_FALSE;
        curConfig->sendHandleV = AM_TRUE;
        mpDemuxer->Config(mHdInfoIndex);

        DoStartSpecifyAudio(mHdInfoIndex, 0);
        */
        //have to config the windows to show the hd source, so we just disable it.
    }
    mbDisconnect = AM_FALSE;
    return err;
}

//-----------------------------------------------------------------------
//
//Start Handlers
//
//-----------------------------------------------------------------------
// 1 run should be ok
// 2 stop will be a very strict cmd which will let everything break while
// 3 PurgeFilter will just  free the bp between g-filter
// 4 FLUSH will remove everything!. USED on seek
//5 pause and resume will be ensured and speedy
AM_ERR CActiveMDecEngine::HandleStart(AM_BOOL isNet)
{
    AM_ERR err;
    err = RunAllFilters();
    if (err != ME_OK) {
        AM_ERROR("Run all filter Failed!\n");
        ClearGraph();
        return err;
    }

    SetState(STATE_ONRUNNING);
    //Dump();
    return ME_OK;
}
//------------------------------------------------
//
//Step Handlers
//
//-------------------------------------------------
AM_ERR CActiveMDecEngine::HandleStep(AM_INT winIndex)
{
    AM_ERR err = ME_OK;
    AM_INT sIndex;
    if(winIndex < 0){
        if(mbOnNVR == AM_TRUE){
            AM_INFO("Step HD Failed on NVR playback! Press '-e index' to step specify stream.\n");
            return ME_BAD_PARAM;
        }
        AM_ASSERT(mHdInfoIndex >= 0);
        if(mpConfig->demuxerConfig[mHdInfoIndex].netused == AM_TRUE){
            err = HandleStepHdNet();
        }else{
            err = HandleStepHdLocal();
        }
    }else{
        if(mbOnNVR == AM_FALSE){
            AM_INFO("Step Failed for winIndex:%d, Press '-e' to step hd on one-stream playback.\n", winIndex);
            return ME_BAD_PARAM;
        }
        sIndex = InfoChangeWinIndex(winIndex);
        if(sIndex < 0)
            return ME_BAD_PARAM;
        if(mpConfig->demuxerConfig[mHdInfoIndex].netused == AM_TRUE){
            err = HandleStepSdNet(winIndex);
        }else{
            err = HandleStepSdLocal(winIndex);
        }
    }
    return err;
}

AM_ERR CActiveMDecEngine::HandleStepSdLocal(AM_INT winIndex)
{
    AM_INFO("Step SD Win:%d playback\n", winIndex);
    if(mbOnNVR == AM_FALSE){
        AM_INFO("Step Failed for winIndex:%d, Press '-e' to step hd on one-stream playback.\n", winIndex);
        return ME_BAD_PARAM;
    }

//    AM_ERR err;
//    AM_ERR err;
    AM_INT sIndex = 0;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    if(mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE){
        sIndex = InfoChangeWinIndex(winIndex);
        if(sIndex < 0)
            return ME_BAD_PARAM;
    }else if(mpLayout->GetLayoutType() == LAYOUT_TYPE_TELECONFERENCE){
        AM_INT sourceG = mpLayout->GetSourceGroupByWindow(winIndex);
        sIndex = InfoChangeWinIndex(sourceG);
        if(winIndex == 0)
            sIndex = InfoHDBySD(sIndex);
    }

    curInfo = &(mpMdecInfo->unitInfo[sIndex]);
    if(0)
    {
        if(curInfo->isPaused == AM_FALSE)
            DoPauseAudioStream();
    }

    CRendererConfig* pRenConfig = &(mpConfig->rendererConfig[sIndex]);
    pRenConfig->configCmd = STEPPLAY_RENDER_OUT;
    mpConfig->generalCmd |= RENDEER_SPECIFY_CONFIG;
    mpConfig->specifyIndex = REN_MAP(sIndex);
    mpRenderer->Config(REN_MAP(sIndex));
    mpConfig->generalCmd &= ~RENDEER_SPECIFY_CONFIG;

    AM_INFO("Step SD source:%d successfully.\n", sIndex);
    curInfo->isPaused = AM_TRUE;
    curInfo->isSteped = AM_TRUE;
    curInfo->pausedBy = NOR_PARE_ACTION;
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleStepHdLocal()
{
    AM_INFO("Step HD playback\n");
    if(mbOnNVR == AM_TRUE){
        AM_INFO("Step HD Failed on NVR playback! Press '-e index' to step specify stream.\n");
        return ME_BAD_PARAM;
    }
    AM_ASSERT(mHdInfoIndex >= 0);
    MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[mHdInfoIndex]);

    if(0)
    {
        if(curInfo->isPaused == AM_FALSE)
            DoPauseAudioStream();
    }

    CRendererConfig* pRenConfig = &(mpConfig->rendererConfig[mHdInfoIndex]);
    pRenConfig->configCmd = STEPPLAY_RENDER_OUT;
    mpConfig->generalCmd |= RENDEER_SPECIFY_CONFIG;
    mpConfig->specifyIndex = REN_MAP(mHdInfoIndex);
    mpRenderer->Config(REN_MAP(mHdInfoIndex));
    mpConfig->generalCmd &= ~RENDEER_SPECIFY_CONFIG;

    AM_INFO("Step HD successfully.\n");
    curInfo->isPaused = AM_TRUE;
    curInfo->isSteped = AM_TRUE;
    curInfo->pausedBy = NOR_PARE_ACTION;
    return ME_OK;
}
AM_ERR CActiveMDecEngine::HandleStepSdNet(AM_INT winIndex)
{
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleStepHdNet()
{
    return ME_OK;
}
//------------------------------------------------
//
//Pasue Resume Handlers
//
//-------------------------------------------------
AM_ERR CActiveMDecEngine::HandlePause(AM_INT winIndex)
{
    AM_ERR err = ME_OK;
    AM_INT sIndex;

    if(winIndex == AllTargetTag){
        return HandlePauseAll();
    }
    if(winIndex < 0){
        if(mbOnNVR == AM_TRUE){
            AM_INFO("Pause HD Failed on NVR playback! Press '-p index' to pause specify stream.\n");
            return ME_BAD_PARAM;
        }
        AM_ASSERT(mHdInfoIndex >= 0);
        if(mpConfig->demuxerConfig[mHdInfoIndex].netused == AM_TRUE){
            err = HandlePauseHdNet();
        }else{
            err = HandlePauseHdLocal();
        }
    }else{
        if(mbOnNVR == AM_FALSE){
            AM_INFO("Pause Failed for winIndex:%d, Press '-p' to pause hd on one-stream playback.\n", winIndex);
            return ME_BAD_PARAM;
        }
        sIndex = InfoChangeWinIndex(winIndex);
        if((sIndex < 0) || (sIndex > MDEC_SOURCE_MAX_NUM-1))
            return ME_BAD_PARAM;
        if(mpConfig->demuxerConfig[sIndex].netused == AM_TRUE){
            err = HandlePauseSdNet(winIndex);
        }else{
            err = HandlePauseSdLocal(winIndex);
        }
    }
    return err;
}

AM_ERR CActiveMDecEngine::HandleResume(AM_INT winIndex)
{
    AM_ERR err = ME_OK;
    AM_INT sIndex;

    if(winIndex == AllTargetTag){
        return HandleResumeAll();
    }
    if(winIndex < 0){
        if(mbOnNVR == AM_TRUE){
            AM_INFO("Resume HD Failed on NVR playback! Press '-p index' to pause specify stream.\n");
            return ME_BAD_PARAM;
        }
        AM_ASSERT(mHdInfoIndex >= 0);
        if(mpConfig->demuxerConfig[mHdInfoIndex].netused == AM_TRUE){
            err = HandleResumeHdNet();
        }else{
            err = HandleResumeHdLocal();
        }
    }else{
        if(mbOnNVR == AM_FALSE){
            AM_INFO("Resume Failed for winIndex:%d, Press '-r' to resume hd on one-stream playback.\n", winIndex);
            return ME_BAD_PARAM;
        }
        sIndex = InfoChangeWinIndex(winIndex);
        if((sIndex < 0) || (sIndex > MDEC_SOURCE_MAX_NUM-1))
            return ME_BAD_PARAM;
        if(mpConfig->demuxerConfig[sIndex].netused == AM_TRUE){
            err = HandleResumeSdNet(winIndex);
        }else{
            err = HandleResumeSdLocal(winIndex);
        }
    }
    return err;
}

// for SDs
AM_ERR CActiveMDecEngine::HandlePauseAll()
{
    AM_INT sIndex;
    if(mbOnNVR == AM_FALSE){
        AM_INFO("HandlePauseAll Failed, !mbOnNVR.\n");
        return ME_BAD_PARAM;
    }
    for(int idx=0; idx<mUdecNum; idx++){
        sIndex = InfoChangeWinIndex(idx);
        if((sIndex < 0) || (sIndex > MDEC_SOURCE_MAX_NUM-1)){
            AM_ASSERT(0);
            continue;
        }
        if(mpConfig->demuxerConfig[sIndex].netused == AM_TRUE){
            //AM_INFO("HandlePauseAll[%d-%d]: live stream!\n", idx, sIndex);
        }else{
            mpRenderer->Pause(REN_MAP(sIndex));
            //mpVideoDecoder->Pause(DEC_MAP(sIndex), 1);
            AM_INFO("HandlePauseAll[%d-%d]: Pause dec!\n", idx, sIndex);
        }
    }
    for(int idx=0; idx<mUdecNum; idx++){
        sIndex = InfoChangeWinIndex(idx);
        if((sIndex < 0) || (sIndex > MDEC_SOURCE_MAX_NUM-1)){
            AM_ASSERT(0);
            continue;
        }
        if(mpConfig->demuxerConfig[sIndex].netused == AM_TRUE){
            //AM_INFO("HandlePauseAll[%d-%d]: live stream!\n", idx, sIndex);
        }else{
            //mpRenderer->Pause(REN_MAP(sIndex));
            mpVideoDecoder->Pause(DEC_MAP(sIndex), 0);
        }
    }
    for(int idx=0; idx<mUdecNum; idx++){
        sIndex = InfoChangeWinIndex(idx);
        if((sIndex < 0) || (sIndex > MDEC_SOURCE_MAX_NUM-1)){
            AM_ASSERT(0);
            continue;
        }
        if(mpConfig->demuxerConfig[sIndex].netused == AM_TRUE){
            AM_INFO("HandlePauseAll[%d-%d]: live stream!\n", idx, sIndex);
        }else{
            MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[sIndex]);
            if(curInfo->playStream & TYPE_AUDIO){
                AM_ASSERT(mpConfig->GetAudioSource() == sIndex);
                AM_INFO("HandlePauseAll Pause the Audio Stream(%d).\n", sIndex);
                mpAudioSinkFilter->Pause(sIndex);
            }
            AM_INFO("HandlePauseAll %d-%d:\n", idx, sIndex);
            mpDemuxer->Pause(sIndex);

            AM_INFO("HandlePauseAll %d-%d on NVR successfully.\n", idx, sIndex);
            curInfo->isPaused = AM_TRUE;
            curInfo->pausedBy = NOR_PARE_ACTION;
        }
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleResumeAll()
{
    AM_INT sIndex;
    if(mbOnNVR == AM_FALSE){
        AM_INFO("HandleResumeAll Failed, !mbOnNVR.\n");
        return ME_BAD_PARAM;
    }
    for(int idx=0; idx<mUdecNum; idx++){
        sIndex = InfoChangeWinIndex(idx);
        if((sIndex < 0) || (sIndex > MDEC_SOURCE_MAX_NUM-1)){
            AM_ASSERT(0);
            continue;
        }
        if(mpConfig->demuxerConfig[sIndex].netused == AM_TRUE){
            //AM_INFO("HandlePauseAll[%d-%d]: live stream!\n", idx, sIndex);
        }else{
            mpVideoDecoder->Resume(DEC_MAP(sIndex));
        }
    }
    for(int idx=0; idx<mUdecNum; idx++){
        sIndex = InfoChangeWinIndex(idx);
        if((sIndex < 0) || (sIndex > MDEC_SOURCE_MAX_NUM-1)){
            AM_ASSERT(0);
            continue;
        }
        if(mpConfig->demuxerConfig[sIndex].netused == AM_TRUE){
            //AM_INFO("HandlePauseAll[%d-%d]: live stream!\n", idx, sIndex);
        }else{
            MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[sIndex]);
            if(curInfo->playStream & TYPE_AUDIO){
                AM_ASSERT(mpConfig->GetAudioSource() == sIndex);
                AM_INFO("Resume the Audio Stream(%d).\n", sIndex);
                mpAudioSinkFilter->Resume(sIndex);
            }
            mpRenderer->Resume(REN_MAP(sIndex));
            mpDemuxer->Resume(sIndex);

            AM_INFO("HandleResumeAll %d-%d on NVR successfully.\n", idx, sIndex);
            curInfo->isPaused = AM_FALSE;
            curInfo->pausedBy = -1;
        }
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandlePauseHdLocal()
{
    if(mbOnNVR == AM_TRUE){
        AM_INFO("Pause HD Failed on NVR playback! Press '-p index' to pause specify stream.\n");
        return ME_BAD_PARAM;
    }
    AM_ASSERT(mHdInfoIndex >= 0);

    MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[mHdInfoIndex]);
    if(curInfo->isPaused == AM_TRUE){
        AM_INFO("HD is already paused!\n");
        return ME_OK;
    }

    if(0)
    {
        DoPauseAudioStream();
    }

    mpVideoDecoder->Pause(DEC_MAP(mHdInfoIndex));
    mpRenderer->Pause(REN_MAP(mHdInfoIndex));
    mpDemuxer->Pause(mHdInfoIndex);

    AM_INFO("Pause HD successfully.\n");
    curInfo->isPaused = AM_TRUE;
    curInfo->pausedBy = NOR_PARE_ACTION;
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandlePauseSdLocal(AM_INT winIndex)
{
    AM_INFO("HandlePauseSdLocal.\n");
    if(mbOnNVR == AM_FALSE){
        AM_INFO("Pause Failed for winIndex:%d, Press '-p' to pause hd on one-stream playback.\n", winIndex);
        return ME_BAD_PARAM;
    }

//    AM_ERR err;
    AM_INT sIndex = 0;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    if(mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE){
        sIndex = InfoChangeWinIndex(winIndex);
        if(sIndex < 0)
            return ME_BAD_PARAM;
    }else if(mpLayout->GetLayoutType() == LAYOUT_TYPE_TELECONFERENCE){
        AM_INT sourceG = mpLayout->GetSourceGroupByWindow(winIndex);
        sIndex = InfoChangeWinIndex(sourceG);
        if(winIndex == 0)
            sIndex = InfoHDBySD(sIndex);
    }
    AM_INFO("debug:%d\n", sIndex);
    curInfo = &(mpMdecInfo->unitInfo[sIndex]);
    if(curInfo->isPaused == AM_TRUE){
        AM_INFO("Win source: %d is already paused!\n", winIndex);
        return ME_OK;
    }

    //this may be blocked if after ren->pause!
    mpVideoDecoder->Pause(DEC_MAP(sIndex));
    if(curInfo->playStream & TYPE_AUDIO){
        AM_ASSERT(mpConfig->GetAudioSource() == sIndex);
        AM_INFO("Pause the Audio Stream(%d).\n", sIndex);
        mpAudioSinkFilter->Pause(sIndex);//ADEC_MAP(I) == I;
    }
    mpRenderer->Pause(REN_MAP(sIndex));
    mpDemuxer->Pause(sIndex);

    AM_INFO("Pause win %d on NVR successfully.\n", winIndex);
    curInfo->isPaused = AM_TRUE;
    curInfo->pausedBy = NOR_PARE_ACTION;
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleResumeHdLocal()
{
    if(mbOnNVR == AM_TRUE){
        AM_INFO("Resume HD Failed on NVR playback! Press '-p index' to pause specify stream.\n");
        return ME_BAD_PARAM;
    }
    AM_ASSERT(mHdInfoIndex >= 0);

    MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[mHdInfoIndex]);
    if(curInfo->isPaused == AM_FALSE){
        AM_INFO("HD is already playing!\n");
        return ME_OK;
    }

    if(0)
    {
        DoPauseAudioStream();
    }

    mpRenderer->Resume(REN_MAP(mHdInfoIndex));
    mpVideoDecoder->Resume(DEC_MAP(mHdInfoIndex));
    mpDemuxer->Resume(mHdInfoIndex);

    AM_INFO("Resume HD successfully.\n");
    curInfo->isPaused = AM_FALSE;
    curInfo->isSteped = AM_FALSE;
    curInfo->pausedBy = -1;
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleResumeSdLocal(AM_INT winIndex)
{
    if(mbOnNVR == AM_FALSE){
        AM_INFO("Resume Failed for winIndex:%d, Press '-r' to resume hd on one-stream playback.\n", winIndex);
        return ME_BAD_PARAM;
    }

//    AM_ERR err;
    AM_INT sIndex = 0;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    if(mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE){
        sIndex = InfoChangeWinIndex(winIndex);
        if(sIndex < 0)
            return ME_BAD_PARAM;
    }else if(mpLayout->GetLayoutType() == LAYOUT_TYPE_TELECONFERENCE){
        AM_INT sourceG = mpLayout->GetSourceGroupByWindow(winIndex);
        sIndex = InfoChangeWinIndex(sourceG);
        if(winIndex == 0)
            sIndex = InfoHDBySD(sIndex);
    }

    curInfo = &(mpMdecInfo->unitInfo[sIndex]);
    if(curInfo->isPaused == AM_FALSE){
        AM_INFO("Win source :%d is already playing.\n", winIndex);
        return ME_OK;
    }

    mpRenderer->Resume(REN_MAP(sIndex));
    mpVideoDecoder->Resume(DEC_MAP(sIndex));
    if(curInfo->playStream & TYPE_AUDIO){
        AM_ASSERT(mpConfig->GetAudioSource() == sIndex);
        AM_INFO("Resume the Audio Stream.\n");
        mpAudioSinkFilter->Resume(sIndex);
    }
    mpDemuxer->Resume(sIndex);

    AM_INFO("Resume win %d on NVR successfully.\n", winIndex);
    curInfo->isPaused = AM_FALSE;
    curInfo->isSteped = AM_FALSE;
    curInfo->pausedBy = -1;
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandlePauseHdNet()
{
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandlePauseSdNet(AM_INT winIndex)
{
    mpVideoDecoder->Pause(DEC_MAP(winIndex));
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleResumeHdNet()
{
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleResumeSdNet(AM_INT winIndex)
{
    return ME_OK;
}
//------------------------------------------------
//
//Switch Handlers
//
//-------------------------------------------------
AM_ERR CActiveMDecEngine::HandleRevertNVR()
{
    if(mbNohd){
        return HandleBackNoHD();
    }
    if(mpConfig->globalFlag & SWITCH_ARM_SEEK_FEED_DSP_DEBUG2){
        if(mpConfig->demuxerConfig[mHdInfoIndex].netused == AM_TRUE)
            return HandleBackByArm();
    }

    return HandleSwitchBack();
/*
    if(mpConfig->globalFlag & SWITCH_JUST_FOR_WIN_ARM2){
        return HandleRBNoAudioNetJustWin2();
    }
    if(mpConfig->globalFlag & SWITCH_JUST_FOR_WIN_ARM3){
        return HandleRBNoAudioNetJustWin3();
    }
    if(mpConfig->globalFlag & SWITCH_ARM_SEEK_FEED_DSP_DEBUG2){
        return HandleRBNoAudioNetJustWin2();
    }

    if(mpConfig->globalFlag & SWITCH_JUST_FOR_WIN_ARM){
        //if(mpConfig->globalFlag & USING_FOR_NET_PB)
        if(mpConfig->demuxerConfig[mHdInfoIndex].netused == AM_TRUE)
            return HandleRBNoAudioNetJustWin();
        else
            return HandleRBNoAudioLocalJustWin();
    }

    if(mpConfig->globalFlag & SWITCH_ARM_SEEK_FEED_DSP_DEBUG){
        //if(mpConfig->globalFlag & USING_FOR_NET_PB)
        if(mpConfig->demuxerConfig[mHdInfoIndex].netused == AM_TRUE)
            return HandleRBNoAudioNet();
        else
            return HandleRBNoAudioLocal();
    }

    return HandleRevertNVRTest();
*/
}

AM_ERR CActiveMDecEngine::HandleAutoSwitch(AM_INT winIndex)
{
    AM_ERR err;
    if(mbNohd){
        return HandleSwitchNoHD(winIndex);
    }
    if(mbOnNVR == AM_FALSE && mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE){
        AM_ASSERT(mHdInfoIndex >= 0);
        err = HandleSwitchDuringHd(winIndex);
        return err;
    }

    return HandleSwitchStream(winIndex);
/*
    if(mpConfig->globalFlag & SWITCH_JUST_FOR_WIN_ARM2){
        return HandleSWNoAudioNetJustWin2(winIndex);
    }
    if(mpConfig->globalFlag & SWITCH_JUST_FOR_WIN_ARM3){
        return HandleSWNoAudioNetJustWin3(winIndex);
    }
    if(mpConfig->globalFlag & SWITCH_ARM_SEEK_FEED_DSP_DEBUG2){
        return HandleSWNoAudioNet2(winIndex);
    }

    if(mpConfig->globalFlag & SWITCH_JUST_FOR_WIN_ARM){
        //if(mpConfig->globalFlag & USING_FOR_NET_PB)
        if(mpConfig->demuxerConfig[winIndex].netused == AM_TRUE)
            return HandleSWNoAudioNetJustWin(winIndex);
        else
            return HandleSWNoAudioLocalJustWin(winIndex);
    }

    if(mpConfig->globalFlag & SWITCH_ARM_SEEK_FEED_DSP_DEBUG){
        //if(mpConfig->globalFlag & USING_FOR_NET_PB)
        if(mpConfig->demuxerConfig[winIndex].netused == AM_TRUE)
            return HandleSWNoAudioNet(winIndex);
        else
            return HandleSWNoAudioLocal(winIndex);
    }

    return HandleAutoSwitchTest(winIndex);
*/
}
//-----------------------------------------------------------------------------------------
//Summarize!
// Separate each windows
//-----------------------------------------------------------------------------------------
AM_ERR CActiveMDecEngine::HandleBackNoHD()
{
    AM_INFO("HandleBackNoHD.\n");
    AM_ERR err;
//    AM_INT dspNum = mpConfig->dspConfig.dspNumRequested;

    err = mpLayout->SyncLayoutByAction(ACTION_BACK, -1);
    mpMdecInfo->isNvr = AM_TRUE;
    mbOnNVR = AM_TRUE;
    return err;
}

AM_ERR CActiveMDecEngine::HandleSwitchNoHD(AM_INT winIndex)
{
    //just swith the window to full screen
    AM_ERR err;
    AM_INT index, aIndex;
    AM_INT sourceGroup;
    AM_INT dspNum = mpConfig->dspConfig.dspNumRequested;

    if(winIndex >= dspNum){
        AM_ERROR("Wrong winIndex:%d.\n", winIndex);
        return ME_BAD_COMMAND;
    }

    sourceGroup = mpLayout->GetSourceGroupByWindow(winIndex);
    err = mpLayout->SyncLayoutByAction(ACTION_SWITCH, sourceGroup);

    if(mpLayout->GetLayoutType() != LAYOUT_TYPE_TELECONFERENCE){
        mpMdecInfo->isNvr = AM_FALSE;
        mbOnNVR = AM_FALSE;
    }

    aIndex = mpConfig->GetAudioSource();
    index = InfoChangeWinIndex(mpLayout->GetSourceGroupByWindow(0));

    if(aIndex != index){
        AM_INFO("mNvrAudioIndex %d, winIndex %d.\n", aIndex, index);
        DoRemoveAudioStream(aIndex);
        DoStartSpecifyAudio(index);
        mNvrAudioIndex = index;
    }

    return err;
}

AM_ERR CActiveMDecEngine::HandleBackByArm()
{
    AM_INFO("HandleBackByArm.\n");
    if(mbOnNVR == AM_TRUE || mSwitchWait != SWITCH_WAIT_NOON  || mAudioWait == SWITCH_AUDIO_WAIT){
        AM_ERROR("Already on Nvr %d %d %d !\n", mbOnNVR, mSwitchWait, mAudioWait);
        return ME_BUSY;
    }

//    CDemuxerConfig* curConfig = NULL;
    //AM_ASSERT(mNvrAudioIndex < 0);

    mSwitchWait = BACK_HD_SD_WAIT;

    InfoHideAllSmall(AM_FALSE);
    mpDemuxer->Config(CMD_EACH_COMPONENT);
    DoResumeSmallDspMw(SWITCH_ACTION);

    return ME_OK;
}

/*
AM_ERR CActiveMDecEngine::HandleBackByDsp(AM_BOOL seamless)
{
    AM_INFO("HandleBackByDsp(%d).\n", seamless);
    if(mbOnNVR == AM_TRUE || mAudioWait == BACK_AUDIO_WAIT){
        AM_ERROR("Already on Nvr!\n");
        return ME_BUSY;
    }

    CDemuxerConfig* curConfig = NULL;
    AM_ERR err;
    CParam64 parVec(3);
    AM_S64 targetTime;

    //AM_ASSERT(mNvrAudioIndex < 0);
    //switch hd to cur sd
    err = mpVideoDecoder->QueryInfo(DEC_MAP(mHdInfoIndex), INFO_TIME_PTS, parVec);
    if(err != ME_OK){
        AM_ASSERT(0);
        parVec[0] = parVec[1] = 0;
    }
    targetTime = parVec[1];
    if(parVec[1] == -1)
        targetTime = parVec[0] - 2102100;
    AM_INFO("Info Dump for Stream %d:Feed Pts:%lld, Render Pts:%lld, TargetTime:%lld\n", DEC_MAP(mHdInfoIndex), parVec[0], parVec[1], targetTime);
    mpDemuxer->SeekTo(mSdInfoIndex, targetTime, SEEK_BY_SWITCH);
    curConfig = &(mpConfig->demuxerConfig[mSdInfoIndex]);
    curConfig->hided = AM_FALSE;
    mpDemuxer->Config(mSdInfoIndex);
    Dump();
    //switch
    AM_INFO("Switch to Dsp, Wait!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    mpConfig->generalCmd |= RENDEER_SPECIFY_CONFIG;
    mpConfig->specifyIndex = REN_MAP(mSdInfoIndex); //switch this udec instance
    if(seamless == AM_TRUE)
        mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = RENDER_SWITCH_BACK;
    else
        mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = RENDER_SWITCH_BACK2;

    mpRenderer->Config(REN_MAP(mSdInfoIndex));
    mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = 0;
    mpConfig->generalCmd &= ~RENDEER_SPECIFY_CONFIG;
    AM_INFO("Well! Handle Audio And SD.\n");

    DoSwitchBackAudioLocal();

    DoResumeSmallDspMw(SWITCH_ACTION);
    mpAudioSinkFilter->Resume(mNvrAudioIndex);
    InfoHideAllSmall(AM_FALSE);
    curConfig = &(mpConfig->demuxerConfig[mHdInfoIndex]);
    curConfig->hided = AM_TRUE;
    mpDemuxer->Config(CMD_EACH_COMPONENT);

    if(mpMdecInfo->unitInfo[mHdInfoIndex].isPaused == AM_TRUE){
        AM_INFO("Paused will be resumed by switch back for Hd %d.\n", mHdInfoIndex);
        DoFlushSpecifyDspMw(mHdInfoIndex);
    }else{
        mpVideoDecoder->Pause(DEC_MAP(mHdInfoIndex));
        DoFlushSpecifyDspMw(mHdInfoIndex);
        mpVideoDecoder->Resume(DEC_MAP(mHdInfoIndex));
    }

    mpMdecInfo->isNvr = AM_TRUE;
    MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[mHdInfoIndex]);
    curInfo->isPaused = AM_TRUE;
    //curInfo->playStream = TYPE_VIDEO;

    mHdInfoIndex = -1;//just a check
    //mbOnNVR = AM_TRUE;
    mAudioWait = BACK_AUDIO_WAIT;
    UpdateNvrOsdInfo();
    return ME_OK;
}
*/

AM_ERR CActiveMDecEngine::HandleSwitchBack()
{
    //seek four sd to same timestamp
    AM_INFO("HandleSwitchBack(%d).\n", mHdInfoIndex);
    if(mbOnNVR == AM_TRUE || mAudioWait == BACK_AUDIO_WAIT){
        AM_ERROR("Already on Nvr!\n");
        if(!(mpConfig->globalFlag & NOTHING_DONOTHING))
            return ME_BUSY;
    }

    CDemuxerConfig* curConfig = NULL;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    AM_ERR err = ME_OK;

#if 0
//us seemless switch api
    AM_BOOL seamless = DoDetectSeamless(mSdInfoIndex, mHdInfoIndex);

    //AM_ASSERT(mNvrAudioIndex < 0);
    //switch hd to cur sd
    //Dump();
    DoDspSwitchForSource(mHdInfoIndex, mSdInfoIndex, seamless);
#else
//start SD streams and stop HD stream directly
    curInfo = &(mpMdecInfo->unitInfo[mSdInfoIndex]);
    if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE && curInfo->isNet == AM_FALSE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
        CParam64 parVec(3);
        AM_S64 targetTime;
        err = mpVideoDecoder->QueryInfo(DEC_MAP(mHdInfoIndex), INFO_TIME_PTS, parVec);
        if(err != ME_OK){
            AM_ASSERT(0);
            parVec[0] = parVec[1] = 0;
        }
        targetTime = parVec[1];
        if(parVec[1] == -1)
            targetTime = parVec[0] - 2102100;
        if(targetTime < targetTime)//sd still has not frame out
            targetTime = targetTime;

        mpDemuxer->SeekTo(mSdInfoIndex, targetTime, SEEK_BY_SWITCH);
        curConfig = &(mpConfig->demuxerConfig[mSdInfoIndex]);
        curConfig->hided = AM_FALSE;
        mpDemuxer->Config(mSdInfoIndex);
    }
#endif
    DoSyncSDStreamBySource(mHdInfoIndex);
    mpLayout->SyncLayoutByAction(ACTION_BACK, -1);

    DoRemoveAudioStream(mHdInfoIndex);
    mNvrAudioIndex = mSdInfoIndex;
    DoStartSpecifyAudio(mNvrAudioIndex);
    //mpRenderer->Resume(mNvrAudioIndex);
    DoResumeSmallDspMw(SWITCH_ACTION);
    //mpAudioSinkFilter->Resume(mNvrAudioIndex);
    curConfig = &(mpConfig->demuxerConfig[mHdInfoIndex]);
    curConfig->hided = AM_TRUE;
    mpDemuxer->Config(CMD_EACH_COMPONENT);

    if(mpMdecInfo->unitInfo[mHdInfoIndex].isPaused == AM_TRUE){
        AM_INFO("Paused will be resumed by switch back for Hd %d.\n", mHdInfoIndex);
        DoFlushSpecifyDspMw(mHdInfoIndex, AM_TRUE);
    }else{
        mpVideoDecoder->Pause(DEC_MAP(mHdInfoIndex));
        DoFlushSpecifyDspMw(mHdInfoIndex, AM_TRUE);
        mpVideoDecoder->Resume(DEC_MAP(mHdInfoIndex));
    }

    mpMdecInfo->isNvr = AM_TRUE;
    curInfo = &(mpMdecInfo->unitInfo[mHdInfoIndex]);
    curInfo->isPaused = AM_TRUE;
    //curInfo->playStream = TYPE_VIDEO;

    mHdInfoIndex = -1;//just a check
    mbOnNVR = AM_TRUE;
    if(curConfig->hasAudio != AM_FALSE && !(mpConfig->globalFlag & NO_AUDIO_ON_GMF))
        mAudioWait = BACK_AUDIO_WAIT;
    UpdateNvrOsdInfo();
    return err;
}

#define enable_pre_switch
AM_ERR CActiveMDecEngine::HandleSwitchStream(AM_INT winIndex)
{
    AM_INFO("HandleSwitchStream(%d), %d, %d.\n", winIndex, mAudioWait, mSwitchWait);
    if(mAudioWait != AUDIO_WAIT_NOON || mSwitchWait != SWITCH_WAIT_NOON){
        AM_ERROR("Still processing for previous action! Be patient plz(%d,%d)...\n",
            mAudioWait, mSwitchWait);
        //AM_ERROR("Audio is still be processed for previous switch action! Be patient plz...\n");
        //AM_ERROR("Todo Me, Support switch to other HD when palyback a HD.\n");
        if(!(mpConfig->globalFlag & NOTHING_DONOTHING))
            return ME_BUSY;
    }
    //CDemuxerConfig* curConfig = NULL;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    AM_INT i, sIndex, hdIndex, sourceGroup;
    AM_ERR err = ME_OK;
    AM_BOOL seamless;

    if((mpLayout->GetLayoutType() == LAYOUT_TYPE_TELECONFERENCE) || (mpLayout->GetLayoutType() == LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN)){
        if(winIndex == 0){
            AM_ERROR("HandleSwitchStream: current layout: %d, window 0 is HD already!!\n", mpLayout->GetLayoutType());
            return ME_OK;
        }
    }
    AM_INT aIndex = mpConfig->GetAudioSource();
    AM_BOOL bSwitchAudioFlag = AM_TRUE;
    AM_ASSERT(aIndex >= 0);
    //AM_INFO("Auto Switch Info: sIndex:%d, hdIndex:%d, Audio:%d\n", sIndex, hdIndex, mNvrAudioIndex);
    //if(mpMdecInfo->unitInfo[sIndex].isPaused == AM_TRUE){
        //AM_INFO("Paused will be resumed by switch for Nvr Windows %d.\n", sIndex);
    //}

    AM_BOOL hasPreSwith = AM_FALSE;
#ifdef enable_pre_switch
    do{
        if(DoFindStreamIndex(winIndex, sIndex, hdIndex) != ME_OK)
            break;
        curInfo = &(mpMdecInfo->unitInfo[hdIndex]);
        if(curInfo->isNet == AM_FALSE) break;

        if((mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE) || (mpLayout->GetLayoutType() == LAYOUT_TYPE_TELECONFERENCE)){
            sourceGroup = mpLayout->GetSourceGroupByWindow(winIndex);
            mpLayout->SyncLayoutByAction(ACTION_PRE_SWITCH, sourceGroup);
            hasPreSwith = AM_TRUE;
            AM_INFO("mpLayout->SyncLayoutByAction(ACTION_PRE_SWITCH, %d) done.\n", sourceGroup);
        }
    }while(0);
#endif
    if((mpLayout->GetLayoutType() == LAYOUT_TYPE_TELECONFERENCE) || (mpLayout->GetLayoutType() == LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN)){
        AM_INT focusSourceGroup = mpLayout->GetSourceGroupByWindow(hasPreSwith?winIndex:0);
        if(DoFindStreamIndex(focusSourceGroup, sIndex, hdIndex) != ME_OK)
            return ME_BAD_STATE;
        if((aIndex == hdIndex)){
            //we always keep mNvrAudioIndex on this layout as the current audio source
            DoRemoveAudioStream(aIndex);
            DoStartSpecifyAudio(sIndex);
            mNvrAudioIndex = sIndex;
        }else{
            bSwitchAudioFlag = AM_FALSE;
        }

        seamless = DoDetectSeamless(sIndex, hdIndex);
        DoDspSwitchForSource(hdIndex, sIndex, seamless);
        InactiveStream(hdIndex);
        ActiveStream(sIndex, bSwitchAudioFlag==AM_TRUE);

        sourceGroup = mpLayout->GetSourceGroupByWindow(hasPreSwith?0:winIndex);
    }else{
        sourceGroup = mpLayout->GetSourceGroupByWindow(winIndex);
    }

    if(DoFindStreamIndex(sourceGroup, sIndex, hdIndex) != ME_OK)
        return ME_BAD_STATE;

    seamless = DoDetectSeamless(sIndex, hdIndex);


    //Switch then pause.
    DoDspSwitchForSource(sIndex, hdIndex, seamless);
    InactiveStream(sIndex);

    mpLayout->SyncLayoutByAction(ACTION_SWITCH, sourceGroup);

    if(mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE){
        DoPauseSmallDspMw(SWITCH_ACTION);
        InfoHideAllSmall(AM_TRUE);
        mpDemuxer->Config(CMD_EACH_COMPONENT);

        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE
                && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup])// && curInfo->isNet == AM_TRUE)
                DoFlushSpecifyDspMw(i);
        }
        mNvrAudioIndex = mpConfig->GetAudioSource();
        DoRemoveAudioStream(mNvrAudioIndex);
        DoStartSpecifyAudio(hdIndex);
        mNvrAudioIndex = sIndex;
        ActiveStream(hdIndex);
    }else if((mpLayout->GetLayoutType() == LAYOUT_TYPE_TELECONFERENCE) || (mpLayout->GetLayoutType() == LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN)){
        //InactiveStream(sIndex);
         if((aIndex == sIndex)){
            //we always keep mNvrAudioIndex on this layout as the current audio source
            DoRemoveAudioStream(aIndex);
            DoStartSpecifyAudio(hdIndex);
            mNvrAudioIndex = sIndex;
        }else{
            bSwitchAudioFlag = AM_FALSE;
        }
        ActiveStream(hdIndex, bSwitchAudioFlag==AM_TRUE);
    }else{
        AM_ASSERT(0);
    }
    //DoSwitchAudioLocal(sIndex, hdIndex);
    //DoFlushSpecifyDspMw(sIndex);
    if( (mpLayout->GetLayoutType() != LAYOUT_TYPE_TELECONFERENCE) && (mpLayout->GetLayoutType() != LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN)){
        mpMdecInfo->isNvr = AM_FALSE;
        mbOnNVR = AM_FALSE;
    }else{
        mSwitchWait = SWITCH_TELE_WAIT1;
    }

    curInfo = &(mpMdecInfo->unitInfo[hdIndex]);
    curInfo->isPaused = AM_FALSE;
    curInfo->playStream |= TYPE_VIDEO;
    //audio can be added in DoStartSpecifyAudio
    //curInfo->playStream |= TYPE_AUDIO;

    mHdInfoIndex = hdIndex;
    mSdInfoIndex = sIndex;
    //mbOnNVR = AM_FALSE;
    if(0 == (mpConfig->globalFlag & NO_AUDIO_ON_GMF)) {
        if(/*(mAudioWait == AUDIO_WAIT_NOON) || */mpConfig->GetAudioSource() < 0 || mNvrAudioIndex < 0)
            mAudioWait = AUDIO_WAIT_NOON;
        else if(bSwitchAudioFlag == AM_FALSE)
            mAudioWait = AUDIO_WAIT_NOON;
        else
            mAudioWait = SWITCH_AUDIO_WAIT;
    }
    UpdateNvrOsdInfo();
    return err;
}

/*
AM_ERR CActiveMDecEngine::HandleSwitchForNet(AM_INT winIndex, AM_BOOL seamless)
{
    AM_INFO("HandleSwitchForNet(seamless:%d).\n", seamless);
    if(mAudioWait != AUDIO_WAIT_NOON){
        AM_ERROR("Audio is still be processed for previous switch action! Be patient plz...\n");
        //AM_ERROR("Todo Me, Support switch to other HD when palyback a HD.\n");
        return ME_BUSY;
    }

    CDemuxerConfig* curConfig = NULL;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    AM_INT i, sIndex, hdIndex;
    AM_ERR err;

    if(DoFindStreamIndex(winIndex, sIndex, hdIndex) != ME_OK)
        return ME_BAD_STATE;

    mNvrAudioIndex = mpConfig->GetAudioSource();
    AM_ASSERT(mNvrAudioIndex >= 0);
    AM_INFO("Auto Switch Info: sIndex:%d, hdIndex:%d, Audio:%d\n", sIndex, hdIndex, mNvrAudioIndex);
    if(mpMdecInfo->unitInfo[sIndex].isPaused == AM_TRUE){
        AM_INFO("Paused will be resumed by switch for Nvr Windows %d.\n", sIndex);
    }

    //Switch then pause.
    DoDspSwitchForSource(sIndex, hdIndex, seamless);

    //process 4sd, cur sd +3sd
    DoPauseSmallDspMw(SWITCH_ACTION);
    InfoHideAllSmall(AM_TRUE); //will do clearqueue
    mpDemuxer->Config(CMD_EACH_COMPONENT);

    DoSwitchAudioNet(sIndex, hdIndex);
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup])
            DoFlushSpecifyDspMw(i);
    }

    mpMdecInfo->isNvr = AM_FALSE;
    curInfo = &(mpMdecInfo->unitInfo[hdIndex]);
    curInfo->isPaused = AM_FALSE;
    //curInfo->playStream = TYPE_VIDEO;

    mHdInfoIndex = hdIndex;
    mSdInfoIndex = sIndex;
    //mbOnNVR = AM_FALSE;
    mAudioWait = SWITCH_AUDIO_WAIT;
    UpdateNvrOsdInfo();
    return ME_OK;
}
*/

AM_ERR CActiveMDecEngine::HandleSwitchDuringHd(AM_INT winIndex)
{
    AM_INFO("HandleSwitchDuringHd(%d).\n", winIndex);
    if(mAudioWait != AUDIO_WAIT_NOON || mSwitchWait != SWITCH_WAIT_NOON){
        AM_ERROR("Still processing for previous action! Be patient plz(%d,%d)...\n",
            mAudioWait, mSwitchWait);
        return ME_BUSY;
    }

    AM_INT hdIndex, sIndex;
    AM_ERR err = ME_OK;

    if(DoFindStreamIndex(winIndex, sIndex, hdIndex) != ME_OK)
        return ME_BAD_STATE;

    AM_ASSERT(mHdInfoIndex >= 0);
    if(mHdInfoIndex == hdIndex)
        return ME_OK;
    AM_INFO("Support switch to different hd source:%d, current:%d.\n", hdIndex, mHdInfoIndex);

    CDemuxerConfig* curConfig = NULL, *showConfig = NULL;
    MdecInfo::MdecUnitInfo* curInfo = NULL, *showInfo = NULL;
    curConfig = &(mpConfig->demuxerConfig[mHdInfoIndex]);
    showConfig = &(mpConfig->demuxerConfig[hdIndex]);
    curInfo = &(mpMdecInfo->unitInfo[mHdInfoIndex]);
    showInfo = &(mpMdecInfo->unitInfo[hdIndex]);

    DoRemoveAudioStream(mHdInfoIndex);
    mpVideoDecoder->Pause(DEC_MAP(mHdInfoIndex));
    curConfig->hided = AM_TRUE;
    mpDemuxer->Config(mHdInfoIndex);
    DoFlushSpecifyDspMw(mHdInfoIndex, AM_TRUE);
    showConfig->hided = AM_FALSE;
    mpDemuxer->Config(hdIndex);
    mpVideoDecoder->Resume(DEC_MAP(mHdInfoIndex));
    DoStartSpecifyAudio(hdIndex, 0);

    curInfo->playStream &= ~(TYPE_AUDIO | TYPE_VIDEO);
    showInfo->playStream |= TYPE_VIDEO;
    showInfo->playStream |= TYPE_AUDIO;
    mHdInfoIndex = hdIndex;
    mNvrAudioIndex = mSdInfoIndex = sIndex;
    if(curConfig->hasAudio != AM_FALSE && !(mpConfig->globalFlag & NO_AUDIO_ON_GMF))
        mAudioWait = SWITCH_AUDIO_WAIT;
    UpdateNvrOsdInfo();
    return err;
}

AM_ERR CActiveMDecEngine::HandleSwitchSet(AM_INT setIndex)
{
    if(mbOnNVR == AM_FALSE){
        AM_INFO("Switch Set Only Permitted on Multi Windows!\n");
        return ME_CLOSED;
    }

    AM_ERR err = ME_OK;
    AM_INT curSet = 0;
    AM_INT sIndex = InfoChangeWinIndex(0);
    AM_INT setSize = (mbNohd == AM_TRUE) ? mUdecNum : ((mUdecNum - 1) * 2);
    curSet = sIndex /setSize ;
    if(setIndex == -1){
        AM_INT bIndex, fIndex, loop = 1;
        bIndex = (curSet + loop) * setSize;
        fIndex = (curSet - loop) * setSize;
        while(bIndex < MDEC_SOURCE_MAX_NUM || fIndex >= 0)
        {
            if(bIndex < MDEC_SOURCE_MAX_NUM && mpMdecInfo->unitInfo[bIndex].isUsed == AM_TRUE){
                setIndex = curSet + loop;
                break;
            }else if(fIndex >= 0 && mpMdecInfo->unitInfo[fIndex].isUsed == AM_TRUE){
                setIndex = curSet - loop;
                break;
            }
            loop++;
            bIndex = (curSet + loop) * setSize;
            fIndex = (curSet - loop) * setSize;
        }
        if(bIndex >= MDEC_SOURCE_MAX_NUM && fIndex < 0){
            AM_INFO("Donot find a adjacent Set for curSet:%d\n", curSet);
            return ME_CLOSED;
        }
    }
    AM_INFO("HandleSwitchSet(%d->%d).\n", curSet, setIndex);
    if(curSet == setIndex){
        AM_INFO("Already on Set %d.\n", setIndex);
        return ME_OK;
    }

    //switch
    CDemuxerConfig* curConfig = NULL;
    MdecInfo::MdecUnitInfo* oldInfo = NULL, *showInfo = NULL;
    AM_INT sd = sIndex, targetSd = setIndex * setSize, addOr;

    AM_INT aIndex = mpConfig->GetAudioSource();
    AM_INT sdSetSize = (mbNohd == AM_TRUE) ? setSize : setSize /2;
    DoRemoveAudioStream(aIndex);
    for(; sd < sIndex + sdSetSize; sd++, targetSd++){
        curConfig = &(mpConfig->demuxerConfig[sd]);
        mpVideoDecoder->Pause(DEC_MAP(sd));
        curConfig->hided = AM_TRUE;
        mpDemuxer->Config(sd);
        DoFlushSpecifyDspMw(sd);
        mpVideoDecoder->Resume(DEC_MAP(sd));

        //show new
        curConfig = &(mpConfig->demuxerConfig[targetSd]);
        showInfo = &(mpMdecInfo->unitInfo[targetSd]);
        curConfig->sendHandleV = AM_TRUE;
        curConfig->hided = AM_FALSE;
        mpDemuxer->Config(targetSd);
        //assign addOrder
        oldInfo = &(mpMdecInfo->unitInfo[sd]);
        showInfo = &(mpMdecInfo->unitInfo[targetSd]);
        addOr = showInfo->addOrder;
        showInfo->addOrder = oldInfo->addOrder;
        oldInfo->addOrder = addOr;
        if(mbNohd == AM_FALSE){
            oldInfo = &(mpMdecInfo->unitInfo[sd + sdSetSize]);
            showInfo = &(mpMdecInfo->unitInfo[targetSd + sdSetSize]);
            addOr = showInfo->addOrder;
            showInfo->addOrder = oldInfo->addOrder;
            oldInfo->addOrder = addOr;
        }
    }
    DoStartSpecifyAudio(setIndex * setSize, 0);

    //mHdInfoIndex = hdIndex;
    mNvrAudioIndex = mSdInfoIndex = setIndex * setSize;
    //mAudioWait = SWITCH_AUDIO_WAIT;
    return err;
}
//------------------------------------------------
//
//Layout Handlers
//
//-------------------------------------------------
AM_ERR CActiveMDecEngine::HandleReLayout(LAYOUT_TYPE layout, AM_INT index)
{
    AM_ERR err = ME_OK;
    LAYOUT_TYPE oldType = mpLayout->GetLayoutType();
    if (oldType == layout && 0) {
        AM_INFO("HandleReLayout: layout has been %d\n", layout);
        return ME_OK;
    }
    if(mbOnNVR == AM_FALSE && layout == LAYOUT_TYPE_TELECONFERENCE){
        AM_INFO("Future stage: support relayout during HD view!\n");
        if(!(mpConfig->globalFlag & NOTHING_DONOTHING))
            return ME_BUSY;
    }

    if(mbNohd){
        return HandleReLayoutNoHD(layout, index);
    }

    if((mUdecNum > 7)&& !mbNohd){
    //if there are more than 6 480P streams,
    //the performance is not enough to decode all SD streams and 1 HD at the same time
        if((layout==LAYOUT_TYPE_TELECONFERENCE) ||(layout==LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN)){
            AM_ERROR("Can not relayout to TELE-CONFERENCE or HIGHLIGHT mode, performance limited.\n");
            return ME_NOT_SUPPORTED;
        }
    }

    switch (layout) {
        case LAYOUT_TYPE_TABLE:
            err = HandleLayoutTable(layout);
            break;
        case LAYOUT_TYPE_TELECONFERENCE:
            err = HandleLayoutTeleconference(layout, index);
            break;
        case LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN:
            err = HandleLayoutBottomleftHighlightenMode(layout, index);
            break;
        case LAYOUT_TYPE_SINGLE_HD_FULLSCREEN:
            err = HandleLayoutSingleHDFullScreen(layout, index);
            break;
        case LAYOUT_TYPE_RECOVER:
            HandleReLayout(oldType);
            break;
        default:
            err = ME_NO_IMPL;
            AM_WARNING("HandleReLayout: no implementation for layout:%d\n", layout);
            break;
    }

    if (err != ME_OK) {
        AM_ERROR("HandleReLayout: failure for layout:%d\n", layout);
        return err;
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleLayoutTeleconference(LAYOUT_TYPE type, AM_INT index)
{
    /*
     *  Currently, two cases exist:
     *  1. 1HD Fullscreen + 3SD Subscreen
     *  2. 1HD Fullscreen + 5SD Subscreen
     *
     *  Steps:
     *  0.  check status for LayoutTeleconference
     *  1.  active Streams which to show
     *  2.  ReconfigWindowRenderLayoutTeleconference()
     *  3.  SwitchAudio which to focus
     *  4.  inactive Streams which to hid
     *  5.  update status for LayoutTeleconference
     *
     */
    AM_INFO("HandleLayoutTeleconference, type:%d %d.\n", type, index);
    if (mAudioWait != AUDIO_WAIT_NOON) {
        AM_ERROR("HandleLayoutTeleconference: Audio Module is busy now\n");
        if(!(mpConfig->globalFlag & NOTHING_DONOTHING))
            return ME_BUSY;
    }

    AM_ERR err;
    AM_INT srcStreamIndex=0, focusWinIndex=0, destStreamIndex=0;
    LAYOUT_TYPE layout = mpLayout->GetLayoutType();
    if(layout == LAYOUT_TYPE_TABLE){
        // suppose,
        // maybe parameter or member variable someday
        focusWinIndex = mpLayout->GetSourceGroupByWindow(index);

        srcStreamIndex = InfoSDByWindow(focusWinIndex);
        if (srcStreamIndex < 0) {
            AM_ERROR("HandleLayoutTeleconference: InfoSDByWindow failure, focusWinIndex=%d srcStreamIndex=%d\n", focusWinIndex, srcStreamIndex);
            return ME_BAD_PARAM;
        }

        destStreamIndex = InfoHDBySD(srcStreamIndex);
        if (destStreamIndex < 0) {
            AM_ERROR("HandleLayoutTeleconference: InfoFindHDIndex failure, focusWinIndex:%d srcStreamIndex:%d destStreamIndex:%d\n", focusWinIndex, srcStreamIndex, destStreamIndex);
            return ME_BAD_PARAM;
        }
        AM_INFO("srcStreamIndex:%d destStreamIndex:%d\n", srcStreamIndex, destStreamIndex);
        AM_ASSERT(mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE);
        AM_ASSERT(mbOnNVR == AM_TRUE);


        err = mpLayout->SetPreLayoutType(LAYOUT_TYPE_PRE_TELECONFERENCE, index);
        mpLayout->UpdateLayout();

        // todo: should consider
        // case 1: 1 HD -> HD + SD later;
        //         ActiveStream hd & sd first
//        AM_INT i, sIndex;
        //err = ActiveStream(destStreamIndex);
        err = DoDspSwitchForSource(srcStreamIndex, destStreamIndex, AM_FALSE);
        if (err != ME_OK) {
            AM_ERROR("HandleLayoutTeleconference: ActiveStream failure, err:%d focusWinIndex:%d srcStreamIndex:%d destStreamIndex:%d\n", err, focusWinIndex, srcStreamIndex, destStreamIndex);
            return ME_ERROR;
        }
        mHdInfoIndex = destStreamIndex;

        if(srcStreamIndex==mpConfig->GetAudioSource()){
            err = SwitchAudio(mpConfig->GetAudioSource(), destStreamIndex);
            if (err != ME_OK) {
                AM_ERROR("HandleLayoutTeleconference: SwitchAudio failure, err:%d focusWinIndex:%d srcStreamIndex:%d destStreamIndex:%d\n", err, focusWinIndex, srcStreamIndex, destStreamIndex);
                return ME_ERROR;
            }
            mNvrAudioIndex = srcStreamIndex;
        }else{
            mAudioWait = AUDIO_WAIT_DONE;
        }
    }

    err = mpLayout->SetLayoutType(type, index); //only window config done
    mpLayout->UpdateLayout();

    if(layout == LAYOUT_TYPE_TABLE){
        err = InactiveStream(srcStreamIndex);
        if (err != ME_OK) {
            AM_ERROR("HandleLayoutTeleconference: InactiveStream failure, err:%d focusWinIndex:%d srcStreamIndex:%d destStreamIndex:%d\n", err, focusWinIndex, srcStreamIndex, destStreamIndex);
            return ME_ERROR;
        }
        mSdInfoIndex = srcStreamIndex;
    }
    if(0 != (mpConfig->globalFlag & NO_AUDIO_ON_GMF)) {
        mAudioWait = AUDIO_WAIT_DONE;
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleLayoutTable(LAYOUT_TYPE type)
{
    /*
     *  Currently, two cases exist:
     *  1. 4SD Subscreen
     *  2. 6SD Subscreen
     *
     *  Steps:
     *  0.  check status for LayoutTable
     *  1.  active Streams which to show
     *  2.  ReconfigWindowRenderLayoutTable()
     *  3.  SwitchAudio which to focus
     *  4.  inactive Streams which to hid
     *  5.  update status for LayoutTable
     *
     */
    AM_ERR err;

    if (mAudioWait != AUDIO_WAIT_NOON) {
        AM_ERROR("HandleLayoutTeleconference: Audio Module is busy now\n");
        if(!(mpConfig->globalFlag & NOTHING_DONOTHING))
            return ME_BUSY;
    }

    AM_ASSERT((mpLayout->GetLayoutType() == LAYOUT_TYPE_TELECONFERENCE) ||
        (mpLayout->GetLayoutType() == LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN));
    //AM_ASSERT(mbOnNVR == AM_TRUE);

    // suppose,
    // maybe parameter or member variable someday
    AM_INT focusWinIndex = mpLayout->GetSourceGroupByWindow(0);

    AM_INT srcStreamIndex = InfoHDByWindow(focusWinIndex);
    if (srcStreamIndex < 0) {
        AM_ERROR("HandleLayoutTable: InfoHDByWindow failure, focusWinIndex=%d srcStreamIndex=%d\n", focusWinIndex, srcStreamIndex);
        return ME_BAD_PARAM;
    }

    AM_INT destStreamIndex = InfoSDByHD(srcStreamIndex);
    if (destStreamIndex < 0) {
        AM_ERROR("HandleLayoutTeleconference: InfoSDByHD failure, focusWinIndex:%d destStreamIndex:%d srcStreamIndex:%d\n", focusWinIndex, destStreamIndex, srcStreamIndex);
        return ME_BAD_PARAM;
    }

    err = mpLayout->SetPreLayoutType(LAYOUT_TYPE_PRE_TABLE, 0);
    mpLayout->UpdateLayout();

    AM_INT aIndex = mpConfig->GetAudioSource();

    // todo: consider different layout switch
    // AM_INT i, sIndex;
    err = ActiveStream(destStreamIndex, (aIndex == srcStreamIndex));
    if (err != ME_OK) {
        AM_ERROR("HandleLayoutTeleconference: ActiveStream failure, err:%d focusWinIndex:%d destStreamIndex:%d srcStreamIndex:%d\n", err, focusWinIndex, destStreamIndex, srcStreamIndex);
        return ME_ERROR;
    }

    if(aIndex == srcStreamIndex){
        err = SwitchAudio(srcStreamIndex, destStreamIndex);
        if (err != ME_OK) {
            AM_ERROR("HandleLayoutTeleconference: SwitchAudio failure, err:%d focusWinIndex:%d destStreamIndex:%d srcStreamIndex:%d\n", err, focusWinIndex, destStreamIndex, srcStreamIndex);
            return ME_ERROR;
        }
        //mNvrAudioIndex has no change
    }else{
        mAudioWait = AUDIO_WAIT_DONE;
    }

    err = mpLayout->SetLayoutType(type); //only window config done
    mpLayout->UpdateLayout();

    err = InactiveStream(srcStreamIndex);
    if (err != ME_OK) {
        AM_ERROR("HandleLayoutTeleconference: InactiveStream failure, err:%d focusWinIndex:%d destStreamIndex:%d srcStreamIndex:%d\n", err, focusWinIndex, destStreamIndex, srcStreamIndex);
        return ME_ERROR;
    }

    mpMdecInfo->isNvr = AM_TRUE;
    mbOnNVR = AM_TRUE;
    mHdInfoIndex = -1;
    if(0 != (mpConfig->globalFlag & NO_AUDIO_ON_GMF)) {
        mAudioWait = AUDIO_WAIT_DONE;
    }
    UpdateNvrOsdInfo();

    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleLayoutBottomleftHighlightenMode(LAYOUT_TYPE type, AM_INT index)
{
    AM_INFO("HandleLayoutBottomleftHighlightenMode, type:%d.\n", type);
    if (mAudioWait != AUDIO_WAIT_NOON) {
        AM_ERROR("HandleLayoutBottomleftHighlightenMode: Audio Module is busy now\n");
        if(!(mpConfig->globalFlag & NOTHING_DONOTHING))
            return ME_BUSY;
    }

    AM_ERR err;
    AM_INT srcStreamIndex=0, focusWinIndex=0, destStreamIndex=0;
    AM_INT aIndex = mpConfig->GetAudioSource();
    if(mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE){
        // suppose,
        // maybe parameter or member variable someday
        focusWinIndex = mpLayout->GetSourceGroupByWindow(0);

        srcStreamIndex = InfoSDByWindow(focusWinIndex);
        if (srcStreamIndex < 0) {
            AM_ERROR("HandleLayoutBottomleftHighlightenMode: InfoSDByWindow failure, focusWinIndex=%d srcStreamIndex=%d\n", focusWinIndex, srcStreamIndex);
            return ME_BAD_PARAM;
        }

        destStreamIndex = InfoHDBySD(srcStreamIndex);
        if (destStreamIndex < 0) {
            AM_ERROR("HandleLayoutBottomleftHighlightenMode: InfoFindHDIndex failure, focusWinIndex:%d srcStreamIndex:%d destStreamIndex:%d\n", focusWinIndex, srcStreamIndex, destStreamIndex);
            return ME_BAD_PARAM;
        }
        AM_INFO("srcStreamIndex:%d destStreamIndex:%d\n", srcStreamIndex, destStreamIndex);
        AM_ASSERT(mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE);
        AM_ASSERT(mbOnNVR == AM_TRUE);

        // todo: should consider
        // case 1: 1 HD -> HD + SD later;
        //         ActiveStream hd & sd first
//        AM_INT i, sIndex;
        //err = ActiveStream(destStreamIndex);
        err = DoDspSwitchForSource(srcStreamIndex, destStreamIndex, AM_FALSE);
        if (err != ME_OK) {
            AM_ERROR("HandleLayoutBottomleftHighlightenMode: ActiveStream failure, err:%d focusWinIndex:%d srcStreamIndex:%d destStreamIndex:%d\n", err, focusWinIndex, srcStreamIndex, destStreamIndex);
            return ME_ERROR;
        }
        mHdInfoIndex = destStreamIndex;
        if(srcStreamIndex==aIndex){
            err = SwitchAudio(aIndex, destStreamIndex);
            if (err != ME_OK) {
                AM_ERROR("HandleLayoutBottomleftHighlightenMode: SwitchAudio failure, err:%d focusWinIndex:%d srcStreamIndex:%d destStreamIndex:%d\n", err, focusWinIndex, srcStreamIndex, destStreamIndex);
                return ME_ERROR;
            }
            //mNvrAudioIndex has no change
        }else{
            mAudioWait = AUDIO_WAIT_DONE;
        }
    }

    if((0 != (mpConfig->globalFlag & NO_AUDIO_ON_GMF)) || (aIndex != srcStreamIndex)) {
        mAudioWait = AUDIO_WAIT_DONE;
    }
    err = mpLayout->SetLayoutType(type); //only window config done
    mpLayout->UpdateLayout();

    if(mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE){
        err = InactiveStream(srcStreamIndex);
        if (err != ME_OK) {
        AM_ERROR("HandleLayoutBottomleftHighlightenMode: InactiveStream failure, err:%d focusWinIndex:%d srcStreamIndex:%d destStreamIndex:%d\n", err, focusWinIndex, srcStreamIndex, destStreamIndex);
            return ME_ERROR;
        }
        mSdInfoIndex = srcStreamIndex;
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleLayoutSingleHDFullScreen(LAYOUT_TYPE type, AM_INT index)
{
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleReLayoutNoHD(LAYOUT_TYPE layout, AM_INT index)
{
    //todo: merge this api into HandleReLayoutNoHD
    AM_INFO("HandleReLayoutNoHD: change to: %d\n", layout);
//    AM_INT i, sIndex;
    AM_ERR err = ME_OK;
    if(layout == LAYOUT_TYPE_TABLE){
        err = mpLayout->SetLayoutType(layout); //only window config done
        mpLayout->UpdateLayout();
    }else if(layout == LAYOUT_TYPE_TELECONFERENCE){
        // AM_INT aIndex = mpConfig->GetAudioSource();
        // AM_INT index = InfoSDByWindow(0);

        /*
        //do not switch audio is this case
        err = SwitchAudio(aIndex, index);
        if (err != ME_OK){
            AM_ERROR("HandleLayoutTeleconference: SwitchAudio failure, err:%d\n", err);
            return ME_ERROR;
        }
        */
        err = mpLayout->SetLayoutType(layout); //only window config done
        mpLayout->UpdateLayout();
    }else if(layout == LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN){
        err = mpLayout->SetLayoutType(layout);
        mpLayout->UpdateLayout();
    }else if(layout == LAYOUT_TYPE_RECOVER){
        err = mpLayout->SetLayoutType(mpLayout->GetLayoutType());
        mpLayout->UpdateLayout();
    } else{
        AM_ASSERT(0);
    }

    mpMdecInfo->isNvr = AM_TRUE;
    mbOnNVR = AM_TRUE;
    //if(0 != (mpConfig->globalFlag & NO_AUDIO_ON_GMF)) {
    //No HD mode, do not change audio when relayout
        mAudioWait = AUDIO_WAIT_DONE;
    //}
    UpdateNvrOsdInfo();
    return err;
}

AM_ERR CActiveMDecEngine::HandleConfigTargetWindow(AM_INT target, CParam* pParam)
{
    AM_ERR err;
    if(mpLayout->GetLayoutType() != LAYOUT_TYPE_TELECONFERENCE){
        AM_ERROR("current layout type(%d) is not LAYOUT_TYPE_TELECONFERENCE, changing window position is not suported!\n", mpLayout->GetLayoutType());
        //return ME_ERROR;
    }

    if(target == 0){
        AM_WARNING("window 0 is the full screen window, changing window position is not supported!\n");
        //return ME_ERROR;
    }

    err = mpLayout->ConfigTargetWindow(target, pParam);
    if(err != ME_OK){
        AM_ERROR("HandleMoveWindow: MoveTargetWindow falied!\n");
        return err;
    }
    err = mpLayout->UpdateLayout();

    return err;
}
//-----------------------------------------------------------------------
//
//Msg Handlers
//
//-----------------------------------------------------------------------
void CActiveMDecEngine::HandleReadyMsg(AM_MSG& msg)
{

}

void CActiveMDecEngine::HandleEOSMsg(AM_MSG& msg)
{
    /*
    IFilter *pFilter = (IFilter*)msg.p1;
    AM_ERR err;
    AM_PlayItem* pnext;
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
    */
}

void CActiveMDecEngine::HandleSyncMsg(AM_MSG& msg)
{
    return;
}

void CActiveMDecEngine::HandleForceSyncMsg(AM_MSG& msg)
{
    return;
}

void CActiveMDecEngine::HandleErrorMsg(AM_MSG& msg)
{
    return;
}

AM_ERR CActiveMDecEngine::HandleMsgDspShow(AM_MSG& msg)
{
    AM_INT index = msg.p2;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    CDemuxerConfig* curConfig = NULL;
    AM_INT i;

    static AM_U64 gap = 0, save = 0;
    struct timeval tv;
    gettimeofday(&tv,NULL);
    gap = tv.tv_sec*1000000+tv.tv_usec - save;
    save = tv.tv_sec*1000000+tv.tv_usec;
    AM_INFO("HandleMsgDspShow For %d, SwitchWait:%d. Time:%lld Usec.\n", index, mSwitchWait, gap);

    switch(mSwitchWait)
    {
    case SWITCH_SD_HD_WAIT:
    case SWITCH_SD_HD_WAIT2:
        AM_ASSERT(index == mHdInfoIndex);
        {
            AM_INFO("Config Windows to Show HD On Msg.\n");
            HandleConfigWinMap(index, mUdecNum - 1, 0);
            HandlePerformWinConfig();
            AM_INFO("Well.\n");
            //process 4sd, cur sd +3sd
            DoPauseSmallDspMw(SWITCH_ACTION);
            InfoHideAllSmall(AM_TRUE);
            mpDemuxer->Config(CMD_EACH_COMPONENT);
            //flush every thing
            for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
                curInfo = &(mpMdecInfo->unitInfo[i]);
                if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE
                    && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup])
                    DoFlushSpecifyDspMw(i);
            }
            mpMdecInfo->isNvr = AM_FALSE;
            curInfo = &(mpMdecInfo->unitInfo[index]);
            curInfo->isPaused = AM_FALSE;
            curInfo->playStream = TYPE_VIDEO;
            mbOnNVR = AM_FALSE;
            UpdateNvrOsdInfo();
        }
        mSwitchWait = SWITCH_WAIT_NOON;
        break;

    case BACK_HD_SD_WAIT:
    case BACK_HD_SD_WAIT2:
        curInfo = &(mpMdecInfo->unitInfo[index]);
        if(curInfo->is1080P == AM_TRUE){
            //HD SWITCH THEN BACK IMM
            break;
        }
        AM_ASSERT(curInfo->is1080P == AM_FALSE);
        curInfo->isCheck = AM_TRUE;
        AM_INFO("Check Msg Show for %d OK\n", index);

        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE &&
                curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup])
                    if(curInfo->isCheck == AM_FALSE){
                        AM_INFO("Wait sd:%d\n", i);
                        break;
                     }
        }

        if(i < MDEC_SOURCE_MAX_NUM)
            break;
        {
            AM_INFO("Switch to Sd playback..\n");
            mpLayout->SyncLayoutByAction(ACTION_BACK, -1);
            AM_INFO("Well!\n");
            //handle audio
            curInfo = &(mpMdecInfo->unitInfo[mHdInfoIndex]);
            if(0 == (mpConfig->globalFlag & NO_AUDIO_ON_GMF)) {
                if(curInfo->isNet == AM_FALSE)
                    DoSwitchBackAudioLocal();
                else{
                    if(mHdInfoIndex==mpConfig->GetAudioSource())
                        DoSwitchBackAudioNet();
                }
            }

            //handle 4sd and hd
            mpVideoDecoder->Pause(DEC_MAP(mHdInfoIndex));

            curConfig = &(mpConfig->demuxerConfig[mHdInfoIndex]);
            curConfig->hided = AM_TRUE;
            mpDemuxer->Config(mHdInfoIndex);

            if(mpMdecInfo->unitInfo[mHdInfoIndex].isPaused == AM_TRUE){
                AM_INFO("Paused will be resumed by switch back for Hd %d.\n", mHdInfoIndex);
                DoFlushSpecifyDspMw(mHdInfoIndex);
            }else{
                DoFlushSpecifyDspMw(mHdInfoIndex);
                mpVideoDecoder->Resume(DEC_MAP(mHdInfoIndex));
            }
            mpMdecInfo->isNvr = AM_TRUE;
            curInfo = &(mpMdecInfo->unitInfo[mHdInfoIndex]);
            curInfo->isPaused = AM_TRUE;
            curInfo->playStream = TYPE_VIDEO;
            if(0 == (mpConfig->globalFlag & NO_AUDIO_ON_GMF)) {
                if(mAudioWait == AUDIO_WAIT_DONE){
                    mAudioWait = AUDIO_WAIT_NOON;
                    mbOnNVR = AM_TRUE;
                }else{
                    AM_ASSERT(mAudioWait == AUDIO_WAIT_NOON);
                    mAudioWait = BACK_AUDIO_WAIT;
                }
            }
            UpdateNvrOsdInfo();
        }
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE)
                    curInfo->isCheck = AM_FALSE;
        }
        mHdInfoIndex = -1;//just a check
        mSwitchWait = SWITCH_WAIT_NOON;
        //finalize
        if(mNvrAudioIndex == -1 ||mpConfig->GetAudioSource() == -1){
            AM_INFO("The sources don't have audio source.\n");
            mAudioWait = AUDIO_WAIT_NOON;
            mbOnNVR = AM_TRUE;
        }
        break;

    case SWITCH_TELE_WAIT1:
        mSwitchWait = SWITCH_TELE_WAIT2;
        break;

    case SWITCH_TELE_WAIT2:
        mSwitchWait = SWITCH_WAIT_NOON;
        break;

    case SWITCH_WAIT_NOON:{
        if(0 == (mpConfig->globalFlag & NO_AUDIO_ON_GMF)) {
            if(mAudioWait == AUDIO_WAIT_DONE){
                mAudioWait = AUDIO_WAIT_NOON;
                mbOnNVR = AM_TRUE;
            }
        }
        if(mbSeeking == AM_TRUE){
            mbSeeking = AM_FALSE;
            AM_INFO("Get DSP outpic msg, reset mbSeeking.\n");
            break;
        }
        if(mHdInfoIndex!=-1){
            //hardcode, to reset HD zoom
            MdecInfo::MdecUnitInfo* curInfo = NULL;
            curInfo = &(mpMdecInfo->unitInfo[mHdInfoIndex]);
            AM_INFO("mpRenderer: %p, curInfo->is1080P %d, REN_MAP(hdIndex) 0.\n",
                mpRenderer, curInfo->is1080P);
            if (mpRenderer && curInfo->is1080P) {
                //hardcode here
                mpRenderer->PlaybackZoom(0, 1920, 1080, 960, 540);
            }
        }
        break;
    }
    default:
        AM_ASSERT(0);
        break;
    }

    return ME_OK;
}

//Have to handle two case:
//one, render sync at the begining for (some)SD + (0 or some)HD
//two, dynamic add source for SD and HD
AM_ERR CActiveMDecEngine::HandleMsgRenderSync(AM_MSG& msg)
{
    //We use this to sync net playback(buffer is not ok during net playback)
    AM_INT i;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    //Dump();
    AM_INFO("Dsp and vout releated resouce are created done!Begin the NetPlayback\n");
    if(!(mpConfig->mDecoderCap & DECODER_CAP_DSP)){
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE){
                mpConfig->demuxerConfig[i].needStart = AM_TRUE;
                mpDemuxer->Config(i);
            }
        }
        return ME_OK;
    }

    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->isNet == AM_TRUE){
            if(curInfo->is1080P == AM_FALSE){
                mpVideoDecoder->Pause(DEC_MAP(i));
                if(curInfo->playStream & TYPE_AUDIO)
                    mpAudioSinkFilter->Pause(i);
            }
            mpConfig->demuxerConfig[i].needStart = AM_TRUE;//start all net streaming. hd will be hided.
            mpDemuxer->Config(i);
        }else if(curInfo->isUsed == AM_TRUE && curInfo->isNet == AM_FALSE){
            if(curInfo->is1080P == AM_FALSE){
                mpVideoDecoder->Pause(DEC_MAP(i));
                if(curInfo->playStream & TYPE_AUDIO)
                    mpAudioSinkFilter->Pause(i);
            }
            mpConfig->demuxerConfig[i].needStart = AM_TRUE;//start all local file, fill bufferpool. hd will be hided
            mpDemuxer->Config(i);
        }
    }
    //Flush Net Buffer to offset the buffer data during parsing.
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->isNet == AM_TRUE && curInfo->is1080P == AM_FALSE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
            //mpVideoDecoder->Pause(DEC_MAP(i));
            //DoFlushSpecifyDspMw(i);
            mpDemuxer->Pause(i);
            mpDemuxer->Flush(i);
            mpDemuxer->Resume(i);
            //mpVideoDecoder->Resume(DEC_MAP(i)); //wait until the all net buffer 15 frames
        }
    }
    AM_INT frameOk = 0;
    AM_INT smallNet = 0;
    CParam64 par(1);
    //still has diff between windows during local file
    //usleep(50000);
    //still have diff , feed to bsb buffer and the start, For Net, data speed is ok.
    //Local file may have high speed and will let bsb goto full before this buffer-check. Fix by pause demuxer.
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->isNet == AM_FALSE && curInfo->is1080P == AM_FALSE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
            mpConfig->decoderConfig[i].onlyFeed = AM_TRUE;
            mpVideoDecoder->Resume(DEC_MAP(i));
            mpDemuxer->Pause(i);
        }
    }
    while(AM_TRUE && mpLayout->GetLayoutType() == LAYOUT_TYPE_TABLE)
    {
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
        {
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->isNet == AM_FALSE && curInfo->is1080P == AM_FALSE
                && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
                smallNet |= (1<<i);
                mpVideoDecoder->QueryInfo(DEC_MAP(i), DATA_FEED_TO_BSB, par);
                //AM_INFO("Engine get %d Info %lld\n", i, par[0]);
                if(frameOk & (1<<i)){
                    //AM_INFO("Stream %d BsB buffer too much->%lld\n", i, par[0]);
                    continue;
                }
                if(par[0] >= BSB_HOLD_SIZE_BEFORE_PLAY){
                    AM_INFO("Stream %d Feed Data Size %lld To BSB Bufferpool.\n", i, par[0]);
                    mpVideoDecoder->Pause(DEC_MAP(i));
                    AM_INFO("Stream %d Pause Done\n", i);
                    frameOk |= (1<<i);
                }
            }
        }
        if(frameOk == smallNet)
            break;
    }
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->isNet == AM_FALSE && curInfo->is1080P == AM_FALSE
            && curInfo->addOrder == mpMdecInfo->winOrder[curInfo->sourceGroup]){
            mpConfig->decoderConfig[i].onlyFeed = AM_FALSE;
        }
    }
    Dump();

    //MW PreBuffer
    frameOk = smallNet = 0;
    while(1 && mBufferNum != 0)
    {
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
        {
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->isNet == AM_TRUE && curInfo->is1080P == AM_FALSE){
                smallNet |= (1<<i);
                mpVideoDecoder->QueryInfo(DEC_MAP(i), FRAME_BUFFER_NUM, par);
                if(frameOk & (1<<i)){
                    AM_INFO("Stream %d buffer too much->%lld\n", i, par[0]);
                    continue;
                }
                if(par[0] >= mBufferNum)
                    frameOk |= (1<<i);
            }
        }
        if(frameOk == smallNet)
            break;
    }
    //AM_INFO("Debug: NetMask:%d, FrameOk:%d\n", smallNet, frameOk);
    //mpVideoDecoder->Config(CMD_GENERAL_ONLY, CHECK_VDEC_FRAME_NET);


    //Dump();
    //struct timeval tvbefore, tvafter;
    //gettimeofday(&tvbefore,NULL);
    //AM_U64 totalTime = 0;

    /*//Start the local file
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->isNet == AM_FALSE){
            mpConfig->demuxerConfig[i].needStart = AM_TRUE;
            mpDemuxer->Config(i);
        }
        gettimeofday(&tvafter,NULL);
        totalTime = (tvafter.tv_sec -tvbefore.tv_sec)*1000000 + tvafter.tv_usec - tvbefore.tv_usec;
        AM_INFO("Consume Time::::%lld\n", totalTime);
    }*/
    //gettimeofday(&tvbefore,NULL);
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE){
            if(curInfo->isNet == AM_FALSE)
                mpDemuxer->Resume(i);
            mpVideoDecoder->Resume(DEC_MAP(i));
            if(curInfo->playStream & TYPE_AUDIO)
                mpAudioSinkFilter->Resume(i);
            //Dump();
        }
        //gettimeofday(&tvafter,NULL);
        //totalTime = (tvafter.tv_sec -tvbefore.tv_sec)*1000000 + tvafter.tv_usec - tvbefore.tv_usec;
        //AM_INFO("Consume Time::::%lld\n", totalTime);
    }
    //Dump();
    ///
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE){
            DoCreatePipeLine(i);
        }
    }
    SetState(STATE_SYNCED);
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleMsgRenderSyncDynamic(AM_MSG& msg)
{
    AM_ERR err = ME_OK;
    AM_INT infoIndex = msg.p1;
    AM_INFO("Handle for Dynamic add Source:%d\n", infoIndex);
    mpDemuxer->Config(infoIndex);
    //AM_INFO("\ndoNE------------------------------------------3\n");
    DoCreatePipeLine(infoIndex);
    return err;
}

AM_ERR CActiveMDecEngine::HandleMsgUdecError(AM_MSG& msg)
{
    AM_ERR err = ME_OK;
    AM_INT infoIndex = msg.p2;
    AM_INT dspIndex = DSP_MAP(infoIndex);
    AM_ASSERT(dspIndex >= 0);
    IUDECHandler* pDsp = mpConfig->dspConfig.udecHandler;
    AM_UINT udecState, voutState, errorCode;
    err = pDsp->GetUDECState(dspIndex, udecState, voutState, errorCode);
    AM_ASSERT(err == ME_UDEC_ERROR);
    UDECErrorCode error;
    error.mu32 = errorCode;
    AM_INFO("HandleMsgUdecError for Dsp %d, ErrorCode:0x%x, Error Type:%d, Level:%d, Dec Type:%d",
        dspIndex, errorCode, error.detail.error_type, error.detail.error_level, error.detail.decoder_type);

    CParam64 parVec(3);
//    AM_S64 targetTime;

    //Check
    if (error.detail.error_level >= EH_ErrorLevel_Last || error.detail.decoder_type >=EH_DecoderType_Cnt){
        AM_ASSERT(0);
        AM_ERROR("Must have errors, (AnalyseUdecErrorCode) decoder_type/error level out of range.\n");
        err = ME_OK;
        goto exit;
    }
    if(error.detail.error_level < EH_ErrorLevel_Fatal){
        AM_INFO("Dsp Can handle this error by itself! Just Notify to App\n");
        PostAppMsg(msg);
        err = ME_OK;
        goto exit;
    }
    AM_ASSERT(error.detail.error_level == EH_ErrorLevel_Fatal);

    //error handle
    switch(error.detail.decoder_type)
    {
    case EH_DecoderType_H264:
    case EH_DecoderType_Common:
        AM_INFO("Begin Handle error....\n");
        mpDemuxer->Pause(infoIndex);
        DoFlushSpecifyDspMw(infoIndex);
        if(mpConfig->demuxerConfig[infoIndex].netused == AM_TRUE){

        }else{
            /*
            err = mpVideoDecoder->QueryInfo(DEC_MAP(infoIndex), INFO_TIME_PTS, parVec);
            if(err != ME_OK){
                AM_ASSERT(0);
                parVec[0] = parVec[1] = 0;
            }
            targetTime = parVec[0]; //FEED PTS
            mpDemuxer->SeekTo(infoIndex, targetTime + 90000, 0);
            */
        }
        mpDemuxer->Resume(infoIndex);
        err = ME_OK;
        break;

    default:
        AM_INFO("NVR now only support h264 decode!\n");
        AM_ASSERT(0);
        err = ME_ERROR;
    }

exit:
    //recover
    mpVideoDecoder->TryAgain(DEC_MAP(infoIndex));
    return err;
}

//a case AUDIO_WAIT_DONE && SWITCH_WAIT_NOON means only wait audio done.
AM_ERR CActiveMDecEngine::HandleMsgAddAudio(AM_MSG& msg)
{
    AM_INT index = msg.p1;
    AM_INFO("HandleMsgAddAudio %d\n", mAudioWait);
    if(mAudioWait == AUDIO_WAIT_NOON)
        return ME_OK;
    //if mAudioWait is seted someWhere, Then even this msg erase too early, it will still ok, except for switch by msg.
    //handle switch by msg
    if(mSwitchWait != SWITCH_WAIT_NOON)
    {
        mAudioWait = AUDIO_WAIT_DONE;
        return ME_OK;
    }
    if(mAudioWait == SWITCH_AUDIO_WAIT){
        if(mpLayout->GetLayoutType() != LAYOUT_TYPE_TELECONFERENCE){
            mbOnNVR = AM_FALSE;
        }
        AM_ASSERT(index == mHdInfoIndex);
    }else if(mAudioWait == BACK_AUDIO_WAIT){
        //AM_ASSERT(mAudioWait == BACK_AUDIO_WAIT);
        mbOnNVR = AM_TRUE;
        AM_ASSERT(index == mNvrAudioIndex);
    }
    mAudioWait = AUDIO_WAIT_NOON;
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleMsgEnd(AM_MSG& msg)
{
    AM_INT index, i;
    index = msg.p1;
    MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[index]);

    curInfo->isEnd = AM_TRUE;
    if(mpConfig->globalFlag & LOOP_FOR_LOCAL_FILE)
    {
        AM_INFO("Loop for Stream %d.\n", index);
        mpDemuxer->SeekTo(index, 0, SEEK_FOR_LOOP);
        return ME_OK;
    }
    if(mbOnNVR == AM_FALSE)
    {
        AM_INFO("Stream End During Full Windows PlayBack\n");
        //Stop();
    }else{
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            curInfo = &(mpMdecInfo->unitInfo[i]);
            if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE && curInfo->isEnd== AM_FALSE){
                break;
            }
        }
        AM_INFO("Show i :%d\n", i);
        if(i == MDEC_SOURCE_MAX_NUM){
            AM_INFO("Stream End During NVR Windows PlayBack\n");
            //Stop();
        }
    }
    return ME_OK;
}
//
//
//-----------------------------------------------------------------------------------------
/*
void CActiveMDecEngine::CheckFileType(const char *pFileName)
{
    // default
    mbUseAbsoluteTime = true;

    if (!pFileName) {
        return ;
    }

    // rtsp protocol
    if (!strncmp(pFileName, "rtsp", 4)) {
        mbUseAbsoluteTime = false;
        AMLOG_INFO("CActiveMDecEngine::CheckFileType: rtsp protocol.\n");
        return ;
    }
}
*/
AM_ERR CActiveMDecEngine::UpdateNvrOsdInfo()
{
    return ME_OK;
}

AM_ERR CActiveMDecEngine::UpdateUdecConfigInfo()
{
    AM_ERR err = ME_OK;
    AM_INT i;
    CParam param(2);
    for(i = 0; i < mUdecNum; i++){
        if(mStartIndex >= 0){
            err = mpDemuxer->GetStreamInfo(i + mStartIndex, param);
        }else{
            //there is no any source.
            err = ME_NO_IMPL;
        }
        if(err == ME_OK){
            AM_INFO("stream %d, video size: %d x %d\n", i, param[0], param[1]);
            mShared.dspConfig.udecInstanceConfig[i].max_frm_width = param[0];
            mShared.dspConfig.udecInstanceConfig[i].max_frm_height = param[1];
        }else if(err == ME_NO_IMPL && mNvrPbMode != -1){
            AM_INFO("update udec info to mNvrMode: %d\n", mNvrPbMode);
            mShared.dspConfig.udecInstanceConfig[i].max_frm_width = (mNvrPbMode == NVR_4X480P)? 720: 1280;
            mShared.dspConfig.udecInstanceConfig[i].max_frm_height = (mNvrPbMode == NVR_4X480P)? 480: 720;
            err = ME_OK;
        }else{
            AM_INFO("max_frame size default to 720x480\n");
            mShared.dspConfig.udecInstanceConfig[i].max_frm_width = 720;
            mShared.dspConfig.udecInstanceConfig[i].max_frm_height = 480;
        }
    }

    return err;
}

//ADEC_MAP(I) == I;
AM_INT CActiveMDecEngine::DEC_MAP(AM_INT index)
{
    return mpConfig->indexTable[index].decIndex;
}

AM_INT CActiveMDecEngine::REN_MAP(AM_INT index)
{
    return mpConfig->indexTable[index].renIndex;
}

AM_INT CActiveMDecEngine::DSP_MAP(AM_INT index)
{
    return mpConfig->indexTable[index].dspIndex;
}

AM_ERR CActiveMDecEngine::Dump(AM_INT flag)
{
    AM_INT i = 0;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    CMapTable* curMap = NULL;

    AM_INFO("====================BEGING DUMP=============\n");
    if(flag & DUMP_FLAG_FFMPEG){
        mpDemuxer->Dump(flag);
        return ME_OK;
    }

    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE)
        {
            AM_INFO("       {---Source %d Info: isPaused :%d, isNet:%d, isHided:%d, playStream:%d, Group:%d, addOrder:%d---}\n",
                i, curInfo->isPaused, mpConfig->demuxerConfig[i].netused, mpConfig->demuxerConfig[i].hided, curInfo->playStream, curInfo->sourceGroup, curInfo->addOrder);
        }
    }
    AM_INFO("       ---------------------Map Dump------------\n");
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        curMap = &(mpConfig->indexTable[i]);
        if(curMap->index != -1)
        {
            AM_INFO("       {---Source %s Index %d, %s, Map:[Win(DRen):%d(%d), Dec:%d, Ren:%d, Dsp:%d]---}\n", curMap->file,
                curMap->index, (curMap->isHd == AM_TRUE) ? " HD File" : " SD File",
                curMap->winIndex, curMap->dsprenIndex, curMap->decIndex, curMap->renIndex, curMap->dspIndex);
        }
    }
    AM_INFO("       ---------------------Layout Dump------------\n");
    mpLayout->Dump(flag);

    AM_INFO("\n---------------------Engine Info------------\n");
    AM_INFO("       Use Case: [%s] [ ]\n", (mbNetStream == AM_TRUE) ? "Net Play" : "Local Play");
    AM_INFO("\n");
    mpDemuxer->Dump();
    mpVideoDecoder->Dump();
    if(mpAudioDecoder) mpAudioDecoder->Dump();
    if(mpTansAudioSink) mpTansAudioSink->Dump();
    mpRenderer->Dump();
    mpMuxer->Dump();
    if(mbEnableTanscode) mpTranscoder->Dump();
    mpPipeLine->Dump(flag);
    AM_INFO("====================END DUMP===============\n");
    return ME_OK;
}

AM_ERR CActiveMDecEngine::BuildMDecGragh()
{
    AM_ERR err;

    if((mpDemuxer = CreateGeneralDemuxerFilter((IEngine*)this, mpConfig)) == NULL)
        return ME_ERROR;
    AddFilter((IFilter*)mpDemuxer);

    if((mpVideoDecoder = CreateGeneralDecoderFilter((IEngine*)this, mpConfig)) == NULL)
        return ME_ERROR;
    AddFilter((IFilter*)mpVideoDecoder);

    if(mbEnableTanscode){
        if((mpTansAudioSink = CreateGeneralTransAudioSinkFilter((IEngine*)this, mpConfig)) == NULL)
            return ME_ERROR;
        AddFilter((IFilter*)mpTansAudioSink);
        mpAudioSinkFilter = (CInterActiveFilter*)mpTansAudioSink;
    }else{
        if((mpAudioDecoder = CreateGeneralDecoderFilter((IEngine*)this, mpConfig)) == NULL)
            return ME_ERROR;
        AddFilter((IFilter*)mpAudioDecoder);
        mpAudioSinkFilter = (CInterActiveFilter*)mpAudioDecoder;
    }

    if((mpRenderer = CreateGeneralRendererFilter((IEngine*)this, mpConfig)) == NULL)
        return ME_ERROR;
    AddFilter((IFilter*)mpRenderer);

    if(mbEnableTanscode){
        if((mpTranscoder = CreateGeneralTranscoderFilter((IEngine*)this, mpConfig)) == NULL)
            return ME_ERROR;
        AddFilter((IFilter*)mpTranscoder);
    }

    //connect video
    if((err = Connect((IFilter*)mpDemuxer, 0, (IFilter*)mpVideoDecoder, 0)) != ME_OK){
        AM_ERROR("Connect mpDemuxer<->mpVideoDecoder on video failed.\n");
        return err;
    }
    //connect audio
    if(!mbEnableTanscode){
        if((err = Connect((IFilter*)mpDemuxer, 1, (IFilter*)mpAudioDecoder, 0)) != ME_OK){
            AM_ERROR("Connect mpDemuxer<->mpVideoDecoder on audio failed.\n");
            return err;
        }
    }else{
        if((err = Connect((IFilter*)mpDemuxer, 1, (IFilter*)mpTansAudioSink, 0)) != ME_OK){
            AM_ERROR("Connect mpDemuxer<->mpTansAudioSink on audio failed.\n");
            return err;
        }
    }
    //renderer
    if((err = Connect((IFilter*)mpVideoDecoder, 0, (IFilter*)mpRenderer, 0)) != ME_OK){
        AM_ERROR("Connect mpRenderer on video failed.\n");
        return err;
    }

    if(!mbEnableTanscode){
        if((err = Connect((IFilter*)mpAudioDecoder, 0, (IFilter*)mpRenderer, 1)) != ME_OK){
            AM_ERROR("Connect mpRenderer on audio failed.\n");
            return err;
        }
    }else{
        if((err = Connect((IFilter*)mpTansAudioSink, 0, (IFilter*)mpTranscoder, 0)) != ME_OK){
            AM_ERROR("Connect mpDemuxer<->mpTansAudioSink on audio failed.\n");
            return err;
        }
        mpTansAudioSink->SetTranscoderContext(mpTranscoder);
    }

    //Transcoder filter has no video input, which get video data from dsp.
    //But it may need audio data from audio manager/demuxer, todo
    AMLOG_INFO("BuildMDecFilterGragh done.\n");
    return ME_OK;
}

AM_ERR CActiveMDecEngine::InitDspAudio(CParam& par)
{
    AM_ERR err;
    AM_INT i;
    AM_INT num;
    //audio
    if(!mbEnableTanscode) {
        mpConfig->audioConfig.audioHandle = AM_CreateAudioHAL(this, 1, 0);
        if(mpConfig->audioConfig.audioHandle == NULL)
            return ME_ERROR;
    }
    mpConfig->audioConfig.isOpened = AM_FALSE;
    //-------------------------------
    //Todo, discard this shared object
    memset((void*)&mShared, 0, sizeof(mShared));
    mShared.mEngineVersion = 1;
    mShared.mbIavInited = 0;
    mShared.mIavFd = -1;//init value
    char configfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};
    snprintf(configfilename, sizeof(configfilename), "%s/pb.config", AM_GetPath(AM_PathConfig));
    i = AM_LoadPBConfigFile(configfilename, &mShared);
    snprintf(configfilename, sizeof(configfilename), "%s/log.config", AM_GetPath(AM_PathConfig));
    AM_LoadLogConfigFile(configfilename);
    if (i==0) {
        AM_DefaultPBConfig(&mShared);
        AM_DefaultPBDspConfig(&mShared);
    }
    AMLOG_INFO("Load config file, configNumber=%d:\n", i);
    //pthread_mutex_init(&mShared.mMutex, NULL);
    AM_PrintLogConfig();
    AM_PrintPBDspConfig(&mShared);
    mpOpaque = (void*)&mShared;
    mShared.ppmode = 3;
    mShared.mDSPmode = 16;
    //config both lcd and hdmi
    mShared.pbConfig.vout_config = 3;
    mShared.bEnableDeinterlace = AM_FALSE;
    mShared.dspConfig.hdWin = 1;
    mShared.dspConfig.voutMask = 0;
    mShared.dspConfig.curInstanceIsHd = -1;

    //about set
    if(!(mpConfig->mDecoderCap & DECODER_CAP_DSP)){
        return ME_OK;
    }

    DSPConfig* dc = &(mShared.dspConfig);
    if(par[GLOBAL_FLAG] & NO_HD_WIN_NVR_PB){
        AM_INFO("Mdec Only for Nvr Playback.\n");
        dc->hdWin = 0;
    }

    dc->voutMask= par[VOUT_MASK];
    dc->preset_tilemode = par[DSP_TILEMODE];
    dc->pre_buffer_len = mpConfig->dspConfig.preBufferLen;
    dc->enable_dsp_pre_ctrl = mpConfig->dspConfig.enDspBufferCtrl;
    for(i = 0; i < DMAX_UDEC_INSTANCE_NUM; i++)
        dc->udecInstanceConfig[i].max_frm_num = mpConfig->dspConfig.eachDecBufferNum;

    //transcode
    if(mbEnableTanscode){
        mShared.sTranscConfig.enableTranscode = true;
    }
    if (mShared.udecHandler == NULL)
    {
        err = AM_CreateDSPHandler(&mShared);
        if(err != ME_OK)
            return err;
        //todo
    }
    mpMdecInfo->nvrIavFd = mShared.mIavFd;
    mpConfig->oldCompatibility = (void*)&mShared;
    CDspGConfig* pDsp = &mpConfig->dspConfig;
    pDsp->iavHandle = mShared.mIavFd;
    pDsp->udecHandler = mShared.udecHandler;

    num = par[REQUEST_DSP_NUM];
    AM_INFO("Udec Num:%d\n", num);
    if(num != STAGE1_UDEC_NUM){
        AM_INFO("Do cc!\n");
    }
    mUdecNum = num;
    pDsp->udecHandler->SetUdecNums(num);
    pDsp->udecHandler->InitWindowRender(mpConfig);
    if (IsShowOnLCD()) {
        mpMdecInfo->displayWidth = mShared.dspConfig.voutConfigs.voutConfig[eVoutLCD].height;
        mpMdecInfo->displayHeight = mShared.dspConfig.voutConfigs.voutConfig[eVoutLCD].width;
    } else {
        mpMdecInfo->displayWidth = mShared.dspConfig.voutConfigs.voutConfig[eVoutHDMI].width;
        mpMdecInfo->displayHeight = mShared.dspConfig.voutConfigs.voutConfig[eVoutHDMI].height;
    }
    if(mpLayout->GetLayoutType() != mpConfig->mInitialLayout){
        AM_INFO("Rejust init Layout to %d\n", mpConfig->mInitialLayout);
        mpLayout->SetLayoutType((LAYOUT_TYPE)mpConfig->mInitialLayout);
        mpLayout->UpdateLayout();
    }
    return ME_OK;
}

bool CActiveMDecEngine::ProcessGenericCmd(CMD& cmd)
{
    AM_ERR err;
    AM_UINT par;
    AM_INT par2;
    CParam* param;

    //AMLOG_CMD("****CActiveMDecEngine::ProcessGenericCmd, cmd.code %d, state %d.\n", cmd.code, mInfo.state);

    switch (cmd.code) {
        case CMD_STOP:
            AMLOG_INFO("****CActiveMDecEngine CMD_STOP cmd, start.\n");
            mbRun = false;
            err = HandleStop();
            if(err != ME_OK)
            {
                AM_ERROR("Stop on Engine Failed!\n");
            }
            mpWorkQ->CmdAck(err);
            AMLOG_INFO("****CActiveMDecEngine CMD_STOP cmd, done.\n");
            break;

         case MDCMD_EDIT_SOURCE_DONE:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_EDIT_SOURCE_DONE cmd, start.\n");
            err = HandleEditSourceDone();
            mpWorkQ->CmdAck(err);
            AMLOG_INFO("****CActiveMDecEngine MDCMD_EDIT_SOURCE_DONE cmd, done.\n");
            break;

         case MDCMD_EDIT_SOURCE:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_EDIT_SOURCE cmd, start.\n");
            par = cmd.res32_1;
            par2 = cmd.res64_1;
            err = HandleEditSource(par2, par);
            mpWorkQ->CmdAck(err);
            AMLOG_INFO("****CActiveMDecEngine MDCMD_EDIT_SOURCE cmd, done.\n");
            break;

         case MDCMD_BACK_SOURCE:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_BACK_SOURCE cmd, start.\n");
            par = cmd.res32_1;
            par2 = cmd.res64_1;
            err = HandleBackSource(par2, cmd.res64_2, par);
            mpWorkQ->CmdAck(err);
            AMLOG_INFO("****CActiveMDecEngine MDCMD_BACK_SOURCE cmd, done.\n");
            break;

         case MDCMD_DELETE_SOURCE:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_DELETE_SOURCE cmd, start.\n");
            par = cmd.res32_1;
            par2 = cmd.res64_1;
            err = HandleDeleteSource(par2, par);
            mpWorkQ->CmdAck(err);
            AMLOG_INFO("****CActiveMDecEngine MDCMD_DELETE_SOURCE cmd, done.\n");
            break;


       case MDCMD_ADD_SOURCE:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_ADD_SOURCE cmd, start.\n");
            par = cmd.res32_1;
            par2 = cmd.res64_1;
            err = HandleAddSource(par2, par);
            mpWorkQ->CmdAck(err);
            AMLOG_INFO("****CActiveMDecEngine MDCMD_ADD_SOURCE cmd, done.\n");
            break;

        case MDCMD_PREPARE:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PREPARE cmd, start.\n");
            mpWorkQ->CmdAck(HandlePrepare());
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PREPARE cmd, done.\n");
            break;

        case MDCMD_START:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PREPARE cmd, start.\n");
            err = HandleStart(mbNetStream == AM_TRUE);
            mpWorkQ->CmdAck(err);
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PREPARE cmd, done.\n");
            break;

        case MDCMD_STOP:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PREPARE cmd, start.\n");
            mpWorkQ->CmdAck(HandleStop());
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PREPARE cmd, done.\n");
           break;

        case MDCMD_DISCONNECT:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PREPARE cmd, start.\n");
            mpWorkQ->CmdAck(HandleDisconnectHw());
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PREPARE cmd, done.\n");
           break;

        case MDCMD_RECONNECT:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PREPARE cmd, start.\n");
            mpWorkQ->CmdAck(HandleReconnectHw());
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PREPARE cmd, done.\n");
           break;

        case MDCMD_SEEKTO:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_SEEK cmd, start.\n");
            //Todo: seek while trickplay?
            par2 = cmd.res64_1;
            par = cmd.res32_1;
            err = HandleSeekTo(par2 , par);
            mpWorkQ->CmdAck(err);
            AMLOG_INFO("****CActiveMDecEngine MDCMD_SEEK cmd, done.\n");
            break;

        case MDCMD_PAUSEPLAY:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PAUSEPLAY cmd, start.\n");
            if(cmd.flag & PAUSE_RESUME_STEP_HD){
                err = HandlePause(-1);
            }else{
                par = cmd.res32_1;
                err = HandlePause(par);
            }
            mpWorkQ->CmdAck(err);
            AMLOG_INFO("****CActiveMDecEngine MDCMD_PAUSEPLAY cmd, done.\n");
            break;

        case MDCMD_RESUMEPLAY:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_RESUMEPLAY cmd, start.\n");
            if(cmd.flag & PAUSE_RESUME_STEP_HD){
                err = HandleResume(-1);
            }else{
                par = cmd.res32_1;
                err = HandleResume(par);
            }
            mpWorkQ->CmdAck(err);
            AMLOG_INFO("****CActiveMDecEngine MDCMD_RESUMEPLAY cmd, done.\n");
            break;

        case MDCMD_STEPPLAY:
            AMLOG_INFO("****CActiveMDecEngine MDCMD_STEP cmd, start.\n");
            if(cmd.flag & PAUSE_RESUME_STEP_HD){
                err = HandleStep(-1);
            }else{
                par = cmd.res32_1;
                err = HandleStep(par);
            }
            mpWorkQ->CmdAck(err);
            AMLOG_INFO("****CActiveMDecEngine MDCMD_STEP cmd, done.\n");
            break;

        case MDCMD_SELECT_AUDIO:
            AM_INFO("****CActiveMDecEngine MDCMD_SELECT_AUDIO cmd, start.\n");
            par = cmd.res32_1;
            mpWorkQ->CmdAck(HandleSelectAudio(par));
            AM_INFO("****CActiveMDecEngine MDCMD_SELECT_AUDIO cmd, done.\n");
            break;

        case MDCMD_AUDIO_TALKBACK:
            AM_INFO("****CActiveMDecEngine MDCMD_AUDIO_TALKBACK cmd, start.\n");
            par = cmd.res32_1;
            par2 = (AM_INT)cmd.flag2;
            mpWorkQ->CmdAck(HandleTalkBack(par2, par));
            AM_INFO("****CActiveMDecEngine MDCMD_AUDIO_TALKBACK cmd, done.\n");
            break;

        case MDCMD_AUTOSWITCH:
            AM_INFO("****CActiveMDecEngine MDCMD_AUTOSWITCH cmd, start.\n");
            par = cmd.res32_1;
            if(cmd.flag & REVERT_TO_NVR)
                err = HandleRevertNVR();
            else
                err = HandleAutoSwitch(par);
            mpWorkQ->CmdAck(err);
            AM_INFO("****CActiveMDecEngine MDCMD_AUTOSWITCH cmd, done(%d).\n", err);
            break;

        case MDCMD_SWITCHSET:
            AM_INFO("****CActiveMDecEngine MDCMD_SWITCHSET cmd, start.\n");
            par = cmd.res32_1;
            err = HandleSwitchSet(par);
            mpWorkQ->CmdAck(err);
            AM_INFO("****CActiveMDecEngine MDCMD_SWITCHSET cmd, done(%d).\n", err);
            break;

        case MDCMD_PERFORM_WIN:
            AM_INFO("****CActiveMDecEngine MDCMD_PERFORM_WIN cmd, start.\n");
            mpWorkQ->CmdAck(HandlePerformWinConfig());
            AM_INFO("****CActiveMDecEngine MDCMD_PERFORM_WIN cmd, done.\n");
            break;

        case MDCMD_CONFIG_WIN:
            AM_INFO("****CActiveMDecEngine MDCMD_CONFIG_WIN cmd, start.\n");
            param = (CParam* )cmd.pExtra;
            par = cmd.res32_1;
            mpWorkQ->CmdAck(HandleConfigWindow(par, param));
            AM_INFO("****CActiveMDecEngine MDCMD_CONFIG_WIN cmd, done.\n");
            break;

        case MDCMD_CONFIG_WINMAP:
            AM_INFO("****CActiveMDecEngine MDCMD_CONFIG_WINMAP cmd, start.\n");
            mpWorkQ->CmdAck(HandleConfigWinMap(cmd.res64_1, cmd.res64_2, cmd.res32_1));
            AM_INFO("****CActiveMDecEngine MDCMD_CONFIG_WINMAP cmd, done.\n");
            break;

        case MDCMD_SPEED_FEEDING_RULE:
            AM_INFO("****CActiveMDecEngine MDCMD_SPEED_FEEDING_RULE cmd, start.\n");
            mpWorkQ->CmdAck(HandleSpeedFeedingRule((AM_INT)cmd.flag2, cmd.res32_1, (IParameters::DecoderFeedingRule)cmd.res64_1, (AM_UINT)cmd.res64_2));
            AM_INFO("****CActiveMDecEngine MDCMD_SPEED_FEEDING_RULE cmd, done.\n");
            break;

        case MDCMD_RELAYOUT: {
            AM_INFO("****CActiveMDecEngine MDCMD_RELAYOUT cmd, start.\n");
            LAYOUT_TYPE layout = (LAYOUT_TYPE)cmd.res32_1;
            AM_INT index = (AM_INT)cmd.flag2;
            err = HandleReLayout(layout, index);
            mpWorkQ->CmdAck(err);
            AM_INFO("****CActiveMDecEngine MDCMD_RELAYOUT cmd, done.\n");
            }
            break;
        case MDCMD_EXCHANGE_WINDOW: {
            AM_INFO("****CActiveMDecEngine MDCMD_EXCHANGE_WINDOW cmd, start.\n");
            AM_INT winIndex0 = (cmd.res64_1>>16);
            AM_INT winIndex1 = (cmd.res64_1&0xFFFFFFFF);
            err = HandleExchangeWindow(winIndex0, winIndex1);
            mpWorkQ->CmdAck(err);
            AM_INFO("****CActiveMDecEngine MDCMD_EXCHANGE_WINDOW cmd, done.\n");
            }
            break;

        case MDCMD_CONFIG_WINDOW:
            AM_INFO("****CActiveMDecEngine MDCMD_MOVE_WINDOW cmd, start.\n");
            param = (CParam* )cmd.pExtra;
            err = HandleConfigTargetWindow((AM_INT)cmd.flag, param);
            mpWorkQ->CmdAck(err);
            AM_INFO("****CActiveMDecEngine MDCMD_MOVE_WINDOW cmd, done.\n");
            break;

        default:
            return false;
    }
    return true;
}

bool CActiveMDecEngine::ProcessGenericMsg(AM_MSG& msg)
{
//    AM_ERR err;
    //AMLOG_CMD("****CActiveMDecEngine::ProcessGenericMsg, msg.code %d, msg.sessionID %d, state %d.\n", msg.code, msg.sessionID, mInfo.state);

    //discard old sessionID msg first
    if (!IsSessionMsg(msg)) {
        AM_ERROR("got msg(code %d) with old sessionID %d, current sessionID %d.\n", msg.code, msg.sessionID, mSessionID);
        //return true;
    }
    AM_INFO("ProcessGenericMsg %d\n", msg.code);
    switch (msg.code) {
        case MSG_READY:
            AMLOG_INFO("-----MSG_READY-----\n");
            HandleReadyMsg(msg);
            break;

        case MSG_EOS:
            /*
            AMLOG_INFO("-----MSG_EOS-----\n");
            if(mInfo.state == STATE_RUN){
                AM_ASSERT(mpClockManager);
                mpClockManager->StopClock();
                ClearGraph();
                //ResetParameters();
                NewSession();
                SetState(STATE_COMPLETED);
                //SetState(STATE_STOPPED);
            }else{
                HandleEOSMsg(msg);
            }
            AMLOG_INFO("-----MSG_EOS done-----\n");
            */
            break;

        case MSG_AVSYNC:
            //AllFiltersCmd(IActiveObject::CMD_AVSYNC);
            //mShared.mbAlreadySynced = 1;
            break;

        case MSG_ERROR:
            //HandleErrorMsg(msg);
            break;

        case CInterActiveFilter::MSG_RENDER_ADD_AUDIO:
            HandleMsgAddAudio(msg);
            break;

        case CInterActiveFilter::MSG_RENDER_END:
            HandleMsgEnd(msg);
            break;

        case CInterActiveFilter::MSG_SWITCH_TEST:
            break;

        case CInterActiveFilter::MSG_SWITCHBACK_TEST:
            break;

        case CInterActiveFilter::MSG_TEST:
            Dump();
            break;

        case CInterActiveFilter::MSG_UDEC_ERROR:
            HandleMsgUdecError(msg);
            break;

        case CInterActiveFilter::MSG_SYNC_RENDER_DONE:
            if(mState != STATE_SYNCED)
                HandleMsgRenderSync(msg);
            else
                HandleMsgRenderSyncDynamic(msg);
            break;

        case CInterActiveFilter::MSG_DSP_GOTO_SHOW_PIC:
            HandleMsgDspShow(msg);
            break;

        default:
            break;
    }
    //AMLOG_CMD("****CActiveMDecEngine::ProcessGenericMsg, msg.code %d done, msg.sessionID %d, state %d.\n", msg.code, msg.sessionID, mInfo.state);

    return true;
}

void CActiveMDecEngine::ProcessCMD(CMD& oricmd)
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

void CActiveMDecEngine::OnRun()
{
    CMD cmd, nextCmd;
    AM_MSG msg;
    CQueue::QType type;
    CQueue::WaitResult result;
    AM_ASSERT(mpFilterMsgQ);
    AM_ASSERT(mpCmdQueue);
    mpWorkQ->CmdAck(ME_OK);

    SetState(STATE_IDLE);
    while (mbRun) {
        //AMLOG_STATE("CActiveMDecEngine: state=%d, msg cnt from filters %d.\n", mInfo.state, mpFilterMsgQ->GetDataCnt());
        if (mState == STATE_HAS_ERROR) {
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
