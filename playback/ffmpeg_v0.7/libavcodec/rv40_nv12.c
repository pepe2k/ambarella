/*
 * RV40 decoder
 * Copyright (c) 2007 Konstantin Shishkov
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
 * @file libavcodec/rv40.c
 * RV40 decoder
 */

#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "golomb.h"

#include "rv34_nv12.h"
#include "rv40vlc2.h"
#include "rv40data.h"
#include "log_dump.h"

#ifdef _config_rv40_neon_
#undef _config_rv40_neon_
#endif
/**
 * Initialize all tables.
 */
static av_cold void rv40_nv12_init_tables(void)
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

static int rv40_nv12_parse_slice_header(RV34DecContext *r, GetBitContext *gb, SliceInfo *si)
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
    mb_bits = ff_rv34_nv12_get_start_offset(gb, mb_size);
    si->start = get_bits(gb, mb_bits);

    return 0;
}

/**
 * Decode 4x4 intra types array.
 */
static int rv40_nv12_decode_intra_types(RV34DecContext *r, GetBitContext *gb, int8_t *dst)
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
static int rv40_nv12_decode_mb_info(RV34DecContext *r)
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

#ifndef _config_rv40_neon_
#define CLIP_SYMM(a, b) av_clip(a, -(b), b)
/**
 * weaker deblocking very similar to the one described in 4.4.2 of JVT-A003r1
 */
static inline void rv40_weak_loop_filter_nv12(uint8_t *src, const int step,
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

static inline void rv40_adaptive_loop_filter_nv12(uint8_t *src, const int step,
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
            rv40_weak_loop_filter_nv12(src, step, 1, 1, alpha, beta, lims, lim_q1, lim_p1,
                                  diff_p1p0[i], diff_q1q0[i], diff_p1p2[i], diff_q1q2[i]);
    }else{
        for(i = 0; i < 4; i++, src += stride)
            rv40_weak_loop_filter_nv12(src, step, filter_p1, filter_q1,
                                  alpha, beta, lims>>1, lim_q1>>1, lim_p1>>1,
                                  diff_p1p0[i], diff_q1q0[i], diff_p1p2[i], diff_q1q2[i]);
    }
}

static void rv40_v_loop_filter_y(uint8_t *src, int stride, int dmode,
                               int lim_q1, int lim_p1,
                               int alpha, int beta, int beta2, int chroma, int edge){
    rv40_adaptive_loop_filter_nv12(src, 1, stride, dmode, lim_q1, lim_p1,
                              alpha, beta, beta2, chroma, edge);
}
static void rv40_h_loop_filter_y(uint8_t *src, int stride, int dmode,
                               int lim_q1, int lim_p1,
                               int alpha, int beta, int beta2, int chroma, int edge){
    rv40_adaptive_loop_filter_nv12(src, stride, 1, dmode, lim_q1, lim_p1,
                              alpha, beta, beta2, chroma, edge);
}

static void rv40_v_loop_filter_uv(uint8_t *src, int stride, int dmode,
                               int lim_q1, int lim_p1,
                               int alpha, int beta, int beta2, int chroma, int edge){
    rv40_adaptive_loop_filter_nv12(src, 2, stride, dmode, lim_q1, lim_p1,
                              alpha, beta, beta2, chroma, edge);
}
static void rv40_h_loop_filter_uv(uint8_t *src, int stride, int dmode,
                               int lim_q1, int lim_p1,
                               int alpha, int beta, int beta2, int chroma, int edge){
    rv40_adaptive_loop_filter_nv12(src, stride, 2, dmode, lim_q1, lim_p1,
                              alpha, beta, beta2, chroma, edge);
}
#endif
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

/**
 * RV40 loop filtering function
 */
 #ifndef _config_rv40_neon_
static void rv40_nv12_loop_filter(RV34DecContext *r, int row)
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

#ifdef __dump_deblock_DSP_TXT__
    char txtch[160];
    int log_index;
    char* pctmp;
    snprintf(txtch,159,"parameters for row %d, mb_width %d, height %d, stride %d:", row, s->mb_width, s->mb_height, s->mb_stride);
    log_text(log_fd_dsp_deblock_text,txtch);
    snprintf(txtch,159,"     linesize %d, uvlinesize %d", s->linesize, s->uvlinesize);
    log_text(log_fd_dsp_deblock_text,txtch);

    for(mb_x = 0; mb_x < s->mb_width; mb_x++, mb_pos++) {
        snprintf(txtch,159,"     mb %d, deblock_coefs 0x%x, mb_types 0x%x", mb_x, r->deblock_coefs[mb_pos], s->current_picture_ptr->mb_type[mb_pos]);
        log_text(log_fd_dsp_deblock_text,txtch);
        snprintf(txtch,159,"     cbp_luma 0x%x, cbp_chroma 0x%x, qscale_table 0x%x", r->cbp_luma[mb_pos], r->cbp_chroma[mb_pos], s->current_picture_ptr->qscale_table[mb_pos]);
        log_text(log_fd_dsp_deblock_text,txtch);
    }
    mb_pos = row * s->mb_stride;
#endif

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

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"detailed deblock info MB[%d, %d]:", row, mb_x);
    log_text(log_fd_dsp_deblock_text,txtch);

    for(log_index = 0; log_index < 4; log_index++) {
        snprintf(txtch,159,"  [%d]mvmasks 0x%x, mbtype 0x%x, cbp 0x%x ", log_index, mvmasks[log_index], mbtype [log_index], cbp[log_index]);
        log_text(log_fd_dsp_deblock_text,txtch);
        snprintf(txtch,159,"     uvcbp[0] 0x%x, uvcbp[1] 0x%x", uvcbp[log_index][0], uvcbp[log_index][1]);
        log_text(log_fd_dsp_deblock_text,txtch);
        snprintf(txtch,159,"     mb_strong 0x%x, clip 0x%x", mb_strong[log_index], clip[log_index]);
        log_text(log_fd_dsp_deblock_text,txtch);
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

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"detailed 2 deblock info MB[%d, %d]:", row, mb_x);
    log_text(log_fd_dsp_deblock_text,txtch);

    snprintf(txtch,159,"    y_h_deblock 0x%x, y_v_deblock 0x%x, y_to_deblock 0x%x:", y_h_deblock, y_v_deblock, y_to_deblock);
    log_text(log_fd_dsp_deblock_text,txtch);

    for(log_index = 0; log_index < 2; log_index++) {
        snprintf(txtch,159,"    c_h_deblock 0x%x, c_v_deblock 0x%x, c_to_deblock 0x%x:", c_h_deblock[log_index], c_v_deblock[log_index], c_to_deblock[log_index]);
        log_text(log_fd_dsp_deblock_text,txtch);
    }

    pctmp = s->current_picture_ptr->data[0] + mb_x*16 + (row*16) * s->linesize;
    log_text(log_fd_dsp_deblock_text,"Before deblock, Y:");
    log_text_rect_char_hex(log_fd_dsp_deblock_text, pctmp, 16, 16, s->linesize-16);

    pctmp = s->current_picture_ptr->data[1] + mb_x*16 + (row*8) * s->uvlinesize;
    log_text(log_fd_dsp_deblock_text,"Before deblock, U:");
    log_text_rect_char_hex_chroma(log_fd_dsp_deblock_text,pctmp, 8, 8, s->uvlinesize -16);
    log_text(log_fd_dsp_deblock_text,"Before deblock, V:");
    log_text_rect_char_hex_chroma(log_fd_dsp_deblock_text,pctmp + 1, 8, 8, s->uvlinesize -16);
#endif

        for(j = 0; j < 16; j += 4){
            Y = s->current_picture_ptr->data[0] + mb_x*16 + (row*16 + j) * s->linesize;
            for(i = 0; i < 4; i++, Y += 4){
                int ij = i + j;
                int clip_cur = y_to_deblock & (MASK_CUR << ij) ? clip[POS_CUR] : 0;
                int dither = j ? ij : i*4;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    Y block[ij %d], offset %d:", ij, Y - s->current_picture_ptr->data[0]);
    log_text(log_fd_dsp_deblock_text,txtch);
    snprintf(txtch,159,"    clip_cur 0x%x, dither 0x%x:", clip_cur, dither);
    log_text(log_fd_dsp_deblock_text,txtch);
    snprintf(txtch,159,"    alpha %d, beta %d, betaY %d:", alpha, beta, betaY);
    log_text(log_fd_dsp_deblock_text,txtch);
#endif

                // if bottom block is coded then we can filter its top edge
                // (or bottom edge of this block, which is the same)
                if(y_h_deblock & (MASK_BOTTOM << ij)){

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    MASK_BOTTOM:");
    log_text(log_fd_dsp_deblock_text,txtch);
#endif

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

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    MASK_CUR, clip_left 0x%x:", clip_left);
    log_text(log_fd_dsp_deblock_text,txtch);
#endif

                    rv40_v_loop_filter_y(Y, s->linesize, dither,
                                       clip_cur,
                                       clip_left,
                                       alpha, beta, betaY, 0, 0);
                }
                // filter top edge of the current macroblock when filtering strength is high
                if(!j && y_h_deblock & (MASK_CUR << i) && (mb_strong[POS_CUR] || mb_strong[POS_TOP])){

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    MASK_TOP:");
    log_text(log_fd_dsp_deblock_text,txtch);
#endif

                    rv40_h_loop_filter_y(Y, s->linesize, dither,
                                       clip_cur,
                                       mvmasks[POS_TOP] & (MASK_TOP << i) ? clip[POS_TOP] : 0,
                                       alpha, beta, betaY, 0, 1);
                }
                // filter left block edge in edge mode (with high filtering strength)
                if(y_v_deblock & (MASK_CUR << ij) && !i && (mb_strong[POS_CUR] || mb_strong[POS_LEFT])){
                    clip_left = mvmasks[POS_LEFT] & (MASK_RIGHT << j) ? clip[POS_LEFT] : 0;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    MASK_CUR 2, clip_left 0x%x:", clip_left);
    log_text(log_fd_dsp_deblock_text,txtch);
#endif

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

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    C block[ij %d], offset %d:", ij, C - s->current_picture_ptr->data[k+1]);
    log_text(log_fd_dsp_deblock_text,txtch);
    snprintf(txtch,159,"    clip_cur 0x%x", clip_cur);
    log_text(log_fd_dsp_deblock_text,txtch);
    snprintf(txtch,159,"    alpha %d, beta %d, betaC %d:", alpha, beta, betaC);
    log_text(log_fd_dsp_deblock_text,txtch);
#endif

                    if(c_h_deblock[k] & (MASK_CUR << (ij+2))){
                        int clip_bot = c_to_deblock[k] & (MASK_CUR << (ij+2)) ? clip[POS_CUR] : 0;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    C 1, clip_bot 0x%x:", clip_bot);
    log_text(log_fd_dsp_deblock_text,txtch);
#endif

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

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    C 2, clip_left 0x%x:", clip_left);
    log_text(log_fd_dsp_deblock_text,txtch);
#endif

                        rv40_v_loop_filter_uv(C, s->uvlinesize, j*8,
                                           clip_cur,
                                           clip_left,
                                           alpha, beta, betaC, 1, 0);
                    }
                    if(!j && c_h_deblock[k] & (MASK_CUR << ij) && (mb_strong[POS_CUR] || mb_strong[POS_TOP])){
                        int clip_top = uvcbp[POS_TOP][k] & (MASK_CUR << (ij+2)) ? clip[POS_TOP] : 0;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    C 3, clip_top 0x%x:", clip_top);
    log_text(log_fd_dsp_deblock_text,txtch);
#endif

                        rv40_h_loop_filter_uv(C, s->uvlinesize, i*8,
                                           clip_cur,
                                           clip_top,
                                           alpha, beta, betaC, 1, 1);
                    }
                    if(c_v_deblock[k] & (MASK_CUR << ij) && !i && (mb_strong[POS_CUR] || mb_strong[POS_LEFT])){
                        clip_left = uvcbp[POS_LEFT][k] & (MASK_CUR << (2*j+1)) ? clip[POS_LEFT] : 0;

#ifdef __dump_deblock_DSP_TXT__
    snprintf(txtch,159,"    C 4, clip_left 0x%x:", clip_left);
    log_text(log_fd_dsp_deblock_text,txtch);
#endif

                        rv40_v_loop_filter_uv(C, s->uvlinesize, j*8,
                                           clip_cur,
                                           clip_left,
                                           alpha, beta, betaC, 1, 1);
                    }
                }
            }
        }
#ifdef __dump_deblock_DSP_TXT__
    pctmp = s->current_picture_ptr->data[0] + mb_x*16 + (row*16) * s->linesize;
    log_text(log_fd_dsp_deblock_text,"After deblock, Y:");
    log_text_rect_char_hex(log_fd_dsp_deblock_text, pctmp, 16, 16, s->linesize-16);

    pctmp = s->current_picture_ptr->data[1] + mb_x*16 + (row*8) * s->uvlinesize;
    log_text(log_fd_dsp_deblock_text,"After deblock, U:");
    log_text_rect_char_hex_chroma(log_fd_dsp_deblock_text,pctmp, 8, 8, s->uvlinesize -16);
    log_text(log_fd_dsp_deblock_text,"After deblock, V:");
    log_text_rect_char_hex_chroma(log_fd_dsp_deblock_text,pctmp + 1, 8, 8, s->uvlinesize -16);
#endif
    }
}
#else
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

void rv40_neon_loop_filter(RV34DecContext *r, int row)
{
    MpegEncContext *s = &r->s;
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
    int linesize=s->current_picture_ptr->linesize[0];
    int uvlinesize=s->current_picture_ptr->linesize[1];

    mb_pos = row * s->mb_stride;
    for(mb_x = 0; mb_x < s->mb_width; mb_x++, mb_pos++){
        i = s->current_picture_ptr->mb_type[mb_pos];
        if(IS_INTRA(i) || IS_SEPARATE_DC(i))
            r->cbp_luma  [mb_pos] = r->deblock_coefs[mb_pos] = 0xFFFF;
        if(IS_INTRA(i))
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
        //alpha = rv40_alpha_tab[q];
        argu[e_alpha]=rv40_alpha_tab[q];
        //beta  = rv40_beta_tab [q];
        argu[e_beta]=rv40_alpha_tab[q];
        //betaY = betaC = beta * 3;
        argu[e_beta2]=rv40_alpha_tab[q]*3;
        if(s->width * s->height <= 176*144)
            argu[e_beta2]+=argu[e_beta];
            //betaY += beta;

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
            Y = s->current_picture_ptr->data[0] + mb_x*16 + (row*16 + j) * linesize;
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
            C = s->current_picture_ptr->data[1] + mb_x*16 + (row*8 + j*4) * uvlinesize;
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
            C = s->current_picture_ptr->data[1] + mb_x*16 + (row*8 + j*4) * uvlinesize;
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
 * Initialize decoder.
 */
static av_cold int rv40_nv12_decode_init(AVCodecContext *avctx)
{
    RV34DecContext *r = avctx->priv_data;

    r->rv30 = 0;
    ff_rv34_nv12_decode_init(avctx);
    if(!aic_top_vlc.bits)
        rv40_nv12_init_tables();
    r->parse_slice_header = rv40_nv12_parse_slice_header;
    r->decode_intra_types = rv40_nv12_decode_intra_types;
    r->decode_mb_info     = rv40_nv12_decode_mb_info;
    #ifdef _config_rv40_neon_
    r->loop_filter        = rv40_neon_loop_filter;
    #else
    r->loop_filter        = rv40_nv12_loop_filter;
    #endif
    r->luma_dc_quant_i = rv40_luma_dc_quant[0];
    r->luma_dc_quant_p = rv40_luma_dc_quant[1];
    return 0;
}

AVCodec ff_rv40_nv12_decoder = {
    "rv40-nv12",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_RV40,
    sizeof(RV34DecContext),
    rv40_nv12_decode_init,
    NULL,
    ff_rv34_nv12_decode_end,
    ff_rv34_nv12_decode_frame,
    CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    .flush = ff_mpeg_flush,
    .long_name = NULL_IF_CONFIG_SMALL("RealVideo 4.0 with nv12 output"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

