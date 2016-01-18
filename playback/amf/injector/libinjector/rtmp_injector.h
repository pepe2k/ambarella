#ifndef _RTMP_INJECTOR_H__
#define _RTMP_INJECTOR_H__

#include <string.h>
#include "rtmpclient.h" //for RtmpClient::DataType, IRtmpEventHandler
#include "rtmp_active_object.h"

using namespace AMBA_RTMP;

class IRtmpEventHandler{
public:
    virtual ~IRtmpEventHandler(){}
    virtual void onPeerClose() = 0;
};

class RtmpDataQueue;
class RtmpBuffer;
class RtmpData;
class RtmpInjector:public AO_Proxy,public IRtmpEventHandler{
public:
    RtmpInjector(int queue_size = 100);
    ~RtmpInjector();
    //void setEventHandler(IRtmpEventHandler *handler){handler_ = handler;}
    int addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate);
    int addAudioAACStream(int sample_rate,int channels,int bitrate);
    int addAudioG711Stream(int isALaw);
    int setDestination(char *const rtmpUrl);
    int sendData(RtmpClient::DataType type,unsigned char *data,int len,unsigned int timestamp);
    virtual void onPeerClose();
private:
    void setExitFlag(int flag){
        pthread_mutex_lock(&mutex_);
        exit_flag_ = flag;
        pthread_mutex_unlock(&mutex_);
    }
    int getExitFlag(){
         int flag;
         pthread_mutex_lock(&mutex_);
         flag = exit_flag_;
         pthread_mutex_unlock(&mutex_);
         return flag;
    }
    void enableReconnect(int flag){
        pthread_mutex_lock(&mutex_);
        enable_reconect_ = flag;
        pthread_mutex_unlock(&mutex_);
    }
    int isReconnectEnabled(){
         int flag;
         pthread_mutex_lock(&mutex_);
         flag = enable_reconect_;
         pthread_mutex_unlock(&mutex_);
         return flag;
    }
private:
    int write_header();
    int write_h264_header();
    int write_aac_header();
    int write_g711_header();
    int write_packet(RtmpData *pkt);
    int write_h264_packet(RtmpData *pkt);
    int write_aac_packet(RtmpData *pkt);
    int write_g711_packet(RtmpData *pkt);
    int write_g711_frame(unsigned char *buf,int len,unsigned int timestamp);
private:
    //IRtmpEventHandler *handler_;
    pthread_mutex_t mutex_;
    int exit_flag_;
    int enable_reconect_;
    Future<int> *m_future_;
    RtmpDataQueue *queue_;
    unsigned char *m_tmpBuffer;
    char *rtmpUrl_;

    struct video_param{
        int codec_id;
        int width;
        int height;
        double framerate;
        int bitrate;
        unsigned char *extra_data;
        int extra_data_len;
    }*video_enc;

    struct audio_param{
        RtmpClient::DataType  codec_id;
        int bitrate;
        int sample_rate;
        int sample_size;
        int channels;
        unsigned char *extra_data;
        int extra_data_len;
    }*audio_enc;

    RtmpBuffer *rtmp_;
    unsigned char g711_buffer[160];
    int g711_data_size;
};

#endif //_RTMP_INJECTOR_H__

