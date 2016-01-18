/*******************************************************************************
 * input_record.cpp
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

#include "audio_capture_if.h"
#include "audio_encoder_if.h"
#include "video_recorder_if.h"
#include "event_sender_if.h"
#include "sei_sender_if.h"
#include "packet_aggregator_if.h"

#include "input_record_if.h"
#include "input_record.h"

extern IPacketFilter* create_video_recorder_filter   (IEngine *engine);
extern IPacketFilter* create_audio_capture_filter    (IEngine *engine);
extern IPacketFilter* create_audio_encoder_filter    (IEngine *engine);
extern IPacketFilter* create_sei_sender_filter       (IEngine *engine);
extern IPacketFilter* create_event_sender_filter     (IEngine *engine);
extern IPacketFilter* create_packet_aggregator_filter(IEngine *engine);

IInputRecord* create_record_input(CPacketFilterGraph *graph, IEngine *engine)
{
  return CInputRecord::Create(graph, engine);
}

CInputRecord* CInputRecord::Create(CPacketFilterGraph *graph, IEngine *engine)
{
  CInputRecord *result = new CInputRecord(graph);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct(engine)))) {
    delete result;
    result = NULL;
  }

  return result;
}

AM_ERR CInputRecord::Construct(IEngine *engine)
{
  mVideoRecorder    = create_video_recorder_filter(engine);
  mAudioCapture     = create_audio_capture_filter(engine);
  mAudioEncoder     = create_audio_encoder_filter(engine);
  mSeiSender        = create_sei_sender_filter(engine);
  mEventSender      = create_event_sender_filter(engine);
  mPacketAggregator = create_packet_aggregator_filter(engine);

  if (AM_UNLIKELY(!mVideoRecorder)) {
    ERROR("Failed to create video recorder!");
  }

  if (AM_UNLIKELY(!mAudioCapture)) {
    ERROR("Failed to create audio capture!");
  }

  if (AM_UNLIKELY(!mAudioEncoder)) {
    ERROR("Failed to create audio encoder!");
  }

  if (AM_UNLIKELY(!mSeiSender)) {
    ERROR("Failed to create SEI sender!");
  }

  if (AM_UNLIKELY(!mEventSender)) {
    ERROR("Failed to create event sender!");
  }

  if (AM_UNLIKELY(!mPacketAggregator)) {
    ERROR("Failed to create packet aggregator!");
  }

  return ((mAudioCapture && mAudioEncoder && mVideoRecorder &&
           mPacketAggregator && mSeiSender && mEventSender) ?
               ME_OK : ME_NO_MEMORY);
}

AM_ERR CInputRecord::SetStreamNumber(AM_UINT streamNum)
{
  IPacketAggregator* aggregator = (IPacketAggregator*)
      (mPacketAggregator->GetInterface(IID_IPacketAggregator));
  return aggregator ? aggregator->SetStreamNumber(streamNum) : ME_ERROR;
}

AM_ERR CInputRecord::SetInputNumber(AM_UINT inputNum)
{
  IPacketAggregator* aggregator = (IPacketAggregator*)
      (mPacketAggregator->GetInterface(IID_IPacketAggregator));
  return aggregator ? aggregator->SetInputNumber(inputNum) : ME_ERROR;
}

AM_UINT CInputRecord::GetMaxEncodeNumber()
{
  IVideoSource *vSrc = (IVideoSource *)
      (mVideoRecorder->GetInterface(IID_IVideoSource));
  return vSrc ? vSrc->GetMaxEncodeNumber() : 0;
}

AM_ERR CInputRecord::SetAudioInfo(AM_UINT samplerate, AM_UINT channels,
                                  AM_UINT codecType,  void* codecInfo)
{
  IAudioSource *aSrc = (IAudioSource*)
      (mAudioCapture->GetInterface(IID_IAudioSource));
  IAudioEncoder *aEncoder = (IAudioEncoder *)
      (mAudioEncoder->GetInterface(IID_IAudioEncoder));
  AM_ERR ret = ME_ERROR;

  if (AM_LIKELY(aSrc && aEncoder)) {
    aSrc->SetAudioParameters(samplerate, channels, codecType);
    ret = aEncoder->SetAudioCodecType(codecType, codecInfo);
  } else {
    if (AM_LIKELY(!aSrc)) {
      ERROR("AudioCapture is not created!");
    }
    if (AM_LIKELY(!aEncoder)) {
      ERROR("AudioEncoder is not created!");
    }
  }

  return ret;
}

AM_ERR CInputRecord::SetEnabledVideoId(AM_UINT streams)
{
  AM_ERR ret = ME_ERROR;
  IVideoSource *vSrc = (IVideoSource *)
      (mVideoRecorder->GetInterface(IID_IVideoSource));

  mVideoStreamMap = streams;
  if (AM_LIKELY(vSrc)) {
    vSrc->SetEnabledVideoId(mVideoStreamMap);
    ret = ME_OK;
  } else {
    ERROR("VideoRecorder is not created!");
  }

  return ret;
}

AM_ERR CInputRecord::CreateSubGraph()
{
   if (AM_UNLIKELY(mFilterGraph->AddFilter (mAudioCapture) != ME_OK)) {
      ERROR ("Failed to add AudioCapture!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(mFilterGraph->AddFilter (mAudioEncoder) != ME_OK)) {
      ERROR ("Failed to add AudioEncoder!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(mFilterGraph->AddFilter (mVideoRecorder) != ME_OK)) {
      ERROR ("Failed to add VideoRecorder!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(mFilterGraph->AddFilter (mSeiSender) != ME_OK)) {
      ERROR ("Failed to add SeiSender!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(mFilterGraph->AddFilter (mEventSender) != ME_OK)) {
      ERROR ("Failed to add EventSender!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(mFilterGraph->AddFilter (mPacketAggregator) != ME_OK)) {
      ERROR ("Failed to add PacketAggregator!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(mFilterGraph->Connect (mAudioCapture, 0, mAudioEncoder, 0)
                   != ME_OK)) {
      ERROR ("Failed to connect AudioCapture with AudioEncoder!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(mFilterGraph->Connect (
       mAudioEncoder, 0, mPacketAggregator, 0) != ME_OK)) {
      ERROR ("Failed to connect AudioEncoder with PacketAggregator!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(mFilterGraph->Connect (
       mVideoRecorder, 0, mPacketAggregator, 1) != ME_OK)) {
      ERROR ("Failed to connect mVideoRecorder with PacketAggregator!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(mFilterGraph->Connect (
       mSeiSender, 0, mPacketAggregator, 2) != ME_OK)) {
      ERROR ("Failed to connect mSeiSender with PacketAggregator!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(mFilterGraph->Connect (
       mEventSender, 0, mPacketAggregator, 3) != ME_OK)) {
      ERROR ("Failed to connect mSeiSender with PacketAggregator!");
      return ME_ERROR;
   }

   return ME_OK;
}

AM_ERR CInputRecord::SetEnabledAudioId(AM_UINT streams)
{
  AM_ERR ret = ME_ERROR;
  IAudioSource *aSrc = (IAudioSource*)(mAudioCapture ?
      mAudioCapture->GetInterface(IID_IAudioSource) : NULL);

  mAudioStreamMap = streams;
  if (AM_LIKELY(aSrc)) {
    aSrc->SetEnabledAudioId(mAudioStreamMap);
    ret = ME_OK;
  } else {
    ERROR("AudioCapture is not created!");
  }

  return ret;
}

AM_ERR CInputRecord::SetAvSyncMap(AM_UINT avSyncMap)
{
  AM_ERR ret = ME_ERROR;
  IAudioSource *aSrc = (IAudioSource*)(mAudioCapture ?
      mAudioCapture->GetInterface(IID_IAudioSource) : NULL);
  IVideoSource *vSrc = (IVideoSource *)(mVideoRecorder ?
        mVideoRecorder->GetInterface(IID_IVideoSource) : NULL);
  mAVSyncMap = avSyncMap;
  if (AM_UNLIKELY(!aSrc)) {
    ERROR("AudioCapture is not created!");
  }
  if (AM_UNLIKELY(!vSrc)) {
    ERROR("VideoRecorder is not created!");
  }
  if (AM_LIKELY(aSrc && vSrc)) {
    aSrc->SetAvSyncMap(mAVSyncMap);
    vSrc->SetAvSyncMap(mAVSyncMap);
    ret = ME_OK;
  }

  return ret;
}

#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
void CInputRecord::SetFrameStatisticsCallback(void (*callback)(void *data))
{
  IVideoSource *vSrc = (IVideoSource*)(mVideoRecorder ?
      mVideoRecorder->GetInterface(IID_IVideoSource) : NULL);
  if (AM_LIKELY(vSrc)) {
    vSrc->SetFrameStatisticCallback(callback);
  }
}
#endif

AM_ERR CInputRecord::StartAudio()
{
  IAudioSource *aSrc = (IAudioSource*)(mAudioCapture ?
      mAudioCapture->GetInterface(IID_IAudioSource) : NULL);
  return aSrc ? aSrc->Start() : ME_ERROR;
}

AM_ERR CInputRecord::StopAudio ()
{
  AM_ERR ret = ME_ERROR;
  IAudioSource *aSrc = (IAudioSource*)(mAudioCapture ?
      mAudioCapture->GetInterface(IID_IAudioSource) : NULL);
  IAudioEncoder *aEnc = (IAudioEncoder*)(mAudioEncoder ?
          mAudioEncoder->GetInterface(IID_IAudioEncoder) : NULL);
  ret = aSrc ? aSrc->Stop() : ME_ERROR;
  if (AM_UNLIKELY((ret == ME_OK) && (mAudioStreamMap == 0))) {
    ret = aEnc ? aEnc->Stop() : ME_ERROR;
  }
  return ret;
}

AM_ERR CInputRecord::StartAll()
{
  IAudioSource *aSrc = (IAudioSource*)(mAudioCapture ?
      mAudioCapture->GetInterface(IID_IAudioSource) : NULL);

  IVideoSource *vSrc = (IVideoSource*)(mVideoRecorder ?
      mVideoRecorder->GetInterface(IID_IVideoSource) : NULL);

  ISeiSender *seiSender = (ISeiSender *)(mSeiSender ?
      mSeiSender->GetInterface(IID_ISeiSender) : NULL);

  IEventSender *eventSender = (IEventSender *)(mEventSender ?
       mEventSender->GetInterface(IID_IEventSender) : NULL);

  if (aSrc->Start () != ME_OK) {
     ERROR ("Failed to start audio capture!");
     return ME_ERROR;
  }

  if (vSrc->Start () != ME_OK) {
     ERROR ("Failed to start video recorder!");
     return ME_ERROR;
  }

  if (seiSender->Start () != ME_OK) {
     ERROR ("Failed to start seiSender!");
     return ME_ERROR;
  }

  if (eventSender->Start () != ME_OK) {
     ERROR ("Failed to start eventSender!");
     return ME_ERROR;
  }

  return ME_OK;
}

AM_ERR CInputRecord::StopAll()
{
  IAudioSource *aSrc = (IAudioSource*)(mAudioCapture ?
      mAudioCapture->GetInterface(IID_IAudioSource) : NULL);

  IVideoSource *vSrc = (IVideoSource*)(mVideoRecorder ?
      mVideoRecorder->GetInterface(IID_IVideoSource) : NULL);

  ISeiSender *seiSender = (ISeiSender *)(mSeiSender ?
      mSeiSender->GetInterface(IID_ISeiSender) : NULL);

  IEventSender *eventSender = (IEventSender *)(mEventSender ?
       mEventSender->GetInterface(IID_IEventSender) : NULL);

  if (seiSender->Stop () != ME_OK) {
     ERROR ("Failed to stop seiSender!");
     return ME_ERROR;
  }

  if (eventSender->Stop () != ME_OK) {
     ERROR ("Failed to stop eventSender!");
     return ME_ERROR;
  }

  if (vSrc->Stop () != ME_OK) {
     ERROR ("Failed to stop video recorder!");
     return ME_ERROR;
  }

  if (aSrc->Stop () != ME_OK) {
     ERROR ("Failed to stop audio capture!");
     return ME_ERROR;
  }

  if (AM_UNLIKELY(mAudioStreamMap == 0)) {
    IAudioEncoder *aEnc = (IAudioEncoder*)
        (mAudioEncoder->GetInterface(IID_IAudioEncoder));
    if (aEnc->Stop() != ME_OK) {
      ERROR("Failed to stop audio encoder!");
    }
  }

  return ME_OK;
}

AM_ERR CInputRecord::StartVideo (AM_UINT streamid)
{
  IVideoSource *vSrc = (IVideoSource*)
      (mVideoRecorder->GetInterface(IID_IVideoSource));
  return vSrc ? vSrc->Start(streamid) : ME_ERROR;
}

AM_ERR CInputRecord::StopVideo (AM_UINT streamid)
{
  IVideoSource *vSrc = (IVideoSource*)
      (mVideoRecorder->GetInterface(IID_IVideoSource));
  return vSrc ? vSrc->Stop(streamid) : ME_ERROR;
}

AM_ERR  CInputRecord::SendSEI(void *pUserData, AM_U32 len)
{
  ISeiSender *seiSender = (ISeiSender *)
      (mSeiSender->GetInterface(IID_ISeiSender));
  return seiSender ? seiSender->SendSEI(pUserData, len) : ME_ERROR;
}

AM_ERR  CInputRecord::SendEvent(CPacket::AmPayloadAttr eventType)
{
  IEventSender *eventSender = (IEventSender *)
       (mEventSender->GetInterface(IID_IEventSender));
  return eventSender ? eventSender->SendEvent(eventType) : ME_ERROR;
}
