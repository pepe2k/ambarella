#include <stdio.h>
#include <sys/time.h>
#include "rtmp_queue.h"

RtmpDataQueue::RtmpDataQueue(int queueSize)
    :sizeQueue(queueSize),nData(0),exit_flag(0),prev_msecs(0),prev_timestamp(0)
{
    pthread_mutex_init(&q_mutex_,NULL);
    pthread_cond_init(&cond_get_,0);
    head_ = NULL;
    tail_ = NULL;
    video_key_frame_got = 0;
}

RtmpDataQueue::~RtmpDataQueue()
{
    flush();
    pthread_mutex_destroy(&q_mutex_);
    pthread_cond_destroy(&cond_get_);
}

static int gettimeofday_clock(struct timeval *tv, int *tz){
    //return gettimeofday(tp,tz);
    struct timespec timeNow;
    clock_gettime(CLOCK_MONOTONIC, &timeNow);
    tv->tv_sec = timeNow.tv_sec;
    tv->tv_usec = timeNow.tv_nsec/1000;
    return 0;
}

void RtmpDataQueue::calcTimestamp(RtmpData *pkt){
    /*TODO, fill correct timestamp,
    *  temp codes, use recvtime to calc timestamp
    */
    struct timeval timeNow;
    gettimeofday_clock(&timeNow,NULL);
    unsigned long long msecs = timeNow.tv_sec * 1000 + timeNow.tv_usec/1000; 
    unsigned long curr_msecs = (unsigned long)(msecs & 0xffffffff);
    if(!prev_msecs){
        prev_msecs = curr_msecs;
        prev_timestamp = 0;
    }
    unsigned long diff;
    if(curr_msecs >= prev_msecs){
       diff = curr_msecs - prev_msecs;
    }else{
        diff = curr_msecs + (0xffffffff - prev_msecs);
    }
    prev_msecs = curr_msecs;

    pkt->timestamp = prev_timestamp + diff;
    prev_timestamp += diff;
}

void RtmpDataQueue::discard_pkts(){
    //first pkt in queue is video-key-frame, discard it now.
    if(is_video_pkt(head_) && head_->keyframe){
        RtmpData *tmp = head_;
        head_ = head_->next;
        RtmpData::rtmp_data_free(tmp);
        --nData;
    }

    RtmpData *pkt = head_;
    int video_key_exist = 0;
    while(pkt){
        if(is_video_pkt(pkt) && pkt->keyframe){
            video_key_exist = 1;
            break;
        }
        pkt = pkt->next;
    }
    if(!video_key_exist){
        while(head_){
            RtmpData *tmp= head_;
            head_ = head_->next;
            RtmpData::rtmp_data_free(tmp);
        }
        head_ = tail_ = NULL;
        nData = 0;
        video_key_frame_got = 0;
        return;
    }

    pkt = head_;
    while(pkt){
        if(!is_video_pkt(pkt)){
            RtmpData *tmp = pkt;
            pkt = pkt->next;
            RtmpData::rtmp_data_free(tmp);
            --nData;
        }else if(!pkt->keyframe){
            RtmpData *tmp = pkt;
            pkt = pkt->next;
            RtmpData::rtmp_data_free(tmp);
            --nData;
        }else{
            head_ = pkt;
            break;
        }
    }
}

void RtmpDataQueue::putq(RtmpData *pkt){
    pthread_mutex_lock(&q_mutex_);
    if(exit_flag){
        RtmpData::rtmp_data_free(pkt);
        pthread_mutex_unlock(&q_mutex_);
        return;
    }

    if(nData == sizeQueue){
        discard_pkts();
    }

    if(is_video_pkt(pkt)){
        if(!video_key_frame_got){
            if(!pkt->keyframe){
                RtmpData::rtmp_data_free(pkt);
                pthread_mutex_unlock(&q_mutex_);
                return;
            }else{
                video_key_frame_got = 1;
            }
        }
    }
    calcTimestamp(pkt);

    if(tail_){
        tail_->next = pkt;
    }
    tail_ = pkt;

    if(!nData){
        head_ = tail_;
    }
    nData++;

    pthread_cond_signal(&cond_get_);
    pthread_mutex_unlock(&q_mutex_);
}

int RtmpDataQueue::notify_exit(int value){
    pthread_mutex_lock(&q_mutex_);
    exit_flag = value;
    pthread_mutex_unlock(&q_mutex_);
    return 0;
}
int RtmpDataQueue::getq(RtmpData **pkt)
{
    struct timeval now;
    struct timespec outtime;
    int rc = 0;
    pthread_mutex_lock(&q_mutex_);
    while(nData==0&& rc == 0){
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + 1;  //delay 1 seconds
        outtime.tv_nsec = now.tv_usec * 1000;
        rc = pthread_cond_timedwait(&cond_get_,&q_mutex_,&outtime);
    }
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

    pthread_mutex_unlock(&q_mutex_);
    return 0;
}

int RtmpDataQueue::flush(){
    pthread_mutex_lock(&q_mutex_);
    while(head_){
        RtmpData *pkt = head_->next;
        RtmpData::rtmp_data_free(head_);
        head_ = pkt;
    }
    head_ = tail_ = NULL;
    nData = 0;
    video_key_frame_got = 0;
    pthread_mutex_unlock(&q_mutex_);
    return 0;
}

