/*
 * MP4 demuxer for video recorded by AMBA
 *
 * Copyright (c) 2010 <Ambarella>
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


#include "isom.h"
#include "mp4_dv.h"
#include "libavcodec/mpeg4audio.h"

/* links atom IDs to parse functions */
typedef struct SMP4ParseTableEntry {
    uint32_t type;
    int (*parse)(SMP4Context *ctx, AVIOContext *pb, SMP4Atom atom);
} SMP4ParseTableEntry;

static const SMP4ParseTableEntry dv_mp4_parse_table[];

static int dv_read_uuid_type(AVIOContext* pb)
{
    uint32_t uuid = 0;



    return uuid;
}

int dv_mp4_read_descr_len(AVIOContext *pb)
{
    int len = 0;
    int count = 4;
    while (count--) {
        int c = avio_r8(pb);
        len = (len << 7) | (c & 0x7f);
        if (!(c & 0x80))
            break;
    }
    return len;
}

int dv_mp4_60p_playmode[ ][60] =
{
{0}
};
int dv_mp4_30p_playmode[ ][40] =
{
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,       1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, //normal playback ,see bugnote 1
    {1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0}//index for backward play
};

static const AVCodecTag dv_mp4_audio_types[] = {
    { CODEC_ID_MP3ON4, AOT_PS   }, /* old mp3on4 draft */
    { CODEC_ID_MP3ON4, AOT_L1   }, /* layer 1 */
    { CODEC_ID_MP3ON4, AOT_L2   }, /* layer 2 */
    { CODEC_ID_MP3ON4, AOT_L3   }, /* layer 3 */
    { CODEC_ID_MP4ALS, AOT_ALS  }, /* MPEG-4 ALS */
    { CODEC_ID_NONE,   AOT_NULL },
};

int dv_mp4_read_descr(AVFormatContext *fc, AVIOContext *pb, int *tag)
{
    int len;
    *tag = avio_r8(pb);
    len = dv_mp4_read_descr_len(pb);
    //dprintf(fc, "MPEG4 description: tag=0x%02x len=%d\n", *tag, len);
    return len;
}


int ff_dv_read_esds(AVFormatContext *fc, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    int tag, len;

    if (fc->nb_streams < 1)
        return 0;
    st = fc->streams[fc->nb_streams-1];

    avio_rb32(pb); /* version + flags */
    len = dv_mp4_read_descr(fc, pb, &tag);/* tag: 03h  len: 19h ==25*/
    if (tag == SMP4ESDescrTag) {
        avio_rb16(pb); /* ID */
        avio_r8(pb); /* priority */
    } else
        avio_rb16(pb); /* ID */

    len = dv_mp4_read_descr(fc, pb, &tag);
    if (tag == SMP4DecConfigDescrTag) { /* tag: 04h  len: 11h ==17*/
        int object_type_id = avio_r8(pb);
        avio_r8(pb); /* stream type */
        avio_rb24(pb); /* buffer size db */
        avio_rb32(pb); /* max bitrate */
        avio_rb32(pb); /* avg bitrate */

        //st->codec->codec_id= ff_codec_get_id(ff_mp4_obj_type, object_type_id);
        if(object_type_id != 0x40)
        {
            av_log(NULL, AV_LOG_ERROR, "c");
            return -1;
        }
        //dprintf(fc, "esds object type id %d\n", object_type_id);
        len = dv_mp4_read_descr(fc, pb, &tag); /* tag: 05h  len: 02h ==2*/
        if (tag == SMP4DecSpecificDescrTag) {
            //dprintf(fc, "Specific MPEG4 header len=%d\n", len);
            if((uint64_t)len > (1<<30))
                return -1;
            st->codec->extradata = av_mallocz(len + FF_INPUT_BUFFER_PADDING_SIZE);
            if (!st->codec->extradata)
                return AVERROR(ENOMEM);
            avio_read(pb, st->codec->extradata, len);
            st->codec->extradata_size = len;
            if (st->codec->codec_id == CODEC_ID_AAC) {
                MPEG4AudioConfig cfg;
                ff_mpeg4audio_get_config(&cfg, st->codec->extradata,
                                         st->codec->extradata_size);
                if (cfg.chan_config > 7)
                    return -1;
#if 0
                st->codec->channels = ff_mpeg4audio_channels[cfg.chan_config];
                if (cfg.object_type == 29 && cfg.sampling_index < 3) // old mp3on4
                    st->codec->sample_rate = ff_mpa_freq_tab[cfg.sampling_index];
                else
                    st->codec->sample_rate = cfg.sample_rate; // ext sample rate ?
                //dprintf(fc, "mp4a config channels %d obj %d ext obj %d "
                        "sample rate %d ext sample rate %d\n", st->codec->channels,
                        cfg.object_type, cfg.ext_object_type,
                        cfg.sample_rate, cfg.ext_sample_rate);
#endif
                if (!(st->codec->codec_id = ff_codec_get_id(dv_mp4_audio_types,
                                                            cfg.object_type)))
                    st->codec->codec_id = CODEC_ID_AAC;
            }
        }
        //SLConfigDescriptor
        len = dv_mp4_read_descr(fc, pb, &tag);/* tag: 06h  len: 01h ==1*/
        avio_r8(pb);
    }
    return 0;
}

/* this atom contains actual media data */
static int dv_mp4_read_mdat(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    if(atom.size == 0) /* wrong one (MP4) */
        return 0;
    c->found_mdat=1;
    return 0; /* now go for moov */
}

/* read major brand, minor version and compatible brands and store them as metadata */
static int dv_mp4_read_ftyp(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    uint32_t minor_ver;
    int comp_brand_size;
    char minor_ver_str[11]; /* 32 bit integer -> 10 digits + null */
    char* comp_brands_str;
    uint8_t major[5] = {0};

    avio_read(pb, major, 4);
    av_log(c->fc, AV_LOG_DEBUG, "ISO: File Type Major Brand: %.4s\n",(char *)&major);
    av_metadata_set2(&c->fc->metadata, "major_brand", major, 0);

    /* we can check again here */
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

/* read the top uuid */
static int dv_mp4_read_uuid_top(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    uint32_t left;
    left = atom.size - (avio_tell(pb) - atom.offset);
    if(left > 0)
        avio_skip(pb, left);
    return 0;

}

//excluding the size and type fields, we read the usertype first:
// 3DDS for video stream. USMT for metadata
static int dv_mp4_read_uuid(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    uint32_t uuid;
    AVStream *st;
    SMP4StreamContext* sc;

    return 0;
    if (c->fc->nb_streams < 1)
        return -1;
    //st = c->fc->streams[c->fc->nb_streams-1];
    //sc = st->priv_data;
    uuid = dv_read_uuid_type(pb);
    if(uuid == MKTAG('U', 'S', 'M', 'T'))
    {
        uint32_t size, type;
        uint32_t num, id, i, encoding_type;
        size = avio_rb32(pb);
        type = avio_rl32(pb);
        if(type == MKTAG('M', 'T', 'D', 'T'))
        {
            //User Specific MetaData in track
            st = c->fc->streams[c->fc->nb_streams-1];
            num = avio_rb16(pb);/*should be 1*/
            for(i = 0; i < num; i++)
            {
                avio_rb16(pb); /*data size*/
                id = avio_rb32(pb); /*should be 0xA*/
                avio_rb16(pb);
                encoding_type = avio_rb16(pb);
                avio_rb32(pb);
                avio_rb16(pb);
                avio_rb16(pb);
             }
        }else if (type == MKTAG('M', 'T', 'D', 'F')){
            //user data with in moov box
            num = avio_rb16(pb);/*should be 1*/
            avio_rb16(pb); /*data size*/
            id = avio_rb32(pb); /*should be 0xA*/



        }

    }else if (uuid == MKTAG('3', 'D', 'D', 'S')){



    }else{



    }

    //skip by read_default
    return 0;
}

static int dv_mp4_read_default(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    int64_t total_size = 0;
    SMP4Atom a;
    int i;
    int err = 0;

    a.offset = atom.offset;
    while(((total_size + 8) < atom.size) && !url_feof(pb) && !err)
    {
        int (*parse)(SMP4Context*, AVIOContext*, SMP4Atom) = NULL;
        a.size = avio_rb32(pb);
        a.type = avio_rl32(pb);
        total_size += 8;
        a.offset += 8;
        av_log(NULL, AV_LOG_INFO, "type: %08x  %.4s  sz: %"PRIx64"  %"PRIx64"   %"PRIx64"\n",
                a.type, (char*)&a.type, a.size, atom.size, total_size);

        if (a.size == 1) { /* 64 bit extended size. a.size-8 let size no include this 8 bytes.*/
            a.size = avio_rb64(pb) - 8;
            a.offset += 8;
            total_size += 8;
        }
        if (a.size == 0) { /*box extends to end of file */
            //a.size = atom.size - total_size;
            break;
        }
        a.size -= 8;
        if(a.size < 0)
            break;
        a.size = FFMIN(a.size, atom.size - total_size);

        for (i = 0; dv_mp4_parse_table[i].type; i++)
            if (dv_mp4_parse_table[i].type == a.type)
            {
                parse = dv_mp4_parse_table[i].parse;
                break;
            }

        // container is user data, here parse the trak's uuid
        if (!parse && (a.type == MKTAG('u','u','i','d')))
            parse = dv_mp4_read_uuid;

        if (!parse) { /* skip leaf atoms data, such as free box*/
            avio_skip(pb, a.size);
        } else {
            int64_t start_pos = avio_tell(pb);
            int64_t left;
            err = parse(c, pb, a);
            if(err < 0)
            {
                //do something;may no return.
            }
            if (c->found_moov && c->found_mdat)
                break;
            left = a.size - avio_tell(pb) + start_pos;
            if (left > 0) /* skip garbage at atom end */
                avio_skip(pb, left);
        }

        a.offset += a.size;
        total_size += a.size;
    }

    if (!err && total_size < atom.size && atom.size < 0x7ffff)
        avio_skip(pb, atom.size - total_size);

    return err;
}

/* this atom should contain all header atoms */
static int dv_mp4_read_moov(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    if (dv_mp4_read_default(c, pb, atom) < 0)
        return -1;
    /* we parsed the 'moov' atom, we can terminate the parsing as soon as we find the 'mdat' */
    /* so we don't parse the whole file if over a network */
    c->found_moov=1;
    return 0; /* now go for mdat */
}

static int dv_mp4_read_mvhd(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    int version = avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */


    avio_rb32(pb); /* creation time */
    avio_rb32(pb); /* modification time */
    c->time_scale = avio_rb32(pb); /* time scale */
    if(c->time_scale == 90000)
    {
        av_log(NULL, AV_LOG_INFO, "FPS: 29.97p\n");
        c->video_gop_size = 15;
    }else if(c->time_scale == 60000){
        av_log(NULL, AV_LOG_INFO, "FPS: 59.94p\n");
        c->video_gop_size = 30;
    }else//TODO
        c->video_gop_size = 12;
        //return -1;
    c->duration = avio_rb32(pb); /* duration */
    //dprintf(c->fc, "time scale = %i\n", c->time_scale);

    avio_rb32(pb); /* preferred rate */
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

static void dv_mp4_build_index(SMP4Context *mov, AVStream *st);

static int dv_mp4_read_trak(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    SMP4StreamContext *sc;
    int ret;

    st = av_new_stream(c->fc, c->fc->nb_streams);
    if (!st) return AVERROR(ENOMEM);
    sc = av_mallocz(sizeof(SMP4StreamContext));
    if (!sc) return AVERROR(ENOMEM);

    st->priv_data = sc;
    st->codec->codec_type = AVMEDIA_TYPE_DATA;
    sc->ffindex = st->index;

    if ((ret = dv_mp4_read_default(c, pb, atom)) < 0)
        return ret;

    /* sanity checks */
    if (sc->chunk_count && (!sc->stts_count || !sc->stsc_count ||
                            (!sc->sample_size && !sc->sample_count))) {
        av_log(c->fc, AV_LOG_ERROR, "stream %d, missing mandatory atoms, broken header\n",
               st->index);
        return -1;
    }

    if (!sc->time_scale)
        sc->time_scale = c->time_scale;
    av_set_pts_info(st, 64, 1, sc->time_scale);

    if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
        !st->codec->frame_size /*&& sc->stts_count == 1*/) {
        st->codec->frame_size = av_rescale(sc->stts_data[0].duration,
                                           st->codec->sample_rate, sc->time_scale);
        //dprintf(c->fc, "frame size %d\n", st->codec->frame_size);
    }
    //sc->chunk_index = 1;
    dv_mp4_build_index(c, st);

    sc->pb = c->fc->pb;
    if(st->codec->codec_id == CODEC_ID_H264)
    {
        if (sc->stts_count == 1 || (sc->stts_count == 2 && sc->stts_data[1].count == 1))
            av_reduce(&st->r_frame_rate.num, &st->r_frame_rate.den,
                      sc->time_scale, sc->stts_data[0].duration, INT_MAX);
        //TODO, this is done optionally, just let ffmpeg_demuxer filter donot sikp this video stream
        st->codec->pix_fmt = PIX_FMT_YUV420P;
        sc->stss_index = 1;//TODO, init stream in where
    }
    if(st->codec->codec_id == CODEC_ID_AAC)
    {
        switch(st->codec->bits_per_coded_sample)
        {
        case 16:
            st->codec->sample_fmt = AV_SAMPLE_FMT_S16;
            break;
        default:
            av_log(NULL, AV_LOG_INFO, "AAC Sample size not equal 16(%d), Set to 16 for default!\n", st->codec->bits_per_coded_sample);
            st->codec->sample_fmt = AV_SAMPLE_FMT_S16;
        }
    }
    /* Do not need those anymore. */
    av_freep(&sc->chunk_offsets);
    av_freep(&sc->stsc_data);
    //av_freep(&sc->sample_sizes);
    //av_freep(&sc->keyframes);
    //av_freep(&sc->stts_data);
    //av_freep(&sc->stps_data);

    return 0;
}

//return 0 will let skip this atom and continue parse left atom. return -1 will let leave while loop, result in stop up-level
//atom parsing stop
static int dv_mp4_read_tkhd(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    int i, flags;
    int width;
    int height;
    int display_matrix[3][2];
    int64_t disp_transform[2];


    AVStream *st;
    SMP4StreamContext *sc;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_r8(pb); /*verson*/
    flags =avio_rb24(pb); /* flags, should be 7*/
    if(flags != 7)
        av_log(NULL, AV_LOG_WARNING, "c");
    /*
    MOV_TRACK_ENABLED 0x0001
    MOV_TRACK_IN_MOVIE 0x0002
    MOV_TRACK_IN_PREVIEW 0x0004
    MOV_TRACK_IN_POSTER 0x0008
    */
    avio_rb32(pb); /* creation time */
    avio_rb32(pb); /* modification time */
    st->id = (int)avio_rb32(pb); /* track id (NOT 0 !)*/
    avio_rb32(pb); /* reserved */

    /* highlevel (considering edits) duration in movie timebase */
    avio_rb32(pb);
    avio_rb32(pb); /* reserved */
    avio_rb32(pb); /* reserved */

    avio_rb16(pb); /* layer */
    avio_rb16(pb); /* alternate group */
    avio_rb16(pb); /* volume */
    avio_rb16(pb); /* reserved */

    for (i = 0; i < 3; i++) {
        display_matrix[i][0] = avio_rb32(pb);   // 16.16 fixed point
        display_matrix[i][1] = avio_rb32(pb);   // 16.16 fixed point
        avio_rb32(pb);   // 2.30 fixed point (not used)
    }

    width = avio_rb32(pb);       // 16.16 fixed point track width, here we can calu the 1080 or 720
    height = avio_rb32(pb);      // 16.16 fixed point track height
    sc->width = width >> 16;
    sc->height = height >> 16;

    // transform the display width/height according to the matrix
    // skip this if the display matrix is the default identity matrix
    // or if it is rotating the picture, ex iPhone 3GS
    // to keep the same scale, use [width height 1<<16]
    if (width && height &&
        ((display_matrix[0][0] != 65536  ||
          display_matrix[1][1] != 65536) &&
         !display_matrix[0][1] &&
         !display_matrix[1][0] &&
         !display_matrix[2][0] && !display_matrix[2][1])) {
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

    return 0;
}

/* edit list atom, store those infos in each ssc->time_offset(just first elst)*/
static int dv_mp4_read_elst(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    SMP4StreamContext *sc;
    int i, edit_count;

    if (c->fc->nb_streams < 1)
        return 0;
    sc = c->fc->streams[c->fc->nb_streams-1]->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    edit_count = avio_rb32(pb); /* entries should be 1*/

    if(edit_count != 1)
        av_log(NULL, AV_LOG_WARNING, "c");

    for(i=0; i<edit_count; i++){
        int time;
        int duration = avio_rb32(pb); /* Track duration */
        time = avio_rb32(pb); /* Media time, for video is 1001*/
        avio_rb32(pb); /* Media rate */
        if (i == 0 && time >= -1) {
            sc->time_offset = time != -1 ? time : -duration;
        }
    }

    //dprintf(c->fc, "track[%i].edit_count = %i\n", c->fc->nb_streams-1, edit_count);
    return 0;
}

static int dv_mp4_read_mdhd(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    SMP4StreamContext *sc;
    int version;
    char language[4] = {0};
    unsigned lang;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];/*the nearest avstream, we create it when meet a trak box */
    sc = st->priv_data;

    version = avio_r8(pb);
    if (version > 0)
        return -1; /* unsupported */

    avio_rb24(pb); /* flags */
    avio_rb32(pb); /* creation time */
    avio_rb32(pb); /* modification time */

    sc->time_scale = avio_rb32(pb); /*here we can calu the 30p or 60p */
    st->duration = avio_rb32(pb); /* duration */

    lang = avio_rb16(pb); /* language */
    if (ff_mov_lang_to_iso639(lang, language))
        av_metadata_set2(&st->metadata, "language", language, 0);
    avio_rb16(pb); /* quality */

    return 0;
}

static int dv_mp4_read_hdlr(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    uint32_t type;

    if (c->fc->nb_streams < 1) // meta before first trak
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    avio_rl32(pb); /* pre_defined */
    type = avio_rl32(pb); /* component subtype */
    //dprintf(c->fc, "stype= %.4s\n", (char*)&type);

    if(type == MKTAG('v','i','d','e'))
        st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    else if(type == MKTAG('s','o','u','n'))
        st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    else
        return -1;

    avio_rb32(pb); /* component  manufacture */
    avio_rb32(pb); /* component flags */
    avio_rb32(pb); /* component flags mask */

    //may no need
    //avio_skip(pb, atom.size - (avio_tell(pb) - atom.offset));
    return 0;
}

static int dv_mp4_read_vmhd(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    uint32_t flags;

    avio_r8(pb); /*version*/
    flags = avio_rb24(pb); //000001h
    avio_rb16(pb); /* graphics mode 0000h*/
    /*opcolor*/
    avio_rb16(pb); //r 0000
    avio_rb16(pb); //g 0000
    avio_rb16(pb); //b 0000

    return 0;
}

static int dv_mp4_read_smhd(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    int balance;
    avio_rb32(pb); //version and flags
    balance = avio_rb16(pb); /* should be 0, for center*/
    avio_rb16(pb);
    return 0;
}

static int dv_mp4_read_dref(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    /*should be only 1 entry, and url flags be 000001h*/
    AVStream *st;
    SMP4StreamContext *sc;
    int entries, i, j;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_rb32(pb); // version + flags for dref container dref
    entries = avio_rb32(pb);
    if (entries != 1)
        av_log(NULL, AV_LOG_ERROR, "c");
    sc->drefs = av_mallocz(entries * sizeof(*sc->drefs));
    if (!sc->drefs)
        return AVERROR(ENOMEM);
    sc->drefs_count = entries;

    for (i = 0; i < sc->drefs_count; i++) {
        SMP4Dref *dref = &sc->drefs[i];
        uint32_t size = avio_rb32(pb);
        int64_t next = avio_tell(pb) + size - 4;

        dref->type = avio_rl32(pb); /*should be url box */
        avio_r8(pb); // version
        avio_rb24(pb); /*flags , should be 000001h*/
        //dprintf(c->fc, "type %.4s size %d\n", (char*)&dref->type, size);
        avio_seek(pb, next, SEEK_SET);
    }
    return 0;
}

static int dv_mp4_read_stsd(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    SMP4StreamContext *sc;
    int entries, stream_id;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    //int64_t stsd_pos = avio_tell(pb);
    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    entries = avio_rb32(pb); /* should be 1 */

    for(stream_id=0; stream_id<entries; stream_id++) {
        //Parsing Sample description table
        enum CodecID id;
        int dref_id = 1;
        SMP4Atom a = { 0, 0, 0 };
        int64_t start_pos = avio_tell(pb);
        int size = avio_rb32(pb); /* size */
        uint32_t format = avio_rl32(pb); /* data format */

        avio_rb32(pb); /* reserved */
        avio_rb16(pb); /* reserved */
        dref_id = avio_rb16(pb); /*should be 1*/
        sc->dref_id= dref_id; /*no used, we use local file only*/
        sc->pseudo_stream_id = 0;
        st->codec->codec_tag = format;
        if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if(format != MKTAG('a', 'v', 'c', '1'))
                av_log(NULL, AV_LOG_ERROR, "C");
            id = CODEC_ID_H264;
            sc->index_by_chunk = 1;
        }else if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            if(format != MKTAG('m', 'p', '4', 'a'))
                av_log(NULL, AV_LOG_ERROR, "C");
            id = CODEC_ID_AAC;
            sc->index_by_chunk = 0;
        }

        //dprintf(c->fc, "size=%d 4CC= %c%c%c%c codec_type=%d\n", size,
                //(format >> 0) & 0xff, (format >> 8) & 0xff, (format >> 16) & 0xff,
                //(format >> 24) & 0xff, st->codec->codec_type);

        if(st->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            uint8_t codec_name[32];

            st->codec->codec_id = id;
            avio_rb16(pb); /* pre_defined */
            avio_rb16(pb); /* reserved */
            avio_rb32(pb); /* pre_defined */
            avio_rb32(pb); /* pre_defined */
            avio_rb32(pb); /* pre_defined */

            /*set 0 in read trak, but seem can be asign */
            st->codec->width = avio_rb16(pb); /* width diff 1080 from 720 and 270*/
            st->codec->height = avio_rb16(pb); /* height */
/*
            if(st->codec->width == 0x01E0)
            {
                av_log(NULL, AV_LOG_INFO, "Video: 270p\n");
                c->video_type = VIDEO_270_30P;
            }else if(st->codec->width == 0x0500){
                av_log(NULL, AV_LOG_INFO, "Video: 720p\n");
                if(c->video_gop_size == 15)
                    c->video_type = VIDEO_720_30P;
                else
                    c->video_type = VIDEO_720_60P;
            }else if(st->codec->width == 0x0780){
                av_log(NULL, AV_LOG_INFO, "Video: 1080p\n");
                c->video_type = VIDEO_1080_30P;
            }else
                return -1;
*/
            avio_rb32(pb); /* horiz resolution */
            avio_rb32(pb); /* vert resolution */
            avio_rb32(pb); /* reserved, always 0 */
            avio_rb16(pb); /* frames per samples */

            avio_read(pb, codec_name, 32); /* codec name, pascal string, the first byte be seted to the length */
            if (codec_name[0] <= 31) {
                memcpy(st->codec->codec_name, &codec_name[1],codec_name[0]);
                st->codec->codec_name[codec_name[0]] = 0;
            }

            st->codec->bits_per_coded_sample = avio_rb16(pb); /* depth should be 0x0018, means RGB24, no palettized*/
            st->codec->color_table_id = avio_rb16(pb); /* colortable id */
            //dprintf(c->fc, "depth %d, ctab id %d\n",
                 //  st->codec->bits_per_coded_sample, st->codec->color_table_id);

            /*next is the AVCConfiguration */

        } else if(st->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
            int bits_per_sample, flags;
            st->codec->codec_id = id;

            avio_rb16(pb); /*just 0*/
            avio_rb16(pb); /* revision level */
            avio_rb32(pb); /* vendor */

            st->codec->channels = avio_rb16(pb);             /* channel count */
            //dprintf(c->fc, "audio channels %d\n", st->codec->channels);
            st->codec->bits_per_coded_sample = avio_rb16(pb);      /* sample size */
            //av_log(NULL, AV_LOG_INFO, "\n\n st->codec->bits_per_coded_sample%d \n",  st->codec->bits_per_coded_sample);
            //sc->audio_cid = avio_rb16(pb);
            avio_rb16(pb);
            avio_rb16(pb); /* packet size = 0 */
            st->codec->sample_rate = ((avio_rb32(pb) >> 16));

            //left esds: Elementary Stream Descriptor
        }else{
            /* other codec type, just skip (rtp, mp4s, tmcd ...) */
            avio_skip(pb, size - (avio_tell(pb) - start_pos));
        }

        /* this will read extra atoms at the end of stsd (avcC for video, esds for audio) */
        a.size = size - (avio_tell(pb) - start_pos);
        if (a.size > 8){
            if (dv_mp4_read_default(c, pb, a) < 0)
                return -1;
        }else if(a.size > 0)
            avio_skip(pb, a.size);
    }
    //avio_skip(pb, atom.size - (avio_tell(pb) - stsd_pos));
    return 0;
}

static int dv_mp4_read_avvc(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVFormatContext* fc = c->fc;
    AVStream *st;
    int tag, len;

    if (fc->nb_streams < 1)
        return -1;
    st = fc->streams[fc->nb_streams-1];

        if((uint64_t)atom.size > (1<<30))
        return -1;

    av_free(st->codec->extradata);
        st->codec->extradata = av_mallocz(atom.size + FF_INPUT_BUFFER_PADDING_SIZE);
    if (!st->codec->extradata)
        return AVERROR(ENOMEM);
    st->codec->extradata_size = atom.size;
    avio_read(pb, st->codec->extradata, atom.size);
    return 0;


/*
    avio_rb32(pb); /*version + flags */
//    avio_r8(pb);
//    avio_r8(pb);
//
//    avio_r8(pb); /*AVCProfileIndication */
//    avio_r8(pb); /*profile_compatibility */
//    avio_r8(pb);/*AVCLevelIndication*/
//    avio_rb16(pb);


    /* this is called by read_default, which atom.size is the length of avvc box,
        it will be skip in read_default if we do not reach the end of box */
   /* return 0; */

}


static int dv_mp4_read_esds(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    return ff_dv_read_esds(c->fc, pb, atom);
}

static int dv_mp4_read_ctts(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    SMP4StreamContext *sc;
    unsigned int i, entries;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    entries = avio_rb32(pb);

    //dprintf(c->fc, "track[%i].ctts.entries = %i\n", c->fc->nb_streams-1, entries);

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
    }
    return 0;
}

//using this box to calu the stream duration!
static int dv_mp4_read_stts(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    SMP4StreamContext *sc;
    unsigned int i, entries;
    int64_t duration=0;
    int64_t total_sample_count=0;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */
    entries = avio_rb32(pb); /* should be 1*/

    //dprintf(c->fc, "track[%i].stts.entries = %i\n", c->fc->nb_streams-1, entries);

    if(entries != 1)
    {
        av_log(NULL, AV_LOG_WARNING, "Should be checked in muxer, AmbaDV mp4 should contain only one STTS box.\n");
        //return -1;
    }
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

        //dprintf(c->fc, "sample_count=%d, sample_duration=%d\n",sample_count,sample_duration);

        duration+=(int64_t)sample_duration*sample_count;
        total_sample_count+=sample_count;
    }

    st->nb_frames= total_sample_count;
    if(duration)
        st->duration= duration;
    return 0;
}

static int dv_mp4_read_stsc(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    SMP4StreamContext *sc;
    unsigned int i, entries;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    entries = avio_rb32(pb);

    //dprintf(c->fc, "track[%i].stsc.entries = %i\n", c->fc->nb_streams-1, entries);

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


static int dv_mp4_read_stco(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    SMP4StreamContext *sc;
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

    if(atom.type == MKTAG('s','t','c','o'))
        for(i=0; i<entries; i++)
            sc->chunk_offsets[i] = avio_rb32(pb);
    else
        return -1;

    return 0;
}

static int dv_mp4_read_stsz(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    SMP4StreamContext *sc;
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
        return -1;
    }
    entries = avio_rb32(pb);

    //dprintf(c->fc, "sample_size = %d sample_count = %d\n", sc->sample_size, entries);

    sc->sample_count = entries;
    if (sample_size)
        return 0;

    if (entries >= UINT_MAX / sizeof(int) || entries >= (UINT_MAX - 4) / field_size)
        return -1;
    sc->sample_sizes = av_malloc(entries * sizeof(int));
    if (!sc->sample_sizes)
        return AVERROR(ENOMEM);

    //each entry is 4 bytes
    num_bytes = (entries * field_size)>>3;

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

static int dv_mp4_read_stss(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    SMP4StreamContext *sc;
    unsigned int i, entries;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_r8(pb); /* version */
    avio_rb24(pb); /* flags */

    entries = avio_rb32(pb);

    //dprintf(c->fc, "keyframe_count = %d\n", entries);

    if(entries >= UINT_MAX / sizeof(int))
        return -1;
    sc->keyframes = av_malloc(entries * sizeof(int));
    if (!sc->keyframes)
        return AVERROR(ENOMEM);
    sc->keyframe_count = entries;

    for(i=0; i<entries; i++) {
        sc->keyframes[i] = avio_rb32(pb);
        ////dprintf(c->fc, "keyframes[]=%d\n", sc->keyframes[i]);
    }
    return 0;
}

static int dv_mp4_read_cslg(SMP4Context *c, AVIOContext *pb, SMP4Atom atom)
{
    AVStream *st;
    SMP4StreamContext *sc;

    if (c->fc->nb_streams < 1)
        return 0;
    st = c->fc->streams[c->fc->nb_streams-1];
    sc = st->priv_data;

    avio_rb32(pb); // version + flags

    //sc->dts_shift = avio_rb32(pb);
    avio_rb32(pb);
    //dprintf(c->fc, "dts shift %d\n", sc->dts_shift);

    avio_rb32(pb); // least dts to pts delta
    avio_rb32(pb); // greatest dts to pts delta
    avio_rb32(pb); // pts start
    avio_rb32(pb); // pts end

    return 0;
}


//each parser function's atom->size is the length not include the header and size(-8)
//and the begin offset is as same as up.(-8)
static const SMP4ParseTableEntry dv_mp4_parse_table[] = {
{ MKTAG('c','s','l','g'), dv_mp4_read_cslg },
{ MKTAG('c','t','t','s'), dv_mp4_read_ctts }, /* composition time to sample */
{ MKTAG('d','i','n','f'), dv_mp4_read_default },
{ MKTAG('d','r','e','f'), dv_mp4_read_dref },
{ MKTAG('e','d','t','s'), dv_mp4_read_default },
{ MKTAG('e','l','s','t'), dv_mp4_read_elst },
{ MKTAG('f','t','y','p'), dv_mp4_read_ftyp },
{ MKTAG('h','d','l','r'), dv_mp4_read_hdlr },
{ MKTAG('m','d','a','t'), dv_mp4_read_mdat },
{ MKTAG('m','d','h','d'), dv_mp4_read_mdhd },
{ MKTAG('m','d','i','a'), dv_mp4_read_default },
{ MKTAG('m','i','n','f'), dv_mp4_read_default },
{ MKTAG('m','o','o','v'), dv_mp4_read_moov },
{ MKTAG('m','v','h','d'), dv_mp4_read_mvhd },
{ MKTAG('a','v','c','C'), dv_mp4_read_avvc },
{ MKTAG('s','t','b','l'), dv_mp4_read_default },
{ MKTAG('s','t','c','o'), dv_mp4_read_stco },
{ MKTAG('s','t','s','c'), dv_mp4_read_stsc },
{ MKTAG('s','t','s','d'), dv_mp4_read_stsd }, /* sample description */
{ MKTAG('s','t','s','s'), dv_mp4_read_stss }, /* sync sample */
{ MKTAG('s','t','s','z'), dv_mp4_read_stsz }, /* sample size */
{ MKTAG('s','t','t','s'), dv_mp4_read_stts },
{ MKTAG('t','k','h','d'), dv_mp4_read_tkhd }, /* track header */
{ MKTAG('t','r','a','k'), dv_mp4_read_trak },
{ MKTAG('e','s','d','s'), dv_mp4_read_esds },
{ MKTAG('v','m','h','d'), dv_mp4_read_esds },
{ 0, NULL }
};

static void dv_mp4_build_index(SMP4Context *mov, AVStream *st)
{
    SMP4StreamContext *sc = st->priv_data;
    int64_t current_offset;
    int64_t current_dts = 0;
    unsigned int current_sample = 0;
    unsigned int stts_index = 0;
    unsigned int stsc_index = 0;
    unsigned int stss_index = 0;
    unsigned int i, j;
    unsigned int distance = 0;
    unsigned int stts_sample = 0;


    /* adjust first dts according to edit list, video has 1001 time offset*/
    if (sc->time_offset) {
        //int rescaled = sc->time_offset;
        current_dts = -sc->time_offset;
    }
    //current_dts -= sc->dts_shift;
    st->nb_frames = sc->sample_count;

    /* only use old uncompressed audio chunk demuxing when stts specifies it */
   // sc->index_by_chunk = 1;
    av_log(NULL, AV_LOG_INFO, "__info:: chunk_nums::%d, stsc(video is 1) nums::%d, num in one chunk:%d\n",
                sc->chunk_count, sc->stsc_count, sc->stsc_data[0].count);
    if (sc->index_by_chunk)
    {
        unsigned int chunk_samples = 0;
        unsigned int chunk_duration = 0; //dts
        unsigned int chunk_size = 0;

        //for each chunk
        for (i = 0; i < sc->chunk_count; i++) {
            int keyframe = 0;
            current_offset = sc->chunk_offsets[i];
            /* if i(index for chunk) not equit the next stsc[index+1], it share the same num of samples in stsc[index] */
            if (stsc_index + 1 < sc->stsc_count &&
                i + 1 == sc->stsc_data[stsc_index + 1].first)
                stsc_index++;
            //current_sample += sc->stsc_data[stsc_index].count
            // not use the stss
            chunk_samples = sc->stsc_data[stsc_index].count;
            while(chunk_samples)
            {
                chunk_size += sc->sample_sizes[current_sample];
                //we no check the stts_index(always be 0)
                chunk_duration += sc->stts_data[stts_index].duration;
                current_sample ++;
                chunk_samples--;
            }
            if (current_sample > sc->sample_count)
            {
                 av_log(mov->fc, AV_LOG_ERROR, "wrong sample count::%d(is %d)\n", current_sample, sc->sample_count);
                 return;
            }

            if (!sc->keyframe_count || current_sample == sc->keyframes[stss_index])
            {
                keyframe = 1;
                if (stss_index + 1 < sc->keyframe_count)
                stss_index++;
            }
            //this may alway succ.
            if(sc->pseudo_stream_id == -1 ||
                   sc->stsc_data[stsc_index].id - 1 == sc->pseudo_stream_id)
            {
                av_add_index_entry(st, current_offset, current_dts, chunk_size, distance,
                                    keyframe ? AVINDEX_KEYFRAME : 0);
                //dprintf(mov->fc, "AVIndex stream %d, sample %d, offset %"PRIx64", dts %"PRId64", "
                            //"size %d, distance %d, keyframe %d\n", st->index, current_sample,
                            //current_offset, current_dts, chunk_size, distance, 1);
            }

            current_dts += chunk_duration;
            chunk_duration = 0;
            chunk_size = 0;
        }
    } else {
        for (i = 0; i < sc->chunk_count; i++)
        {
            current_offset = sc->chunk_offsets[i];
            if (stsc_index + 1 < sc->stsc_count &&
                i + 1 == sc->stsc_data[stsc_index + 1].first)
                stsc_index++;
            for (j = 0; j < sc->stsc_data[stsc_index].count; j++)/* for each sample in chunk */
            {
                int keyframe = 0;
                int key_off = sc->keyframes && sc->keyframes[0] == 1;
                unsigned int sample_size;
                if (current_sample >= sc->sample_count) {
                    av_log(mov->fc, AV_LOG_ERROR, "wrong sample count\n");
                    return;
                }

                if (!sc->keyframe_count || current_sample+key_off == sc->keyframes[stss_index])
                {
                    keyframe = 1;
                    if (stss_index + 1 < sc->keyframe_count)
                        stss_index++;
                }
                if (keyframe)
                    distance = 0;

                sample_size = sc->sample_size > 0 ? sc->sample_size : sc->sample_sizes[current_sample];
                if(sc->pseudo_stream_id == -1 ||
                   sc->stsc_data[stsc_index].id - 1 == sc->pseudo_stream_id) {
                    av_add_index_entry(st, current_offset, current_dts, sample_size, distance,
                                    keyframe ? AVINDEX_KEYFRAME : 0);
                    //dprintf(mov->fc, "AVIndex stream %d, sample %d, offset %"PRIx64", dts %"PRId64", "
                           // "size %d, distance %d, keyframe %d\n", st->index, current_sample,
                            //current_offset, current_dts, sample_size, distance, keyframe);
                }

                current_offset += sample_size;
                current_dts += sc->stts_data[stts_index].duration;
                distance++;
                stts_sample++;
                current_sample++;
                if (stts_index + 1 < sc->stts_count && stts_sample == sc->stts_data[stts_index].count){
                    stts_sample = 0;
                    stts_index++;
                }
            }
        }
    }
}

static int dv_mp4_probe(AVProbeData *p)
{
//temp
#if TARGET_USE_AMBARELLA_A5S_DSP
    uint32_t size, offset = 0;
    uint32_t tag, major_brand;
    int score = 0;

    size = AV_RB32(p->buf);
    tag = AV_RL32(p->buf+4);
    offset += 8;
    if(tag == MKTAG('f','t','y','p'))
    {
        //major_brand = AV_RL32(p->buf+offset);
        //if(major_brand == MKTAG('M','S','N','V'))
            score = AVPROBE_SCORE_MAX;
    }

    return score;
#endif
    return 0;
}

static int dv_mp4_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    SMP4Context* mov = s->priv_data;
    AVIOContext* pb = s->pb;
    int err;
    SMP4Atom atom = {0, 0, 0}, a = {0, 0, 0};
    unsigned int tmp;

    mov->fc = s;
    atom.size = avio_size(pb);
    /* read the ftyp first */
    a.size = avio_rb32(pb) - 8;
    a.offset += 8;
    a.type = avio_rl32(pb);
    tmp = a.size +8;
    if((err = dv_mp4_read_ftyp(mov, pb, a)) < 0)
        return err;
#if 0
    /* then we read the top uuid */
    a.size = avio_rb32(pb)-8;
    a.offset = tmp+8;
    a.type = avio_rl32(pb);
    tmp = tmp + a.size + 8;
    if((err = dv_mp4_read_uuid_top(mov, pb, a)) < 0)
        return err;
#endif

    /* now check the moov box */
    atom.size -= tmp;
    atom.offset += tmp;
    if ((err = dv_mp4_read_default(mov, pb, atom)) < 0) {
        av_log(s, AV_LOG_ERROR, "error reading header: %d\n", err);
        return err;
    }
    if (!mov->found_moov) {
        av_log(s, AV_LOG_ERROR, "moov atom not found\n");
        return -1;
    }
    //dprintf(mov->fc, "on_parse_exit_offset=%lld\n", avio_tell(pb));

    AVStream *st = s->streams[0];
    //av_log(NULL, AV_LOG_INFO, "@@%d,%d", st->sample_aspect_ratio.num, st->sample_aspect_ratio.den);

    //if(mov->video_type == VIDEO_1080_30P)
    //{
        mov->sample_select_array = dv_mp4_30p_playmode[0];
    //}else if(mov->video_type == VIDEO_720_60P){
        //mov->sample_select_array = dv_mp4_60p_playmode[0];
   // }
    mov->play_mode = NORMAL_PLAY;
    mov->mode_change = 0;
    mov->video_gop_size = 30;
    mov->start_n = 800;
    mov->end_m = 1400;
    return 0;
}

//also need the prev-prev chunk
static AVIndexEntry* dv_mp4_find_prev_index(AVFormatContext* s, AVStream** st, AVIndexEntry* prev_entry)
{
    AVIndexEntry* entry = NULL;
    int i, index, pre_index;
    AVStream *avst;
    SMP4Context* sc = s->priv_data;

    for (i = 0; i < s->nb_streams; i++)
    {
        //find first video stream;
        avst = s->streams[i];
        if(avst->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            break;
    }
    if(i == s->nb_streams)
        return NULL;
    /*Get the first video stream */
    *st = avst;
    SMP4StreamContext *ssc = avst->priv_data;

    if(ssc->pb && ssc->stss_index <=5)//(ssc->stss_index == 1 || ssc->stss_index == 0))
    {
        //TODO, should be 1 always.
        index = ssc->keyframes[0];
        assert(index == 1);
        entry = &avst->index_entries[index - 1];
        ssc->current_chunk = index - 1;
        prev_entry = NULL;
        return entry;
    }

    if (ssc->pb && ssc->stss_index < ssc->keyframe_count)
    {
        index = ssc->keyframes[ssc->stss_index - 5];
        /* if a frame n in GOP(m, form 0), then the stss_index is m+1, this is easy to handle the over gop when playback.
             cause maybe exit some GOP which be droped. */
        /* due to demuxer->dsp->vout delay. -1 is not enough. the empirical value of this delay is 5s.
             almost 5 Gop*/
        pre_index = ssc->keyframes[ssc->stss_index -6];
        entry = &avst->index_entries[index - 1];
        prev_entry = &avst->index_entries[pre_index - 1];
        ssc->current_chunk = index - 1;
    }
    return entry;
}


//for video index by chunk, for audio index by sample
static AVIndexEntry *dv_mp4_find_next_index(AVFormatContext *s, AVStream **st)
{
    AVIndexEntry *entry = NULL;
    int64_t best_dts = INT64_MAX;
    int i;
    for (i = 0; i < s->nb_streams; i++)
    {
        AVStream *avst = s->streams[i];
        SMP4StreamContext *ssc = avst->priv_data;

        //msc->current_chunk ++ in get_packet when succ combine a avpacket
        //av_log(NULL, AV_LOG_INFO, "avst->nb_index_entries: %d\n", avst->nb_index_entries);
        if (ssc->pb && ssc->current_chunk < avst->nb_index_entries)
        {
            AVIndexEntry *current_entry = &avst->index_entries[ssc->current_chunk];
            int64_t dts = av_rescale(current_entry->timestamp, AV_TIME_BASE, ssc->time_scale);
            //dprintf(s, "stream %d, sample %d, dts %"PRId64"\n", i, ssc->current_chunk, dts);

          //av_log(NULL, AV_LOG_INFO, "Debug: i:%d, current_entry->pos: %lld[index:%d in %d], entry->pos: %lld, dts:%lld\n", i,
           //current_entry->pos,ssc->current_chunk, avst->nb_index_entries,(entry==NULL)?0:entry->pos, dts);

            if (!entry ||(current_entry->pos < entry->pos) ||(dts < best_dts))
            {
                entry = current_entry;
                best_dts = dts;
                *st = avst;
            }
        }
    }
    return entry;
}


//i from 0 to 14, output one by one samples
static int dv_mp4_get_packet_cc(AVFormatContext* s, AVStream* st, AVPacket* pkt, int i, AVIndexEntry* entry)
{
    unsigned int index, cur_size;
    unsigned int prev_size = 0, prev_index;
    int j;
    uint8_t* buf, tmp;
    SMP4StreamContext* ssc = st->priv_data;
    SMP4Context* sc = s->priv_data;

    if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        index = ssc->current_chunk * sc->video_gop_size + i; //sc->video_gop_size
        cur_size = ssc->sample_sizes[index];
        for(j = i-1; j >= 0; j--)
        {
            prev_index =  ssc->current_chunk * sc->video_gop_size + j;
            prev_size += ssc->sample_sizes[prev_index];
        }
        avio_skip(ssc->pb, prev_size);
        pkt->pos = avio_tell(ssc->pb);
        int ret =av_get_packet(ssc->pb, pkt, cur_size);
        //av_log(NULL, AV_LOG_INFO, "ret:%d ,  cur_size:%d\n", ret , cur_size);
        if(ret < 0)
            return ret;
        if((i%14 == 0) && (i != 0))
            ssc->current_chunk ++;
    }else{
        int ret = av_get_packet(ssc->pb, pkt, entry->size);
        if (ret < 0)
            return ret;
        ssc->current_chunk++;
    }

    return 0;
}


//For now, a chunk is just a video frame. ssc->pb is in the current chunk offset;
//TODO:: For drop by pause/block.
/*BugNote: 8/18
    1.GOP maybe lager than 30 on the chance of 30-GOP.....
        Arose gop++ in the end of playmode table. which let the left goto confusion!
        Solution::Muxer may makesure that GOP maybe less but should never larger.......- -|||
        Alternative: Using larger playmode table.=>will backcross on 30. GOP decided only by stss.
*/
static int dv_mp4_get_packet(AVFormatContext* s, AVStream* st,
                                        AVPacket* pkt, AVIndexEntry* entry, AVIndexEntry* prev_entry)
{
    unsigned int index, pkt_size = 0, cur_size;
    int speed, i, ret;
    uint8_t* buf;
    SMP4Context* sc = s->priv_data;
    SMP4StreamContext* ssc = st->priv_data;

    if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        if(sc->play_mode < BACKWARD_PLAY || sc->play_mode == PLAY_N_TO_M)
        {
            index = ssc->current_chunk; //a chunk just a video sample
            pkt_size = ssc->sample_sizes[index];
            ret = av_get_packet(ssc->pb, pkt, pkt_size);

            //av_log(NULL, AV_LOG_INFO, "ssc->current_chunk :%d, sample_in_gop: %d\n", ssc->current_chunk, ssc->sample_in_gop);
            for(i = ssc->sample_in_gop + 1; i < 40; ++i)// 30 to sc->video_gop_size, 40 for bugnote 1
            {
                ssc->current_chunk ++;
                //av_log(NULL, AV_LOG_INFO, "sample_select_array[i]: %d, value:%d\n", i, sc->sample_select_array[i]);
                //for drop frame, a GOP not full, stss begin with 1.
                if((ssc->keyframes[ssc->stss_index] - 1) == ssc->current_chunk)/* && ssc->stss_index != ssc->keyframe_count -1*/
                {
                    if(ssc->stss_index + 1 < ssc->keyframe_count)
                        ssc->stss_index++;
                    i = 0;
                    break;
                }
                if(sc->sample_select_array[i] == 1)
                {
                    //av_log(NULL, AV_LOG_INFO, "===>:i%d   chunk:%d, index:%d\n", i,  ssc->current_chunk,  index);
                    break;
                }
            }
            if(i == 40)
            {
                /*will never happen cause bugnote 1*/
                ssc->sample_in_gop = 0;
                ssc->current_chunk++;
                //ssc->current_gop++;//next GOP
                assert((ssc->keyframes[ssc->stss_index] - 1) == ssc->current_chunk);
                if(ssc->stss_index + 1 < ssc->keyframe_count)
                    ssc->stss_index++;
            }else{
                ssc->sample_in_gop = i;
            }
        }else{
            /*===========backward play get packet=============*/
            index = ssc->current_chunk;
            pkt_size = ssc->sample_sizes[index];
            ret = av_get_packet(ssc->pb, pkt, pkt_size);

            if (ssc->stss_index > 0)
            {
                ssc->stss_index--;
            }
            //ssc->current_chunk = ssc->current_gop * sc->video_gop_size;
            //ssc->current_gop--;
        }
        //now process the error in av_get_packet
        if(ret < 0){
            av_log(NULL, AV_LOG_INFO, "dv_mp4_get_video_packet return -2!!!\n");
            return -2; //re-read;
        }
    }else if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO){
        ssc->current_chunk++;
        if(sc->play_mode > NORMAL_PLAY && sc->play_mode != PLAY_N_TO_M)
        {
            //av_log(NULL, AV_LOG_INFO, "Skip audio packet in not normal play mode.\n");
            return -2; // audio only accur in normal play, re-read
        }
        int ret = av_get_packet(ssc->pb, pkt, entry->size);
        if (ret < 0)
            return -2;
    }else{
        //should be skip and read index again.todo
        return -2;
    }
    return 0;
}


#if 0
//There has diff way according to the chunk structure, if a chunk = a gop, that using this .
static int dv_mp4_get_packet_backup(AVFormatContext* s, AVStream* st,
                                        AVPacket* pkt, AVIndexEntry* entry, AVIndexEntry* follow_entry)
{
    unsigned int index, total_size = 0, cur_size;
    int speed, i;
    uint8_t* buf;
    SMP4Context* sc = s->priv_data;
    SMP4StreamContext* ssc = st->priv_data;

    if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        int (* dv_sample_select)[15]; //def later
        int* speed_select_array;
        //switch(sc->playrate)
        //if(sc->video_type)
        dv_sample_select = dv_mp4__ff;
        //speed = s->play_speed;
        speed = 0;
        speed_select_array = *(dv_sample_select + speed); //+ speed;

        for(i = 0; i < sc->video_gop_size; ++i)// N sc->video_gop_size
        {
            index = ssc->current_chunk * sc->video_gop_size +i;

            if(speed_select_array[i] != 0)
            {
                //av_log(NULL, AV_LOG_INFO, "===>:i%d   chunk:%d, index:%d\n", i,  ssc->current_chunk,  index);
                total_size = ssc->sample_sizes[index] + total_size;
            }
        }
        av_new_packet(pkt, total_size);
        buf = pkt->data;
        for(i = 0; i < sc->video_gop_size; ++i)//N sc->video_gop_size
        {
            index = ssc->current_chunk * sc->video_gop_size +i;
            cur_size = ssc->sample_sizes[index];
            if(speed_select_array[i] != 0)
            {
                if((buf + cur_size) > (pkt->data + pkt->size))
                {
                    av_log(NULL, AV_LOG_ERROR, "c");
                    return -1;
                }
                avio_read(ssc->pb, buf, cur_size);
                buf+= cur_size;
                if((buf - pkt->data) > pkt->size)
                    av_log(NULL, AV_LOG_ERROR, "c");
            }else{
                avio_skip(ssc->pb, cur_size);
            }
        }
        ssc->current_chunk ++;
        //ssc->current_chunk --; fb
    }else if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO){
        int ret = av_get_packet(ssc->pb, pkt, entry->size);
        if (ret < 0)
            return ret;
        ssc->current_chunk++;
    }else{
        //should be skip and read index again.
        return -2;
    }
    return 0;
}
#endif

static int dv_mp4_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    SMP4Context* sc = s->priv_data;
    SMP4StreamContext* ssc;
    AVIndexEntry* entry, *follow_entry = NULL;
    AVStream *st = NULL;
    int ret;

    entry = dv_mp4_find_next_index(s, &st);
    if(!entry)
    {
        av_log(NULL, AV_LOG_INFO, "_ _|||\n");
        return AVERROR_EOF;
    }
    //if(entry != NULL)
        //av_log(NULL, AV_LOG_INFO, "got a entry: 0x%"PRIx64"\n", entry->pos);

    ssc = st->priv_data;
    if (st->discard != AVDISCARD_ALL)
    {
        if (avio_seek(ssc->pb, entry->pos, SEEK_SET) != entry->pos)
        {
            av_log(sc->fc, AV_LOG_ERROR, "stream %d, offset 0x%"PRIx64": partial file\n",
                   ssc->ffindex, entry->pos);
            return -1;
        }
        //recombine the packet from a whole chunk: GOP, s->playrate
        ret = dv_mp4_get_packet(s, st, pkt, entry, follow_entry);
        //this output sample one by one
        //ret = dv_mp4_get_packet_cc(s, st, pkt, ssc->current_sample - (ssc->current_chunk * sc->video_gop_size), entry);
        if (ret < 0)
            return ret;
    }


    //*we get the packet, set this set of samples dts to the first sample's dts(stts)
    pkt->stream_index = ssc->ffindex;
    if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        pkt->dts = entry->timestamp;
    else
        pkt->dts = entry->timestamp;//;, first sample's dts in a GOP
        //pkt->dts = entry->timestamp + (ssc->current_sample - (ssc->current_chunk * sc->video_gop_size)) * ssc->stts_data[0].duration;
        //for one by one sample

    if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        ssc->current_sample ++;
    else
        ssc->current_sample += sc->video_gop_size;//sc->video_gop_size;
        //ssc->current_sample ++; //for one by one sample

    if (ssc->ctts_data)
    {
        pkt->pts = pkt->dts + ssc->ctts_data[ssc->ctts_index].duration;
        /* update ctts context */
        ssc->ctts_sample++;
        //todo  ctts
        if (ssc->ctts_index < ssc->ctts_count &&
            (ssc->ctts_data[ssc->ctts_index].count) == ssc->ctts_sample) {
            ssc->ctts_index++;
            ssc->ctts_sample = 0;
        }
        //if (ssc->wrong_dts)
            //pkt->dts = AV_NOPTS_VALUE;
    }else{
        int64_t next_dts = (ssc->current_sample < st->nb_index_entries) ?
            st->index_entries[ssc->current_sample].timestamp : st->duration;
        pkt->duration = next_dts - pkt->dts;
        pkt->pts = pkt->dts;
    }
    //if(st->codec->codec_type == AVMEDIA_TYPE_AUDIO ||(ssc->current_sample - (ssc->current_chunk * sc->video_gop_size)) == 0)
        pkt->flags |= AV_PKT_FLAG_KEY;

    pkt->pos = entry->pos;
    //dprintf(s, "stream %d, pts %"PRId64", dts %"PRId64", pos 0x%"PRIx64", duration %d\n",
            //pkt->stream_index, pkt->pts, pkt->dts, pkt->pos, pkt->duration);
    return 0;
}

static int dv_mp4_seek_audio_stream(AVStream *st, int64_t timestamp, int flags)
{
    SMP4StreamContext* ssc = st->priv_data;
    int entry_index, chunk_num;
    int i;

    //audio using less timestamp sample.
    flags |= AVSEEK_FLAG_BACKWARD;
    flags |= AVSEEK_FLAG_ANY;

    if(timestamp == 0)
    {
        ssc->current_chunk = 0;
        return 0;
    }
    //
    //return a IDR index
    entry_index = av_index_search_timestamp(st, timestamp, flags);
    //dprintf(st->codec, "stream %d, timestamp %"PRId64", sample %d\n", st->index, timestamp, sample);
    if (entry_index < 0) /* not sure what to do */
    {
        return -1;
    }
    //get a idr entry
    ssc->current_chunk = entry_index;
    assert(ssc->ctts_count == 0);
    //dprintf(st->codec, "stream %d, found sample %d\n", st->index, sc->current_sample);
    av_log(NULL, AV_LOG_INFO, "Dv_mp4 Seek Audio! => current_chunk: %d, stss_index: %d, get timestampe: %lld\n",
                 ssc->current_chunk, ssc->stss_index, st->index_entries[entry_index].timestamp);

    return entry_index;
}

static int dv_mp4_seek_stream(AVStream *st, int64_t timestamp, int flags)
{
    SMP4StreamContext* ssc = st->priv_data;
    int entry_index, chunk_num;
    int i;

    //must set flags correct here
    if(timestamp < ssc->current_pts)
        flags |= AVSEEK_FLAG_BACKWARD;
    //flags &= AVSEEK_FLAG_ANY; //just seek to IDR Frame;

    if(timestamp == 0)
    {
        ssc->current_chunk = 0;
        ssc->sample_in_gop = 0;
        ssc->stss_index = 1;
        ssc->ctts_index = 0;
        ssc->ctts_sample = 0;
        return 0;
    }
    //return a IDR index
    entry_index = av_index_search_timestamp(st, timestamp, flags);
    //dprintf(st->codec, "stream %d, timestamp %"PRId64", sample %d\n", st->index, timestamp, sample);
    if (entry_index < 0) /* not sure what to do */
    {
        return -1;
    }
    //get a idr entry
    ssc->current_chunk = entry_index;
    ssc->sample_in_gop = 0;
    /*adjust stss index */
    for(i = 0; i < ssc->keyframe_count; i++)
    {
        if((ssc->keyframes[i] - 1) == entry_index)
            break;
    }
    assert(i < ssc->keyframe_count);
    ssc->stss_index = i+1;
    /* adjust ctts index */
    if (ssc->ctts_data)
    {
        chunk_num = 0;
        for (i = 0; i < ssc->ctts_count; i++)
        {
            int next = chunk_num + ssc->ctts_data[i].count;
            if (next > ssc->current_chunk) {
                ssc->ctts_index = i;
                ssc->ctts_sample = ssc->current_chunk - chunk_num;
                break;
            }
            chunk_num = next;
        }
    }
    av_log(NULL, AV_LOG_INFO, "Dv_mp4 Seek Video! => current_chunk: %d, stss_index: %d, get timestampe: %lld\n",
                 ssc->current_chunk, ssc->stss_index, st->index_entries[entry_index].timestamp);
    //dprintf(st->codec, "stream %d, found sample %d\n", st->index, sc->current_sample);

    return entry_index;
}

static int dv_mp4_play_nm(AVFormatContext* s)
{
    SMP4Context* sc = s->priv_data;
    AVStream* st;
    SMP4StreamContext* ssc;
    AVIndexEntry* entry;
    int i, last_idr, stss, chunk_num;
    int stream_index;
    int64_t timestamp, timestamp2;
    int set_n = 0, set_m = 0;
    //find video stream
    for (i = 0; i < s->nb_streams; i++)
    {
        st = s->streams[i];
        if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            stream_index = i;
            break;
        }
    }
    assert(i < s->nb_streams);
    ssc = st->priv_data;

    //verify n, m
    last_idr = ssc->keyframes[ssc->keyframe_count -1];
    if(sc->start_n < 1)
        sc->start_n = 1; //begin with 1
    if(sc->start_n >= last_idr)
    {
        sc->start_n = last_idr;
        stss = ssc->keyframe_count -1;
        set_n = 1;
    }

    if(sc->end_m <= sc->start_n)
        sc->end_m = sc->start_n + 1;
    if(sc->end_m > last_idr)
    {
        if(sc->start_n == last_idr)
            sc->end_m = st->nb_index_entries + 1; // play to end of file
        else
            sc->end_m = last_idr;
        set_m = 1;
    }
    //find idr include n, m
    for(i = 0; i < ssc->keyframe_count; i++)
    {
        if(set_n && set_m)
            break;

        if((ssc->keyframes[i] >= sc->start_n) && (set_n == 0))
        {
            sc->start_n = (ssc->keyframes[i] == sc->start_n) ? ssc->keyframes[i] : ssc->keyframes[i -1];
            stss = (ssc->keyframes[i] == sc->start_n) ? (i+1) : i;
            set_n = 1;
        }
        if((ssc->keyframes[i] >= sc->end_m) && (set_m == 0))
        {
            sc->end_m = ssc->keyframes[i];
            set_m = 1;
        }
    }

    //set current_chunk
    ssc->current_chunk = sc->start_n - 1; //current_chunk begin with 0
    ssc->sample_in_gop = 0;
    ssc->stss_index = stss;
    if (ssc->ctts_data)
    {
        chunk_num = 0;
        for (i = 0; i < ssc->ctts_count; i++)
        {
            int next = chunk_num + ssc->ctts_data[i].count;
            if (next > ssc->current_chunk) {
                ssc->ctts_index = i;
                ssc->ctts_sample = ssc->current_chunk - chunk_num;
                break;
            }
            chunk_num = next;
        }
    }
    av_log(NULL, AV_LOG_INFO, "Play From:%d To %d! STSS index:%d, CTTS index:%d\n", sc->start_n,
            sc->end_m, ssc->stss_index, ssc->ctts_index);

    /* adjust timestamp to found other stream's seek timestamp */
    timestamp = st->index_entries[ssc->current_chunk].timestamp;
    if(ssc->current_chunk == 0)
        timestamp = 0;
    /*seek other(audio) stream to av sync*/
    for (i = 0; i < s->nb_streams; i++)
    {
        st = s->streams[i];
        if (stream_index == i)
            continue;

        timestamp2 = av_rescale_q(timestamp, s->streams[stream_index]->time_base, st->time_base);
        av_log(NULL, AV_LOG_INFO, "timestamp:%lld ,  timestamp2 :%lld!\n", timestamp, timestamp2);
        dv_mp4_seek_audio_stream(st, timestamp2, 0);
    }

    return 0;
}

static int dv_mp4_change_mode(AVFormatContext* s)
{
    SMP4Context* sc = s->priv_data;
    AVStream* st;
    SMP4StreamContext* ssc;
    AVIndexEntry* entry;
    int64_t timestamp, timestamp2;
    int flags, i, chunk_num, stream_index;
    if(sc->play_mode == PLAY_N_TO_M)
    {
        dv_mp4_play_nm(s);
        sc->sample_select_array = dv_mp4_30p_playmode[0];
        //sc->play_mode = NORMAL_PLAY;
        sc->old_mode = sc->play_mode;
        sc->mode_change = 0;
        return 0;
    }
    switch(sc->video_type)
    {
    case VIDEO_720_30P:
    case VIDEO_1080_30P:
        //sc->sample_select_array = dv_mp4_30p_playmode[sc->play_mode];
        //break;

    case VIDEO_720_60P:
        //sc->sample_select_array = dv_mp4_60p_playmode[sc->play_mode];
        //break;

    default:
        if(sc->play_mode < BACKWARD_PLAY)
            sc->sample_select_array = dv_mp4_30p_playmode[sc->play_mode];
        break;
    }
    if(sc->old_mode > BACKWARD_PLAY && sc->old_mode != PLAY_N_TO_M && sc->play_mode >= NORMAL_PLAY && sc->play_mode < BACKWARD_PLAY)
    {
        for (i = 0; i < s->nb_streams; i++)
        {
            st = s->streams[i];
            if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                ssc = st->priv_data;
                stream_index = i;
                AVIndexEntry* entry = &st->index_entries[ssc->current_chunk];
                break;
            }
         }

        ssc->sample_in_gop = 0;
        if(ssc->stss_index == 0)
            ssc->stss_index = 1;
        if (ssc->ctts_data)
        {
            chunk_num = 0;
            for (i = 0; i < ssc->ctts_count; i++)
            {
                int next = chunk_num + ssc->ctts_data[i].count;
                if (next > ssc->current_chunk) {
                    ssc->ctts_index = i;
                    ssc->ctts_sample = ssc->current_chunk - chunk_num;
                    break;
                }
                chunk_num = next;
            }
        }

        /* adjust timestamp to found other stream's seek timestamp */
        timestamp = entry->timestamp;
        if(ssc->current_chunk == 0)
            timestamp = 0;
        /*seek other(audio) stream to av sync*/
        for (i = 0; i < s->nb_streams; i++)
        {
            st = s->streams[i];
            if (stream_index == i)
                continue;

            timestamp2 = av_rescale_q(timestamp, s->streams[stream_index]->time_base, st->time_base);
            dv_mp4_seek_audio_stream(st, timestamp, 0);
        }
    //done process form backplay to normal playback
    }

    sc->old_mode = sc->play_mode;
    sc->mode_change = 0;
    return 0;
}

//using this to do ff/fb
static int dv_mp4_read_packet_cc(AVFormatContext *s, AVPacket *pkt)
{
    SMP4Context* sc = s->priv_data;
    SMP4StreamContext* ssc;
    AVIndexEntry* entry, *prev_entry;
    AVStream *st = NULL;
    int ret, temp;
    if(sc->mode_change)
    {
        av_log(NULL, AV_LOG_INFO, "\nChange Mode, Old Mode:%d==>New Mode:%d!\n", sc->old_mode, sc->play_mode);
        dv_mp4_change_mode(s);
    }
 retry:
    //if(s->f)else if(s->b)dv_mp4_find_previous_sample, video st select
    if(sc->play_mode > BACKWARD_PLAY && sc->play_mode != PLAY_N_TO_M)
    {
        entry = dv_mp4_find_prev_index(s, &st, prev_entry);
        //prev_entry =
    }else{
        entry = dv_mp4_find_next_index(s, &st);
    }
    if(!entry)
        return AVERROR_EOF;
    if(entry != NULL)
    {
        ssc = st->priv_data;
        if(sc->play_mode == PLAY_N_TO_M)
        {
            if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                if((ssc->current_chunk + 1) >= sc->end_m)
                    return AVERROR_EOF;
            }
        }
        int64_t dts = av_rescale(entry->timestamp, AV_TIME_BASE, ssc->time_scale);
        //av_log(NULL, AV_LOG_INFO, "Get a entry, pos: 0x%"PRIx64", size: %d, timestamp:%lld==Dts: %lld,  stream_id:==>%d, current_chunk:%d, stss_index:%d, sample_in_gop:%d\n"
           //, entry->pos, entry->size, entry->timestamp,dts,st->index, ssc->current_chunk, ssc->stss_index, ssc->sample_in_gop);
    }
   /* if(!entry)
    {
        sc->found_mdat = 0;
        if (dv_mp4_read_default(sc, s->pb, (SMP4Atom){ 0, 0, INT64_MAX }) < 0 ||
            url_feof(s->pb))
            return AVERROR_EOF;
        //dprintf(s, "read fragments, offset 0x%llx\n", avio_tell(s->pb));
        goto retry;
    }*/
    ssc = st->priv_data;
    //sc->playrate = s->playrate;
    /* must be done just before reading, to avoid infinite loop on sample */
    //todo
    //ssc->current_sample += 32;
    //
    //temp = ssc->sample_in_gop;

    if (st->discard != AVDISCARD_ALL)
    {
        if (avio_seek(ssc->pb, entry->pos, SEEK_SET) != entry->pos) {
            av_log(sc->fc, AV_LOG_ERROR, "stream %d, offset 0x%"PRIx64": partial file\n",
                   ssc->ffindex, entry->pos);
            return -1;
        }
        //recombine the packet from a whole chunk: GOP, s->playrate
        ret = dv_mp4_get_packet(s, st, pkt, entry, prev_entry);
        //if(pkt)
            //av_log(NULL, AV_LOG_INFO, "pkt:%p, size:%d\n", pkt->data, pkt->size);
        if (ret < 0)
        {
            if(ret == -2)
            {
                //av_log(NULL, AV_LOG_INFO, "dv_mp4_get_packet return -2, retry!!!\n");
                goto retry;
            }
            return ret;
        }
    }

    //*we get the packet, set this set of samples dts to the first sample's dts
    pkt->stream_index = ssc->ffindex;
    pkt->dts = entry->timestamp;
    if (ssc->ctts_data) {
        int duration = ssc->ctts_data[ssc->ctts_index].duration;
        //if(duration < 0 || duration > 30*ssc->stts_data[0].duration)
            //duration = ssc->stts_data[0].duration;
        pkt->pts = pkt->dts + ssc->dts_shift + duration;
        if(pkt->pts < 0)
            pkt->pts = 0;
        /* how to do to wait eos form dsp
        if(sc->play_mode == PLAY_N_TO_M)
        {
            pkt->dts -= ((sc->start_n-1) * ssc->stts_data[0].duration);
            pkt->pts -= ((sc->start_n-1) * ssc->stts_data[0].duration);
        }*/
        //av_log(NULL, AV_LOG_INFO, "pts: %lld, dts:%lld, duration gap: %d, chunk index:%d!!!!\n", pkt->pts, pkt->dts,
        //ssc->ctts_data[ssc->ctts_index].duration, ssc->current_chunk-1);

        /* update ctts context */
        ssc->ctts_sample++;
        //todo  ctts
        if (ssc->ctts_index < ssc->ctts_count &&
            (ssc->ctts_data[ssc->ctts_index].count) == ssc->ctts_sample) {
            ssc->ctts_index++;
            ssc->ctts_sample = 0;
        }
        //if (ssc->wrong_dts)
            //pkt->dts = AV_NOPTS_VALUE;
    } else {
        int64_t next_dts = (ssc->current_sample < st->nb_index_entries) ?
            st->index_entries[ssc->current_sample].timestamp : st->duration;
        pkt->duration = next_dts - pkt->dts;
        //int duration = ssc->stts_data[0].duration;
        //pkt->pts = pkt->dts+duration;
        pkt->pts = pkt->dts;
    }
    //if(pkt->pts < pkt->dts)
        //if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        //av_log(NULL, AV_LOG_INFO, "pts: %lld, dts:%lld,!current_chunk: %d!!!!\n",pkt->pts, pkt->dts, ssc->current_chunk -1);
    if (st->discard == AVDISCARD_ALL)
        goto retry;
    pkt->flags |= entry->flags & AVINDEX_KEYFRAME ? AV_PKT_FLAG_KEY : 0;
/*    if(ssc->current_chunk == 0 || ssc->current_chunk == 28)
    {
        //av_log(NULL, AV_LOG_INFO, "IDR frame!\n");
        pkt->flags |= AV_PKT_FLAG_KEY;
    }else if((ssc->current_chunk - 28 > 0)&&((ssc->current_chunk-28) %30 == 0))
            pkt->flags |= AV_PKT_FLAG_KEY;
*/
   // if(pkt->flags & AV_PKT_FLAG_KEY && st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        //av_log(NULL, AV_LOG_INFO, "IDR frame!current_chunk: %d\n", ssc->current_chunk -1);
    pkt->pos = entry->pos;
    //dprintf(s, "stream %d, pts %"PRId64", dts %"PRId64", pos 0x%"PRIx64", duration %d\n",
            //pkt->stream_index, pkt->pts, pkt->dts, pkt->pos, pkt->duration);
    return 0;
}

static int dv_mp4_read_seek(AVFormatContext *s, int stream_index, int64_t seek_time, int flags)
{
    av_log(NULL, AV_LOG_INFO, "Dv_mp4_read_seek! stream: %d, seek_time:%lld\n", stream_index, seek_time);
    AVStream *st;
    int64_t seek_timestamp, timestamp;
    int entry_index;
    int i;

    if (stream_index >= s->nb_streams)
        return -1;
    /*seek by video
    if(s->streams[stream_index]->codec->codec_type != AVMEDIA_TYPE_VIDEO)
    {
        for(int index = 0; index < s->nb_streams; index++)
            if(s->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
                stream_index = index;
    }*/
    if (seek_time < 0)
        seek_time = 0;
    if(flags & AVSEEK_FLAG_BYTE)
    {
        av_log(NULL, AV_LOG_WARNING, "Amba DV Mp4: Should never seek by byte! Try to seek by timestamp.\n");
    }
    flags = 0;
    st = s->streams[stream_index];
    entry_index = dv_mp4_seek_stream(st, seek_time, flags);
    if (entry_index < 0)
        return -1;

    /* adjust seek timestamp to found other stream's seek timestamp */
    seek_timestamp = st->index_entries[entry_index].timestamp;
    if(entry_index == 0)
        seek_timestamp = 0;

    /*seek other(audio) stream to av sync*/
    for (i = 0; i < s->nb_streams; i++)
    {
        st = s->streams[i];
        if (stream_index == i)
            continue;

        timestamp = av_rescale_q(seek_timestamp, s->streams[stream_index]->time_base, st->time_base);
        dv_mp4_seek_audio_stream(st, timestamp, flags);
    }
    av_log(NULL, AV_LOG_INFO, "Dv_mp4_read_seek return succ!\n");
    return 0;
}

static int dv_mp4_read_close(AVFormatContext *s)
{
    SMP4Context *mov = s->priv_data;
    int i, j;

    for (i = 0; i < s->nb_streams; i++)
    {
        AVStream *st = s->streams[i];
        SMP4StreamContext *sc = st->priv_data;

        av_freep(&sc->ctts_data);
        for (j = 0; j < sc->drefs_count; j++)
            av_freep(&sc->drefs[j].path);
        av_freep(&sc->drefs);

        av_freep(&sc->sample_sizes);
        av_freep(&sc->stts_data);
        av_freep(&sc->keyframes);
        if (sc->pb && sc->pb != s->pb)
            avio_close(sc->pb);

        av_freep(&st->codec->palctrl);
    }

    return 0;
}


AVInputFormat ff_amba_dv_mp4_demuxer = {
    "a5dv mp4",
    NULL_IF_CONFIG_SMALL("DV MP4 Format for A5S"),
    sizeof(SMP4Context),
    dv_mp4_probe,
    dv_mp4_read_header,
    dv_mp4_read_packet_cc,
    dv_mp4_read_close,
    dv_mp4_read_seek,
};

