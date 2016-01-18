/*
 * RV10/RV20 parallel nv12 decoder
 * Copyright (c) 2000,2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer
 * Copyright (c) 2010- zhe@ambarella.com
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

/**
 * @file libavcodec/amba_rv12.c
 * RV10/RV20 parallel nv12 decoder
 */

#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "amba_h263.h"

//#define DEBUG


#define DC_VLC_BITS 14 //FIXME find a better solution

static const uint16_t rv_lum_code[256] =
{
 0x3e7f, 0x0f00, 0x0f01, 0x0f02, 0x0f03, 0x0f04, 0x0f05, 0x0f06,
 0x0f07, 0x0f08, 0x0f09, 0x0f0a, 0x0f0b, 0x0f0c, 0x0f0d, 0x0f0e,
 0x0f0f, 0x0f10, 0x0f11, 0x0f12, 0x0f13, 0x0f14, 0x0f15, 0x0f16,
 0x0f17, 0x0f18, 0x0f19, 0x0f1a, 0x0f1b, 0x0f1c, 0x0f1d, 0x0f1e,
 0x0f1f, 0x0f20, 0x0f21, 0x0f22, 0x0f23, 0x0f24, 0x0f25, 0x0f26,
 0x0f27, 0x0f28, 0x0f29, 0x0f2a, 0x0f2b, 0x0f2c, 0x0f2d, 0x0f2e,
 0x0f2f, 0x0f30, 0x0f31, 0x0f32, 0x0f33, 0x0f34, 0x0f35, 0x0f36,
 0x0f37, 0x0f38, 0x0f39, 0x0f3a, 0x0f3b, 0x0f3c, 0x0f3d, 0x0f3e,
 0x0f3f, 0x0380, 0x0381, 0x0382, 0x0383, 0x0384, 0x0385, 0x0386,
 0x0387, 0x0388, 0x0389, 0x038a, 0x038b, 0x038c, 0x038d, 0x038e,
 0x038f, 0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396,
 0x0397, 0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e,
 0x039f, 0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6,
 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce,
 0x00cf, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056,
 0x0057, 0x0020, 0x0021, 0x0022, 0x0023, 0x000c, 0x000d, 0x0004,
 0x0000, 0x0005, 0x000e, 0x000f, 0x0024, 0x0025, 0x0026, 0x0027,
 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
 0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
 0x03a0, 0x03a1, 0x03a2, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7,
 0x03a8, 0x03a9, 0x03aa, 0x03ab, 0x03ac, 0x03ad, 0x03ae, 0x03af,
 0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
 0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
 0x0f40, 0x0f41, 0x0f42, 0x0f43, 0x0f44, 0x0f45, 0x0f46, 0x0f47,
 0x0f48, 0x0f49, 0x0f4a, 0x0f4b, 0x0f4c, 0x0f4d, 0x0f4e, 0x0f4f,
 0x0f50, 0x0f51, 0x0f52, 0x0f53, 0x0f54, 0x0f55, 0x0f56, 0x0f57,
 0x0f58, 0x0f59, 0x0f5a, 0x0f5b, 0x0f5c, 0x0f5d, 0x0f5e, 0x0f5f,
 0x0f60, 0x0f61, 0x0f62, 0x0f63, 0x0f64, 0x0f65, 0x0f66, 0x0f67,
 0x0f68, 0x0f69, 0x0f6a, 0x0f6b, 0x0f6c, 0x0f6d, 0x0f6e, 0x0f6f,
 0x0f70, 0x0f71, 0x0f72, 0x0f73, 0x0f74, 0x0f75, 0x0f76, 0x0f77,
 0x0f78, 0x0f79, 0x0f7a, 0x0f7b, 0x0f7c, 0x0f7d, 0x0f7e, 0x0f7f,
};

static const uint8_t rv_lum_bits[256] =
{
 14, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10,
 10,  8,  8,  8,  8,  8,  8,  8,
  8,  8,  8,  8,  8,  8,  8,  8,
  8,  7,  7,  7,  7,  7,  7,  7,
  7,  6,  6,  6,  6,  5,  5,  4,
  2,  4,  5,  5,  6,  6,  6,  6,
  7,  7,  7,  7,  7,  7,  7,  7,
  8,  8,  8,  8,  8,  8,  8,  8,
  8,  8,  8,  8,  8,  8,  8,  8,
 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
};

static const uint16_t rv_chrom_code[256] =
{
 0xfe7f, 0x3f00, 0x3f01, 0x3f02, 0x3f03, 0x3f04, 0x3f05, 0x3f06,
 0x3f07, 0x3f08, 0x3f09, 0x3f0a, 0x3f0b, 0x3f0c, 0x3f0d, 0x3f0e,
 0x3f0f, 0x3f10, 0x3f11, 0x3f12, 0x3f13, 0x3f14, 0x3f15, 0x3f16,
 0x3f17, 0x3f18, 0x3f19, 0x3f1a, 0x3f1b, 0x3f1c, 0x3f1d, 0x3f1e,
 0x3f1f, 0x3f20, 0x3f21, 0x3f22, 0x3f23, 0x3f24, 0x3f25, 0x3f26,
 0x3f27, 0x3f28, 0x3f29, 0x3f2a, 0x3f2b, 0x3f2c, 0x3f2d, 0x3f2e,
 0x3f2f, 0x3f30, 0x3f31, 0x3f32, 0x3f33, 0x3f34, 0x3f35, 0x3f36,
 0x3f37, 0x3f38, 0x3f39, 0x3f3a, 0x3f3b, 0x3f3c, 0x3f3d, 0x3f3e,
 0x3f3f, 0x0f80, 0x0f81, 0x0f82, 0x0f83, 0x0f84, 0x0f85, 0x0f86,
 0x0f87, 0x0f88, 0x0f89, 0x0f8a, 0x0f8b, 0x0f8c, 0x0f8d, 0x0f8e,
 0x0f8f, 0x0f90, 0x0f91, 0x0f92, 0x0f93, 0x0f94, 0x0f95, 0x0f96,
 0x0f97, 0x0f98, 0x0f99, 0x0f9a, 0x0f9b, 0x0f9c, 0x0f9d, 0x0f9e,
 0x0f9f, 0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4, 0x03c5, 0x03c6,
 0x03c7, 0x03c8, 0x03c9, 0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce,
 0x03cf, 0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6,
 0x00e7, 0x0030, 0x0031, 0x0032, 0x0033, 0x0008, 0x0009, 0x0002,
 0x0000, 0x0003, 0x000a, 0x000b, 0x0034, 0x0035, 0x0036, 0x0037,
 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
 0x03d0, 0x03d1, 0x03d2, 0x03d3, 0x03d4, 0x03d5, 0x03d6, 0x03d7,
 0x03d8, 0x03d9, 0x03da, 0x03db, 0x03dc, 0x03dd, 0x03de, 0x03df,
 0x0fa0, 0x0fa1, 0x0fa2, 0x0fa3, 0x0fa4, 0x0fa5, 0x0fa6, 0x0fa7,
 0x0fa8, 0x0fa9, 0x0faa, 0x0fab, 0x0fac, 0x0fad, 0x0fae, 0x0faf,
 0x0fb0, 0x0fb1, 0x0fb2, 0x0fb3, 0x0fb4, 0x0fb5, 0x0fb6, 0x0fb7,
 0x0fb8, 0x0fb9, 0x0fba, 0x0fbb, 0x0fbc, 0x0fbd, 0x0fbe, 0x0fbf,
 0x3f40, 0x3f41, 0x3f42, 0x3f43, 0x3f44, 0x3f45, 0x3f46, 0x3f47,
 0x3f48, 0x3f49, 0x3f4a, 0x3f4b, 0x3f4c, 0x3f4d, 0x3f4e, 0x3f4f,
 0x3f50, 0x3f51, 0x3f52, 0x3f53, 0x3f54, 0x3f55, 0x3f56, 0x3f57,
 0x3f58, 0x3f59, 0x3f5a, 0x3f5b, 0x3f5c, 0x3f5d, 0x3f5e, 0x3f5f,
 0x3f60, 0x3f61, 0x3f62, 0x3f63, 0x3f64, 0x3f65, 0x3f66, 0x3f67,
 0x3f68, 0x3f69, 0x3f6a, 0x3f6b, 0x3f6c, 0x3f6d, 0x3f6e, 0x3f6f,
 0x3f70, 0x3f71, 0x3f72, 0x3f73, 0x3f74, 0x3f75, 0x3f76, 0x3f77,
 0x3f78, 0x3f79, 0x3f7a, 0x3f7b, 0x3f7c, 0x3f7d, 0x3f7e, 0x3f7f,
};

static const uint8_t rv_chrom_bits[256] =
{
 16, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10,
 10,  8,  8,  8,  8,  8,  8,  8,
  8,  6,  6,  6,  6,  4,  4,  3,
  2,  3,  4,  4,  6,  6,  6,  6,
  8,  8,  8,  8,  8,  8,  8,  8,
 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14,
};

static VLC rv_dc_lum, rv_dc_chrom;

int rv_decode_dc(MpegEncContext *s, int n)
{
    int code;

    if (n < 4) {
        code = get_vlc2(&s->gb, rv_dc_lum.table, DC_VLC_BITS, 2);
        if (code < 0) {
            /* XXX: I don't understand why they use LONGER codes than
               necessary. The following code would be completely useless
               if they had thought about it !!! */
            code = get_bits(&s->gb, 7);
            if (code == 0x7c) {
                code = (int8_t)(get_bits(&s->gb, 7) + 1);
            } else if (code == 0x7d) {
                code = -128 + get_bits(&s->gb, 7);
            } else if (code == 0x7e) {
                if (get_bits1(&s->gb) == 0)
                    code = (int8_t)(get_bits(&s->gb, 8) + 1);
                else
                    code = (int8_t)(get_bits(&s->gb, 8));
            } else if (code == 0x7f) {
                skip_bits(&s->gb, 11);
                code = 1;
            }
        } else {
            code -= 128;
        }
    } else {
        code = get_vlc2(&s->gb, rv_dc_chrom.table, DC_VLC_BITS, 2);
        /* same remark */
        if (code < 0) {
            code = get_bits(&s->gb, 9);
            if (code == 0x1fc) {
                code = (int8_t)(get_bits(&s->gb, 7) + 1);
            } else if (code == 0x1fd) {
                code = -128 + get_bits(&s->gb, 7);
            } else if (code == 0x1fe) {
                skip_bits(&s->gb, 9);
                code = 1;
            } else {
                av_log(s->avctx, AV_LOG_ERROR, "chroma dc error\n");
                return 0xffff;
            }
        } else {
            code -= 128;
        }
    }
    return -code;
}

/* read RV 1.0 compatible frame header */
static int rv10_decode_picture_header_amba(MpegEncContext *s)
{
    int mb_count, pb_frame, marker, unk, mb_xy;

    marker = get_bits1(&s->gb);

    if (get_bits1(&s->gb))
        s->pict_type = FF_P_TYPE;
    else
        s->pict_type = FF_I_TYPE;
    if(!marker) av_log(s->avctx, AV_LOG_ERROR, "marker missing\n");
    pb_frame = get_bits1(&s->gb);

    av_dlog(s->avctx, "pict_type=%d pb_frame=%d\n", s->pict_type, pb_frame);

    if (pb_frame){
        av_log(s->avctx, AV_LOG_ERROR, "pb frame not supported\n");
        return -1;
    }

    s->qscale = get_bits(&s->gb, 5);
    if(s->qscale==0){
        av_log(s->avctx, AV_LOG_ERROR, "error, qscale:0\n");
        return -1;
    }

    if (s->pict_type == FF_I_TYPE) {
        if (s->rv10_version == 3) {
            /* specific MPEG like DC coding not used */
            s->last_dc[0] = get_bits(&s->gb, 8);
            s->last_dc[1] = get_bits(&s->gb, 8);
            s->last_dc[2] = get_bits(&s->gb, 8);
            av_dlog(s->avctx, "DC:%d %d %d\n", s->last_dc[0],
                    s->last_dc[1], s->last_dc[2]);
        }
    }
    /* if multiple packets per frame are sent, the position at which
       to display the macroblocks is coded here */

    mb_xy= s->mb_x + s->mb_y*s->mb_width;
    if(show_bits(&s->gb, 12)==0 || (mb_xy && mb_xy < s->mb_num)){
        s->mb_x = get_bits(&s->gb, 6); /* mb_x */
        s->mb_y = get_bits(&s->gb, 6); /* mb_y */
        mb_count = get_bits(&s->gb, 12);
    } else {
        s->mb_x = 0;
        s->mb_y = 0;
        mb_count = s->mb_width * s->mb_height;
    }
    unk= get_bits(&s->gb, 3);   /* ignored */
    s->f_code = 1;
    s->unrestricted_mv = 1;

    return mb_count;
}

static int rv20_decode_picture_header_amba(MpegEncContext *s)
{
    int seq, mb_pos, i;

#if 0
    GetBitContext gb= s->gb;
    for(i=0; i<64; i++){
        av_log(s->avctx, AV_LOG_DEBUG, "%d", get_bits1(&gb));
        if(i%4==3) av_log(s->avctx, AV_LOG_DEBUG, " ");
    }
    av_log(s->avctx, AV_LOG_DEBUG, "\n");
#endif
#if 0
    av_log(s->avctx, AV_LOG_DEBUG, "%3dx%03d/%02Xx%02X ", s->width, s->height, s->width/4, s->height/4);
    for(i=0; i<s->avctx->extradata_size; i++){
        av_log(s->avctx, AV_LOG_DEBUG, "%02X ", ((uint8_t*)s->avctx->extradata)[i]);
        if(i%4==3) av_log(s->avctx, AV_LOG_DEBUG, " ");
    }
    av_log(s->avctx, AV_LOG_DEBUG, "\n");
#endif

    if(s->avctx->sub_id == 0x30202002 || s->avctx->sub_id == 0x30203002){
        if (get_bits(&s->gb, 3)){
            av_log(s->avctx, AV_LOG_ERROR, "unknown triplet set\n");
            return -1;
        }
    }

    i= get_bits(&s->gb, 2);
    switch(i){
    case 0: s->pict_type= FF_I_TYPE; break;
    case 1: s->pict_type= FF_I_TYPE; break; //hmm ...
    case 2: s->pict_type= FF_P_TYPE; break;
    case 3: s->pict_type= FF_B_TYPE; break;
    default:
        av_log(s->avctx, AV_LOG_ERROR, "unknown frame type\n");
        return -1;
    }

    if(s->last_picture_ptr==NULL && s->pict_type==FF_B_TYPE){
        av_log(s->avctx, AV_LOG_ERROR, "early B pix\n");
        return -1;
    }

    if (get_bits1(&s->gb)){
        av_log(s->avctx, AV_LOG_ERROR, "unknown bit set\n");
        return -1;
    }

    s->qscale = get_bits(&s->gb, 5);
    if(s->qscale==0){
        av_log(s->avctx, AV_LOG_ERROR, "error, qscale:0\n");
        return -1;
    }
    if(s->avctx->sub_id == 0x30203002){
        if (get_bits1(&s->gb)){
            av_log(s->avctx, AV_LOG_ERROR, "unknown bit2 set\n");
            return -1;
        }
    }

    if(s->avctx->has_b_frames){
        int f, new_w, new_h;
        int v= s->avctx->extradata_size >= 4 ? 7&((uint8_t*)s->avctx->extradata)[1] : 0;

        if (get_bits1(&s->gb)){
            av_log(s->avctx, AV_LOG_ERROR, "unknown bit3 set\n");
        }
        seq= get_bits(&s->gb, 13)<<2;

        f= get_bits(&s->gb, av_log2(v)+1);

        if(f){
            new_w= 4*((uint8_t*)s->avctx->extradata)[6+2*f];
            new_h= 4*((uint8_t*)s->avctx->extradata)[7+2*f];
        }else{
            new_w= s->width; //FIXME wrong we of course must save the original in the context
            new_h= s->height;
        }
        if(new_w != s->width || new_h != s->height){
            av_log(s->avctx, AV_LOG_DEBUG, "attempting to change resolution to %dx%d\n", new_w, new_h);
            if (av_image_check_size(new_w, new_h, 0, s->avctx) < 0)
                return -1;
            MPV_common_end(s);
            s->width  = s->avctx->width = new_w;
            s->height = s->avctx->height= new_h;
            if (MPV_common_init(s) < 0)
                return -1;
        }

        if(s->avctx->debug & FF_DEBUG_PICT_INFO){
            av_log(s->avctx, AV_LOG_DEBUG, "F %d/%d\n", f, v);
        }
    }else{
        seq= get_bits(&s->gb, 8)*128;
    }

//     if(s->avctx->sub_id <= 0x20201002){ //0x20201002 definitely needs this
    mb_pos= ff_h263_decode_mba(s);
/*    }else{
        mb_pos= get_bits(&s->gb, av_log2(s->mb_num-1)+1);
        s->mb_x= mb_pos % s->mb_width;
        s->mb_y= mb_pos / s->mb_width;
    }*/
//av_log(s->avctx, AV_LOG_DEBUG, "%d\n", seq);
    seq |= s->time &~0x7FFF;
    if(seq - s->time >  0x4000) seq -= 0x8000;
    if(seq - s->time < -0x4000) seq += 0x8000;
    if(seq != s->time){
        if(s->pict_type!=FF_B_TYPE){
            s->time= seq;
            s->pp_time= s->time - s->last_non_b_time;
            s->last_non_b_time= s->time;
        }else{
            s->time= seq;
            s->pb_time= s->pp_time - (s->last_non_b_time - s->time);
            if(s->pp_time <=s->pb_time || s->pp_time <= s->pp_time - s->pb_time || s->pp_time<=0){
                av_log(s->avctx, AV_LOG_DEBUG, "messed up order, possible from seeking? skipping current b frame\n");
                return FRAME_SKIPPED;
            }
            ff_mpeg4_init_direct_mv(s);
        }
    }
//    printf("%d %d %d %d %d\n", seq, (int)s->time, (int)s->last_non_b_time, s->pp_time, s->pb_time);
/*for(i=0; i<32; i++){
    av_log(s->avctx, AV_LOG_DEBUG, "%d", get_bits1(&s->gb));
}
av_log(s->avctx, AV_LOG_DEBUG, "\n");*/
    s->no_rounding= get_bits1(&s->gb);

    s->f_code = 1;
    s->unrestricted_mv = 1;
    s->h263_aic= s->pict_type == FF_I_TYPE;
//    s->alt_inter_vlc=1;
//    s->obmc=1;
//    s->umvplus=1;
    s->modified_quant=1;
    if(!s->avctx->lowres)
        s->loop_filter=1;

    if(s->avctx->debug & FF_DEBUG_PICT_INFO){
            av_log(s->avctx, AV_LOG_INFO, "num:%5d x:%2d y:%2d type:%d qscale:%2d rnd:%d\n",
                   seq, s->mb_x, s->mb_y, s->pict_type, s->qscale, s->no_rounding);
    }

    assert(s->pict_type != FF_B_TYPE || !s->low_delay);

    return s->mb_width*s->mb_height - mb_pos;
}

static av_cold int rv10_decode_init(AVCodecContext *avctx)
{
    MpegEncContext *s = avctx->priv_data;
    static int done=0;

    if (avctx->extradata_size < 8) {
        av_log(avctx, AV_LOG_ERROR, "Extradata is too small.\n");
        return -1;
    }

    MPV_decode_defaults(s);

    s->avctx= avctx;
    s->out_format = FMT_H263;
    s->codec_id= avctx->codec_id;

    s->width = avctx->coded_width;
    s->height = avctx->coded_height;

    s->h263_long_vectors= ((uint8_t*)avctx->extradata)[3] & 1;
    avctx->sub_id= AV_RB32((uint8_t*)avctx->extradata + 4);

    if (avctx->sub_id == 0x10000000) {
        s->rv10_version= 0;
        s->low_delay=1;
    } else if (avctx->sub_id == 0x10001000) {
        s->rv10_version= 3;
        s->low_delay=1;
    } else if (avctx->sub_id == 0x10002000) {
        s->rv10_version= 3;
        s->low_delay=1;
        s->obmc=1;
    } else if (avctx->sub_id == 0x10003000) {
        s->rv10_version= 3;
        s->low_delay=1;
    } else if (avctx->sub_id == 0x10003001) {
        s->rv10_version= 3;
        s->low_delay=1;
    } else if (    avctx->sub_id == 0x20001000
               || (avctx->sub_id >= 0x20100000 && avctx->sub_id < 0x201a0000)) {
        s->low_delay=1;
    } else if (    avctx->sub_id == 0x30202002
               ||  avctx->sub_id == 0x30203002
               || (avctx->sub_id >= 0x20200002 && avctx->sub_id < 0x20300000)) {
        s->low_delay=0;
        s->avctx->has_b_frames=1;
    } else
        av_log(s->avctx, AV_LOG_ERROR, "unknown header %X\n", avctx->sub_id);

    if(avctx->debug & FF_DEBUG_PICT_INFO){
        av_log(avctx, AV_LOG_DEBUG, "ver:%X ver0:%X\n", avctx->sub_id, avctx->extradata_size >= 4 ? ((uint32_t*)avctx->extradata)[0] : -1);
    }

    avctx->pix_fmt = PIX_FMT_YUV420P;

    if (MPV_common_init(s) < 0)
        return -1;

    h263_decode_init_vlc(s);

    /* init rv vlc */
    if (!done) {
        INIT_VLC_STATIC(&rv_dc_lum, DC_VLC_BITS, 256,
                 rv_lum_bits, 1, 1,
                 rv_lum_code, 2, 2, 16384);
        INIT_VLC_STATIC(&rv_dc_chrom, DC_VLC_BITS, 256,
                 rv_chrom_bits, 1, 1,
                 rv_chrom_code, 2, 2, 16388);
        done = 1;
    }

    return 0;
}

static av_cold int rv10_decode_end(AVCodecContext *avctx)
{
    MpegEncContext *s = avctx->priv_data;

    MPV_common_end(s);
    return 0;
}

static int rv10_decode_packet(AVCodecContext *avctx,
                             const uint8_t *buf, int buf_size)
{
    MpegEncContext *s = avctx->priv_data;
    int mb_count, mb_pos, left, start_mb_x;

    init_get_bits(&s->gb, buf, buf_size*8);
    if(s->codec_id ==CODEC_ID_RV10)
        mb_count = rv10_decode_picture_header_amba(s);
    else
        mb_count = rv20_decode_picture_header_amba(s);
    if (mb_count < 0) {
        av_log(s->avctx, AV_LOG_ERROR, "HEADER ERROR\n");
        return -1;
    }

    if (s->mb_x >= s->mb_width ||
        s->mb_y >= s->mb_height) {
        av_log(s->avctx, AV_LOG_ERROR, "POS ERROR %d %d\n", s->mb_x, s->mb_y);
        return -1;
    }
    mb_pos = s->mb_y * s->mb_width + s->mb_x;
    left = s->mb_width * s->mb_height - mb_pos;
    if (mb_count > left) {
        av_log(s->avctx, AV_LOG_ERROR, "COUNT ERROR\n");
        return -1;
    }

    if ((s->mb_x == 0 && s->mb_y == 0) || s->current_picture_ptr==NULL) {
        if(s->current_picture_ptr){ //FIXME write parser so we always have complete frames?
            ff_er_frame_end(s);
            MPV_frame_end(s);
            s->mb_x= s->mb_y = s->resync_mb_x = s->resync_mb_y= 0;
        }
        if(MPV_frame_start(s, avctx) < 0)
            return -1;
        ff_er_frame_start(s);
    }

    av_dlog(avctx, "qscale=%d\n", s->qscale);

    /* default quantization values */
    if(s->codec_id== CODEC_ID_RV10){
        if(s->mb_y==0) s->first_slice_line=1;
    }else{
        s->first_slice_line=1;
        s->resync_mb_x= s->mb_x;
    }
    start_mb_x= s->mb_x;
    s->resync_mb_y= s->mb_y;
    if(s->h263_aic){
        s->y_dc_scale_table=
        s->c_dc_scale_table= ff_aic_dc_scale_table;
    }else{
        s->y_dc_scale_table=
        s->c_dc_scale_table= ff_mpeg1_dc_scale_table;
    }

    if(s->modified_quant)
        s->chroma_qscale_table= ff_h263_chroma_qscale_table;

    ff_set_qscale(s, s->qscale);

    s->rv10_first_dc_coded[0] = 0;
    s->rv10_first_dc_coded[1] = 0;
    s->rv10_first_dc_coded[2] = 0;
    s->block_wrap[0]=
    s->block_wrap[1]=
    s->block_wrap[2]=
    s->block_wrap[3]= s->b8_stride;
    s->block_wrap[4]=
    s->block_wrap[5]= s->mb_stride;
    ff_init_block_index(s);
    /* decode each macroblock */

    for(s->mb_num_left= mb_count; s->mb_num_left>0; s->mb_num_left--) {
        int ret;
        ff_update_block_index(s);
        av_dlog(avctx, "**mb x=%d y=%d\n", s->mb_x, s->mb_y);

        s->mv_dir = MV_DIR_FORWARD;
        s->mv_type = MV_TYPE_16X16;
        ret=ff_h263_decode_mb(s, s->block);

        if (ret == SLICE_ERROR || s->gb.size_in_bits < get_bits_count(&s->gb)) {
            av_log(s->avctx, AV_LOG_ERROR, "ERROR at MB %d %d\n", s->mb_x, s->mb_y);
            return -1;
        }
        if(s->pict_type != FF_B_TYPE)
            ff_h263_update_motion_val(s);
        MPV_decode_mb(s, s->block);
        if(s->loop_filter)
            ff_h263_loop_filter(s);

        if (++s->mb_x == s->mb_width) {
            s->mb_x = 0;
            s->mb_y++;
            ff_init_block_index(s);
        }
        if(s->mb_x == s->resync_mb_x)
            s->first_slice_line=0;
        if(ret == SLICE_END) break;
    }

    ff_er_add_slice(s, start_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);

    return buf_size;
}

static int get_slice_offset(AVCodecContext *avctx, const uint8_t *buf, int n)
{
    if(avctx->slice_count) return avctx->slice_offset[n];
    else                   return AV_RL32(buf + n*8);
}

void* thread_rv12_mc_idct_addresidue(void* p)
{
    H263AmbaDecContext_t *thiz=(H263AmbaDecContext_t *)p;
    MpegEncContext *s = &thiz->s;
    int ret;
    ctx_nodef_t* p_node;
    h263_pic_data_t* p_pic;
//    int i,j;
    
    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"thread_rv12_mc_idct_addresidue start.\n");
    #endif
    
    while(thiz->mc_idct_loop)
    {
        //get data
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Waiting: thread_rv12_mc_idct_addresidue input: wait on p_mc_idct_dataq.\n");
        #endif
        
        p_node=thiz->p_mc_idct_dataq->get(thiz->p_mc_idct_dataq);

        p_pic=(h263_vld_data_t*)p_node->p_ctx;
        
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Escape: thread_rv12_mc_idct_addresidue input: escape from p_mc_idct_dataq,p_node->p_ctx=%p.\n",p_node->p_ctx);
        #endif   
                
        if(!p_pic)
        {
            if(p_node->flag==_flag_cmd_exit_next_)
            {
                #ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"thread_rv12_mc_idct_addresidue get NULL data(_flag_cmd_exit_next_), start exit.\n");
                #endif
                
                #ifdef __log_parallel_control__
                    av_log(NULL,AV_LOG_ERROR,"**** thread_rv12_mc_idct_addresidue, get exit cmd, start exit.\n");
                #endif  
                
                goto mc_idct_thread_exit;
            }
            else
            {
                #ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"thread_rv12_mc_idct_addresidue get NULL data(_flag_cmd_exit_next_), start exit.\n");
                #endif

                goto mc_idct_thread_exit;
            }
        }

        #ifdef __dump_DSP_test_data__
        int mo_x,mo_y;
        short* pdct=p_pic->pdct;
        #ifdef __dump_binary__
        if(!p_pic->use_permutated)//!thiz->use_dsp)
        {
            //temp transpose dct for dsp
             for(mo_y=0;mo_y<p_pic->mb_height;mo_y++)
            {
                for(mo_x=0;mo_x<p_pic->mb_width;mo_x++,pdct+=64*6)       
                {
                    transpose_matrix(pdct);
                    transpose_matrix(pdct+64);
                    transpose_matrix(pdct+64*2);
                    transpose_matrix(pdct+64*3);
                    transpose_matrix(pdct+64*4);
                    transpose_matrix(pdct+64*5);
                }                
            }
        }
        #ifdef __dump_whole__
        if(p_pic->frame_cnt>=log_start_frame && p_pic->frame_cnt<=log_end_frame)
        {
            //static int cnt1=0;
            //av_log(NULL,AV_LOG_ERROR,"cnt1=%d.\n",cnt1++);
            log_dump_f(log_fd_residual,p_pic->pdct, p_pic->mb_width*p_pic->mb_height*768);    
        }
        //av_log(NULL,AV_LOG_ERROR,"residue p_pic->frame_cnt=%d,log_start_frame=%d,log_end_frame=%d.\n",p_pic->frame_cnt,log_start_frame,log_end_frame);
        #else
        //idct coef
        log_openfile_p(log_fd_residual, "mpeg4_coef", p_pic->frame_cnt);
        log_dump_p(log_fd_residual,p_pic->pdct, p_pic->mb_width*p_pic->mb_height*768,p_pic->frame_cnt);
        log_closefile_p(log_fd_residual,p_pic->frame_cnt);
        #endif
        
        pdct=p_pic->pdct;
        if(!p_pic->use_permutated)//!thiz->use_dsp)
        {
            //temp transpose back
             for(mo_y=0;mo_y<p_pic->mb_height;mo_y++)
            {
                for(mo_x=0;mo_x<p_pic->mb_width;mo_x++,pdct+=64*6)       
                {
                    transpose_matrix(pdct);
                    transpose_matrix(pdct+64);
                    transpose_matrix(pdct+64*2);
                    transpose_matrix(pdct+64*3);
                    transpose_matrix(pdct+64*4);
                    transpose_matrix(pdct+64*5);
                }                
            }
        }
        #endif
        #endif

        #ifdef __dump_DSP_test_data__
        //vop info
        //calculate dram address for vop_coef_daddr, vop_mv_daddr
        p_pic->vopinfo.vop_coef_daddr=VOP_COEF_DADDR+(p_pic->mb_width*p_pic->mb_height*64*2*6)*p_pic->frame_cnt;
        p_pic->vopinfo.vop_mv_daddr=VOP_MV_DADDR+(p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t))*p_pic->frame_cnt;
        //get before
        //p_pic->vopinfo.vop_time_increment_resolution=s->avctx->time_base.den;
        //p_pic->vopinfo.vop_PTS_high=p_pic->vopinfo.vop_PTS_low=0;
        p_pic->vopinfo.vop_reserved=0;
        p_pic->vopinfo.vop_chroma_format=1;//set for dsp 
        //valid value
        p_pic->vopinfo.vop_width=p_pic->width;
        p_pic->vopinfo.vop_height=p_pic->height;
        #ifdef __dsp_interpret_gmc__
            if(p_pic->current_picture_ptr->pict_type==FF_S_TYPE)
                p_pic->vopinfo.vop_coding_type=FF_P_TYPE-1;
            else
                p_pic->vopinfo.vop_coding_type=(int)p_pic->current_picture_ptr->pict_type-1;
        #else
            p_pic->vopinfo.vop_coding_type=(int)p_pic->current_picture_ptr->pict_type-1;
        #endif
        //p_pic->vopinfo.vop_coding_type=(int)p_pic->current_picture_ptr->pict_type-1;
        p_pic->vopinfo.vop_rounding_type=p_pic->no_rounding;
        p_pic->vopinfo.vop_interlaced=p_pic->current_picture_ptr->interlaced_frame;
        p_pic->vopinfo.vop_top_field_first=p_pic->current_picture_ptr->top_field_first;
        
        #ifdef __dump_binary__        
        #ifdef __dump_whole__
        if(p_pic->frame_cnt>=log_start_frame && p_pic->frame_cnt<=log_end_frame)
        {
            //static int cnt2=0;
            //av_log(NULL,AV_LOG_ERROR,"cnt2=%d.\n",cnt2++);
            log_dump_f(log_fd_picinfo,&p_pic->vopinfo, sizeof(udec_decode_t));
        }
        //av_log(NULL,AV_LOG_ERROR,"picinfo p_pic->frame_cnt=%d,log_start_frame=%d,log_end_frame=%d.\n",p_pic->frame_cnt,log_start_frame,log_end_frame);
        #else
        //pic info
        log_openfile_p(log_fd_picinfo, "mpeg4_picinfo", p_pic->frame_cnt);
        log_dump_p(log_fd_picinfo,&p_pic->vopinfo, sizeof(udec_decode_t),p_pic->frame_cnt);
        log_closefile_p(log_fd_picinfo,p_pic->frame_cnt);
        #endif
        #endif
        #endif

        #if 1
        if(log_mask[decoding_config_pre_interpret_gmc] && p_pic->current_picture_ptr->pict_type==FF_S_TYPE)
        {//change mv settings, 1mv to 4mv, interpret global mc, calculate chroma mvs
            mb_mv_P_t* pmvp=p_pic->pmvp;
            mb_mv_B_t* pmvb=p_pic->pmvb;
            int mo_x,mo_y;
            
            av_log(NULL,AV_LOG_ERROR,"get here decoding_config_pre_interpret_gmc.\n");

            void (*interpret_gmc)(h263_pic_data_t* p_pic, mb_mv_P_t* pmvp, int mbx, int mby);
            if(p_pic->num_sprite_warping_points==1)
                interpret_gmc=interpret_gmc1_pre;
            else if(p_pic->num_sprite_warping_points==2 ||p_pic->num_sprite_warping_points==3)
                interpret_gmc=interpret_gmc23_pre;
            else
            {
                av_log(NULL,AV_LOG_ERROR,"fatal error: p_pic->num_sprite_warping_points=%d, not support.\n",p_pic->num_sprite_warping_points);
                ambadec_assert_ffmpeg(0);
                interpret_gmc=NULL;
            }
            
            for(mo_y=0;mo_y<p_pic->mb_height;mo_y++)
            {
                for(mo_x=0;mo_x<p_pic->mb_width;mo_x++,pmvp++)       
                {
                    if(!pmvp->mb_setting.intra && pmvp->mb_setting.mv_type==mv_type_16x16_gmc)
                    {
                        interpret_gmc(p_pic,pmvp,mo_x,mo_y);                              
                    }
                }
            }
            p_pic->current_picture_ptr->pict_type=FF_P_TYPE;
        }
        #endif

        //sw sim
        if(!p_pic->use_dsp)
        {

            //need transpose back
            if(p_pic->use_permutated)
            {
                short* pdct=p_pic->pdct;
                int mo_y,mo_x;
                for(mo_y=0;mo_y<p_pic->mb_height;mo_y++)
                {
                    for(mo_x=0;mo_x<p_pic->mb_width;mo_x++,pdct+=64*6)       
                    {
                        transpose_matrix(pdct);
                        transpose_matrix(pdct+64);
                        transpose_matrix(pdct+64*2);
                        transpose_matrix(pdct+64*3);
                        transpose_matrix(pdct+64*4);
                        transpose_matrix(pdct+64*5);
                    }                
                }
            }
                    
            if(p_pic->current_picture_ptr->pict_type==FF_P_TYPE ||p_pic->current_picture_ptr->pict_type==FF_S_TYPE )
                h263_mpeg4_p_process_mc_idct_amba(thiz,p_pic);
            else if(p_pic->current_picture_ptr->pict_type==FF_B_TYPE)
                h263_mpeg4_b_process_mc_idct_amba(thiz,p_pic);
            else if(p_pic->current_picture_ptr->pict_type==FF_I_TYPE)
                h263_mpeg4_i_process_idct_amba(thiz,p_pic);
            else 
            {
                av_log(NULL,AV_LOG_ERROR,"not support yet pict_type=%d, in thread_rv12_mc_idct_addresidue .\n",p_pic->current_picture_ptr->pict_type);
            }
        }

        #ifdef __dump_DSP_test_data__        
        #ifdef __dump_binary__        
        //mv
        //log_openfile_p(log_fd_mvdsp, "mpeg4_mv_ori", p_pic->frame_cnt);
        //log_dump_p(log_fd_mvdsp,p_pic->pmvp, p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t),p_pic->frame_cnt);
        //log_closefile_p(log_fd_mvdsp,p_pic->frame_cnt);

        //change mv settings, 1mv to 4mv, interpret global mc, calculate chroma mvs
        mb_mv_P_t* pmvp=p_pic->pmvp;
        mb_mv_B_t* pmvb=p_pic->pmvb;
        int* pcp;
        int pcpv,pcpv1;
        
        //quanter sample DSP not support, we need not to do this filling
        ambadec_assert_ffmpeg(!p_pic->quarter_sample);
        if(!p_pic->quarter_sample)
        {
            if(p_pic->current_picture_ptr->pict_type==FF_P_TYPE )
            {                
                for(mo_y=0;mo_y<p_pic->mb_height;mo_y++)
                {
                    for(mo_x=0;mo_x<p_pic->mb_width;mo_x++,pmvp++)       
                    {
                        if(!pmvp->mb_setting.intra)
                        {
                            //filled 4mvs if 16x16 
                            if(!pmvp->mb_setting.mv_type)
                            {
                                //pmvp->mb_setting.mv_type=mv_type_8x8;

                                //calculate chroma mv
                                pcpv=pmvp->mv[0].mv_x;
                                pcpv1=pmvp->mv[0].mv_y;
                                pmvp->mv[4].mv_x=(pcpv>>1)|(pcpv & 1);                                
                                pmvp->mv[4].valid=1;
                                pmvp->mv[4].mv_y=(pcpv1>>1)|(pcpv1 & 1);

                                //temp scale mv by 2, 1/4 pixel
                                pmvp->mv[0].mv_x*=2;
                                pmvp->mv[0].mv_y*=2;
                                
                                pcp=&pmvp->mv[0];                          
                                pcpv=*pcp++;
                                *pcp++=pcpv;
                                *pcp++=pcpv;
                                *pcp=pcpv;

                                //temp scale mv by 2, 1/4 pixel
                                pmvp->mv[4].mv_x*=2;
                                pmvp->mv[4].mv_y*=2;
                                
                                pcp=&pmvp->mv[4];
                                pcpv=*pcp++;
                                *pcp++=pcpv;
                            }
                            else if(pmvp->mb_setting.mv_type==mv_type_8x8)
                            {
                                pcpv=pmvp->mv[0].mv_x+pmvp->mv[1].mv_x+pmvp->mv[2].mv_x+pmvp->mv[3].mv_x;
                                pcpv1=pmvp->mv[0].mv_y+pmvp->mv[1].mv_y+pmvp->mv[2].mv_y+pmvp->mv[3].mv_y;
                                //special rounding, from ffmpeg
                                pmvp->mv[4].valid=1;
                                pmvp->mv[4].mv_x=ff_h263_round_chroma_mv(pcpv);
                                pmvp->mv[4].mv_y=ff_h263_round_chroma_mv(pcpv1);
                                
                                //temp scale mv by 2, 1/4 pixel
                                pmvp->mv[0].mv_x*=2;
                                pmvp->mv[0].mv_y*=2;
                                pmvp->mv[1].mv_x*=2;
                                pmvp->mv[1].mv_y*=2;
                                pmvp->mv[2].mv_x*=2;
                                pmvp->mv[2].mv_y*=2;
                                pmvp->mv[3].mv_x*=2;
                                pmvp->mv[3].mv_y*=2;
                                
                                pmvp->mv[4].mv_x*=4;
                                pmvp->mv[4].mv_y*=4;
                                //pmvp->mv[5].mv_x*=4;
                                //pmvp->mv[5].mv_y*=4; 
                                pcp=&pmvp->mv[4];
                                pcpv=*pcp++;
                                *pcp++=pcpv;
                                
                            }
                            else if(pmvp->mb_setting.mv_type==mv_type_16x8)
                            {
                                //top field
                                pcpv=pmvp->mv[0].mv_x;
                                pcpv1=pmvp->mv[0].mv_y;
                                pmvp->mv[4].mv_x=(pcpv>>1)|(pcpv & 1);
                                pmvp->mv[4].valid=1;
                                pmvp->mv[4].mv_y=(pcpv1>>1)|(pcpv1 & 1);
                                //bottom field
                                pcpv=pmvp->mv[1].mv_x;
                                pcpv1=pmvp->mv[1].mv_y;
                                pmvp->mv[5].mv_x=(pcpv>>1)|(pcpv & 1);
                                pmvp->mv[5].valid=1;
                                pmvp->mv[5].mv_y=(pcpv1>>1)|(pcpv1 & 1);

                                //temp scale mv by 2, 1/4 pixel
                                pmvp->mv[0].mv_x*=2;
                                pmvp->mv[0].mv_y*=2;
                                pmvp->mv[1].mv_x*=2;
                                pmvp->mv[1].mv_y*=2;
                                
                                pmvp->mv[4].mv_x*=4;
                                pmvp->mv[4].mv_y*=4;
                                pmvp->mv[5].mv_x*=4;
                                pmvp->mv[5].mv_y*=4;                                 
                            }
                            else
                                ambadec_assert_ffmpeg(0);
                                
                        }
                    }
                }
            }
            else if(p_pic->current_picture_ptr->pict_type==FF_B_TYPE)
            {
                for(mo_y=0;mo_y<p_pic->mb_height;mo_y++)
                {
                    for(mo_x=0;mo_x<p_pic->mb_width;mo_x++,pmvb++)       
                    {
                        if(!pmvb->mb_setting.intra)
                        {
                            //filled 4mvs if 16x16 
                            if(!pmvb->mb_setting.mv_type)
                            {

                                //pmvb->mb_setting.mv_type=mv_type_8x8;

                                if(pmvb->mvp[0].fw.valid)
                                {
                                    //calculate chroma mv
                                    pcpv=pmvb->mvp[0].fw.mv_x;
                                    pcpv1=pmvb->mvp[0].fw.mv_y;
                                    pmvb->mvp[4].fw.mv_x=((pcpv>>1)|(pcpv & 1))*4;                                
                                    pmvb->mvp[4].fw.valid=1;
                                    pmvb->mvp[4].fw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;
                                    pmvb->mvp[0].fw.mv_x*=2;
                                    pmvb->mvp[0].fw.mv_y*=2;

                                    pcp=&pmvb->mvp[0].fw;
                                    pcpv=*pcp;
                                    pcp[2]=pcpv;
                                    pcp[4]=pcpv;
                                    pcp[6]=pcpv;

                                    pcp=&pmvb->mvp[4].fw;
                                    pcpv=*pcp;
                                    pcp[2]=pcpv;
                                }

                                if(pmvb->mvp[0].bw.valid)
                                {
                                    pcpv=pmvb->mvp[0].bw.mv_x;
                                    pcpv1=pmvb->mvp[0].bw.mv_y;
                                    pmvb->mvp[4].bw.mv_x=((pcpv>>1)|(pcpv & 1))*4;                                
                                    pmvb->mvp[4].bw.valid=1;
                                    pmvb->mvp[4].bw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;
                                    pmvb->mvp[0].bw.mv_x*=2;
                                    pmvb->mvp[0].bw.mv_y*=2;
                                    pmvb->mvp[4].bw.ref_frame_num=1;

                                    pcp=&pmvb->mvp[0].bw;
                                    pcpv=*pcp;
                                    pcp[2]=pcpv;
                                    pcp[4]=pcpv;
                                    pcp[6]=pcpv;

                                    pcp=&pmvb->mvp[4].bw;
                                    pcpv=*pcp;
                                    pcp[2]=pcpv;
                                }
                                
                            }
                            else if(pmvb->mb_setting.mv_type==mv_type_8x8)
                            {
                                if(pmvb->mvp[0].fw.valid)
                                {
                                    //fw
                                    ambadec_assert_ffmpeg(pmvb->mvp[1].fw.valid && pmvb->mvp[2].fw.valid && pmvb->mvp[3].fw.valid);
                                    pcpv=pmvb->mvp[0].fw.mv_x+pmvb->mvp[1].fw.mv_x+pmvb->mvp[2].fw.mv_x+pmvb->mvp[3].fw.mv_x;
                                    pcpv1=pmvb->mvp[0].fw.mv_y+pmvb->mvp[1].fw.mv_y+pmvb->mvp[2].fw.mv_y+pmvb->mvp[3].fw.mv_y;
                                    //special rounding, from ffmpeg
                                    pmvb->mvp[4].fw.valid=1;
                                    pmvb->mvp[4].fw.mv_x=ff_h263_round_chroma_mv(pcpv)*4;
                                    pmvb->mvp[4].fw.mv_y=ff_h263_round_chroma_mv(pcpv1)*4;
                                    
                                    pmvb->mvp[0].fw.mv_x*=2;
                                    pmvb->mvp[0].fw.mv_y*=2;
                                    pmvb->mvp[1].fw.mv_x*=2;
                                    pmvb->mvp[1].fw.mv_y*=2;
                                    pmvb->mvp[2].fw.mv_x*=2;
                                    pmvb->mvp[2].fw.mv_y*=2;
                                    pmvb->mvp[3].fw.mv_x*=2;
                                    pmvb->mvp[3].fw.mv_y*=2;
                                    
                                    pcp=&pmvb->mvp[4].fw;
                                    pcpv=*pcp;
                                    pcp[2]=pcpv;
                                }

                                if(pmvb->mvp[0].bw.valid)
                                {
                                    //bw
                                    ambadec_assert_ffmpeg(pmvb->mvp[1].bw.valid && pmvb->mvp[2].bw.valid && pmvb->mvp[3].bw.valid);
                                    ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num && pmvb->mvp[1].bw.ref_frame_num && pmvb->mvp[2].bw.ref_frame_num && pmvb->mvp[3].bw.ref_frame_num);
                                    pcpv=pmvb->mvp[0].bw.mv_x+pmvb->mvp[1].bw.mv_x+pmvb->mvp[2].bw.mv_x+pmvb->mvp[3].bw.mv_x;
                                    pcpv1=pmvb->mvp[0].bw.mv_y+pmvb->mvp[1].bw.mv_y+pmvb->mvp[2].bw.mv_y+pmvb->mvp[3].bw.mv_y;
                                    //special rounding, from ffmpeg
                                    pmvb->mvp[4].bw.valid=1;
                                    pmvb->mvp[4].bw.mv_x=ff_h263_round_chroma_mv(pcpv)*4;
                                    pmvb->mvp[4].bw.mv_y=ff_h263_round_chroma_mv(pcpv1)*4;
                                    pmvb->mvp[4].bw.ref_frame_num=1;

                                    pmvb->mvp[0].bw.mv_x*=2;
                                    pmvb->mvp[0].bw.mv_y*=2;
                                    pmvb->mvp[1].bw.mv_x*=2;
                                    pmvb->mvp[1].bw.mv_y*=2;
                                    pmvb->mvp[2].bw.mv_x*=2;
                                    pmvb->mvp[2].bw.mv_y*=2;
                                    pmvb->mvp[3].bw.mv_x*=2;
                                    pmvb->mvp[3].bw.mv_y*=2;
                                    
                                    pcp=&pmvb->mvp[4].bw;
                                    pcpv=*pcp;
                                    pcp[2]=pcpv;
                                }

                            }
                            else if(pmvb->mb_setting.mv_type==mv_type_16x8)
                            {
                                if(pmvb->mvp[0].fw.valid)
                                {
                                    //top field fw
                                    pcpv=pmvb->mvp[0].fw.mv_x;
                                    pcpv1=pmvb->mvp[0].fw.mv_y;
                                    pmvb->mvp[4].fw.mv_x=((pcpv>>1)|(pcpv & 1))*4;                                
                                    pmvb->mvp[4].fw.valid=1;
                                    pmvb->mvp[4].fw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;
                                    pmvb->mvp[4].fw.bottom=pmvb->mvp[0].fw.bottom;

                                    pmvb->mvp[0].fw.mv_x*=2;
                                    pmvb->mvp[0].fw.mv_y*=2;
                                }
                                
                                if(pmvb->mvp[0].bw.valid)
                                {
                                    ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num);
                                    //top field bw
                                    pcpv=pmvb->mvp[0].bw.mv_x;
                                    pcpv1=pmvb->mvp[0].bw.mv_y;
                                    pmvb->mvp[4].bw.mv_x=((pcpv>>1)|(pcpv & 1))*4;                                
                                    pmvb->mvp[4].bw.valid=1;
                                    pmvb->mvp[4].bw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;  
                                    pmvb->mvp[4].bw.bottom=pmvb->mvp[0].bw.bottom;
                                    pmvb->mvp[4].bw.ref_frame_num=1;

                                    pmvb->mvp[0].bw.mv_x*=2;
                                    pmvb->mvp[0].bw.mv_y*=2;
                                }

                                if(pmvb->mvp[1].fw.valid)
                                {
                                    //bottom field fw
                                    pcpv=pmvb->mvp[1].fw.mv_x;
                                    pcpv1=pmvb->mvp[1].fw.mv_y;
                                    pmvb->mvp[5].fw.mv_x=((pcpv>>1)|(pcpv & 1))*4;                                
                                    pmvb->mvp[5].fw.valid=1;
                                    pmvb->mvp[5].fw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;
                                    pmvb->mvp[5].fw.bottom=pmvb->mvp[1].fw.bottom;

                                    pmvb->mvp[1].fw.mv_x*=2;
                                    pmvb->mvp[1].fw.mv_y*=2;
                                }

                                if(pmvb->mvp[1].bw.valid)
                                {
                                    ambadec_assert_ffmpeg(pmvb->mvp[1].bw.ref_frame_num);
                                    //bottom field bw
                                    pcpv=pmvb->mvp[1].bw.mv_x;
                                    pcpv1=pmvb->mvp[1].bw.mv_y;
                                    pmvb->mvp[5].bw.mv_x=((pcpv>>1)|(pcpv & 1))*4;                                
                                    pmvb->mvp[5].bw.valid=1;
                                    pmvb->mvp[5].bw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;
                                    pmvb->mvp[5].bw.bottom=pmvb->mvp[1].bw.bottom;
                                    pmvb->mvp[5].bw.ref_frame_num=1;

                                    pmvb->mvp[1].bw.mv_x*=2;
                                    pmvb->mvp[1].bw.mv_y*=2;
                                }

                            }
                            else
                                ambadec_assert_ffmpeg(0);
                        }
                    }
                }
            }
            else if(p_pic->current_picture_ptr->pict_type==FF_S_TYPE )
            {          
                void (*interpret_gmc)(h263_pic_data_t* p_pic, mb_mv_P_t* pmvp, int mbx, int mby);
                if(p_pic->num_sprite_warping_points==1)
                    interpret_gmc=interpret_gmc1;
                else if(p_pic->num_sprite_warping_points==2 ||p_pic->num_sprite_warping_points==3)
                    interpret_gmc=interpret_gmc23;
                else
                {
                    av_log(NULL,AV_LOG_ERROR,"fatal error: p_pic->num_sprite_warping_points=%d, not support.\n",p_pic->num_sprite_warping_points);
                    ambadec_assert_ffmpeg(0);
                    interpret_gmc=NULL;
                }
                
                for(mo_y=0;mo_y<p_pic->mb_height;mo_y++)
                {
                    for(mo_x=0;mo_x<p_pic->mb_width;mo_x++,pmvp++)       
                    {
                        if(!pmvp->mb_setting.intra)
                        {
                            //filled 4mvs if 16x16 
                            if(!pmvp->mb_setting.mv_type)
                            {
                                //pmvp->mb_setting.mv_type=mv_type_8x8;

                                //calculate chroma mv
                                pcpv=pmvp->mv[0].mv_x;
                                pcpv1=pmvp->mv[0].mv_y;
                                pmvp->mv[4].mv_x=(pcpv>>1)|(pcpv & 1);                                
                                pmvp->mv[4].valid=1;
                                pmvp->mv[4].mv_y=(pcpv1>>1)|(pcpv1 & 1);

                                //temp scale mv by 2, 1/4 pixel
                                pmvp->mv[0].mv_x*=2;
                                pmvp->mv[0].mv_y*=2;
                                
                                pcp=&pmvp->mv[0];                          
                                pcpv=*pcp++;
                                *pcp++=pcpv;
                                *pcp++=pcpv;
                                *pcp=pcpv;

                                //temp scale mv by 2, 1/4 pixel
                                pmvp->mv[4].mv_x*=2;
                                pmvp->mv[4].mv_y*=2;
                                
                                pcp=&pmvp->mv[4];
                                pcpv=*pcp++;
                                *pcp++=pcpv;
                            }
                            else if(pmvp->mb_setting.mv_type==mv_type_8x8)
                            {
                                pcpv=pmvp->mv[0].mv_x+pmvp->mv[1].mv_x+pmvp->mv[2].mv_x+pmvp->mv[3].mv_x;
                                pcpv1=pmvp->mv[0].mv_y+pmvp->mv[1].mv_y+pmvp->mv[2].mv_y+pmvp->mv[3].mv_y;
                                //special rounding, from ffmpeg
                                pmvp->mv[4].valid=1;
                                pmvp->mv[4].mv_x=ff_h263_round_chroma_mv(pcpv);
                                pmvp->mv[4].mv_y=ff_h263_round_chroma_mv(pcpv1);
                                
                                //temp scale mv by 2, 1/4 pixel
                                pmvp->mv[0].mv_x*=2;
                                pmvp->mv[0].mv_y*=2;
                                pmvp->mv[1].mv_x*=2;
                                pmvp->mv[1].mv_y*=2;
                                pmvp->mv[2].mv_x*=2;
                                pmvp->mv[2].mv_y*=2;
                                pmvp->mv[3].mv_x*=2;
                                pmvp->mv[3].mv_y*=2;
                                
                                pmvp->mv[4].mv_x*=4;
                                pmvp->mv[4].mv_y*=4;
                                //pmvp->mv[5].mv_x*=4;
                                //pmvp->mv[5].mv_y*=4; 
                                pcp=&pmvp->mv[4];
                                pcpv=*pcp++;
                                *pcp++=pcpv;
                                
                            }
                            else if(pmvp->mb_setting.mv_type==mv_type_16x8)
                            {
                                //top field
                                pcpv=pmvp->mv[0].mv_x;
                                pcpv1=pmvp->mv[0].mv_y;
                                pmvp->mv[4].mv_x=(pcpv>>1)|(pcpv & 1);
                                pmvp->mv[4].valid=1;
                                pmvp->mv[4].mv_y=(pcpv1>>1)|(pcpv1 & 1);
                                //bottom field
                                pcpv=pmvp->mv[1].mv_x;
                                pcpv1=pmvp->mv[1].mv_y;
                                pmvp->mv[5].mv_x=(pcpv>>1)|(pcpv & 1);
                                pmvp->mv[5].valid=1;
                                pmvp->mv[5].mv_y=(pcpv1>>1)|(pcpv1 & 1);

                                //temp scale mv by 2, 1/4 pixel
                                pmvp->mv[0].mv_x*=2;
                                pmvp->mv[0].mv_y*=2;
                                pmvp->mv[1].mv_x*=2;
                                pmvp->mv[1].mv_y*=2;
                                
                                pmvp->mv[4].mv_x*=4;
                                pmvp->mv[4].mv_y*=4;
                                pmvp->mv[5].mv_x*=4;
                                pmvp->mv[5].mv_y*=4;                                 
                            }
                            else if(pmvp->mb_setting.mv_type==mv_type_16x16_gmc)
                            {
                                interpret_gmc(p_pic,pmvp,mo_x,mo_y);
                            }
                            else
                                ambadec_assert_ffmpeg(0);
                                
                        }
                    }
                }
            }
        }

        #ifdef __dump_whole__
        if(p_pic->frame_cnt>=log_start_frame && p_pic->frame_cnt<=log_end_frame)
        {
            //static int cnt3=0;
            //av_log(NULL,AV_LOG_ERROR,"cnt3=%d.\n",cnt3++);
            log_dump_f(log_fd_mvdsp,p_pic->pmvp, p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t));
        }
        //av_log(NULL,AV_LOG_ERROR,"mvdsp p_pic->frame_cnt=%d,log_start_frame=%d,log_end_frame=%d.\n",p_pic->frame_cnt,log_start_frame,log_end_frame);
        #else
        log_openfile_p(log_fd_mvdsp, "mpeg4_mv", p_pic->frame_cnt);
        log_dump_p(log_fd_mvdsp,p_pic->pmvp, p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t),p_pic->frame_cnt);        
        log_closefile_p(log_fd_mvdsp,p_pic->frame_cnt);
        #endif
        #endif
        #endif


                   
        //trigger next
        
        if(!s->loop_filter)
        {

            if(s->unrestricted_mv && p_pic->current_picture_ptr->pict_type!=FF_B_TYPE
               && !s->intra_only
               && !(s->flags&CODEC_FLAG_EMU_EDGE)) {
                //av_log(NULL,AV_LOG_ERROR,"here!!!!here!! reference,s->intra_only=%d,s->flags=%x,s->unrestricted_mv=%d,s->current_picture_ptr->reference=%d\n.\n",s->intra_only,s->flags,s->unrestricted_mv,p_pic->current_picture_ptr->reference);
                    int edges = EDGE_BOTTOM | EDGE_TOP;
                
                    s->dsp.draw_edges(p_pic->current_picture_ptr->data[0], p_pic->current_picture_ptr->linesize[0]  , p_pic->h_edge_pos   , p_pic->v_edge_pos   , EDGE_WIDTH  , edges);
                    s->dsp.draw_edges_nv12(p_pic->current_picture_ptr->data[1], p_pic->current_picture_ptr->linesize[1] , p_pic->h_edge_pos>>1, p_pic->v_edge_pos>>1);
        //            s->dsp.draw_edges(s->current_picture.data[2], s->uvlinesize, s->h_edge_pos>>1, s->v_edge_pos>>1, EDGE_WIDTH/2 , edges);
            }
            emms_c();

            #ifdef __log_dump_data__
            AVFrame* pdecpic=(AVFrame*)p_pic->current_picture_ptr;
            if(pdecpic->data[0])
            {
                //av_log(NULL,AV_LOG_WARNING,"s->flags&CODEC_FLAG_EMU_EDGE=%d,avctx->width=%d,avctx->height=%d, pict->linesize[0]=%d,pict->linesize[1]=%d.\n",s->flags&CODEC_FLAG_EMU_EDGE,avctx->width,avctx->height,pdecpic->linesize[0],pdecpic->linesize[1]);

                if(s->flags&CODEC_FLAG_EMU_EDGE || !dump_config_extedge)
                {
                    log_openfile_p(log_fd_frame_data,"frame_data_Y",p_pic->frame_cnt);
            //    av_log(NULL,AV_LOG_WARNING,"pict->data[0]=%p,pict->linesize[0]*avctx->height-16=%d.\n",pict->data[0],pict->linesize[0]*avctx->height-16);

                    //dump with extended edge
                    //log_dump(log_fd_frame_data,pdecpic->data[0]-16-pdecpic->linesize[0]*16,pdecpic->linesize[0]*(avctx->height+32));
                    //dump only picture data
                    int itt=0,jtt=0;uint8_t* ptt=pdecpic->data[0];
                    for(itt=0;itt<p_pic->height;itt++,ptt+=pdecpic->linesize[0])
                        log_dump_p(log_fd_frame_data,ptt,p_pic->width,p_pic->frame_cnt);
                
                    log_closefile_p(log_fd_frame_data,p_pic->frame_cnt);

                    int htmp=(p_pic->height+1)>>1;
                    int wtmp=(p_pic->width+1)>>1;
                    char* ptmpu=av_malloc(htmp*wtmp);
                    char* ptmpv=av_malloc(htmp*wtmp);

                    for(jtt=0;jtt<htmp;jtt++)
                    {
                        ptt=pdecpic->data[1]+jtt*pdecpic->linesize[1];
                        for(itt=0;itt<wtmp;itt++)
                        {
                            *ptmpu++= *ptt++;
                            *ptmpv++= *ptt++;
                        }
                    }
                    ptmpu-=htmp*wtmp;   
                    ptmpv-=htmp*wtmp;   
                    log_openfile_p(log_fd_frame_data,"frame_data_U",p_pic->frame_cnt);
                    log_dump_p(log_fd_frame_data,ptmpu,htmp*wtmp,p_pic->frame_cnt);
                    log_closefile_p(log_fd_frame_data,p_pic->frame_cnt);
                    log_openfile_p(log_fd_frame_data,"frame_data_V",p_pic->frame_cnt);
                    log_dump_p(log_fd_frame_data,ptmpv,htmp*wtmp,p_pic->frame_cnt);
                    log_closefile_p(log_fd_frame_data,p_pic->frame_cnt);
                    av_free(ptmpu);
                    av_free(ptmpv);
                }
                else
                { 
                    log_openfile_p(log_fd_frame_data,"frame_data_Y",p_pic->frame_cnt);
            //    av_log(NULL,AV_LOG_WARNING,"pict->data[0]=%p,pict->linesize[0]*avctx->height-16=%d.\n",pict->data[0],pict->linesize[0]*avctx->height-16);

                    //dump with extended edge
                    log_dump_p(log_fd_frame_data,pdecpic->data[0]-16-pdecpic->linesize[0]*16,pdecpic->linesize[0]*(p_pic->height+32),p_pic->frame_cnt);
                    log_closefile_p(log_fd_frame_data,p_pic->frame_cnt);

                    int htmp=((p_pic->height+1)>>1)+16;
                    int wtmp=(pdecpic->linesize[1]+1)>>1;
            //    av_log(NULL,AV_LOG_WARNING,"htmp=%d,wtmp=%d,pict->linesize[1]=%d.\n",htmp,wtmp,pict->linesize[1]);
                    char* ptmpu=av_malloc(htmp*wtmp);
                    char* ptmpv=av_malloc(htmp*wtmp);
            //    av_log(NULL,AV_LOG_WARNING,"ptmpu=%p,ptmpv=%p.\n",ptmpu,ptmpv);
                    int itmp,jtmp;
                    char* ptmp=pdecpic->data[1]-16-pdecpic->linesize[1]*8;
            //    av_log(NULL,AV_LOG_WARNING,"get uv.\n");
                    for(jtmp=0;jtmp<htmp;jtmp++)
                    {
                        for(itmp=0;itmp<wtmp;itmp++)
                        {
                            *ptmpu++= ptmp[0];
                            *ptmpv++= ptmp[1];
                            ptmp+=2;
                        }
                    }
            //    av_log(NULL,AV_LOG_WARNING,"end get uv.\n");
                    ptmpu-=htmp*wtmp;   
                    ptmpv-=htmp*wtmp;   
                    log_openfile_p(log_fd_frame_data,"frame_data_U",p_pic->frame_cnt);        
                    log_dump_p(log_fd_frame_data,ptmpu,htmp*wtmp,p_pic->frame_cnt);
                    log_closefile_p(log_fd_frame_data,p_pic->frame_cnt);
            //    av_log(NULL,AV_LOG_WARNING,"end dump uu, htmp=%d,wtmp=%d,ptmpu=%p.\n",htmp,wtmp,ptmpu);
                    log_openfile_p(log_fd_frame_data,"frame_data_V",p_pic->frame_cnt);
                    log_dump_p(log_fd_frame_data,ptmpv,htmp*wtmp,p_pic->frame_cnt);
                    log_closefile_p(log_fd_frame_data,p_pic->frame_cnt);
            //     av_log(NULL,AV_LOG_WARNING,"end dump v, htmp=%d,wtmp=%d. ret=%d. ptmpv=%p \n",htmp,wtmp,ret,ptmpv);       
                    av_free(ptmpu);
                    av_free(ptmpv);
            //    av_log(NULL,AV_LOG_WARNING,"end free.\n");   

                }
                
            }
            #endif

            #ifdef __dump_DSP_test_data__
            #ifdef __dump_binary__
            int dumpi=0,stride=0;
            int dumpwidth=0;
            char *psrc;   
            
            //dump decoded result
            #ifdef __dump_DSP_result__
            char logfilename[50];
            
                #ifdef __dump_DSP_result_with_display_order__
                    Picture* dump_pic;
                    static int dump_frame_cnt=0;
                    if(p_pic->current_picture_ptr->pict_type==FF_B_TYPE)
                        dump_pic=p_pic->current_picture_ptr;
                    else
                        dump_pic=p_pic->last_picture_ptr;
                    if(dump_pic)
                    {
                        if(log_start_frame<=dump_frame_cnt && log_end_frame>=dump_frame_cnt)
                        {
                            snprintf(logfilename,49,"ref_%d.y",dump_frame_cnt);
                            log_openfile_f(log_fd_result, logfilename);
                            psrc=dump_pic->data[0];
                            stride=dump_pic->linesize[0];
                            for(dumpi=0;dumpi<p_pic->height;dumpi++,psrc+=stride)
                            {
                                log_dump_f(log_fd_result,psrc, p_pic->width);
                            }
                            log_closefile(log_fd_result);

                            snprintf(logfilename,49,"ref_%d.uv",dump_frame_cnt);
                            log_openfile_f(log_fd_result, logfilename);
                            psrc=dump_pic->data[1];
                            stride=dump_pic->linesize[1];
                            for(dumpi=0;dumpi<(p_pic->height/2);dumpi++,psrc+=stride)
                            {
                                log_dump_f(log_fd_result,psrc, p_pic->width);
                            }
                            log_closefile(log_fd_result);
                        }
                        dump_frame_cnt++;
                    }
                #else            
                    if(p_pic->current_picture_ptr && log_start_frame<=p_pic->frame_cnt && log_end_frame>=p_pic->frame_cnt)
                    {
                        snprintf(logfilename,49,"ref_%d.y",p_pic->frame_cnt);
                        log_openfile_f(log_fd_result, logfilename);
                        psrc=p_pic->current_picture_ptr->data[0];
                        stride=p_pic->current_picture_ptr->linesize[0];
                        for(dumpi=0;dumpi<p_pic->height;dumpi++,psrc+=stride)
                        {
                            log_dump_f(log_fd_result,psrc, p_pic->width);
                        }
                        log_closefile(log_fd_result);

                        snprintf(logfilename,49,"ref_%d.uv",p_pic->frame_cnt);
                        log_openfile_f(log_fd_result, logfilename);
                        psrc=p_pic->current_picture_ptr->data[1];
                        stride=p_pic->current_picture_ptr->linesize[1];
                        for(dumpi=0;dumpi<(p_pic->height/2);dumpi++,psrc+=stride)
                        {
                            log_dump_f(log_fd_result,psrc, p_pic->width);
                        }
                        log_closefile(log_fd_result);

                    }            
                #endif
            
            #endif
            #ifdef __dump_whole__
            //dump decoded frames
            /*if(p_pic->current_picture_ptr)
            {
                psrc=p_pic->current_picture_ptr->data[0];
                stride=p_pic->current_picture_ptr->linesize[0];
                for(dumpi=0;dumpi<p_pic->height;dumpi++,psrc+=stride)
                {
                    log_dump(log_fd_result,psrc, p_pic->width);
                }

                psrc=p_pic->current_picture_ptr->data[1];
                stride=p_pic->current_picture_ptr->linesize[1];
                for(dumpi=0;dumpi<(p_pic->height/2);dumpi++,psrc+=stride)
                {
                    log_dump(log_fd_result_2,psrc, p_pic->width);
                }

            }*/
            #else
            //dump reference frames
            //forward reference
            if(p_pic->last_picture_ptr)
            {
                log_openfile_p(log_fd_reffyraw, "mpeg4_fwref_y", p_pic->frame_cnt);
                psrc=p_pic->last_picture_ptr->data[0];
                stride=p_pic->last_picture_ptr->linesize[0];
                for(dumpi=0;dumpi<p_pic->height;dumpi++,psrc+=stride)
                {
                    log_dump_p(log_fd_reffyraw,psrc, p_pic->width, p_pic->frame_cnt);
                }
                log_closefile_p(log_fd_reffyraw,p_pic->frame_cnt);

                log_openfile_p(log_fd_reffuvraw, "mpeg4_fwref_uv", p_pic->frame_cnt);
                psrc=p_pic->last_picture_ptr->data[1];
                stride=p_pic->last_picture_ptr->linesize[1];
                for(dumpi=0;dumpi<(p_pic->height/2);dumpi++,psrc+=stride)
                {
                    log_dump_p(log_fd_reffuvraw,psrc, p_pic->width, p_pic->frame_cnt);
                }
                log_closefile_p(log_fd_reffuvraw,p_pic->frame_cnt);

                //extended
                log_openfile_p(log_fd_reffyraw, "mpeg4_fwref_y_ext", p_pic->frame_cnt);
                psrc=p_pic->last_picture_ptr->data[0]-16-p_pic->last_picture_ptr->linesize[0]*16;
                log_dump_p(log_fd_reffyraw,psrc, p_pic->last_picture_ptr->linesize[0]*(p_pic->height+32), p_pic->frame_cnt);
                log_closefile_p(log_fd_reffyraw,p_pic->frame_cnt);

                log_openfile_p(log_fd_reffuvraw, "mpeg4_fwref_uv_ext", p_pic->frame_cnt);
                psrc=p_pic->last_picture_ptr->data[1]-16-p_pic->last_picture_ptr->linesize[1]*8;
                log_dump_p(log_fd_reffuvraw,psrc, p_pic->last_picture_ptr->linesize[1]*(p_pic->height/2+16), p_pic->frame_cnt);
                log_closefile_p(log_fd_reffuvraw,p_pic->frame_cnt);
            }
            
            if(p_pic->next_picture_ptr)
            {
                log_openfile_p(log_fd_refbyraw, "mpeg4_bwref_y", p_pic->frame_cnt);
                psrc=p_pic->next_picture_ptr->data[0];
                stride=p_pic->next_picture_ptr->linesize[0];
                for(dumpi=0;dumpi<p_pic->height;dumpi++,psrc+=stride)
                {
                    log_dump_p(log_fd_refbyraw,psrc, p_pic->width, p_pic->frame_cnt);
                }
                log_closefile_p(log_fd_refbyraw,p_pic->frame_cnt);

                log_openfile_p(log_fd_refbuvraw, "mpeg4_bwref_uv", p_pic->frame_cnt);
                psrc=p_pic->next_picture_ptr->data[1];
                stride=p_pic->next_picture_ptr->linesize[1];
                for(dumpi=0;dumpi<(p_pic->height/2);dumpi++,psrc+=stride)
                {
                    log_dump_p(log_fd_refbuvraw,psrc, p_pic->width, p_pic->frame_cnt);
                }
                log_closefile_p(log_fd_refbuvraw,p_pic->frame_cnt);

                //extended
                log_openfile_p(log_fd_refbyraw, "mpeg4_bwref_y_ext", p_pic->frame_cnt);
                psrc=p_pic->next_picture_ptr->data[0]-16-p_pic->next_picture_ptr->linesize[0]*16;
                log_dump_p(log_fd_refbyraw,psrc, p_pic->next_picture_ptr->linesize[0]*(p_pic->height+32), p_pic->frame_cnt);
                log_closefile_p(log_fd_refbyraw,p_pic->frame_cnt);

                log_openfile_p(log_fd_refbuvraw, "mpeg4_bwref_uv_ext", p_pic->frame_cnt);
                psrc=p_pic->next_picture_ptr->data[1]-16-p_pic->next_picture_ptr->linesize[1]*8;
                log_dump_p(log_fd_refbuvraw,psrc, p_pic->next_picture_ptr->linesize[1]*(p_pic->height/2+16), p_pic->frame_cnt);
                log_closefile_p(log_fd_refbuvraw,p_pic->frame_cnt);
            }

            
            //dump decoded frames

            if(p_pic->current_picture_ptr)
            {
                log_openfile_p(log_fd_result, "mpeg4_result_y", p_pic->frame_cnt);
                psrc=p_pic->current_picture_ptr->data[0];
                stride=p_pic->current_picture_ptr->linesize[0];
                for(dumpi=0;dumpi<p_pic->height;dumpi++,psrc+=stride)
                {
                    log_dump_p(log_fd_result,psrc, p_pic->width, p_pic->frame_cnt);
                }
                log_closefile_p(log_fd_result,p_pic->frame_cnt);

                log_openfile_p(log_fd_result, "mpeg4_result_uv", p_pic->frame_cnt);
                psrc=p_pic->current_picture_ptr->data[1];
                stride=p_pic->current_picture_ptr->linesize[1];
                for(dumpi=0;dumpi<(p_pic->height/2);dumpi++,psrc+=stride)
                {
                    log_dump_p(log_fd_result,psrc, p_pic->width, p_pic->frame_cnt);
                }
                log_closefile_p(log_fd_result,p_pic->frame_cnt);

            }
            #endif
            
            #endif
            #endif
    
            s->last_pict_type    = p_pic->current_picture_ptr->pict_type;
            if(p_pic->current_picture_ptr->pict_type!=FF_B_TYPE){
                s->last_non_b_pict_type= p_pic->current_picture_ptr->pict_type;
            }

            s->avctx->coded_frame= (AVFrame*)p_pic->current_picture_ptr;
                    
            //send output frame
            pthread_mutex_lock(&thiz->mutex);
            thiz->decoding_frame_cnt--;
            pthread_mutex_unlock(&thiz->mutex);
            
            if(p_pic->current_picture_ptr->pict_type==FF_B_TYPE)
            {
                thiz->pic_pool->inc_lock(thiz->pic_pool,p_pic->current_picture_ptr);
                p_node=thiz->p_frame_pool->get_free(thiz->p_frame_pool);
                p_node->p_ctx=p_pic->current_picture_ptr;
                #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"   mc_idct thread output: send picture=%p.\n",p_pic->current_picture_ptr);
                #endif
                thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
            }
            else
            {
                thiz->pic_pool->inc_lock(thiz->pic_pool,p_pic->last_picture_ptr);
                p_node=thiz->p_frame_pool->get_free(thiz->p_frame_pool);
                p_node->p_ctx=p_pic->last_picture_ptr;
                #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"   mc_idct thread output: send picture=%p.\n",p_pic->last_picture_ptr);
                #endif                
                thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);  
            }

            if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_pic->last_picture_ptr)==1)
                thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_pic->last_picture_ptr);
            if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_pic->current_picture_ptr)==1)
                thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_pic->current_picture_ptr);
            if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_pic->next_picture_ptr)==1)
                thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_pic->next_picture_ptr);
            
            #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR," mc_idct thread release : p_pic=%p start.\n",p_pic);
            #endif 
            
            #ifdef __dump_DSP_TXT__
            log_closefile_p(log_fd_dsp_text,p_pic->frame_cnt);
            #ifdef __dump_separate__
            log_closefile_p(log_fd_dsp_text+log_offset_mc,p_pic->frame_cnt);
            log_closefile_p(log_fd_dsp_text+log_offset_dct,p_pic->frame_cnt);
            #endif
            #endif
            
            thiz->p_pic_dataq->release_toready(thiz->p_pic_dataq,p_pic,0);   
            #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR," mc_idct thread release : p_pic=%p done.\n",p_pic);
            #endif 
        }
        else
        {
            p_node= thiz->p_deblock_dataq->get_free(thiz->p_deblock_dataq);
            p_node->p_ctx=p_pic;        
            thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,0);
        }
        
        //release data
        thiz->p_mc_idct_dataq->release(thiz->p_mc_idct_dataq,p_pic,0);        
    }  


mc_idct_thread_exit:
            
    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"thread_rv12_mc_idct_addresidue exit.\n");
    #endif   
    
    #ifdef __log_parallel_control__
        av_log(NULL,AV_LOG_ERROR,"thread_rv12_mc_idct_addresidue exit.\n");
    #endif  
    
    thiz->mc_idct_loop=0;
    pthread_exit(NULL);
    return NULL;
}

//to do
void* thread_h263_idct_addresidue(void* p)
{

    pthread_exit(NULL);
    return NULL;
}

//to do
void* thread_h263_mc(void* p)
{

    pthread_exit(NULL);
    return NULL;
}

//to do
void* thread_h263_deblock(void* p)
{
    H263AmbaDecContext_t *thiz=(H263AmbaDecContext_t *)p;
    MpegEncContext *s = &thiz->s;
    int ret;
    ctx_nodef_t* p_node;
    h263_pic_data_t* p_pic;
    
    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"deblock_thread start.\n");
    #endif
    
    while(thiz->deblock_loop)
    {
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Waiting: deblock_thread input: wait on p_deblock_dataq.\n");
        #endif
        
        //get data
        p_node=thiz->p_deblock_dataq->get(thiz->p_deblock_dataq);
        
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Escape: deblock_thread escape input: from p_deblock_dataq,p_node->p_ctx=%p.\n",p_node->p_ctx);
        #endif 
        
        p_pic=p_node->p_ctx;
        
        if(!p_pic)
        {
            if(p_node->flag==_flag_cmd_exit_next_)
            {
                #ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"deblock_thread get NULL data(_flag_cmd_exit_next_), start exit.\n");
                #endif
                #ifdef __log_parallel_control__
                    av_log(NULL,AV_LOG_ERROR,"**** deblock_thread, get exit cmd and start exit.\n");
                #endif
                goto deblock_thread_exit;
            }
            else
            {
                av_log(NULL, AV_LOG_ERROR, "Error data indicater in thread_wmv2_deblock.\n");
                goto deblock_thread_exit;
            }
        }
                       
        #ifdef __log_decoding_config__
            if(log_mask[decoding_config_deblock])
        #endif  
        {
            ff_h263_loop_filter_amba(s, p_pic->current_picture_ptr, p_pic->pswinfo, p_pic->mb_width, p_pic->mb_height);
        }

        //if decoding done

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"decoding picture done, thiz->pic_pool->used_cnt=%d,thiz->pic_pool->free_cnt=%d .\n",thiz->pic_pool->used_cnt,thiz->pic_pool->free_cnt);
        #endif  

        #ifdef __dump_DSP_test_data__
        log_dump_p(log_fd_residual,p_pic->pdct,p_pic->mb_height*p_pic->mb_width*768,p_pic->frame_cnt);
        log_dump_p(log_fd_mvdsp,p_pic->pinfo,p_pic->mb_height*p_pic->mb_width*sizeof(mb_mv_P_t),p_pic->frame_cnt);
        #endif
        
        pthread_mutex_lock(&thiz->mutex);
        thiz->decoding_frame_cnt--;
        pthread_mutex_unlock(&thiz->mutex);
        
        {
            thiz->pic_pool->inc_lock(thiz->pic_pool,p_pic->last_picture_ptr);
            p_node=thiz->p_frame_pool->get_free(thiz->p_frame_pool);
            p_node->p_ctx=p_pic->last_picture_ptr;
            #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"   deblock thread output: send picture=%p.\n",p_pic->last_picture_ptr);
            #endif                
            thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);  
        }
                   
        if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_pic->last_picture_ptr)==1)
            thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_pic->last_picture_ptr);
        if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_pic->current_picture_ptr)==1)
            thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_pic->current_picture_ptr);
        if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_pic->next_picture_ptr)==1)
            thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_pic->next_picture_ptr);


        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"   deblock thread release p_pic=%p.\n",p_pic);
        #endif
                
        thiz->p_pic_dataq->release_toready(thiz->p_pic_dataq,p_pic,0);        
        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"!!!!frame done: after release p_pic, thiz->pic_pool->used_cnt=%d,thiz->pic_pool->free_cnt=%d .\n",thiz->pic_pool->used_cnt,thiz->pic_pool->free_cnt);
        #endif 
    
        thiz->p_deblock_dataq->release(thiz->p_deblock_dataq,p_pic,0);

    }

deblock_thread_exit:
    
    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"deblock_thread exit.\n");
    #endif   

    #ifdef __log_parallel_control__
    av_log(NULL,AV_LOG_ERROR,"deblock_thread exit end.\n");
    #endif  
    thiz->deblock_loop=0;
    pthread_exit(NULL);
    return NULL;
}

void* thread_h263_vld(void* p)
{
    H263AmbaDecContext_t *thiz=(H263AmbaDecContext_t *)p;
    MpegEncContext *s = &thiz->s;
    const uint8_t *buf ;
    int buf_size;
    int ret;
    AVFrame *pict;
    ctx_nodef_t* p_node;
    h263_vld_data_t* p_vld;
    int i=0;
    void* pv;

    while(thiz->vld_loop)
    {
        
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Waiting: vld_thread input: wait on p_vld_dataq.\n");
        #endif
        
        p_node=thiz->p_vld_dataq->get(thiz->p_vld_dataq);

        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Escape: vld_thread input: escape from p_vld_dataq,p_node->p_ctx=%p.\n",p_node->p_ctx);
        #endif
        
        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"**** vld_thread get p_node, start a loop, log_cur_frame=%d .\n",log_cur_frame);
        #endif
        
        ambadec_assert_ffmpeg(thiz->parallel_method==1);

        p_vld=p_node->p_ctx;
        if(!p_vld )
        {
            if(p_node->flag==_flag_cmd_exit_next_)
            {
                #ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"vld_thread get exit cmd(_flag_cmd_exit_next_), start exit.\n");
                #endif
                
                thiz->vld_loop=0;
                break;
            }
            else //unknown cmd, exit too
            {
                #ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"vld_thread get unknown cmd, start exit.\n");
                #endif
                
                thiz->vld_loop=0;                
            }
            break;
        }
        
//        av_log(NULL,AV_LOG_ERROR,"start decoding a frame.\n");
                
        buf = p_vld->pbuf;
        buf_size = p_vld->size;

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"  vld_thread get data, buf_size=%d .\n",p_vld->size);
        #endif
        
        #ifdef __log_decoding_config__
        if(log_cur_frame==log_config_start_frame)
            apply_decoding_config();
        #endif

        if(s->flags&CODEC_FLAG_TRUNCATED){
            int next;

            if(CONFIG_MPEG4_DECODER && s->codec_id==CODEC_ID_AMBA_P_MPEG4){
                next= ff_mpeg4_find_frame_end(&s->parse_context, buf, buf_size);
            }else if(CONFIG_H263_DECODER && s->codec_id==CODEC_ID_AMBA_P_H263){
                next= ff_h263_find_frame_end(&s->parse_context, buf, buf_size);
            }else{
                av_log(s->avctx, AV_LOG_ERROR, "this codec does not support truncated bitstreams\n");
                goto freedata;
            }

            if( ff_combine_frame(&s->parse_context, next, (const uint8_t **)&buf, &buf_size) < 0 )
                goto freedata;
        }


    retry:

#ifdef _dump_raw_data_    
        av_log(NULL, AV_LOG_ERROR, "s->flags=%x,s->divx_packed=%d,s->bitstream_buffer_size=%d,buf_size=%d\n",s->flags,s->divx_packed,s->bitstream_buffer_size,buf_size);
        av_log(NULL, AV_LOG_ERROR, "s->bitstream_buffer_size=%d,dump_cnt=%d\n",s->bitstream_buffer_size,dump_cnt);
        snprintf(dump_fn,39,dump_fn_t,dump_cnt++);
        dump_pf=fopen(dump_fn,"wb");
        if(s->bitstream_buffer_size && (s->divx_packed || buf_size<20)){ //divx 5.01+/xvid frame reorder
            av_log(NULL, AV_LOG_ERROR,"using s->bitstream_buffer.\n");
            if(dump_pf)
            {
                fwrite(s->bitstream_buffer,1,s->bitstream_buffer_size,dump_pf);
                fclose(dump_pf);
            }
            init_get_bits(&s->gb, s->bitstream_buffer, s->bitstream_buffer_size*8);
        }else
        {
            av_log(NULL, AV_LOG_ERROR,"using buf.\n");
            if(dump_pf)
            {
                fwrite(buf,1,buf_size,dump_pf);
                fclose(dump_pf);
            }
            init_get_bits(&s->gb, buf, buf_size*8);
        }
#else
        if(s->bitstream_buffer_size && (s->divx_packed || buf_size<20)){ //divx 5.01+/xvid frame reorder
            init_get_bits(&s->gb, s->bitstream_buffer, s->bitstream_buffer_size*8);
        }else
        {
            init_get_bits(&s->gb, buf, buf_size*8);
        }
#endif

        s->bitstream_buffer_size=0;

        if (!s->context_initialized) {
            if (MPV_common_init(s) < 0) //we need the idct permutaton for reading a custom matrix
            {
                av_log(NULL,AV_LOG_ERROR,"**** Error: MPV_common_init fail in thread_h263_vld, exit.\n");
                break;
            }

        }

        ambadec_assert_ffmpeg(s->context_initialized);
        
        //#ifdef __log_decoding_process__
        //    av_log(NULL,AV_LOG_ERROR,"  before ambadec_create_pool,s->mb_width=%d,s->mb_height=%d.\n",s->mb_width,s->mb_height);
        //#endif

        if(!thiz->pic_pool)
        {
            thiz->pic_pool=ambadec_create_pool();
            for(i=0;i<6;i++)
            {
                thiz->pic_pool->put(thiz->pic_pool,&s->picture[i]);
                ff_alloc_picture_amba(s, &s->picture[i]);
            }
        }

        /* We need to set current_picture_ptr before reading the header,
         * otherwise we cannot store anyting in there */
        ambadec_assert_ffmpeg(thiz->pic_finished);
        if(thiz->pic_finished || !s->current_picture_ptr)
        {
            s->current_picture_ptr=(Picture*)thiz->pic_pool->get(thiz->pic_pool,&ret);
//            s->current_picture_ptr=thiz->p_pic->current_picture_ptr=(Picture*)thiz->pic_pool->get(thiz->pic_pool,&ret);
            s->avctx->get_buffer(s->avctx,(AVFrame*)s->current_picture_ptr);
            thiz->pic_finished=0;
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"  vld_thread get s->current_picture_ptr=%p.\n",s->current_picture_ptr);
            #endif
        }
//        else
//        {
//            thiz->p_pic->current_picture_ptr=s->current_picture_ptr;            
//        }
//        thiz->p_pic->frame_cnt=log_cur_frame;
        
    //    av_log(NULL, AV_LOG_ERROR, "in 1 s->picture_number=%d,s->avctx->extradata_size=%d\n",s->picture_number,s->avctx->extradata_size);

        /* let's go :-) */
        if (CONFIG_WMV2_DECODER && s->msmpeg4_version==5) {
            ret= ff_wmv2_decode_picture_header(s);
        } else if (CONFIG_MSMPEG4_DECODER && s->msmpeg4_version) {
            ret = msmpeg4_decode_picture_header(s);
        } else if (s->h263_pred) {
            if(s->avctx->extradata_size && s->picture_number==0){
    //            av_log(NULL, AV_LOG_ERROR, "***** decoding extradata s->picture_number=%d,s->avctx->extradata_size=%d\n",s->picture_number,s->avctx->extradata_size);

                GetBitContext gb;

                init_get_bits(&gb, s->avctx->extradata, s->avctx->extradata_size*8);
                ret = ff_mpeg4_decode_picture_header_amba(s, &gb);
            }
            ret = ff_mpeg4_decode_picture_header_amba(s, &s->gb);
        } else if (s->codec_id == CODEC_ID_AMBA_P_H263I) {
            ret = ff_intel_h263_decode_picture_header(s);
        } else if (s->h263_flv) {
            ret = ff_flv_decode_picture_header(s);
        } else {
            ret = h263_decode_picture_header(s);
        }

        if(ret==FRAME_SKIPPED) goto freedata;

        /* skip if the header was thrashed */
        if (ret < 0){
            av_log(s->avctx, AV_LOG_ERROR, "header damaged, ret=%d\n",ret);
            goto freedata;
        }

        #ifdef __tmp_use_amba_permutation__
        //permutated matrix and whether or not use dsp will determined here
        if((thiz->use_dsp || log_mask[decoding_config_use_dsp_permutated]) && thiz->use_permutated!=1)
        {
            thiz->use_permutated=1;
            switch_permutated(s,1);
            av_log(NULL,AV_LOG_ERROR,"config: using dsp permutated matrix now.\n");
        }
        else if(thiz->use_permutated!=0 && ((!thiz->use_dsp) || log_mask[decoding_config_use_dsp_permutated]==0))
        {
            thiz->use_permutated=0;
            switch_permutated(s,0);
            av_log(NULL,AV_LOG_ERROR,"config: not using dsp permutated matrix now.\n");
        }
        #endif
        
        s->avctx->has_b_frames= !s->low_delay;

        if(s->xvid_build==0 && s->divx_version==0 && s->lavc_build==0){
            if(s->stream_codec_tag == AV_RL32("XVID") ||
               s->codec_tag == AV_RL32("XVID") || s->codec_tag == AV_RL32("XVIX") ||
               s->codec_tag == AV_RL32("RMP4"))
                s->xvid_build= -1;
#if 0
            if(s->codec_tag == AV_RL32("DIVX") && s->vo_type==0 && s->vol_control_parameters==1
               && s->padding_bug_score > 0 && s->low_delay) // XVID with modified fourcc
                s->xvid_build= -1;
#endif
        }

        if(s->xvid_build==0 && s->divx_version==0 && s->lavc_build==0){
            if(s->codec_tag == AV_RL32("DIVX") && s->vo_type==0 && s->vol_control_parameters==0)
                s->divx_version= 400; //divx 4
        }

        if(s->xvid_build && s->divx_version){
            s->divx_version=
            s->divx_build= 0;
        }

        if(s->workaround_bugs&FF_BUG_AUTODETECT){
            if(s->codec_tag == AV_RL32("XVIX"))
                s->workaround_bugs|= FF_BUG_XVID_ILACE;

            if(s->codec_tag == AV_RL32("UMP4")){
                s->workaround_bugs|= FF_BUG_UMP4;
            }

            if(s->divx_version>=500 && s->divx_build<1814){
                s->workaround_bugs|= FF_BUG_QPEL_CHROMA;
            }

            if(s->divx_version>502 && s->divx_build<1814){
                s->workaround_bugs|= FF_BUG_QPEL_CHROMA2;
            }

            if(s->xvid_build && s->xvid_build<=3)
                s->padding_bug_score= 256*256*256*64;

            if(s->xvid_build && s->xvid_build<=1)
                s->workaround_bugs|= FF_BUG_QPEL_CHROMA;

            if(s->xvid_build && s->xvid_build<=12)
                s->workaround_bugs|= FF_BUG_EDGE;

            if(s->xvid_build && s->xvid_build<=32)
                s->workaround_bugs|= FF_BUG_DC_CLIP;

#define SET_QPEL_FUNC(postfix1, postfix2) \
        s->dsp.put_ ## postfix1 = ff_put_ ## postfix2;\
        s->dsp.put_no_rnd_ ## postfix1 = ff_put_no_rnd_ ## postfix2;\
        s->dsp.avg_ ## postfix1 = ff_avg_ ## postfix2;

            if(s->lavc_build && s->lavc_build<4653)
                s->workaround_bugs|= FF_BUG_STD_QPEL;

            if(s->lavc_build && s->lavc_build<4655)
                s->workaround_bugs|= FF_BUG_DIRECT_BLOCKSIZE;

            if(s->lavc_build && s->lavc_build<4670){
                s->workaround_bugs|= FF_BUG_EDGE;
            }

            if(s->lavc_build && s->lavc_build<=4712)
                s->workaround_bugs|= FF_BUG_DC_CLIP;

            if(s->divx_version)
                s->workaround_bugs|= FF_BUG_DIRECT_BLOCKSIZE;
    //printf("padding_bug_score: %d\n", s->padding_bug_score);
            if(s->divx_version==501 && s->divx_build==20020416)
                s->padding_bug_score= 256*256*256*64;

            if(s->divx_version && s->divx_version<500){
                s->workaround_bugs|= FF_BUG_EDGE;
            }

            if(s->divx_version)
                s->workaround_bugs|= FF_BUG_HPEL_CHROMA;
#if 0
            if(s->divx_version==500)
                s->padding_bug_score= 256*256*256*64;

            /* very ugly XVID padding bug detection FIXME/XXX solve this differently
             * Let us hope this at least works.
             */
            if(   s->resync_marker==0 && s->data_partitioning==0 && s->divx_version==0
               && s->codec_id==CODEC_ID_MPEG4 && s->vo_type==0)
                s->workaround_bugs|= FF_BUG_NO_PADDING;

            if(s->lavc_build && s->lavc_build<4609) //FIXME not sure about the version num but a 4609 file seems ok
                s->workaround_bugs|= FF_BUG_NO_PADDING;
#endif
        }

        if(s->workaround_bugs& FF_BUG_STD_QPEL){
            SET_QPEL_FUNC(qpel_pixels_tab[0][ 5], qpel16_mc11_old_c)
            SET_QPEL_FUNC(qpel_pixels_tab[0][ 7], qpel16_mc31_old_c)
            SET_QPEL_FUNC(qpel_pixels_tab[0][ 9], qpel16_mc12_old_c)
            SET_QPEL_FUNC(qpel_pixels_tab[0][11], qpel16_mc32_old_c)
            SET_QPEL_FUNC(qpel_pixels_tab[0][13], qpel16_mc13_old_c)
            SET_QPEL_FUNC(qpel_pixels_tab[0][15], qpel16_mc33_old_c)

            SET_QPEL_FUNC(qpel_pixels_tab[1][ 5], qpel8_mc11_old_c)
            SET_QPEL_FUNC(qpel_pixels_tab[1][ 7], qpel8_mc31_old_c)
            SET_QPEL_FUNC(qpel_pixels_tab[1][ 9], qpel8_mc12_old_c)
            SET_QPEL_FUNC(qpel_pixels_tab[1][11], qpel8_mc32_old_c)
            SET_QPEL_FUNC(qpel_pixels_tab[1][13], qpel8_mc13_old_c)
            SET_QPEL_FUNC(qpel_pixels_tab[1][15], qpel8_mc33_old_c)
        }

/*        if(avctx->debug & FF_DEBUG_BUGS)
            av_log(s->avctx, AV_LOG_DEBUG, "bugs: %X lavc_build:%d xvid_build:%d divx_version:%d divx_build:%d %s\n",
                   s->workaround_bugs, s->lavc_build, s->xvid_build, s->divx_version, s->divx_build,
                   s->divx_packed ? "p" : "");*/

#if 0 // dump bits per frame / qp / complexity
    {
        static FILE *f=NULL;
        if(!f) f=fopen("rate_qp_cplx.txt", "w");
        fprintf(f, "%d %d %f\n", buf_size, s->qscale, buf_size*(double)s->qscale);
    }
#endif

#if HAVE_MMX
        if(s->codec_id == CODEC_ID_AMBA_P_MPEG4 && s->xvid_build && s->avctx->idct_algo == FF_IDCT_AUTO && (mm_flags & FF_MM_MMX)){
            s->avctx->idct_algo= FF_IDCT_XVIDMMX;
            s->avctx->coded_width= 0; // force reinit
    //        dsputil_init(&s->dsp, avctx);
            s->picture_number=0;
        }
#endif

            /* After H263 & mpeg4 header decode we have the height, width,*/
            /* and other parameters. So then we could init the picture   */
            /* FIXME: By the way H263 decoder is evolving it should have */
            /* an H263EncContext                                         */

        if (   s->width  != p_vld->coded_width
            || s->height != p_vld->coded_height) {
            av_log(NULL,AV_LOG_ERROR,"!!!!!attention: [pic size changed],changing picture size, before is %d, %d, after is %d, %d.\n",s->width,s->height,p_vld->coded_width,p_vld->coded_height);
            /* H.263 could change picture size any time */
            ParseContext pc= s->parse_context; //FIXME move these demuxng hack to avformat
            s->parse_context.buffer=0;
            
            MPV_common_end(s);
            s->parse_context= pc;
        }
        if (!s->context_initialized) {
            avcodec_set_dimensions(s->avctx, s->width, s->height);
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"*****  vld_thread s->context_initialized=%d, retry.\n");
            #endif
            goto retry;
        }
        
        #ifdef __log_special_case__
        if(s->h263_plus)
        {
            av_log(NULL,AV_LOG_ERROR,"!!!!!attention: [h263 plus] detected in mpeg4/h263 decoder, s->h263_plus=%d.\n",s->h263_plus);            
        }
        if(s->loop_filter)
        {
            av_log(NULL,AV_LOG_ERROR,"!!!!!attention: [in-loop-filter] detected in mpeg4/h263 decoder, would be h263+ format.\n");
        }
        #endif
        
        if((s->codec_id==CODEC_ID_AMBA_P_H263 || s->codec_id==CODEC_ID_H263P || s->codec_id == CODEC_ID_AMBA_P_H263I))
            s->gob_index = ff_h263_get_gob_height(s);

        if(thiz->need_restart_pipeline && thiz->pipe_line_started)
        {
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"*****  vld_thread exit previous pipe line.\n");
            #endif
            //parallel related , exit sub threads
            ambadec_assert_ffmpeg(thiz->parallel_method==1);
            if(thiz->parallel_method==1)
            {
                p_node=thiz->p_mc_idct_dataq->get_cmd(thiz->p_mc_idct_dataq);
                thiz->p_mc_idct_dataq->put_ready(thiz->p_mc_idct_dataq,p_node,_flag_cmd_exit_next_);
                if(s->loop_filter)
                {
                    p_node=thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                    thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,_flag_cmd_exit_next_);            
                }
                ret=pthread_join(thiz->tid_mc_idct,&pv);
                if(s->loop_filter)
                {
                    ret=pthread_join(thiz->tid_deblock,&pv);
                    ambadec_destroy_triqueue(thiz->p_deblock_dataq);  
                    thiz->p_deblock_dataq=NULL;
                }

                ambadec_destroy_triqueue(thiz->p_mc_idct_dataq);
                thiz->p_mc_idct_dataq=NULL;
            }
            else if(thiz->parallel_method==2)
            {
                p_node=thiz->p_idct_dataq->get_cmd(thiz->p_idct_dataq);
                thiz->p_idct_dataq->put_ready(thiz->p_idct_dataq,p_node,_flag_cmd_exit_next_);
                p_node=thiz->p_mc_dataq->get_cmd(thiz->p_mc_dataq);
                thiz->p_mc_dataq->put_ready(thiz->p_mc_dataq,p_node,_flag_cmd_exit_next_);

                if(s->loop_filter)
                {
                    p_node=thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                    thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,_flag_cmd_exit_next_);            
                }
                ret=pthread_join(thiz->tid_idct,&pv);
                ret=pthread_join(thiz->tid_mc,&pv);
                if(s->loop_filter)
                {
                    ret=pthread_join(thiz->tid_deblock,&pv);
                    ambadec_destroy_triqueue(thiz->p_deblock_dataq); 
                    thiz->p_deblock_dataq=NULL;
                }

                ambadec_destroy_triqueue(thiz->p_idct_dataq);  
                thiz->p_idct_dataq=NULL;
                ambadec_destroy_triqueue(thiz->p_mc_dataq);
                thiz->p_mc_dataq=NULL;
            }
            
            ambadec_reset_triqueue(thiz->p_frame_pool);
            
            ambadec_destroy_triqueue(thiz->p_pic_dataq);
            thiz->p_pic_dataq=NULL;
            p_node= thiz->pic_pool->used_head.p_next;
            while(p_node != &thiz->pic_pool->used_head)
            {
                thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_node->p_ctx);
                p_node=p_node->p_next;
            }
            ambadec_destroy_pool(thiz->pic_pool);
            for(i=0;i<6;i++)
            {
                free_picture_amba(&s->picture[i]);
            }

            thiz->need_restart_pipeline=0;
            thiz->pipe_line_started=0;
            thiz->pic_pool=NULL;

            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"*****  vld_thread exit previous pipe line done.\n");
            #endif
        }

        if(!thiz->pipe_line_started)
        {
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"*****  vld_thread start new pipe line.\n");
            #endif
            ambadec_assert_ffmpeg(!thiz->p_pic_dataq);

            thiz->p_pic_dataq=ambadec_create_triqueue(_h263_callback_destroy_pic_data);
           
            //resource alloc
            for(i=0;i<2;i++)
            {
                thiz->picdata[i].pbase=av_malloc(s->mb_width*s->mb_height*sizeof(mb_mv_B_t)+16);
                thiz->picdata[i].pmvb=(mb_mv_B_t*)(((unsigned long)(thiz->picdata[i].pbase)+15)&(~0xf));
                thiz->picdata[i].pmvp=(mb_mv_P_t*)thiz->picdata[i].pmvb;
                        
                thiz->picdata[i].pdctbase=av_malloc(sizeof(short)*s->mb_width*s->mb_height*(256+128)+32);
                thiz->picdata[i].pdct=(short*)(((unsigned long)(thiz->picdata[i].pdctbase)+31)&(~0x1f));

                thiz->picdata[i].pswinfo=av_malloc(sizeof(h263_pic_swinfo_t)*s->mb_width*s->mb_height);
                
                p_node=thiz->p_pic_dataq->get_free(thiz->p_pic_dataq);
                p_node->p_ctx=&thiz->picdata[i];
                thiz->p_pic_dataq->put_ready(thiz->p_pic_dataq,p_node,0);
            }

//            thiz->vld_sent_row=thiz->mc_sent_row=0;
            thiz->pic_finished=1;
            thiz->p_pic=NULL;    

            if(thiz->parallel_method==1)
            {
                ambadec_assert_ffmpeg(!thiz->p_mc_idct_dataq);
                thiz->p_mc_idct_dataq=ambadec_create_triqueue(_h263_callback_NULL);
                thiz->mc_idct_loop=1;
                pthread_create(&thiz->tid_mc_idct,NULL,thread_rv12_mc_idct_addresidue,thiz);    
                if(s->loop_filter)
                {
                    thiz->deblock_loop=1;
                    ambadec_assert_ffmpeg(!thiz->p_deblock_dataq);
                    thiz->p_deblock_dataq=ambadec_create_triqueue(_h263_callback_NULL);
                    pthread_create(&thiz->tid_deblock,NULL,thread_h263_deblock,thiz);
                }
            }
            else
            {
                ambadec_assert_ffmpeg(!thiz->p_mc_dataq);
                ambadec_assert_ffmpeg(!thiz->p_idct_dataq);
                
                thiz->p_mc_dataq=ambadec_create_triqueue(_h263_callback_NULL);
                thiz->p_idct_dataq=ambadec_create_triqueue(_h263_callback_NULL);
               
                thiz->mc_loop=1;
                pthread_create(&thiz->tid_mc,NULL,thread_h263_mc,thiz);  

                thiz->idct_loop=1;
                pthread_create(&thiz->tid_idct,NULL,thread_h263_idct_addresidue,thiz);    
                if(s->loop_filter)
                {
                    ambadec_assert_ffmpeg(!thiz->p_deblock_dataq);
                    thiz->p_deblock_dataq=ambadec_create_triqueue(_h263_callback_NULL);
                    thiz->deblock_loop=1;
                    pthread_create(&thiz->tid_deblock,NULL,thread_h263_deblock,thiz);
                }
            }    
            thiz->pipe_line_started=1;
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"*****  vld_thread start new pipe line done.\n");
            #endif
        }

        // for hurry_up==5
        s->current_picture_ptr->pict_type= s->pict_type;
        s->current_picture_ptr->key_frame= s->pict_type == FF_I_TYPE;

        /* skip B-frames if we don't have reference frames */
        if(s->last_picture_ptr==NULL && (s->pict_type==FF_B_TYPE || s->dropable)) goto freedata;
        /* skip b frames if we are in a hurry */
        if(s->avctx->hurry_up && s->pict_type==FF_B_TYPE) goto freedata;
        if(   (s->avctx->skip_frame >= AVDISCARD_NONREF && s->pict_type==FF_B_TYPE)
           || (s->avctx->skip_frame >= AVDISCARD_NONKEY && s->pict_type!=FF_I_TYPE)
           ||  s->avctx->skip_frame >= AVDISCARD_ALL)
            goto freedata;
        /* skip everything if we are in a hurry>=5 */
        if(p_vld->hurry_up>=5) goto freedata;

        if(s->next_p_frame_damaged){
            if(s->pict_type==FF_B_TYPE)
                goto freedata;
            else
                s->next_p_frame_damaged=0;
        }

        if((s->avctx->flags2 & CODEC_FLAG2_FAST) && s->pict_type==FF_B_TYPE){
            s->me.qpel_put= s->dsp.put_2tap_qpel_pixels_tab;
            s->me.qpel_avg= s->dsp.avg_2tap_qpel_pixels_tab;
        }else if((!s->no_rounding) || s->pict_type==FF_B_TYPE){
            s->me.qpel_put= s->dsp.put_qpel_pixels_tab;
            s->me.qpel_avg= s->dsp.avg_qpel_pixels_tab;
        }else{
            s->me.qpel_put= s->dsp.put_no_rnd_qpel_pixels_tab;
            s->me.qpel_avg= s->dsp.avg_qpel_pixels_tab;
        }
        
#ifdef __dump_DSP_TXT__
        log_openfile_text_p(log_fd_dsp_text,"mpeg4_text",log_cur_frame);
        if(s->pict_type == FF_P_TYPE)
            log_text_p(log_fd_dsp_text,"===================== P-VOP ====================\n",log_cur_frame);
        else if(s->pict_type == FF_B_TYPE)
            log_text_p(log_fd_dsp_text,"===================== B-VOP ====================\n",log_cur_frame);
        else if(s->pict_type == FF_I_TYPE) 
            log_text_p(log_fd_dsp_text,"===================== I-VOP ====================\n",log_cur_frame);
        else if(s->pict_type==FF_S_TYPE)
            log_text_p(log_fd_dsp_text,"===================== S-VOP ====================\n",log_cur_frame);
        else 
        {
            char txtt[30];
            snprintf(txtt,29,"unknown type=%d",s->pict_type);
            log_text_p(log_fd_dsp_text,txtt,log_cur_frame);
        }

        #ifdef __dump_separate__
            log_openfile_text_p(log_fd_dsp_text+log_offset_vld,"mpeg4_text_vld",log_cur_frame);
            if(s->pict_type == FF_P_TYPE)
                log_text_p(log_fd_dsp_text+log_offset_vld,"===================== P-VOP ====================\n",log_cur_frame);
            else if(s->pict_type == FF_B_TYPE)
                log_text_p(log_fd_dsp_text+log_offset_vld,"===================== B-VOP ====================\n",log_cur_frame);
            else if(s->pict_type == FF_I_TYPE) 
                log_text_p(log_fd_dsp_text+log_offset_vld,"===================== I-VOP ====================\n",log_cur_frame);
            else if(s->pict_type==FF_S_TYPE)
                log_text_p(log_fd_dsp_text+log_offset_vld,"===================== S-VOP ====================\n",log_cur_frame);
            else 
            {
                char txtt[30];
                snprintf(txtt,29,"unknown type=%d",s->pict_type);
                log_text_p(log_fd_dsp_text+log_offset_vld,txtt,log_cur_frame);
            }
            log_openfile_text_p(log_fd_dsp_text+log_offset_mc,"mpeg4_text_mc",log_cur_frame);
            log_openfile_text_p(log_fd_dsp_text+log_offset_dct,"mpeg4_text_dct",log_cur_frame);
        #endif
#endif

        if(!thiz->p_pic)
        {
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR," Waiting: thread_h263_vld get thiz->p_pic.\n");
            #endif
            p_node=thiz->p_pic_dataq->get(thiz->p_pic_dataq);
            thiz->p_pic=p_node->p_ctx; 
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR," Escape: thread_h263_vld get thiz->p_pic=%p.\n",thiz->p_pic);
            #endif
            //exit indicator?
            if(!thiz->p_pic)
            {
                av_log(NULL,AV_LOG_ERROR,"r->p_pic_dataq->get==NULL,exit.\n");
                break;
            }
            ambadec_assert_ffmpeg(p_node->p_ctx==(&thiz->picdata[0])||p_node->p_ctx==(&thiz->picdata[1]));
        }
        //av_log(NULL,AV_LOG_ERROR,"s->avctx->time_base.den=%d,s->avctx->time_base.num=%d.\n",s->avctx->time_base.den,s->avctx->time_base.num);
        //get pts related
        thiz->p_pic->vopinfo.vop_time_increment_resolution=s->avctx->time_base.den;
        int64_t log_pts=(90000/thiz->p_pic->vopinfo.vop_time_increment_resolution)*s->time;
        thiz->p_pic->vopinfo.vop_PTS_low=log_pts&0xffffffff;
        thiz->p_pic->vopinfo.vop_PTS_high=log_pts&0xffffffff00000000;

        //s->current_picture_ptr->pict_type=s->pict_type;
        thiz->p_pic->width=s->width;
        thiz->p_pic->height=s->height;
        thiz->p_pic->mb_width=s->mb_width;
        thiz->p_pic->mb_height=s->mb_height;
        thiz->p_pic->mb_stride=s->mb_stride;
        
        s->linesize=s->current_picture_ptr->linesize[0];
        s->uvlinesize=s->current_picture_ptr->linesize[1];
        
        _h263_reset_picture_data(thiz->p_pic);    
        
        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"!!!start decoding new picture: picture type=%d, r->p_pic=%p,[0]=%p,[1]=%p .\n",s->pict_type,thiz->p_pic,&thiz->picdata[0],&thiz->picdata[1]);
        #endif  
            
        if(s->pict_type!=FF_B_TYPE)
        {
            assert(s->key_frame ||s->last_picture_ptr);
            //reference 
            if(thiz->pic_pool->dec_lock(thiz->pic_pool,s->last_picture_ptr)==1)
                s->avctx->release_buffer(s->avctx,(AVFrame*)s->last_picture_ptr);
            s->last_picture_ptr=s->next_picture_ptr;
            s->next_picture_ptr=s->current_picture_ptr;
            thiz->pic_pool->inc_lock(thiz->pic_pool,s->next_picture_ptr);             
        }
        
        thiz->p_pic->last_picture_ptr=s->last_picture_ptr;
        thiz->p_pic->next_picture_ptr=s->next_picture_ptr;
        thiz->p_pic->current_picture_ptr=s->current_picture_ptr;
        thiz->pic_pool->inc_lock(thiz->pic_pool,s->last_picture_ptr);
        thiz->pic_pool->inc_lock(thiz->pic_pool,s->next_picture_ptr);
        thiz->pic_pool->inc_lock(thiz->pic_pool,s->current_picture_ptr);
        
        s->current_picture_ptr->top_field_first= s->top_field_first;
        s->current_picture_ptr->interlaced_frame= !s->progressive_frame && !s->progressive_sequence;


        s->current_picture_ptr->reference= 0;
        if (!s->dropable && s->pict_type != FF_B_TYPE)
        {
            s->current_picture_ptr->reference = 3;
        }


        ff_er_frame_start(s);

        //the second part of the wmv2 header contains the MB skip bits which are stored in current_picture->mb_type
        //which is not available before MPV_frame_start()
        //try remove wmv2  
        /*if (CONFIG_WMV2_DECODER && s->msmpeg4_version==5){
            ret = ff_wmv2_decode_secondary_picture_header(s);
            if(ret<0) goto freedata;
            if(ret==1) goto intrax8_decoded;
        }*/

        /* decode each macroblock */
        s->mb_x=0;
        s->mb_y=0;

#if 0          
//#ifdef __dump_temp__
        char tempc[80];
        snprintf(tempc,79,"s->msmpeg4_version=%d,s->h263_msmpeg4=%d",s->msmpeg4_version,s->h263_msmpeg4);
        log_text(log_fd_temp, tempc);   
#endif

        decode_slice_amba(s);
        while(s->mb_y<s->mb_height){
#if 0  
//#ifdef __dump_temp__
        snprintf(tempc,79,"in decoding slice,s->mb_y=%d",s->mb_y);
        log_text(log_fd_temp, tempc);   
#endif
            if(s->msmpeg4_version){
                if(s->slice_height==0 || s->mb_x!=0 || (s->mb_y%s->slice_height)!=0 || get_bits_count(&s->gb) > s->gb.size_in_bits)
                    break;
            }else{
                if(ff_h263_resync_amba(s)<0)
                    break;
            }

            if(s->msmpeg4_version<4 && s->h263_pred)
                ff_mpeg4_clean_buffers_nv12(s);

            decode_slice_amba(s);
        }

        if (s->h263_msmpeg4 && s->msmpeg4_version<4 && s->pict_type==FF_I_TYPE)
            if(!CONFIG_MSMPEG4_DECODER || msmpeg4_decode_ext_header(s, buf_size) < 0){
                s->error_status_table[s->mb_num-1]= AC_ERROR|DC_ERROR|MV_ERROR;
            }

        /* divx 5.01+ bistream reorder stuff */
        if(s->codec_id==CODEC_ID_AMBA_P_MPEG4 && s->bitstream_buffer_size==0 && s->divx_packed){
            int current_pos= get_bits_count(&s->gb)>>3;
            int startcode_found=0;

            if(buf_size - current_pos > 5){
                int i;
                for(i=current_pos; i<buf_size-3; i++){
                    if(buf[i]==0 && buf[i+1]==0 && buf[i+2]==1 && buf[i+3]==0xB6){
                        startcode_found=1;
                        break;
                    }
                }
            }
            if(s->gb.buffer == s->bitstream_buffer && buf_size>20){ //xvid style
                startcode_found=1;
                current_pos=0;
            }

            if(startcode_found){
                av_fast_malloc(
                    &s->bitstream_buffer,
                    &s->allocated_bitstream_buffer_size,
                    buf_size - current_pos + FF_INPUT_BUFFER_PADDING_SIZE);
                if (!s->bitstream_buffer)
                    goto freedata;
                memcpy(s->bitstream_buffer, buf + current_pos, buf_size - current_pos);
                s->bitstream_buffer_size= buf_size - current_pos;
            }
        }

    intrax8_decoded:
        ff_er_frame_end_nv12(s);

//    assert(s->current_picture.pict_type == s->current_picture_ptr->pict_type);
//    assert(s->current_picture.pict_type == s->pict_type);
#if 0
        if (s->pict_type == FF_B_TYPE || s->low_delay) {
            *pict= *(AVFrame*)s->current_picture_ptr;
        } else if (s->last_picture_ptr != NULL) {
            *pict= *(AVFrame*)s->last_picture_ptr;
        }

        if(s->last_picture_ptr || s->low_delay){
            ff_print_debug_info(s, pict);
        }
#endif
        thiz->pic_finished=1;

        //pass info
        thiz->p_pic->no_rounding=s->no_rounding;
        thiz->p_pic->f_code=s->f_code;
        thiz->p_pic->b_code=s->b_code;
        thiz->p_pic->progressive_sequence=s->progressive_sequence;
        thiz->p_pic->quarter_sample=s->quarter_sample;
        thiz->p_pic->mpeg_quant=s->mpeg_quant;
        thiz->p_pic->alternate_scan=s->alternate_scan;
        thiz->p_pic->h_edge_pos=s->h_edge_pos;
        thiz->p_pic->v_edge_pos=s->v_edge_pos;///< horizontal / vertical position of the right/bottom edge (pixel replication)

        //log
        //av_log(NULL,AV_LOG_ERROR,"s->pict_type=%d.\n",s->pict_type);
        //av_log(NULL,AV_LOG_ERROR,"thiz->p_pic->quarter_sample=%d.\n",thiz->p_pic->quarter_sample);
        //av_log(NULL,AV_LOG_ERROR,"thiz->p_pic->mpeg_quant=%d.\n",thiz->p_pic->mpeg_quant);
        //av_log(NULL,AV_LOG_ERROR,"thiz->p_pic->no_rounding=%d.\n",thiz->p_pic->no_rounding);   
        //av_log(NULL,AV_LOG_ERROR,"s->current_picture_ptr->interlaced_frame=%d.\n",s->current_picture_ptr->interlaced_frame);
        //av_log(NULL,AV_LOG_ERROR,"s->current_picture_ptr->top_field_first=%d.\n",s->current_picture_ptr->top_field_first);
        //av_log(NULL,AV_LOG_ERROR,"thiz->p_pic->progressive_sequence=%d.\n",thiz->p_pic->progressive_sequence);
        //av_log(NULL,AV_LOG_ERROR,"s->picture_structure=%d.\n",s->picture_structure);
        //pay attention
        ambadec_assert_ffmpeg(s->picture_structure==PICT_FRAME);
//        ambadec_assert_ffmpeg(thiz->p_pic->progressive_sequence);
//        ambadec_assert_ffmpeg(!s->current_picture_ptr->interlaced_frame);
//        ambadec_assert_ffmpeg(!thiz->p_pic->quarter_sample);
//        ambadec_assert_ffmpeg(s->pict_type != FF_S_TYPE);
        
        //sprite related
        if(s->pict_type == FF_S_TYPE)
        {
            thiz->p_pic->sprite_warping_accuracy=s->sprite_warping_accuracy;
            thiz->p_pic->vol_sprite_usage=s->vol_sprite_usage;
            thiz->p_pic->sprite_width=s->sprite_width;
            thiz->p_pic->sprite_height=s->sprite_height;
            thiz->p_pic->sprite_left=s->sprite_left;
            thiz->p_pic->sprite_top=s->sprite_top;
            thiz->p_pic->sprite_brightness_change=s->sprite_brightness_change;
            thiz->p_pic->num_sprite_warping_points=s->num_sprite_warping_points;
            thiz->p_pic->real_sprite_warping_points=s->real_sprite_warping_points;
            memcpy(thiz->p_pic->sprite_traj,s->sprite_traj,sizeof(s->sprite_traj));
            memcpy(thiz->p_pic->sprite_offset,s->sprite_offset,sizeof(s->sprite_offset));
            memcpy(thiz->p_pic->sprite_delta,s->sprite_delta,sizeof(s->sprite_delta));
            memcpy(thiz->p_pic->sprite_shift,s->sprite_shift,sizeof(s->sprite_shift));
        }
        thiz->p_pic->use_dsp=thiz->use_dsp;
        thiz->p_pic->use_permutated=thiz->use_permutated;

        #ifdef __dump_DSP_test_data__
        #ifdef __dump_DSP_TXT__
        #ifdef __dump_separate__
        log_closefile_p(log_fd_dsp_text+log_offset_vld,thiz->p_pic->frame_cnt);
        #endif
        #endif
        #endif

        thiz->p_pic->frame_cnt=log_cur_frame;
        //trigger next thread
        if(thiz->parallel_method==1)
        {
            p_node=thiz->p_mc_idct_dataq->get_free(thiz->p_mc_idct_dataq);
            p_node->p_ctx=thiz->p_pic;
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR," Putting p_pic to p_mc_idct_dataq, thiz->p_pic=%p.\n", thiz->p_pic);
            #endif
            thiz->p_pic=NULL;
            thiz->p_mc_idct_dataq->put_ready(thiz->p_mc_idct_dataq,p_node,0);
        }
        else if(thiz->parallel_method==2)
        {
            p_node=thiz->p_mc_dataq->get_cmd(thiz->p_mc_dataq);
            p_node->p_ctx=thiz->p_pic;
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR," Putting p_pic to p_mc_dataq, thiz->p_pic=%p.\n", thiz->p_pic);
            #endif
            thiz->p_pic=NULL;
            thiz->p_mc_dataq->put_ready(thiz->p_mc_dataq,p_node,0);

        }
        
        pthread_mutex_lock(&thiz->mutex);
        thiz->decoding_frame_cnt++;
        pthread_mutex_unlock(&thiz->mutex);
        
        log_cur_frame++;

        if(0)
        {
freedata:
            p_node=thiz->p_frame_pool->get_free(thiz->p_frame_pool);
            p_node->p_ctx=NULL;
            #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"****error, send NULL pic when vld decoding error 2.\n");
            #endif                
            thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
        }

        //free raw data and release p_vld
        if(p_vld->pbuf)
        {
            //av_log(NULL,AV_LOG_ERROR,"free p_vld->pbuf=%p.\n",p_vld->pbuf);
            av_free(p_vld->pbuf);
            p_vld->pbuf=NULL;
        }
        thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);


    }
    
    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR," vld_thread start exit.\n");
    #endif
    
    if(thiz->pipe_line_started)
    {
        if(thiz->parallel_method==1)
        {
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR," vld_thread start exit(thiz->parallel_method==1).\n");
            #endif
            p_node=thiz->p_mc_idct_dataq->get_cmd(thiz->p_mc_idct_dataq);
            thiz->p_mc_idct_dataq->put_ready(thiz->p_mc_idct_dataq,p_node,_flag_cmd_exit_next_);
            if(s->loop_filter)
            {
                p_node=thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,_flag_cmd_exit_next_);            
            }
            ret=pthread_join(thiz->tid_mc_idct,&pv);
            if(s->loop_filter)
            {
                ret=pthread_join(thiz->tid_deblock,&pv);
                ambadec_destroy_triqueue(thiz->p_deblock_dataq); 
            }
            ambadec_destroy_triqueue(thiz->p_mc_idct_dataq);
        }
        else if(thiz->parallel_method==2)
        {
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR," vld_thread start exit(thiz->parallel_method==2).\n");
            #endif

            p_node=thiz->p_mc_dataq->get_cmd(thiz->p_mc_dataq);
            thiz->p_mc_dataq->put_ready(thiz->p_mc_dataq,p_node,_flag_cmd_exit_next_);
            p_node=thiz->p_idct_dataq->get_cmd(thiz->p_idct_dataq);
            thiz->p_idct_dataq->put_ready(thiz->p_idct_dataq,p_node,_flag_cmd_exit_next_);
            if(s->loop_filter)
            {
                p_node=thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,_flag_cmd_exit_next_);            
            }
            ret=pthread_join(thiz->tid_mc,&pv);
            ret=pthread_join(thiz->tid_idct,&pv);
            if(s->loop_filter)
            {
                ret=pthread_join(thiz->tid_deblock,&pv);
                ambadec_destroy_triqueue(thiz->p_deblock_dataq); 
            }
            ambadec_destroy_triqueue(thiz->p_mc_dataq);
            ambadec_destroy_triqueue(thiz->p_idct_dataq);
        }
        ambadec_destroy_triqueue(thiz->p_pic_dataq);
    }

    ambadec_assert_ffmpeg(thiz->pic_pool);
    if(thiz->pic_pool)
    {
        p_node= thiz->pic_pool->used_head.p_next;
        while(p_node != &thiz->pic_pool->used_head)
        {
            thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_node->p_ctx);
            p_node=p_node->p_next;
        }
        ambadec_destroy_pool(thiz->pic_pool);
    }

    if(s->context_initialized)
    {
        for(i=0;i<6;i++)
        {
            free_picture_amba(&s->picture[i]);
        }
    }
    
    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR," vld_thread exit done.\n");
    #endif

    pthread_exit(NULL);
    return NULL;
}

int ff_h263_decode_frame_amba(AVCodecContext *avctx,void *data, int *data_size,AVPacket *avpkt)
{
    H263AmbaDecContext_t*thiz = avctx->priv_data;
    AVFrame *pict = data;
    AVFrame* pready;
    ctx_nodef_t* p_node;
    h263_vld_data_t* p_data;
//    int must_get_pic=0;

    thiz->s.flags= avctx->flags;
    thiz->s.flags2= avctx->flags2;  
    
    //send to vld sub thread 
    if (avpkt->size) 
    {
        if(p_node=thiz->p_vld_dataq->get_free(thiz->p_vld_dataq))
        {
            p_data=p_node->p_ctx;
        }
        else
        {
            p_node=av_malloc(sizeof(ctx_nodef_t));
            p_node->p_ctx=NULL;
        }
        
        if(!p_node->p_ctx)
        {
            p_node->p_ctx=av_malloc(sizeof(h263_vld_data_t));
        }
        p_data=p_node->p_ctx;

        p_data->pbuf=avpkt->data;
        p_data->size=avpkt->size;

        p_data->skip_frame=avctx->skip_frame;
        p_data->hurry_up=avctx->hurry_up;
        p_data->coded_width=avctx->coded_width;
        p_data->coded_height=avctx->coded_height;

        #if 0
        avpkt->data=NULL;//trick here: prevent AP release it
        avpkt->size=0;
        #else
        p_data->pbuf=av_malloc(avpkt->size);
        memcpy(p_data->pbuf,avpkt->data,avpkt->size);
        #endif
//        av_log(NULL,AV_LOG_ERROR,"get p_data->pbuf=%p.\n",p_data->pbuf);
        
        thiz->p_vld_dataq->put_ready(thiz->p_vld_dataq,p_node,0);

        //get frame
        if(thiz->p_frame_pool->ready_cnt || thiz->p_vld_dataq->ready_cnt>1)
        {
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"Waiting: decode frame wait on p_frame_pool, r->p_frame_pool->ready_cnt=%d,r->p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_vld_dataq->ready_cnt);
            #endif
            
            p_node=thiz->p_frame_pool->get(thiz->p_frame_pool);
            
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"Escape: decode frame escape from p_frame_pool, p_node->p_ctx=%p.\n",p_node->p_ctx);
            #endif
            
            pready=p_node->p_ctx;
            
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR," in decode_frame, get a decoded frame pready=%p,pict=%p.\n",pready,pict);
            #endif  
            
            if(!pready)
            {
                *data_size=0;
                return avpkt->size;
            }
            *pict=*pready;
            if(thiz->pic_pool->dec_lock(thiz->pic_pool,pready)==1)        
                avctx->release_buffer(avctx,pready);
            
            *data_size=sizeof(AVFrame);
            return avpkt->size;
        }
    }
    else
    {
        //special case for last frames
        if(thiz->p_frame_pool->ready_cnt || thiz->p_vld_dataq->ready_cnt || thiz->decoding_frame_cnt )
        {
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"Waiting: decode frame wait on p_frame_pool(last frames), thiz->p_frame_pool->ready_cnt=%d,thiz->p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_vld_dataq->ready_cnt);
            #endif
            
            p_node=thiz->p_frame_pool->get(thiz->p_frame_pool);
            
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"Escape: decode frame escape from p_frame_pool(last frames), p_node->p_ctx=%p.\n",p_node->p_ctx);
            #endif
            
            pready=p_node->p_ctx;
            
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR," in decode_frame, get a decoded frame(last frames), pready=%p,pict=%p.\n",pready,pict);
            #endif  
            
            if(!pready)
            {
                *data_size=0;
                return (thiz->p_frame_pool->ready_cnt+thiz->p_vld_dataq->ready_cnt+thiz->decoding_frame_cnt+1);
            }
            *pict=*pready;
            if(thiz->pic_pool->dec_lock(thiz->pic_pool,pready)==1)        
                avctx->release_buffer(avctx,pready);
            
            *data_size=sizeof(AVFrame);
            return (thiz->p_frame_pool->ready_cnt+thiz->p_vld_dataq->ready_cnt+thiz->decoding_frame_cnt+1);
        }
        else if(thiz->s.next_picture_ptr)// get next reference frame
        {
            *pict=*((AVFrame*)(thiz->s.next_picture_ptr));
            *data_size=sizeof(AVFrame);
            return 0;
        }
    }
    
    *data_size=0;
    
    return 0;
}

static int rv10_decode_frame(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    MpegEncContext *s = avctx->priv_data;
    int i;
    AVFrame *pict = data;
    int slice_count;
    const uint8_t *slices_hdr = NULL;

    av_dlog(avctx, "*****frame %d size=%d\n", avctx->frame_number, buf_size);

    /* no supplementary picture */
    if (buf_size == 0) {
        return 0;
    }

    if(!avctx->slice_count){
        slice_count = (*buf++) + 1;
        slices_hdr = buf + 4;
        buf += 8 * slice_count;
    }else
        slice_count = avctx->slice_count;

    for(i=0; i<slice_count; i++){
        int offset= get_slice_offset(avctx, slices_hdr, i);
        int size;

        if(i+1 == slice_count)
            size= buf_size - offset;
        else
            size= get_slice_offset(avctx, slices_hdr, i+1) - offset;

        rv10_decode_packet(avctx, buf+offset, size);
    }

    if(s->current_picture_ptr != NULL && s->mb_y>=s->mb_height){
        ff_er_frame_end(s);
        MPV_frame_end(s);

        if (s->pict_type == FF_B_TYPE || s->low_delay) {
            *pict= *(AVFrame*)s->current_picture_ptr;
        } else if (s->last_picture_ptr != NULL) {
            *pict= *(AVFrame*)s->last_picture_ptr;
        }

        if(s->last_picture_ptr || s->low_delay){
            *data_size = sizeof(AVFrame);
            ff_print_debug_info(s, pict);
        }
        s->current_picture_ptr= NULL; //so we can detect if frame_end wasnt called (find some nicer solution...)
    }

    return buf_size;
}

AVCodec rv10_amba_decoder = {
    "rv10-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_RV10,
    sizeof(MpegEncContext),
    rv10_decode_init_amba,
    NULL,
    rv10_decode_end_amba,
    rv10_decode_frame_amba,
    CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("RealVideo 1.0 (parallel nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec rv20_amba_decoder = {
    "rv20-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_RV20,
    sizeof(MpegEncContext),
    rv10_decode_init_amba,
    NULL,
    rv10_decode_end_amba,
    rv10_decode_frame_amba,
    CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    .flush= ff_mpeg_flush,
    .long_name = NULL_IF_CONFIG_SMALL("RealVideo 2.0 (parallel nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};
