/*******************************************************************************
 * rtp_packager_mjpeg.cpp
 *
 * History:
 *   2013年8月20日 - [ypchang] created file
 *
 * Copyright (C) 2008-2013, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_utility.h"
#include "am_data.h"

#include "mjpeg_structs.h"
#include "rtp_packager.h"
#include "rtp_packager_mjpeg.h"

static inline bool ParseJpeg(JpegParams *jpeg, uint8_t *data, uint32_t len)
{
  bool ret = false;
  if (AM_LIKELY(jpeg && data && len > 4)) {
    do {
      uint8_t *start  = data;
      uint8_t *end    = data + len - 2;
      uint32_t count  = 0;

      memset(jpeg, 0, sizeof(JpegParams));
      while (end > start) {
        /* Check picture end mark 0xFFD9 */
        if (AM_LIKELY(((end[0] << 8) | end[1]) == 0xFFD9)) {
          break;
        }
        -- end;
      }
      if (AM_UNLIKELY(((end[0] << 8) | end[1]) != 0xFFD9)) {
        ERROR("Failed to check end mark 0xffd9!");
        break;
      }

      if (AM_UNLIKELY(((start[0] << 8) | start[1]) != 0xFFD8)) {
        /* Check picture start mark 0xFFD8 SOI */
        ERROR("Failed to check start mark 0xffd8!");
        break;
      } else {
        start += 2; /* Skip 0xFFD8 */
      }

      /* Skip APP0 ~ APPn */
      while (((end - start) > 2) &&
          (((start[0] << 8) | (start[1] & 0xF0)) == 0xFFE0)) {
        start += 2; /* Skip 0xFFEx */
        start += ((start[0] << 8) | start[1]);
      }

      while (((end - start) > 2) && (((start[0] << 8) | start[1]) == 0xFFDB)) {
        /* Check DQT mark 0xFFDB */
        uint8_t precision = 0;
        uint32_t qTableStart = jpeg->qTableLen;

        start += 2; /* Skip 0xFFDB */
        precision = start[2] >> 4;

        if (0 == precision) {
          jpeg->precision &= ~(1 << count);
          jpeg->qTableLen += 64;
        } else if (1 == precision) {
          jpeg->precision |= (1 << count);
          jpeg->qTableLen += 64 << 1;
        } else {
          ERROR("Invalid precision value: %hhu", precision);
          start = end; /* Make an error to quit parsing */
          break;
        }
        memcpy(&(jpeg->qTable[qTableStart]), &start[3], jpeg->qTableLen);
        start += ((start[0] << 8) | start[1]);
        ++ count;
      }

      if (AM_UNLIKELY(((end - start) <= 2) ||
                      (((start[0] << 8) | start[1]) != 0xFFC0))) {
        /* Check SOF0 mark 0xFFC0 */
        ERROR("Failed to check SOF0 0xffc0: end - start = %u "
              "start[0] == %02x, start[1] == %02x",
              end - start, start[0], start[1]);
        break;
      } else {
        start += 2;
      }
      if (AM_UNLIKELY((end - start) < 17)) {
        ERROR("Invalid data length!");
        break;
      }
      if (AM_UNLIKELY(start[2] != 8)) {
        /* Need precision to be 8 */
        ERROR("Invalid precision value: %hhu!", start[2]);
        break;
      }
      jpeg->height = ((start[3] << 8) | start[4]);
      jpeg->width  = ((start[5] << 8) | start[6]);
      if (AM_UNLIKELY(start[7] != 3)) {
        /* component number must be 3 */
        ERROR("Invalid component number: %hhu", start[7]);
        break;
      } else {
        uint8_t id[3]        = {0};
        uint8_t factor[3]    = {0};
        uint8_t qTableNum[3] = {0};
        for (uint32_t i = 0; i < 3; ++ i) {
          id[i]        = start[8 + i * 3];
          factor[i]    = start[9 + i * 3];
          qTableNum[i] = start[10 + i * 3];
        }
        if (((id[0] == 1) && (factor[0] == 0x21) && (qTableNum[0] == 0)) &&
            ((id[1] == 2) && (factor[1] == 0x11) && (qTableNum[1] == 1)) &&
            ((id[2] == 3) && (factor[2] == 0x11) && (qTableNum[2] == 1))) {
          jpeg->type = 0;
        } else if (((id[0]==1) && (factor[0]==0x22) && (qTableNum[0]==0)) &&
                   ((id[1]==2) && (factor[1]==0x11) && (qTableNum[1]==1)) &&
                   ((id[2]==3) && (factor[2]==0x11) && (qTableNum[2]==1))) {
          jpeg->type = 1;
        } else {
          ERROR("Invalid type!");
          break;
        }
      }
      start += ((start[0] << 8) | start[1]);

      while(((end - start) > 2) && (((start[0] << 8) | start[1]) == 0xFFC4)) {
        /* Skip DHT */
        start += 2; /* Skip 0xFFC4 */
        start += ((start[0] << 8) | start[1]);
      }

      if (((end - start) > 2) && (((start[0] << 8) | start[1]) == 0xFFDD)) {
        /* DRI exists */
        start += 2; /* Skip 0xFFDD */
        start += ((start[0] << 8) | start[1]);
      }

      if (AM_UNLIKELY(((end - start) <= 2) ||
                      (((start[0] << 8) | start[1]) != 0xFFDA))) {
        /* Check SOS mark */
        ERROR("Failed to check SOS mark 0xffda!");
        break;
      } else {
        start += 2; /* Skip 0xFFDA */
      }
      start += ((start[0] << 8) | start[1]);
      if (AM_UNLIKELY(start >= end)) {
        ERROR("Invalid data length!");
        break;
      }
      jpeg->data = start;
      jpeg->len  = end - start;
      ret = true;
    } while(0);
  }

  return ret;
}

CRtpPackagerMjpeg::CRtpPackagerMjpeg() :
    CRtpPackager(),
    mJpegParam(NULL)
{}

CRtpPackagerMjpeg::~CRtpPackagerMjpeg()
{
  delete mJpegParam;
  DEBUG("~CRtpPackagerMjpeg");
}

RtspPacket* CRtpPackagerMjpeg::assemblePacketJpeg(uint8_t  *data,
                                                  uint8_t  &qfactor,
                                                  uint32_t &len,
                                                  uint32_t &maxTcpPayload,
                                                  uint32_t &packetNum,
                                                  uint16_t &sequenceNum,
                                                  uint32_t &timeStamp,
                                                  uint32_t &ssrc,
                                                  int32_t  &timeStampInc)
{
  uint8_t *buf           = NULL;
  uint32_t bufferSize    = 0;
  uint32_t maxPacketSize =
        maxTcpPayload - sizeof(RtspTcpHeader) - sizeof(RtpHeader);
  bool needQTable = (qfactor >= 128);

  if (AM_UNLIKELY(NULL == mJpegParam)) {
    mJpegParam = new JpegParams();
  }
  if (AM_UNLIKELY((NULL == mJpegParam) || !ParseJpeg(mJpegParam, data, len))) {
    if (AM_LIKELY(!mJpegParam)) {
      ERROR("Failed to create JpegParam!");
    } else {
      ERROR("Failed to parse MJPEG frame!");
    }
    packetNum = 0;
    return NULL;
  } else {
    uint32_t totalPayloadSize = mJpegParam->len +
        (needQTable ? (sizeof(JpegQuantizationHdr)+mJpegParam->qTableLen) : 0);
    uint32_t payloadSize = maxPacketSize - sizeof(JpegHdr);
    packetNum = (totalPayloadSize / payloadSize) +
        ((totalPayloadSize % payloadSize) ? 1 : 0);
    if (AM_UNLIKELY(!packetNum)) {
      return NULL;
    }
  }

  bufferSize = packetNum * maxTcpPayload;

  if (AM_LIKELY(bufferSize > mBufMaxSize)) {
    delete[] mBuffer;
    mBuffer = new uint8_t[bufferSize];
    if (AM_UNLIKELY(!mBuffer)) {
      ERROR("%08x: Failed to realloc memory to %u!", ssrc, bufferSize);
      return NULL;
    }
    mBufMaxSize = bufferSize;
    NOTICE("%08x: MJPEG buffer size increased to %u bytes!", ssrc, mBufMaxSize);
  }

  if (AM_LIKELY(packetNum > mPktMaxNum)) {
    delete[] mPacket;
    mPacket = new RtspPacket[packetNum];
    if (AM_UNLIKELY(!mPacket)) {
      ERROR("%08x: Failed to realloc memory to %u!",
            ssrc, sizeof(RtspPacket) * packetNum);
      return NULL;
    }
    mPktMaxNum = packetNum;
    NOTICE("%08x: MJPEG packet buffer increased to %u bytes (%u packets)",
           ssrc, sizeof(RtspPacket) * packetNum, packetNum);
  }

  memset(mBuffer, 0, bufferSize);
  buf = mBuffer;

  timeStamp += timeStampInc;
  for (uint32_t count = 0; count < packetNum; ++ count) {
    uint8_t *tcpHeader     = &buf[0];
    uint8_t *rtpHeader     = &buf[sizeof(RtspTcpHeader)];
    uint8_t *jpgHeader     = rtpHeader + sizeof(RtpHeader);
    uint8_t *dataPtr       = jpgHeader + sizeof(JpegHdr);
    uint32_t packetHdrSize =
        sizeof(RtspTcpHeader) + sizeof(RtpHeader) + sizeof(JpegHdr) +
        ((needQTable && (count == 0)) ?
            (sizeof(JpegQuantizationHdr) + mJpegParam->qTableLen) : 0);
    uint32_t payloadDataSize = ((mJpegParam->len - mJpegParam->offset) >
    (maxTcpPayload - packetHdrSize)) ? (maxTcpPayload - packetHdrSize) :
        (mJpegParam->len - mJpegParam->offset);
    uint32_t rtspTcpPayloadSize = packetHdrSize - sizeof(RtspTcpHeader) +
        payloadDataSize;
    ++ sequenceNum;

    /* Build RTSP TCP Header */
    tcpHeader[0] = 0x24; /* Always 0x24 */
    /* tcpHeader[1] should be set to client rtp id */
    tcpHeader[2] = (rtspTcpPayloadSize & 0xff00) >> 8;
    tcpHeader[3] = rtspTcpPayloadSize & 0x00ff;

    /* Build RTP Header */
    rtpHeader[0]  = 0x80;
    rtpHeader[1] = (count == (packetNum - 1)) ?
        (0x80 | ((RTP_PAYLOAD_TYPE_JPEG) & 0x7f)) : RTP_PAYLOAD_TYPE_JPEG;
    rtpHeader[2]  = (sequenceNum & 0xff00) >> 8;
    rtpHeader[3]  = sequenceNum & 0x00ff;
    rtpHeader[4]  = (timeStamp & 0xff000000) >> 24;
    rtpHeader[5]  = (timeStamp & 0x00ff0000) >> 16;
    rtpHeader[6]  = (timeStamp & 0x0000ff00) >> 8;
    rtpHeader[7]  = (timeStamp & 0x000000ff);
    rtpHeader[8]  = (ssrc & 0xff000000) >> 24;
    rtpHeader[9]  = (ssrc & 0x00ff0000) >> 16;
    rtpHeader[10] = (ssrc & 0x0000ff00) >> 8;
    rtpHeader[11] = (ssrc & 0x000000ff);

    jpgHeader[0]  = 0; /* type-specific */
    jpgHeader[1]  = (mJpegParam->offset >> 16) & 0xff;
    jpgHeader[2]  = (mJpegParam->offset >> 8)  & 0xff;
    jpgHeader[3]  = (mJpegParam->offset & 0xff);
    jpgHeader[4]  = mJpegParam->type;
    jpgHeader[5]  = qfactor;
    jpgHeader[6]  = mJpegParam->width  >> 3;
    jpgHeader[7]  = mJpegParam->height >> 3;
    if (AM_UNLIKELY(needQTable && (count == 0))) {
      JpegQuantizationHdr *quantizationHdr = (JpegQuantizationHdr*)dataPtr;
      quantizationHdr->mbz       = 0;
      quantizationHdr->precision = mJpegParam->precision;
      quantizationHdr->length2   = (mJpegParam->qTableLen >> 8) & 0xff;
      quantizationHdr->length1   = (mJpegParam->qTableLen & 0xff);
      dataPtr += sizeof(JpegQuantizationHdr);
      memcpy(dataPtr, mJpegParam->qTable, mJpegParam->qTableLen);
      dataPtr += mJpegParam->qTableLen;
    }
    memcpy(dataPtr, mJpegParam->data + mJpegParam->offset, payloadDataSize);
    mJpegParam->offset += payloadDataSize;
    mPacket[count].tcpData = tcpHeader;
    mPacket[count].udpData = rtpHeader;
    mPacket[count].totalSize = packetHdrSize + payloadDataSize;
    buf = dataPtr + payloadDataSize;
  }

  return mPacket;
}
