/*******************************************************************************
 * demuxer_wav.h
 *
 * History:
 *   2013-6-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef DEMUXER_WAV_H_
#define DEMUXER_WAV_H_

class CDemuxerWav: public CDemuxer
{
    typedef CDemuxer inherited;

    enum WavAudioFormat {
      WAVE_FORMAT_PCM  = 0x0001,
      WAVE_FORMAT_G726 = 0x0045
    };
  public:
    static CDemuxer* Create(AM_UINT streamid);

  public:
    virtual AmDemuxerError GetPacket(CPacket*& packet);
    virtual void Delete()
    {
      Enable(false);
      delete this;
    }

  private:
    CDemuxerWav(AM_UINT streamid) :
      inherited(AM_DEMUXER_WAV, streamid),
      mIsNewFile(true),
      mAudioCodecType(AM_AUDIO_CODEC_NONE),
      mReadDataSize(0){}
    virtual ~CDemuxerWav()
    {
      DEBUG("~CDemuxerWav");
    }
    AM_ERR Construct();
    bool WavFileParser(AM_AUDIO_INFO& audioInfo, AmFile& wav);

  private:
    bool             mIsNewFile;
    AmAudioCodecType mAudioCodecType;
    uint32_t         mReadDataSize;
};

#endif /* DEMUXER_WAV_H_ */
