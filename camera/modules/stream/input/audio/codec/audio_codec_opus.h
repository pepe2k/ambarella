/*******************************************************************************
 * audio_codec_opus.h
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

#ifndef AUDIO_CODEC_OPUS_H_
#define AUDIO_CODEC_OPUS_H_

#include <opus/opus.h>
class CAudioCodecOpus: public CAudioCodec
{
  public:
    CAudioCodecOpus();
    virtual ~CAudioCodecOpus();

  public:
    virtual bool InitCodec(AudioCodecInfo &codecInfo,
                           AmAudioCodecMode mode);
    virtual bool FiniCodec();
    virtual AM_UINT encode(AM_U8 *input, AM_UINT inDataSize,
                           AM_U8 *output, AM_UINT *outDataSize);
    virtual AM_UINT decode(AM_U8 *input,  AM_UINT inDataSize,
                           AM_U8 *output, AM_UINT *outDataSize);

  private:
    OpusEncoder      *mEncoder;
    OpusDecoder      *mDecoder;
    OpusRepacketizer *mRepacketizer;
    AM_U8             mEncodeBuf[4096];
    AM_INT            mEncFrameSize;
    AM_UINT           mEncFrameBytes;
};

#endif /* AUDIO_CODEC_OPUS_H_ */
