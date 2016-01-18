/*
 * common_ts_muxer.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 11/09/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __TS_MUXER_H__
#define __TS_MUXER_H__

#include "config.h"

struct CTSMUX_PID_INFO
{
    AM_U16 PMT_PID;
    AM_U16 VIDEO_PES_PID;
    AM_U16 AUDIO_PES_PID;
    AM_U16 reserved;
};

struct CTSMUX_CONFIG
{
    char *dest_name;
    AM_U32 max_filesize;
    AM_U32 max_videocnt;
    CTSMUX_PID_INFO *pid_info;
};

struct PACKET_BUF
{
    AM_U16 pid;
    AM_U8 buf[MPEG_TS_TP_PACKET_SIZE];
};

enum
{
  TS_PACKET_SIZE = MPEG_TS_TP_PACKET_SIZE,
  TS_DATA_BUFFER_SIZE = MPEG_TS_TP_PACKET_SIZE * 1000,
  MAX_CODED_AUDIO_FRAME_SIZE = 8192,
  AUDIO_CHUNK_BUF_SIZE = MAX_CODED_AUDIO_FRAME_SIZE + MPEG_TS_TP_PACKET_SIZE * 4
};

enum
{
  TS_AUDIO_PACKET = 0x1,
  TS_VIDEO_PACKET = 0x2,
};

class CTsMuxerInput;

class CTsMuxer: public CPacketActiveFilter, public IMediaMuxer
{
    typedef CPacketActiveFilter inherited;
    friend class CTsMuxerInput;

  public:
    static CTsMuxer *Create(IEngine *engine,
#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
                            bool RTPriority = true,
#else
                            bool RTPriority = false,
#endif
                            int priority = CThread::PRIO_LOW);

  private:
    CTsMuxer(IEngine *);
    AM_ERR Construct(bool RTPriority, int priority);
    virtual ~CTsMuxer();

  public:
    /* Interfaces declared at IInterface */
    virtual void *GetInterface(AM_REFIID);
    virtual void Delete()
    {
      inherited::Delete();
    }

    /* Interfaces declared at IFilter */
    virtual AM_ERR Stop();
    virtual void GetInfo(INFO &);
    virtual IPacketPin *GetInputPin(AM_UINT);

    /* Interfaces declared at IMediaMuxer */
    virtual AM_ERR SetMediaSink(AmSinkType, const char *);
    virtual AM_ERR SetMediaSourceMask(AM_UINT mediaSourceMask);
    virtual AM_ERR SetSplitDuration(AM_U64 durationIn90KBase = 0);
    virtual AM_ERR SetMaxFileAmount(AM_UINT maxFileAmount)
    {
      return ME_OK;
    }

    /* Interfaces declared at IActiveObject */
    virtual void OnRun();

  protected:
    virtual AM_ERR WriteData(AM_U8 *, int);
    virtual AM_ERR CleanUp();
    virtual AM_ERR OnEOS(CPacket *);
    virtual AM_ERR OnEOF(CPacket *);
    virtual AM_ERR OnEvent(CPacket *);
    virtual AM_ERR OnAVBuffer(CPacket *);

  private:
    AM_ERR OnAVInfo(CPacket *);
    AM_ERR InitTs();
    inline void BuildAudioTsPacket(AM_U8 *data, AM_UINT size, AM_PTS pts);
    inline void BuildAndFlushAudio(AM_PTS pts);
    inline void BuildVideoTsPacket(AM_U8 *data, AM_UINT size, AM_PTS pts);
    inline void BuildPatPmt();
    inline void UpdatePatPmt();
    inline AM_ERR PcrSync(AM_PTS);
    inline AM_ERR PcrIncrement(AM_PTS);
    inline AM_ERR PcrCalcPktDuration(AM_U32, AM_U32);

  private:
    bool mbRun;
    bool mbAudioEnable;
    bool mbIsFirstAudio;
    bool mbIsFirstVideo;
    bool mbNeedSplitted;
    bool mbNeedSplittedCopy;
    bool mbEventFlag;
    bool mbEventNormalSync;

    AM_UINT mEOFMap;
    AM_UINT mEOSMap;
    AM_U64 mSplittedDuration;
    AM_U64 mNextFileBoundary;
    AM_U64 mPtsBase;

    CTsBuilder *mpTsBuilder;
    CTsMuxerInput *mpMediaInput;
    ITsDataWriter *mpDataWriter;

    PACKET_BUF mPatBuf;
    PACKET_BUF mPmtBuf;
    PACKET_BUF mVideoPesBuf;
    PACKET_BUF mAudioPesBuf;

    CTSMUXPSI_PAT_INFO mPatInfo;
    CTSMUXPSI_PMT_INFO mPmtInfo;
    CTSMUXPSI_PRG_INFO mPrgInfo;
    CTSMUXPSI_STREAM_INFO mVideoStreamInfo;
    CTSMUXPSI_STREAM_INFO mAudioStreamInfo;

    AM_VIDEO_INFO mH264Info;
    AM_AUDIO_INFO mAudioInfo;

    AM_U8 mLpcmDescriptor[8];
    AM_U8 mPmtDescriptor[4];
    AM_PTS mPcrBase;
    AM_U16 mPcrExt;
    AM_PTS mPcrIncBase;
    AM_U16 mPcrIncExt;

    AM_U8 *mpAudioChunkBuf;
    AM_U8 *mpAudioChunkBufWrPtr;
    AM_UINT mAudioChunkBufAvail;
    AM_PTS mLastVideoPTS;
    AM_UINT mFileVideoFrameCount;
    AM_UINT mVideoFrameCount;
};

class CTsMuxerInput: public CPacketQueueInputPin
{
    typedef CPacketQueueInputPin inherited;
    friend class CTsMuxer;

  public:
    static CTsMuxerInput *Create(CPacketFilter *pFilter)
    {
      CTsMuxerInput* result = new CTsMuxerInput (pFilter);
      if (result && result->Construct () != ME_OK) {
        delete result;
        result = NULL;
      }
      return result;
    }

  private:
    CTsMuxerInput(CPacketFilter *pFilter) :
        inherited(pFilter)
    {
    }
    virtual ~CTsMuxerInput()
    {
    }
    AM_ERR Construct()
    {
      return inherited::Construct(((CTsMuxer*)mpFilter)->MsgQ());
    }
};

#endif
