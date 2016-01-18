/*******************************************************************************
 * audio_encoder.h
 *
 * Histroy:
 *   2012-9-26 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AUDIO_ENCODER_H_
#define AUDIO_ENCODER_H_

#include "config.h"

class CAudioEncoderInput;
class CAudioEncoderOutput;
class CAudioCodec;

class CAudioEncoder: public CPacketActiveFilter, public IAudioEncoder
{
    typedef CPacketActiveFilter inherited;
    friend class CAudioEncoderInput;

  public:
    static CAudioEncoder* Create(IEngine *pEngine,
#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
                                 bool RTPriority = true,
#else
                                 bool RTPriority = false,
#endif
                                 int priority = CThread::PRIO_NORMAL);

  public:
    /* IPacketFilter */
    virtual void GetInfo(INFO &info)
    {
      info.nInput = 1;
      info.nOutput = 1;
      info.pName = (const char *)"AudioEncoder";
    }
    virtual IPacketPin* GetInputPin(AM_UINT index)
    {
      return (IPacketPin*)((index == 0) ? mInputPin : NULL);
    }
    virtual IPacketPin* GetOutputPin(AM_UINT index)
    {
      return (IPacketPin*)mOutputPin;
    }
    virtual AM_ERR Start() {return ME_OK;}
    virtual AM_ERR Stop()
    {
      mRun = false;
      return inherited::Stop();
    }

    /* IActiveObject */
    virtual void OnRun();

    /* IAudioEncoder */
    virtual void *GetInterface(AM_REFIID refiid) {
      if (refiid == IID_IAudioEncoder) {
        return (IAudioEncoder*)this;
      }

      return inherited::GetInterface(refiid);
    }

    virtual void Delete() {inherited::Delete();}
    virtual AM_ERR SetAudioCodecType(AM_UINT type, void *codecInfo);

  private:
    void CalculateEncoderInputSize();
    inline AM_ERR OnInfo(CPacket *packet);
    inline AM_ERR OnData(CPacket *packet);
    inline AM_ERR OnEOS(CPacket *packet);

  private:
    CAudioEncoder(IEngine *pEngine) :
      inherited(pEngine, "AudioEncoder"),
      mAudioCodec(NULL),
      mInputPin(NULL),
      mOutputPin(NULL),
      mPacketPool(NULL),
      mRun(false),
      mAudioDataSize(0),
      mEncoderInputSize(0),
      mAudioStreamNum(0),
      mCodecType(AM_AUDIO_CODEC_NONE)
    {
      memset(&mCodecInfo, 0, sizeof(mCodecInfo));
    }
    virtual ~CAudioEncoder();
    AM_ERR Construct(bool RTPriority, int priority);

  private:
    CAudioCodec         *mAudioCodec;
    CAudioEncoderInput  *mInputPin;
    CAudioEncoderOutput *mOutputPin;
    IPacketPool         *mPacketPool;
    bool                 mRun;
    AM_UINT              mAudioDataSize;
    AM_UINT              mEncoderInputSize;
    AM_INT               mAudioStreamNum;
    AudioCodecInfo       mCodecInfo;
    AmAudioCodecType     mCodecType;
};

class CAudioEncoderInput: public CPacketQueueInputPin
{
    typedef CPacketQueueInputPin inherited;
    friend class CAudioEncoder;

  public:
    static CAudioEncoderInput* Create(CPacketFilter *pFilter);

  private:
    CAudioEncoderInput(CPacketFilter *pFilter):
      inherited(pFilter){}
    virtual ~CAudioEncoderInput(){}
    AM_ERR Construct()
    {
      return inherited::Construct(((CAudioEncoder*)mpFilter)->MsgQ());
    }
};

class CAudioEncoderOutput: public CPacketOutputPin
{
    typedef CPacketOutputPin inherited;
    friend class CAudioEncoder;

  public:
    static CAudioEncoderOutput* Create(CPacketFilter *pFilter);

  private:
    CAudioEncoderOutput(CPacketFilter *pFilter) :
      inherited(pFilter){}
    virtual ~CAudioEncoderOutput(){}
    AM_ERR Construct() {return ME_OK;}

};

#endif /* AUDIO_ENCODER_H_ */
