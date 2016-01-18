#if NEW_RTSP_CLIENT

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "RtspClientSession.h"
#include "RtspClientManager.h"
#include "RtpManager.h"
#include "rtsp_utils.h"

#define  AMBA_USER_AGENT "Ambarella New RtspClient v2013.03.28"

void RtspClientSession::send_timeout(CTimer *,void *owner){
    unsigned int id = (unsigned int)owner;
    RtspClientManager::instance()->request_timeout(id);
}

void RtspClientSession::heartbeat_timeout(CTimer *,void *owner){
    unsigned int id = (unsigned int)owner;
    RtspClientManager::instance()->heartbeat_timeout(id);
}

RtspClientSession::RtspClientSession(const std::string &url,IRtspClientCallback *cb,void *usr_data)
	:url_(url), m_cb(cb),m_usr_data(usr_data),timer_(RTSP_DEFAULT_TIMEOUT,NULL,NULL,CTimer::TIMER_CIRCLE),rtp_session_video_(NULL),rtp_session_audio_(NULL)
{
    size_t pos, pos1,pos2;

    server_type_ = RTSP_SERVER_RTP;
    get_parameter_supported = 0;

    stream_mask = 0;// RTSP_FLAG_VIDEO | RTSP_FLAG_AUDIO;

    //parse to get rtsp_url_ and options
    pos = url.find("?");
    if(pos == string::npos){
        rtsp_url_ = url;
    }else{
        //TODO, now options are not supported.
        rtsp_url_.assign(url,0,pos);
    }

    //parse to get "ip" and "port"
    pos = rtsp_url_.find("rtsp://");
    if(pos == string::npos){
        //invalid rtsp_url, todo
        rtsp_url_ = "";
        ip_= "";
        port_ = 554;
    }else{
        pos1 = rtsp_url_.find(":",7);
        if(pos1 == string::npos){
            port_ = 554;
            pos2 = rtsp_url_.find("/",7);
            if(pos2 != string::npos){
                ip_.assign(rtsp_url_,pos + 7,pos2 - pos - 7);
            }
        }else{
            ip_.assign(rtsp_url_,pos + 7,pos1 - pos - 7);
            pos2 = rtsp_url_.find("/",pos1);
            if(pos2 != string::npos){
                std::string str_port;
                str_port.assign(rtsp_url_,pos1 + 1,pos2 - pos1 - 1);
                port_ = atoi(str_port.c_str());
            }
        }
    }
    //printf("RtspClientSession::RtspClientSession --- url[%s], ip[%s],port[%d]\n",rtsp_url_.c_str(),ip_.c_str(),port_);

    play_state_ = RTSP_STATE_CONNECT;
    connect(ip_,port_);
}

RtspClientSession::~RtspClientSession(){
    //printf("RtspClientSession::~RtspClientSession()\n");
    timer_.stop();
    close_connection();
    destroy_rtp_sessions();
    av_parse_free();
}
void RtspClientSession::destroy_rtp_sessions(){
    if(rtp_session_video_){
        //printf("RtspClientSession::destroy_rtp_sessions() rtp_session_video_\n");
        RtpManager::instance()->destroy_rtp_session(rtp_session_video_);
        rtp_session_video_ = NULL;
    }
    if(rtp_session_audio_){
        //printf("RtspClientSession::destroy_rtp_sessions() rtp_session_audio_\n");
        RtpManager::instance()->destroy_rtp_session(rtp_session_audio_);
        rtp_session_audio_ = NULL;
    }
}

int RtspClientSession::send_rtsp_options(){
    RtspRequestMsg msg;
    msg.set_method(RTSP_OPTIONS_MTHD);
    msg.set_url(rtsp_url_);
    if(!sessionid_.empty()){
        msg.set_header(RTSP_SESSION_HDR,sessionid_);
    }
    send_message(encode_request(&msg));
    return 0;
}
int RtspClientSession::send_rtsp_describe(){
    RtspRequestMsg msg;
    msg.set_method(RTSP_DESCRIBE_MTHD);
    msg.set_url(rtsp_url_);
    msg.set_header(RTSP_ACCEPT_HDR,"application/sdp");
    msg.set_header(RTSP_USERAGENT_HDR,AMBA_USER_AGENT);
    if(!sessionid_.empty()){
        msg.set_header(RTSP_SESSION_HDR,sessionid_);
    }
    send_message(encode_request(&msg));
    return 0;
}

int RtspClientSession::send_rtsp_setup_video(){
    std::string request_url;
    FormatContext context;
    memset(&context,0,sizeof(context));
    struct RTSPStream *rtsp_st;

    RTSPState *rt = (RTSPState *)av_context_->priv_data;
    for(int i = 0; i < rt->nb_rtsp_streams; i++){
        rtsp_st = rt->rtsp_streams[i];
        AVStream *st = av_context_->streams[rtsp_st->stream_index];
        if(st->codec->codec_id == CODEC_ID_H264){
            context.codec_id = st->codec->codec_id;
            context.time_base = st->time_base;
            context.stream_index = st->index;
            context.payload_type = rtsp_st->sdp_payload_type;
            request_url = rtsp_st->control_url;
            goto find;
        }
    }
    printf("RtspClientSession::send_rtsp_setup_video, error, not found\n");
    return  -1;
find:
    if(request_url.find("rtsp://") == std::string::npos){
        if(*rtsp_url_.rbegin() == '/'){
            if(request_url[0] == '/'){
                request_url.assign(request_url,1,request_url.length() - 1);
            }
            request_url = rtsp_url_ + request_url;
        }else{
            request_url = rtsp_url_ + request_url;
        }
    }
    rtp_session_video_ = RtpManager::instance()->create_rtp_session(this,&context,id());
    if(!rtp_session_video_){
        printf("RtspClientSession::send_rtsp_setup_video() -- create_rtp_session failed\n");
        return -1;
    }
    rtsp_st->transport_priv = (void*)rtp_session_video_;

    int port = rtp_session_video_->get_local_port();
    RtspRequestMsg msg;
    msg.set_method(RTSP_SETUP_MTHD);
    msg.set_url(request_url);
    char bufTrans[200];
    sprintf(bufTrans,"RTP/AVP/UDP;unicast;client_port=%d-%d",port ,port  + 1);
    msg.set_header(RTSP_TRANSPORT_HDR,bufTrans);
    if(!sessionid_.empty()){
        msg.set_header(RTSP_SESSION_HDR,sessionid_);
    }
    msg.set_header(RTSP_USERAGENT_HDR,AMBA_USER_AGENT);
    send_message(encode_request(&msg));
    return 0;
}

int  RtspClientSession::send_rtsp_setup_audio(){
    std::string request_url;
    FormatContext context;
    memset(&context,0,sizeof(context));
    struct RTSPStream *rtsp_st;

    RTSPState *rt = (RTSPState *)av_context_->priv_data;
    for(int i = 0; i < rt->nb_rtsp_streams; i++){
        rtsp_st = rt->rtsp_streams[i];
        AVStream *st = av_context_->streams[rtsp_st->stream_index];
        if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            context.codec_id = st->codec->codec_id;
            context.time_base = st->time_base;
            context.stream_index = st->index;
            context.payload_type = rtsp_st->sdp_payload_type;
            request_url = rtsp_st->control_url;
            goto find;
        }
    }
    return  -1;
find:
    if(request_url.find("rtsp://") == std::string::npos){
        if(*rtsp_url_.rbegin() == '/'){
            if(request_url[0] == '/'){
                request_url.assign(request_url,1,request_url.length() - 1);
            }
            request_url = rtsp_url_ + request_url;
        }else{
            request_url = rtsp_url_ + request_url;
        }
    }
    rtp_session_audio_ = RtpManager::instance()->create_rtp_session(this,&context,id());
    if(!rtp_session_audio_){
        printf("RtspClientSession::send_rtsp_setup_audio() -- create_rtp_session failed\n");
        return -1;
    }
    rtsp_st->transport_priv = (void*)rtp_session_audio_;

    int port = rtp_session_audio_->get_local_port();
    RtspRequestMsg msg;
    msg.set_method(RTSP_SETUP_MTHD);
    msg.set_url(request_url);
    char bufTrans[200];
    sprintf(bufTrans,"RTP/AVP/UDP;unicast;client_port=%d-%d",port,port + 1);
    msg.set_header(RTSP_TRANSPORT_HDR,bufTrans);
    if(!sessionid_.empty()){
        msg.set_header(RTSP_SESSION_HDR,sessionid_);
    }
    msg.set_header(RTSP_USERAGENT_HDR,AMBA_USER_AGENT);
    send_message(encode_request(&msg));
    return 0;
}

int RtspClientSession::send_rtsp_play(){
    RtspRequestMsg msg;
    msg.set_method(RTSP_PLAY_MTHD);
    msg.set_url(rtsp_url_);
    if(!sessionid_.empty()){
        msg.set_header(RTSP_SESSION_HDR,sessionid_);
    }
    msg.set_header(RTSP_RANGE_HDR,"npt=0.000-");
    msg.set_header(RTSP_USERAGENT_HDR,AMBA_USER_AGENT);
    send_message(encode_request(&msg));
    return 0;
}

int RtspClientSession::send_rtsp_teardown(){
    RtspRequestMsg msg;
    msg.set_method(RTSP_TEARDOWN_MTHD);
    msg.set_url(rtsp_url_);
    if(!sessionid_.empty()){
        msg.set_header(RTSP_SESSION_HDR,sessionid_);
    }
    msg.set_header(RTSP_USERAGENT_HDR,AMBA_USER_AGENT);
    send_message(encode_request(&msg));
    return 0;
}

int RtspClientSession::send_rtsp_get_parameter(){
    RtspRequestMsg msg;
    msg.set_method(RTSP_GET_PARAMETER_MTHD);
    msg.set_url(rtsp_url_);
    if(!sessionid_.empty()){
        msg.set_header(RTSP_SESSION_HDR,sessionid_);
    }
    msg.set_header(RTSP_USERAGENT_HDR,AMBA_USER_AGENT);
    send_message(encode_request(&msg));
    return 0;
}

void RtspClientSession::connect_completed(bool result){
    if(result==true){
        set_owner((void *)id());
        set_timer_param(&send_timeout,(void*)id());
    }
    if(m_cb){
        m_cb->on_rtsp_create_session(result,id(),m_usr_data);
    }
}
void RtspClientSession::on_response(RtspMethodsType method,const RtspResponseMsg & msg){
    switch(method){
    case RTSP_OPTIONS_MTHD:
        on_options_response(msg.get_code(),msg.get_header(RTSP_PUBLIC_HDR));
        break;
    case RTSP_DESCRIBE_MTHD:
        if(msg.get_header(RTSP_CONTENT_TYPE_HDR) != "application/sdp"){
            on_describe_response(RTSP_UNKNOWN_STATUS, "");
        }else{
            std::string conent_base = msg.get_header(RTSP_CONTENT_BASE_HDR);
            if(conent_base!= ""){
                rtsp_url_ = conent_base;
            }
            on_describe_response(msg.get_code(),msg.get_body());
        }
        break;
    case RTSP_SETUP_MTHD:
        {
            string transport=msg.get_header(RTSP_TRANSPORT_HDR);
            size_t begin;
            unsigned int rtp_port=0;
            if((begin=transport.find("server_port="))!=string::npos){
                sscanf(transport.substr(begin+12).c_str(),"%d",&rtp_port);
            }
            on_setup_response(msg.get_code(),msg.get_header(RTSP_SESSION_HDR),rtp_port);
        }
        break;
    case RTSP_PLAY_MTHD:
        on_play_response(msg.get_code(), msg.get_header(RTSP_RTPINFO_HDR));
        break;
    case RTSP_GET_PARAMETER_MTHD:
        //TODO
        break;
    default:
        break;
    }
}

void RtspClientSession::on_request(const RtspRequestMsg & msg){
    RtspMethodsType method=msg.get_method();
    switch(method){
    default:
       printf("RtspClientSession::on_request, unsupported method %d\n",method);
       break;
    }
}
void RtspClientSession::on_request_timeout(RtspMethodsType method)
{
    switch(method){
    case RTSP_OPTIONS_MTHD:
        on_options_response(RTSP_408_STATUS);
        break;
    case RTSP_DESCRIBE_MTHD:
        on_describe_response(RTSP_408_STATUS);
        break;
    case RTSP_SETUP_MTHD:
        on_setup_response(RTSP_408_STATUS);
        break;
    case RTSP_PLAY_MTHD:
        on_play_response(RTSP_408_STATUS);
        break;
    default:
        break;
    }
}

//
void RtspClientSession::start(){
    play_state_ = RTSP_STATE_DESCRIBE;
    send_rtsp_options();
}
void RtspClientSession::on_options_response(const RtspStatusCodesType & code,const std::string &hdr_public){
    if(code>=RTSP_200_STATUS&&code<RTSP_300_STATUS){
        if(hdr_public.find("GET_PARAMETER") != string::npos){
            get_parameter_supported = 1;
        }else{
            get_parameter_supported = 0;
        }
        if(play_state_ == RTSP_STATE_DESCRIBE){
            send_rtsp_describe();
        }
    }else{
        if(play_state_ == RTSP_STATE_DESCRIBE){
            if(m_cb){
                m_cb->on_rtsp_start(id(),false,"", m_usr_data);
            }
        }else{
            //TODO, notify manager to disconnect and free resources
        }
    }
}

void RtspClientSession::on_describe_response(const RtspStatusCodesType & code,const std::string &sdp)
{
    //printf("RtspClientSession::on_describe_response\n");
    if(code>=RTSP_200_STATUS&&code<RTSP_300_STATUS){
        if(!av_parse_media(sdp)){
            if(m_cb){
                m_cb->on_rtsp_start(id(), true,sdp,m_usr_data);
            }
            return;
        }
    }
    if(m_cb){
        m_cb->on_rtsp_start(id(),false, "",m_usr_data);
    }
}

void  RtspClientSession::play(){
    //printf("RtspClientSession::play, stream_mask = %d\n",stream_mask);
    int result = -1;
    if(stream_mask & RTSP_FLAG_VIDEO){
        play_state_ = RTSP_STATE_SETUP_VIDEO;
        result = send_rtsp_setup_video();
    }else if(stream_mask & RTSP_FLAG_AUDIO){
        play_state_ = RTSP_STATE_SETUP_AUDIO;
        result = send_rtsp_setup_audio();
    }else{
        play_state_ = RTSP_STATE_FAIL_PLAY;
    }

    if(result < 0){
        if(m_cb){
            m_cb->on_rtsp_play(id(),false,m_usr_data);
        }
    }
}

void RtspClientSession::on_setup_response(const RtspStatusCodesType & code,const string & sessionid,unsigned int rtp_port)
{
    int result = -1;
    if(code>=RTSP_200_STATUS&&code<RTSP_300_STATUS){
        if(sessionid_.empty()){
/*
VLC RTSP-SERVER:

SETUP rtsp://10.0.0.1:554/mpeg1.mpg/trackID=25 RTSP/1.0
Transport: RTP/AVP/UDP;unicast;client_port=30000-30001
User-Agent: Ambarella New RtspClient v2013.03.31
CSeq: 3

RTSP/1.0 200 OK
Server: VLC/2.1.0-git-20121008-0003
Date: Mon, 11 Mar 2013 08:38:40 GMT
Transport: RTP/AVP/UDP;unicast;client_port=30000-30001;server_port=61441-61442;ssrc=13FAE354;mode=play
Session: 2bef5b2fa73832fb;timeout=60
Content-Length: 0
Cache-Control: no-cache
Cseq: 3
*/
            size_t pos = sessionid.find(";");
            if(pos == std::string::npos){
                sessionid_ = sessionid;
            }else{
                sessionid_.assign(sessionid,0,pos);
            }
        }
        if(play_state_ == RTSP_STATE_SETUP_VIDEO){
            rtp_session_video_->set_peer_addr(ip_,rtp_port);
            if(stream_mask & RTSP_FLAG_AUDIO){
                play_state_ = RTSP_STATE_SETUP_AUDIO;
                result = send_rtsp_setup_audio();
            }else{
                play_state_ = RTSP_STATE_PLAY;
                result = send_rtsp_play();
            }
        }else if(play_state_ == RTSP_STATE_SETUP_AUDIO){
            rtp_session_audio_->set_peer_addr(ip_,rtp_port);
            play_state_ = RTSP_STATE_PLAY;
            result = send_rtsp_play();
        }else{
            printf("RtspClientSession::on_setup_response() -- should not go here\n");
        }
    }

    if(result < 0){
        destroy_rtp_sessions();
        if(m_cb){
            m_cb->on_rtsp_play(id(),false,m_usr_data);
        }
    }
}

void RtspClientSession::on_play_response(const RtspStatusCodesType & code,const string &rtp_info)
{
    if(code>=RTSP_200_STATUS&&code<RTSP_300_STATUS){
        if(1/*!rtsp_rtpinfo_parse(av_context_,rtp_info.c_str())*/){
            if(m_cb){
                m_cb->on_rtsp_play(id(),true,m_usr_data);
            }
            //start heartbeat timer
            timer_.set_func_data(&heartbeat_timeout,(void*)id());
            timer_.start();
            return;
        }
    }
    if(m_cb){
        m_cb->on_rtsp_play(id(),false,m_usr_data);
    }
}

void RtspClientSession::tear_down(){
   //printf("RtspClientSession::tear_down, id = %d\n",id());
   send_rtsp_teardown();
   //todo
   timer_.stop();
   close_connection();
   destroy_rtp_sessions();
}

void RtspClientSession::on_disconnect(){
    //printf("RtspClientSession::on_disconnect, id = %d\n",id());
    if(m_cb){
        m_cb->on_rtsp_error(id(),AVERROR(EIO),m_usr_data);
    }
    //todo
    timer_.stop();
    close_connection();
    destroy_rtp_sessions();
}

void RtspClientSession::request_timeout(){
    //printf("RtspClientSession::request_timeout  called\n");
    check_timeout();
}

//#define AMBA_CHECK_FPS_PTS

#ifdef AMBA_CHECK_FPS_PTS
#include <sys/time.h>
pthread_mutex_t test_mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned int  frame_cnt[2] = {0};
long long pts[2] = {0};
#endif

void RtspClientSession::on_heartbeat_timeout(){
#ifdef AMBA_CHECK_FPS_PTS
    struct timeval tv;
    pthread_mutex_lock(&test_mutex);
    gettimeofday(&tv,NULL);
    printf("gettimeofday -- tv.tv_sec = %10d, tv.tv_usec = %10d, frame_cnt_0[%10d],pts[%20lld]\n",tv.tv_sec,tv.tv_usec,frame_cnt[0],pts[0]);
    pthread_mutex_unlock(&test_mutex);
#endif

    //TODO, server_type && get_parameter_supported
    if(get_parameter_supported){
        send_rtsp_get_parameter();
    }else{
        send_rtsp_options();
    }
}


void RtspClientSession::on_packet(AVPacket *pkt){
    //printf("on_packet -- len = %08d,index = %d,pts = %lld\n",pkt->size,pkt->stream_index,pkt->pts);

#ifdef AMBA_CHECK_FPS_PTS
    pthread_mutex_lock(&test_mutex);
    ++frame_cnt[pkt->stream_index];
    pts[pkt->stream_index] = pkt->pts;
    pthread_mutex_unlock(&test_mutex);
#endif

    if(m_cb){
        m_cb->on_rtsp_avframe(id(),pkt,m_usr_data);
    }else{
        av_free_packet(pkt);
    }
}


extern "C"{
    #include "libavformat/avformat.h"
    #include "libavcodec/avcodec.h"
    #include "libavutil/avstring.h"
};
#include "am_ffmpeg.h"

int RtspClientSession:: av_parse_media(const std::string &sdp)
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
        goto fail;
    }

    /*check codec_type, now only H264 and AAC/G711 supported
    */
    int i;
    for(i = 0; i < (int)av_context_->nb_streams; i ++){
        AVStream *st = av_context_->streams[i];
        if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            if(st->codec->codec_id != CODEC_ID_H264){
                goto fail;
            }
            stream_mask |= RTSP_FLAG_VIDEO;
            st->codec->pix_fmt = PIX_FMT_YUV420P;//TODO

            if(st->codec->extradata){
                //
                //ffmpeg extradata
                //    StartCode + SPS: 00  00  00  01 67  64  00  1f  ac  d9  40  50  05  bb  ff  00 86  00  a1  10  00  00  03  00  10  00  00  03  03  28  f1  83 19  60
                //    StartCode + PPS: 00  00  00  01 68  eb  e3  cb  22  c0
                //
                H264_INFO h264_info;
                memset(&h264_info,0,sizeof(h264_info));
                if(get_h264_info(st->codec->extradata + 5,&h264_info) < 0){
                    printf("RtspClientSession:: av_parse_media --- failed to get H264 info\n");
                    goto fail;
                }
                st->codec->width = h264_info.width;
                st->codec->height = h264_info.height;
                st->codec->time_base.den = h264_info.time_base_num;
                st->codec->time_base.num = h264_info.time_base_den;
                st->r_frame_rate.den = h264_info.time_base_den;
                st->r_frame_rate.num = h264_info.time_base_num;
            }
        }
        else if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            if(st->codec->codec_id == CODEC_ID_AAC
                || st->codec->codec_id == CODEC_ID_PCM_ALAW
                || st->codec->codec_id == CODEC_ID_PCM_MULAW
                || st->codec->codec_id == CODEC_ID_ADPCM_G726){
                stream_mask |= RTSP_FLAG_AUDIO;
            }else{
                goto fail;
            }
        }
    }
    if(!stream_mask){
        goto fail;
    }
    //printf("RtspClientSession:: av_parse_media() OK\n");
    //av_dump_format(av_context_, 0,rtsp_url_.c_str(), 0);
    return 0;
 fail:
    av_parse_free();
    return err;
}

int RtspClientSession::av_parse_free(){
    if (av_context_) {
        RTSPState *rt = (RTSPState *)av_context_->priv_data;
        for (int i = 0; i < rt->nb_rtsp_streams; i++) {
            RTSPStream *rtsp_st = rt->rtsp_streams[i];
            if (rtsp_st) {
                if (rtsp_st->dynamic_handler && rtsp_st->dynamic_protocol_context)
                    rtsp_st->dynamic_handler->close(rtsp_st->dynamic_protocol_context);
                av_free(rtsp_st);
            }
        }
        av_freep(&av_context_->priv_data);
        for(int i=0;i<(int)av_context_->nb_streams;i++) {
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

