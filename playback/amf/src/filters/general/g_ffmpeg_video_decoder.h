/*
 * g_ffmpeg_video_decoder.h
 *
 * History:
 *    2013/4/6 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __CG_FFMPEG_VIDEO_DECODER_H__
#define __CG_FFMPEG_VIDEO_DECODER_H__

#define GDECODER_FFMPEG_VIDEO_QNUM 32

class CGFFMpegVideoDecoder: public IGDecoder, public CActiveObject
{
    typedef CActiveObject inherited;
    typedef CInterActiveFilter GFilter;
    //using IMsgSink::MSG;
    enum{
        STATE_DECODING = LAST_COMMON_STATE,
    };

public:
    static AM_INT ParseBuffer(const CGBuffer* gBuffer);
    static AM_ERR ClearParse();
    static IGDecoder* Create(IFilter* pFilter,CGConfig* pconfig);

public:
    virtual void* GetInterface(AM_REFIID refiid)
    {
        //if (refiid == IID_IGDecoder)
            //return (IGDecoder*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete();
    // IGDecoder
    virtual AM_ERR IsReady(CGBuffer * pBuffer);
    virtual AM_ERR Decode(CGBuffer * pBuffer)
    {
        //fast process this action
        mState = STATE_DECODING;
        PostCmd(CMD_DECODE);
        return ME_OK;
    }
    virtual AM_ERR GetDecodedGBuffer(CGBuffer& oBuffer);
    virtual AM_ERR FillEOS();
    virtual AM_ERR PerformCmd(CMD& cmd, AM_BOOL isSend);
    virtual AM_ERR QueryInfo(AM_INT type, CParam64& param){ return ME_OK;}
    virtual AM_ERR AdaptStream(CGBuffer* pBuffer){ return ME_OK;}
    virtual AM_ERR OnReleaseBuffer(CGBuffer* buffer);
    virtual void Dump();
private:
    CGFFMpegVideoDecoder(IFilter* pFilter, CGConfig* pConfig);
    AM_ERR Construct();
    AM_ERR ConstructFFMpeg();
    void ClearQueue(CQueue* queue);
    virtual ~CGFFMpegVideoDecoder();

private:
    AM_ERR ProcessCmd(CMD & cmd);
    AM_ERR DoPause();
    AM_ERR DoResume();
    AM_ERR DoFlush();
    AM_ERR DoStop();
    AM_ERR DoDecode();
    AM_ERR DoIsReady();
    virtual void OnRun();
    virtual AM_ERR ReSet();
    AM_ERR FillHandleBuffer();

private:
    AM_ERR DecodeVideo();
    AM_ERR UpdateVideoInfo();
    AM_ERR FillVideoFrameQueue();

    void ProcessEOS();
    AM_ERR PostFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);
    AM_ERR SendFilterMsg(AM_UINT code, AM_INTPTR ptr = (AM_INTPTR)NULL);

private:
    static AM_INT mConfigIndex;
    static AM_INT mStreamIndex;
    static STREAM_TYPE mType;
    static CQueue* mpDataQ;

    IFilter* mpFilter;
    CGConfig* mpGConfig;
    CDecoderConfig* mpConfig;
    AM_INT mConIndex;
    STREAM_TYPE mStreamType;

    AVFormatContext* mpAVFormat;
    AVStream* mpStream;
    AVCodecContext* mpCodec;
    AVCodec *mpDecoder;
    AVFrame mFrame;

    //IMP flag(just add for audio decode 4/23)
    AM_INT mSentBufferCnt;
    AM_INT mBufferCnt;
    AM_UINT mConsumeCnt;

    AM_BOOL mbIsEOS;//if eos than must makesure sent out buffer clearly

    CGBuffer mBuffer;
    CGBuffer mOBuffer; //Output Buffer to g_decoder_filter

    CQueue* mpTempMianQ;
    CQueue* mpVideoQ;
    CQueue* mpInputQ;
    void* mpQOwner;

private:
    AM_INT mTotalFrame;
    AM_INT mDurationA;
    AM_INT mCurPtsA;
    struct timeval mTimeStart;
};

#endif
