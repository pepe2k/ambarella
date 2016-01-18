/*******************************************************************************
 * record_engine.cpp
 *
 * Histroy:
 *   2012-10-9 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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

#include "input_record_if.h"

#include "audio_codec_info.h"
#include "core_if.h"

#include "am_muxer_info.h"
#include "output_record_if.h"

#include "stream_module_info.h"
#include "record_engine_if.h"
#include "record_engine.h"

#define HW_TIMER ((const char*)"/proc/ambarella/ambarella_hwtimer")

extern IInputRecord*  create_record_input (CPacketFilterGraph *graph,
                                           IEngine *engine);
extern ICore*         create_record_core  (CPacketFilterGraph *graph,
                                           IEngine *engine);
extern IOutputRecord* create_record_output(CPacketFilterGraph *graph,
                                           IEngine *engine);

IRecordEngine* create_record_engine()
{
  return CRecordEngine::Create();
}

CRecordEngine* CRecordEngine::Create()
{
  CRecordEngine* result = new CRecordEngine();
  if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }

  return result;
}

CRecordEngine::~CRecordEngine()
{
  StopAllFilters();
  PurgeAllFilters();
  DeleteAllConnections();
  mnFilters = 0;
  AM_DELETE(mModuleInput);
  AM_DELETE(mModuleCore);
  AM_DELETE(mModuleOutput);
  AM_DELETE(mEvent);
  AM_DELETE(mMutex);
}

AM_ERR CRecordEngine::Construct()
{
  AM_ERR ret = inherited::Construct();
  if (AM_LIKELY(ME_OK ==  ret)) {
    mModuleInput  = create_record_input(this, (IEngine*)this);
    mModuleCore   = create_record_core(this, (IEngine*)this);
    mModuleOutput = create_record_output(this, (IEngine*)this);
    SetAppMsgCallback(StaticAppProc, this);
    mEvent = CEvent::Create();
    mMutex = CMutex::Create();
    if (AM_UNLIKELY(!mModuleInput)) {
      ERROR("Failed to create input module!");
    }

    if (AM_UNLIKELY(!mModuleCore)) {
      ERROR("Failed to create core module!");
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

    ret = (mModuleInput && mModuleCore && mModuleOutput && mEvent && mMutex)
        ? ME_OK : ME_NO_MEMORY;
  }

  return ret;
}

bool CRecordEngine::CreateGraph()
{
  if (AM_LIKELY(!mIsGraphCreated &&
                (mModuleInput && mModuleCore && mModuleOutput))) {
    if (AM_LIKELY(ME_OK == mModuleInput->CreateSubGraph())) {
      if (AM_LIKELY(ME_OK == mModuleCore->CreateSubGraph())) {
        if (AM_LIKELY(ME_OK == mModuleOutput->CreateSubGraph())) {
          if (AM_LIKELY(ME_OK == CreateConnection(
              mModuleInput->GetModuleOutputPin(),
              mModuleCore->GetModuleInputPin()))) {
            if (AM_LIKELY(ME_OK == CreateConnection(
                mModuleCore->GetModuleOutputPin(),
                mModuleOutput->GetModuleInputPin()))) {
              mIsGraphCreated = true;
            } else {
              ERROR("Failed to connect Core Module to Output Module!");
            }
          } else {
            ERROR("Failed to connect Input Module to Core Module!");
          }
        } else {
          ERROR("Failed to create Output Module sub-graph!");
        }
      } else {
        ERROR("Failed to create Core Module sub-graph!");
      }
    } else {
      ERROR("Failed to create Input Module sub-graph!");
    }
  } else if (AM_UNLIKELY(!mModuleInput)) {
    ERROR("Failed to create Input Module!");
  } else if (AM_UNLIKELY(!mModuleCore)) {
    ERROR("Failed to create Core Module!");
  } else if (AM_UNLIKELY(!mModuleOutput)) {
    ERROR("Failed to create Output Module!");
  }

  return mIsGraphCreated;
}

bool CRecordEngine::SetModuleParameters(ModuleCoreInfo *info)
{
  bool ret = true;

  if (mModuleCore->SetEventStreamId(info->eventStreamId) != ME_OK) {
    ret = false;
  }
  if (mModuleCore->SetEventDuration(info->eventHistoryDuration,
                                    info->eventFutureDuration) != ME_OK) {
    ret = false;
  }
  return ret;
}

bool CRecordEngine::SetModuleParameters(ModuleInputInfo *info)
{
  if (mModuleInput == NULL) {
    ERROR ("Input module is not created!");
    return false;
  }

  if (info == NULL) {
    ERROR ("Invalid ModuleInputInfo");
    return false;
  }

  if (AM_UNLIKELY(mModuleInput->SetStreamNumber(info->streamNumber) != ME_OK)) {
    ERROR("Failed to set Stream Number!");
    return false;
  }

  if (AM_UNLIKELY(mModuleInput->SetInputNumber(info->inputNumber) != ME_OK)) {
    ERROR("Failed to set Input Number!");
    return false;
  }

  if (AM_UNLIKELY(mModuleInput->SetEnabledAudioId(info->audioMap) != ME_OK)) {
    ERROR ("Failed to set Input Module: Audio stream ID!");
    return false;
  }

  if (AM_UNLIKELY(mModuleInput->SetAudioInfo(info->audioSampleRate,
                                             info->audioChannels,
                                             info->audioCodecInfo.codecType,
                                             &info->audioCodecInfo.codecInfo)
                  != ME_OK)) {
    ERROR("Failed to set Input Module: Audio Info Parameters!");
    return false;
  }

  if (AM_UNLIKELY(mModuleInput->SetEnabledVideoId(info->videoMap) != ME_OK)) {
    ERROR ("Failed to set Input Module: Video Stream ID!");
    return false;
  }

  if (AM_UNLIKELY(mModuleInput->SetAvSyncMap(info->avSyncMap) != ME_OK)) {
    ERROR("Failed to set Input Module: AV sync map!");
    return false;
  }

  return true;
}

bool CRecordEngine::SetModuleParameters(ModuleOutputInfo *info)
{
  bool ret = false;
  const char *streamName[] = {"first", "second", "third", "fourth"};
  if (AM_LIKELY(mModuleOutput && info)) {
    for (AM_UINT i = 0; i < AM_STREAM_NUMBER_MAX; ++ i) {
      if (AM_UNLIKELY(ME_OK != mModuleOutput->SetMuxerMask(
          (AmStreamNumber)i, info->muxerMask[i]))) {
        ERROR ("Failed to set muxer mask for %s stream.", streamName[i]);
        ret = false;
        break;
      } else {
        ret = true;
      }
    }
    if (AM_LIKELY(ret)) {
      for (OutputMuxerInfo *mInfo = info->muxerInfo;
          mInfo != NULL; mInfo = mInfo->next) {
        INFO ("Info->muxerInfo->uri = %s\n", mInfo->uri);
        if (AM_LIKELY(mInfo->muxerType == AM_MUXER_TYPE_RTSP)) {
          mModuleOutput->SetRtspAttribute(mInfo->rtspSendWait,
                                          mInfo->rtspNeedAuth);
        }
        if (AM_LIKELY(ME_OK == mModuleOutput->SetSplitDuration(
            mInfo->streamId, mInfo->muxerType,
            mInfo->duration, mInfo->alignGop))) {
          if (AM_LIKELY(ME_OK == mModuleOutput->SetMediaSource(
              mInfo->streamId, mInfo->muxerType))) {
            if (AM_LIKELY(ME_OK == mModuleOutput->SetMaxFileAmount (
                mInfo->streamId, mInfo->muxerType, mInfo->maxFileAmount))) {
              ret = true;
            } else {
              ret = false;
              ERROR ("Failed to set Output Module: Max File Amount!");
              break;
            }
          } else {
            ret = false;
            ERROR("Failed to set Output Module: Media Source!");
            break;
          }
        } else {
          ret = false;
          ERROR("Failed to set Output Module: Duration!");
          break;
        }
      }
    }
  } else if (AM_UNLIKELY(!mModuleOutput)) {
    ERROR("Output Module is not created!");
  } else if (AM_UNLIKELY(!info)) {
    ERROR("Invalid ModuleOutputInfo!");
  }

  return ret;
}

bool CRecordEngine::SetOutputMuxerUri(ModuleOutputInfo *info)
{
  bool ret = true;
  for (OutputMuxerInfo *mInfo = info->muxerInfo; mInfo; mInfo = mInfo->next) {
    ret = (ME_OK == mModuleOutput->SetMediaSink(mInfo->streamId,
                                                mInfo->muxerType,
                                                mInfo->uri));
    if (AM_UNLIKELY(!ret)) {
      ERROR("Failed to set Output Module: Media Sink!");
      break;
    }
  }

  return ret;
}

bool CRecordEngine::Start()
{
  bool ret = true;
  AUTO_LOCK(mMutex);

  if (AM_LIKELY(mState == AM_RECORD_ENGINE_STOPPED)) {
    if (AM_LIKELY(ResetHwTimer())) {
      if (AM_LIKELY((ME_OK == mModuleInput->PrepareToRun()) &&
                    (ME_OK == mModuleCore->PrepareToRun()) &&
                    (ME_OK == mModuleOutput->PrepareToRun()))) {
        if (AM_UNLIKELY(ME_OK != RunAllFilters())) {
          StopAllFilters();
          PurgeAllFilters();
          ret = false;
        } else if (AM_LIKELY(ME_OK == mModuleInput->StartAll())) {
          mState = AM_RECORD_ENGINE_RECORDING;
        } else {
          ERROR("Failed to start Input Module!");
          ret = false;
        }
      } else {
        ERROR("Failed to prepare modules!");
        ret = false;
      }
    } else {
      ERROR("Failed to reset hw timer!");
      ret = false;
    }
  } else if (AM_UNLIKELY(mState == AM_RECORD_ENGINE_STOPPING)) {
    ERROR("Stream engine is stopping now, start again when it's stopped!");
    ret = false;
  } else {
    NOTICE("Stream engine has been started already!");
  }

  return ret;
}

bool CRecordEngine::Stop()
{
  bool ret = true;
  AUTO_LOCK(mMutex);
  if (AM_LIKELY(mState == AM_RECORD_ENGINE_RECORDING)) {
    if (AM_UNLIKELY(ME_OK != mModuleInput->StopAll())) {
      ERROR("Failed to stop input module!");
      ret = false;
    } else {
      if (AM_LIKELY(mNeedBlock)) {
        mEvent->Wait();
      }
      INFO("Input module stopped successfully!");
    }
  } else if (AM_UNLIKELY(mState == AM_RECORD_ENGINE_STOPPED)) {
    NOTICE("Stream engine has been stopped already!");
  }

  return ret;
}

void CRecordEngine::MsgProc(AM_MSG& msg)
{
  if (AM_LIKELY(IsSessionMsg(msg))) {
    switch(msg.code) {
      case MSG_EOS: {
        ++ mStoppedMuxerNum;
        INFO("Output Module supports %u muxers, received %u muxer EOS!",
             mModuleOutput->GetMuxerAmount(), mStoppedMuxerNum);
        if (AM_LIKELY(mStoppedMuxerNum ==
            mModuleOutput->GetMuxerAmount())) {
          mState = AM_RECORD_ENGINE_STOPPED;
          mStoppedMuxerNum = 0;
          PurgeAllFilters();
          mEvent->Signal();
          PostAppMsg(msg.code);
        }
      }break;
      case MSG_ABORT: {
        AUTO_LOCK(mMutex);
        ERROR("Fatal error occurred in muxer, abort engine!");
        mNeedBlock = false;
        Stop();
        mState = AM_RECORD_ENGINE_STOPPED;
        mStoppedMuxerNum = 0;
        StopAllFilters();
        PurgeAllFilters();
        mNeedBlock = true;
        PostAppMsg(msg.code);
      }break;
      case MSG_ERROR: {
        AUTO_LOCK(mMutex);
        ERROR("Fatal error occurred, abort engine!");
        mNeedBlock = false;
        if (AM_UNLIKELY(!Stop())) {
          mState = AM_RECORD_ENGINE_STOPPED;
          mStoppedMuxerNum = 0;
          StopAllFilters();
          PurgeAllFilters();
        }
        mNeedBlock = true;
        PostAppMsg(msg.code);
      }break;
      case MSG_OVFL: {
        AUTO_LOCK(mMutex);
        ERROR ("Network condition is not good, recording will restart!");
        mNeedBlock = false;
        if (AM_UNLIKELY(!Stop())) {
          mState = AM_RECORD_ENGINE_STOPPED;
          mStoppedMuxerNum = 0;
          StopAllFilters();
          PurgeAllFilters();
        }
        mNeedBlock = true;
        PostAppMsg (msg.code);
      }break;
      default:break;
    }
  }
}

void CRecordEngine::AppProc(void* context, AM_MSG& msg)
{
  if (AM_LIKELY(mAppCallback)) {
    mAppMsg.data = context;
    switch(msg.code) {
      case IEngine::MSG_EOS: {
        mAppMsg.msg = IRecordEngine::AM_RECORD_ENGINE_MSG_EOS;
      }break;
      case IEngine::MSG_ABORT: {
        mAppMsg.msg = IRecordEngine::AM_RECORD_ENGINE_MSG_ABORT;
      }break;
      case IEngine::MSG_ERROR: {
        mAppMsg.msg = IRecordEngine::AM_RECORD_ENGINE_MSG_ERR;
      }break;
      case IEngine::MSG_OVFL: {
        mAppMsg.msg = IRecordEngine::AM_RECORD_ENGINE_MSG_OVFL;
      }break;
      default: {
        mAppMsg.msg = IRecordEngine::AM_RECORD_ENGINE_MSG_NULL;
      }break;
    }
    mAppCallback(&mAppMsg);
  }
}

void CRecordEngine::StaticAppProc(void* context, AM_MSG& msg)
{
  ((CRecordEngine*)context)->AppProc(((CRecordEngine*)context)->mAppData, msg);
}

bool CRecordEngine::ResetHwTimer()
{
  bool ret = true;
  int fd = open(HW_TIMER, O_WRONLY);
  if (AM_LIKELY(fd >= 0)) {
    char value = '0';
    if (AM_UNLIKELY(write(fd, &value, sizeof(value)) != sizeof(value))) {
      PERROR("write");
      ret = false;
    }
    close(fd);
  } else {
    PERROR("open");
    ret = false;
  }

  return ret;
}

bool CRecordEngine::SendUsrSEI(void *pUserData, AM_U32 len)
{
  bool ret = true;
  AUTO_LOCK(mMutex);
  if (AM_LIKELY(mState == AM_RECORD_ENGINE_RECORDING)) {
    if (AM_UNLIKELY(ME_OK != mModuleInput->SendSEI(pUserData, len))) {
      ERROR("Failed to SendSEI!");
      ret = false;
    } else {
      INFO("Input module SendSEI successfully!");
    }
  }

  return ret;
}

bool CRecordEngine::SendUsrEvent(CPacket::AmPayloadAttr eventType)
{
  bool ret = true;
  AUTO_LOCK(mMutex);
  if (AM_LIKELY((mState == AM_RECORD_ENGINE_RECORDING) &&
                mModuleCore->IsReadyForEvent())) {
    if (AM_UNLIKELY(ME_OK != mModuleInput->SendEvent(eventType))) {
      ERROR("Failed to SendEvent!");
      ret = false;
    } else {
      INFO("Input module SendEvent successfully!");
    }
  } else {
      ERROR("Current state is error!");
  }

  return ret;
}

void CRecordEngine::SetAppMsgCallback(AmRecordEngineAppCb callback, void* data)
{
  mAppCallback = callback;
  mAppData     = data;
}

#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
void CRecordEngine::SetFrameStatisticsCallback(void (*callback)(void *data))
{
  mModuleInput->SetFrameStatisticsCallback(callback);
}
#endif
