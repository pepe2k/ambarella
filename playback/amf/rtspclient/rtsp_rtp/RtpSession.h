#ifndef _RTP_SESSION_H_
#define _RTP_SESSION_H_

#if NEW_RTSP_CLIENT

#include <string>
#include "IRtpSession.h"
#include "RtpDemuxer.h"

class RtpSession:public IRtpSession
{
public:
    RtpSession(IRtpCallback *cb,FormatContext *context,unsigned int rtsp_id,RtpType type);
    virtual ~RtpSession();
    virtual void on_parse_init();
    virtual void on_parse_close();
    virtual void parse_data(void **buf, int len);
private:
    IRtpCallback *cb_;
    unsigned int rtsp_id_;
    RTPDemuxer demuxer_;
    FormatContext context_;
};

#endif //NEW_RTSP_CLIENT

#endif //_RTP_SESSION_H_


