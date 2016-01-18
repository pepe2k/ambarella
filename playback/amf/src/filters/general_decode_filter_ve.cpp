/*
 * general_decode_filter_ve.cpp
 *
 * History:
 *    2010/6/1 - [QingXiong Z] create file
 *    deprecated, used for transcoding, *** to be removed ***
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_new.h"
#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#include "engine_guids.h"
#if PLATFORM_ANDROID
#include <basetypes.h>
#else
#include "basetypes.h"
#endif
#include "iav_drv.h"
//#include "msgsys.h"
extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "amdsp_common.h"
#include "filter_list.h"

#include "video_decoder_dsp_ve.h"
//#include "video_decoder_ffmpeg.h"

#include "general_decode_filter_ve.h"

filter_entry g_general_video_decoder = {
    "CGeneralDecoderVE",
    CGeneralDecoderVE::Create,
    NULL,
    CGeneralDecoderVE::AcceptMedia,
};

IFilter* CreateGeneralVideoDecodeFilter(IEngine *pEngine)
{
	return CGeneralDecoderVE::Create(pEngine);
}

void CGVideoBufferPool::OnReleaseBuffer(CBuffer *pBuffer)
{
    CVideoBuffer* pVideoBuffer = (CVideoBuffer *)pBuffer;
    if ((pBuffer->GetType() == CBuffer::DATA) && mpUDECHandler) {
        mpUDECHandler->ReleaseFrameBuffer(mUdecIndex, pVideoBuffer);
    }
}

//todo engine create do more!
IFilter* CGeneralDecoderVE::Create(IEngine *pEngine)
{
    CGeneralDecoderVE* result = new CGeneralDecoderVE(pEngine);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CGeneralDecoderVE::Construct()
{
    AM_INFO("--->CGeneralDecoderVE::Construct .\n");
    //if (mpSharedRes->mGeneralDecoder != 1) {
    //    AMLOG_INFO("Not select general decoder .\n");
    //    return ME_ERROR;
   // }
    AM_ERR err = inherited::Construct();
    if (err != ME_OK) {
        AM_ERROR("CGeneralDecoderVE::Construct fail err %d .\n", err);
        return err;
    }
    DSetModuleLogConfig(LogModuleGeneralDecoder);
    //generate pin, common the outputpin is 1, the inputpin can be more than 1
    err = ConstructPin();
    if (err != ME_OK) {
        AM_ERROR("CGeneralDecoderVE::Construct Pin err %d .\n", err);
        return err;
    }
    AM_INFO("<---CGeneralDecoderVE::Construct . Done.\n");
    //todo needed a mutex?
    return ME_OK;
}

AM_ERR CGeneralDecoderVE::ConstructPin()
{
    //Output
    if(mpOutputPin == NULL)
    {
        mpOutputPin = CGeneralOutputPinVE::Create(this);
        if(mpOutputPin == NULL)
            return ME_ERROR;
        if((mpBufferPool = CGVideoBufferPool::Create("DecoderOuputBP", 32)) == NULL)
            return ME_ERROR;
        mpOutputPin->SetBufferPool(mpBufferPool);
    }
    //mMaxInput = 1;
    return ME_OK;
    //CGeneralInputPinVE* pInput = CGeneralInputPinVE::Create(this);
    //mInputPinVec[mInputPinNum] = pInput;
    //mInputPinNum ++;
}

AM_ERR CGeneralDecoderVE::ConstructInputPin()
{
    if(mpInputPin != NULL)
        return ME_OK;
    CGeneralInputPinVE* pInput = CGeneralInputPinVE::Create(this);
    if(pInput == NULL)
    {
        AM_INFO("CGeneralInputPinVE::Create Failed!\n");
    }
    mpInputPin = pInput;
    return ME_OK;
    //mInputPinVec[mInputPinNum] = pInput;
    //mInputPinNum ++;
}

void CGeneralDecoderVE::GetInfo(INFO& info)
{
    info.nInput = 1;
    info.nOutput = 1;
    info.mPriority = 0;
    info.mFlags = 0;
    if(mpVDecoder)
        info.mIndex = mpVDecoder->GetDecoderInstance();
    else
        info.mIndex = -1;
    info.pName = "CGeneralDecoderVE";
}

int CGeneralDecoderVE::AcceptMedia(CMediaFormat& format)
{
//debug use
#if (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
    if (g_GlobalCfg.mUseGeneralDecoder == 0)
        return 0;
#endif

    if (*format.pFormatType != GUID_Format_FFMPEG_Stream)
        return 0;

    if (*format.pMediaType == GUID_Video/* && *format.pSubType == GUID_AmbaVideoDecoder*/)
        return 1;

    return 0;
}
//==============================
//
//==============================
AM_ERR CGeneralDecoderVE::DoStop()
{
    //AM_ERR err;
    mpVDecoder->Stop();
    //if(err  != ME_OK)
    //{
        //AM_INFO("mpVDecoder Stop Failed\n");
        //return err;
    //}
    //ifmpbuffer != NULL
    return ME_OK;
}

AM_ERR CGeneralDecoderVE::DoPause()
{
    //mpVDecoder->Pause();
    return ME_OK;
}
AM_ERR CGeneralDecoderVE::DoResume()
{
    //mpVDecoder->Resume();
    return ME_OK;
}
AM_ERR CGeneralDecoderVE::DoFlush()
{
    mpVDecoder->Flush();
    if(mpBuffer != NULL)
    {
        mpBuffer->Release();
        mpBuffer = NULL;
    }
    return ME_OK;
}

AM_ERR CGeneralDecoderVE::DoConfig()
{
    //get out config cmd
    //mpVDecoder->Config(cmd);
    return ME_OK;
}
//==============================
//
//process the msg from decoder, OnCmd
//==============================
void CGeneralDecoderVE::MsgProc(AM_MSG& msg)
{
    switch(msg.code)
    {
    case MSG_DECODED:
        //do something recive a cbuffer send out and release
        //COPY this CBuffer to output buffer;
        /*if(ppmod =1)
        {
               CBuffer* pBuffer = (CBuffer*)(msg.p0);
               SendBuffer(pBuffer);
        }
        */
        AMLOG_DEBUG("--MSG_DECODED\n");
        //mpBuffer->Release();
        //mpBuffer = NULL;
        break;
    case MSG_READY:
        AMLOG_DEBUG("--MSG_READY\n");
        mpWorkQ->PostMsg(CMD_ACK);
        //AM_INFO("--MSG_READY, DOne\n");
        break;
    case MSG_UDEC_ERROR:
        AMLOG_DEBUG("--MSG_UDEC_ERROR\n");
        //mpVDecoder->Stop();
        mpWorkQ->SendCmd(CMD_STOP);
        PostEngineErrorMsg(ME_UDEC_ERROR);
        break;
        //
    case MSG_DECODER_ERROR:
         break;

    case MSG_TEST:
        AMLOG_DEBUG("--MSG_TEST\n");
        ChangeDecoder();
        break;
/*
    case MSG_DSP_GET_OUTPIC:
        AMLOG_DEBUG("--MSG_DSP_GET_OUTPIC\n");
        if(1 == mpSharedRes->ppmode)
            Renderbuffer();
//        mpWorkQ->PostMsg(CMD_ACK);
        break;
*/
    default:
        break;
    }
}

//todo general filter's cmd process.
bool CGeneralDecoderVE::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("CGeneralDecoderVE::ProcessCmd %d.\n", cmd.code);
    switch (cmd.code)
    {
    case CMD_STOP:
        DoStop();
        mbRun = false;
        CmdAck(ME_OK);
        break;

     //todo check this
    case CMD_OBUFNOTIFY:
        if(msState == STATE_WAIT_OUTPUT)
        {
            msState = STATE_READY;
        }
        break;

    case CMD_PAUSE:
        if(msState != STATE_PENDING)
        {
            DoPause();
            msOldState = msState;
            msState = STATE_PENDING;
        }
        CmdAck(ME_OK);
        break;

    case CMD_RESUME:
        if(msState == STATE_PENDING)
        {
            DoResume();
            msState = msOldState;
        }
        CmdAck(ME_OK);
        break;

    case CMD_FLUSH:
        AM_INFO("CMD_FLUSH, the msState: %d \n", msState);
        DoFlush();
        msState = STATE_PENDING;
        mbStreamStart = false;
        CmdAck(ME_OK);
        break;

    case CMD_CONFIG:
        DoConfig();
        CmdAck(ME_OK);
        break;

    case CMD_AVSYNC:
        CmdAck(ME_OK);
        break;

    case CMD_ACK:
        //CmdAck(ME_OK);
        break;

    case CMD_BEGIN_PLAYBACK:
        AM_ASSERT(msState == STATE_PENDING);
        AM_INFO("CMD_BEGIN_PLAYBACK, the msState: %d, oldmsState: %d\n", msState, msOldState);
        msState = STATE_IDLE;
        mbStreamStart = false;
        //mbPaused = false;
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }

    return true;
}

void CGeneralDecoderVE::OnRun()
{
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
//    AM_INT ret = 0;
    CmdAck(ME_OK);

    mbRun = true;
    msState = STATE_IDLE;
    while(mbRun)
    {
        //AMLOG_STATE("Amba Decoder: start switch, msState=%d, %d input data. \n", msState, mpInputPin->mpBufferQ->GetDataCnt());
        switch (msState)
        {
        case STATE_IDLE:
            //wait till a data or a msg, or do nothing--OK
            //then change to inputdata state, that meet idle not chang into has output state? -- seem ok
            type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
            if(type == CQueue::Q_MSG)
            {
                ProcessCmd(cmd);
            }else{
                //which inputpin
                if(ReadInputData(result) == ME_OK)
                {
                    msState = STATE_HAS_INPUTDATA;
                }
            }
            break;

        case STATE_HAS_INPUTDATA:
            //has a data to process, try to do it, it has output, to ready state--OK
            //if no have output, change to new state-->wait output, in this case, here will on wait msg!!-->seem ok
            AM_ASSERT(mpBuffer);
            if(mpBufferPool->GetFreeBufferCnt() > 0)
            {
                msState = STATE_READY;
            }else{
                msState = STATE_WAIT_OUTPUT;
            }
            break;

            //todo, makesure that output will always hold enthoy buffer for msg send buffer.(add decoded num and comp to fbc)
        case STATE_WAIT_OUTPUT:
            //so, here wait msg, only wait msg, we can take the msg, only wait a out, than goto ready--OK
            //ADD if ...else...:: if recive a pause cmd, during pause revice a outn, will be dismissed.
            AM_ASSERT(mpBuffer);
            if(mpBufferPool->GetFreeBufferCnt() <= 0)
            {
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
            }else{
                msState = STATE_READY;
            }
            break;

       /* case STATE_WAIT_READY:
            //wait till decoder is ready
            if(mpBuffer->GetMediaType == M_VIDEO)
                if(mpVDecoder->IsReady() == AM_FALSE)
                    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                else msState = STATE_READY;

            break;*/

        case STATE_READY:
            //do and dododododoo, no msg wait here -->OK
            AM_ASSERT(mpBuffer);
            if(IsDecoderReady() != ME_OK)
            {
                //AM_INFO("IsDecoderReady() != ME_OK\n");
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
            }else{
                ProcessBuffer();
                msState = STATE_IDLE;
            }
            //msState = STATE_IDLE;
            break;

        //Pending only respond CMD_RESUME
        case STATE_PENDING:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            break;

        case STATE_ERROR:
            break;

        default:
            AM_ERROR(" %d",(AM_UINT)msState);
            break;
        }
    }
    AM_INFO("CGeneralDecoderVE OnRun Exit!\n");
}
//==============================
//
//==============================
AM_ERR CGeneralDecoderVE::IsDecoderReady()
{
    //if(mpBuffer->GetMediaType() == M_VIDEO)
    //{
    return mpVDecoder->IsReady(mpBuffer);
    //}else if(mpBuffer->GetMediaType() == M_AUDIO){
        //return mpADecoder->IsReady(mpBuffer);
    //}else{
        //post err
        //return AM_FALSE;
   // }
}

AM_ERR CGeneralDecoderVE::ReadInputData(CQueue::WaitResult& result)
{
    //AM_INT num = 0;
    //AM_INFO("--ReadInputData\n");
    CGeneralInputPinVE* pInput = (CGeneralInputPinVE* )(result.pOwner);
    AM_ASSERT(pInput == mpInputPin);
    if(!pInput->PeekBuffer(mpBuffer))
    {
        AM_ERROR("!!PeekBuffer Failed!\n");
        return ME_ERROR;
    }
    AMLOG_DEBUG("ReadInputData mpBuffer %p.\n", mpBuffer);
    /* test todo
    while(num < mInputPinNum)
    {
        pInput = mInputPinVec[num];
        if (pInput->PeekBuffer(mpBuffer))
        {
            //do something
        }
        num++;
    }
    */
    return ME_OK;
}

AM_ERR CGeneralDecoderVE::ProcessBuffer()
{
    //AM_INFO("ProcessBuffer\n");
    //static int num = 1;

    if(mpBuffer->GetType() == CBuffer::EOS)
    {
        ProcessEOS();
        return ME_OK;
    }

    // mpVideoDecoder->Decode(mpBuffer);
    //if(mpBuffer->GetMediaType() == M_AUDIO)
        //DecodeAudio(mpBuffer);
    //if(mpBuffer->GetMediaType() == M_VIDEO)
    AM_ERR err;
    err = mpVDecoder->Decode(mpBuffer);
    if(err != ME_OK)
    {
        AM_INFO("Decode Failed!\n");
        return err;
    }
// set out
    //waitmsg for decoder and process it?
    //AM_INFO("ProcessBuffer Done\n");
    /*if(num++ == 400)
    {
        AM_INFO("Test for ChangeDecoder!\n");
        AM_MSG msg;
        msg.code = MSG_TEST;
        msState = STATE_PENDING;
        PostMsgToFilter(msg);
    }*/

    mpBuffer = NULL;
    return ME_OK;
}
/*
AM_ERR CGeneralDecoderVE::Renderbuffer()
{
//    AMLOG_INFO("CGeneralDecoderVE::Renderbuffer. [%d]\n", mpVDecoder->GetDecoderInstance());
    AM_ERR err;
    CBuffer* pBuffer;
    CVideoBuffer* pVideoBuffer;
    if (!mpOutputPin->AllocBuffer(pBuffer)) {
        AM_ERROR("CGeneralDecoderVE::Renderbuffer: Alloc output buffer Fail!\n");
        return ME_ERROR;
    }
    pVideoBuffer = (CVideoBuffer*)pBuffer;
    mpVDecoder->GetDecodedFrame(pVideoBuffer);

    pBuffer->SetType(CBuffer::DATA);
    pBuffer->SetDataSize(0);
    pBuffer->SetDataPtr(NULL);
    mpOutputPin->SendBuffer(pBuffer);
    AMLOG_DEBUG("Renderbuffer pBuffer %p.\n", pBuffer);
    return ME_OK;
}
*/
void CGeneralDecoderVE::DecodeVideo()
{
    AMLOG_PRINTF("DecodeVideo mpBuffer %p.Index=[%d]\n", mpBuffer, mpVDecoder->GetDecoderInstance());
    mpVDecoder->Decode(mpBuffer);
    if (!mbStreamStart) {
        pthread_mutex_lock(&mpSharedRes->mMutex);
        GetUdecState(mpSharedRes->mIavFd, &mpSharedRes->udec_state, &mpSharedRes->vout_state, &mpSharedRes->error_code);
        pthread_mutex_unlock(&mpSharedRes->mMutex);
        AM_ASSERT(IAV_UDEC_STATE_RUN == mpSharedRes->udec_state);
        if (IAV_UDEC_STATE_RUN == mpSharedRes->udec_state) {
            PostEngineMsg(IEngine::MSG_NOTIFY_UDEC_IS_RUNNING);
            mbStreamStart = true;
        }
    }
}

void CGeneralDecoderVE::DecodeAudio()
{
    mpADecoder->Decode(mpBuffer);
}

AM_ERR CGeneralDecoderVE::ProcessEOS()
{
    AM_ERR err=ME_OK;
    CBuffer* pBuffer;
    err = mpVDecoder->FillEOS();
    AM_VERBOSE("err=%d\n", err);
    if (!mpOutputPin->AllocBuffer(pBuffer)) {
        AM_ERROR("CGeneralDecoderVE Alloc output buffer Fail!\n");
        return ME_ERROR;
    }
    pBuffer->mPTS = mpBuffer->mPTS;
    pBuffer->SetType(CBuffer::EOS);
    pBuffer->SetDataSize(0);
    pBuffer->SetDataPtr(NULL);
    mpOutputPin->SendBuffer(pBuffer);
    return ME_OK;
}

AM_ERR CGeneralDecoderVE::SendBuffer(CBuffer* oBuffer)
{
    CBuffer *pBuffer;
    if (!mpOutputPin->AllocBuffer(pBuffer)) {
        AM_ERROR("CGeneralDecoderVE Alloc output buffer Fail!\n");
        return ME_ERROR;
    }
    pBuffer->SetType(CBuffer::DATA);
    //::memcpy(pBuffer, oBuffer, sizeof(CBuffer));
    *pBuffer = *oBuffer;

    pBuffer->SetDataSize(0);
    pBuffer->SetDataPtr(NULL);
    mpOutputPin->SendBuffer(pBuffer);
    return ME_OK;
}
//==============================
//
//==============================

//todo
AM_ERR CGeneralDecoderVE::CreateDecoder()
{
    //if(mpMediaFormat->......)
        //mpVDecoder = Create(this, FFdecoder);
    //else
    if(*(mpMediaFormat->pFormatType) != GUID_Format_FFMPEG_Stream ||
        *(mpMediaFormat->pMediaType) != GUID_Video)
        return ME_ERROR;
    if(*(mpMediaFormat->pSubType) == GUID_AmbaVideoDecoder)
    {
        mpVDecoder = CVideoDecoderDspVE::Create((IFilter* )this, mpMediaFormat);
    }else{
/*        DecoderConfig config;
        VideoDecoderFFMpeg* pDecoder = VideoDecoderFFMpeg::Create(mpSharedRes);
        mpVDecoder = (IDecoder*) pDecoder;
        AM_ASSERT(mpVDecoder);
        config.pHandle = (void*)mpMediaFormat->format;
        config.p_filter = (void*)((CInterActiveFilter*)this);
        config.p_outputpin = (void*)((COutputPin*)mpOutputPin);
        if (mpVDecoder->ConstructContext(&config) != ME_OK) {
            AMLOG_ERROR("ConstructContext Failed!\n");
            mpVDecoder = NULL;
            delete pDecoder;
            return ME_ERROR;
        }
*/
    }
    if(mpVDecoder == NULL)
    {
        AM_ERROR("CreateDecoder Failed!\n");
        return ME_ERROR;
    }
    AM_ASSERT(mpBufferPool);

    mpBufferPool->SetDSPRelated(mpSharedRes->udecHandler, mpVDecoder->GetDecoderInstance());
    return ME_OK;
    //mpADecoder = Create(this, ffdecoder);
}

AM_ERR CGeneralDecoderVE::ChangeDecoder()
{
    AM_ERR err=ME_OK;
    err = Stop();
    AM_VERBOSE("err=%d\n", err);
    AM_DELETE(mpVDecoder);
    //DoStop();
    mpVDecoder = CVideoDecoderDspVE::Create((IFilter* )this, mpMediaFormat);
    if(mpVDecoder == NULL)
    {
        AM_INFO("ChangeDecoder Failed!\n");
        return ME_ERROR;
    }
    mpWorkQ->SendCmd(CMD_RUN);
    AM_INFO("ChangeDecoder Done!\n");
    return ME_OK;
}

//==============================
//
//==============================
IPin* CGeneralDecoderVE::GetInputPin(AM_UINT index)//AM_UINT index)
{
    //if(mInputPinNum == max)
        //return NULL;
    if(index != 0)
        return NULL;
    AM_INFO("Call CGeneralDecoderVE::GetInputPin.\n");
    ConstructInputPin();
    //return mInputPinVec[mInputPinNum];
    return mpInputPin;
}

IPin* CGeneralDecoderVE::GetOutputPin(AM_UINT index)//AM_UINT index)
{
    if(mpOutputPin->mpPeer)
    {
        //
        return NULL;
    }
    return mpOutputPin;
}

//take something for mutil inputpin
//Called by inputpin's CheckMediaFormat
AM_ERR CGeneralDecoderVE::SetInputFormat(CMediaFormat* pFormat)
{
    AM_ERR err;
    mpMediaFormat = pFormat;
    //imp todo.
    err = CreateDecoder();
    if(err != ME_OK)
    {
        AM_INFO("CGeneralDecoderVE SetInputFormat Failed!\n");
        return err;
    }

    return ME_OK;
}

//called by output's GetMediaFormat
AM_ERR CGeneralDecoderVE::SetOutputFormat(CMediaFormat* pFormat)
{
    pFormat->pMediaType = &GUID_Amba_Decoded_Video;
    pFormat->pSubType = &GUID_Video_YUV420NV12;
    pFormat->pFormatType = &GUID_Format_FFMPEG_Media;
    pFormat->format = mpSharedRes->mIavFd;
    pFormat->mDspIndex = mpVDecoder->GetDecoderInstance();
    return ME_OK;
}

void CGeneralDecoderVE::PrintState()
{
    AMLOG_INFO("CGeneralDecoderVE [%d] State: %d, Input Data:%d \n", mpVDecoder->GetDecoderInstance(), msState, mpInputPin->GetDataCnt());
    mpVDecoder->PrintState();
}

//May be called by engine, reset all the state and value whenever.
AM_ERR CGeneralDecoderVE::ReSet()
{
    AM_ERR err;
    err = ((CVideoDecoderDspVE* )mpVDecoder)->ReSet();
    err = Stop();
    err = mpWorkQ->SendCmd(CMD_RUN);
    return err;
}

void CGeneralDecoderVE::Delete()
{
    inherited::Delete();
}

CGeneralDecoderVE::~CGeneralDecoderVE()
{
    AMLOG_PRINTF("***~CGeneralDecoderVE start...\n");
    AM_DELETE(mpInputPin);
    mpInputPin = NULL;
    AM_DELETE(mpOutputPin);//will free the bufferpool
    mpOutputPin = NULL;

    AM_DELETE(mpVDecoder);
    mpVDecoder = NULL;
    AMLOG_PRINTF("***~CGeneralDecoderVE end.\n");
}
