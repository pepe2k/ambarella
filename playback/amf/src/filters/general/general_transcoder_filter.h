/*
 * general_transcoder_filter.h
 *
 * History:
 *    2013/7/21 - [GLiu] create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __GENERAL_TRANCODER_FILTER_H__
#define __GENERAL_TRANCODER_FILTER_H__

 #include <basetypes.h>
#include "iav_drv.h"
#include "ambas_common.h"
#include "iav_transcode_drv.h"
#include "iav_encode_drv.h"
class CGeneralTranscoder: public CInterActiveFilter
{
    typedef CInterActiveFilter inherited;

public:
    static CGeneralTranscoder* Create(IEngine* pEngine, CGConfig* pConfig);
public:
    //My Api
    AM_ERR ReSet();

    //IGTranscoder
    virtual AM_ERR IsReady(CGBuffer* pBuffer){return ME_NO_IMPL;};
    virtual AM_ERR SetEncParam(AM_INT w, AM_INT h, AM_INT br);
    virtual AM_ERR SetBitrate(AM_INT kbps);
    virtual AM_ERR SetFramerate(AM_INT fps, AM_INT reduction);
    virtual AM_ERR SetGOP(AM_INT M, AM_INT N, AM_INT interval, AM_INT structure);
    virtual AM_ERR DemandIDR(AM_BOOL now);
    virtual AM_ERR RecordToFile(AM_BOOL en, char* name);

    virtual AM_ERR UploadTranscode2Cloud(char* url, UPLOAD_NVR_TRANSOCDE_FLAG flag);
    virtual AM_ERR StopUploadTranscode2Cloud(UPLOAD_NVR_TRANSOCDE_FLAG flag);

    virtual AM_ERR PerformCmd(IActiveObject::CMD& cmd, AM_BOOL isSend){return ME_NO_IMPL;};
    virtual AM_ERR QueryInfo(AM_INT type, CParam& param);
    void Dump();

    //CInterActiveFilter
    virtual IPin* GetInputPin(AM_UINT index);
    virtual AM_ERR SetInputFormat(CMediaFormat* pFormat);
    virtual AM_ERR SetOutputFormat(CMediaFormat* pFormat);
    virtual void Delete();
    virtual void GetInfo(INFO& info);

public:
    void SetSavingTimeDuration(AM_UINT duration);
    AM_ERR ConfigLoader(AM_BOOL en, char* path, char* m3u8name, char* host, AM_INT count);

private:
    CGeneralTranscoder(IEngine *pEngine, CGConfig* pConfig);
    AM_ERR Construct();
    AM_ERR ConstructPin();
    AM_ERR ConstructInputPin(AM_INT index);
    virtual ~CGeneralTranscoder();

private:
    virtual void MsgProc(AM_MSG& msg);
    AM_ERR HandRenderCmd(AM_INT target,  CMD& cmd, AM_BOOL isSend);
    virtual void OnRun();
    virtual bool ProcessCmd(CMD& cmd);
    CQueue::QType WaitPolicy(CMD& cmd, CQueue::WaitResult& result);

    AM_ERR InitTranscoder();
    AM_ERR ReleaseTranscoder();
    AM_ERR StartTranscoder();
    AM_ERR StopTranscoder();
    AM_ERR ReadInputData();

    AM_INT GetSpsPpsLen(AM_U8 *pBuffer);

    AM_ERR DoPause(AM_INT target, AM_U8 flag);
    AM_ERR DoResume(AM_INT target, AM_U8 flag);
    AM_ERR DoFlush(AM_INT target, AM_U8 flag);
    AM_ERR DoConfig(AM_INT target, AM_U8 flag);
    AM_ERR DoStop();
    AM_ERR DoRemove(AM_INT index, AM_U8 flag);

private:
    AM_ERR StartAllRenderOut(AM_INT index);
    AM_ERR IsRenderReady();
    AM_ERR ProcessBuffer();
    AM_ERR DiscardAudioBuffer();
    AM_ERR CreateRenderer();
    AM_ERR ProcessEOS();
    AM_ERR FeedDataToMuxer(void* p_desc, AM_U64 pts);
    void UpdateConfig();

public:
    AM_ERR ProcessAudioBuffer();
private:
    CGConfig* mpConfig;
    CGBuffer* mpBuffer;
    CGBuffer* mpABuffer;
    IUDECHandler* mpDsp;

    CGeneralBufferPool* mpBufferPool;
    CGeneralInputPin* mpAudioInputPin;
    CGeneralOutputPin* mpFakeAudiooutputPin;
    //IGTranscoder* mpTranscoder;
    IGInjector* mprtspInjector;
    IGInjector* mprtspAInjector;
    IGInjector* mprtmpInjector;

    CUintMuxerConfig* mprtspInjectorConfig;
    CUintMuxerConfig* mprtspAInjectorConfig;
    CUintMuxerConfig* mprtmpInjectorConfig;
    AM_BOOL mbrtspInjectorConfiged;
    AM_BOOL mbrtspAInjectorConfiged;
    AM_BOOL mbrtmpInjectorConfiged;

    IGMuxer* mpMuxer;
    AM_BOOL mbMuxerConfiged;

    //iav_enc_info2_t mEncInfo;
    //iav_enc_config_t mEncConfig;
    AM_UINT mFrames;
    iav_udec_transcoder_bs_info_t mBitInfo;
    AM_BOOL mbTranscoderInited;
    FILE *pDumpFile;
    AM_BOOL mbDumpFile;

    AM_INT mEncWidth;
    AM_INT mEncHeight;
    AM_INT mBitrate;

    AM_U64 mLastPTS;
    AM_U64 mPTSLoop;

    //AM_UINT mFrameLifeTime;
};

#endif
