#include "rtmpclient_impl.h"
RtmpClientImpl::RtmpClientImpl(){
    injector_ = new RtmpInjector();
}
RtmpClientImpl::~RtmpClientImpl(){
    delete injector_;
}
int RtmpClientImpl::addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate){
    return injector_->addVideoH264Stream(extra_data,extra_data_len,bitrate);
}
int RtmpClientImpl::addAudioAACStream(int sample_rate,int channels,int bitrate){
    return injector_->addAudioAACStream(sample_rate,channels,bitrate);
}
int RtmpClientImpl::addAudioG711Stream(int isALaw){
    return injector_->addAudioG711Stream(isALaw);
}
int RtmpClientImpl::setDestination(char *const rtmpUrl){
    return injector_->setDestination(rtmpUrl);
}
int RtmpClientImpl::sendData(RtmpClient::DataType type,unsigned char *data,int len,unsigned int timestamp){
    return injector_->sendData(type,data,len,timestamp);
}

