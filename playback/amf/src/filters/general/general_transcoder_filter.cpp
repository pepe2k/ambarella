/*
 * general_transcoder_filter.cpp
 *
 * History:
 *    2013/7/21 - [GLiu] create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
 #include <unistd.h>
//#include <fcntl.h>
//#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
 #include <basetypes.h>
#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"
extern "C" {
//#define INT64_C
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

#include "g_muxer_ffmpeg.h"
#include "general_dsp_related.h"
#include "general_transcoder_filter.h"
#include "general_transc_audiosink_filter.h"
#include "g_injector_rtsp.h"
#include "g_injector_rtmp.h"

#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
static struct timeval pre_time = {0,0};
static struct timeval curr_time = {0,0};

CGeneralTranscoder* CreateGeneralTranscoderFilter(IEngine* pEngine, CGConfig* config)
{
    return CGeneralTranscoder::Create(pEngine, config);
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
CGeneralTranscoder* CGeneralTranscoder::Create(IEngine* pEngine, CGConfig* pConfig)
{
    CGeneralTranscoder* result = new CGeneralTranscoder(pEngine, pConfig);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CGeneralTranscoder::CGeneralTranscoder(IEngine* pEngine, CGConfig* pConfig):
    inherited(pEngine, "CGeneralTranscoder"),
    mpConfig(pConfig),
    mpBuffer(NULL),
    mpABuffer(NULL),
    mpDsp(NULL),
    mpBufferPool(NULL),
    mpAudioInputPin(NULL),
    mpFakeAudiooutputPin(NULL),
    //mpTranscoder(NULL),
    mprtspInjector(NULL),
    mprtmpInjector(NULL),
    mpMuxer(NULL),
    mFrames(0),
    pDumpFile(NULL),
    mbDumpFile(AM_FALSE),
    mLastPTS(0xffffffffffffffffLL),
    mPTSLoop(0)
{
    mbRun = false;
    mbTranscoderInited = AM_FALSE;
    mbrtspInjectorConfiged = AM_FALSE;
    mbrtspAInjectorConfiged = AM_FALSE;
    mbrtmpInjectorConfiged = AM_FALSE;
    mbMuxerConfiged = AM_FALSE;
    mprtspInjectorConfig = new CUintMuxerConfig;
    mprtspAInjectorConfig = new CUintMuxerConfig;
    mprtmpInjectorConfig = new CUintMuxerConfig;

    mpBuffer = new(CGBuffer);
    mpABuffer = new(CGBuffer);
}

AM_ERR CGeneralTranscoder::Construct()
{
    AM_INFO("****CGeneralTranscoder::Construct .\n");

    AM_ERR err = inherited::Construct();
    if (err != ME_OK) {
        AM_ERROR("CGeneralTranscoder Construct fail err %d .\n", err);
        return err;
    }

    CDspGConfig* pDspConfig = &(mpConfig->dspConfig);
    mpDsp = pDspConfig->udecHandler;

#if !PLATFORM_ANDROID
    mprtspInjector = CGInjectorRtsp::Create(NULL);
    if(mprtspInjector == NULL){
        AM_ERROR("Create CGInjectorRtsp Failed!\n");
        err = ME_ERROR;
    }
    mprtspAInjector = CGInjectorRtsp::Create(NULL);
    if(mprtspAInjector == NULL){
        AM_ERROR("Create A CGInjectorRtsp Failed!\n");
        err = ME_ERROR;
    }
#endif
    err = ConstructPin();
    if (err != ME_OK) {
        AM_ERROR("CGeneralTransAudioSink::Construct Pin err %d .\n", err);
        return err;
    }
    SetThreadPrio(1, 1);
    DSetModuleLogConfig(LogModuleVideoTranscoder);
    return ME_OK;
}

AM_ERR CGeneralTranscoder::ConstructPin()
{
    //Output
/*    if(mpFakeAudiooutputPin == NULL) {
        mpFakeAudiooutputPin = CGeneralOutputPin::Create(this);
        if(mpFakeAudiooutputPin == NULL)
            return ME_ERROR;
        if((mpFakeAudiooutputPin = CGeneralBufferPool::Create("G-Transcoder mpFakeAudiooutputPin", 8)) == NULL)
            return ME_ERROR;
        mpFakeAudiooutputPin->SetBufferPool(mpBufferPool);
    }*/
    return ME_OK;
}

AM_ERR CGeneralTranscoder::ConstructInputPin(AM_INT index)
{
    if(mpAudioInputPin != NULL)
        return ME_OK;

    CGeneralInputPin* pInput = CGeneralInputPin::Create(this);
    if(pInput == NULL)
    {
        AM_ERROR("CGeneralInputPin::Create Failed!\n");
        //return ME_ERROR;
    }
    AM_ASSERT(index==0);
    mpAudioInputPin = pInput;
    return ME_OK;
}

IPin* CGeneralTranscoder::GetInputPin(AM_UINT index)
{
    AM_INFO("Call CGeneralTranscoder::GetInputPin %d.\n", index);
    if(index != 0)
        return NULL;
    ConstructInputPin(0);
    return mpAudioInputPin;
}

AM_ERR CGeneralTranscoder::SetInputFormat(CMediaFormat* pFormat)
{
    return ME_OK;
}

AM_ERR CGeneralTranscoder::SetOutputFormat(CMediaFormat* pFormat)
{
    return ME_OK;
}

void CGeneralTranscoder::Delete()
{
    AM_INFO(" ====== CGeneralTranscoder Exit OnRun Loop====== \n");
    //AM_INT i = 0;

    StopTranscoder();
    if (mpMuxer) {
        mpMuxer->FinishMuxer();
        AM_DELETE(mpMuxer);
        mpMuxer = NULL;
    }
    if(mprtspInjector){
        mprtspInjector->FinishInjector();
    }
    AM_DELETE(mprtspInjector);
    mprtspInjector = NULL;
    if(mprtmpInjector){
        mprtmpInjector->FinishInjector();
    }
    AM_DELETE(mprtmpInjector);
    mprtmpInjector = NULL;
    ReleaseTranscoder();
    AM_DELETE(mpAudioInputPin);
    mpAudioInputPin = NULL;
    //AM_DELETE(mpTranscoder);
    //mpTranscoder = NULL;
    if(mpBuffer){
        delete mpBuffer;
        mpBuffer= NULL;
    };
    if(mpABuffer){
        delete mpABuffer;
        mpABuffer= NULL;
    };

    if(pDumpFile!=NULL){
        fclose(pDumpFile);
        pDumpFile = NULL;
    }

    inherited::Delete();
}

CGeneralTranscoder::~CGeneralTranscoder()
{
    AM_INFO("Destroy CGeneralTranscoder Done.\n");
}

void CGeneralTranscoder::GetInfo(INFO& info)
{
    info.nInput = 0;
    info.nOutput = 0;
    info.mPriority = 0;
    info.mFlags = 0;
    info.mIndex = -1;
    info.pName = "CGeneralTranscoder";
}

AM_ERR CGeneralTranscoder::SetEncParam(AM_INT w, AM_INT h, AM_INT br)
{
    if(w>1920 || w< 64 || h>1088 || h<64 || br<1000 || br> 10000000){
        return ME_ERROR;
    }
    mEncWidth = w;
    mEncHeight = h;
    mBitrate = br;
    UpdateConfig();

    //mFrameLifeTime = (((AM_U64)IParameters::TimeUnitDen_90khz)*(12*1024*1024)*8)/(mBitrate);
    //mFrameLifeTime /= 2;

    return ME_OK;
}

AM_ERR CGeneralTranscoder::QueryInfo(AM_INT type, CParam& param)
{
    return ME_OK;
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
void CGeneralTranscoder::MsgProc(AM_MSG& msg)
{
    //AM_INT index;
    switch(msg.code){
        default:
            break;
    }
}

AM_ERR CGeneralTranscoder::DoStop()
{
    if(mprtspInjector){mprtspInjector->FinishInjector();}
    AM_DELETE(mprtspInjector);
    mprtspInjector = NULL;
    return ME_OK;
}

AM_ERR CGeneralTranscoder::DoPause(AM_INT target, AM_U8 flag)
{
    //AM_INFO("CGeneralTranscoder Pause, %d\n", target);

    return ME_OK;
}

AM_ERR CGeneralTranscoder::DoResume(AM_INT target, AM_U8 flag)
{
    //AM_INFO("CGeneralTranscoder Resume\n");

    return ME_OK;
}

AM_ERR CGeneralTranscoder::DoFlush(AM_INT target, AM_U8 flag)
{
    //AM_INFO("CGeneralTranscoder Flush\n");

    return ME_OK;
}

AM_ERR CGeneralTranscoder::DoConfig(AM_INT target, AM_U8 flag)
{
    AM_INFO("CGeneralTranscoder Config:%d, %d\n", mpConfig->generalCmd, mpConfig->specifyIndex);

    if(mpConfig->generalCmd & RENDERER_DISCONNECT_HW){

    }

    return ME_OK;
}

AM_ERR CGeneralTranscoder::DoRemove(AM_INT index, AM_U8 flag)
{
    AM_ERR err = ME_OK;

    return err;
}

bool CGeneralTranscoder::ProcessCmd(CMD& cmd)
{
    //AM_INFO("CGeneralTranscoder::ProcessCmd:%d.\n", cmd.code);
    AM_ERR err;
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
        par = cmd.res32_1;
        flag = cmd.flag;
        DoFlush(par, flag);
        CmdAck(ME_OK);
        break;


    case CMD_ACK:
        CmdAck(ME_OK);
        break;

    case CMD_CONFIG:
        par = cmd.res32_1;
        flag = cmd.flag;
        err = DoConfig(par, flag);
        CmdAck(err);
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
    return false;
}

CQueue::QType CGeneralTranscoder::WaitPolicy(CMD& cmd, CQueue::WaitResult& result)
{
    CQueue::QType type;
    type = mpWorkQ->WaitDataMsgCircularly(&cmd, sizeof(cmd), &result);
    return type;
}

AM_ERR CGeneralTranscoder::InitTranscoder()
{
    AM_ERR err;
    AM_INFO("CGeneralTranscoder InitEncoder.\n");

    //mEncInfo.input_format = IAV_INPUT_DRAM_422_PROG_IDSP;
    /*mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_29_97;
    mEncInfo.input_img.width = 1280;//get ??
    mEncInfo.input_img.height = 720;
    mEncInfo.encode_img.width = mEncWidth;
    mEncInfo.encode_img.height = mEncHeight;
    //mEncInfo.preview_enable = mbEnablePreview;*/
    err = mpDsp->InitMdecTranscoder(mEncWidth, mEncHeight, mBitrate);
    if(err!=ME_OK){
        AM_ERROR("InitMdecTranscoder error[%d], %d %d %d\n", err, mEncWidth, mEncHeight, mBitrate);
    }
    return err;
}

AM_ERR CGeneralTranscoder::ReleaseTranscoder()
{
    AM_ERR err;
    AM_INFO("CGeneralTranscoder ReleaseTranscoder.\n");

    err = mpDsp->ReleaseMdecTranscoder();
    if(err!=ME_OK){
        AM_ERROR("InitMdecTranscoder error[%d]\n", err);
    }
    return err;
}

AM_ERR CGeneralTranscoder::StartTranscoder()
{
    AM_ERR err;
    AM_INFO("CGeneralTranscoder StartTranscoder.\n");

    err = mpDsp->StartMdecTranscoder();
    if(err!=ME_OK){
        AM_ERROR("StartMdecTranscoder error[%d]\n", err);
    }
    return err;
}

AM_ERR CGeneralTranscoder::StopTranscoder()
{
    AM_ERR err;
    AM_INFO("CGeneralTranscoder StopTranscoder.\n");

    err = mpDsp->StopMdecTranscoder();
    if(err!=ME_OK){
        AM_ERROR("StopMdecTranscoder error[%d]\n", err);
    }
    return err;
}

AM_ERR CGeneralTranscoder::ReadInputData()
{
    AM_ERR err;

    err = mpDsp->MdecTranscoderReadBits(&mBitInfo);
    if(err != ME_OK){
        if (err == ME_BUSY){
            AMLOG_DEBUG("MdecTranscoderReadBits Return busy!\n");
            return ME_BUSY;
        }
        AMLOG_DEBUG("MdecTranscoderReadBits Return error: %d!\n", err);
        return err;
    }

    AMLOG_PTS("CGeneralTranscoder get: total %u frames.\n", mBitInfo.desc[0].frame_num+mBitInfo.count);
    return ME_OK;
}

AM_ERR CGeneralTranscoder::SetBitrate(AM_INT kbps)
{
    return mpDsp->MdecTranscoderSetBitrate(kbps);
}

AM_ERR CGeneralTranscoder::SetFramerate(AM_INT fps, AM_INT reduction)
{
    return mpDsp->MdecTranscoderSetFramerate(fps, reduction);
}

AM_ERR CGeneralTranscoder::SetGOP(AM_INT M, AM_INT N, AM_INT interval, AM_INT structure)
{
    return mpDsp->MdecTranscoderSetGOP(M, N, interval, structure);
}

AM_ERR CGeneralTranscoder::DemandIDR(AM_BOOL now)
{
    return mpDsp->MdecTranscoderDemandIDR(now);
}

AM_ERR CGeneralTranscoder::RecordToFile(AM_BOOL en, char* name)
{
    if (en == mbDumpFile)  return ME_OK;
    if (en == AM_TRUE) {
        mpMuxer = CGMuxerFFMpeg::Create(NULL, 0xff);
        if (mpMuxer == NULL) {
            AM_ERROR("Create CGMuxerFFMpeg Failed!\n");
        } else {
            mpMuxer->SetSavingTimeDuration(60, 0);//default
            if(name) strcpy(mprtspInjectorConfig->fileName, name);
            mbDumpFile = AM_TRUE;
            AM_INFO("CGeneralTranscoder: start dump transcode file %s.\n", mprtspInjectorConfig->fileName);
        }
    } else {
        mbDumpFile = AM_FALSE;
        AM_INFO("CGeneralTranscoder: stop dump transcode file %s.\n", mprtspInjectorConfig->fileName);
    }
    return ME_OK;
}

AM_ERR CGeneralTranscoder::UploadTranscode2Cloud(char* url, UPLOAD_NVR_TRANSOCDE_FLAG flag)
{
    AM_ERR err = ME_OK;
    mprtmpInjectorConfig->sendMe = AM_TRUE;
    strcpy(mprtmpInjectorConfig->rtmpUrl, url);

    if(mprtmpInjector!=NULL){
        AM_ERROR("mprtmpInjector = %p!\n", mprtmpInjector);
        return ME_TOO_MANY;
    }
#if !PLATFORM_ANDROID
    mprtmpInjector = CGInjectorRtmp::Create(NULL);
    if(mprtmpInjector == NULL){
        AM_ERROR("Create CGInjectorRtsp Failed!\n");
        err = ME_ERROR;
    }
#endif
    if(mprtmpInjector != NULL){
        if(AM_FALSE == mbrtmpInjectorConfiged){
            pthread_mutex_lock(&mprtmpInjectorConfig->mutex_);
            err = mprtmpInjector->ConfigMe(mprtmpInjectorConfig);
            pthread_mutex_unlock(&mprtmpInjectorConfig->mutex_);
            if(err != ME_OK){
                AM_ERROR("Create CGInjectorRtsp Failed!\n");
                err = ME_ERROR;
            }else{
                mbrtmpInjectorConfiged = AM_TRUE;
            }
        }
    }
    return ME_OK;
}

AM_ERR CGeneralTranscoder::StopUploadTranscode2Cloud(UPLOAD_NVR_TRANSOCDE_FLAG flag)
{
    mprtmpInjectorConfig->sendMe = AM_FALSE;
    return ME_OK;
}

AM_INT CGeneralTranscoder::GetSpsPpsLen(AM_U8 *pBuffer)
{
    AM_INT start_code, times, pos;
    unsigned char tmp;

    times = pos = 0;
    for (start_code = 0xffffffff; times < 3; pos++) {
        tmp = (unsigned char)pBuffer[pos];
        start_code <<= 8;
        start_code |= (unsigned int)tmp;
        if (start_code == 0x00000001) {
            times = times + 1;
        }
    }
    AM_INFO("sps-pps length = %d\n", pos - 4);
    return pos - 4;
}

void CGeneralTranscoder::SetSavingTimeDuration(AM_UINT duration)
{
    if (mpMuxer) {
        mpMuxer->SetSavingTimeDuration(duration, 0);
    }
    return;
}

AM_ERR CGeneralTranscoder::ConfigLoader(AM_BOOL en, char* path, char* m3u8name, char* host, AM_INT count)
{
    AM_ERR ret = ME_OK;
    if (mpMuxer) {
        mpMuxer->EnableLoader(en);
        if (en == AM_TRUE) {
            ret = mpMuxer->ConfigLoader(path, m3u8name, host, count);
        }
    }
    return ret;
}

void CGeneralTranscoder::OnRun()
{
    CMD cmd;
//    CQueue::QType type;
//    CQueue::WaitResult result;
    AM_ERR err;

    mbRun = true;
    msState = STATE_IDLE;
    CmdAck(ME_OK);

//    CBuffer* cBuffer;

    while(mbRun)
    {
        //AM_INFO("###CGeneral Renderer: start switch, msState=%d.\n", msState);
        switch (msState)
        {
        case STATE_IDLE:
            if(mbTranscoderInited == AM_FALSE){
                /*err = InitTranscoder();
                if (err == ME_OK) {
                    AMLOG_INFO("CGeneralTranscoder setupVideoTranscoder done!!\n");
                }else{
                    AM_ASSERT(0);
                    AM_ERROR("setupVideoTranscoder fail.\n");
                    msState = STATE_ERROR;
                }*/
                if(mpWorkQ->PeekCmd(cmd)){
                    ProcessCmd(cmd);
                    break;
                }
/*
                while (mpAudioInputPin->PeekBuffer(cBuffer)) {
                    mpABuffer = (CGBuffer* )cBuffer;
                    DiscardAudioBuffer();
                    if(mpAudioInputPin->GetDataCnt()<2)break;
                }
*/
                AM_UINT udecState, voutState, errorCode;
                err = mpDsp->GetUDECState(0, udecState, voutState, errorCode);
                if((err<0) || (udecState != IAV_UDEC_STATE_RUN)){
                    usleep(100000);
                    break;
                }
                AM_INFO("transcoder is ready\n");
                mbTranscoderInited = AM_TRUE;
            }
            if(mpWorkQ->PeekCmd(cmd)){
                ProcessCmd(cmd);
                break;
            }
            if (mbRun == false || msState != STATE_IDLE) {
                break;
            }
            err = ReadInputData();
            if (err == ME_OK) {
                msState = STATE_READY;
            } else if (err == ME_CLOSED) {
                AM_INFO("all buffers comes out??.\n");
                ProcessEOS();
                mbRun = false;
                msState = STATE_PENDING;
            } else if (err == ME_OS_ERROR) {
                AM_ERROR("IAV Error, goto Error state.\n");
                PostEngineMsg(IEngine::MSG_OS_ERROR);
                msState = STATE_ERROR;
            } else if (err == ME_ERROR) {
                AM_ERROR("How to handle this case?\n");
                msState = STATE_ERROR;
            } else {
                AM_ERROR("unsupposed value, return %d.\n", err);
                msState = STATE_ERROR;
            }
            break;

        case STATE_READY:
                if (ProcessBuffer() == ME_OK) {
                    msState = STATE_IDLE;
                } else {
                    msState = STATE_ERROR;
                }
/*                while (mpAudioInputPin->PeekBuffer(cBuffer)) {
                    mpABuffer = (CGBuffer* )cBuffer;
                    if(mpAudioInputPin->GetDataCnt()>4){
                        DiscardAudioBuffer();
                        continue;
                    }
                    ProcessAudioBuffer();
                    if(mpAudioInputPin->GetDataCnt()<1)break;
                }
*/
            break;

        case STATE_PENDING:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        case STATE_ERROR:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            if (cmd.code == CMD_STOP) {
                ProcessEOS();
                mbRun = false;
                CmdAck(ME_OK);
            } else {
                //ignore other cmds
                AM_WARNING("how to handle this type of cmd %d, in error state.\n", cmd.code);
            }
            break;

        default:
            AM_ERROR("ERROR State %d!!\n",(AM_UINT)msState);
            break;
        }
    }

    AM_INFO("CGeneralTranscoder OnRun Exit!\n");
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------

AM_ERR CGeneralTranscoder::ProcessBuffer()
{
    //AdjustAVSync();
    AM_ERR err = ME_OK;
    __read_bits_info_t* p_desc = NULL;
    AVPacket packet;
    AM_U64 curPTS = 0;

    for(int i=0; i<mBitInfo.count; i++){
        p_desc = &mBitInfo.desc[i];
        av_init_packet(&packet);
        packet.stream_index = 0;
        packet.size = p_desc->pic_size;
        packet.data = (uint8_t*)p_desc->start_addr;

        //packet.pts = p_desc->PTS;
        curPTS = p_desc->PTS & 0x00ffffffffLL;
        if(mLastPTS == 0xffffffffffffffffLL){
            //first frame
        }else{
            if(curPTS<mLastPTS){
                if(p_desc->pic_type==PredefinedPictureType_B){
                    //B frame, pts < lastPTS
                }
                else{
                    if((mLastPTS>(0x100000000LL-90000)) &&(curPTS<90000)){
                        mPTSLoop++;
                        AM_INFO("mLastPTS = %llu, curPTS = %llu, mPTSLoop=%llu.\n", mLastPTS, curPTS, mPTSLoop);
                    }else{
                        AM_ERROR("get a wrong PTS[%lld]: last: %lld, cur: %lld!\n", p_desc->PTS, mLastPTS, curPTS);
                    }
                }
            }else{
                if(p_desc->pic_type==PredefinedPictureType_B){
                    //B frame, pst>lastPTS, todo
                }
            }
        }
        mLastPTS = curPTS;
        packet.pts = mLastPTS + (mPTSLoop<<32);

        mFrames++;
        if(pre_time.tv_usec==0 && pre_time.tv_sec==0){
            gettimeofday(&pre_time, NULL);
        }
        if(mFrames%300==0){
            gettimeofday(&curr_time, NULL);
            if(pre_time.tv_usec != curr_time.tv_usec){
                AM_INFO("Transcoding FrameRate = %2.2f.\n",
                    300000000.0f/((curr_time.tv_sec - pre_time.tv_sec) * 1000000 + curr_time.tv_usec - pre_time.tv_usec));
                pre_time.tv_sec = curr_time.tv_sec;
                pre_time.tv_usec = curr_time.tv_usec;
            }
        }

        AM_ASSERT(mprtspInjectorConfig);

        if(mprtspInjectorConfig->videoInfo.extrasize == 0) {
            if(p_desc->pic_type == 1) {
                AM_INFO("Beginning to calculate sps-pps's length.\n");
                mprtspInjectorConfig->videoInfo.extrasize = GetSpsPpsLen((AM_U8*)p_desc->start_addr);
                AM_INFO("Calculate sps-pps's length completely.\n");
                memcpy(mprtspInjectorConfig->videoInfo.extradata, packet.data, mprtspInjectorConfig->videoInfo.extrasize + 8);
            }else{
                AM_DBG("Get a p frame before the first I frame, ignore.\n");
                continue;
            }
        }
        mpBuffer->SetExtraPtr((AM_INTPTR)(&packet));
        mpBuffer->SetOwnerType(TRANSCODER_DSP);
        mpBuffer->SetBufferType(DATA_BUFFER);
        //mpBuffer->mExpireTime = expired_time;
        mpBuffer->mBlockSize = mpBuffer->mDataSize = p_desc->pic_size;
        mpBuffer->mpData = (AM_U8*)p_desc->start_addr;
        //mpBuffer->SetIdentity(Index);

        if(mprtspInjector != NULL){
            if(!mbrtspInjectorConfiged){
                pthread_mutex_lock(&mprtspInjectorConfig->mutex_);
                err = mprtspInjector->ConfigMe(mprtspInjectorConfig);
                pthread_mutex_unlock(&mprtspInjectorConfig->mutex_);
                if(err != ME_OK){
                    AM_ERROR("Create CGInjectorRtsp Failed!\n");
                    err = ME_ERROR;
                }else{
                    mbrtspInjectorConfiged = AM_TRUE;
                }
            }
            if(mbrtspInjectorConfiged){
                mpBuffer->SetStreamType(STREAM_VIDEO);
                err = mprtspInjector->FeedData(mpBuffer, packet.pts, packet.pts);
            }

        }else{
            AM_ERROR("Create CGInjectorRtsp Failed!\n");
            err = ME_ERROR;
        }

        if(err == ME_ERROR) break;

        if(mprtmpInjectorConfig->sendMe==AM_TRUE){
            if(mprtmpInjector != NULL){
                if(mbrtmpInjectorConfiged){
                    mpBuffer->SetStreamType(STREAM_VIDEO);
                    //err = mprtmpInjector->FeedData(mpBuffer, packet.pts, packet.pts);
                    err = mprtmpInjector->FeedData(mpBuffer, -1, -1);
                }
            }
        }else{
            if(mprtmpInjector!=NULL){
                if(mprtmpInjector){
                    mprtmpInjector->FinishInjector();
                }
                AM_DELETE(mprtmpInjector);
                mprtmpInjector = NULL;
                mbrtmpInjectorConfiged = AM_FALSE;
            }
        }

        if(mbDumpFile == AM_TRUE){
            err = FeedDataToMuxer(p_desc, packet.pts);
            if (err != ME_OK) {
                AM_ERROR("Feed Data to Muxer Failed!\n");
                break;
            }
        }else{
            if (mpMuxer) {
                mpMuxer->FinishMuxer();
                AM_DELETE(mpMuxer);
                mpMuxer = NULL;
                mbMuxerConfiged = AM_FALSE;
            }
        }
    }
    return err;
}

AM_ERR CGeneralTranscoder::DiscardAudioBuffer()
{
    AM_ERR err = ME_ERROR;
    AM_ASSERT(mpABuffer);
    if(mpABuffer){
        CGBuffer* cbuffer = (CGBuffer*)mpABuffer->GetDataPtr();
        AM_INTPTR owner = mpABuffer->GetExtraPtr();
        //AM_INFO("CGeneralTranscoder::DiscardAudioBuffer(%d) PTS: %llu~~~~~~~~~~~\n",
        //    cbuffer->GetCount(), cbuffer->GetPTS());
        ((IGDemuxer*)owner)->OnReleaseBuffer(cbuffer);
        free(cbuffer);
        mpABuffer->Release();
        mpABuffer = NULL;
    }

    return err;
}

//#define V_A_in_1 1
AM_ERR CGeneralTranscoder::ProcessAudioBuffer()
{
    AM_ERR err = ME_ERROR;
    CBuffer* cBuffer;
    if(mpAudioInputPin->PeekBuffer(cBuffer)==false) {
        AM_ASSERT(0);
        return err;
    }
    mpABuffer = (CGBuffer* )cBuffer;
/*    if(mpAudioInputPin->GetDataCnt()>4){
        DiscardAudioBuffer();
        continue;
    }
*/
    AM_ASSERT(mpABuffer);
    if(mpABuffer){
        CGBuffer* cbuffer = (CGBuffer*)mpABuffer->GetDataPtr();

#if !V_A_in_1
        if(mprtspAInjector != NULL){
            if(!mbrtspAInjectorConfiged){
                pthread_mutex_lock(&mprtspAInjectorConfig->mutex_);
                err = mprtspAInjector->ConfigMe(mprtspAInjectorConfig);
                pthread_mutex_unlock(&mprtspAInjectorConfig->mutex_);
                if(err != ME_OK){
                    AM_ERROR("Create CGInjectorRtsp Failed!\n");
                    err = ME_ERROR;
                }else{
                    mbrtspAInjectorConfiged = AM_TRUE;
                }
            }
            if(mbrtspInjectorConfiged){
                if(mprtspAInjector){
                    cbuffer->SetStreamType(STREAM_AUDIO);
                    //err = mprtspAInjector->FeedData(cbuffer, cbuffer->GetPTS(), cbuffer->GetPTS());
                    err = mprtspAInjector->FeedData(cbuffer, -1, -1);
                }
            }
        }
#else
        if(mprtspInjector && mbrtspInjectorConfiged){
            cbuffer->SetStreamType(STREAM_AUDIO);
            //err = mprtspInjector->FeedData(cbuffer, cbuffer->GetPTS(), cbuffer->GetPTS());
            err = mprtspInjector->FeedData(cbuffer, -1, -1);
        }
#endif

        if(0 && (mbDumpFile == AM_TRUE) && (mbMuxerConfiged == AM_TRUE)){
            mpABuffer->SetStreamType(STREAM_AUDIO);
            mpMuxer->FeedData(mpABuffer);
        }

        //AM_INFO("CGeneralTranscoder::ProcessAudioBuffer(%d) PTS: %llu  %p %p------------\n",
        //    cbuffer->GetCount(), cbuffer->GetPTS(), cbuffer, mpABuffer);
        AM_INTPTR owner = mpABuffer->GetExtraPtr();
        ((IGDemuxer*)owner)->OnReleaseBuffer(cbuffer);
        free(cbuffer);
        mpABuffer->Release();
        mpABuffer = NULL;
    }

    return err;
}

AM_ERR CGeneralTranscoder::ProcessEOS()
{
    //AM_ERR err;

    return ME_OK;
}

AM_ERR CGeneralTranscoder::ReSet()
{
    return ME_OK;
}

void CGeneralTranscoder::Dump()
{
    AM_INFO("\n---------------------GeneralTranscoder Info------------\n");
    AM_INFO("       BufferPool--->Video input DataCnt:%d\n", mFrames);
    if(mpAudioInputPin){
        AM_INFO("  Audio input DataCnt:%d\n",
            mpAudioInputPin->GetDataCnt());
    }
    if(mprtspInjector){
        mprtspInjector->Dump();
    }
    AM_INFO("       mpMuxer:%p\n", mpMuxer);
    if(mpMuxer){
        mpMuxer->Dump();
    }
    AM_INFO("       State---> on state: %d\n", msState);
    AM_INFO("\n");
}

AM_ERR CGeneralTranscoder:: FeedDataToMuxer(void* p_desc, AM_U64 pts)
{
    if (mpMuxer == NULL) {
        AM_ERROR("creat Muxer failed!\n");
        return ME_ERROR;
    }

    if (mbMuxerConfiged == AM_FALSE) {
        if (mpMuxer->ConfigMe(mprtspInjectorConfig) != ME_OK) {
            AM_ERROR("Config Muxer failed!\n");
            return ME_ERROR;
        }
        mbMuxerConfiged = AM_TRUE;
        AM_INFO("mpMuxer %p config done.\n", mpMuxer);
    }

    AM_ERR err = ME_OK;
    AM_ASSERT(mpBuffer);
    __read_bits_info_t* pDesc = (__read_bits_info_t*)p_desc;
    //mpBuffer->mExpireTime = expired_time;
    AM_ASSERT(mpBuffer->mpData);

    mpBuffer->SetExtraPtr((AM_INTPTR)NULL);//remove packet
    mpBuffer->SetPTS(pts);
    mpBuffer->mFrameType = pDesc->pic_type;

    mpBuffer->SetStreamType(STREAM_VIDEO);
    err = mpMuxer->FeedData(mpBuffer);
    return err;
}

void CGeneralTranscoder:: UpdateConfig()
{
    if(mprtspInjectorConfig!=NULL){
        mprtspInjectorConfig->configVideo = AM_TRUE;
#if !V_A_in_1
        mprtspInjectorConfig->configAudio = AM_FALSE;
#else
        mprtspInjectorConfig->configAudio = AM_TRUE;
#endif
        mprtspInjectorConfig->videoInfo.width = mEncWidth;
        mprtspInjectorConfig->videoInfo.height = mEncHeight;
        mprtspInjectorConfig->videoInfo.bitrate = mBitrate;
        mprtspInjectorConfig->videoInfo.codec = CODEC_ID_H264;
        mprtspInjectorConfig->videoInfo.extrasize = 0;
        mprtspInjectorConfig->ds_type_live = 1;
        strcpy(mprtspInjectorConfig->rtmpUrl, "nvr.amba");
        //todo
        strcpy(mprtspInjectorConfig->fileName, "transcode.ts");

        mprtspInjectorConfig->framerate_den = 3003;
        mprtspInjectorConfig->framerate_num = 90000;

        mprtspInjectorConfig->nameWithTime = AM_FALSE;

        mprtspInjectorConfig->audioInfo.codec = CODEC_ID_AAC;
        mprtspInjectorConfig->audioInfo.bitrate = 40000;
        mprtspInjectorConfig->audioInfo.channels = 2;
        mprtspInjectorConfig->audioInfo.samplerate = 48000;
        mprtspInjectorConfig->audioInfo.samplefmt = AV_SAMPLE_FMT_S16;
    }
    if(mprtspAInjectorConfig!=NULL){
        mprtspAInjectorConfig->configVideo = AM_FALSE;
        mprtspAInjectorConfig->configAudio = AM_TRUE;
        mprtspAInjectorConfig->videoInfo.width = mEncWidth;
        mprtspAInjectorConfig->videoInfo.height = mEncHeight;
        mprtspAInjectorConfig->videoInfo.bitrate = mBitrate;
        mprtspAInjectorConfig->videoInfo.codec = CODEC_ID_H264;
        mprtspAInjectorConfig->videoInfo.extrasize = 0;
        mprtspAInjectorConfig->ds_type_live = 1;
        strcpy(mprtspAInjectorConfig->rtmpUrl, "nvr.back");
        //todo
        strcpy(mprtspAInjectorConfig->fileName, "transcode.ts");

        mprtspAInjectorConfig->framerate_den = 3003;
        mprtspAInjectorConfig->framerate_num = 90000;

        mprtspAInjectorConfig->nameWithTime = AM_FALSE;

        mprtspAInjectorConfig->audioInfo.codec = CODEC_ID_AAC;
        mprtspAInjectorConfig->audioInfo.bitrate = 40000;
        mprtspAInjectorConfig->audioInfo.channels = 2;
        mprtspAInjectorConfig->audioInfo.samplerate = 48000;
        mprtspInjectorConfig->audioInfo.samplefmt = AV_SAMPLE_FMT_S16;
    }

    if(mprtmpInjectorConfig!=NULL){
        mprtmpInjectorConfig->configVideo = AM_TRUE;
        mprtmpInjectorConfig->configAudio = AM_FALSE;
        mprtmpInjectorConfig->videoInfo.width = mEncWidth;
        mprtmpInjectorConfig->videoInfo.height = mEncHeight;
        mprtmpInjectorConfig->videoInfo.bitrate = mBitrate;
        mprtmpInjectorConfig->videoInfo.codec = CODEC_ID_H264;
        mprtmpInjectorConfig->videoInfo.extrasize = 0;
        mprtmpInjectorConfig->ds_type_live = 1;
        strcpy(mprtmpInjectorConfig->rtmpUrl, "");

        mprtmpInjectorConfig->framerate_den = 3003;
        mprtmpInjectorConfig->framerate_num = 90000;

        mprtmpInjectorConfig->nameWithTime = AM_FALSE;
    }
    return;
}