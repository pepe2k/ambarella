/*******************************************************************************
 * output_playback.cpp
 *
 * History:
 *   2013-3-20 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include "config.h"
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

#include "audio_player.h"
#include "player_if.h"
#include "player.h"
#include "output_playback_if.h"
#include "output_playback.h"
#include "packet_distributor.h"

IOutputPlayback* create_playback_output(CPacketFilterGraph *graph,
                                        IEngine *engine)
{
  return COutputPlayback::Create(graph, engine);
}

COutputPlayback* COutputPlayback::Create(CPacketFilterGraph *graph,
                                         IEngine *engine)
{
  COutputPlayback* result = new COutputPlayback(graph);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct(engine)))) {
    delete result;
    result = NULL;
  }

  return result;
}

COutputPlayback::COutputPlayback(CPacketFilterGraph *graph) :
    mFilterGraph(graph),
    mPktDistributor(NULL),
    mPlayerFilter(NULL),
    mPlayer(NULL)
{
}

COutputPlayback::~COutputPlayback()
{
  AM_DELETE(mPktDistributor);
  AM_DELETE(mPlayerFilter);
  DEBUG("~COutputPlaback");
}

AM_ERR COutputPlayback::Construct(IEngine* engine)
{
  AM_ERR ret = ME_ERROR;
  do {
    if (AM_UNLIKELY(NULL ==
        (mPktDistributor = CPacketDistributor::Create(engine, 1)))) {
      ERROR("Failed to create packet distributor!");
      break;
    }

    if (AM_UNLIKELY(NULL == (mPlayerFilter = CPlayer::Create(engine)))) {
      ERROR("Failed to create player filter!");
      break;
    }

    if (AM_UNLIKELY(NULL == (mPlayer = (IPlayer*)((CPlayer*)mPlayerFilter)->\
        GetInterface(IID_IPlayer)))) {
      ERROR("Failed to get player interface!");
      break;
    }

    ret = ME_OK;
  } while (0);

  return ret;
}

AM_ERR COutputPlayback::Stop()
{
  return mPlayer->Stop();
}

AM_ERR COutputPlayback::Pause(bool enable)
{
  return mPlayer->Pause(enable);
}

AM_ERR COutputPlayback::CreateSubGraph()
{
  AM_ERR ret = ME_ERROR;
  do {
    if (AM_UNLIKELY(ME_OK != mFilterGraph->AddFilter(mPktDistributor))) {
      ERROR("Failed to add packet distributor to pipeline!");
      break;
    }

    if (AM_UNLIKELY(ME_OK != mFilterGraph->AddFilter(mPlayerFilter))) {
      ERROR("Failed to add player to pipeline!");
      break;
    }

    if (AM_UNLIKELY(ME_OK != mFilterGraph->Connect(mPktDistributor,
                                                   0,
                                                   mPlayerFilter,
                                                   0))) {
      ERROR("Failed to connect packet distributor to player!");
      break;
    }
    ((CPacketDistributor *)mPktDistributor)->SetSourceMask(0, 1);
    ret = ME_OK;
  } while(0);

  return ret;
}
