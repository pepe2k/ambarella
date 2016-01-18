/*******************************************************************************
 * player.h
 *
 * History:
 *   2013-3-21 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef PLAYER_H_
#define PLAYER_H_

class CPlayerInput;
class CPlayer: public CPacketActiveFilter, public IPlayer
{
    typedef CPacketActiveFilter inherited;
    friend class CPlayerInput;

  public:
    static CPlayer* Create(IEngine *engine,
#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
                           bool RTPriority = true,
#else
                           bool RTPriority = false,
#endif
                           int priority = CThread::PRIO_LOW);

  public:
    virtual void* GetInterface(AM_REFIID refiid)
    {
      if (AM_LIKELY(refiid == IID_IPlayer)) {
        return (IPlayer*)this;
      }

      return inherited::GetInterface(refiid);
    }
    virtual void Delete()
    {
      inherited::Delete();
    }
    virtual void GetInfo(INFO& info)
    {
      info.nInput = 1;
      info.nOutput = 0;
      info.pName = "PlayerFiter";
    }
    virtual IPacketPin* GetInputPin(AM_UINT id)
    {
      return (IPacketPin*)mInputPin;
    }
    virtual AM_ERR Start();
    virtual AM_ERR Stop();
    virtual AM_ERR Pause(bool enable);
    virtual void OnRun();

  private:
    inline AM_ERR OnInfo(CPacket *packet);
    inline AM_ERR OnData(CPacket *packet);
    inline AM_ERR OnEOF(CPacket *packet);
    AM_ERR ProcessBuffer(CPacket *packet);

  private:
    CPlayer(IEngine *engine);
    virtual ~CPlayer();
    AM_ERR Construct(bool RTPriority, int priority);

  private:
    CMutex       *mMutex;
    CPlayerInput *mInputPin;
    CPlayerAudio *mAudioPlayer;
    AM_UINT       mEosMap;
    bool          mRun;
    bool          mIsPause;
};

class CPlayerInput: public CPacketActiveInputPin
{
    typedef CPacketActiveInputPin inherited;
    friend class CPlayer;

  public:
    static CPlayerInput* Create(CPacketFilter *filter, const char *name)
    {
      CPlayerInput* result = new CPlayerInput(filter, name);
      if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
        delete result;
        result = NULL;
      }
      return result;
    }
  private:
    CPlayerInput(CPacketFilter *filter, const char *name) :
      inherited(filter, name){}
    ~CPlayerInput(){}
    AM_ERR Construct()
    {
      return inherited::Construct();
    }
    virtual AM_ERR ProcessBuffer(CPacket *buffer)
    {
      return ((CPlayer*)mpFilter)->ProcessBuffer(buffer);
    }
};

#endif /* PLAYER_H_ */
