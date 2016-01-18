#ifndef _RTSP_QUEUE_H_
#define _RTSP_QUEUE_H_

#include <string.h>
#include "BasicUsageEnvironment.hh"
#include "rtsp_vod.h"

struct RtspData{
    RtspMediaSession::DataType type;
    unsigned char *data;
    int len;
    long long timestamp;
    struct timeval ts;
    RtspData *next;

    static RtspData* rtsp_data_alloc(RtspMediaSession::DataType  type_,unsigned char *data_,int len_){
        int alloc_size = (sizeof(RtspData) + len_ + 15)/16*16;
        unsigned char *mem = new unsigned char [alloc_size];
        if(!mem){
            return NULL;
        }
        RtspData *pkt = (RtspData *)mem;
        pkt->len = len_;
        pkt->data = mem + sizeof(RtspData);
        pkt->type = type_;
        pkt->next = NULL;
        memcpy(pkt->data,data_,len_);
        return pkt;
    }
    static void rtsp_data_free(RtspData *&pkt){
       if(pkt){
           delete [](unsigned char *)pkt;
           pkt = NULL;
       }
    }
private:
    RtspData():type(RtspMediaSession::TYPE_INVALID),data(NULL),len(0),next(NULL){}
    ~RtspData(){}
};

class RtspDataQueue{
public:
    RtspDataQueue(int queue_size = 30);
    ~RtspDataQueue();
    void putq(RtspMediaSession::DataType type,unsigned char *data,int len,long long timestamp,int sample_rate);
    int getq(RtspData **data,int &isLive);
    int flush();
    int notify_exit(int value = 1);
    int set_ds_type(int is_live = 1);
private:
    void discard_pkts();
    int is_video_pkt(RtspData *pkt){
        return (pkt->type >= RtspMediaSession::TYPE_H264 && pkt->type < RtspMediaSession::TYPE_AAC);
    }
    int exit_flag(){
        int flag;
        pthread_mutex_lock(&flag_mutex_);
        flag = exit_flag_;
        pthread_mutex_unlock(&flag_mutex_);
        return flag;
    }
private:
    pthread_mutex_t q_mutex_;
    pthread_cond_t cond_get_;
    RtspData *head_;
    RtspData *tail_;
    int sizeQueue;
    int nData;
    pthread_mutex_t flag_mutex_;
    int exit_flag_;
    int video_key_frame_got;
    int is_ds_type_live_;
    int64_t  base_ts;
    struct timeval ptAdjustment;
};
#endif //_RTMP_QUEUE_H_

