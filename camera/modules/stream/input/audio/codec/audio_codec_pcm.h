/*******************************************************************************
 * audio_codec_pcm.h
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

#ifndef AUDIO_CODEC_PCM_H_
#define AUDIO_CODEC_PCM_H_

class CAudioCodecPcm: public CAudioCodec
{
  public:
    CAudioCodecPcm() :
      CAudioCodec(AM_AUDIO_CODEC_PCM),
      mSrcChannelNum(2),
      mOutChannelNum(2){}
    virtual ~CAudioCodecPcm(){FiniCodec();}

  public:
    virtual bool InitCodec(AudioCodecInfo &codecInfo,
                           AmAudioCodecMode mode);
    virtual bool FiniCodec();
    virtual AM_UINT encode(AM_U8 *input,  AM_UINT inDataSize,
                           AM_U8 *output, AM_UINT *outDataSize);
    virtual AM_UINT decode(AM_U8 *input,  AM_UINT inDataSize,
                           AM_U8 *output, AM_UINT *outDataSize);

  private:
    inline AM_UINT ChangeChannel(AM_U8  *input,
                                 AM_U8  *output,
                                 AM_UINT inDataSize,
                                 AM_UINT *outDataSize);
  private:
    AM_U32 mSrcChannelNum;
    AM_U32 mOutChannelNum;
};


#endif /* AUDIO_CODEC_PCM_H_ */
