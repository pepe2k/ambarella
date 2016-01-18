
/*
 * video_effecter_preview.cpp
 *
 * History:
 *    2011/1/16 - [LiuGang] create file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "video_effecter_preview"
#define AMDROID_DEBUG

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "pbif.h"
#include "ve_if.h"
#include "am_ve_if.h"
#include "engine_guids.h"

#include <basetypes.h>
#include "iav_drv.h"
#include "filter_list.h"

#include "iav_transcode_drv.h"
#include "video_effecter.h"
#include "video_effecter_if.h"

int (*GPUCreateStreamTextureDevice)(bool [],int [], int [], int [], unsigned int [][5])=0;
int (*GPURenderTextureBuffer)(bool [],int [])=0;

void SetGPUFuncs(int (*RenderTextureBuffer)(bool [],int []),
        int (*CreateStreamTextureDevice)(bool [],int [], int [], int [], unsigned int [][5]))
{
    GPUCreateStreamTextureDevice = CreateStreamTextureDevice;
    GPURenderTextureBuffer = RenderTextureBuffer;
    return;
}
//#define debug_wait_2udec_render
//#define debug_wait_2udec_render_NO_together

filter_entry g_video_effecter = {
    "VideoEffecterPreview",
    CVideoEffecter::Create,
    NULL,
    CVideoEffecter::AcceptMedia,
};

IFilter* CreateVideoEffecterPreviewFilter(IEngine *pEngine)
{
	return CVideoEffecter::Create(pEngine);
}
//-----------------------------------------------------------------------
//
// CAmbaVideoEffecter
//
//-----------------------------------------------------------------------

IFilter* CVideoEffecter::Create(IEngine *pEngine)
{
    CVideoEffecter *result = new CVideoEffecter(pEngine);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_INT CVideoEffecter::AcceptMedia(CMediaFormat& format)
{
    if ((*format.pMediaType == GUID_Decoded_Video || *format.pMediaType == GUID_Amba_Decoded_Video) &&
		*format.pSubType == GUID_Video_YUV420NV12)
        return 1;
    return 0;
}

AM_ERR CVideoEffecter::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;

    DSetModuleLogConfig(LogModuleVideoEffecter);

/*
    if(InitStreamTexture()!=0)
        return ME_ERROR;
    AMLOG_DEBUG("VideoEffecter: InitStreamTexture OK.\n");
*/
    return ME_OK;
}

CVideoEffecter::~CVideoEffecter()
{
    AMLOG_DESTRUCTOR("~CVideoEffecterPreview.\n");
    for(int i=0;i<MaxVideoStreamNumber;++i)
        if(mpVideoEffecterInputPin[i])
            AM_DELETE(mpVideoEffecterInputPin[i]);

/*    if(mbStreamTextureDeviceCreated)
        DestroyStreamTextureDevice();
*/
}

bool CVideoEffecter::ProcessCmd(CMD& cmd)
{
    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            CmdAck(ME_OK);
            AMLOG_DEBUG("****CVideoEffecter::ProcessCmd, STOP cmd.\n");
            break;

        case CMD_OBUFNOTIFY:
            AMLOG_DEBUG("****CVideoEffecter::ProcessCmd, CMD_OBUFNOTIFY cmd.\n");
            break;

        case CMD_PAUSE:
            AMLOG_DEBUG("****CVideoEffecter::ProcessCmd, CMD_PAUSE cmd.\n");
            break;

        case CMD_RESUME:
            AMLOG_DEBUG("****CVideoEffecter::ProcessCmd, CMD_RESUME cmd.\n");
            break;

        case CMD_FLUSH:
            CmdAck(ME_OK);
            break;

        case CMD_AVSYNC:
            AMLOG_DEBUG("** recieve avsync cmd, state %d.\n", msState);
            CmdAck(ME_OK);
            break;

        case CMD_SOURCE_FILTER_BLOCKED:
            break;

        default:
            AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return false;
}

void CVideoEffecter::OnRun()
{
    CMD cmd;
//    CQueue::QType type;
//    CQueue::WaitResult result;
    AM_ERR err;

    CmdAck(ME_OK);

    err = setupVideoEffecter();
    if (err == ME_OK) {
        AMLOG_INFO("CVideoEffecter setupVideoEffecter done!!\n");
    }else{
        AM_ASSERT(0);
        AM_ERROR("setupVideoEffecter fail.\n");
        msState = STATE_ERROR;
    }

    mbRun = true;
    msState = STATE_IDLE;

    while(mbRun) {
        AMLOG_STATE("VideoEffecterPreview: start switch, msState=%d, [%d] [%d] [%d] input data.\n", msState,
            mbInputPin[0]?mpVideoEffecterInputPin[0]->GetDataCnt():0,
            mbInputPin[1]?mpVideoEffecterInputPin[1]->GetDataCnt():0,
            mbInputPin[2]?mpVideoEffecterInputPin[2]->GetDataCnt():0);
        AMLOG_STATE("\t\tgetpic: %d, %d, %d.\n",mHasInput[0], mHasInput[1], mHasInput[2]);
        switch (msState) {

            case STATE_IDLE:

                if (ME_OK != WaitAllInputBuffer()) {
                    AM_ERROR("WaitInputBuffer failed when get MSG\n");
                    break;
                }

                for(int i=0; i<MaxVideoStreamNumber; ++i){
                    if(mbInputPin[i]){
#ifdef debug_wait_2udec_render_NO_together
if(mHasInput[i]==0) continue;
#endif
                        if(mbEOS[i] == true){
                            if (mpVideoEffecterInputPin[i]->PeekBuffer(mpBuffer[i])) {
                                AM_ASSERT(mpBuffer[i]);
                                AM_ASSERT(mpBuffer[i]->GetType() == CBuffer::EOS);
                                if (mpBuffer[i]->GetType() == CBuffer::EOS) {
                                    //get eos
                                    //todo
                                }else{//should not be here
                                    AM_ASSERT(0);
                                    //msState = STATE_PENDING;
                                }
                                mpBuffer[i]->Release();
                                mpBuffer[i] = NULL;
                                break;
                            }else{//should mv to WaitAllInputBuffer()
                                //If no EOS sent, this should be a seq-eos in the mid of stream, ignore it.
                                mbEOS[i] = false;
                                mHasInput[i] = 0;
                                break;
                            }
                        }else{
                            //get outpic
                            msState = STATE_HAS_INPUTDATA;
                        }
                    }
                }

                break;

            case STATE_PENDING:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                break;

            case STATE_HAS_INPUTDATA:
                if(ME_OK == RenderBuffer()){
                    msState = STATE_IDLE;
                }else{
                    AM_ERROR("RenderBuffer Error!\n");
                    msState = STATE_PENDING;
                }
                break;

            case STATE_READY:
                break;

            default:
                AM_ERROR(" %d",(AM_UINT)msState);
                break;
        }
   }

}

AM_ERR CVideoEffecter::Run()
{
    mpWorkQ->Run();
    return ME_OK;
}

AM_ERR CVideoEffecter::Start()
{
 /*   //init GPU & OpenGL ES
    if(InitGLES()<0){
        return ME_ERROR;
    }
    AMLOG_DEBUG("VideoEffecter: InitGLES OK.\n");
    if(CreateStreamTextureDevice(1280, 720, 1280, (void*)NULL)<0){
        return ME_ERROR;
    }
    AMLOG_DEBUG("VideoEffecter: CreateStreamTextureDevice OK.\n");
*/
    mpWorkQ->Start();
    return ME_OK;
}

AM_ERR CVideoEffecter::AddInputPin(AM_UINT& index, AM_UINT type)
{
    if ((mTotalInputPinNumber+1) >= MaxVideoStreamNumber) {
        AMLOG_ERROR("Max stream number reached, now %d, Max %d", mTotalInputPinNumber, MaxVideoStreamNumber);
        return ME_BAD_PARAM;
    }

    index = mTotalInputPinNumber;
    AM_ASSERT(mpVideoEffecterInputPin[index] == NULL);
    if (mpVideoEffecterInputPin[index]) {
        AMLOG_ERROR("InputPin[%d] have context, must have error, delete and create new one.\n", index);
        delete mpVideoEffecterInputPin[index];
        mpVideoEffecterInputPin[index] = NULL;
    }

    mpVideoEffecterInputPin[index] = CVideoEffecterInput::Create((CFilter *)this);
    if (mpVideoEffecterInputPin[index] == NULL) {
        AMLOG_ERROR("mpVideoEffecterInputPin[%d] create fail.\n", index);
        return ME_ERROR;
    }
    mpVideoEffecterInputPin[index]->mIndex = index;
    mbInputPin[index] = true;
    mTotalInputPinNumber++;
    AMLOG_INFO("CVideoEffecter::AddInputPin %d done, total %d. \n", index, mTotalInputPinNumber);

    return ME_OK;
}

void CVideoEffecter::GetInfo(INFO& info)
{
    info.nInput = 3;//max num of input is 3
    info.nOutput = 0;

//    AMLOG_INFO("VideoEffecterPreview: nOutput = %d.\n", info.nOutput);
    info.mPriority = 4;
    info.mFlags = SYNC_FLAG;
    info.pName = "VideoEffecter";
}

IPin* CVideoEffecter::GetInputPin(AM_UINT index)
{
    if (index == 0)
        return mpVideoEffecterInputPin[0];
    if (index == 1)
        return mpVideoEffecterInputPin[1];
    if (index == 2)
        return mpVideoEffecterInputPin[2];
    return NULL;
}

AM_ERR CVideoEffecter::GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs)
{
    //
    absoluteTimeMs = 0;
    relativeTimeMs = 0;
    return ME_OK;
}

void* CVideoEffecter::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IClockObserver)
        return (IClockObserver*)NULL;
    else if (refiid == IID_IVideoOutput)
        return (IVideoOutput*)NULL;
    else if (refiid == IID_IRenderer)
        return (IRender*)this;
    else if (refiid == IID_IVideoEffecterControl)
        return (IVideoEffecterControl*)this;

    return inherited::GetInterface(refiid);
}

void CVideoEffecter::Delete()
{
	//AM_RELEASE(mpFrameBufferPool);
	//mpFrameBufferPool = NULL;
	AMLOG_DESTRUCTOR("CVideoEffecter::Delete().\n");
	AM_DELETE(mpVideoEffecterInputPin[0]);
	AM_DELETE(mpVideoEffecterInputPin[1]);
	AM_DELETE(mpVideoEffecterInputPin[2]);
	memset(mpVideoEffecterInputPin, 0, sizeof(mpVideoEffecterInputPin));

	inherited::Delete();
}

AM_ERR CVideoEffecter::SetInputFormat(CMediaFormat *pFormat)
{
    mIavFd = pFormat->format;
    return ME_OK;
}

AM_U16 CVideoEffecter::findFID(AM_U16 fid, AM_INT index)
{
    for(AM_U32 i=0; i<mDecppBufpool.buf_nums[index]; ++i){
        if(fid == mDecppBufpool.fid[index][i]){
            return i;
        }
    }
    AMLOG_WARN("CVideoEffecter::findFID %u error: FID[%d]: %u  %u  %u  %u.\n", fid, index, mDecppBufpool.fid[index][0],
        mDecppBufpool.fid[index][1], mDecppBufpool.fid[index][2], mDecppBufpool.fid[index][3]);
    return 0;
}

AM_ERR CVideoEffecter::setupVideoEffecter()
{
    iav_frm_buf_pool_info_t mFrmPoolInfo;
    AMLOG_INFO("GET_FRM_BUF_POOL_INFO:\n");
    mFrmPoolInfo.frm_buf_pool_type = 0;// decpp frm buf pool
    if (::ioctl(mIavFd, IAV_IOC_GET_FRM_BUF_POOL_INFO, &mFrmPoolInfo) < 0) {
        AM_ERROR("IAV_IOC_GET_FRM_BUF_POOL_INFO error, mIavFd: %d.\n", mIavFd);
        AM_PERROR("IAV_IOC_GET_FRM_BUF_POOL_INFO\n");
    }
    mDecppBufpool.buf_nums[0] = mFrmPoolInfo.num_frm_bufs;
    mDecppBufpool.buf_width[0] = mFrmPoolInfo.buffer_width;
    mDecppBufpool.buf_height[0] = mFrmPoolInfo.buffer_height;
    mDecppBufpool.buf_pitch[0] = mFrmPoolInfo.buffer_width;
    for(AM_INT i=0;i<mFrmPoolInfo.num_frm_bufs;i++){
        mDecppBufpool.fid[0][i] = mFrmPoolInfo.frm_buf_id[i];
        mDecppBufpool.buf_addr[0][i] = (AM_U32)mFrmPoolInfo.luma_img_base_addr[i];
        AMLOG_INFO("i=%d; fid: %u, buf_addr %x.\n", i, mDecppBufpool.fid[0][i], mDecppBufpool.buf_addr[0][i]);
    }
    AMLOG_INFO("GET_FRM_BUF_POOL_INFO udec 0, num %u.\n", mFrmPoolInfo.num_frm_bufs);


    if(mTotalInputPinNumber>=2){
        memset(&mFrmPoolInfo, 0, sizeof(iav_frm_buf_pool_info_t));
        mFrmPoolInfo.frm_buf_pool_type = 1;// decpp frm buf pool
        if (::ioctl(mIavFd, IAV_IOC_GET_FRM_BUF_POOL_INFO, &mFrmPoolInfo) < 0) {
            AM_ERROR("IAV_IOC_GET_FRM_BUF_POOL_INFO error, mIavFd: %d.\n", mIavFd);
            AM_PERROR("IAV_IOC_GET_FRM_BUF_POOL_INFO\n");
        }

        mDecppBufpool.buf_nums[1] = mFrmPoolInfo.num_frm_bufs;
        mDecppBufpool.buf_width[1] = mFrmPoolInfo.buffer_width;
        mDecppBufpool.buf_height[1] = mFrmPoolInfo.buffer_height;
        mDecppBufpool.buf_pitch[1] = mFrmPoolInfo.buffer_width;
        for(int i=0;i<mFrmPoolInfo.num_frm_bufs;i++){
            mDecppBufpool.fid[1][i] = mFrmPoolInfo.frm_buf_id[i];
            mDecppBufpool.buf_addr[1][i] = (AM_U32)mFrmPoolInfo.luma_img_base_addr[i];
            AMLOG_INFO("i=%d; fid: %u, buf_addr %x.\n", i, mDecppBufpool.fid[1][i], mDecppBufpool.buf_addr[1][i]);
        }
        AMLOG_INFO("GET_FRM_BUF_POOL_INFO udec 1, num %u.\n", mFrmPoolInfo.num_frm_bufs);
    }

    if((*GPUCreateStreamTextureDevice)(mbInputPin, mDecppBufpool.buf_width,
        mDecppBufpool.buf_height, mDecppBufpool.buf_pitch, mDecppBufpool.buf_addr)<0){
        AMLOG_ERROR("VideoEffecter: GPUCreateStreamTextureDevice failed!\n");
        return ME_ERROR;
    }
    AMLOG_INFO("VideoEffecter: GPUCreateStreamTextureDevice OK.\n");
    return  ME_OK;
}

AM_ERR CVideoEffecter::GetOutPic(iav_transc_frame_t& frame, AM_INT Index)
{
    AM_INT err;
    memset(&frame, 0, sizeof(frame));
    frame.decoder_id = Index;

    AMLOG_DEBUG("CVideoEffecter::GetOutPic:\n");
    err = ioctl(mIavFd, IAV_IOC_GET_DECODED_FRAME2, &frame) ;
    if (err == -EAGAIN){
        AMLOG_DEBUG("CVideoEffecter::GetOutPic return EAGAIN.\n");
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
            AMLOG_INFO("CVideoEffecter get EOS from udec[%d], last PTS: h %u, l %u.\n", Index, frame.pts_high, frame.pts);
            if((mEOSPTS[Index].pts_low == mLastPTS[Index].pts_low)&&(mEOSPTS[Index].pts_high== mLastPTS[Index].pts_high)){
                AMLOG_INFO("CVideoEffecter: index[%d] Last frame has already arrived.\n", Index);
                return ME_NOT_EXIST;
            }
            return ME_BUSY;
        }
    }

    if(true == mbWaitEOS[Index]){
        if((frame.pts_high == mEOSPTS[Index].pts_high)&&(frame.pts == mEOSPTS[Index].pts_low)){//get last pts
            mFrames[Index]++;
            AMLOG_INFO("CVideoEffecter get last frame! FID %u, total %d frms.\n",frame.real_fb_id, mFrames[Index]);
            return ME_CLOSED;
        }else{
            //get a valid pic, not last frame.
            AMLOG_PTS("CVideoEffecter wait last frame: h %u, l %u..\n", frame.pts_high, frame.pts);
        }
    }else{
        AMLOG_PTS("CVideoEffecter get pts: h %u, l %u.\n", frame.pts_high, frame.pts);
    }

    mLastPTS[Index].pts_low = frame.pts;
    mLastPTS[Index].pts_high= frame.pts_high;
    mFrames[Index]++;

    AMLOG_DEBUG("CVideoEffecter::GetOutPic %u, total %d frms.\n",frame.real_fb_id, mFrames[Index]);

    return ME_OK;
}

AM_ERR CVideoEffecter::WaitAllInputBuffer()
{
/*    CQueueInputPin* pPin;
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

AM_ERR CVideoEffecter::RenderOutBuffer(const char* from,char* to_y, char* to_uv)
{
    const char* rgb_frame_addr = from;
    char* y_addr=to_y;
    char* uv_addr=to_uv;

    rgba8888_2_yuv422(rgb_frame_addr, 1280,720,1280, y_addr, uv_addr, 1280,720,1280);
    //add other iav_decoded_frame_t info to output_buffer

    return ME_OK;
}

AM_ERR CVideoEffecter::RenderBuffer()
{
    static AM_UINT Frm_cnt = 0;
//    static int StreamTextureDevicecreated=0;

    AMLOG_DEBUG("RenderBuffer:\n");
/*
//for debug
{
#ifdef debug_wait_2udec_render
    //relase buffer, then decode will go on
    for(int i=0;i<MaxVideoStreamNumber;++i){
        if (mbInputPin[i]){
#ifdef debug_wait_2udec_render_NO_together
if(mHasInput[i]==0) continue;
#endif
Frm_cnt++;
            if (::ioctl(mIavFd, IAV_IOC_RENDER_FRAME, &mFrame[i]) < 0) {
                 AM_PERROR("IAV_IOC_RELEASE_FRAME.\n");
            }
    AMLOG_INFO("VideoEffecter: IAV_IOC_RENDER_FRAME OK, [%d] total %d.\n", i, Frm_cnt);
            //mFrame[i] = {NULL};
            mpBuffer[i]->Release();
            mpBuffer[i] = NULL;
            mHasInput[i]--;
            AM_ASSERT(mHasInput[i]>=0);
        }
    }
    return ME_OK;
#endif
}
*/
    for(int i=0; i<MaxVideoStreamNumber; ++i){
        if(mbInputPin[i]){
            AMLOG_DEBUG("VideoEffecter: Pin [%d], get buffer[%d] addr = %p %p,\tbuffer: %d*%d~%d\tpic: %d*%d.\n",
                i, mFrame[i].real_fb_id, mFrame[i].lu_buf_addr, mFrame[i].ch_buf_addr,
                mFrame[i].buffer_width, mFrame[i].buffer_height, mFrame[i].buffer_pitch,
                mFrame[i].pic_width, mFrame[i].pic_height);
        }
    }

    //add some effect in
    if(!GPURenderTextureBuffer){
        AMLOG_ERROR("VideoEffecter: GPURenderTextureBuffer is not initialized!\n");
        return ME_ERROR;
    }

    AMLOG_DEBUG("Ready to Render Buffer: real ID: %d  %d  %d.\n",
		mFrame[0].real_fb_id,mFrame[1].real_fb_id,mFrame[2].real_fb_id);
    AM_INT RenderBuffer[3]={
        mbInputPin[0]?findFID(mFrame[0].real_fb_id, 0):0,
        mbInputPin[1]?findFID(mFrame[1].real_fb_id, 1):0,
        mbInputPin[2]?findFID(mFrame[2].real_fb_id, 2):0};

    AM_ASSERT(RenderBuffer[0]>=0 && RenderBuffer[0]<5);
    AM_ASSERT(RenderBuffer[1]>=0 && RenderBuffer[1]<5);
    AM_ASSERT(RenderBuffer[2]>=0 && RenderBuffer[2]<5);

    if((*GPURenderTextureBuffer)(mbInputPin, RenderBuffer)<0){
    //if(GPURenderTextureBuffer()<0){
        AMLOG_ERROR("VideoEffecter: GPURenderTextureBuffer failed!\n");
        return ME_ERROR;
    }
//Frm_cnt++;
    AMLOG_DEBUG("VideoEffecter: GPURenderTextureBuffer OK, total %d.\n",Frm_cnt);

    //relase buffer, then decode will go on
    for(int i=0;i<MaxVideoStreamNumber;++i){
        if (mbInputPin[i] && mHasInput[i]){
            if (::ioctl(mIavFd, IAV_IOC_RELEASE_FRAME2, &mFrame[i]) < 0) {
                 AM_PERROR("IAV_IOC_RELEASE_FRAME2.\n");
            }
            //mFrame[i] = {NULL};
            mHasInput[i]=0;
            //AM_ASSERT(mHasInput[i]>=0);
        }
    }

    AMLOG_DEBUG("RenderBuffer done.\n");

    return ME_OK;
}

AM_ERR CVideoEffecter::rgba8888_2_yuv422(const char* from, AM_UINT width_rgb, AM_UINT height_rgb, AM_UINT stride_rgb,
		char* to_y, char* to_uv, AM_UINT width_yuv, AM_UINT height_yuv, AM_UINT stride_yuv)
{
//Y = 0.257R¡ä + 0.504G¡ä + 0.098B¡ä + 16
//Cb = -0.148R¡ä - 0.291G¡ä + 0.439B¡ä + 128
//Cr = 0.439R¡ä - 0.368G¡ä - 0.071B¡ä + 128

    char r,g,b,a;
    //assume sizes are same and width==stride
    for(AM_UINT lines=0;lines!=height_rgb;++lines){

        for(AM_UINT i=0;i!=width_rgb;++i){
            char* lines_y   = to_y  + lines*width_rgb;
            char* lines_uv = to_uv + height_rgb*width_rgb + lines*width_rgb;

            r = *(from+4*(lines*width_rgb+i));
            g = *(from+4*(lines*width_rgb+i)+1);
            b = *(from+4*(lines*width_rgb+i)+2);
            a = *(from+4*(lines*width_rgb+i)+3);
            AM_VERBOSE("r=%d, g=%d, b=%d, a=%d\n", r,g,b,a);

            //y
            *(lines_y+i) = ((66*r + 129*g + 25*b )>>8) + 16;
            //uv
            // lines%2 used for YUV420(NV12)
            if(/*lines%2==0 &&*/ i%2==0){
                *(lines_uv+2*i) = ((-38*r - 74*g + 112*b )>>8) + 128;
                *(lines_uv+2*i+1) = ((112*r - 94*g - 18 *b )>>8) + 128;
            }
        }
    }

    return ME_OK;
}
/*
AM_INT CVideoEffecter::InitStreamTexture()
{
    if(!GPUInitStreamTexture){
        AMLOG_ERROR("VideoEffecter: GPUInitStreamTexture is not initialized!\n");
        return -1;
    }
    if((*GPUInitStreamTexture)()<0){
        AMLOG_ERROR("VideoEffecter: InitStreamTexture Failed!\n");
        return -1;
    }
    return 0;
}

AM_INT CVideoEffecter::InitGLES()
{
    if(!GPUInitGLES){
        AMLOG_ERROR("VideoEffecter: GPUInitGLES is not initialized!\n");
        return -1;
    }
    if((*GPUInitGLES)()<0){
        AMLOG_ERROR("VideoEffecter: InitGLES Failed!\n");
        return -1;
    }
    return 0;
}
AM_INT CVideoEffecter::CreateStreamTextureDevice(int width, int height, int pitch, unsigned int mVideoFrameBuffers[3][])
{
    if(!GPUCreateStreamTextureDevice){
        AMLOG_ERROR("VideoEffecter: GPUCreateStreamTextureDevice is not initialized!\n");
        return -1;
    }
    if((*GPUCreateStreamTextureDevice)(width, height, pitch, mVideoFrameBuffers)<0){
        AMLOG_ERROR("VideoEffecter: CreateStreamTextureDevice Failed!\n");
        return -1;
    }
    return 0;
}
*/
/*
AM_INT CVideoEffecter::DestroyStreamTextureDevice()
{
    if(!GPUDestroyStreamTextureDevice){
        AMLOG_ERROR("VideoEffecter: GPUDestroyStreamTextureDevice is not initialized!\n");
        return -1;
    }
    if((*GPUDestroyStreamTextureDevice)()<0){
        AMLOG_ERROR("VideoEffecter: DestroyStreamTextureDevice Failed!\n");
        return -1;
    }
    mbStreamTextureDeviceCreated = false;
    return 0;
}
*/

void CVideoEffecter::PrintState()
{
    AMLOG_INFO("VideoEffecter: State:%d, \tpin0: %s, %d input data.\n\t\t\t\tpin1: %s, %d input data.\n\t\t\t\tpin2: %s, %d input data.\n", msState,
        mbInputPin[0]?"Enabled":"Disabled", mHasInput[0],
        mbInputPin[1]?"Enabled":"Disabled", mHasInput[1],
        mbInputPin[2]?"Enabled":"Disabled", mHasInput[2]);
}

//-----------------------------------------------------------------------
//
// CVideoEffecterInput
//
//-----------------------------------------------------------------------
CVideoEffecterInput* CVideoEffecterInput::Create(CFilter *pFilter)
{
    CVideoEffecterInput *result = new CVideoEffecterInput(pFilter);
    if (result && result->Construct()) {
        delete result;
        result = NULL;
    }
    AM_INFO("CVideoEffecterInput mpBufferPool: %p.\n", result->mpBufferPool);
    return result;
}

AM_ERR CVideoEffecterInput::Construct()
{
    AM_ERR err = inherited::Construct(((CVideoEffecter*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

AM_UINT CVideoEffecterInput::GetDataCnt() { return mpBufferQ->GetDataCnt();}

CVideoEffecterInput::~CVideoEffecterInput()
{
}

AM_ERR CVideoEffecterInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    return ((CVideoEffecter*)mpFilter)->SetInputFormat(pFormat);
}
/*
#ifndef show_on_osd
//-----------------------------------------------------------------------
//
// CVideoEffecterOutput
//
//-----------------------------------------------------------------------
CVideoEffecterOutput *CVideoEffecterOutput::Create(CFilter *pFilter)
{
    CVideoEffecterOutput *result = new CVideoEffecterOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CVideoEffecterOutput::Construct()
{
    return ME_OK;
}

CVideoEffecterOutput::~CAmbaVideoEffecterOutput()
{
    AM_PRINTF("~~CAmbaVideoEffecterOutput\n");
}

AM_ERR CVideoEffecterOutput::SetOutputFormat()
{
    mMediaFormat.pMediaType = &GUID_Amba_Decoded_Video;
    mMediaFormat.pSubType = &GUID_Video_YUV420NV12;
    mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Media;
    mMediaFormat.format = ((CVideoEffecter*)mpFilter)->mIavFd;
    return ME_OK;
}
#endif
*/

