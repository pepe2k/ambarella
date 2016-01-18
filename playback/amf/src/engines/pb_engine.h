
/**
 * pb_engine.h
 *
 * History:
 *    2009/12/16 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __PB_ENGINE_H__
#define __PB_ENGINE_H__

#define MAXPATHLENGTH 260
class CMediaRecognizer;


//-----------------------------------------------------------------------
//
// CPBEngine
//
//-----------------------------------------------------------------------
class CPBEngine: public CBaseEngine, public IPBEngine, public IPBControl
{
    typedef CBaseEngine inherited;

public:
	static CPBEngine* Create(void *audiosink);

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete();

	// IEngine
	virtual AM_ERR PostEngineMsg(AM_MSG& msg)
	{
		return inherited::PostEngineMsg(msg);
	}

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
            if ((DSPMode_UDEC== dspMode) || (DSPMode_DuplexLowdelay == dspMode)) {
                AMLOG_INFO("active-pb-engine, set mode %d, vout mask %d.\n", dspMode, voutMask);
                mShared.encoding_mode_config.dsp_mode = dspMode;
                return ME_OK;
            } else {
                AM_ERROR("app try set BAD DSP mode %d, vout mask %d.\n", dspMode, voutMask);
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
    virtual AM_ERR StartwithPlayVideoESMode(AM_INT also_demuxfile, AM_INT param_0, AM_INT param_1) {return ME_NOT_SUPPORTED;}
    virtual AM_ERR SpecifyVideoFrameRate(AM_UINT framerate_num, AM_UINT framerate_den) {return ME_NOT_SUPPORTED;}
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

    virtual AM_ERR EnableDewarpFeature(AM_UINT enable, AM_UINT dewarp_param) {return ME_NO_IMPL;}
    virtual AM_ERR SetDeWarpControlWidth(AM_UINT enable, AM_UINT width_top, AM_UINT width_bottom) {return ME_NO_IMPL;}

    //source/dest rect and scale mode config
    virtual AM_ERR SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h);
    virtual AM_ERR SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h);
    virtual AM_ERR SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y);
    virtual AM_ERR SetLooping(int loop);
	//debug only
#ifdef AM_DEBUG
    virtual AM_ERR SetLogConfig(AM_UINT index, AM_UINT level, AM_UINT option);
    virtual void PrintState();
#endif
    /*A5S DV Playback Test */
    virtual AM_ERR  A5SPlayMode(AM_UINT mode) {return ME_NOT_SUPPORTED;}
    virtual AM_ERR  A5SPlayNM(AM_INT start_n, AM_INT end_m){return ME_NOT_SUPPORTED;}
    /*end A5S DV Test*/

    virtual AM_ERR SetPBProperty(AM_UINT prop, AM_UINT value);

private:
    virtual void MsgProc(AM_MSG& msg);

private:
	CPBEngine(void *audiosink);
	AM_ERR Construct();
	virtual ~CPBEngine();

private:
	AM_ERR CreateRecognizer();
	void EnterErrorState(AM_ERR err);
	AM_ERR RenderFilter(IFilter *pFilter);
	void SetState(PB_STATE state)
	{
		AM_INFO("Enter state %d\n", state);
		mState = state;
	}

    void PauseAllFilters();
    void ResumeAllFilters();

private:
    void HandleParseMediaDone();
    void HandleReadyMsg(AM_MSG& msg);
    void HandleEOSMsg(AM_MSG& msg);
    void HandleSyncMsg(AM_MSG& msg);
    void HandleSourceFilterBlockedMsg(AM_MSG& msg);
    void HandleForceSyncMsg(AM_MSG& msg);
    void HandleErrorMsg(AM_MSG& msg);

    void CheckFileType(const char *pFileType);
public:
	void GetDecType(DECSETTING decSetDefault[]);
	void SetDecType(DECSETTING decSetCur[]);
    void Reset();

    //need reset when a playitem is finished
private:
    AM_UINT mFatalErrorCnt;
    AM_UINT mTrySeekTimeGap;

private:
	PB_STATE	mState;
	PB_DIR		mDir;
	PB_SPEED	mSpeed;
	AM_ERR		mError;

	char		mFileName[MAXPATHLENGTH];
	CMediaRecognizer	*mpRecognizer;
	IDemuxer	*mpDemuxer;
	IRender *mpMasterRenderer;
	IVideoOutput* mpVoutController;

	IClockManager	*mpClockManager;
	void *mpAudioSink;
	IFilter*		mpMasterFilter;
	AM_UINT mMostPriority;

	AM_U64 mSeekTime;
	AM_INT mLoop;
    // indicates whether the method of absolute or relative time is used when get current time
    AM_BOOL mbUseAbsoluteTime;

    SConsistentConfig mShared;
    // Capability of decoder type
    AM_UINT mdecCap[CODEC_NUM];

    bool mbNeedDisableHdmiOSD;
};

#endif

