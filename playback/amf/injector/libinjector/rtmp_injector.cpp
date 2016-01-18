#include <unistd.h>
#include "rtmp_buffer.h"
#include "rtmp_queue.h"
#include "rtmp_get_h264_info.h"
#include "rtmp_injector.h"

using namespace AMBA_RTMP;

static const int max_nals_length = 320* 1024;
RtmpInjector::RtmpInjector(int queue_size)
    :video_enc(NULL),audio_enc(NULL),rtmp_(NULL){
    queue_ = new RtmpDataQueue(queue_size);
    m_tmpBuffer = new unsigned char[max_nals_length];
    pthread_mutex_init(&mutex_,NULL);
    setExitFlag(0);
    enableReconnect(1);
    m_future_ = new Future<int>;
    rtmpUrl_ = NULL;
    g711_data_size = 0;
    ao_start_service();
}

RtmpInjector::~RtmpInjector(){
    enableReconnect(0);
    setExitFlag(1);
    ao_exit_service();
    if(rtmp_){
        delete rtmp_;
        rtmp_ = NULL;
    }

    pthread_mutex_destroy(&mutex_);
    if(video_enc){
        if(video_enc->extra_data) delete []video_enc->extra_data;
        delete video_enc;
    }
    if(audio_enc){
        if(audio_enc->extra_data) delete []audio_enc->extra_data;
        delete audio_enc;
    }
    delete m_future_;
    delete queue_;
    delete []m_tmpBuffer;
    if(rtmpUrl_){
        delete []rtmpUrl_;
    }
}

int RtmpInjector::addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate){
    class AddVideoStream:public Method_Request{
    public:
        AddVideoStream(RtmpInjector *proxy,unsigned char *extra_data, int extra_data_len,int bitrate)
            :proxy_(proxy),bitrate_(bitrate){
            extra_data_ = new unsigned char[(extra_data_len + 15)/16 * 16];
            memcpy(extra_data_,extra_data,extra_data_len);
            extra_data_len_ = extra_data_len;
        }
        ~AddVideoStream(){delete []extra_data_;}
        bool guard() {return true;}
        int call(){
            if(proxy_->video_enc){
                 delete []proxy_->video_enc->extra_data;
                 delete proxy_->video_enc;
            }
            proxy_->video_enc = new RtmpInjector::video_param;
            if(!proxy_->video_enc){
                return -1;
            }
            proxy_->video_enc->bitrate = bitrate_;
            proxy_->video_enc->extra_data = new unsigned char[(extra_data_len_ + 15)/16 * 16];
            if(!proxy_->video_enc->extra_data){
                 delete  proxy_->video_enc;
                 proxy_->video_enc = NULL;
                 return -1;
            }
            memcpy(proxy_->video_enc->extra_data,extra_data_,extra_data_len_);
            proxy_->video_enc->extra_data_len = extra_data_len_;
            H264_Helper::H264_INFO info;
            H264_Helper::get_h264_info(proxy_->video_enc->extra_data + 5,&info);
            proxy_->video_enc->width = info.width;
            proxy_->video_enc->height = info.height;
            proxy_->video_enc->framerate = (double)info.time_base_num * (double)1.0/(double)info.time_base_den;
            printf("AddVideoStream -- [%dx%d], fps [%lf]\n",info.width,info.height,proxy_->video_enc->framerate);
            return 0;
        }
    private:
        RtmpInjector *proxy_;
        int width_;
        int height_;
        int framerate_;
        int bitrate_;
        unsigned char *extra_data_;
        int extra_data_len_;
    };
    ao_send_request(new AddVideoStream(this,extra_data,extra_data_len,bitrate));
    return 0;
}

int RtmpInjector::addAudioAACStream(int sample_rate,int channels,int bitrate){
    class AddAudioAACStream:public Method_Request{
    public:
        AddAudioAACStream(RtmpInjector *proxy,int sample_rate,int channels,int bitrate)
            :proxy_(proxy),sample_rate_(sample_rate),channels_(channels),bitrate_(bitrate){
        }
        bool guard() {return true;}
        int call(){
            if(proxy_->audio_enc){
                delete proxy_->audio_enc;
            }
            proxy_->audio_enc = new RtmpInjector::audio_param;
            if(!proxy_->audio_enc){
                return -1;
            }
            proxy_->audio_enc->codec_id = RtmpClient::TYPE_AAC;
            proxy_->audio_enc->bitrate = bitrate_;
            proxy_->audio_enc->sample_rate = sample_rate_;
            proxy_->audio_enc->sample_size = 16;
            proxy_->audio_enc->channels = channels_;

           static const int ff_mpeg4audio_sample_rates[16] = {
               96000, 88200, 64000, 48000, 44100, 32000,
               24000, 22050, 16000, 12000, 11025, 8000, 7350,
               0,0,0
           };

            int index;
            for (index = 0; index < 16; index++)
                if (sample_rate_ == ff_mpeg4audio_sample_rates[index])
                    break;
            if (index == 16) {
                printf("RtmpInjector::addAudioAACStream() -- Unsupported sample rate %d\n",sample_rate_);
                return -1;
            }
            proxy_->audio_enc->extra_data_len = 2;
            proxy_->audio_enc->extra_data = new unsigned char[16];
            if (!proxy_->audio_enc->extra_data){
                printf("RtmpInjector::addAudioAACStream() -- Failed to alloc extra_data\n");
                return -1;
            }
            proxy_->audio_enc->extra_data[0] = 0x02 << 3 | index >> 1;
            proxy_->audio_enc->extra_data[1] = (index & 0x01) << 7 | channels_ << 3;
            printf("AddAudioStream -- sample_rate[%d], channels[%d]\n",sample_rate_,channels_);
            return 0;
        }
    private:
        RtmpInjector *proxy_;
        int sample_rate_;
        int channels_;
        int bitrate_;
    };
    ao_send_request(new AddAudioAACStream(this,sample_rate,channels,bitrate));
    return 0;
}

int RtmpInjector::addAudioG711Stream(int isALaw){
    class AddAudioG711Stream:public Method_Request{
    public:
        AddAudioG711Stream(RtmpInjector *proxy,int isALaw)
            :proxy_(proxy),isALaw_(isALaw){
        }
        bool guard() {return true;}
        int call(){
            if(proxy_->audio_enc){
                delete proxy_->audio_enc;
            }
            proxy_->audio_enc = new RtmpInjector::audio_param;
            if(!proxy_->audio_enc){
                return -1;
            }
            if(isALaw_){
                proxy_->audio_enc->codec_id = RtmpClient::TYPE_G711_A;
            }else{
                proxy_->audio_enc->codec_id = RtmpClient::TYPE_G711_MU; 
            }
            proxy_->audio_enc->bitrate = 64000;
            proxy_->audio_enc->sample_rate = 8000;
            proxy_->audio_enc->sample_size = 16;
            proxy_->audio_enc->channels = 1;
            proxy_->audio_enc->extra_data = NULL;
            proxy_->audio_enc->extra_data_len = 0;
            printf("AddAudioStream -G711[isALaw %d]- sample_rate[8000], channels[1]\n",isALaw_);
            return 0;
        }
    private:
        RtmpInjector *proxy_;
        int isALaw_;
    };
    ao_send_request(new AddAudioG711Stream(this,isALaw));
    return 0;
}

int RtmpInjector::setDestination(char *const rtmpUrl){
    class SetDestination:public Method_Request{
    public:
        SetDestination(RtmpInjector *proxy,char *rtmpUrl)
             :proxy_(proxy){
             rtmpUrl_ = RtmpBuffer::strDup(rtmpUrl);
        }
         ~SetDestination(){
            if(rtmpUrl_) delete []rtmpUrl_;
        }
        bool guard() {return true;}
        int call(){
            int value = 0;//-1;
            do{
                proxy_->rtmpUrl_ = RtmpBuffer::strDup(rtmpUrl_);
                proxy_->rtmp_ = new RtmpBuffer(proxy_);
                if(!proxy_->rtmp_){
                    break;
                }
                printf("setDestination URL %s\n",rtmpUrl_);
                if(proxy_->rtmp_->init(rtmpUrl_)){
                    break;
                }
                if(proxy_->write_header()){
                    break;
                }
                value = 0;
                proxy_->m_future_->set(value);
                proxy_->queue_->flush();

                printf("setDestination URL %s OK\n",rtmpUrl_);
                //event loop
                while(!proxy_->getExitFlag()){
                    if(proxy_->queue_){
                        RtmpData *pkt = NULL;
                        if(proxy_->queue_->getq(&pkt)){
                            continue;
                        }
                        if(proxy_->write_packet(pkt)){
                            printf("RtmpInjector::write_packet() failed\n");
                            RtmpData::rtmp_data_free(pkt);
                            break;
                        }
                        RtmpData::rtmp_data_free(pkt);
                    }else{
                        usleep(1000);
                    }
                }
                printf("RtmpInjector::setDestination::call exit\n");
                return 0;
            }while(0);
            proxy_->m_future_->set(value);
            proxy_->onPeerClose(); //auto reconecct
            return 0;
        }
    private:
        RtmpInjector *proxy_;
        char *rtmpUrl_;
    };
    ao_send_request(new SetDestination(this,rtmpUrl));
    int value = m_future_->get();
    m_future_->reset();
    printf("RtmpInjector::setDestination --value = %d\n",value);
    return value;
}


void RtmpInjector::onPeerClose(){
    printf("RtmpInjector::onPeerClose()\n");
    class HandleReconnect:public Method_Request{
    public:
        HandleReconnect(RtmpInjector *proxy):proxy_(proxy){}
        virtual bool guard() {return true;}
        int call(){
            do{
                if(proxy_->rtmp_){
                    delete proxy_->rtmp_;
                    proxy_->rtmp_ = NULL;
                }
                proxy_->rtmp_ = new RtmpBuffer(proxy_);
                if(!proxy_->rtmp_){
                    break;
                }
                printf("HandleReconnect::setDestination URL %s\n",proxy_->rtmpUrl_);
                if(proxy_->rtmp_->init(proxy_->rtmpUrl_)){
                    break;
                }
                if(proxy_->write_header()){
                    break;
                }
                proxy_->queue_->flush();
                proxy_->queue_->notify_exit(0);
                printf("HandleReconnect::setDestination URL %s OK\n",proxy_->rtmpUrl_);
                //event loop
                while(!proxy_->getExitFlag()){
                    if(proxy_->queue_){
                        RtmpData *pkt = NULL;
                        if(proxy_->queue_->getq(&pkt)){
                            continue;
                        }
                        if(proxy_->write_packet(pkt)){
                            printf("RtmpInjector::HandleReconnect::write_packet() failed\n");
                            RtmpData::rtmp_data_free(pkt);
                            break;
                        }
                        RtmpData::rtmp_data_free(pkt);
                    }else{
                        usleep(1000);
                    }
                }
                printf("RtmpInjector::HandleReconnect::call exit\n");
                return 0;
            }while(0);

            usleep(1000*1000);
            proxy_->onPeerClose();
            return 0;
        }
    private:
        RtmpInjector *proxy_;
    };

    if(isReconnectEnabled()){
        queue_->notify_exit();
        ao_send_request(new HandleReconnect(this));
    }
}

int RtmpInjector::sendData(RtmpClient::DataType type,unsigned char *data,int len,unsigned int timestamp){
    if(type == RtmpClient::TYPE_AAC){
        if(len <= 7) {
            printf("RtmpInjector::sendData, AAC data length is invalid, len = %d\n",len);
            return 0;
        }
        //printf("RtmpInjector::sendData() -- AAC --- data = %p,len = %d\n",data,len);
        //TODO, remove AU-Header 7 bytes
        RtmpData *pkt = RtmpData::rtmp_data_alloc(type,data + 7,len -7);
        if(pkt){
            queue_->putq(pkt);
            //printf("RtmpInjector::sendData() -- AAC --- putq\n");
        }else{
            printf("RtmpData::rtmp_data_alloc() failed -- AAC\n");
        }
    }else if(type == RtmpClient::TYPE_H264){
        class GetNalFromBuffer{
        public:
            GetNalFromBuffer(RtmpInjector *proxy):proxy_(proxy){}
            RtmpInjector *proxy_;
            int total_len;
            int key_frame;
            int parse(unsigned char *data,int len){
                if(len > max_nals_length){
                    return -1;
                }
                total_len = 0;
                key_frame = 0;

                int pos = 0,prepos;
                int nal_len,nal_type;
                unsigned char *nal;
                while(pos < len - 4){
                    if(data[pos] == 0x00)
                        if(data[pos + 1] == 0x00)
                            if(data[pos + 2] == 0x00)
                                if(data[pos + 3] == 0x01)
                                    break;
                   ++pos;
                }
                prepos = pos;
                //printf("RtmpInjector::sendData() --H264 ---parse -- pos = %d,len = %d\n",pos,len);     

                while(1){
                    pos = prepos + 4;
                    while(pos < len - 4){
                        if(data[pos] == 0x00)
                            if(data[pos + 1] == 0x00)
                                if(data[pos + 2] == 0x00)
                                    if(data[pos + 3] == 0x01)
                                        break;
                       ++pos;
                    }

                    if(pos >= len -4){
                        //printf("RtmpInjector::sendData() --H264 ---parse -- pos = %d, END\n",pos);
                        pos = len;                      
                        nal_len = pos - prepos - 4;
                        nal = &data[prepos + 4];
                        nal_type = nal[0] & 0x1F;
                        if((!key_frame) && (nal_type == 5)){ //IDR
                            key_frame = 1;
                        }
                        setNalLength(proxy_->m_tmpBuffer + total_len, nal_len);
                        memcpy(proxy_->m_tmpBuffer + total_len + 4,nal,nal_len);
                        total_len += 4 + nal_len;
                        break;
                    }

                    //printf("RtmpInjector::sendData() --H264 ---parse -- pos = %d\n",pos);
                    nal_len = pos - prepos - 4;
                    nal = &data[prepos + 4];
                    nal_type = nal[0] &  0x1F;
                    if((!key_frame) && (nal_type == 5)){//IDR
                         key_frame = 1;
                    }
                    setNalLength(proxy_->m_tmpBuffer + total_len, nal_len);
                    memcpy(proxy_->m_tmpBuffer + total_len + 4,nal,nal_len);
                    total_len += 4 + nal_len;

                    //prepare for next 
                    prepos = pos;
                }
                if(!total_len){
                    return -1;
                }
                return 0;
            }
        private:
            void setNalLength(unsigned char *buf,unsigned int length){
                *(buf + 0) = length >> 24;
                *(buf + 1) = length >> 16;
                *(buf + 2) = length >> 8;
                *(buf + 3) = length >> 0;
            }
        };

        if(len <= 4){
            printf("RtmpInjector::sendData, H264 data length is invalid, len = %d\n",len);
            return 0;
        }
        //printf("RtmpInjector::sendData() --H264 ---data %p,len %d\n",data,len);
        GetNalFromBuffer buffer(this);
        if(!buffer.parse(data,len)){
            //printf("RtmpInjector::sendData() --H264 ---parsed data %p,len %d\n",m_tmpBuffer,buffer.total_len);
            RtmpData *pkt = RtmpData::rtmp_data_alloc(type,m_tmpBuffer,buffer.total_len,buffer.key_frame);
            if(pkt){
                queue_->putq(pkt);
                //printf("RtmpInjector::sendData() --H264 ---putq\n");
            }else{
                printf("RtmpData::rtmp_data_alloc() failed --H264\n");
            }
        }else{
            printf("Unknown H264 ES format\n");
        }
    }else if(type == RtmpClient::TYPE_G711_A || type == RtmpClient::TYPE_G711_MU){
        RtmpData *pkt = RtmpData::rtmp_data_alloc(type,data,len);
        if(pkt){
            queue_->putq(pkt);
        }else{
            printf("RtmpData::rtmp_data_alloc() failed --G711\n");
        }
    }
    return 0;
}

enum{
    FLV_TAG_TYPE_AUDIO = 0x08,
    FLV_TAG_TYPE_VIDEO = 0x09,
    FLV_TAG_TYPE_META  = 0x12,
};
enum{
    FLV_HEADER_FLAG_HASVIDEO = 1,
    FLV_HEADER_FLAG_HASAUDIO = 4,
};
enum{
    AMF_DATA_TYPE_NUMBER      = 0x00,
    AMF_DATA_TYPE_BOOL        = 0x01,
    AMF_DATA_TYPE_STRING      = 0x02,
    AMF_DATA_TYPE_OBJECT      = 0x03,
    AMF_DATA_TYPE_NULL        = 0x05,
    AMF_DATA_TYPE_UNDEFINED   = 0x06,
    AMF_DATA_TYPE_REFERENCE   = 0x07,
    AMF_DATA_TYPE_MIXEDARRAY  = 0x08,
    AMF_DATA_TYPE_OBJECT_END  = 0x09,
    AMF_DATA_TYPE_ARRAY       = 0x0a,
    AMF_DATA_TYPE_DATE        = 0x0b,
    AMF_DATA_TYPE_LONG_STRING = 0x0c,
    AMF_DATA_TYPE_UNSUPPORTED = 0x0d,
};


/* offsets for packed values */
#define FLV_AUDIO_SAMPLESSIZE_OFFSET 1
#define FLV_AUDIO_SAMPLERATE_OFFSET  2
#define FLV_AUDIO_CODECID_OFFSET     4
#define FLV_VIDEO_FRAMETYPE_OFFSET   4

enum {
    FLV_MONO   = 0,
    FLV_STEREO = 1,
};

enum {
    FLV_SAMPLESSIZE_8BIT  = 0,
    FLV_SAMPLESSIZE_16BIT = 1 << FLV_AUDIO_SAMPLESSIZE_OFFSET,
};

enum {
    FLV_SAMPLERATE_SPECIAL = 0, /**< signifies 5512Hz and 8000Hz in the case of NELLYMOSER */
    FLV_SAMPLERATE_11025HZ = 1 << FLV_AUDIO_SAMPLERATE_OFFSET,
    FLV_SAMPLERATE_22050HZ = 2 << FLV_AUDIO_SAMPLERATE_OFFSET,
    FLV_SAMPLERATE_44100HZ = 3 << FLV_AUDIO_SAMPLERATE_OFFSET,
};

enum {
    FLV_CODECID_PCM                  = 0,
    FLV_CODECID_ADPCM                = 1 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_MP3                  = 2 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_PCM_LE               = 3 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_NELLYMOSER_16KHZ_MONO = 4 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_NELLYMOSER_8KHZ_MONO = 5 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_NELLYMOSER           = 6 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_G711_ALAW  = 7 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_G711_MULAW  = 8 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_AAC                  = 10<< FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_SPEEX                = 11<< FLV_AUDIO_CODECID_OFFSET,
};

enum {
    FLV_CODECID_H263    = 2,
    FLV_CODECID_SCREEN  = 3,
    FLV_CODECID_VP6     = 4,
    FLV_CODECID_VP6A    = 5,
    FLV_CODECID_SCREEN2 = 6,
    FLV_CODECID_H264    = 7,
};

enum {
    FLV_FRAME_KEY        = 1 << FLV_VIDEO_FRAMETYPE_OFFSET,
    FLV_FRAME_INTER      = 2 << FLV_VIDEO_FRAMETYPE_OFFSET,
    FLV_FRAME_DISP_INTER = 3 << FLV_VIDEO_FRAMETYPE_OFFSET,
};

#define AMF_END_OF_OBJECT         0x09

int RtmpInjector::write_header(){
    int audio_exist = !!audio_enc;
    int video_exist = !!video_enc;

    rtmp_->put_tag("FLV"); // Signature
    rtmp_->put_byte(1);    // Version
    int flag = audio_exist * FLV_HEADER_FLAG_HASAUDIO + video_exist * FLV_HEADER_FLAG_HASVIDEO;
    printf("video&audio flag = %d\n",flag);
    rtmp_->put_byte(flag);  //Video/Audio
    rtmp_->put_be32(9);    // DataOffset
    rtmp_->put_be32(0);    // PreviousTagSize0

    /* write meta_tag */
    int metadata_size_pos;
    rtmp_->put_byte(FLV_TAG_TYPE_META); // tag type META
    metadata_size_pos= rtmp_->tell();
    rtmp_->put_be24(0); // size of data part (sum of all parts below)
    rtmp_->put_be24(0); // time stamp
    rtmp_->put_be32(0); // reserved

    /* now data of data_size size */
    /* first event name as a string */
    rtmp_->put_byte(AMF_DATA_TYPE_STRING);
    rtmp_->put_amf_string("onMetaData"); // 12 bytes

    /* mixed array (hash) with size and string/type/data tuples */
    rtmp_->put_byte(AMF_DATA_TYPE_MIXEDARRAY);
    rtmp_->put_be32(5*video_exist + 5*audio_exist + 2); // +2 for duration and file size

    rtmp_->put_amf_string("duration");
    rtmp_->put_amf_double(0/*s->duration / AV_TIME_BASE*/); // fill in the guessed duration, it'll be corrected later if incorrect

    if(video_exist){
        //printf("VideoInfo -- %dx%d, bitrate %d,framerate %d\n",video_enc->width,video_enc->height,video_enc->bitrate,(int)video_enc->framerate * 100);
        rtmp_->put_amf_string("width");
        rtmp_->put_amf_double(video_enc->width);

        rtmp_->put_amf_string("height");
        rtmp_->put_amf_double(video_enc->height);

        rtmp_->put_amf_string("videodatarate");
        rtmp_->put_amf_double(video_enc->bitrate/1024.0);

        rtmp_->put_amf_string("framerate");
        rtmp_->put_amf_double(0/*video_enc->framerate*/);//TODO

        rtmp_->put_amf_string("videocodecid");
        rtmp_->put_amf_double(FLV_CODECID_H264);
    }

    if(audio_exist){
        rtmp_->put_amf_string("audiodatarate");
        rtmp_->put_amf_double(audio_enc->bitrate /1024.0);

        rtmp_->put_amf_string("audiosamplerate");
        rtmp_->put_amf_double(audio_enc->sample_rate);

        rtmp_->put_amf_string("audiosamplesize");
        rtmp_->put_amf_double(audio_enc->sample_size);

        rtmp_->put_amf_string("stereo");
        rtmp_->put_byte(AMF_DATA_TYPE_BOOL);
        rtmp_->put_byte(!!(audio_enc->channels == 2));

        rtmp_->put_amf_string("audiocodecid");
        unsigned int codec_id = 0xffffffff;
        switch(audio_enc->codec_id){
        case RtmpClient::TYPE_AAC: codec_id = 10;break;
        case RtmpClient::TYPE_G711_A: codec_id = 7; break;
        case RtmpClient::TYPE_G711_MU: codec_id = 8;break;
        default:break;
        }
        if(codec_id != 0xffffffff){
            rtmp_->put_amf_double(codec_id);
        }
    }
    rtmp_->put_amf_string("filesize");
    rtmp_->put_amf_double(0); // delayed write

    rtmp_->put_amf_string("");
    rtmp_->put_byte(AMF_END_OF_OBJECT);

    /* write total size of tag */
    int data_size= rtmp_->tell() - metadata_size_pos - 10;
    rtmp_->update_amf_be24(data_size,metadata_size_pos);
    rtmp_->put_be32(data_size + 11);

    if(video_exist){
        write_h264_header();
    }
    if(audio_exist){
        switch(audio_enc->codec_id){
        case RtmpClient::TYPE_AAC:  write_aac_header();break;
        case RtmpClient::TYPE_G711_A:
        case RtmpClient::TYPE_G711_MU: write_g711_header();break;
        default:break;
        }
    }
    if(rtmp_->flush_data_force() < 0){
        printf("RtmpInjector::write_header FAILED\n");
        return -1;
    }
    printf("RtmpInjector::write_header OK\n");
    return 0;
}

int RtmpInjector::write_h264_header(){
    #   define AV_RB32(x)                           \
         ((((const unsigned char*)(x))[0] << 24) |         \
         (((const unsigned char*)(x))[1] << 16) |         \
         (((const unsigned char*)(x))[2] <<  8) |         \
         ((const unsigned char*)(x))[3])

    unsigned int timestamp = 0;
    rtmp_->put_byte(FLV_TAG_TYPE_VIDEO);
    rtmp_->put_be24(0); //size patched later
    rtmp_->put_be24(timestamp); // ts
    rtmp_->put_byte((timestamp >> 24)&0x7f); // ts ext, timestamp is signed-32 bit
    rtmp_->put_be24(0); // streamid
    unsigned int pos = rtmp_->tell();

    rtmp_->put_byte(FLV_CODECID_H264| FLV_FRAME_KEY); // flags
    rtmp_->put_byte(0); // AVC sequence header
    rtmp_->put_be24(0); // composition time, 0 for AVC

    //parse extra_data to get sps/pps,TODO
    unsigned char *sps,*pps;
    int sps_size,pps_size;
    int startcode_len = 4;

    //printf("RtmpInjector::write_h264_sps_pps -- extradata_len = %d\n",video_enc->extra_data_len);
    sps = video_enc->extra_data;
    sps_size = 0;
    for(int i = 4; i < video_enc->extra_data_len - 4; i++){
        if((AV_RB32(&sps[i]) == 0x00000001) /*|| (AV_RB24(&sps[i]) == 0x000001)*/){
            sps_size = i;
            //startcode_len = (AV_RB32(&sps[i]) == 0x00000001) ? 4:3;
            break;
        }
    }
    if(!sps_size){
        return -1;
    }
    pps = sps + sps_size;
    pps_size = video_enc->extra_data_len - sps_size;

    //printf("Decoded sps pps length[%d][%d]\n",sps_size,pps_size);
    // fill SPS
    sps += startcode_len;
    rtmp_->put_byte(1);      // version
    rtmp_->put_byte(sps[1]); // profile
    rtmp_->put_byte(sps[2]); // profile
    rtmp_->put_byte(sps[3]); // level

    //lengthSizeMinusOne = ff -- FLV NALU-length bytes = (lengthSizeMinusOne & 3£©+1 = 4
    rtmp_->put_byte(0xff);
    //numOfSequenceParameterSets = E1 -- (nummOfSequenceParameterSets & 0x1F) == 1, one sps exists
    rtmp_->put_byte(0xe1);
    rtmp_->put_be16(sps_size - startcode_len);
    rtmp_->append_data(sps,sps_size - startcode_len);

    // fill PPS
    rtmp_->put_byte(1); // number of pps
    rtmp_->put_be16(pps_size - startcode_len);
    rtmp_->append_data(pps + startcode_len, pps_size - startcode_len );

    unsigned int data_size = rtmp_->tell() - pos;
    rtmp_->update_amf_be24(data_size,pos - 10);
    rtmp_->put_be32(data_size + 11); // Last tag size
    return 0;
}

int RtmpInjector::write_aac_header(){
    unsigned int timestamp = 0;
    rtmp_->put_byte(FLV_TAG_TYPE_AUDIO);
    rtmp_->put_be24(0); //size patched later
    rtmp_->put_be24(timestamp); // ts
    rtmp_->put_byte((timestamp >> 24)&0x7f); // ts ext, timestamp is signed-32 bit
    rtmp_->put_be24(0); // streamid

    unsigned int pos = rtmp_->tell();
    //specs force these parameters
    int flags = FLV_CODECID_AAC | FLV_SAMPLERATE_44100HZ | FLV_SAMPLESSIZE_16BIT | FLV_STEREO;
    rtmp_->put_byte(flags);
    rtmp_->put_byte(0);//AAC sequence header
    rtmp_->append_data(audio_enc->extra_data, audio_enc->extra_data_len);

    unsigned int data_size = rtmp_->tell() - pos;
    rtmp_->update_amf_be24(data_size,pos - 10);
    rtmp_->put_be32(data_size + 11); // Last tag size
    return 0;
}

int RtmpInjector::write_g711_header(){
    return 0;
}

int RtmpInjector::write_packet(RtmpData *pkt){
    if(pkt->type == RtmpClient::TYPE_H264){
        return write_h264_packet(pkt);
    }
    if(pkt->type == RtmpClient::TYPE_AAC){
        return write_aac_packet(pkt);
    }
    if(pkt->type == RtmpClient::TYPE_G711_A || pkt->type == RtmpClient::TYPE_G711_MU){
        return write_g711_packet(pkt);
    }
    return 0;
}

int RtmpInjector::write_h264_packet(RtmpData *pkt){
    rtmp_->put_byte(FLV_TAG_TYPE_VIDEO);
    rtmp_->put_be24(pkt->len + 5);
    rtmp_->put_be24(pkt->timestamp); //timestamp is dts
    rtmp_->put_byte((pkt->timestamp >> 24)&0x7f);// timestamps are 32bits _signed_
    rtmp_->put_be24(0);//streamId, always 0

    int tag_flags = FLV_CODECID_H264 | (pkt->keyframe ? FLV_FRAME_KEY: FLV_FRAME_INTER);
    rtmp_->put_byte(tag_flags);
    rtmp_->put_byte(1);// AVC NALU
    rtmp_->put_be24(0);// composition time offset, TODO, pts - dts

    //lengthSizeMinusOne = ff -- FLV NALU-length bytes = (lengthSizeMinusOne & 3£©+1 = 4
    //   so NALU-length occupies four bytes.
    /*NALU-length(4bytes) + NALU-data, constructured in data_source
    */
    //rtmp_->put_be32(pkt->len);
    rtmp_->append_data(pkt->data, pkt->len);

    //send NALU to server
    rtmp_->put_be32(11 + pkt->len + 5);
    if(rtmp_->flush_data()){
        return -1;
    }
    return 0;
}

int RtmpInjector::write_aac_packet(RtmpData *pkt){
    rtmp_->put_byte(FLV_TAG_TYPE_AUDIO);
    rtmp_->put_be24(pkt->len + 2);
    rtmp_->put_be24(pkt->timestamp);
    rtmp_->put_byte((pkt->timestamp >> 24)&0x7f);
    rtmp_->put_be24(0);//streamId, always 0

    //specs force these parameters
    int flags = FLV_CODECID_AAC | FLV_SAMPLERATE_44100HZ | FLV_SAMPLESSIZE_16BIT | FLV_STEREO;
    rtmp_->put_byte(flags);
    rtmp_->put_byte(1);// AAC raw
    rtmp_->append_data(pkt->data, pkt->len);

    rtmp_->put_be32(11 + pkt->len  + 2);

    if(rtmp_->flush_data()){
        return -1;
    }
    return 0;
}

int RtmpInjector::write_g711_packet(RtmpData *pkt){
    static const int g711_frame_size = 80; //10 ms
    unsigned int timestamp = pkt->timestamp;
    int remained_len = pkt->len;
    unsigned char *data = pkt->data;
    if(g711_data_size){
        memcpy(&g711_buffer[g711_data_size],data,g711_frame_size -g711_data_size);
        write_g711_frame(g711_buffer,g711_frame_size,timestamp);
        remained_len -= g711_frame_size -g711_data_size;
        data += g711_frame_size -g711_data_size;
        g711_data_size = 0;
    }
    while(remained_len >= g711_frame_size){
        write_g711_frame(data,g711_frame_size,timestamp);
        remained_len -= g711_frame_size;
        data += g711_frame_size;
    }
    if(remained_len){
        memcpy(g711_buffer,data,remained_len);
        g711_data_size = remained_len;
    }

    if(rtmp_->flush_data()){
        return -1;
    }
    return 0;
}

int RtmpInjector::write_g711_frame(unsigned char *buf,int len,unsigned int timestamp){
    rtmp_->put_byte(FLV_TAG_TYPE_AUDIO);
    rtmp_->put_be24(len + 1);
    rtmp_->put_be24(timestamp);
    rtmp_->put_byte((timestamp >> 24)&0x7f);
    rtmp_->put_be24(0);//streamId, always 0

    int flags = FLV_SAMPLERATE_SPECIAL | FLV_SAMPLESSIZE_16BIT | FLV_MONO;
    if(audio_enc->codec_id == RtmpClient::TYPE_G711_A){
        flags |= FLV_CODECID_G711_ALAW;
    }else{
        flags |= FLV_CODECID_G711_MULAW;
    }
    rtmp_->put_byte(flags);
    rtmp_->append_data(buf, len);

    rtmp_->put_be32(11 + len  + 1);
    return 0;
}

