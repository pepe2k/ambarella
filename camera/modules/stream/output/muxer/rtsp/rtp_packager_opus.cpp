/*******************************************************************************
 * rtp_packager_opus.cpp
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

#include "adts.h"
#include "rtp_packager.h"
#include "rtp_packager_opus.h"

RtspPacket* CRtpPackagerOpus::assemblePacketOpus(uint8_t  *data,
                                                 uint32_t &len,
                                                 uint32_t &inPacketNum,
                                                 uint32_t &outPacketNum,
                                                 uint16_t &sequenceNum,
                                                 uint32_t &timeStamp,
                                                 uint32_t &ssrc,
                                                 uint32_t &timeStampInc)
{
  uint8_t         *buf = NULL;
  uint8_t   *tcpHeader = NULL;
  uint8_t   *rtpHeader = NULL;
  uint8_t *payloadAddr = NULL;
  uint16_t frameLength = len;
  uint32_t  bufferSize = len + sizeof(RtpHeader) + sizeof(RtspTcpHeader);
  uint32_t rtspTcpPayloadSize = frameLength + RTP_HEADER_SIZE;

  outPacketNum = 1;
  if (AM_UNLIKELY(1 != inPacketNum)) {
    return NULL;
  }

  if (AM_LIKELY(bufferSize > mBufMaxSize)) {
    delete[] mBuffer;
    mBuffer = new uint8_t[bufferSize];
    if (AM_UNLIKELY(!mBuffer)) {
      ERROR("%08x: Failed to realloc memory to %u!", ssrc, bufferSize);
      return NULL;
    }
    mBufMaxSize = bufferSize;
    NOTICE("%08x: OPUS buffer size increased to %u bytes!", ssrc, mBufMaxSize);
  }

  if (AM_LIKELY(0 == mPktMaxNum)) {
    delete[] mPacket;
    mPacket = new RtspPacket[1];
    if (AM_UNLIKELY(!mPacket)) {
      ERROR("%08x: Failed to realloc memory to %u!",
            ssrc, sizeof(RtspPacket));
      return NULL;
    }
    mPktMaxNum = 1;
    NOTICE("%08x: OPUS packet buffer increased to %u bytes",
           ssrc, sizeof(RtspPacket));
  }

  memset(mBuffer, 0, bufferSize);
  buf = mBuffer;
  tcpHeader   = &buf[0];
  rtpHeader   = &buf[sizeof(RtspTcpHeader)];
  payloadAddr = (rtpHeader + sizeof(RtpHeader));
  timeStamp += timeStampInc;
  ++ sequenceNum;

  /* Build RTSP TCP Header */
  tcpHeader[0] = 0x24; /* Always 0x24 */
  /* tcpHeader[1] should be set to client rtp id */
  tcpHeader[2] = (rtspTcpPayloadSize & 0xff00) >> 8;
  tcpHeader[3] = rtspTcpPayloadSize & 0x00ff;

  /* Build RTP Header */
  rtpHeader[0]  = 0x80;
  rtpHeader[1]  = RTP_PAYLOAD_TYPE_OPUS;
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
  memcpy(payloadAddr, data, frameLength);
  mPacket[0].tcpData = buf;
  mPacket[0].udpData = rtpHeader;
  mPacket[0].totalSize =
      frameLength + sizeof(RtspTcpHeader) + sizeof(RtpHeader);

  return mPacket;
}
