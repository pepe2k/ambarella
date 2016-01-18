/*
 * general_audio_manager.cpp
 *
 * History:
 *    2012/8/13 - [QingXiong Z] create file
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

extern "C" {
#define INT64_C
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

#include "general_audio_manager.h"

#define CHECK_RANGE(I) \
    do{ \
        if((I) < 0 || (I) >= MDEC_SOURCE_MAX_NUM){ \
            AM_ERROR("Check Index Failed On Line:%d!\n", __LINE__); \
            return ME_BAD_PARAM; \
        } \
        if(mpAVArray[I] != NULL){ \
            AM_ERROR("The Index %d isnot Empty On Line:%d!\n", I, __LINE__); \
            return ME_BUSY; \
        } \
    }while(0);

#define CHECK_RANGE_NULL(I) \
    do{ \
        if((I) < 0 || (I) >= MDEC_SOURCE_MAX_NUM){ \
            AM_ERROR("Check Index Failed On Line:%d!\n", __LINE__); \
            return ME_BAD_PARAM; \
        } \
        if(mpAVArray[I] == NULL){ \
            AM_ERROR("The Index %d is NULL!On Line:%d!\n", I, __LINE__); \
            return ME_BUSY; \
        } \
    }while(0);

#define SEEK_AUDIO_GAP(I) (FRAME_TIME_DIFF(I, 10))

//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
CGeneralAudioManager* CGeneralAudioManager::Create(CGConfig* pConfig)
{
    CGeneralAudioManager* result = new CGeneralAudioManager(pConfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
        }
    return result;
}

CGeneralAudioManager::CGeneralAudioManager(CGConfig* pConfig)
{
    AM_INT i = 0;
    for(; i < MDEC_SOURCE_MAX_NUM; i++){
        mpAVArray[i] = NULL;
    }
    mpGConfig = pConfig;
}

AM_ERR CGeneralAudioManager::Construct()
{
    mpAudioMainQ = CQueue::Create(NULL, this, sizeof(AM_INT), 1);
    if(mpAudioMainQ == NULL){
        AM_ASSERT(0);
        return ME_NO_MEMORY;
    }

    return ME_OK;
}

void CGeneralAudioManager::Delete()
{
    AM_INT i = 0;
    for(; i < MDEC_SOURCE_MAX_NUM; i++){
        if(mpAVArray[i] != NULL)
        {
            av_close_input_file(mpAVArray[i]);
            mpAVArray[i] = NULL;
        }
    }
    AM_DELETE(mpAudioMainQ);
    CObject::Delete();
}

CGeneralAudioManager::~CGeneralAudioManager()
{
}
//---------------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------------
CQueue* CGeneralAudioManager::AudioMainQ()
{
    return mpAudioMainQ;
}

AM_ERR CGeneralAudioManager::AddAudioSource(AM_INT index, AM_INT audio)
{
    CHECK_RANGE(index);
    if(mpGConfig->demuxerConfig[index].netused == AM_TRUE)
        return ME_OK;
    if(audio < 0)
        return ME_OK;

    AM_INT rval;
    const char* filename = (char*)mpGConfig->indexTable[index].file;
    AVFormatContext* pAV = mpAVArray[index];
    AM_INFO("Construct Audio Source for Index(%d), Source(%s).\n", index, filename);

    av_register_all();
    rval = av_open_input_file(&pAV, filename, NULL, 0, NULL);
    if (rval < 0)
    {
        AM_ASSERT(0);
        av_close_input_file(pAV);
        //mpAVArray[index] = NULL;
    }
    rval = av_find_stream_info(pAV);
    if (rval < 0)
    {
        AM_ASSERT(0);
        av_close_input_file(pAV);
        //mpAVArray[index] = NULL;
    }
    mAudio[index] = audio;
    mpAVArray[index] = pAV;
    AM_ASSERT(pAV->streams[audio]->codec->codec_type == AVMEDIA_TYPE_AUDIO);
    return ME_OK;
}

AM_ERR CGeneralAudioManager::DeleteAudioSource(AM_INT index)
{
    if(index < 0 || index >= MDEC_SOURCE_MAX_NUM){
        AM_ERROR("Check Index(%d) Failed On DeleteAudioSource!\n", index);
        return ME_BAD_PARAM;
    }
    if(mpAVArray[index] == NULL)
        return ME_OK;

    av_close_input_file(mpAVArray[index]);
    mpAVArray[index] = NULL;
    return ME_OK;
}

AM_ERR CGeneralAudioManager::SetAudioPosition(AM_INT index, AM_S64 startPts)
{
    AM_INFO("SetAudioPosition, want Pts:%lld\n", startPts);
    CHECK_RANGE_NULL(index);
    AM_ERR err = ME_OK;
    //AM_INFO("Debug:SetAudioPosition %lld\n", startPts);
    err = SeekToPosition2(index, startPts);
    //err = SeekToPosition(index, startPts);
    return err;
}

//change PTS outside!
AM_ERR CGeneralAudioManager::ReadAudioData(AM_INT index, CGBuffer& oBuffer)
{
    CHECK_RANGE_NULL(index);
//    AM_ERR err = ME_OK;
    AM_INT ret;
    AVFormatContext* pAV = mpAVArray[index];
    AVPacket tmpPkt;
//    AM_U64 curPts = 0;

    while(1)
    {
        av_init_packet(&tmpPkt);
        ret = av_read_frame(pAV, &tmpPkt);
        if(ret < 0)
        {
            AM_INFO("AudioManager for stream(%d) go to end!ret=%d.\n", index, ret);
            av_free_packet(&tmpPkt);
            oBuffer.SetOwnerType(DEMUXER_FFMPEG);
            oBuffer.SetBufferType(EOS_BUFFER);
            oBuffer.SetIdentity(index);
            oBuffer.SetStreamType(STREAM_AUDIO);
            return ME_CLOSED;
        }

        if(tmpPkt.stream_index != mAudio[index]){
            av_free_packet(&tmpPkt);
            continue;
        }

        av_dup_packet(&tmpPkt);
        AVPacket* pPkt = new AVPacket;
        ::memcpy(pPkt, &tmpPkt, sizeof(tmpPkt));

        oBuffer.SetExtraPtr((AM_INTPTR)pPkt);
        oBuffer.SetOwnerType(DEMUXER_FFMPEG);
        oBuffer.SetBufferType(DATA_BUFFER);
        oBuffer.SetIdentity(index);

        oBuffer.SetStreamType(STREAM_AUDIO);
        oBuffer.SetPTS(tmpPkt.pts);//change outside
        break;
    }

    return ME_OK;
}
//---------------------------------------------------------------------------
// Private Fun.
//---------------------------------------------------------------------------
#ifndef INT64_MIN
#define INT64_MIN (-0x7fffffffffffffffLL - 1)
#endif
#ifndef INT64_MAX
#define INT64_MAX (0x7FFFFFFFFFFFFFFFLL)
#endif
AM_ERR CGeneralAudioManager::SeekToPosition2(AM_INT index, AM_S64 startPts)
{
    AM_ERR err = ME_OK;
    AVFormatContext* pAV = mpAVArray[index];
    AM_INT ret;

    /*if((AM_U64)pAV->start_time != AV_NOPTS_VALUE && (AM_U64)pAV->start_time >= startPts){
        if ((ret = avformat_seek_file( pAV, -1,INT64_MIN, 0, INT64_MAX, AVSEEK_FLAG_BYTE))<0){
            AM_ERROR("seek by bytes fail, ret =%d, Seek return error.\n", ret);
            return ME_ERROR;
        }
    }else*/{

        AM_INFO("SeekToPosition2, Seek, seek_target = %lld.\n", startPts);
        if ((ret = avformat_seek_file(pAV, mAudio[index],INT64_MIN, startPts,INT64_MAX, 0)) < 0) {
            AM_ERROR("seek by time fail, err =%d, try by bytes.\n", ret);

            if(pAV->duration<=0) {
                AM_ERROR("duration unknow, cannot seek!!\n");
                return ME_ERROR;
            }
            //try seek by byte
            AM_S64 seek_target = startPts *pAV->file_size/pAV->duration;
            if ((ret = avformat_seek_file( pAV, -1,INT64_MIN, seek_target,INT64_MAX, AVSEEK_FLAG_BYTE))<0){
                AM_ERROR("seek by bytes fail, err =%d, Seek return error.\n", ret);
                return ME_ERROR;
            }
        }
    }
    AM_INFO("SeekToPosition2, Seek done, pos = %lld.\n", pAV->pb->pos);
    return err;
}

AM_ERR CGeneralAudioManager::SeekToPosition(AM_INT index, AM_S64 startPts)
{
    AM_ERR err = ME_OK;
    AVFormatContext* pAV = mpAVArray[index];
    AM_INT ret;
    AVPacket tmpPkt;
    AM_INFO("Audio SeekToPosition %lld for Stream: %d\n", startPts, index);
    //we need a self-directed mode to check the seek jump.
    bool bigFindBack = false;
    bool smallFindCloser = false;
    bool findCloser = true;
    bool checkDone = false;
    AM_INT smallCloser = 0;//need fast cross the small gap

    err = DoFFMpegSeek(index, startPts);
    AM_ASSERT(err == ME_OK);
    AM_S64 lastSeek = startPts;
    while(1)
    {
        av_init_packet(&tmpPkt);
        ret = av_read_frame(pAV, &tmpPkt);
        if(ret < 0)
        {
            AM_INFO("seek check meet eos! check me!ret=%d.\n", ret);
            AM_ASSERT(0);
            av_free_packet(&tmpPkt);
            return ME_CLOSED;
        }
        if(tmpPkt.stream_index != mAudio[index])
        {
            av_free_packet(&tmpPkt);
            continue;
        }

        if(bigFindBack == true && smallFindCloser == true){
            findCloser = false;
        }

        if(1)
        {
            if(tmpPkt.pts > startPts && checkDone != true){
                //AM_INFO("Adjuct Seek to: %lld, cur:%lld\n", startPts, tmpPkt.pts);
                //May be we cannot find a smaller position...
                if(lastSeek == 0){
                    //we set to 0, and still find a larger position
                    break;
                }
                lastSeek -= SEEK_AUDIO_GAP(index);
                if(lastSeek < 0){
                    lastSeek = 0;
                }
                err = DoFFMpegSeek(index, lastSeek);
                bigFindBack = true;
                if(err != ME_OK){
                    AM_ERROR("Seek Failed, PTS adjuct failed\n");
                    AM_ASSERT(0);
                    av_free_packet(&tmpPkt);
                    return ME_ERROR;
                }else{
                    av_free_packet(&tmpPkt);
                    continue;
                }
            }else{
                if(((startPts - tmpPkt.pts) >= FRAME_TIME_DIFF(index, 100)) && findCloser == true){
                    //AM_INFO("Too Long Diff , Seek again %lld, Want(%lld).\n", tmpPkt.pts, startPts);
                    av_free_packet(&tmpPkt);
                    lastSeek += (smallCloser/3 + 1) * SEEK_AUDIO_GAP(index);
                    smallFindCloser = true;
                    smallCloser++;
                    err = DoFFMpegSeek(index, lastSeek);
                    AM_ASSERT(err == ME_OK);
                    continue;
                }
                checkDone = true;
                if((startPts - tmpPkt.pts) > FRAME_TIME_DIFF(index, 3)){
                    av_free_packet(&tmpPkt);
                    continue;
                }
            }
        }
        break;
    }

    av_free_packet(&tmpPkt);
    return ME_OK;
}

AM_ERR CGeneralAudioManager::DoFFMpegSeek(AM_INT index, AM_S64 startPts)
{
    AM_INT ret;
    AM_U64 targeByte;
    AVFormatContext* pAV = mpAVArray[index];

    AM_INFO("DoFFMpegSeek:%lld\n", startPts);
    if((ret = av_seek_frame(pAV, -1, startPts, 0)) < 0)
    {
        targeByte = startPts * pAV->file_size /pAV->duration;
        AM_INFO("seek by timestamp(%lld) fail, ret =%d, try by bytes(%lld)--duration:%lld.\n", startPts, ret, targeByte, pAV->duration);
        if((ret = av_seek_frame(pAV, -1, targeByte, AVSEEK_FLAG_BYTE )) < 0) {
            AM_ERROR("seek by bytes fail, ret =%d, Seek return error.\n", ret);
            return ME_ERROR;
        }
    }
    return ME_OK;
}

//1audio frame diff TODO
inline AM_INT CGeneralAudioManager::FRAME_TIME_DIFF(AM_INT index, AM_INT diff)
{
/*    AM_INT ptsDiff;
    AM_INT temp, temp1;
    AM_INT eachDiff;
    AVFormatContext* pAV = mpAVArray[index];
    AM_INT streamId = mAudio[index];*/
    return diff * 3000;
}

