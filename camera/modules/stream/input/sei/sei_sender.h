/*******************************************************************************
 * sei_sender.h
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

#ifndef __SEI_SENDER_H__
#define __SEI_SENDER_H__

class CSeiSenderOutput;

//-----------------------------------------------------------------------
//
// CSeiSender
//
//-----------------------------------------------------------------------
class CSeiSender: public CPacketFilter, public ISeiSender
{
    typedef CPacketFilter inherited;

  public:
    static IPacketFilter* Create(IEngine *pEngine);

  private:
    CSeiSender(IEngine *pEngine):
      inherited(pEngine),
      mRun(false),
      mpOutPin(NULL),
      mpBufferPool(NULL)	,
      mpMutex(NULL),
      mHwTimerFd(-1),
      mLastPts(0LLU)
  {}
    AM_ERR Construct();
    virtual ~CSeiSender();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
  {
    if (refiid == IID_ISeiSender)
      return (ISeiSender*)this;
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

  // ISeiSender
  virtual AM_ERR SendSEI(void *pUserData, AM_U32 len);

  private:
  enum {
    UUID_SIZE = 16,
    BUFFER_POOL_SIZE = 64,
    MAX_SEI_NALU_SIZE = 256,
  };

  bool mRun;
  CSeiSenderOutput *mpOutPin;
  CFixedPacketPool *mpBufferPool;
  CMutex *mpMutex;
  int mHwTimerFd;
  AM_PTS mLastPts;
};

//-----------------------------------------------------------------------
//
// CSeiSenderOutput
//
//-----------------------------------------------------------------------
class CSeiSenderOutput: public CPacketOutputPin
{
    typedef CPacketOutputPin inherited;
    friend class CSeiSender;

  public:
    static CSeiSenderOutput* Create(CPacketFilter *pFilter);

  private:
    CSeiSenderOutput(CPacketFilter *pFilter):
      inherited(pFilter)
  {
  }
    AM_ERR Construct();
    virtual ~CSeiSenderOutput() {}
};

#endif
