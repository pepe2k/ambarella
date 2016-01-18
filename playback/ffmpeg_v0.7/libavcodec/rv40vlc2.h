/*
 * RealVideo 4 decoder
 * copyright (c) 2007 Konstantin Shishkov
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
 * RV40 VLC tables used for macroblock information decoding
 */

#ifndef AVCODEC_RV40VLC2_H
#define AVCODEC_RV40VLC2_H

#include <stdint.h>

/**
 * codes used for the first four block types
 */
//@{
#define AIC_TOP_BITS  8
#define AIC_TOP_SIZE 16
extern const uint8_t rv40_aic_top_vlc_codes[AIC_TOP_SIZE];

extern const uint8_t rv40_aic_top_vlc_bits[AIC_TOP_SIZE];

//@}

/**
 * codes used for determining a pair of block types
 */
//@{
#define AIC_MODE2_NUM  20
#define AIC_MODE2_SIZE 81
#define AIC_MODE2_BITS  9

extern const uint16_t aic_mode2_vlc_codes[AIC_MODE2_NUM][AIC_MODE2_SIZE];

extern const uint8_t aic_mode2_vlc_bits[AIC_MODE2_NUM][AIC_MODE2_SIZE];

//@}

/**
 * Codes used for determining block type
 */
//@{
#define AIC_MODE1_NUM  90
#define AIC_MODE1_SIZE  9
#define AIC_MODE1_BITS  7

extern const uint8_t aic_mode1_vlc_codes[AIC_MODE1_NUM][AIC_MODE1_SIZE];

extern const uint8_t aic_mode1_vlc_bits[AIC_MODE1_NUM][AIC_MODE1_SIZE];

//@}

#define PBTYPE_ESCAPE 0xFF

/** tables used for P-frame macroblock type decoding */
//@{
#define NUM_PTYPE_VLCS 7
#define PTYPE_VLC_SIZE 8
#define PTYPE_VLC_BITS 7

extern const uint8_t ptype_vlc_codes[NUM_PTYPE_VLCS][PTYPE_VLC_SIZE];

extern const uint8_t ptype_vlc_bits[NUM_PTYPE_VLCS][PTYPE_VLC_SIZE];

extern const uint8_t ptype_vlc_syms[PTYPE_VLC_SIZE];

/** reverse of ptype_vlc_syms */
extern const uint8_t block_num_to_ptype_vlc_num[12];
//@}

/** tables used for P-frame macroblock type decoding */
//@{
#define NUM_BTYPE_VLCS 6
#define BTYPE_VLC_SIZE 7
#define BTYPE_VLC_BITS 6

extern const uint8_t btype_vlc_codes[NUM_BTYPE_VLCS][BTYPE_VLC_SIZE];

extern const uint8_t btype_vlc_bits[NUM_BTYPE_VLCS][PTYPE_VLC_SIZE];

extern const uint8_t btype_vlc_syms[BTYPE_VLC_SIZE];

/** reverse of btype_vlc_syms */
extern const uint8_t block_num_to_btype_vlc_num[12];
//@}


extern VLC aic_top_vlc;
extern VLC aic_mode1_vlc[AIC_MODE1_NUM];
extern VLC aic_mode2_vlc[AIC_MODE2_NUM];
extern VLC ptype_vlc[NUM_PTYPE_VLCS];
extern VLC btype_vlc[NUM_BTYPE_VLCS];

extern const int16_t mode2_offs[];

#endif /* AVCODEC_RV40VLC2_H */
