/*******************************************************************************
 * input_playback.cpp
 *
 * History:
 *   2013-3-4 - [ypchang] created file
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
#include "am_data.h"

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
#include "am_media_info.h"

#include "utilities/am_file.h"
#include "audio_codec_info.h"

#include "packet_aggregator_if.h"
#include "audio_decoder_if.h"
#include "demuxer_if.h"

#include "input_playback_if.h"
#include "input_playback.h"

extern IPacketFilter* create_packet_aggregator_filter(IEngine *pEngine);
extern IPacketFilter* create_audio_decoder_filter(IEngine* engine);
extern IPacketFilter* create_demuxer_filter(IEngine* engine);

IInputPlayback* create_playback_input(CPacketFilterGraph *graph,
                                       IEngine *engine)
{
  return CInputPlayback::Create(graph, engine);
}

CInputPlayback* CInputPlayback::Create(CPacketFilterGraph *graph,
                                       IEngine *engine)
{
  CInputPlayback *result = new CInputPlayback(graph);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct(engine)))) {
    delete result;
    result = NULL;
  }

  return result;
}

AM_ERR CInputPlayback::Construct(IEngine *engine)
{
  mPacketAggregator = create_packet_aggregator_filter(engine);
  mAudioDecoder     = create_audio_decoder_filter(engine);
  mDemuxerFilter    = create_demuxer_filter(engine);
  mUniversalDemuxer = (mDemuxerFilter ?
      (IDemuxer*)mDemuxerFilter->GetInterface(IID_IDemuxer) : NULL);

  if (AM_LIKELY(mPacketAggregator)) {
    IPacketAggregator *aggregator = (IPacketAggregator*)
        (mPacketAggregator->GetInterface(IID_IPacketAggregator));
    if (AM_LIKELY(aggregator)) {
      aggregator->SetStreamNumber(1);
      aggregator->SetInputNumber(1);
    }
  }
  return ((mPacketAggregator && mAudioDecoder && mUniversalDemuxer) ?
      ME_OK : ME_NO_MEMORY);
}

AM_ERR CInputPlayback::AddUri(const char* uri)
{
  return mUniversalDemuxer->AddMedia(uri);
}

AM_ERR CInputPlayback::Play(const char* uri)
{
  return mUniversalDemuxer->Play(uri);
}

AM_ERR CInputPlayback::Stop()
{
  return mUniversalDemuxer->Stop();
}

AM_ERR CInputPlayback::CreateSubGraph()
{
  if (AM_UNLIKELY(mFilterGraph->AddFilter(mDemuxerFilter) != ME_OK)) {
    ERROR("Failed to add universal demuxer filter!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(mFilterGraph->AddFilter(mAudioDecoder) != ME_OK)) {
    ERROR("Failed to add audio decoder filter!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(mFilterGraph->AddFilter(mPacketAggregator) != ME_OK)) {
    ERROR("Failed to add PacketAggregator!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(mFilterGraph->Connect(
      mDemuxerFilter, 0, mAudioDecoder, 0) != ME_OK)) {
    ERROR("Failed to connect Universal Demuxer to audio decoder!");
    return ME_ERROR;
  }

  if (AM_UNLIKELY(mFilterGraph->Connect(
      mAudioDecoder, 0, mPacketAggregator, 0) != ME_OK)) {
    ERROR("Failed to connect AudioDecoder to PacketAggregator!");
    return ME_ERROR;
  }

  return ME_OK;
}
