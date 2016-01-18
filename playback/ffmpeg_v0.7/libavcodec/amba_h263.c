/*
 * H263/MPEG4 backend for ffmpeg encoder and decoder
 * Copyright (c) 2000,2001 Fabrice Bellard
 * H263+ support.
 * Copyright (c) 2001 Juan J. Sierralta P
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2010 ambarella Zhi He <ayu3405@gmail.com>
 *
 * ac prediction encoding, B-frame support, error resilience, optimizations,
 * qpel decoding, gmc decoding, interlaced decoding
 * by Michael Niedermayer <michaelni@gmx.at>
 * NV12 support by Zhi He
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
 * @file libavcodec/amba_h263.c
 * h263/mpeg4 codec (NV12 parallel).
 */

//#define DEBUG
#include <limits.h>

#include "dsputil.h"
#include "avcodec.h"
#include "mpegvideo.h"
#include "h263data_extern.h"
#include "mpeg4data_extern.h"
#include "mathops.h"
#include "unary.h"
//#include "h263_global.h"
#include "amba_dsp_define.h"
#include "amba_dec_dsputil.h"
#include "amba_h263.h"

//#undef NDEBUG
//#include <assert.h>

static RLTable ff_h263_rl_inter = {
    102,
    58,
    inter_vlc,
    inter_run,
    inter_level,
};

static RLTable ff_mpeg4_rl_intra = {
    102,
    67,
    ff_mpeg4_intra_vlc,
    ff_mpeg4_intra_run,
    ff_mpeg4_intra_level,
};

static RLTable rvlc_rl_inter = {
    169,
    103,
    inter_rvlc,
    inter_rvlc_run,
    inter_rvlc_level,
};

static RLTable rvlc_rl_intra = {
    169,
    103,
    intra_rvlc,
    intra_rvlc_run,
    intra_rvlc_level,
};

static RLTable rl_intra_aic = {
    102,
    58,
    intra_vlc_aic,
    intra_run_aic,
    intra_level_aic,
};

#define INTRA_MCBPC_VLC_BITS 6
#define INTER_MCBPC_VLC_BITS 7
#define CBPY_VLC_BITS 6
#define MV_VLC_BITS 9
#define DC_VLC_BITS 9
#define SPRITE_TRAJ_VLC_BITS 6
#define MB_TYPE_B_VLC_BITS 4
#define TEX_VLC_BITS 9
#define H263_MBTYPE_B_VLC_BITS 6
#define CBPC_B_VLC_BITS 3
/*
static void h263_encode_block(MpegEncContext * s, DCTELEM * block,
                              int n);
static void h263p_encode_umotion(MpegEncContext * s, int val);
static inline void mpeg4_encode_block(MpegEncContext * s, DCTELEM * block,
                               int n, int dc, uint8_t *scan_table,
                               PutBitContext *dc_pb, PutBitContext *ac_pb);
static int mpeg4_get_block_length(MpegEncContext * s, DCTELEM * block, int n, int intra_dc,
                                  uint8_t *scan_table);
*/
static int h263_decode_motion(MpegEncContext * s, int pred, int fcode);
static int h263p_decode_umotion(MpegEncContext * s, int pred);
static int h263_decode_block(MpegEncContext * s, DCTELEM * block,
                             int n, int coded);
static inline int mpeg4_decode_dc(MpegEncContext * s, int n, int *dir_ptr);
static inline int mpeg4_decode_block(MpegEncContext * s, DCTELEM * block,
                              int n, int coded, int intra, int rvlc);
/*
static int h263_pred_dc(MpegEncContext * s, int n, int16_t **dc_val_ptr);
static void mpeg4_encode_visual_object_header(MpegEncContext * s);
static void mpeg4_encode_vol_header(MpegEncContext * s, int vo_number, int vol_number);
*/
static void mpeg4_decode_sprite_trajectory(MpegEncContext * s, GetBitContext *gb);
static inline int ff_mpeg4_pred_dc(MpegEncContext * s, int n, int level, int *dir_ptr, int encoding);

#if CONFIG_ENCODERS
/*static uint8_t uni_DCtab_lum_len[512];
static uint8_t uni_DCtab_chrom_len[512];
static uint16_t uni_DCtab_lum_bits[512];
static uint16_t uni_DCtab_chrom_bits[512];

static uint8_t mv_penalty[MAX_FCODE+1][MAX_MV*2+1];
static uint8_t fcode_tab[MAX_MV*2+1];
static uint8_t umv_fcode_tab[MAX_MV*2+1];

static uint32_t uni_mpeg4_intra_rl_bits[64*64*2*2];
static uint8_t  uni_mpeg4_intra_rl_len [64*64*2*2];
static uint32_t uni_mpeg4_inter_rl_bits[64*64*2*2];
static uint8_t  uni_mpeg4_inter_rl_len [64*64*2*2];
static uint8_t  uni_h263_intra_aic_rl_len [64*64*2*2];
static uint8_t  uni_h263_inter_rl_len [64*64*2*2];*/
//#define UNI_MPEG4_ENC_INDEX(last,run,level) ((last)*128 + (run)*256 + (level))
//#define UNI_MPEG4_ENC_INDEX(last,run,level) ((last)*128*64 + (run) + (level)*64)
#define UNI_MPEG4_ENC_INDEX(last,run,level) ((last)*128*64 + (run)*128 + (level))



/* mpeg4
inter
max level: 24/6
max run: 53/63

intra
max level: 53/16
max run: 29/41
*/
#endif

static uint8_t static_rl_table_store[5][2][2*MAX_RUN + MAX_LEVEL + 3];

#if 0 //3IV1 is quite rare and it slows things down a tiny bit
#define IS_3IV1 s->codec_tag == AV_RL32("3IV1")
#else
#define IS_3IV1 0
#endif


/***********************************************/
/* decoding */

static VLC intra_MCBPC_vlc;
static VLC inter_MCBPC_vlc;
static VLC cbpy_vlc;
static VLC mv_vlc;
static VLC dc_lum, dc_chrom;
static VLC sprite_trajectory;
static VLC mb_type_b_vlc;
static VLC h263_mbtype_b_vlc;
static VLC cbpc_b_vlc;

/* init vlcs */


/* XXX: find a better solution to handle static init */
void h263_decode_init_vlc_amba(MpegEncContext *s)
{
    static int done = 0;

    if (!done) {
        done = 1;

        INIT_VLC_STATIC(&intra_MCBPC_vlc, INTRA_MCBPC_VLC_BITS, 9,
                 ff_h263_intra_MCBPC_bits, 1, 1,
                 ff_h263_intra_MCBPC_code, 1, 1, 72);
        INIT_VLC_STATIC(&inter_MCBPC_vlc, INTER_MCBPC_VLC_BITS, 28,
                 ff_h263_inter_MCBPC_bits, 1, 1,
                 ff_h263_inter_MCBPC_code, 1, 1, 198);
        INIT_VLC_STATIC(&cbpy_vlc, CBPY_VLC_BITS, 16,
                 &ff_h263_cbpy_tab[0][1], 2, 1,
                 &ff_h263_cbpy_tab[0][0], 2, 1, 64);
        INIT_VLC_STATIC(&mv_vlc, MV_VLC_BITS, 33,
                 &mvtab[0][1], 2, 1,
                 &mvtab[0][0], 2, 1, 538);
        init_rl(&ff_h263_rl_inter, static_rl_table_store[0]);
        init_rl(&ff_mpeg4_rl_intra, static_rl_table_store[1]);
        init_rl(&rvlc_rl_inter, static_rl_table_store[3]);
        init_rl(&rvlc_rl_intra, static_rl_table_store[4]);
        init_rl(&rl_intra_aic, static_rl_table_store[2]);
        INIT_VLC_RL(ff_h263_rl_inter, 554);
        INIT_VLC_RL(ff_mpeg4_rl_intra, 554);
        INIT_VLC_RL(rvlc_rl_inter, 1072);
        INIT_VLC_RL(rvlc_rl_intra, 1072);
        INIT_VLC_RL(rl_intra_aic, 554);
        INIT_VLC_STATIC(&dc_lum, DC_VLC_BITS, 10 /* 13 */,
                 &ff_mpeg4_DCtab_lum[0][1], 2, 1,
                 &ff_mpeg4_DCtab_lum[0][0], 2, 1, 512);
        INIT_VLC_STATIC(&dc_chrom, DC_VLC_BITS, 10 /* 13 */,
                 &ff_mpeg4_DCtab_chrom[0][1], 2, 1,
                 &ff_mpeg4_DCtab_chrom[0][0], 2, 1, 512);
        INIT_VLC_STATIC(&sprite_trajectory, SPRITE_TRAJ_VLC_BITS, 15,
                 &sprite_trajectory_tab[0][1], 4, 2,
                 &sprite_trajectory_tab[0][0], 4, 2, 128);
        INIT_VLC_STATIC(&mb_type_b_vlc, MB_TYPE_B_VLC_BITS, 4,
                 &mb_type_b_tab[0][1], 2, 1,
                 &mb_type_b_tab[0][0], 2, 1, 16);
        INIT_VLC_STATIC(&h263_mbtype_b_vlc, H263_MBTYPE_B_VLC_BITS, 15,
                 &h263_mbtype_b_tab[0][1], 2, 1,
                 &h263_mbtype_b_tab[0][0], 2, 1, 80);
        INIT_VLC_STATIC(&cbpc_b_vlc, CBPC_B_VLC_BITS, 4,
                 &cbpc_b_tab[0][1], 2, 1,
                 &cbpc_b_tab[0][0], 2, 1, 8);
    }
}



static inline void set_intra_p(mb_mv_P_t* pmvp)
{
    uint32_t *pi=(uint32_t *)&pmvp->mv[0];
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi=not_valid_mv_value;
}

static inline void set_intra_b(mb_mv_B_t* pmvb)
{
    uint32_t *pi=(uint32_t *)&pmvb->mvp[0].fw;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi++=not_valid_mv_value;
    *pi=not_valid_mv_value;
}
/*
static void show_pict_info(MpegEncContext *s){
    av_log(s->avctx, AV_LOG_DEBUG, "qp:%d %c size:%d rnd:%d%s%s%s%s%s%s%s%s%s %d/%d\n",
         s->qscale, av_get_pict_type_char(s->pict_type),
         s->gb.size_in_bits, 1-s->no_rounding,
         s->obmc ? " AP" : "",
         s->umvplus ? " UMV" : "",
         s->h263_long_vectors ? " LONG" : "",
         s->h263_plus ? " +" : "",
         s->h263_aic ? " AIC" : "",
         s->alt_inter_vlc ? " AIV" : "",
         s->modified_quant ? " MQ" : "",
         s->loop_filter ? " LOOP" : "",
         s->h263_slice_structured ? " SS" : "",
         s->avctx->time_base.den, s->avctx->time_base.num
    );
}
*/
#define tab_size ((signed)FF_ARRAY_ELEMS(s->direct_scale_mv[0]))
#define tab_bias (tab_size/2)

static inline void ff_mpeg4_set_one_direct_mv_amba(MpegEncContext *s, int mx, int my, int i, mb_mv_B_t* pmvb){
    int xy= s->block_index[i];
    uint16_t time_pp= s->pp_time;
    uint16_t time_pb= s->pb_time;
    int p_mx, p_my;

    p_mx= s->next_picture_ptr->motion_val[0][xy][0];
    //pmvb->mvp[i].fw.valid=1;
    //pmvb->mvp[i].bw.valid=1;
    pmvb->mvp[i].fw.ref_frame_num=forward_ref_num;
    pmvb->mvp[i].bw.ref_frame_num=backward_ref_num;
    if((unsigned)(p_mx + tab_bias) < tab_size){
        pmvb->mvp[i].fw.mv_x= s->mv[0][i][0] = s->direct_scale_mv[0][p_mx + tab_bias] + mx;
        pmvb->mvp[i].bw.mv_x=s->mv[1][i][0]  = mx ? s->mv[0][i][0] - p_mx
                            : s->direct_scale_mv[1][p_mx + tab_bias];
    }else{
        pmvb->mvp[i].fw.mv_x = s->mv[0][i][0] = p_mx*time_pb/time_pp + mx;
        pmvb->mvp[i].bw.mv_x =s->mv[1][i][0] = mx ? s->mv[0][i][0] - p_mx
                            : p_mx*(time_pb - time_pp)/time_pp;
    }
    p_my= s->next_picture_ptr->motion_val[0][xy][1];
    if((unsigned)(p_my + tab_bias) < tab_size){
        pmvb->mvp[i].fw.mv_y = s->mv[0][i][1] = s->direct_scale_mv[0][p_my + tab_bias] + my;
        pmvb->mvp[i].bw.mv_y =s->mv[1][i][1] = my ? s->mv[0][i][1] - p_my
                            : s->direct_scale_mv[1][p_my + tab_bias];
    }else{
        pmvb->mvp[i].fw.mv_y = s->mv[0][i][1] = p_my*time_pb/time_pp + my;
        pmvb->mvp[i].bw.mv_y = s->mv[1][i][1] =  my ? s->mv[0][i][1] - p_my
                            : p_my*(time_pb - time_pp)/time_pp;
    }
}

#undef tab_size
#undef tab_bias

/**
 *
 * @return the mb_type
 */
static int ff_mpeg4_set_direct_mv_amba(MpegEncContext *s, int mx, int my, mb_mv_B_t* pmvb){
    const int mb_index= s->mb_x + s->mb_y*s->mb_stride;
    const int colocated_mb_type= s->next_picture_ptr->mb_type[mb_index];
    uint16_t time_pp;
    uint16_t time_pb;
    int i;
//    H263AmbaDecContext_t* thiz=(H263AmbaDecContext_t*)s;
//    mb_mv_B_t* pmvb= thiz->p_pic->pmvb+s->mb_x + s->mb_y*s->mb_width;
    //FIXME avoid divides
    // try special case with shifts for 1 and 3 B-frames?

    if(IS_8X8(colocated_mb_type)){
        s->mv_type = MV_TYPE_8X8;
        for(i=0; i<4; i++){
            ff_mpeg4_set_one_direct_mv_amba(s, mx, my, i, pmvb);
        }
        pmvb->mb_setting.mv_type=mv_type_8x8;
        return MB_TYPE_DIRECT2 | MB_TYPE_8x8 | MB_TYPE_L0L1;
    } else if(IS_INTERLACED(colocated_mb_type)){
        pmvb->mb_setting.mv_type=mv_type_16x8;
        pmvb->mb_setting.field_prediction=1;
        s->mv_type = MV_TYPE_FIELD;
        for(i=0; i<2; i++){
            int field_select= s->next_picture_ptr->ref_index[0][s->block_index[2*i]];
            pmvb->mvp[i].fw.bottom=field_select;
            pmvb->mvp[i].bw.bottom=i;
            pmvb->mvp[i].fw.ref_frame_num=forward_ref_num;
            pmvb->mvp[i].bw.ref_frame_num=backward_ref_num;
//            s->field_select[0][i]= field_select;
//            s->field_select[1][i]= i;
            if(s->top_field_first){
                time_pp= s->pp_field_time - field_select + i;
                time_pb= s->pb_field_time - field_select + i;
            }else{
                time_pp= s->pp_field_time + field_select - i;
                time_pb= s->pb_field_time + field_select - i;
            }
            pmvb->mvp[i].fw.mv_x= s->mv[0][i][0] = s->p_field_mv_table[i][0][mb_index][0]*time_pb/time_pp + mx;
            //   s->mv[0][i][0] =
            pmvb->mvp[i].fw.mv_y= s->mv[0][i][1] = s->p_field_mv_table[i][0][mb_index][1]*time_pb/time_pp + my;
            //s->mv[0][i][1] =
            pmvb->mvp[i].bw.mv_x= s->mv[1][i][0] =   mx ? pmvb->mvp[i].fw.mv_x - s->p_field_mv_table[i][0][mb_index][0]
                                : s->p_field_mv_table[i][0][mb_index][0]*(time_pb - time_pp)/time_pp;
            //s->mv[1][i][0] =
            pmvb->mvp[i].bw.mv_y= s->mv[1][i][1] =  my ? pmvb->mvp[i].fw.mv_y - s->p_field_mv_table[i][0][mb_index][1]
                                : s->p_field_mv_table[i][0][mb_index][1]*(time_pb - time_pp)/time_pp;
            //pmvb->mvp[i].fw.valid=1;
            //pmvb->mvp[i].bw.valid=1;
            //s->mv[1][i][1] =
        }

//  try_remove B picture need not fill this
/*        s->current_picture_ptr->ref_index[0][s->block_index[0]          ]=
        s->current_picture_ptr->ref_index[0][s->block_index[0]        + 1]= s->field_select[0][0];
        s->current_picture_ptr->ref_index[0][s->block_index[0] + s->b8_stride    ]=
        s->current_picture_ptr->ref_index[0][s->block_index[0] + s->b8_stride + 1]= s->field_select[0][1];*/

        return MB_TYPE_DIRECT2 | MB_TYPE_16x8 | MB_TYPE_L0L1 | MB_TYPE_INTERLACED;
    }else{

        ff_mpeg4_set_one_direct_mv_amba(s, mx, my, 0,pmvb);
        /* try_remove
        s->mv[0][1][0] = s->mv[0][2][0] = s->mv[0][3][0] = s->mv[0][0][0];
        s->mv[0][1][1] = s->mv[0][2][1] = s->mv[0][3][1] = s->mv[0][0][1];
        s->mv[1][1][0] = s->mv[1][2][0] = s->mv[1][3][0] = s->mv[1][0][0];
        s->mv[1][1][1] = s->mv[1][2][1] = s->mv[1][3][1] = s->mv[1][0][1];*/
        //need_change
        if((s->avctx->workaround_bugs & FF_BUG_DIRECT_BLOCKSIZE) || !s->quarter_sample)
            s->mv_type= MV_TYPE_16X16;
        else
            s->mv_type= MV_TYPE_8X8;
        return MB_TYPE_DIRECT2 | MB_TYPE_16x16 | MB_TYPE_L0L1; //Note see prev line
    }
}

void ff_h263_update_motion_val_amba_new(MpegEncContext * s,mb_mv_P_t* pmvp){
    const int mb_xy = s->mb_y * s->mb_stride + s->mb_x;
               //FIXME a lot of that is only needed for !low_delay
    const int wrap = s->b8_stride;
    const int xy = s->block_index[0];

    s->current_picture_ptr->mbskip_table[mb_xy]= s->mb_skipped;

    if(s->mv_type != MV_TYPE_8X8){
        int motion_x, motion_y;
        if (s->mb_intra) {
            motion_x = 0;
            motion_y = 0;
        } else if (s->mv_type == MV_TYPE_16X16) {
            motion_x = pmvp->mv[0].mv_x;
            motion_y = pmvp->mv[0].mv_y;
        } else /*if (s->mv_type == MV_TYPE_FIELD)*/ {
            int i;
            motion_x = pmvp->mv[0].mv_x + pmvp->mv[1].mv_x;
            motion_y = pmvp->mv[0].mv_y + pmvp->mv[1].mv_y;
            motion_x = (motion_x>>1) | (motion_x&1);
            for(i=0; i<2; i++){
                s->p_field_mv_table[i][0][mb_xy][0]= pmvp->mv[i].mv_x;
                s->p_field_mv_table[i][0][mb_xy][1]= pmvp->mv[i].mv_y;
            }
            s->current_picture_ptr->ref_index[0][xy           ]=
            s->current_picture_ptr->ref_index[0][xy        + 1]= pmvp->mv[0].bottom;
            s->current_picture_ptr->ref_index[0][xy + wrap    ]=
            s->current_picture_ptr->ref_index[0][xy + wrap + 1]= pmvp->mv[1].bottom;
        }

        /* no update if 8X8 because it has been done during parsing */
        s->current_picture_ptr->motion_val[0][xy][0] = motion_x;
        s->current_picture_ptr->motion_val[0][xy][1] = motion_y;
        s->current_picture_ptr->motion_val[0][xy + 1][0] = motion_x;
        s->current_picture_ptr->motion_val[0][xy + 1][1] = motion_y;
        s->current_picture_ptr->motion_val[0][xy + wrap][0] = motion_x;
        s->current_picture_ptr->motion_val[0][xy + wrap][1] = motion_y;
        s->current_picture_ptr->motion_val[0][xy + 1 + wrap][0] = motion_x;
        s->current_picture_ptr->motion_val[0][xy + 1 + wrap][1] = motion_y;
    }

}

void ff_h263_update_motion_val_amba(MpegEncContext * s){
    const int mb_xy = s->mb_y * s->mb_stride + s->mb_x;
               //FIXME a lot of that is only needed for !low_delay
    const int wrap = s->b8_stride;
    const int xy = s->block_index[0];

    s->current_picture_ptr->mbskip_table[mb_xy]= s->mb_skipped;

    if(s->mv_type != MV_TYPE_8X8){
        int motion_x, motion_y;
        if (s->mb_intra) {
            motion_x = 0;
            motion_y = 0;
        } else if (s->mv_type == MV_TYPE_16X16) {
            motion_x = s->mv[0][0][0];
            motion_y = s->mv[0][0][1];
        } else /*if (s->mv_type == MV_TYPE_FIELD)*/ {
            int i;
            motion_x = s->mv[0][0][0] + s->mv[0][1][0];
            motion_y = s->mv[0][0][1] + s->mv[0][1][1];
            motion_x = (motion_x>>1) | (motion_x&1);
            for(i=0; i<2; i++){
                s->p_field_mv_table[i][0][mb_xy][0]= s->mv[0][i][0];
                s->p_field_mv_table[i][0][mb_xy][1]= s->mv[0][i][1];
            }
            s->current_picture_ptr->ref_index[0][xy           ]=
            s->current_picture_ptr->ref_index[0][xy        + 1]= s->field_select[0][0];
            s->current_picture_ptr->ref_index[0][xy + wrap    ]=
            s->current_picture_ptr->ref_index[0][xy + wrap + 1]= s->field_select[0][1];
        }

        /* no update if 8X8 because it has been done during parsing */
        s->current_picture_ptr->motion_val[0][xy][0] = motion_x;
        s->current_picture_ptr->motion_val[0][xy][1] = motion_y;
        s->current_picture_ptr->motion_val[0][xy + 1][0] = motion_x;
        s->current_picture_ptr->motion_val[0][xy + 1][1] = motion_y;
        s->current_picture_ptr->motion_val[0][xy + wrap][0] = motion_x;
        s->current_picture_ptr->motion_val[0][xy + wrap][1] = motion_y;
        s->current_picture_ptr->motion_val[0][xy + 1 + wrap][0] = motion_x;
        s->current_picture_ptr->motion_val[0][xy + 1 + wrap][1] = motion_y;
    }

    if(s->encoding){ //FIXME encoding MUST be cleaned up
        if (s->mv_type == MV_TYPE_8X8)
            s->current_picture_ptr->mb_type[mb_xy]= MB_TYPE_L0 | MB_TYPE_8x8;
        else if(s->mb_intra)
            s->current_picture_ptr->mb_type[mb_xy]= MB_TYPE_INTRA;
        else
            s->current_picture_ptr->mb_type[mb_xy]= MB_TYPE_L0 | MB_TYPE_16x16;
    }
}

#if 0
void ff_h263_loop_filter_amba(MpegEncContext * s){
    int qp_c;
    const int linesize  = s->linesize;
    const int uvlinesize= s->uvlinesize;
    const int xy = s->mb_y * s->mb_stride + s->mb_x;
    uint8_t *dest_y = s->dest[0];
    uint8_t *dest_cb= s->dest[1];
    uint8_t *dest_cr= s->dest[2];

//    if(s->pict_type==FF_B_TYPE && !s->readable) return;

    /*
       Diag Top
       Left Center
    */
    if(!IS_SKIP(s->current_picture_ptr->mb_type[xy])){
        qp_c= s->qscale;
        s->dsp.h263_v_loop_filter(dest_y+8*linesize  , linesize, qp_c);
        s->dsp.h263_v_loop_filter(dest_y+8*linesize+8, linesize, qp_c);
    }else
        qp_c= 0;

    if(s->mb_y){
        int qp_dt, qp_tt, qp_tc;

        if(IS_SKIP(s->current_picture_ptr->mb_type[xy-s->mb_stride]))
            qp_tt=0;
        else
            qp_tt= s->current_picture_ptr->qscale_table[xy-s->mb_stride];

        if(qp_c)
            qp_tc= qp_c;
        else
            qp_tc= qp_tt;

        if(qp_tc){
            const int chroma_qp= s->chroma_qscale_table[qp_tc];
            s->dsp.h263_v_loop_filter(dest_y  ,   linesize, qp_tc);
            s->dsp.h263_v_loop_filter(dest_y+8,   linesize, qp_tc);

            s->dsp.h263_v_loop_filter(dest_cb , uvlinesize, chroma_qp);
            s->dsp.h263_v_loop_filter(dest_cr , uvlinesize, chroma_qp);
        }

        if(qp_tt)
            s->dsp.h263_h_loop_filter(dest_y-8*linesize+8  ,   linesize, qp_tt);

        if(s->mb_x){
            if(qp_tt || IS_SKIP(s->current_picture_ptr->mb_type[xy-1-s->mb_stride]))
                qp_dt= qp_tt;
            else
                qp_dt= s->current_picture_ptr->qscale_table[xy-1-s->mb_stride];

            if(qp_dt){
                const int chroma_qp= s->chroma_qscale_table[qp_dt];
                s->dsp.h263_h_loop_filter(dest_y -8*linesize  ,   linesize, qp_dt);
                s->dsp.h263_h_loop_filter(dest_cb-8*uvlinesize, uvlinesize, chroma_qp);
                s->dsp.h263_h_loop_filter(dest_cr-8*uvlinesize, uvlinesize, chroma_qp);
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
        if(qp_c || IS_SKIP(s->current_picture_ptr->mb_type[xy-1]))
            qp_lc= qp_c;
        else
            qp_lc= s->current_picture_ptr->qscale_table[xy-1];

        if(qp_lc){
            s->dsp.h263_h_loop_filter(dest_y,   linesize, qp_lc);
            if(s->mb_y + 1 == s->mb_height){
                const int chroma_qp= s->chroma_qscale_table[qp_lc];
                s->dsp.h263_h_loop_filter(dest_y +8*  linesize,   linesize, qp_lc);
                s->dsp.h263_h_loop_filter(dest_cb             , uvlinesize, chroma_qp);
                s->dsp.h263_h_loop_filter(dest_cr             , uvlinesize, chroma_qp);
            }
        }
    }
}
#endif

static void h263_pred_acdc(MpegEncContext * s, DCTELEM *block, int n)
{
    int x, y, wrap, a, c, pred_dc, scale, i;
    int16_t *dc_val, *ac_val, *ac_val1;

    /* find prediction */
    if (n < 4) {
        x = 2 * s->mb_x + (n & 1);
        y = 2 * s->mb_y + (n>> 1);
        wrap = s->b8_stride;
        dc_val = s->dc_val[0];
        ac_val = s->ac_val[0][0];
        scale = s->y_dc_scale;
    } else {
        x = s->mb_x;
        y = s->mb_y;
        wrap = s->mb_stride;
        dc_val = s->dc_val[n - 4 + 1];
        ac_val = s->ac_val[n - 4 + 1][0];
        scale = s->c_dc_scale;
    }

    ac_val += ((y) * wrap + (x)) * 16;
    ac_val1 = ac_val;

    /* B C
     * A X
     */
    a = dc_val[(x - 1) + (y) * wrap];
    c = dc_val[(x) + (y - 1) * wrap];

    /* No prediction outside GOB boundary */
    if(s->first_slice_line && n!=3){
        if(n!=2) c= 1024;
        if(n!=1 && s->mb_x == s->resync_mb_x) a= 1024;
    }

    if (s->ac_pred) {
        pred_dc = 1024;
        if (s->h263_aic_dir) {
            /* left prediction */
            if (a != 1024) {
                ac_val -= 16;
                for(i=1;i<8;i++) {
                    block[s->dsp.idct_permutation[i<<3]] += ac_val[i];
                }
                pred_dc = a;
            }
        } else {
            /* top prediction */
            if (c != 1024) {
                ac_val -= 16 * wrap;
                for(i=1;i<8;i++) {
                    block[s->dsp.idct_permutation[i   ]] += ac_val[i + 8];
                }
                pred_dc = c;
            }
        }
    } else {
        /* just DC prediction */
        if (a != 1024 && c != 1024)
            pred_dc = (a + c) >> 1;
        else if (a != 1024)
            pred_dc = a;
        else
            pred_dc = c;
    }

    /* we assume pred is positive */
    block[0]=block[0]*scale + pred_dc;

    if (block[0] < 0)
        block[0] = 0;
    else
        block[0] |= 1;

    /* Update AC/DC tables */
    dc_val[(x) + (y) * wrap] = block[0];

    /* left copy */
    for(i=1;i<8;i++)
        ac_val1[i    ] = block[s->dsp.idct_permutation[i<<3]];
    /* top copy */
    for(i=1;i<8;i++)
        ac_val1[8 + i] = block[s->dsp.idct_permutation[i   ]];
}


/**
 * predicts the dc.
 * encoding quantized level -> quantized diff
 * decoding quantized diff -> quantized level
 * @param n block index (0-3 are luma, 4-5 are chroma)
 * @param dir_ptr pointer to an integer where the prediction direction will be stored
 */
static inline int ff_mpeg4_pred_dc(MpegEncContext * s, int n, int level, int *dir_ptr, int encoding)
{
    int a, b, c, wrap, pred, scale, ret;
    int16_t *dc_val;

    /* find prediction */
    if (n < 4) {
        scale = s->y_dc_scale;
    } else {
        scale = s->c_dc_scale;
    }
    if(IS_3IV1)
        scale= 8;

    wrap= s->block_wrap[n];
    dc_val = s->dc_val[0] + s->block_index[n];

    /* B C
     * A X
     */
    a = dc_val[ - 1];
    b = dc_val[ - 1 - wrap];
    c = dc_val[ - wrap];

//    av_log(NULL, AV_LOG_ERROR, "pred a=%d,b=%d,c=%d,wrap=%d.\n",a,b,c,wrap);

    /* outside slice handling (we can't do that by memset as we need the dc for error resilience) */
    if(s->first_slice_line && n!=3){
        if(n!=2) b=c= 1024;
        if(n!=1 && s->mb_x == s->resync_mb_x) b=a= 1024;
    }
    if(s->mb_x == s->resync_mb_x && s->mb_y == s->resync_mb_y+1){
        if(n==0 || n==4 || n==5)
            b=1024;
    }

    if (abs(a - b) < abs(b - c)) {
        pred = c;
        *dir_ptr = 1; /* top */
    } else {
        pred = a;
        *dir_ptr = 0; /* left */
    }
    /* we assume pred is positive */
    pred = FASTDIV((pred + (scale >> 1)), scale);

    if(encoding){
        ret = level - pred;
    }else{
        level += pred;
        ret= level;
        if(s->error_recognition>=3){
            if(level<0){
                av_log(s->avctx, AV_LOG_ERROR, "dc<0 at %dx%d\n", s->mb_x, s->mb_y);
                return -1;
            }
            if(level*scale > 2048 + scale){
                av_log(s->avctx, AV_LOG_ERROR, "dc overflow at %dx%d\n", s->mb_x, s->mb_y);
                return -1;
            }
        }
    }
    level *=scale;
    if(level&(~2047)){
        if(level<0)
            level=0;
        else if(!(s->workaround_bugs&FF_BUG_DC_CLIP))
            level=2047;
    }
    dc_val[0]= level;

//    av_log(NULL, AV_LOG_ERROR, "pred ret=%d.\n", ret);

    return ret;
}

/**
 * predicts the ac.
 * @param n block index (0-3 are luma, 4-5 are chroma)
 * @param dir the ac prediction direction
 */

/***********************************************/

/**
 * decodes the group of blocks header or slice header.
 * @return <0 if an error occurred
 */
static int h263_decode_gob_header(MpegEncContext *s)
{
    unsigned int val, gob_number, gfid=0;
    int left;

    /* Check for GOB Start Code */
    val = show_bits(&s->gb, 16);
    if(val)
        return -1;

        /* We have a GBSC probably with GSTUFF */
    skip_bits(&s->gb, 16); /* Drop the zeros */
    left= s->gb.size_in_bits - get_bits_count(&s->gb);
    //MN: we must check the bits left or we might end in a infinite loop (or segfault)
    for(;left>13; left--){
        if(get_bits1(&s->gb)) break; /* Seek the '1' bit */
    }
    if(left<=13)
        return -1;

    if(s->h263_slice_structured){
        if(get_bits1(&s->gb)==0)
            return -1;

        ff_h263_decode_mba(s);

        if(s->mb_num > 1583)
            if(get_bits1(&s->gb)==0)
                return -1;

        s->qscale = get_bits(&s->gb, 5); /* SQUANT */
        if(get_bits1(&s->gb)==0)
            return -1;
        gfid = get_bits(&s->gb, 2); /* GFID */
        av_log(s->avctx, AV_LOG_DEBUG, "gfid=%u\n", gfid);
    }else{
        gob_number = get_bits(&s->gb, 5); /* GN */
        s->mb_x= 0;
        s->mb_y= s->gob_index* gob_number;
        gfid = get_bits(&s->gb, 2); /* GFID */
        av_log(s->avctx, AV_LOG_DEBUG, "gfid=%u\n", gfid);
        s->qscale = get_bits(&s->gb, 5); /* GQUANT */
    }

    if(s->mb_y >= s->mb_height)
        return -1;

    if(s->qscale==0)
        return -1;

    return 0;
}

static inline void memsetw(short *tab, int val, int n)
{
    int i;
    for(i=0;i<n;i++)
        tab[i] = val;
}


/**
 * check if the next stuff is a resync marker or the end.
 * @return 0 if not
 */
static inline int mpeg4_is_resync(MpegEncContext *s){
    int bits_count= get_bits_count(&s->gb);
    int v= show_bits(&s->gb, 16);

    if(s->workaround_bugs&FF_BUG_NO_PADDING){
        return 0;
    }

    while(v<=0xFF){
        if(s->pict_type==FF_B_TYPE || (v>>(8-s->pict_type)!=1) || s->partitioned_frame)
            break;
        skip_bits(&s->gb, 8+s->pict_type);
        bits_count+= 8+s->pict_type;
        v= show_bits(&s->gb, 16);
    }

    if(bits_count + 8 >= s->gb.size_in_bits){
        v>>=8;
        v|= 0x7F >> (7-(bits_count&7));

        if(v==0x7F)
            return 1;
    }else{
        if(v == ff_mpeg4_resync_prefix[bits_count&7]){
            int len;
            GetBitContext gb= s->gb;

            skip_bits(&s->gb, 1);
            align_get_bits(&s->gb);

            for(len=0; len<32; len++){
                if(get_bits1(&s->gb)) break;
            }

            s->gb= gb;

            if(len>=ff_mpeg4_get_video_packet_prefix_length(s))
                return 1;
        }
    }
    return 0;
}

/**
 * decodes the next video packet.
 * @return <0 if something went wrong
 */
static int mpeg4_decode_video_packet_header(MpegEncContext *s)
{
    int mb_num_bits= av_log2(s->mb_num - 1) + 1;
    int header_extension=0, mb_num, len;

    /* is there enough space left for a video packet + header */
    if( get_bits_count(&s->gb) > s->gb.size_in_bits-20) return -1;

    for(len=0; len<32; len++){
        if(get_bits1(&s->gb)) break;
    }

    if(len!=ff_mpeg4_get_video_packet_prefix_length(s)){
        av_log(s->avctx, AV_LOG_ERROR, "marker does not match f_code\n");
        return -1;
    }

    if(s->shape != RECT_SHAPE){
        header_extension= get_bits1(&s->gb);
        //FIXME more stuff here
    }

    mb_num= get_bits(&s->gb, mb_num_bits);
    if(mb_num>=s->mb_num){
        av_log(s->avctx, AV_LOG_ERROR, "illegal mb_num in video packet (%d %d) \n", mb_num, s->mb_num);
        return -1;
    }
    if(s->pict_type == FF_B_TYPE){
        while(s->next_picture_ptr->mbskip_table[ s->mb_index2xy[ mb_num ] ]) mb_num++;
        if(mb_num >= s->mb_num) return -1; // slice contains just skipped MBs which where already decoded
    }

    s->mb_x= mb_num % s->mb_width;
    s->mb_y= mb_num / s->mb_width;

    if(s->shape != BIN_ONLY_SHAPE){
        int qscale= get_bits(&s->gb, s->quant_precision);
        if(qscale)
            s->chroma_qscale=s->qscale= qscale;
    }

    if(s->shape == RECT_SHAPE){
        header_extension= get_bits1(&s->gb);
    }
    if(header_extension){
        int time_increment=0;
        int time_incr=0;

        while (get_bits1(&s->gb) != 0)
            time_incr++;

        check_marker(&s->gb, "before time_increment in video packed header");
        time_increment= get_bits(&s->gb, s->time_increment_bits);
        av_log(s->avctx, AV_LOG_DEBUG, "time_increment=%d\n", time_increment);
        check_marker(&s->gb, "before vop_coding_type in video packed header");

        skip_bits(&s->gb, 2); /* vop coding type */
        //FIXME not rect stuff here

        if(s->shape != BIN_ONLY_SHAPE){
            skip_bits(&s->gb, 3); /* intra dc vlc threshold */
//FIXME don't just ignore everything
            if(s->pict_type == FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE){
                mpeg4_decode_sprite_trajectory(s, &s->gb);
                av_log(s->avctx, AV_LOG_ERROR, "untested\n");
            }

            //FIXME reduced res stuff here

            if (s->pict_type != FF_I_TYPE) {
                int f_code = get_bits(&s->gb, 3);       /* fcode_for */
                if(f_code==0){
                    av_log(s->avctx, AV_LOG_ERROR, "Error, video packet header damaged (f_code=0)\n");
                }
            }
            if (s->pict_type == FF_B_TYPE) {
                int b_code = get_bits(&s->gb, 3);
                if(b_code==0){
                    av_log(s->avctx, AV_LOG_ERROR, "Error, video packet header damaged (b_code=0)\n");
                }
            }
        }
    }
    //FIXME new-pred stuff

//printf("parse ok %d %d %d %d\n", mb_num, s->mb_x + s->mb_y*s->mb_width, get_bits_count(gb), get_bits_count(&s->gb));

    return 0;
}

/**
 * decodes the group of blocks / video packet header.
 * @return bit position of the resync_marker, or <0 if none was found
 */
int ff_h263_resync_amba(MpegEncContext *s){
    int left, pos, ret;

    if(s->codec_id==CODEC_ID_AMBA_P_MPEG4){
        skip_bits1(&s->gb);
        align_get_bits(&s->gb);
    }

    if(show_bits(&s->gb, 16)==0){
        pos= get_bits_count(&s->gb);
        if(s->codec_id==CODEC_ID_AMBA_P_MPEG4)
            ret= mpeg4_decode_video_packet_header(s);
        else
            ret= h263_decode_gob_header(s);
        if(ret>=0)
            return pos;
    }
    //OK, it's not where it is supposed to be ...
    s->gb= s->last_resync_gb;
    align_get_bits(&s->gb);
    left= s->gb.size_in_bits - get_bits_count(&s->gb);

    for(;left>16+1+5+5; left-=8){
        if(show_bits(&s->gb, 16)==0){
            GetBitContext bak= s->gb;

            pos= get_bits_count(&s->gb);
            if(s->codec_id==CODEC_ID_AMBA_P_MPEG4)
                ret= mpeg4_decode_video_packet_header(s);
            else
                ret= h263_decode_gob_header(s);
            if(ret>=0)
                return pos;

            s->gb= bak;
        }
        skip_bits(&s->gb, 8);
    }

    return -1;
}

/**
 * gets the average motion vector for a GMC MB.
 * @param n either 0 for the x component or 1 for y
 * @returns the average MV for a GMC MB
 */
static inline int get_amv(MpegEncContext *s, int n){
    int x, y, mb_v, sum, dx, dy, shift;
    int len = 1 << (s->f_code + 4);
    const int a= s->sprite_warping_accuracy;

    if(s->workaround_bugs & FF_BUG_AMV)
        len >>= s->quarter_sample;

    if(s->real_sprite_warping_points==1){
        if(s->divx_version==500 && s->divx_build==413)
            sum= s->sprite_offset[0][n] / (1<<(a - s->quarter_sample));
        else
            sum= RSHIFT(s->sprite_offset[0][n]<<s->quarter_sample, a);
    }else{
        dx= s->sprite_delta[n][0];
        dy= s->sprite_delta[n][1];
        shift= s->sprite_shift[0];
        if(n) dy -= 1<<(shift + a + 1);
        else  dx -= 1<<(shift + a + 1);
        mb_v= s->sprite_offset[0][n] + dx*s->mb_x*16 + dy*s->mb_y*16;

        sum=0;
        for(y=0; y<16; y++){
            int v;

            v= mb_v + dy*y;
            //XXX FIXME optimize
            for(x=0; x<16; x++){
                sum+= v>>shift;
                v+= dx;
            }
        }
        sum= RSHIFT(sum, a+8-s->quarter_sample);
    }

    if      (sum < -len) sum= -len;
    else if (sum >= len) sum= len-1;

    return sum;
}
#if 0
/**
 * decodes first partition.
 * @return number of MBs decoded or <0 if an error occurred
 */
static int mpeg4_decode_partition_a(MpegEncContext *s){
    int mb_num;
    static const int8_t quant_tab[4] = { -1, -2, 1, 2 };

    /* decode first partition */
    mb_num=0;
    s->first_slice_line=1;
    for(; s->mb_y<s->mb_height; s->mb_y++){
        ff_init_block_index(s);
        for(; s->mb_x<s->mb_width; s->mb_x++){
            const int xy= s->mb_x + s->mb_y*s->mb_stride;
            int cbpc;
            int dir=0;

            mb_num++;
            ff_update_block_index(s);
            if(s->mb_x == s->resync_mb_x && s->mb_y == s->resync_mb_y+1)
                s->first_slice_line=0;

            if(s->pict_type==FF_I_TYPE){
                int i;

                do{
                    if(show_bits_long(&s->gb, 19)==DC_MARKER){
                        return mb_num-1;
                    }

                    cbpc = get_vlc2(&s->gb, intra_MCBPC_vlc.table, INTRA_MCBPC_VLC_BITS, 2);
                    if (cbpc < 0){
                        av_log(s->avctx, AV_LOG_ERROR, "cbpc corrupted at %d %d\n", s->mb_x, s->mb_y);
                        return -1;
                    }
                }while(cbpc == 8);

                s->cbp_table[xy]= cbpc & 3;
                s->current_picture_ptr->mb_type[xy]= MB_TYPE_INTRA;
                s->mb_intra = 1;

                if(cbpc & 4) {
                    ff_set_qscale(s, s->qscale + quant_tab[get_bits(&s->gb, 2)]);
                }
                s->current_picture_ptr->qscale_table[xy]= s->qscale;

                s->mbintra_table[xy]= 1;
                for(i=0; i<6; i++){
                    int dc_pred_dir;
                    int dc= mpeg4_decode_dc(s, i, &dc_pred_dir);
                    if(dc < 0){
                        av_log(s->avctx, AV_LOG_ERROR, "DC corrupted at %d %d\n", s->mb_x, s->mb_y);
                        return -1;
                    }
                    dir<<=1;
                    if(dc_pred_dir) dir|=1;
                }
                s->pred_dir_table[xy]= dir;
            }else{ /* P/S_TYPE */
                int mx, my, pred_x, pred_y, bits;
                int16_t * const mot_val= s->current_picture_ptr->motion_val[0][s->block_index[0]];
                const int stride= s->b8_stride*2;

try_again:
                bits= show_bits(&s->gb, 17);
                if(bits==MOTION_MARKER){
                    return mb_num-1;
                }
                skip_bits1(&s->gb);
                if(bits&0x10000){
                    /* skip mb */
                    if(s->pict_type==FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE){
                        s->current_picture_ptr->mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_GMC | MB_TYPE_L0;
                        mx= get_amv(s, 0);
                        my= get_amv(s, 1);
                    }else{
                        s->current_picture_ptr->mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_L0;
                        mx=my=0;
                    }
                    mot_val[0       ]= mot_val[2       ]=
                    mot_val[0+stride]= mot_val[2+stride]= mx;
                    mot_val[1       ]= mot_val[3       ]=
                    mot_val[1+stride]= mot_val[3+stride]= my;

                    if(s->mbintra_table[xy])
                        ff_clean_intra_table_entries(s);
                    continue;
                }

                cbpc = get_vlc2(&s->gb, inter_MCBPC_vlc.table, INTER_MCBPC_VLC_BITS, 2);
                if (cbpc < 0){
                    av_log(s->avctx, AV_LOG_ERROR, "cbpc corrupted at %d %d\n", s->mb_x, s->mb_y);
                    return -1;
                }
                if(cbpc == 20)
                    goto try_again;

                s->cbp_table[xy]= cbpc&(8+3); //8 is dquant

                s->mb_intra = ((cbpc & 4) != 0);

                if(s->mb_intra){
                    s->current_picture_ptr->mb_type[xy]= MB_TYPE_INTRA;
                    s->mbintra_table[xy]= 1;
                    mot_val[0       ]= mot_val[2       ]=
                    mot_val[0+stride]= mot_val[2+stride]= 0;
                    mot_val[1       ]= mot_val[3       ]=
                    mot_val[1+stride]= mot_val[3+stride]= 0;
                }else{
                    if(s->mbintra_table[xy])
                        ff_clean_intra_table_entries(s);

                    if(s->pict_type==FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE && (cbpc & 16) == 0)
                        s->mcsel= get_bits1(&s->gb);
                    else s->mcsel= 0;

                    if ((cbpc & 16) == 0) {
                        /* 16x16 motion prediction */

                        h263_pred_motion_amba(s, 0, 0, &pred_x, &pred_y);
                        if(!s->mcsel){
                            mx = h263_decode_motion(s, pred_x, s->f_code);
                            if (mx >= 0xffff)
                                return -1;

                            my = h263_decode_motion(s, pred_y, s->f_code);
                            if (my >= 0xffff)
                                return -1;
                            s->current_picture_ptr->mb_type[xy]= MB_TYPE_16x16 | MB_TYPE_L0;
                        } else {
                            mx = get_amv(s, 0);
                            my = get_amv(s, 1);
                            s->current_picture_ptr->mb_type[xy]= MB_TYPE_16x16 | MB_TYPE_GMC | MB_TYPE_L0;
                        }

                        mot_val[0       ]= mot_val[2       ] =
                        mot_val[0+stride]= mot_val[2+stride]= mx;
                        mot_val[1       ]= mot_val[3       ]=
                        mot_val[1+stride]= mot_val[3+stride]= my;
                    } else {
                        int i;
                        s->current_picture_ptr->mb_type[xy]= MB_TYPE_8x8 | MB_TYPE_L0;
                        for(i=0;i<4;i++) {
                            int16_t *mot_val= h263_pred_motion_amba(s, i, 0, &pred_x, &pred_y);
                            mx = h263_decode_motion(s, pred_x, s->f_code);
                            if (mx >= 0xffff)
                                return -1;

                            my = h263_decode_motion(s, pred_y, s->f_code);
                            if (my >= 0xffff)
                                return -1;
                            mot_val[0] = mx;
                            mot_val[1] = my;
                        }
                    }
                }
            }
        }
        s->mb_x= 0;
    }

    return mb_num;
}

/**
 * decode second partition.
 * @return <0 if an error occurred
 */
static int mpeg4_decode_partition_b(MpegEncContext *s, int mb_count){
    int mb_num=0;
    static const int8_t quant_tab[4] = { -1, -2, 1, 2 };

    s->mb_x= s->resync_mb_x;
    s->first_slice_line=1;
    for(s->mb_y= s->resync_mb_y; mb_num < mb_count; s->mb_y++){
        ff_init_block_index(s);
        for(; mb_num < mb_count && s->mb_x<s->mb_width; s->mb_x++){
            const int xy= s->mb_x + s->mb_y*s->mb_stride;

            mb_num++;
            ff_update_block_index(s);
            if(s->mb_x == s->resync_mb_x && s->mb_y == s->resync_mb_y+1)
                s->first_slice_line=0;

            if(s->pict_type==FF_I_TYPE){
                int ac_pred= get_bits1(&s->gb);
                int cbpy = get_vlc2(&s->gb, cbpy_vlc.table, CBPY_VLC_BITS, 1);
                if(cbpy<0){
                    av_log(s->avctx, AV_LOG_ERROR, "cbpy corrupted at %d %d\n", s->mb_x, s->mb_y);
                    return -1;
                }

                s->cbp_table[xy]|= cbpy<<2;
                s->current_picture_ptr->mb_type[xy] |= ac_pred*MB_TYPE_ACPRED;
            }else{ /* P || S_TYPE */
                if(IS_INTRA(s->current_picture_ptr->mb_type[xy])){
                    int dir=0,i;
                    int ac_pred = get_bits1(&s->gb);
                    int cbpy = get_vlc2(&s->gb, cbpy_vlc.table, CBPY_VLC_BITS, 1);

                    if(cbpy<0){
                        av_log(s->avctx, AV_LOG_ERROR, "I cbpy corrupted at %d %d\n", s->mb_x, s->mb_y);
                        return -1;
                    }

                    if(s->cbp_table[xy] & 8) {
                        ff_set_qscale(s, s->qscale + quant_tab[get_bits(&s->gb, 2)]);
                    }
                    s->current_picture_ptr->qscale_table[xy]= s->qscale;

                    for(i=0; i<6; i++){
                        int dc_pred_dir;
                        int dc= mpeg4_decode_dc(s, i, &dc_pred_dir);
                        if(dc < 0){
                            av_log(s->avctx, AV_LOG_ERROR, "DC corrupted at %d %d\n", s->mb_x, s->mb_y);
                            return -1;
                        }
                        dir<<=1;
                        if(dc_pred_dir) dir|=1;
                    }
                    s->cbp_table[xy]&= 3; //remove dquant
                    s->cbp_table[xy]|= cbpy<<2;
                    s->current_picture_ptr->mb_type[xy] |= ac_pred*MB_TYPE_ACPRED;
                    s->pred_dir_table[xy]= dir;
                }else if(IS_SKIP(s->current_picture_ptr->mb_type[xy])){
                    s->current_picture_ptr->qscale_table[xy]= s->qscale;
                    s->cbp_table[xy]= 0;
                }else{
                    int cbpy = get_vlc2(&s->gb, cbpy_vlc.table, CBPY_VLC_BITS, 1);

                    if(cbpy<0){
                        av_log(s->avctx, AV_LOG_ERROR, "P cbpy corrupted at %d %d\n", s->mb_x, s->mb_y);
                        return -1;
                    }

                    if(s->cbp_table[xy] & 8) {
                        ff_set_qscale(s, s->qscale + quant_tab[get_bits(&s->gb, 2)]);
                    }
                    s->current_picture_ptr->qscale_table[xy]= s->qscale;

                    s->cbp_table[xy]&= 3; //remove dquant
                    s->cbp_table[xy]|= (cbpy^0xf)<<2;
                }
            }
        }
        if(mb_num >= mb_count) return 0;
        s->mb_x= 0;
    }
    return 0;
}
#endif

/**
 * read the next MVs for OBMC. yes this is a ugly hack, feel free to send a patch :)
 */
static void preview_obmc(MpegEncContext *s){
    GetBitContext gb= s->gb;

    int cbpc, i, pred_x, pred_y, mx, my;
    int16_t *mot_val;
    const int xy= s->mb_x + 1 + s->mb_y * s->mb_stride;
    const int stride= s->b8_stride*2;

    for(i=0; i<4; i++)
        s->block_index[i]+= 2;
    for(i=4; i<6; i++)
        s->block_index[i]+= 1;
    s->mb_x++;

    assert(s->pict_type == FF_P_TYPE);

    do{
        if (get_bits1(&s->gb)) {
            /* skip mb */
            mot_val = s->current_picture_ptr->motion_val[0][ s->block_index[0] ];
            mot_val[0       ]= mot_val[2       ]=
            mot_val[0+stride]= mot_val[2+stride]= 0;
            mot_val[1       ]= mot_val[3       ]=
            mot_val[1+stride]= mot_val[3+stride]= 0;

            s->current_picture_ptr->mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_L0;
            goto end;
        }
        cbpc = get_vlc2(&s->gb, inter_MCBPC_vlc.table, INTER_MCBPC_VLC_BITS, 2);
    }while(cbpc == 20);

    if(cbpc & 4){
        s->current_picture_ptr->mb_type[xy]= MB_TYPE_INTRA;
    }else{
        get_vlc2(&s->gb, cbpy_vlc.table, CBPY_VLC_BITS, 1);
        if (cbpc & 8) {
            if(s->modified_quant){
                if(get_bits1(&s->gb)) skip_bits(&s->gb, 1);
                else                  skip_bits(&s->gb, 5);
            }else
                skip_bits(&s->gb, 2);
        }

        if ((cbpc & 16) == 0) {
                s->current_picture_ptr->mb_type[xy]= MB_TYPE_16x16 | MB_TYPE_L0;
                /* 16x16 motion prediction */
                mot_val= h263_pred_motion_amba(s, 0, 0, &pred_x, &pred_y);
                if (s->umvplus)
                   mx = h263p_decode_umotion(s, pred_x);
                else
                   mx = h263_decode_motion(s, pred_x, 1);

                if (s->umvplus)
                   my = h263p_decode_umotion(s, pred_y);
                else
                   my = h263_decode_motion(s, pred_y, 1);

                mot_val[0       ]= mot_val[2       ]=
                mot_val[0+stride]= mot_val[2+stride]= mx;
                mot_val[1       ]= mot_val[3       ]=
                mot_val[1+stride]= mot_val[3+stride]= my;
        } else {
            s->current_picture_ptr->mb_type[xy]= MB_TYPE_8x8 | MB_TYPE_L0;
            for(i=0;i<4;i++) {
                mot_val = h263_pred_motion_amba(s, i, 0, &pred_x, &pred_y);
                if (s->umvplus)
                  mx = h263p_decode_umotion(s, pred_x);
                else
                  mx = h263_decode_motion(s, pred_x, 1);

                if (s->umvplus)
                  my = h263p_decode_umotion(s, pred_y);
                else
                  my = h263_decode_motion(s, pred_y, 1);
                if (s->umvplus && (mx - pred_x) == 1 && (my - pred_y) == 1)
                  skip_bits1(&s->gb); /* Bit stuffing to prevent PSC */
                mot_val[0] = mx;
                mot_val[1] = my;
            }
        }
    }
end:

    for(i=0; i<4; i++)
        s->block_index[i]-= 2;
    for(i=4; i<6; i++)
        s->block_index[i]-= 1;
    s->mb_x--;

    s->gb= gb;
}

static void h263_decode_dquant(MpegEncContext *s){
    static const int8_t quant_tab[4] = { -1, -2, 1, 2 };

    if(s->modified_quant){
        if(get_bits1(&s->gb))
            s->qscale= modified_quant_tab[get_bits1(&s->gb)][ s->qscale ];
        else
            s->qscale= get_bits(&s->gb, 5);
    }else
        s->qscale += quant_tab[get_bits(&s->gb, 2)];
    ff_set_qscale(s, s->qscale);
}

static int h263_skip_b_part(MpegEncContext *s, int cbp)
{
    DECLARE_ALIGNED(16, DCTELEM, dblock[64]);
    int i, mbi;

    /* we have to set s->mb_intra to zero to decode B-part of PB-frame correctly
     * but real value should be restored in order to be used later (in OBMC condition)
     */
    mbi = s->mb_intra;
    s->mb_intra = 0;
    for (i = 0; i < 6; i++) {
        if (h263_decode_block(s, dblock, i, cbp&32) < 0)
            return -1;
        cbp+=cbp;
    }
    s->mb_intra = mbi;
    return 0;
}

static int h263_get_modb(GetBitContext *gb, int pb_frame, int *cbpb)
{
    int c, mv = 1;

    if (pb_frame < 3) { // h.263 Annex G and i263 PB-frame
        c = get_bits1(gb);
        if (pb_frame == 2 && c)
            mv = !get_bits1(gb);
    } else { // h.263 Annex M improved PB-frame
        mv = get_unary(gb, 0, 4) + 1;
        c = mv & 1;
        mv = !!(mv & 2);
    }
    if(c)
        *cbpb = get_bits(gb, 6);
    return mv;
}

static inline void dct_unquantize_mpeg2_intra(MpegEncContext *s,
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
    #ifdef __tmp_use_amba_permutation__
    quant_matrix = s->p_intra_matrix;
    uint8_t* permutated=s->p_intra_scantable->permutated;
    #else
    quant_matrix = s->intra_matrix;
    #endif
    for(i=1;i<=nCoeffs;i++) {
        #ifdef __tmp_use_amba_permutation__
        int j= permutated[i];
        #else
        int j= s->intra_scantable.permutated[i];
        #endif
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

static inline void dct_unquantize_mpeg2_inter(MpegEncContext *s,
                                   DCTELEM *block, int n, int qscale)
{
    int i, level, nCoeffs;
    const uint16_t *quant_matrix;
    int sum=-1;

    if(s->alternate_scan) nCoeffs= 63;
    else nCoeffs= s->block_last_index[n];
    #ifdef __tmp_use_amba_permutation__
    quant_matrix = s->p_inter_matrix;
    uint8_t* permutated=s->p_intra_scantable->permutated;
    #else
    quant_matrix = s->inter_matrix;
    #endif
    for(i=0; i<=nCoeffs; i++) {
        #ifdef __tmp_use_amba_permutation__
        int j= permutated[i];
        #else
        int j= s->intra_scantable.permutated[i];
        #endif
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

static inline void dct_unquantize_h263_intra(MpegEncContext *s,
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
    {
        #ifdef __tmp_use_amba_permutation__
        nCoeffs= s->p_inter_scantable->raster_end[ s->block_last_index[n] ];
        #else
        nCoeffs= s->inter_scantable.raster_end[ s->block_last_index[n] ];
        #endif
    }

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

static void dct_unquantize_h263_inter(MpegEncContext *s,
                                  DCTELEM *block, int n, int qscale)
{
    int i, level, qmul, qadd;
    int nCoeffs;

    assert(s->block_last_index[n]>=0);

    qadd = (qscale - 1) | 1;
    qmul = qscale << 1;

    #ifdef __tmp_use_amba_permutation__
    nCoeffs= s->p_inter_scantable->raster_end[ s->block_last_index[n] ];
    #else
    nCoeffs= s->inter_scantable.raster_end[ s->block_last_index[n] ];
    #endif

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
 * decodes a intra block and dequant
 * @return <0 if an error occurred
 */

/**
 * decodes a intra block and dequant
 * @return <0 if an error occurred
 */
static inline int mpeg4_decode_block_intra_dequant(MpegEncContext * s, DCTELEM * block, int n, int coded)//, int rvlc)
{
    int level, i, last, run;
    int dc_pred_dir;
    RLTable * rl;
    RL_VLC_ELEM * rl_vlc;
    const uint8_t * scan_table;

    //Note intra & rvlc should be optimized away if this is inlined

      if(s->use_intra_dc_vlc){
        /* DC coef */
        if(s->partitioned_frame){
            level = s->dc_val[0][ s->block_index[n] ];
            if(n<4) level= FASTDIV((level + (s->y_dc_scale>>1)), s->y_dc_scale);
            else    level= FASTDIV((level + (s->c_dc_scale>>1)), s->c_dc_scale);
            dc_pred_dir= (s->pred_dir_table[s->mb_x + s->mb_y*s->mb_stride]<<n)&32;
        }else{
            level = mpeg4_decode_dc(s, n, &dc_pred_dir);
            if (level < 0)
                return -1;
        }
        block[0] = level;
        i = 0;
      }else{
            i = -1;
            ff_mpeg4_pred_dc(s, n, 0, &dc_pred_dir, 0);
      }
      if (!coded)
          goto not_coded;

/*      if(rvlc){
          rl = &rvlc_rl_intra;
          rl_vlc = rvlc_rl_intra.rl_vlc[0];
      }else*/{
          rl = &ff_mpeg4_rl_intra;
          rl_vlc = ff_mpeg4_rl_intra.rl_vlc[0];
      }

    #ifdef __tmp_use_amba_permutation__
        if (s->ac_pred) {
            if (dc_pred_dir == 0)
                scan_table = s->p_intra_v_scantable->permutated; /* left */
            else
                scan_table = s->p_intra_h_scantable->permutated; /* top */
        } else {
            scan_table = s->p_intra_scantable->permutated;
        }
    #else
        if (s->ac_pred) {
            if (dc_pred_dir == 0)
                scan_table = s->intra_v_scantable.permutated; /* left */
            else
                scan_table = s->intra_h_scantable.permutated; /* top */
        } else {
            scan_table = s->intra_scantable.permutated;
        }
    #endif


  {
    OPEN_READER(re, &s->gb);
    for(;;) {
        UPDATE_CACHE(re, &s->gb);
        GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 0);
        if (level==0) {
          /* escape */
/*          if(rvlc){
                if(SHOW_UBITS(re, &s->gb, 1)==0){
                    av_log(s->avctx, AV_LOG_ERROR, "1. marker bit missing in rvlc esc\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 1);

                last=  SHOW_UBITS(re, &s->gb, 1); SKIP_CACHE(re, &s->gb, 1);
                run=   SHOW_UBITS(re, &s->gb, 6); LAST_SKIP_CACHE(re, &s->gb, 6);
                SKIP_COUNTER(re, &s->gb, 1+1+6);
                UPDATE_CACHE(re, &s->gb);

                if(SHOW_UBITS(re, &s->gb, 1)==0){
                    av_log(s->avctx, AV_LOG_ERROR, "2. marker bit missing in rvlc esc\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 1);

                level= SHOW_UBITS(re, &s->gb, 11); SKIP_CACHE(re, &s->gb, 11);

                if(SHOW_UBITS(re, &s->gb, 5)!=0x10){
                    av_log(s->avctx, AV_LOG_ERROR, "reverse esc missing\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 5);

                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1); LAST_SKIP_CACHE(re, &s->gb, 1);
                SKIP_COUNTER(re, &s->gb, 1+11+5+1);

                i+= run + 1;
                if(last) i+=192;
          }else*/{
            int cache;
            cache= GET_CACHE(re, &s->gb);

            if(IS_3IV1)
                cache ^= 0xC0000000;

            if (cache&0x80000000) {
                if (cache&0x40000000) {
                    /* third escape */
                    SKIP_CACHE(re, &s->gb, 2);
                    last=  SHOW_UBITS(re, &s->gb, 1); SKIP_CACHE(re, &s->gb, 1);
                    run=   SHOW_UBITS(re, &s->gb, 6); LAST_SKIP_CACHE(re, &s->gb, 6);
                    SKIP_COUNTER(re, &s->gb, 2+1+6);
                    UPDATE_CACHE(re, &s->gb);

                    if(IS_3IV1){
                        level= SHOW_SBITS(re, &s->gb, 12); LAST_SKIP_BITS(re, &s->gb, 12);
                    }else{
                        if(SHOW_UBITS(re, &s->gb, 1)==0){
                            av_log(s->avctx, AV_LOG_ERROR, "1. marker bit missing in 3. esc\n");
                            return -1;
                        }; SKIP_CACHE(re, &s->gb, 1);

                        level= SHOW_SBITS(re, &s->gb, 12); SKIP_CACHE(re, &s->gb, 12);

                        if(SHOW_UBITS(re, &s->gb, 1)==0){
                            av_log(s->avctx, AV_LOG_ERROR, "2. marker bit missing in 3. esc\n");
                            return -1;
                        }; LAST_SKIP_CACHE(re, &s->gb, 1);

                        SKIP_COUNTER(re, &s->gb, 1+12+1);
                    }

                    if((unsigned)(level + 2048) > 4095){
                        if(s->error_recognition > FF_ER_COMPLIANT){
                            if(level > 2560 || level<-2560){
                                av_log(s->avctx, AV_LOG_ERROR, "|level| overflow in 3. esc, qp=%d\n", s->qscale);
                                return -1;
                            }
                        }
                        level= level<0 ? -2048 : 2047;
                    }

                    i+= run + 1;
                    if(last) i+=192;
                } else {
                    /* second escape */
#if MIN_CACHE_BITS < 20
                    LAST_SKIP_BITS(re, &s->gb, 2);
                    UPDATE_CACHE(re, &s->gb);
#else
                    SKIP_BITS(re, &s->gb, 2);
#endif
                    GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 1);
                    i+= run + rl->max_run[run>>7][level] +1; //FIXME opt indexing
                    level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                    LAST_SKIP_BITS(re, &s->gb, 1);
                }
            } else {
                /* first escape */
#if MIN_CACHE_BITS < 19
                LAST_SKIP_BITS(re, &s->gb, 1);
                UPDATE_CACHE(re, &s->gb);
#else
                SKIP_BITS(re, &s->gb, 1);
#endif
                GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 1);
                i+= run;
                level = level + rl->max_level[run>>7][(run-1)&63] ;
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                LAST_SKIP_BITS(re, &s->gb, 1);
            }
          }
        } else {
            i+= run;
            level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
            LAST_SKIP_BITS(re, &s->gb, 1);
        }
        if (i > 62){
            i-= 192;
            if(i&(~63)){
                av_log(s->avctx, AV_LOG_ERROR, "ac-tex damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }

            block[scan_table[i]] = level;
            break;
        }

        block[scan_table[i]] = level;
    }
    CLOSE_READER(re, &s->gb);
  }

 not_coded:

#ifdef __dump_DSP_TXT__
    memcpy(s->logdct[n],block,128);
#endif


    if(!s->use_intra_dc_vlc){
        block[0] = ff_mpeg4_pred_dc(s, n, block[0], &dc_pred_dir, 0);

        i -= i>>31; //if(i == -1) i=0;
    }

    mpeg4_pred_ac_amba(s, block, n, dc_pred_dir);
    if (s->ac_pred) {
        i = 63; /* XXX: not optimal */
    }

    s->block_last_index[n] = i;
    //pinfo->block_last_index[n] =i;

#ifdef __dump_DSP_TXT__
    memcpy(s->logdct_acdc[n],block,128);
#endif
    if(n<4)
        i=s->qscale;
    else
        i=s->chroma_qscale;
    //dequant
    if(s->mpeg_quant)
    {
        dct_unquantize_mpeg2_intra(s,block,n,i);
    }
    else
    {
        dct_unquantize_h263_intra(s,block,n,i);
    }

    return 0;
}

//mpeg4 dequant method 0(quant_type=0), no dequant matrix
static inline int mpeg4_decode_block_inter_dequant_0(MpegEncContext * s, DCTELEM * block,int n, int coded)//, int rvlc)
{
    int level, i, last, run;
//    int dc_pred_dir;
    RLTable * rl;
    RL_VLC_ELEM * rl_vlc;
    const uint8_t * scan_table;
    int qmul= s->qscale << 1, qadd= (s->qscale - 1) | 1;

    //Note intra & rvlc should be optimized away if this is inlined
    i = -1;
    if (!coded) {
        s->block_last_index[n] = i;
        //pinfo->block_last_index[n] = i;
        return 0;
    }
    /*if(rvlc) rl = &rvlc_rl_inter;
    else     */rl = &ff_h263_rl_inter;

    #ifdef __tmp_use_amba_permutation__
    scan_table = s->p_intra_scantable->permutated;
    #else
    scan_table = s->intra_scantable.permutated;
    #endif

/*    if(rvlc){
        rl_vlc = rvlc_rl_inter.rl_vlc[s->qscale];
    }else*/{
        rl_vlc = ff_h263_rl_inter.rl_vlc[s->qscale];
    }

  {
    OPEN_READER(re, &s->gb);
    for(;;) {
        UPDATE_CACHE(re, &s->gb);
        GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 0);
        if (level==0) {
          /* escape */
/*          if(rvlc){
                if(SHOW_UBITS(re, &s->gb, 1)==0){
                    av_log(s->avctx, AV_LOG_ERROR, "1. marker bit missing in rvlc esc\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 1);

                last=  SHOW_UBITS(re, &s->gb, 1); SKIP_CACHE(re, &s->gb, 1);
                run=   SHOW_UBITS(re, &s->gb, 6); LAST_SKIP_CACHE(re, &s->gb, 6);
                SKIP_COUNTER(re, &s->gb, 1+1+6);
                UPDATE_CACHE(re, &s->gb);

                if(SHOW_UBITS(re, &s->gb, 1)==0){
                    av_log(s->avctx, AV_LOG_ERROR, "2. marker bit missing in rvlc esc\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 1);

                level= SHOW_UBITS(re, &s->gb, 11); SKIP_CACHE(re, &s->gb, 11);

                if(SHOW_UBITS(re, &s->gb, 5)!=0x10){
                    av_log(s->avctx, AV_LOG_ERROR, "reverse esc missing\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 5);

                level=  level * qmul + qadd;
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1); LAST_SKIP_CACHE(re, &s->gb, 1);
                SKIP_COUNTER(re, &s->gb, 1+11+5+1);

                i+= run + 1;
                if(last) i+=192;
          }else*/{
            int cache;
            cache= GET_CACHE(re, &s->gb);

            if(IS_3IV1)
                cache ^= 0xC0000000;

            if (cache&0x80000000) {
                if (cache&0x40000000) {
                    /* third escape */
                    SKIP_CACHE(re, &s->gb, 2);
                    last=  SHOW_UBITS(re, &s->gb, 1); SKIP_CACHE(re, &s->gb, 1);
                    run=   SHOW_UBITS(re, &s->gb, 6); LAST_SKIP_CACHE(re, &s->gb, 6);
                    SKIP_COUNTER(re, &s->gb, 2+1+6);
                    UPDATE_CACHE(re, &s->gb);

                    if(IS_3IV1){
                        level= SHOW_SBITS(re, &s->gb, 12); LAST_SKIP_BITS(re, &s->gb, 12);
                    }else{
                        if(SHOW_UBITS(re, &s->gb, 1)==0){
                            av_log(s->avctx, AV_LOG_ERROR, "1. marker bit missing in 3. esc\n");
                            return -1;
                        }; SKIP_CACHE(re, &s->gb, 1);

                        level= SHOW_SBITS(re, &s->gb, 12); SKIP_CACHE(re, &s->gb, 12);

                        if(SHOW_UBITS(re, &s->gb, 1)==0){
                            av_log(s->avctx, AV_LOG_ERROR, "2. marker bit missing in 3. esc\n");
                            return -1;
                        }; LAST_SKIP_CACHE(re, &s->gb, 1);

                        SKIP_COUNTER(re, &s->gb, 1+12+1);
                    }

                    if (level>0) level= level * qmul + qadd;
                    else         level= level * qmul - qadd;

                    if((unsigned)(level + 2048) > 4095){
                        if(s->error_recognition > FF_ER_COMPLIANT){
                            if(level > 2560 || level<-2560){
                                av_log(s->avctx, AV_LOG_ERROR, "|level| overflow in 3. esc, qp=%d\n", s->qscale);
                                return -1;
                            }
                        }
                        level= level<0 ? -2048 : 2047;
                    }

                    i+= run + 1;
                    if(last) i+=192;
                } else {
                    /* second escape */
#if MIN_CACHE_BITS < 20
                    LAST_SKIP_BITS(re, &s->gb, 2);
                    UPDATE_CACHE(re, &s->gb);
#else
                    SKIP_BITS(re, &s->gb, 2);
#endif
                    GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 1);
                    i+= run + rl->max_run[run>>7][level/qmul] +1; //FIXME opt indexing
                    level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                    LAST_SKIP_BITS(re, &s->gb, 1);
                }
            } else {
                /* first escape */
#if MIN_CACHE_BITS < 19
                LAST_SKIP_BITS(re, &s->gb, 1);
                UPDATE_CACHE(re, &s->gb);
#else
                SKIP_BITS(re, &s->gb, 1);
#endif
                GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 1);
                i+= run;
                level = level + rl->max_level[run>>7][(run-1)&63] * qmul;//FIXME opt indexing
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                LAST_SKIP_BITS(re, &s->gb, 1);
            }
          }
        } else {
            i+= run;
            level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
            LAST_SKIP_BITS(re, &s->gb, 1);
        }
        if (i > 62){
            i-= 192;
            if(i&(~63)){
                av_log(s->avctx, AV_LOG_ERROR, "ac-tex damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }

            block[scan_table[i]] = level;
            break;
        }

        block[scan_table[i]] = level;
    }
    CLOSE_READER(re, &s->gb);
  }

// not_coded:

#ifdef __dump_DSP_TXT__
    memcpy(s->logdct[n],block,128);
#endif

    s->block_last_index[n] = i;
    //pinfo->block_last_index[n] = i;
#ifdef __dump_DSP_TXT__
    memcpy(s->logdct_acdc[n],block,128);
#endif
    return 0;
}

//mpeg4 dequant method 1(quant_type=1), have dequant matrix
static inline int mpeg4_decode_block_inter_dequant_1(MpegEncContext * s, DCTELEM * block,int n, int coded)//, int rvlc)
{
    int level, i, last, run;
//    int dc_pred_dir;
    RLTable * rl;
    RL_VLC_ELEM * rl_vlc;
    const uint8_t * scan_table;

    i = -1;
    if (!coded) {
        s->block_last_index[n] = i;
        //pinfo->block_last_index[n] = i;
        return 0;
    }
/*    if(rvlc) rl = &rvlc_rl_inter;
    else     */rl = &ff_h263_rl_inter;
    #ifdef __tmp_use_amba_permutation__
    scan_table = s->p_intra_scantable->permutated;
    #else
    scan_table = s->intra_scantable.permutated;
    #endif

//    if(s->mpeg_quant){
/*        if(rvlc){
            rl_vlc = rvlc_rl_inter.rl_vlc[0];
        }else*/{
            rl_vlc = ff_h263_rl_inter.rl_vlc[0];
        }
//    }

  {
    OPEN_READER(re, &s->gb);
    for(;;) {
        UPDATE_CACHE(re, &s->gb);
        GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 0);
        if (level==0) {
          /* escape */
/*          if(rvlc){
                if(SHOW_UBITS(re, &s->gb, 1)==0){
                    av_log(s->avctx, AV_LOG_ERROR, "1. marker bit missing in rvlc esc\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 1);

                last=  SHOW_UBITS(re, &s->gb, 1); SKIP_CACHE(re, &s->gb, 1);
                run=   SHOW_UBITS(re, &s->gb, 6); LAST_SKIP_CACHE(re, &s->gb, 6);
                SKIP_COUNTER(re, &s->gb, 1+1+6);
                UPDATE_CACHE(re, &s->gb);

                if(SHOW_UBITS(re, &s->gb, 1)==0){
                    av_log(s->avctx, AV_LOG_ERROR, "2. marker bit missing in rvlc esc\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 1);

                level= SHOW_UBITS(re, &s->gb, 11); SKIP_CACHE(re, &s->gb, 11);

                if(SHOW_UBITS(re, &s->gb, 5)!=0x10){
                    av_log(s->avctx, AV_LOG_ERROR, "reverse esc missing\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 5);

                level=  level;
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1); LAST_SKIP_CACHE(re, &s->gb, 1);
                SKIP_COUNTER(re, &s->gb, 1+11+5+1);

                i+= run + 1;
                if(last) i+=192;
          }else*/{
            int cache;
            cache= GET_CACHE(re, &s->gb);

            if(IS_3IV1)
                cache ^= 0xC0000000;

            if (cache&0x80000000) {
                if (cache&0x40000000) {
                    /* third escape */
                    SKIP_CACHE(re, &s->gb, 2);
                    last=  SHOW_UBITS(re, &s->gb, 1); SKIP_CACHE(re, &s->gb, 1);
                    run=   SHOW_UBITS(re, &s->gb, 6); LAST_SKIP_CACHE(re, &s->gb, 6);
                    SKIP_COUNTER(re, &s->gb, 2+1+6);
                    UPDATE_CACHE(re, &s->gb);

                    if(IS_3IV1){
                        level= SHOW_SBITS(re, &s->gb, 12); LAST_SKIP_BITS(re, &s->gb, 12);
                    }else{
                        if(SHOW_UBITS(re, &s->gb, 1)==0){
                            av_log(s->avctx, AV_LOG_ERROR, "1. marker bit missing in 3. esc\n");
                            return -1;
                        }; SKIP_CACHE(re, &s->gb, 1);

                        level= SHOW_SBITS(re, &s->gb, 12); SKIP_CACHE(re, &s->gb, 12);

                        if(SHOW_UBITS(re, &s->gb, 1)==0){
                            av_log(s->avctx, AV_LOG_ERROR, "2. marker bit missing in 3. esc\n");
                            return -1;
                        }; LAST_SKIP_CACHE(re, &s->gb, 1);

                        SKIP_COUNTER(re, &s->gb, 1+12+1);
                    }

                    if((unsigned)(level + 2048) > 4095){
                        if(s->error_recognition > FF_ER_COMPLIANT){
                            if(level > 2560 || level<-2560){
                                av_log(s->avctx, AV_LOG_ERROR, "|level| overflow in 3. esc, qp=%d\n", s->qscale);
                                return -1;
                            }
                        }
                        level= level<0 ? -2048 : 2047;
                    }

                    i+= run + 1;
                    if(last) i+=192;
                } else {
                    /* second escape */
#if MIN_CACHE_BITS < 20
                    LAST_SKIP_BITS(re, &s->gb, 2);
                    UPDATE_CACHE(re, &s->gb);
#else
                    SKIP_BITS(re, &s->gb, 2);
#endif
                    GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 1);
                    i+= run + rl->max_run[run>>7][level] +1; //FIXME opt indexing
                    level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                    LAST_SKIP_BITS(re, &s->gb, 1);
                }
            } else {
                /* first escape */
#if MIN_CACHE_BITS < 19
                LAST_SKIP_BITS(re, &s->gb, 1);
                UPDATE_CACHE(re, &s->gb);
#else
                SKIP_BITS(re, &s->gb, 1);
#endif
                GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 1);
                i+= run;
                level = level + rl->max_level[run>>7][(run-1)&63] ;//FIXME opt indexing
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                LAST_SKIP_BITS(re, &s->gb, 1);
            }
          }
        } else {
            i+= run;
            level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
            LAST_SKIP_BITS(re, &s->gb, 1);
        }
        if (i > 62){
            i-= 192;
            if(i&(~63)){
                av_log(s->avctx, AV_LOG_ERROR, "ac-tex damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }

            block[scan_table[i]] = level;
            break;
        }

        block[scan_table[i]] = level;
    }
    CLOSE_READER(re, &s->gb);
  }

// not_coded:

#ifdef __dump_DSP_TXT__
    memcpy(s->logdct[n],block,128);
#endif

    s->block_last_index[n] = i;
    //pinfo->block_last_index[n] = i;
#ifdef __dump_DSP_TXT__
    memcpy(s->logdct_acdc[n],block,128);
#endif

    dct_unquantize_mpeg2_inter(s,block,n,s->qscale);
    return 0;
}


static int h263_decode_block_intra_dequant(MpegEncContext * s, DCTELEM * block, int n, int coded)
{
    int code, level, i, j, last, run;
    RLTable *rl = &ff_h263_rl_inter;
    const uint8_t *scan_table;
    GetBitContext gb= s->gb;

    #ifdef __tmp_use_amba_permutation__
    scan_table = s->p_intra_scantable->permutated;
    #else
    scan_table = s->intra_scantable.permutated;
    #endif
    if (s->h263_aic ) {
        rl = &rl_intra_aic;
        i = 0;
        if (s->ac_pred) {
            #ifdef __tmp_use_amba_permutation__
            if (s->h263_aic_dir)
                scan_table = s->p_intra_v_scantable->permutated; /* left */
            else
                scan_table = s->p_intra_h_scantable->permutated; /* top */
            #else
            if (s->h263_aic_dir)
                scan_table = s->intra_v_scantable.permutated; /* left */
            else
                scan_table = s->intra_h_scantable.permutated; /* top */
            #endif
        }
    } else
    {
        /* DC coef */
        level = get_bits(&s->gb, 8);
        if((level&0x7F) == 0){
            av_log(s->avctx, AV_LOG_ERROR, "illegal dc %d at %d %d\n", level, s->mb_x, s->mb_y);
            if(s->error_recognition >= FF_ER_COMPLIANT)
                return -1;
        }
        if (level == 255)
            level = 128;
        block[0] = level;
        i = 1;
    }

    if (!coded) {
        if (s->h263_aic)
            goto not_coded;
        s->block_last_index[n] = i - 1;
        if(n<4)
            block[0]*=s->y_dc_scale;
        else
            block[0]*=s->c_dc_scale;
        return 0;
    }
retry:
    for(;;) {
        code = get_vlc2(&s->gb, rl->vlc.table, TEX_VLC_BITS, 2);
        if (code < 0){
            av_log(s->avctx, AV_LOG_ERROR, "illegal ac vlc code at %dx%d\n", s->mb_x, s->mb_y);
            return -1;
        }
        if (code == rl->n) {
            /* escape */
            if (s->h263_flv > 1) {
                int is11 = get_bits1(&s->gb);
                last = get_bits1(&s->gb);
                run = get_bits(&s->gb, 6);
                if(is11){
                    level = get_sbits(&s->gb, 11);
                } else {
                    level = get_sbits(&s->gb, 7);
                }
            } else {
                last = get_bits1(&s->gb);
                run = get_bits(&s->gb, 6);
                level = (int8_t)get_bits(&s->gb, 8);
                if(level == -128){
                    if (s->codec_id == CODEC_ID_RV10) {
                        /* XXX: should patch encoder too */
                        level = get_sbits(&s->gb, 12);
                    }else{
                        level = get_bits(&s->gb, 5);
                        level |= get_sbits(&s->gb, 6)<<5;
                    }
                }
            }
        } else {
            run = rl->table_run[code];
            level = rl->table_level[code];
            last = code >= rl->last;
            if (get_bits1(&s->gb))
                level = -level;
        }
        i += run;
        if (i >= 64){
            if(s->alt_inter_vlc && rl == &ff_h263_rl_inter && !s->mb_intra){
                //Looks like a hack but no, it's the way it is supposed to work ...
                rl = &rl_intra_aic;
                i = 0;
                s->gb= gb;
                s->dsp.clear_block(block);
                goto retry;
            }
            av_log(s->avctx, AV_LOG_ERROR, "run overflow at %dx%d i:%d\n", s->mb_x, s->mb_y, s->mb_intra);
            return -1;
        }
        j = scan_table[i];
        block[j] = level;
        if (last)
            break;
        i++;
    }
not_coded:
    if (s->mb_intra && s->h263_aic) {
        h263_pred_acdc(s, block, n);
        i = 63;
    }
    s->block_last_index[n] = i;

    if(n<4)
        i=s->qscale;
    else
        i=s->chroma_qscale;

    ambadec_assert_ffmpeg(!s->mpeg_quant);
    //need refine
    if(!s->mpeg_quant)
    {
        dct_unquantize_h263_intra(s,block,n,i);
    }
    else
    {
        dct_unquantize_mpeg2_intra(s,block,n,i);
    }

    return 0;
}

static int h263_decode_block_inter_dequant(MpegEncContext * s, DCTELEM * block,
                             int n, int coded)
{
    int code, level, i=0, j, last, run;
    RLTable *rl = &ff_h263_rl_inter;
    const uint8_t *scan_table;
    GetBitContext gb= s->gb;

    #ifdef __tmp_use_amba_permutation__
    scan_table = s->p_intra_scantable->permutated;
    #else
    scan_table = s->intra_scantable.permutated;
    #endif

    if (!coded) {
        s->block_last_index[n] = i - 1;
/*        if(n<4)
            block[0]*=s->qscale;
        else
            block[0]*=s->chroma_qscale;*/
        return 0;
    }

retry:
    for(;;) {
        code = get_vlc2(&s->gb, rl->vlc.table, TEX_VLC_BITS, 2);
        if (code < 0){
            av_log(s->avctx, AV_LOG_ERROR, "illegal ac vlc code at %dx%d\n", s->mb_x, s->mb_y);
            return -1;
        }
        if (code == rl->n) {
            /* escape */
            if (s->h263_flv > 1) {
                int is11 = get_bits1(&s->gb);
                last = get_bits1(&s->gb);
                run = get_bits(&s->gb, 6);
                if(is11){
                    level = get_sbits(&s->gb, 11);
                } else {
                    level = get_sbits(&s->gb, 7);
                }
            } else {
                last = get_bits1(&s->gb);
                run = get_bits(&s->gb, 6);
                level = (int8_t)get_bits(&s->gb, 8);
                if(level == -128){
                    if (s->codec_id == CODEC_ID_RV10) {
                        /* XXX: should patch encoder too */
                        level = get_sbits(&s->gb, 12);
                    }else{
                        level = get_bits(&s->gb, 5);
                        level |= get_sbits(&s->gb, 6)<<5;
                    }
                }
            }
        } else {
            run = rl->table_run[code];
            level = rl->table_level[code];
            last = code >= rl->last;
            if (get_bits1(&s->gb))
                level = -level;
        }
        i += run;
        if (i >= 64){
            if(s->alt_inter_vlc && rl == &ff_h263_rl_inter && !s->mb_intra){
                //Looks like a hack but no, it's the way it is supposed to work ...
                rl = &rl_intra_aic;
                i = 0;
                s->gb= gb;
                s->dsp.clear_block(block);
                goto retry;
            }
            av_log(s->avctx, AV_LOG_ERROR, "run overflow at %dx%d i:%d\n", s->mb_x, s->mb_y, s->mb_intra);
            return -1;
        }
        j = scan_table[i];
        block[j] = level;
        if (last)
            break;
        i++;
    }

    if (s->mb_intra && s->h263_aic) {
        h263_pred_acdc(s, block, n);
        i = 63;
    }
    s->block_last_index[n] = i;

    if(n<4)
        i=s->qscale;
    else
        i=s->chroma_qscale;

    ambadec_assert_ffmpeg(!s->mpeg_quant);
    //need refine
    if(!s->mpeg_quant)
    {
        dct_unquantize_h263_inter(s,block,n,i);
    }
    else
    {
        dct_unquantize_mpeg2_inter(s,block,n,i);
    }

    return 0;
}

int ff_h263_decode_mb_amba(MpegEncContext *s,
                      DCTELEM block[6][64])
{
    int mx, my;
    int cbpc, cbpy, i, cbp, pred_x, pred_y, dquant;
    const int stride= s->b8_stride;
//    const int blockindex=s->block_index[0];
//    int* pcpdes, pcpval;

    int16_t *mot_val;
    const int xy= s->mb_x + s->mb_y * s->mb_stride;
    int cbpb = 0, pb_mv_count = 0;
    H263AmbaDecContext_t* thiz=(H263AmbaDecContext_t*)s;
    h263_pic_swinfo_t* pinfo=thiz->p_pic->pswinfo+s->mb_x + s->mb_y*s->mb_width;
    assert(!s->h263_pred);

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
    ambadec_assert_ffmpeg(((void*)pmvp)==((void*)pmvb));
    pmvp->mb_setting.slice_start=1;

//#ifdef __dump_DSP_test_data__
    if(s->pict_type != FF_I_TYPE)
    {
        pmvp->mb_setting.frame_left=!s->mb_x;
        pmvp->mb_setting.frame_top=!s->mb_y;
    }
//#endif

    if (s->pict_type == FF_P_TYPE) {
        do{
            if (get_bits1(&s->gb)) {
                /* skip mb */
                s->mb_intra = 0;
                for(i=0;i<6;i++)
                {
                    s->block_last_index[i] = -1;
                }
                s->mv_dir = MV_DIR_FORWARD;
                s->mv_type = MV_TYPE_16X16;
                s->current_picture_ptr->mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_L0;
                //s->mv[0][0][0] = 0;
                //s->mv[0][0][1] = 0;
                //pmvp->mv[0].valid=1;
                pmvp->mv[0].ref_frame_num=forward_ref_num;
                //calculate other mvs
                /*pcpdes=(int*)pmvp->mv;
                pcpval=*pcpdes++;
                *pcpdes++=pcpval;
                *pcpdes++=pcpval;
                *pcpdes++=pcpval;
                *pcpdes=pcpval;*/
                //pmvp->mv[1].valid=1;
                //pmvp->mv[2].valid=1;
                //pmvp->mv[3].valid=1;
                //pmvp->mv[4].valid=1;

                s->mb_skipped = !(s->obmc | s->loop_filter);
                goto end;
            }
            cbpc = get_vlc2(&s->gb, inter_MCBPC_vlc.table, INTER_MCBPC_VLC_BITS, 2);
            //fprintf(stderr, "\tCBPC: %d", cbpc);
            if (cbpc < 0){
                av_log(s->avctx, AV_LOG_ERROR, "cbpc damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }
        }while(cbpc == 20);

//        s->dsp.clear_blocks(s->block[0]);

        dquant = cbpc & 8;
        s->mb_intra = ((cbpc & 4) != 0);
        if (s->mb_intra)
        {
            set_intra_p(pmvp);
            goto intra;
        }

        if(s->pb_frame && get_bits1(&s->gb))
            pb_mv_count = h263_get_modb(&s->gb, s->pb_frame, &cbpb);
        cbpy = get_vlc2(&s->gb, cbpy_vlc.table, CBPY_VLC_BITS, 1);

        if(s->alt_inter_vlc==0 || (cbpc & 3)!=3)
            cbpy ^= 0xF;

        cbp = (cbpc & 3) | (cbpy << 2);
        if (dquant) {
            h263_decode_dquant(s);
        }

        s->mv_dir = MV_DIR_FORWARD;
        if ((cbpc & 16) == 0) {
            s->current_picture_ptr->mb_type[xy]= MB_TYPE_16x16 | MB_TYPE_L0;
            /* 16x16 motion prediction */
            s->mv_type = MV_TYPE_16X16;
            h263_pred_motion_amba(s, 0, 0, &pred_x, &pred_y);
            if (s->umvplus)
               mx= h263p_decode_umotion(s, pred_x);
            else
               mx = h263_decode_motion(s, pred_x, 1);

            if (mx >= 0xffff)
                return -1;

            if (s->umvplus)
               my = h263p_decode_umotion(s, pred_y);
            else
               my = h263_decode_motion(s, pred_y, 1);

            if (my >= 0xffff)
                return -1;
            //s->mv[0][0][0] = mx;
            //s->mv[0][0][1] = my;

            if (s->umvplus && (mx - pred_x) == 1 && (my - pred_y) == 1)
               skip_bits1(&s->gb); /* Bit stuffing to prevent PSC */

            pmvp->mv[0].mv_x=mx;
            pmvp->mv[0].mv_y=my;
            //pmvp->mv[0].valid=1;
            pmvp->mv[0].ref_frame_num=forward_ref_num;
            //calculate other mvs
/*            pcpdes=(int*)pmvp->mv;
            pcpval=*pcpdes++;
            *pcpdes++=pcpval;
            *pcpdes++=pcpval;
            *pcpdes=pcpval;*/
            //need calculate chroma mv here
            //todo
        } else {
            s->current_picture_ptr->mb_type[xy]= MB_TYPE_8x8 | MB_TYPE_L0;
            s->mv_type = MV_TYPE_8X8;
            pmvp->mb_setting.mv_type=mv_type_8x8;
            for(i=0;i<4;i++) {
                mot_val = h263_pred_motion_amba(s, i, 0, &pred_x, &pred_y);
                if (s->umvplus)
                  mx= h263p_decode_umotion(s, pred_x);
                else
                  mx = h263_decode_motion(s, pred_x, 1);
                if (mx >= 0xffff)
                    return -1;

                if (s->umvplus)
                  my = h263p_decode_umotion(s, pred_y);
                else
                  my = h263_decode_motion(s, pred_y, 1);
                if (my >= 0xffff)
                    return -1;
                //s->mv[0][i][0] = mx;
                //s->mv[0][i][1] = my;
                if (s->umvplus && (mx - pred_x) == 1 && (my - pred_y) == 1)
                  skip_bits1(&s->gb); /* Bit stuffing to prevent PSC */
                mot_val[0] = mx;
                mot_val[1] = my;

                pmvp->mv[i].mv_x=mx;
                pmvp->mv[i].mv_y=my;
                //pmvp->mv[i].valid=1;
                pmvp->mv[i].ref_frame_num=forward_ref_num;
            }
            //need calculate chroma mv here
            //todo

        }
    } else if(s->pict_type==FF_B_TYPE) {

        int mb_type;

        do{
            mb_type= get_vlc2(&s->gb, h263_mbtype_b_vlc.table, H263_MBTYPE_B_VLC_BITS, 2);
            if (mb_type < 0){
                av_log(s->avctx, AV_LOG_ERROR, "b mb_type damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }

            mb_type= h263_mb_type_b_map[ mb_type ];
        }while(!mb_type);

        s->mb_intra = IS_INTRA(mb_type);
        if(HAS_CBP(mb_type)){
//            s->dsp.clear_blocks(s->block[0]);
            cbpc = get_vlc2(&s->gb, cbpc_b_vlc.table, CBPC_B_VLC_BITS, 1);
            if(s->mb_intra){
                dquant = IS_QUANT(mb_type);
                mot_val = s->current_picture_ptr->motion_val[0][ 2*(s->mb_x + s->mb_y*stride) ];
                int16_t *mot_val1 = s->current_picture_ptr->motion_val[1][ 2*(s->mb_x + s->mb_y*stride) ];
//                const int mv_xy= s->mb_x + 1 + s->mb_y * s->mb_stride;

                //FIXME ugly
                //try remove
               mot_val[0       ]= mot_val[2       ]= mot_val[0+2*stride]= mot_val[2+2*stride]=
                mot_val[1       ]= mot_val[3       ]= mot_val[1+2*stride]= mot_val[3+2*stride]=
                mot_val1[0       ]= mot_val1[2       ]= mot_val1[0+2*stride]= mot_val1[2+2*stride]=
                mot_val1[1       ]= mot_val1[3       ]= mot_val1[1+2*stride]= mot_val1[3+2*stride]= 0;

                set_intra_b(pmvb);
                goto intra;
            }

            cbpy = get_vlc2(&s->gb, cbpy_vlc.table, CBPY_VLC_BITS, 1);

            if (cbpy < 0){
                av_log(s->avctx, AV_LOG_ERROR, "b cbpy damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }

            if(s->alt_inter_vlc==0 || (cbpc & 3)!=3)
                cbpy ^= 0xF;

            cbp = (cbpc & 3) | (cbpy << 2);
        }else
            cbp=0;

        assert(!s->mb_intra);

        if(IS_QUANT(mb_type)){
            h263_decode_dquant(s);
        }

        if(IS_DIRECT(mb_type)){
            s->mv_dir = MV_DIR_FORWARD | MV_DIR_BACKWARD | MV_DIRECT;
            //pmvb->mvp[0].fw.valid=1;
            //pmvb->mvp[0].bw.valid=1;
            mb_type |= ff_mpeg4_set_direct_mv_amba(s, 0, 0,pmvb);
        }else{
            s->mv_dir = 0;
            s->mv_type= MV_TYPE_16X16;

//FIXME UMV

            if(USES_LIST(mb_type, 0)){
                mot_val= h263_pred_motion_amba(s, 0, 0, &mx, &my);
                s->mv_dir = MV_DIR_FORWARD;

                mx = h263_decode_motion(s, mx, 1);
                my = h263_decode_motion(s, my, 1);

                //try remove
                mot_val[0       ]= mot_val[2       ]= mot_val[0+2*stride]= mot_val[2+2*stride]= mx;
                mot_val[1       ]= mot_val[3       ]= mot_val[1+2*stride]= mot_val[3+2*stride]= my;

                //pmvb->mvp[0].fw.valid=1;
                pmvb->mvp[0].fw.ref_frame_num=forward_ref_num;
                pmvb->mvp[0].fw.mv_x=mx;
                pmvb->mvp[0].fw.mv_y=my;
            }
            else
            {
                pmvb->mvp[0].fw.ref_frame_num=not_valid_mv;
            }

            if(USES_LIST(mb_type, 1)){
                mot_val= h263_pred_motion_amba(s, 0, 1, &mx, &my);
                s->mv_dir |= MV_DIR_BACKWARD;
                mx = h263_decode_motion(s, mx, 1);
                my = h263_decode_motion(s, my, 1);

//                s->mv[1][0][0] = mx;
//                s->mv[1][0][1] = my;
                //try remove
                mot_val[0       ]= mot_val[2       ]= mot_val[0+2*stride]= mot_val[2+2*stride]= mx;
                mot_val[1       ]= mot_val[3       ]= mot_val[1+2*stride]= mot_val[3+2*stride]= my;

                //pmvb->mvp[0].bw.valid=1;
                pmvb->mvp[0].bw.ref_frame_num=backward_ref_num;
                pmvb->mvp[0].bw.mv_x=mx;
                pmvb->mvp[0].bw.mv_y=my;
            }
            else
            {
                pmvb->mvp[0].bw.ref_frame_num=not_valid_mv;
            }

            /*
            pcpdes=&pmvb->mvp[0].fw;
            pcpval=*pcpdes;
            pcpdes+=2;
            *pcpdes=pcpval;
            pcpdes+=2;
            *pcpdes=pcpval;
            pcpdes+=2;
            *pcpdes=pcpval;

            pcpdes=&pmvb->mvp[0].bw;
            pcpval=*pcpdes;
            pcpdes+=2;
            *pcpdes=pcpval;
            pcpdes+=2;
            *pcpdes=pcpval;
            pcpdes+=2;
            *pcpdes=pcpval;*/

            //calculate other mvs
            //todo

        }

        s->current_picture_ptr->mb_type[xy]= mb_type;
    } else { /* I-Frame */
        do{
            cbpc = get_vlc2(&s->gb, intra_MCBPC_vlc.table, INTRA_MCBPC_VLC_BITS, 2);
            if (cbpc < 0){
                av_log(s->avctx, AV_LOG_ERROR, "I cbpc damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }
        }while(cbpc == 8);

//        pmvp->mb_setting.slice_start=1;
//        pmvp->mb_setting.frame_left=!s->mb_x;
//        pmvp->mb_setting.frame_top=!s->mb_y;
//        s->dsp.clear_blocks(s->block[0]);

        dquant = cbpc & 4;
        s->mb_intra = 1;
        /*try remove
        //update motion_val
        s->current_picture_ptr->motion_val[0][blockindex][0] = 0;
        s->current_picture_ptr->motion_val[0][blockindex][1] = 0;
        s->current_picture_ptr->motion_val[0][blockindex + 1][0] = 0;
        s->current_picture_ptr->motion_val[0][blockindex + 1][1] = 0;
        s->current_picture_ptr->motion_val[0][blockindex + stride][0] = 0;
        s->current_picture_ptr->motion_val[0][blockindex + stride][1] = 0;
        s->current_picture_ptr->motion_val[0][blockindex + 1 + stride][0] = 0;
        s->current_picture_ptr->motion_val[0][blockindex + 1 + stride][1] = 0;*/
        //can remove?
        //set_intra_p(pmvp);

intra:
        pmvp->mb_setting.intra=1;

        s->current_picture_ptr->mb_type[xy]= MB_TYPE_INTRA;
        if (s->h263_aic) {
            s->ac_pred = get_bits1(&s->gb);
            if(s->ac_pred){
                s->current_picture_ptr->mb_type[xy]= MB_TYPE_INTRA | MB_TYPE_ACPRED;

                s->h263_aic_dir = get_bits1(&s->gb);
            }
        }else
            s->ac_pred = 0;

        if(s->pb_frame && get_bits1(&s->gb))
            pb_mv_count = h263_get_modb(&s->gb, s->pb_frame, &cbpb);
        cbpy = get_vlc2(&s->gb, cbpy_vlc.table, CBPY_VLC_BITS, 1);
        if(cbpy<0){
            av_log(s->avctx, AV_LOG_ERROR, "I cbpy damaged at %d %d\n", s->mb_x, s->mb_y);
            return -1;
        }
        cbp = (cbpc & 3) | (cbpy << 2);
        if (dquant) {
            h263_decode_dquant(s);
        }

        pb_mv_count += !!s->pb_frame;
    }

    while(pb_mv_count--){
        h263_decode_motion(s, 0, 1);
        h263_decode_motion(s, 0, 1);
    }

    /* decode each block */
    if(!s->mb_intra)
    {
        for (i = 0; i < 6; i++) {
            if (h263_decode_block_inter_dequant(s, block[i], i, cbp&32) < 0)
                return -1;
            cbp+=cbp;
        }
    }
    else
    {
        for (i = 0; i < 6; i++) {
            if (h263_decode_block_intra_dequant(s, block[i], i, cbp&32) < 0)
                return -1;
            cbp+=cbp;
        }
    }

    if(s->pb_frame && h263_skip_b_part(s, cbpb) < 0)
        return -1;
    if(s->obmc && !s->mb_intra){
        if(s->pict_type == FF_P_TYPE && s->mb_x+1<s->mb_width && s->mb_num_left != 1)
            preview_obmc(s);
    }
end:

    //
    pinfo->block_last_index[0]=s->block_last_index[0];
    pinfo->block_last_index[1]=s->block_last_index[1];
    pinfo->block_last_index[2]=s->block_last_index[2];
    pinfo->block_last_index[3]=s->block_last_index[3];
    pinfo->block_last_index[4]=s->block_last_index[4];
    pinfo->block_last_index[5]=s->block_last_index[5];

        /* per-MB end of slice check */
    {
        int v= show_bits(&s->gb, 16);

        if(get_bits_count(&s->gb) + 16 > s->gb.size_in_bits){
            v>>= get_bits_count(&s->gb) + 16 - s->gb.size_in_bits;
        }

        if(v==0)
            return SLICE_END;
    }

    return SLICE_OK;
}

int ff_mpeg4_decode_mb_amba(MpegEncContext *s,
                      DCTELEM block[6][64])
{
    int mx, my;
    int cbpc, cbpy, i, cbp, pred_x, pred_y, dquant;
//    const int stride= s->b8_stride;
//    const int blockindex=s->block_index[0];
    int16_t *mot_val;
    static int8_t quant_tab[4] = { -1, -2, 1, 2 };
    const int xy= s->mb_x + s->mb_y * s->mb_stride;

    H263AmbaDecContext_t* thiz=(H263AmbaDecContext_t*)s;
    h263_pic_swinfo_t* pinfo=thiz->p_pic->pswinfo+s->mb_x + s->mb_y*s->mb_width;
//    int* pcpdes, pcpval;
    assert(s->h263_pred);

#ifdef __dump_DSP_TXT__
    char logt[80];
    log_mbtype log_mbtype;//0: intra, 1: skip 16x16, 2: skip
    int log_fielddct=0;
    snprintf(logt,79,">>>>>>>>>>>>>>>>>>>> [MB] = [%d %d] <<<<<<<<<<<<<<<<<<<<",s->mb_y,s->mb_x);
    log_text_p(log_fd_dsp_text,logt,log_cur_frame);
    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_vld,logt,log_cur_frame);
    #endif
//    memset(s->logdct,0,sizeof(s->logdct));
//    memset(s->logdct_acdc,0,sizeof(s->logdct_acdc));
//    memset(s->logdct_deqaun,0,sizeof(s->logdct_deqaun));
//    memset(s->logdct_result,0,sizeof(s->logdct_result));
#endif
    mb_mv_P_t* pmvp=thiz->p_pic->pmvp+(s->mb_width*s->mb_y+s->mb_x);
    mb_mv_B_t* pmvb=thiz->p_pic->pmvb+(s->mb_width*s->mb_y+s->mb_x);
    ambadec_assert_ffmpeg(((void*)pmvp)==((void*)pmvb));
    pmvp->mb_setting.slice_start=1;
    if(s->pict_type != FF_I_TYPE)
    {
        pmvp->mb_setting.frame_left=!s->mb_x;
        pmvp->mb_setting.frame_top=!s->mb_y;
    }
//    pmvp->mb_setting.frame_left=!s->mb_x;
//    pmvp->mb_setting.frame_top=!s->mb_y;

    if (s->pict_type == FF_P_TYPE || s->pict_type==FF_S_TYPE) {

        do{
            if (get_bits1(&s->gb)) {
                /* skip mb */
                s->mb_intra = 0;
                for(i=0;i<6;i++)
                {
                    s->block_last_index[i] = -1;
                    //pinfo->block_last_index[i]=-1;
                }
                s->mv_dir = MV_DIR_FORWARD;
                s->mv_type = MV_TYPE_16X16;
                if(s->pict_type==FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE){
                    s->current_picture_ptr->mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_GMC | MB_TYPE_16x16 | MB_TYPE_L0;
                    s->mcsel=1;

                    #ifdef __dsp_interpret_gmc__
                    pmvp->mb_setting.mv_type=mv_type_16x16_gmc;
                    #endif

                    mx= get_amv(s, 0);
                    my= get_amv(s, 1);

                    #ifdef __dump_DSP_TXT__
                    log_mbtype=skipgmc16x16;
                    #endif

                    s->mb_skipped = 0;
                }else{
                    s->current_picture_ptr->mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_L0;
                    s->mcsel=0;
                    mx = 0;
                    my = 0;
                    s->mb_skipped = 1;

                    #ifdef __dump_DSP_TXT__
                    log_mbtype=skip16x16;
                    #endif

                }

                //pmvp->mv[0].valid=1;
                pmvp->mv[0].ref_frame_num=forward_ref_num;
                pmvp->mv[0].mv_x=mx;
                pmvp->mv[0].mv_y=my;

                //calculate other mvs
/*                pcpdes=(int*)pmvp->mv;
                pcpval=*pcpdes++;
                *pcpdes++=pcpval;
                *pcpdes++=pcpval;
                *pcpdes=pcpval;*/
                //need calculate chroma mv
                //todo

                goto end;
            }
            cbpc = get_vlc2(&s->gb, inter_MCBPC_vlc.table, INTER_MCBPC_VLC_BITS, 2);
            //fprintf(stderr, "\tCBPC: %d", cbpc);
            if (cbpc < 0){
                av_log(s->avctx, AV_LOG_ERROR, "cbpc damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }
        }while(cbpc == 20);

//        s->dsp.clear_blocks(s->block[0]);
        dquant = cbpc & 8;
        s->mb_intra = ((cbpc & 4) != 0);
        if (s->mb_intra)
        {
            #ifdef __dump_DSP_TXT__
            log_mbtype=intra;
            #endif

            set_intra_p(pmvp);
            goto intra;
        }

        if(s->pict_type==FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE && (cbpc & 16) == 0)
            s->mcsel= get_bits1(&s->gb);
        else s->mcsel= 0;
        cbpy = get_vlc2(&s->gb, cbpy_vlc.table, CBPY_VLC_BITS, 1) ^ 0x0F;

        cbp = (cbpc & 3) | (cbpy << 2);
        if (dquant) {
            ff_set_qscale(s, s->qscale + quant_tab[get_bits(&s->gb, 2)]);
        }
        if((!s->progressive_sequence) && (cbp || (s->workaround_bugs&FF_BUG_XVID_ILACE)))
        {
            s->interlaced_dct= get_bits1(&s->gb);
            pmvp->mb_setting.dct_type=s->interlaced_dct;
        }

        s->mv_dir = MV_DIR_FORWARD;
        if ((cbpc & 16) == 0) {
            if(s->mcsel){
                s->current_picture_ptr->mb_type[xy]= MB_TYPE_GMC | MB_TYPE_16x16 | MB_TYPE_L0;
                /* 16x16 global motion prediction */
                s->mv_type = MV_TYPE_16X16;
                pmvp->mv[0].mv_x= get_amv(s, 0);
                pmvp->mv[0].mv_y= get_amv(s, 1);
                //pmvp->mv[0].valid=1;
                pmvp->mv[0].ref_frame_num=forward_ref_num;

                #ifdef __dsp_interpret_gmc__
                pmvp->mb_setting.mv_type=mv_type_16x16_gmc;
                #endif
//                s->mv[0][0][0] = mx;
//                s->mv[0][0][1] = my;
                #ifdef __dump_DSP_TXT__
                log_mbtype=gmc16x16;
                #endif

            }else if((!s->progressive_sequence) && get_bits1(&s->gb)){
                s->current_picture_ptr->mb_type[xy]= MB_TYPE_16x8 | MB_TYPE_L0 | MB_TYPE_INTERLACED;
                /* 16x8 field motion prediction */
                s->mv_type= MV_TYPE_FIELD;
                pmvp->mb_setting.mv_type=mv_type_16x8;
                pmvp->mb_setting.field_prediction=1;
                //pmvp->mv[0].valid=1;
                pmvp->mv[0].ref_frame_num=forward_ref_num;
                pmvp->mv[0].bottom=get_bits1(&s->gb);
                //pmvp->mv[1].valid=1;
                pmvp->mv[1].ref_frame_num=forward_ref_num;
                pmvp->mv[1].bottom=get_bits1(&s->gb);

//                s->field_select[0][0]= get_bits1(&s->gb);
//                s->field_select[0][1]= get_bits1(&s->gb);

                h263_pred_motion_amba(s, 0, 0, &pred_x, &pred_y);

                for(i=0; i<2; i++){
                    mx = h263_decode_motion(s, pred_x, s->f_code);
                    if (mx >= 0xffff)
                        return -1;

                    my = h263_decode_motion(s, pred_y/2, s->f_code);
                    if (my >= 0xffff)
                        return -1;

//                    s->mv[0][i][0] = mx;
//                    s->mv[0][i][1] = my;
                    pmvp->mv[i].mv_x=mx;
                    pmvp->mv[i].mv_y=my;
                }
                //need to set chorma mvs
                //todo
                #ifdef __dump_DSP_TXT__
                log_mbtype=field16x8;
                #endif
            }else{
                s->current_picture_ptr->mb_type[xy]= MB_TYPE_16x16 | MB_TYPE_L0;
                /* 16x16 motion prediction */
                s->mv_type = MV_TYPE_16X16;
                h263_pred_motion_amba(s, 0, 0, &pred_x, &pred_y);
                mx = h263_decode_motion(s, pred_x, s->f_code);

                if (mx >= 0xffff)
                    return -1;

                my = h263_decode_motion(s, pred_y, s->f_code);

                if (my >= 0xffff)
                    return -1;
//                s->mv[0][0][0] = mx;
//                s->mv[0][0][1] = my;
                #ifdef __dump_DSP_TXT__
                log_mbtype=frame16x16;
                #endif
                //av_log(NULL,AV_LOG_ERROR,"y=%d,x=%d.\n",s->mb_x,s->mb_y);
                //av_log(NULL,AV_LOG_ERROR,"mvx=%d,mvy=%d,pred_x=%d,pred_y=%d.\n",mx,my,pred_x,pred_y);

                pmvp->mv[0].mv_x= mx;
                pmvp->mv[0].mv_y= my;
                //pmvp->mv[0].valid=1;
                pmvp->mv[0].ref_frame_num=forward_ref_num;
                //set other mvs
/*                pcpdes=(int*)pmvp->mv;
                pcpval=*pcpdes++;
                *pcpdes++=pcpval;
                *pcpdes++=pcpval;
                *pcpdes=pcpval;*/
                //need calculate chroma mv
                //todo
            }
        } else {
            s->current_picture_ptr->mb_type[xy]= MB_TYPE_8x8 | MB_TYPE_L0;
            s->mv_type = MV_TYPE_8X8;
            pmvp->mb_setting.mv_type=mv_type_8x8;
            for(i=0;i<4;i++) {
                mot_val = h263_pred_motion_amba(s, i, 0, &pred_x, &pred_y);
                mx = h263_decode_motion(s, pred_x, s->f_code);
                if (mx >= 0xffff)
                    return -1;

                my = h263_decode_motion(s, pred_y, s->f_code);
                if (my >= 0xffff)
                    return -1;
                //s->mv[0][i][0] = mx;
                //s->mv[0][i][1] = my;
                mot_val[0] = mx;
                mot_val[1] = my;
                pmvp->mv[i].mv_x= mx;
                pmvp->mv[i].mv_y= my;
                //pmvp->mv[i].valid=1;
                pmvp->mv[0].ref_frame_num=forward_ref_num;
            }
            //need calculate chroma mv
            //todo
            #ifdef __dump_DSP_TXT__
            log_mbtype=frame8x8;
            #endif
        }
    } else if(s->pict_type==FF_B_TYPE) {
        int modb1; // first bit of modb
        int modb2; // second bit of modb
        int mb_type;
        //mb_mv_B_t* pmvb=thiz->p_pic->pmvb+(s->mb_width*s->mb_y+s->mb_x);
//        pmvb->mb_setting.slice_start=1;
//        pmvb->mb_setting.frame_left=!s->mb_x;
//        pmvb->mb_setting.frame_top=!s->mb_y;

        s->mb_intra = 0; //B-frames never contain intra blocks
        s->mcsel=0;      //     ...               true gmc blocks

        if(s->mb_x==0){
            for(i=0; i<2; i++){
                s->last_mv[i][0][0]=
                s->last_mv[i][0][1]=
                s->last_mv[i][1][0]=
                s->last_mv[i][1][1]= 0;
            }
        }

        /* if we skipped it in the future P Frame than skip it now too */
        s->mb_skipped= s->next_picture_ptr->mbskip_table[s->mb_y * s->mb_stride + s->mb_x]; // Note, skiptab=0 if last was GMC

        if(s->mb_skipped){
                /* skip mb */
            for(i=0;i<6;i++)
            {
                s->block_last_index[i] = -1;
                //pinfo->block_last_index[i]=-1;
            }

            s->mv_dir = MV_DIR_FORWARD;
            s->mv_type = MV_TYPE_16X16;
            s->mv[0][0][0] = 0;
            s->mv[0][0][1] = 0;
            s->mv[1][0][0] = 0;
            s->mv[1][0][1] = 0;
            s->current_picture_ptr->mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_L0;
            #ifdef __dump_DSP_TXT__
            log_mbtype=skip16x16;
            #endif

            pmvb->mvp[0].fw.ref_frame_num= forward_ref_num;
            pmvb->mvp[0].bw.ref_frame_num= not_valid_mv;
            //pmvb->mvp[0].fw.valid= 1;
            //pmvb->mvp[0].bw.valid= 1;

            //set other mvs
/*            pcpdes=&pmvb->mvp[0].bw;
            pcpval=*pcpdes++;
            *pcpdes++=pcpval;
            *pcpdes++=pcpval;
            *pcpdes++=pcpval;
            *pcpdes++=pcpval;
            *pcpdes++=pcpval;
            *pcpdes++=pcpval;
            *pcpdes++=pcpval;
            *pcpdes=pcpval;*/

            goto end;
        }

        modb1= get_bits1(&s->gb);
        if(modb1){
            mb_type= MB_TYPE_DIRECT2 | MB_TYPE_SKIP | MB_TYPE_L0L1; //like MB_TYPE_B_DIRECT but no vectors coded
            cbp=0;
        }else{
            modb2= get_bits1(&s->gb);
            mb_type= get_vlc2(&s->gb, mb_type_b_vlc.table, MB_TYPE_B_VLC_BITS, 1);
            if(mb_type<0){
                av_log(s->avctx, AV_LOG_ERROR, "illegal MB_type\n");
                return -1;
            }
            mb_type= mb_type_b_map[ mb_type ];
            if(modb2) cbp= 0;
            else{
//                s->dsp.clear_blocks(s->block[0]);
                cbp= get_bits(&s->gb, 6);
            }

            if ((!IS_DIRECT(mb_type)) && cbp) {
                if(get_bits1(&s->gb)){
                    ff_set_qscale(s, s->qscale + get_bits1(&s->gb)*4 - 2);
                }
            }

            if(!s->progressive_sequence){
                if(cbp)
                {
                    s->interlaced_dct= get_bits1(&s->gb);
                    pmvb->mb_setting.dct_type=s->interlaced_dct;
                }

                if(!IS_DIRECT(mb_type) && get_bits1(&s->gb)){
                    mb_type |= MB_TYPE_16x8 | MB_TYPE_INTERLACED;
                    mb_type &= ~MB_TYPE_16x16;
                    pmvb->mb_setting.field_prediction=1;
                    pmvb->mb_setting.mv_type=mv_type_16x8;

                    if(USES_LIST(mb_type, 0)){
                        //s->field_select[0][0]= get_bits1(&s->gb);
                        //s->field_select[0][1]= get_bits1(&s->gb);
                        pmvb->mvp[0].fw.bottom=get_bits1(&s->gb);
                        pmvb->mvp[1].fw.bottom=get_bits1(&s->gb);
                        //pmvb->mvp[0].fw.ref_frame_num=forward_ref_num;
                        //pmvb->mvp[1].fw.ref_frame_num=forward_ref_num;
                    }
                    /*else
                    {
                        pmvb->mvp[0].fw.ref_frame_num=not_valid_mv;
                        pmvb->mvp[1].fw.ref_frame_num=not_valid_mv;
                    }*/
                    if(USES_LIST(mb_type, 1)){
                        //s->field_select[1][0]= get_bits1(&s->gb);
                        //s->field_select[1][1]= get_bits1(&s->gb);
                        pmvb->mvp[0].bw.bottom=get_bits1(&s->gb);
                        pmvb->mvp[1].bw.bottom=get_bits1(&s->gb);
                        //pmvb->mvp[0].bw.ref_frame_num=backward_ref_num;
                        //pmvb->mvp[1].bw.ref_frame_num=backward_ref_num;
                    }
                    /*else
                    {
                        pmvb->mvp[0].bw.ref_frame_num=not_valid_mv;
                        pmvb->mvp[1].bw.ref_frame_num=not_valid_mv;
                    }*/
                    #ifdef __dump_DSP_TXT__
                    log_mbtype=field16x8;
                    log_fielddct=s->interlaced_dct;
                    #endif
                }
            }

            s->mv_dir = 0;
            if((mb_type & (MB_TYPE_DIRECT2|MB_TYPE_INTERLACED)) == 0){
                s->mv_type= MV_TYPE_16X16;

                if(USES_LIST(mb_type, 0)){
                    s->mv_dir = MV_DIR_FORWARD;

                    mx = h263_decode_motion(s, s->last_mv[0][0][0], s->f_code);
                    my = h263_decode_motion(s, s->last_mv[0][0][1], s->f_code);
                    s->last_mv[0][1][0]= s->last_mv[0][0][0]= s->mv[0][0][0] = mx;
                    s->last_mv[0][1][1]= s->last_mv[0][0][1]= s->mv[0][0][1] = my;
                    //pmvb->mvp[0].fw.valid=1;
                    pmvb->mvp[0].fw.ref_frame_num=forward_ref_num;
                    pmvb->mvp[0].fw.mv_x=mx;
                    pmvb->mvp[0].fw.mv_y=my;
                    //need to set other mvs
                    //todo
                }
                else
                {
                    pmvb->mvp[0].fw.ref_frame_num=not_valid_mv;
                }

                if(USES_LIST(mb_type, 1)){
                    s->mv_dir |= MV_DIR_BACKWARD;

                    mx = h263_decode_motion(s, s->last_mv[1][0][0], s->b_code);
                    my = h263_decode_motion(s, s->last_mv[1][0][1], s->b_code);
                    s->last_mv[1][1][0]= s->last_mv[1][0][0]= s->mv[1][0][0] = mx;
                    s->last_mv[1][1][1]= s->last_mv[1][0][1]= s->mv[1][0][1] = my;
                    //pmvb->mvp[0].bw.valid=1;
                    pmvb->mvp[0].bw.ref_frame_num=backward_ref_num;
                    pmvb->mvp[0].bw.mv_x=mx;
                    pmvb->mvp[0].bw.mv_y=my;
                    //need to set other mvs
                    //todo
                }
                else
                {
                    pmvb->mvp[0].bw.ref_frame_num=not_valid_mv;
                }

                #ifdef __dump_DSP_TXT__
                if(s->mv_dir==MV_DIR_FORWARD)
                {
                    log_mbtype=frame16x16;
                }
                else if(s->mv_dir==MV_DIR_BACKWARD)
                {
                    log_mbtype=frame16x16bw;
                }
                else if(s->mv_dir==(MV_DIR_BACKWARD|MV_DIR_FORWARD))
                {
                     log_mbtype=frame16x16bi;
                }
                else
                {
                    ambadec_assert_ffmpeg(0);
                }
                #endif
            }else if(!IS_DIRECT(mb_type)){
                s->mv_type= MV_TYPE_FIELD;
                pmvb->mb_setting.mv_type=mv_type_16x8;
                pmvb->mb_setting.field_prediction=1;
                ambadec_assert_ffmpeg(pmvb->mb_setting.field_prediction);
                if(USES_LIST(mb_type, 0)){
                    s->mv_dir = MV_DIR_FORWARD;

                    for(i=0; i<2; i++){
                        mx = h263_decode_motion(s, s->last_mv[0][i][0]  , s->f_code);
                        my = h263_decode_motion(s, s->last_mv[0][i][1]/2, s->f_code);
                        s->last_mv[0][i][0]=  s->mv[0][i][0] = mx;
                        s->last_mv[0][i][1]= (s->mv[0][i][1] = my)*2;
                        //pmvb->mvp[i].fw.valid=1;
                        pmvb->mvp[i].fw.ref_frame_num=forward_ref_num;
                        pmvb->mvp[i].fw.mv_x=mx;
                        pmvb->mvp[i].fw.mv_y=my;
                        //need to set other mvs
                        //todo
                    }
                }
                else
                {
                    pmvb->mvp[0].fw.ref_frame_num=not_valid_mv;
                    pmvb->mvp[1].fw.ref_frame_num=not_valid_mv;
                }

                if(USES_LIST(mb_type, 1)){
                    s->mv_dir |= MV_DIR_BACKWARD;

                    for(i=0; i<2; i++){
                        mx = h263_decode_motion(s, s->last_mv[1][i][0]  , s->b_code);
                        my = h263_decode_motion(s, s->last_mv[1][i][1]/2, s->b_code);
                        s->last_mv[1][i][0]=  s->mv[1][i][0] = mx;
                        s->last_mv[1][i][1]= (s->mv[1][i][1] = my)*2;
                        //pmvb->mvp[i].bw.valid=1;
                        pmvb->mvp[i].bw.mv_x=mx;
                        pmvb->mvp[i].bw.mv_y=my;
                        pmvb->mvp[i].bw.ref_frame_num=backward_ref_num;
                        //need to set other mvs
                        //todo
                    }
                }
                else
                {
                    pmvb->mvp[0].bw.ref_frame_num=not_valid_mv;
                    pmvb->mvp[1].bw.ref_frame_num=not_valid_mv;
                }

                #ifdef __dump_DSP_TXT__
                if(s->mv_dir==MV_DIR_FORWARD)
                {
                    log_mbtype=field16x8;
                }
                else if(s->mv_dir==MV_DIR_BACKWARD)
                {
                    log_mbtype=field16x8bw;
                }
                else if(s->mv_dir==(MV_DIR_BACKWARD|MV_DIR_FORWARD))
                {
                     log_mbtype=field16x8bi;
                }
                else
                {
                    ambadec_assert_ffmpeg(0);
                }
                #endif
            }
        }

        if(IS_DIRECT(mb_type)){
            if(IS_SKIP(mb_type))
                mx=my=0;
            else{
                mx = h263_decode_motion(s, 0, 1);
                my = h263_decode_motion(s, 0, 1);
            }

            s->mv_dir = MV_DIR_FORWARD | MV_DIR_BACKWARD | MV_DIRECT;
            mb_type |= ff_mpeg4_set_direct_mv_amba(s, mx, my,pmvb);

            #ifdef __dump_DSP_TXT__
            log_mbtype=direct;
            #endif

        }
        s->current_picture_ptr->mb_type[xy]= mb_type;
    } else { /* I-Frame */

//        pmvp->mb_setting.slice_start=1;
//        pmvp->mb_setting.frame_left=!s->mb_x;
//        pmvp->mb_setting.frame_top=!s->mb_y;

        do{
            cbpc = get_vlc2(&s->gb, intra_MCBPC_vlc.table, INTRA_MCBPC_VLC_BITS, 2);
            if (cbpc < 0){
                av_log(s->avctx, AV_LOG_ERROR, "I cbpc damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }
        }while(cbpc == 8);

        dquant = cbpc & 4;
        s->mb_intra = 1;

        #ifdef __dump_DSP_TXT__
        log_mbtype=intra;
        #endif
intra:
        pmvp->mb_setting.intra=1;

        s->ac_pred = get_bits1(&s->gb);
        if(s->ac_pred)
            s->current_picture_ptr->mb_type[xy]= MB_TYPE_INTRA | MB_TYPE_ACPRED;
        else
            s->current_picture_ptr->mb_type[xy]= MB_TYPE_INTRA;

        cbpy = get_vlc2(&s->gb, cbpy_vlc.table, CBPY_VLC_BITS, 1);
        if(cbpy<0){
            av_log(s->avctx, AV_LOG_ERROR, "I cbpy damaged at %d %d\n", s->mb_x, s->mb_y);
            return -1;
        }
        cbp = (cbpc & 3) | (cbpy << 2);

        s->use_intra_dc_vlc= s->qscale < s->intra_dc_threshold;

        if (dquant) {
            ff_set_qscale(s, s->qscale + quant_tab[get_bits(&s->gb, 2)]);
        }

        if(!s->progressive_sequence)
        {
            s->interlaced_dct= get_bits1(&s->gb);
            pmvp->mb_setting.dct_type=s->interlaced_dct;
            #ifdef __dump_DSP_TXT__
            log_fielddct=s->interlaced_dct;
            #endif
        }

        #ifdef __dump_DSP_TXT__
        if(log_fielddct)
            snprintf(logt,79,"[MODE, FIELD_DCT, QP, CBP] = [  INTRA  Yes    %d    %d].\n",s->qscale,cbp);
        else
            snprintf(logt,79,"[MODE, FIELD_DCT, QP, CBP] = [  INTRA  No    %d    %d].\n",s->qscale,cbp);
        log_text_p(log_fd_dsp_text, logt,log_cur_frame);
        #ifdef __dump_separate__
        log_text_p(log_fd_dsp_text+log_offset_vld,logt,log_cur_frame);
        #endif
        #endif

//        s->dsp.clear_blocks(s->block[0]);
        /* decode each block */
        #if 0
        for (i = 0; i < 6; i++) {
            if (mpeg4_decode_block(s, block[i], i, cbp&32, 1, 0) < 0)
                return -1;
            cbp+=cbp;
        }
        #endif

        for (i = 0; i < 6; i++) {
            if (mpeg4_decode_block_intra_dequant(s, block[i], i, cbp&32) < 0)
                return -1;
            cbp+=cbp;
        }

        goto end;
    }

    #ifdef __dump_DSP_TXT__
    if(log_fielddct)
        snprintf(logt,79,"[%s, FIELD_DCT, QP, CBP] = [  INTER  Yes    %d    %d].\n",log_mbtypestr[log_mbtype],s->qscale,cbp);
    else
        snprintf(logt,79,"[%s, FIELD_DCT, QP, CBP] = [  INTER  No    %d    %d].\n",log_mbtypestr[log_mbtype],s->qscale,cbp);
    log_text_p(log_fd_dsp_text, logt,log_cur_frame);
    #ifdef __dump_separate__
    log_text_p(log_fd_dsp_text+log_offset_vld,logt,log_cur_frame);
    #endif
    #endif

    /* decode each block */
    #if 0
    for (i = 0; i < 6; i++) {
        if (mpeg4_decode_block(s, block[i], i, cbp&32, 0, 0) < 0)
            return -1;
        cbp+=cbp;
    }
    #endif

    if(s->mpeg_quant)
    {
        for (i = 0; i < 6; i++)
        {
            if (mpeg4_decode_block_inter_dequant_1(s, block[i], i, cbp&32) < 0)
                return -1;
            cbp+=cbp;
        }
    }
    else
    {
        for (i = 0; i < 6; i++)
        {
            if (mpeg4_decode_block_inter_dequant_0(s, block[i], i, cbp&32) < 0)
                return -1;
            cbp+=cbp;
        }
    }

end:

    //
    pinfo->block_last_index[0]=s->block_last_index[0];
    pinfo->block_last_index[1]=s->block_last_index[1];
    pinfo->block_last_index[2]=s->block_last_index[2];
    pinfo->block_last_index[3]=s->block_last_index[3];
    pinfo->block_last_index[4]=s->block_last_index[4];
    pinfo->block_last_index[5]=s->block_last_index[5];

        /* per-MB end of slice check */
    if(s->codec_id==CODEC_ID_AMBA_P_MPEG4){
        if(mpeg4_is_resync(s)){
            const int delta= s->mb_x + 1 == s->mb_width ? 2 : 1;
            if(s->pict_type==FF_B_TYPE && s->next_picture_ptr->mbskip_table[xy + delta])
                return SLICE_OK;
            return SLICE_END;
        }
    }

    return SLICE_OK;
}

static int h263_decode_motion(MpegEncContext * s, int pred, int f_code)
{
    int code, val, sign, shift, l;
    code = get_vlc2(&s->gb, mv_vlc.table, MV_VLC_BITS, 2);

    if (code == 0)
        return pred;
    if (code < 0)
        return 0xffff;

    sign = get_bits1(&s->gb);
    shift = f_code - 1;
    val = code;
    if (shift) {
        val = (val - 1) << shift;
        val |= get_bits(&s->gb, shift);
        val++;
    }
    if (sign)
        val = -val;
    val += pred;

    /* modulo decoding */
    if (!s->h263_long_vectors) {
        l = INT_BIT - 5 - f_code;
        val = (val<<l)>>l;
    } else {
        /* horrible h263 long vector mode */
        if (pred < -31 && val < -63)
            val += 64;
        if (pred > 32 && val > 63)
            val -= 64;

    }
    return val;
}

/* Decodes RVLC of H.263+ UMV */
static int h263p_decode_umotion(MpegEncContext * s, int pred)
{
   int code = 0, sign;

   if (get_bits1(&s->gb)) /* Motion difference = 0 */
      return pred;

   code = 2 + get_bits1(&s->gb);

   while (get_bits1(&s->gb))
   {
      code <<= 1;
      code += get_bits1(&s->gb);
   }
   sign = code & 1;
   code >>= 1;

   code = (sign) ? (pred - code) : (pred + code);
   av_dlog(s->avctx,"H.263+ UMV Motion = %d\n", code);
   return code;

}


static int h263_decode_block(MpegEncContext * s, DCTELEM * block,
                             int n, int coded)
{
    int code, level, i, j, last, run;
    RLTable *rl = &ff_h263_rl_inter;
    const uint8_t *scan_table;
    GetBitContext gb= s->gb;

    //should not use this function
    ambadec_assert_ffmpeg(0);

    scan_table = s->intra_scantable.permutated;
    if (s->h263_aic && s->mb_intra) {
        rl = &rl_intra_aic;
        i = 0;
        if (s->ac_pred) {
            if (s->h263_aic_dir)
                scan_table = s->intra_v_scantable.permutated; /* left */
            else
                scan_table = s->intra_h_scantable.permutated; /* top */
        }
    } else if (s->mb_intra) {
        /* DC coef */
        if(s->codec_id == CODEC_ID_RV10){
#if CONFIG_RV10_DECODER
          if (s->rv10_version == 3 && s->pict_type == FF_I_TYPE) {
            int component, diff;
            component = (n <= 3 ? 0 : n - 4 + 1);
            level = s->last_dc[component];
            if (s->rv10_first_dc_coded[component]) {
                diff = rv_decode_dc(s, n);
                if (diff == 0xffff)
                    return -1;
                level += diff;
                level = level & 0xff; /* handle wrap round */
                s->last_dc[component] = level;
            } else {
                s->rv10_first_dc_coded[component] = 1;
            }
          } else {
                level = get_bits(&s->gb, 8);
                if (level == 255)
                    level = 128;
          }
#endif
        }else{
            level = get_bits(&s->gb, 8);
            if((level&0x7F) == 0){
                av_log(s->avctx, AV_LOG_ERROR, "illegal dc %d at %d %d\n", level, s->mb_x, s->mb_y);
                if(s->error_recognition >= FF_ER_COMPLIANT)
                    return -1;
            }
            if (level == 255)
                level = 128;
        }
        block[0] = level;
        i = 1;
    } else {
        i = 0;
    }
    if (!coded) {
        if (s->mb_intra && s->h263_aic)
            goto not_coded;
        s->block_last_index[n] = i - 1;
        return 0;
    }
retry:
    for(;;) {
        code = get_vlc2(&s->gb, rl->vlc.table, TEX_VLC_BITS, 2);
        if (code < 0){
            av_log(s->avctx, AV_LOG_ERROR, "illegal ac vlc code at %dx%d\n", s->mb_x, s->mb_y);
            return -1;
        }
        if (code == rl->n) {
            /* escape */
            if (s->h263_flv > 1) {
                int is11 = get_bits1(&s->gb);
                last = get_bits1(&s->gb);
                run = get_bits(&s->gb, 6);
                if(is11){
                    level = get_sbits(&s->gb, 11);
                } else {
                    level = get_sbits(&s->gb, 7);
                }
            } else {
                last = get_bits1(&s->gb);
                run = get_bits(&s->gb, 6);
                level = (int8_t)get_bits(&s->gb, 8);
                if(level == -128){
                    if (s->codec_id == CODEC_ID_RV10) {
                        /* XXX: should patch encoder too */
                        level = get_sbits(&s->gb, 12);
                    }else{
                        level = get_bits(&s->gb, 5);
                        level |= get_sbits(&s->gb, 6)<<5;
                    }
                }
            }
        } else {
            run = rl->table_run[code];
            level = rl->table_level[code];
            last = code >= rl->last;
            if (get_bits1(&s->gb))
                level = -level;
        }
        i += run;
        if (i >= 64){
            if(s->alt_inter_vlc && rl == &ff_h263_rl_inter && !s->mb_intra){
                //Looks like a hack but no, it's the way it is supposed to work ...
                rl = &rl_intra_aic;
                i = 0;
                s->gb= gb;
                s->dsp.clear_block(block);
                goto retry;
            }
            av_log(s->avctx, AV_LOG_ERROR, "run overflow at %dx%d i:%d\n", s->mb_x, s->mb_y, s->mb_intra);
            return -1;
        }
        j = scan_table[i];
        block[j] = level;
        if (last)
            break;
        i++;
    }
not_coded:
    if (s->mb_intra && s->h263_aic) {
        h263_pred_acdc(s, block, n);
        i = 63;
    }
    s->block_last_index[n] = i;
    return 0;
}

/**
 * decodes the dc value.
 * @param n block index (0-3 are luma, 4-5 are chroma)
 * @param dir_ptr the prediction direction will be stored here
 * @return the quantized dc
 */
static inline int mpeg4_decode_dc(MpegEncContext * s, int n, int *dir_ptr)
{
    int level, code;

    if (n < 4)
        code = get_vlc2(&s->gb, dc_lum.table, DC_VLC_BITS, 1);
    else
        code = get_vlc2(&s->gb, dc_chrom.table, DC_VLC_BITS, 1);
    if (code < 0 || code > 9 /* && s->nbit<9 */){
        av_log(s->avctx, AV_LOG_ERROR, "illegal dc vlc\n");
        return -1;
    }
    if (code == 0) {
        level = 0;
    } else {
        if(IS_3IV1){
            if(code==1)
                level= 2*get_bits1(&s->gb)-1;
            else{
                if(get_bits1(&s->gb))
                    level = get_bits(&s->gb, code-1) + (1<<(code-1));
                else
                    level = -get_bits(&s->gb, code-1) - (1<<(code-1));
            }
        }else{
            level = get_xbits(&s->gb, code);
        }

        if (code > 8){
            if(get_bits1(&s->gb)==0){ /* marker */
                if(s->error_recognition>=2){
                    av_log(s->avctx, AV_LOG_ERROR, "dc marker bit missing\n");
                    return -1;
                }
            }
        }
    }

    return ff_mpeg4_pred_dc(s, n, level, dir_ptr, 0);
}

/**
 * decodes a block.
 * @return <0 if an error occurred
 */

static inline int mpeg4_decode_block(MpegEncContext * s, DCTELEM * block,
                              int n, int coded, int intra, int rvlc)
{
    int level, i, last, run;
    int dc_pred_dir;
    RLTable * rl;
    RL_VLC_ELEM * rl_vlc;
    const uint8_t * scan_table;
    int qmul, qadd;

    //Note intra & rvlc should be optimized away if this is inlined
    //should not use this function
    ambadec_assert_ffmpeg(0);

    if(intra) {
      if(s->use_intra_dc_vlc){
        /* DC coef */
        if(s->partitioned_frame){
            level = s->dc_val[0][ s->block_index[n] ];
            if(n<4) level= FASTDIV((level + (s->y_dc_scale>>1)), s->y_dc_scale);
            else    level= FASTDIV((level + (s->c_dc_scale>>1)), s->c_dc_scale);
            dc_pred_dir= (s->pred_dir_table[s->mb_x + s->mb_y*s->mb_stride]<<n)&32;
        }else{
            level = mpeg4_decode_dc(s, n, &dc_pred_dir);
            if (level < 0)
                return -1;
        }
        block[0] = level;
        i = 0;
      }else{
            i = -1;
            ff_mpeg4_pred_dc(s, n, 0, &dc_pred_dir, 0);
      }
      if (!coded)
          goto not_coded;

      if(rvlc){
          rl = &rvlc_rl_intra;
          rl_vlc = rvlc_rl_intra.rl_vlc[0];
      }else{
          rl = &ff_mpeg4_rl_intra;
          rl_vlc = ff_mpeg4_rl_intra.rl_vlc[0];
      }
      if (s->ac_pred) {
          if (dc_pred_dir == 0)
              scan_table = s->intra_v_scantable.permutated; /* left */
          else
              scan_table = s->intra_h_scantable.permutated; /* top */
      } else {
            scan_table = s->intra_scantable.permutated;
      }
      qmul=1;
      qadd=0;
    } else {
        i = -1;
        if (!coded) {
            s->block_last_index[n] = i;
            return 0;
        }
        if(rvlc) rl = &rvlc_rl_inter;
        else     rl = &ff_h263_rl_inter;

        scan_table = s->intra_scantable.permutated;

        if(s->mpeg_quant){
            qmul=1;
            qadd=0;
            if(rvlc){
                rl_vlc = rvlc_rl_inter.rl_vlc[0];
            }else{
                rl_vlc = ff_h263_rl_inter.rl_vlc[0];
            }
        }else{
            qmul = s->qscale << 1;
            qadd = (s->qscale - 1) | 1;
            if(rvlc){
                rl_vlc = rvlc_rl_inter.rl_vlc[s->qscale];
            }else{
                rl_vlc = ff_h263_rl_inter.rl_vlc[s->qscale];
            }
        }
    }
  {
    OPEN_READER(re, &s->gb);
    for(;;) {
        UPDATE_CACHE(re, &s->gb);
        GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 0);
        if (level==0) {
          /* escape */
          if(rvlc){
                if(SHOW_UBITS(re, &s->gb, 1)==0){
                    av_log(s->avctx, AV_LOG_ERROR, "1. marker bit missing in rvlc esc\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 1);

                last=  SHOW_UBITS(re, &s->gb, 1); SKIP_CACHE(re, &s->gb, 1);
                run=   SHOW_UBITS(re, &s->gb, 6); LAST_SKIP_CACHE(re, &s->gb, 6);
                SKIP_COUNTER(re, &s->gb, 1+1+6);
                UPDATE_CACHE(re, &s->gb);

                if(SHOW_UBITS(re, &s->gb, 1)==0){
                    av_log(s->avctx, AV_LOG_ERROR, "2. marker bit missing in rvlc esc\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 1);

                level= SHOW_UBITS(re, &s->gb, 11); SKIP_CACHE(re, &s->gb, 11);

                if(SHOW_UBITS(re, &s->gb, 5)!=0x10){
                    av_log(s->avctx, AV_LOG_ERROR, "reverse esc missing\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 5);

                level=  level * qmul + qadd;
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1); LAST_SKIP_CACHE(re, &s->gb, 1);
                SKIP_COUNTER(re, &s->gb, 1+11+5+1);

                i+= run + 1;
                if(last) i+=192;
          }else{
            int cache;
            cache= GET_CACHE(re, &s->gb);

            if(IS_3IV1)
                cache ^= 0xC0000000;

            if (cache&0x80000000) {
                if (cache&0x40000000) {
                    /* third escape */
                    SKIP_CACHE(re, &s->gb, 2);
                    last=  SHOW_UBITS(re, &s->gb, 1); SKIP_CACHE(re, &s->gb, 1);
                    run=   SHOW_UBITS(re, &s->gb, 6); LAST_SKIP_CACHE(re, &s->gb, 6);
                    SKIP_COUNTER(re, &s->gb, 2+1+6);
                    UPDATE_CACHE(re, &s->gb);

                    if(IS_3IV1){
                        level= SHOW_SBITS(re, &s->gb, 12); LAST_SKIP_BITS(re, &s->gb, 12);
                    }else{
                        if(SHOW_UBITS(re, &s->gb, 1)==0){
                            av_log(s->avctx, AV_LOG_ERROR, "1. marker bit missing in 3. esc\n");
                            return -1;
                        }; SKIP_CACHE(re, &s->gb, 1);

                        level= SHOW_SBITS(re, &s->gb, 12); SKIP_CACHE(re, &s->gb, 12);

                        if(SHOW_UBITS(re, &s->gb, 1)==0){
                            av_log(s->avctx, AV_LOG_ERROR, "2. marker bit missing in 3. esc\n");
                            return -1;
                        }; LAST_SKIP_CACHE(re, &s->gb, 1);

                        SKIP_COUNTER(re, &s->gb, 1+12+1);
                    }

#if 0
                    if(s->error_recognition >= FF_ER_COMPLIANT){
                        const int abs_level= FFABS(level);
                        if(abs_level<=MAX_LEVEL && run<=MAX_RUN){
                            const int run1= run - rl->max_run[last][abs_level] - 1;
                            if(abs_level <= rl->max_level[last][run]){
                                av_log(s->avctx, AV_LOG_ERROR, "illegal 3. esc, vlc encoding possible\n");
                                return -1;
                            }
                            if(s->error_recognition > FF_ER_COMPLIANT){
                                if(abs_level <= rl->max_level[last][run]*2){
                                    av_log(s->avctx, AV_LOG_ERROR, "illegal 3. esc, esc 1 encoding possible\n");
                                    return -1;
                                }
                                if(run1 >= 0 && abs_level <= rl->max_level[last][run1]){
                                    av_log(s->avctx, AV_LOG_ERROR, "illegal 3. esc, esc 2 encoding possible\n");
                                    return -1;
                                }
                            }
                        }
                    }
#endif
                    if (level>0) level= level * qmul + qadd;
                    else         level= level * qmul - qadd;

                    if((unsigned)(level + 2048) > 4095){
                        if(s->error_recognition > FF_ER_COMPLIANT){
                            if(level > 2560 || level<-2560){
                                av_log(s->avctx, AV_LOG_ERROR, "|level| overflow in 3. esc, qp=%d\n", s->qscale);
                                return -1;
                            }
                        }
                        level= level<0 ? -2048 : 2047;
                    }

                    i+= run + 1;
                    if(last) i+=192;
                } else {
                    /* second escape */
#if MIN_CACHE_BITS < 20
                    LAST_SKIP_BITS(re, &s->gb, 2);
                    UPDATE_CACHE(re, &s->gb);
#else
                    SKIP_BITS(re, &s->gb, 2);
#endif
                    GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 1);
                    i+= run + rl->max_run[run>>7][level/qmul] +1; //FIXME opt indexing
                    level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                    LAST_SKIP_BITS(re, &s->gb, 1);
                }
            } else {
                /* first escape */
#if MIN_CACHE_BITS < 19
                LAST_SKIP_BITS(re, &s->gb, 1);
                UPDATE_CACHE(re, &s->gb);
#else
                SKIP_BITS(re, &s->gb, 1);
#endif
                GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 1);
                i+= run;
                level = level + rl->max_level[run>>7][(run-1)&63] * qmul;//FIXME opt indexing
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                LAST_SKIP_BITS(re, &s->gb, 1);
            }
          }
        } else {
            i+= run;
            level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
            LAST_SKIP_BITS(re, &s->gb, 1);
        }
        if (i > 62){
            i-= 192;
            if(i&(~63)){
                av_log(s->avctx, AV_LOG_ERROR, "ac-tex damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }

            block[scan_table[i]] = level;
            break;
        }

        block[scan_table[i]] = level;
    }
    CLOSE_READER(re, &s->gb);
  }

 not_coded:

#ifdef __dump_DSP_TXT__
    memcpy(s->logdct[n],block,128);
#endif

    if (intra) {
        if(!s->use_intra_dc_vlc){
            block[0] = ff_mpeg4_pred_dc(s, n, block[0], &dc_pred_dir, 0);

            i -= i>>31; //if(i == -1) i=0;
        }

        mpeg4_pred_ac_amba(s, block, n, dc_pred_dir);
        if (s->ac_pred) {
            i = 63; /* XXX: not optimal */
        }
    }
    s->block_last_index[n] = i;
#ifdef __dump_DSP_TXT__
    memcpy(s->logdct_acdc[n],block,128);
#endif
    return 0;
}


static void mpeg4_decode_sprite_trajectory(MpegEncContext * s, GetBitContext *gb)
{
    int i;
    int a= 2<<s->sprite_warping_accuracy;
    int rho= 3-s->sprite_warping_accuracy;
    int r=16/a;
    const int vop_ref[4][2]= {{0,0}, {s->width,0}, {0, s->height}, {s->width, s->height}}; // only true for rectangle shapes
    int d[4][2]={{0,0}, {0,0}, {0,0}, {0,0}};
    int sprite_ref[4][2];
    int virtual_ref[2][2];
    int w2, h2, w3, h3;
    int alpha=0, beta=0;
    int w= s->width;
    int h= s->height;
    int min_ab;

    for(i=0; i<s->num_sprite_warping_points; i++){
        int length;
        int x=0, y=0;

        length= get_vlc2(gb, sprite_trajectory.table, SPRITE_TRAJ_VLC_BITS, 3);
        if(length){
            x= get_xbits(gb, length);
        }
        if(!(s->divx_version==500 && s->divx_build==413)) skip_bits1(gb); /* marker bit */

        length= get_vlc2(gb, sprite_trajectory.table, SPRITE_TRAJ_VLC_BITS, 3);
        if(length){
            y=get_xbits(gb, length);
        }
        skip_bits1(gb); /* marker bit */
//printf("%d %d %d %d\n", x, y, i, s->sprite_warping_accuracy);
        s->sprite_traj[i][0]= d[i][0]= x;
        s->sprite_traj[i][1]= d[i][1]= y;
    }
    for(; i<4; i++)
        s->sprite_traj[i][0]= s->sprite_traj[i][1]= 0;

    while((1<<alpha)<w) alpha++;
    while((1<<beta )<h) beta++; // there seems to be a typo in the mpeg4 std for the definition of w' and h'
    w2= 1<<alpha;
    h2= 1<<beta;

// Note, the 4th point isn't used for GMC
    if(s->divx_version==500 && s->divx_build==413){
        sprite_ref[0][0]= a*vop_ref[0][0] + d[0][0];
        sprite_ref[0][1]= a*vop_ref[0][1] + d[0][1];
        sprite_ref[1][0]= a*vop_ref[1][0] + d[0][0] + d[1][0];
        sprite_ref[1][1]= a*vop_ref[1][1] + d[0][1] + d[1][1];
        sprite_ref[2][0]= a*vop_ref[2][0] + d[0][0] + d[2][0];
        sprite_ref[2][1]= a*vop_ref[2][1] + d[0][1] + d[2][1];
    } else {
        sprite_ref[0][0]= (a>>1)*(2*vop_ref[0][0] + d[0][0]);
        sprite_ref[0][1]= (a>>1)*(2*vop_ref[0][1] + d[0][1]);
        sprite_ref[1][0]= (a>>1)*(2*vop_ref[1][0] + d[0][0] + d[1][0]);
        sprite_ref[1][1]= (a>>1)*(2*vop_ref[1][1] + d[0][1] + d[1][1]);
        sprite_ref[2][0]= (a>>1)*(2*vop_ref[2][0] + d[0][0] + d[2][0]);
        sprite_ref[2][1]= (a>>1)*(2*vop_ref[2][1] + d[0][1] + d[2][1]);
    }
/*    sprite_ref[3][0]= (a>>1)*(2*vop_ref[3][0] + d[0][0] + d[1][0] + d[2][0] + d[3][0]);
    sprite_ref[3][1]= (a>>1)*(2*vop_ref[3][1] + d[0][1] + d[1][1] + d[2][1] + d[3][1]); */

// this is mostly identical to the mpeg4 std (and is totally unreadable because of that ...)
// perhaps it should be reordered to be more readable ...
// the idea behind this virtual_ref mess is to be able to use shifts later per pixel instead of divides
// so the distance between points is converted from w&h based to w2&h2 based which are of the 2^x form
    virtual_ref[0][0]= 16*(vop_ref[0][0] + w2)
        + ROUNDED_DIV(((w - w2)*(r*sprite_ref[0][0] - 16*vop_ref[0][0]) + w2*(r*sprite_ref[1][0] - 16*vop_ref[1][0])),w);
    virtual_ref[0][1]= 16*vop_ref[0][1]
        + ROUNDED_DIV(((w - w2)*(r*sprite_ref[0][1] - 16*vop_ref[0][1]) + w2*(r*sprite_ref[1][1] - 16*vop_ref[1][1])),w);
    virtual_ref[1][0]= 16*vop_ref[0][0]
        + ROUNDED_DIV(((h - h2)*(r*sprite_ref[0][0] - 16*vop_ref[0][0]) + h2*(r*sprite_ref[2][0] - 16*vop_ref[2][0])),h);
    virtual_ref[1][1]= 16*(vop_ref[0][1] + h2)
        + ROUNDED_DIV(((h - h2)*(r*sprite_ref[0][1] - 16*vop_ref[0][1]) + h2*(r*sprite_ref[2][1] - 16*vop_ref[2][1])),h);

    switch(s->num_sprite_warping_points)
    {
        case 0:
            s->sprite_offset[0][0]= 0;
            s->sprite_offset[0][1]= 0;
            s->sprite_offset[1][0]= 0;
            s->sprite_offset[1][1]= 0;
            s->sprite_delta[0][0]= a;
            s->sprite_delta[0][1]= 0;
            s->sprite_delta[1][0]= 0;
            s->sprite_delta[1][1]= a;
            s->sprite_shift[0]= 0;
            s->sprite_shift[1]= 0;
            break;
        case 1: //GMC only
            s->sprite_offset[0][0]= sprite_ref[0][0] - a*vop_ref[0][0];
            s->sprite_offset[0][1]= sprite_ref[0][1] - a*vop_ref[0][1];
            s->sprite_offset[1][0]= ((sprite_ref[0][0]>>1)|(sprite_ref[0][0]&1)) - a*(vop_ref[0][0]/2);
            s->sprite_offset[1][1]= ((sprite_ref[0][1]>>1)|(sprite_ref[0][1]&1)) - a*(vop_ref[0][1]/2);
            s->sprite_delta[0][0]= a;
            s->sprite_delta[0][1]= 0;
            s->sprite_delta[1][0]= 0;
            s->sprite_delta[1][1]= a;
            s->sprite_shift[0]= 0;
            s->sprite_shift[1]= 0;
            break;
        case 2:
            s->sprite_offset[0][0]= (sprite_ref[0][0]<<(alpha+rho))
                                                  + (-r*sprite_ref[0][0] + virtual_ref[0][0])*(-vop_ref[0][0])
                                                  + ( r*sprite_ref[0][1] - virtual_ref[0][1])*(-vop_ref[0][1])
                                                  + (1<<(alpha+rho-1));
            s->sprite_offset[0][1]= (sprite_ref[0][1]<<(alpha+rho))
                                                  + (-r*sprite_ref[0][1] + virtual_ref[0][1])*(-vop_ref[0][0])
                                                  + (-r*sprite_ref[0][0] + virtual_ref[0][0])*(-vop_ref[0][1])
                                                  + (1<<(alpha+rho-1));
            s->sprite_offset[1][0]= ( (-r*sprite_ref[0][0] + virtual_ref[0][0])*(-2*vop_ref[0][0] + 1)
                                     +( r*sprite_ref[0][1] - virtual_ref[0][1])*(-2*vop_ref[0][1] + 1)
                                     +2*w2*r*sprite_ref[0][0]
                                     - 16*w2
                                     + (1<<(alpha+rho+1)));
            s->sprite_offset[1][1]= ( (-r*sprite_ref[0][1] + virtual_ref[0][1])*(-2*vop_ref[0][0] + 1)
                                     +(-r*sprite_ref[0][0] + virtual_ref[0][0])*(-2*vop_ref[0][1] + 1)
                                     +2*w2*r*sprite_ref[0][1]
                                     - 16*w2
                                     + (1<<(alpha+rho+1)));
            s->sprite_delta[0][0]=   (-r*sprite_ref[0][0] + virtual_ref[0][0]);
            s->sprite_delta[0][1]=   (+r*sprite_ref[0][1] - virtual_ref[0][1]);
            s->sprite_delta[1][0]=   (-r*sprite_ref[0][1] + virtual_ref[0][1]);
            s->sprite_delta[1][1]=   (-r*sprite_ref[0][0] + virtual_ref[0][0]);

            s->sprite_shift[0]= alpha+rho;
            s->sprite_shift[1]= alpha+rho+2;
            break;
        case 3:
            min_ab= FFMIN(alpha, beta);
            w3= w2>>min_ab;
            h3= h2>>min_ab;
            s->sprite_offset[0][0]=  (sprite_ref[0][0]<<(alpha+beta+rho-min_ab))
                                   + (-r*sprite_ref[0][0] + virtual_ref[0][0])*h3*(-vop_ref[0][0])
                                   + (-r*sprite_ref[0][0] + virtual_ref[1][0])*w3*(-vop_ref[0][1])
                                   + (1<<(alpha+beta+rho-min_ab-1));
            s->sprite_offset[0][1]=  (sprite_ref[0][1]<<(alpha+beta+rho-min_ab))
                                   + (-r*sprite_ref[0][1] + virtual_ref[0][1])*h3*(-vop_ref[0][0])
                                   + (-r*sprite_ref[0][1] + virtual_ref[1][1])*w3*(-vop_ref[0][1])
                                   + (1<<(alpha+beta+rho-min_ab-1));
            s->sprite_offset[1][0]=  (-r*sprite_ref[0][0] + virtual_ref[0][0])*h3*(-2*vop_ref[0][0] + 1)
                                   + (-r*sprite_ref[0][0] + virtual_ref[1][0])*w3*(-2*vop_ref[0][1] + 1)
                                   + 2*w2*h3*r*sprite_ref[0][0]
                                   - 16*w2*h3
                                   + (1<<(alpha+beta+rho-min_ab+1));
            s->sprite_offset[1][1]=  (-r*sprite_ref[0][1] + virtual_ref[0][1])*h3*(-2*vop_ref[0][0] + 1)
                                   + (-r*sprite_ref[0][1] + virtual_ref[1][1])*w3*(-2*vop_ref[0][1] + 1)
                                   + 2*w2*h3*r*sprite_ref[0][1]
                                   - 16*w2*h3
                                   + (1<<(alpha+beta+rho-min_ab+1));
            s->sprite_delta[0][0]=   (-r*sprite_ref[0][0] + virtual_ref[0][0])*h3;
            s->sprite_delta[0][1]=   (-r*sprite_ref[0][0] + virtual_ref[1][0])*w3;
            s->sprite_delta[1][0]=   (-r*sprite_ref[0][1] + virtual_ref[0][1])*h3;
            s->sprite_delta[1][1]=   (-r*sprite_ref[0][1] + virtual_ref[1][1])*w3;

            s->sprite_shift[0]= alpha + beta + rho - min_ab;
            s->sprite_shift[1]= alpha + beta + rho - min_ab + 2;
            break;
    }
    /* try to simplify the situation */
    if(   s->sprite_delta[0][0] == a<<s->sprite_shift[0]
       && s->sprite_delta[0][1] == 0
       && s->sprite_delta[1][0] == 0
       && s->sprite_delta[1][1] == a<<s->sprite_shift[0])
    {
        s->sprite_offset[0][0]>>=s->sprite_shift[0];
        s->sprite_offset[0][1]>>=s->sprite_shift[0];
        s->sprite_offset[1][0]>>=s->sprite_shift[1];
        s->sprite_offset[1][1]>>=s->sprite_shift[1];
        s->sprite_delta[0][0]= a;
        s->sprite_delta[0][1]= 0;
        s->sprite_delta[1][0]= 0;
        s->sprite_delta[1][1]= a;
        s->sprite_shift[0]= 0;
        s->sprite_shift[1]= 0;
        s->real_sprite_warping_points=1;
    }
    else{
        int shift_y= 16 - s->sprite_shift[0];
        int shift_c= 16 - s->sprite_shift[1];
//printf("shifts %d %d\n", shift_y, shift_c);
        for(i=0; i<2; i++){
            s->sprite_offset[0][i]<<= shift_y;
            s->sprite_offset[1][i]<<= shift_c;
            s->sprite_delta[0][i]<<= shift_y;
            s->sprite_delta[1][i]<<= shift_y;
            s->sprite_shift[i]= 16;
        }
        s->real_sprite_warping_points= s->num_sprite_warping_points;
    }
#if 0
printf("vop:%d:%d %d:%d %d:%d, sprite:%d:%d %d:%d %d:%d, virtual: %d:%d %d:%d\n",
    vop_ref[0][0], vop_ref[0][1],
    vop_ref[1][0], vop_ref[1][1],
    vop_ref[2][0], vop_ref[2][1],
    sprite_ref[0][0], sprite_ref[0][1],
    sprite_ref[1][0], sprite_ref[1][1],
    sprite_ref[2][0], sprite_ref[2][1],
    virtual_ref[0][0], virtual_ref[0][1],
    virtual_ref[1][0], virtual_ref[1][1]
    );

printf("offset: %d:%d , delta: %d %d %d %d, shift %d\n",
    s->sprite_offset[0][0], s->sprite_offset[0][1],
    s->sprite_delta[0][0], s->sprite_delta[0][1],
    s->sprite_delta[1][0], s->sprite_delta[1][1],
    s->sprite_shift[0]
    );
#endif
}

static int mpeg4_decode_gop_header(MpegEncContext * s, GetBitContext *gb){
    int hours, minutes, seconds;

    hours= get_bits(gb, 5);
    minutes= get_bits(gb, 6);
    skip_bits1(gb);
    seconds= get_bits(gb, 6);

    s->time_base= seconds + 60*(minutes + 60*hours);

    skip_bits1(gb);
    skip_bits1(gb);

    return 0;
}

static int decode_vol_header(MpegEncContext *s, GetBitContext *gb){
    int width, height, vo_ver_id;

    /* vol header */
    skip_bits(gb, 1); /* random access */
    s->vo_type= get_bits(gb, 8);
    if (get_bits1(gb) != 0) { /* is_ol_id */
        vo_ver_id = get_bits(gb, 4); /* vo_ver_id */
        skip_bits(gb, 3); /* vo_priority */
    } else {
        vo_ver_id = 1;
    }
//printf("vo type:%d\n",s->vo_type);
    s->aspect_ratio_info= get_bits(gb, 4);
    if(s->aspect_ratio_info == FF_ASPECT_EXTENDED){
        s->avctx->sample_aspect_ratio.num= get_bits(gb, 8); // par_width
        s->avctx->sample_aspect_ratio.den= get_bits(gb, 8); // par_height
    }else{
        s->avctx->sample_aspect_ratio= pixel_aspect[s->aspect_ratio_info];
    }

    if ((s->vol_control_parameters=get_bits1(gb))) { /* vol control parameter */
        int chroma_format= get_bits(gb, 2);
        if(chroma_format!=CHROMA_420){
            av_log(s->avctx, AV_LOG_ERROR, "illegal chroma format\n");
        }
        s->low_delay= get_bits1(gb);
        if(get_bits1(gb)){ /* vbv parameters */
            get_bits(gb, 15);   /* first_half_bitrate */
            skip_bits1(gb);     /* marker */
            get_bits(gb, 15);   /* latter_half_bitrate */
            skip_bits1(gb);     /* marker */
            get_bits(gb, 15);   /* first_half_vbv_buffer_size */
            skip_bits1(gb);     /* marker */
            get_bits(gb, 3);    /* latter_half_vbv_buffer_size */
            get_bits(gb, 11);   /* first_half_vbv_occupancy */
            skip_bits1(gb);     /* marker */
            get_bits(gb, 15);   /* latter_half_vbv_occupancy */
            skip_bits1(gb);     /* marker */
        }
    }else{
        // set low delay flag only once the smartest? low delay detection won't be overriden
        if(s->picture_number==0)
            s->low_delay=0;
    }

    s->shape = get_bits(gb, 2); /* vol shape */
    if(s->shape != RECT_SHAPE) av_log(s->avctx, AV_LOG_ERROR, "only rectangular vol supported\n");
    if(s->shape == GRAY_SHAPE && vo_ver_id != 1){
        av_log(s->avctx, AV_LOG_ERROR, "Gray shape not supported\n");
        skip_bits(gb, 4);  //video_object_layer_shape_extension
    }

    check_marker(gb, "before time_increment_resolution");

    s->avctx->time_base.den = get_bits(gb, 16);
    if(!s->avctx->time_base.den){
        av_log(s->avctx, AV_LOG_ERROR, "time_base.den==0\n");
        return -1;
    }

    s->time_increment_bits = av_log2(s->avctx->time_base.den - 1) + 1;
    if (s->time_increment_bits < 1)
        s->time_increment_bits = 1;

    check_marker(gb, "before fixed_vop_rate");

    if (get_bits1(gb) != 0) {   /* fixed_vop_rate  */
        s->avctx->time_base.num = get_bits(gb, s->time_increment_bits);
    }else
        s->avctx->time_base.num = 1;

    s->t_frame=0;

    if (s->shape != BIN_ONLY_SHAPE) {
        if (s->shape == RECT_SHAPE) {
            skip_bits1(gb);   /* marker */
            width = get_bits(gb, 13);
            skip_bits1(gb);   /* marker */
            height = get_bits(gb, 13);
            skip_bits1(gb);   /* marker */
            if(width && height && !(s->width && s->codec_tag == AV_RL32("MP4S"))){ /* they should be non zero but who knows ... */
                s->width = width;
                s->height = height;
//                printf("width/height: %d %d\n", width, height);
            }
        }

        s->progressive_sequence=
        s->progressive_frame= get_bits1(gb)^1;
        s->interlaced_dct=0;
        if(!get_bits1(gb) && (s->avctx->debug & FF_DEBUG_PICT_INFO))
            av_log(s->avctx, AV_LOG_INFO, "MPEG4 OBMC not supported (very likely buggy encoder)\n");   /* OBMC Disable */
        if (vo_ver_id == 1) {
            s->vol_sprite_usage = get_bits1(gb); /* vol_sprite_usage */
        } else {
            s->vol_sprite_usage = get_bits(gb, 2); /* vol_sprite_usage */
        }
        if(s->vol_sprite_usage==STATIC_SPRITE) av_log(s->avctx, AV_LOG_ERROR, "Static Sprites not supported\n");
        if(s->vol_sprite_usage==STATIC_SPRITE || s->vol_sprite_usage==GMC_SPRITE){
            if(s->vol_sprite_usage==STATIC_SPRITE){
                s->sprite_width = get_bits(gb, 13);
                skip_bits1(gb); /* marker */
                s->sprite_height= get_bits(gb, 13);
                skip_bits1(gb); /* marker */
                s->sprite_left  = get_bits(gb, 13);
                skip_bits1(gb); /* marker */
                s->sprite_top   = get_bits(gb, 13);
                skip_bits1(gb); /* marker */
            }
            s->num_sprite_warping_points= get_bits(gb, 6);
            if(s->num_sprite_warping_points > 3){
                av_log(s->avctx, AV_LOG_ERROR, "%d sprite_warping_points\n", s->num_sprite_warping_points);
                s->num_sprite_warping_points= 0;
                return -1;
            }
            s->sprite_warping_accuracy = get_bits(gb, 2);
            s->sprite_brightness_change= get_bits1(gb);
            if(s->vol_sprite_usage==STATIC_SPRITE)
                s->low_latency_sprite= get_bits1(gb);
        }
        // FIXME sadct disable bit if verid!=1 && shape not rect

        if (get_bits1(gb) == 1) {   /* not_8_bit */
            s->quant_precision = get_bits(gb, 4); /* quant_precision */
            if(get_bits(gb, 4)!=8) av_log(s->avctx, AV_LOG_ERROR, "N-bit not supported\n"); /* bits_per_pixel */
            if(s->quant_precision!=5) av_log(s->avctx, AV_LOG_ERROR, "quant precision %d\n", s->quant_precision);
        } else {
            s->quant_precision = 5;
        }

        // FIXME a bunch of grayscale shape things

        if((s->mpeg_quant=get_bits1(gb))){ /* vol_quant_type */
            int i, v;

            /* load default matrixes */
            for(i=0; i<64; i++){
                int j= s->dsp.idct_permutation[i];
                v= ff_mpeg4_default_intra_matrix[i];
                s->intra_matrix[j]= v;
                s->chroma_intra_matrix[j]= v;

                v= ff_mpeg4_default_non_intra_matrix[i];
                s->inter_matrix[j]= v;
                s->chroma_inter_matrix[j]= v;
                #ifdef __tmp_use_amba_permutation__
                j= s->dsp.amba_idct_permutation[i];
                v= ff_mpeg4_default_intra_matrix[i];
                s->amba_intra_matrix[j]= v;
                s->amba_chroma_intra_matrix[j]= v;

                v= ff_mpeg4_default_non_intra_matrix[i];
                s->amba_inter_matrix[j]= v;
                s->amba_chroma_inter_matrix[j]= v;
                #endif
            }

            /* load custom intra matrix */
            if(get_bits1(gb)){
                int last=0;
                for(i=0; i<64; i++){
                    int j;
                    v= get_bits(gb, 8);
                    if(v==0){
                        ambadec_assert_ffmpeg(0);
                        break;}

                    last= v;
                    j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
                    s->intra_matrix[j]= v;
                    s->chroma_intra_matrix[j]= v;
                    #ifdef __tmp_use_amba_permutation__
                    j= s->dsp.amba_idct_permutation[ ff_zigzag_direct[i] ];
                    s->amba_intra_matrix[j]= v;
                    s->amba_chroma_intra_matrix[j]= v;
                    #endif
                }

                /* replicate last value */
                for(; i<64; i++){
                    int j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
                    s->intra_matrix[j]= last;
                    s->chroma_intra_matrix[j]= last;
                    #ifdef __tmp_use_amba_permutation__
                    j= s->dsp.amba_idct_permutation[ ff_zigzag_direct[i] ];
                    s->amba_intra_matrix[j]= last;
                    s->amba_chroma_intra_matrix[j]= last;
                    #endif
                }
            }

            /* load custom non intra matrix */
            if(get_bits1(gb)){
                int last=0;
                for(i=0; i<64; i++){
                    int j;
                    v= get_bits(gb, 8);
                    if(v==0) break;

                    last= v;
                    j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
                    s->inter_matrix[j]= v;
                    s->chroma_inter_matrix[j]= v;
                    #ifdef __tmp_use_amba_permutation__
                    j= s->dsp.amba_idct_permutation[ ff_zigzag_direct[i] ];
                    s->amba_inter_matrix[j]= v;
                    s->amba_chroma_inter_matrix[j]= v;
                    #endif
                }

                /* replicate last value */
                for(; i<64; i++){
                    int j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
                    s->inter_matrix[j]= last;
                    s->chroma_inter_matrix[j]= last;
                    #ifdef __tmp_use_amba_permutation__
                    j= s->dsp.amba_idct_permutation[ ff_zigzag_direct[i] ];
                    s->amba_inter_matrix[j]= last;
                    s->amba_chroma_inter_matrix[j]= last;
                    #endif
                }
            }

            // FIXME a bunch of grayscale shape things
        }

        if(vo_ver_id != 1)
             s->quarter_sample= get_bits1(gb);
        else s->quarter_sample=0;

        if(!get_bits1(gb)){
            int pos= get_bits_count(gb);
            int estimation_method= get_bits(gb, 2);
            if(estimation_method<2){
                if(!get_bits1(gb)){
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //opaque
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //transparent
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //intra_cae
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //inter_cae
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //no_update
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //upampling
                }
                if(!get_bits1(gb)){
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //intra_blocks
                    s->cplx_estimation_trash_p += 8*get_bits1(gb); //inter_blocks
                    s->cplx_estimation_trash_p += 8*get_bits1(gb); //inter4v_blocks
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //not coded blocks
                }
                if(!check_marker(gb, "in complexity estimation part 1")){
                    skip_bits_long(gb, pos - get_bits_count(gb));
                    goto no_cplx_est;
                }
                if(!get_bits1(gb)){
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //dct_coeffs
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //dct_lines
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //vlc_syms
                    s->cplx_estimation_trash_i += 4*get_bits1(gb); //vlc_bits
                }
                if(!get_bits1(gb)){
                    s->cplx_estimation_trash_p += 8*get_bits1(gb); //apm
                    s->cplx_estimation_trash_p += 8*get_bits1(gb); //npm
                    s->cplx_estimation_trash_b += 8*get_bits1(gb); //interpolate_mc_q
                    s->cplx_estimation_trash_p += 8*get_bits1(gb); //forwback_mc_q
                    s->cplx_estimation_trash_p += 8*get_bits1(gb); //halfpel2
                    s->cplx_estimation_trash_p += 8*get_bits1(gb); //halfpel4
                }
                if(!check_marker(gb, "in complexity estimation part 2")){
                    skip_bits_long(gb, pos - get_bits_count(gb));
                    goto no_cplx_est;
                }
                if(estimation_method==1){
                    s->cplx_estimation_trash_i += 8*get_bits1(gb); //sadct
                    s->cplx_estimation_trash_p += 8*get_bits1(gb); //qpel
                }
            }else
                av_log(s->avctx, AV_LOG_ERROR, "Invalid Complexity estimation method %d\n", estimation_method);
        }else{
no_cplx_est:
            s->cplx_estimation_trash_i=
            s->cplx_estimation_trash_p=
            s->cplx_estimation_trash_b= 0;
        }

        s->resync_marker= !get_bits1(gb); /* resync_marker_disabled */

        s->data_partitioning= get_bits1(gb);
        if(s->data_partitioning){
            s->rvlc= get_bits1(gb);
        }

        if(vo_ver_id != 1) {
            s->new_pred= get_bits1(gb);
            if(s->new_pred){
                av_log(s->avctx, AV_LOG_ERROR, "new pred not supported\n");
                skip_bits(gb, 2); /* requested upstream message type */
                skip_bits1(gb); /* newpred segment type */
            }
            s->reduced_res_vop= get_bits1(gb);
            if(s->reduced_res_vop) av_log(s->avctx, AV_LOG_ERROR, "reduced resolution VOP not supported\n");
        }
        else{
            s->new_pred=0;
            s->reduced_res_vop= 0;
        }

        s->scalability_real=
        s->scalability= get_bits1(gb);

        if (s->scalability) {
            GetBitContext bak= *gb;
            int ref_layer_id;
            int ref_layer_sampling_dir;
            int h_sampling_factor_n;
            int h_sampling_factor_m;
            int v_sampling_factor_n;
            int v_sampling_factor_m;

            s->hierachy_type= get_bits1(gb);
            ref_layer_id= get_bits(gb, 4);
            ref_layer_sampling_dir= get_bits1(gb);
            h_sampling_factor_n= get_bits(gb, 5);
            h_sampling_factor_m= get_bits(gb, 5);
            v_sampling_factor_n= get_bits(gb, 5);
            v_sampling_factor_m= get_bits(gb, 5);
            s->enhancement_type= get_bits1(gb);
            av_log(s->avctx, AV_LOG_DEBUG, "ref_layer_id=%d, ref_layer_sampling_dir=%d.\n",ref_layer_id, ref_layer_sampling_dir);

            if(   h_sampling_factor_n==0 || h_sampling_factor_m==0
               || v_sampling_factor_n==0 || v_sampling_factor_m==0){

//                fprintf(stderr, "illegal scalability header (VERY broken encoder), trying to workaround\n");
                s->scalability=0;

                *gb= bak;
            }else
                av_log(s->avctx, AV_LOG_ERROR, "scalability not supported\n");

            // bin shape stuff FIXME
        }
    }
    return 0;
}

/**
 * decodes the user data stuff in the header.
 * Also initializes divx/xvid/lavc_version/build.
 */
static int decode_user_data(MpegEncContext *s, GetBitContext *gb){
    char buf[256];
    int i;
    int e;
    int ver = 0, build = 0, ver2 = 0, ver3 = 0;
    char last;

    for(i=0; i<255 && get_bits_count(gb) < gb->size_in_bits; i++){
        if(show_bits(gb, 23) == 0) break;
        buf[i]= get_bits(gb, 8);
    }
    buf[i]=0;

    /* divx detection */
    e=sscanf(buf, "DivX%dBuild%d%c", &ver, &build, &last);
    if(e<2)
        e=sscanf(buf, "DivX%db%d%c", &ver, &build, &last);
    if(e>=2){
        s->divx_version= ver;
        s->divx_build= build;
        s->divx_packed= e==3 && last=='p';
        if(s->divx_packed && !s->showed_packed_warning) {
            av_log(s->avctx, AV_LOG_WARNING, "Invalid and inefficient vfw-avi packed B frames detected\n");
            s->showed_packed_warning=1;
        }
    }

    /* ffmpeg detection */
    e=sscanf(buf, "FFmpe%*[^b]b%d", &build)+3;
    if(e!=4)
        e=sscanf(buf, "FFmpeg v%d.%d.%d / libavcodec build: %d", &ver, &ver2, &ver3, &build);
    if(e!=4){
        e=sscanf(buf, "Lavc%d.%d.%d", &ver, &ver2, &ver3)+1;
        if (e>1)
            build= (ver<<16) + (ver2<<8) + ver3;
    }
    if(e!=4){
        if(strcmp(buf, "ffmpeg")==0){
            s->lavc_build= 4600;
        }
    }
    if(e==4){
        s->lavc_build= build;
    }

    /* Xvid detection */
    e=sscanf(buf, "XviD%d", &build);
    if(e==1){
        s->xvid_build= build;
    }

//printf("User Data: %s\n", buf);
    return 0;
}

static int decode_vop_header_amba(MpegEncContext *s, GetBitContext *gb){
    int time_incr, time_increment;

    s->pict_type = get_bits(gb, 2) + FF_I_TYPE;        /* pict type: I = 0 , P = 1 */
    if(s->pict_type==FF_B_TYPE && s->low_delay && s->vol_control_parameters==0 && !(s->flags & CODEC_FLAG_LOW_DELAY)){
        av_log(s->avctx, AV_LOG_ERROR, "low_delay flag incorrectly, clearing it\n");
        s->low_delay=0;
    }

    s->partitioned_frame= s->data_partitioning && s->pict_type!=FF_B_TYPE;
    ambadec_assert_ffmpeg(!s->partitioned_frame);
//    if(s->partitioned_frame)
//        s->decode_mb= mpeg4_decode_partitioned_mb_amba;
//    else
        s->decode_mb= ff_mpeg4_decode_mb_amba;

    time_incr=0;
    while (get_bits1(gb) != 0)
        time_incr++;

    check_marker(gb, "before time_increment");

    if(s->time_increment_bits==0 || !(show_bits(gb, s->time_increment_bits+1)&1)){
        av_log(s->avctx, AV_LOG_ERROR, "hmm, seems the headers are not complete, trying to guess time_increment_bits\n");

        for(s->time_increment_bits=1 ;s->time_increment_bits<16; s->time_increment_bits++){
            if(show_bits(gb, s->time_increment_bits+1)&1) break;
        }

        av_log(s->avctx, AV_LOG_ERROR, "my guess is %d bits ;)\n",s->time_increment_bits);
    }

    if(IS_3IV1) time_increment= get_bits1(gb); //FIXME investigate further
    else time_increment= get_bits(gb, s->time_increment_bits);

//    printf("%d %X\n", s->time_increment_bits, time_increment);
//av_log(s->avctx, AV_LOG_DEBUG, " type:%d modulo_time_base:%d increment:%d t_frame %d\n", s->pict_type, time_incr, time_increment, s->t_frame);
    if(s->pict_type!=FF_B_TYPE){
        s->last_time_base= s->time_base;
        s->time_base+= time_incr;
        s->time= s->time_base*s->avctx->time_base.den + time_increment;
        if(s->workaround_bugs&FF_BUG_UMP4){
            if(s->time < s->last_non_b_time){
//                fprintf(stderr, "header is not mpeg4 compatible, broken encoder, trying to workaround\n");
                s->time_base++;
                s->time+= s->avctx->time_base.den;
            }
        }
        s->pp_time= s->time - s->last_non_b_time;
        s->last_non_b_time= s->time;
    }else{
        s->time= (s->last_time_base + time_incr)*s->avctx->time_base.den + time_increment;
        s->pb_time= s->pp_time - (s->last_non_b_time - s->time);
        if(s->pp_time <=s->pb_time || s->pp_time <= s->pp_time - s->pb_time || s->pp_time<=0){
//            printf("messed up order, maybe after seeking? skipping current b frame\n");
            return FRAME_SKIPPED;
        }
        ff_mpeg4_init_direct_mv(s);

        if(s->t_frame==0) s->t_frame= s->pb_time;
        if(s->t_frame==0) s->t_frame=1; // 1/0 protection
        s->pp_field_time= (  ROUNDED_DIV(s->last_non_b_time, s->t_frame)
                           - ROUNDED_DIV(s->last_non_b_time - s->pp_time, s->t_frame))*2;
        s->pb_field_time= (  ROUNDED_DIV(s->time, s->t_frame)
                           - ROUNDED_DIV(s->last_non_b_time - s->pp_time, s->t_frame))*2;
        if(!s->progressive_sequence){
            if(s->pp_field_time <= s->pb_field_time || s->pb_field_time <= 1)
                return FRAME_SKIPPED;
        }
    }
//av_log(s->avctx, AV_LOG_DEBUG, "last nonb %"PRId64" last_base %d time %"PRId64" pp %d pb %d t %d ppf %d pbf %d\n", s->last_non_b_time, s->last_time_base, s->time, s->pp_time, s->pb_time, s->t_frame, s->pp_field_time, s->pb_field_time);

    /*if(s->avctx->time_base.num)
        s->current_picture_ptr->pts= (s->time + s->avctx->time_base.num/2) / s->avctx->time_base.num;
    else
        s->current_picture_ptr->pts= AV_NOPTS_VALUE;
    if(s->avctx->debug&FF_DEBUG_PTS)
        av_log(s->avctx, AV_LOG_DEBUG, "MPEG4 PTS: %"PRId64"\n", s->current_picture_ptr->pts);*/

    check_marker(gb, "before vop_coded");

    /* vop coded */
    if (get_bits1(gb) != 1){
        if(s->avctx->debug&FF_DEBUG_PICT_INFO)
            av_log(s->avctx, AV_LOG_ERROR, "vop not coded\n");
        return FRAME_SKIPPED;
    }
//printf("time %d %d %d || %"PRId64" %"PRId64" %"PRId64"\n", s->time_increment_bits, s->avctx->time_base.den, s->time_base,
//s->time, s->last_non_b_time, s->last_non_b_time - s->pp_time);
    if (s->shape != BIN_ONLY_SHAPE && ( s->pict_type == FF_P_TYPE
                          || (s->pict_type == FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE))) {
        /* rounding type for motion estimation */
        s->no_rounding = get_bits1(gb);
    } else {
        s->no_rounding = 0;
    }
//FIXME reduced res stuff

     if (s->shape != RECT_SHAPE) {
         if (s->vol_sprite_usage != 1 || s->pict_type != FF_I_TYPE) {
             int width, height, hor_spat_ref, ver_spat_ref;

             width = get_bits(gb, 13);
             skip_bits1(gb);   /* marker */
             height = get_bits(gb, 13);
             skip_bits1(gb);   /* marker */
             hor_spat_ref = get_bits(gb, 13); /* hor_spat_ref */
             skip_bits1(gb);   /* marker */
             ver_spat_ref = get_bits(gb, 13); /* ver_spat_ref */
             av_log(s->avctx, AV_LOG_DEBUG, "width=%d, height=%d, hor_spat_ref=%d, ver_spat_ref=%d\n",
                width, height, hor_spat_ref, ver_spat_ref);
         }
         skip_bits1(gb); /* change_CR_disable */

         if (get_bits1(gb) != 0) {
             skip_bits(gb, 8); /* constant_alpha_value */
         }
     }
//FIXME complexity estimation stuff

     if (s->shape != BIN_ONLY_SHAPE) {
         skip_bits_long(gb, s->cplx_estimation_trash_i);
         if(s->pict_type != FF_I_TYPE)
            skip_bits_long(gb, s->cplx_estimation_trash_p);
         if(s->pict_type == FF_B_TYPE)
            skip_bits_long(gb, s->cplx_estimation_trash_b);

         s->intra_dc_threshold= mpeg4_dc_threshold[ get_bits(gb, 3) ];
         if(!s->progressive_sequence){
             s->top_field_first= get_bits1(gb);
             s->alternate_scan= get_bits1(gb);
         }else
             s->alternate_scan= 0;
     }

     if(s->alternate_scan){
        ff_init_scantable(s->dsp.idct_permutation, &s->inter_scantable  , ff_alternate_vertical_scan);
        ff_init_scantable(s->dsp.idct_permutation, &s->intra_scantable  , ff_alternate_vertical_scan);
        ff_init_scantable(s->dsp.idct_permutation, &s->intra_h_scantable, ff_alternate_vertical_scan);
        ff_init_scantable(s->dsp.idct_permutation, &s->intra_v_scantable, ff_alternate_vertical_scan);
        #ifdef __tmp_use_amba_permutation__
        ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_inter_scantable  , ff_alternate_vertical_scan);
        ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_intra_scantable  , ff_alternate_vertical_scan);
        ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_intra_h_scantable, ff_alternate_vertical_scan);
        ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_intra_v_scantable, ff_alternate_vertical_scan);
        #endif
     } else{
         ff_init_scantable(s->dsp.idct_permutation, &s->inter_scantable  , ff_zigzag_direct);
         ff_init_scantable(s->dsp.idct_permutation, &s->intra_scantable  , ff_zigzag_direct);
         ff_init_scantable(s->dsp.idct_permutation, &s->intra_h_scantable, ff_alternate_horizontal_scan);
         ff_init_scantable(s->dsp.idct_permutation, &s->intra_v_scantable, ff_alternate_vertical_scan);
         #ifdef __tmp_use_amba_permutation__
         ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_inter_scantable  , ff_zigzag_direct);
         ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_intra_scantable  , ff_zigzag_direct);
         ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_intra_h_scantable, ff_alternate_horizontal_scan);
         ff_init_scantable(s->dsp.amba_idct_permutation, &s->amba_intra_v_scantable, ff_alternate_vertical_scan);
         #endif
     }

     if(s->pict_type == FF_S_TYPE && (s->vol_sprite_usage==STATIC_SPRITE || s->vol_sprite_usage==GMC_SPRITE)){
         mpeg4_decode_sprite_trajectory(s, gb);
         if(s->sprite_brightness_change) av_log(s->avctx, AV_LOG_ERROR, "sprite_brightness_change not supported\n");
         if(s->vol_sprite_usage==STATIC_SPRITE) av_log(s->avctx, AV_LOG_ERROR, "static sprite not supported\n");
     }

     if (s->shape != BIN_ONLY_SHAPE) {
         s->chroma_qscale= s->qscale = get_bits(gb, s->quant_precision);
         if(s->qscale==0){
             av_log(s->avctx, AV_LOG_ERROR, "Error, header damaged or not MPEG4 header (qscale=0)\n");
             return -1; // makes no sense to continue, as there is nothing left from the image then
         }

         if (s->pict_type != FF_I_TYPE) {
             s->f_code = get_bits(gb, 3);       /* fcode_for */
             if(s->f_code==0){
                 av_log(s->avctx, AV_LOG_ERROR, "Error, header damaged or not MPEG4 header (f_code=0)\n");
                 return -1; // makes no sense to continue, as the MV decoding will break very quickly
             }
         }else
             s->f_code=1;

         if (s->pict_type == FF_B_TYPE) {
             s->b_code = get_bits(gb, 3);
         }else
             s->b_code=1;

         if(s->avctx->debug&FF_DEBUG_PICT_INFO){
             av_log(s->avctx, AV_LOG_DEBUG, "qp:%d fc:%d,%d %s size:%d pro:%d alt:%d top:%d %spel part:%d resync:%d w:%d a:%d rnd:%d vot:%d%s dc:%d ce:%d/%d/%d\n",
                 s->qscale, s->f_code, s->b_code,
                 s->pict_type == FF_I_TYPE ? "I" : (s->pict_type == FF_P_TYPE ? "P" : (s->pict_type == FF_B_TYPE ? "B" : "S")),
                 gb->size_in_bits,s->progressive_sequence, s->alternate_scan, s->top_field_first,
                 s->quarter_sample ? "q" : "h", s->data_partitioning, s->resync_marker, s->num_sprite_warping_points,
                 s->sprite_warping_accuracy, 1-s->no_rounding, s->vo_type, s->vol_control_parameters ? " VOLC" : " ", s->intra_dc_threshold, s->cplx_estimation_trash_i, s->cplx_estimation_trash_p, s->cplx_estimation_trash_b);
         }

         if(!s->scalability){
             if (s->shape!=RECT_SHAPE && s->pict_type!=FF_I_TYPE) {
                 skip_bits1(gb); // vop shape coding type
             }
         }else{
             if(s->enhancement_type){
                 int load_backward_shape= get_bits1(gb);
                 if(load_backward_shape){
                     av_log(s->avctx, AV_LOG_ERROR, "load backward shape isn't supported\n");
                 }
             }
             skip_bits(gb, 2); //ref_select_code
         }
     }
     /* detect buggy encoders which don't set the low_delay flag (divx4/xvid/opendivx)*/
     // note we cannot detect divx5 without b-frames easily (although it's buggy too)
     if(s->vo_type==0 && s->vol_control_parameters==0 && s->divx_version==0 && s->picture_number==0){
         av_log(s->avctx, AV_LOG_ERROR, "looks like this file was encoded with (divx4/(old)xvid/opendivx) -> forcing low_delay flag\n");
         s->low_delay=1;
     }

     s->picture_number++; // better than pic number==0 always ;)

     s->y_dc_scale_table= ff_mpeg4_y_dc_scale_table; //FIXME add short header support
     s->c_dc_scale_table= ff_mpeg4_c_dc_scale_table;

     if(s->workaround_bugs&FF_BUG_EDGE){
         s->h_edge_pos= s->width;
         s->v_edge_pos= s->height;
     }
     return 0;
}

/**
 * decode mpeg4 headers
 * @return <0 if no VOP found (or a damaged one)
 *         FRAME_SKIPPED if a not coded VOP is found
 *         0 if a VOP is found
 */
int ff_mpeg4_decode_picture_header_amba(MpegEncContext * s, GetBitContext *gb)
{
    int startcode, v;

    /* search next start code */
    align_get_bits(gb);

    if(s->codec_tag == AV_RL32("WV1F") && show_bits(gb, 24) == 0x575630){
//        av_log(NULL, AV_LOG_ERROR, "check path1.\n");

        skip_bits(gb, 24);
        if(get_bits(gb, 8) == 0xF0)
            goto end;
    }
//    av_log(NULL, AV_LOG_ERROR, "check path2.\n");

    startcode = 0xff;
    for(;;) {
//        av_log(NULL, AV_LOG_ERROR, "***pre-get get_bits_count(gb)=%d,gb->size_in_bits=%d.\n",get_bits_count(gb),gb->size_in_bits);

        if(get_bits_count(gb) >= gb->size_in_bits){
//            av_log(NULL, AV_LOG_ERROR, "***in here,gb->size_in_bits=%d, s->divx_version=%d,s->xvid_build=%d.\n",gb->size_in_bits,s->divx_version,s->xvid_build);

            if(gb->size_in_bits==8 && (s->divx_version || s->xvid_build)){
//                av_log(s->avctx, AV_LOG_ERROR, "frame skip %d\n", gb->size_in_bits);
                return FRAME_SKIPPED; //divx bug
            }else
            {
//                av_log(NULL, AV_LOG_ERROR, "****end of stream,gb->size_in_bits=%d, s->divx_version=%d,s->xvid_build=%d.\n",gb->size_in_bits,s->divx_version,s->xvid_build);
                return -1; //end of stream
            }
        }

        /* use the bits after the test */
        v = get_bits(gb, 8);
        startcode = ((startcode << 8) | v) & 0xffffffff;

        if((startcode&0xFFFFFF00) != 0x100)
            continue; //no startcode

        if(s->avctx->debug&FF_DEBUG_STARTCODE){
            av_log(s->avctx, AV_LOG_DEBUG, "startcode: %3X ", startcode);
            if     (startcode<=0x11F) av_log(s->avctx, AV_LOG_DEBUG, "Video Object Start");
            else if(startcode<=0x12F) av_log(s->avctx, AV_LOG_DEBUG, "Video Object Layer Start");
            else if(startcode<=0x13F) av_log(s->avctx, AV_LOG_DEBUG, "Reserved");
            else if(startcode<=0x15F) av_log(s->avctx, AV_LOG_DEBUG, "FGS bp start");
            else if(startcode<=0x1AF) av_log(s->avctx, AV_LOG_DEBUG, "Reserved");
            else if(startcode==0x1B0) av_log(s->avctx, AV_LOG_DEBUG, "Visual Object Seq Start");
            else if(startcode==0x1B1) av_log(s->avctx, AV_LOG_DEBUG, "Visual Object Seq End");
            else if(startcode==0x1B2) av_log(s->avctx, AV_LOG_DEBUG, "User Data");
            else if(startcode==0x1B3) av_log(s->avctx, AV_LOG_DEBUG, "Group of VOP start");
            else if(startcode==0x1B4) av_log(s->avctx, AV_LOG_DEBUG, "Video Session Error");
            else if(startcode==0x1B5) av_log(s->avctx, AV_LOG_DEBUG, "Visual Object Start");
            else if(startcode==0x1B6) av_log(s->avctx, AV_LOG_DEBUG, "Video Object Plane start");
            else if(startcode==0x1B7) av_log(s->avctx, AV_LOG_DEBUG, "slice start");
            else if(startcode==0x1B8) av_log(s->avctx, AV_LOG_DEBUG, "extension start");
            else if(startcode==0x1B9) av_log(s->avctx, AV_LOG_DEBUG, "fgs start");
            else if(startcode==0x1BA) av_log(s->avctx, AV_LOG_DEBUG, "FBA Object start");
            else if(startcode==0x1BB) av_log(s->avctx, AV_LOG_DEBUG, "FBA Object Plane start");
            else if(startcode==0x1BC) av_log(s->avctx, AV_LOG_DEBUG, "Mesh Object start");
            else if(startcode==0x1BD) av_log(s->avctx, AV_LOG_DEBUG, "Mesh Object Plane start");
            else if(startcode==0x1BE) av_log(s->avctx, AV_LOG_DEBUG, "Still Texture Object start");
            else if(startcode==0x1BF) av_log(s->avctx, AV_LOG_DEBUG, "Texture Spatial Layer start");
            else if(startcode==0x1C0) av_log(s->avctx, AV_LOG_DEBUG, "Texture SNR Layer start");
            else if(startcode==0x1C1) av_log(s->avctx, AV_LOG_DEBUG, "Texture Tile start");
            else if(startcode==0x1C2) av_log(s->avctx, AV_LOG_DEBUG, "Texture Shape Layer start");
            else if(startcode==0x1C3) av_log(s->avctx, AV_LOG_DEBUG, "stuffing start");
            else if(startcode<=0x1C5) av_log(s->avctx, AV_LOG_DEBUG, "reserved");
            else if(startcode<=0x1FF) av_log(s->avctx, AV_LOG_DEBUG, "System start");
            av_log(s->avctx, AV_LOG_DEBUG, " at %d\n", get_bits_count(gb));
        }

        if(startcode >= 0x120 && startcode <= 0x12F){
            if(decode_vol_header(s, gb) < 0)
            {
                av_log(s->avctx, AV_LOG_ERROR, "****error when decode_vol_header.\n");
                return -1;
            }
        }
        else if(startcode == USER_DATA_STARTCODE){
            decode_user_data(s, gb);
        }
        else if(startcode == GOP_STARTCODE){
            mpeg4_decode_gop_header(s, gb);
        }
        else if(startcode == VOP_STARTCODE){
            break;
        }

        align_get_bits(gb);
        startcode = 0xff;
    }
end:
    if(s->flags& CODEC_FLAG_LOW_DELAY)
        s->low_delay=1;
    s->avctx->has_b_frames= !s->low_delay;
    return decode_vop_header_amba(s, gb);
}

