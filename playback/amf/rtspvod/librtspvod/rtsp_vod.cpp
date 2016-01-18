#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "rtsp_vod.h"
#include "rtsp_service_impl.h"

//
//RtspMediaSession implmentation
//
int RtspMediaSession::start_rtsp_server(int rtspPort){
    return RtspService::instance()->start_service(rtspPort);
}

void RtspMediaSession::stop_rtsp_server(){
    return RtspService::instance()->stop_service();
}

RtspMediaSession::RtspMediaSession(){
    impl_ = new RtspMediaSessionImpl();
}

RtspMediaSession::~ RtspMediaSession(){
    delete impl_,impl_ = NULL;
}

int RtspMediaSession::addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate){
    return impl_->addVideoH264Stream(extra_data,extra_data_len,bitrate);
}

int RtspMediaSession::addAudioAACStream(int sample_rate,int channels,int bitrate){
    return impl_->addAudioAACStream(sample_rate,channels,bitrate);
}

int RtspMediaSession::addAudioG711Stream(int isALaw){
    return impl_->addAudioG711Stream(isALaw);
}

int RtspMediaSession::addAudioG726Stream(int bitrate){
    return impl_->addAudioG726Stream(bitrate);
}

int RtspMediaSession::setDataSourceType(DataSourceType type){
    return impl_->setDataSourceType(type);
}

int RtspMediaSession::setDestination(char *const rtspStreamName){
    return impl_->setDestination(rtspStreamName);
}

int RtspMediaSession::sendData(DataType type,unsigned char *data,int len,long long  timestamp){
    return impl_->sendData(type,data,len,timestamp);
}

int RtspMediaSession::setDestination(char *const rtspStreamName, char *const inputAddr,int inputPort,int isRawUdp){
    return impl_->setDestination(rtspStreamName,inputAddr,inputPort,isRawUdp);
}

class MyHashTableImpl{
public:
    MyHashTableImpl(){
        table_ = HashTable::create(STRING_HASH_KEYS);
    }
    ~MyHashTableImpl(){
        delete table_,table_ = NULL;
    }
    void* Add(char const* key, void* value){
        return table_->Add(key,value);
    }
    Boolean Remove(char const* key){
        return table_->Remove(key);
    }
    void* Lookup(char const* key) const{
        return table_->Lookup(key);
    }
    void* RemoveNext(){
        return table_->RemoveNext();
    }
    void* getFirst(){
        return table_->getFirst();
    }
private:
    HashTable *table_;
};

MyHashTable::MyHashTable(){
    impl_ = new MyHashTableImpl();
}
MyHashTable::~MyHashTable(){
    delete impl_,impl_ = NULL;
}
void* MyHashTable::Add(char const* key, void* value){
    return impl_->Add(key,value);
}

void MyHashTable::Remove(char const* key){
    impl_->Remove(key);
}

void* MyHashTable::Lookup(char const* key) const{
    return impl_->Lookup(key);
}

void* MyHashTable::RemoveNext(){
    return impl_->RemoveNext();
}
void* MyHashTable::getFirst(){
    return impl_->getFirst();
}

