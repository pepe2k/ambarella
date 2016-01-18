/*******************************************************************************
 * record_engine.h
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

#ifndef STREAM_ENGINE_H_
#define STREAM_ENGINE_H_

class IInputRecord;
class ICore;
class IOutputRecord;

class CRecordEngine: public CPacketFilterGraph, public IRecordEngine
{
    typedef CPacketFilterGraph inherited;

  public:
    static CRecordEngine* Create();

  public:
    virtual void *GetInterface(AM_REFIID ref_iid)
    {
      if (ref_iid == IID_IRecordEngine) {
        return (IRecordEngine*)this;
      }
      return inherited::GetInterface(ref_iid);
    }
    virtual void Delete() { inherited::Delete(); }
    virtual AM_ERR PostEngineMsg(AM_MSG& msg)
    {
      return inherited::PostEngineMsg(msg);
    }

    virtual AM_ERR SetAppMsgSink (IMsgSink *pAppMsgSink)
    {
      return inherited::SetAppMsgSink (pAppMsgSink);
    }

  public:
    virtual RecordEngineStatus GetEngineStatus() {return mState;}
    virtual bool CreateGraph();
    virtual bool SetModuleParameters(ModuleInputInfo *info);
    virtual bool SetModuleParameters(ModuleCoreInfo *info);
    virtual bool SetModuleParameters(ModuleOutputInfo *info);
    virtual bool SetOutputMuxerUri(ModuleOutputInfo *info);
    virtual bool Start();
    virtual bool Stop();
    virtual bool SendUsrSEI(void *pUserData, AM_U32 len);
    virtual bool SendUsrEvent(CPacket::AmPayloadAttr eventType);
    virtual void SetAppMsgCallback(AmRecordEngineAppCb callback, void* data);
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
    virtual void SetFrameStatisticsCallback(void (*callback)(void *data));
#endif

  private:
    virtual AM_ERR SetAppMsgCallback (void (*MsgProc)(void *, AM_MSG &),
                                      void *context)
    {
      return inherited::SetAppMsgCallback (MsgProc, context);
    }
    virtual void MsgProc(AM_MSG& msg);
    virtual void AppProc(void* context, AM_MSG& msg);
    static void StaticAppProc(void* context, AM_MSG& msg);
    bool ResetHwTimer();

  private:
    CRecordEngine() :
      mEvent(NULL),
      mMutex(NULL),
      mModuleInput(NULL),
      mModuleCore(NULL),
      mModuleOutput(NULL),
      mAppData(NULL),
      mAppCallback(NULL),
      mState(AM_RECORD_ENGINE_STOPPED),
      mIsGraphCreated(false),
      mNeedBlock(true),
      mStoppedMuxerNum(0) {}
    virtual ~CRecordEngine();
    AM_ERR Construct();

  private:
    CEvent             *mEvent;
    CMutex             *mMutex;
    IInputRecord       *mModuleInput;
    ICore              *mModuleCore;
    IOutputRecord      *mModuleOutput;
    void               *mAppData;
    AmRecordEngineAppCb mAppCallback;
    RecordEngineStatus  mState;
    bool                mIsGraphCreated;
    bool                mNeedBlock;
    AM_UINT             mStoppedMuxerNum;
    AmEngineMsg         mAppMsg;
};


#endif /* STREAM_ENGINE_H_ */
