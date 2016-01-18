/*******************************************************************************
 * rtp_session_audio.h
 *
 * History:
 *   2012-11-19 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef RTP_SESSION_AUDIO_H_
#define RTP_SESSION_AUDIO_H_
class CAuPacket;

class CRtpSessionAudio: public CRtpSession
{
    typedef CRtpSession inherited;

  public:
    static CRtpSessionAudio* Create(AM_UINT        streamid,
                                    AM_UINT        oldstreamid,
                                    CRtspServer   *server,
                                    AM_AUDIO_INFO *audioInfo);

  public:
    virtual void GetSdp(char* buf, AM_UINT size,
                        struct sockaddr_in& clientAddr,
                        AM_U16 clientPort = 0);

  private:
    virtual RtpSessionError SendData(CPacket* packet);

  private:
    CRtpSessionAudio(AM_UINT streamid, AM_UINT oldstreamid);
    virtual ~CRtpSessionAudio();
    AM_ERR Construct(CRtspServer* server, AM_AUDIO_INFO* audioInfo);

  private:
    AM_AUDIO_INFO mAudioInfo;
    AM_U32        mPktPtsIncr;
};

#endif /* RTP_SESSION_AUDIO_H_ */
