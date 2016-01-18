/*******************************************************************************
 * audio_codec.h
 *
 * Histroy:
 *   2012-9-25 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AUDIO_CODEC_H_
#define AUDIO_CODEC_H_

static const char *CodecTypeToName[] =
{
  "NONE",
  "AAC",
  "OPUS",
  "PCM",
  "BPCM",
  "G726"
};

class CAudioCodec
{
  public:
    enum AmAudioCodecMode {
      AM_AUDIO_CODEC_MODE_NONE,
      AM_AUDIO_CODEC_MODE_ENCODE,
      AM_AUDIO_CODEC_MODE_DECODE
    };

  public:
    CAudioCodec(AmAudioCodecType type) :
      mCodecType(type),
      mIsInitialized(false),
      mAudioInfo(NULL){}
    virtual ~CAudioCodec() {}

    const char *GetCodecName(){return CodecTypeToName[mCodecType];}
    AmAudioCodecType GetCodecType() {return mCodecType;}
    bool IsInitialized() {return mIsInitialized;}

  public:
    virtual bool InitCodec(AudioCodecInfo &codedInfo,
                           AmAudioCodecMode mode) = 0;
    virtual bool FiniCodec() = 0;
    virtual AM_UINT encode(AM_U8 *input,  AM_UINT inDataSize,
                           AM_U8 *output, AM_UINT *outDataSize) = 0;
    virtual AM_UINT decode(AM_U8 *input,  AM_UINT inDataSize,
                           AM_U8 *output, AM_UINT *outDataSize) = 0;

  protected:
    AmAudioCodecType mCodecType;
    bool             mIsInitialized;
    AM_AUDIO_INFO*   mAudioInfo;
};

#endif /* AUDIO_CODEC_H_ */
