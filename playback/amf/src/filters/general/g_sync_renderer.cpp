
/*
 * g_sync_renderer.cpp
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

#define LOG_NDEBUG 0
#define LOG_TAG "g_sync_renderer"
//#define AMDROID_DEBUG
#include <unistd.h> //sleep

#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

#include "g_render_out.h"
#include "g_sync_renderer.h"

GRendererParser gSyncRenderer = {
    "Sync-Renderer-G",
    CGSyncRenderer::Create,
    CGSyncRenderer::ParseBuffer,
    CGSyncRenderer::ClearParse,
};

AM_INT CGSyncRenderer::mConfigIndex = 0;
CGBuffer CGSyncRenderer::mHandleBufferA = CGBuffer();
CGBuffer CGSyncRenderer::mHandleBufferV = CGBuffer();
//todo take care when cbuffer(bp) reasele
//-----------------------------------------------------------------------
//
// CGSyncRenderer
//
//-----------------------------------------------------------------------
AM_INT CGSyncRenderer::ParseBuffer(const CGBuffer* gBuffer)
{

    mConfigIndex  = gBuffer->GetIdentity();
    //AM_INFO("xxxxxxxxxxx:%d", mConfigIndex);
    //mHandleBufferA.Dump("ParseBuffer");
    (const_cast<CGBuffer*>(gBuffer))->Dump("Render");
    //AM_INFO("ParseBuffer:%p\n", (const_cast<CGBuffer*>(gBuffer))->GetGBufferPtr());


    if(gBuffer->GetStreamType() == STREAM_AUDIO)
        mHandleBufferA = *gBuffer;

    if(gBuffer->GetStreamType() == STREAM_VIDEO)
        mHandleBufferV = *gBuffer;

    return 90;
}

AM_ERR CGSyncRenderer::ClearParse()
{
    mHandleBufferA.Clear();
    mHandleBufferV.Clear();
    return ME_OK;
}

IGRenderer* CGSyncRenderer::Create(IFilter* pFilter, CGConfig* config)
{
    CGSyncRenderer* result = new CGSyncRenderer(pFilter, config);
    if (result && result->Construct() != ME_OK)
    {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

//-----------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------
AM_ERR CGSyncRenderer::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    if ((mpVideoOut = CVideoRenderOut::Create(mpGConfig, mConIndex)) == NULL)
        return ME_NO_MEMORY;
    if ((mpAudioOut = CAudioRenderOut::Create(mpGConfig, mConIndex)) == NULL)
        return ME_NO_MEMORY;

    //openHal by GR's cmd
    if(mHandleBufferA.GetBufferType() != NOINITED_BUFFER){
        AM_ASSERT(mHandleBufferA.GetBufferType() == HANDLE_BUFFER);
        //err = mpAudioOut->OpenHal(&mHandleBufferA);
        mLocalHandlerA = mHandleBufferA;
        mHandleBufferA.Clear();
        //mLocalHandlerA.Dump("Dump InputA");

        mpInputA = (CQueue* )(mLocalHandlerA.GetExtraPtr());
        CQueue* cmdQ = MsgQ();
        mpInputA->Attach(cmdQ);
        mpAOwner = (void*)(mLocalHandlerA.GetGBufferPtr());
    }
    if(mHandleBufferV.GetBufferType() != NOINITED_BUFFER){
        AM_ASSERT(mHandleBufferV.GetBufferType() == HANDLE_BUFFER);
        //err = mpVideoOut->OpenHal(&mHandleBufferV);
        mLocalHandlerV = mHandleBufferV;
        mHandleBufferV.Clear();
        //AM_ASSERT(mLocalHandlerV.GetExtraPtr() == 0);
    }

    mpWorkQ->SetThreadPrio(1, 1);
    return ME_OK;
}

void CGSyncRenderer::ClearQueue(CQueue* queue)
{
    if(queue == NULL)
        return;

    AM_BOOL rval;
    CGBuffer* buffer;
    while(1)
    {
        rval = queue->PeekData(&buffer, sizeof(CGBuffer*));
        if(rval == AM_FALSE)
        {
            break;
        }
        buffer->Release();
        continue;
    }
}


void CGSyncRenderer::Delete()
{
    AM_INFO("CGSyncRenderer::Delete().\n");
    AM_DELETE(mpVideoOut);
    mpVideoOut = NULL;
    AM_DELETE(mpAudioOut);
    mpAudioOut = NULL;

    if(mpInputA)
        mpInputA->Detach();
    mpInputA = NULL;
    if(mpInputV)
        mpInputV->Detach();
    mpInputV = NULL;

    inherited::Delete();
}

CGSyncRenderer::~CGSyncRenderer()
{
    AM_INFO("~CGSyncRenderer.\n");
    AM_INFO("~CGSyncRenderer done.\n");
}

//-------------------------------------------------------------
//
//
AM_ERR CGSyncRenderer::CheckAnother(CGBuffer* pBuffer)
{
//    AM_ERR err;
    STREAM_TYPE type = pBuffer->GetStreamType();
    if(type == STREAM_AUDIO)
    {
        //AM_ASSERT(mLocalHandlerV.GetBufferType() == HANDLE_BUFFER);
        AM_ASSERT(mLocalHandlerA.GetBufferType() == NOINITED_BUFFER);
        mLocalHandlerA = *pBuffer;

        mpInputA = (CQueue* )(mLocalHandlerA.GetExtraPtr());
        CQueue* cmdQ = MsgQ();
        mpInputA->Attach(cmdQ);
        mpAOwner = (void*)(mLocalHandlerA.GetGBufferPtr());

        if(mState == STATE_WAIT_START){
            PostFilterMsg(GFilter::MSG_SYNC_RENDER_A, 0);
        }else{
            mpWorkQ->SendCmd(CMD_ADD_AUDIO);
        }
        Dump();
        PostFilterMsg(GFilter::MSG_RENDER_ADD_AUDIO, 0);
    }else if(type == STREAM_VIDEO){
        //AM_ASSERT(mLocalHandlerA.GetBufferType() == HANDLE_BUFFER);
        AM_ASSERT(mLocalHandlerV.GetBufferType() == NOINITED_BUFFER);
        mLocalHandlerV = *pBuffer;
        AM_ASSERT(mLocalHandlerV.GetExtraPtr() == 0);
        PostFilterMsg(GFilter::MSG_SYNC_RENDER_V, 0);
    }
    //pBuffer->Release();
    return ME_OK;
}

AM_ERR CGSyncRenderer::IsReady(CGBuffer* pBuffer)
{
    return ME_OK;
}

AM_ERR CGSyncRenderer::PerformCmd(CMD& cmd, AM_BOOL isSend)
{
    AM_ERR err;
    if(isSend == AM_TRUE)
    {
        err = (MsgQ())->SendMsg(&cmd, sizeof(CMD));
    }else{
        err = (MsgQ())->PostMsg(&cmd, sizeof(CMD));
    }
    return err;
}

//CMD FROM GENERAL DEMUXER
AM_ERR CGSyncRenderer::ProcessCmd(CMD& cmd)
{
    AM_ERR err = ME_OK;
    switch (cmd.code)
    {
    case CMD_STOP:
        err = DoStop();
        CmdAck(err);
        break;

    case CMD_PAUSE:
        err = DoPause();
        CmdAck(err);
        break;

    case CMD_RESUME:
        err = DoResume();
        CmdAck(err);
        break;

    case CMD_FLUSH:
        err = DoFlush(cmd.flag);
        CmdAck(err);
        break;

    case CMD_CONFIG:
        err = DoConfig();
        CmdAck(err);
        break;

    case CMD_BEGIN_PLAYBACK:
        BeginRenderOut();
        mState = STATE_PRE_RENDER;
        break;

    case CMD_ADD_AUDIO:
        err = DoAddAudio();
        CmdAck(err);
        break;

    case CMD_EOS:
        DoEOS();
        AM_INFO("CMD_EOS[%d, %d], state: %d\n", mConIndex, mConfigIndex, mState);
        CmdAck(ME_OK);
        break;

    case CMD_PLAYBACK_ZOOM:
        CmdAck(ME_OK);
        break;

    case CMD_DISCONNECT_HW:
        DoDisconnect();
        CmdAck(ME_OK);
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }

    return ME_OK;
}

AM_ERR CGSyncRenderer::DoEOS()
{
    if(mState == STATE_EOS)
        return ME_CLOSED;

    if(mbRecEOSA == AM_TRUE || mpGConfig->demuxerConfig[mConIndex].disableAudio == AM_TRUE)
        mState = STATE_WAIT_EOS;
    return ME_OK;
}

AM_ERR CGSyncRenderer::DoDisconnect()
{
    if(mState == STATE_EOS)
        return ME_CLOSED;
    mState = STATE_WAIT_START;
    mLocalHandlerA.Clear();
    mLocalHandlerA.Clear();
    return ME_OK;
}

AM_ERR CGSyncRenderer::DoConfig()
{
    if(mState == STATE_EOS)
        return ME_CLOSED;

    AM_ERR err = ME_OK;
    switch(mpConfig->configCmd)
    {
    case REMOVE_RENDER_AOUT:
        err = DoRemoveAudio();
        break;

    case STEPPLAY_RENDER_OUT:
        err = DoStep();
        break;

    case CONFIG_WINDOW_RENDER:
        err = DoConfigWinRen();
        break;

    case RENDER_SWITCH_HD:
        err = DoRenderSwitch(SWITCH_SEAMLESS);
        break;

    case RENDER_SWITCH_BACK:
        err = DoRenderSwitchBack(SWITCH_SEAMLESS);
        break;

    case RENDER_SWITCH_HD2:
        err = DoRenderSwitch(SWITCH_NON_SEAMLESS);
        break;

    case RENDER_SWITCH_BACK2:
        err = DoRenderSwitchBack(SWITCH_NON_SEAMLESS);
        break;

    default:
        AM_ERROR("wrong configCmd %d.\n", mpConfig->configCmd);
        break;
    }
    return err;
}

AM_ERR CGSyncRenderer::DoRenderSwitch(AM_INT flag)
{
    AM_ERR err = ME_OK;
    AM_ASSERT(mpVideoOut != NULL);
    CMD cmd(CMD_SWITCH_HD);
    cmd.res32_1 = flag;
    err = mpVideoOut->PerformCmd(cmd, AM_TRUE);
    return err;
}

AM_ERR CGSyncRenderer::DoRenderSwitchBack(AM_INT flag)
{
    AM_ERR err = ME_OK;
    AM_ASSERT(mpVideoOut != NULL);
    CMD cmd(CMD_SWITCH_BACK);
    cmd.res32_1 = flag;
    err = mpVideoOut->PerformCmd(cmd, AM_TRUE);
    return err;
}

AM_ERR CGSyncRenderer::PlaybackZoom(AM_INT index, AM_U16 w, AM_U16 h, AM_U16 x, AM_U16 y)
{
    AM_ERR err = ME_OK;
    AM_ASSERT(mpVideoOut != NULL);
    CMD cmd(CMD_PLAYBACK_ZOOM);
    cmd.res32_1 = index;
    cmd.res64_1 = ((AM_U64)w) | (((AM_U64)h) <<16) | (((AM_U64)x) << 32) | (((AM_U64)y) << 48);
    err = mpVideoOut->PerformCmd(cmd, AM_TRUE);
    return err;
}

AM_ERR CGSyncRenderer::DoConfigWinRen()
{
    AM_ERR err = ME_OK;
    AM_ASSERT(mpVideoOut != NULL);
    CMD cmd(CMD_CONFIG_WIN);
    err = mpVideoOut->PerformCmd(cmd, AM_TRUE);
    return err;
}

AM_ERR CGSyncRenderer::DoFlush(AM_U8 flag)
{
    AM_ERR err = ME_OK;
    CMD cmd(CMD_FLUSH);
    cmd.flag = flag;
    AM_ASSERT(mpVideoOut != NULL);
    err = mpVideoOut->PerformCmd(cmd, AM_TRUE);
    if(mpInputA != NULL){
        AM_ASSERT(mpAudioOut != NULL);
        err = mpAudioOut->PerformCmd(cmd, AM_TRUE);
    }
    return err;
}

AM_ERR CGSyncRenderer::DoStop()
{
    //AM_INFO("DoStop\n");
    AM_ASSERT(mBuffer.GetBufferType() == NOINITED_BUFFER);
    mbRun = false;
    return ME_OK;
}

AM_ERR CGSyncRenderer::DoStep()
{
    AM_ERR err = ME_OK;
    CMD cmd(CMD_STEP);
    AM_ASSERT(mpVideoOut != NULL);
    err = mpVideoOut->PerformCmd(cmd, AM_TRUE);
    return err;
}

AM_ERR CGSyncRenderer::DoPause()
{
    if(mState == STATE_EOS)
        return ME_CLOSED;
    if(mState == STATE_PENDING)
        return ME_OK;

    AM_ERR err = ME_OK;
    CMD cmd(CMD_PAUSE);

    AM_ASSERT(mpVideoOut != NULL);
    err = mpVideoOut->PerformCmd(cmd, AM_TRUE);

    if(mpInputA != NULL){
        AM_ASSERT(mpAudioOut != NULL);
        err = mpAudioOut->PerformCmd(cmd, AM_TRUE);
    }

    mState = STATE_PENDING;
    return err;
}

AM_ERR CGSyncRenderer::DoResume()
{
    AM_ASSERT(mState == STATE_PENDING);

    AM_ERR err = ME_OK;
    CMD cmd(CMD_RESUME);

    AM_ASSERT(mpVideoOut != NULL);
    err = mpVideoOut->PerformCmd(cmd, AM_TRUE);
    if(mpInputA != NULL){
        AM_ASSERT(mpAudioOut != NULL);
        err = mpAudioOut->PerformCmd(cmd, AM_TRUE);
    }

    mState = STATE_PRE_RENDER;
    return err;
}

AM_ERR CGSyncRenderer::DoRemoveAudio()
{
    if(mpInputA == NULL){
        AM_ASSERT(0);
        return ME_CLOSED;
    }

    //CMD cmd(CMD_FLUSH);
    //AM_ASSERT(mpAudioOut != NULL);
    //mpAudioOut->PerformCmd(cmd, AM_TRUE);

    mpInputA->Detach();
    mLocalHandlerA.Clear();
    mpInputA = NULL;
    return ME_OK;
}

AM_ERR CGSyncRenderer::DoAddAudio()
{
    //call by checkAnother() to notify OnRun() the audio's change
    mpAudioOut->OpenHal(&mLocalHandlerA);//just config the param, same on current stage.
    AM_INFO("DoAddAudio\n");
    return ME_OK;
}
//-----------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------
void CGSyncRenderer::OnRun()
{
    AM_ERR err;
    CMD cmd;
//    CQueue::QType type;
//    CQueue::WaitResult result;
    AM_INT ret = 0;

    //"audio_discard" added to handle buffer-overflow
    //    which dues to that "data" received is late.
    struct audio_discard_t{
        int high_water;
        int low_water;
	 int gap;
	 int discard_flag;
	 int count;
    }audio_discard;
    audio_discard.high_water = 16;
    audio_discard.low_water = 4;
    audio_discard.gap = 10;
    audio_discard.discard_flag = 0;
    audio_discard.count = 0;

    NotifyInitState(); //same time render out
    //WaitStreamStart();
    mState = STATE_WAIT_START;
    mbRun = true;
    while(mbRun)
    {
        //check the input
        switch(mState)
        {
        case STATE_WAIT_START:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            if(cmd.code != CMD_STOP && cmd.code != CMD_BEGIN_PLAYBACK)
                AM_ASSERT(0);
            ProcessCmd(cmd);
            break;

        case STATE_PRE_RENDER:
            err = InputWaitPolicy();
            if(err == ME_OK)
                mState = STATE_RENDER;
            break;

        case STATE_RENDER:
            if(mRenderType == RENDER_VIDEO){
                ret = mpInputV->PeekData(&mBuffer, sizeof(CGBuffer));
                AM_ASSERT(ret == true);
                DoRenderVideo();
                mState = STATE_PRE_RENDER;
            }else if(mRenderType == RENDER_AUDIO){
                if(mpGConfig->demuxerConfig[mConIndex].netused == AM_TRUE){
                    AM_INT num = mpInputA->GetDataCnt();
                    if(audio_discard.discard_flag){
                        if(num < audio_discard.low_water){
                            audio_discard.discard_flag = 0;
                            audio_discard.count = 0;
                        }else{
                            audio_discard.count++;
                            if(audio_discard.count >= audio_discard.gap){
                                ret = mpInputA->PeekData(&mBuffer, sizeof(CGBuffer));
                                AM_ASSERT(ret == true);
                                ((IGDecoder*)mpAOwner)->OnReleaseBuffer(&mBuffer);
                                mBuffer.Clear();
                                AM_WARNING("g_sync_renderer -- discard one audio frame, count = %d\n",num);
                                audio_discard.count = 0;
                            }
                        }
		      }else{
		          if(num >= audio_discard.high_water){
                            ret = mpInputA->PeekData(&mBuffer, sizeof(CGBuffer));
                            AM_ASSERT(ret == true);
                            ((IGDecoder*)mpAOwner)->OnReleaseBuffer(&mBuffer);
                            mBuffer.Clear();
                            AM_WARNING("g_sync_renderer -- discard one audio frame,count = %d\n",num);
                            audio_discard.count = 0;
                            audio_discard.discard_flag = 1;
		          }
		      }
                }
                ret = mpInputA->PeekData(&mBuffer, sizeof(CGBuffer));
                AM_ASSERT(ret == true);
                if(ret) DoRenderAudio();
                else{ //why come here??
                    Dump();
                }
            }
            break;

        case STATE_WAIT_EOS:
            //WAIT DSP EOS
            if(WaitStreamEnd() == ME_OK)
                break;
            if(mpWorkQ->PeekCmd(cmd)){
                err = ProcessCmd(cmd);
            }
            //if(mpAudioQ->GetDataCnt() > 0){
                //DoRenderAudio();
            //}
            break;

        case STATE_EOS:
        case STATE_PENDING:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        default:
            AM_ERROR("Check Me. %d\n", mState);
            break;
        }
    }
    AM_INFO("CGSyncRenderer OnRun Exit.\n");
}

AM_ERR CGSyncRenderer::DoRenderVideo()
{
    AM_ASSERT(mBuffer.GetStreamType() == STREAM_VIDEO);
    ((IGDecoder*)mpVOwner)->OnReleaseBuffer(&mBuffer);
    mBuffer.Clear();

    return ME_OK;
}

AM_ERR CGSyncRenderer::DoRenderAudio()
{
    //AM_INFO("DoRenderAudio mbEnAudio:%d \n", mbEnAudio);
    AM_ASSERT(mBuffer.GetStreamType() == STREAM_AUDIO);
    AM_ERR err = ME_OK;
    //mBuffer.Dump("DoRenderAudio");
    if(mBuffer.GetBufferType() != EOS_BUFFER){
        err = mpAudioOut->RenderOutBuffer(&mBuffer);
        mState = STATE_PRE_RENDER;
    }else{
        AM_INFO("Stream(%d) Audio End\n", mConIndex);
        mbRecEOSA = AM_TRUE;
        mbAudioEnd = AM_TRUE;
        if(mbRecEOSV == AM_TRUE)
            mState = STATE_WAIT_EOS;
        else
            mState = STATE_PRE_RENDER;
    }
    ((IGDecoder*)mpAOwner)->OnReleaseBuffer(&mBuffer);

    mBuffer.Clear();
    return err;
}

AM_ERR CGSyncRenderer::Render(CGBuffer* pBuffer)
{
    AM_ASSERT(0);
    //SyncRender(pBuffer);
    AM_ERR err = ME_OK;
    STREAM_TYPE type = pBuffer->GetStreamType();
    if(type == STREAM_VIDEO){
        //video just render it, time consume must be same
        //UPDATE VIDEO INFO
        err = mpVideoOut->RenderOutBuffer(pBuffer);
        pBuffer->Release();
        //return ME_OK;
   }else{
        pBuffer->Release();
   }
        AM_VERBOSE("err=%d\n", err);
    return ME_OK;
}

AM_ERR CGSyncRenderer::FillEOS(CGBuffer* pBuffer)
{
    AM_INFO("Sync Renderer(%d) Fill Eos, Stream type:%d\n", pBuffer->GetIdentity(), pBuffer->GetStreamType());
    if(pBuffer->GetStreamType() == STREAM_VIDEO){
        mbRecEOSV = AM_TRUE;
        pBuffer->Release();
    }else if(pBuffer->GetStreamType() == STREAM_AUDIO){
        AM_ASSERT(0);//ONLY notify from data Queue.
        mbRecEOSA = AM_TRUE;
        //mpAudioQ->PutData(&pBuffer, sizeof(CGBuffer*));
    }
    SendCmd(CMD_EOS);
    //AM_INFO("FillEOS return.\n");
    return ME_OK;
}

inline AM_ERR CGSyncRenderer::NotifyInitState()
{
    if(mLocalHandlerA.GetBufferType() != NOINITED_BUFFER){
        AM_ASSERT(mLocalHandlerA.GetBufferType() == HANDLE_BUFFER);
        PostFilterMsg(GFilter::MSG_SYNC_RENDER_A, 0);
    }
    if(mLocalHandlerV.GetBufferType() != NOINITED_BUFFER){
        AM_ASSERT(mLocalHandlerV.GetBufferType() == HANDLE_BUFFER);
        PostFilterMsg(GFilter::MSG_SYNC_RENDER_V, 0);
    }
    return ME_OK;
}

inline AM_ERR CGSyncRenderer::InputWaitPolicy()
{
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
//    AM_INT ret = 0;
    AM_INT vNum, aNum;

    if(mpInputA == NULL && mpInputV == NULL){
        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
        ProcessCmd(cmd);
        return ME_TRYNEXT;
    }

    if(mpInputA == NULL || mpInputV == NULL){
        AM_ASSERT(mpInputV == NULL);
        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
        if(type == CQueue::Q_MSG)
        {
            ProcessCmd(cmd);
            return ME_TRYNEXT;
        }else{
            mRenderType = (result.pDataQ == mpInputV) ? RENDER_VIDEO : RENDER_AUDIO;
        }
    }

    if(mpInputA != NULL && mpInputV != NULL)
    {
        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
        if(type == CQueue::Q_MSG)
        {
            ProcessCmd(cmd);
            return ME_TRYNEXT;
        }else{
            vNum = mpInputV->GetDataCnt();
            aNum = mpInputA->GetDataCnt();
            if(aNum <= 0){
                mRenderType = RENDER_VIDEO;
            }else if(vNum <= 0){
                mRenderType = RENDER_AUDIO;
            }else{
                mRenderType = (mCurVideoPts < mCurAudioPts) ? RENDER_VIDEO : RENDER_AUDIO;
            }
        }
    }

    return ME_OK;
}

inline AM_ERR CGSyncRenderer::BeginRenderOut()
{
    AM_ERR err=ME_OK;
    Dump();
    if(mLocalHandlerA.GetBufferType() != NOINITED_BUFFER){
        err = mpAudioOut->OpenHal(&mLocalHandlerA);
        mLocalHandlerA.Clear();
    }
    if(mLocalHandlerV.GetBufferType() != NOINITED_BUFFER){
        err = mpVideoOut->OpenHal(&mLocalHandlerV);
        mLocalHandlerV.Clear();
    }
    AM_VERBOSE("err=%d\n", err);
    return ME_OK;
}

AM_ERR CGSyncRenderer::WaitStreamEnd()
{
    AM_ERR err;
    if(mState != STATE_WAIT_EOS)
        return ME_BAD_STATE;

    //if loop for local file, no need wait the uncertainty eos from dsp
    if(mpGConfig->globalFlag & LOOP_FOR_LOCAL_FILE)
    {
        AM_INFO("Send Stream(%d) EOS on Loop Case!\n", mConIndex);
        SendFilterMsg(GFilter::MSG_RENDER_END, 0);
        mState = STATE_PRE_RENDER;
        mbRecEOSA = mbRecEOSV = mbAudioEnd = mbVideoEnd = AM_FALSE;
        return ME_OK;
    }

    //AM_INFO("WaitStreamEnd(%d).\n", mConIndex);
    if(mbVideoEnd == AM_FALSE){
        CMD cmd(CMD_ISEOS);
        sleep(1);
        err = mpVideoOut->PerformCmd(cmd, AM_TRUE);
        if(err == ME_OK){
            AM_INFO("Stream(%d) Video End\n", mConIndex);
            mbVideoEnd = AM_TRUE;
        }
    }

    //no need to wait audio end comes
    if(mpGConfig->demuxerConfig[mConIndex].disableAudio == AM_TRUE && mbVideoEnd == AM_TRUE)
    {
        mbAudioEnd = AM_TRUE;
    }

    if(mbVideoEnd == AM_TRUE && mbAudioEnd == AM_TRUE)
    {
        AM_INFO("Stream(%d) Goto  End!\n", mConIndex);
        SendFilterMsg(GFilter::MSG_RENDER_END, 0);
        mState = STATE_PRE_RENDER;
        mbRecEOSA = mbRecEOSV = mbAudioEnd = mbVideoEnd = AM_FALSE;
        return ME_OK;
    }
    return ME_TRYNEXT;
}
//========================================================================
//
//
AM_ERR CGSyncRenderer::PostFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    msg.p1 = mConIndex;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->PostMsgToFilter(msg);
    return ME_OK;
}

AM_ERR CGSyncRenderer::SendFilterMsg(AM_UINT code, AM_INTPTR ptr)
{
    AM_MSG msg;
    msg.code = code;
    msg.p0 = ptr;
    msg.p1 = mConIndex;
    CInterActiveFilter* pFilter = (CInterActiveFilter* )mpFilter;
    pFilter->SendMsgToFilter(msg);
    return ME_OK;
}

void CGSyncRenderer::Dump()
{
    AM_INFO("       {---Sync Renderer(%d)---}\n", mConIndex);
    AM_INFO("           Config->Has Audio Pool:%p, Has Video Pool:%p\n", mpInputA, mpInputV);
    AM_INFO("           State->%d, EOS flag: VREos:%d, AREos:%d, VEnd:%d, AEnd:%d\n",
        mState, mbRecEOSV, mbRecEOSA, mbVideoEnd, mbAudioEnd);

    AM_INFO("           BufferPool->Audio Bp Cnt:%d  Video Bp Cnt:%d\n", (mpInputA != NULL) ? mpInputA->GetDataCnt() : -1,
        (mpInputV != NULL) ? mpInputV->GetDataCnt() : -1);
}
