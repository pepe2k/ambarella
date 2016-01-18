#if NEW_RTSP_CLIENT

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "RtspClientManager.h"
#include "active_object.h"
#include "amf_rtspclient.h"
#include "misc_utils.h"

pthread_mutex_t mgr_mutex_ = PTHREAD_MUTEX_INITIALIZER;
AmfRtspClientManager *AmfRtspClientManager::m_instance = NULL;
AmfRtspClientManager *AmfRtspClientManager::instance(){
    if(m_instance == NULL){
        pthread_mutex_lock(&mgr_mutex_);
        if(m_instance == NULL){
            m_instance = new AmfRtspClientManager();
            if(m_instance == NULL){
                KILL_SELF();
            }
        }
        pthread_mutex_unlock(&mgr_mutex_);
    }
    return m_instance;
}

AmfRtspClientManager::AmfRtspClientManager(){
}

AmfRtspClientManager::~AmfRtspClientManager(){
    //printf("AmfRtspClientManager::~AmfRtspClientManager() called\n");
    pthread_mutex_lock(&mgr_mutex_);
    m_instance = NULL;
    pthread_mutex_unlock(&mgr_mutex_);
}

int AmfRtspClientManager::start_service(){
    RtspClientManager::instance()->start_service();
    return 0;
}

int AmfRtspClientManager::stop_service(){
    //printf("AmfRtspClientManager::stop_service() called\n");
    RtspClientManager::instance()->stop_service();
    delete this;
    return 0;
}

AmfRtspClient *AmfRtspClientManager::create_rtspclient(const char *rtsp_url){
    AmfRtspClient *client = new AmfRtspClient;
    if(client){
        if(client->create(rtsp_url) < 0){
            return NULL;
        }
    }
    return client;
}
void AmfRtspClientManager::destroy_rtspclient(AmfRtspClient* client){
    delete client;
}


//
class RtspClientCallback: public IRtspClientCallback
{
public:
    virtual void on_rtsp_create_session(bool success,unsigned int session_id,void *usr_data){
        AmfRtspClient *client = (AmfRtspClient*)usr_data;
        client->on_rtsp_create_session(success,session_id);
    }
     virtual void on_rtsp_start(unsigned int session_id, bool success,const std::string &sdp,void *usr_data){
        AmfRtspClient *client = (AmfRtspClient*)usr_data;
        client->on_rtsp_start(success,sdp);
    }
    virtual void on_rtsp_play(unsigned int session_id,bool success,void *usr_data){
        AmfRtspClient *client = (AmfRtspClient*)usr_data;
        client->on_rtsp_play(success);
    }
    virtual void on_rtsp_error(unsigned int session_id,int error_type,void *usr_data){
        //printf("RtspClientCallback::on_rtsp_error\n");
        AmfRtspClient *client = (AmfRtspClient*)usr_data;
        client->on_rtsp_error(error_type);
    }

    virtual void on_rtsp_avframe(unsigned int session_id,AVPacket *pkt,void *usr_data){
        //printf("on_rtsp_avframe --KEY_FRAME[%d], len = %08d,index = %d,pts = %lld\n",((pkt->flags & AV_PKT_FLAG_KEY)!= 0),pkt->size,pkt->stream_index,pkt->pts);
        //av_free_packet(pkt);
        AmfRtspClient *client = (AmfRtspClient*)usr_data;
        client->on_rtsp_avframe(pkt);
    }
};

static RtspClientCallback rtspclient_callback;

AmfRtspClient::AmfRtspClient(): av_context_(NULL),m_sessionid_(0),state_(STATE_INVALID),rtsp_error_(0){
    queue_ = new RtspAvQueue();
    if(!queue_){
        KILL_SELF();
    }
    m_future_ = new Future<rtsp_result_t>;
    if(!m_future_){
        KILL_SELF();
    }
}

AmfRtspClient::~AmfRtspClient(){
    if(state_ != STATE_INVALID){
        RtspClientManager::instance()->rtsp_tear_down(m_sessionid_);
        RtspClientManager::instance()->rtsp_del_session(m_sessionid_);
        state_ = STATE_INVALID;
    }
    delete queue_;
    delete m_future_;
    av_parse_free();
}

void AmfRtspClient::on_rtsp_create_session(bool success,unsigned int session_id){
    if(m_future_){
        //printf("AmfRtspClient::on_rtsp_create_session --- success %d,session_id %d\n",success,session_id);
        rtsp_result_t value;
        value.success = success;
        value.session_id = session_id;
        m_future_->set(value);
    }
}

void AmfRtspClient::on_rtsp_start(bool success,const std::string &sdp){
    if(m_future_){
        //printf("AmfRtspClient::on_rtsp_start --- success %d,session_id %d\n",success,m_sessionid_);
        rtsp_result_t value;
        value.success = success;
        value.sdp = sdp;
        m_future_->set(value);
    }
}

void AmfRtspClient::on_rtsp_play(bool success){
    if(m_future_){
        //printf("AmfRtspClient::on_rtsp_play --- success %d,session_id %d\n",success,m_sessionid_);
        rtsp_result_t value;
        value.success = success;
        m_future_->set(value);
    }
}

int AmfRtspClient::create(const char *rtsp_url){
    rtsp_url_ = rtsp_url;
    RtspClientManager::instance()->rtsp_create_session(rtsp_url,&rtspclient_callback,(void*)this);
    rtsp_result_t value = m_future_->get();
    m_future_->reset();

    if(value.success){
        m_sessionid_ = value.session_id;
        state_ = STATE_CREATE_OK;
        return 0;
    }
    return -1;
}

int AmfRtspClient::start(){
    RtspClientManager::instance()->rtsp_start(m_sessionid_);
    rtsp_result_t value = m_future_->get();
    m_future_->reset();

    if(value.success){
        if(av_parse_media(value.sdp) < 0){
            return -1;
        }
        state_ = STATE_START_OK;
        return 0;
    }
    return -1;
}

int AmfRtspClient::play(){
    RtspClientManager::instance()->rtsp_play(m_sessionid_);
    rtsp_result_t value = m_future_->get();
    m_future_->reset();

    if(value.success){
        state_ = STATE_PLAY_OK;
        return 0;
    }
    return -1;
}

AmfRtspClient::RtspAvQueue::RtspAvQueue(int queueSize)
    :sizeQueue(queueSize),lput(0),lget(0),nData(0)
{
    pthread_mutex_init(&q_mutex_,NULL);
    pthread_cond_init(&cond_get_,0);
    buffer = new AVPacket[queueSize];
}

AmfRtspClient::RtspAvQueue::~RtspAvQueue()
{
    flush();
    pthread_mutex_destroy(&q_mutex_);
    pthread_cond_destroy(&cond_get_);
    if(buffer){
        delete []buffer;
        buffer = NULL;
    }
}

void AmfRtspClient::RtspAvQueue::putq(AVPacket *pkt)
{
    pthread_mutex_lock(&q_mutex_);
    if(lput==lget&&nData){
        AVPacket *pkt_ = &buffer[lget++];
        av_free_packet(pkt_);
        nData--;
        if(lget==sizeQueue){
            lget=0;
        }
    }
    //buffer[lput++] = *pkt;
    memcpy(&buffer[lput++],pkt,sizeof(AVPacket));
    nData++;
    if(lput==sizeQueue){
        lput=0;
    }
    pthread_cond_signal(&cond_get_);
    pthread_mutex_unlock(&q_mutex_);
}

int AmfRtspClient::RtspAvQueue::getq(AVPacket *pkt)
{
    struct timeval now;
    struct timespec outtime;
    int rc = 0;
    pthread_mutex_lock(&q_mutex_);
    while(lget==lput&&nData==0&& rc == 0){
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + 1;  //delay 1 seconds
        outtime.tv_nsec = now.tv_usec * 1000;
        rc = pthread_cond_timedwait(&cond_get_,&q_mutex_,&outtime);
    }
    if(lget==lput&&nData==0){
        pthread_mutex_unlock(&q_mutex_);
        return AVERROR(EAGAIN);
    }
    //*pkt = buffer[lget++];
    memcpy(pkt,&buffer[lget++],sizeof(AVPacket));
    nData--;
    if(lget==sizeQueue){
        lget=0;
    }
    pthread_mutex_unlock(&q_mutex_);
    return 0;
}

int AmfRtspClient::RtspAvQueue::flush(){
    pthread_mutex_lock(&q_mutex_);
    while(nData > 0){
        av_free_packet(&buffer[lget++]);
        nData--;
        if(lget==sizeQueue){
            lget=0;
        }
    }
    lget = lput = 0;
    pthread_mutex_unlock(&q_mutex_);
    return 0;
}


extern "C"{
    #include "libavformat/avformat.h"
    #include "libavcodec/avcodec.h"
    #include "libavutil/avstring.h"
};
#include "am_ffmpeg.h"
#include "rtsp_utils.h"

int AmfRtspClient:: av_parse_media(const std::string &sdp)
{
    int err = -1;
    mw_ffmpeg_init();
    av_context_ = avformat_alloc_context();

    AVProbeData probe_data, *pd = &probe_data;
    pd->filename = rtsp_url_.c_str();
    pd->buf = NULL;
    pd->buf_size = 0;

    AVInputFormat *fmt = av_probe_input_format(pd, 0);
    if(!fmt){
        goto fail;
    }
    fmt->flags |= AVFMT_NOFILE;

    av_context_->iformat = fmt;
    av_context_->pb = NULL;
    av_context_->duration = AV_NOPTS_VALUE;
    av_context_->start_time = AV_NOPTS_VALUE;
    av_strlcpy(av_context_->filename, rtsp_url_.c_str(), sizeof(av_context_->filename));

    /* allocate private data */
    if (fmt->priv_data_size > 0) {
        av_context_->priv_data = av_mallocz(fmt->priv_data_size);
        if (!av_context_->priv_data) {
            err = AVERROR(ENOMEM);
            goto fail;
        }
    } else {
        av_context_->priv_data = NULL;
    }
    av_context_->drm_flag = 0;

    if(rtsp_sdp_parse(av_context_,sdp.c_str())){
        printf("AmfRtspClient:: av_parse_media --- failed to parse sdp,sdp = \n%s\n",sdp.c_str());
        goto fail;
    }

    /*check codec_type, now only H264 and AAC/G711 supported
    */
    int i;
    for(i = 0; i < (int)av_context_->nb_streams; i ++){
        AVStream *st = av_context_->streams[i];
        if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            if(st->codec->codec_id == CODEC_ID_H264){
                st->codec->pix_fmt = PIX_FMT_YUV420P;//TODO
                if(st->codec->extradata){
                    H264_INFO h264_info;
                    memset(&h264_info,0,sizeof(h264_info));
                    if(get_h264_info(st->codec->extradata + 5,&h264_info) < 0){
                        printf("AmfRtspClient:: av_parse_media --- failed to get H264 info\n");
                        goto fail;
                    }
                    st->codec->width = h264_info.width;
                    st->codec->height = h264_info.height;
                    st->codec->time_base.den = h264_info.time_base_num;
                    st->codec->time_base.num = h264_info.time_base_den;
                    st->r_frame_rate.den = h264_info.time_base_den;
                    st->r_frame_rate.num = h264_info.time_base_num;
                }else{
                    printf("Server SDP does not provide  sprop-parameter-sets, what should we do???? TODO\n");
                }
            }else{
                goto fail;
            }
        }
        else if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            if(st->codec->codec_id == CODEC_ID_PCM_ALAW
                || st->codec->codec_id == CODEC_ID_PCM_MULAW){
                //TODO
                st->codec->sample_fmt = AV_SAMPLE_FMT_S16;
            }else if(st->codec->codec_id == CODEC_ID_AAC){
                //TODO
                st->codec->sample_fmt = AV_SAMPLE_FMT_S16;
                st->codec->frame_size = 1024;//TODO hard code
            }else if(st->codec->codec_id == CODEC_ID_ADPCM_G726){
                //st->codec->bit_rate = ;
                //TODO
                st->codec->sample_fmt = AV_SAMPLE_FMT_S16;
            }else{
               goto fail;
            }
            if(!st->codec->bits_per_coded_sample)
                st->codec->bits_per_coded_sample= av_get_bits_per_sample(st->codec->codec_id);
            // set stream disposition based on audio service type
            switch (st->codec->audio_service_type) {
            case AV_AUDIO_SERVICE_TYPE_EFFECTS:
                st->disposition = AV_DISPOSITION_CLEAN_EFFECTS;    break;
            case AV_AUDIO_SERVICE_TYPE_VISUALLY_IMPAIRED:
                st->disposition = AV_DISPOSITION_VISUAL_IMPAIRED;  break;
            case AV_AUDIO_SERVICE_TYPE_HEARING_IMPAIRED:
                st->disposition = AV_DISPOSITION_HEARING_IMPAIRED; break;
            case AV_AUDIO_SERVICE_TYPE_COMMENTARY:
                st->disposition = AV_DISPOSITION_COMMENT;          break;
            case AV_AUDIO_SERVICE_TYPE_KARAOKE:
                st->disposition = AV_DISPOSITION_KARAOKE;          break;
            default:
                // TODO: add new type handler
                break;
            }
        }
    }
    //printf("AmfRtspClient:: av_parse_media OK\n");
    return 0;
 fail:
    av_parse_free();
    return err;
}

int AmfRtspClient::av_parse_free(){
    if (av_context_) {
        RTSPState *rt =  (RTSPState *)av_context_->priv_data;
        for (int i = 0; i < rt->nb_rtsp_streams; i++) {
            RTSPStream *rtsp_st = rt->rtsp_streams[i];
            if (rtsp_st) {
                if (rtsp_st->dynamic_handler && rtsp_st->dynamic_protocol_context)
                    rtsp_st->dynamic_handler->close(rtsp_st->dynamic_protocol_context);
                av_free(rtsp_st);
            }
        }
        av_freep(&av_context_->priv_data);
        for(int i=0;i < (int)av_context_->nb_streams;i++) {
            AVStream *st = av_context_->streams[i];
            if (st) {
                av_free(st->priv_data);
                av_free(st->codec->extradata);
                av_free(st->codec);
                av_free(st->info);
            }
            av_free(st);
        }
        av_free(av_context_);
        av_context_ = NULL;
    }
    return 0;
}

#endif //NEW_RTSP_CLIENT

