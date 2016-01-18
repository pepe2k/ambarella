/*
 * general_muxer_save.h
 *
 * History:
 *    2012/6/15 - [QingXiong Z] create file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __GENERAL_MUXER_SAVE_H__
#define __GENERAL_MUXER_SAVE_H__


class CGeneralMuxer : public CObject
{
    enum MuxerState
    {
        STATE_NULL,
        STATE_INITED,
        STATE_FEED,
        STATE_ERROR,
    };
    enum InjectorState
    {
        STATE_NULL_INJECTOR,
        STATE_INITED_INJECTOR,
        STATE_FEED_INJECTOR,
        STATE_ERROR_INJECTOR,
    };
public:
    static CGeneralMuxer* Create(CGConfig* pConfig);

public:
    virtual void* GetInterface(AM_REFIID refiid)
    {
        return CObject::GetInterface(refiid);
    }
    virtual void Delete();

    AM_ERR ConstructMuxer(AM_INT index);
    AM_ERR ConfigMuxer(AM_INT index);
    AM_ERR UpdateConfig(AM_INT index);
    AM_ERR FinishMuxer(AM_INT index);
    AM_ERR TerminateAll();
    AM_ERR NotifyFromMuxer(AM_INT index, AM_MSG& msg);
    AM_ERR FeedData(AM_INT index, CGBuffer* buffer);
    AM_ERR QueryInfo(AM_INT index, AM_INT type, CParam& par);
    AM_ERR Dump();

    AM_ERR AutoMuxer(AM_INT index, CGBuffer* buffer);

    AM_ERR SetSavingTimeDuration(AM_UINT duration);

    AM_ERR processMDMsg(AM_INT index, AM_INT event);

#if PLATFORM_LINUX
    AM_ERR SendToInjector(AM_INT index, CGBuffer* buffer,int64_t pts,AM_INT is_ds_live = 1);
private:
    AM_ERR ConstructInjector(AM_INT index);
    AM_ERR ConfigInjector(AM_INT index,AM_INT is_ds_live);
    AM_ERR SendDataInjector(AM_INT index, CGBuffer* buffer,int64_t pts);
#endif

private:
    CGeneralMuxer(CGConfig* pConfig);
    AM_ERR Construct();
    ~CGeneralMuxer();

private:
    MuxerState mState[MDEC_SOURCE_MAX_NUM];
    IGMuxer* mpMuxer[MDEC_SOURCE_MAX_NUM];

    InjectorState mSendState[MDEC_SOURCE_MAX_NUM];
    IGInjector* mpInjector[MDEC_SOURCE_MAX_NUM];

    CGConfig* mpGConfig;
    CGMuxerConfig* mpConfig;

//auto separate file
public:
    AM_UINT mDuration;//every file duration(s)
    AM_UINT mMaxFileCount;
};
#endif
