#ifndef _RTMP_CLIENT_IMPL_H_
#define _RTMP_CLIENT_IMPL_H_

#include "rtmpclient.h"
#include "rtmp_injector.h"

class RtmpClientImpl{
public:
    RtmpClientImpl();
    ~RtmpClientImpl();
public:
    int addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate = 0);
    int addAudioAACStream(int sample_rate,int channels,int bitrate = 0);
    int addAudioG711Stream(int isALaw);
    int setDestination(char *const rtmpUrl);
    int sendData(RtmpClient::DataType type,unsigned char *data,int len,unsigned int timestamp);
private:
    RtmpInjector *injector_;
};
#endif //_RTMP_CLIENT_IMPL_H_

