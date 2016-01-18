/*******************************************************************************
 * player.cpp
 *
 * History:
 *   2013-3-21 - [ypchang] created file
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
#include "am_muxer_info.h"
#include "output_playback_if.h"
#include "mpeg_ts_defs.h"
#include "am_media_info.h"

#include "audio_player.h"
#include "player_if.h"
#include "player.h"

CPlayer* CPlayer::Create(IEngine* engine, bool RTPriority, int priority)
{
  CPlayer* result = new CPlayer(engine);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct(RTPriority,
                                                        priority)))) {
    delete result;
    result = NULL;
  }

  return result;
}

CPlayer::CPlayer(IEngine *engine) :
  inherited(engine, "PlayerFilter"),
  mMutex(NULL),
  mInputPin(NULL),
  mAudioPlayer(NULL),
  mEosMap(0),
  mRun(false),
  mIsPause(false)
{}

CPlayer::~CPlayer()
{
  AM_DELETE(mMutex);
  AM_DELETE(mInputPin);
  delete mAudioPlayer;
}

AM_ERR CPlayer::Construct(bool RTPriority, int priority)
{
  AM_ERR ret = ME_ERROR;
  do {
    if (AM_UNLIKELY(ME_OK != inherited::Construct(RTPriority, priority))) {
      ERROR("Failed to construct parent!");
      break;
    }

    if (AM_UNLIKELY(NULL == (mMutex = CMutex::Create()))) {
      ERROR("Failed to create mutex!");
      break;
    }

    if (AM_UNLIKELY(NULL == (mInputPin = CPlayerInput::Create(
        this, (const char*)"PlayerInput")))) {
      ERROR("Failed to create input pin for player filter!");
      break;
    }

    if (AM_UNLIKELY(NULL == (mAudioPlayer = CPlayerPulse::Create()))) {
      ERROR("Failed to create audio player!");
      break;
    }
    ret = ME_OK;
  } while (0);

  return ret;
}

AM_ERR CPlayer::Start()
{
  return ME_OK;
}

AM_ERR CPlayer::Stop()
{
  AUTO_LOCK(mMutex);
  AM_ERR ret = ME_OK;
  if (AM_UNLIKELY(mIsPause)) {
    mInputPin->Enable(true);
    mIsPause = false;
  }
  if (AM_LIKELY(mRun)) {
    mRun = false;
    mInputPin->Enable(false);
    DEBUG("After mInputPin->Enable(false)");
    mInputPin->Stop();
    DEBUG("After mInputPin->Stop()");
    ret = inherited::Stop();
    NOTICE("CPlayer stop!");
  }

  return ret;
}

AM_ERR CPlayer::Pause(bool enable)
{
  AUTO_LOCK(mMutex);
  AM_ERR ret = ME_ERROR;
  if (AM_LIKELY(mAudioPlayer->IsPlayerRunning())) {
    ret = mAudioPlayer->Pause(enable);
    mIsPause = (ME_OK == ret) ? enable : mIsPause;
    mInputPin->Enable(!mIsPause);
  }
  PostEngineMsg((ME_OK == ret) ? IEngine::MSG_OK : IEngine::MSG_ERROR);

  return ret;
}

void CPlayer::OnRun()
{
  CmdAck(ME_OK);
  mInputPin->Enable(true);
  mInputPin->Run();
  mRun = true;

  while(mRun) {
    if (AM_UNLIKELY(mIsPause)) {
      usleep(10000);
      continue;
    }
    mAudioPlayer->FeedData();
  }

  DEBUG("After while(mRun)");
  mAudioPlayer->Stop(false);
  DEBUG("After mAudioPlayer->Stop(false)");
  if (AM_LIKELY(!mRun)) {
    NOTICE("Player post EOS!");
    PostEngineMsg(IEngine::MSG_EOS);;
  } else {
    NOTICE("Player post ABORT!");
    PostEngineMsg(IEngine::MSG_ABORT);
  }
  mRun = false;
  INFO("Player filter exit mainloop!");
}

inline AM_ERR CPlayer::OnInfo(CPacket *packet)
{
  AM_ERR ret = ME_ERROR;
  switch(packet->GetAttr()) {
    case CPacket::AM_PAYLOAD_ATTR_AUDIO: {
      AM_AUDIO_INFO* audioinfo = (AM_AUDIO_INFO*)(packet->GetDataPtr());
      INFO("\nPlayer received audio info packet:"
           "\n                         Channels: %u"
           "\n                       SampleRate: %u"
           "\n                       SampleSize: %u",
           audioinfo->channels, audioinfo->sampleRate, audioinfo->sampleSize);
      ret = mAudioPlayer->Start(*audioinfo);
      mEosMap |= 1 << 0;
    }break;
    case CPacket::AM_PAYLOAD_ATTR_VIDEO: {
      /* todo: Add video play */
      mEosMap |= 1 << 1;
    } break;
    default:
      ERROR("Only audio and video can be played!");
      break;
  }

  return ret;
}

inline AM_ERR CPlayer::OnData(CPacket *packet)
{
  AM_ERR ret = ME_OK;
  switch(packet->GetAttr()) {
    case CPacket::AM_PAYLOAD_ATTR_AUDIO: {
      mAudioPlayer->AddPacket(packet);
    } break;
    case CPacket::AM_PAYLOAD_ATTR_VIDEO: {
      /* todo: Add video player */
    } break;
    default:
      ERROR("Only audio and video can be played!");
      ret = ME_ERROR;
      break;
  }

  return ret;
}

inline AM_ERR CPlayer::OnEOF(CPacket *packet)
{
  NOTICE("Player received EOF!");
  if (mEosMap & (1 << 1)) {
    NOTICE("Prepare to stop video!");
    /* todo Add video playback */
    mEosMap &= ~(1 << 1);
  }
  if (mEosMap & (1 << 0)) {
    NOTICE("Prepare to stop audio!");
    mEosMap &= ~(1 << 0);
    mAudioPlayer->AddPacket(packet);
    mAudioPlayer->Stop();
  }

  PostEngineMsg(IEngine::MSG_EOF);
  return ME_OK;
}

AM_ERR CPlayer::ProcessBuffer(CPacket *packet)
{
  AUTO_LOCK(mMutex);
  AM_ERR ret = ME_OK;
  if (AM_LIKELY(packet)) {
    switch(packet->GetType()) {
      case CPacket::AM_PAYLOAD_TYPE_INFO: {
        ret = OnInfo(packet);
        if (AM_LIKELY(ME_OK == ret)) {
          PostEngineMsg(IEngine::MSG_OK);
        }
      }break;
      case CPacket::AM_PAYLOAD_TYPE_DATA: {
        ret = OnData(packet);
      }break;
      case CPacket::AM_PAYLOAD_TYPE_EOF: {
        ret = OnEOF(packet);
      }break;
      default: {
        ERROR("Unknown packet type: %u!", packet->GetType());
      }break;
    }
    packet->Release();
  }

  return ret;
}
