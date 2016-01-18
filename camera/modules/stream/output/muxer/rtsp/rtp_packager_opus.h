/*******************************************************************************
 * rtp_packager_opus.h
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

#ifndef RTP_PACKAGER_OPUS_H_
#define RTP_PACKAGER_OPUS_H_

class CRtpPackager;
class CRtpPackagerOpus: public CRtpPackager
{
  public:
    CRtpPackagerOpus() :
      CRtpPackager(){}
    virtual ~CRtpPackagerOpus()
    {
      DEBUG("~CRtpPackagerOpus");
    }

  public:
    virtual RtspPacket* assemblePacketOpus(uint8_t  *data,
                                           uint32_t &len,
                                           uint32_t &inPacketNum,
                                           uint32_t &outPacketNum,
                                           uint16_t &sequenceNum,
                                           uint32_t &timeStamp,
                                           uint32_t &ssrc,
                                           uint32_t &timeStampInc);
};

#endif /* RTP_PACKAGER_OPUS_H_ */
