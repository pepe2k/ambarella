/*******************************************************************************
 * event_sender.h
 *
 * History:
 *    2012/11/20 - [Dongge Wu] create file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ******************************************************************************/

#ifndef __EVENT_SENDER_H__
#define __EVENT_SENDER_H__

class CEventSenderOutput;

//-----------------------------------------------------------------------
//
// CEventSender
//
//-----------------------------------------------------------------------
class CEventSender: public CPacketFilter, public IEventSender
{
    typedef CPacketFilter inherited;

  public:
    static IPacketFilter* Create(IEngine *pEngine);

  private:
    CEventSender(IEngine *pEngine):
      inherited(pEngine),
      mRun(false),
      mpOutPin(NULL),
      mpBufferPool(NULL)	,
      mpMutex(NULL),
      mHwTimerFd(-1),
      mLastPts(0LLU)
  {}
    AM_ERR Construct();
    virtual ~CEventSender();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
  {
    if (refiid == IID_IEventSender)
      return (IEventSender*)this;
    return inherited::GetInterface(refiid);
  }
  virtual void Delete() { inherited::Delete(); }

  // IPacketFilter
  virtual AM_ERR Run();
  virtual AM_ERR Start();
  virtual AM_ERR Stop();
  virtual void GetInfo(INFO& info);
  virtual IPacketPin* GetOutputPin(AM_UINT index);
  virtual AM_PTS GetCurrentPts();

  // IEventSender
  virtual AM_ERR SendEvent(CPacket::AmPayloadAttr eventType);

  private:
  enum {
    BUFFER_POOL_SIZE = 64,
    MAX_EVENT_NALU_SIZE = 64,
  };

  bool mRun;
  CEventSenderOutput *mpOutPin;
  CFixedPacketPool *mpBufferPool;
  CMutex *mpMutex;
  int mHwTimerFd;
  AM_PTS mLastPts;
};

//-----------------------------------------------------------------------
//
// CEventSenderOutput
//
//-----------------------------------------------------------------------
class CEventSenderOutput: public CPacketOutputPin
{
    typedef CPacketOutputPin inherited;
    friend class CEventSender;

  public:
    static CEventSenderOutput* Create(CPacketFilter *pFilter);

  private:
    CEventSenderOutput(CPacketFilter *pFilter):
      inherited(pFilter)
  {
  }
    AM_ERR Construct();
    virtual ~CEventSenderOutput() {}
};

#endif

