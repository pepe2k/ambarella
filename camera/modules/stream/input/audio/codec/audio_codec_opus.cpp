/*******************************************************************************
 * audio_codec_opus.cpp
 *
 * History:
 *   2013年7月15日 - [ypchang] created file
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
#include "am_media_info.h"

#include "audio_codec_info.h"
#include "audio_codec.h"
#include "audio_codec_opus.h"

CAudioCodecOpus::CAudioCodecOpus() :
  CAudioCodec(AM_AUDIO_CODEC_OPUS),
  mEncoder(NULL),
  mDecoder(NULL),
  mRepacketizer(NULL),
  mEncFrameSize(0),
  mEncFrameBytes(0)
{}

CAudioCodecOpus::~CAudioCodecOpus()
{
  FiniCodec();
}

bool CAudioCodecOpus::InitCodec(AudioCodecInfo &codecInfo,
                                AmAudioCodecMode mode)
{
  mAudioInfo = &codecInfo.audio_info;
  if (AM_LIKELY(!mIsInitialized)) {
    codecInfo.audio_info.format = MF_OPUS;
    switch(mode) {
      case AM_AUDIO_CODEC_MODE_ENCODE: {
        int error;
        mEncoder = opus_encoder_create(codecInfo.audio_info.sampleRate,
                                       codecInfo.audio_info.channels,
                                       OPUS_APPLICATION_AUDIO,
                                       &error);
        mEncFrameSize = 20 * codecInfo.audio_info.sampleRate / 1000;
        mEncFrameBytes = mEncFrameSize * codecInfo.audio_info.channels *
            codecInfo.audio_info.sampleSize;
        if (AM_LIKELY(mEncoder)) {
          AM_U32& bitrate = codecInfo.codec_opus.opus_avg_bitrate;
          AM_U32& complexity = codecInfo.codec_opus.opus_complexity;
          int ret = opus_encoder_ctl(mEncoder, OPUS_SET_BITRATE(bitrate));
          if (AM_UNLIKELY(ret != OPUS_OK)) {
            ERROR("Failed to set bitrate to %u: %s", bitrate,
                  opus_strerror(ret));
          } else {
            ret = opus_encoder_ctl(mEncoder, OPUS_SET_COMPLEXITY(complexity));
            if (AM_UNLIKELY(ret != OPUS_OK)) {
              ERROR("Failed to set complexity to %u: %s",
                    complexity, opus_strerror(ret));
            } else {
              mRepacketizer = opus_repacketizer_create();
              if (AM_LIKELY(mRepacketizer)) {
                mIsInitialized = true;
              } else {
                ERROR("Failed to create Opus repacketizer!");
              }
            }
          }
        } else {
          ERROR("Failed to create OPUS encoder: %s!", opus_strerror(error));
        }
      }break;
      case AM_AUDIO_CODEC_MODE_DECODE: {
        int error;
        codecInfo.audio_info.channels = 2;
        mDecoder = opus_decoder_create(
            codecInfo.audio_info.sampleRate, /* Always 48000 */
            codecInfo.audio_info.channels,   /* Always decode to stereo */
            &error);
        mIsInitialized = (mDecoder != NULL);
        if (AM_UNLIKELY(!mDecoder)) {
          ERROR("Failed to create OPUS decoder: %s!", opus_strerror(error));
        }
      }break;
      default: {
        ERROR("Invalid Opus codec mode!");
      }break;
    }
  } else {
    int ret = 0;
    INFO("Audio codec %s is already initialized, reset to initial state!",
         CodecTypeToName[mCodecType]);
    mEncFrameSize = 20 * codecInfo.audio_info.sampleRate / 1000;
    mEncFrameBytes = mEncFrameSize * codecInfo.audio_info.channels *
                codecInfo.audio_info.sampleSize;
    switch(mode) {
      case AM_AUDIO_CODEC_MODE_ENCODE:
      case AM_AUDIO_CODEC_MODE_DECODE: {
        if (AM_LIKELY(mEncoder)) {
          ret = opus_encoder_ctl(mEncoder, OPUS_RESET_STATE);
        }
        if (AM_LIKELY(mDecoder)) {
          ret = opus_decoder_ctl(mDecoder, OPUS_RESET_STATE);
        }
        mIsInitialized = (ret == OPUS_OK);
        if (AM_UNLIKELY(!mIsInitialized)) {
          ERROR("Failed to reset audio codec %s: %s",
                CodecTypeToName[mCodecType], opus_strerror(ret));
        }
      }break;
      default: {
        ERROR("Invalid mode!");
        ret = -1;
        mIsInitialized = false;
      }break;
    }
  }

  return mIsInitialized;
}

bool CAudioCodecOpus::FiniCodec()
{
  if (AM_LIKELY(mEncoder)) {
    opus_encoder_destroy(mEncoder);
    mEncoder = NULL;
  }

  if (AM_LIKELY(mDecoder)) {
    opus_decoder_destroy(mDecoder);
    mDecoder = NULL;
  }

  if (AM_LIKELY(mRepacketizer)) {
    opus_repacketizer_destroy(mRepacketizer);
    mRepacketizer = NULL;
  }
  mIsInitialized = false;
  return true;
}

AM_UINT CAudioCodecOpus::encode(AM_U8 *input, AM_UINT inDataSize,
                                AM_U8 *output, AM_UINT *outDataSize)
{
  *outDataSize = 0;
  if (AM_LIKELY(0 == (inDataSize % mEncFrameBytes))) {
    bool isOk = true;
    AM_U8 *out = mEncodeBuf;

    memset(out, 0, sizeof(mEncodeBuf));
    mRepacketizer = opus_repacketizer_init(mRepacketizer);
    for (AM_UINT i = 0; i < (inDataSize / mEncFrameBytes); ++ i) {
      const opus_int16* pcm = (opus_int16*)(input + i * mEncFrameBytes);
      int ret = opus_encode(mEncoder, pcm, mEncFrameSize, out, 4096);
      if (AM_LIKELY(ret > 0)) {
        int retval = opus_repacketizer_cat(mRepacketizer, out, ret);
        if (AM_UNLIKELY(retval != OPUS_OK)) {
          ERROR("Opus repacketizer error: %s", opus_strerror(retval));
          isOk = false;
          break;
        }
        out += ret;
      } else {
        ERROR("Opus encode error: %s", opus_strerror(ret));
        isOk = false;
        break;
      }
    }
    if (AM_LIKELY(isOk)) {
      int ret = opus_repacketizer_out(mRepacketizer, output, 4096);
      if (AM_LIKELY(ret > 0)) {
        *outDataSize = ret;
      } else {
        ERROR("Opus repacketizer error: %s", opus_strerror(ret));
      }
    }
  } else {
    ERROR("Invalid input data length: %u, must be n times of %u",
          inDataSize, mEncFrameBytes);
  }
  return *outDataSize;
}

AM_UINT CAudioCodecOpus::decode(AM_U8 *input,  AM_UINT inDataSize,
                                AM_U8 *output, AM_UINT *outDataSize)
{
  int totalFrames  = opus_packet_get_nb_frames(input, inDataSize);
  int totalSamples = (totalFrames > 0) ?
      totalFrames * opus_packet_get_samples_per_frame(input, 48000) : -1;
  AM_UINT ret = (AM_UINT)-1;
  if (AM_LIKELY((totalSamples >= 120) && (totalSamples <= 120*48))) {
    *outDataSize = 0;
    while (totalSamples > 0) {
      int decodedSample = opus_decode(mDecoder, (const unsigned char*)input,
                                      (opus_int32)inDataSize,
                                      (((opus_int16*)output) + *outDataSize),
                                      120*48, 0);
      if (AM_LIKELY(decodedSample > 0)) {
        totalSamples -= decodedSample;
        *outDataSize += decodedSample * sizeof(AM_U16) * 2;
      } else {
        ret = (AM_UINT)-1;
        break;
      }
    }
    if (AM_LIKELY(totalSamples == 0)) {
      ret = inDataSize;
    }
  }

  return ret;
}
