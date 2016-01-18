/*
 * build_ts.cpp
 *
 * History:
 *	2011/4/17 - [Yi Zhu] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <sys/ioctl.h> //for ioctl
#include <fcntl.h>     //for open O_* flags
#include <unistd.h>    //for read/write/lseek
#include <stdlib.h>    //for malloc/free
#include <string.h>    //for strlen/memset
#include <stdio.h>     //for printf
#include <time.h>

#include "am_types.h"
#include "am_media_info.h"
#include "ts_builder.h"

CTsBuilder::CTsBuilder()
{
  ver.pat = 0;
  ver.pmt = 0;
  memset(&mAudioInfo, 0, sizeof(mAudioInfo));
  memset(&mVideoInfo, 0, sizeof(mVideoInfo));
}

void CTsBuilder::Delete()
{
   delete this;
}

AM_ERR CTsBuilder::CreatePAT(CTSMUXPSI_PAT_INFO *pPatInfo, AM_U8 *pBufPAT)
{
   MPEG_TS_TP_HEADER *tsHdr = (MPEG_TS_TP_HEADER *)pBufPAT;
   /*TS hdr size + PSI pointer field*/
   PAT_HDR *patHeader       = (PAT_HDR *)(pBufPAT + 5);
   AM_U8 *pPatContent       = (AM_U8 *)(((AM_U8 *)patHeader) + PAT_HDR_SIZE);
   int crc32                = 0;

   // TS HEADER
   tsHdr->sync_byte = MPEG_TS_TP_SYNC_BYTE;
   tsHdr->transport_error_indicator    =
      MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
   tsHdr->payload_unit_start_indicator =
      MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START;
   tsHdr->transport_priority           =
      MPEG_TS_TRANSPORT_PRIORITY_PRIORITY;
   tsHdr->transport_scrambling_control =
      MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
   tsHdr->adaptation_field_control     =
      MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;
   tsHdr->continuity_counter           = 0;
   MPEG_TS_TP_HEADER_PID_SET(tsHdr, MPEG_TS_PAT_PID);

   // Set PSI pointer field
   pBufPAT[4] = 0x00;

   // PAT Header
   patHeader->table_id                 = 0x00;
   patHeader->section_syntax_indicator = 1;
   patHeader->b0                       = 0;
   patHeader->reserved0                = 0x3;
   patHeader->transport_stream_id_l    = 0x00;
   patHeader->transport_stream_id_h    = 0x00;
   patHeader->reserved1                = 0x3;
   patHeader->version_number           = ver.pat;
   patHeader->current_next_indicator   = 1;
   patHeader->section_number           = 0x0;
   patHeader->last_section_number      = 0x0;
   patHeader->section_length0to7       = 0; //Update later
   patHeader->section_length8to11      = 0; //Update later

   // add informations for all programs	(only one for now)
   pPatContent[0] = (pPatInfo->prgInfo->prgNum >> 8) & 0xff;
   pPatContent[1] = pPatInfo->prgInfo->prgNum & 0xff;
   pPatContent[2] = 0xE0 | ((pPatInfo->prgInfo->pidPMT & 0x1fff) >> 8);
   pPatContent[3] = pPatInfo->prgInfo->pidPMT & 0xff;
   pPatContent += 4;

   // update patHdr.section_length
   AM_U16 section_len = pPatContent + 4 - (AM_U8 *)patHeader - 3 ;
   patHeader->section_length8to11 = (section_len & 0x0fff) >> 8;
   patHeader->section_length0to7  = (section_len & 0x00ff);

   // Calc CRC32
   crc32 = Cal_crc32((AM_U8*)patHeader, (int)(pPatContent-(AM_U8 *)patHeader));
   pPatContent[0] = (crc32>>24)&0xff;
   pPatContent[1] = (crc32>>16)&0xff;
   pPatContent[2] = (crc32>>8)&0xff;
   pPatContent[3] = crc32&0xff;

   // Stuff rest of the packet
   memset(pPatContent + 4, /*Stuffing Btypes*/
         0xff,
         MPEG_TS_TP_PACKET_SIZE - (pPatContent + 4 - pBufPAT));

   return ME_OK;
}

AM_ERR CTsBuilder::UpdatePSIcc(AM_U8 * pBufTS)
{
   ((MPEG_TS_TP_HEADER *)pBufTS)->continuity_counter ++;
   return ME_OK;
}

AM_ERR CTsBuilder::CreatePMT(CTSMUXPSI_PMT_INFO *pPmtInfo, AM_U8 *pBufPMT)
{
   MPEG_TS_TP_HEADER *tsHdr = (MPEG_TS_TP_HEADER *)pBufPMT;
   PMT_HDR       *pmtHeader = (PMT_HDR *)(pBufPMT + 5);
   AM_U8       *pPmtContent = (AM_U8 *)(((AM_U8 *)pmtHeader) + PMT_HDR_SIZE);
   int crc32;

   // TS HEADER
   tsHdr->sync_byte = MPEG_TS_TP_SYNC_BYTE;
   tsHdr->transport_error_indicator =
      MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
   tsHdr->payload_unit_start_indicator =
      MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START;
   tsHdr->transport_priority =
      MPEG_TS_TRANSPORT_PRIORITY_PRIORITY;

   MPEG_TS_TP_HEADER_PID_SET(tsHdr, pPmtInfo->prgInfo->pidPMT);

   tsHdr->transport_scrambling_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
   tsHdr->adaptation_field_control = MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;
   tsHdr->continuity_counter = 0;

   // Set PSI poiter field
   pBufPMT[4] = 0x00;

   // PMT HEADER
   pmtHeader->table_id               = 0x02;
   pmtHeader->section_syntax_indicator = 1;
   pmtHeader->b0                     = 0;
   pmtHeader->reserved0              = 0x3;
   pmtHeader->section_length0to7     = 0; //update later
   pmtHeader->section_length8to11    = 0; //update later
   pmtHeader->program_number_h       = (pPmtInfo->prgInfo->prgNum >> 8) & 0xff;
   pmtHeader->program_number_l       = pPmtInfo->prgInfo->prgNum & 0xff;
   pmtHeader->reserved1              = 0x3;
   pmtHeader->version_number         = ver.pmt;
   pmtHeader->current_next_indicator = 1;
   pmtHeader->section_number         = 0x0;
   pmtHeader->last_section_number    = 0x0;
   pmtHeader->reserved2              = 0x7;
   pmtHeader->PCR_PID8to12   = (pPmtInfo->prgInfo->pidPCR >> 8) & 0x1f;
   pmtHeader->PCR_PID0to7    = pPmtInfo->prgInfo->pidPCR & 0xff;
   pmtHeader->reserved3      = 0xf;
   if (pPmtInfo->descriptor_len == 0) {
      pmtHeader->program_info_length0to7 = 0;
      pmtHeader->program_info_length8to11 = 0;
   } else {
      pmtHeader->program_info_length8to11 =
         ((2 + pPmtInfo->descriptor_len) >> 8) & 0x0f;
      pmtHeader->program_info_length0to7 =
         ((2 + pPmtInfo->descriptor_len) & 0xff);
   }

   if (pPmtInfo->descriptor_len > 0) {
      pPmtContent[0] = pPmtInfo->descriptor_tag;
      pPmtContent[1] = pPmtInfo->descriptor_len;
      memcpy(&pPmtContent[2], pPmtInfo->pDescriptor, pPmtInfo->descriptor_len);
      pPmtContent += (2 + pPmtInfo->descriptor_len);
   }

   // Add all stream elements
   for(AM_U16 strNum = 0; strNum < pPmtInfo->totStream; ++ strNum) {
      CTSMUXPSI_STREAM_INFO * pStr = pPmtInfo->stream[strNum];
      PMT_ELEMENT *pPmtElement = (PMT_ELEMENT *)pPmtContent;
      pPmtElement->stream_type = pStr->type;
      pPmtElement->reserved0   = 0x7; // 3 bits
      pPmtElement->elementary_PID8to12 = ((pStr->pid & 0x1fff) >> 8);
      pPmtElement->elementary_PID0to7  = (pStr->pid & 0xff);
      pPmtElement->reserved1   = 0xf; // 4 bits
      pPmtElement->ES_info_length_h = (((pStr->descriptor_len + 2)>>8) & 0x0f);
      pPmtElement->ES_info_length_l = (pStr->descriptor_len + 2) & 0xff;

      pPmtContent += PMT_ELEMENT_SIZE;
      pPmtContent[0] = pStr->descriptor_tag;	//descriptor_tag
      pPmtContent[1] = pStr->descriptor_len;	//descriptor_length

      // printf("pPmtContent2 %d, descriptor_len %d\n",
      //        pPmtContent - pBufPMT, pStr->descriptor_len);
      if (pStr->descriptor_len > 0) {
         memcpy(&pPmtContent[2], pStr->pDescriptor, pStr->descriptor_len);
         //printf("pPmtContent3 %d, 0x%x 0x%x, 0x%x\n",
         //       pPmtContent - pBufPMT, ((AM_U8*)pStr->pDescriptor)[0],
         //       ((AM_U8*)pStr->pDescriptor)[1],
         //       ((AM_U8*)pStr->pDescriptor)[2]);
      }
      pPmtContent += (2 + pStr->descriptor_len);
   }

   // update pmtHdr.section_length
   AM_U16 section_len = pPmtContent + 4 - ((AM_U8 *)pmtHeader + 3);
   pmtHeader->section_length8to11 = (section_len >> 8) & 0x0f;
   pmtHeader->section_length0to7  = (section_len & 0xff);

   // Calc CRC32
   crc32 = Cal_crc32((AM_U8*)pmtHeader, (int)(pPmtContent-(AM_U8*)pmtHeader));
   pPmtContent[0] = (crc32>>24)&0xff;
   pPmtContent[1] = (crc32>>16)&0xff;
   pPmtContent[2] = (crc32>>8)&0xff;
   pPmtContent[3] = crc32&0xff;

   // printf("pPmtContent4 %d, 0x%x, 0x%x, 0x%x, 0x%x\n",
   //        pPmtContent - pBufPMT, pPmtContent[0], pPmtContent[1],
   //        pPmtContent[2], pPmtContent[3]);

   // Stuff rest of the packet
   memset((pPmtContent + 4), //Stuffing bytes
          0xff,
          (MPEG_TS_TP_PACKET_SIZE - (((AM_U8*)pPmtContent + 4) - pBufPMT)));

   return ME_OK;
}

AM_ERR CTsBuilder::CreatePCR(AM_U16 pidPCR, AM_U8 *pBufPCR)
{
   MPEG_TS_TP_HEADER *tsHeader = (MPEG_TS_TP_HEADER *)pBufPCR;
   AM_U8 *adaptationField      = (AM_U8*)(pBufPCR +
         MPEG_TS_TP_PACKET_HEADER_SIZE);

   // TS HEADER
   tsHeader->sync_byte = MPEG_TS_TP_SYNC_BYTE;
   tsHeader->transport_error_indicator =
      MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
   tsHeader->payload_unit_start_indicator =
      MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_NORMAL;
   tsHeader->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY;
   MPEG_TS_TP_HEADER_PID_SET(tsHeader, pidPCR);
   tsHeader->transport_scrambling_control =
      MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
   tsHeader->adaptation_field_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
   tsHeader->continuity_counter = 0;

   // Set adaptation_field
   adaptationField[0] = 183;  /* adapt_length : 1+6+176 bytes */
   adaptationField[1] = 0x10; /* discont - adapt_ext_flag : PCR on */

   memset(&adaptationField[2], 0xcc, 6);   /* dummy PCR */
   memset(&adaptationField[8], 0xff, 176); /* stuffing_byte */

   return ME_OK;
}

AM_ERR CTsBuilder::CreateNULL(AM_U8 * pBuf)
{
   MPEG_TS_TP_HEADER *tsHeader = (MPEG_TS_TP_HEADER *)pBuf;

   // TS HEADER
   tsHeader->sync_byte = MPEG_TS_TP_SYNC_BYTE;
   tsHeader->transport_error_indicator =
      MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
   tsHeader->payload_unit_start_indicator =
      MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_NORMAL;
   tsHeader->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY;
   MPEG_TS_TP_HEADER_PID_SET(tsHeader, MPEG_TS_NULL_PID);
   tsHeader->transport_scrambling_control =
      MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;

   // No adaptation field payload only.
   tsHeader->adaptation_field_control = MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;
   tsHeader->continuity_counter = 0;

   // Stuff bytes
   memset((pBuf + MPEG_TS_TP_PACKET_HEADER_SIZE),
         0xff,
         MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE);

   return ME_OK;
}

int  CTsBuilder::CreateTransportPacket(CTSMUXPSI_STREAM_INFO  *pStr,
                                       CTSMUXPES_PAYLOAD_INFO *pFd,
                                       AM_U8                  *pBufPES)
{
   MPEG_TS_TP_HEADER *tsHeader = (MPEG_TS_TP_HEADER *)pBufPES;
   AM_U8 *pesPacket   = (AM_U8 *)(pBufPES + MPEG_TS_TP_PACKET_HEADER_SIZE);
   AM_U8 *tmpWritePtr = NULL;

   /* pre-calculate pesHeaderDataLength so that
    * Adapt filed can decide if stuffing is required
    */
   AM_UINT pesPtsDtsFlag = MPEG_TS_PTS_DTS_NO_PTSDTS;
   AM_UINT pesHeaderDataLength = 0; /* PES optional field data length */
   if (pFd->firstSlice) { /*the PES packet will start in the first slice*/
      /* check PTS DTS delta */
      if ((pFd->pts <= (pFd->dts + 1)) && (pFd->dts <= (pFd->pts + 1))) {
         pesPtsDtsFlag = MPEG_TS_PTS_DTS_PTS_ONLY;
         pesHeaderDataLength = PTS_FIELD_SIZE;
      } else {
         pesPtsDtsFlag = MPEG_TS_PTS_DTS_PTSDTS_BOTH;
         pesHeaderDataLength = PTS_FIELD_SIZE + DTS_FIELD_SIZE;
      }
   }

   // TS header
   tsHeader->sync_byte = MPEG_TS_TP_SYNC_BYTE;
   tsHeader->transport_error_indicator =
       MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
   tsHeader->payload_unit_start_indicator = /*a PES starts in the current pkt*/
       (pFd->firstSlice ? MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START :
                          MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_NORMAL);
   tsHeader->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY;
   tsHeader->transport_scrambling_control =
       MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
   tsHeader->continuity_counter = ((pFd->firstFrame && pFd->firstSlice) ?
       0 : ((pBufPES[3] + 1) & 0x0f)); /* increase the counter */
   MPEG_TS_TP_HEADER_PID_SET(tsHeader, pStr->pid);

   /* Adaptation field */
   /* For Transport Stream packets carrying PES packets,
    * stuffing is needed when there is insufficient
    * PES packet data to completely fill the Transport
    * Stream packet payload bytes */

   MPEG_TS_TP_ADAPTATION_FIELD_HEADER *adaptHdr =
       (MPEG_TS_TP_ADAPTATION_FIELD_HEADER *)(pBufPES +
                                              MPEG_TS_TP_PACKET_HEADER_SIZE);
   tmpWritePtr = (AM_U8 *)adaptHdr; /* point to adapt header */

   // fill the common fields
   adaptHdr->adaptation_field_length         = 0;
   adaptHdr->adaptation_field_extension_flag =
       MPEG_TS_ADAPTATION_FIELD_EXTENSION_NOT_PRESENT;
   adaptHdr->transport_private_data_flag     =
       MPEG_TS_TRANSPORT_PRIVATE_DATA_NOT_PRESENT;
   adaptHdr->splicing_point_flag             =
       MPEG_TS_SPLICING_POINT_FLAG_NOT_PRESENT;
   adaptHdr->opcr_flag                       = MPEG_TS_OPCR_FLAG_NOT_PRESENT;
   adaptHdr->pcr_flag                        = MPEG_TS_PCR_FLAG_NOT_PRESENT;
   adaptHdr->elementary_stream_priority_indicator =
       MPEG_TS_ELEMENTARY_STREAM_PRIORITY_NO_PRIORITY;
   adaptHdr->random_access_indicator              =
       MPEG_TS_RANDOM_ACCESS_INDICATOR_NOT_PRESENT;
   adaptHdr->discontinuity_indicator = /* assume no discontinuity occur */
       MPEG_TS_DISCONTINUITY_INDICATOR_NO_DISCONTINUTY;

   AM_INT initialAdaptFiledSize = 0;
   if (pFd->withPCR) {
     //# of bytes following adaptation_field_length (1 + 6 pcr size)
     adaptHdr->adaptation_field_length = 1 + PCR_FIELD_SIZE;
     adaptHdr->pcr_flag                = MPEG_TS_PCR_FLAG_PRESENT;
     adaptHdr->random_access_indicator =
         MPEG_TS_RANDOM_ACCESS_INDICATOR_PRESENT;

     /*skip adapt hdr and point to optional field (PCR field)*/
     tmpWritePtr   += ADAPT_HEADER_LEN;
     tmpWritePtr[0] = pFd->pcr_base >> 25;
     tmpWritePtr[1] = (pFd->pcr_base & 0x1fe0000) >> 17;
     tmpWritePtr[2] = (pFd->pcr_base & 0x1fe00) >> 9;
     tmpWritePtr[3] = (pFd->pcr_base & 0x1fe) >> 1;
     tmpWritePtr[4] = ((pFd->pcr_base & 0x1)<<7) | (0x7e) | (pFd->pcr_ext>>8);
     tmpWritePtr[5] = pFd->pcr_ext & 0xff;
     /*point to the start of potential stuffing area*/
     tmpWritePtr += PCR_FIELD_SIZE;
     initialAdaptFiledSize = adaptHdr->adaptation_field_length + 1;
   }

   // check if stuffing is required and calculate stuff size
   AM_UINT PESHdrSize = 0;
   AM_INT stuffSize = 0;

   if (pFd->firstSlice) {
      PESHdrSize = PES_HEADER_LEN + pesHeaderDataLength;
   }

   AM_INT PESPayloadSpace = MPEG_TS_TP_PACKET_SIZE -
                            MPEG_TS_TP_PACKET_HEADER_SIZE -
                            initialAdaptFiledSize - PESHdrSize;
   /* printf("PESPayloadSpace %d (%d %d)\n",
    *        PESPayloadSpace, initialAdaptFiledSize, PESHdrSize);*/
   if (pFd->payloadSize < PESPayloadSpace) {
      stuffSize = PESPayloadSpace - pFd->payloadSize;
   }

   if (stuffSize > 0) { // need stuffing
      AM_INT realStuffSize = stuffSize;

      if (initialAdaptFiledSize == 0) { // adapt header is not written yet
         if (stuffSize == 1) {
            tmpWritePtr ++;   // write the adapt_field_length byte (=0)
            realStuffSize--;
         } else {
            tmpWritePtr += 2; // write the two byte adapt header
            realStuffSize -= 2;
            // adaptation_field_length 0 --> 1
            adaptHdr->adaptation_field_length = 1;
         }
      }

      if (realStuffSize > 0) { // stuff size should be >= 2
         // tmpWritePtr should point to the start of stuffing area
         memset(tmpWritePtr, 0xff, realStuffSize);
         tmpWritePtr += realStuffSize;
         adaptHdr->adaptation_field_length += realStuffSize;
      }
   }

   // update adaptation_field_control of TS header
   tsHeader->adaptation_field_control = (pFd->withPCR || stuffSize > 0) ?
      MPEG_TS_ADAPTATION_FIELD_BOTH : MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;

   // calcuate the start addr of PES packet
   pesPacket = tmpWritePtr;

   //printf("FirstSlice %d, withPCR %d, payload %d, adaptField %d, stuff %d\n",
   //       pFd->firstSlice, pFd->withPCR, pFd->payloadSize,
   //       pesPacket - (AM_U8*)adaptHdr, stuffSize);

   /* TS packet payload (PES header + PES payload or PES playload only) */

   if (tsHeader->payload_unit_start_indicator) {
     // one PES packet is started in this transport packet
     MPEG_TS_TP_PES_HEADER *pesHeader = (MPEG_TS_TP_PES_HEADER *)pesPacket;

     pesHeader->packet_start_code_23to16 = 0;
     pesHeader->packet_start_code_15to8  = 0;
     pesHeader->packet_start_code_7to0   = 0x01;
     pesHeader->marker_bits              = 2;
     pesHeader->pes_scrambling_control   =
         MPEG_TS_PES_SCRAMBLING_CTRL_NOT_SCRAMBLED;
     pesHeader->pes_priority             = 0;
     pesHeader->data_alignment_indicator =
         MPEG_TS_PES_ALIGNMENT_CONTROL_STARTCODE;
     pesHeader->copyright            = MPEG_TS_PES_COPYRIGHT_UNDEFINED;
     pesHeader->original_or_copy     = MPEG_TS_PES_ORIGINAL_OR_COPY_COPY;
     pesHeader->escr_flag            = MPEG_TS_PES_ESCR_NOT_PRESENT;
     pesHeader->es_rate_flag         = MPEG_TS_PES_ES_NOT_PRESENT;
     pesHeader->dsm_trick_mode_flag  = MPEG_TS_PES_DSM_TRICK_MODE_NOT_PRESENT;
     pesHeader->add_copy_info_flag   = MPEG_TS_PES_ADD_COPY_INFO_NOT_PRESENT;
     pesHeader->pes_crc_flag         = MPEG_TS_PES_CRC_NOT_PRESENT;
     pesHeader->pes_ext_flag         = MPEG_TS_PES_EXT_NOT_PRESENT;
     pesHeader->pts_dts_flags        = pesPtsDtsFlag;
     pesHeader->header_data_length   = pesHeaderDataLength;

     /*Set stream_id & pes_packet_size*/
     AM_U16 pesPacketSize;
     if (pStr->type == MPEG_SI_STREAM_TYPE_AVC_VIDEO) {
       pesHeader->stream_id = MPEG_TS_STREAM_ID_VIDEO_00;
       if (pFd->payloadSize < (MPEG_TS_TP_PACKET_SIZE - (((AM_U8 *)pesPacket) -
                               pBufPES) -/* TS header + adaptation field */
                               PES_HEADER_LEN -
                               pesHeader->header_data_length)) {
         // 3 bytes following PES_packet_length field
         pesPacketSize = 3 + pesHeader->header_data_length + (pFd->payloadSize);
       } else {
         pesPacketSize = 0;
       }
     } else if (pStr->type == MPEG_SI_STREAM_TYPE_AAC) {
       pesHeader->stream_id = MPEG_TS_STREAM_ID_AUDIO_00;
       // 3 bytes following PES_packet_length field
       pesPacketSize = 3 + pesHeader->header_data_length + (pFd->payloadSize);
     } else if (pStr->type == MPEG_SI_STREAM_TYPE_LPCM_AUDIO) {
       pesHeader->stream_id = MPEG_TS_STREAM_ID_PRIVATE_STREAM_1;
       // add 4 bytes for LPCMAudioDataHeader
       pesPacketSize = 3 + pesHeader->header_data_length +
                       (pFd->payloadSize) + 4;
     } else {
       return ME_ERROR;
     }

     pesHeader->pes_packet_length_15to8 = ((pesPacketSize) >> 8) & 0xff;
     pesHeader->pes_packet_length_7to0  = (pesPacketSize) & 0xff;

     // point to PES header optional field
     tmpWritePtr += PES_HEADER_LEN;

     switch (pesHeader->pts_dts_flags) {
       case MPEG_TS_PTS_DTS_PTS_ONLY: {
         tmpWritePtr[0] = 0x21 | (((pFd->pts >> 30) & 0x07) << 1);
         tmpWritePtr[1] = ((pFd->pts >> 22) & 0xff);
         tmpWritePtr[2] = (((pFd->pts >> 15) & 0x7f) << 1) | 0x1;
         tmpWritePtr[3] = ((pFd->pts >> 7) & 0xff);
         tmpWritePtr[4] = ((pFd->pts & 0x7f) << 1) | 0x1;
       } break;
       case MPEG_TS_PTS_DTS_PTSDTS_BOTH: {
         tmpWritePtr[0]  = 0x31 | (((pFd->pts >> 30) & 0x07) << 1);
         tmpWritePtr[1]  = ((pFd->pts >> 22) & 0xff);
         tmpWritePtr[2]  = (((pFd->pts >> 15) & 0x7f) << 1) | 0x1;
         tmpWritePtr[3]  = ((pFd->pts >> 7) & 0xff);
         tmpWritePtr[4]  = ((pFd->pts & 0x7f) << 1) | 0x1;

         tmpWritePtr[5]  = 0x11 | (((pFd->dts >> 30) & 0x07) << 1);
         tmpWritePtr[6]  = ((pFd->dts >> 22) & 0xff);
         tmpWritePtr[7]  = (((pFd->dts >> 15) & 0x7f) << 1) | 0x1;
         tmpWritePtr[8]  = ((pFd->dts >> 7) & 0xff);
         tmpWritePtr[9]  = ((pFd->dts & 0x7f) << 1) | 0x1;
       } break;
       case MPEG_TS_PTS_DTS_NO_PTSDTS:
       default: break;
     }
     tmpWritePtr += pesHeader->header_data_length;

     if (pStr->type == MPEG_SI_STREAM_TYPE_LPCM_AUDIO) {
       //AVCHDLPCMAudioDataHeader() {
       //      AudioDataPayloadSize 16
       //      ChannelAssignment     4      3 for stereo
       //      SamplingFrequency     4      1 for 48kHz
       //      BitPerSample          2      1 for 16-bit
       //      StartFlag             1
       //      reserved              5
       //}
       tmpWritePtr[0]  = (pFd->payloadSize >> 8) & 0xFF;//0x03
       tmpWritePtr[1]  = pFd->payloadSize & 0xFF;//0xC0;
       if (mAudioInfo.channels == 2) {
         tmpWritePtr[2] = 0x31; //48kHz stereo
       } else if (mAudioInfo.channels == 1) {
         tmpWritePtr[2] = 0x11; //48kHz mono
       }
       tmpWritePtr[3]  = 0x60; //16 bits per sample
       tmpWritePtr += 4;
     }
   }

   // tmpWritePtr should be the latest write point
   int payload_fillsize = MPEG_TS_TP_PACKET_SIZE - (tmpWritePtr - pBufPES);
   // printf("fillsize %d, payload %d, offset %d, pesHdr %d\n",
   //        payload_fillsize, pFd->payloadSize, tmpWritePtr - pBufPES,
   //        tmpWritePtr - pesPacket);

   AM_ASSERT(payload_fillsize <= pFd->payloadSize);
   memcpy(tmpWritePtr, pFd->pPlayload, payload_fillsize);
   return payload_fillsize;
}

inline int CTsBuilder::Cal_crc32(AM_U8 * buf, int size)
{
   int crc = 0xffffffffL;

   for(int i = 0; i < size; ++ i) {
      Crc32_Byte(&crc, (int)buf[i]);
   }
   return (crc);
}

inline void CTsBuilder::Crc32_Byte(int *preg, int x)
{
   int i;
   for(i = 0, x <<= 24; i < 8; ++ i, x <<= 1) {
      (*preg) = ((*preg) << 1) ^ (((x ^ (*preg)) >> 31) & 0x04C11DB7);
   }
}

void CTsBuilder::SetAudioInfo(AM_AUDIO_INFO* aInfo)
{
  if (AM_LIKELY(aInfo)) {
    memcpy(&mAudioInfo, aInfo, sizeof(mAudioInfo));
  } else {
    memset(&mAudioInfo, 0, sizeof(mAudioInfo));
  }
}

void CTsBuilder::SetVideoInfo(AM_VIDEO_INFO* vInfo)
{
  if (AM_LIKELY(vInfo)) {
    memcpy(&mVideoInfo, vInfo, sizeof(mVideoInfo));
  } else {
    memset(&mVideoInfo, 0, sizeof(mVideoInfo));
  }
}

