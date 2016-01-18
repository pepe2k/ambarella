/*******************************************************************************
 * core.cpp
 *
 * Histroy:
 *   2012-10-8 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_data.h"

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

#include "av_queue_builder.h"
#include "av_queue_if.h"
#include "av_queue.h"

#include "core_if.h"
#include "core.h"

ICore* create_record_core(CPacketFilterGraph *graph, IEngine *engine)
{
  return CCore::Create(graph, engine);
}

CCore *CCore::Create(CPacketFilterGraph *graph, IEngine *engine)
{
  CCore *result = new CCore(graph);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct(engine)))) {
    delete result;
    result = NULL;
  }

  return result;
}

AM_ERR CCore::Construct(IEngine *engine)
{
  if ((mAvQueue = CAVQueue::Create(engine)) == NULL) {
     ERROR ("Failed to create AVQueue!");
     return ME_ERROR;
  }

  return ME_OK;
}

AM_ERR CCore::CreateSubGraph()
{
   return (mFilterGraph && mAvQueue) ?
      mFilterGraph->AddFilter (mAvQueue) : ME_ERROR;
}

bool CCore::IsReadyForEvent()
{
  IAVQueue *AvQueue = (IAVQueue *)( mAvQueue ?
      mAvQueue->GetInterface(IID_IAVQueue) : NULL);
  return AvQueue ? AvQueue->IsReadyForEvent() : false;
}

AM_ERR CCore::SetEventStreamId(AM_UINT StreamId)
{
  IAVQueue *AvQueue = (IAVQueue*)(mAvQueue ?
        mAvQueue->GetInterface(IID_IAVQueue) : NULL);
  if (!AvQueue) {
    return ME_ERROR;
  }
  AvQueue->SetEventStreamId(StreamId);
  return ME_OK;
}

AM_ERR CCore::SetEventDuration(AM_UINT history_duration, AM_UINT future_duration)
{
  IAVQueue *AvQueue = (IAVQueue*)(mAvQueue ?
      mAvQueue->GetInterface(IID_IAVQueue) : NULL);
  if (!AvQueue) {
    return ME_ERROR;
  }
  AvQueue->SetEventDuration(history_duration, future_duration);
  return ME_OK;
}
