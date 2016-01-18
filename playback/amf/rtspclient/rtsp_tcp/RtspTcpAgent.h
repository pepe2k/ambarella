#ifndef _RTSP_TCP_AGENT_H__
#define _RTSP_TCP_AGENT_H__

#if NEW_RTSP_CLIENT

#include <string>
#include <map>
#include <pthread.h>
#include "IXServer.h"

class RtspTcpAgent
{
public:
    int connect(const std::string &ip,int port);
    void connect_completed(void * XConn, bool result);
    void close(unsigned int id);
public:
    void peer_close(void *XConn);
public:
    static RtspTcpAgent * instance();
    void del_self() {delete this;}
    void receive_message(void * XConn,char * base,int len);
    void send_message(unsigned int id,const std::string &msg);
    void set_server(IXServer *server){server_=server;}
private:
    RtspTcpAgent():server_(NULL){}
    ~RtspTcpAgent();
    static RtspTcpAgent * instance_;
    static pthread_mutex_t mutex_;

private:
    IXServer * server_;
    std::map<int, void *> connIdentifers_;
};

#endif //NEW_RTSP_CLIENT
#endif //_RTSP_TCP_AGENT_H__
