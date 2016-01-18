#if NEW_RTSP_CLIENT

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include<signal.h>
#include "RtpService.h"
#include "RtpManager.h"
#include "IRtpSession.h"
#include "misc_utils.h"

RtpService::RtpService(){
    mXPoll=new XEpoll();
    pid_ = -1;
}

RtpService::~RtpService(){
    //printf("RtpService::~RtpService() called\n");
    stop();
    if(mXPoll){
        delete mXPoll;
        mXPoll = NULL;
    }
    //printf("RtpService::~RtpService()  END\n");
}

#include "pthread_utils.h"
void RtpService::start(int cpu_id){
    if (pid_ == -1U){
        pthread_create_rr(&pid_,process,this,90,cpu_id);
    }
}

void RtpService::stop(){
    if(pid_ != -1U){
       mPipe.write(1,XPIPE_STOP_SERVICE);
       pthread_join(pid_,NULL);
       pid_ = -1;
    }
}

void RtpService::add_session(IRtpSession * session){
    mPipe.write((int)session,XPIPE_CONNECT_SUCC);
}

void RtpService::del_session(IRtpSession * session){
    future_ = new Future<int>;
    mPipe.write((int)session,XPIPE_DELETE);
    future_->get();
    delete future_;
    //printf("RtpService::del_session() called\n");
}

void *RtpService::process(void *arg){
    RtpService *service=(RtpService *)arg;

    signal(SIGPIPE, SIG_IGN);

    if(service->mXPoll->init() < 0){
        KILL_SELF();
    }

    int pipe_fd0 = service->mPipe.get_fd0();
    // int pipe_fd1 = service->mPipe.get_fd1();
    XPollData data;
    data.data=&pipe_fd0;
    data.event=XPOLLIN;
    if(service->mXPoll->add_data(pipe_fd0, &data) < 0){
        KILL_SELF();
    }

    /*varaiant for read from pipe*/
    XPipeOperateType opFromPipe;
    int objFromPipe;
    RtpSessionInfo * obj;

    /*service main thread start*/
    void *result;
    int res;
    while(1){
        while((res=service->mXPoll->wait()) < 0 && errno == EINTR);
        if(res < 0 && errno != EINTR) KILL_SELF();

        while((result=service->mXPoll->next_result())!=(void *)-1)	{
            if(service->mXPoll->get_data(result)==&pipe_fd0){
                if(service->mPipe.read(pipe_fd0)<0){
                    return (void*)NULL;
                }
                while((res=service->mPipe.parse(&objFromPipe, &opFromPipe))!=-1){
                    if(opFromPipe==XPIPE_CONNECT_SUCC){
                        service->add_session_((IRtpSession *)objFromPipe, XPOLLIN);
                    }else if(opFromPipe==XPIPE_DELETE){
                        //printf("RtpService::process() DELETE ---1\n");
                        service->delete_session_((IRtpSession *)objFromPipe);
                        //printf("RtpService::process() DELETE ---2\n");
                        service->mXPoll->delete_from_results((IRtpSession *)objFromPipe);
                        //printf("RtpService::process() DELETE ---3\n");
                        int value = 0;
                        service->future_->set(value);
                    }else if(opFromPipe==XPIPE_STOP_SERVICE){
                        return (void*)NULL;
                    }
                    if(res==0){
                        break;
                    }
                }
                data.data=&pipe_fd0;
                data.event=XPOLLIN;
                if(service->mXPoll->reset_data(pipe_fd0, &data) < 0){
                    KILL_SELF();
                }
            }else	{
                obj=(RtpSessionInfo *)service->mXPoll->get_data(result);
                if(service->mXPoll->get_event(result) & (XPOLLHUP | XPOLLERR)){
                    service->delete_session_(obj->session);
                }else if(service->mXPoll->get_event(result) & XPOLLIN){
                    data.data=obj;
                    data.event=XPOLLIN;
		      if(service->mXPoll->reset_data(obj->fd, &data) < 0){
                        KILL_SELF();
		      }
                    if(service->mapSession.find(obj->fd) != service->mapSession.end()){
                        if(obj->session->read_data(obj->fd)==-1){
                            service->delete_session_(obj->session);
                        }
                    }else{
                        printf("RtpService::process() fd --session has been deleted\n");
                    }
                }
            }
        }
    }

    return (void*)NULL;
}

void RtpService::add_session_(IRtpSession *session, int event){
    session->parse_init();

    RtpSessionInfo *rtp_info = new RtpSessionInfo;
    rtp_info->session  = session;
    rtp_info->fd = session->sockfd();

    RtpSessionInfo *rtcp_info = new RtpSessionInfo;
    rtcp_info->session  = session;
    rtcp_info->fd = session->sockfd_rtcp();

    XPollData data;
    data.data=rtp_info;
    data.event=event;
    if(mXPoll->add_data(session->sockfd(), &data) < 0){
        KILL_SELF();
    }
    mapSession[session->sockfd()]=rtp_info;

    XPollData data2;
    data2.data=rtcp_info;
    data2.event=event;
    if(mXPoll->add_data(session->sockfd_rtcp(), &data2) < 0){
        KILL_SELF();
    }
    mapSession[session->sockfd_rtcp()]=rtcp_info;
}

void RtpService::delete_session_(IRtpSession *session){
    if(mapSession.find(session->sockfd()) != mapSession.end()){
        RtpSessionInfo *rtp_info = mapSession[session->sockfd()];
        XPollData data;
        data.data=rtp_info;
        data.event=0;
        if(mXPoll->delete_data(session->sockfd(), &data) < 0){
            KILL_SELF();
        }
        mapSession.erase(session->sockfd());
        delete rtp_info;
    }

    if(mapSession.find(session->sockfd_rtcp()) != mapSession.end()){
        RtpSessionInfo *rtcp_info = mapSession[session->sockfd_rtcp()];
        XPollData data;
        data.data=rtcp_info;
        data.event=0;
        if(mXPoll->delete_data(session->sockfd_rtcp(), &data) < 0){
            KILL_SELF();
        }
        mapSession.erase(session->sockfd_rtcp());
        delete rtcp_info;
    }

    session->parse_close();
}

#endif //NEW_RTSP_CLIENT


