/*
 * general_demuxer_filter.cpp
 *
 * History:
 *    2012/3/27 - [QingXiong Z] create file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <unistd.h>
#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"
#include "general_demuxer_filter.h"

#define LOG_RETURN \
    do{ \
        AM_ERROR("%s,  %d", __FILE__, __LINE__);   \
        return ME_ERROR;    \
     }while(0);

CGeneralDemuxer* CreateGeneralDemuxerFilter(IEngine* pEngine, CGConfig* config)
{
    return CGeneralDemuxer::Create(pEngine, config);
}

//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
CGeneralDemuxer* CGeneralDemuxer::Create(IEngine* pEngine, CGConfig* pConfig)
{
    CGeneralDemuxer* result = new CGeneralDemuxer(pEngine, pConfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
        }
    return result;
}

AM_ERR CGeneralDemuxer::Construct()
{
    AM_INFO("****CGeneralDemuxer::Construct .\n");
    AM_ERR err = inherited::Construct();
    if (err != ME_OK) {
        AM_ERROR("CGeneralDemuxer::Construct fail err %d .\n", err);
        return err;
    }
    err = ConstructPin();
    if (err != ME_OK) {
        AM_ERROR("CGeneralDemuxer::Construct Pin err %dxx .\n", err);
        return err;
    }

    //todo needed a mutex?
    SetThreadPrio(1, 1);
    return ME_OK;
}



AM_ERR CGeneralDemuxer::ConstructPin()
{
    if(mpVideoPin == NULL)
    {
        mpVideoPin = CGeneralOutputPin::Create(this);
        if(mpVideoPin == NULL)
            LOG_RETURN;
        if((mpVideoBufferPool = CGeneralBufferPool::Create("GDemuxer Video OuputBP", GDEMUXER_VIDEO_CTRL_BP)) == NULL)
            LOG_RETURN;
        mpVideoPin->SetBufferPool(mpVideoBufferPool);
        AM_INFO("CGeneralDemuxer BP:%p\n", (IBufferPool*)mpVideoBufferPool);
    }

    if(mpAudioPin == NULL)
    {
        mpAudioPin = CGeneralOutputPin::Create(this);
        if(mpAudioPin == NULL)
            LOG_RETURN;
        if((mpAudioBufferPool = CGeneralBufferPool::Create("GDemuxer Audio OuputBP", GDEMUXER_AUDIO_CTRL_BP)) == NULL)
            LOG_RETURN;
        mpAudioPin->SetBufferPool(mpAudioBufferPool);
    }
    return ME_OK;
}

void CGeneralDemuxer::Delete()
{
    AM_INT i = 0;
    AM_DELETE(mpVideoPin);
    AM_DELETE(mpAudioPin);
    mpCurPin = NULL;
    AM_DELETE(mpVideoBufferPool);
    AM_DELETE(mpAudioBufferPool);
    mpCurBufferPool = NULL;

    for(; i < MDEC_SOURCE_MAX_NUM; i++){
        if(mpDemuxer[i] != NULL){
            AM_DELETE(mpDemuxer[i]);
        }
    }
    inherited::Delete();
}

CGeneralDemuxer:: ~CGeneralDemuxer()
{
    AM_INFO("Destroy CGeneralDemuxer Done.\n");
}

IPin* CGeneralDemuxer::GetOutputPin(AM_UINT index)
{
    if(index == 0){
        return mpVideoPin;
    }
    if(index == 1){
        return mpAudioPin;
    }
    return NULL;
}

void CGeneralDemuxer::GetInfo(INFO& info)
{
    info.nInput = 1;
    info.nOutput = 2;
    info.mPriority = 0;
    info.mFlags = 0;
    info.mIndex = -1;
    info.pName = "CGeneralDemuxer";
}

AM_ERR CGeneralDemuxer::AcceptMedia(const char* filename, AM_BOOL run)
{
    AM_INT score, index, max = 0;
    AM_ERR err = ME_ERROR;
    GDemuxerParser** parserList = gDemuxerList;
    GDemuxerParser* parser = *parserList;
    GDemuxerParser* maxParser = NULL;
    while(parser != NULL)
    {
        score = parser->Parser(filename, mpConfig);
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
        AM_ERROR("No Find Demuxer!");
        return ME_NOT_SUPPORTED;
    }
    AM_INFO("Find Demuxer:%s", maxParser->Name);

    index = mpConfig->curIndex;
    AM_ASSERT(mpDemuxer[index] == NULL);
    mpDemuxer[index] = maxParser->Creater((IFilter* )this, mpConfig);
    err = mpDemuxer[index]->LoadFile();
    if(err != ME_OK)
    {
        AM_DELETE(mpDemuxer[index]);
        mpDemuxer[index] = NULL;
        //AM_ASSERT(0);
        return ME_NOT_SUPPORTED;
    }
    //let it run
    if(run){
        CMD cmd(CMD_RUN);
        mpDemuxer[index]->PerformCmd(cmd, AM_TRUE);
    }

    return ME_OK;
}

AM_ERR CGeneralDemuxer::UpdatePBDirection(AM_INT target, AM_U8 flag)
{
    CMD cmd;
    cmd.res32_1 = target;
    cmd.flag = flag;
    cmd.code = CMD_UPDATE_PLAYBACK_DIRECTION;
    mpWorkQ->MsgQ()->PostMsg(&cmd, sizeof(cmd));
    return ME_OK;
}

AM_ERR CGeneralDemuxer::SeekTo(AM_INT target, AM_U64 ms, AM_INT flag)
{
    if(mpConfig->globalFlag & NOTHING_DONOTHING)
        return ME_OK;
    AM_ERR err = ME_OK;
    if(target < 0 || target >= MDEC_SOURCE_MAX_NUM){
        AM_ERROR("SeekTo target(%d) Wrong!\n", target);
        return ME_BAD_PARAM;
    }
    if(mpDemuxer[target] != NULL)
        err = mpDemuxer[target]->SeekTo(ms, flag);
    AM_INFO("Demuxer(%d) Seek To %lld Done, return:%d.\n", target, ms, err);
    return err;
}

AM_ERR CGeneralDemuxer::AudioCtrl(AM_INT target, AM_INT flag)
{
    if(mpConfig->globalFlag & NOTHING_DONOTHING)
        return ME_OK;
    AM_ERR err = ME_OK;
    if(target < 0 || target >= MDEC_SOURCE_MAX_NUM){
        AM_ERROR("AudioCtrl target(%d) Wrong!\n", target);
        return ME_BAD_PARAM;
    }
    AO::CMD cmd(CMD_AUDIO);
    cmd.flag = flag;
    if(mpDemuxer[target] != NULL)
        mpDemuxer[target]->PerformCmd(cmd, AM_TRUE);
    return err;
}

AM_ERR CGeneralDemuxer::ReSet()
{
    return ME_OK;
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
AM_ERR CGeneralDemuxer::DoStop()
{
    AM_INFO("CGeneralDemuxer Stop\n");

    if(mBuffer.GetBufferType() != NOINITED_BUFFER)
    {
        ReleaseBuffer(&mBuffer);
    }
    HandDemuxerCmd(CMD_EACH_COMPONENT, CMD_STOP, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralDemuxer::DoPause(AM_INT target, AM_U8 flag)
{
    AM_INFO("CGeneralDemuxer Pause target:%d.\n", target);
    if(target == CMD_GENERAL_ONLY){
        if(msState != STATE_PENDING){
            msOldState = msState;
            msState = STATE_PENDING;
        }
        return ME_OK;
    }
    HandDemuxerCmd(target, CMD_PAUSE, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralDemuxer::DoResume(AM_INT target, AM_U8 flag)
{
    AM_INFO("CGeneralDemuxer Resume target:%d.\n", target);
    if(target == CMD_GENERAL_ONLY){
        if(msState == STATE_PENDING){
            if(flag & RESUME_TO_IDLE){
                msState = STATE_IDLE;
            }else{
                msState = msOldState;
            }
        }
        return ME_OK;
    }
    HandDemuxerCmd(target, CMD_RESUME, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralDemuxer::DoFlush(AM_INT target, AM_U8 flag)
{
    if(mBuffer.GetBufferType() != NOINITED_BUFFER){
        ReleaseBuffer(&mBuffer);
        mBuffer.Clear();
    }
    if(target == CMD_GENERAL_ONLY){
        return ME_OK;
    }

    HandDemuxerCmd(target, CMD_FLUSH, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralDemuxer::DoConfig(AM_INT target, AM_U8 flag)
{
    AM_INFO("CGeneralDemuxer Config target:%d.\n", target);
    if(flag & CONFIG_DEMUXER_GO_RUN){
        HandDemuxerCmd(target, CMD_RUN, AM_TRUE);
        return ME_OK;
    }

    if(mpConfig->generalCmd & DEMUXER_SPECIFY_GETBUFFER){
        mSpecify = mpConfig->specifyIndex;
        AM_INFO("Specify read from Q or Canncel:%d.\n", mSpecify);
    }

    HandDemuxerCmd(target, CMD_CONFIG, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralDemuxer::DoUpdatePBDirection(AM_INT target, AM_U8 flag)
{
    AM_ERR err;
    AM_INFO("CGeneralDemuxer DoUpdatePBDirection, target 0x%08x, flag %d\n", target, flag);
    if (flag) {
        err = HandDemuxerCmd(target, CMD_BW_PLAYBACK_DIRECTION, false);
    } else {
        err = HandDemuxerCmd(target, CMD_FW_PLAYBACK_DIRECTION, false);
    }
    return err;
}

AM_ERR CGeneralDemuxer::DoRemove(AM_UINT index, AM_U8 flag)
{
    AM_INFO("CGeneralDemuxer Remove, target:%d.\n", index);
//    AM_ERR err;
    if(mpDemuxer[index] == NULL)
        AM_ASSERT(0);

    //send stop
    AO::CMD cmd(CMD_STOP);
    mpDemuxer[index]->PerformCmd(cmd, AM_TRUE);
    AM_DELETE(mpDemuxer[index]);
    mpDemuxer[index] = NULL;
    return ME_OK;
}

AM_ERR CGeneralDemuxer::HandDemuxerCmd(AM_INT target, AM_UINT code, AM_BOOL isSend)
{
    AM_INT i;
    AO::CMD cmd(code);

    if(target == CMD_EACH_COMPONENT){
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            if(mpDemuxer[i] != NULL){
                mpDemuxer[i]->PerformCmd(cmd, isSend);
            }
        }
        return ME_OK;
    }
    if(target < 0 || target >= MDEC_SOURCE_MAX_NUM){
        AM_ERROR("HandDemuxerCmd target(%d) Wrong!\n", target);
        return ME_BAD_PARAM;
    }
    if(mpDemuxer[target] != NULL)
        mpDemuxer[target]->PerformCmd(cmd, isSend);
    return ME_OK;
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
void CGeneralDemuxer::MsgProc(AM_MSG& msg)
{
    AM_INT index;
    switch(msg.code)
    {
    case MSG_DEMUXER_A:
        index = (AM_INT)(msg.p1);
        SendAudioStream(index);
        break;

    case MSG_SWITCH_TEST:
        PostEngineMsg(msg);
        break;

    case MSG_SWITCHBACK_TEST:
        PostEngineMsg(msg);
        break;

    case MSG_TEST:
        PostEngineMsg(msg);
        break;

    default:
        break;
    }
}

bool CGeneralDemuxer::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGeneralDemuxer::ProcessCmd:%d.\n", cmd.code);
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

    case CMD_OBUFNOTIFY:
        if(msState == STATE_WAIT_OUTPUT){
            //let msState loop, test if is more trustinees.
            //msState = STATE_READY;
        }
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

    case CMD_CONFIG:
        par = cmd.res32_1;
        flag = cmd.flag;
        err = DoConfig(par, flag);
        CmdAck(err);
        break;

    case CMD_UPDATE_PLAYBACK_DIRECTION:
        par = cmd.res32_1;
        flag = cmd.flag;
        err = DoUpdatePBDirection(par, flag);
        //CmdAck(err);
        break;

    case CMD_FIRST_SOURCE:
        AM_ASSERT(msState == STATE_PENDING);
        msState = STATE_IDLE;
        break;

    case CMD_REMOVE:
        par = cmd.res32_1;
        DoRemove(par, 0);
        CmdAck(ME_OK);
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }

    return true;
}

CQueue::QType CGeneralDemuxer::WaitPolicy(CMD& cmd, CQueue::WaitResult& result)
{
    CQueue::QType type;
    if(mSpecify >= 0)
    {
        const CQueue* queue = mpDemuxer[mSpecify]->GetBufferQ();
        type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), queue);
        //hack here
        result.pOwner = (void* )mpDemuxer[mSpecify];
    }else{
            type = mpWorkQ->WaitDataMsgCircularly(&cmd, sizeof(cmd), &result);
    }

    return type;
}

void CGeneralDemuxer::OnRun()
{
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
//    AM_INT ret = 0;

    mbRun = true;
    msState = STATE_IDLE;
    HandDemuxerCmd(CMD_EACH_COMPONENT, CMD_RUN, AM_TRUE);
    CmdAck(ME_OK);
    while(mbRun)
    {
        //AM_INFO("while onrun debug: %d\n\n", msState);
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
            AM_ASSERT(mBuffer.GetBufferType() != NOINITED_BUFFER);
            if(mpCurBufferPool->GetFreeBufferCnt() > 0)
            {
                msState = STATE_READY;
            }else{
                msState = STATE_WAIT_OUTPUT;
            }
            break;

        case STATE_WAIT_OUTPUT:
            AM_ASSERT(mBuffer.GetBufferType() != NOINITED_BUFFER);
            if(mpCurBufferPool->GetFreeBufferCnt() <= 0)
            {
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
            }else{
                msState = STATE_READY;
            }
            break;

        case STATE_READY:
            AM_ASSERT(mBuffer.GetExtraPtr());
            if(ProcessBuffer() == ME_OK)
            {
                msState = STATE_IDLE;
            }else{
                msState = STATE_ERROR;
            }
            break;

        case STATE_PENDING:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        case STATE_ERROR:
            AM_ERROR("STATE_ERROR\n");
            //post to engine
            break;

        default:
            AM_ERROR(" %d",(AM_UINT)msState);
            break;
        }
    }
    AM_INFO("CGeneralDemuxer OnRun Exit!\n");
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
AM_ERR CGeneralDemuxer::ReadInputData(CQueue::WaitResult& result)
{
//    AM_INT i = 0;
    IGDemuxer* pDem = (IGDemuxer* )(result.pOwner);
    if((pDem->GetCGBuffer(mBuffer)) != ME_OK)
    {
        //AM_ERROR("!!GetCGBuffer Failed!\n");
        return ME_ERROR;
    }
    enum STREAM_TYPE type = (STREAM_TYPE)mBuffer.GetStreamType();

    if(type == STREAM_VIDEO)
    {
        mpCurBufferPool = mpVideoBufferPool;
        mpCurPin = mpVideoPin;
    }else if(type == STREAM_AUDIO){
        AM_ASSERT(mBuffer.GetBufferType() == HANDLE_BUFFER);
        mpCurBufferPool = mpAudioBufferPool;
        mpCurPin = mpAudioPin;
    }
    return ME_OK;
}

//do something with mBuffer(universal env)
void CGeneralDemuxer::SetCBuffer(CGBuffer* gBuffer)
{
    mBuffer.mpPool = gBuffer->mpPool;
    mBuffer.mpNext = gBuffer->mpNext;
    mBuffer.mRefCount = gBuffer->mRefCount;
}

AM_ERR CGeneralDemuxer::ProcessBuffer()
{
    CBuffer* cBuffer;
    CGBuffer* pBuffer;
    AM_ASSERT(mpCurBufferPool->GetFreeBufferCnt() > 0);
    if (!mpCurPin->AllocBuffer(cBuffer))
    {
        AM_ERROR("Demuxer mpOutputPin is disabled!\n");
        //i think over this case may happen on release one side (change audio)
        ReleaseBuffer(&mBuffer);
        return ME_OK;
    }

    pBuffer = (CGBuffer* )cBuffer;
    SetCBuffer(pBuffer);
    *pBuffer = mBuffer;
    mpCurPin->SendBuffer(cBuffer);

    if(mBuffer.GetStreamType() == STREAM_VIDEO)
        mBuffer.Dump("FromDemuxer");
    if(mBuffer.GetStreamType() == STREAM_AUDIO)
        mBuffer.Dump("FromDemuxerA");
    mBuffer.Clear();
    return ME_OK;
}

AM_ERR CGeneralDemuxer::SendAudioStream(AM_INT index)
{
    AM_ERR err;
    CBuffer* cBuffer;
    CGBuffer* gBuffer;
    CGBuffer oBuffer;
    AM_INT sendNum = 0;
    while(1)
    {
        //AM_INFO("SendGBufferQueue, %d\n", sendNum);
        if(mpAudioBufferPool->GetFreeBufferCnt() <= 0)
            break;
        err = mpDemuxer[index]->GetAudioBuffer(oBuffer);
        if(err != ME_OK)
            break;

        if (!mpAudioPin->AllocBuffer(cBuffer)) {
            AM_ASSERT(0);
            AM_ERROR("CGeneralDecoder Alloc output buffer Fail!\n");
            return ME_ERROR;
        }
        gBuffer = (CGBuffer* )cBuffer;
        oBuffer.mpPool = gBuffer->mpPool;
        oBuffer.mpNext = gBuffer->mpNext;
        oBuffer.mRefCount = gBuffer->mRefCount;
        *gBuffer = oBuffer;
        oBuffer.Clear();
        mpAudioPin->SendBuffer(cBuffer);
        sendNum++;
    }
    if(sendNum != 0){
        //AM_INFO("Send Audio %d, bp free:%d\n", sendNum, mpAudioBufferPool->GetFreeBufferCnt());
        return ME_OK;
    }
    return ME_IO_ERROR;
}

//Buffer Release from BP
AM_ERR CGeneralDemuxer::ReleaseBuffer(CBuffer* buffer)
{
    CGBuffer* gBuffer = (CGBuffer* )buffer;
    AM_INT index = gBuffer->GetIdentity();
    //AM_INFO("Debug Release on GDemuxer:%d.\n", index);
    AM_ASSERT(mpDemuxer[index]);
    mpDemuxer[index]->OnReleaseBuffer(gBuffer);
    //can be removed
    gBuffer->Clear();
    return ME_OK;
}

AM_ERR CGeneralDemuxer::QueryInfo(AM_INT target, AM_INT type, CParam64& param)
{
    AM_ERR err;
    if(target < 0 || target >= MDEC_SOURCE_MAX_NUM){
        AM_ERROR("QueryInfo target(%d) Wrong!\n", target);
        return ME_BAD_PARAM;
    }
    if(mpDemuxer[target] == NULL){
        AM_ERROR("QueryInfo target(%d) No Exist!\n", target);
        return ME_NOT_EXIST;
    }
    err = mpDemuxer[target]->QueryInfo(type, param);
    return err;
}

AM_ERR CGeneralDemuxer::GetStreamInfo(AM_INT index, CParam & param)
{
    AM_ERR err;
    AM_INT type = VIDEO_WIDTH_HEIGHT;
    CParam64 par64(2);
    if(mpDemuxer[index]){
        err = mpDemuxer[index]->QueryInfo(type, par64);
        param[0] = par64[0];
        param[1] = par64[1];
    }else{
        err = ME_NO_IMPL;
    }
    return err;
}

void CGeneralDemuxer::Dump(AM_INT flag)
{
    AM_INFO("\n---------------------GDemuxer Info------------\n");
    AM_INFO("       BufferPool--->VideoBp FreeDataCnt:%d,  AudioBp FreeDataCnt:%d\n"
        ,mpVideoBufferPool->GetFreeBufferCnt(), mpAudioBufferPool->GetFreeBufferCnt());
    AM_INFO("       State---> on state:%d, cur BufferType:%d\n", msState, mBuffer.GetStreamType());
    AM_INT i;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        if(mpDemuxer[i] != NULL)
            mpDemuxer[i]->Dump(flag);
    }
    AM_INFO("\n");
};
//---------------------------------------------------------------------------
//Discarded function
//NeedAdvanceOut can select a suitable output to transmit data.
//RetrieveBuffer can retrieve the buffer and buffered on demuxer module.
//---------------------------------------------------------------------------
AM_BOOL CGeneralDemuxer::NeedAdvanceOut()
{
/*    STREAM_TYPE type = mBuffer.GetStreamType();
    CGeneralBufferPool* pBp = (type == STREAM_AUDIO) ? mpVideoBufferPool : mpAudioBufferPool;
    AM_INT bpSize = (type == STREAM_AUDIO) ? GDEMUXER_VIDEO_Q_NUM : GDEMUXER_AUDIO_Q_NUM;
    AM_ASSERT(pBp != mpCurBufferPool);
    mpExRetrieve[STREAM_AUDIO] = 0;
    mpExRetrieve[STREAM_VIDEO] = 0;
    AM_INT aIndex = mpConfig->GetAudioSource();

    if(pBp->GetFreeBufferCnt() >= (bpSize /2))
    {
        if(pBp == mpAudioBufferPool && aIndex == -1)
        {
        }else{
            mExtractType = (type == STREAM_AUDIO) ? STREAM_VIDEO : STREAM_AUDIO;
            AM_INFO("Read Policy on GDM::%d\n", mExtractType);
            return AM_TRUE;
        }
    }
            //AM_INFO("Debug, %d, %d, %d, waittype:%d\n", pBp->GetFreeBufferCnt(), bpSize, aIndex,type);
    mExtractType = STREAM_NULL;
    */
    return AM_FALSE;
}

//Buffer Retrieve from BP to demuxer
AM_ERR CGeneralDemuxer::RetrieveBuffer(CBuffer* buffer)
{
    CGBuffer* gBuffer = (CGBuffer* )buffer;
    //gBuffer->Dump("Retrieve");
    AM_INT index = gBuffer->GetIdentity();
    AM_ASSERT(mpDemuxer[index]);
    mpDemuxer[index]->OnRetrieveBuffer(gBuffer);
    return ME_OK;
}


