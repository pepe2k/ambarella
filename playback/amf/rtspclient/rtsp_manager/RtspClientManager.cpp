#if NEW_RTSP_CLIENT

#include <stdio.h>
#include "timer_manager.h"
#include "RtspTcpService.h"
#include "RtspClientManager.h"
#include "RtspClientSession.h"
#include "RtpManager.h"
#include "am_ffmpeg.h"

RtspClientManager * RtspClientManager::instance_=NULL;
pthread_mutex_t RtspClientManager::mutex_= PTHREAD_MUTEX_INITIALIZER;
RtspClientManager * RtspClientManager::instance(){
    if(instance_==NULL){
        pthread_mutex_lock(&mutex_);
        if(instance_==NULL){
            instance_=new RtspClientManager();
        }
        pthread_mutex_unlock(&mutex_);
    }
    return instance_;
}

RtspClientManager::RtspClientManager(){
}

RtspClientManager:: ~RtspClientManager(){
    //printf("RtspClientManager:: ~RtspClientManager() called\n");
    pthread_mutex_lock(&mutex_);
    instance_ = NULL;
    pthread_mutex_unlock(&mutex_);
}

void RtspClientManager::start_service(){
    mw_ffmpeg_init();
    CTimerManager::instance()->start(1000,20);
    RtpManager::instance()->start_service();
    RtspTcpService::instance()->start_service(this);
    ao_start_service();
}
void RtspClientManager::stop_service(){
    CTimerManager::instance()->stop();
    CTimerManager::instance()->del_self();
    RtpManager::instance()->stop_service();
    RtpManager::instance()->del_self();
    RtspTcpService::instance()->stop_service();
    RtspTcpService::instance()->del_self();
    ao_exit_service();
    delete this;
}

void RtspClientManager::connect_completed(unsigned int id, bool result){
    class ConnectCompleted:public Method_Request{
    public:
        ConnectCompleted(RtspClientManager *manager,unsigned int id, bool result)
            :manager_(manager),id_(id),result_(result)
        {}
        virtual bool guard(){
            return true;
        }
        virtual int call(){
            if(manager_->mapRtspClient.find(id_)!=manager_->mapRtspClient.end()){
                RtspClientSession * session = manager_->mapRtspClient[id_];
                session->connect_completed(result_);
            }
            return 0;
        }
    private:
        RtspClientManager *manager_;
        unsigned int id_;
        bool result_;
    };
    ao_send_request(new ConnectCompleted(this,id,result));
}

void RtspClientManager::peer_close(unsigned int id){
    class PeerClose:public Method_Request{
    public:
        PeerClose(RtspClientManager *manager,unsigned int id)
            :manager_(manager),id_(id)
        {}
        virtual bool guard(){
            return true;
        }
        virtual int call(){
            if(manager_->mapRtspClient.find(id_)!=manager_->mapRtspClient.end()){
                RtspClientSession * session = manager_->mapRtspClient[id_];
                session->on_disconnect();
            }
            return 0;
        }
    private:
        RtspClientManager *manager_;
        unsigned int id_;
    };
    ao_send_request(new PeerClose(this,id));
}

void  RtspClientManager::rtsp_create_session(const std::string &url,IRtspClientCallback *cb,void *usr_data){
    class RtspCreateSession:public Method_Request{
    public:
        RtspCreateSession(RtspClientManager *manager,const std::string &url,IRtspClientCallback *cb,void *usr_data)
            :manager_(manager),url_(url),cb_(cb),usr_data_(usr_data)
        {}
        virtual bool guard(){
            return true;
        }
        virtual int call(){
            RtspClientSession * pSession=NULL;
            pSession=new RtspClientSession(url_,cb_,usr_data_);
            manager_->mapRtspClient[pSession->id()]=pSession;
            return 0;
        }
    private:
        RtspClientManager *manager_;
        std::string url_;
        IRtspClientCallback *cb_;
        void *usr_data_;
    };
    ao_send_request(new RtspCreateSession(this,url,cb,usr_data));
}

void RtspClientManager::rtsp_del_session(unsigned int session_id){
    class RtspDelSession:public Method_Request{
    public:
        RtspDelSession(RtspClientManager *manager,unsigned int id,Future<int> *future)
            :manager_(manager),id_(id),future_(future)
        {}
        virtual bool guard(){
            return true;
        }
        virtual int call(){
            if(manager_->mapRtspClient.find(id_)!=manager_->mapRtspClient.end()){
                RtspClientSession * session = manager_->mapRtspClient[id_];
                manager_->mapRtspClient.erase(id_);
                delete session;
            }
            int result = 0;
            future_->set(result);
            return 0;
        }
    private:
        RtspClientManager *manager_;
        unsigned int id_;
        Future<int> *future_;
    };
    Future<int> *future = new Future<int>;
    ao_send_request(new RtspDelSession(this,session_id,future));
    future->get();
    delete future;
}

void RtspClientManager::rtsp_start(unsigned int session_id){
    class RtspStart:public Method_Request{
    public:
        RtspStart(RtspClientManager *manager,unsigned int id)
            :manager_(manager),id_(id)
        {}
        virtual bool guard(){
            return true;
        }
        virtual int call(){
            if(manager_->mapRtspClient.find(id_)!=manager_->mapRtspClient.end()){
                RtspClientSession * session = manager_->mapRtspClient[id_];
                session->start();
            }
            return 0;
        }
    private:
        RtspClientManager *manager_;
        unsigned int id_;
    };
    ao_send_request(new RtspStart(this,session_id));
}


void RtspClientManager::rtsp_play(unsigned int session_id){
    class RtspPlay:public Method_Request{
    public:
        RtspPlay(RtspClientManager *manager,unsigned int id)
            :manager_(manager),id_(id)
        {}
        virtual bool guard(){
            return true;
        }
        virtual int call(){
            if(manager_->mapRtspClient.find(id_)!=manager_->mapRtspClient.end()){
                RtspClientSession * session = manager_->mapRtspClient[id_];
                session->play();
            }
            return 0;
        }
    private:
        RtspClientManager *manager_;
        unsigned int id_;
    };
    ao_send_request(new RtspPlay(this,session_id));
}

void RtspClientManager::rtsp_tear_down(unsigned int session_id){
    class RtspTearDown:public Method_Request{
    public:
        RtspTearDown(RtspClientManager *manager,unsigned int id,Future<int> *future)
            :manager_(manager),id_(id),future_(future)
        {}
        virtual bool guard(){
            return true;
        }
        virtual int call(){
            if(manager_->mapRtspClient.find(id_)!=manager_->mapRtspClient.end()){
                RtspClientSession * session = manager_->mapRtspClient[id_];
                session->tear_down();
            }
            int result = 0;
            future_->set(result);
            return 0;
        }
    private:
        RtspClientManager *manager_;
        unsigned int id_;
        Future<int> *future_;
    };

    Future<int> *future = new Future<int>;
    ao_send_request(new RtspTearDown(this,session_id,future));
    future->get();
    delete future;
}

void RtspClientManager::receive_message(unsigned int id,char * base,int len){
    class ReceiveMessage:public Method_Request{
    public:
        ReceiveMessage(RtspClientManager *manager,unsigned int id,const std::string &msg)
            :manager_(manager),id_(id),msg_(msg)
        {}
        virtual bool guard(){
            return true;
        }
        virtual int call(){
            if(manager_->mapRtspClient.find(id_)!=manager_->mapRtspClient.end()){
                RtspClientSession * session = manager_->mapRtspClient[id_];
                //printf("%s\n",msg_.c_str());
                session->decode_message(msg_);
            }
            return 0;
        }
    private:
        RtspClientManager *manager_;
        unsigned int id_;
        std::string msg_;
    };

    std::string msg;
    msg.assign(base,0,len);
    ao_send_request(new ReceiveMessage(this,id,msg));
}

void RtspClientManager::request_timeout(unsigned int session_id){
    class RequestTimeout:public Method_Request{
    public:
        RequestTimeout(RtspClientManager *manager,unsigned int id)
            :manager_(manager),id_(id)
        {}
        virtual bool guard(){
            return true;
        }
        virtual int call(){
            if(manager_->mapRtspClient.find(id_)!=manager_->mapRtspClient.end()){
                RtspClientSession * session = manager_->mapRtspClient[id_];
                session->request_timeout();
            }
            return 0;
        }
    private:
        RtspClientManager *manager_;
        unsigned int id_;
    };
    ao_send_request(new RequestTimeout(this,session_id));
}

void RtspClientManager::heartbeat_timeout(unsigned int session_id){
    class HearbeatTimeout:public Method_Request{
    public:
        HearbeatTimeout(RtspClientManager *manager,unsigned int id)
            :manager_(manager),id_(id)
        {}
        virtual bool guard(){
            return true;
        }
        virtual int call(){
            if(manager_->mapRtspClient.find(id_)!=manager_->mapRtspClient.end()){
                RtspClientSession * session = manager_->mapRtspClient[id_];
                session->on_heartbeat_timeout();
            }
            return 0;
        }
    private:
        RtspClientManager *manager_;
        unsigned int id_;
    };
    ao_send_request(new HearbeatTimeout(this,session_id));
}

#endif //NEW_RTSP_CLIENT

