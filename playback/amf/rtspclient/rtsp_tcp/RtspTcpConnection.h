#ifndef _RTSP_TCP_CONNECTION_H__
#define _RTSP_TCP_CONNECTION_H__

#if NEW_RTSP_CLIENT

#include <string>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "XTcpBuffer.h"

#define XCONN_INTERVAL 20000

class RtspTcpConnection
{
    friend class RtspTcpService;
public:
    typedef enum{
        XCONN_IDLE=0,
        XCONN_CONNECTING,
        XCONN_CONNECTED,
        XCONN_CLOSING
    }XConnState;
    typedef enum{
        XCONNECT_SUCC=0,
        XCONNECT_FAIL
    }XConnectResult;

    RtspTcpConnection();
    void connect();
    int write(const std::string &msg,bool isSnyc=true);
    void dump();
    void close();
    ~RtspTcpConnection();
public:
    void set_peer_addr(const std::string &ip,int port);
    int get_id(){return id_;}
    const struct sockaddr_in & get_peer_addr()const{return peer_addr_;}
protected:
    int read();
    void connect_completed(XConnectResult state);
private:
    void close_(bool isSnyc=true);
    void complete_read_msg_(char * base,int len);
private:
    int mSockfd;
    XConnState mState;
    struct sockaddr_in peer_addr_;
private:
    static unsigned int mark;
    unsigned int id_;
private:
    static int mInterval;
    XTcpBuffer mBuffer;
};

#endif //NEW_RTSP_CLIENT

#endif//_RTSP_TCP_CONNECTION_H__

