/*******************************************************************************
 * playback_engine_if.h
 *
 * History:
 *   2013-4-7 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef PLAYBACK_ENGINE_IF_H_
#define PLAYBACK_ENGINE_IF_H_

extern const AM_IID IID_IPlaybackEngine;

class IPlaybackEngine: public IInterface
{
  public:
    enum PlaybackEngineStatus {
      AM_PLAYBACK_ENGINE_STARTING,
      AM_PLAYBACK_ENGINE_PLAYING,
      AM_PLAYBACK_ENGINE_PAUSING,
      AM_PLAYBACK_ENGINE_PAUSED,
      AM_PLAYBACK_ENGINE_STOPPING,
      AM_PLAYBACK_ENGINE_STOPPED
    };
    enum AmPlaybackEngineMsg {
      AM_PLAYBACK_ENGINE_MSG_START_OK,
      AM_PLAYBACK_ENGINE_MSG_PAUSE_OK,
      AM_PLAYBACK_ENGINE_MSG_ERR,
      AM_PLAYBACK_ENGINE_MSG_ABORT,
      AM_PLAYBACK_ENGINE_MSG_EOS,
      AM_PLAYBACK_ENGINE_MSG_NULL
    };

    struct AmEngineMsg {
        AM_UINT msg;
        void *data;
        AmEngineMsg() :
          msg(AM_PLAYBACK_ENGINE_MSG_NULL),
          data(NULL){}
    };

    typedef void (*AmPlaybackEngineAppCb)(IPlaybackEngine::AmEngineMsg *msg);
  public:
    DECLARE_INTERFACE(IPlaybackEngine, IID_IPlaybackEngine);
    virtual PlaybackEngineStatus GetEngineStatus() = 0;
    virtual bool CreateGraph()                     = 0;
    virtual bool AddUri(const char* uri)           = 0;
    virtual bool Play(const char* uri = NULL)      = 0;
    virtual bool Stop()                            = 0;
    virtual bool Pause(bool enable)                = 0;
    virtual void SetAppMsgCallback(AmPlaybackEngineAppCb callback,
                                   void* data)     = 0;
};


#endif /* PLAYBACK_ENGINE_IF_H_ */
