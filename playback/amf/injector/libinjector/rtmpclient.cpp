#include "rtmpclient.h"
#include "rtmpclient_impl.h"

RtmpClient::RtmpClient(){
    impl_ = new RtmpClientImpl;
}
RtmpClient::~RtmpClient(){
    delete impl_;
}
int RtmpClient::addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate){
    return impl_->addVideoH264Stream(extra_data,extra_data_len,bitrate);
}
int RtmpClient::addAudioAACStream(int sample_rate,int channels,int bitrate){
    return impl_->addAudioAACStream(sample_rate,channels,bitrate);
}
int RtmpClient::addAudioG711Stream(int isALaw){
    return impl_->addAudioG711Stream(isALaw);
}
int RtmpClient::setDestination(char *const rtmpUrl){
    return impl_->setDestination(rtmpUrl);
}
int RtmpClient::sendData(RtmpClient::DataType type,unsigned char *data,int len,unsigned int timestamp){
    return impl_->sendData(type,data,len,timestamp);
}

