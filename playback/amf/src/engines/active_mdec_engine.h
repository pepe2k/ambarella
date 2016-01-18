
/**
 * active_mdec_engine.h
 *
 * History:
 *    2011/12/08 - [GangLiu] created file
 *    2012/3/30- [Qingxiong Z] modify file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __A_MDEC_ENGINE_H__
#define __A_MDEC_ENGINE_H__

#define MAXPATHLENGTH 256
#define MAX_VIDEO_NUMS 16

#define STAGE1_STREAM_NUM 4
#define STAGE1_HD_WIN_NUM 1
#define STAGE1_UDEC_NUM (STAGE1_STREAM_NUM + STAGE1_HD_WIN_NUM)
//-----------------------------------------------------------------------
//
// CActiveMDecEngine
//
//-----------------------------------------------------------------------
class CActiveMDecEngine: public CActiveEngine, public IMDECEngine, public IMDecControl
{
    friend class CPipeLineManager;
    typedef CActiveEngine inherited;
    enum MISC_FLAG{
        REVERT_TO_NVR = 0x000001,
        PAUSE_RESUME_STEP_HD = 0x000010,
    };
    enum ACTION_IDENTIFY{
        SWITCH_ACTION = 1,
        NOR_PARE_ACTION = 2, //normal pause resume.
    };
    enum SWITCH_WAIT{
        SWITCH_SD_HD_WAIT,
        SWITCH_SD_HD_WAIT2,
        BACK_HD_SD_WAIT,
        BACK_HD_SD_WAIT2,
        SWITCH_TELE_WAIT1,
        SWITCH_TELE_WAIT2,
        SWITCH_WAIT_NOON,
    };

    enum ADUIO_WAIT{
        SWITCH_AUDIO_WAIT,
        BACK_AUDIO_WAIT,
        AUDIO_WAIT_DONE, //notify to switchmsg
        AUDIO_WAIT_NOON,
    };

    enum NVR_MODE{
        NVR_4X480P,
        NVR_4X720P,
    };


public:
    static CActiveMDecEngine* Create(void *audiosink, CParam& par);

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();

    //IIEngine
    virtual void *QueryEngineInterface(AM_REFIID refiid);
    virtual AM_ERR PostEngineMsg(AM_MSG& msg)
    {
        return inherited::PostEngineMsg(msg);
    }

    // IMediaControl
    virtual AM_ERR SetAppMsgSink(IMsgSink *pAppMsgSink)
    {
        return inherited::SetAppMsgSink(pAppMsgSink);
    }
    virtual AM_ERR SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&), void *context)
    {
        return inherited::SetAppMsgCallback(MsgProc, context);
    }
public:
    virtual AM_ERR EditSourceDone();
    virtual AM_ERR EditSource(AM_INT sourceGroup, const char* pFileName, AM_INT flag);
    virtual AM_ERR BackSource(AM_INT sourceGroup, AM_INT orderLevel, AM_INT flag);
    virtual AM_ERR DeleteSource(AM_INT sourceGroup, AM_INT flag);

    //added for TsOverUdp/Rtp as inputSource
    virtual AM_ERR SaveInputSource(char *streamName,char * inputAddress,int inputPort,int isRawUdp);
    virtual AM_ERR StopInputSource(char *streamName);
    //end

    //added for forward audio, for bi-direction audio feature
    virtual AM_ERR StartAudioProxy(const char *rtspUrl, const char* saveName);
    virtual AM_ERR StopAudioProxy(const char *rtspUrl);
    //end

    virtual AM_ERR SaveSource(AM_INT index, const char* saveName, AM_INT flag);
    virtual AM_ERR StopSaveSource(AM_INT index, AM_INT flag);
    virtual AM_ERR AddSource(const char *pFileName, AM_INT sourceGroup, AM_INT flag);
    virtual AM_ERR SelectAudio(AM_INT index);
    virtual AM_ERR TalkBack(AM_UINT enable, AM_UINT winIndexMask, const char* url);
    virtual AM_ERR ConfigWindow(AM_INT winIndex, CParam& param);
    virtual AM_ERR ConfigWinMap(AM_INT sourceIndex, AM_INT win, AM_INT zOrder);
    virtual AM_ERR PerformWinConfig();
    virtual AM_ERR AutoSwitch(AM_INT index);
    virtual AM_ERR SwitchSet(AM_INT index);

    virtual AM_ERR UpdatePBSpeed(AM_INT target, AM_U8 pb_speed, AM_U8 pb_speed_frac, IParameters::DecoderFeedingRule feeding_rule, AM_UINT flush = 1);
    virtual AM_ERR PlaybackZoom(AM_INT index, AM_U16 w, AM_U16 h, AM_U16 x, AM_U16 y);
    virtual AM_ERR GetStreamInfo(AM_INT index, AM_INT* width, AM_INT* height);

    virtual AM_ERR Prepare();
    virtual AM_ERR Start();
    virtual AM_ERR Stop();
    virtual AM_ERR DisconnectHw(AM_INT flag);
    virtual AM_ERR ReconnectHw(AM_INT flag);

    virtual AM_ERR SetTranscode(AM_UINT w, AM_UINT h, AM_UINT br);
    virtual AM_ERR SetTranscodeBitrate(AM_INT kbps);
    virtual AM_ERR SetTranscodeFramerate(AM_INT fps, AM_INT reduction);
    virtual AM_ERR SetTranscodeGOP(AM_INT M, AM_INT N, AM_INT interval, AM_INT structure);
    virtual AM_ERR TranscodeDemandIDR(AM_BOOL now);
    virtual AM_ERR RecordToFile(AM_BOOL en, char* name);

    virtual AM_ERR UploadNVRTranscode2Cloud(char* url, AM_INT flag);
    virtual AM_ERR StopUploadNVRTranscode2Cloud(AM_INT flag);

    //next stage:tricplay
    virtual AM_ERR PausePlay(AM_INT index);
    virtual AM_ERR ResumePlay(AM_INT index);
    virtual AM_ERR SeekTo(AM_U64 time, AM_INT index);
    virtual AM_ERR StepPlay(AM_INT index);

    virtual AM_ERR GetMdecInfo(MdecInfo& info);
    //virtual void setLoop(AM_INT loop) = 0;
    virtual AM_ERR Dump(AM_INT flag = 0);
    virtual AM_ERR Test(AM_INT flag);

    virtual AM_ERR SetNvrPlayBackMode(AM_INT mode);
    virtual AM_ERR ChangeSavingTimeDuration(AM_INT duration);

    virtual AM_ERR ReLayout(AM_INT layout, AM_INT winIndex);
    virtual AM_ERR ExchangeWindow(AM_INT winIndex0, AM_INT winIndex1);
    virtual AM_ERR ConfigRender(AM_INT render, AM_INT window, AM_INT dsp);
    virtual AM_ERR ConfigRenderDone();
    virtual AM_ERR ConfigTargetWindow(AM_INT target, CParam& param);

    virtual AM_ERR CreateMotionDetectReceiver();
    virtual AM_ERR RemoveMotionDetectReceiver();
    virtual void MotionDetecthandler(const char* ip,void* event);

    virtual AM_ERR ConfigLoader(AM_BOOL en, char* path, char* m3u8name, char* host, AM_INT count, AM_UINT duration);

private:
    CActiveMDecEngine(void *audiosink);
    AM_ERR Construct(CParam& par);
    virtual ~CActiveMDecEngine();
    AM_ERR SetGConfig(CParam& par);
    void OnRun();

private:
    void SetState(MDENGINE_STATE state);
    AM_ERR InitDspAudio(CParam& par);
    AM_ERR DeInitDspAudio();
    AM_ERR CheckActionByLayout(ACTION_TYPE type, AM_INT& index);

    AM_INT InfoChooseIndex(AM_BOOL isNet, AM_INT flag, AM_INT order = 0);
    AM_INT InfoChangeWinIndex(AM_INT winIndex);
    AM_INT InfoFindHDIndex(AM_INT sdIndex);
    AM_ERR InfoPauseAllSmall();
    AM_ERR InfoResumeAllSmall();
    AM_ERR InfoHideAllSmall(AM_BOOL hided);

    AM_INT InfoSDByWindow(AM_INT winIndex);
    AM_INT InfoHDByWindow(AM_INT winIndex);
    AM_INT InfoHDBySD(AM_INT sdIndex);
    AM_INT InfoSDByHD(AM_INT hdIndex);
    AM_INT InfoFindIndexBySource(AM_UINT ip);

    AM_ERR DoAutoChooseAudioStream();
    AM_ERR DoInterPausePlay();
    AM_ERR DoResumeAudioStream(AM_INT aIndex = -1);
    AM_ERR DoPauseAudioStream(AM_INT aIndex = -1);
    AM_ERR DoRemoveAudioStream(AM_INT aIndex = -1);
    AM_ERR DoStartSpecifyAudio(AM_INT index, AM_INT flag = SYNC_AUDIO_LASTPTS);
    AM_ERR DoFlushAllStream();
    AM_ERR DoSwitchAudioLocal(AM_INT sIndex, AM_INT hdIndex);
    AM_ERR DoSwitchBackAudioLocal();
    AM_ERR DoSwitchAudioNet(AM_INT sIndex, AM_INT hdIndex);
    AM_ERR DoSwitchBackAudioNet();
    AM_ERR DoFlushSpecifyDspMw(AM_INT index, AM_U8 flag= 0);
    AM_ERR DoPauseSmallDspMw(AM_INT action);
    AM_ERR DoResumeSmallDspMw(AM_INT action);
    AM_ERR DoFlushSmallDspMw();

    AM_BOOL DoDetectSeamless(AM_INT sdIndex, AM_INT hdIndex);
    AM_ERR DoSyncSDStreamBySource(AM_INT source);
    AM_ERR DoDspSwitchForSource(AM_INT oriSource, AM_INT destSource, AM_BOOL seamless);
    AM_ERR DoFindStreamIndex(AM_INT sourceGroup, AM_INT& sIndex, AM_INT& hdIndex);

    AM_ERR DoCreatePipeLine(AM_INT streamIndex);

    AM_ERR InitSourceConfig(AM_INT source, AM_INT flag, AM_BOOL isNet);
    AM_ERR CheckBeforeAddSource(AM_INT source, AM_INT flag);

    AM_ERR HandleEditSourceDone();
    AM_ERR HandleEditSource(AM_INT source, AM_INT flag);
    AM_ERR HandleBackSource(AM_INT sourceGroup, AM_INT orderLevel, AM_INT flag);
    AM_ERR HandleDeleteSource(AM_INT sourceGroup, AM_INT flag);
    AM_ERR HandleAddSource(AM_INT source, AM_INT flag);
    AM_ERR HandleSelectAudio(AM_INT index);
    AM_ERR HandleTalkBack(AM_INT enable, AM_UINT winIndexMask);
    AM_ERR HandleConfigWindow(AM_INT index, CParam* pParam);
    AM_ERR HandleConfigWinMap(AM_INT sourceIndex, AM_INT win, AM_INT zOrder);
    AM_ERR HandlePerformWinConfig();
    AM_ERR HandlePrepare();
    AM_ERR HandleStop();
    AM_ERR HandleSeekTo(AM_U64 time, AM_INT index);
    AM_ERR HandleSpeedFeedingRule(AM_INT target, AM_UINT speed, IParameters::DecoderFeedingRule feeding_rule, AM_UINT flush);

    AM_ERR HandleDisconnectHw();
    AM_ERR HandleReconnectHw();

    AM_ERR HandleStart(AM_BOOL isNet);

    AM_ERR HandleStep(AM_INT winIndex);
    AM_ERR HandleStepSdLocal(AM_INT winIndex);
    AM_ERR HandleStepHdLocal();
    AM_ERR HandleStepSdNet(AM_INT winIndex);
    AM_ERR HandleStepHdNet();

    AM_ERR HandlePause(AM_INT index);
    AM_ERR HandleResume(AM_INT index);
    AM_ERR HandlePauseHdLocal();
    AM_ERR HandlePauseSdLocal(AM_INT winIndex);
    AM_ERR HandleResumeHdLocal();
    AM_ERR HandleResumeSdLocal(AM_INT winIndex);
    AM_ERR HandlePauseHdNet();
    AM_ERR HandlePauseSdNet(AM_INT winIndex);
    AM_ERR HandleResumeHdNet();
    AM_ERR HandleResumeSdNet(AM_INT winIndex);
    AM_ERR HandlePauseAll();
    AM_ERR HandleResumeAll();

    AM_ERR HandleRevertNVR();
    AM_ERR HandleAutoSwitch(AM_INT index);
    AM_ERR HandleBackNoHD();
    AM_ERR HandleSwitchNoHD(AM_INT winIndex);
    AM_ERR HandleBackByArm();
    AM_ERR HandleSwitchStream(AM_INT winIndex);
    AM_ERR HandleSwitchBack();
    AM_ERR HandleSwitchDuringHd(AM_INT winIndex);
    AM_ERR HandleSwitchSet(AM_INT setIndex);

    AM_ERR HandleReLayout(LAYOUT_TYPE layout, AM_INT index=0);
    AM_ERR HandleReLayoutNoHD(LAYOUT_TYPE layout, AM_INT index=0);
    AM_ERR HandleLayoutTable(LAYOUT_TYPE Type);
    AM_ERR HandleExchangeWindow(AM_INT winIndex0, AM_INT winIndex1);

    AM_ERR HandleLayoutTeleconference(LAYOUT_TYPE Type, AM_INT index=0);
    AM_ERR HandleLayoutSingleHDFullScreen(LAYOUT_TYPE Type, AM_INT index=0);
    AM_ERR HandleLayoutBottomleftHighlightenMode(LAYOUT_TYPE type, AM_INT index=0);

    AM_ERR HandleConfigTargetWindow(AM_INT target, CParam* pParam);

    AM_ERR ActiveStream(AM_INT streamIndex, bool activeAudio=true);
    AM_ERR InactiveStream(AM_INT streamIndex);
    AM_ERR PauseStream(AM_INT streamIndex);
    AM_ERR ResumeStream(AM_INT streamIndex);
    AM_ERR FlushStream(AM_INT streamIndex);

    // [foreground]
    // corresponding demux thread starts sending data downstream
    //
    // [background]
    // corresponding demux thread stops sending data downstream except recording functionality
    AM_ERR ForegroundStream(AM_INT streamIndex, bool activeAudio=true);
    AM_ERR BackgroundStream(AM_INT streamIndex);

    AM_ERR ActiveSDStreams();
    AM_ERR InactiveSDStreams();
    AM_ERR PauseSDStreams();
    AM_ERR ResumeSDStreams();
    AM_ERR FlushSDStreams();
    AM_ERR ForegroundSDStreams();
    AM_ERR BackgroundSDStreams();

    AM_ERR SwitchAudio(AM_INT audioSrcIndex, AM_INT audioTargetIndex);

    AM_ERR ReconfigWindowRenderLayoutTeleconference();
    AM_ERR ReconfigWindowRenderLayoutTable();

    inline AM_BOOL IsShowOnLCD();

private:
    AM_ERR BuildMDecGragh();
    bool ProcessGenericCmd(CMD& cmd);
    bool ProcessGenericMsg(AM_MSG& msg);
    void ProcessCMD(CMD& oricmd);

    void HandleReadyMsg(AM_MSG& msg);
    void HandleEOSMsg(AM_MSG& msg);
    void HandleSyncMsg(AM_MSG& msg);
    void HandleForceSyncMsg(AM_MSG& msg);
    void HandleErrorMsg(AM_MSG& msg);

    AM_ERR HandleMsgDspShow(AM_MSG& msg);
    AM_ERR HandleMsgRenderSync(AM_MSG& msg);
    AM_ERR HandleMsgRenderSyncDynamic(AM_MSG& msg);
    AM_ERR HandleMsgUdecError(AM_MSG& msg);
    AM_ERR HandleMsgAddAudio(AM_MSG& msg);
    AM_ERR HandleMsgEnd(AM_MSG& msg);

    AM_ERR UpdateNvrOsdInfo();

    AM_ERR UpdateUdecConfigInfo();
private:
    AM_INT DEC_MAP(AM_INT index);
    AM_INT REN_MAP(AM_INT index);
    AM_INT DSP_MAP(AM_INT index);
private:
    bool mbRun;
    AM_UINT mFatalErrorCnt;
    const char* mpCurSource;
private:
    //IMP DATA STRUCT.
    CGConfig* mpConfig;

    CGeneralDemuxer* mpDemuxer;
    CGeneralDecoder* mpVideoDecoder;
    CGeneralDecoder* mpAudioDecoder;
    CGeneralRenderer* mpRenderer;
    CGeneralTranscoder* mpTranscoder;
    CGeneralTransAudioSink* mpTansAudioSink;
    CInterActiveFilter* mpAudioSinkFilter;

    CGeneralMuxer* mpMuxer;
    CGeneralAudioManager* mpAudioMan;
    CGeneralLayoutManager* mpLayout;
    IPipeLineManager* mpPipeLine;//...

    MdecInfo* mpMdecInfo;
private:
    CMsgSys* mpMsgSys;

    IClockManager *mpClockManager;
    void *mpAudioSink;

    void* mpMDReceiver;

    AM_INT mState;
    AM_INT mEditPlayAudio;
    AM_INT mNvrAudioIndex;
    AM_INT mHdInfoIndex;
    AM_INT mSdInfoIndex;
    AM_BOOL mbNohd;
    AM_BOOL mbOnNVR;
    AM_BOOL mbNetStream;
    AM_BOOL mbDisconnect;
    AM_INT mUdecNum;

    //Add for other switch
    SWITCH_WAIT mSwitchWait;
    ADUIO_WAIT mAudioWait;
    AM_INT mBufferNum;

    AM_INT mNvrPbMode;

    AM_BOOL mbSeeking;

    char mTalkBackURL[128];
    char mTalkBackName[32];
    AM_BOOL mbTalkBackStreamOn;
    AM_UINT mTalkBackIndexMask;
    //transcode
    AM_BOOL mbEnableTanscode;
    AM_UINT mEncWidth;
    AM_UINT mEncHeight;
    AM_UINT mEncBitRate;

    AM_INT mStartIndex;//use to get every stream's info before RunAllFilter
    AM_U8 mbTrickPlayDisabledAudio;
    AM_U8 mbRTSPServerStarted;
    AM_U8 mReserved[2];
/*
    AM_INT StreamNums;


    IFilter* mpMasterFilter;
    AM_UINT mMostPriority;

    AM_U64 mSeekTime[MAX_VIDEO_NUMS];
    AM_U64 mDuration[MAX_VIDEO_NUMS];
    AM_U64 mCurTime[MAX_VIDEO_NUMS];

    // indicates whether the method of absolute or relative time is used when get current time
    AM_BOOL mbUseAbsoluteTime;
    SConsistentConfig mShared;
    // Capability of decoder type
    AM_UINT mdecCap[CODEC_NUM];
*/
    //1 TODO!
    SharedResource mShared;
private:
    //added for TsOverUdp/Rtp as InputSource
#if PLATFORM_LINUX
    pthread_mutex_t ts_mutex_;
    void *hash_table_;
#endif
    //added for audioProxy
#if PLATFORM_LINUX
    pthread_mutex_t audio_mutex_;
    void *hash_table_audio_;
#endif
};

#endif

