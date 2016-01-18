/**
 * ffmpeg_util.cpp
 *
 * History:
 *  2012/05/09 - [Zhi He] created file
 *
 * Desc: some common used function, based on ffmpeg
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"

extern "C" {
#define INT64_C(a) (a ## LL)
#include "libavutil/avstring.h"
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec_export.h"
}

enum {
    eAudioObjectType_AAC_MAIN = 1,
    eAudioObjectType_AAC_LC = 2,
    eAudioObjectType_AAC_SSR = 3,
    eAudioObjectType_AAC_LTP = 4,
    eAudioObjectType_AAC_scalable = 6,
    //add others, todo

    eSamplingFrequencyIndex_96000 = 0,
    eSamplingFrequencyIndex_88200 = 1,
    eSamplingFrequencyIndex_64000 = 2,
    eSamplingFrequencyIndex_48000 = 3,
    eSamplingFrequencyIndex_44100 = 4,
    eSamplingFrequencyIndex_32000 = 5,
    eSamplingFrequencyIndex_24000 = 6,
    eSamplingFrequencyIndex_22050 = 7,
    eSamplingFrequencyIndex_16000 = 8,
    eSamplingFrequencyIndex_12000 = 9,
    eSamplingFrequencyIndex_11025 = 0xa,
    eSamplingFrequencyIndex_8000 = 0xb,
    eSamplingFrequencyIndex_7350 = 0xc,
    eSamplingFrequencyIndex_escape = 0xf,//should not be this value
};

//refer to iso14496-3
typedef struct
{
    AM_U8 samplingFrequencyIndex_high : 3;
    AM_U8 audioObjectType : 5;
    AM_U8 bitLeft : 3;
    AM_U8 channelConfiguration : 4;
    AM_U8 samplingFrequencyIndex_low : 1;
} __attribute__((packed))SSimpleAudioSpecificConfig;

static void _generate_audio_extra_data(enum CodecID codec_id, AM_INT samplerate, AM_INT channel_number, AVCodecContext * audio_enc)
{
    SSimpleAudioSpecificConfig* p_simple_header;
    switch (codec_id) {
        case CODEC_ID_AAC:
            audio_enc->extradata_size = 2;
            audio_enc->extradata = (AM_U8*)av_mallocz(2);
            p_simple_header = (SSimpleAudioSpecificConfig*)audio_enc->extradata;
            p_simple_header->audioObjectType = eAudioObjectType_AAC_LC;//hard code here
            switch (samplerate) {
                case 44100:
                    samplerate = eSamplingFrequencyIndex_44100;
                    break;
                case 48000:
                    samplerate = eSamplingFrequencyIndex_48000;
                    break;
                case 24000:
                    samplerate = eSamplingFrequencyIndex_24000;
                    break;
                case 16000:
                    samplerate = eSamplingFrequencyIndex_16000;
                    break;
                case 8000:
                    samplerate = eSamplingFrequencyIndex_8000;
                    break;
                case 12000:
                    samplerate = eSamplingFrequencyIndex_12000;
                    break;
                case 32000:
                    samplerate = eSamplingFrequencyIndex_32000;
                    break;
                case 22050:
                    samplerate = eSamplingFrequencyIndex_22050;
                    break;
                case 11025:
                    samplerate = eSamplingFrequencyIndex_11025;
                    break;
                default:
                    AM_ERROR("NOT support sample rate (%d) here.\n", samplerate);
                    break;
            }
            p_simple_header->samplingFrequencyIndex_high = samplerate >> 1;
            p_simple_header->samplingFrequencyIndex_low = samplerate & 0x1;
            p_simple_header->channelConfiguration = channel_number;
            p_simple_header->bitLeft = 0;
            //audio_enc->frame_size = 1152;
            break;
        case CODEC_ID_MP2:
        case CODEC_ID_MP3:
            break;
        default:
            AM_ERROR("NOT support audio codec (%d) here.\n", codec_id);
            break;
    }
}

static void _muxer_new_video_stream(AVFormatContext* pFormat, enum CodecID codec_id,
        enum PixelFormat pixel_format,
        AM_INT pic_width, AM_INT pic_height,
        AM_INT timebase_num, AM_INT timebase_den,
        AM_INT framerate_num, AM_INT framerate_den,
        AM_INT bitrate, AM_INT gopsize, AM_INT has_b_frames)
{
    AVStream* pStream = av_new_stream(pFormat, pFormat->nb_streams);
    AVCodecContext * video_enc = NULL;

    pStream->probe_data.filename = pFormat->filename;
    avcodec_get_context_defaults2(pStream->codec, AVMEDIA_TYPE_VIDEO);
    video_enc = pStream->codec;
    if(pFormat->oformat->flags & AVFMT_GLOBALHEADER) {
        video_enc->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    video_enc->codec_id = codec_id;

#if 0
    video_enc->time_base.num = framerate_den;
    video_enc->time_base.den = framerate_num*3000;
    AM_ERROR("[hard code]: check ffmpeg's code, (why need x3000, to fix frame rate issue)\n");
#else
    video_enc->time_base.num = timebase_num;
    video_enc->time_base.den = timebase_den;
#endif

    AM_INFO("[check]: framerate_num %d, framerate_den %d.\n", framerate_num, framerate_den);
    AM_INFO("[check]: encoder->time_base num %d, den %d.\n", video_enc->time_base.num, video_enc->time_base.den);

    pStream->time_base.num = timebase_num;
    pStream->time_base.den = timebase_den;

    AM_INFO("[check]: timebase_num %d, timebase_den %d.\n", timebase_num, timebase_den);

    video_enc->width = pic_width;
    video_enc->height = pic_height;

    video_enc->sample_aspect_ratio = pStream->sample_aspect_ratio = (AVRational) {1, 1};
    video_enc->has_b_frames = has_b_frames;
    video_enc->bit_rate = bitrate;
    video_enc->gop_size = gopsize;
    video_enc->pix_fmt = pixel_format;
    video_enc->codec_type = AVMEDIA_TYPE_VIDEO;
}

static void _muxer_new_audio_stream(AVFormatContext* pFormat, enum CodecID codec_id,
        AM_INT samplerate, AM_INT channels,
        AM_INT timebase_num, AM_INT timebase_den,
        AM_INT bitrate, AM_INT frame_size, enum AVSampleFormat sampleformat)
{
    AVStream* pStream = av_new_stream(pFormat, pFormat->nb_streams);
    AVCodecContext * audio_enc = NULL;

    pStream->probe_data.filename = pFormat->filename;
    avcodec_get_context_defaults2(pStream->codec, AVMEDIA_TYPE_AUDIO);
    audio_enc = pStream->codec;
    if(pFormat->oformat->flags & AVFMT_GLOBALHEADER) {
        audio_enc->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    audio_enc->codec_id = codec_id;

    pStream->time_base.num = timebase_num;
    pStream->time_base.den = timebase_den;

    audio_enc->bit_rate = bitrate;
    audio_enc->sample_rate = samplerate;
    audio_enc->sample_fmt = sampleformat;
    audio_enc->frame_size = frame_size;

    if (1 == channels) {
        audio_enc->channels = 1;
        audio_enc->channel_layout = CH_LAYOUT_MONO;
    } else if (2 == channels) {
        audio_enc->channel_layout = CH_LAYOUT_STEREO;
        audio_enc->channels = 2;
    } else {
        AM_ERROR("channel >2 is not supported yet.\n");
        audio_enc->channels = 2;
        audio_enc->channel_layout = CH_LAYOUT_STEREO;
    }
    audio_enc->codec_type = AVMEDIA_TYPE_AUDIO;

    _generate_audio_extra_data(codec_id, samplerate, audio_enc->channels, audio_enc);

    AM_INFO("[check audio parameters] 1. bit rate %d, samplerate %d, channels %d.\n", audio_enc->bit_rate, audio_enc->sample_rate, audio_enc->channels);
    AM_INFO("[check audio parameters] 1. codec_id 0x%x, codec_type %d, sample_fmt %d.\n", audio_enc->codec_id, audio_enc->codec_type, audio_enc->sample_fmt);

    AVCodec *codec;
    if (!(codec = avcodec_find_encoder(audio_enc->codec_id))) {
        AM_ERROR("Failed to find audio codec\n");
        return;
    }
    if (avcodec_open(audio_enc, codec) < 0) {
        AM_ERROR("Failed to open audio codec\n");
        return;
    }

}

static AVFormatContext* _open_muxer(char* filename,
        enum CodecID video_codec_id, enum CodecID audio_codec_id,
        enum PixelFormat pixel_format,
        AM_INT pic_width, AM_INT pic_height,
        AM_INT timebase_num, AM_INT timebase_den,
        AM_INT framerate_num, AM_INT framerate_den,
        AM_INT video_bitrate, AM_INT gopsize, AM_INT has_b_frames,
        AM_INT samplerate, AM_INT channels,
        AM_INT audio_bitrate, AM_INT frame_size, enum AVSampleFormat sampleformat)
{
    AVFormatContext* pFormat;
    AVOutputFormat* pOutFormat;
    AVFormatParameters AVFormatParam;
    AM_INT ret;

    pOutFormat = av_guess_format(NULL, filename, NULL);
    if (!pOutFormat) {
        AM_ERROR("Could not deduce output format from file(%s) extension.\n", filename);
        return NULL;
    }

    pFormat = avformat_alloc_context();
    if (!pFormat) {
        AM_ERROR("avformat_alloc_context fail.\n");
        return NULL;
    }

    pFormat->oformat = pOutFormat;
    snprintf(pFormat->filename, sizeof(pFormat->filename), "%s", filename);

    memset(&AVFormatParam, 0, sizeof(AVFormatParam));
    if (av_set_parameters(pFormat, &AVFormatParam) < 0) {
        AM_ERROR("av_set_parameters error.\n");
        av_free(pFormat);
        return NULL;
    }

    if (CODEC_ID_NONE != video_codec_id) {
        AM_INFO("new video stream %d.\n", video_codec_id);
        _muxer_new_video_stream(pFormat, video_codec_id, pixel_format, pic_width, pic_height, timebase_num, timebase_den, framerate_num, framerate_den, video_bitrate, gopsize, has_b_frames);
    }

    if (CODEC_ID_NONE != audio_codec_id) {
        AM_INFO("new audio stream %d.\n", audio_codec_id);
        _muxer_new_audio_stream(pFormat, audio_codec_id, samplerate, channels, timebase_num, timebase_den, audio_bitrate, frame_size, sampleformat);
    }

    //debug only
    av_dump_format(pFormat, 0, filename, 1);

    AM_INFO("** before avio_open, filename %s.\n", filename);
    /* open the output file, if needed */
    if (!(pFormat->oformat ->flags & AVFMT_NOFILE)) {

        ret = avio_open(&pFormat->pb, filename, AVIO_FLAG_WRITE);

        if (ret  < 0) {
            AM_ERROR("Could not open '%s', ret %d, READ-ONLY file system, or BAD filepath/filename?\n", filename, ret);
            av_free(pFormat);
            return NULL;
        }

    }

    ret = av_write_header(pFormat);
    if (ret < 0) {
        AM_ERROR("av_write_header fail, ret %d\n", ret);
        av_free(pFormat);
        return NULL;
    }

    return pFormat;
}

static AVCodecContext* _open_video_encoder(enum CodecID codec_id, enum PixelFormat pixel_format, AM_INT pic_width, AM_INT pic_height,
        AM_INT bitrate, AM_INT framerate_num, AM_INT framerate_den, AM_INT gopsize, AM_INT max_b_frames)
{
    AVCodec *codec;
    AVCodecContext* encoder;
    codec = avcodec_find_encoder(codec_id);

    if (!codec) {
        AM_ERROR("cannot find encoder(id %d).\n", codec_id);
        return NULL;
    }

    encoder = avcodec_alloc_context();
    encoder->codec_id = codec_id;
    encoder->codec_type = AVMEDIA_TYPE_VIDEO;
    AM_ASSERT(AVMEDIA_TYPE_VIDEO == codec->type);
    encoder->pix_fmt = pixel_format;
    encoder->width = pic_width;
    encoder->height = pic_height;

    if (bitrate) {
        encoder->bit_rate = bitrate;
    }

    if (framerate_num && framerate_den) {
        encoder->time_base= (AVRational){framerate_den, framerate_num};
    }

    if (gopsize) {
        encoder->gop_size = gopsize;
    }

    if (max_b_frames) {
        encoder->max_b_frames = max_b_frames;
    }

    //open codec
    if (avcodec_open(encoder, codec) < 0) {
        AM_ERROR("could not open codec\n");
        av_free(encoder);
        return NULL;
    }

    return encoder;
}

static void _encoding_to_jpg_file(char *filename, AVCodecContext  *pJepgCodecCtx, AVFrame *picture, AM_INT width, AM_INT height, AM_U8* p_buffer, AM_INT buffer_size, AM_INT jpeg_bitrate)
{
    AM_INT ret;
    AVFormatContext* pjpegfile;

    //some debug check
    AM_ASSERT(filename);
    AM_ASSERT(pJepgCodecCtx);
    AM_ASSERT(picture);
    AM_ASSERT(width > 0);
    AM_ASSERT(height > 0);
    AM_ASSERT(p_buffer);
    AM_ASSERT(buffer_size);

    pjpegfile = _open_muxer(filename, CODEC_ID_MJPEG, CODEC_ID_NONE, pJepgCodecCtx->pix_fmt, width, height, 1, 90000, 10, 1, jpeg_bitrate, 0, 0, 0, 0, 0, 0, AV_SAMPLE_FMT_NONE);
    if (!pjpegfile) {
        AM_ERROR("open jpeg muxer fail.\n");
        return;
    }

    ret = avcodec_encode_video(pJepgCodecCtx, p_buffer, buffer_size, picture);
    if (ret <= 0) {
        AM_ERROR("encoding to jpeg fail.\n");
        av_close_input_file(pjpegfile);
        return;
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.stream_index = 0;
    packet.size = ret;
    packet.data = p_buffer;

    AM_INFO("before write jpeg file, data size %d, stream index %d.\n", packet.size, packet.stream_index);
    if ((ret = av_write_frame(pjpegfile, &packet)) != 0) {
        AM_ERROR("write jpeg file fail.\n");
    } else {
        if ((ret =av_write_trailer(pjpegfile)) < 0) {
            AM_ERROR("av_write_trailer err\n");
        }
    }

    //release context
    if (!(pjpegfile->oformat ->flags & AVFMT_NOFILE)) {
        avio_close(pjpegfile->pb);
    }
    avformat_free_context(pjpegfile);
}

AM_ERR FF_GenerateJpegFile(char* filename, AM_U8* buffer[], AM_UINT width, AM_UINT height, IParameters::PixFormat format, AM_UINT jpeg_bitrate, AM_U8* bitstream_buffer, AM_UINT bitstream_buffer_size)
{
    enum PixelFormat jpeg_pixelformat = PIX_FMT_YUVJ420P;
    enum CodecID jpeg_codec_id = CODEC_ID_MJPEG;
    AVCodecContext  *pJepgCodecCtx = NULL;
    AVFrame* pFrame = NULL;
    AM_U8* pbitstream_buffer = NULL;

    AM_ASSERT(filename);
    AM_ASSERT(IParameters::PixFormat_YUV420P == format);

    if (!jpeg_bitrate) {
        //use default
        jpeg_bitrate = 100000;//default
    }

    //prepare buffer for decoding output
    pFrame = avcodec_alloc_frame();
    if (!pFrame) {
        AM_ERROR("alloc pFrame fail.\n");
        return ME_ERROR;
    }

    pJepgCodecCtx = _open_video_encoder(jpeg_codec_id, jpeg_pixelformat, width, height, jpeg_bitrate, 10, 1, 0, 0);
    if (!pJepgCodecCtx) {
        AM_ERROR("open codec id %d, pix format %d encoder fail.\n", jpeg_codec_id, jpeg_pixelformat);
        return ME_ERROR;
    }

    pFrame->data[0] = buffer[0];
    pFrame->data[1] = buffer[1];
    pFrame->data[2] = buffer[2];

    pFrame->linesize[0] = width;
    pFrame->linesize[1] = width/2;
    pFrame->linesize[2] = width/2;

    if (!bitstream_buffer) {
        pbitstream_buffer = (AM_U8*)malloc(width*height*2);
        bitstream_buffer_size = width*height*2;
    } else {
        pbitstream_buffer = bitstream_buffer;
    }

    AM_ASSERT(pbitstream_buffer);
    _encoding_to_jpg_file(filename, pJepgCodecCtx, pFrame, width, height, pbitstream_buffer, bitstream_buffer_size, jpeg_bitrate);

    avcodec_close(pJepgCodecCtx);
    pJepgCodecCtx = NULL;

    if (!bitstream_buffer) {
        free(pbitstream_buffer);
    }

    if (pFrame) {
        av_free(pFrame);
    }

    return ME_OK;
}

