/*
 * MOV demuxer
 * Copyright (c) 2001 Fabrice Bellard
 * Copyright (c) 2009 Baptiste Coudurier <baptiste dot coudurier at gmail dot com>
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

#include <limits.h>

//#define DEBUG
//#define DEBUG_METADATA
//#define MOV_EXPORT_ALL_METADATA

#include "libavutil/intreadwrite.h"
#include "libavutil/avstring.h"
#include "avformat.h"
#include "avio_internal.h"
#include "riff.h"
#include "isom.h"
#include "libavcodec/get_bits.h"

#if CONFIG_ZLIB
#include <zlib.h>
#endif

/*
 * First version by Francois Revol revol@free.fr
 * Seek function by Gael Chardon gael.dev@4now.net
 *
 * Features and limitations:
 * - reads most of the QT files I have (at least the structure),
 *   Sample QuickTime files with mp3 audio can be found at: http://www.3ivx.com/showcase.html
 * - the code is quite ugly... maybe I won't do it recursive next time :-)
 *
 * Funny I didn't know about http://sourceforge.net/projects/qt-ffmpeg/
 * when coding this :) (it's a writer anyway)
 *
 * Reference documents:
 * http://www.geocities.com/xhelmboyx/quicktime/formats/qtm-layout.txt
 * Apple:
 *  http://developer.apple.com/documentation/QuickTime/QTFF/
 *  http://developer.apple.com/documentation/QuickTime/QTFF/qtff.pdf
 * QuickTime is a trademark of Apple (AFAIK :))
 */

#include "qtpalette.h"


#undef NDEBUG
#include <assert.h>
#include <semaphore.h>

#define KB 1024
#define MB (1024*1024)

#define DEFAULT_LESS_FREE_THRESHOLD     (256*KB)
#define DEFAULT_BASE_MOVE_OFFSET        (1*MB)

// #define LOW_BITRATE_THRESHOLD            (0<10)   // Mbps
#define LOW_BITRATE_NET_IO_BUFFER_SIZE      (64*MB)
#define LOW_BITRATE_SHORT_SEEK_THRESHOLD    (4*MB)
#define LOW_BITRATE_PRE_CACHE_DATA_SIZE     (1*MB)
#define LOW_BITRATE_ING_CACHE_DATA_SIZE     (1*MB)
#define LOW_BITRATE_LESS_DATA_THRESHOLD     (256*KB)

#define MID_BITRATE_THRESHOLD               12       // Mbps
#define MID_BITRATE_NET_IO_BUFFER_SIZE      (64*MB)
#define MID_BITRATE_SHORT_SEEK_THRESHOLD    (4*MB)
#define MID_BITRATE_PRE_CACHE_DATA_SIZE     (16*MB)
#define MID_BITRATE_ING_CACHE_DATA_SIZE     (16*MB)
#define MID_BITRATE_LESS_DATA_THRESHOLD     (512*KB)

#define HIGH_BITRATE_THRESHOLD              15       // Mbps
#define HIGH_BITRATE_IO_BUFFER_SIZE         (64*MB)
#define HIGH_BITRATE_SHORT_SEEK_THRESHOLD   (4*MB)
#define HIGH_BITRATE_PRE_CACHE_DATA_SIZE    (32*MB)
#define HIGH_BITRATE_ING_CACHE_DATA_SIZE    (32*MB)
#define HIGH_BITRATE_LESS_DATA_THRESHOLD    (512*KB)

// ip4/ipad/ipad2 or LOW_BITRATE streaming needs a single cache_data thread.
#define IP4_NET_IO_BUFFER_SIZE            LOW_BITRATE_NET_IO_BUFFER_SIZE
#define IP4_NET_SHORT_SEEK_THRESHOLD      LOW_BITRATE_SHORT_SEEK_THRESHOLD
#define IP4_NET_PRE_CACHE_DATA_SIZE       LOW_BITRATE_PRE_CACHE_DATA_SIZE
#define IP4_NET_ING_CACHE_DATA_SIZE       LOW_BITRATE_ING_CACHE_DATA_SIZE
#define IP4_NET_LESS_DATA_THRESHOLD       LOW_BITRATE_LESS_DATA_THRESHOLD

// ip4s/ipad3 or MID_BITRATE streaming needs a single pull_data thread.
#define IP4S_NET_IO_BUFFER_SIZE           HIGH_BITRATE_IO_BUFFER_SIZE
#define IP4S_NET_SHORT_SEEK_THRESHOLD     HIGH_BITRATE_SHORT_SEEK_THRESHOLD
#define IP4S_NET_PRE_CACHE_DATA_SIZE      HIGH_BITRATE_PRE_CACHE_DATA_SIZE
#define IP4S_NET_ING_CACHE_DATA_SIZE      HIGH_BITRATE_ING_CACHE_DATA_SIZE
#define IP4S_NET_LESS_DATA_THRESHOLD      HIGH_BITRATE_LESS_DATA_THRESHOLD

#define HTTP_WR_DBG              0
#define HTTP_RD_SEEK_DBG         0
#define HTTP_RD_READ_DBG         0
#define AV_STATUS_DBG            0
#define STATE_DBG                0

#if STATE_DBG
char *g_dbg_state_name[] = {
    "invalid",
    "start",
    "cache",
    "normal",
    "error",
    "eos",
    "end",
};
#endif

typedef enum {
    HTTP_THREAD_REQ_INVALID,

    HTTP_THREAD_REQ_START,
    HTTP_THREAD_REQ_STOP,
    HTTP_THREAD_REQ_READ,
    HTTP_THREAD_REQ_SEEK,

    HTTP_THREAD_REQ_CNT
} HTTP_THREAED_REQ;

typedef enum {
    HTTP_THREAD_START_INVALID,
    HTTP_THREAD_STATE_START,
    HTTP_THREAD_STATE_CACHE,
    HTTP_THREAD_STATE_NORMAL,
    HTTP_THREAD_STATE_ERROR,
    HTTP_THREAD_STATE_EOS,
    HTTP_THREAD_STATE_END,

    HTTP_THREAD_STATE_CNT
} HTTP_THREAD_STATE;

typedef struct {
    HTTP_THREAED_REQ req;
    int64_t          reply;
    int64_t          para1;
    int64_t          para2;
} HTTPThreadReq;

typedef struct {
    pthread_t tid;
    sem_t semReq;
    sem_t semReply;

    pthread_mutex_t buf_lock;
    AVIOContext   *pAVIO;

    int bRun;
    HTTP_THREAD_STATE mState;
    HTTPThreadReq *pReq;

    AVFormatContext *pFmtCtx;
    HTTPThreadCallBack pCB;
    void *pCtx;

    int64_t last_v_pos;
    int64_t last_a_pos;

    int recover_network_time_out;
} HTTPThreadContext;


void init_http_thread(HTTPThreadContext **ppThreadCtx, AVFormatContext *s);
void deint_http_thread(HTTPThreadContext **ppThreadCtx);
static void* HTTPThreadProc(void *p);

int64_t http_thread_send_reply(HTTPThreadContext *pThreadCtx, int64_t reply);
int64_t http_thread_send_request(HTTPThreadContext *pThreadCtx, HTTPThreadReq *pReq);

// for mov demux
int http_av_get_packet(HTTPThreadContext *pThreadCtx, AVIOContext *s, AVPacket *pkt, int size);
int http_avio_read(HTTPThreadContext *pThreadCtx, AVIOContext *s, unsigned char *buf, int size);
int read_http_thread(HTTPThreadContext *pThreadCtx, AVIOContext *s, int size);

int64_t http_av_seek(HTTPThreadContext *pThreadCtx, AVIOContext *s, int64_t offset, int whence);
int64_t http_avio_seek(HTTPThreadContext *pThreadCtx, AVIOContext *s, int64_t offset, int whence);
int64_t seek_http_thread(HTTPThreadContext *pThreadCtx, AVIOContext *s, int64_t offset);

// thread internal
void on_state_start(HTTPThreadContext *pThreadCtx);
void on_state_cache(HTTPThreadContext *pThreadCtx);
void on_state_normal(HTTPThreadContext *pThreadCtx);
void on_state_error(HTTPThreadContext *pThreadCtx);
void on_state_eos(HTTPThreadContext *pThreadCtx);

int http_thread_wait_req(HTTPThreadContext *pThreadCtx);
int http_thread_check_req(HTTPThreadContext *pThreadCtx);
int http_thread_is_req(HTTPThreadContext *pThreadCtx, HTTP_THREAED_REQ req);
int http_thread_handle_req(HTTPThreadContext *pThreadCtx);
int http_thread_start_handler(HTTPThreadContext *pThreadCtx, HTTPThreadReq *pReq);
int http_thread_stop_handler(HTTPThreadContext *pThreadCtx, HTTPThreadReq *pReq);
int http_thread_seek_handler(HTTPThreadContext *pThreadCtx, HTTPThreadReq *pReq);
int http_thread_read_handler(HTTPThreadContext *pThreadCtx, HTTPThreadReq *pReq);
int http_thread_fetch_loop(HTTPThreadContext *pThreadCtx);

// util
int get_buf_data_len(AVIOContext *s);
int get_buf_data_readable_len(AVIOContext *s);
int get_buf_free_len(AVIOContext *s);
int get_buf_free_writable_len(AVIOContext *s);
void inc_buf_rd(AVIOContext *s, int len);
void inc_buf_wr(AVIOContext *s, int len);
void inc_buf_base(AVIOContext *s, int64_t len);
void reset_buf_pos(AVIOContext *s, int64_t pos);

typedef struct {
    // static
    int buf_size_update;
    int buf_size;

    int seek_threshold_update;
    int seek_threshold;

    int less_data_size_update;
    int less_data_size;

    int less_free_size_update;
    int less_free_size;

    // dynamic
    int cache_data_size_update;
    int cache_data_size;
} BufParams;

void async_notify_ap(HTTPThreadContext *pThreadCtx, int msg);
int init_params(HTTPThreadContext *pThreadCtx);
int update_buf_cache_size(HTTPThreadContext *pThreadCtx);
int config_buf_params(AVIOContext *s, const BufParams *pBufParams);

/* XXX: it's the first time I make a recursive parser I think... sorry if it's ugly :P */

/* those functions parse an atom */
/* return code:
  0: continue to parse next atom
 <0: error occurred, exit
*/
/* links atom IDs to parse functions */
typedef struct MOVParseTableEntry {
    uint32_t type;
    int (*parse)(MOVContext *ctx, AVIOContext *pb, MOVAtom atom);
} MOVParseTableEntry;

static const MOVParseTableEntry mov_default_parse_table[];

static int mov_metadata_trkn(MOVContext *c, AVIOContext *pb, unsigned len)
{
    char buf[16];

    avio_rb16(pb); // unknown
    snprintf(buf, sizeof(buf), "%d", avio_rb16(pb));
    av_metadata_set2(&c->fc->metadata, "track", buf, 0);

    avio_rb16(pb); // total tracks

    return 0;
}

static const uint32_t mac_to_unicode[128] = {
    0x00C4,0x00C5,0x00C7,0x00C9,0x00D1,0x00D6,0x00DC,0x00E1,
    0x00E0,0x00E2,0x00E4,0x00E3,0x00E5,0x00E7,0x00E9,0x00E8,
    0x00EA,0x00EB,0x00ED,0x00EC,0x00EE,0x00EF,0x00F1,0x00F3,
    0x00F2,0x00F4,0x00F6,0x00F5,0x00FA,0x00F9,0x00FB,0x00FC,
    0x2020,0x00B0,0x00A2,0x00A3,0x00A7,0x2022,0x00B6,0x00DF,
    0x00AE,0x00A9,0x2122,0x00B4,0x00A8,0x2260,0x00C6,0x00D8,
    0x221E,0x00B1,0x2264,0x2265,0x00A5,0x00B5,0x2202,0x2211,
    0x220F,0x03C0,0x222B,0x00AA,0x00BA,0x03A9,0x00E6,0x00F8,
    0x00BF,0x00A1,0x00AC,0x221A,0x0192,0x2248,0x2206,0x00AB,
    0x00BB,0x2026,0x00A0,0x00C0,0x00C3,0x00D5,0x0152,0x0153,
    0x2013,0x2014,0x201C,0x201D,0x2018,0x2019,0x00F7,0x25CA,
    0x00FF,0x0178,0x2044,0x20AC,0x2039,0x203A,0xFB01,0xFB02,
    0x2021,0x00B7,0x201A,0x201E,0x2030,0x00C2,0x00CA,0x00C1,
    0x00CB,0x00C8,0x00CD,0x00CE,0x00CF,0x00CC,0x00D3,0x00D4,
    0xF8FF,0x00D2,0x00DA,0x00DB,0x00D9,0x0131,0x02C6,0x02DC,
    0x00AF,0x02D8,0x02D9,0x02DA,0x00B8,0x02DD,0x02DB,0x02C7,
};

static int mov_read_mac_string(MOVContext *c, AVIOContext *pb, int len,
                               char *dst, int dstlen)
{
    char *p = dst;
    char *end = dst+dstlen-1;
    int i;

    for (i = 0; i < len; i++) {
        uint8_t t, c = avio_r8(pb);
        if (c < 0x80 && p < end)
            *p++ = c;
        else
            PUT_UTF8(mac_to_unicode[c-0x80], t, if (p < end) *p++ = t;);
    }
    *p = 0;
    return p - dst;
}

static int mov_read_udta_string(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
#ifdef MOV_EXPORT_ALL_METADATA
    char tmp_key[5];
#endif
    char str[1024], key2[16], language[4] = {0};
    const char *key = NULL;
    uint16_t str_size, langcode = 0;
    uint32_t data_type = 0;
    int (*parse)(MOVContext*, AVIOContext*, unsigned) = NULL;

    switch (atom.type) {
    case MKTAG(0xa9,'n','a','m'): key = "title";     break;
    case MKTAG(0xa9,'a','u','t'):
    case MKTAG(0xa9,'A','R','T'): key = "artist";    break;
    case MKTAG(0xa9,'w','r','t'): key = "composer";  break;
    case MKTAG( 'c','p','r','t'):
    case MKTAG(0xa9,'c','p','y'): key = "copyright"; break;
    case MKTAG(0xa9,'c','m','t'):
    case MKTAG(0xa9,'i','n','f'): key = "comment";   break;
    case MKTAG(0xa9,'a','l','b'): key = "album";     break;
    case MKTAG(0xa9,'d','a','y'): key = "date";      break;
    case MKTAG(0xa9,'g','e','n'): key = "genre";     break;
    case MKTAG(0xa9,'t','o','o'):
    case MKTAG(0xa9,'s','w','r'): key = "encoder";   break;
    case MKTAG(0xa9,'e','n','c'): key = "encoder";   break;
    case MKTAG( 'd','e','s','c'): key = "description";break;
    case MKTAG( 'l','d','e','s'): key = "synopsis";  break;
    case MKTAG( 't','v','s','h'): key = "show";      break;
    case MKTAG( 't','v','e','n'): key = "episode_id";break;
    case MKTAG( 't','v','n','n'): key = "network";   break;
    case MKTAG( 't','r','k','n'): key = "track";
        parse = mov_metadata_trkn; break;
    }

    if (c->itunes_metadata && atom.size > 8) {
        int data_size = avio_rb32(pb);
        int tag = avio_rl32(pb);
        if (tag == MKTAG('d','a','t','a')) {
            data_type = avio_rb32(pb); // type
            avio_rb32(pb); // unknown
            str_size = data_size - 16;
            atom.size -= 16;
        } else return 0;
    } else if (atom.size > 4 && key && !c->itunes_metadata) {
        str_size = avio_rb16(pb); // string length
        langcode = avio_rb16(pb);
        ff_mov_lang_to_iso639(langcode, language);
        atom.size -= 4;
    } else
        str_size = atom.size;

#ifdef MOV_EXPORT_ALL_METADATA
    if (!key) {
        snprintf(tmp_key, 5, "%.4s", (char*)&atom.type);
        key = tmp_key;
    }
#endif

    if (!key)
        return 0;
    if (atom.size < 0)
        return -1;

    str_size = FFMIN3(sizeof(str)-1, str_size, atom.size);

    if (parse)
        parse(c, pb, str_size);
    else {
        if (data_type == 3 || (data_type == 0 && langcode < 0x800)) { // MAC Encoded
            mov_read_mac_string(c, pb, str_size, str, sizeof(str));
        } else {
            avio_read(pb, str, str_size);
            str[str_size] = 0;
        }
        av_metadata_set2(&c->fc->metadata, key, str, 0);
        if (*language && strcmp(language, "und")) {
            snprintf(key2, sizeof(key2), "%s-%s", key, language);
            av_metadata_set2(&c->fc->metadata, key2, str, 0);
        }
    }
#ifdef DEBUG_METADATA
    av_log(c->fc, AV_LOG_DEBUG, "lang \"%3s\" ", language);
    av_log(c->fc, AV_LOG_DEBUG, "tag \"%s\" value \"%s\" atom \"%.4s\" %d %lld\n",
           key, str, (char*)&atom.type, str_size, atom.size);
#endif

    return 0;
}

static int mov_read_chpl(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int64_t start;
    int i, nb_chapters, str_len, version;
    char str[256+1];

    if ((atom.size -= 5) < 0)
        return 0;

    version = avio_r8(pb);
    avio_rb24(pb);
    if (version)
        avio_rb32(pb); // ???
    nb_chapters = avio_r8(pb);

    for (i = 0; i < nb_chapters; i++) {
        if (atom.size < 9)
            return 0;

        start = avio_rb64(pb);
        str_len = avio_r8(pb);

        if ((atom.size -= 9+str_len) < 0)
            return 0;

        avio_read(pb, str, str_len);
        str[str_len] = 0;
        ff_new_chapter(c->fc, i, (AVRational){1,10000000}, start, AV_NOPTS_VALUE, str);
    }
    return 0;
}

static int mov_read_default(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int64_t total_size = 0;
    MOVAtom a;
    int i;

    if (atom.size < 0)
        atom.size = INT64_MAX;
    while (total_size + 8 < atom.size && !url_feof(pb)) {
        int (*parse)(MOVContext*, AVIOContext*, MOVAtom) = NULL;
        a.size = atom.size;
        a.type=0;
        if(atom.size >= 8) {
            a.size = avio_rb32(pb);
            a.type = avio_rl32(pb);
        }
        av_dlog(c->fc, "type: %08x '%.4s' parent:'%.4s' sz: %"PRId64" %"PRId64" %"PRId64"\n",
                a.type, (char*)&a.type, (char*)&atom.type, a.size, total_size, atom.size);
        total_size += 8;
        if (a.size == 1) { /* 64 bit extended size */
            a.size = avio_rb64(pb) - 8;
            total_size += 8;
        }
        if (a.size == 0) {
            a.size = atom.size - total_size;
            if (a.size <= 8)
                break;
        }
        a.size -= 8;
        if(a.size < 0)
            break;
        a.size = FFMIN(a.size, atom.size - total_size);

        for (i = 0; mov_default_parse_table[i].type; i++)
            if (mov_default_parse_table[i].type == a.type) {
                parse = mov_default_parse_table[i].parse;
                break;
            }

        // container is user data
        if (!parse && (atom.type == MKTAG('u','d','t','a') ||
                       atom.type == MKTAG('i','l','s','t')))
            parse = mov_read_udta_string;

        if (!parse) { /* skip leaf atoms data */
            avio_skip(pb, a.size);
        } else {
            int64_t start_pos = avio_tell(pb);
            int64_t left;
            int err = parse(c, pb, a);
            if (err < 0)
                return err;
            if (c->found_moov && c->found_mdat &&
                (!pb->seekable || start_pos + a.size == avio_size(pb)))
                return 0;
            left = a.size - avio_tell(pb) + start_pos;
            if (left > 0) /* skip garbage at atom end */
                avio_skip(pb, left);
        }

        total_size += a.size;
    }

    if (total_size < atom.size && atom.size < 0x7ffff)
        avio_skip(pb, atom.size - total_size);

    return 0;
}

static int mov_read_dref(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    int entries, i, j;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_rb32(pb); // version + flags
    entries = avio_rb32(pb);
    if (entries >= UINT_MAX / sizeof(*sc->drefs))
        return -1;
    sc->drefs = av_mallocz(entries * sizeof(*sc->drefs));
    if (!sc->drefs)
        return AVERROR(ENOMEM);
    sc->drefs_count = entries;

    for (i = 0; i < sc->drefs_count; i++) {
        MOVDref *dref = &sc->drefs[i];
        uint32_t size = avio_rb32(pb);
        int64_t next = avio_tell(pb) + size - 4;

        if (size < 12)
            return -1;

        dref->type = avio_rl32(pb);
        avio_rb32(pb); // version + flags
        av_dlog(c->fc, "type %.4s size %d\n", (char*)&dref->type, size);

        if (dref->type == MKTAG('a','l','i','s') && size > 150) {
            /* macintosh alias record */
            uint16_t volume_len, len;
            int16_t type;

            avio_skip(pb, 10);

            volume_len = avio_r8(pb);
            volume_len = FFMIN(volume_len, 27);
            avio_read(pb, dref->volume, 27);
            dref->volume[volume_len] = 0;
            av_log(c->fc, AV_LOG_DEBUG, "volume %s, len %d\n", dref->volume, volume_len);

            avio_skip(pb, 12);

            len = avio_r8(pb);
            len = FFMIN(len, 63);
            avio_read(pb, dref->filename, 63);
            dref->filename[len] = 0;
            av_log(c->fc, AV_LOG_DEBUG, "filename %s, len %d\n", dref->filename, len);

            avio_skip(pb, 16);

            /* read next level up_from_alias/down_to_target */
            dref->nlvl_from = avio_rb16(pb);
            dref->nlvl_to   = avio_rb16(pb);
            av_log(c->fc, AV_LOG_DEBUG, "nlvl from %d, nlvl to %d\n",
                   dref->nlvl_from, dref->nlvl_to);

            avio_skip(pb, 16);

            for (type = 0; type != -1 && avio_tell(pb) < next; ) {
                type = avio_rb16(pb);
                len = avio_rb16(pb);
                av_log(c->fc, AV_LOG_DEBUG, "type %d, len %d\n", type, len);
                if (len&1)
                    len += 1;
                if (type == 2) { // absolute path
                    av_free(dref->path);
                    dref->path = av_mallocz(len+1);
                    if (!dref->path)
                        return AVERROR(ENOMEM);
                    avio_read(pb, dref->path, len);
                    if (len > volume_len && !strncmp(dref->path, dref->volume, volume_len)) {
                        len -= volume_len;
                        memmove(dref->path, dref->path+volume_len, len);
                        dref->path[len] = 0;
                    }
                    for (j = 0; j < len; j++)
                        if (dref->path[j] == ':')
                            dref->path[j] = '/';
                    av_log(c->fc, AV_LOG_DEBUG, "path %s\n", dref->path);
                } else if (type == 0) { // directory name
                    av_free(dref->dir);
                    dref->dir = av_malloc(len+1);
                    if (!dref->dir)
                        return AVERROR(ENOMEM);
                    avio_read(pb, dref->dir, len);
                    dref->dir[len] = 0;
                    for (j = 0; j < len; j++)
                        if (dref->dir[j] == ':')
                            dref->dir[j] = '/';
                    av_log(c->fc, AV_LOG_DEBUG, "dir %s\n", dref->dir);
                } else
                    avio_skip(pb, len);
            }
        }
        avio_seek(pb, next, SEEK_SET);
    }
    return 0;
}

static int mov_read_hdlr(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    uint32_t type;
    uint32_t ctype;

    if (c->fc->nb_streams < 1) // meta before first trak
        return 0;

    st = c->fc->streams[c->fc->nb_streams-1];

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    /* component type */
    ctype = avio_rl32(pb);
    type = avio_rl32(pb); /* component subtype */

    av_dlog(c->fc, "ctype= %.4s (0x%08x)\n", (char*)&ctype, ctype);
    av_dlog(c->fc, "stype= %.4s\n", (char*)&type);

    if     (type == MKTAG('v','i','d','e'))
        st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    else if(type == MKTAG('s','o','u','n'))
        st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    else if(type == MKTAG('m','1','a',' '))
        st->codec->codec_id = CODEC_ID_MP2;
    else if(type == MKTAG('s','u','b','p'))
        st->codec->codec_type = AVMEDIA_TYPE_SUBTITLE;

    avio_rb32(pb); /* component  manufacture */
    avio_rb32(pb); /* component flags */
    avio_rb32(pb); /* component flags mask */

    return 0;
}

int ff_mov_read_esds(AVFormatContext *fc, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    int tag, len;

    if (fc->nb_streams < 1)
        return 0;
    st = fc->streams[fc->nb_streams-1];

    avio_rb32(pb); /* version + flags */
    len = ff_mp4_read_descr(fc, pb, &tag);
    if (tag == MP4ESDescrTag) {
        avio_rb16(pb); /* ID */
        avio_r8(pb); /* priority */
    } else
        avio_rb16(pb); /* ID */

    len = ff_mp4_read_descr(fc, pb, &tag);
    if (tag == MP4DecConfigDescrTag)
        ff_mp4_read_dec_config_descr(fc, st, pb);
    return 0;
}

static int mov_read_esds(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    return ff_mov_read_esds(c->fc, pb, atom);
}

static int mov_read_dac3(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    int ac3info, acmod, lfeon, bsmod;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];

    ac3info = avio_rb24(pb);
    bsmod = (ac3info >> 14) & 0x7;
    acmod = (ac3info >> 11) & 0x7;
    lfeon = (ac3info >> 10) & 0x1;
    st->codec->channels = ((int[]){2,1,2,3,3,4,4,5})[acmod] + lfeon;
    st->codec->audio_service_type = bsmod;
    if (st->codec->channels > 1 && bsmod == 0x7)
        st->codec->audio_service_type = AV_AUDIO_SERVICE_TYPE_KARAOKE;

    return 0;
}

static int mov_read_pasp(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    const int num = avio_rb32(pb);
    const int den = avio_rb32(pb);
    AVStream *st;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];

    if ((st->sample_aspect_ratio.den != 1 || st->sample_aspect_ratio.num) && // default
        (den != st->sample_aspect_ratio.den || num != st->sample_aspect_ratio.num)) {
        av_log(c->fc, AV_LOG_WARNING,
               "sample aspect ratio already set to %d:%d, ignoring 'pasp' atom (%d:%d)\n",
               st->sample_aspect_ratio.num, st->sample_aspect_ratio.den,
               num, den);
    } else if (den != 0) {
        st->sample_aspect_ratio.num = num;
        st->sample_aspect_ratio.den = den;
    }
    return 0;
}

/* this atom contains actual media data */
static int mov_read_mdat(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    if(atom.size == 0) /* wrong one (MP4) */
        return 0;
    c->found_mdat=1;
    return 0; /* now go for moov */
}

/* read major brand, minor version and compatible brands and store them as metadata */
static int mov_read_ftyp(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    uint32_t minor_ver;
    int comp_brand_size;
    char minor_ver_str[11]; /* 32 bit integer -> 10 digits + null */
    char* comp_brands_str;
    uint8_t type[5] = {0};

    avio_read(pb, type, 4);
    if (strcmp(type, "qt  "))
        c->isom = 1;
    av_log(c->fc, AV_LOG_DEBUG, "ISO: File Type Major Brand: %.4s\n",(char *)&type);
    av_metadata_set2(&c->fc->metadata, "major_brand", type, 0);
    minor_ver = avio_rb32(pb); /* minor version */
    snprintf(minor_ver_str, sizeof(minor_ver_str), "%d", minor_ver);
    av_metadata_set2(&c->fc->metadata, "minor_version", minor_ver_str, 0);

    comp_brand_size = atom.size - 8;
    if (comp_brand_size < 0)
        return -1;
    comp_brands_str = av_malloc(comp_brand_size + 1); /* Add null terminator */
    if (!comp_brands_str)
        return AVERROR(ENOMEM);
    avio_read(pb, comp_brands_str, comp_brand_size);
    comp_brands_str[comp_brand_size] = 0;
    av_metadata_set2(&c->fc->metadata, "compatible_brands", comp_brands_str, 0);
    av_freep(&comp_brands_str);

    return 0;
}

/* this atom should contain all header atoms */
static int mov_read_moov(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    if (mov_read_default(c, pb, atom) < 0)
        return -1;
    /* we parsed the 'moov' atom, we can terminate the parsing as soon as we find the 'mdat' */
    /* so we don't parse the whole file if over a network */
    c->found_moov=1;
    return 0; /* now go for mdat */
}

static int mov_read_moof(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    c->fragment.moof_offset = avio_tell(pb) - 8;
    av_dlog(c->fc, "moof offset %llx\n", c->fragment.moof_offset);
    return mov_read_default(c, pb, atom);
}

static void mov_metadata_creation_time(AVMetadata **metadata, time_t time)
{
    char buffer[32];
    if (time) {
        struct tm *ptm;
        time -= 2082844800;  /* seconds between 1904-01-01 and Epoch */
        ptm = gmtime(&time);
        if (!ptm) return;
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ptm);
        av_metadata_set2(metadata, "creation_time", buffer, 0);
    }
}

static int mov_read_mdhd(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    int version;
    char language[4] = {0};
    unsigned lang;
    time_t creation_time;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    version = avio_r8(pb);
    if (version > 1)
        return -1; /* unsupported */

    avio_rb24(pb); /* flags */
    if (version == 1) {
        creation_time = avio_rb64(pb);
        avio_rb64(pb);
    } else {
        creation_time = avio_rb32(pb);
        avio_rb32(pb); /* modification time */
    }
    mov_metadata_creation_time(&st->metadata, creation_time);

    sc->time_scale = avio_rb32(pb);
    st->duration = (version == 1) ? avio_rb64(pb) : avio_rb32(pb); /* duration */

    lang = avio_rb16(pb); /* language */
    if (ff_mov_lang_to_iso639(lang, language))
        av_metadata_set2(&st->metadata, "language", language, 0);
    avio_rb16(pb); /* quality */

    return 0;
}

static int mov_read_mvhd(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    time_t creation_time;
    int version = avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    if (version == 1) {
        creation_time = avio_rb64(pb);
        avio_rb64(pb);
    } else {
        creation_time = avio_rb32(pb);
        avio_rb32(pb); /* modification time */
    }
    mov_metadata_creation_time(&c->fc->metadata, creation_time);
    c->time_scale = avio_rb32(pb); /* time scale */

    av_dlog(c->fc, "time scale = %i\n", c->time_scale);

    c->duration = (version == 1) ? avio_rb64(pb) : avio_rb32(pb); /* duration */
    avio_rb32(pb); /* preferred scale */

    avio_rb16(pb); /* preferred volume */

    avio_skip(pb, 10); /* reserved */

    avio_skip(pb, 36); /* display matrix */

    avio_rb32(pb); /* preview time */
    avio_rb32(pb); /* preview duration */
    avio_rb32(pb); /* poster time */
    avio_rb32(pb); /* selection time */
    avio_rb32(pb); /* selection duration */
    avio_rb32(pb); /* current time */
    avio_rb32(pb); /* next track ID */

    return 0;
}

static int mov_read_smi(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];

    if((uint64_t)atom.size > (1<<30))
        return -1;

    // currently SVQ3 decoder expect full STSD header - so let's fake it
    // this should be fixed and just SMI header should be passed
    av_free(st->codec->extradata);
    st->codec->extradata = av_mallocz(atom.size + 0x5a + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!st->codec->extradata)
        return AVERROR(ENOMEM);
    st->codec->extradata_size = 0x5a + atom.size;
    memcpy(st->codec->extradata, "SVQ3", 4); // fake
    avio_read(pb, st->codec->extradata + 0x5a, atom.size);
    av_dlog(c->fc, "Reading SMI %"PRId64"  %s\n", atom.size, st->codec->extradata + 0x5a);
    return 0;
}

static int mov_read_enda(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    int little_endian;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];

    little_endian = avio_rb16(pb);
    av_dlog(c->fc, "enda %d\n", little_endian);
    if (little_endian == 1) {
        switch (st->codec->codec_id) {
        case CODEC_ID_PCM_S24BE:
            st->codec->codec_id = CODEC_ID_PCM_S24LE;
            break;
        case CODEC_ID_PCM_S32BE:
            st->codec->codec_id = CODEC_ID_PCM_S32LE;
            break;
        case CODEC_ID_PCM_F32BE:
            st->codec->codec_id = CODEC_ID_PCM_F32LE;
            break;
        case CODEC_ID_PCM_F64BE:
            st->codec->codec_id = CODEC_ID_PCM_F64LE;
            break;
        default:
            break;
        }
    }
    return 0;
}

/* FIXME modify qdm2/svq3/h264 decoders to take full atom as extradata */
static int mov_read_extradata(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    uint64_t size;
    uint8_t *buf;

    if (c->fc->nb_streams < 1) // will happen with jp2 files
        return 0;
    st= c->fc->streams[c->fc->nb_streams-1];
    size= (uint64_t)st->codec->extradata_size + atom.size + 8 + FF_INPUT_BUFFER_PADDING_SIZE;
    if(size > INT_MAX || (uint64_t)atom.size > INT_MAX)
        return -1;
    buf= av_realloc(st->codec->extradata, size);
    if(!buf)
        return -1;
    st->codec->extradata= buf;
    buf+= st->codec->extradata_size;
    st->codec->extradata_size= size - FF_INPUT_BUFFER_PADDING_SIZE;
    AV_WB32(       buf    , atom.size + 8);
    AV_WL32(       buf + 4, atom.type);
    avio_read(pb, buf + 8, atom.size);
    return 0;
}

static int mov_read_wave(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];

    if((uint64_t)atom.size > (1<<30))
        return -1;

    if (st->codec->codec_id == CODEC_ID_QDM2 || st->codec->codec_id == CODEC_ID_QDMC) {
        // pass all frma atom to codec, needed at least for QDMC and QDM2
        av_free(st->codec->extradata);
        st->codec->extradata = av_mallocz(atom.size + FF_INPUT_BUFFER_PADDING_SIZE);
        if (!st->codec->extradata)
            return AVERROR(ENOMEM);
        st->codec->extradata_size = atom.size;
        avio_read(pb, st->codec->extradata, atom.size);
    } else if (atom.size > 8) { /* to read frma, esds atoms */
        if (mov_read_default(c, pb, atom) < 0)
            return -1;
    } else
        avio_skip(pb, atom.size);
    return 0;
}

/**
 * This function reads atom content and puts data in extradata without tag
 * nor size unlike mov_read_extradata.
 */
static int mov_read_glbl(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];

    if((uint64_t)atom.size > (1<<30))
        return -1;

    av_free(st->codec->extradata);
    st->codec->extradata = av_mallocz(atom.size + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!st->codec->extradata)
        return AVERROR(ENOMEM);
    st->codec->extradata_size = atom.size;
    avio_read(pb, st->codec->extradata, atom.size);
    return 0;
}

/**
 * An strf atom is a BITMAPINFOHEADER struct. This struct is 40 bytes itself,
 * but can have extradata appended at the end after the 40 bytes belonging
 * to the struct.
 */
static int mov_read_strf(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;

    if (c->fc->nb_streams < 1)
        return 0;
    if (atom.size <= 40)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];

    if((uint64_t)atom.size > (1<<30))
        return -1;

    av_free(st->codec->extradata);
    st->codec->extradata = av_mallocz(atom.size - 40 + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!st->codec->extradata)
        return AVERROR(ENOMEM);
    st->codec->extradata_size = atom.size - 40;
    avio_skip(pb, 40);
    avio_read(pb, st->codec->extradata, atom.size - 40);
    return 0;
}

static int mov_read_stco(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    entries = avio_rb32(pb);

    if(entries >= UINT_MAX/sizeof(int64_t))
        return -1;

    sc->chunk_offsets = av_malloc(entries * sizeof(int64_t));
    if (!sc->chunk_offsets)
        return AVERROR(ENOMEM);
    sc->chunk_count = entries;

    if      (atom.type == MKTAG('s','t','c','o'))
        for(i=0; i<entries; i++)
            sc->chunk_offsets[i] = avio_rb32(pb);
    else if (atom.type == MKTAG('c','o','6','4'))
        for(i=0; i<entries; i++)
            sc->chunk_offsets[i] = avio_rb64(pb);
    else
        return -1;

    return 0;
}

/**
 * Compute codec id for 'lpcm' tag.
 * See CoreAudioTypes and AudioStreamBasicDescription at Apple.
 */
enum CodecID ff_mov_get_lpcm_codec_id(int bps, int flags)
{
    if (flags & 1) { // floating point
        if (flags & 2) { // big endian
            if      (bps == 32) return CODEC_ID_PCM_F32BE;
            else if (bps == 64) return CODEC_ID_PCM_F64BE;
        } else {
            if      (bps == 32) return CODEC_ID_PCM_F32LE;
            else if (bps == 64) return CODEC_ID_PCM_F64LE;
        }
    } else {
        if (flags & 2) {
            if      (bps == 8)
                // signed integer
                if (flags & 4)  return CODEC_ID_PCM_S8;
                else            return CODEC_ID_PCM_U8;
            else if (bps == 16) return CODEC_ID_PCM_S16BE;
            else if (bps == 24) return CODEC_ID_PCM_S24BE;
            else if (bps == 32) return CODEC_ID_PCM_S32BE;
        } else {
            if      (bps == 8)
                if (flags & 4)  return CODEC_ID_PCM_S8;
                else            return CODEC_ID_PCM_U8;
            else if (bps == 16) return CODEC_ID_PCM_S16LE;
            else if (bps == 24) return CODEC_ID_PCM_S24LE;
            else if (bps == 32) return CODEC_ID_PCM_S32LE;
        }
    }
    return CODEC_ID_NONE;
}

int ff_mov_read_stsd_entries(MOVContext *c, AVIOContext *pb, int entries)
{
    AVStream *st;
    MOVStreamContext *sc;
    int j, pseudo_stream_id;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    for(pseudo_stream_id=0; pseudo_stream_id<entries; pseudo_stream_id++) {
        //Parsing Sample description table
        enum CodecID id;
        int dref_id = 1;
        MOVAtom a = { 0 };
        int64_t start_pos = avio_tell(pb);
        int size = avio_rb32(pb); /* size */
        uint32_t format = avio_rl32(pb); /* data format */

        if (size >= 16) {
            avio_rb32(pb); /* reserved */
            avio_rb16(pb); /* reserved */
            dref_id = avio_rb16(pb);
        }

        if (st->codec->codec_tag &&
            st->codec->codec_tag != format &&
            (c->fc->video_codec_id ? ff_codec_get_id(codec_movvideo_tags, format) != c->fc->video_codec_id
                                   : st->codec->codec_tag != MKTAG('j','p','e','g'))
           ){
            /* Multiple fourcc, we skip JPEG. This is not correct, we should
             * export it as a separate AVStream but this needs a few changes
             * in the MOV demuxer, patch welcome. */
            av_log(c->fc, AV_LOG_WARNING, "multiple fourcc not supported\n");
            avio_skip(pb, size - (avio_tell(pb) - start_pos));
            continue;
        }
        sc->pseudo_stream_id = st->codec->codec_tag ? -1 : pseudo_stream_id;
        sc->dref_id= dref_id;

        st->codec->codec_tag = format;
        id = ff_codec_get_id(codec_movaudio_tags, format);
        if (id<=0 && ((format&0xFFFF) == 'm'+('s'<<8) || (format&0xFFFF) == 'T'+('S'<<8)))
            id = ff_codec_get_id(ff_codec_wav_tags, av_bswap32(format)&0xFFFF);

        if (st->codec->codec_type != AVMEDIA_TYPE_VIDEO && id > 0) {
            st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
        } else if (st->codec->codec_type != AVMEDIA_TYPE_AUDIO && /* do not overwrite codec type */
                   format && format != MKTAG('m','p','4','s')) { /* skip old asf mpeg4 tag */
            id = ff_codec_get_id(codec_movvideo_tags, format);
            if (id <= 0)
                id = ff_codec_get_id(ff_codec_bmp_tags, format);
            if (id > 0)
                st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
            else if(st->codec->codec_type == AVMEDIA_TYPE_DATA){
                id = ff_codec_get_id(ff_codec_movsubtitle_tags, format);
                if(id > 0)
                    st->codec->codec_type = AVMEDIA_TYPE_SUBTITLE;
            }
        }

        av_dlog(c->fc, "size=%d 4CC= %c%c%c%c codec_type=%d\n", size,
                (format >> 0) & 0xff, (format >> 8) & 0xff, (format >> 16) & 0xff,
                (format >> 24) & 0xff, st->codec->codec_type);

        if(st->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            unsigned int color_depth, len;
            int color_greyscale;

            st->codec->codec_id = id;
            avio_rb16(pb); /* version */
            avio_rb16(pb); /* revision level */
            avio_rb32(pb); /* vendor */
            avio_rb32(pb); /* temporal quality */
            avio_rb32(pb); /* spatial quality */

            st->codec->width = avio_rb16(pb); /* width */
            st->codec->height = avio_rb16(pb); /* height */

            avio_rb32(pb); /* horiz resolution */
            avio_rb32(pb); /* vert resolution */
            avio_rb32(pb); /* data size, always 0 */
            avio_rb16(pb); /* frames per samples */

            len = avio_r8(pb); /* codec name, pascal string */
            if (len > 31)
                len = 31;
            mov_read_mac_string(c, pb, len, st->codec->codec_name, 32);
            if (len < 31)
                avio_skip(pb, 31 - len);
            /* codec_tag YV12 triggers an UV swap in rawdec.c */
            if (!memcmp(st->codec->codec_name, "Planar Y'CbCr 8-bit 4:2:0", 25))
                st->codec->codec_tag=MKTAG('I', '4', '2', '0');

            st->codec->bits_per_coded_sample = avio_rb16(pb); /* depth */
            st->codec->color_table_id = avio_rb16(pb); /* colortable id */
            av_dlog(c->fc, "depth %d, ctab id %d\n",
                   st->codec->bits_per_coded_sample, st->codec->color_table_id);
            /* figure out the palette situation */
            color_depth = st->codec->bits_per_coded_sample & 0x1F;
            color_greyscale = st->codec->bits_per_coded_sample & 0x20;

            /* if the depth is 2, 4, or 8 bpp, file is palettized */
            if ((color_depth == 2) || (color_depth == 4) ||
                (color_depth == 8)) {
                /* for palette traversal */
                unsigned int color_start, color_count, color_end;
                unsigned char r, g, b;

                if (color_greyscale) {
                    int color_index, color_dec;
                    /* compute the greyscale palette */
                    st->codec->bits_per_coded_sample = color_depth;
                    color_count = 1 << color_depth;
                    color_index = 255;
                    color_dec = 256 / (color_count - 1);
                    for (j = 0; j < color_count; j++) {
                        r = g = b = color_index;
                        sc->palette[j] =
                            (r << 16) | (g << 8) | (b);
                        color_index -= color_dec;
                        if (color_index < 0)
                            color_index = 0;
                    }
                } else if (st->codec->color_table_id) {
                    const uint8_t *color_table;
                    /* if flag bit 3 is set, use the default palette */
                    color_count = 1 << color_depth;
                    if (color_depth == 2)
                        color_table = ff_qt_default_palette_4;
                    else if (color_depth == 4)
                        color_table = ff_qt_default_palette_16;
                    else
                        color_table = ff_qt_default_palette_256;

                    for (j = 0; j < color_count; j++) {
                        r = color_table[j * 3 + 0];
                        g = color_table[j * 3 + 1];
                        b = color_table[j * 3 + 2];
                        sc->palette[j] =
                            (r << 16) | (g << 8) | (b);
                    }
                } else {
                    /* load the palette from the file */
                    color_start = avio_rb32(pb);
                    color_count = avio_rb16(pb);
                    color_end = avio_rb16(pb);
                    if ((color_start <= 255) &&
                        (color_end <= 255)) {
                        for (j = color_start; j <= color_end; j++) {
                            /* each R, G, or B component is 16 bits;
                             * only use the top 8 bits; skip alpha bytes
                             * up front */
                            avio_r8(pb);
                            avio_r8(pb);
                            r = avio_r8(pb);
                            avio_r8(pb);
                            g = avio_r8(pb);
                            avio_r8(pb);
                            b = avio_r8(pb);
                            avio_r8(pb);
                            sc->palette[j] =
                                (r << 16) | (g << 8) | (b);
                        }
                    }
                }
                sc->has_palette = 1;
            }
        } else if(st->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
            int bits_per_sample, flags;
            uint16_t version = avio_rb16(pb);

            st->codec->codec_id = id;
            avio_rb16(pb); /* revision level */
            avio_rb32(pb); /* vendor */

            st->codec->channels = avio_rb16(pb);             /* channel count */
            av_dlog(c->fc, "audio channels %d\n", st->codec->channels);
            st->codec->bits_per_coded_sample = avio_rb16(pb);      /* sample size */

            sc->audio_cid = avio_rb16(pb);
            avio_rb16(pb); /* packet size = 0 */

            st->codec->sample_rate = ((avio_rb32(pb) >> 16));

            //Read QT version 1 fields. In version 0 these do not exist.
            av_dlog(c->fc, "version =%d, isom =%d\n",version,c->isom);
            if(!c->isom) {
                if(version==1) {
                    sc->samples_per_frame = avio_rb32(pb);
                    avio_rb32(pb); /* bytes per packet */
                    sc->bytes_per_frame = avio_rb32(pb);
                    avio_rb32(pb); /* bytes per sample */
                } else if(version==2) {
                    avio_rb32(pb); /* sizeof struct only */
                    st->codec->sample_rate = av_int2dbl(avio_rb64(pb)); /* float 64 */
                    st->codec->channels = avio_rb32(pb);
                    avio_rb32(pb); /* always 0x7F000000 */
                    st->codec->bits_per_coded_sample = avio_rb32(pb); /* bits per channel if sound is uncompressed */
                    flags = avio_rb32(pb); /* lpcm format specific flag */
                    sc->bytes_per_frame = avio_rb32(pb); /* bytes per audio packet if constant */
                    sc->samples_per_frame = avio_rb32(pb); /* lpcm frames per audio packet if constant */
                    if (format == MKTAG('l','p','c','m'))
                        st->codec->codec_id = ff_mov_get_lpcm_codec_id(st->codec->bits_per_coded_sample, flags);
                }
            }

            switch (st->codec->codec_id) {
            case CODEC_ID_PCM_S8:
            case CODEC_ID_PCM_U8:
                if (st->codec->bits_per_coded_sample == 16)
                    st->codec->codec_id = CODEC_ID_PCM_S16BE;
                break;
            case CODEC_ID_PCM_S16LE:
            case CODEC_ID_PCM_S16BE:
                if (st->codec->bits_per_coded_sample == 8)
                    st->codec->codec_id = CODEC_ID_PCM_S8;
                else if (st->codec->bits_per_coded_sample == 24)
                    st->codec->codec_id =
                        st->codec->codec_id == CODEC_ID_PCM_S16BE ?
                        CODEC_ID_PCM_S24BE : CODEC_ID_PCM_S24LE;
                break;
            /* set values for old format before stsd version 1 appeared */
            case CODEC_ID_MACE3:
                sc->samples_per_frame = 6;
                sc->bytes_per_frame = 2*st->codec->channels;
                break;
            case CODEC_ID_MACE6:
                sc->samples_per_frame = 6;
                sc->bytes_per_frame = 1*st->codec->channels;
                break;
            case CODEC_ID_ADPCM_IMA_QT:
                sc->samples_per_frame = 64;
                sc->bytes_per_frame = 34*st->codec->channels;
                break;
            case CODEC_ID_GSM:
                sc->samples_per_frame = 160;
                sc->bytes_per_frame = 33;
                break;
            default:
                break;
            }

            bits_per_sample = av_get_bits_per_sample(st->codec->codec_id);
            if (bits_per_sample) {
                st->codec->bits_per_coded_sample = bits_per_sample;
                sc->sample_size = (bits_per_sample >> 3) * st->codec->channels;
            }
        } else if(st->codec->codec_type==AVMEDIA_TYPE_SUBTITLE){
            // ttxt stsd contains display flags, justification, background
            // color, fonts, and default styles, so fake an atom to read it
            MOVAtom fake_atom = { .size = size - (avio_tell(pb) - start_pos) };
            if (format != AV_RL32("mp4s")) // mp4s contains a regular esds atom
                mov_read_glbl(c, pb, fake_atom);
            st->codec->codec_id= id;
            st->codec->width = sc->width;
            st->codec->height = sc->height;
        } else {
            /* other codec type, just skip (rtp, mp4s, tmcd ...) */
            avio_skip(pb, size - (avio_tell(pb) - start_pos));
        }
        /* this will read extra atoms at the end (wave, alac, damr, avcC, SMI ...) */
        a.size = size - (avio_tell(pb) - start_pos);
        if (a.size > 8) {
            if (mov_read_default(c, pb, a) < 0)
                return -1;
        } else if (a.size > 0)
            avio_skip(pb, a.size);
    }

    if(st->codec->codec_type==AVMEDIA_TYPE_AUDIO && st->codec->sample_rate==0 && sc->time_scale>1)
        st->codec->sample_rate= sc->time_scale;

    /* special codec parameters handling */
    switch (st->codec->codec_id) {
#if CONFIG_DV_DEMUXER
    case CODEC_ID_DVAUDIO:
        c->dv_fctx = avformat_alloc_context();
        c->dv_demux = dv_init_demux(c->dv_fctx);
        if (!c->dv_demux) {
            av_log(c->fc, AV_LOG_ERROR, "dv demux context init error\n");
            return -1;
        }
        sc->dv_audio_container = 1;
        st->codec->codec_id = CODEC_ID_PCM_S16LE;
        break;
#endif
    /* no ifdef since parameters are always those */
    case CODEC_ID_QCELP:
        // force sample rate for qcelp when not stored in mov
        if (st->codec->codec_tag != MKTAG('Q','c','l','p'))
            st->codec->sample_rate = 8000;
        st->codec->frame_size= 160;
        st->codec->channels= 1; /* really needed */
        break;
    case CODEC_ID_AMR_NB:
    case CODEC_ID_AMR_WB:
        st->codec->frame_size= sc->samples_per_frame;
        st->codec->channels= 1; /* really needed */
        /* force sample rate for amr, stsd in 3gp does not store sample rate */
        if (st->codec->codec_id == CODEC_ID_AMR_NB)
            st->codec->sample_rate = 8000;
        else if (st->codec->codec_id == CODEC_ID_AMR_WB)
            st->codec->sample_rate = 16000;
        break;
    case CODEC_ID_MP2:
    case CODEC_ID_MP3:
        st->codec->codec_type = AVMEDIA_TYPE_AUDIO; /* force type after stsd for m1a hdlr */
        st->need_parsing = AVSTREAM_PARSE_FULL;
        break;
    case CODEC_ID_GSM:
    case CODEC_ID_ADPCM_MS:
    case CODEC_ID_ADPCM_IMA_WAV:
        st->codec->frame_size = sc->samples_per_frame;
        st->codec->block_align = sc->bytes_per_frame;
        break;
    case CODEC_ID_ALAC:
        if (st->codec->extradata_size == 36) {
            st->codec->frame_size = AV_RB32(st->codec->extradata+12);
            st->codec->channels   = AV_RB8 (st->codec->extradata+21);
            st->codec->sample_rate = AV_RB32(st->codec->extradata+32);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int mov_read_stsd(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int entries;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    entries = avio_rb32(pb);

    return ff_mov_read_stsd_entries(c, pb, entries);
}

static int mov_read_stsc(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    entries = avio_rb32(pb);

    av_dlog(c->fc, "track[%i].stsc.entries = %i\n", c->fc->nb_streams-1, entries);

    if(entries >= UINT_MAX / sizeof(*sc->stsc_data))
        return -1;
    sc->stsc_data = av_malloc(entries * sizeof(*sc->stsc_data));
    if (!sc->stsc_data)
        return AVERROR(ENOMEM);
    sc->stsc_count = entries;

    for(i=0; i<entries; i++) {
        sc->stsc_data[i].first = avio_rb32(pb);
        sc->stsc_data[i].count = avio_rb32(pb);
        sc->stsc_data[i].id = avio_rb32(pb);
    }
    return 0;
}

static int mov_read_stps(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned i, entries;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_rb32(pb); // version + flags

    entries = avio_rb32(pb);
    if (entries >= UINT_MAX / sizeof(*sc->stps_data))
        return -1;
    sc->stps_data = av_malloc(entries * sizeof(*sc->stps_data));
    if (!sc->stps_data)
        return AVERROR(ENOMEM);
    sc->stps_count = entries;

    for (i = 0; i < entries; i++) {
        sc->stps_data[i] = avio_rb32(pb);
        //av_dlog(c->fc, "stps %d\n", sc->stps_data[i]);
    }

    return 0;
}

static int mov_read_stss(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    entries = avio_rb32(pb);

    av_dlog(c->fc, "keyframe_count = %d\n", entries);

    if(entries >= UINT_MAX / sizeof(int))
        return -1;
    sc->keyframes = av_malloc(entries * sizeof(int));
    if (!sc->keyframes)
        return AVERROR(ENOMEM);
    sc->keyframe_count = entries;

    for(i=0; i<entries; i++) {
        sc->keyframes[i] = avio_rb32(pb);
        //av_dlog(c->fc, "keyframes[]=%d\n", sc->keyframes[i]);
    }
    return 0;
}

static int mov_read_stsz(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries, sample_size, field_size, num_bytes;
    GetBitContext gb;
    unsigned char* buf;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    if (atom.type == MKTAG('s','t','s','z')) {
        sample_size = avio_rb32(pb);
        if (!sc->sample_size) /* do not overwrite value computed in stsd */
            sc->sample_size = sample_size;
        field_size = 32;
    } else {
        sample_size = 0;
        avio_rb24(pb); /* reserved */
        field_size = avio_r8(pb);
    }
    entries = avio_rb32(pb);

    av_dlog(c->fc, "sample_size = %d sample_count = %d\n", sc->sample_size, entries);

    sc->sample_count = entries;
    if (sample_size)
        return 0;

    if (field_size != 4 && field_size != 8 && field_size != 16 && field_size != 32) {
        av_log(c->fc, AV_LOG_ERROR, "Invalid sample field size %d\n", field_size);
        return -1;
    }

    if (entries >= UINT_MAX / sizeof(int) || entries >= (UINT_MAX - 4) / field_size)
        return -1;
    sc->sample_sizes = av_malloc(entries * sizeof(int));
    if (!sc->sample_sizes)
        return AVERROR(ENOMEM);

    num_bytes = (entries*field_size+4)>>3;

    buf = av_malloc(num_bytes+FF_INPUT_BUFFER_PADDING_SIZE);
    if (!buf) {
        av_freep(&sc->sample_sizes);
        return AVERROR(ENOMEM);
    }

    if (avio_read(pb, buf, num_bytes) < num_bytes) {
        av_freep(&sc->sample_sizes);
        av_free(buf);
        return -1;
    }

    init_get_bits(&gb, buf, 8*num_bytes);

    for(i=0; i<entries; i++)
        sc->sample_sizes[i] = get_bits_long(&gb, field_size);

    av_free(buf);
    return 0;
}

static int mov_read_stts(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries;
    int64_t duration=0;
    int64_t total_sample_count=0;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    entries = avio_rb32(pb);

    av_dlog(c->fc, "track[%i].stts.entries = %i\n", c->fc->nb_streams-1, entries);

    if(entries >= UINT_MAX / sizeof(*sc->stts_data))
        return -1;
    sc->stts_data = av_malloc(entries * sizeof(*sc->stts_data));
    if (!sc->stts_data)
        return AVERROR(ENOMEM);
    sc->stts_count = entries;

    for(i=0; i<entries; i++) {
        int sample_duration;
        int sample_count;

        sample_count=avio_rb32(pb);
        sample_duration = avio_rb32(pb);
        sc->stts_data[i].count= sample_count;
        sc->stts_data[i].duration= sample_duration;

        av_dlog(c->fc, "sample_count=%d, sample_duration=%d\n",sample_count,sample_duration);

        duration+=(int64_t)sample_duration*sample_count;
        total_sample_count+=sample_count;
    }

    st->nb_frames= total_sample_count;
    if(duration) {
        // fix bug1903:
        // two methods to calculate duration:
        // 1. from mvhd atom, read duration & time_scale, then duration/time_scale
        // 2. from stts atom, get total_sample_count & total_sample_duration,
        // then (total_sample_count*total_sample_duration)/time_scale
        //
        // in bug1903: when inconsitency happens between mvhd and stts, we choose the bigger one
        // jywang@2011-12-20
        if (st->duration == AV_NOPTS_VALUE ||
            duration > st->duration) {
            st->duration= duration;
        }
    }
    return 0;
}

static int mov_read_ctts(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    unsigned int i, entries;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    int sample_tick = 0;
    AVRational r_frame_rate = { 0 };
    if (sc->stts_count == 1 || (sc->stts_count == 2 && sc->stts_data[1].count == 1)) {
        av_reduce(&r_frame_rate.num, &r_frame_rate.den,
                      sc->time_scale, sc->stts_data[0].duration, INT_MAX);
        sample_tick = (st->time_base.den*r_frame_rate.den)/(st->time_base.num*r_frame_rate.num);
    }
    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    entries = avio_rb32(pb);

    av_dlog(c->fc, "track[%i].ctts.entries = %i\n", c->fc->nb_streams-1, entries);

    if(entries >= UINT_MAX / sizeof(*sc->ctts_data))
        return -1;
    sc->ctts_data = av_malloc(entries * sizeof(*sc->ctts_data));
    if (!sc->ctts_data)
        return AVERROR(ENOMEM);
    sc->ctts_count = entries;

    for(i=0; i<entries; i++) {
        int count    =avio_rb32(pb);
        int duration =avio_rb32(pb);

        sc->ctts_data[i].count   = count;
        sc->ctts_data[i].duration= duration;
        if (duration < 0 && i+1<entries)
            sc->dts_shift = FFMAX(sc->dts_shift, -duration);

        // check whether there's error existed in ctts
        // condition: duration_diff between two consecutive frames must be dividec by sample tick
        // add "check stts_data[0].duration" to fix 2103
        if (!sc->ctts_error && (i>0) && (sc->ctts_count>1) && (sample_tick > 0)) {
            if ((sc->ctts_data[i].duration - sc->ctts_data[i-1].duration)%sample_tick != 0
             && (sc->ctts_data[i].duration - sc->ctts_data[i-1].duration)%sc->stts_data[0].duration != 0) {
                sc->ctts_error = 1;
            }
        }
    }

    av_dlog(c->fc, "dts shift %d\n", sc->dts_shift);

    return 0;
}

static void mov_build_index(MOVContext *mov, AVStream *st)
{
    MOVStreamContext *sc = st->priv_data;
    int64_t current_offset;
    int64_t current_dts = 0;
    unsigned int stts_index = 0;
    unsigned int stsc_index = 0;
    unsigned int stss_index = 0;
    unsigned int stps_index = 0;
    unsigned int i, j;
    uint64_t stream_size = 0;

    /* adjust first dts according to edit list */
    if (sc->time_offset && mov->time_scale > 0) {
        int rescaled = sc->time_offset < 0 ? av_rescale(sc->time_offset, sc->time_scale, mov->time_scale) : sc->time_offset;
        current_dts = -rescaled;
        if (sc->ctts_data && sc->stts_data &&
            sc->stts_data[0].duration &&    /*added by liujian, to avoid div_0*/
            sc->ctts_data[0].duration / sc->stts_data[0].duration > 16) {
            /* more than 16 frames delay, dts are likely wrong
               this happens with files created by iMovie */
            sc->wrong_dts = 1;
            st->codec->has_b_frames = 1;
        }
    }

    /* only use old uncompressed audio chunk demuxing when stts specifies it */
    if (!(st->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
          sc->stts_count == 1 && sc->stts_data[0].duration == 1)) {
        unsigned int current_sample = 0;
        unsigned int stts_sample = 0;
        unsigned int sample_size;
        unsigned int distance = 0;
        int key_off = sc->keyframes && sc->keyframes[0] == 1;

        current_dts -= sc->dts_shift;

        if (sc->sample_count >= UINT_MAX / sizeof(*st->index_entries))
            return;
        st->index_entries = av_malloc(sc->sample_count*sizeof(*st->index_entries));
        if (!st->index_entries)
            return;
        st->index_entries_allocated_size = sc->sample_count*sizeof(*st->index_entries);

        for (i = 0; i < sc->chunk_count; i++) {
            current_offset = sc->chunk_offsets[i];
            while (stsc_index + 1 < sc->stsc_count &&
                i + 1 == sc->stsc_data[stsc_index + 1].first)
                stsc_index++;
            for (j = 0; j < sc->stsc_data[stsc_index].count; j++) {
                int keyframe = 0;
                if (current_sample >= sc->sample_count) {
                    av_log(mov->fc, AV_LOG_ERROR, "wrong sample count\n");
                    return;
                }

                if (!sc->keyframe_count || current_sample+key_off == sc->keyframes[stss_index]) {
                    keyframe = 1;
                    if (stss_index + 1 < sc->keyframe_count)
                        stss_index++;
                } else if (sc->stps_count && current_sample+key_off == sc->stps_data[stps_index]) {
                    keyframe = 1;
                    if (stps_index + 1 < sc->stps_count)
                        stps_index++;
                }
                if (keyframe)
                    distance = 0;
                sample_size = sc->sample_size > 0 ? sc->sample_size : sc->sample_sizes[current_sample];
                if(sc->pseudo_stream_id == -1 ||
                   sc->stsc_data[stsc_index].id - 1 == sc->pseudo_stream_id) {
                    AVIndexEntry *e = &st->index_entries[st->nb_index_entries++];
                    e->pos = current_offset;
                    e->timestamp = current_dts;
                    e->size = sample_size;
                    e->min_distance = distance;
                    e->flags = keyframe ? AVINDEX_KEYFRAME : 0;
                    av_dlog(mov->fc, "AVIndex stream %d, sample %d, offset %"PRIx64", dts %"PRId64", "
                            "size %d, distance %d, keyframe %d\n", st->index, current_sample,
                            current_offset, current_dts, sample_size, distance, keyframe);
                }

                current_offset += sample_size;
                stream_size += sample_size;
                current_dts += sc->stts_data[stts_index].duration;
                distance++;
                stts_sample++;
                current_sample++;
                if (stts_index + 1 < sc->stts_count && stts_sample == sc->stts_data[stts_index].count) {
                    stts_sample = 0;
                    stts_index++;
                }
            }
        }
        if (st->duration > 0)
            st->codec->bit_rate = stream_size*8*sc->time_scale/st->duration;
    } else {
        unsigned chunk_samples, total = 0;

        // compute total chunk count
        for (i = 0; i < sc->stsc_count; i++) {
            unsigned count, chunk_count;

            chunk_samples = sc->stsc_data[i].count;
            if (sc->samples_per_frame && chunk_samples % sc->samples_per_frame) {
                av_log(mov->fc, AV_LOG_ERROR, "error unaligned chunk\n");
                return;
            }

            if (sc->samples_per_frame >= 160) { // gsm
                count = chunk_samples / sc->samples_per_frame;
            } else if (sc->samples_per_frame > 1) {
                unsigned samples = (1024/sc->samples_per_frame)*sc->samples_per_frame;
                count = (chunk_samples+samples-1) / samples;
            } else {
                count = (chunk_samples+1023) / 1024;
            }

            if (i < sc->stsc_count - 1)
                chunk_count = sc->stsc_data[i+1].first - sc->stsc_data[i].first;
            else
                chunk_count = sc->chunk_count - (sc->stsc_data[i].first - 1);
            total += chunk_count * count;
        }

        av_dlog(mov->fc, "chunk count %d\n", total);
        if (total >= UINT_MAX / sizeof(*st->index_entries))
            return;
        st->index_entries = av_malloc(total*sizeof(*st->index_entries));
        if (!st->index_entries)
            return;
        st->index_entries_allocated_size = total*sizeof(*st->index_entries);

        // populate index
        for (i = 0; i < sc->chunk_count; i++) {
            current_offset = sc->chunk_offsets[i];
            if (stsc_index + 1 < sc->stsc_count &&
                i + 1 == sc->stsc_data[stsc_index + 1].first)
                stsc_index++;
            chunk_samples = sc->stsc_data[stsc_index].count;

            while (chunk_samples > 0) {
                AVIndexEntry *e;
                unsigned size, samples;

                if (sc->samples_per_frame >= 160) { // gsm
                    samples = sc->samples_per_frame;
                    size = sc->bytes_per_frame;
                } else {
                    if (sc->samples_per_frame > 1) {
                        samples = FFMIN((1024 / sc->samples_per_frame)*
                                        sc->samples_per_frame, chunk_samples);
                        size = (samples / sc->samples_per_frame) * sc->bytes_per_frame;
                    } else {
                        samples = FFMIN(1024, chunk_samples);
                        size = samples * sc->sample_size;
                    }
                }

                if (st->nb_index_entries >= total) {
                    av_log(mov->fc, AV_LOG_ERROR, "wrong chunk count %d\n", total);
                    return;
                }
                e = &st->index_entries[st->nb_index_entries++];
                e->pos = current_offset;
                e->timestamp = current_dts;
                e->size = size;
                e->min_distance = 0;
                e->flags = AVINDEX_KEYFRAME;
                av_dlog(mov->fc, "AVIndex stream %d, chunk %d, offset %"PRIx64", dts %"PRId64", "
                        "size %d, duration %d\n", st->index, i, current_offset, current_dts,
                        size, samples);

                current_offset += size;
                current_dts += samples;
                chunk_samples -= samples;
            }
        }
    }
}

static int mov_open_dref(AVIOContext **pb, char *src, MOVDref *ref)
{
    /* try relative path, we do not try the absolute because it can leak information about our
       system to an attacker */
    if (ref->nlvl_to > 0 && ref->nlvl_from > 0) {
        char filename[1024];
        char *src_path;
        int i, l;

        /* find a source dir */
        src_path = strrchr(src, '/');
        if (src_path)
            src_path++;
        else
            src_path = src;

        /* find a next level down to target */
        for (i = 0, l = strlen(ref->path) - 1; l >= 0; l--)
            if (ref->path[l] == '/') {
                if (i == ref->nlvl_to - 1)
                    break;
                else
                    i++;
            }

        /* compose filename if next level down to target was found */
        if (i == ref->nlvl_to - 1 && src_path - src  < sizeof(filename)) {
            memcpy(filename, src, src_path - src);
            filename[src_path - src] = 0;

            for (i = 1; i < ref->nlvl_from; i++)
                av_strlcat(filename, "../", 1024);

            av_strlcat(filename, ref->path + l + 1, 1024);

            if (!avio_open(pb, filename, AVIO_FLAG_READ))
                return 0;
        }
    }

    return AVERROR(ENOENT);
}

static int mov_read_trak(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    AVStream *st;
    MOVStreamContext *sc;
    int ret;

    st = av_new_stream(c->fc, c->fc->nb_streams);
    if (!st) return AVERROR(ENOMEM);
    sc = av_mallocz(sizeof(MOVStreamContext));
    if (!sc) return AVERROR(ENOMEM);

    st->priv_data = sc;
    st->codec->codec_type = AVMEDIA_TYPE_DATA;
    sc->ffindex = st->index;

    if ((ret = mov_read_default(c, pb, atom)) < 0)
        return ret;

    /* sanity checks */
    if (sc->chunk_count && (!sc->stts_count || !sc->stsc_count ||
                            (!sc->sample_size && !sc->sample_count))) {
        av_log(c->fc, AV_LOG_ERROR, "stream %d, missing mandatory atoms, broken header\n",
               st->index);
        return 0;
    }

    if (sc->time_scale <= 0) {
        av_log(c->fc, AV_LOG_WARNING, "stream %d, timescale not set\n", st->index);
        sc->time_scale = c->time_scale;
        if (sc->time_scale <= 0)
            sc->time_scale = 1;
    }

    av_set_pts_info(st, 64, 1, sc->time_scale);

    if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
        !st->codec->frame_size && sc->stts_count == 1) {
        st->codec->frame_size = av_rescale(sc->stts_data[0].duration,
                                           st->codec->sample_rate, sc->time_scale);
        av_dlog(c->fc, "frame size %d\n", st->codec->frame_size);
    }

    mov_build_index(c, st);

    if (sc->dref_id-1 < sc->drefs_count && sc->drefs[sc->dref_id-1].path) {
        MOVDref *dref = &sc->drefs[sc->dref_id - 1];
        if (mov_open_dref(&sc->pb, c->fc->filename, dref) < 0)
            av_log(c->fc, AV_LOG_ERROR,
                   "stream %d, error opening alias: path='%s', dir='%s', "
                   "filename='%s', volume='%s', nlvl_from=%d, nlvl_to=%d\n",
                   st->index, dref->path, dref->dir, dref->filename,
                   dref->volume, dref->nlvl_from, dref->nlvl_to);
    } else
        sc->pb = c->fc->pb;

    if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (!st->sample_aspect_ratio.num &&
            (st->codec->width != sc->width || st->codec->height != sc->height)) {
            st->sample_aspect_ratio = av_d2q(((double)st->codec->height * sc->width) /
                                             ((double)st->codec->width * sc->height), INT_MAX);
        }

        av_reduce(&st->avg_frame_rate.num, &st->avg_frame_rate.den,
                  sc->time_scale*st->nb_frames, st->duration, INT_MAX);

        if (sc->stts_count == 1 || (sc->stts_count == 2 && sc->stts_data[1].count == 1))
            av_reduce(&st->r_frame_rate.num, &st->r_frame_rate.den,
                      sc->time_scale, sc->stts_data[0].duration, INT_MAX);
    }

    switch (st->codec->codec_id) {
#if CONFIG_H261_DECODER
    case CODEC_ID_H261:
#endif
#if CONFIG_H263_DECODER
    case CODEC_ID_H263:
#endif
#if CONFIG_H264_DECODER
    case CODEC_ID_H264:
#endif
#if CONFIG_MPEG4_DECODER
    case CODEC_ID_MPEG4:
#endif
        st->codec->width = 0; /* let decoder init width/height */
        st->codec->height= 0;
        break;
    }

    /* Do not need those anymore. */
    av_freep(&sc->chunk_offsets);
    av_freep(&sc->stsc_data);
    av_freep(&sc->sample_sizes);
    av_freep(&sc->keyframes);
    av_freep(&sc->stts_data);
    av_freep(&sc->stps_data);

    return 0;
}

static int mov_read_ilst(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int ret;
    c->itunes_metadata = 1;
    ret = mov_read_default(c, pb, atom);
    c->itunes_metadata = 0;
    return ret;
}

static int mov_read_meta(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    while (atom.size > 8) {
        uint32_t tag = avio_rl32(pb);
        atom.size -= 4;
        if (tag == MKTAG('h','d','l','r')) {
            avio_seek(pb, -8, SEEK_CUR);
            atom.size += 8;
            return mov_read_default(c, pb, atom);
        }
    }
    return 0;
}

static int mov_read_tkhd(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int i;
    int width;
    int height;
    int64_t disp_transform[2];
    int display_matrix[3][2];
    AVStream *st;
    MOVStreamContext *sc;
    int version;
    int display_matrix_x_2;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    version = avio_r8(pb);
    avio_rb24(pb); /* flags */
    /*
    MOV_TRACK_ENABLED 0x0001
    MOV_TRACK_IN_MOVIE 0x0002
    MOV_TRACK_IN_PREVIEW 0x0004
    MOV_TRACK_IN_POSTER 0x0008
    */

    if (version == 1) {
        avio_rb64(pb);
        avio_rb64(pb);
    } else {
        avio_rb32(pb); /* creation time */
        avio_rb32(pb); /* modification time */
    }
    st->id = (int)avio_rb32(pb); /* track id (NOT 0 !)*/
    avio_rb32(pb); /* reserved */

    /* highlevel (considering edits) duration in movie timebase */
    (version == 1) ? avio_rb64(pb) : avio_rb32(pb);
    avio_rb32(pb); /* reserved */
    avio_rb32(pb); /* reserved */

    avio_rb16(pb); /* layer */
    avio_rb16(pb); /* alternate group */
    avio_rb16(pb); /* volume */
    avio_rb16(pb); /* reserved */

    //read in the display matrix (outlined in ISO 14496-12, Section 6.2.2)
    // they're kept in fixed point format through all calculations
    // ignore u,v,z b/c we don't need the scale factor to calc aspect ratio
    for (i = 0; i < 3; i++) {
        display_matrix[i][0] = avio_rb32(pb);   // 16.16 fixed point
        display_matrix[i][1] = avio_rb32(pb);   // 16.16 fixed point
        display_matrix_x_2 = avio_rb32(pb);           // 2.30 fixed point (not used)
        av_log(NULL, AV_LOG_DEBUG, "[Matrix Structure] %8x %8x %8x\n", display_matrix[i][0],display_matrix[i][1],display_matrix_x_2);
    }

    width = avio_rb32(pb);       // 16.16 fixed point track width
    height = avio_rb32(pb);      // 16.16 fixed point track height
    av_log(NULL, AV_LOG_DEBUG, "[--track width/height] %8x %8x\n", width,height);
    sc->width = width >> 16;
    sc->height = height >> 16;

    //audio: w=0,h=0;
    //video:  w>0,h>0;
    //roy mdf 2012.03.21, ONLY for video
    if (width && height)
    {
        // transform the display width/height according to the matrix
        // skip this if the display matrix is the default identity matrix
        // or if it is rotating the picture, ex iPhone 3GS
        // to keep the same scale, use [width height 1<<16]
        if((display_matrix[0][0] != 65536  ||
          display_matrix[1][1] != 65536) &&
         !display_matrix[0][1] &&
         !display_matrix[1][0] &&
         !display_matrix[2][0] && !display_matrix[2][1]) {
            for (i = 0; i < 2; i++)
                disp_transform[i] =
                    (int64_t)  width  * display_matrix[0][i] +
                    (int64_t)  height * display_matrix[1][i] +
                    ((int64_t) display_matrix[2][i] << 16);

            //sample aspect ratio is new width/height divided by old width/height
            st->sample_aspect_ratio = av_d2q(
                ((double) disp_transform[0] * height) /
                ((double) disp_transform[1] * width), INT_MAX);
        }

        /* roy mdf 2012.03.21, get the rotation information

            0 degree: (x', y') = (x, y)
            1 0
            0 1
            0 0

            90 degree:(x', y') = (height - y, x)
            0 1
            -1 0
            height 0 || 0  0

            180 degree:(x', y') = (widht - x, height - y)
            -1 0
            0 -1
            width height || 0  0

            270 degree:(x', y') = (y, width - x)
            0 -1
            1 0
            0 width
        */
            if(display_matrix[0][0]==0x10000 && !display_matrix[0][1] &&
                !display_matrix[1][0] && display_matrix[1][1]==0x10000 &&
                !display_matrix[2][0] && !display_matrix[2][1])
            {
                st->rotation_info=0;
                av_log(NULL, AV_LOG_DEBUG, "[MOV] Rotation 0.\n");
            }
            else if(!display_matrix[0][0] && display_matrix[0][1]==0x10000 &&
                display_matrix[1][0]==0xffff0000 && !display_matrix[1][1] &&
                ((display_matrix[2][0]==height && !display_matrix[2][1])||(!display_matrix[2][0] && !display_matrix[2][1])))
            {
                st->rotation_info=90;
                av_log(NULL, AV_LOG_DEBUG, "[MOV] Rotation 90.\n");
            }
            else if(display_matrix[0][0]==0xffff0000 && !display_matrix[0][1] &&
                !display_matrix[1][0] && display_matrix[1][1]==0xffff0000 &&
                ((display_matrix[2][0]==width && display_matrix[2][1]==height)||(!display_matrix[2][0] && !display_matrix[2][1])))
            {
                st->rotation_info=180;
                av_log(NULL, AV_LOG_DEBUG, "[MOV] Rotation 180.\n");
            }
            else if(!display_matrix[0][0] && display_matrix[0][1]==0xffff0000 &&
                display_matrix[1][0]==0x10000 && !display_matrix[1][1] &&
                !display_matrix[2][0] && display_matrix[2][1]==width)
            {
                st->rotation_info=270;
                av_log(NULL, AV_LOG_DEBUG, "[MOV] Rotation 270.\n");
            }
            else
            {
                st->rotation_info=0;
                av_log(NULL, AV_LOG_DEBUG, "[MOV] invalid Matrix Structure, default Rotation 0.\n");
            }
    }
    return 0;
}

static int mov_read_tfhd(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    MOVFragment *frag = &c->fragment;
    MOVTrackExt *trex = NULL;
    int flags, track_id, i;

    avio_r8(pb); /* version */
    flags = avio_rb24(pb);

    track_id = avio_rb32(pb);
    if (!track_id)
        return -1;
    frag->track_id = track_id;
    for (i = 0; i < c->trex_count; i++)
        if (c->trex_data[i].track_id == frag->track_id) {
            trex = &c->trex_data[i];
            break;
        }
    if (!trex) {
        av_log(c->fc, AV_LOG_ERROR, "could not find corresponding trex\n");
        return -1;
    }

    if (flags & 0x01) frag->base_data_offset = avio_rb64(pb);
    else              frag->base_data_offset = frag->moof_offset;
    if (flags & 0x02) frag->stsd_id          = avio_rb32(pb);
    else              frag->stsd_id          = trex->stsd_id;

    frag->duration = flags & 0x08 ? avio_rb32(pb) : trex->duration;
    frag->size     = flags & 0x10 ? avio_rb32(pb) : trex->size;
    frag->flags    = flags & 0x20 ? avio_rb32(pb) : trex->flags;
    av_dlog(c->fc, "frag flags 0x%x\n", frag->flags);
    return 0;
}

static int mov_read_chap(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    c->chapter_track = avio_rb32(pb);
    return 0;
}

static int mov_read_trex(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    MOVTrackExt *trex;

    if ((uint64_t)c->trex_count+1 >= UINT_MAX / sizeof(*c->trex_data))
        return -1;
    trex = av_realloc(c->trex_data, (c->trex_count+1)*sizeof(*c->trex_data));
    if (!trex)
        return AVERROR(ENOMEM);
    c->trex_data = trex;
    trex = &c->trex_data[c->trex_count++];
    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    trex->track_id = avio_rb32(pb);
    trex->stsd_id  = avio_rb32(pb);
    trex->duration = avio_rb32(pb);
    trex->size     = avio_rb32(pb);
    trex->flags    = avio_rb32(pb);
    return 0;
}

static int mov_read_trun(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    MOVFragment *frag = &c->fragment;
    AVStream *st = NULL;
    MOVStreamContext *sc;
    uint64_t offset;
    int64_t dts;
    int data_offset = 0;
    unsigned entries, first_sample_flags = frag->flags;
    int flags, distance, i;

    for (i = 0; i < c->fc->nb_streams; i++) {
        if (c->fc->streams[i]->id == frag->track_id) {
            st = c->fc->streams[i];
            break;
        }
    }
    if (!st) {
        av_log(c->fc, AV_LOG_ERROR, "could not find corresponding track id %d\n", frag->track_id);
        return -1;
    }
    sc = st->priv_data;
    if (sc->pseudo_stream_id+1 != frag->stsd_id)
        return 0;
    avio_r8(pb); /* version */
    flags = avio_rb24(pb);
    entries = avio_rb32(pb);
    av_dlog(c->fc, "flags 0x%x entries %d\n", flags, entries);
    if (flags & 0x001) data_offset        = avio_rb32(pb);
    if (flags & 0x004) first_sample_flags = avio_rb32(pb);
    if (flags & 0x800) {
        MOVStts *ctts_data;
        if ((uint64_t)entries+sc->ctts_count >= UINT_MAX/sizeof(*sc->ctts_data))
            return -1;
        ctts_data = av_realloc(sc->ctts_data,
                               (entries+sc->ctts_count)*sizeof(*sc->ctts_data));
        if (!ctts_data)
            return AVERROR(ENOMEM);
        sc->ctts_data = ctts_data;
    }
    dts = st->duration;
    offset = frag->base_data_offset + data_offset;
    distance = 0;
    av_dlog(c->fc, "first sample flags 0x%x\n", first_sample_flags);
    for (i = 0; i < entries; i++) {
        unsigned sample_size = frag->size;
        int sample_flags = i ? frag->flags : first_sample_flags;
        unsigned sample_duration = frag->duration;
        int keyframe;

        if (flags & 0x100) sample_duration = avio_rb32(pb);
        if (flags & 0x200) sample_size     = avio_rb32(pb);
        if (flags & 0x400) sample_flags    = avio_rb32(pb);
        if (flags & 0x800) {
            sc->ctts_data[sc->ctts_count].count = 1;
            sc->ctts_data[sc->ctts_count].duration = avio_rb32(pb);
            sc->ctts_count++;
        }
        if ((keyframe = st->codec->codec_type == AVMEDIA_TYPE_AUDIO ||
             (flags & 0x004 && !i && !sample_flags) || sample_flags & 0x2000000))
            distance = 0;
        av_add_index_entry(st, offset, dts, sample_size, distance,
                           keyframe ? AVINDEX_KEYFRAME : 0);
        av_dlog(c->fc, "AVIndex stream %d, sample %d, offset %"PRIx64", dts %"PRId64", "
                "size %d, distance %d, keyframe %d\n", st->index, sc->sample_count+i,
                offset, dts, sample_size, distance, keyframe);
        distance++;
        dts += sample_duration;
        offset += sample_size;
    }
    frag->moof_offset = offset;
    st->duration = dts;
    return 0;
}

/* this atom should be null (from specs), but some buggy files put the 'moov' atom inside it... */
/* like the files created with Adobe Premiere 5.0, for samples see */
/* http://graphics.tudelft.nl/~wouter/publications/soundtests/ */
static int mov_read_wide(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    int err;

    if (atom.size < 8)
        return 0; /* continue */
    if (avio_rb32(pb) != 0) { /* 0 sized mdat atom... use the 'wide' atom size */
        avio_skip(pb, atom.size - 4);
        return 0;
    }
    atom.type = avio_rl32(pb);
    atom.size -= 8;
    if (atom.type != MKTAG('m','d','a','t')) {
        avio_skip(pb, atom.size);
        return 0;
    }
    err = mov_read_mdat(c, pb, atom);
    return err;
}

static int mov_read_cmov(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
#if CONFIG_ZLIB
    AVIOContext ctx;
    uint8_t *cmov_data;
    uint8_t *moov_data; /* uncompressed data */
    long cmov_len, moov_len;
    int ret = -1;

    avio_rb32(pb); /* dcom atom */
    if (avio_rl32(pb) != MKTAG('d','c','o','m'))
        return -1;
    if (avio_rl32(pb) != MKTAG('z','l','i','b')) {
        av_log(c->fc, AV_LOG_ERROR, "unknown compression for cmov atom !");
        return -1;
    }
    avio_rb32(pb); /* cmvd atom */
    if (avio_rl32(pb) != MKTAG('c','m','v','d'))
        return -1;
    moov_len = avio_rb32(pb); /* uncompressed size */
    cmov_len = atom.size - 6 * 4;

    cmov_data = av_malloc(cmov_len);
    if (!cmov_data)
        return AVERROR(ENOMEM);
    moov_data = av_malloc(moov_len);
    if (!moov_data) {
        av_free(cmov_data);
        return AVERROR(ENOMEM);
    }
    avio_read(pb, cmov_data, cmov_len);
    if(uncompress (moov_data, (uLongf *) &moov_len, (const Bytef *)cmov_data, cmov_len) != Z_OK)
        goto free_and_return;
    if(ffio_init_context(&ctx, moov_data, moov_len, 0, NULL, NULL, NULL, NULL) != 0)
        goto free_and_return;
    atom.type = MKTAG('m','o','o','v');
    atom.size = moov_len;
#ifdef DEBUG
//    { int fd = open("/tmp/uncompheader.mov", O_WRONLY | O_CREAT); write(fd, moov_data, moov_len); close(fd); }
#endif
    ret = mov_read_default(c, &ctx, atom);
free_and_return:
    av_free(moov_data);
    av_free(cmov_data);
    return ret;
#else
    av_log(c->fc, AV_LOG_ERROR, "this file requires zlib support compiled in\n");
    return -1;
#endif
}

/* edit list atom */
static int mov_read_elst(MOVContext *c, AVIOContext *pb, MOVAtom atom)
{
    MOVStreamContext *sc;
    int i, edit_count;

    if (c->fc->nb_streams < 1)
        return 0;
    sc = c->fc->streams[c->fc->nb_streams-1]->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    edit_count = avio_rb32(pb); /* entries */

    if((uint64_t)edit_count*12+8 > atom.size)
        return -1;

    for(i=0; i<edit_count; i++){
        int time;
        int duration = avio_rb32(pb); /* Track duration */
        time = avio_rb32(pb); /* Media time */
        avio_rb32(pb); /* Media rate */
        if (i == 0 && time >= -1) {
            sc->time_offset = time != -1 ? time : -duration;
        }
    }

    if(edit_count > 1)
        av_log(c->fc, AV_LOG_WARNING, "multiple edit list entries, "
               "a/v desync might occur, patch welcome\n");

    av_dlog(c->fc, "track[%i].edit_count = %i\n", c->fc->nb_streams-1, edit_count);
    return 0;
}

static const MOVParseTableEntry mov_default_parse_table[] = {
{ MKTAG('a','v','s','s'), mov_read_extradata },
{ MKTAG('c','h','p','l'), mov_read_chpl },
{ MKTAG('c','o','6','4'), mov_read_stco },
{ MKTAG('c','t','t','s'), mov_read_ctts }, /* composition time to sample */
{ MKTAG('d','i','n','f'), mov_read_default },
{ MKTAG('d','r','e','f'), mov_read_dref },
{ MKTAG('e','d','t','s'), mov_read_default },
{ MKTAG('e','l','s','t'), mov_read_elst },
{ MKTAG('e','n','d','a'), mov_read_enda },
{ MKTAG('f','i','e','l'), mov_read_extradata },
{ MKTAG('f','t','y','p'), mov_read_ftyp },
{ MKTAG('g','l','b','l'), mov_read_glbl },
{ MKTAG('h','d','l','r'), mov_read_hdlr },
{ MKTAG('i','l','s','t'), mov_read_ilst },
{ MKTAG('j','p','2','h'), mov_read_extradata },
{ MKTAG('m','d','a','t'), mov_read_mdat },
{ MKTAG('m','d','h','d'), mov_read_mdhd },
{ MKTAG('m','d','i','a'), mov_read_default },
{ MKTAG('m','e','t','a'), mov_read_meta },
{ MKTAG('m','i','n','f'), mov_read_default },
{ MKTAG('m','o','o','f'), mov_read_moof },
{ MKTAG('m','o','o','v'), mov_read_moov },
{ MKTAG('m','v','e','x'), mov_read_default },
{ MKTAG('m','v','h','d'), mov_read_mvhd },
{ MKTAG('S','M','I',' '), mov_read_smi }, /* Sorenson extension ??? */
{ MKTAG('a','l','a','c'), mov_read_extradata }, /* alac specific atom */
{ MKTAG('a','v','c','C'), mov_read_glbl },
{ MKTAG('p','a','s','p'), mov_read_pasp },
{ MKTAG('s','t','b','l'), mov_read_default },
{ MKTAG('s','t','c','o'), mov_read_stco },
{ MKTAG('s','t','p','s'), mov_read_stps },
{ MKTAG('s','t','r','f'), mov_read_strf },
{ MKTAG('s','t','s','c'), mov_read_stsc },
{ MKTAG('s','t','s','d'), mov_read_stsd }, /* sample description */
{ MKTAG('s','t','s','s'), mov_read_stss }, /* sync sample */
{ MKTAG('s','t','s','z'), mov_read_stsz }, /* sample size */
{ MKTAG('s','t','t','s'), mov_read_stts },
{ MKTAG('s','t','z','2'), mov_read_stsz }, /* compact sample size */
{ MKTAG('t','k','h','d'), mov_read_tkhd }, /* track header */
{ MKTAG('t','f','h','d'), mov_read_tfhd }, /* track fragment header */
{ MKTAG('t','r','a','k'), mov_read_trak },
{ MKTAG('t','r','a','f'), mov_read_default },
{ MKTAG('t','r','e','f'), mov_read_default },
{ MKTAG('c','h','a','p'), mov_read_chap },
{ MKTAG('t','r','e','x'), mov_read_trex },
{ MKTAG('t','r','u','n'), mov_read_trun },
{ MKTAG('u','d','t','a'), mov_read_default },
{ MKTAG('w','a','v','e'), mov_read_wave },
{ MKTAG('e','s','d','s'), mov_read_esds },
{ MKTAG('d','a','c','3'), mov_read_dac3 }, /* AC-3 info */
{ MKTAG('w','i','d','e'), mov_read_wide }, /* place holder */
{ MKTAG('c','m','o','v'), mov_read_cmov },
{ 0, NULL }
};

static int mov_probe(AVProbeData *p)
{
    unsigned int offset;
    uint32_t tag;
    int score = 0;

    /* check file header */
    offset = 0;
    for(;;) {
        /* ignore invalid offset */
        if ((offset + 8) > (unsigned int)p->buf_size)
            return score;
        tag = AV_RL32(p->buf + offset + 4);
        switch(tag) {
        /* check for obvious tags */
        case MKTAG('j','P',' ',' '): /* jpeg 2000 signature */
        case MKTAG('m','o','o','v'):
        case MKTAG('m','d','a','t'):
        case MKTAG('p','n','o','t'): /* detect movs with preview pics like ew.mov and april.mov */
        case MKTAG('u','d','t','a'): /* Packet Video PVAuthor adds this and a lot of more junk */
        case MKTAG('f','t','y','p'):
            return AVPROBE_SCORE_MAX - 1;
        /* those are more common words, so rate then a bit less */
        case MKTAG('e','d','i','w'): /* xdcam files have reverted first tags */
        case MKTAG('w','i','d','e'):
        case MKTAG('f','r','e','e'):
        case MKTAG('j','u','n','k'):
        case MKTAG('p','i','c','t'):
            return AVPROBE_SCORE_MAX - 5;
        case MKTAG(0x82,0x82,0x7f,0x7d):
        case MKTAG('s','k','i','p'):
        case MKTAG('u','u','i','d'):
        case MKTAG('p','r','f','l'):
            offset = AV_RB32(p->buf+offset) + offset;
            /* if we only find those cause probedata is too small at least rate them */
            score = AVPROBE_SCORE_MAX - 50;
            break;
        default:
            /* unrecognized tag */
            return score;
        }
    }
    return score;
}

// must be done after parsing all trak because there's no order requirement
static void mov_read_chapters(AVFormatContext *s)
{
    MOVContext *mov = s->priv_data;
    AVStream *st = NULL;
    MOVStreamContext *sc;
    int64_t cur_pos;
    int i;

    for (i = 0; i < s->nb_streams; i++)
        if (s->streams[i]->id == mov->chapter_track) {
            st = s->streams[i];
            break;
        }
    if (!st) {
        av_log(s, AV_LOG_ERROR, "Referenced QT chapter track not found\n");
        return;
    }

    st->discard = AVDISCARD_ALL;
    sc = st->priv_data;
    cur_pos = avio_tell(sc->pb);

    for (i = 0; i < st->nb_index_entries; i++) {
        AVIndexEntry *sample = &st->index_entries[i];
        int64_t end = i+1 < st->nb_index_entries ? st->index_entries[i+1].timestamp : st->duration;
        uint8_t *title;
        uint16_t ch;
        int len, title_len;

        if (avio_seek(sc->pb, sample->pos, SEEK_SET) != sample->pos) {
            av_log(s, AV_LOG_ERROR, "Chapter %d not found in file\n", i);
            goto finish;
        }

        // the first two bytes are the length of the title
        len = avio_rb16(sc->pb);
        if (len > sample->size-2)
            continue;
        title_len = 2*len + 1;
        if (!(title = av_mallocz(title_len)))
            goto finish;

        // The samples could theoretically be in any encoding if there's an encd
        // atom following, but in practice are only utf-8 or utf-16, distinguished
        // instead by the presence of a BOM
        ch = avio_rb16(sc->pb);
        if (ch == 0xfeff)
            avio_get_str16be(sc->pb, len, title, title_len);
        else if (ch == 0xfffe)
            avio_get_str16le(sc->pb, len, title, title_len);
        else {
            AV_WB16(title, ch);
            get_strz(sc->pb, title + 2, len - 1);
        }

        ff_new_chapter(s, i, st->time_base, sample->timestamp, end, title);
        av_freep(&title);
    }
finish:
    avio_seek(sc->pb, cur_pos, SEEK_SET);
}

static int mov_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    MOVContext *mov = s->priv_data;
    AVIOContext *pb = s->pb;
    int err;
    MOVAtom atom = { AV_RL32("root") };

    mov->fc = s;
    /* .mov and .mp4 aren't streamable anyway (only progressive download if moov is before mdat) */
    if(pb->seekable)
        atom.size = avio_size(pb);
    else
        atom.size = INT64_MAX;

    /* check MOV header */
    if ((err = mov_read_default(mov, pb, atom)) < 0) {
        av_log(s, AV_LOG_ERROR, "error reading header: %d\n", err);
        return err;
    }
    if (!mov->found_moov) {
        av_log(s, AV_LOG_ERROR, "moov atom not found\n");
        return -1;
    }
    av_dlog(mov->fc, "on_parse_exit_offset=%lld\n", avio_tell(pb));

    if (pb->seekable && mov->chapter_track > 0)
        mov_read_chapters(s);

    init_http_thread((HTTPThreadContext**)&mov->http_thread_ctx_ptr, s);
    return 0;
}

static AVIndexEntry *mov_find_next_sample(AVFormatContext *s, AVStream **st)
{
    AVIndexEntry *sample = NULL;
    int64_t best_dts = INT64_MAX;
    int i;
    AVIndexEntry *current_sample = NULL;


    MOVContext *mov = s->priv_data;
    HTTPThreadContext *http_thread_ctx_ptr = (HTTPThreadContext*) mov->http_thread_ctx_ptr;

    if (http_thread_ctx_ptr && !http_thread_ctx_ptr->bRun) {
        AVIndexEntry *tmp_sample = NULL;
        for (i = 0; i < s->nb_streams; i++) {
            AVStream *avst = s->streams[i];
            MOVStreamContext *msc = avst->priv_data;
            current_sample = &avst->index_entries[msc->current_sample];
            if(!sample || (!s->pb->seekable && current_sample->pos < sample->pos) ||
                (s->pb->seekable && current_sample->pos < sample->pos &&
                ((msc->pb != s->pb) || (msc->pb == s->pb)))){
                if(tmp_sample==NULL || current_sample->pos < tmp_sample->pos){
                    tmp_sample = current_sample;
                    *st = avst;
                }
            }
        }
        sample = tmp_sample;
    }
    else
    {
        for (i = 0; i < s->nb_streams; i++) {
            AVStream *avst = s->streams[i];
            MOVStreamContext *msc = avst->priv_data;
            if (msc->pb && msc->current_sample < avst->nb_index_entries) {
                current_sample = &avst->index_entries[msc->current_sample];
                int64_t dts = av_rescale(current_sample->timestamp, AV_TIME_BASE, msc->time_scale);
                av_dlog(s, "stream %d, sample %d, dts %"PRId64"\n", i, msc->current_sample, dts);

                if (!sample || (!s->pb->seekable && current_sample->pos < sample->pos) ||
                    (s->pb->seekable && current_sample->pos < sample->pos &&
                     ((msc->pb != s->pb && dts < best_dts) || (msc->pb == s->pb &&
                     ((FFABS(best_dts - dts) <= AV_TIME_BASE) ||
                      (FFABS(best_dts - dts) > AV_TIME_BASE && dts < best_dts)))))) {
                    sample = current_sample;
                    best_dts = dts;
                    *st = avst;
                }
                else {
                    if (!sample || (!s->pb->seekable && current_sample->pos < sample->pos) ||
                        (s->pb->seekable &&
                         ((msc->pb != s->pb && dts < best_dts) || (msc->pb == s->pb &&
                         ((FFABS(best_dts - dts) <= AV_TIME_BASE && current_sample->pos < sample->pos) ||
                          (FFABS(best_dts - dts) > AV_TIME_BASE && dts < best_dts)))))) {
                        sample = current_sample;
                        best_dts = dts;
                        *st = avst;
                    }
                }
            }
        }
    }

    if (sample) {
#if AV_STATUS_DBG
        av_log(NULL, AV_LOG_ERROR, "%s pos=%lld size=%d base=%lld end=%lld\n", ((*st)->codec->codec_type == AVMEDIA_TYPE_AUDIO) ? "A" : "V", sample->pos, sample->size, (s->pb->pos - (s->pb->buf_end - s->pb->buffer)), s->pb->pos);
#endif
        if (http_thread_ctx_ptr) {
            if ((*st)->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
                http_thread_ctx_ptr->last_a_pos = sample->pos;
            } else if ((*st)->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                http_thread_ctx_ptr->last_v_pos = sample->pos;
            }
#if AV_STATUS_DBG
            av_log(NULL, AV_LOG_ERROR, "last_a_pos=%lld last_v_pos=%lld\n", http_thread_ctx_ptr->last_a_pos, http_thread_ctx_ptr->last_v_pos);
#endif
        }
    }

    return sample;
}

static int mov_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    MOVContext *mov = s->priv_data;
    MOVStreamContext *sc;
    AVIndexEntry *sample;
    AVStream *st = NULL;
    int ret;
    HTTPThreadContext *pThreadCtx = (HTTPThreadContext*)mov->http_thread_ctx_ptr;

 retry:
    sample = mov_find_next_sample(s, &st);
    if (!sample) {
        mov->found_mdat = 0;
        if (s->pb->seekable||
            mov_read_default(mov, s->pb, (MOVAtom){ AV_RL32("root"), INT64_MAX }) < 0 ||
            url_feof(s->pb))
            return AVERROR_EOF;
        av_dlog(s, "read fragments, offset 0x%llx\n", avio_tell(s->pb));
        goto retry;
    }
    sc = st->priv_data;
    /* must be done just before reading, to avoid infinite loop on sample */
    sc->current_sample++;

    if (st->discard != AVDISCARD_ALL) {
        int64_t pos = http_av_seek(pThreadCtx, sc->pb, sample->pos, SEEK_SET);
        if (pos != sample->pos) {
            if (pos == AVERROR(EAGAIN))
                return AVERROR(EAGAIN);

            av_log(mov->fc, AV_LOG_ERROR, "stream %d, offset 0x%"PRIx64": partial file\n",
                   sc->ffindex, sample->pos);
            return -1;
        }

        ret = http_av_get_packet(pThreadCtx, sc->pb, pkt, sample->size);
        if (ret < 0){
            if(ret == AVERROR(EAGAIN))
                sc->current_sample--;
            return ret;
        }

        if (sc->has_palette) {
            uint8_t *pal;

            pal = av_packet_new_side_data(pkt, AV_PKT_DATA_PALETTE, AVPALETTE_SIZE);
            if (!pal) {
                av_log(mov->fc, AV_LOG_ERROR, "Cannot append palette to packet\n");
            } else {
                memcpy(pal, sc->palette, AVPALETTE_SIZE);
                sc->has_palette = 0;
            }
        }
#if CONFIG_DV_DEMUXER
        if (mov->dv_demux && sc->dv_audio_container) {
            dv_produce_packet(mov->dv_demux, pkt, pkt->data, pkt->size);
            av_free(pkt->data);
            pkt->size = 0;
            ret = dv_get_packet(mov->dv_demux, pkt);
            if (ret < 0)
                return ret;
        }
#endif
    }

    pkt->stream_index = sc->ffindex;
    pkt->dts = sample->timestamp;
    if (sc->ctts_data) {
        /*
         *  CTTS(Composition Time-To-Sample) is equal to PTS for video
         *  1. detect error in CTTS table of bitstream. see func: "mov_read_ctts"
         *     condition:  diff between two concecutive frames should be divided by video_tick,eg: 3003, 3000
         *  2. if error exists, use default pts calculation method
         *
         *  This can fix A5s buggy bitstream, which fill wrong ctts.
         */
        if (sc->ctts_error) {
            int64_t next_dts = (sc->current_sample < st->nb_index_entries) ?
            st->index_entries[sc->current_sample].timestamp : st->duration;
            pkt->duration = next_dts - pkt->dts;
            pkt->pts = pkt->dts;
        } else {
            pkt->pts = pkt->dts + sc->dts_shift + sc->ctts_data[sc->ctts_index].duration;
            /* update ctts context */
            sc->ctts_sample++;
            if (sc->ctts_index < sc->ctts_count &&
                sc->ctts_data[sc->ctts_index].count == sc->ctts_sample) {
                sc->ctts_index++;
                sc->ctts_sample = 0;
            }
            if (sc->wrong_dts)
                pkt->dts = AV_NOPTS_VALUE;
        }
    } else {
        int64_t next_dts = (sc->current_sample < st->nb_index_entries) ?
            st->index_entries[sc->current_sample].timestamp : st->duration;
        pkt->duration = next_dts - pkt->dts;
        pkt->pts = pkt->dts;
    }
    if (st->discard == AVDISCARD_ALL)
        goto retry;
    pkt->flags |= sample->flags & AVINDEX_KEYFRAME ? AV_PKT_FLAG_KEY : 0;
    pkt->pos = sample->pos;
    av_dlog(s, "stream %d, pts %"PRId64", dts %"PRId64", pos 0x%"PRIx64", duration %d\n",
            pkt->stream_index, pkt->pts, pkt->dts, pkt->pos, pkt->duration);
    return 0;
}

static int mov_seek_stream(AVFormatContext *s, AVStream *st, int64_t timestamp, int flags)
{
    MOVStreamContext *sc = st->priv_data;
    int sample, time_sample;
    int i;

    sample = av_index_search_timestamp(st, timestamp, flags);
    av_dlog(s, "stream %d, timestamp %"PRId64", sample %d\n", st->index, timestamp, sample);
    if (sample < 0 && st->nb_index_entries && timestamp < st->index_entries[0].timestamp)
        sample = 0;
    if (sample < 0) /* not sure what to do */
        return -1;
    sc->current_sample = sample;
    av_dlog(s, "stream %d, found sample %d\n", st->index, sc->current_sample);
    /* adjust ctts index */
    if (sc->ctts_data) {
        time_sample = 0;
        for (i = 0; i < sc->ctts_count; i++) {
            int next = time_sample + sc->ctts_data[i].count;
            if (next > sc->current_sample) {
                sc->ctts_index = i;
                sc->ctts_sample = sc->current_sample - time_sample;
                break;
            }
            time_sample = next;
        }
    }
    return sample;
}

static int mov_read_seek(AVFormatContext *s, int stream_index, int64_t sample_time, int flags)
{
    AVStream *st;
    int64_t seek_timestamp, timestamp;
    int sample;
    int i;

    if (stream_index >= s->nb_streams)
        return -1;
    if (sample_time < 0)
        sample_time = 0;

    st = s->streams[stream_index];
    sample = mov_seek_stream(s, st, sample_time, flags);
    if (sample < 0)
        return -1;

    /* adjust seek timestamp to found sample timestamp */
    seek_timestamp = st->index_entries[sample].timestamp;

    for (i = 0; i < s->nb_streams; i++) {
        st = s->streams[i];
        if (stream_index == i)
            continue;

        timestamp = av_rescale_q(seek_timestamp, s->streams[stream_index]->time_base, st->time_base);
        mov_seek_stream(s, st, timestamp, flags);
    }

#if 0
    MOVContext *mov = s->priv_data;
    HTTPThreadContext *pThreadCtx = (HTTPThreadContext*) mov->http_thread_ctx_ptr;
    if (pThreadCtx && pThreadCtx->bRun) {
        MOVContext *mov = s->priv_data;
        MOVStreamContext *sc;
        AVIndexEntry *sample;
        AVStream *st = NULL;
        sample = mov_find_next_sample(s, &st);
        sc = st->priv_data;
        if (sample && force_seek_http_thread(pThreadCtx, sc->pb, sample->pos) != sample->pos) {
            av_log(mov->fc, AV_LOG_ERROR, "stream %d, offset 0x%"PRIx64": partial file\n",
                   sc->ffindex, sample->pos);
        }
    }
#endif

    return 0;
}

static int mov_read_close(AVFormatContext *s)
{
    MOVContext *mov = s->priv_data;
    int i, j;

    deint_http_thread((HTTPThreadContext**)&mov->http_thread_ctx_ptr);

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        MOVStreamContext *sc = st->priv_data;

        av_freep(&sc->ctts_data);
        for (j = 0; j < sc->drefs_count; j++) {
            av_freep(&sc->drefs[j].path);
            av_freep(&sc->drefs[j].dir);
        }
        av_freep(&sc->drefs);
        if (sc->pb && sc->pb != s->pb)
            avio_close(sc->pb);

        av_freep(&st->codec->palctrl);
    }

    if (mov->dv_demux) {
        for(i = 0; i < mov->dv_fctx->nb_streams; i++) {
            av_freep(&mov->dv_fctx->streams[i]->codec);
            av_freep(&mov->dv_fctx->streams[i]);
        }
        av_freep(&mov->dv_fctx);
        av_freep(&mov->dv_demux);
    }

    av_freep(&mov->trex_data);

    return 0;
}

int http_av_get_packet(HTTPThreadContext *pThreadCtx, AVIOContext *s, AVPacket *pkt, int size)
{
    // if http && http thread isStarted -> call http_avio_read
    // else -> call avio_read as before

    int ret = av_new_packet(pkt, size);
    if(ret<0)
        return ret;

    pkt->pos= avio_tell(s);

    if (pThreadCtx && pThreadCtx->bRun) {
        ret = http_avio_read(pThreadCtx, s, pkt->data, size);
    } else {
        ret= avio_read(s, pkt->data, size);
    }

    if(ret<=0)
        av_free_packet(pkt);
    else
        av_shrink_packet(pkt, ret);

    return ret;
}

int64_t http_av_seek(HTTPThreadContext *pThreadCtx, AVIOContext *s, int64_t offset, int whence)
{
    // if http && http thread isStarted -> call http_avio_seek
    // else -> call avio_read as before

    if (pThreadCtx && pThreadCtx->bRun) {
        return http_avio_seek(pThreadCtx, s, offset, whence);
    } else {
        return avio_seek(s, offset, whence);
    }
}

int http_avio_read(HTTPThreadContext *pThreadCtx, AVIOContext *s, unsigned char *buf, int size)
{
    // if left buf is enough -> read it and update avio
    // else -> request thread to pull enough data, then read it and update avio

#if HTTP_RD_READ_DBG
    int dbg_dataLen = get_buf_data_len(s);
    int dbg_dataReadableLen = get_buf_data_readable_len(s);
    av_log(NULL, AV_LOG_ERROR, "rd: pos r1-1 base=%lld ptr=%lld end=%lld size=%d dataLen=%d readableLen=%d\n", s->pos_base, s->pos_ptr, s->pos, size, dbg_dataLen, dbg_dataReadableLen);
    av_log(NULL, AV_LOG_ERROR, "rd: ptr r1-2 base=%p ptr=%p end=%p\n", s->buf_base, s->buf_ptr, s->buf_end);
#endif

    if (get_buf_data_len(s) < size) {
        if(s->eof_reached) {
            av_log(NULL, AV_LOG_ERROR, "rd: %s error=%d.\n", s->eof_reached?"eos reached,":"", s->error);
            return -1;
        }

        int ret = read_http_thread(pThreadCtx, s, size);
        if (ret < 0) {
            return ret;
        }
    }

    int rdLen = 0;
    while (rdLen < size) {
        int len = get_buf_data_readable_len(s);
        len = (size - rdLen > len) ? len : (size - rdLen);

        memmove(buf, s->buf_ptr, len);

        pthread_mutex_lock(&pThreadCtx->buf_lock);
        inc_buf_rd(s, len);
        pthread_mutex_unlock(&pThreadCtx->buf_lock);

        rdLen += len;
        buf += len;
    }

#if HTTP_RD_READ_DBG
    dbg_dataLen = get_buf_data_len(s);
    dbg_dataReadableLen = get_buf_data_readable_len(s);
    av_log(NULL, AV_LOG_ERROR, "rd: pos r2-1 base=%lld ptr=%lld end=%lld size=%d dataLen=%d readableLen=%d\n", s->pos_base, s->pos_ptr, s->pos, size, dbg_dataLen, dbg_dataReadableLen);
    av_log(NULL, AV_LOG_ERROR, "rd: ptr r2-2 base=%p ptr=%p end=%p\n", s->buf_base, s->buf_ptr, s->buf_end);
#endif

    return size;
}

int64_t http_avio_seek(HTTPThreadContext *pThreadCtx, AVIOContext *s, int64_t offset, int whence)
{
    // if seek point is in left buf -> seek and update avio
    // else -> request thread to seek

    if (whence != SEEK_CUR && whence != SEEK_SET) {
        av_log(NULL, AV_LOG_ERROR, "seek_http: invalid arg whence=%d\n", whence);
        return AVERROR(EINVAL);
    }

    if (whence == SEEK_CUR) {
        if (offset == 0)
            return s->pos_ptr;
        offset += s->pos_ptr;
    }

#if HTTP_RD_SEEK_DBG
    int dbg_dataLen = get_buf_data_len(s);
    int dbg_dataReadableLen = get_buf_data_readable_len(s);
    av_log(NULL, AV_LOG_ERROR, "rd: pos s1-1 base=%lld ptr=%lld end=%lld offset=%lld dataLen=%d readableLen=%d\n", s->pos_base, s->pos_ptr, s->pos, offset, dbg_dataLen, dbg_dataReadableLen);
    av_log(NULL, AV_LOG_ERROR, "rd: ptr s1-2 base=%p ptr=%p end=%p\n", s->buf_base, s->buf_ptr, s->buf_end);
#endif

    if (offset >= s->pos_base && offset <= s->pos) {
        pthread_mutex_lock(&pThreadCtx->buf_lock);
        inc_buf_rd(s, offset - s->pos_ptr);
        pthread_mutex_unlock(&pThreadCtx->buf_lock);
    } else {
        offset = seek_http_thread(pThreadCtx, s, offset);
    }

#if HTTP_RD_SEEK_DBG
    dbg_dataLen = get_buf_data_len(s);
    dbg_dataReadableLen = get_buf_data_readable_len(s);
    av_log(NULL, AV_LOG_ERROR, "rd: pos s2-1 base=%lld ptr=%lld end=%lld offset=%lld dataLen=%d readableLen=%d\n", s->pos_base, s->pos_ptr, s->pos, offset, dbg_dataLen, dbg_dataReadableLen);
    av_log(NULL, AV_LOG_ERROR, "rd: ptr s2-2 base=%p ptr=%p end=%p\n", s->buf_base, s->buf_ptr, s->buf_end);
#endif

    return offset;
}

int get_buf_data_len(AVIOContext *s)
{
    return (s->pos - s->pos_ptr);
}

int get_buf_data_readable_len(AVIOContext *s)
{
    int len = s->pos - s->pos_ptr;
    if (s->buf_ptr + len > s->buffer + s->buffer_size) {
        len = s->buffer_size - (s->buf_ptr - s->buffer);
    }
    return len;
}

int get_buf_free_len(AVIOContext *s)
{
    return (s->buffer_size - (s->pos - s->pos_base));
}

int get_buf_free_writable_len(AVIOContext *s)
{
    int len = s->buffer_size - (s->pos - s->pos_base);
    if (s->buf_end + len > s->buffer + s->buffer_size) {
        len = s->buffer_size - (s->buf_end - s->buffer);
    }
    return len;
}

void inc_buf_rd(AVIOContext *s, int len)
{
#if 0 // HTTP_RD_READ_DBG
    av_log(NULL, AV_LOG_ERROR, "inc_buf_rd: pos 1 base=%lld ptr=%lld end=%lld len=%d\n", s->pos_base, s->pos_ptr, s->pos, len);
    av_log(NULL, AV_LOG_ERROR, "inc_buf_rd: ptr 1 base=%p ptr=%p end=%p\n", s->buf_base, s->buf_ptr, s->buf_end);
#endif

    if (s->buf_ptr + len < s->buffer) {
        s->buf_ptr = s->buf_ptr + len + s->buffer_size;
    } else if (s->buf_ptr + len > s->buffer + s->buffer_size) {
        s->buf_ptr = s->buf_ptr + len - s->buffer_size;
    } else if ((s->buf_ptr + len == s->buffer) || (s->buf_ptr + len == s->buffer + s->buffer_size)) {
        s->buf_ptr = s->buffer;
    } else {
        s->buf_ptr += len;
    }
    s->pos_ptr += len;

#if 0 // HTTP_RD_READ_DBG
    av_log(NULL, AV_LOG_ERROR, "inc_buf_rd: pos 2 base=%lld ptr=%lld end=%lld len=%d\n", s->pos_base, s->pos_ptr, s->pos, len);
    av_log(NULL, AV_LOG_ERROR, "inc_buf_rd: ptr 2 base=%p ptr=%p end=%p\n", s->buf_base, s->buf_ptr, s->buf_end);
#endif
}

void inc_buf_wr(AVIOContext *s, int len)
{
#if 0 // HTTP_WR_DBG
    av_log(NULL, AV_LOG_ERROR, "inc_buf_wr: pos 1 base=%lld ptr=%lld end=%lld len=%d\n", s->pos_base, s->pos_ptr, s->pos, len);
    av_log(NULL, AV_LOG_ERROR, "inc_buf_wr: ptr 1 base=%p ptr=%p end=%p\n", s->buf_base, s->buf_ptr, s->buf_end);
#endif

    if (s->buf_end + len < s->buffer) {
        s->buf_end = s->buf_end + len + s->buffer_size;
    } else if (s->buf_end + len > s->buffer + s->buffer_size) {
        s->buf_end = s->buf_end + len - s->buffer_size;
    } else if ((s->buf_end + len == s->buffer) || (s->buf_end + len == s->buffer + s->buffer_size)) {
        s->buf_end = s->buffer;
    } else {
        s->buf_end += len;
    }
    s->pos += len;

#if 0 // HTTP_WR_DBG
    av_log(NULL, AV_LOG_ERROR, "inc_buf_wr: pos 2 base=%lld ptr=%lld end=%lld len=%d\n", s->pos_base, s->pos_ptr, s->pos, len);
    av_log(NULL, AV_LOG_ERROR, "inc_buf_wr: ptr 2 base=%p ptr=%p end=%p\n", s->buf_base, s->buf_ptr, s->buf_end);
#endif
}

void inc_buf_base(AVIOContext *s, int64_t len)
{
    if (s->buf_base + len < s->buffer) {
        s->buf_base = s->buf_base + len + s->buffer_size;
    } else if (s->buf_base + len > s->buffer + s->buffer_size) {
        s->buf_base = s->buf_base + len - s->buffer_size;
    } else if ((s->buf_base + len == s->buffer) || (s->buf_base + len == s->buffer + s->buffer_size)) {
        s->buf_base = s->buffer;
    } else {
        s->buf_base += len;
    }

#if HTTP_WR_DBG
    av_log(NULL, AV_LOG_ERROR, "inc_buf_base: ~~~pos_base=%lld -> %lld\n", s->pos_base, len);
#endif
    s->pos_base += len;
}

void reset_buf_pos(AVIOContext *s, int64_t pos)
{
    s->buf_base = s->buffer;
    s->buf_ptr = s->buffer;
    s->buf_end = s->buffer;

    s->pos_base = pos;
    s->pos_ptr = pos;
    s->pos = pos;
}

void init_http_thread(HTTPThreadContext **ppThreadCtx, AVFormatContext *s)
{
    if (*ppThreadCtx) {
        av_log(NULL, AV_LOG_WARNING, "init_http_thread: pThreadCtx can't be re-init\n");
        return ;
    }

    URLContext *url = (URLContext*)s->pb->opaque;
    if (strcmp(url->prot->name, "http") != 0) {
        av_log(NULL, AV_LOG_WARNING, "init_http_thread: isn't HTTP protocol\n");
        return ;
    }

    HTTPThreadContext* pThreadCtx = (HTTPThreadContext*) av_mallocz(sizeof(HTTPThreadContext));
    pthread_mutex_init(&pThreadCtx->buf_lock, NULL);
    sem_init(&pThreadCtx->semReq, 0, 0);
    sem_init(&pThreadCtx->semReply, 0, 0);
    pThreadCtx->tid = -1;
    pThreadCtx->bRun = 0;

    pThreadCtx->pAVIO = s->pb;
    pThreadCtx->pFmtCtx = s;
    pThreadCtx->pCB = NULL;
    pThreadCtx->pCtx = NULL;

    pThreadCtx->last_v_pos = -1;
    pThreadCtx->last_a_pos = -1;

    pThreadCtx->recover_network_time_out = 0;

    // config thread attributes
    pthread_attr_t attr;
    struct sched_param param;
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = 99;  //todo, hardcode now
    pthread_attr_setschedparam(&attr, &param);

    pthread_create(&pThreadCtx->tid, &attr, HTTPThreadProc, pThreadCtx);
    pthread_attr_destroy(&attr);

    *ppThreadCtx = pThreadCtx;

    av_log(NULL, AV_LOG_ERROR, "init_http_thread: success\n");
}

void deint_http_thread(HTTPThreadContext **ppThreadCtx)
{
    if (*ppThreadCtx == NULL) {
        av_log(NULL, AV_LOG_WARNING, "deint_http_thread: pThreadCtx can't be re-deinit\n");
        return ;
    }

    HTTPThreadContext* pThreadCtx = *ppThreadCtx;
    if (pThreadCtx->bRun) {
        stop_http_thread(pThreadCtx->pFmtCtx);
    }

    pthread_mutex_destroy(&pThreadCtx->buf_lock);
    sem_destroy(&pThreadCtx->semReq);
    sem_destroy(&pThreadCtx->semReply);
    av_free(pThreadCtx);

    *ppThreadCtx = NULL;
}

void start_http_thread(AVFormatContext *s, void *pCtx, HTTPThreadCallBack pCB)
{
    if (!strcmp(s->iformat->name, "mov,mp4,m4a,3gp,3g2,mj2")) {
        MOVContext *mov = s->priv_data;
        HTTPThreadContext *pThreadCtx = (HTTPThreadContext*)mov->http_thread_ctx_ptr;

        av_log(NULL, AV_LOG_ERROR, "!!!start_http_thread\n");

        if (pThreadCtx) {
            if(!pCB || !pCtx) {
                av_log(NULL, AV_LOG_DEBUG, "!!!start_http_thread: invalid pCB=%p, pCtx=%p.\n", pCB, pCtx);
                return ;
            }

            HTTPThreadReq req;
            req.req = HTTP_THREAD_REQ_START;
            req.para1 = (int64_t)pCB;
            req.para2 = (int64_t)pCtx;
            http_thread_send_request(pThreadCtx, &req);
        } else{
            av_log(NULL, AV_LOG_ERROR, "!!!start_http_thread: pThreadCtx == NULL!!!\n");
        }
    }
}

void stop_http_thread(AVFormatContext *s)
{
    MOVContext *mov = s->priv_data;
    HTTPThreadContext *pThreadCtx = (HTTPThreadContext*)mov->http_thread_ctx_ptr;

    if (pThreadCtx) {
        HTTPThreadReq req;
        req.req = HTTP_THREAD_REQ_STOP;
        http_thread_send_request(pThreadCtx, &req);
    } else {
        av_log(NULL, AV_LOG_ERROR, "!!!stop_http_thread: pThreadCtx == NULL!!!\n");
    }
}

int read_http_thread(HTTPThreadContext *pThreadCtx, AVIOContext *s, int size)
{
    if (pThreadCtx) {
        HTTPThreadReq req;
        req.req = HTTP_THREAD_REQ_READ;
        req.para1 = size;
        http_thread_send_request(pThreadCtx, &req);
        return req.reply;
    } else {
        av_log(NULL, AV_LOG_ERROR, "!!!read_http_thread: pThreadCtx == NULL!!!\n");
        return -1;
    }
}

int64_t seek_http_thread(HTTPThreadContext *pThreadCtx, AVIOContext *s, int64_t offset)
{
    if (pThreadCtx) {
        HTTPThreadReq req;
        req.req = HTTP_THREAD_REQ_SEEK;
        req.para1 = offset;
        req.para2 = 0; // force_seek flag: 0 no; 1 yes;
        http_thread_send_request(pThreadCtx, &req);
        return req.reply;
    } else {
        av_log(NULL, AV_LOG_ERROR, "!!!seek_http_thread: pThreadCtx == NULL!!!\n");
        return -1;
    }
}

void* HTTPThreadProc(void *p)
{
    /**
     *  HTTP THREAD STD:  seven state
     *  1. default:
     *     HTTP_THREAD_STATE_START
     *
     *  2. HTTP_THREAD_STATE_START
     *     start_cmd    -> config params, then enter HTTP_THREAD_STATE_CACHE
     *     stop_cmd     -> HTTP_THREAD_STATE_END
     *
     *  3. HTTP_THREAD_STATE_CACHE
     *     stop_cmd     -> HTTP_THREAD_STATE_END
     *     seek_cmd     -> case 1: seek_point isn't in buf,
     *                          do seek op first, then re-enter HTTP_THREAD_STATE_CACHE
     *                     case 2: seek_point is in buf,
     *                          adjust buf pointer only
     *     cache_ok     -> HTTP_THREAD_STATE_NORMAL
     *     eos_flag     -> HTTP_THREAD_STATE_EOS
     *     error        -> HTTP_THREAD_STATE_ERROR
     *
     *  4. HTTP_THREAD_STATE_NORMAL
     *     stop_cmd     -> HTTP_THREAD_STATE_END
     *     seek_cmd     -> case 1: seek_point isn't in buf,
     *                          do seek op first, then re-enter HTTP_THREAD_STATE_CACHE
     *                     case 2: seek_point is in buf,
     *                          adjust buf pointer only
     *     less_data    -> config params, then enter HTTP_THREAD_STATE_CACHE
     *     eos_flag     -> HTTP_THREAD_STATE_EOS
     *     error        -> HTTP_THREAD_STATE_ERROR
     *
     *  5. HTTP_THREAD_STATE_ERROR
     *     stop_cmd     -> HTTP_THREAD_STATE_END
     *     seek_cmd     -> ignore
     *     recover_ok   -> HTTP_THREAD_STATE_NORMAL
     *     recovery_bad -> set recovery_timeout and wait stop_cmd only
     *
     *
     *  6. HTTP_THREAD_STATE_EOS
     *     stop_cmd     -> HTTP_THREAD_STATE_END
     *     seek_cmd     -> case 1: seek_point isn't in buf,
     *                             do seek op first, then re-enter HTTP_THREAD_STATE_CACHE
     *                     case 2: seek_point is in buf,
     *                             adjust buf pointer only
     *
     *  7. HTTP_THREAD_STATE_END
     *     deinit and exit ThreadProc
     *
     **/

    HTTPThreadContext *pThreadCtx = (HTTPThreadContext*) p;
    pThreadCtx->mState = HTTP_THREAD_STATE_START;
    do {
        #if STATE_DBG
        av_log(NULL, AV_LOG_ERROR, "enter state %s\n", g_dbg_state_name[pThreadCtx->mState]);
        #endif

        switch (pThreadCtx->mState) {
            case HTTP_THREAD_STATE_START:
                on_state_start(pThreadCtx);
                break;
            case HTTP_THREAD_STATE_CACHE:
                on_state_cache(pThreadCtx);
                break;
            case HTTP_THREAD_STATE_NORMAL:
                on_state_normal(pThreadCtx);
                break;
            case HTTP_THREAD_STATE_ERROR:
                on_state_error(pThreadCtx);
                break;
            case HTTP_THREAD_STATE_EOS:
                on_state_eos(pThreadCtx);
                break;
            case HTTP_THREAD_STATE_END:
                av_log(NULL, AV_LOG_ERROR, "!!!!!!HTTPThreadProc: shouldn't come in HTTP_THREAD_STATE_END\n");
                break;
            default:
                av_log(NULL, AV_LOG_ERROR, "!!!!!!HTTPThreadProc: invalid state %d\n", pThreadCtx->mState);
                break;
        }

        #if STATE_DBG
        av_log(NULL, AV_LOG_ERROR, "exit state %s\n", g_dbg_state_name[pThreadCtx->mState]);
        #endif
    } while (pThreadCtx->bRun);

	// cleanup
	av_log(NULL, AV_LOG_ERROR, "HTTPThreadProc exit\n");
	return NULL;
}

void on_state_start(HTTPThreadContext *pThreadCtx)
{
    /**
     *     HTTP_THREAD_STATE_START
     *     start_cmd    -> config params, then enter HTTP_THREAD_STATE_CACHE
     *     stop_cmd     -> HTTP_THREAD_STATE_END
     **/

    if (http_thread_wait_req(pThreadCtx) < 0) {
        av_log(NULL, AV_LOG_ERROR, "on_state_start: wait req error\n");
        return ;
    }

    if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_START)) {
        http_thread_handle_req(pThreadCtx);
    } else if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_STOP)) {
        http_thread_handle_req(pThreadCtx);
    } else {
        av_log(NULL, AV_LOG_ERROR, "on_state_start: !!!!!!unexpected req %d\n", pThreadCtx->pReq->req);
    }
}

void on_state_cache(HTTPThreadContext *pThreadCtx)
{
    /**
     *     HTTP_THREAD_STATE_CACHE
     *     stop_cmd     -> HTTP_THREAD_STATE_END
     *     seek_cmd     -> case 1: seek_point isn't in buf,
     *                             do seek op first, then re-enter HTTP_THREAD_STATE_CACHE
     *                     case 2: seek_point is in buf,
     *     cache_ok     -> HTTP_THREAD_STATE_NORMAL
     *     eos_flag     -> HTTP_THREAD_STATE_EOS
     *     error        -> HTTP_THREAD_STATE_ERROR
     **/

    if (http_thread_check_req(pThreadCtx)) {
        if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_STOP)) {
            http_thread_handle_req(pThreadCtx);
        } else if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_SEEK)) {
            http_thread_handle_req(pThreadCtx);
        }  else if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_READ)) {
            http_thread_handle_req(pThreadCtx);
        } else {
            av_log(NULL, AV_LOG_ERROR, "on_state_cache: !!!!!!unexpected req %d\n", pThreadCtx->pReq->req);
        }
        return ;
    }

    AVIOContext *s = pThreadCtx->pAVIO;
    int dataLen = get_buf_data_len(s);

    if (dataLen >= s->cache_data_size) {
        av_log(NULL, AV_LOG_ERROR, "on_state_cache: cache_ok: pos base=%lld ptr=%lld end=%lld dataLen=%d\n", s->pos_base, s->pos_ptr, s->pos, dataLen);
        pThreadCtx->mState = HTTP_THREAD_STATE_NORMAL;
        async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_END_BUFFERING);
        return ;
    }

    http_thread_fetch_loop(pThreadCtx);

    if (s->error) {
        av_log(NULL, AV_LOG_ERROR, "on_state_cache: error_flag=%d dataLen=%d\n", s->error, dataLen);

        if (s->error == AVERROR(ETIMEDOUT)) {
            av_log(NULL, AV_LOG_ERROR, "on_state_cache: !!!!!!recover network AVERROR(ETIMEDOUT)\n");
            pThreadCtx->recover_network_time_out = 1;
            async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_ERROR_UNFIXABLE);
        }

        pThreadCtx->mState = HTTP_THREAD_STATE_ERROR;
        return ;
    }

    if (s->eof_reached) {
        av_log(NULL, AV_LOG_ERROR, "on_state_cache: eos_flag dataLen=%d\n", dataLen);
        pThreadCtx->mState = HTTP_THREAD_STATE_EOS;
        async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_END_BUFFERING);
        return ;
    }

    async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_UPDATE_BUFFER);
}

void on_state_normal(HTTPThreadContext *pThreadCtx)
{
    /**
     *     HTTP_THREAD_STATE_NORMAL
     *     stop_cmd     -> HTTP_THREAD_STATE_END
     *     seek_cmd     -> case 1: seek_point isn't in buf,
     *                             do seek op first, then re-enter HTTP_THREAD_STATE_CACHE
     *                     case 2: seek_point is in buf,
     *                             adjust buf pointer only
     *     less_data    -> config params, then enter HTTP_THREAD_STATE_CACHE
     *     eos_flag     -> HTTP_THREAD_STATE_EOS
     *     error        -> HTTP_THREAD_STATE_ERROR
     **/

    if (http_thread_check_req(pThreadCtx)) {
        if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_STOP)) {
            http_thread_handle_req(pThreadCtx);
        } else if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_SEEK)) {
            http_thread_handle_req(pThreadCtx);
        } else if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_READ)) {
            http_thread_handle_req(pThreadCtx);
        } else {
            av_log(NULL, AV_LOG_ERROR, "on_state_normal: !!!!!!unexpected req %d\n", pThreadCtx->pReq->req);
        }
        return ;
    }

    AVIOContext *s = pThreadCtx->pAVIO;
    int dataLen = get_buf_data_len(s);

    if (dataLen < s->less_data_size) {
        av_log(NULL, AV_LOG_ERROR, "on_state_normal: !!!!!!less_data dataLen=%d less_data_size=%d\n", dataLen, s->less_data_size);
        update_buf_cache_size(pThreadCtx);
        pThreadCtx->mState = HTTP_THREAD_STATE_CACHE;
        async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_START_BUFFERING);
        return ;
    }

    http_thread_fetch_loop(pThreadCtx);

    if (s->error) {
        av_log(NULL, AV_LOG_ERROR, "on_state_normal: !!!!!!error_flag=%d dataLen=%d\n", s->error, dataLen);

        if (s->error == AVERROR(ETIMEDOUT)) {
            av_log(NULL, AV_LOG_ERROR, "on_state_cache: !!!!!!recover network AVERROR(ETIMEDOUT)\n");
            pThreadCtx->recover_network_time_out = 1;
        }

        pThreadCtx->mState = HTTP_THREAD_STATE_ERROR;
        return ;
    }

    if (s->eof_reached) {
        av_log(NULL, AV_LOG_ERROR, "on_state_normal: !!!!!!eos_flag dataLen=%d\n", dataLen);
        pThreadCtx->mState = HTTP_THREAD_STATE_EOS;
        return ;
    }
}

void on_state_error(HTTPThreadContext *pThreadCtx)
{
    /**
     *     HTTP_THREAD_STATE_ERROR
     *     stop_cmd     -> HTTP_THREAD_STATE_END
     *     seek_cmd     -> ignore
     *     recover_ok   -> HTTP_THREAD_STATE_NORMAL
     *     recovery_bad -> set recovery_timeout and wait stop_cmd only
     **/

    if (pThreadCtx->recover_network_time_out) {
        if (http_thread_wait_req(pThreadCtx) < 0) {
            av_log(NULL, AV_LOG_ERROR, "on_state_error: wait req error\n");
            return ;
        }

        if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_STOP)) {
            av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!stop req time_out\n");
            http_thread_handle_req(pThreadCtx);
        } else if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_SEEK)) {
            av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!seek req time_out\n");
            http_thread_send_reply(pThreadCtx, -1);
        } else if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_READ)) {
            av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!read req time_out\n");
            http_thread_send_reply(pThreadCtx, -1);
        } else {
            av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!unexpected req %d\n", pThreadCtx->pReq->req);
        }
        return ;
    } else {
        if (http_thread_check_req(pThreadCtx)) {
            if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_STOP)) {
                av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!stop req recover_network\n");
                http_thread_handle_req(pThreadCtx);
            } else if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_SEEK)) {
                av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!seek req recover_network\n");
                async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_ERROR_FOUND);
                http_thread_send_reply(pThreadCtx, AVERROR(EAGAIN));
            } else if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_READ)) {
                av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!read req recover_network\n");
                async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_ERROR_FOUND);
                http_thread_send_reply(pThreadCtx, AVERROR(EAGAIN));
            } else {
                av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!unexpected req %d\n", pThreadCtx->pReq->req);
            }
            return ;
        }

        int ret = http_thread_fetch_loop(pThreadCtx);
        if (ret > 0 || (ret == AVERROR(ECONNRESET))) {
            av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!recover network success\n");

            // clear recover parameters
            pThreadCtx->recover_network_time_out = 0;
            pThreadCtx->mState = HTTP_THREAD_STATE_NORMAL;
            async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_ERROR_FIXED);
            return ;
        } else if (ret == AVERROR(EAGAIN)) {
            av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!recover network AVERROR(EAGAIN)\n");
        } else if (ret == AVERROR(ETIMEDOUT)) {
            av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!recover network AVERROR(ETIMEDOUT)\n");
            pThreadCtx->recover_network_time_out = 1;
            async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_ERROR_UNFIXABLE);
        } else {
            av_log(NULL, AV_LOG_ERROR, "on_state_error: !!!!!!unhandled ret value=%d\n", ret);
        }

        usleep(10000);
    }
}

void on_state_eos(HTTPThreadContext *pThreadCtx)
{
    /**
     *     HTTP_THREAD_STATE_EOS
     *     stop_cmd     -> HTTP_THREAD_STATE_END
     *     seek_cmd     -> case 1: seek_point isn't in buf,
     *                             do seek op first, then re-enter HTTP_THREAD_STATE_CACHE
     *                     case 2: seek_point is in buf,
     *                             adjust buf pointer only
     **/

    if (http_thread_wait_req(pThreadCtx) < 0) {
        av_log(NULL, AV_LOG_ERROR, "on_state_eos: wait req error\n");
        return ;
    }

    if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_STOP)) {
        http_thread_handle_req(pThreadCtx);
    } else if (http_thread_is_req(pThreadCtx, HTTP_THREAD_REQ_SEEK)) {
        http_thread_handle_req(pThreadCtx);
    } else {
        av_log(NULL, AV_LOG_ERROR, "on_state_eos: unexpected req %d\n", pThreadCtx->pReq->req);
    }
}

int http_thread_wait_req(HTTPThreadContext *pThreadCtx)
{
    return sem_wait(&pThreadCtx->semReq);
}

int http_thread_check_req(HTTPThreadContext *pThreadCtx)
{
	return (sem_trywait(&pThreadCtx->semReq) == 0);
}

int http_thread_is_req(HTTPThreadContext *pThreadCtx, HTTP_THREAED_REQ req)
{
    HTTPThreadReq *pReq = pThreadCtx->pReq;
	return (pReq->req == req);
}

int64_t http_thread_send_reply(HTTPThreadContext *pThreadCtx, int64_t reply)
{
	HTTPThreadReq *pReq = pThreadCtx->pReq;
	pReq->reply = reply;
    sem_post(&pThreadCtx->semReply);
    return 0;
}

int64_t http_thread_send_request(HTTPThreadContext *pThreadCtx, HTTPThreadReq *pReq)
{
    pThreadCtx->pReq = pReq;
    sem_post(&pThreadCtx->semReq);
    sem_wait(&pThreadCtx->semReply);
    return pReq->reply;
}

int http_thread_handle_req(HTTPThreadContext *pThreadCtx)
{
	HTTPThreadReq *pReq = pThreadCtx->pReq;
    av_log(NULL, AV_LOG_ERROR, "http_thread_handle_req: !!!!!!mState=%d req=%d\n", pThreadCtx->mState, pReq->req);

    switch (pReq->req) {
        case HTTP_THREAD_REQ_START:
            http_thread_start_handler(pThreadCtx, pReq);
            break;
        case HTTP_THREAD_REQ_STOP:
            http_thread_stop_handler(pThreadCtx, pReq);
            break;
        case HTTP_THREAD_REQ_READ:
            http_thread_read_handler(pThreadCtx, pReq);
            break;
        case HTTP_THREAD_REQ_SEEK:
            http_thread_seek_handler(pThreadCtx, pReq);
            break;
        default:
            av_log(NULL, AV_LOG_ERROR, "http_thread_handle_req: !!!!!!uncovered case %d\n", pReq->req);
            break;
    }
	return 0;
}

int http_thread_start_handler(HTTPThreadContext *pThreadCtx, HTTPThreadReq *pReq)
{
    pThreadCtx->pCB = (HTTPThreadCallBack)pReq->para1;
    pThreadCtx->pCtx = (void*)pReq->para2;

    init_params(pThreadCtx);

    pThreadCtx->bRun = 1;
    pThreadCtx->mState = HTTP_THREAD_STATE_CACHE;
    async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_START_BUFFERING);

    http_thread_send_reply(pThreadCtx, 0);
    return 0;
}

int http_thread_stop_handler(HTTPThreadContext *pThreadCtx, HTTPThreadReq *pReq)
{
    pThreadCtx->bRun = 0;
    pThreadCtx->tid = -1;
    pThreadCtx->pCB = NULL;
    pThreadCtx->pCtx = NULL;

    pThreadCtx->mState = HTTP_THREAD_STATE_END;
    http_thread_send_reply(pThreadCtx, 0);
    return 0;
}

int http_thread_read_handler(HTTPThreadContext *pThreadCtx, HTTPThreadReq *pReq)
{
    // read
    // case 1: if data is enough, return immediately
    // case 2: else tell ap to wait buffering, then config cache_data_size and enter HTTP_THREAD_STATE_CACHE
    AVIOContext *s = pThreadCtx->pAVIO;
    int size = (int)pReq->para1;

    int dataLen = get_buf_data_len(s);
    if (size <= get_buf_data_len(s)) {
        dataLen = size;
        http_thread_send_reply(pThreadCtx, dataLen);
    } else {
        av_log(NULL, AV_LOG_ERROR, "!!!http_thread_read_handler: not enough data dataLen=%d size=%d\n", dataLen, size);
        pThreadCtx->mState = HTTP_THREAD_STATE_CACHE;
        update_buf_cache_size(pThreadCtx);
        async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_START_BUFFERING);
        http_thread_send_reply(pThreadCtx, AVERROR(EAGAIN));
    }

    return 0;
}

int http_thread_seek_handler(HTTPThreadContext *pThreadCtx, HTTPThreadReq *pReq)
{
    // seek
    // 1. check whether buffer needs flushing
    // 1.1 if Yes, flush buffer and update avio, then request thread to seek
    // 1.2 else,   fill buffer until it's enough for seek

    AVIOContext *s = pThreadCtx->pAVIO;
    int64_t pos = pReq->para1;

    int freeLen = get_buf_free_len(s);
    if (pos >= s->pos_base && pos <= s->pos) {
        av_log(NULL, AV_LOG_ERROR, "http_thread_seek_handler: ~~~case 1\n");
        inc_buf_rd(s, pos - s->pos_ptr);
    } else if (pos < s->pos_base ||
               pos > s->pos_ptr + s->seek_threshold ||
               pos > s->pos_ptr + freeLen) {
        av_log(NULL, AV_LOG_ERROR, "http_thread_seek_handler: ~~~case 2 s->pos_base=%lld s->pos=%lld pos=%lld diff=%lld\n", s->pos_base, s->pos, pos, pos - s->pos);

        if (s->seek(s->opaque, pos, SEEK_SET) < 0) {
            av_log(NULL, AV_LOG_ERROR, "http_thread_seek_handler: seek failure\n");
            http_thread_send_reply(pThreadCtx, -1);
            return -1;
        }

        s->eof_reached = 0;
        reset_buf_pos(s, pos);

        pThreadCtx->mState = HTTP_THREAD_STATE_CACHE;
        update_buf_cache_size(pThreadCtx);
        async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_START_BUFFERING);
    } else {
        av_log(NULL, AV_LOG_ERROR, "http_thread_seek_handler: ~~~case 3\n");
        pThreadCtx->mState = HTTP_THREAD_STATE_CACHE;
        update_buf_cache_size(pThreadCtx);
        async_notify_ap(pThreadCtx, HTTP_THREAD_MSG_START_BUFFERING);
    }

    http_thread_send_reply(pThreadCtx, pos);
    return 0;
}

int http_thread_fetch_loop(HTTPThreadContext *pThreadCtx)
{
    AVIOContext *s = pThreadCtx->pAVIO;

    /* no need to do anything if EOF already reached */
    if (s->eof_reached) {
        av_log(NULL, AV_LOG_ERROR, "http_thread_fetch_loop: has been EOF\n");
        return 0;
    }

    int len = get_buf_free_len(s);

#if HTTP_WR_DBG
    av_log(NULL, AV_LOG_ERROR, "wr: 0 pos base=%lld ptr=%lld end=%lld freeLen=%d\n", s->pos_base, s->pos_ptr, s->pos, len);
    av_log(NULL, AV_LOG_ERROR, "wr: 0 ptr base=%p ptr=%p end=%p\n", s->buf_base, s->buf_ptr, s->buf_end);
#endif

    // check whether need to move base
    if (len < s->less_free_size) {

        #if 0
        // check whether there's enough data
        len = get_buf_data_len(s);
        if (len >= 1*MB) {
            av_log(NULL, AV_LOG_ERROR, "http_thread_fetch_loop: enough data and needn't make space\n");
            usleep(100000);
            return 0;
        }
        #endif

        int64_t pos = (pThreadCtx->last_a_pos > pThreadCtx->last_v_pos) ? pThreadCtx->last_v_pos : pThreadCtx->last_a_pos;
        if (pos >= s->pos_base) {
            int64_t offset = (pos > s->pos_base + DEFAULT_BASE_MOVE_OFFSET) ? DEFAULT_BASE_MOVE_OFFSET : (pos - s->pos_base);
            if (offset == 0) {
                #if HTTP_WR_DBG
                av_log(NULL, AV_LOG_ERROR, "wr: buffer is full of data\n");
                #endif
                usleep(200000);
                return 0;
            }
            inc_buf_base(s, offset);
        } else {
            av_log(NULL, AV_LOG_ERROR, "http_thread_fetch_loop: shouldn't come in this case\n");
            return -1;
        }
    }

    len = get_buf_free_writable_len(s);

#if HTTP_WR_DBG
    av_log(NULL, AV_LOG_ERROR, "wr: 1 pos base=%lld ptr=%lld end=%lld len=%d\n", s->pos_base, s->pos_ptr, s->pos, len);
    av_log(NULL, AV_LOG_ERROR, "wr: 1 ptr base=%p ptr=%p end=%p buf_base=%p buf_end=%p\n", s->buf_base, s->buf_ptr, s->buf_end, s->buffer, s->buffer + s->buffer_size);
#endif

    // read data from http server
    len = s->read_packet(s->opaque, s->buf_end, len);

    if (len == 0) {
        if (s->pos >= pThreadCtx->pFmtCtx->file_size) {
            s->error = 0;
            s->eof_reached = 1;
            av_log(NULL, AV_LOG_ERROR, "http_thread_fetch_loop: reach EOF just now len=%d\n", len);
        } else {
            av_log(NULL, AV_LOG_ERROR, "http_thread_fetch_loop: network is slow, wait for a while and read again: pos=%lld file_size=%lld\n", s->pos, pThreadCtx->pFmtCtx->file_size);
            usleep(100000);
        }
    } else if (len == AVERROR_EOF) {
        s->error = 0;
        s->eof_reached = 1;
        av_log(NULL, AV_LOG_ERROR, "http_thread_fetch_loop: AVERROR_EOF pos=%lld file_size=%lld\n", s->pos, pThreadCtx->pFmtCtx->file_size);
    } else if (len < 0) {
        s->error = len;
        av_log(NULL, AV_LOG_ERROR, "http_thread_fetch_loop: errno=%d\n", len);
    } else {
        s->error = 0;
        pthread_mutex_lock(&pThreadCtx->buf_lock);
        inc_buf_wr(s, len);
        pthread_mutex_unlock(&pThreadCtx->buf_lock);
    }

#if HTTP_WR_DBG
    av_log(NULL, AV_LOG_ERROR, "wr: 2 pos base=%lld ptr=%lld end=%lld len=%d\n", s->pos_base, s->pos_ptr, s->pos, len);
    av_log(NULL, AV_LOG_ERROR, "wr: 2 ptr base=%p ptr=%p end=%p buf_base=%p buf_end=%p\n", s->buf_base, s->buf_ptr, s->buf_end, s->buffer, s->buffer + s->buffer_size);
#endif

	return len;
}

void async_notify_ap(HTTPThreadContext *pThreadCtx, int msg)
{
    AVIOContext *s = pThreadCtx->pAVIO;

    if (pThreadCtx->pCtx && pThreadCtx->pCB) {
        HTTPThreadParams params;
        params.msg = msg;
        params.curPos = s->pos;
        params.cacheSize = s->cache_data_size;
        params.dataLen = get_buf_data_len(s);

        pThreadCtx->pCB(pThreadCtx->pCtx, &params);
    } else {
        av_log(NULL, AV_LOG_ERROR, "!!!async_notify_ap: invalid params pCtx=%p pCB=%p\n",
            pThreadCtx->pCtx, pThreadCtx->pCB);
    }
}

int init_params(HTTPThreadContext *pThreadCtx)
{
    /**  Config Buffer Parameters
     *
     * 1. ip4s explictly
     *   a. config buf_size=IP4S_NET_IO_BUFFER_SIZE & seek_threshold=IP4S_NET_SHORT_SEEK_THRESHOLD
     *   b. pre-buffering IP4S_NET_PRE_CACHE_DATA_SIZE before play,
     *      then fetch IP4S_NET_ING_CACHE_DATA_SIZE each time when rd catchs up with wr
     *
     * 2. ip4 explictly
     *   a. config buf_size=IP4_NET_IO_BUFFER_SIZE & seek_threshold=IP4_NET_SHORT_SEEK_THRESHOLD
     *   b. work as old style
     *
     * 3. unknown
     *   3.1. bitrate >= MID_BITRATE_THRESHOLD
     *        a. config buf_size=MID_BITRATE_IO_BUFFER_SIZE & seek_threshold=MID_BITRATE_SHORT_SEEK_THRESHOLD
     *        b. pre-buffering MID_BITRATE_PRE_CACHE_DATA_SIZE before play,
     *           then fetch MID_BITRATE_ING_CACHE_DATA_SIZE each time when rd catchs up with wr
     *
     *   3.2. LOW_BITRATE_THRESHOLD <= bitrate < MID_BITRATE_THRESHOLD
     *        a. config buf_size=LOW_BITRATE_NET_IO_BUFFER_SIZE & seek_threshold=LOW_BITRATE_SHORT_SEEK_THRESHOLD
     *        b. work as old style
     *
     *   3.3. bitrate < LOW_BITRATE_THRESHOLD
     *        a. keep config as old
     *        b. work as old style
     **/

    BufParams bufParams;
    memset(&bufParams, 0, sizeof(bufParams));

    int bit_rate = pThreadCtx->pFmtCtx->bit_rate/1000000;
    if ((NULL != strstr(pThreadCtx->pFmtCtx->filename, "ip4s") ||
         NULL != strstr(pThreadCtx->pFmtCtx->filename, "ipad3"))) {
        bufParams.buf_size_update = 1;
        bufParams.buf_size = IP4S_NET_IO_BUFFER_SIZE;

        bufParams.seek_threshold_update = 1;
        bufParams.seek_threshold = IP4S_NET_SHORT_SEEK_THRESHOLD;

        bufParams.less_data_size_update = 1;
        bufParams.less_data_size = IP4S_NET_LESS_DATA_THRESHOLD;

        bufParams.less_free_size_update = 1;
        bufParams.less_free_size = DEFAULT_LESS_FREE_THRESHOLD;

        bufParams.cache_data_size_update = 1;
        bufParams.cache_data_size = IP4S_NET_PRE_CACHE_DATA_SIZE;

        av_log(NULL, AV_LOG_ERROR, "init_params: ip4s/ipad3 series bit_rate=%dMbps\n", bit_rate);

    } else if ((NULL != strstr(pThreadCtx->pFmtCtx->filename, "ip4") ||
                NULL != strstr(pThreadCtx->pFmtCtx->filename, "ipad") ||
                NULL != strstr(pThreadCtx->pFmtCtx->filename, "ipad2"))) {
        bufParams.buf_size_update = 1;
        bufParams.buf_size = IP4_NET_IO_BUFFER_SIZE;

        bufParams.seek_threshold_update = 1;
        bufParams.seek_threshold = IP4_NET_SHORT_SEEK_THRESHOLD;

        bufParams.less_data_size_update = 1;
        bufParams.less_data_size = IP4S_NET_LESS_DATA_THRESHOLD;

        bufParams.less_free_size_update = 1;
        bufParams.less_free_size = DEFAULT_LESS_FREE_THRESHOLD;

        bufParams.cache_data_size_update = 1;
        bufParams.cache_data_size = IP4_NET_PRE_CACHE_DATA_SIZE;

        av_log(NULL, AV_LOG_ERROR, "init_params: ip4/ipad/ipad2 series bit_rate=%dMbps\n", bit_rate);
    } else {
        if (bit_rate >= HIGH_BITRATE_THRESHOLD) {
            bufParams.buf_size_update = 1;
            bufParams.buf_size = HIGH_BITRATE_IO_BUFFER_SIZE;

            bufParams.seek_threshold_update = 1;
            bufParams.seek_threshold = HIGH_BITRATE_SHORT_SEEK_THRESHOLD;

            bufParams.less_data_size_update = 1;
            bufParams.less_data_size = HIGH_BITRATE_LESS_DATA_THRESHOLD;

            bufParams.less_free_size_update = 1;
            bufParams.less_free_size = DEFAULT_LESS_FREE_THRESHOLD;

            bufParams.cache_data_size_update = 1;
            bufParams.cache_data_size = HIGH_BITRATE_PRE_CACHE_DATA_SIZE;

            av_log(NULL, AV_LOG_ERROR, "init_params: high-bitrate series bitrate=%dMbps threshold>=%dMbps\n", bit_rate, HIGH_BITRATE_THRESHOLD);
        } else if (bit_rate >= MID_BITRATE_THRESHOLD) {
            bufParams.buf_size_update = 1;
            bufParams.buf_size = MID_BITRATE_NET_IO_BUFFER_SIZE;

            bufParams.seek_threshold_update = 1;
            bufParams.seek_threshold = MID_BITRATE_SHORT_SEEK_THRESHOLD;

            bufParams.less_data_size_update = 1;
            bufParams.less_data_size = MID_BITRATE_LESS_DATA_THRESHOLD;

            bufParams.less_free_size_update = 1;
            bufParams.less_free_size = DEFAULT_LESS_FREE_THRESHOLD;

            bufParams.cache_data_size_update = 1;
            bufParams.cache_data_size = MID_BITRATE_PRE_CACHE_DATA_SIZE;

            av_log(NULL, AV_LOG_ERROR, "init_params: middle-bitrate series bitrate=%dMbps %dMbps<=threshold<=%dMbps\n", bit_rate, MID_BITRATE_THRESHOLD, HIGH_BITRATE_THRESHOLD);
        } else {
            bufParams.buf_size_update = 1;
            bufParams.buf_size = LOW_BITRATE_NET_IO_BUFFER_SIZE;

            bufParams.seek_threshold_update = 1;
            bufParams.seek_threshold = LOW_BITRATE_SHORT_SEEK_THRESHOLD;

            bufParams.less_data_size_update = 1;
            bufParams.less_data_size = LOW_BITRATE_LESS_DATA_THRESHOLD;

            bufParams.less_free_size_update = 1;
            bufParams.less_free_size = DEFAULT_LESS_FREE_THRESHOLD;

            bufParams.cache_data_size_update = 1;
            bufParams.cache_data_size = LOW_BITRATE_PRE_CACHE_DATA_SIZE;

            av_log(NULL, AV_LOG_ERROR, "init_params: low-bitrate series bitrate=%dMbps threshold<%dMbps\n", bit_rate, MID_BITRATE_THRESHOLD);
        }
    }

    AVIOContext *s = pThreadCtx->pAVIO;
    if (config_buf_params(s, &bufParams) < 0) {
        av_log(NULL, AV_LOG_ERROR, "!!!init_params: config_buf_params failure\n");
        return -1;
    }

    return 0;
}

int update_buf_cache_size(HTTPThreadContext *pThreadCtx)
{
    BufParams bufParams;
    memset(&bufParams, 0, sizeof(bufParams));

    int bit_rate = pThreadCtx->pFmtCtx->bit_rate/1000000;
    if ((NULL != strstr(pThreadCtx->pFmtCtx->filename, "ip4s") ||
         NULL != strstr(pThreadCtx->pFmtCtx->filename, "ipad3"))) {
        bufParams.cache_data_size_update = 1;
        bufParams.cache_data_size = IP4S_NET_ING_CACHE_DATA_SIZE;
        av_log(NULL, AV_LOG_ERROR, "update_buf_cache_size: ip4s/ipad3 series bit_rate=%dMbps\n", bit_rate);
    } else if ((NULL != strstr(pThreadCtx->pFmtCtx->filename, "ip4") ||
                NULL != strstr(pThreadCtx->pFmtCtx->filename, "ipad") ||
                NULL != strstr(pThreadCtx->pFmtCtx->filename, "ipad2"))) {
        bufParams.cache_data_size_update = 1;
        bufParams.cache_data_size = IP4_NET_ING_CACHE_DATA_SIZE;

        av_log(NULL, AV_LOG_ERROR, "update_buf_cache_size: ip4/ipad/ipad2 series bit_rate=%dMbps\n", bit_rate);
    } else {
        if (bit_rate >= HIGH_BITRATE_THRESHOLD) {
            bufParams.cache_data_size_update = 1;
            bufParams.cache_data_size = HIGH_BITRATE_ING_CACHE_DATA_SIZE;

            av_log(NULL, AV_LOG_ERROR, "update_buf_cache_size: high-bitrate series bitrate=%dMbps threshold>=%dMbps\n", bit_rate, HIGH_BITRATE_THRESHOLD);
        } else if (bit_rate >= MID_BITRATE_THRESHOLD) {
            bufParams.cache_data_size_update = 1;
            bufParams.cache_data_size = MID_BITRATE_ING_CACHE_DATA_SIZE;

            av_log(NULL, AV_LOG_ERROR, "update_buf_cache_size: middle-bitrate series bitrate=%dMbps %dMbps<=threshold<=%dMbps\n", bit_rate, MID_BITRATE_THRESHOLD, HIGH_BITRATE_THRESHOLD);
        } else {
            bufParams.cache_data_size_update = 1;
            bufParams.cache_data_size = LOW_BITRATE_ING_CACHE_DATA_SIZE;

            av_log(NULL, AV_LOG_ERROR, "update_buf_cache_size: low-bitrate series bitrate=%dMbps threshold<%dMbps\n", bit_rate, MID_BITRATE_THRESHOLD);
        }
    }

    return config_buf_params(pThreadCtx->pAVIO, &bufParams);
}

int config_buf_params(AVIOContext *s, const BufParams *pBufParams)
{
    assert(s);
    assert(pBufParams);

    if (pBufParams->buf_size_update) {
        uint8_t *p_buf = av_malloc(pBufParams->buf_size);
        if (!p_buf) {
            av_log(NULL, AV_LOG_ERROR, "config_buf_params: av_malloc failure\n");
            return AVERROR(ENOMEM);
        }

        // !!!FIX ME: don't consider buffer-round case now
        int data_len = s->buf_end - s->buffer;
        int cur_pos_offset = s->buf_ptr - s->buffer;

        memmove(p_buf, s->buffer, data_len);
        av_free(s->buffer);
        s->buffer = p_buf;
        s->buffer_size = pBufParams->buf_size;
        s->buf_ptr = p_buf + cur_pos_offset;
        s->buf_end = p_buf + data_len;

        s->buf_base = s->buffer;
        s->pos_base = s->pos - data_len;
        s->pos_ptr = s->pos - (s->buf_end - s->buf_ptr);

        av_log(NULL, AV_LOG_ERROR, "config_buf_params: pos base=%lld ptr=%lld end=%lld data_len=%d\n",
            s->pos_base, s->pos_ptr, s->pos, data_len);
    }

    if (pBufParams->seek_threshold_update) {
        s->seek_threshold = pBufParams->seek_threshold;
    }

    if (pBufParams->less_data_size_update) {
        s->less_data_size = pBufParams->less_data_size;
    }

    if (pBufParams->less_free_size_update) {
        s->less_free_size = pBufParams->less_free_size;
    }

    if (pBufParams->cache_data_size_update) {
        s->cache_data_size = pBufParams->cache_data_size;
    }

    return 0;
}

AVInputFormat ff_mov_demuxer = {
    "mov,mp4,m4a,3gp,3g2,mj2",
    NULL_IF_CONFIG_SMALL("QuickTime/MPEG-4/Motion JPEG 2000 format"),
    sizeof(MOVContext),
    mov_probe,
    mov_read_header,
    mov_read_packet,
    mov_read_close,
    mov_read_seek,
};
