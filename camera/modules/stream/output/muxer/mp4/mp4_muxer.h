/*
 * mp4_muxer.h
 *
 * History:
 *	2009/5/26 - [Jacky Zhang] created file
 *	2011/2/23 - [ Yi Zhu] modified
 *	2011/9/13 - [ Jay Zhang] modified
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __MP4_MUXER_H__
#define __MP4_MUXER_H__

class CMp4MuxerInput;
#ifndef MAX_COUNT_BUFFER
#define MAX_COUNT_BUFFER 16
#endif

class CMp4Muxer: public CPacketActiveFilter, public IMediaMuxer
{
    typedef CPacketActiveFilter inherited;
    friend class CMp4MuxerInput;

  public:
    static CMp4Muxer *Create(IEngine *pEngine,
#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
                             bool RTPriority = true,
#else
                             bool RTPriority = false,
#endif
                             int priority = CThread::PRIO_LOW
                             );

  private:
    CMp4Muxer(IEngine *pEngine);
    AM_ERR Construct(bool RTPriority, int priority);
    virtual ~CMp4Muxer();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete() { inherited::Delete(); }

  // IFilter
  virtual AM_ERR Stop();
  virtual void GetInfo(INFO& info);
  virtual IPacketPin* GetInputPin(AM_UINT index);

  /* Interfaces declared at IMediaMuxer */
  virtual AM_ERR SetMediaSink (AmSinkType, const char *);
  virtual AM_ERR SetMediaSourceMask (AM_UINT mediaSourceMask);
  virtual AM_ERR SetSplitDuration (AM_U64 durationIn90KBase = 0);
  virtual AM_ERR SetMaxFileAmount (AM_UINT maxFileAmount) { return ME_OK; }

  private:
  // IActiveObject
  virtual void OnRun();

  private:
  AM_ERR OnInfo(CPacket *packet);
  AM_ERR OnAVData(CPacket *packet);
  AM_ERR OnData(CPacket *packet);
  AM_ERR OnEventData (CPacket *packet);
  AM_ERR OnEOF(CPacket *packet);
  AM_ERR OnEvent(CPacket *packet);
  AM_ERR OnEOS(CPacket *packet);
  AM_ERR AddBuffer(CPacket *packet);

  private:
  bool mbRun;
  bool mbAudioEnable;
  bool mbIsFirstAudio;
  bool mbIsFirstVideo;
  bool mbNeedSplitted;

  AM_UINT mEOSMap;
  AM_UINT mEOFMap;
  AM_UINT mEventMap;
  AM_UINT mEventNormalSync;
  char   *mpFileName;
  AM_UINT mFileCount;

  AM_U64 mSplittedDuration;
  AM_U64 mNextFileBoundary;

  CMp4Builder    *mpMP4Builder;
  CMp4MuxerInput *mpMediaInput;
  IMp4DataWriter *mpDataWriter;

  CPacket        *mBufferdUsrSEI;
  CPacket        *mBuffer[MAX_COUNT_BUFFER];
  AM_INT        mCountBuffer;

  AM_PTS          mLastVideoPTS;
  AM_UINT         mFileVideoFrameCount;
  AM_UINT         mVideoFrameCount;
};

//-----------------------------------------------------------------------
//
// CMp4MuxerInput
//
//-----------------------------------------------------------------------
class CMp4MuxerInput: public CPacketQueueInputPin
{
    typedef CPacketQueueInputPin inherited;
    friend class CMp4Muxer;

  public:
    static CMp4MuxerInput* Create(CPacketFilter *pFilter);

  private:
    CMp4MuxerInput(CPacketFilter *pFilter):
      inherited(pFilter)
  {}
    AM_ERR Construct();
    virtual ~CMp4MuxerInput(){}
};

#endif //__MP4_MUXER_H__
