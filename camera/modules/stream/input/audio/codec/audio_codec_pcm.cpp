/*******************************************************************************
 * audio_codec_pcm.cpp
 *
 * History:
 *   2013-6-3 - [ypchang] created file
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
#include "am_media_info.h"

#include "audio_codec_info.h"
#include "audio_codec.h"
#include "audio_codec_pcm.h"

bool CAudioCodecPcm::InitCodec(AudioCodecInfo &codecInfo,
                               AmAudioCodecMode mode)
{
  mAudioInfo = &codecInfo.audio_info;
  if (AM_LIKELY(false == mIsInitialized)) {
    codecInfo.audio_info.format = MF_PCM;
    mSrcChannelNum = codecInfo.audio_info.channels;
    if (mode == AM_AUDIO_CODEC_MODE_ENCODE) {
      mOutChannelNum = codecInfo.codec_pcm.out_channel_num;
    } else {
      mOutChannelNum = codecInfo.audio_info.channels;
    }
    mIsInitialized = true;
  }

  return mIsInitialized;
}

bool CAudioCodecPcm::FiniCodec()
{
  mIsInitialized = false;
  return true;
}

AM_UINT CAudioCodecPcm::encode(AM_U8 *input,  AM_UINT inDataSize,
                               AM_U8 *output, AM_UINT *outDataSize)
{
  return ChangeChannel(input, output, inDataSize, outDataSize);
}

AM_UINT CAudioCodecPcm::decode(AM_U8 *input,  AM_UINT inDataSize,
                               AM_U8 *output, AM_UINT *outDataSize)
{
  return ChangeChannel(input, output, inDataSize, outDataSize);
}

inline AM_UINT CAudioCodecPcm::ChangeChannel(AM_U8 *input,
                                             AM_U8 *output,
                                             AM_UINT inDataSize,
                                             AM_UINT *outDataSize)
{

  if (mSrcChannelNum == mOutChannelNum) {
    memcpy(output, input, inDataSize);
    *outDataSize = inDataSize;
  } else if ((mSrcChannelNum == 1) && (mOutChannelNum == 2)) {
    for (AM_UINT i = 0, j = 0; i < inDataSize; i += 2, j += 4) {
      output[j]     = input[i];
      output[j + 1] = input[i + 1];
      output[j + 2] = input[i];
      output[j + 3] = input[i + 1];
    }
    *outDataSize = inDataSize * 2;
  } else if ((mSrcChannelNum == 2) && (mOutChannelNum == 1)) {
    for (AM_UINT i = 0, j = 0; i < inDataSize; i += 4, j += 2) {
      output[j]     = input[i];
      output[j + 1] = input[i + i];
    }
    *outDataSize = inDataSize / 2;
  }

  return inDataSize;
}
