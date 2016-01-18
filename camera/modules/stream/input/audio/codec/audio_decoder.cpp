/*******************************************************************************
 * audio_decoder.cpp
 *
 * History:
 *   2013-3-18 - [ypchang] created file
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

#include "audio_codec_info.h"
#include "audio_codec.h"
#include "audio_codec_aac.h"
#include "audio_codec_opus.h"
#include "audio_codec_pcm.h"
#include "audio_codec_bpcm.h"
#include "audio_codec_g726.h"

#include "audio_decoder_if.h"
#include "audio_decoder.h"

#define PACKET_POOL_SIZE 32

IPacketFilter* create_audio_decoder_filter(IEngine* engine)
{
  return CAudioDecoder::Create(engine);
}

class CAudioDecoderPktPool: public CSimplePacketPool
{
    typedef CSimplePacketPool inherited;
  public:
    static CAudioDecoderPktPool* Create(const char* name,
                                    AM_UINT count)
    {
      CAudioDecoderPktPool* result = new CAudioDecoderPktPool(name);
      if (AM_UNLIKELY(result && (ME_OK != result->Construct(count)))) {
        delete result;
        result = NULL;
      }

      return result;
    }

  public:
    bool AllocBuffer(CPacket*& packet, AM_UINT size)
    {
      bool ret = (mDataMemory != NULL);

      if (AM_LIKELY(ret)) {
        ret = CPacketPool::AllocBuffer(packet, size);
      } else {
        ERROR("Data memory is not set!");
      }

      return ret;
    }

    AM_ERR SetBuffer(AM_U8 *buffer, AM_UINT dataSize)
    {
      AM_ERR ret = ME_ERROR;
      if (AM_LIKELY(buffer && (dataSize > 0))) {
        mDataMemory = buffer;
        for (AM_UINT i = 0; i < mPacketCount; ++ i) {
          mpPacketMemory[i].mPayload->mData.mBuffer =
              (mDataMemory + (i * dataSize));
        }

        ret = ME_OK;
      }

      return ret;
    }

  private:
    CAudioDecoderPktPool(const char* name) :
      inherited(name),
      mPayloadMemory(NULL),
      mDataMemory(NULL),
      mPacketCount(0){}
    virtual ~CAudioDecoderPktPool()
    {
      delete[] mPayloadMemory;
    }
    AM_ERR Construct(AM_UINT count)
    {
      AM_ERR ret = inherited::Construct(count);

      if (AM_LIKELY(ME_OK == ret)) {
        mPacketCount = count;
        mPayloadMemory = new CPacket::Payload[count];
        if (AM_LIKELY(mPayloadMemory)) {
          for (AM_UINT i = 0; i < mPacketCount; ++ i) {
            mpPacketMemory[i].mPayload = &mPayloadMemory[i];
          }
        } else {
          ret = ME_NO_MEMORY;
        }
      }
      return ret;
    }

  private:
      CPacket::Payload *mPayloadMemory;
      AM_U8            *mDataMemory;
      AM_UINT           mPacketCount;
};

CAudioDecoder* CAudioDecoder::Create(IEngine *engine,
                                     bool RTPriority,
                                     int priority)
{
  CAudioDecoder *result = new CAudioDecoder(engine);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct(RTPriority,
                                                        priority)))) {
    delete result;
    result = NULL;
  }

  return result;
}

CAudioDecoder::~CAudioDecoder()
{
  AM_DELETE(mInputPin);
  AM_DELETE(mOutputPin);
  AM_RELEASE(mPacketPool);
  delete[] mBuffer;
}

AM_ERR CAudioDecoder::Construct(bool RTPriority, int priority)
{
  AM_ERR ret = inherited::Construct(RTPriority, priority);

  if (AM_LIKELY(ME_OK == ret)) {
    do {
      if (AM_UNLIKELY(NULL ==
          (mInputPin = CAudioDecoderInput::Create(this)))) {
        ERROR("Failed to create audio decoder input pin!");
        ret = ME_NO_MEMORY;
        break;
      }

      if (AM_UNLIKELY(NULL ==
          (mOutputPin = CAudioDecoderOutput::Create(this)))) {
        ERROR("Failed to create audio decoder output pin!");
        ret = ME_NO_MEMORY;
        break;
      }

      if (AM_UNLIKELY(NULL ==
          (mPacketPool = CAudioDecoderPktPool::Create(
              "AudioDecoderPktPool", PACKET_POOL_SIZE)))) {
        ERROR("Failed to create audio decoder packet pool!");
        ret = ME_NO_MEMORY;
        break;
      } else {
        mOutputPin->SetBufferPool(mPacketPool);
      }
    } while(0);
  }

  return ret;
}

void CAudioDecoder::OnRun()
{
  CPacketQueueInputPin *inputPin = NULL;
  CPacket              *inputPkt = NULL;
  bool                 needBreak = false;
  CmdAck(ME_OK);
  mRun = true;

  while (mRun) {
    if (AM_UNLIKELY(!WaitInputBuffer(inputPin, inputPkt))) {
      if (AM_LIKELY(!mRun)) {
        NOTICE("Stop is called!");
      } else {
        NOTICE("Filter aborted!");
      }
      break;
    }
    if (AM_UNLIKELY(!inputPkt)) {
      ERROR("Invalid packet!");
      continue;
    }

    if (AM_LIKELY(inputPkt->GetAttr() == CPacket::AM_PAYLOAD_ATTR_AUDIO)) {
      switch(inputPkt->GetType()) {
        case CPacket::AM_PAYLOAD_TYPE_INFO: {
          needBreak = (ME_OK != OnInfo(inputPkt));
        }break;
        case CPacket::AM_PAYLOAD_TYPE_DATA: {
          needBreak = (ME_OK != OnData(inputPkt));
        }break;
        case CPacket::AM_PAYLOAD_TYPE_EOF: {
          needBreak = (ME_OK != OnEOF(inputPkt));
        }break;
        default: {
          ERROR("Invalid packet type!");
        }break;
      }
      inputPkt->Release();
      if (AM_UNLIKELY(needBreak && mRun)) {
        break;
      }
    } else {
      ERROR("Audio decoder received non-audio packet!");
    }
  }
  /* Finalize audio codec */
  if (AM_LIKELY(mAudioCodec)) {
    mAudioCodec->FiniCodec();
  }

  if (AM_UNLIKELY(needBreak && mRun)) {
    NOTICE("AudioDecoder post MSG_ABORT!");
    PostEngineMsg(IEngine::MSG_ABORT);
  }

  INFO("AudioDecoder exits mainloop!");
}

AM_ERR CAudioDecoder::SetAudioCodecType(AM_UINT type)
{
  AM_ERR ret = ME_OK;
  do {
    delete[] mBuffer;
    mBuffer = NULL;
    switch(type) {
      case AM_AUDIO_CODEC_AAC: {
        mCodecType = (AmAudioCodecType)type;
        mAudioCodec = new CAudioCodecAac();
        mAudioDataSize = AAC_DEC_OUT_BUF_SIZE;
        INFO("AudioCodecType is AAC!");
      }break;
      case AM_AUDIO_CODEC_PCM: {
        mCodecType = (AmAudioCodecType)type;
        mAudioCodec = new CAudioCodecPcm();
        mAudioDataSize = 1024*8;
        INFO("AudioCodecType is PCM!");
      }break;
      case AM_AUDIO_CODEC_BPCM: {
        mCodecType = (AmAudioCodecType)type;
        mAudioCodec = new CAudioCodecBpcm();
        mAudioDataSize = 1024*8;
        INFO("AudioCodecType is BPCM!");
      }break;
      case AM_AUDIO_CODEC_OPUS: {
        mCodecType = (AmAudioCodecType)type;
        mAudioCodec = new CAudioCodecOpus();
        mAudioDataSize = 120*48000/1000*sizeof(AM_U16)*2;
        INFO("AudioCodecType is OPUS!");
      }break;
      case AM_AUDIO_CODEC_G726: {
        AM_UINT bits = 2;
        mCodecType = (AmAudioCodecType)type;
        mAudioCodec = new CAudioCodecG726();
        switch(mCodecInfo.audio_info.format) {
          case MF_G726_40: bits = 5; break;
          case MF_G726_32: bits = 4; break;
          case MF_G726_24: bits = 3; break;
          case MF_G726_16: bits = 2; break;
          default:break;
        }
        mAudioDataSize = round_up(sizeof(int16_t) * (960 * 8 / bits + 1), 4);
        INFO("AudioCodecType is G.726");
      }break;
      default: {
        ERROR("Unknown audio codec type!");
        mCodecType = AM_AUDIO_CODEC_NONE;
        mAudioCodec = NULL;
        mAudioDataSize = 0;
        ret = ME_ERROR;
      }break;
    }
    if (AM_LIKELY(ret == ME_OK)) {
      mBuffer = new AM_U8[mAudioDataSize * PACKET_POOL_SIZE];
      if (AM_UNLIKELY(!mBuffer)) {
        ret = ME_NO_MEMORY;
        ERROR("Failed to allocate buffer for %s decoder!",
              mAudioCodec ? mAudioCodec->GetCodecName() : "unknown");
      } else {
        ret = ((CAudioDecoderPktPool*)mPacketPool)->SetBuffer(mBuffer,
                                                              mAudioDataSize);
      }
    }
  } while(0);

  return ret;
}

inline AM_ERR CAudioDecoder::OnInfo(CPacket *packet)
{
  AM_ERR ret = ME_OK;
  AM_AUDIO_INFO *srcAudioInfo = (AM_AUDIO_INFO*)(packet->GetDataPtr());

  mIsInfoSent = false;
  if (AM_LIKELY((mSourceInfo.channels != srcAudioInfo->channels) ||
                (mSourceInfo.sampleRate != srcAudioInfo->sampleRate) ||
                (mCodecType != packet->GetFrameType()))) {
    delete mAudioCodec;
    mAudioCodec = NULL;
  }
  memcpy(&mSourceInfo, srcAudioInfo, sizeof(mSourceInfo));
  if (AM_LIKELY(!mAudioCodec)) {
    memcpy(&mCodecInfo.audio_info, srcAudioInfo, sizeof(mCodecInfo.audio_info));
    ret = SetAudioCodecType(packet->GetFrameType());
  }
  if (AM_UNLIKELY((ME_OK == ret) && !mAudioCodec->IsInitialized())) {
    mAudioCodec->InitCodec(mCodecInfo,
                           CAudioCodec::AM_AUDIO_CODEC_MODE_DECODE);
  }

  if (AM_LIKELY(mAudioCodec->IsInitialized())) {
    INFO("\nAudio %s Information for stream%hu:\n"
        "                      Sample rate: %u\n"
        "                         Channels: %u\n"
        "                           Format: %u\n",
        mAudioCodec->GetCodecName(),
        packet->GetStreamId(),
        mSourceInfo.sampleRate,
        mSourceInfo.channels,
        mCodecInfo.audio_info.format);
    INFO("Sucessfully initialized %s audio codec for decoding!",
         mAudioCodec->GetCodecName());
    ret = ME_OK;
  } else {
    ERROR("Failed to initialize %s audio codec!", mAudioCodec->GetCodecName());
    ret = ME_ERROR;
  }

  return ret;
}

inline AM_ERR CAudioDecoder::OnData(CPacket *packet)
{
  AM_UINT dataSize = packet->GetDataSize();
  AM_UINT usedSize = 0;

  if (AM_LIKELY(dataSize > 0)) {
    while (dataSize > 0) {
      CPacket *outputPkt = NULL;
      if (AM_UNLIKELY(!mOutputPin->AllocBuffer(outputPkt))) {
        NOTICE("Failed to allocate output packet!"
               " Is buffer pool disabled?");
        mRun = false;
        break;
      } else {
        AM_UINT outSize = 0;

        usedSize = mAudioCodec->decode(packet->GetDataPtr() + usedSize,
                                       dataSize,
                                       outputPkt->GetDataPtr(),
                                       &outSize);
        if (AM_UNLIKELY(usedSize == 0xffffffff)) {
          outputPkt->Release();
          break;
        } else {
          if (AM_UNLIKELY(!mIsInfoSent)) {
            CPacket *audioInfoPkt = NULL;
            if (AM_UNLIKELY(!mOutputPin->AllocBuffer(audioInfoPkt))) {
              NOTICE("Failed to allocate output packet for audio info!"
                     " Is buffer pool disabled?");
              mRun = false;
              outputPkt->Release();
              break;
            } else {
              AM_AUDIO_INFO* audioInfo = &mCodecInfo.audio_info;
              AM_AUDIO_INFO* outAinfo  =
                  ((AM_AUDIO_INFO*)audioInfoPkt->GetDataPtr());
              memcpy(outAinfo, audioInfo, sizeof(AM_AUDIO_INFO));
              audioInfoPkt->SetType(CPacket::AM_PAYLOAD_TYPE_INFO);
              audioInfoPkt->SetAttr(CPacket::AM_PAYLOAD_ATTR_AUDIO);
              audioInfoPkt->SetDataSize(sizeof(AM_AUDIO_INFO));
              audioInfoPkt->SetStreamId(packet->GetStreamId());
              audioInfoPkt->SetFrameType((AM_U16)mCodecType);
              mOutputPin->SendBuffer(audioInfoPkt);
              mIsInfoSent = true;
            }
            switch(mCodecType) {
              case AM_AUDIO_CODEC_OPUS: {
                mAudioDataPreSkip =
                    packet->GetDataOffset() * sizeof(AM_U16) * 2;
              }break;
              default: {
                mAudioDataPreSkip = 0;
              }break;
            }
          }

          if (AM_UNLIKELY(mAudioDataPreSkip >= outSize)) {
            outputPkt->Release();
            mAudioDataPreSkip -= outSize;
          } else {
            dataSize -= usedSize;
            outputPkt->SetDataOffset(mAudioDataPreSkip);
            outputPkt->SetDataSize(outSize);
            outputPkt->SetPTS(packet->GetPTS());
            outputPkt->SetStreamId(packet->GetStreamId());
            outputPkt->SetType(CPacket::AM_PAYLOAD_TYPE_DATA);
            outputPkt->SetAttr(CPacket::AM_PAYLOAD_ATTR_AUDIO);
            outputPkt->SetFrameType((AM_U16)mCodecType);
            mOutputPin->SendBuffer(outputPkt);
            mAudioDataPreSkip = 0;
          }
        }
      }
    }
  } else {
    dataSize = (AM_UINT)-1;
  }

  return ((dataSize == 0) ? ME_OK : ME_ERROR);
}

inline AM_ERR CAudioDecoder::OnEOF(CPacket *packet)
{
  AM_ERR ret = ME_OK;
  CPacket *outputPkt = NULL;
  if (AM_UNLIKELY(!mOutputPin->AllocBuffer(outputPkt))) {
    if (AM_LIKELY(!mRun)) {
      NOTICE("Stop is called!");
    } else {
      NOTICE("Failed to allocate output packet!");
      ret = ME_ERROR;
    }
  } else {
    outputPkt->SetDataSize(0);
    outputPkt->SetPTS(0);
    outputPkt->SetStreamId(packet->GetStreamId());
    outputPkt->SetType(CPacket::AM_PAYLOAD_TYPE_EOF);
    outputPkt->SetAttr(CPacket::AM_PAYLOAD_ATTR_AUDIO);
    outputPkt->SetFrameType((AM_U16)mCodecType);
    mOutputPin->SendBuffer(outputPkt);
    INFO("AudioDecoder send EOF to stream%hu, remain buffer %u!",
         packet->GetStreamId(),
         mOutputPin->GetAvailBufNum());
  }

  return ret;
}

/* CAudioDecoderInput */
CAudioDecoderInput* CAudioDecoderInput::Create(CPacketFilter *filter)
{
  CAudioDecoderInput *result = new CAudioDecoderInput(filter);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }
  return result;
}

/* CAudioDecoderOutput */
CAudioDecoderOutput* CAudioDecoderOutput::Create(CPacketFilter *filter)
{
  CAudioDecoderOutput* result = new CAudioDecoderOutput(filter);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }

  return result;
}
