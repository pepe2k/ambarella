
/*
 * amba_video_renderer.cpp
 *
 * History:
 *    2010/10/15 - [Yu Jiankang] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "amba_video_renderer"
//#define AMDROID_DEBUG

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


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

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "ambas_vin.h"

#include "amdsp_common.h"
#include "amba_video_renderer.h"

filter_entry g_amba_video_renderer = {
	"AmbaVideoRenderer",
	CAmbaVideoRenderer::Create,
	NULL,
	CAmbaVideoRenderer::AcceptMedia,
};


//-----------------------------------------------------------------------
//
// CAmbaFrameBufferPool
//
//-----------------------------------------------------------------------
CAmbaFrameBufferPool* CAmbaFrameBufferPool::Create(const char *name, AM_UINT count)
{
	CAmbaFrameBufferPool *result = new CAmbaFrameBufferPool(name);
	if (result && result->Construct(count) != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}


//-----------------------------------------------------------------------
//
// CAmbaVideoRenderer
//
//-----------------------------------------------------------------------

IFilter* CAmbaVideoRenderer::Create(IEngine *pEngine)
{
	CAmbaVideoRenderer *result = new CAmbaVideoRenderer(pEngine);
	if (result && result->Construct() != ME_OK) {
		AM_DELETE(result);
		result = NULL;
	}
	return result;
}

int CAmbaVideoRenderer::AcceptMedia(CMediaFormat& format)
{
	if (*format.pMediaType == GUID_Amba_Decoded_Video &&
		*format.pSubType == GUID_Video_YUV420NV12)
		return 1;
	return 0;
}


AM_ERR CAmbaVideoRenderer::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	if ((mpInput = CAmbaVideoRendererInput::Create(this)) == NULL)
		return ME_ERROR;

	if ((mpAmbaFrameBufferPool = CAmbaFrameBufferPool::Create("amba frame buffer", 32)) == NULL)
		return ME_ERROR;

	mpInput->SetBufferPool(mpAmbaFrameBufferPool);

	return ME_OK;
}

CAmbaVideoRenderer::~CAmbaVideoRenderer()
{
	printf("AM_DELETE(mpInput);\n");
	AM_DELETE(mpInput);
	printf("AM_RELEASE(mpAmbaFrameBufferPool);\n");
	AM_RELEASE(mpAmbaFrameBufferPool);
	printf("~~CAmbaVideoRenderer done.\n");
}

AM_ERR CAmbaVideoRenderer::Run()
{
    mpWorkQ->Run();
    return ME_OK;
}


AM_ERR CAmbaVideoRenderer::Start()
{
    mpClockManager = (IClockManager *)mpEngine->QueryEngineInterface(IID_IClockManager);
    if(!mpClockManager) {
        AM_ERROR("CVideoRenderer::Run without mpClockManager?\n");
        return ME_ERROR;
    }
    mpWorkQ->Start();
    return ME_OK;
}



bool CAmbaVideoRenderer::ProcessCmd(CMD& cmd)
{
    iav_wake_vout_s wake;
    AM_INT ret = 0;

	switch (cmd.code) {

		case CMD_STOP:
			mbRun = false;
			CmdAck(ME_OK);
			break;

		case CMD_TIMERNOTIFY:
			if ( msState == STATE_READY ) {
				RenderBuffer(mpBuffer);
				msState = STATE_IDLE;
			} else if (msState == STATE_PENDING && mpBuffer) {
				//paused
				AM_ASSERT(mbPaused);
				RenderBuffer(mpBuffer);
			}
			break;

		case CMD_PAUSE:
			AM_ASSERT(!mbPaused);
			mbPaused = true;
			break;

            case CMD_RESUME:
                if (msState == STATE_PENDING) {
                    if(mbFlushed)
                        msState = STATE_WAIT_RENDERER_READY;
                    else
                        msState = msOldState;
                } else if (msState == STATE_FILTER_STEP_MODE){
                    if (mpBuffer) {
                        mpBuffer->Release();
                        mpBuffer = NULL;
                    }
                    msState = STATE_IDLE;
                }
                mbFlushed = false;
                mbPaused = false;
                if (!mpSharedRes->mbStartWithStepMode) {
                    //mbStepMode = false;
                    //mStepCnt = 0;
                }
                break;

		case CMD_FLUSH:
			msState = STATE_PENDING;
			mbVideoStarted = false;
			if (mpBuffer) {
				mpBuffer->Release();
				mpBuffer = NULL;
			}
			CmdAck(ME_OK);
			break;
            case CMD_START:
                CmdAck(ME_OK);
                break;

            case CMD_STEP:
                mbStepMode = true;
                mStepCnt ++;
                break;

            case CMD_AVSYNC:
                AMLOG_PRINTF("** recieve avsync cmd, state %d, get_outpic %d.\n", msState, mpSharedRes->get_outpic);
                CmdAck(ME_OK);
                AM_ASSERT(msState == STATE_WAIT_RENDERER_READY);
                if (msState == STATE_WAIT_RENDERER_READY) {
                    //start rendering
                    if (!mpSharedRes->get_outpic) {
                        AM_ASSERT(mpSharedRes->ppmode == 2);
                        msState = STATE_PPMODE2_AVSYNC;
                    } else {
                        //AM_ASSERT(mpBuffer);
                        msState = STATE_IDLE;
                    }

                    //start vout
                    wake.decoder_id = 0;//hard code here
                    AMLOG_PRINTF("get engine's avsync cmd, start wake vout, IAV_IOC_WAKE_VOUT.\n");
                    if ((ret = ioctl(mIavFd, IAV_IOC_WAKE_VOUT, &wake)) < 0) {
                        perror("IAV_IOC_WAKE_VOUT");
                        AMLOG_ERROR("IAV_IOC_WAKE_VOUT, ret %d.\n", ret);
                    }
                    AMLOG_PRINTF("IAV_IOC_WAKE_VOUT done.\n");
                }
                break;

		default:
			AM_ERROR("wrong cmd.code: %d", cmd.code);
	}
	return false;
}


//static int rendered_frame_count = 0;

void CAmbaVideoRenderer::OnRun()
{
    iav_udec_status_t status;
    iav_udec_status_t eos_status = {0};
    iav_audio_clk_offset_s offset;

    AM_INT eos_reached = 0;
    AM_S64 diff;
    am_pts_t curTime;
    am_pts_t udecTime;
    CmdAck(ME_OK);

    CMD cmd;
    CQueue::QType type;
    CQueueInputPin* pPin;
    CQueue::WaitResult result;

    mbRun = true;
    mbVideoStarted = false;

    //send ready
    AMLOG_PRINTF("VRenderer: post IEngine::MSG_READY.\n");
    PostEngineMsg(IEngine::MSG_READY);
    //wait start
    if(!ReadyWaitStart()) {
        AMLOG_PRINTF("VRenderer: get stop cmd, exit.\n");
        return;
    }
    AMLOG_PRINTF("VRenderer: start, mpSharedRes->get_outpic %d, mpClockManager %p, mIavFd %d.\n", mpSharedRes->get_outpic, mpClockManager, mIavFd);
    AM_ASSERT(mpClockManager);

    if (!mpSharedRes->get_outpic) {
        msState = STATE_WAIT_RENDERER_READY;
    } else {
        msState = STATE_IDLE;
    }

    while (mbRun) {

        //go to pending if is pasued
        if(mbPaused && msState != STATE_PENDING) {
            msOldState = msState;
            msState = STATE_PENDING;
        }

        AMLOG_STATE("Amba Renderer: start switch, msState=%d, %d input data.\n", msState, mpInput->mpBufferQ->GetDataCnt());
        switch (msState) {
            case STATE_IDLE:
                AM_ASSERT(!mpBuffer);

                //wait input data, process msg
                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                if(type == CQueue::Q_MSG) {
                    //AM_ASSERT(CMD_TIMERNOTIFY != cmd.code);
                    ProcessCmd(cmd);
                } else {
                    pPin = (CQueueInputPin*)result.pOwner;
                    if (!pPin->PeekBuffer(mpBuffer)) {
                        AM_ERROR("No buffer?\n");
                    }
                    if (mpBuffer->GetType() == CBuffer::EOS) {
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        PostEngineMsg(IEngine::MSG_EOS);
                        msState = STATE_PENDING;
                        AMLOG_PRINTF("VRenderer get EOS.\n");
                        break;
                    }

                    if (!mbVideoStarted) {
                        //first packet comes, need wait all renderer sync, and wake vout first
                        msState = STATE_WAIT_RENDERER_READY;
                        mpSharedRes->mVideoStartPTS = mpBuffer->mPTS;
                        mbVideoStarted = true;
                        AM_ASSERT(mFirstPTS == 0x0fffffff);
                        if(mFirstPTS == 0x0fffffff)
                        {
                            mFirstPTS = mpBuffer->mPTS;
                            AMLOG_INFO("the beginning of the file:: start PTS = %llu, first pts = %llu.\n", mpBuffer->mPTS, mFirstPTS);
                        }
                        mTimeOffset =mpBuffer->mPTS - mFirstPTS;
                        AMLOG_INFO("VRenderer: start PTS = %llu.\n", mpBuffer->mPTS);
                        RenderBuffer(mpBuffer);

                        //wait first pts out first
                        while (1) {
                            //check there's cmd
                            if (mpWorkQ->PeekCmd(cmd)) {
                                if (cmd.code == CMD_STOP) {
                                    CmdAck(ME_OK);
                                    mbRun = false;
                                    AMLOG_ERROR("recieve STOP when waiting first pts, exit OnRun.\n");
                                    return;
                                }
                                AMLOG_ERROR("How to process these cmd %d 1.\n", cmd.code);
                                CmdAck(ME_OK);
                                break;
                            }
                            memset(&status, 0, sizeof(status));
                            status.decoder_id = 0;//hard code here, todo
                            status.nowait = 0;
                            AMLOG_PRINTF("start IAV_IOC_WAIT_UDEC_STATUS\n");
                            if (ioctl(mIavFd, IAV_IOC_WAIT_UDEC_STATUS, &status) < 0) {
                                //AM_PERROR("IAV_IOC_WAIT_UDEC_STATUS");
                                perror("IAV_IOC_WAIT_UDEC_STATUS");
                                AMLOG_PRINTF("error IAV_IOC_WAIT_UDEC_STATUS, pts 0x%x, 0x%x.\n", status.pts_high, status.pts_low);
                                break;
                            }
                            AMLOG_PRINTF("IAV_IOC_WAIT_UDEC_STATUS done.\n");

                            if (status.pts_high == 0xffffffff && status.pts_low == 0xffffffff) {
                                continue;
                            }
                            AMLOG_PRINTF("Start pts comes h %d, l %d.\n", status.pts_high, status.pts_low);
                            break;
                        }

                        //send ready
                        AMLOG_PRINTF("VRenderer: post IEngine::MSG_AVSYNC.\n");
                        PostEngineMsg(IEngine::MSG_AVSYNC);
                    } else if (mbStepMode && msState != STATE_FILTER_STEP_MODE) {
                        //clear cnt
                        mStepCnt = 1;
                        msState = STATE_FILTER_STEP_MODE;
                    } else {
                        msState = STATE_HAS_INPUTDATA;
                    }
                }
                break;

            case STATE_HAS_INPUTDATA:
                AM_ASSERT(mpBuffer);
                msState = STATE_IDLE;
                RenderBuffer(mpBuffer);

                if (mAdjustInterval > 0) {
                    mAdjustInterval --;
                    break;
                }
                mAdjustInterval = D_AVSYNC_ADJUST_INTERVEL;

                //avsync
                //get last udec pts, estimate current udec time
                memset(&status, 0, sizeof(status));
                status.decoder_id = 0;//hard code here, todo
                status.nowait = 1;
                //AMLOG_PRINTF("start IAV_IOC_WAIT_UDEC_STATUS\n");
                if (ioctl(mIavFd, IAV_IOC_WAIT_UDEC_STATUS, &status) < 0) {
                    AM_PERROR("IAV_IOC_WAIT_UDEC_STATUS");
                    AMLOG_PRINTF("error IAV_IOC_WAIT_UDEC_STATUS\n");
                    break;
                }
                if (status.pts_high == 0xffffffff && status.pts_low == 0xffffffff) {
                    break;
                }
                curTime = mpClockManager->GetCurrentTime();
                udecTime = (((am_pts_t)status.pts_high) << 32)|((am_pts_t)status.pts_low);
                diff = curTime - udecTime;

#ifdef AM_DEBUG
                AMLOG_PTS("    video renderer rendering, curTime =%llu , udecTime %llu, diff=%lld.\n", curTime, udecTime, diff);
#endif

                //1500 = a half of frame intervel, udec_time is not current pts, it's last pts on screen
                diff -= 1500;
                if (!(mLogOutput & LogDisableNewPath)) {
                    if (diff > mNotSyncThreshold || diff < (-mNotSyncThreshold)) {
                        offset.decoder_id = 0;//hard code here
                        offset.audio_clk_offset = ((AM_S32)diff) >> 2;

                        AMLOG_PTS(" [AV Sync], offset.audio_clk_offset %d.\n", offset.audio_clk_offset);
                        if (ioctl(mIavFd, IAV_IOC_SET_AUDIO_CLK_OFFSET, &offset) < 0) {
                            AM_PERROR("IAV_IOC_SET_AUDIO_CLK_OFFSET");
                            AMLOG_PRINTF("error IAV_IOC_SET_AUDIO_CLK_OFFSET\n");
                        }
                    }
                }
/*
				if(IsMaster()) {
					//master should sync startPTS/startTime for clockmanager when needed
					//note: diff > 10*mNotSyncThreshold is prevent the case if the video decoding is very fast, the video renderer will work very fast.
					if((diff > 10*mNotSyncThreshold) || diff < (-mNotSyncThreshold)) {
						mpClockManager->SetClockMgr(mpBuffer->mPTS);
					} else if(diff > mWaitThreshold) {
						mpClockManager->SetTimer(this, mpBuffer->mPTS);
						msState = STATE_READY;
						break;
					}
					RenderBuffer(mpBuffer);
				} else {
					//slave should consider time/PTS differerence: discard/render immediately/render later
					if(diff < (-mWaitThreshold) || (diff > 10*mNotSyncThreshold)) { //10*mNotSyncThreshold is a pts error detection
						AMLOG_PTS("    video renderer, discard.\n");
						DiscardBuffer(mpBuffer);
					} else if (diff > mWaitThreshold) {
						AMLOG_PTS("    video renderer, waiting.\n");
						mpClockManager->SetTimer(this, mpBuffer->mPTS);
						msState = STATE_READY;
						break;
					} else {
						//proper time
						AMLOG_PTS("    video renderer, proper time.\n");
						RenderBuffer(mpBuffer);
					}
				}
*/
                break;

            case STATE_ERROR:
            case STATE_PENDING:
            case STATE_READY:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_WAIT_RENDERER_READY:
                AM_ASSERT(mpSharedRes);

                if (!mpSharedRes->get_outpic && !mbVideoStarted) {
                    if (mpWorkQ->PeekCmd(cmd)) {
                        ProcessCmd(cmd);
                        break;
                    }

                    //wait first pts comes
                    memset(&status, 0, sizeof(status));
                    status.decoder_id = 0;//hard code here, todo
                    status.nowait = 0;
                    AMLOG_PRINTF("start IAV_IOC_WAIT_UDEC_STATUS\n");
                    if (ioctl(mIavFd, IAV_IOC_WAIT_UDEC_STATUS, &status) < 0) {
                        //AM_PERROR("IAV_IOC_WAIT_UDEC_STATUS");
                        perror("IAV_IOC_WAIT_UDEC_STATUS");
                        AMLOG_PRINTF("error IAV_IOC_WAIT_UDEC_STATUS, pts 0x%x, 0x%x.\n", status.pts_high, status.pts_low);
                        break;
                    }
                    AMLOG_PRINTF("IAV_IOC_WAIT_UDEC_STATUS done.\n");

                    if (status.pts_high == 0xffffffff && status.pts_low == 0xffffffff) {
                        break;
                    }
                    AMLOG_PRINTF("Start pts comes h %d, l %d.\n", status.pts_high, status.pts_low);

                    mbVideoStarted = true;
                    mpSharedRes->mVideoStartPTS = ((am_pts_t)status.pts_low) | (((am_pts_t)status.pts_high)<<32);
                    PostEngineMsg(IEngine::MSG_AVSYNC);
                } else {
                    //wait other renderers ready
                    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                    ProcessCmd(cmd);
                    break;
                }
                break;

            case STATE_PPMODE2_AVSYNC:
                AM_ASSERT(!mpBuffer);
                if (mbStepMode) {
                    AMLOG_PRINTF("enter step mode %d.\n", msState);
                    msState = STATE_FILTER_STEP_MODE;
                    mStepCnt = 1;
                    break;
                }
                //ppmode = 2, watch udec/dsp's pts and do avsync issues
                //check if there's cmd
                if (mpWorkQ->PeekCmd(cmd)) {
                    ProcessCmd(cmd);
                    break;
                }

                if (mpInput->PeekBuffer(mpBuffer)) {
                    if (mpBuffer->GetType() == CBuffer::EOS) {
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        AM_PRINTF("Amba Renderer Get EOS, go to watch eos stage.\n");
                        msState = STATE_PPMODE2_WAIT_EOS;
                        break;
                    }
                }

                //wait udec status
                memset(&status, 0, sizeof(status));
                status.decoder_id = 0;//hard code here, todo
                status.nowait = 0;
                //AMLOG_PRINTF("start IAV_IOC_WAIT_UDEC_STATUS\n");
                if (ioctl(mIavFd, IAV_IOC_WAIT_UDEC_STATUS, &status) < 0) {
                    AM_PERROR("IAV_IOC_WAIT_UDEC_STATUS");
                    AMLOG_PRINTF("error IAV_IOC_WAIT_UDEC_STATUS..\n");
                    //msState = STATE_PENDING;
                    break;
                }
                //AMLOG_PRINTF("IAV_IOC_WAIT_UDEC_STATUS done.\n");

                if (status.eos_flag) {
                    eos_reached = 1;
                    eos_status = status;
                    AMLOG_PRINTF("!!!!Get udec eos flag(ppmode2), send data done.\n");
                    AMLOG_PRINTF("**get last udec pts l %d h %d.\n", status.pts_low, status.pts_high);
                    status.eos_flag=0;
                    if (eos_reached && status.eos_flag == 0 &&
                        eos_status.pts_low == status.pts_low && eos_status.pts_high == status.pts_high) {
                        AMLOG_PRINTF("!!!!Get last pts, eos comes 1.\n");
                        PostEngineMsg(IEngine::MSG_EOS);
                        msState = STATE_PENDING;
                        AM_PRINTF("Amba Renderer Get EOS!\n");
                        break;
                    }
                    //go to wait eos stage
                    msState = STATE_PPMODE2_WAIT_EOS;
                    break;
                }

                //avsync IAV_IOC_SET_AUDIO_CLK_OFFSET
                curTime = mpClockManager->GetCurrentTime();
                udecTime = (((am_pts_t)status.pts_high) << 32)|((am_pts_t)status.pts_low);
                diff = curTime - udecTime;
                AMLOG_PTS("   AM VRender: get udec time=%llu , curTime =%llu , diff=%lld.\n", udecTime, curTime, diff);

                if (!(mLogOutput & LogDisableNewPath)) {
                    if (mAdjustInterval > 0) {
                        mAdjustInterval --;
                    } else {
                        if (diff > mNotSyncThreshold || diff < (-mNotSyncThreshold)) {
                            offset.decoder_id = 0;//hard code here
                            offset.audio_clk_offset = ((AM_S32)diff) >> 2;

                            AMLOG_PTS(" [AV Sync], offset.audio_clk_offset %d.\n", offset.audio_clk_offset);
                            if (ioctl(mIavFd, IAV_IOC_SET_AUDIO_CLK_OFFSET, &offset) < 0) {
                                AM_PERROR("IAV_IOC_SET_AUDIO_CLK_OFFSET");
                                AMLOG_PRINTF("error IAV_IOC_SET_AUDIO_CLK_OFFSET\n");
                            }
                            mAdjustInterval = D_AVSYNC_ADJUST_INTERVEL;
                        }
                    }
                }
                break;

            case STATE_PPMODE2_WAIT_EOS:
                AM_ASSERT(!mpBuffer);
                //AM_ASSERT(eos_reached);
                //ppmode = 2, wait udec eos
                //check if there's cmd
                if (mpWorkQ->PeekCmd(cmd)) {
                    ProcessCmd(cmd);
                    break;
                }

                //wait udec status
                memset(&status, 0, sizeof(status));
                status.decoder_id = 0;//hard code here, todo
                status.nowait = 0;
                AMLOG_PRINTF("start IAV_IOC_WAIT_UDEC_STATUS\n");
                if (ioctl(mIavFd, IAV_IOC_WAIT_UDEC_STATUS, &status) < 0) {
                    AM_PERROR("IAV_IOC_WAIT_UDEC_STATUS");
                    AMLOG_PRINTF("error IAV_IOC_WAIT_UDEC_STATUS, post eos msg\n");
                    PostEngineMsg(IEngine::MSG_EOS);
                    msState = STATE_PENDING;
                    break;
                }
                AMLOG_PRINTF("IAV_IOC_WAIT_UDEC_STATUS done.\n");

                if (status.eos_flag) {
                    eos_reached = 1;
                    eos_status = status;
                    AMLOG_PRINTF("!!!!Get udec eos flag(ppmode2), send data done.\n");
                    AMLOG_PRINTF("**get last udec pts l %d h %d.\n", status.pts_low, status.pts_high);
                    status.eos_flag=0;
                    if (eos_reached && status.eos_flag == 0 &&
                        eos_status.pts_low == status.pts_low && eos_status.pts_high == status.pts_high) {
                        AMLOG_PRINTF("!!!!Get last pts, eos comes 1.\n");
                        PostEngineMsg(IEngine::MSG_EOS);
                        msState = STATE_PENDING;
                        AM_PRINTF("Amba Renderer Get EOS!\n");
                        break;
                    }
                }

                if (eos_reached && status.eos_flag==0 && eos_status.pts_low == status.pts_low && eos_status.pts_high == status.pts_high) {
                    AMLOG_PRINTF("!!!!Get last pts, eos comes 2.\n");
                    PostEngineMsg(IEngine::MSG_EOS);
                    msState = STATE_PENDING;
                    AM_PRINTF("Amba Renderer Get EOS!\n");
                    break;
                } else {
                    //computing current time
                    udecTime = (((am_pts_t)status.pts_high) << 32)|((am_pts_t)status.pts_low);
                    diff = udecTime - curTime;
                    AMLOG_PTS("   AM VRender: get udec time=%llu , curTime =%llu , diff=%lld .\n", udecTime, curTime, diff);
                    AMLOG_PRINTF("**get udec pts l %d h %d.\n", status.pts_low, status.pts_high);
                }
                break;

            case STATE_FILTER_STEP_MODE:
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
                                break;
                            }
                            if (mpBuffer->GetType() == CBuffer::EOS) {
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                PostEngineMsg(IEngine::MSG_EOS);
                                msState = STATE_PENDING;
                                AM_PRINTF("Amba Renderer Get EOS!\n");
                                break;
                            }
                            RenderBuffer(mpBuffer);
                            mStepCnt -- ;
                            mpBuffer = NULL;
                        }
                    }else {
                        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                        ProcessCmd(cmd);
                    }
                } else {
                    iav_udec_trickplay_t trickplay;
                    trickplay.decoder_id = 0; //hard code here
                    trickplay.mode = 2; //step
                    AM_INT mRet = 0;

                    if (mStepCnt) {
                        AMLOG_PRINTF("****VideoRenderer, STEP: start IAV_IOC_UDEC_TRICKPLAY.\n");
                        mRet = ioctl(mIavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
                        AMLOG_PRINTF("****VideoDecoder, STEP: IAV_IOC_UDEC_TRICKPLAY done, ret %d.\n", mRet);
                        mStepCnt -- ;
                        if (mpWorkQ->PeekCmd(cmd)) {
                            ProcessCmd(cmd);
                            break;
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
                                break;
                            }
                            if (mpBuffer->GetType() == CBuffer::EOS) {
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                PostEngineMsg(IEngine::MSG_EOS);
                                msState = STATE_PENDING;
                                AM_PRINTF("Amba Renderer Get EOS!\n");
                                break;
                            }
                        }
                    }
                }
                break;

            default:
                AM_ERROR("error state: %d.\n", msState);
        }
    }

    if(mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    AMLOG_INFO("AmbaVideoRenderer OnRun: end.\n");
}

void CAmbaVideoRenderer::Delete()
{
	AM_RELEASE(mpAmbaFrameBufferPool);
	mpAmbaFrameBufferPool = NULL;
	AM_DELETE(mpInput);
	mpInput = NULL;

	inherited::Delete();
}

void CAmbaVideoRenderer::DiscardBuffer(CBuffer*& pBuffer)
{
	//to do, just rendering it here for testing
	RenderBuffer(pBuffer);
}



void CAmbaVideoRenderer::RenderBuffer(CBuffer*& pBuffer)
{
	iav_decoded_frame_t *pframe = (iav_decoded_frame_t*)((AM_U8*)mpBuffer + sizeof(CBuffer));

	//rendered_frame_count++;

	if (::ioctl(mIavFd, IAV_IOC_RENDER_FRAME, pframe) < 0) {
		AM_PERROR("IAV_IOC_RENDER_FRAME");
		//return -1;
	}

	pBuffer->Release();
//	pBuffer = NULL;
}


AM_ERR CAmbaVideoRenderer::SetInputFormat(CMediaFormat *pFormat)
{
	mIavFd = pFormat->format;
	return ME_OK;
}

AM_ERR CAmbaVideoRenderer::OnTimer(am_pts_t curr_pts)
{
	mpWorkQ->PostMsg(CMD_TIMERNOTIFY);
	return ME_OK;
}

void* CAmbaVideoRenderer::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IClockObserver)
        return (IClockObserver*)this;
    else if (refiid == IID_IVideoOutput)
        return (IVideoOutput*)this;
    else if (refiid == IID_IRenderer)
        return (IRender*)this;

    return inherited::GetInterface(refiid);
}

bool CAmbaVideoRenderer::ReadyWaitStart()
{
    AO::CMD cmd;

    while (1) {
        AMLOG_INFO("CVideoRenderer::ReadyWaitStart...\n");
        GetCmd(cmd);
        if (cmd.code == AO::CMD_START) {
            AMLOG_INFO("CVideoRenderer::ReadyWaitStart done.\n");
            CmdAck(ME_OK);
            return true;
        }

        if (cmd.code == AO::CMD_STOP) {
            AMLOG_INFO("CVideoRenderer::ReadyWaitStart stop comes...\n");
            CmdAck(ME_OK);
            return false;
        }
    }
}

AM_ERR CAmbaVideoRenderer::GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs)
{
    absoluteTimeMs = mLastPTS/90;
    relativeTimeMs = mLastPTS/90;

    //AMLOG_PTS("GetCurrentTime: absoluteTimeMs=%llu relativeTimeMs=%llu--------\n", absoluteTimeMs, relativeTimeMs);
    return ME_OK;
}

AM_ERR CAmbaVideoRenderer::Step()
{
    mpWorkQ->PostMsg(CMD_STEP);
    return ME_OK;
}

AM_ERR CAmbaVideoRenderer::SetDisplayPositionSize(AM_INT vout, AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height)
{
return ME_NO_IMPL;//without libvout
#if 0
    AMLOG_INFO("SetDisplayPositionSize: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, pos_x, pos_y, width, height);
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayPositionSize: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    //check valid
    if ((pos_x + width) > mVoutConfig[vout].width || (pos_y + height) > mVoutConfig[vout].height || (width < 0) || (height < 0) || (pos_x < 0) || (pos_y < 0)) {
        AMLOG_ERROR("SetDisplayPositionSize: pos&size out of range.\n");
        return ME_BAD_PARAM;
    }

    AM_INT ret;
    ret = ambarella_vout_change_video_offset((u32)vout, (u32)pos_x, (u32)pos_y);
    if (ret) {
        AMLOG_ERROR("ambarella_vout_change_video_offset fail. ret %d, pos_x %d, pos_y %d .\n", ret, pos_x, pos_y);
        return ME_OS_ERROR;
    }
    mVoutConfig[vout].pos_x = pos_x;
    mVoutConfig[vout].pos_y = pos_y;

    ret = ambarella_vout_change_video_size((u32)vout, (u32)width, (u32)height);
    if (ret) {
        AMLOG_ERROR("ambarella_vout_change_video_size fail. ret %d, width %d, height %d .\n", ret, width, height);
        return ME_OS_ERROR;
    }
    mVoutConfig[vout].size_x = width;
    mVoutConfig[vout].size_y = height;

    AMLOG_INFO("SetDisplayPositionSize done: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    return ME_OK;
#endif
}


AM_ERR CAmbaVideoRenderer::GetDisplayPositionSize(AM_INT vout, AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height)
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


AM_ERR CAmbaVideoRenderer::SetDisplayPosition(AM_INT vout, AM_INT pos_x, AM_INT pos_y)
{
return ME_NO_IMPL;//without libvout
#if 0
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
    ret = ambarella_vout_change_video_offset((u32)vout, (u32)pos_x, (u32)pos_y);
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

AM_ERR CAmbaVideoRenderer::SetDisplaySize(AM_INT vout, AM_INT width, AM_INT height)
{
return ME_NO_IMPL;//without libvout
#if 0
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

    AM_INT ret;

    ret = ambarella_vout_change_video_size((u32)vout, (u32)width, (u32)height);
    if (ret) {
        AMLOG_ERROR("ambarella_vout_change_video_size fail. ret %d, width %d, height %d .\n", ret, width, height);
        return ME_OS_ERROR;
    }
    mVoutConfig[vout].size_x = width;
    mVoutConfig[vout].size_y = height;

    AMLOG_INFO("SetDisplaySize done: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    return ME_OK;
#endif
}

AM_ERR CAmbaVideoRenderer::GetDisplayDimension(AM_INT vout, AM_INT* width, AM_INT* height)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("GetDisplayDimension: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    *width = mVoutConfig[vout].width;
    *height = mVoutConfig[vout].height;

    return ME_OK;
}

AM_ERR CAmbaVideoRenderer::GetVideoPictureSize(AM_INT* width, AM_INT* height)
{
    *width = mPicWidth;
    *height = mPicHeight;
    AMLOG_INFO("GetVideoPictureSize, get picture width %d, height %d.\n", *width, *height);
    return ME_OK;
}

/*
AM_ERR CAmbaVideoRenderer::GetDisplayRotation(AM_INT vout, AM_INT* degree)
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
            AMLOG_PRINTF("GetDisplayRotation, has flip state %d.\n", mVoutConfig[vout].rotationflip);
            *degree = 0;//set to zero?
            break;
        case AMBA_VOUT_FLIP_VERTICAL_ROTATE_90:
        case AMBA_VOUT_FLIP_HORIZONTAL_ROTATE_90:
        case AMBA_VOUT_FLIP_HV_ROTATE_90:
            AMLOG_PRINTF("GetDisplayRotation, has flip state %d.\n", mVoutConfig[vout].rotationflip);
            *degree = 90;//set to 90?
            break;
        default:
            AMLOG_ERROR("GetDisplayRotation, error rotation/flip states.\n");
            return ME_ERROR;
    }
    return ME_OK;
}
*/

AM_ERR CAmbaVideoRenderer::SetDisplayRotation(AM_INT vout, AM_INT degree)
{
return ME_NO_IMPL;//without libvout
#if 0
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayRotation: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    AM_INT ret, rotate, target;

    if (degree == 0) {
        rotate = 0;
        switch (mVoutConfig[vout].rotationflip) {
            case AMBA_VOUT_FLIP_NORMAL_ROTATE_90:
                target = AMBA_VOUT_FLIP_NORMAL;
                break;
            case AMBA_VOUT_FLIP_HORIZONTAL_ROTATE_90:
                target = AMBA_VOUT_FLIP_HORIZONTAL;
                break;
            case AMBA_VOUT_FLIP_VERTICAL_ROTATE_90:
                target = AMBA_VOUT_FLIP_VERTICAL;
                break;
            case AMBA_VOUT_FLIP_HV_ROTATE_90:
                target = AMBA_VOUT_FLIP_HV;
                break;
            default:
                AMLOG_INFO("need do nothing.\n");
                return ME_OK;
        }
    } else {
        rotate = 1;
        switch (mVoutConfig[vout].rotationflip) {
            case AMBA_VOUT_FLIP_NORMAL:
                target = AMBA_VOUT_FLIP_NORMAL_ROTATE_90;
                break;
            case AMBA_VOUT_FLIP_HORIZONTAL:
                target = AMBA_VOUT_FLIP_HORIZONTAL_ROTATE_90;
                break;
            case AMBA_VOUT_FLIP_VERTICAL:
                target = AMBA_VOUT_FLIP_VERTICAL_ROTATE_90;
                break;
            case AMBA_VOUT_FLIP_HV:
                target = AMBA_VOUT_FLIP_HV_ROTATE_90;
                break;
            default:
                AMLOG_INFO("need do nothing.\n");
                return ME_OK;
        }
    }

    ret = ambarella_vout_rotate_video((u32)vout, rotate);

    if (!ret) {
        AMLOG_INFO("'SetDisplayRotation(%d) done, ori %d, target %d.\n", vout, mVoutConfig[vout].rotationflip, target);
        mVoutConfig[vout].rotationflip = target;
        return ME_OK;
    }

    AMLOG_ERROR("'SetDisplayRotation(%d) fail, ret %d, ori %d, target %d.\n", vout, ret, mVoutConfig[vout].rotationflip, target);
    return ME_OS_ERROR;
#endif
}

AM_ERR CAmbaVideoRenderer::SetDisplayFlip(AM_INT vout, AM_INT flip)
{
return ME_NO_IMPL;//without libvout
#if 0
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayFlip: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    AMLOG_INFO("'SetDisplayFlip(%d), flip %d, ori %d.\n", vout, flip, mVoutConfig[vout].rotationflip);

    AM_INT ret, target;
    if (flip == 0) {
        switch (mVoutConfig[vout].rotationflip) {
            case AMBA_VOUT_FLIP_VERTICAL:
                target = AMBA_VOUT_FLIP_NORMAL;
                break;
            case AMBA_VOUT_FLIP_HV:
                target = AMBA_VOUT_FLIP_HORIZONTAL;
                break;
            case AMBA_VOUT_FLIP_VERTICAL_ROTATE_90:
                target = AMBA_VOUT_FLIP_NORMAL_ROTATE_90;
                break;
            case AMBA_VOUT_FLIP_HV_ROTATE_90:
                target = AMBA_VOUT_FLIP_HORIZONTAL_ROTATE_90;
                break;
            default:
                AMLOG_INFO("need do nothing.\n");
                return ME_OK;
        }
    } else {
        switch (mVoutConfig[vout].rotationflip) {
            case AMBA_VOUT_FLIP_NORMAL:
                target = AMBA_VOUT_FLIP_VERTICAL;
                break;
            case AMBA_VOUT_FLIP_HORIZONTAL:
                target = AMBA_VOUT_FLIP_HV;
                break;
            case AMBA_VOUT_FLIP_NORMAL_ROTATE_90:
                target = AMBA_VOUT_FLIP_VERTICAL_ROTATE_90;
                break;
            case AMBA_VOUT_FLIP_HORIZONTAL_ROTATE_90:
                target = AMBA_VOUT_FLIP_HV_ROTATE_90;
                break;
            default:
                AMLOG_INFO("need do nothing.\n");
                return ME_OK;
        }
    }

    ret = ambarella_vout_flip_video((u32)vout, (amba_vout_flipate_info)target);

    if (!ret) {
        AMLOG_INFO("'SetDisplayFlip(%d) done, ori %d, target %d.\n", vout, mVoutConfig[vout].rotationflip, target);
        mVoutConfig[vout].rotationflip = target;
        return ME_OK;
    }

    AMLOG_ERROR("'SetDisplayFlip(%d) fail, ret %d, ori %d, target %d.\n", vout, ret, mVoutConfig[vout].rotationflip, target);
    return ME_OS_ERROR;
#endif
}

AM_ERR CAmbaVideoRenderer::SetDisplayMirror(AM_INT vout, AM_INT mirror)
{
return ME_NO_IMPL;//without libvout
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

AM_ERR CAmbaVideoRenderer::EnableVout(AM_INT vout, AM_INT enable)
{
return ME_NO_IMPL;//without libvout
#if 0
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("EnableVout: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    AM_INT ret = ambarella_vout_enable_video((u32)vout, (u32)enable);
    if (ret) {
        AMLOG_ERROR("ambarella_vout_enable_video(%d, %d), fail ret = %d.\n", vout, enable, ret);
        return ME_OS_ERROR;
    }

    return ME_OK;
#endif
}

AM_ERR CAmbaVideoRenderer::SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h)
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

AM_ERR CAmbaVideoRenderer::SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h)
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

AM_ERR CAmbaVideoRenderer::SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y)
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

#ifdef AM_DEBUG
void CAmbaVideoRenderer::PrintState()
{
    AMLOG_PRINTF("CAmbaVideoRenderer: msState=%d, %d input data.\n", msState, mpInput->mpBufferQ->GetDataCnt());
}
#endif

//-----------------------------------------------------------------------
//
// CAmbaRendererInput
//
//-----------------------------------------------------------------------
CAmbaVideoRendererInput* CAmbaVideoRendererInput::Create(CFilter *pFilter)
{
	CAmbaVideoRendererInput *result = new CAmbaVideoRendererInput(pFilter);
	if (result && result->Construct() != ME_OK) {
		AM_DELETE(result);
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaVideoRendererInput::Construct()
{
	AM_ERR err = inherited::Construct(((CAmbaVideoRenderer*)mpFilter)->MsgQ());
	if (err != ME_OK)
		return err;

	return ME_OK;
}

CAmbaVideoRendererInput::~CAmbaVideoRendererInput()
{
	//printf("~CAmbaRendererInput.\n");
	// todo
}

AM_ERR CAmbaVideoRendererInput::CheckMediaFormat(CMediaFormat *pFormat)
{
	if (*pFormat->pMediaType == GUID_Amba_Decoded_Video &&
			*pFormat->pSubType == GUID_Video_YUV420NV12 &&
			*pFormat->pFormatType == GUID_Format_FFMPEG_Media) {
		((CAmbaVideoRenderer*)mpFilter)->SetInputFormat(pFormat);
		return ME_OK;
	}

	return ME_NOT_SUPPORTED;
}


