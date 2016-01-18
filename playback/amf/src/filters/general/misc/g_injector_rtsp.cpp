/*
 * g_injector_rtsp.cpp
 *
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

#include "rtsp_vod.h"
#include "g_injector_rtsp.h"


//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
IGInjector* CGInjectorRtsp::Create(CGeneralMuxer* manager)
{
    CGInjectorRtsp* result = new CGInjectorRtsp(manager);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CGInjectorRtsp::CGInjectorRtsp(CGeneralMuxer* manager):
    inherited(),
    mpManager(manager),
    mpConfig(NULL)
{
    mpRtspInjector = NULL;
}

AM_ERR CGInjectorRtsp::Construct()
{
    AM_INFO("CGInjectorRtsp Construct!\n");

    mpRtspInjector = new RtspMediaSession;
    if(mpRtspInjector == NULL){
        AM_ERROR("Create rtsp injector failed!!!");
        return ME_NO_MEMORY;
    }

    //mpWorkQ->SetThreadPrio(1, 1);
    return ME_OK;
}

AM_ERR CGInjectorRtsp::SetupInjectorEnv(AM_INT ds_type_live)
{
    AM_ASSERT(mpConfig != NULL);

    if(mpConfig->configVideo == AM_TRUE)
    {
        if(mpConfig->videoInfo.codec == CODEC_ID_H264){
            if(mpConfig->videoInfo.extrasize > 0){
                mpRtspInjector->addVideoH264Stream(mpConfig->videoInfo.extradata, mpConfig->videoInfo.extrasize);
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
            mpRtspInjector->addAudioAACStream(mpConfig->audioInfo.samplerate, mpConfig->audioInfo.channels);
        }else if(mpConfig->audioInfo.codec == CODEC_ID_PCM_MULAW){
            mpRtspInjector->addAudioG711Stream(0);
        }else if(mpConfig->audioInfo.codec == CODEC_ID_PCM_ALAW){
            mpRtspInjector->addAudioG711Stream(1);
        }else if(mpConfig->audioInfo.codec == CODEC_ID_ADPCM_G726){
            mpRtspInjector->addAudioG726Stream(mpConfig->audioInfo.bitrate/*32000*/);
        }else{
            AM_INFO("Audio is not AAC/G711/G726, don't add to RTSP.");
        }
    }

    if(mpConfig->rtmpUrl[0] == '\0'){
        AM_ERROR("Rtsp StreamName don't setted, Error!");
        return ME_ERROR;
    }
    mpRtspInjector->setDestination(mpConfig->rtmpUrl);
    if(ds_type_live){
        mpRtspInjector->setDataSourceType(RtspMediaSession::DS_TYPE_LIVE);
    }else{
        mpRtspInjector->setDataSourceType(RtspMediaSession::DS_TYPE_FILE);
    }
    return ME_OK;
}

void CGInjectorRtsp::Delete()
{
    AM_INFO("CGInjectorRtmp Delete\n");
    if(mpRtspInjector != NULL)
        delete mpRtspInjector;

    inherited::Delete();
    AM_INFO("CGInjectorRtsp Delete Done\n");
}

CGInjectorRtsp::~CGInjectorRtsp()
{

}
//---------------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------------
AM_ERR CGInjectorRtsp::ConfigMe(CUintMuxerConfig* con)
{
    AM_ERR err = ME_OK;
    mpConfig = con;
    err = SetupInjectorEnv(mpConfig->ds_type_live);
    AM_INFO("ConfigMe Done:%d\n", err);
    return err;
}

AM_ERR CGInjectorRtsp::UpdateConfig(CUintMuxerConfig* con)
{
    return ME_OK;
}

AM_ERR CGInjectorRtsp::PerformCmd(AO::CMD& cmd, AM_BOOL isSend)
{
    return ProcessCmd(cmd);
}

AM_ERR CGInjectorRtsp::FeedData(CGBuffer* buffer,int64_t pts,int64_t dts)
{
    STREAM_TYPE type = buffer->GetStreamType();
    if(type != STREAM_VIDEO && type != STREAM_AUDIO){
        AM_ASSERT(0);
        return ME_BAD_FORMAT;
    }
    if(type == STREAM_VIDEO)
    {
        mpRtspInjector->sendData(RtspMediaSession::TYPE_H264, buffer->PureDataPtr(), buffer->PureDataSize(), pts);
        mInfo.videoSize += buffer->PureDataSize();
    }else{
        if(mpConfig->audioInfo.codec == CODEC_ID_AAC){
            mpRtspInjector->sendData(RtspMediaSession::TYPE_AAC, buffer->PureDataPtr(), buffer->PureDataSize(), pts);
        }else if(mpConfig->audioInfo.codec == CODEC_ID_PCM_MULAW){
            mpRtspInjector->sendData(RtspMediaSession::TYPE_G711_MU, buffer->PureDataPtr(), buffer->PureDataSize(), pts);
        }else if(mpConfig->audioInfo.codec == CODEC_ID_PCM_ALAW){
            mpRtspInjector->sendData(RtspMediaSession::TYPE_G711_A, buffer->PureDataPtr(), buffer->PureDataSize(), pts);
        }else if(mpConfig->audioInfo.codec == CODEC_ID_ADPCM_G726){
            mpRtspInjector->sendData(RtspMediaSession::TYPE_G726, buffer->PureDataPtr(), buffer->PureDataSize(), pts);
        }
        mInfo.audioSize += buffer->PureDataSize();
    }
    return ME_OK;
}

AM_ERR CGInjectorRtsp::FinishInjector()
{
    AM_INFO("FinishRtspInjector Done\n");
    return ME_OK;
}

AM_ERR CGInjectorRtsp::QueryInfo(AM_INT type, CParam& par)
{
    return ME_OK;
}

AM_ERR CGInjectorRtsp::Dump()
{
    return ME_OK;
}

AM_ERR CGInjectorRtsp::DoStop()
{
    return ME_OK;
}

AM_ERR CGInjectorRtsp::ProcessCmd(AO::CMD& cmd)
{
    return ME_OK;
}

