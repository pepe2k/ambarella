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

#ifndef AVCODEC_RV12_WMV2_H
#define AVCODEC_RV12_WMV2_H

#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "intrax8.h"
#include "amba_h263.h"

#define SKIP_TYPE_NONE 0
#define SKIP_TYPE_MPEG 1
#define SKIP_TYPE_ROW  2
#define SKIP_TYPE_COL  3

typedef struct wmv2_pic_data_s
{
    //mv
    void* pbase;
    mb_mv_B_t* pmvb;
    mb_mv_P_t* pmvp;
        
    //idct coef/result
    //y 4x8x8,cb 8x8,cr 8x8 
    void* pdctbase;
    short* pdct;// 32 byte align

    //vop info
    udec_decode_t vopinfo;
        
    //deblock 
    
    //picture information
    int width, height;///< picture size. must be a multiple of 16
    int mb_width, mb_height;   ///< number of MBs horizontally & vertically
    int mb_stride;
    
    Picture *last_picture_ptr;     ///< pointer to the previous picture.
    Picture *next_picture_ptr;     ///< pointer to the next picture (for bidir pred)
    Picture *current_picture_ptr;  ///< pointer to the current picture

    //from s
//    int f_code,b_code;//wmv2 no f_code, b_code
    int no_rounding;  
//    int progressive_sequence;//wmv2 no interlace mode
    
//    int quarter_sample;//wmv2 no quarter pixel
//    int mpeg_quant; //wmv2 no customized quant matrix
//    int alternate_scan;
    int h_edge_pos, v_edge_pos;///< horizontal / vertical position of the right/bottom edge (pixel replication)

/*
    //sprite related
    int sprite_warping_accuracy;
    int vol_sprite_usage;
    int sprite_width;
    int sprite_height;
    int sprite_left;
    int sprite_top;
    int sprite_brightness_change;
    int num_sprite_warping_points;
    int real_sprite_warping_points;
    uint16_t sprite_traj[4][2];      ///< sprite trajectory points
    int sprite_offset[2][2];         ///< sprite offset[isChroma][isMVY]
    int sprite_delta[2][2];          ///< sprite_delta [isY][isMVY]
    int sprite_shift[2];             ///< sprite shift [isChroma]*/

    //sw decoding needed info
    h263_pic_swinfo_t* pswinfo; 
    
    //used for using dsp->not using dsp or vice vesa
    int use_dsp;//sw:0, amba Ione: 1, 
    int use_permutated;//use permutated matrix
    
//debug use
    int frame_cnt;
}wmv2_pic_data_t;


typedef struct Wmv2Context_amba{
    MpegEncContext s;
    IntraX8Context x8;
    int j_type_bit;
    int j_type;
    int abt_flag;
    int abt_type;
    int abt_type_table[6];
    int per_mb_abt;
    int per_block_abt;
    int mspel_bit;
    int cbp_table_index;
    int top_left_mv_flag;
    int per_mb_rl_bit;
    int skip_type;
    int hshift;

    ScanTable abt_scantable[2];
    DECLARE_ALIGNED(16, DCTELEM, abt_block2)[6][64];

    //use dsp?
    int use_dsp;//sw:0, amba Ione: 1, 
    int use_permutated;
    //parallel related
    
    //config
    int parallel_method;// 1: use way 1, 2: use way 2
//    int use_dsp;

    //thread control 
    pthread_t tid_vld,tid_mc_idct,tid_idct,tid_mc,tid_deblock;
    int vld_loop, mc_idct_loop, idct_loop, mc_loop, deblock_loop;
    
    //communicate queue
    ambadec_triqueue_t* p_vld_dataq;
    //way 1 vld->mc_idct
    ambadec_triqueue_t* p_mc_idct_dataq;
    //way 2 vld->idct->mc
    ambadec_triqueue_t* p_idct_dataq;
    ambadec_triqueue_t* p_mc_dataq;
    
    ambadec_triqueue_t* p_deblock_dataq;//optional
    ambadec_triqueue_t* p_frame_pool;//output pool

    //data queue
    ambadec_triqueue_t* p_pic_dataq;
    int pic_finished;//current picture vld decoding success, initial value 1; 
//    int vld_sent_row,mc_sent_row;
//    int previous_pic_is_b;//previous picture is BVOP(not reference), initial value 1;
    ambadec_pool_t* pic_pool;//parallel decoder used picture pool
    wmv2_pic_data_t  picdata[2];

    //store state and indicate next to do..
    int need_restart_pipeline; //should restart
    int pipe_line_started; //pipe line status

    wmv2_pic_data_t* p_pic;

    pthread_mutex_t mutex;
    unsigned long  decoding_frame_cnt;//already in decoding frames
}Wmv2Context_amba;

void ff_wmv2_common_init_amba(Wmv2Context_amba * w);
//only transform 
void wmv2_tran_block(Wmv2Context_amba*w, DCTELEM *block1,int n);

#endif /* AVCODEC_WMV2_H */

