/*******************************************************************************
 * rtp_packager_h264.h
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

#ifndef RTP_PACKAGER_H264_H_
#define RTP_PACKAGER_H264_H_

class CRtpPackager;
class CRtpPackagerH264: public CRtpPackager
{
    enum {
      max_nalu_number = 8
    };

  public:
    CRtpPackagerH264() :
      CRtpPackager(),
      mProfileLevelId(0),
      mSpsStr(NULL),
      mPpsStr(NULL){}

    virtual ~CRtpPackagerH264()
    {
      delete[] mSpsStr;
      delete[] mPpsStr;
      DEBUG("~CRtpPackagerH264");
    }

  public:
    virtual uint8_t* getSpsString();
    virtual uint8_t* getPpsString();
    virtual uint32_t getProfileLevelId();
    virtual RtspPacket* assemblePacketH264(uint8_t  *data,
                                               uint32_t &len,
                                               uint32_t &maxTcpPayload,
                                               uint32_t &packetNum,
                                               uint16_t &sequenceNum,
                                               uint32_t &timeStamp,
                                               uint32_t &ssrc,
                                               int32_t  &timeStampInc);

  private:
    uint32_t    mProfileLevelId;
    uint8_t    *mSpsStr;
    uint8_t    *mPpsStr;
};

#endif /* RTP_PACKAGER_H264_H_ */
