/*
 * video_decoder_ffmpeg.h
 *
 * History:
 *    2011/5/27 - [Zhi He] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __VIDEO_DECODER_FFMPEG_H__
#define __VIDEO_DECODER_FFMPEG_H__

enum {
    DECType_notInited = 0,
    DECType_YUVPlane = 1,
    DECType_NV12DirectRendering,
    DECType_PipelineNV12DirectRendering,
};

class CGeneralDecoder;

class CVideoDecoderFFMpeg : public IDecoder, public CActiveObject
{
    typedef CActiveObject inherited;
    typedef CInterActiveFilter GFilter;

public:
    static IGDecoder* Create(IFilter* pFilter,CGConfig* pconfig);
    static AM_INT ParseBuffer(const CGBuffer* gBuffer);
    static AM_ERR ClearParse();
public:



private:
    CVideoDecoderFFMpeg(SharedResource* pSharedRes);
    AM_ERR Construct();
    virtual ~CVideoDecoderFFMpeg();
    virtual void *GetInterface(AM_REFIID refiid);
public:
// from IDecoder
    virtual AM_ERR ConstructContext(DecoderConfig* configData);
    virtual AM_ERR Decode(CBuffer* pBuffer);//need handle data/eos
    virtual void Flush();
    virtual void Stop();

    virtual AM_ERR IsReady(CBuffer* pBuffer)
    {
        if (msStatus == DecoderStatus_ready)
            return ME_OK;
        else if (msStatus == DecoderStatus_notInited)
            return ME_NOT_EXIST;
        else if (msStatus == DecoderStatus_genericError || msStatus == DecoderStatus_udecError)
            return ME_ERROR;
        else
            return ME_BUSY;
    }

    virtual AM_ERR FillEOS();
    virtual void PrintState() {AMLOG_PRINTF("msState %d.\n", msStatus);}

    virtual AM_UINT GetDecoderStatus();
    AM_INT GetDecoderInstance() {return mUdecIndex;}
    //virtual AM_ERR GetDecodedFrame(CBuffer * pVideoBuffer){return ME_NO_IMPL;}

    //CActiveObject
    virtual void Delete() { DestroyDecoderContext(); inherited::Delete(); }
    virtual void OnRun();

//direct rendering
    virtual void SetOutputPin(void* pOutPin);
    void GetHandler(IUDECHandler* & pUdecHandler, COutputPin* & pOutPin) {pUdecHandler = mpUDECHandler; pOutPin = mpOutputPin;}

    void DestroyDecoderContext();

protected:
    void DoFlush();
    void DoStop();
    AM_ERR ProcessBuffer();
    AM_ERR ProcessEOS();
    AM_ERR ProcessCmd(CMD& cmd);
    void ConvertFrame(CVideoBuffer *pBuffer);
    void planar2x(AM_U8* src, AM_U8* des, AM_INT src_stride, AM_INT des_stride, AM_UINT width, AM_UINT height);
    bool SendEOS();
    void SendAllVideoPictures();

private:
    static AM_INT mConfigIndex;

    IFilter* mpFilter;
    CGConfig* mpGConfig;
    CDecoderConfig* mpConfig;

    AM_UINT msStatus;
    //CInterActiveFilter* mpFilter;
    //COutputPin* mpOutputPin;
    SharedResource* mpSharedRes;

    IUDECHandler* mpUDECHandler;
    AM_INT mUdecIndex;
    AM_INT mbExtEdge;
    //DSPHandler* mpDSPHandler;
    amba_decoding_accelerator_t* mpAccelerator;
    amba_decoding_accelerator_t mAccelerator;

    AVFormatContext* mpAVFormat;
    AVCodecContext *mpCodec;
    AVStream *mpStream;
    AVCodec *mpDecoder;
    AM_INT mStreamIndex;
    AVFrame mFrame;

    CBuffer* tobeDecodedBuffer;
    CBuffer* mpBuffer;

    AM_INT mbOpened;
    AM_INT mbUseDSPAcceleration;
    AM_UINT mDecoderType;
    AM_UINT mDecoderFormat;
    AM_UINT mLastError;
    AM_U64 mCurrInputTimestamp;

    AM_U32 time_count_before_dec;
    AM_U32 time_count_after_dec;

    AM_U32 decodedFrame;
    AM_U32 droppedFrame;
    AM_U32 totalTime;
};

#endif

