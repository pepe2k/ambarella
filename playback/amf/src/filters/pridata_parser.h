/*
 * pridata_parser.h
 *
 * History:
 *    2011/12/13 - [Zhi He] created file
 *
 * Copyright (C) 2010-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __PRIDATA_PARSER_H__
#define __PRIDATA_PARSER_H__

class CPridataParserInput;

class CPridataParser: public CActiveFilter, public IClockObserver, public IDataRetriever
{
    typedef CActiveFilter inherited ;
    friend class CPridataParserInput;

    typedef struct {
        AM_U8* pdata;
        AM_U32 data_len;
        AM_U32 total_length;
        AM_U16 data_type;
        AM_U16 sub_type;

        AM_U16 needsend2app;
        am_pts_t pts;
    } SPriDataPacket;

public:
    static IFilter * Create(IEngine * pEngine);
    static int AcceptMedia(CMediaFormat& format);

protected:
    CPridataParser(IEngine * pEngine) :
        inherited(pEngine,"PridataParser"),
        mpClockManager(NULL),
        mnInput(0),
        mpMutex(NULL),
        mpInputBuffer(NULL),
        mbInCallBack(0),
        mPrivateDataCallbackCookie(NULL),
        mPrivatedataCallback(NULL)
    {
        for (AM_UINT i = 0; i < MAX_NUM_INPUT_PIN; i++) {
            mpInput[i] = NULL;
        }
    }
    AM_ERR Construct();
    virtual ~CPridataParser();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
    {
        if (refiid == IID_IDataRetriever)
            return (IDataRetriever*)this;
        else if (refiid == IID_IClockObserver)
            return (IClockObserver*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete();

    // IFilter
    virtual void GetInfo(INFO& info);
    virtual IPin* GetInputPin(AM_UINT index);
    virtual AM_ERR  Start();
    virtual AM_ERR Stop();

    // IActiveObject
    virtual void OnRun();

    // IFilter
    virtual AM_ERR AddInputPin(AM_UINT& index, AM_UINT type);

    // IClockObserver
    virtual AM_ERR OnTimer(am_pts_t curr_pts);

    //IDataRetriever
    virtual AM_ERR RetieveData(AM_U8* pdata, AM_UINT max_len, IParameters::DataCategory data_category = IParameters::DataCategory_PrivateData);
    virtual AM_ERR RetieveDataByType(AM_U8* pdata, AM_UINT max_len, AM_U16 type, AM_U16 sub_type, IParameters::DataCategory data_category = IParameters::DataCategory_PrivateData);
    virtual AM_ERR SetDataRetrieverCallBack(void* cookie, data_retriever_callback_t callback) {AUTO_LOCK(mpMutex); mPrivateDataCallbackCookie = cookie; mPrivatedataCallback = callback; return ME_OK;}

    virtual bool ProcessCmd(CMD& cmd);
#ifdef AM_DEBUG
    virtual void PrintState();
#endif

protected:
    bool ReadInputData(CPridataParserInput* inputpin);
    bool requestRun();

private:
    void postPrivateDataMsg(AM_U32 size, AM_U16 type, AM_U16 sub_type);
    void postDataByCallback(AM_U8* pdata, AM_U32 size, AM_U16 type, AM_U16 sub_type);
    AM_UINT sendPrivateData(am_pts_t& cur_time);
    void storePrivateData(AM_U8* pdata, AM_UINT max_len, am_pts_t pts);
    void freeAllPrivateData();
    void printBytes(AM_U8* p, AM_UINT size);

private:
    enum {
        MAX_NUM_INPUT_PIN = 2,
    };

    IClockManager* mpClockManager;
    AM_UINT mnInput;
    CPridataParserInput * mpInput[MAX_NUM_INPUT_PIN];
    CMutex* mpMutex;

    CBuffer* mpInputBuffer;

private:
    AM_UINT mbInCallBack;
    void* mPrivateDataCallbackCookie;
    data_retriever_callback_t mPrivatedataCallback;

private:
    CDoubleLinkedList mPridataList;
};

//-----------------------------------------------------------------------
//
// CPridataParserInput
//
//-----------------------------------------------------------------------
class CPridataParserInput: public CQueueInputPin
{
    typedef CQueueInputPin inherited;
    friend class CPridataParser;

public:
    static CPridataParserInput* Create(CFilter *pFilter);
public:
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
private:
    CPridataParserInput(CFilter *pFilter):
        inherited(pFilter)
    {
        mMediaFormat.pMediaType = &GUID_Pridata;
        mMediaFormat.pSubType = &GUID_NULL;
        mMediaFormat.pFormatType = &GUID_NULL;
    }
    AM_ERR Construct();
    virtual ~CPridataParserInput() {}

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
