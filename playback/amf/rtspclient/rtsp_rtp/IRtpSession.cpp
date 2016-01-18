#if NEW_RTSP_CLIENT

#include <stdio.h>
#include <unistd.h>
#include <sys/poll.h>
#include "IRtpSession.h"
extern "C"{
   #include "libavformat/avformat.h"
   #include "libavformat/rtpdec.h"
};

#include "misc_utils.h"

/*
static int do_fcntl(int fd, int cmd, long arg){
    int result;
    do{
        result = fcntl(fd,cmd,arg);
        if(result < 0){
            if(errno != EINTR){
                KILL_SELF();
            }
        }else{
            break;
        }
    }while(1);
    return result;
}
*/

unsigned int IRtpSession::low_port_ = IRtpSession::BASE_RTP_PORT;
unsigned int IRtpSession::high_port_ = IRtpSession::TOP_RTP_PORT;
unsigned int IRtpSession::mark_port_ = IRtpSession::BASE_RTP_PORT;
unsigned int IRtpSession::mark_=0;

void IRtpSession::set_port_range(unsigned int low,unsigned int high){
    low_port_=low;
    high_port_=high;
    mark_port_=(low_port_%2==0?low_port_:low_port_+1);
}

IRtpSession::IRtpSession(RtpType type):rtp_type_(type),id_(mark_++),buf_(NULL){
    if((sockfd_=socket(AF_INET,SOCK_DGRAM,0)) < 0){
        KILL_SELF();
    }
    //int flags = do_fcntl(sockfd_,F_GETFL,0);
    //do_fcntl(sockfd_,F_SETFL,flags|O_NONBLOCK);

    if(rtp_type_ == RTP_NORMAL){
        int  tmp = 4*1024 * 1024;
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVBUFFORCE, &tmp, sizeof(tmp));

        socklen_t optlen = sizeof(tmp);
        getsockopt(sockfd_, SOL_SOCKET, SO_RCVBUF, (char*)&tmp, &optlen);
        printf("IRtpSession::IRtpSession --- SO_RCVBUF = %d\n", tmp);
    }

    if((sockRtcpfd_=socket(AF_INET,SOCK_DGRAM,0)) < 0){
        KILL_SELF();
    }

    //flags = do_fcntl(sockRtcpfd_,F_GETFL,0);
    //do_fcntl(sockRtcpfd_,F_SETFL,flags|O_NONBLOCK);

    char name[65];
    gethostname(name, sizeof(name));
    hostname_ = name;
}

IRtpSession::~IRtpSession(){
    close(sockfd_);
    close(sockRtcpfd_);
}

int IRtpSession::bind_port(){
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(struct sockaddr_in ));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);

    int num=0;
    do{
        if(num++>1000){
            return -1;
        }
        port_=mark_port_;
        mark_port_+=2;
        if(mark_port_>=high_port_){
            mark_port_=low_port_;
        }
        addr.sin_port=htons(port_);
    }while(bind(sockfd_,(struct sockaddr *)&addr,sizeof(struct sockaddr_in))==-1);

    addr.sin_port=htons(port_+1);
    bind(sockRtcpfd_,(struct sockaddr *)&addr,sizeof(struct sockaddr_in));
    return 0;
}

int IRtpSession::read_data(int sock_fd){
    struct sockaddr_in peer_addr;
    socklen_t addrlen=sizeof(struct sockaddr);
    int len;
    if(!buf_){
        buf_ = (char*)av_malloc(MAX_RTP_LEN);
    }
    while(1){
        len=recvfrom(sock_fd,buf_,MAX_RTP_LEN,0,(struct sockaddr *)&peer_addr,&addrlen);
        if(len<0){
            if(errno==EINTR){
                continue;
            }else if(errno==EAGAIN || errno == EWOULDBLOCK){
                break;
            }else{
                return -1;
            }
	 }
        parse_data((void**)&buf_,len);
        break;
    }
    return 0;
}

void IRtpSession::send_data(void *buf_,int len){
    struct sockaddr_in to;
    memset(&to,0,sizeof(struct sockaddr_in));
    to.sin_family  = AF_INET;
    to.sin_addr.s_addr  =inet_addr(peer_ip_.c_str());

    unsigned char *buf = (unsigned char *)buf_;
    int fd;
    if (buf[1] >= RTCP_SR && buf[1] <= RTCP_APP) {
        /* RTCP payload type */
        fd = sockRtcpfd_;
        //printf("RtpSession::send_data --- send_rtcp, [%p][%s:%d],len = %d\n",this,peer_ip_.c_str(),peer_port_base_+ 1,len);
        to.sin_port = htons(peer_port_base_ + 1);
    } else {
        /* RTP payload type */
        fd = sockfd_;
        to.sin_port = htons(peer_port_base_);
    }

    socklen_t addrlen=sizeof(struct sockaddr);
    sendto(fd,buf,len,0,(struct sockaddr *)&to,addrlen);
}

void IRtpSession::parse_init(){
    buf_ = NULL;
    on_parse_init();
}
void IRtpSession::parse_close(){
    on_parse_close();
    if(buf_){
        av_free(buf_);
        buf_ = NULL;
    }
}
#endif //NEW_RTSP_CLIENT

