/*
 * general_header.cpp
 *
 * History:
 *    2012/6/11 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

extern "C" {
#define INT64_C
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

#include "general_header.h"

#define CHECK_RANGE(I) \
    do{ \
        if((I) < 0 || (I) >= MDEC_SOURCE_MAX_NUM){ \
            AM_ERROR("Check Index(%d) Failed On Line:%d!\n", I,  __LINE__); \
            return ME_BAD_PARAM; \
        } \
    }while(0);

//-----------------
//CGConfig
//-----------------
CGConfig::CGConfig():
    curHdIndex(-1),
    sourceNum(0),
    curIndex(0),
    version(2),
    generalCmd(0),
    globalFlag(0),
    oldCompatibility(NULL)
{
    AM_INT i;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        CDemuxerConfig* curDemConfig = &demuxerConfig[i];
        CDecoderConfig* curDecConfig = &decoderConfig[i];
        CRendererConfig* curRenConfig = &rendererConfig[i];
        CMapTable* curMap = &indexTable[i];
        //
        curDemConfig->disableAudio = AM_TRUE;
        curDemConfig->disableVideo = AM_FALSE;
        curDemConfig->hasVideo = AM_TRUE;
        curDemConfig->hasAudio = AM_TRUE;
        curDemConfig->paused = AM_FALSE;
        curDemConfig->sendHandleV = AM_FALSE;
        curDemConfig->sendHandleA = AM_FALSE;
        curDemConfig->pausedCnt = 0;
        curDemConfig->hided = AM_FALSE;
        curDemConfig->needStart = AM_TRUE;
        curDemConfig->hd = AM_FALSE;
        curDemConfig->netused = AM_FALSE;
        curDemConfig->edited = AM_FALSE;
        //
        curDecConfig->enDeinterlace = 0;
        curDecConfig->onlyFeed = AM_FALSE;
        //curDecConfig->isHdInstance = AM_FALSE;
        //
        curRenConfig->audioReady = AM_FALSE;
        curRenConfig->videoReady = AM_FALSE;
        //
        curMap->index = curMap->winIndex = curMap->sourceGroup = curMap->decIndex = curMap->renIndex = curMap->dspIndex = curMap->dsprenIndex = -1;
        curMap->isHd = AM_FALSE;
    }
    mainMuxer = new CGMuxerConfig();
    AM_ASSERT(mainMuxer != NULL);
    dspWinConfig.winChanged = AM_FALSE;
    dspWinConfig.winNumConfiged = dspRenConfig.renNumConfiged = 0;
    dspWinConfig.winNumNeedConfig = dspRenConfig.renNumNeedConfig = 0;
    dspRenConfig.renChanged = AM_FALSE;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        dspRenConfig.renConfig[i].renIndex = i;
        dspRenConfig.renConfig[i].winIndex2 = 0xff;

        dspWinConfig.winConfig[i].winIndex = i;
        dspWinConfig.winConfig[i].winOffsetX = -1;//default value;
    }
    audioManager = NULL;
    audioMainQ = CQueue::Create(NULL, this, sizeof(AM_INT), 1);
    AM_ASSERT(audioMainQ != NULL);

    //init
    dspConfig.addVideoDataType = 0;
    dspConfig.udecHandler = NULL;
    dspConfig.iavHandle = -1;

    mDecoderCap = DECODER_CAP_DSP;
}

CGConfig::~CGConfig()
{
    delete mainMuxer;
    AM_DELETE(audioMainQ);
}

AM_ERR CGConfig::SetDefaultWinRenMap(AM_INT index, AM_INT dspInstance)
{
    CHECK_RANGE(index);
/*
    CDecoderConfig* decConfig = &decoderConfig[index];
    CRendererConfig* renConfig = &rendererConfig[index];

    decConfig->dspInstance = dspInstance;
    renConfig->winConfig.winIndex = dspInstance;
    renConfig->renConfig.winIndex = dspInstance;
    renConfig->renConfig.renIndex = dspInstance;
    renConfig->renConfig.decIndex = dspInstance;
*/
    return ME_OK;
}

AM_ERR CGConfig::DemuxerSetDecoderEnv(AM_INT index, void* env, CParam& par)
{
    CHECK_RANGE(index);

    CDecoderConfig* decConfig = &decoderConfig[index];
    decConfig->decoderEnv = env;
    //handle par
    return ME_OK;
}

AM_INT CGConfig::GetAudioSource()
{
    AM_INT i = 0;
    AM_INT index = -1;
    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        CDemuxerConfig* demConfig = &demuxerConfig[i];
        if(demConfig->disableAudio == AM_FALSE){
            index = i;
            break;
        }
    }
    return index;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        CDemuxerConfig* demConfig = &demuxerConfig[i];
        if(demConfig->disableAudio == AM_FALSE){
            AM_ASSERT(i == index);
        }
    }
    return index;
}

void CGConfig::DisableAllAudio()
{
    AM_INT i = 0;
    for(; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        CDemuxerConfig* demConfig = &demuxerConfig[i];
        demConfig->disableAudio = AM_TRUE;
    }
}

//GRD call me on the very begining. Check the input source and check dsp/render are all be created.
AM_BOOL CGConfig::AllRenderOutReady()
{
    AM_INT i = 0;
    CDemuxerConfig* curDemConfig = NULL;
    CRendererConfig* curRenConfig = NULL;
    CMapTable* curMap = NULL;
    AM_BOOL hdCreated = AM_FALSE;

    // 1-way support, todo check this flag for nohd case
    if(globalFlag & NO_HD_WIN_NVR_PB){
        //AM_ASSERT(sourceNum == 1);
        //return AM_TRUE;
        hdCreated = AM_TRUE;
    }

    //AM_INFO("AllRender Sync Debug: sourceNum:%d\n", sourceNum);
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++)
    {
        //if(i >= sourceNum)
            //continue;
        curRenConfig = &rendererConfig[i];
        curDemConfig = &demuxerConfig[i];
        curMap = &indexTable[i];
        //AM_INFO("AllRender Sync Debug:%d, hd:%d, winIndex(sourceGroup):%d(%d), ready, A:%d, V:%d, disable, A:%d, V:%d\n", i, curDemConfig->hd, curMap->winIndex
            //,curMap->sourceGroup, curRenConfig->audioReady, curRenConfig->videoReady, curDemConfig->disableAudio, curDemConfig->disableVideo);

        if(curMap->sourceGroup < 0)
            continue;

        if(curDemConfig->hd == AM_FALSE){
            //audio
            if((!(globalFlag & HAVE_TRANSCODE)) && curDemConfig->disableAudio == AM_FALSE && curRenConfig->audioReady == AM_FALSE)
                break;
            //video
            if(curRenConfig->videoReady == AM_FALSE)
                break;
        }else{
            if(curMap->dspIndex != -1){
                if(curRenConfig->videoReady == AM_TRUE)
                    hdCreated = AM_TRUE;
            }
            if(hdCreated == AM_FALSE)
                break;
        }
    }
    if(i == MDEC_SOURCE_MAX_NUM)
        return AM_TRUE;
    return AM_FALSE;
}

AM_ERR CGConfig::InitMapTable(AM_INT index, const char* pFile, AM_INT sourceGroup)
{
    CHECK_RANGE(index);
    AM_ASSERT(index == curIndex);
    //AM_ASSERT(index == sourceNum);

    CMapTable* curMap = &indexTable[index];
    curMap->index = index;
    char* dst = (char*)curMap->file;
    strncpy(dst, pFile, 127);
    *(dst + 127) = '\0';

    CDemuxerConfig* pDemConfig = &(demuxerConfig[index]);
    curMap->isHd = pDemConfig->hd;
    curMap->sourceGroup = sourceGroup;
    return ME_OK;
}

AM_ERR CGConfig::DeInitMap(AM_INT index)
{
    CHECK_RANGE(index);
    CMapTable* curMap = &indexTable[index];
    curMap->index = curMap->winIndex = curMap->decIndex = curMap->renIndex = curMap->dspIndex = curMap->sourceGroup =  curMap->dsprenIndex = -1;
    curMap->isHd = AM_FALSE;
    return ME_OK;
}

AM_ERR CGConfig::SetMapDec(AM_INT index, AM_INT dec)
{
    CHECK_RANGE(index);

    CMapTable* curMap = &indexTable[index];
    AM_ASSERT(curMap->index == index);
    curMap->decIndex = dec;
    return ME_OK;
}

AM_ERR CGConfig::SetMapDsp(AM_INT index, AM_INT dsp)
{
    CHECK_RANGE(index);

    CMapTable* curMap = &indexTable[index];
    AM_ASSERT(curMap->index == index);
    curMap->dspIndex = dsp;
    return ME_OK;
}

AM_ERR CGConfig::SetMapRen(AM_INT index, AM_INT ren)
{
    CHECK_RANGE(index);

    CMapTable* curMap = &indexTable[index];
    AM_ASSERT(curMap->index == index);
    curMap->renIndex = ren;
    return ME_OK;
}
//-----------------
//CGBuffer
//-----------------
CGBuffer::CGBuffer():
    bufferType(NOINITED_BUFFER),
    gbufferPtr(0)
{
    mpExtra = 0;
    mFlags = 0;
}

void CGBuffer::Dump(const char* str)
{
    AM_INFO("----%s Buffer Dump(BP:%p):", str, mpPool);
    AM_INFO(" identity(%d), Count(%d), owner(%d), bufferType(%d), streamType(%d), extraPtr(%d), gExtraPtr(%d).----\n",
        identity, count, owner, bufferType, streamType,mpExtra, gbufferPtr);
}

AM_ERR CGBuffer::Retrieve()
{
    CGeneralBufferPool* gBp = (CGeneralBufferPool* )mpPool;
    gBp->Retrieve(this);
    return ME_OK;
}

AM_ERR CGBuffer::ReleaseContent()
{
    if(bufferType != DATA_BUFFER)
        return ME_OK;
    AVPacket* pkt = NULL;

    switch(owner)
    {
    case DEMUXER_FFMPEG:
        pkt = (AVPacket* )mpExtra;
        if(pkt != NULL){
            av_free_packet(pkt);
            delete pkt;
            mpExtra = (AM_INTPTR)NULL;
        }else{
            AM_ASSERT(0);
        }
        break;

    case DECODER_FFMEPG:
        AM_ASSERT(0);
        ReleaseFFMpegDec();
        break;

    case TRANSCODER_DSP:
        break;

    default:
        AM_ASSERT(0);
        break;
    }
    return ME_OK;
}

inline AM_ERR CGBuffer::ReleaseFFMpegDec()
{
    if(streamType == STREAM_AUDIO){
        AM_U16* ptr = (AM_U16* )mpExtra;
        delete[] ptr;
    }else{
        AM_ASSERT(0);
    }
    return ME_OK;
}

AM_U8* CGBuffer::PureDataPtr()
{
    AM_U8* rPtr = NULL;
    if(bufferType != DATA_BUFFER)
        return rPtr;

    AVPacket* pkt = NULL;
    switch(owner)
    {
    case DEMUXER_FFMPEG:
        pkt = (AVPacket* )mpExtra;
        if(pkt != NULL){
            rPtr = pkt->data;
        }else{
            AM_ASSERT(0);
        }
        break;

    case TRANSCODER_DSP:
        if (mpData != NULL) {
            rPtr = mpData;
        } else {
            AM_ASSERT(0);
        }
        break;

    default:
        AM_ASSERT(0);
        break;
    }
    return rPtr;
}

AM_UINT CGBuffer::PureDataSize()
{
    AM_UINT size = 0;
    if(bufferType != DATA_BUFFER)
        return size;

    AVPacket* pkt = NULL;
    switch(owner)
    {
    case DEMUXER_FFMPEG:
        pkt = (AVPacket* )mpExtra;
        if(pkt != NULL){
            size = pkt->size;
        }else{
            AM_ASSERT(0);
        }
        break;

    case TRANSCODER_DSP:
        if (mpData != NULL) {
            size = mBlockSize;
        } else {
            AM_ASSERT(0);
        }
        break;

    default:
        AM_ASSERT(0);
        break;
    }
    return size;
}

//-----------------
//CGMuxer
//-----------------
CUintMuxerConfig::CUintMuxerConfig():
    saveMe(AM_FALSE),
    sendMe(AM_FALSE),
    configVideo(AM_FALSE),
    configAudio(AM_FALSE),
    index(-1)
{
    fileName[0] = '\0';
    rtmpUrl[0] = '\0';
    ds_type_live = 1;
    pthread_mutex_init(&mutex_,NULL);
}
CUintMuxerConfig::~CUintMuxerConfig(){
    pthread_mutex_destroy(&mutex_);
}

AM_ERR CGMuxerConfig::CheckAction(AM_INT index, AM_BOOL isSave, AM_INT flag)
{
    CHECK_RANGE(index);
    AM_ERR err = ME_OK;
    CUintMuxerConfig& curUnit = unitConfig[index];
    pthread_mutex_lock(&curUnit.mutex_);
    switch(flag)
    {
    case SAVE_INJECTOR_FLAG:
    case SAVE_INJECTOR_RTSP_FLAG:
        if(curUnit.sendMe == isSave)
            err = ME_BUSY;
        break;

    default:
        if(curUnit.saveMe == isSave){
            err = ME_BUSY;
        }
    }
    pthread_mutex_unlock(&curUnit.mutex_);
    return err;
}

void CGMuxerConfig::SetGeneralMuxer(CGeneralMuxer* muxer)
{
    AM_ASSERT(muxer != NULL);
    generalMuxer = muxer;
}

AM_ERR CGMuxerConfig::InitUnitConfig(AM_INT index, const char* saveName, AM_INT flag)
{
    CHECK_RANGE(index);
    CUintMuxerConfig& curUnit = unitConfig[index];
    pthread_mutex_lock(&curUnit.mutex_);

    curUnit.index = index;

    if(flag == SAVE_INJECTOR_FLAG){
        curUnit.sendMe = AM_TRUE;
        //AM_INFO("1%s2\n", saveName);
        AM_ASSERT(strlen(saveName) <= 4095);
        if(!saveName){
            curUnit.rtmpUrl[0] = '\0';
        }else{
            strncpy((char*)curUnit.rtmpUrl, saveName, 4095);
            curUnit.rtmpUrl[strlen(saveName)+1] = '\0';
        }
        curUnit.injectorType = INJECTOR_RTMP;
    }else if(flag == SAVE_INJECTOR_RTSP_FLAG){
        curUnit.sendMe = AM_TRUE;
        //AM_INFO("1%s2\n", saveName);
        AM_ASSERT(strlen(saveName) <= 4095);
        if(!saveName){
            curUnit.rtmpUrl[0] = '\0';
        }else{
            strncpy((char*)curUnit.rtmpUrl, saveName, 4095);
            curUnit.rtmpUrl[strlen(saveName)+1] = '\0';
        }
        curUnit.injectorType = INJECTOR_RTSP;
    }else{
        curUnit.saveMe = AM_TRUE;
        //AM_INFO("1%s2\n", saveName);
        AM_ASSERT(strlen(saveName) <= 127);
        if(!saveName){
            curUnit.fileName[0] = '\0';
        }else{
            strncpy((char*)curUnit.fileName, saveName, 127);
            curUnit.fileName[strlen(saveName)+1] = '\0';
            if(flag & SAVE_NAME_WITH_TIME_FLAG)
                curUnit.nameWithTime = AM_TRUE;
            else
                curUnit.nameWithTime = AM_FALSE;
        }
        curUnit.fileType = ParseFileType((char*)curUnit.fileName);
    }
    pthread_mutex_unlock(&curUnit.mutex_);
    return ME_OK;
}

AM_ERR CGMuxerConfig::DeInitUnitConfig(AM_INT index, AM_INT flag)
{
    CHECK_RANGE(index);
    CUintMuxerConfig& curUnit = unitConfig[index];
    pthread_mutex_lock(&curUnit.mutex_);
    curUnit.index = -1;
    if(flag == SAVE_INJECTOR_FLAG || flag == SAVE_INJECTOR_RTSP_FLAG){
        curUnit.sendMe = AM_FALSE;
    }else{
        curUnit.saveMe = AM_FALSE;
    }
    pthread_mutex_unlock(&curUnit.mutex_);
    return ME_OK;
}

AM_ERR CGMuxerConfig::SetupMuxerInfo(AM_INT index, AM_BOOL isVideo, CParam& par)
{
    CHECK_RANGE(index);
    CUintMuxerConfig& curUnit = unitConfig[index];

    pthread_mutex_lock(&curUnit.mutex_);
    if(isVideo == AM_TRUE)
    {
        AM_INFO("SetupMuxerInfo Video for %d\n", index);
        curUnit.videoInfo.width = par[0];
        curUnit.videoInfo.height = par[1];
        curUnit.videoInfo.bitrate = par[2];
        curUnit.videoInfo.codec = par[3];
        if(curUnit.videoInfo.codec != CODEC_ID_H264)
            AM_ASSERT(0);

        curUnit.videoInfo.extrasize = par[4];
        if(curUnit.videoInfo.extrasize >= 1024){
            AM_ASSERT(0);
            AM_INFO("Extrasize:%d.\n", curUnit.videoInfo.extrasize);
        }
        AM_U8* ptr = (AM_U8* )(par[5]);
        memcpy(curUnit.videoInfo.extradata, ptr, curUnit.videoInfo.extrasize);
        curUnit.configVideo = AM_TRUE;

        //frame rate
        curUnit.framerate_den = par[6];
        curUnit.framerate_num = par[7];
    }else{
        AM_INFO("debug:%d, %d, %d, %d, %d\n", par[0], par[1], par[2], par[3], par[4]);
        curUnit.audioInfo.channels = par[0];
        curUnit.audioInfo.samplerate = par[1];
        curUnit.audioInfo.bitrate = par[2];
        curUnit.audioInfo.samplefmt = par[3];
        curUnit.audioInfo.codec = par[4];
        curUnit.configAudio = AM_TRUE;
    }
    pthread_mutex_unlock(&curUnit.mutex_);
    return ME_OK;
}

MuxerType CGMuxerConfig::ParseFileType(char* file)
{
    //AM_INFO("Begin!!!%s\n", file);

    MuxerType type = MUXER_SIMPLE_ES;
    const char* ext;
    const char* ffmpeg_muxer_ext = "avi,ts,mp4,mov";
    char ext_copy[32];
    char* copy_ptr;
    const char* ffmpeg_ptr;

    ext = strrchr(file, '.');
    if(ext)
    {
        ext++;
        ffmpeg_ptr = ffmpeg_muxer_ext;

        for(;;){
            copy_ptr = ext_copy;
            while(*ffmpeg_ptr != '\0' && *ffmpeg_ptr != ',')
                *copy_ptr++ = *ffmpeg_ptr++;

            *copy_ptr = '\0';
            //AM_INFO("copy test:%s00\n", ext_copy);
            if(!strcasecmp(ext, ext_copy)){
                type = MUXER_FFMPEG_MULTI;
                break;
            }
            if(*ffmpeg_ptr == '\0')
                break;
            ffmpeg_ptr++;
        }//end for(;;)
    }
    //AM_INFO("END!!! type%d\n", type);
    return type;
}
