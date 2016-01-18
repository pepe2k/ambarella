/*******************************************************************************
 * core.h
 *
 * Histroy:
 *   2012-10-8 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef CORE_H_
#define CORE_H_

class CCore: public ICore
{
  public:
    static CCore *Create(CPacketFilterGraph *graph, IEngine *engine);

  public:
    virtual void *GetInterface(AM_REFIID ref_iid) {
      if (ref_iid == IID_ICore) {
        return (ICore*)this;
      }
      return NULL;
    }
    virtual void Delete() {delete this;}
    virtual AM_ERR CreateSubGraph();
    virtual AM_ERR PrepareToRun(){ return ME_OK; }
    virtual IPacketPin *GetModuleInputPin()
    {
      return (mAvQueue ? mAvQueue->GetInputPin(0) : NULL);
    }
    virtual IPacketPin *GetModuleOutputPin()
    {
      return (mAvQueue ? mAvQueue->GetOutputPin(0) : NULL);
    }
    virtual bool IsReadyForEvent();
    virtual AM_ERR SetEventStreamId(AM_UINT StreamId);
    virtual AM_ERR SetEventDuration(AM_UINT history_duration,
                                    AM_UINT future_duration);

  private:
    CCore(CPacketFilterGraph *graph) :
      mFilterGraph(graph),
      mAvQueue(NULL) {}

    virtual ~CCore() {
      AM_DELETE(mAvQueue);
      DEBUG("~CCore");
    }
    AM_ERR Construct(IEngine *engine);

  private:
    CPacketFilterGraph *mFilterGraph;
    IPacketFilter      *mAvQueue;
};


#endif /* CORE_H_ */
