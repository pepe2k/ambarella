/*******************************************************************************
 * rtsp_filter.h
 *
 * History:
 *   2012-11-9 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef RTSP_FILTER_H_
#define RTSP_FILTER_H_

#include "config.h"

class CRtspServer;
class CRtspFilterInput;

class CRtspFilter: public CPacketActiveFilter,
                   public IMediaMuxer,
                   public IRtspFilter
{
    enum FilterExitState {
      AM_FILTER_EXIT_STATE_NORMAL,
      AM_FILTER_EXIT_STATE_ABORT
    };
    typedef CPacketActiveFilter inherited;
    friend class CRtspServer;
    friend class CRtspFilterInput;

  public:
    static CRtspFilter *Create(IEngine *engine,
#if defined(BUILD_AMBARELLA_CAMERA_ENABLE_RT) || \
    defined(BUILD_AMBARELLA_CAMERA_ENABLE_RTSP_RT)
                               bool RTPriority = true,
#else
                               bool RTPriority = false,
#endif
                               int priority = CThread::PRIO_LOW);

  public:
    virtual void SetRtspAttribute(bool needWait, bool needAuth);
    virtual void* GetInterface(AM_REFIID refiid);
    virtual void Delete()
    {
      inherited::Delete();
    }

    virtual void GetInfo(INFO& info);
    virtual IPacketPin* GetInputPin(AM_UINT index);
    virtual AM_ERR Stop();

    virtual AM_ERR SetMediaSink(AmSinkType sinkType, const char* destStr);
    virtual AM_ERR SetSplitDuration (AM_U64 durationIn90KBase = 0);
    virtual AM_ERR SetMediaSourceMask(AM_UINT mediaSourceMask);
    virtual AM_ERR SetMaxFileAmount (AM_UINT maxFileAmount) { return ME_OK; }

  private:
    CRtspFilter(IEngine* engine, bool RTPriority, int priority );
    AM_ERR Construct(bool RTPriority, int priority);
    virtual ~CRtspFilter();
    void Abort() {mIsServerAlive = false;}

  private:
    virtual void OnRun();
    inline AM_ERR OnInfo(CPacket* packet);
    inline AM_INT OnData(CPacket* packet);
    inline void OnEos(CPacket* packet);

  private:
    bool              mRun;
    bool              mIsServerAlive;
    bool              mRTPriority;
    int               mPriority;
    AM_UINT           mEosFlag;
    AM_UINT           mSourceMask;
    AM_UINT           mMinStream;
    FilterExitState   mExitState;
    CEvent           *mEvent;
    CRtspFilterInput *mMediaInput;
    CRtspServer      *mRtspServer;
};

class CRtspFilterInput: public CPacketQueueInputPin
{
    typedef CPacketQueueInputPin inherited;
    friend class CRtspFilter;

  public:
    static CRtspFilterInput* Create(CPacketFilter* filter);

  private:
    CRtspFilterInput(CPacketFilter* filter) :
      inherited(filter){}
    virtual ~CRtspFilterInput() {}
    AM_ERR Construct();
};

#endif /* RTSP_FILTER_H_ */
