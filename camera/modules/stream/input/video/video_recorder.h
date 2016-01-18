/*******************************************************************************
 * video_recorder.h
 *
 * Histroy:
 *   2012-10-3 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef VIDEO_RECORDER_H_
#define VIDEO_RECORDER_H_

#include "config.h"

class CVideoRecorderOutput;
class CSimpleVideoPool;

class CVideoRecorder: public CPacketActiveFilter, public IVideoSource
{
    typedef CPacketActiveFilter inherited;

  public:
    static CVideoRecorder* Create(IEngine *pEngine,
#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
                                  bool RTPriority = true,
#else
                                  bool RTPriority = false,
#endif
                                  int priority = CThread::PRIO_HIGH);

  public:
    /* Interface */
    virtual void *GetInterface(AM_REFIID refiid) {
      if (refiid == IID_IVideoSource) {
        return (IVideoSource*)this;
      }

      return inherited::GetInterface(refiid);
    }

    virtual void Delete() {
      inherited::Delete();
    }

    /* IPacketFilter */
    virtual void GetInfo(INFO &info) {
      info.nInput = 0;
      info.nOutput = mpVdev->GetMaxEncodeNumber();
      info.pName = "VideoRecorder";
    }
    virtual IPacketPin* GetOutputPin(AM_UINT index);
    virtual AM_ERR Stop();

    /* IVideoSource */
    virtual AM_ERR Start();
    virtual AM_ERR Start(AM_UINT streamid);
    virtual AM_ERR Stop(AM_UINT streamid);
    virtual void SetEnabledVideoId(AM_UINT streams) {
      mVideoStreamMap = streams;
      mpVdev->SetVideoStreamMap(mVideoStreamMap);
    }
    virtual void SetAvSyncMap(AM_UINT avSyncMap) {
      mAvSyncMap = avSyncMap;
      mpVdev->SetAvSyncMap(mAvSyncMap);
    }
    virtual AM_UINT GetMaxEncodeNumber() {
      return mpVdev->GetMaxEncodeNumber();
    }
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
    virtual void SetFrameStatisticCallback(void (*callback)(void *)) {
      mpVdev->SetFrameStatisticsCallback(callback);
    }
#endif

  private:
    /* IActiveObject */
    virtual void OnRun();

  private:
    CVideoRecorder(IEngine *pEngine) :
      inherited(pEngine, "VideoRecorder"),
      mpEvent(NULL),
      mpVdev(NULL),
      mpPktPool(NULL),
      mpOutPin(NULL),
      mVideoInfo(NULL),
      mVideoStreamMap(0),
      mEndedStreamMap(0),
      mAvSyncMap(0),
      mRun(false) {}
    virtual ~CVideoRecorder();
    AM_ERR Construct(bool RTPriority, int priority);

  private:
    CEvent                *mpEvent;
    CVideoDevice          *mpVdev;
    CSimpleVideoPool      *mpPktPool;
    CVideoRecorderOutput  *mpOutPin;
    AM_VIDEO_INFO        **mVideoInfo;
    AM_UINT                mVideoStreamMap;
    AM_UINT                mEndedStreamMap;
    AM_UINT                mAvSyncMap;
    bool                   mRun;
};

class CVideoRecorderOutput: public CPacketOutputPin
{
    typedef CPacketOutputPin inherited;
    friend class CVideoRecorder;

  public:
    static CVideoRecorderOutput *Create(CPacketFilter *pFilter)
    {
      CVideoRecorderOutput *result = new CVideoRecorderOutput(pFilter);
      if (AM_UNLIKELY(result && (ME_OK != result->Construct()))) {
        delete result;
        result = NULL;
      }
      return result;
    }

  private:
    CVideoRecorderOutput(CPacketFilter *pFilter) :
      inherited(pFilter) {}
    virtual ~CVideoRecorderOutput(){}
    AM_ERR Construct() {return ME_OK;}
};

#endif /* VIDEO_RECORDER_H_ */
