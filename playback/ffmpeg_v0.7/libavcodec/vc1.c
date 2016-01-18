/*
 * VC-1 and WMV3 decoder common code
 * Copyright (c) 2006-2007 Konstantin Shishkov
 * Partly based on vc9.c (c) 2005 Anonymous, Alex Beregszaszi, Michael Niedermayer
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
 * VC-1 and WMV3 decoder common code
 *
 */
#include "internal.h"
#include "dsputil.h"
#include "avcodec.h"
#include "mpegvideo.h"
#include "vc1.h"
#include "vc1data.h"
#include "msmpeg4data.h"
#include "unary.h"
#include "simple_idct.h"

#undef NDEBUG
#include <assert.h>

#include "log_dump.h"

#ifdef __log_special_case__
static int gvc1_max_width,gvc1_max_height;
static int gvc1_previous_width,gvc1_previous_height;
#endif


/***********************************************************************/
/**
 * @defgroup vc1bitplane VC-1 Bitplane decoding
 * @see 8.7, p56
 * @{
 */

/**
 * Imode types
 * @{
 */
enum Imode {
    IMODE_RAW,
    IMODE_NORM2,
    IMODE_DIFF2,
    IMODE_NORM6,
    IMODE_DIFF6,
    IMODE_ROWSKIP,
    IMODE_COLSKIP
};
/** @} */ //imode defines

/** Decode rows by checking if they are skipped
 * @param plane Buffer to store decoded bits
 * @param[in] width Width of this buffer
 * @param[in] height Height of this buffer
 * @param[in] stride of this buffer
 */
static void decode_rowskip(uint8_t* plane, int width, int height, int stride, GetBitContext *gb){
    int x, y;

    for (y=0; y<height; y++){
        if (!get_bits1(gb)) //rowskip
            memset(plane, 0, width);
        else
            for (x=0; x<width; x++)
                plane[x] = get_bits1(gb);
        plane += stride;
    }
}

/** Decode columns by checking if they are skipped
 * @param plane Buffer to store decoded bits
 * @param[in] width Width of this buffer
 * @param[in] height Height of this buffer
 * @param[in] stride of this buffer
 * @todo FIXME: Optimize
 */
static void decode_colskip(uint8_t* plane, int width, int height, int stride, GetBitContext *gb){
    int x, y;

    for (x=0; x<width; x++){
        if (!get_bits1(gb)) //colskip
            for (y=0; y<height; y++)
                plane[y*stride] = 0;
        else
            for (y=0; y<height; y++)
                plane[y*stride] = get_bits1(gb);
        plane ++;
    }
}

/** Decode a bitplane's bits
 * @param data bitplane where to store the decode bits
 * @param[out] raw_flag pointer to the flag indicating that this bitplane is not coded explicitly
 * @param v VC-1 context for bit reading and logging
 * @return Status
 * @todo FIXME: Optimize
 */
static int bitplane_decoding(uint8_t* data, int *raw_flag, VC1Context *v)
{
    GetBitContext *gb = &v->s.gb;

    int imode, x, y, code, offset;
    uint8_t invert, *planep = data;
    int width, height, stride;

    width = v->s.mb_width;
    height = v->s.mb_height;
    stride = v->s.mb_stride;
    invert = get_bits1(gb);
    imode = get_vlc2(gb, ff_vc1_imode_vlc.table, VC1_IMODE_VLC_BITS, 1);

    *raw_flag = 0;
    switch (imode)
    {
    case IMODE_RAW:
        //Data is actually read in the MB layer (same for all tests == "raw")
        *raw_flag = 1; //invert ignored
        return invert;
    case IMODE_DIFF2:
    case IMODE_NORM2:
        if ((height * width) & 1)
        {
            *planep++ = get_bits1(gb);
            offset = 1;
        }
        else offset = 0;
        // decode bitplane as one long line
        for (y = offset; y < height * width; y += 2) {
            code = get_vlc2(gb, ff_vc1_norm2_vlc.table, VC1_NORM2_VLC_BITS, 1);
            *planep++ = code & 1;
            offset++;
            if(offset == width) {
                offset = 0;
                planep += stride - width;
            }
            *planep++ = code >> 1;
            offset++;
            if(offset == width) {
                offset = 0;
                planep += stride - width;
            }
        }
        break;
    case IMODE_DIFF6:
    case IMODE_NORM6:
        if(!(height % 3) && (width % 3)) { // use 2x3 decoding
            for(y = 0; y < height; y+= 3) {
                for(x = width & 1; x < width; x += 2) {
                    code = get_vlc2(gb, ff_vc1_norm6_vlc.table, VC1_NORM6_VLC_BITS, 2);
                    if(code < 0){
                        av_log(v->s.avctx, AV_LOG_DEBUG, "invalid NORM-6 VLC\n");
                        return -1;
                    }
                    planep[x + 0] = (code >> 0) & 1;
                    planep[x + 1] = (code >> 1) & 1;
                    planep[x + 0 + stride] = (code >> 2) & 1;
                    planep[x + 1 + stride] = (code >> 3) & 1;
                    planep[x + 0 + stride * 2] = (code >> 4) & 1;
                    planep[x + 1 + stride * 2] = (code >> 5) & 1;
                }
                planep += stride * 3;
            }
            if(width & 1) decode_colskip(data, 1, height, stride, &v->s.gb);
        } else { // 3x2
            planep += (height & 1) * stride;
            for(y = height & 1; y < height; y += 2) {
                for(x = width % 3; x < width; x += 3) {
                    code = get_vlc2(gb, ff_vc1_norm6_vlc.table, VC1_NORM6_VLC_BITS, 2);
                    if(code < 0){
                        av_log(v->s.avctx, AV_LOG_DEBUG, "invalid NORM-6 VLC\n");
                        return -1;
                    }
                    planep[x + 0] = (code >> 0) & 1;
                    planep[x + 1] = (code >> 1) & 1;
                    planep[x + 2] = (code >> 2) & 1;
                    planep[x + 0 + stride] = (code >> 3) & 1;
                    planep[x + 1 + stride] = (code >> 4) & 1;
                    planep[x + 2 + stride] = (code >> 5) & 1;
                }
                planep += stride * 2;
            }
            x = width % 3;
            if(x) decode_colskip(data  ,             x, height    , stride, &v->s.gb);
            if(height & 1) decode_rowskip(data+x, width - x, 1, stride, &v->s.gb);
        }
        break;
    case IMODE_ROWSKIP:
        decode_rowskip(data, width, height, stride, &v->s.gb);
        break;
    case IMODE_COLSKIP:
        decode_colskip(data, width, height, stride, &v->s.gb);
        break;
    default: break;
    }

    /* Applying diff operator */
    if (imode == IMODE_DIFF2 || imode == IMODE_DIFF6)
    {
        planep = data;
        planep[0] ^= invert;
        for (x=1; x<width; x++)
            planep[x] ^= planep[x-1];
        for (y=1; y<height; y++)
        {
            planep += stride;
            planep[0] ^= planep[-stride];
            for (x=1; x<width; x++)
            {
                if (planep[x-1] != planep[x-stride]) planep[x] ^= invert;
                else                                 planep[x] ^= planep[x-1];
            }
        }
    }
    else if (invert)
    {
        planep = data;
        for (x=0; x<stride*height; x++) planep[x] = !planep[x]; //FIXME stride
    }
    return (imode<<1) + invert;
}

/** @} */ //Bitplane group

/***********************************************************************/
/** VOP Dquant decoding
 * @param v VC-1 Context
 */
static int vop_dquant_decoding(VC1Context *v)
{
    GetBitContext *gb = &v->s.gb;
    int pqdiff;

    //variable size
    if (v->dquant == 2)
    {
        pqdiff = get_bits(gb, 3);
        if (pqdiff == 7) v->altpq = get_bits(gb, 5);
        else v->altpq = v->pq + pqdiff + 1;
    }
    else
    {
        v->dquantfrm = get_bits1(gb);
        if ( v->dquantfrm )
        {
            v->dqprofile = get_bits(gb, 2);
            switch (v->dqprofile)
            {
            case DQPROFILE_SINGLE_EDGE:
            case DQPROFILE_DOUBLE_EDGES:
                v->dqsbedge = get_bits(gb, 2);
                break;
            case DQPROFILE_ALL_MBS:
                v->dqbilevel = get_bits1(gb);
                if(!v->dqbilevel)
                    v->halfpq = 0;
            default: break; //Forbidden ?
            }
            if (v->dqbilevel || v->dqprofile != DQPROFILE_ALL_MBS)
            {
                pqdiff = get_bits(gb, 3);
                if (pqdiff == 7) v->altpq = get_bits(gb, 5);
                else v->altpq = v->pq + pqdiff + 1;
            }
        }
    }
    return 0;
}

static int decode_sequence_header_adv(VC1Context *v, GetBitContext *gb);

static void simple_idct_put_rangered(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;
    ff_simple_idct(block);
    for (i = 0; i < 64; i++) block[i] = (block[i] - 64) << 1;
    ff_put_pixels_clamped_c(block, dest, line_size);
}

static void simple_idct_put_signed(uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_simple_idct(block);
    ff_put_signed_pixels_clamped_c(block, dest, line_size);
}

static void simple_idct_put_signed_rangered(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;
    ff_simple_idct(block);
    for (i = 0; i < 64; i++) block[i] <<= 1;
    ff_put_signed_pixels_clamped_c(block, dest, line_size);
}

/**
 * Decode Simple/Main Profiles sequence header
 * @see Figure 7-8, p16-17
 * @param avctx Codec context
 * @param gb GetBit context initialized from Codec context extra_data
 * @return Status
 */
int vc1_decode_sequence_header(AVCodecContext *avctx, VC1Context *v, GetBitContext *gb)
{
    av_log(avctx, AV_LOG_DEBUG, "Header: %0X\n", show_bits(gb, 32));
    v->profile = get_bits(gb, 2);
    #ifdef __log_special_case__
    av_log(NULL,AV_LOG_ERROR,"get vc-1 profile %d.\n", v->profile);
    switch (v->profile) {
        case PROFILE_SIMPLE:
            av_log(NULL,AV_LOG_ERROR," simple profile.\n");
            break;
        case PROFILE_MAIN:
            av_log(NULL,AV_LOG_ERROR," main profile.\n");
            break;
        case PROFILE_COMPLEX:
            av_log(NULL,AV_LOG_ERROR," !!!complex profile, not support.\n");
            break;
        case PROFILE_ADVANCED:
            av_log(NULL,AV_LOG_ERROR," advanced profile.\n");
            break;
        default:
            av_log(NULL,AV_LOG_ERROR," !!!! error, here, wrong profile in vc1_decode_sequence_header.\n");
            break;
    }
    #endif
    if (v->profile == PROFILE_COMPLEX)
    {
        #ifdef __log_special_case__
        av_log(NULL,AV_LOG_ERROR,"!!!!!attention: get WMV9 [Complex Profile], not supported.\n");
        #endif
        av_log(avctx, AV_LOG_ERROR, "WMV3 Complex Profile is not fully supported\n");
    }

    if (v->profile == PROFILE_ADVANCED)
    {
        v->zz_8x4 = ff_vc1_adv_progressive_8x4_zz;
        v->zz_4x8 = ff_vc1_adv_progressive_4x8_zz;
        return decode_sequence_header_adv(v, gb);
    }
    else
    {
        v->zz_8x4 = wmv2_scantableA;
        v->zz_4x8 = wmv2_scantableB;
        v->res_y411   = get_bits1(gb);
        v->res_sprite = get_bits1(gb);
        if (v->res_y411)
        {
            av_log(avctx, AV_LOG_ERROR,
                   "Old interlaced mode is not supported\n");
            return -1;
        }
        if (v->res_sprite) {
            av_log(avctx, AV_LOG_ERROR, "WMVP is not fully supported\n");
        }
    }

    // (fps-2)/4 (->30)
    v->frmrtq_postproc = get_bits(gb, 3); //common
    // (bitrate-32kbps)/64kbps
    v->bitrtq_postproc = get_bits(gb, 5); //common
    v->s.loop_filter = get_bits1(gb); //common
    if(v->s.loop_filter == 1 && v->profile == PROFILE_SIMPLE)
    {
        av_log(avctx, AV_LOG_ERROR,
               "LOOPFILTER shall not be enabled in Simple Profile\n");
    }
    if(v->s.avctx->skip_loop_filter >= AVDISCARD_ALL)
        v->s.loop_filter = 0;

    v->res_x8 = get_bits1(gb); //reserved
    v->multires = get_bits1(gb);
    #ifdef __log_special_case__
    if(v->multires)
        av_log(NULL,AV_LOG_ERROR,"!!!!!attention: detected [Multi-Resolution] flag WMV9(vc-1 simple&main profile) in seq header.\n");
    #endif
    v->res_fasttx = get_bits1(gb);
    if (!v->res_fasttx)
    {
        v->vc1dsp.vc1_inv_trans_8x8_add = ff_simple_idct_add;
        v->vc1dsp.vc1_inv_trans_8x8_put[0] = ff_simple_idct_put;
        v->vc1dsp.vc1_inv_trans_8x8_put[1] = simple_idct_put_rangered;
        v->vc1dsp.vc1_inv_trans_8x8_put_signed[0] = simple_idct_put_signed;
        v->vc1dsp.vc1_inv_trans_8x8_put_signed[1] = simple_idct_put_signed_rangered;
        v->vc1dsp.vc1_inv_trans_8x4 = ff_simple_idct84_add;
        v->vc1dsp.vc1_inv_trans_4x8 = ff_simple_idct48_add;
        v->vc1dsp.vc1_inv_trans_4x4 = ff_simple_idct44_add;
        v->vc1dsp.vc1_inv_trans_8x8_dc = ff_simple_idct_add;
        v->vc1dsp.vc1_inv_trans_8x4_dc = ff_simple_idct84_add;
        v->vc1dsp.vc1_inv_trans_4x8_dc = ff_simple_idct48_add;
        v->vc1dsp.vc1_inv_trans_4x4_dc = ff_simple_idct44_add;
    }

    v->fastuvmc =  get_bits1(gb); //common
    if (!v->profile && !v->fastuvmc)
    {
        av_log(avctx, AV_LOG_ERROR,
               "FASTUVMC unavailable in Simple Profile\n");
        return -1;
    }
    v->extended_mv =  get_bits1(gb); //common
    if (!v->profile && v->extended_mv)
    {
        av_log(avctx, AV_LOG_ERROR,
               "Extended MVs unavailable in Simple Profile\n");
        return -1;
    }
    v->dquant =  get_bits(gb, 2); //common
    v->vstransform =  get_bits1(gb); //common

    v->res_transtab = get_bits1(gb);
    if (v->res_transtab)
    {
        av_log(avctx, AV_LOG_ERROR,
               "1 for reserved RES_TRANSTAB is forbidden\n");
        return -1;
    }

    v->overlap = get_bits1(gb); //common

    v->s.resync_marker = get_bits1(gb);
    v->rangered = get_bits1(gb);
    #ifdef __log_special_case__
    if(v->rangered)
        av_log(NULL,AV_LOG_ERROR,"!!!!!attention: [Range Reduction] detected WMV9(vc-1 simple&main profile) range reduction flag in seq header.\n");
    #endif
    if (v->rangered && v->profile == PROFILE_SIMPLE)
    {
        av_log(avctx, AV_LOG_INFO,
               "RANGERED should be set to 0 in Simple Profile\n");
    }

    v->s.max_b_frames = avctx->max_b_frames = get_bits(gb, 3); //common
    v->quantizer_mode = get_bits(gb, 2); //common

    v->finterpflag = get_bits1(gb); //common

    if (v->res_sprite) {
        v->s.avctx->width  = v->s.avctx->coded_width  = get_bits(gb, 11);
        v->s.avctx->height = v->s.avctx->coded_height = get_bits(gb, 11);
        skip_bits(gb, 5); //frame rate
        v->res_x8 = get_bits1(gb);
        if (get_bits1(gb)) { // something to do with DC VLC selection
            av_log(avctx, AV_LOG_ERROR, "Unsupported sprite feature\n");
            return -1;
        }
        skip_bits(gb, 3); //slice code
        v->res_rtm_flag = 0;
    } else {
        v->res_rtm_flag = get_bits1(gb); //reserved
    }
    if (!v->res_rtm_flag)
    {
//            av_log(avctx, AV_LOG_ERROR,
//                   "0 for reserved RES_RTM_FLAG is forbidden\n");
        av_log(avctx, AV_LOG_ERROR,
               "Old WMV3 version detected, some frames may be decoded incorrectly\n");
        //return -1;
    }
    //TODO: figure out what they mean (always 0x402F)
    if(!v->res_fasttx) skip_bits(gb, 16);
    av_log(avctx, AV_LOG_DEBUG,
               "Profile %i:\nfrmrtq_postproc=%i, bitrtq_postproc=%i\n"
               "LoopFilter=%i, MultiRes=%i, FastUVMC=%i, Extended MV=%i\n"
               "Rangered=%i, VSTransform=%i, Overlap=%i, SyncMarker=%i\n"
               "DQuant=%i, Quantizer mode=%i, Max B frames=%i\n",
               v->profile, v->frmrtq_postproc, v->bitrtq_postproc,
               v->s.loop_filter, v->multires, v->fastuvmc, v->extended_mv,
               v->rangered, v->vstransform, v->overlap, v->s.resync_marker,
               v->dquant, v->quantizer_mode, avctx->max_b_frames
               );
    return 0;
}

static int decode_sequence_header_adv(VC1Context *v, GetBitContext *gb)
{
    v->res_rtm_flag = 1;
    v->level = get_bits(gb, 3);
    if(v->level >= 5)
    {
        av_log(v->s.avctx, AV_LOG_ERROR, "Reserved LEVEL %i\n",v->level);
    }
    v->chromaformat = get_bits(gb, 2);
    if (v->chromaformat != 1)
    {
        av_log(v->s.avctx, AV_LOG_ERROR,
               "Only 4:2:0 chroma format supported\n");
        return -1;
    }

    // (fps-2)/4 (->30)
    v->frmrtq_postproc = get_bits(gb, 3); //common
    // (bitrate-32kbps)/64kbps
    v->bitrtq_postproc = get_bits(gb, 5); //common
    v->postprocflag = get_bits1(gb); //common

    v->s.avctx->coded_width = (get_bits(gb, 12) + 1) << 1;
    v->s.avctx->coded_height = (get_bits(gb, 12) + 1) << 1;
    #ifdef __log_special_case__
    gvc1_previous_width=gvc1_max_width=v->s.avctx->coded_width;
    gvc1_previous_height=gvc1_max_height=v->s.avctx->coded_height;
    av_log(NULL, AV_LOG_ERROR,"vc-1 (advanced profile) get max coded_width=%d,coded_height=%d.\n",gvc1_max_width,gvc1_max_height);
    #endif
    v->s.avctx->width = v->s.avctx->coded_width;
    v->s.avctx->height = v->s.avctx->coded_height;
    v->broadcast = get_bits1(gb);
    v->interlace = get_bits1(gb);
    v->tfcntrflag = get_bits1(gb);
    v->finterpflag = get_bits1(gb);
    skip_bits1(gb); // reserved

    v->s.h_edge_pos = v->s.avctx->coded_width;
    v->s.v_edge_pos = v->s.avctx->coded_height;

    av_log(v->s.avctx, AV_LOG_DEBUG,
               "Advanced Profile level %i:\nfrmrtq_postproc=%i, bitrtq_postproc=%i\n"
               "LoopFilter=%i, ChromaFormat=%i, Pulldown=%i, Interlace: %i\n"
               "TFCTRflag=%i, FINTERPflag=%i\n",
               v->level, v->frmrtq_postproc, v->bitrtq_postproc,
               v->s.loop_filter, v->chromaformat, v->broadcast, v->interlace,
               v->tfcntrflag, v->finterpflag
               );

    v->psf = get_bits1(gb);
    if(v->psf) { //PsF, 6.1.13
        av_log(v->s.avctx, AV_LOG_ERROR, "Progressive Segmented Frame mode: not supported (yet)\n");
        return -1;
    }
    v->s.max_b_frames = v->s.avctx->max_b_frames = 7;
    if(get_bits1(gb)) { //Display Info - decoding is not affected by it
        int w, h, ar = 0;
        av_log(v->s.avctx, AV_LOG_DEBUG, "Display extended info:\n");
        v->s.avctx->width  = w = get_bits(gb, 14) + 1;
        v->s.avctx->height = h = get_bits(gb, 14) + 1;
        av_log(v->s.avctx, AV_LOG_DEBUG, "Display dimensions: %ix%i\n", w, h);
        if(get_bits1(gb))
            ar = get_bits(gb, 4);
        if(ar && ar < 14){
            v->s.avctx->sample_aspect_ratio = ff_vc1_pixel_aspect[ar];
        }else if(ar == 15){
            w = get_bits(gb, 8);
            h = get_bits(gb, 8);
            v->s.avctx->sample_aspect_ratio = (AVRational){w, h};
        }
        av_log(v->s.avctx, AV_LOG_DEBUG, "Aspect: %i:%i\n", v->s.avctx->sample_aspect_ratio.num, v->s.avctx->sample_aspect_ratio.den);

        if(get_bits1(gb)){ //framerate stuff
            if(get_bits1(gb)) {
                v->s.avctx->time_base.num = 32;
                v->s.avctx->time_base.den = get_bits(gb, 16) + 1;
            } else {
                int nr, dr;
                nr = get_bits(gb, 8);
                dr = get_bits(gb, 4);
                if(nr && nr < 8 && dr && dr < 3){
                    v->s.avctx->time_base.num = ff_vc1_fps_dr[dr - 1];
                    v->s.avctx->time_base.den = ff_vc1_fps_nr[nr - 1] * 1000;
                }
            }
        }

        if(get_bits1(gb)){
            v->color_prim = get_bits(gb, 8);
            v->transfer_char = get_bits(gb, 8);
            v->matrix_coef = get_bits(gb, 8);
        }
    }

    v->hrd_param_flag = get_bits1(gb);
    if(v->hrd_param_flag) {
        int i;
        v->hrd_num_leaky_buckets = get_bits(gb, 5);
        skip_bits(gb, 4); //bitrate exponent
        skip_bits(gb, 4); //buffer size exponent
        for(i = 0; i < v->hrd_num_leaky_buckets; i++) {
            skip_bits(gb, 16); //hrd_rate[n]
            skip_bits(gb, 16); //hrd_buffer[n]
        }
    }
    return 0;
}

int vc1_decode_entry_point(AVCodecContext *avctx, VC1Context *v, GetBitContext *gb)
{
    int i;

    av_log(avctx, AV_LOG_DEBUG, "Entry point: %08X\n", show_bits_long(gb, 32));
    v->broken_link = get_bits1(gb);
    v->closed_entry = get_bits1(gb);
    v->panscanflag = get_bits1(gb);
    v->refdist_flag = get_bits1(gb);
    v->s.loop_filter = get_bits1(gb);
    v->fastuvmc = get_bits1(gb);
    v->extended_mv = get_bits1(gb);
    v->dquant = get_bits(gb, 2);
    v->vstransform = get_bits1(gb);
    v->overlap = get_bits1(gb);
    v->quantizer_mode = get_bits(gb, 2);

    if(v->hrd_param_flag){
        for(i = 0; i < v->hrd_num_leaky_buckets; i++) {
            skip_bits(gb, 8); //hrd_full[n]
        }
    }

    if(get_bits1(gb)){
        avctx->coded_width = (get_bits(gb, 12)+1)<<1;
        avctx->coded_height = (get_bits(gb, 12)+1)<<1;
        #ifdef __log_special_case__
        if(avctx->coded_width!=gvc1_previous_width ||avctx->coded_height!= gvc1_previous_height)
        {
            av_log(NULL,AV_LOG_ERROR,"!!!!!attention: [Multi-Resolution] detected in vc-1(advanced profie) entry point,current width=%d,height=%d.\n",avctx->coded_width,avctx->coded_height);
            av_log(NULL,AV_LOG_ERROR,"previous width=%d, height=%d, max width=%d,max height=%d.\n",gvc1_previous_width,gvc1_previous_height,gvc1_max_width,gvc1_max_height);
            gvc1_previous_width=avctx->coded_width;
            gvc1_previous_height=avctx->coded_height;
        }
        #endif
    }
    if(v->extended_mv)
        v->extended_dmv = get_bits1(gb);
    if((v->range_mapy_flag = get_bits1(gb))) {
        #ifdef __log_special_case__
        av_log(NULL,AV_LOG_ERROR,"!!!!!attention: detected [Range Map Y], range_mapy_flag in entry point, not support in ffmpeg now.\n");
        #endif

        av_log(avctx, AV_LOG_ERROR, "Luma scaling is not supported, expect wrong picture\n");
        v->range_mapy = get_bits(gb, 3);
    }
    if((v->range_mapuv_flag = get_bits1(gb))) {
        #ifdef __log_special_case__
        av_log(NULL,AV_LOG_ERROR,"!!!!!attention: detected [Range Map UV], range_mapuv_flag in entry point, not support in ffmpeg now.\n");
        #endif
        av_log(avctx, AV_LOG_ERROR, "Chroma scaling is not supported, expect wrong picture\n");
        v->range_mapuv = get_bits(gb, 3);
    }

    av_log(avctx, AV_LOG_DEBUG, "Entry point info:\n"
        "BrokenLink=%i, ClosedEntry=%i, PanscanFlag=%i\n"
        "RefDist=%i, Postproc=%i, FastUVMC=%i, ExtMV=%i\n"
        "DQuant=%i, VSTransform=%i, Overlap=%i, Qmode=%i\n",
        v->broken_link, v->closed_entry, v->panscanflag, v->refdist_flag, v->s.loop_filter,
        v->fastuvmc, v->extended_mv, v->dquant, v->vstransform, v->overlap, v->quantizer_mode);

    return 0;
}

int vc1_parse_frame_header(VC1Context *v, GetBitContext* gb)
{
    int pqindex, lowquant, status;

    if(v->finterpflag) v->interpfrm = get_bits1(gb);
    skip_bits(gb, 2); //framecnt unused
    v->rangeredfrm = 0;
    if (v->rangered) v->rangeredfrm = get_bits1(gb);
    v->s.pict_type = get_bits1(gb);
    if (v->s.avctx->max_b_frames) {
        if (!v->s.pict_type) {
            if (get_bits1(gb)) v->s.pict_type = FF_I_TYPE;
            else v->s.pict_type = FF_B_TYPE;
        } else v->s.pict_type = FF_P_TYPE;
    } else v->s.pict_type = v->s.pict_type ? FF_P_TYPE : FF_I_TYPE;

    v->bi_type = 0;
    if(v->s.pict_type == FF_B_TYPE) {
        v->bfraction_lut_index = get_vlc2(gb, ff_vc1_bfraction_vlc.table, VC1_BFRACTION_VLC_BITS, 1);
        v->bfraction = ff_vc1_bfraction_lut[v->bfraction_lut_index];
        if(v->bfraction == 0) {
            v->s.pict_type = FF_BI_TYPE;
        }
    }
    if(v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE)
        skip_bits(gb, 7); // skip buffer fullness

    if(v->parse_only)
        return 0;

    /* calculate RND */
    if(v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE)
        v->rnd = 1;
    if(v->s.pict_type == FF_P_TYPE)
        v->rnd ^= 1;

    /* Quantizer stuff */
    pqindex = get_bits(gb, 5);
    if(!pqindex) return -1;
    if (v->quantizer_mode == QUANT_FRAME_IMPLICIT)
        v->pq = ff_vc1_pquant_table[0][pqindex];
    else
        v->pq = ff_vc1_pquant_table[1][pqindex];

    v->pquantizer = 1;
    if (v->quantizer_mode == QUANT_FRAME_IMPLICIT)
        v->pquantizer = pqindex < 9;
    if (v->quantizer_mode == QUANT_NON_UNIFORM)
        v->pquantizer = 0;
    v->pqindex = pqindex;
    if (pqindex < 9) v->halfpq = get_bits1(gb);
    else v->halfpq = 0;
    if (v->quantizer_mode == QUANT_FRAME_EXPLICIT)
        v->pquantizer = get_bits1(gb);
    v->dquantfrm = 0;
    if (v->extended_mv == 1) v->mvrange = get_unary(gb, 0, 3);
    v->k_x = v->mvrange + 9 + (v->mvrange >> 1); //k_x can be 9 10 12 13
    v->k_y = v->mvrange + 8; //k_y can be 8 9 10 11
    v->range_x = 1 << (v->k_x - 1);
    v->range_y = 1 << (v->k_y - 1);
    if (v->multires && v->s.pict_type != FF_B_TYPE) v->respic = get_bits(gb, 2);

    if(v->res_x8 && (v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE)){
        v->x8_type = get_bits1(gb);
    }else v->x8_type = 0;
//av_log(v->s.avctx, AV_LOG_INFO, "%c Frame: QP=[%i]%i (+%i/2) %i\n",
//        (v->s.pict_type == FF_P_TYPE) ? 'P' : ((v->s.pict_type == FF_I_TYPE) ? 'I' : 'B'), pqindex, v->pq, v->halfpq, v->rangeredfrm);

    if(v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_P_TYPE) v->use_ic = 0;

    switch(v->s.pict_type) {
    case FF_P_TYPE:
        if (v->pq < 5) v->tt_index = 0;
        else if(v->pq < 13) v->tt_index = 1;
        else v->tt_index = 2;

        lowquant = (v->pq > 12) ? 0 : 1;
        v->mv_mode = ff_vc1_mv_pmode_table[lowquant][get_unary(gb, 1, 4)];
        if (v->mv_mode == MV_PMODE_INTENSITY_COMP)
        {
            int scale, shift, i;
            v->mv_mode2 = ff_vc1_mv_pmode_table2[lowquant][get_unary(gb, 1, 3)];
            v->lumscale = get_bits(gb, 6);
            v->lumshift = get_bits(gb, 6);

            #ifdef __log_special_case__
            av_log(NULL,AV_LOG_ERROR,"!!!!!attention: [Intensity Compensation] flag detected(wmv3) in frame header lumascale=%d, lumashift=%d.\n",v->lumscale,v->lumshift);
            #endif

            v->use_ic = 1;
            /* fill lookup tables for intensity compensation */
            if(!v->lumscale) {
                scale = -64;
                shift = (255 - v->lumshift * 2) << 6;
                if(v->lumshift > 31)
                    shift += 128 << 6;
            } else {
                scale = v->lumscale + 32;
                if(v->lumshift > 31)
                    shift = (v->lumshift - 64) << 6;
                else
                    shift = v->lumshift << 6;
            }
            for(i = 0; i < 256; i++) {
                v->luty[i] = av_clip_uint8((scale * i + shift + 32) >> 6);
                v->lutuv[i] = av_clip_uint8((scale * (i - 128) + 128*64 + 32) >> 6);
            }
        }
        if(v->mv_mode == MV_PMODE_1MV_HPEL || v->mv_mode == MV_PMODE_1MV_HPEL_BILIN)
            v->s.quarter_sample = 0;
        else if(v->mv_mode == MV_PMODE_INTENSITY_COMP) {
            if(v->mv_mode2 == MV_PMODE_1MV_HPEL || v->mv_mode2 == MV_PMODE_1MV_HPEL_BILIN)
                v->s.quarter_sample = 0;
            else
                v->s.quarter_sample = 1;
        } else
            v->s.quarter_sample = 1;
        v->s.mspel = !(v->mv_mode == MV_PMODE_1MV_HPEL_BILIN || (v->mv_mode == MV_PMODE_INTENSITY_COMP && v->mv_mode2 == MV_PMODE_1MV_HPEL_BILIN));

        if ((v->mv_mode == MV_PMODE_INTENSITY_COMP &&
                 v->mv_mode2 == MV_PMODE_MIXED_MV)
                || v->mv_mode == MV_PMODE_MIXED_MV)
        {
            status = bitplane_decoding(v->mv_type_mb_plane, &v->mv_type_is_raw, v);
            if (status < 0) return -1;
            av_log(v->s.avctx, AV_LOG_DEBUG, "MB MV Type plane encoding: "
                   "Imode: %i, Invert: %i\n", status>>1, status&1);
        } else {
            v->mv_type_is_raw = 0;
            memset(v->mv_type_mb_plane, 0, v->s.mb_stride * v->s.mb_height);
        }
        status = bitplane_decoding(v->s.mbskip_table, &v->skip_is_raw, v);
        if (status < 0) return -1;
        av_log(v->s.avctx, AV_LOG_DEBUG, "MB Skip plane encoding: "
               "Imode: %i, Invert: %i\n", status>>1, status&1);

        /* Hopefully this is correct for P frames */
        v->s.mv_table_index = get_bits(gb, 2); //but using ff_vc1_ tables
        v->cbpcy_vlc = &ff_vc1_cbpcy_p_vlc[get_bits(gb, 2)];

        if (v->dquant)
        {
            av_log(v->s.avctx, AV_LOG_DEBUG, "VOP DQuant info\n");
            vop_dquant_decoding(v);
        }

        v->ttfrm = 0; //FIXME Is that so ?
        if (v->vstransform)
        {
            v->ttmbf = get_bits1(gb);
            if (v->ttmbf)
            {
                v->ttfrm = ff_vc1_ttfrm_to_tt[get_bits(gb, 2)];
            }
        } else {
            v->ttmbf = 1;
            v->ttfrm = TT_8X8;
        }
        break;
    case FF_B_TYPE:
        if (v->pq < 5) v->tt_index = 0;
        else if(v->pq < 13) v->tt_index = 1;
        else v->tt_index = 2;

        v->mv_mode = get_bits1(gb) ? MV_PMODE_1MV : MV_PMODE_1MV_HPEL_BILIN;
        v->s.quarter_sample = (v->mv_mode == MV_PMODE_1MV);
        v->s.mspel = v->s.quarter_sample;

        status = bitplane_decoding(v->direct_mb_plane, &v->dmb_is_raw, v);
        if (status < 0) return -1;
        av_log(v->s.avctx, AV_LOG_DEBUG, "MB Direct Type plane encoding: "
               "Imode: %i, Invert: %i\n", status>>1, status&1);
        status = bitplane_decoding(v->s.mbskip_table, &v->skip_is_raw, v);
        if (status < 0) return -1;
        av_log(v->s.avctx, AV_LOG_DEBUG, "MB Skip plane encoding: "
               "Imode: %i, Invert: %i\n", status>>1, status&1);

        v->s.mv_table_index = get_bits(gb, 2);
        v->cbpcy_vlc = &ff_vc1_cbpcy_p_vlc[get_bits(gb, 2)];

        if (v->dquant)
        {
            av_log(v->s.avctx, AV_LOG_DEBUG, "VOP DQuant info\n");
            vop_dquant_decoding(v);
        }

        v->ttfrm = 0;
        if (v->vstransform)
        {
            v->ttmbf = get_bits1(gb);
            if (v->ttmbf)
            {
                v->ttfrm = ff_vc1_ttfrm_to_tt[get_bits(gb, 2)];
            }
        } else {
            v->ttmbf = 1;
            v->ttfrm = TT_8X8;
        }
        break;
    }

    if(!v->x8_type)
    {
        /* AC Syntax */
        v->c_ac_table_index = decode012(gb);
        if (v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE)
        {
            v->y_ac_table_index = decode012(gb);
        }
        /* DC Syntax */
        v->s.dc_table_index = get_bits1(gb);
    }

    if(v->s.pict_type == FF_BI_TYPE) {
        v->s.pict_type = FF_B_TYPE;
        v->bi_type = 1;
    }
    return 0;
}

//
//added by liujian, 11-25-2011
//    for interlaced case, we also need to know picture type.
//
static  int  get_adv_interlaced_pict_type(VC1Context *v, GetBitContext* gb)
{
    if(v->fcm == 1){
        //frame interlaced
        /*
        Table 35: Advanced Profile Picture Type VLC
        [PTYPE VLC] [Picture Type]
        110               I
        0                  P
        10                 B
        1110             BI
        1111             Skipped
        */
        switch(get_unary(gb, 0, 4)) {
        case 0:
            v->s.pict_type = FF_P_TYPE;
            break;
        case 1:
            v->s.pict_type = FF_B_TYPE;
            break;
        case 2:
            v->s.pict_type = FF_I_TYPE;
            break;
        case 3:
            v->s.pict_type = FF_BI_TYPE;
            break;
        case 4:
            v->s.pict_type = FF_P_TYPE; // skipped pic
            v->p_frame_skipped = 1;
            break;
        }
        return 0;
    } else if (v->fcm == 2){
        //field interlaced
        /*
        Table 105: Field Picture Type FLC
        [FPTYPE]  [First Field Picture Type]  [Second Field Picture Type]
        000          I                                   I
        001          I                                   P
        010          P                                  I
        011          P                                  P
        100          B                                  B
        101          B                                  BI
        110          BI                                 B
        111          BI                                 BI
        */
        unsigned int fptype = get_bits(gb, 3);
        switch(fptype){
        case 0:
        case 1:
        case 2: v->s.pict_type = FF_I_TYPE; break;
        case 3: v->s.pict_type = FF_P_TYPE; break;
        case 4: v->s.pict_type = FF_B_TYPE; break;
        case 5:
        case 6:
        case 7:
        default: v->s.pict_type = FF_BI_TYPE; break;
        }
        return 0;
    }
    else{
        av_log(v->s.avctx, AV_LOG_ERROR, "Interlaced frames/fields support is not implemented, error fcm\n");
        return -1;
    }
}
int vc1_parse_frame_header_adv(VC1Context *v, GetBitContext* gb)
{
    int pqindex, lowquant;
    int status;

    v->p_frame_skipped = 0;

    if(v->interlace){
        v->fcm = decode012(gb);
        if (!v->s.avctx->vc1_interlaced) {
            switch (v->fcm) {
            case 0:
                break;
            case 1:
            case 2:
                v->s.avctx->vc1_interlaced = 1;
                break;
            default:
                break;
            }
        }
        if(v->fcm){
            if(!v->warn_interlaced++)
                av_log(v->s.avctx, AV_LOG_ERROR, "Interlaced frames/fields support is not implemented\n");
            get_adv_interlaced_pict_type(v,gb);
            return -1;
        }
    }
    switch(get_unary(gb, 0, 4)) {
    case 0:
        v->s.pict_type = FF_P_TYPE;
        break;
    case 1:
        v->s.pict_type = FF_B_TYPE;
        break;
    case 2:
        v->s.pict_type = FF_I_TYPE;
        break;
    case 3:
        v->s.pict_type = FF_BI_TYPE;
        break;
    case 4:
        v->s.pict_type = FF_P_TYPE; // skipped pic
        v->p_frame_skipped = 1;
        return 0;
    }
    if(v->tfcntrflag)
        skip_bits(gb, 8);
    if(v->broadcast) {
        if(!v->interlace || v->psf) {
            v->rptfrm = get_bits(gb, 2);
        } else {
            v->tff = get_bits1(gb);
            v->rff = get_bits1(gb);
        }
    }
    if(v->panscanflag) {
        av_log_missing_feature(v->s.avctx, "Pan-scan", 0);
        //...
    }
    v->rnd = get_bits1(gb);
    if(v->interlace)
        v->uvsamp = get_bits1(gb);
    if(v->finterpflag) v->interpfrm = get_bits1(gb);
    if(v->s.pict_type == FF_B_TYPE) {
        v->bfraction_lut_index = get_vlc2(gb, ff_vc1_bfraction_vlc.table, VC1_BFRACTION_VLC_BITS, 1);
        v->bfraction = ff_vc1_bfraction_lut[v->bfraction_lut_index];
        if(v->bfraction == 0) {
            v->s.pict_type = FF_BI_TYPE; /* XXX: should not happen here */
        }
    }
    pqindex = get_bits(gb, 5);
    if(!pqindex) return -1;
    v->pqindex = pqindex;
    if (v->quantizer_mode == QUANT_FRAME_IMPLICIT)
        v->pq = ff_vc1_pquant_table[0][pqindex];
    else
        v->pq = ff_vc1_pquant_table[1][pqindex];

    v->pquantizer = 1;
    if (v->quantizer_mode == QUANT_FRAME_IMPLICIT)
        v->pquantizer = pqindex < 9;
    if (v->quantizer_mode == QUANT_NON_UNIFORM)
        v->pquantizer = 0;
    v->pqindex = pqindex;
    if (pqindex < 9) v->halfpq = get_bits1(gb);
    else v->halfpq = 0;
    if (v->quantizer_mode == QUANT_FRAME_EXPLICIT)
        v->pquantizer = get_bits1(gb);
    if(v->postprocflag)
        v->postproc = get_bits(gb, 2);

    if(v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_P_TYPE) v->use_ic = 0;

    if(v->parse_only)
        return 0;

    switch(v->s.pict_type) {
    case FF_I_TYPE:
    case FF_BI_TYPE:
        status = bitplane_decoding(v->acpred_plane, &v->acpred_is_raw, v);
        if (status < 0) return -1;
        av_log(v->s.avctx, AV_LOG_DEBUG, "ACPRED plane encoding: "
                "Imode: %i, Invert: %i\n", status>>1, status&1);
        v->condover = CONDOVER_NONE;
        if(v->overlap && v->pq <= 8) {
            v->condover = decode012(gb);
            if(v->condover == CONDOVER_SELECT) {
                status = bitplane_decoding(v->over_flags_plane, &v->overflg_is_raw, v);
                if (status < 0) return -1;
                av_log(v->s.avctx, AV_LOG_DEBUG, "CONDOVER plane encoding: "
                        "Imode: %i, Invert: %i\n", status>>1, status&1);
            }
        }
        break;
    case FF_P_TYPE:
        if (v->extended_mv) v->mvrange = get_unary(gb, 0, 3);
        else v->mvrange = 0;
        v->k_x = v->mvrange + 9 + (v->mvrange >> 1); //k_x can be 9 10 12 13
        v->k_y = v->mvrange + 8; //k_y can be 8 9 10 11
        v->range_x = 1 << (v->k_x - 1);
        v->range_y = 1 << (v->k_y - 1);

        if (v->pq < 5) v->tt_index = 0;
        else if(v->pq < 13) v->tt_index = 1;
        else v->tt_index = 2;

        lowquant = (v->pq > 12) ? 0 : 1;
        v->mv_mode = ff_vc1_mv_pmode_table[lowquant][get_unary(gb, 1, 4)];
        if (v->mv_mode == MV_PMODE_INTENSITY_COMP)
        {
            int scale, shift, i;
            v->mv_mode2 = ff_vc1_mv_pmode_table2[lowquant][get_unary(gb, 1, 3)];
            v->lumscale = get_bits(gb, 6);
            v->lumshift = get_bits(gb, 6);
            #ifdef __log_special_case__
            av_log(NULL,AV_LOG_ERROR,"!!!!!attention: [Intensity Compensation] flag detected(vc-1 adv) in frame header lumascale=%d, lumashift=%d.\n",v->lumscale,v->lumshift);
            #endif
            /* fill lookup tables for intensity compensation */
            if(!v->lumscale) {
                scale = -64;
                shift = (255 - v->lumshift * 2) << 6;
                if(v->lumshift > 31)
                    shift += 128 << 6;
            } else {
                scale = v->lumscale + 32;
                if(v->lumshift > 31)
                    shift = (v->lumshift - 64) << 6;
                else
                    shift = v->lumshift << 6;
            }
            for(i = 0; i < 256; i++) {
                v->luty[i] = av_clip_uint8((scale * i + shift + 32) >> 6);
                v->lutuv[i] = av_clip_uint8((scale * (i - 128) + 128*64 + 32) >> 6);
            }
            v->use_ic = 1;
        }
        if(v->mv_mode == MV_PMODE_1MV_HPEL || v->mv_mode == MV_PMODE_1MV_HPEL_BILIN)
            v->s.quarter_sample = 0;
        else if(v->mv_mode == MV_PMODE_INTENSITY_COMP) {
            if(v->mv_mode2 == MV_PMODE_1MV_HPEL || v->mv_mode2 == MV_PMODE_1MV_HPEL_BILIN)
                v->s.quarter_sample = 0;
            else
                v->s.quarter_sample = 1;
        } else
            v->s.quarter_sample = 1;
        v->s.mspel = !(v->mv_mode == MV_PMODE_1MV_HPEL_BILIN || (v->mv_mode == MV_PMODE_INTENSITY_COMP && v->mv_mode2 == MV_PMODE_1MV_HPEL_BILIN));

        if ((v->mv_mode == MV_PMODE_INTENSITY_COMP &&
                 v->mv_mode2 == MV_PMODE_MIXED_MV)
                || v->mv_mode == MV_PMODE_MIXED_MV)
        {
            status = bitplane_decoding(v->mv_type_mb_plane, &v->mv_type_is_raw, v);
            if (status < 0) return -1;
            av_log(v->s.avctx, AV_LOG_DEBUG, "MB MV Type plane encoding: "
                   "Imode: %i, Invert: %i\n", status>>1, status&1);
        } else {
            v->mv_type_is_raw = 0;
            memset(v->mv_type_mb_plane, 0, v->s.mb_stride * v->s.mb_height);
        }
        status = bitplane_decoding(v->s.mbskip_table, &v->skip_is_raw, v);
        if (status < 0) return -1;
        av_log(v->s.avctx, AV_LOG_DEBUG, "MB Skip plane encoding: "
               "Imode: %i, Invert: %i\n", status>>1, status&1);

        /* Hopefully this is correct for P frames */
        v->s.mv_table_index = get_bits(gb, 2); //but using ff_vc1_ tables
        v->cbpcy_vlc = &ff_vc1_cbpcy_p_vlc[get_bits(gb, 2)];
        if (v->dquant)
        {
            av_log(v->s.avctx, AV_LOG_DEBUG, "VOP DQuant info\n");
            vop_dquant_decoding(v);
        }

        v->ttfrm = 0; //FIXME Is that so ?
        if (v->vstransform)
        {
            v->ttmbf = get_bits1(gb);
            if (v->ttmbf)
            {
                v->ttfrm = ff_vc1_ttfrm_to_tt[get_bits(gb, 2)];
            }
        } else {
            v->ttmbf = 1;
            v->ttfrm = TT_8X8;
        }
        break;
    case FF_B_TYPE:
        if (v->extended_mv) v->mvrange = get_unary(gb, 0, 3);
        else v->mvrange = 0;
        v->k_x = v->mvrange + 9 + (v->mvrange >> 1); //k_x can be 9 10 12 13
        v->k_y = v->mvrange + 8; //k_y can be 8 9 10 11
        v->range_x = 1 << (v->k_x - 1);
        v->range_y = 1 << (v->k_y - 1);

        if (v->pq < 5) v->tt_index = 0;
        else if(v->pq < 13) v->tt_index = 1;
        else v->tt_index = 2;

        v->mv_mode = get_bits1(gb) ? MV_PMODE_1MV : MV_PMODE_1MV_HPEL_BILIN;
        v->s.quarter_sample = (v->mv_mode == MV_PMODE_1MV);
        v->s.mspel = v->s.quarter_sample;

        status = bitplane_decoding(v->direct_mb_plane, &v->dmb_is_raw, v);
        if (status < 0) return -1;
        av_log(v->s.avctx, AV_LOG_DEBUG, "MB Direct Type plane encoding: "
               "Imode: %i, Invert: %i\n", status>>1, status&1);
        status = bitplane_decoding(v->s.mbskip_table, &v->skip_is_raw, v);
        if (status < 0) return -1;
        av_log(v->s.avctx, AV_LOG_DEBUG, "MB Skip plane encoding: "
               "Imode: %i, Invert: %i\n", status>>1, status&1);

        v->s.mv_table_index = get_bits(gb, 2);
        v->cbpcy_vlc = &ff_vc1_cbpcy_p_vlc[get_bits(gb, 2)];

        if (v->dquant)
        {
            av_log(v->s.avctx, AV_LOG_DEBUG, "VOP DQuant info\n");
            vop_dquant_decoding(v);
        }

        v->ttfrm = 0;
        if (v->vstransform)
        {
            v->ttmbf = get_bits1(gb);
            if (v->ttmbf)
            {
                v->ttfrm = ff_vc1_ttfrm_to_tt[get_bits(gb, 2)];
            }
        } else {
            v->ttmbf = 1;
            v->ttfrm = TT_8X8;
        }
        break;
    }

    /* AC Syntax */
    v->c_ac_table_index = decode012(gb);
    if (v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE)
    {
        v->y_ac_table_index = decode012(gb);
    }
    /* DC Syntax */
    v->s.dc_table_index = get_bits1(gb);
    if ((v->s.pict_type == FF_I_TYPE || v->s.pict_type == FF_BI_TYPE) && v->dquant) {
        av_log(v->s.avctx, AV_LOG_DEBUG, "VOP DQuant info\n");
        vop_dquant_decoding(v);
    }

    v->bi_type = 0;
    if(v->s.pict_type == FF_BI_TYPE) {
        v->s.pict_type = FF_B_TYPE;
        v->bi_type = 1;
    }
    return 0;
}

int ff_vc1_decode_extradata(VC1Context *v)
{
    AVCodecContext *avctx = v->s.avctx;
    GetBitContext gb1, *pGb = &gb1;

    if (avctx->codec_id == CODEC_ID_WMV3)
    {
//        int count = 0;

        // looks like WMV3 has a sequence header stored in the extradata
        // advanced sequence header may be before the first frame
        // the last byte of the extradata is a version number, 1 for the
        // samples we can decode
        init_get_bits(pGb, avctx->extradata, avctx->extradata_size*8);

        if (vc1_decode_sequence_header(avctx, v, pGb) < 0)
          return -1;
    } else { // VC1/WVC1/WVP2
        const uint8_t *start = avctx->extradata;
        uint8_t *end = avctx->extradata + avctx->extradata_size;
        const uint8_t *next;
        int size, buf2_size;
        uint8_t *buf2 = NULL;
        int seq_initialized = 0, ep_initialized = 0;

        if(avctx->extradata_size < 16) {
            av_log(avctx, AV_LOG_ERROR, "Extradata size too small: %i\n", avctx->extradata_size);
            return -1;
        }

        buf2 = av_mallocz(avctx->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
        start = find_next_marker(start, end); // in WVC1 extradata first byte is its size, but can be 0 in mkv
        next = start;
        for(; next < end; start = next){
            next = find_next_marker(start + 4, end);
            size = next - start - 4;
            if(size <= 0) continue;
            buf2_size = vc1_unescape_buffer(start + 4, size, buf2);
            init_get_bits(pGb, buf2, buf2_size * 8);
            switch(AV_RB32(start)){
            case VC1_CODE_SEQHDR:
                if(vc1_decode_sequence_header(avctx, v, pGb) < 0){
                    av_free(buf2);
                    return -1;
                }
                seq_initialized = 1;
                break;
            case VC1_CODE_ENTRYPOINT:
                if(vc1_decode_entry_point(avctx, v, pGb) < 0){
                    av_free(buf2);
                    return -1;
                }
                ep_initialized = 1;
                break;
            }
        }
        av_free(buf2);
        if(!seq_initialized || !ep_initialized){
            av_log(avctx, AV_LOG_ERROR, "Incomplete extradata\n");
            return -1;
        }
        v->res_sprite = (avctx->codec_tag == MKTAG('W','V','P','2'));
    }
    return 0;
}