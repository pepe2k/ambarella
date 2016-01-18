/*
 * g_render_out.cpp
 *
 * History:
 *    2012/4/5 - [QingXiong Z] created file
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

#include "record_if.h"

#include "general_dsp_related.h"
#include "g_render_out.h"
//==============================================================
//
//CVideoRenderOut
//
//==============================================================
IUDECHandler* CVideoRenderOut::mpIAV = NULL;
int CVideoRenderOut::mIavFd = -1;

IRenderOut* CVideoRenderOut::Create(CGConfig* pConfig, AM_INT index)
{
    CVideoRenderOut* result = new CVideoRenderOut(pConfig, index);
    if (result && result->Construct() != ME_OK)
    {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CVideoRenderOut::Construct()
{
    return ME_OK;
}

void CVideoRenderOut::Delete()
{
    AMLOG_DESTRUCTOR("CGSyncRenderer::Delete().\n");
}

CVideoRenderOut::~CVideoRenderOut()
{
    AMLOG_DESTRUCTOR("~CGSyncRenderer.\n");
    AMLOG_DESTRUCTOR("~CGSyncRenderer done.\n");
}

AM_ERR CVideoRenderOut::RenderOutBuffer(CGBuffer* pBuffer)
{
    return ME_OK;
}

AM_ERR CVideoRenderOut::OpenHal(CGBuffer* handleBuffer)
{
    if(mpIAV == NULL)
    {
        mpIAV = mpGConfig->dspConfig.udecHandler;
        mIavFd = mpGConfig->dspConfig.iavHandle;
    }
    AM_ASSERT(mpIAV == mpGConfig->dspConfig.udecHandler);
    return ME_OK;
}

//All is done on here
AM_ERR CVideoRenderOut::PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend)
{
    AM_ERR err = ME_ERROR;
    switch(cmd.code)
    {
    case CMD_ISEOS:
        err = DoWaitEOS();
        break;

    case CMD_CONFIG_WIN:
        err = DoConfigWin();
        break;

    case IActiveObject::CMD_FLUSH:
        err = DoFlush(cmd);
        mStateCheck = VDSP_IDLE;
        break;

    case CMD_SWITCH_HD:
        err = DoSwitch(cmd);
        break;

    case CMD_SWITCH_BACK:
        err = DoSwitchBack(cmd);
        break;

    case IActiveObject::CMD_PAUSE:
        AM_INFO("mStateCheck: %d\n", mStateCheck);
        if(mStateCheck == VDSP_IDLE)
            err = DoPause();
        mStateCheck = VDSP_PAUSED;
        break;

    case IActiveObject::CMD_RESUME:
        AM_INFO("mStateCheck: %d\n", mStateCheck);
        if((mStateCheck == VDSP_PAUSED) || (mStateCheck == VDSP_STEP))
            err = DoResume();
        mStateCheck = VDSP_IDLE;
        break;

    case IActiveObject::CMD_STEP:
        AM_INFO("mStateCheck: %d\n", mStateCheck);
        mStateCheck = VDSP_STEP;
        err = DoStep();
        break;

    case CMD_PLAYBACK_ZOOM:
        err = DoPlaybackZoom(cmd);
        break;

    default:
        AM_ERROR("Wrong cmd:%d\n", cmd.code);
    }

    return err;
}

AM_ERR CVideoRenderOut::DoStep()
{
    AM_INT dspInstance = mpGConfig->indexTable[mConIndex].dspIndex;
    AM_INFO("Video out DoStep Dsp%d.\n", dspInstance);

    AM_ERR err;
    err = mpIAV->StepPlay(dspInstance);
    return err;
}

AM_ERR CVideoRenderOut::DoPause()
{
    AM_INT dspInstance = mpGConfig->indexTable[mConIndex].dspIndex;
    AM_INFO("Video out DoPause Dsp%d.\n", dspInstance);

    AM_ERR err;
    err = mpIAV->PauseUDEC(dspInstance);
    return err;
}

AM_ERR CVideoRenderOut::DoResume()
{
    AM_INT dspInstance = mpGConfig->indexTable[mConIndex].dspIndex;
    AM_INFO("Video out DoResume Dsp%d.\n", dspInstance);

    AM_ERR err;
    err = mpIAV->ResumeUDEC(dspInstance);
    return err;
}

AM_ERR CVideoRenderOut::DoConfigWin()
{
    AM_INFO("DoConfigWin.\n");

    AM_ERR err;
    err = mpIAV->ConfigWindowRender(mpGConfig);
    return err;
}

AM_ERR CVideoRenderOut::DoWaitEOS()
{
    AM_INT ret;
    AM_INT dspInstance = mpGConfig->indexTable[mConIndex].dspIndex;

    iav_udec_status_t iavState;
    memset(&iavState, 0, sizeof(iavState));
    iavState.decoder_id = dspInstance;
    iavState.nowait = 0;
    if((ret = ioctl(mIavFd, IAV_IOC_WAIT_UDEC_STATUS, &iavState)) < 0){
        AM_INFO("No Handle error right now\n");
        return ME_ERROR;
    }
    if(iavState.eos_flag){
        AM_INFO("[udec %d]get eos flag %u, %u!!\n", dspInstance, iavState.pts_low, iavState.pts_high);
        mbEosFlag = AM_TRUE;
        mPTSLow = iavState.pts_low;
        mPTSHigh = iavState.pts_high;
        return ME_TRYNEXT;
        //return ME_OK;//debug just, pts not retruen when nvr palyback
    }
    //AM_INFO("sHOW FOR DEBUG: PTSLOW:%d, ptshigh:%d\n", iavState.pts_low, iavState.pts_high);
    if(mbEosFlag && iavState.pts_low == mPTSLow && iavState.pts_high == mPTSHigh){
        mbEosFlag = AM_FALSE;
        mPTSLow = mPTSHigh = 0;
        return ME_OK;
    }

    return ME_ERROR;
}

AM_ERR CVideoRenderOut::DoFlush(IActiveObject::CMD& cmd)
{
    AM_INT dspInstance = mpGConfig->indexTable[mConIndex].dspIndex;
    AM_INFO("Video out DoFlush Dsp%d.\n", dspInstance);
    AM_ERR err;
    AM_BOOL lastPic = cmd.flag;
    err = mpIAV->FlushUDEC(dspInstance, lastPic);
    if(err == ME_OK){
        err = mpIAV->ClearUDEC(dspInstance);
    }
    return err;
}

AM_ERR CVideoRenderOut::DoSwitch(IActiveObject::CMD& cmd)
{
    return ME_OK;
}

AM_ERR CVideoRenderOut::DoSwitchBack(IActiveObject::CMD& cmd)
{
    return ME_OK;
}

AM_ERR CVideoRenderOut::DoPlaybackZoom(IActiveObject::CMD& cmd)
{
    AM_ERR err;
    AM_INT render_index = cmd.res32_1;
    AM_U16 w,h,x,y;

    w = cmd.res64_1 & 0xffff;
    h = (cmd.res64_1 >> 16) & 0xffff;
    x = (cmd.res64_1 >> 32) & 0xffff;
    y = (cmd.res64_1 >> 48) & 0xffff;
    //AM_ERROR("w %d, h %d, x %d, y %d\n", w, h, x, y);
    err = mpIAV->PlaybackZoom(render_index, w, h, x, y);
    return err;
}

//==============================================================
//
//CAudioRenderOut
// A Mutex device(so assume the info of audio is same)
// dynamic change audio info is next stage.
//==============================================================
//Only one handle to audio device
IAudioHAL* CAudioRenderOut::mpAudioOut = NULL;
AM_BOOL CAudioRenderOut::mbOpened = AM_FALSE;

IRenderOut* CAudioRenderOut::Create(CGConfig* pConfig, AM_INT index)
{
    CAudioRenderOut* result = new CAudioRenderOut(pConfig, index);
    if (result && result->Construct() != ME_OK)
    {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CAudioRenderOut::Construct()
{
//    AM_ERR err;
    //todo
    return ME_OK;
}

void CAudioRenderOut::Delete()
{
    AMLOG_DESTRUCTOR("CAudioRenderOut::Delete().\n");
    //mpAudioOut = NULL;
    //mbOpened = AM_FALSE;
    //AM_DELETE(fw);
    inherited::Delete();
}

CAudioRenderOut::~CAudioRenderOut()
{
    AMLOG_DESTRUCTOR("~CAudioRenderOut.\n");
    AMLOG_DESTRUCTOR("~CAudioRenderOut done.\n");
}

AM_ERR CAudioRenderOut::PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend)
{
    AM_ERR err = ME_OK;
    //AM_ASSERT(isSend == AM_FALSE); no block
    switch(cmd.code)
    {
    case AO::CMD_STOP:
        break;

    case AO::CMD_PAUSE:
        //err = DoPause();
        break;

    default:
        break;
    }
    return err;
}

AM_ERR CAudioRenderOut::DoPause()
{
    AM_ERR err;
    if(mbOpened == AM_FALSE)
    {
        AM_ERROR("Open Audio Hal First when Pause Audio\n");
        return ME_CLOSED;
    }
    err = mpAudioOut->Pause();
    return err;
}

AM_ERR CAudioRenderOut::OnStart()
{
    mpAudioOut->Start();
    return ME_OK;
}

AM_ERR CAudioRenderOut::OpenHal(CGBuffer* pBuffer)
{
    if(mpAudioOut == NULL){
        mpAudioOut = mpGConfig->audioConfig.audioHandle;
    }
    if(mpAudioOut == NULL)
    {
        AM_ERROR("Create AudioOut Failed!!\n");
        return ME_IO_ERROR;
    }

    IAudioHAL::AudioParam audioParam;
    mChannels = pBuffer->audioInfo.numChannels;
    mSamplerate = pBuffer->audioInfo.sampleRate;
    mSampleFormat = pBuffer->audioInfo.sampleFormat;
    AM_INFO("Aduio OpenHal Debug:::%d, %d, %d\n", mChannels, mSampleFormat, mSamplerate);
    if(mSampleFormat != 0 && mSampleFormat != 1)
        AM_ASSERT(0);
    if(mSampleFormat == 0)
        mBytePerFrame = 2;
    else
        mBytePerFrame = 1;

    audioParam.sampleFormat = (IAudioHAL::PCM_FORMAT)mSampleFormat;//((mSampleFormat == 1) ? IAudioHAL::FORMAT_S16_LE: IAudioHAL::FORMAT_U8_LE);
    audioParam.stream = IAudioHAL::STREAM_PLAYBACK;
    audioParam.sampleRate = mSamplerate;
    audioParam.numChannels = mChannels;

    if(mbOpened == AM_TRUE){
        //TODO re adjuct par for hal.
        return ME_OK;
    }
    //TODO dynamic change those info

    if(mpAudioOut->OpenAudioHAL(&audioParam, &mLatency, &mBufferSize) != ME_OK){
        AM_ERROR("CAudioRenderer: mpAudioOut OpenAudioHAL Failed\n");
        return ME_ERROR;
    }
    //start imme
    OnStart();
    mbOpened = AM_TRUE;
    return ME_OK;
}

AM_ERR CAudioRenderOut::RenderOutBuffer(CGBuffer* pBuffer)
{
    //static IFileWriter* fw = NULL;
    AM_ERR err;
    if(mbOpened == AM_FALSE)
    {
        AM_ERROR("Open Audio Hal First when Render Out Audio\n");
        return ME_CLOSED;
    }
    AM_U8* data = (AM_U8* )(pBuffer->GetExtraPtr());
    AM_UINT size = pBuffer->audioInfo.sampleSize;
    AM_UINT num = 0;
    AM_UINT totalSize = size;

    AM_UINT consumeFrame = 0;
    AM_UINT consumeByte = 0;

    while(1)
    {
        err = mpAudioOut->Render(data, size, &num);
        if(err != ME_OK){
            AM_ASSERT(0);
            break;
        }
        consumeFrame += num;
        consumeByte = consumeFrame * mChannels * mBytePerFrame;
        if(consumeByte == totalSize)
            break;

        if(consumeByte > totalSize){
            AM_ASSERT(0);
            AM_INFO("Check Me for audio add silence.\n");
            break;
        }

        if(consumeByte < totalSize){
            data += consumeByte;
            size -= consumeByte;
            continue;
        }
    }
    //AM_INFO("CAudioRenderOut RenderOut Start, %p,(%d, %d, %d, %d), %d, consume num:%d\n", data, data[0], data[1], data[2], data[3], size, num);
/*
    if(fw == NULL){
        if ((fw = CFileWriter::Create()) == NULL)
            AM_ASSERT(0);
        if(fw->CreateFile("dump/a.pcm") != ME_OK)
            AM_ASSERT(0);
    }
    err = fw->WriteFile(data, size);
*/
    mSampleTotal += consumeFrame;
    //AM_INFO("CAudioRenderOut RenderOut Succ\n");
    //delete[] douData;
    return err;
}

