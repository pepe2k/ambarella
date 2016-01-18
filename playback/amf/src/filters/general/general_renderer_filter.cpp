/*
 * general_renderer_filter.cpp
 *
 * History:
 *    2012/4/1 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

#include "general_renderer_filter.h"

CGeneralRenderer* CreateGeneralRendererFilter(IEngine* pEngine, CGConfig* config)
{
    return CGeneralRenderer::Create(pEngine, config);
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
CGeneralRenderer* CGeneralRenderer::Create(IEngine* pEngine, CGConfig* pConfig)
{
    CGeneralRenderer* result = new CGeneralRenderer(pEngine, pConfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CGeneralRenderer::CGeneralRenderer(IEngine* pEngine, CGConfig* pConfig):
    inherited(pEngine, "CGeneralRenderer"),
    mpConfig(pConfig),
    mpBuffer(NULL),
    mRenNum(0),
    mbSyncDone(AM_FALSE)
{
    mbRun = false;
    AM_INT i = 0;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
        mpRenderer[i] = NULL;
    for(i = 0; i < RENDERER_INPUT_NUM; i++)
        mpInputPin[i] = NULL;
}

AM_ERR CGeneralRenderer::Construct()
{
    AM_INFO("****CGeneralRenderer::Construct .\n");

    AM_ERR err = inherited::Construct();
    if (err != ME_OK) {
        AM_ERROR("CGeneralRenderer Construct fail err %d .\n", err);
        return err;
    }
    SetThreadPrio(1, 1);
    return ME_OK;
}

AM_ERR CGeneralRenderer::ConstructInputPin(AM_INT index)
{
    if(mpInputPin[index] != NULL)
        return ME_OK;

    CGeneralInputPin* pInput = CGeneralInputPin::Create(this);
    if(pInput == NULL)
    {
        AM_ERROR("CGeneralInputPin::Create Failed!\n");
        //return ME_ERROR;
    }
    mpInputPin[index] = pInput;
    return ME_OK;
}

IPin* CGeneralRenderer::GetInputPin(AM_UINT index)
{
    AM_INFO("Call CGeneralRenderer::GetInputPin %d.\n", index);
    if(index >= RENDERER_INPUT_NUM)
        return NULL;
    ConstructInputPin(index);
    return mpInputPin[index];
}

AM_ERR CGeneralRenderer::SetInputFormat(CMediaFormat* pFormat)
{
    return ME_OK;
}

AM_ERR CGeneralRenderer::SetOutputFormat(CMediaFormat* pFormat)
{
    return ME_OK;
}

void CGeneralRenderer::Delete()
{
    AM_INFO(" ====== All Filter Exit OnRun Loop====== \n");
    AM_INT i = 0;
    for(i = 0; i < RENDERER_INPUT_NUM; i++){
        AM_DELETE(mpInputPin[i]);
    }
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        if(mpRenderer[i] != NULL){
            AM_DELETE(mpRenderer[i]);
        }
    }
    inherited::Delete();
}

CGeneralRenderer:: ~CGeneralRenderer()
{
    AM_INFO("Destroy CGeneralRenderer Done.\n");
}

void CGeneralRenderer::GetInfo(INFO& info)
{
    info.nInput = 2;
    info.nOutput = 0;
    info.mPriority = 0;
    info.mFlags = 0;
    info.mIndex = -1;
    info.pName = "CGeneralRenderer";
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
void CGeneralRenderer::MsgProc(AM_MSG& msg)
{
    AM_INT index;
    switch(msg.code)
    {
    case MSG_SYNC_RENDER_V:
        index = msg.p1;
        mpConfig->rendererConfig[index].videoReady = AM_TRUE;
        StartAllRenderOut(index);
        break;

    case MSG_SYNC_RENDER_A:
        index = msg.p1;
        mpConfig->rendererConfig[index].audioReady = AM_TRUE;
        StartAllRenderOut(index);
        break;

    case MSG_RENDER_END:
        PostEngineMsg(msg);
        break;

    case MSG_RENDER_ADD_AUDIO:
        PostEngineMsg(msg);
        break;

    default:
        break;
    }
}

AM_ERR CGeneralRenderer::HandRenderCmd(AM_INT target,  CMD& cmd, AM_BOOL isSend)
{
    AM_INT i;
    if(target == CMD_EACH_COMPONENT){
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            if(mpRenderer[i] != NULL){
                mpRenderer[i]->PerformCmd(cmd, isSend);
            }
        }
        return ME_OK;
    }
    if(target < 0 || target >= MDEC_SOURCE_MAX_NUM){
        AM_ERROR("HandRenderCmd target(%d) Wrong!\n", target);
        return ME_BAD_PARAM;
    }
    if(mpRenderer[target] != NULL)
        mpRenderer[target]->PerformCmd(cmd, isSend);
    return ME_OK;
}

AM_ERR CGeneralRenderer::PlaybackZoom(AM_INT index, AM_U16 w, AM_U16 h, AM_U16 x, AM_U16 y)
{
    AM_ASSERT(mpRenderer[index]);
    if(mpRenderer[index]==NULL){
        AM_ERROR("PlaybackZoom index(%d) Wrong!\n", index);
        return ME_BAD_PARAM;
    }
    return mpRenderer[index]->PlaybackZoom(index, w, h, x, y);
}

AM_ERR CGeneralRenderer::DoStop()
{
    CMD cmd(CMD_STOP);
    HandRenderCmd(CMD_EACH_COMPONENT, cmd, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralRenderer::DoPause(AM_INT target, AM_U8 flag)
{
    //AM_INFO("CGeneralRenderer Pause, %d\n", target);
    if(target == CMD_GENERAL_ONLY){
        if(msState != STATE_PENDING){
            msOldState = msState;
            msState = STATE_PENDING;
        }
        return ME_OK;
    }
    CMD cmd(CMD_PAUSE);
    HandRenderCmd(target, cmd, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralRenderer::DoResume(AM_INT target, AM_U8 flag)
{
    //AM_INFO("CGeneralRenderer Resume\n");
    if(target == CMD_GENERAL_ONLY){
        if(msState == STATE_PENDING){
            if(flag & RESUME_TO_IDLE){
                //AM_ASSERT(0);
                msState = STATE_IDLE;
            }else{
                msState = msOldState;
            }
        }
        return ME_OK;
    }

    CMD cmd(CMD_RESUME);
    HandRenderCmd(target, cmd, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralRenderer::DoFlush(AM_INT target, AM_U8 flag)
{
    //AM_INFO("CGeneralRenderer Flush\n");
    if(mpBuffer != NULL){
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    if(target == CMD_GENERAL_ONLY){
        return ME_OK;
    }

    CMD cmd(CMD_FLUSH);
    cmd.flag = flag;
    HandRenderCmd(target, cmd, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralRenderer::DoConfig(AM_INT target, AM_U8 flag)
{
    AM_INFO("CGeneralRenderer Config:%d, %d\n", mpConfig->generalCmd, mpConfig->specifyIndex);
    if(mpConfig->generalCmd & RENDERER_DISABLE_AUDIO){
        AM_ASSERT(0);
    }

    if(mpConfig->generalCmd & RENDERER_ENABLE_AUDIO){
        AM_ASSERT(0);
    }

    if(mpConfig->generalCmd & RENDERER_DISCONNECT_HW){
        CMD cmd(CMD_DISCONNECT_HW);
        HandRenderCmd(target, cmd, AM_TRUE);
    }

    if(mpConfig->generalCmd & RENDEER_SPECIFY_CONFIG){
        AM_INT index = mpConfig->specifyIndex;
        AM_ASSERT(mpRenderer[index] != NULL);
        AM_ASSERT(index == target);
        CMD cmd(CMD_CONFIG);
        HandRenderCmd(target, cmd, AM_TRUE);
    }
    return ME_OK;
}

AM_ERR CGeneralRenderer::DoRemove(AM_INT index, AM_U8 flag)
{
    AM_ERR err = ME_OK;
    if(index < 0 || index >= MDEC_SOURCE_MAX_NUM)
        AM_ASSERT(0);
    if(mpRenderer[index] == NULL)
        AM_ASSERT(0);

    //send stop
    AO::CMD cmd(CMD_STOP);
    mpRenderer[index]->PerformCmd(cmd, AM_TRUE);
    AM_DELETE(mpRenderer[index]);
    mpRenderer[index] = NULL;
    mpConfig->SetMapRen(index, -1);
    //msOldState = STATE_IDLE;
    return err;
}

bool CGeneralRenderer::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGeneralRenderer::ProcessCmd:%d.\n", cmd.code);
    AM_ERR err;
    AM_INT par;
    AM_U8 flag;
    switch (cmd.code)
    {
    case CMD_STOP:
        DoStop();
        mbRun = false;
        CmdAck(ME_OK);
        break;

     //todo check this
    case CMD_OBUFNOTIFY:
        break;

    case CMD_PAUSE:
        par = cmd.res32_1;
        flag = cmd.flag;
        DoPause(par, flag);
        CmdAck(ME_OK);
        break;

    case CMD_RESUME:
        par = cmd.res32_1;
        flag = cmd.flag;
        DoResume(par, flag);
        CmdAck(ME_OK);
        break;

    case CMD_FLUSH:
        par = cmd.res32_1;
        flag = cmd.flag;
        DoFlush(par, flag);
        CmdAck(ME_OK);
        break;

    case CMD_AVSYNC:
        CmdAck(ME_OK);
        break;

    case CMD_ACK:
        CmdAck(ME_OK);
        break;

    case CMD_CONFIG:
        par = cmd.res32_1;
        flag = cmd.flag;
        err = DoConfig(par, flag);
        CmdAck(err);
        break;

    case CMD_REMOVE:
        //AM_ASSERT(msState == STATE_PENDING);
        par = cmd.res32_1;
        flag = cmd.flag;
        DoRemove(par, flag);
        CmdAck(ME_OK);
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return false;
}

CQueue::QType CGeneralRenderer::WaitPolicy(CMD& cmd, CQueue::WaitResult& result)
{
    CQueue::QType type;
    type = mpWorkQ->WaitDataMsgCircularly(&cmd, sizeof(cmd), &result);
    return type;
}

void CGeneralRenderer::OnRun()
{
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
//    AM_INT ret = 0;

    mbRun = true;
    msState = STATE_IDLE;
    CmdAck(ME_OK);
    while(mbRun)
    {
        //AM_INFO("###CGeneral Renderer: start switch, msState=%d.\n", msState);
        switch (msState)
        {
        case STATE_IDLE:
            type = WaitPolicy(cmd, result);
            if(type == CQueue::Q_MSG)
            {
                ProcessCmd(cmd);
            }else{
                if(ReadInputData(result) == ME_OK)
                {
                    msState = STATE_HAS_INPUTDATA;
                }
            }
            break;

        case STATE_HAS_INPUTDATA:
            AM_ASSERT(mpBuffer);
            msState = STATE_READY;
            break;

        case STATE_READY:
            AM_ASSERT(mpBuffer);
            if(IsRenderReady() != ME_OK)
            {
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
            }else{
                ProcessBuffer();
                msState = STATE_IDLE;
            }
            break;

        case STATE_PENDING:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        case STATE_ERROR:
            break;

        default:
            AM_ERROR(" %d",(AM_UINT)msState);
            break;
        }
    }
    if(mpBuffer != NULL)
    {
        AM_INFO("CGeneralRenderer exit release buffer\n");
        mpBuffer->Release();
        mpBuffer = NULL;
    }
    AM_INFO("CGeneralRenderer OnRun Exit!\n");
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
AM_ERR CGeneralRenderer::ReadInputData(CQueue::WaitResult& result)
{
    //AM_INFO("--ReadInputData\n");
    AM_ERR err;
    CGeneralInputPin* pInput = (CGeneralInputPin* )(result.pOwner);
    CBuffer* cBuffer;
    if(!pInput->PeekBuffer(cBuffer))
    {
        AM_ERROR("!!PeekBuffer Failed!\n");
        return ME_ERROR;
    }
    mpBuffer = (CGBuffer* )cBuffer;
    mCurIndex = mpBuffer->GetIdentity();
    //if(mpBuffer->GetStreamType() == STREAM_AUDIO)
    mpBuffer->Dump("RenderInput");
    if(mpBuffer->GetBufferType() == HANDLE_BUFFER)
    {
        if(mpBuffer->GetStreamType() == STREAM_VIDEO)
            mpConfig->SetMapRen(mCurIndex, mCurIndex);

        if(mpRenderer[mCurIndex] != NULL){
            CGBuffer tmpBuffer = *mpBuffer;
            mpBuffer->Release();
            mpBuffer = NULL;
            err = mpRenderer[mCurIndex]->CheckAnother(&tmpBuffer);
            AM_ASSERT(err == ME_OK); //todo
        }else{
            CreateRenderer();//error handle
        }

        if(mpBuffer != NULL){
            mpBuffer->Release();
            mpBuffer = NULL;
        }
        return ME_TRYNEXT;
    }

    AM_ASSERT(mpRenderer[mCurIndex]);
    AMLOG_DEBUG("ReadInputData mpBuffer %p.\n", mpBuffer);
    return ME_OK;
}

AM_ERR CGeneralRenderer::CreateRenderer()
{
    AM_INT score, max = 0;

    GRendererParser** parserList = gRendererList;
    GRendererParser* parser = *parserList;
    GRendererParser* maxParser = NULL;
    while(parser != NULL)
    {
        score = parser->Parser(mpBuffer);
        if(score > max){
            max = score;
            if(maxParser)
                maxParser->Clearer();
            maxParser = parser;
        }
        parserList++;
        parser = *parserList;
    }
    if(maxParser == NULL)
    {
        return ME_NOT_SUPPORTED;
    }
    AM_INFO("Find GRenderer:%s", maxParser->Name);

    mpRenderer[mCurIndex] = maxParser->Creater((IFilter* )this, mpConfig);
    if(mpRenderer[mCurIndex] == NULL)
        return ME_ERROR;

    mpBuffer->Release();
    mpBuffer = NULL;

    AO::CMD cmd(CMD_RUN);
    mpRenderer[mCurIndex]->PerformCmd(cmd, AM_FALSE);

    mRenNum++;
    return ME_OK;
}

AM_ERR CGeneralRenderer::IsRenderReady()
{
    return mpRenderer[mCurIndex]->IsReady(mpBuffer);
}

AM_ERR CGeneralRenderer::ProcessBuffer()
{
    //AdjustAVSync();
    AM_ERR err;
    AM_ASSERT(mpBuffer->GetBufferType() != DATA_BUFFER);

    if(mpBuffer->GetBufferType() == EOS_BUFFER)
    {
        err = ProcessEOS();
        return err;
    }
    return ME_OK;
}

AM_ERR CGeneralRenderer::ProcessEOS()
{
    AM_ERR err;

    err = mpRenderer[mCurIndex]->FillEOS(mpBuffer);
    AM_VERBOSE("err=%d\n", err);
    mpBuffer = NULL;
    return ME_OK;
}

inline AM_ERR CGeneralRenderer::StartAllRenderOut(AM_INT index)
{
    AM_INT i;
    if(mbSyncDone == AM_TRUE){
        //we have done the init's sync, notice engine for dynamic add
        AO::CMD cmd2(CMD_BEGIN_PLAYBACK);
        AM_ASSERT(mpRenderer[index] != NULL);
        mpRenderer[index]->PerformCmd(cmd2, AM_FALSE);
        //case1: audio/video all ready
        //case2: video ready and audio is enabled by other stream.
        if((mpConfig->rendererConfig[index].videoReady == AM_TRUE &&
            mpConfig->rendererConfig[index].audioReady == AM_TRUE) ||
            (mpConfig->rendererConfig[index].videoReady == AM_TRUE &&
            mpConfig->GetAudioSource() != index))
        {
            AM_MSG msg;
            msg.code = MSG_SYNC_RENDER_DONE;
            msg.p1 = index;
            PostEngineMsg(msg);
        }
    }

    if(mbSyncDone == AM_FALSE && mpConfig->AllRenderOutReady() == AM_TRUE)
    {
        AM_INFO("StartAllRenderOut\n");
        AO::CMD cmd(CMD_BEGIN_PLAYBACK);
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            if(mpRenderer[i] != NULL){
                mpRenderer[i]->PerformCmd(cmd, AM_FALSE);
            }
        }
        PostEngineMsg(MSG_SYNC_RENDER_DONE);
        mbSyncDone = AM_TRUE;
    }
    return ME_OK;
}

AM_ERR CGeneralRenderer::ReSet()
{
    return ME_OK;
}

void CGeneralRenderer::Dump()
{
    AM_INFO("\n---------------------GRenderer Info------------\n");
    if(mpInputPin[0])
        AM_INFO("       BufferPool--->Video input DataCnt:%d",
        mpInputPin[0]->GetDataCnt());
    if(mpInputPin[1])
        AM_INFO("  Audio input DataCnt:%d\n",
            mpInputPin[1]->GetDataCnt());
    AM_INFO("       State---> on state: %d\n", msState);
    AM_INT i;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        if(mpRenderer[i] != NULL)
            mpRenderer[i]->Dump();
    }
    AM_INFO("\n");
}

/*
AM_ERR CGeneralRenderer::UpdataStatData()
{

}

AM_ERR CGeneralRenderer::AdjustAVSync()
{


}
*/


