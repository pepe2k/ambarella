#include "rtsp_service_impl.h"
#include "rtsp_active_object.h"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "DynamicRTSPServer.hh"

using namespace NvrRtspServer;

class MyRtspServer:public AO_Proxy{
public:
    MyRtspServer():AO_Proxy(){
        eventLoopWatchVariable = 0;
        scheduler_ = BasicTaskScheduler::createNew();
        env_ = BasicUsageEnvironment::createNew(*scheduler_);
        proxy_running = 0;
        m_future_ = new Future<int>;
        eventTriggerIdExit = env_->taskScheduler().createEventTrigger(eventExitService0);
        eventTriggerId = env_->taskScheduler().createEventTrigger(eventHandler0);
        pthread_mutex_init(&q_mutex_,NULL);
        linkhead_ = linktail_= NULL;
        ao_start_service();
    }
    ~MyRtspServer(){
        env_->taskScheduler().triggerEvent(this->eventTriggerIdExit, this);
        ao_exit_service();
        pthread_mutex_destroy(&q_mutex_);
        env_->taskScheduler().deleteEventTrigger(eventTriggerId),eventTriggerId = 0;
        env_->taskScheduler().deleteEventTrigger(eventTriggerIdExit),eventTriggerIdExit = 0;
        env_->reclaim(); env_ = NULL;
        delete scheduler_; scheduler_ = NULL;
    }
    static void eventExitService0(void* clientData) {
        ((MyRtspServer*)clientData)->eventExitService();
    }
    void eventExitService(){
        // Signal the event loop that we're done:
        this->eventLoopWatchVariable = ~0;
    }
    static void eventHandler0(void* clientData) {
        ((MyRtspServer*)clientData)->eventHandler();
    }
    void eventHandler(){
        pthread_mutex_lock(&q_mutex_);
        while(linkhead_){
            linkNode *node = linkhead_->next;
            //printf("MyRtspServer::eventHandler() -- deleteServerMediaSession ---streamName %s\n",linkhead_->streamName);
            this->rtspServer->deleteServerMediaSession(linkhead_->streamName);
            delete linkhead_;
            linkhead_ = node;
        }
        linktail_ = NULL;
        pthread_mutex_unlock(&q_mutex_);
    }
    void deleteServerMediaSession(char *streamName) {
        linkNode *node = new linkNode(streamName);
        pthread_mutex_lock(&q_mutex_);
        if(linktail_){
            linktail_->next = node;
        }
        linktail_ = node;
        if(!linkhead_){
            linkhead_ = linktail_;
        }
        pthread_mutex_unlock(&q_mutex_);
        env_->taskScheduler().triggerEvent(this->eventTriggerId, this);
    }

    int open(int rtspPort = 554){
        class Open:public Method_Request{
        public:
            Open(MyRtspServer *proxy,int rtspPort):proxy_(proxy),rtspPort_(rtspPort){
            }
            virtual bool guard(){return true;}
            virtual int call(){
                if(proxy_->proxy_running){
                    int value = 0;
                    proxy_->m_future_->set(value);
                    return 0;
                }
                UserAuthenticationDatabase* authDB = NULL;
            #ifdef ACCESS_CONTROL
                // To implement client access control to the RTSP server, do the following:
                authDB = new UserAuthenticationDatabase;
                authDB->addUserRecord("username1", "password1"); // replace these with real strings
                // Repeat the above with each <username>, <password> that you wish to allow
                // access to the server.
            #endif
                // Create the RTSP server.  Try first with the default port number (554),
                // and then with the alternative port number (8554):
                portNumBits rtspServerPortNum = (portNumBits)rtspPort_;
                proxy_->rtspServer = DynamicRTSPServer::createNew(*proxy_->env_, rtspServerPortNum, authDB);
                if (proxy_->rtspServer == NULL) {
                    rtspServerPortNum = 8554;
                    proxy_->rtspServer = DynamicRTSPServer::createNew(*proxy_->env_, rtspServerPortNum, authDB);
                }
                if (proxy_->rtspServer == NULL) {
                    //*proxy_->env_ << "Failed to create RTSP server: " << proxy_->env_->getResultMsg() << "\n";
                    printf("Failed to create RTSP server: %s\n",proxy_->env_->getResultMsg());
                    int value = -1;
                    proxy_->m_future_->set(value);
                    return 0;
                }
                char* urlPrefix = proxy_->rtspServer->rtspURLPrefix();
                //*proxy_->env_ << "Play streams from this server using the URL\n\t"<< urlPrefix << "<streamName>\n";
                printf("RtspServer created, Play streams from this server using the URL[%s<streamName>]\n",urlPrefix);

                proxy_->proxy_running = 1;
                int value = 0;
                proxy_->m_future_->set(value);

                //the function will not exit until ExitFlag set.
                proxy_->env_->taskScheduler().doEventLoop(&proxy_->eventLoopWatchVariable);
                return 0;
            }
        private:
            MyRtspServer *proxy_;
            int rtspPort_;
        };
        ao_send_request(new Open(this,rtspPort));
        int value = m_future_->get();
        m_future_->reset();
        return value;
    }
private:
    TaskScheduler* scheduler_;
    UsageEnvironment* env_;
    int proxy_running;
    RTSPServer* rtspServer;
    Future<int> *m_future_;
    //exit rtspServer eventloop
    char eventLoopWatchVariable;
    EventTriggerId eventTriggerIdExit;
    //removeMediaSession
    EventTriggerId eventTriggerId;
    struct linkNode{
        linkNode(char *streamName_){
            streamName = strDup(streamName_);
            next = NULL;
        }
        ~linkNode(){
            delete []streamName;
        }
        char *streamName;
        struct linkNode *next;
    } *linkhead_,*linktail_;
    pthread_mutex_t q_mutex_;
};

//
//Impl definition and implmentation
//
pthread_mutex_t rtsp_service_mutex_ = PTHREAD_MUTEX_INITIALIZER;
RtspService *RtspService::m_instance = NULL;
RtspService *RtspService::instance(){
    if(m_instance == NULL){
        pthread_mutex_lock(&rtsp_service_mutex_);
        if(m_instance == NULL){
            m_instance = new RtspService();
        }
        pthread_mutex_unlock(&rtsp_service_mutex_);
    }
    return m_instance;
}

RtspService::RtspService():is_started(0),rtspServer_(NULL){
    hash_table_ = HashTable::create(STRING_HASH_KEYS);
    hash_table_ts_ = HashTable::create(STRING_HASH_KEYS);
    hash_table_sig_ = HashTable::create(STRING_HASH_KEYS);
    hash_table_sig_audio_ = HashTable::create(STRING_HASH_KEYS);
}
RtspService::~RtspService(){
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_)	delete hash_table_,hash_table_ = NULL;
    //
    //TODO free TsOverUdpInfo *
    //
    if(hash_table_ts_)	delete hash_table_ts_,hash_table_ts_ = NULL;

    //TODO, free hash_table_sig_
    //TODO, free hash_table_sig_audio_
    if(rtspServer_)	delete rtspServer_,rtspServer_ = NULL;
    m_instance = NULL;
    pthread_mutex_unlock(&rtsp_service_mutex_);
}

int RtspService::start_service(int rtspPort){
    int value = -1;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(is_started){
        value = 0;
    }else{
        if(!rtspServer_){
            rtspServer_ = new MyRtspServer();
        }
        if(rtspServer_){
            value = rtspServer_->open(rtspPort);
            if(value < 0){
                delete rtspServer_;
                rtspServer_ = NULL;
            }else{
                is_started = 1;
            }
        }	
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return value;
}
void RtspService::stop_service(){
    //remove_all_sessions();
    delete this;
}
void RtspService::registerDataSource(char *const streamName,MyDataSource *ds){
    if(streamName && ds){
        unregisterDataSource(streamName);
        pthread_mutex_lock(&rtsp_service_mutex_);
        if(hash_table_){
            hash_table_->Add(streamName,ds);
        }
        pthread_mutex_unlock(&rtsp_service_mutex_);
    }
}
void RtspService::unregisterDataSource(char *const streamName){
    pthread_mutex_lock(&rtsp_service_mutex_);
    /*
    MyDataSource *ds = (MyDataSource*)hash_table_->Lookup(streamName);
    if(ds){
        delete ds;
    }
    */
    if(hash_table_){
        hash_table_->Remove(streamName);
    }
    rtspServer_->deleteServerMediaSession(streamName);
    pthread_mutex_unlock(&rtsp_service_mutex_);
}
int RtspService::isStreamExist(char *streamName){
    MyDataSource *ds = NULL;
    int value = 0;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_){
        ds = (MyDataSource*)hash_table_->Lookup(streamName);
        if(ds){
            value = 1;
        }
    }
    if(!value && hash_table_ts_){
        TsOverUdpInfo *info = (TsOverUdpInfo*)hash_table_ts_->Lookup(streamName);
        if(info){
            value = 1;
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return value;
};

int RtspService::flushStreamSource(char *streamName,int is_video,int exit_flag){
    MyDataSource *ds = NULL;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_){
        ds = (MyDataSource*)hash_table_->Lookup(streamName);
        if(ds){
            ds->flush(is_video,exit_flag);
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return 0;
}

int RtspService::getAVTypes(char *streamName,RtspMediaSession::DataType &video_type_,RtspMediaSession::DataType &audio_type_){
    MyDataSource *ds = NULL;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_){
        ds = (MyDataSource*)hash_table_->Lookup(streamName);
        if(ds){
            ds->getAVTypes(video_type_,audio_type_);
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return (ds != NULL) ? 0:-1;
}

int RtspService::getVideoParameters(char *streamName,unsigned char *&extra_data,int &extra_data_len){
    MyDataSource *ds = NULL;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_){
        ds = (MyDataSource*)hash_table_->Lookup(streamName);
        if(ds){
            ds->getVideoParameters(extra_data,extra_data_len);
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return (ds != NULL) ? 0:-1;
}
int RtspService::getAudioParameters(char *streamName,int &profile,int &sampleFrequency,int &channels,int &bitrate){
    MyDataSource *ds = NULL;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_){
        ds = (MyDataSource*)hash_table_->Lookup(streamName);
        if(ds){
            ds->getAudioParameters(profile,sampleFrequency,channels,bitrate);
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return (ds != NULL) ? 0:-1;
}

int RtspService::getVideoFrame(char *const streamName,unsigned char *To, unsigned maxLen,unsigned &frameSize,int &isLive){
    MyDataSource *ds = NULL;
    int value = -1;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_){
        ds = (MyDataSource*)hash_table_->Lookup(streamName);
        if(ds){
            value = ds->getVideoFrame(To,maxLen,frameSize,isLive);
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return value;
}
int RtspService::getVideoFrame(char *const streamName, RtspData *&item,int &isLive){
    MyDataSource *ds = NULL;
    int value = -1;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_){
        ds = (MyDataSource*)hash_table_->Lookup(streamName);
        if(ds){
            value = ds->getVideoFrame(item,isLive);
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return value;
}

int RtspService::getAudioFrame(char *const streamName,unsigned char *To,unsigned maxLen,unsigned &frameSize,int &isLive){
    MyDataSource *ds = NULL;
    int value = -1;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_){
        ds = (MyDataSource*)hash_table_->Lookup(streamName);
        if(ds){
            value = ds->getAudioFrame(To,maxLen,frameSize,isLive);
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return value;
}

int RtspService::getAudioFrame(char *const streamName, RtspData *&item,int &isLive){
    MyDataSource *ds = NULL;
    int value = -1;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_){
        ds = (MyDataSource*)hash_table_->Lookup(streamName);
        if(ds){
            value = ds->getAudioFrame(item,isLive);
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return value;
}

int RtspService::registerTsOverUdp(char *const streamName, char *inputAddr,int inputPort,int isRawUdp){
    if(!streamName) return -1;
    unregisterTsOverUdp(streamName);
    TsOverUdpInfo *info = new TsOverUdpInfo(inputAddr,inputPort,isRawUdp);
    if(!info) return -1;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_ts_){
        hash_table_ts_->Add(streamName,info);
        printf("RtspService::registerTsOverUdp -- streamName %s, inputPort %d -- registered\n",streamName,inputPort);
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return 0;
}

void RtspService::unregisterTsOverUdp(char *const streamName){
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_ts_){
        TsOverUdpInfo *info = (TsOverUdpInfo*)hash_table_ts_->Lookup(streamName);
        if(info){
            delete info;
        }
        hash_table_ts_->Remove(streamName);
    }
    rtspServer_->deleteServerMediaSession(streamName);
    pthread_mutex_unlock(&rtsp_service_mutex_);
}

int RtspService::getTsOverUdpInfo(char *const streamName,char *&inputAddr,int &inputPort,int &isRawUdp){
    TsOverUdpInfo *info = NULL;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_ts_){
        info = (TsOverUdpInfo*)hash_table_ts_->Lookup(streamName);
        if(info){
            if(info->inputAddress) inputAddr = strDup(info->inputAddress);
            else inputAddr  = NULL;
            inputPort = info->inputPort;
            isRawUdp = info->isRawUdp;
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return (info != NULL) ? 0:-1;
}

 int RtspService::registerSigFrameData(char *const streamName, sigFrameDataFunc func,void *arg){
    if(!streamName) return -1;
    unregisterSigFrameData(streamName);
    SigParam *info = new SigParam();
    if(!info) return -1;
    info->func = func;
    info->arg = arg;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_sig_){
        hash_table_sig_->Add(streamName,info);
        printf("RtspService::registerSigFrameData(%s) \n",streamName);
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return 0;
 }
 void RtspService::unregisterSigFrameData(char *const streamName){
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_sig_){
        SigParam *info = (SigParam*)hash_table_sig_->Lookup(streamName);
        if(info){
            printf("RtspService::unregisterSigFrameData(%s) \n",streamName);
            delete info;
        }
        hash_table_sig_->Remove(streamName);
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
 }
 int RtspService::signalFrameData(char *const streamName){
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_sig_){
        SigParam *info = (SigParam*)hash_table_sig_->Lookup(streamName);
        if(info){
            info->func(info->arg);
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return 0;
 }

 int RtspService::registerSigFrameDataAudio(char *const streamName, sigFrameDataFunc func,void *arg){
    if(!streamName) return -1;
    unregisterSigFrameDataAudio(streamName);
    SigParam *info = new SigParam();
    if(!info){
        printf("RtspService::registerSigFrameDataAudio(%s),failed to alloc memory\n",streamName);
        return -1;
    }
    info->func = func;
    info->arg = arg;
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_sig_audio_){
        hash_table_sig_audio_->Add(streamName,info);
        printf("RtspService::registerSigFrameDataAudio(%s) \n",streamName);
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return 0;
 }
 void RtspService::unregisterSigFrameDataAudio(char *const streamName){
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_sig_audio_){
        SigParam *info = (SigParam*)hash_table_sig_audio_->Lookup(streamName);
        if(info){
            printf("RtspService::unregisterSigFrameDataAudio(%s) \n",streamName);
            delete info;
        }
        hash_table_sig_audio_->Remove(streamName);
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
 }
 
 int RtspService::signalFrameDataAudio(char *const streamName){
    pthread_mutex_lock(&rtsp_service_mutex_);
    if(hash_table_sig_audio_){
        SigParam *info = (SigParam*)hash_table_sig_audio_->Lookup(streamName);
        if(info){
            info->func(info->arg);
        }
    }
    pthread_mutex_unlock(&rtsp_service_mutex_);
    return 0;
}

//Implementation of RtspMediaSessionImpl
RtspMediaSessionImpl::RtspMediaSessionImpl():streamName(NULL),ds(NULL),streamNameTs(NULL){
    video_extra_data = NULL;
    video_extra_data_len = 0;
    video_bitrate = 0;

    audio_type = RtspMediaSession::TYPE_INVALID;
    audio_sample_rate = 0;
    audio_channels = 0;
    audio_bitrate = 0;
}

RtspMediaSessionImpl::~ RtspMediaSessionImpl(){
    if(streamName){
        RtspService::instance()->unregisterDataSource(streamName);
        RtspService::instance()->unregisterSigFrameData(streamName);
        RtspService::instance()->unregisterSigFrameDataAudio(streamName);
        delete[]streamName;
        streamName = NULL;
    }
    if(ds){
        delete ds;
        ds = NULL;
    }
    if(video_extra_data){
        delete []video_extra_data,video_extra_data = NULL;
    }

    if(streamNameTs){
        RtspService::instance()->unregisterTsOverUdp(streamNameTs);
        delete[]streamNameTs;
        streamNameTs = NULL;
    }
}
int RtspMediaSessionImpl::addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate){
    if(video_extra_data) delete []video_extra_data;
    int alloc_size = (extra_data_len + 15)/16*16 + 16;
    video_extra_data = new unsigned char [alloc_size];
    if(!video_extra_data){
        return -1;
    }
    memcpy(video_extra_data,extra_data,extra_data_len);
    video_extra_data_len = extra_data_len;
    video_bitrate = bitrate;
    return 0;
}

int RtspMediaSessionImpl::addAudioAACStream(int sample_rate,int channels,int bitrate){
    audio_type = RtspMediaSession::TYPE_AAC;
    audio_sample_rate = sample_rate;
    audio_channels = channels;
    audio_bitrate = bitrate;
    return 0;
}

int RtspMediaSessionImpl::addAudioG711Stream(int isALaw){
    audio_type = isALaw ? RtspMediaSession::TYPE_G711_A: RtspMediaSession::TYPE_G711_MU;
    audio_sample_rate = 8000;
    audio_channels = 1;
    audio_bitrate = 64000;
    return 0;
}

int RtspMediaSessionImpl::addAudioG726Stream(int bitrate){
    audio_type = RtspMediaSession::TYPE_G726;
    audio_sample_rate = 8000;
    audio_channels = 1;
    if(bitrate != 16000 && bitrate !=24000 && bitrate != 32000 && bitrate !=40000){
        printf("addAudioG726Stream(%d), bitrate error, only 16/24/32/40 kbps supported\n",bitrate);
        return -1;
    }
    audio_bitrate = bitrate;
    return 0;
}

int RtspMediaSessionImpl::setDestination(char *const rtspStreamName){
    if(streamName) delete []streamName;
    streamName = strDup(rtspStreamName);
    ds = new MyDataSource();
    if(!ds){
        return -1;
    }
    if(video_extra_data){
        ds->addVideoH264Stream(video_extra_data,video_extra_data_len,video_bitrate);
    }
    if(audio_type == RtspMediaSession::TYPE_AAC){
        ds->addAudioAACStream(audio_sample_rate,audio_channels,audio_bitrate);
    }else if(audio_type == RtspMediaSession::TYPE_G711_MU){
        ds->addAudioG711Stream(0);
    }else if(audio_type == RtspMediaSession::TYPE_G711_A){
        ds->addAudioG711Stream(1);
    }else if(audio_type == RtspMediaSession::TYPE_G726){
        ds->addAudioG726Stream(audio_bitrate);
    }
    RtspService::instance()->registerDataSource(streamName,ds);
    return 0;
}

int RtspMediaSessionImpl::setDataSourceType(RtspMediaSession::DataSourceType type){
    if(ds){
        ds->setDataSourceType(type);
    }
    return 0;
}

int RtspMediaSessionImpl::sendData(RtspMediaSession::DataType type,unsigned char *data,int len,long long  timestamp){
    if(RtspService::instance()->isStreamExist(streamName)){
        if(ds){
            ds->sendData(type,data,len,timestamp);
            if(type == RtspMediaSession::TYPE_H264){
                RtspService::instance()->signalFrameData(streamName);
            }else{
                RtspService::instance()->signalFrameDataAudio(streamName);
            }
            return 0;
        }
    }
    return -1;
}

int RtspMediaSessionImpl::setDestination(char *const rtspStreamName, char *const inputAddr,int inputPort,int isRawUdp){
    if(streamNameTs) delete []streamNameTs;
    streamNameTs = strDup(rtspStreamName);
    RtspService::instance()->registerTsOverUdp(streamNameTs,inputAddr,inputPort,isRawUdp);
    return 0;
}

//implementatiof of MyDataSource
MyDataSource::MyDataSource(){
    pthread_mutex_init(&mutex_,NULL);
    video_queue_ = new RtspDataQueue();
    video_tmp_buffer_ = new unsigned char[MAX_VIDEO_FRAME_SIZE];
    video_pos = 0;
    video_tmp_size = 0;
    video_extra_data = NULL;
    video_extra_data_len = 0;
    video_bitrate = 0;

    audio_queue_ = new RtspDataQueue();
    audio_type = RtspMediaSession::TYPE_INVALID;
    audio_sample_rate = 0;
    audio_channels = 0;
    audio_bitrate = 0;

    ds_type = RtspMediaSession::DS_TYPE_LIVE;
    video_queue_->set_ds_type();
    audio_queue_->set_ds_type();
}

MyDataSource::~MyDataSource(){
    delete video_queue_, video_queue_ = NULL;
    delete []video_tmp_buffer_, video_tmp_buffer_ = NULL;
    delete audio_queue_, audio_queue_ = NULL;
    pthread_mutex_destroy(&mutex_);
}

int MyDataSource::flush(int is_video,int exit_flag){
    if(is_video){
        video_queue_->flush();
        video_queue_->notify_exit(exit_flag);
        video_tmp_size = 0;
        video_pos = 0;
    }else{
        audio_queue_->flush();
        audio_queue_->notify_exit(exit_flag);
    }
    return 0;
}
int MyDataSource::addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate){
    pthread_mutex_lock(&mutex_);
    if(video_extra_data)  delete []video_extra_data;
    int alloc_size = (extra_data_len + 15)/16*16 + 16;
    video_extra_data = new unsigned char [alloc_size];
    if(!video_extra_data){
        pthread_mutex_unlock(&mutex_);
        return -1;
    }
    memcpy(video_extra_data,extra_data,extra_data_len);
    video_extra_data_len = extra_data_len;
    video_bitrate = bitrate;
    pthread_mutex_unlock(&mutex_);
    return 0;
}

int MyDataSource::addAudioAACStream(int sample_rate,int channels,int bitrate){
    pthread_mutex_lock(&mutex_);
    audio_type = RtspMediaSession::TYPE_AAC;
    audio_sample_rate = sample_rate;
    audio_channels = channels;
    audio_bitrate = bitrate;
    pthread_mutex_unlock(&mutex_);
    return 0;
}

int MyDataSource::addAudioG711Stream(int isALaw){
    pthread_mutex_lock(&mutex_);
    audio_type = isALaw ? RtspMediaSession::TYPE_G711_A: RtspMediaSession::TYPE_G711_MU;
    audio_sample_rate = 8000;
    audio_channels = 1;
    audio_bitrate = 64000;
    pthread_mutex_unlock(&mutex_);
    return 0;
}

int MyDataSource::addAudioG726Stream(int bitrate){
    pthread_mutex_lock(&mutex_);
    audio_type = RtspMediaSession::TYPE_G726;
    audio_sample_rate = 8000;
    audio_channels = 1;
    audio_bitrate = bitrate;
    pthread_mutex_unlock(&mutex_);
    return 0;
}

int MyDataSource::getAVTypes(RtspMediaSession::DataType &video_type_,RtspMediaSession::DataType &audio_type_){
    pthread_mutex_lock(&mutex_);
    video_type_ = RtspMediaSession::TYPE_INVALID;
    audio_type_ = RtspMediaSession::TYPE_INVALID;
    if(video_extra_data){
        video_type_ = RtspMediaSession::TYPE_H264;
    }
    if(audio_type != RtspMediaSession::TYPE_INVALID){
        audio_type_ = audio_type;
    }
    pthread_mutex_unlock(&mutex_);
    return 0;
}

int MyDataSource::setDataSourceType(RtspMediaSession::DataSourceType type){
    pthread_mutex_lock(&mutex_);
    ds_type = type;
    if(ds_type == RtspMediaSession::DS_TYPE_LIVE){
        video_queue_->set_ds_type();
        audio_queue_->set_ds_type();
    }else{
        video_queue_->set_ds_type(0);
        audio_queue_->set_ds_type(0);
    }
    pthread_mutex_unlock(&mutex_);
    return 0;
}
int MyDataSource::sendData(RtspMediaSession::DataType type,unsigned char *data,int len,long long timestamp){
    switch(type){
    case RtspMediaSession::TYPE_H264:sendDataH264(data,len,timestamp);break;
    case RtspMediaSession::TYPE_AAC:sendDataAAC(data,len,timestamp);break;
    case RtspMediaSession::TYPE_G711_A:sendDataG711A(data,len,timestamp);break;
    case RtspMediaSession::TYPE_G711_MU:sendDataG711Mu(data,len,timestamp);break;
    case RtspMediaSession::TYPE_G726:sendDataG726(data,len,timestamp);break;
    default:return -1;
    }
    return 0;
}

int MyDataSource::sendDataH264(unsigned char *data,int len,long long timestamp){
    video_queue_->putq(RtspMediaSession::TYPE_H264,data,len ,timestamp,90000);
    return 0;
}

int MyDataSource::sendDataAAC(unsigned char *data,int len,long long timestamp){
    //remove 7 bytes header
    if(len <= 7){
        return -1;
    }
    audio_queue_->putq(RtspMediaSession::TYPE_AAC,data + 7,len - 7,timestamp,audio_sample_rate);
    return 0;
}

int MyDataSource::sendDataG711A(unsigned char *data,int len,long long timestamp){
    audio_queue_->putq(RtspMediaSession::TYPE_G711_A,data,len,timestamp,audio_sample_rate);
    return 0;
}

int MyDataSource::sendDataG711Mu(unsigned char *data,int len,long long timestamp){
    audio_queue_->putq(RtspMediaSession::TYPE_G711_MU,data,len,timestamp,audio_sample_rate);
    return 0;
}

int MyDataSource::sendDataG726(unsigned char *data,int len,long long timestamp){
    audio_queue_->putq(RtspMediaSession::TYPE_G726,data,len,timestamp,audio_sample_rate);
    return 0;
}

int MyDataSource::getVideoParameters(unsigned char *&extra_data,int &extra_data_len){
    pthread_mutex_lock(&mutex_);
    extra_data_len = 0;
    extra_data = NULL;
    if(video_extra_data_len){
        unsigned char *extra_data_ = new unsigned char [(video_extra_data_len + 15)/16 * 16];
        if(extra_data_){
            memcpy(extra_data_,video_extra_data,video_extra_data_len);
            extra_data_len = video_extra_data_len;
            extra_data = extra_data_;
        }
    }
    pthread_mutex_unlock(&mutex_);
    return 0;
}

int MyDataSource::getAudioParameters(int &profile,int &sampleFrequency,int &channels,int &bitrate){
    pthread_mutex_lock(&mutex_);
    if(audio_type == RtspMediaSession::TYPE_AAC){
        profile = 1;//TODO
        sampleFrequency =  audio_sample_rate;
        channels = audio_channels;
    }else if(audio_type == RtspMediaSession::TYPE_G726){
        profile = -1;//unused
        sampleFrequency =  audio_sample_rate;
        channels = audio_channels;
        bitrate = audio_bitrate;
    }else{
        printf("MyDataSource::getAudioParameters()  type not supported yet\n");
    }
    pthread_mutex_unlock(&mutex_);
    return 0;
}

int MyDataSource::getVideoFrame(unsigned char *buf, unsigned maxLen,unsigned &frameSize,int &isLive){
    RtspData *item = NULL;
    if(video_queue_->getq(&item,isLive) < 0){
        frameSize = 0;
        return 0;
    }
    memcpy(buf,item->data,item->len);
    frameSize = item->len;
    RtspData::rtsp_data_free(item);
    return 0;
}

int MyDataSource::getVideoFrame(RtspData *&item,int &isLive){
    RtspData *data = NULL;
    video_queue_->getq(&data,isLive);
    item = data;
    return 0;
}

int MyDataSource::getAudioFrame(unsigned char *buf, unsigned maxLen,unsigned &frameSize,int &isLive){
    RtspData *item = NULL;
    if(audio_queue_->getq(&item,isLive) < 0){
        frameSize = 0;
        return 0;
    }
    frameSize = (unsigned)(item->len);
    memcpy(buf,item->data,item->len);
    RtspData::rtsp_data_free(item);
    return 0;
}

int MyDataSource::getAudioFrame(RtspData *&item,int &isLive){
    RtspData *data = NULL;
    audio_queue_->getq(&data,isLive);
    item = data;
    return 0;
}

//
MyMap::MyMap():head_(NULL),tail_(NULL){}
MyMap::~MyMap(){
    /*TODO*/
}
int MyMap::get_and_inc(char *streamName){
    struct node_t *node = head_;
    while(node){
        if(!strcmp(node->streamName,streamName)){
            int ref_count = node->ref_count;
            ++node->ref_count;
            return ref_count;
        }
        node = node->next;
    }
    node = new node_t;
    node->streamName = strDup(streamName);
    node->ref_count = 1;
    node->next = NULL;

    if(tail_){
        tail_->next = node;
    }
    tail_ = node;
    if(!head_)  head_ = tail_;
    return 0;
}

int MyMap::dec_and_get(char *streamName){
    int ref_count = -1;
    struct node_t *node = head_,*prev = NULL;
    while(node){
        if(!strcmp(node->streamName,streamName)){
            --node->ref_count;
            ref_count = node->ref_count;
            if(ref_count == 0){ 
                if(prev){
                    prev->next = node->next;
                    if(node == tail_){
                        tail_ = prev;
                    }
                }else{
                        head_ = node->next;
                        if(node == tail_){
                            tail_ = head_;
                        }
                }
                delete []node->streamName;
                delete node;
            }
            break;
        }
        prev = node;
        node = node->next;
    }
    return ref_count;
}

