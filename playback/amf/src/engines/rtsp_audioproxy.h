#ifndef RTSP_AUDIO_PROXY_H__
#define RTSP_AUDIO_PROXY_H__

#include "rtsp_vod.h"
#include "amf_rtspclient.h"
#include "active_object.h"

class RtspAudioProxySession : public AO_Proxy {
public:
    explicit RtspAudioProxySession(const char *rtspUrl,const char *streamName){
        class MyUtil{
        public:
            static char* strDup(char const* str) {
                 if (str == NULL) return NULL;
                 size_t len = strlen(str) + 1;
                 char* copy = new char[len];

                 if (copy != NULL) {
                    memcpy(copy, str, len);
                  }
                  return copy;
            }
        };

        rtspUrl_ = MyUtil::strDup(rtspUrl);
        streamName_= MyUtil::strDup(streamName);
        pthread_mutex_init(&mutex_,NULL);
        m_future_ = new Future<int>;
        ao_start_service();
        setExitFlag(0);
        start();
    }
    ~RtspAudioProxySession(){
        setExitFlag(1);
        ao_exit_service();
        pthread_mutex_destroy(&mutex_);
        delete []rtspUrl_,rtspUrl_ = NULL;
        delete []streamName_,streamName_ = NULL;
        delete m_future_,m_future_ = NULL;
    }
private:
    void start(){
        class StartImpl:public Method_Request{
        public:
            StartImpl(RtspAudioProxySession *manager):manager_(manager){}
            virtual bool guard(){return true;}
            virtual int call(){
                manager_->start(manager_->rtspUrl_, manager_->streamName_);
                return 0;
            }
        private:
            RtspAudioProxySession *manager_;
        };
        ao_send_request(new StartImpl(this));
        int value = m_future_->get();
        m_future_->reset();
        value = value;//disable warinig
    }
    void start(const char *rtspUrl,const char *streamName){
        AmfRtspClient *rtspclient  = AmfRtspClientManager::instance()->create_rtspclient(rtspUrl);
        rtspclient->start();

        //rtspMediaSession create/config/start
        RtspMediaSession *ms = new RtspMediaSession;

        AVFormatContext *av_context_ = rtspclient->get_avformat();
        av_dump_format(av_context_, 0,"rtsp", 0);

        int audio_index = -1;
        enum CodecID audio_codec_id = CODEC_ID_NONE;
        int i;
        for(i = 0; i < (int)av_context_->nb_streams; i ++){
            AVStream *st = av_context_->streams[i];
            if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO){
                audio_index = i;
                audio_codec_id = st->codec->codec_id;
                switch(st->codec->codec_id){
                case CODEC_ID_AAC:ms->addAudioAACStream(st->codec->sample_rate,st->codec->channels);break;
                case CODEC_ID_PCM_MULAW:ms->addAudioG711Stream(0);break;
                case CODEC_ID_PCM_ALAW:ms->addAudioG711Stream(1);break;
                default:audio_index = -1,audio_codec_id = CODEC_ID_NONE;break;
                }
            }
        }
        ms->setDataSourceType(RtspMediaSession::DS_TYPE_LIVE);
        ms->setDestination((char*)streamName);
        rtspclient->play();

        int value = 0;
        m_future_->set(value);

        //printf("AudioProxy[%s,%s] -- audio_index = %d\n",rtspUrl,streamName,audio_index);
        int retry_count = 0;
        while(1){
            AVPacket packet, *pkt =&packet;
            int result = rtspclient->read_avframe(pkt);
            if( result == 0){
                retry_count = 0;
                if(pkt->stream_index == audio_index){
                    switch(audio_codec_id){
                    case CODEC_ID_AAC:ms->sendData(RtspMediaSession::TYPE_AAC,pkt->data,pkt->size,pkt->pts);break;
                    case CODEC_ID_PCM_MULAW:ms->sendData(RtspMediaSession::TYPE_G711_MU,pkt->data,pkt->size,pkt->pts);break;
                    case CODEC_ID_PCM_ALAW:ms->sendData(RtspMediaSession::TYPE_G711_A,pkt->data,pkt->size,pkt->pts);break;
                    default:break;
                    }
                }
                av_free_packet(pkt);
            }else if(result == AVERROR(EAGAIN)){
                ++retry_count;
                if(retry_count >= 3){
                    printf("rtsp_client->read_avframe error[%d]",result);
                    break;
                }
            }else{
                printf("rtsp_client->read_avframe error[%d]",result);
                break;//error
            }
            if(getExitFlag()){
                printf("rtsp_client->exit_flag set\n");
                break;
            }
        }
        //printf("AudioProxy[%s,%s] -- audio_index = %d, exit\n",rtspUrl,streamName,audio_index);
        AmfRtspClientManager::instance()->destroy_rtspclient(rtspclient);
        delete ms;
    }
    void setExitFlag(int flag) {
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
private:
    char *rtspUrl_;
    char *streamName_;
    pthread_mutex_t  mutex_;
    int exit_flag_;
    Future<int> *m_future_;
};

#endif //RTSP_AUDIO_PROXY_H__

