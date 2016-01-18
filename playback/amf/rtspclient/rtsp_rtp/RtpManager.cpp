#if NEW_RTSP_CLIENT

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include "RtpManager.h"
#include "IRtpSession.h"
#include "RtpSession.h"
#include "RtpDemuxer.h"


// Write a C string ('\0' terminated) to a file.
//
static int writeStringToFile(const char *filename, const char *start)
{
    int fd = open(filename, O_WRONLY);
    if (fd < 0){
        return -1;
    }
    write(fd, start, strlen(start));
    close(fd);
    return 0;
}

static int writeIntToFile(const char *filename, long value)
{
    char buffer[16] = {0,};
    sprintf(buffer, "%ld", value);
    return writeStringToFile(filename, buffer);
}

RtpManager *RtpManager::instance_=NULL;
pthread_mutex_t RtpManager::mutex_= PTHREAD_MUTEX_INITIALIZER;
RtpManager *RtpManager::instance(){
    if(instance_==NULL){
        pthread_mutex_lock(&mutex_);
        if(instance_==NULL){
            instance_=new RtpManager;
        }
        pthread_mutex_unlock(&mutex_);
    }
    return instance_;
}

RtpManager::RtpManager():service_num_(0){
}

RtpManager::~RtpManager(){
    //printf("RtpManager::~RtpManager() called\n");
    pthread_mutex_lock(&mutex_);
    instance_ = NULL;
    pthread_mutex_unlock(&mutex_);
}

void RtpManager::start_service(){
    //hardcode, thread-number,TODO
    RTPDemuxer::start_service(2/*num*/);

    //hardcode, eth0 irq handled by CPU0
    writeIntToFile("/proc/irq/59/smp_affinity",1);
    int num = 2;
    service_num_=num;
    while(num-->0){
        RtpService * service=new RtpService();
        vecService_.push_back(service);
        service->start(num %2);
    }
}

void RtpManager::stop_service(){
    while(!vecService_.empty()){
        RtpService * service=*(vecService_.begin());
        vecService_.erase(vecService_.begin());
        service->stop();
        delete service;
        //printf("RtpManager::stop_service() called %d\n",vecService_.size());
    }
    writeIntToFile("/proc/irq/59/smp_affinity",3);
    RTPDemuxer::stop_service();
}

IRtpSession * RtpManager::create_rtp_session(IRtpCallback *cb,FormatContext *context,unsigned int id,RtpType type)
{
    IRtpSession * session = new RtpSession(cb,context,id,type);
    if(session->bind_port()==0)	{
        int num=id%service_num_;
        session->set_owner(vecService_[num]);
        vecService_[num]->add_session(session);
    }else{
        delete session;
        session=NULL;
    }
    return session;
}

void RtpManager::destroy_rtp_session(IRtpSession *session){
    RtpService * service=(RtpService *)session->get_owner();
    service->del_session(session);
    delete session;
}

#endif //NEW_RTSP_CLIENT

