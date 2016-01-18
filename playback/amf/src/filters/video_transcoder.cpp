
/*
 * video_transcoder.cpp
 *
 * History:
 *    2011/9/20 - [GangLiu] create file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "video_transcoder"
//#define AMDROID_DEBUG

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
#include "ambas_common.h"
#include "iav_transcode_drv.h"
#include "video_transcoder.h"
#include <math.h>


IFilter* CreateVideoTranscoderFilter(IEngine *pEngine)
{
	return CVideoTranscoder::Create(pEngine);
}

//===============================================
//
//===============================================

IFilter* CVideoTranscoder::Create(IEngine *pEngine)
{
    CVideoTranscoder *result = new CVideoTranscoder(pEngine);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CVideoTranscoder::Construct()
{
//    AM_INT i = 0;

    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    if ((mpMutex = CMutex::Create(true)) == NULL)
        return ME_NO_MEMORY;

    //Output
    mpOutputPin = CVideoTranscoderOutput::Create((CFilter *)this);
    if(mpOutputPin == NULL)
    {
        AM_ERROR("CVideoTranscoder::Create mpOutputPin Failed!\n");
    }

    if ((mpBufferPool = CSimpleBufferPool::Create("amba Transcode frame buffer", 32)) == NULL)
        return ME_ERROR;
    mpOutputPin->SetBufferPool(mpBufferPool);

    DSetModuleLogConfig(LogModuleVideoTranscoder);

    return ME_OK;
}

CVideoTranscoder::~CVideoTranscoder()
{
    if (ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0) != 0) {
        AMLOG_ERROR("DSP enter IDLE mode fail!.\n");
    }
    AM_DELETE(mpInputPin[0]);
    AM_DELETE(mpInputPin[1]);
    AM_DELETE(mpOutputPin);
    mpOutputPin = NULL;
}

AM_ERR CVideoTranscoder::Run()
{
	mpClockManager = (IClockManager *)mpEngine->QueryEngineInterface(IID_IClockManager);
	if(!mpClockManager) {
		AM_ERROR("CVideoTranscoder::Run without mpClockManager?\n");
		return ME_ERROR;
	}

	mpWorkQ->Run();
	return ME_OK;
}

AM_ERR CVideoTranscoder::Start()
{
	mpWorkQ->Start();
	return ME_OK;
}

void CVideoTranscoder::GetInfo(INFO& info)
{
    info.nInput = mTotalInputPinNumber;
    info.nOutput = 1;
    info.pName = "AmbaTransc";
}

IPin* CVideoTranscoder::GetOutputPin(AM_UINT index)
{
    return mpOutputPin;
}

AM_ERR CVideoTranscoder::AddInputPin(AM_UINT& index, AM_UINT type)
{
    if ((mTotalInputPinNumber+1) > 2) {
        AMLOG_ERROR("Max stream number reached, now %u, Max 2\n", mTotalInputPinNumber);
        return ME_BAD_PARAM;
    }

    index = mTotalInputPinNumber;
    AM_ASSERT(mpInputPin[index] == NULL);
    if (mpInputPin[index]) {
        AMLOG_ERROR("InputPin[%d] have context, must have error, delete and create new one.\n", index);
        delete mpInputPin[index];
        mpInputPin[index] = NULL;
    }

    mpInputPin[index] = CVideoTranscoderInput::Create((CFilter *)this);
    if (mpInputPin[index] == NULL) {
        AMLOG_ERROR("mpInputPin[%d] create fail.\n", index);
        return ME_ERROR;
    }
    mpInputPin[index]->mIndex = index;
    mbInputPin[index] = true;
    mTotalInputPinNumber++;
    AMLOG_INFO("CVideoTranscoder::AddInputPin %d done, total %d. \n", index, mTotalInputPinNumber);

    return ME_OK;
}

AM_ERR CVideoTranscoder::InitEncoder()
{
    AMLOG_INFO("CVideoTranscoder get  mFpsBase=%d.\n", mFpsBase);
    switch(mFpsBase){
        case 3754:
        case 3753:{
            mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_23_976;
            break;
        }
        case 3750:{
            mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_24;
            break;
        }
        case 3600:{
            mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_25;
            break;
        }
        case 3003:{
            mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_29_97;
            break;
        }
        case 3000:{
            mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_30;
            break;
        }
        case 1800:{
            mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_50;
            break;
        }
        case 1502:
        case 1501:{
            mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_59_94;
            break;
        }
        case 1500:{
            mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_60;
            break;
        }
        default:{
            AM_ERROR("CVideoTranscoder: Unknow fps: 90kHz/%u. Set to 29.97.\n", mFpsBase);
            mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_29_97;
        }
        break;
    }
    if(mbUseDefaultEncInfo){
        mEncInfo.input_format = IAV_INPUT_DRAM_422_PROG_IDSP;
        //mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_29_97;
        mEncInfo.input_img.width = 1280;
        mEncInfo.input_img.height = 720;
        mEncInfo.encode_img.width = 720;
        mEncInfo.encode_img.height = 480;
        mEncInfo.preview_enable = mbEnablePreview;
    }else{
        //get encinfo from app
    }
    if (::ioctl(mIavFd, IAV_IOC_INIT_ENCODE2, &mEncInfo) < 0) {
        AM_ERROR("IAV_IOC_INIT_ENCODE2 error, mIavFd: %d.\n", mIavFd);
        AM_PERROR("IAV_IOC_INIT_ENCODE2\n");
        return ME_ERROR;
    }
    AMLOG_INFO("InitEncoder: format: %d, preview: %u,\n\tinput: %u*%u,\n\tencode: %u*%u.\n",
        mEncInfo.input_format, mEncInfo.preview_enable,
        mEncInfo.input_img.width, mEncInfo.input_img.height,
        mEncInfo.encode_img.width, mEncInfo.encode_img.height);
    return ME_OK;
}

AM_ERR CVideoTranscoder::SetupEncoder()
{
    if(mbUseDefaultEncConfig){
        mEncConfig.enc_id = IAV_ENC_ID_MAIN;
        mEncConfig.encode_type = IAV_ENCODE_H264;
        mEncConfig.u.h264.gop_model = IAV_GOP_SIMPLE;
        mEncConfig.u.h264.bitrate_control = IAV_BRC_CBR;
        mEncConfig.u.h264.calibration = 100;
        mEncConfig.u.h264.vbr_ness = 50;
        mEncConfig.u.h264.min_vbr_rate_factor = 50,
        mEncConfig.u.h264.max_vbr_rate_factor = 300;
        mEncConfig.u.h264.entropy_codec = 0;

        mEncConfig.u.h264.M = 1;
        mEncConfig.u.h264.N = 15;
        mEncConfig.u.h264.idr_interval = 2;
        mEncConfig.u.h264.average_bitrate = 2000000;
    }else{
        //get encconfig from app
    }
    if (::ioctl(mIavFd, IAV_IOC_ENCODE_SETUP2, &mEncConfig) < 0) {
        AM_ERROR("IAV_IOC_ENCODE_SETUP2 error, mIavFd: %d.\n", mIavFd);
        AM_PERROR("IAV_IOC_ENCODE_SETUP2\n");
        return ME_ERROR;
    }
    AMLOG_INFO("SetupEncoder: M: %u, N: %u, idr_interval: %u, average_bitrate: %u.\n",
        mEncConfig.u.h264.M, mEncConfig.u.h264.N,
        mEncConfig.u.h264.idr_interval,
        mEncConfig.u.h264.average_bitrate);

    return ME_OK;
}

AM_ERR CVideoTranscoder::SetConfig()
{
/*    iav_mmap_info_t mmap;
    memset(&mmap,0, sizeof(iav_mmap_info_t));
    if (::ioctl(mIavFd, IAV_IOC_MAP_BSB2, &mmap) < 0) {
        AM_PERROR("IAV_IOC_MAP_BSB2");
        return ME_BAD_STATE;
    }

    AMLOG_INFO("mem_base: 0x%p, size = 0x%x\n", mmap.addr, mmap.length);
    mbMemMapped = true;
*/
    mFrmPoolInfo.frm_buf_pool_type = 0;// decpp frm buf pool
    if (::ioctl(mIavFd, IAV_IOC_GET_FRM_BUF_POOL_INFO, &mFrmPoolInfo) < 0) {
        AM_ERROR("IAV_IOC_GET_FRM_BUF_POOL_INFO error, mIavFd: %d.\n", mIavFd);
        AM_PERROR("IAV_IOC_GET_FRM_BUF_POOL_INFO\n");
        return ME_ERROR;
    }
    AMLOG_INFO("IAV_IOC_GET_FRM_BUF_POOL_INFO 0:\n\t num %u!\n", mFrmPoolInfo.num_frm_bufs);
    AMLOG_INFO("\t chroma_format %u!\n", mFrmPoolInfo.chroma_format);
    AMLOG_INFO("\t buffer_width %u!\n", mFrmPoolInfo.buffer_width);
    AMLOG_INFO("\t buffer_height %u!\n", mFrmPoolInfo.buffer_height);
    for(int i=0;i<mFrmPoolInfo.num_frm_bufs;i++){
        AMLOG_INFO("\t frm_buf_id[%d] %u;\n", i, mFrmPoolInfo.frm_buf_id[i]);
    }
    if(2==mTotalInputPinNumber){
        mFrmPoolInfo.frm_buf_pool_type = 1;// decpp frm buf pool
        if (::ioctl(mIavFd, IAV_IOC_GET_FRM_BUF_POOL_INFO, &mFrmPoolInfo) < 0) {
            AM_ERROR("IAV_IOC_GET_FRM_BUF_POOL_INFO error, mIavFd: %d.\n", mIavFd);
            AM_PERROR("IAV_IOC_GET_FRM_BUF_POOL_INFO\n");
            return ME_ERROR;
        }
        AMLOG_INFO("IAV_IOC_GET_FRM_BUF_POOL_INFO 1:\n\t num %u!\n", mFrmPoolInfo.num_frm_bufs);
        AMLOG_INFO("\t chroma_format %u!\n", mFrmPoolInfo.chroma_format);
        AMLOG_INFO("\t buffer_width %u!\n", mFrmPoolInfo.buffer_width);
        AMLOG_INFO("\t buffer_height %u!\n", mFrmPoolInfo.buffer_height);
        for(int i=0;i<mFrmPoolInfo.num_frm_bufs;i++){
            AMLOG_INFO("\t frm_buf_id[%d] %u;\n", i, mFrmPoolInfo.frm_buf_id[i]);
        }
    }
/*
    mFrmPoolInfo.frm_buf_pool_type = (1<<8);// enc input frm buf pool
    if (::ioctl(mIavFd, IAV_IOC_GET_FRM_BUF_POOL_INFO, &mFrmPoolInfo) < 0) {
        AM_ERROR("IAV_IOC_GET_FRM_BUF_POOL_INFO error, mIavFd: %d.\n", mIavFd);
        AM_PERROR("IAV_IOC_GET_FRM_BUF_POOL_INFO\n");
        return ME_ERROR;
    }*/
    AMLOG_INFO("IAV_IOC_GET_FRM_BUF_POOL_INFO done!\n");
    return ME_OK;
}

void CVideoTranscoder::DoStop()
{
    AMLOG_INFO("CVideoTranscoder DoStop()!\n");
    mbRun = false;
}

AM_ERR CVideoTranscoder::StopEncoding(AM_INT stopcode)
{
    //AUTO_LOCK(mpMutex);
    AMLOG_INFO("CVideoTranscoder StopEncoding (%d):\n", stopcode);
    if(mbStopped){
        AMLOG_INFO("CVideoTranscoder StopEncoding has been called, wait...\n");
        return ME_OK;
    }
    iav_transc_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    if(mbEOS || 2 == stopcode){
        if (ioctl(mIavFd, IAV_IOC_STOP_ENCODE2, IAV_ENC_ID_MAIN) < 0) {
            AM_PERROR("IAV_IOC_STOP_ENCODE2");
            return ME_ERROR;
        }
    }else{//stopcode == 1, not used now
        AM_ERR err = GetOutPic(frame, 0);
        if(ME_OK == err){//get 1 frame
            frame.flags = 0;
            frame.stop_cntl = 1;
            if (ioctl(mIavFd, IAV_IOC_SEND_INPUT_DATA, &frame) < 0) {
                AM_PERROR("IAV_IOC_SEND_INPUT_DATA");
                return ME_ERROR;
            }
            if (ioctl(mIavFd, IAV_IOC_RELEASE_FRAME2, &frame) < 0) {
                AM_PERROR("IAV_IOC_RELEASE_FRAME2");
                AM_ERROR("IAV_IOC_RELEASE_FRAME2 failed.\n");
                return ME_ERROR;
            }
        }else{
            if (ioctl(mIavFd, IAV_IOC_STOP_ENCODE2, IAV_ENC_ID_MAIN) < 0) {
                AM_PERROR("IAV_IOC_STOP_ENCODE2");
                return ME_ERROR;
            }
        }

    }
    ProcessEOS();
    mbStopped = true;

    AMLOG_INFO("CVideoTranscoder StopEncoding done.\n");
    return ME_OK;
}

void CVideoTranscoder::setTranscFrms(AM_INT frames)
{
    AMLOG_INFO("CVideoTranscoder setTranscFrms %d.\n", frames);
    mWantedFrms = frames;
}

AM_ERR CVideoTranscoder::SetEncParamaters(AM_UINT en_w, AM_UINT en_h, AM_UINT fpsBase, AM_UINT bitrate, bool hasBfrm)
{
    AMLOG_INFO("CVideoTranscoder SetEncParamaters enc: %u*%u, fpsBase: %u, bitrate: %u. Bfrm: %d.\n", en_w, en_h, fpsBase, bitrate, hasBfrm);
    mEncInfo.input_format = IAV_INPUT_DRAM_422_PROG_IDSP;
    //mEncInfo.input_frame_rate = AMBA_VIDEO_FPS_29_97;//get from demux or setting
    if(0 != bitrate){
        mFpsBase = fpsBase;
    }else{
        mFpsBase = mpSharedRes->mVideoTicks;
    }
    mEncInfo.input_img.width = mpSharedRes->sTranscConfig.dec_w[0];
    mEncInfo.input_img.height = mpSharedRes->sTranscConfig.dec_h[0];
    mEncInfo.encode_img.width = en_w;
    mEncInfo.encode_img.height = en_h;
    mEncInfo.preview_enable = mbEnablePreview;

    mEncConfig.enc_id = IAV_ENC_ID_MAIN;
    mEncConfig.encode_type = IAV_ENCODE_H264;
    mEncConfig.u.h264.gop_model = IAV_GOP_SIMPLE;
    mEncConfig.u.h264.bitrate_control = IAV_BRC_CBR;
    mEncConfig.u.h264.calibration = 100;
    mEncConfig.u.h264.vbr_ness = 50;
    mEncConfig.u.h264.min_vbr_rate_factor = 50,
    mEncConfig.u.h264.max_vbr_rate_factor = 300;
    mEncConfig.u.h264.entropy_codec = 0;
    mEncConfig.u.h264.M = hasBfrm?3:1;
    mEncConfig.u.h264.N = 15;
    mEncConfig.u.h264.idr_interval = 2;
    mEncConfig.u.h264.average_bitrate = (0 == bitrate)?2000000:bitrate;

    mbUseDefaultEncInfo = false;
    mbUseDefaultEncConfig = false;

    AMLOG_INFO("CVideoTranscoder SetEncParamaters done.\n");
    return ME_OK;

}

AM_ERR CVideoTranscoder::SetTranscSettings(bool RenderHDMI, bool RenderLCD, bool PreviewEnc, bool ctrlfps)
{
    AMLOG_INFO("CVideoTranscoder SetTranscSettings HDMI:%d LCD:%d Preview:%d ctrlFps:%d.\n", RenderHDMI, RenderLCD, PreviewEnc, ctrlfps);
    mbCtrlFPS = ctrlfps;
    mRenderMethord = 2;
    if(RenderHDMI) mRenderMethord |=0x01;
    if(RenderLCD) mRenderMethord |=0x04;
    mEncInfo.preview_enable = mbEnablePreview = PreviewEnc;
    AMLOG_INFO("CVideoTranscoder SetTranscSettings done.\n");
    return ME_OK;

}

bool CVideoTranscoder::ProcessCmd(CMD& cmd)
{
    //AM_ERR err = ME_OK;
    switch(cmd.code)
    {
    case CMD_STOP:
        DoStop();
        //mbRun = false;
        CmdAck(ME_OK);
        break;

    case CMD_TIMERNOTIFY:
        if ( msState == STATE_READY ) {
            RenderBuffer();
            msState = STATE_IDLE;
        }else{
            AM_ASSERT(0);
            msState = STATE_IDLE;
        }
    case CMD_OBUFNOTIFY:
        break;

    case CMD_AVSYNC:
        CmdAck(ME_OK);
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return 1;
}

void CVideoTranscoder::OnRun()
{
    CmdAck(ME_OK);
//    CQueue::QType type;
//    CQueue::WaitResult result;
    CMD cmd;
    AM_ERR err;

    mpBufferPool = mpOutputPin->mpBufferPool;
    mbRun = true;
    msState = STATE_IDLE;
    //SendOutInfo();

    err = setupVideoTranscoder();
    if (err == ME_OK) {
        AMLOG_INFO("CVideoTranscoder setupVideoTranscoder done!!\n");
    }else{
        AM_ASSERT(0);
        AM_ERROR("setupVideoTranscoder fail.\n");
        msState = STATE_ERROR;
    }

    while(mbRun) {

        AMLOG_STATE("CVideoTranscoder, state = %d.\n", msState);
        switch (msState) {

            case STATE_IDLE:
                if(mpWorkQ->PeekCmd(cmd)){
                    ProcessCmd(cmd);
                }else{
                    if(mbStopped){//get EOS, by calling StopEncoding()
                        AMLOG_INFO("CVideoTranscoder stopped from StopEncoding().\n");
                        //ProcessEOS();
                        msState = STATE_PENDING;
                        break;
                    }
                    if (ME_OK != WaitAllInputBuffer()) {
                        AM_ERROR("WaitInputBuffer failed\n");
                        msState = STATE_PENDING;
                        break;
                    }

                    for(AM_INT i=0; i<2; ++i){
                        if(mbInputPin[i]){//this pin is connected
                            if(1 == mHasInput[i]){//get 1 frame
                                if((true == mbEOS[i])){//this is the last frame
                                    if(false == mbWaitEOS[i]){//mbEOS but !mbWaitEOS, stop with mWantedFrms
                                        //in this case, no mater whether EOS is got from inputpin
                                    }else{//Only check EOS when get eos from udec
                                        if (mpInputPin[i]->PeekBuffer(mpBuffer[i])) {
                                            AM_ASSERT(mpBuffer[i]);
                                            AM_ASSERT(mpBuffer[i]->GetType() == CBuffer::EOS);
                                            if (mpBuffer[i]->GetType() == CBuffer::EOS) {
                                                //get eos
                                                AMLOG_INFO("CVideoTranscoder get eos from decoder filter[%d] frms=%u.\n", i, mFrames[i]);
                                            }else{//should not be here
                                                AM_ASSERT(0);
                                                //msState = STATE_PENDING;
                                            }
                                            mpBuffer[i]->Release();
                                            mpBuffer[i] = NULL;
                                        }else{
                                            //If no EOS sent, this should be a seq-eos in the mid of stream, ignore it.
                                            mbWaitEOS[i] = false;
                                            mbEOS[i] = false;
                                            AMLOG_INFO("CVideoTranscoder get an EOS before EOF, ignore it.\n");
                                        }
                                    }
                                }
                                //get 1 outpic
                                msState = STATE_HAS_INPUTDATA;
                            }else if(-1 == mHasInput[i]){//eos
                                if((mbInputPin[0]?mbEOS[0]:true)&&(mbInputPin[1]?mbEOS[1]:true)){
                                    AMLOG_INFO("CVideoTranscoder All EOS come frms %u %u.\n", mFrames[0], mFrames[1]);
                                    StopEncoding(2);
                                    msState = STATE_PENDING;
                                }
                            }else{// 0 == mHasInput[i], EOS had been received.
                                //AM_ASSERT(0);
                            }
                        }
                    }
                }
                break;

            case STATE_PENDING:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_HAS_INPUTDATA:
                if(ME_OK != ProcessBuffer()){
                    AM_ASSERT(0);
                    //msState = STATE_PENDING;
                }
                if((1 == (mRenderMethord & 0x01)) && mbCtrlFPS){
                    if(!mbSetClockMgr){
                        mpClockManager->SetClockMgr(0);
                        mbSetClockMgr = true;
                        RenderBuffer();//the 1st frame, render directly
                        msState = STATE_IDLE;
                        break;
                    }else{
                        //Workaround here for CES demo, +50 to deal with the out-sync between iOne and ipad
                        mpClockManager->SetTimer(this, mFrames[0]*(mFpsBase+50));
                    }
                    msState = STATE_READY;
                }else{
                    RenderBuffer();//the 1st frame, render directly
                    msState = STATE_IDLE;
                }
                break;

            case STATE_READY:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            default:
                AM_ERROR(" %d",(AM_UINT)msState);
                break;
        }
   }

}

AM_ERR CVideoTranscoder::SetInputFormat(CMediaFormat *pFormat)
{
    mIavFd = pFormat->format;
    mDspIndex = pFormat->mDspIndex;

    mpOutputPin->SetOutputFormat();
    return ME_OK;
}

AM_ERR CVideoTranscoder::ProcessBuffer()
{
    static AM_INT i=0;
    static AM_INT w,h,dir=2;
    static AM_INT delta=4;
    static AM_INT Radius,Rx,Ry,CenterX, CenterY;
    //iav_transc_frame_t *pframe = &mFrame[0];

    if(2==mTotalInputPinNumber && (1==mHasInput[0])&&(1==mHasInput[1])){//both 2 decoders have pics
        if(i==0){
            AMLOG_INFO("mFrame[0].buffer_pitch %d. mFrame[0].buffer_height %d.\n",mFrame[0].buffer_pitch,mFrame[0].buffer_height);
            AMLOG_INFO("mFrame[1].buffer_pitch %d. mFrame[1].buffer_height %d.\n",mFrame[1].buffer_pitch,mFrame[1].buffer_height);
            w=mFrame[1].buffer_pitch;
            h=mFrame[1].buffer_height;
            CenterX= mFrame[1].buffer_width/2;
            CenterY=mFrame[1].buffer_height/2;
            Radius = 0;
            i++;
        }

#if 0
        {//up-down
            if(i<delta){dir = delta;}
            if(i>= (h/2-delta-1)){dir = -delta;}
            i+=dir;
            //combine the 2 buffer
            memcpy((void*)((u32)mFrame[0].lu_buf_addr+w*2*(i%(h/2))),
                          (void*)((u32)mFrame[1].lu_buf_addr+w*2*(i%(h/2))),
                                              w*(h-2*(i%(h/2))));
            //set chroma to 0x7f
            memcpy((void*)((u32)mFrame[0].ch_buf_addr+w*2*(i%(h/2))),
                          (void*)((u32)mFrame[1].ch_buf_addr+w*2*(i%(h/2))),
                                              w*(h-2*(i%(h/2))));
        }
#else
        {//circle
            for(AM_INT lines=0;lines<h;++lines){
                Ry=(lines>=CenterY)?(CenterY-lines):(lines-CenterY);
                if(Ry<Radius){
                    Rx=sqrt(Radius*Radius - Ry*Ry);
                    Rx= (Rx>CenterX)?CenterX:Rx;
                    memcpy((void*)((u32)mFrame[0].lu_buf_addr+w*lines+CenterX-Rx),
                                  (void*)((u32)mFrame[1].lu_buf_addr+w*lines+CenterX-Rx),
                                  2*Rx);
                    memcpy((void*)((u32)mFrame[0].ch_buf_addr+w*lines+CenterX-Rx),
                                  (void*)((u32)mFrame[1].ch_buf_addr+w*lines+CenterX-Rx),
                                  2*Rx);
                }else{//no need to copy
                }
            }
            if(Radius<30){dir = delta;}
            if(Radius>= (CenterX/2/*+CenterY/2*/)){dir = -delta;}
            Radius+=dir;
        }
#endif

        AMLOG_PTS("CVideoTranscoder::ProcessBuffer[%u] %u %u.\n",mFrames[0], mFrame[0].real_fb_id,mFrame[1].real_fb_id);
        mFrame[1].flags = 0;
        if (ioctl(mIavFd, IAV_IOC_RELEASE_FRAME2, &mFrame[1]) < 0) {
            AM_PERROR("IAV_IOC_RELEASE_FRAME2");
            AM_ERROR("IAV_IOC_RELEASE_FRAME2 failed.\n");
            return ME_ERROR;
        }
    }else{//only 1 pin has pic
        if(1==mHasInput[0]){
            pFrame2Render = &mFrame[0];
        }else if(1==mHasInput[1]){
            pFrame2Render = &mFrame[1];
        }else{
            AM_ASSERT(0);
        }
        AMLOG_PTS("CVideoTranscoder::ProcessBuffer[%u] RenderMethord %d, fid %u.\n",(1==mHasInput[0])?mFrames[0]:mFrames[1], mRenderMethord, pFrame2Render->real_fb_id);
    }

    //move here from RenderBuffer(), to ensure that buffer is not overwrited when rendering to vout
    if(1 == (mRenderMethord & 0x01)){
        pFrame2Render->flags = IAV_FRAME2_NO_RELEASE;
        if (ioctl(mIavFd, IAV_IOC_RENDER_FRAME2, pFrame2Render) < 0) {
            AM_PERROR("IAV_IOC_RENDER_FRAME2");
            AM_ERROR("IAV_IOC_RENDER_FRAME2 failed.\n");
            return ME_ERROR;
        }
    }

    return ME_OK;
}

AM_ERR CVideoTranscoder::RenderBuffer()
{
    AM_ASSERT((pFrame2Render == &mFrame[0]) || (pFrame2Render == &mFrame[1]));

    pFrame2Render->flags = IAV_FRAME2_NO_RELEASE;
    if (ioctl(mIavFd, IAV_IOC_SEND_INPUT_DATA, pFrame2Render) < 0) {
        AM_PERROR("IAV_IOC_SEND_INPUT_DATA");
        AM_ERROR("IAV_IOC_SEND_INPUT_DATA failed.\n");
        return ME_ERROR;
    }
    if(1 == (mRenderMethord & 0x01)){
/*      if (ioctl(mIavFd, IAV_IOC_RENDER_FRAME2, pFrame2Render) < 0) {
            AM_PERROR("IAV_IOC_RENDER_FRAME2");
            AM_ERROR("IAV_IOC_RENDER_FRAME2 failed.\n");
            return ME_ERROR;
        }
*/
    }else{
        if (ioctl(mIavFd, IAV_IOC_RELEASE_FRAME2, pFrame2Render) < 0) {
            AM_PERROR("IAV_IOC_RELEASE_FRAME2");
            AM_ERROR("IAV_IOC_RELEASE_FRAME2 failed.\n");
            return ME_ERROR;
        }
    }
    return ME_OK;
}

AM_ERR CVideoTranscoder::ProcessEOS()
{
    AMLOG_INFO("CVideoTranscoder ProcessEOS()\n");

    CBuffer *pBuffer;
    if (!mpOutputPin->AllocBuffer(pBuffer))
        return ME_ERROR;
    pBuffer->SetType(CBuffer::EOS);
    //AM_INFO("CVideoMemEncoder send EOS\n");
    mpOutputPin->SendBuffer(pBuffer);

    return ME_OK;
}

AM_ERR CVideoTranscoder::setupVideoTranscoder()
{
    if(InitEncoder() != ME_OK)
    {
        AM_PERROR("InitEncoder Failed!\n");
        return ME_ERROR;
    }
    if(SetupEncoder() != ME_OK)
    {
        AM_PERROR("SetupEncoder Failed!\n");
        return ME_ERROR;
    }

    // start encoding
    AMLOG_INFO("CVideoTranscoder IAV_IOC_START_ENCODE2 stream[%d]:\n", IAV_ENC_ID_MAIN);
    if (::ioctl(mIavFd, IAV_IOC_START_ENCODE2, IAV_ENC_ID_MAIN) < 0) {
        AM_ERROR("IAV_IOC_START_ENCODE2 error, mIavFd: %d.\n", mIavFd);
        AM_PERROR("IAV_IOC_START_ENCODE2\n");
        return ME_ERROR;
    }

    if(SetConfig() != ME_OK)
    {
        AM_PERROR("SetConfig Failed!\n");
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CVideoTranscoder::GetOutPic(iav_transc_frame_t& frame, AM_INT Index)
{
    AUTO_LOCK(mpMutex);
    AM_INT err;
    memset(&frame, 0, sizeof(frame));
    frame.decoder_id = Index;

    AMLOG_DEBUG("CVideoTranscoder::GetOutPic:\n");
    err = ioctl(mIavFd, IAV_IOC_GET_DECODED_FRAME2, &frame) ;
    if (err == -EAGAIN){
        AMLOG_DEBUG("CVideoTranscoder::GetOutPic return EAGAIN.\n");
        return ME_BUSY;
    }else if(err < 0){
        AM_PERROR("IAV_IOC_GET_DECODED_FRAME2");
        AM_ERROR("IAV_IOC_GET_DECODED_FRAME2 failed, index %d.\n",Index);
        return ME_ERROR;
    }

    if (frame.fb_id == IAV_INVALID_FB_ID) {
        if (frame.eos_flag) {
            mEOSPTS[Index].pts_low = frame.pts;
            mEOSPTS[Index].pts_high= frame.pts_high;
            AMLOG_INFO("CVideoTranscoder get EOS from udec[%d], last PTS: h %u, l %u.\n", Index, frame.pts_high, frame.pts);
            if((mEOSPTS[Index].pts_low == mLastPTS[Index].pts_low)&&(mEOSPTS[Index].pts_high== mLastPTS[Index].pts_high)){
                AMLOG_INFO("CVideoTranscoder: index[%d] Last frame has already arrived.\n", Index);
                return ME_NOT_EXIST;
            }
            return ME_BUSY;
        }
    }

    if(true == mbWaitEOS[Index]){
        if((frame.pts_high == mEOSPTS[Index].pts_high)&&(frame.pts == mEOSPTS[Index].pts_low)){//get last pts
            mFrames[Index]++;
            AMLOG_INFO("CVideoTranscoder get last frame! FID %u, total %d frms.\n",frame.real_fb_id, mFrames[Index]);
            return ME_CLOSED;
        }else{
            //get a valid pic, not last frame.
            AMLOG_PTS("CVideoTranscoder wait last frame: h %u, l %u..\n", frame.pts_high, frame.pts);
        }
    }else{
        AMLOG_PTS("CVideoTranscoder get pts: h %u, l %u.\n", frame.pts_high, frame.pts);
    }

    mLastPTS[Index].pts_low = frame.pts;
    mLastPTS[Index].pts_high= frame.pts_high;
    mFrames[Index]++;
    if((0 != mWantedFrms) && (mWantedFrms == mFrames[Index])){
        AMLOG_INFO("CVideoTranscoder::GetOutPic get frms == mWantedFrms(%d).\n",mFrames[Index]);
        return ME_CLOSED;
    }
    AMLOG_DEBUG("CVideoTranscoder::GetOutPic %u, total %d frms.\n",frame.real_fb_id, mFrames[Index]);

    return ME_OK;
}

AM_ERR CVideoTranscoder::WaitAllInputBuffer()
{
    /*CQueueInputPin* pPin;
    CBuffer *pBuffer;

    CQueue::WaitResult result;*/
    AO::CMD cmd;

//    AM_BOOL AllRecieved = false;
    AM_ERR err;

    AM_INT i=0;
    while(i<2){
        if(mbInputPin[i] && !mbEOS[i]){
            err = GetOutPic(mFrame[i], i);
            if(ME_OK == err){//get 1 frame
                mHasInput[i]=1;
            }else if(ME_BUSY == err){//retry
                if(mFrame[i].eos_flag){
                    mbWaitEOS[i] = true;
                    AMLOG_INFO("get EOS from udec[%d].\n", i);
                }else{
                    AMLOG_DEBUG("ReadInputData fail.\n");
                    sleep(0.1);
                }
                continue;
            }else if(ME_CLOSED == err){//get last frame from decpp
                //AMLOG_INFO("CVideoTranscoder Get last frame from decoder.\n");
                mbEOS[i] = true;
                mHasInput[i]=1;
            }else if(ME_NOT_EXIST == err){//alrdy arrived
                mbEOS[i] = true;
                mHasInput[i]=-1;
            }else{//error
                //AMLOG_INFO("GetOutPic failed, DSP may be flushed or stoped, go to pending!\n");
                return ME_ERROR;
            }
        }else{//this pin has no data
            if(1 == mHasInput[i]){
                mHasInput[i]=-1;
            }else if(-1 == mHasInput[i]){
                mHasInput[i]=0;
            }
        }
        ++i;
    }

    AMLOG_DEBUG("get frames: %d, %d!\n", mHasInput[0], mHasInput[1]);
    return ME_OK;
}

IPin* CVideoTranscoder::GetInputPin(AM_UINT index)
{
    if (index == 0)
        return mpInputPin[0];
    if (index == 1)
        return mpInputPin[1];
    return NULL;
}

void CVideoTranscoder::PrintState()
{
    AMLOG_INFO("CVideoTranscoder State: %d.\n", msState);
}
//-----------------------------------------------------------------------
//
// CVideoTranscoderInput
//
//-----------------------------------------------------------------------
CVideoTranscoderInput* CVideoTranscoderInput::Create(CFilter *pFilter)
{
    CVideoTranscoderInput *result = new CVideoTranscoderInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CVideoTranscoderInput::Construct()
{
    AM_ERR err = inherited::Construct(((CVideoTranscoder*)mpFilter)->MsgQ());
    if (err != ME_OK){
        AM_ERROR("CVideoTranscoderInput::inherited::Construct failed, err %d.\n",err);
        return err;
    }

    return ME_OK;
}

AM_ERR CVideoTranscoderInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    return ((CVideoTranscoder*)mpFilter)->SetInputFormat(pFormat);
}
//-----------------------------------------------------------------------
//
// CVideoTranscoderOutput
//
//-----------------------------------------------------------------------
CVideoTranscoderOutput* CVideoTranscoderOutput::Create(CFilter *pFilter)
{
    CVideoTranscoderOutput *result = new CVideoTranscoderOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CVideoTranscoderOutput::Construct()
{

    return ME_OK;
}

AM_ERR CVideoTranscoderOutput::SetOutputFormat()
{
	mMediaFormat.pMediaType = &GUID_Amba_Decoded_Video;
	mMediaFormat.pSubType = &GUID_Video_YUV420NV12;
	mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Media;
	mMediaFormat.format = ((CVideoTranscoder*)mpFilter)->mIavFd;
	mMediaFormat.mDspIndex = ((CVideoTranscoder*)mpFilter)->mDspIndex;
	return ME_OK;
}

