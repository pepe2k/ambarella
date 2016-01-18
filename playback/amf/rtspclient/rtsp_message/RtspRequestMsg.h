#ifndef _RTSP_REQUEST_MSG_H_
#define _RTSP_REQUEST_MSG_H_

#if NEW_RTSP_CLIENT

#include "RtspMessage.h"
class RtspRequestMsg:public RtspMessage
{
public:
    RtspRequestMsg(){}
    RtspRequestMsg(const string & msg);
    int decode();
    virtual const string &  encode() ;

    void set_method(RtspMethodsType method){method_=method;}
    void set_url(const string & url){url_=url;}

    RtspMethodsType get_method() const {return method_;}
    const string & get_url()const {return url_;}
    const string & get_session() const{return sessionId_;}
    const string & get_range() const {return range_;}
private:
    RtspMethodsType method_;

    string url_;
    string sessionId_;
    string range_;
};

#endif //NEW_RTSP_CLIENT
#endif

