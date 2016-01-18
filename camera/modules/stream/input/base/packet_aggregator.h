/*******************************************************************************
 * packet_aggregator.h
 *
 * @Author: Hanbo Xiao
 * @Time  : 10/09/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ******************************************************************************/
#ifndef __PACKET_AGGREGATOR_H__
#define __PACKET_AGGREGATOR_H__

#include "config.h"

class CPacketAggregatorInput;
class CPacketAggregatorOutput;

class CPacketAggregator: public CPacketActiveFilter, public IPacketAggregator
{
    typedef CPacketActiveFilter inherited;
    friend class CPacketAggregatorInput;

  public:
    static CPacketAggregator *Create (IEngine *pEngine,
#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
                                      bool RTPriority = true,
#else
                                      bool RTPriority = false,
#endif
                                      int priority = CThread::PRIO_NORMAL);

  private:
    CPacketAggregator (IEngine *engine);
    AM_ERR Construct (bool RTPriority, int priority);
    virtual ~CPacketAggregator ();

  public:
    virtual AM_ERR SetStreamNumber(AM_UINT streamNum);
    virtual AM_ERR SetInputNumber(AM_UINT inputNum);

  public:
    /* Interfaces declared at IInterface. */
    virtual void *GetInterface (AM_REFIID);
    virtual void Delete ();

  /* Interfaces declared at IFilter. */
  virtual void GetInfo (INFO &);
  virtual IPacketPin *GetOutputPin (AM_UINT);
  virtual IPacketPin *GetInputPin (AM_UINT);
  virtual AM_ERR Stop ();
  virtual void OnRun ();

  private:
    inline void ProcessBuffer (CPacket *);

  private:
    bool                     mRun;
    int                      mEOSNum;
    int                      mInputNum;
    int                      mStreamNum;
    char                   **mInputName;
    CMutex                  *mpMutex;
    CEvent                  *mpEvent;
    CPacketAggregatorInput **mppMediaInput;
    CPacketAggregatorOutput *mpMediaOutput;
    CPacketPool             *mpMediaPacketPool;
};

class CPacketAggregatorInput: public CPacketActiveInputPin
{
    typedef CPacketActiveInputPin inherited;
    friend class CPacketAggregator;

  public:
    static CPacketAggregatorInput *Create (CPacketFilter *, const char *);

  private:
    CPacketAggregatorInput (CPacketFilter *pFilter, const char *pName):
      inherited (pFilter, pName) {}
    AM_ERR Construct ();
    virtual AM_ERR ProcessBuffer (CPacket *pBuffer);
    virtual void Delete () { inherited::Delete (); }
    virtual ~CPacketAggregatorInput () {}
};

class CPacketAggregatorOutput: public CPacketOutputPin
{
    typedef CPacketOutputPin inherited;
    friend class CPacketAggregator;

  public:
    static CPacketAggregatorOutput *Create (CPacketFilter *pFilter)
    {
      return new CPacketAggregatorOutput (pFilter);
    }
    virtual void Delete () { inherited::Delete (); }
    virtual ~CPacketAggregatorOutput () {}

  private:
    CPacketAggregatorOutput (CPacketFilter *pFilter):
      inherited (pFilter) {}
};

#endif
