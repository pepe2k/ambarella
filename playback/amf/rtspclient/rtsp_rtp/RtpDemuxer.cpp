#if NEW_RTSP_CLIENT

#include <map>
#include <vector>
#include <strings.h>
extern "C"{
    #include "libavcodec/get_bits.h"
    #include "libavformat/avformat.h"
    #include "libavformat/rtp.h"
};
#include "RtpDemuxer.h"


//FOR AAC, from ffmpeg
struct PayloadContext
{
    int sizelength;
    int indexlength;
    int indexdeltalength;
    int profile_level_id;
    int streamtype;
    int objecttype;
    char *mode;

    /** mpeg 4 AU headers */
    struct AUHeaders {
        int size;
        int index;
        int cts_flag;
        int cts;
        int dts_flag;
        int dts;
        int rap_flag;
        int streamstate;
    } *au_headers;
    int au_headers_allocated;
    int nb_au_headers;
    int au_headers_length_bytes;
    int cur_au_index;
};

static PayloadContext *new_context(void)
{
    return (PayloadContext*)av_mallocz(sizeof(PayloadContext));
}

static void free_context(PayloadContext * data)
{
    int i;
    for (i = 0; i < data->nb_au_headers; i++) {
         /* according to rtp_parse_mp4_au, we treat multiple
          * au headers as one, so nb_au_headers is always 1.
          * loop anyway in case this changes.
          * (note: changes done carelessly might lead to a double free)
          */
       av_free(&data->au_headers[i]);
    }
    av_free(data->mode);
    av_free(data);
}

static int rtp_parse_mp4_au(PayloadContext *data, const uint8_t *buf)
{
    int au_headers_length, au_header_size, i;
    GetBitContext getbitcontext;

    /* decode the first 2 bytes where the AUHeader sections are stored
       length in bits */
    au_headers_length = AV_RB16(buf);

    if (au_headers_length > RTP_MAX_PACKET_LENGTH)
      return -1;

    data->au_headers_length_bytes = (au_headers_length + 7) / 8;

    /* skip AU headers length section (2 bytes) */
    buf += 2;

    init_get_bits(&getbitcontext, buf, data->au_headers_length_bytes * 8);

    /* XXX: Wrong if optionnal additional sections are present (cts, dts etc...) */
    au_header_size = data->sizelength + data->indexlength;
    if (au_header_size <= 0 || (au_headers_length % au_header_size != 0))
        return -1;

    data->nb_au_headers = au_headers_length / au_header_size;
    if (!data->au_headers || data->au_headers_allocated < data->nb_au_headers) {
        av_free(data->au_headers);
        data->au_headers = (PayloadContext::AUHeaders*)av_malloc(sizeof(PayloadContext::AUHeaders) * data->nb_au_headers);
        data->au_headers_allocated = data->nb_au_headers;
    }

    /* XXX: We handle multiple AU Section as only one (need to fix this for interleaving)
       In my test, the FAAD decoder does not behave correctly when sending each AU one by one
       but does when sending the whole as one big packet...  */
    data->au_headers[0].size = 0;
    data->au_headers[0].index = 0;
    for (i = 0; i < data->nb_au_headers; ++i) {
        data->au_headers[0].size += get_bits_long(&getbitcontext, data->sizelength);
        data->au_headers[0].index = get_bits_long(&getbitcontext, data->indexlength);
    }

    data->nb_au_headers = 1;

    return 0;
}


/* Follows RFC 3640 */
static int aac_parse_packet(PayloadContext *data, int stream_index, AVPacket *pkt, const uint8_t *buf, int len)
{
    if (rtp_parse_mp4_au(data, buf))
        return -1;

    buf += data->au_headers_length_bytes + 2;
    len -= data->au_headers_length_bytes + 2;

    /* XXX: Fixme we only handle the case where rtp_parse_mp4_au define
                    one au_header */
    av_new_packet2(pkt, data->au_headers[0].size);
    memcpy(pkt->data, buf, data->au_headers[0].size);

    pkt->stream_index = stream_index;
    return 0;
}

/*
static int aac_parse_fmtp(PayloadContext *data, char *attr, char *value)
{
    //Looking for a known attribute
    for (int i = 0; attr_names[i].str; ++i) {
        if (!strcasecmp(attr, attr_names[i].str)) {
            if (attr_names[i].type == ATTR_NAME_TYPE_INT) {
                *(int *)((char *)data+ attr_names[i].offset) = atoi(value);
            } else if (attr_names[i].type == ATTR_NAME_TYPE_STR){
                *(char **)((char *)data+ attr_names[i].offset) = av_strdup(value);
            }
        }
    }
    return 0;
}
*/

#define RTP_SEQ_MOD (1<<16)

/**
* called on parse open packet
*/
static void rtp_init_statistics(RTPStatistics *s, uint16_t base_sequence) // called on parse open packet.
{
    memset(s, 0, sizeof(RTPStatistics));
    s->max_seq= base_sequence;
    s->probation= 1;
}

/**
* called whenever there is a large jump in sequence numbers, or when they get out of probation...
*/
static void rtp_init_sequence(RTPStatistics *s, uint16_t seq)
{
    s->max_seq= seq;
    s->cycles= 0;
    s->base_seq= seq -1;
    s->bad_seq= RTP_SEQ_MOD + 1;
    s->received= 0;
    s->expected_prior= 0;
    s->received_prior= 0;
    s->jitter= 0;
    s->transit= 0;
}

/**
* returns 1 if we should handle this packet.
*/
static int rtp_valid_packet_in_sequence(RTPStatistics *s, uint16_t seq)
{
    uint16_t udelta= seq - s->max_seq;
    const int MAX_DROPOUT= 3000;
    const int MAX_MISORDER = 100;
    const int MIN_SEQUENTIAL = 2;

    /* source not valid until MIN_SEQUENTIAL packets with sequence seq. numbers have been received */
    if(s->probation)
    {
        if(seq==s->max_seq + 1) {
            s->probation--;
            s->max_seq= seq;
            if(s->probation==0) {
                rtp_init_sequence(s, seq);
                s->received++;
                return 1;
            }
        } else {
            s->probation= MIN_SEQUENTIAL - 1;
            s->max_seq = seq;
        }
    } else if (udelta < MAX_DROPOUT) {
        // in order, with permissible gap
        if(seq < s->max_seq) {
            //sequence number wrapped; count antother 64k cycles
            s->cycles += RTP_SEQ_MOD;
        }
        s->max_seq= seq;
    } else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
        // sequence made a large jump...
        if(seq==s->bad_seq) {
            // two sequential packets-- assume that the other side restarted without telling us; just resync.
            rtp_init_sequence(s, seq);
        } else {
            s->bad_seq= (seq + 1) & (RTP_SEQ_MOD-1);
            return 0;
        }
    } else {
        // duplicate or reordered packet...
    }
    s->received++;
    return 1;
}

static uint8_t *avio_w8(uint8_t *buf, int b){
    *buf = b;
    return buf + 1;
}
static uint8_t *avio_wb16(uint8_t *buf, unsigned int val){
    buf = avio_w8(buf, val >> 8);
    buf = avio_w8(buf, val);
    return buf;
}

static uint8_t *avio_wb32(uint8_t *buf, unsigned int val){
    buf = avio_w8(buf, val >> 24);
    buf = avio_w8(buf + 1, val >> 16);
    buf = avio_w8(buf + 2, val >> 8);
    buf = avio_w8(buf + 3, val);
    return buf;
}

static void finalize_packet(DemuxContext *s, AVPacket *pkt, uint32_t timestamp)
{
    if (pkt->pts != (int64_t)AV_NOPTS_VALUE || pkt->dts != (int64_t)AV_NOPTS_VALUE)
        return; /* Timestamp already set by depacketizer */
    if (timestamp == RTP_NOTS_VALUE)
        return;

    if (s->last_rtcp_ntp_time != (int64_t)AV_NOPTS_VALUE /*&& s->ic->nb_streams > 1*/) {
        int64_t addend;
        int delta_timestamp;

        /* compute pts from timestamp with received ntp_time */
        delta_timestamp = timestamp - s->last_rtcp_timestamp;
        /* convert to the PTS timebase */
        addend = av_rescale(s->last_rtcp_ntp_time - s->first_rtcp_ntp_time, s->time_base.den, (uint64_t)s->time_base.num << 32);
        pkt->pts = s->range_start_offset + s->rtcp_ts_offset + addend +
                   delta_timestamp;
        return;
    }

    if (!s->base_timestamp)
        s->base_timestamp = timestamp;
    /* assume that the difference is INT32_MIN < x < INT32_MAX, but allow the first timestamp to exceed INT32_MAX */
    if (!s->timestamp)
        s->unwrapped_timestamp += timestamp;
    else
        s->unwrapped_timestamp += (int32_t)(timestamp - s->timestamp);
    s->timestamp = timestamp;
    pkt->pts = s->unwrapped_timestamp + s->range_start_offset - s->base_timestamp;
}

static int rtp_check_and_send_back_rr(DemuxContext *s,int count)
{
    //printf("rtp_check_and_send_back_rr() --- called()\n");
    uint8_t buf[2048];
    int len;
    int rtcp_bytes;
    RTPStatistics *stats= &s->statistics;
    uint32_t lost;
    uint32_t extended_max;
    uint32_t expected_interval;
    uint32_t received_interval;
    uint32_t lost_interval;
    uint32_t expected;
    uint32_t fraction;
    uint64_t ntp_time= s->last_rtcp_ntp_time; //  TODO: Get local ntp time?

    if (count < 1){
        return -1;
    }

    /* TODO: I think this is way too often; RFC 1889 has algorithm for this */
    /* XXX: mpeg pts hardcoded. RTCP send every 0.5 seconds */
    s->octet_count += count;
    rtcp_bytes = ((s->octet_count - s->last_octet_count) * RTCP_TX_RATIO_NUM) /RTCP_TX_RATIO_DEN;
    rtcp_bytes /= 50; // mmu_man: that's enough for me... VLC sends much less btw !?
    if (rtcp_bytes < 28){
        return -1;
    }
    s->last_octet_count = s->octet_count;

    // Receiver Report
    uint8_t *buf_ptr = buf;
    buf_ptr = avio_w8(buf_ptr, (RTP_VERSION << 6) + 1); /* 1 report block */
    buf_ptr = avio_w8(buf_ptr, RTCP_RR);
    buf_ptr = avio_wb16(buf_ptr, 7); /* length in words - 1 */
    // our own SSRC: we use the server's SSRC + 1 to avoid conflicts
    buf_ptr = avio_wb32(buf_ptr, s->ssrc + 1);
    buf_ptr = avio_wb32(buf_ptr, s->ssrc); // server SSRC
    // some placeholders we should really fill...
    // RFC 1889/p64
    extended_max= stats->cycles + stats->max_seq;
    expected= extended_max - stats->base_seq + 1;
    lost= expected - stats->received;
    lost= FFMIN(lost, 0xffffff); // clamp it since it's only 24 bits...
    expected_interval= expected - stats->expected_prior;
    stats->expected_prior= expected;
    received_interval= stats->received - stats->received_prior;
    stats->received_prior= stats->received;
    lost_interval= expected_interval - received_interval;
    if (expected_interval==0 || lost_interval<=0) fraction= 0;
    else fraction = (lost_interval<<8)/expected_interval;

    fraction= (fraction<<24) | lost;

    buf_ptr = avio_wb32(buf_ptr, fraction); /* 8 bits of fraction, 24 bits of total packets lost */
    buf_ptr = avio_wb32(buf_ptr, extended_max); /* max sequence received */
    buf_ptr = avio_wb32(buf_ptr, stats->jitter>>4); /* jitter */

    if(s->last_rtcp_ntp_time==(int64_t)AV_NOPTS_VALUE)
    {
        buf_ptr = avio_wb32(buf_ptr, 0); /* last SR timestamp */
        buf_ptr = avio_wb32(buf_ptr, 0); /* delay since last SR */
    } else {
        uint32_t middle_32_bits= s->last_rtcp_ntp_time>>16; // this is valid, right? do we need to handle 64 bit values special?
        uint32_t delay_since_last= ntp_time - s->last_rtcp_ntp_time;

        buf_ptr = avio_wb32(buf_ptr, middle_32_bits); /* last SR timestamp */
        buf_ptr = avio_wb32(buf_ptr, delay_since_last); /* delay since last SR */
    }

    // CNAME
    char hostname[128];
    hostname[0] = '\0';
    len = 0;
    if(s->rtp_ctx_){
        len = snprintf(hostname,sizeof(hostname),"%s",s->rtp_ctx_->get_hostname().c_str());
    }
    buf_ptr = avio_w8(buf_ptr, (RTP_VERSION << 6) + 1); /* 1 report block */
    buf_ptr = avio_w8(buf_ptr, RTCP_SDES);
    buf_ptr = avio_wb16(buf_ptr,(6 + len + 3) / 4); /* length in words - 1 */
    buf_ptr = avio_wb32(buf_ptr, s->ssrc + 1);
    buf_ptr = avio_w8(buf_ptr, 0x01);
    buf_ptr = avio_w8(buf_ptr, len);
    memcpy(buf_ptr,hostname,len);
    buf_ptr += len;
    // padding
    for (len = (6 + len) % 4; len % 4; len++) {
        buf_ptr = avio_w8(buf_ptr, 0);
    }

    len = buf_ptr - buf;
    if (len > 0){
        // int result;
        if(s->rtp_ctx_){
           //printf("rtp_check_and_send_back_rr() --- sending %d bytes of RR, hostname = %s\n", len,hostname);
           s->rtp_ctx_->send_data(buf,len);
        }
    }
    return 0;
}

static void rtpdemux_reset_packet_queue(DemuxContext *s){
    while (s->queue) {
        RTPPacket *next = s->queue->next;
        av_free(s->queue->buf);
        s->pool->returnNode(s->queue);//av_free(s->queue);
        s->queue = next;
    }
    s->seq       = 0;
    s->queue_len = 0;
    s->prev_ret  = 0;

    if(s->handle_h264_packet_loss){
        while (s->recv_queue) {
            RTPPacket *next = s->recv_queue->next;
            av_free(s->recv_queue->buf);
            s->pool->returnNode(s->recv_queue);//av_free(s->recv_queue);
            s->recv_queue = next;
        }
        s->recv_queue_tail = NULL;
        s->recv_queue_len = 0;
    }
}

/*H264 process, packet_loss check and frame_combination
*/
static inline int checkFramePacketNumber(DemuxContext *s){
    // RTPPacket *cur = s->recv_queue;
    RTPPacket *tail = s->recv_queue_tail;
    int count;

    if(s->recv_queue->seq <= tail->seq){
        count = tail->seq - s->recv_queue->seq + 1;
    }else{
        count = 0xffff - s->recv_queue->seq + 1;
        count += tail->seq + 1;
    }

    if(count !=  s->recv_queue_len){
        av_log(NULL, AV_LOG_WARNING,"RTP Discard--- check_frame_packets, head_seq = 0x%x, tail_seq = %x,count = %d, queue_len = %d\n",\
            s->recv_queue->seq,tail->seq,count,s->recv_queue_len);
        return -1;
    }
    return 0;
}
static inline int skip_rtp_header(const uint8_t *buf,int len, uint8_t **bufptr,int *lenptr){
    int ext = buf[0] & 0x10;
    if (buf[0] & 0x20) {
        int padding = buf[len - 1];
        if (len >= 12 + padding)
            len -= padding;
    }

    len -= 12;
    buf += 12;

    /* RFC 3550 Section 5.3.1 RTP Header Extension handling */
    if (ext) {
        if (len < 4){
            //av_log(NULL, AV_LOG_ERROR, "skip_rtp_header --- error 1\n");
            return -1;
        }
        /* calculate the header extension length (stored as number
         * of 32-bit words) */
        ext = (AV_RB16(buf + 2) + 1) << 2;

        if (len < ext){
            //av_log(NULL, AV_LOG_ERROR, "skip_rtp_header --- error 2\n");
            return -1;
        }
        // skip past RTP header extension
        len -= ext;
        buf += ext;
    }
    *bufptr = (uint8_t *)buf;
    *lenptr = len;
    return 0;
}

static int getNalType(uint8_t *buf,int len,uint8_t *type,uint8_t *fu_header){
    uint8_t *payload;
    int payload_len;
    uint8_t nal;
    if(skip_rtp_header(buf,len,&payload,&payload_len) < 0){
        //av_log(NULL, AV_LOG_ERROR, "getNalType --- skip_rtp_header error\n");
        return -1;
    }
    if(payload_len < 2){
        //av_log(NULL, AV_LOG_ERROR, "getNalType --- payload_len too smal,len =  %d\n",payload_len);
        return -1;
    }

    nal = payload[0];
    *type = (nal & 0x1f);
    if(*type == 28){//FU-A
        *fu_header = payload[1];
    }
    return 0;
}
static void check_frame_packets(DemuxContext *s){
    int discard_flag = 0;

    if(checkFramePacketNumber(s) < 0){
        discard_flag = 1;
    }else{
        RTPPacket * packet = s->recv_queue;
        RTPPacket *first = NULL,*last = NULL;
        while (packet) {
            uint8_t type,fu_header;
            if(getNalType(packet->buf,packet->len,&type,&fu_header) < 0){
                av_log(NULL, AV_LOG_WARNING,"RTP Discard--- check_frame_packets, invalid rtp packet\n");
                discard_flag = 1;
                break;
            }else{
                if(type == 28){//FU-A
                    if(!first) {
                        first = packet;
                        uint8_t start_bit =  fu_header>> 7;
                        if(!start_bit){
                            av_log(NULL, AV_LOG_WARNING,"RTP Discard--- check_frame_packets,FU-A start_bit not found\n");
                            discard_flag = 1;
                            break;
                        }
                    }
                    last = packet;
                    if(!last->next){
                        uint8_t end_bit = (fu_header & 0x40) >> 6;
                        if(!end_bit){
                            av_log(NULL, AV_LOG_WARNING,"RTP Discard--- check_frame_packets,FU-A end_bit not found\n");
                            discard_flag = 1;
                            break;
                        }
                    }
                }else{
                    //do nothing now. TODO
                }
            }
            packet = packet->next;
        }
    }

    if(discard_flag){
        while (s->recv_queue) {
            RTPPacket *next = s->recv_queue->next;
            av_free(s->recv_queue->buf);
            s->pool->returnNode(s->recv_queue);//av_free(s->recv_queue);
            s->recv_queue = next;
        }
        s->recv_queue_len = 0;
        s->recv_queue_tail = NULL;
        av_log(NULL, AV_LOG_WARNING,"CodecID %d, RTP Discard--- check_frame_packets\n",s->codec_id);
    }else{
        if(s->queue_tail){
            s->queue_tail->next = s->recv_queue;
            s->queue_tail = s->recv_queue_tail;
        }else{
            s->queue = s->recv_queue;
            s->queue_tail = s->recv_queue_tail;
        }
        s->recv_queue = NULL;
        s->recv_queue_tail = NULL;
        s->queue_len += s->recv_queue_len;
        s->recv_queue_len = 0;
        s->queue_size = 0;
    }
}


static int rtp_check_h264_packet(DemuxContext *s, const uint8_t *buf, int len)
{
    unsigned int ssrc;
    int payload_type, seq, flags = 0;
    int ext;

    ext = buf[0] & 0x10;
    payload_type = buf[1] & 0x7f;
    if (buf[1] & 0x80)
        flags |= RTP_FLAG_MARKER;
    seq  = AV_RB16(buf + 2);
    //uint32_t timestamp = AV_RB32(buf + 4);
    ssrc = AV_RB32(buf + 8);
    if(s->ssrc_flag && s->ssrc != ssrc){
        /* discard invalid rtp packets */
        //av_log(NULL, AV_LOG_ERROR, "############rtp_parse_packet_internal --- SSRC = %x, expected = %x\n",ssrc,s->ssrc);
        return -1;
    }else if(!s->ssrc_flag){
        /* store the ssrc in the RTPDemuxContext
        */
        s->ssrc = ssrc;
    }

    /* NOTE: we can handle only one payload type */
    if (s->payload_type != payload_type)
        return -1;


    // only do something with this if all the rtp checks pass...
    if(!rtp_valid_packet_in_sequence(&s->statistics, seq))
    {
        //av_log(s->st ? s->st->codec : NULL, AV_LOG_ERROR, "rtp_check_h264_packet RTP: PT=%02x: bad cseq %04x expected=%04x\n",
        //       payload_type, seq, ((s->seq + 1) & 0xffff));
        return -1;
    }

    if (buf[0] & 0x20) {
        int padding = buf[len - 1];
        if (len >= 12 + padding)
            len -= padding;
    }

    s->seq = seq;

    len -= 12;
    buf += 12;

    /* RFC 3550 Section 5.3.1 RTP Header Extension handling */
    if (ext) {
        if (len < 4)
            return -1;
        /* calculate the header extension length (stored as number
         * of 32-bit words) */
        ext = (AV_RB16(buf + 2) + 1) << 2;

        if (len < ext)
            return -1;
        // skip past RTP header extension
        len -= ext;
        buf += ext;
    }
    return 0;
}


static int rtp_add_h264_nalu(const uint8_t * buf, int len,AVPacket * pkt,int offset)
{
    uint8_t nal,type;
    static const uint8_t start_sequence[]= {0, 0, 0, 1};

    assert(buf);
    if(len < 2){
        //av_log(NULL, AV_LOG_ERROR, "rtp_add_h264_nalu --- payload_len too smal,len =  %d\n",len);
        return -1;
    }

    nal = buf[0];
    type = (nal & 0x1f);
    if (type <= 23){
        memcpy(pkt->data + offset, start_sequence, sizeof(start_sequence));
        memcpy(pkt->data + offset + sizeof(start_sequence), buf, len);

        if(type == 5){//IDR
            pkt->flags |= AV_PKT_FLAG_KEY;
        }
        return len+sizeof(start_sequence);
    }
    if(type == 28){ // FU-A (fragmented nal)
        buf++;
        len--;                  // skip the fu_indicator
        {
            // these are the same as above, we just redo them here for clarity...
            uint8_t fu_indicator = nal;
            uint8_t fu_header = *buf;   // read the fu_header.
            uint8_t start_bit = fu_header >> 7;
            //            uint8_t end_bit = (fu_header & 0x40) >> 6;
            uint8_t nal_type = (fu_header & 0x1f);
            uint8_t reconstructed_nal;

            // reconstruct this packet's true nal; only the data follows..
            reconstructed_nal = fu_indicator & (0xe0);  // the original nal forbidden bit and NRI are stored in this packet's nal;
            reconstructed_nal |= nal_type;

            // skip the fu_header...
            buf++;
            len--;

            if(start_bit) {
                // copy in the start sequence, and the reconstructed nal....
                memcpy(pkt->data + offset, start_sequence, sizeof(start_sequence));
                pkt->data[offset + sizeof(start_sequence)]= reconstructed_nal;
                if(nal_type == 5){//IDR
                    pkt->flags |= AV_PKT_FLAG_KEY;
                }
                memcpy(pkt->data+ offset + sizeof(start_sequence)+sizeof(nal), buf, len);
                return sizeof(start_sequence)+sizeof(nal)+len;
            } else {
                memcpy(pkt->data + offset, buf, len);
                return len;
            }
        }
    }

    if(type == 24){
        // consume the STAP-A NAL
        buf++;
        len--;
        {
            int total_length= 0;
            const uint8_t *src= buf;
            int src_len= len;

            while (src_len > 2) {
                uint16_t nal_size = AV_RB16(src); // this going to be a problem if unaligned (can it be?)
                // consume the length of the aggregate...
                src += 2;
                src_len -= 2;

                if (nal_size <= src_len) {
                    // copying
                    memcpy(pkt->data + offset, start_sequence, sizeof(start_sequence));
                    memcpy(pkt->data+ offset + sizeof(start_sequence), src, nal_size);
                    total_length += sizeof(start_sequence) + nal_size;
                }

                // eat what we handled...
                src += nal_size;
                src_len -= nal_size;
            }
            return total_length;
        }
    }
    //av_log(NULL, AV_LOG_ERROR, "Unimplemented type or Undefined type (%d)", type);
    return -1;
}

static inline int rtp_h264_add_packet(const uint8_t * buf, int len,AVPacket *pkt,int offset)
{
    uint8_t *payload;
    int payload_len;
    if(skip_rtp_header(buf,len,&payload,&payload_len) < 0){
        //av_log(NULL, AV_LOG_ERROR, "rtp_h264_add_packet --- skip_rtp_header error\n");
        return -1;
    }
    return rtp_add_h264_nalu(payload,payload_len,pkt,offset);
}

static void discard_frame_data(DemuxContext *s){
    RTPPacket *packet = s->queue,*next;
    packet = s->queue;
    while(packet){
        next = packet->next;
        av_free(packet->buf);
        s->pool->returnNode(packet);//av_free(packet);
        packet = next;
    }
    s->queue = NULL;
    s->queue_tail = NULL;
    s->queue_len = 0;
}
static int rtp_parse_queued_frame_h264(DemuxContext *s, AVPacket *pkt)
{
    int total_len = 0,offset;
    RTPPacket *packet = s->queue,*next;

    if(!packet){
        return -1;
    }

    //estimate total frame length, to alloc buffer
    while(packet){
        total_len += packet->len;
        packet = packet->next;
    }

    if(av_new_packet2(pkt,total_len) < 0){
        av_log(NULL, AV_LOG_ERROR, "rtp_parse_queued_frame_h264, av_new_packet failed,TODO\n");
        discard_frame_data(s);
        return -1;
    }
    pkt->flags &= ~AV_PKT_FLAG_KEY;

    packet = s->queue;
    offset = 0;
    while(packet){
        next = packet->next;
        offset += rtp_h264_add_packet(packet->buf,packet->len,pkt,offset);
        av_free(packet->buf);
        s->pool->returnNode(packet);//av_free(packet);
        packet = next;
    }
    s->queue = NULL;
    s->queue_tail = NULL;
    s->queue_len = 0;
    pkt->size = offset;//fix frame length

    // now perform timestamp things....
    pkt->stream_index = s->stream_index;
    finalize_packet(s, pkt, s->recv_queue_timestamp);
    return 0;
}


/* For Project NVR,
*     discard frame-packets if packet loss occured.
*     combine packets to frame.
*     Reorder has not been considered.  TODO
*/

static int handle_recv_h264_packet(DemuxContext *s,AVPacket *pkt, uint8_t **buf_, int len)
{
    uint8_t *buf = *buf_;
    uint32_t timestamp = AV_RB32(buf + 4);
    int rv = -1;

    if(s->recv_queue && (s->recv_queue_timestamp != timestamp)){
        check_frame_packets(s);
        rv = rtp_parse_queued_frame_h264(s, pkt);
    }

    //parse and discard invalid rtp packet here according to RFC
    if(rtp_check_h264_packet(s,buf,len) < 0){
        return rv;
    }

    //send back rr here
    rtp_check_and_send_back_rr(s,len);


    RTPPacket *packet = (RTPPacket *)s->pool->getNode();//av_mallocz(sizeof(*packet));
    if (packet){
        //packet->recvtime = av_gettime();
        uint16_t seq = AV_RB16(buf + 2);
        packet->seq = seq;
        packet->len = len;
        packet->buf = buf;
        packet->next = NULL;
        if (s->recv_queue_tail){
            s->recv_queue_tail->next = packet;
            s->recv_queue_tail = packet;
        }else{
            s->recv_queue = packet;
            s->recv_queue_timestamp = timestamp;
            s->recv_queue_tail = s->recv_queue;
        }
        s->recv_queue_len++;
        *buf_ = NULL;
    }
    return rv;
}

static int rtp_parse_packet_internal(DemuxContext *s,AVPacket *pkt,const uint8_t *buf, int len){
    unsigned int ssrc;
    int payload_type, seq, flags = 0;
    int ext;
    //AVStream *st;
    uint32_t timestamp;
    int rv= 0;
    int count = len;

    ext = buf[0] & 0x10;
    payload_type = buf[1] & 0x7f;
    if (buf[1] & 0x80)
        flags |= RTP_FLAG_MARKER;
    seq  = AV_RB16(buf + 2);
    timestamp = AV_RB32(buf + 4);
    ssrc = AV_RB32(buf + 8);
    if(s->ssrc_flag && s->ssrc != ssrc){
        /* discard invalid rtp packets */
        //av_log(NULL, AV_LOG_ERROR, "############rtp_parse_packet_internal --- SSRC = %x, expected = %x\n",ssrc,s->ssrc);
        return -1;
    }else if(!s->ssrc_flag){
        /* store the ssrc in the RTPDemuxContext
        */
        s->ssrc = ssrc;
    }

    /* NOTE: we can handle only one payload type */
    if (s->payload_type != payload_type)
        return -1;

    //st = s->st;
    // only do something with this if all the rtp checks pass...
    if(!rtp_valid_packet_in_sequence(&s->statistics, seq))
    {
        //av_log(st?st->codec:NULL, AV_LOG_ERROR, "RTP: PT=%02x: bad cseq %04x expected=%04x\n", payload_type, seq, ((s->seq + 1) & 0xffff));
        return -1;
    }

    if (buf[0] & 0x20) {
        int padding = buf[len - 1];
        if (len >= 12 + padding)
            len -= padding;
    }

    s->seq = seq;

    len -= 12;
    buf += 12;

    /* RFC 3550 Section 5.3.1 RTP Header Extension handling */
    if (ext) {
        if (len < 4)
            return -1;
        /* calculate the header extension length (stored as number
         * of 32-bit words) */
        ext = (AV_RB16(buf + 2) + 1) << 2;

        if (len < ext)
            return -1;
        // skip past RTP header extension
        len -= ext;
        buf += ext;
    }

    switch(s->codec_id){
    case CODEC_ID_AAC:
        rv = aac_parse_packet((PayloadContext *)s->dynamic_protocol_context,s->stream_index,pkt,buf, len);
        break;
    default: //G711 muLaw/aLaw
        av_new_packet2(pkt, len);
        memcpy(pkt->data, buf, len);
        pkt->stream_index = s->stream_index;
        break;
    }

    // now perform timestamp things....
    finalize_packet(s,pkt, timestamp);
    rtp_check_and_send_back_rr(s,count);
    return rv;
}

static int rtcp_parse_packet(DemuxContext *s,const unsigned char *buf, int len){
    int payload_len;
    while (len >= 4) {
        payload_len = FFMIN(len, (AV_RB16(buf + 2) + 1) * 4);

        switch (buf[1]) {
        case RTCP_SR:
            if (payload_len < 20) {
                //av_log(NULL, AV_LOG_ERROR, "Invalid length for RTCP SR packet\n");
                return AVERROR_INVALIDDATA;
            }
            s->last_rtcp_ntp_time = AV_RB64(buf + 8);
            s->last_rtcp_timestamp = AV_RB32(buf + 16);
            if (s->first_rtcp_ntp_time == (int64_t)AV_NOPTS_VALUE) {
                s->first_rtcp_ntp_time = s->last_rtcp_ntp_time;
                if (!s->base_timestamp)
                    s->base_timestamp = s->last_rtcp_timestamp;
                s->rtcp_ts_offset = s->last_rtcp_timestamp - s->base_timestamp;
            }
            //printf("rtcp_parse_packet RTSP_SR recevied, rtp_ctx = %p\n",s->rtp_ctx_);
            break;
        case RTCP_BYE:
            return -RTCP_BYE;
        }

        buf += payload_len;
        len -= payload_len;
    }
    return -1;
}


static void reset_rtp_demuxer(DemuxContext *s){
    rtpdemux_reset_packet_queue(s);
    s->last_rtcp_ntp_time  = AV_NOPTS_VALUE;
    s->first_rtcp_ntp_time = AV_NOPTS_VALUE;
    s->base_timestamp      = 0;
    s->timestamp           = 0;
    s->unwrapped_timestamp = 0;
    s->rtcp_ts_offset      = 0;
}

static int do_rtp_parse_packet(DemuxContext *s,AVPacket *pkt,uint8_t **buf_,int len){
    if (len < 12){
        return -1;
    }

    uint8_t *buf = *buf_;
    if ((buf[0] & 0xc0) != (RTP_VERSION << 6)){
        return -1;
    }
    if (buf[1] >= RTCP_SR && buf[1] <= RTCP_APP) {
        return rtcp_parse_packet(s,buf, len);
    }
    if(s->handle_h264_packet_loss) {
        return handle_recv_h264_packet(s,pkt,buf_,len);
    }
    return rtp_parse_packet_internal(s,pkt, buf, len);
}

class RtpDemuxService:public AO_Proxy{
public:
    RtpDemuxService(int cpu_id){
        pthread_mutex_init(&mutex_,NULL);
        ao_start_service(256,1,cpu_id);
    }
    ~RtpDemuxService(){
        ao_exit_service();
        pthread_mutex_destroy(&mutex_);
    }
    int addDemux(DemuxContext *demux,IRtpSession *rtp_ctx,FormatContext *info){
        class RtpParseOpen:public Method_Request{
        public:
            RtpParseOpen(RtpDemuxService *manager,DemuxContext *demux_,IRtpSession *rtp_ctx_,FormatContext *info_,Future<int> *future)
                :manager_(manager),demux(demux_),rtp_ctx(rtp_ctx_),info(info_),future_(future){
            }
            virtual bool guard(){
                return true;
            }
            virtual int call(){
                DemuxContext *s = demux;
                memset(s,0,sizeof(DemuxContext));
                s->rtp_ctx_ = rtp_ctx;
                s->payload_type= info->payload_type;
                s->codec_id = info->codec_id;
                s->stream_index = info->stream_index;
                s->time_base = info->time_base;
                s->last_rtcp_ntp_time = AV_NOPTS_VALUE;
                s->first_rtcp_ntp_time = AV_NOPTS_VALUE;
                rtp_init_statistics(&s->statistics, 0); // do we know the initial sequence from sdp?
                /**/
                reset_rtp_demuxer(s);
                /**/
               if(s->codec_id == CODEC_ID_H264){
                   s->pool = new RTPPacketPool();
                   s->handle_h264_packet_loss = 1;
               }else if (s->codec_id == CODEC_ID_AAC){
                   s->dynamic_protocol_context = new_context();
                   //TODO, hardcode here now
                   //aac_parse_fmtp(PayloadContext *data, char *attr, char *value)
                   //a=fmtp:98 streamtype=5; profile-level-id=41; mode=AAC-hbr; config=1188; SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1;
                   PayloadContext *payload_context = (PayloadContext*)s->dynamic_protocol_context;
                   payload_context->sizelength = 13;
                   payload_context->indexlength = 3;
                   payload_context->indexdeltalength = 3;
                   payload_context->profile_level_id = 41;
                   payload_context->streamtype = 5;
                   payload_context->mode = av_strdup("AAC-hbr");
               }
               int result = 0;
               future_->set(result);
               return 0;
            }
        private:
            RtpDemuxService *manager_;
            DemuxContext *demux;
            IRtpSession *rtp_ctx;
            FormatContext *info;
            Future<int> *future_;
        };

        pthread_mutex_lock(&mutex_);
        Future<int> *future = new Future<int>;
        ao_send_request(new RtpParseOpen(this,demux,rtp_ctx,info,future));
        future->get();
        delete future;
        RtpPackets *packets = new RtpPackets();
        mapDemuxPackets[demux] = packets;
        pthread_mutex_unlock(&mutex_);
        return 0;
    }
    int removeDemux(DemuxContext *demux){
        class RtpParseClose:public Method_Request{
        public:
            RtpParseClose(RtpDemuxService *manager,DemuxContext *demux_,Future<int> *future)
                :manager_(manager),demux(demux_),future_(future){}
            virtual bool guard(){
                return true;
            }
            virtual int call(){
                DemuxContext *s = demux;
                if(s->dynamic_protocol_context){
                    free_context((PayloadContext *)s->dynamic_protocol_context);
                    s->dynamic_protocol_context = NULL;
                }
                rtpdemux_reset_packet_queue(s);
                if(s->pool){
                    delete s->pool;
                    s->pool = NULL;
                }
                int result = 0;
                future_->set(result);
                return 0;
            }
        private:
            RtpDemuxService *manager_;
            DemuxContext *demux;
            Future<int> *future_;
        };

        pthread_mutex_lock(&mutex_);
        Future<int> *future = new Future<int>;
        ao_send_request(new RtpParseClose(this,demux,future));
        future->get();
        delete future;
        if(mapDemuxPackets.find(demux)!=mapDemuxPackets.end()){
              RtpPackets *packets = mapDemuxPackets[demux];
              mapDemuxPackets.erase(demux);
              delete packets;
        }
        pthread_mutex_unlock(&mutex_);
        return 0;
    }
    int do_demux(DemuxContext *demux,IRtpCallback *cb,uint8_t **buf,int len){
        class RtpParsePacket:public Method_Request{
        public:
            RtpParsePacket(RtpDemuxService *manager,RtpPackets *packets)
                :manager_(manager){
                packets_ = *packets;
                packets->packet_num_ = 0;
            }
            virtual bool guard(){
                return true;
            }
            virtual int call(){
                for(int i = 0; i < packets_.packet_num_; i++){
                    AVPacket avpacket,*pkt = &avpacket;
                    PacketNode *node = &packets_.packets_[i];
                    if(do_rtp_parse_packet(node->demux_,pkt,&node->buf_,node->len_) < 0){
                        //do nothing
                    }else{
                        if(node->cb_){
                            node->cb_->on_packet(pkt);
                        }else{
                            av_free_packet(pkt);
                        }
                    }
                    if(node->buf_){
                        av_free(node->buf_);
                    }
                }
                //all packets have been processed.
                packets_.packet_num_ = 0;
                return 0;
            }
        private:
            RtpDemuxService *manager_;
            RtpPackets packets_;
        };

        pthread_mutex_lock(&mutex_);
        if(mapDemuxPackets.find(demux)!=mapDemuxPackets.end()){
            int max_packet_num = (demux->codec_id == CODEC_ID_H264) ? MAX_PACKET_NUM:1;
            RtpPackets *packets = mapDemuxPackets[demux];
            PacketNode *node = &packets->packets_[packets->packet_num_++];
            node->buf_ = *buf; *buf = NULL;
            node->len_ = len;
            node->cb_ = cb;
            node->demux_ = demux;
            if(packets->packet_num_ >= max_packet_num){
               ao_send_request(new RtpParsePacket(this,packets));
            }
        }
        pthread_mutex_unlock(&mutex_);
        return 0;
    }
private:
    enum {MAX_PACKET_NUM = 4};
    struct PacketNode{
        DemuxContext *demux_;
        IRtpCallback *cb_;
        uint8_t *buf_;
        int len_;
    };

    struct RtpPackets{
        PacketNode packets_[MAX_PACKET_NUM];
        int packet_num_;
        RtpPackets(){
            memset(packets_,0,sizeof(PacketNode) * MAX_PACKET_NUM);
            packet_num_ = 0;
        }
        ~RtpPackets(){
            for(int i = 0; i < packet_num_; i++){
                 PacketNode *node = &packets_[i];
                 av_free(node->buf_);
            }
        }
    };
    pthread_mutex_t mutex_;
    std::map< DemuxContext*,RtpPackets *> mapDemuxPackets;
};

class RtpDemuxManager:public AO_Proxy{
public:
    static RtpDemuxManager * instance();
    void start_service(int thread_num){
         pthread_mutex_lock(&mutex_);
         if(!is_start){
             service_num_ =  thread_num;
             while(thread_num-->0){
                 RtpDemuxService * service=new RtpDemuxService(thread_num % 2);
                 vecService_.push_back(service);
             }
             ao_start_service();
             is_start = 1;
         }
         pthread_mutex_unlock(&mutex_);
    }
    void stop_service(){
        pthread_mutex_lock(&mutex_);
        if(is_start){
            ao_exit_service();
            while(!vecService_.empty()){
                RtpDemuxService * service=*(vecService_.begin());
                vecService_.erase(vecService_.begin());
                delete service;
            }
            is_start = 0;
        }
        pthread_mutex_unlock(&mutex_);
        delete this;
    }
    RtpDemuxService *addDemux(DemuxContext *demux,IRtpSession *rtp_ctx,FormatContext *info){
        RtpDemuxService *service;
        pthread_mutex_lock(&mutex_);
        int num=(service_index++)%service_num_;
        mapDemuxIndex[demux] = num;
        service = vecService_[num];
        service->addDemux(demux,rtp_ctx,info);
        pthread_mutex_unlock(&mutex_);
        return service;
    }
    int removeDemux(DemuxContext *demux){
         int ret = 0;
         pthread_mutex_lock(&mutex_);
         if(mapDemuxIndex.find(demux)!=mapDemuxIndex.end()){
              int num = mapDemuxIndex[demux];
              mapDemuxIndex.erase(demux);
              ret =  vecService_[num]->removeDemux(demux);
         }
         pthread_mutex_unlock(&mutex_);
         return ret;
    }
    int do_demux(RtpDemuxService *service,DemuxContext *demux,IRtpCallback *cb,uint8_t **buf,int len){
        service->do_demux(demux,cb,buf,len);
        return 0;
    }
private:
    RtpDemuxManager():is_start(0),service_index(0),service_num_(0){
    }
    ~RtpDemuxManager(){
        instance_ = NULL;
    }
    static RtpDemuxManager * instance_;
    static pthread_mutex_t mutex_;
    int is_start;
    int service_index;
    int service_num_;
    std::vector<RtpDemuxService *> vecService_;
    std::map<DemuxContext *,int> mapDemuxIndex;
};

RtpDemuxManager * RtpDemuxManager::instance_=NULL;
pthread_mutex_t RtpDemuxManager::mutex_= PTHREAD_MUTEX_INITIALIZER;

RtpDemuxManager * RtpDemuxManager::instance(){
    if(instance_==NULL){
        pthread_mutex_lock(&mutex_);
        if(instance_==NULL){
            instance_=new RtpDemuxManager();
        }
        pthread_mutex_unlock(&mutex_);
    }
    return instance_;
}

//
//RTPDemuxer implementation
//
RTPDemuxer::RTPDemuxer():service(NULL),state_(DEMUXER_STOP){
    //memset(&context_,0,sizeof(context_));
}
RTPDemuxer::~RTPDemuxer(){
}

int RTPDemuxer::start_service(int num){
    RtpDemuxManager::instance()->start_service(num);
    return 0;
}
int RTPDemuxer::stop_service(){
    RtpDemuxManager::instance()->stop_service();
    return 0;
}

int RTPDemuxer::rtp_parse_open(IRtpSession *rtp_ctx,FormatContext *info){
    RtpDemuxService *service_ = RtpDemuxManager::instance()->addDemux(&context_,rtp_ctx,info);
    service = (void*)service_;
    return 0;
}

int RTPDemuxer::rtp_parse_packet(IRtpCallback *cb,uint8_t **buf,int len){
    RtpDemuxService *service_ = (RtpDemuxService *)service;
    RtpDemuxManager::instance()->do_demux(service_,&context_, cb,buf,len);
    return 0;
}

void RTPDemuxer::rtp_parse_close(){
    RtpDemuxManager::instance()->removeDemux(&context_);
}

#endif //NEW_RTSP_CLIENT

