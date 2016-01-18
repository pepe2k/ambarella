#ifndef _IRTSP_SESSION_H_
#define _IRTSP_SESSION_H_

#if NEW_RTSP_CLIENT

#include "RtspTcpAgent.h"
class IRtspSession
{
public:
    IRtspSession(unsigned int id=0):id_(id){}
    void connect(const std::string &ip,int port){id_=RtspTcpAgent::instance()->connect(ip, port);}
    virtual void connect_completed(bool result){}
    void close_connection(){RtspTcpAgent::instance()->close(id_);}
    void send_message(const std::string &msg){	RtspTcpAgent::instance()->send_message(id_,msg);}
    void id(unsigned int id){id_=id;}
    unsigned int id()const{return id_;}
    virtual ~IRtspSession(){}
private:
    unsigned int id_;
};

#endif //NEW_RTSP_CLIENT
#endif


