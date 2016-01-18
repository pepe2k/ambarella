/*
 * pridata_composer.cpp
 *
 * History:
 *    2011/11/09 - [Zhi He] create file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_NDEBUG 0
#define LOG_TAG "pridata_composer"
#define AMDROID_DEBUG

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "record_if.h"
#include "engine_guids.h"
#include "am_record_if.h"
#include "am_util.h"
#include "am_pridata.h"
#include "pridata_composer.h"

#define DGetPridataDuration(x) (mOverSampling<2)? x:(x/mOverSampling)


IFilter* CreatePridataComposer(IEngine * pEngine)
{
    return CPridataComposer::Create(pEngine);
}

IFilter* CPridataComposer::Create(IEngine * pEngine)
{
    CPridataComposer * result = new CPridataComposer(pEngine);

    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CPridataComposer::Construct()
{
    AM_ERR err = inherited::Construct();
    DSetModuleLogConfig(LogModulePridataComposer);
    if (err != ME_OK) {
        AM_ERROR("inherited::Construct() fail in CPridataComposer::Construct().\n");
        return err;
    }

    if ((mpBufferPool = CSimpleBufferPool::Create("PridataBufferPool", 32, sizeof(CBuffer) + sizeof(SPriDataHeader) + DMAX_PRIDATA_LENGTH)) == NULL) {
        AM_ERROR("!!!not enough memory? CSimpleBufferPool::Create fail in CPridataComposer::Construct().\n");
        return ME_NO_MEMORY;
    }

    mpClockManager = (IClockManager *)mpEngine->QueryEngineInterface(IID_IClockManagerExt);
    if(!mpClockManager) {
        AM_ERROR("CPridataComposer without mpClockManager?\n");
        return ME_ERROR;
    }

    return ME_OK;
}

CPridataComposer::~CPridataComposer()
{
    AM_UINT i;
    CDoubleLinkedList::SNode* pnode;
    void* p_free;

    AMLOG_DESTRUCTOR("~CPridataComposer start.\n");

    for (i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
        AMLOG_DESTRUCTOR("~CPridataComposer, AM_DELETE(mpOutput[%d]) start, mpOutput[%d] = %p.\n", i, i, mpOutput[i]);
        AM_DELETE(mpOutput[i]);
        mpOutput[i] = NULL;
        AMLOG_DESTRUCTOR("~CPridataComposer, AM_DELETE(mpOutput[%d]) done.\n", i);
    }

    AMLOG_DESTRUCTOR("~CPridataComposer, AM_RELEASE(mpBufferPool %p) start.\n", mpBufferPool);
    AM_RELEASE(mpBufferPool);
    mpBufferPool = NULL;
    AMLOG_DESTRUCTOR("~CPridataComposer, AM_RELEASE(mpBufferPool) done.\n");

    //find same data type/sub type
    pnode = mPridataList.FirstNode();
    while (pnode) {
        p_free = pnode->p_context;
        if (p_free) {
            free(p_free);
        }
        pnode = mPridataList.NextNode(pnode);
    }

    pnode = mPriDurationList.FirstNode();
    while (pnode) {
        p_free = pnode->p_context;
        if (p_free) {
            free(p_free);
        }
        pnode = mPriDurationList.NextNode(pnode);
    }

}

void CPridataComposer::GetInfo(INFO& info)
{
    info.nInput = 0;
    info.nOutput = mnOutput;
    info.pName = mpName;
}

IPin* CPridataComposer::GetOutputPin(AM_UINT index)
{
    AMLOG_INFO("CPridataComposer::GetOutputPin %d, %p\n", index, mpOutput[index]);
    if (index < MAX_NUM_OUTPUT_PIN) {
        return (IPin*)mpOutput[index];
    }
    return NULL;
}

AM_ERR CPridataComposer::Stop()
{
    AMLOG_INFO("=== CPridataComposer::Stop()\n");
    inherited::Stop();
    return ME_OK;
}

AM_ERR CPridataComposer::AddOutputPin(AM_UINT& index, AM_UINT type)
{
    AM_UINT i;

    AM_ASSERT(type == IParameters::StreamType_PrivateData);
    if (type != IParameters::StreamType_PrivateData) {
        AM_ERROR("CPridataComposer::AddOutputPin only support private data.\n");
        return ME_ERROR;
    }

    for (i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
        if (mpOutput[i] == NULL)
            break;
    }

    if (i >= MAX_NUM_OUTPUT_PIN) {
        AM_ERROR(" max output pin %d (max %d) reached, please check code.\n", i, MAX_NUM_OUTPUT_PIN);
        return ME_ERROR;
    }

    if ((mpOutput[i] = CPridataComposerOutput::Create(this)) == NULL)
        return ME_NO_MEMORY;

    AM_ASSERT(mpBufferPool);
    mpOutput[i]->SetBufferPool(mpBufferPool);

    index = i;
    mnOutput ++;
    return ME_OK;
}

AM_ERR CPridataComposer::SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index)
{
    AM_ASSERT(type == StreamType_PrivateData);
    AMLOG_WARN(" please implement CPridataComposer::SetParameters.\n");
    return ME_OK;
}

void CPridataComposer::SendoutBuffer(CBuffer* pBuffer)
{
    AM_ASSERT(pBuffer);
    if (NULL == pBuffer) {
        AM_ERROR("NULL pointer in CPridataComposer::SendoutBuffer.\n");
        return;
    }

    //send out buffer
    for (AM_UINT i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
        if (mpOutput[i] == NULL)
            continue;
        pBuffer->AddRef();
        mpOutput[i]->SendBuffer(pBuffer);
    }
}

AM_ERR CPridataComposer::SetGetPridataCallback(AM_CallbackGetUnPackedPriData f_unpacked, AM_CallbackGetPackedPriData f_packed)
{
    mWorkMode = ePridataComposerWorkMode_active;
    mpfUnPackedCallBack = f_unpacked;
    mpfPackedCallBack = f_packed;
    return ME_OK;
}

void CPridataComposer::getDurationStruct(SPriDataPacket* packet)
{
//    AM_UINT i = 0;
    CDoubleLinkedList::SNode* pnode;
    SPriDataDuration* data_duration;

    //find same data type/sub type
    pnode = mPriDurationList.FirstNode();
    while (pnode) {
        data_duration = (SPriDataDuration*) pnode->p_context;

        //replace the duration
        if (data_duration->data_type == packet->data_type) {
            packet->p_cooldown = data_duration;
            return;
        }
        AMLOG_DEBUG("data_duration %p, duration %llu, cooldown %llu, data_type %d.\n", data_duration, data_duration->duration, data_duration->cool_down, data_duration->data_type);

        pnode = mPriDurationList.NextNode(pnode);
    }

    //new data type comes, add it to list
    data_duration = (SPriDataDuration*)malloc(sizeof(SPriDataDuration));
    if (NULL == data_duration) {
        AM_ERROR("NO MEMORY.\n");
        return;
    }

    data_duration->data_type = packet->data_type;
    data_duration->duration = DefaultPridataDuration;
    data_duration->cool_down = DefaultPridataDuration;
    data_duration->need_write = 0;

    AMLOG_DEBUG("CPridataComposer::getDurationStruct, new data_duration %p.\n", data_duration);

    packet->p_cooldown = data_duration;
    //add packet to list
    mPriDurationList.InsertContent(NULL, data_duration, 0);

}

AM_ERR CPridataComposer::SetPridata(AM_U8* rawdata, AM_UINT len, AM_U16 data_type, AM_U16 sub_type)
{
    AM_ASSERT(ePridataComposerWorkMode_passive == mWorkMode);

    AMLOG_DEBUG("CPridataComposer::SetPridata, len %d, data_type %d, sub_type %d.\n", len, data_type, sub_type);

    CDoubleLinkedList::SNode* pnode;
    SPriDataPacket* packet;
    AM_ASSERT(mpClockManager);
    if (mpClockManager) {
        mPTS = mpClockManager->GetCurrentTime();
    }
    SPriDataHeader* pheader;

    //find same data type/sub type
    pnode = mPridataList.FirstNode();
    while (pnode) {
        packet = (SPriDataPacket*) pnode->p_context;

        AMLOG_DEBUG("packet %p, data_type %d, sub_type %d.\n", packet, packet->data_type, packet->sub_type);

        //replace the data
        if ((packet->data_type == data_type) &&(packet->sub_type == sub_type)) {
            //check the memory is enough
            if (packet->total_length < (len + sizeof(SPriDataHeader))) {
                if (packet->pdata) {
                    free(packet->pdata);
                    packet->pdata = NULL;
                }
                packet->pdata = (AM_U8*) malloc(len + sizeof(SPriDataHeader));
                packet->total_length = len + sizeof(SPriDataHeader);
            }

            AM_ASSERT(packet->pdata);
            if (NULL == packet->pdata) {
                AM_ERROR("NO MEMORY.\n");
                return ME_NO_MEMORY;
            }

            //set pridata header
            pheader = (SPriDataHeader*) packet->pdata;
            pheader->magic_number = DAM_PRI_MAGIC_TAG;
            pheader->data_length = len;
            pheader->type = data_type;
            pheader->subtype = sub_type;
            pheader->reserved = 0;

            memcpy(packet->pdata + sizeof(SPriDataHeader), rawdata, len);
            packet->data_type = data_type;
            packet->sub_type = sub_type;
            packet->data_len = len + sizeof(SPriDataHeader);
            packet->pts = mPTS;
            return ME_OK;
        }

        pnode = mPridataList.NextNode(pnode);
    }

    //new data type comes, add it to list
    packet = (SPriDataPacket*)malloc(sizeof(SPriDataPacket));
    if (NULL == packet) {
        AM_ERROR("NO MEMORY.\n");
        return ME_NO_MEMORY;
    }

    packet->pdata = (AM_U8*) malloc(len + sizeof(SPriDataHeader));
    if (NULL == packet->pdata) {
        AM_ERROR("NO MEMORY.\n");
        free(packet);
        return ME_NO_MEMORY;
    }

    //set pridata header
    pheader = (SPriDataHeader*) packet->pdata;
    pheader->magic_number = DAM_PRI_MAGIC_TAG;
    pheader->data_length = len;
    pheader->type = data_type;
    pheader->subtype = sub_type;
    pheader->reserved = 0;

    packet->total_length = len + sizeof(SPriDataHeader);
    memcpy(packet->pdata + sizeof(SPriDataHeader), rawdata, len);
    packet->data_type = data_type;
    packet->sub_type = sub_type;
    packet->data_len = len + sizeof(SPriDataHeader);
    packet->pts = mPTS;

    AMLOG_DEBUG("CPridataComposer::SetPridata, new packet %p.\n", packet);

    getDurationStruct(packet);

    //add packet to list
    mPridataList.InsertContent(NULL, packet, 0);

    mnTotalDataTypes++;
    return ME_OK;
}

void CPridataComposer::setDuration(am_pts_t new_duration)
{
    am_pts_t internal_duration = 0;
    if (mOverSampling > 1) {
        //consider over sampling
        internal_duration = new_duration/mOverSampling;
        AMLOG_INFO("CPridataComposer:setDuration(over sampling) from %llu to %llu, input duration %llu, oversampling %u.\n", mDuration, internal_duration, new_duration, mOverSampling);
    } else {
        internal_duration = new_duration;
        AMLOG_INFO("CPridataComposer:setDuration(no over sampling) from %llu to %llu, oversampling %u.\n", mDuration, internal_duration, mOverSampling);
    }

    if (internal_duration < mDuration) {
        mDuration = internal_duration;
        AMLOG_INFO("** CPridataComposer:setDuration: new duration %llu.\n", mDuration);
    }
}

AM_ERR CPridataComposer::SetPridataDuration(AM_UINT _duration, AM_U16 data_type)
{
    AM_ASSERT(ePridataComposerWorkMode_passive == mWorkMode);
    am_pts_t duration = (am_pts_t)_duration*IParameters::TimeUnitDen_90khz/IParameters::TimeUnitDen_ms;
    AMLOG_DEBUG("CPridataComposer::SetPridataDuration, _duration %d, data_type %d.\n", _duration, data_type);
    CDoubleLinkedList::SNode* pnode;
    SPriDataDuration* data_duration;

    setDuration(duration);

    //find same data type/sub type
    pnode = mPriDurationList.FirstNode();
    while (pnode) {
        data_duration = (SPriDataDuration*) pnode->p_context;

        //replace the duration
        if (data_duration->data_type == data_type) {
            data_duration->cool_down = data_duration->duration = DGetPridataDuration(duration);
            AMLOG_DEBUG("data_duration %p, duration %llu, cooldown %llu, data_type %d.\n", data_duration, data_duration->duration, data_duration->cool_down, data_duration->data_type);
            return ME_OK;
        }

        pnode = mPriDurationList.NextNode(pnode);
    }

    //new data type comes, add it to list
    data_duration = (SPriDataDuration*)malloc(sizeof(SPriDataDuration));
    if (NULL == data_duration) {
        AM_ERROR("NO MEMORY.\n");
        return ME_NO_MEMORY;
    }

    data_duration->data_type = data_type;
    data_duration->cool_down = data_duration->duration = DGetPridataDuration(duration);
    data_duration->need_write = 0;

    AMLOG_DEBUG("CPridataComposer::SetPridataDuration, new data_duration %p.\n", data_duration);

    //add packet to list
    mPriDurationList.InsertContent(NULL, data_duration, 0);

    return ME_OK;
}

void CPridataComposer::setNextTimer()
{
    AM_ASSERT(mpClockManager);

    //set next timer
    if (mpClockManager) {
        mNextTimer += mDuration;
        mpClockManager->SetTimer(this, mNextTimer);
        AMLOG_PTS("[PriComposer], set next timer %llu.\n", mNextTimer);
    }
}

void CPridataComposer::updateNeedWriteFlags()
{
    CDoubleLinkedList::SNode* pnode;
    SPriDataDuration* data_duration;

    //find same data type/sub type
    pnode = mPriDurationList.FirstNode();
    while (pnode) {
        data_duration = (SPriDataDuration*) pnode->p_context;

        if (data_duration->cool_down <= mDuration) {
            data_duration->need_write = 1;
            data_duration->cool_down += data_duration->duration;
        }
        AM_ASSERT(data_duration->duration >= mDuration);
        AM_ASSERT(data_duration->cool_down >= mDuration);

        data_duration->cool_down -= mDuration;

        pnode = mPriDurationList.NextNode(pnode);
    }

}

void CPridataComposer::clearNeedWriteFlags()
{
    CDoubleLinkedList::SNode* pnode;
    SPriDataDuration* data_duration;

    //find same data type/sub type
    pnode = mPriDurationList.FirstNode();
    while (pnode) {
        data_duration = (SPriDataDuration*) pnode->p_context;
        data_duration->need_write = 0;
        pnode = mPriDurationList.NextNode(pnode);
    }
}

AM_UINT CPridataComposer::packPrivateData(AM_U8* dest, AM_UINT max_len, am_pts_t& pts)
{
    if (!dest || max_len <= (sizeof(SPriDataHeader))) {
        AM_ERROR("BAD params in CPridataComposer::packPrivateData, dest %p, max_len %u.\n", dest, max_len);
        return 0;
    }

    CDoubleLinkedList::SNode* pnode;
    SPriDataPacket* packet;
    AM_U8* p_cur = dest;
    AM_UINT len_cur = 0;

    AMLOG_DEBUG("CPridataComposer::packPrivateData start.\n");

    pnode = mPridataList.FirstNode();
    while (pnode) {
        packet = (SPriDataPacket*) pnode->p_context;
        AM_ASSERT(packet->p_cooldown);
        if (packet->p_cooldown->need_write) {
            if (len_cur + packet->data_len < max_len) {
                memcpy(p_cur, packet->pdata, packet->data_len);
                p_cur += packet->data_len;
                len_cur += packet->data_len;
                pts = packet->pts;//choose which pts, is a question
            } else {
                AM_ERROR("Max size reached, max_len %u, len_cur %u, packet->data_len %u.\n", max_len, len_cur, packet->data_len);
                break;
            }
        }
        pnode = mPridataList.NextNode(pnode);
    }

    AMLOG_DEBUG("CPridataComposer::packPrivateData done, length %u.\n", len_cur);
    return len_cur;
}

void CPridataComposer::sendFlowControlBuffer(FlowControlType type)
{
    CBuffer* pBuffer = NULL;
    AM_ASSERT(mpBufferPool);

    if (true == mpBufferPool->AllocBuffer(pBuffer, 0)) {
        pBuffer->SetType(CBuffer::FLOW_CONTROL);
        pBuffer->SetFlags(type);
        //send out buffer
        for (AM_UINT i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
            if (mpOutput[i] == NULL)
                continue;
            pBuffer->AddRef();
            mpOutput[i]->SendBuffer(pBuffer);
        }
    } else {
        AM_ERROR("CAN NOT alloc buffer.\n");
    }
}

void CPridataComposer::sendEOSBuffer()
{
    CBuffer* pBuffer = NULL;
    AM_ASSERT(mpBufferPool);

    if (true == mpBufferPool->AllocBuffer(pBuffer, 0)) {
        pBuffer->SetType(CBuffer::EOS);
        pBuffer->SetFlags(0);
        //send out buffer
        for (AM_UINT i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
            if (mpOutput[i] == NULL)
                continue;
            pBuffer->AddRef();
            mpOutput[i]->SendBuffer(pBuffer);
        }
    } else {
        AM_ERROR("CAN NOT alloc buffer.\n");
    }
}

AM_ERR CPridataComposer::FlowControl(FlowControlType type)
{
    CMD cmd0;

    cmd0.code = CMD_FLOW_CONTROL;
    cmd0.flag = (AM_U8)type;

    mpWorkQ->MsgQ()->PostMsg((void*)&cmd0, sizeof(cmd0));
    return ME_OK;
}

bool CPridataComposer::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("****CPridataComposer::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);
    switch (cmd.code) {

        case CMD_STOP:
            mbRun = false;
            CmdAck(ME_OK);
            AMLOG_INFO("****CPridataComposer::ProcessCmd, STOP cmd.\n");
            break;

        case CMD_RUN:
            //re-run
            AMLOG_INFO("CPridataComposer re-Run, state %d.\n", msState);
            AM_ASSERT(STATE_PENDING == msState || STATE_ERROR == msState);
            msState = STATE_IDLE;
            CmdAck(ME_OK);
            break;

        case CMD_OBUFNOTIFY:
            if (mpBufferPool->GetFreeBufferCnt() > 0) {
                if (msState == STATE_IDLE)
                    msState = STATE_HAS_OUTPUTBUFFER;
                else if(msState == STATE_HAS_INPUTDATA)
                    msState = STATE_READY;
            }
            break;

        case CMD_TIMERNOTIFY:
            AM_ASSERT(mpClockManager);
            setNextTimer();

            if (msState == STATE_IDLE)
                msState = STATE_HAS_INPUTDATA;
            else if(msState == STATE_HAS_OUTPUTBUFFER)
                msState = STATE_READY;

            break;

        case CMD_FLOW_CONTROL:
            //EOS is special
            if ((FlowControlType)cmd.flag == FlowControl_eos) {
                AMLOG_INFO("CPridataComposer::SendEOS.\n");
                sendEOSBuffer();
                AMLOG_INFO("CPridataComposer::SendEOS done.\n");
                msState = STATE_PENDING;
                break;
            } else {
                sendFlowControlBuffer((FlowControlType)cmd.flag);
            }
            break;


        default:
            AM_ERROR(" CPridataComposer: wrong cmd %d.\n",cmd.code);
    }
    return false;
}

AM_ERR CPridataComposer::OnTimer(am_pts_t curr_pts)
{
    AMLOG_VERBOSE("CPridataComposer::OnTimer comes.\n");
    mpWorkQ->PostMsg(CMD_TIMERNOTIFY);
    return ME_OK;
}

void CPridataComposer::OnRun()
{
    CmdAck(ME_OK);
    CBuffer *pOutBuffer = NULL;
/*    AM_UINT encodeOutSize =0;
    AM_U8* outData = NULL;
    short *inData = NULL;*/
    AM_UINT data_size;
//    CPridataComposerOutput *pCurrOutPin;
    CMD cmd;

    msState = STATE_IDLE;

    AM_ASSERT(mpBufferPool);

    if (NULL == mpBufferPool) {
        AM_ERROR("NULL pointer: mpBufferPool(%p), must have errors.\n", mpBufferPool);
        msState = STATE_ERROR;
    }

    AM_ASSERT(mpClockManager);
    setDuration(mDuration);
    //tmp code, pts start from 0, same with video/audio, no avsync in record engine now, set it here
    mpClockManager->SetClockMgr(0);
    mNextTimer = 0;

    //start timer
    setNextTimer();

    mbRun = true;
    while (mbRun) {

        AMLOG_STATE("CPridataComposer::OnRun loop begin, msState %d.\n", msState);
        switch (msState) {
            case STATE_IDLE:
                AM_ASSERT(!pOutBuffer);
                if (mpBufferPool->GetFreeBufferCnt() > 0) {
                    msState = STATE_HAS_OUTPUTBUFFER;
                } else {
                    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                    ProcessCmd(cmd);
                }
                break;

            case STATE_HAS_INPUTDATA:
                AM_ASSERT(!pOutBuffer);

                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_HAS_OUTPUTBUFFER:
                AM_ASSERT(mpBufferPool->GetFreeBufferCnt() > 0);
                if (!pOutBuffer) {
                    //get outbuffer here
                    if (!mpBufferPool->AllocBuffer(pOutBuffer, 0)) {
                        AM_ERROR("Failed to AllocBuffer\n");
                        msState = STATE_ERROR;
                        break;
                    }
                }

                AM_ASSERT(pOutBuffer);

                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_READY:
                if (!pOutBuffer) {
                    //get outbuffer here
                    if (!mpBufferPool->AllocBuffer(pOutBuffer, 0)) {
                        AM_ERROR("Failed to AllocBuffer\n");
                        msState = STATE_ERROR;
                        break;
                    }
                }
                AM_ASSERT(pOutBuffer);
                updateNeedWriteFlags();
                data_size = packPrivateData(pOutBuffer->GetDataPtr(), pOutBuffer->mBlockSize, pOutBuffer->mPTS);
                clearNeedWriteFlags();

                if (data_size) {
                    //send buffer
                    pOutBuffer->SetDataSize(data_size);
                    pOutBuffer->SetType(CBuffer::DATA);
                    pOutBuffer->mPTS = mpClockManager->GetCurrentTime();
                    AMLOG_INFO("sending private data.\n");
                    SendoutBuffer(pOutBuffer);
                    pOutBuffer->Release();
                    pOutBuffer = NULL;
                    msState = STATE_IDLE;
                } else {
                    //have no data
                    msState = STATE_HAS_OUTPUTBUFFER;
                }
                break;

            default:
                AM_ERROR("BAD STATE(%d) in CPridataComposer OnRun.\n", msState);
            case STATE_PENDING:
            case STATE_ERROR:
                //clear buffer here
                if (pOutBuffer) {
                    pOutBuffer->Release();
                    pOutBuffer = NULL;
                }
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;
        }
    }

    AMLOG_INFO("CPridataComposer::OnRun end");
}

#ifdef AM_DEBUG
void CPridataComposer::PrintState()
{
    AMLOG_WARN("CPridataComposer: msState=%d.\n", msState);
    AM_ASSERT(mpBufferPool);

    if (mpBufferPool) {
        AMLOG_WARN(" buffer pool have %d free buffers.\n", mpBufferPool->GetFreeBufferCnt());
    }
}
#endif


//-----------------------------------------------------------------------
//
// CPridataComposerOutput
//
//-----------------------------------------------------------------------
CPridataComposerOutput* CPridataComposerOutput::Create(CFilter *pFilter)
{
	CPridataComposerOutput *result = new CPridataComposerOutput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

