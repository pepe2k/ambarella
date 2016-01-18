/*
 * mp4_muxer.cpp
 *
 * History:
 *	2009/5/26 - [Jacky Zhang] created file
 *	2011/2/23 - [ Yi Zhu] modified
 *	2011/9/13 - [ Jay Zhang] modified
 *	2012/11/7 - [Dongge Wu] modified for Cloud cam
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "am_include.h"
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_muxer_info.h"
#include "output_record_if.h"
#include "am_media_info.h"
#include "mp4_builder.h"
#include "am_muxer_info.h"
#include "file_sink.h"
#include "mp4_file_writer.h"
#include "mp4_builder.h"
#include "mp4_muxer.h"

//-----------------------------------------------------------------------
//
// CMp4Muxer
//
//-----------------------------------------------------------------------
IPacketFilter * CreateMp4Muxer(IEngine *pEngine)
{
  return (IPacketFilter *)CMp4Muxer::Create(pEngine);
}

CMp4Muxer *CMp4Muxer::Create(IEngine *pEngine, bool RTPriority, int priority)
{
  CMp4Muxer *result = new CMp4Muxer(pEngine);
  if (AM_UNLIKELY(result && result->Construct(RTPriority, priority) != ME_OK)) {
    delete result;
    result = NULL;
  }
  return result;
}

CMp4Muxer::CMp4Muxer(IEngine *pEngine):
      inherited(pEngine, "mp4_muxer"),
      mbRun(false),
      mbAudioEnable (false),
      mbIsFirstAudio (true),
      mbIsFirstVideo (true),
      mbNeedSplitted (false),
      mEOSMap(0),
      mEOFMap(0),
      mEventMap(0),
      mEventNormalSync(1),
      mpFileName(NULL),
      mFileCount(0),
      mSplittedDuration (0LL),
      mNextFileBoundary (0LL),
      mpMP4Builder (NULL),
      mpMediaInput (NULL),
      mpDataWriter (NULL),
      mBufferdUsrSEI(NULL),
      mCountBuffer(-1),
      mLastVideoPTS (0),
      mFileVideoFrameCount (0),
      mVideoFrameCount (0)
{
  memset(mBuffer, 0, sizeof(mBuffer[0]) * MAX_COUNT_BUFFER);
}

AM_ERR CMp4Muxer::Construct(bool RTPriority, int priority)
{
  AM_ERR err = inherited::Construct(RTPriority, priority);
  if(err != ME_OK) {
    return err;
  }

  if(AM_UNLIKELY((mpMediaInput = CMp4MuxerInput::Create(this)) == NULL)) {
    return ME_ERROR;
  }

  mpMediaInput->Enable (true);

  return ME_OK;
}

CMp4Muxer::~CMp4Muxer()
{
  AM_DELETE(mpMediaInput);
  AM_DELETE(mpMP4Builder);
}

void *CMp4Muxer::GetInterface(AM_REFIID refiid)
{
  if (refiid == IID_IMediaMuxer) {
    return (IMediaMuxer *)this;
  }

  return inherited::GetInterface(refiid);
}

void CMp4Muxer::GetInfo(INFO& info)
{
  info.nInput = 3;
  info.nOutput = 0;
  info.pName = "Mp4Muxer";
}

IPacketPin* CMp4Muxer::GetInputPin(AM_UINT index)
{
  if (index == 0) {
    return mpMediaInput;
  }

  return NULL;
}

AM_ERR CMp4Muxer::Stop()
{
  if (mbRun) {
    mbRun = false;
  }

  inherited::Stop();
  return ME_OK;
}

AM_ERR CMp4Muxer::SetMediaSink (AmSinkType sinkType, const char *pDestStr)
{
  AM_DELETE (mpDataWriter);
  AM_DELETE (mpMP4Builder);

  if (sinkType == AM_SINK_TYPE_MP4) {
    mpDataWriter = CMp4FileWriter::Create ();
  } else {
    ERROR ("No such mp4 writer adapter.\n");
    return ME_ERROR;
  }

  if (mpDataWriter == NULL) {
    ERROR ("Failed to create mp4 writer adapter.\n");
    return ME_ERROR;
  }

  if ((mpMP4Builder = new CMp4Builder(mpDataWriter)) == NULL) {
    return ME_ERROR;
  }

  return mpDataWriter->SetMediaSink (pDestStr);
}

AM_ERR CMp4Muxer::SetMediaSourceMask (AM_UINT mediaSourceMask)
{
  return ME_OK;
}

AM_ERR CMp4Muxer::SetSplitDuration (AM_U64 durationIn90KBase)
{
  if (durationIn90KBase == 0) {
    mbNeedSplitted = false;
    return ME_OK;
  }

  mbNeedSplitted = true;
  mSplittedDuration = durationIn90KBase;
  mNextFileBoundary = durationIn90KBase;

  return ME_OK;
}

inline AM_ERR CMp4Muxer::OnInfo (CPacket *packet)
{
  static int AVInfoMap = 0;

  if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
    /* Video info */
    AM_VIDEO_INFO *pInfo =  (AM_VIDEO_INFO *)packet->GetDataPtr ();
    NOTICE ("Mp4 Muxer receive H264 INFO: "
        "size %dx%d, M %d, N %d, rate %d, scale %d\n",
        pInfo->width, pInfo->height, pInfo->M,
        pInfo->N, pInfo->rate, pInfo->scale);
    mpMP4Builder->InitH264(pInfo);
    mFileVideoFrameCount = (((mSplittedDuration * pInfo->scale) << 1) + \
                              pInfo->rate * 90000) / ((pInfo->rate * 90000) << 1) \
                              * pInfo->mul / pInfo->div;
    AVInfoMap |= 1 << 1;
  } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
    /* Audio info */
    AM_AUDIO_INFO *pInfo =  (AM_AUDIO_INFO *)packet->GetDataPtr ();
    NOTICE ("Mp4 Muxer receive Audio INFO: freq %d, chuck %d, channel %d\n",
            pInfo->sampleRate, pInfo->chunkSize, pInfo->channels);
    mpMP4Builder->InitAudio(pInfo);

    if (pInfo->format != MF_AAC) {
      ERROR ("Currently, MP4 muxer only support aac audio.\n");
      return ME_ERROR;
    }

    AVInfoMap |= 1 << 0;
  } else {
    NOTICE ("Currently, MP4 muxer only support audio and video stream.\n");
    return ME_ERROR;
  }

  if (AM_LIKELY(AVInfoMap == 3)) {
    INFO ("Mp4 muxer has received both audio and video info.\n");
    mpMP4Builder->InitProcess();
    AVInfoMap = 0;
  }

  return ME_OK;
}

inline AM_ERR CMp4Muxer::AddBuffer (CPacket *packet)
{
  if(++mCountBuffer <= MAX_COUNT_BUFFER - 1) {
    packet->AddRef();
    mBuffer[mCountBuffer] = packet;
  } else {
    WARN("Buffer is full, please increase its size.\n");
    mCountBuffer--;
  }

  INFO("mCountBuffer = %d \n", mCountBuffer);
  return ME_OK;
}

inline AM_ERR CMp4Muxer::OnData (CPacket *packet)
{
  if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
    /* Video Data */
    if (AM_UNLIKELY (mbIsFirstVideo)) {
      /* first frame must be video frame,
       * and the first video frame must be I frame */
      mbAudioEnable = true;
      mbIsFirstVideo = !(packet->GetFrameType() == IDR_FRAME && mbIsFirstAudio);
      AM_ASSERT (packet->GetFrameType () == IDR_FRAME && mbIsFirstAudio);
    }

    if (AM_UNLIKELY(mbNeedSplitted && /* Splitting file is needed */
                    ((mVideoFrameCount >= mFileVideoFrameCount) ||
                     (packet->GetPTS() >= mNextFileBoundary)) &&
                        (packet->GetFrameType() == IDR_FRAME) &&
                        (mEOFMap == 0))) { /* EOF is not set */

      OnEOF (packet); /* Finalize old file & create new file */
      INFO("### Video EOF is reached, PTS: %llu, Video frame count: %u",
           packet->GetPTS(), mVideoFrameCount);
      mVideoFrameCount = 0;
      AddBuffer(packet);
      return ME_OK;
    }

    if (AM_UNLIKELY(mEOFMap == 2)){
      AddBuffer(packet);
      return ME_OK;
    }

    AM_ENSURE_OK_( mpMP4Builder->put_VideoData(packet->GetDataPtr(),
                                               packet->GetDataSize(),
                                               packet->GetPTS(),
                                               mBufferdUsrSEI));
    if (AM_UNLIKELY(mBufferdUsrSEI)) {
      mBufferdUsrSEI->Release();
      mBufferdUsrSEI = NULL;
    }

    ++ mVideoFrameCount;
    mLastVideoPTS = packet->GetPTS();
  } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
    /* Audio Data */
    if (AM_UNLIKELY(!mbAudioEnable)) {
      INFO ("Video frame has not been received, "
          "drop current audio packet: %llu", packet->GetPTS());
      return ME_OK;
    }

    if (AM_UNLIKELY(mpMP4Builder->put_AudioData(packet->GetDataPtr(),
                                                packet->GetDataSize(),
                                                packet->GetFrameCount())
                    == ME_TOO_MANY)){
      NOTICE("The file size is too larger, stop stream engine!");
      PostEngineMsg(IEngine::MSG_EOS);
      return ME_OK;
    }
    mbIsFirstAudio = false;

    if (AM_UNLIKELY(mbNeedSplitted && /* Splitting file is needed */
                    (mEOFMap == 0x2))) { /* Video is first */
      INFO("*** Audio EOF is reached, PTS: %llu", packet->GetPTS());
      OnEOF (packet);
      //mpMediaInput->Enable(false);
      mbIsFirstAudio = true;
    }
  } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_SEI) {
    if (AM_UNLIKELY(!mBufferdUsrSEI)) {
      WARN("Last SEI data is not processed! dropped!");
      mBufferdUsrSEI->Release();
      mBufferdUsrSEI = NULL;
    }
    packet->AddRef();
    mBufferdUsrSEI = packet;
    INFO("*** Get usr SEI data");
  } else {
    NOTICE ("Currently, mp4 muxer just support audio and video stream.\n");
    return ME_ERROR;
  }

  return ME_OK;
}

inline AM_ERR CMp4Muxer::OnEventData (CPacket *packet)
{
  if (packet->GetAttr() == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
    /* Video Data */
    if (AM_UNLIKELY (mbIsFirstVideo)) {
      /* first frame must be video frame,
       * and the first video frame must be I frame */
      mbAudioEnable = true;
      mbIsFirstVideo = !(packet->GetFrameType() == IDR_FRAME && mbIsFirstAudio);
      AM_ASSERT (packet->GetFrameType () == IDR_FRAME && mbIsFirstAudio);
    }

    AM_ENSURE_OK_( mpMP4Builder->put_VideoData(packet->GetDataPtr(),
                                               packet->GetDataSize(),
                                               packet->GetPTS(),
                                               mBufferdUsrSEI));
    if (mBufferdUsrSEI) {
      mBufferdUsrSEI->Release();
      mBufferdUsrSEI = NULL;
    }

    mLastVideoPTS = packet->GetPTS();
  } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
    /* Audio Data */
    if (AM_UNLIKELY(!mbAudioEnable)) {
      INFO ("Video frame has not been received, "
          "drop current audio packet: %llu", packet->GetPTS());
      return ME_OK;
    }

    if (AM_UNLIKELY(mpMP4Builder->put_AudioData(packet->GetDataPtr(),
                                                packet->GetDataSize(),
                                                packet->GetFrameCount())
                    == ME_TOO_MANY)) {
      NOTICE("The file size is too larger, stop stream engine!");
      PostEngineMsg(IEngine::MSG_EOS);
      return ME_OK;
    }
  } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_SEI) {
    packet->AddRef();
    mBufferdUsrSEI = packet;
  } else {
    NOTICE ("Currently, mp4 muxer just support audio and video stream.\n");
    return ME_ERROR;
  }

  return ME_OK;
}

inline AM_ERR CMp4Muxer::OnAVData(CPacket *packet)
{
  if (AM_UNLIKELY(packet->mPacketType & CPacket::AM_PACKET_TYPE_SYNC)) {
    /*Event pointer equals normal pointer*/
    mEventNormalSync = 1;
    if (mEventMap == 0){
      return OnData(packet);
    } else {
      return OnEventData(packet);
    }
  }

  /* mEventMap determins calling which function,
   * mEventNormalSync and mPacketType
   * determin getting which type packet
   */
  if (mEventMap == 0 && ((mEventNormalSync == 1 &&
      packet->mPacketType == CPacket::AM_PACKET_TYPE_NORMAL) ||
      (mEventNormalSync == 0 &&
          packet->mPacketType == CPacket::AM_PACKET_TYPE_EVENT))) {
    /*Normal recording*/
    return OnData(packet);
  } else if (mEventMap == 1 && ((mEventNormalSync == 1 &&
      packet->mPacketType == CPacket::AM_PACKET_TYPE_NORMAL) ||
      (mEventNormalSync == 0 &&
          packet->mPacketType == CPacket::AM_PACKET_TYPE_EVENT))) {
    /*Event recording*/
    return OnEventData(packet);
  } else if (mEventMap == 1 &&
      (packet->mPacketType & CPacket::AM_PACKET_TYPE_STOP)) {
    /*Switch to normal recording*/
    mpMP4Builder->FinishProcess();
    mpDataWriter->OnEOF ();

    mEventMap = 0;
    mpMP4Builder->InitProcess();
    mNextFileBoundary = mLastVideoPTS + mSplittedDuration;
    return OnData(packet);
  } else {
    DEBUG("Discard the packet");
  }

  return ME_OK;
}

inline AM_ERR CMp4Muxer::OnEOF(CPacket *packet)
{
  AM_INT i = 0;
  if (!mbRun)
    return ME_OK;

  if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) { //video EOF
    mEOFMap |= 0x1 << 1;
  } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_AUDIO) { //audio EOF
    mEOFMap |= 0x1 << 0;
  } else {
    NOTICE ("Currently, MP4 muxer just support audio and video stream.\n");
    return ME_ERROR;
  }

  if (mEOFMap == 0x3) {
    mpMP4Builder->FinishProcess();
    mpDataWriter->OnEOF ();
    mEOFMap = 0;

    mpMP4Builder->InitProcess();
    mNextFileBoundary = mLastVideoPTS + mSplittedDuration;
    mbIsFirstVideo = true;
    mbIsFirstAudio = true;
    for (i = 0; i <= mCountBuffer; i++) {
      OnData(mBuffer[i]);
      mBuffer[i]->Release();
    }

    mCountBuffer = -1;
    INFO("Next file boundary is %llu", mNextFileBoundary);
  }

  return ME_OK;
}

inline AM_ERR CMp4Muxer::OnEvent(CPacket *packet)
{
  if (!mbRun)
    return ME_OK;

  mpMP4Builder->FinishProcess();
  mpDataWriter->OnEvent();
  mEOFMap = 0;

  mpMP4Builder->InitProcess();
  mEventMap = 1 << 0;
  mVideoFrameCount = 0;
  mEventNormalSync = 0;
  INFO("call OnEvent");

  return ME_OK;
}

inline AM_ERR CMp4Muxer::OnEOS (CPacket *packet)
{
  if (!mbRun) {
    return ME_OK;
  }

  if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
    mEOSMap |= 1 << 0;
  } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
    mEOSMap |= 1 << 1;
  } else {
    NOTICE ("Currently, mp4 muxer just support audio and video stream.\n");
    return ME_ERROR;
  }

  DEBUG ("EOS map 0x%x\n", mEOSMap);
  if (mEOSMap == 0x3 ) { /* received both video and audio EOS */
    mpMP4Builder->FinishProcess();
    mpDataWriter->OnEOS ();
    mbRun = false;
    mbIsFirstVideo = true;
    mbIsFirstAudio = true;
    mEOFMap = 0;
    mEOSMap = 0;
    mEventMap = 0;
    mEventNormalSync = 1;
    mCountBuffer = -1;
  }

  return ME_OK;
}

void CMp4Muxer::OnRun()
{
  mbRun = true;
  CmdAck(ME_OK);
  AM_ENSURE_OK_ (mpDataWriter->Init());

  while (mbRun) {
    CPacket *pInBuffer;
    CPacketQueueInputPin *pInPin;
    if (!WaitInputBuffer(pInPin, pInBuffer))
      break;   // filter aborted, exit main loop

    switch (pInBuffer->GetType()) {
      case CPacket::AM_PAYLOAD_TYPE_INFO:
        OnInfo(pInBuffer);
        break;

      case  CPacket::AM_PAYLOAD_TYPE_DATA:
        OnAVData(pInBuffer);
        break;

      case  CPacket::AM_PAYLOAD_TYPE_EOS:
        OnEOS(pInBuffer);
        break;

      case  CPacket::AM_PAYLOAD_TYPE_EVENT:
        OnEvent(pInBuffer);
        break;

      default:
        break;
    }
    pInBuffer->Release();
  }
  AM_ENSURE_OK_ (mpDataWriter->Deinit());

  PostEngineMsg(IEngine::MSG_EOS);
  INFO("MP4 Muxer exit mainloop\n");
}

//-----------------------------------------------------------------------
//
// CMp4MuxerInput
//
//-----------------------------------------------------------------------
CMp4MuxerInput* CMp4MuxerInput::Create(CPacketFilter *pFilter)
{
  CMp4MuxerInput* result = new CMp4MuxerInput(pFilter);
  if (result && result->Construct() != ME_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

AM_ERR CMp4MuxerInput::Construct()
{
  AM_ERR err = inherited::Construct(((CMp4Muxer*)mpFilter)->MsgQ());
  if (err != ME_OK)
    return err;
  return ME_OK;
}

