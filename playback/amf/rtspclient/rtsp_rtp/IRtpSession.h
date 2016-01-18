#ifndef _I_RTP_SESSION_H_
#define _I_RTP_SESSION_H_

#if NEW_RTSP_CLIENT

//TODO,
//RTP_PACKET mode has not been implemented now.


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include<signal.h>
#include <string>

#include <linux/if_packet.h>

enum RtpType{ RTP_NORMAL, RTP_PACKET};

class IRtpSession
{
public:
    IRtpSession(RtpType = RTP_NORMAL);
    virtual ~IRtpSession();
    void set_owner(void *owner){owner_=owner;}
    void *get_owner(){return owner_;}
    std::string &get_hostname() { return hostname_;}
    virtual unsigned int get_local_port()const {return port_;}
    void set_peer_addr(const std::string &ip, int port_base){peer_ip_ = ip; peer_port_base_ = port_base;}
    int sockfd() const { return sockfd_;}
    int sockfd_rtcp()const {return sockRtcpfd_;}
    unsigned int id()const {return id_;}
    virtual int bind_port();
    int read_data(int sock_fd);
    void parse_init();
    void parse_close();
    virtual void on_parse_init() = 0;
    virtual void on_parse_close() = 0;
    virtual void parse_data(void **buf, int len) = 0;
    virtual void send_data(void *buf,int len);
public:
    enum {BASE_RTP_PORT = 30000,TOP_RTP_PORT = 62000,MAX_RTP_LEN = 1536};
    static void set_port_range(unsigned int low,unsigned int high);
protected:
    static unsigned int mark_port_;
    static  unsigned int low_port_;
    static  unsigned int high_port_;
    static unsigned int mark_;
    RtpType rtp_type_;

    void * owner_;
    unsigned int id_;
    unsigned int port_;

    int sockfd_;
    int sockRtcpfd_;

    //char buf_[MAX_RTP_LEN];
    char *buf_;

    //
    std::string hostname_;
    std::string peer_ip_;
    int peer_port_base_;
};
#endif //NEW_RTSP_CLIENT

#endif //_I_RTP_SESSION_H_


