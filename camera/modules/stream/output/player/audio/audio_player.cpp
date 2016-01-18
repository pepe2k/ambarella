/*******************************************************************************
 * audio_player.cpp
 *
 * History:
 *   2013-3-22 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_muxer_info.h"
#include "output_playback_if.h"
#include "mpeg_ts_defs.h"
#include "am_media_info.h"

#include "am_utility.h"
#include "audio_player.h"
#include "player_if.h"
#include "player.h"

#define CONTEXT_NAME ((const char*)"AudioPlayback")
#define STREAM_NAME  ((const char*)"AudioPlaybackStream")
#define MAX_PCM_FRAME_SIZE (2048 * sizeof(AM_S16) * 2)

#define PA_MAINLOOP_LOCK(a) {   \
  /*NOTICE("mainloop lock!");*/ \
  pa_threaded_mainloop_lock(a); \
}

#define PA_MAINLOOP_UNLOCK(a) {   \
  /*NOTICE("mainloop unlock!");*/ \
  pa_threaded_mainloop_unlock(a); \
}

static pa_sample_format_t samplesize_to_format[] =
{
  PA_SAMPLE_S16LE, //0
  PA_SAMPLE_U8,    //1
  PA_SAMPLE_S16LE, //2
  PA_SAMPLE_S24LE, //3
  PA_SAMPLE_S32LE  //4
};

struct PaData {
    CPlayerPulse *adev;
    void         *data;
    PaData(CPlayerPulse *dev, void *userdata) :
      adev(dev),
      data(userdata){}
    PaData() :
      adev(NULL),
      data(NULL){}
};

CPlayerPulse* CPlayerPulse::Create()
{
  CPlayerPulse* result = new CPlayerPulse();

  if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CPlayerPulse::StartPlayer()
{
  AUTO_LOCK(mMutex);
  if (AM_LIKELY((ME_OK == Initialize()) && mLoopRun)) {
    int retvalue = -1;
    NOTICE("Initial latency is set to %u ms", mPlayLatency / 1000);
    PA_MAINLOOP_LOCK(mPaThreadMainLoop);
    mPaStreamPlayback = pa_stream_new(mPaContext,
                                      "AudioPlayback",
                                      &mPaSampleSpec,
                                      &mPaChannelMap);
    if (AM_LIKELY(mPaStreamPlayback)) {
      mPaBufferAttr.fragsize  = ((AM_U32) -1);
      mPaBufferAttr.maxlength = pa_usec_to_bytes(mPlayLatency, &mPaSampleSpec);
      mPaBufferAttr.minreq    = pa_usec_to_bytes(0, &mPaSampleSpec);
      mPaBufferAttr.prebuf    = ((uint32_t) -1);
      mPaBufferAttr.tlength   = pa_usec_to_bytes(mPlayLatency, &mPaSampleSpec);

      pa_stream_set_underflow_callback(mPaStreamPlayback,
                                       StaticPaUnderflow,
                                       mUnderFlow);

      if (AM_UNLIKELY((retvalue = pa_stream_connect_playback(
          mPaStreamPlayback,
          mDefSinkName,
          (const pa_buffer_attr*)&mPaBufferAttr,
          (pa_stream_flags)(PA_STREAM_INTERPOLATE_TIMING |
                            PA_STREAM_ADJUST_LATENCY     |
#if 0
                            /* This increases alsa-sink CPU by 30% */
                            PA_STREAM_VARIABLE_RATE      |
#endif
                            PA_STREAM_AUTO_TIMING_UPDATE),
          NULL, /* Default Initial volume */
          NULL)) < 0)) {
        ERROR("Failed to connect with playback stream!");
        PA_MAINLOOP_UNLOCK(mPaThreadMainLoop);
      } else {
        pa_stream_state_t streamState;
        PA_MAINLOOP_UNLOCK(mPaThreadMainLoop);
        while (PA_STREAM_READY
            != (streamState = pa_stream_get_state(mPaStreamPlayback))) {
          if (AM_UNLIKELY((streamState == PA_STREAM_FAILED) ||
                          (streamState == PA_STREAM_TERMINATED))) {
            break;
          }
        }
        if (AM_LIKELY(PA_STREAM_READY == streamState)) {
          const pa_buffer_attr *attr =
              pa_stream_get_buffer_attr(mPaStreamPlayback);
          if (AM_LIKELY(attr)) {
            INFO("\n Client request max length: %u"
                "\n        min require length: %u"
                "\n         pre buffer length: %u"
                "\n             target length: %u"
                "\nServer returned max length: %u"
                "\n        min require length: %u"
                "\n         pre buffer length: %u"
                "\n             target length: %u",
                mPaBufferAttr.maxlength, mPaBufferAttr.minreq,
                mPaBufferAttr.prebuf, mPaBufferAttr.tlength,
                attr->maxlength, attr->minreq, attr->prebuf, attr->tlength);
            memcpy(&mPaBufferAttr, attr, sizeof(mPaBufferAttr));
          } else {
            ERROR("Failed to get buffer attribute!");
          }
        } else {
          ERROR("Failed to connect playback stream to audio server: %s!",
                pa_strerror(retvalue));
          PA_MAINLOOP_UNLOCK(mPaThreadMainLoop);
          pa_threaded_mainloop_stop(mPaThreadMainLoop);
          mLoopRun = false;
        }
      }
    } else {
      ERROR("Failed to create playback stream: %s",
            pa_strerror(pa_context_errno(mPaContext)));
      PA_MAINLOOP_UNLOCK(mPaThreadMainLoop);
      pa_threaded_mainloop_stop(mPaThreadMainLoop);
      mLoopRun = false;
    }
    NOTICE("Start playback stream: %s", mLoopRun ? "OK" : "Failed");
  } else {
    NOTICE("Failed to start PulseAudio mainloop!");
  }

  return mLoopRun ? ME_OK : ME_ERROR;
}

void CPlayerPulse::AddPacket(CPacket *packet)
{
  if (AM_LIKELY(packet)) {
    packet->AddRef();
    mAudioQueue->push(packet);
  }
}

bool CPlayerPulse::FeedData()
{
  bool ret = false;
  size_t writableSize = 0;
  if (AM_LIKELY(mPaStreamPlayback &&
                (pa_stream_get_state(mPaStreamPlayback) == PA_STREAM_READY) &&
                !mIsDraining && ((writableSize =
                    pa_stream_writable_size(mPaStreamPlayback)) > 0))) {
    PaWrite(mPaStreamPlayback, writableSize, &ret);
  } else {
    usleep(60000);
  }

  return !ret;
}

AM_ERR CPlayerPulse::Start(AM_AUDIO_INFO& audioinfo)
{
  AM_ERR ret = ME_OK;
  bool needRestart = ((audioinfo.channels != mPaSampleSpec.channels) ||
                      (audioinfo.sampleRate != mPaSampleSpec.rate)   ||
                      (samplesize_to_format[audioinfo.sampleSize] !=
                          mPaSampleSpec.format));

  mPaSampleSpec.channels = audioinfo.channels;
  mPaSampleSpec.rate = audioinfo.sampleRate;
  mPaSampleSpec.format = samplesize_to_format[audioinfo.sampleSize];
  mPlayLatency = pa_bytes_to_usec(mChunkBytes, &mPaSampleSpec) * 4;

  if (AM_LIKELY(!mIsPlayerStarted || needRestart)) {
    if (AM_LIKELY(mPaStreamPlayback && mPaThreadMainLoop)) {
      PA_MAINLOOP_LOCK(mPaThreadMainLoop);
      pa_stream_disconnect(mPaStreamPlayback);
      DEBUG("pa_stream_disconnect(mPaStreamPlayback)");
      pa_stream_unref(mPaStreamPlayback);
      mPaStreamPlayback = NULL;
      PA_MAINLOOP_UNLOCK(mPaThreadMainLoop);
    }
    ret = StartPlayer();
    mIsDraining = false;
    mIsPlayerStarted = (ret == ME_OK);
  }
#if 0
  else if (AM_LIKELY((audioinfo.sampleRate != mPaSampleSpec.rate))) {
    if (AM_LIKELY(mLoopRun && mPaStreamPlayback)) {
      pa_operation *op = pa_stream_update_sample_rate(mPaStreamPlayback,
                                                      audioinfo.sampleRate,
                                                      NULL,
                                                      NULL);
      if (AM_LIKELY(op)) {
        pa_operation_state opState;
        while ((opState = pa_operation_get_state(op)) != PA_OPERATION_DONE) {
          if (AM_UNLIKELY((opState == PA_OPERATION_CANCELLED))) {
            break;
          }
        }
        if (AM_LIKELY(opState == PA_OPERATION_DONE)) {
          ret = ME_OK;
        } else {
          ret = ME_ERROR;
          ERROR("Failed to update stream sample rate to %u",
                audioinfo.sampleRate);
        }
        pa_operation_unref(op);
      } else {
        ret = ME_ERROR;
      }
    }
  }
#endif
  return ret;
}

AM_ERR CPlayerPulse::Stop(bool wait)
{
  AUTO_LOCK(mMutex);
  AM_ERR ret = ME_OK;
  if (AM_LIKELY(mLoopRun)) {
    if (AM_LIKELY(wait)) {
      /* Wait for stream to complete */
      mEvent->Wait();
      NOTICE("Drained!");
    }
    pa_threaded_mainloop_stop(mPaThreadMainLoop);
    mLoopRun = false;
    Finalize();
  }
  while (mAudioQueue && !mAudioQueue->empty()) {
    mAudioQueue->front()->Release();
    mAudioQueue->pop();
  }

  return ret;
}

AM_ERR CPlayerPulse::Pause(bool enable)
{
  AUTO_LOCK(mMutex);
  AM_ERR ret = ME_OK;

  if (AM_LIKELY(mPaStreamPlayback && mLoopRun)) {
    PA_MAINLOOP_LOCK(mPaThreadMainLoop);
    if (AM_LIKELY((enable && (0 == pa_stream_is_corked(mPaStreamPlayback))) ||
                  (!enable && (1 == pa_stream_is_corked(mPaStreamPlayback))))) {
      pa_operation_state_t opState = PA_OPERATION_CANCELLED;
      pa_operation* paOp =
          pa_stream_cork(mPaStreamPlayback, (enable ? 1 : 0), NULL, NULL);

      PA_MAINLOOP_UNLOCK(mPaThreadMainLoop)
      while ((opState = pa_operation_get_state(paOp)) != PA_OPERATION_DONE) {
        if (AM_UNLIKELY(opState == PA_OPERATION_CANCELLED)) {
          WARN("Pause stream operation canceled!");
          break;
        }
        usleep(10000);
      }
      pa_operation_unref(paOp);
      PA_MAINLOOP_LOCK(mPaThreadMainLoop);
      ret = ((opState == PA_OPERATION_DONE) &&
          ((enable && (1 == pa_stream_is_corked(mPaStreamPlayback))) ||
              (!enable && (0 == pa_stream_is_corked(mPaStreamPlayback))))) ?
                  ME_OK : ME_ERROR;
      NOTICE("%s %s", (enable ? "Pause" : "Resume"),
             ((ret == ME_OK) ? "OK" : "Failed"));
      if (AM_LIKELY((ret == ME_OK) && !enable)) {
        pa_operation *op = pa_stream_trigger(mPaStreamPlayback, NULL, NULL);
        if (AM_LIKELY(op)) {
          pa_operation_unref(op);
        }
      }
    }
    PA_MAINLOOP_UNLOCK(mPaThreadMainLoop);
  } else {
    ERROR("Player is already stopped!");
    ret = ME_ERROR;
  }

  return ret;
}

bool CPlayerPulse::IsPlayerRunning()
{
  return (mLoopRun && mPaStreamPlayback &&
          (PA_STREAM_READY == pa_stream_get_state(mPaStreamPlayback)));
}

CPlayerPulse::CPlayerPulse() :
    CPlayerAudio(MAX_PCM_FRAME_SIZE),
    mPaThreadMainLoop(NULL),
    mPaContext(NULL),
    mPaStreamPlayback(NULL),
    mMutex(NULL),
    mEvent(NULL),
    mWriteData(NULL),
    mUnderFlow(NULL),
    mDrainData(NULL),
    mDefSinkName(NULL),
    mAudioQueue(NULL),
    mPlayLatency(20000),
    mUnderFlowCount(0),
    mPaContextState(PA_CONTEXT_UNCONNECTED),
    mCtxConnected(false),
    mLoopRun(false),
    mIsPlayerStarted(false),
    mIsDraining(false)
{
  memset(&mPaSampleSpec, 0, sizeof(mPaSampleSpec));
  memset(&mPaChannelMap, 0, sizeof(mPaChannelMap));
  memset(&mPaBufferAttr, 0, sizeof(mPaBufferAttr));
}

CPlayerPulse::~CPlayerPulse()
{
  Finalize();
  AM_DELETE(mMutex);
  AM_DELETE(mEvent);
  delete mWriteData;
  delete mUnderFlow;
  delete mDrainData;
  while (mAudioQueue && !mAudioQueue->empty()) {
    mAudioQueue->front()->Release();
    mAudioQueue->pop();
  }
  delete mAudioQueue;
}

AM_ERR CPlayerPulse::Construct()
{
  AM_ERR ret = ME_OK;
  do {
    if (AM_UNLIKELY(mChunkBytes == 0)) {
      ret = ME_ERROR;
      ERROR("Invalid chunk bytes: %u!", mChunkBytes);
      break;
    }

    if (AM_UNLIKELY(NULL == (mMutex = CMutex::Create()))) {
      ret = ME_NO_MEMORY;
      ERROR("Failed to create mutex!");
      break;
    }

    if (AM_UNLIKELY(NULL == (mEvent = CEvent::Create()))) {
      ret = ME_NO_MEMORY;
      ERROR("Failed to create event!");
      break;
    }

    if (AM_UNLIKELY(NULL == (mWriteData = new PaData(this, NULL)))) {
      ret = ME_NO_MEMORY;
      ERROR("Failed to create write data!");
      break;
    }

    if (AM_UNLIKELY(NULL == (mUnderFlow = new PaData(this, NULL)))) {
      ret = ME_NO_MEMORY;
      ERROR("Failed to create under flow data!");
      break;
    }

    if (AM_UNLIKELY(NULL == (mDrainData = new PaData(this, NULL)))) {
      ret = ME_NO_MEMORY;
      ERROR("Failed to create drain data!");
      break;
    }

    if (AM_UNLIKELY(NULL == (mAudioQueue = new PacketQueue()))) {
      ret = ME_NO_MEMORY;
      ERROR("Failed to create audio queue!");
      break;
    }
  }while(0);

  return ret;
}

AM_ERR CPlayerPulse::Initialize()
{
  AM_ERR ret = ME_OK;
  AUTO_LOCK(mMutex);
  do {
    if (AM_LIKELY(NULL == mPaThreadMainLoop)) {
      mPaThreadMainLoop = pa_threaded_mainloop_new();
      if (AM_LIKELY(mPaThreadMainLoop)) {
        mPaContext = pa_context_new(
            pa_threaded_mainloop_get_api(mPaThreadMainLoop), CONTEXT_NAME);
        if (AM_UNLIKELY(!mPaContext)) {
          ret = ME_ERROR;
          ERROR("Failed to new pulse audio context!");
          break;
        } else {
          PaData state(this, mPaThreadMainLoop);
          pa_context_set_state_callback(mPaContext, StaticPaState, &state);
          pa_context_connect(mPaContext, NULL, PA_CONTEXT_NOFLAGS, NULL);

          if (AM_UNLIKELY(0 != pa_threaded_mainloop_start(mPaThreadMainLoop))) {
            ret = ME_ERROR;
            ERROR("Failed to start threaded mainloop!");
            break;
          } else {
            PA_MAINLOOP_LOCK(mPaThreadMainLoop);
            while (mPaContextState != PA_CONTEXT_READY) {
              pa_threaded_mainloop_wait(mPaThreadMainLoop);
            }
            PA_MAINLOOP_UNLOCK(mPaThreadMainLoop);
            pa_context_set_state_callback(mPaContext, NULL, NULL);
            mCtxConnected = (mPaContextState == PA_CONTEXT_READY);
            if (AM_UNLIKELY(!mCtxConnected)) {
              if ((mPaContextState == PA_CONTEXT_TERMINATED) ||
                  (mPaContextState == PA_CONTEXT_FAILED)) {
                PA_MAINLOOP_LOCK(mPaThreadMainLoop);
                pa_context_disconnect(mPaContext);
                PA_MAINLOOP_UNLOCK(mPaThreadMainLoop);
              }
              mCtxConnected = false;
              ERROR("pa_context_connect failed: %u!", mPaContextState);
              pa_threaded_mainloop_stop(mPaThreadMainLoop);
              ret = ME_ERROR;
              break;
            }
          }
        }
      } else {
        ERROR("Failed to create PaThreadMainloop!");
        ret = ME_ERROR;
        break;
      }
    } else {
      NOTICE("PaThreadMainloop already created!");
    }
    if (AM_LIKELY(ME_OK == ret)) {
      PaData servInfo(this, mPaThreadMainLoop);
      pa_operation *paOp = NULL;
      pa_operation_state_t opState;
      PA_MAINLOOP_LOCK(mPaThreadMainLoop);
      paOp = pa_context_get_server_info(mPaContext,
                                        StaticPaServerInfo,
                                        &servInfo);
      while ((opState = pa_operation_get_state(paOp)) !=
          PA_OPERATION_DONE) {
        if (AM_UNLIKELY(opState == PA_OPERATION_CANCELLED)) {
          WARN("Get server info operation canceled!");
          break;
        }
        pa_threaded_mainloop_wait(mPaThreadMainLoop);
      }
      pa_operation_unref(paOp);
      PA_MAINLOOP_UNLOCK(mPaThreadMainLoop);

      if (AM_LIKELY(opState != PA_OPERATION_DONE)) {
        ret = ME_ERROR;
        pa_threaded_mainloop_stop(mPaThreadMainLoop);
        break;
      }
    }
  }while(0);

  mLoopRun = (ret == ME_OK);

  return ret;
}

void CPlayerPulse::Finalize()
{
  if (AM_LIKELY(mPaStreamPlayback)) {
    pa_stream_disconnect(mPaStreamPlayback);
    DEBUG("pa_stream_disconnect(mPaStreamPlayback)");
    pa_stream_unref(mPaStreamPlayback);
    mPaStreamPlayback = NULL;
  }

  if (AM_LIKELY(mCtxConnected)) {
    pa_context_set_state_callback(mPaContext, NULL, NULL);
    pa_context_disconnect(mPaContext);
    mCtxConnected = false;
    DEBUG("pa_context_disconnect(mPaContext)");
  }

  if (AM_LIKELY(mPaContext)) {
    pa_context_unref(mPaContext);
    mPaContext = NULL;
    DEBUG("pa_context_unref(mPaContext)");
  }

  if (AM_LIKELY(mPaThreadMainLoop)) {
    pa_threaded_mainloop_free(mPaThreadMainLoop);
    mPaThreadMainLoop = NULL;
    DEBUG("pa_threaded_mainloop_free(mPaThreadMainLoop)");
  }

  delete[] mDefSinkName;
  mDefSinkName = NULL;
  mPlayLatency = 20000;
  mUnderFlowCount = 0;
  mIsPlayerStarted = false;
  mIsDraining = false;
  INFO("PulseAudio's resource released!");
}

inline void CPlayerPulse::PaState(pa_context *context, void *data)
{
  if (AM_LIKELY(context)) {
    mPaContextState = pa_context_get_state(context);
  }
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
  DEBUG("PaState called!");
}

inline void CPlayerPulse::PaServerInfo(pa_context *context,
                                       const pa_server_info *info,
                                       void *data)
{
  mDefSinkName = amstrdup(info->default_sink_name);
  memcpy(&mPaChannelMap, &info->channel_map, sizeof(info->channel_map));
  switch(mPaSampleSpec.channels) {
    case 1 :
      pa_channel_map_init_mono(&mPaChannelMap);
      break;
    case 2 :
      pa_channel_map_init_stereo(&mPaChannelMap);
      break;
    default:
      pa_channel_map_init_auto(&mPaChannelMap,
                               mPaSampleSpec.channels,
                               PA_CHANNEL_MAP_ALSA);
      break;
  }

  INFO("Audio Server Information:");
  INFO("          Server Version: %s",   info->server_version);
  INFO("             Server Name: %s",   info->server_name);
  INFO("     Default Source Name: %s",   info->default_source_name);
  INFO("       Default Sink Name: %s",   info->default_sink_name);
  INFO("               Host Name: %s",   info->host_name);
  INFO("               User Name: %s",   info->user_name);
  INFO("                Channels: %hhu", info->sample_spec.channels);
  INFO("                    Rate: %u",   info->sample_spec.rate);
  INFO("              Frame Size: %u",   pa_frame_size(&info->sample_spec));
  INFO("             Sample Size: %u",   pa_sample_size(&info->sample_spec));
  INFO(" Def ChannelMap Channels: %hhu", info->channel_map.channels);
  INFO("                  Cookie: %08x", info->cookie);
  INFO("       Audio Information:");
  INFO("                Channels: %hhu", mPaSampleSpec.channels);
  INFO("             Sample Rate: %u",   mPaSampleSpec.rate);
  INFO("     ChannelMap Channels: %hhu", mPaChannelMap.channels);
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
}

inline void CPlayerPulse::PaWrite(pa_stream *stream, size_t bytes, void *data)
{
  CPacket    *packet = NULL;
  AM_U8     *dataPtr = NULL;
  AM_U8   *dataWrite = NULL;
  AM_UINT  availSize = 0;
  AM_U32  dataOffset = 0;
  AM_U32 writtenSize = 0;
  size_t    dataSize = bytes;
  int            ret = 0;

  PA_MAINLOOP_LOCK(mPaThreadMainLoop);
  if (AM_LIKELY(0 == (ret = pa_stream_begin_write(stream,
                                                  (void**)&dataWrite,
                                                  &dataSize)))) {
    bool needDrain = false;
    while ((!mAudioQueue->empty()) && (writtenSize < dataSize)) {
      packet = mAudioQueue->front();
      if (AM_LIKELY(packet->GetType() == CPacket::AM_PAYLOAD_TYPE_DATA)) {
        dataOffset = packet->GetDataOffset();
        dataPtr = (AM_U8*) (packet->GetDataPtr() + dataOffset);
        availSize = packet->GetDataSize() - dataOffset;
        availSize =
            (availSize < (dataSize - writtenSize)) ? availSize :
                (dataSize - writtenSize);
        memcpy(dataWrite + writtenSize, dataPtr, availSize);
        writtenSize += availSize;
        dataOffset += availSize;
        if (AM_LIKELY(dataOffset == packet->GetDataSize())) {
          mAudioQueue->pop();
          packet->SetDataOffset(0);
          packet->Release();
        } else {
          packet->SetDataOffset(dataOffset);
        }
      } else if (packet->GetType() == CPacket::AM_PAYLOAD_TYPE_EOF) {
        needDrain = true;
        mAudioQueue->pop();
        packet->Release();
        break;
      }
    }
    if (AM_UNLIKELY(mAudioQueue->empty() && (0 == writtenSize))) {
      memset(dataWrite, 0, dataSize);
      writtenSize = dataSize;
    }
    if (AM_LIKELY(writtenSize > 0)) {
      pa_stream_write(stream,
                      dataWrite,
                      writtenSize,
                      NULL,
                      0,
                      PA_SEEK_RELATIVE);
    } else {
      pa_stream_cancel_write(stream);
    }
    if (AM_UNLIKELY(needDrain)) {
      pa_operation *op = NULL;
      mIsDraining = true;
      pa_stream_set_underflow_callback(stream, NULL, NULL);
      op = pa_stream_drain(stream, StaticPaDrain, mDrainData);
      if (AM_LIKELY(op)) {
        pa_operation_unref(op);
        PA_MAINLOOP_UNLOCK(mPaThreadMainLoop);
        pa_threaded_mainloop_wait(mPaThreadMainLoop);
        PA_MAINLOOP_LOCK(mPaThreadMainLoop);
      } else {
        mEvent->Signal();
      }
      NOTICE("Draining!");
    }
  } else {
    ERROR("Error occurred: %s", pa_strerror(ret));
  }
  PA_MAINLOOP_UNLOCK(mPaThreadMainLoop);
}

inline void CPlayerPulse::PaUnderflow(pa_stream *stream, void *data)
{
  if (AM_UNLIKELY((++ mUnderFlowCount >= 6) && (mPlayLatency < 5000000))) {
    mPlayLatency = mPlayLatency * 4;
    mPaBufferAttr.maxlength = pa_usec_to_bytes(mPlayLatency, &mPaSampleSpec);
    mPaBufferAttr.tlength = pa_usec_to_bytes(mPlayLatency, &mPaSampleSpec);
    pa_stream_set_buffer_attr(stream, (const pa_buffer_attr*)&mPaBufferAttr,
                              NULL, NULL);
    mUnderFlowCount = 0;
    NOTICE("Underflow, increase latency to %u ms!", mPlayLatency / 1000);
  }
}

inline void CPlayerPulse::PaDrain(pa_stream *stream, int success, void *data)
{
  pa_threaded_mainloop_signal(mPaThreadMainLoop, 0);
  mEvent->Signal();
}

void CPlayerPulse::StaticPaState(pa_context *context, void *data)
{
  ((PaData*)data)->adev->PaState(context, ((PaData*)data)->data);
}

void CPlayerPulse::StaticPaServerInfo(pa_context *context,
                                      const pa_server_info *info,
                                      void *data)
{
  ((PaData*)data)->adev->PaServerInfo(context, info, ((PaData*)data)->data);
}

void CPlayerPulse::StaticPaWrite(pa_stream *stream, size_t bytes, void *data)
{
  ((PaData*)data)->adev->PaWrite(stream, bytes, ((PaData*)data)->data);
}

void CPlayerPulse::StaticPaUnderflow(pa_stream *stream, void *data)
{
  ((PaData*)data)->adev->PaUnderflow(stream, ((PaData*)data)->data);
}

void CPlayerPulse::StaticPaDrain(pa_stream *stream, int success, void *data)
{
  ((PaData*)data)->adev->PaDrain(stream, success, ((PaData*)data)->data);
}
