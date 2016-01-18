
/*
 * video_renderer.cpp
 *
 * History:
 *    2010/1/29 - [Oliver Li] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define LOG_NDEBUG 0
#define LOG_TAG "video_renderer"
//#define AMDROID_DEBUG

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

#include "am_pbif.h"
#include "pbif.h"
#include "engine_guids.h"
#include "filter_list.h"

extern "C" {
#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_transcode_drv.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
//#include "vout.h"
}

#include "amdsp_common.h"
#include "am_util.h"
#include "video_renderer.h"

filter_entry g_video_renderer = {
    "VideoRenderer",
    CVideoRenderer::Create,
    NULL,
    CVideoRenderer::AcceptMedia,
};

//debug only
#define _debug_
#ifdef _debug_
static AM_UINT _max_error_count_ = 100;
static AM_UINT _current_error_count_ = 0;
#endif

static bool _in_range(AM_S32 value, AM_UINT range)
{
    if ((value > ((AM_INT)range)) || (value <((AM_INT)(-range)))) {
        return false;
    }
    return true;
}

IFilter* CreateVideoRenderFilter(IEngine *pEngine)
{
    return CVideoRenderer::Create(pEngine);
}

//color test
static void _color_test(AM_U8* py, AM_U8* puv, AM_UINT height, AM_UINT pitch, AM_U32* colors, AM_UINT color_number)
{
    AM_UINT color_height = 1, i = 0, j = 0;
    AM_U8 y, cb, cr;
    AM_U8* p_uv0, *p_uv1;

    AM_ASSERT(height > color_number);

    if (height > color_number) {
        color_height = height/color_number;
        //AM_WARNING("    1    :color_height %d, height %d, color_number %d\n", color_height, height, color_number);

        for (i = 0; i < color_number; i ++) {
            //y
            y = colors[i] >> 24;
            //AM_WARNING("    2y    : i %d, i * color_height * pitch %d, y %08x\n", i,  i * color_height * pitch, y);
            memset(py + i * color_height * pitch, y, color_height * pitch);

            //cb cr
            cb = (colors[i] >> 16) & 0xff;
            cr = (colors[i] >> 8) & 0xff;
            //AM_WARNING("    2uv    : i %d,  i * (color_height/2) * pitch %d, cb %08x, cr %08x\n", i,   i * (color_height/2) * pitch, cb, cr);

            if ((color_height/2) > 0) {
                p_uv0 = puv + i * (color_height/2) * pitch;
                for (j = 0; j < (pitch/2); j ++) {
                    //AM_WARNING("    3uv    : j %d,  p_uv0 %p\n", j, p_uv0);
                    p_uv0[0] = cb;
                    p_uv0[1] = cr;
                    p_uv0 += 2;
                }
            }

            if ((color_height/2) > 1) {
                p_uv0 = puv + i * (color_height/2) * pitch;
                p_uv1 = p_uv0 + pitch;
                for (j = 1; j < (color_height/2); j ++, p_uv1 += pitch) {
                    //AM_WARNING("    3uv1    : j %d,  p_uv0 %p, p_uv1 %p\n", j, p_uv0, p_uv1);
                    memcpy(p_uv1, p_uv0, pitch);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------
//
// CVideoRenderer
//
//-----------------------------------------------------------------------

IFilter* CVideoRenderer::Create(IEngine *pEngine)
{
    CVideoRenderer *result = new CVideoRenderer(pEngine);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

int CVideoRenderer::AcceptMedia(CMediaFormat& format)
{
    if ((*format.pMediaType == GUID_Decoded_Video || *format.pMediaType == GUID_Amba_Decoded_Video) &&
        *format.pSubType == GUID_Video_YUV420NV12)
        return 1;
    return 0;
}

CVideoRenderer::CVideoRenderer(IEngine *pEngine):
    CActiveFilter(pEngine, "VideoRenderer"),
    mpInput(NULL),
    mpFrameBufferPool(NULL),
    mpClockManager(NULL),
    mpBuffer(NULL),

    mIavFd(-1),
    mDspIndex(0),
    mpDSPHandler(NULL),

    mPicWidth(0),
    mPicHeight(0),
    mDisplayWidth(0),
    mDisplayHeight(0),
    mbDisplayRectNeedUpdate(false),

    mbStepMode(false),
    mStepCnt(0),

    mbVideoStarted(false),
    mFirstPTS(0),
    mLastPTS(0),
    mLastSWRenderPTS(0),
    mTimeOffset(0),
    mAdjustInterval(0),
    mRunningMode(RUNNING_MODE_INVALID),
    mEstimatedLatency(0),
    mbRecievedSynccmd(false),
    mbRecievedBegincmd(false),
    mbRecievedUDECRunningcmd(false),
    mbPostAVSynccmd(false),
    mbVoutWaken(false),
    mbVoutPaused(false),
    mbVoutAAR(false),

    mbEOSReached(false),
    mWaitingUdecEOS(0),
    mDebugWaitCount(0),
    mPreciseSyncCheckCount(0),
    mPreciseSyncCheckCountThreshold(8),
    mPreciseSyncAccumulatedDiff(0)
{
    ::memset((void*)mVoutConfig, 0x0, sizeof(mVoutConfig));
    ::memset((void*)mDisplayRectMap, 0x0, sizeof(mDisplayRectMap));

    mAudioClkOffset.decoder_id = 0;
    mAudioClkOffset.audio_clk_offset = 0;
}

AM_ERR CVideoRenderer::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    DSetModuleLogConfig(LogModuleVideoRenderer);
    if ((mIavFd = ::open("/dev/iav", O_RDWR, 0)) < 0) {
        perror("/dev/iav");
        return ME_ERROR;
    }

    //init vout config struct
    for (AM_INT i = 0; i <eVoutCnt; i++) {
        getVoutPrams(mIavFd, i, &mVoutConfig[i]);
    }
    ::close(mIavFd);
    mIavFd = -1;

    if ((mpInput = CVideoRendererInput::Create(this)) == NULL)
        return ME_ERROR;

    return ME_OK;
}

CVideoRenderer::~CVideoRenderer()
{
    AMLOG_DESTRUCTOR("~CVideoRenderer.\n");
    AM_DELETE(mpInput);
//    AM_RELEASE(mpFrameBufferPool);
//    if (mIavFd >= 0)
//    ::close(mIavFd);
//    mIavFd = -1;

    AMLOG_DESTRUCTOR("~CVideoRenderer done.\n");
}

AM_ERR CVideoRenderer::Run()
{
    mpWorkQ->Run();
    return ME_OK;
}

AM_ERR CVideoRenderer::Start()
{
    mpClockManager = (IClockManager *)mpEngine->QueryEngineInterface(IID_IClockManager);
    if(!mpClockManager) {
        AM_ERROR("CVideoRenderer::Run without mpClockManager?\n");
        return ME_ERROR;
    }
/*
//move EnableVoutAAR() to SetInputFormat()
    if(mpSharedRes->pbConfig.ar_enable)
    {
        EnableVoutAAR(1);
    }
*/
    mpWorkQ->Start();
    return ME_OK;
}

void CVideoRenderer::GetInfo(INFO& info)
{
    info.nInput = 1;
    info.nOutput = 0;
    info.mPriority = 5;
    info.mFlags = SYNC_FLAG | RECIEVE_UDEC_RUNNING_FLAG;
    info.mIndex = mDspIndex;
    info.pName = "VideoRenderer";
}

IPin* CVideoRenderer::GetInputPin(AM_UINT index)
{
    if (index == 0)
        return mpInput;
    return NULL;
}

bool CVideoRenderer::ProcessCmd(CMD& cmd)
{
/*    iav_wake_vout_s wake;
    iav_udec_trickplay_t trickplay;
    AM_INT ret = 0;*/
    AMLOG_CMD("****CVideoRenderer::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);

    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            CmdAck(ME_OK);
            break;

        case CMD_TIMERNOTIFY:
            break;

        case CMD_PAUSE:
            AM_ASSERT(!mbPaused);
            mbPaused = true;
            break;

        case CMD_RESUME:
            AM_ASSERT(mbPaused);
            mbPaused = false;
            //resume from step mode?
            if (!mpSharedRes->mbStartWithStepMode) {
                mbStepMode = false;
                mStepCnt = 0;
            }
            break;

        case CMD_FLUSH:
            msResumeState = msState = STATE_PENDING;
            reset();
            AM_RELEASE(mpBuffer);
            mpBuffer = NULL;
            CmdAck(ME_OK);
            break;

        case CMD_START:
            mTimeOffset = mpSharedRes->mPlaybackStartTime;
            CmdAck(ME_OK);
            break;

        case CMD_STEP:
            mbStepMode = true;
            mStepCnt ++;
            break;

        case CMD_AVSYNC:
            AMLOG_INFO("[VR flow] recieve avsync cmd, udecIndex [%d], state %d, get_outpic %d.\n",mDspIndex, msState, mpSharedRes->get_outpic);
            CmdAck(ME_OK);
            AM_ASSERT(!mbRecievedSynccmd);
            mbRecievedSynccmd = true;
            break;

        case CMD_BEGIN_PLAYBACK:
            AM_ASSERT(msState == STATE_PENDING);
            AM_ASSERT(!mpBuffer);

            if (!mpSharedRes->get_outpic) {
                AM_ASSERT(2 == mpSharedRes->ppmode);
                AM_ASSERT(RUNNING_MODE2 == mRunningMode);
                msState = STATE_WAIT_RENDERER_READY_MODE2;
                mRunningMode = RUNNING_MODE2;
            } else if (DEC_HARDWARE == mpSharedRes->decCurrType || 3 == mpSharedRes->ppmode) {
                AM_ASSERT(RUNNING_MODE3 == mRunningMode);
                msState = STATE_RUNNING_MODE3;
                mRunningMode = RUNNING_MODE3;
            } else {
                AM_ASSERT(RUNNING_MODE1 == mRunningMode);
                msState = STATE_IDLE;
                mRunningMode = RUNNING_MODE1;
            }
            msResumeState = msState;
            reset();
            mbRecievedBegincmd = true;
            mTimeOffset = mpSharedRes->mPlaybackStartTime;
            break;

        case CMD_SOURCE_FILTER_BLOCKED:
            break;

        case CMD_UDEC_IN_RUNNING_STATE:
            AM_ASSERT(!mbRecievedUDECRunningcmd);
            mbRecievedUDECRunningcmd = true;
            break;

        default:
            AM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }
    return false;
}

void CVideoRenderer::OnRun()
{
    CMD cmd;
    AMLOG_INFO("VR::OnRun: enter.\n");

    // reply ME_OK to CMD_RUN cmd
    CmdAck(ME_OK);

    mbRun = false;
    mbVideoStarted = false;
    mbRecievedSynccmd = false;
    mWaitingUdecEOS = 0;
    mTimeOffset = mpSharedRes->mPlaybackStartTime;

    mbRun = requestRun();

    //get init state
    if (!mpSharedRes->get_outpic) {
        AM_ASSERT(2 == mpSharedRes->ppmode);
        msState = STATE_WAIT_RENDERER_READY_MODE2;
        mRunningMode = RUNNING_MODE2;
    } else if (DEC_HARDWARE == mpSharedRes->decCurrType || 3 == mpSharedRes->ppmode) {
        msState = STATE_RUNNING_MODE3;
        mRunningMode = RUNNING_MODE3;
    } else {
        msState = STATE_IDLE;
        mRunningMode = RUNNING_MODE1;
    }

    msResumeState = msState;
    if (mbStepMode) {
        msState = STATE_FILTER_STEP_MODE;
    }

    mEstimatedLatency = mpSharedRes->mVideoTicks/2;//to do

    AMLOG_INFO("VR before running, mpSharedRes->ppmode %d, mpSharedRes->get_outpic %d, mpSharedRes->decCurrType %d.\n", mpSharedRes->ppmode, mpSharedRes->get_outpic, mpSharedRes->decCurrType);
    AMLOG_INFO("    init state %d, mode %d, estimated latency %u.\n ", msState, mRunningMode, mEstimatedLatency);

    //state machine:
    //mode1 before sync: IDLE --> STATE_WAIT_RENDERER_READY_MODE1 --> STATE_WAIT_PROPER_START_TIME --> IDLE
    //mode1 running: IDLE --> HAS_INPUT_BUFFER --> IDLE
    //mode1 eos: --> PENDING

    //mode2 before sync: STATE_WAIT_RENDERER_READY_MODE2 --> STATE_WAIT_PROPER_START_TIME -->
    //mode2 running: --> STATE_RUNNING_MODE2
    //mode2 eos: --> STATE_WAIT_EOS_MODE2 --> PENDING

    //mode3 running: --> STATE_RUNNING_MODE3
    //mode3 eos: --> PENDING

    while (mbRun) {

        if(16 == mpSharedRes->mDSPmode){
            if(STATE_PENDING != msState){
                msState = STATE_PENDING;
                AMLOG_INFO("VIDEO_RENDER, State %d, Renderer do nothing in MDEC mode, enter STATE_PENDING.\n", msState);
            }
        }

        AMLOG_STATE("VR::OnRun: start switch, msState=%d, %d input data.\n", msState, mpInput->mpBufferQ->GetDataCnt());
        switch (msState) {
            case STATE_IDLE:
                onStateIdle();
                break;

            case STATE_HAS_INPUTDATA:
                onStateHasInputData();
                break;

            case STATE_WAIT_RENDERER_READY_MODE1:
                onStateWaitRendererReadyMode1();
                break;

            case STATE_WAIT_RENDERER_READY_MODE2:
                onStateWaitRendererReadyMode2();
                break;

            case STATE_WAIT_PROPER_START_TIME:
                onStateWaitProperStartTime();
                break;

            case STATE_RUNNING_MODE2:
                onStateRunningMode2();
                break;

            case STATE_RUNNING_MODE3:
                onStateRunningMode3();
                break;

            case STATE_WAIT_EOS_MODE2:
                onStateWaitEOSMode2();
                break;

            case STATE_READY:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                AM_ASSERT(mpBuffer);
                AM_ASSERT(mpSharedRes->get_outpic);
                AM_ASSERT(1 == mpSharedRes->ppmode||2 == mpSharedRes->ppmode);
                if (CMD_TIMERNOTIFY == cmd.code) {
                    if (mpBuffer) {
                        renderBuffer(mpBuffer);
                    }
                    msState = STATE_IDLE;
                }
                break;

            case STATE_ERROR:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_PENDING:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);

                //can not resume to wrong msResumeState(msResumeState at previous session), stay at pendding when not begin
                if ((CMD_RESUME == cmd.code) && mbRecievedBegincmd) {
                    msState = msResumeState;
                    AMLOG_DEBUG("[checking pending]: mbVoutPaused %d, mbVoutWaken %d.\n", mbVoutPaused, mbVoutWaken);
                    if (mbVoutPaused && mbVoutWaken) {
                        trickPlay(1);
                    }
                    AMLOG_DEBUG("[checking pending] done, mbVoutPaused %d, mbVoutWaken %d.\n", mbVoutPaused, mbVoutWaken);
                }
                break;

            case STATE_FILTER_STEP_MODE:
                onStateStepMode();
                break;

            default:
                AMLOG_ERROR("VR::OnRun: BAD state=%d.\n", msState);
                break;
        }
    }

    if (mpBuffer) {
        AM_ERROR("Buffer not released.\n");
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    AMLOG_INFO("VR::OnRun: exit.\n");
}

void CVideoRenderer::Delete()
{
    //AM_RELEASE(mpFrameBufferPool);
    //mpFrameBufferPool = NULL;
    AMLOG_DESTRUCTOR("CVideoRenderer::Delete().\n");
    AM_DELETE(mpInput);
    mpInput = NULL;

    inherited::Delete();
}

void CVideoRenderer::discardBuffer(CBuffer*& pBuffer)
{
    //to do, just rendering it here for testing
    renderBuffer(pBuffer);
}

void CVideoRenderer::syncWithUDEC(AM_INT no_wait)
{
    AM_S64 diff = 0;
    am_pts_t curTime;
    am_pts_t udecTime;

    AM_UINT udec_state;
    AM_UINT vout_state;
    AM_UINT error_code;

    iav_udec_status_t status;

    //get last udec pts, estimate current udec time
    memset(&status, 0, sizeof(status));
    status.decoder_id = mDspIndex;
    status.nowait = no_wait;
    //AMLOG_INFO("start IAV_IOC_WAIT_UDEC_STATUS\n");
    if ((mRet = ioctl(mIavFd, IAV_IOC_WAIT_UDEC_STATUS, &status)) < 0) {
        AM_PERROR("IAV_IOC_WAIT_UDEC_STATUS");
        AM_ERROR("!!!!!IAV_IOC_WAIT_UDEC_STATUS error, ret %d.\n", mRet);
        if ((-EPERM) == mRet) {
            AM_ASSERT(!no_wait);//no wait must have no udec error
            if (GetUdecState (mIavFd, &udec_state, &vout_state, &error_code)) {
                reportUDECErrorIfNeeded(udec_state, vout_state, error_code);
                msState = STATE_PENDING;
                return;
            }
        } else {
            GetUdecState (mIavFd, &udec_state, &vout_state, &error_code);
            AMLOG_ERROR("error IAV_IOC_WAIT_UDEC_STATUS, udec_state %d, vout state %d, error_code 0x%x.\n", udec_state, vout_state, error_code);
            if (udec_state != IAV_UDEC_STATE_RUN) {
                AMLOG_ERROR("UDEC not in RUN state(%d), goto pending(Video renderer state %d).\n", udec_state, msState);
                msState = STATE_PENDING;
                return;
            }
        }
        return;
    }

    curTime = mpClockManager->GetCurrentTime();
    udecTime = (((am_pts_t)status.pts_high) << 32)|((am_pts_t)status.pts_low);
    diff = curTime - (udecTime + mEstimatedLatency);
    mLastPTS = udecTime;

    if (diff > 20 * mNotSyncThreshold || diff < (-20*mNotSyncThreshold)) {
        AM_ERROR("VR: time gap(%lld) greater than 20*mNotSyncThreshold, should have some problem.\n", diff);
        //how to handle this case?
        return;
    }

    if(mpSharedRes->pbConfig.avsync_enable)
    {
        if (diff > mNotSyncThreshold || diff < (-mNotSyncThreshold)) {
            //sync UDEC here
            AMLOG_WARN("[Sync UDEC]: diff %lld, udec time %llu, current time %llu, accumulated diff %d.\n", diff, udecTime, curTime, mAudioClkOffset.audio_clk_offset);
            mAudioClkOffset.decoder_id = mDspIndex;
            mAudioClkOffset.audio_clk_offset += diff/4;//adjust smoothly

            if (ioctl(mIavFd, IAV_IOC_SET_AUDIO_CLK_OFFSET, &mAudioClkOffset) < 0) {
                AM_PERROR("IAV_IOC_SET_AUDIO_CLK_OFFSET");
                AM_ERROR("error IAV_IOC_SET_AUDIO_CLK_OFFSET\n");
            }
        }
    }
}

void CVideoRenderer::reportUDECErrorIfNeeded(AM_UINT& udec_state, AM_UINT& vout_state, AM_UINT& error_code)
{
    pthread_mutex_lock(&mpSharedRes->mMutex);
    if (mpSharedRes->udec_state != IAV_UDEC_STATE_ERROR && udec_state == IAV_UDEC_STATE_ERROR) {
        mpSharedRes->udec_state = udec_state;
        mpSharedRes->vout_state = vout_state;
        mpSharedRes->error_code = error_code;
        AM_ERROR("UDEC error: post msg, udec state %d, vout state %d, error_code 0x%x.\n", udec_state, vout_state, error_code);
        PostEngineErrorMsg(ME_UDEC_ERROR);
    }
    pthread_mutex_unlock(&mpSharedRes->mMutex);
}

bool CVideoRenderer::waitVoutWaken()
{
    AM_UINT udec_state;
    AM_UINT vout_state;
    AM_UINT error_code;
    GetUdecState (mIavFd, &udec_state, &vout_state, &error_code);
    if (IAV_VOUT_STATE_RUN == vout_state) {
        AM_ASSERT(IAV_UDEC_STATE_RUN == udec_state);
        AMLOG_WARN("vout already waken.\n");
        return true;
    }

    iav_wait_decoder_t wait;
    //wait vout to RUN state
    do {
        wait.decoder_id = mDspIndex;
        wait.flags = IAV_WAIT_VOUT_STATE;
        mRet = ::ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait);
        if (mRet < 0) {
            if ((-EPERM) == mRet) {
                AM_ASSERT("udec error can not happen here\n");
                if (GetUdecState (mIavFd, &udec_state, &vout_state, &error_code)) {
                    reportUDECErrorIfNeeded(udec_state, vout_state, error_code);
                    msState = STATE_ERROR;
                    return false;
                }
            }
            AM_ERROR("how to handle this case? ret %d, wait.flags 0x%x\n", mRet, wait.flags);
            msState = STATE_ERROR;
            return false;
        } else {
            if (IAV_WAIT_VOUT_STATE == wait.flags) {
                GetUdecState (mIavFd, &udec_state, &vout_state, &error_code);
                AMLOG_INFO("VR: vout state changes, current udec state %d, vout state %d.\n", udec_state, vout_state);
                if (IAV_VOUT_STATE_RUN == vout_state) {
                    AM_ASSERT(IAV_UDEC_STATE_RUN == udec_state);
                    AMLOG_INFO("[VR flow]: vout state switched to run....\n");
                    return true;
                }
            } else {
                AM_ASSERT(0);
            }
        }
    } while (1);
    AM_ERROR("why comes here.\n");
    return false;
}

void CVideoRenderer::wakeVout()
{
    iav_wake_vout_t wake;
    wake.decoder_id = mDspIndex;

    if (2 != mpSharedRes->ppmode) {
        AM_ERROR("!!!wake vout when ppmode(%d) !=2, please check code.\n", mpSharedRes->ppmode);
        return;
    }

    AM_ASSERT(!mbVoutWaken);

    AMLOG_INFO("[VR flow cmd]: wake vout start.\n");
    if ((mRet = ioctl(mIavFd, IAV_IOC_WAKE_VOUT, &wake)) < 0) {
        perror("IAV_IOC_WAKE_VOUT");
        AM_ERROR("IAV_IOC_WAKE_VOUT, ret %d.\n", mRet);
        return;
    }
    AMLOG_INFO("[VR flow cmd]: wake vout end.\n");

    mbVoutWaken = waitVoutWaken();
    if(mbVoutWaken)//fix pause-resume-block issue when no seek, 12.02.28, roy mdf
    {
        mbRecievedBegincmd=true;
    }

}

void CVideoRenderer::trickPlay(AM_UINT mode)
{
    if (0 == mode) {
        //pause
        AM_ASSERT(!mbVoutPaused);
        if (mbVoutPaused) {
            AM_ERROR("vout already paused, please check code.\n");
            return;
        }
        mbVoutPaused = true;
    } else if (1 == mode) {
        //resume
        AM_ASSERT(mbVoutPaused);
        if (!mbVoutPaused) {
            AM_ERROR("vout already resumed, please check code.\n");
            return;
        }
        mbVoutPaused = false;
    }

    iav_udec_trickplay_s trickplay;
    trickplay.decoder_id = mDspIndex;
    trickplay.mode = mode;
    AMLOG_INFO("[VR flow cmd]: trickplay mode %d start.\n", mode);
    ::ioctl(mIavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
    AMLOG_INFO("[VR flow cmd]: trickplay mode %d done.\n", mode);
}

void CVideoRenderer::sendSyncInErrorCase()
{
    AMLOG_WARN("[VR flow]: some error case, post sync anyway.\n");
    PostEngineMsg(IEngine::MSG_AVSYNC);
    mbPostAVSynccmd = true;
}

void CVideoRenderer::onStateIdle()
{
    CMD cmd;
    CQueue::QType type;
    CQueueInputPin* pPin;
    CQueue::WaitResult result;

#ifdef AM_DEBUG
    //debug check
    AM_ASSERT(RUNNING_MODE1 == mRunningMode);
    AM_ASSERT(mpSharedRes->get_outpic);
    AM_ASSERT(DEC_HARDWARE != mpSharedRes->decCurrType);
#endif

    //goto pending if paused and vout is started
    if (mbPaused && mbVideoStarted) {
        AM_ASSERT(STATE_IDLE == msState);
        msResumeState = STATE_IDLE;
        msState = STATE_PENDING;
        return;
    }

    //wait input data, process msg
    type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
    if(type == CQueue::Q_MSG) {
        //AM_ASSERT(CMD_TIMERNOTIFY != cmd.code);
        ProcessCmd(cmd);
    } else {
        pPin = (CQueueInputPin*)result.pOwner;
        if (!pPin->PeekBuffer(mpBuffer)) {
            AM_ERROR("VR::onStateIdle: Can't peek buffer\n");
            return;
        }

        if (mpBuffer->GetType() == CBuffer::EOS) {
            AMLOG_INFO("[VR flow]: get EOS buffer.\n");
            handleEOS();
            msState = STATE_IDLE;
            return;
        }

        if (mbStepMode) {
            msState = STATE_FILTER_STEP_MODE;
            return;
        } else if (mbStreamStart) {
            msState = STATE_HAS_INPUTDATA;
            return;
        } else {
            if(!mbStreamStart) {
                mbStreamStart = true;
                mFirstPTS = mpBuffer->mPTS;
                mLastPTS = mFirstPTS;//init value
                mLastSWRenderPTS = mFirstPTS;

                if (mpSharedRes->mbVideoFirstPTSVaild && mpSharedRes->mbVideoStreamStartTimeValid) {
                    AMLOG_PTS("[VR flow]: update mTimeOffset: old=%lld new=%lld.\n", mTimeOffset, mpSharedRes->videoFirstPTS - mpSharedRes->videoStreamStartPTS);
                    // AMLOG_PTS("[VR flow]: update mTimeOffset: videoFirstPTS=%lld videoStreamStartPTS=%lld.\n", mpSharedRes->videoFirstPTS, mpSharedRes->videoStreamStartPTS);
                    mTimeOffset = mpSharedRes->videoFirstPTS - mpSharedRes->videoStreamStartPTS;
                } else {
                    AMLOG_PTS("[VR flow]: update mTimeOffset failure: mbVideoFirstPTSVaild=%d mbVideoStreamStartTimeValid=%d.\n", mpSharedRes->mbVideoFirstPTSVaild, mpSharedRes->mbVideoStreamStartTimeValid);
                }
            }
            AMLOG_INFO("[VR flow]: get first CBuffer, mTimeOffset = %llu, mFirstPTS %llu, mpBuffer->mPTS %llu, mpSharedRes->mPlaybackStartTime %llu.\n", mTimeOffset, mFirstPTS, mpBuffer->mPTS, mpSharedRes->mPlaybackStartTime);
            renderBuffer(mpBuffer);
            msState = STATE_WAIT_RENDERER_READY_MODE1;

#ifdef AM_DEBUG
            //debug check
            AM_ASSERT(mIavFd>=0);
            AM_ASSERT(mpSharedRes);
            AM_ASSERT(mpSharedRes->udec_state == IAV_UDEC_STATE_RUN);
#endif
        }
    }
}

void CVideoRenderer::onStateHasInputData()
{
    AM_ASSERT(mpBuffer);
#ifdef AM_DEBUG
    //debug check
    AM_ASSERT(RUNNING_MODE1 == mRunningMode);
    AM_ASSERT(mpSharedRes->get_outpic);
    AM_ASSERT(DEC_HARDWARE != mpSharedRes->decCurrType);
#endif

    AM_ASSERT(1 == mpSharedRes->ppmode || 2 == mpSharedRes->ppmode);
    am_pts_t curTime = mpClockManager->GetCurrentTime();
    if ((mpBuffer->GetPTS() + mEstimatedLatency + mWaitThreshold) < curTime) {
        renderBuffer(mpBuffer);
        msState = STATE_IDLE;
    } else if (mpBuffer->GetPTS() > (10*mNotSyncThreshold + curTime + mWaitThreshold)) {
        AMLOG_WARN("BAD pts value %llu, curTime %llu?\n", mpBuffer->GetPTS(), curTime);
        renderBuffer(mpBuffer);
        msState = STATE_IDLE;
    } else {
        am_pts_t timer = (mpBuffer->GetPTS() > (AM_U64)mWaitThreshold) ? (mpBuffer->GetPTS() - mWaitThreshold) : 0;//fix bug#2148, mpBuffer->GetPTS() maybe less than mWaitThreshold, roy 12.03.31
        mpClockManager->SetTimer(this, timer);
        msState = STATE_READY;
        return;//ctrl sw/sw-pipeline decoder render speed
    }

    if (!mAdjustInterval) {
        mAdjustInterval = D_AVSYNC_ADJUST_INTERVEL;
        syncWithUDEC(1);
    } else {
        mAdjustInterval --;
    }
}

void CVideoRenderer::onStateWaitRendererReadyMode1()
{
    CMD cmd;
    AM_UINT udec_state;
    AM_UINT vout_state;
    AM_UINT error_code;

    AM_ASSERT(!mbRecievedSynccmd);

    //check if needed
    GetUdecState (mIavFd, &udec_state, &vout_state, &error_code);

    //check udec_state first
    if (IAV_UDEC_STATE_RUN != udec_state) {
        if (!mbRecievedUDECRunningcmd) {
            while (1) {
                AMLOG_INFO("[VR flow]: start wait udec running cmd.\n");
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                if (!mbRun || STATE_WAIT_RENDERER_READY_MODE1 != msState) {
                    AMLOG_WARN("[VR flow]: wait udec running cmd is broken.\n");
                    return;
                }
                if (CMD_UDEC_IN_RUNNING_STATE == cmd.code) {
                    AMLOG_INFO("[VR flow]: wait udec running cmd done.\n");
                    GetUdecState(mIavFd, &udec_state, &vout_state, &error_code);
                    break;
                }
            }
        } else {
            AM_ERROR("recieve udec is running msg, but udec(%d) is not in run state.\n", udec_state);
        }
    }

    if (IAV_UDEC_STATE_RUN != udec_state) {
        AM_ERROR("why comes here, udec is expected as run state.\n");
    }

    AM_ASSERT(IAV_UDEC_STATE_RUN == udec_state || IAV_UDEC_STATE_READY == udec_state);
    AM_ASSERT(IAV_VOUT_STATE_PRE_RUN == vout_state || IAV_VOUT_STATE_DORMANT == vout_state);

    if (IAV_VOUT_STATE_DORMANT == vout_state) {
        AMLOG_WARN("[VR flow]: vout is already dormant state, enter wait proper start time.\n");
        if (!mbPostAVSynccmd) {
            PostEngineMsg(IEngine::MSG_AVSYNC);
            mbPostAVSynccmd = true;
        }
        msState = STATE_WAIT_PROPER_START_TIME;
        return;
    } else {
        AMLOG_INFO("[VR flow]: current udec state %d, vout state %d, need continue waitting.\n", udec_state, vout_state);
    }

    iav_wait_decoder_t wait;
    wait.decoder_id = mDspIndex;
    wait.flags = IAV_WAIT_VOUT_STATE;
    mRet = ::ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait);
    if (mRet < 0) {
        if ((-EPERM) == mRet) {
            AM_ASSERT("udec error can not happen here\n");
            if (GetUdecState (mIavFd, &udec_state, &vout_state, &error_code)) {
                reportUDECErrorIfNeeded(udec_state, vout_state, error_code);
                msState = STATE_PENDING;
                return;
            }
        }
        GetUdecState(mIavFd, &udec_state, &vout_state, &error_code);
        if (IAV_UDEC_STATE_RUN != udec_state) {
            AMLOG_WARN("udec is stopped? exit waiting, udec state %d, vout state %d, error code %d.\n", udec_state, vout_state, error_code);
            msState = STATE_PENDING;
            return;
        }
        AM_ERROR("how to handle this case? ret %d, wait.flags 0x%x\n", mRet, wait.flags);
        return;
    } else {
        if (IAV_WAIT_VOUT_STATE == wait.flags) {
            GetUdecState (mIavFd, &udec_state, &vout_state, &error_code);
            AMLOG_INFO("VR: vout state changes, ori udec state %d, vout state %d, current udec state %d, vout state %d.\n", mpSharedRes->udec_state, mpSharedRes->vout_state, udec_state, vout_state);
            if (IAV_VOUT_STATE_DORMANT == vout_state) {
                AM_ASSERT(IAV_UDEC_STATE_RUN == udec_state);
                if (!mbPostAVSynccmd) {
                    PostEngineMsg(IEngine::MSG_AVSYNC);
                    mbPostAVSynccmd = true;
                }
                msState = STATE_WAIT_PROPER_START_TIME;
                AMLOG_INFO("[VR flow]: vout goes to dormant state, post msg to engine.\n");
            }
        } else {
            AM_ASSERT(0);
        }
    }
}

void CVideoRenderer::onStateWaitRendererReadyMode2()
{
    CMD cmd;
    AM_UINT udec_state;
    AM_UINT vout_state;
    AM_UINT error_code;

    AM_ASSERT(!mbRecievedSynccmd);

    //check if needed
    GetUdecState(mIavFd, &udec_state, &vout_state, &error_code);

    //check udec_state first
    if (IAV_UDEC_STATE_RUN != udec_state) {
        if (!mbRecievedUDECRunningcmd) {
            while (1) {
                AMLOG_INFO("[VR flow]: start wait udec running cmd.\n");
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                if (!mbRun || STATE_WAIT_RENDERER_READY_MODE2 != msState) {
                    AMLOG_WARN("[VR flow]: wait udec running cmd is broken.\n");
                    return;
                }
                if (CMD_UDEC_IN_RUNNING_STATE == cmd.code) {
                    AMLOG_INFO("[VR flow]: wait udec running cmd done.\n");
                    GetUdecState(mIavFd, &udec_state, &vout_state, &error_code);
                    break;
                }
            }
        } else {
            AM_ERROR("recieve udec is running msg, but udec(%d) is not in run state.\n", udec_state);
        }
    }

    AM_ASSERT(IAV_UDEC_STATE_RUN == udec_state || IAV_UDEC_STATE_READY == udec_state);
    AM_ASSERT(IAV_VOUT_STATE_PRE_RUN == vout_state || IAV_VOUT_STATE_DORMANT == vout_state);

    //check vout_state
    if (IAV_VOUT_STATE_DORMANT == vout_state) {
        AM_ASSERT(IAV_UDEC_STATE_RUN == udec_state);
        AMLOG_WARN("[VR flow]: vout is already dormant state, enter wait proper start time.\n");
        if (!mbPostAVSynccmd) {
            PostEngineMsg(IEngine::MSG_AVSYNC);
            mbPostAVSynccmd = true;
        }
        msState = STATE_WAIT_PROPER_START_TIME;
        return;
    } else {
        AMLOG_INFO("[VR flow]: current udec state %d, vout state %d, need continue waitting.\n", udec_state, vout_state);
    }

    iav_wait_decoder_t wait;
    wait.decoder_id = mDspIndex;
    wait.flags = IAV_WAIT_VOUT_STATE | IAV_WAIT_UDEC_EOS | IAV_WAIT_VDSP_INTERRUPT;// | IAV_WAIT_UDEC_ERROR;
    mRet = ::ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait);
    if (mRet < 0) {
        if ((-EPERM) == mRet) {
            AM_ASSERT("udec error happens here\n");
            if (GetUdecState (mIavFd, &udec_state, &vout_state, &error_code)) {
                reportUDECErrorIfNeeded(udec_state, vout_state, error_code);
                msState = STATE_PENDING;
                return;
            }
        }
        GetUdecState(mIavFd, &udec_state, &vout_state, &error_code);
        if (IAV_UDEC_STATE_RUN != udec_state) {
            AMLOG_WARN("udec is stopped? exit waiting, udec state %d, vout state %d, error code %d.\n", udec_state, vout_state, error_code);
            msState = STATE_PENDING;
            return;
        }
        AM_ERROR("how to handle this case? ret %d, wait.flags 0x%x\n", mRet, wait.flags);
#ifdef _debug_
        _current_error_count_ ++;
        if (_current_error_count_ > _max_error_count_) {
            AM_ERROR("max %d iav error reaches, go to error state.\n", _current_error_count_);
            msState = STATE_ERROR;
        }
#endif
        return;
    } else {
        if (IAV_WAIT_VOUT_STATE == wait.flags) {
            GetUdecState (mIavFd, &udec_state, &vout_state, &error_code);
            AMLOG_INFO("[VR flow]: vout state changes, ori udec state %d, vout state %d, current udec state %d, vout state %d.\n", mpSharedRes->udec_state, mpSharedRes->vout_state, udec_state, vout_state);
            if (IAV_VOUT_STATE_DORMANT == vout_state) {
                AM_ASSERT(IAV_UDEC_STATE_RUN == udec_state);
                AMLOG_INFO("[VR flow]: vout goes to dormant state, post msg to engine.\n");
                if (!mbPostAVSynccmd) {
                    PostEngineMsg(IEngine::MSG_AVSYNC);
                    mbPostAVSynccmd = true;
                }
                msState = STATE_WAIT_PROPER_START_TIME;
            }
        } else if (IAV_WAIT_UDEC_EOS == wait.flags) {
            AMLOG_WARN("EOS comes in wait renderer ready, bit-stream should have errors.\n");
            sendSyncInErrorCase();
            PostEngineMsg(IEngine::MSG_EOS);
            msState = STATE_PENDING;
            return;
        } else if (IAV_WAIT_UDEC_ERROR == wait.flags) {
            AMLOG_WARN("get udec error(not Fatal), ignore it, udec state %d, vout state %d, error code %d.\n", udec_state, vout_state, error_code);
        } else if (IAV_WAIT_VDSP_INTERRUPT == wait.flags) {
            AMLOG_INFO("get VDSP interrupt, continue wait, udec state %d, vout state %d, error code %d.\n", udec_state, vout_state, error_code);
            while (mpWorkQ->PeekCmd(cmd)) {
                ProcessCmd(cmd);
                if (CMD_SOURCE_FILTER_BLOCKED == cmd.code) {
                    AMLOG_WARN("Can't wait VR domant state and source filter is blocked, so post AV_SYNC, udec state %d, vout state %d, error code %d.\n", udec_state, vout_state, error_code);
                    sendSyncInErrorCase();
                }
            }
        } else {
            AM_ASSERT(0);
        }
    }
}

void CVideoRenderer::onStateWaitProperStartTime()
{
    AM_ASSERT(!mbRecievedSynccmd);
    CMD cmd;

    if (!mbRecievedSynccmd) {
        //wait sync cmd first
        AMLOG_INFO("[VR flow]: onStateWaitProperTime before wait sync cmd.\n");
        while (1) {
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            if (CMD_AVSYNC == cmd.code) {
                break;
            }
            if (msState != STATE_WAIT_PROPER_START_TIME || !mbRun) {
                AM_ERROR("[VR flow]: change state cmd comes, in waiting proper time, break out, msState %d, mbRun %d.\n", msState, mbRun);
                if (2 == mpSharedRes->ppmode) {
                    wakeVout();
                }
                return;
            }
        }
    }

    am_pts_t curtime = mpClockManager->GetCurrentTime();
    AM_UINT skip = 0;
    if(!IsMaster() && !mpSharedRes->pbConfig.avsync_enable) {
        AMLOG_WARN("[VR flow]: not master, avsync disabled, wake vout immediately.\n");
        skip = 1;
    }else if (!mpSharedRes->mbVideoFirstPTSVaild) {
        AMLOG_WARN("[VR flow]: No valid video first pts, must not comes here, escape from waiting logic.\n");
        skip = 1;
    } else if ((mpSharedRes->videoFirstPTS) < (curtime + mEstimatedLatency + mWaitThreshold)) {
        AMLOG_INFO("[VR flow]: exactly matched time, wake vout immediately, curtime %llu, mpSharedRes->videoFirstPTS %llu.\n", curtime, mpSharedRes->videoFirstPTS);
        skip = 1;
    } else if ((mpSharedRes->videoFirstPTS) > (curtime + 100*mNotSyncThreshold + mEstimatedLatency)) {
        AMLOG_WARN("[VR flow]: mpSharedRes->videoFirstPTS is wrong? gap greater than 100*mNotSyncThreshold.\n");
        skip = 1;
    } else {
        curtime = mpSharedRes->videoFirstPTS - mEstimatedLatency;
    }

    if (skip) {
        if (RUNNING_MODE2 == mRunningMode) {
            wakeVout();
            msState = STATE_RUNNING_MODE2;
        } else if (RUNNING_MODE1 == mRunningMode) {
            if (2 == mpSharedRes->ppmode) {
                wakeVout();
            }
            mbVideoStarted = true;
            msState = STATE_IDLE;
        } else if (RUNNING_MODE3 == mRunningMode) {
            AM_ASSERT(0);
            msState = STATE_RUNNING_MODE3;
        } else {
            AM_ASSERT(0);
        }
        return;
    } else {
        AMLOG_INFO("[VR flow]: set timer %llu, cur time %llu.\n", curtime, mpClockManager->GetCurrentTime());
        mpClockManager->SetTimer(this, curtime);
    }

    AMLOG_INFO("[VR flow]: onStateWaitProperTime wait sync cmd done, wait proper time.\n");
    while (1) {
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        ProcessCmd(cmd);
        if (CMD_TIMERNOTIFY == cmd.code) {
            AMLOG_INFO("[VR flow]: onStateWaitProperTime wait timenotify done, start playback..., mRunningMode %d.\n", mRunningMode);
            if (RUNNING_MODE2 == mRunningMode) {
                wakeVout();
                msState = STATE_RUNNING_MODE2;
            } else if (RUNNING_MODE1 == mRunningMode) {
                if (2 == mpSharedRes->ppmode) {
                    wakeVout();
                }
                mbVideoStarted = true;
                msState = STATE_IDLE;
            } else if (RUNNING_MODE3 == mRunningMode) {
                msState = STATE_RUNNING_MODE3;
            } else {
                AM_ASSERT(0);
            }
            return;
        }
        if (msState != STATE_WAIT_PROPER_START_TIME || !mbRun) {
            AM_ERROR("[VR flow]: change state cmd comes, in waiting proper time, break out, msState %d, mbRun %d.\n", msState, mbRun);
            if (2 == mpSharedRes->ppmode) {
                wakeVout();
            }
            return;
        }
    }
}

void CVideoRenderer::onStateRunningMode2()
{
    CMD cmd;
    iav_udec_status_t status;
    AM_UINT udec_state;
    AM_UINT vout_state;
    AM_UINT error_code;

    AM_S64 diff = 0;
    am_pts_t curTime;
    am_pts_t udecTime;

#ifdef AM_DEBUG
    AM_ASSERT(mbVoutWaken);
#endif

    if(!mbStreamStart) {
        mbStreamStart = true;
        mbVideoStarted = true;

        //wait udec status
        memset(&status, 0, sizeof(status));
        status.decoder_id = mDspIndex;
        status.nowait = 0;

        //AMLOG_INFO("start IAV_IOC_WAIT_UDEC_STATUS\n");
        if ((mRet = ioctl(mIavFd, IAV_IOC_WAIT_UDEC_STATUS, &status)) >= 0)  {
            mFirstPTS = (((am_pts_t)status.pts_high) << 32)|((am_pts_t)status.pts_low);
            mLastPTS = mFirstPTS;//initial value
        } else {
            AM_ERROR("get first udec time fail.\n");
            mFirstPTS = 0;
            mLastPTS = 0;
        }

        if (mpSharedRes->mbVideoFirstPTSVaild && mpSharedRes->mbVideoStreamStartTimeValid) {
            AMLOG_PTS("[VR flow]: update mTimeOffset: old=%lld new=%lld.\n", mTimeOffset, mpSharedRes->videoFirstPTS - mpSharedRes->videoStreamStartPTS);
            // AMLOG_PTS("[VR flow]: update mTimeOffset: videoFirstPTS=%lld videoStreamStartPTS=%lld.\n", mpSharedRes->videoFirstPTS, mpSharedRes->videoStreamStartPTS);
            mTimeOffset = mpSharedRes->videoFirstPTS - mpSharedRes->videoStreamStartPTS;
        } else {
            AMLOG_PTS("[VR flow]: update mTimeOffset failure: mbVideoFirstPTSVaild=%d mbVideoStreamStartTimeValid=%d.\n", mpSharedRes->mbVideoFirstPTSVaild, mpSharedRes->mbVideoStreamStartTimeValid);
        }
    }

    while (1) {

        if (mbStepMode) {
            AMLOG_INFO("enter step mode %d.\n", msState);
            AM_ASSERT(STATE_RUNNING_MODE2 == msState);
            msResumeState = msState;
            msState = STATE_FILTER_STEP_MODE;
            return;
        }

        //ppmode = 2, watch udec/dsp's pts and do avsync issues
        //check if there's cmd
        while (mpWorkQ->PeekCmd(cmd)) {
            ProcessCmd(cmd);
        }

        if (mbPaused) {
            AM_ASSERT(STATE_RUNNING_MODE2 == msState);
            msResumeState = STATE_RUNNING_MODE2;
            AMLOG_DEBUG("[checking mode2]: mbVoutPaused %d, mbVoutWaken %d.\n", mbVoutPaused, mbVoutWaken);
            if (mbVoutWaken && !mbVoutPaused) {
                trickPlay(0);
            }
            AMLOG_DEBUG("[checking mode2 done]: mbVoutPaused %d, mbVoutWaken %d.\n", mbVoutPaused, mbVoutWaken);
            msState = STATE_PENDING;
            return;
        }

        if (STATE_RUNNING_MODE2 != msState || !mbRun) {
            return;
        }

        //wait udec status
        memset(&status, 0, sizeof(status));
        status.decoder_id = mDspIndex;
        status.nowait = 0;

        //AMLOG_INFO("start IAV_IOC_WAIT_UDEC_STATUS\n");
        if ((mRet = ioctl(mIavFd, IAV_IOC_WAIT_UDEC_STATUS, &status)) < 0) {
            AM_PERROR("IAV_IOC_WAIT_UDEC_STATUS");
            AM_ERROR("!!!!!IAV_IOC_WAIT_UDEC_STATUS error, ret %d.\n", mRet);
            if (mRet == (-EPERM)) {
                if (GetUdecState (mIavFd, &udec_state, &vout_state, &error_code)) {
                    reportUDECErrorIfNeeded(udec_state, vout_state, error_code);
                    msState = STATE_PENDING;
                    return;
                }
            } else {
                AM_PERROR("IAV_IOC_WAIT_UDEC_STATUS");
                GetUdecState (mIavFd, &udec_state, &vout_state, &error_code);
                AMLOG_ERROR("error IAV_IOC_WAIT_UDEC_STATUS, udec_state %d, vout state %d, error_code %d.\n", udec_state, vout_state, error_code);
                if (udec_state != IAV_UDEC_STATE_RUN) {
                    AMLOG_ERROR("UDEC not in RUN state(%d), goto pending(Video renderer state %d).\n", udec_state, msState);
                    msState = STATE_PENDING;
                    return;
                }
            }
            msState = STATE_PENDING;
            return;
        }
        //AMLOG_INFO("IAV_IOC_WAIT_UDEC_STATUS done.\n");

        if (status.eos_flag) {
            //In STATE_PPMODE2_AVSYNC state, if get status.eos_flag, peek buffer.
            if (mpInput->PeekBuffer(mpBuffer)) {
                AM_ASSERT(mpBuffer);
                AM_ASSERT(mpBuffer->GetType() == CBuffer::EOS);
                //If EOS sent by decoder filter, no data can be decoded by dsp, stop video stream directly.
                if (mpBuffer->GetType() == CBuffer::EOS) {
                    mbEOSReached = true;
                    mStatusEOS = status;
                    AMLOG_INFO("STATE_MODE2_AVSYNC, !!!!Get udec eos flag(ppmode2), send data done.\n");
                    AMLOG_INFO("STATE_MODE2_AVSYNC, **get last udec pts l %d h %d.\n", status.pts_low, status.pts_high);
                    msState = STATE_WAIT_EOS_MODE2;
                    mWaitingUdecEOS = 1;
                    mpBuffer->Release();
                    mpBuffer = NULL;
                } else {
                    AM_ERROR("!!!Why comes here.\n");
                    msState = STATE_WAIT_EOS_MODE2;
                    renderBuffer(mpBuffer);
                }
                return;
            } else {
                AM_ERROR("no EOS buffer comes, previous filter blocked?\n");
                //If no EOS sent, this should be a seq-eos in the mid of stream, ignore it.
                msState = STATE_WAIT_EOS_MODE2;
                return;
            }
        } else {
            curTime = mpClockManager->GetCurrentTime();
            udecTime = (((am_pts_t)status.pts_high) << 32)|((am_pts_t)status.pts_low);
            diff = curTime - (udecTime + mEstimatedLatency);
            mLastPTS = udecTime;
            if(mpSharedRes->pbConfig.avsync_enable)
            {
                //sync if needed
                if (!mAdjustInterval) {
                    mAdjustInterval = D_AVSYNC_ADJUST_INTERVEL;

                    if (diff > 20 * mNotSyncThreshold || diff < (-20*mNotSyncThreshold)) {
                        AM_ERROR("VR: time gap(%lld) greater than 20*mNotSyncThreshold, should have some problem.\n", diff);
                        //how to handle this case?
                        return;
                    }

                    //rough sync, make sure diff < mNotSyncThreshold
                    if (diff > mNotSyncThreshold || diff < (-mNotSyncThreshold)) {
                        //sync UDEC here
                        AMLOG_INFO("[Sync UDEC]: diff %lld, udec time %llu, current time %llu, accumulated diff %d.\n", diff, udecTime, curTime, mAudioClkOffset.audio_clk_offset);
                        mAudioClkOffset.decoder_id = mDspIndex;
                        mAudioClkOffset.audio_clk_offset += diff/4;//adjust smoothly

                        if (ioctl(mIavFd, IAV_IOC_SET_AUDIO_CLK_OFFSET, &mAudioClkOffset) < 0) {
                            AM_PERROR("IAV_IOC_SET_AUDIO_CLK_OFFSET");
                            AM_ERROR("error IAV_IOC_SET_AUDIO_CLK_OFFSET\n");
                        }

                        //reset after rough sync
                        mPreciseSyncAccumulatedDiff = 0;
                        mPreciseSyncCheckCount = 0;
                    } else {
                        //more precise check, make sure diff < (mWaitThreshold*2/3)
                        mPreciseSyncAccumulatedDiff += diff;
                        AMLOG_PTS("[Sync precisely]: sync count %d reached %d]: diff %lld, udec time %llu, current time %llu, accumulated diff %d, precise accumulated diff %d.\n", mPreciseSyncCheckCount, mPreciseSyncCheckCountThreshold, diff, udecTime, curTime, mAudioClkOffset.audio_clk_offset, mPreciseSyncAccumulatedDiff);
                        if (mPreciseSyncCheckCount >= mPreciseSyncCheckCountThreshold) {
                            AM_ASSERT(mPreciseSyncCheckCount);
                            diff = mPreciseSyncAccumulatedDiff/mPreciseSyncCheckCount;
                            if (_in_range(diff, mNotSyncThreshold)) {
                                if (false == _in_range(diff, mWaitThreshold*2/3)) {
                                    AMLOG_WARN("[Sync precisely]: avg diff %lld, udec time %llu, current time %llu, accumulated diff %d, precise accumulated diff %d, sync count %d.\n", diff, udecTime, curTime, mAudioClkOffset.audio_clk_offset, mPreciseSyncAccumulatedDiff, mPreciseSyncCheckCount);
                                    mAudioClkOffset.decoder_id = mDspIndex;
                                    mAudioClkOffset.audio_clk_offset += diff;
                                    if (ioctl(mIavFd, IAV_IOC_SET_AUDIO_CLK_OFFSET, &mAudioClkOffset) < 0) {
                                        AM_PERROR("IAV_IOC_SET_AUDIO_CLK_OFFSET");
                                        AM_ERROR("error IAV_IOC_SET_AUDIO_CLK_OFFSET\n");
                                    }
                                } else {
                                    AMLOG_INFO("[Sync precisely]: video sync to system clock closely, good, avg diff %lld!\n", diff);
                                }
                            } else {
                                AMLOG_WARN("[Sync precisely]: avg diff(%lld) greater than mNotSyncThreshold(%lld), please check code.\n", diff, mNotSyncThreshold);
                            }

                            mPreciseSyncCheckCount = 0;
                            mPreciseSyncAccumulatedDiff = 0;
                        } else {
                            mPreciseSyncCheckCount++;
                        }
                    }
                } else {
                    mAdjustInterval --;
                }
                AMLOG_PTS("[VR PTS]: udectime=%llu  curTime=%llu, diff %lld, mAdjustInterval %d.\n", udecTime, curTime, -diff, mAdjustInterval);
            }
        }
    }

}

void CVideoRenderer::onStateRunningMode3()
{
    CMD cmd;
    iav_frame_buffer_t frame;
    iav_transc_frame_t Trscframe;

    while (1) {

        if (mbStepMode) {
            AMLOG_INFO("enter step mode %d.\n", msState);
            msState = STATE_FILTER_STEP_MODE;
            return;
        }

        //ppmode = 2, watch udec/dsp's pts and do avsync issues
        //check if there's cmd
        while (mpWorkQ->PeekCmd(cmd)) {
            ProcessCmd(cmd);
        }

        if (mbPaused) {
            AM_ASSERT(STATE_RUNNING_MODE3 == msState);
            msResumeState = STATE_RUNNING_MODE3;
            AMLOG_DEBUG("[checking mode3]: mbVoutPaused %d, mbVoutWaken %d.\n", mbVoutPaused, mbVoutWaken);
            if (mbVoutWaken && !mbVoutPaused) {
                trickPlay(0);
            }
            AMLOG_DEBUG("[checking mode3 done]: mbVoutPaused %d, mbVoutWaken %d.\n", mbVoutPaused, mbVoutWaken);
            msState = STATE_PENDING;
            return;
        }

        if (STATE_RUNNING_MODE3 != msState || !mbRun) {
            return;
        }

        if (3 == mpSharedRes->mDSPmode) {
            mRet = getOutPicTransc(Trscframe);
            if (1 == mRet) {
                AM_ASSERT(!mpBuffer);
                if (mpInput->PeekBuffer(mpBuffer)) {
                    AM_ASSERT(CBuffer::EOS == mpBuffer->GetType());
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
                handleEOS();
                msState = STATE_PENDING;
                return;
            }
            Trscframe.flags = IAV_FRAME_SYNC_VOUT;
            if (ioctl(mIavFd, IAV_IOC_RENDER_FRAME2, &Trscframe) < 0) {
                AM_PERROR("IAV_IOC_RENDER_FRAME2");
            }
        } else {
            getOutPic(frame);
            if (1 == mRet) {
                AM_ASSERT(!mpBuffer);
                if (mpInput->PeekBuffer(mpBuffer)) {
                    AM_ASSERT(CBuffer::EOS == mpBuffer->GetType());
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
                handleEOS();
                msState = STATE_PENDING;
                return;
            }
            frame.flags = IAV_FRAME_SYNC_VOUT;
            if (ioctl(mIavFd, IAV_IOC_RENDER_FRAME, &frame) < 0) {
                AM_PERROR("IAV_IOC_RENDER_FRAME");
            }
        }
    }

}

void CVideoRenderer::onStateWaitEOSMode2()
{
    CMD cmd;
    AM_ASSERT(mbVoutWaken);
    iav_udec_status_t status;
    iav_udec_status_t status_60;

    AM_UINT udec_state;
    AM_UINT vout_state;
    AM_UINT error_code;

    AM_S64 diff = 0;
    am_pts_t curTime;
    am_pts_t udecTime;

    AM_ASSERT(!mpBuffer);
    AM_ASSERT(mWaitingUdecEOS);
    AM_ASSERT(mbVideoStarted);
    //AM_ASSERT(mbEOSReached);
    //ppmode = 2, wait udec eos

    memset(&status_60, 0x0, sizeof(status_60));

    while (1) {

        if (mbStepMode) {
            AMLOG_INFO("enter step mode %d.\n", msState);
            msState = STATE_FILTER_STEP_MODE;
            return;
        }

        //ppmode = 2, watch udec/dsp's pts and do avsync issues
        //check if there's cmd
        while (mpWorkQ->PeekCmd(cmd)) {
            ProcessCmd(cmd);
        }

        if (mbPaused) {
            AM_ASSERT(STATE_WAIT_EOS_MODE2 == msState);
            msResumeState = STATE_WAIT_EOS_MODE2;
            AMLOG_DEBUG("[checking mode2 eos]: mbVoutPaused %d, mbVoutWaken %d.\n", mbVoutPaused, mbVoutWaken);
            if (mbVoutWaken && !mbVoutPaused) {
                trickPlay(0);
            }
            AMLOG_DEBUG("[checking mode2 eos done]: mbVoutPaused %d, mbVoutWaken %d.\n", mbVoutPaused, mbVoutWaken);
            msState = STATE_PENDING;
            return;
        }

        if (STATE_WAIT_EOS_MODE2!= msState || !mbRun) {
            return;
        }

        //wait udec status
        memset(&status, 0, sizeof(status));
        status.decoder_id = mDspIndex;
        status.nowait = 0;

        //AMLOG_INFO("start IAV_IOC_WAIT_UDEC_STATUS\n");
        if ((mRet = ioctl(mIavFd, IAV_IOC_WAIT_UDEC_STATUS, &status)) < 0) {
            AM_PERROR("IAV_IOC_WAIT_UDEC_STATUS");
            AM_ERROR("!!!!!IAV_IOC_WAIT_UDEC_STATUS error, ret %d.\n", mRet);
            if (mRet == (-EPERM)) {
                if (GetUdecState (mIavFd, &udec_state, &vout_state, &error_code)) {
                    reportUDECErrorIfNeeded(udec_state, vout_state, error_code);
                    msState = STATE_PENDING;
                    return;
                }
            } else {
                AM_PERROR("IAV_IOC_WAIT_UDEC_STATUS");
                GetUdecState (mIavFd, &udec_state, &vout_state, &error_code);
                AMLOG_ERROR("error IAV_IOC_WAIT_UDEC_STATUS, udec_state %d, vout state %d, error_code %d.\n", udec_state, vout_state, error_code);
                if (udec_state != IAV_UDEC_STATE_RUN) {
                    AMLOG_ERROR("UDEC not in RUN state(%d), goto pending(Video renderer state %d).\n", udec_state, msState);
                    msState = STATE_PENDING;
                    return;
                }
            }
            handleEOS();
            msState = STATE_PENDING;
            return;
        }
        //AMLOG_INFO("IAV_IOC_WAIT_UDEC_STATUS done.\n");

        if (status.eos_flag) {
            mbEOSReached = true;
            mStatusEOS = status;
            AMLOG_INFO("STATE_PPMODE2_WAIT_EOS, !!!!Get udec eos flag(ppmode2), send data done.\n");
            AMLOG_INFO("STATE_PPMODE2_WAIT_EOS, **get last udec pts l %d h %d.\n", status.pts_low, status.pts_high);
            mDebugWaitCount = 0;
        }
        if (mbEOSReached && status.eos_flag == 0 &&
            mStatusEOS.pts_low == status.pts_low && mStatusEOS.pts_high == status.pts_high) {
            AMLOG_INFO("STATE_PPMODE2_WAIT_EOS, !!Get last ptsl %d h %d, eos comes.\n", status.pts_low, status.pts_high);
            handleEOS();
            msState = STATE_PENDING;
            return;
        } else {
            //computing current time
            udecTime = (((am_pts_t)status.pts_high) << 32)|((am_pts_t)status.pts_low);
            mLastPTS = udecTime;
            curTime = mpClockManager->GetCurrentTime();
            diff = curTime - (udecTime + mEstimatedLatency);
            AMLOG_PTS("   AM VRender: get udec time=%llu , curTime =%llu , diff=%lld .\n", udecTime, curTime, -diff);
            AMLOG_INFO("STATE_PPMODE2_WAIT_EOS, **get udec pts l %d h %d.\n", status.pts_low, status.pts_high);
            mDebugWaitCount++;
            if (mDebugWaitCount > 50) {
                if (((status.pts_low + 3500) >= mStatusEOS.pts_low) && (mStatusEOS.pts_high == status.pts_high)) {
                    AMLOG_WARN("here 1, last pts %d, %d\n", status.pts_low, status.pts_high);
                    handleEOS();
                    msState = STATE_PENDING;
                    return;
                }
                if (status_60.pts_low == status.pts_low) {
                    AMLOG_WARN("return eos here, mDebugWaitCount %d\n", mDebugWaitCount);
                } else if (mDebugWaitCount < 60) {
                    continue;
                }
                handleEOS();
                msState = STATE_PENDING;
                return;
            } else if (mDebugWaitCount > 40) {
                if (((status.pts_low + 3500) >= mStatusEOS.pts_low) && (mStatusEOS.pts_high == status.pts_high)) {
                    AMLOG_WARN("here 2, last pts %d, %d\n", status.pts_low, status.pts_high);
                    handleEOS();
                    msState = STATE_PENDING;
                    return;
                }
                status_60 = status;
                AMLOG_WARN("here, last pts %d, %d\n", status.pts_low, status.pts_high);
            }
        }
    }
}

void CVideoRenderer::onStateStepMode()
{
    CMD cmd;
    CQueue::QType type;
    CQueueInputPin* pPin;
    CQueue::WaitResult result;

    if (mpSharedRes->get_outpic) {
        if (mStepCnt) {
            //wait input data, process msg
            type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
            if(type == CQueue::Q_MSG) {
                //AM_ASSERT(CMD_TIMERNOTIFY != cmd.code);
                ProcessCmd(cmd);
            } else {
                pPin = (CQueueInputPin*)result.pOwner;
                if (!pPin->PeekBuffer(mpBuffer)) {
                    AM_ERROR("No buffer?\n");
                    mbStepMode = false;
                    msState = STATE_ERROR;
                    return;
                }
                if (mpBuffer->GetType() == CBuffer::EOS) {
                    AM_RELEASE(mpBuffer);
                    mpBuffer = NULL;
                    PostEngineMsg(IEngine::MSG_EOS);
                    msState = STATE_PENDING;
                    AM_INFO("Amba Renderer Get EOS!\n");
                    mbStreamStart = false;
                    reset();
                    return;
                }
                renderBuffer(mpBuffer);
                mStepCnt -- ;
                mpBuffer = NULL;
            }
        }else {
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
        }
    } else {
        iav_udec_trickplay_t trickplay;
        trickplay.decoder_id = mDspIndex;
        trickplay.mode = 2; //step
        AM_INT mRet = 0;

        if (mStepCnt) {
            AMLOG_INFO("****VideoRenderer, STEP: start IAV_IOC_UDEC_TRICKPLAY.\n");
            mRet = ioctl(mIavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
            AMLOG_INFO("****VideoDecoder, STEP: IAV_IOC_UDEC_TRICKPLAY done, ret %d.\n", mRet);
            mStepCnt -- ;
            if (mpWorkQ->PeekCmd(cmd)) {
                ProcessCmd(cmd);
                return;
            }
        } else {
            //wait input data, process msg
            type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
            if(type == CQueue::Q_MSG) {
                //AM_ASSERT(CMD_TIMERNOTIFY != cmd.code);
                ProcessCmd(cmd);
            } else {
                pPin = (CQueueInputPin*)result.pOwner;
                if (!pPin->PeekBuffer(mpBuffer)) {
                    AM_ERROR("No buffer?\n");
                    mbStepMode = false;
                    msState = STATE_ERROR;
                    return;
                }
                if (mpBuffer->GetType() == CBuffer::EOS) {
                    AM_RELEASE(mpBuffer);
                    mpBuffer = NULL;
                    //PostEngineMsg(IEngine::MSG_EOS);
                    //msState = STATE_PENDING;
                    AM_INFO("Amba Renderer Get EOS!\n");
                    reset();
                    return;
                }
            }
        }
    }
}
#if 0
//simple check for wrong pts, to do
static bool _not_reasonable_pts(am_pts_t last_pts, am_pts_t cur_pts, AM_UINT expected_gap)
{
    AM_S64 diff = cur_pts - last_pts;
    AM_S64 gap_max = (AM_S64)(expected_gap + (expected_gap>>2));
    AM_S64 gap_min = (AM_S64)(expected_gap - (expected_gap>>2));
    if (diff < gap_min || diff > gap_max) {
        return true;
    }
    return false;
}
#endif
void CVideoRenderer::reset()
{
    mbRecievedSynccmd = false;
    mbRecievedBegincmd = false;
    mbRecievedUDECRunningcmd = false;
    mbPostAVSynccmd = false;
    mbStreamStart = false;
    mbVideoStarted = false;
    mbVoutWaken = false;
    mWaitingUdecEOS = 0;
    mbVoutPaused = false;
    mbStepMode = false;
    mStepCnt = 0;
    mAudioClkOffset.audio_clk_offset = 0;
    mbEOSReached = false;

    mPreciseSyncCheckCount = 0;
    mPreciseSyncAccumulatedDiff = 0;
}

void CVideoRenderer::handleEOS()
{
    AMLOG_INFO("VR: handleEOS.\n");

    PostEngineMsg(IEngine::MSG_EOS);
    AM_RELEASE(mpBuffer);
    mpBuffer = NULL;

    reset();
}

AM_INT CVideoRenderer::getOutPic(iav_frame_buffer_t& frame)
{
    memset(&frame, 0, sizeof(frame));
    frame.flags = 0;
    frame.decoder_id = mDspIndex;

    if (ioctl(mIavFd, IAV_IOC_GET_DECODED_FRAME, &frame) < 0) {
        AM_PERROR("IAV_IOC_GET_DECODED_FRAME");
        return -1;
    }

    if (frame.fb_id == IAV_INVALID_FB_ID) {
        if (frame.eos_flag) {
            return 1;
        }
    }
    return 0;
}

AM_INT CVideoRenderer::getOutPicTransc(iav_transc_frame_t& frame)
{
    memset(&frame, 0, sizeof(frame));
    frame.flags = 0;
    frame.decoder_id = mDspIndex;

    if (ioctl(mIavFd, IAV_IOC_GET_DECODED_FRAME2, &frame) < 0) {
        AM_PERROR("IAV_IOC_GET_DECODED_FRAME2");
        return -1;
    }

    if (frame.fb_id == IAV_INVALID_FB_ID) {
        if (frame.eos_flag) {
            return 1;
        }
    }
    AMLOG_DEBUG("CVideoRenderer::getOutPicTransc %d.\n",frame.fb_id);
    return 0;
}

void CVideoRenderer::renderBuffer(CBuffer*& pBuffer)
{
    CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pBuffer;
    //AMLOG_WARN("before mLastSWRenderPTS %llu,mpSharedRes->mVideoTicks %llu, pBuffer->mPTS %llu.\n", mLastSWRenderPTS, mpSharedRes->mVideoTicks, pBuffer->mPTS);
    //AM_INFO("*****render buffer %p, %p, buf_id %d, real id %d.\n", pBuffer, pVideoBuffer, pVideoBuffer->buffer_id, pVideoBuffer->real_buffer_id);
    if (((AM_S64)pBuffer->mPTS) <= 0 /* || (mpSharedRes->mVideoCurrentPTS && _not_reasonable_pts(mpSharedRes->mVideoCurrentPTS, pBuffer->mPTS, mpSharedRes->mVideoTicks))*/) {
        //no pts or wrong pts, generate a reasonable pts
        mLastSWRenderPTS += mpSharedRes->mVideoTicks;
        pBuffer->mPTS = mLastSWRenderPTS;
    } else {
        mLastSWRenderPTS = pBuffer->mPTS;
    }
    //AMLOG_WARN("before mLastSWRenderPTS %llu,mpSharedRes->mVideoTicks %llu, pBuffer->mPTS %llu.\n", mLastSWRenderPTS, mpSharedRes->mVideoTicks, pBuffer->mPTS);

    //reset video input center if the source is multiresolution
    //SetInputCenter is not ready for ppmode=1
    if(2 == mpSharedRes->ppmode){
        if(pVideoBuffer->picWidth != mPicWidth || pVideoBuffer->picHeight != mPicHeight){
            mPicWidth = pVideoBuffer->picWidth;
            mDisplayRectMap[0].rect[0].w = mPicWidth;
            mPicHeight = pVideoBuffer->picHeight;
            mDisplayRectMap[0].rect[0].h = mPicHeight;
            AM_INT ret;
            ret = SetInputCenter(mPicWidth, mPicHeight);
            if(ret != 0){
                AM_ERROR("reset video input center failed!\n");
            }
            //AM_INFO("mPicWidth :%dx%d\n",mPicWidth,mPicHeight);
        }
    }

    if (2 == mpSharedRes->ppmode) {
        if(mLastSWRenderPTS > mLastPTS)//fix bug#2296, for sw decoding, mLastSWRenderPTS maybe not always increase
        {
            mLastPTS = mLastSWRenderPTS;
        }
        AMLOG_PTS("    rendering pts %llu.\n", mLastPTS);
    } else {
        AMLOG_PTS("    rendering pts [udec %d] %llu.\n",mDspIndex, pBuffer->mPTS);
    }
    renderBufferI1(mIavFd,mDspIndex, pVideoBuffer, &mDisplayRectMap[0].rect[0], mpSharedRes->ppmode);
    AM_RELEASE(pBuffer);
    pBuffer = NULL;
}

void CVideoRenderer::renderBufferI1(int iavFd, int DspIndex, CVideoBuffer *pVideoBuffer, SRect* srcRect, AM_UINT mode)
{
    AM_ASSERT(pVideoBuffer);
    AM_ASSERT(srcRect);
    iav_frame_buffer_t frame;
    int ret=0;
    memset(&frame, 0, sizeof(frame));
    if(1 == mode)
        frame.flags = IAV_FRAME_SYNC_VOUT;
    else
        frame.flags = IAV_FRAME_NO_RELEASE;//only display
    frame.fb_id = (u16)pVideoBuffer->buffer_id;
    frame.decoder_id = DspIndex;
    frame.real_fb_id = (u16)pVideoBuffer->real_buffer_id;

    //source rect
    frame.pic_width = srcRect->w;
    frame.pic_height = srcRect->h;
    frame.lu_off_x = pVideoBuffer->picXoff + srcRect->x;
    frame.lu_off_y = pVideoBuffer->picYoff + srcRect->y;
    frame.ch_off_x = frame.lu_off_x  / 2;
    frame.ch_off_y = frame.lu_off_y / 2;

    frame.buffer_pitch = pVideoBuffer->fbWidth;
    frame.lu_buf_addr = pVideoBuffer->pLumaAddr;
    frame.ch_buf_addr = pVideoBuffer->pChromaAddr;

    //need send pts to ucode
    am_pts_t pts = pVideoBuffer->mPTS;
    frame.pts = (u32)(pts & 0xffffffff);
    frame.pts_high = (u32)(pts>>32);

    if (mpSharedRes->enable_color_test) {
        AM_ASSERT(mpSharedRes->color_test_number <= 32);
        //AMLOG_WARN("mpSharedRes->enable_color_test %d, mpSharedRes->color_test_number %d\n", mpSharedRes->enable_color_test, mpSharedRes->color_test_number);
        _color_test((AM_U8*)frame.lu_buf_addr, (AM_U8*)frame.ch_buf_addr, frame.pic_height, frame.pic_width, mpSharedRes->color_ycbcralpha, mpSharedRes->color_test_number);
        //AMLOG_WARN("memset cbcr frame.buffer_pitch %d, frame.pic_width %d, frame.pic_height %d\n", frame.buffer_pitch, frame.pic_width, frame.pic_height);
        //memset((AM_U8*)frame.ch_buf_addr, 0x128, (frame.pic_width * frame.pic_height)/2);
    }

    if (mode == 2) {
        //AM_INFO("frame.pic_width=%d,frame.pic_height=%d.\n",frame.pic_width,frame.pic_height);
        //AM_INFO("frame.lu_off_x=%d,frame.lu_off_y=%d.\n",frame.lu_off_x,frame.lu_off_y);
        //AM_INFO("frame.ch_off_x=%d,frame.ch_off_y=%d.\n",frame.ch_off_x,frame.ch_off_y);
        AMLOG_DEBUG("IAV_IOC_POSTP_FRAME, fb_id=%d, real_fb_id %d frame.flags %d, pts high %u, low %u.\n",frame.fb_id, frame.real_fb_id, frame.flags, frame.pts_high, frame.pts);
        if ((ret=::ioctl(iavFd, IAV_IOC_POSTP_FRAME, &frame)) < 0) {
            AM_PERROR("IAV_IOC_POSTP_FRAME.\n");
            AM_ERROR("IAV_IOC_POSTP_FRAME fail,fb_id=%d,ret=%d.\n",frame.fb_id,ret);
        }

        pVideoBuffer->flags = 0;
        AMLOG_DEBUG("IAV_IOC_POSTP_FRAME end.\n");
    } else {
        //AM_INFO("frame.pic_width=%d,frame.pic_height=%d.\n",frame.pic_width,frame.pic_height);
        //AM_INFO("frame.lu_off_x=%d,frame.lu_off_y=%d.\n",frame.lu_off_x,frame.lu_off_y);
        //AM_INFO("frame.ch_off_x=%d,frame.ch_off_y=%d.\n",frame.ch_off_x,frame.ch_off_y);
        AMLOG_DEBUG("IAV_IOC_RENDER_FRAME, fb_id=%d.\n",frame.fb_id);
        if ((ret=::ioctl(iavFd, IAV_IOC_RENDER_FRAME, &frame)) < 0) {
            AM_PERROR("IAV_IOC_RENDER_FRAME.\n");
            AM_ERROR("IAV_IOC_RENDER_FRAME[%d] fail,fb_id=%d,ret=%d, pts high %u, low %u.\n",DspIndex,frame.fb_id, ret, frame.pts_high, frame.pts);
        }

        pVideoBuffer->flags = 0;
        AMLOG_DEBUG("IAV_IOC_RENDER_FRAME end.\n");
    }

}

AM_ERR CVideoRenderer::SetInputFormat(CFFMpegMediaFormat *pFormat)
{
#if 1
    if (*pFormat->pMediaType == GUID_Decoded_Video || *pFormat->pMediaType == GUID_Amba_Decoded_Video) {
        AM_ASSERT(mpSharedRes);
        AM_ASSERT(mpSharedRes->mbIavInited == 1);
        mpDSPHandler = NULL;
        mDspIndex = pFormat->mDspIndex;
        mIavFd = mpSharedRes->mIavFd;
        mDisplayRectMap[0].rect[0].x = mDisplayRectMap[0].rect[0].y = 0;
        mPicWidth = mDisplayRectMap[0].rect[0].w = pFormat->picWidth;
        mPicHeight = mDisplayRectMap[0].rect[0].h = pFormat->picHeight;
        for(int vout = 0;vout < 2;vout++){
            mVoutConfig[vout].video_x= mPicWidth;
            mVoutConfig[vout].video_y= mPicHeight;
            mVoutConfig[vout].input_center_x = mPicWidth /2;
            mVoutConfig[vout].input_center_y = mPicHeight /2;
            mVoutConfig[vout].zoom_factor_x = 1;
            mVoutConfig[vout].zoom_factor_y = 1;
        }

        if(mpSharedRes->pbConfig.ar_enable) {
            //EnableVoutAAR(1);
        }
    } else {
        AMLOG_ERROR("Error pFormat->pMediaType in CVideoRenderer::SetInputFormat.\n");
        return ME_ERROR;
    }

#else
    if (*pFormat->pMediaType == GUID_Decoded_Video) {
        //only use mDisplayRectMap[0] as input rect, set default value here
        mDisplayRectMap[0].rect[0].x = mDisplayRectMap[0].rect[0].y = 0;
        mPicWidth = mDisplayRectMap[0].rect[0].w = pFormat->picWidth;
        mPicHeight = mDisplayRectMap[0].rect[0].h = pFormat->picHeight;
        for(int vout = 0;vout < 2;vout++){
            mVoutConfig[vout].video_x= mPicWidth;
            mVoutConfig[vout].video_y= mPicHeight;
            mVoutConfig[vout].input_center_x = mPicWidth /2;
            mVoutConfig[vout].input_center_y = mPicHeight /2;
        }
        mpDSPHandler = (DSPHandler*) pFormat->format;
        AM_ASSERT(mpDSPHandler);
        AM_ASSERT(mpDSPHandler->mfpRenderVideoBuffer);
        mIavFd = mpDSPHandler->mIavFd;
        //if ((mpFrameBufferPool = CIAVVideoBufferPool::Create(mIavFd, pFormat->bufWidth, pFormat->bufHeight, pFormat->picWidth, pFormat->picHeight, pFormat->picWidthWithEdge, pFormat->picHeightWithEdge)) == NULL)
        //    return ME_ERROR;

        //mpInput->SetBufferPool(mpFrameBufferPool);
    } else if (*pFormat->pMediaType == GUID_Amba_Decoded_Video) {
        mIavFd = pFormat->format;
        mPicWidth = pFormat->picWidth;
        mPicHeight = pFormat->picHeight;
        for(int vout = 0;vout < 2;vout++){
            mVoutConfig[vout].video_x= mPicWidth;
            mVoutConfig[vout].video_y= mPicHeight;
            mVoutConfig[vout].input_center_x = mPicWidth /2;
            mVoutConfig[vout].input_center_y = mPicHeight /2;
        }
        AM_ASSERT(!mpDSPHandler);
        mpDSPHandler = NULL;
    } else {
        AMLOG_ERROR("Error pFormat->pMediaType in CVideoRenderer::SetInputFormat.\n");
        return ME_ERROR;
    }
#endif
    return ME_OK;
}

AM_ERR CVideoRenderer::OnTimer(am_pts_t curr_pts)
{
    mpWorkQ->PostMsg(CMD_TIMERNOTIFY);
    return ME_OK;
}

void* CVideoRenderer::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IClockObserver)
        return (IClockObserver*)this;
    else if (refiid == IID_IVideoOutput)
        return (IVideoOutput*)this;
    else if (refiid == IID_IRenderer)
        return (IRender*)this;

    return inherited::GetInterface(refiid);
}

bool CVideoRenderer::requestRun()
{
    AO::CMD cmd;

    //send ready
    AMLOG_INFO("VRenderer: post IEngine::MSG_READY.\n");
    PostEngineMsg(IEngine::MSG_READY);

    while (1) {
        AMLOG_INFO("CVideoRenderer::requestRun...\n");
        GetCmd(cmd);
        if (cmd.code == AO::CMD_START) {
            AMLOG_INFO("CVideoRenderer::requestRun done.\n");
            CmdAck(ME_OK);
            return true;
        }

        if (cmd.code == AO::CMD_STOP) {
            AMLOG_INFO("CVideoRenderer::requestRun stop comes...\n");
            CmdAck(ME_OK);
            return false;
        }

        if (cmd.code == AO::CMD_STEP) {
            mbStepMode = true;
            return false;
        }

        if (cmd.code == AO::CMD_UDEC_IN_RUNNING_STATE) {
            AM_ASSERT(!mbRecievedUDECRunningcmd);
            mbRecievedUDECRunningcmd = true;
        } else {
            AMLOG_ERROR("not handled cmd %d.\n", cmd.code);
        }
    }
}

AM_ERR CVideoRenderer::GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs)
{
    if(!mbVideoStarted) {
        absoluteTimeMs = mTimeOffset/90;//seek point
        relativeTimeMs = 0;
        AMLOG_PTS("videorender:GetCurrentTime not started, %llu, %llu\n", absoluteTimeMs, relativeTimeMs);
        return ME_OK;
    }

    if (mLastPTS >= mFirstPTS) {
        relativeTimeMs = (mLastPTS - mFirstPTS)/90;
    } else {
        AM_ERROR("why comes here, buffer's PTS abnormal? mLastPTS %llu, mFirstPTS %llu.\n", mLastPTS, mFirstPTS);
        relativeTimeMs = mLastPTS/90;
    }

    absoluteTimeMs = relativeTimeMs + mTimeOffset/90;
    //AMLOG_PTS("mLastPTS %llu, mFirstPTS %llu, mTimeOffset %llu, diff %lld.\n", mLastPTS, mFirstPTS, mTimeOffset, mLastPTS - mFirstPTS);
    AMLOG_PTS("videorender:GetCurrentTime absoluteTimeMs = %llu relativeTimeMs=%llu\n", absoluteTimeMs, relativeTimeMs);
    return ME_OK;
}

AM_ERR CVideoRenderer::Step()
{
    mpWorkQ->PostMsg(CMD_STEP);
    return ME_OK;
}

int CVideoRenderer::configVout(AM_INT vout, AM_INT video_rotate, AM_INT video_flip, AM_INT target_pos_x, AM_INT target_pos_y, AM_INT target_width, AM_INT target_height)
{
    iav_udec_vout_configs_t iav_cvs;
    iav_udec_vout_config_t  vout_config;

    memset(&iav_cvs, 0x0, sizeof(iav_cvs));

    iav_cvs.vout_config = &vout_config;
    iav_cvs.udec_id = mDspIndex;
    iav_cvs.num_vout = 1;
    iav_cvs.vout_config->disable = mVoutConfig[vout].enable ? 0 : 1;
    iav_cvs.vout_config->vout_id= (u8)vout;
    iav_cvs.vout_config->rotate = (u8)video_rotate;
    iav_cvs.vout_config->flip = (u8)video_flip;
    iav_cvs.vout_config->zoom_factor_x = mVoutConfig[vout].zoom_factor_x;
    iav_cvs.vout_config->zoom_factor_y = mVoutConfig[vout].zoom_factor_y;
    iav_cvs.input_center_x = mVoutConfig[vout].input_center_x;
    iav_cvs.input_center_y = mVoutConfig[vout].input_center_y;
    iav_cvs.vout_config->win_offset_x = (u16)target_pos_x;
    iav_cvs.vout_config->win_offset_y = (u16)target_pos_y >> mVoutConfig[vout].vout_mode;
    iav_cvs.vout_config->win_width = (u16)target_width;
    iav_cvs.vout_config->win_height = (u16)target_height >> mVoutConfig[vout].vout_mode;
    iav_cvs.vout_config->target_win_offset_x = (u16)target_pos_x;
    iav_cvs.vout_config->target_win_offset_y = (u16)target_pos_y >> mVoutConfig[vout].vout_mode;
    iav_cvs.vout_config->target_win_width = (u16)target_width;
    iav_cvs.vout_config->target_win_height = (u16)target_height >> mVoutConfig[vout].vout_mode;

    AMLOG_INFO("vout:%d,zoom_factor_x = 0x%x,zoom_factor_y = 0x%x,target_width: %d,target_height: %d\n",vout,iav_cvs.vout_config->zoom_factor_x,iav_cvs.vout_config->zoom_factor_y,target_width,target_height);
    return  ioctl(mIavFd, IAV_IOC_UPDATE_VOUT_CONFIG, &iav_cvs);
}

int CVideoRenderer::SetInputCenter(AM_INT pic_x, AM_INT pic_y){
    iav_udec_vout_configs_t iav_cvs;
    iav_udec_vout_config_t  vout_config[2];

    memset(&iav_cvs, 0x0, sizeof(iav_cvs));
    AMLOG_INFO("SetInputCenter:%dx%d\n",pic_x,pic_y);
    iav_cvs.vout_config = &vout_config[0];
    iav_cvs.udec_id = mDspIndex;
    iav_cvs.num_vout = 2;
    iav_cvs.input_center_x = pic_x / 2;
    iav_cvs.input_center_y = pic_y / 2;
    for(AM_INT i = 0;i < (AM_INT)iav_cvs.num_vout; i++){
        iav_cvs.vout_config[i].vout_id= i;
        iav_cvs.vout_config[i].disable = mVoutConfig[i].enable ? 0 : 1;
        iav_cvs.vout_config[i].rotate = (u8)mVoutConfig[i].rotate;
        iav_cvs.vout_config[i].flip = (u8)mVoutConfig[i].flip;
        iav_cvs.vout_config[i].win_offset_x = (u16)mVoutConfig[i].pos_x;
        iav_cvs.vout_config[i].win_offset_y = (u16)mVoutConfig[i].pos_y >> mVoutConfig[i].vout_mode;
        iav_cvs.vout_config[i].win_width = (u16)mVoutConfig[i].size_x;
        iav_cvs.vout_config[i].win_height = (u16)mVoutConfig[i].size_y >> mVoutConfig[i].vout_mode;
        iav_cvs.vout_config[i].target_win_offset_x = (u16)mVoutConfig[i].pos_x;
        iav_cvs.vout_config[i].target_win_offset_y = (u16)mVoutConfig[i].pos_y >> mVoutConfig[i].vout_mode;
        iav_cvs.vout_config[i].target_win_width = (u16)mVoutConfig[i].size_x;
        iav_cvs.vout_config[i].target_win_height = (u16)mVoutConfig[i].size_y >> mVoutConfig[i].vout_mode;

        float ratio_x,ratio_y;
        ratio_x = (float)mVoutConfig[i].size_x / pic_x;
        ratio_y = (float)(mVoutConfig[i].size_y >> mVoutConfig[i].vout_mode)/ pic_y;
        //AM_INFO("ratio_x= %f, %f\n",ratio_x,ratio_y);
        iav_cvs.vout_config[i].zoom_factor_x = (int)(ratio_x * 0x10000);
        iav_cvs.vout_config[i].zoom_factor_y = (int)(ratio_y * 0x10000);

        //save input_center and zoom_factor
        mVoutConfig[i].input_center_x = iav_cvs.input_center_x;
        mVoutConfig[i].input_center_y = iav_cvs.input_center_y;
        mVoutConfig[i].zoom_factor_x = iav_cvs.vout_config[i].zoom_factor_x;
        mVoutConfig[i].zoom_factor_y = iav_cvs.vout_config[i].zoom_factor_y;

        mVoutConfig[i].video_x = pic_x;
        mVoutConfig[i].video_y = pic_y;
    }
    return  ioctl(mIavFd, IAV_IOC_UPDATE_VOUT_CONFIG, &iav_cvs);
}

AM_ERR CVideoRenderer::ChangeInputCenter(AM_INT input_center_x, AM_INT input_center_y)
{
    AM_INFO("ChangeInputCenter: input_center_x %d, input_center_y %d\n",input_center_x,input_center_y);
    if(input_center_x > mVoutConfig[0].input_center_x || input_center_y > mVoutConfig[0].input_center_y){
        AMLOG_ERROR("ChangeInputCenter: the input center out of range\n");
        return ME_BAD_PARAM;
    } else if(input_center_x == mVoutConfig[0].input_center_x && input_center_y == mVoutConfig[0].input_center_y){
        AM_INFO("ChangeInputCenter: same input center, no change\n");
        return ME_OK;
    }
    AM_INT ret;
    ret = SetInputCenter(2*input_center_x, 2*input_center_y);
    if (ret) {
        AMLOG_ERROR("ChangeInputCenter fail. ret %d, input_center_x %d, input_center_y %d .\n", ret, input_center_x, input_center_y);
        return ME_OS_ERROR;
    }

    return ME_OK;
}

AM_ERR CVideoRenderer::SetDisplayPositionSize(AM_INT vout, AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height)
{
//return ME_NO_IMPL;//without libvout
#if 1
    AMLOG_INFO("SetDisplayPositionSize: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, pos_x, pos_y, width, height);
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayPositionSize: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    //check valid
    if ((pos_x + width) > mVoutConfig[vout].width || (pos_y + height) > mVoutConfig[vout].height || (width < 0) || (height < 0) || (pos_x < 0) || (pos_y < 0)) {
        AMLOG_ERROR("SetDisplayPositionSize: pos&size out of range. vout=%d, vout_w=%d, vout_h=%d, pos_x=%d, pos_y=%d, width=%d, height=%d.\n",
            vout,mVoutConfig[vout].width,mVoutConfig[vout].height,pos_x, pos_y, width, height);
        return ME_BAD_PARAM;
    }

    AM_U32 zoom_factor_x, zoom_factor_y;
    zoom_factor_x = mVoutConfig[vout].zoom_factor_x;
    zoom_factor_y = mVoutConfig[vout].zoom_factor_y;

    /*if((!mpSharedRes->dspConfig.enableDeinterlace)&&((mPicWidth&(15)) || (mPicHeight&(15)))){//for NO-HDMI playback hang issue, temporarily mdf for Interlaced files, roy 2012.02.17
        float ratio_x,ratio_y;
        ratio_x = (float)width/ mVoutConfig[vout].video_x;
        ratio_y = (float)(height >> mVoutConfig[vout].vout_mode)/ mVoutConfig[vout].video_y;
        mVoutConfig[vout].zoom_factor_x = (int)(ratio_x * 0x10000);
        mVoutConfig[vout].zoom_factor_y = (int)(ratio_y * 0x10000);
    }*/

    AM_INT ret;
    ret = configVout(vout, mVoutConfig[vout].rotate, mVoutConfig[vout].flip, pos_x, pos_y, width, height);
    if (ret) {
        AMLOG_ERROR("ambarella_vout_change_video_offset fail. ret %d, pos_x %d, pos_y %d .\n", ret, pos_x, pos_y);
        mVoutConfig[vout].zoom_factor_x = zoom_factor_x;
        mVoutConfig[vout].zoom_factor_y = zoom_factor_y;
        return ME_OS_ERROR;
    }
    mVoutConfig[vout].pos_x = pos_x;
    mVoutConfig[vout].pos_y = pos_y;
    mVoutConfig[vout].size_x = width;
    mVoutConfig[vout].size_y = height;

    AMLOG_INFO("SetDisplayPositionSize done: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    return ME_OK;
#endif
}


AM_ERR CVideoRenderer::GetDisplayPositionSize(AM_INT vout, AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("GetDisplayPositionSize: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    *pos_x = mVoutConfig[vout].pos_x;
    *pos_y = mVoutConfig[vout].pos_y;
    *width = mVoutConfig[vout].size_x;
    *height = mVoutConfig[vout].size_y;

    AMLOG_INFO("GetDisplayPositionSize done: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    return ME_OK;
}


AM_ERR CVideoRenderer::SetDisplayPosition(AM_INT vout, AM_INT pos_x, AM_INT pos_y)
{
//return ME_NO_IMPL;//without libvout
#if 1
    AMLOG_INFO("SetDisplayPosition: vout %d, pos_x %d, pos_y %d.\n", vout, pos_x, pos_y);

    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayPosition: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    //check valid
    if ((pos_x + mVoutConfig[vout].size_x) > mVoutConfig[vout].width || (pos_y + mVoutConfig[vout].size_y) > mVoutConfig[vout].height || (pos_x < 0) || (pos_y < 0)) {
        AMLOG_ERROR("SetDisplayPosition: pos out of range.\n");
        return ME_BAD_PARAM;
    }

    AM_INT ret;
    ret = configVout(vout, mVoutConfig[vout].rotate, mVoutConfig[vout].flip, pos_x, pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);
    if (ret) {
        AMLOG_ERROR("ambarella_vout_change_video_offset fail. ret %d, pos_x %d, pos_y %d .\n", ret, pos_x, pos_y);
        return ME_OS_ERROR;
    }
    mVoutConfig[vout].pos_x = pos_x;
    mVoutConfig[vout].pos_y = pos_y;

    AMLOG_INFO("SetDisplayPosition done: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    return ME_OK;
#endif
}

AM_ERR CVideoRenderer::SetDisplaySize(AM_INT vout, AM_INT width, AM_INT height)
{
//return ME_NO_IMPL;//without libvout
#if 1
    AMLOG_INFO("SetDisplaySize: vout %d, x_size %d, y_size %d.\n", vout, width, height);

    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplaySize: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    //check valid
    if ((mVoutConfig[vout].pos_x+ width) > mVoutConfig[vout].width || (mVoutConfig[vout].pos_y + height) > mVoutConfig[vout].height || (width < 0) || (height < 0) ) {
        AMLOG_ERROR("SetDisplaySize: size out of range.\n");
        return ME_BAD_PARAM;
    }

    AM_U32 zoom_factor_x, zoom_factor_y;
    zoom_factor_x = mVoutConfig[vout].zoom_factor_x;
    zoom_factor_y = mVoutConfig[vout].zoom_factor_y;

    /*float ratio_x,ratio_y;
    ratio_x = (float)width/ mVoutConfig[vout].video_x;
    ratio_y = (float)height/ mVoutConfig[vout].video_y;
    mVoutConfig[vout].zoom_factor_x = (int)(ratio_x * 0x10000);
    mVoutConfig[vout].zoom_factor_y = (int)(ratio_y * 0x10000);*/

    AM_INT ret;

    ret = configVout(vout, mVoutConfig[vout].rotate, mVoutConfig[vout].flip, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, width, height);
    if (ret) {
        AMLOG_ERROR("ambarella_vout_change_video_size fail. ret %d, width %d, height %d .\n", ret, width, height);
        mVoutConfig[vout].zoom_factor_x = zoom_factor_x;
        mVoutConfig[vout].zoom_factor_y = zoom_factor_y;
        return ME_OS_ERROR;
    }
    mVoutConfig[vout].size_x = width;
    mVoutConfig[vout].size_y = height;

    AMLOG_INFO("SetDisplaySize done: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    return ME_OK;
#endif
}

AM_ERR CVideoRenderer::GetDisplayDimension(AM_INT vout, AM_INT* width, AM_INT* height)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("GetDisplayDimension: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    *width = mVoutConfig[vout].width;
    *height = mVoutConfig[vout].height;

    return ME_OK;
}

AM_ERR CVideoRenderer::GetVideoPictureSize(AM_INT* width, AM_INT* height)
{
/*    if(mpSharedRes->vid_rotation_info == 90 || mpSharedRes->vid_rotation_info ==270){
        *width = mPicHeight;
        *height = mPicWidth;
    }else{
        *width = mPicWidth;
        *height = mPicHeight;
    }
*/
    //calculate the render size in EnableVoutAAR()
    *width = mDisplayWidth;
    *height = mDisplayHeight;
    AMLOG_INFO("GetVideoPictureSize, get picture width %d, height %d.\n", *width, *height);
    return ME_OK;
}

AM_ERR CVideoRenderer::GetCurrentVideoPictureSize(AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height)
{
    AM_INT vout;
    if(mpSharedRes->pbConfig.vout_config & (1<<eVoutHDMI)){
        vout = 1;
    } else {
        vout = 0;//no HDMI
    }
    *pos_x = mVoutConfig[vout].pos_x;
    *pos_y = mVoutConfig[vout].pos_y;
    *width = mVoutConfig[vout].video_x;
    *height = mVoutConfig[vout].video_y;
    AMLOG_INFO("GetCurrentVideoPictureSize, get current display picture width %d, height %d.\n", *width, *height);
    return ME_OK;
}

AM_ERR CVideoRenderer::VideoDisplayZoom(AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height)
{
    AMLOG_INFO("VideoDisplayZoom: pos_x %d, pos_y %d, x_size %d, y_size %d.\n", pos_x, pos_y, width, height);

    //check valid
    if ((pos_x + width) > (AM_INT)mPicWidth || (pos_y + height) > (AM_INT)mPicHeight || (width < 0) || (height < 0) || (pos_x < 0) || (pos_y < 0)) {
        AMLOG_ERROR("VideoDisplayZoom: pos&size out of range.\n");
        return ME_BAD_PARAM;
    }

    AM_INT vout;
    if(mpSharedRes->pbConfig.vout_config & (1<<eVoutHDMI)){
        vout = 1;
    } else {
        vout = 0;//no HDMI
    }
    AM_INFO("mpSharedRes->pbConfig.vout_config = %d,vout = %d\n",mpSharedRes->pbConfig.vout_config,vout);

    AM_U32 zoom_factor_x, zoom_factor_y;
    zoom_factor_x = mVoutConfig[vout].zoom_factor_x;
    zoom_factor_y = mVoutConfig[vout].zoom_factor_y;

    float ratio_x,ratio_y;
    ratio_x = (float)mVoutConfig[vout].size_x/ width;
    ratio_y = (float)(mVoutConfig[vout].size_y >> mVoutConfig[vout].vout_mode)/ height;
    mVoutConfig[vout].zoom_factor_x = (int)(ratio_x * 0x10000);
    mVoutConfig[vout].zoom_factor_y = (int)(ratio_y * 0x10000);

    AM_INT ret;
    ret = configVout(vout, mVoutConfig[vout].rotate, mVoutConfig[vout].flip, pos_x, pos_y, mVoutConfig[vout].width, mVoutConfig[vout].height);
    if (ret) {
        AMLOG_ERROR("ambarella_vout_change_video_offset fail. ret %d, pos_x %d, pos_y %d .\n", ret, pos_x, pos_y);
        mVoutConfig[vout].zoom_factor_x = zoom_factor_x;
        mVoutConfig[vout].zoom_factor_y = zoom_factor_y;
        return ME_OS_ERROR;
    }
    mVoutConfig[vout].pos_x = pos_x;
    mVoutConfig[vout].pos_y = pos_y;
    mVoutConfig[vout].video_x = width;
    mVoutConfig[vout].video_y = height;

    AMLOG_INFO("VideoDisplayZoom done: vout %d, pos_x %d, pos_y %d, zoom_factor_x 0x%x, zoom_factor_y 0x%x.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].zoom_factor_x, mVoutConfig[vout].zoom_factor_y);

    return ME_OK;

}


/*
AM_ERR CVideoRenderer::GetDisplayRotation(AM_INT vout, AM_INT* degree)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("GetDisplayRotation: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    switch (mVoutConfig[vout].rotationflip) {
        case AMBA_VOUT_FLIP_NORMAL:
            *degree = 0;
            break;
        case AMBA_VOUT_FLIP_NORMAL_ROTATE_90:
            *degree = 90;
            break;

        case AMBA_VOUT_FLIP_VERTICAL:
        case AMBA_VOUT_FLIP_HORIZONTAL:
        case AMBA_VOUT_FLIP_HV:
            AMLOG_INFO("GetDisplayRotation, has flip state %d.\n", mVoutConfig[vout].rotationflip);
            *degree = 0;//set to zero?
            break;
        case AMBA_VOUT_FLIP_VERTICAL_ROTATE_90:
        case AMBA_VOUT_FLIP_HORIZONTAL_ROTATE_90:
        case AMBA_VOUT_FLIP_HV_ROTATE_90:
            AMLOG_INFO("GetDisplayRotation, has flip state %d.\n", mVoutConfig[vout].rotationflip);
            *degree = 90;//set to 90?
            break;
        default:
            AMLOG_ERROR("GetDisplayRotation, error rotation/flip states.\n");
            return ME_ERROR;
    }
    return ME_OK;
}
*/

AM_ERR CVideoRenderer::SetDisplayRotation(AM_INT vout, AM_INT degree)
{
//return ME_NO_IMPL;//without libvout
#if 1
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayRotation: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    AM_INT ret;
    if(degree == mVoutConfig[vout].rotate){
        AMLOG_INFO("same degree %d, need do nothing.\n", degree);
        return ME_OK;
    }
    AMLOG_INFO("SetDisplayRotation degree=%d, mVoutConfig[%d].rotate=%d.\n", degree, vout, mVoutConfig[vout].rotate);
    ret = configVout(vout, degree, mVoutConfig[vout].flip, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    if (!ret) {
        AMLOG_INFO("'SetDisplayRotation(%d) done, ori %d, target %d.\n", vout, mVoutConfig[vout].rotate, degree);
        mVoutConfig[vout].rotate = degree;
        return ME_OK;
    }

    AMLOG_ERROR("'SetDisplayRotation(%d) fail, ret %d, ori %d, target %d.\n", vout, ret, mVoutConfig[vout].rotate, degree);
    return ME_OS_ERROR;
#endif
}

AM_ERR CVideoRenderer::SetDisplayFlip(AM_INT vout, AM_INT flip)
{
//return ME_NO_IMPL;//without libvout
#if 1
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayFlip: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    AMLOG_INFO("'SetDisplayFlip(%d), flip %d, ori %d.\n", vout, flip, mVoutConfig[vout].flip);

    AM_INT ret;
    if(flip == mVoutConfig[vout].flip){
        AMLOG_INFO("need do nothing.\n");
        return ME_OK;
    }

    ret = configVout(vout, mVoutConfig[vout].rotate, flip, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    if (!ret) {
        AMLOG_INFO("'SetDisplayFlip(%d) done, ori %d, target %d.\n", vout, mVoutConfig[vout].flip, flip);
        mVoutConfig[vout].flip = flip;
        return ME_OK;
    }

    AMLOG_ERROR("'SetDisplayFlip(%d) fail, ret %d, ori %d, target %d.\n", vout, ret, mVoutConfig[vout].flip, flip);
    return ME_OS_ERROR;
#endif
}

AM_ERR CVideoRenderer::SetDisplayMirror(AM_INT vout, AM_INT mirror)
{
return ME_NO_IMPL; //without libvout
#if 0
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayFlip: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    AMLOG_INFO("'SetDisplayMirror(%d), flip %d, ori %d.\n", vout, mirror, mVoutConfig[vout].rotationflip);

    AM_INT ret, target;
    if (mirror == 0) {
        switch (mVoutConfig[vout].rotationflip) {
            case AMBA_VOUT_FLIP_HORIZONTAL:
                target = AMBA_VOUT_FLIP_NORMAL;
                break;
            case AMBA_VOUT_FLIP_HV:
                target = AMBA_VOUT_FLIP_VERTICAL;
                break;
            case AMBA_VOUT_FLIP_HORIZONTAL_ROTATE_90:
                target = AMBA_VOUT_FLIP_NORMAL_ROTATE_90;
                break;
            case AMBA_VOUT_FLIP_HV_ROTATE_90:
                target = AMBA_VOUT_FLIP_VERTICAL_ROTATE_90;
                break;
            default:
                AMLOG_INFO("need do nothing.\n");
                return ME_OK;
        }
    } else {
        switch (mVoutConfig[vout].rotationflip) {
            case AMBA_VOUT_FLIP_NORMAL:
                target = AMBA_VOUT_FLIP_HORIZONTAL;
                break;
            case AMBA_VOUT_FLIP_VERTICAL:
                target = AMBA_VOUT_FLIP_HV;
                break;
            case AMBA_VOUT_FLIP_NORMAL_ROTATE_90:
                target = AMBA_VOUT_FLIP_HORIZONTAL_ROTATE_90;
                break;
            case AMBA_VOUT_FLIP_VERTICAL_ROTATE_90:
                target = AMBA_VOUT_FLIP_HV_ROTATE_90;
                break;
            default:
                AMLOG_INFO("need do nothing.\n");
                return ME_OK;
        }
    }

    ret = ambarella_vout_flip_video((u32)vout, (amba_vout_flipate_info)target);

    if (!ret) {
        AMLOG_INFO("'SetDisplayMirror(%d) done, ori %d, target %d.\n", vout, mVoutConfig[vout].rotationflip, target);
        mVoutConfig[vout].rotationflip = target;
        return ME_OK;
    }

    AMLOG_ERROR("'SetDisplayMirror(%d) fail, ret %d, ori %d, target %d.\n", vout, ret, mVoutConfig[vout].rotationflip, target);
    return ME_OS_ERROR;
#endif
}

AM_ERR CVideoRenderer::EnableVout(AM_INT vout, AM_INT enable)
{
//return ME_NO_IMPL;//without libvout
#if 1
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("EnableVout: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    iav_vout_enable_video_t     iav_enable_video;

    iav_enable_video.vout_id = vout ? 1 : 0;
    iav_enable_video.video_en = enable ? 1 : 0;

    AM_INT ret =ioctl(mIavFd, IAV_IOC_VOUT_ENABLE_VIDEO, &iav_enable_video);
    if (ret) {
        AMLOG_ERROR("ambarella_vout_enable_video(%d, %d), fail ret = %d.\n", vout, enable, ret);
        return ME_OS_ERROR;
    }
    AM_INT origenable = mVoutConfig[vout].enable;
    mVoutConfig[vout].enable = enable ? 1 : 0;

    if(vout == 1){
        ret = configVout(vout, mVoutConfig[vout].rotate, mVoutConfig[vout].flip, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);
        if(ret){
            AMLOG_ERROR("disable/enable(%d) vout(%d), vout_setting failed\n", enable, vout);
            mVoutConfig[vout].enable = origenable;
            return ME_OS_ERROR;
        }
        AMLOG_INFO("disable/enable(%d) vout(%d), vout_setting successfully\n", enable, vout);
    }

    return ME_OK;
#endif
}

AM_ERR CVideoRenderer::EnableOSD(AM_INT vout, AM_INT enable)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("EnableOSD: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    iav_vout_fb_sel_t fb_sel;

    memset(&fb_sel, 0, sizeof(iav_vout_fb_sel_t));
    fb_sel.vout_id = vout;

    if (!enable) {
        AMLOG_WARN("disable osd on vout %d.\n", vout);
        fb_sel.fb_id = -1;
    } else {
        AMLOG_WARN("enable osd on vout %d.\n", vout);
        fb_sel.fb_id = 0;//link to fb 0, hard code here
    }

    if(ioctl(mIavFd, IAV_IOC_VOUT_SELECT_FB, &fb_sel)) {
        AM_ERROR("IAV_IOC_VOUT_SELECT_FB Failed!");
        perror("IAV_IOC_VOUT_SELECT_FB");
        return ME_OS_ERROR;
    }
    return ME_OK;
}

AM_ERR CVideoRenderer::EnableVoutAAR(AM_INT enable)
{
    AM_ERROR("disable it here\n");
    return ME_OK;

    if((mbVoutAAR && enable)||(!mbVoutAAR && !enable)){
        AM_INFO("EnableVoutAAR: do nothing, mbVoutAAR=%d, enable=%d.\n", mbVoutAAR, enable);
        return ME_OK;
    }

    mDisplayWidth = mPicWidth;
    mDisplayHeight = mPicHeight;

    if(enable){
        for(AM_INT vout=eVoutLCD; vout<eVoutCnt; vout++){
            AM_ERR err;
            AM_INT x_offset = 0, y_offset = 0;
            AM_INT pic_width = 0, pic_height = 0, pic_temp = 0;
            AM_INT vout_width = 0, vout_height = 0;

            //get vout size
            err = GetDisplayDimension(vout, &vout_width, &vout_height);
            if (err != ME_OK) {
                AM_ERROR("[AAR]: GetDisplayDimension failed, vout=%d.\n", vout);
                continue ;
            }
            AMLOG_INFO("[AAR]: GetDisplayDimension ok, vout=%d, vout_width=%d, vout_height=%d.\n", vout, vout_width,vout_height);

            pic_width = mPicWidth;
            pic_height = mPicHeight;

            //for bug#2105, streams which need adjusting width to keep DAR should config UPDATE_VOUT_CONFIG in video renderer
            if(mpSharedRes->vid_width_DAR){
                pic_width = mpSharedRes->vid_width_DAR;
                AMLOG_INFO("[AAR]: video need adjust for DAR, vout=%d, pic_width=%d, pic_height=%d.\n", vout, pic_width,pic_height);
            }

            if (vout_width<vout_height) {    //pic's w-h need to fit vout ratio
                if((eVoutHDMI==vout) || (0==mpSharedRes->vid_rotation_info ||180==mpSharedRes->vid_rotation_info)){//roy mdf 2012.03.21, for vout LCD and video rotate 90/270 case, NO switch width-height{
                    pic_temp = pic_width;
                    pic_width = pic_height;
                    pic_height = pic_temp;
                }
            }else{
                if((eVoutLCD==vout) && ((90==mpSharedRes->vid_rotation_info ||270==mpSharedRes->vid_rotation_info))){
                    pic_temp = pic_width;
                    pic_width = pic_height;
                    pic_height = pic_temp;
                }
            }
            AMLOG_INFO("[AAR]: vout=%d, vid_rotation_info=%d, after pic width-height switch for ratio, pic_width=%d, pic_height=%d.\n", vout,mpSharedRes->vid_rotation_info,pic_width,pic_height);

            //a. if source_ratio==vout_ratio, do nothing
            //b. other case, fit display to picture size
            if((pic_width * vout_height  == vout_width * pic_height) && !(mpSharedRes->vid_rotation_info)){
                if(eVoutLCD==vout) {
                    mDisplayWidth = vout_width;
                    mDisplayHeight = vout_height;
                }
                AMLOG_INFO("[AAR]: do nothing for video, vout=%d, pic_width=%d, pic_height=%d, vout_width=%d, vout_height=%d.\n", vout, pic_width,pic_height,vout_width,vout_height);
                continue;
            }

            if(pic_width * vout_height  > vout_width * pic_height){
                pic_height = vout_width * pic_height /pic_width;
                pic_width = vout_width;
                pic_height = (pic_height+15)&(~15);//round up
                pic_height = (pic_height > vout_height) ? vout_height : pic_height;//needless, pic_height must be not greater than vout_height
                x_offset = 0;
                y_offset = (vout_height-pic_height)/2;
            }else{
                pic_width = vout_height * pic_width /pic_height;
                pic_height = vout_height;
                pic_width = (pic_width+15)&(~15);
                pic_width = (pic_width > vout_width) ? vout_width : pic_width;
                y_offset = 0;
                x_offset = (vout_width-pic_width)/2;
            }

            if(eVoutLCD==vout) {
                if(vout_width<vout_height){
                    mDisplayWidth = pic_height;
                    mDisplayHeight = pic_width;
                }else{
                    mDisplayWidth = pic_width;
                    mDisplayHeight = pic_height;
                }
            }

            if(  ( (x_offset + pic_width) > vout_width ) ||( (y_offset + pic_height) > vout_height )
                || x_offset<0 ||y_offset<0 || pic_width<0 ||pic_height<0) {
                AMLOG_INFO("[AAR]: out-of-range, vout:%d, x = %d,y = %d, width: %d, height: %d\n",vout,x_offset, y_offset, pic_width, pic_height);
            } else {
                SetDisplayPositionSize(vout, x_offset, y_offset, pic_width, pic_height);
                AMLOG_INFO("[AAR]: SetDisplayPositionSize vout=%d x_offset=%d y_offset=%d pic_width=%d pic_height=%d\n",
                    vout, x_offset, y_offset, pic_width, pic_height);
                if(eVoutLCD==vout && mpSharedRes->vid_rotation_info){//roy mdf 2012.03.21, for vout LCD and video rotate case, currently HDMI no support AAR-FullScreen switch
                    AMLOG_INFO("[AAR]: SetDisplayFlip and Rotation vout=%d, vid_rotation_info=%d.\n",vout,mpSharedRes->vid_rotation_info);
                    if(vout_width>vout_height){//for aof pj203, LCD w>h
                        switch(mpSharedRes->vid_rotation_info){
                        case 90:
                            SetDisplayRotation(vout, 1);
                            break;
                        case 180:
                            SetDisplayFlip(vout, 1);
                            break;
                        case 270:
                            SetDisplayFlip(vout, 1);
                            SetDisplayRotation(vout, 1);
                            break;
                        default:
                            break;
                        }
                    }else{//iOne EVK, lcd w<h
                        switch(mpSharedRes->vid_rotation_info){
                        case 90:
                            SetDisplayFlip(vout, 1);
                            SetDisplayRotation(vout, 0);
                            break;
                        case 180:
                            SetDisplayFlip(vout, 1);
                            break;
                        case 270:
                            SetDisplayRotation(vout, 0);
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
    } else {
        SetDisplayPositionSize(eVoutLCD, 0, 0, mVoutConfig[eVoutLCD].width, mVoutConfig[eVoutLCD].height);//currently HDMI no support AAR-FullScreen switch
        SetDisplayFlip(eVoutLCD, 0);
        SetDisplayRotation(eVoutLCD, 1);
    }

    AMLOG_INFO("EnableVoutAAR: original mbVoutAAR=%d, target enable=%d.\n", mbVoutAAR, enable);
    mbVoutAAR = enable ? true : false;
    return ME_OK;
}

AM_ERR CVideoRenderer::SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h)
{
/*    if (!mpFrameBufferPool) {
        AMLOG_ERROR("SetVideoSourceRect: NULL mpFrameBufferPool.\n");
        return ME_ERROR;
    }
*/

    AMLOG_INFO("'SetVideoSourceRect, x %d, y %d, w %d, h %d.\n", x, y, w, h);

    if ( x < 0 || y < 0 || w < 1 || h < 1 || ((AM_UINT)(x+w))>mPicWidth || ((AM_UINT)(y+h))>mPicHeight) {
        AMLOG_ERROR("SetVideoSourceRect: x y w h out of range, mpFrameBufferPool->mPicWidth %d, mpFrameBufferPool->mPicHeight %d.\n", mPicWidth, mPicHeight);
        return ME_BAD_PARAM;
    }

    mDisplayRectMap[0].rect[0].x = x;
    mDisplayRectMap[0].rect[0].y = y;
    mDisplayRectMap[0].rect[0].w = w;
    mDisplayRectMap[0].rect[0].h = h;

    mbDisplayRectNeedUpdate = true;
    return ME_OK;
}

AM_ERR CVideoRenderer::SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h)
{
    if (/*!mpFrameBufferPool || */vout_id < 0 || vout_id >= eVoutCnt) {
        AMLOG_ERROR("SetVideoDestRect: vout_id(%d) out of range.\n", vout_id);
        return ME_BAD_PARAM;
    }

    AMLOG_INFO("'SetVideoDestRect %d, x %d, y %d, w %d, h %d.\n", vout_id, x, y, w, h);

    if ( x < 0 || y < 0 || w < 1 || h < 1 || (x+w)>mVoutConfig[vout_id].width || (y+h)>mVoutConfig[vout_id].height) {
        AMLOG_ERROR("SetVideoDestRect: x y w h out of range, mVoutConfig[vout_id].width %d, mVoutConfig[vout_id].height %d.\n", mVoutConfig[vout_id].width, mVoutConfig[vout_id].height);
        return ME_BAD_PARAM;
    }

    mDisplayRectMap[vout_id].rect[1].x = x;
    mDisplayRectMap[vout_id].rect[1].y = y;
    mDisplayRectMap[vout_id].rect[1].w = w;
    mDisplayRectMap[vout_id].rect[1].h = h;

    mbDisplayRectNeedUpdate = true;
    return ME_OK;
}

AM_ERR CVideoRenderer::SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y)
{
    if (vout_id < 0 || vout_id >= eVoutCnt) {
        AMLOG_ERROR("SetVideoScaleMode: vout_id(%d) out of range.\n", vout_id);
        return ME_BAD_PARAM;
    }

    AMLOG_INFO("'SetVideoScaleMode %d, mode_x %d, mode_y %d.\n", vout_id, mode_x, mode_y);

    mDisplayRectMap[vout_id].factor_x = mode_x;
    mDisplayRectMap[vout_id].factor_y = mode_y;
    mbDisplayRectNeedUpdate = true;
    return ME_OK;
}

AM_ERR CVideoRenderer::SetDeWarpControlWidth(AM_UINT enable, AM_UINT width_top, AM_UINT width_bottom)
{
    AMLOG_INFO("'SetDeWarpControlWidth: enable %d, width_top %d, width_bottom %d.\n", enable, width_top, width_bottom);
    AM_INT ret;
    iav_udec_vout_dewarp_config_t dewarp;

    memset(&dewarp, 0x0, sizeof(dewarp));

    dewarp.udec_id = mDspIndex;
    dewarp.horz_warp_enable = enable;
    dewarp.warp_table_type = IAV_DEWARP_PARAMS_TOP_BOTTOM_WIDTH;
    dewarp.use_customized_dewarp_params = 0;
    dewarp.vout_rotated = mVoutConfig[eVoutLCD].rotate;//hard code to LCD here

    dewarp.warp_param0 = width_top;
    dewarp.warp_param1 = width_bottom;

    ret = ioctl(mIavFd, IAV_IOC_UPDATE_VOUT_DEWARP_CONFIG, &dewarp);
    if (ret < 0) {
        perror("IAV_IOC_UPDATE_VOUT_DEWARP_CONFIG");
        AM_ERROR("IAV_IOC_UPDATE_VOUT_DEWARP_CONFIG fail.\n");
        return ME_ERROR;
    }

    return ME_OK;
}

#ifdef AM_DEBUG
void CVideoRenderer::PrintState()
{
    AMLOG_INFO("VRenderer [%d]: msState=%d, %d input data.\n",mDspIndex, msState, mpInput->mpBufferQ->GetDataCnt());
}
#endif

//-----------------------------------------------------------------------
//
// CVideoRendererInput
//
//-----------------------------------------------------------------------
CVideoRendererInput* CVideoRendererInput::Create(CFilter *pFilter)
{
    CVideoRendererInput *result = new CVideoRendererInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CVideoRendererInput::Construct()
{
    AM_ERR err = inherited::Construct(((CVideoRenderer*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;

    return ME_OK;
}

CVideoRendererInput::~CVideoRendererInput()
{
    AMLOG_DESTRUCTOR("~CVideoRendererInput.\n");
    // todo
}

AM_ERR CVideoRendererInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    if ((*pFormat->pMediaType == GUID_Decoded_Video || *pFormat->pMediaType == GUID_Amba_Decoded_Video) &&
            *pFormat->pSubType == GUID_Video_YUV420NV12 &&
            *pFormat->pFormatType == GUID_Format_FFMPEG_Media) {
        ((CVideoRenderer*)mpFilter)->SetInputFormat((CFFMpegMediaFormat*)pFormat);
        return ME_OK;
    }

    return ME_NOT_SUPPORTED;
}

