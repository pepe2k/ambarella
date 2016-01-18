/*
 * RV30/40 decoder common data
 * Copyright (c) 2007 Mike Melanson, Konstantin Shishkov
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
 * @file libavcodec/amba_rv34.c
 * amba RV30/40 decoder common data
 */

#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "golomb.h"
#include "mathops.h"
#include "rectangle.h"

#include "rv34vlc.h"
#include "rv34data.h"
#include "amba_rv34.h"
#include "log_dump.h"

//#define DEBUG
//#define __log_decoding_process__

//#define __log_communicate_queue__

//#define __log_parallel_control__

//commit out motion comp
//#define __commit_out_motion_comp__

//commit out dequant/transform
//#define __commit_out_idct_dequant__

//remove memory op
//#define __remove_memory_op__

//commit out deblocking
//#define __commit_out_deblocking__

//optimize rv40
//#define __optimize_rv40__

//dump data
//#define __dump_data__

//8x8 block
#define _remove_block_

//generic functions
//format: 0 yuv420p, 1,nv12
void dump_yuv_data(char* filename, void* p_pic, unsigned int width, unsigned int height, unsigned int informat, unsigned int outformat, unsigned int cnt)
{
    char* p=NULL;
    FILE* pfile = NULL;
    Picture* pic = (Picture*)p_pic;
    unsigned int i,j;
    unsigned char* ptmp;
    if (cnt <((unsigned int)log_start_frame) || cnt>((unsigned int)log_end_frame)) {
        return;
    }
    if (!filename || !pic || informat>1 || outformat>1 || width==0 || height==0) {
        ambadec_assert_ffmpeg(0);
        return;
    }

    if (!pic->data[0] || !pic->data[1] || !pic->linesize[0] ||!pic->linesize[1]) {
        ambadec_assert_ffmpeg(0);
        return;
    }

    p=av_malloc(strlen(filename)+60);
    if(!p) {
        ambadec_assert_ffmpeg(0);
        return;
    }

    //y
    sprintf(p,"%s_%c_%d",filename,'Y',cnt);

    pfile=fopen(p,"wb");

    if (!pfile) {
        ambadec_assert_ffmpeg(0);
        av_free(p);
        return;
    }

    ptmp = pic->data[0];
    for (j = 0; j<height; j++, ptmp+=pic->linesize[0]) {
        fwrite(ptmp, 1, width, pfile);
    }
    fclose(pfile);

    //dump 420 plane
    if (informat == 0 && outformat == 0) {
        //u
        sprintf(p,"%s_%c_%d",filename,'U',cnt);

        pfile=fopen(p,"wb");

        if (!pfile) {
            ambadec_assert_ffmpeg(0);
            av_free(p);
            return;
        }

        ptmp = pic->data[1];
        for (j = 0; j<(height+1)/2; j++, ptmp+=pic->linesize[1]) {
            fwrite(ptmp, 1, (width+1)/2, pfile);
        }
        fclose(pfile);

        //v
        sprintf(p,"%s_%c_%d",filename,'V',cnt);

        pfile=fopen(p,"wb");

        if (!pfile) {
            ambadec_assert_ffmpeg(0);
            av_free(p);
            return;
        }

        ptmp = pic->data[2];
        for (j = 0; j<(height+1)/2; j++, ptmp+=pic->linesize[2]) {
            fwrite(ptmp, 1, (width+1)/2, pfile);
        }
        fclose(pfile);
    } else if (informat == 1 && outformat == 0) {
        unsigned int htmp=(height+1)>>1;
        unsigned int wtmp=(width+1)>>1;
        unsigned char* ptmpu=av_malloc(htmp*wtmp);
        unsigned char* ptmpv=av_malloc(htmp*wtmp);
        ambadec_assert_ffmpeg(ptmpu);
        ambadec_assert_ffmpeg(ptmpv);
        if (!ptmpu ||!ptmpv) {
            if (ptmpu)
                av_free(ptmpu);
            if (ptmpv)
                av_free(ptmpv);
            av_free(p);
            return;
        }

        for(j=0;j<htmp;j++) {
            ptmp=pic->data[1]+j*pic->linesize[1];
            for(i=0;i<wtmp;i++) {
                *ptmpu++= *ptmp++;
                *ptmpv++= *ptmp++;
            }
        }
        ptmpu-=htmp*wtmp;
        ptmpv-=htmp*wtmp;
        //u
        sprintf(p,"%s_%c_%d",filename,'U',cnt);

        pfile=fopen(p,"wb");

        if (!pfile) {
            ambadec_assert_ffmpeg(0);
            av_free(p);
            return;
        }

        fwrite(ptmpu, 1, htmp*wtmp, pfile);
        fclose(pfile);

        //v
        sprintf(p,"%s_%c_%d",filename,'V',cnt);

        pfile=fopen(p,"wb");

        if (!pfile) {
            ambadec_assert_ffmpeg(0);
            av_free(p);
            return;
        }

        fwrite(ptmpv, 1, htmp*wtmp, pfile);
        fclose(pfile);
    } else if (informat == 1 && outformat == 1) {
        //uv
        sprintf(p,"%s_%s_%d",filename,"UV",cnt);

        pfile=fopen(p,"wb");

        if (!pfile) {
            ambadec_assert_ffmpeg(0);
            free(p);
            return;
        }

        ptmp = pic->data[1];
        for (j = 0; j<(height+1)/2; j++, ptmp+=pic->linesize[1]) {
            fwrite(ptmp, 1, width, pfile);
        }
        fclose(pfile);
    } else {
        //not implement
        ambadec_assert_ffmpeg(0);
    }

    free(p);
}

void dump_yuv_data_mb(char* filename, void* p_pic, unsigned int width, unsigned int height, unsigned int informat, unsigned int cnt)
{
    char* p=NULL;
    FILE* pfile = NULL;
    Picture* pic = (Picture*)p_pic;
    unsigned int i,j;
    int ii=0,jj=0;
    unsigned char* py,*pu,*pv,*ptmp;
    if (cnt <((unsigned int)log_start_frame) || cnt>((unsigned int)log_end_frame)) {
        return;
    }
    if (!filename || !pic || informat>1 || width==0 || height==0) {
        ambadec_assert_ffmpeg(0);
        return;
    }

    if (!pic->data[0] || !pic->data[1] || !pic->linesize[0] ||!pic->linesize[1]) {
        ambadec_assert_ffmpeg(0);
        return;
    }

    p=av_malloc(strlen(filename)+60);
    if(!p) {
        ambadec_assert_ffmpeg(0);
        return;
    }

    //y
    sprintf(p,"%s_%s_%d",filename,"MB",cnt);

    pfile=fopen(p,"wt");

    if (!pfile) {
        ambadec_assert_ffmpeg(0);
        av_free(p);
        return;
    }

    if (informat == 0) {
        for (j = 0; j< (height>>4); j++) {
            for (i = 0; i< (width>>4); i++) {
                //mb, y
                //mb, u v
                py = pic->data[0] + (j<<4)*pic->linesize[0] + (i<<4);
                pu = pic->data[1] + (j<<3)*pic->linesize[1] + (i<<3);
                pv = pic->data[2] + (j<<3)*pic->linesize[2] + (i<<3);

                fprintf(pfile,"MB[y=%d,x=%d], Y:\n",j,i);

                ptmp = py;
                //y
                for(jj=0;jj<16;jj++) {
                    for(ii=0;ii<16;ii++) {
//                        fprintf(pfile,"%02.2x ", ptmp[ii]);
                        fprintf(pfile,"%2.2x ", ptmp[ii]);
                    }
                    fprintf(pfile,"\n");
                    ptmp+=pic->linesize[0];
                }
                fprintf(pfile," U:\n");
                //u
                ptmp = pu;
                for(jj=0;jj<8;jj++) {
                    for(ii=0;ii<8;ii++) {
 //                       fprintf(pfile,"%02.2x ", ptmp[ii]);
                        fprintf(pfile,"%2.2x ", ptmp[ii]);
                    }
                    fprintf(pfile,"\n");
                    ptmp+=pic->linesize[1];
                }
                fprintf(pfile," V:\n");
                //v
                ptmp = pv;
                for(jj=0;jj<8;jj++) {
                    for(ii=0;ii<8;ii++) {
//                        fprintf(pfile,"%02.2x ", ptmp[ii]);
                        fprintf(pfile,"%2.2x ", ptmp[ii]);
                    }
                    fprintf(pfile,"\n");
                    ptmp+=pic->linesize[2];
                }
            }
        }
        fclose(pfile);
    }
    if (informat == 1) {
        for (j = 0; j< (height>>4); j++) {
            for (i = 0; i< (width>>4); i++) {
                //mb, y
                //mb, u v
                py = pic->data[0] + (j<<4)*pic->linesize[0] + (i<<4);
                pu = pic->data[1] + (j<<3)*pic->linesize[1] + (i<<4);

                fprintf(pfile,"MB[y=%d,x=%d], Y:\n",j,i);

                ptmp = py;
                //y
                for(jj=0;jj<16;jj++) {
                    for(ii=0;ii<16;ii++) {
//                        fprintf(pfile,"%02.2x ", ptmp[ii]);
                        fprintf(pfile,"%2.2x ", ptmp[ii]);
                    }
                    fprintf(pfile,"\n");
                    ptmp+=pic->linesize[0];
                }
                fprintf(pfile," U:\n");
                //u
                ptmp = pu;
                for(jj=0;jj<8;jj++) {
                    for(ii=0;ii<16;ii+=2) {
//                        fprintf(pfile,"%02.2x ", ptmp[ii]);
                        fprintf(pfile,"%2.2x ", ptmp[ii]);
                    }
                    fprintf(pfile,"\n");
                    ptmp+=pic->linesize[1];
                }
                fprintf(pfile," V:\n");
                //v
                ptmp = pu;
                for(jj=0;jj<8;jj++) {
                    for(ii=1;ii<16;ii+=2) {
//                        fprintf(pfile,"%02.2x ", ptmp[ii]);
                        fprintf(pfile,"%2.2x ", ptmp[ii]);
                    }
                    fprintf(pfile,"\n");
                    ptmp+=pic->linesize[1];
                }

            }
        }
        fclose(pfile);
    } else {
        ambadec_assert_ffmpeg(0);
        av_free(p);
        return;
    }

    free(p);
}

static inline void ZERO8x2(void* dst, int stride)
{
    fill_rectangle(dst,                 1, 2, stride, 0, 4);
    fill_rectangle(((uint8_t*)(dst))+4, 1, 2, stride, 0, 4);
}

/** translation of RV30/40 macroblock types to lavc ones */
static const int rv34_mb_type_to_lavc[12] = {
    MB_TYPE_INTRA,
    MB_TYPE_INTRA16x16              | MB_TYPE_SEPARATE_DC,
    MB_TYPE_16x16   | MB_TYPE_L0,
    MB_TYPE_8x8     | MB_TYPE_L0,
    MB_TYPE_16x16   | MB_TYPE_L0,
    MB_TYPE_16x16   | MB_TYPE_L1,
    MB_TYPE_SKIP,
    MB_TYPE_DIRECT2 | MB_TYPE_16x16,
    MB_TYPE_16x8    | MB_TYPE_L0,
    MB_TYPE_8x16    | MB_TYPE_L0,
    MB_TYPE_16x16   | MB_TYPE_L0L1,
    MB_TYPE_16x16   | MB_TYPE_L0    | MB_TYPE_SEPARATE_DC
};


static RV34VLC intra_vlcs[NUM_INTRA_TABLES], inter_vlcs[NUM_INTER_TABLES];

/**
 * @defgroup vlc RV30/40 VLC generating functions
 * @{
 */

static const int table_offs[] = {
      0,   1818,   3622,   4144,   4698,   5234,   5804,   5868,   5900,   5932,
   5996,   6252,   6316,   6348,   6380,   7674,   8944,  10274,  11668,  12250,
  14060,  15846,  16372,  16962,  17512,  18148,  18180,  18212,  18244,  18308,
  18564,  18628,  18660,  18692,  20036,  21314,  22648,  23968,  24614,  26384,
  28190,  28736,  29366,  29938,  30608,  30640,  30672,  30704,  30768,  31024,
  31088,  31120,  31184,  32570,  33898,  35236,  36644,  37286,  39020,  40802,
  41368,  42052,  42692,  43348,  43380,  43412,  43444,  43476,  43604,  43668,
  43700,  43732,  45100,  46430,  47778,  49160,  49802,  51550,  53340,  53972,
  54648,  55348,  55994,  56122,  56154,  56186,  56218,  56346,  56410,  56442,
  56474,  57878,  59290,  60636,  62036,  62682,  64460,  64524,  64588,  64716,
  64844,  66076,  67466,  67978,  68542,  69064,  69648,  70296,  72010,  72074,
  72138,  72202,  72330,  73572,  74936,  75454,  76030,  76566,  77176,  77822,
  79582,  79646,  79678,  79742,  79870,  81180,  82536,  83064,  83672,  84242,
  84934,  85576,  87384,  87448,  87480,  87544,  87672,  88982,  90340,  90902,
  91598,  92182,  92846,  93488,  95246,  95278,  95310,  95374,  95502,  96878,
  98266,  98848,  99542, 100234, 100884, 101524, 103320, 103352, 103384, 103416,
 103480, 104874, 106222, 106910, 107584, 108258, 108902, 109544, 111366, 111398,
 111430, 111462, 111494, 112878, 114320, 114988, 115660, 116310, 116950, 117592
};

static VLC_TYPE table_data[117592][2];

/**
 * Generate VLC from codeword lengths.
 * @param bits   codeword lengths (zeroes are accepted)
 * @param size   length of input data
 * @param vlc    output VLC
 * @param insyms symbols for input codes (NULL for default ones)
 * @param num    VLC table number (for static initialization)
 */
static void rv34_gen_vlc(const uint8_t *bits, int size, VLC *vlc, const uint8_t *insyms,
                         const int num)
{
    int i;
    int counts[17] = {0}, codes[17];
    uint16_t cw[size], syms[size];
    uint8_t bits2[size];
    int maxbits = 0, realsize = 0;

    for(i = 0; i < size; i++){
        if(bits[i]){
            bits2[realsize] = bits[i];
            syms[realsize] = insyms ? insyms[i] : i;
            realsize++;
            maxbits = FFMAX(maxbits, bits[i]);
            counts[bits[i]]++;
        }
    }

    codes[0] = 0;
    for(i = 0; i < 16; i++)
        codes[i+1] = (codes[i] + counts[i]) << 1;
    for(i = 0; i < realsize; i++)
        cw[i] = codes[bits2[i]]++;

    vlc->table = &table_data[table_offs[num]];
    vlc->table_allocated = table_offs[num + 1] - table_offs[num];
    init_vlc_sparse(vlc, FFMIN(maxbits, 9), realsize,
                    bits2, 1, 1,
                    cw,    2, 2,
                    syms,  2, 2, INIT_VLC_USE_NEW_STATIC);
}

/**
 * Initialize all tables.
 */
static av_cold void rv34_amba_init_tables(void)
{
    int i, j, k;

    for(i = 0; i < NUM_INTRA_TABLES; i++){
        for(j = 0; j < 2; j++){
            rv34_gen_vlc(rv34_table_intra_cbppat   [i][j], CBPPAT_VLC_SIZE,   &intra_vlcs[i].cbppattern[j],     NULL, 19*i + 0 + j);
            rv34_gen_vlc(rv34_table_intra_secondpat[i][j], OTHERBLK_VLC_SIZE, &intra_vlcs[i].second_pattern[j], NULL, 19*i + 2 + j);
            rv34_gen_vlc(rv34_table_intra_thirdpat [i][j], OTHERBLK_VLC_SIZE, &intra_vlcs[i].third_pattern[j],  NULL, 19*i + 4 + j);
            for(k = 0; k < 4; k++){
                rv34_gen_vlc(rv34_table_intra_cbp[i][j+k*2],  CBP_VLC_SIZE,   &intra_vlcs[i].cbp[j][k],         rv34_cbp_code, 19*i + 6 + j*4 + k);
            }
        }
        for(j = 0; j < 4; j++){
            rv34_gen_vlc(rv34_table_intra_firstpat[i][j], FIRSTBLK_VLC_SIZE, &intra_vlcs[i].first_pattern[j], NULL, 19*i + 14 + j);
        }
        rv34_gen_vlc(rv34_intra_coeff[i], COEFF_VLC_SIZE, &intra_vlcs[i].coefficient, NULL, 19*i + 18);
    }

    for(i = 0; i < NUM_INTER_TABLES; i++){
        rv34_gen_vlc(rv34_inter_cbppat[i], CBPPAT_VLC_SIZE, &inter_vlcs[i].cbppattern[0], NULL, i*12 + 95);
        for(j = 0; j < 4; j++){
            rv34_gen_vlc(rv34_inter_cbp[i][j], CBP_VLC_SIZE, &inter_vlcs[i].cbp[0][j], rv34_cbp_code, i*12 + 96 + j);
        }
        for(j = 0; j < 2; j++){
            rv34_gen_vlc(rv34_table_inter_firstpat [i][j], FIRSTBLK_VLC_SIZE, &inter_vlcs[i].first_pattern[j],  NULL, i*12 + 100 + j);
            rv34_gen_vlc(rv34_table_inter_secondpat[i][j], OTHERBLK_VLC_SIZE, &inter_vlcs[i].second_pattern[j], NULL, i*12 + 102 + j);
            rv34_gen_vlc(rv34_table_inter_thirdpat [i][j], OTHERBLK_VLC_SIZE, &inter_vlcs[i].third_pattern[j],  NULL, i*12 + 104 + j);
        }
        rv34_gen_vlc(rv34_inter_coeff[i], COEFF_VLC_SIZE, &inter_vlcs[i].coefficient, NULL, i*12 + 106);
    }
}

/** @} */ // vlc group

//#define _test_idct_
/**
 * @defgroup transform RV30/40 inverse transform functions
 * @{
 */
#if 0
#ifndef __commit_out_idct_dequant__

#ifdef _remove_block_

static void rv34_amba_inv_transform_y(DCTELEM *block){

    int i;
    int z0, z1, z2, z3;
    int t0,t1;
    int temp[16];

    for(i=0; i<4; i++){
        t0=block[i];t1=block[i+32];

        z0= t0+t1;
        z0+=(z0<<3)+(z0<<2);
        z1= (t0-t1);
        z1+=(z1<<3)+(z1<<2);

        t0=block[i+16];t1=block[i+48];
        z2=  (t0<<3)-t0- (t1<<4)-t1;
        z3=  (t0<<4)+t0 + (t1<<3)-t1;

        temp[(i<<2)]= z0+z3;
        temp[(i<<2)+1]= z1+z2;
        temp[(i<<2)+2]= z1-z2;
        temp[(i<<2)+3]= z0-z3;
    }


    for(i=0; i<4; i++){
        t0=temp[i];t1=temp[i+8];

        z0= (t0+t1);
        z0+= (z0<<3)+(z0<<2) + 0x200;

        z1= (t0-t1);
        z1+=(z1<<3)+(z1<<2)+0x200;

        t0=temp[i+4];t1=temp[i+12];

        z2= (t0<<3)-t0-(t1<<4)-t1;
        z3= (t0<<4)+t0 + (t1<<3)-t1;

        block[(i<<4)+0]= (z0 + z3)>>10;
        block[(i<<4)+1]= (z1 + z2)>>10;
        block[(i<<4)+2]= (z1 - z2)>>10;
        block[(i<<4)+3]= (z0 - z3)>>10;
    }

}



static void rv34_amba_inv_transform_uv(DCTELEM *block){

    int i;
    int z0, z1, z2, z3;
    int t0,t1;
    int temp[16];

    for(i=0; i<4; i++){
        t0=block[i<<1];t1=block[(i<<1)+32];

        z0= t0+t1;
        z0+=(z0<<3)+(z0<<2);
        z1= (t0-t1);
        z1+=(z1<<3)+(z1<<2);

        t0=block[(i<<1)+16];t1=block[(i<<1)+48];
        z2=  (t0<<3)-t0- (t1<<4)-t1;
        z3=  (t0<<4)+t0 + (t1<<3)-t1;

        temp[(i<<2)]= z0+z3;
        temp[(i<<2)+1]= z1+z2;
        temp[(i<<2)+2]= z1-z2;
        temp[(i<<2)+3]= z0-z3;
    }


    for(i=0; i<4; i++){
        t0=temp[i];t1=temp[i+8];

        z0= (t0+t1);
        z0+= (z0<<3)+(z0<<2) + 0x200;

        z1= (t0-t1);
        z1+=(z1<<3)+(z1<<2)+0x200;

        t0=temp[i+4];t1=temp[i+12];

        z2= (t0<<3)-t0-(t1<<4)-t1;
        z3= (t0<<4)+t0 + (t1<<3)-t1;

        block[(i<<4)]= (z0 + z3)>>10;
        block[(i<<4)+2]= (z1 + z2)>>10;
        block[(i<<4)+4]= (z1 - z2)>>10;
        block[(i<<4)+6]= (z0 - z3)>>10;
    }

}

#else

static av_always_inline void rv34_row_transform(int temp[16], DCTELEM *block)
{
    int i;
    int z0,z1,z2,z3;
    int t0,t1;

    for(i=0; i<4; i++){
        z0= (block[i] +    block[i+16]);
        z0+=(z0<<3)+(z0<<2);
        z1= (block[i] -    block[i+16]);
        z1+=(z1<<3)+(z1<<2);
        t0=block[i+8];t1=block[i+24];
        z2=  (t0<<3)-t0 ;
        z2-= (t1<<4)+t1;
        z3=  (t0<<4)+t0 ;
        z3+=  (t1<<3)-t1;

        temp[(i<<2)]= z0+z3;
        temp[(i<<2)+1]= z1+z2;
        temp[(i<<2)+2]= z1-z2;
        temp[(i<<2)+3]= z0-z3;
    }
}

/**
 * Real Video 3.0/4.0 inverse transform
 * Code is almost the same as in SVQ3, only scaling is different.
 */
static void rv34_amba_inv_transform(DCTELEM *block){

    int i;
    int z0, z1, z2, z3;
    int t0,t1;
    int temp[16];

    for(i=0; i<4; i++){
        t0=block[i];t1=block[i+16];

        z0= t0+t1;
        z0+=(z0<<3)+(z0<<2);
        z1= (t0-t1);
        z1+=(z1<<3)+(z1<<2);

        t0=block[i+8];t1=block[i+24];
        z2=  (t0<<3)-t0- (t1<<4)-t1;
        z3=  (t0<<4)+t0 + (t1<<3)-t1;

        temp[(i<<2)]= z0+z3;
        temp[(i<<2)+1]= z1+z2;
        temp[(i<<2)+2]= z1-z2;
        temp[(i<<2)+3]= z0-z3;
    }


    for(i=0; i<4; i++){
        t0=temp[i];t1=temp[i+8];

        z0= (t0+t1);
        z0+= (z0<<3)+(z0<<2) + 0x200;

        z1= (t0-t1);
        z1+=(z1<<3)+(z1<<2)+0x200;

        t0=temp[i+4];t1=temp[i+12];

        z2= (t0<<3)-t0-(t1<<4)-t1;
        z3= (t0<<4)+t0 + (t1<<3)-t1;

        block[(i<<3)+0]= (z0 + z3)>>10;
        block[(i<<3)+1]= (z1 + z2)>>10;
        block[(i<<3)+2]= (z1 - z2)>>10;
        block[(i<<3)+3]= (z0 - z3)>>10;
    }

}

static  void rv34_amba_inv_transform_new_y(DCTELEM *block,DCTELEM *des){
    int temp[16];
    int i;

    rv34_row_transform(temp, block);

    for(i=0; i<4; i++){
        const int z0= 13*(temp[4*0+i] +    temp[4*2+i]) + 0x200;
        const int z1= 13*(temp[4*0+i] -    temp[4*2+i]) + 0x200;
        const int z2=  7* temp[4*1+i] - 17*temp[4*3+i];
        const int z3= 17* temp[4*1+i] +  7*temp[4*3+i];

        des[i*16+0]= (z0 + z3)>>10;
        des[i*16+1]= (z1 + z2)>>10;
        des[i*16+2]= (z1 - z2)>>10;
        des[i*16+3]= (z0 - z3)>>10;
    }

}

static void rv34_amba_inv_transform_new_uv(DCTELEM *block,DCTELEM *des){
    int temp[16];
    int i;

    rv34_row_transform(temp, block);

    for(i=0; i<4; i++){
        const int z0= 13*(temp[4*0+i] +    temp[4*2+i]) + 0x200;
        const int z1= 13*(temp[4*0+i] -    temp[4*2+i]) + 0x200;
        const int z2=  7* temp[4*1+i] - 17*temp[4*3+i];
        const int z3= 17* temp[4*1+i] +  7*temp[4*3+i];

        des[i*16+0]= (z0 + z3)>>10;
        des[i*16+2]= (z1 + z2)>>10;
        des[i*16+4]= (z1 - z2)>>10;
        des[i*16+6]= (z0 - z3)>>10;
    }

}
#endif


/**
 * RealVideo 3.0/4.0 inverse transform for DC block
 *
 * Code is almost the same as rv34_amba_inv_transform()
 * but final coefficients are multiplied by 1.5 and have no rounding.
 */
static void rv34_amba_inv_transform_noround(DCTELEM *block){

    int i;
    int z0, z1, z2, z3;
    int t0,t1;
    int temp[16];

    for(i=0; i<4; i++){
        t0=block[i];t1=block[i+16];

        z0= t0+t1;
        z0+=(z0<<3)+(z0<<2);
        z1= (t0-t1);
        z1+=(z1<<3)+(z1<<2);

        t0=block[i+8];t1=block[i+24];
        z2=  (t0<<3)-t0- (t1<<4)-t1;
        z3=  (t0<<4)+t0 + (t1<<3)-t1;

        temp[(i<<2)]= z0+z3;
        temp[(i<<2)+1]= z1+z2;
        temp[(i<<2)+2]= z1-z2;
        temp[(i<<2)+3]= z0-z3;
    }


    for(i=0; i<4; i++){
        t0=temp[i];t1=temp[i+8];

        z0= (t0+t1);
        z0+= (z0<<3)+(z0<<2) + 0x200;

        z1= (t0-t1);
        z1+=(z1<<3)+(z1<<2)+0x200;

        t0=temp[i+4];t1=temp[i+12];

        z2= (t0<<3)-t0-(t1<<4)-t1;
        z3= (t0<<4)+t0 + (t1<<3)-t1;

        block[(i<<3)+0]= ((z0 + z3)*3)>>11;
        block[(i<<3)+1]= ((z1 + z2)*3)>>11;
        block[(i<<3)+2]= ((z1 - z2)*3)>>11;
        block[(i<<3)+3]= ((z0 - z3)*3)>>11;
    }

}

#endif

#endif

#ifndef __commit_out_idct_dequant__

static inline void rv34_row_transform(int temp[16], DCTELEM *block)
{
    int i;

    for(i=0; i<4; i++){
        const int z0= 13*(block[i+8*0] +    block[i+8*2]);
        const int z1= 13*(block[i+8*0] -    block[i+8*2]);
        const int z2=  7* block[i+8*1] - 17*block[i+8*3];
        const int z3= 17* block[i+8*1] +  7*block[i+8*3];

        temp[4*i+0]= z0+z3;
        temp[4*i+1]= z1+z2;
        temp[4*i+2]= z1-z2;
        temp[4*i+3]= z0-z3;
    }
}
/**
 * Real Video 3.0/4.0 inverse transform
 * Code is almost the same as in SVQ3, only scaling is different.
 */
static void rv34_amba_inv_transform(DCTELEM *block){
    int temp[16];
    int i;

    rv34_row_transform(temp, block);

    for(i=0; i<4; i++){
        const int z0= 13*(temp[4*0+i] +    temp[4*2+i]) + 0x200;
        const int z1= 13*(temp[4*0+i] -    temp[4*2+i]) + 0x200;
        const int z2=  7* temp[4*1+i] - 17*temp[4*3+i];
        const int z3= 17* temp[4*1+i] +  7*temp[4*3+i];

        block[i*8+0]= (z0 + z3)>>10;
        block[i*8+1]= (z1 + z2)>>10;
        block[i*8+2]= (z1 - z2)>>10;
        block[i*8+3]= (z0 - z3)>>10;
    }

}

/*
static  void rv34_row_transform(int temp[16], DCTELEM *block)
{
    int i;

    for(i=0; i<4; i++){
        int z0= (block[i+8*0] +    block[i+8*2]);
        z0+=(z0<<3)+(z0<<2);
        int z1= (block[i+8*0] -    block[i+8*2]);
        z1+=(z1<<3)+(z1<<2);
        int z2=  (block[i+8*1]<<3)-block[i+8*1] - block[i+8*3]-(block[i+8*3]<<4);
        int z3= block[i+8*1] +(block[i+8*1]<<4)+  (block[i+8*3]<<3)-block[i+8*3];

        temp[(i<<2)+0]= z0+z3;
        temp[(i<<2)+1]= z1+z2;
        temp[(i<<2)+2]= z1-z2;
        temp[(i<<2)+3]= z0-z3;
    }
}*/

#ifdef _remove_block_

static av_always_inline void rv34_row_transform_y(int temp[16], DCTELEM *block)
{
    int i;

    for(i=0; i<4; i++){
        const int z0= 13*(block[i+16*0] +    block[i+16*2]);
        const int z1= 13*(block[i+16*0] -    block[i+16*2]);
        const int z2=  7* block[i+16*1] - 17*block[i+16*3];
        const int z3= 17* block[i+16*1] +  7*block[i+16*3];

        temp[4*i+0]= z0+z3;
        temp[4*i+1]= z1+z2;
        temp[4*i+2]= z1-z2;
        temp[4*i+3]= z0-z3;
    }
}

static av_always_inline void rv34_row_transform_uv(int temp[16], DCTELEM *block)
{
    int i;

    for(i=0; i<4; i++){
        const int z0= 13*(block[i*2+16*0] +    block[i*2+16*2]);
        const int z1= 13*(block[i*2+16*0] -    block[i*2+16*2]);
        const int z2=  7* block[i*2+16*1] - 17*block[i*2+16*3];
        const int z3= 17* block[i*2+16*1] +  7*block[i*2+16*3];

        temp[4*i+0]= z0+z3;
        temp[4*i+1]= z1+z2;
        temp[4*i+2]= z1-z2;
        temp[4*i+3]= z0-z3;
    }
}
static void rv34_amba_inv_transform_y(DCTELEM *block){
    int temp[16];
    int i;

    rv34_row_transform_y(temp, block);

    for(i=0; i<4; i++){
        const int z0= 13*(temp[4*0+i] +    temp[4*2+i]) + 0x200;
        const int z1= 13*(temp[4*0+i] -    temp[4*2+i]) + 0x200;
        const int z2=  7* temp[4*1+i] - 17*temp[4*3+i];
        const int z3= 17* temp[4*1+i] +  7*temp[4*3+i];

        block[i*16+0]= (z0 + z3)>>10;
        block[i*16+1]= (z1 + z2)>>10;
        block[i*16+2]= (z1 - z2)>>10;
        block[i*16+3]= (z0 - z3)>>10;
    }

}
#ifndef _d_new_parallel_
static void rv34_amba_inv_transform_uv(DCTELEM *block){
    int temp[16];
    int i;

    rv34_row_transform_uv(temp, block);

    for(i=0; i<4; i++){
        const int z0= 13*(temp[4*0+i] +    temp[4*2+i]) + 0x200;
        const int z1= 13*(temp[4*0+i] -    temp[4*2+i]) + 0x200;
        const int z2=  7* temp[4*1+i] - 17*temp[4*3+i];
        const int z3= 17* temp[4*1+i] +  7*temp[4*3+i];

        block[i*16+0]= (z0 + z3)>>10;
        block[i*16+2]= (z1 + z2)>>10;
        block[i*16+4]= (z1 - z2)>>10;
        block[i*16+6]= (z0 - z3)>>10;
    }

}
#endif
#else


static void rv34_amba_inv_transform_new_y(DCTELEM *block,DCTELEM *des){
    int temp[16];
    int i;

    rv34_row_transform(temp, block);

    for(i=0; i<4; i++){
        const int z0= 13*(temp[4*0+i] +    temp[4*2+i]) + 0x200;
        const int z1= 13*(temp[4*0+i] -    temp[4*2+i]) + 0x200;
        const int z2=  7* temp[4*1+i] - 17*temp[4*3+i];
        const int z3= 17* temp[4*1+i] +  7*temp[4*3+i];

        des[i*16+0]= (z0 + z3)>>10;
        des[i*16+1]= (z1 + z2)>>10;
        des[i*16+2]= (z1 - z2)>>10;
        des[i*16+3]= (z0 - z3)>>10;
    }

}

static void rv34_amba_inv_transform_new_uv(DCTELEM *block,DCTELEM *des){
    int temp[16];
    int i;

    rv34_row_transform(temp, block);

    for(i=0; i<4; i++){
        const int z0= 13*(temp[4*0+i] +    temp[4*2+i]) + 0x200;
        const int z1= 13*(temp[4*0+i] -    temp[4*2+i]) + 0x200;
        const int z2=  7* temp[4*1+i] - 17*temp[4*3+i];
        const int z3= 17* temp[4*1+i] +  7*temp[4*3+i];

        des[i*16+0]= (z0 + z3)>>10;
        des[i*16+2]= (z1 + z2)>>10;
        des[i*16+4]= (z1 - z2)>>10;
        des[i*16+6]= (z0 - z3)>>10;
    }

}
#endif


/**
 * RealVideo 3.0/4.0 inverse transform for DC block
 *
 * Code is almost the same as rv34_amba_inv_transform()
 * but final coefficients are multiplied by 1.5 and have no rounding.
 */
static void rv34_amba_inv_transform_noround(DCTELEM *block){
    int temp[16];
    int i;

    rv34_row_transform(temp, block);

    for(i=0; i<4; i++){
        const int z0= 13*(temp[4*0+i] +    temp[4*2+i]);
        const int z1= 13*(temp[4*0+i] -    temp[4*2+i]);
        const int z2=  7* temp[4*1+i] - 17*temp[4*3+i];
        const int z3= 17* temp[4*1+i] +  7*temp[4*3+i];

        block[i*8+0]= ((z0 + z3)*3)>>11;
        block[i*8+1]= ((z1 + z2)*3)>>11;
        block[i*8+2]= ((z1 - z2)*3)>>11;
        block[i*8+3]= ((z0 - z3)*3)>>11;
    }

}
#endif

//#endif

/** @} */ // transform


/**
 * @defgroup block RV30/40 4x4 block decoding functions
 * @{
 */

/**
 * Decode coded block pattern.
 */
static int rv34_decode_cbp(GetBitContext *gb, RV34VLC *vlc, int table)
{
    int pattern, code, cbp=0;
    int ones;
    static const int cbp_masks[3] = {0x100000, 0x010000, 0x110000};
    static const int shifts[4] = { 0, 2, 8, 10 };
    const int *curshift = shifts;
    int i, t, mask;

    code = get_vlc2(gb, vlc->cbppattern[table].table, 9, 2);
    pattern = code & 0xF;
    code >>= 4;

    ones = rv34_count_ones[pattern];

    for(mask = 8; mask; mask >>= 1, curshift++){
        if(pattern & mask)
            cbp |= get_vlc2(gb, vlc->cbp[table][ones].table, vlc->cbp[table][ones].bits, 1) << curshift[0];
    }

    for(i = 0; i < 4; i++){
        t = modulo_three_table[code][i];
        if(t == 1)
            cbp |= cbp_masks[get_bits1(gb)] << i;
        if(t == 2)
            cbp |= cbp_masks[2] << i;
    }
    return cbp;
}

/**
 * Get one coefficient value from the bistream and store it.
 */
static inline void decode_coeff(DCTELEM *dst, int coef, int esc, GetBitContext *gb, VLC* vlc)
{
    if(coef){
        if(coef == esc){
            coef = get_vlc2(gb, vlc->table, 9, 2);
            if(coef > 23){
                coef -= 23;
                coef = 22 + ((1 << coef) | get_bits(gb, coef));
            }
            coef += esc;
        }
        if(get_bits1(gb))
            coef = -coef;
        *dst = coef;
    }
}


#ifdef _remove_block_
static inline void decode_subblock_y(DCTELEM *dst, int code, const int is_block2, GetBitContext *gb, VLC *vlc)
{
    int coeffs[4];

    coeffs[0] = modulo_three_table[code][0];
    coeffs[1] = modulo_three_table[code][1];
    coeffs[2] = modulo_three_table[code][2];
    coeffs[3] = modulo_three_table[code][3];
    decode_coeff(dst  , coeffs[0], 3, gb, vlc);
    if(is_block2){
        decode_coeff(dst+16, coeffs[1], 2, gb, vlc);
        decode_coeff(dst+1, coeffs[2], 2, gb, vlc);
    }else{
        decode_coeff(dst+1, coeffs[1], 2, gb, vlc);
        decode_coeff(dst+16, coeffs[2], 2, gb, vlc);
    }
    decode_coeff(dst+17, coeffs[3], 2, gb, vlc);
}

static inline void decode_subblock_uv(DCTELEM *dst, int code, const int is_block2, GetBitContext *gb, VLC *vlc)
{
    int coeffs[4];

    coeffs[0] = modulo_three_table[code][0];
    coeffs[1] = modulo_three_table[code][1];
    coeffs[2] = modulo_three_table[code][2];
    coeffs[3] = modulo_three_table[code][3];
    decode_coeff(dst  , coeffs[0], 3, gb, vlc);
    if(is_block2){
        decode_coeff(dst+16, coeffs[1], 2, gb, vlc);
        decode_coeff(dst+2, coeffs[2], 2, gb, vlc);
    }else{
        decode_coeff(dst+2, coeffs[1], 2, gb, vlc);
        decode_coeff(dst+16, coeffs[2], 2, gb, vlc);
    }
    decode_coeff(dst+18, coeffs[3], 2, gb, vlc);
}

/**
 * Decode coefficients for 4x4 block.
 *
 * This is done by filling 2x2 subblocks with decoded coefficients
 * in this order (the same for subblocks and subblock coefficients):
 *  o--o
 *    /
 *   /
 *  o--o
 */

static inline void rv34_amba_decode_block_y(DCTELEM *dst, GetBitContext *gb, RV34VLC *rvlc, int fc, int sc)
{
    int code, pattern;

    code = get_vlc2(gb, rvlc->first_pattern[fc].table, 9, 2);

    pattern = code & 0x7;

    code >>= 3;
    decode_subblock_y(dst, code, 0, gb, &rvlc->coefficient);

    if(pattern & 4){
        code = get_vlc2(gb, rvlc->second_pattern[sc].table, 9, 2);
        decode_subblock_y(dst + 2, code, 0, gb, &rvlc->coefficient);
    }
    if(pattern & 2){ // Looks like coefficients 1 and 2 are swapped for this block
        code = get_vlc2(gb, rvlc->second_pattern[sc].table, 9, 2);
        decode_subblock_y(dst + 32, code, 1, gb, &rvlc->coefficient);
    }
    if(pattern & 1){
        code = get_vlc2(gb, rvlc->third_pattern[sc].table, 9, 2);
        decode_subblock_y(dst + 34, code, 0, gb, &rvlc->coefficient);
    }

}

static inline void rv34_amba_decode_block_uv(DCTELEM *dst, GetBitContext *gb, RV34VLC *rvlc, int fc, int sc)
{
    int code, pattern;

    code = get_vlc2(gb, rvlc->first_pattern[fc].table, 9, 2);

    pattern = code & 0x7;

    code >>= 3;
    decode_subblock_uv(dst, code, 0, gb, &rvlc->coefficient);

    if(pattern & 4){
        code = get_vlc2(gb, rvlc->second_pattern[sc].table, 9, 2);
        decode_subblock_uv(dst + 4, code, 0, gb, &rvlc->coefficient);
    }
    if(pattern & 2){ // Looks like coefficients 1 and 2 are swapped for this block
        code = get_vlc2(gb, rvlc->second_pattern[sc].table, 9, 2);
        decode_subblock_uv(dst + 32, code, 1, gb, &rvlc->coefficient);
    }
    if(pattern & 1){
        code = get_vlc2(gb, rvlc->third_pattern[sc].table, 9, 2);
        decode_subblock_uv(dst + 36, code, 0, gb, &rvlc->coefficient);
    }

}

#endif
/**
 * Decode 2x2 subblock of coefficients.
 */
static inline void decode_subblock(DCTELEM *dst, int code, const int is_block2, GetBitContext *gb, VLC *vlc)
{
    int coeffs[4];

    coeffs[0] = modulo_three_table[code][0];
    coeffs[1] = modulo_three_table[code][1];
    coeffs[2] = modulo_three_table[code][2];
    coeffs[3] = modulo_three_table[code][3];
    decode_coeff(dst  , coeffs[0], 3, gb, vlc);
    if(is_block2){
        decode_coeff(dst+8, coeffs[1], 2, gb, vlc);
        decode_coeff(dst+1, coeffs[2], 2, gb, vlc);
    }else{
        decode_coeff(dst+1, coeffs[1], 2, gb, vlc);
        decode_coeff(dst+8, coeffs[2], 2, gb, vlc);
    }
    decode_coeff(dst+9, coeffs[3], 2, gb, vlc);
}

/**
 * Decode coefficients for 4x4 block.
 *
 * This is done by filling 2x2 subblocks with decoded coefficients
 * in this order (the same for subblocks and subblock coefficients):
 *  o--o
 *    /
 *   /
 *  o--o
 */

static inline void rv34_amba_decode_block(DCTELEM *dst, GetBitContext *gb, RV34VLC *rvlc, int fc, int sc)
{
    int code, pattern;

    code = get_vlc2(gb, rvlc->first_pattern[fc].table, 9, 2);

    pattern = code & 0x7;

    code >>= 3;
    decode_subblock(dst, code, 0, gb, &rvlc->coefficient);

    if(pattern & 4){
        code = get_vlc2(gb, rvlc->second_pattern[sc].table, 9, 2);
        decode_subblock(dst + 2, code, 0, gb, &rvlc->coefficient);
    }
    if(pattern & 2){ // Looks like coefficients 1 and 2 are swapped for this block
        code = get_vlc2(gb, rvlc->second_pattern[sc].table, 9, 2);
        decode_subblock(dst + 8*2, code, 1, gb, &rvlc->coefficient);
    }
    if(pattern & 1){
        code = get_vlc2(gb, rvlc->third_pattern[sc].table, 9, 2);
        decode_subblock(dst + 8*2+2, code, 0, gb, &rvlc->coefficient);
    }

}

/**
 * Dequantize ordinary 4x4 block.
 * @todo optimize
 */
#ifndef __commit_out_idct_dequant__
#ifdef _remove_block_
static inline void rv34_amba_dequant4x4_y(DCTELEM *block, int Qdc, int Q)
{
    int i, j;

    block[0] = (block[0] * Qdc + 8) >> 4;
    for(i = 0; i < 4; i++)
        for(j = !i; j < 4; j++)
            block[ (i<<4)+j ] = (block[(i<<4)+j] * Q + 8) >> 4;
}
static inline void rv34_amba_dequant4x4_uv(DCTELEM *block, int Qdc, int Q)
{
    int i, j;

    block[0] = (block[0] * Qdc + 8) >> 4;
    for(i = 0; i < 4; i++)
        for(j = (!i)<<1; j < 8; j+=2)
            block[ (i<<4)+j] = (block[ (i<<4)+j] * Q + 8) >> 4;
}
#endif
static inline void rv34_amba_dequant4x4(DCTELEM *block, int Qdc, int Q)
{
    int i, j;

    block[0] = (block[0] * Qdc + 8) >> 4;
    for(i = 0; i < 4; i++)
        for(j = !i; j < 4; j++)
            block[j + i*8] = (block[j + i*8] * Q + 8) >> 4;
}


/**
 * Dequantize 4x4 block of DC values for 16x16 macroblock.
 * @todo optimize
 */
static inline void rv34_amba_dequant4x4_16x16(DCTELEM *block, int Qdc, int Q)
{
    int i;

    for(i = 0; i < 3; i++)
         block[rv34_dezigzag[i]] = (block[rv34_dezigzag[i]] * Qdc + 8) >> 4;
    for(; i < 16; i++)
         block[rv34_dezigzag[i]] = (block[rv34_dezigzag[i]] * Q + 8) >> 4;
}
 #endif
/** @} */ //block functions


/**
 * @defgroup bitstream RV30/40 bitstream parsing
 * @{
 */

/**
 * Decode starting slice position.
 * @todo Maybe replace with ff_h263_decode_mba() ?
 */
int ff_rv34_amba_get_start_offset(GetBitContext *gb, int mb_size)
{
    int i;
    for(i = 0; i < 5; i++)
        if(rv34_mb_max_sizes[i] >= mb_size - 1)
            break;
    return rv34_mb_bits_sizes[i];
}

/**
 * Select VLC set for decoding from current quantizer, modifier and frame type.
 */
static inline RV34VLC* choose_vlc_set(int quant, int mod, int type)
{
    if(mod == 2 && quant < 19) quant += 10;
    else if(mod && quant < 26) quant += 5;
    return type ? &inter_vlcs[rv34_quant_to_vlc_set[1][av_clip(quant, 0, 30)]]
                : &intra_vlcs[rv34_quant_to_vlc_set[0][av_clip(quant, 0, 30)]];
}

/**
 * Decode quantizer difference and return modified quantizer.
 */
static inline int rv34_decode_dquant(GetBitContext *gb, int quant)
{
    if(get_bits1(gb))
        return rv34_dquant_tab[get_bits1(gb)][quant];
    else
        return get_bits(gb, 5);
}

/** @} */ //bitstream functions

/**
 * @defgroup mv motion vector related code (prediction, reconstruction, motion compensation)
 * @{
 */

/** macroblock partition width in 8x8 blocks */
static const uint8_t part_sizes_w[RV34_MB_TYPES] = { 2, 2, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2 };

/** macroblock partition height in 8x8 blocks */
static const uint8_t part_sizes_h[RV34_MB_TYPES] = { 2, 2, 2, 1, 2, 2, 2, 2, 1, 2, 2, 2 };

/** availability index for subblocks */
static const uint8_t avail_indexes[4] = { 6, 7, 10, 11 };

/**
 * motion vector prediction
 *
 * Motion prediction performed for the block by using median prediction of
 * motion vectors from the left, top and right top blocks but in corner cases
 * some other vectors may be used instead.
 */
static void rv34_pred_mv(RV34AmbaDecContext *r, int block_type, int subblock_no, int dmv_no)
{
    MpegEncContext *s = &r->s;
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int A[2] = {0}, B[2], C[2];
    int i, j;
    int mx, my;
    int avail_index = avail_indexes[subblock_no];
    int c_off = part_sizes_w[block_type];

    mv_pos += (subblock_no & 1) + (subblock_no >> 1)*s->b8_stride;
    if(subblock_no == 3)
        c_off = -1;

    if(r->avail_cache[avail_index - 1]){
        A[0] = s->current_picture_ptr->motion_val[0][mv_pos-1][0];
        A[1] = s->current_picture_ptr->motion_val[0][mv_pos-1][1];
    }
    if(r->avail_cache[avail_index - 4]){
        B[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride][0];
        B[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride][1];
    }else{
        B[0] = A[0];
        B[1] = A[1];
    }
    if(!r->avail_cache[avail_index - 4 + c_off]){
        if(r->avail_cache[avail_index - 4] && (r->avail_cache[avail_index - 1] || r->rv30)){
            C[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride-1][0];
            C[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride-1][1];
        }else{
            C[0] = A[0];
            C[1] = A[1];
        }
    }else{
        C[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride+c_off][0];
        C[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride+c_off][1];
    }
    mx = mid_pred(A[0], B[0], C[0]);
    my = mid_pred(A[1], B[1], C[1]);
    mx += r->dmv[dmv_no][0];
    my += r->dmv[dmv_no][1];
    for(j = 0; j < part_sizes_h[block_type]; j++){
        for(i = 0; i < part_sizes_w[block_type]; i++){
            s->current_picture_ptr->motion_val[0][mv_pos + i + j*s->b8_stride][0] = mx;
            s->current_picture_ptr->motion_val[0][mv_pos + i + j*s->b8_stride][1] = my;
        }
    }
}

#define GET_PTS_DIFF(a, b) ((a - b + 8192) & 0x1FFF)

/**
 * Calculate motion vector component that should be added for direct blocks.
 */
static int calc_add_mv(RV34AmbaDecContext *r, int dir, int val)
{
    int refdist = GET_PTS_DIFF(r->next_pts, r->last_pts);
    int dist = dir ? -GET_PTS_DIFF(r->next_pts, r->cur_pts) : GET_PTS_DIFF(r->cur_pts, r->last_pts);
    int mul;

    if(!refdist) return 0;
    mul = (dist << 14) / refdist;
    return (val * mul + 0x2000) >> 14;
}

/**
 * Predict motion vector for B-frame macroblock.
 */
static inline void rv34_pred_b_vector(int A[2], int B[2], int C[2],
                                      int A_avail, int B_avail, int C_avail,
                                      int *mx, int *my)
{
    if(A_avail + B_avail + C_avail != 3){
        *mx = A[0] + B[0] + C[0];
        *my = A[1] + B[1] + C[1];
        if(A_avail + B_avail + C_avail == 2){
            *mx /= 2;
            *my /= 2;
        }
    }else{
        *mx = mid_pred(A[0], B[0], C[0]);
        *my = mid_pred(A[1], B[1], C[1]);
    }
}

/**
 * motion vector prediction for B-frames
 */
static void rv34_pred_mv_b(RV34AmbaDecContext *r, int block_type, int dir)
{
    MpegEncContext *s = &r->s;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int A[2], B[2], C[2];
    int has_A = 0, has_B = 0, has_C = 0;
    int mx, my;
    int i, j;
    Picture *cur_pic = s->current_picture_ptr;
    const int mask = dir ? MB_TYPE_L1 : MB_TYPE_L0;
    int type = cur_pic->mb_type[mb_pos];

    memset(A, 0, sizeof(A));
    memset(B, 0, sizeof(B));
    memset(C, 0, sizeof(C));
    if((r->avail_cache[6-1] & type) & mask){
        A[0] = cur_pic->motion_val[dir][mv_pos - 1][0];
        A[1] = cur_pic->motion_val[dir][mv_pos - 1][1];
        has_A = 1;
    }
    if((r->avail_cache[6-4] & type) & mask){
        B[0] = cur_pic->motion_val[dir][mv_pos - s->b8_stride][0];
        B[1] = cur_pic->motion_val[dir][mv_pos - s->b8_stride][1];
        has_B = 1;
    }
    if(r->avail_cache[6-4] && ((r->avail_cache[6-2] & type) & mask)){
        C[0] = cur_pic->motion_val[dir][mv_pos - s->b8_stride + 2][0];
        C[1] = cur_pic->motion_val[dir][mv_pos - s->b8_stride + 2][1];
        has_C = 1;
    }else if((s->mb_x+1) == s->mb_width && ((r->avail_cache[6-5] & type) & mask)){
        C[0] = cur_pic->motion_val[dir][mv_pos - s->b8_stride - 1][0];
        C[1] = cur_pic->motion_val[dir][mv_pos - s->b8_stride - 1][1];
        has_C = 1;
    }

    rv34_pred_b_vector(A, B, C, has_A, has_B, has_C, &mx, &my);

    mx += r->dmv[dir][0];
    my += r->dmv[dir][1];

    for(j = 0; j < 2; j++){
        for(i = 0; i < 2; i++){
            cur_pic->motion_val[dir][mv_pos + i + j*s->b8_stride][0] = mx;
            cur_pic->motion_val[dir][mv_pos + i + j*s->b8_stride][1] = my;
        }
    }
    if(block_type == RV34_MB_B_BACKWARD || block_type == RV34_MB_B_FORWARD){
        ZERO8x2(cur_pic->motion_val[!dir][mv_pos], s->b8_stride);
    }
}

/**
 * motion vector prediction - RV3 version
 */
static void rv34_pred_mv_rv3(RV34AmbaDecContext *r, int block_type, int dir)
{
    MpegEncContext *s = &r->s;
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int A[2] = {0}, B[2], C[2];
    int i, j, k;
    int mx, my;
    int avail_index = avail_indexes[0];

    if(r->avail_cache[avail_index - 1]){
        A[0] = s->current_picture_ptr->motion_val[0][mv_pos-1][0];
        A[1] = s->current_picture_ptr->motion_val[0][mv_pos-1][1];
    }
    if(r->avail_cache[avail_index - 4]){
        B[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride][0];
        B[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride][1];
    }else{
        B[0] = A[0];
        B[1] = A[1];
    }
    if(!r->avail_cache[avail_index - 4 + 2]){
        if(r->avail_cache[avail_index - 4] && (r->avail_cache[avail_index - 1])){
            C[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride-1][0];
            C[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride-1][1];
        }else{
            C[0] = A[0];
            C[1] = A[1];
        }
    }else{
        C[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride+2][0];
        C[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride+2][1];
    }
    mx = mid_pred(A[0], B[0], C[0]);
    my = mid_pred(A[1], B[1], C[1]);
    mx += r->dmv[0][0];
    my += r->dmv[0][1];
    for(j = 0; j < 2; j++){
        for(i = 0; i < 2; i++){
            for(k = 0; k < 2; k++){
                s->current_picture_ptr->motion_val[k][mv_pos + i + j*s->b8_stride][0] = mx;
                s->current_picture_ptr->motion_val[k][mv_pos + i + j*s->b8_stride][1] = my;
            }
        }
    }
}

static const int chroma_coeffs[3] = { 0, 3, 5 };

/** number of motion vectors in each macroblock type */
static const int num_mvs[RV34_MB_TYPES] = { 0, 0, 1, 4, 1, 1, 0, 0, 2, 2, 2, 1 };


/** @} */ // mv group

/**
 * @defgroup recons Macroblock reconstruction functions
 * @{
 */
/** mapping of RV30/40 intra prediction types to standard H.264 types */
static const int ittrans[9] = {
 DC_PRED, VERT_PRED, HOR_PRED, DIAG_DOWN_RIGHT_PRED, DIAG_DOWN_LEFT_PRED,
 VERT_RIGHT_PRED, VERT_LEFT_PRED, HOR_UP_PRED, HOR_DOWN_PRED,
};

/** mapping of RV30/40 intra 16x16 prediction types to standard H.264 types */
static const int ittrans16[4] = {
 DC_PRED8x8, VERT_PRED8x8, HOR_PRED8x8, PLANE_PRED8x8,
};


static inline int adjust_pred16(int itype, int up, int left)
{
    if(!up && !left)
        itype = DC_128_PRED8x8;
    else if(!up){
        if(itype == PLANE_PRED8x8)itype = HOR_PRED8x8;
        if(itype == VERT_PRED8x8) itype = HOR_PRED8x8;
        if(itype == DC_PRED8x8)   itype = LEFT_DC_PRED8x8;
    }else if(!left){
        if(itype == PLANE_PRED8x8)itype = VERT_PRED8x8;
        if(itype == HOR_PRED8x8)  itype = VERT_PRED8x8;
        if(itype == DC_PRED8x8)   itype = TOP_DC_PRED8x8;
    }
    return itype;
}


/**
 * @addtogroup recons
 * @{
 */
/**
 * mask for retrieving all bits in coded block pattern
 * corresponding to one 8x8 block
 */
#define LUMA_CBP_BLOCK_MASK 0x33

#define U_CBP_MASK 0x0F0000
#define V_CBP_MASK 0xF00000


static int is_mv_diff_gt_3(int16_t (*motion_val)[2], int step)
{
    int d;
    d = motion_val[0][0] - motion_val[-step][0];
    if(d < -3 || d > 3)
        return 1;
    d = motion_val[0][1] - motion_val[-step][1];
    if(d < -3 || d > 3)
        return 1;
    return 0;
}

static inline int slice_compare(SliceInfo *si1, SliceInfo *si2)
{
    return si1->type   != si2->type  ||
           si1->start  >= si2->start ||
           si1->width  != si2->width ||
           si1->height != si2->height||
           si1->pts    != si2->pts;
}

static int check_slice_end_amba(RV34AmbaDecContext *r, MpegEncContext *s)
{
    int bits;
    if(s->mb_y >= s->mb_height)
        return 1;
    if(!s->mb_num_left)
        return 1;
    if(r->s.mb_skip_run > 1)
        return 0;
    bits = r->bits - get_bits_count(&s->gb);
    if(bits < 0 || (bits < 8 && !show_bits(&s->gb, bits)))
        return 1;
    return 0;
}

#ifndef _d_new_parallel_

static int get_slice_offset(AVCodecContext *avctx, const uint8_t *buf, int n)
{
    if(avctx->slice_count) return avctx->slice_offset[n];
    else                   return AV_RL32(buf + n*8 - 4) == 1 ? AV_RL32(buf + n*8) :  AV_RB32(buf + n*8);
}

static int rv34_amba_set_deblock_coef(RV34AmbaDecContext *r)
{
    MpegEncContext *s = &r->s;
    int hmvmask = 0, vmvmask = 0, i, j;
    int midx = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int16_t (*motion_val)[2] = s->current_picture_ptr->motion_val[0][midx];
    for(j = 0; j < 16; j += 8){
        for(i = 0; i < 2; i++){
            if(is_mv_diff_gt_3(motion_val + i, 1))
                vmvmask |= 0x11 << (j + i*2);
            if((j || s->mb_y) && is_mv_diff_gt_3(motion_val + i, s->b8_stride))
                hmvmask |= 0x03 << (j + i*2);
        }
        motion_val += s->b8_stride;
    }
    if(s->first_slice_line)
        hmvmask &= ~0x000F;
    if(!s->mb_x)
        vmvmask &= ~0x1111;
    if(r->rv30){ //RV30 marks both subblocks on the edge for filtering
        vmvmask |= (vmvmask & 0x4444) >> 1;
        hmvmask |= (hmvmask & 0x0F00) >> 4;
        if(s->mb_x)
            r->deblock_coefs[s->mb_x - 1 + s->mb_y*s->mb_stride] |= (vmvmask & 0x1111) << 3;
        if(!s->first_slice_line)
            r->deblock_coefs[s->mb_x + (s->mb_y - 1)*s->mb_stride] |= (hmvmask & 0xF) << 12;
    }
    return hmvmask | vmvmask;
}

//multithread related

static inline MBLineMCIntraPred* new_mc_intrapred(int mbwidth, int mbheight)
{
    MBLineMCIntraPred* thiz=av_malloc(sizeof(MBLineMCIntraPred)+mbwidth*(sizeof(MBInfoMCIntraPred)));
    if(!thiz)
    {
        av_log(NULL,AV_LOG_ERROR,"*error* new_mc_intrapred: malloc buffer fail.\n");
        return NULL;
    }

    #ifdef __log_communicate_queue__
        static int cnt=0;
        av_log(NULL,AV_LOG_ERROR," %d new_mc_intrapred in.\n",cnt++);
    #endif

    thiz->mbwidth=mbwidth;
    thiz->mbheight=mbheight;
    thiz->pMBInfo=(MBInfoMCIntraPred*)((unsigned char*)thiz+sizeof(MBLineMCIntraPred));

    //pre alloc resources
    thiz->p_idct_result_base=av_malloc((mbwidth<<9)+15+(mbwidth<<8)+15);
    if(!thiz->p_idct_result_base)
    {
        av_log(NULL,AV_LOG_ERROR,"*error* new_mc_intrapred: malloc buffer fail 1.\n");
        return NULL;
    }

    thiz->p_idct_result_y=(DCTELEM*)((((unsigned int)thiz->p_idct_result_base)+15)&(~15));
    thiz->p_idct_result_uv=(DCTELEM*)((((unsigned int)thiz->p_idct_result_y)+(mbwidth<<9)+15)&(~15));

    //need optimize
    #ifdef _remove_block_
    memset(thiz->p_idct_result_y,0,mbwidth<<9);
    memset(thiz->p_idct_result_uv,0,mbwidth<<8);
    #endif
    thiz->stride=(mbwidth<<4);

    #ifdef __log_communicate_queue__
        av_log(NULL,AV_LOG_ERROR," new_mc_intrapred out.\n");
    #endif

    return thiz;
}

static inline void delete_mc_intrapred(MBLineMCIntraPred* p_mc_intrapred)
{
    if(!p_mc_intrapred)
        return;
    if(p_mc_intrapred->p_idct_result_base)
        av_free(p_mc_intrapred->p_idct_result_base);
    av_free(p_mc_intrapred);
}

static inline void process_mc(RV34AmbaDecContext* thiz,MBInfoMCIntraPred* pMB,int mbindex,int mbrow)
{
    MpegEncContext *s = &thiz->s;
    uint8_t *Y, *U, *V, *srcY, *srcU, *srcV;
    int dxy, mx, my, umx, umy, lx, ly, uvmx, uvmy, src_x, src_y, uvsrc_x, uvsrc_y;
    int cx,cy;
    int mv_x,mv_y;
//    int is16x16 = 1;
    int i=0;
    MBInfoMC* p_mc=&pMB->info.mc;

    Y = s->current_picture_ptr->data[0]+(mbindex <<4) +(mbrow<<4)*s->linesize;
    U = s->current_picture_ptr->data[1] +  (mbindex<<4) + (mbrow<<3) *s->uvlinesize;
    V = U+1;

    #ifdef __log_mv__
        char tstr[100];
        int use_em=0;
        uint8_t* ptf=s->last_picture_ptr->data[0];
        uint8_t* ptb=s->next_picture_ptr->data[0];
        uint8_t* ptfc=s->last_picture_ptr->data[1];
        uint8_t* ptbc=s->next_picture_ptr->data[1];
    #endif

    #ifdef _d_use_DSP_format_
    RVDEC_MB_MV_B_t* pbmv=thiz->pdspmv.b+mbrow*(s->mb_width)+mbindex;
    RVDEC_MB_MV_P_t* ppmv=thiz->pdspmv.p+mbrow*(s->mb_width)+mbindex;
    int it=0,offset=0;
    #endif

    #ifdef __log_mv_new__
    char lt[100];
    int rmv_x,rmv_y,rlx,rly,ruvmx,ruvmy,rem,iitt;
    uint8_t* rdesy,*rdesuv,*rsrcy,*rsrcuv;
    int em=0;
    snprintf(lt,99,"MB=[%d %d]",mbrow,mbindex);
    log_text(log_fd_mv_new,lt);
    #endif

    if(!(pMB->intrapredmc_type&MC4MV))
    {
        if(pMB->intrapredmc_type&MCForward)
        {
            mv_x=p_mc->mvx[0];
            mv_y=p_mc->mvy[0];

            mx=mv_x>>2;
            my=mv_y>>2;
            lx=mv_x&3;
            ly=mv_y&3;
            cx=mv_x/2;
            cy=mv_y/2;
            umx=cx>>2;
            umy=cy>>2;
            uvmx=(cx&3)<<1;
            uvmy=(cy&3)<<1;
            //due to some flaw RV40 uses the same MC compensation routine for H2V2 and H3V3
            if(uvmx == 6 && uvmy == 6)
                uvmx = uvmy = 4;

            dxy = ly*4 + lx;
            srcY =s->last_picture_ptr->data[0];
            srcU =s->last_picture_ptr->data[1];
            src_x = (mbindex <<4) + mx;
            src_y = (mbrow<<4)  + my;
            uvsrc_x = (mbindex<<3)  + umx;
            uvsrc_y = (mbrow<<3)  + umy;
            srcY += src_y * s->linesize + src_x;
            srcU += uvsrc_y * s->uvlinesize + (uvsrc_x<<1);
            srcV = srcU+1;

            if(   (unsigned)(src_x - !!lx*2) > s->h_edge_pos - !!lx*2 - 16 - 4
               || (unsigned)(src_y - !!ly*2) > s->v_edge_pos - !!ly*2 - 16 - 4){
                uint8_t *uvbuf= s->edge_emu_buffer + 22 * s->linesize;
                #ifdef __log_mv_new__
                    em=1;
                #endif
                srcY -= 2 + 2*s->linesize;
                s->dsp.emulated_edge_mc(s->edge_emu_buffer, srcY, s->linesize, 22, 22,
                                    src_x - 2, src_y - 2, s->h_edge_pos, s->v_edge_pos);
                srcY = s->edge_emu_buffer + 2 + 2*s->linesize;

                s->dsp.emulated_edge_mc_nv12(uvbuf, srcU, s->uvlinesize, 9, 9,
                                    uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
                srcU = uvbuf;
                srcV = uvbuf + 1;

                #ifdef __log_mv__
                use_em=1;
                #endif
            }
            #ifdef __log_mv_new__
            else
                em=0;
            #endif

            #ifdef __log_mv__
                if(!use_em)
                    snprintf(tstr,99,"y mc dxy=%d, des offset=%d, src offset=%d, dir=%d, size=%d",dxy,Y-s->current_picture_ptr->data[0],srcY-ptf,0,0);
                else
                    snprintf(tstr,99,"y mc_e dxy=%d, des offset=%d, src offset=%d, dir=%d, size=%d",dxy,Y-s->current_picture_ptr->data[0],srcY-s->edge_emu_buffer,0,0);
                log_text(log_fd_mv, tstr);
                if(!use_em)
                    snprintf(tstr,99,"c mc  des offset=%d, src offset=%d",U-s->current_picture_ptr->data[1],srcU-ptfc);
                else
                    snprintf(tstr,99,"c mc_e des offset=%d, src offset=%d",U-s->current_picture_ptr->data[1],srcU-s->edge_emu_buffer);
                log_text(log_fd_mv, tstr);

                snprintf(tstr,99,"y mv mv_x=%d, mv_y=%d; mx=%d,my=%d,lx=%d,ly=%d",mv_x,mv_y,mx,my,lx,ly);
                log_text(log_fd_mv, tstr);
                snprintf(tstr,99,"c mv cx=%d,cy=%d,umx=%d,umy=%d,uvmx=%d,uvmy=%d",cx,cy,umx,umy,uvmx,uvmy);
                log_text(log_fd_mv, tstr);
                snprintf(tstr,99,"current pic id=%d, ref pic id=%d",(int)s->current_picture_ptr->opaque,(int)s->last_picture_ptr->opaque);
                log_text(log_fd_mv, tstr);
            #endif

            //dump src before mc
            #ifdef __dump_mb_data__
                if(mbindex>=log_start_mb_x && mbindex<=log_end_mb_x && mbrow>=log_start_mb_y && mbrow<=log_end_mb_y )
                {
                    char pmbstr[80];
                    snprintf(pmbstr,79,"MBmcsrc2_x=%d_y=%d_%d_amba",mbindex,mbrow,0);
                    log_openfile(log_fd_mb_data,pmbstr);
                    log_dump_rect(log_fd_mb_data, srcY, 16, 16,s->linesize);
                    log_dump_rect(log_fd_mb_data, srcU, 16, 8,s->uvlinesize);
                    log_closefile(log_fd_mb_data);
                }
            #endif

            #ifdef _d_use_DSP_format_
            if(s->pict_type == FF_B_TYPE)
            {
                pbmv->fwd0.mv_x=mv_x;
                pbmv->fwd0.mv_y=mv_y;
                pbmv->fwd0.valid=1;
                pbmv->fwd0.ref_idx=0;
                pbmv->fwd1=pbmv->fwd2=pbmv->fwd3=pbmv->fwd0;
            }
            else
            {
                ppmv->fwd0.mv_x=mv_x;
                ppmv->fwd0.mv_y=mv_y;
                ppmv->fwd0.valid=1;
                ppmv->fwd0.ref_idx=0;
                ppmv->fwd1=ppmv->fwd2=ppmv->fwd3=ppmv->fwd0;
            }
            #endif

            #ifdef __dump_DSP_test_data__
            for(it=0;it<8;it++)
            {
                memcpy(thiz->fy.y[it*2],srcY+(it*2)*s->linesize,16);
                memcpy(thiz->fy.y[it*2+1],srcY+(it*2+1)*s->linesize,16);
                memcpy(thiz->fuv.uv[it],srcU+it*s->uvlinesize,16);
            }
            #endif

            #ifdef __log_mv_new__
            log_text(log_fd_mv_new,"Forward");
            snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
            log_text(log_fd_mv_new,lt);
            rmv_x=mv_x;rmv_y=mv_y;ruvmx=uvmx;ruvmy=uvmy;rlx=lx;rly=ly;rem=em;
            rdesy=Y;rdesuv=U;rsrcy=srcY;rsrcuv=srcU;
            snprintf(lt,99,"des y offset=%d, des uv offset=%d",Y-s->current_picture_ptr->data[0],U-s->current_picture_ptr->data[1]);
            log_text(log_fd_mv_new,lt);
            if(!em)
                snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY-s->last_picture_ptr->data[0],srcU-s->last_picture_ptr->data[1]);
            else
                snprintf(lt,99,"use em buffer");
            log_text(log_fd_mv_new,lt);
            #endif


            s->dsp.put_rv40_qpel_pixels_tab[0][dxy](Y,srcY,s->linesize);
            s->dsp.put_rv40_chroma_pixels_tab_nv12[0](U, srcU, s->uvlinesize, 8, uvmx, uvmy);
            s->dsp.put_rv40_chroma_pixels_tab_nv12[0](V, srcV, s->uvlinesize, 8, uvmx, uvmy);


            if(pMB->intrapredmc_type&MCBackward)
            {
                #ifdef __log_mv__
                use_em=0;
                #endif

                ambadec_assert_ffmpeg(s->pict_type == FF_B_TYPE);

                mv_x=p_mc->mvx[1];
                mv_y=p_mc->mvy[1];
                mx=mv_x>>2;
                my=mv_y>>2;
                lx=mv_x&3;
                ly=mv_y&3;
                cx=mv_x/2;
                cy=mv_y/2;
                umx=cx>>2;
                umy=cy>>2;
                uvmx=(cx&3)<<1;
                uvmy=(cy&3)<<1;
                //due to some flaw RV40 uses the same MC compensation routine for H2V2 and H3V3
                if(uvmx == 6 && uvmy == 6)
                    uvmx = uvmy = 4;

                dxy = ly*4 + lx;
                srcY =s->next_picture_ptr->data[0];
                srcU =s->next_picture_ptr->data[1];
                src_x = (mbindex <<4) + mx;
                src_y = (mbrow<<4) + my;
                uvsrc_x = (mbindex<<3)  + umx;
                uvsrc_y = (mbrow<<3) + umy;
                srcY += src_y * s->linesize + src_x;
                srcU += uvsrc_y * s->uvlinesize + (uvsrc_x<<1);
                srcV = srcU+1;

                if(   (unsigned)(src_x - !!lx*2) > s->h_edge_pos - !!lx*2 - 16 - 4
                   || (unsigned)(src_y - !!ly*2) > s->v_edge_pos - !!ly*2 - 16 - 4){
                    uint8_t *uvbuf= s->edge_emu_buffer + 22 * s->linesize;
                    #ifdef __log_mv_new__
                        em=1;
                    #endif
                    srcY -= 2 + 2*s->linesize;
                    s->dsp.emulated_edge_mc(s->edge_emu_buffer, srcY, s->linesize, 22, 22,
                                        src_x - 2, src_y - 2, s->h_edge_pos, s->v_edge_pos);
                    srcY = s->edge_emu_buffer + 2 + 2*s->linesize;

                    s->dsp.emulated_edge_mc_nv12(uvbuf, srcU, s->uvlinesize, 9, 9,
                                        uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
                    srcU = uvbuf;
                    srcV = uvbuf + 1;

                    #ifdef __log_mv__
                    use_em=1;
                    #endif

                }
                #ifdef __log_mv_new__
                else
                    em=0;
                #endif

                #ifdef __log_mv__
                    if(!use_em)
                        snprintf(tstr,99,"y mc dxy=%d, des offset=%d, src offset=%d, dir=%d, size=%d",dxy,Y-s->current_picture_ptr->data[0],srcY-ptb,1,0);
                    else
                        snprintf(tstr,99,"y mc_e dxy=%d, des offset=%d, src offset=%d, dir=%d, size=%d",dxy,Y-s->current_picture_ptr->data[0],srcY-s->edge_emu_buffer,1,0);
                    log_text(log_fd_mv, tstr);
                    if(!use_em)
                        snprintf(tstr,99,"c mc  des offset=%d, src offset=%d",U-s->current_picture_ptr->data[1],srcU-ptbc);
                    else
                        snprintf(tstr,99,"c mc_e des offset=%d, src offset=%d",U-s->current_picture_ptr->data[1],srcU-s->edge_emu_buffer);
                    log_text(log_fd_mv, tstr);

                    snprintf(tstr,99,"y mv mv_x=%d, mv_y=%d; mx=%d,my=%d,lx=%d,ly=%d",mv_x,mv_y,mx,my,lx,ly);
                    log_text(log_fd_mv, tstr);
                    snprintf(tstr,99,"c mv cx=%d,cy=%d,umx=%d,umy=%d,uvmx=%d,uvmy=%d",cx,cy,umx,umy,uvmx,uvmy);
                    log_text(log_fd_mv, tstr);
                    snprintf(tstr,99,"current pic id=%d, ref pic id=%d",(int)s->current_picture_ptr->opaque,(int)s->next_picture_ptr->opaque);
                    log_text(log_fd_mv, tstr);
                #endif

                        //dump src before mc
                #ifdef __dump_mb_data__
                    if(mbindex>=log_start_mb_x && mbindex<=log_end_mb_x && mbrow>=log_start_mb_y && mbrow<=log_end_mb_y )
                    {
                        char pmbstr[80];
                        snprintf(pmbstr,79,"MBmcsrc2_x=%d_y=%d_%d_amba",mbindex,mbrow,1);
                        log_openfile(log_fd_mb_data,pmbstr);
                        log_dump_rect(log_fd_mb_data, srcY, 16, 16,s->linesize);
                        log_dump_rect(log_fd_mb_data, srcU, 16, 8,s->uvlinesize);
                        log_closefile(log_fd_mb_data);
                    }
                #endif

                #ifdef _d_use_DSP_format_
                    pbmv->bwd0.mv_x=mv_x;
                    pbmv->bwd0.mv_y=mv_y;
                    pbmv->bwd0.valid=1;
                    pbmv->bwd0.ref_idx=1;
                    pbmv->bwd1=pbmv->bwd2=pbmv->bwd3=pbmv->bwd0;
                #endif

                #ifdef __dump_DSP_test_data__
                for(it=0;it<8;it++)
                {
                    memcpy(thiz->by.y[it*2],srcY+(it*2)*s->linesize,16);
                    memcpy(thiz->by.y[it*2+1],srcY+(it*2+1)*s->linesize,16);
                    memcpy(thiz->buv.uv[it],srcU+it*s->uvlinesize,16);
                }
                #endif

                #ifdef __log_mv_new__
                log_text(log_fd_mv_new,"Backward");
                snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
                log_text(log_fd_mv_new,lt);
                snprintf(lt,99,"des y offset=%d, des uv offset=%d",Y-s->current_picture_ptr->data[0],U-s->current_picture_ptr->data[1]);
                log_text(log_fd_mv_new,lt);
                if(!em)
                    snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY-s->next_picture_ptr->data[0],srcU-s->next_picture_ptr->data[1]);
                else
                    snprintf(lt,99,"use em buffer");
                log_text(log_fd_mv_new,lt);

                //emu 4mv
                for(iitt=1;iitt<4;iitt++)
                {
                    //fw
                    log_text(log_fd_mv_new,"Forward");
                    snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",rmv_x,rmv_y,ruvmx,ruvmy,rlx,rly);
                    log_text(log_fd_mv_new,lt);
                    snprintf(lt,99,"des y offset=%d, des uv offset=%d",rdesy+(iitt&1)*8+(iitt>>1)*s->linesize*8-s->current_picture_ptr->data[0],rdesuv+(iitt&1)*8+(iitt>>1)*s->uvlinesize*4-s->current_picture_ptr->data[1]);
                    log_text(log_fd_mv_new,lt);
                    if(!rem)
                        snprintf(lt,99,"src y offset=%d, src uv offset=%d",rsrcy+(iitt&1)*8+(iitt>>1)*s->linesize*8-s->last_picture_ptr->data[0],rsrcuv+(iitt&1)*8+(iitt>>1)*s->uvlinesize*4-s->last_picture_ptr->data[1]);
                    else
                        snprintf(lt,99,"use em buffer");
                    log_text(log_fd_mv_new,lt);

                    log_text(log_fd_mv_new,"Backward");
                    snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
                    log_text(log_fd_mv_new,lt);
                    snprintf(lt,99,"des y offset=%d, des uv offset=%d",Y+(iitt&1)*8+(iitt>>1)*s->linesize*8-s->current_picture_ptr->data[0],U+(iitt&1)*8+(iitt>>1)*s->uvlinesize*4-s->current_picture_ptr->data[1]);
                    log_text(log_fd_mv_new,lt);
                    if(!em)
                        snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY+(iitt&1)*8+(iitt>>1)*s->linesize*8-s->next_picture_ptr->data[0],srcU+(iitt&1)*8+(iitt>>1)*s->uvlinesize*4-s->next_picture_ptr->data[1]);
                    else
                        snprintf(lt,99,"use em buffer");
                    log_text(log_fd_mv_new,lt);

                }
                #endif

                s->dsp.avg_rv40_qpel_pixels_tab[0][dxy](Y,srcY,s->linesize);
                s->dsp.avg_rv40_chroma_pixels_tab_nv12[0](U, srcU, s->uvlinesize, 8, uvmx, uvmy);
                s->dsp.avg_rv40_chroma_pixels_tab_nv12[0](V, srcV, s->uvlinesize, 8, uvmx, uvmy);
            }
            #ifdef __log_mv_new__
            else
            {
                //emu 4mv
                for(iitt=1;iitt<4;iitt++)
                {
                    //fw
                    log_text(log_fd_mv_new,"Forward");
                    snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",rmv_x,rmv_y,ruvmx,ruvmy,rlx,rly);
                    log_text(log_fd_mv_new,lt);
                    snprintf(lt,99,"des y offset=%d, des uv offset=%d",rdesy+(iitt&1)*8+(iitt>>1)*s->linesize*8-s->current_picture_ptr->data[0],rdesuv+(iitt&1)*8+(iitt>>1)*s->uvlinesize*4-s->current_picture_ptr->data[1]);
                    log_text(log_fd_mv_new,lt);
                    if(!rem)
                        snprintf(lt,99,"src y offset=%d, src uv offset=%d",rsrcy+(iitt&1)*8+(iitt>>1)*s->linesize*8-s->last_picture_ptr->data[0],rsrcuv+(iitt&1)*8+(iitt>>1)*s->uvlinesize*4-s->last_picture_ptr->data[1]);
                    else
                        snprintf(lt,99,"use em buffer");
                    log_text(log_fd_mv_new,lt);
                }
            }
            #endif


        }
        else if(pMB->intrapredmc_type&MCBackward)
        {
            mv_x=p_mc->mvx[0];
            mv_y=p_mc->mvy[0];
            mx=mv_x>>2;
            my=mv_y>>2;
            lx=mv_x&3;
            ly=mv_y&3;
            cx=mv_x/2;
            cy=mv_y/2;
            umx=cx>>2;
            umy=cy>>2;
            uvmx=(cx&3)<<1;
            uvmy=(cy&3)<<1;
            //due to some flaw RV40 uses the same MC compensation routine for H2V2 and H3V3
            if(uvmx == 6 && uvmy == 6)
                uvmx = uvmy = 4;

            dxy = ly*4 + lx;
            srcY =s->next_picture_ptr->data[0];
            srcU =s->next_picture_ptr->data[1];
            src_x = (mbindex <<4) + mx;
            src_y = (mbrow<<4) + my;
            uvsrc_x = (mbindex<<3)  + umx;
            uvsrc_y = (mbrow<<3)  + umy;
            srcY += src_y * s->linesize + src_x;
            srcU += uvsrc_y * s->uvlinesize + (uvsrc_x<<1);
            srcV = srcU+1;

            if(   (unsigned)(src_x - !!lx*2) > s->h_edge_pos - !!lx*2 - 16 - 4
               || (unsigned)(src_y - !!ly*2) > s->v_edge_pos - !!ly*2 - 16 - 4){
                uint8_t *uvbuf= s->edge_emu_buffer + 22 * s->linesize;
                #ifdef __log_mv_new__
                    em=1;
                #endif
                srcY -= 2 + 2*s->linesize;
                s->dsp.emulated_edge_mc(s->edge_emu_buffer, srcY, s->linesize, 22, 22,
                                    src_x - 2, src_y - 2, s->h_edge_pos, s->v_edge_pos);
                srcY = s->edge_emu_buffer + 2 + 2*s->linesize;

                s->dsp.emulated_edge_mc_nv12(uvbuf, srcU, s->uvlinesize, 9, 9,
                                    uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
                srcU = uvbuf;
                srcV = uvbuf + 1;

                #ifdef __log_mv__
                use_em=1;
                #endif
            }
            #ifdef __log_mv_new__
            else
                em=0;
            #endif

            #ifdef __log_mv__
                if(!use_em)
                    snprintf(tstr,99,"y mc dxy=%d, des offset=%d, src offset=%d, dir=%d, size=%d",dxy,Y-s->current_picture_ptr->data[0],srcY-ptb,1,0);
                else
                    snprintf(tstr,99,"y mc_e dxy=%d, des offset=%d, src offset=%d, dir=%d, size=%d",dxy,Y-s->current_picture_ptr->data[0],srcY-s->edge_emu_buffer,1,0);
                log_text(log_fd_mv, tstr);
                if(!use_em)
                    snprintf(tstr,99,"c mc  des offset=%d, src offset=%d",U-s->current_picture_ptr->data[1],srcU-ptbc);
                else
                    snprintf(tstr,99,"c mc_e des offset=%d, src offset=%d",U-s->current_picture_ptr->data[1],srcU-s->edge_emu_buffer);
                log_text(log_fd_mv, tstr);

                snprintf(tstr,99,"y mv mv_x=%d, mv_y=%d; mx=%d,my=%d,lx=%d,ly=%d",mv_x,mv_y,mx,my,lx,ly);
                log_text(log_fd_mv, tstr);
                snprintf(tstr,99,"c mv cx=%d,cy=%d,umx=%d,umy=%d,uvmx=%d,uvmy=%d",cx,cy,umx,umy,uvmx,uvmy);
                log_text(log_fd_mv, tstr);
                snprintf(tstr,99,"current pic id=%d, ref pic id=%d",(int)s->current_picture_ptr->opaque,(int)s->next_picture_ptr->opaque);
                log_text(log_fd_mv, tstr);
            #endif

            #ifdef _d_use_DSP_format_
                pbmv->bwd0.mv_x=mv_x;
                pbmv->bwd0.mv_y=mv_y;
                pbmv->bwd0.valid=1;
                pbmv->bwd0.ref_idx=1;
                pbmv->bwd1=pbmv->bwd2=pbmv->bwd3=pbmv->bwd0;
            #endif

            #ifdef __dump_DSP_test_data__
            for(it=0;it<8;it++)
            {
                memcpy(thiz->by.y[it*2],srcY+(it*2)*s->linesize,16);
                memcpy(thiz->by.y[it*2+1],srcY+(it*2+1)*s->linesize,16);
                memcpy(thiz->buv.uv[it],srcU+it*s->uvlinesize,16);
            }
            #endif

            #ifdef __log_mv_new__
            log_text(log_fd_mv_new,"Backward");
            snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
            log_text(log_fd_mv_new,lt);
            snprintf(lt,99,"des y offset=%d, des uv offset=%d",Y-s->current_picture_ptr->data[0],U-s->current_picture_ptr->data[1]);
            log_text(log_fd_mv_new,lt);
            if(!em)
                snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY-s->next_picture_ptr->data[0],srcU-s->next_picture_ptr->data[1]);
            else
                snprintf(lt,99,"use em buffer");
            log_text(log_fd_mv_new,lt);

            //emu 4mv
            for(iitt=1;iitt<4;iitt++)
            {
                //bw
                log_text(log_fd_mv_new,"Backward");
                snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
                log_text(log_fd_mv_new,lt);
                snprintf(lt,99,"des y offset=%d, des uv offset=%d",Y+(iitt&1)*8+(iitt>>1)*s->linesize*8-s->current_picture_ptr->data[0],U+(iitt&1)*8+(iitt>>1)*s->uvlinesize*4-s->current_picture_ptr->data[1]);
                log_text(log_fd_mv_new,lt);
                if(!em)
                    snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY+(iitt&1)*8+(iitt>>1)*s->linesize*8-s->next_picture_ptr->data[0],srcU+(iitt&1)*8+(iitt>>1)*s->uvlinesize*4-s->next_picture_ptr->data[1]);
                else
                    snprintf(lt,99,"use em buffer");
                log_text(log_fd_mv_new,lt);

            }

            #endif

            s->dsp.put_rv40_qpel_pixels_tab[0][dxy](Y,srcY,s->linesize);
            s->dsp.put_rv40_chroma_pixels_tab_nv12[0](U, srcU, s->uvlinesize, 8, uvmx, uvmy);
            s->dsp.put_rv40_chroma_pixels_tab_nv12[0](V, srcV, s->uvlinesize, 8, uvmx, uvmy);
        }

    }
    else
    {// 4mv
        uint8_t *Yy, *Uu, *Vv;
        // 4mv forward or bidirectional, no 4mv backward?
        ambadec_assert_ffmpeg(pMB->intrapredmc_type&MCForward);
        for(i=0;i<4;i++)
        {
            #ifdef __log_mv__
                use_em=0;
            #endif

            mv_x=p_mc->mvx[i];
            mv_y=p_mc->mvy[i];
            mx=mv_x>>2;
            my=mv_y>>2;
            lx=mv_x&3;
            ly=mv_y&3;
            cx=mv_x/2;
            cy=mv_y/2;
            umx=cx>>2;
            umy=cy>>2;
            uvmx=(cx&3)<<1;
            uvmy=(cy&3)<<1;
            //due to some flaw RV40 uses the same MC compensation routine for H2V2 and H3V3
            if(uvmx == 6 && uvmy == 6)
                uvmx = uvmy = 4;

            dxy = ly*4 + lx;
            srcY =s->last_picture_ptr->data[0];
            srcU =s->last_picture_ptr->data[1];
            src_x = (mbindex <<4) +((i&0x1)<<3)+ mx;
            src_y = (mbrow<<4) + ((i>>1)<<3) + my;
            uvsrc_x = (mbindex<<3) + ((i&0x1)<<2) + umx;
            uvsrc_y = (mbrow<<3) + ((i>>1)<<2) + umy;
            srcY += src_y * s->linesize + src_x;
            srcU += uvsrc_y * s->uvlinesize + (uvsrc_x<<1);
            srcV = srcU+1;

            if(   (unsigned)(src_x - !!lx*2) > s->h_edge_pos - !!lx*2 - 8 - 4
               || (unsigned)(src_y - !!ly*2) > s->v_edge_pos - !!ly*2 - 8 - 4){
                uint8_t *uvbuf= s->edge_emu_buffer + 22 * s->linesize;
                #ifdef __log_mv_new__
                em=1;
                #endif
                srcY -= 2 + 2*s->linesize;
                s->dsp.emulated_edge_mc(s->edge_emu_buffer, srcY, s->linesize, 14,14,
                                    src_x - 2, src_y - 2, s->h_edge_pos, s->v_edge_pos);
                srcY = s->edge_emu_buffer + 2 + 2*s->linesize;

                s->dsp.emulated_edge_mc_nv12(uvbuf, srcU, s->uvlinesize, 5, 5,
                                    uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
                srcU = uvbuf;
                srcV = uvbuf + 1;

                #ifdef __log_mv__
                use_em=1;
                #endif
            }
            #ifdef __log_mv_new__
            else
            em=0;
            #endif

            Yy=Y+((i&0x1)<<3)+((i>>1)<<3)*s->linesize;
            Uu=U+((i&0x1)<<3)+((i>>1)<<2)*s->uvlinesize;
            Vv=Uu+1;

            #ifdef __log_mv__
                if(!use_em)
                    snprintf(tstr,99,"y mc dxy=%d, des offset=%d, src offset=%d, dir=%d, size=%d",dxy,Yy-s->current_picture_ptr->data[0],srcY-ptf,0,1);
                else
                    snprintf(tstr,99,"y mc_e dxy=%d, des offset=%d, src offset=%d, dir=%d, size=%d",dxy,Yy-s->current_picture_ptr->data[0],srcY-s->edge_emu_buffer,0,1);
                log_text(log_fd_mv, tstr);
                if(!use_em)
                    snprintf(tstr,99,"c mc  des offset=%d, src offset=%d",Uu-s->current_picture_ptr->data[1],srcU-ptfc);
                else
                    snprintf(tstr,99,"c mc_e des offset=%d, src offset=%d",Uu-s->current_picture_ptr->data[1],srcU-s->edge_emu_buffer);
                log_text(log_fd_mv, tstr);

                snprintf(tstr,99,"y mv mv_x=%d, mv_y=%d; mx=%d,my=%d,lx=%d,ly=%d",mv_x,mv_y,mx,my,lx,ly);
                log_text(log_fd_mv, tstr);
                snprintf(tstr,99,"c mv cx=%d,cy=%d,umx=%d,umy=%d,uvmx=%d,uvmy=%d",cx,cy,umx,umy,uvmx,uvmy);
                log_text(log_fd_mv, tstr);
                snprintf(tstr,99,"current pic id=%d, ref pic id=%d",(int)s->current_picture_ptr->opaque,(int)s->last_picture_ptr->opaque);
                log_text(log_fd_mv, tstr);
            #endif

            #ifdef _d_use_DSP_format_
            if(s->pict_type == FF_B_TYPE)
            {
                switch(i)
                {
                    case 0:
                    pbmv->fwd0.mv_x=mv_x;
                    pbmv->fwd0.mv_y=mv_y;
                    pbmv->fwd0.valid=1;
                    pbmv->fwd0.ref_idx=0;
                        break;

                    case 1:
                    pbmv->fwd1.mv_x=mv_x;
                    pbmv->fwd1.mv_y=mv_y;
                    pbmv->fwd1.valid=1;
                    pbmv->fwd1.ref_idx=0;
                        break;

                    case 2:
                    pbmv->fwd2.mv_x=mv_x;
                    pbmv->fwd2.mv_y=mv_y;
                    pbmv->fwd2.valid=1;
                    pbmv->fwd2.ref_idx=0;
                        break;

                    case 3:
                    pbmv->fwd3.mv_x=mv_x;
                    pbmv->fwd3.mv_y=mv_y;
                    pbmv->fwd3.valid=1;
                    pbmv->fwd3.ref_idx=0;
                        break;
                }

            }
            else
            {
                switch(i)
                {
                    case 0:
                    ppmv->fwd0.mv_x=mv_x;
                    ppmv->fwd0.mv_y=mv_y;
                    ppmv->fwd0.valid=1;
                    ppmv->fwd0.ref_idx=0;
                        break;
                    case 1:
                    ppmv->fwd1.mv_x=mv_x;
                    ppmv->fwd1.mv_y=mv_y;
                    ppmv->fwd1.valid=1;
                    ppmv->fwd1.ref_idx=0;
                        break;
                    case 2:
                    ppmv->fwd2.mv_x=mv_x;
                    ppmv->fwd2.mv_y=mv_y;
                    ppmv->fwd2.valid=1;
                    ppmv->fwd2.ref_idx=0;
                        break;
                    case 3:
                    ppmv->fwd3.mv_x=mv_x;
                    ppmv->fwd3.mv_y=mv_y;
                    ppmv->fwd3.valid=1;
                    ppmv->fwd3.ref_idx=0;
                        break;
                }
            }
            #endif

            #ifdef __dump_DSP_test_data__
            offset=(i&2)*64+(i&1)*8;
            for(it=0;it<4;it++)
            {
                memcpy(thiz->fy.y[it*2]+offset,srcY+(it*2)*s->linesize,8);
                memcpy(thiz->fy.y[it*2+1]+offset,srcY+(it*2+1)*s->linesize,8);
            }
            offset=(i&2)*32+(i&1)*8;
            for(it=0;it<4;it++)
            {
                memcpy(thiz->fuv.uv[it]+offset,srcU+it*s->uvlinesize,8);
            }
            #endif

            #ifdef __log_mv_new__
            log_text(log_fd_mv_new,"Forward");
            snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
            log_text(log_fd_mv_new,lt);
            snprintf(lt,99,"des y offset=%d, des uv offset=%d",Yy-s->current_picture_ptr->data[0],Uu-s->current_picture_ptr->data[1]);
            log_text(log_fd_mv_new,lt);
            if(!em)
                snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY-s->last_picture_ptr->data[0],srcU-s->last_picture_ptr->data[1]);
            else
                snprintf(lt,99,"use em buffer");
            log_text(log_fd_mv_new,lt);
            #endif

            s->dsp.put_rv40_qpel_pixels_tab[1][dxy](Yy,srcY,s->linesize);
            s->dsp.put_rv40_chroma_pixels_tab_nv12[1](Uu, srcU, s->uvlinesize, 4, uvmx, uvmy);
            s->dsp.put_rv40_chroma_pixels_tab_nv12[1](Vv, srcV, s->uvlinesize, 4, uvmx, uvmy);

            if(pMB->intrapredmc_type&MCBackward)
            {
                #ifdef __log_mv__
                    use_em=0;
                #endif
                mv_x=p_mc->mvx[i+4];
                mv_y=p_mc->mvy[i+4];
                mx=mv_x>>2;
                my=mv_y>>2;
                lx=mv_x&3;
                ly=mv_y&3;
                cx=mv_x/2;
                cy=mv_y/2;
                umx=cx>>2;
                umy=cy>>2;
                uvmx=(cx&3)<<1;
                uvmy=(cy&3)<<1;
                //due to some flaw RV40 uses the same MC compensation routine for H2V2 and H3V3
                if(uvmx == 6 && uvmy == 6)
                    uvmx = uvmy = 4;

                dxy = ly*4 + lx;
                srcY =s->next_picture_ptr->data[0];
                srcU =s->next_picture_ptr->data[1];
                src_x = (mbindex <<4) +((i&0x1)<<3)+ mx;
                src_y = (mbrow<<4) + ((i>>1)<<3) + my;
                uvsrc_x = (mbindex<<3) + ((i&0x1)<<2) + umx;
                uvsrc_y = (mbrow<<3) + ((i>>1)<<2) + umy;
                srcY += src_y * s->linesize + src_x;
                srcU += uvsrc_y * s->uvlinesize + (uvsrc_x<<1);
                srcV = srcU+1;

                if(   (unsigned)(src_x - !!lx*2) > s->h_edge_pos - !!lx*2 - 8 - 4
                   || (unsigned)(src_y - !!ly*2) > s->v_edge_pos - !!ly*2 - 8 - 4){
                    uint8_t *uvbuf= s->edge_emu_buffer + 22 * s->linesize;
                    #ifdef __log_mv_new__
                    em=1;
                    #endif
                    srcY -= 2 + 2*s->linesize;
                    s->dsp.emulated_edge_mc(s->edge_emu_buffer, srcY, s->linesize, 14,14,
                                        src_x - 2, src_y - 2, s->h_edge_pos, s->v_edge_pos);
                    srcY = s->edge_emu_buffer + 2 + 2*s->linesize;

                    s->dsp.emulated_edge_mc_nv12(uvbuf, srcU, s->uvlinesize, 5, 5,
                                        uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
                    srcU = uvbuf;
                    srcV = uvbuf + 1;

                    #ifdef __log_mv__
                    use_em=1;
                    #endif
                }
                #ifdef __log_mv_new__
                else
                em=0;
                #endif

                #ifdef __log_mv__
                    if(!use_em)
                        snprintf(tstr,99,"y mc dxy=%d, des offset=%d, src offset=%d, dir=%d, size=%d",dxy,Yy-s->current_picture_ptr->data[0],srcY-ptb,1,1);
                    else
                        snprintf(tstr,99,"y mc_e dxy=%d, des offset=%d, src offset=%d, dir=%d, size=%d",dxy,Yy-s->current_picture_ptr->data[0],srcY-s->edge_emu_buffer,1,1);
                    if(!use_em)
                        snprintf(tstr,99,"c mc  des offset=%d, src offset=%d",Uu-s->current_picture_ptr->data[1],srcU-ptbc);
                    else
                        snprintf(tstr,99,"c mc_e des offset=%d, src offset=%d",Uu-s->current_picture_ptr->data[1],srcU-s->edge_emu_buffer);
                    log_text(log_fd_mv, tstr);

                    log_text(log_fd_mv, tstr);
                    snprintf(tstr,99,"y mv mv_x=%d, mv_y=%d; mx=%d,my=%d,lx=%d,ly=%d",mv_x,mv_y,mx,my,lx,ly);
                    log_text(log_fd_mv, tstr);
                    snprintf(tstr,99,"c mv cx=%d,cy=%d,umx=%d,umy=%d,uvmx=%d,uvmy=%d",cx,cy,umx,umy,uvmx,uvmy);
                    log_text(log_fd_mv, tstr);
                    snprintf(tstr,99,"current pic id=%d, ref pic id=%d",(int)s->current_picture_ptr->opaque,(int)s->next_picture_ptr->opaque);
                    log_text(log_fd_mv, tstr);
                #endif

                #ifdef _d_use_DSP_format_
                switch(i)
                {
                    case 0:
                    pbmv->bwd0.mv_x=mv_x;
                    pbmv->bwd0.mv_y=mv_y;
                    pbmv->bwd0.valid=1;
                    pbmv->bwd0.ref_idx=1;
                        break;

                    case 1:
                    pbmv->bwd1.mv_x=mv_x;
                    pbmv->bwd1.mv_y=mv_y;
                    pbmv->bwd1.valid=1;
                    pbmv->bwd1.ref_idx=1;
                        break;

                    case 2:
                    pbmv->bwd2.mv_x=mv_x;
                    pbmv->bwd2.mv_y=mv_y;
                    pbmv->bwd2.valid=1;
                    pbmv->bwd2.ref_idx=1;
                        break;

                    case 3:
                    pbmv->bwd3.mv_x=mv_x;
                    pbmv->bwd3.mv_y=mv_y;
                    pbmv->bwd3.valid=1;
                    pbmv->bwd3.ref_idx=1;
                        break;
                }
                #endif

                #ifdef __dump_DSP_test_data__
                offset=(i&2)*64+(i&1)*8;
                for(it=0;it<4;it++)
                {
                    memcpy(thiz->by.y[it*2]+offset,srcY+(it*2)*s->linesize,8);
                    memcpy(thiz->by.y[it*2+1]+offset,srcY+(it*2+1)*s->linesize,8);
                }
                offset=(i&2)*32+(i&1)*8;
                for(it=0;it<4;it++)
                {
                    memcpy(thiz->buv.uv[it]+offset,srcU+it*s->uvlinesize,8);
                }
                #endif

                #ifdef __log_mv_new__
                log_text(log_fd_mv_new,"Backward");
                snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
                log_text(log_fd_mv_new,lt);
                snprintf(lt,99,"des y offset=%d, des uv offset=%d",Yy-s->current_picture_ptr->data[0],Uu-s->current_picture_ptr->data[1]);
                log_text(log_fd_mv_new,lt);
                if(!em)
                    snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY-s->next_picture_ptr->data[0],srcU-s->next_picture_ptr->data[1]);
                else
                    snprintf(lt,99,"use em buffer");
                log_text(log_fd_mv_new,lt);
                #endif

                s->dsp.avg_rv40_qpel_pixels_tab[1][dxy](Yy,srcY,s->linesize);
                s->dsp.avg_rv40_chroma_pixels_tab_nv12[1](Uu, srcU, s->uvlinesize, 4, uvmx, uvmy);
                s->dsp.avg_rv40_chroma_pixels_tab_nv12[1](Vv, srcV, s->uvlinesize, 4, uvmx, uvmy);
            }
        }

    }

}

static inline void add_result_mb_c(uint8_t* pdesy,uint8_t* pdesuv ,DCTELEM* y,DCTELEM* uv,int stride)
{
    int i,j;

    //Y
    for(j=0;j<16;j++)
    {
        for(i=0;i<16;i++)
        {
            pdesy[i]=av_clip_uint8((*y++)+pdesy[i]);
        }
        pdesy+=stride;
    }

    //uv
    for(j=0;j<8;j++)
    {
        for(i=0;i<16;i++)
        {
            pdesuv[i]=av_clip_uint8((*uv++)+pdesuv[i]);
        }
        pdesuv+=stride;
    }
}

static inline void add_result_4x4_y_c(uint8_t* pdes ,DCTELEM* psrc,int stride)
{
    int i,j;

    //Y
    for(j=0;j<4;j++)
    {
        for(i=0;i<4;i++)
        {
            pdes[i]=av_clip_uint8(psrc[i]+pdes[i]);
        }
        psrc+=16;
        pdes+=stride;
    }

}

static inline void add_result_4x4_uv_c(uint8_t* pdes,DCTELEM* psrc,int stride)
{
    int i,j;

    for(j=0;j<4;j++)
    {
        for(i=0;i<8;i++)
        {
            pdes[i]=av_clip_uint8(psrc[i]+pdes[i]);
        }
        pdes+=stride;
        psrc+=16;
    }
}


static inline void process_intrapred_add_idct(RV34AmbaDecContext* thiz,MBInfoMCIntraPred* pMB,uint8_t* Y, uint8_t*UV,DCTELEM* py,DCTELEM* puv)
{
    MpegEncContext *s = &thiz->s;
    int i,j,ij;
    uint8_t* ptype=pMB->info.intrapred.intratype;

    if(pMB->intrapredmc_type&IntraPred16x16)
    {
        if(log_mask[decoding_config_intrapred])
        {
            thiz->h.pred16x16[ptype[0]](Y, s->linesize);
            thiz->h.pred8x8_nv12[ptype[1]](UV, s->uvlinesize);
        }

        #ifdef __log_intrapred__
            char tstr[40];
            log_text(log_fd_intrapred, "y index=0 ");
            snprintf(tstr,39,"y type=%d, offset=%d ",ptype[0],Y-s->current_picture_ptr->data[0]);
            log_text(log_fd_intrapred, tstr);
            log_text(log_fd_intrapred, "uv index=1 ");
            snprintf(tstr,39,"uv type=%d, offset=%d ",ptype[1],UV-s->current_picture_ptr->data[1]);
            log_text(log_fd_intrapred, tstr);
        #endif

        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_add_idct])
        #endif
        add_result_mb_c(Y,UV,py,puv,s->linesize);
    }
    else
    {
        uint32_t topleft32;
        uint64_t topleft64;

        //Y
        for(j=0;j<4;j++)
        {
            for(i=0;i<4;i++)
            {
                ij=(j<<2)+i;
                #ifdef __log_intrapred__
                    char tstr[40];
                    snprintf(tstr,39,"y index=%d ",ij);
                    log_text(log_fd_intrapred, tstr);
                #endif

                //use topleft
                if(pMB->intrapredmc_type&(1<<(12+ij)))
                {
                    topleft32=Y[-s->linesize+3]*0x01010101;
                    #ifdef __log_decoding_config__
                    if(log_mask[decoding_config_intrapred])
                    #endif
                    thiz->h.pred4x4[ptype[ij]](Y, (uint8_t*)(&topleft32), s->linesize);
                    #ifdef __log_intrapred__
                        log_text(log_fd_intrapred, "y topleft ");
                    #endif
                }
                else
                {
                    #ifdef __log_decoding_config__
                    if(log_mask[decoding_config_intrapred])
                    #endif
                    thiz->h.pred4x4[ptype[ij]](Y, Y-s->linesize+4, s->linesize);
                }

                #ifdef __log_intrapred__
                    snprintf(tstr,39,"y type=%d, offset=%d ",ptype[ij],Y-s->current_picture_ptr->data[0]);
                    log_text(log_fd_intrapred, tstr);
                #endif

                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_add_idct])
                #endif
                add_result_4x4_y_c(Y, py,s->linesize);

                py+=4;
                Y+=4;
            }
            py+=48;
            Y+= (s->linesize<<2) - 16;
        }

        //UV
        for(j=0;j<2;j++)
        {
            for(i=0;i<2;i++)
            {
                ij=(j<<1)+i+16;

                #ifdef __log_intrapred__
                    char tstr[40];
                    snprintf(tstr,39,"uv index=%d ",ij);
                    log_text(log_fd_intrapred, tstr);
                #endif

                //use topleft
                if(pMB->intrapredmc_type&(1<<(12+ij)))
                {
                    #ifdef __log_intrapred__
                        log_text(log_fd_intrapred, "uv topleft ");
                    #endif
                    topleft64=UV[-s->uvlinesize+ 6] * 0x0001000100010001+UV[-s->uvlinesize+ 7]*0x0100010001000100;
                    #ifdef __log_decoding_config__
                    if(log_mask[decoding_config_intrapred])
                    #endif
                    thiz->h.pred4x4_nv12[ptype[ij]]( UV, (uint8_t*) (&topleft64), s->uvlinesize);
                }
                else
                {
                    #ifdef __log_decoding_config__
                    if(log_mask[decoding_config_intrapred])
                    #endif
                    thiz->h.pred4x4_nv12[ptype[ij]](UV, UV-s->uvlinesize+8, s->uvlinesize);
                }

                #ifdef __log_intrapred__
                    snprintf(tstr,39,"uv type=%d, offset=%d ",ptype[ij],UV-s->current_picture_ptr->data[1]);
                    log_text(log_fd_intrapred, tstr);
                #endif

                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_add_idct])
                #endif
                add_result_4x4_uv_c(UV,puv,s->uvlinesize);

                puv+=8;
                UV+=8;
            }
            puv+=48;
            UV += (s->uvlinesize<<2)-16;
        }
    }
}

static inline void trigger_deblock(RV34AmbaDecContext* thiz,int mbrow)
{
    MBLineDeblock* p_deblock;

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"trigger_deblock mbrow=%d start.\n",mbrow);
    #endif

    thiz->p_deblock_free_queue->lock(thiz->p_deblock_free_queue);
    if(thiz->p_deblock_free_queue->getcnt(thiz->p_deblock_free_queue)>0)
    {
        thiz->p_deblock_free_queue->unlock(thiz->p_deblock_free_queue);
        p_deblock=thiz->p_deblock_free_queue->dequeue(thiz->p_deblock_free_queue);
        p_deblock->mbrow=mbrow;
    }
    else
    {
        thiz->p_deblock_free_queue->unlock(thiz->p_deblock_free_queue);
        p_deblock=av_malloc(sizeof(MBLineDeblock));
        p_deblock->mbrow=mbrow;
    }
    thiz->p_deblock_queue->enqueue(thiz->p_deblock_queue,p_deblock);

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"trigger_deblock mbrow=%d done.\n",mbrow);
    #endif
}


//NULL packet indicate exit
void* mc_intraprediction_thread(void* p)
{
    RV34AmbaDecContext* thiz=(RV34AmbaDecContext*)p;
    MpegEncContext *s = &thiz->s;
    MBLineMCIntraPred* p_mc_intrapred=NULL;
    MBInfoMCIntraPred* pMB;
    int stride=0;

    int mbwidth;
    int mbindex=0,mbrow=0;
    uint8_t* pdesy,*pdesuv;
    DCTELEM* y,*uv;

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"mc_intraprediction_thread start.\n");
    #endif

    while(thiz->mc_intrapred_loop)
    {
//        thiz->intrapred_mc_wait=1;
        p_mc_intrapred=thiz->p_mc_intrapred_queue->dequeue(thiz->p_mc_intrapred_queue);
//        thiz->intrapred_mc_wait=0;

        if(!thiz->mc_intrapred_loop || !p_mc_intrapred)
        {
//            ambadec_assert_ffmpeg(!p_mc_intrapred && !thiz->mc_intrapred_loop);

            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"*error* mc_intraprediction_thread quit after get packet=%p, thiz->mc_intrapred_loop=%d.\n",p_mc_intrapred,thiz->mc_intrapred_loop);
            #endif
//            av_log(NULL,AV_LOG_ERROR,"get NULL data or loop=0 in mcintrapred_thread start exit.\n");

            break;
        }

        mbwidth=p_mc_intrapred->mbwidth;
        pMB=p_mc_intrapred->pMBInfo;
        mbrow=p_mc_intrapred->mbrow;

        pdesy=s->current_picture_ptr->data[0]+(mbrow<<4)*s->linesize;
        pdesuv = s->current_picture_ptr->data[1] +(mbrow<<3) *s->uvlinesize;
        y=p_mc_intrapred->p_idct_result_y;
        uv=p_mc_intrapred->p_idct_result_uv;
        stride=s->linesize;

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"*count* mc_intraprediction_thread process mbrow=%d.\n",mbrow);
        #endif

        //process mc
        for(mbindex=0;mbindex<mbwidth;++mbindex,++pMB)
        {
            #ifdef __log_mv__
                char tstre[40];
                snprintf(tstre,39,"MB_x=%d_y=%d ",mbindex,mbrow);
                log_text(log_fd_mv, tstre);
            #endif

            #ifdef __dump_DSP_test_data__
            memset(&thiz->result,0,sizeof(RVDEC_MB_RESULT_t));
            memset(&thiz->fy,0,sizeof(RVDEC_MB_Y_REF_t));
            memset(&thiz->fuv,0,sizeof(RVDEC_MB_UV_REF_t));
            //if(s->pict_type == FF_B_TYPE)
            {
                memset(&thiz->by,0,sizeof(RVDEC_MB_Y_REF_t));
                memset(&thiz->buv,0,sizeof(RVDEC_MB_UV_REF_t));
            }
            #endif

            if(pMB->intrapredmc_type&useMC)
            {
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_mc])
                #endif
                process_mc(thiz,pMB,mbindex,mbrow);

                #ifdef __log_mv__
                //idct
                log_text(log_fd_mv, "inverse transform result is ");
                log_idct_txt(log_fd_mv, y+(mbindex<<8), 16);
                log_idct_txt_uv_separated(log_fd_mv,uv+(mbindex<<7));
                #endif
            }
            #ifdef __dump_DSP_test_data__
            else//fake valid=1 for inta MB for hardware use
            {
                RVDEC_MB_MV_B_t* pbmv=thiz->pdspmv.b+mbrow*(s->mb_width)+mbindex;
                RVDEC_MB_MV_P_t* ppmv=thiz->pdspmv.p+mbrow*(s->mb_width)+mbindex;
                if(s->pict_type == FF_B_TYPE)
                {
                    pbmv->fwd0.valid=1;
                    pbmv->fwd0.ref_idx=0;
                    pbmv->fwd1=pbmv->fwd2=pbmv->fwd3=pbmv->fwd0;
                    pbmv->bwd0.valid=1;
                    pbmv->bwd0.ref_idx=1;
                    pbmv->bwd1=pbmv->bwd2=pbmv->bwd3=pbmv->bwd0;
                }
                else if(s->pict_type == FF_P_TYPE)
                {
                    ppmv->fwd0.valid=1;
                    ppmv->fwd0.ref_idx=0;
                    ppmv->fwd1=ppmv->fwd2=ppmv->fwd3=ppmv->fwd0;
                }
            }
            #endif

            //dump after mc
            #ifdef __dump_mb_data__
                if(mbindex>=log_start_mb_x && mbindex<=log_end_mb_x && mbrow>=log_start_mb_y && mbrow<=log_end_mb_y )
                {
                    char pmbstr[80];
                    snprintf(pmbstr,79,"MBmc_x=%d_y=%d_amba",mbindex,mbrow);
                    log_openfile(log_fd_mb_data,pmbstr);
                    log_dump_rect(log_fd_mb_data, pdesy+(mbindex<<4), 16, 16,stride);
                    log_dump_rect(log_fd_mb_data, pdesuv+(mbindex<<4), 16, 8,stride);
                    log_closefile(log_fd_mb_data);
                }
            #endif

            if(pMB->intrapredmc_type&useMC)
            {
                //add inverse transform result
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_add_idct])
                #endif
                add_result_mb_c(pdesy+(mbindex<<4),pdesuv+(mbindex<<4),y+(mbindex<<8),uv+(mbindex<<7),stride);
            }

            #ifdef __dump_DSP_test_data__
            log_dump(log_fd_reffytiled,&thiz->fy,sizeof(RVDEC_MB_Y_REF_t));
            log_dump(log_fd_reffuvtiled,&thiz->fuv,sizeof(RVDEC_MB_UV_REF_t));
            //if(s->pict_type == FF_B_TYPE)
            {
                log_dump(log_fd_refbytiled,&thiz->by,sizeof(RVDEC_MB_Y_REF_t));
                log_dump(log_fd_refbuvtiled,&thiz->buv,sizeof(RVDEC_MB_UV_REF_t));
            }

            #ifdef __log_mv__
            if(pMB->intrapredmc_type&useMC)
            {    //refernce tiled
                log_text(log_fd_mv, "fw reference is ");
                log_text(log_fd_mv, "    y  ");
                log_reference_txt(log_fd_mv,&thiz->fy.y[0][0],16,16);
                log_text(log_fd_mv, "    uv  ");
                log_reference_txt(log_fd_mv,&thiz->fuv.uv[0][0],16,8);
                if(s->pict_type == FF_B_TYPE)
                {
                    log_text(log_fd_mv, "bw reference is ");
                    log_text(log_fd_mv, "    y  ");
                    log_reference_txt(log_fd_mv,&thiz->by.y[0][0],16,16);
                    log_text(log_fd_mv, "    uv  ");
                    log_reference_txt(log_fd_mv,&thiz->buv.uv[0][0],16,8);
                }
            }
            #endif

            #endif



            #ifdef __dump_DSP_test_data__
            int itt=0,jtt=0;
            uint8_t* ptt=pdesy+(mbindex<<4);

            if(pMB->intrapredmc_type&useMC)
            {
                //y
                for(itt=0;itt<16;itt++,ptt+=s->linesize)
                {
                    memcpy(&thiz->result.y[itt][0],ptt,16);
                }

                ptt=pdesuv+(mbindex<<4);
                for(itt=0;itt<8;itt++)
                {
                    for(jtt=0;jtt<8;jtt++)
                    {
                        thiz->result.u[itt][jtt]=ptt[jtt*2];
                        thiz->result.v[itt][jtt]=ptt[jtt*2+1];
                    }
                    ptt+=s->uvlinesize;
                }
            }
            log_dump(log_fd_result,&thiz->result,sizeof(RVDEC_MB_RESULT_t));

           #ifdef __log_mv__
                if(pMB->intrapredmc_type&useMC)
                {
                    //result
                    log_text(log_fd_mv, "result is ");
                    log_text(log_fd_mv, "    y  ");
                    log_reference_txt(log_fd_mv,&thiz->result.y[0][0],16,16);
                    log_text(log_fd_mv, "    u  ");
                    log_reference_txt(log_fd_mv,&thiz->result.u[0][0],8,8);
                    log_text(log_fd_mv, "    v  ");
                    log_reference_txt(log_fd_mv,&thiz->result.v[0][0],8,8);
                }
                #endif

            #endif



        }



        pMB=p_mc_intrapred->pMBInfo;

        //process intrapred and add idct result
        for(mbindex=0;mbindex<mbwidth;++mbindex,++pMB,pdesy+=16,pdesuv+=16,y+=256,uv+=128)
        {
            if(pMB->intrapredmc_type&useIntrapred)
            {
                #ifdef __log_intrapred__
                    char tstr[40];
                    snprintf(tstr,39,"MBintrapred_x=%d_y=%d ",mbindex,mbrow);
                    log_text(log_fd_intrapred, tstr);
                #endif
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_intrapred] || log_mask[decoding_config_add_idct])
                #endif
                process_intrapred_add_idct(thiz,pMB,pdesy,pdesuv,y,uv);
            }
            /*else
            {
                //add inverse transform result
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_add_idct])
                #endif
                add_result_mb_c(pdesy,pdesuv,y,uv,stride);
            }*/

            //dump after add inverse transform result
            #ifdef __dump_mb_data__
                if(mbindex>=log_start_mb_x && mbindex<=log_end_mb_x && mbrow>=log_start_mb_y && mbrow<=log_end_mb_y )
                {
                    char pmbstr[80];
                    snprintf(pmbstr,79,"MBadd_x=%d_y=%d_amba",mbindex,mbrow);
                    log_openfile(log_fd_mb_data,pmbstr);
                    log_dump_rect(log_fd_mb_data, pdesy, 16, 16,stride);
                    log_dump_rect(log_fd_mb_data, pdesuv, 16, 8,stride);
                    log_closefile(log_fd_mb_data);
                }
            #endif

        }

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"mc_intraprediction_thread done a row=%d, thiz->p_mc_intrapred_free_queue->getcnt=%d.\n",mbrow,thiz->p_mc_intrapred_free_queue->getcnt(thiz->p_mc_intrapred_free_queue));
        #endif

        //done, put to free queue
        thiz->p_mc_intrapred_free_queue->enqueue(thiz->p_mc_intrapred_free_queue,p_mc_intrapred);

        //trigger deblock process
        if(mbrow>0)
        {
            trigger_deblock(thiz,mbrow-1);
            if(mbrow==(thiz->s.mb_height-1))
                trigger_deblock(thiz,mbrow);
        }
    }

    //decoder exit, send frame done, too
//    thiz->p_thread_exit->enqueue(thiz->p_thread_exit,NULL);
    //need indicate next quit
    thiz->deblock_loop=0;
    thiz->p_deblock_queue->enqueue(thiz->p_deblock_queue,NULL);

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"mc_intraprediction_thread exit.\n");
    #endif
//    av_log(NULL,AV_LOG_ERROR,"mc_intraprediction_thread exit.\n");

    pthread_exit(NULL);
    return NULL;
}

void* deblock_thread(void* p)
{
    RV34AmbaDecContext* thiz=(RV34AmbaDecContext*)p;
    MBLineDeblock* p_deblock;

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"deblock_thread start.\n");
    #endif

    while(thiz->deblock_loop)
    {
//        thiz->deblock_wait=1;
        p_deblock=thiz->p_deblock_queue->dequeue(thiz->p_deblock_queue);
//        thiz->deblock_wait=0;
        if(!thiz->deblock_loop || !p_deblock)
        {
//            ambadec_assert_ffmpeg(!p_deblock && !thiz->deblock_loop);

            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"*error* deblock_thread quit after get packet=%p, thiz->deblock_loop=%d.\n",p_deblock,thiz->deblock_loop);
            #endif
//            av_log(NULL,AV_LOG_ERROR,"get NULL data or loop=0 in deblock_thread start exit.\n");

            break;
        }

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"*count* deblock_thread process mbrow=%d.\n",p_deblock->mbrow);
        #endif

        //process deblock
        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_deblock])
        #endif
        thiz->loop_filter(thiz,p_deblock->mbrow);

        //put to free queue
        thiz->p_deblock_free_queue->enqueue(thiz->p_deblock_free_queue,p_deblock);

        //send frame done signal
        if((thiz->s.mb_height-1)==p_deblock->mbrow)
        {
            thiz->p_frame_done->enqueue(thiz->p_frame_done,NULL);
        }

    }

    //thread exit
//    thiz->p_thread_exit->enqueue(thiz->p_thread_exit,NULL);
    //need indicate frame done for exit
//    av_log(NULL,AV_LOG_ERROR,"deblock_thread push decode_frame exit.\n");
    thiz->p_frame_done->enqueue(thiz->p_frame_done,NULL);

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"deblock_thread exit.\n");
    #endif
//    av_log(NULL,AV_LOG_ERROR,"deblock_thread exit.\n");

    pthread_exit(NULL);
    return NULL;
}

#ifndef _remove_block_
static inline void _zero_sublock_y(DCTELEM* p)
{
    int* pt=(int*)p;
    pt[0]=0;pt[1]=0;
    pt[8]=0;pt[9]=0;
    pt[16]=0;pt[17]=0;
    pt[24]=0;pt[25]=0;
}

static inline void _zero_sublock_uv(DCTELEM* p)
{
    p[0]=p[2]=p[4]=p[6]=0;
    p[16]=p[18]=p[20]=p[22]=0;
    p[32]=p[34]=p[36]=p[38]=0;
    p[48]=p[50]=p[52]=p[54]=0;
}
#endif

static inline void rv34_pred_4x4_block_uv(MBInfoMCIntraPred* p_mbinfo,int index, int itype, int up, int left, int down, int right)
{
    if(!up && !left)
        itype = DC_128_PRED;
    else if(!up){
        if(itype == VERT_PRED) itype = HOR_PRED;
        if(itype == DC_PRED)   itype = LEFT_DC_PRED;
    }else if(!left){
        if(itype == HOR_PRED)  itype = VERT_PRED;
        if(itype == DC_PRED)   itype = TOP_DC_PRED;
        if(itype == DIAG_DOWN_LEFT_PRED) itype = DIAG_DOWN_LEFT_PRED_RV40_NODOWN;
    }
    if(!down){
        if(itype == DIAG_DOWN_LEFT_PRED) itype = DIAG_DOWN_LEFT_PRED_RV40_NODOWN;
        if(itype == HOR_UP_PRED) itype = HOR_UP_PRED_RV40_NODOWN;
        if(itype == VERT_LEFT_PRED) itype = VERT_LEFT_PRED_RV40_NODOWN;
    }
    if(!right && up){
        p_mbinfo->intrapredmc_type|=1<<(index+28);
    }

    p_mbinfo->info.intrapred.intratype[index+16]=itype;
}

/**
 * Perform 4x4 intra prediction.
 */
 static inline void rv34_pred_4x4_block_y_amba(MBInfoMCIntraPred* p_mbinfo,int index,int itype, int up, int left, int down, int right)
{

    if(!up && !left)
        itype = DC_128_PRED;
    else if(!up){
        if(itype == VERT_PRED) itype = HOR_PRED;
        if(itype == DC_PRED)   itype = LEFT_DC_PRED;
    }else if(!left){
        if(itype == HOR_PRED)  itype = VERT_PRED;
        if(itype == DC_PRED)   itype = TOP_DC_PRED;
        if(itype == DIAG_DOWN_LEFT_PRED) itype = DIAG_DOWN_LEFT_PRED_RV40_NODOWN;
    }
    if(!down){
        if(itype == DIAG_DOWN_LEFT_PRED) itype = DIAG_DOWN_LEFT_PRED_RV40_NODOWN;
        if(itype == HOR_UP_PRED) itype = HOR_UP_PRED_RV40_NODOWN;
        if(itype == VERT_LEFT_PRED) itype = VERT_LEFT_PRED_RV40_NODOWN;
    }
    if(!right && up){
        p_mbinfo->intrapredmc_type|=1<<(index+12);
    }
    p_mbinfo->info.intrapred.intratype[index]=itype;

}

static void rv34_amba_output_macroblock(RV34AmbaDecContext *r, int8_t *intra_types,int is16)
{
    MpegEncContext *s = &r->s;
    DSPContext *dsp = &s->dsp;
    int i, j;
//    uint8_t *Y, *UV;
    int itype;
    int avail[6*8] = {0};
    int idx;
    MBInfoMCIntraPred* p_mbinfo=r->p_mbinfo;
    int stride=r->p_mc_intrapred->stride;

    // Set neighbour information.
    if(r->avail_cache[1])
        avail[0] = 1;
    if(r->avail_cache[2])
        avail[1] = avail[2] = 1;
    if(r->avail_cache[3])
        avail[3] = avail[4] = 1;
    if(r->avail_cache[4])
        avail[5] = 1;
    if(r->avail_cache[5])
        avail[8] = avail[16] = 1;
    if(r->avail_cache[9])
        avail[24] = avail[32] = 1;

#if 0
//#ifdef __dump_temp__
    char tmpchr[40];
    snprintf(tmpchr,39,"MB x=%d,y=%d",r->s.mb_x,r->s.mb_y);
    log_text(log_fd_temp,tmpchr);
    log_text_rect_int(log_fd_temp,r->avail_cache,4,3,0);

    log_text_rect_int(log_fd_temp,avail,8,6,0);
#endif

    #ifdef __log_dump_data__
    //int diff=s->dest[1]-s->current_picture_ptr->data[1];
    //log_dump(2,&diff,sizeof(int));
    #endif
    if(!is16){
        p_mbinfo->intrapredmc_type|=useIntrapred;
        for(j = 0; j < 4; j++){
            idx = 9 + j*8;
            for(i = 0; i < 4; i++,  idx++){
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_intrapred])
                #endif
                rv34_pred_4x4_block_y_amba(r->p_mbinfo,(j<<2)+i,ittrans[intra_types[i]], avail[idx-8], avail[idx-1], avail[idx+7], avail[idx-7]);
                avail[idx] = 1;

            }
            intra_types += r->intra_types_stride;
        }
        intra_types -= r->intra_types_stride * 4;
        fill_rectangle(r->avail_cache + 6, 2, 2, 4, 0, 4);
        for(j = 0; j < 2; j++){
            idx = 6 + j*4;
            for(i = 0; i < 2; i++, idx++){
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_intrapred])
                #endif
                {
                    rv34_pred_4x4_block_uv(r->p_mbinfo,(j<<1)+i, ittrans[intra_types[i*2+j*2*r->intra_types_stride]], r->avail_cache[idx-4], r->avail_cache[idx-1], !i && !j, r->avail_cache[idx-3]);
//                rv34_pred_4x4_block_amba(r, V + (i<<3) + j*4*s->uvlinesize, s->uvlinesize, ittrans[intra_types[i*2+j*2*r->intra_types_stride]], r->avail_cache[idx-4], r->avail_cache[idx-1], !i && !j, r->avail_cache[idx-3]);
                }
                r->avail_cache[idx] = 1;
            }
        }
    }else{
        p_mbinfo->intrapredmc_type|=useIntrapred|IntraPred16x16;
        itype = ittrans16[intra_types[0]];
        p_mbinfo->info.intrapred.intratype[0] = adjust_pred16(itype, r->avail_cache[6-4], r->avail_cache[6-1]);

        itype = ittrans16[intra_types[0]];
        if(itype == PLANE_PRED8x8) itype = DC_PRED8x8;
        p_mbinfo->info.intrapred.intratype[1]  = adjust_pred16(itype, r->avail_cache[6-4], r->avail_cache[6-1]);

    }
}

/**
 * Decode motion vector differences
 * and perform motion vector reconstruction and motion compensation.
 */
static int rv34_amba_decode_mv(RV34AmbaDecContext *r, int block_type)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int i, j, k, l;
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int next_bt;

    memset(r->dmv, 0, sizeof(r->dmv));
    for(i = 0; i < num_mvs[block_type]; i++){
        r->dmv[i][0] = svq3_get_se_golomb(gb);
        r->dmv[i][1] = svq3_get_se_golomb(gb);
    }

    #ifdef __log_mc__
        char tstr[60];
        snprintf(tstr,59,"mc type=%d ",block_type);
        log_text(log_fd_mc, tstr);
    #endif

    switch(block_type){
    case RV34_MB_TYPE_INTRA:
    case RV34_MB_TYPE_INTRA16x16:
        ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);

        #ifdef __log_mc__
            log_text(log_fd_mc, "intra mb ");
        #endif

        return 0;
    case RV34_MB_SKIP:
        if(s->pict_type == FF_P_TYPE){
            ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);

            #ifdef __log_mc__
                log_text(log_fd_mc,"RV34_MB_SKIP ");
            #endif

            #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
            if(log_mask[decoding_config_mc])
            #endif
            //rv34_amba_mc_1mv (r, block_type, 0, 0, 0, 2, 2, 0);
            {
            // parallel related: 1 mv, forward
                r->p_mbinfo->intrapredmc_type |=useMC|MCForward;
                ambadec_assert_ffmpeg(!s->current_picture_ptr->motion_val[0][mv_pos][0]);
                ambadec_assert_ffmpeg(!s->current_picture_ptr->motion_val[0][mv_pos][1]);
                r->p_mbinfo->info.mc.mvx[0]=0;// s->current_picture_ptr->motion_val[0][mv_pos][0];
                r->p_mbinfo->info.mc.mvy[0] =0;// s->current_picture_ptr->motion_val[0][mv_pos][1];
            }
            #endif

            break;
        }
    case RV34_MB_B_DIRECT:
        //surprisingly, it uses motion scheme from next reference frame
        next_bt = s->next_picture_ptr->mb_type[s->mb_x + s->mb_y * s->mb_stride];
        if(IS_INTRA(next_bt) || IS_SKIP(next_bt)){
            ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);
            ZERO8x2(s->current_picture_ptr->motion_val[1][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);
        }else
            for(j = 0; j < 2; j++)
                for(i = 0; i < 2; i++)
                    for(k = 0; k < 2; k++)
                        for(l = 0; l < 2; l++)
                            s->current_picture_ptr->motion_val[l][mv_pos + i + j*s->b8_stride][k] = calc_add_mv(r, l, s->next_picture_ptr->motion_val[0][mv_pos + i + j*s->b8_stride][k]);
        if(!(IS_16X8(next_bt) || IS_8X16(next_bt) || IS_8X8(next_bt))) //we can use whole macroblock MC
        {

            #ifdef __log_mc__
                log_text(log_fd_mc,"RV34_MB_B_DIRECT whole mb ");
            #endif

        #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
            if(log_mask[decoding_config_mc])
            #endif
            //rv34_amba_mc_2mv(r, block_type);
            {
                // parallel related: 1 mv, Bidirectional
                r->p_mbinfo->intrapredmc_type |=useMC|MCBidirectional;
                r->p_mbinfo->info.mc.mvx[0]= s->current_picture_ptr->motion_val[0][mv_pos][0];
                r->p_mbinfo->info.mc.mvy[0] = s->current_picture_ptr->motion_val[0][mv_pos][1];
                r->p_mbinfo->info.mc.mvx[1]= s->current_picture_ptr->motion_val[1][mv_pos][0];
                r->p_mbinfo->info.mc.mvy[1] = s->current_picture_ptr->motion_val[1][mv_pos][1];
            }
        #endif
        }
        else
        {
            #ifdef __log_mc__
                log_text(log_fd_mc,"RV34_MB_B_DIRECT 4mv bi-directional ");
            #endif

         #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
            if(log_mask[decoding_config_mc])
            #endif
//            rv34_amba_mc_2mv_skip(r);
            {
                // parallel related: 4 mv, Bidirectional ?
                r->p_mbinfo->intrapredmc_type |=useMC|MCBidirectional|MC4MV;
                k=0;
                int cpos=0;
                for(j=0;j<2;j++)
                {
                    for(i=0;i<2;i++)
                    {
                        cpos=mv_pos+i+j*s->b8_stride;
                        r->p_mbinfo->info.mc.mvx[k]= s->current_picture_ptr->motion_val[0][cpos][0];
                        r->p_mbinfo->info.mc.mvy[k] = s->current_picture_ptr->motion_val[0][cpos][1];
                        r->p_mbinfo->info.mc.mvx[k+4]= s->current_picture_ptr->motion_val[1][cpos][0];
                        r->p_mbinfo->info.mc.mvy[k+4] = s->current_picture_ptr->motion_val[1][cpos][1];
                        k++;
                    }
                }
            }
         #endif
        }
        ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);
        break;
    case RV34_MB_P_16x16:
    case RV34_MB_P_MIX16x16:
        rv34_pred_mv(r, block_type, 0, 0);

        #ifdef __log_mc__
            log_text(log_fd_mc,"RV34_MB_P_16x16");
        #endif

        #ifndef __commit_out_motion_comp__
        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_mc])
        #endif
        //rv34_amba_mc_1mv (r, block_type, 0, 0, 0, 2, 2, 0);
        {
            // parallel related: 1 mv, forward
            r->p_mbinfo->intrapredmc_type |=useMC|MCForward;
            r->p_mbinfo->info.mc.mvx[0]= s->current_picture_ptr->motion_val[0][mv_pos][0];
            r->p_mbinfo->info.mc.mvy[0] = s->current_picture_ptr->motion_val[0][mv_pos][1];
        }
        #endif
        break;
    case RV34_MB_B_FORWARD:
    case RV34_MB_B_BACKWARD:
        r->dmv[1][0] = r->dmv[0][0];
        r->dmv[1][1] = r->dmv[0][1];
        if(r->rv30)
            rv34_pred_mv_rv3(r, block_type, block_type == RV34_MB_B_BACKWARD);
        else
            rv34_pred_mv_b  (r, block_type, block_type == RV34_MB_B_BACKWARD);

        #ifdef __log_mc__
            log_text(log_fd_mc,"RV34_MB_B_FORWARD or RV34_MB_B_BACKWARD");
        #endif

        #ifndef __commit_out_motion_comp__
        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_mc])
        #endif
        //rv34_amba_mc_1mv     (r, block_type, 0, 0, 0, 2, 2, block_type == RV34_MB_B_BACKWARD);
        {
            // parallel related: 1 mv, forward or backward
            if(block_type == RV34_MB_B_FORWARD)
            {
                r->p_mbinfo->intrapredmc_type |=useMC|MCForward;
                r->p_mbinfo->info.mc.mvx[0]= s->current_picture_ptr->motion_val[0][mv_pos][0];
                r->p_mbinfo->info.mc.mvy[0] = s->current_picture_ptr->motion_val[0][mv_pos][1];
            }
            else
            {
                r->p_mbinfo->intrapredmc_type |=useMC|MCBackward;
                r->p_mbinfo->info.mc.mvx[0]= s->current_picture_ptr->motion_val[1][mv_pos][0];
                r->p_mbinfo->info.mc.mvy[0] = s->current_picture_ptr->motion_val[1][mv_pos][1];
            }
        }
        #endif
        break;
    case RV34_MB_P_16x8:
    case RV34_MB_P_8x16:
        rv34_pred_mv(r, block_type, 0, 0);
        rv34_pred_mv(r, block_type, 1 + (block_type == RV34_MB_P_16x8), 1);
        if(block_type == RV34_MB_P_16x8){

            #ifdef __log_mc__
                log_text(log_fd_mc,"RV34_MB_P_16x8");
            #endif

            #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
            if(log_mask[decoding_config_mc])
            #endif
//            {rv34_amba_mc_1mv(r, block_type, 0, 0, 0,            2, 1, 0);
//           rv34_amba_mc_1mv(r, block_type, 0, 8, s->b8_stride, 2, 1, 0);}
            {
                // parallel related: 4 mv, forward
                r->p_mbinfo->intrapredmc_type |=useMC|MCForward|MC4MV;

                r->p_mbinfo->info.mc.mvx[0]=r->p_mbinfo->info.mc.mvx[1]= s->current_picture_ptr->motion_val[0][mv_pos][0];
                r->p_mbinfo->info.mc.mvy[0]=r->p_mbinfo->info.mc.mvy[1] = s->current_picture_ptr->motion_val[0][mv_pos][1];

                r->p_mbinfo->info.mc.mvx[2]=r->p_mbinfo->info.mc.mvx[3]= s->current_picture_ptr->motion_val[0][mv_pos+s->b8_stride][0];
                r->p_mbinfo->info.mc.mvy[2]=r->p_mbinfo->info.mc.mvy[3] = s->current_picture_ptr->motion_val[0][mv_pos+s->b8_stride][1];
            }
            #endif
        }
        if(block_type == RV34_MB_P_8x16){

            #ifdef __log_mc__
                log_text(log_fd_mc,"RV34_MB_P_8x16");
            #endif

            #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
            if(log_mask[decoding_config_mc])
            #endif
            //{rv34_amba_mc_1mv(r, block_type, 0, 0, 0, 1, 2, 0);
            //rv34_amba_mc_1mv(r, block_type, 8, 0, 1, 1, 2, 0);}
            {
                // parallel related: 4 mv, forward
                r->p_mbinfo->intrapredmc_type |=useMC|MCForward|MC4MV;

                r->p_mbinfo->info.mc.mvx[0]=r->p_mbinfo->info.mc.mvx[2]= s->current_picture_ptr->motion_val[0][mv_pos][0];
                r->p_mbinfo->info.mc.mvy[0]=r->p_mbinfo->info.mc.mvy[2] = s->current_picture_ptr->motion_val[0][mv_pos][1];

                r->p_mbinfo->info.mc.mvx[1]=r->p_mbinfo->info.mc.mvx[3]= s->current_picture_ptr->motion_val[0][mv_pos+1][0];
                r->p_mbinfo->info.mc.mvy[1]=r->p_mbinfo->info.mc.mvy[3] = s->current_picture_ptr->motion_val[0][mv_pos+1][1];
            }
            #endif
        }
        break;
    case RV34_MB_B_BIDIR:
        rv34_pred_mv_b  (r, block_type, 0);
        rv34_pred_mv_b  (r, block_type, 1);

        #ifdef __log_mc__
            log_text(log_fd_mc,"RV34_MB_B_BIDIR");
        #endif

        #ifndef __commit_out_motion_comp__
        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_mc])
        #endif
        //rv34_amba_mc_2mv     (r, block_type);
        {
            // parallel related: 1 mv, Bidirectional
            r->p_mbinfo->intrapredmc_type |=useMC|MCBidirectional;
            r->p_mbinfo->info.mc.mvx[0]= s->current_picture_ptr->motion_val[0][mv_pos][0];
            r->p_mbinfo->info.mc.mvy[0] = s->current_picture_ptr->motion_val[0][mv_pos][1];
            r->p_mbinfo->info.mc.mvx[1]= s->current_picture_ptr->motion_val[1][mv_pos][0];
            r->p_mbinfo->info.mc.mvy[1] = s->current_picture_ptr->motion_val[1][mv_pos][1];
        }
        #endif
        break;
    case RV34_MB_P_8x8:

        #ifdef __log_mc__
            log_text(log_fd_mc,"RV34_MB_P_8x8");
        #endif

        for(i=0;i< 4;i++){
            rv34_pred_mv(r, block_type, i, i);
            #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
            if(log_mask[decoding_config_mc])
            #endif
            //rv34_amba_mc_1mv (r, block_type, (i&1)<<3, (i&2)<<2, (i&1)+(i>>1)*s->b8_stride, 1, 1, 0);
            {
                // parallel related: 1 mv, forward
                r->p_mbinfo->info.mc.mvx[i]= s->current_picture_ptr->motion_val[0][mv_pos+(i&1)+(i>>1)*s->b8_stride][0];
                r->p_mbinfo->info.mc.mvy[i] = s->current_picture_ptr->motion_val[0][mv_pos+(i&1)+(i>>1)*s->b8_stride][1];
            }
            #endif
        }
        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_mc])
        #endif
            r->p_mbinfo->intrapredmc_type |=useMC|MCForward|MC4MV;
        break;
    }

    return 0;
}

static int rv34_amba_decode_mb_header(RV34AmbaDecContext *r, int8_t *intra_types)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int i, t;

    if(!r->si.type){
        r->is16 = get_bits1(gb);
        if(!r->is16 && !r->rv30){
            if(!get_bits1(gb))
                av_log(s->avctx, AV_LOG_ERROR, "Need DQUANT\n");
        }
        s->current_picture_ptr->mb_type[mb_pos] = r->is16 ? MB_TYPE_INTRA16x16 : MB_TYPE_INTRA;
        r->block_type = r->is16 ? RV34_MB_TYPE_INTRA16x16 : RV34_MB_TYPE_INTRA;
    }else{
        r->block_type = r->decode_mb_info(r);
        if(r->block_type == -1)
            return -1;
        s->current_picture_ptr->mb_type[mb_pos] = rv34_mb_type_to_lavc[r->block_type];
        r->mb_type[mb_pos] = r->block_type;
        if(r->block_type == RV34_MB_SKIP){
            if(s->pict_type == FF_P_TYPE)
                r->mb_type[mb_pos] = RV34_MB_P_16x16;
            if(s->pict_type == FF_B_TYPE)
                r->mb_type[mb_pos] = RV34_MB_B_DIRECT;
        }
        r->is16 = !!IS_INTRA16x16(s->current_picture_ptr->mb_type[mb_pos]);
        rv34_amba_decode_mv(r, r->block_type);
        if(r->block_type == RV34_MB_SKIP){
            fill_rectangle(intra_types, 4, 4, r->intra_types_stride, 0, sizeof(intra_types[0]));
            return 0;
        }
        r->chroma_vlc = 1;
        r->luma_vlc   = 0;
    }
    if(IS_INTRA(s->current_picture_ptr->mb_type[mb_pos])){
        if(r->is16){
            t = get_bits(gb, 2);
            fill_rectangle(intra_types, 4, 4, r->intra_types_stride, t, sizeof(intra_types[0]));
            r->luma_vlc   = 2;
        }else{
            if(r->decode_intra_types(r, gb, intra_types) < 0)
                return -1;
            r->luma_vlc   = 1;
        }
        r->chroma_vlc = 0;
        r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 0);
    }else{
        for(i = 0; i < 16; i++)
            intra_types[(i & 3) + (i>>2) * r->intra_types_stride] = 0;
        r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 1);
        if(r->mb_type[mb_pos] == RV34_MB_P_MIX16x16){
            r->is16 = 1;
            r->chroma_vlc = 1;
            r->luma_vlc   = 2;
            r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 0);
        }
    }

    return rv34_decode_cbp(gb, r->cur_vlcs, r->is16);
}

static int rv34_amba_decode_macroblock(RV34AmbaDecContext *r, int8_t *intra_types)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int cbp, cbp2;

    #ifndef _remove_block_
    int  blknum, blkoff;
    #endif
    int i;

    DCTELEM block16[64];
    int luma_dc_quant;
    int dist;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int j=0,k=0;

    //parallel related
    r->p_mbinfo=&r->p_mc_intrapred->pMBInfo[s->mb_x];
    r->p_mbinfo->intrapredmc_type=0;
    DCTELEM* pmby=r->p_mc_intrapred->p_idct_result_y+(s->mb_x<<8);
    DCTELEM* pmbuv=r->p_mc_intrapred->p_idct_result_uv+(s->mb_x<<7);
    DCTELEM* ptmp;
//    int* pts,*ptd;

    #ifdef __log_dump_idct_data__
    if(s->mb_x>=log_start_mb_x && s->mb_x<=log_end_mb_x && s->mb_y>=log_start_mb_y && s->mb_y<=log_end_mb_y )
    {
        char str[40];
        snprintf(str,39,"MBidct_x=%d_y=%d ",s->mb_x,s->mb_y);
        log_text(log_fd_idct_text, str);
    }
    #endif

    #ifdef __log_mc__
        char strtt[40];
        snprintf(strtt,39,"MB_x=%d_y=%d ",s->mb_x,s->mb_y);
        log_text(log_fd_mc, strtt);
    #endif

    #ifdef __dump_deblock_DSP_TXT__
        char txtch[80];
        snprintf(txtch,79,">>>>>>>>>>>>>>>>>>>> [MB] = [%d  %d] <<<<<<<<<<<<<<<<<<<<",s->mb_x,s->mb_y);
        log_text(log_fd_dsp_text,txtch);
    #endif


    // Calculate which neighbours are available. Maybe it's worth optimizing too.
    memset(r->avail_cache, 0, sizeof(r->avail_cache));
    fill_rectangle(r->avail_cache + 6, 2, 2, 4, 1, 4);
    dist = (s->mb_x - s->resync_mb_x) + (s->mb_y - s->resync_mb_y) * s->mb_width;
    if(s->mb_x && dist)
        r->avail_cache[5] =
        r->avail_cache[9] = s->current_picture_ptr->mb_type[mb_pos - 1];
    if(dist >= s->mb_width)
        r->avail_cache[2] =
        r->avail_cache[3] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride];
    if(((s->mb_x+1) < s->mb_width) && dist >= s->mb_width - 1)
        r->avail_cache[4] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride + 1];
    if(s->mb_x && dist > s->mb_width)
        r->avail_cache[1] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride - 1];

    s->qscale = r->si.quant;
    cbp = cbp2 = rv34_amba_decode_mb_header(r, intra_types);
    r->cbp_luma  [mb_pos] = cbp;
    r->cbp_chroma[mb_pos] = cbp >> 16;
    if(s->pict_type == FF_I_TYPE)
        r->deblock_coefs[mb_pos] = 0xFFFF;
    else
        r->deblock_coefs[mb_pos] = rv34_amba_set_deblock_coef(r) | r->cbp_luma[mb_pos];
    s->current_picture_ptr->qscale_table[mb_pos] = s->qscale;

    if(cbp == -1)
        return -1;

    luma_dc_quant = r->block_type == RV34_MB_P_MIX16x16 ? r->luma_dc_quant_p[s->qscale] : r->luma_dc_quant_i[s->qscale];

    #ifdef __dump_DSP_TXT__
        if(r->mb_type[mb_pos]==RV34_MB_TYPE_INTRA)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  INTRA  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_TYPE_INTRA16x16)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  INTRA16x16  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_P_16x16)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  P_16x16  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_P_8x8)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  P_8x8  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_B_FORWARD)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  B_FWD  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_B_BACKWARD)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  B_BWD  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_SKIP)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  SKIP  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_B_DIRECT)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  B_DIRECT  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_P_16x8)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  P_16x8  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_P_8x16)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  P_8x16  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_B_BIDIR)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  B_BIDIR  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_P_MIX16x16)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  P_MIX16x16  %d %d]",s->qscale,cbp);

        log_text(log_fd_dsp_text,txtch);
        snprintf(txtch,79,"16x16: dc q=%d, ac q=%d; 4v4 luma dc q=%d, ac q=%d; 4x4 chroma dc q=%d, ac q=%d;"\
                    ,rv34_qscale_tab[luma_dc_quant],rv34_qscale_tab[s->qscale]\
                    ,rv34_qscale_tab[s->qscale],rv34_qscale_tab[s->qscale]\
                    ,rv34_qscale_tab[rv34_chroma_quant[1][s->qscale]],rv34_qscale_tab[rv34_chroma_quant[0][s->qscale]]);
        log_text(log_fd_dsp_text,txtch);
    #endif

    if(r->is16){
        memset(block16, 0, sizeof(block16));
        rv34_amba_decode_block(block16, gb, r->cur_vlcs, 3, 0);

        #ifdef __dump_DSP_TXT__
        log_text(log_fd_dsp_text,"{ 16x16 SubBlock:}");
        log_text(log_fd_dsp_text,"<< Before Inverse Quantization: >>");
        log_text_rect_short(log_fd_dsp_text, block16, 4, 4, 4);
        #endif

        #ifndef __commit_out_idct_dequant__

        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_idct_deq])
        #endif
        {
        rv34_amba_dequant4x4_16x16(block16, rv34_qscale_tab[luma_dc_quant],rv34_qscale_tab[s->qscale]);

        #ifdef __dump_DSP_TXT__
        log_text(log_fd_dsp_text,"<< After Inverse Quantization: >>");
        log_text_rect_short(log_fd_dsp_text, block16, 4, 4, 4);
        #endif

        #ifdef _config_rv40_neon_
        r->neon.trans[trans_type_dc](block16);
        #else
        rv34_amba_inv_transform_noround(block16);
        #endif

        #ifdef __dump_DSP_TXT__
        log_text(log_fd_dsp_text,"<< After Inverse transform: >>");
        log_text_rect_short_hex(log_fd_dsp_text, block16, 4, 4, 4);
        #endif

        }
        #endif
    }

    for(i = 0; i < 16; i++, cbp >>= 1){
        ptmp=pmby+((i&0xc)<<4)+((i&0x3)<<2);
        if(!r->is16 && !(cbp & 1))
        {
            #ifndef _remove_block_
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_idct_deq])
                #endif
                _zero_sublock_y(ptmp);
            #endif
            continue;
        }

        #ifndef _remove_block_
        blknum = ((i & 2) >> 1) + ((i & 8) >> 2);
        blkoff = ((i & 1) << 2) + ((i & 4) << 3);
        #endif

        #ifdef __dump_DSP_TXT__
        snprintf(txtch,79,"{ SubBlock: %d }",i);
        log_text(log_fd_dsp_text,txtch);
        #endif

        if(cbp & 1)
        {
            #ifdef _remove_block_
                rv34_amba_decode_block_y(ptmp, gb, r->cur_vlcs, r->luma_vlc, 0);

                #ifdef __dump_DSP_TXT__
                log_text(log_fd_dsp_text,"<< Before Inverse Quantization: >>");
                log_text_rect_short(log_fd_dsp_text, ptmp, 4, 4, 12);
                #endif

            #else
                rv34_amba_decode_block(s->block[blknum] + blkoff, gb, r->cur_vlcs, r->luma_vlc, 0);
            #endif
        }

        #ifndef __commit_out_idct_dequant__

        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_idct_deq])
        #endif
        {
            #ifdef _remove_block_
                rv34_amba_dequant4x4_y(ptmp, rv34_qscale_tab[s->qscale],rv34_qscale_tab[s->qscale]);
                if(r->is16) //FIXME: optimize
                    *ptmp = block16[(i & 3) | ((i & 0xC) << 1)];

                #ifdef __dump_DSP_TXT__
                log_text(log_fd_dsp_text,"<< After Inverse Quantization and add DC: >>");
                log_text_rect_short(log_fd_dsp_text, ptmp, 4, 4, 12);
                #endif

                #ifdef _config_rv40_neon_
                r->neon.trans[trans_type_16x16](ptmp);
                #else
                rv34_amba_inv_transform_y(ptmp);
                #endif

                #ifdef __dump_DSP_TXT__
                log_text(log_fd_dsp_text,"<< After Inverse transform: >>");
                log_text_rect_short_hex(log_fd_dsp_text, ptmp, 4, 4, 12);
                #endif

            #else
                rv34_amba_dequant4x4(s->block[blknum] + blkoff, rv34_qscale_tab[s->qscale],rv34_qscale_tab[s->qscale]);
                if(r->is16) //FIXME: optimize
                    s->block[blknum][blkoff] = block16[(i & 3) | ((i & 0xC) << 1)];
                rv34_amba_inv_transform_new_y(s->block[blknum] + blkoff,ptmp);
            #endif
        }

        #endif

    }
    if(r->block_type == RV34_MB_P_MIX16x16)
        r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 1);

    for(; i < 24; i++, cbp >>= 1){
        ptmp=pmbuv+((i&0x2)<<5)+((i&0x1)<<3)+(!(i<20));
        if(!(cbp & 1))
        {
            #ifndef _remove_block_
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_idct_deq])
                #endif
                _zero_sublock_uv(ptmp);
            #endif
            continue;
        }

        #ifdef __dump_DSP_TXT__
        snprintf(txtch,79,"{ SubBlock: %d }",i);
        log_text(log_fd_dsp_text,txtch);
        #endif

        #ifdef _remove_block_
        rv34_amba_decode_block_uv(ptmp, gb, r->cur_vlcs, r->chroma_vlc, 1);

            #ifdef __dump_DSP_TXT__
            log_text(log_fd_dsp_text,"<< Before Inverse Quantization: >>");
            log_text_rect_short_chroma(log_fd_dsp_text, ptmp, 4, 4, 12);
            #endif

        #else
        blknum = ((i & 4) >> 2) + 4;
        blkoff = ((i & 1) << 2) + ((i & 2) << 4);
        rv34_amba_decode_block(s->block[blknum] + blkoff, gb, r->cur_vlcs, r->chroma_vlc, 1);
        #endif

        #ifndef __commit_out_idct_dequant__

        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_idct_deq])
        #endif
        {
            #ifdef _remove_block_
                rv34_amba_dequant4x4_uv(ptmp, rv34_qscale_tab[rv34_chroma_quant[1][s->qscale]],rv34_qscale_tab[rv34_chroma_quant[0][s->qscale]]);

                #ifdef __dump_DSP_TXT__
                log_text(log_fd_dsp_text,"<< After Inverse Quantization: >>");
                log_text_rect_short_chroma(log_fd_dsp_text, ptmp, 4, 4, 12);
                #endif

                //need implement uv interlave
                //#ifdef _config_rv40_neon_
                //r->neon.trans[trans_type_8x8](ptmp);
                //#else
                rv34_amba_inv_transform_uv(ptmp);
                //#endif

                #ifdef __dump_DSP_TXT__
                log_text(log_fd_dsp_text,"<< After Inverse transform: >>");
                log_text_rect_short_hex_chroma(log_fd_dsp_text, ptmp, 4, 4, 12);
                #endif

            #else
                rv34_amba_dequant4x4(s->block[blknum] + blkoff, rv34_qscale_tab[rv34_chroma_quant[1][s->qscale]],rv34_qscale_tab[rv34_chroma_quant[0][s->qscale]]);
                //rv34_amba_inv_transform(s->block[blknum] + blkoff);
                rv34_amba_inv_transform_new_uv(s->block[blknum] + blkoff,ptmp);
            #endif
        }
        #endif

    }

    #ifdef __log_dump_idct_data__
    if(s->mb_x>=log_start_mb_x && s->mb_x<=log_end_mb_x && s->mb_y>=log_start_mb_y && s->mb_y<=log_end_mb_y )
    {
        log_idct_txt(log_fd_idct_text,pmby,16);
        log_idct_txt_uv(log_fd_idct_text,pmbuv,8);
    }
    #endif

    #ifndef __remove_memory_op__
    {
        if(IS_INTRA(s->current_picture_ptr->mb_type[mb_pos]))
        {
            rv34_amba_output_macroblock(r, intra_types, r->is16);
            #ifdef __dump_DSP_test_data__
            #if 1
                //zero for dsp test
                memset(&r->residual,0,sizeof(RVDEC_MB_RESIDUAL_t));
                log_dump(log_fd_residual,&r->residual,sizeof(RVDEC_MB_RESIDUAL_t));
            #else
                //y
                log_dump(log_fd_residual,pmby,512);
                //u v
                int j=0;
                ptmp=pmbuv;
                for(i=0;i<8;i++)
                {
                    for(j=0;j<8;j++)
                    {
                        r->residual.u[i][j]=*ptmp++;
                        r->residual.v[i][j]=*ptmp++;
                    }
                }
                log_dump(log_fd_residual,r->residual.u,128);
                log_dump(log_fd_residual,r->residual.v,128);
            #endif
            uint8_t logmbtype=1;
            log_dump(log_fd_mbtype,&logmbtype,1);
            #endif
        }
        #ifdef __dump_DSP_test_data__
        else
        {
            //y
            log_dump(log_fd_residual,pmby,512);
            //u v
            int j=0;
            ptmp=pmbuv;
            for(i=0;i<8;i++)
            {
                for(j=0;j<8;j++)
                {
                    r->residual.u[i][j]=*ptmp++;
                    r->residual.v[i][j]=*ptmp++;
                }
            }
            log_dump(log_fd_residual,r->residual.u,128);
            log_dump(log_fd_residual,r->residual.v,128);
            uint8_t logmbtype=2;
            log_dump(log_fd_mbtype,&logmbtype,1);
        }
        #endif
    }
    #endif

    return 0;
}


static int rv34_amba_decode_slice(RV34AmbaDecContext *r, int end, const uint8_t* buf, int buf_size)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int mb_pos;
    int res;

    init_get_bits(&r->s.gb, buf, buf_size*8);
    res = r->parse_slice_header(r, gb, &r->si);
    if(res < 0){
        av_log(s->avctx, AV_LOG_ERROR, "Incorrect or unknown slice header\n");
        return -1;
    }

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR," rv34_amba_decode_slice in, s->mb_y=%d.\n",s->mb_y);
    #endif

    if ((s->mb_x == 0 && s->mb_y == 0) || s->current_picture_ptr==NULL) {
        if(s->width != r->si.width || s->height != r->si.height){
            av_log(s->avctx, AV_LOG_ERROR, "Changing dimensions to %dx%d\n", r->si.width,r->si.height);

            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"Changing dimensions to %dx%d\n", r->si.width,r->si.height);
            #endif
            void* pv1,*pv2;
            int ret2,ret1;
            MBLineMCIntraPred* pmc;
            MBLineDeblock* pdeblock;
            //parallel related
            //quit sub process thread
            r->mc_intrapred_loop=0;//r->deblock_loop=0;

            //need indicate next quit
            r->p_mc_intrapred_queue->enqueue(r->p_mc_intrapred_queue,NULL);
//            r->p_deblock_queue->enqueue(r->p_deblock_queue,NULL);

//            r->p_thread_exit->dequeue(r->p_thread_exit);
//            r->p_thread_exit->dequeue(r->p_thread_exit);

            ret1=pthread_join(r->tid_mc_intrapred,&pv1);
            ret2=pthread_join(r->tid_deblock,&pv2);


            #ifdef _d_use_DSP_format_
            if(r->pdspmv.pbase)
                av_free(r->pdspmv.pbase);
            r->pdspmv.p=r->pdspmv.b=r->pdspmv.pbase=NULL;
            #endif

            //free resources
            //free mc intrapred related
            while(r->p_mc_intrapred_free_queue->getcnt(r->p_mc_intrapred_free_queue))
            {
                pmc=r->p_mc_intrapred_free_queue->dequeue(r->p_mc_intrapred_free_queue);
                delete_mc_intrapred(pmc);
            }
            while(r->p_mc_intrapred_queue->getcnt(r->p_mc_intrapred_queue))
            {
                pmc=r->p_mc_intrapred_queue->dequeue(r->p_mc_intrapred_queue);
                delete_mc_intrapred(pmc);
            }

            //free deblock related
            while(r->p_deblock_free_queue->getcnt(r->p_deblock_free_queue))
            {
                pdeblock=r->p_deblock_free_queue->dequeue(r->p_deblock_free_queue);
                av_free(pdeblock);
            }
            while(r->p_deblock_queue->getcnt(r->p_deblock_queue))
            {
                pdeblock=r->p_deblock_queue->dequeue(r->p_deblock_queue);
                av_free(pdeblock);
            }

            while(r->p_frame_done->getcnt(r->p_frame_done))
            {
                r->p_frame_done->dequeue(r->p_frame_done);
            }

            delete_mc_intrapred(r->p_mc_intrapred);
            r->p_mc_intrapred=NULL;

            MPV_common_end(s);
            s->width  = r->si.width;
            s->height = r->si.height;
            if(MPV_common_init(s) < 0)
                return -1;
            r->intra_types_stride = s->mb_width*4 + 4;
            r->intra_types_hist = av_realloc(r->intra_types_hist, r->intra_types_stride * 4 * 2 * sizeof(*r->intra_types_hist));
            r->intra_types = r->intra_types_hist + r->intra_types_stride * 4;
            r->mb_type = av_realloc(r->mb_type, r->s.mb_stride * r->s.mb_height * sizeof(*r->mb_type));
            r->cbp_luma   = av_realloc(r->cbp_luma,   r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_luma));
            r->cbp_chroma = av_realloc(r->cbp_chroma, r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_chroma));
            r->deblock_coefs = av_realloc(r->deblock_coefs, r->s.mb_stride * r->s.mb_height * sizeof(*r->deblock_coefs));

            //parallel related
            r->mc_intrapred_loop=r->deblock_loop=1;

            #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR," restart sub threads in.\n");
            #endif

            #ifdef _d_use_DSP_format_
            r->pdspmv.pbase=av_malloc(sizeof(RVDEC_MB_MV_B_t)*s->mb_width*s->mb_height+16);
            r->pdspmv.p=(RVDEC_MB_MV_P_t*)(((unsigned int)(r->pdspmv.pbase)+15)&(~15));
            r->pdspmv.b=(RVDEC_MB_MV_B_t*)(((unsigned int)(r->pdspmv.pbase)+15)&(~15));
            #endif

            //spwan sub process thread
            //start mc intrapred thread
            pthread_create(&r->tid_mc_intrapred,NULL,mc_intraprediction_thread,r);

            //start deblock thread
            pthread_create(&r->tid_deblock,NULL,deblock_thread,r);

        }
        s->pict_type = r->si.type ? r->si.type : FF_I_TYPE;
        if(MPV_frame_start(s, s->avctx) < 0)
            return -1;
        ff_er_frame_start(s);
        r->cur_pts = r->si.pts;
        if(s->pict_type != FF_B_TYPE){
            r->last_pts = r->next_pts;
            r->next_pts = r->cur_pts;
        }
        s->mb_x = s->mb_y = 0;

        ambadec_assert_ffmpeg(!r->p_mc_intrapred);
        r->p_mc_intrapred_free_queue->lock(r->p_mc_intrapred_free_queue);

        #ifdef __log_communicate_queue__
        av_log(NULL,AV_LOG_ERROR," 1 r->p_mc_intrapred_free_queue->getcnt=%d.\n",r->p_mc_intrapred_free_queue->getcnt(r->p_mc_intrapred_free_queue));
        #endif

        if(r->p_mc_intrapred_free_queue->getcnt(r->p_mc_intrapred_free_queue))
        {
            r->p_mc_intrapred_free_queue->unlock(r->p_mc_intrapred_free_queue);
            r->p_mc_intrapred=r->p_mc_intrapred_free_queue->dequeue(r->p_mc_intrapred_free_queue);
            //need optimize
            #ifdef _remove_block_
            memset(r->p_mc_intrapred->p_idct_result_y,0,r->p_mc_intrapred->stride<<5);
            memset(r->p_mc_intrapred->p_idct_result_uv,0,r->p_mc_intrapred->stride<<4);
            #endif
        }
        else
        {
            r->p_mc_intrapred_free_queue->unlock(r->p_mc_intrapred_free_queue);
            r->p_mc_intrapred=new_mc_intrapred(s->mb_width, s->mb_height);
        }
        ambadec_assert_ffmpeg(r->p_mc_intrapred);
        r->p_mc_intrapred->mbrow=0;
    }

    r->si.end = end;
    s->qscale = r->si.quant;
    r->bits = buf_size*8;
    s->mb_num_left = r->si.end - r->si.start;
    r->s.mb_skip_run = 0;

    mb_pos = s->mb_x + s->mb_y * s->mb_width;
    if(r->si.start != mb_pos){
        av_log(s->avctx, AV_LOG_ERROR, "Slice indicates MB offset %d, got %d\n", r->si.start, mb_pos);
        s->mb_x = r->si.start % s->mb_width;
        s->mb_y = r->si.start / s->mb_width;
    }
    memset(r->intra_types_hist, -1, r->intra_types_stride * 4 * 2 * sizeof(*r->intra_types_hist));
    s->first_slice_line = 1;
    s->resync_mb_x= s->mb_x;
    s->resync_mb_y= s->mb_y;

    ff_init_block_index_amba(s);
    while(!check_slice_end_amba(r, s)) {
        ff_update_block_index_amba(s);

        #ifndef _remove_block_
        s->dsp.clear_blocks(s->block[0]);
        #endif

        if(rv34_amba_decode_macroblock(r, r->intra_types + s->mb_x * 4 + 4) < 0){
            ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_ERROR|DC_ERROR|MV_ERROR);
            return -1;
        }
        if (++s->mb_x == s->mb_width) {
            s->mb_x = 0;
            s->mb_y++;

            #ifdef __log_dump_idct_data__
                log_dump(log_fd_idct_y, (char*)r->p_mc_intrapred->p_idct_result_y,r->p_mc_intrapred->stride<<5);
                log_dump(log_fd_idct_uv, (char*)r->p_mc_intrapred->p_idct_result_uv,r->p_mc_intrapred->stride<<4);
            #endif

            ff_init_block_index_amba(s);

            memmove(r->intra_types_hist, r->intra_types, r->intra_types_stride * 4 * sizeof(*r->intra_types_hist));
            memset(r->intra_types, -1, r->intra_types_stride * 4 * sizeof(*r->intra_types_hist));

            //parallel related
            ambadec_assert_ffmpeg(r->p_mc_intrapred);
            ambadec_assert_ffmpeg(r->p_mc_intrapred->mbrow==s->mb_y-1);

            #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"*count* vld done, sending mbrow %d.\n",r->p_mc_intrapred->mbrow);
            #endif

            r->p_mc_intrapred_queue->enqueue(r->p_mc_intrapred_queue,r->p_mc_intrapred);

            #ifdef __log_communicate_queue__
            av_log(NULL,AV_LOG_ERROR," 2 r->p_mc_intrapred_free_queue->getcnt=%d.\n",r->p_mc_intrapred_free_queue->getcnt(r->p_mc_intrapred_free_queue));
            #endif
            if(s->mb_y != s->mb_height)
            {
                r->p_mc_intrapred_free_queue->lock(r->p_mc_intrapred_free_queue);
                if(r->p_mc_intrapred_free_queue->getcnt(r->p_mc_intrapred_free_queue))
                {
                    r->p_mc_intrapred_free_queue->unlock(r->p_mc_intrapred_free_queue);
                    r->p_mc_intrapred=r->p_mc_intrapred_free_queue->dequeue(r->p_mc_intrapred_free_queue);
                        //need optimize
                    #ifdef _remove_block_
                    memset(r->p_mc_intrapred->p_idct_result_y,0,r->p_mc_intrapred->stride<<5);
                    memset(r->p_mc_intrapred->p_idct_result_uv,0,r->p_mc_intrapred->stride<<4);
                    #endif
                }
                else
                {
                    r->p_mc_intrapred_free_queue->unlock(r->p_mc_intrapred_free_queue);
                    r->p_mc_intrapred=new_mc_intrapred(s->mb_width, s->mb_height);
                }
                r->p_mc_intrapred->mbrow=s->mb_y;
                ambadec_assert_ffmpeg(r->p_mc_intrapred);
            }
            else
            {
                r->p_mc_intrapred=NULL;//end of frame
            }
        }
        if(s->mb_x == s->resync_mb_x)
            s->first_slice_line=0;
        s->mb_num_left--;
    }
    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR," rv34_amba_decode_slice out.\n");
    #endif

    return s->mb_y == s->mb_height;
}

int ff_rv34_amba_decode_frame(AVCodecContext *avctx,
                            void *data, int *data_size,
                            AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    RV34AmbaDecContext *r = avctx->priv_data;
    MpegEncContext *s = &r->s;
    AVFrame *pict = data;
    SliceInfo si;
    int i;
    int slice_count;
    const uint8_t *slices_hdr = NULL;
    int last = 0;

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

    if(!avctx->slice_count){
        slice_count = (*buf++) + 1;
        slices_hdr = buf + 4;
        buf += 8 * slice_count;
    }else
        slice_count = avctx->slice_count;

    //parse first slice header to check whether this frame can be decoded
    if(get_slice_offset(avctx, slices_hdr, 0) > buf_size){
        av_log(avctx, AV_LOG_ERROR, "Slice offset is greater than frame size\n");
        return -1;
    }

    init_get_bits(&s->gb, buf+get_slice_offset(avctx, slices_hdr, 0), buf_size-get_slice_offset(avctx, slices_hdr, 0));
    if(r->parse_slice_header(r, &r->s.gb, &si) < 0 || si.start){
        av_log(avctx, AV_LOG_ERROR, "First slice header is incorrect\n");
        return -1;
    }
    if((!s->last_picture_ptr || !s->last_picture_ptr->data[0]) && si.type == FF_B_TYPE)
        return -1;

//deprecated hurry_up flag
#if 0
    /* skip b frames if we are in a hurry */
    if(
#if FF_API_HURRY_UP
    avctx->hurry_up &&
#endif
    si.type==FF_B_TYPE) return buf_size;
    if(   (avctx->skip_frame >= AVDISCARD_NONREF && si.type==FF_B_TYPE)
       || (avctx->skip_frame >= AVDISCARD_NONKEY && si.type!=FF_I_TYPE)
       ||  avctx->skip_frame >= AVDISCARD_ALL)
        return buf_size;
    /* skip everything if we are in a hurry>=5 */
#if FF_API_HURRY_UP
    if(avctx->hurry_up>=5)
        return buf_size;
#endif
#endif

    #ifdef __log_decoding_config__
    if(log_cur_frame==log_config_start_frame)
        apply_decoding_config();
    #endif

#ifdef __log_dump_idct_data__
log_openfile(log_fd_idct_y,"idct_data_amba_y");
log_openfile(log_fd_idct_uv,"idct_data_amba_uv");
log_openfile_text(log_fd_idct_text,"idct_text_amba");
#endif

#ifdef __log_intrapred__
log_openfile(log_fd_intrapred, "amba/intrapred_type");
#endif
#ifdef __log_mc__
log_openfile(log_fd_mc, "amba/mc_type");
#endif
#ifdef __log_mv_new__
    log_openfile(log_fd_mv_new, "amba/mv_new");
#endif
#ifdef __log_mv__
log_openfile(log_fd_mv, "amba/mv_value");
#endif

#if 0
//#ifdef __dump_temp__
    log_openfile_text(log_fd_temp,"avail");
#endif

#ifndef __dump_whole__
#ifdef __dump_DSP_test_data__

#ifdef __dump_binary__
    log_openfile(log_fd_residual,"rv_residual");
    log_openfile(log_fd_mvdsp,"rv_mv");
    log_openfile(log_fd_reffyraw,"rv_reff_y_raw");
    log_openfile(log_fd_reffuvraw,"rv_reff_uv_raw");
    log_openfile(log_fd_reffytiled,"rv_reff_y_tiled");
    log_openfile(log_fd_reffuvtiled,"rv_reff_uv_tiled");
    log_openfile(log_fd_refbyraw,"rv_refb_y_raw");
    log_openfile(log_fd_refbuvraw,"rv_refb_uv_raw");
    log_openfile(log_fd_refbytiled,"rv_refb_y_tiled");
    log_openfile(log_fd_refbuvtiled,"rv_refb_uv_tiled");
    log_openfile(log_fd_result,"rv_result");
    log_openfile(log_fd_picinfo,"rv_picinfo");
    log_openfile(log_fd_mbtype,"mb_type");
#endif

#ifdef __dump_DSP_TXT__
    log_openfile_text(log_fd_dsp_text,"rv_text");
    if(si.type == FF_P_TYPE)
        log_text(log_fd_dsp_text,"===================== P-VOP ====================\n");
    else if(si.type == FF_B_TYPE)
        log_text(log_fd_dsp_text,"===================== B-VOP ====================\n");
    else if(si.type == FF_I_TYPE)
        log_text(log_fd_dsp_text,"===================== I-VOP ====================\n");
#endif

#endif
#endif


#ifdef _d_use_DSP_format_
    memset(r->pdspmv.b,0,sizeof(RVDEC_MB_MV_B_t)*s->mb_height*s->mb_width);
#endif

    for(i=0; i<slice_count; i++){
        int offset= get_slice_offset(avctx, slices_hdr, i);
        int size;
        if(i+1 == slice_count)
            size= buf_size - offset;
        else
            size= get_slice_offset(avctx, slices_hdr, i+1) - offset;

        if(offset > buf_size){
            av_log(avctx, AV_LOG_ERROR, "Slice offset is greater than frame size\n");
            break;
        }

        r->si.end = s->mb_width * s->mb_height;
        if(i+1 < slice_count){
            init_get_bits(&s->gb, buf+get_slice_offset(avctx, slices_hdr, i+1), (buf_size-get_slice_offset(avctx, slices_hdr, i+1))*8);
            if(r->parse_slice_header(r, &r->s.gb, &si) < 0){
                if(i+2 < slice_count)
                    size = get_slice_offset(avctx, slices_hdr, i+2) - offset;
                else
                    size = buf_size - offset;
            }else
                r->si.end = si.start;
        }
        last = rv34_amba_decode_slice(r, r->si.end, buf + offset, size);
        s->mb_num_left = r->s.mb_x + r->s.mb_y*r->s.mb_width - r->si.start;
        if(last)
            break;
    }

    if(last){

        //wait deblock done
        r->p_frame_done->dequeue(r->p_frame_done);

        #ifdef __log_picinfo__
        char str[80];
        snprintf(str,79,"\n\n picture cnt=%d, picture->index=%d",log_cur_frame,s->current_picture_ptr->opaque);
        log_text(log_fd_picinfo_text, str);
        snprintf(str,79,"picture type=%d",s->pict_type);
        log_text(log_fd_picinfo_text, str);
        if(s->last_picture_ptr)
        {
            snprintf(str,79,"fwref index=%d",s->last_picture_ptr->opaque);
            log_text(log_fd_picinfo_text, str);
        }
        if(s->next_picture_ptr)
        {
            snprintf(str,79,"bwref index=%d",s->next_picture_ptr->opaque);
            log_text(log_fd_picinfo_text, str);
        }
        #endif

        #ifdef __dump_DSP_test_data__
        {
            int itt=0; uint8_t* ptt;

            //memset(&r->picinfo,0,sizeof(iav_rv40_mc_decode_t));
            r->picinfo.uu.rv40.pic_width=avctx->width;
            r->picinfo.uu.rv40.pic_height=avctx->height;
            //r->picinfo.mb_num=s->mb_width*s->mb_height;
            r->picinfo.uu.rv40.coding_type=s->pict_type;
            log_dump(log_fd_picinfo,&r->picinfo,sizeof(udec_decode_t));


//            log_openfile_with_num(log_fd_curyraw,"rv_cur_y_raw",s->current_picture_ptr->opaque);
            log_openfile(log_fd_curyraw,"rv_cur_y_raw");
            ptt=s->current_picture_ptr->data[0];
            for(itt=0;itt<avctx->height;itt++,ptt+=s->current_picture_ptr->linesize[0])
                log_dump(log_fd_curyraw,ptt,avctx->width);
            log_closefile(log_fd_curyraw);

//            log_openfile_with_num(log_fd_curuvraw,"rv_cur_uv_raw",s->current_picture_ptr->opaque);
            log_openfile(log_fd_curuvraw,"rv_cur_uv_raw");
            ptt=s->current_picture_ptr->data[1];
            for(itt=0;(itt<avctx->height/2);itt++,ptt+=s->current_picture_ptr->linesize[1])
                log_dump(log_fd_curuvraw,ptt,avctx->width);
            log_closefile(log_fd_curuvraw);

            if(s->last_picture_ptr)
            {
                ptt=s->last_picture_ptr->data[0];
                for(itt=0;itt<avctx->height;itt++,ptt+=s->last_picture_ptr->linesize[0])
                    log_dump(log_fd_reffyraw,ptt,avctx->width);

                ptt=s->last_picture_ptr->data[1];
                for(itt=0;(itt<avctx->height/2);itt++,ptt+=s->last_picture_ptr->linesize[1])
                    log_dump(log_fd_reffuvraw,ptt,avctx->width);
            }

            if(s->next_picture_ptr)
            {
                ptt=s->next_picture_ptr->data[0];
                for(itt=0;itt<avctx->height;itt++,ptt+=s->next_picture_ptr->linesize[0])
                    log_dump(log_fd_refbyraw,ptt,avctx->width);

                ptt=s->next_picture_ptr->data[1];
                for(itt=0;(itt<avctx->height/2);itt++,ptt+=s->next_picture_ptr->linesize[1])
                    log_dump(log_fd_refbuvraw,ptt,avctx->width);
            }
        }
        #endif

        #ifdef __log_dump_data__
        AVFrame* pdecpic=(AVFrame*)s->current_picture_ptr;
        if(pdecpic->data[0])
        {
            int ret=0;
        //    av_log(NULL,AV_LOG_WARNING,"start dump y.\n");
                log_openfile_with_num(log_fd_frame_data,"frame_data_Y_amba",pdecpic->opaque);
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
                        *ptmpu++= ptt++;
                        *ptmpv++= ptt++;
                    }
                }
                ptmpu-=htmp*wtmp;
                ptmpv-=htmp*wtmp;
                log_openfile_with_num(log_fd_frame_data,"frame_data_U_amba",pdecpic->opaque);
                log_dump(log_fd_frame_data,ptmpu,htmp*wtmp);
                log_closefile(log_fd_frame_data);
                log_openfile_with_num(log_fd_frame_data,"frame_data_V_amba",pdecpic->opaque);
                ret=log_dump(log_fd_frame_data,ptmpv,htmp*wtmp);
                log_closefile(log_fd_frame_data);
                av_free(ptmpu);
                av_free(ptmpv);

//dump with extended edge
#if 0
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

        //    av_log(NULL,AV_LOG_WARNING,"start dump y.\n");
                log_openfile_with_num(log_fd_frame_data,"frame_data_U_amba",pdecpic->opaque);
                log_dump(log_fd_frame_data,ptmpu,htmp*wtmp);
                log_closefile(log_fd_frame_data);
        //    av_log(NULL,AV_LOG_WARNING,"end dump u, htmp=%d,wtmp=%d,ptmpu=%p.\n",htmp,wtmp,ptmpu);
                log_openfile_with_num(log_fd_frame_data,"frame_data_V_amba",pdecpic->opaque);
                ret=log_dump(log_fd_frame_data,ptmpv,htmp*wtmp);
                log_closefile(log_fd_frame_data);
        //     av_log(NULL,AV_LOG_WARNING,"end dump v, htmp=%d,wtmp=%d. ret=%d. ptmpv=%p \n",htmp,wtmp,ret,ptmpv);
                av_free(ptmpu);
                av_free(ptmpv);
        //    av_log(NULL,AV_LOG_WARNING,"end free.\n");
#endif
        }
        #endif

        //assert
        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"**** frame decode done****.\n");
        #endif

        ff_er_frame_end(s);
        MPV_frame_end_nv12(s);
        if (s->pict_type == FF_B_TYPE || s->low_delay) {
            *pict= *(AVFrame*)s->current_picture_ptr;
        } else if (s->last_picture_ptr != NULL) {
            *pict= *(AVFrame*)s->last_picture_ptr;
        }

        if(s->last_picture_ptr || s->low_delay){
            *data_size = sizeof(AVFrame);
            ff_print_debug_info(s, pict);
        }

/*
        av_log(NULL,AV_LOG_ERROR,"  Decoding type: \n");
        switch (s->pict_type) {
            case FF_I_TYPE: av_log(NULL,AV_LOG_ERROR,"    I\n"); break;
            case FF_P_TYPE: av_log(NULL,AV_LOG_ERROR,"    P\n"); break;
            case FF_B_TYPE: av_log(NULL,AV_LOG_ERROR,"    B\n"); break;
            case FF_S_TYPE: av_log(NULL,AV_LOG_ERROR,"    S\n"); break;
            case FF_SI_TYPE: av_log(NULL,AV_LOG_ERROR,"    SI\n"); break;
            case FF_SP_TYPE: av_log(NULL,AV_LOG_ERROR,"    SP\n"); break;
        }
        av_log(NULL,AV_LOG_ERROR,"  Showing type: \n");
        switch (pict->pict_type) {
            case FF_I_TYPE: av_log(NULL,AV_LOG_ERROR,"    I\n"); break;
            case FF_P_TYPE: av_log(NULL,AV_LOG_ERROR,"    P\n"); break;
            case FF_B_TYPE: av_log(NULL,AV_LOG_ERROR,"    B\n"); break;
            case FF_S_TYPE: av_log(NULL,AV_LOG_ERROR,"    S\n"); break;
            case FF_SI_TYPE: av_log(NULL,AV_LOG_ERROR,"    SI\n"); break;
            case FF_SP_TYPE: av_log(NULL,AV_LOG_ERROR,"    SP\n"); break;
        }
*/

        s->current_picture_ptr= NULL; //so we can detect if frame_end wasnt called (find some nicer solution...)

//av_log(NULL,AV_LOG_WARNING,"s->current_picture_ptr=%p,s->last_picture_ptr=%p,s->next_picture_ptr=%p.\n",s->current_picture_ptr,s->last_picture_ptr,s->next_picture_ptr);
//av_log(NULL,AV_LOG_WARNING,"data[0]=%p,%p,%p.\n",s->current_picture_ptr->data[0],s->last_picture_ptr->data[0],s->next_picture_ptr->data[0]);


    }

#ifdef __log_dump_idct_data__
        log_closefile(log_fd_idct_y);
        log_closefile(log_fd_idct_uv);
        log_closefile(log_fd_idct_text);
#endif
#ifdef __log_intrapred__
log_closefile(log_fd_intrapred);
#endif
#ifdef __log_mc__
log_closefile(log_fd_mc);
#endif
#ifdef __log_mv_new__
    log_closefile(log_fd_mv_new);
#endif
#ifdef __log_mv__
log_closefile(log_fd_mv);
#endif

#if 0
//#ifdef __dump_temp__
    log_closefile(log_fd_temp);
#endif

#ifdef __dump_DSP_test_data__

log_dump(log_fd_mvdsp,r->pdspmv.p,sizeof(RVDEC_MB_MV_B_t)*s->mb_height*s->mb_width);
#ifndef __dump_whole__
#ifdef __dump_binary__
log_closefile(log_fd_residual);
log_closefile(log_fd_mvdsp);
log_closefile(log_fd_reffyraw);
log_closefile(log_fd_reffuvraw);
log_closefile(log_fd_reffytiled);
log_closefile(log_fd_reffuvtiled);
log_closefile(log_fd_refbyraw);
log_closefile(log_fd_refbuvraw);
log_closefile(log_fd_refbytiled);
log_closefile(log_fd_refbuvtiled);
log_closefile(log_fd_result);
log_closefile(log_fd_picinfo);
log_closefile(log_fd_mbtype);
#endif
#ifdef __dump_DSP_TXT__
log_closefile(log_fd_dsp_text);
#endif
#endif
#endif

    log_cur_frame++;
    return buf_size;
}

av_cold int ff_rv34_amba_decode_end(AVCodecContext *avctx)
{
    RV34AmbaDecContext *r = avctx->priv_data;
//    int i=0,j=0;
    MBLineMCIntraPred* pmc;
    MBLineDeblock* pdeblock;
    void* pv1,*pv2;
    int ret2,ret1;

//    av_log(NULL,AV_LOG_ERROR,"****end, start quit sub threads.\n");

    //quit sub process thread
    r->mc_intrapred_loop=0;//r->deblock_loop=0;

    //need indicate next quit
    r->p_mc_intrapred_queue->enqueue(r->p_mc_intrapred_queue,NULL);
//    r->p_deblock_queue->enqueue(r->p_deblock_queue,NULL);

    ret1=pthread_join(r->tid_mc_intrapred,&pv1);
//    av_log(NULL,AV_LOG_ERROR,"    end, join mc_intrapred threads, ret=%d.\n",ret1);

    ret2=pthread_join(r->tid_deblock,&pv2);
//    av_log(NULL,AV_LOG_ERROR,"    end, join deblock threads, ret=%d.\n",ret2);

//    r->p_thread_exit->dequeue(r->p_thread_exit);
//    r->p_thread_exit->dequeue(r->p_thread_exit);

    #ifdef _d_use_DSP_format_
    if(r->pdspmv.pbase)
        av_free(r->pdspmv.pbase);
    r->pdspmv.p=r->pdspmv.b=r->pdspmv.pbase=NULL;
    #endif

    #ifdef __log_picinfo__
        log_closefile(log_fd_picinfo_text);
    #endif

#ifdef __dump_whole__
    #ifdef __dump_DSP_test_data__
        #ifdef __dump_binary__
        log_closefile(log_fd_residual);
        log_closefile(log_fd_mvdsp);
        log_closefile(log_fd_reffyraw);
        log_closefile(log_fd_reffuvraw);
        log_closefile(log_fd_reffytiled);
        log_closefile(log_fd_reffuvtiled);
        log_closefile(log_fd_refbyraw);
        log_closefile(log_fd_refbuvraw);
        log_closefile(log_fd_refbytiled);
        log_closefile(log_fd_refbuvtiled);
        log_closefile(log_fd_result);
        log_closefile(log_fd_picinfo);
        #endif
    #endif
#endif

    //free resources
    //free mc intrapred related
    while(r->p_mc_intrapred_free_queue->getcnt(r->p_mc_intrapred_free_queue))
    {
        pmc=r->p_mc_intrapred_free_queue->dequeue(r->p_mc_intrapred_free_queue);
        delete_mc_intrapred(pmc);
    }
    while(r->p_mc_intrapred_queue->getcnt(r->p_mc_intrapred_queue))
    {
        pmc=r->p_mc_intrapred_queue->dequeue(r->p_mc_intrapred_queue);
        delete_mc_intrapred(pmc);
    }

    //free deblock related
    while(r->p_deblock_free_queue->getcnt(r->p_deblock_free_queue))
    {
        pdeblock=r->p_deblock_free_queue->dequeue(r->p_deblock_free_queue);
        av_free(pdeblock);
    }
    while(r->p_deblock_queue->getcnt(r->p_deblock_queue))
    {
        pdeblock=r->p_deblock_queue->dequeue(r->p_deblock_queue);
        av_free(pdeblock);
    }

    ambadec_destroy_queue(r->p_mc_intrapred_free_queue);
    ambadec_destroy_queue(r->p_mc_intrapred_queue);
    ambadec_destroy_queue(r->p_deblock_free_queue);
    ambadec_destroy_queue(r->p_deblock_queue);

    ambadec_destroy_queue(r->p_frame_done);
//    ambadec_destroy_queue(r->p_thread_exit);

    MPV_common_end(&r->s);

    av_freep(&r->intra_types_hist);
    r->intra_types = NULL;
    av_freep(&r->mb_type);
    av_freep(&r->cbp_luma);
    av_freep(&r->cbp_chroma);
    av_freep(&r->deblock_coefs);

    return 0;
}

/**
 * Initialize decoder.
 */
av_cold int ff_rv34_amba_decode_init(AVCodecContext *avctx)
{
    RV34AmbaDecContext *r = avctx->priv_data;
    MpegEncContext *s = &r->s;

    MPV_decode_defaults(s);
    s->avctx= avctx;
    s->out_format = FMT_H263;
    s->codec_id= avctx->codec_id;

    s->width = avctx->width;
    s->height = avctx->height;

    r->s.avctx = avctx;
    avctx->flags |= CODEC_FLAG_EMU_EDGE;
    r->s.flags |= CODEC_FLAG_EMU_EDGE;
    avctx->pix_fmt = PIX_FMT_NV12;
//        avctx->pix_fmt = PIX_FMT_YUV420P;

    avctx->has_b_frames = 1;
    s->low_delay = 0;

    if (MPV_common_init(s) < 0)
        return -1;

    ff_h264_pred_init(&r->h, CODEC_ID_RV40, 8);

#if HAVE_NEON
    init_rv40_neon(&r->neon, (void*)&r->h);
#endif

    r->intra_types_stride = 4*s->mb_stride + 4;
    r->intra_types_hist = av_malloc(r->intra_types_stride * 4 * 2 * sizeof(*r->intra_types_hist));
    r->intra_types = r->intra_types_hist + r->intra_types_stride * 4;

    r->mb_type = av_mallocz(r->s.mb_stride * r->s.mb_height * sizeof(*r->mb_type));

    r->cbp_luma   = av_malloc(r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_luma));
    r->cbp_chroma = av_malloc(r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_chroma));
    r->deblock_coefs = av_malloc(r->s.mb_stride * r->s.mb_height * sizeof(*r->deblock_coefs));

    if(!intra_vlcs[0].cbppattern[0].bits)
        rv34_amba_init_tables();

    //parallel related
    r->p_mc_intrapred_queue=ambadec_create_queue(20);
    r->p_mc_intrapred_free_queue=ambadec_create_queue(1000);
    r->p_deblock_queue=ambadec_create_queue(20);
    r->p_deblock_free_queue=ambadec_create_queue(1000);
    r->p_frame_done=ambadec_create_queue(1);
//    r->p_thread_exit=ambadec_create_queue(2);
    r->mc_intrapred_loop=r->deblock_loop=1;

    #ifdef _d_use_DSP_format_
    r->pdspmv.pbase=av_malloc(sizeof(RVDEC_MB_MV_B_t)*s->mb_width*s->mb_height+16);
    r->pdspmv.p=(RVDEC_MB_MV_P_t*)(((unsigned int)(r->pdspmv.pbase)+15)&(~15));
    r->pdspmv.b=(RVDEC_MB_MV_B_t*)(((unsigned int)(r->pdspmv.pbase)+15)&(~15));
    #endif

    //spwan sub process thread
    //start mc intrapred thread
    pthread_create(&r->tid_mc_intrapred,NULL,mc_intraprediction_thread,r);

    //start deblock thread
    pthread_create(&r->tid_deblock,NULL,deblock_thread,r);

    #ifdef __log_picinfo__
    log_openfile_text_f(log_fd_picinfo_text, "rv_picinfo_txt");
    #endif

#ifdef __dump_whole__
    #ifdef __dump_DSP_test_data__
        #ifdef __dump_binary__
        log_openfile_f(log_fd_residual,"rv_residual");
        log_openfile_f(log_fd_mvdsp,"rv_mv");
        log_openfile_f(log_fd_reffyraw,"rv_reff_y_raw");
        log_openfile_f(log_fd_reffuvraw,"rv_reff_uv_raw");
        log_openfile_f(log_fd_reffytiled,"rv_reff_y_tiled");
        log_openfile_f(log_fd_reffuvtiled,"rv_reff_uv_tiled");
        log_openfile_f(log_fd_refbyraw,"rv_refb_y_raw");
        log_openfile_f(log_fd_refbuvraw,"rv_refb_uv_raw");
        log_openfile_f(log_fd_refbytiled,"rv_refb_y_tiled");
        log_openfile_f(log_fd_refbuvtiled,"rv_refb_uv_tiled");
        log_openfile_f(log_fd_result,"rv_result");
        log_openfile_f(log_fd_picinfo,"rv_picinfo");
        #endif
    #endif
#endif

    return 0;
}

#else

#ifdef _intra_using_allocate_residue_
//get continus mv/residue buffer
static int _get_nextbuffers(RV34AmbaDecContext *thiz, rv40_pic_data_t* p_pic, int is_i)
{
    if (!is_i) {
        p_pic->pinfo = thiz->pb_mv[thiz->pb_current_index];
        p_pic->pdct = thiz->pb_residue[thiz->pb_current_index];

        p_pic->picinfo.uu.rv40.mv_daddr_start = thiz->pb_mv[thiz->pb_current_index];
        p_pic->picinfo.uu.rv40.residual_daddr_start  = thiz->pb_residue[thiz->pb_current_index];

        p_pic->picinfo.uu.rv40.mv_daddr_end = thiz->pb_mv[thiz->pb_current_index + 1];
        p_pic->picinfo.uu.rv40.residual_daddr_end = thiz->pb_residue[thiz->pb_current_index +1];

        if (thiz->pb_current_index != thiz->pb_tot_index) {
            ambadec_assert_ffmpeg(thiz->pb_current_index < thiz->pb_tot_index);
            thiz->pb_current_index ++;
        } else {
            ambadec_assert_ffmpeg(thiz->pb_current_index == thiz->pb_tot_index);
            thiz->pb_current_index = 0;
        }

        p_pic->pmc_result = thiz->pAcc->p_mcresult_buffer[thiz->result_current_index];
        p_pic->pmc_result_buffer_id = thiz->pAcc->mcresult_buffer_id[thiz->result_current_index];
        p_pic->picinfo.uu.rv40.target_fb_id = p_pic->pmc_result_buffer_id;
        if (thiz->result_current_index != thiz->result_tot_index) {
            ambadec_assert_ffmpeg(thiz->result_current_index < thiz->result_tot_index);
            thiz->result_current_index ++;
        } else {
            ambadec_assert_ffmpeg(thiz->result_current_index == thiz->result_tot_index);
            thiz->result_current_index = 0;
        }
    } else {
        p_pic->pdct = thiz->i_residue[thiz->i_current_index];
        p_pic->pinfo = NULL;
        if (thiz->i_current_index != thiz->i_tot_index) {
            ambadec_assert_ffmpeg(thiz->i_current_index < thiz->i_tot_index);
            thiz->i_current_index ++;
        } else {
            ambadec_assert_ffmpeg(thiz->i_current_index == thiz->i_tot_index);
            thiz->i_current_index = 0;
        }
    }

}
#endif

//need_optimize
static void _writeback_new_mc_result(unsigned char* psrcc, unsigned char* pdes_y, unsigned char* pdes_uv, unsigned int stride, unsigned int mb_width, unsigned int mb_height, int* mbtype)
{
    unsigned int src_stride;
    unsigned int i,j;
    unsigned int* psrc_y1;//first half
    unsigned int* psrc_y2;//sencond half
    unsigned char* psrc_u, *psrc_v;

    unsigned int* pdesy_1;// first half
    unsigned int* pdesy_2;// second half
    unsigned char* pdesuv;// = pdes_uv;
    //av_log(NULL, AV_LOG_ERROR, "stride %d, mb_width %d.\n", stride, mb_width);
    ambadec_assert_ffmpeg(stride>=mb_width*16);
    ambadec_assert_ffmpeg(!(stride&0xf));

    for (j=0; j<mb_height; j++) {

        src_stride = mb_width*16*8;
        psrc_y1 = (unsigned int*)(psrcc + src_stride*3*j);
        psrc_y2 = (unsigned int*)((unsigned char*)psrc_y1 + src_stride);
        psrc_u = (unsigned char*)psrc_y2 + src_stride;
        psrc_v = psrc_u + 64;

        for (i=0; i<mb_width; i++, mbtype++) {
            if (!(mbtype[0] & mbisinter)) {
                psrc_y1 += 32;//128/4=32
                psrc_y2 += 32;
                psrc_u += 128;
                psrc_v += 128;
                continue;
            }
            pdesy_1 = (unsigned int*)(pdes_y+ j*16*stride + i*16);
            pdesy_2 = pdesy_1 + 2*stride;//8/4*stride=2*stride
            pdesuv = pdes_uv + j*8*stride + i*16;

            //y, line 0 line 8
            pdesy_1[0] = *psrc_y1++;
            pdesy_2[0] = *psrc_y2++;
            pdesy_1[1] = *psrc_y1++;
            pdesy_2[1] = *psrc_y2++;
            pdesy_1[2] = *psrc_y1++;
            pdesy_2[2] = *psrc_y2++;
            pdesy_1[3] = *psrc_y1++;
            pdesy_2[3] = *psrc_y2++;
            pdesy_1 += stride>>2;
            pdesy_2 += stride>>2;

            //y, line 1 line 9
            pdesy_1[0] = *psrc_y1++;
            pdesy_2[0] = *psrc_y2++;
            pdesy_1[1] = *psrc_y1++;
            pdesy_2[1] = *psrc_y2++;
            pdesy_1[2] = *psrc_y1++;
            pdesy_2[2] = *psrc_y2++;
            pdesy_1[3] = *psrc_y1++;
            pdesy_2[3] = *psrc_y2++;
            pdesy_1 += stride>>2;
            pdesy_2 += stride>>2;

            //y, line 2 line 10
            pdesy_1[0] = *psrc_y1++;
            pdesy_2[0] = *psrc_y2++;
            pdesy_1[1] = *psrc_y1++;
            pdesy_2[1] = *psrc_y2++;
            pdesy_1[2] = *psrc_y1++;
            pdesy_2[2] = *psrc_y2++;
            pdesy_1[3] = *psrc_y1++;
            pdesy_2[3] = *psrc_y2++;
            pdesy_1 += stride>>2;
            pdesy_2 += stride>>2;

            //y, line 3 line 11
            pdesy_1[0] = *psrc_y1++;
            pdesy_2[0] = *psrc_y2++;
            pdesy_1[1] = *psrc_y1++;
            pdesy_2[1] = *psrc_y2++;
            pdesy_1[2] = *psrc_y1++;
            pdesy_2[2] = *psrc_y2++;
            pdesy_1[3] = *psrc_y1++;
            pdesy_2[3] = *psrc_y2++;
            pdesy_1 += stride>>2;
            pdesy_2 += stride>>2;

            //y, line 4 line 12
            pdesy_1[0] = *psrc_y1++;
            pdesy_2[0] = *psrc_y2++;
            pdesy_1[1] = *psrc_y1++;
            pdesy_2[1] = *psrc_y2++;
            pdesy_1[2] = *psrc_y1++;
            pdesy_2[2] = *psrc_y2++;
            pdesy_1[3] = *psrc_y1++;
            pdesy_2[3] = *psrc_y2++;
            pdesy_1 += stride>>2;
            pdesy_2 += stride>>2;

            //y, line 5 line 13
            pdesy_1[0] = *psrc_y1++;
            pdesy_2[0] = *psrc_y2++;
            pdesy_1[1] = *psrc_y1++;
            pdesy_2[1] = *psrc_y2++;
            pdesy_1[2] = *psrc_y1++;
            pdesy_2[2] = *psrc_y2++;
            pdesy_1[3] = *psrc_y1++;
            pdesy_2[3] = *psrc_y2++;
            pdesy_1 += stride>>2;
            pdesy_2 += stride>>2;

            //y, line 6 line 14
            pdesy_1[0] = *psrc_y1++;
            pdesy_2[0] = *psrc_y2++;
            pdesy_1[1] = *psrc_y1++;
            pdesy_2[1] = *psrc_y2++;
            pdesy_1[2] = *psrc_y1++;
            pdesy_2[2] = *psrc_y2++;
            pdesy_1[3] = *psrc_y1++;
            pdesy_2[3] = *psrc_y2++;
            pdesy_1 += stride>>2;
            pdesy_2 += stride>>2;

            //y, line 7 line 15
            pdesy_1[0] = *psrc_y1++;
            pdesy_2[0] = *psrc_y2++;
            pdesy_1[1] = *psrc_y1++;
            pdesy_2[1] = *psrc_y2++;
            pdesy_1[2] = *psrc_y1++;
            pdesy_2[2] = *psrc_y2++;
            pdesy_1[3] = *psrc_y1++;
            pdesy_2[3] = *psrc_y2++;
            pdesy_1 += stride>>2;
            pdesy_2 += stride>>2;

            //u v
            pdesuv[0] = *psrc_u++;
            pdesuv[1] = *psrc_v++;
            pdesuv[2] = *psrc_u++;
            pdesuv[3] = *psrc_v++;
            pdesuv[4] = *psrc_u++;
            pdesuv[5] = *psrc_v++;
            pdesuv[6] = *psrc_u++;
            pdesuv[7] = *psrc_v++;
            pdesuv[8] = *psrc_u++;
            pdesuv[9] = *psrc_v++;
            pdesuv[10] = *psrc_u++;
            pdesuv[11] = *psrc_v++;
            pdesuv[12] = *psrc_u++;
            pdesuv[13] = *psrc_v++;
            pdesuv[14] = *psrc_u++;
            pdesuv[15] = *psrc_v++;
            pdesuv += stride;

            pdesuv[0] = *psrc_u++;
            pdesuv[1] = *psrc_v++;
            pdesuv[2] = *psrc_u++;
            pdesuv[3] = *psrc_v++;
            pdesuv[4] = *psrc_u++;
            pdesuv[5] = *psrc_v++;
            pdesuv[6] = *psrc_u++;
            pdesuv[7] = *psrc_v++;
            pdesuv[8] = *psrc_u++;
            pdesuv[9] = *psrc_v++;
            pdesuv[10] = *psrc_u++;
            pdesuv[11] = *psrc_v++;
            pdesuv[12] = *psrc_u++;
            pdesuv[13] = *psrc_v++;
            pdesuv[14] = *psrc_u++;
            pdesuv[15] = *psrc_v++;
            pdesuv += stride;

            pdesuv[0] = *psrc_u++;
            pdesuv[1] = *psrc_v++;
            pdesuv[2] = *psrc_u++;
            pdesuv[3] = *psrc_v++;
            pdesuv[4] = *psrc_u++;
            pdesuv[5] = *psrc_v++;
            pdesuv[6] = *psrc_u++;
            pdesuv[7] = *psrc_v++;
            pdesuv[8] = *psrc_u++;
            pdesuv[9] = *psrc_v++;
            pdesuv[10] = *psrc_u++;
            pdesuv[11] = *psrc_v++;
            pdesuv[12] = *psrc_u++;
            pdesuv[13] = *psrc_v++;
            pdesuv[14] = *psrc_u++;
            pdesuv[15] = *psrc_v++;
            pdesuv += stride;

            pdesuv[0] = *psrc_u++;
            pdesuv[1] = *psrc_v++;
            pdesuv[2] = *psrc_u++;
            pdesuv[3] = *psrc_v++;
            pdesuv[4] = *psrc_u++;
            pdesuv[5] = *psrc_v++;
            pdesuv[6] = *psrc_u++;
            pdesuv[7] = *psrc_v++;
            pdesuv[8] = *psrc_u++;
            pdesuv[9] = *psrc_v++;
            pdesuv[10] = *psrc_u++;
            pdesuv[11] = *psrc_v++;
            pdesuv[12] = *psrc_u++;
            pdesuv[13] = *psrc_v++;
            pdesuv[14] = *psrc_u++;
            pdesuv[15] = *psrc_v++;
            pdesuv += stride;

            pdesuv[0] = *psrc_u++;
            pdesuv[1] = *psrc_v++;
            pdesuv[2] = *psrc_u++;
            pdesuv[3] = *psrc_v++;
            pdesuv[4] = *psrc_u++;
            pdesuv[5] = *psrc_v++;
            pdesuv[6] = *psrc_u++;
            pdesuv[7] = *psrc_v++;
            pdesuv[8] = *psrc_u++;
            pdesuv[9] = *psrc_v++;
            pdesuv[10] = *psrc_u++;
            pdesuv[11] = *psrc_v++;
            pdesuv[12] = *psrc_u++;
            pdesuv[13] = *psrc_v++;
            pdesuv[14] = *psrc_u++;
            pdesuv[15] = *psrc_v++;
            pdesuv += stride;

            pdesuv[0] = *psrc_u++;
            pdesuv[1] = *psrc_v++;
            pdesuv[2] = *psrc_u++;
            pdesuv[3] = *psrc_v++;
            pdesuv[4] = *psrc_u++;
            pdesuv[5] = *psrc_v++;
            pdesuv[6] = *psrc_u++;
            pdesuv[7] = *psrc_v++;
            pdesuv[8] = *psrc_u++;
            pdesuv[9] = *psrc_v++;
            pdesuv[10] = *psrc_u++;
            pdesuv[11] = *psrc_v++;
            pdesuv[12] = *psrc_u++;
            pdesuv[13] = *psrc_v++;
            pdesuv[14] = *psrc_u++;
            pdesuv[15] = *psrc_v++;
            pdesuv += stride;

            pdesuv[0] = *psrc_u++;
            pdesuv[1] = *psrc_v++;
            pdesuv[2] = *psrc_u++;
            pdesuv[3] = *psrc_v++;
            pdesuv[4] = *psrc_u++;
            pdesuv[5] = *psrc_v++;
            pdesuv[6] = *psrc_u++;
            pdesuv[7] = *psrc_v++;
            pdesuv[8] = *psrc_u++;
            pdesuv[9] = *psrc_v++;
            pdesuv[10] = *psrc_u++;
            pdesuv[11] = *psrc_v++;
            pdesuv[12] = *psrc_u++;
            pdesuv[13] = *psrc_v++;
            pdesuv[14] = *psrc_u++;
            pdesuv[15] = *psrc_v++;
            pdesuv += stride;

            pdesuv[0] = *psrc_u++;
            pdesuv[1] = *psrc_v++;
            pdesuv[2] = *psrc_u++;
            pdesuv[3] = *psrc_v++;
            pdesuv[4] = *psrc_u++;
            pdesuv[5] = *psrc_v++;
            pdesuv[6] = *psrc_u++;
            pdesuv[7] = *psrc_v++;
            pdesuv[8] = *psrc_u++;
            pdesuv[9] = *psrc_v++;
            pdesuv[10] = *psrc_u++;
            pdesuv[11] = *psrc_v++;
            pdesuv[12] = *psrc_u++;
            pdesuv[13] = *psrc_v++;
            pdesuv[14] = *psrc_u++;
            pdesuv[15] = *psrc_v++;
            pdesuv += stride;

            psrc_u += 64;
            psrc_v += 64;
        }
    }
}
/*
//need_optimize
static void _writeback_mc_result(unsigned char* psrcc1, unsigned char* pdes_y, unsigned char* pdes_uv, unsigned int stride, unsigned int mb_width, unsigned int mb_height, int* mbtype)
{
    unsigned int i,j;
//    unsigned int v1, v2, u1, u2;
    unsigned int* psrc = (unsigned int*)psrcc1;
    unsigned char* psrcc2;
    unsigned int* pdesy;// = pdes_y;
    unsigned char* pdesuv;// = pdes_uv;
    av_log(NULL, AV_LOG_ERROR, "stride %d, mb_width %d.\n", stride, mb_width);
    ambadec_assert_ffmpeg(stride>=mb_width*16);
    ambadec_assert_ffmpeg(!(stride&0xf));
    stride >>= 2;

    for (j=0; j<mb_height; j++) {
        for (i=0; i<mb_width; i++, mbtype++) {
            if (!(mbtype[0] & mbisinter)) {
                psrc += 96;
                continue;
            }
            pdesy = (unsigned int*)(pdes_y + ((stride * j)<<6) + (i<<4));
            pdesuv = ((stride * j)<<5) + pdes_uv + (i<<4);
            //y, line 0
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 1
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 2
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 3
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;

            //y, line 4
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 5
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 6
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 7
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;

            //y, line 0
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 1
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 2
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 3
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;

            //y, line 4
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 5
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 6
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;
            pdesy += stride;
            //y, line 7
            pdesy[0] = *psrc++;
            pdesy[1] = *psrc++;
            pdesy[2] = *psrc++;
            pdesy[3] = *psrc++;

            psrcc1 = (unsigned char*)psrc;
            psrcc2 = psrcc1 + 64;
            psrc += 32;

            //u v
            pdesuv[0] = *psrcc1++;
            pdesuv[1] = *psrcc2++;
            pdesuv[2] = *psrcc1++;
            pdesuv[3] = *psrcc2++;
            pdesuv[4] = *psrcc1++;
            pdesuv[5] = *psrcc2++;
            pdesuv[6] = *psrcc1++;
            pdesuv[7] = *psrcc2++;
            pdesuv[8] = *psrcc1++;
            pdesuv[9] = *psrcc2++;
            pdesuv[10] = *psrcc1++;
            pdesuv[11] = *psrcc2++;
            pdesuv[12] = *psrcc1++;
            pdesuv[13] = *psrcc2++;
            pdesuv[14] = *psrcc1++;
            pdesuv[15] = *psrcc2++;

            pdesuv += stride<<2;
            pdesuv[0] = *psrcc1++;
            pdesuv[1] = *psrcc2++;
            pdesuv[2] = *psrcc1++;
            pdesuv[3] = *psrcc2++;
            pdesuv[4] = *psrcc1++;
            pdesuv[5] = *psrcc2++;
            pdesuv[6] = *psrcc1++;
            pdesuv[7] = *psrcc2++;
            pdesuv[8] = *psrcc1++;
            pdesuv[9] = *psrcc2++;
            pdesuv[10] = *psrcc1++;
            pdesuv[11] = *psrcc2++;
            pdesuv[12] = *psrcc1++;
            pdesuv[13] = *psrcc2++;
            pdesuv[14] = *psrcc1++;
            pdesuv[15] = *psrcc2++;

            pdesuv += stride<<2;
            pdesuv[0] = *psrcc1++;
            pdesuv[1] = *psrcc2++;
            pdesuv[2] = *psrcc1++;
            pdesuv[3] = *psrcc2++;
            pdesuv[4] = *psrcc1++;
            pdesuv[5] = *psrcc2++;
            pdesuv[6] = *psrcc1++;
            pdesuv[7] = *psrcc2++;
            pdesuv[8] = *psrcc1++;
            pdesuv[9] = *psrcc2++;
            pdesuv[10] = *psrcc1++;
            pdesuv[11] = *psrcc2++;
            pdesuv[12] = *psrcc1++;
            pdesuv[13] = *psrcc2++;
            pdesuv[14] = *psrcc1++;
            pdesuv[15] = *psrcc2++;

            pdesuv += stride<<2;
            pdesuv[0] = *psrcc1++;
            pdesuv[1] = *psrcc2++;
            pdesuv[2] = *psrcc1++;
            pdesuv[3] = *psrcc2++;
            pdesuv[4] = *psrcc1++;
            pdesuv[5] = *psrcc2++;
            pdesuv[6] = *psrcc1++;
            pdesuv[7] = *psrcc2++;
            pdesuv[8] = *psrcc1++;
            pdesuv[9] = *psrcc2++;
            pdesuv[10] = *psrcc1++;
            pdesuv[11] = *psrcc2++;
            pdesuv[12] = *psrcc1++;
            pdesuv[13] = *psrcc2++;
            pdesuv[14] = *psrcc1++;
            pdesuv[15] = *psrcc2++;

            pdesuv += stride<<2;
            pdesuv[0] = *psrcc1++;
            pdesuv[1] = *psrcc2++;
            pdesuv[2] = *psrcc1++;
            pdesuv[3] = *psrcc2++;
            pdesuv[4] = *psrcc1++;
            pdesuv[5] = *psrcc2++;
            pdesuv[6] = *psrcc1++;
            pdesuv[7] = *psrcc2++;
            pdesuv[8] = *psrcc1++;
            pdesuv[9] = *psrcc2++;
            pdesuv[10] = *psrcc1++;
            pdesuv[11] = *psrcc2++;
            pdesuv[12] = *psrcc1++;
            pdesuv[13] = *psrcc2++;
            pdesuv[14] = *psrcc1++;
            pdesuv[15] = *psrcc2++;

            pdesuv += stride<<2;
            pdesuv[0] = *psrcc1++;
            pdesuv[1] = *psrcc2++;
            pdesuv[2] = *psrcc1++;
            pdesuv[3] = *psrcc2++;
            pdesuv[4] = *psrcc1++;
            pdesuv[5] = *psrcc2++;
            pdesuv[6] = *psrcc1++;
            pdesuv[7] = *psrcc2++;
            pdesuv[8] = *psrcc1++;
            pdesuv[9] = *psrcc2++;
            pdesuv[10] = *psrcc1++;
            pdesuv[11] = *psrcc2++;
            pdesuv[12] = *psrcc1++;
            pdesuv[13] = *psrcc2++;
            pdesuv[14] = *psrcc1++;
            pdesuv[15] = *psrcc2++;

            pdesuv += stride<<2;
            pdesuv[0] = *psrcc1++;
            pdesuv[1] = *psrcc2++;
            pdesuv[2] = *psrcc1++;
            pdesuv[3] = *psrcc2++;
            pdesuv[4] = *psrcc1++;
            pdesuv[5] = *psrcc2++;
            pdesuv[6] = *psrcc1++;
            pdesuv[7] = *psrcc2++;
            pdesuv[8] = *psrcc1++;
            pdesuv[9] = *psrcc2++;
            pdesuv[10] = *psrcc1++;
            pdesuv[11] = *psrcc2++;
            pdesuv[12] = *psrcc1++;
            pdesuv[13] = *psrcc2++;
            pdesuv[14] = *psrcc1++;
            pdesuv[15] = *psrcc2++;

            pdesuv += stride<<2;
            pdesuv[0] = *psrcc1++;
            pdesuv[1] = *psrcc2++;
            pdesuv[2] = *psrcc1++;
            pdesuv[3] = *psrcc2++;
            pdesuv[4] = *psrcc1++;
            pdesuv[5] = *psrcc2++;
            pdesuv[6] = *psrcc1++;
            pdesuv[7] = *psrcc2++;
            pdesuv[8] = *psrcc1++;
            pdesuv[9] = *psrcc2++;
            pdesuv[10] = *psrcc1++;
            pdesuv[11] = *psrcc2++;
            pdesuv[12] = *psrcc1++;
            pdesuv[13] = *psrcc2++;
            pdesuv[14] = *psrcc1++;
            pdesuv[15] = *psrcc2++;
        }
    }
}
*/
//new
static void _callback_free(void* p)
{
    ambadec_assert_ffmpeg(p);
    av_free(p);
}

static void _callback_free_vld_data(void* p)
{
    vld_data_t* pvld=(vld_data_t*)p;
    ambadec_assert_ffmpeg(p);
    if(pvld->pbuf)
        av_free(pvld->pbuf);
    if(pvld->slice_offset)
        av_free(pvld->slice_offset);
    av_free(p);
}

static void _callback_destroy_pic_data(void* p)
{
    ambadec_assert_ffmpeg(p);
    rv40_pic_data_t* ppic=(rv40_pic_data_t*)p;

    if(ppic->pbase)
        av_free(ppic->pbase);
    if(ppic->pdct)
        av_free(ppic->pdctbase);
    if(ppic->cbp_luma)
        av_free(ppic->cbp_luma);
    if(ppic->cbp_chroma)
        av_free(ppic->cbp_chroma);
    if(ppic->deblock_coefs)
        av_free(ppic->deblock_coefs);
    #ifdef _tmp_rv_dsp_setting_
    if(ppic->pintra)
        av_free(ppic->pintra);
    if(ppic->pmbtype)
        av_free(ppic->pmbtype);
    #endif
}

static void _callback_NULL(void* p)
{
    return;
}

static void _reset_picture_data(rv40_pic_data_t* ppic)
{
//    av_log(NULL,AV_LOG_ERROR,"_reset_picture_data ppic->mb_width=%d,ppic->mb_height=%d.\n",ppic->mb_width,ppic->mb_height);
    //clear all data
    if(ppic->current_picture_ptr->pict_type!=FF_I_TYPE)
        memset(ppic->pinfo,0x0,ppic->mb_width*ppic->mb_height*sizeof(RVDEC_MBINFO_t));
//    memset(ppic->pintra,0x0,ppic->mb_width*ppic->mb_height*sizeof(RVDEC_MBINFO_t));
    memset(ppic->pdct,0x0,ppic->mb_width*ppic->mb_height*768);
#ifdef _tmp_rv_dsp_setting_
//    memset(ppic->pintra,0x0,ppic->mb_width*ppic->mb_height*sizeof(RVDEC_INTRAPRED_t));
#endif

}

void flush_pictrue(RV34AmbaDecContext *thiz, Picture* pic)
{
    pthread_mutex_lock(&thiz->mutex);
    if (!thiz || ! pic) {
        ambadec_assert_ffmpeg(0);
        pthread_mutex_unlock(&thiz->mutex);
        return;
    }

    if (thiz->pic_pool->dec_lock(thiz->pic_pool,pic)==1) {
        if (pic->ref_added_for_extern) {
            pic->type = DFlushing_frame;
        }
        thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)pic);
    }
    pthread_mutex_unlock(&thiz->mutex);
}

static int get_slice_offset_amba(vld_data_t * p_vld, const uint8_t *buf, int n)
{
#ifdef __log_dump_decocer_input_data__
    static int cnt = 0;
    int ret = 0;
    char dump_str[100];

    //dump basic info to txt

    if (!buf)
        snprintf(dump_str,99,"start %d get slice offset, buf NULL, n %d, p_vld->slice_cnt %d", cnt++, n, p_vld->slice_cnt);
    else
        snprintf(dump_str,99,"start %d get slice offset, n %d, p_vld->slice_cnt %d", cnt++, n, p_vld->slice_cnt);

    log_text_f(log_fd_temp, dump_str);

    if(p_vld->slice_cnt)
    {
        ret = p_vld->slice_offset[n];
        snprintf(dump_str,99, "1have slice_cnt %d, ret %d.\n", p_vld->slice_cnt, ret);
        log_text_f(log_fd_temp, dump_str);
    }
    else {
        ret = AV_RL32(buf + n*8 - 4) == 1 ? AV_RL32(buf + n*8) :  AV_RB32(buf + n*8);
        snprintf(dump_str,99, "2have slice_cnt %d, ret %d.\n", p_vld->slice_cnt, ret);
        log_text_f(log_fd_temp, dump_str);
    }
    return ret;
#else
    if(p_vld->slice_cnt) return p_vld->slice_offset[n];
    else                   return AV_RL32(buf + n*8 - 4) == 1 ? AV_RL32(buf + n*8) :  AV_RB32(buf + n*8);
#endif
}

static int rv34new_amba_set_deblock_coef(RV34AmbaDecContext *r)
{
    MpegEncContext *s = &r->s;
    int hmvmask = 0, vmvmask = 0, i, j;
    int midx = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int16_t (*motion_val)[2] = (int16_t (*)[2])s->current_picture_ptr->motion_val[0][midx];
    for(j = 0; j < 16; j += 8){
        for(i = 0; i < 2; i++){
            if(is_mv_diff_gt_3(motion_val + i, 1))
                vmvmask |= 0x11 << (j + i*2);
            if((j || s->mb_y) && is_mv_diff_gt_3(motion_val + i, s->b8_stride))
                hmvmask |= 0x03 << (j + i*2);
        }
        motion_val += s->b8_stride;
    }
    if(s->first_slice_line)
        hmvmask &= ~0x000F;
    if(!s->mb_x)
        vmvmask &= ~0x1111;
    if(r->rv30){ //RV30 marks both subblocks on the edge for filtering
        vmvmask |= (vmvmask & 0x4444) >> 1;
        hmvmask |= (hmvmask & 0x0F00) >> 4;
        if(s->mb_x)
            r->p_pic->deblock_coefs[s->mb_x - 1 + s->mb_y*s->mb_stride] |= (vmvmask & 0x1111) << 3;
        if(!s->first_slice_line)
            r->p_pic->deblock_coefs[s->mb_x + (s->mb_y - 1)*s->mb_stride] |= (hmvmask & 0xF) << 12;
    }
    return hmvmask | vmvmask;
}

static int rv34new_amba_decode_mv_p(RV34AmbaDecContext *r, int block_type)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int i;//, j, k, l
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
//    int next_bt;
    RVDEC_MBMV_P_t* pmvp=(RVDEC_MBMV_P_t*)r->p_pic->pinfo+s->mb_y*s->mb_width+s->mb_x;

    memset(r->dmv, 0, sizeof(r->dmv));
    for(i = 0; i < num_mvs[block_type]; i++){
        r->dmv[i][0] = svq3_get_se_golomb(gb);
        r->dmv[i][1] = svq3_get_se_golomb(gb);
    }

    #ifdef __log_mc__
        char tstr[60];
        snprintf(tstr,59,"mc type=%d ",block_type);
        log_text_p(log_fd_mc, tstr,r->p_pic->frame_cnt);
    #endif
//    av_log(NULL,AV_LOG_ERROR,"p block_type=%d",block_type);
    switch(block_type){
    case RV34_MB_TYPE_INTRA:
    case RV34_MB_TYPE_INTRA16x16:
        ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);

        #ifdef __log_mc__
            log_text_p(log_fd_mc, "intra mb ",r->p_pic->frame_cnt);
        #endif

        return 0;
    case RV34_MB_SKIP:
//        if(s->pict_type == FF_P_TYPE){
            ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);

            #ifdef __log_mc__
                log_text_p(log_fd_mc,"RV34_MB_SKIP ",r->p_pic->frame_cnt);
            #endif

            #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
//            if(log_mask[decoding_config_mc])
            #endif
            //rv34_amba_mc_1mv (r, block_type, 0, 0, 0, 2, 2, 0);
            {
            // parallel related: 1 mv, forward
                pmvp->fwd[0].valid=1;
//                pmvp->fwd[0].ref_idx=0;
                pmvp->fwd[1]=pmvp->fwd[2]=pmvp->fwd[3]=pmvp->fwd[0];
            }
            #endif

            break;
//        }
    case RV34_MB_P_16x16:
    case RV34_MB_P_MIX16x16:
        rv34_pred_mv(r, block_type, 0, 0);

        #ifdef __log_mc__
            log_text_p(log_fd_mc,"RV34_MB_P_16x16",r->p_pic->frame_cnt);
        #endif

        #ifndef __commit_out_motion_comp__
        #ifdef __log_decoding_config__
//        if(log_mask[decoding_config_mc])
        #endif
        //rv34_amba_mc_1mv (r, block_type, 0, 0, 0, 2, 2, 0);
        {
            // parallel related: 1 mv, forward
            pmvp->fwd[0].mv_x= s->current_picture_ptr->motion_val[0][mv_pos][0];
            pmvp->fwd[0].mv_y=s->current_picture_ptr->motion_val[0][mv_pos][1];
            pmvp->fwd[0].valid=1;
//            pmvp->fwd[0].ref_idx=0;
            pmvp->fwd[1]=pmvp->fwd[2]=pmvp->fwd[3]=pmvp->fwd[0];
        }
        #endif
        break;
    case RV34_MB_P_16x8:
    case RV34_MB_P_8x16:
        rv34_pred_mv(r, block_type, 0, 0);
        rv34_pred_mv(r, block_type, 1 + (block_type == RV34_MB_P_16x8), 1);
        if(block_type == RV34_MB_P_16x8){

            #ifdef __log_mc__
                log_text_p(log_fd_mc,"RV34_MB_P_16x8",r->p_pic->frame_cnt);
            #endif

            #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
//            if(log_mask[decoding_config_mc])
            #endif
//            {rv34_amba_mc_1mv(r, block_type, 0, 0, 0,            2, 1, 0);
//           rv34_amba_mc_1mv(r, block_type, 0, 8, s->b8_stride, 2, 1, 0);}
            {
                // parallel related: 4 mv, forward
                pmvp->fwd[0].mv_x= s->current_picture_ptr->motion_val[0][mv_pos][0];
                pmvp->fwd[0].mv_y= s->current_picture_ptr->motion_val[0][mv_pos][1];
                pmvp->fwd[0].valid=1;
//                pmvp->fwd[0].ref_idx=0;
                pmvp->fwd[1]=pmvp->fwd[0];

                pmvp->fwd[2].mv_x= s->current_picture_ptr->motion_val[0][mv_pos+s->b8_stride][0];
                pmvp->fwd[2].mv_y= s->current_picture_ptr->motion_val[0][mv_pos+s->b8_stride][1];
                pmvp->fwd[2].valid=1;
//                pmvp->fwd[2].ref_idx=0;
                pmvp->fwd[3]=pmvp->fwd[2];
            }
            #endif
        }
        if(block_type == RV34_MB_P_8x16){

            #ifdef __log_mc__
                log_text_p(log_fd_mc,"RV34_MB_P_8x16",r->p_pic->frame_cnt);
            #endif

            #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
//            if(log_mask[decoding_config_mc])
            #endif
            //{rv34_amba_mc_1mv(r, block_type, 0, 0, 0, 1, 2, 0);
            //rv34_amba_mc_1mv(r, block_type, 8, 0, 1, 1, 2, 0);}
            {
                // parallel related: 4 mv, forward
                pmvp->fwd[0].mv_x= s->current_picture_ptr->motion_val[0][mv_pos][0];
                pmvp->fwd[0].mv_y= s->current_picture_ptr->motion_val[0][mv_pos][1];
                pmvp->fwd[0].valid=1;
//                pmvp->fwd[0].ref_idx=0;
                pmvp->fwd[2]=pmvp->fwd[0];

                pmvp->fwd[1].mv_x= s->current_picture_ptr->motion_val[0][mv_pos+1][0];
                pmvp->fwd[1].mv_y= s->current_picture_ptr->motion_val[0][mv_pos+1][1];
                pmvp->fwd[1].valid=1;
//                pmvp->fwd[1].ref_idx=0;
                pmvp->fwd[3]=pmvp->fwd[1];
            }
            #endif
        }
        break;
    case RV34_MB_P_8x8:

        #ifdef __log_mc__
            log_text_p(log_fd_mc,"RV34_MB_P_8x8",r->p_pic->frame_cnt);
        #endif

        for(i=0;i< 4;i++){
            rv34_pred_mv(r, block_type, i, i);
            #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
//            if(log_mask[decoding_config_mc])
            #endif
            //rv34_amba_mc_1mv (r, block_type, (i&1)<<3, (i&2)<<2, (i&1)+(i>>1)*s->b8_stride, 1, 1, 0);
            {
                // parallel related: 4 mv, forward
                pmvp->fwd[i].mv_x= s->current_picture_ptr->motion_val[0][mv_pos+(i&1)+(i>>1)*s->b8_stride][0];
                pmvp->fwd[i].mv_y= s->current_picture_ptr->motion_val[0][mv_pos+(i&1)+(i>>1)*s->b8_stride][1];
                pmvp->fwd[i].valid=1;
//                pmvp->fwd[i].ref_idx=0;
            }
            #endif
        }
        break;
    default:
        av_log(NULL,AV_LOG_ERROR,"rv34new_amba_decode_mv_p unknown MB type %d.\n",block_type);
        ambadec_assert_ffmpeg(0);
    }

    #ifdef _tmp_rv_dsp_setting_
    r->p_pic->pmbtype[s->mb_y*s->mb_width+s->mb_x]=usefw;
//    int* t=(int*)pmvp;
//    t[4]=t[5]=t[6]=t[7]=0x0;
    #endif

    return 0;
}

static int rv34new_amba_decode_mv_b(RV34AmbaDecContext *r, int block_type)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int i, j, k, l;
    int mv_pos = s->mb_y*s->mb_width+s->mb_x;
    int next_bt;
    RVDEC_MBMV_B_t* pmvb=(RVDEC_MBMV_B_t*)r->p_pic->pinfo+mv_pos;
    #ifdef _tmp_rv_dsp_setting_
    int* pmbtype=r->p_pic->pmbtype+mv_pos;
    #endif
    mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;

    memset(r->dmv, 0, sizeof(r->dmv));
    for(i = 0; i < num_mvs[block_type]; i++){
        r->dmv[i][0] = svq3_get_se_golomb(gb);
        r->dmv[i][1] = svq3_get_se_golomb(gb);
    }

    #ifdef __log_mc__
        char tstr[60];
        snprintf(tstr,59,"mc type=%d ",block_type);
        log_text_p(log_fd_mc, tstr,r->p_pic->frame_cnt);
    #endif
//    av_log(NULL,AV_LOG_ERROR,"b block_type=%d",block_type);
    switch(block_type){
    case RV34_MB_TYPE_INTRA:
    case RV34_MB_TYPE_INTRA16x16:
        ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);

        #ifdef __log_mc__
            log_text_p(log_fd_mc, "intra mb ",r->p_pic->frame_cnt);
        #endif

        return 0;
    case RV34_MB_SKIP:
    case RV34_MB_B_DIRECT:
        //surprisingly, it uses motion scheme from next reference frame
        next_bt = s->next_picture_ptr->mb_type[s->mb_x + s->mb_y * s->mb_stride];
        if(IS_INTRA(next_bt) || IS_SKIP(next_bt)){
            ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);
            ZERO8x2(s->current_picture_ptr->motion_val[1][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);
        }else
            for(j = 0; j < 2; j++)
                for(i = 0; i < 2; i++)
                    for(k = 0; k < 2; k++)
                        for(l = 0; l < 2; l++)
                            s->current_picture_ptr->motion_val[l][mv_pos + i + j*s->b8_stride][k] = calc_add_mv(r, l, s->next_picture_ptr->motion_val[0][mv_pos + i + j*s->b8_stride][k]);
        if(!(IS_16X8(next_bt) || IS_8X16(next_bt) || IS_8X8(next_bt))) //we can use whole macroblock MC
        {

            #ifdef __log_mc__
                log_text_p(log_fd_mc,"RV34_MB_B_DIRECT whole mb ",r->p_pic->frame_cnt);
            #endif

        #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
//            if(log_mask[decoding_config_mc])
            #endif
            //rv34_amba_mc_2mv(r, block_type);
            {
                // parallel related: 1 mv, Bidirectional
                pmvb->mv[0].fwd.mv_x=s->current_picture_ptr->motion_val[0][mv_pos][0];
                pmvb->mv[0].fwd.mv_y=s->current_picture_ptr->motion_val[0][mv_pos][1];
                pmvb->mv[0].fwd.valid=1;
//                pmvb->mv[0].fwd.ref_idx=0;

                pmvb->mv[1].fwd=pmvb->mv[2].fwd=pmvb->mv[3].fwd=pmvb->mv[0].fwd;

                pmvb->mv[0].bwd.mv_x=s->current_picture_ptr->motion_val[1][mv_pos][0];
                pmvb->mv[0].bwd.mv_y=s->current_picture_ptr->motion_val[1][mv_pos][1];
                pmvb->mv[0].bwd.valid=1;
                pmvb->mv[0].bwd.ref_idx=1;

                pmvb->mv[1].bwd=pmvb->mv[2].bwd=pmvb->mv[3].bwd=pmvb->mv[0].bwd;
                #ifdef _tmp_rv_dsp_setting_
                *pmbtype=mbisinter;
                #endif
            }
        #endif
        }
        else
        {
            #ifdef __log_mc__
                log_text_p(log_fd_mc,"RV34_MB_B_DIRECT 4mv bi-directional ",r->p_pic->frame_cnt);
            #endif

         #ifndef __commit_out_motion_comp__
            #ifdef __log_decoding_config__
//            if(log_mask[decoding_config_mc])
            #endif
//            rv34_amba_mc_2mv_skip(r);
            {
                // parallel related: 4 mv, Bidirectional ?
                k=0;
                int cpos=0;
                for(j=0;j<2;j++)
                {
                    for(i=0;i<2;i++)
                    {
                        cpos=mv_pos+i+j*s->b8_stride;

                        pmvb->mv[k].fwd.mv_x=s->current_picture_ptr->motion_val[0][cpos][0];
                        pmvb->mv[k].fwd.mv_y=s->current_picture_ptr->motion_val[0][cpos][1];
                        pmvb->mv[k].fwd.valid=1;
//                        pmvb->mv[k].fwd.ref_idx=0;

                        pmvb->mv[k].bwd.mv_x=s->current_picture_ptr->motion_val[1][cpos][0];
                        pmvb->mv[k].bwd.mv_y=s->current_picture_ptr->motion_val[1][cpos][1];
                        pmvb->mv[k].bwd.valid=1;
                        pmvb->mv[k].bwd.ref_idx=1;

                        k++;
                    }
                }
                #ifdef _tmp_rv_dsp_setting_
                *pmbtype=mbisinter;
                #endif
            }
         #endif
        }
        ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);
        break;
    case RV34_MB_B_FORWARD:
    case RV34_MB_B_BACKWARD:
        r->dmv[1][0] = r->dmv[0][0];
        r->dmv[1][1] = r->dmv[0][1];
        if(r->rv30)
            rv34_pred_mv_rv3(r, block_type, block_type == RV34_MB_B_BACKWARD);
        else
            rv34_pred_mv_b  (r, block_type, block_type == RV34_MB_B_BACKWARD);

        #ifdef __log_mc__
            log_text_p(log_fd_mc,"RV34_MB_B_FORWARD or RV34_MB_B_BACKWARD",r->p_pic->frame_cnt);
        #endif

        #ifndef __commit_out_motion_comp__
        #ifdef __log_decoding_config__
//        if(log_mask[decoding_config_mc])
        #endif
        //rv34_amba_mc_1mv     (r, block_type, 0, 0, 0, 2, 2, block_type == RV34_MB_B_BACKWARD);
        {
            // parallel related: 1 mv, forward or backward
            if(block_type == RV34_MB_B_FORWARD)
            {
                pmvb->mv[0].fwd.mv_x=s->current_picture_ptr->motion_val[0][mv_pos][0];
                pmvb->mv[0].fwd.mv_y=s->current_picture_ptr->motion_val[0][mv_pos][1];
                pmvb->mv[0].fwd.valid=1;
//                pmvb->mv[0].fwd.ref_idx=0;
                pmvb->mv[1].fwd=pmvb->mv[2].fwd=pmvb->mv[3].fwd=pmvb->mv[0].fwd;
                #ifdef _tmp_rv_dsp_setting_
                *pmbtype=usefw;
                //t=(int*)pmvb;
                //t[1]=t[3]=t[5]=t[7]=0;
                #endif
            }
            else
            {
                pmvb->mv[0].bwd.mv_x=s->current_picture_ptr->motion_val[1][mv_pos][0];
                pmvb->mv[0].bwd.mv_y=s->current_picture_ptr->motion_val[1][mv_pos][1];
                pmvb->mv[0].bwd.valid=1;
                pmvb->mv[0].bwd.ref_idx=1;
                pmvb->mv[1].bwd=pmvb->mv[2].bwd=pmvb->mv[3].bwd=pmvb->mv[0].bwd;
                #ifdef _tmp_rv_dsp_setting_
                *pmbtype=usebw;
                //t=(int*)pmvb;
                //t[0]=t[2]=t[4]=t[6]=0;
                #endif
            }
        }
        #endif
        break;

    case RV34_MB_B_BIDIR:
        rv34_pred_mv_b  (r, block_type, 0);
        rv34_pred_mv_b  (r, block_type, 1);

        #ifdef __log_mc__
            log_text_p(log_fd_mc,"RV34_MB_B_BIDIR",r->p_pic->frame_cnt);
        #endif

        #ifndef __commit_out_motion_comp__
        #ifdef __log_decoding_config__
//        if(log_mask[decoding_config_mc])
        #endif
        //rv34_amba_mc_2mv     (r, block_type);
        {
            // parallel related: 1 mv, Bidirectional
            pmvb->mv[0].fwd.mv_x=s->current_picture_ptr->motion_val[0][mv_pos][0];
            pmvb->mv[0].fwd.mv_y=s->current_picture_ptr->motion_val[0][mv_pos][1];
            pmvb->mv[0].fwd.valid=1;
//            pmvb->mv[0].fwd.ref_idx=0;

            pmvb->mv[1].fwd=pmvb->mv[2].fwd=pmvb->mv[3].fwd=pmvb->mv[0].fwd;

            pmvb->mv[0].bwd.mv_x=s->current_picture_ptr->motion_val[1][mv_pos][0];
            pmvb->mv[0].bwd.mv_y=s->current_picture_ptr->motion_val[1][mv_pos][1];
            pmvb->mv[0].bwd.valid=1;
            pmvb->mv[0].bwd.ref_idx=1;

            pmvb->mv[1].bwd=pmvb->mv[2].bwd=pmvb->mv[3].bwd=pmvb->mv[0].bwd;
            #ifdef _tmp_rv_dsp_setting_
            *pmbtype=mbisinter;
            #endif
        }
        #endif
        break;
    default:
        av_log(NULL,AV_LOG_ERROR,"rv34new_amba_decode_mv_b unknown MB type %d.\n",block_type);
        ambadec_assert_ffmpeg(0);
    }

    return 0;
}

static int rv34new_amba_decode_mb_header(RV34AmbaDecContext *r, int8_t *intra_types)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int i, t;

    if(!r->si.type){
        r->is16 = get_bits1(gb);
        if(!r->is16 && !r->rv30){
            if(!get_bits1(gb))
                av_log(s->avctx, AV_LOG_ERROR, "Need DQUANT\n");
        }
        s->current_picture_ptr->mb_type[mb_pos] = r->is16 ? MB_TYPE_INTRA16x16 : MB_TYPE_INTRA;
        r->block_type = r->is16 ? RV34_MB_TYPE_INTRA16x16 : RV34_MB_TYPE_INTRA;
    }else{
        r->block_type = r->decode_mb_info(r);
        if(r->block_type == -1)
            return -1;
        s->current_picture_ptr->mb_type[mb_pos] = rv34_mb_type_to_lavc[r->block_type];
        r->mb_type[mb_pos] = r->block_type;
        if(r->block_type == RV34_MB_SKIP){
            if(s->pict_type == FF_P_TYPE)
                r->mb_type[mb_pos] = RV34_MB_P_16x16;
            if(s->pict_type == FF_B_TYPE)
                r->mb_type[mb_pos] = RV34_MB_B_DIRECT;
        }
        r->is16 = !!IS_INTRA16x16(s->current_picture_ptr->mb_type[mb_pos]);
        if(s->pict_type == FF_P_TYPE)
            rv34new_amba_decode_mv_p(r, r->block_type);
        else if(s->pict_type == FF_B_TYPE)
            rv34new_amba_decode_mv_b(r, r->block_type);
        if(r->block_type == RV34_MB_SKIP){
            fill_rectangle(intra_types, 4, 4, r->intra_types_stride, 0, sizeof(intra_types[0]));
            return 0;
        }
        r->chroma_vlc = 1;
        r->luma_vlc   = 0;
    }
    if(IS_INTRA(s->current_picture_ptr->mb_type[mb_pos])){
        if(r->is16){
            t = get_bits(gb, 2);
            fill_rectangle(intra_types, 4, 4, r->intra_types_stride, t, sizeof(intra_types[0]));
            r->luma_vlc   = 2;
        }else{
            if(r->decode_intra_types(r, gb, intra_types) < 0)
                return -1;
            r->luma_vlc   = 1;
        }
        r->chroma_vlc = 0;
        r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 0);
    }else{
        for(i = 0; i < 16; i++)
            intra_types[(i & 3) + (i>>2) * r->intra_types_stride] = 0;
        r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 1);
        if(r->mb_type[mb_pos] == RV34_MB_P_MIX16x16){
            r->is16 = 1;
            r->chroma_vlc = 1;
            r->luma_vlc   = 2;
            r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 0);
        }
    }

    return rv34_decode_cbp(gb, r->cur_vlcs, r->is16);
}

#ifdef _tmp_rv_dsp_setting_
static inline void rv34new_pred_4x4_block(uint8_t* p_mbinfo,int itype, int up, int left, int down, int right)
{

    if(!up && !left)
        itype = DC_128_PRED;
    else if(!up){
        if(itype == VERT_PRED) itype = HOR_PRED;
        if(itype == DC_PRED)   itype = LEFT_DC_PRED;
    }else if(!left){
        if(itype == HOR_PRED)  itype = VERT_PRED;
        if(itype == DC_PRED)   itype = TOP_DC_PRED;
        if(itype == DIAG_DOWN_LEFT_PRED) itype = DIAG_DOWN_LEFT_PRED_RV40_NODOWN;
    }
    if(!down){
        if(itype == DIAG_DOWN_LEFT_PRED) itype = DIAG_DOWN_LEFT_PRED_RV40_NODOWN;
        if(itype == HOR_UP_PRED) itype = HOR_UP_PRED_RV40_NODOWN;
        if(itype == VERT_LEFT_PRED) itype = VERT_LEFT_PRED_RV40_NODOWN;
    }
    if(!right && up){
        *p_mbinfo=(itype&intratypemask)|usetopleft;
    }
    else
    {
        *p_mbinfo=itype&intratypemask;
    }
}
#else
static inline void rv34new_pred_4x4_block(RVDEC_INTRAPRED_FMT_t* p_mbinfo, int itype, int up, int left, int down, int right)
{
    if(!up && !left)
        itype = DC_128_PRED;
    else if(!up){
        if(itype == VERT_PRED) itype = HOR_PRED;
        if(itype == DC_PRED)   itype = LEFT_DC_PRED;
    }else if(!left){
        if(itype == HOR_PRED)  itype = VERT_PRED;
        if(itype == DC_PRED)   itype = TOP_DC_PRED;
        if(itype == DIAG_DOWN_LEFT_PRED) itype = DIAG_DOWN_LEFT_PRED_RV40_NODOWN;
    }
    if(!down){
        if(itype == DIAG_DOWN_LEFT_PRED) itype = DIAG_DOWN_LEFT_PRED_RV40_NODOWN;
        if(itype == HOR_UP_PRED) itype = HOR_UP_PRED_RV40_NODOWN;
        if(itype == VERT_LEFT_PRED) itype = VERT_LEFT_PRED_RV40_NODOWN;
    }
    if(!right && up){
        p_mbinfo->use_topleft=1;
    }

    p_mbinfo->intratype=itype;
}
#endif
static void rv34new_amba_output_macroblock(RV34AmbaDecContext *r, int8_t *intra_types,int is16)
{
    MpegEncContext *s = &r->s;
//    DSPContext *dsp = &s->dsp;
    int i, j;
//    uint8_t *Y, *UV;
    int itype;
    int avail[6*8] = {0};
    int idx;
    RVDEC_INTRAPRED_t* pintra=r->p_pic->pintra+s->mb_width*s->mb_y+s->mb_x;

    // Set neighbour information.
    if(r->avail_cache[1])
        avail[0] = 1;
    if(r->avail_cache[2])
        avail[1] = avail[2] = 1;
    if(r->avail_cache[3])
        avail[3] = avail[4] = 1;
    if(r->avail_cache[4])
        avail[5] = 1;
    if(r->avail_cache[5])
        avail[8] = avail[16] = 1;
    if(r->avail_cache[9])
        avail[24] = avail[32] = 1;

    if(!is16){

        for(j = 0; j < 4; j++){
            idx = 9 + j*8;
            for(i = 0; i < 4; i++,  idx++){

//                #ifdef __log_decoding_config__
//                if(log_mask[decoding_config_intrapred])
//                #endif
                rv34new_pred_4x4_block(&pintra->intratype[j*4+i],ittrans[intra_types[i]], avail[idx-8], avail[idx-1], avail[idx+7], avail[idx-7]);
                avail[idx] = 1;
            }
            intra_types += r->intra_types_stride;
        }
        intra_types -= r->intra_types_stride * 4;
        fill_rectangle(r->avail_cache + 6, 2, 2, 4, 0, 4);
        for(j = 0; j < 2; j++){
            idx = 6 + j*4;
            for(i = 0; i < 2; i++, idx++){
//                #ifdef __log_decoding_config__
//                if(log_mask[decoding_config_intrapred])
//                #endif
                {
                    rv34new_pred_4x4_block(&pintra->intratype[j*2+i+16], ittrans[intra_types[i*2+j*2*r->intra_types_stride]], r->avail_cache[idx-4], r->avail_cache[idx-1], !i && !j, r->avail_cache[idx-3]);
//                rv34_pred_4x4_block_amba(r, V + (i<<3) + j*4*s->uvlinesize, s->uvlinesize, ittrans[intra_types[i*2+j*2*r->intra_types_stride]], r->avail_cache[idx-4], r->avail_cache[idx-1], !i && !j, r->avail_cache[idx-3]);
                }
                r->avail_cache[idx] = 1;
            }
        }
    }else{
    #ifdef _tmp_rv_dsp_setting_
        itype = ittrans16[intra_types[0]];
        pintra->intratype[0]= adjust_pred16(itype, r->avail_cache[6-4], r->avail_cache[6-1]);

        itype = ittrans16[intra_types[0]];
        if(itype == PLANE_PRED8x8) itype = DC_PRED8x8;
        pintra->intratype[1]= adjust_pred16(itype, r->avail_cache[6-4], r->avail_cache[6-1]);
    #else
        pintra->intratype[0].is16x16=1;

        itype = ittrans16[intra_types[0]];
        pintra->intratype[0].intratype= adjust_pred16(itype, r->avail_cache[6-4], r->avail_cache[6-1]);

        itype = ittrans16[intra_types[0]];
        if(itype == PLANE_PRED8x8) itype = DC_PRED8x8;
        pintra->intratype[1].intratype = adjust_pred16(itype, r->avail_cache[6-4], r->avail_cache[6-1]);
    #endif
    }
}

static int rv34new_amba_decode_macroblock(RV34AmbaDecContext *r, int8_t *intra_types)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int cbp, cbp2;

    int i;

    DCTELEM block16[64];
    int luma_dc_quant;
    int dist;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
//    int j=0,k=0;

    DCTELEM* pmby=r->p_pic->pdct+((s->mb_x+s->mb_y*s->mb_width)*384);
    DCTELEM* pmbuv=pmby+256;
    DCTELEM* ptmp;

    #ifdef __log_dump_idct_data__
    if(s->mb_x>=log_start_mb_x && s->mb_x<=log_end_mb_x && s->mb_y>=log_start_mb_y && s->mb_y<=log_end_mb_y )
    {
        char str[40];
        snprintf(str,39,"MBidct_x=%d_y=%d ",s->mb_x,s->mb_y);
        log_text_p(log_fd_idct_text, str,r->p_pic->frame_cnt);
    }
    #endif

    #ifdef __log_mc__
        char strtt[40];
        snprintf(strtt,39,"MB_x=%d_y=%d ",s->mb_x,s->mb_y);
        log_text_p(log_fd_mc, strtt,r->p_pic->frame_cnt);
    #endif

    #ifdef __dump_DSP_TXT__
        char txtch[80];
        snprintf(txtch,79,">>>>>>>>>>>>>>>>>>>> [MB] = [%d  %d] <<<<<<<<<<<<<<<<<<<<",s->mb_y,s->mb_x);
        log_text_p(log_fd_dsp_text,txtch,r->p_pic->frame_cnt);
    #endif

    // Calculate which neighbours are available. Maybe it's worth optimizing too.
    memset(r->avail_cache, 0, sizeof(r->avail_cache));
    fill_rectangle(r->avail_cache + 6, 2, 2, 4, 1, 4);
    dist = (s->mb_x - s->resync_mb_x) + (s->mb_y - s->resync_mb_y) * s->mb_width;
    if(s->mb_x && dist)
        r->avail_cache[5] =
        r->avail_cache[9] = s->current_picture_ptr->mb_type[mb_pos - 1];
    if(dist >= s->mb_width)
        r->avail_cache[2] =
        r->avail_cache[3] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride];
    if(((s->mb_x+1) < s->mb_width) && dist >= s->mb_width - 1)
        r->avail_cache[4] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride + 1];
    if(s->mb_x && dist > s->mb_width)
        r->avail_cache[1] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride - 1];

    s->qscale = r->si.quant;
    cbp = cbp2 = rv34new_amba_decode_mb_header(r, intra_types);
    r->p_pic->cbp_luma  [mb_pos] = cbp;
    r->p_pic->cbp_chroma[mb_pos] = cbp >> 16;
    if(s->pict_type == FF_I_TYPE)
        r->p_pic->deblock_coefs[mb_pos] = 0xFFFF;
    else
        r->p_pic->deblock_coefs[mb_pos] = rv34new_amba_set_deblock_coef(r) | r->p_pic->cbp_luma[mb_pos];
    s->current_picture_ptr->qscale_table[mb_pos] = s->qscale;

    if(cbp == -1)
        return -1;

    luma_dc_quant = r->block_type == RV34_MB_P_MIX16x16 ? r->luma_dc_quant_p[s->qscale] : r->luma_dc_quant_i[s->qscale];

    #ifdef __dump_DSP_TXT__
        if(r->mb_type[mb_pos]==RV34_MB_TYPE_INTRA)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  INTRA  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_TYPE_INTRA16x16)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  INTRA16x16  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_P_16x16)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  P_16x16  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_P_8x8)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  P_8x8  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_B_FORWARD)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  B_FWD  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_B_BACKWARD)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  B_BWD  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_SKIP)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  SKIP  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_B_DIRECT)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  B_DIRECT  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_P_16x8)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  P_16x8  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_P_8x16)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  P_8x16  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_B_BIDIR)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  B_BIDIR  %d %d]",s->qscale,cbp);
        else if(r->mb_type[mb_pos]==RV34_MB_P_MIX16x16)
            snprintf(txtch,79,"[MODE, QSCALE, CBP] = [  P_MIX16x16  %d %d]",s->qscale,cbp);

        log_text_p(log_fd_dsp_text,txtch, r->p_pic->frame_cnt);
        snprintf(txtch,79,"16x16: dc q=%d, ac q=%d;"\
                    ,rv34_qscale_tab[luma_dc_quant],rv34_qscale_tab[s->qscale]);
        log_text_p(log_fd_dsp_text,txtch, r->p_pic->frame_cnt);
        snprintf(txtch,79," 4v4 luma dc q=%d, ac q=%d;"\
                ,rv34_qscale_tab[s->qscale],rv34_qscale_tab[s->qscale]);
        log_text_p(log_fd_dsp_text,txtch, r->p_pic->frame_cnt);
        snprintf(txtch,79," 4x4 chroma dc q=%d, ac q=%d;"\
                    ,rv34_qscale_tab[rv34_chroma_quant[1][s->qscale]],rv34_qscale_tab[rv34_chroma_quant[0][s->qscale]]);
        log_text_p(log_fd_dsp_text,txtch,r->p_pic->frame_cnt);

        log_text_p(log_fd_dsp_text,"<<<<< Inverse transform & DeQuant >>>>>", r->p_pic->frame_cnt);

    #endif


    if(r->is16){
        memset(block16, 0, sizeof(block16));
        rv34_amba_decode_block(block16, gb, r->cur_vlcs, 3, 0);

        #ifdef __dump_DSP_TXT__
        log_text_p(log_fd_dsp_text,"{ 16x16 SubBlock:}",r->p_pic->frame_cnt);
        log_text_p(log_fd_dsp_text,"<< Before Inverse Quantization: >>",r->p_pic->frame_cnt);
        log_text_rect_short_p(log_fd_dsp_text, block16, 4, 4, 4,r->p_pic->frame_cnt);
        #endif

        #ifndef __commit_out_idct_dequant__

        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_idct_deq])
        #endif
        {
        rv34_amba_dequant4x4_16x16(block16, rv34_qscale_tab[luma_dc_quant],rv34_qscale_tab[s->qscale]);

        #ifdef __dump_DSP_TXT__
        log_text_p(log_fd_dsp_text,"<< After Inverse Quantization: >>",r->p_pic->frame_cnt);
        log_text_rect_short_p(log_fd_dsp_text, block16, 4, 4, 4,r->p_pic->frame_cnt);
        #endif

        if(r->neon.trans[trans_type_dc] != NULL){
           r->neon.trans[trans_type_dc](block16);
        }else{
           rv34_amba_inv_transform_noround(block16);
        }

        #ifdef __dump_DSP_TXT__
        log_text_p(log_fd_dsp_text,"<< After Inverse transform: >>",r->p_pic->frame_cnt);
        log_text_rect_short_hex_p(log_fd_dsp_text, block16, 4, 4, 4,r->p_pic->frame_cnt);
        #endif

        }
        #endif
    }

    for(i = 0; i < 16; i++, cbp >>= 1){
        ptmp=pmby+((i&0xc)<<4)+((i&0x3)<<2);
        if(!r->is16 && !(cbp & 1))
        {
            continue;
        }


        #ifdef __dump_DSP_TXT__
        snprintf(txtch,79,"{ SubBlock: %d }",i);
        log_text_p(log_fd_dsp_text,txtch,r->p_pic->frame_cnt);
        #endif

        if(cbp & 1)
        {
            rv34_amba_decode_block_y(ptmp, gb, r->cur_vlcs, r->luma_vlc, 0);

            #ifdef __dump_DSP_TXT__
            log_text_p(log_fd_dsp_text,"<< Before Inverse Quantization: >>",r->p_pic->frame_cnt);
            log_text_rect_short_p(log_fd_dsp_text, ptmp, 4, 4, 12,r->p_pic->frame_cnt);
            #endif
        }

        #ifndef __commit_out_idct_dequant__

        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_idct_deq])
        #endif
        {
            rv34_amba_dequant4x4_y(ptmp, rv34_qscale_tab[s->qscale],rv34_qscale_tab[s->qscale]);
            if(r->is16) //FIXME: optimize
                *ptmp = block16[(i & 3) | ((i & 0xC) << 1)];

            #ifdef __dump_DSP_TXT__
            log_text_p(log_fd_dsp_text,"<< After Inverse Quantization and add DC: >>",r->p_pic->frame_cnt);
            log_text_rect_short_p(log_fd_dsp_text, ptmp, 4, 4, 12,r->p_pic->frame_cnt);
            #endif

            if(r->neon.trans[trans_type_16x16] != NULL){
                r->neon.trans[trans_type_16x16](ptmp);
            }else{
                rv34_amba_inv_transform_y(ptmp);
            }

            #ifdef __dump_DSP_TXT__
            log_text_p(log_fd_dsp_text,"<< After Inverse transform: >>",r->p_pic->frame_cnt);
            log_text_rect_short_hex_p(log_fd_dsp_text, ptmp, 4, 4, 12,r->p_pic->frame_cnt);
            #endif
        }
        #endif

    }
    if(r->block_type == RV34_MB_P_MIX16x16)
        r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 1);

    for(; i < 24; i++, cbp >>= 1){
//        ptmp=pmbuv+((i&0x2)<<5)+((i&0x1)<<3)+(!(i<20));
        ptmp=pmbuv+((i&0x6)<<4)+((i&0x1)<<2);
        if(!(cbp & 1))
        {
            continue;
        }

        #ifdef __dump_DSP_TXT__
        snprintf(txtch,79,"{ SubBlock: %d }",i);
        log_text_p(log_fd_dsp_text,txtch,r->p_pic->frame_cnt);
        #endif

        rv34_amba_decode_block(ptmp, gb, r->cur_vlcs, r->chroma_vlc, 1);

        #ifdef __dump_DSP_TXT__
        log_text_p(log_fd_dsp_text,"<< Before Inverse Quantization: >>",r->p_pic->frame_cnt);
        log_text_rect_short_p(log_fd_dsp_text, ptmp, 4, 4, 4,r->p_pic->frame_cnt);
        #endif

        #ifndef __commit_out_idct_dequant__

        #ifdef __log_decoding_config__
        if(log_mask[decoding_config_idct_deq])
        #endif
        {
                rv34_amba_dequant4x4(ptmp, rv34_qscale_tab[rv34_chroma_quant[1][s->qscale]],rv34_qscale_tab[rv34_chroma_quant[0][s->qscale]]);

                #ifdef __dump_DSP_TXT__
                log_text_p(log_fd_dsp_text,"<< After Inverse Quantization: >>",r->p_pic->frame_cnt);
                log_text_rect_short_p(log_fd_dsp_text, ptmp, 4, 4, 4,r->p_pic->frame_cnt);
                #endif

                if(r->neon.trans[trans_type_8x8] != NULL){
                    r->neon.trans[trans_type_8x8](ptmp);
                }else{
                    rv34_amba_inv_transform(ptmp);
                }

                #ifdef __dump_DSP_TXT__
                log_text_p(log_fd_dsp_text,"<< After Inverse transform: >>",r->p_pic->frame_cnt);
                log_text_rect_short_hex_p(log_fd_dsp_text, ptmp, 4, 4, 4,r->p_pic->frame_cnt);
                #endif
        }
        #endif

    }

    #ifdef __log_dump_idct_data__
    if(s->mb_x>=log_start_mb_x && s->mb_x<=log_end_mb_x && s->mb_y>=log_start_mb_y && s->mb_y<=log_end_mb_y )
    {
        log_idct_txt(log_fd_idct_text,pmby,16);
        log_idct_txt_uv(log_fd_idct_text,pmbuv,8);
    }
    #endif

#ifdef __dump_DSP_TXT__
    log_text_p(log_fd_dsp_text,"<<<<< total inverse transform result: >>>>>",r->p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text,"Y 0:",r->p_pic->frame_cnt);
    log_text_rect_short_hex_p(log_fd_dsp_text,pmby,8,8,8,r->p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text,"Y 1:",r->p_pic->frame_cnt);
    log_text_rect_short_hex_p(log_fd_dsp_text,pmby+8,8,8,8,r->p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text,"Y 2:",r->p_pic->frame_cnt);
    log_text_rect_short_hex_p(log_fd_dsp_text,pmby+128,8,8,8,r->p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text,"Y 3:",r->p_pic->frame_cnt);
    log_text_rect_short_hex_p(log_fd_dsp_text,pmby+136,8,8,8,r->p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text,"<< inverse transform result U: >>",r->p_pic->frame_cnt);
    log_text_rect_short_hex_p(log_fd_dsp_text,pmbuv,8,8,0,r->p_pic->frame_cnt);
    log_text_p(log_fd_dsp_text,"<< inverse transform result V: >>",r->p_pic->frame_cnt);
    log_text_rect_short_hex_p(log_fd_dsp_text,pmbuv+64,8,8,0,r->p_pic->frame_cnt);
#endif

    #ifndef __remove_memory_op__
    {
        if(IS_INTRA(s->current_picture_ptr->mb_type[mb_pos]))
        {
            rv34new_amba_output_macroblock(r, intra_types, r->is16);
            #ifdef _tmp_rv_dsp_setting_
            i=s->mb_width*s->mb_y+s->mb_x;
            r->p_pic->pmbtype[i]=r->is16?(is16x16|cbp2<<4):(cbp2<<4);
            //fake mv=0 for intra mb
            int* pmvb;//
            if(s->pict_type==FF_B_TYPE)
            {
                pmvb=(int*)((RVDEC_MBMV_B_t*)r->p_pic->pinfo+i);
                pmvb[0]=pmvb[2]=pmvb[4]=pmvb[6]=0x80000000;
                pmvb[1]=pmvb[3]=pmvb[5]=pmvb[7]=0x84000000;
            }
            else if(s->pict_type==FF_P_TYPE)
            {
                pmvb=(int*)((RVDEC_MBMV_P_t*)r->p_pic->pinfo+i);
                pmvb[0]=pmvb[1]=pmvb[2]=pmvb[3]=0x80000000;
                pmvb[4]=pmvb[5]=pmvb[6]=pmvb[7]=0x0;
            }

            #endif
        }
    }
    #endif

    return 0;
}

void* thread_rv34_amba_deblock(void* p);
void* thread_rv34_amba_mcintrapred(void* p);

static int reset_width_height(RV34AmbaDecContext *r);
static int reset_width_height_hybrid(RV34AmbaDecContext *r);
static int rv34new_amba_decode_slice(RV34AmbaDecContext *r, int end, const uint8_t* buf, int buf_size, int64_t pts)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int mb_pos;
    int res;
    ctx_nodef_t* p_node;
    Picture* ppic;

#ifdef __log_dump_decocer_input_data__
        log_dump_with_audo_cnt("slicedata2", buf, buf_size);
#endif
    init_get_bits(&r->s.gb, buf, buf_size*8);
    res = r->parse_slice_header(r, gb, &r->si);
    if(res < 0){
        av_log(s->avctx, AV_LOG_ERROR, "Incorrect or unknown slice header\n");
        return -1;
    }

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR," rv34_amba_decode_slice in, s->mb_y=%d.\n",s->mb_y);
    #endif

    if ((s->mb_x == 0 && s->mb_y == 0) || s->current_picture_ptr==NULL) {

        if (s->width != r->si.width || s->height != r->si.height) {
            av_log(s->avctx, AV_LOG_ERROR, "Changing dimensions to %dx%d\n", r->si.width,r->si.height);

            if(r->use_dsp){
                res = reset_width_height_hybrid(r);
            }else{
                res = reset_width_height(r);
            }
            if(res < 0){
                return -1;
            }
        }

        s->pict_type = r->si.type ? r->si.type : FF_I_TYPE;
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"        start decoding picture, s->pict_type=%d.\n",s->pict_type);
        #endif

        //update previous_pic_is_b
//        r->previous_pic_is_b=s->pict_type==FF_B_TYPE;
        ambadec_assert_ffmpeg(r->pic_finished);
        if(!r->p_pic)
        {
            //first start decoding?
//            av_log(NULL,AV_LOG_ERROR,"start decoding, r->p_pic=NULL.\n");
            p_node=r->p_pic_dataq->get(r->p_pic_dataq);
            r->p_pic=p_node->p_ctx;
            //exit indicator?
            if(!r->p_pic)
            {
                av_log(NULL,AV_LOG_ERROR,"r->p_pic_dataq->get==NULL,exit.\n");
                return -2;
            }
            ambadec_assert_ffmpeg(p_node->p_ctx==(&r->picdata[0])||p_node->p_ctx==(&r->picdata[1]));
        }

#ifdef _intra_using_allocate_residue_
        if (!r->p_pic->pdct) {
            if (s->pict_type == FF_I_TYPE)
                _get_nextbuffers(r, r->p_pic, 1);
            else
                _get_nextbuffers(r, r->p_pic, 0);
        }
#endif
        ambadec_assert_ffmpeg(r->p_pic);
        ambadec_assert_ffmpeg(r->p_pic->pdct);

        r->p_pic->new_residue_layout = 0;

        if(r->pic_finished)
        {
            s->current_picture_ptr=(Picture*)r->pic_pool->get(r->pic_pool,&res);
            if (s->avctx->get_buffer(s->avctx,(AVFrame*)s->current_picture_ptr) < 0) {
                av_log(NULL,AV_LOG_ERROR,"get_buffer fail, udec in error state, or stoped.\n");
                r->pic_pool->put(r->pic_pool, s->current_picture_ptr);
                s->current_picture_ptr = NULL;
                return -3;
            }
            s->current_picture_ptr->ref_added_for_extern = 1;
            s->current_picture_ptr->pts = pts;
            pthread_mutex_lock(&r->mutex);
            r->p_pic->current_picture_ptr=s->current_picture_ptr;
            pthread_mutex_unlock(&r->mutex);
        }
        else
        {
            pthread_mutex_lock(&r->mutex);
            r->p_pic->current_picture_ptr=s->current_picture_ptr;
            pthread_mutex_unlock(&r->mutex);
        }

//        #ifdef __dump_DSP_test_data__
        r->p_pic->frame_cnt=log_cur_frame;
//        #endif
        ppic=s->current_picture_ptr;
        ppic->pict_type=s->pict_type;
        r->p_pic->width=s->width;
        r->p_pic->height=s->height;
        r->p_pic->mb_width=s->mb_width;
        r->p_pic->mb_height=s->mb_height;
        r->p_pic->mb_stride=s->mb_stride;

        s->linesize=ppic->linesize[0];
        s->uvlinesize=ppic->linesize[1];

        _reset_picture_data(r->p_pic);

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"!!!start decoding new picture: picture type=%d, r->p_pic=%p,[0]=%p,[1]=%p .\n",s->pict_type,r->p_pic,&r->picdata[0],&r->picdata[1]);
        #endif

        ppic->pict_type= s->pict_type;
        ppic->key_frame= s->pict_type == FF_I_TYPE;
        if(ppic->pict_type!=FF_B_TYPE)
        {
            assert(ppic->key_frame ||s->last_picture_ptr);
            //reference
            if(r->pic_pool->dec_lock(r->pic_pool,s->last_picture_ptr)==1)
                r->s.avctx->release_buffer(r->s.avctx,(AVFrame*)s->last_picture_ptr);
            s->last_picture_ptr=s->next_picture_ptr;
            s->next_picture_ptr=ppic;
            r->pic_pool->inc_lock(r->pic_pool,s->next_picture_ptr);
        }

        pthread_mutex_lock(&r->mutex);
        r->p_pic->last_picture_ptr=s->last_picture_ptr;
        r->p_pic->next_picture_ptr=s->next_picture_ptr;
        r->pic_pool->inc_lock(r->pic_pool,s->last_picture_ptr);
        r->pic_pool->inc_lock(r->pic_pool,s->next_picture_ptr);
        r->pic_pool->inc_lock(r->pic_pool,ppic);
        pthread_mutex_unlock(&r->mutex);

        ff_er_frame_start(s);
        r->cur_pts = r->si.pts;
        if(s->pict_type != FF_B_TYPE){
            r->last_pts = r->next_pts;
            r->next_pts = r->cur_pts;
        }
        s->mb_x = s->mb_y = 0;

    }

    r->si.end = end;
    s->qscale = r->si.quant;
    r->bits = buf_size*8;
    s->mb_num_left = r->si.end - r->si.start;
    r->s.mb_skip_run = 0;

    mb_pos = s->mb_x + s->mb_y * s->mb_width;
    if(r->si.start != mb_pos){
        av_log(s->avctx, AV_LOG_ERROR, "Slice indicates MB offset %d, got %d\n", r->si.start, mb_pos);
        s->mb_x = r->si.start % s->mb_width;
        s->mb_y = r->si.start / s->mb_width;
    }
    memset(r->intra_types_hist, -1, r->intra_types_stride * 4 * 2 * sizeof(*r->intra_types_hist));
    s->first_slice_line = 1;
    s->resync_mb_x= s->mb_x;
    s->resync_mb_y= s->mb_y;

    ff_init_block_index_amba(s);
    while(!check_slice_end_amba(r, s)) {
        //av_log(NULL,AV_LOG_ERROR,"3. mb_height =%d, sync counter = %u.\n", r->picdata[0].mb_height, *r->pAcc->p_sync_counter);
        ff_update_block_index_amba(s);

        if(rv34new_amba_decode_macroblock(r, r->intra_types + s->mb_x * 4 + 4) < 0){
            ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_ERROR|DC_ERROR|MV_ERROR);
            return -1;
        }
        if (++s->mb_x == s->mb_width) {
            s->mb_x = 0;
            s->mb_y++;

            ff_init_block_index_amba(s);

            memmove(r->intra_types_hist, r->intra_types, r->intra_types_stride * 4 * sizeof(*r->intra_types_hist));
            memset(r->intra_types, -1, r->intra_types_stride * 4 * sizeof(*r->intra_types_hist));

            if(s->pict_type==FF_I_TYPE)
            {
                //trigger next thread
                process_data_t* p_mcintrapred;

                p_node= r->p_mcintrapred_dataq->get_free(r->p_mcintrapred_dataq);
                if(!p_node->p_ctx)
                {
                    p_node->p_ctx=av_malloc(sizeof(process_data_t));
                }
                p_mcintrapred=p_node->p_ctx;
                p_mcintrapred->p_picdata=r->p_pic;
                p_mcintrapred->end_row=s->mb_y;
                p_mcintrapred->start_row=r->vld_sent_row;

                ambadec_assert_ffmpeg(r->vld_sent_row<s->mb_y);
                #ifdef __log_communicate_queue__
                    av_log(NULL,AV_LOG_ERROR,"    trigger mc_intrapred mbrow=%d-%d start.\n",p_mcintrapred->start_row,p_mcintrapred->end_row);
                #endif
                ambadec_assert_ffmpeg(r->p_pic);
                if(r->s.mb_y==r->s.mb_height)
                {
                    pthread_mutex_lock(&r->mutex);
                    r->decoding_frame_cnt++;
                    pthread_mutex_unlock(&r->mutex);
                }
                r->vld_sent_row=s->mb_y;
                r->p_mcintrapred_dataq->put_ready(r->p_mcintrapred_dataq,p_node,0);
            }

        }
        if(s->mb_x == s->resync_mb_x)
            s->first_slice_line=0;
        s->mb_num_left--;
    }
    ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR," rv34new_amba_decode_slice out.\n");
    #endif

    return s->mb_y == s->mb_height;
}

static int reset_width_height(RV34AmbaDecContext *r)
{
    MpegEncContext *s = &r->s;
    ctx_nodef_t* p_node;
    int res;
#ifdef __log_decoding_process__
    av_log(NULL,AV_LOG_ERROR,"Changing dimensions to %dx%d\n", r->si.width,r->si.height);
#endif

#ifdef __log_parallel_control__
//                av_log(NULL,AV_LOG_ERROR,"Changing dimensions to %dx%d,start quit sub threads,r->p_thread_exit->current_cnt=%d \n", r->si.width,r->si.height,r->p_thread_exit->current_cnt);
#endif
    //parallel related
    //quit sub process thread
    r->mc_intrapred_loop=r->deblock_loop=0;

    //need indicate next quit
    p_node=r->p_mcintrapred_dataq->get_cmd(r->p_mcintrapred_dataq);
    r->p_mcintrapred_dataq->put_ready(r->p_mcintrapred_dataq,p_node,_flag_cmd_exit_next_);

    p_node=r->p_deblock_dataq->get_cmd(r->p_deblock_dataq);
    r->p_deblock_dataq->put_ready(r->p_deblock_dataq,p_node,_flag_cmd_exit_next_);

    //r->p_thread_exit->dequeue(r->p_thread_exit);
    //r->p_thread_exit->dequeue(r->p_thread_exit);
    void* pv1,*pv2;
    int ret1, ret2;
    ret1=pthread_join(r->tid_mc_intrapred,&pv1);
    ret2=pthread_join(r->tid_deblock,&pv2);
    av_log(NULL,AV_LOG_DEBUG,"    joined result=%d,%d.\n",ret1,ret2);

#ifdef __log_parallel_control__
    av_log(NULL,AV_LOG_ERROR,"    quit sub threads done.\n");
#endif

    av_freep(&r->intra_types_hist);
    av_freep(&r->mb_type);

    ambadec_reset_triqueue(r->p_frame_pool);
    ambadec_reset_triqueue(r->p_mcintrapred_dataq);
    ambadec_reset_triqueue( r->p_deblock_dataq);
    ambadec_destroy_triqueue( r->p_pic_dataq);

    //release pictures
    p_node= r->pic_pool->used_head.p_next;
    while(p_node != &r->pic_pool->used_head)
    {
        s->avctx->release_buffer(s->avctx,(AVFrame*)p_node->p_ctx);
        p_node=p_node->p_next;
    }
    ambadec_destroy_pool(r->pic_pool);
    for(res=0;res<6;res++)
    {
        free_picture_amba(&s->picture[res]);
    }

    MPV_common_end(s);
    s->width  = r->si.width;
    s->height = r->si.height;
    if(MPV_common_init(s) < 0)
        return -1;

    //reset codec width/height
    s->avctx->width = r->si.width;
    s->avctx->height = r->si.height;

    r->intra_types_stride = 4*s->mb_stride + 4;
    r->intra_types_hist = av_malloc(r->intra_types_stride * 4 * 2 * sizeof(*r->intra_types_hist));
    r->intra_types = r->intra_types_hist + r->intra_types_stride * 4;
    r->mb_type = av_mallocz(r->s.mb_stride * r->s.mb_height * sizeof(*r->mb_type));

    r->pic_pool=ambadec_create_pool();
    for(res=0;res<6;res++)
    {
        r->pic_pool->put(r->pic_pool,&s->picture[res]);
        ff_alloc_picture_amba(s, &s->picture[res]);
    }

    r->p_pic_dataq=ambadec_create_triqueue(_callback_NULL);
    //r->p_pic_dataq=ambadec_create_triqueue();

    r->pAcc->amba_picture_real_width = r->si.width;
    r->pAcc->amba_picture_real_height = r->si.height;
    r->pAcc->amba_picture_width = ( r->si.width + 15)&(~15);
    r->pAcc->amba_picture_height = ( r->si.height + 15)&(~15);

    for(res=0;res<2;res++)
    {
        r->picdata[res].width = r->pAcc->amba_picture_width;//s->width;
        r->picdata[res].height = r->pAcc->amba_picture_height;//s->height;
        r->picdata[res].mb_width=s->mb_width;
        r->picdata[res].mb_height=s->mb_height;
        r->picdata[res].mb_stride=s->mb_stride;

        r->picdata[res].pbase=av_malloc(s->mb_width*s->mb_height*sizeof(RVDEC_MBINFO_t)+15);
        r->picdata[res].pinfo=(RVDEC_MBINFO_t*)(((unsigned long)(r->picdata[res].pbase)+15)&(~0xf));

        #ifdef _tmp_rv_dsp_setting_
            r->picdata[res].pintra=av_malloc(s->mb_width*s->mb_height*sizeof(RVDEC_INTRAPRED_t));
            r->picdata[res].pmbtype=av_malloc(s->mb_width*s->mb_height*sizeof(int));
        #else
            r->picdata[res].pintra=(RVDEC_INTRAPRED_t*)r->picdata[res].pinfo;
        #endif

        //r->picdata[res].pdctbase=av_malloc(sizeof(DCTELEM)*s->mb_width*s->mb_height*(256+128)+15);
        //r->picdata[res].pdct=(DCTELEM*)(((unsigned long)(r->picdata[res].pdctbase)+15)&(~0xf));  
        r->picdata[res].pdctbase=av_malloc(sizeof(DCTELEM)*s->mb_width*s->mb_height*(256+128) +  r->pAcc->amba_picture_width*48 +  15);
        r->picdata[res].pdct=(DCTELEM*)(((unsigned long)(r->picdata[res].pdctbase)+15)&(~0xf));
        r->picdata[res].pdct +=  s->mb_width*8*8*6;

        //r->picdata[res].pdctu=r->picdata[res].pdct+s->mb_width*s->mb_height*256;
        //r->picdata[res].pdctv=r->picdata[res].pdctu+s->mb_width*s->mb_height*64;

        r->picdata[res].cbp_luma   = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->p_pic->cbp_luma));
        r->picdata[res].cbp_chroma = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->p_pic->cbp_chroma));
        r->picdata[res].deblock_coefs = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->p_pic->deblock_coefs));

        p_node=r->p_pic_dataq->get_free(r->p_pic_dataq);
        p_node->p_ctx=&r->picdata[res];
        r->p_pic_dataq->put_ready(r->p_pic_dataq,p_node,0);
    }

    r->vld_sent_row=r->mc_sent_row=0;
    r->pic_finished=1;
    r->previous_pic_is_b=1;
    /*p_node=r->p_pic_dataq->get(r->p_pic_dataq);
    ambadec_assert_ffmpeg(p_node->p_ctx==(&r->picdata[0])||p_node->p_ctx==(&r->picdata[1]));
    r->p_pic=p_node->p_ctx;      */
    r->p_pic = NULL;

    //parallel related
    r->mc_intrapred_loop=r->deblock_loop=1;

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR," restart sub threads in.\n");
    #endif

    #ifdef __log_parallel_control__
        av_log(NULL,AV_LOG_ERROR,"    restart sub threads.\n");
    #endif

    //spwan sub process thread
    //start mc intrapred thread
    pthread_create(&r->tid_mc_intrapred,NULL,thread_rv34_amba_mcintrapred,r);

    //start deblock thread
    pthread_create(&r->tid_deblock,NULL,thread_rv34_amba_deblock,r);

    #ifdef __log_parallel_control__
        av_log(NULL,AV_LOG_ERROR,"    restart sub threads done.\n");
    #endif
    return 0;
}

static int reset_width_height_hybrid(RV34AmbaDecContext *r)
{
    MpegEncContext *s = &r->s;
    ctx_nodef_t* p_node;
    int res,i;

    void* pv1,*pv2;
    int ret1, ret2;

#ifdef __log_decoding_process__
    av_log(NULL,AV_LOG_ERROR,"Changing dimensions to %dx%d --- bybrid\n", r->si.width,r->si.height);
#endif

#ifdef __log_parallel_control__
    av_log(NULL,AV_LOG_ERROR,"Changing dimensions to %dx%d,start quit sub threads ---bybrid\n", r->si.width,r->si.height);
#endif

    //parallel related
    //quit sub process thread
    r->mc_intrapred_loop=r->deblock_loop=0;

    p_node=r->p_mcintrapred_dataq->get_cmd(r->p_mcintrapred_dataq);
    r->p_mcintrapred_dataq->put_ready(r->p_mcintrapred_dataq,p_node,_flag_cmd_exit_next_);
    ret1=pthread_join(r->tid_mc_intrapred,&pv1);

    p_node=r->p_deblock_dataq->get_cmd(r->p_deblock_dataq);
    r->p_deblock_dataq->put_ready(r->p_deblock_dataq,p_node,_flag_cmd_exit_next_);
    ret2=pthread_join(r->tid_deblock,&pv2);
    av_log(NULL,AV_LOG_DEBUG,"    joined result=%d,%d.\n",ret1,ret2);

#ifdef __log_parallel_control__
    av_log(NULL,AV_LOG_ERROR,"    quit sub threads done.\n");
#endif

    av_freep(&r->intra_types_hist);
    av_freep(&r->mb_type);

    ambadec_reset_triqueue(r->p_frame_pool);
    ambadec_reset_triqueue(r->p_mcintrapred_dataq);
    ambadec_reset_triqueue( r->p_deblock_dataq);
    ambadec_destroy_triqueue( r->p_pic_dataq);

    //release pictures
    p_node= r->pic_pool->used_head.p_next;
    while(p_node != &r->pic_pool->used_head)
    {
        s->avctx->release_buffer(s->avctx,(AVFrame*)p_node->p_ctx);
        p_node=p_node->p_next;
    }
    ambadec_destroy_pool(r->pic_pool);
    for(res=0;res<6;res++)
    {
        free_picture_amba(&s->picture[res]);
    }

    MPV_common_end(s);

    //reinit with new width/height
    s->width  = r->si.width;
    s->height = r->si.height;
    if(MPV_common_init(s) < 0)
        return -1;

    //reset codec width/height
    s->avctx->width = s->width;
    s->avctx->height = s->height;

    r->intra_types_stride = 4*s->mb_stride + 4;
    r->intra_types_hist = av_malloc(r->intra_types_stride * 4 * 2 * sizeof(*r->intra_types_hist));
    r->intra_types = r->intra_types_hist + r->intra_types_stride * 4;
    r->mb_type = av_mallocz(r->s.mb_stride * r->s.mb_height * sizeof(*r->mb_type));

    r->pic_pool=ambadec_create_pool();
    for(res=0;res<6;res++)
    {
        r->pic_pool->put(r->pic_pool,&s->picture[res]);
        ff_alloc_picture_amba(s, &s->picture[res]);
    }

    r->p_pic_dataq=ambadec_create_triqueue(_callback_NULL);

    r->pAcc->amba_picture_real_width = r->si.width;
    r->pAcc->amba_picture_real_height = r->si.height;
    r->pAcc->amba_picture_width = ( r->si.width + 15)&(~15);
    r->pAcc->amba_picture_height = ( r->si.height + 15)&(~15);

    r->pAcc->amba_mv_buffer_size = (r->pAcc->amba_picture_width>>4) * (r->pAcc->amba_picture_height>>4) * sizeof(RVDEC_MBINFO_t);
    r->pAcc->amba_idct_buffer_size = (r->pAcc->amba_picture_width>>4) * (r->pAcc->amba_picture_height>>4) * sizeof(mb_idctcoef_t);
    //notify dsp that resolution changed
    if(r->pAcc->rv40_change_resolution){
        av_log(NULL,AV_LOG_DEBUG,"rv40_change_resolution %dx%d, decoder_id = %d,amba_iav_fd = %d\n",\
            r->pAcc->amba_picture_real_width,r->pAcc->amba_picture_real_height,r->pAcc->decode_id,r->pAcc->amba_iav_fd);
        r->pAcc->rv40_change_resolution(r->pAcc);
    }

#ifdef _intra_using_allocate_residue_
    //circlar
    r->pb_current_index = 0;
    r->pb_tot_index = r->pAcc->amba_buffer_number - 1;
    r->i_current_index = 0;
    r->i_tot_index = max_i_number - 1;
    r->result_current_index = 0;
    r->result_tot_index = r->pAcc->amba_buffer_number - 1;
#endif

    //av_log(NULL,AV_LOG_ERROR,"****amba_buffer_number %d.s->mb_height = %d,s->mb_width = %d,slice_count %d\n", r->pAcc->amba_buffer_number,s->mb_height,s->mb_width,s->avctx->slice_count);
    ambadec_assert_ffmpeg(r->pAcc->amba_buffer_number);
    for(i=0; i<r->pAcc->amba_buffer_number; i++)
    {
        r->picdata[i].pbase = NULL;

        r->picdata[i].width = r->pAcc->amba_picture_width;//s->width;
        r->picdata[i].height = r->pAcc->amba_picture_height;//s->height;
        r->picdata[i].mb_width=s->mb_width;
        r->picdata[i].mb_height=s->mb_height;
        r->picdata[i].mb_stride=s->mb_stride;

        #ifdef _tmp_rv_dsp_setting_
            r->picdata[i].pintra=av_malloc(s->mb_width*s->mb_height*sizeof(RVDEC_INTRAPRED_t));
            r->picdata[i].pmbtype=av_malloc(s->mb_width*s->mb_height*sizeof(int));
        #else
            r->picdata[i].pintra=(RVDEC_INTRAPRED_t*)r->picdata[i].pinfo;
        #endif

        r->picdata[i].pdctbase = NULL;
        r->picdata[i].picinfo.decoder_id = r->pAcc->decode_id;
        r->picdata[i].picinfo.udec_type = r->pAcc->decode_type;//rv40
        ambadec_assert_ffmpeg(r->pAcc->decode_type == rv40_hybird);
        r->picdata[i].picinfo.num_pics = 1;
        r->picdata[i].picinfo.uu.rv40.tiled_mode = 0;//raster scan mode
        r->picdata[i].picinfo.uu.rv40.pic_width = r->pAcc->amba_picture_width;
        r->picdata[i].picinfo.uu.rv40.pic_height = r->pAcc->amba_picture_height;

#ifdef _intra_using_allocate_residue_
        r->pb_mv[i] = r->pAcc->p_amba_mv_ringbuffer + r->pAcc->amba_mv_buffer_size * i;
        r->pb_residue[i] = r->pAcc->p_amba_idct_ringbuffer + r->pAcc->amba_idct_buffer_size * i;
#else
        r->picdata[i].pmc_result = r->pAcc->p_mcresult_buffer[i];
        r->picdata[i].pmc_result_buffer_id = r->pAcc->mcresult_buffer_id[i];
        av_log(NULL, AV_LOG_ERROR, "**reserved for mc result buffer: %p, %d.\n", r->picdata[i].pmc_result, r->picdata[i].pmc_result_buffer_id);

        r->picdata[i].pinfo = (RVDEC_MBINFO_t*)(r->pAcc->p_amba_mv_ringbuffer + r->pAcc->amba_mv_buffer_size * i);

#ifdef _d_use_new_residue_layout_
        r->picdata[i].pdct=(DCTELEM*)(r->pAcc->p_amba_idct_ringbuffer + (r->pAcc->amba_idct_buffer_size + r->pAcc->amba_picture_width*48) * i);
        r->picdata[i].picinfo.uu.rv40.residual_daddr_start = (unsigned char *)r->picdata[i].pdct;
        r->picdata[i].picinfo.uu.rv40.residual_daddr_end = r->picdata[i].picinfo.uu.rv40.residual_daddr_start + r->pAcc->amba_idct_buffer_size;
        r->picdata[i].pdct += s->mb_width *8*8*6;

#else
        r->picdata[i].pdct=(DCTELEM*)(r->pAcc->p_amba_idct_ringbuffer + (r->pAcc->amba_idct_buffer_size + r->pAcc->amba_picture_width*48) * i);
        r->picdata[i].picinfo.uu.rv40.residual_daddr_start = r->picdata[i].pdct;
        r->picdata[i].picinfo.uu.rv40.residual_daddr_end = r->picdata[i].picinfo.uu.rv40.residual_daddr_start + r->pAcc->amba_idct_buffer_size;
#endif

        //constant picinfo settings
        r->picdata[i].picinfo.uu.rv40.mv_daddr_start = (unsigned char *)r->picdata[i].pinfo;
        r->picdata[i].picinfo.uu.rv40.mv_daddr_end = r->picdata[i].picinfo.uu.rv40.mv_daddr_start + r->pAcc->amba_mv_buffer_size;
#endif
        r->picdata[i].picinfo.uu.rv40.target_fb_id = r->pAcc->mcresult_buffer_id[i];
        r->picdata[i].cbp_luma   = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->picdata[i].cbp_luma));
        r->picdata[i].cbp_chroma = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->picdata[i].cbp_chroma));
        r->picdata[i].deblock_coefs = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->picdata[i].deblock_coefs));

        p_node=r->p_pic_dataq->get_free(r->p_pic_dataq);
        p_node->p_ctx=&r->picdata[i];
        r->p_pic_dataq->put_ready(r->p_pic_dataq,p_node,0);
    }
#ifdef _intra_using_allocate_residue_
    ambadec_assert_ffmpeg(i == r->pAcc->amba_buffer_number);
    ambadec_assert_ffmpeg(i < max_pb_number);
    r->pb_mv[i] = r->pAcc->p_amba_mv_ringbuffer + r->pAcc->amba_mv_buffer_size * i;
    r->pb_residue[i] = r->pAcc->p_amba_idct_ringbuffer + r->pAcc->amba_idct_buffer_size * i;
#endif

    r->vld_sent_row=r->mc_sent_row=0;
    r->pic_finished=1;
    r->previous_pic_is_b=1;
    r->p_pic = NULL;

    //parallel related
    r->mc_intrapred_loop=r->deblock_loop=1;

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR," restart sub threads in.\n");
    #endif

    #ifdef __log_parallel_control__
        av_log(NULL,AV_LOG_ERROR,"    restart sub threads.\n");
    #endif

    //spwan sub process thread
    //start mc intrapred thread
    pthread_create(&r->tid_mc_intrapred,NULL,thread_rv34_amba_mcintrapred,r);

    //start deblock thread
    pthread_create(&r->tid_deblock,NULL,thread_rv34_amba_deblock,r);

    #ifdef __log_parallel_control__
        av_log(NULL,AV_LOG_ERROR,"    restart sub threads done.\n");
    #endif

    return 0;
}
//
static inline void _add_dct_result_mb(uint8_t* pdesy,uint8_t* pdesuv ,DCTELEM* dct,int stride)
{
    int i,j;
    DCTELEM* dctv;
    //Y
    for(j=0;j<16;j++)
    {
        for(i=0;i<16;i++)
        {
            pdesy[i]=av_clip_uint8((*dct++)+pdesy[i]);
        }
        pdesy+=stride;
    }

    dctv=dct+64;
    //uv
    for(j=0;j<8;j++)
    {
        for(i=0;i<8;i++)
        {
            pdesuv[i<<1]=av_clip_uint8((*dct++)+pdesuv[i<<1]);
            pdesuv[(i<<1)+1]=av_clip_uint8((*dctv++)+pdesuv[(i<<1)+1]);
        }
        pdesuv+=stride;
    }
}

static inline void _add_dct_result_4x4_y(uint8_t* pdes ,DCTELEM* psrc,int stride)
{
    int i,j;

    //Y
    for(j=0;j<4;j++)
    {
        for(i=0;i<4;i++)
        {
            pdes[i]=av_clip_uint8((*psrc++)+pdes[i]);
        }
        psrc+=12;
        pdes+=stride;
    }

}

static inline void _add_dct_result_4x4_chroma_nv12(uint8_t* pdes,DCTELEM* psrc,int stride)
{
    int i,j;

    for(j=0;j<4;j++)
    {
        for(i=0;i<4;i++)
        {
            pdes[i*2]=av_clip_uint8(psrc[i] + pdes[i*2]);
        }
        pdes+=stride;
        psrc+=8;
    }
}

static inline void _add_dct_result_4x4_uv(uint8_t* pdes,DCTELEM* psrc,int stride)
{
    int i,j;
    DCTELEM* psrcv=psrc+64;

    for(j=0;j<4;j++)
    {
        for(i=0;i<4;i++)
        {
            pdes[i*2]=av_clip_uint8((*psrc++)+pdes[i*2]);
            pdes[i*2+1]=av_clip_uint8((*psrcv++)+pdes[i*2+1]);
        }
        pdes+=stride;
        psrc+=4;
        psrcv+=4;
    }
}

//process data
static void process_mc_rv34_amba_b(RV34AmbaDecContext *thiz,rv40_pic_data_t* p_picdata)
{
    RVDEC_MBMV_B_t* pmvb=(RVDEC_MBMV_B_t*)p_picdata->pinfo;
    int i,j,k;

    uint8_t *Y, *U, *Yy,*Uu, *srcY, *srcU;
    int dxy, mx, my, umx, umy, lx, ly, uvmx, uvmy, src_x, src_y, uvsrc_x, uvsrc_y;
    int cx,cy;
    int mv_x,mv_y;

    MpegEncContext *s = &thiz->s;
    DCTELEM* pdct=p_picdata->pdct;
    int linesize=p_picdata->current_picture_ptr->linesize[0];
    int uvlinesize=p_picdata->current_picture_ptr->linesize[1];

    #ifdef __log_mv_new__
    char lt[100];
    int em=0;
    #endif

    #ifdef __dump_mc_dct_DSP_TXT__
    char txtdump[100];
    #endif

    #ifdef _tmp_rv_dsp_setting_
    int* pmbtype=p_picdata->pmbtype;
    #endif

    for(j=0;j<p_picdata->mb_height;j++)
    {
        #ifdef _tmp_rv_dsp_setting_
        for(i=0;i<p_picdata->mb_width;i++,pmvb++,pdct+=384,pmbtype++)
        #else
        for(i=0;i<p_picdata->mb_width;i++,pmvb++,pdct+=384)
        #endif
        {

            Y = p_picdata->current_picture_ptr->data[0]+(i <<4) +(j<<4)*linesize;
            U = p_picdata->current_picture_ptr->data[1] +  (i<<4) + (j<<3) *uvlinesize;

            #ifdef __log_mv_new__
            snprintf(lt,99,"MB=[%d %d]",j,i);
            log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
            #endif

            #ifdef _tmp_rv_dsp_setting_
            if(*pmbtype==usefw)
            #else
            if(pmvb->mv[0].fwd.valid && !pmvb->mv[0].bwd.valid)
            #endif
            //forward
            {
                for(k=0;k<4;k++)
                {
                    mv_x=pmvb->mv[k].fwd.mv_x;
                    mv_y=pmvb->mv[k].fwd.mv_y;

                    mx=mv_x>>2;
                    my=mv_y>>2;
                    lx=mv_x&3;
                    ly=mv_y&3;
                    cx=mv_x/2;
                    cy=mv_y/2;
                    umx=cx>>2;
                    umy=cy>>2;
                    uvmx=(cx&3)<<1;
                    uvmy=(cy&3)<<1;
                    //due to some flaw RV40 uses the same MC compensation routine for H2V2 and H3V3
                    if(uvmx == 6 && uvmy == 6)
                        uvmx = uvmy = 4;

                    dxy = ly*4 + lx;
                    srcY =p_picdata->last_picture_ptr->data[0];
                    srcU =p_picdata->last_picture_ptr->data[1];

                    src_x = (i <<4) +((k&0x1)<<3)+ mx;
                    src_y = (j<<4) + ((k>>1)<<3) + my;
                    uvsrc_x = (i<<3) + ((k&0x1)<<2) + umx;
                    uvsrc_y = (j<<3) + ((k>>1)<<2) + umy;

                    srcY += src_y * linesize + src_x;
                    srcU += uvsrc_y * uvlinesize + (uvsrc_x<<1);

                    if(   (unsigned)(src_x - !!lx*2) > s->h_edge_pos - !!lx*2 - 8 - 4
                    || (unsigned)(src_y - !!ly*2) > s->v_edge_pos - !!ly*2 - 8 - 4)
                    {
                        #ifdef __log_mv_new__
                        em=1;
                        #endif
                        uint8_t *uvbuf= s->edge_emu_buffer + 22 * linesize;

                        srcY -= 2 + 2*linesize;
                        s->dsp.emulated_edge_mc(s->edge_emu_buffer, srcY, linesize, 14,14,
                                            src_x - 2, src_y - 2, s->h_edge_pos, s->v_edge_pos);
                        srcY = s->edge_emu_buffer + 2 + 2*linesize;

                        s->dsp.emulated_edge_mc_nv12(uvbuf, srcU, uvlinesize, 5, 5,
                                            uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
                        srcU = uvbuf;
                    }
                    #ifdef __log_mv_new__
                    else
                        em=0;
                    #endif

                    Yy=Y+((k&0x1)<<3)+((k>>1)<<3)*linesize;
                    Uu=U+((k&0x1)<<3)+((k>>1)<<2)*uvlinesize;

                    #ifdef __log_mv_new__
                    log_text_p(log_fd_mv_new,"Forward",p_picdata->frame_cnt);
                    snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    snprintf(lt,99,"des y offset=%d, des uv offset=%d",Yy-p_picdata->current_picture_ptr->data[0],Uu-p_picdata->current_picture_ptr->data[1]);
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    if(!em)
                        snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY-p_picdata->last_picture_ptr->data[0],srcU-p_picdata->last_picture_ptr->data[1]);
                    else
                        snprintf(lt,99,"use em buffer");
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    #endif

                    s->dsp.put_rv40_qpel_pixels_tab[1][dxy](Yy,srcY,linesize);
                    s->dsp.put_rv40_chroma_pixels_tab_nv12[1](Uu, srcU, uvlinesize, 4, uvmx, uvmy);
                    s->dsp.put_rv40_chroma_pixels_tab_nv12[1](Uu+1, srcU+1, uvlinesize, 4, uvmx, uvmy);
                }

            }
            #ifdef _tmp_rv_dsp_setting_
            else if(*pmbtype==usebw)
            #else
            else if(!pmvb->mv[0].fwd.valid && pmvb->mv[0].bwd.valid)//backward
            #endif
            {
                for(k=0;k<4;k++)
                {
                    mv_x=pmvb->mv[k].bwd.mv_x;
                    mv_y=pmvb->mv[k].bwd.mv_y;

                    mx=mv_x>>2;
                    my=mv_y>>2;
                    lx=mv_x&3;
                    ly=mv_y&3;
                    cx=mv_x/2;
                    cy=mv_y/2;
                    umx=cx>>2;
                    umy=cy>>2;
                    uvmx=(cx&3)<<1;
                    uvmy=(cy&3)<<1;
                    //due to some flaw RV40 uses the same MC compensation routine for H2V2 and H3V3
                    if(uvmx == 6 && uvmy == 6)
                        uvmx = uvmy = 4;

                    dxy = ly*4 + lx;
                    srcY =p_picdata->next_picture_ptr->data[0];
                    srcU =p_picdata->next_picture_ptr->data[1];

                    src_x = (i <<4) +((k&0x1)<<3)+ mx;
                    src_y = (j<<4) + ((k>>1)<<3) + my;
                    uvsrc_x = (i<<3) + ((k&0x1)<<2) + umx;
                    uvsrc_y = (j<<3) + ((k>>1)<<2) + umy;

                    srcY += src_y * linesize + src_x;
                    srcU += uvsrc_y * uvlinesize + (uvsrc_x<<1);

                    if(   (unsigned)(src_x - !!lx*2) > s->h_edge_pos - !!lx*2 - 8 - 4
                    || (unsigned)(src_y - !!ly*2) > s->v_edge_pos - !!ly*2 - 8 - 4)
                    {
                        #ifdef __log_mv_new__
                            em=1;
                        #endif
                        uint8_t *uvbuf= s->edge_emu_buffer + 22 * linesize;

                        srcY -= 2 + 2*linesize;
                        s->dsp.emulated_edge_mc(s->edge_emu_buffer, srcY, linesize, 14,14,
                                            src_x - 2, src_y - 2, s->h_edge_pos, s->v_edge_pos);
                        srcY = s->edge_emu_buffer + 2 + 2*linesize;

                        s->dsp.emulated_edge_mc_nv12(uvbuf, srcU, uvlinesize, 5, 5,
                                            uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
                        srcU = uvbuf;
                    }
                    #ifdef __log_mv_new__
                    else
                        em=0;
                    #endif

                    Yy=Y+((k&0x1)<<3)+((k>>1)<<3)*linesize;
                    Uu=U+((k&0x1)<<3)+((k>>1)<<2)*uvlinesize;

                    #ifdef __log_mv_new__
                    log_text_p(log_fd_mv_new,"Backward",p_picdata->frame_cnt);
                    snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    snprintf(lt,99,"des y offset=%d, des uv offset=%d",Yy-p_picdata->current_picture_ptr->data[0],Uu-p_picdata->current_picture_ptr->data[1]);
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    if(!em)
                        snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY-p_picdata->next_picture_ptr->data[0],srcU-p_picdata->next_picture_ptr->data[1]);
                    else
                        snprintf(lt,99,"use em buffer");
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    #endif

                    s->dsp.put_rv40_qpel_pixels_tab[1][dxy](Yy,srcY,linesize);
                    s->dsp.put_rv40_chroma_pixels_tab_nv12[1](Uu, srcU, uvlinesize, 4, uvmx, uvmy);
                    s->dsp.put_rv40_chroma_pixels_tab_nv12[1](Uu+1, srcU+1, uvlinesize, 4, uvmx, uvmy);
                }
            }
            #ifdef _tmp_rv_dsp_setting_
            else if(*pmbtype==mbisinter)
            #else
            else if(pmvb->mv[0].fwd.valid && pmvb->mv[0].bwd.valid)//bi-directional
            #endif
            {
                for(k=0;k<4;k++)
                {
                    mv_x=pmvb->mv[k].fwd.mv_x;
                    mv_y=pmvb->mv[k].fwd.mv_y;

                    mx=mv_x>>2;
                    my=mv_y>>2;
                    lx=mv_x&3;
                    ly=mv_y&3;
                    cx=mv_x/2;
                    cy=mv_y/2;
                    umx=cx>>2;
                    umy=cy>>2;
                    uvmx=(cx&3)<<1;
                    uvmy=(cy&3)<<1;
                    //due to some flaw RV40 uses the same MC compensation routine for H2V2 and H3V3
                    if(uvmx == 6 && uvmy == 6)
                        uvmx = uvmy = 4;

                    dxy = ly*4 + lx;
                    srcY =p_picdata->last_picture_ptr->data[0];
                    srcU =p_picdata->last_picture_ptr->data[1];

                    src_x = (i <<4) +((k&0x1)<<3)+ mx;
                    src_y = (j<<4) + ((k>>1)<<3) + my;
                    uvsrc_x = (i<<3) + ((k&0x1)<<2) + umx;
                    uvsrc_y = (j<<3) + ((k>>1)<<2) + umy;

                    srcY += src_y * linesize + src_x;
                    srcU += uvsrc_y * uvlinesize + (uvsrc_x<<1);

                    if(   (unsigned)(src_x - !!lx*2) > s->h_edge_pos - !!lx*2 - 8 - 4
                    || (unsigned)(src_y - !!ly*2) > s->v_edge_pos - !!ly*2 - 8 - 4)
                    {
                        #ifdef __log_mv_new__
                            em=1;
                        #endif
                        uint8_t *uvbuf= s->edge_emu_buffer + 22 * linesize;

                        srcY -= 2 + 2*linesize;
                        s->dsp.emulated_edge_mc(s->edge_emu_buffer, srcY, linesize, 14,14,
                                            src_x - 2, src_y - 2, s->h_edge_pos, s->v_edge_pos);
                        srcY = s->edge_emu_buffer + 2 + 2*linesize;

                        s->dsp.emulated_edge_mc_nv12(uvbuf, srcU, uvlinesize, 5, 5,
                                            uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
                        srcU = uvbuf;
                    }
                    #ifdef __log_mv_new__
                    else
                        em=0;
                    #endif

                    Yy=Y+((k&0x1)<<3)+((k>>1)<<3)*linesize;
                    Uu=U+((k&0x1)<<3)+((k>>1)<<2)*uvlinesize;

                    #ifdef __log_mv_new__
                    log_text_p(log_fd_mv_new,"Forward",p_picdata->frame_cnt);
                    snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    snprintf(lt,99,"des y offset=%d, des uv offset=%d",Yy-p_picdata->current_picture_ptr->data[0],Uu-p_picdata->current_picture_ptr->data[1]);
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    if(!em)
                        snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY-p_picdata->last_picture_ptr->data[0],srcU-p_picdata->last_picture_ptr->data[1]);
                    else
                        snprintf(lt,99,"use em buffer");
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    #endif

                    s->dsp.put_rv40_qpel_pixels_tab[1][dxy](Yy,srcY,linesize);
                    s->dsp.put_rv40_chroma_pixels_tab_nv12[1](Uu, srcU, uvlinesize, 4, uvmx, uvmy);
                    s->dsp.put_rv40_chroma_pixels_tab_nv12[1](Uu+1, srcU+1, uvlinesize, 4, uvmx, uvmy);

                    mv_x=pmvb->mv[k].bwd.mv_x;
                    mv_y=pmvb->mv[k].bwd.mv_y;

                    mx=mv_x>>2;
                    my=mv_y>>2;
                    lx=mv_x&3;
                    ly=mv_y&3;
                    cx=mv_x/2;
                    cy=mv_y/2;
                    umx=cx>>2;
                    umy=cy>>2;
                    uvmx=(cx&3)<<1;
                    uvmy=(cy&3)<<1;
                    //due to some flaw RV40 uses the same MC compensation routine for H2V2 and H3V3
                    if(uvmx == 6 && uvmy == 6)
                        uvmx = uvmy = 4;

                    dxy = ly*4 + lx;
                    srcY =p_picdata->next_picture_ptr->data[0];
                    srcU =p_picdata->next_picture_ptr->data[1];

                    src_x = (i <<4) +((k&0x1)<<3)+ mx;
                    src_y = (j<<4) + ((k>>1)<<3) + my;
                    uvsrc_x = (i<<3) + ((k&0x1)<<2) + umx;
                    uvsrc_y = (j<<3) + ((k>>1)<<2) + umy;

                    srcY += src_y * linesize + src_x;
                    srcU += uvsrc_y * uvlinesize + (uvsrc_x<<1);

                    if(   (unsigned)(src_x - !!lx*2) > s->h_edge_pos - !!lx*2 - 8 - 4
                    || (unsigned)(src_y - !!ly*2) > s->v_edge_pos - !!ly*2 - 8 - 4)
                    {
                        #ifdef __log_mv_new__
                            em=1;
                        #endif
                        uint8_t *uvbuf= s->edge_emu_buffer + 22 * linesize;

                        srcY -= 2 + 2*linesize;
                        s->dsp.emulated_edge_mc(s->edge_emu_buffer, srcY, linesize, 14,14,
                                            src_x - 2, src_y - 2, s->h_edge_pos, s->v_edge_pos);
                        srcY = s->edge_emu_buffer + 2 + 2*linesize;

                        s->dsp.emulated_edge_mc_nv12(uvbuf, srcU, uvlinesize, 5, 5,
                                            uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
                        srcU = uvbuf;
                    }
                    #ifdef __log_mv_new__
                    else
                        em=0;
                    #endif

                    #ifdef __log_mv_new__
                    log_text_p(log_fd_mv_new,"Backward",p_picdata->frame_cnt);
                    snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    snprintf(lt,99,"des y offset=%d, des uv offset=%d",Yy-p_picdata->current_picture_ptr->data[0],Uu-p_picdata->current_picture_ptr->data[1]);
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    if(!em)
                        snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY-p_picdata->next_picture_ptr->data[0],srcU-p_picdata->next_picture_ptr->data[1]);
                    else
                        snprintf(lt,99,"use em buffer");
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    #endif

                    s->dsp.avg_rv40_qpel_pixels_tab[1][dxy](Yy,srcY,linesize);
                    s->dsp.avg_rv40_chroma_pixels_tab_nv12[1](Uu, srcU, uvlinesize, 4, uvmx, uvmy);
                    s->dsp.avg_rv40_chroma_pixels_tab_nv12[1](Uu+1, srcU+1, uvlinesize, 4, uvmx, uvmy);

                }
            }
            else
            {
                #ifdef _tmp_rv_dsp_setting_
                ambadec_assert_ffmpeg(((*pmbtype)&0xf)==0 || ((*pmbtype)&0xf)==is16x16);
                #endif
                continue;
            }

            #ifdef __dump_mc_dct_DSP_TXT__
                snprintf(txtdump, 99, "-----inter MB[y=%d,x=%d]-----", j,i);
                log_text_p(log_fd_dsp_mc_dct_text,txtdump, p_picdata->frame_cnt);
                log_text_p(log_fd_dsp_mc_dct_text,"<<<<< total MC result Y: >>>>>", p_picdata->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_mc_dct_text,Y,16,16,linesize-16, p_picdata->frame_cnt);
                log_text_p(log_fd_dsp_mc_dct_text,"<<<<< total MC result U: >>>>>", p_picdata->frame_cnt);
                log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,U,8,8,uvlinesize-16, p_picdata->frame_cnt);
                log_text_p(log_fd_dsp_mc_dct_text,"<<<<< total MC result V: >>>>>", p_picdata->frame_cnt);
                log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,U+1,8,8,uvlinesize-16, p_picdata->frame_cnt);
            #endif

            if(thiz->neon.mb_addresidue != NULL){
                thiz->neon.mb_addresidue(Y,U,pdct,linesize);
            }else{
                _add_dct_result_mb(Y,U,pdct,linesize);
            }

            #ifdef __dump_mc_dct_DSP_TXT__
            log_text_p(log_fd_dsp_mc_dct_text,"<<<<< before deblock Y: >>>>>", p_picdata->frame_cnt);
            log_text_rect_char_hex_p(log_fd_dsp_mc_dct_text,Y,16,16,linesize-16, p_picdata->frame_cnt);
            log_text_p(log_fd_dsp_mc_dct_text,"<<<<< before deblock U: >>>>>", p_picdata->frame_cnt);
            log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,U,8,8,uvlinesize-16, p_picdata->frame_cnt);
            log_text_p(log_fd_dsp_mc_dct_text,"<<<<< before deblock V: >>>>>", p_picdata->frame_cnt);
            log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,U+1,8,8,uvlinesize-16, p_picdata->frame_cnt);
            #endif

        }
    }

}

static void process_mc_rv34_amba_p(RV34AmbaDecContext *thiz,rv40_pic_data_t* p_picdata)
{
    RVDEC_MBMV_P_t* pmvp=(RVDEC_MBMV_P_t*)p_picdata->pinfo;
    int i,j,k;

    uint8_t *Y, *U, *Yy,*Uu, *srcY, *srcU;
    int dxy, mx, my, umx, umy, lx, ly, uvmx, uvmy, src_x, src_y, uvsrc_x, uvsrc_y;
    int cx,cy;
    int mv_x,mv_y;

    MpegEncContext *s = &thiz->s;
    DCTELEM* pdct=p_picdata->pdct;
    int linesize=p_picdata->current_picture_ptr->linesize[0];
    int uvlinesize=p_picdata->current_picture_ptr->linesize[1];

    #ifdef _tmp_rv_dsp_setting_
    int* pmbtype=p_picdata->pmbtype;
    #endif

    #ifdef __log_mv_new__
    char lt[100];
    int em=0;
    #endif

    #ifdef __dump_mc_dct_DSP_TXT__
    char txtdump[100];
    #endif

    for(j=0;j<p_picdata->mb_height;j++)
    {
        #ifdef _tmp_rv_dsp_setting_
        for(i=0;i<p_picdata->mb_width;i++,pmvp++,pdct+=384,pmbtype++)
        #else
        for(i=0;i<p_picdata->mb_width;i++,pmvp++,pdct+=384)
        #endif
        {
            #ifdef __log_mv_new__
            snprintf(lt,99,"MB=[%d %d]",j,i);
            log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
            #endif

            Y = p_picdata->current_picture_ptr->data[0]+(i <<4) +(j<<4)*linesize;
            U = p_picdata->current_picture_ptr->data[1] +  (i<<4) + (j<<3) *uvlinesize;

            #ifdef _tmp_rv_dsp_setting_
            if(*pmbtype==usefw)
            #else
            //forward
            if(pmvp->fwd[0].valid)
            #endif
            {
                for(k=0;k<4;k++)
                {
                    mv_x=pmvp->fwd[k].mv_x;
                    mv_y=pmvp->fwd[k].mv_y;

                    mx=mv_x>>2;
                    my=mv_y>>2;
                    lx=mv_x&3;
                    ly=mv_y&3;
                    cx=mv_x/2;
                    cy=mv_y/2;
                    umx=cx>>2;
                    umy=cy>>2;
                    uvmx=(cx&3)<<1;
                    uvmy=(cy&3)<<1;
                    //due to some flaw RV40 uses the same MC compensation routine for H2V2 and H3V3
                    if(uvmx == 6 && uvmy == 6)
                        uvmx = uvmy = 4;

                    dxy = ly*4 + lx;
                    srcY =p_picdata->last_picture_ptr->data[0];
                    srcU =p_picdata->last_picture_ptr->data[1];

                    src_x = (i <<4) +((k&0x1)<<3)+ mx;
                    src_y = (j<<4) + ((k>>1)<<3) + my;
                    uvsrc_x = (i<<3) + ((k&0x1)<<2) + umx;
                    uvsrc_y = (j<<3) + ((k>>1)<<2) + umy;

                    srcY += src_y * linesize+ src_x;
                    srcU += uvsrc_y * uvlinesize + (uvsrc_x<<1);

                    if(   (unsigned)(src_x - !!lx*2) > s->h_edge_pos - !!lx*2 - 8 - 4
                    || (unsigned)(src_y - !!ly*2) > s->v_edge_pos - !!ly*2 - 8 - 4)
                    {
                        #ifdef __log_mv_new__
                            em=1;
                        #endif
                        uint8_t *uvbuf= s->edge_emu_buffer + 22 * linesize;

                        srcY -= 2 + 2*linesize;
                        s->dsp.emulated_edge_mc(s->edge_emu_buffer, srcY, linesize, 14,14,
                                            src_x - 2, src_y - 2, s->h_edge_pos, s->v_edge_pos);
                        srcY = s->edge_emu_buffer + 2 + 2*linesize;

                        s->dsp.emulated_edge_mc_nv12(uvbuf, srcU, uvlinesize, 5, 5,
                                            uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
                        srcU = uvbuf;
                    }
                    #ifdef __log_mv_new__
                    else
                        em=0;
                    #endif

                    Yy=Y+((k&0x1)<<3)+((k>>1)<<3)*linesize;
                    Uu=U+((k&0x1)<<3)+((k>>1)<<2)*uvlinesize;

                    #ifdef __log_mv_new__
                    log_text_p(log_fd_mv_new,"Forward",p_picdata->frame_cnt);
                    snprintf(lt,99,"mv_x=%d, mv_y=%d, uvmx=%d, uvmy=%d, lx=%d, ly=%d",mv_x,mv_y,uvmx,uvmy,lx,ly);
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    snprintf(lt,99,"des y offset=%d, des uv offset=%d",Yy-p_picdata->current_picture_ptr->data[0],Uu-p_picdata->current_picture_ptr->data[1]);
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    if(!em)
                        snprintf(lt,99,"src y offset=%d, src uv offset=%d",srcY-p_picdata->last_picture_ptr->data[0],srcU-p_picdata->last_picture_ptr->data[1]);
                    else
                        snprintf(lt,99,"use em buffer");
                    log_text_p(log_fd_mv_new,lt,p_picdata->frame_cnt);
                    #endif

                    s->dsp.put_rv40_qpel_pixels_tab[1][dxy](Yy,srcY,linesize);
                    s->dsp.put_rv40_chroma_pixels_tab_nv12[1](Uu, srcU, uvlinesize, 4, uvmx, uvmy);
                    s->dsp.put_rv40_chroma_pixels_tab_nv12[1](Uu+1, srcU+1, uvlinesize, 4, uvmx, uvmy);
                }

            }
            else
            {
                #ifdef _tmp_rv_dsp_setting_
                ambadec_assert_ffmpeg(((*pmbtype)&0xf)==0 || ((*pmbtype)&0xf)==is16x16);
                #endif
                continue;
            }

            #ifdef __dump_mc_dct_DSP_TXT__
                snprintf(txtdump, 99, "-----inter [MB] = [%d  %d]-----", j,i);
                log_text_p(log_fd_dsp_mc_dct_text,txtdump, p_picdata->frame_cnt);
                log_text_p(log_fd_dsp_mc_dct_text,"<<<<< total MC result Y: >>>>>", p_picdata->frame_cnt);
                log_text_rect_char_hex_p(log_fd_dsp_mc_dct_text,Y,16,16,linesize-16, p_picdata->frame_cnt);
                log_text_p(log_fd_dsp_mc_dct_text,"<<<<< total MC result U: >>>>>", p_picdata->frame_cnt);
                log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,U,8,8,uvlinesize-16, p_picdata->frame_cnt);
                log_text_p(log_fd_dsp_mc_dct_text,"<<<<< total MC result V: >>>>>", p_picdata->frame_cnt);
                log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,U+1,8,8,uvlinesize-16, p_picdata->frame_cnt);
            #endif

            if(thiz->neon.mb_addresidue != NULL){
                thiz->neon.mb_addresidue(Y,U,pdct,linesize);
            }else{
                _add_dct_result_mb(Y,U,pdct,linesize);
            }

            #ifdef __dump_mc_dct_DSP_TXT__
            log_text_p(log_fd_dsp_mc_dct_text,"<<<<< before deblock Y: >>>>>", p_picdata->frame_cnt);
            log_text_rect_char_hex_p(log_fd_dsp_mc_dct_text,Y,16,16,linesize-16, p_picdata->frame_cnt);
            log_text_p(log_fd_dsp_mc_dct_text,"<<<<< before deblock U: >>>>>", p_picdata->frame_cnt);
            log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,U,8,8,uvlinesize-16, p_picdata->frame_cnt);
            log_text_p(log_fd_dsp_mc_dct_text,"<<<<< before deblock V: >>>>>", p_picdata->frame_cnt);
            log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,U+1,8,8,uvlinesize-16, p_picdata->frame_cnt);
            #endif

        }
    }
}

#ifdef _tmp_rv_dsp_setting_
//process data
static void process_intrapred_rv34_amba(RV34AmbaDecContext *thiz,rv40_pic_data_t* p_picdata)
{
    RVDEC_INTRAPRED_t* pintrapred=p_picdata->pintra;
    uint8_t* pfmt;
    int* pmbtype=p_picdata->pmbtype;
    int cbp;
    int i,j,x,y;
//    MpegEncContext *s = &thiz->s;
    DCTELEM* pdct=p_picdata->pdct;
    uint8_t* Y, *UV;
    int linesize=p_picdata->current_picture_ptr->linesize[0];
    ambadec_assert_ffmpeg(linesize==p_picdata->current_picture_ptr->linesize[1]);

    #ifdef __log_intrapred__
        char tstr[40];
    #endif

    #ifdef __dump_mc_dct_DSP_TXT__
    char txtch[80];
    #endif

    #ifdef __dump_DSP_test_data__
    uint8_t logmbtype;
    #endif

    for(y=0;y<p_picdata->mb_height;y++)
    {
        for(x=0;x<p_picdata->mb_width;x++,pintrapred++,pdct+=384,pmbtype++)
        {
            if(pmbtype[0]&mbisinter)
            {
                #ifdef __dump_DSP_test_data__
                    logmbtype=2;
                    log_dump_p(log_fd_mbtype,&logmbtype,1,p_picdata->frame_cnt);
                #endif
                ambadec_assert_ffmpeg(!(pmbtype[0]&(~mbisinter)));
                continue;
            }

            #ifdef __dump_DSP_test_data__
                logmbtype=1;
                log_dump_p(log_fd_mbtype,&logmbtype,1,p_picdata->frame_cnt);
            #endif

            #ifdef __log_intrapred__
                snprintf(tstr,39,"MBintrapred_x=%d_y=%d ",x,y);
                log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
            #endif

            #ifdef __dump_mc_dct_DSP_TXT__
            snprintf(txtch,79,"-----intra [MB] = [%d  %d] -----",y,x);
            log_text_p(log_fd_dsp_mc_dct_text,txtch, p_picdata->frame_cnt);
            #endif

            Y = p_picdata->current_picture_ptr->data[0]+(x<<4) +(y<<4)*linesize;
            UV = p_picdata->current_picture_ptr->data[1] +  (x<<4) + (y<<3) *linesize;
            pfmt=pintrapred->intratype;

            //16x16
            if(pmbtype[0]&is16x16)
            {
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_intrapred])
                #endif
                {

                    #ifdef __dump_mc_dct_DSP_TXT__
                    log_text_p(log_fd_dsp_mc_dct_text,"<<<<< Luma 16x16 Intra Prediction : >>>>>", p_picdata->frame_cnt);
                    snprintf(txtch,79,"intra pred type=%d",((int)pfmt[0])&intratypemask);
                    log_text_p(log_fd_dsp_mc_dct_text,txtch, p_picdata->frame_cnt);
                    #endif

                    thiz->h.pred16x16[((int)pfmt[0])&intratypemask](Y, linesize);

                    #ifdef __dump_mc_dct_DSP_TXT__
                    log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred>>", p_picdata->frame_cnt);
                    log_text_rect_char_hex_p(log_fd_dsp_mc_dct_text,Y,16,16,linesize-16, p_picdata->frame_cnt);

                    log_text_p(log_fd_dsp_mc_dct_text,"<<<<< Chroma 8x8 Intra Prediction : >>>>>", p_picdata->frame_cnt);
                    snprintf(txtch,79,"intra pred type=%d",((int)pfmt[1])&intratypemask);
                    log_text_p(log_fd_dsp_mc_dct_text,txtch, p_picdata->frame_cnt);
                    #endif

                    thiz->h.pred8x8_nv12[((int)pfmt[1])&intratypemask](UV, linesize);

                    #ifdef __dump_mc_dct_DSP_TXT__
                    log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred U>>", p_picdata->frame_cnt);
                    log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV,8,8,linesize-16, p_picdata->frame_cnt);
                    log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred V>>", p_picdata->frame_cnt);
                    log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV+1,8,8,linesize-16, p_picdata->frame_cnt);
                    #endif

                }

                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_add_idct])
                #endif
                {
                    if(thiz->neon.mb_addresidue != NULL){
                       thiz->neon.mb_addresidue(Y,UV,pdct,linesize);
                    }else{
                       _add_dct_result_mb(Y,UV,pdct,linesize);
                    }
                }

                #ifdef __log_intrapred__
                    log_text_p(log_fd_intrapred, "y index=0 ",p_picdata->frame_cnt);
                    snprintf(tstr,39,"y type=%d, offset=%d ",((int)pfmt[0])&intratypemask,Y-p_picdata->current_picture_ptr->data[0]);
                    log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                    log_text_p(log_fd_intrapred, "uv index=1 ",p_picdata->frame_cnt);
                    snprintf(tstr,39,"uv type=%d, offset=%d ",((int)pfmt[1])&intratypemask,UV-p_picdata->current_picture_ptr->data[1]);
                    log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                #endif
            }
            else
            {
                uint32_t topleft32;
                uint64_t topleft64;

                cbp=pmbtype[0]>>4;

                #ifdef __dump_mc_dct_DSP_TXT__
                log_text_p(log_fd_dsp_mc_dct_text,"<<<<< Luma 4x4 Intra Prediction : >>>>>",p_picdata->frame_cnt);
                #endif

                //Y
                for(j=0;j<4;j++)
                {
                    for(i=0;i<4;i++,pfmt++,cbp>>=1)
                    {

                        #ifdef __dump_mc_dct_DSP_TXT__
                        snprintf(txtch,79,"{ SubBlock row=%d, col=%d : }",j,i);
                        log_text_p(log_fd_dsp_mc_dct_text,txtch,p_picdata->frame_cnt);
                        if((pfmt[0])&usetopleft) {
                            log_text_p(log_fd_dsp_mc_dct_text,"use top left",p_picdata->frame_cnt);
                        }
                        snprintf(txtch,79,"intra pred type=%d",pfmt[0]&intratypemask);
                        log_text_p(log_fd_dsp_mc_dct_text,txtch,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"y index=%d ",(j<<2)+i);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif
                        //use topleft
                        if((*pfmt)&usetopleft)
                        {
                            topleft32=Y[-linesize+3]*0x01010101;
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4[((int)pfmt[0])&intratypemask](Y, (uint8_t*)(&topleft32), linesize);
                            #ifdef __log_intrapred__
                                log_text_p(log_fd_intrapred, "y topleft ",p_picdata->frame_cnt);
                            #endif
                        }
                        else
                        {
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4[((int)pfmt[0])&intratypemask](Y, Y-linesize+4,linesize);
                        }

                        #ifdef __dump_mc_dct_DSP_TXT__
                        log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred>>", p_picdata->frame_cnt);
                        log_text_rect_char_hex_p(log_fd_dsp_mc_dct_text,Y,4,4,linesize-4, p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"y type=%d, offset=%d ",((int)pfmt[0])&intratypemask,Y-p_picdata->current_picture_ptr->data[0]);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_decoding_config__
                        if(log_mask[decoding_config_add_idct])
                        #endif
                        {
                            if(cbp&1)
                            {
                                if(thiz->neon.addresidue[addresidue_type_4x4y] != NULL){
                                   thiz->neon.addresidue[addresidue_type_4x4y](Y, pdct+(j<<6)+(i<<2),linesize);
                                }else{
                                   _add_dct_result_4x4_y(Y, pdct+(j<<6)+(i<<2),linesize);
                                }
                            }
                        }

                        #ifdef __dump_mc_dct_DSP_TXT__
                            log_text_p(log_fd_dsp_mc_dct_text,"<<after add reverse transform>>", p_picdata->frame_cnt);
                            log_text_rect_char_hex_p(log_fd_dsp_mc_dct_text,Y,4,4,linesize-4, p_picdata->frame_cnt);
                        #endif

                        Y+=4;
                    }
                    Y+= (linesize<<2) - 16;
                }

                #ifdef __dump_mc_dct_DSP_TXT__
                log_text_p(log_fd_dsp_mc_dct_text,"<<<<< Chroma 4x4 Intra Prediction : >>>>>", p_picdata->frame_cnt);
                #endif

                //UV
                for(j=0;j<2;j++)
                {
                    for(i=0;i<2;i++,pfmt++,cbp>>=1)
                    {

                        #ifdef __dump_mc_dct_DSP_TXT__
                        snprintf(txtch,79,"{ SubBlock row=%d, col=%d : }",j,i);
                        log_text_p(log_fd_dsp_mc_dct_text,txtch,p_picdata->frame_cnt);
                        if((pfmt[0])&usetopleft) {
                            log_text_p(log_fd_dsp_mc_dct_text,"use top left",p_picdata->frame_cnt);
                        }
                        snprintf(txtch,79,"intra pred type=%d",pfmt[0]&intratypemask);
                        log_text_p(log_fd_dsp_mc_dct_text,txtch,p_picdata->frame_cnt);
                        if((pfmt[0])&usetopleft) {
                            log_text_p(log_fd_dsp_mc_dct_text,"use top left",p_picdata->frame_cnt);
                        }
                        log_text_p(log_fd_dsp_mc_dct_text,txtch,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_intrapred__
                            char tstr[40];
                            snprintf(tstr,39,"uv index=%d ",j*2+i+16);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        //use topleft
                        if(((int)pfmt[0])&usetopleft)
                        {
                            #ifdef __log_intrapred__
                                log_text_p(log_fd_intrapred, "uv topleft ",p_picdata->frame_cnt);
                            #endif
                            topleft64=UV[-linesize+ 6] * 0x0001000100010001LL+UV[-linesize+ 7]*0x0100010001000100LL;
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4_nv12[((int)pfmt[0])&intratypemask]( UV, (uint8_t*) (&topleft64), linesize);
                        }
                        else
                        {
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4_nv12[((int)pfmt[0])&intratypemask](UV, UV-linesize+8, linesize);
                        }

                        #ifdef __dump_mc_dct_DSP_TXT__
                        log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred U>>",p_picdata->frame_cnt);
                        log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV,4,4,linesize-8,p_picdata->frame_cnt);
                        log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred V>>",p_picdata->frame_cnt);
                        log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV+1,4,4,linesize-8,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"uv type=%d, offset=%d ",((int)pfmt[0])&intratypemask,UV-p_picdata->current_picture_ptr->data[1]);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_decoding_config__
                        if(log_mask[decoding_config_add_idct])
                        #endif
                        {
                            if(cbp&0x1)
                            {
                                //#ifdef _config_rv40_neon_
                                //thiz->neon.uv4x4_addresidue(UV,pdct+256+(j<<5)+(i<<2),linesize);
                                //#else
                                //_add_dct_result_4x4_uv(UV,pdct+256+(j<<5)+(i<<2),linesize);
                                //#endif
                                _add_dct_result_4x4_chroma_nv12(UV,pdct+256+(j<<5)+(i<<2),linesize);
                            }
                            if(cbp&0x10)
                            {
                                //#ifdef _config_rv40_neon_
                                //thiz->neon.uv4x4_addresidue(UV+1,pdct+256+64+(j<<5)+(i<<2),linesize);
                                //#else
                                //_add_dct_result_4x4_uv(UV+1,pdct+256+64+(j<<5)+(i<<2),linesize);
                                //#endif
                                _add_dct_result_4x4_chroma_nv12(UV+1,pdct+256+64+(j<<5)+(i<<2),linesize);
                            }
                        }

                        #ifdef __dump_mc_dct_DSP_TXT__
                        log_text_p(log_fd_dsp_mc_dct_text,"<<after add reverse transform U>>",p_picdata->frame_cnt);
                        log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV,4,4,linesize-8,p_picdata->frame_cnt);
                        log_text_p(log_fd_dsp_mc_dct_text,"<<after add reverse transform V>>",p_picdata->frame_cnt);
                        log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV+1,4,4,linesize-8,p_picdata->frame_cnt);
                        #endif

                        UV+=8;
                    }
                    UV += (linesize<<2)-16;
                }
            }

        }
    }
}

static void process_intrapred_rv34_amba_i(RV34AmbaDecContext *thiz,rv40_pic_data_t* p_picdata,int start_row,int end_row)
{
    RVDEC_INTRAPRED_t* pintrapred=p_picdata->pintra+start_row*p_picdata->mb_width;
    uint8_t* pfmt;
    int* pmbtype=p_picdata->pmbtype+start_row*p_picdata->mb_width;

    int cbp;
    int i,j,x,y;
    uint8_t* Y, *UV;
    int linesize=p_picdata->current_picture_ptr->linesize[0];

    DCTELEM* pdct=p_picdata->pdct+start_row*p_picdata->mb_width*384;

    ambadec_assert_ffmpeg(linesize==p_picdata->current_picture_ptr->linesize[1]);

    #ifdef __log_intrapred__
        char tstr[40];
    #endif

    #ifdef __dump_mc_dct_DSP_TXT__
    char txtch[80];
    #endif

    for(y=start_row;y<end_row;y++)
    {
        for(x=0;x<p_picdata->mb_width;x++,pintrapred++,pdct+=384,pmbtype++)
        {
            Y = p_picdata->current_picture_ptr->data[0]+(x <<4) +(y<<4)*linesize;
            UV = p_picdata->current_picture_ptr->data[1] +  (x<<4) + (y<<3) *linesize;
            pfmt=pintrapred->intratype;

            #ifdef __log_intrapred__
                snprintf(tstr,39,"MBintrapred_x=%d_y=%d ",x,y);
                log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
            #endif

            #ifdef __dump_mc_dct_DSP_TXT__
            snprintf(txtch,79,"-----intra [MB] = [%d  %d] -----",y,x);
            log_text_p(log_fd_dsp_mc_dct_text,txtch, p_picdata->frame_cnt);
            #endif

            //16x16
            if(pmbtype[0]&is16x16)
            {
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_intrapred])
                #endif
                {

                    #ifdef __dump_mc_dct_DSP_TXT__
                    log_text_p(log_fd_dsp_mc_dct_text,"<<<<< Luma 16x16 Intra Prediction : >>>>>", p_picdata->frame_cnt);
                    snprintf(txtch,79,"intra pred type=%d",((int)pfmt[0])&intratypemask);
                    log_text_p(log_fd_dsp_mc_dct_text,txtch, p_picdata->frame_cnt);
                    #endif

                    thiz->h.pred16x16[((int)pfmt[0])&intratypemask](Y, linesize);

                    #ifdef __dump_mc_dct_DSP_TXT__
                    log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred>>", p_picdata->frame_cnt);
                    log_text_rect_char_hex_p(log_fd_dsp_mc_dct_text,Y,16,16,linesize-16, p_picdata->frame_cnt);

                    log_text_p(log_fd_dsp_mc_dct_text,"<<<<< Chroma 8x8 Intra Prediction : >>>>>", p_picdata->frame_cnt);
                    snprintf(txtch,79,"intra pred type=%d",((int)pfmt[1])&intratypemask);
                    log_text_p(log_fd_dsp_mc_dct_text,txtch, p_picdata->frame_cnt);
                    #endif

                    thiz->h.pred8x8_nv12[((int)pfmt[1])&intratypemask](UV, linesize);

                    #ifdef __dump_mc_dct_DSP_TXT__
                    log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred U>>", p_picdata->frame_cnt);
                    log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV,8,8,linesize-16, p_picdata->frame_cnt);
                    log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred V>>", p_picdata->frame_cnt);
                    log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV+1,8,8,linesize-16, p_picdata->frame_cnt);
                    #endif

                }

                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_add_idct])
                #endif
                {
                    if(thiz->neon.mb_addresidue != NULL){
                       thiz->neon.mb_addresidue(Y,UV,pdct,linesize);
                    }else{
                       _add_dct_result_mb(Y,UV,pdct,linesize);
                    }
                }

                #ifdef __log_intrapred__
                    log_text_p(log_fd_intrapred, "y index=0 ",p_picdata->frame_cnt);
                    snprintf(tstr,39,"y type=%d, offset=%d ",((int)pfmt[0])&intratypemask,Y-p_picdata->current_picture_ptr->data[0]);
                    log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                    log_text_p(log_fd_intrapred, "uv index=1 ",p_picdata->frame_cnt);
                    snprintf(tstr,39,"uv type=%d, offset=%d ",((int)pfmt[1])&intratypemask,UV-p_picdata->current_picture_ptr->data[1]);
                    log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                #endif
            }
            else
            {
                uint32_t topleft32;
                uint64_t topleft64;

                cbp=pmbtype[0]>>4;

                #ifdef __dump_mc_dct_DSP_TXT__
                log_text_p(log_fd_dsp_mc_dct_text,"<<<<< Luma 4x4 Intra Prediction : >>>>>",p_picdata->frame_cnt);
                #endif

                //Y
                for(j=0;j<4;j++)
                {
                    for(i=0;i<4;i++,pfmt++, cbp>>=1)
                    {
                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"y index=%d ",(j<<2)+i);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        #ifdef __dump_mc_dct_DSP_TXT__
                        snprintf(txtch,79,"{ SubBlock row=%d, col=%d : }",j,i);
                        log_text_p(log_fd_dsp_mc_dct_text,txtch,p_picdata->frame_cnt);
                        if((pfmt[0])&usetopleft) {
                            log_text_p(log_fd_dsp_mc_dct_text,"use top left",p_picdata->frame_cnt);
                        }
                        snprintf(txtch,79,"intra pred type=%d",pfmt[0]&intratypemask);
                        log_text_p(log_fd_dsp_mc_dct_text,txtch,p_picdata->frame_cnt);
                        #endif

                        //use topleft
                        if(pfmt[0]&usetopleft)
                        {
                            topleft32=Y[-linesize+3]*0x01010101;
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4[((int)pfmt[0])&intratypemask](Y, (uint8_t*)(&topleft32), linesize);
                            #ifdef __log_intrapred__
                                log_text_p(log_fd_intrapred, "y topleft ",p_picdata->frame_cnt);
                            #endif
                        }
                        else
                        {
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4[((int)pfmt[0])&intratypemask](Y, Y-linesize+4,linesize);
                        }

                        #ifdef __dump_mc_dct_DSP_TXT__
                        log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred>>", p_picdata->frame_cnt);
                        log_text_rect_char_hex_p(log_fd_dsp_mc_dct_text,Y,4,4,linesize-4, p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"y type=%d, offset=%d ",((int)pfmt[0])&intratypemask,Y-p_picdata->current_picture_ptr->data[0]);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_decoding_config__
                        if(log_mask[decoding_config_add_idct])
                        #endif
                        {
                            if(cbp&0x1)
                            {
                                if(thiz->neon.addresidue[addresidue_type_4x4y] != NULL){
                                   thiz->neon.addresidue[addresidue_type_4x4y](Y, pdct+(j<<6)+(i<<2),linesize);
                                }else{
                                   _add_dct_result_4x4_y(Y, pdct+(j<<6)+(i<<2),linesize);
                                }
                            }
                        }

                        #ifdef __dump_mc_dct_DSP_TXT__
                            log_text_p(log_fd_dsp_mc_dct_text,"<<after add reverse transform>>", p_picdata->frame_cnt);
                            log_text_rect_char_hex_p(log_fd_dsp_mc_dct_text,Y,4,4,linesize-4, p_picdata->frame_cnt);
                        #endif

                        Y+=4;
                    }
                    Y+= (linesize<<2) - 16;
                }

                #ifdef __dump_mc_dct_DSP_TXT__
                log_text_p(log_fd_dsp_mc_dct_text,"<<<<< Chroma 4x4 Intra Prediction : >>>>>", p_picdata->frame_cnt);
                #endif

                //UV
                for(j=0;j<2;j++)
                {
                    for(i=0;i<2;i++,pfmt++,cbp>>=1)
                    {

                        #ifdef __dump_mc_dct_DSP_TXT__
                        snprintf(txtch,79,"{ SubBlock row=%d, col=%d : }",j,i);
                        log_text_p(log_fd_dsp_mc_dct_text,txtch,p_picdata->frame_cnt);
                        if((pfmt[0])&usetopleft) {
                            log_text_p(log_fd_dsp_mc_dct_text,"use top left",p_picdata->frame_cnt);
                        }
                        snprintf(txtch,79,"intra pred type=%d",pfmt[0]&intratypemask);
                        log_text_p(log_fd_dsp_mc_dct_text,txtch,p_picdata->frame_cnt);
                        if((pfmt[0])&usetopleft) {
                            log_text_p(log_fd_dsp_mc_dct_text,"use top left",p_picdata->frame_cnt);
                        }
                        log_text_p(log_fd_dsp_mc_dct_text,txtch,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"uv index=%d ",j*2+i+16);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        //use topleft
                        if(pfmt[0]&usetopleft)
                        {
                            #ifdef __log_intrapred__
                                log_text_p(log_fd_intrapred, "uv topleft ",p_picdata->frame_cnt);
                            #endif
                            topleft64=UV[-linesize+ 6] * 0x0001000100010001LL+UV[-linesize+ 7]*0x0100010001000100LL;
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4_nv12[((int)pfmt[0])&intratypemask]( UV, (uint8_t*) (&topleft64), linesize);
                        }
                        else
                        {
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4_nv12[((int)pfmt[0])&intratypemask](UV, UV-linesize+8, linesize);
                        }

                        #ifdef __dump_mc_dct_DSP_TXT__
                        log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred U>>",p_picdata->frame_cnt);
                        log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV,4,4,linesize-8,p_picdata->frame_cnt);
                        log_text_p(log_fd_dsp_mc_dct_text,"<<after intra pred V>>",p_picdata->frame_cnt);
                        log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV+1,4,4,linesize-8,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"uv type=%d, offset=%d ",((int)pfmt[0])&intratypemask,UV-p_picdata->current_picture_ptr->data[1]);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_decoding_config__
                        if(log_mask[decoding_config_add_idct])
                        #endif
                        {
                            if(cbp&0x1)
                            {
                                //#ifdef _config_rv40_neon_
                                //thiz->neon.uv4x4_addresidue(UV,pdct+256+(j<<5)+(i<<2),linesize);
                                //#else
                                //_add_dct_result_4x4_uv(UV,pdct+256+(j<<5)+(i<<2),linesize);
                                //#endif
                                _add_dct_result_4x4_chroma_nv12(UV,pdct+256+(j<<5)+(i<<2),linesize);
                            }
                            if(cbp&0x10)
                            {
                                //#ifdef _config_rv40_neon_
                                //thiz->neon.uv4x4_addresidue(UV+1,pdct+256+64+(j<<5)+(i<<2),linesize);
                                //#else
                                //_add_dct_result_4x4_uv(UV+1,pdct+256+64+(j<<5)+(i<<2),linesize);
                                //#endif
                                _add_dct_result_4x4_chroma_nv12(UV+1,pdct+256+64+(j<<5)+(i<<2),linesize);
                            }
                        }

                        #ifdef __dump_mc_dct_DSP_TXT__
                        log_text_p(log_fd_dsp_mc_dct_text,"<<after add reverse transform U>>",p_picdata->frame_cnt);
                        log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV,4,4,linesize-8,p_picdata->frame_cnt);
                        log_text_p(log_fd_dsp_mc_dct_text,"<<after add reverse transform V>>",p_picdata->frame_cnt);
                        log_text_rect_char_hex_chroma_p(log_fd_dsp_mc_dct_text,UV+1,4,4,linesize-8,p_picdata->frame_cnt);
                        #endif

                        UV+=8;
                    }
                    UV += (linesize<<2)-16;
                }
            }

        }
    }
}
#else

//process data
static void process_intrapred_rv34_amba(RV34AmbaDecContext *thiz,rv40_pic_data_t* p_picdata)
{
    RVDEC_INTRAPRED_t* pintrapred=(RVDEC_INTRAPRED_t*)p_picdata->pinfo;
    RVDEC_INTRAPRED_FMT_t* pfmt;
    RVDEC_MBMV_B_t* pb;

    int i,j,x,y;
    MpegEncContext *s = &thiz->s;
    DCTELEM* pdct=p_picdata->pdct;
    uint8_t* Y, *UV;
    int linesize=p_picdata->current_picture_ptr->linesize[0];
    ambadec_assert_ffmpeg(linesize==p_picdata->current_picture_ptr->linesize[1]);

    #ifdef __log_intrapred__
        char tstr[40];
    #endif

    for(y=0;y<p_picdata->mb_height;y++)
    {
        for(x=0;x<p_picdata->mb_width;x++,pintrapred++,pdct+=384)
        {

            //exclude inter MB
            pb=(RVDEC_MBMV_B_t*)pintrapred;
            if(pb->mv[0].fwd.valid || pb->mv[0].bwd.valid)
                continue;

            #ifdef __log_intrapred__
                snprintf(tstr,39,"MBintrapred_x=%d_y=%d ",x,y);
                log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
            #endif

            Y = p_picdata->current_picture_ptr->data[0]+(x<<4) +(y<<4)*linesize;
            UV = p_picdata->current_picture_ptr->data[1] +  (x<<4) + (y<<3) *linesize;
            pfmt=pintrapred->intratype;

            //16x16
            if(pfmt[0].is16x16)
            {
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_intrapred])
                #endif
                {
                    thiz->h.pred16x16[pfmt[0].intratype](Y, linesize);
                    thiz->h.pred8x8_nv12[pfmt[1].intratype](UV, linesize);
                }

                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_add_idct])
                #endif
                //_add_dct_result_mb(Y,UV,pdct,linesize);
                if(thiz->neon.mb_addresidue != NULL){
                    thiz->neon.mb_addresidue(Y,UV,pdct,linesize);
                }else{
                    _add_dct_result_mb(Y,UV,pdct,linesize);
                }

                #ifdef __log_intrapred__
                    log_text_p(log_fd_intrapred, "y index=0 ",p_picdata->frame_cnt);
                    snprintf(tstr,39,"y type=%d, offset=%d ",pfmt[0].intratype,Y-p_picdata->current_picture_ptr->data[0]);
                    log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                    log_text_p(log_fd_intrapred, "uv index=1 ",p_picdata->frame_cnt);
                    snprintf(tstr,39,"uv type=%d, offset=%d ",pfmt[1].intratype,UV-p_picdata->current_picture_ptr->data[1]);
                    log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                #endif
            }
            else
            {
                uint32_t topleft32;
                uint64_t topleft64;

                //Y
                for(j=0;j<4;j++)
                {
                    for(i=0;i<4;i++,pfmt++)
                    {
                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"y index=%d ",(j<<2)+i);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif
                        //use topleft
                        if(pfmt->use_topleft)
                        {
                            topleft32=Y[-linesize+3]*0x01010101;
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4[pfmt->intratype](Y, (uint8_t*)(&topleft32), linesize);
                            #ifdef __log_intrapred__
                                log_text_p(log_fd_intrapred, "y topleft ",p_picdata->frame_cnt);
                            #endif
                        }
                        else
                        {
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4[pfmt->intratype](Y, Y-linesize+4,linesize);
                        }

                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"y type=%d, offset=%d ",pfmt->intratype,Y-p_picdata->current_picture_ptr->data[0]);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_decoding_config__
                        if(log_mask[decoding_config_add_idct])
                        #endif
                        //_add_dct_result_4x4_y(Y, pdct+(j<<6)+(i<<2),linesize);
                        if(thiz->neon.addresidue[addresidue_type_4x4y] != NULL){
                            thiz->neon.addresidue[addresidue_type_4x4y](Y, pdct+(j<<6)+(i<<2),linesize);
                        }else{
                            _add_dct_result_4x4_y(Y, pdct+(j<<6)+(i<<2),linesize);
                        }

                        Y+=4;
                    }
                    Y+= (linesize<<2) - 16;
                }

                //UV
                for(j=0;j<2;j++)
                {
                    for(i=0;i<2;i++,pfmt++)
                    {
                        #ifdef __log_intrapred__
                            char tstr[40];
                            snprintf(tstr,39,"uv index=%d ",j*2+i+16);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        //use topleft
                        if(pfmt->use_topleft)
                        {
                            #ifdef __log_intrapred__
                                log_text_p(log_fd_intrapred, "uv topleft ",p_picdata->frame_cnt);
                            #endif
                            topleft64=UV[-linesize+ 6] * 0x0001000100010001+UV[-linesize+ 7]*0x0100010001000100;
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4_nv12[pfmt->intratype]( UV, (uint8_t*) (&topleft64), linesize);
                        }
                        else
                        {
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4_nv12[pfmt->intratype](UV, UV-linesize+8, linesize);
                        }

                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"uv type=%d, offset=%d ",pfmt->intratype,UV-p_picdata->current_picture_ptr->data[1]);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_decoding_config__
                        if(log_mask[decoding_config_add_idct])
                        #endif
                        //_add_dct_result_4x4_uv(UV,pdct+256+(j<<5)+(i<<2),linesize);
                        if(thiz->neon.addresidue[addresidue_type_4x4uv] != NULL){
                            thiz->neon.addresidue[addresidue_type_4x4uv](UV,pdct+256+(j<<5)+(i<<2),linesize);
                        }else{
                            _add_dct_result_4x4_uv(UV,pdct+256+(j<<5)+(i<<2),linesize);
                        }
                        UV+=8;
                    }
                    UV += (linesize<<2)-16;
                }
            }

        }
    }
}

static void process_intrapred_rv34_amba_i(RV34AmbaDecContext *thiz,rv40_pic_data_t* p_picdata,int start_row,int end_row)
{
    RVDEC_INTRAPRED_t* pintrapred=(RVDEC_INTRAPRED_t*)(p_picdata->pinfo+start_row*p_picdata->mb_width);;
    RVDEC_INTRAPRED_FMT_t* pfmt;

    int i,j,x,y;
    uint8_t* Y, *UV;
    int linesize=p_picdata->current_picture_ptr->linesize[0];

    DCTELEM* pdct=p_picdata->pdct+start_row*p_picdata->mb_width*384;

    ambadec_assert_ffmpeg(linesize==p_picdata->current_picture_ptr->linesize[1]);

    #ifdef __log_intrapred__
        char tstr[40];
    #endif

    for(y=start_row;y<end_row;y++)
    {
        for(x=0;x<p_picdata->mb_width;x++,pintrapred++,pdct+=384)
        {
            Y = p_picdata->current_picture_ptr->data[0]+(x <<4) +(y<<4)*linesize;
            UV = p_picdata->current_picture_ptr->data[1] +  (x<<4) + (y<<3) *linesize;
            pfmt=pintrapred->intratype;

            #ifdef __log_intrapred__
                snprintf(tstr,39,"MBintrapred_x=%d_y=%d ",x,y);
                log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
            #endif

            //16x16
            if(pfmt[0].is16x16)
            {
                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_intrapred])
                #endif
                {
                    thiz->h.pred16x16[pfmt[0].intratype](Y, linesize);
                    thiz->h.pred8x8_nv12[pfmt[1].intratype](UV, linesize);
                }

                #ifdef __log_decoding_config__
                if(log_mask[decoding_config_add_idct])
                #endif
                //_add_dct_result_mb(Y,UV,pdct,linesize);
                if(thiz->neon.mb_addresidue != NULL){
                    thiz->neon.mb_addresidue(Y,UV,pdct,linesize);
                }else{
                    _add_dct_result_mb(Y,UV,pdct,linesize);
                }

                #ifdef __log_intrapred__
                    log_text_p(log_fd_intrapred, "y index=0 ",p_picdata->frame_cnt);
                    snprintf(tstr,39,"y type=%d, offset=%d ",pfmt[0].intratype,Y-p_picdata->current_picture_ptr->data[0]);
                    log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                    log_text_p(log_fd_intrapred, "uv index=1 ",p_picdata->frame_cnt);
                    snprintf(tstr,39,"uv type=%d, offset=%d ",pfmt[1].intratype,UV-p_picdata->current_picture_ptr->data[1]);
                    log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                #endif
            }
            else
            {
                uint32_t topleft32;
                uint64_t topleft64;

                //Y
                for(j=0;j<4;j++)
                {
                    for(i=0;i<4;i++,pfmt++)
                    {
                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"y index=%d ",(j<<2)+i);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        //use topleft
                        if(pfmt->use_topleft)
                        {
                            topleft32=Y[-linesize+3]*0x01010101;
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4[pfmt->intratype](Y, (uint8_t*)(&topleft32), linesize);
                            #ifdef __log_intrapred__
                                log_text_p(log_fd_intrapred, "y topleft ",p_picdata->frame_cnt);
                            #endif
                        }
                        else
                        {
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4[pfmt->intratype](Y, Y-linesize+4,linesize);
                        }

                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"y type=%d, offset=%d ",pfmt->intratype,Y-p_picdata->current_picture_ptr->data[0]);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_decoding_config__
                        if(log_mask[decoding_config_add_idct])
                        #endif
                        //_add_dct_result_4x4_y(Y, pdct+(j<<6)+(i<<2),linesize);
                        if(thiz->neon.addresidue[addresidue_type_4x4y] != NULL){
                            thiz->neon.addresidue[addresidue_type_4x4y](Y, pdct+(j<<6)+(i<<2),linesize);
                        }else{
                            _add_dct_result_4x4_y(Y, pdct+(j<<6)+(i<<2),linesize);
                        }

                        Y+=4;
                    }
                    Y+= (linesize<<2) - 16;
                }

                //UV
                for(j=0;j<2;j++)
                {
                    for(i=0;i<2;i++,pfmt++)
                    {
                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"uv index=%d ",j*2+i+16);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        //use topleft
                        if(pfmt->use_topleft)
                        {
                            #ifdef __log_intrapred__
                                log_text_p(log_fd_intrapred, "uv topleft ",p_picdata->frame_cnt);
                            #endif
                            topleft64=UV[-linesize+ 6] * 0x0001000100010001+UV[-linesize+ 7]*0x0100010001000100;
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4_nv12[pfmt->intratype]( UV, (uint8_t*) (&topleft64), linesize);
                        }
                        else
                        {
                            #ifdef __log_decoding_config__
                            if(log_mask[decoding_config_intrapred])
                            #endif
                            thiz->h.pred4x4_nv12[pfmt->intratype](UV, UV-linesize+8, linesize);
                        }

                        #ifdef __log_intrapred__
                            snprintf(tstr,39,"uv type=%d, offset=%d ",pfmt->intratype,UV-p_picdata->current_picture_ptr->data[1]);
                            log_text_p(log_fd_intrapred, tstr,p_picdata->frame_cnt);
                        #endif

                        #ifdef __log_decoding_config__
                        if(log_mask[decoding_config_add_idct])
                        #endif
                        //_add_dct_result_4x4_uv(UV,pdct+256+(j<<5)+(i<<2),linesize);
                        if(thiz->neon.addresidue[addresidue_type_4x4uv] != NULL){
                            thiz->neon.addresidue[addresidue_type_4x4uv](UV,pdct+256+(j<<5)+(i<<2),linesize);
                        }else{
                            _add_dct_result_4x4_uv(UV,pdct+256+(j<<5)+(i<<2),linesize);
                        }
                        UV+=8;
                    }
                    UV += (linesize<<2)-16;
                }
            }

        }
    }
}
#endif


#if 0
#ifdef CONFIG_AMBA_RV40_MC_ACCELERATOR
static int init_amba_rv40_mc_accelerator (RV34AmbaDecContext* thiz, int max_width, int max_height)
{
    av_log(NULL, AV_LOG_ERROR, "init_amba_rv40_mc_accelerator, width %d, height %d.\n", max_width, max_height);

    thiz->amba_iav_fd = open("/dev/iav", O_RDWR, 0);
    if (thiz->amba_iav_fd < 0) {
        av_log(NULL, AV_LOG_ERROR, "iav open fail in init_amba_rv40_mc_accelerator.\n");
        return -1;
    }

    thiz->amba_buffer_number = 2;

    ambadec_assert_ffmpeg(max_width>0);
    ambadec_assert_ffmpeg(max_height>0);
    ambadec_assert_ffmpeg(!(max_width&(~15)));
    ambadec_assert_ffmpeg(!(max_height&(~15)));

    //for safe
    thiz->amba_buffer_width = (max_width+15)&(~15);
    thiz->amba_buffer_height = (max_height+15)&(~15);

    thiz->amba_residual_buffer_size = thiz->amba_buffer_width*thiz->amba_buffer_height*sizeof(short);
    thiz->amba_mv_buffer_size = (thiz->amba_buffer_width>>4)*(thiz->amba_buffer_height>>4)*sizeof(RVDEC_MBMV_B_t);

    iav_mmap_info_t info;
    //request ring buffer size
    info.length = (thiz->amba_residual_buffer_size + thiz->amba_mv_buffer_size) * thiz->amba_buffer_number;
    if (ioctl(thiz->amba_iav_fd, IAV_IOC_MAP_DECODE_BSB, &info) < 0) {
        av_log(NULL, AV_LOG_ERROR, "IAV_IOC_MAP_DECODE_BSB fail in init_amba_rv40_mc_accelerator.\n");
        return -2;
    }

    //hack here, 2 ring buffer put together, idct buffer is first, mv buffer is second
    thiz->p_amba_idct_ringbuffer = (unsigned char*)info.addr;
    thiz->p_amba_mv_ringbuffer = thiz->p_amba_idct_ringbuffer + (thiz->amba_residual_buffer_size * thiz->amba_buffer_number);

    iav_config_decoder_t config;
    //config rv40-mc decoder
    config.flags = 0;
    config.decoder_type = IAV_SIMPLE_DECODER;
    config.pic_width = thiz->amba_buffer_width;
    config.pic_height = thiz->amba_buffer_height;
    if (ioctl(thiz->amba_iav_fd, IAV_IOC_CONFIG_DECODER, &config) < 0) {
        av_log(NULL, AV_LOG_ERROR, "IAV_IOC_CONFIG_DECODER fail in init_amba_rv40_mc_accelerator.\n");
        return -3;
    }

    //success, set for amba dsp
    thiz->use_dsp = 1;
    thiz->use_permutated = 1;
    return 0;
}

static void end_amba_mpeg4_idctmc_accelerator (RV34AmbaDecContext* thiz)
{
    ambadec_assert_ffmpeg(thiz->use_dsp);
    ambadec_assert_ffmpeg(thiz->use_permutated == 1);

    if (ioctl(thiz->amba_iav_fd, IAV_IOC_UNMAP_DECODE_BSB, 0) < 0) {
        av_log(NULL, AV_LOG_ERROR, "IAV_IOC_UNMAP_DECODE_BSB fail in end_amba_mpeg4_idctmc_accelerator.\n");
    }

    thiz->use_dsp = 0;
    thiz->use_permutated = 0;
    close(thiz->amba_iav_fd);
    return;
}

static void amba_mpeg4_idctmc_processing (int iav_fd, h263_pic_data_t* p_pic)
{
    p_pic->vopinfo.decode_id = 0;//should not be specified here
    p_pic->vopinfo.decode_type = 4;//mpeg4-sw
    p_pic->vopinfo.num_of_pics = 1;
    p_pic->vopinfo.vop_coef_daddr = p_pic->pdct;
    p_pic->vopinfo.vop_mv_daddr = p_pic->pmvb;
    //other fields are filled before

    ioctl(iav_fd, IAV_IOC_DECODING, &p_pic->vopinfo);
    return;
}
#endif
#endif

#ifdef __dump_DSP_test_data__

static void _dump_residue_only_interMB(rv40_pic_data_t* p_picdata)
{
    int* pmbtype=p_picdata->pmbtype;
    int num = p_picdata->mb_height*p_picdata->mb_width;
    uint8_t* p = (uint8_t*)p_picdata->pdct;
    uint8_t zero[768]={0};

    while (num>0) {
        if (!((*pmbtype)&mbisinter)) {
            log_dump_p(log_fd_residual_2,zero,768,p_picdata->frame_cnt);
        } else {
            log_dump_p(log_fd_residual_2,p,768,p_picdata->frame_cnt);
        }
        p+=768;
        pmbtype++;
        num --;
    }
}

typedef struct _uv_s
{
    uint8_t u[8][8];
    uint8_t v[8][8];
}_uv_t;

static void interleave_uv(_uv_t* puv, uint8_t* p, int stride)
{
    int i,j;
    for (j=0; j<8; j++)
    {
        for(i=0; i<8; i++) {
            puv->u[j][i] = p[i*2];
            puv->v[j][i] = p[i*2+1];
        }
        p+=stride;
    }
}

static void _dump_recon_tilted_result(rv40_pic_data_t* p_picdata)
{
    int x, y;
    int i;
    _uv_t uv;
    uint8_t* py, * puv;

    for (y=0; y<p_picdata->mb_height; y++) {
        ambadec_assert_ffmpeg(p_picdata->current_picture_ptr->linesize[0] == p_picdata->current_picture_ptr->linesize[1]);
        for (x=0; x<p_picdata->mb_width; x++) {
            py = p_picdata->current_picture_ptr->data[0] + (y*16)*p_picdata->current_picture_ptr->linesize[0] + x*16;
            puv = p_picdata->current_picture_ptr->data[1] + (y*8)*p_picdata->current_picture_ptr->linesize[1] + x*16;
            //dump y
            for (i = 0; i<16; i++, py+=p_picdata->current_picture_ptr->linesize[0]) {
                log_dump_p(log_fd_result, py,16,p_picdata->frame_cnt);
            }

            //dump uv
            interleave_uv(&uv, puv, p_picdata->current_picture_ptr->linesize[1]);
            log_dump_p(log_fd_result, &uv,128,p_picdata->frame_cnt);
        }
    }
}

//new
static void _dump_new_mc_result(rv40_pic_data_t* p_picdata)
{
    int x, y;
    int i;
    _uv_t uv;
    uint8_t* py, * puv;

    for (y=0; y<p_picdata->mb_height; y++) {
        ambadec_assert_ffmpeg(p_picdata->current_picture_ptr->linesize[0] == p_picdata->current_picture_ptr->linesize[1]);
        //dump first half y
        for (x=0; x<p_picdata->mb_width; x++) {
            py = p_picdata->current_picture_ptr->data[0] + (y*16)*p_picdata->current_picture_ptr->linesize[0] + x*16;
            for (i = 0; i<8; i++, py+=p_picdata->current_picture_ptr->linesize[0]) {
                log_dump_p(log_fd_result, py,16,p_picdata->frame_cnt);
            }
        }
        //dump second half y
        for (x=0; x<p_picdata->mb_width; x++) {
            py = p_picdata->current_picture_ptr->data[0] + (y*16 + 8)*p_picdata->current_picture_ptr->linesize[0] + x*16;
            for (i = 0; i<8; i++, py+=p_picdata->current_picture_ptr->linesize[0]) {
                log_dump_p(log_fd_result, py,16,p_picdata->frame_cnt);
            }
        }

        for (x=0; x<p_picdata->mb_width; x++) {
            puv = p_picdata->current_picture_ptr->data[1] + (y*8)*p_picdata->current_picture_ptr->linesize[1] + x*16;
            //dump uv
            interleave_uv(&uv, puv, p_picdata->current_picture_ptr->linesize[1]);
            log_dump_p(log_fd_result, &uv,128,p_picdata->frame_cnt);
        }

    }
}

static void _dump_picture_data(rv40_pic_data_t* p_picdata, int is_b)
{
    int itt=0; uint8_t* ptt;

    Picture* pPic = NULL;
    if(is_b) {
        pPic = p_picdata->current_picture_ptr;
    } else {
        pPic = p_picdata->last_picture_ptr;
    }

    ambadec_assert_ffmpeg(pPic);

    if (pPic) {
        ambadec_assert_ffmpeg(pPic->linesize[0] == pPic->linesize[1]);
        //ambadec_assert_ffmpeg(pPic->linesize[0] == p_picdata->width);
        log_openfile_p(log_fd_temp_2, "pic", p_picdata->frame_cnt);
        ptt=pPic->data[0];
        for(itt=0;itt<p_picdata->height;itt++,ptt+=pPic->linesize[0])
            log_dump_p(log_fd_temp_2,ptt,p_picdata->width,p_picdata->frame_cnt);

        ptt=pPic->data[1];
        for(itt=0;(itt<p_picdata->height/2);itt++,ptt+=pPic->linesize[1])
            log_dump_p(log_fd_temp_2,ptt,p_picdata->width,p_picdata->frame_cnt);

        log_closefile_p(log_fd_temp_2,p_picdata->frame_cnt);
    }

}

static void _dump_input_for_dsp(rv40_pic_data_t* p_picdata)
{
    int itt=0; uint8_t* ptt;
#ifdef _d_use_new_residue_layout_
    change_residue_layout(p_picdata->pdct, p_picdata->mb_width, p_picdata->mb_height);
    p_picdata->new_residue_layout = 1;
    log_dump_p(log_fd_residual,(p_picdata->pdct - p_picdata->mb_width*8*8*6),p_picdata->mb_height*p_picdata->mb_width*768,p_picdata->frame_cnt);

//debug only
#ifdef _d_debug_new_layout_
    store_residue_layout_back(p_picdata->pdct, p_picdata->mb_width, p_picdata->mb_height);
    p_picdata->new_residue_layout = 0;
    log_dump_p(log_fd_residual_3,p_picdata->pdct,p_picdata->mb_height*p_picdata->mb_width*768,p_picdata->frame_cnt);
#endif

#else
    _dump_residue_only_interMB(p_picdata);
    log_dump_p(log_fd_residual,p_picdata->pdct,p_picdata->mb_height*p_picdata->mb_width*768,p_picdata->frame_cnt);
#endif
    log_dump_p(log_fd_mvdsp,p_picdata->pinfo,p_picdata->mb_height*p_picdata->mb_width*sizeof(RVDEC_MBINFO_t),p_picdata->frame_cnt);


    //clear current picture for testing
    memset(p_picdata->current_picture_ptr->data[0], 0x0, p_picdata->height*p_picdata->current_picture_ptr->linesize[0]);
    memset(p_picdata->current_picture_ptr->data[1], 0x0, (p_picdata->height/2)*p_picdata->current_picture_ptr->linesize[1]);

    if(p_picdata->last_picture_ptr)
    {
        ptt=p_picdata->last_picture_ptr->data[0];
        for(itt=0;itt<p_picdata->height;itt++,ptt+=p_picdata->last_picture_ptr->linesize[0])
            log_dump_p(log_fd_reffyraw,ptt,p_picdata->width,p_picdata->frame_cnt);

        ptt=p_picdata->last_picture_ptr->data[1];
        for(itt=0;(itt<p_picdata->height/2);itt++,ptt+=p_picdata->last_picture_ptr->linesize[1])
            log_dump_p(log_fd_reffuvraw,ptt,p_picdata->width,p_picdata->frame_cnt);
    }

    if(p_picdata->next_picture_ptr)
    {
        ptt=p_picdata->next_picture_ptr->data[0];
        for(itt=0;itt<p_picdata->height;itt++,ptt+=p_picdata->next_picture_ptr->linesize[0])
            log_dump_p(log_fd_refbyraw,ptt,p_picdata->width,p_picdata->frame_cnt);

        ptt=p_picdata->next_picture_ptr->data[1];
        for(itt=0;(itt<p_picdata->height/2);itt++,ptt+=p_picdata->next_picture_ptr->linesize[1])
            log_dump_p(log_fd_refbuvraw,ptt,p_picdata->width,p_picdata->frame_cnt);
    }

#ifndef __dump_whole__
#ifdef __dump_binary__
    log_closefile_p(log_fd_residual_3,p_picdata->frame_cnt);
    log_closefile_p(log_fd_residual_2,p_picdata->frame_cnt);
    log_closefile_p(log_fd_residual,p_picdata->frame_cnt);
    log_closefile_p(log_fd_mvdsp,p_picdata->frame_cnt);
    log_closefile_p(log_fd_reffyraw,p_picdata->frame_cnt);
    log_closefile_p(log_fd_reffuvraw,p_picdata->frame_cnt);
    //log_closefile_p(log_fd_reffytiled,p_picdata->frame_cnt);
    //log_closefile_p(log_fd_reffuvtiled,p_picdata->frame_cnt);
    log_closefile_p(log_fd_refbyraw,p_picdata->frame_cnt);
    log_closefile_p(log_fd_refbuvraw,p_picdata->frame_cnt);
    //log_closefile_p(log_fd_refbytiled,p_picdata->frame_cnt);
    //log_closefile_p(log_fd_refbuvtiled,p_picdata->frame_cnt);
    //log_closefile_p(log_fd_result,p_picdata->frame_cnt);
#endif
#endif

}

static void _dump_result_for_dsp(rv40_pic_data_t* p_picdata)
{
    int itt=0; uint8_t* ptt;
    //log_openfile_p(log_fd_result_2,"rv_result_y_rasterscaned",p_picdata->frame_cnt);
    ptt=p_picdata->current_picture_ptr->data[0];
    for(itt=0;itt<p_picdata->height;itt++,ptt+=p_picdata->current_picture_ptr->linesize[0])
        log_dump_p(log_fd_result_2,ptt,p_picdata->width,p_picdata->frame_cnt);
    //log_closefile_p(log_fd_result_2,p_picdata->frame_cnt);

    //log_openfile_p(log_fd_result_3,"rv_result_uv_rasterscaned",p_picdata->frame_cnt);
    ptt=p_picdata->current_picture_ptr->data[1];
    for(itt=0;(itt<p_picdata->height/2);itt++,ptt+=p_picdata->current_picture_ptr->linesize[1])
        log_dump_p(log_fd_result_3,ptt,p_picdata->width,p_picdata->frame_cnt);
    //log_closefile_p(log_fd_result_3,p_picdata->frame_cnt);

    _dump_new_mc_result(p_picdata);
    //_dump_recon_tilted_result(p_picdata);

#ifndef __dump_whole__
#ifdef __dump_binary__
    log_closefile_p(log_fd_result,p_picdata->frame_cnt);
    log_closefile_p(log_fd_result_2,p_picdata->frame_cnt);
    log_closefile_p(log_fd_result_3,p_picdata->frame_cnt);
#endif
#endif
}
#endif

#ifdef _d_use_new_residue_layout_
//optimize
void change_residue_layout(short* psrc, int mb_width, int mb_height)
{
    int i, mbline;
    short* pdes_y, *pdes_y2, *pdes_uv;
    int copy_size;

    copy_size = mb_width*8*8*2;
    pdes_y = psrc - copy_size*3;
    pdes_y2 = pdes_y + copy_size;
    pdes_uv = pdes_y + 2*copy_size;

    for (mbline = 0; mbline <mb_height; mbline ++) {

        for (i = 0; i < mb_width; i++) {
            memcpy(pdes_y, psrc, 8*8*2*2);
            psrc += 8*8*2;
            pdes_y += 8*8*2;
            memcpy(pdes_y2, psrc, 8*8*2*2);
            psrc += 8*8*2;
            pdes_y2 += 8*8*2;
            memcpy(pdes_uv, psrc, 8*8*2*2);
            psrc += 8*8*2;
            pdes_uv += 8*8*2;
        }
        pdes_y += 2*copy_size;
        pdes_y2 += 2*copy_size;
        pdes_uv += 2*copy_size;

        ambadec_assert_ffmpeg(psrc == (pdes_y + 3*copy_size));
        ambadec_assert_ffmpeg(psrc == (pdes_y2 + 2*copy_size));
        ambadec_assert_ffmpeg(psrc == (pdes_uv + copy_size));
    }

}

//for dump, des to src
void store_residue_layout_back(short* psrc, int mb_width, int mb_height)
{
    int i, mbline;
    short* pdes_y, *pdes_y2, *pdes_uv;
    short* store_back;
    int copy_size;

    copy_size = mb_width*8*8*2;
    pdes_y = psrc + copy_size*3*(mb_height - 2);
    pdes_y2 = pdes_y + copy_size;
    pdes_uv = pdes_y + 2*copy_size;


    for (mbline = mb_height-1; mbline >=0; mbline --) {
        store_back = psrc + copy_size*3*mbline;
        for (i = 0; i < mb_width; i++) {

            memcpy(store_back, pdes_y, 8*8*2*2);
            store_back += 8*8*2;
            pdes_y += 8*8*2;
            memcpy(store_back, pdes_y2, 8*8*2*2);
            store_back += 8*8*2;
            pdes_y2 += 8*8*2;
            memcpy(store_back, pdes_uv, 8*8*2*2);
            store_back += 8*8*2;
            pdes_uv += 8*8*2;
        }
        pdes_y -= 4*copy_size;
        pdes_y2 -= 4*copy_size;
        pdes_uv -= 4*copy_size;

        ambadec_assert_ffmpeg(store_back == (pdes_y + 9*copy_size));
        //ambadec_assert_ffmpeg(psrc == (pdes_y2 + 8*copy_size));
        //ambadec_assert_ffmpeg(psrc == (pdes_uv + 7*copy_size));
    }

}
#endif

//wake up decode interface
void sendNULLFrame(RV34AmbaDecContext *thiz)
{
    ctx_nodef_t* p_node;
    p_node=thiz->p_frame_pool->get_cmd(thiz->p_frame_pool);
    p_node->p_ctx=NULL;
    thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
}

//threads
void* thread_rv34_amba_vld(void* p)
{
    RV34AmbaDecContext *thiz=(RV34AmbaDecContext *)p;
    MpegEncContext *s = &thiz->s;
    SliceInfo si;
    int i;
    int slice_count;
    const uint8_t *slices_hdr = NULL;
    int last = 0;
    vld_data_t* p_vld;
//    AVCodecContext *avctx;
//    AVPacket *avpkt;
    ctx_nodef_t* p_node;
    const uint8_t *buf ;
    int buf_size ;
    int offset = 0;
    int offset_1 = 0;

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"vld_thread start.\n");
    #endif

    //start mc intrapred thread
    pthread_create(&thiz->tid_mc_intrapred,NULL,thread_rv34_amba_mcintrapred,thiz);

    //start deblock thread
    pthread_create(&thiz->tid_deblock,NULL,thread_rv34_amba_deblock,thiz);

    while(thiz->vld_loop)
    {
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Waiting: vld_thread input: wait on p_vld_dataq.\n");
        #endif

        #ifdef __log_communicate_queue__
            ambadec_print_triqueue_general_status(thiz->p_vld_dataq);
        #endif

        p_node=thiz->p_vld_dataq->get(thiz->p_vld_dataq);

        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Escape: vld_thread input: escape from p_vld_dataq,p_node->p_ctx=%p.\n",p_node->p_ctx);
        #endif

        #ifdef __log_communicate_queue__
            ambadec_print_triqueue_detailed_status(thiz->p_vld_dataq);
        #endif

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"**** vld_thread get p_node, start a loop, log_cur_frame=%d .\n",log_cur_frame);
        #endif

        p_vld=p_node->p_ctx;

        //flush
        if (thiz->vld_need_flush || thiz->error_wait_flush) {
            av_log(NULL,AV_LOG_ERROR," **Flush vld_thread.\n");
            if (!p_vld) {
                if (p_node->flag == _flag_cmd_flush_) {
                    av_log(NULL,AV_LOG_ERROR," ** vldthread start send flush cmd.\n");
                    //flush packet come, send a flush packet to down stream
                    p_node = thiz->p_mcintrapred_dataq->get_cmd(thiz->p_mcintrapred_dataq);
                    ambadec_assert_ffmpeg(p_node->p_ctx == 0);
                    thiz->p_mcintrapred_dataq->put_ready(thiz->p_mcintrapred_dataq,p_node,_flag_cmd_flush_);

                    thiz->p_pic = NULL;
                    thiz->vld_need_flush = 0;
                    av_log(NULL,AV_LOG_ERROR," **Flush vld_thread done.\n");

                    continue;
                } else {
                    av_log(NULL,AV_LOG_ERROR,"must not come here 2.0.\n");
                    ambadec_assert_ffmpeg(0);
                }
            } else {
                ambadec_assert_ffmpeg(p_node->flag == 0);
                //free raw data and release p_vld
                if(p_vld->pbuf)
                {
                    //av_log(NULL,AV_LOG_ERROR,"free p_vld->pbuf=%p.\n",p_vld->pbuf);
                    av_free(p_vld->pbuf);
                    p_vld->pbuf=NULL;
                } else {
                    av_log(NULL,AV_LOG_ERROR,"must not come here 1.0.\n");
                }
                av_log(NULL,AV_LOG_ERROR," **Flushing vld_thread, discard packet %p.\n", p_vld);
                thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
                continue;
            }
        }

        if(!p_vld ||thiz->start_exit)
        {
            //free all datas
            if (p_vld) {
                ambadec_assert_ffmpeg(thiz->start_exit);
                if (p_vld->pbuf) {
                    av_free(p_vld->pbuf);
                    p_vld->pbuf = NULL;
                    thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
                }
                continue;
            }
            if(p_node->flag==_flag_cmd_exit_next_)
            {
                //#ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"vld_thread get exit cmd(_flag_cmd_exit_next_), start exit.\n");
                //#endif

                //p_node=thiz->p_mcintrapred_dataq->get_cmd(thiz->p_mcintrapred_dataq);
                //thiz->p_mcintrapred_dataq->put_ready(thiz->p_mcintrapred_dataq,p_node,_flag_cmd_exit_next_);

                thiz->vld_loop=0;
            }
            else //unknown cmd, exit too
            {
                //#ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"vld_thread get unknown cmd, start exit, flag 0x%x.\n", p_node->flag);
                //#endif

                //p_node=thiz->p_mcintrapred_dataq->get_cmd(thiz->p_mcintrapred_dataq);
                //thiz->p_mcintrapred_dataq->put_ready(thiz->p_mcintrapred_dataq,p_node,_flag_cmd_exit_next_);

                thiz->vld_loop=0;
            }
            break;
        }

//        av_log(NULL,AV_LOG_ERROR,"start decoding a frame.\n");
        slices_hdr = NULL;
        buf = p_vld->pbuf;
        buf_size = p_vld->size;

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"  vld_thread get data, buf_size=%d, slice_cnt=%d .\n",p_vld->size,p_vld->slice_cnt);
        #endif

#ifdef __log_dump_decocer_input_data__
        log_dump_with_audo_cnt_2("rawdata", buf, buf_size);
        char dump_str[100];
        int itt ;
        snprintf(dump_str,99,"p_vld->slice_cnt %d", p_vld->slice_cnt);
        log_text_f(log_fd_temp, dump_str);

        if(!p_vld->slice_cnt){
            slice_count = (*buf++) + 1;
            slices_hdr = buf + 4;
            buf += 8 * slice_count;
            snprintf(dump_str,99,"1 p_vld->slice_cnt %d, data %x", slice_count, *((int*)buf));
            log_text_f(log_fd_temp, dump_str);
        }else {
            slice_count = p_vld->slice_cnt;
            snprintf(dump_str,99,"2 p_vld->slice_cnt %d", slice_count);
            log_text_f(log_fd_temp, dump_str);
            for (itt = 0; itt<slice_count; itt++) {
                snprintf(dump_str,99,"%d p_vld->slice_offset %d", itt, p_vld->slice_offset[itt]);
                log_text_f(log_fd_temp, dump_str);
            }
        }
#else
        if(!p_vld->slice_cnt){
            slice_count = (*buf++) + 1;
            slices_hdr = buf + 4;
            buf += 8 * slice_count;
        }else
            slice_count = p_vld->slice_cnt;
#endif

        offset = get_slice_offset_amba(p_vld, slices_hdr, 0);

        //parse first slice header to check whether this frame can be decoded
        if(offset > buf_size)
        {
            av_log(NULL, AV_LOG_ERROR, "**error** in vld_thread: Slice offset is greater than frame size\n");
            //error here
//            thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
            goto freedata;
        }

#ifdef __log_dump_decocer_input_data__
        log_dump_with_audo_cnt("sliceheaderdata", buf+offset, buf_size-offset);
#endif
        init_get_bits(&s->gb, buf+offset, (buf_size-offset)*8);
        if(thiz->parse_slice_header(thiz, &thiz->s.gb, &si) < 0 || si.start)
        {
            av_log(NULL, AV_LOG_ERROR, "**error** in vld_thread: First slice header is incorrect\n");
            //error here
//            thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
            goto freedata;
        }

        if((thiz->s.avctx->flags&CODEC_FLAG_LOW_DELAY) && (si.type == FF_B_TYPE))
        {
            //av_log(NULL, AV_LOG_ERROR, " Low delay mode Skip B picture\n");
//            thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
            goto freedata;
        }

        if((!s->last_picture_ptr || !s->last_picture_ptr->data[0]) && si.type == FF_B_TYPE)
        {
            av_log(NULL, AV_LOG_ERROR, "**error** in vld_thread: B picture without reference? \n");
            //error here
//            thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
            goto freedata;
        }

        if((!s->next_picture_ptr || !s->next_picture_ptr->data[0]) && si.type == FF_P_TYPE)
        {
            av_log(NULL, AV_LOG_ERROR, "**error** in vld_thread: P picture without reference? \n");
            //error here
//            thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
            goto freedata;
        }

//log & dump
#ifdef __log_decoding_config__
    if(log_cur_frame==log_config_start_frame)
        apply_decoding_config();
#endif

#ifdef __log_dump_idct_data__
    log_openfile_p(log_fd_idct_y,"idct_data_amba_y",log_cur_frame);
    log_openfile_p(log_fd_idct_uv,"idct_data_amba_uv",log_cur_frame);
    log_openfile_text_p(log_fd_idct_text,"idct_text_amba",log_cur_frame);
#endif

#ifdef __log_intrapred__
    log_openfile_p(log_fd_intrapred, "amba/intrapred_type",log_cur_frame);
#endif
#ifdef __log_mc__
    log_openfile_p(log_fd_mc, "amba/mc_type",log_cur_frame);
#endif
#ifdef __log_mv_new__
    log_openfile_p(log_fd_mv_new, "amba/mv_new",log_cur_frame);
#endif
#ifdef __log_mv__
    log_openfile_p(log_fd_mv, "amba/mv_value",log_cur_frame);
#endif

#if 0
//#ifdef __dump_temp__
    log_openfile_text_p(log_fd_temp,"avail",log_cur_frame);
#endif

#ifndef __dump_whole__
#ifdef __dump_DSP_test_data__

#ifdef __dump_binary__
    log_openfile_p(log_fd_mbtype,"mb_type",log_cur_frame);

    log_openfile_p(log_fd_residual,"residual",log_cur_frame);
    log_openfile_p(log_fd_residual_2,"2residual",log_cur_frame);
    log_openfile_p(log_fd_residual_3,"3residual",log_cur_frame);
    log_openfile_p(log_fd_mvdsp,"mv",log_cur_frame);
    log_openfile_p(log_fd_reffyraw,"reff_y",log_cur_frame);
    log_openfile_p(log_fd_reffuvraw,"reff_uv",log_cur_frame);
//    log_openfile_p(log_fd_reffytiled,"rv_reff_y_tiled",log_cur_frame);
//    log_openfile_p(log_fd_reffuvtiled,"rv_reff_uv_tiled",log_cur_frame);
    log_openfile_p(log_fd_refbyraw,"refb_y",log_cur_frame);
    log_openfile_p(log_fd_refbuvraw,"refb_uv",log_cur_frame);
//    log_openfile_p(log_fd_refbytiled,"rv_refb_y_tiled",log_cur_frame);
//    log_openfile_p(log_fd_refbuvtiled,"rv_refb_uv_tiled",log_cur_frame);
    log_openfile_p(log_fd_result,"ref_result",log_cur_frame);
//    log_openfile_p(log_fd_result_2,"rv_result_y_rasterscaned",log_cur_frame);
//    log_openfile_p(log_fd_result_3,"rv_result_uv_rasterscaned",log_cur_frame);
#endif

#endif
#endif

#ifdef __dump_DSP_TXT__
    //av_log(NULL,AV_LOG_ERROR,"**open file.\n");
    if(log_openfile_text_p(log_fd_dsp_text,"rv_text",log_cur_frame)){
        //av_log(NULL,AV_LOG_ERROR,"**open file fail.\n");
    }
    if(si.type == FF_P_TYPE)
        log_text_p(log_fd_dsp_text,"===================== P-VOP ====================\n",log_cur_frame);
    else if(si.type == FF_B_TYPE)
        log_text_p(log_fd_dsp_text,"===================== B-VOP ====================\n",log_cur_frame);
    else if(si.type == FF_I_TYPE)
        log_text_p(log_fd_dsp_text,"===================== I-VOP ====================\n",log_cur_frame);
    else
    {
        char txtt[40];
        snprintf(txtt,39,"unknown type=%d",si.type);
        log_text_p(log_fd_dsp_text,txtt, log_cur_frame);
    }
#endif

#ifdef __dump_deblock_DSP_TXT__
    if(log_openfile_text_p(log_fd_dsp_deblock_text,"rv_deblock_text",log_cur_frame)){
        //av_log(NULL,AV_LOG_ERROR,"**open file fail.\n");
    }
#endif

#ifdef __dump_mc_dct_DSP_TXT__
    if(log_openfile_text_p(log_fd_dsp_mc_dct_text,"rv_mc_dct_text",log_cur_frame)){
        //av_log(NULL,AV_LOG_ERROR,"**open file fail.\n");
    }
#endif

        for(i=0; i<slice_count; i++)
        {
            offset= get_slice_offset_amba(p_vld, slices_hdr, i);
            offset_1 = get_slice_offset_amba(p_vld, slices_hdr, i+1);
            int size;
            if(i+1 == slice_count)
                size= buf_size - offset;
            else
                size= offset_1 - offset;

            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"  vld thread start decoding a slice.\n");
            #endif

            if(offset > buf_size){
                av_log(NULL, AV_LOG_ERROR, "Slice offset is greater than frame size\n");
                p_node=thiz->p_mcintrapred_dataq->get_cmd(thiz->p_mcintrapred_dataq);
                thiz->p_mcintrapred_dataq->put_ready(thiz->p_mcintrapred_dataq,p_node,_flag_cmd_error_data_);
                break;
            }

            thiz->si.end = s->mb_width * s->mb_height;
            if(i+1 < slice_count){
#ifdef __log_dump_decocer_input_data__
        snprintf(dump_str,99,"3 buf_size %d, function %d", buf_size, offset_1);
        log_text_f(log_fd_temp, dump_str);
        log_dump_with_audo_cnt("slicedata1", buf+offset_1, (buf_size-offset_1));
#endif
                init_get_bits(&s->gb, buf+offset_1, (buf_size-offset_1)*8);
                if(thiz->parse_slice_header(thiz, &thiz->s.gb, &si) < 0){
                    if(i+2 < slice_count)
                        size = get_slice_offset_amba(p_vld, slices_hdr, i+2) - offset;
                    else
                        size = buf_size - offset;
                }else
                    thiz->si.end = si.start;
            }
            last = rv34new_amba_decode_slice(thiz, thiz->si.end, buf + offset, size, p_vld->pts);
            s->mb_num_left = thiz->s.mb_x + thiz->s.mb_y*thiz->s.mb_width - thiz->si.start;
            if(last)
                break;
        }

        if(thiz->p_pic)
        {
            //set iav_rv40_mc_decode_t
            thiz->p_pic->picinfo.uu.rv40.pic_width=(thiz->p_pic->width + 15)&(~15);
            thiz->p_pic->picinfo.uu.rv40.pic_height=(thiz->p_pic->height + 15) &(~15);
            thiz->p_pic->picinfo.uu.rv40.coding_type=s->pict_type;
        }

        if(last>0)
        {
            process_data_t* p_mcintrapred;
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"**** current pic vld done****.\n");
            #endif

            #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"   last=%d,thiz->p_pic=%p.\n",last,thiz->p_pic);
            #endif

            thiz->pic_finished=1;
            if(s->pict_type!=FF_I_TYPE || thiz->vld_sent_row!=s->mb_height)
            {
                ambadec_assert_ffmpeg(s->pict_type!=FF_I_TYPE);
                ambadec_assert_ffmpeg(thiz->vld_sent_row==0);

                #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"   vld thread output: non-I picture case.\n");
                #endif

                pthread_mutex_lock(&thiz->mutex);
                thiz->decoding_frame_cnt++;
                pthread_mutex_unlock(&thiz->mutex);

                if(thiz->previous_pic_is_b)//previous is b, can start current pic's mc processing
                {
                    //sending data to mcintrapred thread
                    p_node= thiz->p_mcintrapred_dataq->get_free(thiz->p_mcintrapred_dataq);
                    if(!p_node->p_ctx)
                    {
                        p_node->p_ctx=av_malloc(sizeof(process_data_t));
                    }
                    p_mcintrapred=p_node->p_ctx;
                    p_mcintrapred->p_picdata=thiz->p_pic;
                    p_mcintrapred->start_row=thiz->vld_sent_row;
                    p_mcintrapred->end_row=s->mb_height;

                    #ifdef __log_communicate_queue__
                        av_log(NULL,AV_LOG_ERROR,"    trigger-2 mc_intrapred mbrow=%d-%d start.\n",p_mcintrapred->start_row,p_mcintrapred->end_row);
                    #endif

                    #ifdef __log_parallel_control__
                        av_log(NULL,AV_LOG_ERROR,"   vld thread output: previous_pic_is_b case, sending mcintrapred start_row=%d, end_row=%d, p_mcintrapred->p_picdata=%p .\n",p_mcintrapred->start_row,p_mcintrapred->end_row,p_mcintrapred->p_picdata);
                    #endif
                    ambadec_assert_ffmpeg(thiz->p_pic);

                    thiz->p_mcintrapred_dataq->put_ready(thiz->p_mcintrapred_dataq,p_node,0);

                    //get next picture context
                    p_node= thiz->p_pic_dataq->get(thiz->p_pic_dataq);
                    if(!p_node->p_ctx)
                    {
                        if(p_vld->pbuf)
                        {
                            av_free(p_vld->pbuf);
                            p_vld->pbuf=NULL;
                        }
                        thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
                        av_log(NULL,AV_LOG_ERROR,"**** vld_thread get pic==NULL 1****.\n");
                        continue;
                        //#ifdef __log_parallel_control__
                        //av_log(NULL,AV_LOG_ERROR,"**** vld_thread exit get pic==NULL 1****.\n");
                       //#endif
                        //break;
                    }
                    ambadec_assert_ffmpeg(p_node->p_ctx==(&thiz->picdata[0])||p_node->p_ctx==(&thiz->picdata[1]));
                    thiz->p_pic=p_node->p_ctx;
#ifdef _intra_using_allocate_residue_
                    thiz->p_pic->pdct = NULL;
                    thiz->p_pic->pinfo = NULL;
#endif
                    #ifdef __log_parallel_control__
                        av_log(NULL,AV_LOG_ERROR,"   vld thread output: previous_pic_is_b case, get pic context=%p .\n",thiz->p_pic);
                    #endif
                }
                else
                {
                    #ifdef __log_parallel_control__
                        av_log(NULL,AV_LOG_ERROR,"   vld thread output: previous_pic_is_not_b case, start getting thiz->p_pic .\n");
                    #endif
                    //make sure previous picture is done
                    p_node= thiz->p_pic_dataq->get(thiz->p_pic_dataq);
                    if(!p_node->p_ctx)
                    {
                        if(p_vld->pbuf)
                        {
                            av_free(p_vld->pbuf);
                            p_vld->pbuf=NULL;
                        }
                        thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
                        av_log(NULL,AV_LOG_ERROR,"**** vld_thread get pic==NULL 1****.\n");
                        continue;
                        //#ifdef __log_parallel_control__
                        //av_log(NULL,AV_LOG_ERROR,"**** vld_thread exit get pic==NULL 2****.\n");
                        //#endif
                        //break;
                    }
                    ambadec_assert_ffmpeg(p_node->p_ctx==(&thiz->picdata[0])||p_node->p_ctx==(&thiz->picdata[1]));
                    rv40_pic_data_t* ptmp=thiz->p_pic;
                    thiz->p_pic=p_node->p_ctx;

#ifdef _intra_using_allocate_residue_
                    thiz->p_pic->pdct = NULL;
                    thiz->p_pic->pinfo = NULL;
#endif
                    #ifdef __log_parallel_control__
                        av_log(NULL,AV_LOG_ERROR,"   vld thread output: previous_pic_is_not_b case, get pic context=%p .\n",thiz->p_pic);
                    #endif

                    //sending data to mcintrapred thread
                    p_node= thiz->p_mcintrapred_dataq->get_free(thiz->p_mcintrapred_dataq);
                    if(!p_node->p_ctx)
                    {
                        p_node->p_ctx=av_malloc(sizeof(process_data_t));
                    }
                    p_mcintrapred=p_node->p_ctx;
                    p_mcintrapred->p_picdata=ptmp;
                    p_mcintrapred->start_row=thiz->vld_sent_row;
                    p_mcintrapred->end_row=s->mb_height;

                    ambadec_assert_ffmpeg(ptmp);

                    #ifdef __log_communicate_queue__
                        av_log(NULL,AV_LOG_ERROR,"    trigger-3 mc_intrapred mbrow=%d-%d start.\n",p_mcintrapred->start_row,p_mcintrapred->end_row);
                    #endif

                    #ifdef __log_parallel_control__
                        av_log(NULL,AV_LOG_ERROR,"   vld thread output: previous_pic_is_not_b case, sending mcintrapred start_row=%d, end_row=%d .\n",p_mcintrapred->start_row,p_mcintrapred->end_row);
                    #endif

                    thiz->p_mcintrapred_dataq->put_ready(thiz->p_mcintrapred_dataq,p_node,0);

                }
            }
            else//I picture
            {
                ambadec_assert_ffmpeg(s->pict_type==FF_I_TYPE);
                ambadec_assert_ffmpeg(thiz->vld_sent_row==s->mb_height);

                #ifdef __log_parallel_control__
                    av_log(NULL,AV_LOG_ERROR,"   vld thread output: I picture case, start geting thiz->p_pic.\n");
                #endif

                //get next picture context
                p_node= thiz->p_pic_dataq->get(thiz->p_pic_dataq);
                if(!p_node->p_ctx)
                {
                    #ifdef __log_parallel_control__
                    av_log(NULL,AV_LOG_ERROR,"**** vld_thread exit get pic==NULL 3****.\n");
                    #endif
                    if(p_vld->pbuf)
                    {
                        av_free(p_vld->pbuf);
                        p_vld->pbuf=NULL;
                    }
                    thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
                    continue;
                }
                ambadec_assert_ffmpeg(p_node->p_ctx==(&thiz->picdata[0])||p_node->p_ctx==(&thiz->picdata[1]));
                thiz->p_pic=p_node->p_ctx;
#ifdef _intra_using_allocate_residue_
                thiz->p_pic->pdct = NULL;
                thiz->p_pic->pinfo = NULL;
#endif
                #ifdef __log_parallel_control__
                    av_log(NULL,AV_LOG_ERROR,"   vld thread output: I picture case, get pic context=%p .\n",thiz->p_pic);
                #endif
            }
            s->current_picture_ptr=NULL;
        }
        else if(last==-2)
        {
            av_log(NULL,AV_LOG_ERROR,"   vld thread NULL pic .\n");
            //NULL pic, flush is comming
            if(p_vld->pbuf)
            {
                av_free(p_vld->pbuf);
                p_vld->pbuf=NULL;
            }
            thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
            thiz->vld_sent_row=0;//reset value
            continue;
        }else if(last==-3)
        {
            av_log(NULL,AV_LOG_ERROR,"   vld thread get_buffer fail .\n");
            //get_buffer fail, flush is comming
            if(p_vld->pbuf)
            {
                av_free(p_vld->pbuf);
                p_vld->pbuf=NULL;
            }
            thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);
            thiz->vld_sent_row=0;//reset value
            thiz->error_wait_flush = 1;
            sendNULLFrame(thiz);
            continue;
        }
        else
        {
//            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"**** vld_thread done with error****.\n");
//            #endif
            thiz->pic_finished=0;
            p_node=thiz->p_frame_pool->get_cmd(thiz->p_frame_pool);
            ambadec_assert_ffmpeg(p_node->p_ctx == NULL);
            p_node->p_ctx=NULL;
            #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"****error, send NULL pic when vld decoding error 1.\n");
            #endif
            thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
        }

if (0)
{
freedata:
    p_node=thiz->p_frame_pool->get_cmd(thiz->p_frame_pool);
    ambadec_assert_ffmpeg(p_node->p_ctx == NULL);
    p_node->p_ctx=NULL;
    #ifdef __log_parallel_control__
    av_log(NULL,AV_LOG_ERROR,"****error, send NULL pic when vld decoding error 2.\n");
    #endif
    thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
}
        //update previous_pic_is_b
        thiz->previous_pic_is_b=s->pict_type==FF_B_TYPE;

        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"      update thiz->previous_pic_is_b=%d,s->pict_type=%d.\n",thiz->previous_pic_is_b,s->pict_type);
        #endif

        #if 0
        //#ifdef __dump_temp__
            log_closefile_p(log_fd_temp,log_cur_frame);
        #endif
        thiz->vld_sent_row=0;//reset value
        if(p_vld->pbuf)
        {
            av_free(p_vld->pbuf);
            p_vld->pbuf=NULL;
        }
        thiz->p_vld_dataq->release(thiz->p_vld_dataq,p_vld,0);

        log_cur_frame++;

    }

    //decoder exit
    #ifdef __log_parallel_control__
    av_log(NULL,AV_LOG_ERROR,"****vld_thread start exit, join mcintrapred and deblock threads.\n");
    #endif

    p_node=thiz->p_mcintrapred_dataq->get_cmd(thiz->p_mcintrapred_dataq);
    thiz->p_mcintrapred_dataq->put_ready(thiz->p_mcintrapred_dataq,p_node,_flag_cmd_exit_next_);
    p_node=thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
    thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,_flag_cmd_exit_next_);
    void* pv1, *pv2;
    int ret1, ret2;
    ret1=pthread_join(thiz->tid_mc_intrapred,&pv1);
    ret2=pthread_join(thiz->tid_deblock,&pv2);
    av_log(NULL,AV_LOG_DEBUG,"joined result=%d,%d.\n",ret1,ret2);

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"vld_thread exit.\n");
    #endif

//    thiz->p_thread_exit->enqueue(thiz->p_thread_exit,NULL);
    #ifdef __log_parallel_control__
    av_log(NULL,AV_LOG_ERROR,"vld_thread exit end, joined mcintrapred and deblock threads.\n");
    #endif
    thiz->vld_loop=0;
    thiz->vld_need_flush = 0;
    pthread_exit(NULL);
    return NULL;
}

void* thread_rv34_amba_mcintrapred(void* p)
{
    RV34AmbaDecContext *thiz=(RV34AmbaDecContext *)p;
//    MpegEncContext *s = &thiz->s;
    rv40_pic_data_t* p_picdata;

    process_data_t* p_mcintrapred,*p_deblock;
    ctx_nodef_t* p_node;

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"mcintrapred_thread start.\n");
    #endif

    while(thiz->mc_intrapred_loop)
    {
        //get data
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Waiting: mcintrapred_thread input: wait on p_mcintrapred_dataq.\n");
        #endif

        #ifdef __log_communicate_queue__
            ambadec_print_triqueue_general_status(thiz->p_mcintrapred_dataq);
        #endif

        p_node=thiz->p_mcintrapred_dataq->get(thiz->p_mcintrapred_dataq);

        p_mcintrapred=p_node->p_ctx;

        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Escape: mcintrapred_thread input: escape from p_mcintrapred_dataq,p_node->p_ctx=%p.\n",p_node->p_ctx);
        #endif

        #ifdef __log_communicate_queue__
            ambadec_print_triqueue_detailed_status(thiz->p_mcintrapred_dataq);
        #endif

        //flush
        if (thiz->idct_mc_need_flush || thiz->error_wait_flush) {
            if (!p_mcintrapred) {
                if (p_node->flag == _flag_cmd_flush_) {
                    av_log(NULL,AV_LOG_ERROR," **mc_idct_thread start send flush cmd.\n");
                    //flush packet come, send a flush packet to down stream
                    p_node= thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                    thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,_flag_cmd_flush_);
                    thiz->idct_mc_need_flush = 0;
                    av_log(NULL,AV_LOG_ERROR," **Flush mc_idct_thread done.\n");
                    continue;
                } else {
                    av_log(NULL,AV_LOG_ERROR,"must not come here 3.0.\n");
                    ambadec_assert_ffmpeg(0);
                }
            } else {
                ambadec_assert_ffmpeg(p_node->flag == 0);
                av_log(NULL,AV_LOG_ERROR," **Flushing mc_idct_thread, p_pic_dataq status:.\n");
                //#ifdef __log_communicate_queue__
                //ambadec_print_triqueue_general_status(thiz->p_mcintrapred_dataq);
                //ambadec_print_triqueue_detailed_status(thiz->p_mcintrapred_dataq);
                //#endif
                thiz->p_mcintrapred_dataq->release(thiz->p_mcintrapred_dataq,p_mcintrapred,0);
                continue;
            }
        }

        if(!p_mcintrapred)
        {
            if(p_node->flag==_flag_cmd_exit_next_)
            {
                //#ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"mcintrapred_thread get NULL data(_flag_cmd_exit_next_), start exit.\n");
                //#endif

                #ifdef __log_parallel_control__
                    av_log(NULL,AV_LOG_ERROR,"**** mcintrapred_thread, get exit cmd, start exit.\n");
                #endif

                goto mcintrapred_thread_exit;
            }
            else
            {
                av_log(NULL, AV_LOG_ERROR, "Error data indicater in thread_rv34_amba_mcintrapred\n");
                //p_node=thiz->p_deblock_dataq->get_cmd(thiz->p_deblock_dataq);
                //thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,_flag_cmd_error_data_);
                //continue;
                goto mcintrapred_thread_exit;
            }
        }

        ambadec_assert_ffmpeg(p_mcintrapred);
        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"**** mcintrapred_thread get p_node, start a loop, %d--%d thiz->mc_sent_row=%d.\n",p_mcintrapred->start_row,p_mcintrapred->end_row,thiz->mc_sent_row);
        #endif

        //process data
        p_picdata=p_mcintrapred->p_picdata;
        if(!p_picdata)
        {
            av_log(NULL, AV_LOG_ERROR, "****Error here, p_mcintrapred->p_picdata==NULL, why get this crash data\n");
            av_log(NULL, AV_LOG_ERROR, " p_mcintrapred->start_row=%d,p_mcintrapred->end_row=%d.\n",p_mcintrapred->start_row,p_mcintrapred->end_row);
        }
        ambadec_assert_ffmpeg(p_picdata->current_picture_ptr);

        //av_log(NULL, AV_LOG_ERROR, "******picture type %d, cnt %d .\n",p_picdata->current_picture_ptr->pict_type,p_picdata->frame_cnt);
        //av_log(NULL, AV_LOG_ERROR, "******lu %p, ch %p .\n",p_picdata->current_picture_ptr->data[0],p_picdata->current_picture_ptr->data[1]);
#ifdef __dump_DSP_test_data__
        char dump_str[100];
        snprintf(dump_str,99,"picture type %d, cnt %d ",p_picdata->current_picture_ptr->pict_type,p_picdata->frame_cnt);
        log_text_f(log_fd_general_txt, dump_str);
#endif

        //B
        if(p_picdata->current_picture_ptr->pict_type==FF_B_TYPE )
        {
            ambadec_assert_ffmpeg(p_picdata->last_picture_ptr);
            ambadec_assert_ffmpeg(p_picdata->next_picture_ptr);
            //av_log(NULL, AV_LOG_ERROR, "******fw lu %p, ch %p .\n",p_picdata->last_picture_ptr->data[0],p_picdata->last_picture_ptr->data[1]);
            //av_log(NULL, AV_LOG_ERROR, "******bw lu %p, ch %p .\n",p_picdata->next_picture_ptr->data[0],p_picdata->next_picture_ptr->data[1]);

            //process MC and add result for Inter MB
            #ifdef __log_decoding_config__
                if(log_mask[decoding_config_mc])
            #endif
            {

#ifdef __dump_DSP_test_data__
                _dump_input_for_dsp(p_picdata);
#endif

                if (thiz->use_dsp) {
                    ambadec_assert_ffmpeg(thiz->s.avctx->acceleration);
                    ambadec_assert_ffmpeg(p_picdata->last_picture_ptr);
                    ambadec_assert_ffmpeg(p_picdata->next_picture_ptr);
                    ambadec_assert_ffmpeg(p_picdata->current_picture_ptr);
                    ambadec_assert_ffmpeg(p_picdata->current_picture_ptr->linesize[0] == p_picdata->current_picture_ptr->linesize[1]);
                    ambadec_assert_ffmpeg(p_picdata->picinfo.uu.rv40.coding_type == FF_B_TYPE);

                    //ambadec_assert_ffmpeg(p_picdata->picinfo.uu.rv40.residual_daddr_start == p_picdata->pdct);
                    ambadec_assert_ffmpeg(p_picdata->picinfo.uu.rv40.residual_daddr_end == p_picdata->picinfo.uu.rv40.residual_daddr_start + thiz->pAcc->amba_idct_buffer_size);
                    ambadec_assert_ffmpeg((void *)p_picdata->picinfo.uu.rv40.mv_daddr_start == (void *)p_picdata->pinfo);
                    ambadec_assert_ffmpeg(p_picdata->picinfo.uu.rv40.mv_daddr_end == p_picdata->picinfo.uu.rv40.mv_daddr_start + thiz->pAcc->amba_mv_buffer_size);
                    ambadec_assert_ffmpeg(p_picdata->picinfo.uu.rv40.target_fb_id == p_picdata->pmc_result_buffer_id);
                    //p_picdata->picinfo.uu.rv40.residual_daddr_start = p_picdata->pdct;
                    //p_picdata->picinfo.uu.rv40.residual_daddr_end = p_picdata->picinfo.uu.rv40.residual_daddr_start + thiz->pAcc->amba_idct_buffer_size;
                    //p_picdata->picinfo.uu.rv40.mv_daddr_start = p_picdata->pinfo;
                    //p_picdata->picinfo.uu.rv40.mv_daddr_end = p_picdata->picinfo.uu.rv40.mv_daddr_start + thiz->pAcc->amba_mv_buffer_size;
                    //p_picdata->picinfo.uu.rv40.target_buffer_id = p_picdata->pmc_result_buffer_id;

                    p_picdata->picinfo.uu.rv40.fwd_ref_fb_id = p_picdata->last_picture_ptr->user_buffer_id;
                    p_picdata->picinfo.uu.rv40.bwd_ref_fb_id = p_picdata->next_picture_ptr->user_buffer_id;
                    p_picdata->picinfo.uu.rv40.coding_type = FF_B_TYPE;
                    p_picdata->picinfo.uu.rv40.clean_fwd_ref_fb = 1;
                    p_picdata->picinfo.uu.rv40.clean_bwd_ref_fb = 1;
                    //av_log(NULL, AV_LOG_ERROR, "decoding b fw %d, bw %d, cur %d, target %d.\n", p_picdata->last_picture_ptr->user_buffer_id, p_picdata->next_picture_ptr->user_buffer_id, p_picdata->current_picture_ptr->user_buffer_id, p_picdata->picinfo.uu.rv40.target_fb_id);
                    //av_log(NULL, AV_LOG_ERROR, "real_fb_id fw %d, bw %d, cur %d.\n", p_picdata->last_picture_ptr->real_fb_id, p_picdata->next_picture_ptr->real_fb_id, p_picdata->current_picture_ptr->real_fb_id);
//use or not use dsp do mc
#if 0
                    process_mc_rv34_amba_b(thiz,p_picdata);
#else

#ifdef _d_use_new_residue_layout_
#ifndef _d_debug_new_layout_
                    if (p_picdata->new_residue_layout == 0) {
                        change_residue_layout(p_picdata->pdct,p_picdata->mb_width,p_picdata->mb_height);
                        p_picdata->new_residue_layout = 1;
                    }
#endif
#endif

                    if (thiz->s.avctx->acceleration(thiz->pAcc, &p_picdata->picinfo) < 0 ) {
                        thiz->error_wait_flush = 1;
                        sendNULLFrame(thiz);
                        av_log(NULL,AV_LOG_ERROR," decode error, udec in error state or is stoped.\n");
                        continue;
                    }

                    //log_openfile_p(log_fd_temp,"dsp_mc_result",p_picdata->frame_cnt);
                    //log_dump_p(log_fd_temp,p_picdata->pmc_result,p_picdata->mb_height*p_picdata->mb_width*384,p_picdata->frame_cnt);
                    //log_closefile_p(log_fd_temp,p_picdata->frame_cnt);
                    //_writeback_mc_result(p_picdata->pmc_result, p_picdata->current_picture_ptr->data[0], p_picdata->current_picture_ptr->data[1], p_picdata->current_picture_ptr->linesize[0], p_picdata->mb_width, p_picdata->mb_height, p_picdata->pmbtype);
                    if(thiz->neon.rv40_new_result != NULL){
                        thiz->neon.rv40_new_result(p_picdata->pmc_result, p_picdata->current_picture_ptr->data[0], p_picdata->current_picture_ptr->data[1], p_picdata->current_picture_ptr->linesize[0], p_picdata->mb_width, p_picdata->mb_height, (unsigned char *)p_picdata->pmbtype);
                    }else{
                        _writeback_new_mc_result(p_picdata->pmc_result, p_picdata->current_picture_ptr->data[0], p_picdata->current_picture_ptr->data[1], p_picdata->current_picture_ptr->linesize[0], p_picdata->mb_width, p_picdata->mb_height, p_picdata->pmbtype);
                    }
                    //_writeback_new_mc_result(p_picdata->pmc_result, p_picdata->current_picture_ptr->data[0], p_picdata->current_picture_ptr->data[1], p_picdata->current_picture_ptr->linesize[0], p_picdata->mb_width, p_picdata->mb_height, p_picdata->pmbtype);

#ifdef _d_use_new_residue_layout_
#ifndef _d_debug_new_layout_
                    if (p_picdata->new_residue_layout == 1) {
                        store_residue_layout_back(p_picdata->pdct,p_picdata->mb_width,p_picdata->mb_height);
                        p_picdata->new_residue_layout = 0;
                    }
#endif
#endif

#endif
                } else {

//for dump
#ifdef _d_use_new_residue_layout_
#ifndef _d_debug_new_layout_
                    if (p_picdata->new_residue_layout == 1) {
                        store_residue_layout_back(p_picdata->pdct,p_picdata->mb_width,p_picdata->mb_height);
                        p_picdata->new_residue_layout = 0;
                    }
#endif
#endif

                    process_mc_rv34_amba_b(thiz,p_picdata);
                }

#ifdef __dump_DSP_test_data__
                _dump_result_for_dsp(p_picdata);
#endif

            }
            //process intra prediction and add result for Intra MB
            process_intrapred_rv34_amba(thiz,p_picdata);
        }
        else if(p_picdata->current_picture_ptr->pict_type==FF_P_TYPE)//P
        {
            ambadec_assert_ffmpeg(p_picdata->last_picture_ptr);
            //av_log(NULL, AV_LOG_ERROR, "******fw lu %p, ch %p .\n",p_picdata->last_picture_ptr->data[0],p_picdata->last_picture_ptr->data[1]);

            //process MC and add result for Inter MB
            #ifdef __log_decoding_config__
                if(log_mask[decoding_config_mc])
            #endif
            {

#ifdef __dump_DSP_test_data__
                _dump_input_for_dsp(p_picdata);
#endif

                if (thiz->use_dsp) {
                    ambadec_assert_ffmpeg(thiz->s.avctx->acceleration);
                    ambadec_assert_ffmpeg(p_picdata->last_picture_ptr);
                    ambadec_assert_ffmpeg(p_picdata->current_picture_ptr);
                    ambadec_assert_ffmpeg(p_picdata->current_picture_ptr->linesize[0] == p_picdata->current_picture_ptr->linesize[1]);
                    ambadec_assert_ffmpeg(p_picdata->picinfo.uu.rv40.coding_type == FF_P_TYPE);

                    //ambadec_assert_ffmpeg((void*)p_picdata->picinfo.uu.rv40.residual_daddr_start == (void*)p_picdata->pdct);
                    ambadec_assert_ffmpeg((void*)p_picdata->picinfo.uu.rv40.residual_daddr_end == (void*)(p_picdata->picinfo.uu.rv40.residual_daddr_start + thiz->pAcc->amba_idct_buffer_size));
                    ambadec_assert_ffmpeg((void*)p_picdata->picinfo.uu.rv40.mv_daddr_start == (void*)p_picdata->pinfo);
                    ambadec_assert_ffmpeg((void*)p_picdata->picinfo.uu.rv40.mv_daddr_end == (void*)(p_picdata->picinfo.uu.rv40.mv_daddr_start + thiz->pAcc->amba_mv_buffer_size));
                    ambadec_assert_ffmpeg(p_picdata->picinfo.uu.rv40.target_fb_id == p_picdata->pmc_result_buffer_id);
                    //p_picdata->picinfo.uu.rv40.residual_daddr_start = p_picdata->pdct;
                    //p_picdata->picinfo.uu.rv40.residual_daddr_end = p_picdata->picinfo.uu.rv40.residual_daddr_start + thiz->pAcc->amba_idct_buffer_size;
                    //p_picdata->picinfo.uu.rv40.mv_daddr_start = p_picdata->pinfo;
                    //p_picdata->picinfo.uu.rv40.mv_daddr_end = p_picdata->picinfo.uu.rv40.mv_daddr_start + thiz->pAcc->amba_mv_buffer_size;
                    //p_picdata->picinfo.uu.rv40.target_buffer_id = p_picdata->pmc_result_buffer_id;
                    p_picdata->picinfo.uu.rv40.coding_type = FF_P_TYPE;
                    p_picdata->picinfo.uu.rv40.fwd_ref_fb_id = p_picdata->last_picture_ptr->user_buffer_id;
                    p_picdata->picinfo.uu.rv40.bwd_ref_fb_id = p_picdata->last_picture_ptr->user_buffer_id;//not used, assignment to pass IAV parameter-check
                    p_picdata->picinfo.uu.rv40.clean_fwd_ref_fb = 1;
                    p_picdata->picinfo.uu.rv40.clean_bwd_ref_fb = 0;
                    //av_log(NULL, AV_LOG_ERROR, "decoding p fw %d, bw %d, cur %d, target %d.\n", p_picdata->last_picture_ptr->user_buffer_id, p_picdata->next_picture_ptr->user_buffer_id, p_picdata->next_picture_ptr->user_buffer_id, p_picdata->picinfo.uu.rv40.target_fb_id);
                    //av_log(NULL, AV_LOG_ERROR, "real_fb_id fw %d, bw %d, %d.\n", p_picdata->last_picture_ptr->real_fb_id, p_picdata->next_picture_ptr->real_fb_id, p_picdata->current_picture_ptr->real_fb_id);
//use or not use dsp do mc
#if 0
                    process_mc_rv34_amba_p(thiz,p_picdata);
#else

#ifdef _d_use_new_residue_layout_
#ifndef _d_debug_new_layout_
                    if (p_picdata->new_residue_layout == 0) {
                        change_residue_layout(p_picdata->pdct,p_picdata->mb_width,p_picdata->mb_height);
                        p_picdata->new_residue_layout = 1;
                    }
#endif
#endif

                    if (thiz->s.avctx->acceleration(thiz->pAcc, &p_picdata->picinfo) < 0 ) {
                        thiz->error_wait_flush = 1;
                        sendNULLFrame(thiz);
                        av_log(NULL,AV_LOG_ERROR," decode error, udec in error state or is stoped.\n");
                        continue;
                    }

                    //log_openfile_p(log_fd_temp,"dsp_mc_result",p_picdata->frame_cnt);
                    //log_dump_p(log_fd_temp,p_picdata->pmc_result,p_picdata->mb_height*p_picdata->mb_width*384,p_picdata->frame_cnt);
                    //log_closefile_p(log_fd_temp,p_picdata->frame_cnt);

                    //_writeback_mc_result(p_picdata->pmc_result, p_picdata->current_picture_ptr->data[0], p_picdata->current_picture_ptr->data[1], p_picdata->current_picture_ptr->linesize[0], p_picdata->mb_width, p_picdata->mb_height, p_picdata->pmbtype);
                    if(thiz->neon.rv40_new_result != NULL){
                        thiz->neon.rv40_new_result(p_picdata->pmc_result, p_picdata->current_picture_ptr->data[0], p_picdata->current_picture_ptr->data[1], p_picdata->current_picture_ptr->linesize[0], p_picdata->mb_width, p_picdata->mb_height, (unsigned char *)p_picdata->pmbtype);
                    }else{
                        _writeback_new_mc_result(p_picdata->pmc_result, p_picdata->current_picture_ptr->data[0], p_picdata->current_picture_ptr->data[1], p_picdata->current_picture_ptr->linesize[0], p_picdata->mb_width, p_picdata->mb_height, p_picdata->pmbtype);
                    }
                    //_writeback_new_mc_result(p_picdata->pmc_result, p_picdata->current_picture_ptr->data[0], p_picdata->current_picture_ptr->data[1], p_picdata->current_picture_ptr->linesize[0], p_picdata->mb_width, p_picdata->mb_height, p_picdata->pmbtype);

#ifdef _d_use_new_residue_layout_
#ifndef _d_debug_new_layout_
                    if (p_picdata->new_residue_layout == 1) {
                        store_residue_layout_back(p_picdata->pdct,p_picdata->mb_width,p_picdata->mb_height);
                        p_picdata->new_residue_layout = 0;
                    }
#endif
#endif

#endif
                } else {

//for dump
#ifdef _d_use_new_residue_layout_
#ifndef _d_debug_new_layout_
                    if (p_picdata->new_residue_layout == 1) {
                        store_residue_layout_back(p_picdata->pdct,p_picdata->mb_width,p_picdata->mb_height);
                        p_picdata->new_residue_layout = 0;
                    }
#endif
#endif

                    process_mc_rv34_amba_p(thiz,p_picdata);
                }

#ifdef __dump_DSP_test_data__
                _dump_result_for_dsp(p_picdata);
#endif

            }
            //process intra prediction and add result for Intra MB
            process_intrapred_rv34_amba(thiz,p_picdata);
        }
        else//I
        {
            ambadec_assert_ffmpeg(p_picdata->current_picture_ptr->pict_type==FF_I_TYPE);
            ambadec_assert_ffmpeg(p_mcintrapred->start_row>=0);
            ambadec_assert_ffmpeg(p_mcintrapred->end_row>p_mcintrapred->start_row);
            ambadec_assert_ffmpeg(p_picdata->mb_height>=p_mcintrapred->end_row);
            ambadec_assert_ffmpeg(thiz->mc_sent_row<=p_mcintrapred->start_row);
            //av_log(NULL, AV_LOG_ERROR, "I id %d, real_fb_id cur %d.\n", p_picdata->current_picture_ptr->user_buffer_id, p_picdata->current_picture_ptr->real_fb_id);
            process_intrapred_rv34_amba_i(thiz,p_picdata,p_mcintrapred->start_row,p_mcintrapred->end_row);

            if(p_mcintrapred->end_row>(1+thiz->mc_sent_row))
            {
                p_node= thiz->p_deblock_dataq->get_free(thiz->p_deblock_dataq);
                if(!p_node->p_ctx)
                {
                    p_node->p_ctx=av_malloc(sizeof(process_data_t));
                }
                p_deblock=p_node->p_ctx;
                p_deblock->start_row=thiz->mc_sent_row;
                if(p_mcintrapred->end_row!=p_picdata->mb_height)
                {
                    thiz->mc_sent_row=p_deblock->end_row=p_mcintrapred->end_row-1;
                }
                else
                {
                    p_deblock->end_row=p_picdata->mb_height;
                    thiz->mc_sent_row=0;//reset value
                }
                p_deblock->p_picdata=p_mcintrapred->p_picdata;
                thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,0);

                //trigger next
                #ifdef __log_communicate_queue__
                    av_log(NULL,AV_LOG_ERROR,"    trigger deblock mbrow=%d--%d start.\n",p_deblock->start_row,p_deblock->end_row);
                #endif

            }

            //release data
            thiz->p_mcintrapred_dataq->release(thiz->p_mcintrapred_dataq,p_mcintrapred,0);
            continue;
        }

        //trigger next
        #ifdef __log_communicate_queue__
            av_log(NULL,AV_LOG_ERROR,"    trigger deblock mbrow=%d--%d start.\n",p_mcintrapred->start_row,p_mcintrapred->end_row);
        #endif

        //P B sent whole picture
        ambadec_assert_ffmpeg(p_mcintrapred->start_row==0);
        ambadec_assert_ffmpeg(p_mcintrapred->end_row==p_picdata->mb_height);

        p_node= thiz->p_deblock_dataq->get_free(thiz->p_deblock_dataq);
        if(!p_node->p_ctx)
        {
            p_node->p_ctx=av_malloc(sizeof(process_data_t));
        }
        p_deblock=p_node->p_ctx;
        *p_deblock= *p_mcintrapred;

        #ifdef __log_parallel_control__
        av_log(NULL,AV_LOG_ERROR,"   mcintrapred thread output: p_deblock->start_row=%d, p_deblock->end_row=%d.\n",p_deblock->start_row,p_deblock->end_row);
        #endif

        thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,0);

        //release data
        thiz->p_mcintrapred_dataq->release(thiz->p_mcintrapred_dataq,p_mcintrapred,0);
    }

mcintrapred_thread_exit:

    #ifdef __log_parallel_control__
        av_log(NULL,AV_LOG_ERROR,"**** mcintrapred_thread, push deblock thread exit and wait deblock exit finished.\n");
    #endif

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"mcintrapred_thread exit.\n");
    #endif

//    thiz->p_thread_exit->enqueue(thiz->p_thread_exit,NULL);
    //#ifdef __log_parallel_control__
    av_log(NULL,AV_LOG_ERROR,"mcintrapred_thread exit end.\n");
    //#endif
    thiz->mc_intrapred_loop=0;
    thiz->idct_mc_need_flush = 0;
    pthread_exit(NULL);
    return NULL;
}

void* thread_rv34_amba_deblock(void* p)
{
    RV34AmbaDecContext *thiz=(RV34AmbaDecContext *)p;
    process_data_t* p_deblock_data;
    ctx_nodef_t* p_node;
    int i=0;

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"deblock_thread start.\n");
    #endif

    while(thiz->deblock_loop)
    {
        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Waiting: deblock_thread input: wait on p_deblock_dataq.\n");
        #endif

        #ifdef __log_communicate_queue__
            ambadec_print_triqueue_general_status(thiz->p_deblock_dataq);
        #endif

        //av_log(NULL,AV_LOG_ERROR,"1. mb_height =%d, sync counter = %u.\n", thiz->picdata[0].mb_height, *thiz->pAcc->p_sync_counter);
        //get data
        p_node=thiz->p_deblock_dataq->get(thiz->p_deblock_dataq);

        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"Escape: deblock_thread escape input: from p_deblock_dataq,p_node->p_ctx=%p.\n",p_node->p_ctx);
        #endif

        #ifdef __log_communicate_queue__
            ambadec_print_triqueue_detailed_status(thiz->p_deblock_dataq);
        #endif

        p_deblock_data=p_node->p_ctx;

        //flush
        if (thiz->deblock_need_flush || thiz->error_wait_flush) {
            if (!p_deblock_data) {
                if (p_node->flag == _flag_cmd_flush_) {
                    //send flush frame
                    p_node=thiz->p_frame_pool->get_cmd(thiz->p_frame_pool);
                    av_log(NULL,AV_LOG_ERROR," **send from deblock p_node %p.\n", p_node);
                    thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,_flag_cmd_flush_);
                    thiz->deblock_need_flush = 0;
                    av_log(NULL,AV_LOG_ERROR," **Flush deblock_thread done.\n");
                    continue;
                } else {
                    av_log(NULL,AV_LOG_ERROR,"must not come here 4.0.\n");
                    ambadec_assert_ffmpeg(0);
                }
            } else {
                ambadec_assert_ffmpeg(p_node->flag == 0);
                //if (p_deblock_data->end_row == thiz->s.mb_height) {
                //    flush_holded_pictures(thiz, p_deblock_data->p_picdata);
                //    thiz->p_pic_dataq->release_toready(thiz->p_pic_dataq,p_deblock_data->p_picdata,0);
                //}
                av_log(NULL,AV_LOG_ERROR," **Flushing deblock_thread, p_deblock_dataq status:.\n");
                //#ifdef __log_communicate_queue__
                    //ambadec_print_triqueue_general_status(thiz->p_deblock_dataq);
                    //ambadec_print_triqueue_detailed_status(thiz->p_deblock_dataq);
                //#endif
                thiz->p_deblock_dataq->release(thiz->p_deblock_dataq, p_deblock_data, 0);
                continue;
            }
        }

        if(!p_deblock_data)
        {
            if(p_node->flag==_flag_cmd_exit_next_)
            {
                //#ifdef __log_decoding_process__
                    av_log(NULL,AV_LOG_ERROR,"deblock_thread get NULL data(_flag_cmd_exit_next_), start exit.\n");
                //#endif
                #ifdef __log_parallel_control__
                    av_log(NULL,AV_LOG_ERROR,"**** deblock_thread, get exit cmd and start exit.\n");
                #endif
                goto deblock_thread_exit;
            }
            else
            {
                av_log(NULL, AV_LOG_ERROR, "Error data indicater in thread_rv34_amba_deblock\n");
                continue;
            }
        }

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"**** deblock_thread get p_node, start a loop %d--%d.\n",p_deblock_data->start_row,p_deblock_data->end_row);
        #endif

        #ifdef __log_decoding_config__
            if(log_mask[decoding_config_deblock])
        #endif
        {
            for(i=p_deblock_data->start_row;i<p_deblock_data->end_row;i++)
            {
                //av_log(NULL,AV_LOG_ERROR,"2. mb_height =%d, sync counter = %u.\n", p_deblock_data->p_picdata->mb_height, *thiz->pAcc->p_sync_counter);
                thiz->loop_filter(thiz,p_deblock_data->p_picdata,i);
            }
        }

        //if decoding done
        if(p_deblock_data->end_row==p_deblock_data->p_picdata->mb_height)
        {
            #ifdef __log_decoding_process__
                av_log(NULL,AV_LOG_ERROR,"decoding picture done, thiz->pic_pool->used_cnt=%d,thiz->pic_pool->free_cnt=%d .\n",thiz->pic_pool->used_cnt,thiz->pic_pool->free_cnt);
            #endif

#if 0
            #ifdef __dump_DSP_test_data__
            log_dump_p(log_fd_residual,p_deblock_data->p_picdata->pdct,p_deblock_data->p_picdata->mb_height*p_deblock_data->p_picdata->mb_width*768,p_deblock_data->p_picdata->frame_cnt);
            log_dump_p(log_fd_mvdsp,p_deblock_data->p_picdata->pinfo,p_deblock_data->p_picdata->mb_height*p_deblock_data->p_picdata->mb_width*sizeof(RVDEC_MBINFO_t),p_deblock_data->p_picdata->frame_cnt);
            #endif
#endif

            pthread_mutex_lock(&thiz->mutex);
            thiz->decoding_frame_cnt--;
            pthread_mutex_unlock(&thiz->mutex);

#ifdef __dump_DSP_test_data__
            _dump_picture_data(p_deblock_data->p_picdata, p_deblock_data->p_picdata->current_picture_ptr->pict_type==FF_B_TYPE);
            log_closefile_p(log_fd_mbtype, p_deblock_data->p_picdata->frame_cnt);
#endif

#ifdef __dump_yuv420p__
            if (1 || p_deblock_data->p_picdata->current_picture_ptr->pict_type==FF_B_TYPE) {
                dump_yuv_data("framedata", (void*)p_deblock_data->p_picdata->current_picture_ptr, (unsigned int)p_deblock_data->p_picdata->width, (unsigned int)p_deblock_data->p_picdata->height, 1, 0, p_deblock_data->p_picdata->frame_cnt);
            } else if (p_deblock_data->p_picdata->last_picture_ptr) {
                dump_yuv_data("framedata", (void*)p_deblock_data->p_picdata->last_picture_ptr, (unsigned int)p_deblock_data->p_picdata->width, (unsigned int)p_deblock_data->p_picdata->height, 1, 0, p_deblock_data->p_picdata->frame_cnt);
            }
#endif
#ifdef __dump_yuv420_nv12__
            if (1 || p_deblock_data->p_picdata->current_picture_ptr->pict_type==FF_B_TYPE) {
                dump_yuv_data("framedata", (void*)p_deblock_data->p_picdata->current_picture_ptr, (unsigned int)p_deblock_data->p_picdata->width, (unsigned int)p_deblock_data->p_picdata->height, 1, 1, p_deblock_data->p_picdata->frame_cnt);
            } else if (p_deblock_data->p_picdata->last_picture_ptr) {
                dump_yuv_data("framedata", (void*)p_deblock_data->p_picdata->last_picture_ptr, (unsigned int)p_deblock_data->p_picdata->width, (unsigned int)p_deblock_data->p_picdata->height, 1, 1, p_deblock_data->p_picdata->frame_cnt);
            }
#endif
#ifdef __dump_yuv420_mb__
            if (1 || p_deblock_data->p_picdata->current_picture_ptr->pict_type==FF_B_TYPE) {
                dump_yuv_data_mb("framemb", (void*)p_deblock_data->p_picdata->current_picture_ptr, (unsigned int)p_deblock_data->p_picdata->width, (unsigned int)p_deblock_data->p_picdata->height, 1, p_deblock_data->p_picdata->frame_cnt);
            } else if (p_deblock_data->p_picdata->last_picture_ptr) {
                dump_yuv_data_mb("framemb", (void*)p_deblock_data->p_picdata->last_picture_ptr, (unsigned int)p_deblock_data->p_picdata->width, (unsigned int)p_deblock_data->p_picdata->height, 1, p_deblock_data->p_picdata->frame_cnt);
            }
#endif

#ifdef __dump_DSP_TXT__
log_closefile_p(log_fd_dsp_text, p_deblock_data->p_picdata->frame_cnt);
#endif
#ifdef __dump_deblock_DSP_TXT__
log_closefile_p(log_fd_dsp_deblock_text, p_deblock_data->p_picdata->frame_cnt);
#endif
#ifdef __dump_mc_dct_DSP_TXT__
log_closefile_p(log_fd_dsp_mc_dct_text,p_deblock_data->p_picdata->frame_cnt);
#endif

            if(p_deblock_data->p_picdata->current_picture_ptr->pict_type==FF_B_TYPE)
            {
                thiz->pic_pool->inc_lock(thiz->pic_pool,p_deblock_data->p_picdata->current_picture_ptr);
                p_node=thiz->p_frame_pool->get_free(thiz->p_frame_pool);
                p_node->p_ctx=p_deblock_data->p_picdata->current_picture_ptr;
                #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"   deblock thread output: send picture=%p.\n",p_deblock_data->p_picdata->current_picture_ptr);
                #endif
                thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
            }
            else
            {
                thiz->pic_pool->inc_lock(thiz->pic_pool,p_deblock_data->p_picdata->last_picture_ptr);
                p_node=thiz->p_frame_pool->get_free(thiz->p_frame_pool);
                p_node->p_ctx=p_deblock_data->p_picdata->last_picture_ptr;
                #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"   deblock thread output: send picture=%p.\n",p_deblock_data->p_picdata->last_picture_ptr);
                #endif
                thiz->p_frame_pool->put_ready(thiz->p_frame_pool,p_node,0);
            }

            #ifdef __log_dump_idct_data__
                log_closefile_p(log_fd_idct_y,p_deblock_data->p_picdata->frame_cnt);
                log_closefile_p(log_fd_idct_uv,p_deblock_data->p_picdata->frame_cnt);
                log_closefile_p(log_fd_idct_text,p_deblock_data->p_picdata->frame_cnt);
            #endif
            #ifdef __log_intrapred__
                log_closefile_p(log_fd_intrapred,p_deblock_data->p_picdata->frame_cnt);
            #endif
            #ifdef __log_mc__
                log_closefile_p(log_fd_mc,p_deblock_data->p_picdata->frame_cnt);
            #endif
            #ifdef __log_mv_new__
                log_closefile_p(log_fd_mv_new,p_deblock_data->p_picdata->frame_cnt);
            #endif
            #ifdef __log_mv__
                log_closefile_p(log_fd_mv,p_deblock_data->p_picdata->frame_cnt);
            #endif

            pthread_mutex_lock(&thiz->mutex);
            if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_deblock_data->p_picdata->last_picture_ptr)==1)
                thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_deblock_data->p_picdata->last_picture_ptr);
            if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_deblock_data->p_picdata->current_picture_ptr)==1)
                thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_deblock_data->p_picdata->current_picture_ptr);
            if(thiz->pic_pool->dec_lock(thiz->pic_pool,p_deblock_data->p_picdata->next_picture_ptr)==1)
                thiz->s.avctx->release_buffer(thiz->s.avctx,(AVFrame*)p_deblock_data->p_picdata->next_picture_ptr);

//        av_log(NULL,AV_LOG_ERROR,"  before deblock thread release p_deblock_data->p_picdata=%p,thiz->p_pic_dataq=%p.\n",p_deblock_data->p_picdata,thiz->p_pic_dataq);
//        thiz->p_pic_dataq->release_toready(thiz->p_pic_dataq,p_deblock_data->p_picdata,0);


        #ifdef __log_parallel_control__
            av_log(NULL,AV_LOG_ERROR,"   deblock thread release p_deblock_data->p_picdata=%p.\n",p_deblock_data->p_picdata);
        #endif
        p_deblock_data->p_picdata->last_picture_ptr = NULL;
        p_deblock_data->p_picdata->next_picture_ptr = NULL;
        p_deblock_data->p_picdata->current_picture_ptr = NULL;
        thiz->p_pic_dataq->release_toready(thiz->p_pic_dataq,p_deblock_data->p_picdata,0);
        pthread_mutex_unlock(&thiz->mutex);
        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR,"!!!!frame done: after release p_deblock_data->p_picdata, thiz->pic_pool->used_cnt=%d,thiz->pic_pool->free_cnt=%d .\n",thiz->pic_pool->used_cnt,thiz->pic_pool->free_cnt);
        #endif

        }

        thiz->p_deblock_dataq->release(thiz->p_deblock_dataq,p_deblock_data,0);
    }

deblock_thread_exit:

    //#ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"deblock_thread exit.\n");
    //#endif

    //remove conditional waiting on thiz->p_pic_dataq(vld thread)
//    thiz->p_pic_dataq->release_toready(thiz->p_pic_dataq,thiz->p_pic,0);

//    thiz->deblock_loop=0;


//    thiz->p_thread_exit->enqueue(thiz->p_thread_exit,NULL);
    #ifdef __log_parallel_control__
    av_log(NULL,AV_LOG_ERROR,"deblock_thread exit end.\n");
    #endif
    thiz->deblock_loop=0;
    thiz->deblock_need_flush = 0;
    pthread_exit(NULL);
    return NULL;
}

int ffnew_rv34_amba_decode_frame(AVCodecContext *avctx,void *data, int *data_size,AVPacket *avpkt)
{
    RV34AmbaDecContext *r = avctx->priv_data;
    AVFrame *pict = data;
    AVFrame* pready;
    ctx_nodef_t* p_node;
    vld_data_t* p_data;
//    int must_get_pic=0;

    if (r->error_wait_flush) {
        *data_size = 0;
        return -1;
    }

    //send to vld sub thread
    if (avpkt->size)
    {
        if((p_node=r->p_vld_dataq->get_free(r->p_vld_dataq)))
        {
            p_data=p_node->p_ctx;
        }
        else
        {
            p_node=av_malloc(sizeof(ctx_nodef_t));
            p_node->p_ctx=NULL;
        }
        ambadec_assert_ffmpeg(p_node);
        if(!p_node->p_ctx)
        {
            p_node->p_ctx=av_malloc(sizeof(vld_data_t));
            ((vld_data_t*)p_node->p_ctx)->slice_offset=NULL;
            //memset(p_node->p_ctx,0,sizeof(vld_data_t));
        }
        p_data=p_node->p_ctx;

        #ifdef __log_decoding_process__
            av_log(NULL,AV_LOG_ERROR," in decode_frame: avctx->slice_count=%d, avctx->slice_offset=%p, p_data->slice_offset=%p .\n",avctx->slice_count,avctx->slice_offset,p_data->slice_offset);
        #endif
        p_data->slice_cnt=avctx->slice_count;
        if(p_data->slice_cnt)
        {
//            ambadec_assert_ffmpeg(avctx->slice_offset);
/*            if(p_data->slice_offset) {
                av_free(p_data->slice_offset);
                p_data->slice_offset = NULL;
            }*/
            p_data->slice_offset=av_realloc(p_data->slice_offset, (p_data->slice_cnt + 1)*sizeof(int));
            memcpy(p_data->slice_offset,avctx->slice_offset,p_data->slice_cnt*sizeof(int));
        }
        else
        {
            p_data->slice_cnt=0;
        }

#if 0
        p_data->size=avpkt->size;
        p_data->pbuf = av_malloc(p_data->size);
        memcpy(p_data->pbuf, avpkt->data, p_data->size);
#else
        p_data->pbuf=avpkt->data;
        p_data->size=avpkt->size;
        avpkt->data=NULL;//trick here: prevent AP release it
#endif

        p_data->pts = avpkt->pts;
//        p_data->skip_frame=avctx->skip_frame;
//#if FF_API_HURRY_UP
//        p_data->hurry_up=avctx->hurry_up;
//#endif

        r->p_vld_dataq->put_ready(r->p_vld_dataq,p_node,0);

        //get frame
        if(r->p_frame_pool->ready_cnt || r->p_vld_dataq->ready_cnt>3)
        {
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"Waiting: decode frame wait on p_frame_pool, r->p_frame_pool->ready_cnt=%d,r->p_vld_dataq->ready_cnt=%d.\n",r->p_frame_pool->ready_cnt,r->p_vld_dataq->ready_cnt);
            #endif

            p_node=r->p_frame_pool->get(r->p_frame_pool);

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
            pthread_mutex_lock(&r->mutex);
            if(r->pic_pool->dec_lock(r->pic_pool,pready)==1)
                avctx->release_buffer(avctx,pready);
            ambadec_assert_ffmpeg(((Picture*)pready)->ref_added_for_extern == 1);
            ((Picture*)pready)->ref_added_for_extern = 0;
            pthread_mutex_unlock(&r->mutex);
            *data_size=sizeof(AVFrame);
            return avpkt->size;
        }
    }
    else
    {
        //special case for last frames
        if(r->p_frame_pool->ready_cnt || r->p_vld_dataq->ready_cnt || r->decoding_frame_cnt )
        {
            #ifdef __log_parallel_control__
                av_log(NULL,AV_LOG_ERROR,"Waiting: decode frame wait on p_frame_pool(last frames), r->p_frame_pool->ready_cnt=%d,r->p_vld_dataq->ready_cnt=%d.\n",r->p_frame_pool->ready_cnt,r->p_vld_dataq->ready_cnt);
            #endif

            p_node=r->p_frame_pool->get(r->p_frame_pool);

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
                return (r->p_frame_pool->ready_cnt+r->p_vld_dataq->ready_cnt+r->decoding_frame_cnt+1);
            }
            *pict=*pready;
            pthread_mutex_lock(&r->mutex);
            if(r->pic_pool->dec_lock(r->pic_pool,pready)==1)
                avctx->release_buffer(avctx,pready);

            ambadec_assert_ffmpeg(((Picture*)pready)->ref_added_for_extern == 1);
            ((Picture*)pready)->ref_added_for_extern = 0;
            pthread_mutex_unlock(&r->mutex);

            *data_size=sizeof(AVFrame);
            return (r->p_frame_pool->ready_cnt+r->p_vld_dataq->ready_cnt+r->decoding_frame_cnt+1);
        }
        else if(r->s.next_picture_ptr)// get next reference frame
        {
            *pict=*((AVFrame*)(r->s.next_picture_ptr));
            *data_size=sizeof(AVFrame);
            pthread_mutex_lock(&r->mutex);
            ambadec_assert_ffmpeg(r->s.next_picture_ptr->ref_added_for_extern == 1);
            r->s.next_picture_ptr->ref_added_for_extern = 0;
            pthread_mutex_unlock(&r->mutex);
            return 0;
        }
    }

    *data_size=0;

    return 0;
}



av_cold int ffnew_rv34_amba_decode_end(AVCodecContext *avctx)
{
#ifdef __log_dump_decocer_input_data__
    log_closefile(log_fd_temp);
#endif
    RV34AmbaDecContext *thiz = avctx->priv_data;
//    int i=0,j=0;
    ctx_nodef_t* p_node;
    void* pv;
    Picture *pic;
    int ret=0;
    int i = 0;

#ifdef _intra_using_allocate_residue_
    //release i's residue buffer
    for (i=0; i<=thiz->i_tot_index; i++){
        if (thiz->i_residue[i]) {
            av_free(thiz->i_residue[i]);
            thiz->i_residue[i] = NULL;
        }
    }
#endif

#if 0
//    #ifdef __log_parallel_control__
//    av_log(NULL,AV_LOG_ERROR,"****end, start quit sub threads.\n");
//    av_log(NULL,AV_LOG_ERROR,"****end, start quit sub threads:r->p_thread_exit->current_cnt=%d.\n",r->p_thread_exit->current_cnt);
//    #endif
    thiz->start_exit = 1;
    //quit sub process thread
//    thiz->vld_loop=thiz->mc_intrapred_loop=thiz->deblock_loop=0;
    p_node=r->p_vld_dataq->get_cmd(r->p_vld_dataq);
    thiz->p_vld_dataq->put_ready(thiz->p_vld_dataq,p_node,_flag_cmd_exit_next_);
    //remove vld thread might be blocked by r->p_pic_dataq
    p_node=r->p_pic_dataq->get_cmd(r->p_pic_dataq);
    p_node->p_ctx=NULL;
    thiz->p_pic_dataq->put_ready(thiz->p_pic_dataq,p_node,0);
/*    p_node=r->p_mcintrapred_dataq->get_cmd(r->p_mcintrapred_dataq);
    thiz->p_mcintrapred_dataq->put_ready(thiz->p_mcintrapred_dataq,p_node,_flag_cmd_exit_next_);
    p_node=r->p_deblock_dataq->get_cmd(r->p_deblock_dataq);
    thiz->p_deblock_dataq->put_ready(thiz->p_deblock_dataq,p_node,_flag_cmd_exit_next_);

    thiz->p_thread_exit->dequeue(thiz->p_thread_exit);
    thiz->p_thread_exit->dequeue(thiz->p_thread_exit);
    thiz->p_thread_exit->dequeue(thiz->p_thread_exit);

//    #ifdef __log_parallel_control__
//    av_log(NULL,AV_LOG_ERROR,"****end, sub threads quits:r->p_thread_exit->current_cnt=%d.\n",r->p_thread_exit->current_cnt);
//    #endif
*/
    ret=pthread_join(r->tid_vld,&pv);
//    #ifdef __log_parallel_control__
//    av_log(NULL,AV_LOG_ERROR,"****end, vld sub thread join.\n");
    if(ret)
    {
        av_log(NULL,AV_LOG_ERROR,"**** error here, ret=%d.\n",ret);
        av_log(NULL,AV_LOG_ERROR," error num, EINVAL=%d,ESRCH=%d,EDEADLK=%d.\n",EINVAL,ESRCH,EDEADLK);
    }
//    #endif

#else
    thiz->vld_need_flush = 1;
    thiz->idct_mc_need_flush = 1;
    thiz->deblock_need_flush = 1;

    av_log(NULL,AV_LOG_ERROR,"** Flush pipeline 1.\n");

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
        pic = (Picture*)p_node->p_ctx;
        if (pic && p_node->flag != _flag_cmd_flush_) {
            flush_pictrue(thiz, pic);
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

    if (thiz->s.last_picture_ptr) {
        flush_pictrue(thiz, thiz->s.last_picture_ptr);
        thiz->s.last_picture_ptr = NULL;
    }
    if (thiz->s.current_picture_ptr) {
        flush_pictrue(thiz, thiz->s.current_picture_ptr);
        thiz->s.last_picture_ptr = NULL;
    }
    if (thiz->s.next_picture_ptr) {
        flush_pictrue(thiz, thiz->s.next_picture_ptr);
        thiz->s.next_picture_ptr = NULL;
    }

    //reset pic_data_q
    ambadec_reset_triqueue(thiz->p_pic_dataq);
#endif

    #ifdef __log_picinfo__
        log_closefile(log_fd_picinfo_text);
    #endif

#ifdef __dump_DSP_test_data__
        log_closefile(log_fd_general_txt);
#endif

#ifdef __dump_whole__
    #ifdef __dump_DSP_test_data__
        #ifdef __dump_binary__
        log_closefile(log_fd_residual);
        log_closefile(log_fd_residual_2);
        log_closefile(log_fd_mvdsp);
        log_closefile(log_fd_reffyraw);
        log_closefile(log_fd_reffuvraw);
        log_closefile(log_fd_reffytiled);
        log_closefile(log_fd_reffuvtiled);
        log_closefile(log_fd_refbyraw);
        log_closefile(log_fd_refbuvraw);
        log_closefile(log_fd_refbytiled);
        log_closefile(log_fd_refbuvtiled);
        log_closefile(log_fd_result);
        #endif
    #endif
#endif

    thiz->start_exit = 1;
    p_node=thiz->p_vld_dataq->get_cmd(thiz->p_vld_dataq);
    thiz->p_vld_dataq->put_ready(thiz->p_vld_dataq,p_node,_flag_cmd_exit_next_);
    //remove vld thread might be blocked by r->p_pic_dataq
    p_node=thiz->p_pic_dataq->get_cmd(thiz->p_pic_dataq);
    p_node->p_ctx=NULL;
    thiz->p_pic_dataq->put_ready(thiz->p_pic_dataq,p_node,0);
    ret=pthread_join(thiz->tid_vld,&pv);
    if(ret)
    {
        av_log(NULL,AV_LOG_ERROR,"**** error here, ret=%d.\n",ret);
        av_log(NULL,AV_LOG_ERROR," error num, EINVAL=%d,ESRCH=%d,EDEADLK=%d.\n",EINVAL,ESRCH,EDEADLK);
    }

    //av_log(NULL,AV_LOG_ERROR,"before 1.\n");
    ambadec_destroy_triqueue(thiz->p_vld_dataq);
    //av_log(NULL,AV_LOG_ERROR,"before 2.\n");
    ambadec_destroy_triqueue(thiz->p_mcintrapred_dataq);
    //av_log(NULL,AV_LOG_ERROR,"before 3.\n");
    ambadec_destroy_triqueue(thiz->p_deblock_dataq);
    //av_log(NULL,AV_LOG_ERROR,"before 4.\n");
//    ambadec_destroy_queue(r->p_thread_exit);
    ambadec_destroy_triqueue(thiz->p_pic_dataq);
    av_log(NULL,AV_LOG_ERROR,"before 5.\n");
    ret = 2;
    if (thiz->use_dsp) {
        ret = thiz->pAcc->amba_buffer_number;
    }
    for(i=0; i<ret; i++)
    {
        _callback_destroy_pic_data(&thiz->picdata[i]);
    }

    //av_log(NULL,AV_LOG_ERROR,"before 6.\n");
    ambadec_destroy_pool(thiz->pic_pool);
    //av_log(NULL,AV_LOG_ERROR,"before 7.\n");
    MPV_common_end(&thiz->s);
    //av_log(NULL,AV_LOG_ERROR,"before 8.\n");
    av_freep(&thiz->intra_types_hist);
    thiz->intra_types = NULL;
    av_freep(&thiz->mb_type);
    pthread_mutex_destroy(&thiz->mutex);

    //av_log(NULL,AV_LOG_ERROR,"before 9.\n");
//    av_log(NULL,AV_LOG_ERROR,"****end, success.\n");

    return 0;
}

/**
 * Initialize decoder.
 */
av_cold int ffnew_rv34_amba_decode_init(AVCodecContext *avctx)
{
    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"**ffnew_rv34_amba_decode_init start.\n");
    #endif
    RV34AmbaDecContext *r = avctx->priv_data;
    MpegEncContext *s = &r->s;
    int i=0;
    ctx_nodef_t* p_node;

    MPV_decode_defaults(s);
    s->avctx= avctx;
    s->out_format = FMT_H263;
    s->codec_id= avctx->codec_id;

    s->width = avctx->width;
    s->height = avctx->height;

    r->s.avctx = avctx;
    avctx->flags |= CODEC_FLAG_EMU_EDGE;
    r->s.flags |= CODEC_FLAG_EMU_EDGE;
    avctx->pix_fmt = PIX_FMT_NV12;

    avctx->has_b_frames = 1;
    s->low_delay = 0;

    if (MPV_common_init(s) < 0)
        return -1;

    ff_h264_pred_init(&r->h, CODEC_ID_RV40, 8);

        //check if there's extern accelerator
    if (avctx->p_extern_accelerator && avctx->extern_accelerator_type == accelerator_type_amba_hybirdrv40_mc) {
        ambadec_assert_ffmpeg(avctx->acceleration);
        ambadec_assert_ffmpeg(avctx->delete_accelerator);
        r->pAcc = (amba_decoding_accelerator_t*) avctx->p_extern_accelerator;
        ambadec_assert_ffmpeg(r->pAcc->decode_type == rv40_hybird);//rv40
        r->use_dsp = 1;
    } else {
        r->pAcc = (amba_decoding_accelerator_t*) avctx->p_extern_accelerator;
        r->use_dsp = 0;
    }

#if PLATFORM_LINUX
#else
//#if HAVE_NEON
    if(r->pAcc != NULL){
             r->neon = r->pAcc->rv40_accel;
             rv40_replace_pred(&r->neon, &r->h);
    }
//#endif
#endif

    r->intra_types_stride = 4*s->mb_stride + 4;
    r->intra_types_hist = av_malloc(r->intra_types_stride * 4 * 2 * sizeof(*r->intra_types_hist));
    r->intra_types = r->intra_types_hist + r->intra_types_stride * 4;

    r->mb_type = av_mallocz(r->s.mb_stride * r->s.mb_height * sizeof(*r->mb_type));


    if(!intra_vlcs[0].cbppattern[0].bits)
        rv34_amba_init_tables();

    //parallel related
//    r->p_thread_exit=ambadec_create_queue(3);
    r->vld_loop=r->mc_intrapred_loop=r->deblock_loop=1;

    r->p_vld_dataq=ambadec_create_triqueue(_callback_free_vld_data);
    r->p_mcintrapred_dataq=ambadec_create_triqueue(_callback_free);
    r->p_deblock_dataq=ambadec_create_triqueue(_callback_free);

    r->p_frame_pool=ambadec_create_triqueue(_callback_NULL);

    r->p_pic_dataq=ambadec_create_triqueue(_callback_NULL);


    r->pic_pool=ambadec_create_pool();
    for(i=0;i<6;i++)
    {
        r->pic_pool->put(r->pic_pool,&s->picture[i]);
        ff_alloc_picture_amba(s, &s->picture[i]);
    }

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"**ffnew_rv34_amba_decode_init: request common resource done, avctx->p_extern_accelerator %p, avctx->extern_accelerator_type %d.\n", avctx->p_extern_accelerator, avctx->extern_accelerator_type);
    #endif


    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"**ffnew_rv34_amba_decode_init: r->use_dsp %d.\n", r->use_dsp);
    #endif

    if (r->use_dsp) {

#ifdef _intra_using_allocate_residue_
        //circlar
        r->pb_current_index = 0;
        r->pb_tot_index = r->pAcc->amba_buffer_number - 1;
        r->i_current_index = 0;
        r->i_tot_index = max_i_number - 1;
        r->result_current_index = 0;
        r->result_tot_index = r->pAcc->amba_buffer_number - 1;

        //allocate i's residue buffer
        for (i=0; i<=r->i_tot_index; i++){
            r->i_residue[i] = av_malloc(r->pAcc->amba_idct_buffer_size);
            if (!r->i_residue[i]) {
                av_log(NULL, AV_LOG_ERROR, "error!!!not enough memory when allocate intra picture's residue buffer.\n");
                return -2;
            }
        }
#endif
        av_log(NULL,AV_LOG_ERROR,"****amba_buffer_number %d.\n", r->pAcc->amba_buffer_number);
        ambadec_assert_ffmpeg(r->pAcc->amba_buffer_number);
        for(i=0; i<r->pAcc->amba_buffer_number; i++)
        {
            r->picdata[i].pbase = NULL;

            #ifdef _tmp_rv_dsp_setting_
                r->picdata[i].pintra=av_malloc(s->mb_width*s->mb_height*sizeof(RVDEC_INTRAPRED_t));
                r->picdata[i].pmbtype=av_malloc(s->mb_width*s->mb_height*sizeof(int));
            #else
                r->picdata[i].pintra=(RVDEC_INTRAPRED_t*)r->picdata[i].pinfo;
            #endif

            r->picdata[i].pdctbase = NULL;
            r->picdata[i].picinfo.decoder_id = r->pAcc->decode_id;
            r->picdata[i].picinfo.udec_type = r->pAcc->decode_type;//rv40
            ambadec_assert_ffmpeg(r->pAcc->decode_type == rv40_hybird);
            r->picdata[i].picinfo.num_pics = 1;
            r->picdata[i].picinfo.uu.rv40.tiled_mode = 0;//raster scan mode

#ifdef _intra_using_allocate_residue_
            r->pb_mv[i] = r->pAcc->p_amba_mv_ringbuffer + r->pAcc->amba_mv_buffer_size * i;
            r->pb_residue[i] = r->pAcc->p_amba_idct_ringbuffer + r->pAcc->amba_idct_buffer_size * i;
#else
            r->picdata[i].pmc_result = r->pAcc->p_mcresult_buffer[i];
            r->picdata[i].pmc_result_buffer_id = r->pAcc->mcresult_buffer_id[i];
            av_log(NULL, AV_LOG_ERROR, "**reserved for mc result buffer: %p, %d.\n", r->picdata[i].pmc_result, r->picdata[i].pmc_result_buffer_id);

            r->picdata[i].pinfo = (RVDEC_MBINFO_t*)(r->pAcc->p_amba_mv_ringbuffer + r->pAcc->amba_mv_buffer_size * i);

#ifdef _d_use_new_residue_layout_
            r->picdata[i].pdct=(DCTELEM*)(r->pAcc->p_amba_idct_ringbuffer + (r->pAcc->amba_idct_buffer_size + r->pAcc->amba_picture_width*48) * i);

//debug only
#ifdef _d_debug_new_layout_
            r->picdata[i].pdct += s->mb_width*8*8*6;
            r->picdata[i].picinfo.uu.rv40.residual_daddr_start = r->picdata[i].pdct;
            r->picdata[i].picinfo.uu.rv40.residual_daddr_end = r->picdata[i].picinfo.uu.rv40.residual_daddr_start + r->pAcc->amba_idct_buffer_size;
#else
            r->picdata[i].picinfo.uu.rv40.residual_daddr_start = (unsigned char *)r->picdata[i].pdct;
            r->picdata[i].picinfo.uu.rv40.residual_daddr_end = r->picdata[i].picinfo.uu.rv40.residual_daddr_start + r->pAcc->amba_idct_buffer_size;
            r->picdata[i].pdct += s->mb_width*8*8*6;
#endif

#else
            r->picdata[i].pdct=(DCTELEM*)(r->pAcc->p_amba_idct_ringbuffer + (r->pAcc->amba_idct_buffer_size + r->pAcc->amba_picture_width*48) * i);
            r->picdata[i].pdct += s->mb_width*8*8*6;
            r->picdata[i].picinfo.uu.rv40.residual_daddr_start = r->picdata[i].pdct;
            r->picdata[i].picinfo.uu.rv40.residual_daddr_end = r->picdata[i].picinfo.uu.rv40.residual_daddr_start + r->pAcc->amba_idct_buffer_size;
#endif

            //constant picinfo settings
            r->picdata[i].picinfo.uu.rv40.mv_daddr_start = (unsigned char *)r->picdata[i].pinfo;
            r->picdata[i].picinfo.uu.rv40.mv_daddr_end = r->picdata[i].picinfo.uu.rv40.mv_daddr_start + r->pAcc->amba_mv_buffer_size;
#endif
            r->picdata[i].picinfo.uu.rv40.target_fb_id = r->pAcc->mcresult_buffer_id[i];

            r->picdata[i].cbp_luma   = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->picdata[i].cbp_luma));
            r->picdata[i].cbp_chroma = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->picdata[i].cbp_chroma));
            r->picdata[i].deblock_coefs = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->picdata[i].deblock_coefs));

            p_node=r->p_pic_dataq->get_free(r->p_pic_dataq);
            p_node->p_ctx=&r->picdata[i];
            r->p_pic_dataq->put_ready(r->p_pic_dataq,p_node,0);
        }
#ifdef _intra_using_allocate_residue_
        ambadec_assert_ffmpeg(i == r->pAcc->amba_buffer_number);
        ambadec_assert_ffmpeg(i < max_pb_number);
        r->pb_mv[i] = r->pAcc->p_amba_mv_ringbuffer + r->pAcc->amba_mv_buffer_size * i;
        r->pb_residue[i] = r->pAcc->p_amba_idct_ringbuffer + r->pAcc->amba_idct_buffer_size * i;
#endif
    }else {
        //resource alloc
        for(i=0;i<2;i++)
        {
            r->picdata[i].pbase=av_malloc(s->mb_width*s->mb_height*sizeof(RVDEC_MBINFO_t)+15);
            r->picdata[i].pinfo=(RVDEC_MBINFO_t*)(((unsigned long)(r->picdata[i].pbase)+15)&(~0xf));

            #ifdef _tmp_rv_dsp_setting_
                r->picdata[i].pintra=av_malloc(s->mb_width*s->mb_height*sizeof(RVDEC_INTRAPRED_t));
                r->picdata[i].pmbtype=av_malloc(s->mb_width*s->mb_height*sizeof(int));
            #else
                r->picdata[i].pintra=(RVDEC_INTRAPRED_t*)r->picdata[i].pinfo;
            #endif

#ifdef _d_use_new_residue_layout_
            r->picdata[i].pdctbase=av_malloc(sizeof(DCTELEM)*s->mb_width*(s->mb_height +1)*(256+128)+15);
            r->picdata[i].pdct=(DCTELEM*)(((unsigned long)(r->picdata[i].pdctbase)+15)&(~0xf));
            r->picdata[i].pdct += s->mb_width*8*8*6;
#else
            r->picdata[i].pdctbase=av_malloc(sizeof(DCTELEM)*s->mb_width*s->mb_height*(256+128)+15);
            r->picdata[i].pdct=(DCTELEM*)(((unsigned long)(r->picdata[i].pdctbase)+15)&(~0xf));
#endif

    //        r->picdata[i].pdctu=r->picdata[i].pdct+s->mb_width*s->mb_height*256;
    //        r->picdata[i].pdctv=r->picdata[i].pdctu+s->mb_width*s->mb_height*64;

            r->picdata[i].cbp_luma   = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->picdata[i].cbp_luma));
            r->picdata[i].cbp_chroma = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->picdata[i].cbp_chroma));
            r->picdata[i].deblock_coefs = av_malloc(s->mb_stride * s->mb_height * sizeof(*r->picdata[i].deblock_coefs));

            p_node=r->p_pic_dataq->get_free(r->p_pic_dataq);
            p_node->p_ctx=&r->picdata[i];
            r->p_pic_dataq->put_ready(r->p_pic_dataq,p_node,0);
        }
    }

    r->vld_sent_row=r->mc_sent_row=0;
    r->pic_finished=1;
    r->previous_pic_is_b=1;
    r->p_pic=NULL;
    pthread_mutex_init(&r->mutex,NULL);

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"**ffnew_rv34_amba_decode_init: start vld thread.\n");
    #endif

    //spwan sub process thread
    //start vld thread
    pthread_create(&r->tid_vld,NULL,thread_rv34_amba_vld,r);

    #ifdef __log_picinfo__
    log_openfile_text_f(log_fd_picinfo_text, "rv_picinfo_txt");
    #endif

#ifdef __dump_DSP_test_data__
        log_openfile_f(log_fd_general_txt,"general.txt");
#endif

#ifdef __dump_whole__
    #ifdef __dump_DSP_test_data__
        #ifdef __dump_binary__
        log_openfile_f(log_fd_residual,"rv_residual");
        log_openfile_f(log_fd_residual_2,"rv_2residual");
        log_openfile_f(log_fd_mvdsp,"rv_mv");
        log_openfile_f(log_fd_reffyraw,"rv_reff_y_raw");
        log_openfile_f(log_fd_reffuvraw,"rv_reff_uv_raw");
        log_openfile_f(log_fd_reffytiled,"rv_reff_y_tiled");
        log_openfile_f(log_fd_reffuvtiled,"rv_reff_uv_tiled");
        log_openfile_f(log_fd_refbyraw,"rv_refb_y_raw");
        log_openfile_f(log_fd_refbuvraw,"rv_refb_uv_raw");
        log_openfile_f(log_fd_refbytiled,"rv_refb_y_tiled");
        log_openfile_f(log_fd_refbuvtiled,"rv_refb_uv_tiled");
        log_openfile_f(log_fd_result,"rv_result");
        log_openfile_f(log_fd_result_2,"rv_result_y_rasterscaned");
        log_openfile_f(log_fd_result_3,"rv_result_uv_rasterscaned");
        #endif
    #endif
#endif

    #ifdef __log_decoding_process__
        av_log(NULL,AV_LOG_ERROR,"**ffnew_rv34_amba_decode_init done.\n");
    #endif
#ifdef __log_dump_decocer_input_data__
    log_openfile_text_f(log_fd_temp, "debug_txt");
#endif

    r->error_wait_flush = 0;
    r->start_exit = 0;
    return 0;
}

#endif

