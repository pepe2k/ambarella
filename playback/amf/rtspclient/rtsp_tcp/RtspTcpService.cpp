#if NEW_RTSP_CLIENT

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "RtspTcpService.h"
#include "RtspTcpAgent.h"
//
#include "misc_utils.h"

RtspTcpService *RtspTcpService::mInstance=NULL;
pthread_mutex_t RtspTcpService::m_mutex= PTHREAD_MUTEX_INITIALIZER;
RtspTcpService::RtspTcpService(){
    mXPoll=new XEpoll();
    pid_ = -1;
}

RtspTcpService::~RtspTcpService(){
    stop_service();
    if(mXPoll){
        delete mXPoll;
        mXPoll = NULL;
    }
    //printf("RtspTcpService::~RtspTcpService() called\n");
    pthread_mutex_lock(&m_mutex);
    mInstance = NULL;
    pthread_mutex_unlock(&m_mutex);
}

RtspTcpService *RtspTcpService::instance(){
    if(mInstance==NULL){
        pthread_mutex_lock(&m_mutex);
        if(mInstance == NULL){
            mInstance=new RtspTcpService();
        }
        pthread_mutex_unlock(&m_mutex);
    }
    return mInstance;
}

int RtspTcpService::write_pipe(int xconn,XPipeOperateType op){
    return mPipe.write(xconn,op);
}

void RtspTcpService::close_xconn(RtspTcpConnection *xconn){
    delete_xconn_(xconn);
}

#include "pthread_utils.h"
void RtspTcpService::start_service(IXServer *server,int connlive){
    RtspTcpAgent::instance()->set_server(server);
    RtspTcpConnection::mInterval=connlive;
    pthread_create_rr(&pid_,process,this,50,0);
}

void RtspTcpService::stop_service(){
    if(pid_ != (unsigned int)-1){
	mPipe.write(1,XPIPE_STOP_SERVICE);
       pthread_join(pid_,NULL);
       pid_ = -1;
       RtspTcpAgent::instance()->del_self();
    }
}
void* RtspTcpService::process(void *arg){
    RtspTcpService *service=(RtspTcpService *)arg;

    signal(SIGPIPE, SIG_IGN);
    //int flags;

    int pipe_fd_0 = service->mPipe.get_fd0();
    //int pipe_fd_1 = service->mPipe.get_fd1();

    if(service->mXPoll->init() < 0){
        KILL_SELF();
    }

    XPollData data;
    data.data=&pipe_fd_0;
    data.event=XPOLLIN;
    if(service->mXPoll->add_data(pipe_fd_0, &data) < 0){
        KILL_SELF();
    }

    XPipeOperateType opFromPipe;
    int objFromPipe;
    RtspTcpConnection * objXConn;

    void *result;
    int res;
    while(1){
        while((res=service->mXPoll->wait()) < 0 && errno == EINTR);
        if(res < 0 && errno != EINTR) KILL_SELF();

        while((result=service->mXPoll->next_result())!=(void *)-1){
            if(service->mXPoll->get_data(result)==&pipe_fd_0){
                if(service->mPipe.read(pipe_fd_0)<0){
                    return 0;
                }
                while((res=service->mPipe.parse(&objFromPipe, &opFromPipe))!=-1){
                    if(opFromPipe==XPIPE_CONNECTING){
                        service->add_xconn_((RtspTcpConnection *)objFromPipe, XPOLLOUT);
                    }else if(opFromPipe==XPIPE_CONNECT_SUCC){
                        service->add_xconn_((RtspTcpConnection *)objFromPipe, XPOLLIN);
                        ((RtspTcpConnection *)objFromPipe)->connect_completed(RtspTcpConnection::XCONNECT_SUCC);
                    }else if(opFromPipe==XPIPE_DELETE)	{
                        int deleteobj;
                        if(service->delete_xconn_(objFromPipe,&deleteobj)==0){
                            service->mXPoll->delete_from_results((void *)deleteobj);
                        }
                    }else if(opFromPipe==XPIPE_STOP_SERVICE){
                        return 0;
                    }
                    if(res==0)
                        break;
                }
                data.data=&pipe_fd_0;
                data.event=XPOLLIN;
                if(service->mXPoll->reset_data(pipe_fd_0, &data) < 0){
                    KILL_SELF();
                }
            }else{
                objXConn=(RtspTcpConnection *)service->mXPoll->get_data(result);
                if(service->mXPoll->get_event(result) & (XPOLLHUP | XPOLLERR)){
                    service->delete_xconn_(objXConn);
                }else if(service->mXPoll->get_event(result) & XPOLLOUT){
                    long val;
                    socklen_t len=sizeof(val);
                    getsockopt(objXConn->mSockfd,SOL_SOCKET,SO_ERROR,(char *)&val,&len);
                    if(val>=0){
                        data.event=XPOLLIN;
                        data.data=objXConn;
                        if(service->mXPoll->change_data(objXConn->mSockfd, &data) < 0){
                            KILL_SELF();
                        }
                        objXConn->connect_completed(RtspTcpConnection::XCONNECT_SUCC);
                    }else{
                        service->delete_xconn_(objXConn);
                    }
                }else if(service->mXPoll->get_event(result) & XPOLLIN){
                    data.data=objXConn;
                    data.event=XPOLLIN;
                    if(service->mXPoll->reset_data(objXConn->mSockfd, &data)< 0){
                        KILL_SELF();
                    }
                    if(objXConn->read()==-1){
                        service->delete_xconn_(objXConn);
                    }
                }
            }
        }
    }
    return (void*)NULL;
}

void RtspTcpService::add_xconn_(RtspTcpConnection *xconn, int event){
    XPollData data;
    data.data=xconn;
    data.event=event;
    if(mXPoll->add_data(xconn->mSockfd, &data) < 0){
        KILL_SELF();
    }
    mapXConn[xconn->id_]=xconn;
}
int RtspTcpService::delete_xconn_(unsigned int id,int * obj){
    RtspTcpConnection *xconn;
    if(mapXConn.find(id)!=mapXConn.end()){
        xconn=mapXConn[id];
        *obj=(int)xconn;
        XPollData data;
        data.data=xconn;
        data.event=0;
        if(mXPoll->delete_data(xconn->mSockfd, &data) < 0){
            KILL_SELF();
        }
        mapXConn.erase(id);
        delete(xconn);
        return 0;
    }
    return -1;
}
void RtspTcpService::delete_xconn_(RtspTcpConnection *xconn){
    XPollData data;
    data.data=xconn;
    data.event=0;
    if(mXPoll->delete_data(xconn->mSockfd, &data) < 0){
        KILL_SELF();
    }
    mapXConn.erase(xconn->id_);
    delete(xconn);
}

#endif//NEW_RTSP_CLIENT
