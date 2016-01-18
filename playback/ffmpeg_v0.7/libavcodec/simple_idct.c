/*
 * Simple IDCT
 *
 * Copyright (c) 2001 Michael Niedermayer <michaelni@gmx.at>
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
 * simpleidct in C.
 */

/*
  based upon some outcommented c code from mpeg2dec (idct_mmx.c
  written by Aaron Holtzman <aholtzma@ess.engr.uvic.ca>)
 */
#include "avcodec.h"
#include "dsputil.h"
#include "mathops.h"
#include "simple_idct.h"

#if 0
#define W1 2841 /* 2048*sqrt (2)*cos (1*pi/16) */
#define W2 2676 /* 2048*sqrt (2)*cos (2*pi/16) */
#define W3 2408 /* 2048*sqrt (2)*cos (3*pi/16) */
#define W4 2048 /* 2048*sqrt (2)*cos (4*pi/16) */
#define W5 1609 /* 2048*sqrt (2)*cos (5*pi/16) */
#define W6 1108 /* 2048*sqrt (2)*cos (6*pi/16) */
#define W7 565  /* 2048*sqrt (2)*cos (7*pi/16) */
#define ROW_SHIFT 8
#define COL_SHIFT 17
#else
#define W1  22725  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W2  21407  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W3  19266  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W4  16383  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W5  12873  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W6  8867   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W7  4520   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define ROW_SHIFT 11
#define COL_SHIFT 20 // 6
#endif

static inline void idctRowCondDC_wmv2 (DCTELEM * row, DCTELEM * mid);
static inline void idctSparseCol_wmv2 (DCTELEM * col, DCTELEM * des);

static inline void idct4col_wmv2(const DCTELEM *col,DCTELEM *des);
static inline void idct4row_wmv2(DCTELEM *row,DCTELEM *mid);

//transform space for wmv2 abt(special transform)
static DCTELEM g_transform[64];


uint8_t *gcm = ff_cropTbl + MAX_NEG_CROP;


#ifdef __use_reference_dct__
#include <math.h>
static const double gpi= 3.14159265358979323846;
/* cosine transform matrix for 8x1 IDCT */
static double c[8][8];
#endif

#ifdef __use_cm_dct__
int CM_sat1(int i)
{
  int val;
  val = 1 << 13;
  return ((i > (val-1)) ? (val-1) : (i < -val) ? -val : i);
}

int CM_sat2(int i)
{
  return ((i > 255) ? 255 : (i < -256) ? -256 : i);
}

int CM_sat3(int i)
{
  int val;
  val = 1 << 19;  
  return ((i > (val-1)) ? (val-1) : (i < -val) ? -val : i);
}

#define USE_15BIT_DCT_PRECISION

void CM_IDCT(short *block)
{
#ifdef USE_15BIT_DCT_PRECISION
  // Version 2: DCT coeff 15 bits
  int   i;
  int   tmp[64]; // transposed matrix
  short *din;
  int   *din2;
  int   s1[8];
  long long s3[8];

  int   BITWIDTH1 = 9 + 1;      // 9  = S13.16 ->S13.6, 1 for (a+b)/2
  int   BITWIDTH2 = 15 + 6 + 1; // 15 = C1-C7 scaling, 6 = unscaled bits in row operation,  1 for (a+b)/2
  int   round1 = 1 << (BITWIDTH1 - 1);
  int   round2 = 1 << (BITWIDTH2 - 1);

  // DCT coefficient (s.15) in integer format
  const int C1 = 32138;
  const int C2 = 30273;
  const int C3 = 27245;
  const int C4 = 23170;
  const int C5 = 18204;
  const int C6 = 12539;
  const int C7 = 6392;

  // column first and then row
  din = &block[0];
  for (i = 0; i < 8; i ++)
  {
    s1[0] = C4 * din[0] + C2 * din[16] + C4 * din[32] + C6 * din[48];
    s1[1] = C4 * din[0] + C6 * din[16] - C4 * din[32] - C2 * din[48];
    s1[2] = C4 * din[0] - C6 * din[16] - C4 * din[32] + C2 * din[48];
    s1[3] = C4 * din[0] - C2 * din[16] + C4 * din[32] - C6 * din[48];
    s1[4] = C1 * din[8] + C3 * din[24] + C5 * din[40] + C7 * din[56];
    s1[5] = C3 * din[8] - C7 * din[24] - C1 * din[40] - C5 * din[56];
    s1[6] = C5 * din[8] - C1 * din[24] + C7 * din[40] + C3 * din[56];
    s1[7] = C7 * din[8] - C5 * din[24] + C3 * din[40] - C1 * din[56];

    tmp[0+8*i] = CM_sat3 ((s1[0] + s1[4] + round1 ) >> BITWIDTH1);
    tmp[1+8*i] = CM_sat3 ((s1[1] + s1[5] + round1 ) >> BITWIDTH1);
    tmp[2+8*i] = CM_sat3 ((s1[2] + s1[6] + round1 ) >> BITWIDTH1);
    tmp[3+8*i] = CM_sat3 ((s1[3] + s1[7] + round1 ) >> BITWIDTH1);
    tmp[7+8*i] = CM_sat3 ((s1[0] - s1[4] + round1 ) >> BITWIDTH1);
    tmp[6+8*i] = CM_sat3 ((s1[1] - s1[5] + round1 ) >> BITWIDTH1);
    tmp[5+8*i] = CM_sat3 ((s1[2] - s1[6] + round1 ) >> BITWIDTH1);
    tmp[4+8*i] = CM_sat3 ((s1[3] - s1[7] + round1 ) >> BITWIDTH1);

    din += 1;
  }  

  din2 = &tmp[0];
  for (i = 0; i < 8; i ++)
  {
    s3[0] = C4 * din2[0] + C2 * din2[16] + C4 * din2[32] + C6 * din2[48];
    s3[1] = C4 * din2[0] + C6 * din2[16] - C4 * din2[32] - C2 * din2[48];
    s3[2] = C4 * din2[0] - C6 * din2[16] - C4 * din2[32] + C2 * din2[48];
    s3[3] = C4 * din2[0] - C2 * din2[16] + C4 * din2[32] - C6 * din2[48];
    s3[4] = C1 * din2[8] + C3 * din2[24] + C5 * din2[40] + C7 * din2[56];
    s3[5] = C3 * din2[8] - C7 * din2[24] - C1 * din2[40] - C5 * din2[56];
    s3[6] = C5 * din2[8] - C1 * din2[24] + C7 * din2[40] + C3 * din2[56];
    s3[7] = C7 * din2[8] - C5 * din2[24] + C3 * din2[40] - C1 * din2[56];
    
    block[0+8*i] = CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2));
    block[1+8*i] = CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2));
    block[2+8*i] = CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2));
    block[3+8*i] = CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2));
    block[7+8*i] = CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2));
    block[6+8*i] = CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2));
    block[5+8*i] = CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2));
    block[4+8*i] = CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2));
    din2 += 1;
  }
#else
  // Version 1: DCT coeff 13 bits
  int   i;
  int   tmp[64]; // transposed matrix
  short *din;
  int   *din2;
  int   s1[8], s3[8];
  int   BITWIDTH1 = 10 + 1;     // 10 = S13.14 ->S13.3, 1 for (a+b)/2
  int   BITWIDTH2 = 13 + 3 + 1; // 13 = C1-C7 scaling, 3 = unscaled bits in row operation,  1 for (a+b)/2
  int   round1 = 1 << (BITWIDTH1 - 1);
  int   round2 = 1 << (BITWIDTH2 - 1);
  const int C1 = 8035;
  const int C2 = 7568;
  const int C3 = 6811;
  const int C4 = 5793;
  const int C5 = 4551;
  const int C6 = 3135;
  const int C7 = 1598;
 
  // column first and then row
  din = &block[0];
  for (i = 0; i < 8; i ++)
  {
    s1[0] = C4 * din[0] + C2 * din[16] + C4 * din[32] + C6 * din[48];
    s1[1] = C4 * din[0] + C6 * din[16] - C4 * din[32] - C2 * din[48];
    s1[2] = C4 * din[0] - C6 * din[16] - C4 * din[32] + C2 * din[48];
    s1[3] = C4 * din[0] - C2 * din[16] + C4 * din[32] - C6 * din[48];
    s1[4] = C1 * din[8] + C3 * din[24] + C5 * din[40] + C7 * din[56];
    s1[5] = C3 * din[8] - C7 * din[24] - C1 * din[40] - C5 * din[56];
    s1[6] = C5 * din[8] - C1 * din[24] + C7 * din[40] + C3 * din[56];
    s1[7] = C7 * din[8] - C5 * din[24] + C3 * din[40] - C1 * din[56];

    tmp[0+8*i] = CM_sat1 ((s1[0] + s1[4] + round1 ) >> BITWIDTH1);
    tmp[1+8*i] = CM_sat1 ((s1[1] + s1[5] + round1 ) >> BITWIDTH1);
    tmp[2+8*i] = CM_sat1 ((s1[2] + s1[6] + round1 ) >> BITWIDTH1);
    tmp[3+8*i] = CM_sat1 ((s1[3] + s1[7] + round1 ) >> BITWIDTH1);
    tmp[7+8*i] = CM_sat1 ((s1[0] - s1[4] + round1 ) >> BITWIDTH1);
    tmp[6+8*i] = CM_sat1 ((s1[1] - s1[5] + round1 ) >> BITWIDTH1);
    tmp[5+8*i] = CM_sat1 ((s1[2] - s1[6] + round1 ) >> BITWIDTH1);
    tmp[4+8*i] = CM_sat1 ((s1[3] - s1[7] + round1 ) >> BITWIDTH1);

    din += 1;
  }

  din2 = &tmp[0];
  for (i = 0; i < 8; i ++)
  {
    s3[0] = C4 * din2[0] + C2 * din2[16] + C4 * din2[32] + C6 * din2[48];
    s3[1] = C4 * din2[0] + C6 * din2[16] - C4 * din2[32] - C2 * din2[48];
    s3[2] = C4 * din2[0] - C6 * din2[16] - C4 * din2[32] + C2 * din2[48];
    s3[3] = C4 * din2[0] - C2 * din2[16] + C4 * din2[32] - C6 * din2[48];
    s3[4] = C1 * din2[8] + C3 * din2[24] + C5 * din2[40] + C7 * din2[56];
    s3[5] = C3 * din2[8] - C7 * din2[24] - C1 * din2[40] - C5 * din2[56];
    s3[6] = C5 * din2[8] - C1 * din2[24] + C7 * din2[40] + C3 * din2[56];
    s3[7] = C7 * din2[8] - C5 * din2[24] + C3 * din2[40] - C1 * din2[56];

    block[0+8*i] = CM_sat2 ((s3[0] + s3[4] + round2 ) >> BITWIDTH2);
    block[1+8*i] = CM_sat2 ((s3[1] + s3[5] + round2 ) >> BITWIDTH2);
    block[2+8*i] = CM_sat2 ((s3[2] + s3[6] + round2 ) >> BITWIDTH2);
    block[3+8*i] = CM_sat2 ((s3[3] + s3[7] + round2 ) >> BITWIDTH2);
    block[7+8*i] = CM_sat2 ((s3[0] - s3[4] + round2 ) >> BITWIDTH2);
    block[6+8*i] = CM_sat2 ((s3[1] - s3[5] + round2 ) >> BITWIDTH2);
    block[5+8*i] = CM_sat2 ((s3[2] - s3[6] + round2 ) >> BITWIDTH2);
    block[4+8*i] = CM_sat2 ((s3[3] - s3[7] + round2 ) >> BITWIDTH2);
    din2 += 1;
  }
#endif
}

void CM_IDCT_put(unsigned char*des,int stride, short *block)
{
#ifdef USE_15BIT_DCT_PRECISION
  // Version 2: DCT coeff 15 bits
  int   i;
  int   tmp[64]; // transposed matrix
  short *din;
  int   *din2;
  int   s1[8];
  long long s3[8];

  int   BITWIDTH1 = 9 + 1;      // 9  = S13.16 ->S13.6, 1 for (a+b)/2
  int   BITWIDTH2 = 15 + 6 + 1; // 15 = C1-C7 scaling, 6 = unscaled bits in row operation,  1 for (a+b)/2
  int   round1 = 1 << (BITWIDTH1 - 1);
  int   round2 = 1 << (BITWIDTH2 - 1);

  // DCT coefficient (s.15) in integer format
  const int C1 = 32138;
  const int C2 = 30273;
  const int C3 = 27245;
  const int C4 = 23170;
  const int C5 = 18204;
  const int C6 = 12539;
  const int C7 = 6392;

  // column first and then row
  din = &block[0];
  for (i = 0; i < 8; i ++)
  {
    s1[0] = C4 * din[0] + C2 * din[16] + C4 * din[32] + C6 * din[48];
    s1[1] = C4 * din[0] + C6 * din[16] - C4 * din[32] - C2 * din[48];
    s1[2] = C4 * din[0] - C6 * din[16] - C4 * din[32] + C2 * din[48];
    s1[3] = C4 * din[0] - C2 * din[16] + C4 * din[32] - C6 * din[48];
    s1[4] = C1 * din[8] + C3 * din[24] + C5 * din[40] + C7 * din[56];
    s1[5] = C3 * din[8] - C7 * din[24] - C1 * din[40] - C5 * din[56];
    s1[6] = C5 * din[8] - C1 * din[24] + C7 * din[40] + C3 * din[56];
    s1[7] = C7 * din[8] - C5 * din[24] + C3 * din[40] - C1 * din[56];

    tmp[0+8*i] = CM_sat3 ((s1[0] + s1[4] + round1 ) >> BITWIDTH1);
    tmp[1+8*i] = CM_sat3 ((s1[1] + s1[5] + round1 ) >> BITWIDTH1);
    tmp[2+8*i] = CM_sat3 ((s1[2] + s1[6] + round1 ) >> BITWIDTH1);
    tmp[3+8*i] = CM_sat3 ((s1[3] + s1[7] + round1 ) >> BITWIDTH1);
    tmp[7+8*i] = CM_sat3 ((s1[0] - s1[4] + round1 ) >> BITWIDTH1);
    tmp[6+8*i] = CM_sat3 ((s1[1] - s1[5] + round1 ) >> BITWIDTH1);
    tmp[5+8*i] = CM_sat3 ((s1[2] - s1[6] + round1 ) >> BITWIDTH1);
    tmp[4+8*i] = CM_sat3 ((s1[3] - s1[7] + round1 ) >> BITWIDTH1);

    din += 1;
  }  

  din2 = &tmp[0];
  for (i = 0; i < 8; i ++)
  {
    s3[0] = C4 * din2[0] + C2 * din2[16] + C4 * din2[32] + C6 * din2[48];
    s3[1] = C4 * din2[0] + C6 * din2[16] - C4 * din2[32] - C2 * din2[48];
    s3[2] = C4 * din2[0] - C6 * din2[16] - C4 * din2[32] + C2 * din2[48];
    s3[3] = C4 * din2[0] - C2 * din2[16] + C4 * din2[32] - C6 * din2[48];
    s3[4] = C1 * din2[8] + C3 * din2[24] + C5 * din2[40] + C7 * din2[56];
    s3[5] = C3 * din2[8] - C7 * din2[24] - C1 * din2[40] - C5 * din2[56];
    s3[6] = C5 * din2[8] - C1 * din2[24] + C7 * din2[40] + C3 * din2[56];
    s3[7] = C7 * din2[8] - C5 * din2[24] + C3 * din2[40] - C1 * din2[56];

    des[0+stride*i]  = gcm[ CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2))];
    des[1+stride*i]  = gcm[CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2))];
    des[2+stride*i]  = gcm[CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2))];
    des[3+stride*i]  = gcm[CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2))];
    des[7+stride*i]  = gcm[CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2))];
    des[6+stride*i]  = gcm[CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2))];
    des[5+stride*i]  = gcm[CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2))];
    des[4+stride*i]  = gcm[CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2))];  
    
/*    block[0+8*i] = CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2));
    block[1+8*i] = CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2));
    block[2+8*i] = CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2));
    block[3+8*i] = CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2));
    block[7+8*i] = CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2));
    block[6+8*i] = CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2));
    block[5+8*i] = CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2));
    block[4+8*i] = CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2));*/
    din2 += 1;
  }
#else
  // Version 1: DCT coeff 13 bits
  int   i;
  int   tmp[64]; // transposed matrix
  short *din;
  int   *din2;
  int   s1[8], s3[8];
  int   BITWIDTH1 = 10 + 1;     // 10 = S13.14 ->S13.3, 1 for (a+b)/2
  int   BITWIDTH2 = 13 + 3 + 1; // 13 = C1-C7 scaling, 3 = unscaled bits in row operation,  1 for (a+b)/2
  int   round1 = 1 << (BITWIDTH1 - 1);
  int   round2 = 1 << (BITWIDTH2 - 1);
  const int C1 = 8035;
  const int C2 = 7568;
  const int C3 = 6811;
  const int C4 = 5793;
  const int C5 = 4551;
  const int C6 = 3135;
  const int C7 = 1598;
 
  // column first and then row
  din = &block[0];
  for (i = 0; i < 8; i ++)
  {
    s1[0] = C4 * din[0] + C2 * din[16] + C4 * din[32] + C6 * din[48];
    s1[1] = C4 * din[0] + C6 * din[16] - C4 * din[32] - C2 * din[48];
    s1[2] = C4 * din[0] - C6 * din[16] - C4 * din[32] + C2 * din[48];
    s1[3] = C4 * din[0] - C2 * din[16] + C4 * din[32] - C6 * din[48];
    s1[4] = C1 * din[8] + C3 * din[24] + C5 * din[40] + C7 * din[56];
    s1[5] = C3 * din[8] - C7 * din[24] - C1 * din[40] - C5 * din[56];
    s1[6] = C5 * din[8] - C1 * din[24] + C7 * din[40] + C3 * din[56];
    s1[7] = C7 * din[8] - C5 * din[24] + C3 * din[40] - C1 * din[56];

    tmp[0+8*i] = CM_sat1 ((s1[0] + s1[4] + round1 ) >> BITWIDTH1);
    tmp[1+8*i] = CM_sat1 ((s1[1] + s1[5] + round1 ) >> BITWIDTH1);
    tmp[2+8*i] = CM_sat1 ((s1[2] + s1[6] + round1 ) >> BITWIDTH1);
    tmp[3+8*i] = CM_sat1 ((s1[3] + s1[7] + round1 ) >> BITWIDTH1);
    tmp[7+8*i] = CM_sat1 ((s1[0] - s1[4] + round1 ) >> BITWIDTH1);
    tmp[6+8*i] = CM_sat1 ((s1[1] - s1[5] + round1 ) >> BITWIDTH1);
    tmp[5+8*i] = CM_sat1 ((s1[2] - s1[6] + round1 ) >> BITWIDTH1);
    tmp[4+8*i] = CM_sat1 ((s1[3] - s1[7] + round1 ) >> BITWIDTH1);

    din += 1;
  }

  din2 = &tmp[0];
  for (i = 0; i < 8; i ++)
  {
    s3[0] = C4 * din2[0] + C2 * din2[16] + C4 * din2[32] + C6 * din2[48];
    s3[1] = C4 * din2[0] + C6 * din2[16] - C4 * din2[32] - C2 * din2[48];
    s3[2] = C4 * din2[0] - C6 * din2[16] - C4 * din2[32] + C2 * din2[48];
    s3[3] = C4 * din2[0] - C2 * din2[16] + C4 * din2[32] - C6 * din2[48];
    s3[4] = C1 * din2[8] + C3 * din2[24] + C5 * din2[40] + C7 * din2[56];
    s3[5] = C3 * din2[8] - C7 * din2[24] - C1 * din2[40] - C5 * din2[56];
    s3[6] = C5 * din2[8] - C1 * din2[24] + C7 * din2[40] + C3 * din2[56];
    s3[7] = C7 * din2[8] - C5 * din2[24] + C3 * din2[40] - C1 * din2[56];

    des[0+stride*i]  = gcm[ CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2))];
    des[1+stride*i]  = gcm[CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2))];
    des[2+stride*i]  = gcm[CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2))];
    des[3+stride*i]  = gcm[CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2))];
    des[7+stride*i]  = gcm[CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2))];
    des[6+stride*i]  = gcm[CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2))];
    des[5+stride*i]  = gcm[CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2))];
    des[4+stride*i]  = gcm[CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2))];  
    /*
    block[0+8*i] = CM_sat2 ((s3[0] + s3[4] + round2 ) >> BITWIDTH2);
    block[1+8*i] = CM_sat2 ((s3[1] + s3[5] + round2 ) >> BITWIDTH2);
    block[2+8*i] = CM_sat2 ((s3[2] + s3[6] + round2 ) >> BITWIDTH2);
    block[3+8*i] = CM_sat2 ((s3[3] + s3[7] + round2 ) >> BITWIDTH2);
    block[7+8*i] = CM_sat2 ((s3[0] - s3[4] + round2 ) >> BITWIDTH2);
    block[6+8*i] = CM_sat2 ((s3[1] - s3[5] + round2 ) >> BITWIDTH2);
    block[5+8*i] = CM_sat2 ((s3[2] - s3[6] + round2 ) >> BITWIDTH2);
    block[4+8*i] = CM_sat2 ((s3[3] - s3[7] + round2 ) >> BITWIDTH2);*/
    din2 += 1;
  }
#endif
}

void CM_IDCT_add(unsigned char*des,int stride, short *block)
{
#ifdef USE_15BIT_DCT_PRECISION
  // Version 2: DCT coeff 15 bits
  int   i;
  int   tmp[64]; // transposed matrix
  short *din;
  int   *din2;
  int   s1[8];
  long long  s3[8];

  int   BITWIDTH1 = 9 + 1;      // 9  = S13.16 ->S13.6, 1 for (a+b)/2
  int   BITWIDTH2 = 15 + 6 + 1; // 15 = C1-C7 scaling, 6 = unscaled bits in row operation,  1 for (a+b)/2
  int   round1 = 1 << (BITWIDTH1 - 1);
  int   round2 = 1 << (BITWIDTH2 - 1);

  // DCT coefficient (s.15) in integer format
  const int C1 = 32138;
  const int C2 = 30273;
  const int C3 = 27245;
  const int C4 = 23170;
  const int C5 = 18204;
  const int C6 = 12539;
  const int C7 = 6392;

  // column first and then row
  din = &block[0];
  for (i = 0; i < 8; i ++)
  {
    s1[0] = C4 * din[0] + C2 * din[16] + C4 * din[32] + C6 * din[48];
    s1[1] = C4 * din[0] + C6 * din[16] - C4 * din[32] - C2 * din[48];
    s1[2] = C4 * din[0] - C6 * din[16] - C4 * din[32] + C2 * din[48];
    s1[3] = C4 * din[0] - C2 * din[16] + C4 * din[32] - C6 * din[48];
    s1[4] = C1 * din[8] + C3 * din[24] + C5 * din[40] + C7 * din[56];
    s1[5] = C3 * din[8] - C7 * din[24] - C1 * din[40] - C5 * din[56];
    s1[6] = C5 * din[8] - C1 * din[24] + C7 * din[40] + C3 * din[56];
    s1[7] = C7 * din[8] - C5 * din[24] + C3 * din[40] - C1 * din[56];

    tmp[0+8*i] = CM_sat3 ((s1[0] + s1[4] + round1 ) >> BITWIDTH1);
    tmp[1+8*i] = CM_sat3 ((s1[1] + s1[5] + round1 ) >> BITWIDTH1);
    tmp[2+8*i] = CM_sat3 ((s1[2] + s1[6] + round1 ) >> BITWIDTH1);
    tmp[3+8*i] = CM_sat3 ((s1[3] + s1[7] + round1 ) >> BITWIDTH1);
    tmp[7+8*i] = CM_sat3 ((s1[0] - s1[4] + round1 ) >> BITWIDTH1);
    tmp[6+8*i] = CM_sat3 ((s1[1] - s1[5] + round1 ) >> BITWIDTH1);
    tmp[5+8*i] = CM_sat3 ((s1[2] - s1[6] + round1 ) >> BITWIDTH1);
    tmp[4+8*i] = CM_sat3 ((s1[3] - s1[7] + round1 ) >> BITWIDTH1);

    din += 1;
  }  

  din2 = &tmp[0];
  for (i = 0; i < 8; i ++)
  {
    s3[0] = C4 * din2[0] + C2 * din2[16] + C4 * din2[32] + C6 * din2[48];
    s3[1] = C4 * din2[0] + C6 * din2[16] - C4 * din2[32] - C2 * din2[48];
    s3[2] = C4 * din2[0] - C6 * din2[16] - C4 * din2[32] + C2 * din2[48];
    s3[3] = C4 * din2[0] - C2 * din2[16] + C4 * din2[32] - C6 * din2[48];
    s3[4] = C1 * din2[8] + C3 * din2[24] + C5 * din2[40] + C7 * din2[56];
    s3[5] = C3 * din2[8] - C7 * din2[24] - C1 * din2[40] - C5 * din2[56];
    s3[6] = C5 * din2[8] - C1 * din2[24] + C7 * din2[40] + C3 * din2[56];
    s3[7] = C7 * din2[8] - C5 * din2[24] + C3 * din2[40] - C1 * din2[56];

    des[0+stride*i]  = gcm[des[0+stride*i] + CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2))];
    des[1+stride*i]  = gcm[des[1+stride*i] + CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2))];
    des[2+stride*i]  = gcm[des[2+stride*i] + CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2))];
    des[3+stride*i]  = gcm[des[3+stride*i] + CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2))];
    des[7+stride*i]  = gcm[des[7+stride*i] + CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2))];
    des[6+stride*i]  = gcm[des[6+stride*i] + CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2))];
    des[5+stride*i]  = gcm[des[5+stride*i] + CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2))];
    des[4+stride*i]  = gcm[des[4+stride*i] + CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2))];  
    
/*    block[0+8*i] = CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2));
    block[1+8*i] = CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2));
    block[2+8*i] = CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2));
    block[3+8*i] = CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2));
    block[7+8*i] = CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2));
    block[6+8*i] = CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2));
    block[5+8*i] = CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2));
    block[4+8*i] = CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2));*/
    din2 += 1;
  }
#else
  // Version 1: DCT coeff 13 bits
  int   i;
  int   tmp[64]; // transposed matrix
  short *din;
  int   *din2;
  int   s1[8], s3[8];
  int   BITWIDTH1 = 10 + 1;     // 10 = S13.14 ->S13.3, 1 for (a+b)/2
  int   BITWIDTH2 = 13 + 3 + 1; // 13 = C1-C7 scaling, 3 = unscaled bits in row operation,  1 for (a+b)/2
  int   round1 = 1 << (BITWIDTH1 - 1);
  int   round2 = 1 << (BITWIDTH2 - 1);
  const int C1 = 8035;
  const int C2 = 7568;
  const int C3 = 6811;
  const int C4 = 5793;
  const int C5 = 4551;
  const int C6 = 3135;
  const int C7 = 1598;
 
  // column first and then row
  din = &block[0];
  for (i = 0; i < 8; i ++)
  {
    s1[0] = C4 * din[0] + C2 * din[16] + C4 * din[32] + C6 * din[48];
    s1[1] = C4 * din[0] + C6 * din[16] - C4 * din[32] - C2 * din[48];
    s1[2] = C4 * din[0] - C6 * din[16] - C4 * din[32] + C2 * din[48];
    s1[3] = C4 * din[0] - C2 * din[16] + C4 * din[32] - C6 * din[48];
    s1[4] = C1 * din[8] + C3 * din[24] + C5 * din[40] + C7 * din[56];
    s1[5] = C3 * din[8] - C7 * din[24] - C1 * din[40] - C5 * din[56];
    s1[6] = C5 * din[8] - C1 * din[24] + C7 * din[40] + C3 * din[56];
    s1[7] = C7 * din[8] - C5 * din[24] + C3 * din[40] - C1 * din[56];

    tmp[0+8*i] = CM_sat1 ((s1[0] + s1[4] + round1 ) >> BITWIDTH1);
    tmp[1+8*i] = CM_sat1 ((s1[1] + s1[5] + round1 ) >> BITWIDTH1);
    tmp[2+8*i] = CM_sat1 ((s1[2] + s1[6] + round1 ) >> BITWIDTH1);
    tmp[3+8*i] = CM_sat1 ((s1[3] + s1[7] + round1 ) >> BITWIDTH1);
    tmp[7+8*i] = CM_sat1 ((s1[0] - s1[4] + round1 ) >> BITWIDTH1);
    tmp[6+8*i] = CM_sat1 ((s1[1] - s1[5] + round1 ) >> BITWIDTH1);
    tmp[5+8*i] = CM_sat1 ((s1[2] - s1[6] + round1 ) >> BITWIDTH1);
    tmp[4+8*i] = CM_sat1 ((s1[3] - s1[7] + round1 ) >> BITWIDTH1);

    din += 1;
  }

  din2 = &tmp[0];
  for (i = 0; i < 8; i ++)
  {
    s3[0] = C4 * din2[0] + C2 * din2[16] + C4 * din2[32] + C6 * din2[48];
    s3[1] = C4 * din2[0] + C6 * din2[16] - C4 * din2[32] - C2 * din2[48];
    s3[2] = C4 * din2[0] - C6 * din2[16] - C4 * din2[32] + C2 * din2[48];
    s3[3] = C4 * din2[0] - C2 * din2[16] + C4 * din2[32] - C6 * din2[48];
    s3[4] = C1 * din2[8] + C3 * din2[24] + C5 * din2[40] + C7 * din2[56];
    s3[5] = C3 * din2[8] - C7 * din2[24] - C1 * din2[40] - C5 * din2[56];
    s3[6] = C5 * din2[8] - C1 * din2[24] + C7 * din2[40] + C3 * din2[56];
    s3[7] = C7 * din2[8] - C5 * din2[24] + C3 * din2[40] - C1 * din2[56];

    des[0+stride*i]  = gcm[des[0+stride*i] + CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2))];
    des[1+stride*i]  = gcm[des[1+stride*i] + CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2))];
    des[2+stride*i]  = gcm[des[2+stride*i] + CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2))];
    des[3+stride*i]  = gcm[des[3+stride*i] + CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2))];
    des[7+stride*i]  = gcm[des[7+stride*i] + CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2))];
    des[6+stride*i]  = gcm[des[6+stride*i] + CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2))];
    des[5+stride*i]  = gcm[des[5+stride*i] + CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2))];
    des[4+stride*i]  = gcm[des[4+stride*i] + CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2))];  
    /*
    block[0+8*i] = CM_sat2 ((s3[0] + s3[4] + round2 ) >> BITWIDTH2);
    block[1+8*i] = CM_sat2 ((s3[1] + s3[5] + round2 ) >> BITWIDTH2);
    block[2+8*i] = CM_sat2 ((s3[2] + s3[6] + round2 ) >> BITWIDTH2);
    block[3+8*i] = CM_sat2 ((s3[3] + s3[7] + round2 ) >> BITWIDTH2);
    block[7+8*i] = CM_sat2 ((s3[0] - s3[4] + round2 ) >> BITWIDTH2);
    block[6+8*i] = CM_sat2 ((s3[1] - s3[5] + round2 ) >> BITWIDTH2);
    block[5+8*i] = CM_sat2 ((s3[2] - s3[6] + round2 ) >> BITWIDTH2);
    block[4+8*i] = CM_sat2 ((s3[3] - s3[7] + round2 ) >> BITWIDTH2);*/
    din2 += 1;
  }
#endif
}

void CM_IDCT_put_nv12(unsigned char*des,int stride, short *block)
{
#ifdef USE_15BIT_DCT_PRECISION
  // Version 2: DCT coeff 15 bits
  int   i;
  int   tmp[64]; // transposed matrix
  short *din;
  int   *din2;
  int   s1[8];
  long long  s3[8];

  int   BITWIDTH1 = 9 + 1;      // 9  = S13.16 ->S13.6, 1 for (a+b)/2
  int   BITWIDTH2 = 15 + 6 + 1; // 15 = C1-C7 scaling, 6 = unscaled bits in row operation,  1 for (a+b)/2
  int   round1 = 1 << (BITWIDTH1 - 1);
  int   round2 = 1 << (BITWIDTH2 - 1);

  // DCT coefficient (s.15) in integer format
  const int C1 = 32138;
  const int C2 = 30273;
  const int C3 = 27245;
  const int C4 = 23170;
  const int C5 = 18204;
  const int C6 = 12539;
  const int C7 = 6392;

  // column first and then row
  din = &block[0];
  for (i = 0; i < 8; i ++)
  {
    s1[0] = C4 * din[0] + C2 * din[16] + C4 * din[32] + C6 * din[48];
    s1[1] = C4 * din[0] + C6 * din[16] - C4 * din[32] - C2 * din[48];
    s1[2] = C4 * din[0] - C6 * din[16] - C4 * din[32] + C2 * din[48];
    s1[3] = C4 * din[0] - C2 * din[16] + C4 * din[32] - C6 * din[48];
    s1[4] = C1 * din[8] + C3 * din[24] + C5 * din[40] + C7 * din[56];
    s1[5] = C3 * din[8] - C7 * din[24] - C1 * din[40] - C5 * din[56];
    s1[6] = C5 * din[8] - C1 * din[24] + C7 * din[40] + C3 * din[56];
    s1[7] = C7 * din[8] - C5 * din[24] + C3 * din[40] - C1 * din[56];

    tmp[0+8*i] = CM_sat3 ((s1[0] + s1[4] + round1 ) >> BITWIDTH1);
    tmp[1+8*i] = CM_sat3 ((s1[1] + s1[5] + round1 ) >> BITWIDTH1);
    tmp[2+8*i] = CM_sat3 ((s1[2] + s1[6] + round1 ) >> BITWIDTH1);
    tmp[3+8*i] = CM_sat3 ((s1[3] + s1[7] + round1 ) >> BITWIDTH1);
    tmp[7+8*i] = CM_sat3 ((s1[0] - s1[4] + round1 ) >> BITWIDTH1);
    tmp[6+8*i] = CM_sat3 ((s1[1] - s1[5] + round1 ) >> BITWIDTH1);
    tmp[5+8*i] = CM_sat3 ((s1[2] - s1[6] + round1 ) >> BITWIDTH1);
    tmp[4+8*i] = CM_sat3 ((s1[3] - s1[7] + round1 ) >> BITWIDTH1);

    din += 1;
  }  

  din2 = &tmp[0];
  for (i = 0; i < 8; i ++)
  {
    s3[0] = C4 * din2[0] + C2 * din2[16] + C4 * din2[32] + C6 * din2[48];
    s3[1] = C4 * din2[0] + C6 * din2[16] - C4 * din2[32] - C2 * din2[48];
    s3[2] = C4 * din2[0] - C6 * din2[16] - C4 * din2[32] + C2 * din2[48];
    s3[3] = C4 * din2[0] - C2 * din2[16] + C4 * din2[32] - C6 * din2[48];
    s3[4] = C1 * din2[8] + C3 * din2[24] + C5 * din2[40] + C7 * din2[56];
    s3[5] = C3 * din2[8] - C7 * din2[24] - C1 * din2[40] - C5 * din2[56];
    s3[6] = C5 * din2[8] - C1 * din2[24] + C7 * din2[40] + C3 * din2[56];
    s3[7] = C7 * din2[8] - C5 * din2[24] + C3 * din2[40] - C1 * din2[56];

    des[0+stride*i]  = gcm[ CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2))];
    des[2+stride*i]  = gcm[CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2))];
    des[4+stride*i]  = gcm[CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2))];
    des[6+stride*i]  = gcm[CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2))];
    des[14+stride*i]  = gcm[CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2))];
    des[12+stride*i]  = gcm[CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2))];
    des[10+stride*i]  = gcm[CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2))];
    des[8+stride*i]  = gcm[CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2))];  
    
/*    block[0+8*i] = CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2));
    block[1+8*i] = CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2));
    block[2+8*i] = CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2));
    block[3+8*i] = CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2));
    block[7+8*i] = CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2));
    block[6+8*i] = CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2));
    block[5+8*i] = CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2));
    block[4+8*i] = CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2));*/
    din2 += 1;
  }
#else
  // Version 1: DCT coeff 13 bits
  int   i;
  int   tmp[64]; // transposed matrix
  short *din;
  int   *din2;
  int   s1[8], s3[8];
  int   BITWIDTH1 = 10 + 1;     // 10 = S13.14 ->S13.3, 1 for (a+b)/2
  int   BITWIDTH2 = 13 + 3 + 1; // 13 = C1-C7 scaling, 3 = unscaled bits in row operation,  1 for (a+b)/2
  int   round1 = 1 << (BITWIDTH1 - 1);
  int   round2 = 1 << (BITWIDTH2 - 1);
  const int C1 = 8035;
  const int C2 = 7568;
  const int C3 = 6811;
  const int C4 = 5793;
  const int C5 = 4551;
  const int C6 = 3135;
  const int C7 = 1598;
 
  // column first and then row
  din = &block[0];
  for (i = 0; i < 8; i ++)
  {
    s1[0] = C4 * din[0] + C2 * din[16] + C4 * din[32] + C6 * din[48];
    s1[1] = C4 * din[0] + C6 * din[16] - C4 * din[32] - C2 * din[48];
    s1[2] = C4 * din[0] - C6 * din[16] - C4 * din[32] + C2 * din[48];
    s1[3] = C4 * din[0] - C2 * din[16] + C4 * din[32] - C6 * din[48];
    s1[4] = C1 * din[8] + C3 * din[24] + C5 * din[40] + C7 * din[56];
    s1[5] = C3 * din[8] - C7 * din[24] - C1 * din[40] - C5 * din[56];
    s1[6] = C5 * din[8] - C1 * din[24] + C7 * din[40] + C3 * din[56];
    s1[7] = C7 * din[8] - C5 * din[24] + C3 * din[40] - C1 * din[56];

    tmp[0+8*i] = CM_sat1 ((s1[0] + s1[4] + round1 ) >> BITWIDTH1);
    tmp[1+8*i] = CM_sat1 ((s1[1] + s1[5] + round1 ) >> BITWIDTH1);
    tmp[2+8*i] = CM_sat1 ((s1[2] + s1[6] + round1 ) >> BITWIDTH1);
    tmp[3+8*i] = CM_sat1 ((s1[3] + s1[7] + round1 ) >> BITWIDTH1);
    tmp[7+8*i] = CM_sat1 ((s1[0] - s1[4] + round1 ) >> BITWIDTH1);
    tmp[6+8*i] = CM_sat1 ((s1[1] - s1[5] + round1 ) >> BITWIDTH1);
    tmp[5+8*i] = CM_sat1 ((s1[2] - s1[6] + round1 ) >> BITWIDTH1);
    tmp[4+8*i] = CM_sat1 ((s1[3] - s1[7] + round1 ) >> BITWIDTH1);

    din += 1;
  }

  din2 = &tmp[0];
  for (i = 0; i < 8; i ++)
  {
    s3[0] = C4 * din2[0] + C2 * din2[16] + C4 * din2[32] + C6 * din2[48];
    s3[1] = C4 * din2[0] + C6 * din2[16] - C4 * din2[32] - C2 * din2[48];
    s3[2] = C4 * din2[0] - C6 * din2[16] - C4 * din2[32] + C2 * din2[48];
    s3[3] = C4 * din2[0] - C2 * din2[16] + C4 * din2[32] - C6 * din2[48];
    s3[4] = C1 * din2[8] + C3 * din2[24] + C5 * din2[40] + C7 * din2[56];
    s3[5] = C3 * din2[8] - C7 * din2[24] - C1 * din2[40] - C5 * din2[56];
    s3[6] = C5 * din2[8] - C1 * din2[24] + C7 * din2[40] + C3 * din2[56];
    s3[7] = C7 * din2[8] - C5 * din2[24] + C3 * din2[40] - C1 * din2[56];

    des[0+stride*i]  = gcm[ CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2))];
    des[2+stride*i]  = gcm[CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2))];
    des[4+stride*i]  = gcm[CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2))];
    des[6+stride*i]  = gcm[CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2))];
    des[14+stride*i]  = gcm[CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2))];
    des[12+stride*i]  = gcm[CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2))];
    des[10+stride*i]  = gcm[CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2))];
    des[8+stride*i]  = gcm[CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2))];  
    /*
    block[0+8*i] = CM_sat2 ((s3[0] + s3[4] + round2 ) >> BITWIDTH2);
    block[1+8*i] = CM_sat2 ((s3[1] + s3[5] + round2 ) >> BITWIDTH2);
    block[2+8*i] = CM_sat2 ((s3[2] + s3[6] + round2 ) >> BITWIDTH2);
    block[3+8*i] = CM_sat2 ((s3[3] + s3[7] + round2 ) >> BITWIDTH2);
    block[7+8*i] = CM_sat2 ((s3[0] - s3[4] + round2 ) >> BITWIDTH2);
    block[6+8*i] = CM_sat2 ((s3[1] - s3[5] + round2 ) >> BITWIDTH2);
    block[5+8*i] = CM_sat2 ((s3[2] - s3[6] + round2 ) >> BITWIDTH2);
    block[4+8*i] = CM_sat2 ((s3[3] - s3[7] + round2 ) >> BITWIDTH2);*/
    din2 += 1;
  }
#endif
}

void CM_IDCT_add_nv12(unsigned char*des,int stride, short *block)
{
#ifdef USE_15BIT_DCT_PRECISION
  // Version 2: DCT coeff 15 bits
  int   i;
  int   tmp[64]; // transposed matrix
  short *din;
  int   *din2;
  int   s1[8];
  long long s3[8];

  int   BITWIDTH1 = 9 + 1;      // 9  = S13.16 ->S13.6, 1 for (a+b)/2
  int   BITWIDTH2 = 15 + 6 + 1; // 15 = C1-C7 scaling, 6 = unscaled bits in row operation,  1 for (a+b)/2
  int   round1 = 1 << (BITWIDTH1 - 1);
  int   round2 = 1 << (BITWIDTH2 - 1);

  // DCT coefficient (s.15) in integer format
  const int C1 = 32138;
  const int C2 = 30273;
  const int C3 = 27245;
  const int C4 = 23170;
  const int C5 = 18204;
  const int C6 = 12539;
  const int C7 = 6392;

  // column first and then row
  din = &block[0];
  for (i = 0; i < 8; i ++)
  {
    s1[0] = C4 * din[0] + C2 * din[16] + C4 * din[32] + C6 * din[48];
    s1[1] = C4 * din[0] + C6 * din[16] - C4 * din[32] - C2 * din[48];
    s1[2] = C4 * din[0] - C6 * din[16] - C4 * din[32] + C2 * din[48];
    s1[3] = C4 * din[0] - C2 * din[16] + C4 * din[32] - C6 * din[48];
    s1[4] = C1 * din[8] + C3 * din[24] + C5 * din[40] + C7 * din[56];
    s1[5] = C3 * din[8] - C7 * din[24] - C1 * din[40] - C5 * din[56];
    s1[6] = C5 * din[8] - C1 * din[24] + C7 * din[40] + C3 * din[56];
    s1[7] = C7 * din[8] - C5 * din[24] + C3 * din[40] - C1 * din[56];

    tmp[0+8*i] = CM_sat3 ((s1[0] + s1[4] + round1 ) >> BITWIDTH1);
    tmp[1+8*i] = CM_sat3 ((s1[1] + s1[5] + round1 ) >> BITWIDTH1);
    tmp[2+8*i] = CM_sat3 ((s1[2] + s1[6] + round1 ) >> BITWIDTH1);
    tmp[3+8*i] = CM_sat3 ((s1[3] + s1[7] + round1 ) >> BITWIDTH1);
    tmp[7+8*i] = CM_sat3 ((s1[0] - s1[4] + round1 ) >> BITWIDTH1);
    tmp[6+8*i] = CM_sat3 ((s1[1] - s1[5] + round1 ) >> BITWIDTH1);
    tmp[5+8*i] = CM_sat3 ((s1[2] - s1[6] + round1 ) >> BITWIDTH1);
    tmp[4+8*i] = CM_sat3 ((s1[3] - s1[7] + round1 ) >> BITWIDTH1);

    din += 1;
  }  

  din2 = &tmp[0];
  for (i = 0; i < 8; i ++)
  {
    s3[0] = C4 * din2[0] + C2 * din2[16] + C4 * din2[32] + C6 * din2[48];
    s3[1] = C4 * din2[0] + C6 * din2[16] - C4 * din2[32] - C2 * din2[48];
    s3[2] = C4 * din2[0] - C6 * din2[16] - C4 * din2[32] + C2 * din2[48];
    s3[3] = C4 * din2[0] - C2 * din2[16] + C4 * din2[32] - C6 * din2[48];
    s3[4] = C1 * din2[8] + C3 * din2[24] + C5 * din2[40] + C7 * din2[56];
    s3[5] = C3 * din2[8] - C7 * din2[24] - C1 * din2[40] - C5 * din2[56];
    s3[6] = C5 * din2[8] - C1 * din2[24] + C7 * din2[40] + C3 * din2[56];
    s3[7] = C7 * din2[8] - C5 * din2[24] + C3 * din2[40] - C1 * din2[56];

    des[0+stride*i]  = gcm[des[0+stride*i] + CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2))];
    des[2+stride*i]  = gcm[des[2+stride*i] + CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2))];
    des[4+stride*i]  = gcm[des[4+stride*i] + CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2))];
    des[6+stride*i]  = gcm[des[6+stride*i] + CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2))];
    des[14+stride*i]  = gcm[des[14+stride*i] + CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2))];
    des[12+stride*i]  = gcm[des[12+stride*i] + CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2))];
    des[10+stride*i]  = gcm[des[10+stride*i] + CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2))];
    des[8+stride*i]  = gcm[des[8+stride*i] + CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2))];  
    
/*    block[0+8*i] = CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2));
    block[1+8*i] = CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2));
    block[2+8*i] = CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2));
    block[3+8*i] = CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2));
    block[7+8*i] = CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2));
    block[6+8*i] = CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2));
    block[5+8*i] = CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2));
    block[4+8*i] = CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2));*/
    din2 += 1;
  }
#else
  // Version 1: DCT coeff 13 bits
  int   i;
  int   tmp[64]; // transposed matrix
  short *din;
  int   *din2;
  int   s1[8], s3[8];
  int   BITWIDTH1 = 10 + 1;     // 10 = S13.14 ->S13.3, 1 for (a+b)/2
  int   BITWIDTH2 = 13 + 3 + 1; // 13 = C1-C7 scaling, 3 = unscaled bits in row operation,  1 for (a+b)/2
  int   round1 = 1 << (BITWIDTH1 - 1);
  int   round2 = 1 << (BITWIDTH2 - 1);
  const int C1 = 8035;
  const int C2 = 7568;
  const int C3 = 6811;
  const int C4 = 5793;
  const int C5 = 4551;
  const int C6 = 3135;
  const int C7 = 1598;
 
  // column first and then row
  din = &block[0];
  for (i = 0; i < 8; i ++)
  {
    s1[0] = C4 * din[0] + C2 * din[16] + C4 * din[32] + C6 * din[48];
    s1[1] = C4 * din[0] + C6 * din[16] - C4 * din[32] - C2 * din[48];
    s1[2] = C4 * din[0] - C6 * din[16] - C4 * din[32] + C2 * din[48];
    s1[3] = C4 * din[0] - C2 * din[16] + C4 * din[32] - C6 * din[48];
    s1[4] = C1 * din[8] + C3 * din[24] + C5 * din[40] + C7 * din[56];
    s1[5] = C3 * din[8] - C7 * din[24] - C1 * din[40] - C5 * din[56];
    s1[6] = C5 * din[8] - C1 * din[24] + C7 * din[40] + C3 * din[56];
    s1[7] = C7 * din[8] - C5 * din[24] + C3 * din[40] - C1 * din[56];

    tmp[0+8*i] = CM_sat1 ((s1[0] + s1[4] + round1 ) >> BITWIDTH1);
    tmp[1+8*i] = CM_sat1 ((s1[1] + s1[5] + round1 ) >> BITWIDTH1);
    tmp[2+8*i] = CM_sat1 ((s1[2] + s1[6] + round1 ) >> BITWIDTH1);
    tmp[3+8*i] = CM_sat1 ((s1[3] + s1[7] + round1 ) >> BITWIDTH1);
    tmp[7+8*i] = CM_sat1 ((s1[0] - s1[4] + round1 ) >> BITWIDTH1);
    tmp[6+8*i] = CM_sat1 ((s1[1] - s1[5] + round1 ) >> BITWIDTH1);
    tmp[5+8*i] = CM_sat1 ((s1[2] - s1[6] + round1 ) >> BITWIDTH1);
    tmp[4+8*i] = CM_sat1 ((s1[3] - s1[7] + round1 ) >> BITWIDTH1);

    din += 1;
  }

  din2 = &tmp[0];
  for (i = 0; i < 8; i ++)
  {
    s3[0] = C4 * din2[0] + C2 * din2[16] + C4 * din2[32] + C6 * din2[48];
    s3[1] = C4 * din2[0] + C6 * din2[16] - C4 * din2[32] - C2 * din2[48];
    s3[2] = C4 * din2[0] - C6 * din2[16] - C4 * din2[32] + C2 * din2[48];
    s3[3] = C4 * din2[0] - C2 * din2[16] + C4 * din2[32] - C6 * din2[48];
    s3[4] = C1 * din2[8] + C3 * din2[24] + C5 * din2[40] + C7 * din2[56];
    s3[5] = C3 * din2[8] - C7 * din2[24] - C1 * din2[40] - C5 * din2[56];
    s3[6] = C5 * din2[8] - C1 * din2[24] + C7 * din2[40] + C3 * din2[56];
    s3[7] = C7 * din2[8] - C5 * din2[24] + C3 * din2[40] - C1 * din2[56];

    des[0+stride*i]  = gcm[des[0+stride*i] + CM_sat2 ((int)((s3[0] + s3[4] + round2 ) >> BITWIDTH2))];
    des[2+stride*i]  = gcm[des[2+stride*i] + CM_sat2 ((int)((s3[1] + s3[5] + round2 ) >> BITWIDTH2))];
    des[4+stride*i]  = gcm[des[4+stride*i] + CM_sat2 ((int)((s3[2] + s3[6] + round2 ) >> BITWIDTH2))];
    des[6+stride*i]  = gcm[des[6+stride*i] + CM_sat2 ((int)((s3[3] + s3[7] + round2 ) >> BITWIDTH2))];
    des[14+stride*i]  = gcm[des[14+stride*i] + CM_sat2 ((int)((s3[0] - s3[4] + round2 ) >> BITWIDTH2))];
    des[12+stride*i]  = gcm[des[12+stride*i] + CM_sat2 ((int)((s3[1] - s3[5] + round2 ) >> BITWIDTH2))];
    des[10+stride*i]  = gcm[des[10+stride*i] + CM_sat2 ((int)((s3[2] - s3[6] + round2 ) >> BITWIDTH2))];
    des[8+stride*i]  = gcm[des[8+stride*i] + CM_sat2 ((int)((s3[3] - s3[7] + round2 ) >> BITWIDTH2))];  
    
    /*
    block[0+8*i] = CM_sat2 ((s3[0] + s3[4] + round2 ) >> BITWIDTH2);
    block[1+8*i] = CM_sat2 ((s3[1] + s3[5] + round2 ) >> BITWIDTH2);
    block[2+8*i] = CM_sat2 ((s3[2] + s3[6] + round2 ) >> BITWIDTH2);
    block[3+8*i] = CM_sat2 ((s3[3] + s3[7] + round2 ) >> BITWIDTH2);
    block[7+8*i] = CM_sat2 ((s3[0] - s3[4] + round2 ) >> BITWIDTH2);
    block[6+8*i] = CM_sat2 ((s3[1] - s3[5] + round2 ) >> BITWIDTH2);
    block[5+8*i] = CM_sat2 ((s3[2] - s3[6] + round2 ) >> BITWIDTH2);
    block[4+8*i] = CM_sat2 ((s3[3] - s3[7] + round2 ) >> BITWIDTH2);*/
    din2 += 1;
  }
#endif
}

#endif

static inline void idctRowCondDC (DCTELEM * row)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;
#if HAVE_FAST_64BIT
        uint64_t temp;
#else
        uint32_t temp;
#endif

#if HAVE_FAST_64BIT
#if HAVE_BIGENDIAN
#define ROW0_MASK 0xffff000000000000LL
#else
#define ROW0_MASK 0xffffLL
#endif
        if(sizeof(DCTELEM)==2){
            if ( ((((uint64_t *)row)[0] & ~ROW0_MASK) |
                  ((uint64_t *)row)[1]) == 0) {
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                temp += temp << 32;
                ((uint64_t *)row)[0] = temp;
                ((uint64_t *)row)[1] = temp;
                return;
            }
        }else{
            if (!(row[1]|row[2]|row[3]|row[4]|row[5]|row[6]|row[7])) {
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#else
        if(sizeof(DCTELEM)==2){
            if (!(((uint32_t*)row)[1] |
                  ((uint32_t*)row)[2] |
                  ((uint32_t*)row)[3] |
                  row[1])) {
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                ((uint32_t*)row)[0]=((uint32_t*)row)[1] =
                ((uint32_t*)row)[2]=((uint32_t*)row)[3] = temp;
                return;
            }
        }else{
            if (!(row[1]|row[2]|row[3]|row[4]|row[5]|row[6]|row[7])) {
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#endif

        a0 = (W4 * row[0]) + (1 << (ROW_SHIFT - 1));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        /* no need to optimize : gcc does it */
        a0 += W2 * row[2];
        a1 += W6 * row[2];
        a2 -= W6 * row[2];
        a3 -= W2 * row[2];

        b0 = MUL16(W1, row[1]);
        MAC16(b0, W3, row[3]);
        b1 = MUL16(W3, row[1]);
        MAC16(b1, -W7, row[3]);
        b2 = MUL16(W5, row[1]);
        MAC16(b2, -W1, row[3]);
        b3 = MUL16(W7, row[1]);
        MAC16(b3, -W5, row[3]);

#if HAVE_FAST_64BIT
        temp = ((uint64_t*)row)[1];
#else
        temp = ((uint32_t*)row)[2] | ((uint32_t*)row)[3];
#endif
        if (temp != 0) {
            a0 += W4*row[4] + W6*row[6];
            a1 += - W4*row[4] - W2*row[6];
            a2 += - W4*row[4] + W2*row[6];
            a3 += W4*row[4] - W6*row[6];

            MAC16(b0, W5, row[5]);
            MAC16(b0, W7, row[7]);

            MAC16(b1, -W1, row[5]);
            MAC16(b1, -W5, row[7]);

            MAC16(b2, W7, row[5]);
            MAC16(b2, W3, row[7]);

            MAC16(b3, W3, row[5]);
            MAC16(b3, -W1, row[7]);
        }

        row[0] = (a0 + b0) >> ROW_SHIFT;
        row[7] = (a0 - b0) >> ROW_SHIFT;
        row[1] = (a1 + b1) >> ROW_SHIFT;
        row[6] = (a1 - b1) >> ROW_SHIFT;
        row[2] = (a2 + b2) >> ROW_SHIFT;
        row[5] = (a2 - b2) >> ROW_SHIFT;
        row[3] = (a3 + b3) >> ROW_SHIFT;
        row[4] = (a3 - b3) >> ROW_SHIFT;
}

static inline void idctSparseColPut (uint8_t *dest, int line_size,
                                     DCTELEM * col)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;
        uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

        /* XXX: I did that only to give same values as previous code */
        a0 = W4 * (col[8*0] + ((1<<(COL_SHIFT-1))/W4));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        a0 +=  + W2*col[8*2];
        a1 +=  + W6*col[8*2];
        a2 +=  - W6*col[8*2];
        a3 +=  - W2*col[8*2];

        b0 = MUL16(W1, col[8*1]);
        b1 = MUL16(W3, col[8*1]);
        b2 = MUL16(W5, col[8*1]);
        b3 = MUL16(W7, col[8*1]);

        MAC16(b0, + W3, col[8*3]);
        MAC16(b1, - W7, col[8*3]);
        MAC16(b2, - W1, col[8*3]);
        MAC16(b3, - W5, col[8*3]);

        if(col[8*4]){
            a0 += + W4*col[8*4];
            a1 += - W4*col[8*4];
            a2 += - W4*col[8*4];
            a3 += + W4*col[8*4];
        }

        if (col[8*5]) {
            MAC16(b0, + W5, col[8*5]);
            MAC16(b1, - W1, col[8*5]);
            MAC16(b2, + W7, col[8*5]);
            MAC16(b3, + W3, col[8*5]);
        }

        if(col[8*6]){
            a0 += + W6*col[8*6];
            a1 += - W2*col[8*6];
            a2 += + W2*col[8*6];
            a3 += - W6*col[8*6];
        }

        if (col[8*7]) {
            MAC16(b0, + W7, col[8*7]);
            MAC16(b1, - W5, col[8*7]);
            MAC16(b2, + W3, col[8*7]);
            MAC16(b3, - W1, col[8*7]);
        }

        dest[0] = cm[(a0 + b0) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a1 + b1) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a2 + b2) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a3 + b3) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a3 - b3) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a2 - b2) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a1 - b1) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a0 - b0) >> COL_SHIFT];
}

static inline void idctSparseColAdd (uint8_t *dest, int line_size,
                                     DCTELEM * col)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;
        uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

        /* XXX: I did that only to give same values as previous code */
        a0 = W4 * (col[8*0] + ((1<<(COL_SHIFT-1))/W4));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        a0 +=  + W2*col[8*2];
        a1 +=  + W6*col[8*2];
        a2 +=  - W6*col[8*2];
        a3 +=  - W2*col[8*2];

        b0 = MUL16(W1, col[8*1]);
        b1 = MUL16(W3, col[8*1]);
        b2 = MUL16(W5, col[8*1]);
        b3 = MUL16(W7, col[8*1]);

        MAC16(b0, + W3, col[8*3]);
        MAC16(b1, - W7, col[8*3]);
        MAC16(b2, - W1, col[8*3]);
        MAC16(b3, - W5, col[8*3]);

        if(col[8*4]){
            a0 += + W4*col[8*4];
            a1 += - W4*col[8*4];
            a2 += - W4*col[8*4];
            a3 += + W4*col[8*4];
        }

        if (col[8*5]) {
            MAC16(b0, + W5, col[8*5]);
            MAC16(b1, - W1, col[8*5]);
            MAC16(b2, + W7, col[8*5]);
            MAC16(b3, + W3, col[8*5]);
        }

        if(col[8*6]){
            a0 += + W6*col[8*6];
            a1 += - W2*col[8*6];
            a2 += + W2*col[8*6];
            a3 += - W6*col[8*6];
        }

        if (col[8*7]) {
            MAC16(b0, + W7, col[8*7]);
            MAC16(b1, - W5, col[8*7]);
            MAC16(b2, + W3, col[8*7]);
            MAC16(b3, - W1, col[8*7]);
        }

        dest[0] = cm[dest[0] + ((a0 + b0) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a1 + b1) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a2 + b2) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a3 + b3) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a3 - b3) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a2 - b2) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a1 - b1) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a0 - b0) >> COL_SHIFT)];
}

static inline void idctSparseCol (DCTELEM * col)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;

        /* XXX: I did that only to give same values as previous code */
        a0 = W4 * (col[8*0] + ((1<<(COL_SHIFT-1))/W4));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        a0 +=  + W2*col[8*2];
        a1 +=  + W6*col[8*2];
        a2 +=  - W6*col[8*2];
        a3 +=  - W2*col[8*2];

        b0 = MUL16(W1, col[8*1]);
        b1 = MUL16(W3, col[8*1]);
        b2 = MUL16(W5, col[8*1]);
        b3 = MUL16(W7, col[8*1]);

        MAC16(b0, + W3, col[8*3]);
        MAC16(b1, - W7, col[8*3]);
        MAC16(b2, - W1, col[8*3]);
        MAC16(b3, - W5, col[8*3]);

        if(col[8*4]){
            a0 += + W4*col[8*4];
            a1 += - W4*col[8*4];
            a2 += - W4*col[8*4];
            a3 += + W4*col[8*4];
        }

        if (col[8*5]) {
            MAC16(b0, + W5, col[8*5]);
            MAC16(b1, - W1, col[8*5]);
            MAC16(b2, + W7, col[8*5]);
            MAC16(b3, + W3, col[8*5]);
        }

        if(col[8*6]){
            a0 += + W6*col[8*6];
            a1 += - W2*col[8*6];
            a2 += + W2*col[8*6];
            a3 += - W6*col[8*6];
        }

        if (col[8*7]) {
            MAC16(b0, + W7, col[8*7]);
            MAC16(b1, - W5, col[8*7]);
            MAC16(b2, + W3, col[8*7]);
            MAC16(b3, - W1, col[8*7]);
        }

        col[0] = (a0 + b0) >> COL_SHIFT;
        col[8*1] = (a1 + b1) >> COL_SHIFT;
        col[8*2] = (a2 + b2) >> COL_SHIFT;
        col[8*3] = (a3 + b3) >> COL_SHIFT;
        col[8*4] = (a3 - b3) >> COL_SHIFT;
        col[8*5] = (a2 - b2) >> COL_SHIFT;
        col[8*6] = (a1 - b1) >> COL_SHIFT;
        col[8*7] = (a0 - b0) >> COL_SHIFT;
}

void ff_simple_idct_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;
    for(i=0; i<8; i++)
        idctRowCondDC(block + i*8);

    for(i=0; i<8; i++)
        idctSparseColPut(dest + i, line_size, block + i);
}

void ff_simple_idct_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;
    for(i=0; i<8; i++)
        idctRowCondDC(block + i*8);

    for(i=0; i<8; i++)
        idctSparseColAdd(dest + i, line_size, block + i);
}

void ff_simple_idct(DCTELEM *block)
{
    int i;
    for(i=0; i<8; i++)
        idctRowCondDC(block + i*8);

    for(i=0; i<8; i++)
        idctSparseCol(block + i);
}

/* 2x4x8 idct */

#define CN_SHIFT 12
#define C_FIX(x) ((int)((x) * (1 << CN_SHIFT) + 0.5))
#define C1 C_FIX(0.6532814824)
#define C2 C_FIX(0.2705980501)

/* row idct is multiple by 16 * sqrt(2.0), col idct4 is normalized,
   and the butterfly must be multiplied by 0.5 * sqrt(2.0) */
#define C_SHIFT (4+1+12)

static inline void idct4col_put(uint8_t *dest, int line_size, const DCTELEM *col)
{
    int c0, c1, c2, c3, a0, a1, a2, a3;
    const uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    a0 = col[8*0];
    a1 = col[8*2];
    a2 = col[8*4];
    a3 = col[8*6];
    c0 = ((a0 + a2) << (CN_SHIFT - 1)) + (1 << (C_SHIFT - 1));
    c2 = ((a0 - a2) << (CN_SHIFT - 1)) + (1 << (C_SHIFT - 1));
    c1 = a1 * C1 + a3 * C2;
    c3 = a1 * C2 - a3 * C1;
    dest[0] = cm[(c0 + c1) >> C_SHIFT];
    dest += line_size;
    dest[0] = cm[(c2 + c3) >> C_SHIFT];
    dest += line_size;
    dest[0] = cm[(c2 - c3) >> C_SHIFT];
    dest += line_size;
    dest[0] = cm[(c0 - c1) >> C_SHIFT];
}

#define BF(k) \
{\
    int a0, a1;\
    a0 = ptr[k];\
    a1 = ptr[8 + k];\
    ptr[k] = a0 + a1;\
    ptr[8 + k] = a0 - a1;\
}

/* only used by DV codec. The input must be interlaced. 128 is added
   to the pixels before clamping to avoid systematic error
   (1024*sqrt(2)) offset would be needed otherwise. */
/* XXX: I think a 1.0/sqrt(2) normalization should be needed to
   compensate the extra butterfly stage - I don't have the full DV
   specification */
void ff_simple_idct248_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;
    DCTELEM *ptr;

    /* butterfly */
    ptr = block;
    for(i=0;i<4;i++) {
        BF(0);
        BF(1);
        BF(2);
        BF(3);
        BF(4);
        BF(5);
        BF(6);
        BF(7);
        ptr += 2 * 8;
    }

    /* IDCT8 on each line */
    for(i=0; i<8; i++) {
        idctRowCondDC(block + i*8);
    }

    /* IDCT4 and store */
    for(i=0;i<8;i++) {
        idct4col_put(dest + i, 2 * line_size, block + i);
        idct4col_put(dest + line_size + i, 2 * line_size, block + 8 + i);
    }
}

/* 8x4 & 4x8 WMV2 IDCT */
#undef CN_SHIFT
#undef C_SHIFT
#undef C_FIX
#undef C1
#undef C2
#define CN_SHIFT 12
#define C_FIX(x) ((int)((x) * 1.414213562 * (1 << CN_SHIFT) + 0.5))
#define C1 C_FIX(0.6532814824)
#define C2 C_FIX(0.2705980501)
#define C3 C_FIX(0.5)
#define C_SHIFT (4+1+12)
static inline void idct4col_add(uint8_t *dest, int line_size, const DCTELEM *col)
{
    int c0, c1, c2, c3, a0, a1, a2, a3;
    const uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    a0 = col[8*0];
    a1 = col[8*1];
    a2 = col[8*2];
    a3 = col[8*3];
    c0 = (a0 + a2)*C3 + (1 << (C_SHIFT - 1));
    c2 = (a0 - a2)*C3 + (1 << (C_SHIFT - 1));
    c1 = a1 * C1 + a3 * C2;
    c3 = a1 * C2 - a3 * C1;
    dest[0] = cm[dest[0] + ((c0 + c1) >> C_SHIFT)];
    dest += line_size;
    dest[0] = cm[dest[0] + ((c2 + c3) >> C_SHIFT)];
    dest += line_size;
    dest[0] = cm[dest[0] + ((c2 - c3) >> C_SHIFT)];
    dest += line_size;
    dest[0] = cm[dest[0] + ((c0 - c1) >> C_SHIFT)];
}

#define RN_SHIFT 15
#define R_FIX(x) ((int)((x) * 1.414213562 * (1 << RN_SHIFT) + 0.5))
#define R1 R_FIX(0.6532814824)
#define R2 R_FIX(0.2705980501)
#define R3 R_FIX(0.5)
#define R_SHIFT 11
static inline void idct4row(DCTELEM *row)
{
    int c0, c1, c2, c3, a0, a1, a2, a3;
    //const uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    a0 = row[0];
    a1 = row[1];
    a2 = row[2];
    a3 = row[3];
    c0 = (a0 + a2)*R3 + (1 << (R_SHIFT - 1));
    c2 = (a0 - a2)*R3 + (1 << (R_SHIFT - 1));
    c1 = a1 * R1 + a3 * R2;
    c3 = a1 * R2 - a3 * R1;
    row[0]= (c0 + c1) >> R_SHIFT;
    row[1]= (c2 + c3) >> R_SHIFT;
    row[2]= (c2 - c3) >> R_SHIFT;
    row[3]= (c0 - c1) >> R_SHIFT;
}

void ff_simple_idct84_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;

    /* IDCT8 on each line */
    for(i=0; i<4; i++) {
        idctRowCondDC(block + i*8);
    }

    /* IDCT4 and store */
    for(i=0;i<8;i++) {
        idct4col_add(dest + i, line_size, block + i);
    }
}

void ff_simple_idct48_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;

    /* IDCT4 on each line */
    for(i=0; i<8; i++) {
        idct4row(block + i*8);
    }

    /* IDCT8 and store */
    for(i=0; i<4; i++){
        idctSparseColAdd(dest + i, line_size, block + i);
    }
}

void ff_simple_idct44_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;

    /* IDCT4 on each line */
    for(i=0; i<4; i++) {
        idct4row(block + i*8);
    }

    /* IDCT4 and store */
    for(i=0; i<4; i++){
        idct4col_add(dest + i, line_size, block + i);
    }
}

#ifdef __use_reference_dct__
/* initialize DCT coefficient matrix */
void Initialize_Reference_IDCT()
{
  int freq, time;
  double scale;

  for (freq=0; freq < 8; freq++)
  {
    scale = (freq == 0) ? sqrt(0.125) : 0.5;
    for (time=0; time<8; time++)
      c[freq][time] = scale*cos((gpi/8.0)*freq*(time + 0.5));
  }
}

void Reference_IDCT(short *block)
{
  int i, j, k, v;
  double partial_product;
  double tmp[64];

  for (i=0; i<8; i++)
    for (j=0; j<8; j++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][j]*block[8*i+k];

      tmp[8*i+j] = partial_product;
    }

  /* Transpose operation is integrated into address mapping by switching 
     loop order of i and j */

  for (j=0; j<8; j++)
    for (i=0; i<8; i++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][i]*tmp[8*k+j];

      v = (int) floor(partial_product+0.5);
      block[8*i+j] = (v<-256) ? -256 : ((v>255) ? 255 : v);
    }
}

void Reference_IDCT_put(unsigned char* des, int stride, short *block)
{
  int i, j, k, v;
  double partial_product;
  double tmp[64];

  for (i=0; i<8; i++)
    for (j=0; j<8; j++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][j]*block[8*i+k];

      tmp[8*i+j] = partial_product;
    }

  /* Transpose operation is integrated into address mapping by switching 
     loop order of i and j */

  for (j=0; j<8; j++)
    for (i=0; i<8; i++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][i]*tmp[8*k+j];

      v = (int) floor(partial_product+0.5);
      des[j+i*stride]=gcm[v];
      //block[8*i+j] = (v<-256) ? -256 : ((v>255) ? 255 : v);
    }
}

void Reference_IDCT_put_nv12(unsigned char* des, int stride, short *block)
{
  int i, j, k, v;
  double partial_product;
  double tmp[64];

  for (i=0; i<8; i++)
    for (j=0; j<8; j++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][j]*block[8*i+k];

      tmp[8*i+j] = partial_product;
    }

  /* Transpose operation is integrated into address mapping by switching 
     loop order of i and j */

  for (j=0; j<8; j++)
    for (i=0; i<8; i++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][i]*tmp[8*k+j];

      v = (int) floor(partial_product+0.5);
      des[j*2+i*stride]=gcm[v];
      //block[8*i+j] = (v<-256) ? -256 : ((v>255) ? 255 : v);
    }
}

void Reference_IDCT_add(unsigned char* des, int stride, short *block)
{
  int i, j, k, v;
  double partial_product;
  double tmp[64];

  for (i=0; i<8; i++)
    for (j=0; j<8; j++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][j]*block[8*i+k];

      tmp[8*i+j] = partial_product;
    }

  /* Transpose operation is integrated into address mapping by switching 
     loop order of i and j */

  for (j=0; j<8; j++)
    for (i=0; i<8; i++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][i]*tmp[8*k+j];

      v = (int) floor(partial_product+0.5);
      des[j+i*stride]=gcm[v+des[j+i*stride]];
      //block[8*i+j] = (v<-256) ? -256 : ((v>255) ? 255 : v);
    }
}

void Reference_IDCT_add_nv12(unsigned char* des, int stride, short *block)
{
  int i, j, k, v;
  double partial_product;
  double tmp[64];

  for (i=0; i<8; i++)
    for (j=0; j<8; j++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][j]*block[8*i+k];

      tmp[8*i+j] = partial_product;
    }

  /* Transpose operation is integrated into address mapping by switching 
     loop order of i and j */

  for (j=0; j<8; j++)
    for (i=0; i<8; i++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= c[k][i]*tmp[8*k+j];

      v = (int) floor(partial_product+0.5);
      des[j*2+i*stride]=gcm[v+des[j*2+i*stride]];
      //block[8*i+j] = (v<-256) ? -256 : ((v>255) ? 255 : v);
    }
}
#endif

static inline void idctRowCondDC_wmv2 (DCTELEM * row, DCTELEM * mid)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;
#if HAVE_FAST_64BIT
        uint64_t temp;
#else
        uint32_t temp;
#endif

#if HAVE_FAST_64BIT
#if HAVE_BIGENDIAN
#define ROW0_MASK 0xffff000000000000LL
#else
#define ROW0_MASK 0xffffLL
#endif
        if(sizeof(DCTELEM)==2){
            if ( ((((uint64_t *)row)[0] & ~ROW0_MASK) |
                  ((uint64_t *)row)[1]) == 0) {
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                temp += temp << 32;
                ((uint64_t *)row)[0] = temp;
                ((uint64_t *)row)[1] = temp;
                return;
            }
        }else{
            if (!(row[1]|row[2]|row[3]|row[4]|row[5]|row[6]|row[7])) {
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#else
        if(sizeof(DCTELEM)==2){
            if (!(((uint32_t*)row)[1] |
                  ((uint32_t*)row)[2] |
                  ((uint32_t*)row)[3] |
                  row[1])) {
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                ((uint32_t*)row)[0]=((uint32_t*)row)[1] =
                ((uint32_t*)row)[2]=((uint32_t*)row)[3] = temp;
                return;
            }
        }else{
            if (!(row[1]|row[2]|row[3]|row[4]|row[5]|row[6]|row[7])) {
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#endif

        a0 = (W4 * row[0]) + (1 << (ROW_SHIFT - 1));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        /* no need to optimize : gcc does it */
        a0 += W2 * row[2];
        a1 += W6 * row[2];
        a2 -= W6 * row[2];
        a3 -= W2 * row[2];

        b0 = MUL16(W1, row[1]);
        MAC16(b0, W3, row[3]);
        b1 = MUL16(W3, row[1]);
        MAC16(b1, -W7, row[3]);
        b2 = MUL16(W5, row[1]);
        MAC16(b2, -W1, row[3]);
        b3 = MUL16(W7, row[1]);
        MAC16(b3, -W5, row[3]);

#if HAVE_FAST_64BIT
        temp = ((uint64_t*)row)[1];
#else
        temp = ((uint32_t*)row)[2] | ((uint32_t*)row)[3];
#endif
        if (temp != 0) {
            a0 += W4*row[4] + W6*row[6];
            a1 += - W4*row[4] - W2*row[6];
            a2 += - W4*row[4] + W2*row[6];
            a3 += W4*row[4] - W6*row[6];

            MAC16(b0, W5, row[5]);
            MAC16(b0, W7, row[7]);

            MAC16(b1, -W1, row[5]);
            MAC16(b1, -W5, row[7]);

            MAC16(b2, W7, row[5]);
            MAC16(b2, W3, row[7]);

            MAC16(b3, W3, row[5]);
            MAC16(b3, -W1, row[7]);
        }

        mid[0] = (a0 + b0) >> ROW_SHIFT;
        mid[7] = (a0 - b0) >> ROW_SHIFT;
        mid[1] = (a1 + b1) >> ROW_SHIFT;
        mid[6] = (a1 - b1) >> ROW_SHIFT;
        mid[2] = (a2 + b2) >> ROW_SHIFT;
        mid[5] = (a2 - b2) >> ROW_SHIFT;
        mid[3] = (a3 + b3) >> ROW_SHIFT;
        mid[4] = (a3 - b3) >> ROW_SHIFT;
}

static inline void idctSparseCol_wmv2 (DCTELEM * col, DCTELEM * des)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;

        /* XXX: I did that only to give same values as previous code */
        a0 = W4 * (col[8*0] + ((1<<(COL_SHIFT-1))/W4));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        a0 +=  + W2*col[8*2];
        a1 +=  + W6*col[8*2];
        a2 +=  - W6*col[8*2];
        a3 +=  - W2*col[8*2];

        b0 = MUL16(W1, col[8*1]);
        b1 = MUL16(W3, col[8*1]);
        b2 = MUL16(W5, col[8*1]);
        b3 = MUL16(W7, col[8*1]);

        MAC16(b0, + W3, col[8*3]);
        MAC16(b1, - W7, col[8*3]);
        MAC16(b2, - W1, col[8*3]);
        MAC16(b3, - W5, col[8*3]);

        if(col[8*4]){
            a0 += + W4*col[8*4];
            a1 += - W4*col[8*4];
            a2 += - W4*col[8*4];
            a3 += + W4*col[8*4];
        }

        if (col[8*5]) {
            MAC16(b0, + W5, col[8*5]);
            MAC16(b1, - W1, col[8*5]);
            MAC16(b2, + W7, col[8*5]);
            MAC16(b3, + W3, col[8*5]);
        }

        if(col[8*6]){
            a0 += + W6*col[8*6];
            a1 += - W2*col[8*6];
            a2 += + W2*col[8*6];
            a3 += - W6*col[8*6];
        }

        if (col[8*7]) {
            MAC16(b0, + W7, col[8*7]);
            MAC16(b1, - W5, col[8*7]);
            MAC16(b2, + W3, col[8*7]);
            MAC16(b3, - W1, col[8*7]);
        }

        des[0 ] = ((a0 + b0) >> COL_SHIFT);
        des[8 ] = ((a1 + b1) >> COL_SHIFT);
        des[16] = ((a2 + b2) >> COL_SHIFT);
        des[24] = ((a3 + b3) >> COL_SHIFT);
        des[32] = ((a3 - b3) >> COL_SHIFT);
        des[40] = ((a2 - b2) >> COL_SHIFT);
        des[48] = ((a1 - b1) >> COL_SHIFT);
        des[56] = ((a0 - b0) >> COL_SHIFT);
}

void ff_simple_idct_put_nv12(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;
    for(i=0; i<8; i++)
        idctRowCondDC(block + i*8);

    for(i=0; i<8; i++)
        idctSparseColPut(dest + i*2, line_size, block + i);
}

void ff_simple_idct_add_nv12(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;
    for(i=0; i<8; i++)
        idctRowCondDC(block + i*8);

    for(i=0; i<8; i++)
        idctSparseColAdd(dest + i*2, line_size, block + i);
}

void ff_simple_add_block(DCTELEM *block, uint8_t* des, int linesize)
{
    int i=0;
    for(i=0;i<8;i++,des+=linesize)
    {
        des[0]=gcm[des[0]+(*block++)];
        des[1]=gcm[des[1]+(*block++)];
        des[2]=gcm[des[2]+(*block++)];
        des[3]=gcm[des[3]+(*block++)];
        des[4]=gcm[des[4]+(*block++)];
        des[5]=gcm[des[5]+(*block++)];
        des[6]=gcm[des[6]+(*block++)];
        des[7]=gcm[des[7]+(*block++)];
    }
}

void ff_simple_add_block_nv12(DCTELEM *block, uint8_t* des, int linesize)
{
    int i=0;
    for(i=0;i<8;i++,des+=linesize)
    {
        des[0]=gcm[des[0]+(*block++)];
        des[2]=gcm[des[2]+(*block++)];
        des[4]=gcm[des[4]+(*block++)];
        des[6]=gcm[des[6]+(*block++)];
        des[8]=gcm[des[8]+(*block++)];
        des[10]=gcm[des[10]+(*block++)];
        des[12]=gcm[des[12]+(*block++)];
        des[14]=gcm[des[14]+(*block++)];
    }
}

static inline void idct4col_wmv2(const DCTELEM *col,DCTELEM *des)
{
    int c0, c1, c2, c3, a0, a1, a2, a3;
//    const uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    a0 = col[8*0];
    a1 = col[8*1];
    a2 = col[8*2];
    a3 = col[8*3];
    c0 = (a0 + a2)*C3 + (1 << (C_SHIFT - 1));
    c2 = (a0 - a2)*C3 + (1 << (C_SHIFT - 1));
    c1 = a1 * C1 + a3 * C2;
    c3 = a1 * C2 - a3 * C1;
    des[8*0] = ((c0 + c1) >> C_SHIFT);
    des[8*1] = ((c2 + c3) >> C_SHIFT);
    des[8*2]=  ((c2 - c3) >> C_SHIFT);
    des[8*3]=  ((c0 - c1) >> C_SHIFT);
}

static inline void idct4row_wmv2(DCTELEM *row,DCTELEM *mid)
{
    int c0, c1, c2, c3, a0, a1, a2, a3;
    //const uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    a0 = row[0];
    a1 = row[1];
    a2 = row[2];
    a3 = row[3];
    c0 = (a0 + a2)*R3 + (1 << (R_SHIFT - 1));
    c2 = (a0 - a2)*R3 + (1 << (R_SHIFT - 1));
    c1 = a1 * R1 + a3 * R2;
    c3 = a1 * R2 - a3 * R1;
    mid[0]= (c0 + c1) >> R_SHIFT;
    mid[1]= (c2 + c3) >> R_SHIFT;
    mid[2]= (c2 - c3) >> R_SHIFT;
    mid[3]= (c0 - c1) >> R_SHIFT;
}

void ff_simple_idct84_wmv2(DCTELEM *block)
{
    int i;

    /* IDCT8 on each line */
    for(i=0; i<4; i++) {
        idctRowCondDC_wmv2(block + i*8, g_transform+i*8);
    }

    /* IDCT4 and store */
    for(i=0;i<8;i++) {
        idct4col_wmv2(g_transform+i, block + i);
    }
}

void ff_simple_idct84_wmv2_des(DCTELEM *src,DCTELEM *block)
{
    int i;

    /* IDCT8 on each line */
    for(i=0; i<4; i++) {
        idctRowCondDC_wmv2(src + i*8, g_transform+i*8);
    }

    /* IDCT4 and store */
    for(i=0;i<8;i++) {
        idct4col_wmv2(g_transform+i, block + i);
    }
}

void ff_simple_idct48_wmv2_des(DCTELEM *src,DCTELEM *block)
{
    int i;

    /* IDCT4 on each line */
    for(i=0; i<8; i++) {
        idct4row_wmv2(src + i*8,g_transform+i*8);
    }

    /* IDCT8 and store */
    for(i=0; i<4; i++){
        idctSparseCol_wmv2(g_transform+i,block + i);
    }
}

void ff_simple_idct48_wmv2(DCTELEM *block)
{
    int i;

    /* IDCT4 on each line */
    for(i=0; i<8; i++) {
        idct4row_wmv2(block + i*8,g_transform+i*8);
    }

    /* IDCT8 and store */
    for(i=0; i<4; i++){
        idctSparseCol_wmv2(g_transform+i,block + i);
    }
}