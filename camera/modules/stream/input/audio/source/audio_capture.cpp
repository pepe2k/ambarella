/*******************************************************************************
 * audio_capture.cpp
 *
 * Histroy:
 *   2012-9-18 - [ypchang] created file
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
#include "audio_device.h"
#include "audio_capture.h"

/* Maximum frame size needed */
#define MAX_PCM_FRAME_SIZE (2048 * sizeof(AM_S16) * 2) /* 8192 bytes */
#define PACKET_POOL_SIZE   128

IPacketFilter* create_audio_capture_filter(IEngine *pEngin)
{
  return CAudioCapture::Create(pEngin);
}

CAudioCapture* CAudioCapture::Create(IEngine *pEngin,
                                     bool RTPriority,
                                     int priority)
{
  CAudioCapture *audioCap = new CAudioCapture(pEngin);
  if (AM_UNLIKELY(audioCap && ( ME_OK != audioCap->Construct(RTPriority,
                                                             priority)))) {
    delete audioCap;
    audioCap = NULL;
  }

  return audioCap;
}

CAudioCapture::CAudioCapture(IEngine *pEngine):
  inherited(pEngine, "AudioCapture"),
  mpAdev(NULL),
  mpOutPin(NULL),
  mpPktPool(NULL),
  mpEvent(NULL),
  mpPacketQ(NULL),
  mbRun(false),
  mIsAdevStarted(false),
  mAudioStreamMap(0),
  mAvSyncMap(0)
{
}

CAudioCapture::~CAudioCapture()
{
  AM_DELETE(mpOutPin);
  AM_DELETE(mpEvent);
  AM_RELEASE(mpPktPool);
  delete mpAdev;
  delete mpPacketQ;
}

AM_ERR CAudioCapture::Construct(bool RTPriority, int priority)
{
  AM_ERR ret = inherited::Construct(RTPriority, priority);
  if (AM_LIKELY(ret == ME_OK)) {
    if (AM_UNLIKELY(NULL ==
        (mpPktPool = CFixedPacketPool::Create("AudioCapturePktPool",
                                              PACKET_POOL_SIZE,
                                              MAX_PCM_FRAME_SIZE)))) {
      ERROR("Failed to create packet pool!");
      ret = ME_NO_MEMORY;
    } else if (AM_UNLIKELY(NULL ==
        (mpOutPin = CAudioCaptureOutput::Create(this)))) {
      ERROR("Failed to create capture output!");
      ret = ME_NO_MEMORY;
    } else if (AM_UNLIKELY(NULL == (mpPacketQ = new PacketQueue()))) {
      ERROR("Failed to create packet queue!");
      ret = ME_NO_MEMORY;
    } else if (AM_UNLIKELY(NULL ==
        (mpAdev = CPulseAudio::Create(this, mpPacketQ)))) {
      /*
       * Todo: InputModule should provide API to set audio driver type
       * which is alsa or pulse, auido device should be created according to
       * the audio driver type, here use pulse audio for quick implementation
       */
      ERROR("Failed to create CPulseAudio!");
      ret = ME_NO_MEMORY;
    } else if (AM_UNLIKELY(NULL == (mpEvent = CEvent::Create()))) {
      ERROR("Failed to create event!");
      ret = ME_NO_MEMORY;
    } else {
      ret = ME_OK;
    }
    if (AM_LIKELY(mpOutPin)) {
      /* Reserve packets for audio EOS */
      for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
        CPacket *audioEos = NULL;
        if (mpPktPool->AllocBuffer(audioEos, 0)) {
          mpPacketQ->push(audioEos);
        }
      }
      mpOutPin->SetBufferPool(mpPktPool);
    }
  } else {
    ERROR("Failed to initialize CPacketActiveFilter!");
  }

  return ret;
}

void CAudioCapture::SendBuffer(CPacket *packet)
{
  if (AM_LIKELY(packet)) {
    mpOutPin->SendBuffer(packet);
  }
}

AM_ERR CAudioCapture::Start()
{
  AM_ERR ret = ME_OK;
  if (AM_LIKELY(false == mIsAdevStarted)) {
    mbRun = true;
    mpEvent->Signal();
    mIsAdevStarted = (ME_OK == (ret = mpAdev->Start()));
  }

  return ret;
}

AM_ERR CAudioCapture::Stop()
{
  if (AM_UNLIKELY(!mbRun)) {
    mpEvent->Signal(); /* If mbRun is false, should unlock OnRun first */
  }
  mbRun = false;
  mpEvent->Wait();
  mpAdev->Stop();
  mIsAdevStarted = false;
  return inherited::Stop();
}

void CAudioCapture::OnRun()
{
  CmdAck(ME_OK);
  mpEvent->Clear();
  mpEvent->Wait();
  while (mbRun) {
    CPacket *audio = NULL;
    while (mbRun && (mpOutPin->GetAvailBufNum() > 0) &&
        mpOutPin->AllocBuffer(audio)) {
      mpPacketQ->push(audio);
    }
    if (AM_LIKELY(!mbRun)) {
      NOTICE("Stop is called!");
      /* Get all the available packet to send remaining data and EOS packet */
      while ((mpOutPin->GetAvailBufNum() > 0) &&
              mpOutPin->AllocBuffer(audio)) {
        mpPacketQ->push(audio);
      }
    } else {
      usleep(10000);
    }
  }
  mpEvent->Signal();

  INFO("AudioCapture exit mainloop!");
}
