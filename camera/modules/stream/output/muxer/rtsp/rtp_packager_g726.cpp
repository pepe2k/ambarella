/*******************************************************************************
 * rtp_packager_g726.cpp
 *
 * History:
 *   2013年9月26日 - [ypchang] created file
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

#include "adts.h"
#include "rtp_packager.h"
#include "rtp_packager_g726.h"

RtspPacket* CRtpPackagerG726::assemblePacketG726(uint8_t  *data,
                                                 uint32_t &len,
                                                 uint32_t &inPacketNum,
                                                 uint32_t &outPacketNum,
                                                 uint16_t &sequenceNum,
                                                 uint32_t &timeStamp,
                                                 uint32_t &ssrc,
                                                 uint32_t &timeStampInc)
{
  uint8_t         *buf = NULL;
  uint8_t      *inData = data;
  uint32_t  remainSize = len;
  uint16_t  segmentLen = len / inPacketNum;
  uint32_t  bufferSize = len + inPacketNum * (sizeof(RtpHeader) +
                                              sizeof(RtspTcpHeader));

  outPacketNum = inPacketNum;
  if (AM_LIKELY(bufferSize > mBufMaxSize)) {
    delete[] mBuffer;
    mBuffer = new uint8_t[bufferSize];
    if (AM_UNLIKELY(!mBuffer)) {
      ERROR("%08x: Failed to realloc memory to %u!", ssrc, bufferSize);
      return NULL;
    }
    mBufMaxSize = bufferSize;
    NOTICE("%08x: G726 buffer size increased to %u bytes!", ssrc, mBufMaxSize);
  }

  if (AM_LIKELY(inPacketNum > mPktMaxNum)) {
    delete[] mPacket;
    mPacket = new RtspPacket[inPacketNum];
    if (AM_UNLIKELY(!mPacket)) {
      ERROR("%08x: Failed to realloc memory to %u!",
            ssrc, sizeof(RtspPacket) * inPacketNum);
      return NULL;
    }
    mPktMaxNum = inPacketNum;
    NOTICE("%08x: G726 packet buffer increased to %u bytes (%u packets)",
           ssrc, sizeof(RtspPacket) * inPacketNum, inPacketNum);
  }

  memset(mBuffer, 0, bufferSize);
  buf = mBuffer;
  for (uint32_t i = 0; i < inPacketNum; ++ i) {
    uint8_t*   tcpHeader = &buf[0];
    uint8_t*   rtpHeader = &buf[sizeof(RtspTcpHeader)];
    uint8_t* payloadAddr = (rtpHeader + sizeof(RtpHeader));
    uint16_t frameLength = (segmentLen <= remainSize) ? segmentLen : remainSize;
    uint32_t rtspTcpPayloadSize = frameLength + RTP_HEADER_SIZE;
    remainSize -= frameLength;
    timeStamp += timeStampInc;
    ++ sequenceNum;

    /* Build RTSP TCP Header */
    tcpHeader[0] = 0x24; /* Always 0x24 */
    /* tcpHeader[1] should be set to client rtp id */
    tcpHeader[2] = (rtspTcpPayloadSize & 0xff00) >> 8;
    tcpHeader[3] = rtspTcpPayloadSize & 0x00ff;

    /* Build RTP Header */
    rtpHeader[0]  = 0x80;
    rtpHeader[1]  = 0x80 | (RTP_PAYLOAD_TYPE_G726 & 0x7f);
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
    memcpy(payloadAddr, inData, frameLength);
    mPacket[i].tcpData = buf;
    mPacket[i].udpData = rtpHeader;
    mPacket[i].totalSize =
        frameLength + sizeof(RtspTcpHeader) + sizeof(RtpHeader);
    inData += frameLength;
    buf += mPacket[i].totalSize;
  }

  return mPacket;
}
