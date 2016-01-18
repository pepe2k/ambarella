/**
 * audio_renderer.cpp
 *
 * History:
 *    2010/1/19 - [Yu Jiankang] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define LOG_NDEBUG 0
#define LOG_TAG "audio_renderer"
//#define AMDROID_DEBUG

#include <basetypes.h>
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#include "record_if.h"

#include "am_pbif.h"
#include "pbif.h"
#include "engine_guids.h"
#include "audio_if.h"

#include "audio_renderer.h"
#include "filter_list.h"

extern "C"{
#define INT64_C(a) (a ## LL)
#include "libavcodec/avcodec.h"
}

filter_entry g_audio_renderer = {
    "AudioRenderer",
    CAudioRenderer::Create,
    NULL,
    CAudioRenderer::AcceptMedia,
};

IFilter* CreateAudioRenderer(IEngine *pEngine)
{
    return CAudioRenderer::Create(pEngine);
}

#define DARNoDataCheckFlag 0x1

//-----------------------------------------------------------------------
//
// CAudioFixedBufferPool
//
//-----------------------------------------------------------------------

CAudioFixedBufferPool *CAudioFixedBufferPool::Create(AM_UINT size, AM_UINT count)
{
    CAudioFixedBufferPool *result = new CAudioFixedBufferPool;
    if (result != NULL && result->Construct(size, count) != ME_OK)
    {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CAudioFixedBufferPool::Construct(AM_UINT size, AM_UINT count)
{
    AM_ERR err;

    if ((err = inherited::Construct(count)) != ME_OK)
        return err;

    if ((_pBuffers = new CAudioBuffer[count]) == NULL)
    {
        AM_ERROR("Audio Renderer allocate CBuffers fail.\n");
        return ME_ERROR;
    }

    size = ROUND_UP(size, 4);
    if ((_pMemory = new AM_U8[count * size]) == NULL)
    {
        AM_ERROR("Audio Renderer allocate data buffer fail.\n");
        return ME_ERROR;
    }

    AM_U8 *ptr = _pMemory;
    CAudioBuffer *pAudioBuffer = _pBuffers;
    CBuffer* pBuffer;
    for (AM_UINT i = 0; i < count; i++, ptr += size, pAudioBuffer++)
    {
        pBuffer = (CBuffer*)pAudioBuffer;
        pBuffer->mpData = ptr;
        pBuffer->mBlockSize = size;
        pBuffer->mpPool = this;

        err = mpBufferQ->PostMsg(&pBuffer, sizeof(pBuffer));
        AM_ASSERT_OK(err);
    }

    return ME_OK;
}

CAudioFixedBufferPool::~CAudioFixedBufferPool()
{
    if(_pMemory) {
        delete[] _pMemory;
        _pMemory = NULL;
    }
    if(_pBuffers) {
        delete _pBuffers;
        _pBuffers = NULL;
    }
}

void CAudioFixedBufferPool::Delete()
{
    if(_pMemory) {
        delete[] _pMemory;
        _pMemory = NULL;
    }
    if(_pBuffers) {
        delete _pBuffers;
        _pBuffers = NULL;
    }
    inherited::Delete();
}


//-----------------------------------------------------------------------
//
// CAudioRenderer
//
//-----------------------------------------------------------------------
IFilter* CAudioRenderer::Create(IEngine *pEngine)
{
    CAudioRenderer *result = new CAudioRenderer(pEngine);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

int CAudioRenderer::AcceptMedia(CMediaFormat& format)
{
    if (*format.pMediaType == GUID_Decoded_Audio/* &&
        *format.pSubType == GUID_Audio_PCM*/)
        return 1;
    return 0;
}

AM_ERR CAudioRenderer::Start()
{
    mpWorkQ->Start();
    return ME_OK;
}

AM_ERR CAudioRenderer::Run()
{
    mpWorkQ->Run();
    return ME_OK;
}

void CAudioRenderer::GetInfo(INFO& info)
{
    info.nInput = 1;
    info.nOutput = 0;
    info.mPriority = 10;
    info.mFlags = SYNC_FLAG;
    info.pName = "AudioRenderer";
}

IPin* CAudioRenderer::GetInputPin(AM_UINT index)
{
    if (index == 0)
        return mpAudioInputPin;

    return NULL;
}

#ifdef AM_DEBUG
void CAudioRenderer::PrintState()
{
    AMLOG_INFO("ARenderer: msState=%d, %d input data.\n", msState, mpAudioInputPin->mpBufferQ->GetDataCnt());
}
#endif

void CAudioRenderer::OnRun()
{
    AMLOG_INFO("AR::OnRun: enter.\n");

    // reply ME_OK to CMD_RUN cmd
    CmdAck(ME_OK);

    init();

    mbRun = requestRun();

    //state machine:
    //before start playback: STATE_WAIT_RENDERER_READY  --> STATE_WAIT_PROPER_START_TIME --> IDLE
    //playback: IDLE  --> HAS_INPUT_DATA --> READY(slave mode) --> IDLE
    //eos -->  PENDING

    while (mbRun) {
        AMLOG_STATE("AR:onRun: start switch, msState=%d, %d input data.\n", msState, mpAudioInputPin->mpBufferQ->GetDataCnt());

        switch (msState) {
            case STATE_IDLE:
                onStateIdle();
                break;
            case STATE_HAS_INPUTDATA:
                onStateHasInputData();
                break;
            case STATE_PENDING:
                onStatePending();
                break;
            case STATE_READY:
                onStateReady();
                break;
            case STATE_WAIT_RENDERER_READY:
                onStateWaitRendererReady();
                break;
            case STATE_WAIT_PROPER_START_TIME:
                onStateWaitProperTime();
                break;
            case STATE_ERROR:
                onStateError();
                break;
            default:
                AMLOG_ERROR("AR::OnRun: No handler for state=%d.\n", msState);
                break;
        }
    }

    deinit();

    AMLOG_INFO("AR::OnRun: exit.\n");
}

void* CAudioRenderer::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IClockObserver)
        return (IClockObserver*)this;

    if (refiid == IID_IRenderer)
        return (IRender*)this;

    return inherited::GetInterface(refiid);
}

void CAudioRenderer::Delete()
{
    AM_DELETE(mpAudioOut);
    mpAudioOut = NULL;

    AM_DELETE(mpAudioInputPin);
    mpAudioInputPin = NULL;

    AM_RELEASE(mpABP);
    mpABP = NULL;

    inherited::Delete();
}

AM_ERR CAudioRenderer::OnTimer(am_pts_t curr_pts)
{
    AO::CMD cmd((AM_UINT)CMD_TIMERNOTIFY);

    if (mCheckNoDataTime == curr_pts) {
        AMLOG_WARN("No data check timer comes, msState %d.\n", msState);
        cmd.flag = DARNoDataCheckFlag;
    } else {
        cmd.flag = 0;
    }

    mpWorkQ->MsgQ()->PostMsg((void*)&cmd, sizeof(cmd));
    return ME_OK;
}

AM_ERR CAudioRenderer::GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs)
{
    if(!mbAudioStarted) {
        absoluteTimeMs = mTimeOffset/90;//just at seek's point
        relativeTimeMs = 0;
        AMLOG_PTS("CAudioRenderer::GetCurrentTime not started absoluteTimeMs=%llu relativeTimeMs=%llu.\n", absoluteTimeMs, relativeTimeMs);
        return ME_OK;
    }

    AM_UINT latency = (mLatency/90);
    // init
    relativeTimeMs = mAudioTotalSamples * 1000 / mAudioSamplerate;
    absoluteTimeMs = mTimeOffset/90 + relativeTimeMs;

    // latency compensation
    absoluteTimeMs = (absoluteTimeMs > latency) ? (absoluteTimeMs - latency) : 0;
    relativeTimeMs = (relativeTimeMs > latency) ? (relativeTimeMs - latency) : 0;

    AMLOG_PTS("CAudioRenderer::GetCurrentTime return absoluteTimeMs=%llu relativeTimeMs=%llu.\n", absoluteTimeMs, relativeTimeMs);
    return ME_OK;
}

bool CAudioRenderer::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("****CAudioRenderer::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);
    switch (cmd.code) {

        case CMD_STOP:
            mpAudioOut->Stop();
            mbAudioSinkOpened = AUDIO_SINK_CLOSED;
            mbRun = false;
            CmdAck(ME_OK);
            break;

        case CMD_TIMERNOTIFY:
            if (DARNoDataCheckFlag == cmd.flag) {
                if (STATE_WAIT_RENDERER_READY == msState) {
                    AMLOG_WARN("still no data comes, switch master renderer to video");
                    switchVideoRendererAsMaster();
                }
                break;
            }

            if ( msState == STATE_READY ) {
                AM_ASSERT(mpBuffer);
                if (mpBuffer) {
                    renderBuffer();
                }
                msState = STATE_IDLE;
            } else if (msState == STATE_PENDING && mpBuffer) {
                //paused
                AM_RELEASE(mpBuffer);
                mpBuffer = NULL;
            }
            break;

        case CMD_PAUSE:
            if (mbAudioStarted) {
                AM_ASSERT(!mbAudioOutPaused);
                if (false == mbAudioOutPaused) {
                    mpAudioOut->Pause();
                    mbAudioOutPaused = true;
                }
            }
            mbPaused = true;
            break;

        case CMD_RESUME:
            if (msState == STATE_PENDING) {
                msState = STATE_IDLE;
            }
            if (mbAudioStarted) {
                AM_ASSERT(mbAudioOutPaused);
                if (true == mbAudioOutPaused) {
                    mpAudioOut->Resume();
                    mbAudioOutPaused = false;
                }
            }
            mbPaused = false;
            break;

        case CMD_FLUSH:
            AMLOG_INFO("Audio renderer Flush...\n");
            mpAudioOut->Flush();
            mpAudioOut->Pause();
            mbAudioOutPaused = true;
            mbAudioStarted = false;
            msState = STATE_PENDING;

            AM_RELEASE(mpBuffer);
            mpBuffer = NULL;

            reset();
            CmdAck(ME_OK);
            AMLOG_INFO("Audio renderer Flush done.\n");
            break;

        case CMD_AVSYNC:
            AM_ASSERT(!mbRecievedSyncCmd);
            mbRecievedSyncCmd = true;
            CmdAck(ME_OK);
            break;

        case CMD_BEGIN_PLAYBACK:
            AM_ASSERT(msState == STATE_PENDING);
            AM_ASSERT(!mpBuffer);
            msState = STATE_WAIT_RENDERER_READY;
            reset();
            mTimeOffset = mpSharedRes->mPlaybackStartTime;
            //mpAudioOut->Resume();
            break;

        case CMD_SOURCE_FILTER_BLOCKED:
            AM_ASSERT(!mbRecievedSourceFilterBlockedCmd);
            mbRecievedSourceFilterBlockedCmd = true;
            break;

        case CMD_REALTIME_SPEEDUP:
            mbNeedSpeedUp = 1;
            break;

        default:
            AM_ERROR("Audio renderer wrong cmd.code: %d", cmd.code);
            break;
    }
    return false;
}

AM_ERR CAudioRenderer::Construct()
{
    AM_ERR err = inherited::Construct();
    if(err != ME_OK)
        return err;
    DSetModuleLogConfig(LogModuleAudioRenderer);
    mpAudioOut = AM_CreateAudioHAL(mpEngine, 1, 0);
    if(mpAudioOut == NULL)
    {
        AM_ERROR("Create AudioOut Failed!!\n");
        return ME_IO_ERROR;
    }

    if((mpAudioInputPin = CAudioRendererInput::Create(this)) == NULL)
        return ME_ERROR;

    if((mpABP = CAudioFixedBufferPool::Create((AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2, 64)) == NULL)
        return ME_ERROR;

    mpAudioInputPin->SetBufferPool(mpABP);
#if 0
    mpWriter= CFileWriter::Create();
    if (mpWriter == NULL)
        {
        return ME_ERROR;
        }
        err = mpWriter->CreateFile("/tmp/mmcblk0p1/pcm.dat");
            if (err != ME_OK)
        return err;
#endif
    mpClockManager = (IClockManager *)mpEngine->QueryEngineInterface(IID_IClockManager);
    if(!mpClockManager) {
        AM_ERROR("CAudioRenderer::Run without mpClockManager?\n");
        return ME_ERROR;
    }

    return ME_OK;
}

CAudioRenderer::CAudioRenderer(IEngine *pEngine):
    CActiveFilter(pEngine, "AudioRenderer"),
    mpAudioInputPin(NULL),
    mpABP(NULL),
    mpClockManager(NULL),
    mpBuffer(NULL),
    mbAudioOutPaused(false),
    mbAudioStarted(false),
    mAudioTotalSamples(0),
    mAudioTotalSamplesTime(0),
    //mpAudioSink(NULL),
    mTimeOffset(0),
    mFirstPTS(0),
    mLastPTS(0),
    mLatency(0),
    mEstimatedDurationForAVSync(0),
    mbGetEstimatedDuration(false),
    mbAudioSinkOpened(AUDIO_SINK_CLOSED),
    mbRecievedSyncCmd(false),
    mbRecievedSourceFilterBlockedCmd(false),
    mCheckNoDataTime(0),
    mbNeedSpeedUp(0)
{
    mpAudioOut = NULL;
}

CAudioRenderer::~CAudioRenderer()
{
    AM_DELETE(mpAudioOut);
    AM_DELETE(mpAudioInputPin);
    AM_RELEASE(mpABP);
    //AM_DELETE(mpWriter);
}

void CAudioRenderer::init()
{
    msState = STATE_WAIT_RENDERER_READY;
    reset();
}

void CAudioRenderer::deinit()
{
    AM_RELEASE(mpBuffer);
    mpBuffer = NULL;
}

void CAudioRenderer::reset()
{
    mbAudioStarted = false;
    mbStreamStart = false;
    mFirstPTS = 0;
    mLastPTS = 0;
    mAudioTotalSamples = 0;
    //mbPaused = false;
    mbRecievedSyncCmd = false;
    mbRecievedSourceFilterBlockedCmd = false;
}

bool CAudioRenderer::requestRun()
{
    AO::CMD cmd;

    //send ready
    AMLOG_INFO("ARenderer: post IEngine::MSG_READY.\n");
    PostEngineMsg(IEngine::MSG_READY);

    while (1) {
        AMLOG_INFO("CAudioRenderer::requestRun...\n");
        GetCmd(cmd);
        if (cmd.code == AO::CMD_START) {
            AMLOG_INFO("CAudioRenderer::requestRun done.\n");
            mTimeOffset = mpSharedRes->mPlaybackStartTime;
            CmdAck(ME_OK);
            return true;
        }

        if (cmd.code == AO::CMD_STOP) {
            AMLOG_INFO("CAudioRenderer::requestRun stop comes...\n");
            CmdAck(ME_OK);
            return false;
        }

        if(cmd.code == AO::CMD_AVSYNC) {
            //must not comes here
            AM_ASSERT(0);
            CmdAck(ME_OK);
        }else {
            AMLOG_ERROR("not handled cmd %d.\n", cmd.code);
        }
    }
}

bool CAudioRenderer::waitAVSyncCmd()
{
    AO::CMD cmd;

    while (1) {
        AMLOG_INFO("CAudioRenderer::ReadyWaitAVSync...\n");
        GetCmd(cmd);
        if (cmd.code == AO::CMD_AVSYNC) {
            AMLOG_INFO("CAudioRenderer::waitAVSyncCmd done.\n");
            CmdAck(ME_OK);
            return true;
        }

        if (cmd.code == AO::CMD_STOP) {
            AMLOG_INFO("CAudioRenderer::waitAVSyncCmd stop comes...\n");
            ProcessCmd(cmd);
            return false;
        }

        if (cmd.code == AO::CMD_PAUSE || cmd.code == AO::CMD_FLUSH) {
            AMLOG_INFO("CAudioRenderer::waitAVSyncCmd pause/flush comes...\n");
            ProcessCmd(cmd);
            return false;
        }

    }
}

void CAudioRenderer::renderBuffer()
{
    AM_UINT numOfFrames;
    mLastPTS = mpBuffer->GetPTS();

#ifdef AM_DEBUG
    static AM_UINT skip = 0;
    if (mpSharedRes->discard_half_audio_packet) {
        if (!skip) {
            mpAudioOut->Render(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize(), &numOfFrames);
            mAudioTotalSamples += numOfFrames*2;
            skip = 1;
            AMLOG_INFO("not skip auiod data.\n");
        } else {
            skip = 0;
            AMLOG_INFO("skip auiod data.\n");
        }
    } else {
        mpAudioOut->Render(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize(), &numOfFrames);
        mAudioTotalSamples += numOfFrames;
    }
#else
    mpAudioOut->Render(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize(), &numOfFrames);
    mAudioTotalSamples += numOfFrames;
#endif

#if 0
AM_ERR err;
        //SAVE pcm FORM FFMPEG
        err =mpWriter->WriteFile(pBuffer->GetDataPtr(), pBuffer->GetDataSize());
    if (err != ME_OK)
    {
        AM_INFO("Error in writing audio size:%d\n", pBuffer->GetDataSize());
        //exit(1);
        //return ME_ERROR;
    }
    AM_INFO("Buffer Size: %d, May be sample count:%d   <-> druing: %f\n", pBuffer->GetDataSize(),
                            pBuffer->GetDataSize()*8/16, pBuffer->GetDataSize()*8/16/48000/2);
#endif

    AM_RELEASE(mpBuffer);
    mpBuffer = NULL;
}

void CAudioRenderer::discardBuffer(CBuffer*& pBuffer)
{
    AM_RELEASE(mpBuffer);
    mpBuffer = NULL;
}

AM_ERR CAudioRenderer::openAudioSink(CAudioBuffer* pBuffer)
{
    AM_ASSERT(pBuffer);
    mAudioSamplerate = pBuffer->sampleRate;
    mAudioChannels = pBuffer->numChannels;
    mAudioSampleFormat = pBuffer->sampleFormat;
    if(mAudioSampleFormat !=0 //SAMPLE_FMT_U8
        && mAudioSampleFormat !=1) //SAMPLE_FMT_S16
    {
        AMLOG_INFO("CAudioRenderer: AudioSampleFormat[%d] is wrong\n", mAudioSampleFormat);
        return ME_ERROR;
    }

    // prepare parameters and call Audio HAL
    IAudioHAL::AudioParam audioParam;
    audioParam.sampleFormat = ((mAudioSampleFormat == 1) ? IAudioHAL::FORMAT_S16_LE: IAudioHAL::FORMAT_U8_LE);
    audioParam.stream = IAudioHAL::STREAM_PLAYBACK;
    audioParam.sampleRate = mAudioSamplerate;
    audioParam.numChannels = mAudioChannels;

    if(mpAudioOut->OpenAudioHAL(&audioParam, &mLatency, &mAudioBufferSize) != ME_OK) {
        AMLOG_ERROR("CAudioRenderer: mpAudioOut OpenAudioHAL Failed\n");
        return ME_ERROR;
    }
    mbAudioOutPaused = false;//default is not-paused
    return ME_OK;
}

void CAudioRenderer::handleEOS()
{
    PostEngineMsg(IEngine::MSG_EOS);
    AMLOG_INFO("ARenderer get EOS.\n");

    mbStreamStart = false;

    AM_RELEASE(mpBuffer);
    mpBuffer = NULL;
    reset();
}

AM_ERR CAudioRenderer::switchVideoRendererAsMaster()
{
    AM_MSG msg;
    AM_ERR err;
    msg.code = IEngine::MSG_SWITCH_MASTER_RENDER;
    msg.p0 = 1;
    err = PostEngineMsg(msg);

    return err;
}

void CAudioRenderer::onStateIdle()
{
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    CQueueInputPin* pPin;

//debug check
#ifdef AM_DEBUG
    AM_ASSERT(mbRecievedSyncCmd);
    AM_ASSERT(mbAudioStarted);
    AM_ASSERT(mbAudioSinkOpened);
    AM_ASSERT(!mpBuffer);
#endif

    //goto pending if is pasued, if audio latency is finished
    if(mbPaused && mbAudioStarted) {
        msState = STATE_PENDING;
        return;
    }

    //do speed up if needed
    if (mbNeedSpeedUp) {
        speedUp();
        mbNeedSpeedUp = 0;
        return;
    }

    //wait input data, process msg
    type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
    if(type == CQueue::Q_MSG) {
        ProcessCmd(cmd);
    } else {
        pPin = (CQueueInputPin*)result.pOwner;
        if (!pPin->PeekBuffer(mpBuffer)) {
            AM_ERROR("No buffer?\n");
            return ;
        }

        if (mpBuffer->GetType() == CBuffer::EOS) {
            handleEOS();
            msState = STATE_PENDING;
            return ;
        }

        AM_ASSERT(mpBuffer->GetType() == CBuffer::DATA);
        if (AUDIO_SINK_CLOSED == mbAudioSinkOpened) {
            AMLOG_WARN("!!!!! before openAudioSink.\n");
            if(openAudioSink((CAudioBuffer*)mpBuffer) != ME_OK) {
                AM_ERROR("audio sink open fail, audio renderer goes to error state.\n");
                sendSyncInErrorCase();
                msState = STATE_ERROR;
                return;
            }
            AMLOG_WARN("!!!!! openAudioSink done.\n");
            mbAudioSinkOpened = AUDIO_SINK_OPEN_GOOD;
        }

        if (!mbGetEstimatedDuration) {
            if (!mEstimatedDurationForAVSync) {
                mEstimatedDurationForAVSync = mpBuffer->GetPTS();
            } else {
                //two packet's PTS gap
                mEstimatedDurationForAVSync = mpBuffer->GetPTS() - mEstimatedDurationForAVSync;
                if (mEstimatedDurationForAVSync > ((AM_U64)mWaitThreshold)) {
                    AMLOG_WARN("audio packet's duration(%llu) is larger than mWaitThreshold, would need adjust mWaitThreshold.\n", mEstimatedDurationForAVSync);
                    if ((AM_S64)(mEstimatedDurationForAVSync*2) > mNotSyncThreshold) {
                        //cannot greater than mNotSyncThreshold
                        mWaitThreshold = mNotSyncThreshold;
                    } else {
                        mWaitThreshold = mEstimatedDurationForAVSync*2;
                    }
                    AMLOG_WARN("after adjust, mWaitThreshold %lld.\n", mWaitThreshold);
                } else {
                    AMLOG_INFO(" audio estimated duration is %llu.\n", mEstimatedDurationForAVSync);
                }
                mbGetEstimatedDuration = true;
            }
        }

        msState = STATE_HAS_INPUTDATA;
        return;
    }
}

void CAudioRenderer::onStateHasInputData()
{
    am_pts_t curTime;
    AM_S64 diff;
    AM_ASSERT(mpBuffer);

    curTime = mpClockManager->GetCurrentTime();
    diff = mpBuffer->mPTS - mLatency - curTime;

    //AMLOG_PTS("[AR PTS]: mpBuffer->mPTS=%llu  curTime=%llu, mLatency %u, diff %lld.\n", mpBuffer->mPTS, curTime, mLatency, diff);
    /*AMLOG_WARN("[how2], mNotSyncThreshold=%lld, mWaitThreshold=%lld, mLatency=%d, mpBuffer->mPTS=%lld, curTime =%lld, diff=%lld.\n",
                                mNotSyncThreshold,
                                mWaitThreshold,
                                mLatency,
                                mpBuffer->mPTS,
                                curTime,
                                diff);*/
    if(mpAudioOut->NeedAVSync()) {
        if(IsMaster()) {
            // master should sync startPTS/startTime for clockmanager when needed
            if (diff > 3*mNotSyncThreshold ||diff < -3*mNotSyncThreshold) {
                if(mpBuffer->mPTS > mLatency)
                {
                    mpClockManager->SetClockMgr(mpBuffer->mPTS - mLatency);
                    //important info need always print out
                    AMLOG_WARN("[AV sync], audio detect discontinuty, mpBuffer->mPTS=%llu, curTime =%llu, diff=%lld, adjust system clock to %llu.\n", mpBuffer->mPTS, curTime, diff, mpBuffer->mPTS - mLatency);
                }
            } else if (diff < mWaitThreshold && diff > - mWaitThreshold) {
                //should be normal case
                //AMLOG_WARN("synced ....\n");
            } else {
                //need sync system clock here?
                if (diff > mNotSyncThreshold || diff < -mNotSyncThreshold) {
                    if (mpBuffer->mPTS > mLatency) {
                        mpClockManager->SetClockMgr(mpBuffer->mPTS - mLatency);
                        AMLOG_INFO("[AV sync], audio have significant difference with system clock, mpBuffer->mPTS=%llu, curTime =%llu, diff=%lld, adjust system clock to %llu.\n", mpBuffer->mPTS, curTime, diff, mpBuffer->mPTS - mLatency);
                    }
                } else {
                    AMLOG_PTS("[AV sync], diff (%lld) is not greater than threshold, not sync clock here.\n", diff);
                }
            }
            renderBuffer();
        } else {
            //slave should consider time/PTS differerence: discard/render immediately/render later
            if(diff < (-mWaitThreshold) && (mpBuffer->mPTS < (curTime + mLatency + mNotSyncThreshold*10))) { // mNotSyncThreshold*10 is a pts error detection
                //Warning msg should be print out!
                AMLOG_WARN("discard audio packet because of not-sync, diff %lld.\n", diff);
                discardBuffer(mpBuffer);
            } else if (diff > mWaitThreshold && (mpBuffer->mPTS > mLatency)) {
                mpClockManager->SetTimer(this, mpBuffer->mPTS - mLatency);
                msState = STATE_READY;
                return;
            } else {
                //proper time
                renderBuffer();
            }
        }
    } else {
        renderBuffer();
    }

    msState = STATE_IDLE;
}

void CAudioRenderer::onStatePending()
{
    CMD cmd;

    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
    ProcessCmd(cmd);
}

void CAudioRenderer::onStateReady()
{
    CMD cmd;

    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
    ProcessCmd(cmd);
}

void CAudioRenderer::sendSyncInErrorCase()
{
    //with some default parameters
    if (!mbStreamStart) {
        mbStreamStart = true;
    }

    PostEngineMsg(IEngine::MSG_AVSYNC);
}

void CAudioRenderer::onStateWaitRendererReady()
{
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    CQueueInputPin* pPin;
//    AM_MSG msg;

    while (1) {
        //wait input data, process msg
        type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
        if(type == CQueue::Q_MSG) {
            ProcessCmd(cmd);
            if (msState != STATE_WAIT_RENDERER_READY || !mbRun) {//fix bug#2311
                AM_ERROR("[AR flow]: change state cmd comes OR stop cmd comes in waiting render ready, break out, msState %d, mbRun %d.\n", msState, mbRun);
                return;
            }
            if (CMD_SOURCE_FILTER_BLOCKED == cmd.code) {
                AMLOG_WARN("[AR flow]: source filter blocked, but audio renderer is not ready(audio latency data is not finished), set timer, to check if there's no audio data.\n");
                //switch master's condition should be more strict
                //switchVideoRendererAsMaster();
                mCheckNoDataTime = mpClockManager->GetCurrentTime() + 90000;//hard code here, 1 second later
                mpClockManager->SetTimer(this, mCheckNoDataTime);
                AMLOG_WARN("[AR flow]: source filter blocked, SetTimer mCheckNoDataTime=%llu.\n", mCheckNoDataTime);

                sendSyncInErrorCase();
                msState = STATE_WAIT_PROPER_START_TIME;
                return;
            }
        } else {
            pPin = (CQueueInputPin*)result.pOwner;
            if (!pPin->PeekBuffer(mpBuffer)) {
                AM_ERROR("No buffer?\n");
                sendSyncInErrorCase();
                msState = STATE_ERROR;
                return;
            }

            if (mpBuffer->GetType() == CBuffer::EOS) {
                AMLOG_WARN("[AR flow]: total audio data is less than latency?\n");
                sendSyncInErrorCase();
                handleEOS();
                msState = STATE_PENDING;
                return ;
            } else {
                AM_ASSERT(CBuffer::DATA == mpBuffer->GetType());

                if (AUDIO_SINK_CLOSED == mbAudioSinkOpened) {
                    AMLOG_WARN("!!!!! before openAudioSink.\n");
                    if(openAudioSink((CAudioBuffer*)mpBuffer) != ME_OK) {
                        AM_ERROR("audio sink open fail, audio renderer goes to error state.\n");
                        sendSyncInErrorCase();
                        msState = STATE_ERROR;
                        return;
                    }
                    AMLOG_WARN("!!!!! openAudioSink done.\n");
                    mbAudioSinkOpened = AUDIO_SINK_OPEN_GOOD;
                }

                if(!mbStreamStart) {
                    mbStreamStart = true;
                    mAudioTotalSamples = 0;
                    mFirstPTS = mpBuffer->GetPTS();

                    if (mpSharedRes->mbAudioFirstPTSVaild && mpSharedRes->mbAudioStreamStartTimeValid) {
                        AMLOG_PTS("[AR flow]: update mTimeOffset: old=%lld new=%lld.\n", mTimeOffset, mpSharedRes->audioFirstPTS - mpSharedRes->audioStreamStartPTS);
                        // AMLOG_PTS("[AR flow]: update mTimeOffset: audioFirstPTS=%lld audioStreamStartPTS=%lld.\n", mpSharedRes->audioFirstPTS, mpSharedRes->audioStreamStartPTS);
                        mTimeOffset = mpSharedRes->audioFirstPTS - mpSharedRes->audioStreamStartPTS;
                    } else {
                        AMLOG_PTS("[AR flow]: update mTimeOffset failure: mbAudioFirstPTSVaild=%d mbAudioStreamStartTimeValid=%d.\n", mpSharedRes->mbAudioFirstPTSVaild, mpSharedRes->mbAudioStreamStartTimeValid);
                    }

                    AMLOG_INFO("[AR flow]: audio mTimeOffset = %llu.\n", mTimeOffset);
                }

                if (mpAudioOut->NeedAVSync()) {
                    renderBuffer();
                    mAudioTotalSamplesTime = mAudioTotalSamples * TICK_PER_SECOND / mAudioSamplerate;
                    if (mAudioTotalSamplesTime >= mLatency) {
                        PostEngineMsg(IEngine::MSG_AVSYNC);
                        msState = STATE_WAIT_PROPER_START_TIME;
                        return;
                    }
                } else {
                    PostEngineMsg(IEngine::MSG_AVSYNC);
                    AM_ASSERT(mpAudioOut);
                    if (true == mbAudioOutPaused && mpAudioOut) {
                        mpAudioOut->Resume();
                        mbAudioOutPaused = false;
                    }
                    renderBuffer();

                    msState = STATE_IDLE;
                    return;
                }
            }

        }
    }
}

void CAudioRenderer::onStateWaitProperTime()
{
    AM_ASSERT(!mbRecievedSyncCmd);
    CMD cmd;
    am_pts_t curtime;

    if (!mbRecievedSyncCmd) {
        //wait sync cmd first
        AMLOG_INFO("[AR flow]: onStateWaitProperTime before wait sync cmd.\n");
        while (1) {
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            if (CMD_AVSYNC == cmd.code) {
                AMLOG_INFO("[AR flow]: onStateWaitProperTime wait sync cmd done.\n");
                break;
            }
            if (msState != STATE_WAIT_PROPER_START_TIME || !mbRun) {
                AM_ERROR("[AR flow]: change state cmd comes, in waiting proper time, break out, msState %d, mbRun %d.\n", msState, mbRun);
                return;
            }
        }
    }

    curtime = mpClockManager->GetCurrentTime();
    AM_UINT skip = 0;
    if(!IsMaster() && !mpSharedRes->pbConfig.avsync_enable) {
        AMLOG_WARN("[AR flow]: not master, avsync disabled, escape from waiting logic.\n");
        skip = 1;
    }else if (!mpSharedRes->mbAudioFirstPTSVaild) {
        AMLOG_WARN("[AR flow]: No valid video first pts, must not comes here, escape from waiting logic.\n");
        skip = 1;
    } else if ((mpSharedRes->audioFirstPTS) < (curtime + mLatency + mWaitThreshold)) {
        AMLOG_INFO("[AR flow]: exactly matched time, start immediately, audio begin pts %llu, curtime %llu,  latency %u, diff %lld.\n", mpSharedRes->audioFirstPTS, curtime, mLatency, curtime + mLatency - mpSharedRes->audioFirstPTS);
        skip = 1;
    } else if ((mpSharedRes->audioFirstPTS) > (curtime + 100*mNotSyncThreshold + mLatency)) {
        AMLOG_WARN("[AR flow]: mpSharedRes->audioFirstPTS is wrong? gap greater than 100*mNotSyncThreshold.\n");
        skip = 1;
    } else {
        curtime = mpSharedRes->audioFirstPTS - mLatency;
        if (curtime > (AM_U64)100*mNotSyncThreshold) {
            skip = 1;
            AMLOG_WARN("[AR flow]: mpSharedRes->audioFirstPTS is wrong? gap greater than 100*mNotSyncThreshold1.\n");
        }
    }

    if (skip) {
        mbAudioStarted = true;
        mAudioTotalSamples = 0;
        if (mpSharedRes->mbAudioFirstPTSVaild && mpSharedRes->mbAudioStreamStartTimeValid) {
            AMLOG_PTS("[AR flow]: update mTimeOffset: old=%lld new=%lld.\n", mTimeOffset, mpSharedRes->audioFirstPTS - mpSharedRes->audioStreamStartPTS);
            // AMLOG_PTS("[AR flow]: update mTimeOffset: audioFirstPTS=%lld audioStreamStartPTS=%lld.\n", mpSharedRes->audioFirstPTS, mpSharedRes->audioStreamStartPTS);
            mTimeOffset = mpSharedRes->audioFirstPTS - mpSharedRes->audioStreamStartPTS;
        } else {
            AMLOG_PTS("[AR flow]: update mTimeOffset failure: mbAudioFirstPTSVaild=%d mbAudioStreamStartTimeValid=%d.\n", mpSharedRes->mbAudioFirstPTSVaild, mpSharedRes->mbAudioStreamStartTimeValid);
        }

        AM_ASSERT(mpAudioOut);
        if (false == mbPaused) {
            if (true == mbAudioOutPaused && mpAudioOut) {
                mpAudioOut->Resume();
                mbAudioOutPaused = false;
            } else {
                AMLOG_INFO("need do nothing, mbAudioOutPaused %d, mpAudioOut %p.\n", mbAudioOutPaused, mpAudioOut);
            }
            msState = STATE_IDLE;
            //AMLOG_PTS("[AR PTS begin, skip]: first pts =%llu  curTime=%llu, latency %u, diff %lld.\n", mpSharedRes->audioFirstPT, curtime, mLatency, curtime + mLatency - mpSharedRes->audioFirstPT);
        } else {
            AM_ASSERT(true == mbAudioOutPaused);
            //debug check
            if (false == mbAudioOutPaused && mpAudioOut) {
                mpAudioOut->Pause();
                mbAudioOutPaused = true;
            }
            AMLOG_INFO("audio start as paused.\n");
            msState = STATE_PENDING;
        }
        return;
    } else {
        AMLOG_INFO("[AR flow]: set timer %llu, cur time %llu.\n", curtime, mpClockManager->GetCurrentTime());
        mpClockManager->SetTimer(this, curtime);
    }

    AMLOG_INFO("[AR flow]: onStateWaitProperTime wait sync cmd done, wait proper time.\n");
    while (1) {
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        ProcessCmd(cmd);
        if (CMD_TIMERNOTIFY == cmd.code) {
            AMLOG_INFO("[AR flow]: onStateWaitProperTime wait timenotify done, start playback....\n");
            mbAudioStarted = true;
            AM_ASSERT(mpAudioOut);
            if (false == mbPaused) {
                if (true == mbAudioOutPaused && mpAudioOut) {
                    mpAudioOut->Resume();
                    mbAudioOutPaused = false;
                } else {
                    AMLOG_INFO("need do nothing, mbAudioOutPaused %d, mpAudioOut %p.\n", mbAudioOutPaused, mpAudioOut);
                }
                curtime = mpClockManager->GetCurrentTime();
                AMLOG_INFO("[AR PTS begin]: first pts =%llu  curTime=%llu, latency %u, diff %lld.\n", mpSharedRes->audioFirstPTS, curtime, mLatency, curtime + mLatency - mpSharedRes->audioFirstPTS);
                msState = STATE_IDLE;
            } else {
                AM_ASSERT(true == mbAudioOutPaused);
                //debug check
                if (false == mbAudioOutPaused && mpAudioOut) {
                    mpAudioOut->Pause();
                    mbAudioOutPaused = true;
                }
                AMLOG_INFO("audio start as paused.\n");
                msState = STATE_PENDING;
            }
            return;
        }
        if (msState != STATE_WAIT_PROPER_START_TIME || !mbRun) {
            AM_ERROR("[AR flow]: change state cmd comes, in waiting proper time, break out, msState %d, mbRun %d.\n", msState, mbRun);
            return;
        }
    }
}

void CAudioRenderer::onStateError()
{
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    CQueueInputPin* pPin;

    type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
    if(type == CQueue::Q_MSG) {
        ProcessCmd(cmd);
    } else {
        pPin = (CQueueInputPin*)result.pOwner;
        if (!pPin->PeekBuffer(mpBuffer)) {
            AMLOG_ERROR("No buffer?\n");
            return ;
        }

        if (mpBuffer->GetType() == CBuffer::EOS) {
            handleEOS();
            return ;
        }

        AM_RELEASE(mpBuffer);
        mpBuffer = NULL;
    }
}

void CAudioRenderer::speedUp()
{
    AM_ASSERT(mpAudioInputPin);
    if (!mpAudioInputPin) {
        return;
    }

    AM_ASSERT(!mpBuffer);
    if (mpBuffer) {
        if (CBuffer::EOS == mpBuffer->GetType()) {
            AMLOG_WARN("[Request speed up]: (audio renderer) get EOS buffer.\n");
            PostEngineMsg(IEngine::MSG_EOS);
            mpBuffer->Release();
            msState = STATE_PENDING;
            mpBuffer = NULL;
            return;
        }
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    AMLOG_WARN("[Request speed up]: (audio renderer), start purge input queue.\n");

    //purge input queue
    while (mpAudioInputPin->PeekBuffer(mpBuffer)) {
        if (CBuffer::DATA == mpBuffer->GetType()) {
            mpBuffer->Release();
            mpBuffer = NULL;
            AMLOG_WARN("[Request speed up]: (audio renderer), discard input data.\n");
        } else if (CBuffer::EOS == mpBuffer->GetType()) {
            AMLOG_WARN("[Request speed up]: (audio renderer) get EOS buffer.\n");
            PostEngineMsg(IEngine::MSG_EOS);
            msState = STATE_PENDING;
            return;
        } else {
            AM_ERROR("BAD buffer type %d.\n", mpBuffer->GetType());
            mpBuffer->Release();
            mpBuffer = NULL;
        }
    }

    AMLOG_WARN("[Request speed up]: (audio renderer) done.\n");

}

//-----------------------------------------------------------------------
//
// CAudioRendererInput
//
//-----------------------------------------------------------------------
CAudioRendererInput* CAudioRendererInput::Create(CFilter *pFilter)
{
    CAudioRendererInput* result = new CAudioRendererInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }

    return result;
}

AM_ERR CAudioRendererInput::Construct()
{
    AM_ERR err = inherited::Construct(((CAudioRenderer*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

CAudioRendererInput::~CAudioRendererInput()
{

}

AM_ERR CAudioRendererInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    CAudioRenderer* pfilter = (CAudioRenderer*) mpFilter;
    if (*pFormat->pMediaType == GUID_Decoded_Audio) {
        if (*pFormat->pFormatType == GUID_Format_FFMPEG_Media) {
            //AM_INFO("samplerate:%d,channels:%d\n",((CFFMpegMediaFormat *)pFormat)->auSamplerate, ((CFFMpegMediaFormat *)pFormat)->auChannels);
            pfilter->mAudioSampleFormat = ((CFFMpegMediaFormat *)pFormat)->auSampleFormat;
            pfilter->mAudioSamplerate = ((CFFMpegMediaFormat *)pFormat)->auSamplerate;
            pfilter->mAudioChannels = ((CFFMpegMediaFormat *)pFormat)->auChannels;
        } else if (*pFormat->pFormatType == GUID_Audio_PCM) {
            pfilter->mAudioSampleFormat = ((CAudioMediaFormat *)pFormat)->auSampleFormat;
            pfilter->mAudioSamplerate = ((CAudioMediaFormat *)pFormat)->auSamplerate;
            pfilter->mAudioChannels = ((CAudioMediaFormat *)pFormat)->auChannels;
        }
        AM_INFO("mAudioSampleFormat:%d,samplerate:%d,channels:%d\n",pfilter->mAudioSampleFormat, pfilter->mAudioSamplerate, pfilter->mAudioChannels);
        return ME_OK;
    }
    return ME_NOT_SUPPORTED;
}


