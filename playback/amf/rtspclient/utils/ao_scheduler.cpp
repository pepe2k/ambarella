#if NEW_RTSP_CLIENT

#include "ao_internal.h"
#include "pthread_utils.h"

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
    //printf("AO_Scheduler::~AO_Scheduler() called\n");
}
void AO_Scheduler::putq(Method_Request *request){
    if(queue_){
        queue_->putq(request);
    }
}
void AO_Scheduler::start(int num, int cpu_id){
    if(state_==THREAD_STOP){
        state_=THREAD_START;
        pthread_t pid;
        while(num--){
            pthread_create_rr(&pid,run,this,50,cpu_id);
            pids_.push_back(pid);
        }
    }
}
void AO_Scheduler::stop(){
    if(state_==THREAD_START){
        for(int i = 0; i < pids_.size(); i++){
            putq( new Method_Exit);
        }
        for(int i = 0; i < pids_.size(); i++){
            pthread_join(pids_[i],NULL);
        }
        /*
        while(!pids_.empty()){
            putq(new Method_Exit);
            pids_.pop_back();
        }
        */
        state_=THREAD_STOP;
    }
}

#include <sys/select.h>
#include <sys/time.h>
static void my_usleep(int us){
   struct timeval tm;
   tm.tv_sec =0 ;
   tm.tv_usec = us;
   select(0,0,0,0,&tm);
}
void* AO_Scheduler::run(void * arg){
    AO_Scheduler *manager=(AO_Scheduler *)arg;
    while(1){
        Method_Request *request = manager->queue_->getq();
        if(!request){
            my_usleep(1000);
            continue;
        }
        if(!request->guard()){
            manager->queue_->putq(request); //TODO
        }else{
            int ret = request->call();
            delete request; 
            if(ret < 0){
                break;
            }
        }
    }
    //printf("AO_Scheduler thread exit\n");
    return (void*)NULL;
}

#endif //NEW_RTSP_CLIENT
