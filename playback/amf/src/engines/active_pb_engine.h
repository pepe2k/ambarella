
/**
 * active_pb_engine.h
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

#ifndef __PB_ENGINE_H__
#define __PB_ENGINE_H__

#define MAXPATHLENGTH 280
class CMediaRecognizer;

//-----------------------------------------------------------------------
//
// CActivePBEngine
//
//-----------------------------------------------------------------------
class CActivePBEngine: public CActiveEngine, public IPBEngine, public IPBControl, public IDataRetriever
{
    typedef CActiveEngine inherited;
public:

    //playback defined cmd/msg
    enum {
        PB_FIRST_CMD = IActiveObject::CMD_LAST,
        PBCMD_PREPARE,
        PBCMD_ADD_PLAYITEM,//for playlist
        PBCMD_REMOVE_PLAYITEM,//for playlist
        PBCMD_PLAY_NEXTITEM,//for playlist
        PBCMD_START_PLAY,
        PBCMD_STOP_PLAY,
        PBCMD_PAUSE_RESUME,
        PBCMD_TRICK_PLAY,
        PBCMD_STEP_PLAY,
        PBCMD_SEEK,
        PBCMD_SET_LOOPMODE,//0 no loop, 1 loop with order, 2, randomly select next playitem
        PBCMD_SETVOUT_POS,
        PBCMD_SETVOUT_SIZE,
        PBCMD_SETVOUT_ROTATION,
        PBCMD_SETVOUT_FLIP,
        PBCMD_SETVOUT_MIRROR,
        PBCMD_SETVOUT,//all vout parameters
        PBCMD_ENABLEVOUT,
        PBCMD_GETVOUT_INFO,
        PB_LAST_CMD,
    };
    enum PlayMode
    {
        NORMAL_PLAY,
        FF_2X_SPEED,
        FF_4X_SPEED,
        FF_8X_SPEED,
        FF_16X_SPEED,
        FF_30X_SPEED,
        BACKWARD_PLAY,
        FB_1X_SPEED,
        FB_2X_SPEED,
        FB_4X_SPEED,
        FB_8X_SPEED,
        PLAY_N_TO_M,
    };

public:
    static CActivePBEngine* Create(void *audiosink);

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();

    //IIEngine
    virtual void *QueryEngineInterface(AM_REFIID refiid);

    // IMediaControl
    virtual AM_ERR SetAppMsgSink(IMsgSink *pAppMsgSink)
    {
        return inherited::SetAppMsgSink(pAppMsgSink);
    }
    virtual AM_ERR SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&), void *context)
    {
        return inherited::SetAppMsgCallback(MsgProc, context);
    }

    // IPBControl
    virtual AM_ERR SetWorkingMode(AM_UINT dspMode, AM_UINT voutMask)
    {
        AMLOG_INFO("active-pb-engine, set mode %d vout mask %d.\n", dspMode, voutMask);
        mRequestVoutMask = voutMask;
        if ((DSPMode_UDEC== dspMode) || (DSPMode_DuplexLowdelay == dspMode)) {
            mShared.encoding_mode_config.dsp_mode = dspMode;
            return ME_OK;
        } else {
            AM_ERROR("app try set BAD DSP mode %d.\n", dspMode);
            return ME_BAD_PARAM;
        }
    }
    virtual AM_ERR PlayFile(const char *pFileName);
    virtual AM_ERR PrepareFile(const char *pFileName);

    virtual AM_ERR StopPlay();
    virtual AM_ERR GetPBInfo(PBINFO& info);

    virtual AM_ERR PausePlay();
    virtual AM_ERR ResumePlay();

    virtual AM_ERR Loopplay(AM_U32 loop_mode);

    virtual AM_ERR Seek(AM_U64 ms);
    virtual AM_ERR Step();
    virtual AM_ERR StartwithStepMode(AM_UINT startCnt);
    virtual AM_ERR StartwithPlayVideoESMode(AM_INT also_demuxfile, AM_INT param_0, AM_INT param_1);
    virtual AM_ERR SpecifyVideoFrameRate(AM_UINT framerate_num, AM_UINT framerate_den);
    virtual AM_ERR SetTrickMode(PB_DIR dir, PB_SPEED speed);

    virtual AM_ERR SetDeinterlaceMode(AM_INT bEnable);
    virtual AM_ERR GetDeinterlaceMode(AM_INT *pbEnable);

    //vout config
    virtual AM_ERR ChangeInputCenter(AM_INT input_center_x, AM_INT input_center_y);
    virtual AM_ERR SetDisplayPositionSize(AM_INT vout, AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height);
    virtual AM_ERR GetDisplayPositionSize(AM_INT vout, AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height);
    virtual AM_ERR SetDisplayPosition(AM_INT vout, AM_INT pos_x, AM_INT pos_y);
    virtual AM_ERR SetDisplaySize(AM_INT vout, AM_INT width, AM_INT height);
    virtual AM_ERR GetDisplayDimension(AM_INT vout, AM_INT* width, AM_INT* height);
    virtual AM_ERR GetVideoPictureSize(AM_INT* width, AM_INT* height);
    virtual AM_ERR GetCurrentVideoPictureSize(AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height);
    virtual AM_ERR VideoDisplayZoom(AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height);
    virtual AM_ERR SetDisplayFlip(AM_INT vout, AM_INT flip);
    virtual AM_ERR SetDisplayMirror(AM_INT vout, AM_INT mirror);
    virtual AM_ERR SetDisplayRotation(AM_INT vout, AM_INT degree);
    virtual AM_ERR EnableVout(AM_INT vout, AM_INT enable);
    virtual AM_ERR EnableOSD(AM_INT vout, AM_INT enable);
    virtual AM_ERR EnableVoutAAR(AM_INT enable);

    virtual AM_ERR EnableDewarpFeature(AM_UINT enable, AM_UINT dewarp_param);
    virtual AM_ERR SetDeWarpControlWidth(AM_UINT enable, AM_UINT width_top, AM_UINT width_bottom);

    //source/dest rect and scale mode config
    virtual AM_ERR SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h);
    virtual AM_ERR SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h);
    virtual AM_ERR SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y);
    virtual AM_ERR SetLooping(int loop);

    //IDataRetriever
    virtual AM_ERR RetieveData(AM_U8* pdata, AM_UINT max_len, IParameters::DataCategory data_category = IParameters::DataCategory_PrivateData);
    virtual AM_ERR RetieveDataByType(AM_U8* pdata, AM_UINT max_len, AM_U16 type, AM_U16 sub_type, IParameters::DataCategory data_category = IParameters::DataCategory_PrivateData);
    virtual AM_ERR SetDataRetrieverCallBack(void* cookie, data_retriever_callback_t callback) {AUTO_LOCK(mpMutex); mPrivateDataCallbackCookie = cookie; mPrivatedataCallback = callback; return ME_OK;}

    //debug only
#ifdef AM_DEBUG
    virtual AM_ERR SetLogConfig(AM_UINT index, AM_UINT level, AM_UINT option);
    virtual void PrintState();
#endif
    /*A5S DV Playback Test */
    virtual AM_ERR  A5SPlayMode(AM_UINT mode);
    virtual AM_ERR  A5SPlayNM(AM_INT start_n, AM_INT end_m);
    /*end A5S DV Test*/
    virtual AM_ERR SetPBProperty(AM_UINT prop, AM_UINT value);

private:
    CActivePBEngine(void *audiosink);
    AM_ERR Construct();
    virtual ~CActivePBEngine();

private:
    AM_ERR CreateRecognizer();
    void EnterErrorState(AM_ERR err);
    AM_ERR RenderFilter(IFilter *pFilter);
    void SetState(PB_STATE state)
    {
        AMLOG_INFO("Enter state %d\n", state);
        PB_STATE ostate = mState;
        mState = state;
        if (ostate != mState) {
            PostAppMsg(IMediaControl :: MSG_STATE_CHANGED);
        }
    }

private:
    void ClearCurrentSession();
    bool isCmdQueueFull();
    AM_ERR HandleSeek(AM_U64 ms);
    void HandleReadyMsg(AM_MSG& msg);
    void HandleEOSMsg(AM_MSG& msg);
    void HandleSyncMsg(AM_MSG& msg);
    void HandleNotifyUDECIsRunning();
    void HandleErrorMsg(AM_MSG& msg);
    void HandleSourceFilterBlockedMsg(AM_MSG& msg);
    void HandleSwitchMasterRenderMsg(AM_MSG& msg);//for bug1294
    void CheckFileType(const char *pFileType);

    void checkDupexParameters(void);

public:
    void GetDecType(DECSETTING decSetDefault[]);
    void SetDecType(DECSETTING decSetCur[]);
    void ResetParameters();

public:
    void OnRun();

private:
    bool TryPlayNextItem();
    AM_ERR BuildFilterGragh(AM_PlayItem* item);
    bool ProcessGenericCmd(CMD& cmd);
    bool ProcessGenericMsg(AM_MSG& msg);
    void ProcessCMD(CMD& oricmd);
    void PurgeMsgInQueue();

    //playlist related
    AM_PlayItem* RequestNewPlayItem()
    {
        AM_PlayItem* ret = NULL;
        AUTO_LOCK(mpMutex);
        if (mpFreeList) {
            AM_ASSERT(mnFreedCnt);
            ret = mpFreeList;
            mpFreeList = mpFreeList->pNext;
            mnFreedCnt--;
        } else {
            ret = AM_NewPlayItem();
        }
        ret->pNext = ret->pPre = NULL;
        ret->CntOrTag = 0;
        ret->pSourceFilter = NULL;
        ret->pSourceFilterContext = NULL;
        ret->type = PlayItemType_None;
        return ret;
    }

    void ReleasePlayItem(AM_PlayItem* item)
    {
        AM_ASSERT(item);
        if (!item) {
            return;
        }

        AUTO_LOCK(mpMutex);
        if (mpFreeList) {
            item->pNext = mpFreeList;
            mpFreeList = item;
        }
        mnFreedCnt++;
        mpFreeList = item;
    }

    //append to end
    void AppendPlayItem(AM_PlayItem* header, AM_PlayItem* item)
    {
        AUTO_LOCK(mpMutex);
        AM_InsertPlayItem(header, item, 0);
        header->CntOrTag ++;
    }

    //move out
    void GetoutPlayItem(AM_PlayItem* header, AM_PlayItem* item)
    {
        AUTO_LOCK(mpMutex);

#ifdef AM_DEBUG
        //check the item is in list, for safe
        AM_CheckItemInList(header, item);
#endif

        AM_RemovePlayItem(item);
        header->CntOrTag --;
    }

    AM_PlayItem* FindNextPlayItem(AM_PlayItem* current)
    {
        AM_PlayItem* ret = NULL;
        AUTO_LOCK(mpMutex);

        //find first play item
        if (!current) {
            if (mPlayListHead.pNext != &mPlayListHead) {
                ret = mPlayListHead.pNext;
                return ret;
            } else {
                return NULL;
            }
        }

#ifdef AM_DEBUG
        //check the item is in list, for safe
        AM_CheckItemInList(&mPlayListHead, current);
#endif
        if (!mRandonSelectNextItem) {
            if (current->pNext != &mPlayListHead) {
                ret = current->pNext;
            } else {
                ret = mPlayListHead.pNext;
            }
        } else {
            //implement random select, todo
            AMLOG_ERROR("need implement here.\n");
            ret = current;
        }

        //not equal header
        if (ret == &mPlayListHead) {
            AMLOG_PRINTF("**no more items yet.\n");
            return NULL;
        }
        return ret;
    }

private:
    void StopEngine(AM_UINT stop_flag);
    void SetMasterRender(const char *pFilterName);

private:
    bool mbRun;
    bool mbPaused;
//    bool mbCurrentItemPrepared;//ready to play current item
    AM_INT mLoopPlayList;
    AM_INT mLoopCurrentItem;
    AM_INT mRandonSelectNextItem;

    AM_INT mNeedReConfig;

    AM_PlayItem* mpCurrentItem;
    AM_PlayItem mPlayListHead;//double-lincked list
    AM_PlayItem mToBePlayedItem;//double-lincked list
    AM_PlayItem* mpFreeList;//single-lincked list
    AM_UINT mnFreedCnt;

    //debug use
    bool mbSeeking;//seek is not finished yet

//all filters pointer
private:
    IFilter* mpFilterDemuxer;
    IFilter* mpFilterAudioDecoder;
    IFilter* mpFilterVideoDecoder;
    IFilter* mpFilterAudioRenderer;
    IFilter* mpFilterVideoRenderer;
    IFilter* mpFilterAudioEffecter;
    IFilter* mpFilterVideoEffecter;

//need reset when a playitem is finished
private:
    AM_UINT mFatalErrorCnt;
    AM_UINT mTrySeekTimeGap;

private:
    PB_STATE	mState;
    PB_DIR		mDir;
    PB_SPEED	mSpeed;

    CMediaRecognizer	*mpRecognizer;
    IDemuxer	*mpDemuxer;
    IRender *mpMasterRenderer;
    IVideoOutput* mpVoutController;
    IDecoderControl* mpDecodeController;
    IDataRetriever* mpDataRetriever;

    IClockManager	*mpClockManager;
    void *mpAudioSink;
    IFilter *mpMasterFilter;
    AM_UINT mMostPriority;

    AM_U64 mSeekTime;
    AM_U64 mStartTime;//if app does a seek in STATE_PREPARED, save the seek_time
    AM_U64 mCurrentItemTotLength;

    bool mbCurTimeValid;
    AM_U64 mCurTime;//update mCurTime when seek & GetPBInfo. Assure it moves forward

    // indicates whether the method of absolute or relative time is used when get current time
    AM_BOOL mbUseAbsoluteTime;
    SConsistentConfig mShared;
    // Capability of decoder type
    AM_UINT mdecCap[CODEC_NUM];
    /*A5S DV Playback Test */
    AM_UINT mOldMode;
    /*end A5S DV Test*/

private:
    void* mPrivateDataCallbackCookie;
    data_retriever_callback_t mPrivatedataCallback;
    AM_U8* mpPrivateDataBufferForCallback;
    AM_UINT mPrivateDataBufferToTSize;

private:
    AM_UINT mRequestVoutMask;
};

#endif

