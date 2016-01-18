#if NEW_RTSP_CLIENT

#include "ao_internal.h"

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

#endif//NEW_RTSP_CLIENT

