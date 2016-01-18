#if NEW_RTSP_CLIENT

#include "ao_internal.h"
AO_Proxy::AO_Proxy():scheduler_(NULL){
}
AO_Proxy::~AO_Proxy(){
    ao_exit_service();
}
int AO_Proxy::ao_start_service(int queue_size, int thread_num,int cpu_id){
    if(scheduler_){
        return 0;
    }
    scheduler_ = new AO_Scheduler(queue_size);
    if(!scheduler_){
        return -1;
    }
    scheduler_->start(thread_num,cpu_id);
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

#endif //NEW_RTSP_CLIENT

