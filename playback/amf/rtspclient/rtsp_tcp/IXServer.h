
#ifndef _X_SEVER_H__
#define _X_SEVER_H__

#if NEW_RTSP_CLIENT

class IXServer
{
public:
    virtual ~IXServer(){}
    virtual void connect_completed(unsigned int id, bool result)=0;
    virtual void peer_close(unsigned int id)=0;
    virtual void receive_message(unsigned int id,char * base,int len)=0;
};
#endif //NEW_RTSP_CLIENT

#endif

