/*
 * output_record_if.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 10/09/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __OUTPUT_RECORD_IF_H__
#define __OUTPUT_RECORD_IF_H__

extern const AM_IID IID_IMediaMuxer;
extern const AM_IID IID_IOutputRecord;
extern const AM_IID IID_ITsDataWriter;
extern const AM_IID IID_IMp4DataWriter;
extern const AM_IID IID_IJpegDataWriter;
extern const AM_IID IID_IRtspFilter;

class IMediaMuxer: public IInterface
{
public:
   DECLARE_INTERFACE (IMediaMuxer, IID_IMediaMuxer);
   virtual AM_ERR SetMediaSink (AmSinkType sinkType, const char *destStr) = 0;
   virtual AM_ERR SetSplitDuration (AM_U64 durationIn90KBase = 0)         = 0;
   virtual AM_ERR SetMediaSourceMask (AM_UINT mediaSourceMask)            = 0;
   virtual AM_ERR SetMaxFileAmount (AM_UINT fileAmount)                   = 0;
};

class IOutputRecord: public IInterface
{
public:
   DECLARE_INTERFACE (IOutputRecord, IID_IOutputRecord);
   virtual AM_ERR SetMuxerMask     (AmStreamNumber streamNum,
                                    AM_UINT        muxerMask) = 0;
   virtual AM_ERR SetMediaSource   (AmStreamNumber streamNum,
                                    AmMuxerType    muxer)     = 0;
   virtual AM_ERR SetMediaSink     (AmStreamNumber streamNum,
                                    AmMuxerType    muxer,
                                    const char    *destStr)   = 0;
   virtual AM_ERR SetMaxFileAmount (AmStreamNumber streamNum,
                                    AmMuxerType    muxer,
                                    AM_UINT maxFileAmount)    = 0;
   virtual AM_ERR SetSplitDuration (AmStreamNumber streamNum,
                                    AmMuxerType    muxer,
                                    AM_UINT        duration,
                                    bool           align)     = 0;
   virtual AM_ERR CreateSubGraph ()                           = 0;
   virtual AM_ERR PrepareToRun ()                             = 0;
   virtual IPacketPin *GetModuleInputPin ()                   = 0;
   virtual AM_UINT GetMuxerAmount()                           = 0;
   virtual void SetRtspAttribute(bool SendNeedWait,
                                 bool RtspNeedAuth)           = 0;
};

class ITsDataWriter: public IInterface
{
public:
   DECLARE_INTERFACE (ITsDataWriter, IID_ITsDataWriter);
   virtual AM_ERR Init () = 0;
   virtual AM_ERR Deinit () = 0;
   virtual void OnEOF (int streamType) = 0;
   virtual void OnEOS (int streamType) = 0;
   virtual void OnEvent () = 0;
   virtual AM_ERR SetMediaSink (const char *destStr) = 0;
   virtual AM_ERR SetSplitDuration (AM_U64 splitDuration) = 0;
   virtual AM_ERR WriteData (AM_U8 *pData, int dataLen, int dataType) = 0;
};

class IMp4DataWriter: public IInterface
{
public:
   DECLARE_INTERFACE (ITsDataWriter, IID_IMp4DataWriter);
   virtual AM_ERR Init () = 0;
   virtual AM_ERR Deinit () = 0;
   virtual void OnEOF () = 0;
   virtual void OnEvent () = 0;
   virtual void OnEOS () = 0;
   virtual AM_ERR SetMediaSink (const char *destStr) = 0;
   virtual AM_ERR SetSplitDuration (AM_U64 splitDuration) = 0;
   virtual AM_ERR WriteData (AM_U8 *pData, int dataLen) = 0;
    virtual AM_ERR SeekData(am_file_off_t offset, AM_UINT whence) = 0;
};

class IJpegDataWriter: public IInterface
{
public:
   DECLARE_INTERFACE (IJpegDataWriter, IID_IJpegDataWriter);
   virtual AM_ERR Init () = 0;
   virtual AM_ERR Deinit () = 0;
   virtual AM_ERR SetMediaSink (const char *destStr) = 0;
   virtual AM_ERR SetMaxFileAmount (AM_UINT fileAmount) = 0;
   virtual AM_ERR WriteData (AM_U8 *pData, int dataLen) = 0;
};

class IRtspFilter: public IInterface
{
  public:
    DECLARE_INTERFACE(IRtspFilter, IID_IRtspFilter);
    virtual void SetRtspAttribute(bool needWait, bool needAuth) = 0;
};

#endif //__OUTPUT_RECORD_IF_H__
