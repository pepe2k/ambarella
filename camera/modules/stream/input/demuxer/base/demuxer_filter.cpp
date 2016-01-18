/*******************************************************************************
 * demuxer_filter.cpp
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
#include "config.h"
#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_plugin.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_media_info.h"

#include "demuxer.h"
#include "demuxer_if.h"
#include "demuxer_filter.h"

/* Data packet will be provided by real demuxer,
 * so UniversalDemuxer's packet pool size is limited to minimum size
 */
#define PACKET_POOL_SIZE MAX_ENCODE_STREAM_NUM

#ifdef BUILD_AMBARELLA_CAMERA_DEMUXER_DIR
#define CAMERA_DEMUXER_DIR ((const char*)BUILD_AMBARELLA_CAMERA_DEMUXER_DIR)
#else
#define CAMERA_DEMUXER_DIR ((const char*)"/usr/lib/camera/demuxer")
#endif

#define DEMUXER_WAIT_COUNT 10

IPacketFilter* create_demuxer_filter(IEngine* engine)
{
  return CDemuxerFilter::Create(engine);
}

static const char* demuxer_type_to_str[] =
{
  "Invalid",
  "Unknown",
  "AAC",
  "WAV",
  "MP4",
  "TS",
  "ES"
};

DemuxerObject::DemuxerObject(const char* file) :
  plugin(NULL),
  getDemuxerObj(NULL),
  type(AM_DEMUXER_UNKNOWN)
{
  library = amstrdup(file);
}

DemuxerObject::~DemuxerObject()
{
  close();
  delete[] library;
}

bool DemuxerObject::open()
{
  bool ret = false;
  plugin = CPlugin::Create(library);
  if (AM_LIKELY(plugin)) {
    DemuxerType getDemuxerType = (DemuxerType)plugin->getSymbol(DEMUXER_TYPE);
    if (AM_LIKELY(getDemuxerType)) {
      type = getDemuxerType();
      switch(type) {
        case AM_DEMUXER_AAC:
        case AM_DEMUXER_WAV:
        case AM_DEMUXER_OGG:
        case AM_DEMUXER_MP4:
        case AM_DEMUXER_TS:
        case AM_DEMUXER_ES: {
          ret = true;
        }break;
        case AM_DEMUXER_INVALID:
        case AM_DEMUXER_UNKNOWN:
        default: {
          ret = false;
          ERROR("Cannot determine codec type!");
        }break;
      }
      if (AM_LIKELY(ret)) {
        getDemuxerObj = (DemuxerNew)plugin->getSymbol(DEMUXER_NEW);
        ret = (getDemuxerObj != NULL);
      }
    }
  }

  return ret;
}

void DemuxerObject::close()
{
  if (AM_LIKELY(plugin)) {
    plugin->Delete();
    plugin = NULL;
  }
}

IPacketFilter* GetDemuxerFilter(IEngine *engine)
{
  return CDemuxerFilter::Create(engine);
}

CDemuxerFilter* CDemuxerFilter::Create(IEngine *engine,
                                       bool RTPriority,
                                       int priority)
{
  CDemuxerFilter *ret = new CDemuxerFilter(engine);
  if (AM_UNLIKELY(ret && (ME_OK != ret->Construct(RTPriority,
                                                  priority)))) {
    delete ret;
    ret = NULL;
  }

  return ret;
}

CDemuxerFilter::CDemuxerFilter(IEngine *engine) :
    inherited(engine, "UniversalDemuxer"),
    mMutexPause(NULL),
    mMutexDemuxer(NULL),
    mEvent(NULL),
    mOutPin(NULL),
    mPktPool(NULL),
    mDemuxerList(NULL),
    mDemuxer(NULL),
    mRun(false),
    mPaused(false),
    mStarted(false)
{
}

CDemuxerFilter::~CDemuxerFilter()
{
  AM_DELETE(mMutexPause);
  AM_DELETE(mMutexDemuxer);
  AM_DELETE(mEvent);
  AM_DELETE(mOutPin);
  AM_RELEASE(mPktPool);
  AM_DELETE(mDemuxer);
  while (mDemuxerList && !mDemuxerList->empty()) {
    delete mDemuxerList->front();
    mDemuxerList->pop();
  }
  delete mDemuxerList;
  DEBUG("~CDemuxerFilter");
}

AM_ERR CDemuxerFilter::Construct(bool RTPriority, int priority)
{
  AM_ERR ret = inherited::Construct(RTPriority, priority);
  if (AM_LIKELY(ME_OK == ret)) {
    do {
      if (AM_UNLIKELY(NULL ==
          (mPktPool = CFixedPacketPool::Create("UniversalDemuxerPktPool",
                                                 PACKET_POOL_SIZE, 0)))) {
        ERROR("Failed to create packet pool!");
        ret = ME_NO_MEMORY;
        break;
      }
      if (AM_UNLIKELY(NULL == (mOutPin = CDemuxerOutput::Create(this)))) {
        ERROR("Failed to create output pin!");
        ret = ME_NO_MEMORY;
        break;
      } else {
        mOutPin->SetBufferPool(mPktPool);
      }
      if (AM_UNLIKELY(NULL == (mMutexPause = CMutex::Create()))) {
        ERROR("Failed to create mutex for pause!");
        ret = ME_NO_MEMORY;
        break;
      }
      if (AM_UNLIKELY(NULL == (mMutexDemuxer = CMutex::Create()))) {
        ERROR("Failed to create mutex for demuxer!");
        ret = ME_NO_MEMORY;
        break;
      }
      if (AM_UNLIKELY(NULL == (mEvent = CEvent::Create()))) {
        ERROR("Failed to create event!");
        ret = ME_NO_MEMORY;
        break;
      }
      if (AM_UNLIKELY(NULL == (mDemuxerList = LoadCodecs()))) {
        ERROR("Failed to load any demuxers!");
        ret = ME_ERROR;
        break;
      }
    }while(0);
  }

  return ret;
}

AM_ERR CDemuxerFilter::AddMedia(const char* uri)
{
  AUTO_LOCK(mMutexDemuxer);
  AmDemuxerType type = CheckMediaType(uri);
  AM_ERR ret = ME_ERROR;
  if (AM_LIKELY((AM_DEMUXER_INVALID != type) &&
                (AM_DEMUXER_UNKNOWN != type))) {
    if (AM_LIKELY(!mDemuxer)) {
      mDemuxer = GetDemuxerObject(type, 0);
      ret = (mDemuxer ? ME_OK : ME_ERROR);
    } else {
      if (AM_LIKELY(mDemuxer->GetDemuxerType() == type)) {
        ret = ME_OK;
      } else {
        if (AM_LIKELY(mDemuxer->PlayListEmpty())) {
          AM_DELETE(mDemuxer);
          mDemuxer = GetDemuxerObject(type, 0);
          ret = (mDemuxer ? ME_OK : ME_ERROR);
        } else {
          ERROR("%s is different from the media type of the codec in use!",
                uri);
          ret = ME_ERROR;
        }
      }
    }
    if (AM_LIKELY(ret == ME_OK)) {
      mDemuxer->Enable(true);
      ret = (mDemuxer->AddUri(uri) ? ME_OK : ME_ERROR);
    } else {
      ERROR("Failed to load demuxer object!");
    }
  } else {
    ERROR("Unsupported media type!");
  }

  return ret;
}

AM_ERR CDemuxerFilter::Play(const char* uri)
{
  AUTO_LOCK(mMutexDemuxer);
  AM_ERR ret = ME_OK;
  if (AM_LIKELY(uri)) {
    ret = AddMedia(uri);
  }
  if (AM_LIKELY((ret == ME_OK) && !mRun)) {
    ret = Start();
  }
  return ret;
}

AM_ERR CDemuxerFilter::Start()
{
  AUTO_LOCK(mMutexDemuxer);
  mRun = true;
  mEvent->Signal();
  return ME_OK;
}

AM_ERR CDemuxerFilter::Stop()
{
  AUTO_LOCK(mMutexDemuxer);
  AM_ERR ret = ME_OK;
  if (AM_UNLIKELY(!mStarted)) {
    mEvent->Signal();
  }
  if (AM_LIKELY(mRun)) {
    mRun = false;
    ret = inherited::Stop();
  }

  return ret;
}


void CDemuxerFilter::OnRun()
{
  AM_UINT count = 0;
  AmDemuxerError err = AM_DEMUXER_NONE;
  mStarted = false;
  CmdAck(ME_OK);
  mEvent->Clear();
  mEvent->Wait();
  mStarted = true;
  while (mRun) {
    CPacket *packet = NULL;
    if (AM_UNLIKELY(mDemuxer && mDemuxer->PlayListEmpty())) {
      usleep(100000);
      if (AM_LIKELY(++count < DEMUXER_WAIT_COUNT)) {
        continue;
      }
      if (AM_LIKELY(err == AM_DEMUXER_NO_FILE)) {
        if (mOutPin->AllocBuffer(packet)) {
          NOTICE("Timeout, no files found! Send EOF packet!");
          packet->SetType(CPacket::AM_PAYLOAD_TYPE_EOF);
          packet->SetAttr(CPacket::AM_PAYLOAD_ATTR_AUDIO);
          packet->SetStreamId(mDemuxer->StreamId());
          mOutPin->SendBuffer(packet);
          err = AM_DEMUXER_NONE;
        } else {
          NOTICE("Failed to allocate packet. Is buffer disabled?");
          mRun = false;
        }
      }
      count = 0;
      continue;
    }
    count = 0;
    if (AM_LIKELY(mDemuxer)) {
      err = mDemuxer->GetPacket(packet);
      switch(err) {
        case AM_DEMUXER_OK: {
          mOutPin->SendBuffer(packet);
        }break;
        case AM_DEMUXER_NO_PACKET: {
          usleep(200000);
        }break;
        case AM_DEMUXER_NO_FILE:
        default:break;
      }
    } else {
      usleep(200000);
    }
  }

  if (AM_LIKELY(mDemuxer)) {
    mDemuxer->Enable(false);
  }
  if (AM_UNLIKELY(mRun)) {
    PostEngineMsg(IEngine::MSG_ABORT);
  }

  INFO("UniversalDemuxer exit mainloop!");
}

inline void CDemuxerFilter::SendBuffer(CPacket *packet)
{
  mOutPin->SendBuffer(packet);
}

AmDemuxerType CDemuxerFilter::CheckMediaType(const char* uri)
{
  AmDemuxerType type = AM_DEMUXER_UNKNOWN;

  if (AM_LIKELY(uri && AmFile::exists(uri))) {
    AmFile media(uri);
    if (AM_LIKELY(media.open(AmFile::AM_FILE_READONLY))) {
      char buf[16] = {0};
      ssize_t size = media.read(buf, 12);
      media.close();
      if (12 == size) {
        AM_U16     high = buf[0];
        AM_U16      low = buf[1];
        AM_U32  chunkId = buf[0] | (buf[1]<<8) | (buf[ 2]<<16) | (buf[ 3]<<24);
        AM_U32 riffType = buf[8] | (buf[9]<<8) | (buf[10]<<16) | (buf[11]<<24);
        if (buf[0] == 0x47) {
          type = AM_DEMUXER_TS;
        } else if (0x0FFF == ((high << 4) | (low >> 4))) {
          type = AM_DEMUXER_AAC;
        } else if ((buf[4] == 'f') && (buf[5] == 't') &&
                   (buf[6] == 'y') && (buf[7] == 'p')) {
          type = AM_DEMUXER_MP4;
        } else if (0x00000001 ==
            ((buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3])) {
          type = AM_DEMUXER_ES;
        } else if ((0x46464952 == chunkId) && /* RIFF */
                   (0x45564157 == riffType))  /* WAVE */{
          type = AM_DEMUXER_WAV;
        } else if ((0x5367674f) == chunkId) { /* OggS */
          type = AM_DEMUXER_OGG;
        }
      } else {
        ERROR("Failed to read file: %s", uri);
      }
    }
  } else {
    if (!uri) {
      ERROR("NULL URI!");
    } else {
      ERROR("%s doesn't exist!", uri);
    }
    type = AM_DEMUXER_INVALID;
  }

  return type;
}

DemuxerList* CDemuxerFilter::LoadCodecs()
{
  DemuxerList* list = NULL;
  char **codecs = NULL;
  int number = AmFile::list_files(CAMERA_DEMUXER_DIR, codecs);
  if (AM_LIKELY(number > 0)) {
    list = new DemuxerList();
    for (int i = 0; i < number; ++ i) {
      DemuxerObject *object = new DemuxerObject(codecs[i]);
      if (AM_LIKELY(object && object->open())) {
        list->push(object);
      } else if (AM_LIKELY(object)) {
        delete object;
      }
      delete[] codecs[i];
      codecs[i] = NULL;
    }
    delete[] codecs;
  } else {
    ERROR("No codecs found!");
  }

  return list;
}

/* ToDo: Add multiple demuxer support, currently 1 is supported */
inline IDemuxerCodec* CDemuxerFilter::GetDemuxerObject(AmDemuxerType type,
                                                       AM_UINT streamid)
{
  DemuxerObject *obj = NULL;
  for (AM_UINT i = 0; i < mDemuxerList->size(); ++ i) {
    obj = mDemuxerList->front();
    mDemuxerList->pop();
    mDemuxerList->push(obj);
    if (AM_LIKELY(obj->type == type)) {
      break;
    } else {
      obj = NULL;
    }
  }
  if (AM_UNLIKELY(!obj)) {
    ERROR("Failed to get demuxer for media type: %s",
          demuxer_type_to_str[type]);
  }

  return (obj ? obj->getDemuxerObj(streamid) : NULL);
}
