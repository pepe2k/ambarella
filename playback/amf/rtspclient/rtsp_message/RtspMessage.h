#ifndef _RTSP_MESSGE_H_
#define _RTSP_MESSGE_H_

#if NEW_RTSP_CLIENT

#include <string>
#include <string.h> //for memset
#include "RtspUtil.h"

using std::string;
typedef struct RtspHeaderValueData_t
{
    int offset;
    int len;
}HeaderValueData;

class RtspMessage
{
public:
    RtspMessage()
    {
        memset(mapHeaders_,0,sizeof(mapHeaders_));
    }
    RtspMessage(const string & msg):rawMsg_(msg)
    {
        memset(mapHeaders_,0,sizeof(mapHeaders_));
    }
    virtual ~RtspMessage(){}
    int decode_self();
    string encode_self();
    virtual int decode()=0;
    virtual const string &  encode()=0;

    void set_header(RtspHeadersType header,const string &value);
    void set_body(const string & body);

    const string & get_cseq() const{return cseq_;}
    string get_header(RtspHeadersType header) const;
    const string & get_body()const{return body_;}

    const string & get_raw_buf()const{return rawMsg_;}
    void set_raw_buf(const string & msg){rawMsg_=msg;}
private:
    string rawMsg_;
    HeaderValueData mapHeaders_[RTSP_UNKNOWN_HDR+1];

    string cseq_;
    string contentLen_;
    string body_;

    string headers_;
};

#endif//NEW_RTSP_CLIENT

#endif

