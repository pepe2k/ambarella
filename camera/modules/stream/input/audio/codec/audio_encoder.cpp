/*******************************************************************************
 * audio_encoder.cpp
 *
 * Histroy:
 *   2012-9-26 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
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

#include "audio_codec_info.h"
#include "audio_codec.h"
#include "audio_codec_aac.h"
#include "audio_codec_opus.h"
#include "audio_codec_pcm.h"
#include "audio_codec_bpcm.h"
#include "audio_codec_g726.h"

#include "audio_encoder_if.h"
#include "audio_encoder.h"

#define PACKET_POOL_SIZE 32

IPacketFilter* create_audio_encoder_filter(IEngine* engine)
{
  return CAudioEncoder::Create(engine);
}

CAudioEncoder* CAudioEncoder::Create(IEngine *pEngine,
                                     bool RTPriority,
                                     int priority)
{
  CAudioEncoder *result = new CAudioEncoder(pEngine);

  if (AM_UNLIKELY(result && (ME_OK != result->Construct(RTPriority,
                                                        priority)))) {
    delete result;
    result = NULL;
  }

  return result;
}

CAudioEncoder::~CAudioEncoder()
{
  delete mAudioCodec;
  AM_DELETE(mInputPin);
  AM_DELETE(mOutputPin);
  AM_RELEASE(mPacketPool);
}

AM_ERR CAudioEncoder::Construct(bool RTPriority, int priority)
{
  return inherited::Construct(RTPriority, priority);
}

void CAudioEncoder::OnRun()
{
  CPacketQueueInputPin *inputPin = NULL;
  CPacket              *inputPkt = NULL;
  bool                  hasError = false;
  mRun            = true;
  mAudioStreamNum = 0;
  CmdAck(ME_OK);

  while (mRun) {
    if (AM_UNLIKELY(!WaitInputBuffer(inputPin, inputPkt))) {
      if (AM_LIKELY(false == mRun)) {
        NOTICE("Stop is called!");
      } else {
        NOTICE("Filter aborted!");
      }
      break;
    }

    if (AM_LIKELY(inputPkt->GetAttr() == CPacket::AM_PAYLOAD_ATTR_AUDIO)) {
      switch(inputPkt->GetType()) {
        case CPacket::AM_PAYLOAD_TYPE_INFO: {
          mRun = (ME_OK == OnInfo(inputPkt));
        } break;
        case CPacket::AM_PAYLOAD_TYPE_DATA: {
          mRun = (ME_OK == OnData(inputPkt));
          hasError = !mRun;
        }break;
        case CPacket::AM_PAYLOAD_TYPE_EOS: {
          mRun = (ME_OK == OnEOS(inputPkt));
        }break;
        default: {
          ERROR("Invalid packet type!");
        }break;
      }
      inputPkt->Release();
    } else {
      ERROR("Audio encoder not receiving audio packet!");
    }
  }
  /* Finalize audio codec */
  mAudioCodec->FiniCodec();
  if (AM_UNLIKELY(hasError)) {
    PostEngineMsg(IEngine::MSG_ABORT);
  }
  INFO("AudioEncoder exits mainloop!");
}

AM_ERR CAudioEncoder::SetAudioCodecType(AM_UINT type, void *codecInfo)
{
  AM_ERR ret = ME_OK;
  do {
    delete mAudioCodec;
    mAudioCodec = NULL;
    if (AM_LIKELY(mOutputPin)) {
      mOutputPin->SetBufferPool(NULL);
    }
    AM_RELEASE(mPacketPool);
    mPacketPool = NULL;
    AM_DELETE(mInputPin);
    AM_DELETE(mOutputPin);
    switch(type) {
      case AM_AUDIO_CODEC_AAC: {
        mCodecType = (AmAudioCodecType)type;
        mAudioCodec = new CAudioCodecAac();
        mAudioDataSize = 6144/8*2+100; /* From sample code of libaacenc.a */
        memcpy(&mCodecInfo.codec_aac, codecInfo,
               sizeof(mCodecInfo.codec_aac));
        DEBUG("AudioCodecType is AAC!");
      }break;
      case AM_AUDIO_CODEC_OPUS: {
        mCodecType = (AmAudioCodecType)type;
        mAudioCodec = new CAudioCodecOpus();
        mAudioDataSize = 4096; /* Opus Maximum output size */
        memcpy(&mCodecInfo.codec_opus, codecInfo,
               sizeof(mCodecInfo.codec_opus));
        DEBUG("AudioCodecTypee is OPUS!");
      }break;
      case AM_AUDIO_CODEC_PCM: {
        mCodecType = (AmAudioCodecType)type;
        mAudioCodec = new CAudioCodecPcm();
        mAudioDataSize = 1024*8;
        memcpy(&mCodecInfo.codec_pcm, codecInfo,
               sizeof(mCodecInfo.codec_pcm));
        DEBUG("AudioCodecType is PCM!");
      }break;
      case AM_AUDIO_CODEC_BPCM: {
        mCodecType = (AmAudioCodecType)type;
        mAudioCodec = new CAudioCodecBpcm();
        mAudioDataSize = 1024*8;
        memcpy(&mCodecInfo.codec_bpcm, codecInfo,
               sizeof(mCodecInfo.codec_bpcm));
        DEBUG("AudioCodecType is BPCM!");
      }break;
      case AM_AUDIO_CODEC_G726: {
        mCodecType = (AmAudioCodecType)type;
        mAudioCodec = new CAudioCodecG726();
        mAudioDataSize = 512; /* Larger than G726-40 */
        memcpy(&mCodecInfo.codec_g726, codecInfo,
               sizeof(mCodecInfo.codec_g726));
        DEBUG("AudioCodecType is G726!");
      }break;
      default: {
        ERROR("Unknown audio codec type!");
        mCodecType = AM_AUDIO_CODEC_NONE;
        mAudioCodec = NULL;
        mAudioDataSize = 0;
        ret = ME_ERROR;
        DEBUG("AudioCodecType is NONE!");
      }break;
    }
    if (AM_LIKELY(mAudioCodec)) {
      if (AM_UNLIKELY(NULL ==
          (mInputPin = CAudioEncoderInput::Create(this)))) {
        ERROR("Failed to create audio encoder input pin!");
        ret = ME_NO_MEMORY;
        break;
      }
      if (AM_UNLIKELY(NULL ==
          (mOutputPin = CAudioEncoderOutput::Create(this)))) {
        ERROR("Failed to create audio encoder output pin!");
        ret = ME_NO_MEMORY;
        break;
      }
      if (AM_UNLIKELY(NULL == (mPacketPool = CFixedPacketPool::Create(
          "AudioEncoderPktPool", PACKET_POOL_SIZE, mAudioDataSize)))) {
        ERROR("Failed to create packet pool for audio encoder!");
        ret = ME_NO_MEMORY;
        break;
      } else {
        mOutputPin->SetBufferPool(mPacketPool);
        DEBUG("AudioEncoder Output Pin is %p, packet pool is %p!",
              mOutputPin, mPacketPool);
      }
    } else {
      ret = ME_NO_MEMORY;
    }
  } while (0);

  return ret;
}

void CAudioEncoder::CalculateEncoderInputSize()
{
  AM_AUDIO_INFO &audioInfo = mCodecInfo.audio_info;
  switch(mCodecType) {
    case AM_AUDIO_CODEC_AAC: {
      switch(mCodecInfo.codec_aac.enc_mode) {
        default:
        case 0: { /* AAC_Plain   */
          mEncoderInputSize = 1024 * audioInfo.channels * audioInfo.sampleSize;
        }break;
        case 1:   /* AAC_Plus    */
        case 3: { /* AAC_Plus_PS */
          mEncoderInputSize = 2048 * audioInfo.channels * audioInfo.sampleSize;
        }break;
      }
    }break;
    case AM_AUDIO_CODEC_G726: {
      /* G.726 needs 20ms/40ms/60ms/80ms frame size, here we use 80ms */
      mEncoderInputSize = (80 * audioInfo.sampleRate) / 1000 *
          audioInfo.channels * audioInfo.sampleSize;
    }break;
    case AM_AUDIO_CODEC_OPUS: {
      /* Opus needs 20ms frame size */
      AM_UINT baseSize = (20 * audioInfo.sampleRate) / 1000 *
          audioInfo.channels * audioInfo.sampleSize;
      while ((mEncoderInputSize + baseSize) <= 8192) {
        mEncoderInputSize += baseSize;
      }
    }break;
    case AM_AUDIO_CODEC_PCM:
    case AM_AUDIO_CODEC_BPCM:
    default: {
      mEncoderInputSize = 4096;
    }break;
  }
  INFO("Audio codec needs %u bytes input data.", mEncoderInputSize);
}

inline AM_ERR CAudioEncoder::OnInfo(CPacket *packet)
{
  AM_ERR ret = ME_ERROR;

  if (AM_UNLIKELY(false == mAudioCodec->IsInitialized())) {
    memcpy(&mCodecInfo.audio_info, packet->GetDataPtr(),
           sizeof(mCodecInfo.audio_info));
    if (AM_LIKELY(mAudioCodec->InitCodec(
        mCodecInfo, CAudioCodec::AM_AUDIO_CODEC_MODE_ENCODE))) {
      CalculateEncoderInputSize();
    }
  }

  if (AM_LIKELY(mAudioCodec->IsInitialized())) {
    CPacket *outputPkt = NULL;
    if (AM_UNLIKELY(!mOutputPin->AllocBuffer(outputPkt))) {
      ERROR("Failed to allocate output packet!");
    } else {
      AM_AUDIO_INFO *audioInfo = &mCodecInfo.audio_info;
      AM_AUDIO_INFO *outAinfo  = ((AM_AUDIO_INFO*)outputPkt->GetDataPtr());
      AM_UINT   origPktPtsIncr = audioInfo->pktPtsIncr;
      memcpy(outAinfo, audioInfo, sizeof(AM_AUDIO_INFO));
      outputPkt->SetType(CPacket::AM_PAYLOAD_TYPE_INFO);
      outputPkt->SetAttr(CPacket::AM_PAYLOAD_ATTR_AUDIO);
      outputPkt->SetDataSize(sizeof(AM_AUDIO_INFO));
      outputPkt->SetStreamId(packet->GetStreamId());
      outputPkt->SetFrameType((AM_U16)mCodecType);
      outAinfo->pktPtsIncr =
          origPktPtsIncr * mEncoderInputSize / outAinfo->chunkSize;
      mOutputPin->SendBuffer(outputPkt);
      INFO("\nAudio %s Information for stream%hu:\n"
          "          Sample rate: %u\n"
          "          Sample size: %u\n"
          "             Channels: %u\n"
          "               Format: %u\n"
          "  PTS Increment / Pkt: %u",
          mAudioCodec->GetCodecName(),
          outputPkt->GetStreamId(),
          audioInfo->sampleRate,
          audioInfo->sampleSize,
          audioInfo->channels,
          audioInfo->format,
          outAinfo->pktPtsIncr);
      INFO("Successfully initialized %s audio codec!",
           mAudioCodec->GetCodecName());
      ++ mAudioStreamNum;
      ret = ME_OK;
    }
  } else {
    ERROR("Failed to initialize %s audio codec!", mAudioCodec->GetCodecName());
  }

  return ret;
}

inline AM_ERR CAudioEncoder::OnData(CPacket *packet)
{
  AM_ERR ret = ME_ERROR;
  CPacket *outputPkt = NULL;
  if (AM_UNLIKELY(!mOutputPin->AllocBuffer(outputPkt))) {
    if (AM_LIKELY(false == mRun)) {
      NOTICE("Stop is called!");
    } else {
      NOTICE("Failed to allocate output packet!");
    }
  } else {
    AM_UINT count  = packet->GetDataSize() / mEncoderInputSize;
    AM_UINT outSize = 0;
    AM_UINT totalOutSize = 0;
    bool encodeOk = true;
    outputPkt->SetAttr(CPacket::AM_PAYLOAD_ATTR_AUDIO);
    outputPkt->SetFrameCount(count);
    for (AM_UINT i = 0; i < count; ++ i) {
      AM_U8 *inputPtr = packet->GetDataPtr() + i * mEncoderInputSize;
      if (AM_LIKELY(mAudioCodec->encode(inputPtr, mEncoderInputSize,
                                        outputPkt->GetDataPtr() + totalOutSize,
                                        &outSize))) {
        totalOutSize += outSize;
      } else {
        encodeOk = false;
        ERROR("%s codec encode error!", mAudioCodec->GetCodecName());
        break;
      }
    }
    if (AM_LIKELY(encodeOk)) {
      outputPkt->SetDataSize(totalOutSize);
      outputPkt->SetPTS(packet->GetPTS());
      outputPkt->SetStreamId(packet->GetStreamId());
      outputPkt->SetType(CPacket::AM_PAYLOAD_TYPE_DATA);
      outputPkt->SetFrameType((AM_U16)mCodecType);
      mOutputPin->SendBuffer(outputPkt);
      ret = ME_OK;
    } else {
      outputPkt->Release();
    }
  }

  return ret;
}

inline AM_ERR CAudioEncoder::OnEOS(CPacket *packet)
{
  AM_ERR ret = ME_ERROR;
  CPacket *outputPkt = NULL;
  if (AM_UNLIKELY(!mOutputPin->AllocBuffer(outputPkt))) {
    if (AM_LIKELY(false == mRun)) {
      NOTICE("Stop is called!");
    } else {
      NOTICE("Failed to allocate output packet!");
    }
  } else {
    outputPkt->SetDataSize(0);
    outputPkt->SetPTS(packet->GetPTS());
    outputPkt->SetStreamId(packet->GetStreamId());
    outputPkt->SetType(CPacket::AM_PAYLOAD_TYPE_EOS);
    outputPkt->SetAttr(CPacket::AM_PAYLOAD_ATTR_AUDIO);
    outputPkt->SetFrameType((AM_U16)mCodecType);
    mOutputPin->SendBuffer(outputPkt);
    -- mAudioStreamNum;
    mRun = (0 != mAudioStreamNum);
    INFO("AudioEncoder send EOS to stream%hu!", packet->GetStreamId());
    ret = mRun ? ME_OK : ME_CLOSED;
  }

  return ret;
}

CAudioEncoderInput* CAudioEncoderInput::Create(CPacketFilter *pFilter)
{
  CAudioEncoderInput *result = new CAudioEncoderInput(pFilter);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }
  return result;
}

CAudioEncoderOutput* CAudioEncoderOutput::Create(CPacketFilter *pFilter)
{
  CAudioEncoderOutput* result = new CAudioEncoderOutput(pFilter);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }

  return result;
}
