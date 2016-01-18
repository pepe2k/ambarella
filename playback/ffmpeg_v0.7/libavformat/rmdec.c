/*
 * "Real" compatible demuxer.
 * Copyright (c) 2000, 2001 Fabrice Bellard
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

#include "libavutil/avstring.h"
#include "libavutil/intreadwrite.h"
#include "avformat.h"
#include "riff.h"
#include "rm.h"
#define MAX_RM_STREAMS 32

struct RMStream {
    AVPacket pkt;      ///< place to store merged video frame / reordered audio data
    int videobufsize;  ///< current assembled frame size
    int videobufpos;   ///< position for the next slice in the video buffer
    int curpic_num;    ///< picture number of current frame
    int cur_slice, slices;
    int64_t pktpos;    ///< first slice position in file
    /// Audio descrambling matrix parameters
    int64_t audiotimestamp; ///< Audio packet timestamp
    int sub_packet_cnt; // Subpacket counter, used while reading
    int sub_packet_size, sub_packet_h, coded_framesize; ///< Descrambling parameters from container
    int audio_framesize; /// Audio frame size from container
    int sub_packet_lengths[16]; /// Length of each subpacket

	int index;  //correspond to RMDemuxContext's streams
	int codec_data_size;
	int codec_pos;
	enum CodecID     codec_id;
	enum AVMediaType codec_type;
	enum AVStreamParseType need_parsing;
	unsigned int format; //store the fourcc for each rmstream
	int extradata_size;
	uint8_t* extradata;	
	
	//Stream info from MDPR
	int stream_id; 
	int64_t duration;
	int64_t starttime;
	int bitrate;
	char *descr, *mimet;

	//For Audio
	int channels;
	
	int sample_rate;
	int sample_size; //for ?	
	unsigned int intl_id;
	int codec_block_align;

	//For Video
	int v_fps;
	int height;
	int width;
};

struct RMDemuxContext {
    int nb_packets;
    int old_format;
    int current_stream;
    int remaining_len;
    int audio_stream_num; ///< Stream number for audio packets
    int audio_pkt_cnt; ///< Output packet counter

		
    AVFormatContext* ctx;
    unsigned long duration;

	unsigned int index_off;
	unsigned int flags;
	int data_offset; /* using in is_multirate == 0 for data index */
	int is_multirate;
	int str_data_offset[MAX_RM_STREAMS]; /* Data chunk offset for every audio/video stream */
	int audio_index; //-1
	int video_index;  //-1
	int a_bitrate; /*choose a max audio bitrate stream */
	int v_bitrate;

	RMStream* streams[MAX_RM_STREAMS];
	int nb_streams;
	int a_streams; /*the index for our choosed audio RMStream */
	int v_streams;


	//for is_multirate and have video and audio
	int audio_curpos;
         //unsigned int a_nb_readed;
	unsigned int a_nb_packets; // no used currently
	int video_curpos;
         //unsigned int v_nb_readed;
	unsigned int v_nb_packets;
         int av_gap; // the readed a /v gap (a_readed - v_readed)
	int stream_switch;
};

//if we meet a MDPR, we new a RMStream. this stream has a stream_id from MDPR,
//and also has a index which corresponds to RMDemuxContext's streams index.
RMStream* ff_rm_new_stream(RMDemuxContext* rmctx);

void ff_rm_free_rmstream_cc(RMDemuxContext* rmctx, RMStream* rms);

static int rm_read_audio_stream_info_cc(AVFormatContext *s, AVIOContext *pb,
                                        RMStream* rmst, int read_all);
                                     
int ff_rm_read_mdpr_codecdata_cc (AVFormatContext *s, AVIOContext *pb,
                                  RMStream *rmst);

static void check_rm_av_stream(AVFormatContext* s);

static void seek_to_data(AVFormatContext* s);
static int create_av_stream(AVFormatContext* s);
static void parse_rm_index(AVFormatContext* s);

static const unsigned char sipr_swaps[38][2] = {
    {  0, 63 }, {  1, 22 }, {  2, 44 }, {  3, 90 },
    {  5, 81 }, {  7, 31 }, {  8, 86 }, {  9, 58 },
    { 10, 36 }, { 12, 68 }, { 13, 39 }, { 14, 73 },
    { 15, 53 }, { 16, 69 }, { 17, 57 }, { 19, 88 },
    { 20, 34 }, { 21, 71 }, { 24, 46 }, { 25, 94 },
    { 26, 54 }, { 28, 75 }, { 29, 50 }, { 32, 70 },
    { 33, 92 }, { 35, 74 }, { 38, 85 }, { 40, 56 },
    { 42, 87 }, { 43, 65 }, { 45, 59 }, { 48, 79 },
    { 49, 93 }, { 51, 89 }, { 55, 95 }, { 61, 76 },
    { 67, 83 }, { 77, 80 }
};

const unsigned char ff_sipr_subpk_size[4] = { 29, 19, 37, 20 };

static inline void get_strl(AVIOContext *pb, char *buf, int buf_size, int len)
{
    int i;
    char *q, r;

    q = buf;
    for(i=0;i<len;i++) {
        r = avio_r8(pb);
        if (i < buf_size - 1)
            *q++ = r;
    }
    if (buf_size > 0) *q = '\0';
}

static void get_str8(AVIOContext *pb, char *buf, int buf_size)
{
    get_strl(pb, buf, buf_size, avio_r8(pb));
}

static int rm_read_extradata(AVIOContext *pb, AVCodecContext *avctx, unsigned size)
{
    if (size >= 1<<24)
        return -1;
    avctx->extradata = av_malloc(size + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!avctx->extradata)
        return AVERROR(ENOMEM);
    avctx->extradata_size = avio_read(pb, avctx->extradata, size);
    memset(avctx->extradata + avctx->extradata_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
    if (avctx->extradata_size != size)
        return AVERROR(EIO);
    return 0;
}

static void rm_read_metadata(AVFormatContext *s, int wide)
{
    char buf[1024];
    int i;
    for (i=0; i<FF_ARRAY_ELEMS(ff_rm_metadata); i++) {
        int len = wide ? avio_rb16(s->pb) : avio_r8(s->pb);
        get_strl(s->pb, buf, sizeof(buf), len);
        av_metadata_set2(&s->metadata, ff_rm_metadata[i], buf, 0);
    }
}

RMStream *ff_rm_alloc_rmstream (void)
{
    RMStream *rms = av_mallocz(sizeof(RMStream));
    rms->curpic_num = -1;
    return rms;
}

void ff_rm_free_rmstream (RMStream *rms)
{
    av_free_packet(&rms->pkt);
}

static int rm_read_audio_stream_info(AVFormatContext *s, AVIOContext *pb,
                                     AVStream *st, RMStream *ast, int read_all)
{
    char buf[256];
    uint32_t version;
    int ret;

    /* ra type header */
    version = avio_rb16(pb); /* version */
    if (version == 3) {
        int header_size = avio_rb16(pb);
        int64_t startpos = avio_tell(pb);
        avio_skip(pb, 14);
        rm_read_metadata(s, 0);
        if ((startpos + header_size) >= avio_tell(pb) + 2) {
            // fourcc (should always be "lpcJ")
            avio_r8(pb);
            get_str8(pb, buf, sizeof(buf));
        }
        // Skip extra header crap (this should never happen)
        if ((startpos + header_size) > avio_tell(pb))
            avio_skip(pb, header_size + startpos - avio_tell(pb));
        st->codec->sample_rate = 8000;
        st->codec->channels = 1;
        st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
        st->codec->codec_id = CODEC_ID_RA_144;
    } else {
        int flavor, sub_packet_h, coded_framesize, sub_packet_size;
        int codecdata_length;
        /* old version (4) */
        avio_skip(pb, 2); /* unused */
        avio_rb32(pb); /* .ra4 */
        avio_rb32(pb); /* data size */
        avio_rb16(pb); /* version2 */
        avio_rb32(pb); /* header size */
        flavor= avio_rb16(pb); /* add codec info / flavor */
        ast->coded_framesize = coded_framesize = avio_rb32(pb); /* coded frame size */
        avio_rb32(pb); /* ??? */
        avio_rb32(pb); /* ??? */
        avio_rb32(pb); /* ??? */
        ast->sub_packet_h = sub_packet_h = avio_rb16(pb); /* 1 */
        st->codec->block_align= avio_rb16(pb); /* frame size */
        ast->sub_packet_size = sub_packet_size = avio_rb16(pb); /* sub packet size */
        avio_rb16(pb); /* ??? */
        if (version == 5) {
            avio_rb16(pb); avio_rb16(pb); avio_rb16(pb);
        }
        st->codec->sample_rate = avio_rb16(pb);
        avio_rb32(pb);
        st->codec->channels = avio_rb16(pb);
        if (version == 5) {
            avio_rb32(pb);
            avio_read(pb, buf, 4);
            buf[4] = 0;
        } else {
            get_str8(pb, buf, sizeof(buf)); /* desc */
            get_str8(pb, buf, sizeof(buf)); /* desc */
        }
        st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
        st->codec->codec_tag  = AV_RL32(buf);
        st->codec->codec_id   = ff_codec_get_id(ff_rm_codec_tags,
                                                st->codec->codec_tag);
        switch (st->codec->codec_id) {
        case CODEC_ID_AC3:
            st->need_parsing = AVSTREAM_PARSE_FULL;
            break;
        case CODEC_ID_RA_288:
            st->codec->extradata_size= 0;
            ast->audio_framesize = st->codec->block_align;
            st->codec->block_align = coded_framesize;

            if(ast->audio_framesize >= UINT_MAX / sub_packet_h){
                av_log(s, AV_LOG_ERROR, "ast->audio_framesize * sub_packet_h too large\n");
                return -1;
            }

            av_new_packet(&ast->pkt, ast->audio_framesize * sub_packet_h);
            break;
        case CODEC_ID_COOK:
        case CODEC_ID_ATRAC3:
        case CODEC_ID_SIPR:
            avio_rb16(pb); avio_r8(pb);
            if (version == 5)
                avio_r8(pb);
            codecdata_length = avio_rb32(pb);
            if(codecdata_length + FF_INPUT_BUFFER_PADDING_SIZE <= (unsigned)codecdata_length){
                av_log(s, AV_LOG_ERROR, "codecdata_length too large\n");
                return -1;
            }

            ast->audio_framesize = st->codec->block_align;
            if (st->codec->codec_id == CODEC_ID_SIPR) {
                if (flavor > 3) {
                    av_log(s, AV_LOG_ERROR, "bad SIPR file flavor %d\n",
                           flavor);
                    return -1;
                }
                st->codec->block_align = ff_sipr_subpk_size[flavor];
            } else {
                if(sub_packet_size <= 0){
                    av_log(s, AV_LOG_ERROR, "sub_packet_size is invalid\n");
                    return -1;
                }
                st->codec->block_align = ast->sub_packet_size;
            }
            if ((ret = rm_read_extradata(pb, st->codec, codecdata_length)) < 0)
                return ret;

            if(ast->audio_framesize >= UINT_MAX / sub_packet_h){
                av_log(s, AV_LOG_ERROR, "rm->audio_framesize * sub_packet_h too large\n");
                return -1;
            }

            av_new_packet(&ast->pkt, ast->audio_framesize * sub_packet_h);
            break;
        case CODEC_ID_AAC:
            avio_rb16(pb); avio_r8(pb);
            if (version == 5)
                avio_r8(pb);
            codecdata_length = avio_rb32(pb);
            if(codecdata_length + FF_INPUT_BUFFER_PADDING_SIZE <= (unsigned)codecdata_length){
                av_log(s, AV_LOG_ERROR, "codecdata_length too large\n");
                return -1;
            }
            if (codecdata_length >= 1) {
                avio_r8(pb);
                if ((ret = rm_read_extradata(pb, st->codec, codecdata_length - 1)) < 0)
                    return ret;
            }
            break;
        default:
            av_strlcpy(st->codec->codec_name, buf, sizeof(st->codec->codec_name));
        }
        if (read_all) {
            avio_r8(pb);
            avio_r8(pb);
            avio_r8(pb);
            rm_read_metadata(s, 0);
        }
    }
    return 0;
}

int
ff_rm_read_mdpr_codecdata (AVFormatContext *s, AVIOContext *pb,
                           AVStream *st, RMStream *rst, int codec_data_size)
{
    unsigned int v;
    int size;
    int64_t codec_pos;
    int ret;

    av_set_pts_info(st, 64, 1, 1000);
    codec_pos = avio_tell(pb);
    v = avio_rb32(pb);
    if (v == MKTAG(0xfd, 'a', 'r', '.')) {
        /* ra type header */
        if (rm_read_audio_stream_info(s, pb, st, rst, 0))
            return -1;
    } else {
        int fps, fps2;
        if (avio_rl32(pb) != MKTAG('V', 'I', 'D', 'O')) {
        fail1:
            av_log(st->codec, AV_LOG_ERROR, "Unsupported video codec\n");
            goto skip;
        }
        st->codec->codec_tag = avio_rl32(pb);
        st->codec->codec_id  = ff_codec_get_id(ff_rm_codec_tags,
                                               st->codec->codec_tag);
//        av_log(s, AV_LOG_DEBUG, "%X %X\n", st->codec->codec_tag, MKTAG('R', 'V', '2', '0'));
        if (st->codec->codec_id == CODEC_ID_NONE)
            goto fail1;
        st->codec->width = avio_rb16(pb);
        st->codec->height = avio_rb16(pb);
        st->codec->time_base.num= 1;
        fps= avio_rb16(pb);
        st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
        avio_rb32(pb);
        fps2= avio_rb16(pb);
        av_log(NULL, AV_LOG_DEBUG, "fps2=%d\n", fps2);
        avio_rb16(pb);

        if ((ret = rm_read_extradata(pb, st->codec, codec_data_size - (avio_tell(pb) - codec_pos))) < 0)
            return ret;

//        av_log(s, AV_LOG_DEBUG, "fps= %d fps2= %d\n", fps, fps2);
        st->codec->time_base.den = fps * st->codec->time_base.num;
        //XXX: do we really need that?
        switch(st->codec->extradata[4]>>4){
        case 1: st->codec->codec_id = CODEC_ID_RV10; break;
        case 2: st->codec->codec_id = CODEC_ID_RV20; break;
        case 3: st->codec->codec_id = CODEC_ID_RV30; break;
        case 4: st->codec->codec_id = CODEC_ID_RV40; break;
        default:
            av_log(st->codec, AV_LOG_ERROR, "extra:%02X %02X %02X %02X %02X\n", st->codec->extradata[0], st->codec->extradata[1], st->codec->extradata[2], st->codec->extradata[3], st->codec->extradata[4]);
            goto fail1;
        }
    }

skip:
    /* skip codec info */
    size = avio_tell(pb) - codec_pos;
    avio_skip(pb, codec_data_size - size);

    return 0;
}

/** this function assumes that the demuxer has already seeked to the start
 * of the INDX chunk, and will bail out if not. */
static int rm_read_index(AVFormatContext *s)
{
    AVIOContext *pb = s->pb;
    unsigned int size, n_pkts, str_id, next_off, n, pos, pts;
    AVStream *st;

    do {
        if (avio_rl32(pb) != MKTAG('I','N','D','X'))
            return -1;
        size     = avio_rb32(pb);
        if (size < 20)
            return -1;
        avio_skip(pb, 2);
        n_pkts   = avio_rb32(pb);
        str_id   = avio_rb16(pb);
        next_off = avio_rb32(pb);
        for (n = 0; n < s->nb_streams; n++)
            if (s->streams[n]->id == str_id) {
                st = s->streams[n];
                break;
            }
        if (n == s->nb_streams)
            goto skip;

        for (n = 0; n < n_pkts; n++) {
            avio_skip(pb, 2);
            pts = avio_rb32(pb);
            pos = avio_rb32(pb);
            avio_skip(pb, 4); /* packet no. */

            av_add_index_entry(st, pos, pts, 0, 0, AVINDEX_KEYFRAME);
        }

skip:
        if (next_off && avio_tell(pb) != next_off &&
            avio_seek(pb, next_off, SEEK_SET) < 0)
            return -1;
    } while (next_off);

    return 0;
}

static int rm_read_header_old(AVFormatContext *s, AVFormatParameters *ap)
{
    RMDemuxContext *rm = s->priv_data;
    AVStream *st;

    rm->old_format = 1;
    st = av_new_stream(s, 0);
    if (!st)
        return -1;
    st->priv_data = ff_rm_alloc_rmstream();
    return rm_read_audio_stream_info(s, s->pb, st, st->priv_data, 1);
}

static int rm_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    RMStream* rmst;
    //AVStream *st;
    AVIOContext *pb = s->pb;
    unsigned int tag, tag_size;
    int64_t tag_pos;
    //unsigned int start_time;
    //char buf[128];
    int header_size, num_headers;//, i, tmp

    RMDemuxContext *rm = s->priv_data;
    rm->video_index = -1;
    rm->audio_index = -1;
    rm->ctx = s;
    
    tag = avio_rl32(pb);
    if (tag == MKTAG('.', 'r', 'a', 0xfd)) {
        /* very old .ra format */
        return rm_read_header_old(s, ap);
    } else if (tag != MKTAG('.', 'R', 'M', 'F')) {
        return AVERROR(EIO);
    }

    header_size = avio_rb32(pb); /* header size */
    avio_rb16(pb);
    if (header_size == 0x10) {
      avio_rb16(pb);  /* header file version */
    }	
    else {
      avio_rb32(pb);
    }
    num_headers = avio_rb32(pb); /* number of headers */
    av_log(NULL, AV_LOG_DEBUG, "num_headers=%d\n", num_headers);

    for(;;) {
        if (url_feof(pb))
            return -1;
        //find a new chunk
        tag_pos = avio_tell(pb);
        av_log(NULL, AV_LOG_DEBUG, "tag_pos=%lld\n", tag_pos);
        tag = avio_rl32(pb);
        tag_size = avio_rb32(pb);
        avio_rb16(pb);
#if 0
        printf("tag=%c%c%c%c (%08x) size=%d\n",
               (tag) & 0xff,
               (tag >> 8) & 0xff,
               (tag >> 16) & 0xff,
               (tag >> 24) & 0xff,
               tag,
               tag_size);
#endif
        if (tag_size < 10 && tag != MKTAG('D', 'A', 'T', 'A')) {
            av_log(NULL, AV_LOG_ERROR, "invalid chunksize! (%d)\n", tag_size);
            return -1;
        }
        switch(tag) {
        case MKTAG('P', 'R', 'O', 'P'):
            /* file header */
            av_log(NULL, AV_LOG_INFO, "chunk ! (PROP)\n");
            avio_rb32(pb); /* max bit rate */
            avio_rb32(pb); /* avg bit rate */
            avio_rb32(pb); /* max packet size */
            avio_rb32(pb); /* avg packet size */
            rm->nb_packets = avio_rb32(pb); /* nb packets */
            rm->duration = avio_rb32(pb); /* duration */
            avio_rb32(pb); /* preroll */
            rm->index_off = avio_rb32(pb); /* index offset */
            rm->data_offset = avio_rb32(pb); /* data offset */
            avio_rb16(pb); /* nb streams */
            rm->flags = avio_rb16(pb); /* flags */
            break;
        case MKTAG('C', 'O', 'N', 'T'):
            /* content description header */
            av_log(NULL, AV_LOG_INFO, "chunk ! (CONT)\n");
            rm_read_metadata(s, 1);
            break;
        case MKTAG('M', 'D', 'P', 'R'):
            /* IMP: Media properties header */
            //rm->nb_streams, for this RMStream
            //av_log(NULL, AV_LOG_INFO, "chunk ! (MDPR)\n");
            rmst = ff_rm_new_stream(rm);
            if (!rmst)
              return AVERROR(ENOMEM);

            int len;
            rmst->stream_id = avio_rb16(pb);
            avio_rb32(pb); /* max bit rate */
            rmst->bitrate = avio_rb32(pb); /* bit rate */
            av_log(NULL, AV_LOG_INFO, "chunk ! (MDPR), The Stream id in rm file:%d,  the bitrate:%d\n", 
                                        rmst->stream_id, rmst->bitrate);
            avio_rb32(pb); /* max packet size */
            avio_rb32(pb); /* avg packet size */
            rmst->starttime = avio_rb32(pb); /* start time */
            avio_rb32(pb); /* preroll */
            rmst->duration = avio_rb32(pb); /* duration */
            if((len = avio_r8(pb)) > 0) /* Stream description */
            {
              rmst->descr = av_malloc(len + 1);
              get_strl(pb, rmst->descr, len+1, len);
              //do something
              //av_free(descr);
            }
			
            if((len = avio_r8(pb)) > 0) /* Stream mimetype */
            {
              rmst->mimet = av_malloc(len + 1);
              get_strl(pb, rmst->mimet, len+1, len);
              //do something
              //av_free(mimet);
            }
#if 0
		av_log(NULL, AV_LOG_INFO, "the descr:  %s() \n |||the mimet ::%s(%d)\n", rmst->descr, rmst->mimet, len);
#endif
            rmst->codec_data_size = avio_rb32(pb); /*codec priv data size */
            rmst->codec_pos = avio_tell(pb);
            if (ff_rm_read_mdpr_codecdata_cc(s, s->pb, rmst) < 0) /* only in ENOMEM */
              return -1;
            break;
        case MKTAG('D', 'A', 'T', 'A'):
            av_log(NULL, AV_LOG_INFO, "chunk ! (DATA)\n");
            goto header_end;
        default:
            /* unknown tag: skip it */
            avio_skip(pb, tag_size - 10);
            break;
        }
    }
 header_end:
    //set rmctx->audio_index and video.
    check_rm_av_stream(s);
    //we just create a audio stream and a video stream
    if(create_av_stream(s) < 0)
      return -1;
    seek_to_data(s);
    parse_rm_index(s);
	

    return 0;
}

static int get_num(AVIOContext *pb, int *len)
{
    int n, n1;

    n = avio_rb16(pb);
    (*len)-=2;
    n &= 0x7FFF;
    if (n >= 0x4000) {
        return n - 0x4000;
    } else {
        n1 = avio_rb16(pb);
        (*len)-=2;
        return (n << 16) | n1;
    }
}

/* multiple of 20 bytes for ra144 (ugly) */
#define RAW_PACKET_SIZE 1000
#define A_OVERFLOW  10
#define V_OVERFLOW  -200

//return -2 for EOS, -1 for sikp this packet
static int rm_sync_multirate(AVFormatContext *s, int64_t *timestamp, int *flags, int *num, int64_t *pos, int* len)
{
    RMDemuxContext* rm = s->priv_data;
    AVIOContext* pb = s->pb;
    int version, a_size=0, v_size=0;
    int64_t a_timestamp=0, v_timestamp=0;
    int choose_audio;

    if(rm->audio_curpos < 0 && rm->video_curpos < 0)
        return -2;
    if(rm->audio_curpos > 0)
    {
        avio_seek(pb, rm->audio_curpos, SEEK_SET);
        avio_rb16(pb); //version
        a_size = avio_rb16(pb); //packet size
        avio_rb16(pb); //stream num
        a_timestamp = avio_rb32(pb);
    }
    if(rm->video_curpos > 0)
    {
        avio_seek(pb, rm->video_curpos, SEEK_SET);
        avio_rb16(pb);
        v_size = avio_rb16(pb); //packet size
        avio_rb16(pb); //stream num
        v_timestamp = avio_rb32(pb);
    }
    if(rm->audio_curpos > 0 && rm->video_curpos < 0)
    {
        *pos = rm->audio_curpos;
        rm->audio_curpos += a_size;
        choose_audio = 1;
        goto choosed;
    }
    if(rm->video_curpos > 0 && rm->audio_curpos < 0)
    {
        *pos = rm->video_curpos;
        rm->video_curpos += v_size;
        choose_audio = 0;
        goto choosed;
    }
    
    //three stage decide of a/v  --1--V---2---A--3--
    //in 1, always A, in 3, always V, in 2, decided by timestamp.
    //this seem has some problem, must that the timestamp to be the main condition
    if(a_timestamp < v_timestamp)
    {
        choose_audio = 1;
        if(rm->av_gap < V_OVERFLOW)
            {}//do something
    }else{
        choose_audio = 0;
        if(rm->av_gap >A_OVERFLOW)
            {}//do something
    }

    if(choose_audio)
    {
        *pos = rm->audio_curpos;
        rm->audio_curpos += a_size;
        rm->av_gap++;
        choose_audio = 1;
    }else{
        //(A > av_gap > V_OVERFLOW  and v_timestamp < a) or av_gap > A 
        *pos = rm->video_curpos;
        rm->video_curpos += v_size;
        rm->av_gap--;
        choose_audio = 0;
    }

choosed:
    avio_seek(pb, *pos, SEEK_SET);
    version = avio_rb16(pb);
    *len = avio_rb16(pb);
    //int choose_audio = *pos == rm->audio_curpos

    //Error detection
    if((version==0x4441 && *len==0x5441) ||(version == 0x494e && *len == 0x4458) ) {
        //multirate file never using next DATA chunk?? and INDEX for EOS
        if(choose_audio)
            rm->audio_curpos = -1;
        else
            rm->video_curpos = -1;
        *len = 0;
        return -1;
    }
    if(*len == -256)
        return -2; //EOF
    if(*len < 12){
        //todo: av_seek to next packet
        //return -1;
        return -1;
    }
    //ok here
    *num = avio_rb16(pb);
    *timestamp = avio_rb32(pb);
    avio_r8(pb); /* reserved */
    *flags = avio_r8(pb); /* flags */
    *len = *len -12;
    if(version == 0x01) {
        avio_r8(pb);
        (*len)--;
    }
    //av_log(NULL, AV_LOG_INFO, "The rm av_gap:%d, choose audio:%d, timesample:a %lld---v %lld", rm->av_gap, choose_audio,
            //a_timestamp, v_timestamp);
    return 0;    
}

//return >0 for data size(clear), stream_index for avstream's index.
//first call this the file pointer is in a data packet of a DATA chunk, WHICH assigned in the seek_to_data()
static int rm_sync(AVFormatContext *s, int64_t *timestamp, int *flags, int *stream_index, int64_t *pos){
    RMDemuxContext *rm = s->priv_data;
    AVIOContext *pb = s->pb;
    AVStream *st;
    //uint32_t state=0xFFFFFFFF;
    int version;
    int rval;
	
    while(!url_feof(pb)){
        int len, num = -1, res, i;
        //*pos= avio_tell(pb) - 3;
        *pos = avio_tell(pb); //data packets of data chunk
        if (rm->remaining_len > 0) {
            num= rm->current_stream;
            len= rm->remaining_len;
            *timestamp = AV_NOPTS_VALUE;
            *flags= 0;
        } else {
          if(rm->is_multirate)
          {
              rval = rm_sync_multirate(s, timestamp, flags, &num, pos, &len);
              if(rval == -2)
                return -1;
              if(rval == -1)
                goto skip;
          }else{
            //for un_mulitrate or only video/audio is mulitrate
            version = avio_rb16(pb);
            len = avio_rb16(pb);

            if((version==0x4441) && (len==0x5441)) {
                //new data chunk, multirate using only one data chunk??
                if(rm->is_multirate)
                    return -1;
                //using this new data chunk
                avio_skip(pb, 14);
                version = avio_rb16(pb);
                len = avio_rb16(pb);
            }else if((version == 0x494e) && (len == 0x4458)){
                //find INDX chunk, should be EOF
                return -1;
            }

            if(len == -256)
                return -1; //EOF
            if(len < 12){
                //todo: av_seek to next packet
                //return -1;
                goto skip;
            }
            num = avio_rb16(pb);
            *timestamp = avio_rb32(pb);
            res= avio_r8(pb); /* reserved */
            av_log(NULL, AV_LOG_DEBUG, "res=%d\n", res);
            *flags = avio_r8(pb); /* flags */
            len = len -12;
            if(version == 0x01) {
                avio_r8(pb);
                len--;
            }
        }//end of  rm  is no multirate.
      }
        //we get a packet, test whether it in avstream.
        for(i=0;i<s->nb_streams;i++) {
            st = s->streams[i];
            if (num == st->id)
                break;
        }
        if (i == s->nb_streams) {
skip:
            /* skip packet if unknown number */
            avio_skip(pb, len);
            //set multirate av_curpos.
            rm->remaining_len = 0;
            continue;
        }
        *stream_index= i;

        return len;
    }
    return -1;
}

static int rm_assemble_video_frame(AVFormatContext *s, AVIOContext *pb,
                                   RMDemuxContext *rm, RMStream *vst,
                                   AVPacket *pkt, int len, int *pseq)
{
    int hdr, seq=0, pic_num=0, len2=0, pos=0;
    int type;

    hdr = avio_r8(pb); len--;
    type = hdr >> 6;

    if(type != 3){  // not frame as a part of packet
        seq = avio_r8(pb); len--;
    }
    if(type != 1){  // not whole frame
        len2 = get_num(pb, &len);
        pos  = get_num(pb, &len);
        pic_num = avio_r8(pb); len--;
    }
    if(len<0)
        return -1;
    rm->remaining_len = len;
    if(type&1){     // frame, not slice
        if(type == 3)  // frame as a part of packet
            len= len2;
        if(rm->remaining_len < len)
            return -1;
        rm->remaining_len -= len;
        if(av_new_packet(pkt, len + 9) < 0)
            return AVERROR(EIO);
        pkt->data[0] = 0;
        AV_WL32(pkt->data + 1, 1);
        AV_WL32(pkt->data + 5, 0);
        avio_read(pb, pkt->data + 9, len);
        return 0;
    }
    //now we have to deal with single slice

    *pseq = seq;
    if((seq & 0x7F) == 1 || vst->curpic_num != pic_num){
        vst->slices = ((hdr & 0x3F) << 1) + 1;
        vst->videobufsize = len2 + 8*vst->slices + 1;
        av_free_packet(&vst->pkt); //FIXME this should be output.
        if(av_new_packet(&vst->pkt, vst->videobufsize) < 0)
            return AVERROR(ENOMEM);
        vst->videobufpos = 8*vst->slices + 1;
        vst->cur_slice = 0;
        vst->curpic_num = pic_num;
        vst->pktpos = avio_tell(pb);
    }
    if(type == 2)
        len = FFMIN(len, pos);

    if(++vst->cur_slice > vst->slices)
        return 1;
    AV_WL32(vst->pkt.data - 7 + 8*vst->cur_slice, 1);
    AV_WL32(vst->pkt.data - 3 + 8*vst->cur_slice, vst->videobufpos - 8*vst->slices - 1);
    if(vst->videobufpos + len > vst->videobufsize)
        return 1;
    if (avio_read(pb, vst->pkt.data + vst->videobufpos, len) != len)
        return AVERROR(EIO);
    vst->videobufpos += len;
    rm->remaining_len-= len;

    if(type == 2 || (vst->videobufpos) == vst->videobufsize){
        vst->pkt.data[0] = vst->cur_slice-1;
        *pkt= vst->pkt;
        vst->pkt.data= NULL;
        vst->pkt.size= 0;
        if(vst->slices != vst->cur_slice) //FIXME find out how to set slices correct from the begin
            memmove(pkt->data + 1 + 8*vst->cur_slice, pkt->data + 1 + 8*vst->slices,
                vst->videobufpos - 1 - 8*vst->slices);
        pkt->size = vst->videobufpos + 8*(vst->cur_slice - vst->slices);
        pkt->pts = AV_NOPTS_VALUE;
        pkt->pos = vst->pktpos;
        vst->slices = 0;
        return 0;
    }

    return 1;
}

static inline void
rm_ac3_swap_bytes (AVStream *st, AVPacket *pkt)
{
    uint8_t *ptr;
    int j;

    if (st->codec->codec_id == CODEC_ID_AC3) {
        ptr = pkt->data;
        for (j=0;j<pkt->size;j+=2) {
            FFSWAP(int, ptr[0], ptr[1]);
            ptr += 2;
        }
    }
}

/**
 * Perform 4-bit block reordering for SIPR data.
 * @todo This can be optimized, e.g. use memcpy() if data blocks are aligned
 */
void ff_rm_reorder_sipr_data(uint8_t *buf, int sub_packet_h, int framesize)
{
    int n, bs = sub_packet_h * framesize * 2 / 96; // nibbles per subpacket

    for (n = 0; n < 38; n++) {
        int j;
        int i = bs * sipr_swaps[n][0];
        int o = bs * sipr_swaps[n][1];

        /* swap 4bit-nibbles of block 'i' with 'o' */
        for (j = 0; j < bs; j++, i++, o++) {
            int x = (buf[i >> 1] >> (4 * (i & 1))) & 0xF,
                y = (buf[o >> 1] >> (4 * (o & 1))) & 0xF;

            buf[o >> 1] = (x << (4 * (o & 1))) |
                (buf[o >> 1] & (0xF << (4 * !(o & 1))));
            buf[i >> 1] = (y << (4 * (i & 1))) |
                (buf[i >> 1] & (0xF << (4 * !(i & 1))));
        }
    }
}

int
ff_rm_parse_packet (AVFormatContext *s, AVIOContext *pb,
                    AVStream *st, RMStream *ast, int len, AVPacket *pkt,
                    int *seq, int flags, int64_t timestamp)
{
    RMDemuxContext *rm = s->priv_data;

    if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        rm->current_stream= st->id;
        if(rm_assemble_video_frame(s, pb, rm, ast, pkt, len, seq))
            return -1; //got partial frame
    } else if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
        if ((st->codec->codec_id == CODEC_ID_RA_288) ||
            (st->codec->codec_id == CODEC_ID_COOK) ||
            (st->codec->codec_id == CODEC_ID_ATRAC3) ||
            (st->codec->codec_id == CODEC_ID_SIPR)) {
            int x;
            int sps = ast->sub_packet_size;
            int cfs = ast->coded_framesize;
            int h = ast->sub_packet_h;
            int y = ast->sub_packet_cnt;
            int w = ast->audio_framesize;

            if (flags & 2)
                y = ast->sub_packet_cnt = 0;
            if (!y)
                ast->audiotimestamp = timestamp;

            switch(st->codec->codec_id) {
                case CODEC_ID_RA_288:
                    for (x = 0; x < h/2; x++)
                        avio_read(pb, ast->pkt.data+x*2*w+y*cfs, cfs);
                    break;
                case CODEC_ID_ATRAC3:
                case CODEC_ID_COOK:
                    for (x = 0; x < w/sps; x++)
                        avio_read(pb, ast->pkt.data+sps*(h*x+((h+1)/2)*(y&1)+(y>>1)), sps);
                    break;
                case CODEC_ID_SIPR:
                    avio_read(pb, ast->pkt.data + y * w, w);
                    break;
                default:
                    break;
            }

            if (++(ast->sub_packet_cnt) < h)
                return -1;
            if (st->codec->codec_id == CODEC_ID_SIPR)
                ff_rm_reorder_sipr_data(ast->pkt.data, h, w);

             ast->sub_packet_cnt = 0;
             rm->audio_stream_num = st->index;
             rm->audio_pkt_cnt = h * w / st->codec->block_align;
        } else if (st->codec->codec_id == CODEC_ID_AAC) {
            int x;
            rm->audio_stream_num = st->index;
            ast->sub_packet_cnt = (avio_rb16(pb) & 0xf0) >> 4;
            if (ast->sub_packet_cnt) {
                for (x = 0; x < ast->sub_packet_cnt; x++)
                    ast->sub_packet_lengths[x] = avio_rb16(pb);
                rm->audio_pkt_cnt = ast->sub_packet_cnt;
                ast->audiotimestamp = timestamp;
            } else
                return -1;
        } else {
            av_get_packet(pb, pkt, len);
            rm_ac3_swap_bytes(st, pkt);
        }
    } else
        av_get_packet(pb, pkt, len);

    pkt->stream_index = st->index;

#if 0
    if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        if(st->codec->codec_id == CODEC_ID_RV20){
            int seq= 128*(pkt->data[2]&0x7F) + (pkt->data[3]>>1);
            av_log(s, AV_LOG_DEBUG, "%d %"PRId64" %d\n", *timestamp, *timestamp*512LL/25, seq);

            seq |= (timestamp&~0x3FFF);
            if(seq - timestamp >  0x2000) seq -= 0x4000;
            if(seq - timestamp < -0x2000) seq += 0x4000;
        }
    }
#endif

    pkt->pts= timestamp;
    if (flags & 2)
        pkt->flags |= AV_PKT_FLAG_KEY;

    return st->codec->codec_type == AVMEDIA_TYPE_AUDIO ? rm->audio_pkt_cnt : 0;
}

int
ff_rm_retrieve_cache (AVFormatContext *s, AVIOContext *pb,
                      AVStream *st, RMStream *ast, AVPacket *pkt)
{
    RMDemuxContext *rm = s->priv_data;

    assert (rm->audio_pkt_cnt > 0);

    if (st->codec->codec_id == CODEC_ID_AAC)
        av_get_packet(pb, pkt, ast->sub_packet_lengths[ast->sub_packet_cnt - rm->audio_pkt_cnt]);
    else {
        av_new_packet(pkt, st->codec->block_align);
        memcpy(pkt->data, ast->pkt.data + st->codec->block_align * //FIXME avoid this
               (ast->sub_packet_h * ast->audio_framesize / st->codec->block_align - rm->audio_pkt_cnt),
               st->codec->block_align);
    }
    rm->audio_pkt_cnt--;
    if ((pkt->pts = ast->audiotimestamp) != AV_NOPTS_VALUE) {
        ast->audiotimestamp = AV_NOPTS_VALUE;
        pkt->flags = AV_PKT_FLAG_KEY;
    } else
        pkt->flags = 0;
    pkt->stream_index = st->index;

    return rm->audio_pkt_cnt;
}

static int rm_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    RMDemuxContext *rm = s->priv_data;
    AVStream *st=NULL;
    int i, len, res, seq = 1;
    int64_t timestamp, pos;
    int flags;

    for (;;) {
        if (rm->audio_pkt_cnt) {
            // If there are queued audio packet return them first
            st = s->streams[rm->audio_stream_num];
            ff_rm_retrieve_cache(s, s->pb, st, st->priv_data, pkt);
            flags = 0;
        } else {
            if (rm->old_format) {
                RMStream *ast;

                st = s->streams[0];
                ast = st->priv_data;
                timestamp = AV_NOPTS_VALUE;
                len = !ast->audio_framesize ? RAW_PACKET_SIZE :
                    ast->coded_framesize * ast->sub_packet_h / 2;
                flags = (seq++ == 1) ? 2 : 0;
                pos = avio_tell(s->pb);
            } else {
                len=rm_sync(s, &timestamp, &flags, &i, &pos);
                if (len > 0)
                    st = s->streams[i];
            }

            if(len<0 || url_feof(s->pb))
                return AVERROR(EIO);

            res = ff_rm_parse_packet (s, s->pb, st, st->priv_data, len, pkt,
                                      &seq, flags, timestamp);
            if((flags&2) && (seq&0x7F) == 1)
                av_add_index_entry(st, pos, timestamp, 0, 0, AVINDEX_KEYFRAME);
            if (res)
                continue;
        }

        if(  (st->discard >= AVDISCARD_NONKEY && !(flags&2))
           || st->discard >= AVDISCARD_ALL){
           //av_log(NULL, AV_LOG_INFO, "Be discard!!! stream index:%d, discard:%d, flags:%d\n", st->index, st->discard, flags);
            av_free_packet(pkt);
        } else
            break;
    }

    return 0;
}

static int rm_read_close(AVFormatContext *s)
{
    int i;
    RMDemuxContext* rmctx = s->priv_data;
    for (i=0;i<rmctx->nb_streams;i++)
        ff_rm_free_rmstream_cc(rmctx, rmctx->streams[i]);
    //set our AVStream's priv_data to NULL, or will double free!
    for(i=0;i<s->nb_streams;i++)
    	s->streams[i]->priv_data = NULL;
    return 0;
}

//fix bug_2295,chuchen 2012_3_20
//Every time to read pakets,the pakets in audio queue will be returned firstly, so we need to flush queue when do seek
//add the seek_func, but this func just sets audio_pkt_cnt to 0.
static int rm_read_seek(AVFormatContext *s, int stream_index, int64_t pts, int flags)
{
    RMDemuxContext *rm = s->priv_data;
    if(rm->audio_pkt_cnt){
          rm->audio_pkt_cnt = 0;
    }
    return -1;
}

static int rm_probe(AVProbeData *p)
{
    /* check file header */
    if ((p->buf[0] == '.' && p->buf[1] == 'R' &&
         p->buf[2] == 'M' && p->buf[3] == 'F' &&
         p->buf[4] == 0 && p->buf[5] == 0) ||
        (p->buf[0] == '.' && p->buf[1] == 'r' &&
         p->buf[2] == 'a' && p->buf[3] == 0xfd))
        return AVPROBE_SCORE_MAX;
    else
        return 0;
}

static int64_t rm_read_dts(AVFormatContext *s, int stream_index,
                               int64_t *ppos, int64_t pos_limit)
{
    RMDemuxContext *rm = s->priv_data;
    int64_t pos, dts, pos_tmp;
    int stream_index2, flags, len, h;
    AVStream *st_tmp;
    int loop_tmp = 0;

    pos = *ppos;

    st_tmp = s->streams[stream_index];
    AVIndexEntry *entries_tmp= st_tmp->index_entries;

    for(loop_tmp = 0;loop_tmp < st_tmp->nb_index_entries;loop_tmp++){
        if(pos <=entries_tmp[loop_tmp].pos){
             int len_tmp = 0;
             for(pos_tmp = entries_tmp[loop_tmp-1].pos;pos_tmp < pos;pos_tmp+=len_tmp){
                    url_fseek(s->pb, pos_tmp, SEEK_SET);
                    get_be16(s->pb);
                    len_tmp= get_be16(s->pb);
             }
             pos = pos_tmp;
             break;
        }
    }

    if(rm->old_format)
        return AV_NOPTS_VALUE;

    avio_seek(s->pb, pos, SEEK_SET);
    rm->remaining_len=0;
    for(;;){
        int seq=1;
        AVStream *st;

        len=rm_sync(s, &dts, &flags, &stream_index2, &pos);
        if(len<0)
            return AV_NOPTS_VALUE;

        st = s->streams[stream_index2];
        if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            h= avio_r8(s->pb); len--;
            if(!(h & 0x40)){
                seq = avio_r8(s->pb); len--;
            }
        }

        if((flags&2) && (seq&0x7F) == 1){
//            av_log(s, AV_LOG_DEBUG, "%d %d-%d %"PRId64" %d\n", flags, stream_index2, stream_index, dts, seq);
            av_add_index_entry(st, pos, dts, 0, 0, AVINDEX_KEYFRAME);
            if(stream_index2 == stream_index)
                break;
        }

        avio_skip(s->pb, len);
    }
    *ppos = pos;
    return dts;
}

//if we meet a MDPR, we new a RMStream. this stream has a stream_id from MDPR,
//and also has a index which corresponds to RMDemuxContext's streams index.
RMStream* ff_rm_new_stream(RMDemuxContext* rmctx)
{
    RMStream* st;
//    int i;

    if (rmctx->nb_streams >= MAX_RM_STREAMS)
        return NULL;

    st = ff_rm_alloc_rmstream();
    if (!st)
        return NULL;

    st->index = rmctx->nb_streams;
    st->starttime = AV_NOPTS_VALUE;
    st->duration = AV_NOPTS_VALUE;

    rmctx->streams[rmctx->nb_streams++] = st;
    return st;
}

void ff_rm_free_rmstream_cc(RMDemuxContext* rmctx, RMStream* rms)
{
	//this rmstream's index may be nb_streams during demux
	av_free(rms->descr);
	av_free(rms->mimet);
	av_free(rms->extradata);
	av_free_packet(&rms->pkt);
	rmctx->nb_streams--;
	av_freep(&rmctx->streams[rmctx->nb_streams]);	
}

static int rm_read_audio_stream_info_cc(AVFormatContext *s, AVIOContext *pb,
                                     RMStream* rmst, int read_all)
{
    char buf[256];
    uint32_t version;
    uint32_t head_size, len;
    int codecdata_length;

    /* ra type header */
    version = avio_rb16(pb); /* ra version */
    if (version  == 3) {
	head_size = avio_rb16(pb); /* header size */
	int64_t startpos = avio_tell(pb);
	avio_skip(pb, 10); /* Unknown */
	avio_skip(pb, 4);  /* this codec data size */
	//title, author, copyright, comment for each stream.
	//todo: 
       rm_read_metadata(s, 0);
	avio_skip(pb, 1); /* Unknown */
       len = avio_r8(pb);
	if(len != 4)
	{
    	    av_log(NULL, AV_LOG_WARNING, "Version 3 audio FourCC size  : %8x, should be 4\n", len);
	    avio_skip(pb, len-4);
	}
	// fourcc (should always be "lpcJ")
       	rmst->format = avio_rl32(pb);
	if(rmst->format != MKTAG('l','p','c','J'))
	{
	    av_log(NULL, AV_LOG_WARNING, "Version 3 audio with not FourCC 'lpcj' : %8x\n", rmst->format);
	}
        // Skip extra header crap (this should never happen)
        if ((startpos + head_size) > avio_tell(pb))
           avio_skip(pb, head_size + startpos - avio_tell(pb));
	rmst->codec_type = AVMEDIA_TYPE_AUDIO;
	rmst->codec_id = CODEC_ID_RA_144;
       rmst->channels =1;
	rmst->sample_rate = 8000;
	rmst->sample_size = 16;
	//rmst->frame_size = 240;
    } else {
        int flavor;
        /* old version (4) */
	 avio_rb16(pb); /* always 00 00 */
        avio_rb32(pb); /* .ra4 or .ra5*/
        avio_rb32(pb); /* may be data size */
        avio_rb16(pb); /* version4 or 5 */
        head_size = avio_rb32(pb); /* header size */
        flavor= avio_rb16(pb); /* add codec info / flavor */
	 //using rmst
        rmst->coded_framesize = avio_rb32(pb); /* coded frame size */
        avio_rb32(pb); /* ??? */
        avio_rb32(pb); /* ??? */
        avio_rb32(pb); /* ??? */
        rmst->sub_packet_h = avio_rb16(pb); /* 1 */
        rmst->codec_block_align = avio_rb16(pb); /*  frame size  */
        rmst->sub_packet_size = avio_rb16(pb); /* sub packet size */
        avio_rb16(pb); /* ??? */
		
        if (version == 5) {
            avio_rb16(pb); avio_rb16(pb); avio_rb16(pb); /* Unknown */
        }
        rmst->sample_rate = avio_rb16(pb);
        avio_rb16(pb); /* ??? */
	 rmst->sample_size = avio_rb16(pb);
        rmst->channels = avio_rb16(pb);
		
        if (version == 5)
	{
            get_strl(pb, buf, sizeof(buf), 4); //interleaver id
            rmst->intl_id = MKTAG(buf[0], buf[1], buf[2], buf[3]);
	    get_strl(pb, buf, sizeof(buf), 4);	//FOURCC
        }else{
            get_str8(pb, buf, sizeof(buf)); /* interleaver id */
	     rmst->intl_id = MKTAG(buf[0], buf[1], buf[2], buf[3]);
            get_str8(pb, buf, sizeof(buf)); /* FOURCC */
        }		
        rmst->codec_type = AVMEDIA_TYPE_AUDIO;
	 rmst->format = MKTAG(buf[0], buf[1], buf[2], buf[3]);
	//av_log(NULL, AV_LOG_INFO, "should be cook here: %d", (rmst->format == MKTAG('c', 'o', 'o', 'k'))?1:0);
	switch (rmst->format)
	{
	case MKTAG('d', 'n', 'e', 't'):
		av_log(NULL, AV_LOG_INFO, "Audio: DNET -> AC3\n");
            	rmst->codec_id = CODEC_ID_AC3;
            	rmst->need_parsing = AVSTREAM_PARSE_FULL;
		break;
		
	case MKTAG('1', '4', '_', '4'):
                //sh->wf->nBlockAlign = 0x14;
                break;

	case MKTAG('2', '8', '_', '8'):
		rmst->codec_id = CODEC_ID_RA_288;
           	rmst->audio_framesize = rmst->codec_block_align;
            	rmst->codec_block_align = rmst->coded_framesize;
		if(rmst->audio_framesize >= UINT_MAX / rmst->sub_packet_h)
		{
                	av_log(s, AV_LOG_ERROR, "ast->audio_framesize * sub_packet_h too large\n");
                	return -1;
            	}
            	av_new_packet(&rmst->pkt, rmst->audio_framesize * rmst->sub_packet_h);
		break;

	case MKTAG('s', 'i', 'p', 'r'):
	case MKTAG('a', 't', 'r', 'c'):
	case MKTAG('c', 'o', 'o', 'k'):
		// realaudio codec plugins - common:
		avio_skip(pb, 3);  // Skip 3 unknown bytes 
		if (version==5)
			avio_skip(pb, 1);   // Skip 1 additional unknown byte 
		codecdata_length = avio_rb32(pb);
		// Check extradata len, we can't store bigger values in cbSize anyway
		if ((unsigned)codecdata_length > 0xffff) 
		{
		        av_log(s, AV_LOG_ERROR, "Extradata too big (%d)\n", codecdata_length);
			 return -1;
		}
		
		if (rmst->format == MKTAG('c', 'o', 'o', 'k')) rmst->codec_id = CODEC_ID_COOK;
            	else if (rmst->format == MKTAG('s', 'i', 'p', 'r')) rmst->codec_id = CODEC_ID_SIPR;
            	else rmst->codec_id = CODEC_ID_ATRAC3;
				
		rmst->audio_framesize = rmst->codec_block_align;
            	if (rmst->codec_id == CODEC_ID_SIPR) 
		{
                	if (flavor > 3) 
			{
                    		av_log(s, AV_LOG_ERROR, "bad SIPR file flavor %d\n", flavor);
                    		return -1;
                	}
                	rmst->codec_block_align = ff_sipr_subpk_size[flavor];
           	}else{
                	if(rmst->sub_packet_size <= 0)
			{
                    		av_log(s, AV_LOG_ERROR, "sub_packet_size is invalid\n");
                   		return -1;
                	}
                	rmst->codec_block_align = rmst->sub_packet_size;
            	}

		rmst->extradata_size = codecdata_length;
		rmst->extradata= av_mallocz(rmst->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
            	avio_read(pb, rmst->extradata, rmst->extradata_size);
		if(rmst->audio_framesize >= UINT_MAX / rmst->sub_packet_h)
		{
                	av_log(s, AV_LOG_ERROR, "ast->audio_framesize * sub_packet_h too large\n");
                	return -1;
            	}
            	av_new_packet(&rmst->pkt, rmst->audio_framesize * rmst->sub_packet_h);
		break;
		
             /*   if (priv->intl_id[stream_id] == MKTAG('g', 'e', 'n', 'r'))
    			    sh->wf->nBlockAlign = sub_packet_size;
    			else
    			    sh->wf->nBlockAlign = coded_frame_size;
		break;*/

	case MKTAG('r', 'a', 'a', 'c'):
	case MKTAG('r', 'a', 'c', 'p'):
		/* This is just AAC. The two or five bytes of */
		/* config data needed for libfaad are stored */
		/* after the audio headers. */
		avio_skip(pb, 3);  // Skip 3 unknown bytes 
		if (version==5)
			avio_skip(pb, 1);   // Skip 1 additional unknown byte 
		codecdata_length = avio_rb32(pb);
		if ((unsigned)codecdata_length > 0xffff) 
		{
		        av_log(s, AV_LOG_ERROR, "Extradata too big (%d)\n", codecdata_length);
			return -1;
		}

		if (codecdata_length>=1) {
			rmst->extradata_size = codecdata_length -1;
			rmst->extradata= av_mallocz(rmst->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
			avio_skip(pb, 1);
			avio_read(pb, rmst->extradata, rmst->extradata_size);
		}
	   	rmst->codec_id = CODEC_ID_AAC;

		break;
	default:
		rmst->codec_id = CODEC_ID_NONE;
		av_log(s, AV_LOG_ERROR, "Audio: Unknown (%s)\n", buf);
		return -1;
	}

    }
    return 0;
}

int
ff_rm_read_mdpr_codecdata_cc (AVFormatContext *s, AVIOContext *pb,
                           RMStream *rmst)
{
	RMDemuxContext* rmctx = s->priv_data;
	unsigned int tag;
	int size;

	if (!strncmp(rmst->mimet, "audio/", 6))
	{
		if (strstr(rmst->mimet, "x-pn-realaudio") ||strstr(rmst->mimet, "x-pn-multirate-realaudio"))
		{
			tag = avio_rb32(pb);
			if (tag != MKTAG(0xfd, 'a', 'r', '.'))
			{
		    		av_log(NULL, AV_LOG_ERROR , "Audio: can't find .ra in codec data\n");
				//skip this chunk
				goto SKIP;
			} else {
				av_log(NULL, AV_LOG_ERROR, "Got a x-pn-realaudio chunk\n");
		    		/* audio header */
				int rval = rm_read_audio_stream_info_cc(s, pb, rmst, 0);
				if(rval < 0)
					goto SKIP;
				//here we get a new audio stream
				 if (rmctx->is_multirate && ((rmctx->audio_index == -1) ||
		                               ((rmctx->audio_index >= 0) && rmctx->a_bitrate && (rmst->bitrate > rmctx->a_bitrate))))
				 {
				 	rmctx->audio_index = rmst->index;
					rmctx->a_bitrate = rmst->bitrate;
				 }else if(!rmctx->is_multirate){
				 	rmctx->audio_index = rmst->index;
					rmctx->a_bitrate = rmst->bitrate;
				 }
				 
				rmctx->a_streams++;
			}
                            av_log(NULL, AV_LOG_DEBUG, "The %d audio(ra) stream format, audio bitrate:%d sample rate:%d, stream_id in MDPR:%d\n", rmctx->a_streams-1, 
                                        rmst->bitrate, rmst->sample_rate, rmst->stream_id);

		}else if(strstr(rmst->mimet, "X-MP3-draft-00")){
			/* Is ffmpeg support this format?? */
			av_log(NULL, AV_LOG_INFO, "Got a X-MP3-draft-00 chunk\n");
		       rmst->codec_id = CODEC_ID_MP3ADU;
			rmst->codec_type = AVMEDIA_TYPE_AUDIO;
		 	rmctx->audio_index = rmst->index;
			rmctx->a_bitrate = rmst->bitrate;
			rmctx->a_streams++;
		}else if(strstr(rmst->mimet,"x-ralf-mpeg4")){
			av_log(NULL, AV_LOG_ERROR , "Real lossless audio not supported yet\n");
			goto SKIP;
	  	}else if(strstr(rmst->mimet,"x-pn-encrypted-ra")){
		 	av_log(NULL, AV_LOG_ERROR, "Encrypted audio is not supported\n");
			goto SKIP;
	  	}else{
		 	av_log(NULL, AV_LOG_ERROR, "Unknown audio stream format\n");
			goto SKIP;
		}
	/*end strncmp(rmst->mimet, "audio" */
	}else if(!strncmp(rmst->mimet,"video/", 6)){
		if (strstr(rmst->mimet, "x-pn-realvideo") ||strstr(rmst->mimet, "x-pn-multirate-realvideo")) 
		{
			avio_skip(pb, 4);  // VIDO length, same as codec_data_size
			tag = avio_rb32(pb);
			if(tag != MKTAG('O', 'D', 'I', 'V'))
			{
		    		av_log(NULL, AV_LOG_ERROR, "Video: can't find VIDO in codec data\n");
				//skip this chunk
				goto SKIP;
			}else{
				av_log(NULL, AV_LOG_ERROR, "Got a video chunk\n");
				int fps, fps2;
				rmst->format = avio_rl32(pb);
				//av_log(s, AV_LOG_DEBUG, "%X %X\n", st->codec->codec_tag, MKTAG('R', 'V', '2', '0'));
        			if (rmst->format != MKTAG('R', 'V', '1', '0')
            				&& rmst->format != MKTAG('R', 'V', '2', '0')
            				&& rmst->format != MKTAG('R', 'V', '3', '0')
            				&& rmst->format != MKTAG('R', 'V', '4', '0')
            				&& rmst->format != MKTAG('R', 'V', 'T', 'R'))
            				goto SKIP;
			       rmst->codec_type = AVMEDIA_TYPE_VIDEO;
        			rmst->width = avio_rb16(pb);
				rmst->height = avio_rb16(pb);
				fps= avio_rb16(pb);
				if (fps <= 0)	fps=24; // we probably won't even care about fps
				avio_rb32(pb);
				fps2= avio_rb16(pb);
				if(fps2 > 0)
			    		fps = fps2;
				avio_rb16(pb);
				rmst->v_fps = fps;
				
        			rmst->extradata_size= rmst->codec_data_size - (avio_tell(pb) - rmst->codec_pos);
        			if(rmst->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE <= (unsigned)rmst->extradata_size){
            				//check is redundant as avio_read() will catch this
            				av_log(s, AV_LOG_ERROR, "rmst->extradata_size too large\n");
					goto SKIP;
            			}
        			rmst->extradata= av_mallocz(rmst->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
        			if (!rmst->extradata)
            				return AVERROR(ENOMEM);
        			avio_read(pb, rmst->extradata, rmst->extradata_size);

				// av_log(s, AV_LOG_DEBUG, "fps= %d fps2= %d\n", fps, fps2);
        			//st->codec->time_base.den = fps * st->codec->time_base.num;
        			switch(((uint8_t*)rmst->extradata)[4] >> 4)
				{
        			case 1:  rmst->codec_id = CODEC_ID_RV10; break;
        			case 2:  rmst->codec_id = CODEC_ID_RV20; break;
        			case 3:  rmst->codec_id = CODEC_ID_RV30; break;
        			case 4:  rmst->codec_id = CODEC_ID_RV40; break;
        			default: goto SKIP;
        			}
				//here we get a new video stream
				//av_log(NULL, AV_LOG_ERROR, "Unknown video stream format\n")
				 if (rmctx->is_multirate && ((rmctx->video_index == -1) ||
		                               ((rmctx->video_index >= 0) && rmctx->v_bitrate && (rmst->bitrate > rmctx->v_bitrate))))
				 {
				 	rmctx->video_index = rmst->index;
					rmctx->v_bitrate = rmst->bitrate;
				 }else if(!rmctx->is_multirate){
				 	rmctx->video_index = rmst->index;
					rmctx->v_bitrate = rmst->bitrate;
				 }
	
				rmctx->v_streams++;
                av_log(NULL, AV_LOG_DEBUG, "The %d video stream format, video bitrate:%d, stream_id in MDPR:%d\n", rmctx->v_streams-1, 
                                        rmst->bitrate, rmst->stream_id);
			}//end if(tag != MKTAG('O', 'D', 'I', 'V'))'s else
		}else{
			//other vidoe format
		 	av_log(NULL, AV_LOG_ERROR, "Unknown video stream format\n");
			goto SKIP;
		}
	//end of video stream.
	}else if(strstr(rmst->mimet, "logical-")){
		 	if(strstr(rmst->mimet, "fileinfo")) {
		     		av_log(NULL, AV_LOG_ERROR, "Got a logical-fileinfo chunk\n");
				goto SKIP;
		 	} else if(strstr(rmst->mimet, "-audio") ||strstr(rmst->mimet, "-video")){
		 		av_log(NULL, AV_LOG_ERROR, "Got a logical-audio/video chunk\n");
			    	int i, stream_cnt;
		    		int stream_list[MAX_RM_STREAMS];
		    
		    		rmctx->is_multirate = 1;
		   		avio_skip(pb, 4); // Length of codec data (repeated)
		    		stream_cnt = avio_rb32(pb); // Get number of audio or video streams
		    		if ((unsigned)stream_cnt >= MAX_RM_STREAMS)
				{
		        		av_log(NULL, AV_LOG_ERROR, "Too many streams in %s. Big troubles ahead.\n", rmst->mimet);
		        		goto SKIP;
		    		}
		    		for (i = 0; i < stream_cnt; i++)
		        		stream_list[i] = avio_rb16(pb);
		    		for (i = 0; i < stream_cnt; i++)
		        		if ((unsigned)stream_list[i] >= MAX_RM_STREAMS) 
					{
		            			av_log(NULL, AV_LOG_ERROR, "Stream id out of range: %d. Ignored.\n", stream_list[i]);
		           			avio_skip(pb, 4); // Skip DATA offset for broken stream
		        		}else{
		            			rmctx->str_data_offset[stream_list[i]] = avio_rb32(pb);
		            			//av_log(NULL, AV_LOG_ERROR, "Stream %d with DATA offset 0x%08x\n", stream_list[i], priv->str_data_offset[stream_list[i]]);
		        		}
		    			// Skip this chunk, for this is actual not a stream
		    			goto SKIP;
		 	} else{
		     		av_log(NULL, AV_LOG_ERROR, "Unknown logical stream\n");
				goto SKIP;
		    	}
	//end of the logical stream
	}else {
		   av_log(NULL, AV_LOG_ERROR, "Not audio/video stream or unsupported!\n");
	}
	//there may be some extra bytes in this MDPR chunk.....
	size =  avio_tell(pb) - rmst->codec_pos;
	avio_skip(pb, rmst->codec_data_size - size);
	return 0;
	
SKIP:
	size =  avio_tell(pb) - rmst->codec_pos;
	avio_skip(pb, rmst->codec_data_size - size);
	ff_rm_free_rmstream_cc(rmctx, rmst);
	//free the RMStream.
	return 1;
}


static void check_rm_av_stream(AVFormatContext* s)
{
	RMDemuxContext* rmctx = s->priv_data;
	RMStream* a_stream = rmctx->audio_index >= 0 ? rmctx->streams[rmctx->audio_index] : NULL;
	RMStream* v_stream = rmctx->video_index >= 0 ? rmctx->streams[rmctx->video_index] : NULL;
	if(rmctx->is_multirate)
	{
        /* Perform some sanity checks to avoid checking streams id all over the code*/
	/* add some other check like a_stream's code_type and so on */
		if(a_stream)
		{
			av_log(NULL, AV_LOG_INFO, "RMDemuxer choose audio_id:%d", a_stream->stream_id);
			if (a_stream->stream_id >= MAX_RM_STREAMS)
			{
            			av_log(NULL, AV_LOG_ERROR, "Invalid audio stream %d. No sound will be played.\n", a_stream->stream_id);
            			rmctx->audio_index = -2;
        		}else if((a_stream->stream_id >= 0) && (rmctx->str_data_offset[a_stream->stream_id] == 0)){
            			av_log(NULL, AV_LOG_ERROR, "Audio stream %d not found. No sound will be played.\n", a_stream->stream_id);
            			rmctx->audio_index = -2;
        		}
		}
		if(v_stream)
		{
			av_log(NULL, AV_LOG_INFO, "RMDemuxer choose video_index:%d", v_stream->stream_id);
			if (v_stream->stream_id >= MAX_RM_STREAMS)
			{
            			av_log(NULL, AV_LOG_ERROR, "Invalid audio stream %d. No sound will be played.\n", v_stream->stream_id);
            			rmctx->video_index = -2;
        		}else if((v_stream->stream_id >= 0) && (rmctx->str_data_offset[v_stream->stream_id] == 0)){
            			av_log(NULL, AV_LOG_ERROR, "Audio stream %d not found. No sound will be played.\n", v_stream->stream_id);
            			rmctx->video_index = -2;
        		}
		}
   	}

}

static void seek_to_data(AVFormatContext* s)
{
	RMDemuxContext* rmctx = s->priv_data;
	RMStream* a_stream = rmctx->audio_index >= 0 ? rmctx->streams[rmctx->audio_index] : NULL;
	RMStream* v_stream = rmctx->video_index >= 0 ? rmctx->streams[rmctx->video_index] : NULL;
	if(rmctx->is_multirate && ((rmctx->video_index >= 0) || (rmctx->audio_index >= 0)))
	{
       		/* If audio or video only, seek to right place and behave like standard file */
       		if(rmctx->video_index < 0)
		{
            		// Stream is audio only, or -novideo
            		rmctx->data_offset = rmctx->str_data_offset[a_stream->stream_id];
            		avio_seek(s->pb, rmctx->data_offset+10, SEEK_SET); /*ffmpeg using +18, i add the 8 bytes in the below. */
            		rmctx->is_multirate = 0;
        	         }
        	         if(rmctx->audio_index < 0)
		{
            		// Stream is video only, or -nosound
            		rmctx->data_offset = rmctx->str_data_offset[v_stream->stream_id];
            		avio_seek(s->pb, rmctx->data_offset+10, SEEK_SET); /* same as above */
            		rmctx->is_multirate = 0;
		}
	}
	if(!rmctx->is_multirate)
	{
		//printf("i=%d num_of_headers=%d   \n",i,num_of_headers);
    		rmctx->nb_packets = avio_rb32(s->pb);
		if (!rmctx->nb_packets && (rmctx->flags & 4))
        		rmctx->nb_packets = 3600 * 25;
		avio_rb32(s->pb); /* next data header */
	}else{
		//TODO: is_multirate and have audio and video.(this may be wrong)
		rmctx->audio_curpos = rmctx->str_data_offset[a_stream->stream_id]+18;
            	avio_seek(s->pb, rmctx->audio_curpos - 8, SEEK_SET); 
		rmctx->a_nb_packets = avio_rb32(s->pb);

		rmctx->video_curpos = rmctx->str_data_offset[v_stream->stream_id]+18;
            	avio_seek(s->pb, rmctx->video_curpos - 8, SEEK_SET); 
		rmctx->v_nb_packets = avio_rb32(s->pb);
		rmctx->stream_switch = 1;
		avio_rb32(s->pb); /* next data header */
	}
}


static int create_av_stream(AVFormatContext* s)
{
	RMDemuxContext* rmctx = s->priv_data;
	if(rmctx->audio_index < 0 && rmctx->video_index < 0)
	{
		//no video and audio
		return -1;
	}

	if(rmctx->audio_index >= 0)
	{
		RMStream* a_stream = rmctx->streams[rmctx->audio_index];
		//we create a new AVStream(audio)
		AVStream* st = av_new_stream(s, a_stream->stream_id);
		av_set_pts_info(st, 64, 1, 1000);
		st->start_time = a_stream->starttime;
		st->duration = a_stream->duration;
		st->priv_data = a_stream;

		//codec
		st->codec->codec_type = a_stream->codec_type;
		st->codec->codec_id = a_stream->codec_id;
		st->codec->bit_rate = a_stream->bitrate;
		st->need_parsing = a_stream->need_parsing;
		if(a_stream->codec_id == CODEC_ID_RA_144)
		{
        		st->codec->sample_rate = 8000;
       			st->codec->channels = 1;
		}else{
        		st->codec->sample_rate = a_stream->sample_rate;
       			st->codec->channels = a_stream->channels;
		}
		if(a_stream->codec_id == CODEC_ID_RA_288)
			st->codec->block_align = a_stream->coded_framesize;
		else if(a_stream->codec_id == CODEC_ID_COOK ||a_stream->codec_id == CODEC_ID_ATRAC3)
			st->codec->block_align = a_stream->sub_packet_size;
		else	
			st->codec->block_align = a_stream->codec_block_align;
		
	        st->codec->extradata_size= a_stream->extradata_size;
		if(st->codec->extradata_size)
		{	
			st->codec->extradata = av_mallocz(st->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
			memcpy(st->codec->extradata, a_stream->extradata, a_stream->extradata_size);
			//av_log(NULL, AV_LOG_INFO, "extra_size: %d\n", st->codec->extradata_size);
			//int i =0;
			//for(;i<st->codec->extradata_size; i++)
				//av_log(NULL, AV_LOG_INFO, "%02x", st->codec->extradata[i]);
		}//
	}
	if(rmctx->video_index >= 0)
	{
		RMStream* v_stream = rmctx->streams[rmctx->video_index];
		AVStream* st = av_new_stream(s, v_stream->stream_id);
		av_set_pts_info(st, 64, 1, 1000);
		st->start_time = v_stream->starttime;
		st->duration = v_stream->duration;
		st->priv_data = v_stream;

		//codec
		st->codec->codec_type = v_stream->codec_type;
		st->codec->codec_id = v_stream->codec_id;
		st->codec->bit_rate = v_stream->bitrate;
       		st->codec->width = v_stream->width;
        	st->codec->height = v_stream->height;
        	st->codec->time_base.num= 1;
        	st->codec->time_base.den = v_stream->v_fps * st->codec->time_base.num;
		
	       st->codec->extradata_size= v_stream->extradata_size;
		if(st->codec->extradata_size)
		{
			st->codec->extradata = av_mallocz(st->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
			memcpy(st->codec->extradata, v_stream->extradata, v_stream->extradata_size);
		}//
	}

	return 0;			
}

static void parse_rm_index(AVFormatContext* s)
{
	RMDemuxContext* rmctx = s->priv_data;
	int64_t oripos = avio_tell(s->pb);
	if(rmctx->index_off)
	{
		avio_seek(s->pb, rmctx->index_off, SEEK_SET);
		rm_read_index(s);
	}
	//todo
	//else{}
        avio_seek(s->pb, oripos, SEEK_SET);
}

AVInputFormat ff_rm_demuxer = {
    "rm",
    NULL_IF_CONFIG_SMALL("RealMedia format"),
    sizeof(RMDemuxContext),
    rm_probe,
    rm_read_header,
    rm_read_packet,
    rm_read_close,
    rm_read_seek,//chuchen,2012_3_20
    rm_read_dts,
};

AVInputFormat ff_rdt_demuxer = {
    "rdt",
    NULL_IF_CONFIG_SMALL("RDT demuxer"),
    sizeof(RMDemuxContext),
    NULL,
    NULL,
    NULL,
    rm_read_close,
};
