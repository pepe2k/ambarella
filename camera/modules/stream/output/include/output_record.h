/*
 * output_record.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 21/09/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __OUTPUT_RECORD_H__
#define __OUTPUT_RECORD_H__

typedef IPacketFilter *(*MuxerCreator)(IEngine *);

class COutputRecord: public IOutputRecord
{
  public:
    static COutputRecord *Create(CPacketFilterGraph *, IEngine *);

  private:
    COutputRecord(CPacketFilterGraph *, IEngine *);
    AM_ERR Construct();
    virtual ~COutputRecord();

  public:
    /* Interfaces declared at IInterface */
    virtual void *GetInterface(AM_REFIID);
    virtual void Delete()
    {
      delete this;
    }

    /* Interfaces declared at IOutputRecord */
    virtual AM_ERR SetMuxerMask(AmStreamNumber, AM_UINT);
    virtual AM_ERR SetMediaSource(AmStreamNumber, AmMuxerType);
    virtual AM_ERR SetMediaSink(AmStreamNumber, AmMuxerType, const char *);
    virtual AM_ERR SetMaxFileAmount(AmStreamNumber, AmMuxerType, AM_UINT);
    virtual AM_ERR SetSplitDuration(AmStreamNumber, AmMuxerType, AM_UINT, bool);
    virtual IPacketPin *GetModuleInputPin();
    virtual AM_ERR CreateSubGraph();
    virtual AM_ERR PrepareToRun();
    virtual AM_UINT GetMuxerAmount();
    virtual void SetRtspAttribute(bool SendNeedWait, bool RtspNeedAuth);

  private:
    void SpecialProcessForRTSPAndRaw();
    inline int GetMuxerIndex(AmMuxerType);
    inline AM_UINT GetOneBitNum(AM_UINT);

  private:
    MuxerCreator mpMuxerCreatorArray[MUXER_TYPE_AMOUNT];
    AM_U64 mDuration[AM_STREAM_NUMBER_MAX][MUXER_TYPE_AMOUNT];
    AM_UINT mSourceMask[AM_STREAM_NUMBER_MAX][MUXER_TYPE_AMOUNT];
    AM_UINT mFileAmount[AM_STREAM_NUMBER_MAX][MUXER_TYPE_AMOUNT];
    char *mpMediaSink[AM_STREAM_NUMBER_MAX][MUXER_TYPE_AMOUNT];
    IPacketFilter *mpMuxerFilter[AM_STREAM_NUMBER_MAX][MUXER_TYPE_AMOUNT];
    IMediaMuxer *mpMuxerArray[AM_STREAM_NUMBER_MAX][MUXER_TYPE_AMOUNT];
    AM_UINT mMuxerMask[AM_STREAM_NUMBER_MAX];
    IEngine *mpEngine;
    CPacketFilterGraph *mpFilterGraph;
    IPacketFilter *mpPacketDistributor;
    bool mRtspSendWait;
    bool mRtspNeedAuth;
};

#endif //__OUTPUT_RECORD_H__
