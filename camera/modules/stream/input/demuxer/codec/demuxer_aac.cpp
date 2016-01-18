/*******************************************************************************
 * demuxer_aac.cpp
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
#include "demuxer_aac.h"
#include "adts.h"
#include "audio_codec_info.h"

#define AAC_AUDIO_DATA_SIZE (6144/8*2+100) /* From sample code of libaacenc.a*/
#define PACKET_POOL_SIZE    32
#define FILE_BUFFER_SIZE    4096

AM_UINT index_to_freq[] =
{
  96000,
  88200,
  64000,
  48000,
  44100,
  32000,
  24000,
  22050,
  16000,
  12000,
  11025,
  8000,
  7350,
  0,
  0,
  0
};

AmDemuxerType get_demuxer_type()
{
  return AM_DEMUXER_AAC;
}

IDemuxerCodec* get_demuxer_object(AM_UINT streamid)
{
  return CDemuxerAac::Create(streamid);
}

CDemuxer* CDemuxerAac::Create(AM_UINT streamid)
{
  CDemuxerAac* result = new CDemuxerAac(streamid);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }

  return result;
}

AM_ERR CDemuxerAac::Construct()
{
  AM_ERR ret = inherited::Construct();
  if (AM_LIKELY(ME_OK == ret)) {
    mPktPool = CFixedPacketPool::Create("AacDemuxerPktPool",
                                        PACKET_POOL_SIZE,
                                        AAC_AUDIO_DATA_SIZE);
    if (AM_UNLIKELY(NULL == mPktPool)) {
      ERROR("Failed to create packet pool for AAC demuxer!");
      ret = ME_NO_MEMORY;
    } else {
      mBuffer = new char[FILE_BUFFER_SIZE];
      if (AM_UNLIKELY(NULL == mBuffer)) {
        ERROR("Failed to create file read buffer!");
        ret = ME_NO_MEMORY;
      } else {
        memset(mBuffer, 0, FILE_BUFFER_SIZE);
      }
    }
  }

  return ret;
}

AmDemuxerError CDemuxerAac::GetPacket(CPacket*& packet)
{
  AmDemuxerError ret = AM_DEMUXER_OK;
  packet = NULL;
  while (NULL == packet) {
    if (AM_UNLIKELY(NULL == mMedia)) {
      mNeedToRead = true;
      mAvailSize = 0;
      mSentSize = 0;
      mIsNewFile = (NULL != (mMedia = GetNewFile()));
      if (AM_UNLIKELY(!mMedia)) {
        ret = AM_DEMUXER_NO_FILE;
        break;
      } else {
        mRemainSize = mFileSize;
      }
    }

    if (AM_LIKELY(mMedia && mMedia->open(AmFile::AM_FILE_READONLY))) {
      AdtsHeader *adts = NULL;
      if (AM_LIKELY(mNeedToRead)) {
        ssize_t readSize = mMedia->read(mBuffer, FILE_BUFFER_SIZE);
        if (AM_UNLIKELY(readSize <= 0)) {
          if (readSize < 0) {
            ERROR("%s: %s! Skip!", mMedia->name(), strerror(errno));
          } else {
            INFO("%s EOF", mMedia->name());
          }
          delete mMedia;
          mMedia = NULL;
          continue;
        } else {
          mSentSize = 0;
          mAvailSize = readSize;
          mNeedToRead = false;
        }
      }
      adts = (AdtsHeader*)&mBuffer[mSentSize];
      if (AM_LIKELY(adts->IsSyncWordOk() &&
                    (mAvailSize > sizeof(AdtsHeader)) &&
                    (mAvailSize >= adts->FrameLength()))) {
        if (AM_LIKELY(AllocatePacket(packet))) {
          packet->SetAttr(CPacket::AM_PAYLOAD_ATTR_AUDIO);
          packet->SetFrameType(AM_AUDIO_CODEC_AAC);
          packet->SetStreamId(mStreamId);
          packet->SetPTS(0);
          if (AM_UNLIKELY(mIsNewFile)) {
            AM_AUDIO_INFO *audioInfo = ((AM_AUDIO_INFO*)packet->GetDataPtr());
            audioInfo->channels = adts->AacChannelConf();
            audioInfo->sampleRate = index_to_freq[adts->AacFrequencyIndex()];
            packet->SetType(CPacket::AM_PAYLOAD_TYPE_INFO);
            packet->SetDataSize(sizeof(AM_AUDIO_INFO));
            mIsNewFile = false;
          } else {
            AM_U16 framelen = adts->FrameLength();
            memcpy(packet->GetDataPtr(), &mBuffer[mSentSize], framelen);
            mSentSize += framelen;
            mAvailSize -= framelen;
            mRemainSize -= framelen;
            packet->SetType(CPacket::AM_PAYLOAD_TYPE_DATA);
            packet->SetDataSize(framelen);
            packet->SetDataOffset(0);
            mNeedToRead = (mAvailSize == 0);
          }
        } else {
          ret = AM_DEMUXER_NO_PACKET;
          break;
        }
      } else if (AM_LIKELY(adts->IsSyncWordOk())) {
        if (AM_UNLIKELY(mRemainSize < adts->FrameLength())) {
          WARN("%s is incomplete, the last ADTS reports length is %u bytes, "
               "but available file length is %llu bytes!",
               mMedia->name(), adts->FrameLength(), mRemainSize);
          delete mMedia;
          mMedia = NULL;
          continue;
        } else {
          mMedia->seek(-mAvailSize, AmFile::AM_FILE_SEEK_CUR);
          mNeedToRead = true;
        }
      } else {
        while ((mAvailSize >= sizeof(AdtsHeader)) && !adts->IsSyncWordOk()) {
          -- mAvailSize;
          -- mRemainSize;
          ++ mSentSize;
          adts = (AdtsHeader*)&mBuffer[mSentSize];
        }
        ERROR("Invalid ADTS, %u bytes data skipped!", mSentSize);
        if (AM_UNLIKELY(mRemainSize <= sizeof(AdtsHeader))) {
          delete mMedia;
          mMedia = NULL;
          continue;
        } else {
          mMedia->seek(-mAvailSize, AmFile::AM_FILE_SEEK_CUR);
          mNeedToRead = true;
        }
      }
    }
  }

  return ret;
}
