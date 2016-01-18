/*
 * audio_encoder.h
 *
 * History:
 *    2010/07/12 - [Jay Zhang] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AUDIO_ENCODER_H__
#define __AUDIO_ENCODER_H__

class CAACEncoderInput;
class CAACEncoderOutput;

class CAACEncoder: public CActiveFilter, public IAudioEncoder
{
    typedef CActiveFilter inherited;

public:
    static IFilter * Create(IEngine * pEngine);

private:
    CAACEncoder(IEngine * pEngine) :
        inherited(pEngine,"CAACEncoder"),
        mbRunFlag(false),
        mAudioFormat(IParameters::StreamFormat_AAC),
        mSampleRate(48000),
        mNumOfChannels(2),
        mBitRate(128000),
        mBitsPerSample(16),
        mpInput(NULL),
        mpBufferPool(NULL),
        mpAFile(NULL),
//        mpTmpBuf(NULL),
        mpTmpEncBuf(NULL)
    {
        for (AM_INT i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
            mpOutput[i] = NULL;
        }
    }
    AM_ERR Construct();
    virtual ~CAACEncoder();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
    {
    if (refiid == IID_IAudioEncoder)
    	return (IAudioEncoder*)this;
    return inherited::GetInterface(refiid);
    }
    virtual void Delete() { return inherited::Delete(); }

    // IFilter
    virtual void GetInfo(INFO& info);
    virtual IPin* GetInputPin(AM_UINT index);
    virtual IPin* GetOutputPin(AM_UINT index);
    virtual AM_ERR Stop();

    // IActiveObject
    virtual void OnRun();

    // IParameters
    AM_ERR SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index = 0);

    // IFilter
    virtual AM_ERR AddOutputPin(AM_UINT& index, AM_UINT type);

private:
    AM_ERR OpenEncoder();

private:
    enum {
        MAX_NUM_OUTPUT_PIN = 2,
        //MAX_OUTPUT_FRAME_SIZE = 8192,
        AAC_LIB_TEMP_ENC_BUF_SIZE = 106000,
    };

    bool mbRunFlag;
    StreamFormat mAudioFormat;
    AM_UINT mSampleRate;
    AM_UINT mNumOfChannels;
    AM_UINT mBitRate;
    AM_UINT mBitsPerSample;
    //AM_UINT mChunkSize;
    //AM_UINT mTimeOut;

    CAACEncoderInput * mpInput;
    CSimpleBufferPool *mpBufferPool;
    CAACEncoderOutput * mpOutput[MAX_NUM_OUTPUT_PIN];

    IFileWriter *mpAFile;
//    AM_U8 *mpTmpBuf;
    AM_U8 *mpTmpEncBuf;
    au_aacenc_config_t au_aacenc_config;
};

//-----------------------------------------------------------------------
//
// CAACEncoderInput
//
//-----------------------------------------------------------------------
class CAACEncoderInput: CQueueInputPin
{
    typedef CQueueInputPin inherited;
    friend class CAACEncoder;

public:
    static CAACEncoderInput *Create(CFilter *pFilter);

protected:
    CAACEncoderInput(CFilter *pFilter):
    	inherited(pFilter)
    {}
    AM_ERR Construct();
    virtual ~CAACEncoderInput();

public:
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
};

//-----------------------------------------------------------------------
//
// CAACEncoderOutput
//
//-----------------------------------------------------------------------
class CAACEncoderOutput: public COutputPin
{
    typedef COutputPin inherited;
    friend class CAACEncoder;

public:
    static CAACEncoderOutput* Create(CFilter *pFilter);

private:
    CAACEncoderOutput(CFilter *pFilter):
    	inherited(pFilter)
    {
    	mMediaFormat.pMediaType = &GUID_Audio;
    	mMediaFormat.pSubType = &GUID_Audio_COOK;
    	mMediaFormat.pFormatType = &GUID_NULL;
    }
    AM_ERR Construct();
    virtual ~CAACEncoderOutput() {}

public:
    // IPin
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
    {
    	pMediaFormat = &mMediaFormat;
    	return ME_OK;
    }

private:
    CMediaFormat mMediaFormat;

};

#endif

