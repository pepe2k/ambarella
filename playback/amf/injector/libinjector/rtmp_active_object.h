#ifndef _RTMP_ACTIVE_OBJECT_H_
#define _RTMP_ACTIVE_OBJECT_H_

#include <pthread.h>

namespace AMBA_RTMP {

class Method_Request{
public:
    virtual bool guard() = 0;
    virtual int call() = 0;
    virtual ~Method_Request(){};
};

//"T" must implent "=", TODO
template<typename T>
class Future{
public:
    Future():state_(EMPTY){
        pthread_mutex_init(&mutex_,NULL);
        pthread_cond_init(&cond_,NULL);
    }
    ~Future(){
        pthread_mutex_destroy(&mutex_);
        pthread_cond_destroy(&cond_);
    }
    void set(T &value){
        pthread_mutex_lock(&mutex_);
        if(state_ == EMPTY){
            state_ = SET;
            value_ = value;
            pthread_cond_signal(&cond_);
        }
        pthread_mutex_unlock(&mutex_);
    }
    bool is_ready(){
        bool ready;
        pthread_mutex_lock(&mutex_);
        ready = (state_ != EMPTY);
        pthread_mutex_unlock(&mutex_);
        return ready;
    }
    T get(){
        T value;
        pthread_mutex_lock(&mutex_);
        while(state_ == EMPTY){
            pthread_cond_wait(&cond_,&mutex_);
        }
        value = value_;
        pthread_mutex_unlock(&mutex_);
        return value;
    }
    void reset(){
        pthread_mutex_lock(&mutex_);
        state_ = EMPTY;
        pthread_mutex_unlock(&mutex_);
    }
private:
    T value_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
    enum {EMPTY,SET} state_;
};

class AO_Scheduler;
class AO_Proxy{
protected:
    AO_Proxy();
    virtual ~AO_Proxy();
public:
    int ao_start_service(int requestQueueSize = 1024);
    int ao_exit_service();
    int ao_send_request(Method_Request *request);
private:
    AO_Scheduler *scheduler_;
};

} //namespace

#endif //_RTMP_ACTIVE_OBJECT_H_


