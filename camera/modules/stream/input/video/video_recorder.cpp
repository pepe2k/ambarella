/*******************************************************************************
 * video_recorder.cpp
 *
 * History:
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

#include "am_include.h"
#include "am_data.h"

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_media_info.h"

#include "video_recorder_if.h"
#include "video_device.h"
#include "video_recorder.h"

#define BUFFER_POOL_SIZE 32

IPacketFilter* create_video_recorder_filter(IEngine* engine)
{
  return CVideoRecorder::Create(engine);
}

/*******************************************************************************
 * CSimpleVideoPool
 ******************************************************************************/
class CSimpleVideoPool: public CSimplePacketPool
{
    typedef CSimplePacketPool inherited;
  public:
    static CSimpleVideoPool* Create(const char *pName,
                                    AM_UINT     count)
    {
      CSimpleVideoPool* result = new CSimpleVideoPool(pName);
      if (AM_UNLIKELY(result &&
                      (ME_OK != result->Construct(count)))) {
        delete result;
        result = NULL;
      }
      return result;
    }

  protected:
    CSimpleVideoPool(const char *pName) :
      inherited(pName),
      mpPayloadMemory(NULL){}
    virtual ~CSimpleVideoPool()
    {
      delete[] mpPayloadMemory;
    }
    AM_ERR Construct(AM_UINT count,
                     AM_UINT objectSize = sizeof(CPacket::Payload))
    {
      AM_ERR ret = inherited::Construct(count);
      if (AM_LIKELY(ME_OK == ret)) {
        mpPayloadMemory = new CPacket::Payload[count];
        if (AM_LIKELY(mpPayloadMemory)) {
          for (AM_UINT i = 0; i < count; ++ i) {
            mpPacketMemory[i].mPayload = &mpPayloadMemory[i];
          }
        } else {
          ret = ME_NO_MEMORY;
        }
      }

      return ret;
    }

  private:
    CPacket::Payload *mpPayloadMemory;
};

/*******************************************************************************
 * CVideoRecorder
 ******************************************************************************/
CVideoRecorder* CVideoRecorder::Create(IEngine *pEngine,
                                       bool RTPriority,
                                       int priority)
{
  CVideoRecorder *result = new CVideoRecorder(pEngine);
  if (AM_UNLIKELY(result && (ME_OK != result->Construct(RTPriority,
                                                        priority)))) {
    delete result;
    result = NULL;
  }

  return result;
}

CVideoRecorder::~CVideoRecorder()
{
  AM_DELETE(mpOutPin);
  AM_DELETE(mpEvent);
  AM_RELEASE(mpPktPool);
  if (AM_LIKELY(mpVdev && mVideoInfo)) {
    for (AM_UINT i = 0; i < mpVdev->GetMaxEncodeNumber(); ++ i) {
      delete mVideoInfo[i];
    }
    delete[] mVideoInfo;
    delete mpVdev;
  }
}

AM_ERR CVideoRecorder::Construct(bool RTPriority, int priority)
{
  AM_ERR ret = inherited::Construct(RTPriority,
                                    priority);
  do {
    if (AM_LIKELY(ME_OK == ret)) {
      if (AM_UNLIKELY(NULL == (mpEvent = CEvent::Create()))) {
        ERROR("Failed to create event!");
        ret = ME_NO_MEMORY;
        break;
      }
      mpPktPool = CSimpleVideoPool::Create("VideoRecorderPktPool",
                                            BUFFER_POOL_SIZE);
      if (AM_UNLIKELY(NULL == mpPktPool)) {
        ERROR("Failed to create packet pool!");
        ret = ME_NO_MEMORY;
        break;
      }
      mpVdev = new CVideoDeviceA5s();
      if (AM_UNLIKELY(NULL == mpVdev)) {
        ERROR("Failed to create video device!");
        ret = ME_NO_MEMORY;
        break;
      }
      if (AM_UNLIKELY(false == mpVdev->Init())) {
        ERROR("Failed to initialize video device!");
        ret = ME_ERROR;
        break;
      }
      mpOutPin = CVideoRecorderOutput::Create(this);
      if (AM_UNLIKELY(NULL == mpOutPin)) {
        ERROR("Failed to allocate memory for output pins!");
        ret = ME_NO_MEMORY;
        break;
      } else {
        mpOutPin->SetBufferPool(mpPktPool);
      }
      mVideoInfo = new AM_VIDEO_INFO* [mpVdev->GetMaxEncodeNumber()];
      if (AM_UNLIKELY(NULL == mVideoInfo)) {
        ERROR("Failed to allocate memory for video info!");
        ret = ME_NO_MEMORY;
        break;
      }
      memset(mVideoInfo, 0,
             sizeof(AM_VIDEO_INFO*)*mpVdev->GetMaxEncodeNumber());
    } else {
      ERROR("Failed to initialize CActivePacketFiler!");
    }
  } while(0);

  return ret;
}

IPacketPin* CVideoRecorder::GetOutputPin(AM_UINT index)
{
  return (IPacketPin*)mpOutPin;
}

AM_ERR CVideoRecorder::Start()
{
  AM_ERR ret = ME_OK;
  if (AM_LIKELY(false == mRun)) {
    /* Start All Available Streams */
    ret = mpVdev->Start(mpVdev->GetMaxEncodeNumber());
    mRun = (ME_OK == ret);
    if (AM_LIKELY(mRun)) {
      mpEvent->Signal();
    }
  }

  return ret;
}

AM_ERR CVideoRecorder::Start(AM_UINT streamid)
{
  return mpVdev->Start(streamid);
}


AM_ERR CVideoRecorder::Stop()
{
  if (AM_UNLIKELY(false == mRun)) {
    mpEvent->Signal(); /* If mRun is false, should unlock OnRun first */
  }
  INFO("Start to stop all streams!");
  mpVdev->Stop(mpVdev->GetMaxEncodeNumber()); /* Stop All Streams */
  INFO("All streams stop done!");
  return inherited::Stop();
}

AM_ERR CVideoRecorder::Stop(AM_UINT streamid)
{
  return mpVdev->Stop(streamid);
}

void CVideoRecorder::OnRun()
{
  bool errProcessed = false;
  mEndedStreamMap = 0;
  CmdAck(ME_OK);
  mpEvent->Clear();
  mpEvent->Wait();

  /* Send video information of every stream */
  if (mRun) {
    bool getInfo = false;
    for (AM_UINT i = 0; i < mpVdev->GetMaxEncodeNumber(); ++ i) {
      if (AM_LIKELY((mVideoStreamMap >> i) & 0x1)) {
        CPacket *videoInfoPkt = NULL;
        if (AM_LIKELY(mpOutPin->AllocBuffer(videoInfoPkt))) {
          CPacket::Payload *payload =
              videoInfoPkt->GetPayload(CPacket::AM_PAYLOAD_TYPE_INFO,
                                       CPacket::AM_PAYLOAD_ATTR_VIDEO);
          if (AM_LIKELY(mVideoInfo[i] == NULL)) {
            mVideoInfo[i] = new AM_VIDEO_INFO;
          }
          payload->mData.mBuffer = (AM_U8*)mVideoInfo[i];
          payload->mData.mStreamId = i;

          if (ME_OK == mpVdev->GetVideoParameter(mVideoInfo[i], i)) {
            INFO("\nStream%u Video Information:\n"
                 "                Video Rate: %u\n"
                 "               Video Scale: %u\n"
                 "               Video Width: %hu\n"
                 "              Video Height: %hu\n"
                 "                         M: %hu\n"
                 "                         N: %hu", i,
                 mVideoInfo[i]->rate,
                 mVideoInfo[i]->scale,
                 mVideoInfo[i]->width,
                 mVideoInfo[i]->height,
                 mVideoInfo[i]->M,
                 mVideoInfo[i]->N);
            payload->mData.mSize = sizeof(AM_VIDEO_INFO);
            mpOutPin->SendBuffer(videoInfoPkt);
            getInfo = true;
          }
        } else {
          mRun = false;
        }
      }
    }
    mRun = getInfo;
  }

  if (AM_UNLIKELY(!mRun)) {
    ERROR("VideoRecorder failed to get video information!");
    PostEngineMsg(IEngine::MSG_ERROR);
  }

  while (mRun) {
    CPacket *videoDataPkt = NULL;
    if (AM_UNLIKELY(false == mpOutPin->AllocBuffer(videoDataPkt))) {
      ERROR("Failed to allocate buffer!");
      break;
    } else {
      AM_ERR ret;
      if (AM_LIKELY(ME_OK == (ret = mpVdev->GetVideoData(videoDataPkt)))) {
        if (AM_UNLIKELY(videoDataPkt->GetType() ==
                        CPacket::AM_PAYLOAD_TYPE_EOS)) {
          mEndedStreamMap |= (1 << videoDataPkt->GetStreamId());
          INFO("Stream%u send EOS!", videoDataPkt->GetStreamId());
          if (AM_LIKELY(mEndedStreamMap == mVideoStreamMap)) {
            mRun = false;
            mEndedStreamMap = 0;
            INFO("All streams have sent EOS!");
          }
        }
        mpOutPin->SendBuffer(videoDataPkt);
      } else {
        videoDataPkt->Release();
        if (AM_LIKELY(!errProcessed)) {
          ERROR("Fatal error occurred in VideoRecoder, abort!");
          mpVdev->Stop(mpVdev->GetMaxEncodeNumber());
          PostEngineMsg(IEngine::MSG_ERROR);
          errProcessed = true;
        }
      }
    }
  }
  if (AM_UNLIKELY(mRun)) {
    ERROR("VideoRecorder exited abnormally!");
    PostEngineMsg(IEngine::MSG_ERROR);
  } else {
    INFO("VideoRecorder exit mainloop!");
  }
}
