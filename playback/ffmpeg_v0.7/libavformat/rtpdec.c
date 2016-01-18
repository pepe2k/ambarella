/*
 * RTP input format
 * Copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* needed for gethostname() */
#define _XOPEN_SOURCE 600

#include "libavcodec/get_bits.h"
#include "avformat.h"
#include "mpegts.h"
#include "url.h"

#include <unistd.h>
#include <strings.h>
#include "network.h"

#include "rtpdec.h"
#include "rtpdec_formats.h"

#include "sys/time.h"

//#define DEBUG

/* TODO: - add RTCP statistics reporting (should be optional).

         - add support for h263/mpeg4 packetized output : IDEA: send a
         buffer to 'rtp_write_packet' contains all the packets for ONE
         frame. Each packet should have a four byte header containing
         the length in big endian format (same trick as
         'ffio_open_dyn_packet_buf')
*/

static RTPDynamicProtocolHandler ff_realmedia_mp3_dynamic_handler = {
    .enc_name           = "X-MP3-draft-00",
    .codec_type         = AVMEDIA_TYPE_AUDIO,
    .codec_id           = CODEC_ID_MP3ADU,
};

/* statistics functions */
static RTPDynamicProtocolHandler *RTPFirstDynamicPayloadHandler= NULL;

void ff_register_dynamic_payload_handler(RTPDynamicProtocolHandler *handler)
{
    handler->next= RTPFirstDynamicPayloadHandler;
    RTPFirstDynamicPayloadHandler= handler;
}

void av_register_rtp_dynamic_payload_handlers(void)
{
    ff_register_dynamic_payload_handler(&ff_mp4v_es_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_mpeg4_generic_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_amr_nb_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_amr_wb_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_h263_1998_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_h263_2000_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_h264_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_vorbis_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_theora_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_qdm2_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_svq3_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_mp4a_latm_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_vp8_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_qcelp_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_realmedia_mp3_dynamic_handler);

    ff_register_dynamic_payload_handler(&ff_ms_rtp_asf_pfv_handler);
    ff_register_dynamic_payload_handler(&ff_ms_rtp_asf_pfa_handler);

    ff_register_dynamic_payload_handler(&ff_qt_rtp_aud_handler);
    ff_register_dynamic_payload_handler(&ff_qt_rtp_vid_handler);
    ff_register_dynamic_payload_handler(&ff_quicktime_rtp_aud_handler);
    ff_register_dynamic_payload_handler(&ff_quicktime_rtp_vid_handler);
}

RTPDynamicProtocolHandler *ff_rtp_handler_find_by_name(const char *name,
                                                  enum AVMediaType codec_type)
{
    RTPDynamicProtocolHandler *handler;
    for (handler = RTPFirstDynamicPayloadHandler;
         handler; handler = handler->next)
        if (!strcasecmp(name, handler->enc_name) &&
            codec_type == handler->codec_type)
            return handler;
    return NULL;
}

RTPDynamicProtocolHandler *ff_rtp_handler_find_by_id(int id,
                                                enum AVMediaType codec_type)
{
    RTPDynamicProtocolHandler *handler;
    for (handler = RTPFirstDynamicPayloadHandler;
         handler; handler = handler->next)
        if (handler->static_payload_id && handler->static_payload_id == id &&
            codec_type == handler->codec_type)
            return handler;
    return NULL;
}

static int rtcp_parse_packet(RTPDemuxContext *s, const unsigned char *buf, int len)
{
    int payload_len;
    while (len >= 4) {
        payload_len = FFMIN(len, (AV_RB16(buf + 2) + 1) * 4);

        switch (buf[1]) {
        case RTCP_SR:
            if (payload_len < 20) {
                av_log(NULL, AV_LOG_ERROR, "Invalid length for RTCP SR packet\n");
                return AVERROR_INVALIDDATA;
            }
            s->last_rtcp_ntp_time = AV_RB64(buf + 8);
            s->last_rtcp_timestamp = AV_RB32(buf + 16);
            if (s->first_rtcp_ntp_time == AV_NOPTS_VALUE) {
                s->first_rtcp_ntp_time = s->last_rtcp_ntp_time;
                if (!s->base_timestamp)
                    s->base_timestamp = s->last_rtcp_timestamp;
                s->rtcp_ts_offset = s->last_rtcp_timestamp - s->base_timestamp;
            }

            break;
        case RTCP_BYE:
            return -RTCP_BYE;
        }

        buf += payload_len;
        len -= payload_len;
    }
    return -1;
}

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

#if 0
/**
* This function is currently unused; without a valid local ntp time, I don't see how we could calculate the
* difference between the arrival and sent timestamp.  As a result, the jitter and transit statistics values
* never change.  I left this in in case someone else can see a way. (rdm)
*/
static void rtcp_update_jitter(RTPStatistics *s, uint32_t sent_timestamp, uint32_t arrival_timestamp)
{
    uint32_t transit= arrival_timestamp - sent_timestamp;
    int d;
    s->transit= transit;
    d= FFABS(transit - s->transit);
    s->jitter += d - ((s->jitter + 8)>>4);
}
#endif

static int do_rtp_check_and_send_back_rr(RTPDemuxContext *s, int count)
{
    AVIOContext *pb;
    uint8_t *buf;
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
    uint64_t ntp_time= s->last_rtcp_ntp_time; // TODO: Get local ntp time?

    if (!s->rtp_ctx || (count < 1))
        return -1;

    /* TODO: I think this is way too often; RFC 1889 has algorithm for this */
    /* XXX: mpeg pts hardcoded. RTCP send every 0.5 seconds */
    s->octet_count += count;
    rtcp_bytes = ((s->octet_count - s->last_octet_count) * RTCP_TX_RATIO_NUM) /
        RTCP_TX_RATIO_DEN;
    rtcp_bytes /= 50; // mmu_man: that's enough for me... VLC sends much less btw !?
    if (rtcp_bytes < 28)
        return -1;
    s->last_octet_count = s->octet_count;

    if (avio_open_dyn_buf(&pb) < 0)
        return -1;

    // Receiver Report
    avio_w8(pb, (RTP_VERSION << 6) + 1); /* 1 report block */
    avio_w8(pb, RTCP_RR);
    avio_wb16(pb, 7); /* length in words - 1 */
    // our own SSRC: we use the server's SSRC + 1 to avoid conflicts
    avio_wb32(pb, s->ssrc + 1);
    avio_wb32(pb, s->ssrc); // server SSRC
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

    avio_wb32(pb, fraction); /* 8 bits of fraction, 24 bits of total packets lost */
    avio_wb32(pb, extended_max); /* max sequence received */
    avio_wb32(pb, stats->jitter>>4); /* jitter */

    if(s->last_rtcp_ntp_time==AV_NOPTS_VALUE)
    {
        avio_wb32(pb, 0); /* last SR timestamp */
        avio_wb32(pb, 0); /* delay since last SR */
    } else {
        uint32_t middle_32_bits= s->last_rtcp_ntp_time>>16; // this is valid, right? do we need to handle 64 bit values special?
        uint32_t delay_since_last= ntp_time - s->last_rtcp_ntp_time;

        avio_wb32(pb, middle_32_bits); /* last SR timestamp */
        avio_wb32(pb, delay_since_last); /* delay since last SR */
    }

    // CNAME
    avio_w8(pb, (RTP_VERSION << 6) + 1); /* 1 report block */
    avio_w8(pb, RTCP_SDES);
    len = strlen(s->hostname);
    avio_wb16(pb, (6 + len + 3) / 4); /* length in words - 1 */
    avio_wb32(pb, s->ssrc + 1);
    avio_w8(pb, 0x01);
    avio_w8(pb, len);
    avio_write(pb, s->hostname, len);
    // padding
    for (len = (6 + len) % 4; len % 4; len++) {
        avio_w8(pb, 0);
    }

    avio_flush(pb);
    len = avio_close_dyn_buf(pb, &buf);
    if ((len > 0) && buf) {
        int result;
        av_dlog(s->ic, "sending %d bytes of RR\n", len);
        result= ffurl_write(s->rtp_ctx, buf, len);
        av_dlog(s->ic, "result from ffurl_write: %d\n", result);
        av_log(NULL, AV_LOG_DEBUG, "result from ffurl_write: %d\n", result);
        av_free(buf);
    }
    return 0;
}

int rtp_check_and_send_back_rr(RTPDemuxContext *s, int count)
{
#if PROJECT_NVR
    if(s->st && s->st->codec->codec_id == CODEC_ID_H264){
        return 0;
    }
#endif
    return do_rtp_check_and_send_back_rr(s,count);
}

void rtp_send_punch_packets(URLContext* rtp_handle)
{
    AVIOContext *pb;
    uint8_t *buf;
    int len;

    /* Send a small RTP packet */
    if (avio_open_dyn_buf(&pb) < 0)
        return;

    avio_w8(pb, (RTP_VERSION << 6));
    avio_w8(pb, 0); /* Payload type */
    avio_wb16(pb, 0); /* Seq */
    avio_wb32(pb, 0); /* Timestamp */
    avio_wb32(pb, 0); /* SSRC */

    avio_flush(pb);
    len = avio_close_dyn_buf(pb, &buf);
    if ((len > 0) && buf)
        ffurl_write(rtp_handle, buf, len);
    av_free(buf);

    /* Send a minimal RTCP RR */
    if (avio_open_dyn_buf(&pb) < 0)
        return;

    avio_w8(pb, (RTP_VERSION << 6));
    avio_w8(pb, RTCP_RR); /* receiver report */
    avio_wb16(pb, 1); /* length in words - 1 */
    avio_wb32(pb, 0); /* our own SSRC */

    avio_flush(pb);
    len = avio_close_dyn_buf(pb, &buf);
    if ((len > 0) && buf)
        ffurl_write(rtp_handle, buf, len);
    av_free(buf);
}


/**
 * open a new RTP parse context for stream 'st'. 'st' can be NULL for
 * MPEG2TS streams to indicate that they should be demuxed inside the
 * rtp demux (otherwise CODEC_ID_MPEG2TS packets are returned)
 */
RTPDemuxContext *rtp_parse_open(AVFormatContext *s1, AVStream *st, URLContext *rtpc, int payload_type, int queue_size)
{
    RTPDemuxContext *s;

    s = av_mallocz(sizeof(RTPDemuxContext));
    if (!s)
        return NULL;
    s->payload_type = payload_type;
    s->last_rtcp_ntp_time = AV_NOPTS_VALUE;
    s->first_rtcp_ntp_time = AV_NOPTS_VALUE;
    s->ic = s1;
    s->st = st;
    s->queue_size = queue_size;
    rtp_init_statistics(&s->statistics, 0); // do we know the initial sequence from sdp?
    if (!strcmp(ff_rtp_enc_name(payload_type), "MP2T")) {
        s->ts = ff_mpegts_parse_open(s->ic);
        if (s->ts == NULL) {
            av_free(s);
            return NULL;
        }
    } else {
        switch(st->codec->codec_id) {
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
        case CODEC_ID_MP2:
        case CODEC_ID_MP3:
        case CODEC_ID_MPEG4:
        case CODEC_ID_H263:
        case CODEC_ID_H264:
            st->need_parsing = AVSTREAM_PARSE_FULL;
            break;
        case CODEC_ID_ADPCM_G722:
            /* According to RFC 3551, the stream clock rate is 8000
             * even if the sample rate is 16000. */
            if (st->codec->sample_rate == 8000)
                st->codec->sample_rate = 16000;
            break;
        default:
            break;
        }
    }
    // needed to send back RTCP RR in RTSP sessions
    s->rtp_ctx = rtpc;
    gethostname(s->hostname, sizeof(s->hostname));
    return s;
}

void
rtp_parse_set_dynamic_protocol(RTPDemuxContext *s, PayloadContext *ctx,
                               RTPDynamicProtocolHandler *handler)
{
    s->dynamic_protocol_context = ctx;
    s->parse_packet = handler->parse_packet;
}

#if PROJECT_NVR
/*called after rtp_parse_open()
*/
void rtp_parse_handle_packet_loss(RTPDemuxContext *s, int flags)
{
    if(s->st){
        if(s->st->codec->codec_id == CODEC_ID_H264){
            s->handle_h264_packet_loss = 1;
            s->handle_h264_combine_frame = 1;
            if(s->handle_h264_combine_frame){
                s->st->need_parsing = AVSTREAM_PARSE_HEADERS;
            }
        }
    }
}
#endif

/**
 * This was the second switch in rtp_parse packet.  Normalizes time, if required, sets stream_index, etc.
 */
static void finalize_packet(RTPDemuxContext *s, AVPacket *pkt, uint32_t timestamp)
{
    if (pkt->pts != AV_NOPTS_VALUE || pkt->dts != AV_NOPTS_VALUE)
        return; /* Timestamp already set by depacketizer */
    if (timestamp == RTP_NOTS_VALUE)
        return;

    if (s->last_rtcp_ntp_time != AV_NOPTS_VALUE && s->ic->nb_streams > 1) {
        int64_t addend;
        int delta_timestamp;

        /* compute pts from timestamp with received ntp_time */
        delta_timestamp = timestamp - s->last_rtcp_timestamp;
        /* convert to the PTS timebase */
        addend = av_rescale(s->last_rtcp_ntp_time - s->first_rtcp_ntp_time, s->st->time_base.den, (uint64_t)s->st->time_base.num << 32);
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

#if PROJECT_NVR
static int rtp_parse_packet_internal_h264(RTPDemuxContext *s, AVPacket *pkt,
                                     const uint8_t *buf, int len)
{
    int  flags = 0;
    int ext,seq;
    uint32_t timestamp;
    int rv= 0;

    ext = buf[0] & 0x10;
    if (buf[1] & 0x80)  flags |= RTP_FLAG_MARKER;
    seq  = AV_RB16(buf + 2);
    timestamp = AV_RB32(buf + 4);

    if (buf[0] & 0x20) {
        int padding = buf[len - 1];
        if (len >= 12 + padding)
            len -= padding;
    }
    s->h264_parsed_seq = seq;
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

    if (s->parse_packet) {
        rv = s->parse_packet(s->ic, s->dynamic_protocol_context,
                             s->st, pkt, &timestamp, buf, len, flags);
    }
    // now perform timestamp things....
    finalize_packet(s, pkt, timestamp);
    return rv;
}
#endif

static int rtp_parse_packet_internal(RTPDemuxContext *s, AVPacket *pkt,
                                     const uint8_t *buf, int len)
{
    unsigned int ssrc, h;
    int payload_type, seq, ret, flags = 0;
    int ext;
    AVStream *st;
    uint32_t timestamp;
    int rv= 0;

#if PROJECT_NVR
    if(s->handle_h264_packet_loss) {
        return rtp_parse_packet_internal_h264(s,pkt,buf,len);
    }
#endif

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

    st = s->st;
    // only do something with this if all the rtp checks pass...
    if(!rtp_valid_packet_in_sequence(&s->statistics, seq))
    {
        av_log(st?st->codec:NULL, AV_LOG_ERROR, "RTP: PT=%02x: bad cseq %04x expected=%04x\n",
               payload_type, seq, ((s->seq + 1) & 0xffff));
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

    if (!st) {
        /* specific MPEG2TS demux support */
        ret = ff_mpegts_parse_packet(s->ts, pkt, buf, len);
        /* The only error that can be returned from ff_mpegts_parse_packet
         * is "no more data to return from the provided buffer", so return
         * AVERROR(EAGAIN) for all errors */
        if (ret < 0)
            return AVERROR(EAGAIN);
        if (ret < len) {
            s->read_buf_size = len - ret;
            memcpy(s->buf, buf + ret, s->read_buf_size);
            s->read_buf_index = 0;
            return 1;
        }
        return 0;
    } else if (s->parse_packet) {
        rv = s->parse_packet(s->ic, s->dynamic_protocol_context,
                             s->st, pkt, &timestamp, buf, len, flags);
    } else {
        // at this point, the RTP header has been stripped;  This is ASSUMING that there is only 1 CSRC, which in't wise.
        switch(st->codec->codec_id) {
        case CODEC_ID_MP2:
        case CODEC_ID_MP3:
            /* better than nothing: skip mpeg audio RTP header */
            if (len <= 4)
                return -1;
            h = AV_RB32(buf);
            len -= 4;
            buf += 4;
            av_new_packet(pkt, len);
            memcpy(pkt->data, buf, len);
            break;
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
            /* better than nothing: skip mpeg video RTP header */
            if (len <= 4)
                return -1;
            h = AV_RB32(buf);
            buf += 4;
            len -= 4;
            if (h & (1 << 26)) {
                /* mpeg2 */
                if (len <= 4)
                    return -1;
                buf += 4;
                len -= 4;
            }
            av_new_packet(pkt, len);
            memcpy(pkt->data, buf, len);
            break;
        default:
            av_new_packet(pkt, len);
            memcpy(pkt->data, buf, len);
            break;
        }

        pkt->stream_index = st->index;
    }

    // now perform timestamp things....
    finalize_packet(s, pkt, timestamp);

    return rv;
}

void ff_rtp_reset_packet_queue(RTPDemuxContext *s)
{
    while (s->queue) {
        RTPPacket *next = s->queue->next;
        av_free(s->queue->buf);
        av_free(s->queue);
        s->queue = next;
    }
    s->seq       = 0;
    s->queue_len = 0;
    s->prev_ret  = 0;

#if PROJECT_NVR
    if(s->handle_h264_packet_loss){
        while (s->recv_queue) {
            RTPPacket *next = s->recv_queue->next;
            av_free(s->recv_queue->buf);
            av_free(s->recv_queue);
            s->recv_queue = next;
        }
        s->recv_queue_tail = NULL;
        s->recv_queue_len = 0;
    }
#endif
}

static void enqueue_packet(RTPDemuxContext *s, uint8_t *buf, int len)
{
    uint16_t seq = AV_RB16(buf + 2);
    RTPPacket *cur = s->queue, *prev = NULL, *packet;

    /* Find the correct place in the queue to insert the packet */
    while (cur) {
        int16_t diff = seq - cur->seq;
        if (diff < 0)
            break;
        prev = cur;
        cur = cur->next;
    }

    packet = av_mallocz(sizeof(*packet));
    if (!packet)
        return;
    packet->recvtime = av_gettime();
    packet->seq = seq;
    packet->len = len;
    packet->buf = buf;
    packet->next = cur;
    if (prev)
        prev->next = packet;
    else
        s->queue = packet;
    s->queue_len++;
}

static int has_next_packet(RTPDemuxContext *s)
{
#if PROJECT_NVR
   if(s->handle_h264_combine_frame){
        return 0;
    }
    if(s->handle_h264_packet_loss){
        return s->queue && s->queue->seq == (uint16_t) (s->h264_parsed_seq + 1);
    }
#endif
    return s->queue && s->queue->seq == (uint16_t) (s->seq + 1);
}

int64_t ff_rtp_queued_packet_time(RTPDemuxContext *s)
{
#if PROJECT_NVR
    if(s->handle_h264_packet_loss){
        return 0;
    }
#endif
    return s->queue ? s->queue->recvtime : 0;
}

static int rtp_parse_queued_packet(RTPDemuxContext *s, AVPacket *pkt)
{
    int rv;
    RTPPacket *next;

#if PROJECT_NVR
    if(s->handle_h264_combine_frame){
        return -1;
    }
#endif

    if (s->queue_len <= 0)
        return -1;

#if PROJECT_NVR
    if ((!s->handle_h264_packet_loss)  && (!has_next_packet(s)))
#else
    if (!has_next_packet(s))
#endif
        av_log(s->st ? s->st->codec : NULL, AV_LOG_WARNING,
               "RTP: missed %d packets\n", s->queue->seq - s->seq - 1);

    /* Parse the first packet in the queue, and dequeue it */
    rv = rtp_parse_packet_internal(s, pkt, s->queue->buf, s->queue->len);
    next = s->queue->next;
    av_free(s->queue->buf);
    av_free(s->queue);
    s->queue = next;
    s->queue_len--;
#if PROJECT_NVR
    if(s->handle_h264_packet_loss && (!s->queue_len)){
        s->queue_tail = NULL;
    }
#endif
    return rv;
}

#if PROJECT_NVR
static int checkFramePacketNumber(RTPDemuxContext *s){
    RTPPacket *cur = s->recv_queue, *tail = s->recv_queue_tail;
    int count;

    if(s->recv_queue->seq <= tail->seq){
        count = tail->seq - s->recv_queue->seq + 1;
    }else{
        count = 0xffff - s->recv_queue->seq + 1;
        count += tail->seq + 1;
    }

    if(count !=  s->recv_queue_len){
        av_log(s->st ? s->st->codec : NULL, AV_LOG_WARNING,"RTP Discard--- check_frame_packets, head_seq = 0x%x, tail_seq = %x,count = %d, queue_len = %d\n",\
            s->recv_queue->seq,tail->seq,count,s->recv_queue_len);
        return -1;
    }
    return 0;
}
static int skip_rtp_header(const uint8_t *buf,int len, uint8_t **bufptr,int *lenptr){
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
            av_log(NULL, AV_LOG_ERROR, "skip_rtp_header --- error 1\n");
            return -1;
        }
        /* calculate the header extension length (stored as number
         * of 32-bit words) */
        ext = (AV_RB16(buf + 2) + 1) << 2;

        if (len < ext){
            av_log(NULL, AV_LOG_ERROR, "skip_rtp_header --- error 2\n");
            return -1;
        }
        // skip past RTP header extension
        len -= ext;
        buf += ext;
    }
    *bufptr = buf;
    *lenptr = len;
    return 0;
}

static int getNalType(uint8_t *buf,int len,uint8_t *type,uint8_t *fu_header){
    uint8_t *payload;
    int payload_len;
    uint8_t nal;
    if(skip_rtp_header(buf,len,&payload,&payload_len) < 0){
        av_log(NULL, AV_LOG_ERROR, "getNalType --- skip_rtp_header error\n");
        return -1;
    }
    if(payload_len < 2){
        av_log(NULL, AV_LOG_ERROR, "getNalType --- payload_len too smal,len =  %d\n",payload_len);
        return -1;
    }

    nal = payload[0];
    *type = (nal & 0x1f);
    if(*type == 28){//FU-A
        *fu_header = payload[1];
    }
    return 0;
}
static void check_frame_packets(RTPDemuxContext *s){
    int discard_flag = 0;

    if(checkFramePacketNumber(s) < 0){
        discard_flag = 1;
    }else{
        RTPPacket * packet = s->recv_queue;
        RTPPacket *first = NULL,*last = NULL;
        while (packet) {
            uint8_t type,fu_header;
            if(getNalType(packet->buf,packet->len,&type,&fu_header) < 0){
                av_log(s->st ? s->st->codec : NULL, AV_LOG_WARNING,"RTP Discard--- check_frame_packets, invalid rtp packet\n");
                discard_flag = 1;
                break;
            }else{
                if(type == 28){//FU-A
                    if(!first) {
                        first = packet;
                        uint8_t start_bit =  fu_header>> 7;
                        if(!start_bit){
                            av_log(s->st ? s->st->codec : NULL, AV_LOG_WARNING,"RTP Discard--- check_frame_packets,FU-A start_bit not found\n");
                            discard_flag = 1;
                            break;
                        }
                    }
                    last = packet;
                    if(!last->next){
                        uint8_t end_bit = (fu_header & 0x40) >> 6;
                        if(!end_bit){
                            av_log(s->st ? s->st->codec : NULL, AV_LOG_WARNING,"RTP Discard--- check_frame_packets,FU-A end_bit not found\n");
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
            av_free(s->recv_queue);
            s->recv_queue = next;
        }
        s->recv_queue_len = 0;
        s->recv_queue_tail = NULL;
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


static int rtp_check_h264_packet(RTPDemuxContext *s, const uint8_t *buf, int len)
{
    unsigned int ssrc, h;
    int payload_type, seq, ret, flags = 0;
    int ext;
    uint32_t timestamp;
    int rv= 0;

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


    // only do something with this if all the rtp checks pass...
    if(!rtp_valid_packet_in_sequence(&s->statistics, seq))
    {
        av_log(s->st ? s->st->codec : NULL, AV_LOG_ERROR, "rtp_check_h264_packet RTP: PT=%02x: bad cseq %04x expected=%04x\n",
               payload_type, seq, ((s->seq + 1) & 0xffff));
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


static int  get_h264_nalu_length(const uint8_t * buf, int len)
{
    uint8_t nal,type;
    uint8_t start_sequence[]= {0, 0, 0, 1};

    assert(buf);
    if(len < 2){
        av_log(NULL, AV_LOG_ERROR, "get_h264_nalu_length --- payload_len too smal,len =  %d\n",len);
        return -1;
    }

    nal = buf[0];
    type = (nal & 0x1f);

    if(type <=23){
        return len + sizeof(start_sequence);
    }

    if(type == 28) {//FU-A
        uint8_t fu_header,start_bit;
        buf++;
        len--;                  // skip the fu_indicator

        fu_header = *buf;   // read the fu_header.
        start_bit = fu_header >> 7;

        // skip the fu_header...
        buf++;
        len--;

        if(start_bit) {
            return sizeof(start_sequence)+sizeof(nal)+len;
        } else {
            return len;
        }
    }

    if(type == 24) {//STAP-A (one packet, multiple nals)
        // consume the STAP-A NAL
        buf++;
        len--;
        //figure out the total size....
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
                    total_length+= sizeof(start_sequence)+nal_size;
                } else {
                    av_log(NULL, AV_LOG_ERROR, "nal size exceeds length: %d %d\n", nal_size, src_len);
                    return -1;
                }

                // eat what we handled...
                src += nal_size;
                src_len -= nal_size;

                if (src_len < 0){
                    av_log(NULL, AV_LOG_ERROR, "Consumed more bytes than we got! (%d)\n", src_len);
                    return -1;
                }
            }
            return total_length;
        }
    }
    av_log(NULL, AV_LOG_ERROR,"get_h264_nalu_length -- error type = %d\n",type);
    return -1;
}

static int get_h264_packet_length(const uint8_t * buf, int len)
{
    uint8_t *payload;
    int payload_len;
    if(skip_rtp_header(buf,len,&payload,&payload_len) < 0){
        av_log(NULL, AV_LOG_ERROR, "get_h264_packet_length --- skip_rtp_header error\n");
        return -1;
    }
    return get_h264_nalu_length(payload,payload_len);
}

static int rtp_add_h264_nalu(const uint8_t * buf, int len,AVPacket * pkt,int offset)
{
    uint8_t nal,type;
    uint8_t start_sequence[]= {0, 0, 0, 1};

    assert(buf);
    if(len < 2){
        av_log(NULL, AV_LOG_ERROR, "rtp_add_h264_nalu --- payload_len too smal,len =  %d\n",len);
        return -1;
    }

    nal = buf[0];
    type = (nal & 0x1f);
    if (type <= 23){
        memcpy(pkt->data + offset, start_sequence, sizeof(start_sequence));
        memcpy(pkt->data + offset + sizeof(start_sequence), buf, len);
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
    av_log(NULL, AV_LOG_ERROR, "Unimplemented type or Undefined type (%d)", type);
    return -1;
}

static int rtp_h264_add_packet(const uint8_t * buf, int len,AVPacket *pkt,int offset)
{
    uint8_t *payload;
    int payload_len;
    if(skip_rtp_header(buf,len,&payload,&payload_len) < 0){
        av_log(NULL, AV_LOG_ERROR, "rtp_h264_add_packet --- skip_rtp_header error\n");
        return -1;
    }
    return rtp_add_h264_nalu(payload,payload_len,pkt,offset);
}

static void discard_frame_data(RTPDemuxContext *s){
    RTPPacket *packet = s->queue,*next;
    packet = s->queue;
    while(packet){
        next = packet->next;
        av_free(packet->buf);
        av_free(packet);
        packet = next;
    }
    s->queue = NULL;
    s->queue_tail = NULL;
    s->queue_len = 0;
}
static int rtp_parse_queued_frame_h264(RTPDemuxContext *s, AVPacket *pkt)
{
    int total_len = 0,offset;
    RTPPacket *packet = s->queue,*next;

    if(!packet){
        return -1;
    }

    while(packet){
        int len =get_h264_packet_length(packet->buf,packet->len);
        if(len < 0){
            total_len = -1;
            break;
        }
        total_len += len;
        packet = packet->next;
    }

    if(total_len < 0){
        av_log(s->st ? s->st->codec : NULL, AV_LOG_ERROR, "rtp_parse_queued_frame_h264, discard frame, unexpected, TODO\n");
        discard_frame_data(s);
        return -1;
    }

    //av_log(NULL, AV_LOG_ERROR, "rtp_parse_queued_frame_h264, frame_length = %d\n",total_len);
    if(av_new_packet(pkt,total_len) < 0){
        av_log(s->st ? s->st->codec : NULL, AV_LOG_ERROR, "rtp_parse_queued_frame_h264, av_new_packet failed,TODO\n");
        discard_frame_data(s);
        return -1;
    }

    packet = s->queue;
    offset = 0;
    while(packet){
        next = packet->next;
        offset += rtp_h264_add_packet(packet->buf,packet->len,pkt,offset);
        av_free(packet->buf);
        av_free(packet);
        packet = next;
    }
    s->queue = NULL;
    s->queue_tail = NULL;
    s->queue_len = 0;

    // now perform timestamp things....
    pkt->stream_index = s->st->index;
    finalize_packet(s, pkt, s->recv_queue_timestamp);
    return 0;
}


/* For Project NVR,
*     discard frame-packets if packet loss occured.
*     combine packets to frame.
*     Reorder has not been considered.  TODO
*/
static int handle_recv_h264_packet(RTPDemuxContext *s,AVPacket *pkt, uint8_t *buf, int len)
{
    uint16_t seq = AV_RB16(buf + 2);
    uint32_t timestamp = AV_RB32(buf + 4);
    RTPPacket *packet;
    int rv = -1;

    if(s->recv_queue && (s->recv_queue_timestamp != timestamp)){
        check_frame_packets(s);
        if(s->handle_h264_combine_frame){
            rv = rtp_parse_queued_frame_h264(s, pkt);
        }
    }
    if(!s->handle_h264_combine_frame){
        rv = rtp_parse_queued_packet(s, pkt);
    }

    //parse and discard invalid rtp packet here according to RFC
    if(rtp_check_h264_packet(s,buf,len) < 0){
        av_free(buf);
        return rv;
    }

    //
    //send back rr here, for that rtsp.c will not got this if packet loss occured.
    //
    do_rtp_check_and_send_back_rr(s,len);

    packet = av_mallocz(sizeof(*packet));
    if (packet){
        packet->recvtime = av_gettime();
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
    }else{
        av_log(s->st ? s->st->codec : NULL, AV_LOG_WARNING,"RTP Discard--- handle_recv_packet,failed to alloc RTPPacket\n");
        av_free(buf);
    }
    return rv;
}
#endif

static int rtp_parse_one_packet(RTPDemuxContext *s, AVPacket *pkt,
                     uint8_t **bufptr, int len)
{
    uint8_t* buf = bufptr ? *bufptr : NULL;
    int ret, flags = 0;
    uint32_t timestamp;
    int rv= 0;

    if (!buf) {
#if PROJECT_NVR
        if(s->handle_h264_combine_frame) {
            av_log(s->st ? s->st->codec : NULL, AV_LOG_WARNING,"rtp_parse_one_packet--- should not happen\n");
            return -1;
        }
#endif
        /* If parsing of the previous packet actually returned 0 or an error,
         * there's nothing more to be parsed from that packet, but we may have
         * indicated that we can return the next enqueued packet. */
        if (s->prev_ret <= 0)
            return rtp_parse_queued_packet(s, pkt);
        /* return the next packets, if any */
        if(s->st && s->parse_packet) {
            /* timestamp should be overwritten by parse_packet, if not,
             * the packet is left with pts == AV_NOPTS_VALUE */
            timestamp = RTP_NOTS_VALUE;
            rv= s->parse_packet(s->ic, s->dynamic_protocol_context,
                                s->st, pkt, &timestamp, NULL, 0, flags);
            finalize_packet(s, pkt, timestamp);
            return rv;
        } else {
            // TODO: Move to a dynamic packet handler (like above)
            if (s->read_buf_index >= s->read_buf_size)
                return AVERROR(EAGAIN);
            ret = ff_mpegts_parse_packet(s->ts, pkt, s->buf + s->read_buf_index,
                                      s->read_buf_size - s->read_buf_index);
            if (ret < 0)
                return AVERROR(EAGAIN);
            s->read_buf_index += ret;
            if (s->read_buf_index < s->read_buf_size)
                return 1;
            else
                return 0;
        }
    }

    if (len < 12)
        return -1;

    if ((buf[0] & 0xc0) != (RTP_VERSION << 6))
        return -1;
    if (buf[1] >= RTCP_SR && buf[1] <= RTCP_APP) {
        return rtcp_parse_packet(s, buf, len);
    }

#if PROJECT_NVR
    if(s->handle_h264_packet_loss) {
        rv = handle_recv_h264_packet(s,pkt,buf,len);
        *bufptr = NULL;
        return rv;
    }
#endif

    if ((s->seq == 0 && !s->queue) || s->queue_size <= 1) {
        /* First packet, or no reordering */
        return rtp_parse_packet_internal(s, pkt, buf, len);
    } else {
        uint16_t seq = AV_RB16(buf + 2);
        int16_t diff = seq - s->seq;
        if (diff < 0) {
            /* Packet older than the previously emitted one, drop */
            av_log(s->st ? s->st->codec : NULL, AV_LOG_WARNING,
                   "RTP: dropping old packet received too late\n");
            return -1;
        } else if (diff <= 1) {
            /* Correct packet */
            rv = rtp_parse_packet_internal(s, pkt, buf, len);
            return rv;
        } else {
            /* Still missing some packet, enqueue this one. */
            enqueue_packet(s, buf, len);
            *bufptr = NULL;
            /* Return the first enqueued packet if the queue is full,
             * even if we're missing something */
            if (s->queue_len >= s->queue_size)
                return rtp_parse_queued_packet(s, pkt);
            return -1;
        }
    }
}

/**
 * Parse an RTP or RTCP packet directly sent as a buffer.
 * @param s RTP parse context.
 * @param pkt returned packet
 * @param bufptr pointer to the input buffer or NULL to read the next packets
 * @param len buffer len
 * @return 0 if a packet is returned, 1 if a packet is returned and more can follow
 * (use buf as NULL to read the next). -1 if no packet (error or no more packet).
 */
int rtp_parse_packet(RTPDemuxContext *s, AVPacket *pkt,
                     uint8_t **bufptr, int len)
{
    int rv = rtp_parse_one_packet(s, pkt, bufptr, len);
    s->prev_ret = rv;
    while (rv == AVERROR(EAGAIN) && has_next_packet(s))
        rv = rtp_parse_queued_packet(s, pkt);
    return rv ? rv : has_next_packet(s);
}

void rtp_parse_close(RTPDemuxContext *s)
{
    ff_rtp_reset_packet_queue(s);
    if (!strcmp(ff_rtp_enc_name(s->payload_type), "MP2T")) {
        ff_mpegts_parse_close(s->ts);
    }
    av_free(s);
}

int ff_parse_fmtp(AVStream *stream, PayloadContext *data, const char *p,
                  int (*parse_fmtp)(AVStream *stream,
                                    PayloadContext *data,
                                    char *attr, char *value))
{
    char attr[256];
    char *value;
    int res;
    int value_size = strlen(p) + 1;

    if (!(value = av_malloc(value_size))) {
        av_log(stream, AV_LOG_ERROR, "Failed to allocate data for FMTP.");
        return AVERROR(ENOMEM);
    }

    // remove protocol identifier
    while (*p && *p == ' ') p++; // strip spaces
    while (*p && *p != ' ') p++; // eat protocol identifier
    while (*p && *p == ' ') p++; // strip trailing spaces

    while (ff_rtsp_next_attr_and_value(&p,
                                       attr, sizeof(attr),
                                       value, value_size)) {

        res = parse_fmtp(stream, data, attr, value);
        if (res < 0 && res != AVERROR_PATCHWELCOME) {
            av_free(value);
            return res;
        }
    }
    av_free(value);
    return 0;
}
