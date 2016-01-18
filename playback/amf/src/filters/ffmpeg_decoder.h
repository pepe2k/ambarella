
/*
 * ffmpeg_decoder.h
 *
 * History:
 *    2010/1/27 - [Oliver Li] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FFMPEG_DECODER_H__
#define __FFMPEG_DECODER_H__

class CFFMpegDecoder;
class CFFMpegDecoderInput;
class CFFMpegDecoderOutput;

//-----------------------------------------------------------------------
//
// CFFMpegDecoderInput
//
//-----------------------------------------------------------------------
class CFFMpegDecoderInput: CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CFFMpegDecoder;

public:
	static CFFMpegDecoderInput *Create(CFilter *pFilter);

protected:
	CFFMpegDecoderInput(CFilter *pFilter):
		inherited(pFilter)
	{}
	AM_ERR Construct();
	virtual ~CFFMpegDecoderInput();

public:
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
};

//-----------------------------------------------------------------------
//
// CFFMpegDecoderOutput
//
//-----------------------------------------------------------------------
class CFFMpegDecoderOutput: COutputPin
{
	typedef COutputPin inherited;
	friend class CFFMpegDecoder;

public:
	static CFFMpegDecoderOutput *Create(CFilter *pFilter);

protected:
	CFFMpegDecoderOutput(CFilter *pFilter):
		inherited(pFilter)
		{}
	AM_ERR Construct();
	virtual ~CFFMpegDecoderOutput();

public:
	virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
	{
		pMediaFormat = &mMediaFormat;
		return ME_OK;
	}

private:
	AM_ERR SetOutputFormat();
	void CalcVideoDimension(AVCodecContext *pCodec);

private:
	CFFMpegMediaFormat mMediaFormat;
};

//-----------------------------------------------------------------------
//
// CFFMpegDecoder
//
//-----------------------------------------------------------------------
class CFFMpegDecoder: public CActiveFilter
{
    typedef CActiveFilter inherited;
    friend class CFFMpegDecoderInput;
    friend class CFFMpegDecoderOutput;

public:
    enum {VIDEO_BUFFER_ALIGNMENT_WIDTH=16,VIDEO_BUFFER_ALIGNMENT_HEIGHT=16 };
    static IFilter *Create(IEngine *pEngine);
    static int AcceptMedia(CMediaFormat& format);
    static int DecoderGetBuffer(AVCodecContext *s, AVFrame *pic);
    static void DecoderReleaseBuffer(AVCodecContext *s, AVFrame *pic);

protected:
    CFFMpegDecoder(IEngine *pEngine):
        inherited(pEngine, "FFMpegDecoder"),
        mpInput(NULL),
        mpOutput(NULL),
        mpCodec(NULL),
        mpDecoder(NULL),
        mbOpen(false),
        mbError(false),
        mbNV12(true),
        mbParallelDecoder(false),
        mCurrInputTimestamp(0),
        mbNeedSpeedUp(0),
        mpBufferPool(NULL),
        mbPoolAlloc(false),
        mpBuffer(NULL),
        mpPacket(NULL),
        mDataOffset(0),
        mpOriDataptr(NULL),
        mnFrameReservedMinus1(0),
        mPTSSubtitle_Num(0),
        mPTSSubtitle_Den(0),
        mpDSPHandler(NULL),
        mpAccelerator(NULL),
        mbIsFirstPTS(true),
        mStartPTS(0),
        mFrameCount(0),
        mAccFramePTS(0),
        mDspIndex(0)
    {}
    AM_ERR Construct();
    virtual ~CFFMpegDecoder();

public:
    virtual void Delete();
    virtual bool ProcessCmd(CMD& cmd);

    // IFilter
    virtual void GetInfo(INFO& info)
    {
        inherited::GetInfo(info);
        info.nInput = 1;
        info.nOutput = 1;
        info.mPriority = 0;
        info.mFlags = 0;
        info.mIndex = mDspIndex;
    }
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

    // IActiveObject
    virtual void OnRun();

    // IFilter
    virtual AM_ERR Stop();
#ifdef AM_DEBUG
    virtual void PrintState();
#endif

private:
    bool SendEOS(CFFMpegDecoderOutput *pPin);
    AM_ERR SetInputFormat(CMediaFormat *pFormat);
    AM_ERR OpenDecoder();
    void CloseDecoder();
    void ErrorStop();
    bool ReadInputData();
    void ConvertFrame(CVideoBuffer *pBuffer);//iav's buffer's dimension may differ with ffmpeg's buffer, because of different alignment
    void planar2x(AM_U8* src, AM_U8* des, AM_INT src_stride, AM_INT des_stride, AM_UINT width, AM_UINT height);
    void ProcessVideo();
    void ProcessAudio();
    void ProcessSubtitle();
    //Get the style and text of dialogure. added by cx
    AM_ERR ParseSubAssDialogue(AM_U8* dialog, void* output);
    //Get the field content and move the source point to next item
    AM_UINT GetSubAssFieldContent(AM_U8* dest, AM_U8* source);
    //Skip space, return the space number, return -1 when at the end of buffer.
    AM_INT SkipSpace(AM_U8* buffer);
    //Convert timestamp format to int.
    AM_ERR ConvertSubTimestamp(AM_U8* buffer, AM_U64* time);
    //Convert \N to '\n', and any other convertion
    void ConvertSubAssString(AM_U8* ass_text);
    //Convert pts
    void ConvertSubtitlePTS(AM_U64* pts);
    void ProcessEOS();
    void speedUp();

private:
    CFFMpegDecoderInput *mpInput;
    CFFMpegDecoderOutput *mpOutput;
    AVCodecContext *mpCodec;
    AVStream *mpStream;
    AVCodec *mpDecoder;
    bool mbOpen;
    bool mbError;
    bool mbNV12;
    bool mbParallelDecoder;
    AVFrame mFrame;
    AM_U64 mCurrInputTimestamp;

    AM_U32 time_count_before_dec;
    AM_U32 time_count_after_dec;

    AM_U32 decodedFrame;
    AM_U32 droppedFrame;
    AM_U32 totalTime;

    AM_U8 mbNeedSpeedUp;
    AM_U8 mReserved1[3];

private:
    IBufferPool *mpBufferPool;
    bool mbPoolAlloc;
    CBuffer *mpBuffer;
    AVPacket *mpPacket;
    AM_INT mDataOffset;
    AM_U8* mpOriDataptr;
    AM_UINT mnFrameReservedMinus1;//pipeline needed frame number
    AM_UINT mPTSSubtitle_Num;
    AM_UINT mPTSSubtitle_Den;
private:
    DSPHandler* mpDSPHandler;
    amba_decoding_accelerator_t* mpAccelerator;

    bool mbIsFirstPTS;
    AM_U64 mStartPTS;
    AM_INT mFrameCount;
    AM_U64 mAccFramePTS;
    AM_INT mDspIndex;
};

#endif

