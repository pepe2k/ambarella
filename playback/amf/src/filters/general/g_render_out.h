/*
 * g_render_out.h
 *
 * History:
 *    2012/4/5 - [QingXiong Z] created file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __G_RENDERER_OUT_H__
#define __G_RENDERER_OUT_H__

#include "audio_if.h"


enum RENDER_OUT_FLAG
{
    SWITCH_NON_SEAMLESS,
    SWITCH_SEAMLESS,
};

class CVideoRenderOut : public IRenderOut, public CObject
{
    //typedef CActiveObject inherited;
    enum VDSP_STATE{
        VDSP_IDLE,
        VDSP_PAUSED,
        VDSP_STEP,
    };
public:
    static IRenderOut* Create(CGConfig* pConfig, AM_INT index);
    static void Clear() {mpIAV = NULL; mIavFd = -1;}

public:
    // IInterface
    void* GetInterface(AM_REFIID refiid)
    {
        //if (refiid == IID_IRenderOut)
            //return (IRenderOut*)this;
        return NULL;
    }
    virtual void Delete();
    //api
    virtual AM_ERR OpenHal(CGBuffer* handleBuffer);
    virtual AM_ERR PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend);
    virtual AM_ERR RenderOutBuffer(CGBuffer* pBuffer);
private:
    CVideoRenderOut(CGConfig* pConfig, AM_INT index):
        mpGConfig(pConfig),
        mpConfig(NULL),
        mConIndex(index),
        mbEosFlag(AM_FALSE),
        mPTSLow(0),
        mPTSHigh(0),
        mStateCheck(VDSP_IDLE)
    {
        mpConfig = &(mpGConfig->rendererConfig[index]);
    }
    AM_ERR Construct();
    virtual ~CVideoRenderOut();
private:
    AM_ERR DoWaitEOS();
    AM_ERR DoConfigWin();
    AM_ERR DoFlush(IActiveObject::CMD& cmd);
    AM_ERR DoSwitch(IActiveObject::CMD& cmd);
    AM_ERR DoSwitchBack(IActiveObject::CMD& cmd);
    AM_ERR DoPause();
    AM_ERR DoResume();
    AM_ERR DoStep();
    AM_ERR DoPlaybackZoom(IActiveObject::CMD& cmd);

private:
    static IUDECHandler* mpIAV;
    static int mIavFd;
private:
    CGConfig* mpGConfig;
    CRendererConfig* mpConfig;
    AM_INT mConIndex;

private:
    AM_BOOL mbEosFlag;
    AM_U32 mPTSLow;
    AM_U32 mPTSHigh;

    VDSP_STATE mStateCheck;//like resume cannot sent after flush
};


class CAudioRenderOut : public CObject, public IRenderOut
{
    typedef CObject inherited;
    typedef IActiveObject AO;
public:
    static IRenderOut* Create(CGConfig* pConfig, AM_INT index);
    static void Clear() {mpAudioOut = NULL; mbOpened = AM_FALSE;}
    //static void Update() {mpAudioOut = mpGConfig->audioConfig.audioHandle;}
public:
    // IInterface
    void* GetInterface(AM_REFIID refiid)
    {
        //if (refiid == IID_IRenderOut)
            //return (IRenderOut*)this;
        return NULL;
    }
    virtual void Delete();
    //api
    virtual AM_ERR OpenHal(CGBuffer* handleBuffer);
    virtual AM_ERR RenderOutBuffer(CGBuffer* pBuffer);
    virtual AM_ERR PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend);

private:
    CAudioRenderOut(CGConfig* pConfig, AM_INT index):
        mpGConfig(pConfig),
        mpConfig(NULL),
        mConIndex(index),
        flag(0)
    {
        mpConfig = &(mpGConfig->rendererConfig[index]);
        //fw = NULL;
    }
    AM_ERR Construct();
    virtual ~CAudioRenderOut();
private:
    AM_ERR OnStart();
    AM_ERR DoPause();

private:
    //global
    static IAudioHAL* mpAudioOut;
    static AM_BOOL mbOpened;

    CGConfig* mpGConfig;
    CRendererConfig* mpConfig;
    AM_INT mConIndex;
private:
    AM_INT mSampleFormat;
    AM_UINT mBufferSize;
    AM_UINT mSamplerate;
    AM_UINT mChannels;
    AM_UINT mBytePerFrame;

    AM_U64 mSampleTotal;
    AM_UINT mLatency;
    AM_INT flag;

    //debug
    //IFileWriter* fw;

};

#endif
