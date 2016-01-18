/*******************************************************************************
 * am_base_packet.h
 *
 * History:
 *   2012-9-11 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_BASE_PACKET_H_
#define AM_BASE_PACKET_H_

class CPacketPool;
class CSimplePacketPool;
class CFixedPacketPool;

/*
 * CPacketPool
 */
class CPacketPool: public CObject, public IPacketPool
{
    typedef CObject inherited;

  public:
    static CPacketPool *Create (const char *, AM_UINT);

  protected:
    CPacketPool(const char *pName) :
      mRefCount(1),
      mpBufferQ(NULL),
      mpName(pName) {}
    AM_ERR Construct(AM_UINT nMaxBuffers);
    virtual ~CPacketPool();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete()
    {
      inherited::Delete();
    }

    // IPacketPool
    virtual const char *GetName()
    {
      return mpName;
    }

    virtual void Enable(bool bEnabled = true);
    virtual bool AllocBuffer(CPacket*& pBuffer, AM_UINT size);
    virtual AM_UINT GetAvailBufNum()
    {
      return mpBufferQ->GetAvailDataNum();
    }

    virtual void AddRef(CPacket *pBuffer);
    virtual void Release(CPacket *pBuffer);

    virtual void AddRef();
    virtual void Release();

  protected:
    virtual void OnReleaseBuffer(CPacket *pBuffer)
    {
    }

  protected:
    am_atomic_t mRefCount;
    CQueue     *mpBufferQ;
    const char *mpName;
};

/*
 * CSimplePacketPool
 */
class CSimplePacketPool: public CPacketPool
{
    typedef CPacketPool inherited;

  public:
    static CSimplePacketPool* Create(const char *pName,
                                     AM_UINT     count,
                                     AM_UINT     objectSize = sizeof(CPacket));

  protected:
    CSimplePacketPool(const char *pName) :
      inherited(pName),
      mpPacketMemory(NULL)
    {
    }
    AM_ERR Construct(AM_UINT count, AM_UINT objectSize = sizeof(CPacket));
    virtual ~CSimplePacketPool();

  protected:
    CPacket *mpPacketMemory;
};

/*
 * CFixedPacketPool
 */
class CFixedPacketPool: public CSimplePacketPool
{
    typedef CSimplePacketPool inherited;

  public:
    static CFixedPacketPool* Create(const char *pName,
                                    AM_UINT count,
                                    AM_UINT dataSize);

  protected:
    CFixedPacketPool(const char *pName) :
      inherited(pName),
      mpPayloadMemory(NULL),
      mMaxDataSize(0)
    {
    }
    AM_ERR Construct(AM_UINT count, AM_UINT dataSize);
    virtual ~CFixedPacketPool();

  private:
    AM_U8 *mpPayloadMemory;
    AM_UINT mMaxDataSize;
};

/*
 * CPacketFilter
 */
class CPacketFilter: public CObject, public IPacketFilter
{
    typedef CObject inherited;

  protected:
    CPacketFilter(IEngine *pEngine) :
            mpEngine(pEngine),
            mpName(NULL)
    {
    }
    AM_ERR Construct();
    virtual ~CPacketFilter()
    {
    }

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete()
    {
      inherited::Delete();
    }

    // IPacketFilter
    //virtual AM_ERR Run();
    //virtual AM_ERR Stop();

    //virtual void GetInfo(INFO& info);

    virtual IPacketPin* GetInputPin(AM_UINT index)
    {
      return NULL;
    }

    virtual IPacketPin* GetOutputPin(AM_UINT index)
    {
      return NULL;
    }

    virtual AM_ERR AddOutputPin(AM_INT& index)
    {
      ERROR("!!!must not comes here, not implement.\n");
      return ME_NOT_SUPPORTED;
    }

    virtual AM_ERR AddInputPin(AM_INT& index, AM_UINT type)
    {
      AM_ERROR("!!!must not comes here, not implement.\n");
      return ME_NOT_SUPPORTED;
    }

  public:
    AM_ERR PostEngineMsg(AM_MSG& msg)
    {
      return mpEngine->PostEngineMsg(msg);
    }

    AM_ERR PostEngineMsg(AM_UINT code)
    {
      AM_MSG msg;
      msg.code = code;
      return PostEngineMsg(msg);
    }

  protected:
    IEngine *mpEngine;
    const char *mpName;
};

/*
 * CPacketPin
 */
class CPacketPin: public CObject, public IPacketPin
{
    typedef CObject inherited;

  protected:
    CPacketPin(CPacketFilter *pFilter) :
      mpFilter(pFilter),
      mpPeer(NULL),
      mpPacketPool(NULL),
      mbSetBP(false) {}
    virtual ~CPacketPin();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete()
    {
      inherited::Delete();
    }

    // IPin
    virtual AM_ERR Connect(IPacketPin *pPeer)
    {
      ERROR("Not implemented, should be implemented in child class!");
      return ME_ERROR;
    }
    virtual void Disconnect()
    {
      ERROR("Not implemented, should be implemented in child class!");
    }

    virtual void Receive(CPacket *pBuffer)
    {
      ERROR("Not implemented, should be implemented in child class!");
    }

    virtual void Purge()
    {
      ERROR("Not implemented, should be implemented in child class!");
    }

    virtual void Enable(bool bEnable)
    {
      if (mpPacketPool) {
        mpPacketPool->Enable(bEnable);
      }
    }

    virtual IPacketPin* GetPeer()
    {
      return mpPeer;
    }
    virtual IPacketFilter* GetFilter()
    {
      return mpFilter;
    }
    virtual AM_BOOL IsConnected()
    {
      return mpPeer ? AM_TRUE : AM_FALSE;
    }

  protected:
    virtual AM_ERR OnConnect(IPacketPin *pPeer) = 0;
    virtual void OnDisconnect();

    // When connecting, if there's no buffer pool,
    // this method is called.
    // If it returns an error (default), connecting fails.
    virtual AM_ERR NoBufferPoolHandler();

    // called by derived class to set a buffer pool for this pin
    // The buffer pool will be addref-ed.
    // If pBufferPool is NULL, previous buffer pool is released.
    void SetBufferPool(IPacketPool *pPacketPool);
    void ReleaseBufferPool();

  protected:
    // Helpers
    AM_ERR PostEngineMsg(AM_MSG& msg)
    {
      return mpFilter->PostEngineMsg(msg);
    }

    AM_ERR PostEngineMsg(AM_UINT code)
    {
      AM_MSG msg;
      msg.code = code;
      msg.p1 = (AM_INTPTR) mpFilter;
      return PostEngineMsg(msg);
    }

    AM_ERR PostEngineErrorMsg(AM_ERR err)
    {
      AM_MSG msg;
      msg.code = IEngine::MSG_ERROR;
      msg.p0 = err;
      msg.p1 = (AM_INTPTR) mpFilter;
      return PostEngineMsg(msg);
    }

    const char *FilterName()
    {
      IPacketFilter::INFO info;
      mpFilter->GetInfo(info);
      return info.pName;
    }

  protected:
    CPacketFilter *mpFilter;
    IPacketPin    *mpPeer;
    IPacketPool   *mpPacketPool;
    bool           mbSetBP; // the buffer pool is specified by SetBufferPool()
};

/*
 * CPacketInputPin
 */
class CPacketInputPin: public CPacketPin
{
    typedef CPacketPin inherited;

  protected:
    CPacketInputPin(CPacketFilter *pFilter) :
      inherited(pFilter)
    {
    }
    AM_ERR Construct()
    {
      return ME_OK;
    }
    virtual ~CPacketInputPin()
    {
    }

  public:
    // IPin
    virtual AM_ERR Connect(IPacketPin *pPeer);
    virtual void Disconnect();
    virtual void Receive(CPacket *pBuffer)
    {
      ERROR("Not implemented!");
    }
    virtual void Purge()
    {
      ERROR("Not implemented!");
    }

  protected:
    virtual AM_ERR OnConnect(IPacketPin *pPeer);
};

/*
 * CPacketOutputPin
 */
class CPacketOutputPin: public CPacketPin
{
    typedef CPacketPin inherited;

  protected:
    CPacketOutputPin(CPacketFilter *pFilter) :
      inherited(pFilter)
    {
    }
    virtual ~CPacketOutputPin()
    {
    }

  public:
    // IPin
    virtual AM_ERR Connect(IPacketPin *pPeer);
    virtual void Disconnect();
    virtual void Receive(CPacket *pBuffer)
    {
      AM_ASSERT(0);
    }
    virtual void Purge()
    {
      AM_ASSERT(0);
    }

  public:
    bool AllocBuffer(CPacket*& pBuffer, AM_UINT size = 0)
    {
      AM_ASSERT(mpPacketPool);
      return mpPacketPool->AllocBuffer(pBuffer, size);
    }

    void SendBuffer(CPacket *pBuffer)
    {
      if (mpPeer)
        mpPeer->Receive(pBuffer);
      else {
        AM_INFO("buffer dropped\n");
        pBuffer->Release();
      }
    }

    AM_UINT GetAvailBufNum()
    {
      return mpPacketPool->GetAvailBufNum();
    }

  protected:
    virtual AM_ERR OnConnect(IPacketPin *pPeer);
    //virtual void OnDisconnect();
};

/*
 * CPacketActiveOutputPin
 */
class CPacketActiveOutputPin: public CPacketOutputPin, public IActiveObject
{
    typedef CPacketOutputPin inherited;

  protected:
    typedef IActiveObject AO;

  protected:
    CPacketActiveOutputPin(CPacketFilter *pFilter, const char *pName) :
      inherited(pFilter),
      mpWorkQ(NULL),
      mpName(pName)
    {
    }
    AM_ERR Construct();
    virtual ~CPacketActiveOutputPin();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete()
    {
      inherited::Delete();
    }

    // IPin
    //virtual void GetInfo(INFO& info) = 0;

    // IActiveObject
    virtual const char *GetName()
    {
      return mpName;
    }

  public:
    virtual AM_ERR Run()
    {
      return mpWorkQ->Run();
    }
    virtual AM_ERR Stop()
    {
      return mpWorkQ->Stop();
    }

  protected:
    // called when stopped, and received CMD_RUN
    //virtual void OnRun() = 0;

    // called by the thread when a cmd is received
    // should return false for CMD_TERMINATE
    virtual void OnCmd(CMD& cmd)
    {
    }

  protected:
    CQueue *MsgQ()
    {
      return mpWorkQ->MsgQ();
    }
    void GetCmd(AO::CMD& cmd)
    {
      mpWorkQ->GetCmd(cmd);
    }
    bool PeekCmd(AO::CMD& cmd)
    {
      return mpWorkQ->PeekCmd(cmd);
    }
    void CmdAck(AM_ERR err)
    {
      mpWorkQ->CmdAck(err);
    }

  protected:
    CWorkQueue *mpWorkQ;
    const char *mpName;
};

/*
 * CPacketQueueInputPin
 */
class CPacketQueueInputPin: public CPacketInputPin
{
    typedef CPacketInputPin inherited;

  protected:
    CPacketQueueInputPin(CPacketFilter *pFilter) :
      inherited(pFilter),
      mpBufferQ(NULL)
    {
    }
    AM_ERR Construct(CQueue *pMsgQ);
    virtual ~CPacketQueueInputPin();

  public:
    // IPin
    virtual void Receive(CPacket *pBuffer)
    {
      AM_ASSERT(mpBufferQ);
      AM_ENSURE_OK_(mpBufferQ->PutData(&pBuffer, sizeof(pBuffer)));
    }
    virtual void Purge();

  public:
    /*
     bool ReceiveBuffer(CBuffer*& pBuffer, CQueue *pMsgQ, void *pCmd, AM_UINT cmdSize)
     {
     return mpBufferQ->GetMsg2((void*)&pBuffer, sizeof(pBuffer), pMsgQ, pCmd, cmdSize);
     }
     */
    //  bool ReceiveBuffer(CBuffer*& pBuffer, void *pCmd, AM_UINT cmdSize);
    bool PeekBuffer(CPacket*& pBuffer)
    {
      return mpBufferQ->PeekData((void*) &pBuffer, sizeof(pBuffer));
    }

    virtual void Enable(bool bEnable)
    {
      if (mpBufferQ)
        mpBufferQ->Enable(bEnable);
    }

    AM_UINT GetAvailBufNum()
    {
      AM_ASSERT(mpBufferQ);
      return mpBufferQ->GetAvailDataNum();
    }

  protected:
    CQueue *mpBufferQ;

  private:
    enum
    {
      INITIAL_QUEUE_LENGTH = 16
    };
};

/*
 * CPacketActiveInputPin
 */
class CPacketActiveInputPin: public CPacketQueueInputPin, public IActiveObject
{
    typedef CPacketQueueInputPin inherited;

  protected:
    typedef IActiveObject AO;

  protected:
    CPacketActiveInputPin(CPacketFilter *pFilter, const char *pName) :
      inherited(pFilter),
      mpWorkQ(NULL),
      mpName(pName)
    {
    }
    AM_ERR Construct();
    virtual ~CPacketActiveInputPin();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete()
    {
      inherited::Delete();
    }

    // IActiveObject
    virtual const char *GetName()
    {
      return mpName;
    }

    // called when stopped, and received CMD_RUN
    virtual void OnRun();

    // called by the thread when a cmd is received
    // should return false for CMD_TERMINATE
    // It is called in STOP state
    virtual void OnCmd(CMD& cmd)
    {
    }

  protected:
    // should release the buffer
    // if return an error, the pin will enter stopped,
    //  and the error is posted to engine
    // return ME_CLOSED means EOS
    virtual AM_ERR ProcessBuffer(CPacket *pBuffer) = 0;

  public:
    AM_ERR Run()
    {
      return mpWorkQ->Run();
    }
    AM_ERR Stop()
    {
      return mpWorkQ->Stop();
    }

  protected:
    CQueue *MsgQ()
    {
      return mpWorkQ->MsgQ();
    }
    void GetCmd(AO::CMD& cmd)
    {
      mpWorkQ->GetCmd(cmd);
    }
    bool PeekCmd(AO::CMD& cmd)
    {
      return mpWorkQ->PeekCmd(cmd);
    }
    void CmdAck(AM_ERR err)
    {
      mpWorkQ->CmdAck(err);
    }

  protected:
    CWorkQueue *mpWorkQ;
    const char *mpName;
};

/*
 * CPacketActiveFilter
 */

class CPacketActiveFilter: public CPacketFilter, public IActiveObject
{
    typedef CPacketFilter inherited;

  protected:
    typedef IActiveObject AO;

  protected:
    CPacketActiveFilter(IEngine *pEngine, const char *pName) :
      inherited(pEngine),
      mpName(pName),
      mpWorkQ(NULL) {}
    AM_ERR Construct(bool RTPriority = false, int priority = 90);
    virtual ~CPacketActiveFilter();

  public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid);
    virtual void Delete()
    {
      inherited::Delete();
    }

    // IPacketFilter
    virtual AM_ERR Run()
    {
      return mpWorkQ->Run();
    }
    virtual AM_ERR Stop()
    {
      return mpWorkQ->Stop();
    }

    virtual void GetInfo(INFO& info);
    //virtual IPin* GetInputPin(AM_UINT index);
    //virtual IPin* GetOutputPin(AM_UINT index);

    // IActiveObject
    virtual const char *GetName()
    {
      return mpName;
    }

    // called when stopped, and received CMD_RUN
    //virtual void OnRun() = 0;

    // called by the thread when a cmd is received
    // should return false for CMD_TERMINATE
    // It is called in STOP state
    virtual void OnCmd(CMD& cmd)
    {
    }

  protected:
    bool WaitInputBuffer(CPacketQueueInputPin*& pPin, CPacket*& pBuffer);
    virtual bool ProcessCmd(CMD& cmd)
    {
      return false;
    }
    virtual void OnStop()
    {
    }

  protected:
    CQueue *MsgQ()
    {
      return mpWorkQ->MsgQ();
    }
    void GetCmd(AO::CMD& cmd)
    {
      mpWorkQ->GetCmd(cmd);
    }
    bool PeekCmd(AO::CMD& cmd)
    {
      return mpWorkQ->PeekCmd(cmd);
    }
    bool PeekMsg(AM_MSG& msg)
    {
      return mpWorkQ->PeekMsg(msg);
    }
    void CmdAck(AM_ERR err)
    {
      mpWorkQ->CmdAck(err);
    }

  protected:
    const char *mpName;
    CWorkQueue *mpWorkQ;
};

/*
 * CPacketFilterGraph
 */
class CPacketFilterGraph: public CBaseEngine
{
  typedef CBaseEngine inherited;

protected:
  CPacketFilterGraph():
    mnFilters(0),
    mnConnections(0)
  {}
  AM_ERR Construct() { return inherited::Construct(); }
  virtual ~CPacketFilterGraph();

protected:

  struct Filter {
    IPacketFilter *pFilter;
    AM_U32 flags;
  };

  struct Connection {
    IPacketPin *pOutputPin;
    IPacketPin *pInputPin;
  };

public:
  void ClearGraph();

  AM_ERR RunAllFilters();
  void StopAllFilters();
  void PurgeAllFilters();
  void DeleteAllConnections();
  void RemoveAllFilters();

  void EnableOutputPins(IPacketFilter *pFilter, bool bEnable = true);
  void DisableOutputPins(IPacketFilter *pFilter)
  { EnableOutputPins(pFilter, false); }

  AM_ERR AddFilter(IPacketFilter *pFilter);
  AM_ERR Connect(IPacketFilter *pUpstreamFilter, AM_UINT indexUp,
                 IPacketFilter *pDownstreamFilter, AM_UINT indexDown);
  AM_ERR CreateConnection(IPacketPin *pOutputPin, IPacketPin *pInputPin);

  const char *GetFilterName(IPacketFilter *pFilter);

private:
  void PurgeFilter(IPacketFilter *pFilter);

protected:
  AM_UINT mnFilters;
  Filter mFilters[16];

  AM_UINT mnConnections;
  Connection mConnections[32];
};

#endif /* AM_BASE_PACKET_H_ */
