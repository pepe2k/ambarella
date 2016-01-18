#include <stdio.h>
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh" //for gettimeofday
#include "rtsp_queue.h"

RtspDataQueue::RtspDataQueue(int queueSize)
    :sizeQueue(queueSize),nData(0),exit_flag_(1)
{
    pthread_mutex_init(&q_mutex_,NULL);
    pthread_cond_init(&cond_get_,NULL);
    pthread_mutex_init(&flag_mutex_,NULL);
    head_ = NULL;
    tail_ = NULL;
    video_key_frame_got = 0;
    is_ds_type_live_ = 1;
    base_ts = -1;
}

RtspDataQueue::~RtspDataQueue()
{
    flush();
    pthread_mutex_destroy(&q_mutex_);
    pthread_cond_destroy(&cond_get_);
    pthread_mutex_destroy(&flag_mutex_);
}

void RtspDataQueue::discard_pkts(){
    RtspData *tmp = head_;
    head_ = head_->next;
    RtspData::rtsp_data_free(tmp);
    --nData;
}

void RtspDataQueue::putq(RtspMediaSession::DataType type,unsigned char *data,int len,long long timestamp,int sample_rate){
    pthread_mutex_lock(&q_mutex_);
    if(exit_flag()){
        pthread_mutex_unlock(&q_mutex_);
        return;
    }
    RtspData *pkt = RtspData::rtsp_data_alloc(type,data,len);
    if(!pkt){
        pthread_mutex_unlock(&q_mutex_);
        return;
    }
    if(is_ds_type_live_){
        //flush queue
        if(nData == sizeQueue){
            while(head_){
                RtspData *pkt = head_->next;
                RtspData::rtsp_data_free(head_);
                head_ = pkt;
            }
            head_ = tail_ = NULL;
            nData = 0;
            video_key_frame_got = 0;
            printf("RtspDataQueue::putq() --flush Done,type = %d\n",(int)type);;
        }
        pkt->timestamp = timestamp;
        if(timestamp < 0){
            gettimeofday(&pkt->ts,NULL);//TODO
        }else{
            if(base_ts == -1){
                gettimeofday(&ptAdjustment,NULL);
                base_ts = timestamp;
            }
            int64_t microseconds = (timestamp - base_ts) * 1000000/sample_rate;
            pkt->ts.tv_sec =  ptAdjustment.tv_sec + microseconds/1000000;
            pkt->ts.tv_usec = ptAdjustment.tv_usec + microseconds%1000000;
            while (pkt->ts.tv_usec  >= 1000000) {
                ++pkt->ts.tv_sec;
                pkt->ts.tv_usec  -= 1000000;
            }
        }
    }else{
        //block until there is space or exit_flag set
        struct timeval now;
        struct timespec outtime;
        int rc = 0;
        while(nData==sizeQueue&& rc == 0 && !exit_flag()){
            gettimeofday(&now, NULL);
            outtime.tv_sec = now.tv_sec + 1;
            outtime.tv_nsec = now.tv_usec * 1000;
            rc = pthread_cond_timedwait(&cond_get_,&q_mutex_,&outtime);
        }
        if(nData == sizeQueue || exit_flag()){
            RtspData::rtsp_data_free(pkt);
            pthread_mutex_unlock(&q_mutex_);
            return;
        }
    }

    if(tail_){
        tail_->next = pkt;
    }
    tail_ = pkt;

    if(!nData){
        head_ = tail_;
    }
    nData++;

    pthread_mutex_unlock(&q_mutex_);
}

int RtspDataQueue::notify_exit(int value){
    pthread_mutex_lock(&flag_mutex_);
    exit_flag_ = value;
    pthread_mutex_unlock(&flag_mutex_);
    return 0;
}
int RtspDataQueue::set_ds_type(int is_live){
    pthread_mutex_lock(&q_mutex_);
    is_ds_type_live_ = is_live;
    pthread_mutex_unlock(&q_mutex_);
    return 0;
}
int RtspDataQueue::getq(RtspData **pkt,int &isLive){
    pthread_mutex_lock(&q_mutex_);
    if(nData==0){
        pthread_mutex_unlock(&q_mutex_);
        return -1;
    }
    *pkt = head_;
    head_ = head_->next;
    nData--;
    if(!nData){
        tail_ = NULL;
    }
    if(!is_ds_type_live_){
        pthread_cond_signal(&cond_get_);
    }
    isLive = is_ds_type_live_;
    pthread_mutex_unlock(&q_mutex_);
    return 0;
}

int RtspDataQueue::flush(){
    pthread_mutex_lock(&q_mutex_);
    while(head_){
        RtspData *pkt = head_->next;
        RtspData::rtsp_data_free(head_);
        head_ = pkt;
    }
    head_ = tail_ = NULL;
    nData = 0;
    video_key_frame_got = 0;
    //printf("RtspDataQueue::flush() Done\n");
    pthread_mutex_unlock(&q_mutex_);
    return 0;
}

