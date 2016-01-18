/**
 * general_pipeline.cpp
 *
 * History:
 *    2013/1/21- [Qingxiong Z] create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "general_header.h"
#include "general_interface.h"

#include "general_demuxer_filter.h"
#include "general_decoder_filter.h"
#include "general_renderer_filter.h"
#include "am_mdec_if.h"
#include "mdec_if.h"
#include "general_layout_manager.h"

#include "active_mdec_engine.h"
#include "general_pipeline.h"

IPipeLineManager* CPipeLineManager::Create(IEngine* pEngine, AM_BOOL isGMF)
{
    CPipeLineManager* result = new CPipeLineManager(pEngine, isGMF);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CPipeLineManager::CPipeLineManager(IEngine* pEngine, AM_BOOL isGMF):
    mpEngine(pEngine),
    mpMdecEngine(NULL),
    mNumPBV(0)
{
    if(isGMF)
        mpMdecEngine = reinterpret_cast<CActiveMDecEngine* >(mpEngine);
}

CPipeLineManager::~CPipeLineManager()
{
    AM_INFO("~CPipeLineManager Start.\n");
    AM_INFO("~CPipeLineManager Done.\n");
}

AM_ERR CPipeLineManager::Construct()
{

    return ME_OK;
}

void CPipeLineManager::Delete()
{
    CObject::Delete();
}

void* CPipeLineManager::GetInterface(AM_REFIID refiid)
{
    return CObject::GetInterface(refiid);
}

AM_INT CPipeLineManager::CreatePipeLine(const char* uri, PIPE_TYPE type)
{
    PipeLine* curPipe = NULL;
    if(type == PIPELINE_PLAYBACK){
        curPipe = &(mPlaybackV[mNumPBV]);
        curPipe->Init(uri, type);
        mNumPBV++;
    }else{
        AM_ASSERT(0);
    }
    return mNumPBV-1;
}

AM_ERR CPipeLineManager::AddPipeNode(AM_INT index, void* module, NODE_TYPE type)
{
    return ME_NO_IMPL;
}

AM_ERR CPipeLineManager::AddPipeNode(AM_INT index, AM_INT indentity, NODE_TYPE type)
{
    AM_ERR err;
    PipeLine* curPipe = NULL;
    curPipe = &(mPlaybackV[index]);
    err = curPipe->AddNode(indentity, type);
    return err;
}

AM_ERR CPipeLineManager::UpdatePipeLine(AM_INT index)
{
    return ME_NO_IMPL;
}

AM_ERR CPipeLineManager::PreparePipeLine(AM_INT index)
{
    AM_ERR err;
    err = mPlaybackV[index].CheckBeforePrepare();
    return err;
}

AM_ERR CPipeLineManager::PausePipeLine(AM_INT index)
{
    AM_ERR err = ME_OK;
    AM_INT node = 0, target = 0;
    PipeLine::PipeNode* pNode = mPlaybackV[index].GetNode(node);
    CInterActiveFilter* pFilter = NULL;
    while(pNode)
    {
        pFilter = GetFilterByNode(pNode);
        target = GetTargetByNode(pNode);
        pFilter->Pause(target);
        pNode = mPlaybackV[index].GetNode(++node);
    }
    return err;
}

CInterActiveFilter* CPipeLineManager::GetFilterByNode(PipeLine::PipeNode* pNode)
{
    CInterActiveFilter* pFilter = NULL;
    switch(pNode->type)
    {
    case DEMUXER_FFMPEG:
        pFilter = mpMdecEngine->mpDemuxer;
        break;

    case DECODER_FFMPEG:
        pFilter = mpMdecEngine->mpAudioDecoder;
        break;

    case DECODER_DSP:
        pFilter = mpMdecEngine->mpVideoDecoder;
        break;

    case RENDER_SYNC:
        pFilter = mpMdecEngine->mpRenderer;
        break;

    default:
        break;
    };
    return pFilter;
}

AM_INT CPipeLineManager::GetTargetByNode(PipeLine::PipeNode* pNode)
{
    AM_INT target = 0;
    switch(pNode->type)
    {
    case DEMUXER_FFMPEG:
        target = pNode->identity;
        break;

    case DECODER_FFMPEG:
        target = pNode->identity;
        break;

    case DECODER_DSP:
        target = mpMdecEngine->DEC_MAP(pNode->identity);
        break;

    case RENDER_SYNC:
        target = mpMdecEngine->REN_MAP(pNode->identity);
        break;

    default:
        break;
    };
    return target;
}

AM_ERR CPipeLineManager::FlushPipeLine(AM_INT index)
{
    AM_ERR err = ME_OK;
    AM_INT node = 0, target = 0;
    PipeLine::PipeNode* pNode = mPlaybackV[index].GetNode(node);
    CInterActiveFilter* pFilter = NULL;
    while(pNode)
    {
        pFilter = GetFilterByNode(pNode);
        target = GetTargetByNode(pNode);
        pFilter->Flush(target);
        pNode = mPlaybackV[index].GetNode(++node);
    }
    return err;
}

AM_ERR CPipeLineManager::ResumePipeLine(AM_INT index)
{
    AM_ERR err = ME_OK;
    AM_INT node = 0, target = 0;
    PipeLine::PipeNode* pNode = mPlaybackV[index].GetNode(node);
    CInterActiveFilter* pFilter = NULL;
    while(pNode)
    {
        pFilter = GetFilterByNode(pNode);
        target = GetTargetByNode(pNode);
        pFilter->Resume(target);
        pNode = mPlaybackV[index].GetNode(++node);
    }
    return err;
}

AM_ERR CPipeLineManager::Dump(AM_INT flag)
{
    AM_INFO("CPipeLineManager Dump  PipeLine Num:%d\n", mNumPBV);
    AM_INT i = 0, node = 0;
    PipeLine::PipeNode* pNode = NULL;

    for(; i < mNumPBV; i++)
    {
        node = 0;
        AM_INFO("PipeLine%d(%s)", i, mPlaybackV[i].GetSource());
        pNode = mPlaybackV[i].GetNode(node);
        while(pNode){
            AM_INFO("(Node%d Identity:%d, Type:%d) ", node, pNode->identity, pNode->type);
            node++;
            pNode = mPlaybackV[i].GetNode(node);
        }
        AM_INFO("\n");
    }
    return ME_OK;
}
//------------------------------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------------------------------
CPipeLineManager::PipeLine::PipeLine()
{
    mpList = NULL;
}

CPipeLineManager::PipeLine::~PipeLine()
{
    //AM_INFO("~PipeLine %s", mpSource);
    if(mpList == NULL)
        return;
    PipeNode* pNode = mpList->pre;
    while(pNode != mpList){
        mpList->pre = pNode->pre;
        delete pNode;
        pNode = mpList->pre;
    }
    delete mpList;
}

AM_ERR CPipeLineManager::PipeLine::Init(const char* uri, PIPE_TYPE intype)
{
    strncpy(mpSource, uri, 127);
    mpSource[127] = '\0';
    mType = intype;
    mState = PIPE_IDLE;
    mpList = NULL;
    return ME_OK;
}

AM_ERR CPipeLineManager::PipeLine::AddNode(AM_INT indentity, NODE_TYPE intype)
{
    AM_ERR err = ME_ERROR;
    err = CheckBeforeAdd(intype);
    if(err != ME_OK){
        AM_ASSERT(0);
        return err;
    }
    PipeNode* pNode = new PipeNode;
    pNode->identity = indentity;
    pNode->type = intype;
    if(mpList == NULL){
        mpList = pNode;
        mpList->pre = mpList->next = mpList;
    }else{
        pNode->pre = mpList->pre;
        mpList->pre->next = pNode;
        mpList->pre = pNode;
        pNode->next = mpList;
    }
    return ME_OK;
}

AM_ERR CPipeLineManager::PipeLine::AddNode(void* identity, NODE_TYPE intype)
{
    return ME_NO_IMPL;
}

CPipeLineManager::PipeLine::PipeNode* CPipeLineManager::PipeLine::GetNode(AM_INT index)
{
    PipeNode* retNode = mpList;
    AM_INT i = 0;
    for(; i < index; i++)
    {
        retNode = retNode->next;
        if(retNode == mpList){
            retNode = NULL;
            break;
        }
    }
    return retNode;
}

AM_ERR CPipeLineManager::PipeLine::CheckBeforeAdd(NODE_TYPE intype)
{
    return ME_OK;
}

AM_ERR CPipeLineManager::PipeLine::CheckBeforePrepare()
{
    mState = PIPE_PREPARED;
    return ME_OK;
}

