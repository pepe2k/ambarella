/*
 * copyright (c) 2010
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

#ifndef AVCODEC_EXPORT_H
#define AVCODEC_EXPORT_H

#include "libavformat/avformat.h"


enum InterlacedMode
{
    INTERLACED_MODE_INVALID,
    INTERLACED_MODE_FULL_MODE,      // deint_mode = 0 (full mode) for 480i/576i bit stream to get a better quality.
    INTERLACED_MODE_SIMPLE_MODE,    // deint_mode = 1 (simple mode) for 1080i bit stream due to hardware capability.
    INTERLACED_MODE_CNT
};

/**
 * @file
 * external API header
 */
typedef struct SeqHeader {
    enum CodecID codecID;
    AVRational framerate;
    int width;
    int height;
    enum PixelFormat pix_fmt;

    int  is_interlaced;
    enum InterlacedMode interlaced_mode;
} SeqHeader;

typedef struct H264SeqHeader {
  SeqHeader commSeqHeader;
  int bit_depth_luma;
  int bit_depth_chroma;
} H264SeqHeader;

typedef struct VC1SeqHeader {
  SeqHeader commSeqHeader;

  int profile;

  // Simple/Main Profile
  int res_y411;
  int res_sprite;  //< reserved, sprite mode, MVP2
  int res_rtm_flag; //old WMV9
  /** Advanced Profile */
  //@{
  int broadcast;        ///< TFF/RFF present = PULLDOWN flag
  int interlace;        ///< Progressive/interlaced (RPTFTM syntax element)
  //@}
  AVCodecContext *avctx;
} VC1SeqHeader;

typedef struct MPEG12SeqHeader {
  SeqHeader commSeqHeader;
} MPEG12SeqHeader;

typedef struct MPEG4SeqHeader {
  SeqHeader commSeqHeader;
  int num_sprite_warping_points;
  int real_sprite_warping_points;
  int scalability;
  int quarter_sample;

  int divx_packed;
  AVCodecContext *avctx;
} MPEG4SeqHeader;


/**
 *  Usage:
 *     invoke InitSeqHeader & DeinitSeqHeader in pair,
 *     or it'll cause memory leak
 *
 */
int InitSeqHeader(enum CodecID codecID, SeqHeader **ppSeqHeader);
void DeinitSeqHeader(SeqHeader **ppSeqHeader);

/**
 * Parse extradata.
 *
 * @param avctx          codec context.
 * @param pExtradata     ptr to extradata buf
 * @param iExtradataSize extradata size
 * @param ppSeqHeader    ptr to ptr to SeqHeader
 * @return               whether it succeeds in parsing extradata.
 *
 * Example:
 * @code
 *   AVFormatContext *ic£»
 *   av_open_input_file(&ic, ...)
 *
 *   av_find_stream_info(ic)       // extradata is split by invoking av_find_stream_info
 *
 *   int idx = getStreamIndex(...)
 *
 *   SeqHeader *pSeqHeader = NULL;
 *   int ret = InitSeqHeader(&pSeqHeader);
 *   if (ret < 0) return E_FAIL;
 *
 *   ParseExtradata(ic->stream[idx]->codec, &pSeqHeader);
 *
 *   // process SeqHeader....
 *
 *   DeinitSeqHeader(&pSeqHeader);
 *
 * @endcode
 */
int GetSeqHeader(AVStream *stream, SeqHeader *pSeqHeader);

enum SEQ_HEADER_RET
{
    SEQ_HEADER_RET_INVALID,
    SEQ_HEADER_RET_DSP_UNSUPPORTED,
    SEQ_HEADER_RET_PLAYER_UNSUPPORTED,
    SEQ_HEADER_RET_HYBRID_UNSUPPORTED,
    SEQ_HEADER_RET_OK
};

enum SEQ_HEADER_RET CheckSeqHeader(const SeqHeader *pSeqHeader);


#endif
