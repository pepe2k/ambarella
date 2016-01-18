/*******************************************************************************
 * am_acapture.h
 *
 * Histroy:
 *   2012-9-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AUDIO_CAPTURE_H_
#define AUDIO_CAPTURE_H_

#include "config.h"

class CAudioCaptureOutput;

class CAudioCapture: public CPacketActiveFilter, public IAudioSource
{
    typedef CPacketActiveFilter inherited;
    typedef std::queue<CPacket*> PacketQueue;
    friend class CPulseAudio;

  public:
    static CAudioCapture* Create(IEngine *pEngine,
#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
                                 bool RTPriority = true,
#else
                                 bool RTPriority = false,
#endif
                                 int priority = CThread::PRIO_HIGH);

  public:
    void SendBuffer(CPacket *packet);

  public:
    /* IInterface */
    virtual void* GetInterface(AM_REFIID refiid) {
      if (refiid == IID_IAudioSource) {
        return (IAudioSource*)this;
      }
      return inherited::GetInterface(refiid);
    }
    virtual void Delete() {
      inherited::Delete();
    }

    /* IPacketFilter */
    virtual void GetInfo(INFO& info)
    {
      info.nInput = 0;
      info.nOutput = 1;
      info.pName = "AudioCapture";
    }
    virtual IPacketPin* GetOutputPin(AM_UINT index)
    {
      return (IPacketPin*)mpOutPin;
    }

    /* IAudioSource */
    virtual AM_ERR Start();
    virtual AM_ERR Stop();
    virtual void SetEnabledAudioId(AM_UINT streams)
    {
      mAudioStreamMap = streams;
      mpAdev->SetAudioStreamMap(mAudioStreamMap);
    }
    virtual void SetAvSyncMap(AM_UINT avSyncMap)
    {
      mAvSyncMap = avSyncMap;
      mpAdev->SetAvSyncMap(mAvSyncMap);
    }

    void SetAudioParameters(AM_UINT samplerate,
                            AM_UINT channel,
                            AM_UINT codecType)
    {
      mpAdev->SetAudioSampleRate(samplerate);
      mpAdev->SetAudioChannelNumber(channel);
      mpAdev->SetAudioCodecType(codecType);
    }

  private:
    /* IActiveObject */
    virtual void OnRun();

  private:
    CAudioCapture(IEngine *pEngine);
    virtual ~CAudioCapture();
    AM_ERR Construct(bool RTPriority, int priority);

  private:
    CAudioDevice        *mpAdev;
    CAudioCaptureOutput *mpOutPin;
    CFixedPacketPool    *mpPktPool;
    CEvent              *mpEvent;
    PacketQueue         *mpPacketQ;
    bool                 mbRun;
    bool                 mIsAdevStarted;
    AM_UINT              mAudioStreamMap;
    AM_UINT              mAvSyncMap;
};

/*
 * CAudioCaptureOutput
 */
class CAudioCaptureOutput: public CPacketOutputPin
{
    typedef CPacketOutputPin inherited;
    friend class CAudioCapture;

  public:
    static CAudioCaptureOutput* Create(CPacketFilter *pFilter)
    {
      CAudioCaptureOutput *ret = new CAudioCaptureOutput(pFilter);
      if (ret && (ret->Construct() != ME_OK)) {
        delete ret;
        ret = NULL;
      }

      return ret;
    }

  private:
    CAudioCaptureOutput(CPacketFilter *pFilter) :
      inherited(pFilter){}
    virtual ~CAudioCaptureOutput() {}
    AM_ERR Construct()
    {
      return ME_OK;
    }
};

#endif /* AUDIO_CAPTURE_H_ */
