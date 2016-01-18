/*
 * pridata_composer.h
 *
 * History:
 *    2011/11/09 - [Zhi He] created file
 *
 * Copyright (C) 2010-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __PRIDATA_COMPOSER_H__
#define __PRIDATA_COMPOSER_H__

class CPridataComposerOutput;

class CPridataComposer: public CActiveFilter, public IClockObserver, public IPridataComposer
{
    typedef CActiveFilter inherited ;
    friend class CPridataComposerOutput;

    typedef enum {
        ePridataComposerWorkMode_passive = 0,
        ePridataComposerWorkMode_active,
    } EPridataComposerWorkMode;

    typedef struct {
        //write control
        am_pts_t cool_down;
        am_pts_t duration;
        AM_U16 data_type;
        AM_U16 need_write;
    } SPriDataDuration;

    typedef struct {
        am_pts_t pts;
        AM_U8* pdata;
        AM_U32 data_len;
        AM_U32 total_length;
        AM_U16 data_type;
        AM_U16 sub_type;

        SPriDataDuration* p_cooldown;
    } SPriDataPacket;

public:
    static IFilter * Create(IEngine * pEngine);

protected:
    CPridataComposer(IEngine * pEngine) :
        inherited(pEngine,"PridataComposer"),
        mpBufferPool(NULL),
        mpClockManager(NULL),
        mDuration(DefaultPridataDuration),
        mNextTimer(0),
        mPTS(0),
        mnOutput(0),
        mpMutex(NULL),
        mWorkMode(ePridataComposerWorkMode_passive),
        mpfUnPackedCallBack(NULL),
        mpfPackedCallBack(NULL),
        mDumpFileIndex(0),
        mnTotalDataTypes(0),
        mOverSampling(2)
    {
        for (AM_UINT i = 0; i < MAX_NUM_OUTPUT_PIN; i++) {
            mpOutput[i] = NULL;
        }
    }
    AM_ERR Construct();
    virtual ~CPridataComposer();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
    {
        if (refiid == IID_IPridataComposer)
            return (IPridataComposer*)this;
        else if (refiid == IID_IClockObserver)
            return (IClockObserver*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete() { return inherited::Delete(); }

    // IFilter
    virtual void GetInfo(INFO& info);
    virtual IPin* GetOutputPin(AM_UINT index);
    virtual AM_ERR Stop();
    virtual AM_ERR FlowControl(FlowControlType type);

    // IActiveObject
    virtual void OnRun();

    // IParameters
    AM_ERR SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index = 0);

    // IFilter
    virtual AM_ERR AddOutputPin(AM_UINT& index, AM_UINT type);

    // IClockObserver
    virtual AM_ERR OnTimer(am_pts_t curr_pts);

    //IPridataComposer
    virtual AM_ERR SetGetPridataCallback(AM_CallbackGetUnPackedPriData f_unpacked, AM_CallbackGetPackedPriData f_packed);
    virtual AM_ERR SetPridata(AM_U8* rawdata, AM_UINT len, AM_U16 data_type, AM_U16 sub_type);
    virtual AM_ERR SetPridataDuration(AM_UINT duration, AM_U16 data_type);

    virtual bool ProcessCmd(CMD& cmd);

#ifdef AM_DEBUG
    virtual void PrintState();
#endif

private:
    void SendoutBuffer(CBuffer* pBuffer);
    void setNextTimer();
    AM_UINT packPrivateData(AM_U8* dest, AM_UINT max_len, am_pts_t& pts);
    void sendFlowControlBuffer(FlowControlType type);
    void sendEOSBuffer();

    void getDurationStruct(SPriDataPacket* packet);
    void updateNeedWriteFlags();
    void clearNeedWriteFlags();
    void setDuration(am_pts_t new_duration);

private:
    enum {
        MAX_NUM_OUTPUT_PIN = 2,
    };

    CSimpleBufferPool *mpBufferPool;
    IClockManager* mpClockManager;
    am_pts_t mDuration;
    am_pts_t mNextTimer;
    am_pts_t mPTS;
    AM_UINT mnOutput;
    CPridataComposerOutput * mpOutput[MAX_NUM_OUTPUT_PIN];
    CMutex* mpMutex;

private:
    EPridataComposerWorkMode mWorkMode;

private:
    AM_CallbackGetUnPackedPriData mpfUnPackedCallBack;
    AM_CallbackGetPackedPriData mpfPackedCallBack;

private:
    CDoubleLinkedList mPridataList;
    CDoubleLinkedList mPriDurationList;

//debug use
private:
    AM_UINT mDumpFileIndex;
    AM_UINT mnTotalDataTypes;

private:
    AM_UINT mOverSampling;//option for store private data, 2 means 2 times up sampling
};

//-----------------------------------------------------------------------
//
// CPridataComposerOutput
//
//-----------------------------------------------------------------------
class CPridataComposerOutput: public COutputPin
{
    typedef COutputPin inherited;
    friend class CPridataComposer;

public:
    static CPridataComposerOutput* Create(CFilter *pFilter);

private:
    CPridataComposerOutput(CFilter *pFilter):
        inherited(pFilter)
    {
        mMediaFormat.pMediaType = &GUID_Pridata;
        mMediaFormat.pSubType = &GUID_NULL;
        mMediaFormat.pFormatType = &GUID_NULL;
    }
    AM_ERR Construct() { return ME_OK;}
    virtual ~CPridataComposerOutput() {}

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
