#ifndef _RTSP_RESPONSE_MSG_H_
#define _RTSP_RESPONSE_MSG_H_

#if NEW_RTSP_CLIENT

#include "RtspMessage.h"

class RtspResponseMsg:public RtspMessage
{
public:
    RtspResponseMsg(){}
    RtspResponseMsg(const string & msg):RtspMessage(msg){}
    virtual int decode();
    virtual const string & encode();
    void set_code(RtspStatusCodesType code,const string & status=""){code_=code;status_=status;}
    RtspStatusCodesType get_code()const{return code_;}
private:
    RtspStatusCodesType code_;
    string status_;
};
#endif//NEW_RTSP_CLIENT
#endif

