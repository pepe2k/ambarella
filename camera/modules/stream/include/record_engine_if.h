/*******************************************************************************
 * record_engine_if.h
 *
 * Histroy:
 *   2012-10-9 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef STREAM_ENGINE_IF_H_
#define STREAM_ENGINE_IF_H_

extern const AM_IID IID_IRecordEngine;

class IRecordEngine: public IInterface
{
  public:
    enum RecordEngineStatus {
      AM_RECORD_ENGINE_STOPPED,
      AM_RECORD_ENGINE_STOPPING,
      AM_RECORD_ENGINE_RECORDING
    };

    enum RecordEngineMsg {
      AM_RECORD_ENGINE_MSG_ERR,
      AM_RECORD_ENGINE_MSG_ABORT,
      AM_RECORD_ENGINE_MSG_EOS,
      AM_RECORD_ENGINE_MSG_OVFL,
      AM_RECORD_ENGINE_MSG_NULL
    };

    struct AmEngineMsg {
        AM_UINT msg;
        void  *data;
        AmEngineMsg() :
          msg(AM_RECORD_ENGINE_MSG_NULL),
          data(NULL){}
    };

    typedef void (*AmRecordEngineAppCb)(IRecordEngine::AmEngineMsg *msg);

  public:
    DECLARE_INTERFACE(IRecordEngine, IID_IRecordEngine);
    virtual RecordEngineStatus GetEngineStatus() = 0;
    virtual bool CreateGraph() = 0;
    virtual bool SetModuleParameters(ModuleInputInfo *info) = 0;
    virtual bool SetModuleParameters(ModuleCoreInfo *info) = 0;
    virtual bool SetModuleParameters(ModuleOutputInfo *info) = 0;
    virtual bool SetOutputMuxerUri(ModuleOutputInfo *info) = 0;
    virtual bool Start() = 0;
    virtual bool Stop()  = 0;
    virtual bool SendUsrSEI(void *pUserData, AM_U32 len) = 0;
    virtual bool SendUsrEvent(CPacket::AmPayloadAttr eventType) = 0;
    virtual void SetAppMsgCallback(AmRecordEngineAppCb callback, void* data)= 0;
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
    virtual void SetFrameStatisticsCallback(void (*callback)(void *data)) = 0;
#endif
};

#endif /* STREAM_ENGINE_IF_H_ */
