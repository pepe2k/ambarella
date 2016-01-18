#if NEW_RTSP_CLIENT

/*
 * RTSP SDP client, copy from ffmpeg/rtsp.h and modified
 */
 #include "rtsp_utils.h"
 extern "C"{
     #include "libavutil/parseutils.h"
     #include "libavformat/internal.h"
 };

#define SPACE_CHARS " \t\r\n"

static void get_word_until_chars(char *buf, int buf_size,
                                 const char *sep, const char **pp)
{
    const char *p;
    char *q;

    p = *pp;
    p += strspn(p, SPACE_CHARS);
    q = buf;
    while (!strchr(sep, *p) && *p != '\0') {
        if ((q - buf) < buf_size - 1)
            *q++ = *p;
        p++;
    }
    if (buf_size > 0)
        *q = '\0';
    *pp = p;
}

static void get_word_sep(char *buf, int buf_size, const char *sep,
                         const char **pp)
{
    if (**pp == '/') (*pp)++;
    get_word_until_chars(buf, buf_size, sep, pp);
}

static void get_word(char *buf, int buf_size, const char **pp)
{
    get_word_until_chars(buf, buf_size, SPACE_CHARS, pp);
}

/**added by liujian,
 *   to fix bug#1409, parse invalid range description , for example ,"a=range:npt:0-67.45600000" etc.
 */
static int parse_npt_time(int64_t *timeval, const char *str)
{
    char buf[32];
    int64_t t;
    int len,i;
    const char *p = str;
    get_word_sep(buf, sizeof(buf), ".", &p);
    len = strlen(buf);
    for(t = 0,i = 0; i < len; i ++){
        if (!isdigit(buf[i]))
            break;
        t = t * 10 +  (buf[i] - '0') * 1000000;
    }
    /* parse the .m... part */
    if (*p == '.') {
        int val, n;
        p++;
        for (val = 0, n = 100000; n >= 1; n /= 10,p++) {
            if (!isdigit(*p))
                break;
            val += n * (*p - '0');
        }
        t += val;
    }
    *timeval = t;
    return 0;
}
static int parse_range_npt2(const char *p, int64_t *start, int64_t *end)
{
    char buf[256];
    *start = AV_NOPTS_VALUE;
    *end = AV_NOPTS_VALUE;
    get_word_sep(buf, sizeof(buf), "-", &p);
    if(parse_npt_time(start, buf) < 0)
        return -1;
    if (*p == '-') {
        p++;
        get_word_sep(buf, sizeof(buf), "-", &p);
        if(parse_npt_time(end, buf) < 0)
            return -1;
    }
    return 0;
}

static int av_stristart(const char *str, const char *pfx, const char **ptr)
{
    while (*pfx && toupper((unsigned)*pfx) == toupper((unsigned)*str)) {
        pfx++;
        str++;
    }
    if (!*pfx && ptr)
        *ptr = str;
    return !*pfx;
}

/** Parse a string p in the form of Range:npt=xx-xx, and determine the start
 *  and end time.
 *  Used for seeking in the rtp stream.
 */
static int rtsp_parse_range_npt(const char *p, int64_t *start, int64_t *end)
{
    char buf[256];

    p += strspn(p, SPACE_CHARS);
    //fix bug#1409, parse invalid range description , for example ,"a=range:npt:0-67.45600000" etc.
    if(av_stristart(p, "npt:", &p))
        return parse_range_npt2(p,start,end);
    if (!av_stristart(p, "npt=", &p))
        return -1;

    *start = AV_NOPTS_VALUE;
    *end = AV_NOPTS_VALUE;

    get_word_sep(buf, sizeof(buf), "-", &p);
    av_parse_time(start, buf, 1);
    if (*p == '-') {
        p++;
        get_word_sep(buf, sizeof(buf), "-", &p);
        av_parse_time(end, buf, 1);
    }
//    av_log(NULL, AV_LOG_DEBUG, "Range Start: %lld\n", *start);
//    av_log(NULL, AV_LOG_DEBUG, "Range End: %lld\n", *end);
    return 0;
}

static void init_rtp_handler(RTPDynamicProtocolHandler *handler,
                             RTSPStream *rtsp_st, AVCodecContext *codec)
{
    if (!handler)
        return;
    codec->codec_id          = handler->codec_id;
    rtsp_st->dynamic_handler = handler;
    if (handler->open){
        rtsp_st->dynamic_protocol_context = handler->open();
        if (!rtsp_st->dynamic_protocol_context)
            rtsp_st->dynamic_handler = NULL;
    }
}

/* parse the rtpmap description: <codec_name>/<clock_rate>[/<other params>] */
static int sdp_parse_rtpmap(AVFormatContext *s,
                            AVStream *st, RTSPStream *rtsp_st,
                            int payload_type, const char *p)
{
    AVCodecContext *codec = st->codec;
    char buf[256];
    int i;
    AVCodec *c;
    const char *c_name;

    /* Loop into AVRtpDynamicPayloadTypes[] and AVRtpPayloadTypes[] and
     * see if we can handle this kind of payload.
     * The space should normally not be there but some Real streams or
     * particular servers ("RealServer Version 6.1.3.970", see issue 1658)
     * have a trailing space. */
    get_word_sep(buf, sizeof(buf), "/ ", &p);
    if (payload_type < RTP_PT_PRIVATE) {
        /* We are in a standard case
         * (from http://www.iana.org/assignments/rtp-parameters). */
        /* search into AVRtpPayloadTypes[] */
        codec->codec_id = ff_rtp_codec_id(buf, codec->codec_type);
    }

    if (codec->codec_id == CODEC_ID_NONE) {
        RTPDynamicProtocolHandler *handler =
            ff_rtp_handler_find_by_name(buf, codec->codec_type);
        init_rtp_handler(handler, rtsp_st, codec);
        /* If no dynamic handler was found, check with the list of standard
         * allocated types, if such a stream for some reason happens to
         * use a private payload type. This isn't handled in rtpdec.c, since
         * the format name from the rtpmap line never is passed into rtpdec. */
        if (!rtsp_st->dynamic_handler)
            codec->codec_id = ff_rtp_codec_id(buf, codec->codec_type);
        /*
        *  added to process G726
        */
        if(codec->codec_id == CODEC_ID_NONE){
           if(strcmp(buf, "G726-16") == 0){ // G.726, 16 kbps
               codec->codec_id = CODEC_ID_ADPCM_G726;
               codec->bit_rate = 16000;
               codec->sample_rate = 8000;
               codec->channels = 1;
           }else if(strcmp(buf, "G726-24") == 0){ // G.726, 24 kbps
               codec->codec_id = CODEC_ID_ADPCM_G726;
               codec->bit_rate = 24000;
               codec->sample_rate = 8000;
               codec->channels = 1;
           }else if(strcmp(buf, "G726-32") == 0){ // G.726, 32 kbps
               codec->codec_id = CODEC_ID_ADPCM_G726;
               codec->bit_rate = 32000;
               codec->sample_rate = 8000;
               codec->channels = 1;
           }else if(strcmp(buf, "G726-40") == 0){ // G.726, 40 kbps
               codec->codec_id = CODEC_ID_ADPCM_G726;
               codec->bit_rate = 40000;
               codec->sample_rate = 8000;
               codec->channels = 1;
           }
        }
    }

    c = avcodec_find_decoder(codec->codec_id);
    if (c && c->name)
        c_name = c->name;
    else
        c_name = "(null)";

    get_word_sep(buf, sizeof(buf), "/", &p);
    i = atoi(buf);
    switch (codec->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        av_log(s, AV_LOG_DEBUG, "audio codec set to: %s\n", c_name);
        codec->sample_rate = RTSP_DEFAULT_AUDIO_SAMPLERATE;
        codec->channels = RTSP_DEFAULT_NB_AUDIO_CHANNELS;
        if (i > 0) {
            codec->sample_rate = i;
            av_set_pts_info(st, 32, 1, codec->sample_rate);
            get_word_sep(buf, sizeof(buf), "/", &p);
            i = atoi(buf);
            if (i > 0)
                codec->channels = i;
            // TODO: there is a bug here; if it is a mono stream, and
            // less than 22000Hz, faad upconverts to stereo and twice
            // the frequency.  No problem, but the sample rate is being
            // set here by the sdp line. Patch on its way. (rdm)
        }
        av_log(s, AV_LOG_DEBUG, "audio samplerate set to: %i\n",
               codec->sample_rate);
        av_log(s, AV_LOG_DEBUG, "audio channels set to: %i\n",
               codec->channels);
        break;
    case AVMEDIA_TYPE_VIDEO:
        av_log(s, AV_LOG_DEBUG, "video codec set to: %s\n", c_name);
        if (i > 0)
            av_set_pts_info(st, 32, 1, i);
        break;
    default:
        break;
    }
    return 0;
}

/* parse the attribute line from the fmtp a line of an sdp response. This
 * is broken out as a function because it is used in rtp_h264.c, which is
 * forthcoming. */
int ff_rtsp_next_attr_and_value1(const char **p, char *attr, int attr_size,
                                char *value, int value_size)
{
    *p += strspn(*p, SPACE_CHARS);
    if (**p) {
        get_word_sep(attr, attr_size, "=", p);
        if (**p == '=')
            (*p)++;
        get_word_sep(value, value_size, ";", p);
        if (**p == ';')
            (*p)++;
        return 1;
    }
    return 0;
}

static size_t av_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = 0;
    while (++len < size && *src)
        *dst++ = *src++;
    if (len <= size)
        *dst = 0;
    return len + strlen(src) - 1;
}

static int av_strstart(const char *str, const char *pfx, const char **ptr)
{
    while (*pfx && *pfx == *str) {
        pfx++;
        str++;
    }
    if (!*pfx && ptr)
        *ptr = str;
    return !*pfx;
}

static size_t av_strlcat(char *dst, const char *src, size_t size)
{
    size_t len = strlen(dst);
    if (size <= len + 1)
        return len + strlen(src);
    return len + av_strlcpy(dst + len, src, size - len);
}


typedef struct SDPParseState {
    /* SDP only */
    struct sockaddr_storage default_ip;
    int            default_ttl;
    int            skip_media;  ///< set if an unknown m= line occurs
} SDPParseState;

static void sdp_parse_line(AVFormatContext *s, SDPParseState *s1,
                           int letter, const char *buf)
{
    RTSPState *rt = (RTSPState *)s->priv_data;
    char buf1[64], st_type[64];
    const char *p;
    enum AVMediaType codec_type;
    int payload_type, i;
    AVStream *st = NULL;
    RTSPStream *rtsp_st;

    av_log(s, AV_LOG_DEBUG, "sdp: %c='%s'\n", letter, buf);

    p = buf;
    if (s1->skip_media && letter != 'm')
        return;
    switch (letter) {
    case 'c':
/*
        get_word(buf1, sizeof(buf1), &p);
        if (strcmp(buf1, "IN") != 0)
            return;
        get_word(buf1, sizeof(buf1), &p);
        if (strcmp(buf1, "IP4") && strcmp(buf1, "IP6"))
            return;
        get_word_sep(buf1, sizeof(buf1), "/", &p);
        if (get_sockaddr(buf1, &sdp_ip))
            return;
        ttl = 16;
        if (*p == '/') {
            p++;
            get_word_sep(buf1, sizeof(buf1), "/", &p);
            ttl = atoi(buf1);
        }
        if (s->nb_streams == 0) {
            s1->default_ip = sdp_ip;
            s1->default_ttl = ttl;
        } else {
            rtsp_st = rt->rtsp_streams[rt->nb_rtsp_streams - 1];
            rtsp_st->sdp_ip = sdp_ip;
            rtsp_st->sdp_ttl = ttl;
        }
*/
        break;
    case 's':
        //av_metadata_set2(&s->metadata, "title", p, 0);
        break;
    case 'i':
        if (s->nb_streams == 0) {
            //av_metadata_set2(&s->metadata, "comment", p, 0);
            break;
        }
        break;
    case 'm':
        /* new stream */
        s1->skip_media = 0;
        codec_type = AVMEDIA_TYPE_UNKNOWN;
        get_word(st_type, sizeof(st_type), &p);
        if (!strcmp(st_type, "audio")) {
            codec_type = AVMEDIA_TYPE_AUDIO;
        } else if (!strcmp(st_type, "video")) {
            codec_type = AVMEDIA_TYPE_VIDEO;
        } else if (!strcmp(st_type, "application")) {
            codec_type = AVMEDIA_TYPE_DATA;
        }
        if (codec_type == AVMEDIA_TYPE_UNKNOWN) {
            s1->skip_media = 1;
            return;
        }
        rtsp_st = (RTSPStream*)av_mallocz(sizeof(RTSPStream));
        if (!rtsp_st)
            return;
        rtsp_st->stream_index = -1;
        dynarray_add(&rt->rtsp_streams, &rt->nb_rtsp_streams, rtsp_st);

        rtsp_st->sdp_ip = s1->default_ip;
        rtsp_st->sdp_ttl = s1->default_ttl;

        get_word(buf1, sizeof(buf1), &p); /* port */
        rtsp_st->sdp_port = atoi(buf1);

        get_word(buf1, sizeof(buf1), &p); /* protocol (ignored) */

        /* XXX: handle list of formats */
        get_word(buf1, sizeof(buf1), &p); /* format list */
        rtsp_st->sdp_payload_type = atoi(buf1);

        if (!strcmp(ff_rtp_enc_name(rtsp_st->sdp_payload_type), "MP2T")) {
            /* no corresponding stream */
        } else if (rt->server_type == RTSP_SERVER_WMS &&
                   codec_type == AVMEDIA_TYPE_DATA) {
            /* RTX stream, a stream that carries all the other actual
             * audio/video streams. Don't expose this to the callers. */
        } else {
            st = av_new_stream(s, rt->nb_rtsp_streams - 1);
            if (!st)
                return;
            rtsp_st->stream_index = st->index;
            st->codec->codec_type = codec_type;
            if (rtsp_st->sdp_payload_type < RTP_PT_PRIVATE) {
                RTPDynamicProtocolHandler *handler;
                /* if standard payload type, we can find the codec right now */
                ff_rtp_get_codec_info(st->codec, rtsp_st->sdp_payload_type);
                if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
                    st->codec->sample_rate > 0)
                    av_set_pts_info(st, 32, 1, st->codec->sample_rate);
                /* Even static payload types may need a custom depacketizer */
                handler = ff_rtp_handler_find_by_id(
                              rtsp_st->sdp_payload_type, st->codec->codec_type);
                init_rtp_handler(handler, rtsp_st, st->codec);
            }
        }
        st->first_dts = 0;
        /* put a default control url */
        av_strlcpy(rtsp_st->control_url, rt->control_uri,
                   sizeof(rtsp_st->control_url));
        break;
    case 'a':
        if (av_strstart(p, "control:", &p)) {
            if (s->nb_streams == 0) {
                if (!strncmp(p, "rtsp://", 7))
                    av_strlcpy(rt->control_uri, p,
                               sizeof(rt->control_uri));
            } else {
                char proto[32];
                /* get the control url */
                rtsp_st = rt->rtsp_streams[rt->nb_rtsp_streams - 1];

                /* XXX: may need to add full url resolution */
                av_url_split(proto, sizeof(proto), NULL, 0, NULL, 0,
                             NULL, NULL, 0, p);
                if (proto[0] == '\0') {
                    /* relative control URL */
                    if (rtsp_st->control_url[strlen(rtsp_st->control_url)-1]!='/')
                    av_strlcat(rtsp_st->control_url, "/",
                               sizeof(rtsp_st->control_url));
                    av_strlcat(rtsp_st->control_url, p,
                               sizeof(rtsp_st->control_url));
                } else
                    av_strlcpy(rtsp_st->control_url, p,
                               sizeof(rtsp_st->control_url));
            }
        } else if (av_strstart(p, "rtpmap:", &p) && s->nb_streams > 0) {
            /* NOTE: rtpmap is only supported AFTER the 'm=' tag */
            get_word(buf1, sizeof(buf1), &p);
            payload_type = atoi(buf1);
            rtsp_st = rt->rtsp_streams[rt->nb_rtsp_streams - 1];
            if (rtsp_st->stream_index >= 0) {
                st = s->streams[rtsp_st->stream_index];
                sdp_parse_rtpmap(s, st, rtsp_st, payload_type, p);
            }
        } else if (av_strstart(p, "fmtp:", &p) ||
                   av_strstart(p, "framesize:", &p) ||
                   av_strstart(p, "framrate:", &p) ) {
            /* NOTE: fmtp is only supported AFTER the 'a=rtpmap:xxx' tag */
            // let dynamic protocol handlers have a stab at the line.
            get_word(buf1, sizeof(buf1), &p);
            payload_type = atoi(buf1);
            for (i = 0; i < rt->nb_rtsp_streams; i++) {
                rtsp_st = rt->rtsp_streams[i];
                if (rtsp_st->sdp_payload_type == payload_type &&
                    rtsp_st->dynamic_handler &&
                    rtsp_st->dynamic_handler->parse_sdp_a_line)
                    rtsp_st->dynamic_handler->parse_sdp_a_line(s, i,
                        rtsp_st->dynamic_protocol_context, buf);
            }
        } else if (av_strstart(p, "range:", &p)) {
            int64_t start, end;

            // this is so that seeking on a streamed file can work.
           if(rtsp_parse_range_npt(p, &start, &end) < 0)
               break;

            s->start_time = start;
            /* AV_NOPTS_VALUE means live broadcast (and can't seek) */
            s->duration   = (end == (int64_t)AV_NOPTS_VALUE) ?
                            AV_NOPTS_VALUE : end - start;
        } else if (av_strstart(p, "SampleRate:integer;", &p) &&
                   s->nb_streams > 0) {
            st = s->streams[s->nb_streams - 1];
            st->codec->sample_rate = atoi(p);
        } else {
            //if (rt->server_type == RTSP_SERVER_WMS)
            //    ff_wms_parse_sdp_a_line(s, p);
            if (s->nb_streams > 0) {
                rtsp_st = rt->rtsp_streams[rt->nb_rtsp_streams - 1];

                //if (rt->server_type == RTSP_SERVER_REAL)
                //    ff_real_parse_sdp_a_line(s, rtsp_st->stream_index, p);

                if (rtsp_st->dynamic_handler &&
                    rtsp_st->dynamic_handler->parse_sdp_a_line)
                    rtsp_st->dynamic_handler->parse_sdp_a_line(s,
                        rtsp_st->stream_index,
                        rtsp_st->dynamic_protocol_context, buf);
            }
        }
        break;
    }
}

/**
 * Parse the sdp description and allocate the rtp streams and the
 * pollfd array used for udp ones.
 */

int rtsp_sdp_parse(AVFormatContext *s, const char *content)
{
    // RTSPState *rt = (RTSPState*)s->priv_data;
    const char *p;
    int letter;
    /* Some SDP lines, particularly for Realmedia or ASF RTSP streams,
     * contain long SDP lines containing complete ASF Headers (several
     * kB) or arrays of MDPR (RM stream descriptor) headers plus
     * "rulebooks" describing their properties. Therefore, the SDP line
     * buffer is large.
     *
     * The Vorbis FMTP line can be up to 16KB - see xiph_parse_sdp_line
     * in rtpdec_xiph.c. */
    char buf[16384], *q;
    SDPParseState sdp_parse_state, *s1 = &sdp_parse_state;

    memset(s1, 0, sizeof(SDPParseState));
    p = content;
    for (;;) {
        p += strspn(p, SPACE_CHARS);
        letter = *p;
        if (letter == '\0')
            break;
        p++;
        if (*p != '=')
            goto next_line;
        p++;
        /* get the content */
        q = buf;
        while (*p != '\n' && *p != '\r' && *p != '\0') {
            if ((q - buf) < (int)(sizeof(buf) - 1))
                *q++ = *p;
            p++;
        }
        *q = '\0';
        sdp_parse_line(s, s1, letter, buf);
    next_line:
        while (*p != '\n' && *p != '\0')
            p++;
        if (*p == '\n')
            p++;
    }
    return 0;
}

#endif //NEW_RTSP_CLIENT

