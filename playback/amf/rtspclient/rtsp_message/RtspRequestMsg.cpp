#if NEW_RTSP_CLIENT

#include <stdio.h>
#include "RtspRequestMsg.h"
RtspRequestMsg::RtspRequestMsg(const string & msg):RtspMessage(msg){}
int RtspRequestMsg::decode()
{
    if(decode_self()==-1)
        return -1;

    char method_buf[32]={0};
    char url_buf[512]={0};
    if(sscanf(get_raw_buf().c_str(),"%31s%511s",method_buf,url_buf)<2){
        //cannot find url in first line!
        return -1;
    }

    if((method_=RtspUtil::getMethodInNumber(method_buf))==RTSP_UNKNOWN_MTHD){
        //cannot find correct method in msg!
        return -1;
    }

    url_.assign(url_buf);

    sessionId_=get_header(RTSP_SESSION_HDR);
    range_=get_header(RTSP_RANGE_HDR);

    return 0;
}

const string  & RtspRequestMsg::encode()
{
    set_raw_buf(RtspUtil::getMethodInStr(method_)+" "+url_+ " "+RtspUtil::getVersion()+"\r\n"+encode_self());
    return get_raw_buf();
}

#endif //NEW_RTSP_CLIENT

