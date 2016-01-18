
/*
 * av_queue_builder.h
 *
 * History:
 *    2012/8/10 - [Shupeng Ren] created file
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AV_QUEUE_BUILDER_H__
#define __AV_QUEUE_BUILDER_H__

struct ExPayload: public CPacket::Payload
{
  bool        mIsNormalUse;
  bool        mIsEventUse;
  am_atomic_t mRefCount;
  explicit ExPayload():
    Payload(),
    mIsNormalUse(false),
    mIsEventUse(false),
    mRefCount(0)
  {}
  void operator=(CPacket::Payload& payload)
  {
    memcpy(&mHeader, &(payload.mHeader), sizeof(mHeader));
    memcpy(&mData, &(payload.mData), sizeof(mData));
    mIsNormalUse = false;
    mIsEventUse = false;
    mRefCount = 0;
  }
  virtual ~ExPayload(){}
};

class CRingPacketPool
{
  public:
    static CRingPacketPool* Create(AM_UINT     count,
                                   AM_UINT     dataSize);

  protected:
    CRingPacketPool() :
      mpRingBufferMem(NULL),
      mpRingBufferMemEnd(NULL),
      mpCurrent(NULL),
      mFreeMem(0),
      mpPayloadPool(NULL),
      mPayloadCount(0),
      mMaxDataSize(0),
      mReadablePayloadCount(0),
      mReadablePayloadCount_e(0),
      mReadPos(0),
      mReadPos_e(0),
      mWritePos(0),
      mInUsedPayload(0),
      mInUsedPayload_e(0),
      mpMutex(NULL)
      {}
    AM_ERR Construct(AM_UINT count, AM_UINT dataSize);
    virtual ~CRingPacketPool()
    {
      delete [] mpRingBufferMem;
      delete [] mpPayloadPool;
      AM_DELETE(mpMutex);
    }

  public:
    ExPayload* GetAVQPayload(bool isEvent);
    void AVQPayloadAdvance(bool isEvent);
    void AVQPayloadDrop();
    void AVQRelease(ExPayload* payload);
    void AVQPayloadBack();
    void AVQWrite(CPacket::Payload* payload, bool isEvent);
    ExPayload* GetPrevAVQPayload();
    bool IsAVQPayloadFull(bool isEvent);
    bool IsAVQPayloadEmpty(bool isEvent);
    bool IsAVQPayloadUsed();
    bool IsAVQAboutToFull(bool isEvent);
    bool IsAVQAboutToEmpty(bool isEvent);
    bool IsAVQEventSync();
    void InitVar();
    void InitEvent();
    AM_INT GetFreeMemSize();
    AM_INT GetFreePayloadCount();
    void Delete() {delete this;}

  private:
    AM_U8     *mpRingBufferMem; //Data memery pool ptr
    AM_U8     *mpRingBufferMemEnd; //Data memery pool end ptr
    AM_U8     *mpCurrent; //Current data memery pool write ptr
    AM_UINT     mFreeMem;
    ExPayload *mpPayloadPool;
    AM_UINT    mPayloadCount; // Numbers of Payload struct
    AM_UINT    mMaxDataSize;
    AM_UINT    mReadablePayloadCount; //used AVQueue Info struct numbers
    AM_UINT    mReadablePayloadCount_e;
    AM_UINT    mReadPos; //AVQueue Info read position
    AM_UINT    mReadPos_e;
    AM_UINT    mWritePos; //AVQueue Info write position
    AM_UINT    mInUsedPayload;
    AM_UINT    mInUsedPayload_e;
    CMutex    *mpMutex;
};
#endif
