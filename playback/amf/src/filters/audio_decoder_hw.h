/*
 * video_encoder.cpp
 *
 * History:
 *    2011/7/18 - [QingXiong] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AUDIO_DECODER_HW_H__
#define __AUDIO_DECODER_HW_H__


//-------------------------------------------------------------
//
// CAudioHWInput
//
//-------------------------------------------------------------
class CAudioHWInput: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
    friend class CAudioDecoderHW;
public:
    static CAudioHWInput* Create(CFilter* pFilter);
protected:
    CAudioHWInput(CFilter *pFilter):
        inherited(pFilter)
    { }
    AM_ERR Construct();
    virtual ~CAudioHWInput();
public:
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
};

//-------------------------------------------------------------
//
// CAudioHWOutput
//
//-------------------------------------------------------------
class CAudioHWOutput: public COutputPin
{
    typedef COutputPin inherited;
    friend class CAudioDecoderHW;
public:
    static CAudioHWOutput* Create(CFilter* pFilter);
protected:
    CAudioHWOutput(CFilter *pFilter):
        inherited(pFilter)
    { }
    AM_ERR Construct();
    virtual ~CAudioHWOutput();
public:
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
    {
        pMediaFormat = &mMediaFormat;
        return ME_OK;
    }
    AM_ERR SetOutputFormat();

private:
    CAudioMediaFormat mMediaFormat;
};

//-------------------------------------------------------------
//
// CAudioDecoderHW
//
//-------------------------------------------------------------
class CAudioDecoderHW: public CActiveFilter
{
    typedef CActiveFilter inherited;
    friend class CAudioHWInput;
    friend class CAudioHWOutput;
public:
    static IFilter* Create(IEngine* pEngine);
    static AM_INT AcceptMedia(CMediaFormat& format);
protected:
    CAudioDecoderHW(IEngine* pEngine):
        inherited(pEngine, "CAudioDecoderHW"),
        mpInput(NULL),
        mpOutput(NULL),
        mpBuffer(NULL),
        mpOutBuffer(NULL),
        mpBufferPool(NULL),
        mpStream(NULL),
        mbRun(false),
        mFrameNum(0)
    { }
    AM_ERR Construct();
    virtual ~CAudioDecoderHW();
public:
    virtual void Delete();
    virtual void GetInfo(INFO& info);
    virtual IPin* GetInputPin(AM_UINT index)
    {
        if (index == 0)
            return mpInput;
        return NULL;
    }
    virtual IPin* GetOutputPin(AM_UINT index)
    {
        if (index == 0)
            return mpOutput;
        return NULL;
    }
#ifdef AM_DEBUG
    virtual void PrintState();
#endif
private:
    virtual void OnRun();
    AM_ERR SetInputFormat(CMediaFormat* pFormat);
    AM_ERR SetupHWDecoder();
    AM_ERR ProcessBuffer();
    virtual bool ProcessCmd(CMD& cmd);
    AM_ERR ReadInputData();
    AM_ERR SendBuffer();
    AM_ERR ProcessEOS();
    void Audio32iTo16i(AM_S32* bufin, AM_S16* bufout, AM_S32 ch, AM_S32 proc_size);
    //AM_ERR AddSyncWord(AVPacket* pPacket);
    AM_ERR AddAdtsHeader(AVPacket* pPacket);
    AM_ERR WriteBitBuf(AVPacket* pPacket, int offset);
    AM_ERR InitHWAttr();

private:
    CAudioHWInput* mpInput;
    CAudioHWOutput* mpOutput;

    CBuffer* mpBuffer;
    CBuffer* mpOutBuffer;
    IBufferPool* mpBufferPool;
    AVStream* mpStream;

    bool mbRun;
    AM_UINT msOldState;
    AM_INT mFrameNum;
    //hw
    static AM_U32 mpDecMem[106000];//aacdec work memory.
    static AM_U8 mpDecBackUp[252];
    static AM_U8 mpInputBuf[16384];
    static AM_U32 mpOutBuf[8192];

};
#endif
