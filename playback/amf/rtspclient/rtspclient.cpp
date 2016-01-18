#if NEW_RTSP_CLIENT
#if RTSP_CLIENT_DEMO

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>
#include "amf_rtspclient.h" 

//static char *rtsp_url_ = "rtsp://192.168.0.250/h264";
static char *rtsp_url_ = "rtsp://192.168.0.6/stream1";

static int get_input(){
    struct pollfd p[1] = {{STDIN_FILENO, POLLIN, 0}};
    int  n = poll(p, 1, 0);
    if(n > 0){
        if (p[0].revents & POLLIN){
            return 1;
        }
    }
    return 0;
}
void rtsp_test_run()
{
    AmfRtspClientManager::instance()->start_service();

    static const int CLIENT_NUM = 40;
    AmfRtspClient *clients[CLIENT_NUM] = {NULL};

    for(int i = 0; i < CLIENT_NUM; i++){
        clients[i] = AmfRtspClientManager::instance()->create_rtspclient(rtsp_url_);
        //printf("RtspClient %02d --- %p --- created\n",i,clients[i]);
    }

    for(int i = 0; i < CLIENT_NUM; i++){
        //printf("RtspClient %02d --- %p, av_dump_format\n",i,clients[i]);
        clients[i]->start();
        //av_dump_format(clients[i]->get_avformat(), 0,rtsp_url_, 0);
    }

    for(int i = 0; i < CLIENT_NUM; i++){
        //printf("RtspClient %02d --- %p, play\n",i,clients[i]);
        clients[i]->play();
    }
    while(1){
        for(int i = 0; i < CLIENT_NUM; i++){
            AVPacket packet, *pkt =&packet;
            if(clients[i]->read_avframe(pkt) == 0){
                //printf("rtsp_test_run read_avframe() --KEY_FRAME[%d], len = %08d,index = %d,pts = %lld\n",((pkt->flags & AV_PKT_FLAG_KEY)!= 0),pkt->size,pkt->stream_index,pkt->pts);
                av_free_packet(pkt);
            }
        }
        if(get_input()){
            break;
        }
    }

    for(int i =0; i < CLIENT_NUM; i++){
        AmfRtspClientManager::instance()->destroy_rtspclient(clients[i]);
    }
    AmfRtspClientManager::instance()->stop_service();
}

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include "active_object.h"
#include "RtspClientManager.h"
class  RtspClient{
public:
    RtspClient():state_(STATE_INVALID){pthread_mutex_init(&mutex_,NULL);}
    ~RtspClient(){
        pthread_mutex_lock(&mutex_);
        if(state_ != STATE_INVALID){
            RtspClientManager::instance()->rtsp_tear_down(session_id_);
            RtspClientManager::instance()->rtsp_del_session(session_id_);
            state_ = STATE_INVALID;
        }
        pthread_mutex_unlock(&mutex_);
        pthread_mutex_destroy(&mutex_);
    }
    void rtsp_create_session(const std::string &rtsp_url,IRtspClientCallback *cb){
        RtspClientManager::instance()->rtsp_create_session(rtsp_url,cb,(void*)this);
    }
    void on_rtsp_create_session(bool success,unsigned int session_id){
        if(success){
            pthread_mutex_lock(&mutex_);
            state_ = STATE_CREATE_OK;
            session_id_ = session_id;
            pthread_mutex_unlock(&mutex_);
            RtspClientManager::instance()->rtsp_start(session_id_);
        }
    }
    void on_rtsp_start(bool success,const std::string &sdp){
        if(success){
            pthread_mutex_lock(&mutex_);
            state_ = STATE_START_OK;
            pthread_mutex_unlock(&mutex_);
            RtspClientManager::instance()->rtsp_play(session_id_);
        }
    }
    void on_rtsp_play(bool success){
        if(success){
            pthread_mutex_lock(&mutex_);
            state_ = STATE_PLAY_OK;
            pthread_mutex_unlock(&mutex_);
        }
    }
    void on_rtsp_avframe(AVPacket *pkt){
        /*
        if((pkt->flags & AV_PKT_FLAG_KEY)!= 0){
            printf("on_rtsp_avframe --KEY_FRAME[%d], len = %08d,index = %d,pts = %lld\n",((pkt->flags & AV_PKT_FLAG_KEY)!= 0),pkt->size,pkt->stream_index,pkt->pts);
        }
        */
        //printf("on_rtsp_avframe --KEY_FRAME[%d], len = %08d,index = %d,pts = %lld\n",((pkt->flags & AV_PKT_FLAG_KEY)!= 0),pkt->size,pkt->stream_index,pkt->pts);
        av_free_packet(pkt);
    }

    int get_session_id(unsigned int &session_id){
        int result = -1;
        pthread_mutex_lock(&mutex_);
        if(state_ != STATE_INVALID){
            result = 0;
            session_id = session_id_;
        }
        pthread_mutex_unlock(&mutex_);
        return result;
    }
private:
    pthread_mutex_t mutex_;
    enum {STATE_INVALID,STATE_CREATE_OK,STATE_START_OK,STATE_PLAY_OK,STATE_END} state_;
    unsigned int session_id_;
};

class RtspCallback: public IRtspClientCallback
{
public:
    virtual void on_rtsp_create_session(bool success,unsigned int session_id,void *usr_data){
        printf("RtspClientCallback::on_rtsp_create_session,is_success= %d, id = %d\n",success,session_id);
        RtspClient *client = (RtspClient*)usr_data;
        client->on_rtsp_create_session(success,session_id);
    }
     virtual void on_rtsp_start(unsigned int session_id, bool success,const std::string &sdp,void *usr_data){
        printf("RtspClientCallback::on_rtsp_start, is_success= %d,session_id %d\n",success,session_id);
        RtspClient *client = (RtspClient*)usr_data;
        client->on_rtsp_start(success,sdp);
    }
    virtual void on_rtsp_play(unsigned int session_id,bool success,void *usr_data){
        printf("RtspClientCallback::on_rtsp_play, is_success = %d,session_id = %d\n",success,session_id);
        RtspClient *client = (RtspClient*)usr_data;
        client->on_rtsp_play(success);
    }
    virtual void on_rtsp_error(unsigned int session_id,int error_type,void *usr_data){
        printf("RtspClientCallback::on_rtsp_error\n");
    }

    virtual void on_rtsp_avframe(unsigned int session_id,AVPacket *pkt,void *usr_data){
        //printf("on_rtsp_avframe --KEY_FRAME[%d], len = %08d,index = %d,pts = %lld\n",((pkt->flags & AV_PKT_FLAG_KEY)!= 0),pkt->size,pkt->stream_index,pkt->pts);
        //av_free_packet(pkt);
        RtspClient *client = (RtspClient*)usr_data;
        client->on_rtsp_avframe(pkt);
    }
};

void rtsp_test_run2()
{
    RtspCallback *rtsp_callback = new RtspCallback();
    RtspClientManager::instance()->start_service();

    static const int  CLIENT_NUM = 1;
    RtspClient *clients[CLIENT_NUM] = {NULL};
    for(int i = 0; i < CLIENT_NUM; i++){
        clients[i] = new RtspClient;
        clients[i]->rtsp_create_session(rtsp_url_,rtsp_callback);
    }

    while(char c=getchar()){
        if (c=='q'){
            break;
        }
        struct timeval tm;
        tm.tv_sec=0;
        tm.tv_usec=200000;
        select(0,0,0,0,&tm);
    }

    for(int i =0; i < CLIENT_NUM; i++){
        delete clients[i];
    }
    RtspClientManager::instance()->stop_service();
    delete rtsp_callback;
}


class OperationA:public Method_Request{
public:
    OperationA(Future<int> *future):future_(future){
    }
    virtual bool guard() {return true;}
    virtual int call(){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        printf("OperationA::call called,gettimeofday -- tv.tv_sec = %d, tv.tv_usec = %d\n",tv.tv_sec,tv.tv_usec);

        struct timeval tm;
        tm.tv_sec=10;
	 tm.tv_usec=0;
	 select(0,0,0,0,&tm);

        int value = 1000;
        future_->set(value);
        return 0;
    }
    
private:
    Future<int> *future_;
};

class OperationB:public Method_Request{
public:
    OperationB(Future<int> *future):future_(future){
    }
    virtual bool guard() {return true;}
    virtual int call(){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        printf("OperationB::call called,gettimeofday -- tv.tv_sec = %d, tv.tv_usec = %d\n",tv.tv_sec,tv.tv_usec);

        struct timeval tm;
        tm.tv_sec=5;
	 tm.tv_usec=0;
	 select(0,0,0,0,&tm);
        int value = 2000;
        future_->set(value);
        return 0;
    }
private:
    Future<int> *future_;
};

class MyAoProxy:public AO_Proxy{
public:
    void do_test(){
        struct timeval tv;

        Future<int> *future = new Future<int>;
        Method_Request *request = new OperationA(future);
        ao_send_request(request);
        gettimeofday(&tv,NULL);
        printf("OperationA send_request,gettimeofday -- tv.tv_sec = %d, tv.tv_usec = %d\n",tv.tv_sec,tv.tv_usec);

        Future<int> *future2 = new Future<int>;
        Method_Request *request2 = new OperationB(future2);
        ao_send_request(request2);
        gettimeofday(&tv,NULL);
        printf("OperationB send_request,gettimeofday -- tv.tv_sec = %d, tv.tv_usec = %d\n",tv.tv_sec,tv.tv_usec);

        int value = future->get();
        gettimeofday(&tv,NULL);
        printf("OperationA value = %d,gettimeofday -- tv.tv_sec = %d, tv.tv_usec = %d\n",value,tv.tv_sec,tv.tv_usec);
        delete future;

        int value2 = future2->get();
        gettimeofday(&tv,NULL);
        printf("OperationB value = %d,gettimeofday -- tv.tv_sec = %d, tv.tv_usec = %d\n",value2,tv.tv_sec,tv.tv_usec);
        delete future2;
    }
};

void ao_test(){
    MyAoProxy proxy;
    proxy.ao_start_service(1024,2);
    proxy.do_test();
    proxy.ao_exit_service();
}
#endif//RTSP_CLIENT_DEMO
#endif //NEW_RTSP_CLIENT

