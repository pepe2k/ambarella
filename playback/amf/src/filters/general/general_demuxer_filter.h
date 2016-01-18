/*
 * general_demuxer_filter.cpp
 *
 * History:
 *    2012/3/27 - [QingXiong Z] create file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __GENERAL_DEMUXER_FILTER_H__
#define __GENERAL_DEMUXER_FILTER_H__


#define GDEMUXER_AUDIO_CTRL_BP 16
#define GDEMUXER_VIDEO_CTRL_BP 16

class CGeneralDemuxer: public CInterActiveFilter
{
    typedef CInterActiveFilter inherited;
public:
    static CGeneralDemuxer* Create(IEngine* pEngine, CGConfig* pConfig);

public:
    //My api
    AM_ERR AcceptMedia(const char* filename, AM_BOOL run = AM_TRUE);
    AM_ERR ReSet();
    AM_ERR SeekTo(AM_INT target, AM_U64 ms, AM_INT flag = 0);
    AM_ERR AudioCtrl(AM_INT target, AM_INT flag = SYNC_AUDIO_LASTPTS);
    void Dump(AM_INT flag = 0);
    AM_ERR QueryInfo(AM_INT target, AM_INT type, CParam64& param);
    AM_ERR GetStreamInfo(AM_INT index, CParam& param);
    AM_ERR UpdatePBDirection(AM_INT target, AM_U8 flag);

    //CInterActiveFilter
    virtual AM_ERR ReleaseBuffer(CBuffer* buffer);
    virtual AM_ERR RetrieveBuffer(CBuffer* buffer);
    virtual IPin* GetOutputPin(AM_UINT index);
    virtual void GetInfo(INFO& info);
    virtual void Delete();
private:
    CGeneralDemuxer(IEngine* pEngine, CGConfig* pConfig):
        inherited(pEngine, "CGeneralDemuxer"),
        mpConfig(pConfig),
        mpAudioPin(NULL),
        mpVideoPin(NULL),
        mpCurPin(NULL),
        mpVideoBufferPool(NULL),
        mpAudioBufferPool(NULL),
        mpCurBufferPool(NULL),
        mSpecify(-1)
    {
        AM_INT i = 0;
        for(; i < MDEC_SOURCE_MAX_NUM; i++)
            mpDemuxer[i] = NULL;
        mbRun = false;
        mBuffer.Clear();
    }
    AM_ERR Construct();
    AM_ERR ConstructPin();
    virtual ~CGeneralDemuxer();

private:
    AM_BOOL NeedAdvanceOut();
    CQueue::QType WaitPolicy(CMD& cmd, CQueue::WaitResult& result);
    virtual void MsgProc(AM_MSG& msg);
    virtual void OnRun();
    virtual bool ProcessCmd(CMD& cmd);

    AM_ERR HandDemuxerCmd(AM_INT target, AM_UINT code, AM_BOOL isSend);
    AM_ERR DoPause(AM_INT target, AM_U8 flag);
    AM_ERR DoResume(AM_INT target, AM_U8 flag);
    AM_ERR DoFlush(AM_INT target, AM_U8 flag);
    AM_ERR DoConfig(AM_INT target, AM_U8 flag);
    AM_ERR DoStop();
    AM_ERR DoUpdatePBDirection(AM_INT target, AM_U8 flag);
    AM_ERR DoRemove(AM_UINT index, AM_U8 flag);

    AM_ERR ReadInputData(CQueue::WaitResult& result);
    AM_ERR ProcessBuffer();
    AM_ERR SendAudioStream(AM_INT index);
    void SetCBuffer(CGBuffer* gBuffer);
private:
    CGConfig* mpConfig;
    AM_INT msOldState;

    CGBuffer mBuffer;
    CGeneralOutputPin* mpAudioPin;
    CGeneralOutputPin* mpVideoPin;
    CGeneralOutputPin* mpCurPin;

    CGeneralBufferPool* mpVideoBufferPool;
    CGeneralBufferPool* mpAudioBufferPool;
    CGeneralBufferPool* mpCurBufferPool;

    AM_INT mSpecify; //read data from specify queue, not used.

    IGDemuxer* mpDemuxer[MDEC_SOURCE_MAX_NUM];
};
#endif
