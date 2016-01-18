#ifndef _RTSPCLIENT_MANAGER_H_
#define _RTSPCLIENT_MANAGER_H_

#if NEW_RTSP_CLIENT

#include <string>
#include <map>
#include <pthread.h>
#include "IXServer.h"
#include "active_object.h"

extern "C"{
    #include "libavformat/avformat.h"
};

class IRtspClientCallback{
public:
    virtual ~IRtspClientCallback(){}
    virtual void on_rtsp_create_session(bool sucess,unsigned int session_id,void *usr_data) = 0;
    virtual void on_rtsp_start(unsigned int session_id, bool sucess,const std::string &sdp,void *usr_data) = 0;
    virtual void on_rtsp_play(unsigned int session_id,bool sucess,void *usr_data) = 0;
    virtual void on_rtsp_error(unsigned int session_id,int error_type,void *usr_data) = 0;
    virtual void on_rtsp_avframe(unsigned int session_id,AVPacket *pkt,void *usr_data) = 0;
};

class RtspClientSession;
class RtspClientManager:public IXServer,public AO_Proxy{
public:
    static RtspClientManager * instance();
    ~RtspClientManager();
    void start_service();
    void stop_service();//free self

    /* rtsp_create_session/rtsp_start/rtsp_play/rtsp_tear_down functions are asynchronous, 
    *      the result will be notified by cb(IRtspClientCallback)
    */
    void rtsp_create_session(const std::string &url,IRtspClientCallback *cb,void *usr_data);
    void rtsp_start(unsigned int session_id);
    void rtsp_play(unsigned int session_id);
    void rtsp_tear_down(unsigned int session_id);
    void rtsp_del_session(unsigned int session_id);

    /*rtsp interface*/
    virtual void connect_completed(unsigned int id, bool result);
    virtual void peer_close(unsigned int id);
    virtual void receive_message(unsigned int id,char * base,int len);
    void request_timeout(unsigned int id);
    void heartbeat_timeout(unsigned int id);
private:
    RtspClientManager();
    static RtspClientManager * instance_;
    static pthread_mutex_t mutex_;
    std::map<int,RtspClientSession *> mapRtspClient;
};

#endif //NEW_RTSP_CLIENT

#endif //_RTSPCLIENT_MANAGER_H_


