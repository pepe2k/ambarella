/*******************************************************************************
 * audio_codec_g726.h
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

#ifndef AUDIO_CODEC_G726_H_
#define AUDIO_CODEC_G726_H_

struct g726_state_s;
class CAudioCodecG726: public CAudioCodec
{
  public:
    CAudioCodecG726() :
      CAudioCodec(AM_AUDIO_CODEC_G726),
      mG726State(NULL),
      mBitsPerSample(0){}
    virtual ~CAudioCodecG726()
    {
      FiniCodec();
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
    g726_state_s* mG726State;
    AM_UINT       mBitsPerSample;
};

#endif /* AUDIO_CODEC_G726_H_ */
