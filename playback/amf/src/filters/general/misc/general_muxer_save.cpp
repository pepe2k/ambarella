/*
 * general_muxer_save.cpp
 *
 * History:
 *    2012/6/15 - [QingXiong Z] create file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"


#include "g_simple_save.h"
#include "g_muxer_ffmpeg.h"
#include "g_injector_rtmp.h"
#include "g_injector_rtsp.h"
#include "general_muxer_save.h"


//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
CGeneralMuxer* CGeneralMuxer::Create(CGConfig* pConfig)
{
    CGeneralMuxer* result = new CGeneralMuxer(pConfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
        }
    return result;
}

CGeneralMuxer::CGeneralMuxer(CGConfig* pConfig):
    mDuration(0),
    mMaxFileCount(0)
{
    AM_INT i = 0;
    for(; i < MDEC_SOURCE_MAX_NUM; i++){
        mState[i] = STATE_NULL;
        mSendState[i] = STATE_NULL_INJECTOR;
        mpMuxer[i] = NULL;
        mpInjector[i] = NULL;
    }
    mpGConfig = pConfig;
    mpConfig = pConfig->mainMuxer;
}

AM_ERR CGeneralMuxer::Construct()
{
    return ME_OK;
}

void CGeneralMuxer::Delete()
{
    AM_INT i = 0;
    for(; i < MDEC_SOURCE_MAX_NUM; i++){
        if(mpMuxer[i]){
            AM_DELETE(mpMuxer[i]);
        }
        if(mpInjector[i]){
            AM_DELETE(mpInjector[i]);
        }
    }
    CObject::Delete();
}

CGeneralMuxer::~CGeneralMuxer()
{
}
//---------------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------------
AM_ERR CGeneralMuxer::ConstructMuxer(AM_INT index)
{
    CUintMuxerConfig* pConfig = &(mpConfig->unitConfig[index]);
    IGMuxer* curMuxer = mpMuxer[index];
    if(curMuxer != NULL){
        AM_ASSERT(0);
        return ME_BAD_STATE;
    }

    AM_ERR err = ME_OK;
    MuxerType type = pConfig->fileType;
    AM_INFO("ConstructMuxer type%d\n", type);
    switch(type)
    {
    case MUXER_SIMPLE_ES:
        curMuxer = CGSimpleSave::Create(this);
        if(curMuxer == NULL){
            AM_ERROR("Create CGSimpleSave for stream %d Failed!\n", index);
            err = ME_ERROR;
        }
        mpMuxer[index] = curMuxer;
        break;

    case MUXER_FFMPEG_MULTI:
        curMuxer = CGMuxerFFMpeg::Create(this, index);
        if(curMuxer == NULL){
            AM_ERROR("Create CGMuxerFFMpeg for stream %d Failed!\n", index);
            err = ME_ERROR;
        }
        mpMuxer[index] = curMuxer;
        break;

    default:
        AM_ASSERT(0);
        break;
    }
    return err;
}

AM_ERR CGeneralMuxer::ConfigMuxer(AM_INT index)
{
    IGMuxer* curMuxer = mpMuxer[index];
    if(curMuxer == NULL){
        AM_ASSERT(0);
        return ME_BAD_STATE;
    }
    AM_ERR err = ME_OK;

    //set parameters about auto separate file
    err = curMuxer->SetSavingTimeDuration(mDuration, mMaxFileCount);
    if(err != ME_OK){
        AM_ERROR("SetSavingStrategy Muxer for stream %d Failed!!\n", index);
        return err;
    }

    CUintMuxerConfig* pConfig = &(mpConfig->unitConfig[index]);
    err = curMuxer->ConfigMe(pConfig);
    if(err != ME_OK){
        AM_ERROR("Config Muxer for stream %d Failed!!\n", index);
    }
    return err;
}

AM_ERR CGeneralMuxer::UpdateConfig(AM_INT index)
{

    return ME_NOT_SUPPORTED;
}

AM_ERR CGeneralMuxer::FinishMuxer(AM_INT index)
{
    IGMuxer* curMuxer = mpMuxer[index];
    if(curMuxer == NULL){
        AM_ASSERT(0);
        return ME_BAD_STATE;
    }
    AM_ERR err = ME_OK;
    err = curMuxer->FinishMuxer();
    AM_DELETE(curMuxer);
    mpMuxer[index] = NULL;
    return err;
}

AM_ERR CGeneralMuxer::TerminateAll()
{
    AM_INT i = 0;
    IGMuxer* curMuxer;
    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        curMuxer = mpMuxer[i];
        if(curMuxer != NULL){
        }
    }
    return ME_OK;
}

AM_ERR CGeneralMuxer::NotifyFromMuxer(AM_INT index, AM_MSG& msg)
{
    //handle error msg
    switch(msg.code)
    {
    case IGMuxer::MUXER_MSG_ERROR:
        //TEST space info of sdcard of sata;
        //set all state to state_error; Or set saveMe flag
        break;

    default:
        AM_ASSERT(0);
        break;
    }
    return ME_OK;
}

AM_ERR CGeneralMuxer::FeedData(AM_INT index, CGBuffer* buffer)
{
    IGMuxer* curMuxer = mpMuxer[index];
    if(curMuxer == NULL){
        AM_ASSERT(0);
        buffer->ReleaseContent();
        return ME_BAD_STATE;
    }
    AM_ERR err = ME_OK;
//    CUintMuxerConfig* pConfig = &(mpConfig->unitConfig[index]);
    err = curMuxer->FeedData(buffer);
    return err;
}

AM_ERR CGeneralMuxer::QueryInfo(AM_INT index, AM_INT type, CParam& par)
{
    return ME_OK;
}

AM_ERR CGeneralMuxer::Dump()
{
    CUintMuxerConfig* pConfig = NULL;
    CMapTable* curMap = NULL;

    AM_INT i = 0;
    AM_INFO("\n---------------------MuxerSystem Info------------\n");
    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        pConfig = &(mpConfig->unitConfig[i]);
        curMap = &(mpGConfig->indexTable[i]);

        if(curMap->index != -1)
        {
            AM_INFO("       {---Source %d : EnMuxer:%d, State:%d.---}\n",
                curMap->index, pConfig->saveMe, mState[i]);
            if(mpMuxer[i])
                mpMuxer[i]->Dump();
        }
    }
    AM_INFO("\n");
    return ME_OK;
}

//This is a conven api...
AM_ERR CGeneralMuxer::AutoMuxer(AM_INT index, CGBuffer* buffer)
{
#if PLATFORM_LINUX
    //SendToInjector(index, buffer);
#endif

    //AM_INFO("AutoMuxer %d\n",buffer->GetIdentity());
    CUintMuxerConfig* pConfig = &(mpConfig->unitConfig[index]);
    MuxerState& curState = mState[index];
    IGMuxer* curMuxer = mpMuxer[index];

    pthread_mutex_lock(&pConfig->mutex_);
    if(pConfig->saveMe == AM_FALSE && curState != STATE_NULL){
        //stop save file, stop by video
        if(buffer->GetStreamType() == STREAM_AUDIO){
            buffer->ReleaseContent();
            pthread_mutex_unlock(&pConfig->mutex_);
            return ME_OK;
        }

        FinishMuxer(index);
        mState[index] = STATE_NULL;
        buffer->ReleaseContent();
        pthread_mutex_unlock(&pConfig->mutex_);
        return ME_OK;
    }
    //AM_INFO("aaaaa1 %d\n",buffer->GetIdentity());

    if(pConfig->saveMe == AM_FALSE){
        buffer->ReleaseContent();
        pthread_mutex_unlock(&pConfig->mutex_);
        return ME_OK;
    }
    pthread_mutex_unlock(&pConfig->mutex_);
    //AM_INFO("aaaaa2 %d\n",buffer->GetIdentity());

//    AM_ERR err = ME_OK;
    AM_BOOL bQuit = AM_FALSE;
    while(bQuit != AM_TRUE)
    {
        switch(curState)
        {
        case STATE_NULL:
            AM_ASSERT(curMuxer == NULL);
            if(ConstructMuxer(index) != ME_OK){
                curState = STATE_ERROR;
            }else{
                curState = STATE_INITED;
            }
            break;

        case STATE_INITED:
            if(ConfigMuxer(index) != ME_OK){
                curState = STATE_ERROR;
            }else{
                curState = STATE_FEED;
            }
            break;

        case STATE_FEED:
            FeedData(index, buffer);
            bQuit = AM_TRUE;
            break;

        case STATE_ERROR:
            buffer->ReleaseContent();
            bQuit = AM_TRUE;
            break;

        default:
            break;
        }
    }

    return ME_OK;
}

AM_ERR CGeneralMuxer::SetSavingTimeDuration(AM_UINT duration)
{
    AM_INT index = 0;
    IGMuxer* curMuxer = mpMuxer[index];
    if(mDuration != duration){
        mDuration = duration;
        while(curMuxer){
            curMuxer->SetSavingTimeDuration(mDuration, mMaxFileCount);
            index++;
            curMuxer = mpMuxer[index];
        }
        AM_ASSERT(index <= MDEC_SOURCE_MAX_NUM);
    }else{
        AM_INFO("old duration(%d) == new duration(%d), no change!!\n", mDuration, duration);
    }
    return ME_OK;
}

AM_ERR CGeneralMuxer::processMDMsg(AM_INT index, AM_INT event)
{
    IGMuxer* curMuxer = mpMuxer[index];
    if(curMuxer == NULL){
        //AM_ASSERT(0);
        return ME_BAD_STATE;
    }
    AM_ERR err = ME_OK;
    err = curMuxer->postMDMsg(event);
    return err;
}

#if PLATFORM_LINUX
//---------------------------------------------------------------------------
//RTMP injector
//---------------------------------------------------------------------------
AM_ERR CGeneralMuxer::SendToInjector(AM_INT index, CGBuffer* buffer,int64_t pts,AM_INT is_ds_live)
{
    //AM_INFO("SendToInjector %d\n",buffer->GetIdentity());
    CUintMuxerConfig* pConfig = &(mpConfig->unitConfig[index]);
    InjectorState& curState = mSendState[index];
    IGInjector* curInjector = mpInjector[index];

    pthread_mutex_lock(&pConfig->mutex_);
    if(pConfig->sendMe == AM_FALSE && curState != STATE_NULL_INJECTOR){
        curState = STATE_NULL_INJECTOR;
        if(curInjector == NULL){
            AM_ASSERT(0);
        }else{
            curInjector->FinishInjector();
            AM_DELETE(curInjector);
            mpInjector[index] = NULL;
            mSendState[index] = STATE_NULL_INJECTOR;
        }
    }
    if(pConfig->sendMe == AM_FALSE){
        pthread_mutex_unlock(&pConfig->mutex_);
        return ME_OK;
    }
    pthread_mutex_unlock(&pConfig->mutex_);

    AM_BOOL bQuit = AM_FALSE;
    while(bQuit != AM_TRUE)
    {
        switch(curState)
        {
        case STATE_NULL_INJECTOR:
            AM_ASSERT(curInjector == NULL);
            if(ConstructInjector(index) != ME_OK){
                curState = STATE_ERROR_INJECTOR;
            }else{
                curState = STATE_INITED_INJECTOR;
            }
            break;

        case STATE_INITED_INJECTOR:
            if(ConfigInjector(index,is_ds_live) != ME_OK){
                curState = STATE_ERROR_INJECTOR;
            }else{
                curState = STATE_FEED_INJECTOR;
            }
            break;

        case STATE_FEED_INJECTOR:
            SendDataInjector(index, buffer,pts);
            bQuit = AM_TRUE;
            break;

        case STATE_ERROR_INJECTOR:
            bQuit = AM_TRUE;
            break;

        default:
            break;
        }
    }
    return ME_OK;
}

AM_ERR CGeneralMuxer::ConstructInjector(AM_INT index)
{
    AM_INFO("ConstructInjector index:%d\n", index);
    CUintMuxerConfig* pConfig = &(mpConfig->unitConfig[index]);
    IGInjector* curInjector = mpInjector[index];
    if(curInjector != NULL){
        AM_ASSERT(0);
        return ME_BAD_STATE;
    }

    AM_ERR err = ME_OK;
    pthread_mutex_lock(&pConfig->mutex_);
    InjectorType type = pConfig->injectorType;
    pthread_mutex_unlock(&pConfig->mutex_);
    switch(type)
    {
    case INJECTOR_RTMP:
        curInjector = CGInjectorRtmp::Create(this);
        if(curInjector == NULL){
            AM_ERROR("Create CGInjectorRtmp for stream %d Failed!\n", index);
            err = ME_ERROR;
        }
        mpInjector[index] = curInjector;
        break;
    case INJECTOR_RTSP:
        curInjector = CGInjectorRtsp::Create(this);
        if(curInjector == NULL){
            AM_ERROR("Create CGInjectorRtsp for stream %d Failed!\n", index);
            err = ME_ERROR;
        }
        mpInjector[index] = curInjector;
        break;
    default:
        AM_ASSERT(0);
        break;
    }
    return err;
}

AM_ERR CGeneralMuxer::ConfigInjector(AM_INT index,AM_INT is_ds_live)
{
    IGInjector* curInjector = mpInjector[index];
    if(curInjector == NULL){
        AM_ASSERT(0);
        return ME_BAD_STATE;
    }
    AM_ERR err = ME_OK;

    CUintMuxerConfig* pConfig = &(mpConfig->unitConfig[index]);
    pthread_mutex_lock(&pConfig->mutex_);
    pConfig->ds_type_live = is_ds_live;//TODO
    err = curInjector->ConfigMe(pConfig);
    pthread_mutex_unlock(&pConfig->mutex_);
    if(err != ME_OK){
        AM_ERROR("Config Muxer for stream %d Failed!!\n", index);
    }
    return err;
}

AM_ERR CGeneralMuxer::SendDataInjector(AM_INT index, CGBuffer* buffer,int64_t pts)
{
    IGInjector* curInjector = mpInjector[index];
    if(curInjector == NULL){
        AM_ASSERT(0);
        return ME_BAD_STATE;
    }
    AM_ERR err = ME_OK;
    err = curInjector->FeedData(buffer,pts,pts);//todo
    return err;
}
#endif
