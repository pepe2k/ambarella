/*******************************************************************************
 * audio_player.h
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

#ifndef AUDIO_PLAYER_H_
#define AUDIO_PLAYER_H_

#include <queue>
#include <pulse/pulseaudio.h>

struct PaData;
struct AM_AUDIO_INFO;
class CPlayerAudio
{
  public:
    CPlayerAudio(AM_UINT chunkBytes) :
      mChunkBytes(chunkBytes){}
    virtual ~CPlayerAudio(){}

  public:
    virtual void AddPacket(CPacket *packet) = 0;
    virtual bool FeedData() = 0;
    virtual AM_ERR Start(AM_AUDIO_INFO& audioinfo) = 0;
    virtual AM_ERR Stop(bool wait = true)  = 0;
    virtual AM_ERR Pause(bool enable) = 0;
    virtual bool IsPlayerRunning() = 0;

  protected:
    typedef std::queue<CPacket*> PacketQueue;
    inline AM_UINT GetLcm(AM_UINT a, AM_UINT b)
    {
      AM_UINT c = a;
      AM_UINT d = b;
      while(((c > d) ? (c %= d) : (d %= c)));
      return (a * b) / (c + d);
    }

  protected:
    AM_UINT mChunkBytes;
};

class CPlayerPulse: public CPlayerAudio
{
  public:
    static CPlayerPulse* Create();

  public:
    virtual void AddPacket(CPacket *packet);
    virtual bool FeedData();
    virtual AM_ERR Start(AM_AUDIO_INFO& audioinfo);
    virtual AM_ERR Stop(bool wait = true);
    virtual AM_ERR Pause(bool enable);
    virtual bool IsPlayerRunning();

  private:
    CPlayerPulse();
    virtual ~CPlayerPulse();
    AM_ERR Construct();

  private:
    AM_ERR StartPlayer();
    AM_ERR Initialize();
    void   Finalize();

  private:
    inline void PaState(pa_context *context, void *data);
    inline void PaServerInfo(pa_context *context,
                             const pa_server_info *info,
                             void *data);
    inline void PaWrite(pa_stream *stream, size_t bytes, void *data);
    inline void PaUnderflow(pa_stream *stream, void *data);
    inline void PaDrain(pa_stream *stream, int success, void *data);

  private:
    static void StaticPaState(pa_context *context, void *data);
    static void StaticPaServerInfo(pa_context *context,
                                   const pa_server_info *info,
                                   void *data);
    static void StaticPaWrite(pa_stream *stream, size_t bytes, void *data);
    static void StaticPaUnderflow(pa_stream *stream, void *data);
    static void StaticPaDrain(pa_stream *stream, int success, void *data);

  private:
    pa_threaded_mainloop *mPaThreadMainLoop;
    pa_context           *mPaContext;
    pa_stream            *mPaStreamPlayback;
    CMutex               *mMutex;
    CEvent               *mEvent;
    PaData               *mWriteData;
    PaData               *mUnderFlow;
    PaData               *mDrainData;
    char                 *mDefSinkName;
    PacketQueue          *mAudioQueue;
    AM_UINT               mPlayLatency;
    AM_UINT               mUnderFlowCount;
    pa_sample_spec        mPaSampleSpec;
    pa_channel_map        mPaChannelMap;
    pa_buffer_attr        mPaBufferAttr;
    pa_context_state      mPaContextState;
    bool                  mCtxConnected;
    bool                  mLoopRun;
    bool                  mIsPlayerStarted;
    bool                  mIsDraining;
};

#endif /* AUDIO_PLAYER_H_ */
