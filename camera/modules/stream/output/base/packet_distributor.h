/*
 * packet_distributor.h
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
 */
#ifndef __PACKET_DISTRIBUTOR_H__
#define __PACKET_DISTRIBUTOR_H__

#include "config.h"

class CPacketDistributorInput;
class CPacketDistributorOutput;

class CPacketDistributor: public CPacketActiveFilter
{
   typedef CPacketActiveFilter inherited;
   friend class CPacketDistributorInput;

public:
   static CPacketDistributor *Create (IEngine *engine, AM_UINT muxerAmount,
#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
                                bool RTPriority = true,
#else
                                bool RTPriority = false,
#endif
                                int priority = CThread::PRIO_LOW);

   AM_ERR SetSourceMask (AM_UINT, AM_UINT);

private:
   CPacketDistributor (IEngine *, AM_UINT);
   AM_ERR Construct (bool RTPriority, int priority);
   virtual ~CPacketDistributor ();

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
   bool mRun;
   AM_UINT mMuxerNum;
   AM_UINT mEosNum;
   AM_UINT *mSourceMask;

   CPacketDistributorInput *mpMediaInput;
   CPacketDistributorOutput **mppMediaOutput;
   CPacketPool **mppMediaPacketPool;
   AM_U8 **mppPacketPoolName;
};

class CPacketDistributorInput: public CPacketQueueInputPin
{
   typedef CPacketQueueInputPin inherited;
   friend class CPacketDistributor;

public:
   static CPacketDistributorInput *Create (CPacketFilter *);

private:
   CPacketDistributorInput (CPacketFilter *pFilter):
      inherited (pFilter) {}
   AM_ERR Construct ();
   virtual ~CPacketDistributorInput () {}
};

class CPacketDistributorOutput: public CPacketOutputPin
{
   typedef CPacketOutputPin inherited;
   friend class CPacketDistributor;

public:
   static CPacketDistributorOutput *Create (CPacketFilter *);

private:
   CPacketDistributorOutput (CPacketFilter *pFilter):
      inherited (pFilter) {}
   AM_ERR Construct () { return ME_OK; }
   virtual ~CPacketDistributorOutput () {}
};

#endif
