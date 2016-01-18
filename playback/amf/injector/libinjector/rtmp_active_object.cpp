#include <stdio.h>

#include "rtmp_active_object.h"

namespace AMBA_RTMP{

class AO_Queue{
public:
    virtual ~AO_Queue(){};
    virtual Method_Request *getq() = 0;
    virtual void putq(Method_Request *request) = 0;
};

class AO_Queue_OneLock:public AO_Queue{
public:
    AO_Queue_OneLock(int queueSize=1024);
    ~AO_Queue_OneLock();
    Method_Request* getq();
    void putq(Method_Request *request);
private:
    pthread_mutex_t mux;
    pthread_cond_t condGet;
    pthread_cond_t condPut;
    void **buffer;
    int sizeQueue;	//length of queue
    int lput;		//location put
    int lget;		//location get
    int nFullThread;	//when queue is full, the number of thread blocked in putq
    int nEmptyThread;//when queue is empty, the number of thread blocked in getq
    int nData;	//data number in queue ,using to jusge queue  is full or empty 
};


class AO_Scheduler{
public:
    enum ThreadState{
        THREAD_STOP=0,
        THREAD_START
    };
    AO_Scheduler(int queue_length=1024);
    virtual ~AO_Scheduler();
    void start();
    void stop();
    void putq(Method_Request *request);
protected:
    static void* run(void *arg);
private:
    AO_Queue *queue_;
    ThreadState state_;
    pthread_t pid_;
};

//
//AO_Queue implementation
//
AO_Queue_OneLock::AO_Queue_OneLock(int queueSize)
    :sizeQueue(queueSize),lput(0),lget(0),nFullThread(0),nEmptyThread(0),nData(0)
{
    pthread_mutex_init(&mux,0);
    pthread_cond_init(&condGet,0);
    pthread_cond_init(&condPut,0);
    buffer = new void*[queueSize];
}
AO_Queue_OneLock::~AO_Queue_OneLock()
{
    pthread_cond_destroy(&condPut);
    pthread_cond_destroy(&condGet);
    pthread_mutex_destroy(&mux);
    if(buffer){
        delete []buffer;
        buffer = NULL;
    }
}
void AO_Queue_OneLock::putq(Method_Request  *request)
{
    pthread_mutex_lock(&mux);
    while(lput==lget&&nData){
        nFullThread++;
        pthread_cond_wait(&condPut,&mux);
        nFullThread--;
    }

    buffer[lput++] = (void*)request;
    nData++;
    if(lput==sizeQueue){
        lput=0;
    }
    if(nEmptyThread){
        pthread_cond_signal(&condGet);
    }
    pthread_mutex_unlock(&mux);
}

Method_Request * AO_Queue_OneLock::getq()
{
    Method_Request *request = NULL;

    pthread_mutex_lock(&mux);
    while(lget==lput&&nData==0){
        nEmptyThread++;
        pthread_cond_wait(&condGet,&mux);
        nEmptyThread--;
    }

    request = (Method_Request *)buffer[lget++];
    nData--;
    if(lget==sizeQueue){
        lget=0;
    }
    if(nFullThread){
        pthread_cond_signal(&condPut);
    }
    pthread_mutex_unlock(&mux);
    return request;
}


#include <pthread.h>
static int rtmp_pthread_create(pthread_t *pid,void*(*start_rtn)(void*),void *arg,int priority = 1){
    pthread_attr_t attr;
    struct sched_param param;
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = priority;
    pthread_attr_setschedparam(&attr, &param);
    int err = pthread_create(pid, &attr, start_rtn, (void*)arg);
    pthread_attr_destroy(&attr);
    return err;
}
//
//AO_Scheduler implementation
//
class Method_Exit:public Method_Request{
public:
    virtual bool guard(){return true;}
    virtual int call(){return -1;}
};

AO_Scheduler::AO_Scheduler(int queue_length):state_(THREAD_STOP){
    queue_=new AO_Queue_OneLock(queue_length);
}
AO_Scheduler::~AO_Scheduler(){
    if(queue_){
        delete queue_;
        queue_ = NULL;
    }
}
void AO_Scheduler::putq(Method_Request *request){
    if(queue_){
        queue_->putq(request);
    }
}
void AO_Scheduler::start(){
    if(state_==THREAD_STOP){
        state_=THREAD_START;
        //pthread_create(&pid_,NULL,run,this);
        rtmp_pthread_create(&pid_,run,this);
    }
}
void AO_Scheduler::stop(){
    if(state_==THREAD_START){
        putq( new Method_Exit);
        pthread_join(pid_,NULL);
        state_=THREAD_STOP;
    }
}

#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
static void my_usleep(int us){
   struct timeval tm;
   tm.tv_sec =0 ;
   tm.tv_usec = us;
   select(0,0,0,0,&tm);
}
void* AO_Scheduler::run(void * arg){
    signal(SIGPIPE, SIG_IGN);
    AO_Scheduler *manager=(AO_Scheduler *)arg;
    while(1){
        Method_Request *request = manager->queue_->getq();
        if(!request){
            my_usleep(10000);
            continue;
        }
        if(!request->guard()){
            manager->queue_->putq(request);
        }else{
            int ret = request->call();
            delete request; 
            if(ret < 0){
                break;
            }
        }
    }
    printf("AO_Scheduler thread exit\n");
    return (void *)0;
}

//
//AO_Proxy implementation
//
AO_Proxy::AO_Proxy():scheduler_(NULL){
}
AO_Proxy::~AO_Proxy(){
    ao_exit_service();
}
int AO_Proxy::ao_start_service(int queue_size){
    if(scheduler_){
        return 0;
    }
    scheduler_ = new AO_Scheduler(queue_size);
    if(!scheduler_){
        return -1;
    }
    scheduler_->start();
    return 0;
}

int AO_Proxy::ao_exit_service(){
    if(scheduler_){
        scheduler_->stop();
        delete scheduler_;
        scheduler_ = NULL;
    }
    return 0;
}

int AO_Proxy::ao_send_request(Method_Request *request){
    if(scheduler_){
        scheduler_->putq(request);
        return 0;
    }
    return -1;
}
}

