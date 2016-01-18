
/**
 * active_ve_engine.h
 *
 * History:
 *    2011/08/15 - [GangLiu] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AVE_ENGINE_H__
#define __AVE_ENGINE_H__

#define MAXPATHLENGTH 280
#define MAX_VIDEO_NUMS 2
class CMediaRecognizer;

//-----------------------------------------------------------------------
//
// CActiveVEEngine
//
//-----------------------------------------------------------------------
class CActiveVEEngine: public CActiveEngine, public IVEEngine, public IVEControl
{
    typedef CActiveEngine inherited;
public:

    //playback defined cmd/msg
    enum {
        VE_FIRST_CMD = IActiveObject::CMD_LAST,
        VECMD_ADD_SOURCE,
        VECMD_PREPARE_SOURCE,
        VECMD_DEL_SOURCE,
        VECMD_SEEK,
        VECMD_INIT_PREVIEW,
        VECMD_START_PREVIEW,//start or resume, start with pause state
        VECMD_RESUME_PREVIEW,
        VECMD_PAUSE_PREVIEW,
        VECMD_STOP_PREVIEW,
        VECMD_START_FINALIZE,
        VECMD_STOP_FINALIZE,
        VECMD_COMPELETE_FINALIZE,
        VECMD_STEP_PLAY,
        VECMD_ENABLEVOUT,
        VECMD_GETVOUT_INFO,
        VE_LAST_CMD,
    };

    enum {
        MSG_VE_FIRST = IEngine::MSG_LAST,
        MSG_VE_xxx,//todo
        MSG_VE_LAST,
    };
public:
    static CActiveVEEngine* Create(void *audiosink);

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();

    //IIEngine
    virtual void *QueryEngineInterface(AM_REFIID refiid);

    // IEngine
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

    // IVEControl
    //project
    virtual AM_ERR CreateProject();
    virtual AM_ERR OpenProject(const char *pProjectName);
    virtual AM_ERR SaveProject();
    virtual AM_BOOL IsSaved(){return true;};
    virtual AM_ERR Stop();
    virtual AM_ERR StopTransc(AM_INT stopcode);

    virtual AM_ERR AddSource(const char *pFileName, VE_TYPE type, AM_UINT &sourceID);
    //virtual AM_ERR PrepareSource(const char *pFileName);
    virtual AM_ERR DelSource(AM_UINT sourceID);
    virtual AM_ERR SeekTo(AM_U64 time);
    virtual AM_ERR GetVEInfo(VE_INFO& info);

    //operate of selected item
    virtual AM_ERR SelectID(AM_UINT sourceID);
    virtual AM_ERR GetSourceInfo(VE_SRC_INFO& src_info);
    virtual AM_ERR SetStartTime(AM_U64 time);
    virtual AM_ERR SplitSource(AM_U64 from, AM_U64 to, VE_TYPE src_type);

    //preview
    virtual AM_ERR AddEffect(VE_Effect_t* effect);
    virtual AM_ERR StartPreview(AM_INT mode);
    virtual AM_ERR StartTranscode();
    virtual AM_ERR ResumePreview();
    virtual AM_ERR PausePreview();
    virtual AM_ERR StopPreview();

    virtual AM_ERR StopDecoder(AM_INT index);

    virtual AM_ERR SetEncParamaters(AM_UINT en_w, AM_UINT en_h, bool keepAspectRatio, AM_UINT fpsBase, AM_UINT bitrate, bool hasBfrm);
    virtual AM_ERR SetTranscSettings(bool RenderHDMI, bool RenderLCD, bool PreviewEnc, bool ctrlfps);
    virtual AM_ERR SetMuxParamaters(const char *filename, AM_UINT cutSize);

    //finalize
    virtual AM_ERR StartFinalize();
    virtual AM_INT GetProcess();

    //test
    virtual void setloop(AM_U8 loop);
    virtual void setTranscFrms(AM_INT frames);
    //vout config, todo

//debug only
#ifdef AM_DEBUG
    virtual AM_ERR SetLogConfig(AM_UINT index, AM_UINT level, AM_UINT option);
    virtual void PrintState();
#endif



private:
    CActiveVEEngine(void *audiosink);
    AM_ERR Construct();
    virtual ~CActiveVEEngine();

private:
    AM_ERR CreateRecognizer(AM_INT index);
    void EnterErrorState(AM_ERR err);
    AM_ERR RenderFilter(IFilter *pFilter);
    void SetState(VE_STATE state)
    {
        AM_INFO("Enter state %d\n", state);
        mState = state;
    }

private:
    AM_ERR HandleSeek(AM_U64 ms);
    void HandleReadyMsg(AM_MSG& msg);
    void HandleEOSMsg(AM_MSG& msg);
    void HandleSyncMsg(AM_MSG& msg);
    void HandleForceSyncMsg(AM_MSG& msg);
    void HandleErrorMsg(AM_MSG& msg);
    void CheckFileType(const char *pFileName);

public:
    void GetDecType(DECSETTING decSetDefault[]);
    void SetDecType(DECSETTING decSetCur[]);
    void ResetParameters();

public:
    void OnRun();

private:
    //for transcode mode
    AM_ERR SetDecPPParamaters(AM_INT index);
    AM_ERR setMuxParam();
    AM_ERR generateHLSPlaylist(AM_INTPTR index ,AM_INTPTR container);

private:
    AM_ERR BuildTest2UdecFilterGragh(AM_INT mode);
    AM_ERR BuildPreviewFilterGragh();
//    AM_ERR DelPreviewFilterGragh();
    AM_ERR BuildFinalizeFilterGragh();
    AM_ERR BuildTranscodeFilterGragh();
    bool ProcessGenericCmd(CMD& cmd);
    bool ProcessGenericMsg(AM_MSG& msg);
    void ProcessCMD(CMD& oricmd);
    AM_ERR setOutputFile();

private:
    bool mbRun;
    bool mbPaused;

//all filters pointer
private:
    CMsgSys* mpMsgSys;
    IFilter* mpFilterDemuxer[MAX_VIDEO_NUMS];
    IFilter* mpFilterVideoDecoder[MAX_VIDEO_NUMS];
    IFilter* mpFilterVideoRenderer[MAX_VIDEO_NUMS];
    IFilter* mpFilterVideoEffecter;
    IFilter* mpFilterVideoTranscoder;
    IFilter* mpFilterVideoMemEncoder;
/*     IFilter* mpFilterAudioDecoder[MAX_VIDEO_NUMS];
    IFilter* mpFilterAudioRenderer[MAX_VIDEO_NUMS];//?
    IFilter* mpFilterVideoEncoder;
    IFilter* mpFilterAudioEncoder;
*/
    IFilter* mpFilterMuxer;

    CMediaRecognizer *mpRecognizer[MAX_VIDEO_NUMS];
    IDemuxer *mpDemuxer[MAX_VIDEO_NUMS];
    IVideoEffecterControl *mpVideoEffecterControl;
    IVideoTranscoder *mpVideoTranscoder;
    IVideoMemEncoder *mpVideoMemEncoder;
    IRender *mpMasterRenderer;
//    IAudioEncoder *mpAudioEncoder;
    IMuxerControl *mpMuxerControl;

private:
    IParameters::UFormatSpecific mUFormatSpecific;

private:
    AM_UINT mFatalErrorCnt;
    AM_UINT mTrySeekTimeGap;

private:
    VE_STATE	mState;
    AM_UINT mCurrentItem;

    IVideoOutput* mpVoutController;

    IClockManager *mpClockManager;
    void *mpAudioSink;
    IFilter* mpMasterFilter;
    AM_UINT mMostPriority;

    AM_U64 mSeekTime;
    AM_U64 mDuration[MAX_VIDEO_NUMS];
    AM_U64 mTimeShaftLength;
    AM_U64 mCurTime;

    AM_U64 mTimeEncoded;

    AM_UINT mEncWidth;
    AM_UINT mEncHeight;
    bool mbEncKeepAspectRatio;
    AM_UINT mBitrate;
    AM_UINT mFpsBase;
    bool mbhasBfrm;
    bool mbRenderHDMI;
    bool mbRenderLCD;
    bool mbPreviewEnc;
    bool mbCtrlFps;
    char outFileName[256];
    AM_UINT mSavingStrategy;//0 for total file

    // indicates whether the method of absolute or relative time is used when get current time
    AM_BOOL mbUseAbsoluteTime;
    SConsistentConfig mShared;
    // Capability of decoder type
    AM_UINT mdecCap[CODEC_NUM];


    CMutex *mpMutex;
    CCondition *mpCondStopfilters;
};

#endif

