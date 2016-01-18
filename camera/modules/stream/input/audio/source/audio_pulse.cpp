/*******************************************************************************
 * audio_pulse.cpp
 *
 * Histroy:
 *   2012-9-18 - [ypchang] created file
 *   2013-1-24 - [Hanbo Xiao] Modified file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_data.h"

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

#include "audio_codec_info.h"
#include "audio_capture_if.h"
#include "audio_device.h"
#include "audio_capture.h"

#define CONTEXT_NAME ((const char*)"AudioCapture")
#define STREAM_NAME  ((const char*)"AudioCaptureStream")
#define HW_TIMER     ((const char*)"/proc/ambarella/ambarella_hwtimer")

struct PaData {
    CPulseAudio *adev;
    void        *data;
    PaData(CPulseAudio *dev, void *userdata) :
      adev(dev),
      data(userdata){}
    PaData() :
      adev(NULL),
      data(NULL){}
};

CPulseAudio::CPulseAudio(CAudioCapture *aCapture,
                         PacketQueue *queue) :
  CAudioDevice(),
  mPaThreadMainLoop(NULL),
  mPaMainLoopApi(NULL),
  mPaContext(NULL),
  mPaStreamRecord(NULL),
  mPacketQ(queue),
  mDefSrcName(NULL),
  mAudioCapture(aCapture),
  mMutex(NULL),
  mAudioBuffer(NULL),
  mAudioRdPtr(NULL),
  mAudioWtPtr(NULL),
  mReadData(NULL),
  mOverFlow(NULL),
  mPaContextState(PA_CONTEXT_UNCONNECTED),
  mCtxConnected(false),
  mLoopRun(false),
  mFrameBytes(0),
  mAudioBufSize(0),
  mHwTimerFd(-1),
  mLastPts(0LLU),
  mFragmentPts(0LLU),
  mMainLoopRet(ME_OK)
{
  memset(&mPaSampleSpec, 0, sizeof(mPaSampleSpec));
  memset(&mPaChannelMap, 0, sizeof(mPaChannelMap));
}

CPulseAudio::~CPulseAudio()
{
  Fini();
  delete mReadData;
  delete mOverFlow;
  AM_DELETE(mMutex);
}

AM_ERR CPulseAudio::Construct()
{
  AM_ERR ret = ME_OK;
  do {
    if (AM_UNLIKELY(NULL == (mMutex = CMutex::Create()))) {
      /* Not recursive */
      ret = ME_NO_MEMORY;
      ERROR("Failed to create mutex!");
      break;
    }

    if (AM_UNLIKELY(NULL == (mReadData = new PaData(this, NULL)))) {
      ret = ME_NO_MEMORY;
      ERROR("Failed to create read data");
      break;
    }

    if (AM_UNLIKELY(NULL == (mOverFlow = new PaData(this, NULL)))) {
      ret = ME_NO_MEMORY;
      ERROR("Failed to create over flow data");
      break;
    }

  }while(0);

  return ret;
}

AM_ERR CPulseAudio::Start()
{
  AM_ERR ret = Init();
  AUTO_LOCK(mMutex);

  if (AM_LIKELY(ME_OK == ret)) {
    PaData servInfo(this, mPaThreadMainLoop);
    pa_operation *paOp = NULL;
    pa_operation_state_t opState;

    pa_threaded_mainloop_lock(mPaThreadMainLoop);
    paOp = pa_context_get_server_info(mPaContext,
                                      StaticPaServerInfo,
                                      &servInfo);
    while ((opState = pa_operation_get_state(paOp)) != PA_OPERATION_DONE) {
      if (AM_UNLIKELY(opState == PA_OPERATION_CANCELLED)) {
        WARN("Get server info operation canceled!");
        break;
      }
      pa_threaded_mainloop_wait(mPaThreadMainLoop);
    }
    pa_operation_unref(paOp);
    pa_threaded_mainloop_unlock(mPaThreadMainLoop);

    if (AM_LIKELY(opState == PA_OPERATION_DONE)) {
      pa_buffer_attr bufAttr = { (AM_UINT) -1 };
      if (AM_LIKELY(mAudioBuffer)) {
        /* Send audio info to down stream */
        while (mPacketQ->size() < mAudioStreamCnt) {
          usleep(10); /* Wait for packet to send info data */
        }
        for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
          if (AM_LIKELY(mAudioStreamMap & (1 << i))) {
            CPacket *audioInfoPkt = mPacketQ->front();
            CPacket::Payload *payload =
                audioInfoPkt->GetPayload(CPacket::AM_PAYLOAD_TYPE_INFO,
                                         CPacket::AM_PAYLOAD_ATTR_AUDIO);
            mPacketQ->pop();
            if (AM_LIKELY(ME_OK ==
                (ret = GetAudioParameter(
                    (AM_AUDIO_INFO*)(payload->mData.mBuffer), i)))) {
              payload->mData.mSize = sizeof(AM_AUDIO_INFO);
              payload->mData.mStreamId = i;
              mAudioCapture->SendBuffer(audioInfoPkt);
              INFO("Send audio info to stream%u", i);
            } else {
              ERROR("Failed to get audio information!");
              ret = ME_ERROR;
              break;
            }
          } else {
            INFO("Audio is not enabled for stream%u", i);
          }
        }
        mPaStreamRecord = pa_stream_new(mPaContext,
                                        "AudioRecord",
                                        &mPaSampleSpec,
                                        &mPaChannelMap);
        if (AM_LIKELY(mPaStreamRecord)) {
          bufAttr.fragsize = mAudioChunkBytes * 5;
          mReadData->data  = mPaThreadMainLoop;
          mOverFlow->data  = mPaThreadMainLoop;

          pa_stream_set_read_callback(mPaStreamRecord, StaticPaRead, mReadData);
          pa_stream_set_overflow_callback(mPaStreamRecord,
                                          StaticPaOverflow,
                                          mOverFlow);

          if (AM_UNLIKELY(pa_stream_connect_record(
              mPaStreamRecord,
              mDefSrcName, /* Set to NULL for Default Source Device */
              &bufAttr, /* Default Buffer Attribute */
              (pa_stream_flags)(PA_STREAM_INTERPOLATE_TIMING |
                  PA_STREAM_ADJUST_LATENCY |
                  PA_STREAM_AUTO_TIMING_UPDATE)) < 0)) {
            ERROR("Failed to connect with record stream!");
            ret = ME_ERROR;
          } else {
            pa_stream_state_t streamState;
            while (PA_STREAM_READY !=
                (streamState = pa_stream_get_state(mPaStreamRecord))) {
              if (AM_UNLIKELY((streamState == PA_STREAM_FAILED) ||
                              (streamState == PA_STREAM_TERMINATED))) {
                ret = ME_ERROR;
                break;
              }
            }

            if (AM_LIKELY(PA_STREAM_READY == streamState)) {
              const pa_buffer_attr *attr =
                  pa_stream_get_buffer_attr(mPaStreamRecord);
              if (AM_LIKELY(attr != NULL)) {
                INFO("Client requested fragment size: %u", bufAttr.fragsize);
                INFO(" Server returned fragment size: %u", attr->fragsize);
              } else {
                ERROR("Failed to get buffer attribute!");
                ret = ME_ERROR;
              }
            } else {
              ret = ME_ERROR;
              ERROR("Failed to connect record stream to audio server!");
            }
          }
        } else {
          ret = ME_ERROR;
          ERROR("Failed to create record stream: %s!",
                pa_strerror(pa_context_errno(mPaContext)));
        }
      } else {
        ret = ME_ERROR;
      }
    } else {
      ret = ME_ERROR;
    }
  }
  mLoopRun = (ret == ME_OK);

  return ret;
}

AM_ERR CPulseAudio::Stop()
{
  AUTO_LOCK(mMutex);
  AM_PTS remainDataPtsStart = 0;
  AM_UINT             count = 0;
  AM_UINT         waitCount = 0;
  CPacket            *audio = NULL;
  CPacket         *audioEos = NULL;
  CPacket::Payload *payload = NULL;

  if (AM_LIKELY(mLoopRun)) {
    pa_threaded_mainloop_stop(mPaThreadMainLoop);
    mLoopRun = false;
    if (AM_LIKELY(mAudioStreamMap > 0)) {
      remainDataPtsStart = GetCurrentPts() - ((90000 * GetAvailDataSize()) /
              (pa_frame_size(&mPaSampleSpec) * mPaSampleSpec.rate));
      count = (GetAvailDataSize() % mAudioChunkBytes) ?
          GetAvailDataSize() / mAudioChunkBytes + 1 :
          GetAvailDataSize() / mAudioChunkBytes;
      waitCount = 0;

      for (AM_UINT i = 0; i < count; ++ i) {
        if (AM_LIKELY(mPacketQ->size() > mAudioStreamCnt)) {
          CPacket::Payload *payload = NULL;
          audio                     = mPacketQ->front();
          payload = audio->GetPayload(CPacket::AM_PAYLOAD_TYPE_DATA,
                                      CPacket::AM_PAYLOAD_ATTR_AUDIO);
          AM_UINT dataSize = (GetAvailDataSize() >= mAudioChunkBytes) ?
              mAudioChunkBytes : GetAvailDataSize();
          payload->mData.mSize       = mAudioChunkBytes;
          payload->mData.mFrameType  = 0; /* Todo: Set to audio type */
          payload->mData.mPayloadPts =
              remainDataPtsStart + (mFragmentPts * (i + 1));
          if (AM_UNLIKELY(dataSize < mAudioChunkBytes)) {
            memset(payload->mData.mBuffer, 0, mAudioChunkBytes);
          }
          memcpy(payload->mData.mBuffer, mAudioRdPtr, dataSize);
          mAudioRdPtr = mAudioBuffer +
              (((mAudioRdPtr - mAudioBuffer) + mAudioChunkBytes) %
                  mAudioBufSize);
          mAudioCapture->SendBuffer(audio);
          mPacketQ->pop();
          DEBUG("Get Audio data: %u bytes, PTS is %llu, queue size %d",
                payload->mData.mSize, payload->mData.mPayloadPts,
                mPacketQ->size());
        } else {
          while (mPacketQ->size() <= mAudioStreamCnt) {
            NOTICE("Waiting for packets to send remaining audio data!");
            ++ waitCount;
            usleep(10000); /* 10 ms */
            if (AM_LIKELY(++ waitCount >= 100)) {
              NOTICE("Not enough packets to send remaining data!");
              break;
            }
          }
        }
        if (AM_LIKELY(waitCount >= 100)) {
          break;
        }
      }
      for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
        if (AM_LIKELY(mAudioStreamMap & (1 << i))) {
          while (mPacketQ->empty()) {
            NOTICE("Waiting for a packet to send EOS!");
            usleep(10000); /* 10 ms */
          }
          INFO("PulseAudio Send Audio EOS to stream%u!", i);
          audioEos = mPacketQ->front();
          payload = audioEos->GetPayload(CPacket::AM_PAYLOAD_TYPE_EOS,
                                         CPacket::AM_PAYLOAD_ATTR_AUDIO);
          payload->mData.mPayloadPts =
              remainDataPtsStart + (mFragmentPts * (count + 1));
          payload->mData.mStreamId  = i;
          payload->mData.mSize      = 0;
          payload->mData.mFrameType = 0;
          mAudioCapture->SendBuffer(audioEos);
          mPacketQ->pop();
        }
      }
    }
    /* Purge all packet in queue */
    while (!mPacketQ->empty()) {
      mPacketQ->front()->Release();
      mPacketQ->pop();
    }
  } else {
    NOTICE("PulseAudio is already stopped!");
  }
  Fini();

  return ME_OK;
}

AM_ERR CPulseAudio::GetAudioParameter(AM_AUDIO_INFO *info, AM_UINT streamid)
{
  info->needsync   = mAvSyncMap & (1 << streamid);
  info->sampleRate = mPaSampleSpec.rate;
  info->channels   = mPaSampleSpec.channels;
  info->pktPtsIncr = mFragmentPts;
  info->sampleSize = pa_sample_size(&mPaSampleSpec);
  info->chunkSize  = mAudioChunkBytes;
  return ME_OK;
}

void CPulseAudio::SetAudioStreamMap(AM_UINT map)
{
  mAudioStreamMap = map;
  for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
    if (AM_LIKELY(mAudioStreamMap & (1 << i))) {
       ++ mAudioStreamCnt;
    }
  }
}

void CPulseAudio::SetAvSyncMap(AM_UINT avSyncMap)
{
  mAvSyncMap = avSyncMap;
}

void CPulseAudio::PaState(pa_context *context, void *data)
{
  if (AM_LIKELY(context)) {
    mPaContextState = pa_context_get_state(context);
  }
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
  DEBUG("PaState called!");
}

void CPulseAudio::PaServerInfo(pa_context *context,
                               const pa_server_info *info,
                               void *data)
{
  INFO("Audio Server Information:");
  INFO("       Server Version: %s",   info->server_version);
  INFO("          Server Name: %s",   info->server_name);
  INFO("  Default Source Name: %s",   info->default_source_name);
  INFO("    Default Sink Name: %s",   info->default_sink_name);
  INFO("            Host Name: %s",   info->host_name);
  INFO("            User Name: %s",   info->user_name);
  INFO("             Channels: %hhu", info->sample_spec.channels);
  INFO("                 Rate: %u",   info->sample_spec.rate);
  INFO("           Frame Size: %u",   pa_frame_size(&info->sample_spec));
  INFO("          Sample Size: %u",   pa_sample_size(&info->sample_spec));
  INFO("  ChannelMap Channels: %hhu", info->channel_map.channels);
  memcpy(&mPaSampleSpec, &info->sample_spec, sizeof(info->sample_spec));
  memcpy(&mPaChannelMap, &info->channel_map, sizeof(info->channel_map));
  mPaSampleSpec.rate = mAudioSampleRate;
  mPaSampleSpec.channels = mAudioChannel;
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
  INFO(" Client Configuration:");
  INFO("             Channels: %hhu", mPaSampleSpec.channels);
  INFO("                 Rate: %u",   mPaSampleSpec.rate);
  INFO("  ChannelMap Channels: %hhu", mPaChannelMap.channels);
  switch(mAudioCodecType) {
    case AM_AUDIO_CODEC_AAC:
    case AM_AUDIO_CODEC_PCM:
    case AM_AUDIO_CODEC_BPCM: {
      mAudioChunkBytes = 2048 * mAudioChannel * pa_sample_size(&mPaSampleSpec);
    }break;
    case AM_AUDIO_CODEC_G726: {
      mAudioChunkBytes = (80 * mAudioSampleRate / 1000) * mAudioChannel *
          pa_sample_size(&mPaSampleSpec);
    }break;
    case AM_AUDIO_CODEC_OPUS: {
      AM_UINT baseSize = (20 * mAudioSampleRate / 1000) * mAudioChannel *
          pa_sample_size(&mPaSampleSpec);
      mAudioChunkBytes = 0;
      while ((mAudioChunkBytes + baseSize) <= 8192) {
        mAudioChunkBytes += baseSize;
      }
    }break;
    case AM_AUDIO_CODEC_NONE:
    default: {
      mAudioChunkBytes = 0;
    }break;
  }
  mDefSrcName   = amstrdup(info->default_source_name);
  mFrameBytes   = pa_frame_size(&mPaSampleSpec) * mPaSampleSpec.rate;
  if (AM_LIKELY(mAudioChunkBytes > 0)) {
    mAudioBufSize = GetLcm(mAudioChunkBytes, mFrameBytes);
    mFragmentPts  = (90000 * mAudioChunkBytes) / mFrameBytes;
    INFO("         Fragment PTS: %llu", mFragmentPts);
    mAudioBuffer = new AM_U8[mAudioBufSize];
    if (AM_UNLIKELY(NULL == mAudioBuffer)) {
      ERROR("Failed to allocate audio buffer!");
    } else {
      mAudioRdPtr = mAudioBuffer;
      mAudioWtPtr = mAudioBuffer;
      INFO("Allocated %u bytes audio buffer, this will buffer %u seconds!",
           mAudioBufSize, mAudioBufSize / mFrameBytes);
    }
  } else {
    ERROR("Invalid audio chunk size %u bytes", mAudioChunkBytes);
  }
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
}

void CPulseAudio::PaRead(pa_stream *stream, size_t bytes, void *userData)
{
  AM_PTS currPts = GetCurrentPts();
  if (AM_LIKELY(mPacketQ && (mAudioStreamMap > 0))) {
    const void *data = NULL;
    AM_UINT availDataSize = 0;
    if (AM_UNLIKELY(pa_stream_peek(stream, &data, &bytes) < 0)) {
      ERROR("pa_stream_peek() failed: %s",
            pa_strerror(pa_context_errno(mPaContext)));
    } else if (AM_LIKELY(data && (bytes > 0))) {
      memcpy(mAudioWtPtr, data, bytes);
      mAudioWtPtr = mAudioBuffer +
          (((mAudioWtPtr - mAudioBuffer) + bytes) % mAudioBufSize);
    }
    pa_stream_drop(stream);
    availDataSize = GetAvailDataSize();
    mLastPts = ((mLastPts == 0) ?
        currPts - ((availDataSize * mFragmentPts) / mAudioChunkBytes) :
        mLastPts);

    if (AM_UNLIKELY(availDataSize >= mAudioChunkBytes)) {
      AM_PTS realPtsIncr = currPts - mLastPts;
      AM_PTS  currPtsSeg = ((mAudioChunkBytes * realPtsIncr) / availDataSize);
      AM_UINT  pktNumber = availDataSize / mAudioChunkBytes;

#if 0
      NOTICE("Data: %u bytes, Current PTS: %llu, Last PTS: %llu, "
             "PTS incremental is %llu, %u chunks, "
             "PTS segment/chunk is %llu",
             GetAvailDataSize(), currPts, mLastPts,
             realPtsIncr, (GetAvailDataSize() / mChunkBytes), currPtsSeg);
#endif

      for (AM_UINT i = 0; i < pktNumber; ++ i) {
        if (AM_LIKELY(mPacketQ->size() > mAudioStreamCnt)) {
          CPacket *audio = mPacketQ->front();
          CPacket::Payload *payload =
              audio->GetPayload(CPacket::AM_PAYLOAD_TYPE_DATA,
                                CPacket::AM_PAYLOAD_ATTR_AUDIO);

          mPacketQ->pop();
          mLastPts += currPtsSeg;
          payload->mData.mSize = mAudioChunkBytes;
          payload->mData.mFrameType = 0; /* Todo: Set to audio type */
          payload->mData.mPayloadPts = mLastPts;
          memcpy(payload->mData.mBuffer, mAudioRdPtr, mAudioChunkBytes);
          mAudioCapture->SendBuffer(audio);

          mAudioRdPtr = mAudioBuffer +
              (((mAudioRdPtr-mAudioBuffer) + mAudioChunkBytes) % mAudioBufSize);
        } else {
          ERROR("Packet queue is empty! Send audio next time!");
          break;
        }
      }
    }
  } else if (AM_UNLIKELY(!mPacketQ)) {
    ERROR("PacketQ is not initialized!");
  } else { /* Audio is not initialized for any stream */
    if (AM_LIKELY(!mPacketQ->empty())) {
      mPacketQ->front()->Release();
      mPacketQ->pop();
    }
  }
}

void CPulseAudio::PaOverflow(pa_stream *stream, void *data)
{
  ERROR("Data Overflow!");
}

AM_ERR CPulseAudio::Init()
{
  AM_ERR ret = ME_OK;
  AUTO_LOCK(mMutex);
  do {
    mLastPts = 0;
    if (AM_UNLIKELY(NULL == mAudioCapture)) {
      ERROR("AudioCapture is not initialized!");
      ret = ME_ERROR;
      break;
    }
    if (AM_LIKELY(mHwTimerFd < 0)) {
      if (AM_UNLIKELY((mHwTimerFd = open(HW_TIMER, O_RDONLY)) < 0)) {
        PERROR("open");
        ret = ME_ERROR;
        break;
      }
    }
    if (AM_LIKELY(NULL == mPaThreadMainLoop)) {
      mPaThreadMainLoop = pa_threaded_mainloop_new();
      if (AM_LIKELY(mPaThreadMainLoop)) {
        mPaMainLoopApi = pa_threaded_mainloop_get_api(mPaThreadMainLoop);
        mPaContext = pa_context_new(mPaMainLoopApi, CONTEXT_NAME);
        if (AM_UNLIKELY(NULL == mPaContext)) {
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
            pa_threaded_mainloop_lock(mPaThreadMainLoop);
            while (mPaContextState != PA_CONTEXT_READY) {
              if (AM_UNLIKELY((mPaContextState == PA_CONTEXT_FAILED) ||
                              (mPaContextState == PA_CONTEXT_TERMINATED))) {
                break;
              }
              pa_threaded_mainloop_wait(mPaThreadMainLoop);
            }
            pa_threaded_mainloop_unlock(mPaThreadMainLoop);
            pa_context_set_state_callback(mPaContext, NULL, NULL);
            mCtxConnected = (mPaContextState == PA_CONTEXT_READY);
            if (AM_UNLIKELY(!mCtxConnected)) {
              if ((mPaContextState == PA_CONTEXT_TERMINATED) ||
                  (mPaContextState == PA_CONTEXT_FAILED)) {
                pa_threaded_mainloop_lock(mPaThreadMainLoop);
                pa_context_disconnect(mPaContext);
                pa_threaded_mainloop_unlock(mPaThreadMainLoop);
              }
              mCtxConnected = false;
              ERROR("pa_context_connect failed: %u!", mPaContextState);
              ret = ME_ERROR;
              pa_threaded_mainloop_stop(mPaThreadMainLoop);
              break;
            }
          }
        }
      } else {
        ret = ME_ERROR;
        ERROR("Failed to new PaThreadMainloop!");
      }
    } else {
      NOTICE("PaThreadMainloop already created!");
    }
  } while(0);

  return ret;
}

void CPulseAudio::Fini()
{
  if (AM_LIKELY(mPaStreamRecord)) {
    pa_stream_disconnect(mPaStreamRecord);
    DEBUG("pa_stream_disconnect(mPaStreamRecord)");
    pa_stream_unref(mPaStreamRecord);
    mPaStreamRecord = NULL;
    DEBUG("pa_stream_unref(mPaStreamRecord)");
  }
  if (AM_LIKELY(mCtxConnected)) {
    pa_context_set_state_callback(mPaContext, NULL, NULL);
    pa_context_disconnect(mPaContext);
    mCtxConnected = false;
    DEBUG("pa_context_disconnect");
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
  if (AM_LIKELY(mHwTimerFd > 0)) {
    close(mHwTimerFd);
    mHwTimerFd = -1;
  }
  delete[] mDefSrcName;
  mDefSrcName = NULL;
  delete[] mAudioBuffer;
  mAudioBuffer = NULL;
  mAudioRdPtr = NULL;
  mAudioWtPtr = NULL;
  INFO("PulseAudio's resource released!");
}

inline AM_PTS CPulseAudio::GetCurrentPts()
{
  AM_U8 pts[32] = {0};
  AM_PTS currPts = mLastPts;
  if (AM_LIKELY(mHwTimerFd >= 0)) {
    if (AM_UNLIKELY(read(mHwTimerFd, pts, sizeof(pts)) < 0)) {
      PERROR("read");
    } else {
      currPts = strtoull((const char*)pts, (char **)NULL, 10);
      if (AM_UNLIKELY(currPts < mLastPts)) {
        ERROR("Hardware timer error, Last PTS: %llu, Current PTS: %llu, "
              "reset Current PTS to Last PTS: %llu",
              mLastPts, currPts, mLastPts);
        currPts = mLastPts;
      }
    }
  }
  return currPts;
}

inline AM_UINT CPulseAudio::GetLcm(AM_UINT a, AM_UINT b)
{
  AM_UINT c = a;
  AM_UINT d = b;
  while(((c > d) ? (c %= d) : (d %= c)));
  return (a * b) / (c + d);
}

inline AM_UINT CPulseAudio::GetAvailDataSize()
{
  return (AM_UINT)((mAudioWtPtr >= mAudioRdPtr) ? (mAudioWtPtr - mAudioRdPtr) :
      (mAudioBufSize + mAudioWtPtr - mAudioRdPtr));
}

void CPulseAudio::StaticPaState(pa_context *context, void *data)
{
  ((PaData*)data)->adev->PaState(context, ((PaData*)data)->data);
}

void CPulseAudio::StaticPaServerInfo(pa_context *context,
                                     const pa_server_info *info,
                                     void *data)
{
  ((PaData*)data)->adev->PaServerInfo(context, info, ((PaData*)data)->data);
}

void CPulseAudio::StaticPaRead(pa_stream *stream,
                                       size_t bytes,
                                       void *data)
{
  ((PaData*)data)->adev->PaRead(stream, bytes, ((PaData*)data)->data);
}

void CPulseAudio::StaticPaOverflow(pa_stream *stream,
                                   void *data)
{
  ((PaData*)data)->adev->PaOverflow(stream, ((PaData*)data)->data);
}

