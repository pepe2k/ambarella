/*******************************************************************************
 * rtp_packager.h
 *
 * History:
 *   2012-11-25 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef RTP_PACKAGER_H_
#define RTP_PACKAGER_H_

#include "rtp_structs.h"

struct RtspPacket
{
    uint8_t *tcpData;
    uint8_t *udpData;
    uint32_t totalSize;
    RtspPacket() :
      tcpData(0),
      udpData(0),
      totalSize(0){}
    ~RtspPacket(){}

    uint8_t* rtsp_tcp_data()
    {
      return tcpData;
    }

    uint32_t rtsp_tcp_data_size()
    {
      return totalSize;
    }

    uint8_t* rtsp_udp_data()
    {
      return udpData;
    }

    uint32_t rtsp_udp_data_size()
    {
      return (totalSize - sizeof(RtspTcpHeader));
    }
};

class CRtpPackager
{
  public:
    CRtpPackager();
    virtual ~CRtpPackager();

  public:
    virtual uint8_t* getSpsString(){return NULL;}
    virtual uint8_t* getPpsString(){return NULL;}
    virtual uint32_t getProfileLevelId(){return 0;}
    virtual uint16_t getAacConfigNumber(){return 0;}
    virtual RtspPacket* assemblePacketH264(uint8_t  *data,
                                           uint32_t &len,
                                           uint32_t &maxTcpPayload,
                                           uint32_t &packetNum,
                                           uint16_t &sequenceNum,
                                           uint32_t &timeStamp,
                                           uint32_t &ssrc,
                                           int32_t  &timeStampInc)
    {return NULL;}

    virtual RtspPacket* assemblePacketJpeg(uint8_t  *data,
                                           uint8_t  &qfactor,
                                           uint32_t &len,
                                           uint32_t &maxTcpPayload,
                                           uint32_t &packetNum,
                                           uint16_t &sequenceNum,
                                           uint32_t &timeStamp,
                                           uint32_t &ssrc,
                                           int32_t  &timeStampInc)
    {return NULL;}

    virtual RtspPacket* assemblePacketAdts(uint8_t  *data,
                                           uint32_t &len,
                                           uint32_t &inPacketNum,
                                           uint32_t &outPacketNum,
                                           uint16_t &sequenceNum,
                                           uint32_t &timeStamp,
                                           uint32_t &ssrc,
                                           uint32_t &timeStampInc)
    {return NULL;}

    virtual RtspPacket* assemblePacketOpus(uint8_t  *data,
                                           uint32_t &len,
                                           uint32_t &inPacketNum,
                                           uint32_t &outPacketNum,
                                           uint16_t &sequenceNum,
                                           uint32_t &timeStamp,
                                           uint32_t &ssrc,
                                           uint32_t &timeStampInc)
    {return NULL;}

    virtual RtspPacket* assemblePacketG726(uint8_t  *data,
                                           uint32_t &len,
                                           uint32_t &inPacketNum,
                                           uint32_t &outPacketNum,
                                           uint16_t &sequenceNum,
                                           uint32_t &timeStamp,
                                           uint32_t &ssrc,
                                           uint32_t &timeStampInc)
    {return NULL;}

  protected:
    uint8_t    *mBuffer;
    RtspPacket *mPacket;
    uint32_t    mBufMaxSize;
    uint32_t    mPktMaxNum;
};

#endif /* RTP_PACKAGER_H_ */
