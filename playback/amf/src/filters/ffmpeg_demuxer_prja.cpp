
/*
 * ffmpeg_demuxer_prja.cpp
 *
 * History:
 *    2012/5/11 - [Steven Wang] created file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "ffmpeg_demux"
//#define AMDROID_DEBUG

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"

#include "am_pbif.h"
#include "pbif.h"
#include "engine_guids.h"
#include "filter_list.h"

#if PLATFORM_ANDROID
#include "sys/atomics.h"
#endif
#include "osal.h"

extern "C" {
#define INT64_C(a) (a ## LL)
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec_export.h"
#include "libavformat/mp4_dv.h"
}
#include "am_pridata.h"
#include "ffmpeg_demuxer_prja.h"
#include "am_ffmpeg.h"

#ifndef INT64_MIN
#define INT64_MIN (-0x7fffffffffffffffLL - 1)
#endif
#ifndef INT64_MAX
#define INT64_MAX INT64_C(9223372036854775807)
#endif

#if !FFMPEG_VER_0_6
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_SUBTITLE AVMEDIA_TYPE_SUBTITLE
#define CODEC_TYPE_DATA AVMEDIA_TYPE_DATA
#define guess_format av_guess_format
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif

#define MAX_PTS_DIFF 90000
#define AUDIO_BUF_CNT_IN_POOL   128

filter_entry g_ffmpeg_demuxer = {
    "ffmpegDemuxer",
    CFFMpegDemuxer::Create,
    CFFMpegDemuxer::ParseMedia,
    NULL,
};

IFilter* CreateFFMpegDemuxer(IEngine *pEngine)
{
    return CFFMpegDemuxer::Create(pEngine);
}

static void free_input_format(void *context)
{
    AM_INFO("free avformat\n");
    av_close_input_file((AVFormatContext*)context);
}

void CFFMpegDemuxer::printBytes(AM_U8* p, AM_UINT size)
{
#ifdef AM_DEBUG
    if (!(mLogOption & LogBinaryData))
        return;

    if (!p)
        return;

    while (size > 3) {
        AMLOG_BINARY("  %2.2x %2.2x %2.2x %2.2x.\n", p[0], p[1], p[2], p[3]);
        p += 4;
        size -= 4;
    }

    if (size == 3) {
        AMLOG_BINARY("  %2.2x %2.2x %2.2x.\n", p[0], p[1], p[2]);
    } else if (size == 2) {
        AMLOG_BINARY("  %2.2x %2.2x.\n", p[0], p[1]);
    } else if (size == 1) {
        AMLOG_BINARY("  %2.2x.\n", p[0]);
    }
#endif
}


int CFFMpegDemuxer::ParseMedia(struct parse_file_s *pParseFile, struct parser_obj_s *pParser)
{
    AVFormatContext *pFormat;

    //av_register_all();
    mw_ffmpeg_init();

    int rval = av_open_input_file(&pFormat, pParseFile->pFileName, NULL, 0, NULL);
    if (rval < 0) {
        AM_INFO("ffmpeg does not recognize %s\n", pParseFile->pFileName);
        return 0;
    }
    else{
        AM_INFO("FFMpegDemuxer: %s is [%s] format\n", pParseFile->pFileName, pFormat->iformat->name);
    }
    if(pFormat->drm_flag != 0){
        AM_ERROR("DRM protected stream detected, not supported now!!\n");
        av_close_input_file(pFormat);
        return 0;
    }
    pParser->context = pFormat;
    pParser->free = free_input_format;

    return 1;
}

//-----------------------------------------------------------------------
//
// CFFMpegBufferPool
//
//-----------------------------------------------------------------------
CFFMpegBufferPool* CFFMpegBufferPool::Create(const char *name, AM_UINT count)
{
    CFFMpegBufferPool *result = new CFFMpegBufferPool(name);
    if (result && result->Construct(count) != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CFFMpegBufferPool::OnReleaseBuffer(CBuffer *pBuffer)
{
    if (pBuffer->GetType() == CBuffer::DATA) {
        AVPacket *pPacket = (AVPacket*)((AM_U8*)pBuffer + sizeof(CBuffer));
        av_free_packet(pPacket);
//      static int counter = 0;
//      AM_INFO("release packet %d\n", ++counter);
    }
}

//-----------------------------------------------------------------------
//
// CFFMpegDemuxer
//
//-----------------------------------------------------------------------
IFilter* CFFMpegDemuxer::Create(IEngine *pEngine)
{
    CFFMpegDemuxer *result = new CFFMpegDemuxer(pEngine);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CFFMpegDemuxer::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    DSetModuleLogConfig(LogModuleFFMpegDemuxer);

    // video output pin & bp

    if ((mpVideoOutput = CFFMpegDemuxerOutput::Create(this)) == NULL)
        return ME_ERROR;
#if TARGET_USE_AMBARELLA_A5S_DSP
    /*Don't Buffer Too Many Video, Otherwise FB Look Odd*/
    if ((mpVideoBP = CFFMpegBufferPool::Create("ffmpeg demxer video", 30)) == NULL)
        return ME_ERROR;
#else
    if ((mpVideoBP = CFFMpegBufferPool::Create("ffmpeg demxer video", 196)) == NULL)
        return ME_ERROR;
#endif
    mpVideoOutput->SetBufferPool(mpVideoBP);

    // audio output pin & bp

    if ((mpAudioOutput = CFFMpegDemuxerOutput::Create(this)) == NULL)
        return ME_ERROR;

    if ((mpAudioBP = CFFMpegBufferPool::Create("ffmpeg demuxer audio", 128)) == NULL)
        return ME_ERROR;

    mpAudioOutput->SetBufferPool(mpAudioBP);

#if PLATFORM_ANDROID
    //construct subtitle output pin & buffer pool. added by cx
    if((mpSubtitleOutput = CFFMpegDemuxerOutput::Create(this)) == NULL)
        return ME_ERROR;
    if((mpSubtitleBP = CFFMpegBufferPool::Create("ffmeg demuxer subtitle", 96)) == NULL)
        return ME_ERROR;
    mpSubtitleOutput->SetBufferPool(mpSubtitleBP);
#endif

if (ePriDataParse_filter_parse == mpSharedRes->mPridataParsingMethod) {
    AMLOG_INFO("CFFMpegDemuxer create pridata outputpin, buffer pool, begin.\n");
    if((mpPridataOutput = CFFMpegDemuxerOutput::Create(this)) == NULL) {
        AM_ERROR("CFFMpegDemuxerOutput::Create fail.\n");
        return ME_ERROR;
    }
    if((mpPridataBP = CFFMpegBufferPool::Create("ffmeg demuxer pridata", 32)) == NULL){
        AM_ERROR("CFFMpegBufferPool::Create fail.\n");
        return ME_ERROR;
    }
    mpPridataOutput->SetBufferPool(mpPridataBP);
    AMLOG_INFO("CFFMpegDemuxer create pridata outputpin, buffer pool, end, output %p, bp %p.\n", mpPridataOutput, mpPridataBP);
}

    return ME_OK;
}

void CFFMpegDemuxer::Clear()
{
    if (mpFormat) {
        av_close_input_file(mpFormat);
        mpFormat = NULL;
    }
}

CFFMpegDemuxer::~CFFMpegDemuxer()
{
    AMLOG_DESTRUCTOR("~CFFMpegDemuxer.\n");
    AM_DELETE(mpVideoOutput);
    AM_RELEASE(mpVideoBP);

    AM_DELETE(mpAudioOutput);
    AM_RELEASE(mpAudioBP);

    //delete subtitle output pin and buffer pool. added by cx
    AM_DELETE(mpSubtitleOutput);
    AM_RELEASE(mpSubtitleBP);

    Clear();
    AMLOG_DESTRUCTOR("~CFFMpegDemuxer before freeAllPrivateData().\n");
    freeAllPrivateData();
    AMLOG_DESTRUCTOR("~CFFMpegDemuxer done.\n");
}

void *CFFMpegDemuxer::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IDemuxer)
        return (IDemuxer*)this;
if (ePriDataParse_engine_demuxer_post == mpSharedRes->mPridataParsingMethod) {
    if (refiid == IID_IDataRetriever)
        return (IDataRetriever*)this;
}
    return inherited::GetInterface(refiid);
}

void CFFMpegDemuxer::GetInfo(INFO& info)
{
    inherited::GetInfo(info);
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0;
    info.nOutput = 0;
    info.mIndex = mIndex;

#if ENABLE_VIDEO

    if (mVideo >= 0)
        info.nOutput++;
#endif

#if ENABLE_AUDIO

    if (mAudio >= 0)
        info.nOutput++;
#endif

#if ENABLE_SUBTITLE
    if (mSubtitle >= 0)
        info.nOutput++;
#endif

if (ePriDataParse_filter_parse == mpSharedRes->mPridataParsingMethod) {
    if (mPridata >= 0)
        info.nOutput++;
}

}

//ugly code blow, need improve
IPin* CFFMpegDemuxer::GetOutputPin(AM_UINT index)
{
    if (index == 0) {

#if ENABLE_VIDEO
        if (mVideo >= 0)
            return mpVideoOutput;
#endif
        if (mAudio >= 0)
            return mpAudioOutput;
        if (mSubtitle >= 0)
            return mpSubtitleOutput;
        if (mPridata >= 0) {
            return mpPridataOutput;
        }
    }
    else if (index == 1) {
        if (mVideo >= 0)
        {
            if (mAudio >= 0)
                return mpAudioOutput;
            else if(mSubtitle >= 0)
                return mpSubtitleOutput;
            else if (mPridata >= 0)
                return mpPridataOutput;
        }
    }
    else if (index == 2)
    {
        if(mVideo >= 0 && mAudio >= 0 && mSubtitle >= 0)
            return mpSubtitleOutput;
        else if(mVideo >= 0 && mAudio >= 0 && mPridata>= 0)
            return mpPridataOutput;
    } else if (index == 3) {
        AM_ASSERT(mVideo >= 0);
        AM_ASSERT(mAudio >= 0);
        AM_ASSERT(mSubtitle >= 0);
        AM_ASSERT(mPridata >= 0);
        if (mVideo >= 0 && mAudio >= 0 && mSubtitle >= 0 && mPridata>=0)
            return mpPridataOutput;
    }
    return NULL;
}

void CFFMpegDemuxer::Delete()
{
    AMLOG_DESTRUCTOR("CFFMpegDemuxer::Delete().\n");
    AM_RELEASE(mpVideoBP);
    mpVideoBP = NULL;

    AM_RELEASE(mpAudioBP);
    mpAudioBP = NULL;

    AM_RELEASE(mpSubtitleBP);
    mpSubtitleBP = NULL;

    AM_DELETE(mpVideoOutput);
    mpVideoOutput = NULL;


    AM_DELETE(mpAudioOutput);
    mpAudioOutput = NULL;

    AM_DELETE(mpSubtitleOutput);
    mpSubtitleOutput = NULL;

    Clear();

    inherited::Delete();
}

bool CFFMpegDemuxer::ProcessCmd(CMD& cmd)
{
    AM_ASSERT(msState != STATE_HAS_OUTPUTBUFFER);
    AMLOG_CMD("****CFFMpegDemuxer::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);

    switch (cmd.code) {
        case CMD_STOP:
            if(msState == STATE_HAS_INPUTDATA || msState == STATE_READY )
                av_free_packet(&mPkt);
            mbRun = false;
            CmdAck(ME_OK);
            break;

        case CMD_OBUFNOTIFY:
            if(mpBP && mpBP->GetFreeBufferCnt() > 0 && msState == STATE_HAS_INPUTDATA)
                msState = STATE_READY;
            break;

        case CMD_PAUSE:
            AM_ASSERT(!mbPaused);
            DoPause();
            mbPaused = true;
            break;

        case CMD_RESUME:
            if(msState == STATE_PENDING && !mbEOSAlreadySent)//fix bug#2450, if already EOS, no need continue reading frame, avoid repeating sending EOS
                msState = STATE_IDLE;
            DoResume();
            mbPaused = false;
            break;

        case CMD_FLUSH:
            AMLOG_INFO("CFFMpegDemuxer: CMD_FLUSH STATE %d .\n",msState);
            if (msState == STATE_HAS_INPUTDATA || msState == STATE_READY) { //mPkt has data
                av_free_packet(&mPkt);
            }

            msState = STATE_PENDING;
            mbVideoDataComes = false;
            mbAudioDataComes = false;
            mbFilterBlockedMsgSent = false;
            mbStreamStart = false;
            mLastPts = (AM_U64)AV_NOPTS_VALUE;
            mAudPtsSave4LeapChk = (AM_U64)AV_NOPTS_VALUE;
            mbFlushFlag = true;
            mbEOSAlreadySent = false;
            CmdAck(ME_OK);
            AMLOG_INFO("CFFMpegDemuxer: CMD_FLUSH done, STATE %d .\n",msState);
            break;

        case CMD_AVSYNC:
            CmdAck(ME_OK);
            break;

        case CMD_BEGIN_PLAYBACK:
            AMLOG_INFO("CFFMpegDemuxer: CMD_BEGIN_PLAYBACK, STATE %d .\n",msState);
            AM_ASSERT(msState == STATE_PENDING);
            msState = STATE_IDLE;
            mbVideoDataComes = false;
            mbAudioDataComes = false;
            mbFilterBlockedMsgSent = false;
            mLastPts = (AM_U64)AV_NOPTS_VALUE;
            mbFlushFlag = true;
            mbEOSAlreadySent = false;
            break;

        /*A5S DV Playback Test */
        case CMD_A5S_PLAYMODE:
            DoA5SPlayMode(cmd.res32_1);
            CmdAck(ME_OK);
            break;
        case CMD_A5S_PLAYNM:
            DoA5SPlayNM(cmd.res32_1, (AM_INT)cmd.res64_1);
            CmdAck(ME_OK);
            break;

        /*end A5S DV Test*/

        //only NVR rtsp streams can come here
        case CMD_RUN:
            AM_ASSERT(mpSharedRes->mDSPmode == 16);
            mbNVRRTSP = false;
            CmdAck(ME_OK);
            break;

        case CMD_SOURCE_FILTER_BLOCKED:
            break;

        case CMD_HTTP_THREAD_ENTER_BUFFERING_MODE:
            msState = STATE_HTTP_THREAD_BUFFERING;
            AMLOG_INFO("enter buffering %d%%\n", mHttpThreadDataLen/(mHttpThreadCacheSize/100));
            break;

        case CMD_HTTP_THREAD_EXIT_BUFFERING_MODE:
            msState = STATE_IDLE;
            AMLOG_INFO("leave buffering %d%%\n", mHttpThreadDataLen/(mHttpThreadCacheSize/100));
            break;

        case CMD_HTTP_THREAD_UPDATE_BUFFER:
            AMLOG_INFO("buffering %d%%\n", mHttpThreadDataLen/(mHttpThreadCacheSize/100));
            break;

        case CMD_HTTP_THREAD_ERROR_FOUND:
            AMLOG_INFO("!!!!!!http thread netword error found\n");
            msState = STATE_HTTP_THREAD_ERROR;
            break;

        case CMD_HTTP_THREAD_ERROR_FIXED:
            AMLOG_INFO("!!!!!!http thread netword error fixed\n");
            msState = STATE_IDLE;
            break;

        case CMD_HTTP_THREAD_ERROR_UNFIXABLE:
            AMLOG_INFO("!!!!!!http thread netword error unfixable\n");
            msState = STATE_IDLE;
            break;

        default:
            AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return false;
}

 //do something if using a network-based format:RTSP
void CFFMpegDemuxer::DoPause()
{
    AM_INT ret;
    const char* name = mpFormat->iformat->name;
    if(strcmp(name, "rtsp") == 0)
    {
        ret = av_read_pause(mpFormat);
        if(ret != 0)
            AM_INFO("rtsp pause fail!\n");
    }
}

void CFFMpegDemuxer::DoResume()
{
    AM_INT ret;
    const char* name = mpFormat->iformat->name;
    if(strcmp(name, "rtsp") == 0)
    {
        ret = av_read_play(mpFormat);
        if(ret != 0)
            AM_INFO("rtsp resume fail!\n");
    }
}

void CFFMpegDemuxer::OnRun()
{
    CmdAck(ME_OK);

    CBuffer *pBuffer;
    CMD cmd;
    CFFMpegDemuxerOutput* pOutput = NULL;
    AM_INT ret;
    mbRun = true;
    msState = STATE_IDLE;
    AM_U8 mbGENPTSFlagsSet = 0;
    //UpdatePTSConvertor();
    AM_BOOL mbKeyFramegot = 0;

    if(mSeekByByte<0){
        mSeekByByte= !!(mpFormat->iformat->flags & AVFMT_TS_DISCONT);
        if(mSeekByByte == 1)
            AMLOG_INFO("This File can only seek by byte!\n");
    }

    #if 1
    start_http_thread(mpFormat, (void*)this, httpThreadCallBackEx);
    #else
    mhttpBufferSize = 0;
    mhttpDataSize= 0;
    mbisCallBackUsed = 0;
    httpCallBack = &httpThreadCallBack;
    start_http_thread(httpCallBack, &mbisCallBackUsed, (void*)this);
    size_t buf_per = 0;
    if(!mbisCallBackUsed){
        AMLOG_WARN("CALL BACK is not used in FFMPEG!!!\n");
    }
    while(mbisCallBackUsed){
        if(mhttpDataSize >= mhttpBufferSize){
            AMLOG_WARN("CFFMpegDemuxer::onRun(), mhttpDataSize(%u) >= mhttpBufferSize(%u)!\n", mhttpDataSize, mhttpBufferSize);
            break;
        }

        if(buf_per != mhttpDataSize/(mhttpBufferSize/100)){
            buf_per = mhttpDataSize/(mhttpBufferSize/100);
            AMLOG_WARN("CFFMpegDemuxer::onRun(), buffer status = %u%c (%u %u)!\n", buf_per, '%', mhttpDataSize, mhttpBufferSize);
        }
        if(mpWorkQ->PeekCmd(cmd)){
            if(cmd.code == CMD_STOP){
                ProcessCmd(cmd);
                stop_http_thread();
                AMLOG_WARN("CFFMpegDemuxer::onRun(), stop_http_thread!\n");
                return;
            }else if((cmd.code == CMD_PAUSE) ||(cmd.code == CMD_RESUME) || (cmd.code == CMD_FLUSH)){
                ProcessCmd(cmd);
                AMLOG_WARN("CFFMpegDemuxer::onRun(), mbisCallBackUsed get cmd.code=%d!\n", cmd.code);
            }else{
                AMLOG_WARN("CFFMpegDemuxer::onRun(), mbisCallBackUsed get cmd.code=%d!\n", cmd.code);
            }
        }
        usleep(10000);
    }
    #endif

    //only NVR rtsp streams can come here
    while(mbNVRRTSP){
        if(mpWorkQ->PeekCmd(cmd)){
            AM_ASSERT(cmd.code == CMD_RUN);
            ProcessCmd(cmd);
            AMLOG_INFO("CFFMpegDemuxer::onRun(), NVR Rdy to run!\n");
            mbNVRRTSP = false;
            break;
        }else{
            ret = av_read_frame(mpFormat, &mPkt);
            if(ret < 0) {
                if(ret == -EAGAIN){
                    av_free_packet(&mPkt);
                    continue;
                }
                av_free_packet(&mPkt);
                //AMLOG_WARN("av_read_frame()<0, stream end? ret=%d.\n", ret);
            }else{
                av_free_packet(&mPkt);
            }
        }
    }

    //AMLOG_INFO("CFFMpegDemuxer::OnRun, pridata enabled %d, index %d.\n", mbEnablePridata, mPridata);

    while (mbRun) {
#ifdef AM_DEBUG
        AMLOG_STATE("FFDemuxer: start switch, msState=%d.\n", msState);
        if (mpVideoBP && mVideo>=0) {
            AMLOG_STATE("video buffers %d.\n", mpVideoBP->GetFreeBufferCnt());
        }
        if (mpAudioBP && mAudio>=0) {
            AMLOG_STATE("audio buffers %d.\n", mpAudioBP->GetFreeBufferCnt());
        }
#endif
        switch (msState) {
            case STATE_IDLE:
                if (mpWorkQ->PeekCmd(cmd)) {
                    ProcessCmd(cmd);
                    break;
                }

                if(mbPaused) {
                    msState = STATE_PENDING;
                    break;
                }

                //test case
                if (mpSharedRes->play_video_es && mbEnableVideo) {
                    mbEnableVideo = false;
                    mbVideoDataComes = true;
                    if (!mpVideoOutput->AllocBuffer(pBuffer)) {
                        AM_ERROR("demuxer AllocBuffer Fail, exit \n");
                        mbRun = false;
                        break;
                    }
                    pBuffer->SetType(CBuffer::TEST_ES);
                    pBuffer->SetDataSize(0);
                    pBuffer->SetDataPtr(NULL);
                    mpVideoOutput->SendBuffer(pBuffer);
                    AMLOG_INFO("CFFMpegDemuxer:try send es data, %d.\n", mpSharedRes->also_demux_file);
                    if (0 == mpSharedRes->also_demux_file) {
                        msState = STATE_PENDING;
                        break;
                    }
                }

                if(!mbStreamStart){
                    mpFormat->flags = mpFormat->flags | AVFMT_FLAG_GENPTS;
                    mbStreamStart = true;
                }
                //specail case: should get packet first, than specify use which output
                ret = av_read_frame(mpFormat, &mPkt);
                if(ret < 0) {
                    AMLOG_INFO("av_read_frame()<0, stream end? ret=%d.\n", ret);
                    if(ret == -EAGAIN){
                        av_free_packet(&mPkt);
                        #if 0
                        //for bug#2345, if av_read_frame() return -EAGAIN
                        //(1) demuxer state remain in STATE_IDLE
                        //(2) demuxer also should reply CMD
                        if(mpWorkQ->PeekCmd(cmd)) {
                            ProcessCmd(cmd);
                            if(mbisCallBackUsed && cmd.code == CMD_STOP){
                                stop_http_thread();
                                AMLOG_WARN("CFFMpegDemuxer::onRun(), stop_http_thread while av_read_frame()!\n");
                            } else if((cmd.code == CMD_PAUSE) ||(cmd.code == CMD_RESUME) || (cmd.code == CMD_FLUSH)){
                                ProcessCmd(cmd);
                                AMLOG_WARN("CFFMpegDemuxer::onRun(), av_read_frame get cmd.code=%d!\n", cmd.code);
                            }else{
                                AMLOG_WARN("CFFMpegDemuxer::onRun(), av_read_frame, cmd.code = %d.\n", cmd.code);
                            }
                        }else{
                            AMLOG_WARN("CFFMpegDemuxer::onRun() av_read_frame, buffersize = %u.\n", mhttpDataSize);//will remove this log
                            usleep(100000);
                        }
                        #else
                        #endif
                        break;
                    }

                    if(mpSharedRes->mbLoopPlay){
                        AMLOG_INFO("replay this file, mbLoopPlay = 1.\n");
                        AM_U64 ms = 0;
                        Seek(ms);
                        break;
                    }
                    //to do
                    // send video/audio/subtitle EOS
                    if (mVideo >= 0 && mpVideoOutput)
                        SendEOS(mpVideoOutput);
                    if (mAudio >= 0 && mpAudioOutput)
                        SendEOS(mpAudioOutput);
                    if (mSubtitle >= 0 && mpSubtitleOutput)
                        SendEOS(mpSubtitleOutput);
                    if (mPridata >= 0 && mpPridataOutput)
                        SendEOS(mpPridataOutput);
                    av_free_packet(&mPkt);
                    msState = STATE_PENDING;
                    mbEOSAlreadySent = true;
                    break;
                }

#ifdef AM_DEBUG
               DumpEs(mPkt.data,mPkt.size, mPkt.stream_index);
#endif

                if(mpSharedRes->mDSPmode==16){
                    if(!mbAudioDataComes && (!mbEnableVideo || mVideo<0))
                        mbKeyFramegot = 1;
                    else if(!mbVideoDataComes && !mbAudioDataComes)
                        mbKeyFramegot = 0;
                    if(!mbKeyFramegot){
                        if(mPkt.stream_index == mVideo){
                            if(!(mPkt.flags & AV_PKT_FLAG_KEY)){
                                AMLOG_DEBUG("=========FFDemuxer: get a non-key frame, discard..=========\n");
                                mpBP = NULL;
                                av_free_packet(&mPkt);
                                break;
                            }else{
                                mbKeyFramegot = 1;
                            }
                        }else{
                            mpBP = NULL;
                            av_free_packet(&mPkt);
                            break;
                        }
                    }
                }

                if (mPkt.stream_index == mVideo && mbEnableVideo) {
                    mpBP = mpVideoBP;
                    pOutput = mpVideoOutput;
                    AMLOG_STATE("FFDemuxer: get video packet, data size =%d.\n",mPkt.size);

                    if(mWidth == 0 || mHeight == 0){
                        mWidth = mpFormat->streams[mPkt.stream_index]->codec->width;
                        mHeight = mpFormat->streams[mPkt.stream_index]->codec->height;
                    }

                    // workaround:
                    // support resolution change check for RTSP only
                    // fix 1766
                    if (strcmp(mpFormat->iformat->name, "rtsp") == 0 && mpSharedRes->decCurrType == DEC_HARDWARE) {
                        if (mWidth != mpFormat->streams[mPkt.stream_index]->codec->width
                         || mHeight != mpFormat->streams[mPkt.stream_index]->codec->height) {

                            mWidth = mpFormat->streams[mPkt.stream_index]->codec->width;
                            mHeight = mpFormat->streams[mPkt.stream_index]->codec->height;

                            AM_WARNING("during playback, the video's resolution changed.\n");
                            PostEngineMsg(IEngine::MSG_VIDEO_RESOLUTION_CHANGED);
                            //send eos
                            PostEngineMsg(IEngine::MSG_SOURCE_FILTER_BLOCKED);

                            // send video/audio/subtitle EOS
                            if (mVideo >= 0 && mpVideoOutput)
                                SendEOS(mpVideoOutput);
                            if (mAudio >= 0 && mpAudioOutput)
                                SendEOS(mpAudioOutput);
                            if (mSubtitle >= 0 && mpSubtitleOutput)
                                SendEOS(mpSubtitleOutput);
                            if (mPridata >= 0 && mpPridataOutput)
                                SendEOS(mpPridataOutput);
                            av_free_packet(&mPkt);
                            msState = STATE_PENDING;
                            break;
                        }
                        if(mPkt.pts > (AM_S64)mLastPts){
                            if((mPkt.pts - (AM_S64)mLastPts) >= MAX_PTS_DIFF){
                                AM_WARNING("the video pts diff between last pts and current pts is %lld, greater than MAX_PTS_DIFF: %d\n",mPkt.pts - (AM_S64)mLastPts, MAX_PTS_DIFF);
                                //post one fake err msg to engine, error code: 0x01030000
                                //decoder type = 1, error level = 3, error type = 0
                                mpSharedRes->error_code = 0x1030000;
                                PostEngineErrorMsg(ME_UDEC_ERROR);
                            }
                        }
                    }

                    //LiuGang move this flags out, to GenPTS for 1 more frame
                    //to solve bug1504: there is a BI after I
                    if(!mbGENPTSFlagsSet && mbStreamStart){
                        mpFormat->flags = mpFormat->flags & (~AVFMT_FLAG_GENPTS);
                        mbGENPTSFlagsSet = 1;
                    }
                    if((!mbStreamStart) && (mPkt.flags & AV_PKT_FLAG_KEY)){
                       //mpFormat->flags = mpFormat->flags & (~AVFMT_FLAG_GENPTS);
                       mbStreamStart = true;
                    }
                    //test only
                    if (mLogOutput & LogForDebugTest) {
                        mbEnableVideo = false;
                        mbVideoDataComes = true;
                        av_free_packet(&mPkt);
                        if (!pOutput->AllocBuffer(pBuffer)) {
                            AM_ERROR("demuxer AllocBuffer Fail, exit \n");
                            av_free_packet(&mPkt);
                            mbRun = false;
                            break;
                        }
                        pBuffer->SetType(CBuffer::TEST_ES);
                        pBuffer->SetDataSize(0);
                        pBuffer->SetDataPtr(NULL);
                        pOutput->SendBuffer(pBuffer);
                        msState = STATE_IDLE;
                        AMLOG_INFO("CFFMpegDemuxer:try send es data.\n");
                        break;

                    } else {
                        // for bug1447, handle case: packet pts is 0 when demux error happens
                        if (((AM_S64)mLastPts) >= 0 && (mPkt.pts == 0)) {
                            mPkt.pts = AV_NOPTS_VALUE;
                        }

                        // workaround: fix bug2580
                        // if pts is less than dts, which means something is wrong with this frame
                        // estimate new pts = last_pts + video_tick
                        if (mPkt.pts > 0 && mPkt.pts < mPkt.dts && (AM_S64)mLastPts >= 0) {
                            mPkt.pts = mLastPts*mPTSVideo_Den/mPTSVideo_Num + mpSharedRes->mVideoTicks;
                        }

                        if (mPkt.pts >= 0) {
                            AMLOG_PTS(" [PTS] FFDemuxer: video packet pts %lld, ", mPkt.pts);
                            ConvertVideoPts((am_pts_t&)mPkt.pts);
                            AMLOG_PTS(" after convert pts %llu.\n", mPkt.pts);
                        } else {
                            AMLOG_PTS(" video packet no pts.\n");
                            AMLOG_PTS(" [DTS] FFDemuxer: video packet dts %lld, ", mPkt.dts);
                            ConvertVideoPts((am_pts_t&)mPkt.dts);
                            AMLOG_PTS(" after convert dts %llu.\n", mPkt.dts);
                        }
                        if (!mbVideoDataComes) {
                            mbVideoDataComes = true;
                        }

                        if (mbFlushFlag && mPkt.pts < 0) {
                            mPkt.pts = mPkt.dts;
                            mbFlushFlag = false;
                        }

                        if (!mpSharedRes->mbVideoFirstPTSVaild) {
                            if (!strncmp(mpFormat->iformat->name, "mpegts", 6) ||
                                !strncmp(mpFormat->iformat->name, "mpeg", 4)) {
                                // for mpeg124 ts/ps
                                // it's better to get first key frame pts when av_sync

                                switch (mpFormat->streams[mVideo]->codec->codec_id) {
                                    case CODEC_ID_H264:
                                    case CODEC_ID_VC1:
                                    case CODEC_ID_MPEG1VIDEO:
                                    case CODEC_ID_MPEG2VIDEO:
                                    case CODEC_ID_MPEG4:
                                        if (mPkt.flags&AV_PKT_FLAG_KEY) {
                                            mpSharedRes->videoFirstPTS = (am_pts_t)mPkt.pts;
                                            mpSharedRes->mbVideoFirstPTSVaild = true;
                                        }
                                        break;
                                    default:
                                        mpSharedRes->videoFirstPTS = (am_pts_t)mPkt.pts;
                                        mpSharedRes->mbVideoFirstPTSVaild = true;
                                        break;
                                }
                            } else {
                                mpSharedRes->videoFirstPTS = (am_pts_t)mPkt.pts;
                                mpSharedRes->mbVideoFirstPTSVaild = true;
                            }
                            AMLOG_INFO("[AV sync]: get first video pts %llu.\n", mpSharedRes->videoFirstPTS);
                        }
                    }
                } else if (mPkt.stream_index == mAudio && mbEnableAudio) {
                    mpBP = mpAudioBP;
                    pOutput = mpAudioOutput;
                    AMLOG_STATE("FFDemuxer: get audio packet, data size =%d.\n",mPkt.size);
                    if (mPkt.pts >= 0) {
                        AMLOG_PTS(" [PTS] FFDemuxer: audio packet pts %lld, ", mPkt.pts);
                        ConvertAudioPts((am_pts_t&)mPkt.pts);
                        AMLOG_PTS(" after convert pts %llu.\n", mPkt.pts);
                    } else {
                        AMLOG_PTS(" audio packet no pts.\n");
                    }
                    if (!mbAudioDataComes) {
                        mbAudioDataComes = true;
                    }

                if (!mpSharedRes->mbAudioFirstPTSVaild) {
                    mpSharedRes->audioFirstPTS = (am_pts_t)mPkt.pts;
                    mpSharedRes->mbAudioFirstPTSVaild = true;
                    AMLOG_INFO("[AV sync]: get first audio pts %llu.\n", mpSharedRes->audioFirstPTS);
                }

                    if(mVideo>=0 && mAudio>=0 &&
                        mpSharedRes->pbConfig.avsync_enable && mpSharedRes->pbConfig.quickAVSync_enable &&
                        mpSharedRes->mbAudioFirstPTSVaild && mpSharedRes->mbVideoFirstPTSVaild &&
                        AV_NOPTS_VALUE!=mpSharedRes->audioFirstPTS && AV_NOPTS_VALUE!=mpSharedRes->videoFirstPTS &&
                        mpSharedRes->audioFirstPTS<mpSharedRes->videoFirstPTS &&
                        (mPkt.pts+mWaitThreshold*2)<(mpSharedRes->videoFirstPTS))//modify for bug#2332,  FIXME, conditions improvement needed
                    {
                        AMLOG_WARN("quickAVsync free audio data here,  audioFirstPTS=%llu, videoFirstPTS=%llu, mPkt.pts=%lld, mWaitThreshold=%lld.\n",mpSharedRes->audioFirstPTS,mpSharedRes->videoFirstPTS,mPkt.pts,mWaitThreshold);
                        av_free_packet(&mPkt);
                        break;
                    }

                    //for bug1915
                    if((AV_NOPTS_VALUE!=mPkt.pts)                                                                                                 //condition 1) current PTS is not AV_NOPTS_VALUE
                        && (0x7FFFFFFFFFFFFFFFLL!=mAudPtsSave4LeapChk)                                                               //condition 2) last PTS is not max value 0x7FFFFFFFFFFFFFFF nor AV_NOPTS_VALUE, to void decent wrap round
                        && (AV_NOPTS_VALUE!=mAudPtsSave4LeapChk)
                        && (((AM_S64)mAudPtsSave4LeapChk-mPkt.pts)>(AM_S64)mpSharedRes->mVideoTicks*96))//condition 3) leap is a large gap which larger than 24*4  video frame ticks
                    {
                        AMLOG_WARN("mAudPtsSave4LeapChk=%lld, mPkt.pts=%lld, gap=%lld, threshold=%d, audio PTS leap occur.\n",mAudPtsSave4LeapChk,mPkt.pts,mAudPtsSave4LeapChk-mPkt.pts,mpSharedRes->mVideoTicks*96);
                        PostEngineMsg(IEngine::MSG_AUDIO_PTS_LEAPED);
                    }
                    mAudPtsSave4LeapChk = mPkt.pts;
                    //FIXME, conditions improvement needed

                } else if (mPkt.stream_index == mSubtitle && mbEnableSubtitle) {
                    mpBP = mpSubtitleBP;
                    pOutput = mpSubtitleOutput;
                    AMLOG_STATE("FFDemuxer: get subtitle packet, data size =%d.\n",mPkt.size);
                } else if (mPkt.stream_index == mPridata && mbEnablePridata) {
                    AMLOG_DEBUG("FFDemuxer: get pridata packet, data size %d, data %p.\n",mPkt.size, mPkt.data);
                    SPriDataHeader* p_head = (SPriDataHeader*)mPkt.data;
                    //AM_ASSERT(DAM_PRI_MAGIC_TAG == p_head->magic_number);
                    //only process our private data here
                    if (DAM_PRI_MAGIC_TAG == p_head->magic_number) {
                        AMLOG_DEBUG("our private data comes, output %p, bp %p, method %d.\n", mpPridataOutput, mpPridataBP, mpSharedRes->mPridataParsingMethod);
                        if (ePriDataParse_engine_demuxer_post == mpSharedRes->mPridataParsingMethod) {
                            //store and post msg
                            storePrivateData(mPkt.data, mPkt.size);
#ifdef AM_DEBUG
                            printBytes(mPkt.data, mPkt.size);
#endif
                            mpBP = NULL;
                            av_free_packet(&mPkt);
                            break;
                        } else if (ePriDataParse_filter_parse == mpSharedRes->mPridataParsingMethod) {
                            AM_ASSERT(mpPridataBP);
                            AM_ASSERT(mpPridataOutput);
                            mpBP = mpPridataBP;
                            pOutput = mpPridataOutput;
                        } else {
                            mpBP = NULL;
                            av_free_packet(&mPkt);
                            break;
                        }
                        //AMLOG_INFO("pridata: %s.\n", mPkt.data + sizeof(SPriDataHeader));
                    } else {
                        //discard, not our private data
                        mpBP = NULL;
                        av_free_packet(&mPkt);
                        break;
                    }
                } else {
                    AMLOG_STATE("FFDemuxer: get other packet [%d], data size =%d, mbEnableVideo %d, mbEnableAudio %d.\n",mPkt.stream_index, mPkt.size, mbEnableVideo, mbEnableAudio);
                    mpBP = NULL;
                    av_free_packet(&mPkt);
                    break;
                }

                if(mpBP && mpBP->GetFreeBufferCnt() > 0) {
                    msState = STATE_READY;
                } else {
                    if (!mbFilterBlockedMsgSent) {
                        AMLOG_INFO("[Demuxer flow]: notify source filter is blocked.\n");
                        PostEngineMsg(IEngine::MSG_SOURCE_FILTER_BLOCKED);
                        mbFilterBlockedMsgSent = true;
                    }
                    msState = STATE_HAS_INPUTDATA;
                }
                break;

            case STATE_PENDING:
            case STATE_HAS_INPUTDATA:
            case STATE_HTTP_THREAD_ERROR:
            case STATE_HTTP_THREAD_BUFFERING:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_READY:
                //process cmd/msg first
                while(mpWorkQ->PeekCmd(cmd)) {
                    ProcessCmd(cmd);
                }
                if((msState != STATE_READY) || (mbRun==false))
                    break;
                AM_ASSERT(mpBP && mpBP->GetFreeBufferCnt());
                AMLOG_VERBOSE("CFFMpegDemuxer: bufferpool has %d free buffers, process cmd done.\n",mpBP->GetFreeBufferCnt());

                if (!pOutput->AllocBuffer(pBuffer)) {
                    AM_ERROR("demuxer AllocBuffer Fail, exit \n");
                    av_free_packet(&mPkt);
                    mbRun = false;
                    break;
                }
                pBuffer->SetType(CBuffer::DATA);
                pBuffer->SetDataSize(0);
                pBuffer->SetDataPtr(NULL);
                if (pOutput != mpPridataOutput) {
                    pBuffer->SetPTS((AM_U64)mPkt.pts);
                    if(mPkt.stream_index == mVideo)
                    {
                        mLastPts = mPkt.pts;
                    }
                    if (mPkt.pts > 0) {
                        //last valid pts, for private data
                        mEstimatedPTS = mPkt.pts;
                    }
                }

                av_dup_packet(&mPkt);
                if (pOutput == mpPridataOutput) {
                    //for private data
                    pBuffer->SetDataSize((AM_UINT)mPkt.size);
                    pBuffer->SetDataPtr((AM_U8*)mPkt.data);
                    pBuffer->SetPTS(mEstimatedPTS);
                }
                ::memcpy((AM_U8*)pBuffer + sizeof(CBuffer), &mPkt, sizeof(mPkt));
                pOutput->SendBuffer(pBuffer);
                msState = STATE_IDLE;
                AMLOG_VERBOSE("CFFMpegDemuxer: send buffer done.\n");
                break;

            default:
                AM_ERROR("demuxer error state %d.\n", msState);

        }

    }
    //mpVideoBP->SetNotifyOwner(NULL);
    //mpAudioBP->SetNotifyOwner(NULL);

    mbEOSAlreadySent = false;
    AMLOG_INFO("CFFMpegDemuxer: OnRun exit.\n");
}

bool CFFMpegDemuxer::SendEOS(CFFMpegDemuxerOutput *pPin)
{
    CBuffer *pBuffer;

    AMLOG_INFO("Send EOS: Last PTS%lld.\n", mLastPts);

    if (!pPin->AllocBuffer(pBuffer))
        return false;
    pBuffer->mPTS = mLastPts;
    pBuffer->SetType(CBuffer::EOS);
    pPin->SendBuffer(pBuffer);
    return true;
}


int CFFMpegDemuxer::hasCodecParameters(AVCodecContext *enc)
{
    int val = 1;
    switch(enc->codec_type)
    {
        case CODEC_TYPE_AUDIO:
            //The frame_size may be 0 in some vorbis audio stream, but the stream can be played
            //So the following judge about CODEC_ID_VORBIS is commented out.
            if(!enc->frame_size
                  && (/*enc->codec_id == CODEC_ID_VORBIS ||*/enc->codec_id == CODEC_ID_AAC ||enc->codec_id == CODEC_ID_SPEEX))
            {
                val  = 0;
            }
            else
            {
                val = enc->sample_rate && enc->channels && enc->sample_fmt != SAMPLE_FMT_NONE;
            }

            break;

        case CODEC_TYPE_VIDEO:
            if( enc->codec_id == CODEC_ID_RV40 || enc->codec_id == CODEC_ID_RV30 ||
                enc->codec_id == CODEC_ID_RV20 || enc->codec_id == CODEC_ID_RV10)
            {
                //realvideo's pixfmt in container not valid, in order to play these videos.
                val = enc->width;
                mSeekByByte = 0;
            }else if(enc->codec_id == CODEC_ID_NONE){
                //check nothing for xxx format;
            }else{
                //check all for other codecs
                val = (enc->width) && (enc->pix_fmt != PIX_FMT_NONE);
            }
            AMLOG_DEBUG("enc->width %d, enc->pix_fmt %d.\n", enc->width, enc->pix_fmt);
            break;

        case CODEC_TYPE_SUBTITLE:
            val = 1;
            break;

        case CODEC_TYPE_DATA:
            val = 1;
            break;

        default:
            val = 1;
            break;
    }

    if(avcodec_find_decoder(enc->codec_id) == NULL || enc->codec_id == CODEC_ID_NONE)
    {
        val = 0;
    }

    return val;
}

int CFFMpegDemuxer::FindMediaStream(AVFormatContext *pFormat, int media)
{
    const char *pFileType = pFormat->iformat->name;

    //find first stream, todo
    if (CODEC_TYPE_DATA == media) {
        for (AM_UINT i = 0; i < pFormat->nb_streams; i++) {
            if (CODEC_TYPE_DATA == pFormat->streams[i]->codec->codec_type) {
                return i;
            }
        }
        return -1;
    }

    // format: TS
    if (!strncmp(pFileType, "mpegts", 6)) {
        AVProgram *pProgram = NULL, *pPlayProgram = NULL;
        for (AM_UINT i = 0; i < pFormat->nb_programs; i++) {
            pProgram = pFormat->programs[i];

            int streamType[CODEC_TYPE_NB] = { 0 };

            AVStream *pStream = NULL;
            for (AM_UINT j = 0; j < pProgram->nb_stream_indexes; j++) {
                int streamIndex = pProgram->stream_index[j];
                pStream = pFormat->streams[streamIndex];

                if (pStream->codec->codec_type == CODEC_TYPE_VIDEO) {
                    // if stream is video and there's no extradata, skip it.
                    if (!pStream->codec->extradata || (pStream->codec->extradata_size == 0)) {
                        continue;
                    }
                }
                streamType[pStream->codec->codec_type]++;
            }

            // try to select program, priority list below:
            // 1. both audio and video
            // 2. only video
            // 3. only audio
            if (streamType[CODEC_TYPE_VIDEO] > 0 && streamType[CODEC_TYPE_AUDIO] > 0) {
                pPlayProgram = pProgram;
                break;
            } else if (streamType[CODEC_TYPE_VIDEO] > 0) {
                pPlayProgram = pProgram;
            } else if (streamType[CODEC_TYPE_AUDIO] > 0) {
                pPlayProgram = pPlayProgram;
            }
        }

        // if there's no audio and video, select first
        if (!pPlayProgram) {
            pPlayProgram = pFormat->programs[0];
        }

        // obtain stream index according to requirement
        AM_INT st_best_packet_count = -1;
        AM_INT lastAudioScore = -1, curAudioScore = -1;
        AM_INT index = -1;
        for (AM_UINT i = 0; i < pPlayProgram->nb_stream_indexes; i++) {
            int streamIndex = pPlayProgram->stream_index[i];
            AVCodecContext *avctx = pFormat->streams[streamIndex]->codec;
            if (avctx->codec_type == media) {
                if (hasCodecParameters(avctx)) {
                    if (media == CODEC_TYPE_AUDIO) {
                        // Modifid for bug1836 by jywang:
                        // Priority level list for selecting audio
                        // 1. stereo        21~30
                        // 2. mono          11~20
                        // 3. 5.1           1~10
                        // 4. others        0

                        switch (pFormat->streams[streamIndex]->codec->channels) {
                            case 2: {
                                    // stereo
                                    if (pFormat->streams[streamIndex]->codec->codec_id == CODEC_ID_AC3) {
                                        curAudioScore = 30;
                                    } else {
                                        curAudioScore = 21;
                                    }
                                }
                                break;
                            case 1:     // mono
                                curAudioScore = 20;
                                break;
                            case 6:     // 5.1
                                curAudioScore = 10;
                                break;
                            default:
                                curAudioScore = 0;
                                break;
                        }

                        if(curAudioScore <= lastAudioScore) {
                            continue;
                        }
                        lastAudioScore = curAudioScore;
                        index = streamIndex;
                    } else {
                        //Modify for bug 1133/1135: choose the stream by codec_info_nb_frames. by cxing 20110714
                        if(pFormat->streams[streamIndex]->codec_info_nb_frames < st_best_packet_count) {
                            continue;
                        }
                        st_best_packet_count = pFormat->streams[streamIndex]->codec_info_nb_frames;
                        index = streamIndex;
                        AMLOG_DEBUG("** return i %d.\n", streamIndex);
                        //return streamIndex;
                    }
                }
            }
        }
        return index;
    } else {
        int st_count[CODEC_TYPE_NB]={0};
        int st_best_packet_count = -1;
        int index = -1;
        AMLOG_DEBUG(" start FindMediaStream, media %d, pFormat->nb_streams %d.\n", media, pFormat->nb_streams);
        for (AM_UINT i = 0; i < pFormat->nb_streams; i++) {
            AVCodecContext *avctx = pFormat->streams[i]->codec;
            AMLOG_DEBUG("i %d, st_count[avctx->codec_type] %d, mWantedStream[avctx->codec_type] %d, avctx->type %d.\n", i, st_count[avctx->codec_type], mWantedStream[avctx->codec_type], avctx->codec_type);
            if (avctx->codec_type == media)
            {
                if (st_count[avctx->codec_type]++ != mWantedStream[avctx->codec_type]
                    && mWantedStream[avctx->codec_type] >= 0)
                {
                    AMLOG_DEBUG("continue i %d.\n", i);
                    continue;
                }

                if (hasCodecParameters(avctx))
                {
#if !FFMPEG_VER_0_6
                    if(!strcmp(pFormat->iformat->name, "mov,mp4,m4a,3gp,3g2,mj2") || !strcmp(pFormat->iformat->name, "matroska,webm") || !strcmp(pFormat->iformat->name, "asf"))
                    {
                        //Find the default stream first. added by cx 2011-06-20
                        if((pFormat->streams[i]->disposition & AV_DISPOSITION_DEFAULT) && pFormat->streams[i]->id != CODEC_ID_TRUEHD)
                            return i;
                        if(st_best_packet_count > pFormat->streams[i]->codec_info_nb_frames)
                        {
                            AMLOG_DEBUG("continue i %d.\n", i);
                            continue;
                        }
                        st_best_packet_count = pFormat->streams[i]->codec_info_nb_frames;
                        index = i;
                    }
                    else
#endif
                        {
                        AMLOG_DEBUG("** return i %d.\n", i);
                        return i;
                    }
                }
            }
        }
        return index;
    }

    return -1;
}

AM_ERR CFFMpegDemuxer::LoadFile(const char *pFileName, void *context)
{
    AVFormatContext *pFormat = (AVFormatContext*)context;
    int divx_packed = 0;

    int rval = av_find_stream_info(pFormat);
    if (rval < 0) {
        AM_ERROR("av_find_stream_info failed\n");
        return ME_ERROR;
    }

#ifdef AM_DEBUG
    av_dump_format(pFormat, 0, pFileName, 0);
#endif

    for (int i=0; i < CODEC_TYPE_NB; i++)
        mWantedStream[i] = -1;

    // find video stream & audio stream
    if (!mpSharedRes->pbConfig.not_config_video_path) {
        mVideo = FindMediaStream(pFormat, CODEC_TYPE_VIDEO);
    }

    if (!mpSharedRes->pbConfig.not_config_audio_path) {
        mAudio = FindMediaStream(pFormat, CODEC_TYPE_AUDIO);
    }

#if PLATFORM_ANDROID
    //Use av_find_best_stream of ffmpeg0.7 to choose stream.Set audio stream as the default related stream.
    //added by cx.
    mSubtitle = FindMediaStream(pFormat, CODEC_TYPE_SUBTITLE);//av_find_best_stream(pFormat, CODEC_TYPE_SUBTITLE, -1, (mAudio != -1 ? mAudio : mVideo), NULL, 0);
#endif

    mPridata = FindMediaStream(pFormat, CODEC_TYPE_DATA);

    AMLOG_INFO("CFFMpegDemuxer: FindMediaStream video[%d]audio[%d]subtitle[%d],privatedata[%d].\n", mVideo, mAudio, mSubtitle,mPridata);

    do{
        if (mVideo == -1) {
            break;
        }

        if(3 == mpSharedRes->mDSPmode){//transcode mode
            mpSharedRes->sTranscConfig.video_w[mIndex] = pFormat->streams[mVideo]->codec->width;
            mpSharedRes->sTranscConfig.video_h[mIndex] = pFormat->streams[mVideo]->codec->height;
        }

        switch (pFormat->streams[mVideo]->codec->codec_id) {
            case CODEC_ID_MPEG1VIDEO:
            case CODEC_ID_MPEG2VIDEO:
            {
                mpSharedRes->decCurrType = mpSharedRes->decSet[SCODEC_MPEG12].dectype;
                break;
            }
            case CODEC_ID_MPEG4:
            #if FFMPEG_VER_0_6
            case CODEC_ID_XVID:
            #endif
            {
                mpSharedRes->decCurrType = mpSharedRes->decSet[SCODEC_MPEG4].dectype;
                mpSharedRes->vid_container_width = pFormat->streams[mVideo]->codec->width;//2011.09.23 has resolution info case by roy
                mpSharedRes->vid_container_height = pFormat->streams[mVideo]->codec->height;
                mpSharedRes->is_mp4s_flag = (pFormat->streams[mVideo]->codec->codec_tag == 0x5334504D)&&pFormat->streams[mVideo]->codec->width&&pFormat->streams[mVideo]->codec->height; //AV_RL32("MP4S"), width&height valid
                AMLOG_INFO("CFFMpegDemuxer: is_mp4s_flag=%d, vid_container_width=%d, vid_container_height=%d.\n", mpSharedRes->is_mp4s_flag,mpSharedRes->vid_container_width,mpSharedRes->vid_container_height);
                break;
            }
            case CODEC_ID_H264:
            {
                mpSharedRes->decCurrType = mpSharedRes->decSet[SCODEC_H264].dectype;
                break;
            }
            case CODEC_ID_RV40:
            {
                mpSharedRes->decCurrType = mpSharedRes->decSet[SCODEC_RV40].dectype;
                break;
            }
            case CODEC_ID_MSMPEG4V1:
            case CODEC_ID_MSMPEG4V2:
            case CODEC_ID_MSMPEG4V3:
            case CODEC_ID_WMV1:
            case CODEC_ID_H263P:
            case CODEC_ID_H263I:
            case CODEC_ID_FLV1:
            {
                mpSharedRes->decCurrType = mpSharedRes->decSet[SCODEC_NOT_STANDARD_MPEG4].dectype;
                break;
            }
            case CODEC_ID_WMV3:
            case CODEC_ID_VC1:
            {
                mpSharedRes->decCurrType = mpSharedRes->decSet[SCODEC_VC1].dectype;
                break;
            }
            case CODEC_ID_NONE:
                AM_INFO("LoadFile, ffmpeg not recognized stream.\n");
                return ME_BAD_FORMAT;

            default:
                AM_ASSERT(mpSharedRes->decCurrType == DEC_SOFTWARE);
                mpSharedRes->decCurrType = DEC_SOFTWARE;
                AM_INFO("Other video format, codec id = 0x%x\n", pFormat->streams[mVideo]->codec->codec_id);
                break;

        }

        AM_INFO("got mpSharedRes->decCurrType %d.\n", mpSharedRes->decCurrType);
    }while(0);


    // get sequence header and check feature
    do {
        if (mVideo == -1) {
            break;
        }

        int ret = -1;
        SeqHeader *pSeqHeader = NULL;

        ret = InitSeqHeader(pFormat->streams[mVideo]->codec->codec_id, &pSeqHeader);
        if (ret < 0) {
            mpSharedRes->dspConfig.enableDeinterlace = 0;
            AMLOG_ERROR("@@@@@@InitSeqHeader failure\n");
            break;
        }

        ret = GetSeqHeader(pFormat->streams[mVideo], pSeqHeader);

        enum SEQ_HEADER_RET seqRet = CheckSeqHeader(pSeqHeader);

        if (!strcmp(pFormat->iformat->name, "h264")) {
            pFormat->streams[mVideo]->r_frame_rate.num = pSeqHeader->framerate.num;
            pFormat->streams[mVideo]->r_frame_rate.den = pSeqHeader->framerate.den;
        }
        // check whether mw supports codec.
        // If player doens't support, exit
        // If dsp doesn't support, switch sw decoder
        if(seqRet == SEQ_HEADER_RET_PLAYER_UNSUPPORTED){
            AM_ERROR("Player not supported\n");
            mVideo = -1;
            break;
        }
        switch (seqRet) {
            case SEQ_HEADER_RET_DSP_UNSUPPORTED: {
                    // try to switch decoder
                    switch (pFormat->streams[mVideo]->codec->codec_id) {
                        case CODEC_ID_VC1:
                        case CODEC_ID_WMV3:
                        case CODEC_ID_H264:
                        case CODEC_ID_MPEG1VIDEO:
                        case CODEC_ID_MPEG2VIDEO:
                            mpSharedRes->decCurrType = DEC_SOFTWARE;
                            break;
                        case CODEC_ID_MPEG4:
                            mpSharedRes->decCurrType = DEC_HYBRID;
                            break;
                        default:
                            break;
                    }
                }
                break;
             case SEQ_HEADER_RET_HYBRID_UNSUPPORTED: {
                    switch (pFormat->streams[mVideo]->codec->codec_id) {
                        case CODEC_ID_MPEG4:
                            mpSharedRes->decCurrType = DEC_SOFTWARE;
                            break;
                        default:
                            break;
                    }
                }
                break;
            default:
                break;
        }
        AM_INFO("switch mpSharedRes->decCurrType %d.\n", mpSharedRes->decCurrType);

        // config deinterlace parameters for HW codec video
        if (mpSharedRes->bEnableDeinterlace && (mpSharedRes->decCurrType == DEC_HARDWARE)) {
            if (mpSharedRes->dspConfig.enableDeinterlace) {
                AMLOG_VERBOSE("CFFMpegDemuxer: default deinterlace is on \n");
                switch (pFormat->streams[mVideo]->codec->codec_id) {
                    case CODEC_ID_VC1:
                    case CODEC_ID_MPEG2VIDEO:
                    case CODEC_ID_MPEG4:
                    case CODEC_ID_H264:
                        {
                            mpSharedRes->dspConfig.enableDeinterlace = pSeqHeader->is_interlaced;
                            switch (pSeqHeader->interlaced_mode) {
                                case INTERLACED_MODE_FULL_MODE:
                                    mpSharedRes->dspConfig.deinterlaceConfig.deint_mode = 0;
                                    break;
                                case INTERLACED_MODE_SIMPLE_MODE:
                                    mpSharedRes->dspConfig.deinterlaceConfig.deint_mode = 1;
                                    break;
                                default:
                                    break;
                            }
                        }
                        break;
                    default:
                        mpSharedRes->dspConfig.enableDeinterlace = 0;
                        break;
                }
                AMLOG_INFO("CFFMpegDemuxer: enable_deint=%d deint_mode=%d\n",
                                mpSharedRes->dspConfig.enableDeinterlace,
                                mpSharedRes->dspConfig.deinterlaceConfig.deint_mode);
            } else {
                AMLOG_VERBOSE("CFFMpegDemuxer: default deinterlace is off\n");
            }
        } else {
            mpSharedRes->dspConfig.enableDeinterlace = 0;
            AMLOG_VERBOSE("CFFMpegDemuxer: default deinterlace is off\n");
        }

        switch (pFormat->streams[mVideo]->codec->codec_id) {
            case CODEC_ID_VC1:
                if(((VC1SeqHeader*)pSeqHeader)->broadcast && pFormat->iformat->read_adjust_fps) {
                    pFormat->iformat->read_adjust_fps(pFormat, NULL);
                }
                break;
            case CODEC_ID_MPEG4:
                divx_packed = ((MPEG4SeqHeader*)pSeqHeader)->divx_packed;
                break;
            default:
                break;
        }
        DeinitSeqHeader(&pSeqHeader);
    } while (0);

    if(mVideo>=0)
    {
        //for bug#2322, decoders which need "FB_POOL_CONFIG" should config fbp_config.lu_width with pic original width-height
        //for bug#2105, streams which need adjusting width to keep DAR should config UPDATE_VOUT_CONFIG in video renderer
        mpSharedRes->vid_width_DAR=0;
        if(1!=pFormat->streams[mVideo]->codec->sample_aspect_ratio.num&&1!=pFormat->streams[mVideo]->codec->sample_aspect_ratio.den){
            mpSharedRes->vid_width_DAR=pFormat->streams[mVideo]->codec->width*(float)pFormat->streams[mVideo]->codec->sample_aspect_ratio.num/pFormat->streams[mVideo]->codec->sample_aspect_ratio.den;
            AMLOG_INFO("FFMpeg_Demuxer::LoadFile:Codec:mVideo=%d,width=%d,height=%d, adjusted width_DAR=%d\n",mVideo,pFormat->streams[mVideo]->codec->width,pFormat->streams[mVideo]->codec->height,mpSharedRes->vid_width_DAR);
        }

        //roy mdf 2012.03.21, for rotation info
        mpSharedRes->vid_rotation_info=pFormat->streams[mVideo]->rotation_info;
    }

    // check
    if (mVideo == -1 || !mbEnableVideo) {
        mpSharedRes->videoEnabled = false;
    }

    if (mAudio == -1 || !mbEnableAudio) {
        mpSharedRes->audioEnabled = false;
    }

    if (mVideo < 0 && mAudio < 0) {
        AM_ERROR("No video, no audio\n");
        return ME_ERROR;
    }
    //give debug info if not find subtitle stream. added by cx
    if(mSubtitle < 0)
    {
        AMLOG_DEBUG("No subtitle!\n");
    }
    AM_ERR err;
    // init pins
    if (mVideo >= 0) {
        err = mpVideoOutput->InitStream(pFormat, mVideo, CODEC_TYPE_VIDEO,divx_packed);
        if (err != ME_OK) {
            AMLOG_ERROR("InitStream video stream fail, disable it.\n");
            mbEnableVideo = false;
        }
    }else{
        AM_DELETE(mpVideoOutput);
        mpVideoOutput = NULL;
    }

    if (mAudio >= 0) {
        err = mpAudioOutput->InitStream(pFormat, mAudio, CODEC_TYPE_AUDIO);
        if (err != ME_OK) {
            AMLOG_ERROR("InitStream audio stream fail, disable it.\n");
            mbEnableAudio = false;;
        }
    }else{
        AM_DELETE(mpAudioOutput);
        mpAudioOutput = NULL;
    }
    //Init subtitle output. added by cx
    if (mSubtitle>= 0) {
        err = mpSubtitleOutput->InitStream(pFormat, mSubtitle, CODEC_TYPE_SUBTITLE);
        if (err != ME_OK)
        {
            AMLOG_ERROR("InitStream subtitle stream fail, disable it.\n");
            mbEnableSubtitle= false;;
        }
    }
    else
    {
        AM_DELETE(mpSubtitleOutput);
        mpSubtitleOutput = NULL;
    }

    if (ePriDataParse_filter_parse == mpSharedRes->mPridataParsingMethod) {
        if (mPridata >= 0) {
            AMLOG_INFO("Demuxer find private data, init stream.\n");
            err = mpPridataOutput->InitStream(pFormat, mPridata, CODEC_TYPE_DATA);
            if (err != ME_OK) {
                AMLOG_ERROR("InitStream subtitle stream fail, disable it.\n");
                mbEnablePridata = false;
            }
        } else {
            AM_DELETE(mpPridataOutput);
            mpPridataOutput = NULL;
            mbEnablePridata = false;
        }
    }

    if (mVideo != -1 && pFormat->streams[mVideo]->start_time >= 0) {
        mpSharedRes->mbVideoStreamStartTimeValid = true;
        mpSharedRes->videoStreamStartPTS = pFormat->streams[mVideo]->start_time;
    }

    if (mAudio != -1 && pFormat->streams[mAudio]->start_time >= 0) {
        mpSharedRes->mbAudioStreamStartTimeValid = true;
        mpSharedRes->audioStreamStartPTS = pFormat->streams[mAudio]->start_time;
    }
    AMLOG_PTS("videoStreamStartTime=%lld audioStreamStartTime=%lld\n", mpSharedRes->videoStreamStartPTS, mpSharedRes->audioStreamStartPTS);

    mpFormat = pFormat;
    if (!mbEnableVideo && !mbEnableAudio) {
        AMLOG_ERROR("LoadFile fail(audio&video are all disabled).\n");
        return ME_ERROR;
    }

    //estimate duration if ffmpeg can not calc it.
    EstimateDuration();

    UpdatePTSConvertor();

    if ((mpSharedRes->mDSPmode == 16) &&
      (strcmp(mpFormat->iformat->name, "rtsp") == 0) && (mpSharedRes->decCurrType == DEC_HARDWARE)){
        mbNVRRTSP = true;
        Run();
        AMLOG_INFO("CFFMpegDemuxer[%d] rtsp stream, Run.\n", mIndex);
    }

    return ME_OK;
}

bool CFFMpegDemuxer::isSupportedByDuplex()
{
    //TODO
    if(mVideo != -1){
        if(mpSharedRes->decCurrType != DEC_HARDWARE){
            return false;
        }
        if(mpFormat->streams[mVideo]->codec->codec_id != CODEC_ID_H264) {
            return false;
        }
    }
    return true;
}

void CFFMpegDemuxer::GetVideoSize(AM_INT *pWidth, AM_INT *pHeight){
    AM_ASSERT(mWidth && mHeight);
    *pWidth = mWidth;
    *pHeight = mHeight;
}

void CFFMpegDemuxer::UpdatePTSConvertor()
{
    AM_ASSERT(mpSharedRes);
    AM_ASSERT(mpSharedRes->clockTimebaseNum == 1);
    AM_ASSERT(mpSharedRes->clockTimebaseDen == TICK_PER_SECOND);

    AMLOG_INFO("..mpFormat %p, mVideo %d, mAudio %d.\n", mpFormat, mVideo, mAudio);
    AMLOG_INFO(" [PTS] FFDemuxer: UpdatePTSConvertor system num %d, den %d.\n", mpSharedRes->clockTimebaseNum, mpSharedRes->clockTimebaseDen);
    AM_U64 nPTSNum = 0;
    AM_U64 nPTSDen = 0;
    if (mVideo >= 0) {
        AM_ASSERT(mpFormat->streams[mVideo]->time_base.num ==1);
        AMLOG_INFO(" [PTS] FFDemuxer: UpdatePTSConvertor video(%d) stream num %d, den %d.\n", mVideo, mpFormat->streams[mVideo]->time_base.num, mpFormat->streams[mVideo]->time_base.den);
        nPTSNum = (AM_U64)(mpFormat->streams[mVideo]->time_base.num) * mpSharedRes->clockTimebaseDen;
        nPTSDen = (AM_U64)(mpFormat->streams[mVideo]->time_base.den) * mpSharedRes->clockTimebaseNum;
        if ((nPTSNum % nPTSDen) == 0) {
            nPTSNum /= nPTSDen;
            nPTSDen = 1;
        } else if (nPTSNum > TICK_PER_SECOND) {
            nPTSNum += nPTSDen/2;
            nPTSNum /= nPTSDen;
            nPTSDen = 1;
        }
        mPTSVideo_Num = nPTSNum;
        mPTSVideo_Den = nPTSDen;
        if(0==mpFormat->streams[mVideo]->r_frame_rate.num)
        {
            AMLOG_ERROR("UpdatePTSConvertor, r_frame_rate.num==0, so cannot calculate mpSharedRes->mVideoTicks.\n");
        }
        else
        {
            mpSharedRes->mVideoTicks = (mpSharedRes->clockTimebaseDen * (AM_U64)(mpFormat->streams[mVideo]->r_frame_rate.den)) /(mpSharedRes->clockTimebaseNum *(AM_U64)(mpFormat->streams[mVideo]->r_frame_rate.num));
            AMLOG_INFO("    video convertor num %u, den %u, frame ticks %u.\n", mPTSVideo_Num, mPTSVideo_Den, mpSharedRes->mVideoTicks);
        }
    }
    if (mAudio >= 0) {
        AM_ASSERT(mpFormat->streams[mAudio]->time_base.num ==1);
        AMLOG_INFO(" [PTS] FFDemuxer: UpdatePTSConvertor mAudio(%d) stream num %d, den %d.\n", mAudio, mpFormat->streams[mAudio]->time_base.num, mpFormat->streams[mAudio]->time_base.den);
        nPTSNum = (AM_U64)(mpFormat->streams[mAudio]->time_base.num) * mpSharedRes->clockTimebaseDen;
        nPTSDen = (AM_U64)(mpFormat->streams[mAudio]->time_base.den) * mpSharedRes->clockTimebaseNum;
        if ((nPTSNum % nPTSDen) == 0) {
            nPTSNum /= nPTSDen;
            nPTSDen = 1;
        }
        mPTSAudio_Num = nPTSNum;
        mPTSAudio_Den = nPTSDen;
        //mpSharedRes->mAudioTicks = (mpSharedRes->clockTimebaseDen * (AM_U64)(mpFormat->streams[mAudio]->r_frame_rate.den))/(mpSharedRes->clockTimebaseNum * (AM_U64)mpFormat->streams[mAudio]->r_frame_rate.num));
        AMLOG_INFO("    audio convertor num %u, den %u, audio ticks.\n", mPTSAudio_Num, mPTSAudio_Den/*, mpSharedRes->mAudioTicks*/);
    }

}

AM_UINT CFFMpegDemuxer::CheckDecType(SCODECID decID)
{
    return mpSharedRes->decSet[decID].dectype;
}

void CFFMpegDemuxer::ConvertVideoPts(am_pts_t& pts)
{
    pts *= mPTSVideo_Num;
    if (mPTSVideo_Den != 1) {
        pts /=  mPTSVideo_Den;
    }
}

void CFFMpegDemuxer::ConvertAudioPts(am_pts_t& pts)
{
    pts *= mPTSAudio_Num;
    if (mPTSAudio_Den != 1) {
        pts /=  mPTSAudio_Den;
    }
}

/*A5S DV Playback Test */
AM_ERR  CFFMpegDemuxer::A5SPlayMode(AM_UINT mode)
{
    CMD cmd;
    cmd.code = CMD_A5S_PLAYMODE;
    cmd.res32_1 = mode;
    mpWorkQ->MsgQ()->SendMsg(&cmd, sizeof(cmd));
    return ME_OK;
}
AM_ERR  CFFMpegDemuxer::DoA5SPlayMode(AM_UINT mode)
{
    const char* pFileType = mpFormat->iformat->name;
    if (strncmp(pFileType, "a5dv mp4", 8) != 0)
    {
        AM_INFO("");
        return ME_NOT_SUPPORTED;
    }
    SMP4Context* sc = (SMP4Context* )mpFormat->priv_data;
    sc->play_mode = (PlayMode)mode;
    sc->mode_change = 1;
    return ME_OK;
}

AM_ERR  CFFMpegDemuxer::A5SPlayNM(AM_INT start_n, AM_INT end_m)
{
    CMD cmd;
    cmd.code = CMD_A5S_PLAYNM;
    cmd.res32_1 = start_n;
    cmd.res64_1 = end_m;
    mpWorkQ->MsgQ()->SendMsg(&cmd, sizeof(cmd));
    return ME_OK;
}
AM_ERR  CFFMpegDemuxer::DoA5SPlayNM(AM_INT start_n, AM_INT end_m)
{
    const char* pFileType = mpFormat->iformat->name;
    if (strncmp(pFileType, "a5dv mp4", 8) != 0)
    {
        AM_INFO("");
        return ME_NOT_SUPPORTED;
    }
    SMP4Context* sc = (SMP4Context* )mpFormat->priv_data;
    sc->play_mode = PLAY_N_TO_M;
    sc->mode_change = 1;
    sc->start_n = start_n;
    sc->end_m = end_m;
    return ME_OK;
}
/*end A5S DV Test*/


#if 1
AM_ERR CFFMpegDemuxer::Seek(AM_U64 &ms)
{
    AM_ASSERT(AV_TIME_BASE == 1000000);
    AM_S64 seek_target = ms * 1000;
    AM_INT ret = 0;
    //duration estimated, seek by bytes
    if( mDurationEstimated > 0){
        if(ms >= mDurationEst){
            AMLOG_ERROR("Demux::Seek --  time bigger than length, no seek. ms=%lld,length=%lld.\n", ms,mDurationEst);
            return ME_ERROR;
        }
        seek_target = mpFormat->file_size * ms /mDurationEst;
        AMLOG_INFO("CFFMpegDemuxer::Seek() -- This File can only seek by byte,seek_target=%lld. file_size = %lld\n",seek_target,mpFormat->file_size);
        if ((ret = avformat_seek_file( mpFormat, -1,INT64_MIN, seek_target,INT64_MAX, AVSEEK_FLAG_BYTE))<0){
            AMLOG_ERROR("seek by bytes fail, ret =%d, Seek return error.\n", ret);
            return ME_ERROR;
        }
        //correct current position
        AM_ASSERT(mpFormat->pb);
        ms = mDurationEst * mpFormat->pb->pos/ mpFormat->file_size;
        AMLOG_INFO("CFFMpegDemuxer::Seek() -- data_offset = %lld,ms = %lld\n",mpFormat->pb->pos,ms);
        return ME_OK;
    }
    if(mSeekByByte != 1){
        //seek by time
        if (mpFormat->start_time != AV_NOPTS_VALUE)
            seek_target += mpFormat->start_time;

        //added by liujian, to fix bug#1911, process the case:
        //    both audio and video data exists, but video's duration is less than audio' duration.
        //when seeking to the file part that only audio exists,
        //    avformat_seek_file() param 'stream_index' should be set 'audio_stream_index',
        //and only audio data sent to decoder, notify renders that no video data.
        int stream_index = -1;
        if(mbEnableVideoBakFlag){
            mbEnableVideo = mbEnableVideoBak;
            mpSharedRes->videoEnabled = mbEnableVideoBak;
        }
        if(mbEnableVideo){
            if((mVideo != -1) && (mAudio != -1) && (mpFormat->streams[mVideo]->duration > 0) && (mpFormat->streams[mAudio]->duration > 0)){
                AM_S64 video_duration = mpFormat->streams[mVideo]->duration * mpFormat->streams[mVideo]->time_base.num/mpFormat->streams[mVideo]->time_base.den * AV_TIME_BASE;
                AM_S64 audio_duration = mpFormat->streams[mAudio]->duration * mpFormat->streams[mAudio]->time_base.num/mpFormat->streams[mAudio]->time_base.den * AV_TIME_BASE;
                if((seek_target > video_duration) && (seek_target <= audio_duration) ){
                    stream_index = mAudio;
                    seek_target = av_rescale(seek_target, mpFormat->streams[mAudio]->time_base.den, AV_TIME_BASE * (int64_t)mpFormat->streams[mAudio]->time_base.num);
                    mbEnableVideoBakFlag = true;
                    mbEnableVideoBak = mbEnableVideo;
                    mbEnableVideo = false;
                    mpSharedRes->videoEnabled = false;
                }else{
                    mbEnableVideoBakFlag = false;
                }
            }
        }

        AMLOG_INFO("FFmpeg_demuxer, Seek, seek_target = %lld.\n",seek_target);
        if ((ret = avformat_seek_file(mpFormat, stream_index,INT64_MIN, seek_target,INT64_MAX, 0)) < 0) {
            AMLOG_ERROR("seek by time fail, ret =%d, try by bytes.\n", ret);
            mpSharedRes->mbSeekFailed = 1;
            //try seek by byte
            seek_target =seek_target *mpFormat->file_size/mpFormat->duration;
            if ((ret = avformat_seek_file( mpFormat, -1,INT64_MIN, seek_target,INT64_MAX, AVSEEK_FLAG_BYTE))<0){
                AMLOG_ERROR("seek by bytes fail, ret =%d, Seek return error.\n", ret);
                return ME_ERROR;
            }
        }
    }else{
        seek_target = seek_target *mpFormat->file_size/mpFormat->duration;
        AMLOG_INFO("This File can only seek by byte,seek_target=%lld.\n",seek_target);
        if ((ret = avformat_seek_file( mpFormat, -1,INT64_MIN, seek_target,INT64_MAX, AVSEEK_FLAG_BYTE))<0){
            AMLOG_ERROR("seek by bytes fail, ret =%d, Seek return error.\n", ret);
            return ME_ERROR;
        }
    }
    return ME_OK;
}
#else
AM_ERR CFFMpegDemuxer::Seek(AM_U64 ms)
{
    int stream_index,seek_flags;
    seek_flags  = AVSEEK_FLAG_BACKWARD;//0;
    stream_index = -1;
#if 1
    // Audio first
    if (mAudio >= 0)
        stream_index = mAudio;
    else if (mVideo >= 0)
        stream_index = mVideo;
#else
    stream_index = mVideo;
#endif
    AM_U64 seek_target = (ms * AV_TIME_BASE) / 1000;
    seek_target= av_rescale_q(seek_target, AV_TIME_BASE_Q, mpFormat->streams[stream_index]->time_base);

    if (av_seek_frame(mpFormat, stream_index, seek_target, seek_flags) < 0) {
        AM_ERROR("error while seeking\n");
        return ME_ERROR;
    }
    return ME_OK;
}
#endif

AM_ERR CFFMpegDemuxer::EstimateDuration()
{
    //do not estimate if 'rtsp' used.
    if(strcmp(mpFormat->iformat->name, "rtsp") == 0){
        return ME_OK;
    }
    //ffmpeg has calc duration OK
    if(mpFormat->duration >= 0){
        return ME_OK;
    }
    //estimate duration,  duration = file_size/(bit_rate0+bitrate1+...)
    //     suppose that pStream->codec->bit_rate is filled corret
    AM_S64 file_size = mpFormat->file_size;
    AM_S32 bit_rate = 0;
    AM_UINT i;
    for ( i = 0; i < mpFormat->nb_streams; i++) {
        AVStream *pStream = mpFormat->streams[i];
        if((!pStream)||(!pStream->codec))
            break;
        if((pStream->codec->codec_type == CODEC_TYPE_VIDEO) || pStream->codec->codec_type == CODEC_TYPE_AUDIO){
            bit_rate += pStream->codec->bit_rate;
        }
    }
    if((i == mpFormat->nb_streams) && (bit_rate > 8)){
        bit_rate >>=3;//convert to Bps
        mDurationEst =( file_size /bit_rate) * 1000;
        mDurationEstimated = 1; //duration estimated, and can only seek by bytes
        mpSharedRes->mbDurationEstimated = AM_TRUE;
        return ME_OK;
    }
    mDurationEstimated = -1;
    return ME_ERROR;
}

AM_ERR CFFMpegDemuxer::GetTotalLength(AM_U64& ms)
{
    if(!mpFormat)
        return ME_ERROR;
    if(mpFormat->duration < 0){
        if(mDurationEstimated > 0){
            ms = mDurationEst;
            return ME_OK;
        }
        mpFormat->duration = 0; //can not be estimated
    }
    ms = (mpFormat->duration * 1000) / AV_TIME_BASE;
    return ME_OK;
}

AM_ERR CFFMpegDemuxer::GetFileType(const char *&pFileType)
{
    if(!mpFormat || !mpFormat->iformat) {
        return ME_ERROR;
    }
    pFileType = mpFormat->iformat->name;
    return ME_OK;
}

void CFFMpegDemuxer::EnableAudio(bool enable) {
    mbEnableAudio = enable;
    mpSharedRes->audioEnabled = enable;
}

void CFFMpegDemuxer::EnableVideo(bool enable) {
    mbEnableVideo = enable;
    mpSharedRes->videoEnabled = enable;
}

void CFFMpegDemuxer::EnableSubtitle(bool enable) {
    mbEnableSubtitle = enable;
}

void CFFMpegDemuxer::freeAllPrivateData()
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

void CFFMpegDemuxer::postPrivateDataMsg(AM_U32 size, AM_U16 type, AM_U16 sub_type)
{
    AM_MSG msg;
    msg.code = IEngine::MSG_PRIDATA_GENERATED;
    msg.p0 = size;
    msg.p2 = type;
    msg.p3 = sub_type;
    PostEngineMsg(msg);
    __atomic_inc(&mpSharedRes->mnPrivateDataCountNeedSent);
}

void CFFMpegDemuxer::storePrivateData(AM_U8* pdata, AM_UINT max_len)
{
    CDoubleLinkedList::SNode* pnode;
    SPriDataHeader* data_header = (SPriDataHeader*)pdata;
    SPriDataHeader* cur_header;
    SPriDataPacket* packet;
    AM_U8* ptmp = NULL;
    AM_UINT finded = 0;
    AM_UINT need_post_msg = 0;

    AMLOG_DEBUG("CFFMpegDemuxer::storePrivateData start.\n");

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
            //AM_ERROR("BAD Magic number for pridata, must have errors, %c, %c, %c, %c.\n", pdata[0], pdata[1], pdata[2], pdata[3]);
            return;
        }
        if ((data_header->data_length + sizeof(SPriDataHeader)) > max_len) {
            AM_ERROR("BAD data len for pridata(%d, data left %d), must have errors.\n", data_header->data_length, max_len);
            return;
        }

        finded = 0;

        //not push data too fast to app
        if (mpSharedRes->mnPrivateDataCountNeedSent < 2) {
            need_post_msg = 1;
        } else {
            AMLOG_WARN("app process pridata too slow? not send data this time, mnPrivateDataCountNeedSent %u.\n", mpSharedRes->mnPrivateDataCountNeedSent);
        }

        pnode = mPridataList.FirstNode();
        while (pnode) {
            packet = (SPriDataPacket*) pnode->p_context;
            AM_ASSERT(packet);
            cur_header = (SPriDataHeader*) packet->pdata;
            AM_ASSERT(cur_header);

            AMLOG_VERBOSE("finding type %d, sub type %d.\n", data_header->type, data_header->subtype);
            AMLOG_VERBOSE("current type %d, sub type %d, packet type %d, sub_type %d, size %d.\n", cur_header->type, cur_header->subtype, packet->data_type, packet->sub_type, packet->data_len);

            //find matched data slot
            if ((cur_header->type == data_header->type) && (cur_header->subtype == data_header->subtype)) {
                if ((data_header->data_length + sizeof(SPriDataHeader)) > packet->total_length) {
                    AMLOG_INFO("CFFMpegDemuxer::storePrivateData, need re-alloc memory, type %d, subtype %d, len %d.\n", data_header->type, data_header->subtype, data_header->data_length);
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

                if (need_post_msg) {
                    postPrivateDataMsg(packet->data_len, packet->data_type, packet->sub_type);
                }
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

            mPridataList.InsertContent(NULL, (void *)packet, 0);
            if (need_post_msg) {
                postPrivateDataMsg(packet->data_len, packet->data_type, packet->sub_type);
            }
            AMLOG_INFO("CFFMpegDemuxer::storePrivateData, new data slot, type %d, subtype %d, len %d.\n", packet->data_type, packet->sub_type, packet->data_len);
        }

        pdata += data_header->data_length + sizeof(SPriDataHeader);
        max_len -= data_header->data_length + sizeof(SPriDataHeader);

    }

    AMLOG_DEBUG("CFFMpegDemuxer::storePrivateData done.\n");
}

AM_ERR CFFMpegDemuxer::RetieveData(AM_U8* pdata, AM_UINT max_len, IParameters::DataCategory data_category)
{
    CDoubleLinkedList::SNode* pnode;
    AM_U8* p_cur = pdata;
    SPriDataPacket* packet;
    AM_UINT len_cur = 0;

    AM_ASSERT(IParameters::DataCategory_PrivateData == data_category);
    if (IParameters::DataCategory_PrivateData == data_category) {

        AMLOG_DEBUG("CFFMpegDemuxer::RetieveData start.\n");

        pnode = mPridataList.FirstNode();
        while (pnode) {
            packet = (SPriDataPacket*) pnode->p_context;
            AM_ASSERT(packet);

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

        AMLOG_DEBUG("CFFMpegDemuxer::RetieveData done, length %u.\n", len_cur);

    } else {
        //need implement
        AM_ERROR("NOT SUPPORTED.\n");
        return ME_ERROR;
    }
    return ME_OK;
}

AM_ERR CFFMpegDemuxer::RetieveDataByType(AM_U8* pdata, AM_UINT max_len, AM_U16 type, AM_U16 sub_type, IParameters::DataCategory data_category)
{
    CDoubleLinkedList::SNode* pnode;
    SPriDataPacket* packet;

    AM_ASSERT(IParameters::DataCategory_PrivateData == data_category);
    if (IParameters::DataCategory_PrivateData == data_category) {

        AMLOG_DEBUG("CFFMpegDemuxer::RetieveDataByType start.\n");

        pnode = mPridataList.FirstNode();
        while (pnode) {
            packet = (SPriDataPacket*) pnode->p_context;
            AM_ASSERT(packet);

            if (packet->data_type == type && packet->sub_type == sub_type) {
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

        AMLOG_DEBUG("CFFMpegDemuxer::RetieveDataByType done.\n");

    } else {
        //need implement
        AM_ERROR("NOT SUPPORTED.\n");
        return ME_ERROR;
    }
    AM_ERROR("NOT FOUND data tye %d sub type %d.\n", type, sub_type);
    return ME_ERROR;
}


#ifdef AM_DEBUG
void CFFMpegDemuxer::PrintState()
{
    if (mbEnableVideo && mVideo >= 0) {
        AM_ASSERT(mpVideoBP);
        AMLOG_INFO("FFDemuxer(video packet): msState=%d, %d free video buffers.\n", msState, mpVideoBP->GetFreeBufferCnt());
    }
    if (mbEnableAudio && mAudio >= 0) {
        AM_ASSERT(mpAudioBP);
        AMLOG_INFO("FFDemuxer(audio packet): msState=%d, %d free audio buffers.\n", msState, mpAudioBP->GetFreeBufferCnt());
    }
}

void CFFMpegDemuxer::DumpEs(unsigned char *buf, int len, int stream_index)
{
    if (stream_index == mVideo && mbEnableVideo) {
        ++log_video_frame_index;
        DumpStreamEs(buf,len,"video",log_video_frame_index);
    } else if (stream_index == mAudio && mbEnableAudio) {
        ++log_audio_frame_index;
        DumpStreamEs(buf,len,"audio",log_audio_frame_index);
    } else if (mPkt.stream_index == mSubtitle && mbEnableSubtitle){
        ++log_subtitle_index;
        DumpStreamEs(buf,len,"subtitle",log_subtitle_index);
    }else{
        //do nothing
    }
}
void CFFMpegDemuxer::DumpStreamEs(unsigned char *buf, int len, const char *name,int index)
{
    if (mLogOutput & LogDumpTotalBinary) {
        char filename[64];
        snprintf(filename, sizeof(filename), "%s/demux_%s.dat", AM_GetPath(AM_PathDump), name);

        FILE *mpDumpFile = fopen(filename, "ab");
        if (mpDumpFile) {
            fwrite(buf, 1, len, mpDumpFile);
            fclose(mpDumpFile);
            mpDumpFile = NULL;
        } else {
            //AMLOG_INFO("open  mpDumpFile fail.\n");
        }
    }

    if (mLogOutput & LogDumpSeparateBinary) {
        if(index<log_start_frame || index >log_end_frame){
            return;
        }
        char filename[64];
        snprintf(filename, sizeof(filename), "%s/demux_%s_frame%05d.dat", AM_GetPath(AM_PathDump), name,index);

        FILE *mpDumpFile = fopen(filename, "wb");
        if (mpDumpFile) {
            fwrite(buf, 1, len, mpDumpFile);
            fclose(mpDumpFile);
        }
    }
}

#endif

#if 1
int CFFMpegDemuxer::httpThreadCallBackEx(void *pCtx, const HTTPThreadParams *pParams)
{
    CFFMpegDemuxer *pThis = (CFFMpegDemuxer*)pCtx;
    return pThis->httpThreadProcess(pParams);
}

int CFFMpegDemuxer::httpThreadProcess(const HTTPThreadParams *pParams)
{
    switch (pParams->msg) {
        case HTTP_THREAD_MSG_START_BUFFERING:
            mHttpThreadCurPos = pParams->curPos;
            mHttpThreadCacheSize = pParams->cacheSize;
            mHttpThreadDataLen = pParams->dataLen;
            mpWorkQ->PostMsg(CMD_HTTP_THREAD_ENTER_BUFFERING_MODE);
            break;
        case HTTP_THREAD_MSG_END_BUFFERING:
            mHttpThreadCurPos = pParams->curPos;
            mHttpThreadCacheSize = pParams->cacheSize;
            mHttpThreadDataLen = pParams->dataLen;
            mpWorkQ->PostMsg(CMD_HTTP_THREAD_EXIT_BUFFERING_MODE);
            break;
        case HTTP_THREAD_MSG_UPDATE_BUFFER:
            mHttpThreadCurPos = pParams->curPos;
            mHttpThreadCacheSize = pParams->cacheSize;
            mHttpThreadDataLen = pParams->dataLen;
            mpWorkQ->PostMsg(CMD_HTTP_THREAD_UPDATE_BUFFER);
            break;
        case HTTP_THREAD_MSG_ERROR_FOUND:
            mpWorkQ->PostMsg(CMD_HTTP_THREAD_ERROR_FOUND);
            break;
        case HTTP_THREAD_MSG_ERROR_FIXED:
            mpWorkQ->PostMsg(CMD_HTTP_THREAD_ERROR_FIXED);
            break;
        case HTTP_THREAD_MSG_ERROR_UNFIXABLE:
            mpWorkQ->PostMsg(CMD_HTTP_THREAD_ERROR_UNFIXABLE);
            break;
        default:
            av_log(NULL, AV_LOG_ERROR, "!!!CFFMpegDemuxer::httpThreadProcess: un-handled msg=%d\n", pParams->msg);
            break;
    }
    return 0;
}
#else
int CFFMpegDemuxer::httpThreadCallBack(size_t buffersize, size_t datasize, void *context){
/*    if(context != 1){
        AMLOG_ERROR("httpThreadCallBack error: context = %p!\n", context, 1);
        return -1;
    }*/
    mhttpBufferSize = buffersize;
    mhttpDataSize = datasize;

    return 0;
}
#endif
//-----------------------------------------------------------------------
//
// CFFMpegDemuxerOutput
//
//-----------------------------------------------------------------------
CFFMpegDemuxerOutput* CFFMpegDemuxerOutput::Create(CFilter *pFilter)
{
    CFFMpegDemuxerOutput *result = new CFFMpegDemuxerOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

CFFMpegDemuxerOutput::CFFMpegDemuxerOutput(CFilter *pFilter):
    inherited(pFilter)
{
    mpSharedRes = ((CActiveFilter*)pFilter)->mpSharedRes;
}

AM_ERR CFFMpegDemuxerOutput::Construct()
{
        DSetModuleLogConfig(LogModuleFFMpegDemuxer);
//	AM_ERR err = inherited::Construct();
//	if (err != ME_OK)
//		return err;
	return ME_OK;
}

CFFMpegDemuxerOutput::~CFFMpegDemuxerOutput()
{
    AMLOG_DESTRUCTOR("~CFFMpegDemuxerOutput.\n");
}

AM_ERR CFFMpegDemuxerOutput::InitStream(AVFormatContext *pFormat, int stream, int media,int divx_packed)
{
    //AVCodecContext *pCodec = pFormat->streams[stream]->codec;
    AVStream *pStream = pFormat->streams[stream];
    AVCodecContext *pCodec = pStream->codec;
    AM_ASSERT(mpSharedRes);
    if (media == CODEC_TYPE_VIDEO) {
        AMLOG_INFO("FFDemuxer get video stream, pCodec->codec_id=%x, %p.\n", (AM_INT)pCodec->codec_id, &mMediaFormat);
        mMediaFormat.preferWorkMode = PreferWorkMode_UDEC;//default is UDEC mode
        switch (pCodec->codec_id) {
            case CODEC_ID_MPEG1VIDEO:
            {
                if(mpSharedRes->decCurrType == DEC_HARDWARE)
                {
                    mMediaFormat.pSubType = &GUID_AmbaVideoDecoder;
                    break;
                }
                else
                {
                    mMediaFormat.pSubType = &GUID_Video_MPEG12;
                    break;
                }
            }
            case CODEC_ID_MPEG2VIDEO:
            {
                if(mpSharedRes->decCurrType == DEC_HARDWARE)
                {
                    mMediaFormat.pSubType = &GUID_AmbaVideoDecoder;
                    break;
                }
                else
                {
                    mMediaFormat.pSubType = &GUID_Video_MPEG12;
                    break;
                }
            }
            case CODEC_ID_MPEG4:
            #if FFMPEG_VER_0_6
            case CODEC_ID_XVID:
            #endif
            {
                if(mpSharedRes->decCurrType == DEC_HARDWARE)
                {
                    mMediaFormat.pSubType = &GUID_AmbaVideoDecoder;
                    if(!strcmp(pFormat->iformat->name, "avi"))
                    {
                        mpSharedRes->is_avi_flag = 1;
                        AMLOG_INFO("FFDemuxer, for MPEG4 in AVI container, maybe has P skipped issue.\n");
                    }
                    else if(divx_packed){
                        //modified to fix bug#1405
                        mpSharedRes->is_avi_flag = 1;
                        AMLOG_INFO("FFDemuxer, DIVX_PACKED has P skipped issue.\n");
                    }
                    break;
                }
                else
                {
                    mMediaFormat.pSubType = &GUID_Video_MPEG4;
                    break;
                }
            }
            case CODEC_ID_H264:
            {
                if(mpSharedRes->decCurrType == DEC_HARDWARE)
                {
                    mMediaFormat.pSubType = &GUID_AmbaVideoDecoder;
                    if (DSPMode_DuplexLowdelay == mpSharedRes->encoding_mode_config.dsp_mode) {
                        AMLOG_WARN("[dsp mode]: (demuxer), engine request Duplex mode! try playback H264 in duplex mode.\n");
                        mMediaFormat.preferWorkMode = PreferWorkMode_Duplex;
                    }
                    break;
                }
                else
                {
                    mMediaFormat.pSubType = &GUID_Video_H264;
                    break;
                }
            }
            case CODEC_ID_RV10:             mMediaFormat.pSubType = &GUID_Video_RV10; break;
            case CODEC_ID_RV20:             mMediaFormat.pSubType = &GUID_Video_RV20; break;
            case CODEC_ID_RV30:             mMediaFormat.pSubType = &GUID_Video_RV30; break;
            case CODEC_ID_RV40:
            {
                mMediaFormat.pSubType = &GUID_Video_RV40;
                break;
            }
            case CODEC_ID_MSMPEG4V1:
            {
                mMediaFormat.pSubType = &GUID_Video_MSMPEG4_V1;
                break;
            }
            case CODEC_ID_MSMPEG4V2:
            {
                mMediaFormat.pSubType = &GUID_Video_MSMPEG4_V2;
                break;
            }
            case CODEC_ID_MSMPEG4V3:
            {
                mMediaFormat.pSubType = &GUID_Video_MSMPEG4_V3;
                break;
            }
            case CODEC_ID_WMV1:
            {
                mMediaFormat.pSubType = &GUID_Video_WMV1;
                break;
            }
            case CODEC_ID_WMV2:           mMediaFormat.pSubType = &GUID_Video_WMV2; break;
            case CODEC_ID_WMV3:
            {
                if(mpSharedRes->decCurrType == DEC_HARDWARE)
                {
                    mMediaFormat.pSubType = &GUID_AmbaVideoDecoder;
                    break;
                }
                else
                {
                    mMediaFormat.pSubType = &GUID_Video_WMV3;
                    break;
                }
            }
            case CODEC_ID_H263P:
            {
                mMediaFormat.pSubType = &GUID_Video_H263P;
                break;
            }
            case CODEC_ID_H263I:
            {
                mMediaFormat.pSubType = &GUID_Video_H263I;
                break;
            }
            case CODEC_ID_FLV1:
            {
                mMediaFormat.pSubType = &GUID_Video_FLV1;
                break;
            }
            case CODEC_ID_VC1:
            {
                if(mpSharedRes->decCurrType == DEC_HARDWARE)
                {
                    mMediaFormat.pSubType = &GUID_AmbaVideoDecoder;
                    break;
                }
                else
                {
                    mMediaFormat.pSubType = &GUID_Video_VC1;
                    break;
                }
            }
            case CODEC_ID_VP8:              mMediaFormat.pSubType = &GUID_Video_VP8; break;

            case CODEC_ID_NONE:
                mMediaFormat.pMediaType = &GUID_None;
                AM_INFO("ffmpeg not recognized stream.\n");
                return ME_BAD_FORMAT;

            default:
                mMediaFormat.pSubType = &GUID_Video_FF_OTHER_CODECS;
                break;

        }
        mMediaFormat.pMediaType = &GUID_Video;
        mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Stream;
        //mMediaFormat.format = (AM_INTPTR)pCodec;
        mMediaFormat.format = (AM_INTPTR)pStream;

        //todo, calculate udec nums here, not very good here
        AM_INFO("mpFilter->mpSharedRes->decCurrType %d.\n", mpSharedRes->decCurrType);
        if(mMediaFormat.pSubType == &GUID_AmbaVideoDecoder) mpSharedRes->udecNums++;

        return ME_OK;
    } else if (media == CODEC_TYPE_AUDIO) {
        AMLOG_INFO("FFDemuxer get audio stream, pCodec->codec_id=%x, %p.\n", (AM_INT)pCodec->codec_id, &mMediaFormat);
        switch (pCodec->codec_id) {
            case CODEC_ID_MP2:              mMediaFormat.pSubType = &GUID_Audio_MP2; break;
#if PLATFORM_ANDROID
         case CODEC_ID_MP3PV:
#endif
            case CODEC_ID_MP3:              mMediaFormat.pSubType = &GUID_Audio_MP3; break;
            case CODEC_ID_AAC:              mMediaFormat.pSubType = &GUID_Audio_AAC; break;
            case CODEC_ID_AC3:              mMediaFormat.pSubType = &GUID_Audio_AC3; break;
            case CODEC_ID_DTS:              mMediaFormat.pSubType = &GUID_Audio_DTS; break;

            case CODEC_ID_WMAV1:        mMediaFormat.pSubType = &GUID_Audio_WMAV1; break;
            case CODEC_ID_WMAV2:        mMediaFormat.pSubType = &GUID_Audio_WMAV2; break;

            case CODEC_ID_COOK:          mMediaFormat.pSubType = &GUID_Audio_COOK; break;
            case CODEC_ID_VORBIS:       mMediaFormat.pSubType = &GUID_Audio_VORBIS; break;

            case CODEC_ID_PCM_S16LE:
            //case CODEC_ID_PCM_S16BE:
            case CODEC_ID_PCM_U16LE:
            //case CODEC_ID_PCM_U16BE:
            //case CODEC_ID_PCM_S8:
            //case CODEC_ID_PCM_U8:
            //case CODEC_ID_PCM_MULAW:
            //case CODEC_ID_PCM_ALAW:
            case CODEC_ID_PCM_S32LE:
            //case CODEC_ID_PCM_S32BE:
            case CODEC_ID_PCM_U32LE:
            //case CODEC_ID_PCM_U32BE:
            case CODEC_ID_PCM_S24LE:
            //case CODEC_ID_PCM_S24BE:
            case CODEC_ID_PCM_U24LE:
            //case CODEC_ID_PCM_U24BE:
            //case CODEC_ID_PCM_S24DAUD:
            //case CODEC_ID_PCM_ZORK:
            //case CODEC_ID_PCM_S16LE_PLANAR:
            //case CODEC_ID_PCM_DVD:
            //case CODEC_ID_PCM_F32BE:
            //case CODEC_ID_PCM_F32LE:
            //case CODEC_ID_PCM_F64BE:
            //case CODEC_ID_PCM_F64LE:
            //case CODEC_ID_PCM_BLURAY:
                AM_INFO("will use amba_audio_decoder.\n");
                mMediaFormat.pSubType = &GUID_Audio_PCM;
                mMediaFormat.reserved1 = (AM_UINT)pCodec->codec_id;
                break;

            case CODEC_ID_NONE:
                mMediaFormat.pMediaType = &GUID_None;
                AM_INFO("ffmpeg not recognized stream.\n");
                return ME_BAD_FORMAT;

            default:
                mMediaFormat.pSubType = &GUID_Audio_FF_OTHER_CODECS;
                AM_INFO("Other audio format, codec id = 0x%x\n", pCodec->codec_id);
                break;
        }

        mMediaFormat.pMediaType = &GUID_Audio;
        mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Stream;
        //mMediaFormat.format = (AM_INTPTR)pCodec;
        mMediaFormat.format = (AM_INTPTR)pStream;

        return ME_OK;
    }else if(media == CODEC_TYPE_SUBTITLE) {
        switch (pCodec->codec_id) {
            case CODEC_ID_DVD_SUBTITLE:
                mMediaFormat.pSubType = &GUID_Subtitle_DVD;
                break;
            case CODEC_ID_DVB_SUBTITLE:
                mMediaFormat.pSubType = &GUID_Subtitle_DVB;
                break;
            case CODEC_ID_TEXT://< raw UTF-8 text
                mMediaFormat.pSubType = &GUID_Subtitle_TEXT;
                break;
            case CODEC_ID_XSUB:
                mMediaFormat.pSubType = &GUID_Subtitle_XSUB;
                break;
            case CODEC_ID_SSA:
                mMediaFormat.pSubType = &GUID_Subtitle_SSA;
                AMLOG_INFO("-----found SSA type subtitle----\n");
                break;
            case CODEC_ID_MOV_TEXT:
                mMediaFormat.pSubType = &GUID_Subtitle_MOV_TEXT;
                break;
            case CODEC_ID_HDMV_PGS_SUBTITLE:
                mMediaFormat.pSubType = &GUID_Subtitle_HDMV_PGS_SUBTITLE;
                break;
            case CODEC_ID_DVB_TELETEXT:
                mMediaFormat.pSubType = &GUID_Subtitle_DVB_TELETEXT;
                break;
            case CODEC_ID_SRT:
                mMediaFormat.pSubType = &GUID_Subtitle_SRT;
                break;
            case CODEC_ID_MICRODVD:
                mMediaFormat.pSubType = &GUID_Subtitle_MICRODVD;
                break;
            case CODEC_ID_NONE:
                mMediaFormat.pMediaType = &GUID_None;
                AM_INFO("ffmpeg not recognized stream.\n");
                return ME_BAD_FORMAT;

            default:
                mMediaFormat.pSubType = &GUID_Subtitle_OTHER_CODECS;
                AM_INFO("Other subtitle format, codec id = 0x%x\n", pCodec->codec_id);
                break;
        }
        mMediaFormat.pMediaType = &GUID_Subtitle;
        mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Stream;
        mMediaFormat.format = (AM_INTPTR)pStream;
        return ME_OK;
    } else if (media == CODEC_TYPE_DATA) {
        mMediaFormat.pMediaType = &GUID_Pridata;
        mMediaFormat.pFormatType = &GUID_NULL;
        mMediaFormat.pSubType = &GUID_NULL;
        return ME_OK;
    }

    return ME_ERROR;
}


