
/*
 * video_effecter.h
 *
 * History:
 *    2011/1/16 - [LiuGang] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __VIDEO_EFFECTOR_H__
#define __VIDEO_EFFECTOR_H__

#define MaxVideoStreamNumber  3

class CVideoEffecter;
class CVideoEffecterInput;
class CVideoEffecterOutput;

typedef struct decpp_bufpool_s {
    AM_U32 buf_nums[3];
    AM_INT buf_width[3];
    AM_INT buf_height[3];
    AM_INT buf_pitch[3];
    AM_U16 fid[3][5];
    AM_U32 buf_addr[3][5];
} decpp_bufpool_t;
//-----------------------------------------------------------------------
//
// CVideoEffecter
//
//-----------------------------------------------------------------------
class CVideoEffecter: public CActiveFilter, public IRender, public IVideoEffecterControl
{
    typedef CActiveFilter inherited;
    friend class CVideoEffecterInput;
    friend class CVideoEffecterOutput;

public:
    static IFilter* Create(IEngine *pEngine);
    static int AcceptMedia(CMediaFormat& format);

private:
    CVideoEffecter(IEngine *pEngine):
        inherited(pEngine, "VideoEffecter"),
        mTotalInputPinNumber(0),
        mbVideoStarted(false),
        mIavFd(-1),
        mbStreamTextureDeviceCreated(false)
        {
            memset(mpVideoEffecterInputPin, 0, sizeof(mpVideoEffecterInputPin));
            memset(mpBuffer, 0, sizeof(mpBuffer));
            memset(mbInputPin, false, sizeof(mbInputPin));
            memset(mbWaitEOS, false, sizeof(mbWaitEOS));
            memset(mbEOS, false, sizeof(mbEOS));
            memset(mEOSPTS, 0, 2*sizeof(iav_udec_status_t));
            memset(mLastPTS, 0, 2*sizeof(iav_udec_status_t));
            memset(mHasInput, 0, sizeof(mHasInput));
            memset(mFrames, 0, sizeof(mFrames));
            //memset(mVideoFrameBuffers, NULL, sizeof(mVideoFrameBuffers));
            //memset(mVideoFrameBuffersID, 0, sizeof(mVideoFrameBuffersID));
            memset(&mDecppBufpool, 0, sizeof(decpp_bufpool_t));
        }
    AM_ERR Construct();
    AM_ERR SetInputFormat(CMediaFormat *pFormat);

    AM_ERR RenderOutBuffer(const char* from, char* to_y, char* to_uv);
    AM_ERR RenderBuffer();
    AM_ERR rgba8888_2_yuv422(const char* from, AM_UINT width_rgb, AM_UINT height_rgb, AM_UINT stride_rgb,
		char* to_y, char* to_uv, AM_UINT width_yuv, AM_UINT height_yuv, AM_UINT stride_yuv);

    virtual ~CVideoEffecter();

public:
    // IFilter
    virtual AM_ERR Run();
    virtual AM_ERR Start();
    virtual AM_ERR AddInputPin(AM_UINT& index, AM_UINT type);

#ifdef AM_DEBUG
    virtual void PrintState();
#endif
    virtual void GetInfo(INFO& info);
    virtual IPin* GetInputPin(AM_UINT index);

    //IPin* GetOutputPin(AM_UINT index);

    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();
    // IRender
    virtual AM_ERR GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs);

    virtual bool ProcessCmd(CMD& cmd);

    // IActiveObject
    virtual void OnRun();
private:
    AM_ERR setupVideoEffecter();
    AM_U16 findFID(AM_U16 fid, AM_INT index);
    AM_ERR GetOutPic(iav_transc_frame_t& frame, AM_INT Index);
    AM_ERR WaitAllInputBuffer();
/*    AM_INT GetPinIndex(CQueueInputPin* pPin){
        for(int i=0; i<MaxVideoStreamNumber;++i){
            if(!mbInputPin[i])
                continue;
            else
                if(pPin == (CQueueInputPin*)mpVideoEffecterInputPin[i])
                    return i;
        }
        return -1;
    };
*/
    //GPU & OpenGL ES
    //AM_INT InitStreamTexture();
    //AM_INT InitGLES();
    //AM_INT CreateStreamTextureDevice(bool,int, int, int, bool,int, int, int, bool,int, int, int, unsigned int [3][]);
    //AM_INT DestroyStreamTextureDevice();

private:
    CVideoEffecterInput *mpVideoEffecterInputPin[3];
    bool mbInputPin[3];
    AM_UINT mTotalInputPinNumber;
    CVideoEffecterOutput *mpVideoEffecterOutputPin;

    AM_BOOL mbVideoStarted;
    int mIavFd;
    AM_BOOL mbStreamTextureDeviceCreated;

    bool mbWaitEOS[3];//Get eos from udec, wait decpp's EOS
    bool mbEOS[3];//Get eos from decpp
    iav_udec_status_t mEOSPTS[3];
    iav_udec_status_t mLastPTS[3];
    CBuffer *mpBuffer[3];
    iav_transc_frame_t mFrame[3];
    AM_INT mHasInput[3];
    AM_UINT mFrames[3];
    decpp_bufpool_t mDecppBufpool;
/*
    AM_U32 mVideoFrameBuffers[3][5];//decpp buffers
    CBuffer* mpHoldBufferPool[3][10];//buffers received but not rendered
    AM_INT mCurDecBuf[3];//current buffer in mpInputBufferPool[] get from decoder.
    AM_INT mHoldBufNum[3];//current buffers in mpHoldBufferPool[].
    AM_INT mVideoFrameBuffersID[40];//ID numbers >= MAX(frame->real_fb_id)
*/

    IBufferPool *mpBufferPool;

};


//-----------------------------------------------------------------------
//
// CVideoEffecterInput
//
//-----------------------------------------------------------------------
class CVideoEffecterInput: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
    friend class CVideoEffecter;

public:
    static CVideoEffecterInput *Create(CFilter *pFilter);

protected:
    CVideoEffecterInput(CFilter *pFilter):
        inherited(pFilter)
        {}
    AM_ERR Construct();
    virtual ~CVideoEffecterInput();

public:

    // CPin
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);

    AM_UINT GetDataCnt() ;

private:
    AM_UINT mIndex;
};

//-----------------------------------------------------------------------
//
// CVideoEffecterOutput
//
//-----------------------------------------------------------------------
class CVideoEffecterOutput: public COutputPin
{
    typedef COutputPin inherited;
    friend class CVideoEffecter;

public:
    static CVideoEffecterOutput *Create(CFilter *pFilter);

protected:
    CVideoEffecterOutput(CFilter *pFilter):
        inherited(pFilter)
        {}
    AM_ERR Construct();
    virtual ~CVideoEffecterOutput();

public:
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
    {
        pMediaFormat = &mMediaFormat;
        return ME_OK;
    }

private:
    AM_ERR SetOutputFormat();

private:
    CMediaFormat mMediaFormat;
};


#endif //__VIDEO_EFFECTOR_H__

