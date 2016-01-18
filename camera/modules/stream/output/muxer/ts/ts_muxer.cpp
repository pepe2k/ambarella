/*
 * ts_muxer.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 12/09/2012
 *
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
#include "mpeg_ts_defs.h"
#include "am_media_info.h"
#include "ts_builder.h"
#include "file_sink.h"
#include <curl/curl.h>
#include "ts_file_writer.h"
#include "ts_http_writer.h"
#include "ts_hls_writer.h"
#include "ts_muxer.h"

IPacketFilter *CreateTsMuxer (IEngine *pEngine)
{
   return (IPacketFilter *)CTsMuxer::Create (pEngine);
}

//-----------------------------------------------------------------------
//
// CTsMuxer
//
//-----------------------------------------------------------------------
CTsMuxer *CTsMuxer::Create (IEngine *pEngine, bool RTPriority, int priority)
{
   CTsMuxer *result = new CTsMuxer (pEngine);
   if (result != NULL && result->Construct(RTPriority, priority) != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

CTsMuxer::CTsMuxer (IEngine *pEngine) :
   inherited (pEngine, "ts_muxer"),
   mbRun (false),
   mbAudioEnable (false),
   mbIsFirstAudio (true),
   mbIsFirstVideo (true),
   mbNeedSplitted (false),
   mbNeedSplittedCopy (false),
   mbEventFlag (false),
   mbEventNormalSync (true),
   mEOFMap (0),
   mEOSMap (0),
   mSplittedDuration (0LLU),
   mNextFileBoundary (0LLU),
   mPtsBase(0LLU),
   mpTsBuilder (NULL),
   mpMediaInput (NULL),
   mpDataWriter (NULL),
   mPcrBase (0),
   mPcrExt (0),
   mPcrIncBase (0),
   mPcrIncExt (0),
   mpAudioChunkBuf (NULL),
   mpAudioChunkBufWrPtr (NULL),
   mAudioChunkBufAvail (AUDIO_CHUNK_BUF_SIZE),
   mLastVideoPTS (0),
   mFileVideoFrameCount (0),
   mVideoFrameCount (0)
{}

AM_ERR CTsMuxer::Construct (bool RTPriority, int priority)
{
   if (inherited::Construct (RTPriority, priority) != ME_OK) {
      ERROR ("Failed to construct parent.\n");
      return ME_ERROR;
   }

   if ((mpMediaInput = CTsMuxerInput::Create (this)) == NULL) {
      ERROR ("Failed to create output pin for ts muxer.\n");
      return ME_ERROR;
   }

   mpMediaInput->Enable (true);

   if ((mpTsBuilder = new CTsBuilder ()) == NULL) {
      ERROR ("Failed to create ts builder.\n");
      return ME_ERROR;
   }

   if ((mpAudioChunkBufWrPtr = mpAudioChunkBuf =
            new AM_U8[AUDIO_CHUNK_BUF_SIZE]) == NULL) {
      ERROR ("Failed to allocate memory for audio chunk.\n");
      return ME_NO_MEMORY;
   }

   mPatBuf.pid = 0;
   mPmtBuf.pid = 0x100;
   mVideoPesBuf.pid = 0x200;
   mAudioPesBuf.pid = 0x400;

   return ME_OK;
}

CTsMuxer::~CTsMuxer ()
{
  AM_DELETE (mpTsBuilder);
  AM_DELETE (mpMediaInput);
  AM_DELETE (mpDataWriter);
  delete[] mpAudioChunkBuf;
}

void* CTsMuxer::GetInterface (AM_REFIID refiid)
{
   if (refiid == IID_IMediaMuxer) {
      return (IMediaMuxer *)this;
   }
   return inherited::GetInterface (refiid);
}

void CTsMuxer::GetInfo (INFO& info)
{
   info.nInput = 1;
   info.nOutput = 0;
   info.pName = "TsMuxer";
}

IPacketPin* CTsMuxer::GetInputPin (AM_UINT index)
{
   if (index == 0) {
      return mpMediaInput;
   }

   return NULL;
}

AM_ERR CTsMuxer::Stop ()
{
  AM_ERR ret = ME_OK;
  if (AM_LIKELY(mbRun)) {
    mbRun = false;
    ret = inherited::Stop();
  }
  return ret;
}

AM_ERR CTsMuxer::SetMediaSink (AmSinkType sinkType, const char *pDestStr)
{
   AM_DELETE (mpDataWriter);

   if (sinkType == AM_SINK_TYPE_TS_FILE) {
      mpDataWriter = CTsFileWriter::Create ();
   } else if (sinkType == AM_SINK_TYPE_TS_HTTP) {
      mpDataWriter = CTsHttpWriter::Create (this);
   } else if (sinkType == AM_SINK_TYPE_TS_HLS) {
      mpDataWriter = CTsHlsWriter::Create ();
   } else {
      ERROR ("No such ts writer adapter.\n");
      return ME_ERROR;
   }

   if (mpDataWriter == NULL) {
      ERROR ("Failed to create ts writer adapter.\n");
      return ME_ERROR;
   }

   if (mpDataWriter->SetSplitDuration (mSplittedDuration) != ME_OK) {
      ERROR ("Failed to set split duration for writer adapter");
      return ME_ERROR;
   }

   if (mpDataWriter->SetMediaSink (pDestStr) != ME_OK) {
      ERROR ("Failed to set media sink for writer adapter");
      return ME_ERROR;
   }

   return ME_OK;
}

AM_ERR CTsMuxer::SetMediaSourceMask (AM_UINT mediaSourceMask)
{
   return ME_OK;
}

AM_ERR CTsMuxer::SetSplitDuration (AM_U64 durationIn90KBase)
{
   if (durationIn90KBase == 0) {
      mbNeedSplitted = false;
      return ME_OK;
   }

   mbNeedSplitted = true;
   mbNeedSplittedCopy = true;
   mSplittedDuration = durationIn90KBase;
   mNextFileBoundary = durationIn90KBase;

   return ME_OK;
}

AM_ERR CTsMuxer::CleanUp ()
{
   return ME_OK;
}

AM_ERR CTsMuxer::OnAVInfo (CPacket *packet)
{
   static int AVInfoMap = 0;

   if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
      /* Video info */
      AM_VIDEO_INFO *pInfo =  (AM_VIDEO_INFO *)packet->GetDataPtr ();
      NOTICE ("Ts Muxer receive H264 INFO: "
            "size %dx%d, M %d, N %d, rate %d, scale %d, mul %d, div %d\n",
            pInfo->width, pInfo->height, pInfo->M,
            pInfo->N, pInfo->rate, pInfo->scale, pInfo->mul, pInfo->div);
      memcpy (&mH264Info, pInfo, sizeof (mH264Info));
      mpTsBuilder->SetVideoInfo(pInfo);

      PcrCalcPktDuration (mH264Info.rate, mH264Info.scale);
      mPcrBase = mH264Info.rate * mH264Info.div / mH264Info.mul;
      mPcrExt = 0;
      mFileVideoFrameCount = (((mSplittedDuration * pInfo->scale) << 1) + \
                                pInfo->rate * 90000) / ((pInfo->rate * 90000) << 1) \
                                * pInfo->mul / pInfo->div;
      NOTICE ("mFileVideoFrameCount = %d\n", mFileVideoFrameCount);
      AVInfoMap |= 1 << 1;
   } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
      /* Audio info */
      AM_AUDIO_INFO *pInfo =  (AM_AUDIO_INFO *)packet->GetDataPtr ();
      NOTICE ("Ts Muxer receive Audio INFO: freq %d, chuck %d, channel %d\n",
            pInfo->sampleRate, pInfo->chunkSize, pInfo->channels);
      memcpy (&mAudioInfo, pInfo, sizeof (mAudioInfo));
      mpTsBuilder->SetAudioInfo(pInfo);

      if (pInfo->format == MF_AAC) {
         mAudioStreamInfo.type = MPEG_SI_STREAM_TYPE_AAC;
         mAudioStreamInfo.descriptor_tag = 0x52;
         mAudioStreamInfo.descriptor_len = 0;
         mAudioStreamInfo.pDescriptor = NULL;
      } else if (pInfo->format == MF_BPCM) {
         AM_U8 channelIdx = (pInfo->channels == 1) ? 1 :
             ((pInfo->channels == 2) ? 3 : 6);
         AM_U8 sampleFreqIdx = (pInfo->sampleRate == 48000) ? 1 :
             ((pInfo->sampleRate == 96000) ? 4 : 5);
         mAudioStreamInfo.type = MPEG_SI_STREAM_TYPE_LPCM_AUDIO;
         mAudioStreamInfo.descriptor_len = 8;
         mAudioStreamInfo.pDescriptor    = mLpcmDescriptor;
         mAudioStreamInfo.pDescriptor[0] = 'H'; // 0x48
         mAudioStreamInfo.pDescriptor[1] = 'D'; // 0x44
         mAudioStreamInfo.pDescriptor[2] = 'M'; // 0x4d
         mAudioStreamInfo.pDescriptor[3] = 'V'; // 0x56
         mAudioStreamInfo.pDescriptor[4] = 0x00;
         mAudioStreamInfo.pDescriptor[5] = MPEG_SI_STREAM_TYPE_LPCM_AUDIO;
         mAudioStreamInfo.pDescriptor[6] = (channelIdx << 4) | (sampleFreqIdx & 0xF);
         mAudioStreamInfo.pDescriptor[7] = (1 << 6) | 0x3F;
      } else {
         ERROR ("Currently, we just support aac and bpcm audio.\n");
         return ME_ERROR;
      }

      AVInfoMap |= 1 << 0;
   } else {
      NOTICE ("Currently, ts muxer just support audio and video stream.\n");
      return ME_ERROR;
   }

   if (AVInfoMap == 3) {
      INFO ("Ts muxer has received both audio and video info.\n");
      BuildPatPmt();
      AVInfoMap = 0;
   }

   return ME_OK;
}

AM_ERR CTsMuxer::OnAVBuffer (CPacket *packet)
{
   if (AM_UNLIKELY (packet->mPacketType & CPacket::AM_PACKET_TYPE_SYNC)) {
      /* Event pointer equals normal pointer. */
      mbEventNormalSync = true;
      NOTICE ("Event pointer equals normal pointer!");
   }

   if (!mbEventNormalSync &&
         (packet->mPacketType & CPacket::AM_PACKET_TYPE_NORMAL)) {
      /* Normal data will be discarded in emergency recording */
      DEBUG ("Normal packet [pts = %llu]: will be discarded", packet->GetPTS ());
      return ME_OK;
   }

   if ((mbEventFlag && (packet->mPacketType & CPacket::AM_PACKET_TYPE_STOP)) ||
         ((mPcrBase >= (AM_U64)((1LL << 33) - (mH264Info.scale << 1))) &&
           ((packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) &&
            (packet->GetFrameType () == IDR_FRAME)))) {
      mbIsFirstAudio = true;
      mbIsFirstVideo = true;

      mPcrBase = mH264Info.rate * mH264Info.div / mH264Info.mul;
      mPcrExt = 0;
      mVideoFrameCount = 0;

      mpDataWriter->OnEOF (AUDIO_STREAM);
      mpDataWriter->OnEOF (VIDEO_STREAM);
      mNextFileBoundary = mLastVideoPTS + mSplittedDuration;
      BuildPatPmt ();

      if (mbEventFlag) {
         mbEventFlag = false;
         mbNeedSplitted = mbNeedSplittedCopy;
      }
   }

   if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
      /* Video Data */
      /* INFO("### VideoPts: %llu", packet->GetPTS()); */
      if (AM_UNLIKELY (mbIsFirstVideo)) {
         /* first frame must be video frame,
          * and the first video frame must be I frame */
         mbAudioEnable = true;
         AM_ASSERT (packet->GetFrameType () == IDR_FRAME && mbIsFirstAudio);
         mPtsBase = packet->GetPTS();
      } else {
         PcrIncrement (packet->GetPTS ());
      }

      if (AM_UNLIKELY(mbNeedSplitted && /* Splitting file is needed */
          ((mVideoFrameCount >= mFileVideoFrameCount) ||
           (packet->GetPTS() >= mNextFileBoundary)) &&/* Boundary is reached */
          (packet->GetFrameType() == IDR_FRAME) && /* Video is I frame */
          (mEOFMap == 0))) { /* EOF is not set */
         /* Duplicate this packet to last file
          * to work around TS file duration calculation issue
          */
         UpdatePatPmt();
         BuildVideoTsPacket(packet->GetDataPtr(),
                            packet->GetDataSize(),
                            packet->GetPTS());
         OnEOF (packet); /* Finalize old file & create new file */
         /* Write in new file */
         BuildPatPmt();
         INFO("### Video EOF is reached, PTS: %llu, Video frame count: %u",
              packet->GetPTS(), mVideoFrameCount);
         NOTICE ("PTS = %lld, mPcrBase = %lld",
               packet->GetPTS () - mPtsBase, mPcrBase);
         mVideoFrameCount = 0;
      }
      if (AM_UNLIKELY (packet->GetFrameType () == IDR_FRAME)) {
         if (AM_LIKELY (!mbIsFirstVideo)) {
            UpdatePatPmt();
         }
      }

      BuildVideoTsPacket(packet->GetDataPtr(),
                         packet->GetDataSize(),
                         packet->GetPTS());
      ++ mVideoFrameCount;
      mLastVideoPTS = packet->GetPTS();
      mbIsFirstVideo = false;
   } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
      /* Audio Data */
      /* INFO("*** AudioPts: %llu", packet->GetPTS()); */
      if (AM_UNLIKELY(!mbAudioEnable)) {
         INFO ("Video frame has not been received, "
               "drop current audio packet: %llu", packet->GetPTS());
         return ME_OK;
      }

      BuildAudioTsPacket(packet->GetDataPtr(), packet->GetDataSize(),
                         packet->GetPTS());

      if (AM_UNLIKELY(mbNeedSplitted && /* Splitting file is needed */
          (mEOFMap == 0x2))) { /* Video is first */
         INFO("*** Audio EOF is reached, PTS: %llu", packet->GetPTS());
         BuildAndFlushAudio(packet->GetPTS());
         OnEOF (packet);
         mbIsFirstAudio = true;
      }
   } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_SEI) {
     NOTICE("Got SEI packet, which is useless in TS muxer!");
   } else {
     NOTICE ("Currently, ts muxer just support audio and video stream.\n");
     return ME_ERROR;
   }

   return ME_OK;
}

AM_ERR CTsMuxer::OnEOF (CPacket *packet)
{
   if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
      mEOFMap |= 0x1 << 0;
      mpDataWriter->OnEOF (AUDIO_STREAM);
   } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
      mEOFMap |= 0x1 << 1;
      mpDataWriter->OnEOF (VIDEO_STREAM);
   } else {
      NOTICE ("Currently, ts muxer just support audio and video stream.\n");
      return ME_ERROR;
   }

   if (mEOFMap == 0x3) {
      mEOFMap = 0;
      mNextFileBoundary = mLastVideoPTS + mSplittedDuration;
      INFO("Next file boundary is %llu", mNextFileBoundary);
   }

   return ME_OK;
}

AM_ERR CTsMuxer::OnEOS (CPacket *packet)
{
   if (!mbRun) {
      return ME_OK;
   }

   if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
      BuildAndFlushAudio(packet->GetPTS());
      mEOSMap |= 1 << 0;
      mpDataWriter->OnEOS (AUDIO_STREAM);
   } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
      mEOSMap |= 1 << 1;
      mpDataWriter->OnEOS (VIDEO_STREAM);
   } else {
      NOTICE ("Currently, ts muxer just support audio and video stream.\n");
      return ME_ERROR;
   }

   DEBUG ("EOS map 0x%x\n", mEOSMap);
   if (mEOSMap == 0x3 ) {	/* received both video and audio EOS */
      mbRun = false;
      mbIsFirstVideo = true;
      mbIsFirstAudio = true;
      mEOFMap = 0;
      mEOSMap = 0;
   }

   return ME_OK;
}

AM_ERR CTsMuxer::OnEvent (CPacket *packet)
{
   if (!mbRun) return ME_OK;

   mbEventFlag = true;
   mbEventNormalSync = false;
   mbIsFirstAudio = true;
   mbIsFirstVideo = true;
   mbNeedSplitted = false;

   mPcrBase = mH264Info.rate * mH264Info.div / mH264Info.mul;
   mPcrExt = 0;
   mVideoFrameCount = 0;

   mpDataWriter->OnEvent ();
   BuildPatPmt ();
   NOTICE ("Call OnEvent!");

   return ME_OK;
}

void CTsMuxer::OnRun ()
{
   if (mpDataWriter == NULL) {
      ERROR ("Data writer was not initialized.\n");
      return;
   }

   AM_ENSURE_OK_ (InitTs ());
   AM_ENSURE_OK_ (mpDataWriter->Init ());

   mbRun = true;
   CmdAck (ME_OK);

   while (mbRun) {
      CPacket *pPacket = NULL;
      CPacketQueueInputPin *pInPin = NULL;
      if  (!WaitInputBuffer (pInPin, pPacket))
         break; /* filter aborted, exit main loop */

      switch  (pPacket->GetType ()) {
      case CPacket::AM_PAYLOAD_TYPE_INFO:
         AM_ENSURE_OK_ (OnAVInfo (pPacket));
         break;

      case CPacket::AM_PAYLOAD_TYPE_DATA:
         AM_ENSURE_OK_ (OnAVBuffer (pPacket));
         break;

      case CPacket::AM_PAYLOAD_TYPE_EOS:
         AM_ENSURE_OK_ (OnEOS (pPacket));
         break;

      case CPacket::AM_PAYLOAD_TYPE_EVENT:
         AM_ENSURE_OK_ (OnEvent (pPacket));
         break;

      default:
         ERROR("Unknown packet type: %u!", pPacket->GetType());
         break;
      }

      pPacket->Release ();
   }

   AM_ENSURE_OK_ (mpDataWriter->Deinit ());
   PostEngineMsg (IEngine::MSG_EOS);
   INFO ("TS Muxer exit mainloop\n");
}

AM_ERR CTsMuxer::InitTs ()
{
   mPatInfo.totPrg = 1;
   mVideoStreamInfo.pid = mVideoPesBuf.pid;
   mAudioStreamInfo.pid = mAudioPesBuf.pid;

   mVideoStreamInfo.type = MPEG_SI_STREAM_TYPE_AVC_VIDEO;
   mVideoStreamInfo.descriptor_tag = 0x51;
   mVideoStreamInfo.pDescriptor =  (AM_U8*) &mH264Info;
   mVideoStreamInfo.descriptor_len = sizeof (mH264Info);

   mPrgInfo.pidPMT = mPmtBuf.pid;
   mPrgInfo.pidPCR = mVideoPesBuf.pid;
   mPrgInfo.prgNum = 1;
   mPatInfo.prgInfo = &mPrgInfo;

   mPmtInfo.totStream = 2;
   mPmtInfo.prgInfo = &mPrgInfo;
   mPmtInfo.stream[0] = &mVideoStreamInfo;
   mPmtInfo.stream[1] = &mAudioStreamInfo;
   mPmtInfo.descriptor_tag= 5;
   mPmtInfo.descriptor_len = 4;
   mPmtInfo.pDescriptor = mPmtDescriptor;
   mPmtInfo.pDescriptor[0] = 'H';//0x48;
   mPmtInfo.pDescriptor[1] = 'D';//0x44;
   mPmtInfo.pDescriptor[2] = 'M';//0x4d;
   mPmtInfo.pDescriptor[3] = 'V';//0x56;

   mLastVideoPTS = 0;
   mFileVideoFrameCount = 0;
   mVideoFrameCount = 0;

   return ME_OK;
}

inline void CTsMuxer::BuildAudioTsPacket(AM_U8 *data, AM_UINT size, AM_PTS pts)
{
  /* buffer the incoming audio data */
  AM_ASSERT (mAudioChunkBufAvail > size);
  memcpy (mpAudioChunkBufWrPtr, data, size);
  mpAudioChunkBufWrPtr += size;
  mAudioChunkBufAvail -= size;

  if (AM_LIKELY(mAudioChunkBufAvail < MAX_CODED_AUDIO_FRAME_SIZE)) {
    BuildAndFlushAudio(pts);
    mbIsFirstAudio = false;
  }
}

inline void CTsMuxer::BuildAndFlushAudio(AM_PTS pts)
{
  CTSMUXPES_PAYLOAD_INFO audio_payload_info = {0};
  audio_payload_info.firstFrame  = mbIsFirstAudio;
  audio_payload_info.withPCR     = 0;
  audio_payload_info.firstSlice  = 1;
  audio_payload_info.pPlayload   = mpAudioChunkBuf;
  audio_payload_info.payloadSize = mpAudioChunkBufWrPtr - mpAudioChunkBuf;
  audio_payload_info.pcr_base    = mPcrBase;
  audio_payload_info.pcr_ext     = mPcrExt;
  audio_payload_info.pts         = (pts - mPtsBase) & 0x1FFFFFFFF;
  audio_payload_info.dts         = audio_payload_info.pts;
  /*INFO("------------");*/

  while  (audio_payload_info.payloadSize > 0) {
    int write_len = mpTsBuilder->CreateTransportPacket (
        &mAudioStreamInfo, &audio_payload_info, mAudioPesBuf.buf);
    AM_ENSURE_OK_ (mpDataWriter->WriteData (mAudioPesBuf.buf,
                                            MPEG_TS_TP_PACKET_SIZE,
                                            AUDIO_STREAM));
    audio_payload_info.firstSlice = 0;
    audio_payload_info.pPlayload += write_len;
    audio_payload_info.payloadSize -= write_len;
  }

  mpAudioChunkBufWrPtr = mpAudioChunkBuf;
  mAudioChunkBufAvail = AUDIO_CHUNK_BUF_SIZE;
}

inline void CTsMuxer::BuildVideoTsPacket(AM_U8 *data, AM_UINT size, AM_PTS pts)
{
  CTSMUXPES_PAYLOAD_INFO video_payload_info = {0};
  video_payload_info.firstFrame  = mbIsFirstVideo;
  video_payload_info.withPCR     = 1;
  video_payload_info.firstSlice  = 1;
  video_payload_info.pPlayload   = data;
  video_payload_info.payloadSize = size;
  video_payload_info.pcr_base    = mPcrBase;
  video_payload_info.pcr_ext     = mPcrExt;
  video_payload_info.pts         = (pts - mPtsBase) & 0x1FFFFFFFF;
  video_payload_info.dts         = ((mH264Info.M == 1) ?
      video_payload_info.pts : mPcrBase);

  while  (video_payload_info.payloadSize > 0) {
    int write_len = mpTsBuilder->CreateTransportPacket (&mVideoStreamInfo,
                                                        &video_payload_info,
                                                        mVideoPesBuf.buf);
    AM_ENSURE_OK_ (mpDataWriter->WriteData (mVideoPesBuf.buf,
                                            MPEG_TS_TP_PACKET_SIZE,
                                            VIDEO_STREAM));
    video_payload_info.withPCR      = 0;
    video_payload_info.firstSlice   = 0;
    video_payload_info.pPlayload   += write_len;
    video_payload_info.payloadSize -= write_len;
  }
}

inline void CTsMuxer::BuildPatPmt()
{
  mpTsBuilder->CreatePAT (&mPatInfo, mPatBuf.buf);
  mpTsBuilder->CreatePMT (&mPmtInfo, mPmtBuf.buf);

  AM_ENSURE_OK_ (mpDataWriter->WriteData (mPatBuf.buf,
                                          MPEG_TS_TP_PACKET_SIZE,
                                          VIDEO_STREAM));
  AM_ENSURE_OK_ (mpDataWriter->WriteData (mPmtBuf.buf,
                                          MPEG_TS_TP_PACKET_SIZE,
                                          VIDEO_STREAM));
}

inline void CTsMuxer::UpdatePatPmt()
{
  mpTsBuilder->UpdatePSIcc (mPatBuf.buf);
  mpTsBuilder->UpdatePSIcc (mPmtBuf.buf);
  AM_ENSURE_OK_ (mpDataWriter->WriteData (mPatBuf.buf,
                                          MPEG_TS_TP_PACKET_SIZE,
                                          VIDEO_STREAM));
  AM_ENSURE_OK_ (mpDataWriter->WriteData (mPmtBuf.buf,
                                          MPEG_TS_TP_PACKET_SIZE,
                                          VIDEO_STREAM));
}

inline AM_ERR CTsMuxer::WriteData (AM_U8 * pData, int len)
{
   WARN("Not implemented, should not be called!");
   return ME_OK;
}

inline AM_ERR CTsMuxer::PcrSync (AM_PTS pts)
{
   mPcrBase = pts - mH264Info.rate * mH264Info.div / mH264Info.mul;
   mPcrExt = 0;
   INFO ("PcrSync %lld, pts %lld\n", mPcrBase, pts);
   return ME_OK;
}

inline AM_ERR CTsMuxer::PcrIncrement (AM_PTS curVideoPTS)
{
   // mPcrBase = (mPcrBase + mPcrIncBase) %  (1LL << 33);
   mPcrBase = (mPcrBase + curVideoPTS - mLastVideoPTS) %  (1LL << 33);
   mPcrBase += (mPcrExt + mPcrIncExt) / 300;
   mPcrExt = (mPcrExt + mPcrIncExt) % 300;
   return ME_OK;
}

inline AM_ERR CTsMuxer::PcrCalcPktDuration (AM_U32 rate, AM_U32 scale)
{
   AM_PTS inc= 27000000LL * rate / scale;
   mPcrIncBase= inc / 300;
   mPcrIncExt = inc % 300;
   INFO ("PcrCalcPktDuration (%lld, %d), %d %d\n",
         mPcrIncBase, mPcrIncExt, rate, scale);
   return ME_OK;
}
