
/*
 * android_audio_in.h
 *
 * History:
 *    2010/3/18 - [Kaiming Xie] created file
 *    2010/7/6 - [Luo Fei] modified file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AUDIO_INPUT_H__
#define __AUDIO_INPUT_H__

#include "audio_if.h"

class CAudioInput2Output;

class CAudioInput2: public CActiveFilter, public IAudioInput
{
    typedef CActiveFilter inherited;

public:
    static IFilter * Create(IEngine *pEngine);

private:
    CAudioInput2(IEngine *pEngine) :
        inherited(pEngine,"AudioInput2"),
        mPcmFormat(IAudioHAL::FORMAT_S16_LE),
        mSampleRate(48000),
        mNumOfChannels(1),
        mBitsPerFrame(32),
        mTimeOut(1000),
        mpAFile(NULL),
        mpBufferPool(NULL),
        mpOutput(NULL),
        mpAudioDriver(NULL),
        mbSkip(false),
        mCurrentBufferSize(MAX_AUDIO_BUFFER_SIZE),
        mSpecifiedFrameSize(0),
        mpReserveBuffer(NULL),
        mbBufferFlag(false),
        mAudioTimeInterval(0),
        mbAudioHALClosed(false),
        mToTSample(0),
        mbNeedFadeinAudio(false),
        mFadeInBufferNumber(0),
        mFadeInMaxBufferNumber(4),//some default value
        mFadeInCount(0)
    {}
    AM_ERR Construct();
    virtual ~CAudioInput2();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
    {
        if (refiid == IID_IAudioInput)
            return (IAudioInput*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete() { return inherited::Delete(); }

    // IFilter
    virtual void GetInfo(INFO& info);
    virtual IPin* GetOutputPin(AM_UINT index);
    virtual AM_ERR Stop();
    virtual AM_ERR FlowControl(FlowControlType type);

    bool ProcessCmd(CMD& cmd);
    // IActiveObject
    virtual void OnRun();

    // IParameters
    AM_ERR SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index = 0);

#ifdef AM_DEBUG
    virtual void PrintState();
#endif

private:
    void handleCloseAudioHAL();
    void handleReopenAudioHAL();

    void fakeReadStream(AM_U8 *pData, AM_UINT dataSize, AM_UINT *pNumFrames, AM_U64 *pTimeStamp);

private:
    IAudioHAL::PCM_FORMAT mPcmFormat;
    AM_UINT mSampleRate;
    AM_UINT mNumOfChannels;
    AM_UINT mBitsPerFrame;
    AM_UINT mTimeOut;

    IFileWriter *mpAFile;

    CSimpleBufferPool *mpBufferPool;
    CAudioInput2Output * mpOutput;

    IAudioHAL *mpAudioDriver;

    bool mbSkip;
    AM_UINT mCurrentBufferSize;

    AM_UINT mSpecifiedFrameSize;

    //add the ReserveBuffer inorder to read audio stream from input as soon as possible,
    //but it can't avoid losing audio data
    //chuchen, 2012_5_10
    AM_U8 *mpReserveBuffer;
    bool mbBufferFlag;//true: use the ReserveBuffer

private:
    AM_UINT mAudioTimeInterval;//us
    bool mbAudioHALClosed;
    AM_U64 mToTSample;
    bool mbNeedFadeinAudio;
    AM_UINT mFadeInBufferNumber;
    AM_UINT mFadeInMaxBufferNumber;
    AM_UINT mFadeInCount;
};

//-----------------------------------------------------------------------
//
// CAudioInput2Output
//
//-----------------------------------------------------------------------
class CAudioInput2Output: public COutputPin
{
    typedef COutputPin inherited;
    friend class CAudioInput2;

public:
    static CAudioInput2Output* Create(CFilter *pFilter);

private:
    CAudioInput2Output(CFilter *pFilter):
        inherited(pFilter)
    {
        mMediaFormat.pMediaType = &GUID_Audio;
        mMediaFormat.pSubType = &GUID_Audio_COOK;
        mMediaFormat.pFormatType = &GUID_NULL;
    }
    AM_ERR Construct();
    virtual ~CAudioInput2Output() {}

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

