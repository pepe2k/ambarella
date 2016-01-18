/*
 * general_decode_filter_ve.h
 *
 * History:
 *    2010/6/1 - [QingXiong Z] create file
 *    deprecated, used for transcoding, *** to be removed ***
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
 #ifndef __GENERAL_DECODE_FILTER_VE_H__
#define __GENERAL_DECODE_FILTER_VE_H__

//-----------------------------------------------------------------------
//
// CGeneralInputPinVE
//
//-----------------------------------------------------------------------
class CGeneralInputPinVE: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
public:
    //just want this pin be created by CInterActiveFilter
    static CGeneralInputPinVE* Create(CFilter* pFilter);

private:
    CGeneralInputPinVE(CFilter* pFilter):
        inherited(pFilter)
    { }
    AM_ERR Construct();
    virtual ~CGeneralInputPinVE();

public:
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
    AM_UINT GetDataCnt() { return mpBufferQ->GetDataCnt();}
};
CGeneralInputPinVE* CGeneralInputPinVE::Create(CFilter* pFilter)
{
    CGeneralInputPinVE* result = new CGeneralInputPinVE(pFilter);
    if (result && result->Construct())
    {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CGeneralInputPinVE::Construct()
{
    AM_ERR err;
    CQueue* pQueue = ((CActiveFilter* )mpFilter)->MsgQ();
    err = inherited::Construct(pQueue);
    if (err != ME_OK)
    {
        AM_PRINTF("CGeneralInputPinVE::Construct Failed.\n");
        return err;
    }
    return ME_OK;
}

CGeneralInputPinVE::~CGeneralInputPinVE()
{
    AM_PRINTF("~CGeneralOutputPinVE\n");
}

AM_ERR CGeneralInputPinVE::CheckMediaFormat(CMediaFormat* pFormat)
{
    //add this setinputformat to CInterActiveFilter
    AM_ERR err;
    err = ((CInterActiveFilter* )mpFilter)->SetInputFormat(pFormat);
    if(err != ME_OK)
    {
        AM_PRINTF("CheckMediaFormat Failed!\n");
        return err;
    }
    return ME_OK;
}

//-----------------------------------------------------------------------
//
// CGeneralOutputPinVE
//
//-----------------------------------------------------------------------
class CGeneralOutputPinVE: public COutputPin
{
    typedef COutputPin inherited;
    friend class CGeneralDecoderVE;

public:
    static CGeneralOutputPinVE* Create(CFilter *pFilter);

protected:
    CGeneralOutputPinVE(CFilter *pFilter):
        inherited(pFilter)
     {}
    AM_ERR Construct();
    virtual ~CGeneralOutputPinVE();

public:
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat);
private:
    CMediaFormat mMediaFormat;
};
CGeneralOutputPinVE* CGeneralOutputPinVE::Create(CFilter* pFilter)
{
    CGeneralOutputPinVE *result = new CGeneralOutputPinVE(pFilter);
    if (result && result->Construct() != ME_OK)
    {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CGeneralOutputPinVE::Construct()
{
    return ME_OK;
}

CGeneralOutputPinVE::~CGeneralOutputPinVE()
{
    AM_PRINTF("~CGeneralOutputPinVE\n");
}

AM_ERR CGeneralOutputPinVE::GetMediaFormat(CMediaFormat*& pMediaFormat)
{
    ((CInterActiveFilter* )mpFilter)->SetOutputFormat(&mMediaFormat);
    pMediaFormat = &mMediaFormat;
    return ME_OK;
}

//-----------------------------------------------------------------------
//
//  CGVideoBufferPool
//
//-----------------------------------------------------------------------
class CGVideoBufferPool: public CSimpleBufferPool
{
    typedef CSimpleBufferPool inherited;
public:
    static CGVideoBufferPool* Create(const char* name, AM_UINT count)
    {
        CGVideoBufferPool* result = new CGVideoBufferPool(name);
        if(result && result->Construct(count) != ME_OK)
        {
            delete result;
            result = NULL;
        }
        return result;
    }

    void SetDSPRelated(IUDECHandler* pUDECHandler, AM_INT udecIndex) {mpUDECHandler = pUDECHandler; mUdecIndex = udecIndex;}

protected:
    CGVideoBufferPool(const char* name):
        inherited(name),
        mpUDECHandler(NULL),
        mUdecIndex(eInvalidUdecIndex)
        {}
    AM_ERR Construct(AM_UINT count)
    {
        AM_UINT pSize = sizeof(CVideoBuffer);
        return inherited::Construct(count, pSize + sizeof(CBuffer));
    }
    virtual ~CGVideoBufferPool() {}

protected:
    virtual void OnReleaseBuffer(CBuffer *pBuffer);

private:
    IUDECHandler* mpUDECHandler;
    AM_INT mUdecIndex;
};

class CGeneralDecoderVE: public CInterActiveFilter
{
    typedef CInterActiveFilter inherited;

public:
    static IFilter* Create(IEngine *pEngine);
    static int AcceptMedia(CMediaFormat& format);
    
private:
    CGeneralDecoderVE(IEngine *pEngine):
        inherited(pEngine, "CGeneralDecoderVE"),
        //mpConfig(pConfig),
        mpMediaFormat(NULL),
        mbRun(false),
        mpBuffer(NULL),
        mpVDecoder(NULL),
        mpADecoder(NULL),
        mpBufferPool(NULL),
        mpInputPin(NULL),
        mpOutputPin(NULL)
    { }
    AM_ERR Construct();
    AM_ERR ConstructPin();
    AM_ERR ConstructInputPin();
    void GetInfo(INFO& info);

    virtual void Delete();
    virtual ~CGeneralDecoderVE();
    //todo change send or post
    virtual void Pause() {mpWorkQ->SendCmd(CMD_PAUSE);}
    virtual void Resume(){mpWorkQ->SendCmd(CMD_RESUME);}
    virtual void Flush(){mpWorkQ->SendCmd(CMD_FLUSH);}
    AM_ERR DoPause();
    AM_ERR DoResume();
    AM_ERR DoStop();
    AM_ERR DoFlush();
    AM_ERR DoConfig();

    virtual void MsgProc(AM_MSG& msg);
    virtual void OnRun();
    virtual bool ProcessCmd(CMD& cmd);
    AM_ERR ProcessEOS();


    AM_ERR IsDecoderReady();
    AM_ERR ReadInputData(CQueue::WaitResult& result);
    AM_ERR ProcessBuffer();
    //AM_ERR Renderbuffer();
    void DecodeVideo();
    void DecodeAudio();
    AM_ERR CreateDecoder();
    AM_ERR ChangeDecoder();
    AM_ERR SendBuffer(CBuffer* oBuffer);
    
public:
    IPin* GetInputPin(AM_UINT index);
    IPin* GetOutputPin(AM_UINT index);
    virtual AM_ERR SetInputFormat(CMediaFormat* pFormat);
    virtual AM_ERR SetOutputFormat(CMediaFormat* pFormat);
    AM_ERR ReSet();
    void PrintState();
private:
    //CDecoderConfig* mpConfig;
    CMediaFormat* mpMediaFormat;
    bool mbRun;
    CBuffer* mpBuffer;
    
    IDecoder* mpVDecoder;
    IDecoder* mpADecoder;

    CGVideoBufferPool* mpBufferPool;
    CGeneralInputPinVE* mpInputPin;
    CGeneralOutputPinVE* mpOutputPin;
};
#endif
