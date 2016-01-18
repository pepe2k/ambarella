#ifndef _RTP_DEMUXER_H_
#define _RTP_DEMUXER_H_

#if NEW_RTSP_CLIENT

/*RTPDemuxer related codes are copied from ffmpeg and modified
*/
extern "C"{
   #include "libavformat/avformat.h"
   #include "libavformat/rtpdec.h"
};
#include "IRtpSession.h"
#include "active_object.h"

struct FormatContext{
    int payload_type;
    enum CodecID codec_id;
    AVRational time_base;
    int stream_index;
};

//TODO
struct RTPPacketPool{
    RTPPacketPool(int max_size = 512):queue_size_(max_size),r_index_(0),w_index_(0),len_(max_size){
        item_ = (RTPPacket*)av_mallocz(sizeof(RTPPacket)*max_size);
        item_end_ = &item_[max_size -1];
    }
    ~RTPPacketPool(){
        if(item_){
            av_free(item_);
            item_ = NULL;
        }
    }
    RTPPacket *getNode(){
        RTPPacket *pkt;
        if(len_ > 0){
            pkt = &item_[r_index_];
            ++r_index_;
            r_index_ &= (queue_size_ -1);
            len_--;
            return pkt;
        }
        pkt = (RTPPacket *)av_mallocz(sizeof(RTPPacket));
        return pkt;
    }
    void returnNode(RTPPacket *pkt){
        if(pkt >= item_ && pkt <= item_end_){
            ++w_index_;
            w_index_ &= (queue_size_ -1);
            len_++;
        }else{
            av_free((void*)pkt);
        }
    }
private:
    int queue_size_;
    int r_index_;
    int w_index_;
    int len_;
    RTPPacket *item_;
    RTPPacket *item_end_;
};

struct DemuxContext{
    IRtpSession *rtp_ctx_;
    int payload_type;
    enum CodecID codec_id;
    AVRational time_base;
    int stream_index;

    uint32_t ssrc;
    int ssrc_flag;
    uint16_t seq;
    uint32_t timestamp;
    uint32_t base_timestamp;
    uint32_t cur_timestamp;
    int64_t  unwrapped_timestamp;
    int64_t  range_start_offset;
    int max_payload_size;

    /* used to send back RTCP RR */
    //URLContext *rtp_ctx;
    //char hostname[256];

    RTPStatistics statistics; ///< Statistics for this stream (used by RTCP receiver reports)

    /** Fields for packet reordering @{ */
    int prev_ret;     ///< The return value of the actual parsing of the previous packet
    RTPPacket* queue; ///< A sorted queue of buffered packets not yet returned
    int queue_len;    ///< The number of packets in queue
    int queue_size;   ///< The size of queue, or 0 if reordering is disabled
    /*@}*/

    /*Fields for h264 packet-loss processing and frame-combination
    */
    int handle_h264_packet_loss;
    RTPPacket* recv_queue;
    int recv_queue_len;
    uint32_t recv_queue_timestamp;
    RTPPacket *queue_tail;
    RTPPacket* recv_queue_tail;

    /* rtcp sender statistics receive */
    int64_t last_rtcp_ntp_time;    //  TODO: move into statistics
    int64_t first_rtcp_ntp_time;   //  TODO: move into statistics
    uint32_t last_rtcp_timestamp;  //  TODO: move into statistics
    int64_t rtcp_ts_offset;

    /* rtcp sender statistics */
    unsigned int packet_count;     //  TODO: move into statistics (outgoing)
    unsigned int octet_count;      //  TODO: move into statistics (outgoing)
    unsigned int last_octet_count; //  TODO: move into statistics (outgoing)
    int first_packet;

    /* dynamic payload stuff */
    void *dynamic_protocol_context;        ///< This is a copy from the values setup from the sdp parsing, in rtsp.c don't free me.

    RTPPacketPool *pool;
};

class IRtpCallback{
public:
    virtual ~IRtpCallback(){}
    virtual void on_packet(AVPacket *pkt) = 0;
};

class RTPDemuxer{
public:
    RTPDemuxer();
    ~RTPDemuxer();
    int rtp_parse_open(IRtpSession *rtp_ctx,FormatContext *context);
    int rtp_parse_packet(IRtpCallback *cb,uint8_t **buf,int len);
    void rtp_parse_close();

    static int start_service(int num);
    static int stop_service();
private:
    DemuxContext context_;
    void *service;
    enum {DEMUXER_STOP = 1,DEMUXER_START} state_;
};

#endif //NEW_RTSP_CLIENT

#endif //_RTP_DEMUXER_H_


