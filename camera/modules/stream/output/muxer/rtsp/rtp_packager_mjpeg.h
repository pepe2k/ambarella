/*******************************************************************************
 * rtp_packager_mjpeg.h
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

#ifndef RTP_PACKAGER_MJPEG_H_
#define RTP_PACKAGER_MJPEG_H_

struct JpegParams;
class CRtpPackager;
class CRtpPackagerMjpeg: public CRtpPackager
{
  public:
    CRtpPackagerMjpeg();
    virtual ~CRtpPackagerMjpeg();

  public:
    virtual RtspPacket* assemblePacketJpeg(uint8_t  *data,
                                           uint8_t  &qfactor,
                                           uint32_t &len,
                                           uint32_t &maxTcpPayload,
                                           uint32_t &packetNum,
                                           uint16_t &sequenceNum,
                                           uint32_t &timeStamp,
                                           uint32_t &ssrc,
                                           int32_t  &timeStampInc);
  private:
    JpegParams *mJpegParam;
};


#endif /* RTP_PACKAGER_MJPEG_H_ */
