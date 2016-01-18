/*******************************************************************************
 * audio_decoder.h
 *
 * History:
 *   2013-3-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AUDIO_DECODER_H_
#define AUDIO_DECODER_H_

#include "config.h"

class CAudioDecoderInput;
class CAudioDecoderOutput;
class CAudioCodec;

class CAudioDecoder: public CPacketActiveFilter, public IAudioDecoder
{
    typedef CPacketActiveFilter inherited;
    friend class CAudioDecoderInput;

  public:
    static CAudioDecoder* Create(IEngine *engine,
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
      info.pName = (const char*)"AudioDeocder";
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
      if (AM_LIKELY(mRun)) {
        mRun = false;
        inherited::Stop();
      }
      return ME_OK;
    }
    virtual void OnRun();
    virtual void* GetInterface(AM_REFIID refiid)
    {
      if (AM_LIKELY(refiid == IID_IAudioDecoder)) {
        return (IAudioDecoder*)this;
      }

      return inherited::GetInterface(refiid);
    }
    virtual void Delete() {inherited::Delete();}

  private:
    virtual AM_ERR SetAudioCodecType(AM_UINT type);
    inline AM_ERR OnInfo(CPacket* packet);
    inline AM_ERR OnData(CPacket* packet);
    inline AM_ERR OnEOF(CPacket* packet);

  private:
    CAudioDecoder(IEngine *engine) :
      inherited(engine, "AudioDeocder"),
      mAudioCodec(NULL),
      mInputPin(NULL),
      mOutputPin(NULL),
      mPacketPool(NULL),
      mBuffer(NULL),
      mRun(false),
      mIsInfoSent(false),
      mAudioDataSize(0),
      mAudioDataPreSkip(0),
      mCodecType(AM_AUDIO_CODEC_NONE)
    {
      memset(&mCodecInfo, 0, sizeof(mCodecInfo));
    }
    virtual ~CAudioDecoder();
    AM_ERR Construct(bool RTPriority, int priority);

  private:
    CAudioCodec         *mAudioCodec;
    CAudioDecoderInput  *mInputPin;
    CAudioDecoderOutput *mOutputPin;
    IPacketPool         *mPacketPool;
    AM_U8               *mBuffer;
    bool                 mRun;
    bool                 mIsInfoSent;
    AM_UINT              mAudioDataSize;
    AM_UINT              mAudioDataPreSkip;
    AM_AUDIO_INFO        mSourceInfo;
    AudioCodecInfo       mCodecInfo;
    AmAudioCodecType     mCodecType;
};

class CAudioDecoderInput: public CPacketQueueInputPin
{
    typedef CPacketQueueInputPin inherited;
    friend class CAudioDeocder;

  public:
    static CAudioDecoderInput* Create(CPacketFilter *filter);

  private:
    CAudioDecoderInput(CPacketFilter *filter) :
      inherited(filter){}
    virtual ~CAudioDecoderInput(){}
    AM_ERR Construct()
    {
      return inherited::Construct(((CAudioDecoder*)mpFilter)->MsgQ());
    }
};

class CAudioDecoderOutput: public CPacketOutputPin
{
    typedef CPacketOutputPin inherited;
    friend class CAudioDecoder;

  public:
    static CAudioDecoderOutput* Create(CPacketFilter *filter);

  private:
    CAudioDecoderOutput(CPacketFilter *filter) :
      inherited(filter){}
    virtual ~CAudioDecoderOutput(){}
    AM_ERR Construct() {return ME_OK;}
};

#endif /* AUDIO_DECODER_H_ */
