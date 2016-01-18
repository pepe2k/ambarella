/*******************************************************************************
 * am_record_stream.h
 *
 * Histroy:
 *  2012-3-26 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_DATA_STREAM_H_
#define AM_DATA_STREAM_H_

class IRecordEngine;
struct ModuleInputInfo;
struct ModuleCoreInfo;
struct ModuleOutputInfo;
struct OutputMuxerInfo;

class AmRecordStream
{
  public:
    enum AmEventType {
      AM_EVENT_TYPE_EMG = 0,
      AM_EVENT_TYPE_MD = 1,
    };

    enum AmRecordStreamMsg {
      AM_RECORD_STREAM_MSG_ERR,
      AM_RECORD_STREAM_MSG_ABORT,
      AM_RECORD_STREAM_MSG_EOS,
      AM_RECORD_STREAM_MSG_OVFL,
      AM_RECORD_STREAM_MSG_NULL
    };

    struct AmMsg {
        AmRecordStreamMsg msg;
        void             *data;
        AmMsg() :
          msg(AM_RECORD_STREAM_MSG_NULL),
          data(NULL){}
    };

    typedef void (*AmRecordStreamAppCb)(AmRecordStream::AmMsg *msg);
    typedef void (*AmAudioAlertCallback)();
    typedef void (*AmFetchRawDataCallback)(int streamId,
                                           unsigned char *data,
                                           int dataLen,
                                           int dataType);
    typedef void (*AmFrameStatisticsCb)(void *data);

  public:
    explicit AmRecordStream();
    virtual ~AmRecordStream();

  public:
    bool init_record_stream(RecordParameters *config);
    void set_app_msg_callback(AmRecordStreamAppCb callback, void* data);
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
    void set_frame_statistics_callback(AmFrameStatisticsCb callback);
#endif

  public:
    bool record_start();
    bool record_stop();
    bool send_usr_sei(void * data, uint32_t len);
    bool send_usr_event(AmRecordStream::AmEventType eventType);

  public:
    bool is_recording();

  private:
    inline bool create_resources();
    inline void release_resources();
    inline bool get_current_time_string(char *timeStr, uint32_t len);
    inline void set_output_location(OutputMuxerInfo  *muxerInfo,
                                    RecordParameters *config,
                                    const char       *curTime,
                                    uint32_t          streamNum);

  private:
    IRecordEngine    *mRecordEngine;
    RecordParameters *mRecordParams;
    ModuleInputInfo  *mModuleInputInfo;
    ModuleCoreInfo   *mModuleCoreInfo;
    ModuleOutputInfo *mModuleOutputInfo;
    bool              mIsInitialized;
};

#endif /* AM_DATA_STREAM_H_ */
