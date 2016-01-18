/*******************************************************************************
 * rtp_packager_aac.cpp
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
#include "rtp_packager_aac.h"

#define AU_HEADER_LENGTH_SIZE 2
#define AU_HEADER_SIZE        4

uint16_t CRtpPackagerAac::getAacConfigNumber()
{
  while (mAacConfig == 0) {
    usleep(500000);
  }

  return mAacConfig;
}

RtspPacket* CRtpPackagerAac::assemblePacketAdts(uint8_t  *data,
                                                uint32_t &len,
                                                uint32_t &inPacketNum,
                                                uint32_t &outPacketNum,
                                                uint16_t &sequenceNum,
                                                uint32_t &timeStamp,
                                                uint32_t &ssrc,
                                                uint32_t &timeStampInc)
{
  uint32_t  bufferSize = len + AU_HEADER_LENGTH_SIZE +
      inPacketNum * AU_HEADER_SIZE + sizeof(RtpHeader) + sizeof(RtspTcpHeader);
  AdtsHeader     *adts = (AdtsHeader*)data;
  uint32_t rtspTcpPayloadSize = AU_HEADER_LENGTH_SIZE +
      inPacketNum * AU_HEADER_SIZE + RTP_HEADER_SIZE;
  uint8_t        *buf  = NULL;
  uint8_t   *tcpHeader = NULL;
  uint8_t   *rtpHeader = NULL;
  uint8_t *auHdrLength = NULL;
  uint8_t    *auheader = NULL;
  uint8_t *payloadAddr = NULL;

  outPacketNum = 1;
  if (AM_UNLIKELY(mAacConfig == 0)) {
    uint8_t config0 =
        (adts->AacAudioObjectType() << 3) | (adts->AacFrequencyIndex() >> 1);
    uint8_t config1 =
        (adts->AacFrequencyIndex() << 7) | (adts->AacChannelConf() << 3);
    mAacConfig = ((config0 << 8) | config1);
  }

  if (AM_LIKELY(bufferSize > mBufMaxSize)) {
    delete[] mBuffer;
    mBuffer = new uint8_t[bufferSize];
    if (AM_UNLIKELY(!mBuffer)) {
      ERROR("%08x: Failed to realloc memory to %u!", ssrc, bufferSize);
      return NULL;
    }
    mBufMaxSize = bufferSize;
    NOTICE("%08x: ADTS buffer size increased to %u bytes!", ssrc, mBufMaxSize);
  }

  if (AM_LIKELY(!mPacket)) {
    delete[] mPacket;
    mPacket = new RtspPacket[1];
    if (AM_UNLIKELY(!mPacket)) {
      ERROR("%08x: Failed to realloc memory to %u!", ssrc, sizeof(RtspPacket));
      return NULL;
    }
    NOTICE("%08x: ADTS packet buffer increased to %u bytes (1 packet)",
           ssrc, sizeof(RtspPacket));
  }

  memset(mBuffer, 0, bufferSize);
  buf = mBuffer;
  tcpHeader   = &buf[0];
  rtpHeader   = &buf[sizeof(RtspTcpHeader)];
  auHdrLength = (rtpHeader) + sizeof(RtpHeader);
  auheader    = auHdrLength + AU_HEADER_LENGTH_SIZE;
  payloadAddr = auheader + AU_HEADER_SIZE * inPacketNum;
  timeStamp  += inPacketNum * timeStampInc;
  ++ sequenceNum;

  /* Build RTSP TCP Header */
  tcpHeader[0] = 0x24; /* Always 0x24 */
  /* tcpHeader[1] should be set to client rtp id */

  /* Build RTP Header */
  rtpHeader[0]   = 0x80;
  rtpHeader[1]   = 0x80 | ((RTP_PAYLOAD_TYPE_AAC) & 0x7f);
  rtpHeader[2]   = (sequenceNum & 0xff00) >> 8;
  rtpHeader[3]   = sequenceNum & 0x00ff;
  rtpHeader[4]   = (timeStamp & 0xff000000) >> 24;
  rtpHeader[5]   = (timeStamp & 0x00ff0000) >> 16;
  rtpHeader[6]   = (timeStamp & 0x0000ff00) >> 8;
  rtpHeader[7]   = (timeStamp & 0x000000ff);
  rtpHeader[8]   = (ssrc & 0xff000000) >> 24;
  rtpHeader[9]   = (ssrc & 0x00ff0000) >> 16;
  rtpHeader[10]  = (ssrc & 0x0000ff00) >> 8;
  rtpHeader[11]  = (ssrc & 0x000000ff);
  auHdrLength[0] = (((AU_HEADER_SIZE * inPacketNum) * 8) >> 8) & 0xff;
  auHdrLength[1] = ((AU_HEADER_SIZE * inPacketNum) * 8) & 0xff;

  for (uint32_t i = 0; i < inPacketNum; ++ i) {
    uint16_t frameLength = adts->FrameLength();

    auheader[0] = (frameLength >> 5) & 0xFF;
    auheader[1] = (frameLength & 0x1f) << 3;
    auheader[2] = 0;
    auheader[3] = 0;
    memcpy(payloadAddr, adts, frameLength);
    adts = (AdtsHeader*)((uint8_t*)adts + frameLength);
    auheader += AU_HEADER_SIZE;
    payloadAddr += frameLength;
    rtspTcpPayloadSize += frameLength;
  }
  tcpHeader[2] = (rtspTcpPayloadSize & 0xff00) >> 8;
  tcpHeader[3] = rtspTcpPayloadSize & 0x00ff;
  mPacket[0].tcpData = buf;
  mPacket[0].udpData = rtpHeader;
  mPacket[0].totalSize = rtspTcpPayloadSize + sizeof(RtspTcpHeader);

  return mPacket;
}
