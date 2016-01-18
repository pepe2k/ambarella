/*******************************************************************************
 * demuxer_aac.h
 *
 * History:
 *   2013-3-5 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef DEMUXER_AAC_H_
#define DEMUXER_AAC_H_

class CDemuxerAac: public CDemuxer
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

  private:
    CDemuxerAac(AM_UINT streamid) :
      inherited(AM_DEMUXER_AAC, streamid),
      mBuffer(NULL),
      mSentSize(0),
      mAvailSize(0),
      mRemainSize(0),
      mNeedToRead(true),
      mIsNewFile(true){}
    virtual ~CDemuxerAac()
    {
      delete[] mBuffer;
      DEBUG("~CDemuxerAac");
    };
    AM_ERR Construct();

  private:
    char   *mBuffer;
    AM_UINT mSentSize;
    AM_UINT mAvailSize;
    AM_U64  mRemainSize;
    bool    mNeedToRead;
    bool    mIsNewFile;
};

#endif /* DEMUXER_AAC_H_ */
