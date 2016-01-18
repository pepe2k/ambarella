/*
 * raw ADTS AAC demuxer
 * Copyright (c) 2008 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2009 Robert Swain ( rob opendot cl )
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

#include "libavutil/intreadwrite.h"
#include "avformat.h"
#include "rawdec.h"
#include "id3v1.h"
#include "libavcodec/mpeg4audio.h"

static int adts_aac_probe(AVProbeData *p)
{
    int max_frames = 0, first_frames = 0;
    int fsize, frames;
    uint8_t *buf0 = p->buf;
    uint8_t *buf2;
    uint8_t *buf;
    uint8_t *end = buf0 + p->buf_size - 7;

    buf = buf0;

    for(; buf < end; buf= buf2+1) {
        buf2 = buf;

        for(frames = 0; buf2 < end; frames++) {
            uint32_t header = AV_RB16(buf2);
            if((header&0xFFF6) != 0xFFF0)
                break;
            fsize = (AV_RB32(buf2+3)>>13) & 0x8FFF;
            if(fsize < 7)
                break;
            buf2 += fsize;
        }
        max_frames = FFMAX(max_frames, frames);
        if(buf == buf0)
            first_frames= frames;
    }
    if   (first_frames>=3) return AVPROBE_SCORE_MAX/2+1;
    else if(max_frames>500)return AVPROBE_SCORE_MAX/2;
    else if(max_frames>=3) return AVPROBE_SCORE_MAX/4;
    else if(max_frames>=1) return 1;
    else                   return 0;
}

static int adts_find_next_sync_code(AVIOContext *pb)
{
	int32_t temp, state = 0x00;

	while (1) {
		temp = avio_r8(pb);
		if (temp == 0 && url_feof(pb)) {
			//av_log(NULL, AV_LOG_ERROR, "temp=%d eof=%d\n", temp, url_feof(pb));
			break;
		}

		state = ((state<<8)|temp)&0xFFFF;
        //av_log(NULL, AV_LOG_ERROR, "state=%4X\n", state);
		if ((state&0xFFF0) == 0xFFF0) {//syncword: The bit string "1111 1111 1111". See ISO/IEC 13818-7, fix bug#2449
			avio_seek(pb, -2, SEEK_CUR);
			return 1;
		}
	}
	return 0;
}

static void adts_get_frame_count(AVIOContext *pb, int32_t *frame_cnt, int32_t *sample_rate)
{
	uint8_t header[7];
	int32_t sampling_frequency_index, channel_configuration, frame_length, number_of_raw_data_blocks_in_frame;

	if (avio_read(pb, header, 7) < 0)
	{
		return;
	}

	sampling_frequency_index = (header[2] & 0x3C)>>2;
	channel_configuration = ((header[2]&0x01) << 2) | ((header[3]&0xC0)>>6);
        av_log(NULL, AV_LOG_DEBUG, "channel_configuration=%d\n", channel_configuration);
	frame_length = ((int32_t)(header[3]&0x03)<<11) | ((int32_t)header[4]<<3) | ((header[5]&0xE0)>>5);
	number_of_raw_data_blocks_in_frame = (header[6]&0x03);

	*sample_rate = ff_mpeg4audio_sample_rates[sampling_frequency_index];
	//*channel_cnt = ff_mpeg4audio_channels[channel_configuration];
	*frame_cnt = (number_of_raw_data_blocks_in_frame + 1);

	avio_skip(pb, frame_length - 7);
}

static void adts_parse_total_sample_count(AVFormatContext *s)
{
	// REF ISO/IEC 13818-7 Syntax Part
	// 1. save original position
	// 2. seek 0
	// 3. count audio frame
	// 3.1. find next sync code
	// 3.2. parse header to obtain frame num except EOS, then goto 3.1
	// 4. recover original position

	int64_t position = avio_tell(s->pb);
    int32_t ret;

	int32_t sample_rate = 0, frame_count = 0;
	int32_t total_frame_count = 0, total_sample_count = 0;

	if (s->pb->seekable) {

		avio_seek(s->pb, 0, SEEK_SET);

		while (1) {
			ret = adts_find_next_sync_code(s->pb);
			if (ret > 0) {
				adts_get_frame_count(s->pb, &frame_count, &sample_rate);
				total_frame_count += frame_count;
			} else {
				break;
			}
		}

		total_sample_count = total_frame_count*1024;
		s->precise_duration_aac_fmt = ((int64_t)total_sample_count * AV_TIME_BASE)/sample_rate;


		avio_seek(s->pb, position, SEEK_SET);
	}
}

static int adts_aac_read_header(AVFormatContext *s,
                                AVFormatParameters *ap)
{
    AVStream *st;

    st = av_new_stream(s, 0);
    if (!st)
        return AVERROR(ENOMEM);

    st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    st->codec->codec_id = s->iformat->value;
    st->need_parsing = AVSTREAM_PARSE_FULL;

    ff_id3v1_read(s);

    adts_parse_total_sample_count(s);

    //LCM of all possible ADTS sample rates
    av_set_pts_info(st, 64, 1, 28224000);

    return 0;
}

AVInputFormat ff_aac_demuxer = {
    "aac",
    NULL_IF_CONFIG_SMALL("raw ADTS AAC"),
    0,
    adts_aac_probe,
    adts_aac_read_header,
    ff_raw_read_partial_packet,
    .flags= AVFMT_GENERIC_INDEX,
    .extensions = "aac",
    .value = CODEC_ID_AAC,
};
