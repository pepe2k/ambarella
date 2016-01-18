/*
 * jpeg_muxer.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 11/12/2012
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
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
#include "file_sink.h"
#include "jpeg_file_writer.h"
#include "jpeg_muxer.h"

IPacketFilter *CreateJpegMuxer (IEngine *pEngine)
{
   return (IPacketFilter *)CJpegMuxer::Create (pEngine);
}

//-----------------------------------------------------------------------
//
// CJpegMuxer
//
//-----------------------------------------------------------------------
CJpegMuxer *CJpegMuxer::Create (IEngine *pEngine, bool RTPriority, int priority)
{
   CJpegMuxer *result = new CJpegMuxer (pEngine);
   if (result != NULL && result->Construct(RTPriority, priority) != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

CJpegMuxer::CJpegMuxer (IEngine *pEngine) :
   inherited (pEngine, "jpeg_muxer"),
   mbRun (false),
   mEOSMap (0),
   mMaxFileAmount (0),
   mpMediaInput (NULL),
   mpDataWriter (NULL),
   mVideoFrameCount (0)
{}

AM_ERR CJpegMuxer::Construct (bool RTPriority, int priority)
{
   if (inherited::Construct (RTPriority, priority) != ME_OK) {
      ERROR ("Failed to construct parent.\n");
      return ME_ERROR;
   }

   if ((mpMediaInput = CJpegMuxerInput::Create (this)) == NULL) {
      ERROR ("Failed to create output pin for jpeg muxer.\n");
      return ME_ERROR;
   }

   mpMediaInput->Enable (true);
   return ME_OK;
}

CJpegMuxer::~CJpegMuxer ()
{
   AM_DELETE (mpMediaInput);
   AM_DELETE (mpDataWriter);
}

void* CJpegMuxer::GetInterface (AM_REFIID refiid)
{
   if (refiid == IID_IMediaMuxer) {
      return (IMediaMuxer *)this;
   }
   return inherited::GetInterface (refiid);
}

void CJpegMuxer::GetInfo (INFO& info)
{
   info.nInput = 1;
   info.nOutput = 0;
   info.pName = "JpegMuxer";
}

IPacketPin* CJpegMuxer::GetInputPin (AM_UINT index)
{
   if (index == 0) {
      return mpMediaInput;
   }

   return NULL;
}

AM_ERR CJpegMuxer::Stop ()
{
   if (mbRun) {
      mbRun = false;
      inherited::Stop ();
   }

   return ME_OK;
}

AM_ERR CJpegMuxer::SetMediaSink (AmSinkType sinkType, const char *pDestStr)
{
   AM_DELETE (mpDataWriter);

   if (sinkType == AM_SINK_TYPE_JPEG) {
      mpDataWriter = CJpegFileWriter::Create ();
   } else {
      ERROR ("No such Jpeg writer adapter!");
      return ME_ERROR;
   }

   if (mpDataWriter == NULL) {
      ERROR ("Failed to create jpeg writer adapter!");
      return ME_ERROR;
   }

   if (mpDataWriter->SetMaxFileAmount (mMaxFileAmount) != ME_OK) {
      ERROR ("Failed to set maximum file amount for jpeg writer adapter!");
      return ME_ERROR;
   }

   if (mpDataWriter->SetMediaSink (pDestStr) != ME_OK) {
      ERROR ("Failed to set media sink for jpeg writer adapter!");
      return ME_ERROR;
   }

   return ME_OK;
}

AM_ERR CJpegMuxer::SetMediaSourceMask (AM_UINT mediaSourceMask)
{
   return ME_OK;
}

AM_ERR CJpegMuxer::SetSplitDuration (AM_U64 durationIn90KBase)
{
   return ME_OK;
}

AM_ERR CJpegMuxer::SetMaxFileAmount (AM_UINT maxFileAmount)
{
   if (maxFileAmount == 0) {
      ERROR ("Maximum file amount should not be zero!");
      return ME_ERROR;
   }

   mMaxFileAmount = maxFileAmount;
   return ME_OK;
}

AM_ERR CJpegMuxer::OnAVInfo (CPacket *packet)
{
   if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
      /* Video info */
      AM_VIDEO_INFO *pInfo =  (AM_VIDEO_INFO *)packet->GetDataPtr ();
      NOTICE ("JPEG Muxer receive H264 INFO: "
            "size %dx%d, M %d, N %d, rate %d, scale %d\n",
            pInfo->width, pInfo->height, pInfo->M,
            pInfo->N, pInfo->rate, pInfo->scale);
   } else {
      NOTICE ("Currently, jpeg muxer just display video info.\n");
   }

   return ME_OK;
}

AM_ERR CJpegMuxer::OnAVBuffer (CPacket *packet)
{
   if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
      /* Video Data */
      mpDataWriter->WriteData (packet->GetDataPtr (),
            packet->GetDataSize ());
   } else {
     DEBUG ("Currently, jpeg muxer just use video stream.\n");
   }

   return ME_OK;
}

AM_ERR CJpegMuxer::OnEOS (CPacket *packet)
{
   if (!mbRun) {
      return ME_OK;
   }

   if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
      mEOSMap |= 1 << 1;
   } else {
      DEBUG ("Currently, jpeg muxer just use video stream.\n");
   }

   DEBUG ("EOS map 0x%x\n", mEOSMap);
   if (mEOSMap == 0x2 ) {	/* received video EOS */
      mbRun = false;
      mEOSMap = 0;
   }

   return ME_OK;
}

void CJpegMuxer::OnRun ()
{
   if (mpDataWriter == NULL) {
      ERROR ("Data writer was not initialized.\n");
      return;
   }

   AM_ENSURE_OK_ (mpDataWriter->Init ());

   mbRun = true;
   CmdAck (ME_OK);

   while (mbRun) {
      CPacket *pPacket;
      CPacketQueueInputPin *pInPin;
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

      default:
         ERROR("Unknown packet type: %u!", pPacket->GetType());
         break;
      }

      pPacket->Release ();
   }

   AM_ENSURE_OK_ (mpDataWriter->Deinit ());
   PostEngineMsg (IEngine::MSG_EOS);
   INFO ("JPEG Muxer exit mainloop\n");
}

//-----------------------------------------------------------------------
//
// CJpegMuxerInput
//
//-----------------------------------------------------------------------
CJpegMuxerInput* CJpegMuxerInput::Create (CPacketFilter *pFilter)
{
   CJpegMuxerInput* result = new CJpegMuxerInput (pFilter);
   if (result && result->Construct () != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

AM_ERR CJpegMuxerInput::Construct ()
{
   AM_ERR err = inherited::Construct (((CJpegMuxer*)mpFilter)->MsgQ ());
   if  (err != ME_OK)
      return err;
   return ME_OK;
}
