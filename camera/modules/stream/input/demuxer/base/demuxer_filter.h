/*******************************************************************************
 * demuxer_filter.h
 *
 * History:
 *   2013-3-5 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef DEMUXER_BASE_H_
#define DEMUXER_BASE_H_

#include "config.h"

class CPlugin;
struct DemuxerObject {
    CPlugin *plugin;
    char    *library;
    DemuxerNew getDemuxerObj;
    AmDemuxerType type;
    DemuxerObject(const char* file);
    virtual ~DemuxerObject();
    bool open();
    void close();
};

typedef std::queue<DemuxerObject*> DemuxerList;

class CDemuxerOutput;
class CDemuxerFilter: public CPacketActiveFilter, public IDemuxer
{
    typedef CPacketActiveFilter inherited;

  public:
    static CDemuxerFilter* Create(IEngine *pEngine,
#ifdef BUILD_AMBARELLA_CAMEREA_ENABLE_RT
                            bool RTPriority = true,
#else
                            bool RTPriority = false,
#endif
                            int prority = CThread::PRIO_LOW);

  public:
    virtual void* GetInterface(AM_REFIID refiid) {
      if (AM_LIKELY(refiid == IID_IDemuxer)) {
        return (IDemuxer*)this;
      }
      return inherited::GetInterface(refiid);
    }
    virtual void Delete() {
      inherited::Delete();
    }
    virtual void GetInfo(INFO& info)
    {
      info.nInput = 0;
      info.nOutput = 1;
      info.pName = "UniversalDemuxer";
    }
    virtual IPacketPin* GetOutputPin(AM_UINT index)
    {
      return (IPacketPin*)mOutPin;
    }
    virtual AM_ERR AddMedia(const char* uri);
    virtual AM_ERR Play(const char* uri = NULL);
    virtual AM_ERR Start();
    virtual AM_ERR Stop();

  private:
    virtual void OnRun();
    inline void SendBuffer(CPacket *packet);
    AmDemuxerType CheckMediaType(const char* uri);
    DemuxerList* LoadCodecs();
    inline IDemuxerCodec* GetDemuxerObject(AmDemuxerType type,
                                           AM_UINT streamid);

  private:
    CDemuxerFilter(IEngine *engine);
    virtual ~CDemuxerFilter();
    AM_ERR Construct(bool RTPriority, int priority);

  private:
    CMutex            *mMutexPause;
    CMutex            *mMutexDemuxer;
    CEvent            *mEvent;
    CDemuxerOutput    *mOutPin;
    CSimplePacketPool *mPktPool;
    DemuxerList       *mDemuxerList;
    IDemuxerCodec     *mDemuxer;
    bool               mRun;
    bool               mPaused;
    bool               mStarted;
};

/*
 * CDemuxerOutput
 */
class CDemuxerOutput: public CPacketOutputPin
{
    typedef CPacketOutputPin inherited;
    friend class CDemuxerFilter;

  public:
    static CDemuxerOutput* Create(CPacketFilter *filter)
    {
      CDemuxerOutput *ret = new CDemuxerOutput(filter);
      if (AM_UNLIKELY(ret && (ret->Construct() != ME_OK))) {
        delete ret;
        ret = NULL;
      }

      return ret;
    }

  private:
    CDemuxerOutput(CPacketFilter *filter) :
      inherited(filter){}
    virtual ~CDemuxerOutput() {}
    AM_ERR Construct() {return ME_OK;}
};

#endif /* DEMUXER_BASE_H_ */
