/*
 * The simplest mpeg encoder (well, it was the simplest!)
 * Copyright (c) 2000,2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * 4MV & hq & B-frame encoding stuff by Michael Niedermayer <michaelni@gmx.at>
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
 * @file
 * The simplest mpeg encoder (well, it was the simplest!).
 */

#include "libavutil/intmath.h"
#include "libavutil/imgutils.h"
#include "avcodec.h"
#include "dsputil.h"
#include "internal.h"
#include "mpegvideo.h"
#include "mpegvideo_common.h"
#include "mjpegenc.h"
#include "msmpeg4.h"
#include "faandct.h"
#include "xvmc_internal.h"
#include "thread.h"
#include <limits.h>
#include "log_dump.h"
//#undef NDEBUG
//#include <assert.h>
#include "amba_dec_util.h"
#include "amba_h263.h"
#include "amba_wmv2.h"

void ff_simple_add_block(DCTELEM *block, uint8_t* des, int linesize);
void ff_simple_add_block_nv12(DCTELEM *block, uint8_t* des, int linesize);

static void dct_unquantize_mpeg1_intra_c(MpegEncContext *s,
                                   DCTELEM *block, int n, int qscale);
static void dct_unquantize_mpeg1_inter_c(MpegEncContext *s,
                                   DCTELEM *block, int n, int qscale);
static void dct_unquantize_mpeg2_intra_c(MpegEncContext *s,
                                   DCTELEM *block, int n, int qscale);
static void dct_unquantize_mpeg2_intra_bitexact(MpegEncContext *s,
                                   DCTELEM *block, int n, int qscale);
static void dct_unquantize_mpeg2_inter_c(MpegEncContext *s,
                                   DCTELEM *block, int n, int qscale);
static void dct_unquantize_h263_intra_c(MpegEncContext *s,
                                  DCTELEM *block, int n, int qscale);
static void dct_unquantize_h263_inter_c(MpegEncContext *s,
                                  DCTELEM *block, int n, int qscale);

static av_always_inline void mpeg_motion_lowres_nv12(MpegEncContext *s,
                               uint8_t *dest_y, uint8_t *dest_cb,
                               int field_based, int bottom_field, int field_select,
                               uint8_t **ref_picture, h264_chroma_mc_func *pix_op,h264_chroma_mc_func *pix_op_nv12,
                               int motion_x, int motion_y, int h);

static inline void chroma_4mv_motion_lowres_nv12(MpegEncContext *s,
                                     uint8_t *dest_cb,
                                     uint8_t **ref_picture,
                                     h264_chroma_mc_func *pix_op_nv12,
                                     int mx, int my);

static inline void MPV_motion_lowres_nv12(MpegEncContext *s,
                              uint8_t *dest_y, uint8_t *dest_cb,
                              int dir, uint8_t **ref_picture,
                              h264_chroma_mc_func *pix_op,h264_chroma_mc_func *pix_op_nv12);

static inline void put_dct_nv12(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int qscale);

/* put block[] to dest[] */
static inline void put_dct_amba(MpegEncContext *s,
                           DCTELEM *block,int i, uint8_t *dest, int line_size);

static inline void put_dct_nv12_amba(MpegEncContext *s,
                           DCTELEM *block,int i, uint8_t *dest, int line_size);

static inline void add_dct_nv12(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size);

static inline void add_dequant_dct_nv12(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int qscale);
static inline void add_dct_amba(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int have_idct);
static inline void add_dct_nv12_amba(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int have_idct);
static av_always_inline
void MPV_decode_mb_internal_nv12(MpegEncContext *s, DCTELEM block[12][64],
                            int is_mpeg12);
void MPV_decode_mb_internal_amba(MpegEncContext *s);

void MPV_decode_mb_nv12(MpegEncContext *s, DCTELEM block[12][64]);
static inline void gmc1_motion_amba (MpegEncContext *s, h263_pic_data_t* p_pic, uint8_t *dest_y, uint8_t *dest_cb,uint8_t **ref_picture,int mb_x, int mb_y);
static inline void gmc_motion_amba(MpegEncContext *s,h263_pic_data_t* p_pic,
                               uint8_t *dest_y, uint8_t *dest_cb,
                               uint8_t **ref_picture,int mb_x,int mb_y);
static inline void qpel_motion_amba(MpegEncContext *s,h263_pic_data_t* p_pic,
                               uint8_t *dest_y, uint8_t *dest_cb,
                               int field_based, int bottom_field, int field_select,
                               uint8_t **ref_picture, op_pixels_func (*pix_op)[4],op_pixels_func (*pix_op_nv12)[4],
                               qpel_mc_func (*qpix_op)[16],
                               int motion_x, int motion_y, int h, int mb_x,int mb_y);
static av_always_inline
void mpeg_motion_internal_amba(MpegEncContext *s,h263_pic_data_t* p_pic,
                 uint8_t *dest_y, uint8_t *dest_cb,
                 int field_based, int bottom_field, int field_select,
                 uint8_t **ref_picture, op_pixels_func (*pix_op)[4],op_pixels_func (*pix_op_nv12)[4],
                 int motion_x, int motion_y, int h, int mb_x,int mb_y);
static inline int hpel_motion_amba(MpegEncContext *s,h263_pic_data_t* p_pic,
                                  uint8_t *dest, uint8_t *src,
                                  int field_based, int field_select,
                                  int src_x, int src_y,
                                  int width, int height, int stride,
                                  int h_edge_pos, int v_edge_pos,
                                  int w, int h, op_pixels_func *pix_op,
                                  int motion_x, int motion_y);
static inline void chroma_4mv_motion_amba(MpegEncContext *s,h263_pic_data_t* p_pic,
                                     uint8_t *dest_cb,
                                     uint8_t **ref_picture,
                                     op_pixels_func *pix_op_nv12,
                                     int mx, int my, int mb_x, int mb_y);
void h263_mpeg4_i_process_idct_amba(H263AmbaDecContext_t *thiz, h263_pic_data_t* p_pic);
void h263_mpeg4_p_process_mc_idct_amba(H263AmbaDecContext_t *thiz, h263_pic_data_t* p_pic);
void h263_mpeg4_b_process_mc_idct_amba(H263AmbaDecContext_t *thiz, h263_pic_data_t* p_pic);
void wmv2_i_process_idct_amba(Wmv2Context_amba *thiz, wmv2_pic_data_t* p_pic);
void wmv2_p_process_mc_idct_amba(Wmv2Context_amba *thiz, wmv2_pic_data_t* p_pic);

const enum PixelFormat ff_pixfmt_list_nv12[] = {
    PIX_FMT_NV12,
    PIX_FMT_NONE
};

/* enable all paranoid tests for rounding, overflows, etc... */
//#define PARANOID

//#define DEBUG


static const uint8_t ff_default_chroma_qscale_table[32]={
//  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
};

const uint8_t ff_mpeg1_dc_scale_table[128]={
//  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

static const uint8_t mpeg2_dc_scale_table1[128]={
//  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
};

static const uint8_t mpeg2_dc_scale_table2[128]={
//  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
};

static const uint8_t mpeg2_dc_scale_table3[128]={
//  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

const uint8_t * const ff_mpeg2_dc_scale_table[4]={
    ff_mpeg1_dc_scale_table,
    mpeg2_dc_scale_table1,
    mpeg2_dc_scale_table2,
    mpeg2_dc_scale_table3,
};

const enum PixelFormat ff_pixfmt_list_420[] = {
    PIX_FMT_YUV420P,
    PIX_FMT_NONE
};

const enum PixelFormat ff_hwaccel_pixfmt_list_420[] = {
    PIX_FMT_DXVA2_VLD,
    PIX_FMT_VAAPI_VLD,
    PIX_FMT_YUV420P,
    PIX_FMT_NONE
};

const uint8_t *ff_find_start_code(const uint8_t * restrict p, const uint8_t *end, uint32_t * restrict state){
    int i;

    assert(p<=end);
    if(p>=end)
        return end;

    for(i=0; i<3; i++){
        uint32_t tmp= *state << 8;
        *state= tmp + *(p++);
        if(tmp == 0x100 || p==end)
            return p;
    }

    while(p<end){
        if     (p[-1] > 1      ) p+= 3;
        else if(p[-2]          ) p+= 2;
        else if(p[-3]|(p[-1]-1)) p++;
        else{
            p++;
            break;
        }
    }

    p= FFMIN(p, end)-4;
    *state= AV_RB32(p);

    return p+4;
}

/* init common dct for both encoder and decoder */
av_cold int ff_dct_common_init(MpegEncContext *s)
{
    s->dct_unquantize_h263_intra = dct_unquantize_h263_intra_c;
    s->dct_unquantize_h263_inter = dct_unquantize_h263_inter_c;
    s->dct_unquantize_mpeg1_intra = dct_unquantize_mpeg1_intra_c;
    s->dct_unquantize_mpeg1_inter = dct_unquantize_mpeg1_inter_c;
    s->dct_unquantize_mpeg2_intra = dct_unquantize_mpeg2_intra_c;
    if(s->flags & CODEC_FLAG_BITEXACT)
        s->dct_unquantize_mpeg2_intra = dct_unquantize_mpeg2_intra_bitexact;
    s->dct_unquantize_mpeg2_inter = dct_unquantize_mpeg2_inter_c;

#if   HAVE_MMX
    MPV_common_init_mmx(s);
#elif ARCH_ALPHA
    MPV_common_init_axp(s);
#elif CONFIG_MLIB
    MPV_common_init_mlib(s);
#elif HAVE_MMI
    MPV_common_init_mmi(s);
#elif ARCH_ARM
    MPV_common_init_arm(s);
#elif HAVE_ALTIVEC
    MPV_common_init_altivec(s);
#elif ARCH_BFIN
    MPV_common_init_bfin(s);
#endif

    /* load & permutate scantables
       note: only wmv uses different ones
    */
    if(s->alternate_scan){
        ff_init_scantable(s->dsp.idct_permutation, &s->inter_scantable  , ff_alternate_vertical_scan);
        ff_init_scantable(s->dsp.idct_permutation, &s->intra_scantable  , ff_alternate_vertical_scan);
        #ifdef __tmp_use_amba_permutation__
        ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_inter_scantable  , ff_alternate_vertical_scan);
        ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_intra_scantable  , ff_alternate_vertical_scan);
        #endif
    }else{
        ff_init_scantable(s->dsp.idct_permutation, &s->inter_scantable  , ff_zigzag_direct);
        ff_init_scantable(s->dsp.idct_permutation, &s->intra_scantable  , ff_zigzag_direct);
        #ifdef __tmp_use_amba_permutation__
        ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_inter_scantable  , ff_zigzag_direct);
        ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_intra_scantable  , ff_zigzag_direct);
        #endif
    }
    ff_init_scantable(s->dsp.idct_permutation, &s->intra_h_scantable, ff_alternate_horizontal_scan);
    ff_init_scantable(s->dsp.idct_permutation, &s->intra_v_scantable, ff_alternate_vertical_scan);
    #ifdef __tmp_use_amba_permutation__
    ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_intra_h_scantable, ff_alternate_horizontal_scan);
    ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_intra_v_scantable, ff_alternate_vertical_scan);
    #endif
    return 0;
}

void ff_copy_picture(Picture *dst, Picture *src){
    *dst = *src;
    dst->type= FF_BUFFER_TYPE_COPY;
}

/**
 * Release a frame buffer
 */
static void free_frame_buffer(MpegEncContext *s, Picture *pic)
{
    ff_thread_release_buffer(s->avctx, (AVFrame*)pic);
    av_freep(&pic->hwaccel_picture_private);
}

/**
 * Allocate a frame buffer
 */
static int alloc_frame_buffer(MpegEncContext *s, Picture *pic)
{
    int r;

    if (s->avctx->hwaccel) {
        assert(!pic->hwaccel_picture_private);
        if (s->avctx->hwaccel->priv_data_size) {
            pic->hwaccel_picture_private = av_mallocz(s->avctx->hwaccel->priv_data_size);
            if (!pic->hwaccel_picture_private) {
                av_log(s->avctx, AV_LOG_ERROR, "alloc_frame_buffer() failed (hwaccel private data allocation)\n");
                return -1;
            }
        }
    }

    r = ff_thread_get_buffer(s->avctx, (AVFrame*)pic);

    if (r<0 || !pic->age || !pic->type || !pic->data[0]) {
        av_log(s->avctx, AV_LOG_ERROR, "get_buffer() failed (%d %d %d %p)\n", r, pic->age, pic->type, pic->data[0]);
        av_freep(&pic->hwaccel_picture_private);
        return -1;
    }

    if (s->linesize && (s->linesize != pic->linesize[0] || s->uvlinesize != pic->linesize[1])) {
        av_log(s->avctx, AV_LOG_ERROR, "get_buffer() failed (stride changed)\n");
        free_frame_buffer(s, pic);
        return -1;
    }

    if (pic->linesize[1] != pic->linesize[2]) {
        av_log(s->avctx, AV_LOG_ERROR, "get_buffer() failed (uv stride mismatch)\n");
        free_frame_buffer(s, pic);
        return -1;
    }

    return 0;
}

/**
 * allocates a Picture
 * The pixels are allocated/set by calling get_buffer() if shared=0
 */
int ff_alloc_picture(MpegEncContext *s, Picture *pic, int shared){
    const int big_mb_num= s->mb_stride*(s->mb_height+1) + 1; //the +1 is needed so memset(,,stride*height) does not sig11
    const int mb_array_size= s->mb_stride*s->mb_height;
    const int b8_array_size= s->b8_stride*s->mb_height*2;
    const int b4_array_size= s->b4_stride*s->mb_height*4;
    int i;
    int r= -1;

    if(shared){
        assert(pic->data[0]);
        assert(pic->type == 0 || pic->type == FF_BUFFER_TYPE_SHARED);
        pic->type= FF_BUFFER_TYPE_SHARED;
    }else{
        assert(!pic->data[0]);

        if (alloc_frame_buffer(s, pic) < 0)
            return -1;

        #ifdef __log_dump_data__
/*        if(pic)
        {
            log_openfile(5,"frame_data_UV_nv12_ori_00");
            log_dump(5,pic->data[1]-16-8*pic->linesize[1],pic->linesize[1]*(s->height+16));
            log_closefile(5);
        }*/
        #endif

        s->linesize  = pic->linesize[0];
        s->uvlinesize= pic->linesize[1];
    }

    if(pic->qscale_table==NULL){
        if (s->encoding) {
            FF_ALLOCZ_OR_GOTO(s->avctx, pic->mb_var   , mb_array_size * sizeof(int16_t)  , fail)
            FF_ALLOCZ_OR_GOTO(s->avctx, pic->mc_mb_var, mb_array_size * sizeof(int16_t)  , fail)
            FF_ALLOCZ_OR_GOTO(s->avctx, pic->mb_mean  , mb_array_size * sizeof(int8_t )  , fail)
        }

        FF_ALLOCZ_OR_GOTO(s->avctx, pic->mbskip_table , mb_array_size * sizeof(uint8_t)+2, fail) //the +2 is for the slice end check
        FF_ALLOCZ_OR_GOTO(s->avctx, pic->qscale_table , mb_array_size * sizeof(uint8_t)  , fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, pic->mb_type_base , (big_mb_num + s->mb_stride) * sizeof(uint32_t), fail)
        pic->mb_type= pic->mb_type_base + 2*s->mb_stride+1;
        if(s->out_format == FMT_H264){
            for(i=0; i<2; i++){
                FF_ALLOCZ_OR_GOTO(s->avctx, pic->motion_val_base[i], 2 * (b4_array_size+4)  * sizeof(int16_t), fail)
                pic->motion_val[i]= pic->motion_val_base[i]+4;
                FF_ALLOCZ_OR_GOTO(s->avctx, pic->ref_index[i], 4*mb_array_size * sizeof(uint8_t), fail)
            }
            pic->motion_subsample_log2= 2;
        }else if(s->out_format == FMT_H263 || s->encoding || (s->avctx->debug&FF_DEBUG_MV) || (s->avctx->debug_mv)){
            for(i=0; i<2; i++){
                FF_ALLOCZ_OR_GOTO(s->avctx, pic->motion_val_base[i], 2 * (b8_array_size+4) * sizeof(int16_t), fail)
                pic->motion_val[i]= pic->motion_val_base[i]+4;
                FF_ALLOCZ_OR_GOTO(s->avctx, pic->ref_index[i], 4*mb_array_size * sizeof(uint8_t), fail)
            }
            pic->motion_subsample_log2= 3;
        }
        if(s->avctx->debug&FF_DEBUG_DCT_COEFF) {
            FF_ALLOCZ_OR_GOTO(s->avctx, pic->dct_coeff, 64 * mb_array_size * sizeof(DCTELEM)*6, fail)
        }
        pic->qstride= s->mb_stride;
        FF_ALLOCZ_OR_GOTO(s->avctx, pic->pan_scan , 1 * sizeof(AVPanScan), fail)
    }

    /* It might be nicer if the application would keep track of these
     * but it would require an API change. */
    memmove(s->prev_pict_types+1, s->prev_pict_types, PREV_PICT_TYPES_BUFFER_SIZE-1);
    s->prev_pict_types[0]= s->dropable ? FF_B_TYPE : s->pict_type;
    if(pic->age < PREV_PICT_TYPES_BUFFER_SIZE && s->prev_pict_types[pic->age] == FF_B_TYPE)
        pic->age= INT_MAX; // Skipped MBs in B-frames are quite rare in MPEG-1/2 and it is a bit tricky to skip them anyway.
    pic->owner2 = s;

    return 0;
fail: //for the FF_ALLOCZ_OR_GOTO macro
    if(r>=0)
        free_frame_buffer(s, pic);
    return -1;
}

/**
 * deallocates a picture
 */
static void free_picture(MpegEncContext *s, Picture *pic){
    int i;

    if(pic->data[0] && pic->type!=FF_BUFFER_TYPE_SHARED){
        free_frame_buffer(s, pic);
    }

    av_freep(&pic->mb_var);
    av_freep(&pic->mc_mb_var);
    av_freep(&pic->mb_mean);
    av_freep(&pic->mbskip_table);
    av_freep(&pic->qscale_table);
    av_freep(&pic->mb_type_base);
    av_freep(&pic->dct_coeff);
    av_freep(&pic->pan_scan);
    pic->mb_type= NULL;
    for(i=0; i<2; i++){
        av_freep(&pic->motion_val_base[i]);
        av_freep(&pic->ref_index[i]);
    }

    if(pic->type == FF_BUFFER_TYPE_SHARED){
        for(i=0; i<4; i++){
            pic->base[i]=
            pic->data[i]= NULL;
        }
        pic->type= 0;
    }
}

static int init_duplicate_context(MpegEncContext *s, MpegEncContext *base){
    int y_size = s->b8_stride * (2 * s->mb_height + 1);
    int c_size = s->mb_stride * (s->mb_height + 1);
    int yc_size = y_size + 2 * c_size;
    int i;

    // edge emu needs blocksize + filter length - 1 (=17x17 for halfpel / 21x21 for h264)
    FF_ALLOCZ_OR_GOTO(s->avctx, s->allocated_edge_emu_buffer, (s->width+64)*2*21*2, fail); //(width + edge + align)*interlaced*MBsize*tolerance
    s->edge_emu_buffer= s->allocated_edge_emu_buffer + (s->width+64)*2*21;

     //FIXME should be linesize instead of s->width*2 but that is not known before get_buffer()
    FF_ALLOCZ_OR_GOTO(s->avctx, s->me.scratchpad,  (s->width+64)*4*16*2*sizeof(uint8_t), fail)
    s->me.temp=         s->me.scratchpad;
    s->rd_scratchpad=   s->me.scratchpad;
    s->b_scratchpad=    s->me.scratchpad;
    s->obmc_scratchpad= s->me.scratchpad + 16;
    if (s->encoding) {
        FF_ALLOCZ_OR_GOTO(s->avctx, s->me.map      , ME_MAP_SIZE*sizeof(uint32_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->me.score_map, ME_MAP_SIZE*sizeof(uint32_t), fail)
        if(s->avctx->noise_reduction){
            FF_ALLOCZ_OR_GOTO(s->avctx, s->dct_error_sum, 2 * 64 * sizeof(int), fail)
        }
    }
    FF_ALLOCZ_OR_GOTO(s->avctx, s->blocks, 64*12*2 * sizeof(DCTELEM), fail)
    s->block= s->blocks[0];

    for(i=0;i<12;i++){
        s->pblocks[i] = &s->block[i];
    }

    if (s->out_format == FMT_H263) {
        /* ac values */
        FF_ALLOCZ_OR_GOTO(s->avctx, s->ac_val_base, yc_size * sizeof(int16_t) * 16, fail);
        s->ac_val[0] = s->ac_val_base + s->b8_stride + 1;
        s->ac_val[1] = s->ac_val_base + y_size + s->mb_stride + 1;
        s->ac_val[2] = s->ac_val[1] + c_size;
    }

    return 0;
fail:
    return -1; //free() through MPV_common_end()
}

static void free_duplicate_context(MpegEncContext *s){
    if(s==NULL) return;

    av_freep(&s->allocated_edge_emu_buffer); s->edge_emu_buffer= NULL;
    av_freep(&s->me.scratchpad);
    s->me.temp=
    s->rd_scratchpad=
    s->b_scratchpad=
    s->obmc_scratchpad= NULL;

    av_freep(&s->dct_error_sum);
    av_freep(&s->me.map);
    av_freep(&s->me.score_map);
    av_freep(&s->blocks);
    av_freep(&s->ac_val_base);
    s->block= NULL;
}

static void backup_duplicate_context(MpegEncContext *bak, MpegEncContext *src){
#define COPY(a) bak->a= src->a
    COPY(allocated_edge_emu_buffer);
    COPY(edge_emu_buffer);
    COPY(me.scratchpad);
    COPY(me.temp);
    COPY(rd_scratchpad);
    COPY(b_scratchpad);
    COPY(obmc_scratchpad);
    COPY(me.map);
    COPY(me.score_map);
    COPY(blocks);
    COPY(block);
    COPY(start_mb_y);
    COPY(end_mb_y);
    COPY(me.map_generation);
    COPY(pb);
    COPY(dct_error_sum);
    COPY(dct_count[0]);
    COPY(dct_count[1]);
    COPY(ac_val_base);
    COPY(ac_val[0]);
    COPY(ac_val[1]);
    COPY(ac_val[2]);
#undef COPY
}

void ff_update_duplicate_context(MpegEncContext *dst, MpegEncContext *src){
    MpegEncContext bak;
    int i;
    //FIXME copy only needed parts
//START_TIMER
    backup_duplicate_context(&bak, dst);
    memcpy(dst, src, sizeof(MpegEncContext));
    backup_duplicate_context(dst, &bak);
    for(i=0;i<12;i++){
        dst->pblocks[i] = &dst->block[i];
    }
//STOP_TIMER("update_duplicate_context") //about 10k cycles / 0.01 sec for 1000frames on 1ghz with 2 threads
}

int ff_mpeg_update_thread_context(AVCodecContext *dst, const AVCodecContext *src)
{
    MpegEncContext *s = dst->priv_data, *s1 = src->priv_data;

    if(dst == src || !s1->context_initialized) return 0;

    //FIXME can parameters change on I-frames? in that case dst may need a reinit
    if(!s->context_initialized){
        memcpy(s, s1, sizeof(MpegEncContext));

        s->avctx                 = dst;
        s->picture_range_start  += MAX_PICTURE_COUNT;
        s->picture_range_end    += MAX_PICTURE_COUNT;
        s->bitstream_buffer      = NULL;
        s->bitstream_buffer_size = s->allocated_bitstream_buffer_size = 0;

        MPV_common_init(s);
    }

    s->avctx->coded_height  = s1->avctx->coded_height;
    s->avctx->coded_width   = s1->avctx->coded_width;
    s->avctx->width         = s1->avctx->width;
    s->avctx->height        = s1->avctx->height;

    s->coded_picture_number = s1->coded_picture_number;
    s->picture_number       = s1->picture_number;
    s->input_picture_number = s1->input_picture_number;

    memcpy(s->picture, s1->picture, s1->picture_count * sizeof(Picture));
    memcpy(&s->last_picture, &s1->last_picture, (char*)&s1->last_picture_ptr - (char*)&s1->last_picture);

    s->last_picture_ptr     = REBASE_PICTURE(s1->last_picture_ptr,    s, s1);
    s->current_picture_ptr  = REBASE_PICTURE(s1->current_picture_ptr, s, s1);
    s->next_picture_ptr     = REBASE_PICTURE(s1->next_picture_ptr,    s, s1);

    memcpy(s->prev_pict_types, s1->prev_pict_types, PREV_PICT_TYPES_BUFFER_SIZE);

    //Error/bug resilience
    s->next_p_frame_damaged = s1->next_p_frame_damaged;
    s->workaround_bugs      = s1->workaround_bugs;

    //MPEG4 timing info
    memcpy(&s->time_increment_bits, &s1->time_increment_bits, (char*)&s1->shape - (char*)&s1->time_increment_bits);

    //B-frame info
    s->max_b_frames         = s1->max_b_frames;
    s->low_delay            = s1->low_delay;
    s->dropable             = s1->dropable;

    //DivX handling (doesn't work)
    s->divx_packed          = s1->divx_packed;

    if(s1->bitstream_buffer){
        if (s1->bitstream_buffer_size + FF_INPUT_BUFFER_PADDING_SIZE > s->allocated_bitstream_buffer_size)
            av_fast_malloc(&s->bitstream_buffer, &s->allocated_bitstream_buffer_size, s1->allocated_bitstream_buffer_size);
        s->bitstream_buffer_size  = s1->bitstream_buffer_size;
        memcpy(s->bitstream_buffer, s1->bitstream_buffer, s1->bitstream_buffer_size);
        memset(s->bitstream_buffer+s->bitstream_buffer_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
    }

    //MPEG2/interlacing info
    memcpy(&s->progressive_sequence, &s1->progressive_sequence, (char*)&s1->rtp_mode - (char*)&s1->progressive_sequence);

    if(!s1->first_field){
        s->last_pict_type= s1->pict_type;
        if (s1->current_picture_ptr) s->last_lambda_for[s1->pict_type] = s1->current_picture_ptr->quality;

        if(s1->pict_type!=FF_B_TYPE){
            s->last_non_b_pict_type= s1->pict_type;
        }
    }

    return 0;
}

/**
 * sets the given MpegEncContext to common defaults (same for encoding and decoding).
 * the changed fields will not depend upon the prior state of the MpegEncContext.
 */
void MPV_common_defaults(MpegEncContext *s){
    s->y_dc_scale_table=
    s->c_dc_scale_table= ff_mpeg1_dc_scale_table;
    s->chroma_qscale_table= ff_default_chroma_qscale_table;
    s->progressive_frame= 1;
    s->progressive_sequence= 1;
    s->picture_structure= PICT_FRAME;

    s->coded_picture_number = 0;
    s->picture_number = 0;
    s->input_picture_number = 0;

    s->picture_in_gop_number = 0;

    s->f_code = 1;
    s->b_code = 1;

    s->picture_range_start = 0;
    s->picture_range_end = MAX_PICTURE_COUNT;
}

/**
 * sets the given MpegEncContext to defaults for decoding.
 * the changed fields will not depend upon the prior state of the MpegEncContext.
 */
void MPV_decode_defaults(MpegEncContext *s){
    MPV_common_defaults(s);
}

/**
 * init common structure for both encoder and decoder.
 * this assumes that some variables like width/height are already set
 */
av_cold int MPV_common_init(MpegEncContext *s)
{
    int y_size, c_size, yc_size, i, mb_array_size, mv_table_size, x, y, threads;

    if(s->codec_id == CODEC_ID_MPEG2VIDEO && !s->progressive_sequence)
        s->mb_height = (s->height + 31) / 32 * 2;
    else if (s->codec_id != CODEC_ID_H264)
        s->mb_height = (s->height + 15) / 16;

    if(s->avctx->pix_fmt == PIX_FMT_NONE){
        av_log(s->avctx, AV_LOG_ERROR, "decoding to PIX_FMT_NONE is not supported.\n");
        return -1;
    }

    if(s->avctx->active_thread_type&FF_THREAD_SLICE &&
       (s->avctx->thread_count > MAX_THREADS || (s->avctx->thread_count > s->mb_height && s->mb_height))){
        av_log(s->avctx, AV_LOG_ERROR, "too many threads\n");
        return -1;
    }

    if((s->width || s->height) && av_image_check_size(s->width, s->height, 0, s->avctx))
        return -1;

    dsputil_init(&s->dsp, s->avctx);
    ff_dct_common_init(s);

    s->flags= s->avctx->flags;
    s->flags2= s->avctx->flags2;

    s->mb_width  = (s->width  + 15) / 16;
    s->mb_stride = s->mb_width + 1;
    s->b8_stride = s->mb_width*2 + 1;
    s->b4_stride = s->mb_width*4 + 1;
    mb_array_size= s->mb_height * s->mb_stride;
    mv_table_size= (s->mb_height+2) * s->mb_stride + 1;

    /* set chroma shifts */
    avcodec_get_chroma_sub_sample(s->avctx->pix_fmt,&(s->chroma_x_shift),
                                                    &(s->chroma_y_shift) );

    /* set default edge pos, will be overriden in decode_header if needed */
    s->h_edge_pos= s->mb_width*16;
    s->v_edge_pos= s->mb_height*16;

    s->mb_num = s->mb_width * s->mb_height;

    s->block_wrap[0]=
    s->block_wrap[1]=
    s->block_wrap[2]=
    s->block_wrap[3]= s->b8_stride;
    s->block_wrap[4]=
    s->block_wrap[5]= s->mb_stride;

    y_size = s->b8_stride * (2 * s->mb_height + 1);
    c_size = s->mb_stride * (s->mb_height + 1);
    yc_size = y_size + 2 * c_size;

    /* convert fourcc to upper case */
    s->codec_tag = ff_toupper4(s->avctx->codec_tag);

    s->stream_codec_tag = ff_toupper4(s->avctx->stream_codec_tag);

    s->avctx->coded_frame= (AVFrame*)&s->current_picture;

    FF_ALLOCZ_OR_GOTO(s->avctx, s->mb_index2xy, (s->mb_num+1)*sizeof(int), fail) //error ressilience code looks cleaner with this
    for(y=0; y<s->mb_height; y++){
        for(x=0; x<s->mb_width; x++){
            s->mb_index2xy[ x + y*s->mb_width ] = x + y*s->mb_stride;
        }
    }
    s->mb_index2xy[ s->mb_height*s->mb_width ] = (s->mb_height-1)*s->mb_stride + s->mb_width; //FIXME really needed?

    if (s->encoding) {
        /* Allocate MV tables */
        FF_ALLOCZ_OR_GOTO(s->avctx, s->p_mv_table_base            , mv_table_size * 2 * sizeof(int16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_forw_mv_table_base       , mv_table_size * 2 * sizeof(int16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_back_mv_table_base       , mv_table_size * 2 * sizeof(int16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_bidir_forw_mv_table_base , mv_table_size * 2 * sizeof(int16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_bidir_back_mv_table_base , mv_table_size * 2 * sizeof(int16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_direct_mv_table_base     , mv_table_size * 2 * sizeof(int16_t), fail)
        s->p_mv_table           = s->p_mv_table_base            + s->mb_stride + 1;
        s->b_forw_mv_table      = s->b_forw_mv_table_base       + s->mb_stride + 1;
        s->b_back_mv_table      = s->b_back_mv_table_base       + s->mb_stride + 1;
        s->b_bidir_forw_mv_table= s->b_bidir_forw_mv_table_base + s->mb_stride + 1;
        s->b_bidir_back_mv_table= s->b_bidir_back_mv_table_base + s->mb_stride + 1;
        s->b_direct_mv_table    = s->b_direct_mv_table_base     + s->mb_stride + 1;

        if(s->msmpeg4_version){
            FF_ALLOCZ_OR_GOTO(s->avctx, s->ac_stats, 2*2*(MAX_LEVEL+1)*(MAX_RUN+1)*2*sizeof(int), fail);
        }
        FF_ALLOCZ_OR_GOTO(s->avctx, s->avctx->stats_out, 256, fail);

        /* Allocate MB type table */
        FF_ALLOCZ_OR_GOTO(s->avctx, s->mb_type  , mb_array_size * sizeof(uint16_t), fail) //needed for encoding

        FF_ALLOCZ_OR_GOTO(s->avctx, s->lambda_table, mb_array_size * sizeof(int), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_intra_matrix  , 64*32   * sizeof(int), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_inter_matrix  , 64*32   * sizeof(int), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_intra_matrix16, 64*32*2 * sizeof(uint16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_inter_matrix16, 64*32*2 * sizeof(uint16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->input_picture, MAX_PICTURE_COUNT * sizeof(Picture*), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->reordered_input_picture, MAX_PICTURE_COUNT * sizeof(Picture*), fail)

        if(s->avctx->noise_reduction){
            FF_ALLOCZ_OR_GOTO(s->avctx, s->dct_offset, 2 * 64 * sizeof(uint16_t), fail)
        }
    }
    s->picture_count = MAX_PICTURE_COUNT * FFMAX(1, s->avctx->thread_count);
    FF_ALLOCZ_OR_GOTO(s->avctx, s->picture, s->picture_count * sizeof(Picture), fail)
    for(i = 0; i < s->picture_count; i++) {
        avcodec_get_frame_defaults((AVFrame *)&s->picture[i]);
    }

    FF_ALLOCZ_OR_GOTO(s->avctx, s->error_status_table, mb_array_size*sizeof(uint8_t), fail)

    if((s->codec_id==CODEC_ID_MPEG4 || s->codec_id==CODEC_ID_AMBA_MPEG4 || s->codec_id==CODEC_ID_AMBA_P_MPEG4) || (s->flags & CODEC_FLAG_INTERLACED_ME)){
        /* interlaced direct mode decoding tables */
            for(i=0; i<2; i++){
                int j, k;
                for(j=0; j<2; j++){
                    for(k=0; k<2; k++){
                        FF_ALLOCZ_OR_GOTO(s->avctx,    s->b_field_mv_table_base[i][j][k], mv_table_size * 2 * sizeof(int16_t), fail)
                        s->b_field_mv_table[i][j][k] = s->b_field_mv_table_base[i][j][k] + s->mb_stride + 1;
                    }
                    FF_ALLOCZ_OR_GOTO(s->avctx, s->b_field_select_table [i][j], mb_array_size * 2 * sizeof(uint8_t), fail)
                    FF_ALLOCZ_OR_GOTO(s->avctx, s->p_field_mv_table_base[i][j], mv_table_size * 2 * sizeof(int16_t), fail)
                    s->p_field_mv_table[i][j] = s->p_field_mv_table_base[i][j]+ s->mb_stride + 1;
                }
                FF_ALLOCZ_OR_GOTO(s->avctx, s->p_field_select_table[i], mb_array_size * 2 * sizeof(uint8_t), fail)
            }
    }
    if (s->out_format == FMT_H263) {
        /* cbp values */
        FF_ALLOCZ_OR_GOTO(s->avctx, s->coded_block_base, y_size, fail);
        s->coded_block= s->coded_block_base + s->b8_stride + 1;

        /* cbp, ac_pred, pred_dir */
        FF_ALLOCZ_OR_GOTO(s->avctx, s->cbp_table     , mb_array_size * sizeof(uint8_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->pred_dir_table, mb_array_size * sizeof(uint8_t), fail)
    }

    if (s->h263_pred || s->h263_plus || !s->encoding) {
        /* dc values */
        //MN: we need these for error resilience of intra-frames
        FF_ALLOCZ_OR_GOTO(s->avctx, s->dc_val_base, yc_size * sizeof(int16_t), fail);
        s->dc_val[0] = s->dc_val_base + s->b8_stride + 1;
        s->dc_val[1] = s->dc_val_base + y_size + s->mb_stride + 1;
        s->dc_val[2] = s->dc_val[1] + c_size;
        for(i=0;i<yc_size;i++)
            s->dc_val_base[i] = 1024;
    }

    /* which mb is a intra block */
    FF_ALLOCZ_OR_GOTO(s->avctx, s->mbintra_table, mb_array_size, fail);
    memset(s->mbintra_table, 1, mb_array_size);

    /* init macroblock skip table */
    FF_ALLOCZ_OR_GOTO(s->avctx, s->mbskip_table, mb_array_size+2, fail);
    //Note the +1 is for a quicker mpeg4 slice_end detection
    FF_ALLOCZ_OR_GOTO(s->avctx, s->prev_pict_types, PREV_PICT_TYPES_BUFFER_SIZE, fail);

    s->parse_context.state= -1;
    if((s->avctx->debug&(FF_DEBUG_VIS_QP|FF_DEBUG_VIS_MB_TYPE)) || (s->avctx->debug_mv)){
       s->visualization_buffer[0] = av_malloc((s->mb_width*16 + 2*EDGE_WIDTH) * s->mb_height*16 + 2*EDGE_WIDTH);
       s->visualization_buffer[1] = av_malloc((s->mb_width*16 + 2*EDGE_WIDTH) * s->mb_height*16 + 2*EDGE_WIDTH);
       s->visualization_buffer[2] = av_malloc((s->mb_width*16 + 2*EDGE_WIDTH) * s->mb_height*16 + 2*EDGE_WIDTH);
    }

    s->context_initialized = 1;
    s->thread_context[0]= s;

    if (HAVE_THREADS && s->avctx->active_thread_type&FF_THREAD_SLICE) {
        threads = s->avctx->thread_count;

        for(i=1; i<threads; i++){
            s->thread_context[i]= av_malloc(sizeof(MpegEncContext));
            memcpy(s->thread_context[i], s, sizeof(MpegEncContext));
        }

        for(i=0; i<threads; i++){
            if(init_duplicate_context(s->thread_context[i], s) < 0)
               goto fail;
            s->thread_context[i]->start_mb_y= (s->mb_height*(i  ) + s->avctx->thread_count/2) / s->avctx->thread_count;
            s->thread_context[i]->end_mb_y  = (s->mb_height*(i+1) + s->avctx->thread_count/2) / s->avctx->thread_count;
        }
    } else {
        if(init_duplicate_context(s, s) < 0) goto fail;
        s->start_mb_y = 0;
        s->end_mb_y   = s->mb_height;
    }

    return 0;
 fail:
    MPV_common_end(s);
    return -1;
}

/* init common structure for both encoder and decoder */
void MPV_common_end(MpegEncContext *s)
{
    int i, j, k;

    if (HAVE_THREADS && s->avctx->active_thread_type&FF_THREAD_SLICE) {
        for(i=0; i<s->avctx->thread_count; i++){
            free_duplicate_context(s->thread_context[i]);
        }
        for(i=1; i<s->avctx->thread_count; i++){
            av_freep(&s->thread_context[i]);
        }
    } else free_duplicate_context(s);

    av_freep(&s->parse_context.buffer);
    s->parse_context.buffer_size=0;

    av_freep(&s->mb_type);
    av_freep(&s->p_mv_table_base);
    av_freep(&s->b_forw_mv_table_base);
    av_freep(&s->b_back_mv_table_base);
    av_freep(&s->b_bidir_forw_mv_table_base);
    av_freep(&s->b_bidir_back_mv_table_base);
    av_freep(&s->b_direct_mv_table_base);
    s->p_mv_table= NULL;
    s->b_forw_mv_table= NULL;
    s->b_back_mv_table= NULL;
    s->b_bidir_forw_mv_table= NULL;
    s->b_bidir_back_mv_table= NULL;
    s->b_direct_mv_table= NULL;
    for(i=0; i<2; i++){
        for(j=0; j<2; j++){
            for(k=0; k<2; k++){
                av_freep(&s->b_field_mv_table_base[i][j][k]);
                s->b_field_mv_table[i][j][k]=NULL;
            }
            av_freep(&s->b_field_select_table[i][j]);
            av_freep(&s->p_field_mv_table_base[i][j]);
            s->p_field_mv_table[i][j]=NULL;
        }
        av_freep(&s->p_field_select_table[i]);
    }

    av_freep(&s->dc_val_base);
    av_freep(&s->coded_block_base);
    av_freep(&s->mbintra_table);
    av_freep(&s->cbp_table);
    av_freep(&s->pred_dir_table);

    av_freep(&s->mbskip_table);
    av_freep(&s->prev_pict_types);
    av_freep(&s->bitstream_buffer);
    s->allocated_bitstream_buffer_size=0;

    av_freep(&s->avctx->stats_out);
    av_freep(&s->ac_stats);
    av_freep(&s->error_status_table);
    av_freep(&s->mb_index2xy);
    av_freep(&s->lambda_table);
    av_freep(&s->q_intra_matrix);
    av_freep(&s->q_inter_matrix);
    av_freep(&s->q_intra_matrix16);
    av_freep(&s->q_inter_matrix16);
    av_freep(&s->input_picture);
    av_freep(&s->reordered_input_picture);
    av_freep(&s->dct_offset);

    if(s->picture && !s->avctx->is_copy){
        for(i=0; i<s->picture_count; i++){
            free_picture(s, &s->picture[i]);
        }
    }
    av_freep(&s->picture);
    s->context_initialized = 0;
    s->last_picture_ptr=
    s->next_picture_ptr=
    s->current_picture_ptr= NULL;
    s->linesize= s->uvlinesize= 0;

    for(i=0; i<3; i++)
        av_freep(&s->visualization_buffer[i]);

    if(!(s->avctx->active_thread_type&FF_THREAD_FRAME))
        avcodec_default_free_buffers(s->avctx);
}

void init_rl(RLTable *rl, uint8_t static_store[2][2*MAX_RUN + MAX_LEVEL + 3])
{
    int8_t max_level[MAX_RUN+1], max_run[MAX_LEVEL+1];
    uint8_t index_run[MAX_RUN+1];
    int last, run, level, start, end, i;

    /* If table is static, we can quit if rl->max_level[0] is not NULL */
    if(static_store && rl->max_level[0])
        return;

    /* compute max_level[], max_run[] and index_run[] */
    for(last=0;last<2;last++) {
        if (last == 0) {
            start = 0;
            end = rl->last;
        } else {
            start = rl->last;
            end = rl->n;
        }

        memset(max_level, 0, MAX_RUN + 1);
        memset(max_run, 0, MAX_LEVEL + 1);
        memset(index_run, rl->n, MAX_RUN + 1);
        for(i=start;i<end;i++) {
            run = rl->table_run[i];
            level = rl->table_level[i];
            if (index_run[run] == rl->n)
                index_run[run] = i;
            if (level > max_level[run])
                max_level[run] = level;
            if (run > max_run[level])
                max_run[level] = run;
        }
        if(static_store)
            rl->max_level[last] = static_store[last];
        else
            rl->max_level[last] = av_malloc(MAX_RUN + 1);
        memcpy(rl->max_level[last], max_level, MAX_RUN + 1);
        if(static_store)
            rl->max_run[last] = static_store[last] + MAX_RUN + 1;
        else
            rl->max_run[last] = av_malloc(MAX_LEVEL + 1);
        memcpy(rl->max_run[last], max_run, MAX_LEVEL + 1);
        if(static_store)
            rl->index_run[last] = static_store[last] + MAX_RUN + MAX_LEVEL + 2;
        else
            rl->index_run[last] = av_malloc(MAX_RUN + 1);
        memcpy(rl->index_run[last], index_run, MAX_RUN + 1);
    }
}

void init_vlc_rl(RLTable *rl)
{
    int i, q;

    for(q=0; q<32; q++){
        int qmul= q*2;
        int qadd= (q-1)|1;

        if(q==0){
            qmul=1;
            qadd=0;
        }
        for(i=0; i<rl->vlc.table_size; i++){
            int code= rl->vlc.table[i][0];
            int len = rl->vlc.table[i][1];
            int level, run;

            if(len==0){ // illegal code
                run= 66;
                level= MAX_LEVEL;
            }else if(len<0){ //more bits needed
                run= 0;
                level= code;
            }else{
                if(code==rl->n){ //esc
                    run= 66;
                    level= 0;
                }else{
                    run=   rl->table_run  [code] + 1;
                    level= rl->table_level[code] * qmul + qadd;
                    if(code >= rl->last) run+=192;
                }
            }
            rl->rl_vlc[q][i].len= len;
            rl->rl_vlc[q][i].level= level;
            rl->rl_vlc[q][i].run= run;
        }
    }
}

void ff_release_unused_pictures(MpegEncContext *s, int remove_current)
{
    int i;

    /* release non reference frames */
    for(i=0; i<s->picture_count; i++){
        if(s->picture[i].data[0] && !s->picture[i].reference
           && s->picture[i].owner2 == s
           && (remove_current || &s->picture[i] != s->current_picture_ptr)
           /*&& s->picture[i].type!=FF_BUFFER_TYPE_SHARED*/){
            free_frame_buffer(s, &s->picture[i]);
        }
    }
}

int ff_find_unused_picture(MpegEncContext *s, int shared){
    int i;

    if(shared){
        for(i=s->picture_range_start; i<s->picture_range_end; i++){
            if(s->picture[i].data[0]==NULL && s->picture[i].type==0) return i;
        }
    }else{
        for(i=s->picture_range_start; i<s->picture_range_end; i++){
            if(s->picture[i].data[0]==NULL && s->picture[i].type!=0) return i; //FIXME
        }
        for(i=s->picture_range_start; i<s->picture_range_end; i++){
            if(s->picture[i].data[0]==NULL) return i;
        }
    }

    av_log(s->avctx, AV_LOG_FATAL, "Internal error, picture buffer overflow\n");
    /* We could return -1, but the codec would crash trying to draw into a
     * non-existing frame anyway. This is safer than waiting for a random crash.
     * Also the return of this is never useful, an encoder must only allocate
     * as much as allowed in the specification. This has no relationship to how
     * much libavcodec could allocate (and MAX_PICTURE_COUNT is always large
     * enough for such valid streams).
     * Plus, a decoder has to check stream validity and remove frames if too
     * many reference frames are around. Waiting for "OOM" is not correct at
     * all. Similarly, missing reference frames have to be replaced by
     * interpolated/MC frames, anything else is a bug in the codec ...
     */
    abort();
    return -1;
}

static void update_noise_reduction(MpegEncContext *s){
    int intra, i;

    for(intra=0; intra<2; intra++){
        if(s->dct_count[intra] > (1<<16)){
            for(i=0; i<64; i++){
                s->dct_error_sum[intra][i] >>=1;
            }
            s->dct_count[intra] >>= 1;
        }

        for(i=0; i<64; i++){
            s->dct_offset[intra][i]= (s->avctx->noise_reduction * s->dct_count[intra] + s->dct_error_sum[intra][i]/2) / (s->dct_error_sum[intra][i]+1);
        }
    }
}

/**
 * generic function for encode/decode called after coding/decoding the header and before a frame is coded/decoded
 */
int MPV_frame_start(MpegEncContext *s, AVCodecContext *avctx)
{
    int i;
    Picture *pic;
    s->mb_skipped = 0;

    assert(s->last_picture_ptr==NULL || s->out_format != FMT_H264 || s->codec_id == CODEC_ID_SVQ3);

    /* mark&release old frames */
    if (s->pict_type != FF_B_TYPE && s->last_picture_ptr && s->last_picture_ptr != s->next_picture_ptr && s->last_picture_ptr->data[0]) {
      if(s->out_format != FMT_H264 || s->codec_id == CODEC_ID_SVQ3){
          free_frame_buffer(s, s->last_picture_ptr);

        /* release forgotten pictures */
        /* if(mpeg124/h263) */
        if(!s->encoding){
            for(i=0; i<s->picture_count; i++){
                if(s->picture[i].data[0] && &s->picture[i] != s->next_picture_ptr && s->picture[i].reference){
                    av_log(avctx, AV_LOG_ERROR, "releasing zombie picture\n");
                    free_frame_buffer(s, &s->picture[i]);
                }
            }
        }
      }
    }

    if(!s->encoding){
        ff_release_unused_pictures(s, 1);

        if(s->current_picture_ptr && s->current_picture_ptr->data[0]==NULL)
            pic= s->current_picture_ptr; //we already have a unused image (maybe it was set before reading the header)
        else{
            i= ff_find_unused_picture(s, 0);
            pic= &s->picture[i];
        }

        pic->reference= 0;
        if (!s->dropable){
            if (s->codec_id == CODEC_ID_H264)
                pic->reference = s->picture_structure;
            else if (s->pict_type != FF_B_TYPE)
                pic->reference = 3;
        }

        pic->coded_picture_number= s->coded_picture_number++;

        if(ff_alloc_picture(s, pic, 0) < 0)
            return -1;

        s->current_picture_ptr= pic;
        //FIXME use only the vars from current_pic
        s->current_picture_ptr->top_field_first= s->top_field_first;
        if(s->codec_id == CODEC_ID_MPEG1VIDEO || s->codec_id == CODEC_ID_MPEG2VIDEO) {
            if(s->picture_structure != PICT_FRAME)
                s->current_picture_ptr->top_field_first= (s->picture_structure == PICT_TOP_FIELD) == s->first_field;
        }
        s->current_picture_ptr->interlaced_frame= !s->progressive_frame && !s->progressive_sequence;
        s->current_picture_ptr->field_picture= s->picture_structure != PICT_FRAME;
        #ifdef __log_dump_data__
/*        if(s->current_picture_ptr)
        {
            log_openfile(4,"frame_data_UV_nv12_ori_11");
            log_dump(4,s->current_picture_ptr->data[1]-16-8*s->current_picture_ptr->linesize[1],s->current_picture_ptr->linesize[1]*(s->height+16));
            log_closefile(4);
        }*/
        #endif
    }

    s->current_picture_ptr->pict_type= s->pict_type;
//    if(s->flags && CODEC_FLAG_QSCALE)
  //      s->current_picture_ptr->quality= s->new_picture_ptr->quality;
    s->current_picture_ptr->key_frame= s->pict_type == FF_I_TYPE;

    ff_copy_picture(&s->current_picture, s->current_picture_ptr);

    if (s->pict_type != FF_B_TYPE) {
        s->last_picture_ptr= s->next_picture_ptr;
        if(!s->dropable)
            s->next_picture_ptr= s->current_picture_ptr;
    }
/*    av_log(s->avctx, AV_LOG_DEBUG, "L%p N%p C%p L%p N%p C%p type:%d drop:%d\n", s->last_picture_ptr, s->next_picture_ptr,s->current_picture_ptr,
        s->last_picture_ptr    ? s->last_picture_ptr->data[0] : NULL,
        s->next_picture_ptr    ? s->next_picture_ptr->data[0] : NULL,
        s->current_picture_ptr ? s->current_picture_ptr->data[0] : NULL,
        s->pict_type, s->dropable);*/

    if(s->codec_id != CODEC_ID_H264){
        if((s->last_picture_ptr==NULL || s->last_picture_ptr->data[0]==NULL) && s->pict_type!=FF_I_TYPE){
            av_log(avctx, AV_LOG_ERROR, "warning: first frame is no keyframe\n");
            /* Allocate a dummy frame */
            i= ff_find_unused_picture(s, 0);
            s->last_picture_ptr= &s->picture[i];
            if(ff_alloc_picture(s, s->last_picture_ptr, 0) < 0)
                return -1;
            ff_thread_report_progress((AVFrame*)s->last_picture_ptr, INT_MAX, 0);
            ff_thread_report_progress((AVFrame*)s->last_picture_ptr, INT_MAX, 1);
        }
        if((s->next_picture_ptr==NULL || s->next_picture_ptr->data[0]==NULL) && s->pict_type==FF_B_TYPE){
            /* Allocate a dummy frame */
            i= ff_find_unused_picture(s, 0);
            s->next_picture_ptr= &s->picture[i];
            if(ff_alloc_picture(s, s->next_picture_ptr, 0) < 0)
                return -1;
            ff_thread_report_progress((AVFrame*)s->next_picture_ptr, INT_MAX, 0);
            ff_thread_report_progress((AVFrame*)s->next_picture_ptr, INT_MAX, 1);
        }
    }

    if(s->last_picture_ptr) ff_copy_picture(&s->last_picture, s->last_picture_ptr);
    if(s->next_picture_ptr) ff_copy_picture(&s->next_picture, s->next_picture_ptr);

    assert(s->pict_type == FF_I_TYPE || (s->last_picture_ptr && s->last_picture_ptr->data[0]));

    if(s->picture_structure!=PICT_FRAME && s->out_format != FMT_H264){
        int i;
        for(i=0; i<4; i++){
            if(s->picture_structure == PICT_BOTTOM_FIELD){
                 s->current_picture.data[i] += s->current_picture.linesize[i];
            }
            s->current_picture.linesize[i] *= 2;
            s->last_picture.linesize[i] *=2;
            s->next_picture.linesize[i] *=2;
        }
    }

#if FF_API_HURRY_UP
    s->hurry_up= s->avctx->hurry_up;
#endif
    s->error_recognition= avctx->error_recognition;

    /* set dequantizer, we can't do it during init as it might change for mpeg4
       and we can't do it in the header decode as init is not called for mpeg4 there yet */
    if(s->mpeg_quant || s->codec_id == CODEC_ID_MPEG2VIDEO){
        s->dct_unquantize_intra = s->dct_unquantize_mpeg2_intra;
        s->dct_unquantize_inter = s->dct_unquantize_mpeg2_inter;
    }else if(s->out_format == FMT_H263 || s->out_format == FMT_H261){
        s->dct_unquantize_intra = s->dct_unquantize_h263_intra;
        s->dct_unquantize_inter = s->dct_unquantize_h263_inter;
    }else{
        s->dct_unquantize_intra = s->dct_unquantize_mpeg1_intra;
        s->dct_unquantize_inter = s->dct_unquantize_mpeg1_inter;
    }

    if(s->dct_error_sum){
        assert(s->avctx->noise_reduction && s->encoding);

        update_noise_reduction(s);
    }

    if(CONFIG_MPEG_XVMC_DECODER && s->avctx->xvmc_acceleration)
        return ff_xvmc_field_start(s, avctx);

    return 0;
}

/* generic function for encode/decode called after a frame has been coded/decoded */
void MPV_frame_end(MpegEncContext *s)
{
    int i;
    /* redraw edges for the frame if decoding didn't complete */
    //just to make sure that all data is rendered.
    if(CONFIG_MPEG_XVMC_DECODER && s->avctx->xvmc_acceleration){
        ff_xvmc_field_end(s);
   }else if((s->error_count || s->encoding || !(s->avctx->codec->capabilities&CODEC_CAP_DRAW_HORIZ_BAND))
       && !s->avctx->hwaccel
       && !(s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU)
       && s->unrestricted_mv
       && s->current_picture.reference
       && !s->intra_only
       && !(s->flags&CODEC_FLAG_EMU_EDGE)) {
        int edges = EDGE_BOTTOM | EDGE_TOP, h = s->v_edge_pos;

            s->dsp.draw_edges(s->current_picture.data[0], s->linesize  , s->h_edge_pos   , h   , EDGE_WIDTH  , edges);
            s->dsp.draw_edges(s->current_picture.data[1], s->uvlinesize, s->h_edge_pos>>1, h>>1, EDGE_WIDTH/2, edges);
            s->dsp.draw_edges(s->current_picture.data[2], s->uvlinesize, s->h_edge_pos>>1, h>>1, EDGE_WIDTH/2, edges);

    }

    emms_c();
//av_log(NULL,AV_LOG_WARNING,"s->avctx->hwaccel=%d,vdpau=%d.\n",s->avctx->hwaccel,(s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU));
//av_log(NULL,AV_LOG_WARNING,"s->unrestricted_mv=%d,s->current_picture.reference=%d,s->intra_only=%d.\n",s->unrestricted_mv,s->current_picture.reference,s->intra_only);
//av_log(NULL,AV_LOG_WARNING,"edge=%d, here!!!!\n.\n",(s->flags&CODEC_FLAG_EMU_EDGE));

    s->last_pict_type    = s->pict_type;
    s->last_lambda_for[s->pict_type]= s->current_picture_ptr->quality;
    if(s->pict_type!=FF_B_TYPE){
        s->last_non_b_pict_type= s->pict_type;
    }
#if 0
        /* copy back current_picture variables */
    for(i=0; i<MAX_PICTURE_COUNT; i++){
        if(s->picture[i].data[0] == s->current_picture.data[0]){
            s->picture[i]= s->current_picture;
            break;
        }
    }
    assert(i<MAX_PICTURE_COUNT);
#endif

    if(s->encoding){
        /* release non-reference frames */
        for(i=0; i<s->picture_count; i++){
            if(s->picture[i].data[0] && !s->picture[i].reference /*&& s->picture[i].type!=FF_BUFFER_TYPE_SHARED*/){
                free_frame_buffer(s, &s->picture[i]);
            }
        }
    }
    // clear copies, to avoid confusion
#if 0
    memset(&s->last_picture, 0, sizeof(Picture));
    memset(&s->next_picture, 0, sizeof(Picture));
    memset(&s->current_picture, 0, sizeof(Picture));
#endif
    s->avctx->coded_frame= (AVFrame*)s->current_picture_ptr;

    if (s->codec_id != CODEC_ID_H264 && s->current_picture.reference) {
        ff_thread_report_progress((AVFrame*)s->current_picture_ptr, s->mb_height-1, 0);
    }
}

/**
 * draws an line from (ex, ey) -> (sx, sy).
 * @param w width of the image
 * @param h height of the image
 * @param stride stride/linesize of the image
 * @param color color of the arrow
 */
static void draw_line(uint8_t *buf, int sx, int sy, int ex, int ey, int w, int h, int stride, int color){
    int x, y, fr, f;

    sx= av_clip(sx, 0, w-1);
    sy= av_clip(sy, 0, h-1);
    ex= av_clip(ex, 0, w-1);
    ey= av_clip(ey, 0, h-1);

    buf[sy*stride + sx]+= color;

    if(FFABS(ex - sx) > FFABS(ey - sy)){
        if(sx > ex){
            FFSWAP(int, sx, ex);
            FFSWAP(int, sy, ey);
        }
        buf+= sx + sy*stride;
        ex-= sx;
        f= ((ey-sy)<<16)/ex;
        for(x= 0; x <= ex; x++){
            y = (x*f)>>16;
            fr= (x*f)&0xFFFF;
            buf[ y   *stride + x]+= (color*(0x10000-fr))>>16;
            buf[(y+1)*stride + x]+= (color*         fr )>>16;
        }
    }else{
        if(sy > ey){
            FFSWAP(int, sx, ex);
            FFSWAP(int, sy, ey);
        }
        buf+= sx + sy*stride;
        ey-= sy;
        if(ey) f= ((ex-sx)<<16)/ey;
        else   f= 0;
        for(y= 0; y <= ey; y++){
            x = (y*f)>>16;
            fr= (y*f)&0xFFFF;
            buf[y*stride + x  ]+= (color*(0x10000-fr))>>16;
            buf[y*stride + x+1]+= (color*         fr )>>16;
        }
    }
}

/**
 * draws an arrow from (ex, ey) -> (sx, sy).
 * @param w width of the image
 * @param h height of the image
 * @param stride stride/linesize of the image
 * @param color color of the arrow
 */
static void draw_arrow(uint8_t *buf, int sx, int sy, int ex, int ey, int w, int h, int stride, int color){
    int dx,dy;

    sx= av_clip(sx, -100, w+100);
    sy= av_clip(sy, -100, h+100);
    ex= av_clip(ex, -100, w+100);
    ey= av_clip(ey, -100, h+100);

    dx= ex - sx;
    dy= ey - sy;

    if(dx*dx + dy*dy > 3*3){
        int rx=  dx + dy;
        int ry= -dx + dy;
        int length= ff_sqrt((rx*rx + ry*ry)<<8);

        //FIXME subpixel accuracy
        rx= ROUNDED_DIV(rx*3<<4, length);
        ry= ROUNDED_DIV(ry*3<<4, length);

        draw_line(buf, sx, sy, sx + rx, sy + ry, w, h, stride, color);
        draw_line(buf, sx, sy, sx - ry, sy + rx, w, h, stride, color);
    }
    draw_line(buf, sx, sy, ex, ey, w, h, stride, color);
}

/**
 * prints debuging info for the given picture.
 */
void ff_print_debug_info(MpegEncContext *s, AVFrame *pict){

    if(s->avctx->hwaccel || !pict || !pict->mb_type) return;

    if(s->avctx->debug&(FF_DEBUG_SKIP | FF_DEBUG_QP | FF_DEBUG_MB_TYPE)){
        int x,y;

        av_log(s->avctx,AV_LOG_DEBUG,"New frame, type: ");
        switch (pict->pict_type) {
            case FF_I_TYPE: av_log(s->avctx,AV_LOG_DEBUG,"I\n"); break;
            case FF_P_TYPE: av_log(s->avctx,AV_LOG_DEBUG,"P\n"); break;
            case FF_B_TYPE: av_log(s->avctx,AV_LOG_DEBUG,"B\n"); break;
            case FF_S_TYPE: av_log(s->avctx,AV_LOG_DEBUG,"S\n"); break;
            case FF_SI_TYPE: av_log(s->avctx,AV_LOG_DEBUG,"SI\n"); break;
            case FF_SP_TYPE: av_log(s->avctx,AV_LOG_DEBUG,"SP\n"); break;
        }
        for(y=0; y<s->mb_height; y++){
            for(x=0; x<s->mb_width; x++){
                if(s->avctx->debug&FF_DEBUG_SKIP){
                    int count= s->mbskip_table[x + y*s->mb_stride];
                    if(count>9) count=9;
                    av_log(s->avctx, AV_LOG_DEBUG, "%1d", count);
                }
                if(s->avctx->debug&FF_DEBUG_QP){
                    av_log(s->avctx, AV_LOG_DEBUG, "%2d", pict->qscale_table[x + y*s->mb_stride]);
                }
                if(s->avctx->debug&FF_DEBUG_MB_TYPE){
                    int mb_type= pict->mb_type[x + y*s->mb_stride];
                    //Type & MV direction
                    if(IS_PCM(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "P");
                    else if(IS_INTRA(mb_type) && IS_ACPRED(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "A");
                    else if(IS_INTRA4x4(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "i");
                    else if(IS_INTRA16x16(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "I");
                    else if(IS_DIRECT(mb_type) && IS_SKIP(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "d");
                    else if(IS_DIRECT(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "D");
                    else if(IS_GMC(mb_type) && IS_SKIP(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "g");
                    else if(IS_GMC(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "G");
                    else if(IS_SKIP(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "S");
                    else if(!USES_LIST(mb_type, 1))
                        av_log(s->avctx, AV_LOG_DEBUG, ">");
                    else if(!USES_LIST(mb_type, 0))
                        av_log(s->avctx, AV_LOG_DEBUG, "<");
                    else{
                        assert(USES_LIST(mb_type, 0) && USES_LIST(mb_type, 1));
                        av_log(s->avctx, AV_LOG_DEBUG, "X");
                    }

                    //segmentation
                    if(IS_8X8(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "+");
                    else if(IS_16X8(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "-");
                    else if(IS_8X16(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "|");
                    else if(IS_INTRA(mb_type) || IS_16X16(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, " ");
                    else
                        av_log(s->avctx, AV_LOG_DEBUG, "?");


                    if(IS_INTERLACED(mb_type))
                        av_log(s->avctx, AV_LOG_DEBUG, "=");
                    else
                        av_log(s->avctx, AV_LOG_DEBUG, " ");
                }
//                av_log(s->avctx, AV_LOG_DEBUG, " ");
            }
            av_log(s->avctx, AV_LOG_DEBUG, "\n");
        }
    }

    if((s->avctx->debug&(FF_DEBUG_VIS_QP|FF_DEBUG_VIS_MB_TYPE)) || (s->avctx->debug_mv)){
        const int shift= 1 + s->quarter_sample;
        int mb_y;
        uint8_t *ptr;
        int i;
        int h_chroma_shift, v_chroma_shift, block_height;
        const int width = s->avctx->width;
        const int height= s->avctx->height;
        const int mv_sample_log2= 4 - pict->motion_subsample_log2;
        const int mv_stride= (s->mb_width << mv_sample_log2) + (s->codec_id == CODEC_ID_H264 ? 0 : 1);
        s->low_delay=0; //needed to see the vectors without trashing the buffers

        avcodec_get_chroma_sub_sample(s->avctx->pix_fmt, &h_chroma_shift, &v_chroma_shift);
        for(i=0; i<3; i++){
            memcpy(s->visualization_buffer[i], pict->data[i], (i==0) ? pict->linesize[i]*height:pict->linesize[i]*height >> v_chroma_shift);
            pict->data[i]= s->visualization_buffer[i];
        }
        pict->type= FF_BUFFER_TYPE_COPY;
        ptr= pict->data[0];
        block_height = 16>>v_chroma_shift;

        for(mb_y=0; mb_y<s->mb_height; mb_y++){
            int mb_x;
            for(mb_x=0; mb_x<s->mb_width; mb_x++){
                const int mb_index= mb_x + mb_y*s->mb_stride;
                if((s->avctx->debug_mv) && pict->motion_val){
                  int type;
                  for(type=0; type<3; type++){
                    int direction = 0;
                    switch (type) {
                      case 0: if ((!(s->avctx->debug_mv&FF_DEBUG_VIS_MV_P_FOR)) || (pict->pict_type!=FF_P_TYPE))
                                continue;
                              direction = 0;
                              break;
                      case 1: if ((!(s->avctx->debug_mv&FF_DEBUG_VIS_MV_B_FOR)) || (pict->pict_type!=FF_B_TYPE))
                                continue;
                              direction = 0;
                              break;
                      case 2: if ((!(s->avctx->debug_mv&FF_DEBUG_VIS_MV_B_BACK)) || (pict->pict_type!=FF_B_TYPE))
                                continue;
                              direction = 1;
                              break;
                    }
                    if(!USES_LIST(pict->mb_type[mb_index], direction))
                        continue;

                    if(IS_8X8(pict->mb_type[mb_index])){
                      int i;
                      for(i=0; i<4; i++){
                        int sx= mb_x*16 + 4 + 8*(i&1);
                        int sy= mb_y*16 + 4 + 8*(i>>1);
                        int xy= (mb_x*2 + (i&1) + (mb_y*2 + (i>>1))*mv_stride) << (mv_sample_log2-1);
                        int mx= (pict->motion_val[direction][xy][0]>>shift) + sx;
                        int my= (pict->motion_val[direction][xy][1]>>shift) + sy;
                        draw_arrow(ptr, sx, sy, mx, my, width, height, s->linesize, 100);
                      }
                    }else if(IS_16X8(pict->mb_type[mb_index])){
                      int i;
                      for(i=0; i<2; i++){
                        int sx=mb_x*16 + 8;
                        int sy=mb_y*16 + 4 + 8*i;
                        int xy= (mb_x*2 + (mb_y*2 + i)*mv_stride) << (mv_sample_log2-1);
                        int mx=(pict->motion_val[direction][xy][0]>>shift);
                        int my=(pict->motion_val[direction][xy][1]>>shift);

                        if(IS_INTERLACED(pict->mb_type[mb_index]))
                            my*=2;

                        draw_arrow(ptr, sx, sy, mx+sx, my+sy, width, height, s->linesize, 100);
                      }
                    }else if(IS_8X16(pict->mb_type[mb_index])){
                      int i;
                      for(i=0; i<2; i++){
                        int sx=mb_x*16 + 4 + 8*i;
                        int sy=mb_y*16 + 8;
                        int xy= (mb_x*2 + i + mb_y*2*mv_stride) << (mv_sample_log2-1);
                        int mx=(pict->motion_val[direction][xy][0]>>shift);
                        int my=(pict->motion_val[direction][xy][1]>>shift);

                        if(IS_INTERLACED(pict->mb_type[mb_index]))
                            my*=2;

                        draw_arrow(ptr, sx, sy, mx+sx, my+sy, width, height, s->linesize, 100);
                      }
                    }else{
                      int sx= mb_x*16 + 8;
                      int sy= mb_y*16 + 8;
                      int xy= (mb_x + mb_y*mv_stride) << mv_sample_log2;
                      int mx= (pict->motion_val[direction][xy][0]>>shift) + sx;
                      int my= (pict->motion_val[direction][xy][1]>>shift) + sy;
                      draw_arrow(ptr, sx, sy, mx, my, width, height, s->linesize, 100);
                    }
                  }
                }
                if((s->avctx->debug&FF_DEBUG_VIS_QP) && pict->motion_val){
                    uint64_t c= (pict->qscale_table[mb_index]*128/31) * 0x0101010101010101ULL;
                    int y;
                    for(y=0; y<block_height; y++){
                        *(uint64_t*)(pict->data[1] + 8*mb_x + (block_height*mb_y + y)*pict->linesize[1])= c;
                        *(uint64_t*)(pict->data[2] + 8*mb_x + (block_height*mb_y + y)*pict->linesize[2])= c;
                    }
                }
                if((s->avctx->debug&FF_DEBUG_VIS_MB_TYPE) && pict->motion_val){
                    int mb_type= pict->mb_type[mb_index];
                    uint64_t u,v;
                    int y;
#define COLOR(theta, r)\
u= (int)(128 + r*cos(theta*3.141592/180));\
v= (int)(128 + r*sin(theta*3.141592/180));


                    u=v=128;
                    if(IS_PCM(mb_type)){
                        COLOR(120,48)
                    }else if((IS_INTRA(mb_type) && IS_ACPRED(mb_type)) || IS_INTRA16x16(mb_type)){
                        COLOR(30,48)
                    }else if(IS_INTRA4x4(mb_type)){
                        COLOR(90,48)
                    }else if(IS_DIRECT(mb_type) && IS_SKIP(mb_type)){
//                        COLOR(120,48)
                    }else if(IS_DIRECT(mb_type)){
                        COLOR(150,48)
                    }else if(IS_GMC(mb_type) && IS_SKIP(mb_type)){
                        COLOR(170,48)
                    }else if(IS_GMC(mb_type)){
                        COLOR(190,48)
                    }else if(IS_SKIP(mb_type)){
//                        COLOR(180,48)
                    }else if(!USES_LIST(mb_type, 1)){
                        COLOR(240,48)
                    }else if(!USES_LIST(mb_type, 0)){
                        COLOR(0,48)
                    }else{
                        assert(USES_LIST(mb_type, 0) && USES_LIST(mb_type, 1));
                        COLOR(300,48)
                    }

                    u*= 0x0101010101010101ULL;
                    v*= 0x0101010101010101ULL;
                    for(y=0; y<block_height; y++){
                        *(uint64_t*)(pict->data[1] + 8*mb_x + (block_height*mb_y + y)*pict->linesize[1])= u;
                        *(uint64_t*)(pict->data[2] + 8*mb_x + (block_height*mb_y + y)*pict->linesize[2])= v;
                    }

                    //segmentation
                    if(IS_8X8(mb_type) || IS_16X8(mb_type)){
                        *(uint64_t*)(pict->data[0] + 16*mb_x + 0 + (16*mb_y + 8)*pict->linesize[0])^= 0x8080808080808080ULL;
                        *(uint64_t*)(pict->data[0] + 16*mb_x + 8 + (16*mb_y + 8)*pict->linesize[0])^= 0x8080808080808080ULL;
                    }
                    if(IS_8X8(mb_type) || IS_8X16(mb_type)){
                        for(y=0; y<16; y++)
                            pict->data[0][16*mb_x + 8 + (16*mb_y + y)*pict->linesize[0]]^= 0x80;
                    }
                    if(IS_8X8(mb_type) && mv_sample_log2 >= 2){
                        int dm= 1 << (mv_sample_log2-2);
                        for(i=0; i<4; i++){
                            int sx= mb_x*16 + 8*(i&1);
                            int sy= mb_y*16 + 8*(i>>1);
                            int xy= (mb_x*2 + (i&1) + (mb_y*2 + (i>>1))*mv_stride) << (mv_sample_log2-1);
                            //FIXME bidir
                            int32_t *mv = (int32_t*)&pict->motion_val[0][xy];
                            if(mv[0] != mv[dm] || mv[dm*mv_stride] != mv[dm*(mv_stride+1)])
                                for(y=0; y<8; y++)
                                    pict->data[0][sx + 4 + (sy + y)*pict->linesize[0]]^= 0x80;
                            if(mv[0] != mv[dm*mv_stride] || mv[dm] != mv[dm*(mv_stride+1)])
                                *(uint64_t*)(pict->data[0] + sx + (sy + 4)*pict->linesize[0])^= 0x8080808080808080ULL;
                        }
                    }

                    if(IS_INTERLACED(mb_type) && s->codec_id == CODEC_ID_H264){
                        // hmm
                    }
                }
                s->mbskip_table[mb_index]=0;
            }
        }
    }
}

static inline int hpel_motion_lowres(MpegEncContext *s,
                                  uint8_t *dest, uint8_t *src,
                                  int field_based, int field_select,
                                  int src_x, int src_y,
                                  int width, int height, int stride,
                                  int h_edge_pos, int v_edge_pos,
                                  int w, int h, h264_chroma_mc_func *pix_op,
                                  int motion_x, int motion_y)
{
    const int lowres= s->avctx->lowres;
    const int op_index= FFMIN(lowres, 2);
    const int s_mask= (2<<lowres)-1;
    int emu=0;
    int sx, sy;

    if(s->quarter_sample){
        motion_x/=2;
        motion_y/=2;
    }

    sx= motion_x & s_mask;
    sy= motion_y & s_mask;
    src_x += motion_x >> (lowres+1);
    src_y += motion_y >> (lowres+1);

    src += src_y * stride + src_x;

    if(   (unsigned)src_x > h_edge_pos                 - (!!sx) - w
       || (unsigned)src_y >(v_edge_pos >> field_based) - (!!sy) - h){
        s->dsp.emulated_edge_mc(s->edge_emu_buffer, src, s->linesize, w+1, (h+1)<<field_based,
                            src_x, src_y<<field_based, h_edge_pos, v_edge_pos);
        src= s->edge_emu_buffer;
        emu=1;
    }

    sx= (sx << 2) >> lowres;
    sy= (sy << 2) >> lowres;
    if(field_select)
        src += s->linesize;
    pix_op[op_index](dest, src, stride, h, sx, sy);
    return emu;
}

/* apply one mpeg motion vector to the three components */
static av_always_inline void mpeg_motion_lowres(MpegEncContext *s,
                               uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,
                               int field_based, int bottom_field, int field_select,
                               uint8_t **ref_picture, h264_chroma_mc_func *pix_op,
                               int motion_x, int motion_y, int h, int mb_y)
{
    uint8_t *ptr_y, *ptr_cb, *ptr_cr;
    int mx, my, src_x, src_y, uvsrc_x, uvsrc_y, uvlinesize, linesize, sx, sy, uvsx, uvsy;
    const int lowres= s->avctx->lowres;
    const int op_index= FFMIN(lowres, 2);
    const int block_s= 8>>lowres;
    const int s_mask= (2<<lowres)-1;
    const int h_edge_pos = s->h_edge_pos >> lowres;
    const int v_edge_pos = s->v_edge_pos >> lowres;
    linesize   = s->current_picture.linesize[0] << field_based;
    uvlinesize = s->current_picture.linesize[1] << field_based;

    if(s->quarter_sample){ //FIXME obviously not perfect but qpel will not work in lowres anyway
        motion_x/=2;
        motion_y/=2;
    }

    if(field_based){
        motion_y += (bottom_field - field_select)*((1<<lowres)-1);
    }

    sx= motion_x & s_mask;
    sy= motion_y & s_mask;
    src_x = s->mb_x*2*block_s               + (motion_x >> (lowres+1));
    src_y =(   mb_y*2*block_s>>field_based) + (motion_y >> (lowres+1));

    if (s->out_format == FMT_H263) {
        uvsx = ((motion_x>>1) & s_mask) | (sx&1);
        uvsy = ((motion_y>>1) & s_mask) | (sy&1);
        uvsrc_x = src_x>>1;
        uvsrc_y = src_y>>1;
    }else if(s->out_format == FMT_H261){//even chroma mv's are full pel in H261
        mx = motion_x / 4;
        my = motion_y / 4;
        uvsx = (2*mx) & s_mask;
        uvsy = (2*my) & s_mask;
        uvsrc_x = s->mb_x*block_s               + (mx >> lowres);
        uvsrc_y =    mb_y*block_s               + (my >> lowres);
    } else {
        mx = motion_x / 2;
        my = motion_y / 2;
        uvsx = mx & s_mask;
        uvsy = my & s_mask;
        uvsrc_x = s->mb_x*block_s               + (mx >> (lowres+1));
        uvsrc_y =(   mb_y*block_s>>field_based) + (my >> (lowres+1));
    }

    ptr_y  = ref_picture[0] + src_y * linesize + src_x;
    ptr_cb = ref_picture[1] + uvsrc_y * uvlinesize + uvsrc_x;
    ptr_cr = ref_picture[2] + uvsrc_y * uvlinesize + uvsrc_x;

    if(   (unsigned)src_x > h_edge_pos                 - (!!sx) - 2*block_s
       || (unsigned)src_y >(v_edge_pos >> field_based) - (!!sy) - h){
            s->dsp.emulated_edge_mc(s->edge_emu_buffer, ptr_y, s->linesize, 17, 17+field_based,
                             src_x, src_y<<field_based, h_edge_pos, v_edge_pos);
            ptr_y = s->edge_emu_buffer;
            if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                uint8_t *uvbuf= s->edge_emu_buffer+18*s->linesize;
                s->dsp.emulated_edge_mc(uvbuf  , ptr_cb, s->uvlinesize, 9, 9+field_based,
                                 uvsrc_x, uvsrc_y<<field_based, h_edge_pos>>1, v_edge_pos>>1);
                s->dsp.emulated_edge_mc(uvbuf+16, ptr_cr, s->uvlinesize, 9, 9+field_based,
                                 uvsrc_x, uvsrc_y<<field_based, h_edge_pos>>1, v_edge_pos>>1);
                ptr_cb= uvbuf;
                ptr_cr= uvbuf+16;
            }
    }

    if(bottom_field){ //FIXME use this for field pix too instead of the obnoxious hack which changes picture.data
        dest_y += s->linesize;
        dest_cb+= s->uvlinesize;
        dest_cr+= s->uvlinesize;
    }

    if(field_select){
        ptr_y += s->linesize;
        ptr_cb+= s->uvlinesize;
        ptr_cr+= s->uvlinesize;
    }

    sx= (sx << 2) >> lowres;
    sy= (sy << 2) >> lowres;
    pix_op[lowres-1](dest_y, ptr_y, linesize, h, sx, sy);

    if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
        uvsx= (uvsx << 2) >> lowres;
        uvsy= (uvsy << 2) >> lowres;
        pix_op[op_index](dest_cb, ptr_cb, uvlinesize, h >> s->chroma_y_shift, uvsx, uvsy);
        pix_op[op_index](dest_cr, ptr_cr, uvlinesize, h >> s->chroma_y_shift, uvsx, uvsy);
    }
    //FIXME h261 lowres loop filter
}

static inline void chroma_4mv_motion_lowres(MpegEncContext *s,
                                     uint8_t *dest_cb, uint8_t *dest_cr,
                                     uint8_t **ref_picture,
                                     h264_chroma_mc_func *pix_op,
                                     int mx, int my){
    const int lowres= s->avctx->lowres;
    const int op_index= FFMIN(lowres, 2);
    const int block_s= 8>>lowres;
    const int s_mask= (2<<lowres)-1;
    const int h_edge_pos = s->h_edge_pos >> (lowres+1);
    const int v_edge_pos = s->v_edge_pos >> (lowres+1);
    int emu=0, src_x, src_y, offset, sx, sy;
    uint8_t *ptr;

    if(s->quarter_sample){
        mx/=2;
        my/=2;
    }

    /* In case of 8X8, we construct a single chroma motion vector
       with a special rounding */
    mx= ff_h263_round_chroma(mx);
    my= ff_h263_round_chroma(my);

    sx= mx & s_mask;
    sy= my & s_mask;
    src_x = s->mb_x*block_s + (mx >> (lowres+1));
    src_y = s->mb_y*block_s + (my >> (lowres+1));

    offset = src_y * s->uvlinesize + src_x;
    ptr = ref_picture[1] + offset;
    if(s->flags&CODEC_FLAG_EMU_EDGE){
        if(   (unsigned)src_x > h_edge_pos - (!!sx) - block_s
           || (unsigned)src_y > v_edge_pos - (!!sy) - block_s){
            s->dsp.emulated_edge_mc(s->edge_emu_buffer, ptr, s->uvlinesize, 9, 9, src_x, src_y, h_edge_pos, v_edge_pos);
            ptr= s->edge_emu_buffer;
            emu=1;
        }
    }
    sx= (sx << 2) >> lowres;
    sy= (sy << 2) >> lowres;
    pix_op[op_index](dest_cb, ptr, s->uvlinesize, block_s, sx, sy);

    ptr = ref_picture[2] + offset;
    if(emu){
        s->dsp.emulated_edge_mc(s->edge_emu_buffer, ptr, s->uvlinesize, 9, 9, src_x, src_y, h_edge_pos, v_edge_pos);
        ptr= s->edge_emu_buffer;
    }
    pix_op[op_index](dest_cr, ptr, s->uvlinesize, block_s, sx, sy);
}

/**
 * motion compensation of a single macroblock
 * @param s context
 * @param dest_y luma destination pointer
 * @param dest_cb chroma cb/u destination pointer
 * @param dest_cr chroma cr/v destination pointer
 * @param dir direction (0->forward, 1->backward)
 * @param ref_picture array[3] of pointers to the 3 planes of the reference picture
 * @param pix_op halfpel motion compensation function (average or put normally)
 * the motion vectors are taken from s->mv and the MV type from s->mv_type
 */
static inline void MPV_motion_lowres(MpegEncContext *s,
                              uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,
                              int dir, uint8_t **ref_picture,
                              h264_chroma_mc_func *pix_op)
{
    int mx, my;
    int mb_x, mb_y, i;
    const int lowres= s->avctx->lowres;
    const int block_s= 8>>lowres;

    mb_x = s->mb_x;
    mb_y = s->mb_y;

    switch(s->mv_type) {
    case MV_TYPE_16X16:
        mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,
                    0, 0, 0,
                    ref_picture, pix_op,
                    s->mv[dir][0][0], s->mv[dir][0][1], 2*block_s, mb_y);
        break;
    case MV_TYPE_8X8:
        mx = 0;
        my = 0;
            for(i=0;i<4;i++) {
                hpel_motion_lowres(s, dest_y + ((i & 1) + (i >> 1) * s->linesize)*block_s,
                            ref_picture[0], 0, 0,
                            (2*mb_x + (i & 1))*block_s, (2*mb_y + (i >>1))*block_s,
                            s->width, s->height, s->linesize,
                            s->h_edge_pos >> lowres, s->v_edge_pos >> lowres,
                            block_s, block_s, pix_op,
                            s->mv[dir][i][0], s->mv[dir][i][1]);

                mx += s->mv[dir][i][0];
                my += s->mv[dir][i][1];
            }

        if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY))
            chroma_4mv_motion_lowres(s, dest_cb, dest_cr, ref_picture, pix_op, mx, my);
        break;
    case MV_TYPE_FIELD:
        if (s->picture_structure == PICT_FRAME) {
            /* top field */
            mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,
                        1, 0, s->field_select[dir][0],
                        ref_picture, pix_op,
                        s->mv[dir][0][0], s->mv[dir][0][1], block_s, mb_y);
            /* bottom field */
            mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,
                        1, 1, s->field_select[dir][1],
                        ref_picture, pix_op,
                        s->mv[dir][1][0], s->mv[dir][1][1], block_s, mb_y);
        } else {
            if(s->picture_structure != s->field_select[dir][0] + 1 && s->pict_type != FF_B_TYPE && !s->first_field){
                ref_picture= s->current_picture_ptr->data;
            }

            mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,
                        0, 0, s->field_select[dir][0],
                        ref_picture, pix_op,
                        s->mv[dir][0][0], s->mv[dir][0][1], 2*block_s, mb_y>>1);
        }
        break;
    case MV_TYPE_16X8:
        for(i=0; i<2; i++){
            uint8_t ** ref2picture;

            if(s->picture_structure == s->field_select[dir][i] + 1 || s->pict_type == FF_B_TYPE || s->first_field){
                ref2picture= ref_picture;
            }else{
                ref2picture= s->current_picture_ptr->data;
            }

            mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,
                        0, 0, s->field_select[dir][i],
                        ref2picture, pix_op,
                        s->mv[dir][i][0], s->mv[dir][i][1] + 2*block_s*i, block_s, mb_y>>1);

            dest_y += 2*block_s*s->linesize;
            dest_cb+= (2*block_s>>s->chroma_y_shift)*s->uvlinesize;
            dest_cr+= (2*block_s>>s->chroma_y_shift)*s->uvlinesize;
        }
        break;
    case MV_TYPE_DMV:
        if(s->picture_structure == PICT_FRAME){
            for(i=0; i<2; i++){
                int j;
                for(j=0; j<2; j++){
                    mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,
                                1, j, j^i,
                                ref_picture, pix_op,
                                s->mv[dir][2*i + j][0], s->mv[dir][2*i + j][1], block_s, mb_y);
                }
                pix_op = s->dsp.avg_h264_chroma_pixels_tab;
            }
        }else{
            for(i=0; i<2; i++){
                mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,
                            0, 0, s->picture_structure != i+1,
                            ref_picture, pix_op,
                            s->mv[dir][2*i][0],s->mv[dir][2*i][1],2*block_s, mb_y>>1);

                // after put we make avg of the same block
                pix_op = s->dsp.avg_h264_chroma_pixels_tab;

                //opposite parity is always in the same frame if this is second field
                if(!s->first_field){
                    ref_picture = s->current_picture_ptr->data;
                }
            }
        }
    break;
    default: assert(0);
    }
}

/**
 * find the lowest MB row referenced in the MVs
 */
int MPV_lowest_referenced_row(MpegEncContext *s, int dir)
{
    int my_max = INT_MIN, my_min = INT_MAX, qpel_shift = !s->quarter_sample;
    int my, off, i, mvs;

    if (s->picture_structure != PICT_FRAME) goto unhandled;

    switch (s->mv_type) {
        case MV_TYPE_16X16:
            mvs = 1;
            break;
        case MV_TYPE_16X8:
            mvs = 2;
            break;
        case MV_TYPE_8X8:
            mvs = 4;
            break;
        default:
            goto unhandled;
    }

    for (i = 0; i < mvs; i++) {
        my = s->mv[dir][i][1]<<qpel_shift;
        my_max = FFMAX(my_max, my);
        my_min = FFMIN(my_min, my);
    }

    off = (FFMAX(-my_min, my_max) + 63) >> 6;

    return FFMIN(FFMAX(s->mb_y + off, 0), s->mb_height-1);
unhandled:
    return s->mb_height-1;
}

/* put block[] to dest[] */
static inline void put_dct(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int qscale)
{
    s->dct_unquantize_intra(s, block, i, qscale);
    #ifdef __dump_DSP_TXT__
    memcpy(s->logdct_deqaun[i],block,128);
    memcpy(s->logdct_tmp,block,128);
    s->dsp.idct_put(s->logdct_result[i],8,s->logdct_tmp);
    #endif
    s->dsp.idct_put (dest, line_size, block);
    #ifdef __dump_DSP_TXT__
    int indexy=0,indexx;
    for(indexy=0;indexy<8;indexy++,dest+=line_size)
    {
        for(indexx=0;indexx<8;indexx++)
            s->logdctmc_result[i][indexy*8+indexx]=dest[indexx];
    }
    #endif

}

/* add block[] to dest[] */
static inline void add_dct(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size)
{
    if (s->block_last_index[i] >= 0) {
        #ifdef __dump_DSP_TXT__
        memcpy(s->logdct_deqaun[i],block,128);
        memcpy(s->logdct_tmp,block,128);
        s->dsp.idct_put(s->logdct_result[i],8,s->logdct_tmp);
        #endif
        s->dsp.idct_add (dest, line_size, block);
    }
    #ifdef __dump_DSP_TXT__
    int indexy=0,indexx;
    for(indexy=0;indexy<8;indexy++,dest+=line_size)
    {
        for(indexx=0;indexx<8;indexx++)
            s->logdctmc_result[i][indexy*8+indexx]=dest[indexx];
    }
    #endif
}

static inline void add_dequant_dct(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int qscale)
{
    if (s->block_last_index[i] >= 0) {
        s->dct_unquantize_inter(s, block, i, qscale);
        #ifdef __dump_DSP_TXT__
        memcpy(s->logdct_deqaun[i],block,128);
        memcpy(s->logdct_tmp,block,128);
        s->dsp.idct_put(s->logdct_result[i],8,s->logdct_tmp);
        #endif
        s->dsp.idct_add (dest, line_size, block);
    }

    #ifdef __dump_DSP_TXT__
    int indexy=0,indexx;
    for(indexy=0;indexy<8;indexy++,dest+=line_size)
    {
        for(indexx=0;indexx<8;indexx++)
            s->logdctmc_result[i][indexy*8+indexx]=dest[indexx];
    }
    #endif
}

/**
 * cleans dc, ac, coded_block for the current non intra MB
 */
void ff_clean_intra_table_entries(MpegEncContext *s)
{
    int wrap = s->b8_stride;
    int xy = s->block_index[0];

    s->dc_val[0][xy           ] =
    s->dc_val[0][xy + 1       ] =
    s->dc_val[0][xy     + wrap] =
    s->dc_val[0][xy + 1 + wrap] = 1024;
    /* ac pred */
    memset(s->ac_val[0][xy       ], 0, 32 * sizeof(int16_t));
    memset(s->ac_val[0][xy + wrap], 0, 32 * sizeof(int16_t));
    if (s->msmpeg4_version>=3) {
        s->coded_block[xy           ] =
        s->coded_block[xy + 1       ] =
        s->coded_block[xy     + wrap] =
        s->coded_block[xy + 1 + wrap] = 0;
    }
    /* chroma */
    wrap = s->mb_stride;
    xy = s->mb_x + s->mb_y * wrap;
    s->dc_val[1][xy] =
    s->dc_val[2][xy] = 1024;
    /* ac pred */
    memset(s->ac_val[1][xy], 0, 16 * sizeof(int16_t));
    memset(s->ac_val[2][xy], 0, 16 * sizeof(int16_t));

    s->mbintra_table[xy]= 0;
}

/* generic function called after a macroblock has been parsed by the
   decoder or after it has been encoded by the encoder.

   Important variables used:
   s->mb_intra : true if intra macroblock
   s->mv_dir   : motion vector direction
   s->mv_type  : motion vector type
   s->mv       : motion vector
   s->interlaced_dct : true if interlaced dct used (mpeg2)
 */
static av_always_inline
void MPV_decode_mb_internal(MpegEncContext *s, DCTELEM block[12][64],
                            int lowres_flag, int is_mpeg12)
{
    const int mb_xy = s->mb_y * s->mb_stride + s->mb_x;
    if(CONFIG_MPEG_XVMC_DECODER && s->avctx->xvmc_acceleration){
        ff_xvmc_decode_mb(s);//xvmc uses pblocks
        return;
    }

    if(s->avctx->debug&FF_DEBUG_DCT_COEFF) {
       /* save DCT coefficients */
       int i,j;
       DCTELEM *dct = &s->current_picture.dct_coeff[mb_xy*64*6];
       av_log(s->avctx, AV_LOG_DEBUG, "DCT coeffs of MB at %dx%d:\n", s->mb_x, s->mb_y);
       for(i=0; i<6; i++){
           for(j=0; j<64; j++){
               *dct++ = block[i][s->dsp.idct_permutation[j]];
               av_log(s->avctx, AV_LOG_DEBUG, "%5d", dct[-1]);
           }
           av_log(s->avctx, AV_LOG_DEBUG, "\n");
       }
    }

    s->current_picture.qscale_table[mb_xy]= s->qscale;

    /* update DC predictors for P macroblocks */
    if (!s->mb_intra) {
        if (!is_mpeg12 && (s->h263_pred || s->h263_aic)) {
            if(s->mbintra_table[mb_xy])
                ff_clean_intra_table_entries(s);
        } else {
            s->last_dc[0] =
            s->last_dc[1] =
            s->last_dc[2] = 128 << s->intra_dc_precision;
        }
    }
    else if (!is_mpeg12 && (s->h263_pred || s->h263_aic))
        s->mbintra_table[mb_xy]=1;

    if ((s->flags&CODEC_FLAG_PSNR) || !(s->encoding && (s->intra_only || s->pict_type==FF_B_TYPE) && s->avctx->mb_decision != FF_MB_DECISION_RD)) { //FIXME precalc
        uint8_t *dest_y, *dest_cb, *dest_cr;
        int dct_linesize, dct_offset;
        op_pixels_func (*op_pix)[4];
        qpel_mc_func (*op_qpix)[16];
        const int linesize= s->current_picture.linesize[0]; //not s->linesize as this would be wrong for field pics
        const int uvlinesize= s->current_picture.linesize[1];
        const int readable= s->pict_type != FF_B_TYPE || s->encoding || s->avctx->draw_horiz_band || lowres_flag;
        const int block_size= lowres_flag ? 8>>s->avctx->lowres : 8;

        //ambadec_assert_ffmpeg(readable);
        #if 0 //__dump_DSP_test_data__
            ambadec_assert_ffmpeg(s->current_picture_ptr->linesize[0]==s->linesize);
            ambadec_assert_ffmpeg(s->current_picture_ptr->linesize[1]==s->uvlinesize);

            uint8_t* pcheck=s->current_picture_ptr->data[0]+(s->mb_y*16)*s->current_picture_ptr->linesize[0]+(s->mb_x*16);
            ambadec_assert_ffmpeg(s->dest[0]==pcheck);
            pcheck=s->current_picture_ptr->data[1]+(s->mb_y*8)*s->current_picture_ptr->linesize[1]+(s->mb_x*8);
            ambadec_assert_ffmpeg(s->dest[1]==pcheck);
            pcheck=s->current_picture_ptr->data[2]+(s->mb_y*8)*s->current_picture_ptr->linesize[1]+(s->mb_x*8);
            ambadec_assert_ffmpeg(s->dest[2]==pcheck);
        #endif

        /* avoid copy if macroblock skipped in last frame too */
        /* skip only during decoding as we might trash the buffers during encoding a bit */
        if(!s->encoding){
            uint8_t *mbskip_ptr = &s->mbskip_table[mb_xy];
            const int age= s->current_picture.age;

            assert(age);

            if (s->mb_skipped) {
                s->mb_skipped= 0;
                assert(s->pict_type!=FF_I_TYPE);

                (*mbskip_ptr) ++; /* indicate that this time we skipped it */
                if(*mbskip_ptr >99) *mbskip_ptr= 99;

                /* if previous was skipped too, then nothing to do !  */
                if (*mbskip_ptr >= age && s->current_picture.reference){
                    return;
                }
            } else if(!s->current_picture.reference){
                (*mbskip_ptr) ++; /* increase counter so the age can be compared cleanly */
                if(*mbskip_ptr >99) *mbskip_ptr= 99;
            } else{
                *mbskip_ptr = 0; /* not skipped */
            }
        }

        dct_linesize = linesize << s->interlaced_dct;
        dct_offset =(s->interlaced_dct)? linesize : linesize*block_size;

        if(readable){
            dest_y=  s->dest[0];
            dest_cb= s->dest[1];
            dest_cr= s->dest[2];
        }else{
            dest_y = s->b_scratchpad;
            dest_cb= s->b_scratchpad+16*linesize;
            dest_cr= s->b_scratchpad+32*linesize;
        }

        if (!s->mb_intra) {
            /* motion handling */
            /* decoding or more than one mb_type (MC was already done otherwise) */
            if(!s->encoding){

                if(HAVE_PTHREADS && s->avctx->active_thread_type&FF_THREAD_FRAME) {
                    if (s->mv_dir & MV_DIR_FORWARD) {
                        ff_thread_await_progress((AVFrame*)s->last_picture_ptr, MPV_lowest_referenced_row(s, 0), 0);
                    }
                    if (s->mv_dir & MV_DIR_BACKWARD) {
                        ff_thread_await_progress((AVFrame*)s->next_picture_ptr, MPV_lowest_referenced_row(s, 1), 0);
                    }
                }

                if(lowres_flag){
                    h264_chroma_mc_func *op_pix = s->dsp.put_h264_chroma_pixels_tab;

                    if (s->mv_dir & MV_DIR_FORWARD) {
                        MPV_motion_lowres(s, dest_y, dest_cb, dest_cr, 0, s->last_picture.data, op_pix);
                        op_pix = s->dsp.avg_h264_chroma_pixels_tab;
                    }
                    if (s->mv_dir & MV_DIR_BACKWARD) {
                        MPV_motion_lowres(s, dest_y, dest_cb, dest_cr, 1, s->next_picture.data, op_pix);
                    }
                }else{
                    op_qpix= s->me.qpel_put;
                    if ((!s->no_rounding) || s->pict_type==FF_B_TYPE){
                        op_pix = s->dsp.put_pixels_tab;
                        #ifdef __dump_DSP_TXT__
                            log_text(log_fd_dsp_text, "mc with no rounding");
                        #endif
                    }else{
                        op_pix = s->dsp.put_no_rnd_pixels_tab;
                        #ifdef __dump_DSP_TXT__
                            log_text(log_fd_dsp_text, "mc with rounding");
                        #endif
                    }
                    if (s->mv_dir & MV_DIR_FORWARD) {
                        MPV_motion(s, dest_y, dest_cb, dest_cr, 0, s->last_picture.data, op_pix, op_qpix);
                        op_pix = s->dsp.avg_pixels_tab;
                        op_qpix= s->me.qpel_avg;

                        #ifdef __dump_DSP_TXT__
                        log_text(log_fd_dsp_text, "After Forward MC, result is:");
                        log_text(log_fd_dsp_text, "Y:");
                        log_text_rect_char_hex(log_fd_dsp_text,dest_y,16,16,s->linesize-16);
                        log_text(log_fd_dsp_text, "Cb:");
                        log_text_rect_char_hex(log_fd_dsp_text,dest_cb,8,8,s->uvlinesize-8);
                        log_text(log_fd_dsp_text, "Cr:");
                        log_text_rect_char_hex(log_fd_dsp_text,dest_cr,8,8,s->uvlinesize-8);
                        #endif

                    }
                    if (s->mv_dir & MV_DIR_BACKWARD) {
                        MPV_motion(s, dest_y, dest_cb, dest_cr, 1, s->next_picture.data, op_pix, op_qpix);

                        #ifdef __dump_DSP_TXT__
                        log_text(log_fd_dsp_text, "After Backward MC, result is:");
                        log_text(log_fd_dsp_text, "Y:");
                        log_text_rect_char_hex(log_fd_dsp_text,dest_y,16,16,s->linesize-16);
                        log_text(log_fd_dsp_text, "Cb:");
                        log_text_rect_char_hex(log_fd_dsp_text,dest_cb,8,8,s->uvlinesize-8);
                        log_text(log_fd_dsp_text, "Cr:");
                        log_text_rect_char_hex(log_fd_dsp_text,dest_cr,8,8,s->uvlinesize-8);
                        #endif

                    }
                }
            }

            /* skip dequant / idct if we are really late ;) */
#if FF_API_HURRY_UP
            if(s->hurry_up>1) goto skip_idct;
#endif
            if(s->avctx->skip_idct){
                if(  (s->avctx->skip_idct >= AVDISCARD_NONREF && s->pict_type == FF_B_TYPE)
                   ||(s->avctx->skip_idct >= AVDISCARD_NONKEY && s->pict_type != FF_I_TYPE)
                   || s->avctx->skip_idct >= AVDISCARD_ALL)
                    goto skip_idct;
            }

            /* add dct residue */
            if(s->encoding || !(   s->h263_msmpeg4 || s->codec_id==CODEC_ID_MPEG1VIDEO || s->codec_id==CODEC_ID_MPEG2VIDEO
                                || (s->codec_id==CODEC_ID_MPEG4 && !s->mpeg_quant))){
                add_dequant_dct(s, block[0], 0, dest_y                          , dct_linesize, s->qscale);
                add_dequant_dct(s, block[1], 1, dest_y              + block_size, dct_linesize, s->qscale);
                add_dequant_dct(s, block[2], 2, dest_y + dct_offset             , dct_linesize, s->qscale);
                add_dequant_dct(s, block[3], 3, dest_y + dct_offset + block_size, dct_linesize, s->qscale);

                if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                    if (s->chroma_y_shift){
                        add_dequant_dct(s, block[4], 4, dest_cb, uvlinesize, s->chroma_qscale);
                        add_dequant_dct(s, block[5], 5, dest_cr, uvlinesize, s->chroma_qscale);
                    }else{
                        dct_linesize >>= 1;
                        dct_offset >>=1;
                        add_dequant_dct(s, block[4], 4, dest_cb,              dct_linesize, s->chroma_qscale);
                        add_dequant_dct(s, block[5], 5, dest_cr,              dct_linesize, s->chroma_qscale);
                        add_dequant_dct(s, block[6], 6, dest_cb + dct_offset, dct_linesize, s->chroma_qscale);
                        add_dequant_dct(s, block[7], 7, dest_cr + dct_offset, dct_linesize, s->chroma_qscale);
                    }
                }
            } else if(is_mpeg12 || (s->codec_id != CODEC_ID_WMV2)){
                add_dct(s, block[0], 0, dest_y                          , dct_linesize);
                add_dct(s, block[1], 1, dest_y              + block_size, dct_linesize);
                add_dct(s, block[2], 2, dest_y + dct_offset             , dct_linesize);
                add_dct(s, block[3], 3, dest_y + dct_offset + block_size, dct_linesize);

                if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                    if(s->chroma_y_shift){//Chroma420
                        add_dct(s, block[4], 4, dest_cb, uvlinesize);
                        add_dct(s, block[5], 5, dest_cr, uvlinesize);
                    }else{
                        //chroma422
                        dct_linesize = uvlinesize << s->interlaced_dct;
                        dct_offset =(s->interlaced_dct)? uvlinesize : uvlinesize*8;

                        add_dct(s, block[4], 4, dest_cb, dct_linesize);
                        add_dct(s, block[5], 5, dest_cr, dct_linesize);
                        add_dct(s, block[6], 6, dest_cb+dct_offset, dct_linesize);
                        add_dct(s, block[7], 7, dest_cr+dct_offset, dct_linesize);
                        if(!s->chroma_x_shift){//Chroma444
                            add_dct(s, block[8], 8, dest_cb+8, dct_linesize);
                            add_dct(s, block[9], 9, dest_cr+8, dct_linesize);
                            add_dct(s, block[10], 10, dest_cb+8+dct_offset, dct_linesize);
                            add_dct(s, block[11], 11, dest_cr+8+dct_offset, dct_linesize);
                        }
                    }
                }//fi gray
            }
            else if (CONFIG_WMV2_DECODER || CONFIG_WMV2_ENCODER) {
                ff_wmv2_add_mb(s, block, dest_y, dest_cb, dest_cr);
            }
        } else {
            /* dct only in intra block */
            if(s->encoding || !(s->codec_id==CODEC_ID_MPEG1VIDEO || s->codec_id==CODEC_ID_MPEG2VIDEO)){
                put_dct(s, block[0], 0, dest_y                          , dct_linesize, s->qscale);
                put_dct(s, block[1], 1, dest_y              + block_size, dct_linesize, s->qscale);
                put_dct(s, block[2], 2, dest_y + dct_offset             , dct_linesize, s->qscale);
                put_dct(s, block[3], 3, dest_y + dct_offset + block_size, dct_linesize, s->qscale);

                if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                    if(s->chroma_y_shift){
                        put_dct(s, block[4], 4, dest_cb, uvlinesize, s->chroma_qscale);
                        put_dct(s, block[5], 5, dest_cr, uvlinesize, s->chroma_qscale);
                    }else{
                        dct_offset >>=1;
                        dct_linesize >>=1;
                        put_dct(s, block[4], 4, dest_cb,              dct_linesize, s->chroma_qscale);
                        put_dct(s, block[5], 5, dest_cr,              dct_linesize, s->chroma_qscale);
                        put_dct(s, block[6], 6, dest_cb + dct_offset, dct_linesize, s->chroma_qscale);
                        put_dct(s, block[7], 7, dest_cr + dct_offset, dct_linesize, s->chroma_qscale);
                    }
                }
            }else{
                s->dsp.idct_put(dest_y                          , dct_linesize, block[0]);
                s->dsp.idct_put(dest_y              + block_size, dct_linesize, block[1]);
                s->dsp.idct_put(dest_y + dct_offset             , dct_linesize, block[2]);
                s->dsp.idct_put(dest_y + dct_offset + block_size, dct_linesize, block[3]);

                if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                    if(s->chroma_y_shift){
                        s->dsp.idct_put(dest_cb, uvlinesize, block[4]);
                        s->dsp.idct_put(dest_cr, uvlinesize, block[5]);
                    }else{

                        dct_linesize = uvlinesize << s->interlaced_dct;
                        dct_offset =(s->interlaced_dct)? uvlinesize : uvlinesize*8;

                        s->dsp.idct_put(dest_cb,              dct_linesize, block[4]);
                        s->dsp.idct_put(dest_cr,              dct_linesize, block[5]);
                        s->dsp.idct_put(dest_cb + dct_offset, dct_linesize, block[6]);
                        s->dsp.idct_put(dest_cr + dct_offset, dct_linesize, block[7]);
                        if(!s->chroma_x_shift){//Chroma444
                            s->dsp.idct_put(dest_cb + 8,              dct_linesize, block[8]);
                            s->dsp.idct_put(dest_cr + 8,              dct_linesize, block[9]);
                            s->dsp.idct_put(dest_cb + 8 + dct_offset, dct_linesize, block[10]);
                            s->dsp.idct_put(dest_cr + 8 + dct_offset, dct_linesize, block[11]);
                        }
                    }
                }//gray
            }
        }
skip_idct:
        if(!readable){
            s->dsp.put_pixels_tab[0][0](s->dest[0], dest_y ,   linesize,16);
            s->dsp.put_pixels_tab[s->chroma_x_shift][0](s->dest[1], dest_cb, uvlinesize,16 >> s->chroma_y_shift);
            s->dsp.put_pixels_tab[s->chroma_x_shift][0](s->dest[2], dest_cr, uvlinesize,16 >> s->chroma_y_shift);
        }
    }
#ifdef __dump_DSP_TXT__
int logi=0;
char logt[80];
for(logi=0;logi<6;logi++)
{
    snprintf(logt,79,"{ SubBlock: %d }",logi);
    log_text(log_fd_dsp_text, logt);
    log_text(log_fd_dsp_text,"<< Before Inverse Quantization: >>");
    log_text_rect_short(log_fd_dsp_text,s->logdct[logi],8,8,0);
    log_text(log_fd_dsp_text,"<< After Inverse DCAC Prediction: >>");
    log_text_rect_short(log_fd_dsp_text,s->logdct_acdc[logi],8,8,0);
    log_text(log_fd_dsp_text,"<< After Inverse Quantization: >>");
    log_text_rect_short(log_fd_dsp_text,s->logdct_deqaun[logi],8,8,0);
    log_text(log_fd_dsp_text,"<< After IDCT: >>");
    log_text_rect_char_hex(log_fd_dsp_text,s->logdct_result[logi],8,8,0);
    log_text(log_fd_dsp_text,"<< After IDCT+MC: >>");
    log_text_rect_char_hex(log_fd_dsp_text,s->logdctmc_result[logi],8,8,0);
}
#endif
}




void MPV_decode_mb(MpegEncContext *s, DCTELEM block[12][64]){
#if !CONFIG_SMALL
    if(s->out_format == FMT_MPEG1) {
        if(s->avctx->lowres) MPV_decode_mb_internal(s, block, 1, 1);
        else                 MPV_decode_mb_internal(s, block, 0, 1);
    } else
#endif
    if(s->avctx->lowres) MPV_decode_mb_internal(s, block, 1, 0);
    else                  MPV_decode_mb_internal(s, block, 0, 0);
}

/**
 *
 * @param h is the normal height, this will be reduced automatically if needed for the last row
 */
void ff_draw_horiz_band(MpegEncContext *s, int y, int h){
    const int field_pic= s->picture_structure != PICT_FRAME;
    if(field_pic){
        h <<= 1;
        y <<= 1;
    }

    if (!s->avctx->hwaccel
       && !(s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU)
       && s->unrestricted_mv
       && s->current_picture.reference
       && !s->intra_only
       && !(s->flags&CODEC_FLAG_EMU_EDGE)) {
        int sides = 0, edge_h;
        if (y==0) sides |= EDGE_TOP;
        if (y + h >= s->v_edge_pos) sides |= EDGE_BOTTOM;

        edge_h= FFMIN(h, s->v_edge_pos - y);

        s->dsp.draw_edges(s->current_picture_ptr->data[0] +  y    *s->linesize  , s->linesize  , s->h_edge_pos   , edge_h   , EDGE_WIDTH  , sides);
        s->dsp.draw_edges(s->current_picture_ptr->data[1] + (y>>1)*s->uvlinesize, s->uvlinesize, s->h_edge_pos>>1, edge_h>>1, EDGE_WIDTH/2, sides);
        s->dsp.draw_edges(s->current_picture_ptr->data[2] + (y>>1)*s->uvlinesize, s->uvlinesize, s->h_edge_pos>>1, edge_h>>1, EDGE_WIDTH/2, sides);
    }

    h= FFMIN(h, s->avctx->height - y);

    if(field_pic && s->first_field && !(s->avctx->slice_flags&SLICE_FLAG_ALLOW_FIELD)) return;

    if (s->avctx->draw_horiz_band) {
        AVFrame *src;
        int offset[4];

        if(s->pict_type==FF_B_TYPE || s->low_delay || (s->avctx->slice_flags&SLICE_FLAG_CODED_ORDER))
            src= (AVFrame*)s->current_picture_ptr;
        else if(s->last_picture_ptr)
            src= (AVFrame*)s->last_picture_ptr;
        else
            return;

        if(s->pict_type==FF_B_TYPE && s->picture_structure == PICT_FRAME && s->out_format != FMT_H264){
            offset[0]=
            offset[1]=
            offset[2]=
            offset[3]= 0;
        }else{
            offset[0]= y * s->linesize;
            offset[1]=
            offset[2]= (y >> s->chroma_y_shift) * s->uvlinesize;
            offset[3]= 0;
        }

        emms_c();

        s->avctx->draw_horiz_band(s->avctx, src, offset,
                                  y, s->picture_structure, h);
    }
}

void ff_init_block_index(MpegEncContext *s){ //FIXME maybe rename
    const int linesize= s->current_picture.linesize[0]; //not s->linesize as this would be wrong for field pics
    const int uvlinesize= s->current_picture.linesize[1];
    const int mb_size= 4 - s->avctx->lowres;

    s->block_index[0]= s->b8_stride*(s->mb_y*2    ) - 2 + s->mb_x*2;
    s->block_index[1]= s->b8_stride*(s->mb_y*2    ) - 1 + s->mb_x*2;
    s->block_index[2]= s->b8_stride*(s->mb_y*2 + 1) - 2 + s->mb_x*2;
    s->block_index[3]= s->b8_stride*(s->mb_y*2 + 1) - 1 + s->mb_x*2;
    s->block_index[4]= s->mb_stride*(s->mb_y + 1)                + s->b8_stride*s->mb_height*2 + s->mb_x - 1;
    s->block_index[5]= s->mb_stride*(s->mb_y + s->mb_height + 2) + s->b8_stride*s->mb_height*2 + s->mb_x - 1;
    //block_index is not used by mpeg2, so it is not affected by chroma_format

    s->dest[0] = s->current_picture.data[0] + ((s->mb_x - 1) << mb_size);
    s->dest[1] = s->current_picture.data[1] + ((s->mb_x - 1) << (mb_size - s->chroma_x_shift));
    s->dest[2] = s->current_picture.data[2] + ((s->mb_x - 1) << (mb_size - s->chroma_x_shift));

    if(!(s->pict_type==FF_B_TYPE && s->avctx->draw_horiz_band && s->picture_structure==PICT_FRAME))
    {
        if(s->picture_structure==PICT_FRAME){
        s->dest[0] += s->mb_y *   linesize << mb_size;
        s->dest[1] += s->mb_y * uvlinesize << (mb_size - s->chroma_y_shift);
        s->dest[2] += s->mb_y * uvlinesize << (mb_size - s->chroma_y_shift);
        }else{
            s->dest[0] += (s->mb_y>>1) *   linesize << mb_size;
            s->dest[1] += (s->mb_y>>1) * uvlinesize << (mb_size - s->chroma_y_shift);
            s->dest[2] += (s->mb_y>>1) * uvlinesize << (mb_size - s->chroma_y_shift);
            assert((s->mb_y&1) == (s->picture_structure == PICT_BOTTOM_FIELD));
        }
    }
}

void ff_mpeg_flush(AVCodecContext *avctx){
    int i;
    MpegEncContext *s = avctx->priv_data;

    if(s==NULL || s->picture==NULL)
        return;

    for(i=0; i<s->picture_count; i++){
       if(s->picture[i].data[0] && (   s->picture[i].type == FF_BUFFER_TYPE_INTERNAL
                                    || s->picture[i].type == FF_BUFFER_TYPE_USER))
        free_frame_buffer(s, &s->picture[i]);
    }
    s->current_picture_ptr = s->last_picture_ptr = s->next_picture_ptr = NULL;

    s->mb_x= s->mb_y= 0;
    s->closed_gop= 0;

    s->parse_context.state= -1;
    s->parse_context.frame_start_found= 0;
    s->parse_context.overread= 0;
    s->parse_context.overread_index= 0;
    s->parse_context.index= 0;
    s->parse_context.last_index= 0;
    s->bitstream_buffer_size=0;
    s->pp_time=0;
}

static void dct_unquantize_mpeg1_intra_c(MpegEncContext *s,
                                   DCTELEM *block, int n, int qscale)
{
    int i, level, nCoeffs;
    const uint16_t *quant_matrix;

    nCoeffs= s->block_last_index[n];

    if (n < 4)
        block[0] = block[0] * s->y_dc_scale;
    else
        block[0] = block[0] * s->c_dc_scale;
    /* XXX: only mpeg1 */
    quant_matrix = s->intra_matrix;
    for(i=1;i<=nCoeffs;i++) {
        int j= s->intra_scantable.permutated[i];
        level = block[j];
        if (level) {
            if (level < 0) {
                level = -level;
                level = (int)(level * qscale * quant_matrix[j]) >> 3;
                level = (level - 1) | 1;
                level = -level;
            } else {
                level = (int)(level * qscale * quant_matrix[j]) >> 3;
                level = (level - 1) | 1;
            }
            block[j] = level;
        }
    }
}

static void dct_unquantize_mpeg1_inter_c(MpegEncContext *s,
                                   DCTELEM *block, int n, int qscale)
{
    int i, level, nCoeffs;
    const uint16_t *quant_matrix;

    nCoeffs= s->block_last_index[n];

    quant_matrix = s->inter_matrix;
    for(i=0; i<=nCoeffs; i++) {
        int j= s->intra_scantable.permutated[i];
        level = block[j];
        if (level) {
            if (level < 0) {
                level = -level;
                level = (((level << 1) + 1) * qscale *
                         ((int) (quant_matrix[j]))) >> 4;
                level = (level - 1) | 1;
                level = -level;
            } else {
                level = (((level << 1) + 1) * qscale *
                         ((int) (quant_matrix[j]))) >> 4;
                level = (level - 1) | 1;
            }
            block[j] = level;
        }
    }
}

static void dct_unquantize_mpeg2_intra_c(MpegEncContext *s,
                                   DCTELEM *block, int n, int qscale)
{
    int i, level, nCoeffs;
    const uint16_t *quant_matrix;

    if(s->alternate_scan) nCoeffs= 63;
    else nCoeffs= s->block_last_index[n];

    if (n < 4)
        block[0] = block[0] * s->y_dc_scale;
    else
        block[0] = block[0] * s->c_dc_scale;
    quant_matrix = s->intra_matrix;
    for(i=1;i<=nCoeffs;i++) {
        int j= s->intra_scantable.permutated[i];
        level = block[j];
        if (level) {
            if (level < 0) {
                level = -level;
                level = (int)(level * qscale * quant_matrix[j]) >> 3;
                level = -level;
            } else {
                level = (int)(level * qscale * quant_matrix[j]) >> 3;
            }
            block[j] = level;
        }
    }
}

static void dct_unquantize_mpeg2_intra_bitexact(MpegEncContext *s,
                                   DCTELEM *block, int n, int qscale)
{
    int i, level, nCoeffs;
    const uint16_t *quant_matrix;
    int sum=-1;

    if(s->alternate_scan) nCoeffs= 63;
    else nCoeffs= s->block_last_index[n];

    if (n < 4)
        block[0] = block[0] * s->y_dc_scale;
    else
        block[0] = block[0] * s->c_dc_scale;
    quant_matrix = s->intra_matrix;
    for(i=1;i<=nCoeffs;i++) {
        int j= s->intra_scantable.permutated[i];
        level = block[j];
        if (level) {
            if (level < 0) {
                level = -level;
                level = (int)(level * qscale * quant_matrix[j]) >> 3;
                level = -level;
            } else {
                level = (int)(level * qscale * quant_matrix[j]) >> 3;
            }
            block[j] = level;
            sum+=level;
        }
    }
    block[63]^=sum&1;
}

static void dct_unquantize_mpeg2_inter_c(MpegEncContext *s,
                                   DCTELEM *block, int n, int qscale)
{
    int i, level, nCoeffs;
    const uint16_t *quant_matrix;
    int sum=-1;

    if(s->alternate_scan) nCoeffs= 63;
    else nCoeffs= s->block_last_index[n];

    quant_matrix = s->inter_matrix;
    for(i=0; i<=nCoeffs; i++) {
        int j= s->intra_scantable.permutated[i];
        level = block[j];
        if (level) {
            if (level < 0) {
                level = -level;
                level = (((level << 1) + 1) * qscale *
                         ((int) (quant_matrix[j]))) >> 4;
                level = -level;
            } else {
                level = (((level << 1) + 1) * qscale *
                         ((int) (quant_matrix[j]))) >> 4;
            }
            block[j] = level;
            sum+=level;
        }
    }
    block[63]^=sum&1;
}

static void dct_unquantize_h263_intra_c(MpegEncContext *s,
                                  DCTELEM *block, int n, int qscale)
{
    int i, level, qmul, qadd;
    int nCoeffs;

    assert(s->block_last_index[n]>=0);

    qmul = qscale << 1;

    if (!s->h263_aic) {
        if (n < 4)
            block[0] = block[0] * s->y_dc_scale;
        else
            block[0] = block[0] * s->c_dc_scale;
        qadd = (qscale - 1) | 1;
    }else{
        qadd = 0;
    }
    if(s->ac_pred)
        nCoeffs=63;
    else
        nCoeffs= s->inter_scantable.raster_end[ s->block_last_index[n] ];

    for(i=1; i<=nCoeffs; i++) {
        level = block[i];
        if (level) {
            if (level < 0) {
                level = level * qmul - qadd;
            } else {
                level = level * qmul + qadd;
            }
            block[i] = level;
        }
    }
}

static void dct_unquantize_h263_inter_c(MpegEncContext *s,
                                  DCTELEM *block, int n, int qscale)
{
    int i, level, qmul, qadd;
    int nCoeffs;

    assert(s->block_last_index[n]>=0);

    qadd = (qscale - 1) | 1;
    qmul = qscale << 1;

    nCoeffs= s->inter_scantable.raster_end[ s->block_last_index[n] ];

    for(i=0; i<=nCoeffs; i++) {
        level = block[i];
        if (level) {
            if (level < 0) {
                level = level * qmul - qadd;
            } else {
                level = level * qmul + qadd;
            }
            block[i] = level;
        }
    }
}

/**
 * set qscale and update qscale dependent variables.
 */
void ff_set_qscale(MpegEncContext * s, int qscale)
{
    if (qscale < 1)
        qscale = 1;
    else if (qscale > 31)
        qscale = 31;

    s->qscale = qscale;
    s->chroma_qscale= s->chroma_qscale_table[qscale];

    s->y_dc_scale= s->y_dc_scale_table[ qscale ];
    s->c_dc_scale= s->c_dc_scale_table[ s->chroma_qscale ];
}

void MPV_report_decode_progress(MpegEncContext *s)
{
    if (s->pict_type != FF_B_TYPE && !s->partitioned_frame)
        ff_thread_report_progress((AVFrame*)s->current_picture_ptr, s->mb_y, 0);
}

void ff_h263_loop_filter_nv12(MpegEncContext * s)
{
    int qp_c;
    const int linesize  = s->linesize;
    const int uvlinesize= s->uvlinesize;
    const int xy = s->mb_y * s->mb_stride + s->mb_x;
    uint8_t *dest_y = s->dest[0];
    uint8_t *dest_cb= s->dest[1];
    uint8_t *dest_cr= s->dest[2];
    ambadec_assert_ffmpeg((dest_cb+1)==(dest_cr));
//    if(s->pict_type==FF_B_TYPE && !s->readable) return;

    /*
       Diag Top
       Left Center
    */
    if(!IS_SKIP(s->current_picture.mb_type[xy])){
        qp_c= s->qscale;
        s->dsp.h263_v_loop_filter(dest_y+8*linesize  , linesize, qp_c);
        s->dsp.h263_v_loop_filter(dest_y+8*linesize+8, linesize, qp_c);
    }else
        qp_c= 0;

    if(s->mb_y){
        int qp_dt, qp_tt, qp_tc;

        if(IS_SKIP(s->current_picture.mb_type[xy-s->mb_stride]))
            qp_tt=0;
        else
            qp_tt= s->current_picture.qscale_table[xy-s->mb_stride];

        if(qp_c)
            qp_tc= qp_c;
        else
            qp_tc= qp_tt;

        if(qp_tc){
            const int chroma_qp= s->chroma_qscale_table[qp_tc];
            s->dsp.h263_v_loop_filter(dest_y  ,   linesize, qp_tc);
            s->dsp.h263_v_loop_filter(dest_y+8,   linesize, qp_tc);

            s->dsp.h263_v_loop_filter(dest_cb , uvlinesize, chroma_qp);
            s->dsp.h263_v_loop_filter(dest_cb+8 , uvlinesize, chroma_qp);
        }

        if(qp_tt)
            s->dsp.h263_h_loop_filter(dest_y-8*linesize+8  ,   linesize, qp_tt);

        if(s->mb_x){
            if(qp_tt || IS_SKIP(s->current_picture.mb_type[xy-1-s->mb_stride]))
                qp_dt= qp_tt;
            else
                qp_dt= s->current_picture.qscale_table[xy-1-s->mb_stride];

            if(qp_dt){
                const int chroma_qp= s->chroma_qscale_table[qp_dt];
                s->dsp.h263_h_loop_filter(dest_y -8*linesize  ,   linesize, qp_dt);
                s->dsp.h263_h_loop_filter_nv12(dest_cb-8*uvlinesize, uvlinesize, chroma_qp);
                s->dsp.h263_h_loop_filter_nv12(dest_cr-8*uvlinesize, uvlinesize, chroma_qp);
            }
        }
    }

    if(qp_c){
        s->dsp.h263_h_loop_filter(dest_y +8,   linesize, qp_c);
        if(s->mb_y + 1 == s->mb_height)
            s->dsp.h263_h_loop_filter(dest_y+8*linesize+8,   linesize, qp_c);
    }

    if(s->mb_x){
        int qp_lc;
        if(qp_c || IS_SKIP(s->current_picture.mb_type[xy-1]))
            qp_lc= qp_c;
        else
            qp_lc= s->current_picture.qscale_table[xy-1];

        if(qp_lc){
            s->dsp.h263_h_loop_filter(dest_y,   linesize, qp_lc);
            if(s->mb_y + 1 == s->mb_height){
                const int chroma_qp= s->chroma_qscale_table[qp_lc];
                s->dsp.h263_h_loop_filter(dest_y +8*  linesize,   linesize, qp_lc);
                s->dsp.h263_h_loop_filter_nv12(dest_cb             , uvlinesize, chroma_qp);
                s->dsp.h263_h_loop_filter_nv12(dest_cr             , uvlinesize, chroma_qp);
            }
        }
    }
}

void ff_h263_loop_filter_amba(MpegEncContext * s, Picture* curpic, h263_pic_swinfo_t* pinfo, int mb_width, int mb_height)
{
    int qp_c;
    const int linesize  = curpic->linesize[0];
    const int uvlinesize= curpic->linesize[1];
    int i,j;
    int xy =0;// s->mb_y * s->mb_stride + s->mb_x;
    uint8_t *dest_y = curpic->data[0];
    uint8_t *dest_cb= curpic->data[1];
    //uint8_t *dest_cr= s->dest[2];
    ambadec_assert_ffmpeg((curpic->data[1]+1)==(curpic->data[2]));
    ambadec_assert_ffmpeg(curpic->linesize[1]==curpic->linesize[2]);


    for(j=0;j<mb_height;j++)
    {
        dest_y = curpic->data[0]+j*linesize;
        dest_cb= curpic->data[1]+j*uvlinesize;
        xy=j * (mb_width+1);
        for(i=0;i<mb_width;i++)
        {
            /*
               Diag Top
               Left Center
            */
            if(!IS_SKIP(curpic->mb_type[xy])){
                qp_c= pinfo->qscale;
                s->dsp.h263_v_loop_filter(dest_y+8*linesize  , linesize, qp_c);
                s->dsp.h263_v_loop_filter(dest_y+8*linesize+8, linesize, qp_c);
            }else
                qp_c= 0;

            if(j){
                int qp_dt, qp_tt, qp_tc;

                if(IS_SKIP(curpic->mb_type[xy-(mb_width+1)]))
                    qp_tt=0;
                else
                    qp_tt= curpic->qscale_table[xy-(mb_width+1)];

                if(qp_c)
                    qp_tc= qp_c;
                else
                    qp_tc= qp_tt;

                if(qp_tc){
                    const int chroma_qp= s->chroma_qscale_table[qp_tc];
                    s->dsp.h263_v_loop_filter(dest_y  ,   linesize, qp_tc);
                    s->dsp.h263_v_loop_filter(dest_y+8,   linesize, qp_tc);

                    s->dsp.h263_v_loop_filter(dest_cb , uvlinesize, chroma_qp);
                    s->dsp.h263_v_loop_filter(dest_cb+8 , uvlinesize, chroma_qp);
                }

                if(qp_tt)
                    s->dsp.h263_h_loop_filter(dest_y-8*linesize+8  ,   linesize, qp_tt);

                if(s->mb_x){
                    if(qp_tt || IS_SKIP(curpic->mb_type[xy-2-mb_width]))
                        qp_dt= qp_tt;
                    else
                        qp_dt= curpic->qscale_table[xy-2-mb_width];

                    if(qp_dt){
                        const int chroma_qp= s->chroma_qscale_table[qp_dt];
                        s->dsp.h263_h_loop_filter(dest_y -8*linesize  ,   linesize, qp_dt);
                        s->dsp.h263_h_loop_filter_nv12(dest_cb-8*uvlinesize, uvlinesize, chroma_qp);
                        s->dsp.h263_h_loop_filter_nv12(dest_cb+1-8*uvlinesize, uvlinesize, chroma_qp);
                    }
                }
            }

            if(qp_c){
                s->dsp.h263_h_loop_filter(dest_y +8,   linesize, qp_c);
                if(j + 1 == mb_height)
                    s->dsp.h263_h_loop_filter(dest_y+8*linesize+8,   linesize, qp_c);
            }

            if(s->mb_x){
                int qp_lc;
                if(qp_c || IS_SKIP(curpic->mb_type[xy-1]))
                    qp_lc= qp_c;
                else
                    qp_lc= curpic->qscale_table[xy-1];

                if(qp_lc){
                    s->dsp.h263_h_loop_filter(dest_y,   linesize, qp_lc);
                    if(j + 1 == mb_height){
                        const int chroma_qp= s->chroma_qscale_table[qp_lc];
                        s->dsp.h263_h_loop_filter(dest_y +8*  linesize,   linesize, qp_lc);
                        s->dsp.h263_h_loop_filter_nv12(dest_cb             , uvlinesize, chroma_qp);
                        s->dsp.h263_h_loop_filter_nv12(dest_cb+1             , uvlinesize, chroma_qp);
                    }
                }
            }

            dest_y +=16 ;
            dest_cb += 16;
            xy++;
            pinfo++;
        }

    }

}

//
int ff_alloc_picture_amba(MpegEncContext *s, Picture *pic)
{
    const int big_mb_num= s->mb_stride*(s->mb_height+1) + 1; //the +1 is needed so memset(,,stride*height) does not sig11
    const int mb_array_size= s->mb_stride*s->mb_height;
    const int b8_array_size= s->b8_stride*s->mb_height*2;
    const int b4_array_size= s->b4_stride*s->mb_height*4;
    int i;

//    s->avctx->get_buffer(s->avctx, (AVFrame*)pic);
//    s->linesize  = pic->linesize[0];
//    s->uvlinesize= pic->linesize[1];

    if(pic->qscale_table==NULL){
        /* if (s->encoding) {
            FF_ALLOCZ_OR_GOTO(s->avctx, pic->mb_var   , mb_array_size * sizeof(int16_t)  , fail)
            FF_ALLOCZ_OR_GOTO(s->avctx, pic->mc_mb_var, mb_array_size * sizeof(int16_t)  , fail)
            FF_ALLOCZ_OR_GOTO(s->avctx, pic->mb_mean  , mb_array_size * sizeof(int8_t )  , fail)
        }*/

        FF_ALLOCZ_OR_GOTO(s->avctx, pic->mbskip_table , mb_array_size * sizeof(uint8_t)+2, fail) //the +2 is for the slice end check
        FF_ALLOCZ_OR_GOTO(s->avctx, pic->qscale_table , mb_array_size * sizeof(uint8_t)  , fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, pic->mb_type_base , (big_mb_num + s->mb_stride) * sizeof(uint32_t), fail)
        pic->mb_type= pic->mb_type_base + 2*s->mb_stride+1;
        if(s->out_format == FMT_H264){
            for(i=0; i<2; i++){
                FF_ALLOCZ_OR_GOTO(s->avctx, pic->motion_val_base[i], 2 * (b4_array_size+4)  * sizeof(int16_t), fail)
                pic->motion_val[i]= pic->motion_val_base[i]+4;
                FF_ALLOCZ_OR_GOTO(s->avctx, pic->ref_index[i], b8_array_size * sizeof(uint8_t), fail)
            }
            pic->motion_subsample_log2= 2;
        }else if(s->out_format == FMT_H263 || s->encoding || (s->avctx->debug&FF_DEBUG_MV) || (s->avctx->debug_mv)){
            for(i=0; i<2; i++){
                FF_ALLOCZ_OR_GOTO(s->avctx, pic->motion_val_base[i], 2 * (b8_array_size+4) * sizeof(int16_t), fail)
                pic->motion_val[i]= pic->motion_val_base[i]+4;
                FF_ALLOCZ_OR_GOTO(s->avctx, pic->ref_index[i], b8_array_size * sizeof(uint8_t), fail)
            }
            pic->motion_subsample_log2= 3;
        }
        if(s->avctx->debug&FF_DEBUG_DCT_COEFF) {
            FF_ALLOCZ_OR_GOTO(s->avctx, pic->dct_coeff, 64 * mb_array_size * sizeof(DCTELEM)*6, fail)
        }
        pic->qstride= s->mb_stride;
        FF_ALLOCZ_OR_GOTO(s->avctx, pic->pan_scan , 1 * sizeof(AVPanScan), fail)
    }

    /* It might be nicer if the application would keep track of these
     * but it would require an API change. */
    memmove(s->prev_pict_types+1, s->prev_pict_types, PREV_PICT_TYPES_BUFFER_SIZE-1);
    s->prev_pict_types[0]= s->dropable ? FF_B_TYPE : s->pict_type;
    if(pic->age < PREV_PICT_TYPES_BUFFER_SIZE && s->prev_pict_types[pic->age] == FF_B_TYPE)
        pic->age= INT_MAX; // Skipped MBs in B-frames are quite rare in MPEG-1/2 and it is a bit tricky to skip them anyway.

//form MPV_frame_start
    if(s->mpeg_quant || s->codec_id == CODEC_ID_MPEG2VIDEO){
        s->dct_unquantize_intra = s->dct_unquantize_mpeg2_intra;
        s->dct_unquantize_inter = s->dct_unquantize_mpeg2_inter;
    }else if(s->out_format == FMT_H263 || s->out_format == FMT_H261){
        s->dct_unquantize_intra = s->dct_unquantize_h263_intra;
        s->dct_unquantize_inter = s->dct_unquantize_h263_inter;
    }else{
        s->dct_unquantize_intra = s->dct_unquantize_mpeg1_intra;
        s->dct_unquantize_inter = s->dct_unquantize_mpeg1_inter;
    }

    return 0;

fail: //for the FF_ALLOCZ_OR_GOTO macro
//    s->avctx->release_buffer(s->avctx, (AVFrame*)pic);
    return -1;
}

void free_picture_amba(Picture *pic)
{
    int i;

//    s->avctx->release_buffer(s->avctx, (AVFrame*)pic);
    av_freep(&pic->mb_var);
    av_freep(&pic->mc_mb_var);
    av_freep(&pic->mb_mean);
    av_freep(&pic->mbskip_table);
    av_freep(&pic->qscale_table);
    av_freep(&pic->mb_type_base);
    av_freep(&pic->dct_coeff);
    av_freep(&pic->pan_scan);
    pic->mb_type= NULL;
    for(i=0; i<2; i++){
        av_freep(&pic->motion_val_base[i]);
        av_freep(&pic->ref_index[i]);
    }
}

av_cold int MPV_common_init_amba(MpegEncContext *s)
{
    int y_size, c_size, yc_size, i, mb_array_size, mv_table_size, x, y, threads;

    if(s->codec_id == CODEC_ID_MPEG2VIDEO && !s->progressive_sequence)
        s->mb_height = (s->height + 31) / 32 * 2;
    else
        s->mb_height = (s->height + 15) / 16;

    if(s->avctx->pix_fmt == PIX_FMT_NONE){
        av_log(s->avctx, AV_LOG_ERROR, "decoding to PIX_FMT_NONE is not supported.\n");
        return -1;
    }

    if(s->avctx->thread_count > MAX_THREADS || (s->avctx->thread_count > s->mb_height && s->mb_height)){
        av_log(s->avctx, AV_LOG_ERROR, "too many threads\n");
        return -1;
    }

    if((s->width || s->height) && av_image_check_size(s->width, s->height, 0, s->avctx))
        return -1;

    dsputil_init(&s->dsp, s->avctx);
    ff_dct_common_init(s);

    s->flags= s->avctx->flags;
    s->flags2= s->avctx->flags2;

    s->mb_width  = (s->width  + 15) / 16;
    s->mb_stride = s->mb_width + 1;
    s->b8_stride = s->mb_width*2 + 1;
    s->b4_stride = s->mb_width*4 + 1;
    mb_array_size= s->mb_height * s->mb_stride;
    mv_table_size= (s->mb_height+2) * s->mb_stride + 1;

    /* set chroma shifts */
    avcodec_get_chroma_sub_sample(s->avctx->pix_fmt,&(s->chroma_x_shift),
                                                    &(s->chroma_y_shift) );

    /* set default edge pos, will be overriden in decode_header if needed */
    s->h_edge_pos= s->mb_width*16;
    s->v_edge_pos= s->mb_height*16;

    s->mb_num = s->mb_width * s->mb_height;

    s->block_wrap[0]=
    s->block_wrap[1]=
    s->block_wrap[2]=
    s->block_wrap[3]= s->b8_stride;
    s->block_wrap[4]=
    s->block_wrap[5]= s->mb_stride;

    y_size = s->b8_stride * (2 * s->mb_height + 1);
    c_size = s->mb_stride * (s->mb_height + 1);
    yc_size = y_size + 2 * c_size;

    /* convert fourcc to upper case */
    s->codec_tag=          toupper( s->avctx->codec_tag     &0xFF)
                        + (toupper((s->avctx->codec_tag>>8 )&0xFF)<<8 )
                        + (toupper((s->avctx->codec_tag>>16)&0xFF)<<16)
                        + (toupper((s->avctx->codec_tag>>24)&0xFF)<<24);

    s->stream_codec_tag=          toupper( s->avctx->stream_codec_tag     &0xFF)
                               + (toupper((s->avctx->stream_codec_tag>>8 )&0xFF)<<8 )
                               + (toupper((s->avctx->stream_codec_tag>>16)&0xFF)<<16)
                               + (toupper((s->avctx->stream_codec_tag>>24)&0xFF)<<24);

    s->avctx->coded_frame= (AVFrame*)&s->current_picture;

    FF_ALLOCZ_OR_GOTO(s->avctx, s->mb_index2xy, (s->mb_num+1)*sizeof(int), fail) //error ressilience code looks cleaner with this
    for(y=0; y<s->mb_height; y++){
        for(x=0; x<s->mb_width; x++){
            s->mb_index2xy[ x + y*s->mb_width ] = x + y*s->mb_stride;
        }
    }
    s->mb_index2xy[ s->mb_height*s->mb_width ] = (s->mb_height-1)*s->mb_stride + s->mb_width; //FIXME really needed?

    if (s->encoding) {
        /* Allocate MV tables */
        FF_ALLOCZ_OR_GOTO(s->avctx, s->p_mv_table_base            , mv_table_size * 2 * sizeof(int16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_forw_mv_table_base       , mv_table_size * 2 * sizeof(int16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_back_mv_table_base       , mv_table_size * 2 * sizeof(int16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_bidir_forw_mv_table_base , mv_table_size * 2 * sizeof(int16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_bidir_back_mv_table_base , mv_table_size * 2 * sizeof(int16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_direct_mv_table_base     , mv_table_size * 2 * sizeof(int16_t), fail)
        s->p_mv_table           = s->p_mv_table_base            + s->mb_stride + 1;
        s->b_forw_mv_table      = s->b_forw_mv_table_base       + s->mb_stride + 1;
        s->b_back_mv_table      = s->b_back_mv_table_base       + s->mb_stride + 1;
        s->b_bidir_forw_mv_table= s->b_bidir_forw_mv_table_base + s->mb_stride + 1;
        s->b_bidir_back_mv_table= s->b_bidir_back_mv_table_base + s->mb_stride + 1;
        s->b_direct_mv_table    = s->b_direct_mv_table_base     + s->mb_stride + 1;

        if(s->msmpeg4_version){
            FF_ALLOCZ_OR_GOTO(s->avctx, s->ac_stats, 2*2*(MAX_LEVEL+1)*(MAX_RUN+1)*2*sizeof(int), fail);
        }
        FF_ALLOCZ_OR_GOTO(s->avctx, s->avctx->stats_out, 256, fail);

        /* Allocate MB type table */
        FF_ALLOCZ_OR_GOTO(s->avctx, s->mb_type  , mb_array_size * sizeof(uint16_t), fail) //needed for encoding

        FF_ALLOCZ_OR_GOTO(s->avctx, s->lambda_table, mb_array_size * sizeof(int), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_intra_matrix  , 64*32   * sizeof(int), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_inter_matrix  , 64*32   * sizeof(int), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_intra_matrix16, 64*32*2 * sizeof(uint16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_inter_matrix16, 64*32*2 * sizeof(uint16_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->input_picture, MAX_PICTURE_COUNT * sizeof(Picture*), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->reordered_input_picture, MAX_PICTURE_COUNT * sizeof(Picture*), fail)

        if(s->avctx->noise_reduction){
            FF_ALLOCZ_OR_GOTO(s->avctx, s->dct_offset, 2 * 64 * sizeof(uint16_t), fail)
        }
    }
    FF_ALLOCZ_OR_GOTO(s->avctx, s->picture, MAX_PICTURE_COUNT * sizeof(Picture), fail)
    for(i = 0; i < MAX_PICTURE_COUNT; i++) {
        avcodec_get_frame_defaults((AVFrame *)&s->picture[i]);
    }

    FF_ALLOCZ_OR_GOTO(s->avctx, s->error_status_table, mb_array_size*sizeof(uint8_t), fail)

    if((s->codec_id==CODEC_ID_MPEG4 || s->codec_id==CODEC_ID_AMBA_MPEG4) || (s->flags & CODEC_FLAG_INTERLACED_ME)){
        /* interlaced direct mode decoding tables */
            for(i=0; i<2; i++){
                int j, k;
                for(j=0; j<2; j++){
                    for(k=0; k<2; k++){
                        FF_ALLOCZ_OR_GOTO(s->avctx,    s->b_field_mv_table_base[i][j][k], mv_table_size * 2 * sizeof(int16_t), fail)
                        s->b_field_mv_table[i][j][k] = s->b_field_mv_table_base[i][j][k] + s->mb_stride + 1;
                    }
                    FF_ALLOCZ_OR_GOTO(s->avctx, s->b_field_select_table [i][j], mb_array_size * 2 * sizeof(uint8_t), fail)
                    FF_ALLOCZ_OR_GOTO(s->avctx, s->p_field_mv_table_base[i][j], mv_table_size * 2 * sizeof(int16_t), fail)
                    s->p_field_mv_table[i][j] = s->p_field_mv_table_base[i][j]+ s->mb_stride + 1;
                }
                FF_ALLOCZ_OR_GOTO(s->avctx, s->p_field_select_table[i], mb_array_size * 2 * sizeof(uint8_t), fail)
            }
    }
    if (s->out_format == FMT_H263) {
        /* ac values */
        FF_ALLOCZ_OR_GOTO(s->avctx, s->ac_val_base, yc_size * sizeof(int16_t) * 16, fail);
        s->ac_val[0] = s->ac_val_base + s->b8_stride + 1;
        s->ac_val[1] = s->ac_val_base + y_size + s->mb_stride + 1;
        s->ac_val[2] = s->ac_val[1] + c_size;

        /* cbp values */
        FF_ALLOCZ_OR_GOTO(s->avctx, s->coded_block_base, y_size, fail);
        s->coded_block= s->coded_block_base + s->b8_stride + 1;

        /* cbp, ac_pred, pred_dir */
        FF_ALLOCZ_OR_GOTO(s->avctx, s->cbp_table     , mb_array_size * sizeof(uint8_t), fail)
        FF_ALLOCZ_OR_GOTO(s->avctx, s->pred_dir_table, mb_array_size * sizeof(uint8_t), fail)
    }

    if (s->h263_pred || s->h263_plus || !s->encoding) {
        /* dc values */
        //MN: we need these for error resilience of intra-frames
        FF_ALLOCZ_OR_GOTO(s->avctx, s->dc_val_base, yc_size * sizeof(int16_t), fail);
        s->dc_val[0] = s->dc_val_base + s->b8_stride + 1;
        s->dc_val[1] = s->dc_val_base + y_size + s->mb_stride + 1;
        s->dc_val[2] = s->dc_val[1] + c_size;
        for(i=0;i<yc_size;i++)
            s->dc_val_base[i] = 1024;
    }

    /* which mb is a intra block */
    FF_ALLOCZ_OR_GOTO(s->avctx, s->mbintra_table, mb_array_size, fail);
    memset(s->mbintra_table, 1, mb_array_size);

    /* init macroblock skip table */
    FF_ALLOCZ_OR_GOTO(s->avctx, s->mbskip_table, mb_array_size+2, fail);
    //Note the +1 is for a quicker mpeg4 slice_end detection
    FF_ALLOCZ_OR_GOTO(s->avctx, s->prev_pict_types, PREV_PICT_TYPES_BUFFER_SIZE, fail);

    s->parse_context.state= -1;
    if((s->avctx->debug&(FF_DEBUG_VIS_QP|FF_DEBUG_VIS_MB_TYPE)) || (s->avctx->debug_mv)){
       s->visualization_buffer[0] = av_malloc((s->mb_width*16 + 2*EDGE_WIDTH) * s->mb_height*16 + 2*EDGE_WIDTH);
       s->visualization_buffer[1] = av_malloc((s->mb_width*16 + 2*EDGE_WIDTH) * s->mb_height*16 + 2*EDGE_WIDTH);
       s->visualization_buffer[2] = av_malloc((s->mb_width*16 + 2*EDGE_WIDTH) * s->mb_height*16 + 2*EDGE_WIDTH);
    }

    s->context_initialized = 1;

    s->thread_context[0]= s;
    threads = s->avctx->thread_count;

    for(i=1; i<threads; i++){
        s->thread_context[i]= av_malloc(sizeof(MpegEncContext));
        memcpy(s->thread_context[i], s, sizeof(MpegEncContext));
    }

    for(i=0; i<threads; i++){
        if(init_duplicate_context(s->thread_context[i], s) < 0)
           goto fail;
        s->thread_context[i]->start_mb_y= (s->mb_height*(i  ) + s->avctx->thread_count/2) / s->avctx->thread_count;
        s->thread_context[i]->end_mb_y  = (s->mb_height*(i+1) + s->avctx->thread_count/2) / s->avctx->thread_count;
    }

    return 0;
 fail:
    MPV_common_end_amba(s);
    return -1;
}

void MPV_common_end_amba(MpegEncContext *s)
{
    int i, j, k;

    for(i=0; i<s->avctx->thread_count; i++){
        free_duplicate_context(s->thread_context[i]);
    }
    for(i=1; i<s->avctx->thread_count; i++){
        av_freep(&s->thread_context[i]);
    }

    av_freep(&s->parse_context.buffer);
    s->parse_context.buffer_size=0;

    av_freep(&s->mb_type);
    av_freep(&s->p_mv_table_base);
    av_freep(&s->b_forw_mv_table_base);
    av_freep(&s->b_back_mv_table_base);
    av_freep(&s->b_bidir_forw_mv_table_base);
    av_freep(&s->b_bidir_back_mv_table_base);
    av_freep(&s->b_direct_mv_table_base);
    s->p_mv_table= NULL;
    s->b_forw_mv_table= NULL;
    s->b_back_mv_table= NULL;
    s->b_bidir_forw_mv_table= NULL;
    s->b_bidir_back_mv_table= NULL;
    s->b_direct_mv_table= NULL;
    for(i=0; i<2; i++){
        for(j=0; j<2; j++){
            for(k=0; k<2; k++){
                av_freep(&s->b_field_mv_table_base[i][j][k]);
                s->b_field_mv_table[i][j][k]=NULL;
            }
            av_freep(&s->b_field_select_table[i][j]);
            av_freep(&s->p_field_mv_table_base[i][j]);
            s->p_field_mv_table[i][j]=NULL;
        }
        av_freep(&s->p_field_select_table[i]);
    }

    av_freep(&s->dc_val_base);
    av_freep(&s->ac_val_base);
    av_freep(&s->coded_block_base);
    av_freep(&s->mbintra_table);
    av_freep(&s->cbp_table);
    av_freep(&s->pred_dir_table);

    av_freep(&s->mbskip_table);
    av_freep(&s->prev_pict_types);
    av_freep(&s->bitstream_buffer);
    s->allocated_bitstream_buffer_size=0;

    av_freep(&s->avctx->stats_out);
    av_freep(&s->ac_stats);
    av_freep(&s->error_status_table);
    av_freep(&s->mb_index2xy);
    av_freep(&s->lambda_table);
    av_freep(&s->q_intra_matrix);
    av_freep(&s->q_inter_matrix);
    av_freep(&s->q_intra_matrix16);
    av_freep(&s->q_inter_matrix16);
    av_freep(&s->input_picture);
    av_freep(&s->reordered_input_picture);
    av_freep(&s->dct_offset);

    s->context_initialized = 0;
    s->last_picture_ptr=
    s->next_picture_ptr=
    s->current_picture_ptr= NULL;
    s->linesize= s->uvlinesize= 0;

    for(i=0; i<3; i++)
        av_freep(&s->visualization_buffer[i]);

    avcodec_default_free_buffers(s->avctx);
}

/* generic function for encode/decode called after a frame has been coded/decoded */
void MPV_frame_end_nv12(MpegEncContext *s)
{
    int i;
    /* draw edge for correct motion prediction if outside */
    //just to make sure that all data is rendered.
    if(CONFIG_MPEG_XVMC_DECODER && s->avctx->xvmc_acceleration){
        ff_xvmc_field_end(s);
    }else if(!s->avctx->hwaccel
       && !(s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU)
       && s->unrestricted_mv
       && s->current_picture.reference
       && !s->intra_only
       && !(s->flags&CODEC_FLAG_EMU_EDGE)) {
//            av_log(NULL,AV_LOG_ERROR,"here!!!!here!! reference,s->intra_only=%d,s->flags=%x,s->unrestricted_mv=%d,s->current_picture_ptr->reference=%d\n.\n",s->intra_only,s->flags,s->unrestricted_mv,s->current_picture.reference);
            int edges = EDGE_BOTTOM | EDGE_TOP, h = s->v_edge_pos;

            s->dsp.draw_edges(s->current_picture.data[0], s->linesize  , s->h_edge_pos   , h   , EDGE_WIDTH  , edges);
            s->dsp.draw_edges_nv12(s->current_picture.data[1], s->uvlinesize, s->h_edge_pos>>1, h>>1);
//            s->dsp.draw_edges(s->current_picture.data[2], s->uvlinesize, s->h_edge_pos>>1, h>>1, EDGE_WIDTH/2, edges);
    }
    emms_c();

//av_log(NULL,AV_LOG_WARNING,"s->avctx->hwaccel=%d,vdpau=%d.\n",s->avctx->hwaccel,(s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU));
//av_log(NULL,AV_LOG_WARNING,"s->unrestricted_mv=%d,s->current_picture.reference=%d,s->intra_only=%d.\n",s->unrestricted_mv,s->current_picture.reference,s->intra_only);
//av_log(NULL,AV_LOG_WARNING,"edge=%d, here!!!!\n.\n",(s->flags&CODEC_FLAG_EMU_EDGE));

    s->last_pict_type    = s->pict_type;
    s->last_lambda_for[s->pict_type]= s->current_picture_ptr->quality;
    if(s->pict_type!=FF_B_TYPE){
        s->last_non_b_pict_type= s->pict_type;
    }
#if 0
        /* copy back current_picture variables */
    for(i=0; i<MAX_PICTURE_COUNT; i++){
        if(s->picture[i].data[0] == s->current_picture.data[0]){
            s->picture[i]= s->current_picture;
            break;
        }
    }
    assert(i<MAX_PICTURE_COUNT);
#endif

    if(s->encoding){
        /* release non-reference frames */
        for(i=0; i<MAX_PICTURE_COUNT; i++){
            if(s->picture[i].data[0] && !s->picture[i].reference /*&& s->picture[i].type!=FF_BUFFER_TYPE_SHARED*/){
                free_frame_buffer(s, &s->picture[i]);
            }
        }
    }
    // clear copies, to avoid confusion
#if 0
    memset(&s->last_picture, 0, sizeof(Picture));
    memset(&s->next_picture, 0, sizeof(Picture));
    memset(&s->current_picture, 0, sizeof(Picture));
#endif
    s->avctx->coded_frame= (AVFrame*)s->current_picture_ptr;
}

void MPV_frame_end_amba(MpegEncContext *s)
{
    int i;
    /* draw edge for correct motion prediction if outside */
    //just to make sure that all data is rendered.
    if(!s->avctx->hwaccel
       && !(s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU)
       && s->unrestricted_mv
       && s->current_picture_ptr->reference
       && !s->intra_only
       && !(s->flags&CODEC_FLAG_EMU_EDGE)) {
//            av_log(NULL,AV_LOG_ERROR,"here!!!!here!! reference,s->intra_only=%d,s->flags=%x,s->unrestricted_mv=%d,s->current_picture_ptr->reference=%d\n.\n",s->intra_only,s->flags,s->unrestricted_mv,s->current_picture.reference);
            int edges = EDGE_BOTTOM | EDGE_TOP, h = s->v_edge_pos;

            s->dsp.draw_edges(s->current_picture.data[0], s->linesize  , s->h_edge_pos   , h   , EDGE_WIDTH  , edges);
            s->dsp.draw_edges_nv12(s->current_picture.data[1], s->uvlinesize, s->h_edge_pos>>1, h>>1);
//            s->dsp.draw_edges(s->current_picture.data[2], s->uvlinesize, s->h_edge_pos>>1, h>>1, EDGE_WIDTH/2, edges);
    }
    emms_c();

//av_log(NULL,AV_LOG_WARNING,"s->avctx->hwaccel=%d,vdpau=%d.\n",s->avctx->hwaccel,(s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU));
//av_log(NULL,AV_LOG_WARNING,"s->unrestricted_mv=%d,s->current_picture.reference=%d,s->intra_only=%d.\n",s->unrestricted_mv,s->current_picture.reference,s->intra_only);
//av_log(NULL,AV_LOG_WARNING,"edge=%d, here!!!!\n.\n",(s->flags&CODEC_FLAG_EMU_EDGE));

    s->last_pict_type    = s->pict_type;
    s->last_lambda_for[s->pict_type]= s->current_picture_ptr->quality;
    if(s->pict_type!=FF_B_TYPE){
        s->last_non_b_pict_type= s->pict_type;
    }
#if 0
        /* copy back current_picture variables */
    for(i=0; i<MAX_PICTURE_COUNT; i++){
        if(s->picture[i].data[0] == s->current_picture.data[0]){
            s->picture[i]= s->current_picture;
            break;
        }
    }
    assert(i<MAX_PICTURE_COUNT);
#endif

    if(s->encoding){
        /* release non-reference frames */
        for(i=0; i<MAX_PICTURE_COUNT; i++){
            if(s->picture[i].data[0] && !s->picture[i].reference /*&& s->picture[i].type!=FF_BUFFER_TYPE_SHARED*/){
                free_frame_buffer(s, &s->picture[i]);
            }
        }
    }
    // clear copies, to avoid confusion
#if 0
    memset(&s->last_picture, 0, sizeof(Picture));
    memset(&s->next_picture, 0, sizeof(Picture));
    memset(&s->current_picture, 0, sizeof(Picture));
#endif
    s->avctx->coded_frame= (AVFrame*)s->current_picture_ptr;
}

static av_always_inline void mpeg_motion_lowres_nv12(MpegEncContext *s,
                               uint8_t *dest_y, uint8_t *dest_cb,
                               int field_based, int bottom_field, int field_select,
                               uint8_t **ref_picture, h264_chroma_mc_func *pix_op,h264_chroma_mc_func *pix_op_nv12,
                               int motion_x, int motion_y, int h)
{
    uint8_t *ptr_y, *ptr_cb;//, *ptr_cr;
    int mx, my, src_x, src_y, uvsrc_x, uvsrc_y, uvlinesize, linesize, sx, sy, uvsx, uvsy;
    const int lowres= s->avctx->lowres;
    const int block_s= 8>>lowres;
    const int s_mask= (2<<lowres)-1;
    const int h_edge_pos = s->h_edge_pos >> lowres;
    const int v_edge_pos = s->v_edge_pos >> lowres;
    linesize   = s->current_picture.linesize[0] << field_based;
    uvlinesize = s->current_picture.linesize[1] << field_based;

    if(s->quarter_sample){ //FIXME obviously not perfect but qpel will not work in lowres anyway
        motion_x/=2;
        motion_y/=2;
    }

    if(field_based){
        motion_y += (bottom_field - field_select)*((1<<lowres)-1);
    }

    sx= motion_x & s_mask;
    sy= motion_y & s_mask;
    src_x = s->mb_x*2*block_s               + (motion_x >> (lowres+1));
    src_y =(s->mb_y*2*block_s>>field_based) + (motion_y >> (lowres+1));

    if (s->out_format == FMT_H263) {
        uvsx = ((motion_x>>1) & s_mask) | (sx&1);
        uvsy = ((motion_y>>1) & s_mask) | (sy&1);
        uvsrc_x = src_x>>1;
        uvsrc_y = src_y>>1;
    }else if(s->out_format == FMT_H261){//even chroma mv's are full pel in H261
        mx = motion_x / 4;
        my = motion_y / 4;
        uvsx = (2*mx) & s_mask;
        uvsy = (2*my) & s_mask;
        uvsrc_x = s->mb_x*block_s               + ((mx) >> lowres);
        uvsrc_y = s->mb_y*block_s               + (my >> lowres);
    } else {
        mx = motion_x / 2;
        my = motion_y / 2;
        uvsx = mx & s_mask;
        uvsy = my & s_mask;
        uvsrc_x = s->mb_x*block_s              + (mx >> (lowres+1));
        uvsrc_y =(s->mb_y*block_s>>field_based) + (my >> (lowres+1));
    }

    ptr_y  = ref_picture[0] + src_y * linesize + src_x;
    ptr_cb = ref_picture[1] + uvsrc_y * uvlinesize + uvsrc_x*2;
//    ptr_cr = ref_picture[2] + uvsrc_y * uvlinesize + uvsrc_x;

    if(   (unsigned)src_x > h_edge_pos                 - (!!sx) - 2*block_s
       || (unsigned)src_y >(v_edge_pos >> field_based) - (!!sy) - h){
            s->dsp.emulated_edge_mc(s->edge_emu_buffer, ptr_y, s->linesize, 17, 17+field_based,
                             src_x, src_y<<field_based, h_edge_pos, v_edge_pos);
            ptr_y = s->edge_emu_buffer;
            if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                uint8_t *uvbuf= s->edge_emu_buffer+18*s->linesize;
                s->dsp.emulated_edge_mc_nv12(uvbuf  , ptr_cb, s->uvlinesize, 9, 9+field_based,
                                 uvsrc_x, uvsrc_y<<field_based, h_edge_pos>>1, v_edge_pos>>1);
/*                s->dsp.emulated_edge_mc(uvbuf+16, ptr_cr, s->uvlinesize, 9, 9+field_based,
                                 uvsrc_x, uvsrc_y<<field_based, h_edge_pos>>1, v_edge_pos>>1);*/
                ptr_cb= uvbuf;
//                ptr_cr= uvbuf+16;
            }
    }

    if(bottom_field){ //FIXME use this for field pix too instead of the obnoxious hack which changes picture.data
        dest_y += s->linesize;
        dest_cb+= s->uvlinesize;
//        dest_cr+= s->uvlinesize;
    }

    if(field_select){
        ptr_y += s->linesize;
        ptr_cb+= s->uvlinesize;
//        ptr_cr+= s->uvlinesize;
    }

    sx <<= 2 - lowres;
    sy <<= 2 - lowres;
    pix_op[lowres-1](dest_y, ptr_y, linesize, h, sx, sy);

    if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
        uvsx <<= 2 - lowres;
        uvsy <<= 2 - lowres;
        pix_op_nv12[lowres](dest_cb, ptr_cb, uvlinesize, h >> s->chroma_y_shift, uvsx, uvsy);
//        pix_op[lowres](dest_cr, ptr_cr, uvlinesize, h >> s->chroma_y_shift, uvsx, uvsy);
    }
    //FIXME h261 lowres loop filter
}

static inline void chroma_4mv_motion_lowres_nv12(MpegEncContext *s,
                                     uint8_t *dest_cb,
                                     uint8_t **ref_picture,
                                     h264_chroma_mc_func *pix_op_nv12,
                                     int mx, int my)
{
    const int lowres= s->avctx->lowres;
    const int block_s= 8>>lowres;
    const int s_mask= (2<<lowres)-1;
    const int h_edge_pos = s->h_edge_pos >> (lowres+1);
    const int v_edge_pos = s->v_edge_pos >> (lowres+1);
    int emu=0, src_x, src_y, offset, sx, sy;
    uint8_t *ptr;

    if(s->quarter_sample){
        mx/=2;
        my/=2;
    }

    /* In case of 8X8, we construct a single chroma motion vector
       with a special rounding */
    mx= ff_h263_round_chroma(mx);
    my= ff_h263_round_chroma(my);

    sx= mx & s_mask;
    sy= my & s_mask;
    src_x = s->mb_x*block_s + (mx >> (lowres+1));
    src_y = s->mb_y*block_s + (my >> (lowres+1));

    offset = src_y * s->uvlinesize + src_x*2;
    ptr = ref_picture[1] + offset;
    if(s->flags&CODEC_FLAG_EMU_EDGE){
        if(   (unsigned)src_x > h_edge_pos - (!!sx) - block_s
           || (unsigned)src_y > v_edge_pos - (!!sy) - block_s){
            s->dsp.emulated_edge_mc_nv12(s->edge_emu_buffer, ptr, s->uvlinesize, 9, 9, src_x, src_y, h_edge_pos, v_edge_pos);
            ptr= s->edge_emu_buffer;
            emu=1;
        }
    }
    av_log(NULL, AV_LOG_DEBUG, "emu=%d\n", emu);
    sx <<= 2 - lowres;
    sy <<= 2 - lowres;
    pix_op_nv12[lowres](dest_cb, ptr, s->uvlinesize, block_s, sx, sy);
}

static inline void MPV_motion_lowres_nv12(MpegEncContext *s,
                              uint8_t *dest_y, uint8_t *dest_cb,
                              int dir, uint8_t **ref_picture,
                              h264_chroma_mc_func *pix_op,h264_chroma_mc_func *pix_op_nv12)
{
    int mx, my;
    int mb_x, mb_y, i;
    const int lowres= s->avctx->lowres;
    const int block_s= 8>>lowres;

    mb_x = s->mb_x;
    mb_y = s->mb_y;

    switch(s->mv_type) {
    case MV_TYPE_16X16:
        mpeg_motion_lowres_nv12(s, dest_y, dest_cb,
                    0, 0, 0,
                    ref_picture, pix_op,pix_op_nv12,
                    s->mv[dir][0][0], s->mv[dir][0][1], 2*block_s);
        break;
    case MV_TYPE_8X8:
        mx = 0;
        my = 0;
            for(i=0;i<4;i++) {
                hpel_motion_lowres(s, dest_y + ((i & 1) + (i >> 1) * s->linesize)*block_s,
                            ref_picture[0], 0, 0,
                            (2*mb_x + (i & 1))*block_s, (2*mb_y + (i >>1))*block_s,
                            s->width, s->height, s->linesize,
                            s->h_edge_pos >> lowres, s->v_edge_pos >> lowres,
                            block_s, block_s, pix_op,
                            s->mv[dir][i][0], s->mv[dir][i][1]);

                mx += s->mv[dir][i][0];
                my += s->mv[dir][i][1];
            }

        if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY))
            chroma_4mv_motion_lowres_nv12(s, dest_cb,  ref_picture, pix_op_nv12, mx, my);
        break;
    case MV_TYPE_FIELD:
        if (s->picture_structure == PICT_FRAME) {
            /* top field */
            mpeg_motion_lowres_nv12(s, dest_y, dest_cb,
                        1, 0, s->field_select[dir][0],
                        ref_picture, pix_op,pix_op_nv12,
                        s->mv[dir][0][0], s->mv[dir][0][1], block_s);
            /* bottom field */
            mpeg_motion_lowres_nv12(s, dest_y, dest_cb,
                        1, 1, s->field_select[dir][1],
                        ref_picture, pix_op,pix_op_nv12,
                        s->mv[dir][1][0], s->mv[dir][1][1], block_s);
        } else {
            if(s->picture_structure != s->field_select[dir][0] + 1 && s->pict_type != FF_B_TYPE && !s->first_field){
                ref_picture= s->current_picture_ptr->data;
            }

            mpeg_motion_lowres_nv12(s, dest_y, dest_cb,
                        0, 0, s->field_select[dir][0],
                        ref_picture, pix_op,pix_op_nv12,
                        s->mv[dir][0][0], s->mv[dir][0][1], 2*block_s);
        }
        break;
    case MV_TYPE_16X8:
        for(i=0; i<2; i++){
            uint8_t ** ref2picture;

            if(s->picture_structure == s->field_select[dir][i] + 1 || s->pict_type == FF_B_TYPE || s->first_field){
                ref2picture= ref_picture;
            }else{
                ref2picture= s->current_picture_ptr->data;
            }

            mpeg_motion_lowres_nv12(s, dest_y, dest_cb,
                        0, 0, s->field_select[dir][i],
                        ref2picture, pix_op,pix_op_nv12,
                        s->mv[dir][i][0], s->mv[dir][i][1] + 2*block_s*i, block_s);

            dest_y += 2*block_s*s->linesize;
            dest_cb+= (2*block_s>>s->chroma_y_shift)*s->uvlinesize;
        }
        break;
    case MV_TYPE_DMV:
        if(s->picture_structure == PICT_FRAME){
            for(i=0; i<2; i++){
                int j;
                for(j=0; j<2; j++){
                    mpeg_motion_lowres_nv12(s, dest_y, dest_cb,
                                1, j, j^i,
                                ref_picture, pix_op,pix_op_nv12,
                                s->mv[dir][2*i + j][0], s->mv[dir][2*i + j][1], block_s);
                }
                pix_op = s->dsp.avg_h264_chroma_pixels_tab;
                pix_op_nv12=s->dsp.avg_h264_chroma_pixels_tab_nv12;
            }
        }else{
            for(i=0; i<2; i++){
                mpeg_motion_lowres_nv12(s, dest_y, dest_cb,
                            0, 0, s->picture_structure != i+1,
                            ref_picture, pix_op,pix_op_nv12,
                            s->mv[dir][2*i][0],s->mv[dir][2*i][1],2*block_s);

                // after put we make avg of the same block
                pix_op = s->dsp.avg_h264_chroma_pixels_tab;

                //opposite parity is always in the same frame if this is second field
                if(!s->first_field){
                    ref_picture = s->current_picture_ptr->data;
                }
            }
        }
    break;
    default: assert(0);
    }
}

static inline void put_dct_nv12(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int qscale)
{
    s->dct_unquantize_intra(s, block, i, qscale);
    #ifdef __dump_DSP_TXT__
    memcpy(s->logdct_deqaun[i],block,128);
    memcpy(s->logdct_tmp,block,128);
    s->dsp.idct_put(s->logdct_result[i],8,s->logdct_tmp);
    #endif
    s->dsp.idct_put_nv12 (dest, line_size, block);
    #ifdef __dump_DSP_TXT__
    int indexy=0,indexx;
    for(indexy=0;indexy<8;indexy++,dest+=line_size)
    {
        for(indexx=0;indexx<8;indexx++)
            s->logdctmc_result[i][indexy*8+indexx]=dest[2*indexx];
    }
    #endif

}


/* put block[] to dest[] */
static inline void put_dct_amba(MpegEncContext *s,
                           DCTELEM *block,int i, uint8_t *dest, int line_size)
{

    #ifdef __dump_DSP_TXT__
    memcpy(s->logdct_deqaun[i],block,128);
    memcpy(s->logdct_tmp,block,128);
    s->dsp.idct_put(s->logdct_result[i],8,s->logdct_tmp);
    #endif
    s->dsp.idct_put (dest, line_size, block);
    #ifdef __dump_DSP_TXT__
    int indexy=0,indexx;
    for(indexy=0;indexy<8;indexy++,dest+=line_size)
    {
        for(indexx=0;indexx<8;indexx++)
            s->logdctmc_result[i][indexy*8+indexx]=dest[indexx];
    }
    #endif

}

static inline void put_dct_nv12_amba(MpegEncContext *s,
                           DCTELEM *block,int i, uint8_t *dest, int line_size)
{
    #ifdef __dump_DSP_TXT__
    memcpy(s->logdct_deqaun[i],block,128);
    memcpy(s->logdct_tmp,block,128);
    s->dsp.idct_put(s->logdct_result[i],8,s->logdct_tmp);
    #endif
    s->dsp.idct_put_nv12 (dest, line_size, block);
   #ifdef __dump_DSP_TXT__
    int indexy=0,indexx;
    for(indexy=0;indexy<8;indexy++,dest+=line_size)
    {
        for(indexx=0;indexx<8;indexx++)
            s->logdctmc_result[i][indexy*8+indexx]=dest[2*indexx];
    }
    #endif

}

static inline void add_dct_nv12(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size)
{
    if (s->block_last_index[i] >= 0) {
        #ifdef __dump_DSP_TXT__
        memcpy(s->logdct_deqaun[i],block,128);
        memcpy(s->logdct_tmp,block,128);
        s->dsp.idct_put(s->logdct_result[i],8,s->logdct_tmp);
        #endif
        s->dsp.idct_add_nv12 (dest, line_size, block);
    }
    #ifdef __dump_DSP_TXT__
    int indexy=0,indexx;
    for(indexy=0;indexy<8;indexy++,dest+=line_size)
    {
        for(indexx=0;indexx<8;indexx++)
            s->logdctmc_result[i][indexy*8+indexx]=dest[2*indexx];
    }
    #endif
}

static inline void add_dequant_dct_nv12(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int qscale)
{
    if (s->block_last_index[i] >= 0) {
        s->dct_unquantize_inter(s, block, i, qscale);
        #ifdef __dump_DSP_TXT__
        memcpy(s->logdct_deqaun[i],block,128);
        memcpy(s->logdct_tmp,block,128);
        s->dsp.idct_put(s->logdct_result[i],8,s->logdct_tmp);
        #endif
        s->dsp.idct_add_nv12 (dest, line_size, block);
    }

    #ifdef __dump_DSP_TXT__
    int indexy=0,indexx;
    for(indexy=0;indexy<8;indexy++,dest+=line_size)
    {
        for(indexx=0;indexx<8;indexx++)
            s->logdctmc_result[i][indexy*8+indexx]=dest[2*indexx];
    }
    #endif

}

static inline void add_dct_amba(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int have_idct)
{
    if(have_idct)
    {
        #ifdef __dump_DSP_TXT__
        memcpy(s->logdct_deqaun[i],block,128);
        memcpy(s->logdct_tmp,block,128);
        s->dsp.idct_put(s->logdct_result[i],8,s->logdct_tmp);
        #endif
        s->dsp.idct_add (dest, line_size, block);
    }

    #ifdef __dump_DSP_TXT__
    int indexy=0,indexx;
    for(indexy=0;indexy<8;indexy++,dest+=line_size)
    {
        for(indexx=0;indexx<8;indexx++)
            s->logdctmc_result[i][indexy*8+indexx]=dest[indexx];
    }
    #endif

}

static inline void add_dct_nv12_amba(MpegEncContext *s,
                           DCTELEM *block, int i, uint8_t *dest, int line_size, int have_idct)
{
    if(have_idct)
    {
        #ifdef __dump_DSP_TXT__
        memcpy(s->logdct_deqaun[i],block,128);
        memcpy(s->logdct_tmp,block,128);
        s->dsp.idct_put(s->logdct_result[i],8,s->logdct_tmp);
        #endif
        s->dsp.idct_add_nv12 (dest, line_size, block);
    }

    #ifdef __dump_DSP_TXT__
    int indexy=0,indexx;
    for(indexy=0;indexy<8;indexy++,dest+=line_size)
    {
        for(indexx=0;indexx<8;indexx++)
            s->logdctmc_result[i][indexy*8+indexx]=dest[2*indexx];
    }
    #endif

}


static av_always_inline
void MPV_decode_mb_internal_nv12(MpegEncContext *s, DCTELEM block[12][64],
                            int is_mpeg12)
{
    const int mb_xy = s->mb_y * s->mb_stride + s->mb_x;
    if(CONFIG_MPEG_XVMC_DECODER && s->avctx->xvmc_acceleration){
        ff_xvmc_decode_mb(s);//xvmc uses pblocks
        return;
    }

#ifdef __dump_DSP_TXT__
#ifdef __dump_separate__
char title[40];
snprintf(title,39,"mb [y=%d, x=%d]:\n",s->mb_y,s->mb_x);
log_text_p(log_fd_dsp_text+log_offset_mc,title,log_cur_frame);
#endif
#endif

    if(s->avctx->debug&FF_DEBUG_DCT_COEFF) {
       /* save DCT coefficients */
       int i,j;
       DCTELEM *dct = &s->current_picture.dct_coeff[mb_xy*64*6];
       for(i=0; i<6; i++)
           for(j=0; j<64; j++)
               *dct++ = block[i][s->dsp.idct_permutation[j]];
    }

    s->current_picture.qscale_table[mb_xy]= s->qscale;

//    av_log(NULL,AV_LOG_ERROR,"s->mb_y=%d,s->mb_x=%d.\n",s->mb_y,s->mb_x);

    /* update DC predictors for P macroblocks */
    if (!s->mb_intra) {
        if (!is_mpeg12 && (s->h263_pred || s->h263_aic)) {
            if(s->mbintra_table[mb_xy])
            {
//                av_log(NULL,AV_LOG_ERROR,"!!!hit.\n");
                ff_clean_intra_table_entries(s);
            }
        } else {
//            av_log(NULL,AV_LOG_ERROR,"!!!hit 1.\n");
            s->last_dc[0] =
            s->last_dc[1] =
            s->last_dc[2] = 128 << s->intra_dc_precision;
        }
    }
    else if (!is_mpeg12 && (s->h263_pred || s->h263_aic))
        s->mbintra_table[mb_xy]=1;

    if ((s->flags&CODEC_FLAG_PSNR) || !(s->encoding && (s->intra_only || s->pict_type==FF_B_TYPE) && s->avctx->mb_decision != FF_MB_DECISION_RD)) { //FIXME precalc
        uint8_t *dest_y, *dest_cb;//, *dest_cr;
        int dct_linesize, dct_offset;
        op_pixels_func (*op_pix)[4];
        op_pixels_func (*op_pix_nv12)[4];
        qpel_mc_func (*op_qpix)[16];
        const int linesize= s->current_picture.linesize[0]; //not s->linesize as this would be wrong for field pics
        const int uvlinesize= s->current_picture.linesize[1];
        const int readable= s->pict_type != FF_B_TYPE || s->encoding || s->avctx->draw_horiz_band || s->avctx->lowres ||1;
        const int block_size= s->avctx->lowres ? 8>>s->avctx->lowres : 8;

        ambadec_assert_ffmpeg(readable);
        #ifdef __dump_DSP_test_data__
            ambadec_assert_ffmpeg(s->current_picture_ptr->linesize[0]==s->linesize);
            ambadec_assert_ffmpeg(s->current_picture_ptr->linesize[1]==s->uvlinesize);
            ambadec_assert_ffmpeg(s->uvlinesize==s->linesize);

            uint8_t* pcheck=s->current_picture_ptr->data[0]+(s->mb_y*16)*s->current_picture_ptr->linesize[0]+(s->mb_x*16);
            ambadec_assert_ffmpeg(s->dest[0]==pcheck);
            pcheck=s->current_picture_ptr->data[1]+(s->mb_y*8)*s->current_picture_ptr->linesize[1]+(s->mb_x*16);
            ambadec_assert_ffmpeg(s->dest[1]==pcheck);
        #endif

        /* avoid copy if macroblock skipped in last frame too */
        /* skip only during decoding as we might trash the buffers during encoding a bit */
        if(!s->encoding){
            uint8_t *mbskip_ptr = &s->mbskip_table[mb_xy];
            const int age= s->current_picture.age;

            assert(age);

            if (s->mb_skipped) {
                s->mb_skipped= 0;
                assert(s->pict_type!=FF_I_TYPE);

                (*mbskip_ptr) ++; /* indicate that this time we skipped it */
                if(*mbskip_ptr >99) *mbskip_ptr= 99;

                /* if previous was skipped too, then nothing to do !  */
                if (*mbskip_ptr >= age && s->current_picture.reference){
                    return;
                }
            } else if(!s->current_picture.reference){
                (*mbskip_ptr) ++; /* increase counter so the age can be compared cleanly */
                if(*mbskip_ptr >99) *mbskip_ptr= 99;
            } else{
                *mbskip_ptr = 0; /* not skipped */
            }
        }

        dct_linesize = linesize << s->interlaced_dct;
        dct_offset =(s->interlaced_dct)? linesize : linesize*block_size;

        if(readable){
            dest_y=  s->dest[0];
            dest_cb= s->dest[1];
//            dest_cr= s->dest[2];
        }else{
            dest_y = s->b_scratchpad;
            dest_cb= s->b_scratchpad+16*linesize;
//            dest_cr= s->b_scratchpad+32*linesize;
        }

        if (!s->mb_intra) {
            /* motion handling */
            /* decoding or more than one mb_type (MC was already done otherwise) */
            if(!s->encoding){
                if(s->avctx->lowres){
                    ambadec_assert_ffmpeg(0);
                    av_log(NULL,AV_LOG_ERROR,"lowres not supported.\n");
                    h264_chroma_mc_func *op_pix = s->dsp.put_h264_chroma_pixels_tab;
                    h264_chroma_mc_func *op_pix_nv12 = s->dsp.put_h264_chroma_pixels_tab_nv12;

                    if (s->mv_dir & MV_DIR_FORWARD) {
                        MPV_motion_lowres_nv12(s, dest_y, dest_cb, 0, s->last_picture.data, op_pix,op_pix_nv12);
                        op_pix = s->dsp.avg_h264_chroma_pixels_tab;
                        op_pix_nv12 = s->dsp.avg_h264_chroma_pixels_tab_nv12;
                    }
                    if (s->mv_dir & MV_DIR_BACKWARD) {
                        MPV_motion_lowres_nv12(s, dest_y, dest_cb, 1, s->next_picture.data, op_pix,op_pix_nv12);
                    }
                }else{
                    op_qpix= s->me.qpel_put;
                    if ((!s->no_rounding) || s->pict_type==FF_B_TYPE){
                        op_pix = s->dsp.put_pixels_tab;
                        op_pix_nv12 = s->dsp.put_pixels_tab_nv12;
                        #ifdef __dump_DSP_TXT__
                            log_text(log_fd_dsp_text, "mc with no rounding");
                            #ifdef __dump_separate__
                                log_text_p(log_fd_dsp_text+log_offset_mc, "mc with no rounding",log_cur_frame);
                            #endif
                        #endif
                    }else{
                        op_pix = s->dsp.put_no_rnd_pixels_tab;
                        op_pix_nv12 = s->dsp.put_no_rnd_pixels_tab_nv12;
                        #ifdef __dump_DSP_TXT__
                            log_text(log_fd_dsp_text, "mc with rounding");
                            #ifdef __dump_separate__
                                log_text_p(log_fd_dsp_text+log_offset_mc,  "mc with rounding",log_cur_frame);
                            #endif

                        #endif
                    }
                    if (s->mv_dir & MV_DIR_FORWARD) {
                        MPV_motion_nv12(s, dest_y, dest_cb, 0, s->last_picture.data, op_pix,op_pix_nv12, op_qpix);
                        op_pix = s->dsp.avg_pixels_tab;
                        op_pix_nv12 = s->dsp.avg_pixels_tab_nv12;
                        op_qpix= s->me.qpel_avg;

                        #ifdef __dump_DSP_TXT__
                        log_text(log_fd_dsp_text, "After Forward MC, result is:");
                        log_text(log_fd_dsp_text, "Y:");
                        log_text_rect_char_hex(log_fd_dsp_text,dest_y,16,16,s->linesize-16);
                        log_text(log_fd_dsp_text, "Cb:");
                        log_text_rect_char_hex_chroma(log_fd_dsp_text,dest_cb,8,8,s->uvlinesize-16);
                        log_text(log_fd_dsp_text, "Cr:");
                        log_text_rect_char_hex_chroma(log_fd_dsp_text,dest_cb+1,8,8,s->uvlinesize-16);
                            #ifdef __dump_separate__
                            log_text_p(log_fd_dsp_text+log_offset_mc, "After Forward MC, result is:",log_cur_frame);
                            log_text_p(log_fd_dsp_text+log_offset_mc, "Y:",log_cur_frame);
                            log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc,dest_y,16,16,s->linesize-16,log_cur_frame);
                            log_text_p(log_fd_dsp_text+log_offset_mc, "Cb:",log_cur_frame);
                            log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb,8,8,s->uvlinesize-16,log_cur_frame);
                            log_text_p(log_fd_dsp_text+log_offset_mc, "Cr:",log_cur_frame);
                            log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb+1,8,8,s->uvlinesize-16,log_cur_frame);
                            #endif
                        #endif

                    }
                    if (s->mv_dir & MV_DIR_BACKWARD) {
                        MPV_motion_nv12(s, dest_y, dest_cb, 1, s->next_picture.data, op_pix,op_pix_nv12, op_qpix);

                        #ifdef __dump_DSP_TXT__
                        log_text(log_fd_dsp_text, "After Backward MC, result is:");
                        log_text(log_fd_dsp_text, "Y:");
                        log_text_rect_char_hex(log_fd_dsp_text,dest_y,16,16,s->linesize-16);
                        log_text(log_fd_dsp_text, "Cb:");
                        log_text_rect_char_hex_chroma(log_fd_dsp_text,dest_cb,8,8,s->uvlinesize-16);
                        log_text(log_fd_dsp_text, "Cr:");
                        log_text_rect_char_hex_chroma(log_fd_dsp_text,dest_cb+1,8,8,s->uvlinesize-16);
                            #ifdef __dump_separate__
                            log_text_p(log_fd_dsp_text+log_offset_mc, "After Backward MC, result is:",log_cur_frame);
                            log_text_p(log_fd_dsp_text+log_offset_mc, "Y:",log_cur_frame);
                            log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc,dest_y,16,16,s->linesize-16,log_cur_frame);
                            log_text_p(log_fd_dsp_text+log_offset_mc, "Cb:",log_cur_frame);
                            log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb,8,8,s->uvlinesize-16,log_cur_frame);
                            log_text_p(log_fd_dsp_text+log_offset_mc, "Cr:",log_cur_frame);
                            log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb+1,8,8,s->uvlinesize-16,log_cur_frame);
                            #endif
                        #endif

                    }
                }
            }
            /* skip dequant / idct if we are really late ;) */
#if FF_API_HURRY_UP
            if(s->hurry_up>1) goto skip_idct;
#endif
            if(s->avctx->skip_idct){
                if(  (s->avctx->skip_idct >= AVDISCARD_NONREF && s->pict_type == FF_B_TYPE)
                   ||(s->avctx->skip_idct >= AVDISCARD_NONKEY && s->pict_type != FF_I_TYPE)
                   || s->avctx->skip_idct >= AVDISCARD_ALL)
                    goto skip_idct;
            }

            /* add dct residue */
            if(s->encoding || !(   s->h263_msmpeg4 || s->codec_id==CODEC_ID_MPEG1VIDEO || s->codec_id==CODEC_ID_MPEG2VIDEO
                                || (s->codec_id==CODEC_ID_AMBA_MPEG4 && !s->mpeg_quant))){
                add_dequant_dct(s, block[0], 0, dest_y                          , dct_linesize, s->qscale);
                add_dequant_dct(s, block[1], 1, dest_y              + block_size, dct_linesize, s->qscale);
                add_dequant_dct(s, block[2], 2, dest_y + dct_offset             , dct_linesize, s->qscale);
                add_dequant_dct(s, block[3], 3, dest_y + dct_offset + block_size, dct_linesize, s->qscale);

                if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                    if (s->chroma_y_shift){
                        add_dequant_dct_nv12(s, block[4], 4, dest_cb, uvlinesize, s->chroma_qscale);
                        add_dequant_dct_nv12(s, block[5], 5, dest_cb+1, uvlinesize, s->chroma_qscale);
                    }else{
                        //chroma422
                        //not support here
                        //ambadec_assert_ffmpeg(0);
                        dct_linesize >>= 1;
                        dct_offset >>=1;
                        add_dequant_dct_nv12(s, block[4], 4, dest_cb,              dct_linesize, s->chroma_qscale);
                        add_dequant_dct_nv12(s, block[5], 5, dest_cb+1,              dct_linesize, s->chroma_qscale);
                        add_dequant_dct_nv12(s, block[6], 6, dest_cb + dct_offset, dct_linesize, s->chroma_qscale);
                        add_dequant_dct_nv12(s, block[7], 7, dest_cb+1 + dct_offset, dct_linesize, s->chroma_qscale);
                    }
                }
            } else if(is_mpeg12 || (s->codec_id != CODEC_ID_WMV2)){
                add_dct(s, block[0], 0, dest_y                          , dct_linesize);
                add_dct(s, block[1], 1, dest_y              + block_size, dct_linesize);
                add_dct(s, block[2], 2, dest_y + dct_offset             , dct_linesize);
                add_dct(s, block[3], 3, dest_y + dct_offset + block_size, dct_linesize);

                if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                    if(s->chroma_y_shift){//Chroma420
                        add_dct_nv12(s, block[4], 4, dest_cb, uvlinesize);
                        add_dct_nv12(s, block[5], 5, dest_cb+1, uvlinesize);
                    }else{
                        //chroma422
                         //not support here
                        //ambadec_assert_ffmpeg(0);
                        dct_linesize = uvlinesize << s->interlaced_dct;
                        dct_offset =(s->interlaced_dct)? uvlinesize : uvlinesize*8;

                        add_dct_nv12(s, block[4], 4, dest_cb, dct_linesize);
                        add_dct_nv12(s, block[5], 5, dest_cb+1, dct_linesize);
                        add_dct_nv12(s, block[6], 6, dest_cb+dct_offset, dct_linesize);
                        add_dct_nv12(s, block[7], 7, dest_cb+1+dct_offset, dct_linesize);
                        if(!s->chroma_x_shift){//Chroma444
                            add_dct_nv12(s, block[8], 8, dest_cb+8, dct_linesize);
                            add_dct_nv12(s, block[9], 9, dest_cb+9, dct_linesize);
                            add_dct_nv12(s, block[10], 10, dest_cb+8+dct_offset, dct_linesize);
                            add_dct_nv12(s, block[11], 11, dest_cb+9+dct_offset, dct_linesize);
                        }
                    }
                }//fi gray
            }
            else if (CONFIG_WMV2_DECODER || CONFIG_WMV2_ENCODER) {
                av_log(NULL,AV_LOG_ERROR,"should not come here.\n");
                ff_wmv2_add_mb(s, block, dest_y, dest_cb, dest_cb+1);
            }
        } else {
            /* dct only in intra block */
            if(s->encoding || !(s->codec_id==CODEC_ID_MPEG1VIDEO || s->codec_id==CODEC_ID_MPEG2VIDEO)){
                put_dct(s, block[0], 0, dest_y                          , dct_linesize, s->qscale);
                put_dct(s, block[1], 1, dest_y              + block_size, dct_linesize, s->qscale);
                put_dct(s, block[2], 2, dest_y + dct_offset             , dct_linesize, s->qscale);
                put_dct(s, block[3], 3, dest_y + dct_offset + block_size, dct_linesize, s->qscale);

                if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                    if(s->chroma_y_shift){
                        put_dct_nv12(s, block[4], 4, dest_cb, uvlinesize, s->chroma_qscale);
                        put_dct_nv12(s, block[5], 5, dest_cb+1, uvlinesize, s->chroma_qscale);
                    }else{
                        //chroma422
                        //not support here
                        //ambadec_assert_ffmpeg(0);
                        dct_offset >>=1;
                        dct_linesize >>=1;
                        put_dct_nv12(s, block[4], 4, dest_cb,              dct_linesize, s->chroma_qscale);
                        put_dct_nv12(s, block[5], 5, dest_cb+1,              dct_linesize, s->chroma_qscale);
                        put_dct_nv12(s, block[6], 6, dest_cb + dct_offset, dct_linesize, s->chroma_qscale);
                        put_dct_nv12(s, block[7], 7, dest_cb+1 + dct_offset, dct_linesize, s->chroma_qscale);
                    }
                }
            }else{
                av_log(NULL,AV_LOG_ERROR,"should not come here 2.\n");
                s->dsp.idct_put(dest_y                          , dct_linesize, block[0]);
                s->dsp.idct_put(dest_y              + block_size, dct_linesize, block[1]);
                s->dsp.idct_put(dest_y + dct_offset             , dct_linesize, block[2]);
                s->dsp.idct_put(dest_y + dct_offset + block_size, dct_linesize, block[3]);

                if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                    if(s->chroma_y_shift){
                        s->dsp.idct_put_nv12(dest_cb, uvlinesize, block[4]);
                        s->dsp.idct_put_nv12(dest_cb+1, uvlinesize, block[5]);
                    }else{
                        //chroma422
                        //not support here
                        //ambadec_assert_ffmpeg(0);
                        dct_linesize = uvlinesize << s->interlaced_dct;
                        dct_offset =(s->interlaced_dct)? uvlinesize : uvlinesize*8;

                        s->dsp.idct_put_nv12(dest_cb,              dct_linesize, block[4]);
                        s->dsp.idct_put_nv12(dest_cb+1,              dct_linesize, block[5]);
                        s->dsp.idct_put_nv12(dest_cb + dct_offset, dct_linesize, block[6]);
                        s->dsp.idct_put_nv12(dest_cb+1 + dct_offset, dct_linesize, block[7]);
                        if(!s->chroma_x_shift){//Chroma444
                            s->dsp.idct_put_nv12(dest_cb + 8,              dct_linesize, block[8]);
                            s->dsp.idct_put_nv12(dest_cb+9,              dct_linesize, block[9]);
                            s->dsp.idct_put_nv12(dest_cb + 8 + dct_offset, dct_linesize, block[10]);
                            s->dsp.idct_put_nv12(dest_cb+9 + dct_offset, dct_linesize, block[11]);
                        }
                    }
                }//gray
            }
        }
skip_idct:
        if(!readable){
            av_log(NULL,AV_LOG_ERROR,"should not come here 3.\n");
            s->dsp.put_pixels_tab[0][0](s->dest[0], dest_y ,   linesize,16);
            s->dsp.put_pixels_tab_nv12[s->chroma_x_shift][0](s->dest[1], dest_cb, uvlinesize,16 >> s->chroma_y_shift);
            s->dsp.put_pixels_tab_nv12[s->chroma_x_shift][0](s->dest[2], dest_cb+1, uvlinesize,16 >> s->chroma_y_shift);
        }
    }

#ifdef __dump_DSP_TXT__
int logi=0;
char logt[80];

#ifdef __dump_separate__
snprintf(title,39,"mb [y=%d, x=%d]:\n",s->mb_y,s->mb_x);
log_text_p(log_fd_dsp_text+log_offset_dct,title,log_cur_frame);
#endif

for(logi=0;logi<6;logi++)
{
    snprintf(logt,79,"{ SubBlock: %d }",logi);
    log_text(log_fd_dsp_text, logt);
    log_text(log_fd_dsp_text,"<< Before Inverse Quantization: >>");
    log_text_rect_short(log_fd_dsp_text,s->logdct[logi],8,8,0);
    log_text(log_fd_dsp_text,"<< After Inverse DCAC Prediction: >>");
    log_text_rect_short(log_fd_dsp_text,s->logdct_acdc[logi],8,8,0);
    log_text(log_fd_dsp_text,"<< After Inverse Quantization: >>");
    log_text_rect_short(log_fd_dsp_text,s->logdct_deqaun[logi],8,8,0);
    log_text(log_fd_dsp_text,"<< After IDCT: >>");
    log_text_rect_char_hex(log_fd_dsp_text,s->logdct_result[logi],8,8,0);
    log_text(log_fd_dsp_text,"<< After IDCT+MC: >>");
    log_text_rect_char_hex(log_fd_dsp_text,s->logdctmc_result[logi],8,8,0);
    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_dct,"<< After Inverse Quantization: >>",log_cur_frame);
    log_text_rect_short_p(log_fd_dsp_text+log_offset_dct,s->logdct_deqaun[logi],8,8,0,log_cur_frame);
    log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT: >>",log_cur_frame);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdct_result[logi],8,8,0,log_cur_frame);
    log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT+MC: >>",log_cur_frame);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdctmc_result[logi],8,8,0,log_cur_frame);
    #endif
}
#endif

}

void MPV_decode_mb_internal_amba(MpegEncContext *s)
{
    const int mb_xy = s->mb_y * s->mb_stride + s->mb_x;
//    static int cnt=0;

//    av_log(NULL,AV_LOG_ERROR,"s->mb_y=%d,s->mb_x=%d.\n",s->mb_y,s->mb_x);

//    s->current_picture_ptr->qscale_table[mb_xy]= s->qscale;

    /* update DC predictors for P macroblocks */
    if (!s->mb_intra) {
        if (s->h263_pred || s->h263_aic) {
            if(s->mbintra_table[mb_xy])
            {
 //               av_log(NULL,AV_LOG_ERROR,"!!!hit.\n");
                ff_clean_intra_table_entries(s);
            }
        } else {
//             av_log(NULL,AV_LOG_ERROR,"!!!hit 1.\n");
            s->last_dc[0] =
            s->last_dc[1] =
            s->last_dc[2] = 128 << s->intra_dc_precision;
        }
    }
    else if (s->h263_pred || s->h263_aic)
        s->mbintra_table[mb_xy]=1;


    uint8_t *mbskip_ptr = &s->mbskip_table[mb_xy];
    const int age= s->current_picture_ptr->age;

    assert(age);

    if (s->mb_skipped) {
        s->mb_skipped= 0;
        assert(s->pict_type!=FF_I_TYPE);

        (*mbskip_ptr) ++; /* indicate that this time we skipped it */
        if(*mbskip_ptr >99) *mbskip_ptr= 99;

        /* if previous was skipped too, then nothing to do !  */
        if (*mbskip_ptr >= age && s->current_picture_ptr->reference){
            return;
        }
    } else if(!s->current_picture_ptr->reference){
        (*mbskip_ptr) ++; /* increase counter so the age can be compared cleanly */
        if(*mbskip_ptr >99) *mbskip_ptr= 99;
    } else{
        *mbskip_ptr = 0; /* not skipped */
    }

}

void MPV_decode_mb_nv12(MpegEncContext *s, DCTELEM block[12][64])
{
#if !CONFIG_SMALL
    if(s->out_format == FMT_MPEG1) {
        if(s->avctx->lowres) MPV_decode_mb_internal(s, block, 1, 1);
        else                 MPV_decode_mb_internal(s, block, 0, 1);
    } else
#endif
    if(s->avctx->lowres) MPV_decode_mb_internal_nv12(s, block, 0);
    else                  MPV_decode_mb_internal_nv12(s, block, 0);
}

typedef DCTELEM(* dctarray)[6][64];

//need convert to nv12
static inline void gmc1_motion_amba (MpegEncContext *s, h263_pic_data_t* p_pic, uint8_t *dest_y, uint8_t *dest_cb,uint8_t **ref_picture,int mb_x, int mb_y)
{
    uint8_t *ptr;
    int offset, src_x, src_y, linesize, uvlinesize;
    int motion_x, motion_y;
    //int emu=0;

    #ifdef __dump_DSP_TXT__
    char title[60];
    #endif

    motion_x= p_pic->sprite_offset[0][0];
    motion_y= p_pic->sprite_offset[0][1];
    src_x = mb_x * 16 + (motion_x >> (p_pic->sprite_warping_accuracy+1));
    src_y = mb_y * 16 + (motion_y >> (p_pic->sprite_warping_accuracy+1));
    motion_x<<=(3-p_pic->sprite_warping_accuracy);
    motion_y<<=(3-p_pic->sprite_warping_accuracy);
    src_x = av_clip(src_x, -16, p_pic->width);
    if (src_x == p_pic->width)
        motion_x =0;
    src_y = av_clip(src_y, -16, p_pic->height);
    if (src_y == p_pic->height)
        motion_y =0;

    linesize = p_pic->current_picture_ptr->linesize[0];
    uvlinesize = p_pic->current_picture_ptr->linesize[1];

    ptr = ref_picture[0] + (src_y * linesize) + src_x;

    #ifdef __dump_DSP_TXT__
    //    log_text(log_fd_dsp_text, "in gmc1:");
    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_mc, "in gmc1:",p_pic->frame_cnt);
    snprintf(title,59,"motion_x=%d,motion_y=%d,ptr offset=%d.\n",motion_x,motion_y,ptr-ref_picture[0]);
    log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);
    snprintf(title,59,"src_x=%d,src_y=%d,linesize=%d, uvlinesize=%d.\n",src_x,src_y,linesize,uvlinesize);
    log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);
    snprintf(title,59,"(motion_x|motion_y)&7=%d.\n",(motion_x|motion_y)&7);
    log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text+log_offset_mc,"y reference:\n",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc,ptr,16,16,linesize-16,p_pic->frame_cnt);
    #endif
    #endif

    if(s->flags&CODEC_FLAG_EMU_EDGE){
        if(   (unsigned)src_x >= p_pic->h_edge_pos - 17
           || (unsigned)src_y >= p_pic->v_edge_pos - 17){
            s->dsp.emulated_edge_mc(s->edge_emu_buffer, ptr, linesize, 17, 17, src_x, src_y, p_pic->h_edge_pos, p_pic->v_edge_pos);
            ptr= s->edge_emu_buffer;
            #ifdef __dump_separate__
            log_text_p(log_fd_dsp_text+log_offset_mc,"y emu",p_pic->frame_cnt);
            #endif
        }
    }

    if((motion_x|motion_y)&7){
        s->dsp.gmc1(dest_y  , ptr  , linesize, 16, motion_x&15, motion_y&15, 128 - p_pic->no_rounding);
        s->dsp.gmc1(dest_y+8, ptr+8, linesize, 16, motion_x&15, motion_y&15, 128 - p_pic->no_rounding);
    }else{
        int dxy;

        dxy= ((motion_x>>3)&1) | ((motion_y>>2)&2);
        if (p_pic->no_rounding){
            s->dsp.put_no_rnd_pixels_tab[0][dxy](dest_y, ptr, linesize, 16);
        }else{
            s->dsp.put_pixels_tab       [0][dxy](dest_y, ptr, linesize, 16);
        }
    }

    if(CONFIG_GRAY && s->flags&CODEC_FLAG_GRAY) return;

    motion_x= p_pic->sprite_offset[1][0];
    motion_y= p_pic->sprite_offset[1][1];
    src_x = mb_x * 8 + (motion_x >> (p_pic->sprite_warping_accuracy+1));
    src_y = mb_y * 8 + (motion_y >> (p_pic->sprite_warping_accuracy+1));
    motion_x<<=(3-p_pic->sprite_warping_accuracy);
    motion_y<<=(3-p_pic->sprite_warping_accuracy);
    src_x = av_clip(src_x, -8, p_pic->width>>1);
    if (src_x == p_pic->width>>1)
        motion_x =0;
    src_y = av_clip(src_y, -8, p_pic->height>>1);
    if (src_y == p_pic->height>>1)
        motion_y =0;

    offset = (src_y * uvlinesize) + src_x*2;
    ptr = ref_picture[1] + offset;

    #ifdef __dump_DSP_TXT__
    #ifdef __dump_separate__
    snprintf(title,59,"uv motion_x=%d,motion_y=%d,ptr offset=%d.\n",motion_x,motion_y,ptr-ref_picture[1]);
    log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);
    snprintf(title,59,"src_x=%d,src_y=%d,linesize=%d, uvlinesize=%d.\n",src_x,src_y,linesize,uvlinesize);
    log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text+log_offset_mc,"uv reference:\n",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc,ptr,16,8,uvlinesize-16,p_pic->frame_cnt);
    #endif
    #endif

    if(s->flags&CODEC_FLAG_EMU_EDGE){
        if(   (unsigned)src_x >= (p_pic->h_edge_pos>>1) - 9
           || (unsigned)src_y >= (p_pic->v_edge_pos>>1) - 9){
            s->dsp.emulated_edge_mc_nv12(s->edge_emu_buffer, ptr, uvlinesize, 9, 9, src_x, src_y, p_pic->h_edge_pos>>1, p_pic->v_edge_pos>>1);
            ptr= s->edge_emu_buffer;
            #ifdef __dump_separate__
            log_text_p(log_fd_dsp_text+log_offset_mc,"uv emu",p_pic->frame_cnt);
            #endif
            //emu=1;
        }
    }
    s->dsp.gmc1_nv12(dest_cb, ptr, uvlinesize, 8, motion_x&15, motion_y&15, 128 - p_pic->no_rounding);
    s->dsp.gmc1_nv12(dest_cb+1, ptr+1, uvlinesize, 8, motion_x&15, motion_y&15, 128 - p_pic->no_rounding);
    return;
}


//need convert to nv12
static inline void gmc_motion_amba(MpegEncContext *s,h263_pic_data_t* p_pic,
                               uint8_t *dest_y, uint8_t *dest_cb,
                               uint8_t **ref_picture,int mb_x,int mb_y)
{
    uint8_t *ptr;
    const int linesize=p_pic->current_picture_ptr->linesize[0], uvlinesize=p_pic->current_picture_ptr->linesize[1];
    const int a= p_pic->sprite_warping_accuracy;
    int ox, oy;

    #ifdef __dump_DSP_TXT__
    char title[60];
    #endif

    ptr = ref_picture[0];

    ox= p_pic->sprite_offset[0][0] + p_pic->sprite_delta[0][0]*mb_x*16 + p_pic->sprite_delta[0][1]*mb_y*16;
    oy= p_pic->sprite_offset[0][1] + p_pic->sprite_delta[1][0]*mb_x*16 + p_pic->sprite_delta[1][1]*mb_y*16;

    #ifdef __dump_DSP_TXT__
    //    log_text(log_fd_dsp_text, "in gmc:");
    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_mc, "in gmc:",p_pic->frame_cnt);
    snprintf(title,59,"ox=%d,ox=%d,sprite_delta [0][0]=%d,[0][1]=%d.\n",ox,oy,p_pic->sprite_delta[0][0],p_pic->sprite_delta[0][1]);
    log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);
    snprintf(title,59,"sprite_delta [1][0]=%d,[1][1]=%d.\n",ox,oy,p_pic->sprite_delta[1][0],p_pic->sprite_delta[1][1]);
    log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);
    #endif
    #endif

    s->dsp.gmc(dest_y, ptr, linesize, 16,
           ox,
           oy,
           p_pic->sprite_delta[0][0], p_pic->sprite_delta[0][1],
           p_pic->sprite_delta[1][0], p_pic->sprite_delta[1][1],
           a+1, (1<<(2*a+1)) - p_pic->no_rounding,
           p_pic->h_edge_pos, p_pic->v_edge_pos);
    s->dsp.gmc(dest_y+8, ptr, linesize, 16,
           ox + p_pic->sprite_delta[0][0]*8,
           oy + p_pic->sprite_delta[1][0]*8,
           p_pic->sprite_delta[0][0], p_pic->sprite_delta[0][1],
           p_pic->sprite_delta[1][0], p_pic->sprite_delta[1][1],
           a+1, (1<<(2*a+1)) - p_pic->no_rounding,
           p_pic->h_edge_pos, p_pic->v_edge_pos);

    if(CONFIG_GRAY && s->flags&CODEC_FLAG_GRAY) return;

    ox= p_pic->sprite_offset[1][0] + p_pic->sprite_delta[0][0]*mb_x*8 + p_pic->sprite_delta[0][1]*mb_y*8;
    oy= p_pic->sprite_offset[1][1] + p_pic->sprite_delta[1][0]*mb_x*8 + p_pic->sprite_delta[1][1]*mb_y*8;

    ptr = ref_picture[1];
    s->dsp.gmc_nv12(dest_cb, ptr, uvlinesize, 8,
           ox,
           oy,
           p_pic->sprite_delta[0][0], p_pic->sprite_delta[0][1],
           p_pic->sprite_delta[1][0], p_pic->sprite_delta[1][1],
           a+1, (1<<(2*a+1)) - p_pic->no_rounding,
           p_pic->h_edge_pos>>1, p_pic->v_edge_pos>>1);
    s->dsp.gmc_nv12(dest_cb+1, ptr+1, uvlinesize, 8,
           ox,
           oy,
           p_pic->sprite_delta[0][0], p_pic->sprite_delta[0][1],
           p_pic->sprite_delta[1][0], p_pic->sprite_delta[1][1],
           a+1, (1<<(2*a+1)) - p_pic->no_rounding,
           p_pic->h_edge_pos>>1, p_pic->v_edge_pos>>1);

}

static inline void qpel_motion_amba(MpegEncContext *s,h263_pic_data_t* p_pic,
                               uint8_t *dest_y, uint8_t *dest_cb,
                               int field_based, int bottom_field, int field_select,
                               uint8_t **ref_picture, op_pixels_func (*pix_op)[4],op_pixels_func (*pix_op_nv12)[4],
                               qpel_mc_func (*qpix_op)[16],
                               int motion_x, int motion_y, int h, int mb_x,int mb_y)
{
    uint8_t *ptr_y, *ptr_cb;//, *ptr_cr;
    int dxy, uvdxy, mx, my, src_x, src_y, uvsrc_x, uvsrc_y, v_edge_pos;
    const int linesize=p_pic->current_picture_ptr->linesize[0] << field_based, uvlinesize= p_pic->current_picture_ptr->linesize[1]  << field_based;

    dxy = ((motion_y & 3) << 2) | (motion_x & 3);
    src_x = mb_x *  16                 + (motion_x >> 2);
    src_y = mb_y * (16 >> field_based) + (motion_y >> 2);

    v_edge_pos = p_pic->v_edge_pos >> field_based;

    #ifdef __dump_separate__
    char logc[60];
    int emu=0;
    snprintf(logc,59,"motion_x=%d,motion_y=%d,dxy=%d",motion_x,motion_y,dxy);
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);
    snprintf(logc,59,"h=%d,field_based=%d,bottom_field=%d",h,field_based,bottom_field);
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);
    #endif

    if(field_based){
        mx= motion_x/2;
        my= motion_y>>1;
    }else if(s->workaround_bugs&FF_BUG_QPEL_CHROMA2){
        static const int rtab[8]= {0,0,1,1,0,0,0,1};
        mx= (motion_x>>1) + rtab[motion_x&7];
        my= (motion_y>>1) + rtab[motion_y&7];
    }else if(s->workaround_bugs&FF_BUG_QPEL_CHROMA){
        mx= (motion_x>>1)|(motion_x&1);
        my= (motion_y>>1)|(motion_y&1);
    }else{
        mx= motion_x/2;
        my= motion_y/2;
    }
    mx= (mx>>1)|(mx&1);
    my= (my>>1)|(my&1);

    uvdxy= (mx&1) | ((my&1)<<1);
    mx>>=1;
    my>>=1;

    uvsrc_x = (mb_x *  8                 + mx);
    uvsrc_y = mb_y * (8 >> field_based) + my;

    ptr_y  = ref_picture[0] +   src_y *   linesize +   src_x;
    ptr_cb = ref_picture[1] + uvsrc_y * uvlinesize + uvsrc_x*2;
//    ptr_cr = ref_picture[2] + uvsrc_y * uvlinesize + uvsrc_x;

    if(   (unsigned)src_x > p_pic->h_edge_pos - (motion_x&3) - 16
       || (unsigned)src_y >    v_edge_pos - (motion_y&3) - h  ){
        s->dsp.emulated_edge_mc(s->edge_emu_buffer, ptr_y, linesize,
                            17, 17+field_based, src_x, src_y<<field_based,
                            p_pic->h_edge_pos, p_pic->v_edge_pos);
        ptr_y= s->edge_emu_buffer;
        #ifdef __dump_separate__
        emu=1;
        #endif
        if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
            uint8_t *uvbuf= s->edge_emu_buffer + 18*linesize;
            s->dsp.emulated_edge_mc_nv12(uvbuf, ptr_cb, uvlinesize,
                                9, 9 + field_based,
                                uvsrc_x, uvsrc_y<<field_based,
                                p_pic->h_edge_pos>>1, p_pic->v_edge_pos>>1);
/*            s->dsp.emulated_edge_mc(uvbuf + 16, ptr_cr, s->uvlinesize,
                                9, 9 + field_based,
                                uvsrc_x, uvsrc_y<<field_based,
                                s->h_edge_pos>>1, s->v_edge_pos>>1);*/
            ptr_cb= uvbuf;
//            ptr_cr= uvbuf + 16;
        }
    }

    #ifdef __dump_separate__
    snprintf(logc,49,"des offset=%d,emu=%d",dest_y-p_pic->current_picture_ptr->data[0],emu);
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);
    snprintf(logc,49,"uvdxy=%d",uvdxy);
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);

    log_text_p(log_fd_dsp_text+log_offset_mc, "before MC Y:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, dest_y, 16, h, p_pic->current_picture_ptr->linesize[0]-16,p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text+log_offset_mc, "reference Y:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, ptr_y, 16, h, p_pic->current_picture_ptr->linesize[0]-16,p_pic->frame_cnt);
    #endif

    if(!field_based)
        qpix_op[0][dxy](dest_y, ptr_y, linesize);
    else{
        if(bottom_field){
            dest_y += linesize;
            dest_cb+= uvlinesize;
//            dest_cr+= s->uvlinesize;
        }

        if(field_select){
            ptr_y  += linesize;
            ptr_cb += uvlinesize;
//            ptr_cr += s->uvlinesize;
        }
        //damn interlaced mode
        //FIXME boundary mirroring is not exactly correct here
        av_log(NULL,AV_LOG_ERROR,"interlace mode in qpel_motion_amba, should correct here.\n");
        qpix_op[1][dxy](dest_y  , ptr_y  , linesize);
        qpix_op[1][dxy](dest_y+8, ptr_y+8, linesize);
    }
    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_mc, "after MC Y:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, dest_y, 16, h, p_pic->current_picture_ptr->linesize[0]-16,p_pic->frame_cnt);

    log_text_p(log_fd_dsp_text+log_offset_mc, "before MC UV:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, dest_cb, 16, h>>1, p_pic->current_picture_ptr->linesize[1]-16,p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text+log_offset_mc, "reference UV:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, ptr_cb, 16, h, p_pic->current_picture_ptr->linesize[1]-16,p_pic->frame_cnt);
    #endif

    if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
//        pix_op[1][uvdxy](dest_cr, ptr_cr, uvlinesize, h >> 1);
        pix_op_nv12[1][uvdxy](dest_cb, ptr_cb, uvlinesize, h >> 1);
    }

    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_mc, "after MC UV:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, dest_cb, 16, h>>1, p_pic->current_picture_ptr->linesize[1]-16,p_pic->frame_cnt);
    #endif

}

static av_always_inline
void mpeg_motion_internal_amba(MpegEncContext *s,h263_pic_data_t* p_pic,
                 uint8_t *dest_y, uint8_t *dest_cb,
                 int field_based, int bottom_field, int field_select,
                 uint8_t **ref_picture, op_pixels_func (*pix_op)[4],op_pixels_func (*pix_op_nv12)[4],
                 int motion_x, int motion_y, int h, int mb_x,int mb_y)
{
    uint8_t *ptr_y, *ptr_cb;//, *ptr_cr;
    int dxy, uvdxy, mx, my, src_x, src_y,
        uvsrc_x, uvsrc_y, v_edge_pos, uvlinesize, linesize;

    #ifdef __dump_DSP_TXT__
    char logc[100];
    snprintf(logc,99,"field related: field_based=%d,bottom_field=%d,field_select=%d",field_based,bottom_field,!!field_select);
    log_text_p(log_fd_dsp_text, logc,p_pic->frame_cnt);
    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);
    snprintf(logc,99,"motion_x=%d,motion_y=%d",motion_x,motion_y);
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);
    #endif
    #endif

    v_edge_pos = p_pic->v_edge_pos >> field_based;
    linesize   = p_pic->current_picture_ptr->linesize[0] << field_based;
    uvlinesize = p_pic->current_picture_ptr->linesize[1] << field_based;

    dxy = ((motion_y & 1) << 1) | (motion_x & 1);
    src_x = mb_x* 16               + (motion_x >> 1);
    src_y =(mb_y<<(4-field_based)) + (motion_y >> 1);

    if ( s->out_format == FMT_H263) {
        if((s->workaround_bugs & FF_BUG_HPEL_CHROMA) && field_based){
            mx = (motion_x>>1)|(motion_x&1);
            my = motion_y >>1;
            uvdxy = ((my & 1) << 1) | (mx & 1);
            uvsrc_x = mb_x* 8               + (mx >> 1);
            uvsrc_y = (mb_y<<(3-field_based)) + (my >> 1);
        }else{
            uvdxy = dxy | (motion_y & 2) | ((motion_x & 2) >> 1);
            uvsrc_x = src_x>>1;
            uvsrc_y = src_y>>1;
        }
    }else if( s->out_format == FMT_H261){//even chroma mv's are full pel in H261
        mx = motion_x / 4;
        my = motion_y / 4;
        uvdxy = 0;
        uvsrc_x = mb_x*8 + mx;
        uvsrc_y = mb_y*8 + my;
    } else {
        if(s->chroma_y_shift){
            mx = motion_x / 2;
            my = motion_y / 2;
            uvdxy = ((my & 1) << 1) | (mx & 1);
            uvsrc_x = mb_x* 8               + (mx >> 1);
            uvsrc_y = (mb_y<<(3-field_based)) + (my >> 1);
        } else {
            if(s->chroma_x_shift){
            //Chroma422
                mx = motion_x / 2;
                uvdxy = ((motion_y & 1) << 1) | (mx & 1);
                uvsrc_x = s->mb_x* 8           + (mx >> 1);
                uvsrc_y = src_y;
            } else {
            //Chroma444
                uvdxy = dxy;
                uvsrc_x = src_x;
                uvsrc_y = src_y;
            }
        }
    }

    #ifdef __dump_DSP_TXT__
    snprintf(logc,99,"field related: uvlinesize_yuv=%d,uvlinesize=%d,linesize=%d,v_edge_pos=%d,uvsrc_x=%d,uvsrc_y=%d",uvlinesize/2,uvlinesize,linesize,v_edge_pos,uvsrc_x,uvsrc_y);
    log_text_p(log_fd_dsp_text, logc,p_pic->frame_cnt);
    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);
    #endif
    #endif

    ptr_y  = ref_picture[0] + src_y * linesize + src_x;
    ptr_cb = ref_picture[1] + uvsrc_y * uvlinesize + uvsrc_x*2;
//    ptr_cr = ref_picture[2] + uvsrc_y * uvlinesize + uvsrc_x;

//    if(s->flags&CODEC_FLAG_EMU_EDGE)
//    {
        if(   (unsigned)src_x > s->h_edge_pos - (motion_x&1) - 16
           || (unsigned)src_y >    v_edge_pos - (motion_y&1) - h){


                #ifdef __dump_DSP_TXT__
                log_text_p(log_fd_dsp_text, "in emulate extend edge",p_pic->frame_cnt);
                #endif

                s->dsp.emulated_edge_mc(s->edge_emu_buffer, ptr_y, p_pic->current_picture_ptr->linesize[0] ,
                                    17, 17+field_based,
                                    src_x, src_y<<field_based,
                                    p_pic->h_edge_pos, p_pic->v_edge_pos);
                ptr_y = s->edge_emu_buffer;
                if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
                    uint8_t *uvbuf= s->edge_emu_buffer+18*p_pic->current_picture_ptr->linesize[0] ;
                    s->dsp.emulated_edge_mc_nv12(uvbuf ,
                                        ptr_cb, p_pic->current_picture_ptr->linesize[1] ,
                                        9, 9+field_based,
                                        uvsrc_x, uvsrc_y<<field_based,
                                        p_pic->h_edge_pos>>1, p_pic->v_edge_pos>>1);
    /*                s->dsp.emulated_edge_mc(uvbuf+16,
                                        ptr_cr, s->uvlinesize,
                                        9, 9+field_based,
                                        uvsrc_x, uvsrc_y<<field_based,
                                        s->h_edge_pos>>1, s->v_edge_pos>>1);*/
                    ptr_cb= uvbuf;
    //                ptr_cr= uvbuf+1;
                }
        }
//    }

    if(bottom_field){ //FIXME use this for field pix too instead of the obnoxious hack which changes picture.data
        dest_y += p_pic->current_picture_ptr->linesize[0] ;
        dest_cb+= p_pic->current_picture_ptr->linesize[1] ;
//        dest_cr+= s->uvlinesize;
    }

    if(field_select){
        ptr_y += p_pic->current_picture_ptr->linesize[0] ;
        ptr_cb+= p_pic->current_picture_ptr->linesize[1] ;
//        ptr_cr+= s->uvlinesize;
    }

    #ifdef __dump_DSP_TXT__
        char logt[80];

        snprintf(logt,79,"before pix_op, dxy=%d",dxy);
        log_text_p(log_fd_dsp_text, logt,p_pic->frame_cnt);

        log_text_p(log_fd_dsp_text, "before MC Y:",p_pic->frame_cnt);
        log_text_rect_char_hex_p(log_fd_dsp_text, dest_y, 16, h, linesize-16,p_pic->frame_cnt);
        log_text_p(log_fd_dsp_text, "reference Y:",p_pic->frame_cnt);
        log_text_rect_char_hex_p(log_fd_dsp_text, ptr_y, 16, h, linesize-16,p_pic->frame_cnt);

        #ifdef __dump_separate__
        log_text_p(log_fd_dsp_text+log_offset_mc, logt,p_pic->frame_cnt);
        log_text_p(log_fd_dsp_text+log_offset_mc, "before MC Y:",p_pic->frame_cnt);
        log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, dest_y, 16, h, linesize-16,p_pic->frame_cnt);
        log_text_p(log_fd_dsp_text+log_offset_mc, "reference Y:",p_pic->frame_cnt);
        log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, ptr_y, 16, h, linesize-16,p_pic->frame_cnt);
        #endif

        snprintf(logt,79,"before pix_op_nv12, s->chroma_x_shift=%d:uvdxy=%d",s->chroma_x_shift,uvdxy);
        log_text_p(log_fd_dsp_text, logt,p_pic->frame_cnt);

        log_text_p(log_fd_dsp_text, "before MC Cb:",p_pic->frame_cnt);
        log_text_rect_char_hex_chroma_p(log_fd_dsp_text, dest_cb, 8, h/2, uvlinesize-16,p_pic->frame_cnt);
        log_text_p(log_fd_dsp_text, "reference Cb:",p_pic->frame_cnt);
        log_text_rect_char_hex_chroma_p(log_fd_dsp_text, ptr_cb, 8, h/2, uvlinesize-16,p_pic->frame_cnt);
        log_text_p(log_fd_dsp_text, "before MC Cr:",p_pic->frame_cnt);
        log_text_rect_char_hex_chroma_p(log_fd_dsp_text, dest_cb+1, 8, h/2, uvlinesize-16,p_pic->frame_cnt);
        log_text_p(log_fd_dsp_text, "reference Cr:",p_pic->frame_cnt);
        log_text_rect_char_hex_chroma_p(log_fd_dsp_text, ptr_cb+1, 8, h/2, uvlinesize-16,p_pic->frame_cnt);
        #ifdef __dump_separate__
        log_text_p(log_fd_dsp_text+log_offset_mc, logt,p_pic->frame_cnt);

        log_text_p(log_fd_dsp_text+log_offset_mc, "before MC Cb:",p_pic->frame_cnt);
        log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc, dest_cb, 8, h/2, uvlinesize-16,p_pic->frame_cnt);
        log_text_p(log_fd_dsp_text+log_offset_mc, "reference Cb:",p_pic->frame_cnt);
        log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc, ptr_cb, 8, h/2, uvlinesize-16,p_pic->frame_cnt);
        log_text_p(log_fd_dsp_text+log_offset_mc, "before MC Cr:",p_pic->frame_cnt);
        log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc, dest_cb+1, 8, h/2, uvlinesize-16,p_pic->frame_cnt);
        log_text_p(log_fd_dsp_text+log_offset_mc, "reference Cr:",p_pic->frame_cnt);
        log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc, ptr_cb+1, 8, h/2, uvlinesize-16,p_pic->frame_cnt);
        #endif
    #endif

    pix_op[0][dxy](dest_y, ptr_y, linesize, h);

    if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY)){
        pix_op_nv12[s->chroma_x_shift][uvdxy]
                (dest_cb, ptr_cb, uvlinesize, h >> s->chroma_y_shift);
//        pix_op[s->chroma_x_shift][uvdxy]
//                (dest_cr, ptr_cr, uvlinesize, h >> s->chroma_y_shift);
    }

}

static inline int hpel_motion_amba(MpegEncContext *s,h263_pic_data_t* p_pic,
                                  uint8_t *dest, uint8_t *src,
                                  int field_based, int field_select,
                                  int src_x, int src_y,
                                  int width, int height, int stride,
                                  int h_edge_pos, int v_edge_pos,
                                  int w, int h, op_pixels_func *pix_op,
                                  int motion_x, int motion_y)
{
    int dxy;
    int emu=0;

    dxy = ((motion_y & 1) << 1) | (motion_x & 1);
    src_x += motion_x >> 1;
    src_y += motion_y >> 1;

    #ifdef __dump_separate__
    char logc[50];
    snprintf(logc,49,"motion_x=%d,motion_y=%d,dxy=%d",motion_x,motion_y,dxy);
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);
    #endif

    /* WARNING: do no forget half pels */
    src_x = av_clip(src_x, -16, width); //FIXME unneeded for emu?
    if (src_x == width)
        dxy &= ~1;
    src_y = av_clip(src_y, -16, height);
    if (src_y == height)
        dxy &= ~2;
    src += src_y * stride + src_x;

    if(s->unrestricted_mv && (s->flags&CODEC_FLAG_EMU_EDGE)){
        if(   (unsigned)src_x > h_edge_pos - (motion_x&1) - w
           || (unsigned)src_y > v_edge_pos - (motion_y&1) - h){
            s->dsp.emulated_edge_mc(s->edge_emu_buffer, src, p_pic->current_picture_ptr->linesize[0], w+1, (h+1)<<field_based,
                             src_x, src_y<<field_based, h_edge_pos, p_pic->v_edge_pos);
            src= s->edge_emu_buffer;
            emu=1;
        }
    }
    if(field_select)
        src += p_pic->current_picture_ptr->linesize[0];

    #ifdef __dump_separate__
    snprintf(logc,49,"des offset=%d,emu=%d",dest-p_pic->current_picture_ptr->data[0],emu);
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);

    log_text_p(log_fd_dsp_text+log_offset_mc, "before MC Y:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, dest, w, h, p_pic->current_picture_ptr->linesize[0]-w,p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text+log_offset_mc, "reference Y:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, src, w, h, p_pic->current_picture_ptr->linesize[0]-w,p_pic->frame_cnt);
    #endif

    pix_op[dxy](dest, src, stride, h);

    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_mc, "after MC Y:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, dest, w, h, p_pic->current_picture_ptr->linesize[0]-w,p_pic->frame_cnt);
    #endif

    return emu;
}

static inline void chroma_4mv_motion_amba(MpegEncContext *s,h263_pic_data_t* p_pic,
                                     uint8_t *dest_cb,
                                     uint8_t **ref_picture,
                                     op_pixels_func *pix_op_nv12,
                                     int mx, int my, int mb_x, int mb_y)
{
    int dxy, emu=0, src_x, src_y, offset;
    uint8_t *ptr;

    #ifdef __dump_separate__
    char logc[50];
    #endif

    /* In case of 8X8, we construct a single chroma motion vector
       with a special rounding */
    mx= ff_h263_round_chroma(mx);
    my= ff_h263_round_chroma(my);

    #ifdef __dump_separate__
    snprintf(logc,49,"mx=%d,my=%d",mx,my);
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);
    #endif

    dxy = ((my & 1) << 1) | (mx & 1);
    mx >>= 1;
    my >>= 1;

    src_x = mb_x * 8 + mx;
    src_y = mb_y * 8 + my;

    #ifdef __dump_separate__
    snprintf(logc,49,"dxy=%d,src_x=%d,p_pic->width/2=%d",dxy,src_x,p_pic->width/2);
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);
    snprintf(logc,49,"src_y=%d,p_pic->height/2=%d",src_y,p_pic->height/2);
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);
    #endif

    src_x = av_clip(src_x, -8, p_pic->width/2);
    if (src_x == p_pic->width/2)
        dxy &= ~1;
    src_y = av_clip(src_y, -8, p_pic->height/2);
    if (src_y == p_pic->height/2)
        dxy &= ~2;

    offset = (src_y * (p_pic->current_picture_ptr->linesize[1])) + src_x*2;
    ptr = ref_picture[1] + offset;
    if(s->flags&CODEC_FLAG_EMU_EDGE){
        if(   (unsigned)src_x > (p_pic->h_edge_pos>>1) - (dxy &1) - 8
           || (unsigned)src_y > (p_pic->v_edge_pos>>1) - (dxy>>1) - 8){
            s->dsp.emulated_edge_mc_nv12(s->edge_emu_buffer, ptr, p_pic->current_picture_ptr->linesize[1],
                                9, 9, src_x, src_y,
                                p_pic->h_edge_pos>>1, p_pic->v_edge_pos>>1);
            ptr= s->edge_emu_buffer;
            emu=1;
        }
    }
    av_log(NULL, AV_LOG_DEBUG, "emu=%d\n", emu);

    #ifdef __dump_separate__
    snprintf(logc,49,"dxy=%d,des offset=%d,emu=%d",dxy,dest_cb-p_pic->current_picture_ptr->data[1],emu);
    log_text_p(log_fd_dsp_text+log_offset_mc, logc,p_pic->frame_cnt);

    log_text_p(log_fd_dsp_text+log_offset_mc, "before MC CbCr:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, dest_cb, 16, 8, p_pic->current_picture_ptr->linesize[1]-16,p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text+log_offset_mc, "reference CbCr:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, ptr, 16, 8, p_pic->current_picture_ptr->linesize[1]-16,p_pic->frame_cnt);
    #endif

    pix_op_nv12[dxy](dest_cb, ptr, p_pic->current_picture_ptr->linesize[1], 8);

    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_mc, "after MC CbCr:",p_pic->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc, dest_cb,16, 8, p_pic->current_picture_ptr->linesize[1]-16,p_pic->frame_cnt);
    #endif


}

void h263_mpeg4_i_process_idct_amba(H263AmbaDecContext_t *thiz, h263_pic_data_t* p_pic)
{
    MpegEncContext *s = &thiz->s;
    int i,j;
    //h263_pic_swinfo_t* pinfo=p_pic->pswinfo;
    dctarray pdct=(dctarray)p_pic->pdct;

    uint8_t *dest_y;
    uint8_t *dest_cb;
    int dct_linesize, dct_offset;
    mb_mv_P_t* pmvp=p_pic->pmvp;
    const int linesize= p_pic->current_picture_ptr->linesize[0]; //not s->linesize as this would be wrong for field pics
    const int uvlinesize= p_pic->current_picture_ptr->linesize[1];
    //const int readable= s->pict_type != FF_B_TYPE || s->encoding || s->avctx->draw_horiz_band || s->avctx->lowres ||1;
    const int block_size= s->avctx->lowres ? 8>>s->avctx->lowres : 8;
    ambadec_assert_ffmpeg(!s->avctx->lowres);

    for(j=0;j<p_pic->mb_height;j++)
    {
        dest_y=p_pic->current_picture_ptr->data[0]+((j*linesize)<<4);
        dest_cb=p_pic->current_picture_ptr->data[1]+((j*uvlinesize)<<3);
        for(i=0;i<p_pic->mb_width;i++)
        {

            dct_linesize = linesize << pmvp->mb_setting.dct_type;
            dct_offset =(pmvp->mb_setting.dct_type)? linesize : linesize*block_size;

            #ifdef __dump_DSP_TXT__
            #ifdef __dump_separate__
            char title[40];
            snprintf(title,39,"mb [y=%d, x=%d]:\n",j,i);
            log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);

            memset(s->logdct_deqaun,0,sizeof(s->logdct_deqaun));
            memset(s->logdct_result,0,sizeof(s->logdct_result));
            memset(s->logdctmc_result,0,sizeof(s->logdctmc_result));
            #endif
            #endif


            put_dct_amba(s,(*pdct)[0],0,dest_y,dct_linesize);
            put_dct_amba(s,(*pdct)[1],1,dest_y + block_size,dct_linesize);
            put_dct_amba(s,(*pdct)[2],2,dest_y + dct_offset,dct_linesize);
            put_dct_amba(s,(*pdct)[3],3,dest_y + dct_offset + block_size,dct_linesize);

            put_dct_nv12_amba(s,(*pdct)[4],4,dest_cb,uvlinesize);
            put_dct_nv12_amba(s,(*pdct)[5],5,dest_cb+1,uvlinesize);

            #ifdef __dump_DSP_TXT__
            #ifdef __dump_separate__
            int itmp=0;
            snprintf(title,39,"mb [y=%d, x=%d]:\n",j,i);
            log_text_p(log_fd_dsp_text+log_offset_dct,title,p_pic->frame_cnt);
            for(itmp=0;itmp<6;itmp++)
            {
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After Inverse Quantization: >>",p_pic->frame_cnt);
                log_text_rect_short_p(log_fd_dsp_text+log_offset_dct,s->logdct_deqaun[itmp],8,8,0,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT: >>",p_pic->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdct_result[itmp],8,8,0,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT+MC: >>",p_pic->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdctmc_result[itmp],8,8,0,p_pic->frame_cnt);
            }
            #endif
            #endif


            if(s->loop_filter)
            {
                av_log(NULL, AV_LOG_ERROR, "****Error here, DSP not support h263+ inloop filter.\n");
            }
            //if(s->loop_filter)
                //ff_h263_loop_filter_nv12(s);

            //update
            pdct++;
            dest_y+=16;
            dest_cb+=16;
            pmvp++;
            //pinfo++;
        }
    }

}

void h263_mpeg4_p_process_mc_idct_amba(H263AmbaDecContext_t *thiz, h263_pic_data_t* p_pic)
{
    MpegEncContext *s = &thiz->s;
    int i,j,k;
    mb_mv_P_t* pmvp=p_pic->pmvp;
    h263_pic_swinfo_t* pinfo=p_pic->pswinfo;
    dctarray pdct=(dctarray)p_pic->pdct;
    int mx,my;
    int dxy, src_x, src_y, motion_x, motion_y;
    uint8_t *ptr, *dest;

    uint8_t *dest_y;
    uint8_t *dest_cb;
    int dct_linesize, dct_offset;
    op_pixels_func (*op_pix)[4];
    op_pixels_func (*op_pix_nv12)[4];
    qpel_mc_func (*op_qpix)[16];
    const int linesize= p_pic->current_picture_ptr->linesize[0]; //not s->linesize as this would be wrong for field pics
    const int uvlinesize= p_pic->current_picture_ptr->linesize[1];
    //const int readable= s->pict_type != FF_B_TYPE || s->encoding || s->avctx->draw_horiz_band || s->avctx->lowres ||1;
    const int block_size= s->avctx->lowres ? 8>>s->avctx->lowres : 8;
    ambadec_assert_ffmpeg(!s->avctx->lowres);

    op_qpix= s->me.qpel_put;
    if ((!p_pic->no_rounding))
    {
        op_pix = s->dsp.put_pixels_tab;
        op_pix_nv12 = s->dsp.put_pixels_tab_nv12;
        #ifdef __dump_DSP_TXT__
            log_text_p(log_fd_dsp_text, "mc with no rounding",p_pic->frame_cnt);
            #ifdef __dump_separate__
                log_text_p(log_fd_dsp_text+log_offset_mc, "mc with no rounding",p_pic->frame_cnt);
            #endif
        #endif
    }else{
        op_pix = s->dsp.put_no_rnd_pixels_tab;
        op_pix_nv12 = s->dsp.put_no_rnd_pixels_tab_nv12;
        #ifdef __dump_DSP_TXT__
            log_text_p(log_fd_dsp_text, "mc with rounding",p_pic->frame_cnt);
            #ifdef __dump_separate__
                log_text_p(log_fd_dsp_text+log_offset_mc, "mc with rounding",p_pic->frame_cnt);
            #endif
        #endif
    }

    for(j=0;j<p_pic->mb_height;j++)
    {
        dest_y=p_pic->current_picture_ptr->data[0]+((j*linesize)<<4);
        dest_cb=p_pic->current_picture_ptr->data[1]+((j*uvlinesize)<<3);
        for(i=0;i<p_pic->mb_width;i++)
        {
            dct_linesize = linesize << pmvp->mb_setting.dct_type;
            dct_offset =(pmvp->mb_setting.dct_type)? linesize : linesize*block_size;

            #ifdef __dump_DSP_TXT__
            #ifdef __dump_separate__
            int itmp=0;
            char title[60];
            snprintf(title,59,"mb [y=%d, x=%d]:\n",j,i);
            log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);

            memset(s->logdct_deqaun,0,sizeof(s->logdct_deqaun));
            memset(s->logdct_result,0,sizeof(s->logdct_result));
            memset(s->logdctmc_result,0,sizeof(s->logdctmc_result));
            #endif
            #endif

            //mc
            if (!pmvp->mb_setting.intra)
            {
                /* decoding or more than one mb_type (MC was already done otherwise) */
                //try move
                #if 0
                op_qpix= s->me.qpel_put;
                if ((!p_pic->no_rounding))
                {
                    op_pix = s->dsp.put_pixels_tab;
                    op_pix_nv12 = s->dsp.put_pixels_tab_nv12;
                    #ifdef __dump_DSP_TXT__
                        log_text_p(log_fd_dsp_text, "mc with no rounding",p_pic->frame_cnt);
                        #ifdef __dump_separate__
                            log_text_p(log_fd_dsp_text+log_offset_mc, "mc with no rounding",p_pic->frame_cnt);
                        #endif
                    #endif
                }else{
                    op_pix = s->dsp.put_no_rnd_pixels_tab;
                    op_pix_nv12 = s->dsp.put_no_rnd_pixels_tab_nv12;
                    #ifdef __dump_DSP_TXT__
                        log_text_p(log_fd_dsp_text, "mc with rounding",p_pic->frame_cnt);
                        #ifdef __dump_separate__
                            log_text_p(log_fd_dsp_text+log_offset_mc, "mc with rounding",p_pic->frame_cnt);
                        #endif
                    #endif
                }
                #endif

                ambadec_assert_ffmpeg(pmvp->mv[0].ref_frame_num==forward_ref_num);

                switch(pmvp->mb_setting.mv_type) {

                   case mv_type_16x16:
                    #ifndef __dsp_interpret_gmc__
                        if(p_pic->current_picture_ptr->mb_type[i + j * p_pic->mb_stride]&MB_TYPE_GMC)
                        {
                            #ifdef __dump_DSP_TXT__
                                log_text_p(log_fd_dsp_text, "MV_TYPE_16X16 GMC:",p_pic->frame_cnt);
                                #ifdef __dump_separate__
                                log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_16X16 GMC:",p_pic->frame_cnt);
                                #endif
                            #endif
                            //ambadec_assert_ffmpeg(0);
                            //av_log(NULL,AV_LOG_ERROR,"h263_mpeg4_p_process_mc_idct_amba global mc not supported,mbtype=%x.\n",p_pic->current_picture_ptr->mb_type[i + j * p_pic->mb_stride]);

                            if(p_pic->real_sprite_warping_points==1){
                                gmc1_motion_amba(s,p_pic, dest_y, dest_cb,p_pic->last_picture_ptr->data,i,j);
                            }else{
                                gmc_motion_amba(s,p_pic, dest_y, dest_cb,p_pic->last_picture_ptr->data,i,j);
                            }
                        }else
                    #endif
                        if(p_pic->quarter_sample){
                            #ifdef __dump_DSP_TXT__
                                log_text_p(log_fd_dsp_text, "MV_TYPE_16X16 qpel_motion:",p_pic->frame_cnt);
                                #ifdef __dump_separate__
                                log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_16X16 qpel_motion:",p_pic->frame_cnt);
                                #endif
                            #endif
                            qpel_motion_amba(s,p_pic, dest_y, dest_cb,
                                        0, 0, 0,
                                        p_pic->last_picture_ptr->data, op_pix,op_pix_nv12,op_qpix,
                                        pmvp->mv[0].mv_x, pmvp->mv[0].mv_y, 16,i,j);
                        }
                        else
                        {
                            #ifdef __dump_DSP_TXT__
                                log_text_p(log_fd_dsp_text, "MV_TYPE_16X16 mpeg_motion:",p_pic->frame_cnt);
                                #ifdef __dump_separate__
                                log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_16X16 mpeg_motion:",p_pic->frame_cnt);
                                #endif
                            #endif
                            //debug_trap();
                            mpeg_motion_internal_amba(s, p_pic,dest_y, dest_cb,
                                        0, 0, 0,
                                        p_pic->last_picture_ptr->data, op_pix,op_pix_nv12,
                                        pmvp->mv[0].mv_x, pmvp->mv[0].mv_y, 16,i,j);
                        }
                        break;

                    case mv_type_8x8:
                        #ifdef __dump_DSP_TXT__
                            log_text_p(log_fd_dsp_text, "MV_TYPE_8X8:",p_pic->frame_cnt);
                            #ifdef __dump_separate__
                            log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_8X8:",p_pic->frame_cnt);
                            #endif
                        #endif
                        {
                            mx = 0;
                            my = 0;
                            if(p_pic->quarter_sample){
                                for(k=0;k<4;k++) {
                                    motion_x = pmvp->mv[k].mv_x;
                                    motion_y =pmvp->mv[k].mv_y;

                                    dxy = ((motion_y & 3) << 2) | (motion_x & 3);
                                    src_x = i * 16 + (motion_x >> 2) + (k & 1) * 8;
                                    src_y = j * 16 + (motion_y >> 2) + (k >>1) * 8;

                                    /* WARNING: do no forget half pels */
                                    src_x = av_clip(src_x, -16, p_pic->width);
                                    if (src_x == p_pic->width)
                                        dxy &= ~3;
                                    src_y = av_clip(src_y, -16, p_pic->height);
                                    if (src_y == p_pic->height)
                                        dxy &= ~12;

                                    ptr = p_pic->last_picture_ptr->data[0]+ (src_y * linesize) + (src_x);
                                    if(s->flags&CODEC_FLAG_EMU_EDGE){
                                        if(   (unsigned)src_x > p_pic->h_edge_pos - (motion_x&3) - 8
                                           || (unsigned)src_y > p_pic->v_edge_pos - (motion_y&3) - 8 ){
                                            s->dsp.emulated_edge_mc(s->edge_emu_buffer, ptr,
                                                                linesize, 9, 9,
                                                                src_x, src_y,
                                                                p_pic->h_edge_pos, p_pic->v_edge_pos);
                                            ptr= s->edge_emu_buffer;
                                        }
                                    }

                                    #ifdef __dump_DSP_TXT__
                                    #ifdef __dump_separate__
                                    snprintf(title,59,"motion_x=%d,motion_y=%d,dxy=%d",motion_x,motion_y,dxy);
                                    log_text_p(log_fd_dsp_text+log_offset_mc, title,p_pic->frame_cnt);
                                    #endif
                                    #endif

                                    dest = dest_y + ((k & 1) * 8) + (k >> 1) * 8 * linesize;
                                    op_qpix[1][dxy](dest, ptr, linesize);

                                    mx += pmvp->mv[k].mv_x/2;
                                    my += pmvp->mv[k].mv_y/2;
                                }
                            }else{
                                for(k=0;k<4;k++) {

                                    #ifdef __dump_DSP_TXT__
                                    #ifdef __dump_separate__
                                    snprintf(title,59,"motion_x=%d,motion_y=%d",pmvp->mv[k].mv_x,pmvp->mv[k].mv_y);
                                    log_text_p(log_fd_dsp_text+log_offset_mc, title,p_pic->frame_cnt);
                                    #endif
                                    #endif

                                    hpel_motion_amba(s,p_pic, dest_y + ((k & 1) * 8) + (k >> 1) * 8 * linesize,
                                                p_pic->last_picture_ptr->data[0], 0, 0,
                                                i * 16 + (k & 1) * 8, j * 16 + (k >>1) * 8,
                                                p_pic->width, p_pic->height, linesize,
                                                s->h_edge_pos, s->v_edge_pos,
                                                8, 8, op_pix[1],
                                                pmvp->mv[k].mv_x, pmvp->mv[k].mv_y);

                                    mx += pmvp->mv[k].mv_x;
                                    my += pmvp->mv[k].mv_y;
                                }
                            }

                            #ifdef __dump_DSP_TXT__
                            #ifdef __dump_separate__
                            snprintf(title,59,"chroma mx=%d,my=%d",mx,my);
                            log_text_p(log_fd_dsp_text+log_offset_mc, title,p_pic->frame_cnt);
                            #endif
                            #endif

                            if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY))
                                chroma_4mv_motion_amba(s,p_pic, dest_cb,  p_pic->last_picture_ptr->data, op_pix_nv12[1], mx, my,i,j);
                        }
                        break;

                    case mv_type_16x8:
                        #ifdef __dump_DSP_TXT__
                            log_text_p(log_fd_dsp_text, "MV_TYPE_FIELD:",p_pic->frame_cnt);
                            #ifdef __dump_separate__
                            log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_FIELD:",p_pic->frame_cnt);
                            #endif
                        #endif
                        ambadec_assert_ffmpeg(s->picture_structure == PICT_FRAME);

                        {
                            if(p_pic->quarter_sample){
                                for(k=0; k<2; k++){
                                    qpel_motion_amba(s,p_pic, dest_y, dest_cb,
                                                1, k, pmvp->mv[k].bottom,
                                                p_pic->last_picture_ptr->data, op_pix,op_pix_nv12, op_qpix,
                                                pmvp->mv[k].mv_x, pmvp->mv[k].mv_y, 8,i,j);
                                }
                            }else{
                                /* top field */
                                mpeg_motion_internal_amba(s,p_pic, dest_y, dest_cb,
                                            1, 0, pmvp->mv[0].bottom,
                                            p_pic->last_picture_ptr->data, op_pix,op_pix_nv12,
                                            pmvp->mv[0].mv_x, pmvp->mv[0].mv_y, 8,i,j);
                                /* bottom field */
                                mpeg_motion_internal_amba(s,p_pic, dest_y, dest_cb,
                                            1, 1, pmvp->mv[1].bottom,
                                            p_pic->last_picture_ptr->data, op_pix,op_pix_nv12,
                                            pmvp->mv[1].mv_x, pmvp->mv[1].mv_y, 8,i,j);
                            }
                        }
                        break;

                    case mv_type_16x16_gmc:
                        ambadec_assert_ffmpeg(p_pic->current_picture_ptr->mb_type[i + j * p_pic->mb_stride]&MB_TYPE_GMC);
                        #ifdef __dump_DSP_TXT__
                            log_text_p(log_fd_dsp_text, "MV_TYPE_16X16 GMC:",p_pic->frame_cnt);
                            #ifdef __dump_separate__
                            log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_16X16 GMC:",p_pic->frame_cnt);
                            #endif
                        #endif
                        //ambadec_assert_ffmpeg(0);
                        //av_log(NULL,AV_LOG_ERROR,"h263_mpeg4_p_process_mc_idct_amba global mc not supported,mbtype=%x.\n",p_pic->current_picture_ptr->mb_type[i + j * p_pic->mb_stride]);

                        if(p_pic->real_sprite_warping_points==1){
                            gmc1_motion_amba(s,p_pic, dest_y, dest_cb,p_pic->last_picture_ptr->data,i,j);
                        }else{
                            gmc_motion_amba(s,p_pic, dest_y, dest_cb,p_pic->last_picture_ptr->data,i,j);
                        }
                        break;

                    default: assert(0);
                }

                #ifdef __dump_separate__
                log_text_p(log_fd_dsp_text+log_offset_mc, "After Forward MC, result is:",p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_mc, "Y:",p_pic->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc,dest_y,16,16,s->linesize-16,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_mc, "Cb:",p_pic->frame_cnt);
                log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb,8,8,s->uvlinesize-16,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_mc, "Cr:",p_pic->frame_cnt);
                log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb+1,8,8,s->uvlinesize-16,p_pic->frame_cnt);
                #endif

                ambadec_assert_ffmpeg(s->chroma_y_shift);

                /* add dct residue */
                add_dct_amba(s,(*pdct)[0],0,dest_y,dct_linesize,pinfo->block_last_index[0] >= 0);
                add_dct_amba(s,(*pdct)[1],1,dest_y+ block_size,dct_linesize,pinfo->block_last_index[1] >= 0);
                add_dct_amba(s,(*pdct)[2],2,dest_y + dct_offset,dct_linesize,pinfo->block_last_index[2] >= 0);
                add_dct_amba(s,(*pdct)[3],3,dest_y + dct_offset + block_size,dct_linesize,pinfo->block_last_index[3] >= 0);

                add_dct_nv12_amba(s,(*pdct)[4],4,dest_cb,uvlinesize,pinfo->block_last_index[4] >= 0);
                add_dct_nv12_amba(s,(*pdct)[5],5,dest_cb+1,uvlinesize,pinfo->block_last_index[5] >= 0);

            }else{

                ambadec_assert_ffmpeg(pmvp->mv[0].ref_frame_num==not_valid_mv);

                /* dct only in intra block */

                put_dct_amba(s,(*pdct)[0],0,dest_y,dct_linesize);
                put_dct_amba(s,(*pdct)[1],1,dest_y + block_size,dct_linesize);
                put_dct_amba(s,(*pdct)[2],2,dest_y + dct_offset,dct_linesize);
                put_dct_amba(s,(*pdct)[3],3,dest_y + dct_offset + block_size,dct_linesize);

                put_dct_nv12_amba(s,(*pdct)[4],4,dest_cb,uvlinesize);
                put_dct_nv12_amba(s,(*pdct)[5],5,dest_cb+1,uvlinesize);

            }

            #ifdef __dump_DSP_TXT__
            #ifdef __dump_separate__
            snprintf(title,39,"mb [y=%d, x=%d]:\n",j,i);
            log_text_p(log_fd_dsp_text+log_offset_dct,title,p_pic->frame_cnt);

            for(itmp=0;itmp<6;itmp++)
            {
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After Inverse Quantization: >>",p_pic->frame_cnt);
                log_text_rect_short_p(log_fd_dsp_text+log_offset_dct,s->logdct_deqaun[itmp],8,8,0,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT: >>",p_pic->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdct_result[itmp],8,8,0,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT+MC: >>",p_pic->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdctmc_result[itmp],8,8,0,p_pic->frame_cnt);
            }
            #endif
            #endif

            //idct and add residue

            if(s->loop_filter)
            {
                av_log(NULL, AV_LOG_ERROR, "****Error here, DSP not support h263+ inloop filter.\n");
            }
            //if(s->loop_filter)
                //ff_h263_loop_filter_nv12(s);

            //update
            pmvp++;
            pdct++;
            dest_y+=16;
            dest_cb+=16;
            pinfo++;
        }
    }


}

void h263_mpeg4_b_process_mc_idct_amba(H263AmbaDecContext_t *thiz, h263_pic_data_t* p_pic)
{
    MpegEncContext *s = &thiz->s;
    int i,j,k;
    mb_mv_B_t* pmvb=p_pic->pmvb;
    h263_pic_swinfo_t* pinfo=p_pic->pswinfo;
    dctarray pdct=(dctarray)p_pic->pdct;
    int mx,my;
    int dxy, src_x, src_y, motion_x, motion_y;
    uint8_t *ptr, *dest;

    uint8_t *dest_y;
    uint8_t *dest_cb;
    int dct_linesize, dct_offset;
    op_pixels_func (*op_pix)[4];
    op_pixels_func (*op_pix_nv12)[4];
    qpel_mc_func (*op_qpix)[16];
    const int linesize= p_pic->current_picture_ptr->linesize[0]; //not s->linesize as this would be wrong for field pics
    const int uvlinesize= p_pic->current_picture_ptr->linesize[1];
    //const int readable= s->pict_type != FF_B_TYPE || s->encoding || s->avctx->draw_horiz_band || s->avctx->lowres ||1;
    const int block_size= s->avctx->lowres ? 8>>s->avctx->lowres : 8;
    ambadec_assert_ffmpeg(!s->avctx->lowres);

    for(j=0;j<p_pic->mb_height;j++)
    {
        dest_y=p_pic->current_picture_ptr->data[0]+((j*linesize)<<4);
        dest_cb=p_pic->current_picture_ptr->data[1]+((j*uvlinesize)<<3);
        for(i=0;i<p_pic->mb_width;i++)
        {
            dct_linesize = linesize << pmvb->mb_setting.dct_type;
            dct_offset =(pmvb->mb_setting.dct_type)? linesize : linesize*block_size;

            #ifdef __dump_DSP_TXT__
            #ifdef __dump_separate__
            int itmp=0;
            char title[60];
            snprintf(title,59,"mb [y=%d, x=%d]:\n",j,i);
            log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);

            memset(s->logdct_deqaun,0,sizeof(s->logdct_deqaun));
            memset(s->logdct_result,0,sizeof(s->logdct_result));
            memset(s->logdctmc_result,0,sizeof(s->logdctmc_result));
            #endif
            #endif

            //mc
            if (!pmvb->mb_setting.intra)
            {
                /* decoding or more than one mb_type (MC was already done otherwise) */
                op_qpix= s->me.qpel_put;
                op_pix = s->dsp.put_pixels_tab;
                op_pix_nv12 = s->dsp.put_pixels_tab_nv12;
                #ifdef __dump_separate__
                    log_text_p(log_fd_dsp_text+log_offset_mc, "mc with no rounding",p_pic->frame_cnt);
                #endif

                //forward
                if(pmvb->mvp[0].fw.ref_frame_num==forward_ref_num)
                {
                    switch(pmvb->mb_setting.mv_type) {
                    case mv_type_16x16:
                        /*
                        if(p_pic->current_picture_ptr->mb_type[i + j * p_pic->mb_stride]&MB_TYPE_GMC){
                            #ifdef __dump_DSP_TXT__
                                log_text_p(log_fd_dsp_text, "MV_TYPE_16X16 GMC:",p_pic->frame_cnt);
                                #ifdef __dump_separate__
                                log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_16X16 GMC:",p_pic->frame_cnt);
                                #endif
                            #endif
                            //ambadec_assert_ffmpeg(0);
                            av_log(NULL,AV_LOG_ERROR,"h263_mpeg4_p_process_mc_idct_amba gloabol mc not supported.\n");

                            if(p_pic->real_sprite_warping_points==1){
                                gmc1_motion_amba(s,p_pic, dest_y, dest_cb, dest_cb+1,p_pic->last_picture_ptr->data,i,j);
                            }else{
                                gmc_motion_amba(s,p_pic, dest_y, dest_cb, dest_cb+1,p_pic->last_picture_ptr->data,i,j);
                            }
                        }else */
                        if(p_pic->quarter_sample){
                            #ifdef __dump_DSP_TXT__
                                log_text_p(log_fd_dsp_text, "MV_TYPE_16X16 qpel_motion:",p_pic->frame_cnt);
                                #ifdef __dump_separate__
                                log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_16X16 qpel_motion:",p_pic->frame_cnt);
                                #endif
                            #endif
                            qpel_motion_amba(s,p_pic, dest_y, dest_cb,
                                        0, 0, 0,
                                        p_pic->last_picture_ptr->data, op_pix,op_pix_nv12,op_qpix,
                                        pmvb->mvp[0].fw.mv_x, pmvb->mvp[0].fw.mv_y, 16,i,j);
                        }
                        else
                        {
                            #ifdef __dump_DSP_TXT__
                                log_text_p(log_fd_dsp_text, "MV_TYPE_16X16 mpeg_motion:",p_pic->frame_cnt);
                                #ifdef __dump_separate__
                                log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_16X16 mpeg_motion:",p_pic->frame_cnt);
                                #endif
                            #endif
                            //debug_trap();
                            mpeg_motion_internal_amba(s, p_pic,dest_y, dest_cb,
                                        0, 0, 0,
                                        p_pic->last_picture_ptr->data, op_pix,op_pix_nv12,
                                        pmvb->mvp[0].fw.mv_x, pmvb->mvp[0].fw.mv_y, 16,i,j);
                        }
                        break;
                    case mv_type_8x8:
                    #ifdef __dump_DSP_TXT__
                        log_text_p(log_fd_dsp_text, "MV_TYPE_8X8:",p_pic->frame_cnt);
                        #ifdef __dump_separate__
                        log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_8X8:",p_pic->frame_cnt);
                        #endif
                    #endif
                    {
                        mx = 0;
                        my = 0;
                        if(p_pic->quarter_sample){
                            for(k=0;k<4;k++) {
                                motion_x = pmvb->mvp[k].fw.mv_x;
                                motion_y =pmvb->mvp[k].fw.mv_y;

                                dxy = ((motion_y & 3) << 2) | (motion_x & 3);
                                src_x = i * 16 + (motion_x >> 2) + (k & 1) * 8;
                                src_y = j * 16 + (motion_y >> 2) + (k >>1) * 8;

                                /* WARNING: do no forget half pels */
                                src_x = av_clip(src_x, -16, p_pic->width);
                                if (src_x == p_pic->width)
                                    dxy &= ~3;
                                src_y = av_clip(src_y, -16, p_pic->height);
                                if (src_y == p_pic->height)
                                    dxy &= ~12;

                                ptr = p_pic->last_picture_ptr->data[0]+ (src_y * linesize) + (src_x);
                                if(s->flags&CODEC_FLAG_EMU_EDGE){
                                    if(   (unsigned)src_x > p_pic->h_edge_pos - (motion_x&3) - 8
                                       || (unsigned)src_y > p_pic->v_edge_pos - (motion_y&3) - 8 ){
                                        s->dsp.emulated_edge_mc(s->edge_emu_buffer, ptr,
                                                            linesize, 9, 9,
                                                            src_x, src_y,
                                                            p_pic->h_edge_pos, p_pic->v_edge_pos);
                                        ptr= s->edge_emu_buffer;
                                    }
                                }

                                #ifdef __dump_DSP_TXT__
                                #ifdef __dump_separate__
                                snprintf(title,59,"motion_x=%d,motion_y=%d,dxy=%d",motion_x,motion_y,dxy);
                                log_text_p(log_fd_dsp_text+log_offset_mc, title,p_pic->frame_cnt);
                                #endif
                                #endif

                                dest = dest_y + ((k & 1) * 8) + (k >> 1) * 8 * linesize;
                                op_qpix[1][dxy](dest, ptr, linesize);

                                mx += pmvb->mvp[k].fw.mv_x/2;
                                my += pmvb->mvp[k].fw.mv_y/2;
                            }
                        }else{
                            for(k=0;k<4;k++) {

                                #ifdef __dump_DSP_TXT__
                                #ifdef __dump_separate__
                                snprintf(title,59,"motion_x=%d,motion_y=%d",pmvb->mvp[k].fw.mv_x,pmvb->mvp[k].fw.mv_y);
                                log_text_p(log_fd_dsp_text+log_offset_mc, title,p_pic->frame_cnt);
                                #endif
                                #endif

                                hpel_motion_amba(s,p_pic, dest_y + ((k & 1) * 8) + (k >> 1) * 8 * linesize,
                                            p_pic->last_picture_ptr->data[0], 0, 0,
                                            i * 16 + (k & 1) * 8, j * 16 + (k >>1) * 8,
                                            p_pic->width, p_pic->height, linesize,
                                            s->h_edge_pos, s->v_edge_pos,
                                            8, 8, op_pix[1],
                                            pmvb->mvp[k].fw.mv_x, pmvb->mvp[k].fw.mv_y);

                                mx += pmvb->mvp[k].fw.mv_x;
                                my += pmvb->mvp[k].fw.mv_y;
                            }
                        }

                        #ifdef __dump_DSP_TXT__
                        #ifdef __dump_separate__
                        snprintf(title,59,"chroma mx=%d,my=%d",mx,my);
                        log_text_p(log_fd_dsp_text+log_offset_mc, title,p_pic->frame_cnt);
                        #endif
                        #endif

                        if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY))
                            chroma_4mv_motion_amba(s,p_pic, dest_cb,  p_pic->last_picture_ptr->data, op_pix_nv12[1], mx, my,i,j);
                    }
                        break;
                    case mv_type_16x8:
                        #ifdef __dump_DSP_TXT__
                            log_text_p(log_fd_dsp_text, "MV_TYPE_FIELD:",p_pic->frame_cnt);
                            #ifdef __dump_separate__
                            log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_FIELD:",p_pic->frame_cnt);
                            #endif
                        #endif
                        ambadec_assert_ffmpeg(s->picture_structure == PICT_FRAME);

                        {
                            if(p_pic->quarter_sample){
                                for(k=0; k<2; k++){
                                    qpel_motion_amba(s,p_pic, dest_y, dest_cb,
                                                1, k, pmvb->mvp[k].fw.bottom,
                                                p_pic->last_picture_ptr->data, op_pix,op_pix_nv12, op_qpix,
                                                pmvb->mvp[k].fw.mv_x, pmvb->mvp[k].fw.mv_y, 8,i,j);
                                }
                            }else{
                                /* top field */
                                mpeg_motion_internal_amba(s,p_pic, dest_y, dest_cb,
                                            1, 0, pmvb->mvp[0].fw.bottom,
                                            p_pic->last_picture_ptr->data, op_pix,op_pix_nv12,
                                            pmvb->mvp[0].fw.mv_x, pmvb->mvp[0].fw.mv_y, 8,i,j);
                                /* bottom field */
                                mpeg_motion_internal_amba(s,p_pic, dest_y, dest_cb,
                                            1, 1, pmvb->mvp[1].fw.bottom,
                                            p_pic->last_picture_ptr->data, op_pix,op_pix_nv12,
                                            pmvb->mvp[1].fw.mv_x, pmvb->mvp[1].fw.mv_y, 8,i,j);
                            }
                        }
                        break;
                    default: assert(0);
                    }

                    op_pix = s->dsp.avg_pixels_tab;
                    op_pix_nv12 = s->dsp.avg_pixels_tab_nv12;
                    op_qpix= s->me.qpel_avg;

                    #ifdef __dump_separate__
                    log_text_p(log_fd_dsp_text+log_offset_mc, "After Forward MC, result is:",p_pic->frame_cnt);
                    log_text_p(log_fd_dsp_text+log_offset_mc, "Y:",p_pic->frame_cnt);
                    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc,dest_y,16,16,p_pic->current_picture_ptr->linesize[0]-16,p_pic->frame_cnt);
                    log_text_p(log_fd_dsp_text+log_offset_mc, "Cb:",p_pic->frame_cnt);
                    log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb,8,8,p_pic->current_picture_ptr->linesize[1]-16,p_pic->frame_cnt);
                    log_text_p(log_fd_dsp_text+log_offset_mc, "Cr:",p_pic->frame_cnt);
                    log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb+1,8,8,p_pic->current_picture_ptr->linesize[1]-16,p_pic->frame_cnt);
                    #endif
                }
                #ifdef _config_ambadec_assert_
                else
                {
                    ambadec_assert_ffmpeg(pmvb->mvp[0].fw.ref_frame_num==not_valid_mv);
                    if(pmvb->mvp[0].fw.ref_frame_num!=not_valid_mv)
                    {
                        av_log(NULL,AV_LOG_ERROR,"pmvb->mvp[0].fw.ref_frame_num=%d.\n",pmvb->mvp[0].fw.ref_frame_num);
                    }
                }
                #endif

                //backward
                if(pmvb->mvp[0].bw.ref_frame_num==backward_ref_num)
                {
                    switch(pmvb->mb_setting.mv_type) {
                    case mv_type_16x16:
                        /*if(p_pic->current_picture_ptr->mb_type[i + j * p_pic->mb_stride]&MB_TYPE_GMC){
                            #ifdef __dump_DSP_TXT__
                                log_text_p(log_fd_dsp_text, "MV_TYPE_16X16 GMC:",p_pic->frame_cnt);
                                #ifdef __dump_separate__
                                log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_16X16 GMC:",p_pic->frame_cnt);
                                #endif
                            #endif
                            ambadec_assert_ffmpeg(0);
                            av_log(NULL,AV_LOG_ERROR,"h263_mpeg4_p_process_mc_idct_amba gloabol mc not supported.\n");

                            if(p_pic->real_sprite_warping_points==1){
                                gmc1_motion_amba(s,p_pic, dest_y, dest_cb, dest_cb+1,p_pic->next_picture_ptr->data,i,j);
                            }else{
                                gmc_motion_amba(s,p_pic, dest_y, dest_cb, dest_cb+1,p_pic->next_picture_ptr->data,i,j);
                            }
                        }else*/
                        if(p_pic->quarter_sample){
                            #ifdef __dump_DSP_TXT__
                                log_text_p(log_fd_dsp_text, "MV_TYPE_16X16 qpel_motion:",p_pic->frame_cnt);
                                #ifdef __dump_separate__
                                log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_16X16 qpel_motion:",p_pic->frame_cnt);
                                #endif
                            #endif
                            qpel_motion_amba(s,p_pic, dest_y, dest_cb,
                                        0, 0, 0,
                                        p_pic->next_picture_ptr->data, op_pix,op_pix_nv12,op_qpix,
                                        pmvb->mvp[0].bw.mv_x, pmvb->mvp[0].bw.mv_y, 16,i,j);
                        }
                        else
                        {
                            #ifdef __dump_DSP_TXT__
                                log_text_p(log_fd_dsp_text, "MV_TYPE_16X16 mpeg_motion:",p_pic->frame_cnt);
                                #ifdef __dump_separate__
                                log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_16X16 mpeg_motion:",p_pic->frame_cnt);
                                #endif
                            #endif
                            //debug_trap();
                            mpeg_motion_internal_amba(s, p_pic,dest_y, dest_cb,
                                        0, 0, 0,
                                        p_pic->next_picture_ptr->data, op_pix,op_pix_nv12,
                                        pmvb->mvp[0].bw.mv_x, pmvb->mvp[0].bw.mv_y, 16,i,j);
                        }
                        break;
                    case mv_type_8x8:
                    #ifdef __dump_DSP_TXT__
                        log_text_p(log_fd_dsp_text, "MV_TYPE_8X8:",p_pic->frame_cnt);
                        #ifdef __dump_separate__
                        log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_8X8:",p_pic->frame_cnt);
                        #endif
                    #endif
                    {
                        mx = 0;
                        my = 0;
                        if(p_pic->quarter_sample){
                            for(k=0;k<4;k++) {
                                motion_x = pmvb->mvp[k].bw.mv_x;
                                motion_y =pmvb->mvp[k].bw.mv_y;

                                dxy = ((motion_y & 3) << 2) | (motion_x & 3);
                                src_x = i * 16 + (motion_x >> 2) + (k & 1) * 8;
                                src_y = j * 16 + (motion_y >> 2) + (k >>1) * 8;

                                /* WARNING: do no forget half pels */
                                src_x = av_clip(src_x, -16, p_pic->width);
                                if (src_x == p_pic->width)
                                    dxy &= ~3;
                                src_y = av_clip(src_y, -16, p_pic->height);
                                if (src_y == p_pic->height)
                                    dxy &= ~12;

                                ptr = p_pic->next_picture_ptr->data[0]+ (src_y * linesize) + (src_x);
                                if(s->flags&CODEC_FLAG_EMU_EDGE){
                                    if(   (unsigned)src_x > p_pic->h_edge_pos - (motion_x&3) - 8
                                       || (unsigned)src_y > p_pic->v_edge_pos - (motion_y&3) - 8 ){
                                        s->dsp.emulated_edge_mc(s->edge_emu_buffer, ptr,
                                                            linesize, 9, 9,
                                                            src_x, src_y,
                                                            p_pic->h_edge_pos, p_pic->v_edge_pos);
                                        ptr= s->edge_emu_buffer;
                                    }
                                }

                                #ifdef __dump_DSP_TXT__
                                #ifdef __dump_separate__
                                snprintf(title,59,"motion_x=%d,motion_y=%d,dxy=%d",motion_x,motion_y,dxy);
                                log_text_p(log_fd_dsp_text+log_offset_mc, title,p_pic->frame_cnt);
                                #endif
                                #endif

                                dest = dest_y + ((k & 1) * 8) + (k >> 1) * 8 * linesize;
                                op_qpix[1][dxy](dest, ptr, linesize);

                                mx += pmvb->mvp[k].bw.mv_x/2;
                                my += pmvb->mvp[k].bw.mv_y/2;
                            }
                        }else{
                            for(k=0;k<4;k++) {

                                #ifdef __dump_DSP_TXT__
                                #ifdef __dump_separate__
                                snprintf(title,59,"motion_x=%d,motion_y=%d",pmvb->mvp[k].bw.mv_x,pmvb->mvp[k].bw.mv_y);
                                log_text_p(log_fd_dsp_text+log_offset_mc, title,p_pic->frame_cnt);
                                #endif
                                #endif

                                hpel_motion_amba(s,p_pic, dest_y + ((k & 1) * 8) + (k >> 1) * 8 * linesize,
                                            p_pic->next_picture_ptr->data[0], 0, 0,
                                            i * 16 + (k & 1) * 8, j * 16 + (k >>1) * 8,
                                            p_pic->width, p_pic->height, linesize,
                                            s->h_edge_pos, s->v_edge_pos,
                                            8, 8, op_pix[1],
                                            pmvb->mvp[k].bw.mv_x, pmvb->mvp[k].bw.mv_y);

                                mx += pmvb->mvp[k].bw.mv_x;
                                my += pmvb->mvp[k].bw.mv_y;
                            }
                        }

                        #ifdef __dump_DSP_TXT__
                        #ifdef __dump_separate__
                        snprintf(title,59,"chroma mx=%d,my=%d",mx,my);
                        log_text_p(log_fd_dsp_text+log_offset_mc, title,p_pic->frame_cnt);
                        #endif
                        #endif

                        if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY))
                            chroma_4mv_motion_amba(s,p_pic, dest_cb,  p_pic->next_picture_ptr->data, op_pix_nv12[1], mx, my,i,j);
                    }
                        break;
                    case mv_type_16x8:
                        #ifdef __dump_DSP_TXT__
                            log_text_p(log_fd_dsp_text, "MV_TYPE_FIELD:",p_pic->frame_cnt);
                            #ifdef __dump_separate__
                            log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_FIELD:",p_pic->frame_cnt);
                            #endif
                        #endif
                        ambadec_assert_ffmpeg(s->picture_structure == PICT_FRAME);

                        {
                            if(p_pic->quarter_sample){
                                for(k=0; k<2; k++){
                                    qpel_motion_amba(s,p_pic, dest_y, dest_cb,
                                                1, k, pmvb->mvp[k].bw.bottom,
                                                p_pic->next_picture_ptr->data, op_pix,op_pix_nv12, op_qpix,
                                                pmvb->mvp[k].bw.mv_x, pmvb->mvp[k].bw.mv_y, 8,i,j);
                                }
                            }else{
                                /* top field */
                                mpeg_motion_internal_amba(s,p_pic, dest_y, dest_cb,
                                            1, 0, pmvb->mvp[0].bw.bottom,
                                            p_pic->next_picture_ptr->data, op_pix,op_pix_nv12,
                                            pmvb->mvp[0].bw.mv_x, pmvb->mvp[0].bw.mv_y, 8,i,j);
                                /* bottom field */
                                mpeg_motion_internal_amba(s,p_pic, dest_y, dest_cb,
                                            1, 1, pmvb->mvp[1].bw.bottom,
                                            p_pic->next_picture_ptr->data, op_pix,op_pix_nv12,
                                            pmvb->mvp[1].bw.mv_x, pmvb->mvp[1].bw.mv_y, 8,i,j);
                            }
                        }
                        break;
                    default: assert(0);
                    }

                    #ifdef __dump_separate__
                    log_text_p(log_fd_dsp_text+log_offset_mc, "After Backward MC, result is:",p_pic->frame_cnt);
                    log_text_p(log_fd_dsp_text+log_offset_mc, "Y:",p_pic->frame_cnt);
                    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc,dest_y,16,16,p_pic->current_picture_ptr->linesize[0]-16,p_pic->frame_cnt);
                    log_text_p(log_fd_dsp_text+log_offset_mc, "Cb:",p_pic->frame_cnt);
                    log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb,8,8,p_pic->current_picture_ptr->linesize[1]-16,p_pic->frame_cnt);
                    log_text_p(log_fd_dsp_text+log_offset_mc, "Cr:",p_pic->frame_cnt);
                    log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb+1,8,8,p_pic->current_picture_ptr->linesize[1]-16,p_pic->frame_cnt);
                    #endif
                }
                #ifdef _config_ambadec_assert_
                else
                {
                    ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num==not_valid_mv);
                    if(pmvb->mvp[0].bw.ref_frame_num!=not_valid_mv)
                    {
                        av_log(NULL,AV_LOG_ERROR,"pmvb->mvp[0].bw.ref_frame_num=%d.\n",pmvb->mvp[0].bw.ref_frame_num);
                    }
                }
                #endif


                ambadec_assert_ffmpeg(s->chroma_y_shift);

                add_dct_amba(s,(*pdct)[0],0,dest_y,dct_linesize,pinfo->block_last_index[0] >= 0);
                add_dct_amba(s,(*pdct)[1],1,dest_y+ block_size,dct_linesize,pinfo->block_last_index[1] >= 0);
                add_dct_amba(s,(*pdct)[2],2,dest_y + dct_offset,dct_linesize,pinfo->block_last_index[2] >= 0);
                add_dct_amba(s,(*pdct)[3],3,dest_y + dct_offset + block_size,dct_linesize,pinfo->block_last_index[3] >= 0);

                add_dct_nv12_amba(s,(*pdct)[4],4,dest_cb,uvlinesize,pinfo->block_last_index[4] >= 0);
                add_dct_nv12_amba(s,(*pdct)[5],5,dest_cb+1,uvlinesize,pinfo->block_last_index[5] >= 0);

            }else{

                ambadec_assert_ffmpeg(pmvb->mvp[0].fw.ref_frame_num==not_valid_mv);
                ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num==not_valid_mv);

                //idct and add residue

                /* dct only in intra block */

                put_dct_amba(s,(*pdct)[0],0,dest_y,dct_linesize);
                put_dct_amba(s,(*pdct)[1],1,dest_y + block_size,dct_linesize);
                put_dct_amba(s,(*pdct)[2],2,dest_y + dct_offset,dct_linesize);
                put_dct_amba(s,(*pdct)[3],3,dest_y + dct_offset + block_size,dct_linesize);

                put_dct_nv12_amba(s,(*pdct)[4],4,dest_cb,uvlinesize);
                put_dct_nv12_amba(s,(*pdct)[5],5,dest_cb+1,uvlinesize);

            }

            #ifdef __dump_DSP_TXT__
            #ifdef __dump_separate__
            snprintf(title,39,"mb [y=%d, x=%d]:\n",j,i);
            log_text_p(log_fd_dsp_text+log_offset_dct,title,p_pic->frame_cnt);

            for(itmp=0;itmp<6;itmp++)
            {
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After Inverse Quantization: >>",p_pic->frame_cnt);
                log_text_rect_short_p(log_fd_dsp_text+log_offset_dct,s->logdct_deqaun[itmp],8,8,0,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT: >>",p_pic->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdct_result[itmp],8,8,0,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT+MC: >>",p_pic->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdctmc_result[itmp],8,8,0,p_pic->frame_cnt);
            }
            #endif
            #endif


            if(s->loop_filter)
            {
                av_log(NULL, AV_LOG_ERROR, "****Error here, DSP not support h263+ inloop filter.\n");
            }
            //if(s->loop_filter)
                //ff_h263_loop_filter_nv12(s);

            //update
            pmvb++;
            pdct++;
            dest_y+=16;
            dest_cb+=16;
            pinfo++;
        }
    }


}

void wmv2_i_process_idct_amba(Wmv2Context_amba *thiz, wmv2_pic_data_t* p_pic)
{
    MpegEncContext *s = &thiz->s;
    int i,j;
    //h263_pic_swinfo_t* pinfo=p_pic->pswinfo;
    dctarray pdct=(dctarray)p_pic->pdct;

    uint8_t *dest_y;
    uint8_t *dest_cb;

    mb_mv_P_t* pmvp=p_pic->pmvp;
    const int linesize= p_pic->current_picture_ptr->linesize[0]; //not s->linesize as this would be wrong for field pics
    const int uvlinesize= p_pic->current_picture_ptr->linesize[1];
    //const int readable= s->pict_type != FF_B_TYPE || s->encoding || s->avctx->draw_horiz_band || s->avctx->lowres ||1;
    const int block_size= s->avctx->lowres ? 8>>s->avctx->lowres : 8;
    const int /*dct_linesize, */dct_offset=linesize*block_size;
    ambadec_assert_ffmpeg(!s->avctx->lowres);

    ambadec_assert_ffmpeg(!thiz->use_dsp);


    if(thiz->use_permutated)
    {
        ambadec_assert_ffmpeg(!thiz->abt_flag);
        for(j=0;j<p_pic->mb_height;j++)
        {
            dest_y=p_pic->current_picture_ptr->data[0]+((j*linesize)<<4);
            dest_cb=p_pic->current_picture_ptr->data[1]+((j*uvlinesize)<<3);
            for(i=0;i<p_pic->mb_width;i++)
            {
                ambadec_assert_ffmpeg(!pmvp->mb_setting.dct_type);
                //dct_linesize = linesize << pmvp->mb_setting.dct_type;
                //dct_offset =(pmvp->mb_setting.dct_type)? linesize : linesize*block_size;

                #ifdef __dump_DSP_TXT__
                #ifdef __dump_separate__
                char title[40];
                snprintf(title,39,"mb [y=%d, x=%d]:\n",j,i);
                log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);

                memset(s->logdct_deqaun,0,sizeof(s->logdct_deqaun));
                memset(s->logdct_result,0,sizeof(s->logdct_result));
                memset(s->logdctmc_result,0,sizeof(s->logdctmc_result));
                #endif
                #endif

                transpose_matrix((*pdct)[0]);
                put_dct_amba(s,(*pdct)[0],0,dest_y,linesize);
                transpose_matrix((*pdct)[1]);
                put_dct_amba(s,(*pdct)[1],1,dest_y + block_size,linesize);
                transpose_matrix((*pdct)[2]);
                put_dct_amba(s,(*pdct)[2],2,dest_y + dct_offset,linesize);
                transpose_matrix((*pdct)[3]);
                put_dct_amba(s,(*pdct)[3],3,dest_y + dct_offset + block_size,linesize);

                transpose_matrix((*pdct)[4]);
                put_dct_nv12_amba(s,(*pdct)[4],4,dest_cb,uvlinesize);
                transpose_matrix((*pdct)[5]);
                put_dct_nv12_amba(s,(*pdct)[5],5,dest_cb+1,uvlinesize);

                #ifdef __dump_DSP_TXT__
                #ifdef __dump_separate__
                int itmp=0;
                snprintf(title,39,"mb [y=%d, x=%d]:\n",j,i);
                log_text_p(log_fd_dsp_text+log_offset_dct,title,p_pic->frame_cnt);
                for(itmp=0;itmp<6;itmp++)
                {
                    log_text_p(log_fd_dsp_text+log_offset_dct,"<< After Inverse Quantization: >>",p_pic->frame_cnt);
                    log_text_rect_short_p(log_fd_dsp_text+log_offset_dct,s->logdct_deqaun[itmp],8,8,0,p_pic->frame_cnt);
                    log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT: >>",p_pic->frame_cnt);
                    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdct_result[itmp],8,8,0,p_pic->frame_cnt);
                    log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT+MC: >>",p_pic->frame_cnt);
                    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdctmc_result[itmp],8,8,0,p_pic->frame_cnt);
                }
                #endif
                #endif


                /*if(s->loop_filter)
                {
                    av_log(NULL, AV_LOG_ERROR, "****Error here, DSP not support wmv2 inloop filter.\n");
                }*/
                //if(s->loop_filter)
                    //ff_h263_loop_filter_nv12(s);

                //update
                pdct++;
                dest_y+=16;
                dest_cb+=16;
                pmvp++;
                //pinfo++;
            }
        }
    }
    else
    {
        for(j=0;j<p_pic->mb_height;j++)
        {
            dest_y=p_pic->current_picture_ptr->data[0]+((j*linesize)<<4);
            dest_cb=p_pic->current_picture_ptr->data[1]+((j*uvlinesize)<<3);
            for(i=0;i<p_pic->mb_width;i++)
            {

                #ifdef __dump_DSP_TXT__
                #ifdef __dump_separate__
                char title[40];
                snprintf(title,39,"mb [y=%d, x=%d]:\n",j,i);
                log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);

                memset(s->logdct_deqaun,0,sizeof(s->logdct_deqaun));
                memset(s->logdct_result,0,sizeof(s->logdct_result));
                memset(s->logdctmc_result,0,sizeof(s->logdctmc_result));
                #endif
                #endif


                put_dct_amba(s,(*pdct)[0],0,dest_y,linesize);
                put_dct_amba(s,(*pdct)[1],1,dest_y + block_size,linesize);
                put_dct_amba(s,(*pdct)[2],2,dest_y + dct_offset,linesize);
                put_dct_amba(s,(*pdct)[3],3,dest_y + dct_offset + block_size,linesize);

                put_dct_nv12_amba(s,(*pdct)[4],4,dest_cb,uvlinesize);
                put_dct_nv12_amba(s,(*pdct)[5],5,dest_cb+1,uvlinesize);

                #ifdef __dump_DSP_TXT__
                #ifdef __dump_separate__
                int itmp=0;
                snprintf(title,39,"mb [y=%d, x=%d]:\n",j,i);
                log_text_p(log_fd_dsp_text+log_offset_dct,title,p_pic->frame_cnt);
                for(itmp=0;itmp<6;itmp++)
                {
                    log_text_p(log_fd_dsp_text+log_offset_dct,"<< After Inverse Quantization: >>",p_pic->frame_cnt);
                    log_text_rect_short_p(log_fd_dsp_text+log_offset_dct,s->logdct_deqaun[itmp],8,8,0,p_pic->frame_cnt);
                    log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT: >>",p_pic->frame_cnt);
                    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdct_result[itmp],8,8,0,p_pic->frame_cnt);
                    log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT+MC: >>",p_pic->frame_cnt);
                    log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdctmc_result[itmp],8,8,0,p_pic->frame_cnt);
                }
                #endif
                #endif


                if(s->loop_filter)
                {
                    av_log(NULL, AV_LOG_ERROR, "****Error here, DSP not support wmv2 inloop filter.\n");
                }
                //if(s->loop_filter)
                    //ff_h263_loop_filter_nv12(s);

                //update
                pdct++;
                dest_y+=16;
                dest_cb+=16;
                pmvp++;
                //pinfo++;
            }
        }
    }

}

void wmv2_p_process_mc_idct_amba(Wmv2Context_amba *thiz, wmv2_pic_data_t* p_pic)
{
    MpegEncContext *s = &thiz->s;
    int i,j,k;
    mb_mv_P_t* pmvp=p_pic->pmvp;
    h263_pic_swinfo_t* pinfo=p_pic->pswinfo;
    dctarray pdct=(dctarray)p_pic->pdct;
    int mx,my;
//    int dxy, src_x, src_y, motion_x, motion_y;
//    uint8_t *ptr, *dest;

    uint8_t *dest_y;
    uint8_t *dest_cb;
    //int dct_linesize, dct_offset;
    op_pixels_func (*op_pix)[4];
    op_pixels_func (*op_pix_nv12)[4];
    //qpel_mc_func (*op_qpix)[16];
    const int linesize= p_pic->current_picture_ptr->linesize[0]; //not s->linesize as this would be wrong for field pics
    const int uvlinesize= p_pic->current_picture_ptr->linesize[1];
    //const int readable= s->pict_type != FF_B_TYPE || s->encoding || s->avctx->draw_horiz_band || s->avctx->lowres ||1;
    const int block_size= s->avctx->lowres ? 8>>s->avctx->lowres : 8;
    const int dct_offset=linesize*block_size;
    ambadec_assert_ffmpeg(!s->avctx->lowres);

    ambadec_assert_ffmpeg(!thiz->use_dsp);
//    ambadec_assert_ffmpeg(!p_pic->no_rounding);
    ambadec_assert_ffmpeg(!s->quarter_sample);

    if ((!p_pic->no_rounding))
    {
        op_pix = s->dsp.put_pixels_tab;
        op_pix_nv12 = s->dsp.put_pixels_tab_nv12;
        #ifdef __dump_DSP_TXT__
            log_text_p(log_fd_dsp_text, "mc with no rounding",p_pic->frame_cnt);
            #ifdef __dump_separate__
                log_text_p(log_fd_dsp_text+log_offset_mc, "mc with no rounding",p_pic->frame_cnt);
            #endif
        #endif
    }else{
        op_pix = s->dsp.put_no_rnd_pixels_tab;
        op_pix_nv12 = s->dsp.put_no_rnd_pixels_tab_nv12;
        #ifdef __dump_DSP_TXT__
            log_text_p(log_fd_dsp_text, "mc with rounding",p_pic->frame_cnt);
            #ifdef __dump_separate__
                log_text_p(log_fd_dsp_text+log_offset_mc, "mc with rounding",p_pic->frame_cnt);
            #endif
        #endif
    }

    for(j=0;j<p_pic->mb_height;j++)
    {
        dest_y=p_pic->current_picture_ptr->data[0]+((j*linesize)<<4);
        dest_cb=p_pic->current_picture_ptr->data[1]+((j*uvlinesize)<<3);
        for(i=0;i<p_pic->mb_width;i++)
        {
            ambadec_assert_ffmpeg(!pmvp->mb_setting.dct_type);

            #ifdef __dump_DSP_TXT__
            #ifdef __dump_separate__
            int itmp=0;
            char title[60];
            snprintf(title,59,"mb [y=%d, x=%d]:\n",j,i);
            log_text_p(log_fd_dsp_text+log_offset_mc,title,p_pic->frame_cnt);

            memset(s->logdct_deqaun,0,sizeof(s->logdct_deqaun));
            memset(s->logdct_result,0,sizeof(s->logdct_result));
            memset(s->logdctmc_result,0,sizeof(s->logdctmc_result));
            #endif
            #endif

            if(p_pic->use_permutated)
            {
                //transpose
                transpose_matrix((*pdct)[0]);
                transpose_matrix((*pdct)[1]);
                transpose_matrix((*pdct)[2]);
                transpose_matrix((*pdct)[3]);
                transpose_matrix((*pdct)[4]);
                transpose_matrix((*pdct)[5]);
            }

            //mc
            if (!pmvp->mb_setting.intra)
            {
                /* decoding or more than one mb_type (MC was already done otherwise) */
                switch(pmvp->mb_setting.mv_type) {

                   case mv_type_16x16:
                        #ifdef __dump_DSP_TXT__
                            log_text_p(log_fd_dsp_text, "MV_TYPE_16X16 mpeg_motion:",p_pic->frame_cnt);
                            #ifdef __dump_separate__
                            log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_16X16 mpeg_motion:",p_pic->frame_cnt);
                            #endif
                        #endif
                        //debug_trap();
                        mpeg_motion_internal_amba(s, (h263_pic_data_t*)p_pic,dest_y, dest_cb,
                                    0, 0, 0,
                                    p_pic->last_picture_ptr->data, op_pix,op_pix_nv12,
                                    pmvp->mv[0].mv_x, pmvp->mv[0].mv_y, 16,i,j);
                        break;

                    case mv_type_8x8:
                        #ifdef __dump_DSP_TXT__
                            log_text_p(log_fd_dsp_text, "MV_TYPE_8X8:",p_pic->frame_cnt);
                            #ifdef __dump_separate__
                            log_text_p(log_fd_dsp_text+log_offset_mc, "MV_TYPE_8X8:",p_pic->frame_cnt);
                            #endif
                        #endif
                        {
                            mx = 0;
                            my = 0;

                            for(k=0;k<4;k++) {

                                #ifdef __dump_DSP_TXT__
                                #ifdef __dump_separate__
                                snprintf(title,59,"motion_x=%d,motion_y=%d",pmvp->mv[k].mv_x,pmvp->mv[k].mv_y);
                                log_text_p(log_fd_dsp_text+log_offset_mc, title,p_pic->frame_cnt);
                                #endif
                                #endif

                                hpel_motion_amba(s,(h263_pic_data_t*)p_pic, dest_y + ((k & 1) * 8) + (k >> 1) * 8 * linesize,
                                            p_pic->last_picture_ptr->data[0], 0, 0,
                                            i * 16 + (k & 1) * 8, j * 16 + (k >>1) * 8,
                                            p_pic->width, p_pic->height, linesize,
                                            s->h_edge_pos, s->v_edge_pos,
                                            8, 8, op_pix[1],
                                            pmvp->mv[k].mv_x, pmvp->mv[k].mv_y);

                                mx += pmvp->mv[k].mv_x;
                                my += pmvp->mv[k].mv_y;
                            }

                            #ifdef __dump_DSP_TXT__
                            #ifdef __dump_separate__
                            snprintf(title,59,"chroma mx=%d,my=%d",mx,my);
                            log_text_p(log_fd_dsp_text+log_offset_mc, title,p_pic->frame_cnt);
                            #endif
                            #endif

                            if(!CONFIG_GRAY || !(s->flags&CODEC_FLAG_GRAY))
                                chroma_4mv_motion_amba(s,(h263_pic_data_t*)p_pic, dest_cb,  p_pic->last_picture_ptr->data, op_pix_nv12[1], mx, my,i,j);
                        }
                        break;
                    default: assert(0);
                }

                #ifdef __dump_separate__
                log_text_p(log_fd_dsp_text+log_offset_mc, "After Forward MC, result is:",p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_mc, "Y:",p_pic->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_mc,dest_y,16,16,s->linesize-16,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_mc, "Cb:",p_pic->frame_cnt);
                log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb,8,8,s->uvlinesize-16,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_mc, "Cr:",p_pic->frame_cnt);
                log_text_rect_char_hex_chroma_p(log_fd_dsp_text+log_offset_mc,dest_cb+1,8,8,s->uvlinesize-16,p_pic->frame_cnt);
                #endif

                ambadec_assert_ffmpeg(s->chroma_y_shift);

                if(thiz->abt_flag)
                {
                    ff_simple_add_block((*pdct)[0], dest_y, linesize);
                    ff_simple_add_block((*pdct)[1], dest_y+ block_size, linesize);
                    ff_simple_add_block((*pdct)[2], dest_y + dct_offset, linesize);
                    ff_simple_add_block((*pdct)[3], dest_y + dct_offset + block_size, linesize);

                    ff_simple_add_block_nv12((*pdct)[4], dest_cb, uvlinesize);
                    ff_simple_add_block_nv12((*pdct)[5], dest_cb+1, uvlinesize);
                }
                else
                {
                    /* add dct residue */
                    add_dct_amba(s,(*pdct)[0],0,dest_y,linesize,pinfo->block_last_index[0] >= 0);
                    add_dct_amba(s,(*pdct)[1],1,dest_y+ block_size,linesize,pinfo->block_last_index[1] >= 0);
                    add_dct_amba(s,(*pdct)[2],2,dest_y + dct_offset,linesize,pinfo->block_last_index[2] >= 0);
                    add_dct_amba(s,(*pdct)[3],3,dest_y + dct_offset + block_size,linesize,pinfo->block_last_index[3] >= 0);

                    add_dct_nv12_amba(s,(*pdct)[4],4,dest_cb,uvlinesize,pinfo->block_last_index[4] >= 0);
                    add_dct_nv12_amba(s,(*pdct)[5],5,dest_cb+1,uvlinesize,pinfo->block_last_index[5] >= 0);
                }
            }else{

                /* dct only in intra block */

                put_dct_amba(s,(*pdct)[0],0,dest_y,linesize);
                put_dct_amba(s,(*pdct)[1],1,dest_y + block_size,linesize);
                put_dct_amba(s,(*pdct)[2],2,dest_y + dct_offset,linesize);
                put_dct_amba(s,(*pdct)[3],3,dest_y + dct_offset + block_size,linesize);

                put_dct_nv12_amba(s,(*pdct)[4],4,dest_cb,uvlinesize);
                put_dct_nv12_amba(s,(*pdct)[5],5,dest_cb+1,uvlinesize);

            }

            #ifdef __dump_DSP_TXT__
            #ifdef __dump_separate__
            snprintf(title,39,"mb [y=%d, x=%d]:\n",j,i);
            log_text_p(log_fd_dsp_text+log_offset_dct,title,p_pic->frame_cnt);

            for(itmp=0;itmp<6;itmp++)
            {
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After Inverse Quantization: >>",p_pic->frame_cnt);
                log_text_rect_short_p(log_fd_dsp_text+log_offset_dct,s->logdct_deqaun[itmp],8,8,0,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT: >>",p_pic->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdct_result[itmp],8,8,0,p_pic->frame_cnt);
                log_text_p(log_fd_dsp_text+log_offset_dct,"<< After IDCT+MC: >>",p_pic->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_text+log_offset_dct,s->logdctmc_result[itmp],8,8,0,p_pic->frame_cnt);
            }
            #endif
            #endif

            //idct and add residue
            /*
            if(s->loop_filter)
            {
                av_log(NULL, AV_LOG_ERROR, "****Error here, DSP not support wmv2 inloop filter.\n");
            }*/
            //if(s->loop_filter)
                //ff_h263_loop_filter_nv12(s);

            //update
            pmvp++;
            pdct++;
            dest_y+=16;
            dest_cb+=16;
            pinfo++;
        }
    }

}

void ff_init_block_index_nv12(MpegEncContext *s){ //FIXME maybe rename
    const int linesize= s->current_picture.linesize[0]; //not s->linesize as this would be wrong for field pics
    const int uvlinesize= s->current_picture.linesize[1];
    const int mb_size= 4 - s->avctx->lowres;

    s->block_index[0]= s->b8_stride*(s->mb_y*2    ) - 2 + s->mb_x*2;
    s->block_index[1]= s->b8_stride*(s->mb_y*2    ) - 1 + s->mb_x*2;
    s->block_index[2]= s->b8_stride*(s->mb_y*2 + 1) - 2 + s->mb_x*2;
    s->block_index[3]= s->b8_stride*(s->mb_y*2 + 1) - 1 + s->mb_x*2;
    s->block_index[4]= s->mb_stride*(s->mb_y + 1)                + s->b8_stride*s->mb_height*2 + s->mb_x - 1;
    s->block_index[5]= s->mb_stride*(s->mb_y + s->mb_height + 2) + s->b8_stride*s->mb_height*2 + s->mb_x - 1;
    //block_index is not used by mpeg2, so it is not affected by chroma_format

    s->dest[0] = s->current_picture.data[0] + ((s->mb_x - 1) << mb_size);
    s->dest[1] = s->current_picture.data[1] + ((s->mb_x - 1) << (mb_size ));
    s->dest[2] = s->current_picture.data[2] + ((s->mb_x - 1) << (mb_size ));

    if(!(s->pict_type==FF_B_TYPE && s->avctx->draw_horiz_band && s->picture_structure==PICT_FRAME))
    {
        if(s->picture_structure==PICT_FRAME){
            s->dest[0] += s->mb_y *   linesize << mb_size;
            s->dest[1] += s->mb_y * uvlinesize << (mb_size - s->chroma_y_shift);
            s->dest[2] += s->mb_y * uvlinesize << (mb_size - s->chroma_y_shift);
        }else{
            s->dest[0] += (s->mb_y>>1) *   linesize << mb_size;
            s->dest[1] += (s->mb_y>>1) * uvlinesize << (mb_size - s->chroma_y_shift);
            s->dest[2] += (s->mb_y>>1) * uvlinesize << (mb_size - s->chroma_y_shift);
            assert((s->mb_y&1) == (s->picture_structure == PICT_BOTTOM_FIELD));
        }
    }
}

void ff_init_block_index_amba(MpegEncContext *s){ //FIXME maybe rename
    s->block_index[0]= s->b8_stride*(s->mb_y*2    ) - 2 + s->mb_x*2;
    s->block_index[1]= s->b8_stride*(s->mb_y*2    ) - 1 + s->mb_x*2;
    s->block_index[2]= s->b8_stride*(s->mb_y*2 + 1) - 2 + s->mb_x*2;
    s->block_index[3]= s->b8_stride*(s->mb_y*2 + 1) - 1 + s->mb_x*2;
    s->block_index[4]= s->mb_stride*(s->mb_y + 1)                + s->b8_stride*s->mb_height*2 + s->mb_x - 1;
    s->block_index[5]= s->mb_stride*(s->mb_y + s->mb_height + 2) + s->b8_stride*s->mb_height*2 + s->mb_x - 1;
}

#ifdef __tmp_use_amba_permutation__
void switch_permutated(MpegEncContext *s, int to_dsp)
{
    //to use dsp permutated matrix
    if(to_dsp)
    {
        s->dsp.p_idct_permutation=s->dsp.amba_idct_permutation;
        s->p_intra_matrix=s->amba_intra_matrix;
        s->p_inter_matrix=s->amba_inter_matrix;
        s->p_chroma_intra_matrix=s->amba_chroma_intra_matrix;
        s->p_chroma_inter_matrix=s->amba_chroma_inter_matrix;

        s->p_intra_scantable=&s->amba_intra_scantable;
        s->p_intra_h_scantable=&s->amba_intra_h_scantable;
        s->p_intra_v_scantable=&s->amba_intra_v_scantable;
        s->p_inter_scantable=&s->amba_inter_scantable;
    }
    else
    {
        s->dsp.p_idct_permutation=s->dsp.idct_permutation;
        s->p_intra_matrix=s->intra_matrix;
        s->p_inter_matrix=s->inter_matrix;
        s->p_chroma_intra_matrix=s->chroma_intra_matrix;
        s->p_chroma_inter_matrix=s->chroma_inter_matrix;

        s->p_intra_scantable=&s->intra_scantable;
        s->p_intra_h_scantable=&s->intra_h_scantable;
        s->p_intra_v_scantable=&s->intra_v_scantable;
        s->p_inter_scantable=&s->inter_scantable;
    }
}
#endif
