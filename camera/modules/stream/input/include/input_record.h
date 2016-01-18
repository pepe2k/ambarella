/*******************************************************************************
 * input_record.h
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

#ifndef INPUT_RECORD_H_
#define INPUT_RECORD_H_

class CInputRecord: public IInputRecord
{
  public:
    static CInputRecord *Create(CPacketFilterGraph *graph, IEngine *engine);

  public:
    virtual void *GetInterface(AM_REFIID ref_iid)
    {
      if (AM_LIKELY(ref_iid == IID_IInput_Record)) {
        return (IInputRecord*)this;
      }
      return NULL;
    }
    virtual void Delete() {delete this;}

    virtual AM_ERR SetStreamNumber(AM_UINT streamNum);
    virtual AM_ERR SetInputNumber(AM_UINT inputNum);
    virtual AM_UINT GetMaxEncodeNumber();
    virtual AM_UINT GetEnabledVideoId(){return mVideoStreamMap;}
    virtual AM_ERR SetAudioInfo(AM_UINT samplerate, AM_UINT channels,
                                AM_UINT codecType,  void* codecInfo);
    virtual AM_ERR SetEnabledVideoId(AM_UINT streams); /* 1 | 2 | 4 | 8*/
    virtual AM_ERR CreateSubGraph();
    virtual AM_ERR PrepareToRun(){ return ME_OK; }
    virtual IPacketPin *GetModuleInputPin()
    {
      return NULL;
    }

    virtual IPacketPin *GetModuleOutputPin()
    {
      return (mPacketAggregator ? mPacketAggregator->GetOutputPin(0) : NULL);
    }
    virtual AM_ERR SetEnabledAudioId(AM_UINT streams);
    virtual AM_ERR SetAvSyncMap(AM_UINT avSyncMap);
    virtual AM_ERR StartAudio();
    virtual AM_ERR StopAudio();
    virtual AM_ERR StartAll ();
    virtual AM_ERR StopAll ();
    virtual AM_ERR StartVideo(AM_UINT streamid);
    virtual AM_ERR StopVideo(AM_UINT streamid);
    virtual AM_ERR SendSEI(void *pUserData, AM_U32 len);
    virtual AM_ERR SendEvent(CPacket::AmPayloadAttr eventType);
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
    virtual void SetFrameStatisticsCallback(void (*callback)(void *data));
#endif

  private:
    CInputRecord(CPacketFilterGraph *graph) :
      mFilterGraph(graph),
      mVideoRecorder(NULL),
      mAudioEncoder(NULL),
      mAudioCapture(NULL),
      mSeiSender(NULL),
      mEventSender(NULL),
      mPacketAggregator(NULL),
      mAudioStreamMap(0),
      mVideoStreamMap(0),
      mAVSyncMap(0)
    {
    }
    virtual ~CInputRecord()
    {
      AM_DELETE(mVideoRecorder);
      AM_DELETE(mAudioEncoder);
      AM_DELETE(mAudioCapture);
      AM_DELETE(mSeiSender);
      AM_DELETE(mEventSender);
      AM_DELETE(mPacketAggregator);
      DEBUG ("~CInputRecord");
    }
    AM_ERR Construct(IEngine *engine);

  private:
    CPacketFilterGraph *mFilterGraph;
    IPacketFilter      *mVideoRecorder;
    IPacketFilter      *mAudioEncoder;
    IPacketFilter      *mAudioCapture;
    IPacketFilter      *mSeiSender;
    IPacketFilter      *mEventSender;
    IPacketFilter      *mPacketAggregator;
    AM_UINT             mAudioStreamMap;
    AM_UINT             mVideoStreamMap;
    AM_UINT             mAVSyncMap;
};

#endif /* INPUT_RECORD_H_ */
