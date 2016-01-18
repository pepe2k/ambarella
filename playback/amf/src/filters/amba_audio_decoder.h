
/*
 * amba_audio_decoder.h
 *
 * History:
 *    2010/09/20 - [Zhi He] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBA_AUDIO_DECODER_H__
#define __AMBA_AUDIO_DECODER_H__

typedef enum
{
    eAuDecType_None = 0,
    eAuDecType_PCM
} EAmbaAudioDecoderType;

class CAmbaAudioDecoder;
class CAmbaAudioDecoderInput;
class CAmbaAudioDecoderOutput;

//-----------------------------------------------------------------------
//
// CAmbaAudioDecoderInput
//
//-----------------------------------------------------------------------
class CAmbaAudioDecoderInput: CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CAmbaAudioDecoder;

public:
	static CAmbaAudioDecoderInput *Create(CFilter *pFilter);

protected:
	CAmbaAudioDecoderInput(CFilter *pFilter):
		inherited(pFilter)
	{}
	AM_ERR Construct();
	virtual ~CAmbaAudioDecoderInput();

public:
	virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
};

//-----------------------------------------------------------------------
//
// CAmbaAudioDecoderOutput
//
//-----------------------------------------------------------------------
class CAmbaAudioDecoderOutput: COutputPin
{
	typedef COutputPin inherited;
	friend class CAmbaAudioDecoder;

public:
	static CAmbaAudioDecoderOutput *Create(CFilter *pFilter);

protected:
	CAmbaAudioDecoderOutput(CFilter *pFilter):
		inherited(pFilter)
		{}
	AM_ERR Construct();
	virtual ~CAmbaAudioDecoderOutput();

public:
	virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
	{
		pMediaFormat = &mMediaFormat;
		return ME_OK;
	}

private:
	CAudioMediaFormat mMediaFormat;
};

//-----------------------------------------------------------------------
//
// CAmbaAudioDecoder
//
//-----------------------------------------------------------------------
class CAmbaAudioDecoder: public CActiveFilter
{
    typedef CActiveFilter inherited;
    friend class CAmbaAudioDecoderInput;
    friend class CAmbaAudioDecoderOutput;

public:
    static IFilter *Create(IEngine *pEngine);
    static AM_INT AcceptMedia(CMediaFormat& format);

protected:
    CAmbaAudioDecoder(IEngine *pEngine):
        inherited(pEngine, "ambaAudioDecoder"),
        mpInput(NULL),
        mpOutput(NULL),
        mTimestamp(0),
        mpBuffer(NULL),
        mpPacket(NULL),
        mDataOffset(0),
        mpOriDataptr(NULL),
        mpStream(NULL),
        mpCodec(NULL),
        mbNeedDecode(true),
        mpMiddleUsedBuffer(NULL),
        mbChannelInterlave(false),
        mbChangeToNativeEndian(false),
        mpPCMDecoderContent(NULL),
        mDecType(eAuDecType_None)
    {}

    AM_ERR Construct();
    virtual ~CAmbaAudioDecoder();

public:
    virtual void Delete();
    virtual bool ProcessCmd(CMD& cmd);

    // IFilter
    virtual void GetInfo(INFO& info)
    {
        inherited::GetInfo(info);
        info.nInput = 1;
        info.nOutput = 1;
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

#ifdef AM_DEBUG
    virtual void PrintState();
#endif

private:
    bool SendEOS(CAmbaAudioDecoderOutput *pPin);
    AM_ERR SetInputFormat(CMediaFormat *pFormat);
    bool ReadInputData();
    void ProcessData();
    void CopyPCMData();
    void ChangeToNativeEndian();
    void ConvertChannleInterlaveToSampleInterlave(CBuffer*&);

private:
    AM_ERR SetupDecoder();
    AM_ERR CloseDecoder();
    AM_ERR DecodeData();

private:
    CAmbaAudioDecoderInput *mpInput;
    CAmbaAudioDecoderOutput *mpOutput;
    AM_U64 mTimestamp;

    AM_U32 decodedFrame;
    AM_U32 droppedFrame;
    AM_U32 totalTime;

private:
    IBufferPool *mpBufferPool;
    CBuffer *mpBuffer;
    AVPacket *mpPacket;
    AM_INT mDataOffset;
    AM_U8* mpOriDataptr;
    AVStream* mpStream;
    AVCodecContext* mpCodec;

private:
    //pcm related
    bool mbNeedDecode;// pcm data need convention to android audio format(1-2 channel, signed 8 or 16, native edian(little edian), sample interlave)
    AM_U8* mpMiddleUsedBuffer;//used for convert from channel interlave to sample interlave
    bool mbChannelInterlave;
    bool mbChangeToNativeEndian;
    CAudioMediaFormat mFormat;
    pcm_decode_cs_t* mpPCMDecoderContent;

private:
    EAmbaAudioDecoderType mDecType;
};

#endif

