#ifndef _RTMP_CLIENT_H_
#define _RTMP_CLIENT_H_

#include <stdio.h>
class RtmpClientImpl;
class RtmpClient{
public:
    RtmpClient();
    ~RtmpClient();
public:
    int addVideoH264Stream(unsigned char *extra_data,int extra_data_len,int bitrate = 0);
    int addAudioAACStream(int sample_rate,int channels,int bitrate = 0);
    int addAudioG711Stream(int isALaw);
    /*setDestination() must be called after  addVideoH264Stream()/addAudioAACStream()/etc.
    */
    int setDestination(char *const rtmpUrl);

    enum DataType {TYPE_INVALID,TYPE_H264 = 100, TYPE_AAC = 200,TYPE_G711_A,TYPE_G711_MU};
    int sendData(DataType type,unsigned char *data,int len,unsigned int timestamp);
private:
    RtmpClientImpl *impl_;
};
#endif //_RTMP_CLIENT_H_

/*
*  example codes:
*
int main(int argc,char *argv[])
{
    AmfRtspClientManager::instance()->start_service();
  
    AmfRtspClient *rtspclient;
    if(argc > 1){
        rtspclient  = AmfRtspClientManager::instance()->create_rtspclient(argv[1]);
    }else{
        rtspclient  = AmfRtspClientManager::instance()->create_rtspclient("rtsp://10.0.0.21/stream1");
    }
    rtspclient->start();

    //
    //rtmpInjector create/config/start
    //
    RtmpClient *injector = new RtmpClient;
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
                  injector->addVideoH264Stream(st->codec->extradata,st->codec->extradata_size);
                  video_index = i;
                  printf("H264 added,video_index %d\n",video_index);
            }else{
                  printf("ERROR: H264 extradata not exist\n");
            }
        }else if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_index = i;
            audio_codec_id = st->codec->codec_id;
            switch(st->codec->codec_id){
            case CODEC_ID_AAC:injector->addAudioAACStream(st->codec->sample_rate,st->codec->channels);break;
            case CODEC_ID_PCM_MULAW:injector->addAudioG711Stream(0);break;
            case CODEC_ID_PCM_ALAW:injector->addAudioG711Stream(1);break;
            default:audio_index = -1,audio_codec_id = CODEC_ID_NONE;break;
            }
            if(audio_index != -1);{
                printf("AAC added,audio_index = %d\n",audio_index);
            }
        }
    }
    injector->setDestination("rtmp://121.199.36.25/livepkgr/demo?adbe-live-event=liveevent");

    rtspclient->play();

    //printf("video_index = %d, audio_index = %d\n\n",video_index,audio_index);
    while(1){
        {
            AVPacket packet, *pkt =&packet;
            if(rtspclient->read_avframe(pkt) == 0){
                //We do not use pkt->timestamp now, TODO.
                if(pkt->stream_index == video_index){
                    injector->sendData(RtmpClient::TYPE_H264,pkt->data,pkt->size,0);
                }else if(pkt->stream_index == audio_index){
                    switch(audio_codec_id){
                    case CODEC_ID_AAC:injector->sendData(RtmpClient::TYPE_AAC,pkt->data,pkt->size,0);break;
                    case CODEC_ID_PCM_MULAW:injector->sendData(RtmpClient::TYPE_G711_MU,pkt->data,pkt->size,0);break;
                    case CODEC_ID_PCM_ALAW:injector->sendData(RtmpClient::TYPE_G711_A,pkt->data,pkt->size,0);break;
                    default:break;
                    }
                }
                av_free_packet(pkt);
            }
        }
        if(get_input()){
            break;
        }
    }

    AmfRtspClientManager::instance()->destroy_rtspclient(rtspclient);

     //destroy rtmpclient after rtspclient
    delete injector;

    AmfRtspClientManager::instance()->stop_service();
}
*/

