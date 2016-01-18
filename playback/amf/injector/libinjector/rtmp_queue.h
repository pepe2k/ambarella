#ifndef _RTMP_QUEUE_H_
#define _RTMP_QUEUE_H_

#include <string.h>
#include <pthread.h>
#include "rtmpclient.h"

struct RtmpData{
    RtmpClient::DataType type;
    unsigned char *data;
    int len;
    unsigned int timestamp;
    int keyframe;
    RtmpData *next;

    static RtmpData* rtmp_data_alloc(RtmpClient::DataType  type_,unsigned char *data_,int len_,int keyframe = 1){
        RtmpData *pkt = new RtmpData;
        if(!pkt) return NULL;
        int alloc_size = (len_ + 15)/16*16;
        pkt->data = new unsigned char[alloc_size];
        if(!pkt->data){
            delete pkt;
            return NULL;
        }
        memcpy(pkt->data,data_,len_);
        pkt->len = len_;
        pkt->type = type_;
        pkt->timestamp = 0; //TODO
        pkt->keyframe = keyframe;
        pkt->next = NULL;
        return pkt;
    }
    static void rtmp_data_free(RtmpData *&pkt){
        if(pkt){
            if(pkt->data) {
                delete []pkt->data;
                pkt->data = NULL;
            }
            delete pkt;
            pkt = NULL;
       }
    }
private:
    RtmpData():type(RtmpClient::TYPE_INVALID),data(NULL),len(0),timestamp(0){}
    ~RtmpData(){}
};

class RtmpDataQueue{
public:
    RtmpDataQueue(int queue_size = 100);
    ~RtmpDataQueue();
    void putq(RtmpData *data);
    int getq(RtmpData **data);
    int flush();
    int notify_exit(int value = 1);
private:
    void discard_pkts();
    int is_video_pkt(RtmpData *pkt){
        return (pkt->type >= RtmpClient::TYPE_H264 && pkt->type < RtmpClient::TYPE_AAC);
    }
private:
    pthread_mutex_t q_mutex_;
    pthread_cond_t cond_get_;
    RtmpData *head_;
    RtmpData *tail_;
    int sizeQueue;
    int nData;
    int exit_flag;
    int video_key_frame_got;
private:
    unsigned long prev_msecs;
    unsigned long prev_timestamp;
    void calcTimestamp(RtmpData *pkt);
};
#endif //_RTMP_QUEUE_H_

