/*******************************************************************************
 * demuxer_wav.cpp
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
#include "demuxer_wav.h"
#include "wav.h"

#define PACKET_POOL_SIZE 64
#define PCM_DATA_SIZE    2048
#define G726_DATA_SIZE   960

typedef std::queue<WavChunkData*> WavChunkDataQ;

AmDemuxerType get_demuxer_type()
{
  return AM_DEMUXER_WAV;
}

IDemuxerCodec* get_demuxer_object(AM_UINT streamid)
{
  return CDemuxerWav::Create(streamid);
}

CDemuxer* CDemuxerWav::Create(AM_UINT streamid)
{
  CDemuxerWav* result = new CDemuxerWav(streamid);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }

  return result;
}

AM_ERR CDemuxerWav::Construct()
{
  AM_ERR ret = inherited::Construct();
  if (AM_LIKELY(ME_OK == ret)) {
    mPktPool = CFixedPacketPool::Create("WavDemuxerPktPool",
                                        PACKET_POOL_SIZE,
                                        PCM_DATA_SIZE);
    if (AM_UNLIKELY(NULL == mPktPool)) {
      ERROR("Failed to create packet pool for WAV demuxer!");
      ret = ME_NO_MEMORY;
    }
  }
  return ret;
}

bool CDemuxerWav::WavFileParser(AM_AUDIO_INFO& audioInfo, AmFile& wav)
{
  bool ret = false;

  memset(&audioInfo, 0, sizeof(audioInfo));
  if (AM_LIKELY(wav.is_open())) {
    WavRiffHdr wavRiff;
    ssize_t readSize = wav.read((char*)&wavRiff, sizeof(wavRiff));
    if (AM_LIKELY((readSize == sizeof(wavRiff)) && wavRiff.isChunkOk())) {
      WavChunkDataQ wavChunkDataQ;
      WavChunkData* chunkData = NULL;
      WavChunkHdr*  chunkHdr = NULL;
      do {
        chunkData = new WavChunkData();
        chunkHdr = (WavChunkHdr*)chunkData;
        if (AM_LIKELY(chunkHdr)) {
          readSize = wav.read((char*)chunkHdr, sizeof(*chunkHdr));
          if (AM_UNLIKELY(readSize != sizeof(*chunkHdr))) {
            delete chunkData;
            chunkData = NULL;
          } else {
            char* id = (char*)&chunkHdr->chunkId;
            INFO("Found WAV chunk: \"%c%c%c%c\", header size: %u",
                 id[0], id[1], id[2], id[3], chunkHdr->chunkSize);
            if (AM_LIKELY(!chunkData->isDataChunk())) {
              char* data = chunkData->getChunkData(chunkHdr->chunkSize);
              if (AM_LIKELY(data)) {
                readSize = wav.read(data, chunkHdr->chunkSize);
                if (AM_UNLIKELY(readSize != (ssize_t)(chunkHdr->chunkSize))) {
                  delete chunkData;
                  chunkData = NULL;
                } else {
                  wavChunkDataQ.push(chunkData);
                }
              } else {
                delete chunkData;
                chunkData = NULL;
              }
            } else {
              wavChunkDataQ.push(chunkData);
              ret = true; /* WAV Header parse done */
            }
          }
        }
      } while(chunkData && !chunkData->isDataChunk());
      if (AM_LIKELY(ret)) {
        WavChunkData* wavChunkData = NULL;
        size_t count = wavChunkDataQ.size();

        for (uint32_t i = 0; i < count; ++ i) {
          wavChunkData = wavChunkDataQ.front();
          wavChunkDataQ.pop();
          wavChunkDataQ.push(wavChunkData);
          if (AM_LIKELY(wavChunkData->isFmtChunk())) {
            break;
          }
          wavChunkData = NULL;
        }
        if (AM_LIKELY(wavChunkData)) {
          WavFmtBody* wavFmtBody = (WavFmtBody*)(wavChunkData->chunkData);
          audioInfo.channels = wavFmtBody->channels;
          audioInfo.sampleRate = wavFmtBody->sampleRate;
          switch(wavFmtBody->audioFmt) {
            case WAVE_FORMAT_PCM  : {
              audioInfo.format = MF_PCM;
              audioInfo.sampleSize = wavFmtBody->bitsPerSample/8;
              mAudioCodecType = AM_AUDIO_CODEC_PCM;
              mReadDataSize = PCM_DATA_SIZE;
            }break;
            case WAVE_FORMAT_G726 : {
              switch(wavFmtBody->bitsPerSample) {
                case 2: audioInfo.format = MF_G726_16; break;
                case 3: audioInfo.format = MF_G726_24; break;
                case 4: audioInfo.format = MF_G726_32; break;
                case 5: audioInfo.format = MF_G726_40; break;
                default:
                  break;
              }
              audioInfo.sampleSize = 0;
              mAudioCodecType = AM_AUDIO_CODEC_G726;
              mReadDataSize = G726_DATA_SIZE;
            }break;
            default: {
              audioInfo.format = MF_NULL;
            }break;
          }
        } else {
          ERROR("Cannot find FMT chunk in %s", wav.name());
          ret = false;
        }
        for (uint32_t i = 0; i < count; ++ i) {
          delete wavChunkDataQ.front();
          wavChunkDataQ.pop();
        }
      } else {
        ERROR("Cannot find data chunk in %s", wav.name());
      }
    } else if (AM_UNLIKELY(readSize < 0)) {
      ERROR("Read %s error: %s", wav.name(), strerror(errno));
    }
  } else {
    ERROR("File %s is not open!", wav.name());
  }

  return ret;
}

AmDemuxerError CDemuxerWav::GetPacket(CPacket*& packet)
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
        char* buffer = (char*)packet->GetDataPtr();

        packet->SetAttr(CPacket::AM_PAYLOAD_ATTR_AUDIO);
        packet->SetStreamId(mStreamId);
        packet->SetPTS(0);

        if (AM_UNLIKELY(mIsNewFile)) {
          mIsNewFile = false;
          if (WavFileParser(*((AM_AUDIO_INFO*)buffer), *mMedia)) {
            packet->SetType(CPacket::AM_PAYLOAD_TYPE_INFO);
            packet->SetFrameType(mAudioCodecType);
            packet->SetDataSize(sizeof(AM_AUDIO_INFO));
            continue;
          } else {
            ERROR("Invalid WAV file: %s, skip!", mMedia->name());
          }
        } else {
          ssize_t readSize = mMedia->read(buffer, mReadDataSize);
          if (AM_UNLIKELY(readSize <= 0)) {
            if (readSize < 0) {
              ERROR("%s: %s! Skip!", mMedia->name(), strerror(errno));
            } else {
              INFO("%s EOF", mMedia->name());
            }
          } else {
            packet->SetType(CPacket::AM_PAYLOAD_TYPE_DATA);
            packet->SetFrameType(mAudioCodecType);
            packet->SetDataSize(readSize);
            packet->SetDataOffset(0);
            continue;
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
