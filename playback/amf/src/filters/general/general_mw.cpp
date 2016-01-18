/*
 * general_mw.cpp
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

#include "general_header.h"

#include "general_mw.h"
//-----------------------------------------------------------------------
//
//  CGeneralBufferPool
//
//-----------------------------------------------------------------------
CGeneralBufferPool* CGeneralBufferPool::Create(const char* name, AM_UINT count)
{
    CGeneralBufferPool* result = new CGeneralBufferPool(name);
    if(result && result->Construct(count) != ME_OK)
    {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CGeneralBufferPool::Construct(AM_UINT count)
{
    //AM_UINT pSize = sizeof(AM_INTPTR);
    return inherited::Construct(count, sizeof(CGBuffer));
}

AM_ERR CGeneralBufferPool::Retrieve(CGBuffer* gBuffer)
{
    //AM_INFO("GBP Retrieve Buffer\n");
    CInterActiveFilter* filter = (CInterActiveFilter* )mpNotifyOwner;
    filter->RetrieveBuffer(gBuffer);
    mbRetrieve = AM_TRUE;
    gBuffer->Release();
    mbRetrieve = AM_FALSE;
    return ME_OK;
}

void CGeneralBufferPool::OnReleaseBuffer(CBuffer *pBuffer)
{
    //AM_INFO("GBP OnReleaseBuffer\n");
    if(mbRetrieve == AM_TRUE)
        return;

    CInterActiveFilter* filter = (CInterActiveFilter* )mpNotifyOwner;
    filter->ReleaseBuffer(pBuffer);
}
//-----------------------------------------------------------------------
//
// CGeneralOutputPin
//
//-----------------------------------------------------------------------
CGeneralOutputPin* CGeneralOutputPin::Create(CFilter *pFilter)
{
    CGeneralOutputPin *result = new CGeneralOutputPin(pFilter);
    if (result && result->Construct() != ME_OK)
    {
        AM_DELETE(result);
        result = NULL;
    }
    return result;
}

AM_ERR CGeneralOutputPin::GetMediaFormat(CMediaFormat*& pMediaFormat)
{
    ((CInterActiveFilter* )mpFilter)->SetOutputFormat(&mMediaFormat);
    pMediaFormat = &mMediaFormat;
        return ME_OK;
}
//-----------------------------------------------------------------------
//
//  CGeneralInputPin
//it is no clear for cqueueinputpin, maybe just aford a machine which can be waited by processmsgand data
//-----------------------------------------------------------------------
CGeneralInputPin* CGeneralInputPin::Create(CFilter* pFilter)
{
    CGeneralInputPin* result = new CGeneralInputPin(pFilter);
    if (result && result->Construct())
    {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CGeneralInputPin::Construct()
{
    AM_ERR err;
    CQueue* pQueue = ((CActiveFilter* )mpFilter)->MsgQ();
    err = inherited::Construct(pQueue);
    if (err != ME_OK)
    {
        AM_INFO("CGeneralInputPin::Construct Failed.\n");
        return err;
    }
    return ME_OK;
}

AM_ERR CGeneralInputPin::CheckMediaFormat(CMediaFormat *pFormat)
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
