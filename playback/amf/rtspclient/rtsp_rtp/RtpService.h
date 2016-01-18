#ifndef _RTP_SERVICE_H_
#define _RTP_SERVICE_H_

#if NEW_RTSP_CLIENT

#include <map>
#include <pthread.h>
#include "xpipe.h"
#include "xpoll_epoll.h"
#include "IRtpSession.h"
#include "active_object.h"

struct RtpSessionInfo{
    IRtpSession *session;
    int fd;
};
class RtpService
{
public:
    RtpService();
    ~RtpService();
    void start(int cpu_id = 0xff);
    void stop();
    void add_session(IRtpSession * session);
    void del_session(IRtpSession * session);
    void dump();
protected:
    static void * process(void *arg);
private:
    void add_session_(IRtpSession *session, int event);
    void delete_session_(IRtpSession *session);
private:
    XPipe mPipe;
    XEpoll *mXPoll;
    std::map<int,RtpSessionInfo * > mapSession;
    pthread_t pid_;
    Future<int> *future_;
};

#endif //NEW_RTSP_CLIENT

#endif

