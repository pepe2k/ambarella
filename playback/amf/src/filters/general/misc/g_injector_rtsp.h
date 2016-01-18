/*
 * g_injector_rtsp.h
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __G_INJECTOR_RTSP_H__
#define __G_INJECTOR_RTSP_H__

class RtspMediaSession;
class CGInjectorRtsp : public IGInjector, public CObject
{
    typedef CObject inherited;
    typedef IActiveObject AO;
public:
    static IGInjector* Create(CGeneralMuxer* manager);

public:
    void* GetInterface(AM_REFIID refiid)
    {
        return inherited::GetInterface(refiid);
    }
    virtual void Delete();

    virtual AM_ERR ConfigMe(CUintMuxerConfig* con);
    virtual AM_ERR UpdateConfig(CUintMuxerConfig* con);
    virtual AM_ERR PerformCmd(AO::CMD& cmd, AM_BOOL isSend);
    virtual AM_ERR FeedData(CGBuffer* buffer,int64_t pts,int64_t dts);
    virtual AM_ERR FinishInjector();
    virtual AM_ERR QueryInfo(AM_INT type, CParam& par);
    virtual AM_ERR Dump();

private:
    CGInjectorRtsp(CGeneralMuxer* manager);
    AM_ERR Construct();
    ~CGInjectorRtsp();
private:
    AM_ERR SetupInjectorEnv(AM_INT ds_type_live);

    AM_ERR DoStop();
    AM_ERR ProcessCmd(AO::CMD& cmd);

private:
    class SendInfo
    {
        public:
        SendInfo():
            audioSize(0),
            videoSize(0)
        {}
        AM_INT audioSize;
        AM_INT videoSize;
    };
private:
    CGeneralMuxer* mpManager;
    CUintMuxerConfig* mpConfig;

    RtspMediaSession* mpRtspInjector;

    SendInfo mInfo;
};
#endif
