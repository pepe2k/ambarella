/*
 * general_decode_filter.cpp
 *
 * History:
 *    2010/6/1 - [QingXiong Z] create file
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

#include "general_decoder_filter.h"

CGeneralDecoder* CreateGeneralDecoderFilter(IEngine* pEngine, CGConfig* config)
{
    return CGeneralDecoder::Create(pEngine, config);
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
CGeneralDecoder* CGeneralDecoder::Create(IEngine *pEngine, CGConfig* pconfig)
{
    CGeneralDecoder* result = new CGeneralDecoder(pEngine, pconfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CGeneralDecoder::Construct()
{
    AM_INFO("--->CGeneralDecoder::Construct .\n");
    AM_ERR err = inherited::Construct();
    if (err != ME_OK) {
        AM_ERROR("CGeneralDecoder::Construct fail err %d .\n", err);
        return err;
    }
    err = ConstructPin();
    if (err != ME_OK) {
        AM_ERROR("CGeneralDecoder::Construct Pin err %d .\n", err);
        return err;
    }
    AM_INFO("<---CGeneralDecoder::Construct . Done.\n");

    SetThreadPrio(1, 1);
    return ME_OK;
}

AM_ERR CGeneralDecoder::ConstructPin()
{
    //Output
    if(mpOutputPin == NULL)
    {
        mpOutputPin = CGeneralOutputPin::Create(this);
        if(mpOutputPin == NULL)
            return ME_ERROR;
        if((mpBufferPool = CGeneralBufferPool::Create("G-Decoder OuputBP", GENERAL_DECODER_CTRL_BP)) == NULL)
            return ME_ERROR;
        mpOutputPin->SetBufferPool(mpBufferPool);
    }
    return ME_OK;
}

AM_ERR CGeneralDecoder::ConstructInputPin()
{
    if(mpInputPin != NULL)
        return ME_OK;
    CGeneralInputPin* pInput = CGeneralInputPin::Create(this);
    if(pInput == NULL)
    {
        AM_INFO("CGeneralInputPin::Create Failed!\n");
    }
    mpInputPin = pInput;
    return ME_OK;
}

void CGeneralDecoder::Delete()
{
    AM_INFO("CGeneralDecoder Delete\n");
    AM_INT i = 0;
    AM_DELETE(mpInputPin);
    mpInputPin = NULL;
    AM_DELETE(mpOutputPin);//will no free the bufferpool, need free bp
    mpOutputPin = NULL;
    AM_DELETE(mpBufferPool);
    mpBufferPool = NULL;

    for(; i < MDEC_SOURCE_MAX_NUM; i++){
        if(mpDecoder[i] != NULL){
            AM_DELETE(mpDecoder[i]);
        }
    }
    inherited::Delete();
}

CGeneralDecoder::~CGeneralDecoder()
{
    AM_INFO("~CGeneralDecoder Start.\n");
    AM_INFO("~CGeneralDecoder Done.\n");
}

void CGeneralDecoder::GetInfo(INFO& info)
{
    info.nInput = 1;
    info.nOutput = 1;
    info.mPriority = 0;
    info.mFlags = 0;
    info.mIndex = -1;
    info.pName = "CGeneralDecoder";
}

IPin* CGeneralDecoder::GetInputPin(AM_UINT index)
{
    AM_INFO("Call CGeneralDecoder::GetInputPin.\n");
    if(index != 0)
        return NULL;
    ConstructInputPin();
    return mpInputPin;
}

IPin* CGeneralDecoder::GetOutputPin(AM_UINT index)
{
    if(mpOutputPin->mpPeer)
    {
        return NULL;
    }
    return mpOutputPin;
}

AM_ERR CGeneralDecoder::SetInputFormat(CMediaFormat* pFormat)
{
    return ME_OK;
}

AM_ERR CGeneralDecoder::SetOutputFormat(CMediaFormat* pFormat)
{
    return ME_OK;
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
AM_ERR CGeneralDecoder::HandDecoderCmd(AM_INT target,  CMD& cmd, AM_BOOL isSend)
{
    AM_INT i;
    if(target == CMD_EACH_COMPONENT){
        for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
            if(mpDecoder[i] != NULL){
                mpDecoder[i]->PerformCmd(cmd, isSend);
            }
        }
        return ME_OK;
    }
    if(target < 0 || target >= MDEC_SOURCE_MAX_NUM){
        AM_ERROR("HandDecoderCmd target(%d) Wrong!\n", target);
        return ME_BAD_PARAM;
    }
    if(mpDecoder[target] != NULL)
        mpDecoder[target]->PerformCmd(cmd, isSend);
    else
        AM_ERROR("No Instance for target:%d", target);
    return ME_OK;
}

AM_ERR CGeneralDecoder::DoStop()
{
    AO::CMD cmd(CMD_STOP);
    HandDecoderCmd(CMD_EACH_COMPONENT, cmd, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralDecoder::DoPause(AM_INT target, AM_U8 flag)
{
    if(target == CMD_GENERAL_ONLY){
        if(msState != STATE_PENDING){
            msOldState = msState;
            msState = STATE_PENDING;
        }
        return ME_OK;
    }
    CMD cmd(CMD_PAUSE);
    HandDecoderCmd(target, cmd, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralDecoder::DoResume(AM_INT target, AM_U8 flag)
{
    if(target == CMD_GENERAL_ONLY){
        if(flag & RESUME_TO_IDLE){
            CMD cmd(CMD_CLEAR);
            cmd.res32_1 = flag;
            cmd.pExtra = (void* )mpBuffer;
            //pause from ready, need clear the buffer which sent to isready.
            HandDecoderCmd(CMD_EACH_COMPONENT, cmd, AM_TRUE);

            if(mpBuffer != NULL)
            {
                //AM_INFO("MPBUFFER CNT:%d(%d)\n", mpBuffer->GetCount(), mpBuffer->GetIdentity());
                if(flag & RESUME_BUFFER_RETRIEVE){
                    mpBuffer->Dump("DoResume");
                    mpBuffer->Retrieve();
                }else{
                    mpBuffer->Release();
                }
                mpBuffer = NULL;
            }
            msState = STATE_IDLE;
        }else{
            msState = msOldState;
        }
        return ME_OK;
    }

    CMD cmd2(CMD_RESUME);
    HandDecoderCmd(target, cmd2, AM_TRUE);
    return ME_OK;
}

AM_ERR CGeneralDecoder::DoFlush(AM_INT target, AM_U8 flag)
{
    //AM_INFO("CGeneralDecoder Flush%d flag:%d\n", target, flag);
    if(target == CMD_GENERAL_ONLY || mpBuffer != NULL){
        CMD cmd(CMD_CLEAR);
        cmd.res32_1 = 0;
        cmd.pExtra = (void* )mpBuffer;
        HandDecoderCmd(CMD_EACH_COMPONENT, cmd, AM_TRUE);
    }
    if(mpBuffer != NULL)
    {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    if(target != CMD_GENERAL_ONLY){
        CMD cmd2(CMD_FLUSH);
        HandDecoderCmd(target, cmd2, AM_TRUE);
    }
    return ME_OK;
}

AM_ERR CGeneralDecoder::DoConfig(AM_INT target, AM_U8 flag)
{
//    AM_INT i = 0;
    if(flag & CHECK_VDEC_FRAME_NET)
    {
    }
    return ME_OK;
}

//for video may be bug
AM_ERR CGeneralDecoder::DoRemove(AM_UINT index, AM_U8 flag)
{
//    AM_ERR err;
    if(mpDecoder[index] == NULL){
        AM_ASSERT(0);
        return ME_OK;
    }

    if(flag & DEC_REMOVE_JUST_NOTIFY){
        CMD cmd(CMD_DEC_REMOVE);
        HandDecoderCmd(index, cmd, AM_TRUE);
        return ME_OK;
    }

    if(mpBuffer != NULL)
    {
        mpBuffer->Release();
        mpBuffer = NULL;
    }
    //send stop
    AO::CMD cmd(CMD_STOP);
    mpDecoder[index]->PerformCmd(cmd, AM_TRUE);
    AM_DELETE(mpDecoder[index]);
    mpDecoder[index] = NULL;
    //msOldState = STATE_IDLE;
    return ME_OK;
}

void CGeneralDecoder::MsgProc(AM_MSG& msg)
{
    CGBuffer* gBuffer = NULL;
    AM_INT dataNum = 0;
    AM_INT index;
    switch(msg.code)
    {
    case MSG_DECODED:
        gBuffer = (CGBuffer* )(msg.p0);
        dataNum = (AM_INT)(msg.p1);
        if(gBuffer != NULL){
            SendGBuffer(gBuffer);
        }else if(dataNum != 0){
            index = (AM_INT)(msg.p2);
            SendGBufferQueue(index);
        }
        //AM_INFO("--MSG_DECODED\n");
        mpWorkQ->PostMsg(CMD_ACK);
        break;

    case MSG_READY:
        //AM_INFO("--MSG_READY\n");
        mpWorkQ->PostMsg(CMD_ACK);
        //AM_INFO("--MSG_READY, DOne\n");
        break;

    case MSG_UDEC_ERROR:
        AM_INFO("--MSG_UDEC_ERROR\n");
        PostEngineMsg(msg);
        break;

    case MSG_DECODER_ERROR:
         break;

    //case MSG_DECODER_HANDLE-->Render this decoder.
    case MSG_TEST:
        AMLOG_DEBUG("--MSG_TEST\n");
        ChangeDecoder();
        break;

    case MSG_DSP_GOTO_SHOW_PIC:
        AM_INFO("--MSG_DSP_GOTO_SHOW_PIC\n");
        PostEngineMsg(msg);
        break;

    default:
        AM_ASSERT(0);
        break;
    }
}

bool CGeneralDecoder::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGeneralDecoder::ProcessCmd %d.\n", cmd.code);
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

void CGeneralDecoder::OnRun()
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
            if(mpBufferPool->GetFreeBufferCnt() <= 0)
            {
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
            }else{
                msState = STATE_READY;
            }
            break;

        case STATE_READY:
            AM_ASSERT(mpBuffer);
            if(IsDecoderReady() != ME_OK)
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
        AM_INFO("CGeneralDecoder exit release buffer\n");
        mpBuffer->Release();
        mpBuffer = NULL;
    }
    AM_INFO("CGeneralDecoder OnRun Exit!\n");
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
AM_ERR CGeneralDecoder::IsDecoderReady()
{
    AM_ERR err;
    err = mpDecoder[mCurIndex]->IsReady(mpBuffer);
    AM_ASSERT(err == ME_OK);
    return err;
}

AM_ERR CGeneralDecoder::CreateDecoder()
{
    AM_INFO("CreateDecoder\n");
    AM_INT score, max = 0;

    GDecoderParser** parserList = gDecoderList;
    GDecoderParser* parser = *parserList;
    GDecoderParser* maxParser = NULL;
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
        AM_INFO("No Find Decoder\n");
        return ME_NOT_SUPPORTED;
    }
    AM_INFO("Find GDecoder:%s\n", maxParser->Name);

    mpDecoder[mCurIndex] = maxParser->Creater((IFilter* )this, mpConfig);
    if(mpDecoder[mCurIndex] == NULL){
        AM_INT CREATE_DECODER_FAILED = 0;
        AM_ASSERT(CREATE_DECODER_FAILED);
        return ME_ERROR;
    }
    mpBuffer->Release();
    mpBuffer = NULL;

    AO::CMD cmd(CMD_RUN);
    mpDecoder[mCurIndex]->PerformCmd(cmd, AM_TRUE);
    mDecNum++;
    return ME_OK;
}

AM_ERR CGeneralDecoder::ReadInputData(CQueue::WaitResult& result)
{
    //AM_INFO("CGeneralDecoder--ReadInputData\n");
    CGeneralInputPin* pInput = (CGeneralInputPin* )(result.pOwner);
    AM_ASSERT(pInput == mpInputPin);
    AM_INT dspIndex, index;
    CBuffer* cBuffer;
    if(!pInput->PeekBuffer(cBuffer))
    {
        AM_ERROR("!!PeekBuffer Failed!\n");
        return ME_ERROR;
    }
    mpBuffer = (CGBuffer* )cBuffer;
    index = mpBuffer->GetIdentity();
    dspIndex = mpConfig->indexTable[index].sourceGroup;
    if(mpBuffer->GetBufferType() == HANDLE_BUFFER)
    {
        if(mpBuffer->GetStreamType() == STREAM_VIDEO){
            mAudioGDE = false;
            mCurIndex = dspIndex;
            if(mpBuffer->GetFlags() == HD_STREAM_HANDLE){
                mCurIndex = mHDDecoder;
            }
            mpConfig->SetMapDec(index, mCurIndex); //map before create! or will release a null dec
            if(mpDecoder[mCurIndex] == NULL){
                CreateDecoder();
            }else{
                mpDecoder[mCurIndex]->AdaptStream(mpBuffer);
            }
        }else if(mpBuffer->GetStreamType() == STREAM_AUDIO){
            mAudioGDE = true;
            mCurIndex = index;
            for(int k= 0; k < MDEC_SOURCE_MAX_NUM; k++)
                AM_ASSERT(mpDecoder[k] == NULL);
            CreateDecoder();
        }
        if(mpBuffer != NULL){
            mpBuffer->Release();
            mpBuffer = NULL;
        }
        return ME_TRYNEXT;
    }

    //Pause specify source
    //if(mpConfig->demuxerConfig[mCurIndex].paused == AM_TRUE)
    AM_UINT pauseCnt = mpConfig->demuxerConfig[mCurIndex].pausedCnt;
    if(pauseCnt > 0 && (mpBuffer->GetCount() < pauseCnt))
    {
        AM_INFO("GDE Handle Pause:%d\n", mpBuffer->GetIdentity());
        if(mpBuffer->GetStreamType() == STREAM_VIDEO)
            mpBuffer->Retrieve();
        else
            mpBuffer->Release();
        mpBuffer = NULL;
        return ME_CLOSED;
    }

    AM_ASSERT(mpDecoder[mCurIndex]);
    //AMLOG_DEBUG("ReadInputData mpBuffer %p.\n", mpBuffer);
    return ME_OK;
}

AM_ERR CGeneralDecoder::ProcessBuffer()
{
    AM_ERR err;
    AM_ASSERT(mpBuffer->GetBufferType() != DATA_BUFFER);

    if(mpBuffer->GetBufferType() == EOS_BUFFER)
    {
        err = ProcessEOS();
        return err;
    }
    return ME_OK;
}

AM_ERR CGeneralDecoder::ProcessEOS()
{
    //AM_INFO("CGeneralDecoder ProcessEOS\n");
    AM_ERR err;
    err = mpDecoder[mCurIndex]->FillEOS();
    AM_VERBOSE("err=%d\n", err);
    mpBuffer->Release();
    mpBuffer = NULL;
    return ME_OK;
}

AM_ERR CGeneralDecoder::SendGBuffer(CGBuffer* oBuffer)
{
    CBuffer* cBuffer;
    CGBuffer* gBuffer;
    if (!mpOutputPin->AllocBuffer(cBuffer)) {
        AM_ERROR("CGeneralDecoder Alloc output buffer Fail!\n");
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

AM_ERR CGeneralDecoder::ReleaseBuffer(CBuffer* buffer)
{
    CGBuffer* gBuffer = (CGBuffer* )buffer;
    AM_INT index = gBuffer->GetIdentity();

    if(gBuffer->GetStreamType() == STREAM_VIDEO){
        AM_INT decIndexMap = mpConfig->indexTable[index].decIndex;
        //AM_INFO("dEBUG: %d, %d\n", decIndexMap, index);
        AM_ASSERT(mpDecoder[decIndexMap]);
        mpDecoder[decIndexMap]->OnReleaseBuffer(gBuffer);
        //AM_INFO("dONE \n");
    }else{
        AM_ASSERT(mpDecoder[index]);
        mpDecoder[index]->OnReleaseBuffer(gBuffer);
    }
    return ME_OK;
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
AM_ERR CGeneralDecoder::TryAgain(AM_INT target)
{
    AM_ERR err;
    if(target < 0 || target >= MDEC_SOURCE_MAX_NUM){
        AM_ERROR("TryAgain target(%d) Wrong!\n", target);
        return ME_BAD_PARAM;
    }
    CMD cmd(CMD_RETRY_DECODE);
    HandDecoderCmd(target, cmd, AM_TRUE);
    return err;
}

AM_ERR CGeneralDecoder::QueryInfo(AM_INT target, AM_INT type, CParam64& param)
{
    AM_ERR err = ME_OK;
    if(target < 0 || target >= MDEC_SOURCE_MAX_NUM){
        AM_ERROR("QueryInfo target(%d) Wrong!\n", target);
        return ME_BAD_PARAM;
    }
    if(mpDecoder[target] != NULL)
        err = mpDecoder[target]->QueryInfo(type, param);

    return err;
}

AM_ERR CGeneralDecoder::UpdatePBSpeed(AM_INT target, AM_U8 pb_speed, AM_U8 pb_speed_frac, IParameters::DecoderFeedingRule feeding_rule)
{
    AM_ERR err = ME_OK;
    CMD cmd;

    if (AllTargetTag == target) {
        //to all targets
        AM_UINT i = 0;
        for (i = 0; i < MDEC_SOURCE_MAX_NUM; i ++) {
            if (NULL != mpDecoder[i]) {
                cmd.code = CMD_DEC_PB_SPEED;
                cmd.res32_1 = (pb_speed << 8) | (pb_speed_frac);
                cmd.res64_1 = feeding_rule;
                err = mpDecoder[i]->PerformCmd(cmd, false);
            }
        }
        return ME_OK;
    }

    if (target < 0 || target >= MDEC_SOURCE_MAX_NUM) {
        AM_ERROR("UpdatePBSpeed target(%d) Wrong!\n", target);
        return ME_BAD_PARAM;
    }

    if (NULL != mpDecoder[target]) {
        cmd.code = CMD_DEC_PB_SPEED;
        cmd.res32_1 = (pb_speed << 8) | (pb_speed_frac);
        cmd.res64_1 = feeding_rule;
        err = mpDecoder[target]->PerformCmd(cmd, false);
    }

    return err;
}

void CGeneralDecoder::Dump()
{
    AM_INT index = -1;
    if(mpBuffer != NULL)
    {
        index = mpBuffer->GetIdentity();

    }
    AM_INFO("\n---------------------GDecoder Info------------\n");
    AM_INFO("       BufferPool--->FreeDataCnt:%d\n",
        mpBufferPool->GetFreeBufferCnt());
    AM_INFO("       Input Info--->Has input DataCnt:%d\n",
        mpInputPin->GetDataCnt());
    AM_INFO("       State---> on state:%d, CurIndex:%d\n", msState, index);
    AM_INT i;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        if(mpDecoder[i] != NULL)
            mpDecoder[i]->Dump();
    }
    AM_INFO("\n");
}
//---------------------------------------------------------------------------
//Discard this interaction between GeneralDecoder and Decoder Module.
//Donot call Ready and Decode on the module. Module decode data auto.
//---------------------------------------------------------------------------
AM_ERR CGeneralDecoder::SendGBufferQueue(AM_INT deIndex)
{
    AM_ERR err;
    CBuffer* cBuffer;
    CGBuffer* gBuffer;
    CGBuffer oBuffer;
    AM_INT sendNum = 0;
    while(1)
    {
        //AM_INFO("SendGBufferQueue, %d\n", sendNum);
        if(mpBufferPool->GetFreeBufferCnt() <= 0)
            break;
        err = mpDecoder[deIndex]->GetDecodedGBuffer(oBuffer);
        if(err != ME_OK)
            break;

        if (!mpOutputPin->AllocBuffer(cBuffer)) {
            AM_ASSERT(0);
            AM_ERROR("CGeneralDecoder Alloc output buffer Fail!\n");
            return ME_ERROR;
        }
        gBuffer = (CGBuffer* )cBuffer;
        oBuffer.mpPool = gBuffer->mpPool;
        oBuffer.mpNext = gBuffer->mpNext;
        oBuffer.mRefCount = gBuffer->mRefCount;
        *gBuffer = oBuffer;
        //oBuffer.Dump("DecodeRead");
        oBuffer.Clear();
        mpOutputPin->SendBuffer(cBuffer);
        sendNum++;
    }
    if(sendNum != 0)
        return ME_OK;
    return ME_IO_ERROR;
}

AM_ERR CGeneralDecoder::ChangeDecoder()
{
    /*AM_ERR err;
    err = Stop();
    AM_DELETE(mpVDecoder);
    //DoStop();
    mpVDecoder = CVideoDecoderDsp::Create((IFilter* )this, mpMediaFormat);
    if(mpVDecoder == NULL)
    {
        AM_INFO("ChangeDecoder Failed!\n");
        return ME_ERROR;
    }
    mpWorkQ->SendCmd(CMD_RUN);
    AM_INFO("ChangeDecoder Done!\n");
    */
    return ME_OK;
}

AM_ERR CGeneralDecoder::ReSet()
{
    return ME_OK;
    /*
    AM_ERR err;
    err = ((CVideoDecoderDsp* )mpVDecoder)->ReSet();
    err = Stop();
    err = mpWorkQ->SendCmd(CMD_RUN);
    return err;
    */
}


