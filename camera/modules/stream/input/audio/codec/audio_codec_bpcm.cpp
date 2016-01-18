/*******************************************************************************
 * audio_codec_bpcm.cpp
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
#include "am_media_info.h"

#include "audio_codec_info.h"
#include "audio_codec.h"
#include "audio_codec_bpcm.h"

bool CAudioCodecBpcm::InitCodec(AudioCodecInfo &codecInfo,
                                AmAudioCodecMode mode)
{
  mAudioInfo = &codecInfo.audio_info;
  if (AM_LIKELY(false == mIsInitialized)) {
    codecInfo.audio_info.format = MF_BPCM;
    mSrcChannelNum = codecInfo.audio_info.channels;
    if (mode == AM_AUDIO_CODEC_MODE_ENCODE) {
      mOutChannelNum = codecInfo.codec_bpcm.out_channel_num;
    } else {
      mOutChannelNum = codecInfo.audio_info.channels;
    }
    mIsInitialized = true;
  }

  return mIsInitialized;
}

bool CAudioCodecBpcm::FiniCodec()
{
  mIsInitialized = false;
  return true;
}

AM_UINT CAudioCodecBpcm::encode(AM_U8 *input,  AM_UINT inDataSize,
                                AM_U8 *output, AM_UINT *outDataSize)
{
  return High16ToLow16(input, output, inDataSize, outDataSize);
}

AM_UINT CAudioCodecBpcm::decode(AM_U8 *input,  AM_UINT inDataSize,
                                AM_U8 *output, AM_UINT *outDataSize)
{
  return High16ToLow16(input, output, inDataSize, outDataSize);
}

inline AM_UINT CAudioCodecBpcm::High16ToLow16(AM_U8 *input,
                                              AM_U8 *output,
                                              AM_UINT inDataSize,
                                              AM_UINT *outDataSize)
{
  if (mSrcChannelNum == mOutChannelNum) {
    for (AM_UINT i = 0; i < inDataSize; i += 2) {
      output[i + 1] = input[i];
      output[i]     = input[i + 1];
    }
    *outDataSize = inDataSize;
  } else if ((mSrcChannelNum == 1) && (mOutChannelNum == 2)) {
    for (AM_UINT i = 0, j = 0; i < inDataSize; i += 2, j += 4) {
      output[j + 1] = input[i];
      output[j]     = input[i + 1];
      output[j + 3] = input[i];
      output[j + 2] = input[i + 1];
    }
    *outDataSize = inDataSize * 2;
  } else if ((mSrcChannelNum == 2) && (mOutChannelNum == 1)) {
    for (AM_UINT i = 0, j = 0; i < inDataSize; i += 4, j += 2) {
      output[j + 1] = input[i];
      output[j]     = input[i + 1];
    }
    *outDataSize = inDataSize / 2;
  }

  return inDataSize;
}
