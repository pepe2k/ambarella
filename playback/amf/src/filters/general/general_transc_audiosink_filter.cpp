/*
 * general_transc_audiosink_filter.cpp
 *
 * History:
 *    2010/11/12 - [GLiu] create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"

#include "general_transcoder_filter.h"
#include "general_transc_audiosink_filter.h"

CGeneralTransAudioSink* CreateGeneralTransAudioSinkFilter(IEngine* pEngine, CGConfig* config)
{
    return CGeneralTransAudioSink::Create(pEngine, config);
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
CGeneralTransAudioSink* CGeneralTransAudioSink::Create(IEngine *pEngine, CGConfig* pconfig)
{
    CGeneralTransAudioSink* result = new CGeneralTransAudioSink(pEngine, pconfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CGeneralTransAudioSink::Construct()
{
    AM_INFO("--->CGeneralTransAudioSink::Construct .\n");
    AM_ERR err = inherited::Construct();
    if (err != ME_OK) {
        AM_ERROR("CGeneralTransAudioSink::Construct fail err %d .\n", err);
        return err;
    }
    err = ConstructPin();
    if (err != ME_OK) {
        AM_ERROR("CGeneralTransAudioSink::Construct Pin err %d .\n", err);
        return err;
    }
    AM_INFO("<---CGeneralTransAudioSink::Construct . Done.\n");

    SetThreadPrio(1, 1);
    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::ConstructPin()
{
    //Output
    if(mpOutputPin == NULL) {
        mpOutputPin = CGeneralOutputPin::Create(this);
        if(mpOutputPin == NULL)
            return ME_ERROR;
        if((mpBufferPool = CGeneralBufferPool::Create("G-TransAudioSink OuputBP", 16)) == NULL)
            return ME_ERROR;
        mpOutputPin->SetBufferPool(mpBufferPool);
    }
    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::ConstructInputPin()
{
    if(mpInputPin != NULL)
        return ME_OK;
    CGeneralInputPin* pInput = CGeneralInputPin::Create(this);
    if(pInput == NULL)
    {
        AM_INFO("CGeneralTransAudioSink:Create InputPin Failed!\n");
    }
    mpInputPin = pInput;
    return ME_OK;
}

void CGeneralTransAudioSink::Delete()
{
    AM_INFO("CGeneralTransAudioSink Delete\n");
    AM_DELETE(mpInputPin);
    mpInputPin = NULL;
    AM_DELETE(mpOutputPin);//will no free the bufferpool, need free bp
    mpOutputPin = NULL;
    AM_DELETE(mpBufferPool);
    mpBufferPool = NULL;

    inherited::Delete();
}

CGeneralTransAudioSink::~CGeneralTransAudioSink()
{
    AM_INFO("~CGeneralTransAudioSink Start.\n");
    for(int i=0; i<MDEC_SOURCE_MAX_NUM;i++){
        if(mpInputQ[i] != NULL){
            mpInputQ[i]->Detach();
            mpInputQ[i] = NULL;
        }
    }
    AM_INFO("~CGeneralTransAudioSink Done.\n");
}

void CGeneralTransAudioSink::GetInfo(INFO& info)
{
    info.nInput = 1;
    info.nOutput = 1;
    info.mPriority = 0;
    info.mFlags = 0;
    info.mIndex = -1;
    info.pName = "CGeneralTransAudioSink";
}

IPin* CGeneralTransAudioSink::GetInputPin(AM_UINT index)
{
    AM_INFO("Call CGeneralTransAudioSink::GetInputPin.\n");
    if(index != 0)
        return NULL;
    ConstructInputPin();
    return mpInputPin;
}

IPin* CGeneralTransAudioSink::GetOutputPin(AM_UINT index)
{
    return mpOutputPin;
}

AM_ERR CGeneralTransAudioSink::SetInputFormat(CMediaFormat* pFormat)
{
    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::SetOutputFormat(CMediaFormat* pFormat)
{
    return ME_OK;
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
AM_ERR CGeneralTransAudioSink::HandCmd(AM_INT target,  CMD& cmd, AM_BOOL isSend)
{
    AM_INFO("HandCmd:%d  %d\n", target, cmd.code);
    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::DoStop()
{
    AO::CMD cmd(CMD_STOP);
    HandCmd(CMD_EACH_COMPONENT, cmd, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::DoPause(AM_INT target, AM_U8 flag)
{
    if(target == CMD_GENERAL_ONLY){
        if(msState != STATE_PENDING){
            msOldState = msState;
            msState = STATE_PENDING;
        }
        AM_INFO("CGeneralTransAudioSink::DoPause: state %d->%d\n", msOldState, STATE_PENDING);
        return ME_OK;
    }
    CMD cmd(CMD_PAUSE);
    HandCmd(target, cmd, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::DoResume(AM_INT target, AM_U8 flag)
{
    if(target == CMD_GENERAL_ONLY){
        if(msState == STATE_PENDING){
            if(flag & RESUME_TO_IDLE){
                msState = STATE_IDLE;
            }else{
                msState = msOldState;
            }
            AM_INFO("CGeneralTransAudioSink::DoResume: state %d->%d\n", STATE_PENDING, msState);
        }
        return ME_OK;
    }
    CMD cmd2(CMD_RESUME);
    HandCmd(target, cmd2, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::DoFlush(AM_INT target, AM_U8 flag)
{
    AM_INFO("CGeneralTransAudioSink Flush%d flag:%d\n", target, flag);
    CMD cmd2(CMD_FLUSH);
    HandCmd(target, cmd2, AM_TRUE);

    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::DoConfig(AM_INT target, AM_U8 flag)
{
//    AM_INT i = 0;
    if(flag & CHECK_VDEC_FRAME_NET)
    {
    }
    return ME_OK;
}

//for video may be bug
AM_ERR CGeneralTransAudioSink::DoRemove(AM_UINT index, AM_U8 flag)
{
//    AM_ERR err;
    AM_INT num = 0;
    AM_ASSERT((AM_INT)index==mCurIndex);
    if(mpBuffer){
        free(mpBuffer);
        mpBuffer = NULL;
    }
    mpBuffer = (CGBuffer*)malloc(sizeof(CGBuffer));
    while(mpInputQ[index]->PeekData(mpBuffer, sizeof(CGBuffer))) {
        ((IGDemuxer*)mpInputQOwner[index])->OnReleaseBuffer(mpBuffer);
        if(mpBuffer) free(mpBuffer);
        mpBuffer = NULL;
        mpBuffer = (CGBuffer*)malloc(sizeof(CGBuffer));
        num++;
    }
    if(mpBuffer){
        free(mpBuffer);
        mpBuffer = NULL;
    }
    AM_INFO("CGeneralTransAudioSink::DoRemove free %d frames in inputpin[%d]---------------\n", num, index);
    return ME_OK;
}

void CGeneralTransAudioSink::MsgProc(AM_MSG& msg)
{
    switch(msg.code)
    {
    default:
        AM_ASSERT(0);
        break;
    }
}

bool CGeneralTransAudioSink::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGeneralTransAudioSink::ProcessCmd %d.\n", cmd.code);
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
        if(msState == STATE_WAIT_OUTPUT)
        {
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
        //AM_ASSERT(msState == STATE_PENDING);
        par = cmd.res32_1;
        flag = cmd.flag;
        DoFlush(par, flag);
        CmdAck(ME_OK);
        break;

    case CMD_CONFIG:
        par = cmd.res32_1;
        flag = cmd.flag;
        DoConfig(par, flag);
        CmdAck(ME_OK);
        break;

    case CMD_AVSYNC:
        CmdAck(ME_OK);
        break;

    case CMD_ACK:
        //CmdAck(ME_OK);
        break;

    case CMD_BEGIN_PLAYBACK:
        AM_ASSERT(msState == STATE_PENDING);
        AM_INFO("CMD_BEGIN_PLAYBACK, the msState: %d, oldmsState: %d\n", msState, msOldState);
        msState = STATE_IDLE;
        mbStreamStart = false;
        //mbPaused = false;
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

    return true;
}

void CGeneralTransAudioSink::OnRun()
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
        //AM_INFO("Amba Decoder %p: start switch, msState=%d\n", this, msState);
        switch (msState)
        {
        case STATE_IDLE:
            type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
            if(type == CQueue::Q_MSG)
            {
                ProcessCmd(cmd);
            }else{
                if(ReadInputData(result) == ME_OK)
                {
                    msState = STATE_HAS_INPUTDATA;
                }else{
                    usleep(20000);
                }
            }
            break;

        case STATE_HAS_INPUTDATA:
            AM_ASSERT(mpBuffer);
            if(mpBufferPool->GetFreeBufferCnt() > 0)
            {
                msState = STATE_READY;
            }else{
                msState = STATE_WAIT_OUTPUT;
            }
            break;

        case STATE_WAIT_OUTPUT:
            AM_ASSERT(mpBuffer);
            if(mpBufferPool->GetFreeBufferCnt() > 0){
                msState = STATE_READY;
            }
            break;

        case STATE_READY:
            AM_ASSERT(mpBuffer);
            ProcessBuffer();
            msState = STATE_IDLE;
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
        AM_INFO("CGeneralTransAudioSink exit release buffer\n");
        //free(mpBuffer);
        mpBuffer = NULL;
    }
    AM_INFO("CGeneralTransAudioSink OnRun Exit!\n");
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
AM_ERR CGeneralTransAudioSink::ReadInputData(CQueue::WaitResult& result)
{
    //AM_INFO("CGeneralTransAudioSink--ReadInputData\n");
    AM_INT index;
    if((CGeneralInputPin* )(result.pOwner) == mpInputPin) {
        AM_INFO("CGeneralTransAudioSink ReadInputData: get data from inputpin %p\n", mpInputPin);
        CBuffer* cBuffer;
        CGeneralInputPin* pInput = (CGeneralInputPin* )(result.pOwner);
        if(!pInput->PeekBuffer(cBuffer))
        {
            AM_ERROR("!!PeekBuffer Failed!\n");
            return ME_ERROR;
        }
        mpBuffer = (CGBuffer*)cBuffer;
        AM_INFO("CGeneralTransAudioSink-- %p, buffertype %d\n", pInput, mpBuffer->GetBufferType());
        if(mpBuffer->GetBufferType() == HANDLE_BUFFER) {
            index = mpBuffer->GetIdentity();
            AM_ASSERT(mpBuffer->GetStreamType() == STREAM_AUDIO);
            if(mpBuffer->GetStreamType() == STREAM_AUDIO){
                mCurIndex = index;
                CQueue* cmdQ = MsgQ();
                CQueue* pDataQ = (CQueue* )(mpBuffer->GetExtraPtr());
                if(pDataQ==NULL){
                    AM_ERROR("CGeneralTransAudioSink get pDataQ==NULL!\n");
                    return ME_TRYNEXT;
                }
                pDataQ->Attach(cmdQ);
                mpInputQ[mCurIndex] = pDataQ;
                AM_INFO("CGeneralTransAudioSink--HANDLE_BUFFER mpInputQ[%d] = %p\n", index, mpInputQ[index]);
            }
            if(mpBuffer != NULL){
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            AM_MSG msg;
            msg.p1 = mCurIndex;
            msg.code = MSG_RENDER_ADD_AUDIO;
            PostEngineMsg(msg);
            return ME_TRYNEXT;
        }
    }else{
        if((mCurIndex = DataFromDataQ((CQueue* )(result.pDataQ)))>=0) {
            if(mpInputQOwner[mCurIndex]==NULL){
                mpInputQOwner[mCurIndex] = result.pOwner;
                AM_INFO("CGeneralTransAudioSink--mpInputQOwner[%d] = %p\n", mCurIndex, mpInputQOwner[mCurIndex]);
            }

            AM_BOOL rval;
            //AM_INFO("CGeneralTransAudioSink--mpInputQ[%d] %p PeekData\n", mCurIndex, mpInputQ[mCurIndex]);
            mpBuffer = (CGBuffer*)malloc(sizeof(CGBuffer));
            rval = mpInputQ[mCurIndex]->PeekData(mpBuffer, sizeof(CGBuffer));
            //AM_INFO("CGeneralTransAudioSink--mpInputQ[%d] PeekData done, %d\n", mCurIndex, rval);
            AM_ASSERT(rval==AM_TRUE);
        }else{
            AM_ERROR("CGeneralTransAudioSink::ReadInputData UNKNOW MSG: result DataQ: %p, Owner: %p.\n", result.pDataQ, result.pOwner);
        }
    }

    //AM_INFO("CGeneralTransAudioSink--data type: %d, OwnerType: %d, StreamType: %d, cnt %u\n",
    //    mpBuffer->GetBufferType(), mpBuffer->GetOwnerType(), mpBuffer->GetStreamType(),
    //    mpBuffer->GetCount());

    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::ProcessBuffer()
{
    AM_ERR err;
    if(mpBuffer->GetBufferType() == EOS_BUFFER)
    {
        err = ProcessEOS();
        return err;
    }
    if(mpBuffer->GetBufferType() == DATA_BUFFER)
    {
        err = SendGBufferQueue(mCurIndex);
        return err;
    }
    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::ProcessEOS()
{
    //AM_INFO("CGeneralTransAudioSink ProcessEOS\n");
    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::SendGBuffer(CGBuffer* oBuffer)
{
    CBuffer* cBuffer;
    CGBuffer* gBuffer;
    if (!mpOutputPin->AllocBuffer(cBuffer)) {
        AM_ERROR("CGeneralTransAudioSink Alloc output buffer Fail!\n");
        return ME_ERROR;
    }
    //save universal env
    oBuffer->mpPool = cBuffer->mpPool;
    oBuffer->mpNext = cBuffer->mpNext;
    oBuffer->mRefCount = cBuffer->mRefCount;

    gBuffer = (CGBuffer* )cBuffer;
    *gBuffer = *oBuffer;

    mpOutputPin->SendBuffer((CBuffer*)gBuffer);
    return ME_OK;
}

AM_ERR CGeneralTransAudioSink::ReleaseBuffer(CBuffer* buffer)
{
    //CGBuffer* gBuffer = (CGBuffer* )buffer;
    //AM_INT index = gBuffer->GetIdentity();

    return ME_OK;
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
AM_ERR CGeneralTransAudioSink::QueryInfo(AM_INT target, AM_INT type, CParam64& param)
{
    AM_ERR err = ME_OK;
    return err;
}

AM_ERR CGeneralTransAudioSink::UpdatePBSpeed(AM_INT target, AM_U8 pb_speed, AM_U8 pb_speed_frac, IParameters::DecoderFeedingRule feeding_rule)
{
    AM_ERR err = ME_OK;
    return err;
}

void CGeneralTransAudioSink::Dump()
{
    AM_INT index = -1;
    if(mpBuffer != NULL)
    {
        index = mpBuffer->GetIdentity();

    }
    AM_INFO("\n---------------------CGeneralTransAudioSink Info------------\n");
    AM_INFO("       BufferPool--->FreeDataCnt:%d\n",
        mpBufferPool->GetFreeBufferCnt());
    AM_INFO("       Input Info--->Has input DataCnt:%d\n",
        mpInputPin->GetDataCnt());
    AM_INFO("       State---> on state:%d, CurIndex:%d\n", msState, index);
    AM_INT i;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        if(mpInputQ[i] != NULL){
            AM_INFO("       mpInputQ(%d): %p, mpInputQOwner: %p\n", i, mpInputQ[i], mpInputQOwner[i]);
        }
    }
    AM_INFO("\n");
}

AM_ERR CGeneralTransAudioSink::SendGBufferQueue(AM_INT index)
{
    //AM_ERR err;
    CBuffer* cBuffer;
    //AM_INT sendNum = 0;

    //AM_INFO("SendGBufferQueue, %d\n", sendNum);
    if(mpBufferPool->GetFreeBufferCnt() <= 0){
        AM_INFO("       CGeneralTransAudioSink::SendGBufferQueue---------------\n");
        ((IGDemuxer*)mpInputQOwner[index])->OnReleaseBuffer(mpBuffer);
        free(mpBuffer);
        mpBuffer = NULL;
        return ME_OK;
    }

    if(index<0 || index>MDEC_SOURCE_MAX_NUM || mpInputQ[index]==NULL){
        AM_ERROR("Wrong mpInputQ index %d", index);
        return ME_ERROR;
    }

    if (!mpOutputPin->AllocBuffer(cBuffer)) {
        AM_ASSERT(0);
        AM_ERROR("CGeneralTransAudioSink Alloc output buffer Fail!\n");
        return ME_ERROR;
    }
    //AM_INFO("       CGeneralTransAudioSink::SendGBufferQueue(%d) PTS: %llu  %p %p++++++++++++\n",
    //    mpBuffer->GetCount(), mpBuffer->GetPTS(), mpBuffer, cBuffer);
    cBuffer->SetExtraPtr((AM_INTPTR) mpInputQOwner[index]);
    cBuffer->SetDataPtr((AM_U8*)mpBuffer);
    mpOutputPin->SendBuffer(cBuffer);

    if(mpTranscoderContext) mpTranscoderContext->ProcessAudioBuffer();
    mpBuffer = NULL;
    return ME_OK;
}

