/*
 * copyright (c) 2000,2001 Fabrice Bellard
 * H263+ support
 * copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
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
 * @file libavcodec/mpeg4data_extern.h
 * mpeg4 tables.
 */

#ifndef AVCODEC_MPEG4DATA_EXTERN_H
#define AVCODEC_MPEG4DATA_EXTERN_H

// shapes
#define RECT_SHAPE       0
#define BIN_SHAPE        1
#define BIN_ONLY_SHAPE   2
#define GRAY_SHAPE       3

#define SIMPLE_VO_TYPE             1
#define CORE_VO_TYPE               3
#define MAIN_VO_TYPE               4
#define NBIT_VO_TYPE               5
#define ARTS_VO_TYPE               10
#define ACE_VO_TYPE                12
#define ADV_SIMPLE_VO_TYPE         17

// aspect_ratio_info
#define EXTENDED_PAR 15

//vol_sprite_usage / sprite_enable
#define STATIC_SPRITE 1
#define GMC_SPRITE 2

#define MOTION_MARKER 0x1F001
#define DC_MARKER     0x6B001

extern const int mb_type_b_map[4];

#define VOS_STARTCODE        0x1B0
#define USER_DATA_STARTCODE  0x1B2
#define GOP_STARTCODE        0x1B3
#define VISUAL_OBJ_STARTCODE 0x1B5
#define VOP_STARTCODE        0x1B6

/* dc encoding for mpeg4 */
extern const uint8_t ff_mpeg4_DCtab_lum[13][2] ;

extern const uint8_t ff_mpeg4_DCtab_chrom[13][2] ;

extern const uint16_t ff_mpeg4_intra_vlc[103][2] ;

extern const int8_t ff_mpeg4_intra_level[102] ;

extern const int8_t ff_mpeg4_intra_run[102] ;

//extern  RLTable ff_mpeg4_rl_intra ;

/* Note this is identical to the intra rvlc except that it is reordered. */
extern const uint16_t inter_rvlc[170][2];

extern const int8_t inter_rvlc_run[169];

extern const int8_t inter_rvlc_level[169];

//extern RLTable rvlc_rl_inter ;

extern const uint16_t intra_rvlc[170][2];

extern const int8_t intra_rvlc_run[169];

extern const int8_t intra_rvlc_level[169];

//extern RLTable rvlc_rl_intra;
extern const uint16_t sprite_trajectory_tab[15][2];
extern const uint8_t mb_type_b_tab[4][2];
extern const int16_t ff_mpeg4_default_intra_matrix[64];
extern const int16_t ff_mpeg4_default_non_intra_matrix[64];
extern const uint8_t ff_mpeg4_y_dc_scale_table[32];
extern const uint8_t ff_mpeg4_c_dc_scale_table[32];
extern const uint16_t ff_mpeg4_resync_prefix[8];

extern const uint8_t mpeg4_dc_threshold[8];
extern const AVRational pixel_aspect[16];
#endif /* AVCODEC_MPEG4DATA_H */
