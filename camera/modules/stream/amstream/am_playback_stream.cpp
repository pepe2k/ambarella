/*******************************************************************************
 * am_playback_stream.cpp
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

#include "playback_engine_if.h"

#include "am_include.h"
#include "am_utility.h"
#include "stream/am_playback_stream.h"

extern IPlaybackEngine* create_playback_engine();

AmPlaybackStream::AmPlaybackStream() :
  mPlaybackEngine(NULL),
  mIsInitialized(false)
{
}

AmPlaybackStream::~AmPlaybackStream()
{
  AM_DELETE(mPlaybackEngine);
  if (AM_LIKELY(mIsInitialized)) {
    AMF_Terminate();
  }
  DEBUG("~AmPlaybackStream");
}

bool AmPlaybackStream::init_playback_stream()
{
  if (AM_LIKELY(!mIsInitialized)) {
    mIsInitialized = (ME_OK == AMF_Init());
  }
  if (AM_LIKELY(!mPlaybackEngine)) {
    mPlaybackEngine = create_playback_engine();
  }

  return mIsInitialized && mPlaybackEngine && mPlaybackEngine->CreateGraph();
}

void AmPlaybackStream::set_app_msg_callback(AmPlaybackStreamAppCb callback,
                                            void *data)
{
  mPlaybackEngine->SetAppMsgCallback(
      (IPlaybackEngine::AmPlaybackEngineAppCb)callback, data);
}

bool AmPlaybackStream::is_playing()
{
  return mPlaybackEngine && (mPlaybackEngine->GetEngineStatus() ==
      IPlaybackEngine::AM_PLAYBACK_ENGINE_PLAYING);
}

bool AmPlaybackStream::is_paused()
{
  return mPlaybackEngine && (mPlaybackEngine->GetEngineStatus() ==
      IPlaybackEngine::AM_PLAYBACK_ENGINE_PAUSED);
}

bool AmPlaybackStream::add_uri(const char *uri)
{
  return mPlaybackEngine->AddUri(uri);
}

bool AmPlaybackStream::play(const char *uri)
{
  return mPlaybackEngine->Play(uri);
}

bool AmPlaybackStream::pause(bool enable)
{
  return mPlaybackEngine->Pause(enable);
}

bool AmPlaybackStream::stop()
{
  return mPlaybackEngine->Stop();
}
