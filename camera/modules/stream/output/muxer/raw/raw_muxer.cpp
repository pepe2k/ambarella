/*
 * raw_muxer.cpp
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
#include "transfer/am_data_transfer.h"
#include "transfer/am_transfer_server.h"
#include "file_sink.h"
#include "raw_muxer.h"

IPacketFilter *CreateRawMuxer (IEngine *pEngine)
{
   return (IPacketFilter *)CRawMuxer::Create (pEngine);
}

//-----------------------------------------------------------------------
//
// CRawMuxer
//
//-----------------------------------------------------------------------
CRawMuxer *CRawMuxer::Create (IEngine *pEngine, bool RTPriority, int priority)
{
   CRawMuxer *result = new CRawMuxer (pEngine);
   if (result != NULL && result->Construct(RTPriority, priority) != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

CRawMuxer::CRawMuxer (IEngine *pEngine) :
   inherited (pEngine, "raw_muxer"),
   mbRun (false),
   mVideoTotalEosMap (0),
   mVideoReachEosMap (0),
   mAudioTotalEosMap (0),
   mAudioReachEosMap (0),
   mAudioType (AM_TRANSFER_PACKET_TYPE_NULL),
   mpMediaInput (NULL),
   mpPacket (NULL),
   mpTransferServer (NULL),
   mAudioIndex (0),
   mVideoIndex (0)
{
   int i = 0;

   for (; i < AM_STREAM_NUMBER_MAX; i++) {
      mVideoType[i] = AM_TRANSFER_PACKET_TYPE_NULL;
   }
}

AM_ERR CRawMuxer::Construct (bool RTPriority, int priority)
{
   if (inherited::Construct (RTPriority, priority) != ME_OK) {
      ERROR ("Failed to construct parent.\n");
      return ME_ERROR;
   }

   if ((mpMediaInput = CRawMuxerInput::Create (this)) == NULL) {
      ERROR ("Failed to create output pin for raw muxer.\n");
      return ME_ERROR;
   }

   if ((mpPacket = new AmTransferPacket) == NULL) {
      ERROR ("Failed to create a packet!\n");
      return ME_ERROR;
   }

   if ((mpTransferServer = AmTransferServer::Create ()) == NULL) {
      ERROR ("Failed to create packet transfer!\n");
      return ME_ERROR;
   }

   if (mpTransferServer->Start () < 0) {
      ERROR ("Failed to start transfer server");
      return ME_ERROR;
   }

   mpMediaInput->Enable (true);
   return ME_OK;
}

CRawMuxer::~CRawMuxer ()
{
   AM_DELETE (mpMediaInput);
   if (mpTransferServer->Stop () < 0) {
      NOTICE ("Failed to stop transfer server!");
   }

   delete mpPacket;
   delete mpTransferServer;
}

void* CRawMuxer::GetInterface (AM_REFIID refiid)
{
   if (refiid == IID_IMediaMuxer) {
      return (IMediaMuxer *)this;
   }
   return inherited::GetInterface (refiid);
}

void CRawMuxer::GetInfo (INFO& info)
{
   info.nInput = 1;
   info.nOutput = 0;
   info.pName = "RawMuxer";
}

IPacketPin* CRawMuxer::GetInputPin (AM_UINT index)
{
   if (index == 0) {
      return mpMediaInput;
   }

   return NULL;
}

AM_ERR CRawMuxer::Stop ()
{
   if (mbRun) {
      mbRun = false;
      inherited::Stop ();
   }


   return ME_OK;
}

AM_ERR CRawMuxer::SetMediaSink (AmSinkType sinkType, const char *pDestStr)
{
   return ME_OK;
}

AM_ERR CRawMuxer::SetMediaSourceMask (AM_UINT mediaSourceMask)
{
   return ME_OK;
}

AM_ERR CRawMuxer::SetSplitDuration (AM_U64 durationIn90KBase)
{
   return ME_OK;
}

AM_ERR CRawMuxer::SetMaxFileAmount (AM_UINT maxFileAmount)
{
   return ME_OK;
}

AM_ERR CRawMuxer::OnAVInfo (CPacket *packet)
{
   if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
      /* Video info */
      AM_VIDEO_INFO *pInfo =  (AM_VIDEO_INFO *)packet->GetDataPtr ();
      NOTICE ("RAW Muxer receive H264 INFO: "
            "size %dx%d, M %d, N %d, rate %d, scale %d\n",
            pInfo->width, pInfo->height, pInfo->M,
            pInfo->N, pInfo->rate, pInfo->scale);
      if (pInfo->type == 1) {
         mVideoType[packet->GetStreamId ()] = AM_TRANSFER_PACKET_TYPE_H264;
      } else if (pInfo->type == 2) {
         mVideoType[packet->GetStreamId ()] = AM_TRANSFER_PACKET_TYPE_MJPEG;
      }

      /*
       * Special process for raw muxer. Unlike ts, mp4 and jpeg muxer,
       * we have merge several raw streams into one raw muxer,so we
       * need to record how many raw streams. When recording pipeline
       * sends EOS to each stream, raw muxer has to wait corresponding
       * amount of EOS and then exit.
       */
      mVideoTotalEosMap |= (1 << packet->GetStreamId ());
   } else {
      AM_AUDIO_INFO *pInfo = (AM_AUDIO_INFO *)packet->GetDataPtr ();
      NOTICE ("RAW Muxer receives Audio Info: freq %d, chunk %d, channel %d\n",
            pInfo->sampleRate, pInfo->chunkSize, pInfo->channels);

      if (pInfo->format == MF_PCM) {
         mAudioType = AM_TRANSFER_PACKET_TYPE_PCM;
      } else if (pInfo->format == MF_G711) {
         mAudioType = AM_TRANSFER_PACKET_TYPE_G711;
      } else if (pInfo->format == MF_G726_40) {
         mAudioType = AM_TRANSFER_PACKET_TYPE_G726_40;
      } else if (pInfo->format == MF_G726_32) {
         mAudioType = AM_TRANSFER_PACKET_TYPE_G726_32;
      } else if (pInfo->format == MF_G726_24) {
         mAudioType = AM_TRANSFER_PACKET_TYPE_G726_24;
      } else if (pInfo->format == MF_G726_16) {
         mAudioType = AM_TRANSFER_PACKET_TYPE_G726_16;
      } else if (pInfo->format == MF_AAC) {
         mAudioType = AM_TRANSFER_PACKET_TYPE_AAC;
      } else if (pInfo->format == MF_OPUS) {
         mAudioType = AM_TRANSFER_PACKET_TYPE_OPUS;
      } else if (pInfo->format == MF_BPCM) {
         mAudioType = AM_TRANSFER_PACKET_TYPE_BPCM;
      } else {
         ERROR ("Unknow audio!\n");
         return ME_ERROR;
      }

      mAudioTotalEosMap |= (1 << packet->GetStreamId ());
   }

   return ME_OK;
}

AM_ERR CRawMuxer::OnAVBuffer (CPacket *packet)
{
   CPacket::AmPayloadAttr attr = packet->GetAttr ();

   if (attr == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
      memset (mpPacket, 0, sizeof (AmTransferPacket));
      mpPacket->header.dataType  = mAudioType;
      mpPacket->header.frameType = (AmTransferFrameType)packet->GetFrameType ();
      mpPacket->header.streamId  = packet->GetStreamId ();
      mpPacket->header.dataLen   = packet->GetDataSize ();
      mpPacket->header.dataPTS   = packet->GetPTS ();
      mpPacket->header.seqNum    = ++mAudioIndex;
      memcpy (mpPacket->data, packet->GetDataPtr (), packet->GetDataSize ());

      if (mpTransferServer->SendPacket (mpPacket) < 0) {
         NOTICE ("Failed to send packet to client!\n");
         return ME_ERROR;
      }
   } else if (attr == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
      memset (mpPacket, 0, sizeof (AmTransferPacket));
      mpPacket->header.dataType  = mVideoType[packet->GetStreamId ()];
      mpPacket->header.frameType = (AmTransferFrameType)packet->GetFrameType ();
      mpPacket->header.streamId  = packet->GetStreamId ();
      mpPacket->header.dataLen   = packet->GetDataSize ();
      mpPacket->header.dataPTS   = packet->GetPTS ();
      mpPacket->header.seqNum    = ++mVideoIndex;
      memcpy (mpPacket->data, packet->GetDataPtr (), packet->GetDataSize ());

      if (mpTransferServer->SendPacket (mpPacket) < 0) {
         NOTICE ("Failed to send packet to client!\n");
         return ME_ERROR;
      }
   }

   return ME_OK;
}

AM_ERR CRawMuxer::OnEOS (CPacket *packet)
{
   if (!mbRun) {
      return ME_OK;
   }

   if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_AUDIO) {
      mAudioReachEosMap |= (1 << packet->GetStreamId ());
   } else if (packet->GetAttr () == CPacket::AM_PAYLOAD_ATTR_VIDEO) {
      mVideoReachEosMap |= (1 << packet->GetStreamId ());
   } else {
      NOTICE ("Currently, raw muxer just supports audio and video stream");
      return ME_ERROR;
   }

   if ((mAudioReachEosMap == mAudioTotalEosMap) &&
       (mVideoReachEosMap == mVideoTotalEosMap)) {
      mbRun = false;
      mAudioReachEosMap = 0;
      mAudioTotalEosMap = 0;
      mVideoReachEosMap = 0;
      mVideoTotalEosMap = 0;
   }

   return ME_OK;
}

void CRawMuxer::OnRun ()
{

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

   PostEngineMsg (IEngine::MSG_EOS);
   INFO ("Raw Muxer exit mainloop\n");
}

//-----------------------------------------------------------------------
//
// CRawMuxerInput
//
//-----------------------------------------------------------------------
CRawMuxerInput* CRawMuxerInput::Create (CPacketFilter *pFilter)
{
   CRawMuxerInput* result = new CRawMuxerInput (pFilter);
   if (result && result->Construct () != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

AM_ERR CRawMuxerInput::Construct ()
{
   AM_ERR err = inherited::Construct (((CRawMuxer*)mpFilter)->MsgQ ());
   if  (err != ME_OK)
      return err;
   return ME_OK;
}
