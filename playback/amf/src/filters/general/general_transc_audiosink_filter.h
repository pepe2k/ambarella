/*
 * general_transc_audiosink_filter.h
 *
 * History:
 *    2013/11/12 - [GLiu] create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
 #ifndef __GENERAL_TRANSCAUDIOSINK__FILTER_H__
#define __GENERAL_TRANSCAUDIOSINK__FILTER_H__

class CGeneralTranscoder;
class CGeneralTransAudioSink: public CInterActiveFilter
{
    typedef CInterActiveFilter inherited;

public:
    static CGeneralTransAudioSink* Create(IEngine *pEngine, CGConfig* pConfig);

public:
    //My Api
    void Dump();
    AM_ERR QueryInfo(AM_INT target, AM_INT type, CParam64& param);
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
    CGeneralTransAudioSink(IEngine *pEngine, CGConfig* pConfig):
        inherited(pEngine, "CGeneralTransAudioSink"),
        mpConfig(pConfig),
        mpBuffer(NULL),
        mDecNum(0),
        mpBufferPool(NULL),
        mpInputPin(NULL),
        mpOutputPin(NULL),
        mpTranscoderContext(NULL)
    {
        mbRun = false;
        AM_INT i = 0;
        for(; i < MDEC_SOURCE_MAX_NUM; i++){
            mpInputQ[i] = NULL;
            mpInputQOwner[i] = NULL;
        }
        //debug
        mAudioGDE = false;
        mHDDecoder = MDEC_SOURCE_MAX_NUM -1;
    }

    AM_ERR Construct();
    AM_ERR ConstructPin();
    AM_ERR ConstructInputPin();
    virtual ~CGeneralTransAudioSink();
private:
    //todo change send or post
    AM_ERR HandCmd(AM_INT target, CMD& cmd, AM_BOOL isSend);
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
    AM_ERR ReadInputData(CQueue::WaitResult& result);
    AM_ERR ProcessBuffer();
    AM_ERR SendGBufferQueue(AM_INT deIndex);
    AM_ERR SendGBuffer(CGBuffer* oBuffer);
    AM_ERR ProcessAudioBuffer();
    AM_ERR DiscardAudioBuffer();

    inline AM_INT DataFromDataQ(CQueue* pQ){
        AM_INT index = -1;
        for(AM_INT i=0; i<MDEC_SOURCE_MAX_NUM; i++){
            if(mpInputQ[i] == pQ){
                if(mpInputQ[i] == NULL) break;
                else
                    return i;
            }
        }
        return index;
    }

public:
    AM_ERR SetTranscoderContext(CGeneralTranscoder* p){
        mpTranscoderContext = p;
        return ME_OK;
    }

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

    CQueue* mpInputQ[MDEC_SOURCE_MAX_NUM];
    void* mpInputQOwner[MDEC_SOURCE_MAX_NUM];
    CGeneralTranscoder* mpTranscoderContext;
};

#endif //__GENERAL_TRANSCAUDIOSINK__FILTER_H__

