#ifndef __RTSP_SERVICE_IMPL_H_
#define __RTSP_SERVICE_IMPL_H_

#include "liveMedia.hh"
#include "strDup.hh"
#include "BasicUsageEnvironment.hh"
#include "rtsp_vod.h"
#include "rtsp_queue.h"

typedef void (*sigFrameDataFunc)(void*);
class MyRtspServer;
class MyDataSource;
class RtspService{
public:
    static RtspService *instance();
    int start_service(int rtspPort = 554);
    void stop_service();//delete self

    void registerDataSource(char *const streamName,MyDataSource *ds);
    void unregisterDataSource(char *const streamName);

    int isStreamExist(char *streamName);
    int getAVTypes(char *streamName,RtspMediaSession::DataType &video_type_,RtspMediaSession::DataType &audio_type_);
    int getVideoParameters(char *streamName,unsigned char *&extra_data,int &extra_data_len);
    int getAudioParameters(char *streamName,int &profile,int &sampleFrequency,int &channels,int &bitrate);
    int flushStreamSource(char *streamName,int is_video,int exit_flag = 0);
    int getVideoFrame(char *const streamName,unsigned char *To, unsigned maxLen,unsigned &frameSize,int &isLive);
    int getVideoFrame(char *const streamName,RtspData *&item,int &isLive);
    int getAudioFrame(char *const streamName,unsigned char *To, unsigned maxLen,unsigned &frameSize,int &isLive);
    int getAudioFrame(char *const streamName,RtspData *&item,int &isLive);
    int registerTsOverUdp(char *const streamName, char *inputAddr,int inputPort,int isRawUdp);
    void unregisterTsOverUdp(char *const streamName);
    int getTsOverUdpInfo(char *const streamName,char *&inputAddr,int &inputPort,int &isRawUdp);

    //video
    int registerSigFrameData(char *const streamName, sigFrameDataFunc func,void *arg);
    void unregisterSigFrameData(char *const streamName);
    int signalFrameData(char *const streamName);

    //audio
    int registerSigFrameDataAudio(char *const streamName, sigFrameDataFunc func,void *arg);
    void unregisterSigFrameDataAudio(char *const streamName);
    int signalFrameDataAudio(char *const streamName);
private:
    RtspService();
    ~RtspService();
private:
    static RtspService *m_instance;
    int is_started;
    MyRtspServer *rtspServer_;
    HashTable *hash_table_;
    struct TsOverUdpInfo{
        TsOverUdpInfo(char *inputAddress_,int inputPort_,int isRawUdp_):inputPort(inputPort_),isRawUdp(isRawUdp_){
            if(inputAddress_){
                inputAddress = strDup(inputAddress_);
            }else{
                inputAddress = NULL;
            }
        }
        ~TsOverUdpInfo(){
            if(inputAddress)  delete[]inputAddress;
        }
        char *inputAddress;
        int inputPort;
        int isRawUdp;
    };
    HashTable *hash_table_ts_;

    struct SigParam{
        sigFrameDataFunc func;
        void *arg;
    };
    HashTable *hash_table_sig_;
    HashTable *hash_table_sig_audio_;
};

class RtspMediaSessionImpl{
public:
    RtspMediaSessionImpl();
    ~ RtspMediaSessionImpl();
public:
    int addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate = 0);
    int addAudioAACStream(int sample_rate,int channels,int bitrate = 0);
    int addAudioG711Stream(int isALaw);
    int addAudioG726Stream(int bitrate);
    int setDestination(char *const rtspStreamName);
    int setDataSourceType(RtspMediaSession::DataSourceType type);
    int sendData(RtspMediaSession::DataType type,unsigned char *data,int len,long long timestamp);
    int setDestination(char *const rtspStreamName, char *const inputAddr,int inputPort,int isRawUdp);
private:
    char *streamName;
    MyDataSource *ds;

    unsigned char *video_extra_data;
    int video_extra_data_len;
    int video_bitrate;

    RtspMediaSession::DataType  audio_type;
    int audio_sample_rate;
    int audio_channels;
    int audio_bitrate;

    char *streamNameTs;//for TsOverUdp/RTP
};

class MyDataSource{
public:
    MyDataSource();
    ~MyDataSource();
public:
    int flush(int is_video,int exit_flag);
    int addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate);
    int addAudioAACStream(int sample_rate,int channels,int bitrate);
    int addAudioG711Stream(int isALaw);
    int addAudioG726Stream(int bitrate);
    int setDataSourceType(RtspMediaSession::DataSourceType type);
    int sendData(RtspMediaSession::DataType type,unsigned char *data,int len,long long  timestamp);
    int getAVTypes(RtspMediaSession::DataType &video_type,RtspMediaSession::DataType &audio_type);
    int getVideoParameters(unsigned char *&extra_data,int &extra_data_len);
    int getAudioParameters(int &profile,int &sampleFrequency,int &channelConfiguration,int &bitrate);
    int getVideoFrame(unsigned char *buf, unsigned maxLen,unsigned &frameSize,int &isLive);
    int getVideoFrame(RtspData *&item,int &isLive);
    int getAudioFrame(unsigned char *buf,unsigned maxLen,unsigned &frameSize,int &isLive);
    int getAudioFrame(RtspData *&item,int &isLive);
private:
    int sendDataH264(unsigned char *data,int len,long long  timestamp);
    int sendDataAAC(unsigned char *data,int len,long long  timestamp);
    int sendDataG711A(unsigned char *data,int len,long long  timestamp);
    int sendDataG711Mu(unsigned char *data,int len,long long  timestamp);
    int sendDataG726(unsigned char *data,int len,long long timestamp);
private:
    RtspDataQueue *video_queue_;
    enum {MAX_VIDEO_FRAME_SIZE = 300 * 1024};
    unsigned char *video_tmp_buffer_;
    int video_pos;
    unsigned video_tmp_size;
    unsigned char *video_extra_data;
    int video_extra_data_len;
    int video_bitrate;

    RtspDataQueue *audio_queue_;
    RtspMediaSession::DataType  audio_type;
    int audio_sample_rate;
    int audio_channels;
    int audio_bitrate;

    pthread_mutex_t mutex_;
    RtspMediaSession::DataSourceType ds_type;
};

class MyMap{
public:
    MyMap();
    ~MyMap();
    int get_and_inc(char *streamName);
    int dec_and_get(char *streamName);
private:
    struct node_t{
        char *streamName;
        int ref_count;
        struct node_t *next;
    };
    struct node_t *head_,*tail_;
};

#endif //__RTSP_SERVICE_IMPL_H_

