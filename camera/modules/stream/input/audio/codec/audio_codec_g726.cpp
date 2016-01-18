/*******************************************************************************
 * audio_codec_g726.cpp
 *
 * History:
 *   2013年8月20日 - [ypchang] created file
 *
 * Copyright (C) 2008-2013, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_types.h"
#include "utilities/am_define.h"
#include "am_media_info.h"

#include "audio_codec_info.h"
#include "audio_codec.h"
#include "audio_codec_g726.h"
#include "g726.h"

#define G726_DECODE_BUFFER 4096

enum Rate {
  Rate16kBits = 2,
  Rate24kBits = 3,
  Rate32kBits = 4,
  Rate40kBits = 5
};
bool CAudioCodecG726::InitCodec(AudioCodecInfo &codecInfo,
                                AmAudioCodecMode mode)
{
  mAudioInfo = &codecInfo.audio_info;
  if (AM_LIKELY(!mIsInitialized && (NULL == mG726State))) {
    switch(mode) {
      case AM_AUDIO_CODEC_MODE_ENCODE: {
        mAudioInfo->format = MF_NULL;
        mIsInitialized = ((mAudioInfo->channels == 1) &&
                          (mAudioInfo->sampleRate == 8000));
        if (AM_LIKELY(mIsInitialized)) {
          mBitsPerSample = codecInfo.codec_g726.g726_rate;
          switch(codecInfo.codec_g726.g726_rate) {
            case Rate40kBits: {
              codecInfo.audio_info.format = MF_G726_40;
            }break;
            case Rate32kBits: {
              codecInfo.audio_info.format = MF_G726_32;
            }break;
            case Rate24kBits: {
              codecInfo.audio_info.format = MF_G726_24;
            }break;
            case Rate16kBits: {
              codecInfo.audio_info.format = MF_G726_16;
            }break;
            default: {
              mBitsPerSample = Rate40kBits;
              codecInfo.audio_info.format = MF_G726_40;
              WARN("Invalid bitrate, reset to 40k!");
            }break;
          }
        } else {
          ERROR("G.726 only supports Mono audio with 8000 sample rate, "
                "but input audio is %u channels, sample rate: %u",
                mAudioInfo->channels, mAudioInfo->sampleRate);
        }
      }break;
      case AM_AUDIO_CODEC_MODE_DECODE: {
        mIsInitialized = ((mAudioInfo->channels == 1) &&
                          (mAudioInfo->sampleRate == 8000));
        if (AM_LIKELY(mIsInitialized)) {
          mAudioInfo->sampleSize = sizeof(int16_t);
          switch(mAudioInfo->format) {
            case MF_G726_40: mBitsPerSample = Rate40kBits; break;
            case MF_G726_32: mBitsPerSample = Rate32kBits; break;
            case MF_G726_24: mBitsPerSample = Rate24kBits; break;
            case MF_G726_16: mBitsPerSample = Rate16kBits; break;
            default: {
              ERROR("Invalid G.726 bitrate!");
              mIsInitialized = false;
            }break;
          }
          if (AM_LIKELY(mIsInitialized)) {
            INFO("Source G.726 bitrate is %ukb/s", mBitsPerSample * 8);
          }
        } else {
          ERROR("Invalid G.726 audio, which has %u channels, sample rate: %u",
                mAudioInfo->channels, mAudioInfo->sampleRate);
        }
      }break;
      default: ERROR("Unknown mode!"); break;
    }
    if (AM_LIKELY(mIsInitialized)) {
      mG726State = g726_init(NULL, (mAudioInfo->sampleRate * mBitsPerSample),
                             G726_ENCODING_LINEAR, G726_PACKING_LEFT);
      if (AM_UNLIKELY(!mG726State)) {
        mIsInitialized = false;
        ERROR("Failed to create G726 codec!");
      }
    }
  }

  return mIsInitialized;
}
bool CAudioCodecG726::FiniCodec()
{
  if (AM_LIKELY(mG726State)) {
    g726_release(mG726State);
  }
  mG726State = NULL;
  mIsInitialized = false;
  return true;
}

AM_UINT CAudioCodecG726::encode(AM_U8   *input,
                                AM_UINT  inDataSize,
                                AM_U8   *output,
                                AM_UINT *outDataSize)
{
  *outDataSize = g726_encode(mG726State, output,
                             (int16_t*)input, (inDataSize/sizeof(int16_t)));
  return *outDataSize;
}

AM_UINT CAudioCodecG726::decode(AM_U8   *input,
                                AM_UINT  inDataSize,
                                AM_U8   *output,
                                AM_UINT *outDataSize)
{
  AM_INT usedSmp = g726_decode(mG726State, (int16_t*)output, input, inDataSize);
  *outDataSize = (mAudioInfo->sampleSize * usedSmp);

  return (usedSmp * mBitsPerSample / 8);
}
