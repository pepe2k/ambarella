#ifndef _RTP_MANAGER_H_
#define _RTP_MANAGER_H_

#if NEW_RTSP_CLIENT

#include <pthread.h>
#include <vector>
#include "RtpService.h"
#include "IRtpSession.h"
#include "RtpSession.h"

class RtpManager
{
public:
    static RtpManager *instance();
    void start_service();
    void stop_service();
    void del_self() {delete this;}
    IRtpSession * create_rtp_session(IRtpCallback *cb, FormatContext *context,unsigned int id,RtpType = RTP_NORMAL);
    void destroy_rtp_session(IRtpSession *conn);
private:
    RtpManager();
    ~RtpManager();
private:
    static RtpManager *instance_;
    static pthread_mutex_t mutex_;
private:
    int service_num_;
    std::vector<RtpService *> vecService_;
};

#endif //NEW_RTSP_CLIENT

#endif

