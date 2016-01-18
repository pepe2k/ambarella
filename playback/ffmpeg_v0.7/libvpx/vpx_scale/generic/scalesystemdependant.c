/*
 *  Copyright (c) 2010 The VP8 project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license and patent
 *  grant that can be found in the LICENSE file in the root of the source
 *  tree. All contributing project authors may be found in the AUTHORS
 *  file in the root of the source tree.
 */


#include "vpx_scale/vpxscale.h"

#ifdef HAVE_CONFIG_H
#include "vpx_config.h"
#endif

void (*vp8_yv12_extend_frame_borders_ptr)(YV12_BUFFER_CONFIG *ybf);
extern void vp8_yv12_extend_frame_borders(YV12_BUFFER_CONFIG *ybf);
extern void vp8_yv12_extend_frame_borders_neon(YV12_BUFFER_CONFIG *ybf);

void (*vp8_yv12_copy_frame_yonly_ptr)(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);
extern void vp8_yv12_copy_frame_yonly(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);
extern void vp8_yv12_copy_frame_yonly_neon(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);

void (*vp8_yv12_copy_frame_ptr)(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);
extern void vp8_yv12_copy_frame(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);
extern void vp8_yv12_copy_frame_neon(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);

/****************************************************************************
*  Imports
*****************************************************************************/

/****************************************************************************
 *
 *  ROUTINE       : vp8_scale_machine_specific_config
 *
 *  INPUTS        : UINT32 Version : Codec version number.
 *
 *  OUTPUTS       : None.
 *
 *  RETURNS       : void
 *
 *  FUNCTION      : Checks for machine specifc features such as MMX support
 *                  sets appropriate flags and function pointers.
 *
 *  SPECIAL NOTES : None.
 *
 ****************************************************************************/
void vp8_scale_machine_specific_config()
{
#if CONFIG_SPATIAL_RESAMPLING
    vp8_horizontal_line_1_2_scale        = vp8cx_horizontal_line_1_2_scale_c;
    vp8_vertical_band_1_2_scale          = vp8cx_vertical_band_1_2_scale_c;
    vp8_last_vertical_band_1_2_scale      = vp8cx_last_vertical_band_1_2_scale_c;
    vp8_horizontal_line_3_5_scale        = vp8cx_horizontal_line_3_5_scale_c;
    vp8_vertical_band_3_5_scale          = vp8cx_vertical_band_3_5_scale_c;
    vp8_last_vertical_band_3_5_scale      = vp8cx_last_vertical_band_3_5_scale_c;
    vp8_horizontal_line_3_4_scale        = vp8cx_horizontal_line_3_4_scale_c;
    vp8_vertical_band_3_4_scale          = vp8cx_vertical_band_3_4_scale_c;
    vp8_last_vertical_band_3_4_scale      = vp8cx_last_vertical_band_3_4_scale_c;
    vp8_horizontal_line_2_3_scale        = vp8cx_horizontal_line_2_3_scale_c;
    vp8_vertical_band_2_3_scale          = vp8cx_vertical_band_2_3_scale_c;
    vp8_last_vertical_band_2_3_scale      = vp8cx_last_vertical_band_2_3_scale_c;
    vp8_horizontal_line_4_5_scale        = vp8cx_horizontal_line_4_5_scale_c;
    vp8_vertical_band_4_5_scale          = vp8cx_vertical_band_4_5_scale_c;
    vp8_last_vertical_band_4_5_scale      = vp8cx_last_vertical_band_4_5_scale_c;


    vp8_vertical_band_5_4_scale           = vp8cx_vertical_band_5_4_scale_c;
    vp8_vertical_band_5_3_scale           = vp8cx_vertical_band_5_3_scale_c;
    vp8_vertical_band_2_1_scale           = vp8cx_vertical_band_2_1_scale_c;
    vp8_vertical_band_2_1_scale_i         = vp8cx_vertical_band_2_1_scale_i_c;
    vp8_horizontal_line_2_1_scale         = vp8cx_horizontal_line_2_1_scale_c;
    vp8_horizontal_line_5_3_scale         = vp8cx_horizontal_line_5_3_scale_c;
    vp8_horizontal_line_5_4_scale         = vp8cx_horizontal_line_5_4_scale_c;
#endif

#if HAVE_ARMV7
    vp8_yv12_extend_frame_borders_ptr      = vp8_yv12_extend_frame_borders_neon;
    vp8_yv12_copy_frame_yonly_ptr          = vp8_yv12_copy_frame_yonly_neon;
    vp8_yv12_copy_frame_ptr               = vp8_yv12_copy_frame_neon;
#else
    vp8_yv12_extend_frame_borders_ptr      = vp8_yv12_extend_frame_borders;
    vp8_yv12_copy_frame_yonly_ptr          = vp8_yv12_copy_frame_yonly;
    vp8_yv12_copy_frame_ptr           = vp8_yv12_copy_frame;
#endif

}
