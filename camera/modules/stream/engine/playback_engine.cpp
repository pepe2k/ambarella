/*******************************************************************************
 * playback_engine.cpp
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
#include "am_include.h"

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_media_info.h"

#include "input_playback_if.h"

#include "audio_codec_info.h"

#include "output_playback_if.h"

#include "playback_engine_if.h"
#include "playback_engine.h"

extern IInputPlayback* create_playback_input(CPacketFilterGraph *graph,
                                             IEngine *engine);
extern IOutputPlayback* create_playback_output(CPacketFilterGraph *graph,
                                               IEngine *engine);

IPlaybackEngine* create_playback_engine()
{
  return CPlaybackEngine::Create();
}

CPlaybackEngine* CPlaybackEngine::Create()
{
  CPlaybackEngine* result = new CPlaybackEngine();
  if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }

  return result;
}

CPlaybackEngine::~CPlaybackEngine()
{
  mIsDeleted = true;
  StopPurgeAllFilters();
  DeleteAllConnections();
  mnFilters = 0;
  AM_DELETE(mModuleInput);
  AM_DELETE(mModuleOutput);
  AM_DELETE(mEvent);
  AM_DELETE(mFilterMutex);
  AM_DELETE(mMutex);
  DEBUG("~CPlaybackEngine");
}

AM_ERR CPlaybackEngine::Construct()
{
  AM_ERR ret = inherited::Construct();
  if (AM_LIKELY(ME_OK == ret)) {
    mModuleInput  = create_playback_input(this, (IEngine*)this);
    mModuleOutput = create_playback_output(this, (IEngine*)this);
    mEvent = CEvent::Create();
    mMutex = CMutex::Create(false);
    mFilterMutex = CMutex::Create(false);
    SetAppMsgCallback(StaticAppProc, this);
    if (AM_UNLIKELY(!mModuleInput)) {
      ERROR("Failed to create input module!");
    }
    if (AM_UNLIKELY(!mModuleOutput)) {
      ERROR("Failed to create output module!");
    }
    if (AM_UNLIKELY(!mEvent)) {
      ERROR("Failed to create event!");
    }
    if (AM_UNLIKELY(!mMutex)) {
      ERROR("Failed to create mutex!");
    }
    if (AM_UNLIKELY(!mFilterMutex)) {
      ERROR("Failed to create filter mutex!");
    }

    ret = (mModuleInput && mModuleOutput && mEvent && mMutex && mFilterMutex) ?
        ME_OK : ME_ERROR;
  }

  return ret;
}

bool CPlaybackEngine::CreateGraph()
{
  if (AM_LIKELY(!mIsGraphCreated && mModuleInput && mModuleOutput)) {
    if (AM_LIKELY(ME_OK == mModuleInput->CreateSubGraph())) {
      if (AM_LIKELY(ME_OK == mModuleOutput->CreateSubGraph())) {
        mIsGraphCreated = (ME_OK == CreateConnection(
            mModuleInput->GetModuleOutputPin(),
            mModuleOutput->GetModuleInputPin()));
        if (AM_UNLIKELY(!mIsGraphCreated)) {
          ERROR("Failed to connect input module to output module!");
        }
      } else {
        ERROR ("Failed to create output module sub-graph!");
      }
    } else {
      ERROR("Failed to create input module sub-graph!");
    }
  } else if (AM_UNLIKELY(!mModuleInput)) {
    ERROR("Invalid input module!");
  } else if (AM_UNLIKELY(!mModuleOutput)) {
    ERROR("Invalid output module!");
  }

  return mIsGraphCreated;
}

bool CPlaybackEngine::AddUri(const char* uri)
{
  AUTO_LOCK(mMutex);
  return (ME_OK == mModuleInput->AddUri(uri));
}

bool CPlaybackEngine::Play(const char* uri)
{
  return ((uri ? AddUri(uri) : true) ?
      ChangeEngineState(AM_PLAYBACK_ENGINE_PLAYING) : false);
}

bool CPlaybackEngine::Stop()
{
  return ChangeEngineState(AM_PLAYBACK_ENGINE_STOPPED);
}

bool CPlaybackEngine::Pause(bool enable)
{
  return (enable ? ChangeEngineState(AM_PLAYBACK_ENGINE_PAUSED) :
      ChangeEngineState(AM_PLAYBACK_ENGINE_PLAYING));
}

void CPlaybackEngine::SetAppMsgCallback(AmPlaybackEngineAppCb callback,
                                        void* data)
{
  mAppCallback = callback;
  mAppData     = data;
}

bool CPlaybackEngine::ChangeEngineState(PlaybackEngineStatus targetState,
                                        const char *uri)
{
  AUTO_LOCK(mMutex);
  bool ret = true;
  DEBUG("Target Status is %u", targetState);
  while (ret && (mState != targetState)) {
    switch(targetState) {
      case AM_PLAYBACK_ENGINE_PLAYING: {
        if (AM_LIKELY(mState == AM_PLAYBACK_ENGINE_STOPPED)) {
          mState = AM_PLAYBACK_ENGINE_STARTING;
          if (AM_LIKELY(!mIsFilterRunning)) {
            AUTO_LOCK(mFilterMutex);
            mIsFilterRunning = (ME_OK == RunAllFilters());
          }

          if (AM_LIKELY(mIsFilterRunning)) {
            ret = (ME_OK == mModuleInput->Play(uri));
            if (AM_LIKELY(ret)) {
              __atomic_inc(&mSignalCount);
              DEBUG("Wait count: %d", mSignalCount);
              mEvent->Wait();
              ret = mIsFilterRunning;
            } else {
              mState = AM_PLAYBACK_ENGINE_STOPPED;
            }
          } else {
            ERROR("Failed to run all filters!");
            mState = AM_PLAYBACK_ENGINE_STOPPED;
            ret = false;
          }
        } else if (mState == AM_PLAYBACK_ENGINE_PAUSED) {
          if (AM_UNLIKELY(uri)) {
            /* New file is added, need to stop first */
            mState = AM_PLAYBACK_ENGINE_STOPPING;
            ret = (ME_OK == mModuleOutput->Stop());
            if (AM_LIKELY(ret)) {
              __atomic_inc(&mSignalCount);
              DEBUG("Wait count: %d", mSignalCount);
              mEvent->Wait();
              ret = mIsFilterRunning;
            } else {
              mState = AM_PLAYBACK_ENGINE_PAUSED;
              ERROR("Failed to stop!");
            }
          } else {
            /* Need to resume */
            mState = AM_PLAYBACK_ENGINE_STARTING;
            ret =(ME_OK == mModuleOutput->Pause(false));
            if (AM_LIKELY(ret)) {
              __atomic_inc(&mSignalCount);
              DEBUG("Wait count: %d", mSignalCount);
              mEvent->Wait();
              ret = mIsFilterRunning;
            } else {
              mState = AM_PLAYBACK_ENGINE_PAUSED;
              ERROR("Failed to resume!");
            }
          }
        }
      }break;
      case AM_PLAYBACK_ENGINE_PAUSED: {
        if (AM_LIKELY(mState == AM_PLAYBACK_ENGINE_PLAYING)) {
          mState = AM_PLAYBACK_ENGINE_PAUSING;
          ret = (ME_OK == mModuleOutput->Pause(true));
          if (AM_LIKELY(ret)) {
            __atomic_inc(&mSignalCount);
            DEBUG("Wait count: %d", mSignalCount);
            mEvent->Wait();
            ret = mIsFilterRunning;
          } else {
            mState = AM_PLAYBACK_ENGINE_PLAYING;
            ERROR("Failed to pause!");
          }
        } else {
          WARN("Player is stopped!");
          ret = false;
        }
      }break;
      case AM_PLAYBACK_ENGINE_STOPPED: {
        ret = (ME_OK == mModuleOutput->Stop());
        if (AM_LIKELY(ret)) {
          mState = AM_PLAYBACK_ENGINE_STOPPING;
          __atomic_inc(&mSignalCount);
          DEBUG("Wait count: %d", mSignalCount);
          mEvent->Wait();
          ret = (mState == AM_PLAYBACK_ENGINE_STOPPED);
        } else {
          ERROR("Failed to stop!");
        }
      }break;
      case AM_PLAYBACK_ENGINE_STOPPING:
      default: {
        ret = false;
        ERROR("Invalid status!");
      }break;
    }
  }

  DEBUG("Target %u finished!", targetState);
  return ret;
}

void CPlaybackEngine::MsgProc(AM_MSG& msg)
{
  if (AM_LIKELY(IsSessionMsg(msg))) {
    switch(msg.code) {
      case MSG_OK: {
        if (AM_LIKELY(mState == AM_PLAYBACK_ENGINE_PAUSING)) {
          INFO("Playback paused successfully!");
          mState = AM_PLAYBACK_ENGINE_PAUSED;
          PostAppMsg(IPlaybackEngine::AM_PLAYBACK_ENGINE_MSG_PAUSE_OK);
          mEvent->Signal();
          __atomic_dec(&mSignalCount);
          DEBUG("Wait count: %d", mSignalCount);
        } else if (mState == AM_PLAYBACK_ENGINE_STARTING) {
          INFO("Playback started successfully!");
          mState = AM_PLAYBACK_ENGINE_PLAYING;
          PostAppMsg(IPlaybackEngine::AM_PLAYBACK_ENGINE_MSG_START_OK);
          mEvent->Signal();
          __atomic_dec(&mSignalCount);
          DEBUG("Wait count: %d", mSignalCount);
        } else if (mState == AM_PLAYBACK_ENGINE_PLAYING) {
          INFO("New file started!");
        }
      }break;
      case MSG_EOF: {
        mState = AM_PLAYBACK_ENGINE_STOPPED;
        PostAppMsg(IPlaybackEngine::AM_PLAYBACK_ENGINE_MSG_EOS);
      }break;
      case MSG_EOS:
      case MSG_ERROR:
      case MSG_ABORT: {
        AmPlaybackEngineMsg appMsg;
        if (AM_LIKELY(msg.code == MSG_ABORT)) {
          appMsg = IPlaybackEngine::AM_PLAYBACK_ENGINE_MSG_ABORT;
          NOTICE("Abort engine!");
        } else if (AM_LIKELY(msg.code == MSG_EOS)) {
          appMsg = IPlaybackEngine::AM_PLAYBACK_ENGINE_MSG_EOS;
          INFO("Playback engine received EOS, playback stopped!");
        } else {
          appMsg = IPlaybackEngine::AM_PLAYBACK_ENGINE_MSG_ERR;
          ERROR("Error occurred!");
        }
        if (AM_LIKELY(mState != AM_PLAYBACK_ENGINE_PAUSING)) {
          if (AM_LIKELY(!mIsDeleted)) {
            StopPurgeAllFilters();
          }
          mState = AM_PLAYBACK_ENGINE_STOPPED;
          PostAppMsg(appMsg);
        } else {
          mState = AM_PLAYBACK_ENGINE_PLAYING;
        }
        if (AM_LIKELY(mSignalCount > 0)) {
          mEvent->Signal();
          __atomic_dec(&mSignalCount);
          DEBUG("Wait count: %d", mSignalCount);
        }
      }break;
      default: {
        ERROR("Invalid message!");
        PostAppMsg(IPlaybackEngine::AM_PLAYBACK_ENGINE_MSG_NULL);
      }break;
    }
  }
}

void CPlaybackEngine::AppProc(void* context, AM_MSG& msg)
{
  if (AM_LIKELY(mAppCallback)) {
    mAppMsg.data = context;
    mAppMsg.msg = msg.code;
    mAppCallback(&mAppMsg);
  }
}
