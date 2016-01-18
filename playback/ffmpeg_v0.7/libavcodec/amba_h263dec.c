/*
 * H.263 nv12 parallel decoder
 * Copyright (c) 2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2010 ambarella Zhi He <ayu3405@gmail.com>
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
 * @file libavcodec/amba_h263dec.c
 * H.263/mpeg4 parallel nv12 decoder.
 */
#include <unistd.h>
#include "internal.h"
#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "h263_parser.h"
#include "mpeg4video_parser.h"
#include "msmpeg4.h"
#include "amba_dec_util.h"
#include "amba_dsp_define.h"
#include "amba_h263.h"
#include "log_dump.h"

//#define DEBUG
//#define PRINT_FRAME_TIME

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

static void interpret_gmc1_pre(h263_pic_data_t* p_pic, mb_mv_P_t* pmvp, int mbx, int mby)
{
    int motion_x= p_pic->sprite_offset[0][0];
    int motion_y= p_pic->sprite_offset[0][1];
    int src_x = mbx * 16 + (motion_x >> (p_pic->sprite_warping_accuracy+1));
    int src_y = mby * 16 + (motion_y >> (p_pic->sprite_warping_accuracy+1));

    motion_x<<=(3-p_pic->sprite_warping_accuracy);
    motion_y<<=(3-p_pic->sprite_warping_accuracy);
    src_x = av_clip(src_x, -16, p_pic->width);
    if (src_x == p_pic->width)
        motion_x =0;
    src_y = av_clip(src_y, -16, p_pic->height);
    if (src_y == p_pic->height)
        motion_y =0;

    pmvp->mb_setting.mv_type=mv_type_16x16;
    pmvp->mv[0].mv_x=(((src_x-mbx*16)<<4)|(motion_x&0xf));
    pmvp->mv[0].mv_y=(((src_y-mby*16)<<4)|(motion_y&0xf));
    if(!p_pic->quarter_sample)
    {
        pmvp->mv[0].mv_x>>=1;
        pmvp->mv[0].mv_y>>=1;
    }
    pmvp->mv[0].valid=1;
}

static void interpret_gmc23_pre(h263_pic_data_t* p_pic, mb_mv_P_t* pmvp, int mbx, int mby)
{
    int ox= p_pic->sprite_offset[0][0] + p_pic->sprite_delta[0][0]*(mbx*16+8) + p_pic->sprite_delta[0][1]*(mbx*16+8);
    int oy= p_pic->sprite_offset[0][1] + p_pic->sprite_delta[1][0]*(mbx*16+8) + p_pic->sprite_delta[1][1]*(mbx*16+8);

    pmvp->mb_setting.mv_type=mv_type_16x16;
    ox>>=16;
    oy>>=16;
    pmvp->mv[0].mv_x=((((ox>>(p_pic->sprite_warping_accuracy+1))-(mbx*16+8))<<4)|(ox<<(3-p_pic->sprite_warping_accuracy)));
    pmvp->mv[0].mv_y=((((oy>>(p_pic->sprite_warping_accuracy+1))-(mby*16+8))<<4)|(oy<<(3-p_pic->sprite_warping_accuracy)));
    pmvp->mv[0].valid=1;
    if(!p_pic->quarter_sample)
    {
        pmvp->mv[0].mv_x>>=1;
        pmvp->mv[0].mv_y>>=1;
    }
}

//interpret gmc
static void interpret_gmc1(h263_pic_data_t* p_pic, mb_mv_P_t* pmvp, int mbx, int mby)
{
    int motion_x= p_pic->sprite_offset[0][0];
    int motion_y= p_pic->sprite_offset[0][1];
    int src_x = mbx * 16 + (motion_x >> (p_pic->sprite_warping_accuracy+1));
    int src_y = mby * 16 + (motion_y >> (p_pic->sprite_warping_accuracy+1));
    uint32_t* pv,v;

    motion_x<<=(3-p_pic->sprite_warping_accuracy);
    motion_y<<=(3-p_pic->sprite_warping_accuracy);
    src_x = av_clip(src_x, -16, p_pic->width);
    if (src_x == p_pic->width)
        motion_x =0;
    src_y = av_clip(src_y, -16, p_pic->height);
    if (src_y == p_pic->height)
        motion_y =0;

    pmvp->mb_setting.mv_type=mv_type_16x16;
    pmvp->mv[0].mv_x=((src_x-mbx*16)<<4)|(motion_x&0xf);
    pmvp->mv[0].mv_y=((src_y-mby*16)<<4)|(motion_y&0xf);
    //pmvp->mv[0].valid=1;
    pmvp->mv[0].ref_frame_num=forward_ref_num;
    pv=(uint32_t *)&pmvp->mv[0];
    v=*pv++;
    *pv++=v;
    *pv++=v;
    *pv=v;

    motion_x= p_pic->sprite_offset[1][0];
    motion_y= p_pic->sprite_offset[1][1];
    src_x = mbx * 8 + (motion_x >> (p_pic->sprite_warping_accuracy+1));
    src_y = mby * 8 + (motion_y >> (p_pic->sprite_warping_accuracy+1));
    motion_x<<=(3-p_pic->sprite_warping_accuracy);
    motion_y<<=(3-p_pic->sprite_warping_accuracy);
    src_x = av_clip(src_x, -8, p_pic->width>>1);
    if (src_x == p_pic->width>>1)
        motion_x =0;
    src_y = av_clip(src_y, -8, p_pic->height>>1);
    if (src_y == p_pic->height>>1)
        motion_y =0;

    pmvp->mv[4].mv_x= pmvp->mv[5].mv_x=((src_x-mbx*8)<<4)|(motion_x&0xf);
    pmvp->mv[4].mv_y= pmvp->mv[5].mv_y=((src_y-mby*8)<<4)|(motion_y&0xf);
    //pmvp->mv[4].valid= pmvp->mv[5].valid=1;
    pmvp->mv[4].ref_frame_num=pmvp->mv[5].ref_frame_num=forward_ref_num;

}

static void interpret_gmc23(h263_pic_data_t* p_pic, mb_mv_P_t* pmvp, int mbx, int mby)
{
    int ox= p_pic->sprite_offset[0][0] + p_pic->sprite_delta[0][0]*(mbx*16+8) + p_pic->sprite_delta[0][1]*(mbx*16+8);
    int oy= p_pic->sprite_offset[0][1] + p_pic->sprite_delta[1][0]*(mbx*16+8) + p_pic->sprite_delta[1][1]*(mbx*16+8);
    uint32_t* pv,v;

    pmvp->mb_setting.mv_type=mv_type_16x16;
    ox>>=16;
    oy>>=16;
    pmvp->mv[0].mv_x=(((ox>>(p_pic->sprite_warping_accuracy+1))-(mbx*16+8))<<4)|(ox<<(3-p_pic->sprite_warping_accuracy));
    pmvp->mv[0].mv_y=(((oy>>(p_pic->sprite_warping_accuracy+1))-(mby*16+8))<<4)|(oy<<(3-p_pic->sprite_warping_accuracy));
    //pmvp->mv[0].valid=1;
    pmvp->mv[0].ref_frame_num=forward_ref_num;
    pv=(uint32_t *)&pmvp->mv[0];
    v=*pv++;
    *pv++=v;
    *pv++=v;
    *pv=v;

    ox= p_pic->sprite_offset[1][0] + p_pic->sprite_delta[0][0]*(mbx*8+4) + p_pic->sprite_delta[0][1]*(mby*8+4);
    oy= p_pic->sprite_offset[1][1] + p_pic->sprite_delta[1][0]*(mbx*8+4) + p_pic->sprite_delta[1][1]*(mby*8+4);

    pmvp->mv[4].mv_x = pmvp->mv[5].mv_x=(((ox>>(p_pic->sprite_warping_accuracy+1))-(mbx*8+4))<<4)|(ox<<(3-p_pic->sprite_warping_accuracy));
    pmvp->mv[4].mv_x = pmvp->mv[5].mv_y=(((oy>>(p_pic->sprite_warping_accuracy+1))-(mby*8+4))<<4)|(oy<<(3-p_pic->sprite_warping_accuracy));
    //pmvp->mv[4].valid= pmvp->mv[5].valid=1;
    pmvp->mv[4].ref_frame_num=pmvp->mv[5].ref_frame_num=forward_ref_num;
}

/**
 * returns the number of bytes consumed for building the current frame

static int get_consumed_bytes(MpegEncContext *s, int buf_size){
    int pos= (get_bits_count(&s->gb)+7)>>3;

    if(s->divx_packed || s->avctx->hwaccel){
        //we would have to scan through the whole buf to handle the weird reordering ...
        return buf_size;
    }else if(s->flags&CODEC_FLAG_TRUNCATED){
        pos -= s->parse_context.last_index;
        if(pos<0) pos=0; // padding is not really read so this might be -1
        return pos;
    }else{
        if(pos==0) pos=1; //avoid infinite loops (i doubt that is needed but ...)
        if(pos+10>buf_size) pos=buf_size; // oops ;)

        return pos;
    }
}
 */
static int decode_slice_amba(MpegEncContext *s){
    const int part_mask= s->partitioned_frame ? (AC_END|AC_ERROR) : 0x7F;
    const int mb_size= 16>>s->avctx->lowres;
    H263AmbaDecContext_t* thiz=(H263AmbaDecContext_t*)s;
    short* pdct;
    mb_mv_P_t* pmvp;
    int xy;

    s->last_resync_gb= s->gb;
    s->first_slice_line= 1;

    s->resync_mb_x= s->mb_x;
    s->resync_mb_y= s->mb_y;

    ff_set_qscale(s, s->qscale);

#if 0
//#ifdef __dump_temp__
    char tempc[80];
    snprintf(tempc,79,"decode_slice s->mb_x=%d,s->mb_y=%d,s->codec_id=%d",s->mb_x,s->mb_y,s->codec_id);
    log_text(log_fd_temp, tempc);
    snprintf(tempc,79," mb_size=%d,part_mask=%d,s->partitioned_frame=%d",mb_size,part_mask,s->partitioned_frame);
    log_text(log_fd_temp, tempc);
    snprintf(tempc,79," s->loop_filter=%d",s->loop_filter);
    log_text(log_fd_temp, tempc);
#endif

    if(s->partitioned_frame){
        const int qscale= s->qscale;

        if(s->codec_id==CODEC_ID_AMBA_P_MPEG4){
            if(ff_mpeg4_decode_partitions(s) < 0)
                return -1;
        }

        /* restore variables which were modified */
        s->first_slice_line=1;
        s->mb_x= s->resync_mb_x;
        s->mb_y= s->resync_mb_y;
        ff_set_qscale(s, qscale);
    }
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
            ret= s->decode_mb(s, (DCTELEM (*)[64])pdct);

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

    /* try to detect the padding bug */
    if(      s->codec_id==CODEC_ID_AMBA_P_MPEG4
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
    }

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

static void _h263_reset_picture_data(H263AmbaDecContext_t *thiz)
{
//    av_log(NULL,AV_LOG_ERROR,"_reset_picture_data ppic->mb_width=%d,ppic->mb_height=%d.\n",thiz->p_pic->mb_width,thiz->p_pic->mb_height);
    //clear all data
    if (thiz->use_dsp)
        memset(thiz->p_pic->pvopinfo,0x0,CHUNKHEAD_VOPINFO_SIZE+thiz->p_pic->mb_width*thiz->p_pic->mb_height*sizeof(mb_mv_B_t));
    else
        memset(thiz->p_pic->pmvb,0x0,thiz->p_pic->mb_width*thiz->p_pic->mb_height*sizeof(mb_mv_B_t));
    memset(thiz->p_pic->pdct,0x0,thiz->p_pic->mb_width*thiz->p_pic->mb_height*768);
}

static uint32_t _get_vop_code_type(h263_pic_data_t* ppic)
{
#ifdef __dsp_interpret_gmc__
    if(ppic->current_picture_ptr->pict_type==FF_S_TYPE)
        return FF_P_TYPE-1;
    else
        return (int)ppic->current_picture_ptr->pict_type-1;
#else
    return (int)ppic->current_picture_ptr->pict_type-1;
#endif
}

static void _fill_vopinfo_at_mvfifo_chunk_head(h263_pic_data_t* ppic)
{
    vop_info_t* pvop_info=(vop_info_t*)ppic->pvopinfo;
    pvop_info->vop_width = ppic->width;
    pvop_info->vop_height = ppic->height;
    pvop_info->vop_code_type = _get_vop_code_type(ppic);
    pvop_info->vop_round_type=ppic->no_rounding;
    pvop_info->vop_interlaced=ppic->current_picture_ptr->interlaced_frame;
    pvop_info->vop_top_field_first=ppic->current_picture_ptr->top_field_first;
}

static void _h263_callback_free_vld_data(void* p)
{
    h263_vld_data_t* pvld=(h263_vld_data_t*)p;
    ambadec_assert_ffmpeg(p);
    if(pvld->pbuf)
        av_free(pvld->pbuf);
    pvld->pbuf=NULL;
    av_free(p);
}

static void _h263_callback_NULL(void* p)
{
    return;
}

static void _h263_callback_destroy_pic_data(void* p)
{
    ambadec_assert_ffmpeg(p);
    h263_pic_data_t* ppic=(h263_pic_data_t*)p;

    if(ppic->pbase)
        av_free(ppic->pbase);
    if(ppic->pdctbase)
        av_free(ppic->pdctbase);
    if(ppic->pswinfo)
        av_free(ppic->pswinfo);
    ppic->pbase = ppic->pdctbase = ppic->pswinfo = NULL;

}

static inline int ff_h263_round_chroma_mv(int x){
    static const uint8_t h263_chroma_roundtab[16] = {
    //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
        0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1,
    };
    return h263_chroma_roundtab[x & 0xf] + (x >> 3);
}

static void flush_holded_pictures(H263AmbaDecContext_t *thiz, h263_pic_data_t* p_pic)
{
    if(p_pic->last_picture_ptr)//for bug#2308 case 2: sw pipeline decoder, quick seek at file beginning, safety
    {
        if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_pic->last_picture_ptr)==1) {
        //av_log(NULL,AV_LOG_ERROR,"** Flush last %p.\n", p_pic->last_picture_ptr);
        p_pic->last_picture_ptr->type = DFlushing_frame;
        thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_pic->last_picture_ptr);
        }
    }
    if(p_pic->current_picture_ptr)
    {
        if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_pic->current_picture_ptr)==1) {
        //av_log(NULL,AV_LOG_ERROR,"** Flush current %p.\n", p_pic->current_picture_ptr);
        p_pic->current_picture_ptr->type = DFlushing_frame;
        thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_pic->current_picture_ptr);
        }
    }
    if(p_pic->next_picture_ptr)
    {
        if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_pic->next_picture_ptr)==1) {
        //av_log(NULL,AV_LOG_ERROR,"** Flush next %p.\n", p_pic->next_picture_ptr);
        p_pic->next_picture_ptr->type = DFlushing_frame;
        thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_pic->next_picture_ptr);
        }
    }
}

static int _Does_render_by_filter(H263AmbaDecContext_t *thiz)
{
    amba_decoding_accelerator_t* aac=thiz->pAcc;
    if(!aac)
        return 1;//for safety here, perhaps sw decoding without aac
    if(aac->does_render_by_filter)
        return 1;

    return 0;
}

static inline void _Change_1mv_to_4mv(h263_pic_data_t* p_pic)
{
    //change mv settings, 1mv to 4mv, interpret global mc, calculate chroma mvs
    mb_mv_P_t* pmvp=p_pic->pmvp;
    mb_mv_B_t* pmvb=p_pic->pmvb;
    uint32_t* pcp;
    int pcpv,pcpv1;
    int mo_x,mo_y;

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
                            pmvp->mb_setting.mv_type=mv_type_8x8;
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

                            pcp=(uint32_t *)&pmvp->mv[0];
                            pcpv=*pcp++;
                            *pcp++=pcpv;
                            *pcp++=pcpv;
                            *pcp=pcpv;

                            //temp scale mv by 2, 1/4 pixel
                            pmvp->mv[4].mv_x*=4;
                            pmvp->mv[4].mv_y*=4;

                            pcp=(uint32_t *)&pmvp->mv[4];
                            pcpv=*pcp++;
                            *pcp=pcpv;

                            ambadec_assert_ffmpeg(pmvp->mb_setting.mv_type==mv_type_8x8);
                            ambadec_assert_ffmpeg(pmvp->mv[0].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[1].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[2].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[3].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[4].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[5].ref_frame_num==forward_ref_num);
                        }
                        else if(pmvp->mb_setting.mv_type==mv_type_8x8)
                        {
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
                            pcp=(uint32_t *)&pmvp->mv[4];
                            pcpv=*pcp++;
                            *pcp=pcpv;

                            ambadec_assert_ffmpeg(pmvp->mv[4].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[5].ref_frame_num==forward_ref_num);
                        }
                        else if(pmvp->mb_setting.mv_type==mv_type_16x8)
                        {
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

                            ambadec_assert_ffmpeg(pmvp->mv[4].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[5].ref_frame_num==forward_ref_num);

                            #ifdef __config_fill_field_mv23__
                            pcp=&pmvp->mv[0];
                            pcp[2]=pcp[0];
                            pcp[3]=pcp[1];
                            #endif

                            #ifdef __config_fill_field_mv23_notvalid__
                            pmvp->mv[2].ref_frame_num=pmvp->mv[3].ref_frame_num=not_valid_mv;
                            #endif

                        }
                        else
                            ambadec_assert_ffmpeg(0);

                    }
                    #ifdef _config_ambadec_assert_
                    else
                    {
                        ambadec_assert_ffmpeg(pmvp->mv[0].ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvp->mv[1].ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvp->mv[2].ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvp->mv[3].ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvp->mv[4].ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvp->mv[5].ref_frame_num==not_valid_mv);
                    }
                    #endif
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

                            pmvb->mb_setting.mv_type=mv_type_8x8;

                            if(pmvb->mvp[0].fw.ref_frame_num==forward_ref_num)
                            {
                                //calculate chroma mv
                                pcpv=pmvb->mvp[0].fw.mv_x;
                                pcpv1=pmvb->mvp[0].fw.mv_y;
                                pmvb->mvp[4].fw.mv_x=((pcpv>>1)|(pcpv & 1))*4;
                                pmvb->mvp[4].fw.ref_frame_num=forward_ref_num;
                                pmvb->mvp[4].fw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;
                                pmvb->mvp[0].fw.mv_x*=2;
                                pmvb->mvp[0].fw.mv_y*=2;

                                pcp=(uint32_t *)&pmvb->mvp[0].fw;
                                pcpv=*pcp;
                                pcp[2]=pcpv;
                                pcp[4]=pcpv;
                                pcp[6]=pcpv;

                                pcp=(uint32_t *)&pmvb->mvp[4].fw;
                                pcpv=*pcp;
                                pcp[2]=pcpv;

                                ambadec_assert_ffmpeg(pmvb->mvp[0].fw.ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[1].fw.ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[2].fw.ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[3].fw.ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[4].fw.ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[5].fw.ref_frame_num==forward_ref_num);
                            }
                            else
                            {
                                pmvb->mvp[0].fw.ref_frame_num=not_valid_mv;
                                pmvb->mvp[1].fw.ref_frame_num=not_valid_mv;
                                pmvb->mvp[2].fw.ref_frame_num=not_valid_mv;
                                pmvb->mvp[3].fw.ref_frame_num=not_valid_mv;
                                pmvb->mvp[4].fw.ref_frame_num=not_valid_mv;
                                pmvb->mvp[5].fw.ref_frame_num=not_valid_mv;
                                ambadec_assert_ffmpeg(pmvb->mvp[0].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[1].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[2].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[3].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[4].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[5].fw.ref_frame_num==not_valid_mv);
                                #ifdef _config_ambadec_assert_
                                if(pmvb->mvp[0].fw.ref_frame_num!=not_valid_mv)
                                {
                                    av_log(NULL,AV_LOG_ERROR,"pmvb->mvp[0].fw.ref_frame_num=%d.\n",pmvb->mvp[0].fw.ref_frame_num);
                                }
                                #endif
                            }

                            if(pmvb->mvp[0].bw.ref_frame_num==backward_ref_num)
                            {
                                //ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num==backward_ref_num);
                                //pmvb->mvp[0].bw.ref_frame_num=backward_ref_num;
                                pcpv=pmvb->mvp[0].bw.mv_x;
                                pcpv1=pmvb->mvp[0].bw.mv_y;
                                pmvb->mvp[4].bw.mv_x=((pcpv>>1)|(pcpv & 1))*4;
                                //pmvb->mvp[4].bw.valid=1;
                                pmvb->mvp[4].bw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;
                                pmvb->mvp[0].bw.mv_x*=2;
                                pmvb->mvp[0].bw.mv_y*=2;
                                pmvb->mvp[4].bw.ref_frame_num=backward_ref_num;

                                pcp=(uint32_t *)&pmvb->mvp[0].bw;
                                pcpv=*pcp;
                                pcp[2]=pcpv;
                                pcp[4]=pcpv;
                                pcp[6]=pcpv;

                                pcp=(uint32_t *)&pmvb->mvp[4].bw;
                                pcpv=*pcp;
                                pcp[2]=pcpv;

                                ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num==backward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[1].bw.ref_frame_num==backward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[2].bw.ref_frame_num==backward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[3].bw.ref_frame_num==backward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[4].bw.ref_frame_num==backward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[5].bw.ref_frame_num==backward_ref_num);
                            }
                            else
                            {
                                pmvb->mvp[0].bw.ref_frame_num=not_valid_mv;
                                pmvb->mvp[1].bw.ref_frame_num=not_valid_mv;
                                pmvb->mvp[2].bw.ref_frame_num=not_valid_mv;
                                pmvb->mvp[3].bw.ref_frame_num=not_valid_mv;
                                pmvb->mvp[4].bw.ref_frame_num=not_valid_mv;
                                pmvb->mvp[5].bw.ref_frame_num=not_valid_mv;
                                ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[1].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[2].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[3].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[4].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[5].bw.ref_frame_num==not_valid_mv);
                                #ifdef _config_ambadec_assert_
                                if(pmvb->mvp[0].bw.ref_frame_num!=not_valid_mv)
                                {
                                    av_log(NULL,AV_LOG_ERROR,"pmvb->mvp[0].bw.ref_frame_num=%d.\n",pmvb->mvp[0].bw.ref_frame_num);
                                }
                                #endif
                            }

                            ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num==backward_ref_num || pmvb->mvp[0].fw.ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvb->mb_setting.mv_type==mv_type_8x8);
                        }
                        else if(pmvb->mb_setting.mv_type==mv_type_8x8)
                        {
                            if(pmvb->mvp[0].fw.ref_frame_num==forward_ref_num)
                            {
                                ambadec_assert_ffmpeg(pmvb->mvp[1].fw.ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[2].fw.ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[3].fw.ref_frame_num==forward_ref_num);
                                //fw
                                //ambadec_assert_ffmpeg(pmvb->mvp[1].fw.valid && pmvb->mvp[2].fw.valid && pmvb->mvp[3].fw.valid);
                                pcpv=pmvb->mvp[0].fw.mv_x+pmvb->mvp[1].fw.mv_x+pmvb->mvp[2].fw.mv_x+pmvb->mvp[3].fw.mv_x;
                                pcpv1=pmvb->mvp[0].fw.mv_y+pmvb->mvp[1].fw.mv_y+pmvb->mvp[2].fw.mv_y+pmvb->mvp[3].fw.mv_y;
                                //special rounding, from ffmpeg
                                //pmvb->mvp[4].fw.valid=1;
                                pmvb->mvp[4].fw.ref_frame_num=forward_ref_num;
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

                                pcp=(uint32_t *)&pmvb->mvp[4].fw;
                                pcpv=*pcp;
                                pcp[2]=pcpv;

                                ambadec_assert_ffmpeg(pmvb->mvp[4].fw.ref_frame_num==forward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[5].fw.ref_frame_num==forward_ref_num);

                            }
                            else
                            {
                                pmvb->mvp[4].fw.ref_frame_num=not_valid_mv;
                                pmvb->mvp[5].fw.ref_frame_num=not_valid_mv;

                                ambadec_assert_ffmpeg(pmvb->mvp[0].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[1].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[2].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[3].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[4].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[5].fw.ref_frame_num==not_valid_mv);
                                #ifdef _config_ambadec_assert_
                                if(pmvb->mvp[0].fw.ref_frame_num!=not_valid_mv)
                                {
                                    av_log(NULL,AV_LOG_ERROR,"pmvb->mvp[0].fw.ref_frame_num=%d.\n",pmvb->mvp[0].fw.ref_frame_num);
                                }
                                #endif
                            }


                            if(pmvb->mvp[0].bw.ref_frame_num==backward_ref_num)
                            {
                                ambadec_assert_ffmpeg(pmvb->mvp[1].bw.ref_frame_num==backward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[2].bw.ref_frame_num==backward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[3].bw.ref_frame_num==backward_ref_num);

                                //bw
                                //ambadec_assert_ffmpeg(pmvb->mvp[1].bw.valid && pmvb->mvp[2].bw.valid && pmvb->mvp[3].bw.valid);
                                //ambadec_assert_ffmpeg((pmvb->mvp[0].bw.ref_frame_num==backward_ref_num) && (pmvb->mvp[1].bw.ref_frame_num==backward_ref_num) && (pmvb->mvp[2].bw.ref_frame_num==backward_ref_num) && (pmvb->mvp[3].bw.ref_frame_num==backward_ref_num));
                                pcpv=pmvb->mvp[0].bw.mv_x+pmvb->mvp[1].bw.mv_x+pmvb->mvp[2].bw.mv_x+pmvb->mvp[3].bw.mv_x;
                                pcpv1=pmvb->mvp[0].bw.mv_y+pmvb->mvp[1].bw.mv_y+pmvb->mvp[2].bw.mv_y+pmvb->mvp[3].bw.mv_y;
                                //special rounding, from ffmpeg
                                //pmvb->mvp[4].bw.valid=1;
                                pmvb->mvp[4].bw.mv_x=ff_h263_round_chroma_mv(pcpv)*4;
                                pmvb->mvp[4].bw.mv_y=ff_h263_round_chroma_mv(pcpv1)*4;
                                pmvb->mvp[4].bw.ref_frame_num=backward_ref_num;

                                pmvb->mvp[0].bw.mv_x*=2;
                                pmvb->mvp[0].bw.mv_y*=2;
                                pmvb->mvp[1].bw.mv_x*=2;
                                pmvb->mvp[1].bw.mv_y*=2;
                                pmvb->mvp[2].bw.mv_x*=2;
                                pmvb->mvp[2].bw.mv_y*=2;
                                pmvb->mvp[3].bw.mv_x*=2;
                                pmvb->mvp[3].bw.mv_y*=2;

                                pcp=(uint32_t *)&pmvb->mvp[4].bw;
                                pcpv=*pcp;
                                pcp[2]=pcpv;

                                ambadec_assert_ffmpeg(pmvb->mvp[4].bw.ref_frame_num==backward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[5].bw.ref_frame_num==backward_ref_num);
                            }
                            else
                            {
                                ambadec_assert_ffmpeg(pmvb->mvp[4].bw.ref_frame_num==backward_ref_num);
                                ambadec_assert_ffmpeg(pmvb->mvp[5].bw.ref_frame_num==backward_ref_num);

                                ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[1].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[2].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[3].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[4].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[5].bw.ref_frame_num==not_valid_mv);
                                #ifdef _config_ambadec_assert_
                                if(pmvb->mvp[0].bw.ref_frame_num!=not_valid_mv)
                                {
                                    av_log(NULL,AV_LOG_ERROR,"pmvb->mvp[0].bw.ref_frame_num=%d.\n",pmvb->mvp[0].bw.ref_frame_num);
                                }
                                #endif
                            }

                            ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num==backward_ref_num || pmvb->mvp[0].fw.ref_frame_num==forward_ref_num);

                        }
                        else if(pmvb->mb_setting.mv_type==mv_type_16x8)
                        {
                            if(pmvb->mvp[0].fw.ref_frame_num==forward_ref_num)
                            {
                                //top field fw
                                pcpv=pmvb->mvp[0].fw.mv_x;
                                pcpv1=pmvb->mvp[0].fw.mv_y;
                                pmvb->mvp[4].fw.mv_x=((pcpv>>1)|(pcpv & 1))*4;
                                pmvb->mvp[4].fw.ref_frame_num=forward_ref_num;
                                pmvb->mvp[4].fw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;
                                pmvb->mvp[4].fw.bottom=pmvb->mvp[0].fw.bottom;

                                pmvb->mvp[0].fw.mv_x*=2;
                                pmvb->mvp[0].fw.mv_y*=2;

                                ambadec_assert_ffmpeg(pmvb->mvp[4].fw.ref_frame_num==forward_ref_num);
                            }
                            #ifdef _config_ambadec_assert_
                            else
                            {
                                pmvb->mvp[4].fw.ref_frame_num=not_valid_mv;
                                ambadec_assert_ffmpeg(pmvb->mvp[0].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[4].fw.ref_frame_num==not_valid_mv);
                                if(pmvb->mvp[0].fw.ref_frame_num!=not_valid_mv)
                                {
                                    av_log(NULL,AV_LOG_ERROR,"pmvb->mvp[0].fw.ref_frame_num=%d.\n",pmvb->mvp[0].fw.ref_frame_num);
                                }
                            }
                            #endif


                            if(pmvb->mvp[0].bw.ref_frame_num==backward_ref_num)
                            {
                                //ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num==backward_ref_num);
                                //top field bw
                                //pmvb->mvp[0].bw.ref_frame_num=backward_ref_num;
                                pcpv=pmvb->mvp[0].bw.mv_x;
                                pcpv1=pmvb->mvp[0].bw.mv_y;
                                pmvb->mvp[4].bw.mv_x=((pcpv>>1)|(pcpv & 1))*4;
                                //pmvb->mvp[4].bw.valid=1;
                                pmvb->mvp[4].bw.ref_frame_num=backward_ref_num;
                                pmvb->mvp[4].bw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;
                                pmvb->mvp[4].bw.bottom=pmvb->mvp[0].bw.bottom;

                                pmvb->mvp[0].bw.mv_x*=2;
                                pmvb->mvp[0].bw.mv_y*=2;

                                ambadec_assert_ffmpeg(pmvb->mvp[4].bw.ref_frame_num==backward_ref_num);
                            }
                            #ifdef _config_ambadec_assert_
                            else
                            {
                                pmvb->mvp[4].bw.ref_frame_num=not_valid_mv;
                                ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[4].bw.ref_frame_num==not_valid_mv);
                                if(pmvb->mvp[0].bw.ref_frame_num!=not_valid_mv)
                                {
                                    av_log(NULL,AV_LOG_ERROR,"pmvb->mvp[0].bw.ref_frame_num=%d.\n",pmvb->mvp[0].bw.ref_frame_num);
                                }
                            }
                            #endif

                            if(pmvb->mvp[1].fw.ref_frame_num==forward_ref_num)
                            {
                                //bottom field fw
                                pcpv=pmvb->mvp[1].fw.mv_x;
                                pcpv1=pmvb->mvp[1].fw.mv_y;
                                pmvb->mvp[5].fw.mv_x=((pcpv>>1)|(pcpv & 1))*4;
                                //pmvb->mvp[5].fw.valid=1;
                                pmvb->mvp[5].fw.ref_frame_num=forward_ref_num;
                                pmvb->mvp[5].fw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;
                                pmvb->mvp[5].fw.bottom=pmvb->mvp[1].fw.bottom;

                                pmvb->mvp[1].fw.mv_x*=2;
                                pmvb->mvp[1].fw.mv_y*=2;

                                ambadec_assert_ffmpeg(pmvb->mvp[5].fw.ref_frame_num==forward_ref_num);
                            }
                            #ifdef _config_ambadec_assert_
                            else
                            {
                                pmvb->mvp[5].fw.ref_frame_num=not_valid_mv;
                                ambadec_assert_ffmpeg(pmvb->mvp[1].fw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[5].fw.ref_frame_num==not_valid_mv);
                                if(pmvb->mvp[1].fw.ref_frame_num!=not_valid_mv)
                                {
                                    av_log(NULL,AV_LOG_ERROR,"pmvb->mvp[1].fw.ref_frame_num=%d.\n",pmvb->mvp[1].fw.ref_frame_num);
                                }
                            }
                            #endif

                            if(pmvb->mvp[1].bw.ref_frame_num==backward_ref_num)
                            {
                                //ambadec_assert_ffmpeg(pmvb->mvp[1].bw.ref_frame_num==backward_ref_num);
                                //bottom field bw
                                //pmvb->mvp[1].bw.ref_frame_num=backward_ref_num;
                                pcpv=pmvb->mvp[1].bw.mv_x;
                                pcpv1=pmvb->mvp[1].bw.mv_y;
                                pmvb->mvp[5].bw.mv_x=((pcpv>>1)|(pcpv & 1))*4;
                                //pmvb->mvp[5].bw.valid=1;
                                pmvb->mvp[5].bw.mv_y=((pcpv1>>1)|(pcpv1 & 1))*4;
                                pmvb->mvp[5].bw.bottom=pmvb->mvp[1].bw.bottom;
                                pmvb->mvp[5].bw.ref_frame_num=backward_ref_num;

                                pmvb->mvp[1].bw.mv_x*=2;
                                pmvb->mvp[1].bw.mv_y*=2;

                                ambadec_assert_ffmpeg(pmvb->mvp[5].bw.ref_frame_num==backward_ref_num);
                            }
                            #ifdef _config_ambadec_assert_
                            else
                            {
                                pmvb->mvp[5].bw.ref_frame_num=not_valid_mv;
                                ambadec_assert_ffmpeg(pmvb->mvp[1].bw.ref_frame_num==not_valid_mv);
                                ambadec_assert_ffmpeg(pmvb->mvp[5].bw.ref_frame_num==not_valid_mv);
                                if(pmvb->mvp[1].bw.ref_frame_num!=not_valid_mv)
                                {
                                    av_log(NULL,AV_LOG_ERROR,"pmvb->mvp[1].bw.ref_frame_num=%d.\n",pmvb->mvp[1].bw.ref_frame_num);
                                }
                            }
                            #endif

                            #ifdef __config_fill_field_mv23__
                            pcp=&pmvb->mvp[0].fw;
                            pcp[4]=pcp[0];
                            pcp[5]=pcp[1];
                            pcp[6]=pcp[2];
                            pcp[7]=pcp[3];
                            #endif

                            #ifdef __config_fill_field_mv23_notvalid__
                            pmvb->mvp[2].fw.ref_frame_num=pmvb->mvp[2].bw.ref_frame_num=not_valid_mv;
                            pmvb->mvp[3].fw.ref_frame_num=pmvb->mvp[3].bw.ref_frame_num=not_valid_mv;
                            #endif

                        }
                        else
                            ambadec_assert_ffmpeg(0);
                    }
                    #ifdef _config_ambadec_assert_
                    else
                    {
                        ambadec_assert_ffmpeg(pmvb->mvp[0].fw.ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvb->mvp[1].fw.ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvb->mvp[2].fw.ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvb->mvp[3].fw.ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvb->mvp[4].fw.ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvb->mvp[5].fw.ref_frame_num==not_valid_mv);

                        ambadec_assert_ffmpeg(pmvb->mvp[0].bw.ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvb->mvp[1].bw.ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvb->mvp[2].bw.ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvb->mvp[3].bw.ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvb->mvp[4].bw.ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvb->mvp[5].bw.ref_frame_num==not_valid_mv);
                    }
                    #endif

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
                            pmvp->mb_setting.mv_type=mv_type_8x8;
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

                            pcp=(uint32_t *)&pmvp->mv[0];
                            pcpv=*pcp++;
                            *pcp++=pcpv;
                            *pcp++=pcpv;
                            *pcp=pcpv;

                            //temp scale mv by 2, 1/4 pixel
                            pmvp->mv[4].mv_x*=4;
                            pmvp->mv[4].mv_y*=4;

                            pcp=(uint32_t *)&pmvp->mv[4];
                            pcpv=*pcp++;
                            *pcp=pcpv;

                            ambadec_assert_ffmpeg(pmvp->mb_setting.mv_type==mv_type_8x8);
                            ambadec_assert_ffmpeg(pmvp->mv[0].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[1].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[2].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[3].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[4].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[5].ref_frame_num==forward_ref_num);
                        }
                        else if(pmvp->mb_setting.mv_type==mv_type_8x8)
                        {
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
                            pcp=(uint32_t *)&pmvp->mv[4];
                            pcpv=*pcp++;
                            *pcp=pcpv;

                            ambadec_assert_ffmpeg(pmvp->mv[4].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[5].ref_frame_num==forward_ref_num);
                        }
                        else if(pmvp->mb_setting.mv_type==mv_type_16x8)
                        {
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

                            ambadec_assert_ffmpeg(pmvp->mv[4].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[5].ref_frame_num==forward_ref_num);

                            #ifdef __config_fill_field_mv23__
                            pcp=&pmvp->mv[0];
                            pcp[2]=pcp[0];
                            pcp[3]=pcp[1];
                            #endif

                            #ifdef __config_fill_field_mv23_notvalid__
                            pmvp->mv[2].ref_frame_num=pmvp->mv[3].ref_frame_num=not_valid_mv;
                            #endif

                        }
                        else if(pmvp->mb_setting.mv_type==mv_type_16x16_gmc)
                        {
                            interpret_gmc(p_pic,pmvp,mo_x,mo_y);
                            pmvp->mb_setting.mv_type=mv_type_8x8;

                            ambadec_assert_ffmpeg(pmvp->mv[0].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[1].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[2].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[3].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[4].ref_frame_num==forward_ref_num);
                            ambadec_assert_ffmpeg(pmvp->mv[5].ref_frame_num==forward_ref_num);
                        }
                        else
                            ambadec_assert_ffmpeg(0);

                    }
                    #ifdef _config_ambadec_assert_
                    else
                    {
                        ambadec_assert_ffmpeg(pmvp->mv[0].ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvp->mv[1].ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvp->mv[2].ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvp->mv[3].ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvp->mv[4].ref_frame_num==not_valid_mv);
                        ambadec_assert_ffmpeg(pmvp->mv[5].ref_frame_num==not_valid_mv);
                    }
                    #endif

                }
            }
        }
    }
}

static inline void _Dump_open_files(void)
{
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
}

static inline void _Dump_close_files(void)
{
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
}

static inline void _Dump_coeff(h263_pic_data_t* p_pic)
{
    //temp
#if 0
    log_openfile_p(log_fd_tmp, "mpeg4_coef_tmp", p_pic->frame_cnt);
    log_dump_p(log_fd_tmp,p_pic->pdct, p_pic->mb_width*p_pic->mb_height*768,p_pic->frame_cnt);
    log_closefile_p(log_fd_tmp,p_pic->frame_cnt);
#endif


#ifdef __dump_DSP_test_data__
#ifdef __dump_binary__
    int mo_x,mo_y;
    short* pdct=p_pic->pdct;
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
}

static inline void _Dump_mv(h263_pic_data_t* p_pic)
{
#ifdef __dump_DSP_test_data__
#ifdef __dump_binary__
    if(0==p_pic->use_dsp)//for dump the same mv info with dsp in sw, change mv here
        _Change_1mv_to_4mv(p_pic);
#ifdef __dump_whole__
    if(p_pic->frame_cnt>=log_start_frame && p_pic->frame_cnt<=log_end_frame)
    {
        //static int cnt3=0;
        //av_log(NULL,AV_LOG_ERROR,"cnt3=%d.\n",cnt3++);
        log_dump_f(log_fd_mvdsp,p_pic->pmvp, p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t));
    }
    //av_log(NULL,AV_LOG_ERROR,"mvdsp p_pic->frame_cnt=%d,log_start_frame=%d,log_end_frame=%d.\n",p_pic->frame_cnt,log_start_frame,log_end_frame);
#else
    if(p_pic->use_dsp && CHUNKHEAD_VOPINFO_SIZE)//for dsp dump mvinfo with vopinfo at head
    {
        log_openfile_p(log_fd_mvdsp, "mpeg4_vopinfo_mv", p_pic->frame_cnt);
        log_dump_p(log_fd_mvdsp,p_pic->pvopinfo, CHUNKHEAD_VOPINFO_SIZE+p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t),p_pic->frame_cnt);
    }
    else
    {
        log_openfile_p(log_fd_mvdsp, "mpeg4_mv", p_pic->frame_cnt);
        log_dump_p(log_fd_mvdsp,p_pic->pmvp, p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t),p_pic->frame_cnt);
    }
    log_closefile_p(log_fd_mvdsp,p_pic->frame_cnt);
#endif
#endif
#endif
}

static inline void _Dump_coeff_mv(h263_pic_data_t* p_pic)
{
#ifdef __dump_DSP_test_data__
    log_dump_p(log_fd_residual,p_pic->pdct,p_pic->mb_height*p_pic->mb_width*768,p_pic->frame_cnt);
    log_dump_p(log_fd_mvdsp,p_pic->pmvp,p_pic->mb_height*p_pic->mb_width*sizeof(mb_mv_P_t),p_pic->frame_cnt);
#endif
}

static inline void _Dump_picinfo(h263_pic_data_t* p_pic)
{
#ifdef __dump_DSP_test_data__
    if (!p_pic->use_dsp) {
        //vop info
        //calculate dram address for vop_coef_daddr, mv_coef_daddr
        p_pic->vopinfo.uu.mp4s2.vop_coef_start_addr=VOP_COEF_DADDR+(p_pic->mb_width*p_pic->mb_height*64*2*6)*p_pic->frame_cnt;
        p_pic->vopinfo.uu.mp4s2.vop_mv_start_addr=VOP_MV_DADDR+(p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t))*p_pic->frame_cnt;
    /*            p_pic->vopinfo.uu.mpeg4s.vop_coef_daddr=VOP_COEF_DADDR+(p_pic->mb_width*p_pic->mb_height*64*2*6)*p_pic->frame_cnt;
        p_pic->vopinfo.uu.mpeg4s.mv_coef_daddr=VOP_MV_DADDR+(p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t))*p_pic->frame_cnt;
        //get before
        //p_pic->vopinfo.vop_time_increment_resolution=s->avctx->time_base.den;
        //p_pic->vopinfo.vop_pts_high=p_pic->vopinfo.vop_pts_low=0;
        p_pic->vopinfo.uu.mpeg4s.vop_chroma=1;//set for dsp
        //valid value
        p_pic->vopinfo.uu.mpeg4s.vop_width=p_pic->width;
        p_pic->vopinfo.uu.mpeg4s.vop_height=p_pic->height;
        #ifdef __dsp_interpret_gmc__
            if(p_pic->current_picture_ptr->pict_type==FF_S_TYPE)
                p_pic->vopinfo.uu.mpeg4s.vop_code_type=FF_P_TYPE-1;
            else
                p_pic->vopinfo.uu.mpeg4s.vop_code_type=(int)p_pic->current_picture_ptr->pict_type-1;
        #else
            p_pic->vopinfo.uu.mpeg4s.vop_code_type=(int)p_pic->current_picture_ptr->pict_type-1;
        #endif
        //p_pic->vopinfo.vop_code_type=(int)p_pic->current_picture_ptr->pict_type-1;
        p_pic->vopinfo.uu.mpeg4s.vop_round_type=p_pic->no_rounding;
        p_pic->vopinfo.uu.mpeg4s.vop_interlaced=p_pic->current_picture_ptr->interlaced_frame;
        p_pic->vopinfo.uu.mpeg4s.vop_top_field_first=p_pic->current_picture_ptr->top_field_first;*/
    }
#ifdef __dump_binary__
#ifdef __dump_whole__
    if(p_pic->frame_cnt>=log_start_frame && p_pic->frame_cnt<=log_end_frame)
    {
        //static int cnt2=0;
        //av_log(NULL,AV_LOG_ERROR,"cnt2=%d.\n",cnt2++);
        //log_dump_f(log_fd_picinfo,&p_pic->vopinfo, sizeof(udec_decode_t));
    }
    //av_log(NULL,AV_LOG_ERROR,"picinfo p_pic->frame_cnt=%d,log_start_frame=%d,log_end_frame=%d.\n",p_pic->frame_cnt,log_start_frame,log_end_frame);
#else
    //pic info
    //log_openfile_p(log_fd_picinfo, "mpeg4_picinfo", p_pic->frame_cnt);
    //log_dump_p(log_fd_picinfo,&p_pic->vopinfo, sizeof(udec_decode_t),p_pic->frame_cnt);
    //log_closefile_p(log_fd_picinfo,p_pic->frame_cnt);
#endif
#endif
#endif
}

static inline void _Dump_yuv(h263_pic_data_t* p_pic)
{
return;
#ifdef __log_dump_data__
    AVFrame* pdecpic=(AVFrame*)p_pic->current_picture_ptr;
    if(pdecpic->data[0])
    {
        //av_log(NULL,AV_LOG_WARNING,"s->flags&CODEC_FLAG_EMU_EDGE=%d,avctx->width=%d,avctx->height=%d, pict->linesize[0]=%d,pict->linesize[1]=%d.\n",s->flags&CODEC_FLAG_EMU_EDGE,avctx->width,avctx->height,pdecpic->linesize[0],pdecpic->linesize[1]);

        if(!dump_config_extedge)
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
    if(0==p_pic->use_dsp)//only sw can dump here
    {
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
    }
#endif

#endif
#endif
}

static inline void _Dump_raw_data_init(void)
{
#ifdef _dump_raw_data_
    av_log(NULL, AV_LOG_ERROR, "init 2 s->picture_number=%d\n",s->picture_number);
    dump_cnt=0;
#endif
}

static inline void _Dump_raw_data(MpegEncContext *s, const uint8_t *buf, int buf_size)
{
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
}

static inline void _Dump_dsp_txt(MpegEncContext *s)
{
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
}

static inline void _Dump_dsp_txt_close_vld(H263AmbaDecContext_t *thiz)
{
#ifdef __dump_DSP_test_data__
#ifdef __dump_DSP_TXT__
#ifdef __dump_separate__
    log_closefile_p(log_fd_dsp_text+log_offset_vld,thiz->p_pic->frame_cnt);
#endif
#endif
#endif
}

static inline void _Dump_dsp_txt_close_mcdct(h263_pic_data_t* p_pic)
{
#ifdef __dump_DSP_TXT__
    log_closefile_p(log_fd_dsp_text,p_pic->frame_cnt);
#ifdef __dump_separate__
    log_closefile_p(log_fd_dsp_text+log_offset_mc,p_pic->frame_cnt);
    log_closefile_p(log_fd_dsp_text+log_offset_dct,p_pic->frame_cnt);
#endif
#endif
}

//multi thread related
//two ways: 1: vld->mc_idct_addresidue
// 2: vld->mc->idct_addresidue

static void _mc_idct_err_handle(H263AmbaDecContext_t *thiz, h263_pic_data_t* p_pic, Picture * picture_ptr)
{
    if(picture_ptr)
    {
        if(thiz->pic_pool->dec_lock(thiz->pic_pool,picture_ptr)==1)
            thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)picture_ptr);
    }
    if(p_pic)
    {
        thiz->p_pic_dataq->release_toready(thiz->p_pic_dataq,p_pic,0);
        thiz->p_mc_idct_dataq->release(thiz->p_mc_idct_dataq,p_pic,0);
    }
}

void _sendNULLFrame(H263AmbaDecContext_t *thiz)
{
    ctx_nodef_t* p_node;
    p_node=thiz->p_frame_pool->get_free(thiz->p_frame_pool);
    p_node->p_ctx=NULL;
    thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
}

void* thread_h263_mc_idct_addresidue(void* p)
{
    H263AmbaDecContext_t *thiz=(H263AmbaDecContext_t *)p;
    MpegEncContext *s = &thiz->s;
    int ret;
    ctx_nodef_t* p_node;
    h263_pic_data_t* p_pic;
    int IsDspEOS=0;
    Picture* pic_left;
    AVFrame* pic;
    unsigned int flag;
    int ioctl_ret;
//    int i,j;

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"thread_h263_mc_idct_addresidue start.\n");
    #endif

    while(thiz->mc_idct_loop)
    {
        //get data
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Waiting: thread_h263_mc_idct_addresidue input: wait on p_mc_idct_dataq.\n");
        #endif

        p_node=thiz->p_mc_idct_dataq->get(thiz->p_mc_idct_dataq);
        p_pic=(h263_pic_data_t*)p_node->p_ctx;
        flag = p_node->flag;//for multi-thread sync, cmd flag should be take out first before other operation

        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Escape: thread_h263_mc_idct_addresidue input: escape from p_mc_idct_dataq,p_pic=%p.\n",p_pic);
        #endif

        /*if(thiz->iswaiting4flush && (flag != _flag_cmd_exit_next_) && (flag != _flag_cmd_flush_) && (flag != _flag_cmd_eos_))
        {
            _mc_idct_err_handle(thiz, p_pic, NULL);
            _sendNULLFrame(thiz);
            av_log(NULL,AV_LOG_DEBUG,"thread_h263_mc_idct_addresidue, waiting for flush cmd.\n");
            continue;
        }*/

        //flush
        /*av_log(NULL, AV_LOG_ERROR, "NOW idct_mc_need_flush=%d, iswaiting4flush=%d .  get_cnt %d  ==> 0x%x. \n",
        thiz->idct_mc_need_flush,thiz->iswaiting4flush,thiz->p_mc_idct_dataq->get_cnt, thiz->p_mc_idct_dataq);*/
        if (thiz->idct_mc_need_flush || thiz->iswaiting4flush) {
            av_log(NULL,AV_LOG_DEBUG," **Flush mc_idct_thread.\n");
            //av_log(NULL, AV_LOG_ERROR, "[mci] -> put_cnt before %d mmmmmmmm 0x%x  .\n",thiz->p_frame_pool->put_cnt, thiz->p_frame_pool);
            if (!p_pic) {
                if (flag == _flag_cmd_flush_) {
                    //flush packet come, send a flush packet to down stream
                    if (s->loop_filter)
                    {
                        p_node= thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                        p_node->p_ctx=NULL;
                        thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,_flag_cmd_flush_);
                    } else {
                        //send flush frame
                        p_node=thiz->p_frame_pool->get_cmd(thiz->p_frame_pool);
                        p_node->p_ctx=NULL;
                        thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,_flag_cmd_flush_);
                    }
                    //thiz->idct_mc_need_flush = 0;//for multi-thread sync, it should be set 0 after flush finished
                    //av_log(NULL, AV_LOG_ERROR, "[mci] -> put_cnt after %d wwwwwwww 0x%x  .\n",thiz->p_frame_pool->put_cnt, thiz->p_frame_pool);
                    av_log(NULL,AV_LOG_DEBUG," **Flush mc_idct_thread done.\n");
                    continue;
                }
            } else {
                flush_holded_pictures(thiz, p_pic);
                thiz->p_pic_dataq->release_toready(thiz->p_pic_dataq,p_pic,0);
                thiz->p_mc_idct_dataq->release(thiz->p_mc_idct_dataq, p_pic, 0);
                continue;
            }
        }

	if(flag == _flag_cmd_eos_)
	{
            av_log(NULL,AV_LOG_ERROR,"thread_h263_mc_idct_addresidue: _flag_cmd_eos_<--.\n");

            if(thiz->use_dsp) {
                p_pic->vopinfo.uu.mp4s2.vop_coef_start_addr = (unsigned char *)p_pic->pdct;
                p_pic->vopinfo.uu.mp4s2.vop_coef_end_addr = (unsigned char *)p_pic->pdct+thiz->pAcc->amba_idct_buffer_size;
                p_pic->vopinfo.uu.mp4s2.vop_mv_start_addr = p_pic->pvopinfo;
                p_pic->vopinfo.uu.mp4s2.vop_mv_end_addr = p_pic->pvopinfo+thiz->pAcc->amba_mv_buffer_size;
                av_log(NULL, AV_LOG_DEBUG, "coef size %d, vopinfo size %d, aligned32B mv size %d.\n", thiz->pAcc->amba_idct_buffer_size, CHUNKHEAD_VOPINFO_SIZE, thiz->pAcc->amba_mv_buffer_size-CHUNKHEAD_VOPINFO_SIZE);
                av_log(NULL, AV_LOG_DEBUG, "coef start %p, end %p.\n", p_pic->vopinfo.uu.mp4s2.vop_coef_start_addr, p_pic->vopinfo.uu.mp4s2.vop_coef_end_addr);
                av_log(NULL, AV_LOG_DEBUG, "vopinfo %p, aligned32B mv start %p, end %p.\n", p_pic->vopinfo.uu.mp4s2.vop_mv_start_addr, p_pic->pmvb, p_pic->vopinfo.uu.mp4s2.vop_mv_end_addr);
                av_log(NULL, AV_LOG_DEBUG, "usr sizeof (info) %d.\n", sizeof(p_pic->vopinfo));
                ambadec_assert_ffmpeg(p_pic->vopinfo.num_pics==1);
#ifdef _config_ambadec_assert_
                if(CHUNKHEAD_VOPINFO_SIZE)//check vop info in every frame mv chunk head
                {
                    ambadec_assert_ffmpeg(32==CHUNKHEAD_VOPINFO_SIZE);//hard code 32bytes now
                    vop_info_t* pchk=(vop_info_t*)p_pic->pvopinfo;
                    ambadec_assert_ffmpeg(pchk->end_of_sequence == 1);
                }
#endif
//                thiz->s.avctx->dec_eos_dummy_frame(thiz->pAcc, p_pic->pdct, p_pic->pvopinfo);
//av_log(NULL,AV_LOG_ERROR,"cccc eos acceleration <---.\n");
                    ioctl_ret = thiz->s.avctx->acceleration(thiz->pAcc, &p_pic->vopinfo);
//av_log(NULL,AV_LOG_ERROR,"cccc eos acceleration --->.\n");
                    if(-EPERM == ioctl_ret) {
                        av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, thread_h263_mc_idct acceleration fail.\n");
                        //release queue
                        av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, free p_pic_dataq and release p_mc_idct_dataq.\n");
                        _mc_idct_err_handle(thiz, p_pic, NULL);
                         thiz->iswaiting4flush=1;
                       _sendNULLFrame(thiz);
                        av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, p_frame_pool->ready_cnt=%d,p_mc_idct_dataq->ready_cnt=%d,p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_mc_idct_dataq->ready_cnt,thiz->p_vld_dataq->ready_cnt);
                        continue;
                    }
                /*
                pthread_mutex_lock(&thiz->mutex);
                thiz->decoding_frame_cnt--;
                pthread_mutex_unlock(&thiz->mutex);
                av_log(NULL,AV_LOG_ERROR,"thread_h263_mc_idct_addresidue: decoding_frame_cnt=%d.\n", thiz->decoding_frame_cnt);
                */
                av_log(NULL,AV_LOG_ERROR,"thread_h263_mc_idct_addresidue: wait left frames out of dsp start.\n");
                if(_Does_render_by_filter(thiz)) {
                    do{
                        av_log(NULL,AV_LOG_ERROR," Waiting: thread_h263_mc_idct_addresidue get pic_pool in eos processing.\n");
                        pic_left = (Picture*)thiz->pic_pool->get(thiz->pic_pool,&ret);
                        av_log(NULL,AV_LOG_ERROR," Escape: thread_h263_mc_idct_addresidue get pic_pool=%p in eos processing.\n",pic_left);
                        if(!pic_left)
                        {
                            av_log(NULL,AV_LOG_ERROR,"***********thread_h263_mc_idct_addresidue: thiz->pic_pool->get==NULL  in eos processing, no continue.\n");
                            thiz->pic_pool->rtn(thiz->pic_pool, pic_left);
                            pic_left = NULL;
                            break;
                        }
//av_log(NULL,AV_LOG_ERROR,"cccc eos get_decoded_frame <---.\n");
                        ioctl_ret = thiz->s.avctx->get_decoded_frame(thiz->pAcc, pic_left);
//                        av_log(NULL,AV_LOG_ERROR,"cccc eos get_decoded_frame --->.\n");
                        if(-EPERM == ioctl_ret) {
                            av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, thread_h263_mc_idct eos get_decoded_frame fail, no continue.\n");
                            thiz->pic_pool->rtn(thiz->pic_pool, pic_left);
                            pic_left = NULL;
                            //release queue
                            av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, free p_pic_dataq and release p_mc_idct_dataq.\n");
                            thiz->iswaiting4flush=1;
                            _sendNULLFrame(thiz);
                            av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, p_frame_pool->ready_cnt=%d,p_mc_idct_dataq->ready_cnt=%d,p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_mc_idct_dataq->ready_cnt,thiz->p_vld_dataq->ready_cnt);
                            break;
                        }
                        //check whether left frames out of dsp
                        pic = (AVFrame*)pic_left;
                        IsDspEOS = pic->is_eos;
                        av_log(NULL,AV_LOG_ERROR,"thread_h263_mc_idct_addresidue: left frame out of dsp, IsDspEOS=%d.\n", IsDspEOS);

                        p_node=thiz->p_frame_pool->get_free(thiz->p_frame_pool);
                        p_node->p_ctx=pic_left;
                        thiz->pic_pool->inc_lock(thiz->pic_pool, pic_left);
                        thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
                    }while(!IsDspEOS);
                }
                av_log(NULL,AV_LOG_ERROR,"thread_h263_mc_idct_addresidue: wait left frames out of dsp done.\n");
            }

            p_node=thiz->p_frame_pool->get_cmd(thiz->p_frame_pool);
            p_node->p_ctx=NULL;
            thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,_flag_cmd_eos_);

            //release queue
            _mc_idct_err_handle(thiz, p_pic, NULL);

            av_log(NULL,AV_LOG_ERROR,"thread_h263_mc_idct_addresidue: _flag_cmd_eos_-->.\n");
            continue;
	}

        if(!p_pic)
        {
            if(flag==_flag_cmd_exit_next_)
            {
                #ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"thread_h263_mc_idct_addresidue get NULL data(_flag_cmd_exit_next_), start exit.\n");
                #endif

                #ifdef __log_parallel_control__
                    av_log(NULL,AV_LOG_ERROR,"**** thread_h263_mc_idct_addresidue, get exit cmd, start exit.\n");
                #endif

                goto mc_idct_thread_exit;
            }
            else
            {
                #ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"thread_h263_mc_idct_addresidue get NULL data(_flag_cmd_exit_next_), start exit.\n");
                #endif
                /*if(s->loop_filter)
                {
                    av_log(NULL, AV_LOG_ERROR, "Error data indicater in thread_h263_mc_idct_addresidue\n");
                    p_node=thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                    thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,_flag_cmd_error_data_);
                }*/
                goto mc_idct_thread_exit;
            }
        }

//        av_log(NULL,AV_LOG_ERROR,"pic type=%d.\n",p_pic->current_picture_ptr->pict_type);

        _Dump_coeff(p_pic);
        _Dump_picinfo(p_pic);

        #if 1
        if(log_mask[decoding_config_pre_interpret_gmc] && p_pic->current_picture_ptr->pict_type==FF_S_TYPE)
        {//change mv settings, 1mv to 4mv, interpret global mc, calculate chroma mvs
            mb_mv_P_t* pmvp=p_pic->pmvp;
//            mb_mv_B_t* pmvb=p_pic->pmvb;
            int mo_x,mo_y;

            av_log(NULL,AV_LOG_DEBUG,"get here decoding_config_pre_interpret_gmc.\n");

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

        //feeding vop struct
        if (thiz->use_dsp) {
/*            p_pic->vopinfo.uu.mpeg4s.vop_width = p_pic->width;
            p_pic->vopinfo.uu.mpeg4s.vop_height = p_pic->height;
#ifdef __dsp_interpret_gmc__
            if(p_pic->current_picture_ptr->pict_type==FF_S_TYPE)
                p_pic->vopinfo.uu.mpeg4s.vop_code_type=FF_P_TYPE-1;
            else
                p_pic->vopinfo.uu.mpeg4s.vop_code_type=(int)p_pic->current_picture_ptr->pict_type-1;
#else
            p_pic->vopinfo.uu.mpeg4s.vop_code_type=(int)p_pic->current_picture_ptr->pict_type-1;
#endif
            //p_pic->vopinfo.vop_code_type=(int)p_pic->current_picture_ptr->pict_type-1;
            p_pic->vopinfo.uu.mpeg4s.vop_round_type=p_pic->no_rounding;
            p_pic->vopinfo.uu.mpeg4s.vop_interlaced=p_pic->current_picture_ptr->interlaced_frame;
            p_pic->vopinfo.uu.mpeg4s.vop_top_field_first=p_pic->current_picture_ptr->top_field_first;*/

            if(CHUNKHEAD_VOPINFO_SIZE)
            {
                //fill vopinfo at mv fifo chunk head
                _fill_vopinfo_at_mvfifo_chunk_head(p_pic);
            }
        }

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

            /*
            if (p_pic->current_picture_ptr) {
                av_log(NULL,AV_LOG_ERROR,"c %p, %p\n", p_pic->current_picture_ptr->data[0], p_pic->current_picture_ptr->data[1]);
            }
            if (p_pic->last_picture_ptr) {
                av_log(NULL,AV_LOG_ERROR,"l %p %p\n", p_pic->last_picture_ptr->data[0], p_pic->last_picture_ptr->data[1]);
            }
            if (p_pic->next_picture_ptr) {
                av_log(NULL,AV_LOG_ERROR,"n %p %p \n", p_pic->next_picture_ptr->data[0], p_pic->next_picture_ptr->data[1]);
            }
            */

            if(p_pic->current_picture_ptr->pict_type==FF_P_TYPE ||p_pic->current_picture_ptr->pict_type==FF_S_TYPE )
                h263_mpeg4_p_process_mc_idct_amba(thiz,p_pic);
            else if(p_pic->current_picture_ptr->pict_type==FF_B_TYPE)
                h263_mpeg4_b_process_mc_idct_amba(thiz,p_pic);
            else if(p_pic->current_picture_ptr->pict_type==FF_I_TYPE)
                h263_mpeg4_i_process_idct_amba(thiz,p_pic);
            else
            {
                av_log(NULL,AV_LOG_ERROR,"not support yet pict_type=%d, in thread_h263_mc_idct_addresidue .\n",p_pic->current_picture_ptr->pict_type);
            }
        }

        //mv
        //log_openfile_p(log_fd_mvdsp, "mpeg4_mv_ori", p_pic->frame_cnt);
        //log_dump_p(log_fd_mvdsp,p_pic->pmvp, p_pic->mb_width*p_pic->mb_height*sizeof(mb_mv_P_t),p_pic->frame_cnt);
        //log_closefile_p(log_fd_mvdsp,p_pic->frame_cnt);

        if (thiz->use_dsp) {
            ambadec_assert_ffmpeg(thiz->use_permutated);

            _Change_1mv_to_4mv(p_pic);

//#ifdef CONFIG_AMBA_MPEG4_IDCTMC_ACCELERATOR
//            //use amba dsp accelarator here
//            amba_mpeg4_idctmc_processing(thiz->amba_iav_fd, p_pic);
//#endif
            p_pic->vopinfo.uu.mp4s2.vop_coef_start_addr = (unsigned char *)p_pic->pdct;
            p_pic->vopinfo.uu.mp4s2.vop_coef_end_addr = (unsigned char *)p_pic->pdct+thiz->pAcc->amba_idct_buffer_size;
//            av_log(NULL, AV_LOG_ERROR, "xxxxxxxxxxxxxxx pdct %p, size=%d, pdct+size=%p\n",
//                p_pic->pdct, thiz->pAcc->amba_idct_buffer_size, p_pic->pdct+thiz->pAcc->amba_idct_buffer_size);
            p_pic->vopinfo.uu.mp4s2.vop_mv_start_addr = p_pic->pvopinfo;
            p_pic->vopinfo.uu.mp4s2.vop_mv_end_addr = p_pic->pvopinfo+thiz->pAcc->amba_mv_buffer_size;
            av_log(NULL, AV_LOG_DEBUG, "decode id %d, decode type %d.\n", p_pic->vopinfo.decoder_id, p_pic->vopinfo.udec_type);
            av_log(NULL, AV_LOG_DEBUG, "coef size %d, vopinfo size %d, aligned32B mv size %d.\n", thiz->pAcc->amba_idct_buffer_size, CHUNKHEAD_VOPINFO_SIZE, thiz->pAcc->amba_mv_buffer_size-CHUNKHEAD_VOPINFO_SIZE);
            av_log(NULL, AV_LOG_DEBUG, "coef start %p, end %p.\n", p_pic->vopinfo.uu.mp4s2.vop_coef_start_addr, p_pic->vopinfo.uu.mp4s2.vop_coef_end_addr);
            av_log(NULL, AV_LOG_DEBUG, "vopinfo %p, aligned32B mv start %p, end %p.\n", p_pic->vopinfo.uu.mp4s2.vop_mv_start_addr, p_pic->pmvb, p_pic->vopinfo.uu.mp4s2.vop_mv_end_addr);
            av_log(NULL, AV_LOG_DEBUG, "usr sizeof (info) %d.\n", sizeof(p_pic->vopinfo));
            ambadec_assert_ffmpeg(p_pic->vopinfo.num_pics==1);
#ifdef _config_ambadec_assert_
            if(CHUNKHEAD_VOPINFO_SIZE)//check vop info in every frame mv chunk head
            {
                ambadec_assert_ffmpeg(32==CHUNKHEAD_VOPINFO_SIZE);//hard code 32bytes now
                vop_info_t* pchk=(vop_info_t*)p_pic->pvopinfo;
                uint32_t vop_code_type=_get_vop_code_type(p_pic);
                ambadec_assert_ffmpeg(pchk->vop_width == p_pic->width);
                ambadec_assert_ffmpeg(pchk->vop_height == p_pic->height);
                ambadec_assert_ffmpeg(pchk->vop_code_type==vop_code_type);
                ambadec_assert_ffmpeg(pchk->vop_round_type==p_pic->no_rounding);
                ambadec_assert_ffmpeg(pchk->vop_interlaced==p_pic->current_picture_ptr->interlaced_frame);
                ambadec_assert_ffmpeg(pchk->vop_top_field_first==p_pic->current_picture_ptr->top_field_first);
                ambadec_assert_ffmpeg(pchk->end_of_sequence == 0);
            }
#endif
//av_log(NULL,AV_LOG_ERROR,"cccc acceleration <---.\n");
            ioctl_ret = thiz->s.avctx->acceleration(thiz->pAcc, &p_pic->vopinfo);
//            av_log(NULL,AV_LOG_ERROR,"cccc acceleration --->.\n");
            if(-EPERM == ioctl_ret) {
                av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, thread_h263_mc_idct acceleration fail.\n");
                //release queue
                av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, free p_pic_dataq and release p_mc_idct_dataq.\n");
                _mc_idct_err_handle(thiz, p_pic, p_pic->current_picture_ptr);
                thiz->iswaiting4flush=1;
                _sendNULLFrame(thiz);
                av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, p_frame_pool->ready_cnt=%d,p_mc_idct_dataq->ready_cnt=%d,p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_mc_idct_dataq->ready_cnt,thiz->p_vld_dataq->ready_cnt);
                continue;
            }
        }

        _Dump_mv(p_pic);

        //trigger next

        if(!s->loop_filter)
        {
            //if use ione dsp, the video buffer need not be accessed by sw
            if (!thiz->use_dsp) {
                if(s->unrestricted_mv && p_pic->current_picture_ptr->pict_type!=FF_B_TYPE
                   && !s->intra_only
                   && !(s->flags&CODEC_FLAG_EMU_EDGE)) {
                    //av_log(NULL,AV_LOG_ERROR,"here!!!!here!! reference,s->intra_only=%d,s->flags=%x,s->unrestricted_mv=%d,s->current_picture_ptr->reference=%d\n.\n",s->intra_only,s->flags,s->unrestricted_mv,p_pic->current_picture_ptr->reference);
                        int edges = EDGE_BOTTOM | EDGE_TOP;

                        s->dsp.draw_edges(p_pic->current_picture_ptr->data[0], p_pic->current_picture_ptr->linesize[0]  , p_pic->h_edge_pos   , p_pic->v_edge_pos   , EDGE_WIDTH  , edges);
                        s->dsp.draw_edges_nv12(p_pic->current_picture_ptr->data[1], p_pic->current_picture_ptr->linesize[1] , p_pic->h_edge_pos>>1, p_pic->v_edge_pos>>1);
            //            s->dsp.draw_edges(s->current_picture.data[2], s->uvlinesize, s->h_edge_pos>>1, s->v_edge_pos>>1, EDGE_WIDTH/2  , edges);
                }
                emms_c();
            }

            _Dump_yuv(p_pic);

            s->last_pict_type    = p_pic->current_picture_ptr->pict_type;
            if(p_pic->current_picture_ptr->pict_type!=FF_B_TYPE){
                s->last_non_b_pict_type= p_pic->current_picture_ptr->pict_type;
            }

            s->avctx->coded_frame= (AVFrame*)p_pic->current_picture_ptr;

            //send output frame
/*            pthread_mutex_lock(&thiz->mutex);
            thiz->decoding_frame_cnt--;
            pthread_mutex_unlock(&thiz->mutex);
          */
            if(p_pic->current_picture_ptr->pict_type==FF_B_TYPE)
            {
                if (thiz->use_dsp && _Does_render_by_filter(thiz)) {
//                    av_log(NULL,AV_LOG_ERROR,"cccc get_decoded_frame <---.\n");
                    ioctl_ret = thiz->s.avctx->get_decoded_frame(thiz->pAcc, p_pic->current_picture_ptr);
//                    av_log(NULL,AV_LOG_ERROR,"cccc get_decoded_frame --->.\n");
                    if(-EPERM == ioctl_ret) {
                        av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, thread_h263_mc_idct get_decoded_frame fail.\n");
                        //release queue
                        av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, free p_pic_dataq and release p_mc_idct_dataq.\n");
                        _mc_idct_err_handle(thiz, p_pic, p_pic->current_picture_ptr);
                        thiz->iswaiting4flush=1;
                        _sendNULLFrame(thiz);
                        av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, p_frame_pool->ready_cnt=%d,p_mc_idct_dataq->ready_cnt=%d,p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_mc_idct_dataq->ready_cnt,thiz->p_vld_dataq->ready_cnt);
                        continue;
                    }
                }
                p_node=thiz->p_frame_pool->get_free(thiz->p_frame_pool);
                p_node->p_ctx=p_pic->current_picture_ptr;
                thiz->pic_pool->inc_lock(thiz->pic_pool,p_pic->current_picture_ptr);
                //av_log(NULL,AV_LOG_ERROR,"   B pts=%lld.\n",p_pic->current_picture_ptr->pts);
                #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"   mc_idct thread output: send picture=%p.\n",p_pic->current_picture_ptr);
                #endif
                thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
            }
            else
            {
                //if (p_pic->last_picture_ptr)
                {
                    if (thiz->use_dsp && p_pic->last_picture_ptr && _Does_render_by_filter(thiz)) {
//                        av_log(NULL,AV_LOG_ERROR,"cccc get_decoded_frame <---.\n");
                        ioctl_ret = thiz->s.avctx->get_decoded_frame(thiz->pAcc, p_pic->last_picture_ptr);
//                        av_log(NULL,AV_LOG_ERROR,"cccc get_decoded_frame --->.\n");
                        if(-EPERM == ioctl_ret) {
                            av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, thread_h263_mc_idct get_decoded_frame fail.\n");
                            //release queue
                            av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, free p_pic_dataq and release p_mc_idct_dataq.\n");
                            _mc_idct_err_handle(thiz, p_pic, p_pic->last_picture_ptr);
                            thiz->iswaiting4flush=1;
                            _sendNULLFrame(thiz);
                            av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, p_frame_pool->ready_cnt=%d,p_mc_idct_dataq->ready_cnt=%d,p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_mc_idct_dataq->ready_cnt,thiz->p_vld_dataq->ready_cnt);
                            continue;
                        }
                    }
                    p_node=thiz->p_frame_pool->get_free(thiz->p_frame_pool);
                    p_node->p_ctx=p_pic->last_picture_ptr;
                    thiz->pic_pool->inc_lock(thiz->pic_pool,p_pic->last_picture_ptr);
                    //if (p_pic->last_picture_ptr)
                    //av_log(NULL,AV_LOG_ERROR,"   not B pts=%lld.\n",p_pic->last_picture_ptr->pts);
                    #ifdef __log_parallel_control__
                    av_log(NULL,AV_LOG_ERROR,"   mc_idct thread output: send picture=%p.\n",p_pic->last_picture_ptr);
                    #endif
                    thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
                }
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

            _Dump_dsp_txt_close_mcdct(p_pic);

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
        av_log(NULL,AV_LOG_ERROR,"thread_h263_mc_idct_addresidue exit.\n");
    #endif

    #ifdef __log_parallel_control__
        av_log(NULL,AV_LOG_ERROR,"thread_h263_mc_idct_addresidue exit.\n");
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
//    int ret;
    ctx_nodef_t* p_node;
    h263_pic_data_t* p_pic;
    unsigned int flag;

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

        p_pic=p_node->p_ctx;
        flag = p_node->flag;

        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Escape: deblock_thread escape input: from p_deblock_dataq,p_pic=%p.\n",p_pic);
        #endif

        //flush
        if (thiz->deblock_need_flush) {
            if (!p_pic) {
                if (flag == _flag_cmd_flush_) {
                    //send flush frame
                    p_node=thiz->p_frame_pool->get_cmd(thiz->p_frame_pool);
                    p_node->p_ctx=NULL;
                    thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,_flag_cmd_flush_);
                    thiz->deblock_need_flush = 0;
                    av_log(NULL,AV_LOG_ERROR," **Flush deblock_thread done.\n");
                    continue;
                }
            } else {
                flush_holded_pictures(thiz, p_pic);
                thiz->p_pic_dataq->release_toready(thiz->p_pic_dataq,p_pic,0);
                thiz->p_deblock_dataq->release(thiz->p_deblock_dataq, p_pic, 0);
                continue;
            }
        }
        if(!p_pic)
        {
            if(flag==_flag_cmd_exit_next_)
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

        _Dump_coeff_mv(p_pic);

/*
        pthread_mutex_lock(&thiz->mutex);
        thiz->decoding_frame_cnt--;
        pthread_mutex_unlock(&thiz->mutex);
*/
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

static void _vld_err_handle(H263AmbaDecContext_t *thiz)
{
    MpegEncContext *s = &thiz->s;
    if(s->current_picture_ptr)
    {
        thiz->pic_pool->rtn(thiz->pic_pool, s->current_picture_ptr);
        if(thiz->pic_pool->dec_lock(thiz->pic_pool,s->current_picture_ptr) <= 1)
             thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)s->current_picture_ptr);
        s->current_picture_ptr = NULL;
    }
    if(thiz->p_pic)
    {
        thiz->p_pic_dataq->release_toready(thiz->p_pic_dataq,thiz->p_pic,0);
        thiz->p_pic=NULL;
    }
}

void* thread_h263_vld(void* p)
{
    H263AmbaDecContext_t *thiz=(H263AmbaDecContext_t *)p;
    MpegEncContext *s = &thiz->s;
    const uint8_t *buf ;
    int buf_size;
    int ret;
//    AVFrame *pict;
    ctx_nodef_t* p_node;
    unsigned int flag;
    h263_vld_data_t* p_vld;
    int i=0;
    void* pv;
    int ioctl_ret;

    while(thiz->vld_loop)
    {
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Waiting: vld_thread input: wait on p_vld_dataq.\n");
        #endif

        p_node = thiz->p_vld_dataq->get(thiz->p_vld_dataq);
        p_vld = p_node->p_ctx;
        flag = p_node->flag;//for multi-thread sync, cmd flag should be take out first before other operation

        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Escape: vld_thread input: escape from p_vld_dataq,p_vld=%p.\n",p_vld);
        #endif

        /*if(thiz->iswaiting4flush && (flag != _flag_cmd_exit_next_) && (flag != _flag_cmd_flush_) && (flag != _flag_cmd_eos_))
        {
            _vld_err_handle(thiz);
            if(p_vld)
            {
                if(p_vld->pbuf)
                {
                    av_free(p_vld->pbuf);
                    p_vld->pbuf=NULL;
                }
                thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
            }
            _sendNULLFrame(thiz);
            av_log(NULL,AV_LOG_DEBUG,"thread_h263_vld, waiting for flush cmd.\n");
            continue;
        }*/

        //flush
        /*av_log(NULL, AV_LOG_ERROR, "NOW vld_need_flush=%d, iswaiting4flush=%d, pipe_line_started=%d.  get_cnt %d  ==> 0x%x.\n",
        thiz->vld_need_flush,thiz->iswaiting4flush, thiz->pipe_line_started, thiz->p_vld_dataq->get_cnt,thiz->p_vld_dataq);*/
        if ((thiz->vld_need_flush || thiz->iswaiting4flush) && thiz->pipe_line_started) {
            av_log(NULL,AV_LOG_DEBUG," **Flush vld_thread.\n");

            if (!p_vld) {
                if (flag == _flag_cmd_flush_) {
                    //flush packet come, send a flush packet to down stream
                    if (thiz->parallel_method == 1)
                    {
                        p_node = thiz->p_mc_idct_dataq->get_cmd(thiz->p_mc_idct_dataq);
                        p_node->p_ctx=NULL;
                        thiz->p_mc_idct_dataq->put_ready(thiz->p_mc_idct_dataq,p_node,_flag_cmd_flush_);
                    } else if (thiz->parallel_method == 2) {
                        p_node = thiz->p_idct_dataq->get_cmd(thiz->p_idct_dataq);
                        p_node->p_ctx=NULL;
                        thiz->p_idct_dataq->put_ready(thiz->p_idct_dataq,p_node,_flag_cmd_flush_);
                    }
                    //thiz->vld_need_flush = 0;//for multi-thread sync, it should be set 0 after flush finished
                    thiz->p_pic = NULL;
                    av_log(NULL,AV_LOG_DEBUG," **Flush vld_thread done.\n");
                    continue;
                }
            } else {
                //free raw data and release p_vld
                if(p_vld->pbuf)
                {
                    //av_log(NULL,AV_LOG_ERROR,"free p_vld->pbuf=%p.\n",p_vld->pbuf);
                    av_free(p_vld->pbuf);
                    p_vld->pbuf=NULL;
                }
                thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
                continue;
            }
        }

	if (flag == _flag_cmd_eos_)
	{
		     av_log(NULL,AV_LOG_ERROR,"thread_h263_vld: _flag_cmd_eos_<--.\n");
                   ambadec_assert_ffmpeg(NULL==p_vld);

                   //for fake dec, need fake request
                    if(!thiz->p_pic)
                    {
                        p_node=thiz->p_pic_dataq->get(thiz->p_pic_dataq);
                        thiz->p_pic=p_node->p_ctx;
                        //exit indicator?
                        if(!thiz->p_pic)
                        {
                            av_log(NULL,AV_LOG_ERROR,"thread_h263_vld: _flag_cmd_eos, r->p_pic_dataq->get==NULL,exit.\n");
                            break;
                        }

                        if (thiz->use_dsp) {
//                            av_log(NULL,AV_LOG_ERROR,"cccc eos request_inputbuffer <---.\n");
                            ioctl_ret = thiz->s.avctx->request_inputbuffer(thiz->pAcc, 1, (unsigned char *)thiz->p_pic->pdct);
//                            av_log(NULL,AV_LOG_ERROR,"cccc eos request_inputbuffer --->.\n");
                            if(-EPERM == ioctl_ret) {
                                av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, vld_thread request_inputbuffer fail.\n");
                                _vld_err_handle(thiz);
                                thiz->iswaiting4flush=1;
                                _sendNULLFrame(thiz);
                                av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, p_frame_pool->ready_cnt=%d,p_mc_idct_dataq->ready_cnt=%d,p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_mc_idct_dataq->ready_cnt,thiz->p_vld_dataq->ready_cnt);
                                continue;
                            }
                        }

                        _h263_reset_picture_data(thiz);
#ifdef _config_ambadec_assert_
                        if(thiz->use_dsp)
                        {
                            for(i=0; i<thiz->pAcc->amba_buffer_number; i++) {
                                if(thiz->p_pic==(&thiz->picdata[i]))
                                    break;
                            }
                            ambadec_assert_ffmpeg(i<thiz->pAcc->amba_buffer_number);
                        }
                        else
                            ambadec_assert_ffmpeg(thiz->p_pic==(&thiz->picdata[0]) || thiz->p_pic==(&thiz->picdata[1]));
#endif
                    }else {
                        av_log(NULL,AV_LOG_ERROR,"thread_h263_vld: _flag_cmd_eos, maybe trash, should not come here.\n");
                    }

                    if (thiz->use_dsp)
                    {
                        vop_info_t* pvop_info=(vop_info_t*)thiz->p_pic->pvopinfo;
                        pvop_info->end_of_sequence= 1;
                    }

                    if(thiz->parallel_method==1)
                    {
                        p_node=thiz->p_mc_idct_dataq->get_cmd(thiz->p_mc_idct_dataq);
                        p_node->p_ctx=thiz->p_pic;
                        thiz->p_pic=NULL;
                        thiz->p_mc_idct_dataq->put_ready(thiz->p_mc_idct_dataq,p_node,_flag_cmd_eos_);
                    }
                    else if(thiz->parallel_method==2)
                    {
                        p_node=thiz->p_mc_dataq->get_cmd(thiz->p_mc_dataq);
                        p_node->p_ctx=thiz->p_pic;
                        thiz->p_pic=NULL;
                        thiz->p_mc_dataq->put_ready(thiz->p_mc_dataq,p_node,_flag_cmd_eos_);
                    }

		     av_log(NULL,AV_LOG_ERROR,"thread_h263_vld: _flag_cmd_eos_-->.\n");
/*
			pthread_mutex_lock(&thiz->mutex);
			thiz->decoding_frame_cnt++;
			pthread_mutex_unlock(&thiz->mutex);
			av_log(NULL,AV_LOG_ERROR,"thread_h263_vld: decoding_frame_cnt=%d.\n", thiz->decoding_frame_cnt);*/
			log_cur_frame++;

                    continue;
        }

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"**** vld_thread get p_node, start a loop, log_cur_frame=%d .\n",log_cur_frame);
        #endif

        ambadec_assert_ffmpeg(thiz->parallel_method==1);

        if(!p_vld )
        {
            if(flag==_flag_cmd_exit_next_)
            {
                #ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"vld_thread get exit cmd(_flag_cmd_exit_next_), start exit.\n");
                #endif

                thiz->vld_loop=0;
                break;
            }
            /*else if(flag==_flag_cmd_flush_)
            {
                //flush packet come, send a flush packet to down stream
                if (thiz->parallel_method==1)
                {
                    p_node=thiz->p_mc_idct_dataq->get_cmd(thiz->p_mc_idct_dataq);
                    thiz->p_mc_idct_dataq->put_ready(thiz->p_mc_idct_dataq,p_node,_flag_cmd_flush_);
                }
            }*/
            else  //unknown cmd, exit too
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
                _vld_err_handle(thiz);
                goto freedata;
            }

            if( ff_combine_frame(&s->parse_context, next, (const uint8_t **)&buf, &buf_size) < 0 ){
                _vld_err_handle(thiz);
                goto freedata;
            }
        }


    retry:

        _Dump_raw_data(s, buf, buf_size);

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
        //ambadec_assert_ffmpeg(thiz->pic_finished);
        if(thiz->pic_finished || !s->current_picture_ptr)
        {
            if(s->current_picture_ptr){
                if(thiz->pic_pool->dec_lock(thiz->pic_pool,s->current_picture_ptr)==1)
                      s->avctx->release_buffer(s->avctx,(AVFrame*)s->current_picture_ptr);
                s->current_picture_ptr = NULL;
            }
            s->current_picture_ptr=(Picture*)thiz->pic_pool->get(thiz->pic_pool,&ret);
            thiz->pic_pool->inc_lock(thiz->pic_pool,s->current_picture_ptr);
//            s->current_picture_ptr=thiz->p_pic->current_picture_ptr=(Picture*)thiz->pic_pool->get(thiz->pic_pool,&ret);
//av_log(NULL,AV_LOG_ERROR,"cccc get_buffer <---.\n");
            ioctl_ret = s->avctx->get_buffer(s->avctx,(AVFrame*)s->current_picture_ptr);
//av_log(NULL,AV_LOG_ERROR,"cccc get_buffer --->.\n");
            if (-EPERM == ioctl_ret) {
                _vld_err_handle(thiz);
                /*if(-1 == ioctl_ret) {
                    av_log(NULL,AV_LOG_ERROR,"vld_thread get buffer fail, start exit.\n");
                    thiz->vld_loop=0;
                    break;
                }else*/
                {
                    av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, vld_thread get_buffer fail.\n");
                    if(p_vld)
                    {
                        av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, free raw data and release p_vld.\n");
                        if(p_vld->pbuf)
                        {
                            av_free(p_vld->pbuf);
                            p_vld->pbuf=NULL;
                        }
                        thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
                    }
                    thiz->iswaiting4flush=1;
                    _sendNULLFrame(thiz);
                    av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, p_frame_pool->ready_cnt=%d,p_mc_idct_dataq->ready_cnt=%d,p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_mc_idct_dataq->ready_cnt,thiz->p_vld_dataq->ready_cnt);
                    continue;
                }
            }
            s->current_picture_ptr->pts = p_vld->pts;
            //av_log(NULL,AV_LOG_ERROR,"  vld_thread pts %lld.\n",p_vld->pts);
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

        if(ret==FRAME_SKIPPED) {av_log(NULL, AV_LOG_ERROR, "**!!before freedata1. Frame skipped\n"); /*_vld_err_handle(thiz);*/goto freedata;}

        /* skip if the header was thrashed */
        if (ret < 0){
            av_log(s->avctx, AV_LOG_ERROR, "header damaged, ret=%d\n",ret);
            //_vld_err_handle(thiz); //fix bug 1409, chuchen 2011-10-10
            goto freedata;
        }

        #ifdef __tmp_use_amba_permutation__
        //permutated matrix and whether or not use dsp will determined here
        if((thiz->use_dsp || log_mask[decoding_config_use_dsp_permutated]) && thiz->use_permutated!=1)
        {
            av_log(NULL,AV_LOG_ERROR,"config: using dsp permutated matrix now, thiz->use_dsp %d, logmask %d, per %d.\n", thiz->use_dsp,  log_mask[decoding_config_use_dsp_permutated], thiz->use_permutated);
            thiz->use_permutated=1;
            switch_permutated(s,1);
        }
        else if(thiz->use_permutated!=0 && ((!thiz->use_dsp) && log_mask[decoding_config_use_dsp_permutated]==0))
        {
            av_log(NULL,AV_LOG_ERROR,"config: not using dsp permutated matrix now, thiz->use_dsp %d, logmask %d, per %d.\n", thiz->use_dsp,  log_mask[decoding_config_use_dsp_permutated], thiz->use_permutated);
            thiz->use_permutated=0;
            switch_permutated(s,0);
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
        int mm_flags = av_get_cpu_flags();
        if(s->codec_id == CODEC_ID_AMBA_P_MPEG4 && s->xvid_build && s->avctx->idct_algo == FF_IDCT_AUTO && (mm_flags & AV_CPU_FLAG_MMX)){
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
                p_node->p_ctx=NULL;
                thiz->p_mc_idct_dataq->put_ready(thiz->p_mc_idct_dataq,p_node,_flag_cmd_exit_next_);
                if(s->loop_filter)
                {
                    p_node=thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                    p_node->p_ctx=NULL;
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
                p_node->p_ctx=NULL;
                thiz->p_idct_dataq->put_ready(thiz->p_idct_dataq,p_node,_flag_cmd_exit_next_);
                p_node=thiz->p_mc_dataq->get_cmd(thiz->p_mc_dataq);
                p_node->p_ctx=NULL;
                thiz->p_mc_dataq->put_ready(thiz->p_mc_dataq,p_node,_flag_cmd_exit_next_);
                if(s->loop_filter)
                {
                    p_node=thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                    p_node->p_ctx=NULL;
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

//#ifdef CONFIG_AMBA_MPEG4_IDCTMC_ACCELERATOR
//            if (thiz->use_dsp) {
//                end_amba_mpeg4_idctmc_accelerator(thiz);
//            }
//#endif
        }

        if(!thiz->pipe_line_started)
        {
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"*****  vld_thread start new pipe line.\n");
            #endif
            ambadec_assert_ffmpeg(!thiz->p_pic_dataq);

//#ifdef CONFIG_AMBA_MPEG4_IDCTMC_ACCELERATOR
//            init_amba_mpeg4_idctmc_accelerator(thiz, s->width, s->height);
//#endif
            thiz->p_pic_dataq=ambadec_create_triqueue(_h263_callback_destroy_pic_data);

            //resource alloc
            if (thiz->use_dsp) {
                ambadec_assert_ffmpeg(thiz->pAcc);
                ambadec_assert_ffmpeg(thiz->pAcc->p_amba_idct_ringbuffer);
                ambadec_assert_ffmpeg(thiz->pAcc->p_amba_mv_ringbuffer);
                ambadec_assert_ffmpeg(thiz->pAcc->amba_idct_buffer_size);
                ambadec_assert_ffmpeg(thiz->pAcc->amba_mv_buffer_size);
                for(i=0; i<thiz->pAcc->amba_buffer_number; i++)
                {
                    thiz->picdata[i].mb_width=s->mb_width;
                    thiz->picdata[i].mb_height=s->mb_height;
                    thiz->picdata[i].mb_stride=s->mb_stride;
                    thiz->picdata[i].pdct = (short*) (thiz->pAcc->p_amba_idct_ringbuffer + thiz->pAcc->amba_idct_buffer_size * i);
                    thiz->picdata[i].pvopinfo = thiz->pAcc->p_amba_mv_ringbuffer + thiz->pAcc->amba_mv_buffer_size * i ;
                    thiz->picdata[i].pmvb = (mb_mv_B_t*) (thiz->picdata[i].pvopinfo + CHUNKHEAD_VOPINFO_SIZE);
                    thiz->picdata[i].pmvp = (mb_mv_P_t*) thiz->picdata[i].pmvb;
                    thiz->picdata[i].vopinfo.decoder_id = thiz->pAcc->decode_id;
                    thiz->picdata[i].vopinfo.udec_type = thiz->pAcc->decode_type;//mpeg4-sw
                    thiz->picdata[i].vopinfo.num_pics = 1;
                    av_log(NULL, AV_LOG_ERROR, " init decode_id %d, decode_type %d.\n", thiz->picdata[i].vopinfo.decoder_id, thiz->picdata[i].vopinfo.udec_type);
                    thiz->picdata[i].pswinfo=av_malloc(sizeof(h263_pic_swinfo_t)*s->mb_width*s->mb_height);

                    p_node=thiz->p_pic_dataq->get_free(thiz->p_pic_dataq);
                    p_node->p_ctx=&thiz->picdata[i];
                    thiz->p_pic_dataq->put_ready(thiz->p_pic_dataq,p_node,0);
                }
            } else {
                for(i=0;i<2;i++)
                {
                    thiz->picdata[i].mb_width=s->mb_width;
                    thiz->picdata[i].mb_height=s->mb_height;
                    thiz->picdata[i].mb_stride=s->mb_stride;
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
            }

//            thiz->vld_sent_row=thiz->mc_sent_row=0;
            thiz->pic_finished=1;
            thiz->p_pic=NULL;

            if(thiz->parallel_method==1)
            {
                ambadec_assert_ffmpeg(!thiz->p_mc_idct_dataq);
                thiz->p_mc_idct_dataq=ambadec_create_triqueue(_h263_callback_NULL);
                thiz->mc_idct_loop=1;
                pthread_create(&thiz->tid_mc_idct,NULL,thread_h263_mc_idct_addresidue,thiz);
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

            /*av_log(NULL,AV_LOG_ERROR," triqueue list:  p_vld_dataq=0x%x, p_mc_idct_dataq=0x%x, p_pic_dataq=0x%x, p_frame_pool=0x%x, p_mc_dataq=0x%x, p_idct_dataq=0x%x, p_deblock_dataq=0x%x.\n",
            thiz->p_vld_dataq, thiz->p_mc_idct_dataq, thiz->p_pic_dataq, thiz->p_frame_pool,
            thiz->p_mc_dataq, thiz->p_idct_dataq,thiz->p_deblock_dataq );*/
        }

        // for hurry_up==5
        s->current_picture_ptr->pict_type= s->pict_type;
        //av_log(NULL, AV_LOG_ERROR, "***!!!s->pict_type %d, s->last_picture_ptr %p, current %p, next %p.\n", s->pict_type, s->last_picture_ptr, s->next_picture_ptr, s->current_picture_ptr);

        s->current_picture_ptr->key_frame= s->pict_type == FF_I_TYPE;

        /* no reference, wait for next key frame */
        if (!s->current_picture_ptr->key_frame && s->next_picture_ptr==NULL){ av_log(NULL, AV_LOG_ERROR, "**!!before freedata2.\n");/*_vld_err_handle(thiz);fix bug 1409*/goto freedata;}
        /* skip B-frames if we don't have reference frames */
        if(s->last_picture_ptr==NULL && (s->pict_type==FF_B_TYPE || s->dropable)){av_log(NULL, AV_LOG_ERROR, "**!!before freedata3.\n"); _vld_err_handle(thiz);goto freedata;}
        /* skip b frames if we are in a hurry */
        if(
#if FF_API_HURRY_UP
        s->avctx->hurry_up &&
#endif
        (thiz->s.avctx->flags&CODEC_FLAG_LOW_DELAY) && s->pict_type==FF_B_TYPE){av_log(NULL, AV_LOG_ERROR, "**!!before freedata4. Low delay mode Skip B picture.\n"); /*_vld_err_handle(thiz);*/goto freedata;}

        if(   (s->avctx->skip_frame >= AVDISCARD_NONREF && s->pict_type==FF_B_TYPE)
           || (s->avctx->skip_frame >= AVDISCARD_NONKEY && s->pict_type!=FF_I_TYPE)
           ||  s->avctx->skip_frame >= AVDISCARD_ALL)
           {av_log(NULL, AV_LOG_ERROR, "**!!before freedata5.\n"); _vld_err_handle(thiz);goto freedata;}
        /* skip everything if we are in a hurry>=5 */
#if FF_API_HURRY_UP
        if(p_vld->hurry_up>=5){av_log(NULL, AV_LOG_ERROR, "**!!before freedata6.\n"); _vld_err_handle(thiz);goto freedata;}
#endif

        if(s->next_p_frame_damaged){
            if(s->pict_type==FF_B_TYPE)
            {av_log(NULL, AV_LOG_ERROR, "**!!before freedata7.\n");    _vld_err_handle(thiz);goto freedata;}
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

        _Dump_dsp_txt(s);

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
            if (thiz->use_dsp) {
//                av_log(NULL,AV_LOG_ERROR,"ddddddddddddd request_inputbuffer <----.\n");
                ioctl_ret = thiz->s.avctx->request_inputbuffer(thiz->pAcc, 1, (unsigned char *)thiz->p_pic->pdct);
//                av_log(NULL,AV_LOG_ERROR,"ddddddddddddd request_inputbuffer ---->.\n");
                if(-EPERM == ioctl_ret) {
                    _vld_err_handle(thiz);
                    av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, vld_thread request_inputbuffer fail.\n");
                    av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, free p_pic_dataq and release p_vld_dataq.\n");
                    if(p_vld)
                    {
                        if(p_vld->pbuf)
                        {
                            av_free(p_vld->pbuf);
                            p_vld->pbuf=NULL;
                        }
                        thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
                    }
                    thiz->iswaiting4flush=1;
                    _sendNULLFrame(thiz);
                    av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, p_frame_pool->ready_cnt=%d,p_mc_idct_dataq->ready_cnt=%d,p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_mc_idct_dataq->ready_cnt,thiz->p_vld_dataq->ready_cnt);
                    continue;
                }
            }
            _h263_reset_picture_data(thiz);
#ifdef _config_ambadec_assert_
            if(thiz->use_dsp)
            {
                for(i=0; i<thiz->pAcc->amba_buffer_number; i++) {
                    if(thiz->p_pic==(&thiz->picdata[i]))
                        break;
                }
                ambadec_assert_ffmpeg(i<thiz->pAcc->amba_buffer_number);
            }
            else
                ambadec_assert_ffmpeg(thiz->p_pic==(&thiz->picdata[0]) || thiz->p_pic==(&thiz->picdata[1]));
#endif
        }else {
            av_log(NULL,AV_LOG_ERROR,"maybe trash, should not come here.\n");
        }

        //av_log(NULL,AV_LOG_ERROR,"s->avctx->time_base.den=%d,s->avctx->time_base.num=%d.\n",s->avctx->time_base.den,s->avctx->time_base.num);
        //get pts related
/*        thiz->p_pic->vopinfo.uu.mpeg4s.vop_time_incre_res=s->avctx->time_base.den;
        thiz->p_pic->vopinfo.uu.mpeg4s.vop_pts_low=p_vld->pts&0xffffffff;
        thiz->p_pic->vopinfo.uu.mpeg4s.vop_pts_high=p_vld->pts&0xffffffff00000000;
        av_log(NULL,AV_LOG_DEBUG,"vop_pts_low=%d,vop_pts_high=%d.\n",thiz->p_pic->vopinfo.uu.mpeg4s.vop_pts_low,thiz->p_pic->vopinfo.uu.mpeg4s.vop_pts_high);
*/
        if (thiz->use_dsp)
        {
            vop_info_t* pvop_info=(vop_info_t*)thiz->p_pic->pvopinfo;
            //for bug917/921, hy pts issue
            pvop_info->vop_time_incre_res = thiz->pAcc->video_ticks;//s->avctx->time_base.den;
            if(AV_NOPTS_VALUE!=p_vld->pts)
            {
                pvop_info->vop_pts_low = (unsigned int)(p_vld->pts&0x00000000ffffffffULL);
                pvop_info->vop_pts_high = (unsigned int)((p_vld->pts&0xffffffff00000000ULL) >> 32);
            }
            else
            {
                pvop_info->vop_pts_low = pvop_info->vop_pts_high = 0xdeadbeef;//invalid pts fed to dsp: 3735928559
            }
            av_log(NULL,AV_LOG_DEBUG,"vop_time_incre_res=%u, vop_pts_low=%u,vop_pts_high=%u.\n",pvop_info->vop_time_incre_res,pvop_info->vop_pts_low, pvop_info->vop_pts_high);
        }

        //s->current_picture_ptr->pict_type=s->pict_type;
        thiz->p_pic->width=s->width;
        thiz->p_pic->height=s->height;

        s->linesize=s->current_picture_ptr->linesize[0];
        s->uvlinesize=s->current_picture_ptr->linesize[1];

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
            if(ret<0) {????;goto freedata;}
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
                {av_log(NULL, AV_LOG_ERROR, "**!!before freedata8.\n");
                if(s->current_picture_ptr)
                {
                    if(thiz->pic_pool->dec_lock(thiz->pic_pool,s->current_picture_ptr)==1)
                        thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)s->current_picture_ptr);
                    s->current_picture_ptr=NULL;
                }
                if(s->next_picture_ptr)
                {
                    if(thiz->pic_pool->dec_lock(thiz->pic_pool,s->next_picture_ptr)==1)
                        thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)s->next_picture_ptr);
                    s->next_picture_ptr=NULL;
                }
                if(s->last_picture_ptr)
                {
                    if(thiz->pic_pool->dec_lock(thiz->pic_pool,s->last_picture_ptr)==1)
                        thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)s->last_picture_ptr);
                    s->last_picture_ptr=NULL;
                }
                if(thiz->p_pic)
                {
                    if(thiz->p_pic->current_picture_ptr){
                        if(thiz->pic_pool->dec_lock(thiz->pic_pool,thiz->p_pic->current_picture_ptr)==1)
                             thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)thiz->p_pic->current_picture_ptr);
                        thiz->p_pic->current_picture_ptr=NULL;
                    }
                    if(thiz->p_pic->next_picture_ptr){
                        if(thiz->pic_pool->dec_lock(thiz->pic_pool,thiz->p_pic->next_picture_ptr)==1)
                             thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)thiz->p_pic->next_picture_ptr);
                        thiz->p_pic->next_picture_ptr=NULL;
                    }
                    if(thiz->p_pic->last_picture_ptr){
                        if(thiz->pic_pool->dec_lock(thiz->pic_pool,thiz->p_pic->last_picture_ptr)==1)
                             thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)thiz->p_pic->last_picture_ptr);
                        thiz->p_pic->last_picture_ptr=NULL;
                    }
                    thiz->p_pic_dataq->release_toready(thiz->p_pic_dataq,thiz->p_pic,0);
                    thiz->p_pic=NULL;
                }
                goto freedata;}
                memcpy(s->bitstream_buffer, buf + current_pos, buf_size - current_pos);
                s->bitstream_buffer_size= buf_size - current_pos;
            }
        }

//    intrax8_decoded:
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
        //av_log(NULL,AV_LOG_ERROR,"s->quarter_sample=%d.\n", s->quarter_sample);
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

        _Dump_dsp_txt_close_vld(thiz);

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
            p_node=thiz->p_mc_dataq->get_free(thiz->p_mc_dataq);
            p_node->p_ctx=thiz->p_pic;
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR," Putting p_pic to p_mc_dataq, thiz->p_pic=%p.\n", thiz->p_pic);
            #endif
            thiz->p_pic=NULL;
            thiz->p_mc_dataq->put_ready(thiz->p_mc_dataq,p_node,0);

        }
/*
        pthread_mutex_lock(&thiz->mutex);
        thiz->decoding_frame_cnt++;
        pthread_mutex_unlock(&thiz->mutex);
*/
        log_cur_frame++;

        if(0)
        {
freedata:
            //p_node=thiz->p_frame_pool->get_free(thiz->p_frame_pool);
            //p_node->p_ctx=NULL;
            _sendNULLFrame(thiz);
            #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"****error, send NULL pic when vld decoding error 2.\n");
            #endif
            //thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
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
            p_node->p_ctx=NULL;
            thiz->p_mc_idct_dataq->put_ready(thiz->p_mc_idct_dataq,p_node,_flag_cmd_exit_next_);
            if(s->loop_filter)
            {
                p_node=thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                p_node->p_ctx=NULL;
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
            p_node->p_ctx=NULL;
            thiz->p_mc_dataq->put_ready(thiz->p_mc_dataq,p_node,_flag_cmd_exit_next_);
            p_node=thiz->p_idct_dataq->get_cmd(thiz->p_idct_dataq);
            p_node->p_ctx=NULL;
            thiz->p_idct_dataq->put_ready(thiz->p_idct_dataq,p_node,_flag_cmd_exit_next_);
            if(s->loop_filter)
            {
                p_node=thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                p_node->p_ctx=NULL;
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

av_cold int ff_h263_decode_init_amba(AVCodecContext *avctx)
{
    H263AmbaDecContext_t* thiz=avctx->priv_data;
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
    s->decode_mb= ff_h263_decode_mb_amba;
    s->low_delay= 1;
    avctx->pix_fmt= avctx->get_format(avctx, avctx->codec->pix_fmts);
    s->unrestricted_mv= 1;
//    avctx->flags |= CODEC_FLAG_EMU_EDGE;
//    s->flags|=CODEC_FLAG_EMU_EDGE;
    log_cur_frame=0;

#if 0
//#ifdef __dump_temp__
    log_openfile_text_f(log_fd_temp, "temp");
#endif

    /* select sub codec */
    switch(avctx->codec->id) {
    case CODEC_ID_AMBA_P_H263:
        s->unrestricted_mv= 0;
        avctx->chroma_sample_location = AVCHROMA_LOC_CENTER;
        break;
    case CODEC_ID_AMBA_P_MPEG4:
        s->decode_mb= ff_mpeg4_decode_mb_amba;
        s->time_increment_bits = 4; /* default value for broken headers */
        s->h263_pred = 1;
        s->low_delay = 0; //default, might be overriden in the vol header during header parsing
        avctx->chroma_sample_location = AVCHROMA_LOC_LEFT;
        break;
    case CODEC_ID_AMBA_P_MSMPEG4V1:
        s->h263_msmpeg4 = 1;
        s->h263_pred = 1;
        s->msmpeg4_version=1;
        break;
    case CODEC_ID_AMBA_P_MSMPEG4V2:
        s->h263_msmpeg4 = 1;
        s->h263_pred = 1;
        s->msmpeg4_version=2;
        break;
    case CODEC_ID_AMBA_P_MSMPEG4V3:
        s->h263_msmpeg4 = 1;
        s->h263_pred = 1;
        s->msmpeg4_version=3;
        break;
    case CODEC_ID_AMBA_P_WMV1:
        s->h263_msmpeg4 = 1;
        s->h263_pred = 1;
        s->msmpeg4_version=4;
        break;
/*    case CODEC_ID_WMV2:
        s->h263_msmpeg4 = 1;
        s->h263_pred = 1;
        s->msmpeg4_version=5;
        break;
    case CODEC_ID_VC1:
    case CODEC_ID_WMV3:
        s->h263_msmpeg4 = 1;
        s->h263_pred = 1;
        s->msmpeg4_version=6;
        avctx->chroma_sample_location = AVCHROMA_LOC_LEFT;
        break;*/
    case CODEC_ID_AMBA_P_H263I:
        break;
    case CODEC_ID_AMBA_P_FLV1:
        s->h263_flv = 1;
        break;
    default:
        av_log(NULL,AV_LOG_ERROR,"error: unsupported codec_id=%d, in ff_h263_decode_init_amba.\n",avctx->codec->id);
        return -1;
    }
    s->codec_id= avctx->codec->id;

    /* for h263, we allocate the images after having read the header */
    if (avctx->codec->id != CODEC_ID_AMBA_P_H263 && avctx->codec->id != CODEC_ID_AMBA_P_MPEG4)
        if (MPV_common_init(s) < 0)
            return -1;

    if (CONFIG_MSMPEG4_DECODER && s->h263_msmpeg4)
        ff_msmpeg4_decode_init(avctx);
    else
        h263_decode_init_vlc_amba(s);

    _Dump_open_files();

    //parallel related
    //use default way 1: vld->mc_idct_addresidue
    thiz->vld_loop=1;//=thiz->mc_idct_loop=1;

    thiz->p_vld_dataq=ambadec_create_triqueue(_h263_callback_free_vld_data);
//    thiz->p_mc_idct_dataq=ambadec_create_triqueue(_h263_callback_NULL);

    thiz->p_frame_pool=ambadec_create_triqueue(_h263_callback_NULL);

//    thiz->vld_sent_row=thiz->mc_sent_row=0;
    thiz->pic_finished=1;
//    thiz->previous_pic_is_b=1;
    thiz->p_pic=NULL;
//    pthread_mutex_init(&thiz->mutex,NULL);

//#ifdef CONFIG_AMBA_MPEG4_IDCTMC_ACCELERATOR
//    if (init_amba_mpeg4_idctmc_accelerator(thiz, s->width, s->height))
//#endif

    //check if there's extern accelerator
    if (avctx->p_extern_accelerator && avctx->extern_accelerator_type == accelerator_type_amba_hybirdmpeg4_idctmc) {
        ambadec_assert_ffmpeg(avctx->acceleration);
//        ambadec_assert_ffmpeg(avctx->dec_eos_dummy_frame);
        ambadec_assert_ffmpeg(avctx->delete_accelerator);
        thiz->pAcc = (amba_decoding_accelerator_t*) avctx->p_extern_accelerator;
        thiz->use_dsp=1;
        thiz->use_permutated=-1;
        av_log(NULL, AV_LOG_ERROR, "++++ thiz->pAcc=%p,thiz->use_dsp=%d,thiz->use_permutated=%d\n",thiz->pAcc,thiz->use_dsp,thiz->use_permutated);
    } else {
        thiz->use_dsp=0;
        thiz->pAcc = NULL;
#ifdef __tmp_use_amba_permutation__
        thiz->use_permutated=-1;//un-initialized
#else
        thiz->use_permutated=0;
#endif
        av_log(NULL, AV_LOG_ERROR, "---- thiz->pAcc=%p,thiz->use_dsp=%d,thiz->use_permutated=%d\n",thiz->pAcc,thiz->use_dsp,thiz->use_permutated);
    }

    thiz->parallel_method=1;
    pthread_create(&thiz->tid_vld,NULL,thread_h263_vld,thiz);
    thiz->need_restart_pipeline=0;
    thiz->pipe_line_started=0;
    thiz->vld_need_flush = thiz->idct_mc_need_flush = thiz->deblock_need_flush = thiz->mc_need_flush = thiz->idct_need_flush = 0;
    thiz->iswaiting4flush = 0;
//    pthread_create(&thiz->tid_mc_idct,NULL,thread_h263_mc_idct_addresidue,thiz);

    _Dump_raw_data_init();

    av_log(NULL, AV_LOG_ERROR,"***ff_h263_decode_init_amba end.\n");
    return 0;
}

av_cold int ff_h263_decode_end_amba(AVCodecContext *avctx)
{
    H263AmbaDecContext_t* thiz=avctx->priv_data;
    MpegEncContext *s = &thiz->s;
//    int i=0;
    ctx_nodef_t* p_node;
    void* pv;
    int ret;

#ifdef __log_decoding_process__
    av_log(NULL,AV_LOG_ERROR," **** mpeg4-amba end, start.\n");
#endif

    //parallel related , exit sub threads
    ambadec_assert_ffmpeg(thiz->parallel_method==1);
    p_node=thiz->p_vld_dataq->get_cmd(thiz->p_vld_dataq);
    p_node->p_ctx=NULL;
    thiz->p_vld_dataq->put_ready(thiz->p_vld_dataq,p_node,_flag_cmd_exit_next_);
    ret=pthread_join(thiz->tid_vld,&pv);
    av_log(NULL,AV_LOG_DEBUG,"ret=%d.\n", ret);

#ifdef __log_decoding_process__
    av_log(NULL,AV_LOG_ERROR," **** mpeg4-amba end, pipe-line exit done.\n");
#endif

    ambadec_destroy_triqueue(thiz->p_vld_dataq);

    ambadec_destroy_triqueue(thiz->p_frame_pool);

#ifdef __log_decoding_process__
    av_log(NULL,AV_LOG_ERROR," **** mpeg4-amba end, destroy p_vld_dataq, p_frame_pool done.\n");
#endif

#if 0
//#ifdef __dump_temp__
    log_closefile(log_fd_temp);
#endif

    _Dump_close_files();

    MPV_common_end(s);

#ifdef __log_decoding_process__
    av_log(NULL,AV_LOG_ERROR," **** mpeg4-amba end, end.\n");
#endif

//    pthread_mutex_destroy(&thiz->mutex);

    return 0;
}

int ff_h263_decode_frame_amba(AVCodecContext *avctx,void *data, int *data_size,AVPacket *avpkt)
{
    H263AmbaDecContext_t*thiz = avctx->priv_data;
    AVFrame *pict = data;
    AVFrame* pready;
    unsigned int flag;
    ctx_nodef_t* p_node;
    h263_vld_data_t* p_data;
//    int must_get_pic=0;

    thiz->s.flags= avctx->flags;
    thiz->s.flags2= avctx->flags2;

    if(NULL==avpkt->data && 0==avpkt->size && 0==avpkt->flags)
    {
	   avpkt->flags = 1;//so this section be called ONLY ONCE per eos
	   //send eos packet
	   av_log(NULL,AV_LOG_ERROR,"ff_h263_decode_frame_amba: _flag_cmd_eos_<--.\n");
	    p_node=thiz->p_vld_dataq->get_cmd(thiz->p_vld_dataq);
	    p_node->p_ctx = NULL;
	    thiz->p_vld_dataq->put_ready(thiz->p_vld_dataq, p_node, _flag_cmd_eos_);
	    av_log(NULL,AV_LOG_ERROR,"ff_h263_decode_frame_amba: _flag_cmd_eos_-->.\n");
    }

    if(thiz->iswaiting4flush)
    {
        av_log(NULL,AV_LOG_DEBUG,"-----------------------udec stopped, waiting4flush and rtn.\n");
        *data_size=0;
        return -EPERM;
    }

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
        p_data->pts = avpkt->pts;
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
            pready=p_node->p_ctx;

            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"Escape: decode frame escape from p_frame_pool, pready=%p.\n",pready);
            #endif


            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR," in decode_frame, get a decoded frame pready=%p,pict=%p.\n",pready,pict);
            #endif

            if(!pready)
            {
                *data_size=0;
                return avpkt->size;
            }
            if(_Does_render_by_filter(thiz)) {
                *pict=*pready;
                *data_size=sizeof(AVFrame);
            }else {
                *data_size=0;
            }
            if(thiz->pic_pool->dec_lock(thiz->pic_pool,pready)==1)
                avctx->release_buffer(avctx,pready);

            if(pready)
            {
                thiz->p_frame_pool->release(thiz->p_frame_pool,pready,0);
            }

            return avpkt->size;
        }
    }
    else
    {
        //special case for last frames
        //if(thiz->p_frame_pool->ready_cnt || thiz->p_vld_dataq->ready_cnt  || thiz->decoding_frame_cnt)
        av_log(NULL,AV_LOG_ERROR,"p_frame_pool->get() in<--, p_frame_pool->ready_cnt=%d,p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_vld_dataq->ready_cnt);
        {
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"Waiting: decode frame wait on p_frame_pool(last frames), thiz->p_frame_pool->ready_cnt=%d,thiz->p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_vld_dataq->ready_cnt);
            #endif

            p_node=thiz->p_frame_pool->get(thiz->p_frame_pool);
            pready=p_node->p_ctx;
            flag = p_node->flag;
			av_log(NULL,AV_LOG_ERROR,"p_frame_pool->get() out-->, p_frame_pool->ready_cnt=%d,p_vld_dataq->ready_cnt=%d.\n",thiz->p_frame_pool->ready_cnt,thiz->p_vld_dataq->ready_cnt);
	    	if (flag == _flag_cmd_eos_)
		{
		        ambadec_assert_ffmpeg(NULL==pready);
			*data_size=sizeof(AVFrame);
			return 0;
		}

            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"Escape: decode frame escape from p_frame_pool(last frames), pready=%p.\n",pready);
            #endif


            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR," in decode_frame, get a decoded frame(last frames), pready=%p,pict=%p.\n",pready,pict);
            #endif

            if(!pready)
            {
                *data_size=0;
                return (thiz->p_frame_pool->ready_cnt+thiz->p_vld_dataq->ready_cnt+1);//+thiz->decoding_frame_cnt
            }
            if(_Does_render_by_filter(thiz)) {
                *pict=*pready;
                *data_size=sizeof(AVFrame);
            }else {
                *data_size=0;
            }
            if(thiz->pic_pool->dec_lock(thiz->pic_pool,pready)==1)
                avctx->release_buffer(avctx,pready);

            if(pready)
            {
                thiz->p_frame_pool->release(thiz->p_frame_pool,pready,0);
            }

            return (thiz->p_frame_pool->ready_cnt+thiz->p_vld_dataq->ready_cnt+1);//+thiz->decoding_frame_cnt
        }
/*        else if(thiz->s.next_picture_ptr)// get next reference frame
        {
            *pict=*((AVFrame*)(thiz->s.next_picture_ptr));
            *data_size=sizeof(AVFrame);
            return 0;
        }*/
    }

    *data_size=0;

    return 0;
}

static void amba_h263_flush(AVCodecContext *avctx)
{
    H263AmbaDecContext_t* thiz=avctx->priv_data;
    MpegEncContext *s = &thiz->s;
    ctx_nodef_t* p_node;
    AVFrame *pic;
    unsigned int flag;
    int i=0;
    int buf_num=0;

    if (!thiz->pipe_line_started)
        return;

    av_log(NULL,AV_LOG_DEBUG,"** Flush pipeline 1.\n");

    //set flush flag for each thread
    thiz->vld_need_flush = 1;
    if (thiz->parallel_method == 1) {
        thiz->idct_mc_need_flush = 1;
    } else {
        thiz->idct_need_flush = 1;
        thiz->mc_need_flush = 1;
    }
    if (s->loop_filter) {
        thiz->deblock_need_flush = 1;
    }
    av_log(NULL,AV_LOG_DEBUG,"** Flush pipeline 2.\n");
    //send flush packet
    //usleep(40000);//tmp modify: hard code here to avoid "p_frame_pool->get <---", debug later
    if(!thiz->use_dsp)//tmp modify for bug#2361, sw-pipeline violent seek, CBuffer leak issue, FIXME
    {
        usleep(40000);
    }
    p_node = thiz->p_vld_dataq->get_cmd(thiz->p_vld_dataq);
    p_node->p_ctx = NULL;
    thiz->p_vld_dataq->put_ready(thiz->p_vld_dataq, p_node, _flag_cmd_flush_);
    av_log(NULL,AV_LOG_DEBUG,"** Flush pipeline 3.\n");
    //clear all remaining frames in frame pool, until flush is done
    while (1) {
        //pthread_mutex_lock(&thiz->mutex);
        //av_log(NULL,AV_LOG_DEBUG,"** Flush pipeline 3.1, pframepool->get  get_cnt=%d.<---.\n", thiz->p_frame_pool->get_cnt);
        p_node=thiz->p_frame_pool->get(thiz->p_frame_pool);
        pic = (AVFrame*)p_node->p_ctx;
        flag = p_node->flag;//for multi-thread sync, cmd flag should be take out first before other operation
        //av_log(NULL,AV_LOG_DEBUG,"** Flush pipeline 3.1, pframepool->get  get_cnt=%d. pic=%p, flag=%d --->.\n", thiz->p_frame_pool->get_cnt,pic,flag);
        if (pic && flag != _flag_cmd_flush_) {
            if(thiz->pic_pool->dec_lock(thiz->pic_pool,pic)==1) {
                av_log(NULL,AV_LOG_DEBUG,"** Flush pipeline 3.1 %p.\n", pic);
                pic->type = DFlushing_frame;
                avctx->release_buffer(avctx,pic);
            }
            thiz->p_frame_pool->release(thiz->p_frame_pool,pic,0);
        } else if (!pic  && flag == _flag_cmd_flush_){
            av_log(NULL,AV_LOG_DEBUG,"** Flush pipeline 3.1 done.  pic=%p, flag=%d.\n",pic,flag);
            //pthread_mutex_unlock(&thiz->mutex);
            break;
        }
        //pthread_mutex_unlock(&thiz->mutex);
    }
    //clear last/next/current picture, ensure all is flushed
    buf_num = thiz->use_dsp?thiz->pAcc->amba_buffer_number:2;
    for(i=0; i<buf_num; i++)
    {
        flush_holded_pictures(thiz, &thiz->picdata[i]);
    }

    if(s->last_picture_ptr){
        if(thiz->pic_pool->dec_lock(thiz->pic_pool,s->last_picture_ptr)==1) {
            //av_log(NULL,AV_LOG_ERROR,"** Flush pipeline 4 last %p.\n", s->last_picture_ptr);
            s->last_picture_ptr->type = DFlushing_frame;
            thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)s->last_picture_ptr);
        }
        s->last_picture_ptr = NULL;
    }
    if(s->current_picture_ptr){
        if(thiz->pic_pool->dec_lock(thiz->pic_pool,s->current_picture_ptr)==1) {
            //av_log(NULL,AV_LOG_ERROR,"** Flush pipeline 4 current %p.\n", s->current_picture_ptr);
            s->current_picture_ptr->type = DFlushing_frame;
            thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)s->current_picture_ptr);
        }
        s->current_picture_ptr = NULL;
    }
    if(s->next_picture_ptr){
        if(thiz->pic_pool->dec_lock(thiz->pic_pool,s->next_picture_ptr)==1) {
            //av_log(NULL,AV_LOG_ERROR,"** Flush pipeline 4 next %p.\n", s->next_picture_ptr);
            s->next_picture_ptr->type = DFlushing_frame;
            thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)s->next_picture_ptr);
        }
        s->next_picture_ptr = NULL;
    }

    //s->last_picture_ptr = s->next_picture_ptr = s->current_picture_ptr = NULL;

    av_log(NULL,AV_LOG_DEBUG,"** Flush pipeline 4.\n");

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
//    thiz->decoding_frame_cnt = 0;

    //reset p_pic_dataq, so after seek, first decode command will begin from the base address of ring buffer for DSP
    av_log(NULL,AV_LOG_DEBUG,"++++ reset p_pic_dataq 1: ready_cnt=%d, free_cnt=%d, used_cnt=%d.\n", thiz->p_pic_dataq->ready_cnt,thiz->p_pic_dataq->free_cnt,thiz->p_pic_dataq->used_cnt);
    ambadec_reset_triqueue(thiz->p_pic_dataq);
    av_log(NULL,AV_LOG_DEBUG,"++++ reset p_pic_dataq 2: ready_cnt=%d, free_cnt=%d, used_cnt=%d.\n", thiz->p_pic_dataq->ready_cnt,thiz->p_pic_dataq->free_cnt,thiz->p_pic_dataq->used_cnt);
    for(i=0; i<buf_num; i++)
    {
        p_node=thiz->p_pic_dataq->get_free(thiz->p_pic_dataq);
        p_node->p_ctx=&thiz->picdata[i];
        thiz->p_pic_dataq->put_ready(thiz->p_pic_dataq,p_node,0);
    }
    av_log(NULL,AV_LOG_DEBUG,"++++ reorder p_pic_dataq: ready_cnt=%d, free_cnt=%d, used_cnt=%d.\n", thiz->p_pic_dataq->ready_cnt,thiz->p_pic_dataq->free_cnt,thiz->p_pic_dataq->used_cnt);

    if(thiz->use_dsp)//hard code here, for CHK_TRASH in amdsp_common.cpp
    {
        thiz->pAcc->req_cnt=0;
        thiz->pAcc->dec_cnt=0;
    }

    //reset 3 tri queue
    ambadec_reset_triqueue(thiz->p_frame_pool);
    ambadec_reset_triqueue(thiz->p_mc_idct_dataq);
    ambadec_reset_triqueue(thiz->p_vld_dataq);

    //flush finished, clear the flag to go on decoding
    thiz->iswaiting4flush=0;
    thiz->vld_need_flush = 0;
    thiz->idct_mc_need_flush = 0;
    //printf("** Flush pipeline done.\n");

}

//decoders below all have pipelines, so all need  flag CODEC_CAP_DELAY to get left frames out in EOS
AVCodec ff_mpeg4_amba_decoder = {
    "mpeg4-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_MPEG4,
    sizeof(H263AmbaDecContext_t),
    ff_h263_decode_init_amba,
    NULL,
    ff_h263_decode_end_amba,
    ff_h263_decode_frame_amba,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | CODEC_CAP_DELAY,
    .flush= amba_h263_flush,
    .long_name= NULL_IF_CONFIG_SMALL("MPEG-4 part 2 (parallel nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_h263_amba_decoder = {
    "h263-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_H263,
    sizeof(H263AmbaDecContext_t),
    ff_h263_decode_init_amba,
    NULL,
    ff_h263_decode_end_amba,
    ff_h263_decode_frame_amba,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | CODEC_CAP_DELAY,
    .flush= amba_h263_flush,
    .long_name= NULL_IF_CONFIG_SMALL("H.263 / H.263-1996, H.263+ / H.263-1998 / H.263 version 2 (parallel nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_msmpeg4v1_amba_decoder = {
    "msmpeg4v1-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_MSMPEG4V1,
    sizeof(H263AmbaDecContext_t),
    ff_h263_decode_init_amba,
    NULL,
    ff_h263_decode_end_amba,
    ff_h263_decode_frame_amba,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    .flush= amba_h263_flush,
    .long_name= NULL_IF_CONFIG_SMALL("MPEG-4 part 2 Microsoft variant version 1 (parallel nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_msmpeg4v2_amba_decoder = {
    "msmpeg4v2-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_MSMPEG4V2,
    sizeof(H263AmbaDecContext_t),
    ff_h263_decode_init_amba,
    NULL,
    ff_h263_decode_end_amba,
    ff_h263_decode_frame_amba,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    .flush= amba_h263_flush,
    .long_name= NULL_IF_CONFIG_SMALL("MPEG-4 part 2 Microsoft variant version 2 (parallel nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_msmpeg4v3_amba_decoder = {
    "msmpeg4-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_MSMPEG4V3,
    sizeof(H263AmbaDecContext_t),
    ff_h263_decode_init_amba,
    NULL,
    ff_h263_decode_end_amba,
    ff_h263_decode_frame_amba,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    .flush= amba_h263_flush,
    .long_name= NULL_IF_CONFIG_SMALL("MPEG-4 part 2 Microsoft variant version 3 (parallel nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_wmv1_amba_decoder = {
    "wmv1-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_WMV1,
    sizeof(H263AmbaDecContext_t),
    ff_h263_decode_init_amba,
    NULL,
    ff_h263_decode_end_amba,
    ff_h263_decode_frame_amba,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    .flush= amba_h263_flush,
    .long_name= NULL_IF_CONFIG_SMALL("Windows Media Video 7 (parallel nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_h263i_amba_decoder = {
    "h263i-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_H263I,
    sizeof(H263AmbaDecContext_t),
    ff_h263_decode_init_amba,
    NULL,
    ff_h263_decode_end_amba,
    ff_h263_decode_frame_amba,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    .flush= amba_h263_flush,
    .long_name = NULL_IF_CONFIG_SMALL("Intel H.263 (parallel nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_flv_amba_decoder = {
    "flv-amba",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_P_FLV1,
    sizeof(H263AmbaDecContext_t),
    ff_h263_decode_init_amba,
    NULL,
    ff_h263_decode_end_amba,
    ff_h263_decode_frame_amba,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    .flush= amba_h263_flush,
    .long_name= NULL_IF_CONFIG_SMALL("Flash Video (FLV) (parallel nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};
