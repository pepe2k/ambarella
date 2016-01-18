#ifndef AMF_RTSP_CLIENT_H_
#define AMF_RTSP_CLIENT_H_

#if NEW_RTSP_CLIENT

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string>

extern "C"{
    #include "libavformat/avformat.h"
};
#include "active_object.h"

class AmfRtspClient;
class AmfRtspClientManager{
public:
    static AmfRtspClientManager *instance();
    int start_service();
    int stop_service();//delete self
    AmfRtspClient *create_rtspclient(const char *rtsp_url);
    void destroy_rtspclient(AmfRtspClient*);
private:
    AmfRtspClientManager();
    ~AmfRtspClientManager();
    static AmfRtspClientManager *m_instance;
};

class RtspClientCallback;

class AmfRtspClient{
    friend class AmfRtspClientManager;
    friend class RtspClientCallback;
public:
    int start();
    //AVFormatContext is read only,and will be freed by AmfRtspClient, called after start() successfully
    AVFormatContext *get_avformat(){return av_context_;};
    int play();
    int read_avframe(AVPacket *pkt){
        if(rtsp_error_) return rtsp_error_;
        return queue_->getq(pkt);
    }
    int flush_queue(){return queue_->flush();}
private:
    AmfRtspClient();
    ~AmfRtspClient();
    int create(const char *rtsp_url);

    void on_rtsp_create_session(bool success,unsigned int session_id);
    void on_rtsp_start(bool success,const std::string &sdp);
    void on_rtsp_play(bool success);
    void on_rtsp_error(int error_type){
        rtsp_error_ = error_type;
    }
    void on_rtsp_avframe(AVPacket *pkt){
        if(rtsp_error_){
            av_free_packet(pkt);
            return;
        }
        queue_->putq(pkt);
    }
private:
    std::string rtsp_url_;
    AVFormatContext *av_context_;
    int av_parse_media(const std::string &sdp);
    int av_parse_free();

    unsigned int m_sessionid_;
    enum {STATE_INVALID,STATE_CREATE_OK,STATE_START_OK,STATE_PLAY_OK,STATE_END} state_;
    volatile int rtsp_error_;

    class RtspAvQueue{
    public:
        RtspAvQueue(int queue_size = 64);
        ~RtspAvQueue();
        void putq(AVPacket *pkt);
        int getq(AVPacket *pkt);
        int flush();
    private:
        pthread_mutex_t q_mutex_;
        pthread_cond_t cond_get_;
        AVPacket *buffer;
        int sizeQueue;
        int lput;
        int lget;
        int nData;
    };
    RtspAvQueue *queue_;

    struct rtsp_result_t{
        bool success;
        unsigned int session_id;
        std::string sdp;
    };
    Future<rtsp_result_t> *m_future_;
};

#endif //NEW_RTSP_CLIENT

#endif //AMF_RTSP_CLIENT_H_

