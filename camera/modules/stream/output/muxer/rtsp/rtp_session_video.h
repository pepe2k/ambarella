/*******************************************************************************
 * rtp_session_video.h
 *
 * History:
 *   2012-11-14 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef RTP_SESSION_VIDEO_H_
#define RTP_SESSION_VIDEO_H_

class CRtpSessionVideo: public CRtpSession
{
    typedef CRtpSession inherited;
    enum NALU_TYPE {
      NALUHEAD = 0x01,
      IDRHEAD  = 0x05,
      SEIHEAD  = 0x06,
      SPSHEAD  = 0x07,
      PPSHEAD  = 0x08,
      AUDHEAD  = 0x09,
    };

  public:
    static CRtpSessionVideo* Create(AM_UINT        streamid,
                                    AM_UINT        oldStreamId,
                                    CRtspServer   *server,
                                    AM_VIDEO_INFO *videoInfo);

  public:
    virtual void GetSdp(char* buf, AM_UINT size,
                        struct sockaddr_in& clientAddr,
                        AM_U16 clientPort = 0);

  private:
    virtual RtpSessionError SendData(CPacket* packet);

  private:
    CRtpSessionVideo(AM_UINT streamid, AM_UINT oldstreamid);
    virtual ~CRtpSessionVideo();
    AM_ERR Construct(CRtspServer *server, AM_VIDEO_INFO *videoInfo);
    const char* GetVideoFps(AM_U32& fps);
    const char* GetVideoFps(AM_U32& rate, AM_U32& scale,
                            AM_U16& mul,  AM_U16& div, AM_U32& fps);

  private:
    AM_VIDEO_INFO mVideoInfo;
    char mFrameRateStr[16];
};

#endif /* RTP_SESSION_VIDEO_H_ */
