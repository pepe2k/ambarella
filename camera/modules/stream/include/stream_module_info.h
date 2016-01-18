/*******************************************************************************
 * stream_module_info.h
 *
 * Histroy:
 *   2012-10-10 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef STREAM_MODULE_INFO_H_
#define STREAM_MODULE_INFO_H_

/*
 * Parameters Structure for Input Module
 */
struct CoreAudioCodecInfo {
    AmAudioCodecType codecType;
    union {
        AudioCodecAacInfo  aac;
        AudioCodecOpusInfo opus;
        AudioCodecPcmInfo  pcm;
        AudioCodecBpcmInfo bpcm;
        AudioCodecG726Info g726;
    }codecInfo;
#define codecInfoAac   codecInfo.aac
#define codecInfoOpus  codecInfo.opus
#define codecInfoPcm   codecInfo.pcm
#define codecInfoBpcm  codecInfo.bpcm
#define codecInfoG726  codecInfo.g726
    CoreAudioCodecInfo() :
      codecType(AM_AUDIO_CODEC_NONE)
    {
      memset(&codecInfo, 0, sizeof(codecInfo));
    }
};

struct ModuleInputInfo {
    AM_UINT audioMap;
    AM_UINT videoMap;
    AM_UINT avSyncMap;
    AM_UINT inputNumber;
    AM_UINT streamNumber;
    AM_UINT audioChannels;
    AM_UINT audioSampleRate;
    CoreAudioCodecInfo audioCodecInfo;
    ModuleInputInfo() :
      audioMap(0),
      videoMap(0),
      avSyncMap(0),
      inputNumber(0),
      streamNumber(0),
      audioChannels(2),
      audioSampleRate(48000){}
};

struct ModuleCoreInfo {
    AM_UINT eventStreamId;
    AM_UINT eventHistoryDuration;
    AM_UINT eventFutureDuration;
    ModuleCoreInfo():
      eventStreamId(0),
      eventHistoryDuration(0),
      eventFutureDuration(0)
    {}
};

/*
 * Parameters Structure for Output Module
 */
struct OutputMuxerInfo {
    AmMuxerType       muxerType;
    AmStreamNumber    streamId;
    AM_UINT           duration;
    AM_UINT           maxFileAmount;
    bool              alignGop;
    bool              rtspSendWait;
    bool              rtspNeedAuth;
    char             *uri;
    OutputMuxerInfo  *next;
    OutputMuxerInfo() :
      muxerType(AM_MUXER_TYPE_NONE),
      streamId(AM_STREAM_NUMBER_1),
      duration(0),
      maxFileAmount(0),
      alignGop(false),
      rtspSendWait(false),
      rtspNeedAuth(false),
      uri(NULL),
      next(NULL){}
    ~OutputMuxerInfo() {
      delete[] uri;
      delete next;
    }
    void SetDuration(AM_UINT second)
    {
      duration = second;
    }
    void SetMaxFileAmount (AM_UINT fileAmount)
    {
      maxFileAmount = fileAmount;
    }
    void SetUri(const char *path)
    {
      delete[] uri;
      uri = NULL;
      if (AM_LIKELY(path)) {
        uri = amstrdup(path);
      }
    }
};

struct ModuleOutputInfo {
    AM_UINT          muxerMask[AM_STREAM_NUMBER_MAX];
    OutputMuxerInfo *muxerInfo;
    ModuleOutputInfo() :
      muxerInfo(NULL) {
      memset (muxerMask, 0, AM_STREAM_NUMBER_MAX * sizeof (AM_UINT));
    }
    ~ModuleOutputInfo() {
      delete muxerInfo;
    }
    OutputMuxerInfo *AddMuxerInfo(AmStreamNumber streamid,
                                  AmMuxerType muxerType)
    {
      OutputMuxerInfo **muxerInfo = GetMuxerInfo(streamid, muxerType);
      if (AM_LIKELY(NULL == *muxerInfo)) {
        *muxerInfo = new OutputMuxerInfo();
        if (AM_LIKELY(*muxerInfo)) {
          (*muxerInfo)->streamId = streamid;
          (*muxerInfo)->muxerType = muxerType;
          muxerMask[streamid] |= (*muxerInfo)->muxerType;
          DEBUG("Add stream%u, type %u",
                (*muxerInfo)->streamId, (*muxerInfo)->muxerType);
        } else {
          ERROR("Failed to new OutputMuxerInfo!");
        }
      } else {
        NOTICE("Stream%u, muxer type %u, is already added!",
               streamid, muxerType);
      }

      return *muxerInfo;
    }

    OutputMuxerInfo** GetMuxerInfo(AmStreamNumber streamid,
                                  AmMuxerType muxerType)
    {
      OutputMuxerInfo **info = &muxerInfo;
      for (; *info != NULL; info = &((*info)->next)) {
        if (AM_UNLIKELY(((*info)->streamId == streamid) &&
                        ((*info)->muxerType == muxerType))) {
          DEBUG("Find MuxerInfo: %p, for stream %u",
                *info, (AM_UINT)streamid);
          break;
        }
      }

      return info;
    }
};

#endif /* STREAM_MODULE_INFO_H_ */
