/*
 * general_renderer_filter.h
 *
 * History:
 *    2012/4/1 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __GENERAL_RENDERER_FILTER_H__
#define __GENERAL_RENDERER_FILTER_H__

#define RENDERER_INPUT_NUM 2

class CGeneralRenderer: public CInterActiveFilter
{
    typedef CInterActiveFilter inherited;

public:
    static CGeneralRenderer* Create(IEngine* pEngine, CGConfig* pConfig);
public:
    //My Api
    AM_ERR ReSet();
    void Dump();
    AM_ERR PlaybackZoom(AM_INT index, AM_U16 w, AM_U16 h, AM_U16 x, AM_U16 y);

    //CInterActiveFilter
    virtual IPin* GetInputPin(AM_UINT index);
    virtual AM_ERR SetInputFormat(CMediaFormat* pFormat);
    virtual AM_ERR SetOutputFormat(CMediaFormat* pFormat);
    virtual void Delete();
    virtual void GetInfo(INFO& info);

private:
    CGeneralRenderer(IEngine *pEngine, CGConfig* pConfig);
    AM_ERR Construct();
    AM_ERR ConstructInputPin(AM_INT index);
    virtual ~CGeneralRenderer();

private:
    virtual void MsgProc(AM_MSG& msg);
    AM_ERR HandRenderCmd(AM_INT target,  CMD& cmd, AM_BOOL isSend);
    virtual void OnRun();
    virtual bool ProcessCmd(CMD& cmd);
    CQueue::QType WaitPolicy(CMD& cmd, CQueue::WaitResult& result);
    AM_ERR DoPause(AM_INT target, AM_U8 flag);
    AM_ERR DoResume(AM_INT target, AM_U8 flag);
    AM_ERR DoFlush(AM_INT target, AM_U8 flag);
    AM_ERR DoConfig(AM_INT target, AM_U8 flag);
    AM_ERR DoStop();
    AM_ERR DoRemove(AM_INT index, AM_U8 flag);

private:
    AM_ERR StartAllRenderOut(AM_INT index);
    AM_ERR ReadInputData(CQueue::WaitResult& result);
    AM_ERR IsRenderReady();
    AM_ERR ProcessBuffer();
    AM_ERR CreateRenderer();
    AM_ERR ProcessEOS();

private:
    CGConfig* mpConfig;
    CGBuffer* mpBuffer;

    AM_INT mCurIndex;
    AM_INT mRenNum;

    AM_BOOL mbSyncDone;//all renders are sync done.
    //StatData mAData;
    //StatData mVData;
    CGeneralInputPin* mpInputPin[RENDERER_INPUT_NUM];
    IGRenderer* mpRenderer[MDEC_SOURCE_MAX_NUM];
};

#endif
