/*******************************************************************************
 * demuxer_ogg.h
 *
 * History:
 *   2014年3月14日 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef DEMUXER_OGG_H_
#define DEMUXER_OGG_H_

class Ogg;
class CDemuxerOgg: public CDemuxer
{
    typedef CDemuxer inherited;

  public:
    static CDemuxer* Create(AM_UINT streamid);

  public:
    virtual AmDemuxerError GetPacket(CPacket*& packet);
    virtual void Delete()
    {
      Enable(false);
      delete this;
    }
    virtual void Enable(bool enable);

  private:
    CDemuxerOgg(AM_UINT streamid);
    virtual ~CDemuxerOgg();
    AM_ERR Construct();
  private:
    bool             mIsNewFile;
    AmAudioCodecType mAudioCodecType;
    Ogg*             mOgg;
};

#endif /* DEMUXER_OGG_H_ */
