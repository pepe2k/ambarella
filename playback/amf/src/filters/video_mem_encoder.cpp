
/*
 * video_mem_encoder.cpp
 *
 * History:
 *    2011/7/29 - [GangLiu] create file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "video_mem_encoder"
//#define AMDROID_DEBUG
//#define RECORD_TEST_FILE

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <basetypes.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "ve_if.h"
#include "engine_guids.h"

#include "iav_drv.h"
#include "iav_transcode_drv.h"
#include "video_mem_encoder.h"

//#ifdef print_framerate
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>

static struct timeval pre_time = {0,0};
static struct timeval curr_time = {0,0};
//#endif

#ifdef AM_DEBUG
char g_dumpTranscesPath[DAMF_MAX_FILENAME_LEN+1];
#endif

IFilter* CreateVideoMemEncoderFilter(IEngine *pEngine)
{
	return CVideoMemEncoder::Create(pEngine);
}
//-----------------------------------------------------------------------
//
// CVideoMemEncoderBufferPool
//
//-----------------------------------------------------------------------
void CVideoMemEncoderBufferPool::OnReleaseBuffer(CBuffer *pBuffer)
{
    if(!(pBuffer->mFlags & IAV_ENC_BUFFER)){
        return;
    }
    iav_frame_desc_t frame;
    AM_INT err;
    memset(&frame, 0, sizeof(frame));
    frame.fd_id = pBuffer->mReserved;
    if ((err = ::ioctl(iavfd, IAV_IOC_RELEASE_ENCODED_FRAME2, &frame)) < 0) {
        AM_WARNING("!Just used an expired frame fid %u, iav %d, return %d.\n", frame.fd_id, iavfd, err);
    }
    pBuffer->mFlags = 0;
}
//===============================================
//
//===============================================
IFilter* CVideoMemEncoder::Create(IEngine *pEngine)
{
    CVideoMemEncoder *result = new CVideoMemEncoder(pEngine);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CVideoMemEncoder::Construct()
{
//    AM_INT i = 0;

    DSetModuleLogConfig(LogModuleVideoMemEncoder);

    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    mpInputPin = CVideoMemEncoderInput::Create((CFilter *)this);
    if (mpInputPin == NULL) {
        AM_ERROR("CVideoMemEncoderInput::Create fail in CVideoMemEncoder::Construct().\n");
        return ME_NO_MEMORY;
    }

    mpOutputPin = CVideoMemEncoderOutput::Create((CFilter *)this);
    if (mpOutputPin == NULL) {
        AM_ERROR("CVideoMemEncoderOutput::Create fail in CVideoMemEncoder::Construct().\n");
        return ME_NO_MEMORY;
    }
    mpOutputPin->mpBSB = CVideoMemEncoderBufferPool::Create("EncBSB", 64);
    if (mpOutputPin->mpBSB == NULL) {
        AM_ERROR("CVideoMemEncoderBufferPool::Create fail in CVideoMemEncoder::Construct().\n");
        return ME_NO_MEMORY;
    }
    mpOutputPin->SetBufferPool(mpOutputPin->mpBSB);

    mpClockManager = (IClockManager *)mpEngine->QueryEngineInterface(IID_IClockManagerExt);

#ifdef AM_DEBUG
    //load dump file path
    snprintf(g_dumpTranscesPath, sizeof(g_dumpTranscesPath), "%s/transc_output_es.h264", AM_GetPath(AM_PathDump));
#endif

    AMLOG_DEBUG("CVideoMemEncoder::Construct done.\n");
    return ME_OK;
}

CVideoMemEncoder::~CVideoMemEncoder()
{
    AM_DELETE(mpInputPin);
    mpInputPin= NULL;
    AM_DELETE(mpOutputPin);
    mpOutputPin = NULL;
#ifdef RECORD_TEST_FILE
    mpVFile->CloseFile();
#endif
}

AM_ERR CVideoMemEncoder::SetInputFormat(CMediaFormat *pFormat)
{
    mIavFd = pFormat->format;
    AM_INFO("CVideoMemEncoder SetInputFormat(), mIavFd = %d.\n", mIavFd);
    AM_ERR err = mpOutputPin->SetOutputFormat();
    if (err != ME_OK)
        return err;
    return ME_OK;
}

void CVideoMemEncoder::GetInfo(INFO& info)
{
    info.nInput = 1;
    info.nOutput = 0;//0 now
    info.pName = "AmbaMemVEnc";
}

IPin* CVideoMemEncoder::GetOutputPin(AM_UINT index)
{
    return mpOutputPin;
}
//===============================================
//
//===============================================
AM_ERR CVideoMemEncoder::SetConfig()
{
    //refer to video_encode_iav.cpp
    mFrameLifeTime = 270000;//hardcode here for transcoding

    return ME_OK;
}

AM_ERR CVideoMemEncoder::setupVideoMemEncoder()
{
    AMLOG_INFO("CVideoMemEncoder::setupVideoMemEncoder:\n");
//    AM_UINT i = 0;
    if(SetConfig() != ME_OK)
    {
        AM_PERROR("SetConfig Failed!\n");
        return ME_ERROR;
    }

    if(StartEncoder()!=ME_OK){
        return ME_ERROR;
    }
    return ME_OK;
}

bool CVideoMemEncoder::ProcessCmd(CMD& cmd)
{
    //AM_ERR err = ME_OK;
    switch(cmd.code)
    {
    case CMD_STOP:
        //DoStop();
        mbRun = false;
        CmdAck(ME_OK);
        break;

    case CMD_OBUFNOTIFY:
        break;

    case CMD_SOURCE_FILTER_BLOCKED:
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return 1;
}

AM_ERR CVideoMemEncoder::StartEncoder()
{
    if(mbEncoderStarted){
        AMLOG_INFO("CVideoMemEncoder StartEncoder: encoder is running now!\n");
        return ME_OK;
    }
/*    // start encoding
    AMLOG_INFO("CVideoMemEncoder IAV_IOC_START_ENCODE2 stream[%d]:\n", IAV_ENC_ID_MAIN);
    if (::ioctl(mIavFd, IAV_IOC_START_ENCODE2, IAV_ENC_ID_MAIN) < 0) {
        AM_ERROR("IAV_IOC_START_ENCODE2 error, mIavFd: %d.\n", mIavFd);
        AM_PERROR("IAV_IOC_START_ENCODE2\n");
        return ME_ERROR;
    }*/
    mbEncoderStarted = true;
    AMLOG_INFO("CVideoMemEncoder IAV_IOC_START_ENCODE2 stream[%d] done!\n", IAV_ENC_ID_MAIN);

    return ME_OK;
}
void CVideoMemEncoder::OnRun()
{
    CmdAck(ME_OK);
//    CQueue::QType type;
//    CQueue::WaitResult result;
    CMD cmd;
    AM_ERR err;

    AMLOG_INFO("CVideoMemEncoder::OnRun start.\n");
    err = setupVideoMemEncoder();
    if (err != ME_OK) {
        AM_ASSERT(0);
        AM_ERROR("setupVideoMemEncoder fail.\n");
        msState = STATE_ERROR;
    }

    //mpBufferPool = mpOutputPin->mpBufferPool;
    mbRun = true;
    msState = STATE_IDLE;
    //SendOutInfo();

#ifdef AM_DEBUG
    //dump data write to file
    int out_fd = -1;     // output coded file handle
    if (mLogOutput & LogDumpTotalBinary) {
        if ((out_fd = open(g_dumpTranscesPath, O_CREAT | O_WRONLY | O_TRUNC, 0777)) <= 0) {
            AM_PERROR("open file error!\n");
            //return;
        }
    }
#endif

    gettimeofday(&pre_time, NULL);

    while(mbRun){

        AMLOG_STATE("CVideoMemEncoder, state = %d.\n", msState);
        switch (msState){
            case STATE_IDLE:

            if(mFrames!=0 && mFrames%30==0){
                gettimeofday(&curr_time, NULL);
                if(pre_time.tv_usec != curr_time.tv_usec){
                    AMLOG_PERFORMANCE("Transcoding FrameRate = %ld.\n",
                        30000000/((curr_time.tv_sec - pre_time.tv_sec) * 1000000 + curr_time.tv_usec - pre_time.tv_usec));
                    pre_time.tv_sec = curr_time.tv_sec;
                    pre_time.tv_usec = curr_time.tv_usec;
                }
            }

                if(mpWorkQ->PeekCmd(cmd)){
                    ProcessCmd(cmd);
                }else{
                    if (mpInputPin->PeekBuffer(mpBuffer)){
                        AM_ASSERT(mpBuffer);
                        AM_ASSERT(mpBuffer->GetType() == CBuffer::EOS);
                        if (mpBuffer->GetType() == CBuffer::EOS) {
                            mbWaitEOS = true;
                            //msState = STATE_IDLE;
                            AMLOG_INFO("CVideoMemEncoder get eos from filter.\n");
                            //get eos
                            //todo
                        }else{//should not be here
                            AM_ASSERT(0);
                            //msState = STATE_PENDING;
                        }
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        //break;
                    }

                    err = ReadInputData();
                    if(err == ME_OK){
                        msState = STATE_WAIT_OUTPUT;
                    }else if(err == ME_CLOSED){
                        if(mbEOS){
                            AMLOG_INFO("CVideoMemEncoder Get EOS from encoder.\n");
                            ProcessEOS();
                            msState = STATE_PENDING;
                            break;
                        }else{
                            AM_ASSERT(0);
                        }
                    }else{
                        AMLOG_INFO("ReadInputData fail.\n");
                        usleep(20000);
                        break;
                    }
                }
                break;

            case STATE_WAIT_OUTPUT:
                while(mpOutputPin->mpBSB->GetFreeBufferCnt()<1){
                    AMLOG_DEBUG("CVideoMemEncoder: no enough buffer, wait for output buffer~~\n");
                    usleep(20000);
                }
                msState = STATE_READY;
                break;

            case STATE_READY:
                if(ProcessBuffer() == ME_OK){
#ifdef AM_DEBUG
                if (mLogOutput & LogDumpTotalBinary) {
                    if(out_fd>0) write(out_fd, mFrame.usr_start_addr, mFrame.pic_size);
                }
#endif
                    msState = STATE_IDLE;
                }else{
                    msState = STATE_ERROR;
                }
                break;

            case STATE_PENDING:
                if(mpWorkQ->PeekCmd(cmd)){
                    ProcessCmd(cmd);
                }else{
                    //?
                }
                break;

            case STATE_ERROR:
                //discard all data, wait eos
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                if (cmd.code == CMD_STOP) {
                    ProcessEOS();
                    mbRun = false;
                    CmdAck(ME_OK);
                } else {
                    //ignore other cmds
                    AMLOG_WARN("how to handle this type of cmd %d, in VideoMemEncoder error state.\n", cmd.code);
                }
                break;

            default:
                AM_ERROR("Not Support State Occur %d .\n",(AM_UINT)msState);
                break;
        }
    }
#ifdef AM_DEBUG
    if (mLogOutput & LogDumpTotalBinary) {
        if(out_fd>0) close(out_fd);
    }
#endif
    AMLOG_INFO("CVideoMemEncoder::OnRun end.\n");
}

AM_ERR CVideoMemEncoder::ReadInputData()
{
    AM_INT ret;
    memset(&mFrame, 0, sizeof(mFrame));

    ret = ::ioctl(mIavFd, IAV_IOC_GET_ENCODED_FRAME2, &mFrame);
    if (ret != 0)
    {
        AMLOG_DEBUG("IAV_IOC_GET_ENCODED_FRAME2 Return %d!\n", ret);
        return ME_BUSY;
    }
    if(mFrame.is_eos){
        AMLOG_DEBUG("CVideoMemEncoder::ReadInputData Get EOS!\n");
        if(!mbWaitEOS){
            AM_ASSERT(0);
            AMLOG_INFO("CVideoMemEncoder::ReadInputData: @@but encoder doesn't send EOS!\n");
        }
        mbEOS = true;
        return ME_CLOSED;
    }

    if (mpClockManager) {
        mLastFrameExpiredTime = mpClockManager->GetCurrentTime() + mFrameLifeTime;
    }

    mFrames++;
    AMLOG_PTS("CVideoMemEncoder get [FrmNo %u]: total %u frames.\n", mFrame.frame_num+1, mFrames);
    return ME_OK;
}

AM_ERR CVideoMemEncoder::ProcessBuffer()
{
    CBuffer* pBuffer;
    AM_ASSERT(mpOutputPin->mpBSB->GetFreeBufferCnt() > 0);
    if (!mpOutputPin->AllocBuffer(pBuffer)) {
        AM_ASSERT(0);
        return ME_ERROR;
    }
    pBuffer->mReserved = mFrame.fd_id; //release
    pBuffer->SetType(CBuffer::DATA);
    pBuffer->mFlags |= IAV_ENC_BUFFER ;
    pBuffer->mPTS = mFrame.pts_64;
    pBuffer->mExpireTime = mLastFrameExpiredTime;
    pBuffer->mBlockSize = pBuffer->mDataSize = mFrame.pic_size;
    pBuffer->mpData = (AM_U8*)mFrame.usr_start_addr;
    pBuffer->mSeqNum = mFrame.frame_num;
    pBuffer->mFrameType = mFrame.pic_type;

    mpOutputPin->SendBuffer(pBuffer);

    return ME_OK;
}

AM_ERR CVideoMemEncoder::ProcessEOS()
{
    AMLOG_INFO("CVideoMemEncoder ProcessEOS()\n");
//    PostEngineMsg(IEngine::MSG_EOS);//send MSG_EOS to engine in the last filter
//    return ME_OK;
    CBuffer *pBuffer;

    if (!mpOutputPin->AllocBuffer(pBuffer))
        return ME_ERROR;
    pBuffer->SetType(CBuffer::EOS);
    mpOutputPin->SendBuffer(pBuffer);
    return ME_OK;
}

IPin* CVideoMemEncoder::GetInputPin(AM_UINT index)
{
    if (index == 0)
        return mpInputPin;
    return NULL;
}

void CVideoMemEncoder::PrintState()
{
    AMLOG_INFO("CVideoMemEncoder State: %d.\n", msState);
}
/*
AM_ERR CVideoMemEncoder::StopEncoding()
{
    if(!mbEncoderStarted){
        return ME_OK;
    }
    AM_INFO("CVideoMemEncoder StopEncoding:\n");
    if (::ioctl(mIavFd, IAV_IOC_STOP_ENCODE2, IAV_MAIN_STREAM) < 0) {
        AM_PERROR("IAV_IOC_STOP_ENCODE2");
        //return ME_BAD_STATE;
    }
    mbEncoderStarted = false;

    return ME_OK;
}
*/
/*
AM_ERR CVideoMemEncoder::SendOutInfo()
{
    CBuffer *pBuffer;

    if (!mpOutputPin->AllocBuffer(pBuffer))
        return ME_ERROR;
    pBuffer->SetType(CBuffer::INFO);
    pBuffer->SetDataPtr((AM_U8*)&mH264Config);
    pBuffer->SetDataSize(sizeof(mH264Config));
    mpOutputPin->SendBuffer(pBuffer);

    return ME_OK;
}

void CVideoMemEncoder::PrintH264Config(iav_h264_config_t *config)
{
    if (config==NULL) return;
    AM_INFO("\t     profile = %s\n", (config->entropy_codec == IAV_ENTROPY_CABAC)? "main":"baseline");
    AM_INFO("\t           M = %d\n", config->M);
    AM_INFO("\t           N = %d\n", config->N);
    AM_INFO("\tidr interval = %d\n", config->idr_interval);
    AM_INFO("\t   gop model = %s\n", (config->gop_model == 0)? "simple":"advanced");
    AM_INFO("\t     bitrate = %d bps\n", config->average_bitrate);
    AM_INFO("\tbitrate ctrl = %s\n", (config->bitrate_control == IAV_BRC_CBR)? "cbr":"vbr");
    if (config->bitrate_control == IAV_BRC_VBR) {
        AM_INFO("\tmin_vbr_rate = %d\n", config->min_vbr_rate_factor);
        AM_INFO("\tmax_vbr_rate = %d\n", config->max_vbr_rate_factor);
    }
    AM_INFO("\t    de-intlc = %s\n", (config->deintlc_for_intlc_vin==0)? "off":"on");
    AM_INFO("\t        ar_x = %d\n", config->pic_info.ar_x);
    AM_INFO("\t        ar_y = %d\n", config->pic_info.ar_y);
    AM_INFO("\t  frame mode = %d\n", config->pic_info.frame_mode);
    AM_INFO("\t        rate = %d\n", config->pic_info.rate);
    AM_INFO("\t       scale = %d\n", config->pic_info.scale);
}
*/
//-----------------------------------------------------------------------
//
// CVideoMemEncoderInput
//
//-----------------------------------------------------------------------
CVideoMemEncoderInput* CVideoMemEncoderInput::Create(CFilter *pFilter)
{
    CVideoMemEncoderInput *result = new CVideoMemEncoderInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CVideoMemEncoderInput::Construct()
{
    AM_ERR err = inherited::Construct(((CVideoMemEncoder*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

AM_ERR CVideoMemEncoderInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    return ((CVideoMemEncoder*)mpFilter)->SetInputFormat(pFormat);
}
//-----------------------------------------------------------------------
//
// CVideoMemEncoderOutput
//
//-----------------------------------------------------------------------
CVideoMemEncoderOutput* CVideoMemEncoderOutput::Create(CFilter *pFilter)
{
    CVideoMemEncoderOutput *result = new CVideoMemEncoderOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CVideoMemEncoderOutput::Construct()
{
    return ME_OK;
}

AM_ERR CVideoMemEncoderOutput::SetOutputFormat()
{
    mpBSB->setIavFd(((CVideoMemEncoder*)mpFilter)->mIavFd);
    return ME_OK;
}

