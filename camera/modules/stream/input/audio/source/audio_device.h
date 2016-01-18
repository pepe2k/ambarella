/*******************************************************************************
 * am_audiodevice.h
 *
 * Histroy:
 *   2012-9-18 - [ypchang] created file
 *   2013-1-24 - [Hanbo Xiao] Modified file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AUDIO_DEVICE_H_
#define AUDIO_DEVICE_H_
/*
 * This class is used to interact with Audio device
 */

#include <queue>
#include <pulse/pulseaudio.h>

struct PaData;
class CAudioCapture;
class CAudioAlert;

class CAudioDevice
{
  public:
    CAudioDevice() :
      mAudioStreamMap(0),
      mAvSyncMap(0),
      mAudioStreamCnt(0),
      mAudioChannel(2),
      mAudioSampleRate(48000),
      mAudioCodecType(0),
      mAudioChunkBytes(0){}
    virtual ~CAudioDevice(){}

  public:
    virtual AM_ERR Start() = 0;
    virtual AM_ERR Stop()  = 0;
    virtual AM_ERR GetAudioParameter(AM_AUDIO_INFO *info, AM_UINT streamid) = 0;
    virtual void SetAudioStreamMap(AM_UINT map) = 0;
    virtual void SetAvSyncMap(AM_UINT avSyncMap) = 0;
    virtual void SetAudioChannelNumber(AM_UINT channel)
    {
      NOTICE("Set audio channel to %u", channel);
      mAudioChannel = channel;
    }
    virtual void SetAudioSampleRate(AM_UINT samplerate)
    {
      NOTICE("Set audio sample rate to %u", samplerate);
      mAudioSampleRate = samplerate;
    }
    virtual void SetAudioCodecType(AM_UINT codecType)
    {
      NOTICE("Set audio codec type to %u", codecType);
      mAudioCodecType = codecType;
    }

  protected:
    AM_UINT mAudioStreamMap;
    AM_UINT mAvSyncMap;
    AM_UINT mAudioStreamCnt;
    AM_UINT mAudioChannel;
    AM_UINT mAudioSampleRate;
    AM_UINT mAudioCodecType;
    AM_UINT mAudioChunkBytes;
};

class CAlsaAudio: public CAudioDevice
{
  public:
    CAlsaAudio();
    virtual ~CAlsaAudio();

  public:
    virtual AM_ERR Start();
    virtual AM_ERR Stop();
    virtual AM_ERR GetAudioParameter(AM_AUDIO_INFO *info, AM_UINT streamid);
    virtual void SetAudioStreamMap(AM_UINT map);
    virtual void SetAvSyncMap(AM_UINT avSyncMap);
};

class CPulseAudio: public CAudioDevice
{
    typedef std::queue<CPacket*> PacketQueue;
  public:
    static CPulseAudio* Create(CAudioCapture *aCapture,
                               PacketQueue   *packetQueue)
    {
      CPulseAudio *result = new CPulseAudio(aCapture, packetQueue);
      if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
        delete result;
        result = NULL;
      }

      return result;
    }

  public:
    virtual AM_ERR Start();
    virtual AM_ERR Stop();
    virtual AM_ERR GetAudioParameter(AM_AUDIO_INFO *info, AM_UINT streamid);
    virtual void SetAudioStreamMap(AM_UINT map);
    virtual void SetAvSyncMap(AM_UINT avSyncMap);

  public:
    void PaState(pa_context *context, void *data);
    void PaServerInfo(pa_context *context,
                      const pa_server_info *info,
                      void *data);
    void PaRead(pa_stream *stream, size_t bytes, void *data);
    void PaOverflow(pa_stream *stream, void *data);

  private:
    CPulseAudio(CAudioCapture *aCapture,
                PacketQueue *packetQueue);
    virtual ~CPulseAudio();
    AM_ERR Construct();

  private:
    AM_ERR Init();
    void   Fini();
    AM_PTS GetCurrentPts();
    AM_UINT GetLcm(AM_UINT a, AM_UINT b);
    AM_UINT GetAvailDataSize();

  private:
    static void StaticPaState(pa_context *context, void *data);
    static void StaticPaServerInfo(pa_context *context,
                                   const pa_server_info *info,
                                   void *data);
    static void StaticPaRead(pa_stream *stream, size_t bytes, void *data);
    static void StaticPaOverflow(pa_stream *stream, void *data);

  private:
    pa_threaded_mainloop *mPaThreadMainLoop;
    pa_mainloop_api      *mPaMainLoopApi;
    pa_context           *mPaContext;
    pa_stream            *mPaStreamRecord;
    PacketQueue          *mPacketQ;
    char                 *mDefSrcName;
    CAudioCapture        *mAudioCapture;
    CMutex               *mMutex;
    AM_U8                *mAudioBuffer;
    AM_U8                *mAudioRdPtr;
    AM_U8                *mAudioWtPtr;
    PaData               *mReadData;
    PaData               *mOverFlow;
    pa_sample_spec        mPaSampleSpec;
    pa_channel_map        mPaChannelMap;
    pa_context_state      mPaContextState;
    bool                  mCtxConnected;
    bool                  mLoopRun;
    AM_UINT               mFrameBytes;
    AM_UINT               mAudioBufSize;
    int                   mHwTimerFd;
    AM_PTS                mLastPts;
    AM_PTS                mFragmentPts;
    AM_ERR                mMainLoopRet;
};

#endif /* AUDIO_DEVICE_H_ */
