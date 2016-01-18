/*
 * audio_effecter.h
 *
 * History:
 *    2010/10/20 - [He Zhi] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AUDIO_EFFECTER_H__
#define __AUDIO_EFFECTER_H__

#define D_AUDIO_MAX_CHANNELS  8

//sub modules
typedef enum {
    eNone = 0,
    eFormatConvertor,
    eDeinterleaver,
    eDeinterleaverDownmixer,
    eInterleaver,
    eDownMixer,
    eSampleRateConvertor,
} EAudioEffectType;

//effect flags
#define DEffect_SampleRateConvertor 0x1
#define DEffect_DownMixer 0x2

typedef struct
{
    AM_UINT param1, param2;
    AM_UINT param3, param4;
}SEffectParameter;

typedef struct
{
    AM_UINT isChannelInterleave;
    AM_UINT isSampleInterleave;
    AM_UINT sampleRate;
    AM_INT sampleFormat;
//    AM_UINT nSamples;
    AM_UINT nChannels;
}SEffectFormat;

typedef struct
{
    AM_UINT bufferSize;
    AM_UINT bufferNumber;
    AM_U8* pBuffer[D_AUDIO_MAX_CHANNELS];
    AM_U8* pBufferBase[D_AUDIO_MAX_CHANNELS];
}SEffectBuffer;

class CEffectElement
{
protected:
    #define DElementNameLength 30
    CEffectElement(const char* name);

public:
    virtual ~CEffectElement() {}
    virtual bool SetParameter(SEffectParameter* pParam);          //effect parameter
    virtual bool ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output) {return false;};  //effect can support it or not
    virtual AM_UINT Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples) {return 0;};          //processed samples

    virtual bool CanShareBuffer() {return mbCanShareBuffer;}
    virtual bool IsSkip() {return mbSkip;}
    virtual void QueryBufferInfo(AM_UINT& samplesize, AM_UINT& channels, AM_UINT& isChannelInterleave, AM_UINT& isSampleInterleave, bool isInput);
    virtual const char* GetName() {return mpName;};

protected:
    SEffectFormat mInputFormat, mOutputFormat;
    SEffectParameter mEffectParameter;
    bool mbConfiged;
    bool mbSkip;
    bool mbCanShareBuffer;
    char mpName[DElementNameLength + 1];
};

class CFormatConvertor: public CEffectElement
{
    typedef CEffectElement inherited;

public:
    CFormatConvertor():
        inherited("FormatConvertor")
    {}

    virtual ~CFormatConvertor() {}

    virtual bool ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output);
    virtual AM_UINT Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples);

protected:
    AM_ERR ConvertFormat(AM_U8 * in, AM_U8 * out, AM_UINT istride, AM_UINT ostride, AM_U8 * end);
};

class CDeInterleaver: public CEffectElement
{
    typedef CEffectElement inherited;

public:
    CDeInterleaver():
        inherited("DeInterleaver")
    {}

    virtual ~CDeInterleaver() {}

    virtual bool ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output);
    virtual AM_UINT Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples);

};

class CInterleaver: public CEffectElement
{
    typedef CEffectElement inherited;

public:
    CInterleaver():
        inherited("Interleaver")
    {}

    virtual ~CInterleaver() {}

    virtual bool ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output);
    virtual AM_UINT Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples);

};

class CDownMixer: public CEffectElement
{
    typedef CEffectElement inherited;

public:
    CDownMixer():
        inherited("DownMixer")
    {}

    virtual ~CDownMixer() {}

    virtual bool ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output);
    virtual AM_UINT Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples);

};

class CSimpleDownMixer: public CEffectElement
{
    typedef CEffectElement inherited;

public:
    CSimpleDownMixer():
        inherited("SimpleDownMixer")
    {}

    virtual ~CSimpleDownMixer() {}

    virtual bool ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output);
    virtual AM_UINT Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples);

};

class CDeInterleaverDownMixer: public CEffectElement
{
    typedef CEffectElement inherited;

public:
    CDeInterleaverDownMixer():
        inherited("DeInterleaverDownMixer")
    {}

    virtual ~CDeInterleaverDownMixer() {}

    virtual bool ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output);
    virtual AM_UINT Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples);

};

class CSampleRateConvertor: public CEffectElement
{
    typedef CEffectElement inherited;

public:
    CSampleRateConvertor():
        inherited("SampleRateConvertor"),
        mpResampler(NULL)
    {
        memset((void*)mReservedSamples, 0x0, sizeof(mReservedSamples));
    }

    virtual ~CSampleRateConvertor()
    {
        if (mpResampler) {
            av_resample_close(mpResampler);
        }
    }

    virtual bool ConfigInputOutputFormat(SEffectFormat* input, SEffectFormat* output);
    virtual AM_UINT Process(SEffectBuffer* pInputBuffer, SEffectBuffer* pOutputBuffer, AM_UINT nSamples);

private:
    struct AVResampleContext * mpResampler;
    AM_UINT mReservedSamples[D_AUDIO_MAX_CHANNELS];
};

typedef struct SEffectElementList
{
    CEffectElement* pElement;
    EAudioEffectType type;
    SEffectFormat inputFormat, outputFormat;
    SEffectBuffer inputBuffer;
    SEffectParameter parameter;

    SEffectElementList *pPre, *pNext;
    AM_UINT bInputAllocated;
} SEffectElementList;

class CAudioEffecter;
class CAudioEffecterInput;
class CAudioEffecterOutput;

//-----------------------------------------------------------------------
//
// CAudioEffecterInput
//
//-----------------------------------------------------------------------
class CAudioEffecterInput: CQueueInputPin
{
    typedef CQueueInputPin inherited;
    friend class CAudioEffecter;

public:
    static CAudioEffecterInput *Create(CFilter *pFilter);

protected:
    CAudioEffecterInput(CFilter *pFilter):
        inherited(pFilter)
    {}
    AM_ERR Construct();
    virtual ~CAudioEffecterInput();

public:
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);

private:
    CAudioMediaFormat mMediaFormat;
};

//-----------------------------------------------------------------------
//
// CAudioEffecterOutput
//
//-----------------------------------------------------------------------
class CAudioEffecterOutput: COutputPin
{
    typedef COutputPin inherited;
    friend class CAudioEffecter;

public:
    static CAudioEffecterOutput *Create(CFilter *pFilter);

protected:
    CAudioEffecterOutput(CFilter *pFilter):
        inherited(pFilter)
    {}
    AM_ERR Construct();
    virtual ~CAudioEffecterOutput();

public:
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
    {
        pMediaFormat = &mMediaFormat;
        return ME_OK;
    }

private:
	CAudioMediaFormat mMediaFormat;
};

class CSharedAudioBuffer: public CAudioBuffer
{
public:
    bool mbAttached;
    CAudioBuffer* mpCarrier;
};

//-----------------------------------------------------------------------
//
// CAttachedBufferPool
//
//-----------------------------------------------------------------------
class CAttachedBufferPool: public CBufferPool
{
    typedef CBufferPool inherited;

public:
    static CAttachedBufferPool* Create(AM_UINT size, AM_UINT count);
    void AttachToBufferPool(IBufferPool *pBufferPool);
    void Detach();

protected:
    CAttachedBufferPool(): inherited("CSharedBufferPool") {}
    AM_ERR Construct(AM_UINT size, AM_UINT count);
    virtual ~CAttachedBufferPool();
    //virtual void OnReleaseBuffer(CBuffer *pBuffer);
private:
    IBufferPool* mpAttachedBufferPool;
    AM_UINT mnBuffers;
    CSharedAudioBuffer *_pBuffers;
};

//-----------------------------------------------------------------------
//
// CAudioEffecter
//
//-----------------------------------------------------------------------
class CAudioEffecter: public CActiveFilter
{
    typedef CActiveFilter inherited;
    friend class CAudioEffecterInput;
    friend class CAudioEffecterOutput;

    //80*8
    #define DSampleConvertorReservedMemorySize 640

public:
    static IFilter *Create(IEngine *pEngine);
    static AM_INT AcceptMedia(CMediaFormat& format);

protected:
    CAudioEffecter(IEngine *pEngine):
        inherited(pEngine, "AudioEffecter"),
        mpInput(NULL),
        mpOutput(NULL),
        decodedFrame(0),
        droppedFrame(0),
        totalTime(0),
        mpInBuffer(NULL),
        mpBufferPool(NULL),
        mpAttachedBufferPool(NULL),
        mnElement(0),
        mpElementListHead(NULL),
        mpElementListTail(NULL),
        mpAddEffectEntryPoint(NULL),
        mEffectFlag(0),
        mbDeInterleaverDownmixer(false),
        mnEffects(0),
        mbSkip(true),
        mGuessedInSize(0),
        mGuessedOutSize(0),
        mBufferSizeM(1),
        mBufferSizeN(1),
        mReservedBuffers(0),
        mMaxOutSamples(0),
        mpReservedBufferBase(NULL),
        mReservedToTSize(0),
        mpCurrentPtr(NULL),
        mCurrentDataSize(0),
        mPTS(0),
        mbPipelineInputBufferAllocated(false)
    {}

    AM_ERR Construct();
    virtual ~CAudioEffecter();

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
    bool SendEOS(CAudioEffecterOutput *pPin);
    AM_ERR SetInputFormat();
    bool ReadInputData();
    void ProcessData();
    void ProcessRemainingDataOnly();

private:
    void ConveyBuffers();
    void PrepareInOutBuffer(CBuffer* pOutBuffer);
    CEffectElement* ElementFactory(EAudioEffectType type, AM_UINT& index);
    AM_ERR SetupElement(SEffectElementList* pNode);
    SEffectElementList* AddNewNode(SEffectElementList* list);
    AM_UINT ExecutePipeline(AM_UINT nSamples);
    AM_ERR ConfigPipeline(CAudioMediaFormat* input, CAudioMediaFormat* output);
    AM_ERR SetupPipelineBuffers();
    void ClearPipeline();
    void UpdateFormat();

private:
    CAudioEffecterInput *mpInput;
    CAudioEffecterOutput *mpOutput;

    AM_U32 decodedFrame;
    AM_U32 droppedFrame;
    AM_U32 totalTime;

private:
    CBuffer *mpInBuffer;
    IBufferPool *mpBufferPool;
    CAttachedBufferPool*mpAttachedBufferPool;

private:
    //CAudioMediaFormat mInFormat;
    //CAudioMediaFormat mOutFormat;
    AM_UINT mnElement;
    SEffectElementList* mpElementListHead;//source
    SEffectElementList* mpElementListTail;//sink
    SEffectElementList* mpAddEffectEntryPoint;

private:
    AM_UINT mEffectFlag;
    //bool mbConvertInput;
    //bool mbConvertOutput;
    bool mbDeInterleaverDownmixer;
    //bool mbInterleaver;
    AM_UINT mnEffects;
    bool mbSkip;//the filter need nothing(no effects and no convert/interlave needed)

private:
    AM_UINT mGuessedInSize, mGuessedOutSize;//
    AM_UINT mBufferSizeM;
    AM_UINT mBufferSizeN;
    AM_UINT mReservedBuffers;

private:
    //per channel
    AM_UINT mMaxOutSamples;
    AM_UINT mMaxInSamples;
    AM_UINT mCurrentInSamples;

private:
    AM_U8* mpReservedBufferBase;
    AM_UINT mReservedToTSize;
    AM_U8* mpCurrentPtr;
    AM_UINT mCurrentDataSize;
    am_pts_t mPTS;
    bool mbPipelineInputBufferAllocated;
};

#endif

