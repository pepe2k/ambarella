/*
 * amba RV40 decoder
 *
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
 * @file libavcodec/amba_rv40.c
 * amba RV40 decoder
 */

#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "golomb.h"

#include "amba_rv34.h"
#include "rv40vlc2.h"
#include "rv40data.h"
#include "log_dump.h"

const enum PixelFormat ff_pixfmt_list_amba[] = {
    PIX_FMT_NV12,
    PIX_FMT_NONE
};

#ifdef _config_rv40_neon_

#endif

/**
 * Initialize all tables.
 */
static av_cold void rv40_amba_init_tables(void)
{
    int i;
    static VLC_TYPE aic_table[1 << AIC_TOP_BITS][2];
    static VLC_TYPE aic_mode1_table[AIC_MODE1_NUM << AIC_MODE1_BITS][2];
    static VLC_TYPE aic_mode2_table[11814][2];
    static VLC_TYPE ptype_table[NUM_PTYPE_VLCS << PTYPE_VLC_BITS][2];
    static VLC_TYPE btype_table[NUM_BTYPE_VLCS << BTYPE_VLC_BITS][2];

    aic_top_vlc.table = aic_table;
    aic_top_vlc.table_allocated = 1 << AIC_TOP_BITS;
    init_vlc(&aic_top_vlc, AIC_TOP_BITS, AIC_TOP_SIZE,
             rv40_aic_top_vlc_bits,  1, 1,
             rv40_aic_top_vlc_codes, 1, 1, INIT_VLC_USE_NEW_STATIC);
    for(i = 0; i < AIC_MODE1_NUM; i++){
        // Every tenth VLC table is empty
        if((i % 10) == 9) continue;
        aic_mode1_vlc[i].table = &aic_mode1_table[i << AIC_MODE1_BITS];
        aic_mode1_vlc[i].table_allocated = 1 << AIC_MODE1_BITS;
        init_vlc(&aic_mode1_vlc[i], AIC_MODE1_BITS, AIC_MODE1_SIZE,
                 aic_mode1_vlc_bits[i],  1, 1,
                 aic_mode1_vlc_codes[i], 1, 1, INIT_VLC_USE_NEW_STATIC);
    }
    for(i = 0; i < AIC_MODE2_NUM; i++){
        aic_mode2_vlc[i].table = &aic_mode2_table[mode2_offs[i]];
        aic_mode2_vlc[i].table_allocated = mode2_offs[i + 1] - mode2_offs[i];
        init_vlc(&aic_mode2_vlc[i], AIC_MODE2_BITS, AIC_MODE2_SIZE,
                 aic_mode2_vlc_bits[i],  1, 1,
                 aic_mode2_vlc_codes[i], 2, 2, INIT_VLC_USE_NEW_STATIC);
    }
    for(i = 0; i < NUM_PTYPE_VLCS; i++){
        ptype_vlc[i].table = &ptype_table[i << PTYPE_VLC_BITS];
        ptype_vlc[i].table_allocated = 1 << PTYPE_VLC_BITS;
        init_vlc_sparse(&ptype_vlc[i], PTYPE_VLC_BITS, PTYPE_VLC_SIZE,
                         ptype_vlc_bits[i],  1, 1,
                         ptype_vlc_codes[i], 1, 1,
                         ptype_vlc_syms,     1, 1, INIT_VLC_USE_NEW_STATIC);
    }
    for(i = 0; i < NUM_BTYPE_VLCS; i++){
        btype_vlc[i].table = &btype_table[i << BTYPE_VLC_BITS];
        btype_vlc[i].table_allocated = 1 << BTYPE_VLC_BITS;
        init_vlc_sparse(&btype_vlc[i], BTYPE_VLC_BITS, BTYPE_VLC_SIZE,
                         btype_vlc_bits[i],  1, 1,
                         btype_vlc_codes[i], 1, 1,
                         btype_vlc_syms,     1, 1, INIT_VLC_USE_NEW_STATIC);
    }
}

/**
 * Get stored dimension from bitstream.
 *
 * If the width/height is the standard one then it's coded as a 3-bit index.
 * Otherwise it is coded as escaped 8-bit portions.
 */
static int get_dimension(GetBitContext *gb, const int *dim)
{
    int t   = get_bits(gb, 3);
    int val = dim[t];
    if(val < 0)
        val = dim[get_bits1(gb) - val];
    if(!val){
        do{
            t = get_bits(gb, 8);
            val += t << 2;
        }while(t == 0xFF);
    }
    return val;
}

/**
 * Get encoded picture size - usually this is called from rv40_parse_slice_header.
 */
static void rv40_parse_picture_size(GetBitContext *gb, int *w, int *h)
{
    *w = get_dimension(gb, rv40_standard_widths);
    *h = get_dimension(gb, rv40_standard_heights);
}

static int rv40_amba_parse_slice_header(RV34AmbaDecContext *r, GetBitContext *gb, SliceInfo *si)
{
    int mb_bits;
    int w = r->s.width, h = r->s.height;
    int mb_size;

    memset(si, 0, sizeof(SliceInfo));
    if(get_bits1(gb))
        return -1;
    si->type = get_bits(gb, 2);
    if(si->type == 1) si->type = 0;
    si->quant = get_bits(gb, 5);
    if(get_bits(gb, 2))
        return -1;
    si->vlc_set = get_bits(gb, 2);
    skip_bits1(gb);
    si->pts = get_bits(gb, 13);
    if(!si->type || !get_bits1(gb))
        rv40_parse_picture_size(gb, &w, &h);
    if(av_image_check_size(w, h, 0, r->s.avctx) < 0)
        return -1;
    si->width  = w;
    si->height = h;
    mb_size = ((w + 15) >> 4) * ((h + 15) >> 4);
    mb_bits = ff_rv34_amba_get_start_offset(gb, mb_size);
    si->start = get_bits(gb, mb_bits);

    return 0;
}

/**
 * Decode 4x4 intra types array.
 */
static int rv40_amba_decode_intra_types(RV34AmbaDecContext *r, GetBitContext *gb, int8_t *dst)
{
    MpegEncContext *s = &r->s;
    int i, j, k, v;
    int A, B, C;
    int pattern;
    int8_t *ptr;

    for(i = 0; i < 4; i++, dst += r->intra_types_stride){
        if(!i && s->first_slice_line){
            pattern = get_vlc2(gb, aic_top_vlc.table, AIC_TOP_BITS, 1);
            dst[0] = (pattern >> 2) & 2;
            dst[1] = (pattern >> 1) & 2;
            dst[2] =  pattern       & 2;
            dst[3] = (pattern << 1) & 2;
            continue;
        }
        ptr = dst;
        for(j = 0; j < 4; j++){
            /* Coefficients are read using VLC chosen by the prediction pattern
             * The first one (used for retrieving a pair of coefficients) is
             * constructed from the top, top right and left coefficients
             * The second one (used for retrieving only one coefficient) is
             * top + 10 * left.
             */
            A = ptr[-r->intra_types_stride + 1]; // it won't be used for the last coefficient in a row
            B = ptr[-r->intra_types_stride];
            C = ptr[-1];
            pattern = A + (B << 4) + (C << 8);
            for(k = 0; k < MODE2_PATTERNS_NUM; k++)
                if(pattern == rv40_aic_table_index[k])
                    break;
            if(j < 3 && k < MODE2_PATTERNS_NUM){ //pattern is found, decoding 2 coefficients
                v = get_vlc2(gb, aic_mode2_vlc[k].table, AIC_MODE2_BITS, 2);
                *ptr++ = v/9;
                *ptr++ = v%9;
                j++;
            }else{
                if(B != -1 && C != -1)
                    v = get_vlc2(gb, aic_mode1_vlc[B + C*10].table, AIC_MODE1_BITS, 1);
                else{ // tricky decoding
                    v = 0;
                    switch(C){
                    case -1: // code 0 -> 1, 1 -> 0
                        if(B < 2)
                            v = get_bits1(gb) ^ 1;
                        break;
                    case  0:
                    case  2: // code 0 -> 2, 1 -> 0
                        v = (get_bits1(gb) ^ 1) << 1;
                        break;
                    }
                }
                *ptr++ = v;
            }
        }
    }
    return 0;
}

/**
 * Decode macroblock information.
 */
static int rv40_amba_decode_mb_info(RV34AmbaDecContext *r)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int q, i;
    int prev_type = 0;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int blocks[RV34_MB_TYPES] = {0};
    int count = 0;

    if(!r->s.mb_skip_run)
        r->s.mb_skip_run = svq3_get_ue_golomb(gb) + 1;

    if(--r->s.mb_skip_run)
         return RV34_MB_SKIP;

    if(r->avail_cache[6-1])
        blocks[r->mb_type[mb_pos - 1]]++;
    if(r->avail_cache[6-4]){
        blocks[r->mb_type[mb_pos - s->mb_stride]]++;
        if(r->avail_cache[6-2])
            blocks[r->mb_type[mb_pos - s->mb_stride + 1]]++;
        if(r->avail_cache[6-5])
            blocks[r->mb_type[mb_pos - s->mb_stride - 1]]++;
    }

    for(i = 0; i < RV34_MB_TYPES; i++){
        if(blocks[i] > count){
            count = blocks[i];
            prev_type = i;
        }
    }
    if(s->pict_type == FF_P_TYPE){
        prev_type = block_num_to_ptype_vlc_num[prev_type];
        q = get_vlc2(gb, ptype_vlc[prev_type].table, PTYPE_VLC_BITS, 1);
        if(q < PBTYPE_ESCAPE)
            return q;
        q = get_vlc2(gb, ptype_vlc[prev_type].table, PTYPE_VLC_BITS, 1);
        av_log(s->avctx, AV_LOG_ERROR, "Dquant for P-frame\n");
    }else{
        prev_type = block_num_to_btype_vlc_num[prev_type];
        q = get_vlc2(gb, btype_vlc[prev_type].table, BTYPE_VLC_BITS, 1);
        if(q < PBTYPE_ESCAPE)
            return q;
        q = get_vlc2(gb, btype_vlc[prev_type].table, BTYPE_VLC_BITS, 1);
        av_log(s->avctx, AV_LOG_ERROR, "Dquant for B-frame\n");
    }
    return 0;
}

#define CLIP_SYMM(a, b) av_clip(a, -(b), b)
/**
 * weaker deblocking very similar to the one described in 4.4.2 of JVT-A003r1
 */
static inline void rv40_weak_loop_filter_amba(uint8_t *src, const int step,
                                         const int filter_p1, const int filter_q1,
                                         const int alpha, const int beta,
                                         const int lim_p0q0,
                                         const int lim_q1, const int lim_p1,
                                         const int diff_p1p0, const int diff_q1q0,
                                         const int diff_p1p2, const int diff_q1q2)
{
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
    int t, u, diff;

    t = src[0*step] - src[-1*step];
    if(!t)
        return;
    u = (alpha * FFABS(t)) >> 7;
    if(u > 3 - (filter_p1 && filter_q1))
        return;

    t <<= 2;
    if(filter_p1 && filter_q1)
        t += src[-2*step] - src[1*step];
    diff = CLIP_SYMM((t + 4) >> 3, lim_p0q0);
    src[-1*step] = cm[src[-1*step] + diff];
    src[ 0*step] = cm[src[ 0*step] - diff];
    if(FFABS(diff_p1p2) <= beta && filter_p1){
        t = (diff_p1p0 + diff_p1p2 - diff) >> 1;
        src[-2*step] = cm[src[-2*step] - CLIP_SYMM(t, lim_p1)];
    }
    if(FFABS(diff_q1q2) <= beta && filter_q1){
        t = (diff_q1q0 + diff_q1q2 + diff) >> 1;
        src[ 1*step] = cm[src[ 1*step] - CLIP_SYMM(t, lim_q1)];
    }
}

enum RV40BlockPos{
    POS_CUR,
    POS_TOP,
    POS_LEFT,
    POS_BOTTOM,
};

#define MASK_CUR          0x0001
#define MASK_RIGHT        0x0008
#define MASK_BOTTOM       0x0010
#define MASK_TOP          0x1000
#define MASK_Y_TOP_ROW    0x000F
#define MASK_Y_LAST_ROW   0xF000
#define MASK_Y_LEFT_COL   0x1111
#define MASK_Y_RIGHT_COL  0x8888
#define MASK_C_TOP_ROW    0x0003
#define MASK_C_LAST_ROW   0x000C
#define MASK_C_LEFT_COL   0x0005
#define MASK_C_RIGHT_COL  0x000A

static const int neighbour_offs_x[4] = { 0,  0, -1, 0 };
static const int neighbour_offs_y[4] = { 0, -1,  0, 1 };


//#define neonnotfinished
//#ifdef neonnotfinished
#ifndef _config_rv40_neon_
static inline void rv40_adaptive_loop_filter_amba(uint8_t *src, const int step,
                                             const int stride, const int dmode,
                                             const int lim_q1, const int lim_p1,
                                             const int alpha,
                                             const int beta, const int beta2,
                                             const int chroma, const int edge)
{
    int diff_p1p0[4], diff_q1q0[4], diff_p1p2[4], diff_q1q2[4];
    int sum_p1p0 = 0, sum_q1q0 = 0, sum_p1p2 = 0, sum_q1q2 = 0;
    uint8_t *ptr;
    int flag_strong0 = 1, flag_strong1 = 1;
    int filter_p1, filter_q1;
    int i;
    int lims;

    for(i = 0, ptr = src; i < 4; i++, ptr += stride){
        diff_p1p0[i] = ptr[-2*step] - ptr[-1*step];
        diff_q1q0[i] = ptr[ 1*step] - ptr[ 0*step];
        sum_p1p0 += diff_p1p0[i];
        sum_q1q0 += diff_q1q0[i];
    }
    filter_p1 = FFABS(sum_p1p0) < (beta<<2);
    filter_q1 = FFABS(sum_q1q0) < (beta<<2);
    if(!filter_p1 && !filter_q1)
        return;

    for(i = 0, ptr = src; i < 4; i++, ptr += stride){
        diff_p1p2[i] = ptr[-2*step] - ptr[-3*step];
        diff_q1q2[i] = ptr[ 1*step] - ptr[ 2*step];
        sum_p1p2 += diff_p1p2[i];
        sum_q1q2 += diff_q1q2[i];
    }

    if(edge){
        flag_strong0 = filter_p1 && (FFABS(sum_p1p2) < beta2);
        flag_strong1 = filter_q1 && (FFABS(sum_q1q2) < beta2);
    }else{
        flag_strong0 = flag_strong1 = 0;
    }

    lims = filter_p1 + filter_q1 + ((lim_q1 + lim_p1) >> 1) + 1;
    if(flag_strong0 && flag_strong1){ /* strong filtering */
        for(i = 0; i < 4; i++, src += stride){
            int sflag, p0, q0, p1, q1;
            int t = src[0*step] - src[-1*step];

            if(!t) continue;
            sflag = (alpha * FFABS(t)) >> 7;
            if(sflag > 1) continue;

            p0 = (25*src[-3*step] + 26*src[-2*step]
                + 26*src[-1*step]
                + 26*src[ 0*step] + 25*src[ 1*step] + rv40_dither_l[dmode + i]) >> 7;
            q0 = (25*src[-2*step] + 26*src[-1*step]
                + 26*src[ 0*step]
                + 26*src[ 1*step] + 25*src[ 2*step] + rv40_dither_r[dmode + i]) >> 7;
            if(sflag){
                p0 = av_clip(p0, src[-1*step] - lims, src[-1*step] + lims);
                q0 = av_clip(q0, src[ 0*step] - lims, src[ 0*step] + lims);
            }
            p1 = (25*src[-4*step] + 26*src[-3*step]
                + 26*src[-2*step]
                + 26*p0           + 25*src[ 0*step] + rv40_dither_l[dmode + i]) >> 7;
            q1 = (25*src[-1*step] + 26*q0
                + 26*src[ 1*step]
                + 26*src[ 2*step] + 25*src[ 3*step] + rv40_dither_r[dmode + i]) >> 7;
            if(sflag){
                p1 = av_clip(p1, src[-2*step] - lims, src[-2*step] + lims);
                q1 = av_clip(q1, src[ 1*step] - lims, src[ 1*step] + lims);
            }
            src[-2*step] = p1;
            src[-1*step] = p0;
            src[ 0*step] = q0;
            src[ 1*step] = q1;
            if(!chroma){
                src[-3*step] = (25*src[-1*step] + 26*src[-2*step] + 51*src[-3*step] + 26*src[-4*step] + 64) >> 7;
                src[ 2*step] = (25*src[ 0*step] + 26*src[ 1*step] + 51*src[ 2*step] + 26*src[ 3*step] + 64) >> 7;
            }
        }
    }else if(filter_p1 && filter_q1){
        for(i = 0; i < 4; i++, src += stride)
            rv40_weak_loop_filter_amba(src, step, 1, 1, alpha, beta, lims, lim_q1, lim_p1,
                                  diff_p1p0[i], diff_q1q0[i], diff_p1p2[i], diff_q1q2[i]);
    }else{
        for(i = 0; i < 4; i++, src += stride)
            rv40_weak_loop_filter_amba(src, step, filter_p1, filter_q1,
                                  alpha, beta, lims>>1, lim_q1>>1, lim_p1>>1,
                                  diff_p1p0[i], diff_q1q0[i], diff_p1p2[i], diff_q1q2[i]);
    }
}

static void rv40_v_loop_filter_y(uint8_t *src, int stride, int dmode,
                               int lim_q1, int lim_p1,
                               int alpha, int beta, int beta2, int chroma, int edge){
    rv40_adaptive_loop_filter_amba(src, 1, stride, dmode, lim_q1, lim_p1,
                              alpha, beta, beta2, chroma, edge);
}
static void rv40_h_loop_filter_y(uint8_t *src, int stride, int dmode,
                               int lim_q1, int lim_p1,
                               int alpha, int beta, int beta2, int chroma, int edge){
    rv40_adaptive_loop_filter_amba(src, stride, 1, dmode, lim_q1, lim_p1,
                              alpha, beta, beta2, chroma, edge);
}

static void rv40_v_loop_filter_uv(uint8_t *src, int stride, int dmode,
                               int lim_q1, int lim_p1,
                               int alpha, int beta, int beta2, int chroma, int edge){
    rv40_adaptive_loop_filter_amba(src, 2, stride, dmode, lim_q1, lim_p1,
                              alpha, beta, beta2, chroma, edge);
}
static void rv40_h_loop_filter_uv(uint8_t *src, int stride, int dmode,
                               int lim_q1, int lim_p1,
                               int alpha, int beta, int beta2, int chroma, int edge){
    rv40_adaptive_loop_filter_amba(src, stride, 2, dmode, lim_q1, lim_p1,
                              alpha, beta, beta2, chroma, edge);
}
#else

/*
argu[]=
{
    dmode,     //0
    lim_q1,      //1
    lim_p1,      //2
    alpha,        //3
    beta,         //4
    beta2,       //5
    chroma,     //6
    edge          //7
    rv40_dither_l //8
    rv40_dither_r //9
}
*/
static inline void rv40_v_loop_filter_neon_y(uint8_t *src,
                                             const int stride, int* argu /*const int dmode,
                                             const int lim_q1, const int lim_p1,
                                             const int alpha,
                                             const int beta, const int beta2,
                                             const int chroma, const int edge*/)
{

//output/input: diff_p1p0, diff_q1q0, diff_p1p2, diff_q1q2
//output: filter_p1,filter_q1
//input: src, stride, beta
//middle used: sum_p1p0 , sum_q1q0

__asm__ volatile (
        "sub %0, %0, #4 \n\t"
        "pld [%0] \n\t"
        "pld [%0, %1] \n\t"
        "ldmia %0, {r4, r5} \n\t"   /*load data to d0, d1, d2, d3*/
        "pld [%0, %1, LSL #1] \n\t"
        "add %0, %1 \n\t"
        "vmov d0, r4, r5 \n\t"
        "ldmia %0, {r4, r5} \n\t"
        "vmovl.u8 q4, d0 \n\t"   /* expand data to q4, q5, q6, q7 */
        "vmov d1, r4, r5 \n\t"
        "add %0, %1 \n\t"
        "ldmia %0, {r4, r5} \n\t"
        "vmovl.u8 q5, d1 \n\t"
        "vmov d2, r4, r5 \n\t"
        "add %0, %1 \n\t"
        "ldmia %0, {r4, r5} \n\t"
        "vmovl.u8 q6, d2 \n\t"
        "vmov d3, r4, r5 \n\t"
        "vmovl.u8 q7, d3 \n\t"
        "vtrn.32  q4, q6 \n\t"     /*transpose, q4([-4], [0]), q5([-3], [1], q6([-2], [2]), q7([-1], [3])  (reserved !!)*/
        "vtrn.32  q5, q7 \n\t"
        "ldr        r9, [%2, #16] \n\t"    /*r9= beta*/
        "vtrn.16  q4, q5 \n\t"
        "vtrn.16  q6, q7 \n\t"
        "vsub.s16 d16, d12, d14 \n\t"  /*d16: diff_p1p0[i] = ptr[-2*step] - ptr[-1*step] */
        "vsub.s16 d17, d11, d9 \n\t"    /*d17: diff_q1q0[i] = ptr[ 1*step] - ptr[ 0*step] */
        "vpadd.s16 d4, d16, d17 \n\t"  /*add sum*/
        "ldr      r6, [r2, #20] \n\t"    /*r6: beta2*/
        "mov      r9, r9, LSL #2 \n\t"    /*beta<<=2*/
        "vpaddl.s16 d5, d4 \n\t"          /*sum_p1p0: d5[0], sum_q1q0:d5[1]*/
        "vabs.s32 d6, d5 \n\t"            /*get absolute value*/
        "vmov r4, r5, d6 \n\t"            /*r4=sum_p1p0, r5=sum_q1q0*/
        "cmp r4, r9 \n\t"
        "ite   lt \n\t"
        "movlt r4, #1 \n\t"                /*r4=filter_p1*/
        "movge r4, #0 \n\t"
        "cmp r5, r9 \n\t"
        "ite   lt \n\t"
        "movlt r5, #1 \n\t"                /*r5=filter_q1*/
        "movge r5, #0 \n\t"
        "orrs r6, r5, r4 \n\t"
        "beq 6f \n\t "                    /*if(!filter_p1 && !filter_q1) return*/
        "vsub.s16 d18, d12, d10 \n\t"    /*d18:  diff_p1p2[i] = ptr[-2*step] - ptr[-3*step] */
        "vsub.s16 d19, d11, d13 \n\t"    /*d19:  diff_q1q2[i] = ptr[ 1*step] - ptr[ 2*step]; */
        "vpadd.s16 d6, d18, d19 \n\t"  /*add sum*/
        "vpaddl.s16 d7, d6 \n\t"          /*sum_p1p2: d7[0], sum_q1q2:d7[1]*/
        "vabs.s32 d4, d7 \n\t"            /*get absolute value*/
        "vmov r7, r8, d4 \n\t"            /*r7=sum_p1p2, r8=sum_q1q2*/
        "cmp r7, r6 \n\t"
        "ite  lt \n\t"
        "movlt r7, #2 \n\t"                /*r7=filter_p2*/
        "movge r7, #0 \n\t"
        "cmp r8, r6 \n\t"
        "ite  lt \n\t"
        "movlt r8, #2 \n\t"                /*r8=filter_q2*/
        "movge r8, #0 \n\t"
        "orr r7, r7, r4 \n\t"
        "ldr  r3, [%2, #4] \n\t"          /* load */
        "ldr  r10, [%2, #8] \n\t"
        "ldr  r6, [%2, #28] \n\t"
        "add     r3, r3, r10 \n\t"         /*calculate lims*/
        "add     r10, r4, r5 \n\t"
        "add     r10, r10, r3, LSR #1 \n\t"
        "orr r8, r8, r5 \n\t"
        "add     r10, r10, #1 \n\t"        /*r10: lims (reserved !!)*/
        "orr r8, r8, r7 \n\t"
        "orrs r8, r8, r6 \n\t"              /*r8, flag_strong0 && flag_strong1*/
        "beq    4f \n\t"

        "ldr    r4, [%2, #12] \n\t"         /*start strong filter: r10:lims, r4: alpha*/
        "vsub.s16 d17, d9, d14 \n\t"    /*d17:t = src[0*step] - src[-1*step]  (reserved !!)*/
        "vmov.16  d0[0], r4 \n\t"
        "vabs.s16 d21, d17 \n\t"         /* d21: abs(t)*/
        "vmull.s16 q15, d21, d0[0] \n\t"
        "vshr.u32  q14, q15, #7 \n\t"
        "vmov.u16 d29, #0 \n\t"       /*d29: 0*/
        "vmovn.u32 d16, q14 \n\t"    /*d16: sflag (reserved !!)*/

        "vcgt.u16 d26, d17, d29 \n\t"
        "vmov.s16 d31, #1 \n\t"
        "vcgt.u16 d26, d17, d29 \n\t"
        "vcge.s16 d27, d31, d16 \n\t"
        "vand.u16 d17, d26, d27 \n\t"  /* d17:t!=0 && sflag<=1 (reserved !!) */

        "vmov       r5, r6, d17 \n\t"      /*strong filter check*/
        "orrs          r7, r5, r6 \n\t"
        "beq          6f \n\t"

        "ldr        r5, [%2] \n\t"             /*dmode*/
        "ldr        r6, [%2, #32] \n\t"             /*r6: rv40_dither_l*/
        "ldr        r7, [%2, #36] \n\t"             /*r6: rv40_dither_r*/
        "ldr        r8, [r6, r5, LSL #2] \n\t"                /*read 4 bytes, need fixed, 4bytes align?*/
        "ldr        r9, [r7, r5, LSL #2] \n\t"                /*read 4 bytes, need fixed, 4bytes align?*/
        "vmov    d31, r8, r9  \n\t"
        "vmovl.u8 q9, d31 \n\t"             /*q9: rv40_dither_l[dmode + i] and rv40_dither_r[dmode + i] (reserved !!)*/
        "vadd.s16  d24, d12, d14 \n\t "   /*src[-2]+ src[-1]*/
        "vadd.s16  d23, d9, d11 \n\t "    /*src[0]+ src[1]*/
        "vadd.s16  d24, d24, d23 \n\t "    /* d24:src[0]+ src[1]+src[-2]+src[-1]*/

        "vdup.16    d20, r10 \n\t"                     /*d20 expand limis (!!reserved)*/

        "vadd.s16  d0, d24, d10 \n\t"    /*cal p0: d3*/
        "vshl.u16   d28, d0, #4 \n\t"     /*16x*/
        "vshl.u16   d29, d0, #3 \n\t"     /*8x*/
        "vadd.u16  d29, d28, d29 \n\t"     /*24x*/
        "vadd.u16  d0, d0, d29 \n\t"     /*25x*/
        "vadd.u16  d0, d0, d18 \n\t"
        "vsub.s16  d0, d0, d11 \n\t"
        "vadd.s16  d0, d24, d0 \n\t"
        "vshr.u16   d6, d0, #7 \n\t"        /*d6: p0: [-1]*/

        "vadd.s16  d7, d24, d13 \n\t"     /*cal q0: d4*/
        "vshl.u16   d30, d7, #4 \n\t"     /*16x*/
        "vshl.u16   d31, d7, #3 \n\t"     /*8x*/
        "vadd.u16  d31, d30, d31 \n\t"     /*24x*/
        "vadd.u16  d7, d7, d31 \n\t"     /*25x*/
        "vadd.u16  d7, d7, d19 \n\t"
        "vsub.s16  d7, d7, d12 \n\t"
        "vadd.s16  d7, d24, d6 \n\t"
        "vshr.u16   d1, d7, #7 \n\t"        /*d1: q0: [0]*/

        "vsub.s16   d26, d14, d20 \n\t"             /*clip min*/
        "vadd.s16   d27, d14, d20 \n\t"             /*clip max*/
        "vmax.s16  d26, d26, d6 \n\t"
        "vmin.s16  d26, d26, d27 \n\t"

        "vsub.s16   d24, d9, d20 \n\t"             /*clip min*/
        "vadd.s16   d25, d9, d20 \n\t"             /*clip max*/
        "vmax.s16  d24, d1, d24 \n\t"
        "vmin.s16  d24, d24, d25 \n\t"

        "vmov.u16  d31, #0 \n\t"
        "vcgt.u16   d22, d16, d31 \n\t"              /*clip mask*/
        "vcge.u16    d23, d31, d16 \n\t"

        "vand.u16  d26, d22, d26 \n\t"            /* merge result */
        "vand.u16  d6, d23, d6 \n\t"
        "vorr.u16  d6, d6, d26 \n\t"                /*d6: p0: [-1]*/

        "vand.u16  d24, d22, d24 \n\t"
        "vand.u16  d1, d23, d1 \n\t"
        "vorr.u16  d1, d1, d24 \n\t"               /*d1: q0: [0]*/

        "vadd.s16 d28, d8, d10 \n\t"
        "vadd.s16 d29, d12, d9 \n\t"
        "vadd.s16 d29, d28, d29 \n\t"
        "vadd.s16 d26, d29, d3 \n\t"
        "vshr.u16 d28, d26, #4 \n\t "      /*16x*/
        "vshr.u16 d29, d26, #3 \n\t "      /*8x*/
        "vadd.s16 d29, d28, d29 \n\t"    /*24x*/
        "vadd.s16 d29, d26, d29 \n\t"      /*25x*/
        "vadd.s16 d28, d10, d12 \n\t"
        "vadd.s16 d29, d29, d28 \n\t"
        "vadd.s16 d28, d6, d18 \n\t"
        "vadd.s16 d29, d29, d28 \n\t"
        "vshr.u16  d4, d29, #7 \n\t"           /*d4: p1 [-2]*/

        "vadd.s16 d30, d14, d11 \n\t"
        "vadd.s16 d31, d13, d15 \n\t"
        "vadd.s16 d31, d30, d31 \n\t"
        "vadd.s16 d27, d31, d4 \n\t"
        "vshr.u16 d30, d27, #4 \n\t "      /*16x*/
        "vshr.u16 d31, d27, #3 \n\t "      /*8x*/
        "vadd.s16 d31, d30, d31 \n\t"    /*24x*/
        "vadd.s16 d31, d27, d31 \n\t"      /*25x*/
        "vadd.s16 d30, d13, d15 \n\t"
        "vadd.s16 d31, d30, d31 \n\t"
        "vadd.s16 d30, d1, d19 \n\t"
        "vadd.s16 d31, d30, d31 \n\t"
        "vshr.u16  d3, d31, #7 \n\t"         /* d3: q1 [1]*/

        "vsub.s16   d26, d12, d20 \n\t"             /*clip min*/
        "vadd.s16   d27, d12, d20 \n\t"             /*clip max*/
        "vmax.s16  d26, d4, d26 \n\t"
        "vmin.s16  d26, d27, d26 \n\t"

        "vsub.s16   d24, d11, d20 \n\t"             /*clip min*/
        "vadd.s16   d25, d11, d20 \n\t"             /*clip max*/
        "vmax.s16  d24, d3, d24 \n\t"
        "vmin.s16  d24, d24, d25 \n\t"

        "vmov.u16  d31, #0 \n\t"
        "vcgt.u16   d22, d16, d31 \n\t"              /*clip mask*/
        "vcge.u16    d23, d31, d16 \n\t"

        "vand.u16  d26, d26, d22 \n\t"            /* merge result */
        "vand.u16  d4, d4, d23 \n\t"
        "vorr.u16   d4, d26, d4 \n\t"                /*d4: p1 [-2]*/

        "vand.u16  d24, d24, d22 \n\t"
        "vand.u16  d3, d3, d23 \n\t"
        "vorr.u16  d3, d3, d25 \n\t"               /* d3: q1 [1]*/


        "ldr     r3, [%2, #24] \n\t"                     /*chroma*/
        "movs r3, r3 \n\t"

        "beq 1f \n\t"
        "vadd.s16 q14, q4, q5 \n\t"
        "vadd.s16 q15, q6, q7 \n\t"
        "vadd.s16 q15, q14, q15 \n\t"
        "vmov.s16 q12, q15 \n\t"
        "vmov.s16 q13, #64 \n\t"
        "vadd.s16 d30, d30, d10 \n\t"
        "vadd.s16 d31, d31, d13 \n\t"
        "vadd.s16 q12, q12, q13 \n\t"
        "vshr.u16 q14, q15, #4 \n\t"              /*16x*/
        "vshr.u16 q13, q15, #3 \n\t"              /*8x*/
        "vadd.s16 q15, q15, q14 \n\t"
        "vadd.s16 q15, q15, q13 \n\t"            /*25x*/
        "vsub.s16 d30, d30, d14 \n\t"
        "vsub.s16 d31, d31, d9 \n\t"
        "vadd.s16 d31, d31, d12 \n\t"
        "vshr.u16 d2, d30, #7 \n\t"               /*d2: p2, [-3]*/
        "vshr.u16 d5, d31, #7 \n\t"               /*d5: q2, [2]*/
        "beq 2f \n\t"

        "1:\n\t"
        "vmov.u16  d2, d10  \n\t"
        "vmov.u16  d5, d13  \n\t"

        "2:\n\t"
        "vmov.u16  d0, d8  \n\t"
        "vmov.u16  d7, d15  \n\t"

        "vmovl.u16 q10, d17 \n\t"

        "vtrn.32  q0, q2 \n\t"     /*transpose back, q0([-4], [0]), q1([-3], [1], q2([-2], [2]), q3([-1], [3])  */
        "vtrn.32  q1, q3 \n\t"
        "vmov     r4, r5, d20 \n\t"
        "vtrn.16  q0, q1 \n\t"
        "vtrn.16  q2, q3 \n\t"

        "add       %2, %1, %1, LSL #1 \n\t"
        "vmovn.u16  d22, q0 \n\t"
        "vmovn.u16  d23, q1 \n\t"
        "vmovn.u16  d24, q2 \n\t"
        "vmovn.u16  d25, q3 \n\t"

        "sub       %0, %0, %2 \n\t"

        "vmov     r6, r7, d22 \n\t"
        "movs     r4, r4 \n\t"                                  /*write back data*/
        "itt    ne \n\t"
        "strne     r7, [%0, #4] \n\t"
        "strne     r6, [%0] \n\t"
        "add       %0, %0, %1 \n\t"

        "vmov     r8, r9, d23 \n\t"
        "movs     r5, r5 \n\t"
         "itt    ne \n\t"
        "strne     r9, [%0, #4] \n\t"
        "strne     r8, [%0] \n\t"
        "add       %0, %0, %1 \n\t"

        "vmov     r4, r5, d21 \n\t"
        "vmov     r6, r7, d24 \n\t"
        "movs     r4, r4 \n\t"
        "itt    ne \n\t"
        "strne     r7, [%0, #4] \n\t"
        "strne     r6, [%0] \n\t"
        "add       %0, %0, %1 \n\t"

        "vmov     r8, r9, d25 \n\t"
        "movs     r5, r5 \n\t"
        "itt    ne \n\t"
        "strne     r9, [%0, #4] \n\t"
        "strne     r8, [%0] \n\t"

        "b 6f \n\t"

        "4:\n\t"             /*weak-deblock: d16-d19: diff_p1p0[i], diff_q1q0[i], diff_p1p2[i], diff_q1q2[i]*/
        "ldr    r3, [%2, #12] \n\t"         /*r3: alpha*/

        "vsub.s16 d27, d9, d14 \n\t"    /*d27:t = src[0*step] - src[-1*step]  (reserved !!)*/
        "vmov.16  d0[0], r3 \n\t"
        "vabs.s16 d25, d17 \n\t"         /* d21: abs(t)*/
        "vmull.s16 q15, d25, d0[0]\n\t"
        "vshr.u32  q14, q15, #7 \n\t"
        "vmovn.u32 d16, q14 \n\t"    /*d16: sflag (reserved !!)*/

        "vmov.u16 d31, #0 \n\t"
        "vcgt.u16 d26, d27, d31 \n\t"
        "mov      r6, #3 \n\t"
        "and       r7, r4, r5 \n\t"
        "sub       r6, r6, r7 \n\t"
        "vdup.16  d31, r6 \n\t"
        "vcgt.u16 d28, d27, d31 \n\t"
        "vcge.s16 d29, d31, d26 \n\t"
        "vand.u16 d20, d28, d29 \n\t"  /* d20:t!=0 && sflag<=1 (reserved !!) */

        "vmov       r7, r8, d20 \n\t"      /*strong filter check*/
        "orrs          r7, r8, r7 \n\t"
        "beq          6f \n\t"

        "ands r6, r5, r4 \n\t"   /*r4=filter_p1, r5=filter_q1, r10: lims , r9 beta<<2*/
        "mov  r9, r9, LSR #2 \n\t"     /*r9: beta*/
        "ldr    r7, [%2, #4] \n\t"         /*r7: lim_q1*/
        "ldr    r8, [%2, #8] \n\t"         /*r8: lim_p1*/
        "ittt     eq \n\t"
        "moveq  r10, r10, LSR #1 \n\t"
        "moveq  r7, r7, LSR #1 \n\t"
        "moveq  r8, r8, LSR #1 \n\t"

        "vshl.u16   d27, d27, #2 \n\t"   /*t<<=2*/
        "beq 3f \n\t"
        "vsub.s16  d28, d12, d11 \n\t"   /*d28: src[-2*step] - src[1*step]*/
        "vadd.s16  d27, d27, d28 \n\t"
        "3:\n\t"
        "vmov.s16 d22, #4 \n\t"
        "vadd.s16 d27, d27, d22 \n\t"
        "vshr.u16  d27, #3 \n\t"          /*d27: (t + 4) >> 3*/
        "vdup.16   d30, r10 \n\t"         /*d30: lim_p0q0(lims)*/
        "vabs.s16  d31, d30 \n\t"        /*max*/
        "vneg.s16  d30, d31 \n\t"        /*min*/
        "vmax.s16  d30, d30, d27 \n\t "
        "vmin.s16   d24, d30, d31 \n\t "   /*d24: diff=CLIP_SYMM((t + 4) >> 3, lim_p0q0)*/

        "vmov.u16  q0, q4 \n\t"              /*copy*/
        "vmov.u16  q1, q5 \n\t"
        "vmov.u16  q2, q6 \n\t"
        "vmov.u16  q3, q7 \n\t"

        "vsub.s16   d1, d8, d24 \n\t"       /*d1: src[ 0*step] - diff;*/
        "vadd.s16   d6, d14, d24 \n\t"     /*d6: src[-1*step] + diff;*/

        "vdup.16     d21, r9 \n\t"            /*d21: beta*/
        "vdup.16     d22, r4 \n\t"            /*d22: filter_p1*/
        "vdup.16     d23, r5 \n\t"            /*d23: filter_q1*/

        "vabs.s16   d28, d18 \n\t"          /*d28: FFABS(diff_p1p2)*/
        "vabs.s16   d29, d19 \n\t"          /*d28: FFABS(diff_q1q2)*/

        "vmov.u16   d31, #0 \n\t"
        "vcgt.u16    d22, d22, d31 \n\t"    /*condition mask (filter_p1)*/
        "vcgt.u16    d23, d23, d31 \n\t"    /*condition mask (filter_q1)*/

        "vcge.s16    d30, d21, d28 \n\t"
        "vcge.s16    d31, d21, d29 \n\t"
        "vand.u16    d22, d22, d30 \n\t"   /*condition mask if(FFABS(diff_p1p2) <= beta && filter_p1)*/
        "vand.u16    d23, d23, d31 \n\t"   /*condition mask if(FFABS(diff_q1q2) <= beta && filter_q1)*/

        "vadd.s16    d28, d16, d17 \n\t"
        "vadd.s16    d29, d18, d19 \n\t"
        "vsub.s16    d28, d28, d24 \n\t"
        "vadd.s16    d29, d29, d24 \n\t"
        "vshr.u16     d28, d28, #1 \n\t"   /*d28: t = (diff_p1p0 + diff_p1p2 - diff) >> 1;*/
        "vshr.u16     d29, d29, #1 \n\t"   /*d29: t = (diff_q1q0 + diff_q1q2 + diff) >> 1;*/

        "vdup.16     d21, r8 \n\t"            /*d21: lim_p1*/
        "vdup.16     d25, r7 \n\t"            /*d25: lim_q1*/

        "vneg.s16    d30, d21 \n\t"
        "vneg.s16    d31, d25 \n\t"

        "vmin.s16    d21, d21, d28 \n\t"
        "vmin.s16    d25, d25, d29 \n\t"
        "vmax.s16   d21, d21, d30 \n\t"    /*d21: CLIP_SYMM(t, lim_p1)*/
        "vmax.s16   d25, d25, d31 \n\t"    /*d25: CLIP_SYMM(t, lim_q1)*/

        "vsub.s16    d16, d12, d21 \n\t"
        "vsub.s16    d17, d11, d25 \n\t"

        "vbit.u16      d4,  d16, d22 \n\t"
        "vbit.u16      d3,  d17, d23 \n\t"    /*insert result according to mask*/

        "vmovl.u16 q10, d20 \n\t"

        "vtrn.32  q0, q2 \n\t"     /*transpose back, q0([-4], [0]), q1([-3], [1], q2([-2], [2]), q3([-1], [3])  */
        "vtrn.32  q1, q3 \n\t"
        "vmov     r4, r5, d20 \n\t"
        "vtrn.16  q0, q1 \n\t"
        "vtrn.16  q2, q3 \n\t"

        "add       %2, %1, %1, LSL #1 \n\t"
        "vmovn.u16  d22, q0 \n\t"
        "vmovn.u16  d23, q1 \n\t"
        "vmovn.u16  d24, q2 \n\t"
        "vmovn.u16  d25, q3 \n\t"

        "sub       %0, %0, %2 \n\t"

        "vmov     r6, r7, d22 \n\t"
        "movs     r4, r4 \n\t"                                  /*write back data*/
        "itt    ne \n\t"
        "strne     r7, [%0, #4] \n\t"
        "strne     r6, [%0] \n\t"
        "add       %0, %0, %1 \n\t"

        "vmov     r8, r9, d23 \n\t"
        "movs     r5, r5 \n\t"
         "itt    ne \n\t"
        "strne     r9, [%0, #4] \n\t"
        "strne     r8, [%0] \n\t"
        "add       %0, %0, %1 \n\t"

        "vmov     r4, r5, d21 \n\t"
        "vmov     r6, r7, d24 \n\t"
        "movs     r4, r4 \n\t"
        "itt    ne \n\t"
        "strne     r7, [%0, #4] \n\t"
        "strne     r6, [%0] \n\t"
        "add       %0, %0, %1 \n\t"

        "vmov     r8, r9, d25 \n\t"
        "movs     r5, r5 \n\t"
        "itt    ne \n\t"
        "strne     r9, [%0, #4] \n\t"
        "strne     r8, [%0] \n\t"

        "6:\n\t"

        :
        :"r"(src), "r"(stride), "r"(argu) /*, "r"(dmode), "r"(lim_q1),"r"(lim_p1),"r"(alpha),"r"(beta), "r"(beta2), "r"(chroma), "r"(edge)*/
        // %0          %1            %2              %3           %4             %5          %6          %7            %8              %9
        :"r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "cc", "memory"
);

}

static inline void rv40_h_loop_filter_neon_y(uint8_t *src,
                                             const int stride, int* argu)
{

}

static inline void rv40_v_loop_filter_neon_uv(uint8_t *src,
                                             const int stride, int* argu)
{

}

static inline void rv40_h_loop_filter_neon_uv(uint8_t *src,
                                             const int stride, int* argu)
{

}

//pack input arguments:
/*
static inline void rv40_v_loop_filter_y(uint8_t *src,
                                             const int stride, const int dmode,
                                             const int lim_q1, const int lim_p1,
                                             const int alpha,
                                             const int beta, const int beta2,
                                             const int chroma, const int edge)
             ---->
static inline void rv40_v_loop_filter_y(uint8_t *src,
                                             const int stride, int* argu)
argu[]=
{
    dmode,     //0
    lim_q1,      //1
    lim_p1,      //2
    alpha,        //3
    beta,         //4
    beta2,       //5
    chroma,     //6
    edge          //7
    rv40_dither_l //8
    rv40_dither_r //9
}
*/
typedef enum
{
    e_dmode=0,
    e_lim_q1=1,
    e_lim_p1=2,
    e_alpha=3,        //3
    e_beta=4,         //4
    e_beta2=5,
    e_chroma=6,
    e_edge=7,
    e_rv40_dither_l=8,
    e_rv40_dither_r=9,
    e_loop_filter_argu_tot_cnt,
}e_loop_filter_argu;

void rv40new_neon_loop_filter(RV34AmbaDecContext *r,rv40_pic_data_t* pic_data, int row)
{
//    MpegEncContext *s = &r->s;
    int argu[e_loop_filter_argu_tot_cnt]={0,0,0,0,0, 0,0,0,neon_rv40_dither_l,neon_rv40_dither_r};
    int mb_pos, mb_x;
    int i, j, k;
    uint8_t *Y, *C;
    //int alpha, beta, betaY, betaC;
    int q;
    int mbtype[4];   ///< current macroblock and its neighbours types
    /**
     * flags indicating that macroblock can be filtered with strong filter
     * it is set only for intra coded MB and MB with DCs coded separately
     */
    int mb_strong[4];
    int clip[4];     ///< MB filter clipping value calculated from filtering strength
    /**
     * coded block patterns for luma part of current macroblock and its neighbours
     * Format:
     * LSB corresponds to the top left block,
     * each nibble represents one row of subblocks.
     */
    int cbp[4];
    /**
     * coded block patterns for chroma part of current macroblock and its neighbours
     * Format is the same as for luma with two subblocks in a row.
     */
    int uvcbp[4][2];
    /**
     * This mask represents the pattern of luma subblocks that should be filtered
     * in addition to the coded ones because because they lie at the edge of
     * 8x8 block with different enough motion vectors
     */
    int mvmasks[4];
    int linesize=pic_data->current_picture_ptr->linesize[0];
    int uvlinesize=pic_data->current_picture_ptr->linesize[1];

    mb_pos = row * pic_data->mb_stride;
    for(mb_x = 0; mb_x < pic_data->mb_width; mb_x++, mb_pos++){
        i = pic_data->current_picture_ptr->mb_type[mb_pos];
        if(IS_INTRA(i) || IS_SEPARATE_DC(i))
            pic_data->cbp_luma  [mb_pos] = pic_data->deblock_coefs[mb_pos] = 0xFFFF;
        if(IS_INTRA(i))
            pic_data->cbp_chroma[mb_pos] = 0xFF;
    }
    mb_pos = row * pic_data->mb_stride;
    for(mb_x = 0; mb_x < pic_data->mb_width; mb_x++, mb_pos++){
        int y_h_deblock, y_v_deblock;
        int c_v_deblock[2], c_h_deblock[2];
        int clip_left;
        int avail[4];
        int y_to_deblock, c_to_deblock[2];

        q = pic_data->current_picture_ptr->qscale_table[mb_pos];
        //alpha = rv40_alpha_tab[q];
        argu[e_alpha]=rv40_alpha_tab[q];
        //beta  = rv40_beta_tab [q];
        argu[e_beta]=rv40_alpha_tab[q];
        //betaY = betaC = beta * 3;
        argu[e_beta2]=rv40_alpha_tab[q]*3;
        if(pic_data->width * pic_data->height <= 176*144)
            argu[e_beta2]+=argu[e_beta];
            //betaY += beta;

        avail[0] = 1;
        avail[1] = row;
        avail[2] = mb_x;
        avail[3] = row < pic_data->mb_height - 1;
        for(i = 0; i < 4; i++){
            if(avail[i]){
                int pos = mb_pos + neighbour_offs_x[i] + neighbour_offs_y[i]*pic_data->mb_stride;
                mvmasks[i] = pic_data->deblock_coefs[pos];
                mbtype [i] = pic_data->current_picture_ptr->mb_type[pos];
                cbp    [i] = pic_data->cbp_luma[pos];
                uvcbp[i][0] = pic_data->cbp_chroma[pos] & 0xF;
                uvcbp[i][1] = pic_data->cbp_chroma[pos] >> 4;
            }else{
                mvmasks[i] = 0;
                mbtype [i] = mbtype[0];
                cbp    [i] = 0;
                uvcbp[i][0] = uvcbp[i][1] = 0;
            }
            mb_strong[i] = IS_INTRA(mbtype[i]) || IS_SEPARATE_DC(mbtype[i]);
            clip[i] = rv40_filter_clip_tbl[mb_strong[i] + 1][q];
        }
        y_to_deblock =  mvmasks[POS_CUR]
                     | (mvmasks[POS_BOTTOM] << 16);
        /* This pattern contains bits signalling that horizontal edges of
         * the current block can be filtered.
         * That happens when either of adjacent subblocks is coded or lies on
         * the edge of 8x8 blocks with motion vectors differing by more than
         * 3/4 pel in any component (any edge orientation for some reason).
         */
        y_h_deblock =   y_to_deblock
                    | ((cbp[POS_CUR]                           <<  4) & ~MASK_Y_TOP_ROW)
                    | ((cbp[POS_TOP]        & MASK_Y_LAST_ROW) >> 12);
        /* This pattern contains bits signalling that vertical edges of
         * the current block can be filtered.
         * That happens when either of adjacent subblocks is coded or lies on
         * the edge of 8x8 blocks with motion vectors differing by more than
         * 3/4 pel in any component (any edge orientation for some reason).
         */
        y_v_deblock =   y_to_deblock
                    | ((cbp[POS_CUR]                      << 1) & ~MASK_Y_LEFT_COL)
                    | ((cbp[POS_LEFT] & MASK_Y_RIGHT_COL) >> 3);
        if(!mb_x)
            y_v_deblock &= ~MASK_Y_LEFT_COL;
        if(!row)
            y_h_deblock &= ~MASK_Y_TOP_ROW;
        if(row == pic_data->mb_height - 1 || (mb_strong[POS_CUR] || mb_strong[POS_BOTTOM]))
            y_h_deblock &= ~(MASK_Y_TOP_ROW << 16);
        /* Calculating chroma patterns is similar and easier since there is
         * no motion vector pattern for them.
         */
        for(i = 0; i < 2; i++){
            c_to_deblock[i] = (uvcbp[POS_BOTTOM][i] << 4) | uvcbp[POS_CUR][i];
            c_v_deblock[i] =   c_to_deblock[i]
                           | ((uvcbp[POS_CUR] [i]                       << 1) & ~MASK_C_LEFT_COL)
                           | ((uvcbp[POS_LEFT][i]   & MASK_C_RIGHT_COL) >> 1);
            c_h_deblock[i] =   c_to_deblock[i]
                           | ((uvcbp[POS_TOP][i]    & MASK_C_LAST_ROW)  >> 2)
                           |  (uvcbp[POS_CUR][i]                        << 2);
            if(!mb_x)
                c_v_deblock[i] &= ~MASK_C_LEFT_COL;
            if(!row)
                c_h_deblock[i] &= ~MASK_C_TOP_ROW;
            if(row == pic_data->mb_height - 1 || mb_strong[POS_CUR] || mb_strong[POS_BOTTOM])
                c_h_deblock[i] &= ~(MASK_C_TOP_ROW << 4);
        }

        for(j = 0; j < 16; j += 4){
            Y = pic_data->current_picture_ptr->data[0] + mb_x*16 + (row*16 + j) * linesize;
            for(i = 0; i < 4; i++, Y += 4){
                int ij = i + j;
                //int clip_cur = y_to_deblock & (MASK_CUR << ij) ? clip[POS_CUR] : 0;
                argu[e_lim_q1]=y_to_deblock & (MASK_CUR << ij) ? clip[POS_CUR] : 0;
                //int dither = j ? ij : i*4;
                argu[e_dmode]=j ? ij : i*4;
                argu[e_edge]=0;
                // if bottom block is coded then we can filter its top edge
                // (or bottom edge of this block, which is the same)
                if(y_h_deblock & (MASK_BOTTOM << ij)){
                    argu[e_lim_p1]=y_to_deblock & (MASK_BOTTOM << ij) ? clip[POS_CUR] : 0;

                    _rv40_h_loop_filter_neon_y(Y+4*linesize, linesize, argu);/*dither,
                                       y_to_deblock & (MASK_BOTTOM << ij) ? clip[POS_CUR] : 0,
                                       clip_cur,
                                       alpha, beta, betaY, 0, 0);*/
                }
                // filter left block edge in ordinary mode (with low filtering strength)
                if(y_v_deblock & (MASK_CUR << ij) && (i || !(mb_strong[POS_CUR] || mb_strong[POS_LEFT]))){
                    if(!i)
                    {
                        argu[e_lim_p1]=mvmasks[POS_LEFT] & (MASK_RIGHT << j) ? clip[POS_LEFT] : 0;
                        //clip_left = mvmasks[POS_LEFT] & (MASK_RIGHT << j) ? clip[POS_LEFT] : 0;
                    }
                    else
                    {
                        argu[e_lim_p1]=y_to_deblock & (MASK_CUR << (ij-1)) ? clip[POS_CUR] : 0;
                        //clip_left = y_to_deblock & (MASK_CUR << (ij-1)) ? clip[POS_CUR] : 0;
                    }

                    _rv40_v_loop_filter_neon_y(Y, linesize, argu);/*dither,
                                       clip_cur,
                                       clip_left,
                                       alpha, beta, betaY, 0, 0);*/
                }
                argu[e_edge]=1;
                // filter top edge of the current macroblock when filtering strength is high
                if(!j && y_h_deblock & (MASK_CUR << i) && (mb_strong[POS_CUR] || mb_strong[POS_TOP])){
                    argu[e_lim_p1]=mvmasks[POS_TOP] & (MASK_TOP << i) ? clip[POS_TOP] : 0;
                    _rv40_h_loop_filter_neon_y(Y, linesize, argu);/*dither,
                                       clip_cur,
                                       mvmasks[POS_TOP] & (MASK_TOP << i) ? clip[POS_TOP] : 0,
                                       alpha, beta, betaY, 0, 1);*/
                }
                // filter left block edge in edge mode (with high filtering strength)
                if(y_v_deblock & (MASK_CUR << ij) && !i && (mb_strong[POS_CUR] || mb_strong[POS_LEFT])){
                    //clip_left = mvmasks[POS_LEFT] & (MASK_RIGHT << j) ? clip[POS_LEFT] : 0;
                    argu[e_lim_p1]=mvmasks[POS_LEFT] & (MASK_RIGHT << j) ? clip[POS_LEFT] : 0;
                    _rv40_v_loop_filter_neon_y(Y, linesize,argu);/* dither,
                                       clip_cur,
                                       clip_left,
                                       alpha, beta, betaY, 0, 1);*/
                }
            }
        }
        //betaC
        argu[e_beta2]=argu[e_beta]*3;
        argu[e_chroma]=1;

        for(j = 0; j < 2; j++){
            C = pic_data->current_picture_ptr->data[1] + mb_x*16 + (row*8 + j*4) * uvlinesize;
            for(i = 0; i < 2; i++, C += 8){
                int ij = i + j*2;
                //int clip_cur = c_to_deblock[k] & (MASK_CUR << ij) ? clip[POS_CUR] : 0;
                argu[e_lim_q1]=c_to_deblock[0] & (MASK_CUR << ij) ? clip[POS_CUR] : 0;
                argu[e_edge]=0;
                if(c_h_deblock[0] & (MASK_CUR << (ij+2))){
                    //int clip_bot = c_to_deblock[k] & (MASK_CUR << (ij+2)) ? clip[POS_CUR] : 0;
                    argu[e_lim_p1]=c_to_deblock[0] & (MASK_CUR << (ij+2)) ? clip[POS_CUR] : 0;
                    argu[e_dmode]=i*8;
                    _rv40_h_loop_filter_neon_u(C+4*uvlinesize, uvlinesize, argu);/*i*8,
                                       clip_bot,
                                       clip_cur,
                                       alpha, beta, betaC, 1, 0);*/
                }
                if((c_v_deblock[0] & (MASK_CUR << ij)) && (i || !(mb_strong[POS_CUR] || mb_strong[POS_LEFT]))){
                    if(!i)
                    {
                        //clip_left = uvcbp[POS_LEFT][k] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;
                        argu[e_lim_p1]=uvcbp[POS_LEFT][0] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;
                    }
                    else
                    {
                        //clip_left = c_to_deblock[k]    & (MASK_CUR << (ij-1))  ? clip[POS_CUR]  : 0;
                        argu[e_lim_p1]=c_to_deblock[0]    & (MASK_CUR << (ij-1))  ? clip[POS_CUR]  : 0;
                    }
                    argu[e_dmode]=j*8;
                    _rv40_v_loop_filter_neon_u(C, uvlinesize,argu);/* j*8,
                                       clip_cur,
                                       clip_left,
                                       alpha, beta, betaC, 1, 0);*/
                }
                argu[e_edge]=1;
                if(!j && c_h_deblock[0] & (MASK_CUR << ij) && (mb_strong[POS_CUR] || mb_strong[POS_TOP])){
                    //int clip_top = uvcbp[POS_TOP][k] & (MASK_CUR << (ij+2)) ? clip[POS_TOP] : 0;
                    argu[e_lim_p1]=uvcbp[POS_TOP][0] & (MASK_CUR << (ij+2)) ? clip[POS_TOP] : 0;
                    _rv40_h_loop_filter_neon_u(C, uvlinesize,argu);/* i*8,
                                       clip_cur,
                                       clip_top,
                                       alpha, beta, betaC, 1, 1);*/
                }
                if(c_v_deblock[0] & (MASK_CUR << ij) && !i && (mb_strong[POS_CUR] || mb_strong[POS_LEFT])){
                    //clip_left = uvcbp[POS_LEFT][k] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;
                    argu[e_lim_p1]=uvcbp[POS_LEFT][0] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;
                    _rv40_v_loop_filter_neon_u(C, uvlinesize,argu);/* j*8,
                                       clip_cur,
                                       clip_left,
                                       alpha, beta, betaC, 1, 1);*/
                }
            }
        }

        for(j = 0; j < 2; j++){
            C = pic_data->current_picture_ptr->data[1] + mb_x*16 + (row*8 + j*4) * uvlinesize;
            for(i = 0; i < 2; i++, C += 8){
                int ij = i + j*2;
                //int clip_cur = c_to_deblock[k] & (MASK_CUR << ij) ? clip[POS_CUR] : 0;
                argu[e_lim_q1]=c_to_deblock[1] & (MASK_CUR << ij) ? clip[POS_CUR] : 0;
                argu[e_edge]=0;
                if(c_h_deblock[1] & (MASK_CUR << (ij+2))){
                    //int clip_bot = c_to_deblock[k] & (MASK_CUR << (ij+2)) ? clip[POS_CUR] : 0;
                    argu[e_lim_p1]=c_to_deblock[1] & (MASK_CUR << (ij+2)) ? clip[POS_CUR] : 0;
                    argu[e_dmode]=i*8;
                    _rv40_h_loop_filter_neon_v(C+4*uvlinesize, uvlinesize, argu);/*i*8,
                                       clip_bot,
                                       clip_cur,
                                       alpha, beta, betaC, 1, 0);*/
                }
                if((c_v_deblock[1] & (MASK_CUR << ij)) && (i || !(mb_strong[POS_CUR] || mb_strong[POS_LEFT]))){
                    if(!i)
                    {
                        //clip_left = uvcbp[POS_LEFT][k] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;
                        argu[e_lim_p1]=uvcbp[POS_LEFT][1] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;
                    }
                    else
                    {
                        //clip_left = c_to_deblock[k]    & (MASK_CUR << (ij-1))  ? clip[POS_CUR]  : 0;
                        argu[e_lim_p1]=c_to_deblock[1]    & (MASK_CUR << (ij-1))  ? clip[POS_CUR]  : 0;
                    }
                    argu[e_dmode]=j*8;
                    _rv40_v_loop_filter_neon_v(C, uvlinesize,argu);/* j*8,
                                       clip_cur,
                                       clip_left,
                                       alpha, beta, betaC, 1, 0);*/
                }
                argu[e_edge]=1;
                if(!j && c_h_deblock[1] & (MASK_CUR << ij) && (mb_strong[POS_CUR] || mb_strong[POS_TOP])){
                    //int clip_top = uvcbp[POS_TOP][k] & (MASK_CUR << (ij+2)) ? clip[POS_TOP] : 0;
                    argu[e_lim_p1]=uvcbp[POS_TOP][1] & (MASK_CUR << (ij+2)) ? clip[POS_TOP] : 0;
                    _rv40_h_loop_filter_neon_v(C, uvlinesize,argu);/* i*8,
                                       clip_cur,
                                       clip_top,
                                       alpha, beta, betaC, 1, 1);*/
                }
                if(c_v_deblock[1] & (MASK_CUR << ij) && !i && (mb_strong[POS_CUR] || mb_strong[POS_LEFT])){
                    //clip_left = uvcbp[POS_LEFT][k] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;
                    argu[e_lim_p1]=uvcbp[POS_LEFT][1] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;
                    _rv40_v_loop_filter_neon_v(C, uvlinesize,argu);/* j*8,
                                       clip_cur,
                                       clip_left,
                                       alpha, beta, betaC, 1, 1);*/
                }
            }
        }

    }
}


#endif


/**
 * RV40 loop filtering function
 */
#ifdef _d_new_parallel_
//#ifndef _config_rv40_neon_
void rv40new_amba_loop_filter(RV34AmbaDecContext *r,rv40_pic_data_t* pic_data, int row)
{
//    MpegEncContext *s = &r->s;
    int mb_pos, mb_x;
    int i, j, k;
    uint8_t *Y, *C;
    int alpha, beta, betaY, betaC;
    int q;
    int mbtype[4];   ///< current macroblock and its neighbours types
    /**
     * flags indicating that macroblock can be filtered with strong filter
     * it is set only for intra coded MB and MB with DCs coded separately
     */
    int mb_strong[4];
    int clip[4];     ///< MB filter clipping value calculated from filtering strength
    /**
     * coded block patterns for luma part of current macroblock and its neighbours
     * Format:
     * LSB corresponds to the top left block,
     * each nibble represents one row of subblocks.
     */
    int cbp[4];
    /**
     * coded block patterns for chroma part of current macroblock and its neighbours
     * Format is the same as for luma with two subblocks in a row.
     */
    int uvcbp[4][2];
    /**
     * This mask represents the pattern of luma subblocks that should be filtered
     * in addition to the coded ones because because they lie at the edge of
     * 8x8 block with different enough motion vectors
     */
    int mvmasks[4];
    int linesize=pic_data->current_picture_ptr->linesize[0];
    int uvlinesize=pic_data->current_picture_ptr->linesize[1];
    int pos;

    int argu[8];//{dmode,lim_q1, lim_p1,alpha, beta, beta2, chroma, edge}

    mb_pos = row * pic_data->mb_stride;
    for(mb_x = 0; mb_x < pic_data->mb_width; mb_x++, mb_pos++){
        i = pic_data->current_picture_ptr->mb_type[mb_pos];
        if(IS_INTRA(i) || IS_SEPARATE_DC(i))
            pic_data->cbp_luma  [mb_pos] = pic_data->deblock_coefs[mb_pos] = 0xFFFF;
        if(IS_INTRA(i))
            pic_data->cbp_chroma[mb_pos] = 0xFF;
    }
    mb_pos = row * pic_data->mb_stride;

#ifdef __dump_deblock_DSP_TXT__
    char txtch[160];
    int log_index;
    char* pctmp;
    snprintf(txtch,159,"parameters for row %d, mb_width %d, height %d, stride %d:", row, pic_data->mb_width, pic_data->mb_height, pic_data->mb_stride);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
    snprintf(txtch,159,"     linesize %d, uvlinesize %d", linesize, uvlinesize);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);

    for(mb_x = 0; mb_x < pic_data->mb_width; mb_x++, mb_pos++) {
        snprintf(txtch,159,"     mb %d, deblock_coefs 0x%x, mb_types 0x%x", mb_x, pic_data->deblock_coefs[mb_pos], pic_data->current_picture_ptr->mb_type[mb_pos]);
        log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
        snprintf(txtch,159,"     cbp_luma 0x%x, cbp_chroma 0x%x, qscale_table 0x%x", pic_data->cbp_luma[mb_pos], pic_data->cbp_chroma[mb_pos], pic_data->current_picture_ptr->qscale_table[mb_pos]);
        log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
    }
    mb_pos = row * pic_data->mb_stride;
#endif

    for(mb_x = 0; mb_x < pic_data->mb_width; mb_x++, mb_pos++){
        int y_h_deblock, y_v_deblock;
        int c_v_deblock[2], c_h_deblock[2];
        int clip_left;
        int avail[4];
        int y_to_deblock, c_to_deblock[2];

        q = pic_data->current_picture_ptr->qscale_table[mb_pos];
        alpha = rv40_alpha_tab[q];
        beta  = rv40_beta_tab [q];
        betaY = betaC = beta * 3;
        if(pic_data->width * pic_data->height <= 176*144)
            betaY += beta;

        avail[0] = 1;
        avail[1] = row;
        avail[2] = mb_x;
        avail[3] = row < pic_data->mb_height - 1;
        for(i = 0; i < 4; i++){
            if(avail[i]){
                pos = mb_pos + neighbour_offs_x[i] + neighbour_offs_y[i]*pic_data->mb_stride;
                mvmasks[i] = pic_data->deblock_coefs[pos];
                mbtype [i] = pic_data->current_picture_ptr->mb_type[pos];
                cbp    [i] = pic_data->cbp_luma[pos];
                uvcbp[i][0] = pic_data->cbp_chroma[pos] & 0xF;
                uvcbp[i][1] = pic_data->cbp_chroma[pos] >> 4;
            }else{
                mvmasks[i] = 0;
                mbtype [i] = mbtype[0];
                cbp    [i] = 0;
                uvcbp[i][0] = uvcbp[i][1] = 0;
            }
            mb_strong[i] = IS_INTRA(mbtype[i]) || IS_SEPARATE_DC(mbtype[i]);
            clip[i] = rv40_filter_clip_tbl[mb_strong[i] + 1][q];
        }

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"detailed deblock info MB[%d, %d]:", row, mb_x);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);

    for(log_index = 0; log_index < 4; log_index++) {
        snprintf(txtch,159,"  [%d]mvmasks 0x%x, mbtype 0x%x, cbp 0x%x ", log_index, mvmasks[log_index], mbtype [log_index], cbp[log_index]);
        log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
        snprintf(txtch,159,"     uvcbp[0] 0x%x, uvcbp[1] 0x%x", uvcbp[log_index][0], uvcbp[log_index][1]);
        log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
        snprintf(txtch,159,"     mb_strong 0x%x, clip 0x%x", mb_strong[log_index], clip[log_index]);
        log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
    }
#endif

        y_to_deblock =  mvmasks[POS_CUR]
                     | (mvmasks[POS_BOTTOM] << 16);
        /* This pattern contains bits signalling that horizontal edges of
         * the current block can be filtered.
         * That happens when either of adjacent subblocks is coded or lies on
         * the edge of 8x8 blocks with motion vectors differing by more than
         * 3/4 pel in any component (any edge orientation for some reason).
         */
        y_h_deblock =   y_to_deblock
                    | ((cbp[POS_CUR]                           <<  4) & ~MASK_Y_TOP_ROW)
                    | ((cbp[POS_TOP]        & MASK_Y_LAST_ROW) >> 12);
        /* This pattern contains bits signalling that vertical edges of
         * the current block can be filtered.
         * That happens when either of adjacent subblocks is coded or lies on
         * the edge of 8x8 blocks with motion vectors differing by more than
         * 3/4 pel in any component (any edge orientation for some reason).
         */
        y_v_deblock =   y_to_deblock
                    | ((cbp[POS_CUR]                      << 1) & ~MASK_Y_LEFT_COL)
                    | ((cbp[POS_LEFT] & MASK_Y_RIGHT_COL) >> 3);
        if(!mb_x)
            y_v_deblock &= ~MASK_Y_LEFT_COL;
        if(!row)
            y_h_deblock &= ~MASK_Y_TOP_ROW;
        if(row == pic_data->mb_height - 1 || (mb_strong[POS_CUR] || mb_strong[POS_BOTTOM]))
            y_h_deblock &= ~(MASK_Y_TOP_ROW << 16);
        /* Calculating chroma patterns is similar and easier since there is
         * no motion vector pattern for them.
         */
        for(i = 0; i < 2; i++){
            c_to_deblock[i] = (uvcbp[POS_BOTTOM][i] << 4) | uvcbp[POS_CUR][i];
            c_v_deblock[i] =   c_to_deblock[i]
                           | ((uvcbp[POS_CUR] [i]                       << 1) & ~MASK_C_LEFT_COL)
                           | ((uvcbp[POS_LEFT][i]   & MASK_C_RIGHT_COL) >> 1);
            c_h_deblock[i] =   c_to_deblock[i]
                           | ((uvcbp[POS_TOP][i]    & MASK_C_LAST_ROW)  >> 2)
                           |  (uvcbp[POS_CUR][i]                        << 2);
            if(!mb_x)
                c_v_deblock[i] &= ~MASK_C_LEFT_COL;
            if(!row)
                c_h_deblock[i] &= ~MASK_C_TOP_ROW;
            if(row == pic_data->mb_height - 1 || mb_strong[POS_CUR] || mb_strong[POS_BOTTOM])
                c_h_deblock[i] &= ~(MASK_C_TOP_ROW << 4);
        }

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"detailed 2 deblock info MB[%d, %d]:", row, mb_x);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);

    snprintf(txtch,159,"    y_h_deblock 0x%x, y_v_deblock 0x%x, y_to_deblock 0x%x:", y_h_deblock, y_v_deblock, y_to_deblock);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);

    for(log_index = 0; log_index < 2; log_index++) {
        snprintf(txtch,159,"    c_h_deblock 0x%x, c_v_deblock 0x%x, c_to_deblock 0x%x:", c_h_deblock[log_index], c_v_deblock[log_index], c_to_deblock[log_index]);
        log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
    }

    pctmp = pic_data->current_picture_ptr->data[0] + mb_x*16 + (row*16) * linesize;
    log_text_p(log_fd_dsp_deblock_text,"Before deblock, Y:", pic_data->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_deblock_text, pctmp, 16, 16, linesize-16, pic_data->frame_cnt);

    pctmp = pic_data->current_picture_ptr->data[1] + mb_x*16 + (row*8) * uvlinesize;
    log_text_p(log_fd_dsp_deblock_text,"Before deblock, U:", pic_data->frame_cnt);
    log_text_rect_char_hex_chroma_p(log_fd_dsp_deblock_text,pctmp, 8, 8, uvlinesize -16, pic_data->frame_cnt);
    log_text_p(log_fd_dsp_deblock_text,"Before deblock, V:", pic_data->frame_cnt);
    log_text_rect_char_hex_chroma_p(log_fd_dsp_deblock_text,pctmp + 1, 8, 8, uvlinesize -16, pic_data->frame_cnt);
#endif

        for(j = 0; j < 16; j += 4){
            Y = pic_data->current_picture_ptr->data[0] + mb_x*16 + (row*16 + j) * linesize;
            for(i = 0; i < 4; i++, Y += 4){
                int ij = i + j;
                int clip_cur = y_to_deblock & (MASK_CUR << ij) ? clip[POS_CUR] : 0;
                int dither = j ? ij : i*4;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    Y block[ij %d], offset %d:", ij, Y - pic_data->current_picture_ptr->data[0]);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
    snprintf(txtch,159,"    clip_cur 0x%x, dither 0x%x:", clip_cur, dither);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
    snprintf(txtch,159,"    alpha %d, beta %d, betaY %d:", alpha, beta, betaY);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
#endif

                // if bottom block is coded then we can filter its top edge
                // (or bottom edge of this block, which is the same)
                if(y_h_deblock & (MASK_BOTTOM << ij)){
                    if(r->neon.deblock[h_loop_filter_y] != NULL){
                        argu[0] = dither;
                        argu[1] = y_to_deblock & (MASK_BOTTOM << ij) ? clip[POS_CUR] : 0;
                        argu[2] = clip_cur;
                        argu[3] = alpha;
                        argu[4] = beta;
                        argu[5] = betaY;
                        argu[6] = 0;
                        argu[7] = 0;
                        r->neon.deblock[h_loop_filter_y](Y+4*linesize, linesize, argu);
                    }else{
                        rv40_h_loop_filter_y(Y+4*linesize, linesize, dither,
                                       y_to_deblock & (MASK_BOTTOM << ij) ? clip[POS_CUR] : 0,
                                       clip_cur,
                                       alpha, beta, betaY, 0, 0);
                    }

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    MASK_BOTTOM:");
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
#endif

                }
                // filter left block edge in ordinary mode (with low filtering strength)
                if((y_v_deblock & (MASK_CUR << ij)) && (i || !(mb_strong[POS_CUR] || mb_strong[POS_LEFT]))){
                    if(!i)
                        clip_left = mvmasks[POS_LEFT] & (MASK_RIGHT << j) ? clip[POS_LEFT] : 0;
                    else
                        clip_left = y_to_deblock & (MASK_CUR << (ij-1)) ? clip[POS_CUR] : 0;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    MASK_CUR, clip_left 0x%x:", clip_left);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
#endif
                    if(r->neon.deblock[v_loop_filter_y] != NULL){
                        argu[0] = dither;
                        argu[1] = clip_cur;
                        argu[2] = clip_left;
                        argu[3] = alpha;
                        argu[4] = beta;
                        argu[5] = betaY;
                        argu[6] = 0;
                        argu[7] = 0;
                        r->neon.deblock[v_loop_filter_y](Y, linesize, argu);
                    }else{
                        rv40_v_loop_filter_y(Y, linesize, dither,
                                       clip_cur,
                                       clip_left,
                                       alpha, beta, betaY, 0, 0);
                    }
                }
                // filter top edge of the current macroblock when filtering strength is high
                if(!j && (y_h_deblock & (MASK_CUR << i)) && (mb_strong[POS_CUR] || mb_strong[POS_TOP])){

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    MASK_TOP:");
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
#endif
                    if(r->neon.deblock[h_loop_filter_y] != NULL){
                        argu[0] = dither;
                        argu[1] = clip_cur;
                        argu[2] = mvmasks[POS_TOP] & (MASK_TOP << i) ? clip[POS_TOP] : 0;
                        argu[3] = alpha;
                        argu[4] = beta;
                        argu[5] = betaY;
                        argu[6] = 0;
                        argu[7] = 1;
                        r->neon.deblock[h_loop_filter_y](Y, linesize, argu);
                    }else{
                        rv40_h_loop_filter_y(Y, linesize, dither,
                                       clip_cur,
                                       mvmasks[POS_TOP] & (MASK_TOP << i) ? clip[POS_TOP] : 0,
                                       alpha, beta, betaY, 0, 1);
                	}
                }
                // filter left block edge in edge mode (with high filtering strength)
                if((y_v_deblock & (MASK_CUR << ij)) && !i && (mb_strong[POS_CUR] || mb_strong[POS_LEFT])){
                    clip_left = mvmasks[POS_LEFT] & (MASK_RIGHT << j) ? clip[POS_LEFT] : 0;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    MASK_CUR 2, clip_left 0x%x:", clip_left);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
#endif
                    if(r->neon.deblock[v_loop_filter_y] != NULL){
                        argu[0] = dither;
                        argu[1] = clip_cur;
                        argu[2] = clip_left;
                        argu[3] = alpha;
                        argu[4] = beta;
                        argu[5] = betaY;
                        argu[6] = 0;
                        argu[7] = 1;
                        r->neon.deblock[v_loop_filter_y](Y, linesize, argu);
                    }else{
                        rv40_v_loop_filter_y(Y, linesize, dither,
                                       clip_cur,
                                       clip_left,
                                       alpha, beta, betaY, 0, 1);
                    }
                }
            }
        }
        for(k = 0; k < 2; k++){
            for(j = 0; j < 2; j++){
                C = pic_data->current_picture_ptr->data[k+1] + mb_x*16 + (row*8 + j*4) * uvlinesize;
                for(i = 0; i < 2; i++, C += 8){
                    int ij = i + j*2;
                    int clip_cur = c_to_deblock[k] & (MASK_CUR << ij) ? clip[POS_CUR] : 0;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    C block[ij %d], offset %d:", ij, C - pic_data->current_picture_ptr->data[k+1]);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
    snprintf(txtch,159,"    clip_cur 0x%x", clip_cur);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
    snprintf(txtch,159,"    alpha %d, beta %d, betaC %d:", alpha, beta, betaC);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
#endif

                    if(c_h_deblock[k] & (MASK_CUR << (ij+2))){
                        int clip_bot = c_to_deblock[k] & (MASK_CUR << (ij+2)) ? clip[POS_CUR] : 0;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    C 1, clip_bot 0x%x:", clip_bot);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
#endif
                        if(r->neon.deblock[h_loop_filter_uv] != NULL){
                            argu[0] = i*8;
                            argu[1] = clip_bot;
                            argu[2] = clip_cur;
                            argu[3] = alpha;
                            argu[4] = beta;
                            argu[5] = betaC;
                            argu[6] = 1;
                            argu[7] = 0;
                            r->neon.deblock[h_loop_filter_uv](C+4*uvlinesize, uvlinesize, argu);
                        }else{
                            rv40_h_loop_filter_uv(C+4*uvlinesize, uvlinesize, i*8,
                                           clip_bot,
                                           clip_cur,
                                           alpha, beta, betaC, 1, 0);
                        }
                    }
                    if((c_v_deblock[k] & (MASK_CUR << ij)) && (i || !(mb_strong[POS_CUR] || mb_strong[POS_LEFT]))){
                        if(!i)
                            clip_left = uvcbp[POS_LEFT][k] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;
                        else
                            clip_left = c_to_deblock[k]    & (MASK_CUR << (ij-1))  ? clip[POS_CUR]  : 0;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    C 2, clip_left 0x%x:", clip_left);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
#endif
                        if(r->neon.deblock[v_loop_filter_uv] != NULL){
                            argu[0] = j*8;
                            argu[1] = clip_cur;
                            argu[2] = clip_left;
                            argu[3] = alpha;
                            argu[4] = beta;
                            argu[5] = betaC;
                            argu[6] = 1;
                            argu[7] = 0;
                            r->neon.deblock[v_loop_filter_uv](C, uvlinesize, argu);
                        }else{
                            rv40_v_loop_filter_uv(C, uvlinesize, j*8,
                                           clip_cur,
                                           clip_left,
                                           alpha, beta, betaC, 1, 0);
                        }
                    }
                    if(!j && (c_h_deblock[k] & (MASK_CUR << ij)) && (mb_strong[POS_CUR] || mb_strong[POS_TOP])){
                        int clip_top = uvcbp[POS_TOP][k] & (MASK_CUR << (ij+2)) ? clip[POS_TOP] : 0;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    C 3, clip_top 0x%x:", clip_top);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
#endif
                        if(r->neon.deblock[h_loop_filter_uv] != NULL){
                            argu[0] = i*8;
                            argu[1] = clip_cur;
                            argu[2] = clip_top;
                            argu[3] = alpha;
                            argu[4] = beta;
                            argu[5] = betaC;
                            argu[6] = 1;
                            argu[7] = 1;
                            r->neon.deblock[h_loop_filter_uv](C, uvlinesize, argu);
                        }else{
                            rv40_h_loop_filter_uv(C, uvlinesize, i*8,
                                           clip_cur,
                                           clip_top,
                                           alpha, beta, betaC, 1, 1);
                        }
                    }
                    if((c_v_deblock[k] & (MASK_CUR << ij)) && !i && (mb_strong[POS_CUR] || mb_strong[POS_LEFT])){
                        clip_left = uvcbp[POS_LEFT][k] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    C 4, clip_left 0x%x:", clip_left);
    log_text_p(log_fd_dsp_deblock_text,txtch, pic_data->frame_cnt);
#endif
                        if(r->neon.deblock[v_loop_filter_uv] != NULL){
                            argu[0] = j*8;
                            argu[1] = clip_cur;
                            argu[2] = clip_left;
                            argu[3] = alpha;
                            argu[4] = beta;
                            argu[5] = betaC;
                            argu[6] = 1;
                            argu[7] = 1;
                            r->neon.deblock[v_loop_filter_uv](C, uvlinesize, argu);
                        }else{
                            rv40_v_loop_filter_uv(C, uvlinesize, j*8,
                                           clip_cur,
                                           clip_left,
                                           alpha, beta, betaC, 1, 1);
                        }
                    }
                }
            }
        }

#ifdef __dump_deblock_DSP_TXT__
    pctmp = pic_data->current_picture_ptr->data[0] + mb_x*16 + (row*16) * linesize;
    log_text_p(log_fd_dsp_deblock_text,"After deblock, Y:", pic_data->frame_cnt);
    log_text_rect_char_hex_p(log_fd_dsp_deblock_text, pctmp, 16, 16, linesize-16, pic_data->frame_cnt);

    pctmp = pic_data->current_picture_ptr->data[1] + mb_x*16 + (row*8) * uvlinesize;
    log_text_p(log_fd_dsp_deblock_text,"After deblock, U:", pic_data->frame_cnt);
    log_text_rect_char_hex_chroma_p(log_fd_dsp_deblock_text,pctmp, 8, 8, uvlinesize -16, pic_data->frame_cnt);
    log_text_p(log_fd_dsp_deblock_text,"After deblock, V:", pic_data->frame_cnt);
    log_text_rect_char_hex_chroma_p(log_fd_dsp_deblock_text,pctmp + 1, 8, 8, uvlinesize -16, pic_data->frame_cnt);
#endif

    }
}
//#endif
#else
static void rv40_amba_loop_filter(RV34AmbaDecContext *r, int row)
{
    MpegEncContext *s = &r->s;
    int mb_pos, mb_x;
    int i, j, k;
    uint8_t *Y, *C;
    int alpha, beta, betaY, betaC;
    int q;
    int mbtype[4];   ///< current macroblock and its neighbours types
    /**
     * flags indicating that macroblock can be filtered with strong filter
     * it is set only for intra coded MB and MB with DCs coded separately
     */
    int mb_strong[4];
    int clip[4];     ///< MB filter clipping value calculated from filtering strength
    /**
     * coded block patterns for luma part of current macroblock and its neighbours
     * Format:
     * LSB corresponds to the top left block,
     * each nibble represents one row of subblocks.
     */
    int cbp[4];
    /**
     * coded block patterns for chroma part of current macroblock and its neighbours
     * Format is the same as for luma with two subblocks in a row.
     */
    int uvcbp[4][2];
    /**
     * This mask represents the pattern of luma subblocks that should be filtered
     * in addition to the coded ones because because they lie at the edge of
     * 8x8 block with different enough motion vectors
     */
    int mvmasks[4];

    mb_pos = row * s->mb_stride;
    for(mb_x = 0; mb_x < s->mb_width; mb_x++, mb_pos++){
        int mbtype = s->current_picture_ptr->mb_type[mb_pos];
        if(IS_INTRA(mbtype) || IS_SEPARATE_DC(mbtype))
            r->cbp_luma  [mb_pos] = r->deblock_coefs[mb_pos] = 0xFFFF;
        if(IS_INTRA(mbtype))
            r->cbp_chroma[mb_pos] = 0xFF;
    }
    mb_pos = row * s->mb_stride;
    for(mb_x = 0; mb_x < s->mb_width; mb_x++, mb_pos++){
        int y_h_deblock, y_v_deblock;
        int c_v_deblock[2], c_h_deblock[2];
        int clip_left;
        int avail[4];
        int y_to_deblock, c_to_deblock[2];

        q = s->current_picture_ptr->qscale_table[mb_pos];
        alpha = rv40_alpha_tab[q];
        beta  = rv40_beta_tab [q];
        betaY = betaC = beta * 3;
        if(s->width * s->height <= 176*144)
            betaY += beta;

        avail[0] = 1;
        avail[1] = row;
        avail[2] = mb_x;
        avail[3] = row < s->mb_height - 1;
        for(i = 0; i < 4; i++){
            if(avail[i]){
                int pos = mb_pos + neighbour_offs_x[i] + neighbour_offs_y[i]*s->mb_stride;
                mvmasks[i] = r->deblock_coefs[pos];
                mbtype [i] = s->current_picture_ptr->mb_type[pos];
                cbp    [i] = r->cbp_luma[pos];
                uvcbp[i][0] = r->cbp_chroma[pos] & 0xF;
                uvcbp[i][1] = r->cbp_chroma[pos] >> 4;
            }else{
                mvmasks[i] = 0;
                mbtype [i] = mbtype[0];
                cbp    [i] = 0;
                uvcbp[i][0] = uvcbp[i][1] = 0;
            }
            mb_strong[i] = IS_INTRA(mbtype[i]) || IS_SEPARATE_DC(mbtype[i]);
            clip[i] = rv40_filter_clip_tbl[mb_strong[i] + 1][q];
        }
        y_to_deblock =  mvmasks[POS_CUR]
                     | (mvmasks[POS_BOTTOM] << 16);
        /* This pattern contains bits signalling that horizontal edges of
         * the current block can be filtered.
         * That happens when either of adjacent subblocks is coded or lies on
         * the edge of 8x8 blocks with motion vectors differing by more than
         * 3/4 pel in any component (any edge orientation for some reason).
         */
        y_h_deblock =   y_to_deblock
                    | ((cbp[POS_CUR]                           <<  4) & ~MASK_Y_TOP_ROW)
                    | ((cbp[POS_TOP]        & MASK_Y_LAST_ROW) >> 12);
        /* This pattern contains bits signalling that vertical edges of
         * the current block can be filtered.
         * That happens when either of adjacent subblocks is coded or lies on
         * the edge of 8x8 blocks with motion vectors differing by more than
         * 3/4 pel in any component (any edge orientation for some reason).
         */
        y_v_deblock =   y_to_deblock
                    | ((cbp[POS_CUR]                      << 1) & ~MASK_Y_LEFT_COL)
                    | ((cbp[POS_LEFT] & MASK_Y_RIGHT_COL) >> 3);
        if(!mb_x)
            y_v_deblock &= ~MASK_Y_LEFT_COL;
        if(!row)
            y_h_deblock &= ~MASK_Y_TOP_ROW;
        if(row == s->mb_height - 1 || (mb_strong[POS_CUR] || mb_strong[POS_BOTTOM]))
            y_h_deblock &= ~(MASK_Y_TOP_ROW << 16);
        /* Calculating chroma patterns is similar and easier since there is
         * no motion vector pattern for them.
         */
        for(i = 0; i < 2; i++){
            c_to_deblock[i] = (uvcbp[POS_BOTTOM][i] << 4) | uvcbp[POS_CUR][i];
            c_v_deblock[i] =   c_to_deblock[i]
                           | ((uvcbp[POS_CUR] [i]                       << 1) & ~MASK_C_LEFT_COL)
                           | ((uvcbp[POS_LEFT][i]   & MASK_C_RIGHT_COL) >> 1);
            c_h_deblock[i] =   c_to_deblock[i]
                           | ((uvcbp[POS_TOP][i]    & MASK_C_LAST_ROW)  >> 2)
                           |  (uvcbp[POS_CUR][i]                        << 2);
            if(!mb_x)
                c_v_deblock[i] &= ~MASK_C_LEFT_COL;
            if(!row)
                c_h_deblock[i] &= ~MASK_C_TOP_ROW;
            if(row == s->mb_height - 1 || mb_strong[POS_CUR] || mb_strong[POS_BOTTOM])
                c_h_deblock[i] &= ~(MASK_C_TOP_ROW << 4);
        }

        for(j = 0; j < 16; j += 4){
            Y = s->current_picture_ptr->data[0] + mb_x*16 + (row*16 + j) * s->linesize;
            for(i = 0; i < 4; i++, Y += 4){
                int ij = i + j;
                int clip_cur = y_to_deblock & (MASK_CUR << ij) ? clip[POS_CUR] : 0;
                int dither = j ? ij : i*4;

                // if bottom block is coded then we can filter its top edge
                // (or bottom edge of this block, which is the same)
                if(y_h_deblock & (MASK_BOTTOM << ij)){
                    rv40_h_loop_filter_y(Y+4*s->linesize, s->linesize, dither,
                                       y_to_deblock & (MASK_BOTTOM << ij) ? clip[POS_CUR] : 0,
                                       clip_cur,
                                       alpha, beta, betaY, 0, 0);
                }
                // filter left block edge in ordinary mode (with low filtering strength)
                if(y_v_deblock & (MASK_CUR << ij) && (i || !(mb_strong[POS_CUR] || mb_strong[POS_LEFT]))){
                    if(!i)
                        clip_left = mvmasks[POS_LEFT] & (MASK_RIGHT << j) ? clip[POS_LEFT] : 0;
                    else
                        clip_left = y_to_deblock & (MASK_CUR << (ij-1)) ? clip[POS_CUR] : 0;
                    rv40_v_loop_filter_y(Y, s->linesize, dither,
                                       clip_cur,
                                       clip_left,
                                       alpha, beta, betaY, 0, 0);
                }
                // filter top edge of the current macroblock when filtering strength is high
                if(!j && y_h_deblock & (MASK_CUR << i) && (mb_strong[POS_CUR] || mb_strong[POS_TOP])){
                    rv40_h_loop_filter_y(Y, s->linesize, dither,
                                       clip_cur,
                                       mvmasks[POS_TOP] & (MASK_TOP << i) ? clip[POS_TOP] : 0,
                                       alpha, beta, betaY, 0, 1);
                }
                // filter left block edge in edge mode (with high filtering strength)
                if(y_v_deblock & (MASK_CUR << ij) && !i && (mb_strong[POS_CUR] || mb_strong[POS_LEFT])){
                    clip_left = mvmasks[POS_LEFT] & (MASK_RIGHT << j) ? clip[POS_LEFT] : 0;
                    rv40_v_loop_filter_y(Y, s->linesize, dither,
                                       clip_cur,
                                       clip_left,
                                       alpha, beta, betaY, 0, 1);
                }
            }
        }
        for(k = 0; k < 2; k++){
            for(j = 0; j < 2; j++){
                C = s->current_picture_ptr->data[k+1] + mb_x*16 + (row*8 + j*4) * s->uvlinesize;
                for(i = 0; i < 2; i++, C += 8){
                    int ij = i + j*2;
                    int clip_cur = c_to_deblock[k] & (MASK_CUR << ij) ? clip[POS_CUR] : 0;
                    if(c_h_deblock[k] & (MASK_CUR << (ij+2))){
                        int clip_bot = c_to_deblock[k] & (MASK_CUR << (ij+2)) ? clip[POS_CUR] : 0;
                        rv40_h_loop_filter_uv(C+4*s->uvlinesize, s->uvlinesize, i*8,
                                           clip_bot,
                                           clip_cur,
                                           alpha, beta, betaC, 1, 0);
                    }
                    if((c_v_deblock[k] & (MASK_CUR << ij)) && (i || !(mb_strong[POS_CUR] || mb_strong[POS_LEFT]))){
                        if(!i)
                            clip_left = uvcbp[POS_LEFT][k] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;
                        else
                            clip_left = c_to_deblock[k]    & (MASK_CUR << (ij-1))  ? clip[POS_CUR]  : 0;
                        rv40_v_loop_filter_uv(C, s->uvlinesize, j*8,
                                           clip_cur,
                                           clip_left,
                                           alpha, beta, betaC, 1, 0);
                    }
                    if(!j && c_h_deblock[k] & (MASK_CUR << ij) && (mb_strong[POS_CUR] || mb_strong[POS_TOP])){
                        int clip_top = uvcbp[POS_TOP][k] & (MASK_CUR << (ij+2)) ? clip[POS_TOP] : 0;
                        rv40_h_loop_filter_uv(C, s->uvlinesize, i*8,
                                           clip_cur,
                                           clip_top,
                                           alpha, beta, betaC, 1, 1);
                    }
                    if(c_v_deblock[k] & (MASK_CUR << ij) && !i && (mb_strong[POS_CUR] || mb_strong[POS_LEFT])){
                        clip_left = uvcbp[POS_LEFT][k] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;
                        rv40_v_loop_filter_uv(C, s->uvlinesize, j*8,
                                           clip_cur,
                                           clip_left,
                                           alpha, beta, betaC, 1, 1);
                    }
                }
            }
        }
    }
}
#endif
/**
 * Initialize decoder.
 */
#ifdef _d_new_parallel_
static av_cold int rv40new_amba_decode_init(AVCodecContext *avctx)
{
    RV34AmbaDecContext *r = avctx->priv_data;

    r->rv30 = 0;
    ffnew_rv34_amba_decode_init(avctx);
    if(!aic_top_vlc.bits)
        rv40_amba_init_tables();
    r->parse_slice_header = rv40_amba_parse_slice_header;
    r->decode_intra_types = rv40_amba_decode_intra_types;
    r->decode_mb_info     = rv40_amba_decode_mb_info;
    //#ifdef _config_rv40_neon_
    //r->loop_filter        = rv40new_neon_loop_filter;
    //#else
    r->loop_filter        = rv40new_amba_loop_filter;
    //#endif
    r->luma_dc_quant_i = rv40_luma_dc_quant[0];
    r->luma_dc_quant_p = rv40_luma_dc_quant[1];
    return 0;
}
#else
static av_cold int rv40_amba_decode_init(AVCodecContext *avctx)
{
    RV34AmbaDecContext *r = avctx->priv_data;

    r->rv30 = 0;
    ff_rv34_amba_decode_init(avctx);
    if(!aic_top_vlc.bits)
        rv40_amba_init_tables();
    r->parse_slice_header = rv40_amba_parse_slice_header;
    r->decode_intra_types = rv40_amba_decode_intra_types;
    r->decode_mb_info     = rv40_amba_decode_mb_info;
    r->loop_filter        = rv40_amba_loop_filter;
    r->luma_dc_quant_i = rv40_luma_dc_quant[0];
    r->luma_dc_quant_p = rv40_luma_dc_quant[1];
    return 0;
}
#endif

void flush_holded_pictures(RV34AmbaDecContext *thiz, rv40_pic_data_t* p_pic)
{
    if (p_pic->last_picture_ptr) {
        flush_pictrue(thiz, p_pic->last_picture_ptr);
        p_pic->last_picture_ptr = NULL;
    }

    if (p_pic->current_picture_ptr) {
        flush_pictrue(thiz, p_pic->current_picture_ptr);
        p_pic->current_picture_ptr = NULL;
    }

    if (p_pic->next_picture_ptr) {
        flush_pictrue(thiz, p_pic->next_picture_ptr);
        p_pic->next_picture_ptr = NULL;
    }
}

#ifdef _d_new_parallel_
static void amba_rv40_flush(AVCodecContext *avctx)
{
    //return;
    RV34AmbaDecContext* thiz=avctx->priv_data;
    MpegEncContext *s = &thiz->s;
    ctx_nodef_t* p_node;
    AVFrame *pic;
    int i = 0;
    av_log(NULL,AV_LOG_ERROR,"** Flush pipeline.\n");
    if (!thiz->p_vld_dataq) {
        return;
    }
    //set flush flag for each thread
    thiz->vld_need_flush = 1;
    thiz->idct_mc_need_flush = 1;
    thiz->deblock_need_flush = 1;

    av_log(NULL,AV_LOG_ERROR,"** Flush pipeline 1.\n");

#if 0
    av_log(NULL,AV_LOG_ERROR,"p_vld_dataq status.\n");
    ambadec_print_triqueue_general_status(thiz->p_vld_dataq);
    ambadec_print_triqueue_detailed_status(thiz->p_vld_dataq);

    av_log(NULL,AV_LOG_ERROR,"p_mcintrapred_dataq status.\n");
    ambadec_print_triqueue_general_status(thiz->p_mcintrapred_dataq);
    ambadec_print_triqueue_detailed_status(thiz->p_mcintrapred_dataq);

    av_log(NULL,AV_LOG_ERROR,"p_deblock_dataq status.\n");
    ambadec_print_triqueue_general_status(thiz->p_deblock_dataq);
    ambadec_print_triqueue_detailed_status(thiz->p_deblock_dataq);

    av_log(NULL,AV_LOG_ERROR,"p_pic_dataq status.\n");
    ambadec_print_triqueue_general_status(thiz->p_pic_dataq);
    ambadec_print_triqueue_detailed_status(thiz->p_pic_dataq);
#endif

    //send flush packet
    p_node = thiz->p_vld_dataq->get_cmd(thiz->p_vld_dataq);
    p_node->p_ctx = NULL;
    thiz->p_vld_dataq->put_ready(thiz->p_vld_dataq, p_node, _flag_cmd_flush_);
    av_log(NULL,AV_LOG_ERROR,"** Flush pipeline 2.\n");

    //prevent vld thread waiting p_pic
    p_node = thiz->p_pic_dataq->get_cmd(thiz->p_pic_dataq);
    p_node->p_ctx = NULL;
    thiz->p_pic_dataq->put_ready(thiz->p_pic_dataq, p_node, 0);

    //clear all remaining frames in frame pool, until flush is done
    while (1) {
        p_node=thiz->p_frame_pool->get(thiz->p_frame_pool);
        pic = (AVFrame*)p_node->p_ctx;
        if (pic && p_node->flag != _flag_cmd_flush_) {
            flush_pictrue(thiz, (Picture *)pic);
        } else if (p_node->flag == _flag_cmd_flush_){
            av_log(NULL,AV_LOG_ERROR," **exit from deblock p_node %p.\n", p_node);
            av_log(NULL,AV_LOG_ERROR,"** Flush pipeline done.\n");
            break;
        }
    }
    av_log(NULL,AV_LOG_ERROR,"** Flush pipeline 4.\n");
    //clear last/next/current picture, ensure all is flushed
    flush_holded_pictures(thiz, &thiz->picdata[0]);
    flush_holded_pictures(thiz, &thiz->picdata[1]);

    if (s->last_picture_ptr) {
        flush_pictrue(thiz, s->last_picture_ptr);
        s->last_picture_ptr = NULL;
    }
    if (s->current_picture_ptr) {
        flush_pictrue(thiz, s->current_picture_ptr);
        s->last_picture_ptr = NULL;
    }
    if (s->next_picture_ptr) {
        flush_pictrue(thiz, s->next_picture_ptr);
        s->next_picture_ptr = NULL;
    }

    //reset pic_data_q
    ambadec_reset_triqueue(thiz->p_pic_dataq);
    for(i=0;i<2;i++)
    {
        p_node=thiz->p_pic_dataq->get_free(thiz->p_pic_dataq);
        p_node->p_ctx=&thiz->picdata[i];
        thiz->p_pic_dataq->put_ready(thiz->p_pic_dataq,p_node,0);
    }
    thiz->vld_sent_row=thiz->mc_sent_row=0;
    thiz->pic_finished=1;
    thiz->previous_pic_is_b=1;

    //avcodec/avformat related
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
    thiz->decoding_frame_cnt = 0;

    thiz->vld_need_flush = 0;
    thiz->idct_mc_need_flush = 0;
    thiz->deblock_need_flush = 0;

    thiz->error_wait_flush = 0;
#if 0
    av_log(NULL,AV_LOG_ERROR,"flush done.\n");
    av_log(NULL,AV_LOG_ERROR,"p_vld_dataq status.\n");
    ambadec_print_triqueue_general_status(thiz->p_vld_dataq);
    ambadec_print_triqueue_detailed_status(thiz->p_vld_dataq);

    av_log(NULL,AV_LOG_ERROR,"p_mcintrapred_dataq status.\n");
    ambadec_print_triqueue_general_status(thiz->p_mcintrapred_dataq);
    ambadec_print_triqueue_detailed_status(thiz->p_mcintrapred_dataq);

    av_log(NULL,AV_LOG_ERROR,"p_deblock_dataq status.\n");
    ambadec_print_triqueue_general_status(thiz->p_deblock_dataq);
    ambadec_print_triqueue_detailed_status(thiz->p_deblock_dataq);

    av_log(NULL,AV_LOG_ERROR,"p_pic_dataq status.\n");
    ambadec_print_triqueue_general_status(thiz->p_pic_dataq);
    ambadec_print_triqueue_detailed_status(thiz->p_pic_dataq);
#endif
}
#endif

AVCodec ff_rv40_amba_decoder = {
    "rv40-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_RV40,
    sizeof(RV34AmbaDecContext),
#ifdef _d_new_parallel_
    rv40new_amba_decode_init,
    NULL,
    ffnew_rv34_amba_decode_end,
    ffnew_rv34_amba_decode_frame,
#else
    rv40_amba_decode_init,
    NULL,
    ff_rv34_amba_decode_end,
    ff_rv34_amba_decode_frame,
#endif
    CODEC_CAP_DR1 | CODEC_CAP_DELAY,
#ifdef _d_new_parallel_
    .flush = amba_rv40_flush,
#endif
    .long_name = NULL_IF_CONFIG_SMALL("amba(parallel) RealVideo 4.0 with nv12 output"),
    .pix_fmts= ff_pixfmt_list_amba,
};

