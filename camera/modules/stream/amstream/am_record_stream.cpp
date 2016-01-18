/*******************************************************************************
 * am_record_stream.cpp
 *
 * History:
 *   2012-10-16 - [ypchang] created file
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

#include "audio_codec_info.h"
#include "am_muxer_info.h"

#include "stream_module_info.h"
#include "record_engine_if.h"

#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "stream/am_record_stream.h"

#define INPUT_FILTER_NUMBER 4

extern IRecordEngine* create_record_engine();

AmRecordStream::AmRecordStream() :
  mRecordEngine(NULL),
  mRecordParams(NULL),
  mModuleInputInfo(NULL),
  mModuleCoreInfo(NULL),
  mModuleOutputInfo(NULL),
  mIsInitialized(false)
{
}

AmRecordStream::~AmRecordStream()
{
  release_resources();
  if (AM_LIKELY(mIsInitialized)) {
    AMF_Terminate();
  }
  DEBUG("~AmRecordStream");
}

bool AmRecordStream::init_record_stream(RecordParameters *config)
{
  bool ret = false;
  if (AM_UNLIKELY(!mIsInitialized)) {
    mIsInitialized = (ME_OK == AMF_Init());
  }
  if (AM_LIKELY(config && mIsInitialized)) {
    if (AM_LIKELY(create_resources())) {
      *mRecordParams = *config;
      mModuleInputInfo->audioChannels =
          config->audio_params->adevice_param->audio_channel_num;
      mModuleInputInfo->audioSampleRate =
          config->audio_params->adevice_param->audio_sample_freq;
      mModuleCoreInfo->eventStreamId =
          config->event_stream_id;
      mModuleCoreInfo->eventHistoryDuration =
          config->event_history_duration;
      mModuleCoreInfo->eventFutureDuration =
          config->event_future_duration;

      switch(config->audio_params->audio_format) {
        case AM_AUDIO_FORMAT_AAC: {
          mModuleInputInfo->audioCodecInfo.codecType = AM_AUDIO_CODEC_AAC;
          mModuleInputInfo->audioCodecInfo.codecInfo.aac.tns = 1;
          mModuleInputInfo->audioCodecInfo.codecInfo.aac.ff_type = 't';
          mModuleInputInfo->audioCodecInfo.codecInfo.aac.out_channel_num =
              config->audio_params->codec->aac.aac_output_channel;
          mModuleInputInfo->audioCodecInfo.codecInfo.aac.quantizer_quality =
              config->audio_params->codec->aac.aac_quantizer_quality;
          mModuleInputInfo->audioCodecInfo.codecInfo.aac.bitrate =
              config->audio_params->codec->aac.aac_bitrate;
          mModuleInputInfo->audioCodecInfo.codecInfo.aac.enc_mode =
              (AM_U16)config->audio_params->codec->aac.aac_format;
          ret = true;
        }break;
        case AM_AUDIO_FORMAT_OPUS: {
          AudioCodecOpusInfo &opusInfo =
              mModuleInputInfo->audioCodecInfo.codecInfo.opus;
          OpusEncoderParameters &opusParam =
              config->audio_params->codec->opus;
          mModuleInputInfo->audioCodecInfo.codecType = AM_AUDIO_CODEC_OPUS;
          opusInfo.opus_complexity  = opusParam.opus_complexity;
          opusInfo.opus_avg_bitrate = opusParam.opus_avg_bitrate;
          ret = true;
        }break;
        case AM_AUDIO_FORMAT_PCM: {
          mModuleInputInfo->audioCodecInfo.codecType = AM_AUDIO_CODEC_PCM;
          mModuleInputInfo->audioCodecInfo.codecInfo.pcm.out_channel_num =
              config->audio_params->codec->pcm.pcm_output_channel;
          ret = true;
        }break;
        case AM_AUDIO_FORMAT_BPCM: {
          mModuleInputInfo->audioCodecInfo.codecType = AM_AUDIO_CODEC_BPCM;
          mModuleInputInfo->audioCodecInfo.codecInfo.bpcm.out_channel_num =
              config->audio_params->codec->bpcm.bpcm_output_channel;
          ret = true;
        }break;
        case AM_AUDIO_FORMAT_G726: {
          mModuleInputInfo->audioCodecInfo.codecType = AM_AUDIO_CODEC_G726;
          mModuleInputInfo->audioCodecInfo.codecInfo.g726.g726_law =
              config->audio_params->codec->g726.g726_law;
          mModuleInputInfo->audioCodecInfo.codecInfo.g726.g726_rate =
              config->audio_params->codec->g726.g726_rate;
          ret = true;
        }break;
        case AM_AUDIO_FORMAT_NONE:
        default:{
          ERROR("Invalid audio format!");
          ret = false;
        }break;
      }

      if (AM_LIKELY(ret)) {
        AM_UINT streamCount = 0;
        AM_UINT vStreamMap  = 0;
        AM_UINT aStreamMap  = 0;
        AM_UINT videoCount  = 0;
        AM_UINT audioCount  = 0;

        for (AM_UINT i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
          if (AM_LIKELY(streamCount < config->stream_number)) {
            if (AM_LIKELY(config->stream_map & (1 << i))) {
              bool muxerAdded   = false;
              bool audioEnabled =
                  (config->audio_params->audio_stream_map & (1 << i));

              for (AM_UINT j = 0; j < MAX_ENCODE_STREAM_NUM; ++ j) {
                RecordFileType &muxerType = config->file_type[streamCount][j];
                if (AM_LIKELY(muxerType != AM_RECORD_FILE_TYPE_NULL)) {
                  OutputMuxerInfo *muxerInfo = mModuleOutputInfo->AddMuxerInfo(
                      (AmStreamNumber)i, (AmMuxerType)muxerType);
                  if (AM_UNLIKELY(AM_RECORD_FILE_TYPE_JPEG == muxerType)) {
                    /* If Muxer type is JPEG, need to disable audio */
                    NOTICE("FileType[%u][%u] is %u", streamCount, j, muxerType);
                    audioEnabled = false;
                  }
                  if (AM_LIKELY(muxerInfo)) {
                    if (AM_LIKELY(muxerType == AM_RECORD_FILE_TYPE_RTSP)) {
                      muxerInfo->rtspSendWait = config->rtsp_send_wait;
                      muxerInfo->rtspNeedAuth = config->rtsp_need_auth;
                    }
                    muxerInfo->SetDuration(config->file_duration);
                    muxerInfo->SetMaxFileAmount (config->max_file_amount);
                    muxerAdded = true;
                  } else {
                    ERROR("Failed to add muxer info!");
                    ret = false;
                    muxerAdded = false;
                    break;
                  }
                }
              }
              if (AM_LIKELY(muxerAdded && ret)) {
                vStreamMap |= (1 << i);
                if (AM_LIKELY(audioEnabled)) {
                  aStreamMap |= (1 << i);
                  ++ audioCount;
                }
                ++ streamCount;
                ++ videoCount;
              } else {
                break;
              }
            } else {
              INFO("Stream%u is not enabled!", i);
            }
          } else {
            break;
          }
        }
        if (AM_LIKELY(ret && ((vStreamMap > 0) || (aStreamMap > 0)))) {
          mModuleInputInfo->audioMap = aStreamMap;
          mModuleInputInfo->videoMap = vStreamMap;
          mModuleInputInfo->avSyncMap = (aStreamMap & vStreamMap);
          mModuleInputInfo->streamNumber = audioCount + videoCount;
          /* Todo: make this value configurable
           * Currently: There are VideoRecorder, AudioeEncoder, Sei, Event
           * 4 input modules connected to packetAggregator
           */
          mModuleInputInfo->inputNumber = INPUT_FILTER_NUMBER;

          if (mRecordEngine->SetModuleParameters(mModuleInputInfo) &&
              mRecordEngine->SetModuleParameters(mModuleOutputInfo) &&
              mRecordEngine->SetModuleParameters(mModuleCoreInfo)) {
            ret = mRecordEngine->CreateGraph();
          } else {
            ret = false;
            ERROR("Failed to set modules info!");
          }
        } else {
          ERROR("No stream is enabled!");
          ret = false;
        }
      }
    } else if (!mRecordEngine) {
      ERROR("Failed to create record engine!");
    }
  } else if (!config) {
    ERROR("Invalid record parameters!");
  } else {
    ERROR("AMF_Init failed!");
  }

  return ret;
}

void AmRecordStream::set_app_msg_callback(AmRecordStreamAppCb callback,
                                          void* data)
{
  mRecordEngine->SetAppMsgCallback(
      (IRecordEngine::AmRecordEngineAppCb)callback, data);
}

#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
void AmRecordStream::set_frame_statistics_callback(AmFrameStatisticsCb callback)
{
  mRecordEngine->SetFrameStatisticsCallback(callback);
}
#endif

bool AmRecordStream::record_start()
{
  char curTime[32] = {0};
  AM_UINT streamCount = 0;
  /* File name needs to be re-generated every time when record start */
  if (AM_UNLIKELY(false == get_current_time_string(curTime,
                                                   sizeof(curTime)))) {
    curTime[sprintf(curTime, "current")] = '\0';
  }
  for (OutputMuxerInfo *muxerInfo = mModuleOutputInfo->muxerInfo;
       muxerInfo; muxerInfo = muxerInfo->next) {
    set_output_location(muxerInfo, mRecordParams, curTime, streamCount ++);
  }
  return (mRecordEngine->SetOutputMuxerUri(mModuleOutputInfo) &&
          mRecordEngine->Start());
}

bool AmRecordStream::record_stop()
{
  return mRecordEngine->Stop();
}

bool AmRecordStream::send_usr_sei(void * data, uint32_t len)
{
  if (!data || len <= 0){
    ERROR("The error parameters");
    return false;
  }

  return mRecordEngine->SendUsrSEI(data, len);
}

bool AmRecordStream::send_usr_event(AmRecordStream::AmEventType eventType)
{
  CPacket::AmPayloadAttr payLoadType;

  switch(eventType){
    case AmRecordStream::AM_EVENT_TYPE_EMG :
      payLoadType = CPacket::AM_PAYLOAD_ATTR_EVENT_EMG;
      break;

    case AmRecordStream::AM_EVENT_TYPE_MD :
      payLoadType = CPacket::AM_PAYLOAD_ATTR_EVENT_MD;
      break;

    default:
      ERROR("can not parse the event type");
      return false;
  }

  return mRecordEngine->SendUsrEvent(payLoadType);
}

bool AmRecordStream::is_recording()
{
  return (mIsInitialized && mRecordEngine &&
          (mRecordEngine->GetEngineStatus() ==
           IRecordEngine::AM_RECORD_ENGINE_RECORDING));
}

inline bool AmRecordStream::create_resources()
{
  release_resources();
  if (AM_LIKELY(!mRecordEngine)) {
    mRecordEngine = create_record_engine();
  }
  if (AM_LIKELY(!mModuleInputInfo)) {
    mModuleInputInfo = new ModuleInputInfo();
  }
  if (AM_LIKELY(!mModuleOutputInfo)) {
    mModuleOutputInfo = new ModuleOutputInfo();
  }
  if (AM_LIKELY(!mModuleCoreInfo)) {
    mModuleCoreInfo = new ModuleCoreInfo();
  }
  if (AM_LIKELY(!mRecordParams)) {
    mRecordParams = new RecordParameters();
  }

  return (mRecordEngine && mModuleInputInfo &&
          mModuleOutputInfo && mModuleCoreInfo && mRecordParams);
}

inline void AmRecordStream::release_resources()
{
  delete mModuleOutputInfo;
  delete mModuleCoreInfo;
  delete mModuleInputInfo;
  delete mRecordParams;
  AM_DELETE(mRecordEngine);
  mRecordParams = NULL;
  mModuleInputInfo = NULL;
  mModuleOutputInfo = NULL;
  mModuleCoreInfo = NULL;
}

inline bool AmRecordStream::get_current_time_string(char *timeStr, uint32_t len)
{
  time_t current = time(NULL);
  if (AM_UNLIKELY(strftime(timeStr, len, "%Y%m%d%H%M%S",
                           localtime(&current)) == 0)) {
    ERROR("Date string format error!");
    timeStr[0] = '\0';
    return false;
  }

  return true;
}

inline void AmRecordStream::set_output_location(OutputMuxerInfo  *muxerInfo,
                                                RecordParameters *config,
                                                const char       *curTime,
                                                uint32_t          streamNum)
{
  if (muxerInfo->muxerType == AM_MUXER_TYPE_TS_HTTP) {
    muxerInfo->SetUri((const char*)config->ts_upload_url);
  } else {
    char fileUri[strlen(config->file_store_location) +
                 strlen(config->file_name_prefix) +
                 strlen(curTime) + 64];
    memset(fileUri, 0, sizeof(fileUri));
    if (AM_LIKELY(config->file_name_prefix &&
                  (strlen(config->file_name_prefix) > 0))) {
      if (AM_LIKELY(config->file_name_timestamp)) {
        sprintf(fileUri, "%s/%s_%s_stream%u",
                config->file_store_location,
                config->file_name_prefix,
                curTime, streamNum);
      } else {
        sprintf(fileUri, "%s/%s_stream%u",
                config->file_store_location,
                config->file_name_prefix, streamNum);
      }
    } else {
      WARN("File name prefix is not set, "
          "using time string instead");
      sprintf(fileUri, "%s/%s_stream%u",
              config->file_store_location,
              curTime, streamNum);
    }
    muxerInfo->SetUri((const char*)fileUri);
  }
}
