/*
 * H.263 nv12 decoder
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
 * @file libavcodec/h263dec_nv12.c
 * H.263 nv12 decoder.
 */

#include "internal.h"
#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "h263_parser.h"
#include "mpeg4video_parser.h"
#include "msmpeg4.h"

//#define DEBUG
//#define PRINT_FRAME_TIME

//#define _dump_raw_data_
#ifdef _dump_raw_data_
static int dump_cnt=0;
static char dump_fn[40];
static char dump_fn_t[40]="raw_%d";
static FILE* dump_pf;
#endif

av_cold int ff_h263_decode_init_nv12(AVCodecContext *avctx);
av_cold int ff_h263_decode_end_nv12(AVCodecContext *avctx);
const uint8_t *ff_h263_find_resync_marker(const uint8_t *restrict p, const uint8_t * restrict end);
int ff_mpeg4_decode_partitions(MpegEncContext *s);
void ff_h263_update_motion_val_nv12(MpegEncContext * s);
void MPV_decode_mb_nv12(MpegEncContext *s, DCTELEM block[12][64]);
int ff_h263_decode_frame_nv12(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt);
int ff_intel_h263_decode_picture_header(MpegEncContext *s);
int ff_flv_decode_picture_header(MpegEncContext *s);
int h263_decode_picture_header(MpegEncContext *s);
int ff_h263_get_gob_height(MpegEncContext *s);
void ff_er_frame_end_nv12(MpegEncContext *s);

void ff_thread_finish_setup(AVCodecContext *avctx);
void ff_vdpau_mpeg4_decode_picture(MpegEncContext *s, const uint8_t *buf, int buf_size);

av_cold int ff_h263_decode_init_nv12(AVCodecContext *avctx)
{
    MpegEncContext *s = avctx->priv_data;
    
//    av_log(NULL, AV_LOG_ERROR, "init 1 s->picture_number=%d\n",s->picture_number);

    s->avctx = avctx;
    s->out_format = FMT_H263;

    s->width  = avctx->coded_width;
    s->height = avctx->coded_height;
    s->workaround_bugs= avctx->workaround_bugs;

    // set defaults
    MPV_decode_defaults(s);
    s->quant_precision=5;
    s->decode_mb= ff_h263_decode_mb_nv12;
    s->low_delay= 1;
    avctx->pix_fmt= avctx->get_format(avctx, avctx->codec->pix_fmts);
    s->unrestricted_mv= 1;
//    avctx->flags |= CODEC_FLAG_EMU_EDGE;
//    s->flags|=CODEC_FLAG_EMU_EDGE;

#if 0      
//#ifdef __dump_temp__
    log_openfile_text_f(log_fd_temp, "temp");
#endif

    /* select sub codec */
    switch(avctx->codec->id) {
    case CODEC_ID_AMBA_H263:
        s->unrestricted_mv= 0;
        avctx->chroma_sample_location = AVCHROMA_LOC_CENTER;
        break;
    case CODEC_ID_AMBA_MPEG4:
        s->decode_mb= ff_mpeg4_decode_mb_nv12;
        s->time_increment_bits = 4; /* default value for broken headers */
        s->h263_pred = 1;
        s->low_delay = 0; //default, might be overriden in the vol header during header parsing
        avctx->chroma_sample_location = AVCHROMA_LOC_LEFT;
        break;
    case CODEC_ID_AMBA_MSMPEG4V1:
        s->h263_msmpeg4 = 1;
        s->h263_pred = 1;
        s->msmpeg4_version=1;
        break;
    case CODEC_ID_AMBA_MSMPEG4V2:
        s->h263_msmpeg4 = 1;
        s->h263_pred = 1;
        s->msmpeg4_version=2;
        break;
    case CODEC_ID_AMBA_MSMPEG4V3:
        s->h263_msmpeg4 = 1;
        s->h263_pred = 1;
        s->msmpeg4_version=3;
        break;
    case CODEC_ID_AMBA_WMV1:
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
    case CODEC_ID_AMBA_H263I:
        break;
    case CODEC_ID_AMBA_FLV1:
        s->h263_flv = 1;
        break;
    default:
        av_log(NULL,AV_LOG_ERROR,"error: unsupported codec_id=%d, in ff_h263_decode_init_nv12.\n",avctx->codec->id);
        return -1;
    }
    s->codec_id= avctx->codec->id;
    avctx->hwaccel= ff_find_hwaccel(avctx->codec->id, avctx->pix_fmt);

    /* for h263, we allocate the images after having read the header */
    if (avctx->codec->id != CODEC_ID_AMBA_H263 && avctx->codec->id != CODEC_ID_AMBA_MPEG4)
        if (MPV_common_init(s) < 0)
            return -1;

    if (CONFIG_MSMPEG4_DECODER && s->h263_msmpeg4)
        ff_msmpeg4_decode_init(s->avctx);
    else
        h263_decode_init_vlc_nv12(s);
    
    #ifdef _dump_raw_data_    
    av_log(NULL, AV_LOG_ERROR, "init 2 s->picture_number=%d\n",s->picture_number);
    dump_cnt=0;
    #endif
    log_cur_frame=0;
    return 0;
}

av_cold int ff_h263_decode_end_nv12(AVCodecContext *avctx)
{
    MpegEncContext *s = avctx->priv_data;

#if 0      
//#ifdef __dump_temp__
    log_closefile(log_fd_temp);
#endif

    MPV_common_end(s);
    return 0;
}

/**
 * returns the number of bytes consumed for building the current frame
 */
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

static int decode_slice_nv12(MpegEncContext *s){
    const int part_mask= s->partitioned_frame ? (AC_END|AC_ERROR) : 0x7F;
    const int mb_size= 16>>s->avctx->lowres;
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

    if (s->avctx->hwaccel) {
        const uint8_t *start= s->gb.buffer + get_bits_count(&s->gb)/8;
        const uint8_t *end  = ff_h263_find_resync_marker(start + 1, s->gb.buffer_end);
        skip_bits_long(&s->gb, 8*(end - start));
        return s->avctx->hwaccel->decode_slice(s->avctx, start, end - start);
    }

    if(s->partitioned_frame){
        const int qscale= s->qscale;

        if(s->codec_id==CODEC_ID_AMBA_MPEG4){
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

        ff_init_block_index_nv12(s);
        for(; s->mb_x < s->mb_width; s->mb_x++) {
            int ret;
//            av_log(NULL,AV_LOG_ERROR,"start decoding mb x=%d,y=%d.\n",s->mb_x,s->mb_y);
            ff_update_block_index_nv12(s);

            if(s->resync_mb_x == s->mb_x && s->resync_mb_y+1 == s->mb_y){
                s->first_slice_line=0;
            }

            /* DCT & quantize */

            s->mv_dir = MV_DIR_FORWARD;
            s->mv_type = MV_TYPE_16X16;
//            s->mb_skipped = 0;
//printf("%d %d %06X\n", ret, get_bits_count(&s->gb), show_bits(&s->gb, 24));
            ret= s->decode_mb(s, s->block);

            if (s->pict_type!=FF_B_TYPE)
                ff_h263_update_motion_val_nv12(s);
            
//            av_log(NULL,AV_LOG_ERROR,"after decoding mb ret%d.\n",ret);

            if(ret<0){
                const int xy= s->mb_x + s->mb_y*s->mb_stride;
                if(ret==SLICE_END){
                    MPV_decode_mb_nv12(s, s->block);
                    if(s->loop_filter)
                        ff_h263_loop_filter_nv12(s);

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

            MPV_decode_mb_nv12(s, s->block);
            if(s->loop_filter)
                ff_h263_loop_filter_nv12(s);
        }

        ff_draw_horiz_band(s, s->mb_y*mb_size, mb_size);

        s->mb_x= 0;
    }

    assert(s->mb_x==0 && s->mb_y==s->mb_height);
    
//    av_log(NULL,AV_LOG_ERROR,"end decode mb loop x=%d,y=%d.\n",s->mb_x,s->mb_y);

    /* try to detect the padding bug */
    if(      s->codec_id==CODEC_ID_AMBA_MPEG4
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

int ff_h263_decode_frame_nv12(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    MpegEncContext *s = avctx->priv_data;
    int ret;
    AVFrame *pict = data;
    
#ifdef PRINT_FRAME_TIME
uint64_t time= rdtsc();
#endif
    s->flags= avctx->flags;
    s->flags2= avctx->flags2;

    /* no supplementary picture */
    if (buf_size == 0) {
        /* special case for last picture */
        if (s->low_delay==0 && s->next_picture_ptr) {
            *pict= *(AVFrame*)s->next_picture_ptr;
            s->next_picture_ptr= NULL;

            *data_size = sizeof(AVFrame);
        }

        return 0;
    }

    if(s->flags&CODEC_FLAG_TRUNCATED){
        int next;

        if(CONFIG_MPEG4_DECODER && s->codec_id==CODEC_ID_AMBA_MPEG4){
            next= ff_mpeg4_find_frame_end(&s->parse_context, buf, buf_size);
        }else if(CONFIG_H263_DECODER && s->codec_id==CODEC_ID_AMBA_H263){
            next= ff_h263_find_frame_end(&s->parse_context, buf, buf_size);
        }else{
            av_log(s->avctx, AV_LOG_ERROR, "this codec does not support truncated bitstreams\n");
            return -1;
        }

        if( ff_combine_frame(&s->parse_context, next, (const uint8_t **)&buf, &buf_size) < 0 )
            return buf_size;
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
    if(s->divx_packed && s->xvid_build>=0 && s->bitstream_buffer_size){
        int i;
        for(i=0; i<buf_size-3; i++){
            if(buf[i]==0 && buf[i+1]==0 && buf[i+2]==1){
                if(buf[i+3]==0xB0){
                    av_log(s->avctx, AV_LOG_WARNING, "Discarding excessive bitstream in packed xvid\n");
                    s->bitstream_buffer_size=0;
                }
                break;
            }
        }
    }

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
            return -1;
    }

    /* We need to set current_picture_ptr before reading the header,
     * otherwise we cannot store anyting in there */
    if(s->current_picture_ptr==NULL || s->current_picture_ptr->data[0]){
        int i= ff_find_unused_picture(s, 0);
        s->current_picture_ptr= &s->picture[i];
    }
    
//    av_log(NULL, AV_LOG_ERROR, "in 1 s->picture_number=%d,s->avctx->extradata_size=%d\n",s->picture_number,s->avctx->extradata_size);

    /* let's go :-) */
    if (CONFIG_WMV2_DECODER && s->msmpeg4_version==5) {
        ret= ff_wmv2_decode_picture_header(s);
    } else if (CONFIG_MSMPEG4_DECODER && s->msmpeg4_version) {
        ret = msmpeg4_decode_picture_header(s);
    } else if (CONFIG_MPEG4_DECODER && s->h263_pred) {
        if(s->avctx->extradata_size && s->picture_number==0){
//            av_log(NULL, AV_LOG_ERROR, "***** decoding extradata s->picture_number=%d,s->avctx->extradata_size=%d\n",s->picture_number,s->avctx->extradata_size);

            GetBitContext gb;

            init_get_bits(&gb, s->avctx->extradata, s->avctx->extradata_size*8);
            ret = ff_mpeg4_decode_picture_header_nv12(s, &gb);
        }
        ret = ff_mpeg4_decode_picture_header_nv12(s, &s->gb);
    } else if (s->codec_id == CODEC_ID_AMBA_H263I) {
        ret = ff_intel_h263_decode_picture_header(s);
    } else if (CONFIG_FLV_DECODER && s->h263_flv) {
        ret = ff_flv_decode_picture_header(s);
    } else {
        ret = h263_decode_picture_header(s);
    }

    if(ret==FRAME_SKIPPED) return get_consumed_bytes(s, buf_size);

    /* skip if the header was thrashed */
    if (ret < 0){
        av_log(s->avctx, AV_LOG_ERROR, "header damaged, ret=%d\n",ret);
        return -1;
    }

    avctx->has_b_frames= !s->low_delay;

    if(s->xvid_build==-1 && s->divx_version==-1 && s->lavc_build==-1){
        if(s->stream_codec_tag == AV_RL32("XVID") ||
           s->codec_tag == AV_RL32("XVID") || s->codec_tag == AV_RL32("XVIX") ||
           s->codec_tag == AV_RL32("RMP4") ||
           s->codec_tag == AV_RL32("SIPP")
           )
            s->xvid_build= 0;
#if 0
        if(s->codec_tag == AV_RL32("DIVX") && s->vo_type==0 && s->vol_control_parameters==1
           && s->padding_bug_score > 0 && s->low_delay) // XVID with modified fourcc
            s->xvid_build= 0;
#endif
    }

    if(s->xvid_build==-1 && s->divx_version==-1 && s->lavc_build==-1){
        if(s->codec_tag == AV_RL32("DIVX") && s->vo_type==0 && s->vol_control_parameters==0)
            s->divx_version= 400; //divx 4
    }

    if(s->xvid_build>=0 && s->divx_version>=0){
        s->divx_version=
        s->divx_build= -1;
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

        if(s->xvid_build<=3U)
            s->padding_bug_score= 256*256*256*64;

        if(s->xvid_build<=1U)
            s->workaround_bugs|= FF_BUG_QPEL_CHROMA;

        if(s->xvid_build<=12U)
            s->workaround_bugs|= FF_BUG_EDGE;

        if(s->xvid_build<=32U)
            s->workaround_bugs|= FF_BUG_DC_CLIP;

#define SET_QPEL_FUNC(postfix1, postfix2) \
    s->dsp.put_ ## postfix1 = ff_put_ ## postfix2;\
    s->dsp.put_no_rnd_ ## postfix1 = ff_put_no_rnd_ ## postfix2;\
    s->dsp.avg_ ## postfix1 = ff_avg_ ## postfix2;

        if(s->lavc_build<4653U)
            s->workaround_bugs|= FF_BUG_STD_QPEL;

        if(s->lavc_build<4655U)
            s->workaround_bugs|= FF_BUG_DIRECT_BLOCKSIZE;

        if(s->lavc_build<4670U){
            s->workaround_bugs|= FF_BUG_EDGE;
        }

        if(s->lavc_build<=4712U)
            s->workaround_bugs|= FF_BUG_DC_CLIP;

        if(s->divx_version>=0)
            s->workaround_bugs|= FF_BUG_DIRECT_BLOCKSIZE;
//printf("padding_bug_score: %d\n", s->padding_bug_score);
        if(s->divx_version==501 && s->divx_build==20020416)
            s->padding_bug_score= 256*256*256*64;

        if(s->divx_version<500U){
            s->workaround_bugs|= FF_BUG_EDGE;
        }

        if(s->divx_version>=0)
            s->workaround_bugs|= FF_BUG_HPEL_CHROMA;
#if 0
        if(s->divx_version==500)
            s->padding_bug_score= 256*256*256*64;

        /* very ugly XVID padding bug detection FIXME/XXX solve this differently
         * Let us hope this at least works.
         */
        if(   s->resync_marker==0 && s->data_partitioning==0 && s->divx_version==-1
           && s->codec_id==CODEC_ID_MPEG4 && s->vo_type==0)
            s->workaround_bugs|= FF_BUG_NO_PADDING;

        if(s->lavc_build<4609U) //FIXME not sure about the version num but a 4609 file seems ok
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

    if(avctx->debug & FF_DEBUG_BUGS)
        av_log(s->avctx, AV_LOG_DEBUG, "bugs: %X lavc_build:%d xvid_build:%d divx_version:%d divx_build:%d %s\n",
               s->workaround_bugs, s->lavc_build, s->xvid_build, s->divx_version, s->divx_build,
               s->divx_packed ? "p" : "");

#if 0 // dump bits per frame / qp / complexity
{
    static FILE *f=NULL;
    if(!f) f=fopen("rate_qp_cplx.txt", "w");
    fprintf(f, "%d %d %f\n", buf_size, s->qscale, buf_size*(double)s->qscale);
}
#endif

#if HAVE_MMX
    if(s->codec_id == CODEC_ID_AMBA_MPEG4 && s->xvid_build>=0 && avctx->idct_algo == FF_IDCT_AUTO && (av_get_cpu_flags() & AV_CPU_FLAG_MMX)) {
        avctx->idct_algo= FF_IDCT_XVIDMMX;
        avctx->coded_width= 0; // force reinit
//        dsputil_init(&s->dsp, avctx);
        s->picture_number=0;
    }
#endif

        /* After H263 & mpeg4 header decode we have the height, width,*/
        /* and other parameters. So then we could init the picture   */
        /* FIXME: By the way H263 decoder is evolving it should have */
        /* an H263EncContext                                         */

    if (   s->width  != avctx->coded_width
        || s->height != avctx->coded_height) {
        /* H.263 could change picture size any time */
        ParseContext pc= s->parse_context; //FIXME move these demuxng hack to avformat
        s->parse_context.buffer=0;
        MPV_common_end(s);
        s->parse_context= pc;
    }
    if (!s->context_initialized) {
        avcodec_set_dimensions(avctx, s->width, s->height);

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
        
    if((s->codec_id==CODEC_ID_AMBA_H263 || s->codec_id==CODEC_ID_H263P || s->codec_id == CODEC_ID_AMBA_H263I))
        s->gob_index = ff_h263_get_gob_height(s);

    // for hurry_up==5
    s->current_picture.pict_type= s->pict_type;
    s->current_picture.key_frame= s->pict_type == FF_I_TYPE;

    /* skip B-frames if we don't have reference frames */
    if(s->last_picture_ptr==NULL && (s->pict_type==FF_B_TYPE || s->dropable)) return get_consumed_bytes(s, buf_size);
    /* skip b frames if we are in a hurry */
    if(
#if FF_API_HURRY_UP
    avctx->hurry_up &&
#endif
    s->pict_type==FF_B_TYPE) return get_consumed_bytes(s, buf_size);

    if(   (avctx->skip_frame >= AVDISCARD_NONREF && s->pict_type==FF_B_TYPE)
       || (avctx->skip_frame >= AVDISCARD_NONKEY && s->pict_type!=FF_I_TYPE)
       ||  avctx->skip_frame >= AVDISCARD_ALL)
        return get_consumed_bytes(s, buf_size);    
#if FF_API_HURRY_UP
    /* skip everything if we are in a hurry>=5 */
    if(avctx->hurry_up>=5) return get_consumed_bytes(s, buf_size);
#endif
    if(s->next_p_frame_damaged){
        if(s->pict_type==FF_B_TYPE)
            return get_consumed_bytes(s, buf_size);
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

    if(MPV_frame_start(s, avctx) < 0)
        return -1;

    if (!s->divx_packed) ff_thread_finish_setup(avctx);

    if (CONFIG_MPEG4_VDPAU_DECODER && (s->avctx->codec->capabilities & CODEC_CAP_HWACCEL_VDPAU)) {
        ff_vdpau_mpeg4_decode_picture(s, s->gb.buffer, s->gb.buffer_end - s->gb.buffer);
        goto frame_end;
    }
    
    if (avctx->hwaccel) {
        if (avctx->hwaccel->start_frame(avctx, s->gb.buffer, s->gb.buffer_end - s->gb.buffer) < 0)
            return -1;
    }
    
#ifdef __dump_DSP_TXT__
    log_openfile_text(log_fd_dsp_text,"mpeg4_text");
    if(s->pict_type == FF_P_TYPE)
        log_text(log_fd_dsp_text,"===================== P-VOP ====================\n");
    else if(s->pict_type == FF_B_TYPE)
        log_text(log_fd_dsp_text,"===================== B-VOP ====================\n");
    else if(s->pict_type == FF_I_TYPE) 
        log_text(log_fd_dsp_text,"===================== I-VOP ====================\n");
    else if(s->pict_type==FF_S_TYPE)
        log_text(log_fd_dsp_text,"===================== S-VOP ====================\n");
    else 
    {
        char txtt[30];
        snprintf(txtt,29,"unknown type=%d",s->pict_type);
        log_text(log_fd_dsp_text,txtt);
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

    ff_er_frame_start(s);

    //the second part of the wmv2 header contains the MB skip bits which are stored in current_picture->mb_type
    //which is not available before MPV_frame_start()
    if (CONFIG_WMV2_DECODER && s->msmpeg4_version==5){
        ret = ff_wmv2_decode_secondary_picture_header(s);
        if(ret<0) return ret;
        if(ret==1) goto intrax8_decoded;
    }

    /* decode each macroblock */
    s->mb_x=0;
    s->mb_y=0;

#if 0      
//#ifdef __dump_temp__
    char tempc[80];
    snprintf(tempc,79,"s->msmpeg4_version=%d,s->h263_msmpeg4=%d",s->msmpeg4_version,s->h263_msmpeg4);
    log_text(log_fd_temp, tempc);   
#endif

    decode_slice_nv12(s);
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
            if(ff_h263_resync_nv12(s)<0)
                break;
        }

        if(s->msmpeg4_version<4 && s->h263_pred)
            ff_mpeg4_clean_buffers_nv12(s);

        decode_slice_nv12(s);
    }

    if (s->h263_msmpeg4 && s->msmpeg4_version<4 && s->pict_type==FF_I_TYPE)
        if(!CONFIG_MSMPEG4_DECODER || msmpeg4_decode_ext_header(s, buf_size) < 0){
            s->error_status_table[s->mb_num-1]= AC_ERROR|DC_ERROR|MV_ERROR;
        }
    assert(s->bitstream_buffer_size==0);

frame_end:
    /* divx 5.01+ bistream reorder stuff */
    if(s->codec_id==CODEC_ID_AMBA_MPEG4 && s->divx_packed){
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
        if(s->gb.buffer == s->bitstream_buffer && buf_size>7 && s->xvid_build>=0){ //xvid style
            startcode_found=1;
            current_pos=0;
        }

        if(startcode_found){
            av_fast_malloc(
                &s->bitstream_buffer,
                &s->allocated_bitstream_buffer_size,
                buf_size - current_pos + FF_INPUT_BUFFER_PADDING_SIZE);
            if (!s->bitstream_buffer)
                return AVERROR(ENOMEM);
            memcpy(s->bitstream_buffer, buf + current_pos, buf_size - current_pos);
            s->bitstream_buffer_size= buf_size - current_pos;
        }
    }

intrax8_decoded:
    ff_er_frame_end_nv12(s);

    if (avctx->hwaccel) {
        if (avctx->hwaccel->end_frame(avctx) < 0)
            return -1;
    }

    MPV_frame_end_nv12(s);

assert(s->current_picture.pict_type == s->current_picture_ptr->pict_type);
assert(s->current_picture.pict_type == s->pict_type);
    if (s->pict_type == FF_B_TYPE || s->low_delay) {
        *pict= *(AVFrame*)s->current_picture_ptr;
    } else if (s->last_picture_ptr != NULL) {
        *pict= *(AVFrame*)s->last_picture_ptr;
    }

    if(s->last_picture_ptr || s->low_delay){
        *data_size = sizeof(AVFrame);
        ff_print_debug_info(s, pict);
    }
    
    //av_log(NULL,AV_LOG_ERROR,"pic type=%d.\n",s->current_picture_ptr->pict_type);

    #ifdef __log_dump_data__
    AVFrame* pdecpic=(AVFrame*)s->current_picture_ptr;
    if(pdecpic->data[0])
    {
        int ret=0;
        //av_log(NULL,AV_LOG_WARNING,"s->flags&CODEC_FLAG_EMU_EDGE=%d,avctx->width=%d,avctx->height=%d, pict->linesize[0]=%d,pict->linesize[1]=%d.\n",s->flags&CODEC_FLAG_EMU_EDGE,avctx->width,avctx->height,pdecpic->linesize[0],pdecpic->linesize[1]);

        if(s->flags&CODEC_FLAG_EMU_EDGE || !dump_config_extedge)
        {
            log_openfile(log_fd_frame_data,"frame_data_Y");
    //    av_log(NULL,AV_LOG_WARNING,"pict->data[0]=%p,pict->linesize[0]*avctx->height-16=%d.\n",pict->data[0],pict->linesize[0]*avctx->height-16);

            //dump with extended edge
            //log_dump(log_fd_frame_data,pdecpic->data[0]-16-pdecpic->linesize[0]*16,pdecpic->linesize[0]*(avctx->height+32));
            //dump only picture data
            int itt=0,jtt=0;uint8_t* ptt=pdecpic->data[0];
            for(itt=0;itt<avctx->height;itt++,ptt+=pdecpic->linesize[0])
                log_dump(log_fd_frame_data,ptt,avctx->width);
        
            log_closefile(log_fd_frame_data);

            int htmp=(avctx->height+1)>>1;
            int wtmp=(avctx->width+1)>>1;
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
            log_openfile(log_fd_frame_data,"frame_data_U");
            log_dump(log_fd_frame_data,ptmpu,htmp*wtmp);
            log_closefile(log_fd_frame_data);
            log_openfile(log_fd_frame_data,"frame_data_V");
            ret=log_dump(log_fd_frame_data,ptmpv,htmp*wtmp);
            log_closefile(log_fd_frame_data);
            av_free(ptmpu);
            av_free(ptmpv);
        }
        else
        { 
            log_openfile(log_fd_frame_data,"frame_data_Y");
    //    av_log(NULL,AV_LOG_WARNING,"pict->data[0]=%p,pict->linesize[0]*avctx->height-16=%d.\n",pict->data[0],pict->linesize[0]*avctx->height-16);

            //dump with extended edge
            log_dump(log_fd_frame_data,pdecpic->data[0]-16-pdecpic->linesize[0]*16,pdecpic->linesize[0]*(avctx->height+32));
            log_closefile(log_fd_frame_data);

            int htmp=((avctx->height+1)>>1)+16;
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
            log_openfile(log_fd_frame_data,"frame_data_U");        
            log_dump(log_fd_frame_data,ptmpu,htmp*wtmp);
            log_closefile(log_fd_frame_data);
    //    av_log(NULL,AV_LOG_WARNING,"end dump u, htmp=%d,wtmp=%d,ptmpu=%p.\n",htmp,wtmp,ptmpu);
            log_openfile(log_fd_frame_data,"frame_data_V");
            ret=log_dump(log_fd_frame_data,ptmpv,htmp*wtmp);
            log_closefile(log_fd_frame_data);
    //     av_log(NULL,AV_LOG_WARNING,"end dump v, htmp=%d,wtmp=%d. ret=%d. ptmpv=%p \n",htmp,wtmp,ret,ptmpv);       
            av_free(ptmpu);
            av_free(ptmpv);
    //    av_log(NULL,AV_LOG_WARNING,"end free.\n");   

        }
        
    }
    #endif

    #ifdef __dump_DSP_test_data__
    int dumpi=0,stride=0;
    int dumpwidth=0;
    char *psrc;
    #ifdef __dump_binary__
    
    //dump decoded result
    #ifdef __dump_DSP_result__
    char logfilename[50];
    AVFrame* presult;
    int dumpcnt=0;
    #ifdef __dump_DSP_result_with_display_order__
    static int display_cnt=0;
    if(s->pict_type == FF_B_TYPE)
        presult=s->current_picture_ptr;
    else
        presult=s->last_picture_ptr;
    dumpcnt=display_cnt;
    #else
    presult=s->current_picture_ptr;
    dumpcnt=log_cur_frame;
    #endif
    
    if(presult && log_start_frame<=dumpcnt && log_end_frame>=dumpcnt)
    {
        snprintf(logfilename,49,"ref_%d.y",log_cur_frame);
        log_openfile_f(log_fd_result, logfilename);
        psrc=presult->data[0];
        stride=presult->linesize[0];
        for(dumpi=0;dumpi<s->height;dumpi++,psrc+=stride)
        {
            log_dump_f(log_fd_result,psrc, s->width);
        }
        log_closefile(log_fd_result);

        snprintf(logfilename,49,"ref_%d.uv",log_cur_frame);
        log_openfile_f(log_fd_result, logfilename);
        psrc=presult->data[1];
        stride=presult->linesize[1];
        for(dumpi=0;dumpi<(s->height/2);dumpi++,psrc+=stride)
        {
            log_dump_f(log_fd_result,psrc, s->width);
        }
        log_closefile(log_fd_result);
        
        #ifdef __dump_DSP_result_with_display_order__
        display_cnt++;
        #endif
    }
    #endif
    
    #ifndef __dump_whole__

    //dump reference frames
    //forward reference
    if(s->last_picture_ptr)
    {
        log_openfile_p(log_fd_reffyraw, "mpeg4_fwref_y", log_cur_frame);
        psrc=s->last_picture_ptr->data[0];
        stride=s->last_picture_ptr->linesize[0];
        for(dumpi=0;dumpi<s->height;dumpi++,psrc+=stride)
        {
            log_dump_p(log_fd_reffyraw,psrc, s->width, log_cur_frame);
        }
        log_closefile_p(log_fd_reffyraw,log_cur_frame);

        log_openfile_p(log_fd_reffuvraw, "mpeg4_fwref_uv", log_cur_frame);
        psrc=s->last_picture_ptr->data[1];
        stride=s->last_picture_ptr->linesize[1];
        for(dumpi=0;dumpi<(s->height/2);dumpi++,psrc+=stride)
        {
            log_dump_p(log_fd_reffuvraw,psrc, s->width, log_cur_frame);
        }
        log_closefile_p(log_fd_reffuvraw,log_cur_frame);

        //extended
        log_openfile_p(log_fd_reffyraw, "mpeg4_fwref_y_ext", log_cur_frame);
        psrc=s->last_picture_ptr->data[0]-16-s->last_picture_ptr->linesize[0]*16;
        log_dump_p(log_fd_reffyraw,psrc, s->last_picture_ptr->linesize[0]*(s->height+32), log_cur_frame);
        log_closefile_p(log_fd_reffyraw,log_cur_frame);

        log_openfile_p(log_fd_reffuvraw, "mpeg4_fwref_uv_ext", log_cur_frame);
        psrc=s->last_picture_ptr->data[1]-16-s->last_picture_ptr->linesize[1]*8;
        log_dump_p(log_fd_reffuvraw,psrc, s->last_picture_ptr->linesize[1]*(s->height/2+16), log_cur_frame);
        log_closefile_p(log_fd_reffuvraw,log_cur_frame);
    }
    
    if(s->next_picture_ptr)
    {
        log_openfile_p(log_fd_refbyraw, "mpeg4_bwref_y", log_cur_frame);
        psrc=s->next_picture_ptr->data[0];
        stride=s->next_picture_ptr->linesize[0];
        for(dumpi=0;dumpi<s->height;dumpi++,psrc+=stride)
        {
            log_dump_p(log_fd_refbyraw,psrc, s->width, log_cur_frame);
        }
        log_closefile_p(log_fd_refbyraw,log_cur_frame);

        log_openfile_p(log_fd_refbuvraw, "mpeg4_bwref_uv",log_cur_frame);
        psrc=s->next_picture_ptr->data[1];
        stride=s->next_picture_ptr->linesize[1];
        for(dumpi=0;dumpi<(s->height/2);dumpi++,psrc+=stride)
        {
            log_dump_p(log_fd_refbuvraw,psrc, s->width, log_cur_frame);
        }
        log_closefile_p(log_fd_refbuvraw,log_cur_frame);

        //extended
        log_openfile_p(log_fd_refbyraw, "mpeg4_bwref_y_ext", log_cur_frame);
        psrc=s->next_picture_ptr->data[0]-16-s->next_picture_ptr->linesize[0]*16;
        log_dump_p(log_fd_refbyraw,psrc, s->next_picture_ptr->linesize[0]*(s->height+32), log_cur_frame);
        log_closefile_p(log_fd_refbyraw,log_cur_frame);

        log_openfile_p(log_fd_refbuvraw, "mpeg4_bwref_uv_ext", log_cur_frame);
        psrc=s->next_picture_ptr->data[1]-16-s->next_picture_ptr->linesize[1]*8;
        log_dump_p(log_fd_refbuvraw,psrc, s->next_picture_ptr->linesize[1]*(s->height/2+16), log_cur_frame);
        log_closefile_p(log_fd_refbuvraw,log_cur_frame);
    }

    
    //dump decoded frames

    if(s->current_picture_ptr)
    {
        log_openfile_p(log_fd_result, "mpeg4_result_y", log_cur_frame);
        psrc=s->current_picture_ptr->data[0];
        stride=s->current_picture_ptr->linesize[0];
        for(dumpi=0;dumpi<s->height;dumpi++,psrc+=stride)
        {
            log_dump_p(log_fd_result,psrc, s->width, log_cur_frame);
        }
        log_closefile_p(log_fd_result,log_cur_frame);

        log_openfile_p(log_fd_result, "mpeg4_result_uv", log_cur_frame);
        psrc=s->current_picture_ptr->data[1];
        stride=s->current_picture_ptr->linesize[1];
        for(dumpi=0;dumpi<(s->height/2);dumpi++,psrc+=stride)
        {
            log_dump_p(log_fd_result,psrc, s->width, log_cur_frame);
        }
        log_closefile_p(log_fd_result,log_cur_frame);

    }
    #endif
    #endif
    #endif

    

#ifdef PRINT_FRAME_TIME
av_log(avctx, AV_LOG_DEBUG, "%"PRId64"\n", rdtsc()-time);
#endif

#ifdef __dump_DSP_TXT__
    log_closefile(log_fd_dsp_text);
    #ifdef __dump_separate__
    log_closefile_p(log_fd_dsp_text+log_offset_vld,log_cur_frame);
    log_closefile_p(log_fd_dsp_text+log_offset_mc,log_cur_frame);
    log_closefile_p(log_fd_dsp_text+log_offset_dct,log_cur_frame);
    #endif
#endif

    log_cur_frame++;
    return get_consumed_bytes(s, buf_size);
}

AVCodec ff_mpeg4_nv12_decoder = {
    "mpeg4-nv12",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_MPEG4,
    sizeof(MpegEncContext),
    ff_h263_decode_init_nv12,
    NULL,
    ff_h263_decode_end_nv12,
    ff_h263_decode_frame_nv12,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | CODEC_CAP_DELAY,
    .flush= ff_mpeg_flush,
    .long_name= NULL_IF_CONFIG_SMALL("MPEG-4 part 2 (nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_h263_nv12_decoder = {
    "h263-nv12",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_H263,
    sizeof(MpegEncContext),
    ff_h263_decode_init_nv12,
    NULL,
    ff_h263_decode_end_nv12,
    ff_h263_decode_frame_nv12,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | CODEC_CAP_DELAY,
    .flush= ff_mpeg_flush,
    .long_name= NULL_IF_CONFIG_SMALL("H.263 / H.263-1996, H.263+ / H.263-1998 / H.263 version 2 (nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_msmpeg4v1_nv12_decoder = {
    "msmpeg4v1-nv12",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_MSMPEG4V1,
    sizeof(MpegEncContext),
    ff_h263_decode_init_nv12,
    NULL,
    ff_h263_decode_end_nv12,
    ff_h263_decode_frame_nv12,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name= NULL_IF_CONFIG_SMALL("MPEG-4 part 2 Microsoft variant version 1 (nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_msmpeg4v2_nv12_decoder = {
    "msmpeg4v2-nv12",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_MSMPEG4V2,
    sizeof(MpegEncContext),
    ff_h263_decode_init_nv12,
    NULL,
    ff_h263_decode_end_nv12,
    ff_h263_decode_frame_nv12,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name= NULL_IF_CONFIG_SMALL("MPEG-4 part 2 Microsoft variant version 2 (nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_msmpeg4v3_nv12_decoder = {
    "msmpeg4-nv12",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_MSMPEG4V3,
    sizeof(MpegEncContext),
    ff_h263_decode_init_nv12,
    NULL,
    ff_h263_decode_end_nv12,
    ff_h263_decode_frame_nv12,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name= NULL_IF_CONFIG_SMALL("MPEG-4 part 2 Microsoft variant version 3 (nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_wmv1_nv12_decoder = {
    "wmv1-nv12",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_WMV1,
    sizeof(MpegEncContext),
    ff_h263_decode_init_nv12,
    NULL,
    ff_h263_decode_end_nv12,
    ff_h263_decode_frame_nv12,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name= NULL_IF_CONFIG_SMALL("Windows Media Video 7 (nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_h263i_nv12_decoder = {
    "h263i-nv12",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_H263I,
    sizeof(MpegEncContext),
    ff_h263_decode_init_nv12,
    NULL,
    ff_h263_decode_end_nv12,
    ff_h263_decode_frame_nv12,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("Intel H.263 (nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};

AVCodec ff_flv_nv12_decoder = {
    "flv-nv12",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_AMBA_FLV1,
    sizeof(MpegEncContext),
    ff_h263_decode_init_nv12,
    NULL,
    ff_h263_decode_end_nv12,
    ff_h263_decode_frame_nv12,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1,
    .long_name= NULL_IF_CONFIG_SMALL("Flash Video (FLV) (nv12)"),
    .pix_fmts= ff_pixfmt_list_nv12,
};
