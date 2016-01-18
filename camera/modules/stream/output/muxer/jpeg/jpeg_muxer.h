/*
 * jpeg_muxer.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 11/12/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __JPEG_MUXER_H__
#define __JPEG_MUXER_H__

class CJpegMuxerInput;

class CJpegMuxer: public CPacketActiveFilter, public IMediaMuxer
{
   typedef CPacketActiveFilter inherited;
   friend class CJpegMuxerInput;

public:
   static CJpegMuxer *Create (IEngine *engine,
#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
                            bool RTPriority = true,
#else
                            bool RTPriority = false,
#endif
                            int priority = CThread::PRIO_LOW);

protected:
   CJpegMuxer (IEngine *);
   AM_ERR Construct (bool RTPriority, int priority);
   virtual ~CJpegMuxer ();

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
   virtual AM_ERR OnEOS (CPacket *);
   virtual AM_ERR OnAVInfo (CPacket *);
   virtual AM_ERR OnAVBuffer (CPacket *);

private:
   bool mbRun;
   AM_UINT mEOSMap;
   AM_UINT mMaxFileAmount;

   CJpegMuxerInput *mpMediaInput;
   IJpegDataWriter *mpDataWriter;
   AM_UINT mVideoFrameCount;
};

class CJpegMuxerInput: public CPacketQueueInputPin
{
   typedef CPacketQueueInputPin inherited;
   friend class CJpegMuxer;

public:
   static CJpegMuxerInput *Create (CPacketFilter *);

private:
   CJpegMuxerInput (CPacketFilter *pFilter) :
      inherited (pFilter) {}
   AM_ERR Construct ();
   virtual ~CJpegMuxerInput () {}
};

#endif
