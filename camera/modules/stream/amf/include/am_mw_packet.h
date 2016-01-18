/*******************************************************************************
 * am_mw_packet.h
 *
 * Histroy:
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

#ifndef AM_MW_PACKET_H_
#define AM_MW_PACKET_H_

extern const AM_IID IID_IPacketPool;
extern const AM_IID IID_IPacketPin;
extern const AM_IID IID_IPacketFilter;

class CPacket;
class IPacketPool;
class IPacketFilter;

class IPacketPool: public IInterface
{
  public:
    DECLARE_INTERFACE(IPacketPool, IID_IPacketPool);

    virtual const char *GetName() = 0;
    virtual void Enable(bool bEnabled = true) = 0;
    virtual bool AllocBuffer(CPacket*& pBuffer, AM_UINT size) = 0;
    virtual AM_UINT GetAvailBufNum() = 0;
    virtual void AddRef(CPacket *pBuffer) = 0;
    virtual void Release(CPacket *pBuffer) = 0;
    virtual void AddRef() = 0;
    virtual void Release() = 0;
};

class IPacketPin: public IInterface
{
  public:
    DECLARE_INTERFACE(IPacketPin, IID_IPacketPin);

    virtual AM_ERR Connect(IPacketPin *pPeer) = 0;
    virtual void Disconnect() = 0;

    virtual void Receive(CPacket *pBuffer) = 0;
    virtual void Purge() = 0;
    virtual void Enable(bool bEnable) = 0;

    virtual IPacketPin *GetPeer() = 0;
    virtual IPacketFilter *GetFilter() = 0;
};

class IPacketFilter: public IInterface
{
  public:
    struct INFO
    {
        AM_UINT nInput;
        AM_UINT nOutput;
        const char *pName;
    };

  public:
    DECLARE_INTERFACE(IPacketFilter, IID_IPacketFilter);

    virtual AM_ERR Run() = 0;
    virtual AM_ERR Stop() = 0;

    virtual void GetInfo(INFO& info) = 0;
    virtual IPacketPin* GetInputPin(AM_UINT index) = 0;
    virtual IPacketPin* GetOutputPin(AM_UINT index) = 0;
    virtual AM_ERR AddOutputPin(AM_INT& index) = 0;
    virtual AM_ERR AddInputPin(AM_INT& index, AM_UINT type) = 0;
};

/*******************************************************************************
 * CPacket
 * |-------0-------|-------1-------|-------2-------|-------3-------|
 * +----------------------- Payload Header-------------------------+
 * |---------------------------------------------------------------|
 * |              Type             |              Attr             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * |-------0-------|-------1-------|-------2-------|-------3-------|
 * +--------------------------Payload Data-------------------------+
 * |---------------------------------------------------------------|
 * |                      Data PTS Low 32bit                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                      Data PTS High 32bit                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |           Stream ID           |        Data Frame Type        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                       Data Frame Count                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           Data Size                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          Data Pointer                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ******************************************************************************/

class CPacket
{
  public:
    enum AmPayloadType {
      AM_PAYLOAD_TYPE_NONE  = 0,
      AM_PAYLOAD_TYPE_INFO  = 1,
      AM_PAYLOAD_TYPE_DATA  = 2,
      AM_PAYLOAD_TYPE_EOF   = 3,
      AM_PAYLOAD_TYPE_EOS   = 4,
      AM_PAYLOAD_TYPE_EVENT = 5
    };

    enum AmPayloadAttr {
      AM_PAYLOAD_ATTR_NONE      = 0,
      AM_PAYLOAD_ATTR_VIDEO     = 1,
      AM_PAYLOAD_ATTR_AUDIO     = 2,
      AM_PAYLOAD_ATTR_SEI       = 3,
      AM_PAYLOAD_ATTR_EVENT_EMG = 4,
      AM_PAYLOAD_ATTR_EVENT_MD  = 5,
    };

    enum AmPacketType {
      AM_PACKET_TYPE_NONE   = 0,
      AM_PACKET_TYPE_NORMAL = 1 << 0,
      AM_PACKET_TYPE_EVENT  = 1 << 1,
      AM_PACKET_TYPE_STOP   = 1 << 2,
      AM_PACKET_TYPE_SYNC   = 1 << 3,
    };

    struct PayloadHeader
    {
        AM_U16      mPayloadType; /* Info | Data | EOS */
        AM_U16      mPayloadAttr; /* Video | Audio | Sei */
        explicit PayloadHeader() :
          mPayloadType(AM_PAYLOAD_TYPE_NONE),
          mPayloadAttr(AM_PAYLOAD_ATTR_NONE){}
        explicit PayloadHeader(PayloadHeader &header) :
          mPayloadType(header.mPayloadType),
          mPayloadAttr(header.mPayloadAttr){}
        virtual ~PayloadHeader(){}
    };

    struct PayloadData
    {
        AM_U64 mPayloadPts;
        AM_U16 mStreamId;
        AM_U16 mFrameType;  /* Used to specify video type (I|P|B frame)*/
        AM_U32 mFrameAttr;  /* Used to specify data attribute */
        AM_U32 mFrameCount; /* Frame count in this payload */
        AM_U32 mSize;       /* Size of data pointed by mBuffer */
        AM_U32 mOffset;     /* Data offset, used in playback */
        AM_U8 *mBuffer;     /* Data buffer */
        explicit PayloadData() :
          mPayloadPts(0L),
          mStreamId(0),
          mFrameType(0),
          mFrameAttr(0),
          mFrameCount(0),
          mSize(0),
          mOffset(0),
          mBuffer(NULL){}
        explicit PayloadData(PayloadData &data) :
          mPayloadPts(data.mPayloadPts),
          mStreamId(data.mStreamId),
          mFrameType(data.mFrameType),
          mFrameAttr(data.mFrameAttr),
          mFrameCount(data.mFrameCount),
          mSize(data.mSize),
          mOffset(data.mOffset) {
          if (mBuffer) {
            memcpy(mBuffer, data.mBuffer, data.mSize);
          } else {
            mBuffer = data.mBuffer;
          }
        }
        virtual ~PayloadData(){}
    };

    struct Payload
    {
        PayloadHeader mHeader;
        PayloadData   mData;
        explicit Payload(){}
        explicit Payload(Payload &payload) {
          mHeader = payload.mHeader;
          mData = payload.mData;
        }
        explicit Payload(Payload *payload) {
          mHeader = payload->mHeader;
          mData = payload->mData;
        }
        virtual ~Payload(){}
    };

  public:
    explicit CPacket() :
      mRefCount(0),
      mPacketType(AM_PACKET_TYPE_NORMAL),
      mPayload(NULL),
      mpPool(NULL),
      mpNext(NULL){}
    virtual ~CPacket(){}

  public:
    void AddRef() {
      mpPool->AddRef(this);
    }
    void Release() {
      mpPool->Release(this);
    }

    CPacket::Payload* GetPayload(
        CPacket::AmPayloadType type = AM_PAYLOAD_TYPE_NONE,
        CPacket::AmPayloadAttr attr = AM_PAYLOAD_ATTR_NONE)
    {
      if (mPayload) {
        mPayload->mHeader.mPayloadType = type;
        mPayload->mHeader.mPayloadAttr = attr;
      }
      return mPayload;
    }

    void SetPayload(void* payload, AM_U8* dataptr = NULL) {
      mPayload = (CPacket::Payload*)payload;
      if (NULL == dataptr) {
        mPayload->mData.mBuffer = ((AM_U8*)((AM_U8*)payload + sizeof(Payload)));
      }
    }

    void SetDataPtr(AM_U8 *ptr) {
      mPayload->mData.mBuffer = ptr;
    }

    AM_U8 *GetDataPtr() {
      return mPayload->mData.mBuffer;
    }

    AM_UINT GetDataSize() {
      return mPayload->mData.mSize;
    }
    void SetDataSize(AM_UINT size) {
      mPayload->mData.mSize = size;
    }

    AM_UINT GetDataOffset() {
      return mPayload->mData.mOffset;
    }
    void SetDataOffset(AM_U32 offset) {
      mPayload->mData.mOffset = (offset >= mPayload->mData.mSize) ?
          0 : offset;
    }

    AM_U32 GetPacketType() {
      return mPacketType;
    }
    void SetPacketType(AM_U32 type) {
      mPacketType = type;
    }

    CPacket::AmPayloadType GetType() {
      return (CPacket::AmPayloadType)mPayload->mHeader.mPayloadType;
    }
    void SetType(CPacket::AmPayloadType type) {
      mPayload->mHeader.mPayloadType = (AM_U16)type;
    }

    CPacket::AmPayloadAttr GetAttr() {
      return (CPacket::AmPayloadAttr)mPayload->mHeader.mPayloadAttr;
    }
    void SetAttr(CPacket::AmPayloadAttr attr) {
      mPayload->mHeader.mPayloadAttr = (AM_U16)attr;
    }

    AM_PTS GetPTS() {
      return mPayload->mData.mPayloadPts;
    }
    void SetPTS(AM_PTS pts) {
      mPayload->mData.mPayloadPts = pts;
    }

    AM_U16 GetStreamId() {
      return mPayload->mData.mStreamId;
    }
    void SetStreamId(AM_U16 id) {
      mPayload->mData.mStreamId = id;
    }

    AM_U16 GetFrameType() {
      return mPayload->mData.mFrameType;
    }
    void SetFrameType(AM_U16 type) {
      mPayload->mData.mFrameType = type;
    }

    AM_U32 GetFrameCount() {
      return mPayload->mData.mFrameCount;
    }
    void SetFrameCount(AM_U32 count) {
      mPayload->mData.mFrameCount = count;
    }

    bool IsVideoEOS() {
      return ((GetType() == AM_PAYLOAD_TYPE_EOS) &&
              (GetAttr() == AM_PAYLOAD_ATTR_VIDEO));
    }
    bool IsAudioEOS() {
      return ((GetType() == AM_PAYLOAD_TYPE_EOS) &&
              (GetAttr() == AM_PAYLOAD_ATTR_AUDIO));
    }
    bool IsVideoInfo() {
      return ((GetType() == AM_PAYLOAD_TYPE_INFO) &&
              (GetAttr() == AM_PAYLOAD_ATTR_VIDEO));
    }
    bool IsAudioInfo() {
      return ((GetType() == AM_PAYLOAD_TYPE_INFO) &&
              (GetAttr() == AM_PAYLOAD_ATTR_AUDIO));
    }
    bool IsVideoData() {
      return ((GetType() == AM_PAYLOAD_TYPE_DATA) &&
              (GetAttr() == AM_PAYLOAD_ATTR_VIDEO));
    }
    bool IsAudioData() {
      return ((GetType() == AM_PAYLOAD_TYPE_DATA) &&
              (GetAttr() == AM_PAYLOAD_ATTR_AUDIO));
    }
    void CopyData(CPacket *pSrcBuf) {
      mPayload = pSrcBuf->mPayload;
    }

  public:
    am_atomic_t  mRefCount;
    AM_U32       mPacketType;
    Payload     *mPayload;
    IPacketPool *mpPool;
    CPacket     *mpNext;
};

#endif /* AM_MW_PACKET_H_ */
