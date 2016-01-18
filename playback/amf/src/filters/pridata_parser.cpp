/*
 * pridata_parser.cpp
 *
 * History:
 *    2011/12/13 - [Zhi He] create file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_NDEBUG 0
#define LOG_TAG "pridata_parser"
#define AMDROID_DEBUG

#include "stdio.h"
#include "stdlib.h"

#if PLATFORM_ANDROID
#include "sys/atomics.h"
#endif

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

#include "am_pbif.h"
#include "pbif.h"
#include "engine_guids.h"
#include "filter_list.h"

#include "pridata_parser.h"

IFilter* CreatePridataParser(IEngine * pEngine)
{
    return CPridataParser::Create(pEngine);
}

#define _proper_time_gap_ 45000
//20 seconds treat the gap is wrong
#define _wrong_pts_ 1800000

enum {
    EProperTime = 0,
    ETooLate,
    ETooEarly,
};

#define _too_early_
static AM_INT _checktime (am_pts_t& cur_time, am_pts_t& pts)
{
    if (((cur_time + _proper_time_gap_) > pts) && ((pts + _proper_time_gap_) > cur_time)) {
        return EProperTime;
    } else if ((cur_time + _proper_time_gap_) < pts) {
        return ETooEarly;
    }
    return ETooLate;
}

filter_entry g_pridata_parser = {
    "pridata_parser",
    CPridataParser::Create,
    NULL,
    CPridataParser::AcceptMedia,
};

IFilter* CPridataParser::Create(IEngine * pEngine)
{
    CPridataParser * result = new CPridataParser(pEngine);

    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

int CPridataParser::AcceptMedia(CMediaFormat& format)
{
    if (*format.pMediaType == GUID_Pridata) {
        AM_INFO("CPridataParser::AcceptMedia 1\n");
        return 1;
    }
    AM_INFO("CPridataParser::AcceptMedia 0\n");
    return 0;
}

AM_ERR CPridataParser::Construct()
{
    AM_ERR err = inherited::Construct();
    DSetModuleLogConfig(LogModulePridataParser);
    if (err != ME_OK) {
        AM_ERROR("inherited::Construct() fail in CPridataParser::Construct().\n");
        return err;
    }

    mpClockManager = (IClockManager *)mpEngine->QueryEngineInterface(IID_IClockManager);
    if(!mpClockManager) {
        AM_ERROR("CPridataParser without mpClockManager?\n");
        return ME_ERROR;
    }

    //create on inputpin here
    if ((mpInput[0] = CPridataParserInput::Create(this)) == NULL)
        return ME_NO_MEMORY;

    mnInput = 1;

    return ME_OK;
}

CPridataParser::~CPridataParser()
{
    //freeAllPrivateData();
}

void CPridataParser::Delete()
{
    freeAllPrivateData();
}

void CPridataParser::freeAllPrivateData()
{
    CDoubleLinkedList::SNode* pnode;
    SPriDataPacket* packet;

    pnode = mPridataList.FirstNode();
    while (pnode) {
        packet = (SPriDataPacket*) pnode->p_context;
        AM_ASSERT(packet);
        if (packet->pdata) {
            free(packet->pdata);
        }
        free(packet);
        pnode = mPridataList.NextNode(pnode);
    }
}

void CPridataParser::postPrivateDataMsg(AM_U32 size, AM_U16 type, AM_U16 sub_type)
{
    //not use this method
    AM_ASSERT(0);
    AM_MSG msg;
    msg.code = IEngine::MSG_PRIDATA_GENERATED;
    msg.p0 = size;
    msg.p2 = type;
    msg.p3 = sub_type;
    PostEngineMsg(msg);
    __atomic_inc(&mpSharedRes->mnPrivateDataCountNeedSent);
}

void CPridataParser::postDataByCallback(AM_U8* pdata, AM_U32 size, AM_U16 type, AM_U16 sub_type)
{
    AMLOG_DEBUG("postDataByCallback begin, cookie %p, callback %p.\n", mPrivateDataCallbackCookie, mPrivatedataCallback);

    if (mPrivateDataCallbackCookie && mPrivatedataCallback) {
        mbInCallBack = 1;

        mPrivatedataCallback(mPrivateDataCallbackCookie, pdata, size, type, sub_type);

        mbInCallBack = 0;
    }

    AMLOG_DEBUG("postDataByCallback end.\n");
}

AM_UINT CPridataParser::sendPrivateData(am_pts_t& cur_time)
{
    CDoubleLinkedList::SNode* pnode;
    SPriDataHeader* cur_header;//for check
    SPriDataPacket* packet;
    AM_UINT data_left = 0;
    am_pts_t earliest_pts = 0;

    pnode = mPridataList.FirstNode();
    while (pnode) {
        packet = (SPriDataPacket*) pnode->p_context;
        AM_ASSERT(packet);

#ifdef AM_DEBUG
        //check code for debug
        cur_header = (SPriDataHeader*) packet->pdata;
        AM_ASSERT(cur_header);
        if (cur_header) {
            AM_ASSERT(DAM_PRI_MAGIC_TAG == cur_header->magic_number);
            AM_ASSERT(packet->data_type == cur_header->type);
            AM_ASSERT(packet->sub_type == cur_header->subtype);
            AM_ASSERT(packet->data_len == (cur_header->data_length + sizeof(SPriDataHeader)));
        }
#endif

        //send data if it's not too early
        if (packet && packet->needsend2app) {
            if (ETooEarly != _checktime(cur_time, packet->pts)) {
                packet->needsend2app = 0;
                postDataByCallback(packet->pdata + sizeof(SPriDataHeader), packet->data_len - sizeof(SPriDataHeader), packet->data_type, packet->sub_type);
            } else {
                AMLOG_DEBUG("too early? cur_time %llu, packet->pts %llu.\n", cur_time, packet->pts);
                data_left ++;
                if (!earliest_pts) {
                    //first set
                    earliest_pts = packet->pts;
                } else if (earliest_pts > packet->pts) {
                    //choose earliest
                    earliest_pts = packet->pts;
                }
            }
        }

        pnode = mPridataList.NextNode(pnode);
    }

    if (earliest_pts)
    cur_time = earliest_pts;
    return data_left;
}

void CPridataParser::storePrivateData(AM_U8* pdata, AM_UINT max_len, am_pts_t pts)
{
    CDoubleLinkedList::SNode* pnode;
    SPriDataHeader* data_header = (SPriDataHeader*)pdata;
    SPriDataHeader* cur_header;
    SPriDataPacket* packet;
    AM_U8* ptmp = NULL;
    AM_UINT finded = 0;

    AMLOG_DEBUG("CPridataParser::storePrivateData start.\n");

    while (1) {

        //valid check
        data_header = (SPriDataHeader*)pdata;
        if (max_len <= sizeof(SPriDataHeader)) {
            //end
            return;
        }
        if (data_header->magic_number != DAM_PRI_MAGIC_TAG) {
            //not our private data, skip it now.
            //to do: with others private data's case
            AM_ERROR("BAD Magic number for pridata, must have errors, %c, %c, %c, %c.\n", pdata[0], pdata[1], pdata[2], pdata[3]);
            return;
        }
        if ((data_header->data_length + sizeof(SPriDataHeader)) > max_len) {
            AM_ERROR("BAD data len for pridata(%d, data left %d), must have errors.\n", data_header->data_length, max_len);
            return;
        }

        finded = 0;

        pnode = mPridataList.FirstNode();
        while (pnode) {
            packet = (SPriDataPacket*) pnode->p_context;
            AM_ASSERT(packet);
            cur_header = (SPriDataHeader*) packet->pdata;
            AM_ASSERT(cur_header);

            AMLOG_INFO("finding type %d, sub type %d.\n", data_header->type, data_header->subtype);
            AMLOG_INFO("current type %d, sub type %d, packet type %d, sub_type %d, size %d.\n", cur_header->type, cur_header->subtype, packet->data_type, packet->sub_type, packet->data_len);

            //find matched data slot
            if ((cur_header->type == data_header->type) && (cur_header->subtype == data_header->subtype)) {
                if ((data_header->data_length + sizeof(SPriDataHeader)) > packet->total_length) {
                    AMLOG_INFO("CPridataParser::storePrivateData, need re-alloc memory, type %d, subtype %d, len %d.\n", data_header->type, data_header->subtype, data_header->data_length);
                    packet->total_length = data_header->data_length + sizeof(SPriDataHeader) + 32;
                    ptmp = (AM_U8*)malloc(packet->total_length);
                    AM_ASSERT(ptmp);
                    free(packet->pdata);
                    packet->pdata = ptmp;
                }
                memcpy(packet->pdata, data_header, data_header->data_length + sizeof(SPriDataHeader));
                packet->data_len = data_header->data_length + sizeof(SPriDataHeader);
                packet->data_type = data_header->type;
                packet->sub_type = data_header->subtype;

                packet->needsend2app = 1;
                packet->pts = pts;

                finded = 1;
                break;
            }
            pnode = mPridataList.NextNode(pnode);
        }

        //if not found
        if (!finded) {
            packet = (SPriDataPacket*) malloc(sizeof(SPriDataPacket));
            AM_ASSERT(packet);
            if (!packet) {
                AM_ERROR("NOT ENOUGH memory.\n");
                return;
            }
            packet->total_length = data_header->data_length + sizeof(SPriDataHeader) + 32;
            ptmp = (AM_U8*)malloc(packet->total_length);
            AM_ASSERT(ptmp);
            if (!ptmp) {
                AM_ERROR("NOT ENOUGH memory.\n");
                return;
            }
            packet->pdata = ptmp;
            memcpy(packet->pdata, data_header, data_header->data_length + sizeof(SPriDataHeader));
            packet->data_len = data_header->data_length + sizeof(SPriDataHeader);
            packet->data_type = data_header->type;
            packet->sub_type = data_header->subtype;
            packet->needsend2app = 1;
            packet->pts = pts;

            mPridataList.InsertContent(NULL, (void *)packet, 0);
            AMLOG_INFO("CPridataParser::storePrivateData, new data slot, type %d, subtype %d, len %d.\n", packet->data_type, packet->sub_type, packet->data_len);
        }

        pdata += data_header->data_length + sizeof(SPriDataHeader);
        max_len -= data_header->data_length + sizeof(SPriDataHeader);

    }

    AMLOG_DEBUG("CPridataParser::storePrivateData done.\n");
}

AM_ERR CPridataParser::RetieveData(AM_U8* pdata, AM_UINT max_len, IParameters::DataCategory data_category)
{
    CDoubleLinkedList::SNode* pnode;
    AM_U8* p_cur = pdata;
    SPriDataPacket* packet;
    AM_UINT len_cur = 0;

    AM_ASSERT(IParameters::DataCategory_PrivateData == data_category);
    if (IParameters::DataCategory_PrivateData == data_category) {

        AMLOG_DEBUG("CPridataParser::RetieveData start.\n");

        pnode = mPridataList.FirstNode();
        while (pnode) {
            packet = (SPriDataPacket*) pnode->p_context;
            AM_ASSERT(packet);

            //debug assert
            AM_ASSERT(packet->needsend2app);

            if ((len_cur + packet->data_len) < max_len) {
                memcpy(p_cur, packet->pdata, packet->data_len);
                p_cur += packet->data_len;
                len_cur += packet->data_len;
            } else {
                AM_ERROR("Max size reached, max_len %u, len_cur %u, packet->data_len %u.\n", max_len, len_cur, packet->data_len);
                break;
            }

            pnode = mPridataList.NextNode(pnode);
        }

        AMLOG_DEBUG("CPridataParser::RetieveData done, length %u.\n", len_cur);

    } else {
        //need implement
        AM_ERROR("NOT SUPPORTED.\n");
        return ME_ERROR;
    }
    return ME_OK;
}

AM_ERR CPridataParser::RetieveDataByType(AM_U8* pdata, AM_UINT max_len, AM_U16 type, AM_U16 sub_type, IParameters::DataCategory data_category)
{
    CDoubleLinkedList::SNode* pnode;
    SPriDataPacket* packet;

    AM_ASSERT(IParameters::DataCategory_PrivateData == data_category);
    if (IParameters::DataCategory_PrivateData == data_category) {

        AMLOG_DEBUG("CPridataParser::RetieveDataByType start.\n");

        pnode = mPridataList.FirstNode();
        while (pnode) {
            packet = (SPriDataPacket*) pnode->p_context;
            AM_ASSERT(packet);

            if (packet->data_type == type && packet->sub_type == sub_type) {
                //debug assert
                AM_ASSERT(packet->needsend2app);

                if ((packet->data_len - sizeof(SPriDataHeader)) < max_len) {
                    memcpy(pdata, packet->pdata + sizeof(SPriDataHeader), packet->data_len - sizeof(SPriDataHeader));
                } else {
                    AM_ERROR("Max size reached, max_len %u, packet->data_len %u.\n", max_len, packet->data_len);
                    break;
                }
                return ME_OK;
            }

            pnode = mPridataList.NextNode(pnode);
        }

        AMLOG_DEBUG("CPridataParser::RetieveDataByType done.\n");

    } else {
        //need implement
        AM_ERROR("NOT SUPPORTED.\n");
        return ME_ERROR;
    }
    AM_ERROR("NOT FOUND data tye %d sub type %d.\n", type, sub_type);
    return ME_ERROR;
}

void CPridataParser::GetInfo(INFO& info)
{
    info.mPriority = 0;//not master renderer
    info.mFlags = 0;
    info.nInput = mnInput;
    info.nOutput = 0;
    info.pName = mpName;
}

IPin* CPridataParser::GetInputPin(AM_UINT index)
{
    AMLOG_INFO("CPridataParser::GetInputPin %d, %p\n", index, mpInput[index]);
    if (index < MAX_NUM_INPUT_PIN) {
        return (IPin*)mpInput[index];
    }
    return NULL;
}

AM_ERR CPridataParser::Stop()
{
    AMLOG_INFO("=== CPridataParser::Stop()\n");
    inherited::Stop();
    return ME_OK;
}

AM_ERR CPridataParser::Start()
{
    mpWorkQ->Start();
    return ME_OK;
}

AM_ERR CPridataParser::AddInputPin(AM_UINT& index, AM_UINT type)
{
    AM_UINT i;

    //playback engine, must not invoke this function, todo
    AM_ERROR("playback engine, must not invoke this function.\n");
    return ME_ERROR;

    AM_ASSERT(type == IParameters::StreamType_PrivateData);
    if (type != IParameters::StreamType_PrivateData) {
        AM_ERROR("CPridataParser::AddOutputPin only support private data.\n");
        return ME_ERROR;
    }

    for (i = 0; i < MAX_NUM_INPUT_PIN; i++) {
        if (mpInput[i] == NULL)
            break;
    }

    if (i >= MAX_NUM_INPUT_PIN) {
        AM_ERROR(" max output pin %d (max %d) reached, please check code.\n", i, MAX_NUM_INPUT_PIN);
        return ME_ERROR;
    }

    if ((mpInput[i] = CPridataParserInput::Create(this)) == NULL)
        return ME_NO_MEMORY;


    index = i;
    mnInput ++;
    return ME_OK;
}

bool CPridataParser::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("****CPridataParser::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);
    switch (cmd.code) {

        case CMD_STOP:
            mbRun = false;
            CmdAck(ME_OK);
            AMLOG_INFO("****CPridataParser::ProcessCmd, STOP cmd.\n");
            break;

        case CMD_RUN:
            //re-run
            AMLOG_INFO("CPridataParser re-Run, state %d.\n", msState);
            AM_ASSERT(STATE_PENDING == msState || STATE_ERROR == msState);
            msState = STATE_IDLE;
            CmdAck(ME_OK);
            break;

        case CMD_PAUSE:
            AM_ASSERT(!mbPaused);
            mbPaused = true;
            break;

        case CMD_BEGIN_PLAYBACK:
        case CMD_RESUME:
            if(msState == STATE_PENDING)
                msState = STATE_IDLE;
            mbPaused = false;
            break;

        case CMD_FLUSH:
            AM_ASSERT(!mpInputBuffer);
            if (mpInputBuffer) {
                mpInputBuffer->Release();
                msState = STATE_PENDING;
            }
            CmdAck(ME_OK);
            break;

        case CMD_SOURCE_FILTER_BLOCKED:
            break;

        case CMD_TIMERNOTIFY:
            AM_ASSERT(STATE_READY == msState || STATE_PROCCESS_EOS == msState);
            break;

        default:
            AM_ERROR(" CPridataParser: wrong cmd %d.\n",cmd.code);
    }
    return false;
}

AM_ERR CPridataParser::OnTimer(am_pts_t curr_pts)
{
    AMLOG_VERBOSE("CPridataParser::OnTimer comes.\n");
    mpWorkQ->PostMsg(CMD_TIMERNOTIFY);
    return ME_OK;
}

bool CPridataParser::ReadInputData(CPridataParserInput* inputpin)
{
    AM_ASSERT(!mpInputBuffer);
    AM_ASSERT(inputpin);

    if (!inputpin->PeekBuffer(mpInputBuffer)) {
        AMLOG_ERROR("No buffer?\n");
        return false;
    }
    AMLOG_INFO(" [CPridataParser] get buffer type %d.\n", mpInputBuffer->GetType());
    if (mpInputBuffer->GetType() == CBuffer::EOS) {
        AMLOG_INFO("CPridataParser %p get EOS.\n", this);
        mpInputBuffer->Release();
        mpInputBuffer = NULL;
        msState = STATE_PROCCESS_EOS;
        return false;
    }

    AM_ASSERT(mpInputBuffer);
    AM_ASSERT(mpInputBuffer->GetDataPtr());
    AM_ASSERT(mpInputBuffer->GetType() == CBuffer::DATA);
#ifdef AM_DEBUG
    if (!mpInputBuffer->GetDataPtr()) {
        AMLOG_DEBUG("  Buffer type %d, size %d.\n", mpInputBuffer->GetType(), mpInputBuffer->GetDataSize());
    }
#endif
    AM_ASSERT(mpInputBuffer->GetDataSize());
    return true;
}

bool CPridataParser::requestRun()
{
    AO::CMD cmd;

	//send ready
    AMLOG_INFO("CPridataParser: post IEngine::MSG_READY.\n");
    PostEngineMsg(IEngine::MSG_READY);

    while (1) {
        AMLOG_INFO("CPridataParser::requestRun...\n");
        GetCmd(cmd);
        AMLOG_INFO("CPridataParser::requestRun... cmd %d\n", cmd.code);
        if (cmd.code == AO::CMD_START) {
            AMLOG_INFO("CPridataParser::requestRun done.\n");
            CmdAck(ME_OK);
            return true;
        }

        if(cmd.code == AO::CMD_AVSYNC)
        {
            CmdAck(ME_OK);
        }
        if (cmd.code == AO::CMD_STOP) {
            AMLOG_INFO("CPridataParser::requestRun stop comes...\n");
            CmdAck(ME_OK);
            return false;
        }
    }
}

void CPridataParser::OnRun()
{
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
//    CBuffer::Type buffer_type;

//    AM_UINT data_size;
    CPridataParserInput *pInputPin;
    AM_UINT data_left = 0;
    am_pts_t cur_time;

    CmdAck(ME_OK);

    msState = STATE_IDLE;
    mbRun = true;

    requestRun();

    while (mbRun) {

        AMLOG_STATE("CPridataParser::OnRun loop begin, msState %d.\n", msState);
        switch (msState) {
            case STATE_IDLE:
                if (mbPaused) {
                    msState = STATE_PENDING;
                    break;
                }

                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                if(type == CQueue::Q_MSG) {
                    ProcessCmd(cmd);
                } else {
                    pInputPin = (CPridataParserInput*)result.pOwner;
                    AM_ASSERT(pInputPin);
                    if (ReadInputData(pInputPin)) {
                        msState = STATE_HAS_INPUTDATA;
                        break;
                    }
                }
                break;

            //store data only
            case STATE_HAS_INPUTDATA:
                AM_ASSERT(mpInputBuffer);
                AMLOG_PTS("[CPridataParser] current pts %llu, buffer pts %llu.\n", mpClockManager->GetCurrentTime(), mpInputBuffer->GetPTS());
                storePrivateData(mpInputBuffer->GetDataPtr(), mpInputBuffer->GetDataSize(), mpInputBuffer->GetPTS());
                mpInputBuffer->Release();
                mpInputBuffer = NULL;
                msState = STATE_READY;
                break;

            case STATE_READY:
                AM_ASSERT(!mpInputBuffer);
                //post all data to app, then peek next data
                cur_time = mpClockManager->GetCurrentTime();
                AMLOG_PTS("[CPridataParser] current time %llu.\n", cur_time);
                data_left = sendPrivateData(cur_time);
                if (!data_left) {
                    //all sent
                    msState = STATE_IDLE;
                    break;
                } else {
                    //need wait
                    AMLOG_PTS("[CPridataParser] set timer %llu, data_left %d.\n", cur_time, data_left);
                    mpClockManager->SetTimer(this, cur_time);
                    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                    ProcessCmd(cmd);
                }
                break;

            //send all remaing data to app
            case STATE_PROCCESS_EOS:
                AM_ASSERT(!mpInputBuffer);
                //post all data to app, then peek next data
                cur_time = mpClockManager->GetCurrentTime();
                AMLOG_PTS("[CPridataParser] 2 current time %llu.\n", cur_time);
                data_left = sendPrivateData(cur_time);
                if (!data_left) {

                    //peek all cmd and process it
                    while(mpWorkQ->PeekCmd(cmd)) {
                        ProcessCmd(cmd);
                    }
                    if((msState != STATE_PROCCESS_EOS) || (mbRun==false))
                        break;

                    //peek all remaing data at inputpin, only process pin[0], hard code here
                    AM_ASSERT(mpInput[0]);
                    if (mpInput[0]) {
                        if (!mpInput[0]->PeekBuffer(mpInputBuffer)) {
                            //all sent
                            msState = STATE_PENDING;
                            PostEngineMsg(IEngine::MSG_EOS);
                            AMLOG_INFO("CPridataParser send EOS to engine.\n");
                            break;
                        } else {
                            //write data
                            AMLOG_PTS("[CPridataParser] current pts %llu, buffer pts %llu.\n", mpClockManager->GetCurrentTime(), mpInputBuffer->GetPTS());
                            storePrivateData(mpInputBuffer->GetDataPtr(), mpInputBuffer->GetDataSize(), mpInputBuffer->GetPTS());
                            mpInputBuffer->Release();
                            mpInputBuffer = NULL;
                        }
                    } else {
                        AM_ERROR("NULL inputpin[0], must have error here!!!!, post eos to engine.\n");
                        msState = STATE_PENDING;
                        PostEngineMsg(IEngine::MSG_EOS);
                        AMLOG_INFO("CPridataParser send EOS to engine.\n");
                        break;
                    }
                    break;
                } else {
                    //need wait
                    AMLOG_PTS("[CPridataParser] set timer 2 %llu, data_left %d.\n", cur_time, data_left);
                    mpClockManager->SetTimer(this, cur_time);
                    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                    ProcessCmd(cmd);
                }
                break;

            default:
                AM_ERROR("BAD STATE(%d) in CPridataParser OnRun.\n", msState);
            case STATE_PENDING:
            case STATE_ERROR:
                //clear buffer here
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;
        }
    }

    AMLOG_INFO("CPridataParser::OnRun end");
}

#ifdef AM_DEBUG
void CPridataParser::PrintState()
{
    AMLOG_WARN("CPridataParser: msState=%d, mbInCallBack %d.\n", msState, mbInCallBack);
    AM_UINT i = 0;
    for (i = 0; i < MAX_NUM_INPUT_PIN; i ++) {
        if (mpInput[i]) {
            AMLOG_WARN("   pin[%d] have %d free buffers.\n", i, mpInput[i]->mpBufferQ->GetDataCnt());
        }
    }
}
#endif


//-----------------------------------------------------------------------
//
// CPridataParserInput
//
//-----------------------------------------------------------------------
CPridataParserInput* CPridataParserInput::Create(CFilter *pFilter)
{
    CPridataParserInput *result = new CPridataParserInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CPridataParserInput::Construct()
{
    AM_ERR err = inherited::Construct(((CPridataParser*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

AM_ERR CPridataParserInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    if (*pFormat->pMediaType == GUID_Pridata) {
        return ME_OK;
    }
    return ME_NOT_SUPPORTED;
}

