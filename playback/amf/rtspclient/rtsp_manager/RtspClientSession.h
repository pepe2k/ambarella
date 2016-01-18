#ifndef _RTSP_CLIENT_SESSION_H_
#define _RTSP_CLIENT_SESSION_H_

#if NEW_RTSP_CLIENT

#include <string>
#include "timer.h"
#include "IRtspSession.h"
#include "IMessage.h"
#include "am_timer.h"
#include "RtpSession.h"

class IRtspClientCallback;
class RtspClientSession:public IRtspSession,public IMessage,public IRtpCallback{
public:
    RtspClientSession(const std::string &url,IRtspClientCallback *cb,void *usr_data);
    ~RtspClientSession();

    /**/
    std::string  &get_index() {return url_;}
    void start();
    void play();
    void tear_down();
    void on_disconnect();
    void request_timeout();
    void on_heartbeat_timeout();

    /*rtsp func*/
    virtual void connect_completed(bool result);
    virtual void on_request(const RtspRequestMsg & msg);
    virtual void on_response(RtspMethodsType method,const RtspResponseMsg & msg);
    virtual void on_request_timeout(RtspMethodsType method);

    void on_describe_response(const RtspStatusCodesType & code,const std::string &sdp="");
    void on_setup_response(const RtspStatusCodesType & code,const std::string & sessionid="",unsigned int rtp_port=0);
    void on_play_response(const RtspStatusCodesType & code,const std::string &rtp_info="");
    void on_options_response(const RtspStatusCodesType & code,const std::string &hdr_public="");

    /*IRtpCallback*/
    virtual void on_packet(AVPacket *pkt);

    /**/
    static void send_timeout(CTimer *,void *);
    static void heartbeat_timeout(CTimer *,void *);
private:
    int send_rtsp_options();
    int send_rtsp_describe();
    int send_rtsp_setup_video();
    int send_rtsp_setup_audio();
    int send_rtsp_play();
    int send_rtsp_teardown();
    int send_rtsp_get_parameter();
    void destroy_rtp_sessions();
    int av_parse_media(const std::string &sdp);
    int av_parse_free();
private:
    std::string url_;
    IRtspClientCallback *m_cb;
    void *m_usr_data;

    enum {RTSP_DEFAULT_TIMEOUT = 20000/*20 seconds*/};
    CTimer timer_; //send RTSP options

    //rtsp server ip:port
    std::string rtsp_url_;
    std::string ip_;
    int port_;

    //
    enum {RTSP_SERVER_RTP, RTSP_SERVER_WMS,RTSP_SERVER_REAL} server_type_;
    int get_parameter_supported;

    enum {RTSP_STATE_INVALID,
        RTSP_STATE_CONNECT,
        RTSP_STATE_DESCRIBE,
        RTSP_STATE_SETUP_VIDEO,
        RTSP_STATE_SETUP_AUDIO,
        RTSP_STATE_PLAY,
        RTSP_STATE_FAIL_PLAY
    } play_state_;
    //
    enum {RTSP_FLAG_VIDEO = 1,RTSP_FLAG_AUDIO = 2};
    unsigned int stream_mask;
    std::string sessionid_;

    IRtpSession *rtp_session_video_;
    IRtpSession *rtp_session_audio_;

    AVFormatContext *av_context_;
};

#endif //NEW_RTSP_CLIENT

#endif

