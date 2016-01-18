/*
 * raw_muxer.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 05/20/2013 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __RAW_MUXER_H__
#define __RAW_MUXER_H__

class CRawMuxerInput;

class CRawMuxer: public CPacketActiveFilter, public IMediaMuxer
{
   typedef CPacketActiveFilter inherited;
   friend class CRawMuxerInput;

public:
   static CRawMuxer *Create (IEngine *engine,
#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
                            bool RTPriority = true,
#else
                            bool RTPriority = false,
#endif
                            int priority = CThread::PRIO_LOW);

protected:
   CRawMuxer (IEngine *);
   AM_ERR Construct (bool RTPriority, int priority);
   virtual ~CRawMuxer ();

public:
   /* Interfaces declared at IInterface */
   virtual void *GetInterface (AM_REFIID);
   virtual void Delete () { inherited::Delete (); }

   /* Interfaces declared at IFilter */
   virtual AM_ERR Stop ();
   virtual void GetInfo (INFO &);
   virtual IPacketPin *GetInputPin (AM_UINT);

   /* Interfaces declared at IMediaMuxer */
   virtual AM_ERR SetMediaSink (AmSinkType, const char *);
   virtual AM_ERR SetMediaSourceMask (AM_UINT mediaSourceMask);
   virtual AM_ERR SetSplitDuration (AM_U64 durationIn90KBase = 0);
   virtual AM_ERR SetMaxFileAmount (AM_UINT fileAmount);

   /* Interfaces declared at IActiveObject */
   virtual void OnRun ();

private:
   AM_ERR OnAVInfo (CPacket *packet);
   AM_ERR OnAVBuffer (CPacket *packet);
   AM_ERR OnEOS (CPacket *packet);

private:
   bool                  mbRun;
   AM_UINT               mVideoTotalEosMap;
   AM_UINT               mVideoReachEosMap;
   AM_UINT               mAudioTotalEosMap;
   AM_UINT               mAudioReachEosMap;
   AmTransferPacketType  mAudioType;
   AmTransferPacketType  mVideoType[AM_STREAM_NUMBER_MAX];
   CRawMuxerInput       *mpMediaInput;
   AmTransferPacket     *mpPacket;
   AmTransferServer     *mpTransferServer;
   unsigned long long    mAudioIndex;
   unsigned long long    mVideoIndex;
};

class CRawMuxerInput: public CPacketQueueInputPin
{
   typedef CPacketQueueInputPin inherited;
   friend class CRawMuxer;

public:
   static CRawMuxerInput *Create (CPacketFilter *);

private:
   CRawMuxerInput (CPacketFilter *pFilter) :
      inherited (pFilter) {}
   AM_ERR Construct ();
   virtual ~CRawMuxerInput () {}
};

#endif
