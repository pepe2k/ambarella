/*
 * g_injector_rtmp.cpp
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

extern "C" {
#define INT64_C
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

#include "general_muxer_save.h"

#include "rtmpclient.h"
#include "g_injector_rtmp.h"



//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
IGInjector* CGInjectorRtmp::Create(CGeneralMuxer* manager)
{
    CGInjectorRtmp* result = new CGInjectorRtmp(manager);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CGInjectorRtmp::CGInjectorRtmp(CGeneralMuxer* manager):
    inherited(),
    mpManager(manager),
    mpConfig(NULL)
{
    mpRtmpInjector = NULL;
}

AM_ERR CGInjectorRtmp::Construct()
{
    AM_INFO("CGInjectorRtmp Construct!\n");

    mpRtmpInjector = new RtmpClient;
    if(mpRtmpInjector == NULL){
        AM_ERROR("Create rtmp injector failed!!!");
        return ME_NO_MEMORY;
    }

    //mpWorkQ->SetThreadPrio(1, 1);
    return ME_OK;
}

AM_ERR CGInjectorRtmp::SetupInjectorEnv()
{
    AM_ASSERT(mpConfig != NULL);

    if(mpConfig->configVideo == AM_TRUE)
    {
        if(mpConfig->videoInfo.codec == CODEC_ID_H264){
            if(mpConfig->videoInfo.extrasize > 0){
                mpRtmpInjector->addVideoH264Stream(mpConfig->videoInfo.extradata, mpConfig->videoInfo.extrasize);
            }else{
                AM_INFO("Don't find extradata for video H264, Check me!");
            }
        }else{
            AM_INFO("Video is not H264, don't add to RTMP.");
        }
    }

    if(mpConfig->configAudio == AM_TRUE)
    {
        if(mpConfig->audioInfo.codec == CODEC_ID_AAC){
            mpRtmpInjector->addAudioAACStream(mpConfig->audioInfo.samplerate, mpConfig->audioInfo.channels);
        }else if(mpConfig->audioInfo.codec == CODEC_ID_PCM_MULAW){
            mpRtmpInjector->addAudioG711Stream(0);
        }else if(mpConfig->audioInfo.codec == CODEC_ID_PCM_ALAW){
            mpRtmpInjector->addAudioG711Stream(1);
        }else{
            AM_INFO("Audio is not AAC/G711, don't add to RTMP.");
        }
    }

    if(mpConfig->rtmpUrl[0] == '\0'){
        AM_ERROR("Rtmp Url don't setted, Error!");
        return ME_ERROR;
    }
    mpRtmpInjector->setDestination(mpConfig->rtmpUrl);
        //"rtmp://121.199.36.25/livepkgr/demo?adbe-live-event=liveevent");
    return ME_OK;
}

void CGInjectorRtmp::Delete()
{
    AM_INFO("CGInjectorRtmp Delete\n");
    if(mpRtmpInjector != NULL)
        delete mpRtmpInjector;

    inherited::Delete();
    AM_INFO("CGInjectorRtmp Delete Done\n");
}

CGInjectorRtmp::~CGInjectorRtmp()
{

}
//---------------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------------
AM_ERR CGInjectorRtmp::ConfigMe(CUintMuxerConfig* con)
{
    AM_ERR err = ME_OK;
    mpConfig = con;
    err = SetupInjectorEnv();
    AM_INFO("ConfigMe Done:%d\n", err);
    return err;
}

AM_ERR CGInjectorRtmp::UpdateConfig(CUintMuxerConfig* con)
{
    return ME_OK;
}

AM_ERR CGInjectorRtmp::PerformCmd(AO::CMD& cmd, AM_BOOL isSend)
{
    return ProcessCmd(cmd);
}

AM_ERR CGInjectorRtmp::FeedData(CGBuffer* buffer,int64_t pts,int64_t dts)
{
    STREAM_TYPE type = buffer->GetStreamType();
    if(type != STREAM_VIDEO && type != STREAM_AUDIO){
        AM_ASSERT(0);
        return ME_BAD_FORMAT;
    }
    if(type == STREAM_VIDEO)
    {
        mpRtmpInjector->sendData(RtmpClient::TYPE_H264, buffer->PureDataPtr(), buffer->PureDataSize(), 0);
        mInfo.videoSize += buffer->PureDataSize();
    }else{
        if(mpConfig->audioInfo.codec == CODEC_ID_AAC){
            mpRtmpInjector->sendData(RtmpClient::TYPE_AAC, buffer->PureDataPtr(), buffer->PureDataSize(), 0);
        }else if(mpConfig->audioInfo.codec == CODEC_ID_PCM_MULAW){
            mpRtmpInjector->sendData(RtmpClient::TYPE_G711_MU, buffer->PureDataPtr(), buffer->PureDataSize(), 0);
        }else if(mpConfig->audioInfo.codec == CODEC_ID_PCM_ALAW){
            mpRtmpInjector->sendData(RtmpClient::TYPE_G711_A, buffer->PureDataPtr(), buffer->PureDataSize(), 0);
        }
        mInfo.audioSize += buffer->PureDataSize();
    }
    return ME_OK;
}

AM_ERR CGInjectorRtmp::FinishInjector()
{
    AM_INFO("FinishInjector Done\n");
    return ME_OK;
}

AM_ERR CGInjectorRtmp::QueryInfo(AM_INT type, CParam& par)
{
    return ME_OK;
}

AM_ERR CGInjectorRtmp::Dump()
{
    return ME_OK;
}

AM_ERR CGInjectorRtmp::DoStop()
{
    return ME_OK;
}

AM_ERR CGInjectorRtmp::ProcessCmd(AO::CMD& cmd)
{
    return ME_OK;
}
