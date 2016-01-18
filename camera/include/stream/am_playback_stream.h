/*******************************************************************************
 * am_playback_stream.h
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

#ifndef AM_PLAYBACK_STREAM_H_
#define AM_PLAYBACK_STREAM_H_

class IPlaybackEngine;
class AmPlaybackStream
{
  public:
    enum AmPlaybackStreamMsg {
      AM_PLAYBACK_STREAM_MSG_START_OK,
      AM_PLAYBACK_STREAM_MSG_PAUSE_OK,
      AM_PLAYBACK_STREAM_MSG_ERR,
      AM_PLAYBACK_STREAM_MSG_ABORT,
      AM_PLAYBACK_STREAM_MSG_EOS,
      AM_PLAYBACK_STREAM_MSG_NULL
    };

    struct AmMsg {
        AmPlaybackStreamMsg msg;
        void               *data;
        AmMsg() :
          msg(AM_PLAYBACK_STREAM_MSG_NULL),
          data(NULL){}
    };

    typedef void (*AmPlaybackStreamAppCb)(AmPlaybackStream::AmMsg *msg);

  public:
    explicit AmPlaybackStream();
    virtual ~AmPlaybackStream();

  public:
    bool init_playback_stream();
    void set_app_msg_callback(AmPlaybackStreamAppCb callback, void *data);
    bool is_playing();
    bool is_paused();

  public:
    bool add_uri(const char *uri);
    bool play(const char *uri = NULL);
    bool pause(bool enable);
    bool stop();

  private:
    IPlaybackEngine  *mPlaybackEngine;
    bool              mIsInitialized;
};


#endif /* AM_PLAYBACK_STREAM_H_ */
