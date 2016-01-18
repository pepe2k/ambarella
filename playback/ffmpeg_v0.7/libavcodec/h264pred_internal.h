/*
 * H.26L/H.264/AVC/JVT/14496-10/... encoder/decoder
 * Copyright (c) 2003-2011 Michael Niedermayer <michaelni@gmx.at>
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
 * H.264 / AVC / MPEG4 part10 prediction functions.
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include "mathops.h"
#include "h264_high_depth.h"

static void FUNCC(pred4x4_vertical)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    const pixel4 a= ((pixel4*)(src-stride))[0];
    ((pixel4*)(src+0*stride))[0]= a;
    ((pixel4*)(src+1*stride))[0]= a;
    ((pixel4*)(src+2*stride))[0]= a;
    ((pixel4*)(src+3*stride))[0]= a;
}

static void FUNCC(pred4x4_horizontal)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    ((pixel4*)(src+0*stride))[0]= PIXEL_SPLAT_X4(src[-1+0*stride]);
    ((pixel4*)(src+1*stride))[0]= PIXEL_SPLAT_X4(src[-1+1*stride]);
    ((pixel4*)(src+2*stride))[0]= PIXEL_SPLAT_X4(src[-1+2*stride]);
    ((pixel4*)(src+3*stride))[0]= PIXEL_SPLAT_X4(src[-1+3*stride]);
}

static void FUNCC(pred4x4_dc)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    const int dc= (  src[-stride] + src[1-stride] + src[2-stride] + src[3-stride]
                   + src[-1+0*stride] + src[-1+1*stride] + src[-1+2*stride] + src[-1+3*stride] + 4) >>3;

    ((pixel4*)(src+0*stride))[0]=
    ((pixel4*)(src+1*stride))[0]=
    ((pixel4*)(src+2*stride))[0]=
    ((pixel4*)(src+3*stride))[0]= PIXEL_SPLAT_X4(dc);
}

static void FUNCC(pred4x4_left_dc)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    const int dc= (  src[-1+0*stride] + src[-1+1*stride] + src[-1+2*stride] + src[-1+3*stride] + 2) >>2;

    ((pixel4*)(src+0*stride))[0]=
    ((pixel4*)(src+1*stride))[0]=
    ((pixel4*)(src+2*stride))[0]=
    ((pixel4*)(src+3*stride))[0]= PIXEL_SPLAT_X4(dc);
}

static void FUNCC(pred4x4_top_dc)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    const int dc= (  src[-stride] + src[1-stride] + src[2-stride] + src[3-stride] + 2) >>2;

    ((pixel4*)(src+0*stride))[0]=
    ((pixel4*)(src+1*stride))[0]=
    ((pixel4*)(src+2*stride))[0]=
    ((pixel4*)(src+3*stride))[0]= PIXEL_SPLAT_X4(dc);
}

static void FUNCC(pred4x4_128_dc)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    ((pixel4*)(src+0*stride))[0]=
    ((pixel4*)(src+1*stride))[0]=
    ((pixel4*)(src+2*stride))[0]=
    ((pixel4*)(src+3*stride))[0]= PIXEL_SPLAT_X4(1<<(BIT_DEPTH-1));
}

static void FUNCC(pred4x4_127_dc)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    ((pixel4*)(src+0*stride))[0]=
    ((pixel4*)(src+1*stride))[0]=
    ((pixel4*)(src+2*stride))[0]=
    ((pixel4*)(src+3*stride))[0]= PIXEL_SPLAT_X4((1<<(BIT_DEPTH-1))-1);
}

static void FUNCC(pred4x4_129_dc)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    ((pixel4*)(src+0*stride))[0]=
    ((pixel4*)(src+1*stride))[0]=
    ((pixel4*)(src+2*stride))[0]=
    ((pixel4*)(src+3*stride))[0]= PIXEL_SPLAT_X4((1<<(BIT_DEPTH-1))+1);
}


#define LOAD_TOP_RIGHT_EDGE\
    const int av_unused t4= topright[0];\
    const int av_unused t5= topright[1];\
    const int av_unused t6= topright[2];\
    const int av_unused t7= topright[3];\

#define LOAD_DOWN_LEFT_EDGE\
    const int av_unused l4= src[-1+4*stride];\
    const int av_unused l5= src[-1+5*stride];\
    const int av_unused l6= src[-1+6*stride];\
    const int av_unused l7= src[-1+7*stride];\

#define LOAD_LEFT_EDGE\
    const int av_unused l0= src[-1+0*stride];\
    const int av_unused l1= src[-1+1*stride];\
    const int av_unused l2= src[-1+2*stride];\
    const int av_unused l3= src[-1+3*stride];\

#define LOAD_TOP_EDGE\
    const int av_unused t0= src[ 0-1*stride];\
    const int av_unused t1= src[ 1-1*stride];\
    const int av_unused t2= src[ 2-1*stride];\
    const int av_unused t3= src[ 3-1*stride];\

static void FUNCC(pred4x4_vertical_vp8)(uint8_t *p_src, const uint8_t *p_topright, int p_stride){
    pixel *src = (pixel*)p_src;
    const pixel *topright = (const pixel*)p_topright;
    int stride = p_stride>>(sizeof(pixel)-1);
    const int lt= src[-1-1*stride];
    LOAD_TOP_EDGE
    LOAD_TOP_RIGHT_EDGE
    pixel4 v = PACK_4U8((lt + 2*t0 + t1 + 2) >> 2,
                            (t0 + 2*t1 + t2 + 2) >> 2,
                            (t1 + 2*t2 + t3 + 2) >> 2,
                            (t2 + 2*t3 + t4 + 2) >> 2);

    AV_WN4PA(src+0*stride, v);
    AV_WN4PA(src+1*stride, v);
    AV_WN4PA(src+2*stride, v);
    AV_WN4PA(src+3*stride, v);
}

static void FUNCC(pred4x4_horizontal_vp8)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    const int lt= src[-1-1*stride];
    LOAD_LEFT_EDGE

    AV_WN4PA(src+0*stride, PIXEL_SPLAT_X4((lt + 2*l0 + l1 + 2) >> 2));
    AV_WN4PA(src+1*stride, PIXEL_SPLAT_X4((l0 + 2*l1 + l2 + 2) >> 2));
    AV_WN4PA(src+2*stride, PIXEL_SPLAT_X4((l1 + 2*l2 + l3 + 2) >> 2));
    AV_WN4PA(src+3*stride, PIXEL_SPLAT_X4((l2 + 2*l3 + l3 + 2) >> 2));
}

static void FUNCC(pred4x4_down_right)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    const int lt= src[-1-1*stride];
    LOAD_TOP_EDGE
    LOAD_LEFT_EDGE

    src[0+3*stride]=(l3 + 2*l2 + l1 + 2)>>2;
    src[0+2*stride]=
    src[1+3*stride]=(l2 + 2*l1 + l0 + 2)>>2;
    src[0+1*stride]=
    src[1+2*stride]=
    src[2+3*stride]=(l1 + 2*l0 + lt + 2)>>2;
    src[0+0*stride]=
    src[1+1*stride]=
    src[2+2*stride]=
    src[3+3*stride]=(l0 + 2*lt + t0 + 2)>>2;
    src[1+0*stride]=
    src[2+1*stride]=
    src[3+2*stride]=(lt + 2*t0 + t1 + 2)>>2;
    src[2+0*stride]=
    src[3+1*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[3+0*stride]=(t1 + 2*t2 + t3 + 2)>>2;
}

static void FUNCC(pred4x4_down_left)(uint8_t *p_src, const uint8_t *p_topright, int p_stride){
    pixel *src = (pixel*)p_src;
    const pixel *topright = (const pixel*)p_topright;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_TOP_EDGE
    LOAD_TOP_RIGHT_EDGE
//    LOAD_LEFT_EDGE

    src[0+0*stride]=(t0 + t2 + 2*t1 + 2)>>2;
    src[1+0*stride]=
    src[0+1*stride]=(t1 + t3 + 2*t2 + 2)>>2;
    src[2+0*stride]=
    src[1+1*stride]=
    src[0+2*stride]=(t2 + t4 + 2*t3 + 2)>>2;
    src[3+0*stride]=
    src[2+1*stride]=
    src[1+2*stride]=
    src[0+3*stride]=(t3 + t5 + 2*t4 + 2)>>2;
    src[3+1*stride]=
    src[2+2*stride]=
    src[1+3*stride]=(t4 + t6 + 2*t5 + 2)>>2;
    src[3+2*stride]=
    src[2+3*stride]=(t5 + t7 + 2*t6 + 2)>>2;
    src[3+3*stride]=(t6 + 3*t7 + 2)>>2;
}

static void FUNCC(pred4x4_down_left_svq3)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_TOP_EDGE
    LOAD_LEFT_EDGE
    const av_unused int unu0= t0;
    const av_unused int unu1= l0;

    src[0+0*stride]=(l1 + t1)>>1;
    src[1+0*stride]=
    src[0+1*stride]=(l2 + t2)>>1;
    src[2+0*stride]=
    src[1+1*stride]=
    src[0+2*stride]=
    src[3+0*stride]=
    src[2+1*stride]=
    src[1+2*stride]=
    src[0+3*stride]=
    src[3+1*stride]=
    src[2+2*stride]=
    src[1+3*stride]=
    src[3+2*stride]=
    src[2+3*stride]=
    src[3+3*stride]=(l3 + t3)>>1;
}

static void FUNCC(pred4x4_down_left_rv40)(uint8_t *p_src, const uint8_t *p_topright, int p_stride){
    pixel *src = (pixel*)p_src;
    const pixel *topright = (const pixel*)p_topright;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_TOP_EDGE
    LOAD_TOP_RIGHT_EDGE
    LOAD_LEFT_EDGE
    LOAD_DOWN_LEFT_EDGE

    src[0+0*stride]=(t0 + t2 + 2*t1 + 2 + l0 + l2 + 2*l1 + 2)>>3;
    src[1+0*stride]=
    src[0+1*stride]=(t1 + t3 + 2*t2 + 2 + l1 + l3 + 2*l2 + 2)>>3;
    src[2+0*stride]=
    src[1+1*stride]=
    src[0+2*stride]=(t2 + t4 + 2*t3 + 2 + l2 + l4 + 2*l3 + 2)>>3;
    src[3+0*stride]=
    src[2+1*stride]=
    src[1+2*stride]=
    src[0+3*stride]=(t3 + t5 + 2*t4 + 2 + l3 + l5 + 2*l4 + 2)>>3;
    src[3+1*stride]=
    src[2+2*stride]=
    src[1+3*stride]=(t4 + t6 + 2*t5 + 2 + l4 + l6 + 2*l5 + 2)>>3;
    src[3+2*stride]=
    src[2+3*stride]=(t5 + t7 + 2*t6 + 2 + l5 + l7 + 2*l6 + 2)>>3;
    src[3+3*stride]=(t6 + t7 + 1 + l6 + l7 + 1)>>2;
}

static void FUNCC(pred4x4_down_left_rv40_nodown)(uint8_t *p_src, const uint8_t *p_topright, int p_stride){
    pixel *src = (pixel*)p_src;
    const pixel *topright = (const pixel*)p_topright;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_TOP_EDGE
    LOAD_TOP_RIGHT_EDGE
    LOAD_LEFT_EDGE

    src[0+0*stride]=(t0 + t2 + 2*t1 + 2 + l0 + l2 + 2*l1 + 2)>>3;
    src[1+0*stride]=
    src[0+1*stride]=(t1 + t3 + 2*t2 + 2 + l1 + l3 + 2*l2 + 2)>>3;
    src[2+0*stride]=
    src[1+1*stride]=
    src[0+2*stride]=(t2 + t4 + 2*t3 + 2 + l2 + 3*l3 + 2)>>3;
    src[3+0*stride]=
    src[2+1*stride]=
    src[1+2*stride]=
    src[0+3*stride]=(t3 + t5 + 2*t4 + 2 + l3*4 + 2)>>3;
    src[3+1*stride]=
    src[2+2*stride]=
    src[1+3*stride]=(t4 + t6 + 2*t5 + 2 + l3*4 + 2)>>3;
    src[3+2*stride]=
    src[2+3*stride]=(t5 + t7 + 2*t6 + 2 + l3*4 + 2)>>3;
    src[3+3*stride]=(t6 + t7 + 1 + 2*l3 + 1)>>2;
}

static void FUNCC(pred4x4_vertical_right)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    const int lt= src[-1-1*stride];
    LOAD_TOP_EDGE
    LOAD_LEFT_EDGE

    src[0+0*stride]=
    src[1+2*stride]=(lt + t0 + 1)>>1;
    src[1+0*stride]=
    src[2+2*stride]=(t0 + t1 + 1)>>1;
    src[2+0*stride]=
    src[3+2*stride]=(t1 + t2 + 1)>>1;
    src[3+0*stride]=(t2 + t3 + 1)>>1;
    src[0+1*stride]=
    src[1+3*stride]=(l0 + 2*lt + t0 + 2)>>2;
    src[1+1*stride]=
    src[2+3*stride]=(lt + 2*t0 + t1 + 2)>>2;
    src[2+1*stride]=
    src[3+3*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[3+1*stride]=(t1 + 2*t2 + t3 + 2)>>2;
    src[0+2*stride]=(lt + 2*l0 + l1 + 2)>>2;
    src[0+3*stride]=(l0 + 2*l1 + l2 + 2)>>2;
}

static void FUNCC(pred4x4_vertical_left)(uint8_t *p_src, const uint8_t *p_topright, int p_stride){
    pixel *src = (pixel*)p_src;
    const pixel *topright = (const pixel*)p_topright;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_TOP_EDGE
    LOAD_TOP_RIGHT_EDGE

    src[0+0*stride]=(t0 + t1 + 1)>>1;
    src[1+0*stride]=
    src[0+2*stride]=(t1 + t2 + 1)>>1;
    src[2+0*stride]=
    src[1+2*stride]=(t2 + t3 + 1)>>1;
    src[3+0*stride]=
    src[2+2*stride]=(t3 + t4+ 1)>>1;
    src[3+2*stride]=(t4 + t5+ 1)>>1;
    src[0+1*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[1+1*stride]=
    src[0+3*stride]=(t1 + 2*t2 + t3 + 2)>>2;
    src[2+1*stride]=
    src[1+3*stride]=(t2 + 2*t3 + t4 + 2)>>2;
    src[3+1*stride]=
    src[2+3*stride]=(t3 + 2*t4 + t5 + 2)>>2;
    src[3+3*stride]=(t4 + 2*t5 + t6 + 2)>>2;
}

static void FUNCC(pred4x4_vertical_left_rv40_internal)(uint8_t *p_src, const uint8_t *p_topright, int p_stride,
                                      const int l0, const int l1, const int l2, const int l3, const int l4){
    pixel *src = (pixel*)p_src;
    const pixel *topright = (const pixel*)p_topright;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_TOP_EDGE
    LOAD_TOP_RIGHT_EDGE

    src[0+0*stride]=(2*t0 + 2*t1 + l1 + 2*l2 + l3 + 4)>>3;
    src[1+0*stride]=
    src[0+2*stride]=(t1 + t2 + 1)>>1;
    src[2+0*stride]=
    src[1+2*stride]=(t2 + t3 + 1)>>1;
    src[3+0*stride]=
    src[2+2*stride]=(t3 + t4+ 1)>>1;
    src[3+2*stride]=(t4 + t5+ 1)>>1;
    src[0+1*stride]=(t0 + 2*t1 + t2 + l2 + 2*l3 + l4 + 4)>>3;
    src[1+1*stride]=
    src[0+3*stride]=(t1 + 2*t2 + t3 + 2)>>2;
    src[2+1*stride]=
    src[1+3*stride]=(t2 + 2*t3 + t4 + 2)>>2;
    src[3+1*stride]=
    src[2+3*stride]=(t3 + 2*t4 + t5 + 2)>>2;
    src[3+3*stride]=(t4 + 2*t5 + t6 + 2)>>2;
}

static void FUNCC(pred4x4_vertical_left_rv40)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_LEFT_EDGE
    LOAD_DOWN_LEFT_EDGE

    FUNCC(pred4x4_vertical_left_rv40_internal)(p_src, topright, p_stride, l0, l1, l2, l3, l4);
}

static void FUNCC(pred4x4_vertical_left_rv40_nodown)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_LEFT_EDGE

    FUNCC(pred4x4_vertical_left_rv40_internal)(p_src, topright, p_stride, l0, l1, l2, l3, l3);
}

static void FUNCC(pred4x4_vertical_left_vp8)(uint8_t *p_src, const uint8_t *p_topright, int p_stride){
    pixel *src = (pixel*)p_src;
    const pixel *topright = (const pixel*)p_topright;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_TOP_EDGE
    LOAD_TOP_RIGHT_EDGE

    src[0+0*stride]=(t0 + t1 + 1)>>1;
    src[1+0*stride]=
    src[0+2*stride]=(t1 + t2 + 1)>>1;
    src[2+0*stride]=
    src[1+2*stride]=(t2 + t3 + 1)>>1;
    src[3+0*stride]=
    src[2+2*stride]=(t3 + t4 + 1)>>1;
    src[0+1*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[1+1*stride]=
    src[0+3*stride]=(t1 + 2*t2 + t3 + 2)>>2;
    src[2+1*stride]=
    src[1+3*stride]=(t2 + 2*t3 + t4 + 2)>>2;
    src[3+1*stride]=
    src[2+3*stride]=(t3 + 2*t4 + t5 + 2)>>2;
    src[3+2*stride]=(t4 + 2*t5 + t6 + 2)>>2;
    src[3+3*stride]=(t5 + 2*t6 + t7 + 2)>>2;
}

static void FUNCC(pred4x4_horizontal_up)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_LEFT_EDGE

    src[0+0*stride]=(l0 + l1 + 1)>>1;
    src[1+0*stride]=(l0 + 2*l1 + l2 + 2)>>2;
    src[2+0*stride]=
    src[0+1*stride]=(l1 + l2 + 1)>>1;
    src[3+0*stride]=
    src[1+1*stride]=(l1 + 2*l2 + l3 + 2)>>2;
    src[2+1*stride]=
    src[0+2*stride]=(l2 + l3 + 1)>>1;
    src[3+1*stride]=
    src[1+2*stride]=(l2 + 2*l3 + l3 + 2)>>2;
    src[3+2*stride]=
    src[1+3*stride]=
    src[0+3*stride]=
    src[2+2*stride]=
    src[2+3*stride]=
    src[3+3*stride]=l3;
}

static void FUNCC(pred4x4_horizontal_up_rv40)(uint8_t *p_src, const uint8_t *p_topright, int p_stride){
    pixel *src = (pixel*)p_src;
    const pixel *topright = (const pixel*)p_topright;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_LEFT_EDGE
    LOAD_DOWN_LEFT_EDGE
    LOAD_TOP_EDGE
    LOAD_TOP_RIGHT_EDGE

    src[0+0*stride]=(t1 + 2*t2 + t3 + 2*l0 + 2*l1 + 4)>>3;
    src[1+0*stride]=(t2 + 2*t3 + t4 + l0 + 2*l1 + l2 + 4)>>3;
    src[2+0*stride]=
    src[0+1*stride]=(t3 + 2*t4 + t5 + 2*l1 + 2*l2 + 4)>>3;
    src[3+0*stride]=
    src[1+1*stride]=(t4 + 2*t5 + t6 + l1 + 2*l2 + l3 + 4)>>3;
    src[2+1*stride]=
    src[0+2*stride]=(t5 + 2*t6 + t7 + 2*l2 + 2*l3 + 4)>>3;
    src[3+1*stride]=
    src[1+2*stride]=(t6 + 3*t7 + l2 + 3*l3 + 4)>>3;
    src[3+2*stride]=
    src[1+3*stride]=(l3 + 2*l4 + l5 + 2)>>2;
    src[0+3*stride]=
    src[2+2*stride]=(t6 + t7 + l3 + l4 + 2)>>2;
    src[2+3*stride]=(l4 + l5 + 1)>>1;
    src[3+3*stride]=(l4 + 2*l5 + l6 + 2)>>2;
}

static void FUNCC(pred4x4_horizontal_up_rv40_nodown)(uint8_t *p_src, const uint8_t *p_topright, int p_stride){
    pixel *src = (pixel*)p_src;
    const pixel *topright = (const pixel*)p_topright;
    int stride = p_stride>>(sizeof(pixel)-1);
    LOAD_LEFT_EDGE
    LOAD_TOP_EDGE
    LOAD_TOP_RIGHT_EDGE

    src[0+0*stride]=(t1 + 2*t2 + t3 + 2*l0 + 2*l1 + 4)>>3;
    src[1+0*stride]=(t2 + 2*t3 + t4 + l0 + 2*l1 + l2 + 4)>>3;
    src[2+0*stride]=
    src[0+1*stride]=(t3 + 2*t4 + t5 + 2*l1 + 2*l2 + 4)>>3;
    src[3+0*stride]=
    src[1+1*stride]=(t4 + 2*t5 + t6 + l1 + 2*l2 + l3 + 4)>>3;
    src[2+1*stride]=
    src[0+2*stride]=(t5 + 2*t6 + t7 + 2*l2 + 2*l3 + 4)>>3;
    src[3+1*stride]=
    src[1+2*stride]=(t6 + 3*t7 + l2 + 3*l3 + 4)>>3;
    src[3+2*stride]=
    src[1+3*stride]=l3;
    src[0+3*stride]=
    src[2+2*stride]=(t6 + t7 + 2*l3 + 2)>>2;
    src[2+3*stride]=
    src[3+3*stride]=l3;
}

static void FUNCC(pred4x4_horizontal_down)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    const int lt= src[-1-1*stride];
    LOAD_TOP_EDGE
    LOAD_LEFT_EDGE

    src[0+0*stride]=
    src[2+1*stride]=(lt + l0 + 1)>>1;
    src[1+0*stride]=
    src[3+1*stride]=(l0 + 2*lt + t0 + 2)>>2;
    src[2+0*stride]=(lt + 2*t0 + t1 + 2)>>2;
    src[3+0*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[0+1*stride]=
    src[2+2*stride]=(l0 + l1 + 1)>>1;
    src[1+1*stride]=
    src[3+2*stride]=(lt + 2*l0 + l1 + 2)>>2;
    src[0+2*stride]=
    src[2+3*stride]=(l1 + l2+ 1)>>1;
    src[1+2*stride]=
    src[3+3*stride]=(l0 + 2*l1 + l2 + 2)>>2;
    src[0+3*stride]=(l2 + l3 + 1)>>1;
    src[1+3*stride]=(l1 + 2*l2 + l3 + 2)>>2;
}

static void FUNCC(pred4x4_tm_vp8)(uint8_t *p_src, const uint8_t *topright, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP - src[-1-stride];
    pixel *top = src-stride;
    int y;

    for (y = 0; y < 4; y++) {
        uint8_t *cm_in = cm + src[-1];
        src[0] = cm_in[top[0]];
        src[1] = cm_in[top[1]];
        src[2] = cm_in[top[2]];
        src[3] = cm_in[top[3]];
        src += stride;
    }
}

static void FUNCC(pred16x16_vertical)(uint8_t *p_src, int p_stride){
    int i;
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    const pixel4 a = ((pixel4*)(src-stride))[0];
    const pixel4 b = ((pixel4*)(src-stride))[1];
    const pixel4 c = ((pixel4*)(src-stride))[2];
    const pixel4 d = ((pixel4*)(src-stride))[3];

    for(i=0; i<16; i++){
        ((pixel4*)(src+i*stride))[0] = a;
        ((pixel4*)(src+i*stride))[1] = b;
        ((pixel4*)(src+i*stride))[2] = c;
        ((pixel4*)(src+i*stride))[3] = d;
    }
}

static void FUNCC(pred16x16_horizontal)(uint8_t *p_src, int stride){
    int i;
    pixel *src = (pixel*)p_src;
    stride >>= sizeof(pixel)-1;

    for(i=0; i<16; i++){
        ((pixel4*)(src+i*stride))[0] =
        ((pixel4*)(src+i*stride))[1] =
        ((pixel4*)(src+i*stride))[2] =
        ((pixel4*)(src+i*stride))[3] = PIXEL_SPLAT_X4(src[-1+i*stride]);
    }
}

#define PREDICT_16x16_DC(v)\
    for(i=0; i<16; i++){\
        AV_WN4P(src+ 0, v);\
        AV_WN4P(src+ 4, v);\
        AV_WN4P(src+ 8, v);\
        AV_WN4P(src+12, v);\
        src += stride;\
    }

static void FUNCC(pred16x16_dc)(uint8_t *p_src, int stride){
    int i, dc=0;
    pixel *src = (pixel*)p_src;
    pixel4 dcsplat;
    stride >>= sizeof(pixel)-1;

    for(i=0;i<16; i++){
        dc+= src[-1+i*stride];
    }

    for(i=0;i<16; i++){
        dc+= src[i-stride];
    }

    dcsplat = PIXEL_SPLAT_X4((dc+16)>>5);
    PREDICT_16x16_DC(dcsplat);
}

static void FUNCC(pred16x16_left_dc)(uint8_t *p_src, int stride){
    int i, dc=0;
    pixel *src = (pixel*)p_src;
    pixel4 dcsplat;
    stride >>= sizeof(pixel)-1;

    for(i=0;i<16; i++){
        dc+= src[-1+i*stride];
    }

    dcsplat = PIXEL_SPLAT_X4((dc+8)>>4);
    PREDICT_16x16_DC(dcsplat);
}

static void FUNCC(pred16x16_top_dc)(uint8_t *p_src, int stride){
    int i, dc=0;
    pixel *src = (pixel*)p_src;
    pixel4 dcsplat;
    stride >>= sizeof(pixel)-1;

    for(i=0;i<16; i++){
        dc+= src[i-stride];
    }

    dcsplat = PIXEL_SPLAT_X4((dc+8)>>4);
    PREDICT_16x16_DC(dcsplat);
}

#define PRED16x16_X(n, v) \
static void FUNCC(pred16x16_##n##_dc)(uint8_t *p_src, int stride){\
    int i;\
    pixel *src = (pixel*)p_src;\
    stride >>= sizeof(pixel)-1;\
    PREDICT_16x16_DC(PIXEL_SPLAT_X4(v));\
}

PRED16x16_X(127, (1<<(BIT_DEPTH-1))-1);
PRED16x16_X(128, (1<<(BIT_DEPTH-1))+0);
PRED16x16_X(129, (1<<(BIT_DEPTH-1))+1);

static inline void FUNCC(pred16x16_plane_compat)(uint8_t *p_src, int p_stride, const int svq3, const int rv40){
  int i, j, k;
  int a;
  INIT_CLIP
  pixel *src = (pixel*)p_src;
  int stride = p_stride>>(sizeof(pixel)-1);
  const pixel * const src0 = src +7-stride;
  const pixel *       src1 = src +8*stride-1;
  const pixel *       src2 = src1-2*stride;    // == src+6*stride-1;
  int H = src0[1] - src0[-1];
  int V = src1[0] - src2[ 0];
  for(k=2; k<=8; ++k) {
    src1 += stride; src2 -= stride;
    H += k*(src0[k] - src0[-k]);
    V += k*(src1[0] - src2[ 0]);
  }
  if(svq3){
    H = ( 5*(H/4) ) / 16;
    V = ( 5*(V/4) ) / 16;

    /* required for 100% accuracy */
    i = H; H = V; V = i;
  }else if(rv40){
    H = ( H + (H>>2) ) >> 4;
    V = ( V + (V>>2) ) >> 4;
  }else{
    H = ( 5*H+32 ) >> 6;
    V = ( 5*V+32 ) >> 6;
  }

  a = 16*(src1[0] + src2[16] + 1) - 7*(V+H);
  for(j=16; j>0; --j) {
    int b = a;
    a += V;
    for(i=-16; i<0; i+=4) {
      src[16+i] = CLIP((b    ) >> 5);
      src[17+i] = CLIP((b+  H) >> 5);
      src[18+i] = CLIP((b+2*H) >> 5);
      src[19+i] = CLIP((b+3*H) >> 5);
      b += 4*H;
    }
    src += stride;
  }
}

static void FUNCC(pred16x16_plane)(uint8_t *src, int stride){
    FUNCC(pred16x16_plane_compat)(src, stride, 0, 0);
}

static void FUNCC(pred16x16_plane_svq3)(uint8_t *src, int stride){
    FUNCC(pred16x16_plane_compat)(src, stride, 1, 0);
}

static void FUNCC(pred16x16_plane_rv40)(uint8_t *src, int stride){
    FUNCC(pred16x16_plane_compat)(src, stride, 0, 1);
}

static void FUNCC(pred16x16_tm_vp8)(uint8_t *src, int stride){
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP - src[-1-stride];
    uint8_t *top = src-stride;
    int y;

    for (y = 0; y < 16; y++) {
        uint8_t *cm_in = cm + src[-1];
        src[0]  = cm_in[top[0]];
        src[1]  = cm_in[top[1]];
        src[2]  = cm_in[top[2]];
        src[3]  = cm_in[top[3]];
        src[4]  = cm_in[top[4]];
        src[5]  = cm_in[top[5]];
        src[6]  = cm_in[top[6]];
        src[7]  = cm_in[top[7]];
        src[8]  = cm_in[top[8]];
        src[9]  = cm_in[top[9]];
        src[10] = cm_in[top[10]];
        src[11] = cm_in[top[11]];
        src[12] = cm_in[top[12]];
        src[13] = cm_in[top[13]];
        src[14] = cm_in[top[14]];
        src[15] = cm_in[top[15]];
        src += stride;
    }
}

static void FUNCC(pred8x8_vertical)(uint8_t *p_src, int p_stride){
    int i;
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    const pixel4 a= ((pixel4*)(src-stride))[0];
    const pixel4 b= ((pixel4*)(src-stride))[1];

    for(i=0; i<8; i++){
        ((pixel4*)(src+i*stride))[0]= a;
        ((pixel4*)(src+i*stride))[1]= b;
    }
}

static void FUNCC(pred8x8_horizontal)(uint8_t *p_src, int stride){
    int i;
    pixel *src = (pixel*)p_src;
    stride >>= sizeof(pixel)-1;

    for(i=0; i<8; i++){
        ((pixel4*)(src+i*stride))[0]=
        ((pixel4*)(src+i*stride))[1]= PIXEL_SPLAT_X4(src[-1+i*stride]);
    }
}

#define PRED8x8_X(n, v)\
static void FUNCC(pred8x8_##n##_dc)(uint8_t *p_src, int stride){\
    int i;\
    pixel *src = (pixel*)p_src;\
    stride >>= sizeof(pixel)-1;\
    for(i=0; i<8; i++){\
        ((pixel4*)(src+i*stride))[0]=\
        ((pixel4*)(src+i*stride))[1]= PIXEL_SPLAT_X4(v);\
    }\
}

PRED8x8_X(127, (1<<(BIT_DEPTH-1))-1);
PRED8x8_X(128, (1<<(BIT_DEPTH-1))+0);
PRED8x8_X(129, (1<<(BIT_DEPTH-1))+1);

static void FUNCC(pred8x8_left_dc)(uint8_t *p_src, int stride){
    int i;
    int dc0, dc2;
    pixel4 dc0splat, dc2splat;
    pixel *src = (pixel*)p_src;
    stride >>= sizeof(pixel)-1;

    dc0=dc2=0;
    for(i=0;i<4; i++){
        dc0+= src[-1+i*stride];
        dc2+= src[-1+(i+4)*stride];
    }
    dc0splat = PIXEL_SPLAT_X4((dc0 + 2)>>2);
    dc2splat = PIXEL_SPLAT_X4((dc2 + 2)>>2);

    for(i=0; i<4; i++){
        ((pixel4*)(src+i*stride))[0]=
        ((pixel4*)(src+i*stride))[1]= dc0splat;
    }
    for(i=4; i<8; i++){
        ((pixel4*)(src+i*stride))[0]=
        ((pixel4*)(src+i*stride))[1]= dc2splat;
    }
}

static void FUNCC(pred8x8_left_dc_rv40)(uint8_t *p_src, int stride){
    int i;
    int dc0;
    pixel4 dc0splat;
    pixel *src = (pixel*)p_src;
    stride >>= sizeof(pixel)-1;

    dc0=0;
    for(i=0;i<8; i++)
        dc0+= src[-1+i*stride];
    dc0splat = PIXEL_SPLAT_X4((dc0 + 4)>>3);

    for(i=0; i<8; i++){
        ((pixel4*)(src+i*stride))[0]=
        ((pixel4*)(src+i*stride))[1]= dc0splat;
    }
}

static void FUNCC(pred8x8_top_dc)(uint8_t *p_src, int stride){
    int i;
    int dc0, dc1;
    pixel4 dc0splat, dc1splat;
    pixel *src = (pixel*)p_src;
    stride >>= sizeof(pixel)-1;

    dc0=dc1=0;
    for(i=0;i<4; i++){
        dc0+= src[i-stride];
        dc1+= src[4+i-stride];
    }
    dc0splat = PIXEL_SPLAT_X4((dc0 + 2)>>2);
    dc1splat = PIXEL_SPLAT_X4((dc1 + 2)>>2);

    for(i=0; i<4; i++){
        ((pixel4*)(src+i*stride))[0]= dc0splat;
        ((pixel4*)(src+i*stride))[1]= dc1splat;
    }
    for(i=4; i<8; i++){
        ((pixel4*)(src+i*stride))[0]= dc0splat;
        ((pixel4*)(src+i*stride))[1]= dc1splat;
    }
}

static void FUNCC(pred8x8_top_dc_rv40)(uint8_t *p_src, int stride){
    int i;
    int dc0;
    pixel4 dc0splat;
    pixel *src = (pixel*)p_src;
    stride >>= sizeof(pixel)-1;

    dc0=0;
    for(i=0;i<8; i++)
        dc0+= src[i-stride];
    dc0splat = PIXEL_SPLAT_X4((dc0 + 4)>>3);

    for(i=0; i<8; i++){
        ((pixel4*)(src+i*stride))[0]=
        ((pixel4*)(src+i*stride))[1]= dc0splat;
    }
}


static void FUNCC(pred8x8_dc)(uint8_t *p_src, int stride){
    int i;
    int dc0, dc1, dc2;
    pixel4 dc0splat, dc1splat, dc2splat, dc3splat;
    pixel *src = (pixel*)p_src;
    stride >>= sizeof(pixel)-1;

    dc0=dc1=dc2=0;
    for(i=0;i<4; i++){
        dc0+= src[-1+i*stride] + src[i-stride];
        dc1+= src[4+i-stride];
        dc2+= src[-1+(i+4)*stride];
    }
    dc0splat = PIXEL_SPLAT_X4((dc0 + 4)>>3);
    dc1splat = PIXEL_SPLAT_X4((dc1 + 2)>>2);
    dc2splat = PIXEL_SPLAT_X4((dc2 + 2)>>2);
    dc3splat = PIXEL_SPLAT_X4((dc1 + dc2 + 4)>>3);

    for(i=0; i<4; i++){
        ((pixel4*)(src+i*stride))[0]= dc0splat;
        ((pixel4*)(src+i*stride))[1]= dc1splat;
    }
    for(i=4; i<8; i++){
        ((pixel4*)(src+i*stride))[0]= dc2splat;
        ((pixel4*)(src+i*stride))[1]= dc3splat;
    }
}

//the following 4 function should not be optimized!
static void FUNC(pred8x8_mad_cow_dc_l0t)(uint8_t *src, int stride){
    FUNCC(pred8x8_top_dc)(src, stride);
    FUNCC(pred4x4_dc)(src, NULL, stride);
}

static void FUNC(pred8x8_mad_cow_dc_0lt)(uint8_t *src, int stride){
    FUNCC(pred8x8_dc)(src, stride);
    FUNCC(pred4x4_top_dc)(src, NULL, stride);
}

static void FUNC(pred8x8_mad_cow_dc_l00)(uint8_t *src, int stride){
    FUNCC(pred8x8_left_dc)(src, stride);
    FUNCC(pred4x4_128_dc)(src + 4*stride                  , NULL, stride);
    FUNCC(pred4x4_128_dc)(src + 4*stride + 4*sizeof(pixel), NULL, stride);
}

static void FUNC(pred8x8_mad_cow_dc_0l0)(uint8_t *src, int stride){
    FUNCC(pred8x8_left_dc)(src, stride);
    FUNCC(pred4x4_128_dc)(src                  , NULL, stride);
    FUNCC(pred4x4_128_dc)(src + 4*sizeof(pixel), NULL, stride);
}

static void FUNCC(pred8x8_dc_rv40)(uint8_t *p_src, int stride){
    int i;
    int dc0=0;
    pixel4 dc0splat;
    pixel *src = (pixel*)p_src;
    stride >>= sizeof(pixel)-1;

    for(i=0;i<4; i++){
        dc0+= src[-1+i*stride] + src[i-stride];
        dc0+= src[4+i-stride];
        dc0+= src[-1+(i+4)*stride];
    }
    dc0splat = PIXEL_SPLAT_X4((dc0 + 8)>>4);

    for(i=0; i<4; i++){
        ((pixel4*)(src+i*stride))[0]= dc0splat;
        ((pixel4*)(src+i*stride))[1]= dc0splat;
    }
    for(i=4; i<8; i++){
        ((pixel4*)(src+i*stride))[0]= dc0splat;
        ((pixel4*)(src+i*stride))[1]= dc0splat;
    }
}

static void FUNCC(pred8x8_plane)(uint8_t *p_src, int p_stride){
  int j, k;
  int a;
  INIT_CLIP
  pixel *src = (pixel*)p_src;
  int stride = p_stride>>(sizeof(pixel)-1);
  const pixel * const src0 = src +3-stride;
  const pixel *       src1 = src +4*stride-1;
  const pixel *       src2 = src1-2*stride;    // == src+2*stride-1;
  int H = src0[1] - src0[-1];
  int V = src1[0] - src2[ 0];
  for(k=2; k<=4; ++k) {
    src1 += stride; src2 -= stride;
    H += k*(src0[k] - src0[-k]);
    V += k*(src1[0] - src2[ 0]);
  }
  H = ( 17*H+16 ) >> 5;
  V = ( 17*V+16 ) >> 5;

  a = 16*(src1[0] + src2[8]+1) - 3*(V+H);
  for(j=8; j>0; --j) {
    int b = a;
    a += V;
    src[0] = CLIP((b    ) >> 5);
    src[1] = CLIP((b+  H) >> 5);
    src[2] = CLIP((b+2*H) >> 5);
    src[3] = CLIP((b+3*H) >> 5);
    src[4] = CLIP((b+4*H) >> 5);
    src[5] = CLIP((b+5*H) >> 5);
    src[6] = CLIP((b+6*H) >> 5);
    src[7] = CLIP((b+7*H) >> 5);
    src += stride;
  }
}

static void FUNCC(pred8x8_tm_vp8)(uint8_t *p_src, int p_stride){
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP - src[-1-stride];
    pixel *top = src-stride;
    int y;

    for (y = 0; y < 8; y++) {
        uint8_t *cm_in = cm + src[-1];
        src[0] = cm_in[top[0]];
        src[1] = cm_in[top[1]];
        src[2] = cm_in[top[2]];
        src[3] = cm_in[top[3]];
        src[4] = cm_in[top[4]];
        src[5] = cm_in[top[5]];
        src[6] = cm_in[top[6]];
        src[7] = cm_in[top[7]];
        src += stride;
    }
}

#define SRC(x,y) src[(x)+(y)*stride]
#define PL(y) \
    const int l##y = (SRC(-1,y-1) + 2*SRC(-1,y) + SRC(-1,y+1) + 2) >> 2;
#define PREDICT_8x8_LOAD_LEFT \
    const int l0 = ((has_topleft ? SRC(-1,-1) : SRC(-1,0)) \
                     + 2*SRC(-1,0) + SRC(-1,1) + 2) >> 2; \
    PL(1) PL(2) PL(3) PL(4) PL(5) PL(6) \
    const int l7 av_unused = (SRC(-1,6) + 3*SRC(-1,7) + 2) >> 2

#define PT(x) \
    const int t##x = (SRC(x-1,-1) + 2*SRC(x,-1) + SRC(x+1,-1) + 2) >> 2;
#define PREDICT_8x8_LOAD_TOP \
    const int t0 = ((has_topleft ? SRC(-1,-1) : SRC(0,-1)) \
                     + 2*SRC(0,-1) + SRC(1,-1) + 2) >> 2; \
    PT(1) PT(2) PT(3) PT(4) PT(5) PT(6) \
    const int t7 av_unused = ((has_topright ? SRC(8,-1) : SRC(7,-1)) \
                     + 2*SRC(7,-1) + SRC(6,-1) + 2) >> 2

#define PTR(x) \
    t##x = (SRC(x-1,-1) + 2*SRC(x,-1) + SRC(x+1,-1) + 2) >> 2;
#define PREDICT_8x8_LOAD_TOPRIGHT \
    int t8, t9, t10, t11, t12, t13, t14, t15; \
    if(has_topright) { \
        PTR(8) PTR(9) PTR(10) PTR(11) PTR(12) PTR(13) PTR(14) \
        t15 = (SRC(14,-1) + 3*SRC(15,-1) + 2) >> 2; \
    } else t8=t9=t10=t11=t12=t13=t14=t15= SRC(7,-1);

#define PREDICT_8x8_LOAD_TOPLEFT \
    const int lt = (SRC(-1,0) + 2*SRC(-1,-1) + SRC(0,-1) + 2) >> 2

#define PREDICT_8x8_DC(v) \
    int y; \
    for( y = 0; y < 8; y++ ) { \
        ((pixel4*)src)[0] = \
        ((pixel4*)src)[1] = v; \
        src += stride; \
    }

static void FUNCC(pred8x8l_128_dc)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);

    PREDICT_8x8_DC(PIXEL_SPLAT_X4(1<<(BIT_DEPTH-1)));
}
static void FUNCC(pred8x8l_left_dc)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);

    PREDICT_8x8_LOAD_LEFT;
    const pixel4 dc = PIXEL_SPLAT_X4((l0+l1+l2+l3+l4+l5+l6+l7+4) >> 3);
    PREDICT_8x8_DC(dc);
}
static void FUNCC(pred8x8l_top_dc)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);

    PREDICT_8x8_LOAD_TOP;
    const pixel4 dc = PIXEL_SPLAT_X4((t0+t1+t2+t3+t4+t5+t6+t7+4) >> 3);
    PREDICT_8x8_DC(dc);
}
static void FUNCC(pred8x8l_dc)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);

    PREDICT_8x8_LOAD_LEFT;
    PREDICT_8x8_LOAD_TOP;
    const pixel4 dc = PIXEL_SPLAT_X4((l0+l1+l2+l3+l4+l5+l6+l7
                                     +t0+t1+t2+t3+t4+t5+t6+t7+8) >> 4);
    PREDICT_8x8_DC(dc);
}
static void FUNCC(pred8x8l_horizontal)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);

    PREDICT_8x8_LOAD_LEFT;
#define ROW(y) ((pixel4*)(src+y*stride))[0] =\
               ((pixel4*)(src+y*stride))[1] = PIXEL_SPLAT_X4(l##y)
    ROW(0); ROW(1); ROW(2); ROW(3); ROW(4); ROW(5); ROW(6); ROW(7);
#undef ROW
}
static void FUNCC(pred8x8l_vertical)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    int y;
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);

    PREDICT_8x8_LOAD_TOP;
    src[0] = t0;
    src[1] = t1;
    src[2] = t2;
    src[3] = t3;
    src[4] = t4;
    src[5] = t5;
    src[6] = t6;
    src[7] = t7;
    for( y = 1; y < 8; y++ ) {
        ((pixel4*)(src+y*stride))[0] = ((pixel4*)src)[0];
        ((pixel4*)(src+y*stride))[1] = ((pixel4*)src)[1];
    }
}
static void FUNCC(pred8x8l_down_left)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    PREDICT_8x8_LOAD_TOP;
    PREDICT_8x8_LOAD_TOPRIGHT;
    SRC(0,0)= (t0 + 2*t1 + t2 + 2) >> 2;
    SRC(0,1)=SRC(1,0)= (t1 + 2*t2 + t3 + 2) >> 2;
    SRC(0,2)=SRC(1,1)=SRC(2,0)= (t2 + 2*t3 + t4 + 2) >> 2;
    SRC(0,3)=SRC(1,2)=SRC(2,1)=SRC(3,0)= (t3 + 2*t4 + t5 + 2) >> 2;
    SRC(0,4)=SRC(1,3)=SRC(2,2)=SRC(3,1)=SRC(4,0)= (t4 + 2*t5 + t6 + 2) >> 2;
    SRC(0,5)=SRC(1,4)=SRC(2,3)=SRC(3,2)=SRC(4,1)=SRC(5,0)= (t5 + 2*t6 + t7 + 2) >> 2;
    SRC(0,6)=SRC(1,5)=SRC(2,4)=SRC(3,3)=SRC(4,2)=SRC(5,1)=SRC(6,0)= (t6 + 2*t7 + t8 + 2) >> 2;
    SRC(0,7)=SRC(1,6)=SRC(2,5)=SRC(3,4)=SRC(4,3)=SRC(5,2)=SRC(6,1)=SRC(7,0)= (t7 + 2*t8 + t9 + 2) >> 2;
    SRC(1,7)=SRC(2,6)=SRC(3,5)=SRC(4,4)=SRC(5,3)=SRC(6,2)=SRC(7,1)= (t8 + 2*t9 + t10 + 2) >> 2;
    SRC(2,7)=SRC(3,6)=SRC(4,5)=SRC(5,4)=SRC(6,3)=SRC(7,2)= (t9 + 2*t10 + t11 + 2) >> 2;
    SRC(3,7)=SRC(4,6)=SRC(5,5)=SRC(6,4)=SRC(7,3)= (t10 + 2*t11 + t12 + 2) >> 2;
    SRC(4,7)=SRC(5,6)=SRC(6,5)=SRC(7,4)= (t11 + 2*t12 + t13 + 2) >> 2;
    SRC(5,7)=SRC(6,6)=SRC(7,5)= (t12 + 2*t13 + t14 + 2) >> 2;
    SRC(6,7)=SRC(7,6)= (t13 + 2*t14 + t15 + 2) >> 2;
    SRC(7,7)= (t14 + 3*t15 + 2) >> 2;
}
static void FUNCC(pred8x8l_down_right)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    PREDICT_8x8_LOAD_TOP;
    PREDICT_8x8_LOAD_LEFT;
    PREDICT_8x8_LOAD_TOPLEFT;
    SRC(0,7)= (l7 + 2*l6 + l5 + 2) >> 2;
    SRC(0,6)=SRC(1,7)= (l6 + 2*l5 + l4 + 2) >> 2;
    SRC(0,5)=SRC(1,6)=SRC(2,7)= (l5 + 2*l4 + l3 + 2) >> 2;
    SRC(0,4)=SRC(1,5)=SRC(2,6)=SRC(3,7)= (l4 + 2*l3 + l2 + 2) >> 2;
    SRC(0,3)=SRC(1,4)=SRC(2,5)=SRC(3,6)=SRC(4,7)= (l3 + 2*l2 + l1 + 2) >> 2;
    SRC(0,2)=SRC(1,3)=SRC(2,4)=SRC(3,5)=SRC(4,6)=SRC(5,7)= (l2 + 2*l1 + l0 + 2) >> 2;
    SRC(0,1)=SRC(1,2)=SRC(2,3)=SRC(3,4)=SRC(4,5)=SRC(5,6)=SRC(6,7)= (l1 + 2*l0 + lt + 2) >> 2;
    SRC(0,0)=SRC(1,1)=SRC(2,2)=SRC(3,3)=SRC(4,4)=SRC(5,5)=SRC(6,6)=SRC(7,7)= (l0 + 2*lt + t0 + 2) >> 2;
    SRC(1,0)=SRC(2,1)=SRC(3,2)=SRC(4,3)=SRC(5,4)=SRC(6,5)=SRC(7,6)= (lt + 2*t0 + t1 + 2) >> 2;
    SRC(2,0)=SRC(3,1)=SRC(4,2)=SRC(5,3)=SRC(6,4)=SRC(7,5)= (t0 + 2*t1 + t2 + 2) >> 2;
    SRC(3,0)=SRC(4,1)=SRC(5,2)=SRC(6,3)=SRC(7,4)= (t1 + 2*t2 + t3 + 2) >> 2;
    SRC(4,0)=SRC(5,1)=SRC(6,2)=SRC(7,3)= (t2 + 2*t3 + t4 + 2) >> 2;
    SRC(5,0)=SRC(6,1)=SRC(7,2)= (t3 + 2*t4 + t5 + 2) >> 2;
    SRC(6,0)=SRC(7,1)= (t4 + 2*t5 + t6 + 2) >> 2;
    SRC(7,0)= (t5 + 2*t6 + t7 + 2) >> 2;
}
static void FUNCC(pred8x8l_vertical_right)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    PREDICT_8x8_LOAD_TOP;
    PREDICT_8x8_LOAD_LEFT;
    PREDICT_8x8_LOAD_TOPLEFT;
    SRC(0,6)= (l5 + 2*l4 + l3 + 2) >> 2;
    SRC(0,7)= (l6 + 2*l5 + l4 + 2) >> 2;
    SRC(0,4)=SRC(1,6)= (l3 + 2*l2 + l1 + 2) >> 2;
    SRC(0,5)=SRC(1,7)= (l4 + 2*l3 + l2 + 2) >> 2;
    SRC(0,2)=SRC(1,4)=SRC(2,6)= (l1 + 2*l0 + lt + 2) >> 2;
    SRC(0,3)=SRC(1,5)=SRC(2,7)= (l2 + 2*l1 + l0 + 2) >> 2;
    SRC(0,1)=SRC(1,3)=SRC(2,5)=SRC(3,7)= (l0 + 2*lt + t0 + 2) >> 2;
    SRC(0,0)=SRC(1,2)=SRC(2,4)=SRC(3,6)= (lt + t0 + 1) >> 1;
    SRC(1,1)=SRC(2,3)=SRC(3,5)=SRC(4,7)= (lt + 2*t0 + t1 + 2) >> 2;
    SRC(1,0)=SRC(2,2)=SRC(3,4)=SRC(4,6)= (t0 + t1 + 1) >> 1;
    SRC(2,1)=SRC(3,3)=SRC(4,5)=SRC(5,7)= (t0 + 2*t1 + t2 + 2) >> 2;
    SRC(2,0)=SRC(3,2)=SRC(4,4)=SRC(5,6)= (t1 + t2 + 1) >> 1;
    SRC(3,1)=SRC(4,3)=SRC(5,5)=SRC(6,7)= (t1 + 2*t2 + t3 + 2) >> 2;
    SRC(3,0)=SRC(4,2)=SRC(5,4)=SRC(6,6)= (t2 + t3 + 1) >> 1;
    SRC(4,1)=SRC(5,3)=SRC(6,5)=SRC(7,7)= (t2 + 2*t3 + t4 + 2) >> 2;
    SRC(4,0)=SRC(5,2)=SRC(6,4)=SRC(7,6)= (t3 + t4 + 1) >> 1;
    SRC(5,1)=SRC(6,3)=SRC(7,5)= (t3 + 2*t4 + t5 + 2) >> 2;
    SRC(5,0)=SRC(6,2)=SRC(7,4)= (t4 + t5 + 1) >> 1;
    SRC(6,1)=SRC(7,3)= (t4 + 2*t5 + t6 + 2) >> 2;
    SRC(6,0)=SRC(7,2)= (t5 + t6 + 1) >> 1;
    SRC(7,1)= (t5 + 2*t6 + t7 + 2) >> 2;
    SRC(7,0)= (t6 + t7 + 1) >> 1;
}
static void FUNCC(pred8x8l_horizontal_down)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    PREDICT_8x8_LOAD_TOP;
    PREDICT_8x8_LOAD_LEFT;
    PREDICT_8x8_LOAD_TOPLEFT;
    SRC(0,7)= (l6 + l7 + 1) >> 1;
    SRC(1,7)= (l5 + 2*l6 + l7 + 2) >> 2;
    SRC(0,6)=SRC(2,7)= (l5 + l6 + 1) >> 1;
    SRC(1,6)=SRC(3,7)= (l4 + 2*l5 + l6 + 2) >> 2;
    SRC(0,5)=SRC(2,6)=SRC(4,7)= (l4 + l5 + 1) >> 1;
    SRC(1,5)=SRC(3,6)=SRC(5,7)= (l3 + 2*l4 + l5 + 2) >> 2;
    SRC(0,4)=SRC(2,5)=SRC(4,6)=SRC(6,7)= (l3 + l4 + 1) >> 1;
    SRC(1,4)=SRC(3,5)=SRC(5,6)=SRC(7,7)= (l2 + 2*l3 + l4 + 2) >> 2;
    SRC(0,3)=SRC(2,4)=SRC(4,5)=SRC(6,6)= (l2 + l3 + 1) >> 1;
    SRC(1,3)=SRC(3,4)=SRC(5,5)=SRC(7,6)= (l1 + 2*l2 + l3 + 2) >> 2;
    SRC(0,2)=SRC(2,3)=SRC(4,4)=SRC(6,5)= (l1 + l2 + 1) >> 1;
    SRC(1,2)=SRC(3,3)=SRC(5,4)=SRC(7,5)= (l0 + 2*l1 + l2 + 2) >> 2;
    SRC(0,1)=SRC(2,2)=SRC(4,3)=SRC(6,4)= (l0 + l1 + 1) >> 1;
    SRC(1,1)=SRC(3,2)=SRC(5,3)=SRC(7,4)= (lt + 2*l0 + l1 + 2) >> 2;
    SRC(0,0)=SRC(2,1)=SRC(4,2)=SRC(6,3)= (lt + l0 + 1) >> 1;
    SRC(1,0)=SRC(3,1)=SRC(5,2)=SRC(7,3)= (l0 + 2*lt + t0 + 2) >> 2;
    SRC(2,0)=SRC(4,1)=SRC(6,2)= (t1 + 2*t0 + lt + 2) >> 2;
    SRC(3,0)=SRC(5,1)=SRC(7,2)= (t2 + 2*t1 + t0 + 2) >> 2;
    SRC(4,0)=SRC(6,1)= (t3 + 2*t2 + t1 + 2) >> 2;
    SRC(5,0)=SRC(7,1)= (t4 + 2*t3 + t2 + 2) >> 2;
    SRC(6,0)= (t5 + 2*t4 + t3 + 2) >> 2;
    SRC(7,0)= (t6 + 2*t5 + t4 + 2) >> 2;
}
static void FUNCC(pred8x8l_vertical_left)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    PREDICT_8x8_LOAD_TOP;
    PREDICT_8x8_LOAD_TOPRIGHT;
    SRC(0,0)= (t0 + t1 + 1) >> 1;
    SRC(0,1)= (t0 + 2*t1 + t2 + 2) >> 2;
    SRC(0,2)=SRC(1,0)= (t1 + t2 + 1) >> 1;
    SRC(0,3)=SRC(1,1)= (t1 + 2*t2 + t3 + 2) >> 2;
    SRC(0,4)=SRC(1,2)=SRC(2,0)= (t2 + t3 + 1) >> 1;
    SRC(0,5)=SRC(1,3)=SRC(2,1)= (t2 + 2*t3 + t4 + 2) >> 2;
    SRC(0,6)=SRC(1,4)=SRC(2,2)=SRC(3,0)= (t3 + t4 + 1) >> 1;
    SRC(0,7)=SRC(1,5)=SRC(2,3)=SRC(3,1)= (t3 + 2*t4 + t5 + 2) >> 2;
    SRC(1,6)=SRC(2,4)=SRC(3,2)=SRC(4,0)= (t4 + t5 + 1) >> 1;
    SRC(1,7)=SRC(2,5)=SRC(3,3)=SRC(4,1)= (t4 + 2*t5 + t6 + 2) >> 2;
    SRC(2,6)=SRC(3,4)=SRC(4,2)=SRC(5,0)= (t5 + t6 + 1) >> 1;
    SRC(2,7)=SRC(3,5)=SRC(4,3)=SRC(5,1)= (t5 + 2*t6 + t7 + 2) >> 2;
    SRC(3,6)=SRC(4,4)=SRC(5,2)=SRC(6,0)= (t6 + t7 + 1) >> 1;
    SRC(3,7)=SRC(4,5)=SRC(5,3)=SRC(6,1)= (t6 + 2*t7 + t8 + 2) >> 2;
    SRC(4,6)=SRC(5,4)=SRC(6,2)=SRC(7,0)= (t7 + t8 + 1) >> 1;
    SRC(4,7)=SRC(5,5)=SRC(6,3)=SRC(7,1)= (t7 + 2*t8 + t9 + 2) >> 2;
    SRC(5,6)=SRC(6,4)=SRC(7,2)= (t8 + t9 + 1) >> 1;
    SRC(5,7)=SRC(6,5)=SRC(7,3)= (t8 + 2*t9 + t10 + 2) >> 2;
    SRC(6,6)=SRC(7,4)= (t9 + t10 + 1) >> 1;
    SRC(6,7)=SRC(7,5)= (t9 + 2*t10 + t11 + 2) >> 2;
    SRC(7,6)= (t10 + t11 + 1) >> 1;
    SRC(7,7)= (t10 + 2*t11 + t12 + 2) >> 2;
}
static void FUNCC(pred8x8l_horizontal_up)(uint8_t *p_src, int has_topleft, int has_topright, int p_stride)
{
    pixel *src = (pixel*)p_src;
    int stride = p_stride>>(sizeof(pixel)-1);
    PREDICT_8x8_LOAD_LEFT;
    SRC(0,0)= (l0 + l1 + 1) >> 1;
    SRC(1,0)= (l0 + 2*l1 + l2 + 2) >> 2;
    SRC(0,1)=SRC(2,0)= (l1 + l2 + 1) >> 1;
    SRC(1,1)=SRC(3,0)= (l1 + 2*l2 + l3 + 2) >> 2;
    SRC(0,2)=SRC(2,1)=SRC(4,0)= (l2 + l3 + 1) >> 1;
    SRC(1,2)=SRC(3,1)=SRC(5,0)= (l2 + 2*l3 + l4 + 2) >> 2;
    SRC(0,3)=SRC(2,2)=SRC(4,1)=SRC(6,0)= (l3 + l4 + 1) >> 1;
    SRC(1,3)=SRC(3,2)=SRC(5,1)=SRC(7,0)= (l3 + 2*l4 + l5 + 2) >> 2;
    SRC(0,4)=SRC(2,3)=SRC(4,2)=SRC(6,1)= (l4 + l5 + 1) >> 1;
    SRC(1,4)=SRC(3,3)=SRC(5,2)=SRC(7,1)= (l4 + 2*l5 + l6 + 2) >> 2;
    SRC(0,5)=SRC(2,4)=SRC(4,3)=SRC(6,2)= (l5 + l6 + 1) >> 1;
    SRC(1,5)=SRC(3,4)=SRC(5,3)=SRC(7,2)= (l5 + 2*l6 + l7 + 2) >> 2;
    SRC(0,6)=SRC(2,5)=SRC(4,4)=SRC(6,3)= (l6 + l7 + 1) >> 1;
    SRC(1,6)=SRC(3,5)=SRC(5,4)=SRC(7,3)= (l6 + 3*l7 + 2) >> 2;
    SRC(0,7)=SRC(1,7)=SRC(2,6)=SRC(2,7)=SRC(3,6)=
    SRC(3,7)=SRC(4,5)=SRC(4,6)=SRC(4,7)=SRC(5,5)=
    SRC(5,6)=SRC(5,7)=SRC(6,4)=SRC(6,5)=SRC(6,6)=
    SRC(6,7)=SRC(7,4)=SRC(7,5)=SRC(7,6)=SRC(7,7)= l7;
}
#undef PREDICT_8x8_LOAD_LEFT
#undef PREDICT_8x8_LOAD_TOP
#undef PREDICT_8x8_LOAD_TOPLEFT
#undef PREDICT_8x8_LOAD_TOPRIGHT
#undef PREDICT_8x8_DC
#undef PTR
#undef PT
#undef PL
#undef SRC

static void FUNCC(pred4x4_vertical_add)(uint8_t *p_pix, const DCTELEM *p_block, int stride){
    int i;
    pixel *pix = (pixel*)p_pix;
    const dctcoef *block = (const dctcoef*)p_block;
    stride >>= sizeof(pixel)-1;
    pix -= stride;
    for(i=0; i<4; i++){
        pixel v = pix[0];
        pix[1*stride]= v += block[0];
        pix[2*stride]= v += block[4];
        pix[3*stride]= v += block[8];
        pix[4*stride]= v +  block[12];
        pix++;
        block++;
    }
}

static void FUNCC(pred4x4_horizontal_add)(uint8_t *p_pix, const DCTELEM *p_block, int stride){
    int i;
    pixel *pix = (pixel*)p_pix;
    const dctcoef *block = (const dctcoef*)p_block;
    stride >>= sizeof(pixel)-1;
    for(i=0; i<4; i++){
        pixel v = pix[-1];
        pix[0]= v += block[0];
        pix[1]= v += block[1];
        pix[2]= v += block[2];
        pix[3]= v +  block[3];
        pix+= stride;
        block+= 4;
    }
}

static void FUNCC(pred8x8l_vertical_add)(uint8_t *p_pix, const DCTELEM *p_block, int stride){
    int i;
    pixel *pix = (pixel*)p_pix;
    const dctcoef *block = (const dctcoef*)p_block;
    stride >>= sizeof(pixel)-1;
    pix -= stride;
    for(i=0; i<8; i++){
        pixel v = pix[0];
        pix[1*stride]= v += block[0];
        pix[2*stride]= v += block[8];
        pix[3*stride]= v += block[16];
        pix[4*stride]= v += block[24];
        pix[5*stride]= v += block[32];
        pix[6*stride]= v += block[40];
        pix[7*stride]= v += block[48];
        pix[8*stride]= v +  block[56];
        pix++;
        block++;
    }
}

static void FUNCC(pred8x8l_horizontal_add)(uint8_t *p_pix, const DCTELEM *p_block, int stride){
    int i;
    pixel *pix = (pixel*)p_pix;
    const dctcoef *block = (const dctcoef*)p_block;
    stride >>= sizeof(pixel)-1;
    for(i=0; i<8; i++){
        pixel v = pix[-1];
        pix[0]= v += block[0];
        pix[1]= v += block[1];
        pix[2]= v += block[2];
        pix[3]= v += block[3];
        pix[4]= v += block[4];
        pix[5]= v += block[5];
        pix[6]= v += block[6];
        pix[7]= v +  block[7];
        pix+= stride;
        block+= 8;
    }
}

static void FUNCC(pred16x16_vertical_add)(uint8_t *pix, const int *block_offset, const DCTELEM *block, int stride){
    int i;
    for(i=0; i<16; i++)
        FUNCC(pred4x4_vertical_add)(pix + block_offset[i], block + i*16*sizeof(pixel), stride);
}

static void FUNCC(pred16x16_horizontal_add)(uint8_t *pix, const int *block_offset, const DCTELEM *block, int stride){
    int i;
    for(i=0; i<16; i++)
        FUNCC(pred4x4_horizontal_add)(pix + block_offset[i], block + i*16*sizeof(pixel), stride);
}

static void FUNCC(pred8x8_vertical_add)(uint8_t *pix, const int *block_offset, const DCTELEM *block, int stride){
    int i;
    for(i=0; i<4; i++)
        FUNCC(pred4x4_vertical_add)(pix + block_offset[i], block + i*16*sizeof(pixel), stride);
}

static void FUNCC(pred8x8_horizontal_add)(uint8_t *pix, const int *block_offset, const DCTELEM *block, int stride){
    int i;
    for(i=0; i<4; i++)
        FUNCC(pred4x4_horizontal_add)(pix + block_offset[i], block + i*16*sizeof(pixel), stride);
}

// nv12
#define LOAD_TOP_RIGHT_EDGE_nv12\
    const int av_unused t4= topright[0];\
    const int av_unused t5= topright[2];\
    const int av_unused t6= topright[4];\
    const int av_unused t7= topright[6];\

#define LOAD_DOWN_LEFT_EDGE_nv12\
    const int av_unused l4= src[-2+4*stride];\
    const int av_unused l5= src[-2+5*stride];\
    const int av_unused l6= src[-2+6*stride];\
    const int av_unused l7= src[-2+7*stride];\

#define LOAD_LEFT_EDGE_nv12\
    const int av_unused l0= src[-2+0*stride];\
    const int av_unused l1= src[-2+1*stride];\
    const int av_unused l2= src[-2+2*stride];\
    const int av_unused l3= src[-2+3*stride];\

#define LOAD_TOP_EDGE_nv12\
    const int av_unused t0= src[ 0-1*stride];\
    const int av_unused t1= src[ 2-1*stride];\
    const int av_unused t2= src[ 4-1*stride];\
    const int av_unused t3= src[ 6-1*stride];\

static void FUNCC(pred4x4_vertical_nv12)(uint8_t *src, uint8_t *topright, int stride){
    const uint32_t a= ((uint32_t*)(src-stride))[0];
    const uint32_t b= ((uint32_t*)(src-stride))[1];
    ((uint32_t*)(src+0*stride))[0]= a;
    ((uint32_t*)(src+0*stride))[1]= b;
    ((uint32_t*)(src+1*stride))[0]= a;
    ((uint32_t*)(src+1*stride))[1]= b;
    ((uint32_t*)(src+2*stride))[0]= a;
    ((uint32_t*)(src+2*stride))[1]= b;
    ((uint32_t*)(src+3*stride))[0]= a;
    ((uint32_t*)(src+3*stride))[1]= b;
}

static void FUNCC(pred4x4_horizontal_nv12)(uint8_t *src, uint8_t *topright, int stride){
    uint16_t* pt=(uint16_t*)(src-2);
    uint32_t value=(*pt)|((*pt)<<16);
    ((uint32_t*)(src+0*stride))[0]= ((uint32_t*)(src+0*stride))[1]=value;

    pt=(uint16_t*)(src+stride-2);
    value=(*pt)|((*pt)<<16);
    ((uint32_t*)(src+1*stride))[0]= ((uint32_t*)(src+1*stride))[1]=value;

    pt=(uint16_t*)(src+2*stride-2);
    value=(*pt)|((*pt)<<16);
    ((uint32_t*)(src+2*stride))[0]= ((uint32_t*)(src+2*stride))[1]=value;
    
    pt=(uint16_t*)(src+3*stride-2);
    value=(*pt)|((*pt)<<16);
    ((uint32_t*)(src+3*stride))[0]= ((uint32_t*)(src+3*stride))[1]=value;
}

static void FUNCC(pred4x4_dc_nv12)(uint8_t *src, uint8_t *topright, int stride){
    const int dc= (  src[-stride] + src[2-stride] + src[4-stride] + src[6-stride]
                   + src[-2+0*stride] + src[-2+1*stride] + src[-2+2*stride] + src[-2+3*stride] + 4) >>3;
    const int dcv= (  src[1-stride] + src[3-stride] + src[5-stride] + src[7-stride]
                   + src[-1+0*stride] + src[-1+1*stride] + src[-1+2*stride] + src[-1+3*stride] + 4) >>3;
    uint32_t value;
    uint8_t* p=(uint8_t*)&value;
    p[0]=p[2]=dc;
    p[1]=p[3]=dcv;
    ((uint32_t*)(src+0*stride))[0]=((uint32_t*)(src+0*stride))[1]=value;
    ((uint32_t*)(src+1*stride))[0]=((uint32_t*)(src+1*stride))[1]=value;
    ((uint32_t*)(src+2*stride))[0]=((uint32_t*)(src+2*stride))[1]=value;
    ((uint32_t*)(src+3*stride))[0]=((uint32_t*)(src+3*stride))[1]=value;
}

static void FUNCC(pred4x4_left_dc_nv12)(uint8_t *src, uint8_t *topright, int stride){
    const int dc= (  src[-2+0*stride] + src[-2+1*stride] + src[-2+2*stride] + src[-2+3*stride] + 2) >>2;
    const int dcv= (  src[-1+0*stride] + src[-1+1*stride] + src[-1+2*stride] + src[-1+3*stride] + 2) >>2;

    uint32_t value;
    uint8_t* p=(uint8_t*)&value;
    p[0]=p[2]=dc;
    p[1]=p[3]=dcv;
    
    ((uint32_t*)(src+0*stride))[0]=((uint32_t*)(src+0*stride))[1]=value;
    ((uint32_t*)(src+1*stride))[0]=((uint32_t*)(src+1*stride))[1]=value;
    ((uint32_t*)(src+2*stride))[0]=((uint32_t*)(src+2*stride))[1]=value;
    ((uint32_t*)(src+3*stride))[0]=((uint32_t*)(src+3*stride))[1]=value;
}

static void FUNCC(pred4x4_top_dc_nv12)(uint8_t *src, uint8_t *topright, int stride){
    const int dc= (  src[-stride] + src[2-stride] + src[4-stride] + src[6-stride] + 2) >>2;
    const int dcv= (  src[1-stride] + src[3-stride] + src[5-stride] + src[7-stride] + 2) >>2;

    uint32_t value;
    uint8_t* p=(uint8_t*)&value;
    p[0]=p[2]=dc;
    p[1]=p[3]=dcv;
    
    ((uint32_t*)(src+0*stride))[0]=((uint32_t*)(src+0*stride))[1]=value;
    ((uint32_t*)(src+1*stride))[0]=((uint32_t*)(src+1*stride))[1]=value;
    ((uint32_t*)(src+2*stride))[0]=((uint32_t*)(src+2*stride))[1]=value;
    ((uint32_t*)(src+3*stride))[0]=((uint32_t*)(src+3*stride))[1]=value;

}

static void FUNCC(pred4x4_128_dc_nv12)(uint8_t *src, uint8_t *topright, int stride){
    
    ((uint32_t*)(src+0*stride))[0]=((uint32_t*)(src+0*stride))[1]=0x80808080;
    ((uint32_t*)(src+1*stride))[0]=((uint32_t*)(src+1*stride))[1]=0x80808080;
    ((uint32_t*)(src+2*stride))[0]=((uint32_t*)(src+2*stride))[1]=0x80808080;
    ((uint32_t*)(src+3*stride))[0]=((uint32_t*)(src+3*stride))[1]=0x80808080;

}

static void FUNCC(pred4x4_down_right_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    const int lt= src[-2-1*stride];
    LOAD_TOP_EDGE_nv12
    LOAD_LEFT_EDGE_nv12

    src[0+3*stride]=(l3 + 2*l2 + l1 + 2)>>2;
    src[0+2*stride]=
    src[2+3*stride]=(l2 + 2*l1 + l0 + 2)>>2;
    src[0+1*stride]=
    src[2+2*stride]=
    src[4+3*stride]=(l1 + 2*l0 + lt + 2)>>2;
    src[0+0*stride]=
    src[2+1*stride]=
    src[4+2*stride]=
    src[6+3*stride]=(l0 + 2*lt + t0 + 2)>>2;
    src[2+0*stride]=
    src[4+1*stride]=
    src[6+2*stride]=(lt + 2*t0 + t1 + 2)>>2;
    src[4+0*stride]=
    src[6+1*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[6+0*stride]=(t1 + 2*t2 + t3 + 2)>>2;
}

static void FUNCC(pred4x4_down_right_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_down_right_nv12_2)(src,topright,stride);
    FUNCC(pred4x4_down_right_nv12_2)(src+1,topright+1,stride);
}

static void FUNCC(pred4x4_down_left_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    LOAD_TOP_EDGE_nv12
    LOAD_TOP_RIGHT_EDGE_nv12
//    LOAD_LEFT_EDGE

    src[0+0*stride]=(t0 + t2 + 2*t1 + 2)>>2;
    src[2+0*stride]=
    src[0+1*stride]=(t1 + t3 + 2*t2 + 2)>>2;
    src[4+0*stride]=
    src[2+1*stride]=
    src[0+2*stride]=(t2 + t4 + 2*t3 + 2)>>2;
    src[6+0*stride]=
    src[4+1*stride]=
    src[2+2*stride]=
    src[0+3*stride]=(t3 + t5 + 2*t4 + 2)>>2;
    src[6+1*stride]=
    src[4+2*stride]=
    src[2+3*stride]=(t4 + t6 + 2*t5 + 2)>>2;
    src[6+2*stride]=
    src[4+3*stride]=(t5 + t7 + 2*t6 + 2)>>2;
    src[6+3*stride]=(t6 + 3*t7 + 2)>>2;
}

static void FUNCC(pred4x4_down_left_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_down_left_nv12_2)(src, topright,stride);
    FUNCC(pred4x4_down_left_nv12_2)(src+1, topright+1,stride);
}

static void FUNCC(pred4x4_down_left_svq3_nv12)(uint8_t *src, uint8_t *topright, int stride){
    LOAD_TOP_EDGE_nv12
    LOAD_LEFT_EDGE_nv12
    const av_unused int unu0= t0;
    const av_unused int unu1= l0;

    src[0+0*stride]=(l1 + t1)>>1;
    src[2+0*stride]=
    src[0+1*stride]=(l2 + t2)>>1;
    src[4+0*stride]=
    src[2+1*stride]=
    src[0+2*stride]=
    src[6+0*stride]=
    src[4+1*stride]=
    src[2+2*stride]=
    src[0+3*stride]=
    src[6+1*stride]=
    src[4+2*stride]=
    src[2+3*stride]=
    src[6+2*stride]=
    src[4+3*stride]=
    src[6+3*stride]=(l3 + t3)>>1;
}

static void FUNCC(pred4x4_down_left_rv40_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    LOAD_TOP_EDGE_nv12
    LOAD_TOP_RIGHT_EDGE_nv12
    LOAD_LEFT_EDGE_nv12
    LOAD_DOWN_LEFT_EDGE_nv12

    src[0+0*stride]=(t0 + t2 + 2*t1 + 2 + l0 + l2 + 2*l1 + 2)>>3;
    src[2+0*stride]=
    src[0+1*stride]=(t1 + t3 + 2*t2 + 2 + l1 + l3 + 2*l2 + 2)>>3;
    src[4+0*stride]=
    src[2+1*stride]=
    src[0+2*stride]=(t2 + t4 + 2*t3 + 2 + l2 + l4 + 2*l3 + 2)>>3;
    src[6+0*stride]=
    src[4+1*stride]=
    src[2+2*stride]=
    src[0+3*stride]=(t3 + t5 + 2*t4 + 2 + l3 + l5 + 2*l4 + 2)>>3;
    src[6+1*stride]=
    src[4+2*stride]=
    src[2+3*stride]=(t4 + t6 + 2*t5 + 2 + l4 + l6 + 2*l5 + 2)>>3;
    src[6+2*stride]=
    src[4+3*stride]=(t5 + t7 + 2*t6 + 2 + l5 + l7 + 2*l6 + 2)>>3;
    src[6+3*stride]=(t6 + t7 + 1 + l6 + l7 + 1)>>2;
}

static void FUNCC(pred4x4_down_left_rv40_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_down_left_rv40_nv12_2)(src,topright,stride);
    FUNCC(pred4x4_down_left_rv40_nv12_2)(src+1,topright+1,stride);
}

static void FUNCC(pred4x4_down_left_rv40_nodown_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    LOAD_TOP_EDGE_nv12
    LOAD_TOP_RIGHT_EDGE_nv12
    LOAD_LEFT_EDGE_nv12

    src[0+0*stride]=(t0 + t2 + 2*t1 + 2 + l0 + l2 + 2*l1 + 2)>>3;
    src[2+0*stride]=
    src[0+1*stride]=(t1 + t3 + 2*t2 + 2 + l1 + l3 + 2*l2 + 2)>>3;
    src[4+0*stride]=
    src[2+1*stride]=
    src[0+2*stride]=(t2 + t4 + 2*t3 + 2 + l2 + 3*l3 + 2)>>3;
    src[6+0*stride]=
    src[4+1*stride]=
    src[2+2*stride]=
    src[0+3*stride]=(t3 + t5 + 2*t4 + 2 + l3*4 + 2)>>3;
    src[6+1*stride]=
    src[4+2*stride]=
    src[2+3*stride]=(t4 + t6 + 2*t5 + 2 + l3*4 + 2)>>3;
    src[6+2*stride]=
    src[4+3*stride]=(t5 + t7 + 2*t6 + 2 + l3*4 + 2)>>3;
    src[6+3*stride]=(t6 + t7 + 1 + 2*l3 + 1)>>2;
}

static void FUNCC(pred4x4_down_left_rv40_nodown_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_down_left_rv40_nodown_nv12_2)(src,topright,stride);
    FUNCC(pred4x4_down_left_rv40_nodown_nv12_2)(src+1,topright+1,stride);    
}

static void FUNCC(pred4x4_vertical_right_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    const int lt= src[-2-1*stride];
    LOAD_TOP_EDGE_nv12
    LOAD_LEFT_EDGE_nv12

    src[0+0*stride]=
    src[2+2*stride]=(lt + t0 + 1)>>1;
    src[2+0*stride]=
    src[4+2*stride]=(t0 + t1 + 1)>>1;
    src[4+0*stride]=
    src[6+2*stride]=(t1 + t2 + 1)>>1;
    src[6+0*stride]=(t2 + t3 + 1)>>1;
    src[0+1*stride]=
    src[2+3*stride]=(l0 + 2*lt + t0 + 2)>>2;
    src[2+1*stride]=
    src[4+3*stride]=(lt + 2*t0 + t1 + 2)>>2;
    src[4+1*stride]=
    src[6+3*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[6+1*stride]=(t1 + 2*t2 + t3 + 2)>>2;
    src[0+2*stride]=(lt + 2*l0 + l1 + 2)>>2;
    src[0+3*stride]=(l0 + 2*l1 + l2 + 2)>>2;
}

static void FUNCC(pred4x4_vertical_right_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_vertical_right_nv12_2)(src,topright,stride);
    FUNCC(pred4x4_vertical_right_nv12_2)(src+1,topright+1,stride);
}

static void FUNCC(pred4x4_vertical_left_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    LOAD_TOP_EDGE_nv12
    LOAD_TOP_RIGHT_EDGE_nv12

    src[0+0*stride]=(t0 + t1 + 1)>>1;
    src[2+0*stride]=
    src[0+2*stride]=(t1 + t2 + 1)>>1;
    src[4+0*stride]=
    src[2+2*stride]=(t2 + t3 + 1)>>1;
    src[6+0*stride]=
    src[4+2*stride]=(t3 + t4+ 1)>>1;
    src[6+2*stride]=(t4 + t5+ 1)>>1;
    src[0+1*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[2+1*stride]=
    src[0+3*stride]=(t1 + 2*t2 + t3 + 2)>>2;
    src[4+1*stride]=
    src[2+3*stride]=(t2 + 2*t3 + t4 + 2)>>2;
    src[6+1*stride]=
    src[4+3*stride]=(t3 + 2*t4 + t5 + 2)>>2;
    src[6+3*stride]=(t4 + 2*t5 + t6 + 2)>>2;
}

static void FUNCC(pred4x4_vertical_left_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_vertical_left_nv12_2)(src,topright,stride);
    FUNCC(pred4x4_vertical_left_nv12_2)(src+1,topright+1,stride);
}

static void FUNC(pred4x4_vertical_left_nv12)(uint8_t *src, uint8_t *topright, int stride,
                                      const int l0, const int l1, const int l2, const int l3, const int l4){
    LOAD_TOP_EDGE_nv12
    LOAD_TOP_RIGHT_EDGE_nv12

    src[0+0*stride]=(2*t0 + 2*t1 + l1 + 2*l2 + l3 + 4)>>3;
    src[2+0*stride]=
    src[0+2*stride]=(t1 + t2 + 1)>>1;
    src[4+0*stride]=
    src[2+2*stride]=(t2 + t3 + 1)>>1;
    src[6+0*stride]=
    src[4+2*stride]=(t3 + t4+ 1)>>1;
    src[6+2*stride]=(t4 + t5+ 1)>>1;
    src[0+1*stride]=(t0 + 2*t1 + t2 + l2 + 2*l3 + l4 + 4)>>3;
    src[2+1*stride]=
    src[0+3*stride]=(t1 + 2*t2 + t3 + 2)>>2;
    src[4+1*stride]=
    src[2+3*stride]=(t2 + 2*t3 + t4 + 2)>>2;
    src[6+1*stride]=
    src[4+3*stride]=(t3 + 2*t4 + t5 + 2)>>2;
    src[6+3*stride]=(t4 + 2*t5 + t6 + 2)>>2;
}

static void FUNCC(pred4x4_vertical_left_rv40_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    LOAD_LEFT_EDGE_nv12
    LOAD_DOWN_LEFT_EDGE_nv12

    FUNC(pred4x4_vertical_left_nv12)(src, topright, stride, l0, l1, l2, l3, l4);
}

static void FUNCC(pred4x4_vertical_left_rv40_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_vertical_left_rv40_nv12_2)(src,topright,stride);
    FUNCC(pred4x4_vertical_left_rv40_nv12_2)(src+1,topright+1,stride);
}

static void FUNCC(pred4x4_vertical_left_rv40_nodown_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    LOAD_LEFT_EDGE_nv12

    FUNC(pred4x4_vertical_left_nv12)(src, topright, stride, l0, l1, l2, l3, l3);
}

static void FUNCC(pred4x4_vertical_left_rv40_nodown_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_vertical_left_rv40_nodown_nv12_2)(src,topright,stride);
    FUNCC(pred4x4_vertical_left_rv40_nodown_nv12_2)(src+1,topright+1,stride);
}


static void FUNCC(pred4x4_horizontal_up_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    LOAD_LEFT_EDGE_nv12

    src[0+0*stride]=(l0 + l1 + 1)>>1;
    src[2+0*stride]=(l0 + 2*l1 + l2 + 2)>>2;
    src[4+0*stride]=
    src[0+1*stride]=(l1 + l2 + 1)>>1;
    src[6+0*stride]=
    src[2+1*stride]=(l1 + 2*l2 + l3 + 2)>>2;
    src[4+1*stride]=
    src[0+2*stride]=(l2 + l3 + 1)>>1;
    src[6+1*stride]=
    src[2+2*stride]=(l2 + 2*l3 + l3 + 2)>>2;
    src[6+2*stride]=
    src[2+3*stride]=
    src[0+3*stride]=
    src[4+2*stride]=
    src[4+3*stride]=
    src[6+3*stride]=l3;
}

static void FUNCC(pred4x4_horizontal_up_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_horizontal_up_nv12_2)(src,topright,stride);
    FUNCC(pred4x4_horizontal_up_nv12_2)(src+1,topright+1,stride);
}

static void FUNCC(pred4x4_horizontal_up_rv40_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    LOAD_LEFT_EDGE_nv12
    LOAD_DOWN_LEFT_EDGE_nv12
    LOAD_TOP_EDGE_nv12
    LOAD_TOP_RIGHT_EDGE_nv12

    src[0+0*stride]=(t1 + 2*t2 + t3 + 2*l0 + 2*l1 + 4)>>3;
    src[2+0*stride]=(t2 + 2*t3 + t4 + l0 + 2*l1 + l2 + 4)>>3;
    src[4+0*stride]=
    src[0+1*stride]=(t3 + 2*t4 + t5 + 2*l1 + 2*l2 + 4)>>3;
    src[6+0*stride]=
    src[2+1*stride]=(t4 + 2*t5 + t6 + l1 + 2*l2 + l3 + 4)>>3;
    src[4+1*stride]=
    src[0+2*stride]=(t5 + 2*t6 + t7 + 2*l2 + 2*l3 + 4)>>3;
    src[6+1*stride]=
    src[2+2*stride]=(t6 + 3*t7 + l2 + 3*l3 + 4)>>3;
    src[6+2*stride]=
    src[2+3*stride]=(l3 + 2*l4 + l5 + 2)>>2;
    src[0+3*stride]=
    src[4+2*stride]=(t6 + t7 + l3 + l4 + 2)>>2;
    src[4+3*stride]=(l4 + l5 + 1)>>1;
    src[6+3*stride]=(l4 + 2*l5 + l6 + 2)>>2;
}

static void FUNCC(pred4x4_horizontal_up_rv40_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_horizontal_up_rv40_nv12_2)(src,topright,stride);
    FUNCC(pred4x4_horizontal_up_rv40_nv12_2)(src+1,topright+1,stride);    
}

static void FUNCC(pred4x4_horizontal_up_rv40_nodown_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    LOAD_LEFT_EDGE_nv12
    LOAD_TOP_EDGE_nv12
    LOAD_TOP_RIGHT_EDGE_nv12

    src[0+0*stride]=(t1 + 2*t2 + t3 + 2*l0 + 2*l1 + 4)>>3;
    src[2+0*stride]=(t2 + 2*t3 + t4 + l0 + 2*l1 + l2 + 4)>>3;
    src[4+0*stride]=
    src[0+1*stride]=(t3 + 2*t4 + t5 + 2*l1 + 2*l2 + 4)>>3;
    src[6+0*stride]=
    src[2+1*stride]=(t4 + 2*t5 + t6 + l1 + 2*l2 + l3 + 4)>>3;
    src[4+1*stride]=
    src[0+2*stride]=(t5 + 2*t6 + t7 + 2*l2 + 2*l3 + 4)>>3;
    src[6+1*stride]=
    src[2+2*stride]=(t6 + 3*t7 + l2 + 3*l3 + 4)>>3;
    src[6+2*stride]=
    src[2+3*stride]=l3;
    src[0+3*stride]=
    src[4+2*stride]=(t6 + t7 + 2*l3 + 2)>>2;
    src[4+3*stride]=
    src[6+3*stride]=l3;
}

static void FUNCC(pred4x4_horizontal_up_rv40_nodown_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_horizontal_up_rv40_nodown_nv12_2)(src,topright,stride);
    FUNCC(pred4x4_horizontal_up_rv40_nodown_nv12_2)(src+1,topright+1,stride);
}

static void FUNCC(pred4x4_horizontal_down_nv12_2)(uint8_t *src, uint8_t *topright, int stride){
    const int lt= src[-2-1*stride];
    LOAD_TOP_EDGE_nv12
    LOAD_LEFT_EDGE_nv12

    src[0+0*stride]=
    src[4+1*stride]=(lt + l0 + 1)>>1;
    src[2+0*stride]=
    src[6+1*stride]=(l0 + 2*lt + t0 + 2)>>2;
    src[4+0*stride]=(lt + 2*t0 + t1 + 2)>>2;
    src[6+0*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[0+1*stride]=
    src[4+2*stride]=(l0 + l1 + 1)>>1;
    src[2+1*stride]=
    src[6+2*stride]=(lt + 2*l0 + l1 + 2)>>2;
    src[0+2*stride]=
    src[4+3*stride]=(l1 + l2+ 1)>>1;
    src[2+2*stride]=
    src[6+3*stride]=(l0 + 2*l1 + l2 + 2)>>2;
    src[0+3*stride]=(l2 + l3 + 1)>>1;
    src[2+3*stride]=(l1 + 2*l2 + l3 + 2)>>2;
}

static void FUNCC(pred4x4_horizontal_down_nv12)(uint8_t *src, uint8_t *topright, int stride){
    FUNCC(pred4x4_horizontal_down_nv12_2)(src,topright,stride);
    FUNCC(pred4x4_horizontal_down_nv12_2)(src+1,topright+1,stride);
}

static void FUNCC(pred8x8_vertical_nv12)(uint8_t *src, int stride){
    int i;
    const uint32_t a= ((uint32_t*)(src-stride))[0];
    const uint32_t b= ((uint32_t*)(src-stride))[1];
    const uint32_t c= ((uint32_t*)(src-stride))[2];
    const uint32_t d= ((uint32_t*)(src-stride))[3];

    for(i=0; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]= a;
        ((uint32_t*)(src+i*stride))[1]= b;
        ((uint32_t*)(src+i*stride))[2]= c;
        ((uint32_t*)(src+i*stride))[3]= d;
    }
}

static void FUNCC(pred8x8_horizontal_nv12)(uint8_t *src, int stride){
    int i;
    uint32_t value=0;
    uint32_t* p=NULL;

    for(i=0; i<8; i++){
        value=*((uint16_t*)(src-2+i*stride));
        value|=value<<16;
        p=(uint32_t*)(src+i*stride);
        p[0]=p[1]=p[2]=p[3]=value;
    }
}

static void FUNCC(pred8x8_128_dc_nv12)(uint8_t *src, int stride){
    int i;
    
    for(i=0; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[1]=
        ((uint32_t*)(src+i*stride))[2]=
        ((uint32_t*)(src+i*stride))[3]= 0x01010101U*128U;
    }
}

static void FUNCC(pred8x8_left_dc_nv12)(uint8_t *src, int stride){
    int i;
    int dc0, dc2;

    dc0=dc2=0;
    for(i=0;i<4; i++){
        dc0+= src[-2+i*stride];
        dc2+= src[-2+(i+4)*stride];
    }
    dc0= 0x01010101*((dc0 + 2)>>2);
    dc2= 0x01010101*((dc2 + 2)>>2);

    for(i=0; i<4; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[2]= dc0;
    }
    for(i=4; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[2]= dc2;
    }
}

static void FUNCC(pred8x8_left_dc_rv40_nv12)(uint8_t *src, int stride){
    int i;
    int dc0=0,dc1=0;
    uint32_t value=0;
    uint8_t* p=(uint8_t*)&value;

    for(i=0;i<8; i++)
    {
        dc0+= src[-2+i*stride];
        dc1+= src[-1+i*stride];
    }
    p[0]=p[2]=((dc0 + 4)>>3);
    p[1]=p[3]=((dc1 + 4)>>3);

    for(i=0; i<8; i++){
        p=(src+i*stride);
        ((uint32_t*)p)[0]=((uint32_t*)p)[1]=((uint32_t*)p)[2]=((uint32_t*)p)[3]=value;
    }
}

static void FUNCC(pred8x8_top_dc_nv12)(uint8_t *src, int stride){
    int i;
    int dc0, dc1;

    dc0=dc1=0;
    for(i=0;i<4; i++){
        dc0+= src[(i<<1)-stride];
        dc1+= src[8+(i<<1)-stride];
    }
    dc0= 0x01010101*((dc0 + 2)>>2);
    dc1= 0x01010101*((dc1 + 2)>>2);

    for(i=0; i<4; i++){
        ((uint32_t*)(src+i*stride))[0]= dc0;
        ((uint32_t*)(src+i*stride))[2]= dc1;
    }
    for(i=4; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]= dc0;
        ((uint32_t*)(src+i*stride))[2]= dc1;
    }
}

static void FUNCC(pred8x8_top_dc_rv40_nv12)(uint8_t *src, int stride){
    int i;
    int dc0=0,dc1=0;
    uint32_t value=0;
    uint8_t* p=(uint8_t*)&value;

    for(i=0;i<8; i++)
    {
        dc0+= src[(i<<1)-stride];
        dc1+= src[(i<<1)-stride+1];
    }
    p[0]=p[2]=((dc0 + 4)>>3);
    p[1]=p[3]=((dc1 + 4)>>3);

    for(i=0; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[1]=
        ((uint32_t*)(src+i*stride))[2]=
        ((uint32_t*)(src+i*stride))[3]= value;
    }
}

static void FUNCC(pred8x8_dc_nv12)(uint8_t *src, int stride){
    int i;
    int dc0, dc1, dc2, dc3;

    dc0=dc1=dc2=0;
    for(i=0;i<4; i++){
        dc0+= src[-2+i*stride] + src[2*i-stride];
        dc1+= src[8+2*i-stride];
        dc2+= src[-2+(i+4)*stride];
    }
    dc3= 0x01010101*((dc1 + dc2 + 4)>>3);
    dc0= 0x01010101*((dc0 + 4)>>3);
    dc1= 0x01010101*((dc1 + 2)>>2);
    dc2= 0x01010101*((dc2 + 2)>>2);

    for(i=0; i<4; i++){
        ((uint32_t*)(src+i*stride))[0]= dc0;
        ((uint32_t*)(src+i*stride))[2]= dc1;
    }
    for(i=4; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]= dc2;
        ((uint32_t*)(src+i*stride))[2]= dc3;
    }
}

static void FUNC(pred8x8_mad_cow_dc_l0t_nv12)(uint8_t *src, int stride){
    FUNCC(pred8x8_top_dc_nv12)(src, stride);
    FUNCC(pred4x4_dc_nv12)(src, NULL, stride);
}

static void FUNC(pred8x8_mad_cow_dc_0lt_nv12)(uint8_t *src, int stride){
    FUNCC(pred8x8_dc_nv12)(src, stride);
    FUNCC(pred4x4_top_dc_nv12)(src, NULL, stride);
}

static void FUNC(pred8x8_mad_cow_dc_l00_nv12)(uint8_t *src, int stride){
    FUNCC(pred8x8_left_dc_nv12)(src, stride);
    FUNCC(pred4x4_128_dc_nv12)(src + 4*stride    , NULL, stride);
    FUNCC(pred4x4_128_dc_nv12)(src + 4*stride + 4, NULL, stride);
}

static void FUNC(pred8x8_mad_cow_dc_0l0_nv12)(uint8_t *src, int stride){
    FUNCC(pred8x8_left_dc_nv12)(src, stride);
    FUNCC(pred4x4_128_dc_nv12)(src    , NULL, stride);
    FUNCC(pred4x4_128_dc_nv12)(src + 4, NULL, stride);
}

static void FUNCC(pred8x8_dc_rv40_nv12)(uint8_t *src, int stride){
    int i;
    int dc0=0,dc1=0;
    uint32_t value=0;
    uint8_t* p=(uint8_t*)&value;

    for(i=0;i<4; i++){
//        log_dump(2, &i, sizeof(int));
        dc0+= src[-2+i*stride] + src[(i<<1)-stride];
//        log_dump(2, &dc0, sizeof(int));
        dc0+= src[8+(i<<1)-stride];
//        log_dump(2, &dc0, sizeof(int));
        dc0+= src[-2+(i+4)*stride];
//        log_dump(2, &dc0, sizeof(int));
        
        dc1+= src[-1+i*stride] + src[(i<<1)-stride+1];
        dc1+= src[9+(i<<1)-stride];
        dc1+= src[-1+(i+4)*stride];
    }
    
//    log_dump(2, &dc0, sizeof(int));
//    log_dump(2, &dc1, sizeof(int));
    value=((dc0 + 8)>>4) | (((dc1 + 8)>>4)<<8);
//    log_dump(2, &value, sizeof(int));
//    value|=value<<16;
    p[0]=p[2]=((dc0 + 8)>>4);
    p[1]=p[3]=((dc1 + 8)>>4);

//    log_dump(2, &value, sizeof(int));

    for(i=0; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]= 
        ((uint32_t*)(src+i*stride))[1]= 
        ((uint32_t*)(src+i*stride))[2]= 
        ((uint32_t*)(src+i*stride))[3]= value;
    }
}

static void FUNCC(pred8x8_plane_nv12_2)(uint8_t *src, int stride){
  int j, k;
  int a;
  uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
  const uint8_t * const src0 = src+6-stride;
  const uint8_t *src1 = src+4*stride-2;
  const uint8_t *src2 = src1-2*stride;      // == src+2*stride-1;
  int H = src0[2] - src0[-2];
  int V = src1[0] - src2[ 0];
  for(k=2; k<=4; ++k) {
    src1 += stride; src2 -= stride;
    H += k*(src0[2*k] - src0[-2*k]);
    V += k*(src1[0] - src2[ 0]);
  }
  H = ( 17*H+16 ) >> 5;
  V = ( 17*V+16 ) >> 5;

  a = 16*(src1[0] + src2[16]+1) - 3*(V+H);
  for(j=8; j>0; --j) {
    int b = a;
    a += V;
    src[0] = cm[ (b    ) >> 5 ];
    src[2] = cm[ (b+  H) >> 5 ];
    src[4] = cm[ (b+2*H) >> 5 ];
    src[6] = cm[ (b+3*H) >> 5 ];
    src[8] = cm[ (b+4*H) >> 5 ];
    src[10] = cm[ (b+5*H) >> 5 ];
    src[12] = cm[ (b+6*H) >> 5 ];
    src[14] = cm[ (b+7*H) >> 5 ];
    src += stride;
  }
}

static void FUNCC(pred8x8_plane_nv12)(uint8_t *src, int stride){
  FUNCC(pred8x8_plane_nv12_2)(src, stride);
  FUNCC(pred8x8_plane_nv12_2)(src+1, stride);
}