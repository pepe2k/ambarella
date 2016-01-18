/*
 * MP4 demuxer for sony
 * Copyright (c) 2010 <Ambarella>
 * mycicai@hotmail.com
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

#ifndef __AVFORMAT_MP4_DV_H
#define __AVFORMAT_MP4_DV_H

//for esds box
#define SMP4ESDescrTag                   0x03
#define SMP4DecConfigDescrTag            0x04
#define SMP4DecSpecificDescrTag          0x05

typedef struct {
    int count; /* num of sample*/
    int duration; /*1001 for video, 1024 for audio*/
} SMP4Stts;

typedef struct {
    int first;
    int count;
    int id;
} SMP4Stsc;

typedef struct {
    uint32_t type;
    char *path;
} SMP4Dref;

typedef struct {
    uint32_t type;
    int64_t offset;
    int64_t size; /* total size (excluding the size and type fields) */
} SMP4Atom;

#if 0
#endif

enum VideoType
{
    VIDEO_1080_30P,
    VIDEO_720_30P,
    VIDEO_720_60P,
    VIDEO_270_30P,
    VIDEO_NONE,
};

enum PlayMode
{
    NORMAL_PLAY,
    FF_2X_SPEED,
    FF_4X_SPEED,
    FF_8X_SPEED,
    FF_16X_SPEED,
    FF_30X_SPEED,
    BACKWARD_PLAY,
    FB_1X_SPEED,
    FB_2X_SPEED,
    FB_4X_SPEED,
    FB_8X_SPEED,
    PLAY_N_TO_M, //paly form n frame to m frame. n < m
};

typedef struct SMP4StreamContext {
    AVIOContext *pb;
    int ffindex;          ///< AVStream index
    int next_chunk;
    unsigned int chunk_count; /*chunk num*/
    int64_t *chunk_offsets; /*store the stco */
    unsigned int stts_count; /*dts, should be 1, video:1001  audio:1024*/
    SMP4Stts *stts_data;//need colse free
    unsigned int ctts_count; /*pts, just for video, diff videotype has diff value */
    SMP4Stts *ctts_data;//need colse free
    int dts_shift;        ///< dts shift when ctts is negative, make sure pts > dts
    unsigned int stsc_count; /* 1 for video , and num of chunk for audio */
    SMP4Stsc *stsc_data;
    int ctts_index;
    int ctts_sample;
    unsigned int sample_size;
    unsigned int sample_count; /* calu by the stsz's entry num */
    int *sample_sizes; /*store the stsz */
    unsigned int keyframe_count; /*calu by the stss box */
    int *keyframes; /*store the stss, only exist in video*/
    unsigned int stss_index; //index of GOP ,useful in fb

    int time_scale;
    int time_offset;      ///< time offset of the first edit list entry, calu by elst box. video: 1001, audio: 0
    int current_sample; /*current readed sample num*/
    //unsigned int bytes_per_frame; /* we using the st->codec->bit...... consider this */
    //unsigned int samples_per_frame;
    int pseudo_stream_id; ///< -1 means demux all ids
    //int16_t audio_cid;    ///< stsd audio compression id
    unsigned drefs_count; /*should be 1*/
    SMP4Dref *drefs; /*no used */
    int dref_id; /*no used *, always 1*/
    //int wrong_dts;        ///< dts are wrong due to huge ctts offset (iMovie files)
    int width;            ///< tkhd width
    int height;           ///< tkhd height
    //int dts_shift;        ///< dts shift when ctts is negative---->not use
    int current_chunk;  /*for video, a chunk is a GOP(in a5s just a video frame), for audio, this is for a sample( also a seek entry);*/
    //int current_gop; use stss_index
    unsigned int sample_in_gop; /*for a GOP's index, we get the frame we want. */
    int index_by_chunk; //make index for chunk

    int64_t current_pts; //seek support.
} SMP4StreamContext;

typedef struct SMP4Context {
    AVFormatContext *fc;
    int time_scale;
    int64_t duration;     ///< duration of the longest track
    int found_moov;       ///< 'moov' atom has been found  and parsed succ.
    int found_mdat;       ///< 'mdat' atom has been found
    unsigned trex_count;
    int itunes_metadata;  ///< metadata are itunes style
    enum VideoType video_type;
    int* sample_select_array; //test 30p or 60p
    unsigned int video_gop_size; // sample num of a chunk

    unsigned int start_n;
    unsigned int end_m;
    enum PlayMode play_mode;
    enum PlayMode old_mode;
    unsigned int mode_change; //0 or 1, 1 indicate app change play mode
} SMP4Context;


#endif /* AVFORMAT_ISOM_H */
