#if NEW_RTSP_CLIENT

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "RtspTcpConnection.h"
#include "RtspTcpService.h"
#include "RtspTcpAgent.h"
#include "misc_utils.h"

int RtspTcpConnection::mInterval=XCONN_INTERVAL;
unsigned int RtspTcpConnection::mark=1;

RtspTcpConnection::RtspTcpConnection():mState(XCONN_IDLE),id_(mark++){
}

void RtspTcpConnection::set_peer_addr(const std::string &ip,int port){
    memset(&peer_addr_,0,sizeof(struct sockaddr_in ));
    peer_addr_.sin_family=AF_INET;
    peer_addr_.sin_port=htons(port);
    if(inet_pton(AF_INET,ip.c_str(),&peer_addr_.sin_addr)< 0){
        KILL_SELF();
    }
}

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

void RtspTcpConnection::connect(){
    if((mSockfd=socket(AF_INET,SOCK_STREAM,0)) < 0){
        KILL_SELF();
    }

    int flags = do_fcntl(mSockfd,F_GETFL,0);
    do_fcntl(mSockfd,F_SETFL,flags|O_NONBLOCK);

    int result=::connect(mSockfd,(struct sockaddr *)&peer_addr_,sizeof(struct sockaddr_in));
    mState=XCONN_CONNECTING;
    int res;
    if(result!=-1){
        res=RtspTcpService::instance()->write_pipe((int)this,XPIPE_CONNECT_SUCC);
        if(res==-1){
            connect_completed(XCONNECT_FAIL);
        }
    }else if(errno==EINPROGRESS){
        res=RtspTcpService::instance()->write_pipe((int)this,XPIPE_CONNECTING);
        if(res==-1){
            connect_completed(XCONNECT_FAIL);
        }
    }else{
        connect_completed(XCONNECT_FAIL);
    }
}

void RtspTcpConnection::connect_completed(XConnectResult state){
    if(state==XCONNECT_SUCC){
        mState=XCONN_CONNECTED;
        RtspTcpAgent::instance()->connect_completed(this, true);
    }else if(state==XCONNECT_FAIL){
        delete this;
    }
}

int RtspTcpConnection::read(){
    int len;
    char *ptrBuffer=mBuffer.base();
    while( (len=::recv(mSockfd,mBuffer.top(),mBuffer.capacity(),0)) < 0 && errno == EINTR);
    if(len<= 0){
        return -1;
    }
    mBuffer.add_loc(len);

    char  *begin=NULL,*end=NULL;
    int lenbody=0;
    do{
        if((begin=strcasestr(ptrBuffer,"\nContent-Length"))!=NULL){
            if((end=strstr(begin,"\r\n"))!=NULL){
                sscanf(begin+15,"%*[: ]%d",&lenbody);
                if((begin=strstr(end,"\r\n\r\n"))!=NULL){
                    if(mBuffer.top()>=begin+4+lenbody){
                        complete_read_msg_(ptrBuffer,begin+4+lenbody-ptrBuffer);
                        if(mBuffer.top()>begin+4+lenbody){
                            memmove(ptrBuffer,begin+4+lenbody,mBuffer.top()-begin-4-lenbody);
                            mBuffer.reset_loc(mBuffer.top()-begin-4-lenbody);
                        }else{
                            mBuffer.reset_loc(0);
                        }
                    }
                    break;
                }
            }
        }else if((end=strstr(ptrBuffer,"\r\n\r\n"))!=NULL){
            complete_read_msg_(ptrBuffer,end+4-ptrBuffer);
            if(mBuffer.top()>end+4){
                memmove(ptrBuffer,end+4,mBuffer.top()-end-4);
                mBuffer.reset_loc(mBuffer.top()-end-4);
            }else{
                mBuffer.reset_loc(0);
            }
        }else break;
    }while(1);

    if(mBuffer.capacity()<=0)	{
        mBuffer.add_capacity();
    }
    return 0;
}

int RtspTcpConnection::write(const std::string &msg,bool isSnyc){
    int count=0;
    int leftlen=msg.length();
    const char * ptr=msg.c_str();
    while(leftlen>0)	{
        while((count=::send(mSockfd,ptr,leftlen,0))<0&&(errno==EINTR || errno == EAGAIN || errno == EWOULDBLOCK));
        if(count<0){
            close_(isSnyc);
            return -1;
        }
        leftlen-=count;
        ptr+=count;
    }
    return 0;
}

void RtspTcpConnection::close(){
    RtspTcpService::instance()->write_pipe(id_,XPIPE_DELETE);
}

void RtspTcpConnection::close_(bool isSnyc){
    if(isSnyc){
        RtspTcpService::instance()->close_xconn(this);
    }else{
        RtspTcpService::instance()->write_pipe(id_,XPIPE_DELETE);
    }
}

void RtspTcpConnection::dump(){
}

void RtspTcpConnection::complete_read_msg_(char * base,int len){
    RtspTcpAgent::instance()->receive_message(this,base,len);
}

RtspTcpConnection::~RtspTcpConnection(){
    ::close(mSockfd);
    switch(mState){
    case XCONN_CONNECTING:
        RtspTcpAgent::instance()->connect_completed(this, false);
        break;
    default:
        RtspTcpAgent::instance()->peer_close(this);
        break;
    }
}

#endif//NEW_RTSP_CLIENT

