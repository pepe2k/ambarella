#if NEW_RTSP_CLIENT

#include <string.h>
#include "xpoll_epoll.h"
int XEpoll::init(){
    epollfd_=epoll_create(XPOLL_MAX_EVENT);
    memset(&ev_,0,sizeof(ev_));
    return epollfd_;
}

int XEpoll::wait(){
    result_fds_=epoll_wait(epollfd_,result_events_,XPOLL_MAX_EVENT,-1);
    loc_=0;
    return result_fds_;
}

void * XEpoll::next_result(){
    if(loc_>=result_fds_)
        return (void *)-1;
    else
        return &(result_events_[loc_++]);
}

void XEpoll::delete_from_results(void * data){
    for(int i=result_fds_-1;i>=0;i--){
        if(result_events_[i].data.ptr==data){
            result_events_[i]=result_events_[result_fds_-1];
            result_fds_--;
        }
    }
}

void * XEpoll::get_data(void * event){
    struct epoll_event *pEv=(struct epoll_event *)event;
    return pEv->data.ptr;
}

int XEpoll::get_event(void * event){
    struct epoll_event *pEv=(struct epoll_event *)event;
    return pEv->events;
}

int XEpoll::add_data(int fd,XPollData * data){
    ev_.data.ptr=data->data;
    ev_.events=data->event;
    return epoll_ctl(epollfd_,EPOLL_CTL_ADD,fd,&ev_);
}

int XEpoll::delete_data(int fd,XPollData *data){
    ev_.data.ptr=data->data;
    ev_.events=data->event;
    return epoll_ctl(epollfd_,EPOLL_CTL_DEL,fd,&ev_);
}

int XEpoll::change_data(int fd,XPollData *data){
    ev_.data.ptr=data->data;
    ev_.events=data->event;
    return epoll_ctl(epollfd_,EPOLL_CTL_MOD,fd,&ev_);
}

XEpoll::~XEpoll(){
    ::close(epollfd_);
}

#endif//NEW_RTSP_CLIENT

