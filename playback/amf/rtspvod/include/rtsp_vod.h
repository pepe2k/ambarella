#ifndef __RTSP_VOD_H_
#define __RTSP_VOD_H_

class RtspMediaSessionImpl;
class RtspMediaSession{
public:
    RtspMediaSession();
    ~ RtspMediaSession();
public:
    static int start_rtsp_server(int rtspPort = 554);
    static void stop_rtsp_server();
public:
    int addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate = 0);
    int addAudioAACStream(int sample_rate,int channels,int bitrate = 0);
    int addAudioG711Stream(int isALaw);
    int addAudioG726Stream(int bitrate);/*16000/24000/32000/40000 bps*/
    /*setDestination() -- init dataSource and register streamName
    *     must be called after  addVideoH264Stream()/addAudioAACStream()/etc.
    */
    int setDestination(char *const rtspStreamName);

    /* DataSourceType will be DS_TYPE_LIVE, if setDataSourceType() not called.
    *       if DS_TYPE_FILE set, sendData() will not return immediately, data-fill speed will be controlled.
    *   Called before sendData() and after setDestination(), if needed.
    */
    enum DataSourceType{DS_TYPE_LIVE,DS_TYPE_FILE};
    int setDataSourceType(DataSourceType type);

    enum DataType {TYPE_INVALID,TYPE_H264 = 100, TYPE_AAC = 200,TYPE_G711_A,TYPE_G711_MU, TYPE_G726};
    int sendData(DataType type,unsigned char *data,int len,long long  timestamp  /*=-1,  means pts not filled*/);

    /*For ts-over-udp/rtp,
    *    inputAddr -- If the input UDP source is unicast rather than multicast, then set this to NULL.
    */
    int setDestination(char *const rtspStreamName, char *const inputAddr,int inputPort,int isRawUdp = 0);
private:
    RtspMediaSessionImpl *impl_;
};


/*export HashTable as Util
*/
class MyHashTableImpl;
class MyHashTable{
public:
     MyHashTable();
    ~MyHashTable();
    void* Add(char const* key, void* value);
    void  Remove(char const* key);
    void* Lookup(char const* key) const;
    void* RemoveNext();
    void* getFirst();
private:
    MyHashTableImpl *impl_;
};
#endif //__RTSP_VOD_H_

/************************************************************************************************
*example codes
*************************************************************************************************
 int main(int argc,char *argv[]){
    AmfRtspClientManager::instance()->start_service();

    RtspMediaSession::start_rtsp_server();

    AmfRtspClient *rtspclient;
    if(argc > 1){
        rtspclient  = AmfRtspClientManager::instance()->create_rtspclient(argv[1]);
    }else{
        rtspclient  = AmfRtspClientManager::instance()->create_rtspclient("rtsp://192.168.0.211/h264_2");
    }
    rtspclient->start();

    //
    //rtspMediaSession create/config/start
    //
    RtspMediaSession *ms = new RtspMediaSession;

    AVFormatContext *av_context_ = rtspclient->get_avformat();
    //av_dump_format(av_context_, 0,"rtsp://10.0.0.21/stream1", 0);

    int video_index = -1;
    int audio_index = -1;
    enum CodecID audio_codec_id = CODEC_ID_NONE;
    int i;
    for(i = 0; i < av_context_->nb_streams; i ++){
        AVStream *st = av_context_->streams[i];
        if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO && st->codec->codec_id == CODEC_ID_H264){
            if(st->codec->extradata){
                  ms->addVideoH264Stream(st->codec->extradata,st->codec->extradata_size);
                  video_index = i;
                  printf("H264 added,video_index %d\n",video_index);
            }else{
                  printf("ERROR: H264 extradata not exist\n");
            }
        }else if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_index = i;
            audio_codec_id = st->codec->codec_id;
            switch(st->codec->codec_id){
            case CODEC_ID_AAC:ms->addAudioAACStream(st->codec->sample_rate,st->codec->channels);break;
            case CODEC_ID_PCM_MULAW:ms->addAudioG711Stream(0);break;
            case CODEC_ID_PCM_ALAW:ms->addAudioG711Stream(1);break;
            default:audio_index = -1,audio_codec_id = CODEC_ID_NONE;break;
            }
            if(audio_index != -1);{
                printf("AAC added,audio_index = %d\n",audio_index);
            }
        }
    }
    ms->setDestination("test.amba");

    rtspclient->play();

    //printf("video_index = %d, audio_index = %d\n\n",video_index,audio_index);
    MyStdInput input;
    while(1){
        {
            AVPacket packet, *pkt =&packet;
            if(rtspclient->read_avframe(pkt) == 0){
                if(pkt->stream_index == video_index){
                    ms->sendData(RtspMediaSession::TYPE_H264,pkt->data,pkt->size,-1);
                }else if(pkt->stream_index == audio_index){
                    switch(audio_codec_id){
                    case CODEC_ID_AAC:ms->sendData(RtspMediaSession::TYPE_AAC,pkt->data,pkt->size,-1);break;
                    case CODEC_ID_PCM_MULAW:ms->sendData(RtspMediaSession::TYPE_G711_MU,pkt->data,pkt->size,-1);break;
                    case CODEC_ID_PCM_ALAW:ms->sendData(RtspMediaSession::TYPE_G711_A,pkt->data,pkt->size,-1);break;
                    default:break;
                    }
                }
                av_free_packet(pkt);
            }
        }
        if(input.get_input()){
            //break;
        }
    }

    AmfRtspClientManager::instance()->destroy_rtspclient(rtspclient);

     //destroy rtspMediaSession after rtspclient
    delete ms;
    RtspMediaSession::stop_rtsp_server();

    AmfRtspClientManager::instance()->stop_service();
}
*/


