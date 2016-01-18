/*******************************************************************************
 * demuxer.cpp
 *
 * History:
 *   2013-7-5 - [ypchang] created file
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
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_media_info.h"

#include <unistd.h>
#include "utilities/am_file.h"

#include "demuxer.h"

CDemuxer::CDemuxer(AmDemuxerType type, AM_UINT streamid) :
  mDemuxerType(type),
  mStreamId(streamid),
  mFileSize(0),
  mIsEnabled(false),
  mMedia(NULL),
  mMutex(NULL),
  mPlayList(NULL),
  mPktPool(NULL)
{
}

CDemuxer::~CDemuxer()
{
  if (AM_LIKELY(mPlayList)) {
    while (!mPlayList->empty()) {
      delete mPlayList->front();
      mPlayList->pop();
    }
    delete mPlayList;
  }
  delete mMedia;
  AM_DELETE(mMutex);
  AM_RELEASE(mPktPool);
  DEBUG("~CDemuxer");
}

void CDemuxer::Enable(bool enable)
{
  AUTO_LOCK(mMutex);
  if (AM_LIKELY(mPktPool && (mIsEnabled != enable))) {
    mIsEnabled = enable;
    mPktPool->Enable(enable);
    NOTICE("%s Demuxer %d!", enable ? "Enable" : "Disable", mDemuxerType);
  }
  if (AM_LIKELY(!enable)) {
    delete mMedia;
    mMedia = NULL;
    while (!PlayListEmpty()) {
      delete mPlayList->front();
      mPlayList->pop();
    }
  }
}

AM_ERR CDemuxer::Construct()
{
  mPlayList = new FileQueue();
  mMutex = CMutex::Create();
  return ((mPlayList && mMutex)? ME_OK : ME_NO_MEMORY);
}

inline bool CDemuxer::AllocBuffer(CPacket*& pBuffer)
{
  return (pBuffer ? mPktPool->AllocBuffer(pBuffer, 0) : false);
}

inline AM_UINT CDemuxer::GetAvailBufNum()
{
  return mPktPool->GetAvailBufNum();
}

AmFile* CDemuxer::GetNewFile()
{
  AmFile* file = NULL;
  while ((NULL == file) && !mPlayList->empty()) {
    file = mPlayList->front();
    mPlayList->pop();
    if (AM_UNLIKELY(file && !file->exists())) {
      ERROR("%s does not exist!", file->name());
      delete file;
      file = NULL;
      mFileSize = 0;
    } else {
      mFileSize = file->size();
      NOTICE("%s size: %llu", file->name(), mFileSize);
    }
  }

  return file;
}

bool CDemuxer::AllocatePacket(CPacket*& packet)
{
  if (AM_LIKELY(mPktPool->GetAvailBufNum())) {
    if (!mPktPool->AllocBuffer(packet, sizeof(packet))) {
      ERROR("Failed to allocate buffer from %s", mPktPool->GetName());
      packet = NULL;
    }
  }
  return (packet != NULL);
}

bool CDemuxer::AddUri(const char* uri)
{
  AUTO_LOCK(mMutex);
  bool ret = (uri && AmFile::exists(uri));
  if (AM_LIKELY(ret)) {
    mPlayList->push(new AmFile(uri));
  } else {
    if (!uri) {
      ERROR("Invalid uri!");
    } else {
      ERROR("%s doesn't exist!", uri);
    }
  }

  return ret;
}
