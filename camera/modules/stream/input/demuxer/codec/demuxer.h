/*******************************************************************************
 * demuxer.h
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

#ifndef DEMUXER_H_
#define DEMUXER_H_
#include <queue>

enum AmDemuxerType {
  AM_DEMUXER_INVALID,
  AM_DEMUXER_UNKNOWN,
  AM_DEMUXER_AAC,
  AM_DEMUXER_WAV,
  AM_DEMUXER_OGG,
  AM_DEMUXER_MP4,
  AM_DEMUXER_TS,
  AM_DEMUXER_ES
};

enum AmDemuxerError {
  AM_DEMUXER_NONE,
  AM_DEMUXER_OK,
  AM_DEMUXER_NO_FILE,
  AM_DEMUXER_NO_PACKET,
};

typedef std::queue<AmFile*> FileQueue;

class IDemuxerCodec
{
  public:
    virtual ~IDemuxerCodec(){}
    virtual AmDemuxerType GetDemuxerType() = 0;
    virtual void Delete() = 0;
    virtual AmDemuxerError GetPacket(CPacket*& packet) = 0;
    virtual void Enable(bool enable) = 0;
    virtual bool PlayListEmpty() = 0;
    virtual AM_UINT StreamId() = 0;
    virtual bool AddUri(const char* uri) = 0;
};

class CDemuxer: public IDemuxerCodec
{
  public:
    virtual AmDemuxerType GetDemuxerType(){return mDemuxerType;}
    virtual void Delete() {delete this;}
    void Enable(bool enable);
    bool PlayListEmpty() {return mPlayList->empty() && !mMedia;}
    AM_UINT StreamId(){return mStreamId;}
    bool AddUri(const char* uri);

  protected:
    CDemuxer(AmDemuxerType type, AM_UINT streamid);
    virtual ~CDemuxer();
    virtual AM_ERR Construct();

  protected:
    inline bool AllocBuffer(CPacket*& pBuffer);
    inline AM_UINT GetAvailBufNum();
    AmFile* GetNewFile();
    bool AllocatePacket(CPacket*& packet);

  protected:
    AmDemuxerType mDemuxerType;
    AM_UINT       mStreamId;
    AM_U64        mFileSize;
    bool          mIsEnabled;
    AmFile       *mMedia;
    CMutex       *mMutex;
    FileQueue    *mPlayList;
    IPacketPool  *mPktPool;
};

#ifdef __cplusplus
extern "C" {
#endif
IDemuxerCodec* get_demuxer_object(AM_UINT streamid);
AmDemuxerType get_demuxer_type();
#ifdef __cplusplus
}
#endif

typedef IDemuxerCodec* (*DemuxerNew)(unsigned int id);
typedef AmDemuxerType  (*DemuxerType)();

#define DEMUXER_NEW  ((const char*)"get_demuxer_object")
#define DEMUXER_TYPE ((const char*)"get_demuxer_type")

#endif /* DEMUXER_H_ */
