#if NEW_RTSP_CLIENT

#include <stdio.h>
#include <stdlib.h>
#include "RtspMessage.h"

void RtspMessage::set_header(RtspHeadersType header,const std::string &value)
{
    headers_.append(RtspUtil::getHeaderInStr(header)+": "+value+"\r\n");
}
void RtspMessage::set_body(const string & body)
{
    char buf[20];
    body_=body;
    sprintf(buf,"%d",body_.length());
    set_header(RTSP_CONTENT_LENGTH_HDR,buf);
}

std::string RtspMessage::get_header(RtspHeadersType header) const
{
    if(mapHeaders_[header].len!=0)
        return rawMsg_.substr(mapHeaders_[header].offset,mapHeaders_[header].len);
    return "";
}

int RtspMessage::decode_self()
{
    int begin,end;
    HeaderValueData headerValue;
    if((end=rawMsg_.find("\r\n")) == (int)string::npos){
        return -1;//cannot find /r/n in msg
    }
    size_t p,l;
    while(1)
    {
        begin=end+2;
        if((end=rawMsg_.find("\r\n",begin))==(int)string::npos||end==begin)
            break;
        p=begin;
        while((int)p<end&&rawMsg_[p]!=' '&&rawMsg_[p]!=':')
            p++;
        if((int)p==end)
            continue;
        l=p;
        while(rawMsg_[p]==' '||rawMsg_[p]==':')
	    p++;
        headerValue.offset=p;
        headerValue.len=end-p;
        mapHeaders_[RtspUtil::getHeaderInNumber(rawMsg_.substr(begin,l-begin).c_str())]=headerValue;
    }
    cseq_=get_header(RTSP_CSEQ_HDR);
    contentLen_=get_header(RTSP_CONTENT_LENGTH_HDR);

    if(contentLen_!="")
    {
        int len=atoi(contentLen_.c_str());
        if((begin=rawMsg_.find("\r\n",end))==(int)string::npos||begin!=end){
            return -1;//cannot find '/r/n/r/n' when having content_len!
        }
        if(begin+2+len>(int)rawMsg_.length()){
            return -1;//body is shorter than content_len
        }
        body_=rawMsg_.substr(begin+2,len);
    }
    return 0;
}
std::string RtspMessage::encode_self()
{
    rawMsg_.append(headers_+"\r\n");
    if(body_!="")
    {
         rawMsg_.append(body_);
    }
    return rawMsg_;
}

#endif //NEW_RTSP_CLIENT

