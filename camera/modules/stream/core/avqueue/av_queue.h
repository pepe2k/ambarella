/*
 * av_queue.h
 *
 * History:
 *	2012/8/6 - [Shupeng Ren] created file
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AV_QUEUE_H__
#define __AV_QUEUE_H__

#include "config.h"

#define MUTEX_NUM 1

class CAVQueue;
class CAVQueueInput;
class CAVQueueOutput;
class CSimplePacketPool;

class CSimpleRingPool: public CSimplePacketPool
{
    typedef CSimplePacketPool inherited;

  public:
    static CSimpleRingPool* Create(const char *pName,
                                   AM_UINT     count,
                                   AM_UINT     dataSize,
                                   CAVQueue   *queue);

    virtual void Delete() {inherited::Delete();}
  public:
    //CPacketPool
    virtual void OnReleaseBuffer(CPacket *pPacket);

  protected:
    AM_ERR Construct(AM_UINT count, AM_UINT dataSize) {
      return inherited::Construct(count, dataSize);
    }
    CSimpleRingPool(const char *pName, CAVQueue *queue) :
      inherited(pName),
      mpQueue(queue){}
    virtual ~CSimpleRingPool(){}

  private:
    CAVQueue          *mpQueue;
};

class CAVQueue: public CPacketActiveFilter, public IAVQueue
{
    typedef CPacketActiveFilter inherited;
    friend class CAVQueueInput;

  public:
    static CAVQueue* Create(IEngine *pEngine,
#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
                            bool RTPriority = true,
#else
                            bool RTPriority = false,
#endif
                            int priority = CThread::PRIO_NORMAL);

    void ReleasePacket(CPacket *packet);
    virtual void Delete() { inherited::Delete(); }

  private:
    CAVQueue(IEngine *pEngine);
    AM_ERR Construct(bool RTPriority, int priority);
    virtual ~CAVQueue();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual bool IsReadyForEvent();
    virtual void SetEventStreamId(AM_UINT StreamId);
    virtual void SetEventDuration(AM_UINT history_duration, AM_UINT future_duration);

    // IFilter
    virtual AM_ERR Stop();
    virtual void GetInfo(INFO& info);
    virtual IPacketPin* GetInputPin(AM_UINT index);
    virtual IPacketPin* GetOutputPin(AM_UINT index);

  private:
    // IActiveObject
    virtual void OnRun();

  private:
    inline AM_ERR SendPacket(CPacket *pInPacket);
    inline AM_ERR OnInfo(CPacket *pInPacket);
    inline AM_ERR OnData(CPacket *pInPacet);
    inline AM_ERR OnEvent(CPacket *pInPacket);
    inline AM_ERR OnEOS(CPacket *pInPacket);
    inline AM_ERR SortAndSend();
    inline void SendEventPacket();
    inline AM_ERR ProcessBuffer(CPacket *pInPacket);
    static AM_ERR EventThread(void *p);
    void ProcessEvent();
    void InitVar();

  private:
    bool                 mStop;
    bool                 mIsVideoCome[MAX_ENCODE_STREAM_NUM];
    bool                 mIsAudioCome[MAX_ENCODE_STREAM_NUM];
    AM_INT               mIsVideoBlock;
    AM_INT               mIsAudioBlock;
    AM_INT               mIsVideoEventBlock;
    AM_INT               mIsAudioEventBlock;
    bool                 mVideoEOS[MAX_ENCODE_STREAM_NUM];
    bool                 mAudioEOS[MAX_ENCODE_STREAM_NUM];
    bool                 mVideoEventEOS;
    bool                 mIsInEvent;
    AM_PTS               mFirstVideoPTS;
    AM_PTS               mFirstAudioPTS;
    AM_PTS               mEventEndPTS;
    AM_U32               mEventSyncFlag;
    AM_UINT              mRun;
    AM_UINT              mRunRef;
    AM_UINT              mEOSFlag;
    AM_UINT              mEOSRef; //for not in queue EOS flag
    AM_UINT              mVideoStreamCount;
    AM_UINT              mAudioStreamCount;
    AM_UINT              mEventStreamId;
    AM_UINT              mEventHistoryDuration;
    AM_UINT              mEventFutureDuration;
    AM_UINT              mVideoBufferCount;
    AM_UINT              mAudioBufferCount;
    AM_UINT              mMaxVideoPacketSize;
    AM_UINT              mMaxAudioPacketSize;
    CMutex              *mpMutex[MUTEX_NUM];
    CEvent              *mVideoQEvt;
    CEvent              *mAudioQEvt;
    CEvent              *mVideoEventQEvt;
    CEvent              *mAudioEventQEvt;
    CEvent              *mEventWakeup;
    CThread             *mEventThread;
    CAVQueueInput       *mpStreamInput;
    CAVQueueOutput      *mpAVOutput;
    CSimpleRingPool     *mpAVPacketPool;
    CRingPacketPool     *mpVideoQueue[MAX_ENCODE_STREAM_NUM];
    CRingPacketPool     *mpAudioQueue[MAX_ENCODE_STREAM_NUM];
    AM_VIDEO_INFO       *mVideoInfo[MAX_ENCODE_STREAM_NUM];
    AM_AUDIO_INFO       *mAudioInfo[MAX_ENCODE_STREAM_NUM];
};

//-----------------------------------------------------------------------
//
// CAVQueueInput
//
//-----------------------------------------------------------------------
class CAVQueueInput: public CPacketActiveInputPin
{
    typedef CPacketActiveInputPin inherited;
    friend class CAVQueue;

  public:
    static CAVQueueInput* Create(CPacketFilter *pFilter, const char *pName);

  private:
    CAVQueueInput(CPacketFilter *pFilter, const char *pName):
      inherited(pFilter, pName)
    {}
    AM_ERR Construct();
    virtual AM_ERR ProcessBuffer(CPacket *pBuffer);
    virtual void Delete() {inherited::Delete();}
    virtual ~CAVQueueInput(){}
};

//-----------------------------------------------------------------------
//
// CAVQueueOutput
//
//-----------------------------------------------------------------------
class CAVQueueOutput: public CPacketOutputPin
{
    typedef CPacketOutputPin inherited;
    friend class CAVQueue;
  public:
    static CAVQueueOutput* Create(CPacketFilter *pFilter, bool isVideo)
    {
      CAVQueueOutput* result = new CAVQueueOutput(pFilter,isVideo);
      return result;
    }
    virtual void Delete() {inherited::Delete();}
    virtual ~CAVQueueOutput()
    {
    }

  public:
    virtual AM_ERR GetMediaFormat(CMediaFormat*& pFormat)
    {
      return ME_OK;
    }

  protected:
    CAVQueueOutput(CPacketFilter *pFilter, bool isVideo):
      inherited(pFilter),
      mIsVideo(isVideo) {}

  private:
    bool mIsVideo;
};
#endif //__AV_QUEUE_H__
