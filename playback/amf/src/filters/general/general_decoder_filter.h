/*
 * general_decode_filter.h
 *
 * History:
 *    2010/6/1 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
 #ifndef __GENERAL_DECODE_FILTER_H__
#define __GENERAL_DECODE_FILTER_H__

#define GENERAL_DECODER_CTRL_BP 16

class CGeneralDecoder: public CInterActiveFilter
{
    typedef CInterActiveFilter inherited;

public:
    static CGeneralDecoder* Create(IEngine *pEngine, CGConfig* pConfig);

public:
    //My Api
    AM_ERR ReSet();
    void Dump();
    AM_ERR QueryInfo(AM_INT target, AM_INT type, CParam64& param);
    AM_ERR TryAgain(AM_INT target); // error handle
    AM_ERR UpdatePBSpeed(AM_INT target, AM_U8 pb_speed, AM_U8 pb_speed_frac, IParameters::DecoderFeedingRule feeding_rule);

    //CInterActiveFilter
    virtual IPin* GetInputPin(AM_UINT index);
    virtual IPin* GetOutputPin(AM_UINT index);
    virtual AM_ERR SetInputFormat(CMediaFormat* pFormat);
    virtual AM_ERR SetOutputFormat(CMediaFormat* pFormat);
    virtual AM_ERR ReleaseBuffer(CBuffer* buffer);
    virtual void Delete();
    virtual void GetInfo(INFO& info);
private:
    CGeneralDecoder(IEngine *pEngine, CGConfig* pConfig):
        inherited(pEngine, "CGeneralDecoder"),
        mpConfig(pConfig),
        mpBuffer(NULL),
        mDecNum(0),
        mpBufferPool(NULL),
        mpInputPin(NULL),
        mpOutputPin(NULL)
    {
        mbRun = false;
        AM_INT i = 0;
        for(; i < MDEC_SOURCE_MAX_NUM; i++)
            mpDecoder[i] = NULL;
        //debug
        mAudioGDE = false;
        mHDDecoder = MDEC_SOURCE_MAX_NUM -1;
    }

    AM_ERR Construct();
    AM_ERR ConstructPin();
    AM_ERR ConstructInputPin();
    virtual ~CGeneralDecoder();
private:
    //todo change send or post
    AM_ERR HandDecoderCmd(AM_INT target, CMD& cmd, AM_BOOL isSend);
    AM_ERR DoPause(AM_INT target, AM_U8 flag);
    AM_ERR DoResume(AM_INT target, AM_U8 flag);
    AM_ERR DoFlush(AM_INT target, AM_U8 flag);
    AM_ERR DoConfig(AM_INT target, AM_U8 flag);
    AM_ERR DoStop();
    AM_ERR DoRemove(AM_UINT index, AM_U8 flag);

    virtual void MsgProc(AM_MSG& msg);
    virtual void OnRun();
    virtual bool ProcessCmd(CMD& cmd);
    AM_ERR ProcessEOS();

    //AM_ERR RemoveDecoder();
    AM_ERR IsDecoderReady();
    AM_ERR ReadInputData(CQueue::WaitResult& result);
    AM_ERR ProcessBuffer();
    AM_ERR CreateDecoder();
    AM_ERR ChangeDecoder();
    AM_ERR SendGBufferQueue(AM_INT deIndex);
    AM_ERR SendGBuffer(CGBuffer* oBuffer);

private:
    CGConfig* mpConfig;
    CGBuffer* mpBuffer;

    AM_INT mHDDecoder;

    AM_INT mCurIndex;
    AM_INT mDecNum;
    bool mAudioGDE; //DEBUG

    CGeneralBufferPool* mpBufferPool;
    CGeneralInputPin* mpInputPin;
    CGeneralOutputPin* mpOutputPin;

    IGDecoder* mpDecoder[MDEC_SOURCE_MAX_NUM];
};
#endif
