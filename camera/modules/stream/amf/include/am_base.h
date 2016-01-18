/**
 * am_base.h
 *
 * History:
 *    2009/12/7 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_BASE_H__
#define __AM_BASE_H__

class CMutex;
class CQueue;
class CMsgSys;

class CObject;
class CWorkQueue;

class CEngineMsgProxy;
class CBaseEngine;

//-----------------------------------------------------------------------
//
//  CObject
//
//-----------------------------------------------------------------------
class CObject: public IInterface
{
  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete();
    virtual ~CObject()
    {
    }
};

//-----------------------------------------------------------------------
//
//  CWorkQueue
//
//-----------------------------------------------------------------------
class CWorkQueue
{
    typedef IActiveObject AO;

  public:
    static CWorkQueue* Create(AO *pAO,
                              bool RTPriority = false,
                              int priority = 99 /* Highest Priority */);
    void Delete();

  private:
    CWorkQueue(AO *pAO) :
      mpAO(pAO),
      mpMsgQ(NULL),
      mpThread(NULL)
    {
    }
    AM_ERR Construct(bool RTPriority, int priority);
    ~CWorkQueue();

  public:
    // return receive's reply
    AM_ERR SendCmd(AM_UINT cid, void *pExtra = NULL)
    {
      AO::CMD cmd(cid);
      cmd.pExtra = pExtra;
      return mpMsgQ->SendMsg(&cmd, sizeof(cmd));
    }

    // called by MainLoop when filter is not running
    void GetCmd(AO::CMD& cmd)
    {
      mpMsgQ->GetMsg(&cmd, sizeof(cmd));
    }

    bool PeekCmd(AO::CMD& cmd)
    {
      return mpMsgQ->PeekMsg(&cmd, sizeof(cmd));
    }

    bool PeekMsg(AM_MSG& msg)
    {
      return mpMsgQ->PeekMsg(&msg, sizeof(msg));
    }

    AM_ERR Run()
    {
      return SendCmd((AM_UINT)AO::CMD_RUN);
    }
    AM_ERR Stop()
    {
      return SendCmd((AM_UINT)AO::CMD_STOP);
    }

    void CmdAck(AM_ERR result)
    {
      mpMsgQ->Reply(result);
    }
    CQueue *MsgQ()
    {
      return mpMsgQ;
    }

    // called by inner loop (OnRun) when filter is running
    CQueue::QType WaitDataMsg(void *pMsg,
                              AM_UINT msgSize,
                              CQueue::WaitResult *pResult)
    {
      return mpMsgQ->WaitDataMsg(pMsg, msgSize, pResult);
    }

  private:
    void MainLoop();
    static AM_ERR ThreadEntry(void *p);
    void Terminate();

  private:
    AO *mpAO;
    CQueue *mpMsgQ;
    CThread *mpThread;
};

//-----------------------------------------------------------------------
//
//  CBaseEngine
//
//-----------------------------------------------------------------------
class CBaseEngine;

class CEngineMsgProxy: public CObject, public IMsgSink
{
    typedef CObject inherited;

  public:
    static CEngineMsgProxy* Create(CBaseEngine *pEngine);

  protected:
    CEngineMsgProxy(CBaseEngine *pEngine) :
            mpEngine(pEngine),
            mpMsgPort(NULL)
    {
    }
    AM_ERR Construct();
    virtual ~CEngineMsgProxy();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete()
    {
      inherited::Delete();
    }

    // IMsgSink
    virtual void MsgProc(AM_MSG& msg);

    IMsgPort *MsgPort()
    {
      return mpMsgPort;
    }

  private:
    CBaseEngine *mpEngine;
    IMsgPort *mpMsgPort;
};

class CBaseEngine: public CObject,
                   public IEngine,
                   public IMsgSink,
                   public IMediaControl
{
    typedef CObject inherited;
    friend class CEngineMsgProxy;

  protected:
    CBaseEngine() :
      mSessionID(0),
      mpMutex(NULL),
      mpPortFromFilters(NULL),
      mpMsgProxy(NULL),
      mpAppMsgSink(NULL),
      mAppMsgProc(NULL),
      mAppMsgContext(NULL),
      mpMsgSys(NULL)
    {}
    AM_ERR Construct();
    virtual ~CBaseEngine();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete()
    {
      inherited::Delete();
    }

    // IEngine
    virtual AM_ERR PostEngineMsg(AM_MSG& msg);

    // IMsgSink
    //virtual void MsgProc(AM_MSG& msg);

    // IMediaControl
    virtual AM_ERR SetAppMsgSink(IMsgSink *pAppMsgSink);
    virtual AM_ERR SetAppMsgCallback(void (*MsgProc)(void*, AM_MSG&),
                                     void *context);
  protected:
    AM_ERR PostAppMsg(AM_MSG& msg);
    AM_ERR PostAppMsg(AM_UINT code)
    {
      AM_MSG msg;
      msg.code = code;
      return PostAppMsg(msg);
    }

    void NewSession()
    {
      mSessionID ++;
    }
    bool IsSessionMsg(AM_MSG& msg)
    {
      return msg.sessionID == mSessionID;
    }

  private:
    void OnAppMsg(AM_MSG& msg);

  protected:
    AM_UINT mSessionID;
    CMutex *mpMutex;
    IMsgPort *mpPortFromFilters;
    CEngineMsgProxy *mpMsgProxy;
    IMsgSink *mpAppMsgSink;
    void (*mAppMsgProc)(void*, AM_MSG&);
    void *mAppMsgContext;
    CMsgSys* mpMsgSys;
};

//-----------------------------------------------------------------------
//
// CSystemClockSource
//
//-----------------------------------------------------------------------
class CSystemClockSource: public CObject, public IClockSource
{
    typedef CObject inherited;

  public:
    static CSystemClockSource* Create();

  public:
    CSystemClockSource()
    {}
    AM_ERR Construct()
    {
      return ME_OK;
    }
    virtual ~CSystemClockSource()
    {}

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete()
    {
      inherited::Delete();
    }

    // IClockSource
    AM_PTS GetClockPTS();
};

//-----------------------------------------------------------------------
//
// CClockManager
//
//-----------------------------------------------------------------------
class CClockManager: public CObject, public IClockManager, public IActiveObject
{
    typedef IActiveObject AO;
    typedef CObject inherited;

  public:
    static CClockManager* Create();

  protected:
    CClockManager() :
            mpMutex(NULL),
            mpTimerEvent(NULL),
            mpSource(NULL),
            mpSystemClockSource(NULL),
            mpWorkQ(NULL)
    {
      InitTimers();
    }
    AM_ERR Construct();
    virtual ~CClockManager();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete()
    {
      inherited::Delete();
    }

    // IClockManager
    virtual AM_ERR SetTimer(IClockObserver *pObserver, AM_PTS pts);
    virtual AM_PTS GetCurrentTime();
    virtual AM_ERR StartClock();
    virtual void StopClock();
    virtual void SetSource(IClockSource *pSource);

    // IActiveObject
    virtual const char *GetName()
    {
      return "ClockManager";
    }
    virtual void OnRun();
    virtual void OnCmd(CMD& cmd)
    {
    }

  private:
    CMutex *mpMutex;
    CEvent *mpTimerEvent;
    IClockSource *mpSource;
    IClockSource *mpSystemClockSource;
    AM_PTS mBasePts;
    CWorkQueue *mpWorkQ;

  private:
    struct TimerNode
    {
        IClockObserver *pObserver;
        AM_PTS pts;
    };

    enum
    {
      MAX_TIMERS = 8
    };
    TimerNode mTimerNodes[MAX_TIMERS];

    void InitTimers()
    {
      for (int i = 0; i < MAX_TIMERS; i ++)
        mTimerNodes[i].pObserver = NULL;
    }

    void CheckTimers();
};

#endif

