/*
 * general_mw.h
 *
 * History:
 *    2012/4/6 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __GENERAL_HEADER_MW_H__
#define __GENERAL_HEADER_MW_H__

//========================================================//
class CGBuffer;


//-----------------------------------------------------------------------
//
//  CGeneralBufferPool
//
//-----------------------------------------------------------------------
class CGeneralBufferPool: public CSimpleBufferPool
{
    typedef CSimpleBufferPool inherited;
public:
    static CGeneralBufferPool* Create(const char* name, AM_UINT count);
    AM_ERR Retrieve(CGBuffer* gBuffer);
    CQueue* GetBufferQ() { return mpBufferQ;}
protected:
    CGeneralBufferPool(const char* name):
        inherited(name),
        mbRetrieve(AM_FALSE)
        {}
    AM_ERR Construct(AM_UINT count);
    virtual ~CGeneralBufferPool() {}

protected:
    virtual void OnReleaseBuffer(CBuffer *pBuffer);
private:
    AM_BOOL mbRetrieve;
};
//-----------------------------------------------------------------------
//
// CGeneralOutputPin
//
//-----------------------------------------------------------------------
class CGeneralOutputPin: public COutputPin
{
    typedef COutputPin inherited;
    friend class CGeneralDecoder;
    friend class CGeneralTransAudioSink;
    friend class CGeneralDemuxer;

public:
    static CGeneralOutputPin* Create(CFilter *pFilter);
protected:
    CGeneralOutputPin(CFilter *pFilter):
        inherited(pFilter)
     {}
    AM_ERR Construct()
    {
        return ME_OK;
    }
    virtual ~CGeneralOutputPin()
    {
        AM_PRINTF("~CGeneralOutputPin\n");
    }

public:
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat);
public:
    /*
    //overwrite the function about g-buffer;
    //this is ugly but harmless...
    bool AllocBuffer(CGBuffer*& pBuffer, AM_UINT size = 0)
    {
        bool rval;
        AM_ASSERT(mpBufferPool);
        CBuffer* cBuffer = (CBuffer*)pBuffer;
        rval = mpBufferPool->AllocBuffer(cBuffer, size);
        pBuffer = (CGBuffer* )cBuffer;
        return rval;
    }

    void SendBuffer(CGBuffer *pBuffer)
    {
        if (mpPeer)
            mpPeer->Receive(pBuffer);
        else {
        //AM_INFO("buffer dropped\n");
        pBuffer->Clear();
        }
    }
    */
private:
    CMediaFormat mMediaFormat;
};
//-----------------------------------------------------------------------
//
//  CGeneralInputPin
//it is no clear for cqueueinputpin, maybe just aford a machine which can be waited by processmsgand data
//-----------------------------------------------------------------------
class CGeneralInputPin: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
public:
    //just want this pin be created by CInterActiveFilter
    static CGeneralInputPin* Create(CFilter* pFilter);

private:
    CGeneralInputPin(CFilter* pFilter):
        inherited(pFilter)
    { }
    AM_ERR Construct();
    virtual ~CGeneralInputPin()
    {
        AM_PRINTF("~CGeneralOutputPin\n");
    }


public:
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
    AM_UINT GetDataCnt() { return mpBufferQ->GetDataCnt();}
    CQueue* GetBufferQ() { return mpBufferQ;}
};
#endif
