/*
 * g_sync_renderer.h
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
#ifndef __G_SYNC_RENDERER_H__
#define __G_SYNC_RENDERER_H__

#define G_SYNC_RENDERER_AUDIO_Q GDE_GRE_AUDIO_BP_NUM


//-----------------------------------------------------------------------
//
// CGSyncRenderer
//-----------------------------------------------------------------------
class CGSyncRenderer: public IGRenderer, public CActiveObject
{
    typedef CActiveObject inherited;
    typedef CInterActiveFilter GFilter;
    enum{
        STATE_WAIT_START = LAST_COMMON_STATE,
        STATE_PRE_RENDER,
        STATE_RENDER,
        STATE_HIDED,
    };
    enum RENDER_STATE{
        RENDER_VIDEO,
        RENDER_AUDIO,
    };


public:
    static AM_INT ParseBuffer(const CGBuffer* gBuffer);
    static AM_ERR ClearParse();
    static IGRenderer* Create(IFilter* pFilter,CGConfig* pconfig);

public:
    // IInterface
    void* GetInterface(AM_REFIID refiid)
    {
        //if (refiid == IID_IGRenderer)
            //return (IGDemuxer*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete();

    // IGRenderer
    virtual AM_ERR CheckAnother(CGBuffer* pBuffer);
    virtual AM_ERR PerformCmd(CMD& cmd, AM_BOOL isSend);
    virtual AM_ERR QueryInfo(AM_INT type, CParam& param){ return ME_OK;}
    virtual AM_ERR IsReady(CGBuffer* pBuffer);
    virtual AM_ERR Render(CGBuffer* pBuffer);//need handle data/eos
    virtual AM_ERR FillEOS(CGBuffer* buffer);
    virtual AM_ERR OnReleaseBuffer(CGBuffer* buffer){ return ME_OK;}
    virtual AM_ERR PlaybackZoom(AM_INT index, AM_U16 w, AM_U16 h, AM_U16 x, AM_U16 y);

    virtual void Dump();
private:
    CGSyncRenderer(IFilter* pFilter, CGConfig* pConfig):
        inherited("G_SyncRenderer"),
        mpFilter(pFilter),
        mpGConfig(pConfig),
        mbVideoEnd(AM_FALSE),
        mbAudioEnd(AM_FALSE),
        mbRecEOSV(AM_FALSE),
        mbRecEOSA(AM_FALSE),
        mbEnVideo(AM_TRUE),
        mbEnAudio(AM_TRUE),
        mpVOwner(NULL),
        mpAOwner(NULL),
        mpInputV(NULL),
        mpInputA(NULL),
        mpVideoOut(NULL),
        mpAudioOut(NULL)
    {
        mpConfig = &(pConfig->rendererConfig[mConfigIndex]);
        mConIndex = mConfigIndex;
    }
    AM_ERR Construct();
    void ClearQueue(CQueue* queue);
    virtual ~CGSyncRenderer();

private:
    AM_ERR ProcessCmd(CMD& cmd);
    AM_ERR DoEOS();
    AM_ERR DoDisconnect();
    AM_ERR DoConfigWinRen();
    AM_ERR DoRenderSwitch(AM_INT flag);
    AM_ERR DoRenderSwitchBack(AM_INT flag);
    AM_ERR DoConfig();
    AM_ERR DoFlush(AM_U8 flag);
    AM_ERR DoStop();
    AM_ERR DoPause();
    AM_ERR DoResume();
    AM_ERR DoStep();
    AM_ERR DoRemoveAudio();
    AM_ERR DoAddAudio();
    AM_ERR DoRenderAudio();
    AM_ERR DoRenderVideo();
    void OnRun();

private:
    AM_ERR NotifyInitState();
    AM_ERR InputWaitPolicy();
    AM_ERR BeginRenderOut();
    AM_ERR WaitStreamEnd();
    AM_ERR PostFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);
    AM_ERR SendFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);

private:
    static AM_INT mConfigIndex;
    static CGBuffer mHandleBufferA;
    static CGBuffer mHandleBufferV;

    IFilter* mpFilter;
    CGConfig* mpGConfig;
    CRendererConfig* mpConfig;
    CGBuffer mLocalHandlerA;
    CGBuffer mLocalHandlerV;
    AM_INT mConIndex;

    AM_BOOL mbVideoEnd;
    AM_BOOL mbAudioEnd;
    AM_BOOL mbRecEOSV;
    AM_BOOL mbRecEOSA;
    AM_BOOL mbEnVideo;
    AM_BOOL mbEnAudio;

    AM_UINT mCurVideoPts;
    AM_UINT mCurAudioPts;

    void* mpVOwner;
    void* mpAOwner;
    CQueue* mpInputV;
    CQueue* mpInputA;
    RENDER_STATE mRenderType;
    CGBuffer mBuffer;
private:
    IRenderOut* mpVideoOut;
    IRenderOut* mpAudioOut;

};

#endif
