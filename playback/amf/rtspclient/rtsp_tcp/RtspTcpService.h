#ifndef _RTSP_TCP_SERVICE_H__
#define _RTSP_TCP_SERVICE_H__

#if NEW_RTSP_CLIENT

#include <vector>
#include <map>
#include <pthread.h>
#include "IXServer.h"
#include "RtspTcpConnection.h"
#include "xpipe.h"
#include "xpoll_epoll.h"

class RtspTcpService
{
public:
    static RtspTcpService *instance();
    void del_self() {delete this;}
    void start_service(IXServer *server,int connlive=XCONN_INTERVAL);
    void stop_service();
    int write_pipe(int xconn,XPipeOperateType op);
    void close_xconn(RtspTcpConnection *xconn);
protected:
    static void* process(void *arg);
private:
    void add_xconn_(RtspTcpConnection *xconn, int event);
    int delete_xconn_(unsigned int id,int * obj);
    void delete_xconn_(RtspTcpConnection *xconn);
private:
    RtspTcpService();
    ~RtspTcpService();
    static RtspTcpService *mInstance;
    static pthread_mutex_t m_mutex;
private:
    XEpoll *mXPoll;
    XPipe mPipe;
    std::map<unsigned int,RtspTcpConnection * > mapXConn;
    pthread_t pid_;
};

#endif//NEW_RTSP_CLIENT

#endif //_RTSP_TCP_SERVICE_H__

