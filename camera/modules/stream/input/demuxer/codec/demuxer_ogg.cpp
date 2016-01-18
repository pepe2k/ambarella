/*******************************************************************************
 * demuxer_ogg.cpp
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

#include <queue>
#include <unistd.h>
#include "utilities/am_file.h"

#include "audio_codec_info.h"
#include "demuxer.h"
#include "demuxer_ogg.h"
#include "ogg/ogg.h"

#define PACKET_POOL_SIZE 32
#define OPUS_DATA_SIZE   4096

#define OGGSEEK_BYTES_TO_READ 8192

enum OggState {
  OGG_STATE_OK,
  OGG_STATE_ERROR,
  OGG_STATE_CONTINUE,
  OGG_STATE_DONE
};

class LogicalStream
{
    friend class Ogg;

  public:
    LogicalStream() :
      mSerialno(0),
      mPreSkipSample(0),
      mPageGranule(0),
      mPageGranulePrev(0),
      mCodec(AM_AUDIO_CODEC_NONE)
    {
      memset(&mStreamState, 0, sizeof(mStreamState));
      memset(&mAudioInfo, 0, sizeof(mAudioInfo));
    }
    ~LogicalStream()
    {
      ogg_stream_clear(&mStreamState);
    }

    bool Init(ogg_page* page)
    {
      mSerialno = ogg_page_serialno(page);
      INFO("New logical stream: %08x", mSerialno);
      return (0 == ogg_stream_init(&mStreamState, mSerialno));
    }

    void ResetSerial(int serial)
    {
      ogg_stream_reset_serialno(&mStreamState, serial);
    }

    OggState GetOnePacket(ogg_packet* packet)
    {
      int val = ogg_stream_packetout(&mStreamState, packet);
      OggState ret = (val < 0) ? OGG_STATE_ERROR :
          ((val == 0) ? OGG_STATE_CONTINUE : OGG_STATE_OK);

      return ret;
    }

    OggState OutputPacketFromPage(ogg_page* page, ogg_packet* packet)
    {
      OggState ret = OGG_STATE_OK;

      if (AM_UNLIKELY(!InputPage(page))) {
        ERROR("Failed reading Ogg bitstream data!");
        ret = OGG_STATE_ERROR;
      } else {
        int val = ogg_stream_packetout(&mStreamState, packet);
        ret = (val < 0) ? OGG_STATE_ERROR :
            ((val == 0) ? OGG_STATE_CONTINUE : OGG_STATE_OK);
        if (AM_UNLIKELY(OGG_STATE_ERROR == ret)) {
          ERROR("Failed to read packet!");
        }
      }

      return ret;
    }

    bool InputPage(ogg_page* page)
    {
      bool ret = false;
      if (0 == ogg_stream_pagein(&mStreamState, page)) {
        mPageGranulePrev = mPageGranule;
        mPageGranule = ogg_page_granulepos(page);
        ret = true;
      }
      return ret;
    }

    void ReadOpusHdr(ogg_packet* packet)
    {
      oggpack_buffer opb;

      mCodec = AM_AUDIO_CODEC_OPUS;
      oggpack_readinit(&opb, packet->packet, packet->bytes);
      oggpack_adv(&opb, 64); /* Skip OpusHead  */
      oggpack_adv(&opb, 8);  /* Skip verion_id */
      mAudioInfo.channels = oggpack_read(&opb, 8);
      mAudioInfo.format = MF_OPUS;
      mAudioInfo.sampleRate = 48000;
      mAudioInfo.sampleSize = sizeof(AM_U16);
      mPreSkipSample = oggpack_read(&opb, 16);
      /*mPreSkipSample = AM_MAX(80*48, mPreSkipSample);*/
    }

  private:
    int mSerialno;
    int mPreSkipSample;
    AM_S64 mPageGranule;
    AM_S64 mPageGranulePrev;
    AmAudioCodecType mCodec;
    ogg_stream_state mStreamState;
    AM_AUDIO_INFO    mAudioInfo;
};

class Ogg
{
    friend class CDemuxerOgg;
    typedef std::queue<LogicalStream*> OggStreamQ;
  public:
    static Ogg* Create()
    {
      Ogg* result = new Ogg();
      if (AM_UNLIKELY(result && !result->Construct())) {
        delete result;
        result = NULL;
      }
      return result;
    }
    virtual ~Ogg()
    {
      StreamRelease();
      delete mOggStreamQ;
    }

  public:
    void Reset()
    {
      ogg_sync_reset(&mSyncState);
      StreamRelease();
      mPktCount = 0;
      mIsStreamOk = false;
    }

    bool GetPacket(CPacket*& packet, AmFile& ogg)
    {
      ogg_packet op;
      ogg_page   og;
      bool ret = true;
      OggState opState = OGG_STATE_ERROR;

      while(!mOpusStream || (OGG_STATE_OK !=
                        (opState = mOpusStream->GetOnePacket(&op)))) {
        OggState state = ReadPage(ogg, &og);
        switch(state) {
          case OGG_STATE_OK: {
            if (AM_UNLIKELY(ogg_page_bos(&og))) {
              LogicalStream* newSt = NULL;
              if (AM_LIKELY(mOpusStream && mIsStreamOk)) {
                /* This BOS means this stream is chained without an EOS */
                if ((AM_UNLIKELY(mPktCount > 0 && (mOpusStream->mSerialno ==
                                                   ogg_page_serialno(&og))))) {
                  ERROR("Chaining without changing serial number: "
                        "Last: 0x%08x, Current: 0x%08x",
                        mOpusStream->mSerialno, ogg_page_serialno(&og));
                } else {
                  mIsStreamOk = false;
                  mPktCount = 0;
                  mOggStreamQ->pop();
                  delete mOpusStream;
                  mOpusStream = NULL;
                }
              }
              newSt = new LogicalStream();
              if (AM_LIKELY(newSt)) {
                if (AM_UNLIKELY(!newSt->Init(&og))) {
                  ERROR("Failed to initialize Ogg stream!");
                  ret = false;
                } else if (AM_UNLIKELY(OGG_STATE_OK != newSt->
                                       OutputPacketFromPage(&og, &op))) {
                  ERROR("Failed reading first page of Ogg bitstream data!");
                  ret = false;
                } else if (AM_LIKELY((op.bytes >= 8) &&
                                     (0 == memcmp(op.packet,
                                                  "OpusHead", 8)))) {
                  if (AM_UNLIKELY((OGG_STATE_OK == newSt->GetOnePacket(&op)) ||
                                  (og.header[og.header_len - 1] == 255))) {
                    ERROR("Extra packets on initial header page!");
                    delete newSt;
                    newSt = NULL;
                    state = OGG_STATE_ERROR;
                    ret = false;
                  } else {
                    newSt->ReadOpusHdr(&op);
                  }
                  /* todo: add other ogg supported codec */
                } else {
                  delete newSt;
                  newSt = NULL;
                  state = OGG_STATE_ERROR;
                  ret = false;
                  ERROR("Invalid OGG stream: Only OggOpus is supported!");
                }
                if (AM_LIKELY(newSt)) {
                  if (AM_LIKELY(!mOpusStream && AddLogicalStream(newSt))) {
                    memcpy(((AM_AUDIO_INFO*)packet->GetDataPtr()),
                           &newSt->mAudioInfo, sizeof(newSt->mAudioInfo));
                    packet->SetType(CPacket::AM_PAYLOAD_TYPE_INFO);
                    packet->SetFrameType(newSt->mCodec);
                    packet->SetDataSize(sizeof(AM_AUDIO_INFO));
                    mOpusStream = newSt;
                  } else {
                    delete newSt;
                    newSt = NULL;
                    if (AM_LIKELY(mOpusStream)) {
                      ERROR("Multipexed Opus OGG stream, "
                            "only chained stream is supported!");
                    } else {
                      ret = false;
                      state = OGG_STATE_ERROR;
                    }
                  }
                }
              } else {
                ret = false;
              }
            } else if (AM_UNLIKELY(ogg_page_eos(&og))) {
              state = OGG_STATE_DONE;
              opState = OGG_STATE_DONE;
              ret = true;
              packet->SetDataSize(0);
            } else if (AM_LIKELY(mOpusStream->mSerialno ==
                                 ogg_page_serialno(&og))) {
              mOpusStream->InputPage(&og);
            }
          }break;
          case OGG_STATE_ERROR: {
            packet->Release();
            packet = NULL;
            ret = false;
          }break;
          case OGG_STATE_DONE: {
            packet->SetDataSize(0);
            opState = OGG_STATE_DONE;
            ret = true;
          }break;
          default:break;
        }
        if (AM_UNLIKELY(OGG_STATE_OK != state)) {
          Reset();
          break;
        }
      }
      if (AM_LIKELY(OGG_STATE_OK == opState)) {
        mIsStreamOk = true;
        ++ mPktCount;
        if (AM_UNLIKELY((mPktCount == 1) &&
                        ((OGG_STATE_OK == mOpusStream->GetOnePacket(&op)) ||
                         (og.header[og.header_len - 1] == 255)))) {
          ERROR("Extra packtes on initial tags page! Invalid stream!");
          mIsStreamOk = false;
          ret = false;
        } else if (mPktCount > 1) {
          memcpy(packet->GetDataPtr(), op.packet, op.bytes);
          packet->SetType(CPacket::AM_PAYLOAD_TYPE_DATA);
          packet->SetFrameType(mOpusStream->mCodec);
          packet->SetDataSize(op.bytes);
          packet->mPayload->mData.mOffset = mOpusStream->mPreSkipSample;
        }
      }

      return ret;
    }

  private:
    OggState ReadPage(AmFile& ogg, ogg_page* page)
    {
      OggState ret = OGG_STATE_OK;
      char* buffer = NULL;
      ssize_t readSize = 0;

      while (ogg_sync_pageout(&mSyncState, page) != 1) {
        buffer = ogg_sync_buffer(&mSyncState, OGGSEEK_BYTES_TO_READ);
        if (AM_UNLIKELY(!buffer)) {
          ret = OGG_STATE_ERROR;
          break;
        }
        if (AM_LIKELY((readSize = ogg.read(buffer,
                                           OGGSEEK_BYTES_TO_READ)) <= 0)) {
          ret = ((readSize == 0) ? OGG_STATE_DONE : OGG_STATE_ERROR);
          break;
        }
        ogg_sync_wrote(&mSyncState, readSize);
      }

      return ret;
    }

    bool AddLogicalStream(LogicalStream* stream)
    {
      bool added = false;
      size_t count = mOggStreamQ->size();
      for (size_t i = 0; i < count; ++ i) {
        LogicalStream* st = mOggStreamQ->front();
        mOggStreamQ->pop();
        mOggStreamQ->push(st);
        if (AM_UNLIKELY(st->mSerialno == stream->mSerialno)) {
          added = true;
          break;
        }
      }
      if (AM_LIKELY(!added)) {
        mOggStreamQ->push(stream);
      } else {
        ERROR("Logical Stream with serial No.: 0x%08x is already added!",
              stream->mSerialno);
      }
      return !added;
    }

  private:
    Ogg() :
      mOggStreamQ(NULL),
      mOpusStream(NULL),
      mPktCount(0),
      mIsStreamOk(false)
    {}
    bool Construct()
    {
      bool ret = false;
      do {
        if (0 != ogg_sync_init(&mSyncState)) {
          ERROR("Failed to initialize ogg_sync_state!");
          break;
        }
        if (AM_UNLIKELY(NULL == (mOggStreamQ = new OggStreamQ))) {
          ERROR("Failed to allocate OggStreamQueue!");
          break;
        }
        ret = true;
      } while(0);

      return ret;
    }
    void StreamRelease()
    {
      while (mOggStreamQ && !mOggStreamQ->empty()) {
        delete mOggStreamQ->front();
        mOggStreamQ->pop();
      }
      mOpusStream = NULL;
    }

  private:
    ogg_sync_state mSyncState;
    OggStreamQ*    mOggStreamQ;
    LogicalStream* mOpusStream;
    AM_UINT        mPktCount;
    bool           mIsStreamOk;
};

/*******************************************************************************
 * CDemuxerOgg
 ******************************************************************************/
AmDemuxerType get_demuxer_type()
{
  return AM_DEMUXER_OGG;
}

IDemuxerCodec* get_demuxer_object(AM_UINT streamid)
{
  return CDemuxerOgg::Create(streamid);
}

CDemuxer* CDemuxerOgg::Create(AM_UINT streamid)
{
  CDemuxerOgg* result = new CDemuxerOgg(streamid);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }

  return result;
}

AM_ERR CDemuxerOgg::Construct()
{
  AM_ERR ret = inherited::Construct();
  if (AM_LIKELY(ME_OK == ret)) {
    do {
      mPktPool = CFixedPacketPool::Create("OggDemuxerPktPool",
                                          PACKET_POOL_SIZE,
                                          OPUS_DATA_SIZE);
      if (AM_UNLIKELY(NULL == mPktPool)) {
        ERROR("Failed to create packet pool for OGG demuxer!");
        ret = ME_NO_MEMORY;
        break;
      }
      if (AM_UNLIKELY(NULL == (mOgg = Ogg::Create()))) {
        ERROR("Failed to create Ogg demuxer!");
        ret = ME_NO_MEMORY;
        break;
      }
    } while(0);
  }

  return ret;
}
void CDemuxerOgg::Enable(bool enable)
{
  inherited::Enable(enable);
  if (AM_LIKELY(mOgg)) {
    mOgg->Reset();
  }
}
AmDemuxerError CDemuxerOgg::GetPacket(CPacket*& packet)
{
  AmDemuxerError ret = AM_DEMUXER_OK;

  packet = NULL;
  while (NULL == packet) {
    if (AM_UNLIKELY(NULL == mMedia)) {
      mIsNewFile = (NULL != (mMedia = GetNewFile()));
      if (AM_UNLIKELY(!mMedia)) {
        ret = AM_DEMUXER_NO_FILE;
        break;
      }
    }

    if (AM_LIKELY(mMedia && mMedia->open(AmFile::AM_FILE_READONLY))) {
      if (AM_LIKELY(AllocatePacket(packet))) {
        packet->SetAttr(CPacket::AM_PAYLOAD_ATTR_AUDIO);
        packet->SetStreamId(mStreamId);
        packet->SetPTS(0);

        if (AM_LIKELY(mOgg->GetPacket(packet, *mMedia))) {
          if (AM_LIKELY(packet->GetDataSize() > 0)) {
            continue;
          } else {
            INFO("%s EOF", mMedia->name());
          }
        }
        delete mMedia;
        mMedia = NULL;
        packet->Release();
        packet = NULL;
      } else {
        ret = AM_DEMUXER_NO_PACKET;
        break;
      }
    }
  }

  return ret;
}

CDemuxerOgg::CDemuxerOgg(AM_UINT streamid) :
    inherited(AM_DEMUXER_OGG, streamid),
    mIsNewFile(true),
    mAudioCodecType(AM_AUDIO_CODEC_NONE),
    mOgg(NULL)
{}

CDemuxerOgg::~CDemuxerOgg()
{
  delete mOgg;
  DEBUG("~CDemuxerOgg");
}
