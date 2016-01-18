/*******************************************************************************
 * input_playback.h
 *
 * History:
 *   2013-3-4 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef INPUT_PLAYBACK_H_
#define INPUT_PLAYBACK_H_

class IDemuxer;
class CInputPlayback: public IInputPlayback
{
    enum AmMediaType {
      AM_MEDIA_UNKNOWN,
      AM_MEDIA_AAC,
      AM_MEDIA_TS,
      AM_MEDIA_MP4
    };

  public:
    static CInputPlayback *Create(CPacketFilterGraph *graph, IEngine *engine);

  public:
    virtual void* GetInterface(AM_REFIID ref_iid)
    {
      if (AM_LIKELY(ref_iid == IID_IInput_Playback)) {
        return (IInputPlayback*)this;
      }
      return NULL;
    }
    virtual void Delete() {delete this;}
    virtual AM_ERR AddUri(const char* uri);
    virtual AM_ERR Play(const char* uri);
    virtual AM_ERR Stop();
    virtual AM_ERR CreateSubGraph();
    virtual IPacketPin* GetModuleInputPin()
    {
      return NULL;
    }
    virtual IPacketPin* GetModuleOutputPin()
    {
      return (mPacketAggregator ? mPacketAggregator->GetOutputPin(0) : NULL);
    }

  private:
    CInputPlayback(CPacketFilterGraph *graph) :
      mFilterGraph(graph),
      mDemuxerFilter(NULL),
      mAudioDecoder(NULL),
      mPacketAggregator(NULL),
      mUniversalDemuxer(NULL){}
    virtual ~CInputPlayback()
    {
      AM_DELETE(mDemuxerFilter);
      AM_DELETE(mAudioDecoder);
      AM_DELETE(mPacketAggregator);
      DEBUG("~CInputPlayback");
    }
    AM_ERR Construct(IEngine *engine);

  private:
    CPacketFilterGraph *mFilterGraph;
    IPacketFilter      *mDemuxerFilter;
    IPacketFilter      *mAudioDecoder;
    IPacketFilter      *mPacketAggregator;
    IDemuxer           *mUniversalDemuxer;
};

#endif /* INPUT_PLAYBACK_H_ */
