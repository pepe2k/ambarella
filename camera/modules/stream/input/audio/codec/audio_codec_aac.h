/*******************************************************************************
 * audio_codec_aac.h
 *
 * History:
 *   2013-1-23 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AUDIO_CODEC_AAC_H_
#define AUDIO_CODEC_AAC_H_

#include "aac_audio_enc.h"
#include "aac_audio_dec.h"

#define AAC_DEC_OUT_BUF_SIZE (4*SAMPLES_PER_FRAME)

class CAudioCodecAac: public CAudioCodec
{
  public:
    CAudioCodecAac() :
      CAudioCodec(AM_AUDIO_CODEC_AAC),
      mConfigEnc(NULL),
      mConfigDec(NULL),
      mEncBuffer(NULL),
      mDecBuffer(NULL),
      mDecOutBuf(NULL),
      mCodecMode(AM_AUDIO_CODEC_MODE_NONE)
    {
    }
    virtual ~CAudioCodecAac()
    {
      FiniCodec();
      delete   mConfigEnc;
      delete   mConfigDec;
      delete[] mEncBuffer;
      delete[] mDecBuffer;
      delete[] mDecOutBuf;
    }

  public:
    virtual bool InitCodec(AudioCodecInfo &codecInfo,
                           AmAudioCodecMode mode);
    virtual bool FiniCodec();
    virtual AM_UINT encode(AM_U8 *input,  AM_UINT inDataSize,
                           AM_U8 *output, AM_UINT *outDataSize);
    virtual AM_UINT decode(AM_U8 *input,  AM_UINT inDataSize,
                           AM_U8 *output, AM_UINT *outDataSize);

  private:
    inline void fc32ito16i(AM_S32 *bufin,
                           AM_S16 *bufout,
                           AM_S32 ch,
                           AM_S32 proc_size);
    inline void deinterleave(AM_S16 *data, AM_UINT *size, AM_UINT channel);

  private:
    au_aacenc_config_t *mConfigEnc;
    au_aacdec_config_t *mConfigDec;
    AM_U8              *mEncBuffer;
    AM_U32             *mDecBuffer;
    AM_S32             *mDecOutBuf;
    AmAudioCodecMode    mCodecMode;
};

#endif /* AUDIO_CODEC_AAC_H_ */
