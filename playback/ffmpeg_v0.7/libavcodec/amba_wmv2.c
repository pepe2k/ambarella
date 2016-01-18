/*
 * Copyright (c) 2002 The FFmpeg Project
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

#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "mathops.h"
#include "msmpeg4.h"
#include "msmpeg4data.h"
#include "intrax8.h"
#include "amba_dec_util.h"
#include "amba_dsp_define.h"
#include "amba_wmv2.h"
#include "log_dump.h"

//#define _dump_raw_data_
#ifdef _dump_raw_data_
static int dump_cnt=0;
static char dump_fn[40];
static char dump_fn_t[40]="raw_%d";
static FILE* dump_pf;
#endif

//log macros
//#define __log_parallel_control__
//#define __log_decoding_process__

extern int wmv2_decode_block_inter_dequant(MpegEncContext * s, DCTELEM * block, int n, int coded, const uint8_t *scan_table);
extern void ff_simple_idct84_wmv2(DCTELEM *block);
extern void ff_simple_idct84_wmv2_des(DCTELEM *src,DCTELEM *block);
extern void ff_simple_idct48_wmv2(DCTELEM *block);
extern void ff_simple_idct48_wmv2_des(DCTELEM *src,DCTELEM *block);
extern void wmv2_p_process_mc_idct_amba(Wmv2Context_amba *thiz, wmv2_pic_data_t* p_pic);
extern int ff_msmpeg4_decode_block_amba_intra_dequant(MpegEncContext * s, DCTELEM * block, int n, int coded);
extern void wmv2_i_process_idct_amba(Wmv2Context_amba *thiz, wmv2_pic_data_t* p_pic);

static void _wmv2_reset_picture_data(wmv2_pic_data_t* ppic)
{
//    av_log(NULL,AV_LOG_ERROR,"_reset_picture_data ppic->mb_width=%d,ppic->mb_height=%d.\n",ppic->mb_width,ppic->mb_height);
    //clear all data
    memset(ppic->pmvb,0x0,ppic->mb_width*ppic->mb_height*sizeof(mb_mv_B_t));
    memset(ppic->pdct,0x0,ppic->mb_width*ppic->mb_height*768);
}

static void _wmv2_callback_free_vld_data(void* p)
{
    h263_vld_data_t* pvld=(h263_vld_data_t*)p;
    ambadec_assert_ffmpeg(p);
    if(pvld->pbuf)
        av_free(pvld->pbuf);
    pvld->pbuf=NULL;
    av_free(p);
}

static void _wmv2_callback_NULL(void* p)
{
    return;
}

static void _wmv2_callback_destroy_pic_data(void* p)
{
    ambadec_assert_ffmpeg(p);
    wmv2_pic_data_t* ppic=(wmv2_pic_data_t*)p;

    if(ppic->pbase)
        av_free(ppic->pbase);
    if(ppic->pdct)
        av_free(ppic->pdctbase);
    if(ppic->pswinfo)
        av_free(ppic->pswinfo);

}

//port from other files

static inline int ff_h263_round_chroma_mv(int x){
    static const uint8_t h263_chroma_roundtab[16] = {
    //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
        0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1,
    };
    return h263_chroma_roundtab[x & 0xf] + (x >> 3);
}

static void parse_mb_skip(Wmv2Context_amba * thiz){
    int mb_x, mb_y;
    MpegEncContext * const s= &thiz->s;
    uint32_t * const mb_type= s->current_picture_ptr->mb_type;

    thiz->skip_type= get_bits(&s->gb, 2);
    switch(thiz->skip_type){
    case SKIP_TYPE_NONE:
        for(mb_y=0; mb_y<s->mb_height; mb_y++){
            for(mb_x=0; mb_x<s->mb_width; mb_x++){
                mb_type[mb_y*s->mb_stride + mb_x]= MB_TYPE_16x16 | MB_TYPE_L0;
            }
        }
        break;
    case SKIP_TYPE_MPEG:
        for(mb_y=0; mb_y<s->mb_height; mb_y++){
            for(mb_x=0; mb_x<s->mb_width; mb_x++){
                mb_type[mb_y*s->mb_stride + mb_x]= (get_bits1(&s->gb) ? MB_TYPE_SKIP : 0) | MB_TYPE_16x16 | MB_TYPE_L0;
            }
        }
        break;
    case SKIP_TYPE_ROW:
        for(mb_y=0; mb_y<s->mb_height; mb_y++){
            if(get_bits1(&s->gb)){
                for(mb_x=0; mb_x<s->mb_width; mb_x++){
                    mb_type[mb_y*s->mb_stride + mb_x]=  MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_L0;
                }
            }else{
                for(mb_x=0; mb_x<s->mb_width; mb_x++){
                    mb_type[mb_y*s->mb_stride + mb_x]= (get_bits1(&s->gb) ? MB_TYPE_SKIP : 0) | MB_TYPE_16x16 | MB_TYPE_L0;
                }
            }
        }
        break;
    case SKIP_TYPE_COL:
        for(mb_x=0; mb_x<s->mb_width; mb_x++){
            if(get_bits1(&s->gb)){
                for(mb_y=0; mb_y<s->mb_height; mb_y++){
                    mb_type[mb_y*s->mb_stride + mb_x]=  MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_L0;
                }
            }else{
                for(mb_y=0; mb_y<s->mb_height; mb_y++){
                    mb_type[mb_y*s->mb_stride + mb_x]= (get_bits1(&s->gb) ? MB_TYPE_SKIP : 0) | MB_TYPE_16x16 | MB_TYPE_L0;
                }
            }
        }
        break;
    }
}

av_cold void ff_wmv2_common_init_amba(Wmv2Context_amba * w){
    MpegEncContext * const s= &w->s;

    ff_init_scantable(s->dsp.idct_permutation, &w->abt_scantable[0], wmv2_scantableA);
    ff_init_scantable(s->dsp.idct_permutation, &w->abt_scantable[1], wmv2_scantableB);
}

static int decode_ext_header_amba(Wmv2Context_amba *thiz){
    MpegEncContext * const s= &thiz->s;
    GetBitContext gb;
    int fps;
    int code;

    if(s->avctx->extradata_size<4) return -1;

    init_get_bits(&gb, s->avctx->extradata, s->avctx->extradata_size*8);

    fps                = get_bits(&gb, 5);
    s->bit_rate        = get_bits(&gb, 11)*1024;
    thiz->mspel_bit       = get_bits1(&gb);
    s->loop_filter     = get_bits1(&gb);
    thiz->abt_flag        = get_bits1(&gb);
    thiz->j_type_bit      = get_bits1(&gb);
    thiz->top_left_mv_flag= get_bits1(&gb);
    thiz->per_mb_rl_bit   = get_bits1(&gb);
    code               = get_bits(&gb, 3);

    #ifdef __log_special_case__
    if(thiz->abt_flag)
    {
        av_log(NULL,AV_LOG_ERROR,"!!!!!attention: [abt transfrom] detected in wmv2 decoder.\n");
    }
    if(thiz->mspel_bit)
    {
        av_log(NULL,AV_LOG_ERROR,"!!!!!attention: [mspel mc] detected in wmv2 decoder.\n");
    }
    if(s->loop_filter)
    {
        av_log(NULL,AV_LOG_ERROR,"!!!!!attention: [in-loop-filter] detected in wmv2 decoder.\n");
    }
    if(thiz->j_type_bit)
    {
        av_log(NULL,AV_LOG_ERROR,"!!!!!attention: [j_type_bit] detected in wmv2 decoder.\n");
    }
    #endif

    if(code==0) return -1;

    s->slice_height = s->mb_height / code;

    if(s->avctx->debug&FF_DEBUG_PICT_INFO){
        av_log(s->avctx, AV_LOG_DEBUG, "fps:%d, br:%d, qpbit:%d, abt_flag:%d, j_type_bit:%d, tl_mv_flag:%d, mbrl_bit:%d, code:%d, loop_filter:%d, slices:%d\n",
        fps, s->bit_rate, thiz->mspel_bit, thiz->abt_flag, thiz->j_type_bit, thiz->top_left_mv_flag, thiz->per_mb_rl_bit, code, s->loop_filter,
        code);
    }
    return 0;
}

int ff_wmv2_decode_picture_header_amba(MpegEncContext * s)
{
    Wmv2Context_amba * const thiz= (Wmv2Context_amba*)s;
    int code;

    if(s->picture_number==0)
        decode_ext_header_amba(thiz);

    s->pict_type = get_bits1(&s->gb) + 1;
    if(s->pict_type == FF_I_TYPE){
        code = get_bits(&s->gb, 7);
        av_log(s->avctx, AV_LOG_DEBUG, "I7:%X/\n", code);
    }
    s->chroma_qscale= s->qscale = get_bits(&s->gb, 5);
    if(s->qscale <= 0)
       return -1;

    return 0;
}

int wmv2_decode_secondary_picture_header_amba(MpegEncContext * s)
{
    Wmv2Context_amba * const thiz= (Wmv2Context_amba*)s;

    if (s->pict_type == FF_I_TYPE) {
        if(thiz->j_type_bit) thiz->j_type= get_bits1(&s->gb);
        else              thiz->j_type= 0; //FIXME check

        if(!thiz->j_type){
            if(thiz->per_mb_rl_bit) s->per_mb_rl_table= get_bits1(&s->gb);
            else                 s->per_mb_rl_table= 0;

            if(!s->per_mb_rl_table){
                s->rl_chroma_table_index = decode012(&s->gb);
                s->rl_table_index = decode012(&s->gb);
            }

            s->dc_table_index = get_bits1(&s->gb);
        }
        s->inter_intra_pred= 0;
        s->no_rounding = 1;
        if(s->avctx->debug&FF_DEBUG_PICT_INFO){
            av_log(s->avctx, AV_LOG_DEBUG, "qscale:%d rlc:%d rl:%d dc:%d mbrl:%d j_type:%d \n",
                s->qscale,
                s->rl_chroma_table_index,
                s->rl_table_index,
                s->dc_table_index,
                s->per_mb_rl_table,
                thiz->j_type);
        }
    }else{
        int cbp_index;
        thiz->j_type=0;

        parse_mb_skip(thiz);
        cbp_index= decode012(&s->gb);
        if(s->qscale <= 10){
            int map[3]= {0,2,1};
            thiz->cbp_table_index= map[cbp_index];
        }else if(s->qscale <= 20){
            int map[3]= {1,0,2};
            thiz->cbp_table_index= map[cbp_index];
        }else{
            int map[3]= {2,1,0};
            thiz->cbp_table_index= map[cbp_index];
        }

        if(thiz->mspel_bit) s->mspel= get_bits1(&s->gb);
        else             s->mspel= 0; //FIXME check

        if(thiz->abt_flag){
            thiz->per_mb_abt= get_bits1(&s->gb)^1;
            if(!thiz->per_mb_abt){
                thiz->abt_type= decode012(&s->gb);
            }
        }

        if(thiz->per_mb_rl_bit) s->per_mb_rl_table= get_bits1(&s->gb);
        else                 s->per_mb_rl_table= 0;

        if(!s->per_mb_rl_table){
            s->rl_table_index = decode012(&s->gb);
            s->rl_chroma_table_index = s->rl_table_index;
        }

        s->dc_table_index = get_bits1(&s->gb);
        s->mv_table_index = get_bits1(&s->gb);

        s->inter_intra_pred= 0;//(s->width*s->height < 320*240 && s->bit_rate<=II_BITRATE);
        s->no_rounding ^= 1;

        if(s->avctx->debug&FF_DEBUG_PICT_INFO){
            av_log(s->avctx, AV_LOG_DEBUG, "rl:%d rlc:%d dc:%d mv:%d mbrl:%d qp:%d mspel:%d per_mb_abt:%d abt_type:%d cbp:%d ii:%d\n",
                s->rl_table_index,
                s->rl_chroma_table_index,
                s->dc_table_index,
                s->mv_table_index,
                s->per_mb_rl_table,
                s->qscale,
                s->mspel,
                thiz->per_mb_abt,
                thiz->abt_type,
                thiz->cbp_table_index,
                s->inter_intra_pred);
        }
    }
    s->esc3_level_length= 0;
    s->esc3_run_length= 0;

s->picture_number++; //FIXME ?


    if(thiz->j_type){
        ff_intrax8_decode_picture_amba(&thiz->x8, 2*s->qscale, (s->qscale-1)|1 );
        return 1;
    }

    return 0;
}

static inline int wmv2_decode_motion_amba(Wmv2Context_amba *thiz, int *mx_ptr, int *my_ptr){
    MpegEncContext * const s= &thiz->s;
    int ret;

    ret= ff_msmpeg4_decode_motion(s, mx_ptr, my_ptr);

    if(ret<0) return -1;

    if((((*mx_ptr)|(*my_ptr)) & 1) && s->mspel)
        thiz->hshift= get_bits1(&s->gb);
    else
        thiz->hshift= 0;

//printf("%d %d  ", *mx_ptr, *my_ptr);

    return 0;
}

static int16_t *wmv2_pred_motion_amba(Wmv2Context_amba *thiz, int *px, int *py){
    MpegEncContext * const s= &thiz->s;
    int xy, wrap, diff, type;
    int16_t *A, *B, *C, *mot_val;

    wrap = s->b8_stride;
    xy = s->block_index[0];

    mot_val = s->current_picture_ptr->motion_val[0][xy];

    A = s->current_picture_ptr->motion_val[0][xy - 1];
    B = s->current_picture_ptr->motion_val[0][xy - wrap];
    C = s->current_picture_ptr->motion_val[0][xy + 2 - wrap];

    if(s->mb_x && !s->first_slice_line && !s->mspel && thiz->top_left_mv_flag)
        diff= FFMAX(FFABS(A[0] - B[0]), FFABS(A[1] - B[1]));
    else
        diff=0;

    if(diff >= 8)
        type= get_bits1(&s->gb);
    else
        type= 2;

    if(type == 0){
        *px= A[0];
        *py= A[1];
    }else if(type == 1){
        *px= B[0];
        *py= B[1];
    }else{
        /* special case for first (slice) line */
        if (s->first_slice_line) {
            *px = A[0];
            *py = A[1];
        } else {
            *px = mid_pred(A[0], B[0], C[0]);
            *py = mid_pred(A[1], B[1], C[1]);
        }
    }

    return mot_val;
}

#ifdef __tmp_use_amba_permutation__
static inline int wmv2_decode_inter_block_amba_permutated(Wmv2Context_amba *thiz, DCTELEM *block, int n, int cbp){
    MpegEncContext * const s= &thiz->s;
//    static const int sub_cbp_table[3]= {2,3,1};
//    int sub_cbp;

    if(!cbp){
        s->block_last_index[n] = -1;

        return 0;
    }

    ambadec_assert_ffmpeg(!thiz->abt_flag);
    ambadec_assert_ffmpeg(!thiz->per_block_abt);
    ambadec_assert_ffmpeg(!thiz->abt_type);
    ambadec_assert_ffmpeg(thiz->use_permutated);

    //if(thiz->per_block_abt)
    //    thiz->abt_type= decode012(&s->gb);
#if 0
    if(thiz->per_block_abt)
        printf("B%d", thiz->abt_type);
#endif
    //thiz->abt_type_table[n]= thiz->abt_type;

//    if(!thiz->abt_type)
//    {
//        #ifdef __tmp_use_amba_permutation__
//        if(thiz->use_permutated)
//        {
            return ff_msmpeg4_decode_block(s, block, n, 1, s->p_inter_scantable->permutated);
//        }
//        else
//        #else
//            return ff_msmpeg4_decode_block(s, block, n, 1, s->inter_scantable.permutated);
//        #endif
//    }
    /*else
    {
        ambadec_assert_ffmpeg(0);
//        const uint8_t *scantable= thiz->abt_scantable[thiz->abt_type-1].permutated;
        const uint8_t *scantable= thiz->abt_scantable[thiz->abt_type-1].scantable;
//        const uint8_t *scantable= thiz->abt_type-1 ? thiz->abt_scantable[1].permutated : thiz->abt_scantable[0].scantable;

        sub_cbp= sub_cbp_table[ decode012(&s->gb) ];
//        printf("S%d", sub_cbp);

        if(sub_cbp&1){
            if (ff_msmpeg4_decode_block(s, block, n, 1, scantable) < 0)
                return -1;
        }

        if(sub_cbp&2){
            if (ff_msmpeg4_decode_block(s, thiz->abt_block2[n], n, 1, scantable) < 0)
                return -1;
        }
        s->block_last_index[n] = 63;

        return 0;
    }*/
}
#endif

//with IDCT
static inline int wmv2_decode_inter_block_idct_amba(Wmv2Context_amba *thiz, DCTELEM *block, int n, int cbp){
    MpegEncContext * const s= &thiz->s;
    static const int sub_cbp_table[3]= {2,3,1};
    int sub_cbp;

    if(!cbp){
        s->block_last_index[n] = -1;

        return 0;
    }

    ambadec_assert_ffmpeg(!thiz->use_permutated);

    if(thiz->per_block_abt)
        thiz->abt_type= decode012(&s->gb);
#if 0
    if(thiz->per_block_abt)
        printf("B%d", thiz->abt_type);
#endif
    //thiz->abt_type_table[n]= thiz->abt_type;

    if(thiz->abt_type){
//        const uint8_t *scantable= thiz->abt_scantable[thiz->abt_type-1].permutated;
        const uint8_t *scantable= thiz->abt_scantable[thiz->abt_type-1].scantable;
//        const uint8_t *scantable= thiz->abt_type-1 ? thiz->abt_scantable[1].permutated : thiz->abt_scantable[0].scantable;

        sub_cbp= sub_cbp_table[ decode012(&s->gb) ];
//        printf("S%d", sub_cbp);

        if(sub_cbp&1){
            if (wmv2_decode_block_inter_dequant(s, block, n, 1, scantable) < 0)
                return -1;
        }

        if(sub_cbp&2){
            if (wmv2_decode_block_inter_dequant(s, thiz->abt_block2[n], n, 1, scantable) < 0)
                return -1;
        }
        s->block_last_index[n] = 63;

        if(thiz->abt_type==1)
        {
            ff_simple_idct84_wmv2(block);
            ff_simple_idct84_wmv2_des(thiz->abt_block2[n],block+32);
            s->dsp.clear_block(thiz->abt_block2[n]);
        }
        else
        {
            ambadec_assert_ffmpeg(thiz->abt_type==2);
            ff_simple_idct48_wmv2(block);
            ff_simple_idct48_wmv2_des(thiz->abt_block2[n],block+4);
            s->dsp.clear_block(thiz->abt_block2[n]);
        }
        return 0;
    }else{
        wmv2_decode_block_inter_dequant(s, block, n, 1, s->inter_scantable.permutated);
        s->dsp.idct (block);
    }
    return 0;
}

//without idct, except abt_flag
static inline int wmv2_decode_inter_block_amba(Wmv2Context_amba *thiz, DCTELEM *block, int n, int cbp){
    MpegEncContext * const s= &thiz->s;

    if(!cbp){
        s->block_last_index[n] = -1;
        return 0;
    }

    ambadec_assert_ffmpeg(!thiz->use_permutated);
    ambadec_assert_ffmpeg(!thiz->abt_flag);
    ambadec_assert_ffmpeg(!thiz->per_block_abt);
    ambadec_assert_ffmpeg(!thiz->abt_type);

    wmv2_decode_block_inter_dequant(s, block, n, 1, s->inter_scantable.permutated);
    return 0;
}

static int wmv2_decode_mb_amba(MpegEncContext *s, DCTELEM block[6][64])
{
    Wmv2Context_amba * thiz= (Wmv2Context_amba*)s;
    int cbp, code, i;
    uint8_t *coded_val;
    h263_pic_swinfo_t* pinfo=thiz->p_pic->pswinfo+s->mb_x + s->mb_y*s->mb_width;

#ifdef __dump_DSP_TXT__
    char logt[80];
    log_mbtype log_mbtype;//0: intra, 1: skip 16x16, 2: skip
    int log_fielddct;
    snprintf(logt,79,">>>>>>>>>>>>>>>>>>>> [MB] = [%d %d] <<<<<<<<<<<<<<<<<<<<",s->mb_y,s->mb_x);
    log_text_p(log_fd_dsp_text,logt,log_cur_frame);
    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_vld,logt,log_cur_frame);
    #endif
#endif

    mb_mv_P_t* pmvp=thiz->p_pic->pmvp+(s->mb_width*s->mb_y+s->mb_x);
    mb_mv_B_t* pmvb=thiz->p_pic->pmvb+(s->mb_width*s->mb_y+s->mb_x);
    ambadec_assert_ffmpeg((void *)pmvp==(void *)pmvb);
    pmvp->mb_setting.slice_start=1;

    if(s->pict_type != FF_I_TYPE)
    {
        pmvp->mb_setting.frame_left=!s->mb_x;
        pmvp->mb_setting.frame_top=!s->mb_y;
    }

    if(thiz->j_type) return 0;

    if (s->pict_type == FF_P_TYPE) {
        if(IS_SKIP(s->current_picture_ptr->mb_type[s->mb_y * s->mb_stride + s->mb_x])){
            /* skip mb */
            s->mb_intra = 0;
            for(i=0;i<6;i++)
                s->block_last_index[i] = -1;
            s->mv_dir = MV_DIR_FORWARD;
            s->mv_type = MV_TYPE_16X16;
            //s->mv[0][0][0] = 0;
            //s->mv[0][0][1] = 0;
            //pmvp->mv[0].valid=1;
            pmvp->mv[0].ref_frame_num=forward_ref_num;
            s->mb_skipped = 1;
            thiz->hshift=0;
            goto end;
        }

        code = get_vlc2(&s->gb, ff_mb_non_intra_vlc[thiz->cbp_table_index].table, MB_NON_INTRA_VLC_BITS, 3);
        if (code < 0)
            return -1;
        s->mb_intra = (~code & 0x40) >> 6;

        cbp = code & 0x3f;
    } else {
        s->mb_intra = 1;
        code = get_vlc2(&s->gb, ff_msmp4_mb_i_vlc.table, MB_INTRA_VLC_BITS, 2);
        if (code < 0){
            av_log(s->avctx, AV_LOG_ERROR, "II-cbp illegal at %d %d\n", s->mb_x, s->mb_y);
            return -1;
        }
        /* predict coded block pattern */
        cbp = 0;
        for(i=0;i<6;i++) {
            int val = ((code >> (5 - i)) & 1);
            if (i < 4) {
                int pred = ff_msmpeg4_coded_block_pred(s, i, &coded_val);
                val = val ^ pred;
                *coded_val = val;
            }
            cbp |= val << (5 - i);
        }
    }

    if (!s->mb_intra) {
        int mx, my;
//printf("P at %d %d\n", s->mb_x, s->mb_y);
        wmv2_pred_motion_amba(thiz, &mx, &my);

        if(cbp){
            //s->dsp.clear_blocks(s->block[0]);
            if(s->per_mb_rl_table){
                s->rl_table_index = decode012(&s->gb);
                s->rl_chroma_table_index = s->rl_table_index;
            }

            if(thiz->abt_flag && thiz->per_mb_abt){
                thiz->per_block_abt= get_bits1(&s->gb);
                if(!thiz->per_block_abt)
                    thiz->abt_type= decode012(&s->gb);
            }else
                thiz->per_block_abt=0;
        }

        if (wmv2_decode_motion_amba(thiz, &mx, &my) < 0)
            return -1;

        s->mv_dir = MV_DIR_FORWARD;
        s->mv_type = MV_TYPE_16X16;
        //s->mv[0][0][0] = mx;
        //s->mv[0][0][1] = my;
        pmvp->mv[0].mv_x=mx;
        pmvp->mv[0].mv_y=my;
        pmvp->mv[0].ref_frame_num=forward_ref_num;

    #ifdef __tmp_use_amba_permutation__
        if(thiz->use_permutated)
        {
            //without idct, transposed for dsp, should not with abt_flag
            for (i = 0; i < 6; i++) {
                if (wmv2_decode_inter_block_amba_permutated(thiz, block[i], i, (cbp >> (5 - i)) & 1) < 0)
                {
                    av_log(s->avctx, AV_LOG_ERROR, "\nerror while decoding inter block: %d x %d (%d)\n", s->mb_x, s->mb_y, i);
                    return -1;
                }
            }
        }
        else
    #endif
        if(!thiz->abt_flag)
        {   //without idct
            for (i = 0; i < 6; i++) {
                if (wmv2_decode_inter_block_amba(thiz, block[i], i, (cbp >> (5 - i)) & 1) < 0)
                {
                    av_log(s->avctx, AV_LOG_ERROR, "\nerror while decoding inter block: %d x %d (%d)\n", s->mb_x, s->mb_y, i);
                    return -1;
                }
            }
        }
        else
        {
            //with idct
            for (i = 0; i < 6; i++) {
                if (wmv2_decode_inter_block_idct_amba(thiz, block[i], i, (cbp >> (5 - i)) & 1) < 0)
                {
                    av_log(s->avctx, AV_LOG_ERROR, "\nerror while decoding inter block: %d x %d (%d)\n", s->mb_x, s->mb_y, i);
                    return -1;
                }
            }
        }
    } else {
        pmvp->mb_setting.intra=1;
//if(s->pict_type==FF_P_TYPE)
//   printf("%d%d ", s->inter_intra_pred, cbp);
//printf("I at %d %d %d %06X\n", s->mb_x, s->mb_y, ((cbp&3)? 1 : 0) +((cbp&0x3C)? 2 : 0), show_bits(&s->gb, 24));
        ambadec_assert_ffmpeg(!s->inter_intra_pred);
        s->ac_pred = get_bits1(&s->gb);
        if(s->inter_intra_pred){
            s->h263_aic_dir= get_vlc2(&s->gb, ff_inter_intra_vlc.table, INTER_INTRA_VLC_BITS, 1);
//            printf("%d%d %d %d/", s->ac_pred, s->h263_aic_dir, s->mb_x, s->mb_y);
        }
        if(s->per_mb_rl_table && cbp){
            s->rl_table_index = decode012(&s->gb);
            s->rl_chroma_table_index = s->rl_table_index;
        }

        //s->dsp.clear_blocks(s->block[0]);
        for (i = 0; i < 6; i++) {
//            if (ff_msmpeg4_decode_block_amba_intra_dequant(s, block[i], i, (cbp >> (5 - i)) & 1, NULL) < 0)
            if (ff_msmpeg4_decode_block_amba_intra_dequant(s, block[i], i, (cbp >> (5 - i)) & 1) < 0)
            {
                av_log(s->avctx, AV_LOG_ERROR, "\nerror while decoding intra block: %d x %d (%d)\n", s->mb_x, s->mb_y, i);
                return -1;
            }
        }
    }

end:
    pinfo->block_last_index[0]=s->block_last_index[0];
    pinfo->block_last_index[1]=s->block_last_index[1];
    pinfo->block_last_index[2]=s->block_last_index[2];
    pinfo->block_last_index[3]=s->block_last_index[3];
    pinfo->block_last_index[4]=s->block_last_index[4];
    pinfo->block_last_index[5]=s->block_last_index[5];

    pinfo->qscale=s->qscale;
    pinfo->chroma_qscale=s->chroma_qscale;

    return 0;
}


static int wmv2_decode_slice_amba(MpegEncContext *s){
    const int part_mask= 0x7F;
    const int mb_size= 16>>s->avctx->lowres;
    Wmv2Context_amba* thiz=(Wmv2Context_amba*)s;
    short* pdct;
    mb_mv_P_t* pmvp;
    int xy;

    s->last_resync_gb= s->gb;
    s->first_slice_line= 1;

    s->resync_mb_x= s->mb_x;
    s->resync_mb_y= s->mb_y;

    ff_set_qscale(s, s->qscale);

//    av_log(NULL,AV_LOG_ERROR,"start decoding mb.\n");
    pmvp=thiz->p_pic->pmvp+s->mb_y*s->mb_width+s->mb_x;
    for(; s->mb_y < s->mb_height; s->mb_y++) {
        /* per-row end of slice checks */
        if(s->msmpeg4_version){
            if(s->resync_mb_y + s->slice_height == s->mb_y){
                ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);

                return 0;
            }
        }

        if(s->msmpeg4_version==1){
            s->last_dc[0]=
            s->last_dc[1]=
            s->last_dc[2]= 128;
        }

        ff_init_block_index_amba(s);

        for(; s->mb_x < s->mb_width; s->mb_x++) {
            int ret;
//            av_log(NULL,AV_LOG_ERROR,"start decoding mb x=%d,y=%d.\n",s->mb_x,s->mb_y);
            ff_update_block_index_amba(s);

            if(s->resync_mb_x == s->mb_x && s->resync_mb_y+1 == s->mb_y){
                s->first_slice_line=0;
            }

            /* DCT & quantize */
            pdct=thiz->p_pic->pdct+(s->mb_y*s->mb_width+s->mb_x)*6*64;
            s->mv_dir = MV_DIR_FORWARD;
            s->mv_type = MV_TYPE_16X16;
//            s->mb_skipped = 0;
//printf("%d %d %06X\n", ret, get_bits_count(&s->gb), show_bits(&s->gb, 24));
            ret=wmv2_decode_mb_amba(s,(DCTELEM (*)[64])pdct);
            //ret= s->decode_mb(s, pdct);

            xy= s->mb_x + s->mb_y*s->mb_stride;
            ambadec_assert_ffmpeg(thiz->p_pic->current_picture_ptr==s->current_picture_ptr);
            thiz->p_pic->current_picture_ptr->qscale_table[xy]= s->qscale;

//            if (s->pict_type!=FF_B_TYPE)
//                ff_h263_update_motion_val_amba(s);
            if (s->pict_type!=FF_B_TYPE)
                ff_h263_update_motion_val_amba_new(s,pmvp);

//            av_log(NULL,AV_LOG_ERROR,"after decoding mb ret%d.\n",ret);

            if(ret<0){

                if(ret==SLICE_END){
                    MPV_decode_mb_internal_amba(s);
                   // if(s->loop_filter)
                   //     ff_h263_loop_filter_amba(s);

//printf("%d %d %d %06X\n", s->mb_x, s->mb_y, s->gb.size*8 - get_bits_count(&s->gb), show_bits(&s->gb, 24));
                    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);

                    s->padding_bug_score--;

                    if(++s->mb_x >= s->mb_width){
                        s->mb_x=0;
                        ff_draw_horiz_band(s, s->mb_y*mb_size, mb_size);
                        s->mb_y++;
                    }

//                    av_log(NULL,AV_LOG_ERROR,"reach slice end.\n");

                    return 0;
                }else if(ret==SLICE_NOEND){
                    av_log(s->avctx, AV_LOG_ERROR, "Slice mismatch at MB: %d\n", xy);
                    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x+1, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);
                    return -1;
                }
                av_log(s->avctx, AV_LOG_ERROR, "Error at MB: %d\n", xy);
                ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_ERROR|DC_ERROR|MV_ERROR)&part_mask);

                return -1;
            }

            MPV_decode_mb_internal_amba(s);
//            if(s->loop_filter)
//                ff_h263_loop_filter_amba(s);
            pmvp++;
        }

        ff_draw_horiz_band(s, s->mb_y*mb_size, mb_size);

        s->mb_x= 0;
    }

    assert(s->mb_x==0 && s->mb_y==s->mb_height);

//    av_log(NULL,AV_LOG_ERROR,"end decode mb loop x=%d,y=%d.\n",s->mb_x,s->mb_y);

    //wmv2 try remove this
    /* try to detect the padding bug */
    /*if(      s->codec_id==CODEC_ID_AMBA_P_MPEG4
       &&   (s->workaround_bugs&FF_BUG_AUTODETECT)
       &&    s->gb.size_in_bits - get_bits_count(&s->gb) >=0
       &&    s->gb.size_in_bits - get_bits_count(&s->gb) < 48
//       &&   !s->resync_marker
       &&   !s->data_partitioning){

        const int bits_count= get_bits_count(&s->gb);
        const int bits_left = s->gb.size_in_bits - bits_count;

        if(bits_left==0){
            s->padding_bug_score+=16;
        } else if(bits_left != 1){
            int v= show_bits(&s->gb, 8);
            v|= 0x7F >> (7-(bits_count&7));

            if(v==0x7F && bits_left<=8)
                s->padding_bug_score--;
            else if(v==0x7F && ((get_bits_count(&s->gb)+8)&8) && bits_left<=16)
                s->padding_bug_score+= 4;
            else
                s->padding_bug_score++;
        }
//        av_log(NULL,AV_LOG_ERROR,"coming here in 1.\n");
    }*/

    if(s->workaround_bugs&FF_BUG_AUTODETECT){
        if(s->padding_bug_score > -2 && !s->data_partitioning /*&& (s->divx_version || !s->resync_marker)*/)
            s->workaround_bugs |=  FF_BUG_NO_PADDING;
        else
            s->workaround_bugs &= ~FF_BUG_NO_PADDING;
//        av_log(NULL,AV_LOG_ERROR,"coming here in 2.\n");
    }

    // handle formats which don't have unique end markers
    if(s->msmpeg4_version || (s->workaround_bugs&FF_BUG_NO_PADDING)){ //FIXME perhaps solve this more cleanly
        int left= s->gb.size_in_bits - get_bits_count(&s->gb);
        int max_extra=7;

        /* no markers in M$ crap */
        if(s->msmpeg4_version && s->pict_type==FF_I_TYPE)
            max_extra+= 17;

        /* buggy padding but the frame should still end approximately at the bitstream end */
        if((s->workaround_bugs&FF_BUG_NO_PADDING) && s->error_recognition>=3)
            max_extra+= 48;
        else if((s->workaround_bugs&FF_BUG_NO_PADDING))
            max_extra+= 256*256*256*64;

        if(left>max_extra){
            av_log(s->avctx, AV_LOG_ERROR, "discarding %d junk bits at end, next would be %X\n", left, show_bits(&s->gb, 24));
        }
        else if(left<0){
            av_log(s->avctx, AV_LOG_ERROR, "overreading %d bits\n", -left);
        }else
            ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);
//        av_log(NULL,AV_LOG_ERROR,"coming here in 3.\n");
        return 0;
    }

    av_log(s->avctx, AV_LOG_ERROR, "slice end not reached but screenspace end (%d left %06X, score= %d)\n",
            s->gb.size_in_bits - get_bits_count(&s->gb),
            show_bits(&s->gb, 24), s->padding_bug_score);

    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);

    return -1;
}

//to do
void* thread_wmv2_idct_addresidue(void* p)
{

    pthread_exit(NULL);
    return NULL;
}

//to do
void* thread_wmv2_mc(void* p)
{

    pthread_exit(NULL);
    return NULL;
}

//to do
void* thread_wmv2_deblock(void* p)
{
    Wmv2Context_amba *thiz=(Wmv2Context_amba *)p;
    MpegEncContext *s = &thiz->s;
//    int ret;
    ctx_nodef_t* p_node;
    wmv2_pic_data_t* p_pic;

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
        log_dump_p(log_fd_mvdsp,p_pic->pmvp,p_pic->mb_height*p_pic->mb_width*sizeof(mb_mv_P_t),p_pic->frame_cnt);
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


void* thread_wmv2_mc_idct_addresidue(void* p)
{
    Wmv2Context_amba *thiz=(Wmv2Context_amba *)p;
    MpegEncContext *s = &thiz->s;
//    int ret;
    ctx_nodef_t* p_node;
    wmv2_pic_data_t* p_pic;
//    int i,j;

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"thread_wmv2_mc_idct_addresidue start.\n");
    #endif

    while(thiz->mc_idct_loop)
    {
        //get data
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Waiting: thread_wmv2_mc_idct_addresidue input: wait on p_mc_idct_dataq.\n");
        #endif

        p_node=thiz->p_mc_idct_dataq->get(thiz->p_mc_idct_dataq);

        p_pic=(wmv2_pic_data_t*)p_node->p_ctx;

        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Escape: thread_wmv2_mc_idct_addresidue input: escape from p_mc_idct_dataq,p_node->p_ctx=%p.\n",p_node->p_ctx);
        #endif

        if(!p_pic)
        {
            if(p_node->flag==_flag_cmd_exit_next_)
            {
                #ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"thread_wmv2_mc_idct_addresidue get NULL data(_flag_cmd_exit_next_), start exit.\n");
                #endif

                #ifdef __log_parallel_control__
                    av_log(NULL,AV_LOG_ERROR,"**** thread_wmv2_mc_idct_addresidue, get exit cmd, start exit.\n");
                #endif

                goto mc_idct_thread_exit;
            }
            else
            {
                #ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"thread_wmv2_mc_idct_addresidue get NULL data(_flag_cmd_exit_next_), start exit.\n");
                #endif

                goto mc_idct_thread_exit;
            }
        }

//        av_log(NULL,AV_LOG_ERROR,"pic type=%d.\n",p_pic->current_picture_ptr->pict_type);
        //temp
        #if 0
        log_openfile_p(log_fd_tmp, "mpeg4_coef_tmp", p_pic->frame_cnt);
        log_dump_p(log_fd_tmp,p_pic->pdct, p_pic->mb_width*p_pic->mb_height*768,p_pic->frame_cnt);
        log_closefile_p(log_fd_tmp,p_pic->frame_cnt);
        #endif

        #ifdef __dump_DSP_test_data__
        int mo_x,mo_y;
        short* pdct=p_pic->pdct;
        #ifdef __dump_binary__
        if(!thiz->use_permutated)
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
        if(!thiz->use_permutated)
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
        //calculate dram address for uu.mpeg4s.vop_coef_daddr, uu.mpeg4s.mv_coef_daddr
        p_pic->vopinfo.uu.mp4s2.vop_coef_start_addr=VOP_COEF_DADDR+(p_pic->mb_width*p_pic->mb_height*64*2*6)*p_pic->frame_cnt;
        p_pic->vopinfo.uu.mp4s2.vop_mv_start_addr=VOP_MV_DADDR+(p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t))*p_pic->frame_cnt;
/*        p_pic->vopinfo.uu.mpeg4s.vop_coef_daddr=VOP_COEF_DADDR+(p_pic->mb_width*p_pic->mb_height*64*2*6)*p_pic->frame_cnt;
        p_pic->vopinfo.uu.mpeg4s.mv_coef_daddr=VOP_MV_DADDR+(p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t))*p_pic->frame_cnt;
        //get before
        //p_pic->vopinfo.uu.mpeg4s.vop_time_incre_res=s->avctx->time_base.den;
        //p_pic->vopinfo.uu.mpeg4s.vop_pts_high=p_pic->vopinfo.uu.mpeg4s.vop_pts_low=0;
        //p_pic->vopinfo.vop_reserved=0;
        p_pic->vopinfo.uu.mpeg4s.vop_chroma=1;//set for dsp
        //valid value
        p_pic->vopinfo.uu.mpeg4s.vop_width=p_pic->width;
        p_pic->vopinfo.uu.mpeg4s.vop_height=p_pic->height;

        p_pic->vopinfo.uu.mpeg4s.vop_code_type=(int)p_pic->current_picture_ptr->pict_type-1;

        p_pic->vopinfo.uu.mpeg4s.vop_round_type=p_pic->no_rounding;
        //p_pic->vopinfo.vop_interlaced=p_pic->current_picture_ptr->interlaced_frame;
        //p_pic->vopinfo.vop_top_field_first=p_pic->current_picture_ptr->top_field_first;
*/
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

        //use sw-sim
        if(!p_pic->use_dsp)
        {
            if(p_pic->current_picture_ptr->pict_type==FF_P_TYPE)
                wmv2_p_process_mc_idct_amba(thiz,p_pic);
            else if(p_pic->current_picture_ptr->pict_type==FF_I_TYPE)
                wmv2_i_process_idct_amba(thiz,p_pic);
            else
            {
                av_log(NULL,AV_LOG_ERROR,"not support yet pict_type=%d, in thread_wmv2_mc_idct_addresidue .\n",p_pic->current_picture_ptr->pict_type);
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
        //mb_mv_B_t* pmvb=p_pic->pmvb;
        int* pcp;
        int pcpv,pcpv1;

        //quanter sample DSP not support, we need not to do this filling
        //ambadec_assert_ffmpeg(!p_pic->quarter_sample);
        if(/*!p_pic->quarter_sample && */!thiz->abt_flag && !thiz->mspel_bit && !s->loop_filter)
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
                                ambadec_assert_ffmpeg(pmvp->mv[0].ref_frame_num==forward_ref_num);
                                //calculate chroma mv
                                pcpv=pmvp->mv[0].mv_x;
                                pcpv1=pmvp->mv[0].mv_y;
                                pmvp->mv[4].mv_x=(pcpv>>1)|(pcpv & 1);
                                //pmvp->mv[4].valid=1;
                                pmvp->mv[4].ref_frame_num=forward_ref_num;
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
                                //no 4mv
                                ambadec_assert_ffmpeg(0);
                                ambadec_assert_ffmpeg(pmvp->mv[0].ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvp->mv[1].ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvp->mv[2].ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvp->mv[3].ref_frame_num==forward_ref_num);

                                pcpv=pmvp->mv[0].mv_x+pmvp->mv[1].mv_x+pmvp->mv[2].mv_x+pmvp->mv[3].mv_x;
                                pcpv1=pmvp->mv[0].mv_y+pmvp->mv[1].mv_y+pmvp->mv[2].mv_y+pmvp->mv[3].mv_y;
                                //special rounding, from ffmpeg
                                //pmvp->mv[4].valid=1;
                                pmvp->mv[4].ref_frame_num=forward_ref_num;
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
                                //no field mv
                                ambadec_assert_ffmpeg(0);

                                ambadec_assert_ffmpeg(pmvp->mv[0].ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvp->mv[1].ref_frame_num==forward_ref_num);
                                //top field
                                pcpv=pmvp->mv[0].mv_x;
                                pcpv1=pmvp->mv[0].mv_y;
                                pmvp->mv[4].mv_x=(pcpv>>1)|(pcpv & 1);
                                //pmvp->mv[4].valid=1;
                                pmvp->mv[4].ref_frame_num=forward_ref_num;
                                pmvp->mv[4].mv_y=(pcpv1>>1)|(pcpv1 & 1);
                                //bottom field
                                pcpv=pmvp->mv[1].mv_x;
                                pcpv1=pmvp->mv[1].mv_y;
                                pmvp->mv[5].mv_x=(pcpv>>1)|(pcpv & 1);
                                //pmvp->mv[5].valid=1;
                                pmvp->mv[5].ref_frame_num=forward_ref_num;
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
            #ifdef _config_ambadec_assert_
            else
            {
                ambadec_assert_ffmpeg(p_pic->current_picture_ptr->pict_type==FF_I_TYPE);
            }
            #endif
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

            if(s->unrestricted_mv
               && !s->intra_only
               && !(s->flags&CODEC_FLAG_EMU_EDGE)) {
                //av_log(NULL,AV_LOG_ERROR,"here!!!!here!! reference,s->intra_only=%d,s->flags=%x,s->unrestricted_mv=%d,s->current_picture_ptr->reference=%d\n.\n",s->intra_only,s->flags,s->unrestricted_mv,p_pic->current_picture_ptr->reference);
                    int edges = EDGE_BOTTOM | EDGE_TOP;

                    s->dsp.draw_edges(p_pic->current_picture_ptr->data[0], p_pic->current_picture_ptr->linesize[0]  , p_pic->h_edge_pos   , p_pic->v_edge_pos   , EDGE_WIDTH  , edges);
                    s->dsp.draw_edges_nv12(p_pic->current_picture_ptr->data[1], p_pic->current_picture_ptr->linesize[1] , p_pic->h_edge_pos>>1, p_pic->v_edge_pos>>1);
        //            s->dsp.draw_edges(s->current_picture_ptr->data[2], s->uvlinesize, s->h_edge_pos>>1, s->v_edge_pos>>1, EDGE_WIDTH/2  , edges);
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
        av_log(NULL,AV_LOG_ERROR,"thread_wmv2_mc_idct_addresidue exit.\n");
    #endif

    #ifdef __log_parallel_control__
        av_log(NULL,AV_LOG_ERROR,"thread_wmv2_mc_idct_addresidue exit.\n");
    #endif

    thiz->mc_idct_loop=0;
    pthread_exit(NULL);
    return NULL;
}

void* thread_wmv2_vld(void* p)
{
    Wmv2Context_amba *thiz=(Wmv2Context_amba *)p;
    MpegEncContext *s = &thiz->s;
    const uint8_t *buf ;
    int buf_size;
    int ret;
//    AVFrame *pict;
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

        #ifdef __log_decoding_config__
        if(log_cur_frame==log_config_start_frame)
            apply_decoding_config();
        #endif

        buf = p_vld->pbuf;
        buf_size = p_vld->size;

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"  vld_thread get data, buf_size=%d .\n",p_vld->size);
        #endif

        if(s->flags&CODEC_FLAG_TRUNCATED)
        {
            av_log(s->avctx, AV_LOG_ERROR, "this codec does not support truncated bitstreams\n");
            goto freedata;
        }


    retry:

#ifdef _dump_raw_data_
        //av_log(NULL, AV_LOG_ERROR, "s->flags=%x,s->divx_packed=%d,s->bitstream_buffer_size=%d,buf_size=%d\n",s->flags,s->divx_packed,s->bitstream_buffer_size,buf_size);
        //av_log(NULL, AV_LOG_ERROR, "s->bitstream_buffer_size=%d,dump_cnt=%d\n",s->bitstream_buffer_size,dump_cnt);
        //snprintf(dump_fn,39,dump_fn_t,dump_cnt++);
        //dump_pf=fopen(dump_fn,"wb");
        //{
        //    av_log(NULL, AV_LOG_ERROR,"using buf.\n");
        //    if(dump_pf)
        //    {
        //        fwrite(buf,1,buf_size,dump_pf);
        //        fclose(dump_pf);
         //   }
            init_get_bits(&s->gb, buf, buf_size*8);
        //}
#else
        init_get_bits(&s->gb, buf, buf_size*8);
#endif

        s->bitstream_buffer_size=0;

        if (!s->context_initialized) {
            if (MPV_common_init(s) < 0) //we need the idct permutaton for reading a custom matrix
            {
                av_log(NULL,AV_LOG_ERROR,"**** Error: MPV_common_init fail in thread_wmv2_vld, exit.\n");
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

        /* let's go :-) */
        ret= ff_wmv2_decode_picture_header_amba(s);

        ambadec_assert_ffmpeg(ret!=FRAME_SKIPPED);
        //never return FRAME_SKIPPED?
        //if(ret==FRAME_SKIPPED) goto freedata;

        /* skip if the header was thrashed */
        if (ret < 0){
            av_log(s->avctx, AV_LOG_ERROR, "header damaged, ret=%d\n",ret);
            goto freedata;
        }

        s->avctx->has_b_frames= !s->low_delay;

        //wmv2 no quarter pixel
        /*
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
        */

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

            //wmv2 should not come here
            ambadec_assert_ffmpeg(0);
            MPV_common_end(s);
            s->parse_context= pc;
        }
        if (!s->context_initialized) {
            //wmv2 should not come here
            ambadec_assert_ffmpeg(0);
            avcodec_set_dimensions(s->avctx, s->width, s->height);
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"*****  vld_thread s->context_initialized=%d, retry.\n");
            #endif
            goto retry;
        }

        //determine use or not use dsp(mpeg4 IDCT+MC): abt_flag==0,mspel_bit==0 and loop_filter==0
        thiz->use_dsp=(!thiz->abt_flag)&&(!thiz->mspel_bit)&&(!s->loop_filter);

        #ifdef __tmp_use_amba_permutation__
        //permutated matrix and whether or not use dsp will determined here
        if((thiz->use_dsp || log_mask[decoding_config_use_dsp_permutated]) && thiz->use_permutated!=1 && thiz->use_dsp)
        {
            thiz->use_permutated=1;
            switch_permutated(s,1);
            av_log(NULL,AV_LOG_ERROR,"config: using dsp permutated matrix now.\n");
        }
        else if(thiz->use_permutated!=0 && ((!thiz->use_dsp) || log_mask[decoding_config_use_dsp_permutated]==0))
        {
            thiz->use_permutated=0;
            log_mask[decoding_config_use_dsp_permutated]=0;
            switch_permutated(s,0);
            av_log(NULL,AV_LOG_ERROR,"config: not using dsp permutated matrix now.\n");
        }
        #endif


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

            thiz->p_pic_dataq=ambadec_create_triqueue(_wmv2_callback_destroy_pic_data);

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
                thiz->p_mc_idct_dataq=ambadec_create_triqueue(_wmv2_callback_NULL);
                thiz->mc_idct_loop=1;
                pthread_create(&thiz->tid_mc_idct,NULL,thread_wmv2_mc_idct_addresidue,thiz);
                if(s->loop_filter)
                {
                    thiz->deblock_loop=1;
                    ambadec_assert_ffmpeg(!thiz->p_deblock_dataq);
                    thiz->p_deblock_dataq=ambadec_create_triqueue(_wmv2_callback_NULL);
                    pthread_create(&thiz->tid_deblock,NULL,thread_wmv2_deblock,thiz);
                }
            }
            else
            {
                ambadec_assert_ffmpeg(!thiz->p_mc_dataq);
                ambadec_assert_ffmpeg(!thiz->p_idct_dataq);

                thiz->p_mc_dataq=ambadec_create_triqueue(_wmv2_callback_NULL);
                thiz->p_idct_dataq=ambadec_create_triqueue(_wmv2_callback_NULL);

                thiz->mc_loop=1;
                pthread_create(&thiz->tid_mc,NULL,thread_wmv2_mc,thiz);

                thiz->idct_loop=1;
                pthread_create(&thiz->tid_idct,NULL,thread_wmv2_idct_addresidue,thiz);
                if(s->loop_filter)
                {
                    ambadec_assert_ffmpeg(!thiz->p_deblock_dataq);
                    thiz->p_deblock_dataq=ambadec_create_triqueue(_wmv2_callback_NULL);
                    thiz->deblock_loop=1;
                    pthread_create(&thiz->tid_deblock,NULL,thread_wmv2_deblock,thiz);
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
        if(
#if FF_API_HURRY_UP
        s->avctx->hurry_up &&
#endif
        s->pict_type==FF_B_TYPE) goto freedata;
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


        //try remove quarter pixel related in wmv2
        #if 1
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
        #endif

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
                av_log(NULL,AV_LOG_ERROR," Waiting: thread_wmv2_vld get thiz->p_pic.\n");
            #endif
            p_node=thiz->p_pic_dataq->get(thiz->p_pic_dataq);
            thiz->p_pic=p_node->p_ctx;
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR," Escape: thread_wmv2_vld get thiz->p_pic=%p.\n",thiz->p_pic);
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
/*        thiz->p_pic->vopinfo.uu.mpeg4s.vop_time_incre_res=s->avctx->time_base.den;
        int64_t log_pts=(90000/thiz->p_pic->vopinfo.uu.mpeg4s.vop_time_incre_res)*s->time;
        thiz->p_pic->vopinfo.uu.mpeg4s.vop_pts_low=log_pts&0xffffffff;
        thiz->p_pic->vopinfo.uu.mpeg4s.vop_pts_high=log_pts&0xffffffff00000000;*/

        //s->current_picture_ptr->pict_type=s->pict_type;
        thiz->p_pic->width=s->width;
        thiz->p_pic->height=s->height;
        thiz->p_pic->mb_width=s->mb_width;
        thiz->p_pic->mb_height=s->mb_height;
        thiz->p_pic->mb_stride=s->mb_stride;

        s->linesize=s->current_picture_ptr->linesize[0];
        s->uvlinesize=s->current_picture_ptr->linesize[1];

        _wmv2_reset_picture_data(thiz->p_pic);

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

        //wmv2 no interlace mode, try remove
        ambadec_assert_ffmpeg(!s->top_field_first);
        ambadec_assert_ffmpeg(s->progressive_frame && s->progressive_sequence);
        //s->current_picture_ptr->top_field_first= s->top_field_first;
        //s->current_picture_ptr->interlaced_frame= !s->progressive_frame && !s->progressive_sequence;


        s->current_picture_ptr->reference= 0;
        if (!s->dropable && s->pict_type != FF_B_TYPE)
        {
            s->current_picture_ptr->reference = 3;
        }


        ff_er_frame_start(s);

        //the second part of the wmv2 header contains the MB skip bits which are stored in current_picture_ptr->mb_type
        //which is not available before MPV_frame_start()
        //if (CONFIG_WMV2_DECODER && s->msmpeg4_version==5){
        ret = wmv2_decode_secondary_picture_header_amba(s);

        if(ret<0) goto freedata;
        if(ret==1) goto intrax8_decoded;
        //}

        /* decode each macroblock */
        s->mb_x=0;
        s->mb_y=0;

        wmv2_decode_slice_amba(s);
        while(s->mb_y<s->mb_height){
            if(s->slice_height==0 || s->mb_x!=0 || (s->mb_y%s->slice_height)!=0 || get_bits_count(&s->gb) > s->gb.size_in_bits)
                break;

            wmv2_decode_slice_amba(s);
        }

    intrax8_decoded:
        ff_er_frame_end_nv12(s);

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

        //wmv2 no f_code and b_code
        thiz->p_pic->no_rounding=s->no_rounding;
        //thiz->p_pic->f_code=s->f_code;
        //thiz->p_pic->b_code=s->b_code;
        //ambadec_assert_ffmpeg(!s->no_rounding);
        //wmv2 no interlace mode
        //thiz->p_pic->progressive_sequence=s->progressive_sequence;
        ambadec_assert_ffmpeg(s->progressive_sequence);

        //wmv2 has no quarter pixel, customized quant matrix, and no alternate scan
        //thiz->p_pic->quarter_sample=s->quarter_sample;
        //thiz->p_pic->mpeg_quant=s->mpeg_quant;
        //thiz->p_pic->alternate_scan=s->alternate_scan;
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
        //ambadec_assert_ffmpeg(thiz->p_pic->progressive_sequence);
        ambadec_assert_ffmpeg(!s->current_picture_ptr->interlaced_frame);

        //wmv2 has no quarter pixel?
        ambadec_assert_ffmpeg(!s->quarter_sample);
        ambadec_assert_ffmpeg(!s->alternate_scan);
        ambadec_assert_ffmpeg(!s->quarter_sample);
        ambadec_assert_ffmpeg(s->pict_type != FF_S_TYPE);

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

av_cold int wmv2_decode_init_amba(AVCodecContext *avctx)
{
    Wmv2Context_amba * const thiz= avctx->priv_data;

    if(avctx->idct_algo==FF_IDCT_AUTO){
        avctx->idct_algo=FF_IDCT_WMV2;
    }

    MpegEncContext *s = &thiz->s;
//    int i=0;
//    ctx_nodef_t* p_node;

    s->avctx = avctx;
    s->out_format = FMT_H263;

    s->width  = avctx->coded_width;
    s->height = avctx->coded_height;
    s->workaround_bugs= avctx->workaround_bugs;

    // set defaults
    MPV_decode_defaults(s);
    s->quant_precision=5;
    //s->decode_mb= ff_h263_decode_mb_amba;
    s->low_delay= 1;
    avctx->pix_fmt= avctx->get_format(avctx, avctx->codec->pix_fmts);
    s->unrestricted_mv= 1;
//    avctx->flags |= CODEC_FLAG_EMU_EDGE;
//    s->flags|=CODEC_FLAG_EMU_EDGE;
    log_cur_frame=0;

    //wmv2
    s->h263_msmpeg4 = 1;
    s->h263_pred = 1;
    s->msmpeg4_version=5;

    s->codec_id= avctx->codec->id;

    if (MPV_common_init(s) < 0)
        return -1;

    ff_msmpeg4_decode_init(s->avctx);

#ifdef __dump_DSP_test_data__
#ifdef __dump_binary__
#ifdef __dump_whole__
log_openfile_f(log_fd_picinfo, "vop_t.bin");
log_openfile_f(log_fd_residual,"vop_coef_t.bin");
log_openfile_f(log_fd_mvdsp, "vop_mv_t.bin");
//log_openfile_f(log_fd_result, "vop_result_y.bin");
//log_openfile_f(log_fd_result_2, "vop_result_uv.bin");
#endif
#endif
#endif

    //parallel related
    //use default way 1: vld->mc_idct_addresidue
    thiz->vld_loop=1;//=thiz->mc_idct_loop=1;

    thiz->p_vld_dataq=ambadec_create_triqueue(_wmv2_callback_free_vld_data);
//    thiz->p_mc_idct_dataq=ambadec_create_triqueue(_h263_callback_NULL);

    thiz->p_frame_pool=ambadec_create_triqueue(_wmv2_callback_NULL);

//    thiz->vld_sent_row=thiz->mc_sent_row=0;
    thiz->pic_finished=1;
//    thiz->previous_pic_is_b=1;
    thiz->p_pic=NULL;
    pthread_mutex_init(&thiz->mutex,NULL);

    thiz->parallel_method=1;
    pthread_create(&thiz->tid_vld,NULL,thread_wmv2_vld,thiz);
    thiz->need_restart_pipeline=0;
    thiz->pipe_line_started=0;
//    pthread_create(&thiz->tid_mc_idct,NULL,thread_h263_mc_idct_addresidue,thiz);

    #ifdef _dump_raw_data_
    av_log(NULL, AV_LOG_ERROR, "init 2 s->picture_number=%d\n",s->picture_number);
    dump_cnt=0;
    #endif


    ff_wmv2_common_init_amba(thiz);

    ff_intrax8_common_init(&thiz->x8,&thiz->s);
    thiz->use_dsp=0;

#ifdef __tmp_use_amba_permutation__
    thiz->use_permutated=-1;//un-initialized
#else
    thiz->use_permutated=0;
#endif

    return 0;
}

av_cold int wmv2_decode_end_amba(AVCodecContext *avctx)
{
    Wmv2Context_amba *thiz = avctx->priv_data;

    ff_intrax8_common_end(&thiz->x8);

    MpegEncContext *s = &thiz->s;
//    int i=0;
    ctx_nodef_t* p_node;
    void* pv;
    int ret;

    //parallel related , exit sub threads
    ambadec_assert_ffmpeg(thiz->parallel_method==1);

    p_node=thiz->p_vld_dataq->get_cmd(thiz->p_vld_dataq);
    thiz->p_vld_dataq->put_ready(thiz->p_vld_dataq,p_node,_flag_cmd_exit_next_);
    ret=pthread_join(thiz->tid_vld,&pv);
    av_log(NULL, AV_LOG_DEBUG, "ret=%d\n",ret);

    ambadec_destroy_triqueue(thiz->p_vld_dataq);

    ambadec_destroy_triqueue(thiz->p_frame_pool);


#ifdef __dump_DSP_test_data__
#ifdef __dump_binary__
#ifdef __dump_whole__
log_closefile(log_fd_picinfo);
log_closefile(log_fd_residual);
log_closefile(log_fd_mvdsp);
//log_closefile(log_fd_result);
//log_closefile(log_fd_result_2);
#endif
#endif
#endif

    MPV_common_end(s);
    return 0;
}

int wmv2_decode_frame_amba(AVCodecContext *avctx,void *data, int *data_size,AVPacket *avpkt)
{
    Wmv2Context_amba *thiz  = avctx->priv_data;
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
        if((p_node=thiz->p_vld_dataq->get_free(thiz->p_vld_dataq)))
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
#if FF_API_HURRY_UP
        p_data->hurry_up=avctx->hurry_up;
#endif
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


AVCodec wmv2_amba_decoder = {
    "wmv2-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_WMV2,
    sizeof(Wmv2Context_amba),
    wmv2_decode_init_amba,
    NULL,
    wmv2_decode_end_amba,
    wmv2_decode_frame_amba,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("Windows Media Video 8 (parallel nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};



