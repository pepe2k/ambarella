#if NEW_RTSP_CLIENT

#include <stdio.h>
#include "RtpSession.h"

RtpSession::RtpSession(IRtpCallback *cb,FormatContext *context,unsigned int rtsp_id,RtpType type)
    :IRtpSession(type),cb_(cb),rtsp_id_(rtsp_id),context_(*context)
{}
RtpSession::~RtpSession(){
    //printf("RtpSession::~RtpSession() called\n");
}

void RtpSession::on_parse_init(){
    demuxer_.rtp_parse_open(this, &context_);
}
void RtpSession::on_parse_close(){
    demuxer_.rtp_parse_close();
}
void RtpSession::parse_data(void **buf,int len){
    demuxer_.rtp_parse_packet(cb_,(uint8_t**)buf,len);
}

#endif //NEW_RTSP_CLIENT

