#ifndef _I_MSSAGE_H_
#define _I_MSSAGE_H_

#if NEW_RTSP_CLIENT

#include <string>
#include "am_timer.h"
#include "RtspMessage.h"
#include "RtspResponseMsg.h"
#include "RtspRequestMsg.h"

#define REQUEST_CSEQ_TIMER 10000
#define INVALID_CSEQ 0

class IMessage{
public:
    IMessage(void * owner);
    IMessage();
    virtual ~IMessage(){}
    void decode_message(const std::string & msg);
    virtual void on_request(const RtspRequestMsg & msg)=0;
    virtual void on_response(RtspMethodsType method,const RtspResponseMsg & msg)=0;
    virtual void on_request_timeout(RtspMethodsType method)=0;
public:
    void set_owner(void * owner){owner_=owner;}
    const std::string & encode_response(RtspResponseMsg * msg);
    const std::string & encode_request(RtspRequestMsg * msg);
    void set_timer_param(timer_vfunc vfunc,void *vdata) {timer_.set_func_data(vfunc,vdata);}
    bool check_timeout();
private:
    static int cseqMark_;
    void * owner_;
    int cseq_;
    RtspMethodsType requestMethod_;
    CTimer  timer_;
};

#endif//NEW_RTSP_CLIENT

#endif

