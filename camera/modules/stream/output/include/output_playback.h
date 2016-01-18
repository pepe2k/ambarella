/*******************************************************************************
 * output_playback.h
 *
 * History:
 *   2013-3-20 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef OUTPUT_PLAYBACK_H_
#define OUTPUT_PLAYBACK_H_

class IPlayer;
class COutputPlayback: public IOutputPlayback
{
  public:
    static COutputPlayback* Create(CPacketFilterGraph* graph, IEngine *engine);

  public:
    virtual void* GetInterface(AM_REFIID refiid)
    {
      if (AM_LIKELY(refiid == IID_IOutputPlayback)) {
        return (IOutputPlayback*)this;
      }

      return NULL;
    }
    virtual void Delete()
    {
      delete this;
    }
    virtual AM_ERR Stop();
    virtual AM_ERR Pause(bool enable);
    virtual AM_ERR CreateSubGraph();
    virtual IPacketPin* GetModuleInputPin()
    {
      return mPktDistributor->GetInputPin(0);
    }

  private:
    COutputPlayback(CPacketFilterGraph* graph);
    virtual ~COutputPlayback();
    AM_ERR Construct(IEngine* engine);

  private:
    CPacketFilterGraph *mFilterGraph;
    IPacketFilter      *mPktDistributor;
    IPacketFilter      *mPlayerFilter;
    IPlayer            *mPlayer;
};

#endif /* OUTPUT_PLAYBACK_H_ */
