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
 * simple idct header.
 */

#ifndef AVCODEC_SIMPLE_IDCT_H
#define AVCODEC_SIMPLE_IDCT_H

#include <stdint.h>
#include "dsputil.h"

void ff_simple_idct_put(uint8_t *dest, int line_size, DCTELEM *block);
void ff_simple_idct_add(uint8_t *dest, int line_size, DCTELEM *block);
void ff_simple_idct_mmx(int16_t *block);
void ff_simple_idct_add_mmx(uint8_t *dest, int line_size, int16_t *block);
void ff_simple_idct_put_mmx(uint8_t *dest, int line_size, int16_t *block);
void ff_simple_idct(DCTELEM *block);

void ff_simple_idct248_put(uint8_t *dest, int line_size, DCTELEM *block);

void ff_simple_idct84_add(uint8_t *dest, int line_size, DCTELEM *block);
void ff_simple_idct48_add(uint8_t *dest, int line_size, DCTELEM *block);
void ff_simple_idct44_add(uint8_t *dest, int line_size, DCTELEM *block);

void ff_simple_idct_put_nv12(uint8_t *dest, int line_size, DCTELEM *block);
void ff_simple_idct_add_nv12(uint8_t *dest, int line_size, DCTELEM *block);

void ff_simple_add_block(DCTELEM *block, uint8_t* des, int linesize);
void ff_simple_add_block_nv12(DCTELEM *block, uint8_t* des, int linesize);

void ff_simple_idct84_wmv2(DCTELEM *block);
void ff_simple_idct84_wmv2_des(DCTELEM *src,DCTELEM *block);

void ff_simple_idct48_wmv2_des(DCTELEM *src,DCTELEM *block);
void ff_simple_idct48_wmv2(DCTELEM *block);

#ifdef __use_reference_dct__
//reference DCT
void Initialize_Reference_IDCT();
void Reference_IDCT(short *block);
void Reference_IDCT_put(unsigned char* des, int stride, short *block);
void Reference_IDCT_put_nv12(unsigned char* des, int stride, short *block);
void Reference_IDCT_add(unsigned char* des, int stride, short *block);
void Reference_IDCT_add_nv12(unsigned char* des, int stride, short *block);
#endif

#ifdef __use_cm_dct__
int CM_sat1(int i);
int CM_sat2(int i);
int CM_sat3(int i);

//reference DCT
void CM_IDCT(short *block);
void CM_IDCT_put(unsigned char* des, int stride, short *block);
void CM_IDCT_put_nv12(unsigned char* des, int stride, short *block);
void CM_IDCT_add(unsigned char* des, int stride, short *block);
void CM_IDCT_add_nv12(unsigned char* des, int stride, short *block);
#endif

#endif /* AVCODEC_SIMPLE_IDCT_H */
