/*******************************************************************************
 * playback_engine.h
 *
 * History:
 *   2013-4-8 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef PLAYBACK_ENGINE_H_
#define PLAYBACK_ENGINE_H_

class IInputPlayback;
class IOutputPlayback;

class CPlaybackEngine: public CPacketFilterGraph, public IPlaybackEngine
{
    typedef CPacketFilterGraph inherited;

  public:
    static CPlaybackEngine* Create();

  public:
    virtual void* GetInterface(AM_REFIID ref_iid)
    {
      if (AM_LIKELY(ref_iid == IID_IPlaybackEngine)) {
        return (IPlaybackEngine*)this;
      }
      return inherited::GetInterface(ref_iid);
    }

    virtual void Delete() { inherited::Delete(); }

    virtual AM_ERR PostEngineMsg(AM_MSG& msg)
    {
      return inherited::PostEngineMsg(msg);
    }

    virtual AM_ERR SetAppMsgSink(IMsgSink *pAppMsgSink)
    {
      return inherited::SetAppMsgSink(pAppMsgSink);
    }

  public:
    virtual PlaybackEngineStatus GetEngineStatus()
    {
      AUTO_LOCK(mMutex);
      return mState;
    }
    virtual bool CreateGraph();
    virtual bool AddUri(const char* uri);
    virtual bool Play(const char* uri = NULL);
    virtual bool Stop();
    virtual bool Pause(bool enable);
    virtual void SetAppMsgCallback(AmPlaybackEngineAppCb callback, void* data);

  private:
    bool ChangeEngineState(PlaybackEngineStatus targetState,
                           const char *uri = NULL);
    virtual AM_ERR SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&),
                                     void *context)
    {
      return inherited::SetAppMsgCallback(MsgProc, context);
    }
    virtual void MsgProc(AM_MSG& msg);
    virtual void AppProc(void* context, AM_MSG& msg);
    static void StaticAppProc(void* context, AM_MSG& msg)
    {
      ((CPlaybackEngine*)context)->AppProc(
          ((CPlaybackEngine*)context)->mAppData, msg);
    }

  private:
    CPlaybackEngine() :
      mEvent(NULL),
      mMutex(NULL),
      mFilterMutex(NULL),
      mModuleInput(NULL),
      mModuleOutput(NULL),
      mAppData(NULL),
      mAppCallback(NULL),
      mState(AM_PLAYBACK_ENGINE_STOPPED),
      mIsGraphCreated(false),
      mIsFilterRunning(false),
      mIsDeleted(false),
      mSignalCount(0){}
    virtual ~CPlaybackEngine();
    AM_ERR Construct();
    inline void StopPurgeAllFilters()
    {
      AUTO_LOCK(mFilterMutex);
      if (AM_LIKELY(mIsFilterRunning)) {
        StopAllFilters();
        PurgeAllFilters();
        mIsFilterRunning = false;
      }
    }


  private:
    CEvent               *mEvent;
    CMutex               *mMutex;
    CMutex               *mFilterMutex;
    IInputPlayback       *mModuleInput;
    IOutputPlayback      *mModuleOutput;
    void                 *mAppData;
    AmPlaybackEngineAppCb mAppCallback;
    PlaybackEngineStatus  mState;
    bool                  mIsGraphCreated;
    bool                  mIsFilterRunning;
    bool                  mIsDeleted;
    am_atomic_t           mSignalCount;
    AmEngineMsg           mAppMsg;
};

#endif /* PLAYBACK_ENGINE_H_ */
