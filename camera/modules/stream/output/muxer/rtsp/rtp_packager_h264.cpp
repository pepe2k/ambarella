/*******************************************************************************
 * rtp_packager_h264.cpp
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

#include "h264_structs.h"
#include "rtp_packager.h"
#include "rtp_packager_h264.h"

static inline uint8_t* base64Encode(const uint8_t *data, uint32_t length)
{
  uint8_t *result = NULL;

  if (AM_LIKELY(data)) {
    const char base64Char[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const uint32_t numOrig24BitValues = length / 3;
    bool havePadding  = (length > (numOrig24BitValues * 3));
    bool havePadding2 = (length == (numOrig24BitValues * 3 + 2));
    const uint32_t numResultBytes =
        4 * (numOrig24BitValues + (havePadding ? 1 : 0));
    uint32_t i = 0;

    result = new uint8_t[numResultBytes + 1]; /* allow for trailing '\0' */

    if (AM_LIKELY(result)) {
      memset(result, 0, numResultBytes + 1);
      // Map each full group of 3 input bytes into 4 output base-64 characters:
      for (i = 0; i < numOrig24BitValues; ++ i) {
        result[4 * i + 0] = base64Char[(data[3 * i] >> 2) & 0x3F];
        result[4 * i + 1] = base64Char[(((data[3 * i] & 0x3) << 4) |
                                       (data[3 * i + 1] >> 4)) & 0x3F];
        result[4 * i + 2] = base64Char[((data[3 * i + 1] << 2) |
                                       (data[3 * i + 2] >> 6)) & 0x3F];
        result[4 * i + 3] = base64Char[data[3 * i + 2] & 0x3F];
      }

      // Now, take padding into account.  (Note: i == numOrig24BitValues)
      if (havePadding) {
        result[4 * i + 0] = base64Char[(data[ 3 * i] >> 2) & 0x3F];
        if (havePadding2) {
          result[4 * i + 1] = base64Char[(((data[3 * i] & 0x3) << 4) |
                                         (data[3 * i + 1] >> 4)) & 0x3F];
          result[4 * i + 2] = base64Char[(data[3 * i + 1] << 2) & 0x3F];
        } else {
          result[4 * i + 1] = base64Char[((data[3 * i] & 0x3) << 4) & 0x3F];
          result[4 * i + 2] = '=';
        }
        result[4 * i + 3] = '=';
      }

      result[numResultBytes] = '\0';
    }
  }

  return result;
}

static inline bool FindNalu(NALU*    nalus,     uint32_t* count,
                            uint8_t* bitstream, uint32_t streamsize)
{
  bool ret = false;
  if (AM_LIKELY(nalus && count && bitstream && (streamsize > 4))) {
    int32_t index = -1;
    uint8_t* bs = bitstream;
    uint8_t* lastHeader = bitstream + streamsize - 4;

    while (bs <= lastHeader) {
      if (AM_UNLIKELY(0x00000001 ==
          ((bs[0] << 24) | (bs[1] << 16) | (bs[2] << 8) | bs[3]))) {
        ++ index;
        bs += 4;
        nalus[index].nalu_type = (uint32_t)(0x1F & bs[0]);
        nalus[index].addr = bs;
        if (AM_LIKELY(index > 0)) {
          /* Calculate the previous NALU's size */
          nalus[index - 1].size = nalus[index].addr - nalus[index - 1].addr - 4;
        }
      } else if (bs[3] != 0) {
        bs += 4;
      } else if (bs[2] != 0) {
        bs += 3;
      } else if (bs[1] != 0) {
        bs += 2;
      } else {
        bs += 1;
      }
    }
    if (AM_LIKELY(index >= 0)) {
      /* Calculate the last NALU's size */
      nalus[index].size = bitstream + streamsize - nalus[index].addr;
      *count = index + 1;
      ret = (*count > 0);
    }
    if (AM_UNLIKELY(false == ret)) {
      ERROR("Found 0 NALU in bitstream[%u bytes]", streamsize);
    }
  }

  return ret;
}

static inline uint32_t calcH264PktNum(NALU *nalu, uint32_t maxPacketSize)
{
  uint32_t nalPayloadSize = nalu->size - sizeof(nal_header_type);
  uint32_t fuaPayloadSize = maxPacketSize - sizeof(fu_indicator_type) -
                            sizeof(fu_header_type);

  if (nalPayloadSize <= fuaPayloadSize) {
    nalu->packetsNum = 1;
  } else {
    nalu->packetsNum = (((nalPayloadSize % fuaPayloadSize) > 0) ?
        ((nalPayloadSize / fuaPayloadSize) + 1) :
        (nalPayloadSize / fuaPayloadSize));
  }

  return nalu->packetsNum;
}

uint8_t* CRtpPackagerH264::getSpsString()
{
  while (!mSpsStr) {
    usleep(500000);
  }

  return mSpsStr;
}

uint8_t* CRtpPackagerH264::getPpsString()
{
  while (!mPpsStr) {
    usleep(500000);
  }

  return mPpsStr;
}

uint32_t CRtpPackagerH264::getProfileLevelId()
{
  while (mProfileLevelId == 0) {
    usleep(500000);
  }

  return mProfileLevelId;
}


RtspPacket* CRtpPackagerH264::assemblePacketH264(uint8_t  *data,
                                                 uint32_t &len,
                                                 uint32_t &maxTcpPayload,
                                                 uint32_t &packetNum,
                                                 uint16_t &sequenceNum,
                                                 uint32_t &timeStamp,
                                                 uint32_t &ssrc,
                                                 int32_t  &timeStampInc)
{
  NALU nalu[max_nalu_number];
  uint32_t naluCount     = 0;
  uint32_t packetCount   = 0;
  uint32_t bufferSize    = 0;
  uint8_t *buf           = NULL;
  uint32_t maxPacketSize =
      maxTcpPayload - sizeof(RtspTcpHeader) - sizeof(RtpHeader);
  bool isTimeStampAdd = false;

  packetNum = 0;
  if (AM_UNLIKELY(!FindNalu(nalu, &naluCount, data, len))) {
    return NULL;
  }

  for (uint32_t i = 0; i < naluCount; ++ i) {
    if (AM_LIKELY((nalu[i].nalu_type != SEIHEAD) &&
                  (nalu[i].nalu_type != AUDHEAD))) {
      packetNum += calcH264PktNum(&nalu[i], maxPacketSize);
      if (AM_UNLIKELY(!mSpsStr && (nalu[i].nalu_type == SPSHEAD))) {
        mSpsStr = base64Encode(nalu[i].addr, nalu[i].size);
        mProfileLevelId =
            (nalu[i].addr[1] << 16) | (nalu[i].addr[2] << 8) | nalu[i].addr[3];
      }
      if (AM_UNLIKELY(!mPpsStr && (nalu[i].nalu_type == PPSHEAD))) {
        mPpsStr = base64Encode(nalu[i].addr, nalu[i].size);
      }
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
    NOTICE("%08x: H.264 buffer size increased to %u bytes!", ssrc, mBufMaxSize);
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
    NOTICE("%08x: H.264 packet buffer increased to %u bytes (%u packets)",
           ssrc, sizeof(RtspPacket) * packetNum, packetNum);
  }

  memset(mBuffer, 0, bufferSize);
  buf = mBuffer;

  for (uint32_t i = 0; i < naluCount; ++ i) {
    uint8_t *addr = nalu[i].addr;
    uint32_t size = nalu[i].size;
    if (AM_UNLIKELY((nalu[i].nalu_type == SEIHEAD) ||
                    (nalu[i].nalu_type == AUDHEAD))) {
      continue;
    }
    if (AM_UNLIKELY(!isTimeStampAdd && ((nalu[i].nalu_type == NALUHEAD) ||
                                        (nalu[i].nalu_type == SPSHEAD)))) {
      timeStamp += timeStampInc;
      isTimeStampAdd = true;
    }

    if (AM_LIKELY(nalu[i].packetsNum > 1)) {
      uint8_t          *tempNalu = &addr[sizeof(nal_header_type)];
      uint32_t        remainSize = size - sizeof(nal_header_type);
      uint32_t    fuaPayloadSize = maxPacketSize - sizeof(fu_indicator_type) -
          sizeof(fu_header_type);

      for (uint32_t count  = 0; count < nalu[i].packetsNum; ++ count) {
        uint8_t *tcpHeader   = &buf[0];
        uint8_t *rtpHeader   = &buf[sizeof(RtspTcpHeader)];
        uint8_t *fuaStart    = &buf[sizeof(RtspTcpHeader) + sizeof(RtpHeader)];
        uint8_t *payloadAddr = &fuaStart[2];
        uint32_t rtspTcpPayloadSize = 0;

        ++ sequenceNum;
        mPacket[packetCount].tcpData = (uint8_t*)tcpHeader;
        mPacket[packetCount].udpData = (uint8_t*)rtpHeader;

        if (AM_LIKELY(remainSize >= fuaPayloadSize)) {
          memcpy(payloadAddr, tempNalu, fuaPayloadSize);
          mPacket[packetCount].totalSize = maxTcpPayload;
          tempNalu += fuaPayloadSize;
          remainSize -= fuaPayloadSize;
        } else {
          memcpy(payloadAddr, tempNalu, remainSize);
          mPacket[packetCount].totalSize =
              remainSize + sizeof(RtspTcpHeader) + sizeof(RtpHeader) +
              sizeof(fu_indicator_type) + sizeof(fu_header_type);
        }
        rtspTcpPayloadSize =
            mPacket[packetCount].totalSize - sizeof(RtspTcpHeader);
        /* Build RTSP TCP Header*/
        tcpHeader[0] = 0x24; /* Always 0x24 */
        /* tcpHeader[1] should be set to client rtp id */
        tcpHeader[2] = (rtspTcpPayloadSize & 0xff00) >> 8;
        tcpHeader[3] = rtspTcpPayloadSize & 0x00ff;
        /* Build RTP Header */
        rtpHeader[0] = 0x80;
        rtpHeader[1] = (count == (nalu[i].packetsNum - 1)) ?
            0x80 | ((RTP_PAYLOAD_TYPE_H264) & 0x7f) :
            RTP_PAYLOAD_TYPE_H264;
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
        fuaStart[0] = 0x1c | (0xe0 & addr[0]);
        if (AM_UNLIKELY(count == 0)) {
          fuaStart[1] = 0x80;
        } else if (AM_UNLIKELY(count == (nalu[i].packetsNum - 1))) {
          fuaStart[1] = 0x40;
        }
        fuaStart[1] |= (0x1F & addr[0]);
        buf += maxTcpPayload;
        ++ packetCount;
      }
    } else {
      uint8_t *tcpHeader = &buf[0];
      uint8_t *rtpHeader = &buf[sizeof(RtspTcpHeader)];
      uint8_t *payload   = &buf[sizeof(RtspTcpHeader) + sizeof(RtpHeader)];
      uint32_t rtspTcpPayloadSize = 0;

      ++ sequenceNum;
      memcpy(payload, nalu[i].addr, nalu[i].size);
      mPacket[packetCount].tcpData = (uint8_t*)tcpHeader;
      mPacket[packetCount].udpData = (uint8_t*)rtpHeader;
      mPacket[packetCount].totalSize =
          nalu[i].size + sizeof(RtspTcpHeader) + sizeof(RtpHeader);
      /* Build RTSP TCP Header */
      rtspTcpPayloadSize =
          mPacket[packetCount].totalSize - sizeof(RtspTcpHeader);
      tcpHeader[0] = 0x24; /* Always 0x24 */
      /* tcpHeader[1] should be set to client rtp id */
      tcpHeader[2] = (rtspTcpPayloadSize & 0xff00) >> 8;
      tcpHeader[3] = rtspTcpPayloadSize & 0x00ff;
      /* Build RTP Header */
      rtpHeader[0] = 0x80;
      rtpHeader[1] = 0x80 | ((RTP_PAYLOAD_TYPE_H264) & 0x7f);
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
      buf += maxTcpPayload;
      ++ packetCount;
    }
  }

  return mPacket;
}
