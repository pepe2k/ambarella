#if NEW_RTSP_CLIENT

#include <stdio.h>
#include <stdlib.h>
#include "IMessage.h"
#include "RtspUtil.h"

int IMessage::cseqMark_=1;
IMessage::IMessage(void * owner):
    owner_(owner),
    cseq_(INVALID_CSEQ),
    timer_(REQUEST_CSEQ_TIMER,NULL,(void *)owner_)
{
    //printf("IMessage::IMessage(void * owner)\n");
}
IMessage::IMessage():
    owner_(0),
    cseq_(INVALID_CSEQ),
    timer_(REQUEST_CSEQ_TIMER,NULL,(void *)owner_)
{
    //printf("IMessage::IMessage()\n");
}
void IMessage::decode_message(const std::string & msg)
{
    if(msg.find(RtspUtil::getVersion().c_str())==0)
    {
        RtspResponseMsg response(msg);
        response.decode();
        int cseq=atoi(response.get_cseq().c_str());
        if(cseq==cseq_)
        {
            cseq_=INVALID_CSEQ;
            timer_.stop();
            on_response(requestMethod_,response);
        }
    }else{
        RtspRequestMsg request(msg);
        request.decode();
        on_request(request);
    }
}
bool IMessage::check_timeout()
{
    if(cseq_!=INVALID_CSEQ)
    {
        cseq_=INVALID_CSEQ;
        on_request_timeout(requestMethod_);
    }
    return true;
}
const string & IMessage::encode_response(RtspResponseMsg * msg)
{
    return msg->encode();
}
const string & IMessage::encode_request(RtspRequestMsg * msg)
{
    cseq_=cseqMark_++;
    char buf[20];
    sprintf(buf,"%d",cseq_);
    msg->set_header(RTSP_CSEQ_HDR, buf);
    requestMethod_=msg->get_method();
    timer_.start();
    return msg->encode();
}

#endif //NEW_RTSP_CLIENT