#ifndef XPOLL_EPLL_H_
#define XPOLL_EPLL_H_

#if NEW_RTSP_CLIENT

#include <unistd.h>
#include <sys/epoll.h>

#define XPOLL_MAX_EVENT 10002

#define XPOLLIN EPOLLIN
#define XPOLLPRI EPOLLPRI
#define XPOLLHUP EPOLLHUP
#define XPOLLERR EPOLLERR
#define XPOLLOUT EPOLLOUT

typedef struct XPollData_st{
    void * data;
    int event;
}XPollData;

class XEpoll
{
public:
    XEpoll(){}
    int init();
    int wait();
    void * next_result();
    void delete_from_results(void * data);
    void * get_data(void * event);
    int get_event(void * event);
    int add_data(int fd,XPollData * data);
    int delete_data(int fd,XPollData *data);
    int change_data(int fd,XPollData *data);
    int reset_data(int fd,XPollData *data){return 0;}
    ~XEpoll();
private:
    int result_fds_;
    int loc_;
    struct epoll_event ev_;
    struct epoll_event result_events_[XPOLL_MAX_EVENT];
    int epollfd_;
};
#endif//NEW_RTSP_CLIENT
#endif

