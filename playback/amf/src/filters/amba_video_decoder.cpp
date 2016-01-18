
/*
 * amba_video_decoder.cpp
 *
 * History:
 *    2010/9/30 - [Yu Jiankang] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "amba_video_decoder"
//#define AMDROID_DEBUG

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "pbif.h"
#include "engine_guids.h"
#if PLATFORM_ANDROID
#include <basetypes.h>
#else
#include "basetypes.h"
#endif
#include "iav_drv.h"
#include "ambas_vout.h"


#include "filter_list.h"

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "amdsp_common.h"
#include "am_util.h"
#include "amba_video_decoder.h"

#if !FFMPEG_VER_0_6
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_SUBTITLE AVMEDIA_TYPE_SUBTITLE
#define guess_format av_guess_format
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#endif

filter_entry g_amba_video_decoder = {
	"AmbaVideoDecoder",
	CAmbaVideoDecoder::Create,
	NULL,
	CAmbaVideoDecoder::AcceptMedia,
};

//#define fake_feeding

#define MB	(1024 * 1024)

#define BE_16(x) (((unsigned char *)(x))[0] <<  8 | \
		  ((unsigned char *)(x))[1])

#define BE_32(x) ((((unsigned char *)(x))[0] << 24) | \
                  (((unsigned char *)(x))[1] << 16) | \
                  (((unsigned char *)(x))[2] << 8)  | \
                   ((unsigned char *)(x))[3])

//-----------------------------------------------------------------------
//
// CAmbaVideoDecoder
//
//-----------------------------------------------------------------------
void CAmbaVideoDecoder::PrintBitstremBuffer(AM_U8* p, AM_UINT size)
{
#ifdef AM_DEBUG
    if (!(mLogOption & LogBinaryData))
        return;

    while (size > 3) {
        AMLOG_BINARY("  %2.2x %2.2x %2.2x %2.2x.\n", p[0], p[1], p[2], p[3]);
        p += 4;
        size -= 4;
    }

    if (size == 3) {
        AMLOG_BINARY("  %2.2x %2.2x %2.2x.\n", p[0], p[1], p[2]);
    } else if (size == 2) {
        AMLOG_BINARY("  %2.2x %2.2x.\n", p[0], p[1]);
    } else if (size == 1) {
        AMLOG_BINARY("  %2.2x.\n", p[0]);
    }
#endif
}

#ifdef DDEBUG_CHECK_DRAM_DATA_TRASH
void CAmbaVideoDecoder::storeDramData(AM_UINT i)
{
    AM_ASSERT((i == 0) || (i == 1));
    if ((i == 0) || (i == 1)) {
        memcpy(mpCheckDram[i], mpStartAddr, mSpace);
        if (!i) {
            mCheckCount ++;
        }
    }
}

void CAmbaVideoDecoder::checkDramDataTrashed()
{
    AM_UINT* val1 = (AM_UINT*)mpCheckDram[0], *val2 = (AM_UINT*)mpCheckDram[1];
    AM_UINT index = 0;

    AM_ASSERT(mCheckCount);
    AM_ASSERT(mNumberInt);

    if (mCheckCount) {
        while (index < mNumberInt) {
            if (*val1 != *val2) {
                AMLOG_INFO("data trashed, count %u, pos %u, ori val 0x%x, trashed val 0x%x.\n", mCheckCount, index, *val1, *val2);
            }
            val1 ++;
            val2 ++;
            index ++;
        }
    }
}
#endif

void CAmbaVideoDecoder::SetUdecModeConfig()
{
    AM_ASSERT(mpSharedRes);
    memset(&mUdecModeConfig, 0, sizeof(iav_udec_mode_config_t));
    memset(&mUdecDeintConfig, 0, sizeof(iav_udec_deint_config_t));

    //hard code here, to do
    mUdecModeConfig.postp_mode = mpSharedRes->ppmode;
    mpSharedRes->ppmode = mUdecModeConfig.postp_mode;
    AMLOG_INFO("=============ppmode = %d.============\n",mpSharedRes->ppmode);
    mpSharedRes->get_outpic = (1==mpSharedRes->ppmode);
    mUdecModeConfig.enable_deint = 0;
#if 0
    //enable_error_mode is obsolete
    //mUdecModeConfig.enable_error_mode = mpSharedRes->enable_udec_error_handling;
#endif
    mUdecModeConfig.pp_chroma_fmt_max = 2;
    AM_ASSERT(mMaxVoutWidth);
    AM_ASSERT(mMaxVoutHeight);
    AMLOG_INFO("mMaxVoutWidth %d, mMaxVoutHeight %d.\n", mMaxVoutWidth, mMaxVoutHeight);
    mUdecModeConfig.pp_max_frm_width = mMaxVoutWidth;
    mUdecModeConfig.pp_max_frm_height = mMaxVoutHeight;
    if (muDecType != UDEC_JPEG)
        mUdecModeConfig.pp_max_frm_num = 5;
    else
        mUdecModeConfig.pp_max_frm_num = 1;

    mUdecModeConfig.vout_mask = mVoutConfigMask; //hdmi
    mUdecModeConfig.num_udecs = 1;
    if (mpSharedRes->dspConfig.enableDeinterlace) {
        mUdecModeConfig.enable_deint = 1;

        mUdecDeintConfig.init_tff = mpSharedRes->dspConfig.deinterlaceConfig.init_tff;
        mUdecDeintConfig.deint_lu_en = mpSharedRes->dspConfig.deinterlaceConfig.deint_lu_en;
        mUdecDeintConfig.deint_ch_en = mpSharedRes->dspConfig.deinterlaceConfig.deint_ch_en;
        mUdecDeintConfig.osd_en = mpSharedRes->dspConfig.deinterlaceConfig.osd_en;

        mUdecDeintConfig.deint_mode = mpSharedRes->dspConfig.deinterlaceConfig.deint_mode;
        mUdecDeintConfig.deint_spatial_shift = mpSharedRes->dspConfig.deinterlaceConfig.deint_spatial_shift;
        mUdecDeintConfig.deint_lowpass_shift = mpSharedRes->dspConfig.deinterlaceConfig.deint_lowpass_shift;

        mUdecDeintConfig.deint_lowpass_center_weight = mpSharedRes->dspConfig.deinterlaceConfig.deint_lowpass_center_weight;
        mUdecDeintConfig.deint_lowpass_hor_weight = mpSharedRes->dspConfig.deinterlaceConfig.deint_lowpass_hor_weight;
        mUdecDeintConfig.deint_lowpass_ver_weight = mpSharedRes->dspConfig.deinterlaceConfig.deint_lowpass_ver_weight;

        mUdecDeintConfig.deint_gradient_bias = mpSharedRes->dspConfig.deinterlaceConfig.deint_gradient_bias;
        mUdecDeintConfig.deint_predict_bias = mpSharedRes->dspConfig.deinterlaceConfig.deint_predict_bias;
        mUdecDeintConfig.deint_candidate_bias = mpSharedRes->dspConfig.deinterlaceConfig.deint_candidate_bias;

        mUdecDeintConfig.deint_spatial_score_bias = mpSharedRes->dspConfig.deinterlaceConfig.deint_spatial_score_bias;
        mUdecDeintConfig.deint_temporal_score_bias = mpSharedRes->dspConfig.deinterlaceConfig.deint_temporal_score_bias;

        mUdecModeConfig.deint_config = &mUdecDeintConfig;
    }

    if (mpSharedRes->enable_feature_constrains) {
        mUdecModeConfig.feature_constrains.set_constrains_enable = 1;
        mUdecModeConfig.feature_constrains.always_disable_l2_cache = mpSharedRes->always_disable_l2_cache;
        mUdecModeConfig.feature_constrains.h264_no_fmo = mpSharedRes->h264_no_fmo;
        mUdecModeConfig.feature_constrains.vout_no_lcd = mpSharedRes->vout_no_LCD;
        mUdecModeConfig.feature_constrains.vout_no_hdmi = mpSharedRes->vout_no_HDMI;
        mUdecModeConfig.feature_constrains.no_deinterlacer = mpSharedRes->no_deinterlacer;
    }
    AMLOG_INFO("mpSharedRes(%p)->enable_horz_dewarp %d.\n", mpSharedRes, mpSharedRes->enable_horz_dewarp);
    if (mpSharedRes->enable_horz_dewarp) {
        AM_ASSERT((1<<eVoutLCD) == mVoutConfigMask);
        mVoutConfigMask = (1<<eVoutLCD);
        mUdecModeConfig.vout_mask = (1<<eVoutLCD);

        mUdecModeConfig.enable_horizontal_dewarp = 1;
    }
}

void CAmbaVideoDecoder::SetUdecConfig(AM_INT index)
{
    AM_ASSERT(index >=0 && index < DMAX_UDEC_INSTANCE_NUM);
    if (index < 0 || index >= DMAX_UDEC_INSTANCE_NUM) {
        AMLOG_ERROR("invalid udec index %d in SetUdecConfig.\n", index);
        return;
    }

    memset(&mUdecConfig[index], 0, sizeof(iav_udec_config_t));

    //hard code here
    mUdecConfig[index].frm_chroma_fmt_max = UDEC_CFG_FRM_CHROMA_FMT_422; // 4:2:0
    mUdecConfig[index].dec_types = mpSharedRes->codec_mask;
    if(isVidResLargerThan1080P())
    {
        mUdecConfig[index].tiled_mode = mpSharedRes->dspConfig.preset_tilemode;//TODO: should mdfed to "5" after memory of DSP increased
        mUdecConfig[index].max_frm_num = mpSharedRes->max_frm_number;//7;
        mUdecConfig[index].max_fifo_size = (mpSharedRes->dspConfig.preset_bits_fifo_size>0)?mpSharedRes->dspConfig.preset_bits_fifo_size:(8*MB);
    }
    else
    {
        mUdecConfig[index].tiled_mode = mpSharedRes->dspConfig.preset_tilemode;
        mUdecConfig[index].max_frm_num = 20;
        mUdecConfig[index].max_fifo_size = 4*MB;
    }

    // fix issue2516
    // udec supports clip whose height is greater than width
    // and max_frm_width&max_frm_height should be swap if so
    if (mpCodec->width >= mpCodec->height) {
        mUdecConfig[index].max_frm_width = mpCodec->width>3840?3840:mpCodec->width;//TODO: tmp mdf for S2 memory bottleneck
        mUdecConfig[index].max_frm_height = mpCodec->height;
    } else {
        mUdecConfig[index].max_frm_width = mpCodec->height;
        mUdecConfig[index].max_frm_height = mpCodec->width>3840?3840:mpCodec->width;//TODO: tmp mdf for S2 memory bottleneck
    }

}

AM_INT CAmbaVideoDecoder::EnterUdecMode(void)
{
    AM_INT state;
    AM_INT ret;
    ret = ioctl(mIavFd, IAV_IOC_GET_STATE, &state);
    //AM_ASSERT(state == IAV_STATE_IDLE);
    if (state != IAV_STATE_IDLE) {
        //AMLOG_ERROR("[dsp mode]: try UDEC mode, but dsp state(%d) is not IDLE, deny request.\n", state);
        //return -1;

        AMLOG_ERROR("UDEC Not in IDLE mode, enter IDLE mode first.\n");
        ret = ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
        if (ret != 0) {
            AMLOG_ERROR("UDEC enter IDLE mode fail, ret %d.\n", ret);
            return -1;
        }
        //return 0;
    }

    //preset vout's osd before enter udec mode
    AM_UINT i = 0;
    iav_vout_fb_sel_t fb_sel;
    for (i = 0; i < eVoutCnt; i++) {
        if (mVoutConfig[i].failed) {
            continue;
        }
        if (mpSharedRes->dspConfig.voutConfigs.voutConfig[i].osd_disable/* != mVoutConfig[i].osd_disable*/) {
            AMLOG_WARN("vout(osd) %i have different setting, request osd_disable %d, current osd_disable %d.\n", i, mpSharedRes->dspConfig.voutConfigs.voutConfig[i].osd_disable, mVoutConfig[i].osd_disable);
            memset(&fb_sel, 0, sizeof(iav_vout_fb_sel_t));
            fb_sel.vout_id = i;

            if (mpSharedRes->dspConfig.voutConfigs.voutConfig[i].osd_disable) {
                AMLOG_WARN("disable osd on vout %d.\n", i);
                fb_sel.fb_id = -1;
            } else {
                AMLOG_WARN("enable osd on vout %d.\n", i);
                fb_sel.fb_id = 0;//link to fb 0, hard code here
            }

            if(ioctl(mIavFd, IAV_IOC_VOUT_SELECT_FB, &fb_sel)) {
                AM_ERROR("IAV_IOC_VOUT_SELECT_FB Failed!");
                perror("IAV_IOC_VOUT_SELECT_FB");
                continue;
            }
            mVoutConfig[i].osd_disable = mpSharedRes->dspConfig.voutConfigs.voutConfig[i].osd_disable;
            mpSharedRes->dspConfig.voutConfigs.need_restore_osd[i] = 1;
        }
    }

    SetUdecModeConfig();
    SetUdecConfig(0);
    mUdecModeConfig.udec_config = &mUdecConfig[0];
    AMLOG_INFO("udec mode config:\n");
    AMLOG_INFO("  ppmode %d, enable_deint %d, chroma_format %d.\n", mUdecModeConfig.postp_mode, mUdecModeConfig.enable_deint, mUdecModeConfig.pp_chroma_fmt_max);
    AMLOG_INFO("  pp max width %d, height %d, num of buffers %d.\n", mUdecModeConfig.pp_max_frm_width, mUdecModeConfig.pp_max_frm_height, mUdecModeConfig.pp_max_frm_num);
    AMLOG_INFO("  num_udecs %d, vout_mask %x, enable_horizontal_dewarp %d.\n", mUdecModeConfig.num_udecs, mUdecModeConfig.vout_mask, mUdecModeConfig.enable_horizontal_dewarp);

    AMLOG_INFO("feature constrains: enable %d, codec mask 0x%x, request always disable l2 %d.\n", mUdecModeConfig.feature_constrains.set_constrains_enable, mpSharedRes->codec_mask, mUdecModeConfig.feature_constrains.always_disable_l2_cache);
    AMLOG_INFO("           h264 no fmo %d, no lcd %d, no hdmi %d, no deinterlacer %d.\n", mUdecModeConfig.feature_constrains.h264_no_fmo, mUdecModeConfig.feature_constrains.vout_no_lcd, mUdecModeConfig.feature_constrains.vout_no_hdmi, mUdecModeConfig.feature_constrains.no_deinterlacer);

    AMLOG_INFO("enter udec mode start\n");
    if (ioctl(mIavFd, IAV_IOC_ENTER_UDEC_MODE, &mUdecModeConfig) < 0) {
        perror("IAV_IOC_ENTER_UDEC_MODE");
        return -1;
    }
    mbEnterUDECMode = true;
    AMLOG_INFO("enter udec mode done\n");
    return 0;
}

AM_INT CAmbaVideoDecoder::InitUdec(AM_INT index)
{
    AM_U8 i;
    AM_ASSERT(index >=0 && index < DMAX_UDEC_INSTANCE_NUM);
    if (index < 0 || index >= DMAX_UDEC_INSTANCE_NUM) {
        AMLOG_ERROR("invalid udec index %d in InitUdec.\n", index);
        return -1;
    }

    memset(&mUdecInfo[index], 0, sizeof(mUdecInfo[index]));

    memset(&mUdecVoutConfig[0], 0, eVoutCnt * sizeof(iav_udec_vout_config_t));

    mUdecInfo[index].udec_id = 0;
    mUdecInfo[index].enable_err_handle = mpSharedRes->dspConfig.errorHandlingConfig[0].enable_udec_error_handling;
    mUdecInfo[index].udec_type = muDecType;
    mUdecInfo[index].enable_pp = 1;
    mUdecInfo[index].enable_deint = mUdecModeConfig.enable_deint;
    mUdecInfo[index].interlaced_out = 0;
    if(mpSharedRes->force_decode){
        mUdecInfo[index].other_flags |= IAV_UDEC_FORCE_DECODE;
    }
    if(mpSharedRes->validation_only){//designed for video editing
        mUdecInfo[index].other_flags |= IAV_UDEC_VALIDATION_ONLY;
    }
    if (mUdecInfo[index].enable_err_handle) {
        AM_ASSERT(mpSharedRes->dspConfig.errorHandlingConfig[0].enable_udec_error_handling);
        mUdecInfo[index].concealment_mode = mpSharedRes->dspConfig.errorHandlingConfig[0].error_concealment_mode;
        mUdecInfo[index].concealment_ref_frm_buf_id = mpSharedRes->dspConfig.errorHandlingConfig[0].error_concealment_frame_id;
        AMLOG_INFO("enable udec error handling: concealment mode %d, frame id %d.\n",mUdecInfo[index].concealment_mode, mUdecInfo[index].concealment_ref_frm_buf_id);
    }
//#if _use_iav_udec_info_t_
#if 0
    mUdecInfo[index].num_vout = mVoutNumber;
    mUdecInfo[index].vout_config = &mUdecVoutConfig[mVoutStartIndex];
    mUdecInfo[index].input_center_x = mpCodec->width / 2;
    mUdecInfo[index].input_center_y = mpCodec->height / 2;
#else
    mUdecInfo[index].vout_configs.num_vout = mVoutNumber;
    mUdecInfo[index].vout_configs.vout_config = &mUdecVoutConfig[mVoutStartIndex];
    mUdecInfo[index].vout_configs.input_center_x = (mpCodec->width>3840?3840:mpCodec->width) / 2;//TODO: tmp mdf for S2 memory bottleneck
    mUdecInfo[index].vout_configs.input_center_y = mpCodec->height / 2;
#endif
    AMLOG_INFO("***mpCodec->width %d, mpCodec->height %d.\n", mpCodec->width, mpCodec->height);

    if (isVidResLargerThan1080P()) {
        mUdecInfo[index].bits_fifo_size = (mpSharedRes->dspConfig.preset_bits_fifo_size>0)?mpSharedRes->dspConfig.preset_bits_fifo_size:(8*1024*1024);
        mUdecInfo[index].ref_cache_size = mpSharedRes->dspConfig.preset_ref_cache_size;
    } else {
        mUdecInfo[index].bits_fifo_size = 4*1024*1024;
        mUdecInfo[index].ref_cache_size = 0;
    }

    switch (muDecType) {
    case UDEC_H264:
#if TARGET_USE_AMBARELLA_S2_DSP
        mUdecInfo[index].u.h264.pjpeg_buf_size = 16*1024*1024;
#else
        mUdecInfo[index].u.h264.pjpeg_buf_size = 4*1024*1024;
#endif
        mUdecInfo[index].noncachable_buffer = mpSharedRes->noncachable_buffer;
        break;

    case UDEC_MP12:
    case UDEC_MP4H:
        mUdecInfo[index].u.mpeg.deblocking_flag = mpSharedRes->dspConfig.deblockingFlag;
        mUdecInfo[index].u.mpeg.pquant_mode = mpSharedRes->dspConfig.deblockingConfig.pquant_mode;
        for(i=0; i<32; i++ )
        {
            mUdecInfo[index].u.mpeg.pquant_table[i] = (AM_U8)mpSharedRes->dspConfig.deblockingConfig.pquant_table[i];
        }
        mUdecInfo[index].u.mpeg.is_avi_flag = mpSharedRes->is_avi_flag;
        AMLOG_INFO("MPEG12/4 deblocking_flag %d, pquant_mode %d.\n", mUdecInfo[index].u.mpeg.deblocking_flag, mUdecInfo[index].u.mpeg.pquant_mode);
        for (i = 0; i<4; i++) {
            AMLOG_INFO(" pquant_table[%d - %d]:\n", i*8, i*8+7);
            AMLOG_INFO(" %d, %d, %d, %d, %d, %d, %d, %d.\n", \
                mUdecInfo[index].u.mpeg.pquant_table[i*8], mUdecInfo[index].u.mpeg.pquant_table[i*8+1], mUdecInfo[index].u.mpeg.pquant_table[i*8+2], mUdecInfo[index].u.mpeg.pquant_table[i*8+3], \
                mUdecInfo[index].u.mpeg.pquant_table[i*8+4], mUdecInfo[index].u.mpeg.pquant_table[i*8+5], mUdecInfo[index].u.mpeg.pquant_table[i*8+6], mUdecInfo[index].u.mpeg.pquant_table[i*8+7] \
            );
        }
        AMLOG_INFO("MPEG4 is_avi_flag %d.\n", mUdecInfo[index].u.mpeg.is_avi_flag);
        break;

    case UDEC_JPEG:
        mUdecInfo[index].u.jpeg.still_bits_circular = 0;
        mUdecInfo[index].u.jpeg.still_max_decode_width = mpCodec->width;
        mUdecInfo[index].u.jpeg.still_max_decode_height = mpCodec->height;
        break;

    case UDEC_VC1:
        break;

    default:
        AMLOG_ERROR("udec type %d not implemented\n", muDecType);
        return -1;
    }

    for (AM_INT i = 0; i< eVoutCnt; i ++) {
        mUdecVoutConfig[i].vout_id = i;
        mUdecVoutConfig[i].target_win_width = mVoutConfig[i].size_x;
        mUdecVoutConfig[i].target_win_height = mVoutConfig[i].size_y >> mVoutConfig[i].vout_mode;
        mUdecVoutConfig[i].target_win_offset_x = mVoutConfig[i].pos_x;
        mUdecVoutConfig[i].target_win_offset_y = mVoutConfig[i].pos_y >> mVoutConfig[i].vout_mode;
        //setup win_width win_height should be zero
        mUdecVoutConfig[i].win_width = mVoutConfig[i].size_x;
        mUdecVoutConfig[i].win_height = mVoutConfig[i].size_y >> mVoutConfig[i].vout_mode;
        mUdecVoutConfig[i].win_offset_x = mVoutConfig[i].pos_x;
        mUdecVoutConfig[i].win_offset_y = mVoutConfig[i].pos_y >> mVoutConfig[i].vout_mode;
        mVoutConfig[i].height >>= mVoutConfig[i].vout_mode;
        mUdecVoutConfig[i].zoom_factor_x = 1;
        mUdecVoutConfig[i].zoom_factor_y = 1;
        mUdecVoutConfig[i].flip = mVoutConfig[i].flip;
        mUdecVoutConfig[i].rotate = mVoutConfig[i].rotate;
        mUdecVoutConfig[i].disable = mVoutConfig[i].enable ? 0 : 1;
    }
    /*float ratio_x,ratio_y;
    for(AM_INT j = 0;j< eVoutCnt;j++){
	ratio_x = (float)mVoutConfig[j].width / mpCodec->width;
	ratio_y = (float)mVoutConfig[j].height/ mpCodec->height;
	//AM_INFO("ratio_x= %f, %f\n",ratio_x,ratio_y);
	mUdecVoutConfig[j].zoom_factor_x = (int)(ratio_x * 0x10000);
	mUdecVoutConfig[j].zoom_factor_y = (int)(ratio_y * 0x10000);
	//AM_INFO("mUdecVoutConfig[%d].zoom_factor_x = 0x%x\n",j,mUdecVoutConfig[j].zoom_factor_x);
    }//*/

    //AM_ASSERT(mpSharedRes->mUdecState == _UDEC_IDLE);
    AMLOG_INFO("start IAV_IOC_INIT_UDEC....\n");
    AMLOG_INFO("***num_vout %d, start index %d, vout_mask 0x%x.\n", mUdecInfo[index].vout_configs.num_vout, mVoutStartIndex, mUdecModeConfig.vout_mask);
    if (ioctl(mIavFd, IAV_IOC_INIT_UDEC, &mUdecInfo[index]) < 0) {
        perror("IAV_IOC_INIT_UDEC");
        return -1;
    }
    AMLOG_INFO("IAV_IOC_INIT_UDEC done.\n");

    return 0;
}

AM_INT CAmbaVideoDecoder::ReleaseUdec(AM_INT index)
{
    AM_ASSERT(index >=0 && index < DMAX_UDEC_INSTANCE_NUM);
    if (index < 0 || index >= DMAX_UDEC_INSTANCE_NUM) {
        AMLOG_ERROR("invalid udec index %d in ReleaseUdec.\n", index);
        return -1;
    }

    AMLOG_INFO("start ReleaseUdec %d...\n", index);
    if (ioctl(mIavFd, IAV_IOC_RELEASE_UDEC, index) < 0) {
        perror("IAV_IOC_DESTROY_UDEC");
        AMLOG_ERROR("IAV_IOC_RELEASE_UDEC %d fail.\n", index);
        return -2;
    }
/*
    if(1 == mpSharedRes->ppmode){
             if(UnMapDSP()!=ME_OK){
                  AMLOG_ERROR("CAmbaVideoDecoder: UnMapDSP Failed!\n");
             }
    }
*/
    AMLOG_INFO("end ReleaseUdec\n");
    return 0;
}

AM_ERR CAmbaVideoDecoder::ConfigVOUTVideo(AM_INT vout_id)
{
    AM_INT x_offset = 0, y_offset = 0;
    AM_INT pic_width = 0, pic_height = 0, pic_temp = 0;
    AM_INT vout_width = 0, vout_height = 0;

    pic_width = mpCodec->width;
    pic_height = mpCodec->height;
    vout_width = mVoutConfig[vout_id].width;
    vout_height = mVoutConfig[vout_id].height;

    //for bug#2105, streams which need adjusting width to keep DAR should config UPDATE_VOUT_CONFIG in video renderer
    if(mpSharedRes->vid_width_DAR){
        pic_width = mpSharedRes->vid_width_DAR;
        AMLOG_INFO("[AAR-before-EnterUdecMode]: video need adjust for DAR, vout=%d, pic_width=%d, pic_height=%d.\n", vout_id, pic_width,pic_height);
    }

    if (vout_width<vout_height) {    //pic's w-h need to fit vout ratio
        if((eVoutHDMI==vout_id) || (0==mpSharedRes->vid_rotation_info ||180==mpSharedRes->vid_rotation_info)){//roy mdf 2012.03.21, for vout LCD and video rotate 90/270 case, NO switch width-height{
            pic_temp = pic_width;
            pic_width = pic_height;
            pic_height = pic_temp;
        }
    }else{
        if((eVoutLCD==vout_id) && ((90==mpSharedRes->vid_rotation_info ||270==mpSharedRes->vid_rotation_info))){
            pic_temp = pic_width;
            pic_width = pic_height;
            pic_height = pic_temp;
        }
    }
    AMLOG_INFO("[AAR-before-EnterUdecMode]: vout=%d, vid_rotation_info=%d, after pic width-height switch for ratio, pic_width=%d, pic_height=%d.\n", vout_id,mpSharedRes->vid_rotation_info,pic_width,pic_height);

    if(pic_width * vout_height  > vout_width * pic_height){
        pic_height = vout_width * pic_height /pic_width;
        pic_width = vout_width;
        pic_height = (pic_height+15)&(~15);//round up
        pic_height = (pic_height > vout_height) ? vout_height : pic_height;//needless, pic_height must be not greater than vout_height
        x_offset = 0;
        y_offset = (vout_height-pic_height)/2;
    }else{
        pic_width = vout_height * pic_width /pic_height;
        pic_height = vout_height;
        pic_width = (pic_width+15)&(~15);
        pic_width = (pic_width > vout_width) ? vout_width : pic_width;
        y_offset = 0;
        x_offset = (vout_width-pic_width)/2;
    }

    if(  ( (x_offset + pic_width) > vout_width ) ||( (y_offset + pic_height) > vout_height )
        || x_offset<0 ||y_offset<0 || pic_width<0 ||pic_height<0) {
        AMLOG_INFO("[AAR-before-EnterUdecMode]: out-of-range, vout:%d, x = %d,y = %d, width: %d, height: %d\n",vout_id,x_offset, y_offset, pic_width, pic_height);
        return ME_ERROR;
    }

    AMLOG_INFO("[AAR-before-EnterUdecMode] VOUT id %d, sink_id=%d, video original x=%hu, y=%hu, w=%hu, h=%hu\n",
        vout_id,
        mVoutConfig[vout_id].sink_id,
        sink_mode[vout_id].video_offset.offset_x,
        sink_mode[vout_id].video_offset.offset_y,
        sink_mode[vout_id].video_size.video_width,
        sink_mode[vout_id].video_size.video_height);

    if ((sink_mode[vout_id].video_offset.offset_x == x_offset)
        && (sink_mode[vout_id].video_offset.offset_y == y_offset)
        && (sink_mode[vout_id].video_size.video_width == pic_width)
        && (sink_mode[vout_id].video_size.video_height == pic_height)) {
        AMLOG_INFO("[AAR-before-EnterUdecMode] VOUT id %d, sink_id=%d, video offset-size no change, do nothing.\n", vout_id, mVoutConfig[vout_id].sink_id);
        return ME_ERROR;
    }

    sink_mode[vout_id].video_offset.offset_x = x_offset;
    sink_mode[vout_id].video_offset.offset_y = y_offset;
    sink_mode[vout_id].video_size.video_width = pic_width;
    sink_mode[vout_id].video_size.video_height = pic_height;

    mVoutConfig[vout_id].pos_x = x_offset;
    mVoutConfig[vout_id].pos_y = y_offset;
    mVoutConfig[vout_id].size_x= pic_width;
    mVoutConfig[vout_id].size_y= pic_height;

    AMLOG_INFO("[AAR-before-EnterUdecMode] VOUT id %d, sink_id=%d, video&vout new x=%hu, y=%hu, w=%hu, h=%hu.\n",
        vout_id,
        mVoutConfig[vout_id].sink_id,
        sink_mode[vout_id].video_offset.offset_x,
        sink_mode[vout_id].video_offset.offset_y,
        sink_mode[vout_id].video_size.video_width,
        sink_mode[vout_id].video_size.video_height);

    return ME_OK;
}

AM_ERR CAmbaVideoDecoder::ConfigVOUTMode(AM_INT vout_id)
{
    float fps = (float)mpStream->r_frame_rate.num/(float)mpStream->r_frame_rate.den;
    if (4096==mpCodec->width
        && 2160==mpCodec->height
        && fps>23.5 && fps<24.5) {
        sink_mode[vout_id].mode = AMBA_VIDEO_MODE_2160P24_SE;
        sink_mode[vout_id].frame_rate = AMBA_VIDEO_FPS_24;
        mVoutConfig[vout_id].width = 4096;
        mVoutConfig[vout_id].height = 2160;
    } else if (3840==mpCodec->width
        && 2160==mpCodec->height
        && fps>23.5 && fps<24.5) {
        sink_mode[vout_id].mode = AMBA_VIDEO_MODE_2160P24;
        sink_mode[vout_id].frame_rate = AMBA_VIDEO_FPS_24;
        mVoutConfig[vout_id].width = 3840;
        mVoutConfig[vout_id].height = 2160;
    } else if (3840==mpCodec->width
        && 2160==mpCodec->height
        && fps>24.5 && fps<25.5) {
        sink_mode[vout_id].mode = AMBA_VIDEO_MODE_2160P25;
        sink_mode[vout_id].frame_rate = AMBA_VIDEO_FPS_25;
        mVoutConfig[vout_id].width = 3840;
        mVoutConfig[vout_id].height = 2160;
    } else if (3840==mpCodec->width
        && 2160==mpCodec->height
        && fps>29.5 && fps<30.5) {
        sink_mode[vout_id].mode = AMBA_VIDEO_MODE_2160P30;
        sink_mode[vout_id].frame_rate = AMBA_VIDEO_FPS_29_97;
        mVoutConfig[vout_id].width = 3840;
        mVoutConfig[vout_id].height = 2160;
    } else if (3840>mpCodec->width
        && 2160>mpCodec->height
        && 1920<=mpCodec->width
        && 1080<=mpCodec->height) {
        sink_mode[vout_id].mode = AMBA_VIDEO_MODE_1080P;
        sink_mode[vout_id].frame_rate = AMBA_VIDEO_FPS_AUTO;
        mVoutConfig[vout_id].width = 1920;
        mVoutConfig[vout_id].height = 1080;
    } else if (1920>mpCodec->width
        && 1080>mpCodec->height) {
        sink_mode[vout_id].mode = AMBA_VIDEO_MODE_720P;
        sink_mode[vout_id].frame_rate = AMBA_VIDEO_FPS_AUTO;
        mVoutConfig[vout_id].width = 1280;
        mVoutConfig[vout_id].height = 720;
    } else {
        AMLOG_WARN("[Modefps-before-EnterUdecMode] vout=%d, UNhandled case!!! video stream w=%d,h=%d,fps=%f.\n", vout_id,mpCodec->width, mpCodec->height, fps);
        return ME_NO_IMPL;
    }
    AMLOG_INFO("[Modefps-before-EnterUdecMode] vout=%d, video frame rate num %d, den %d, fps %f, will set sink_mode fps=%u, mode=0x%x, vout_width=%d, vout_height=%d\n",
        vout_id, mpStream->r_frame_rate.num, mpStream->r_frame_rate.den, fps,
        sink_mode[vout_id].frame_rate, sink_mode[vout_id].mode, mVoutConfig[vout_id].width, mVoutConfig[vout_id].height);

    return ME_OK;
}

AM_ERR CAmbaVideoDecoder::ConfigDecoder()
{
    AM_INT ret = 0, i = 0;
    AM_ERR needCfgMode=ME_ERROR, needCfgVideo=ME_ERROR;
    mMaxVoutWidth = 0;
    mMaxVoutHeight = 0;
    //parse voutconfig
    mVoutNumber = 0;
    mVoutStartIndex = 0;

    //get vout parameters
    for (i = 0; i < eVoutCnt; i++) {
        /*if (!(mVoutConfigMask&(1<<i))) {
            AMLOG_INFO(" not config vout %d.\n", i);
            continue;
        }*/
        ret = getVoutParamsEx(mIavFd, i, &mVoutConfig[i], &sink_mode[i]);
        AMLOG_INFO("VOUT id %d, failed?%d.  size_x %d, size_y %d, pos_x %d, pos_y %d.\n", i, mVoutConfig[i].failed, mVoutConfig[i].size_x, mVoutConfig[i].size_y, mVoutConfig[i].pos_x, mVoutConfig[i].pos_y);
        AMLOG_INFO("    width %d, height %d, flip %d, rotate %d.\n", mVoutConfig[i].width, mVoutConfig[i].height, mVoutConfig[i].flip, mVoutConfig[i].rotate);
        if (ret < 0 || mVoutConfig[i].failed || !mVoutConfig[i].width || !mVoutConfig[i].height) {
            mVoutConfigMask &= ~(1<<i);
            AMLOG_ERROR("vout %d failed.\n", i);
            continue;
        }

        if (mVoutConfigMask & (1<<i)) {
            mVoutNumber ++;
            if (mVoutConfig[i].width > mMaxVoutWidth) {
                mMaxVoutWidth = mVoutConfig[i].width;
            }
            if (mVoutConfig[i].height > mMaxVoutHeight) {
                mMaxVoutHeight = mVoutConfig[i].height;
            }
        }

#if TARGET_USE_AMBARELLA_S2_DSP
        if ((mpSharedRes->pbConfig.ar_enable ||mpSharedRes->pbConfig.auto_vout_enable) && 0==mVoutConfig[i].failed) {
            AMLOG_INFO("[xx-before-EnterUdecMode] VOUT id %d,  Original sink_mode=%u, frame_rate=%u.\n", i, sink_mode[i].mode, sink_mode[i].frame_rate);

            if (mpSharedRes->pbConfig.auto_vout_enable && mpStream->r_frame_rate.den) {
                needCfgMode = ConfigVOUTMode(i);
            }

            if (mpSharedRes->pbConfig.ar_enable) {
                needCfgVideo = ConfigVOUTVideo(i);
            }

            if (ME_OK == needCfgMode
                || ME_OK == needCfgVideo) {
                AMLOG_INFO("[xx-before-EnterUdecMode] VOUT id %d,  sink_id=%d, IAV_IOC_VOUT_SELECT_DEV.\n", i, mVoutConfig[i].sink_id);
                if (ioctl(mIavFd, IAV_IOC_VOUT_SELECT_DEV, mVoutConfig[i].sink_id) < 0) {
                    perror("IAV_IOC_VOUT_SELECT_DEV");
                    AMLOG_ERROR("[xx-before-EnterUdecMode] VOUT id %d,  sink_id=%d, IAV_IOC_VOUT_SELECT_DEV failed.\n", i, mVoutConfig[i].sink_id);
                    continue;
                }
                AMLOG_INFO("[xx-before-EnterUdecMode] VOUT id %d,  sink_id=%d, IAV_IOC_VOUT_SELECT_DEV done.\n", i, mVoutConfig[i].sink_id);

                sink_mode[i].direct_to_dsp = 1;
                AMLOG_INFO("[xx-before-EnterUdecMode] VOUT id %d, sink_id=%d, direct_to_dsp=%u, IAV_IOC_VOUT_CONFIGURE_SINK.\n", i, mVoutConfig[i].sink_id, sink_mode[i].direct_to_dsp);
                if (ioctl(mIavFd, IAV_IOC_VOUT_CONFIGURE_SINK, &sink_mode[i]) < 0) {
                    perror("IAV_IOC_VOUT_CONFIGURE_SINK");
                    AMLOG_ERROR("[xx-before-EnterUdecMode] VOUT id %d,  sink_id=%d, IAV_IOC_VOUT_CONFIGURE_SINK failed.\n", i, mVoutConfig[i].sink_id);
                    continue;
                }
                AMLOG_INFO("[xx-before-EnterUdecMode] VOUT id %d, sink_id=%d, IAV_IOC_VOUT_CONFIGURE_SINK done.\n", i, mVoutConfig[i].sink_id);
            }
        }
#endif
    }

    if (mVoutConfigMask & (1<<eVoutLCD)) {
        mVoutStartIndex = eVoutLCD;
    } else if (mVoutConfigMask & (1<<eVoutHDMI)) {
        mVoutStartIndex = eVoutHDMI;
    }
    AMLOG_INFO("mVoutStartIndex %d, mVoutNumber %d, mVoutConfigMask 0x%x.\n", mVoutStartIndex, mVoutNumber, mVoutConfigMask);

    AM_UINT vFormat = 0;

    AMLOG_INFO("stream frame rate num %d, den %d, %f.\n", mpStream->r_frame_rate.num, mpStream->r_frame_rate.den, (float)mpStream->r_frame_rate.num/(float)mpStream->r_frame_rate.den);

    mbAddUdecWarpper = (mpSharedRes->dspConfig.addVideoDataType == eAddVideoDataType_iOneUDEC);

    switch (mpCodec->codec_id) {
        case CODEC_ID_VC1:
            AM_INFO("-----UDEC_VC1\n----");
            vFormat = UDEC_VFormat_VC1;
            muDecType = UDEC_VC1;
            break;
        case CODEC_ID_WMV3:
            AM_INFO("-----UDEC_VC1(WMV3)\n----");
            vFormat = UDEC_VFormat_WMV3;
            muDecType = UDEC_VC1;
            break;

        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
            AM_INFO("-----UDEC_MP12\n----");
            vFormat = UDEC_VFormat_MPEG12;
            muDecType = UDEC_MP12;
            //mbAddUdecWarpper = false;
            break;

        case CODEC_ID_MPEG4:
            AM_INFO("-----UDEC_MP4H\n----");
            muDecType = UDEC_MP4H;
            vFormat = UDEC_VFormat_MPEG4;
            mbMP4WV1F = (mpCodec->codec_tag == 0x46315657); //AV_RL32("WV1F")
            break;
        case CODEC_ID_H264:
            AM_INFO("-----UDEC_H264\n----");
            muDecType = UDEC_H264;
            vFormat = UDEC_VFormat_H264;
            break;
        default:
            AM_INFO("-----not fully hardware support format %d\n----", mpCodec->codec_id);
            mbAddUdecWarpper = false;
            break;
    }
    AMLOG_INFO("try create udecoder %d\n", muDecType);

    ret = EnterUdecMode();
    if (ret) {
        AMLOG_ERROR("EnterUdecMode fail, ret %d.\n", ret);
        return ME_ERROR;
    }

    ret = InitUdec(0);

    if (ret) {
        AMLOG_ERROR("InitUdec fail, ret %d.\n", ret);
        return ME_ERROR;
    }

#ifdef fake_feeding
    mSpace = 16*1024*1024;
    mpStartAddr = (AM_U8*)malloc(mSpace);
    mpEndAddr = mpStartAddr + mSpace;
    mpCurrAddr = mpStartAddr;
#else
    mpStartAddr = mUdecInfo[0].bits_fifo_start;
    mpEndAddr = mpStartAddr + mUdecInfo[0].bits_fifo_size;
    mpCurrAddr = mpStartAddr;
    mSpace = mUdecInfo[0].bits_fifo_size;
#endif

//debug code
#ifdef DDEBUG_CHECK_DRAM_DATA_TRASH
    mpCheckDram[0] = (AM_U8*)malloc(mSpace);
    mpCheckDram[1] = (AM_U8*)malloc(mSpace);
    AM_ASSERT(mpCheckDram[0]);
    AM_ASSERT(mpCheckDram[1]);
    mNumberInt = mSpace/sizeof(AM_UINT);
    AM_ASSERT(!(mSpace%(sizeof(AM_UINT))));
#endif

#ifdef AM_DEBUG
    mdLastEndPointer = mpCurrAddr;
    mdWriteSize = 0;
#endif

    AMLOG_INFO("BSB created, start = 0x%p, length = 0x%x\n", mpStartAddr, mSpace);
    //debug code
    if (mpSharedRes->specified_framerate_num && mpSharedRes->specified_framerate_den) {
        mSpecifiedFrameRateTick = ((AM_U64)mpSharedRes->specified_framerate_den * IParameters::TimeUnitDen_90khz)/mpSharedRes->specified_framerate_num;
        AMLOG_WARN("!!!Play with specified frame rate: num %d, den %d, tick %d.\n", mpSharedRes->specified_framerate_num, mpSharedRes->specified_framerate_den, mSpecifiedFrameRateTick);
    }

    if (mbAddUdecWarpper) {
        AMLOG_INFO("num = %d, den = %d, is_mp4s_flag=%d, vid_container_width=%d, vid_container_height=%d.\n", mpStream->r_frame_rate.num, mpStream->r_frame_rate.den, mpSharedRes->is_mp4s_flag, mpSharedRes->vid_container_width, mpSharedRes->vid_container_height);
        if (!mSpecifiedFrameRateTick) {
            FillUSEQHeader(mUSEQHeader, vFormat, mpStream->r_frame_rate.num, mpStream->r_frame_rate.den, mpSharedRes->is_mp4s_flag, mpSharedRes->vid_container_width, mpSharedRes->vid_container_height);
        } else {
            AMLOG_WARN("Feed specified frame rate to USEQ header, num %d, den %d.\n", mpSharedRes->specified_framerate_num, mpSharedRes->specified_framerate_den);
            FillUSEQHeader(mUSEQHeader, vFormat, mpSharedRes->specified_framerate_num, mpSharedRes->specified_framerate_den, mpSharedRes->is_mp4s_flag, mpSharedRes->vid_container_width, mpSharedRes->vid_container_height);
        }
        InitUPESHeader(mUPESHeader, vFormat);
    }
    GenerateConfigData();

    return ME_OK;

}

AM_ERR CAmbaVideoDecoder::ReConfigDecoder(AM_INT flag)
{

    mpWorkQ->SendCmd(CMD_RECONFIG_DECODER, &flag);
    return ME_OK;

}

AM_ERR CAmbaVideoDecoder::AudioPtsLeaped()
{
    isAudPTSLeaped = 1;
    return ME_OK;

}

AM_ERR CAmbaVideoDecoder::HandleReConfigDecoder()
{

    AM_UINT vFormat = 0;

    mbAddUdecWarpper = (mpSharedRes->dspConfig.addVideoDataType == eAddVideoDataType_iOneUDEC);

    switch (mpCodec->codec_id) {
        case CODEC_ID_VC1:
            AM_INFO("-----UDEC_VC1\n----");
            vFormat = UDEC_VFormat_VC1;
            muDecType = UDEC_VC1;
            break;
        case CODEC_ID_WMV3:
            AM_INFO("-----UDEC_VC1(WMV3)\n----");
            vFormat = UDEC_VFormat_WMV3;
            muDecType = UDEC_VC1;
            break;

        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
            AM_INFO("-----UDEC_MP12\n----");
            vFormat = UDEC_VFormat_MPEG12;
            muDecType = UDEC_MP12;
            //mbAddUdecWarpper = false;
            break;

        case CODEC_ID_MPEG4:
            AM_INFO("-----UDEC_MP4H\n----");
            muDecType = UDEC_MP4H;
            vFormat = UDEC_VFormat_MPEG4;
            mbMP4WV1F = (mpCodec->codec_tag == 0x46315657); //AV_RL32("WV1F")
            break;
        case CODEC_ID_H264:
            AM_INFO("-----UDEC_H264\n----");
            muDecType = UDEC_H264;
            vFormat = UDEC_VFormat_H264;
            break;
        default:
            AM_INFO("-----not fully hardware support format %d\n----", mpCodec->codec_id);
            mbAddUdecWarpper = false;
            break;
    }

#ifdef fake_feeding
    mSpace = 16*1024*1024;
    mpStartAddr = (AM_U8*)malloc(mSpace);
    mpEndAddr = mpStartAddr + mSpace;
    mpCurrAddr = mpStartAddr;
#else
    mpStartAddr = mUdecInfo[0].bits_fifo_start;
    mpEndAddr = mpStartAddr + mUdecInfo[0].bits_fifo_size;
    mpCurrAddr = mpStartAddr;
    mSpace = mUdecInfo[0].bits_fifo_size;
#endif

#ifdef AM_DEBUG
    mdLastEndPointer = mpCurrAddr;
    mdWriteSize = 0;
#endif

    AM_INFO("BSB created, start = 0x%p, length = 0x%x\n", mpStartAddr, mSpace);

    if (mbAddUdecWarpper) {
        AM_INFO("num = %d, den = %d, is_mp4s_flag=%d, vid_container_width=%d, vid_container_height=%d.\n", mpStream->r_frame_rate.num, mpStream->r_frame_rate.den, mpSharedRes->is_mp4s_flag, mpSharedRes->vid_container_width, mpSharedRes->vid_container_height);
        FillUSEQHeader(mUSEQHeader, vFormat, mpStream->r_frame_rate.num, mpStream->r_frame_rate.den, mpSharedRes->is_mp4s_flag, mpSharedRes->vid_container_width, mpSharedRes->vid_container_height);
        InitUPESHeader(mUPESHeader, vFormat);
    }
    GenerateConfigData();

    return ME_OK;
}

AM_U8 *CAmbaVideoDecoder::CopyToBSB(AM_U8 *ptr, AM_U8 *buffer, AM_UINT size)
{
#ifdef AM_DEBUG
    mdWriteSize += size;
#endif

    AMLOG_BINARY("copy to bsb %d, %x.\n", size, *buffer);
    if (ptr + size <= mpEndAddr) {
        memcpy(ptr, buffer, size);
        return ptr + size;
    } else {

        //AM_INFO("-------wrap happened--------\n");
        int room = mpEndAddr - ptr;
        AM_U8 *ptr2;
        memcpy(ptr, buffer, room);
        ptr2 = buffer + room;
        size -= room;
        memcpy(mpStartAddr, ptr2, size);
        return mpStartAddr + size;
    }
}

AM_U8 *CAmbaVideoDecoder::FillEOS(AM_U8 *ptr)
{
	AMLOG_INFO("AmbaVideoDec fill eos.\n");
	switch (muDecType) {
	case UDEC_H264: {
			static AM_U8 eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};
			mbFillEOS= true;
			return CopyToBSB(ptr, eos, sizeof(eos));
		}
		break;

	case UDEC_VC1: {
			mbFillEOS= true;
			// WMV3
			if( mpCodec->codec_id == CODEC_ID_WMV3 ){
			    static AM_U8 eos_wmv3[4] = {0xFF, 0xFF, 0xFF, 0x0A};// special eos for wmv3
			    return CopyToBSB(ptr, eos_wmv3, sizeof(eos_wmv3));
			}
			// VC-1
			else{
			    static AM_U8 eos[4] = {0, 0, 0x01, 0xA};
			    return CopyToBSB(ptr, eos, sizeof(eos));
			}
		}
		break;

	case UDEC_MP12: {
			static AM_U8 eos[4] = {0, 0, 0x01, 0xB7};
			mbFillEOS= true;
			return CopyToBSB(ptr, eos, sizeof(eos));

		}
		break;

	case UDEC_MP4H: {
			static AM_U8 eos[4] = {0, 0, 0x01, 0xB1};
			mbFillEOS= true;
			return CopyToBSB(ptr, eos, sizeof(eos));
		}
		break;

	case UDEC_JPEG:
		return 0;

	default:
		AM_ERROR("not implemented!\n");
		return 0;


	}
}

IFilter* CAmbaVideoDecoder::Create(IEngine *pEngine)
{
	CAmbaVideoDecoder *result = new CAmbaVideoDecoder(pEngine);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

int CAmbaVideoDecoder::AcceptMedia(CMediaFormat& format)
{
    if (*format.pFormatType != GUID_Format_FFMPEG_Stream)
        return 0;

    if (*format.pMediaType == GUID_Video && *format.pSubType == GUID_AmbaVideoDecoder) {
        if (PreferWorkMode_Duplex == (format.preferWorkMode)) {
            AM_ERROR("NOT support in duplex mode.\n");
            return 0;
        }
        return 1;
    }

    return 0;
}

AM_ERR CAmbaVideoDecoder::Construct()
{
	AM_INFO("****CAmbaVideoDecoder::Construct .\n");
	AM_ERR err = inherited::Construct();
	if (err != ME_OK) {
     AM_ERROR("CAmbaVideoDecoder::Construct fail err %d .\n", err);
        return err;
    }
    DSetModuleLogConfig(LogModuleAmbaVideoDecoder);
	if ((mIavFd = open("/dev/iav", O_RDWR, 0)) < 0) {
		AM_PERROR("/dev/iav");
		return ME_ERROR;
	}
	AM_INFO("****opened dev/iav %d.\n", mIavFd);
	//LOGE("****opened dev/iav %d  .\n", mIavFd);
	if ((mpVideoInputPin = CAmbaVideoInput::Create(this)) == NULL)
		return ME_ERROR;

	if ((mpVideoOutputPin = CAmbaVideoOutput::Create(this)) == NULL)
		return ME_ERROR;

    if ((mpBufferPool = CAmbaFrameBufferPool::Create("amba frame buffer", 32)) == NULL)
        return ME_ERROR;
    mpVideoOutputPin->SetBufferPool(mpBufferPool);

    if ((mSeqConfigData = (AM_U8 *)av_malloc(DUDEC_MAX_SEQ_CONFIGDATA_LEN)) == NULL)
        return ME_NO_MEMORY;
    mSeqConfigDataSize = DUDEC_MAX_SEQ_CONFIGDATA_LEN;

    AM_ASSERT(mpSharedRes);
    AM_ASSERT(!mpSharedRes->mbIavInited);
    if (mpSharedRes) {
        mpSharedRes->mIavFd = mIavFd;
        mpSharedRes->mbIavInited = 1;
    }

#ifdef DDEBUG_CHECK_DRAM_DATA_TRASH
    mpCheckDram[0] = NULL;
    mpCheckDram[1] = NULL;
    mNumberInt = 0;
    mCheckCount = 0;
#endif

    return ME_OK;
}

CAmbaVideoDecoder::~CAmbaVideoDecoder()
{
//AM_ASSERT(0);
    AMLOG_DESTRUCTOR("***~CAmbaVideoDecoder start, mIavFd %d.\n", mIavFd);
    AM_DELETE(mpVideoInputPin);
    AMLOG_DESTRUCTOR("~CAmbaVideoDecoder AM_DELETE(mpVideoOutputPin).\n");
    AM_DELETE(mpVideoOutputPin);
#if 0
	if (::ioctl(mIavFd, IAV_IOC_UNMAP_DECODE_BSB, 0) < 0) {
		AM_PERROR("IAV_IOC_UNMAP_DECODE_BSB");
	}
#endif

    if (mIavFd >= 0) {
        if (mbEnterUDECMode) {
            //exit UDEC mode if have entered UDEC mode
            ReleaseUdec(0);
            AMLOG_INFO("IAV_IOC_ENTER_IDLE start.\n");
            ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
        }
        mbEnterUDECMode = false;
        if (mpSharedRes->mIavFd == mIavFd) {
            mpSharedRes->mIavFd = -1;
            mpSharedRes->mbIavInited = 0;
        }
        ::close(mIavFd);
        mIavFd = -1;
    }

    AMLOG_DESTRUCTOR("~CAmbaVideoDecoder AM_DELETE(mSeqConfigData).\n");
    av_free(mSeqConfigData);

#ifdef DDEBUG_CHECK_DRAM_DATA_TRASH
    if (mpCheckDram[0]) {
        free(mpCheckDram[0]);
        mpCheckDram[0] = NULL;
    }
    if (mpCheckDram[1]) {
        free(mpCheckDram[1]);
        mpCheckDram[1] = NULL;
    }
#endif

    AMLOG_DESTRUCTOR("***~CAmbaVideoDecoder end.\n");
    //AM_ASSERT(0);
}

void CAmbaVideoDecoder::Delete()
{
//AM_ASSERT(0);
#ifdef AM_DEBUG
if (mpDumpFile) {
    fclose(mpDumpFile);
    mpDumpFile = NULL;
}
#endif

    AMLOG_INFO("***CAmbaVideoDecoder::Delete start, mIavFd %d.\n", mIavFd);

    //restore vout's osd if needed
    AM_UINT i = 0;
    iav_vout_fb_sel_t fb_sel;
    for (i = 0; i < eVoutCnt; i++) {
        if (mpSharedRes->dspConfig.voutConfigs.need_restore_osd[i]) {
            AMLOG_WARN("vout(osd) %i need restore, current mVoutConfig[i].osd_disable %d.\n", i, mVoutConfig[i].osd_disable);
            memset(&fb_sel, 0, sizeof(iav_vout_fb_sel_t));
            fb_sel.vout_id = i;

            if (mpSharedRes->dspConfig.voutConfigs.voutConfig[i].osd_disable) {
                AMLOG_WARN("restore osd setting, enable osd on vout %d.\n", i);
                fb_sel.fb_id = 0;//link to fb 0, hard code here
            } else {
                AMLOG_WARN("restore osd setting, disable osd on vout %d.\n", i);
                fb_sel.fb_id = -1;
            }

            if(ioctl(mIavFd, IAV_IOC_VOUT_SELECT_FB, &fb_sel)) {
                AM_ERROR("IAV_IOC_VOUT_SELECT_FB Failed!");
                perror("IAV_IOC_VOUT_SELECT_FB");
                continue;
            }
            mpSharedRes->dspConfig.voutConfigs.voutConfig[i].osd_disable = !(mVoutConfig[i].osd_disable);
            mpSharedRes->dspConfig.voutConfigs.need_restore_osd[i] = 0;
        }
    }

    AM_DELETE(mpVideoInputPin);
    mpVideoInputPin = NULL;
    AM_DELETE(mpVideoOutputPin);
    mpVideoOutputPin = NULL;

    if (mIavFd >= 0) {
        if (mbEnterUDECMode) {
            //exit UDEC mode if have entered UDEC mode
            ReleaseUdec(0);
            AMLOG_INFO("IAV_IOC_ENTER_IDLE start.\n");
            ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
        }
        mbEnterUDECMode = false;
        if (mpSharedRes->mIavFd == mIavFd) {
            mpSharedRes->mIavFd = -1;
            mpSharedRes->mbIavInited = 0;
        }
        AMLOG_INFO("**close fd start.\n");
        mRet = ::close(mIavFd);
        //AM_PERROR("close fd");
        AMLOG_INFO("**close fd end ret %d.\n", mRet);
        mIavFd = -1;
    }

#ifdef DDEBUG_CHECK_DRAM_DATA_TRASH
    if (mpCheckDram[0]) {
        free(mpCheckDram[0]);
        mpCheckDram[0] = NULL;
    }
    if (mpCheckDram[1]) {
        free(mpCheckDram[1]);
        mpCheckDram[1] = NULL;
    }
#endif

    AMLOG_INFO("***CAmbaVideoDecoder::Delete end, call base's Delete.\n");
    //AM_ASSERT(0);
    inherited::Delete();
}

void *CAmbaVideoDecoder::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IDecoderControl)
        return (IDecoderControl*)this;
    return inherited::GetInterface(refiid);
}

bool CAmbaVideoDecoder::ReadInputData()
{
	AM_ASSERT(!mpBuffer);

	if (!mpVideoInputPin->PeekBuffer(mpBuffer)) {
		AM_ERROR("No buffer?\n");
		return false;
	}

	return true;
}

bool CAmbaVideoDecoder::ProcessCmd(CMD& cmd)
{
//    iav_udec_trickplay_t trickplay;
    AMLOG_CMD("****CAmbaVideoDecoder::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);

    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            CmdAck(ME_OK);
            AM_INFO("****CAmbaVideoDecoder::ProcessCmd, STOP cmd.\n");
            break;

        case CMD_OBUFNOTIFY:
            if (mpBufferPool->GetFreeBufferCnt() > 0) {
                if (msState == STATE_IDLE)
                    msState = STATE_HAS_OUTPUTBUFFER;
                else if(msState == STATE_HAS_INPUTDATA)
                    msState = STATE_READY;
            }
            break;

        case CMD_PAUSE:
            AM_ASSERT(!mbPaused);
            mbPaused = true;
            break;

        case CMD_RESUME:
            if(msState == STATE_PENDING){
                msState = STATE_IDLE;
                AMLOG_INFO("Decoder: CMD_RESUME STATE %d .\n",msState);
            }
            mbPaused = false;
            break;

        case CMD_FLUSH:
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
#if 0//STOP(1) should always sent to DSP, no depend on flush cmd flow
            if (mbStreamStart) {
                StopUDEC(mIavFd, 0, STOPFLAG_FLUSH);
            } else {
                AMLOG_WARN("udec not started, need not send stop?\n");
            }
#endif
            mbStreamStart = false;
            mbConfigData = false;
            mbFillEOS = false;

            mpStartAddr = mUdecInfo[0].bits_fifo_start;
            mpEndAddr = mpStartAddr + mUdecInfo[0].bits_fifo_size;
            mpCurrAddr = mpStartAddr;
            mSpace = mUdecInfo[0].bits_fifo_size;
#ifdef AM_DEBUG
            mdLastEndPointer = mpCurrAddr;
            mdWriteSize = 0;
#endif
            last_dts=AV_NOPTS_VALUE;//for bug#1883,#1915, after seek flush flow, reset DTS leap catch
            isAudPTSLeaped=0;//clear flag
            msState = STATE_PENDING;
            CmdAck(ME_OK);
            break;

        case CMD_AVSYNC:
            CmdAck(ME_OK);
            break;

        case CMD_BEGIN_PLAYBACK:
            AM_ASSERT(msState == STATE_PENDING);
            AM_ASSERT(!mpBuffer);
            mbStreamStart = false;
            msState = STATE_IDLE;
            mbPaused = false;
            break;

        case CMD_RECONFIG_DECODER:
            AM_ASSERT(msState == STATE_PENDING);
            AM_ASSERT(!mpBuffer);

            StopUDEC(mIavFd, 0, STOPFLAG_FLUSH);
            StopUDEC(mIavFd, 0, STOPFLAG_CLEAR);

            if(*(AM_INT*)cmd.pExtra == 0){
                HandleReConfigDecoder();//todo

                /*iav_fbp_config_t config;
                memset(&config,0,sizeof(config));
                config.decoder_id = mUdecInfo[0].udec_id;
                config.chroma_format = 1; //420
                config.tiled_mode = 0;
                config.lu_width = mpCodec->width;
                config.lu_height = mpCodec->height;
                if (::ioctl(mIavFd, IAV_IOC_UDEC_UPDATE_FB_POOL_CONFIG, &config) < 0) {
                    perror("IAV_IOC_UDEC_UPDATE_FB_POOL_CONFIG");
                    AM_ERROR("IAV_IOC_UDEC_UPDATE_FB_POOL_CONFIG return error\n");
                    //return;
                }*/
            }else{
               ConfigDecoder();
            }
            CmdAck(ME_OK);
            break;

        case CMD_SOURCE_FILTER_BLOCKED:
            //if demuxer blocked during prefetch, abort prefetching to avoid decoder blocked
            if (mInPrefetching && (mpSharedRes->dspConfig.preset_prefetch_count > mCurrentPrefetchCount)) {
                iav_udec_decode_t dec;
                AM_UINT udec_state;
                AM_UINT vout_state;
                AM_UINT error_code;
                AMLOG_WARN("CMD_SOURCE_FILTER_BLOCKED, prebuffering interrupted, decode prefetched %u video frames now.\n", mCurrentPrefetchCount);
                mInPrefetching = 0;
                memset(&dec, 0, sizeof(dec));
                dec.udec_type = muDecType;
                dec.decoder_id = mUdecInfo[0].udec_id;
                dec.u.fifo.start_addr = mpPrefetching;
                dec.u.fifo.end_addr = mpCurrAddr;
                dec.num_pics = mCurrentPrefetchCount;
                if ((mRet = ::ioctl(mIavFd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
                    perror("IAV_IOC_UDEC_DECODE");
                    AMLOG_ERROR("!!!!!IAV_IOC_UDEC_DECODE error, ret %d.\n", mRet);
                    if (mRet == (-EPERM)) {
                        if (GetUdecState (mIavFd, &udec_state, &vout_state, &error_code)) {
                            pthread_mutex_lock(&mpSharedRes->mMutex);
                            if (mpSharedRes->udec_state != IAV_UDEC_STATE_ERROR && udec_state == IAV_UDEC_STATE_ERROR) {
                                mpSharedRes->udec_state = udec_state;
                                mpSharedRes->vout_state = vout_state;
                                mpSharedRes->error_code = error_code;
                                AMLOG_ERROR("IAV_IOC_UDEC_DECODE(1) error, post msg, udec state %d, vout state %d, error_code 0x%x.\n", udec_state, vout_state, error_code);
                                PostEngineErrorMsg(ME_UDEC_ERROR);
                            } else {
                                AMLOG_ERROR("IAV_IOC_UDEC_DECODE(1) error, (udec error has been reported, just print error code), udec state %d, vout state %d, error_code 0x%x, mRet %d.\n", udec_state, vout_state, error_code, mRet);
                            }
                            pthread_mutex_unlock(&mpSharedRes->mMutex);
                        }
                        return false;
                    } else {
                        //AM_ERROR("----IAV_IOC_UDEC_DECODE----ME_ERROR");
                        return false;
                    }
                }

                if (!mbStreamStart) {
                    mbStreamStart = true;
                    pthread_mutex_lock(&mpSharedRes->mMutex);
                    GetUdecState(mIavFd, &mpSharedRes->udec_state, &mpSharedRes->vout_state, &mpSharedRes->error_code);
                    pthread_mutex_unlock(&mpSharedRes->mMutex);
                    AM_ASSERT(IAV_UDEC_STATE_RUN == mpSharedRes->udec_state);
                    PostEngineMsg(IEngine::MSG_NOTIFY_UDEC_IS_RUNNING);
                }
            }
            break;

            default:
                    AM_ERROR("wrong cmd %d.\n",cmd.code);
	}
	return false;
}



void CAmbaVideoDecoder::OnRun()
{
	CMD cmd;
	CQueue::QType type;
	CQueue::WaitResult result;
//    AM_INT ret = 0;
	CmdAck(ME_OK);

	mbRun = true;
	mbFillEOS = false;

	mpBufferPool = mpVideoOutputPin->mpBufferPool;
	msState = STATE_IDLE;

//    if (mLogOutput&LogDisableNewPath)
//    {
//        AMLOG_INFO("***disable wrapper.\n");
//        mbAddUdecWarpper = false;
//    }

	while(mbRun) {
		AMLOG_STATE("Amba Decoder: start switch, msState=%d, %d input data. \n", msState, mpVideoInputPin->mpBufferQ->GetDataCnt());

		switch (msState) {

		case STATE_IDLE:

                    if(mbPaused) {
                        msState = STATE_PENDING;
                        AMLOG_INFO("Decoder: Enter STATE_PENDING .\n");
                        break;
                    }

			if(mpBufferPool->GetFreeBufferCnt() > 0) {
				msState = STATE_HAS_OUTPUTBUFFER;
			} else {
				type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
				if(type == CQueue::Q_MSG) {
					ProcessCmd(cmd);
				} else {
					AM_ASSERT((CQueueInputPin*)result.pOwner == (CQueueInputPin*)mpVideoInputPin);
					if (ReadInputData()) {
						msState = STATE_HAS_INPUTDATA;
					}
				}
			}

			break;

		case STATE_HAS_OUTPUTBUFFER:
			AM_ASSERT(mpBufferPool->GetFreeBufferCnt() > 0);
			type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
			if(type == CQueue::Q_MSG) {
				ProcessCmd(cmd);
			} else {
				AM_ASSERT((CQueueInputPin*)result.pOwner == (CQueueInputPin*)mpVideoInputPin);
				if (ReadInputData()) {
					msState = STATE_READY;
				}
			}
			break;

		case STATE_PENDING:
			mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
			ProcessCmd(cmd);

			break;

		case STATE_HAS_INPUTDATA:

			AM_ASSERT(mpBuffer);

			if(mpBufferPool->GetFreeBufferCnt() > 0) {
				msState = STATE_READY;
			}
			else {
				mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
				ProcessCmd(cmd);
			}
			break;

        case STATE_READY:
            AM_ASSERT(mpBuffer);
            if (mpBuffer->GetType() == CBuffer::DATA) {
                ProcessBuffer(mpBuffer);
            } else if (mpBuffer->GetType() == CBuffer::EOS){
                AMLOG_INFO("!!!!Notice, bit-stream comes to end, should be Legal bit-stream.\n");

                if (!mbESMode) {
                    //fill eos
                    AM_U8* pFrameStart = mpCurrAddr;
                    mpCurrAddr = FillEOS(mpCurrAddr);

#ifdef AM_DEBUG
                    AM_ASSERT(mdLastEndPointer == pFrameStart);
                    mdLastEndPointer = mpCurrAddr;
#endif

                    DecodeBuffer(mpBuffer, 1/*0*/, pFrameStart, mpCurrAddr);
                    mbESMode = 0;
                    mpBuffer = NULL;
                }
                msState = STATE_PENDING;
                AMLOG_INFO("Sending EOS...\n");
                CBuffer* peos = NULL;
                if (!mpVideoOutputPin->AllocBuffer(peos)) {
                    AM_ERROR("amba_video_decoder AllocBuffer Fail.\n");
                    break;
                }
                peos->SetType(CBuffer::EOS);
                peos->SetDataSize(0);
                peos->SetDataPtr(NULL);
                mpVideoOutputPin->SendBuffer(peos);
            }else if (mpBuffer->GetType() == CBuffer::TEST_ES) {
                AMLOG_INFO("!!!!Notice, try send es mode.\n");
                ProcessTestESBuffer(mpBuffer);
                mpBuffer->Release();
                mpBuffer = NULL;
                msState = STATE_IDLE;
                mbESMode = 1;
            }
            break;

        default:
            AM_ERROR(" %d",(AM_UINT)msState);
            break;
        }
    }
}

AM_ERR CAmbaVideoDecoder::Stop()
{
    AMLOG_INFO("[flow cmd]: Call IAV_IOC_UDEC_STOP\n");
    if ((mRet = ::ioctl(mIavFd, IAV_IOC_UDEC_STOP, 0)) < 0) {
        AM_PERROR("IAV_IOC_STOP_DECODE");
        //return ME_BAD_STATE;
    }
    AMLOG_INFO("[flow cmd]: Call IAV_IOC_UDEC_STOP done\n");

    inherited::Stop();

    AMLOG_INFO("CAmbaVideoDecoder after inherited::Stop()\n");

    mpCurrAddr = mpStartAddr;

#ifdef AM_DEBUG
    mdLastEndPointer = mpCurrAddr;
#endif

    return ME_OK;
}

void CAmbaVideoDecoder::GetInfo(INFO& info)
{
    info.nInput = 1;
    info.nOutput = 1;
    info.mPriority = 0;
    info.mFlags = 0;
    info.mIndex = mDspIndex;
    info.pName = "AmbaVDec";
}

IPin* CAmbaVideoDecoder::GetInputPin(AM_UINT index)
{
	if (index == 0)
		return mpVideoInputPin;
	return NULL;
}


IPin* CAmbaVideoDecoder::GetOutputPin(AM_UINT index)
{
	if (index == 0)
		return mpVideoOutputPin;
	return NULL;
}


AM_ERR CAmbaVideoDecoder::SetInputFormat(CMediaFormat *pFormat)
{
	AM_ERR err;
	mpStream = (AVStream*)pFormat->format;
	mpCodec = (AVCodecContext*)mpStream->codec;


	err = ConfigDecoder();
	if (ME_OK != err) {
		AM_ERROR("ConfigDecoder fail, return %d.\n", err);
		return err;
	}
	err = mpVideoOutputPin->SetOutputFormat();
	if (err != ME_OK)
		return err;

	return ME_OK;
}
/*
AM_ERR CAmbaVideoDecoder::MapDSP()
{
    iav_mmap_info_t info;
    if (::ioctl(mIavFd, IAV_IOC_MAP_DSP, &info) < 0) {
        AM_PERROR("IAV_IOC_MAP_DSP");
        return ME_ERROR;
    }
    AM_INFO("IAV_IOC_MAP_DSP done!!\n");
    return ME_OK;
}

AM_ERR CAmbaVideoDecoder::UnMapDSP()
{
    if (::ioctl(mIavFd, IAV_IOC_UNMAP_DSP) < 0) {
        AM_PERROR("IAV_IOC_UNMAP_DSP");
        return ME_ERROR;
    }
    AM_INFO("IAV_IOC_UNMAP_DSP done!!\n");
    return ME_OK;
}

AM_INT CAmbaVideoDecoder::RenderBuffer(AM_INT IsDecoderEOS)
{
#ifdef fake_feeding
    return 0;
#endif
	CBuffer *pBuffer;
    CVideoBuffer* pVideoBuffer;
    //iav_decoded_frame_t deprecated
	iav_decoded_frame_t frame;
	frame.flags = 0;
	frame.flags |= IAV_FRAME_SYNC_VOUT;
	if(1 == mpSharedRes->ppmode){
		frame.flags |= IAV_FRAME_NEED_ADDR;
	}
	frame.decoder_id = mUdecInfo[0].udec_id;

	if (!mpVideoOutputPin->AllocBuffer(pBuffer)) {
		AM_ERROR("demuxer AllocBuffer Fail, exit \n");
	}

	//eos
	if (IsDecoderEOS) {
		pBuffer->SetType(CBuffer::EOS);
	}
	else {
		if (::ioctl(mIavFd, IAV_IOC_GET_DECODED_FRAME, &frame) < 0) {
			AM_PERROR("IAV_IOC_GET_DECODE_FRAME");
			//todo:release buffer
			return -1;
		}

		pBuffer->SetType(CBuffer::DATA);
		//::memcpy((AM_U8*)pBuffer + sizeof(CVideoBuffer), &frame, sizeof(frame));
            pVideoBuffer = (CVideoBuffer*)pBuffer;
            pVideoBuffer->buffer_id = frame.fb_id;
            pVideoBuffer->real_buffer_id = frame.real_fb_id;
	}

	pBuffer->SetDataSize(0);
	pBuffer->SetDataPtr(NULL);
	pBuffer->SetPTS((AM_U64)(frame.pts) | (((AM_U64)(frame.pts_high))<<32));
	mpVideoOutputPin->SendBuffer(pBuffer);


	if (frame.eos_flag) {
		return 1;
	}
	return 0;
}

AM_INT CAmbaVideoDecoder::SendBufferMode1(iav_decoded_frame_t& frame)
{
    AM_ASSERT(mUdecModeConfig.postp_mode == 1);

    CBuffer *pBuffer;
    frame.flags = 0;
    frame.flags |= IAV_FRAME_SYNC_VOUT;
    frame.decoder_id = mUdecInfo[0].udec_id;

    if (!mpVideoOutputPin->AllocBuffer(pBuffer)) {
        AM_ERROR("CAmbaVideoDecoder Alloc output buffer Fail!\n");
        return 1;
    }

    pBuffer->SetType(CBuffer::DATA);
    ::memcpy((AM_U8*)pBuffer + sizeof(CBuffer), &frame, sizeof(frame));

    pBuffer->SetDataSize(0);
    pBuffer->SetDataPtr(NULL);
    mpVideoOutputPin->SendBuffer(pBuffer);

    return 0;
}

AM_ERR CAmbaVideoDecoder::AllocBSBMode1(AM_UINT size)
{
    AM_UINT udec_state;
    AM_UINT vout_state;
    AM_UINT error_code;

#ifdef fake_feeding
    return ME_OK;
#endif

    iav_wait_decoder_t wait;
    wait.emptiness.room = size + mSeqConfigDataLen + DUDEC_PES_HEADER_LENGTH + 128;//for safe
    wait.emptiness.start_addr = mpCurrAddr;

    wait.flags = (IAV_WAIT_BITS_FIFO | IAV_WAIT_OUTPIC);
    wait.decoder_id = mUdecInfo[0].udec_id;
    //AM_INFO("mIavFd %d, wait.decoder_id %d.\n", mIavFd, mUdecInfo[0].udec_id);
    mdRequestSize = wait.emptiness.room;

    if ((mRet = ::ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait)) < 0) {
        perror("IAV_IOC_WAIT_DECODER");
        AMLOG_ERROR("!!!!!IAV_IOC_WAIT_DECODER error, ret %d.\n", mRet);
        if (mRet == (-EPERM)) {
            if (GetUdecState (mIavFd, &udec_state, &vout_state, &error_code)) {
                pthread_mutex_lock(&mpSharedRes->mMutex);
                if (mpSharedRes->udec_state != IAV_UDEC_STATE_ERROR && udec_state == IAV_UDEC_STATE_ERROR) {
                    mpSharedRes->udec_state = udec_state;
                    mpSharedRes->vout_state = vout_state;
                    mpSharedRes->error_code = error_code;
                    AMLOG_ERROR("IAV_IOC_WAIT_DECODER(1) error, post msg, udec state %d, vout state %d, error_code 0x%x.\n", udec_state, vout_state, error_code);
                    PostEngineErrorMsg(ME_UDEC_ERROR);
                } else {
                    AMLOG_ERROR("IAV_IOC_WAIT_DECODER(1) error, (udec error has been reported, just print error code), udec state %d, vout state %d, error_code 0x%x, mRet %d.\n", udec_state, vout_state, error_code, mRet);
                }
                pthread_mutex_unlock(&mpSharedRes->mMutex);
            }
            return ME_ERROR;
        } else if (mRet != EAGAIN) {
            AM_PERROR("---IAV_IOC_WAIT_DECODER!---\n");
            AM_INFO("---IAV_IOC_WAIT_DECODER!---\n");
            return ME_ERROR;
        }
    }

    if (wait.flags == IAV_WAIT_BITS_FIFO) {
        // bsb has room, continue
        //AM_INFO("--------bsb has room\n");
        return ME_OK;
    }
    if (wait.flags == IAV_WAIT_OUTPIC) {
        // frame(s) decoded, render to vout
        //AM_INFO("--------wait.flags == IAV_WAIT_OUTPIC\n");
        if (RenderBuffer(0) < 0) {
            return ME_ERROR;
        }
        return ME_BUSY;
    }

    return ME_OK;

}
*/
AM_ERR CAmbaVideoDecoder::AllocBSBMode(AM_UINT size)
{
    AM_UINT udec_state;
    AM_UINT vout_state;
    AM_UINT error_code;

#ifdef fake_feeding
    return ME_OK;
#endif

    iav_wait_decoder_t wait;
    wait.emptiness.room = size + mSeqConfigDataLen + DUDEC_PES_HEADER_LENGTH + 256;//for safe
    wait.emptiness.start_addr = mpCurrAddr;

    wait.flags = IAV_WAIT_BITS_FIFO;
    wait.decoder_id = mUdecInfo[0].udec_id;
    //AM_INFO("mIavFd %d, wait.decoder_id %d.\n", mIavFd, mUdecInfo[0].udec_id);

#ifdef AM_DEBUG
    mdRequestSize = wait.emptiness.room;
    AMLOG_DEBUG("checking mdRequestSize %d, wait.emptiness.room %d, mpCurrAddr %p.\n", mdRequestSize, wait.emptiness.room, mpCurrAddr);
    //GetUdecState (mIavFd, &udec_state, &vout_state, &error_code);
    //AMLOG_INFO("Before IAV_IOC_WAIT_DECODER udec_state %d.\n", udec_state);
#endif

    if ((mRet = ::ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait)) < 0) {
        perror("IAV_IOC_WAIT_DECODER");
        AMLOG_ERROR("!!!!!IAV_IOC_WAIT_DECODER error, ret %d.\n", mRet);
        if (mRet == (-EPERM)) {
            if (GetUdecState (mIavFd, &udec_state, &vout_state, &error_code)) {
                pthread_mutex_lock(&mpSharedRes->mMutex);
                if (mpSharedRes->udec_state != IAV_UDEC_STATE_ERROR && udec_state == IAV_UDEC_STATE_ERROR) {
                    mpSharedRes->udec_state = udec_state;
                    mpSharedRes->vout_state = vout_state;
                    mpSharedRes->error_code = error_code;
                    AMLOG_ERROR("IAV_IOC_WAIT_DECODER(2) error, post msg, udec state %d, vout state %d, error_code 0x%x.\n", udec_state, vout_state, error_code);
                    PostEngineErrorMsg(ME_UDEC_ERROR);
                } else {
                    AMLOG_ERROR("IAV_IOC_WAIT_DECODER(2) error, (udec error has been reported, just print error code), udec state %d, vout state %d, error_code 0x%x, mRet %d.\n", udec_state, vout_state, error_code, mRet);
                }
                pthread_mutex_unlock(&mpSharedRes->mMutex);
            }
            return ME_ERROR;
        } else if (errno != EAGAIN) {
            AM_PERROR("---IAV_IOC_WAIT_DECODER!---\n");
            AM_INFO("---IAV_IOC_WAIT_DECODER!---\n");
            return ME_ERROR;
        }
    }

#ifdef AM_DEBUG
    AMLOG_DEBUG("request done.\n");
#endif

    AM_ASSERT(wait.flags == IAV_WAIT_BITS_FIFO);

    return ME_OK;
}

AM_UINT CAmbaVideoDecoder::_NextESDatapacket(AM_U8 * start, AM_U8 * end, AM_UINT * totFrames)
{
    AM_UINT totsize = 0;
    AM_UINT size = 0;
    AM_UINT framesCnt = 0;
    AM_U8* pCur = start;

    while (pCur < end) {
        AMLOG_BINARY("while loop 1 pCur %p, end %p.\n", pCur, end);
        size = AM_GetNextStartCode(pCur, end, muDecType);
        totsize += size;
        pCur = start + totsize;
        framesCnt ++;
        AMLOG_BINARY("while loop 2 pCur framesCnt %d, totsize %d, size %d.\n", framesCnt, totsize, size);
        if ( totsize > (2*1024*1024) || framesCnt > 0) {
            break;
        }
    }
    *totFrames = framesCnt;
    return totsize;
}

AM_ERR CAmbaVideoDecoder::ProcessTestESBuffer(CBuffer *pBuffer)
{
    AM_ERR err;
    AM_U8 *pFrameStart;
    iav_udec_decode_t dec;
//    iav_udec_status_t status;
//    AM_INT i = 0;
    AM_UINT sendFrames = 0;
//    AM_UINT totSendSize = 0;

    //read es file
    AM_U8* pEs = NULL, *PEs_end;
    AM_U8* pCur = NULL;
    AM_UINT size = 0;
    AM_UINT totsize = 0;
    AM_UINT bytes_left_in_file = 0;
    AM_UINT sendsize = 0;

    AM_UINT mem_size;
    FILE* pFile = NULL;

    snprintf(mDumpFilename, DAMF_MAX_FILENAME_LEN, "%s/es.data", AM_GetPath(AM_PathDump));
    pFile = fopen(mDumpFilename, "rb");

    AMLOG_INFO("****start send es data directly.\n");

    if (!pFile) {
        AMLOG_ERROR("cannot open input es file(%s).\n", mDumpFilename);
        return ME_ERROR;
    }

    fseek(pFile, 0L, SEEK_END);
    totsize = ftell(pFile);
    AMLOG_INFO("file total size %d.\n", totsize);
    bytes_left_in_file = totsize;

    if (totsize > 16*1024*1024) {
        mem_size = 16*1024*1024;
    } else {
        mem_size = totsize;
    }

    fseek(pFile, 0L, SEEK_SET);
    pEs = (AM_U8*)malloc(mem_size);

    if (!pEs) {
        AMLOG_ERROR("cannot alloc buffer.\n");
        fclose(pFile);
        return ME_ERROR;
    }

    size = mem_size;
    fread(pEs, 1, size, pFile);
    bytes_left_in_file -= size;

    //send data
    PEs_end = pEs + size;
    pCur = pEs;

    while (1) {

        while (size > (1024*1024)) {
            AMLOG_BINARY("**send start size %d.\n", size);
            sendFrames = 0;
    //        if (size < sendsize) {
    //            sendsize = size;
    //        }
            sendsize = _NextESDatapacket(pCur, PEs_end, &sendFrames);
            AMLOG_BINARY(" total size %d, total frame %d.\n", sendsize, sendFrames);
    /*
            if (mUdecModeConfig.postp_mode == 2) {
                err = AllocBSBMode2(sendsize);
            } else {
                AM_ASSERT(mUdecModeConfig.postp_mode == 1);
                err = AllocBSBMode1(sendsize);
            }
    */
            err = AllocBSBMode(sendsize);
    AMLOG_DEBUG("AllocBSBMode ret=%d.\n", err);

            pFrameStart = mpCurrAddr;
            mpCurrAddr = CopyToBSB(mpCurrAddr, pCur, sendsize);

            memset(&dec, 0, sizeof(dec));
            dec.udec_type = muDecType;
            dec.decoder_id = mUdecInfo[0].udec_id;
            dec.u.fifo.start_addr = pFrameStart;
            dec.u.fifo.end_addr = mpCurrAddr;
            dec.num_pics = sendFrames;

            AMLOG_DEBUG("decoding size %d.\n", sendsize);
            AMLOG_DEBUG("pFrameStart %p, mpCurrAddr %p, diff %p.\n", pFrameStart, mpCurrAddr, (AM_U8*)(mpCurrAddr + mSpace - pFrameStart));
            if (::ioctl(mIavFd, IAV_IOC_UDEC_DECODE, &dec) < 0) {
                AM_ERROR("----IAV_IOC_UDEC_DECODE----ME_ERROR");
                free(pEs);
                fclose(pFile);
                return ME_ERROR;
            }

            pCur += sendsize;
            size -= sendsize;

            //AM_INFO("-------DecodeBuffer-----pStart:0x%p,pEnd:0x%p\n",pStart,pEnd);
            AMLOG_BINARY("**send end size %d.\n", size);
        }

        //copy left bytes
        if (size) {
            memcpy(pEs, pCur, size);
        }
        pCur = pEs + size;

        if (bytes_left_in_file) {

            if ((mem_size - size) >= bytes_left_in_file) {

                fread(pCur, 1, bytes_left_in_file, pFile);
                bytes_left_in_file = 0;
                size += bytes_left_in_file;
                pCur = pEs;

                if (size <= (1024*1024)) {
                    //last
                    err = AllocBSBMode(size);
                    mpCurrAddr = CopyToBSB(mpCurrAddr, pEs, size);
                    break;//done
                }
            } else {
                fread(pCur, 1, mem_size - size, pFile);
                bytes_left_in_file -= (mem_size - size);
                size = mem_size;
                pCur = pEs;
            }
        } else {
            //last
            err = AllocBSBMode(size);
            mpCurrAddr = CopyToBSB(mpCurrAddr, pEs, size);
            break;//done
        }

    }

    AMLOG_INFO("****send es data done.\n");
    free(pEs);
    fclose(pFile);
    return ME_OK;
}

//for wv1f,  hw mp4 check data for WV1F file, if data start with 0x575630F0, replace this 4 bytes with vop start code 0x000001B6
inline AM_U8* _Check_WV1F4CC(AM_U8* packet)
{
    AM_U8* ptr = packet;
    if(NULL==ptr)//check for safety
        return NULL;

    if (*ptr == 0x57)
    {
        if (*(ptr + 1)== 0x56)
        {
            if (*(ptr + 2)== 0x30)
            {
                if(*(ptr + 3)== 0xF0)
                {
                    return ptr;
                }
            }
        }
    }

    return NULL;
}

inline AM_U8* _Find_VOLHead(AM_U8* packet, AM_INT size)
{
    AM_U8* ptr = packet;
    if(NULL==ptr)//check for safety
        return NULL;

    while (ptr < packet + size - 4) {
        if (*ptr == 0x00)
        {
            if (*(ptr + 1)== 0x00)
            {
                if (*(ptr + 2)== 0x01)
                {
                    if(*(ptr + 3)== 0x20)
                    {
                        return ptr;
                    }
                }
            }
        }
        ++ptr;
    }
    return NULL;
}

inline AM_U8* _Find_MPEG12_start_code(AM_U8* packet, AM_INT size, AM_U8 start_code_value)
{
    AM_U8* ptr = packet;
    if(NULL==ptr)//check for safety
        return NULL;

    while (ptr < packet + size - 4) {
        if (*ptr == 0x00)
        {
            if (*(ptr + 1)== 0x00)
            {
                if (*(ptr + 2)== 0x01)
                {
                    if(*(ptr + 3)== start_code_value)
                    {
                        return ptr;
                    }
                }
            }
        }
        ++ptr;
    }
    return NULL;
}

bool CAmbaVideoDecoder::Find_H264_SEI(AM_U8* data_base, AM_INT data_size)
{
    AM_U8* ptr = data_base;
    if(NULL==ptr)//check for safety
        return NULL;

    while (ptr < data_base + data_size - 4) {
        if (*ptr == 0x00)
        {
            if (*(ptr + 1)== 0x00)
            {
                if (*(ptr + 2)== 0x00)
                {
                    if (*(ptr + 3)== 0x01)
                    {
                        if (((*(ptr + 4))&0x1F)== 0x06)
                        {
                            return true;
                        }
                    }
                }
            }
        }
        ++ptr;
    }
    return false;
}

AM_ERR CAmbaVideoDecoder::ProcessBuffer(CBuffer *pBuffer)
{
    AM_ERR err;
    AM_U8 *pFrameStart;
    AM_ASSERT(!pBuffer->IsEOS());
//    iav_udec_status_t status;

    if(mpCurrAddr==mpEndAddr)//for bits fifo wrap case
        mpCurrAddr=mpStartAddr;

    AM_U8 *pRecoveryAddr = mpCurrAddr;  // added for recovery when dropping packets

    AVPacket *mpPacket = (AVPacket*)((AM_U8*)pBuffer + sizeof(CBuffer));
    if (mpPacket->size <= 0) {
        AM_ERROR(" data size <= 0 (%d), %p?\n", mpPacket->size, mpPacket);
        av_free_packet(mpPacket);
        mpPacket = NULL;
        mpBuffer->Release();
        mpBuffer = NULL;
        msState = STATE_IDLE;
        return ME_OK;
    }

    if (mbAddUdecWarpper && (mpCodec->codec_id == CODEC_ID_MPEG4) && mbNeedFindVOLHead)//add for bug396, wrapper case
    {
        AM_U8* StartCode_ptr = _Find_VOLHead(mpPacket->data, mpPacket->size);
        if (StartCode_ptr == NULL){
            AMLOG_DEBUG("ProcessBuffer, mpeg4 no vol_header(0x00000120) found in packet, skip it.\n");
            mpBuffer->Release();
            mpBuffer = NULL;
            msState = STATE_IDLE;
            return ME_OK;
        }
        mbNeedFindVOLHead = false;
    }
/*
    if (mUdecModeConfig.postp_mode == 2) {
        err = AllocBSBMode2(mpPacket->size);
    } else {
        AM_ASSERT(mUdecModeConfig.postp_mode == 1);
        err = AllocBSBMode1(mpPacket->size);
    }
*/

#ifdef DDEBUG_CHECK_DRAM_DATA_TRASH
    AMLOG_DEBUG("store(1), count %d.\n", mCheckCount);
    //store last
    storeDramData(1);
#endif

    err = AllocBSBMode(mpPacket->size);

    if (err == ME_ERROR) {
        mpBuffer->Release();
        mpBuffer = NULL;
        msState = STATE_PENDING;
        AMLOG_ERROR("ambadec goto pending 1, %d.\n", msState);
/*  } else if (err == ME_BUSY) {
        AM_ASSERT(mUdecModeConfig.postp_mode == 1);
        msState = STATE_HAS_INPUTDATA;
*/
    } else {
        pFrameStart = mpCurrAddr;
        if (!mbAddUdecWarpper) {
            FeedConfigData(mpPacket);
        } else if (mpCodec->codec_id != CODEC_ID_MPEG1VIDEO&&mpCodec->codec_id != CODEC_ID_MPEG2VIDEO) {//for MPEG12, Amba PES header shall be ahead of its picture start code, so we go another way BELOW to handle it, no use FeedConfigDataWithUDECWrapper,2012.01.05, roy mdf
            FeedConfigDataWithUDECWrapper(mpPacket);
        }

    if (!mbAddUdecWarpper && (mpCodec->codec_id == CODEC_ID_MPEG4) && mbNeedFindVOLHead)//add for bug396, NO wrapper case
    {
        AM_U8* StartCode_ptr = _Find_VOLHead(mpPacket->data, mpPacket->size);
        if (StartCode_ptr == NULL){
            AMLOG_DEBUG("ProcessBuffer, mpeg4 no vol_header(0x00000120) found in packet, skip it.\n");
            mpBuffer->Release();
            mpBuffer = NULL;
            msState = STATE_IDLE;
            return ME_OK;
        }
        mbNeedFindVOLHead = false;
    }

#ifdef AM_DEBUG
        if (mpPacket->size > 15) {
            AMLOG_BINARY("Print first 16 bytes:\n");
            PrintBitstremBuffer(mpPacket->data, 16);
        } else {
            AMLOG_BINARY("Print first %d bytes:\n", mpPacket->size);
            PrintBitstremBuffer(mpPacket->data, mpPacket->size);
        }
#endif

        if (mpCodec->codec_id == CODEC_ID_H264) {
            AM_U8 startcode[4] = {0, 0, 0, 0x01 };
            AM_U8 startcodeEx[3] = {0, 0, 0x01};
#if TARGET_USE_AMBARELLA_S2_DSP
            //s2 playback, h264 need delimiter
            AM_U8 _h264_delimiter[6] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x30};
#endif
            AM_U8 *ptr = mpPacket->data;
            AM_INT curPos = 0;
            AM_INT len = 0;

            if (mH264DataFmt == H264_FMT_AVCC) {
                //AM_ASSERT((mH264AVCCNaluLen == 2) || (mH264AVCCNaluLen == 3) || (mH264AVCCNaluLen == 4));

                while (1) {
                    len = 0;
                    for (int index = 0; index < mH264AVCCNaluLen; index++) {
                        len = len<<8 | ptr[index];
                    }

                    ptr += mH264AVCCNaluLen;
                    curPos += mH264AVCCNaluLen;

                    if (len <= 0 || (curPos + len > mpPacket->size)) {
                        AMLOG_ERROR("CAmbaVideoDecoder::ProcessBuffer error: pkt_size:%d len:%d mH264AVCCNaluLen:%d curPos:%d\n",
                            mpPacket->size, len, mH264AVCCNaluLen, curPos);
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        msState = STATE_IDLE;
                        mpCurrAddr= pRecoveryAddr;
                        PostEngineErrorMsg(ME_VIDEO_DATA_ERROR);
                        return ME_OK;
                    }

                    // check whether there's 00 00 00 01 or 00 00 01 in data
                    // if exist, skip it
                    //AM_DEBUG("~~~~~1: len:%d %x %x %x %x %x\n", len, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4]);
                    AM_INT end = curPos + len;
                    while (curPos + 4 < end) {
                        if (memcmp(ptr, startcode, sizeof(startcode)) == 0) {
                            ptr += 4;
                            curPos += 4;
                            continue;
                        } else if(memcmp(ptr, startcodeEx, sizeof(startcodeEx)) == 0) {
                            ptr += 3;
                            curPos += 3;
                            continue;
                        } else {
                            break;
                        }
                    }

                    AM_ASSERT(curPos < end);
                    len = end - curPos;
                    mpCurrAddr = CopyToBSB(mpCurrAddr, ptr, len);
                    ptr += len;
                    curPos += len;
                    //AM_DEBUG("~~~~~2: len:%d\n", len);

                    if (curPos + mH264AVCCNaluLen >= mpPacket->size) {
                        break;
                    } else {
                        mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
                    }
                }
            } else if (mH264DataFmt == H264_FMT_ANNEXB) {
                AM_INT firstStatcodeOffset = 0;
                while (curPos + 4 < mpPacket->size) {
                    if (memcmp(ptr, startcode, sizeof(startcode)) == 0) {
                        firstStatcodeOffset += 4;
                        ptr += 4;
                        curPos += 4;
                        continue;
                    } else if (memcmp(ptr, startcodeEx, sizeof(startcodeEx)) == 0) {
                        firstStatcodeOffset += 3;
                        ptr += 3;
                        curPos += 3;
                        continue;
                    } else {
                        if (firstStatcodeOffset > 0) {
                            break;
                        } else {
                            ptr++;
                            curPos++;
                        }
                    }
                }

                // patch:
                // if startcode can't be found, fill unit delimiter as placeholder,
                // which doesn't has any side effect
                if (firstStatcodeOffset > 0) {
                    mpCurrAddr = CopyToBSB(mpCurrAddr,ptr, mpPacket->size - curPos);
                } else {
                    AM_U8 unitDelimiter[] = { 0x09, 0x30 };
                    mpCurrAddr = CopyToBSB(mpCurrAddr, unitDelimiter, sizeof(unitDelimiter));
                }
            }else {
                AM_ASSERT(H264_FMT_INVALID != mH264DataFmt);
            }
#if TARGET_USE_AMBARELLA_S2_DSP
            //s2 playback, h264 need delimiter
            mpCurrAddr = CopyToBSB(mpCurrAddr, _h264_delimiter, sizeof(_h264_delimiter));
#endif
        } else if (mpCodec->codec_id == CODEC_ID_MPEG4) {
            AM_U8* StartCode_ptr=NULL;
            AM_INT mdfsize=0;
            //for wv1f
            if (mbMP4WV1F)
            {
                AM_U8* WV1F4CC_ptr = _Check_WV1F4CC(mpPacket->data);
                if(!WV1F4CC_ptr)
                {
                    StartCode_ptr = Get_StartCode_info(mpPacket->data, mpPacket->size, &mdfsize);
                    if (StartCode_ptr == NULL){
                        AMLOG_DEBUG("mpeg4: no start_code(0x000001) found in packet, skip it.\n");
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        msState = STATE_IDLE;
                        mpCurrAddr= pRecoveryAddr;
                        //PostEngineErrorMsg(ME_VIDEO_DATA_ERROR);
                        return ME_OK;
                    }
                    mpCurrAddr = CopyToBSB(mpCurrAddr,StartCode_ptr, mdfsize);
                }
                else
                {
                    AM_U8 vop_startcode[4] = {0, 0, 0x01, 0xB6 };
                    mpCurrAddr = CopyToBSB(mpCurrAddr, vop_startcode, sizeof(vop_startcode));
                    mpCurrAddr = CopyToBSB(mpCurrAddr,mpPacket->data+4, mpPacket->size-4);
                }
            }
            else
            {
                /*StartCode_ptr = Get_StartCode_info(mpPacket->data, mpPacket->size, &mdfsize);
                if (StartCode_ptr == NULL){
                    AMLOG_DEBUG("mpeg4: no start_code(0x000001) found in packet, skip it.\n");
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    msState = STATE_IDLE;
                    mpCurrAddr= pRecoveryAddr;
                    //PostEngineErrorMsg(ME_VIDEO_DATA_ERROR);
                    return ME_OK;
                }
                mpCurrAddr = CopyToBSB(mpCurrAddr,StartCode_ptr, mdfsize);*/
                mpCurrAddr = CopyToBSB(mpCurrAddr,mpPacket->data, mpPacket->size);
            }
        }
        else if (mpCodec->codec_id == CODEC_ID_MPEG1VIDEO||mpCodec->codec_id == CODEC_ID_MPEG2VIDEO) {
            //for mpeg12 DTS/PTS leaping case, bug#1883 #1915, 2012.01.31, roy mdf
            AM_UINT isPTSJump=0;
            if((AV_NOPTS_VALUE!=(AM_U64)mpPacket->dts)                                            //condition 1) current DTS is not AV_NOPTS_VALUE
                && (0x7FFFFFFFFFFFFFFFLL!=last_dts)                                         //condition 2) last DTS is not max value 0x7FFFFFFFFFFFFFFF nor AV_NOPTS_VALUE, to void decent wrap round
                && (AV_NOPTS_VALUE!=(AM_U64)last_dts)
                && ((last_dts-mpPacket->dts)>mpSharedRes->mVideoTicks*96)     //condition 3) leap is a large gap which larger than 24*4 frame ticks
                &&isAudPTSLeaped)                                                                   //condition 4) audio pts leap occured
            {
                isPTSJump=1;
                AMLOG_WARN("MPEG12, last_dts=%lld, mpPacket->dts=%lld, gap=%lld, threshold=%d, video DTS leap occur, isAudPTSLeaped=%u, isPTSJump=%u.\n",last_dts,mpPacket->dts,last_dts-mpPacket->dts,mpSharedRes->mVideoTicks*96,isAudPTSLeaped,isPTSJump);
                isAudPTSLeaped=0;//clear flag
            }

            last_dts=mpPacket->dts;//save last DTS
            if(mbAddUdecWarpper)
            {
                //2012.01.05, roy mdf
                //for MPEG12, Amba PES header should be ahead of its picture start code, if no pic start code 0x00000100 in packet, no add Amba PES header
                AM_UINT pesHeaderLen = 0;
                AM_U8* startcode_base_ptr = _Find_MPEG12_start_code(mpPacket->data, mpPacket->size, 0x00);
                if (startcode_base_ptr != NULL)
                {
                    //b1. copy the data before picture start code
                    AMLOG_DEBUG("AmbaVdec b1,  CopyToBSB size=%d.\n", startcode_base_ptr-mpPacket->data);
                    mpCurrAddr = CopyToBSB(mpCurrAddr,mpPacket->data, startcode_base_ptr-mpPacket->data);

                    //b2. insert Amba PES header
                    if (!mSpecifiedFrameRateTick) {
                        //generate pes header from pts, data length
                        AMLOG_PTS("AmbaVdec b2,  pkt->size=%d. pts=%lld, dts=%lld.\n", mpPacket->size,mpPacket->pts,mpPacket->dts);
                        if (mpPacket->pts >= 0) {
                            pesHeaderLen = FillPESHeader(mUPESHeader, (AM_U32)(mpPacket->pts&0xffffffff), (AM_U32)(((AM_U64)mpPacket->pts) >> 32), mpPacket->size, 1, isPTSJump);
                        } else if (mpPacket->dts >= 0) {
                            pesHeaderLen = FillPESHeader(mUPESHeader, (AM_U32)(mpPacket->dts&0xffffffff), (AM_U32)(((AM_U64)mpPacket->dts) >> 32), mpPacket->size, 1, isPTSJump);
                        } else {
                            pesHeaderLen = FillPESHeader(mUPESHeader, 0, 0, mpPacket->size, 0, 0);
                        }
                    } else {
                        //use seq frame rate
                        pesHeaderLen = FillPESHeader(mUPESHeader, 0, 0, mpPacket->size, 0, 0);
                    }
                    AM_ASSERT(pesHeaderLen <= DUDEC_PES_HEADER_LENGTH);
                    AMLOG_DEBUG("comes here?, pesHeaderLen %d, mpCurrAddr %p.\n", pesHeaderLen, mpCurrAddr);
                    AMLOG_DEBUG("AmbaVdec b2,  CopyToBSB size=%d.\n", pesHeaderLen);
                    mpCurrAddr = CopyToBSB(mpCurrAddr, mUPESHeader, pesHeaderLen);
                    AMLOG_DEBUG("done pesHeaderLen %d.mpCurrAddr %p.\n", pesHeaderLen, mpCurrAddr);
#ifdef AM_DEBUG
                    AMLOG_BINARY("Print UDEC PES Header: %d.\n", pesHeaderLen);
                    PrintBitstremBuffer(mUPESHeader, pesHeaderLen);
#endif

                    //b3. copy the data left, from picture start code
                    AMLOG_DEBUG("AmbaVdec b3,  CopyToBSB size=%d.\n", mpPacket->size-(startcode_base_ptr-mpPacket->data));
                    mpCurrAddr = CopyToBSB(mpCurrAddr,startcode_base_ptr, mpPacket->size-(startcode_base_ptr-mpPacket->data));
                }
                else
                {
                    mpCurrAddr = CopyToBSB(mpCurrAddr,mpPacket->data, mpPacket->size);
                }
            }
            else
            {
                mpCurrAddr = CopyToBSB(mpCurrAddr,mpPacket->data, mpPacket->size);
            }
        }
        else
        {
            mpCurrAddr = CopyToBSB(mpCurrAddr,mpPacket->data, mpPacket->size);
        }

#ifdef AM_DEBUG
        AM_ASSERT((mdWriteSize == (AM_UINT)(mpCurrAddr - pFrameStart)) || (mdWriteSize == (AM_UINT)(mpCurrAddr - mpStartAddr + mpEndAddr - pFrameStart)));
        AM_ASSERT(pFrameStart == mdLastEndPointer);
        AM_ASSERT(mpCurrAddr <= mpEndAddr);
        AM_ASSERT(mpCurrAddr >= mpStartAddr);
        AM_ASSERT(mdLastEndPointer == pFrameStart);
        mdLastEndPointer = mpCurrAddr;
        AM_ASSERT(mdRequestSize >= mdWriteSize);
        AMLOG_DEBUG("checking wrapper around mdRequestSize %d, mdWriteSize %d.\n", mdRequestSize, mdWriteSize);
#endif
        if (mpCurrAddr > pFrameStart)
            AMLOG_DEBUG("decoding size %d.\n", mpCurrAddr - pFrameStart);
        else
            AMLOG_DEBUG("decoding size %d.\n", mpCurrAddr + mSpace - pFrameStart);
        AMLOG_DEBUG("pFrameStart %p, mpCurrAddr %p, diff %d.\n", pFrameStart, mpCurrAddr, mpCurrAddr + mSpace - pFrameStart);
        //AMLOG_DEBUG("AmbaVideoDec: DecodeBuffer start, start %p, end %p, diff %d, totdiff %d, %d.\n", pFrameStart, mpCurrAddr, mpCurrAddr - pFrameStart, mpCurrAddr - mpStartAddr, mpEndAddr - mpCurrAddr);
        err = DecodeBuffer(pBuffer, 1, pFrameStart, mpCurrAddr);
        AMLOG_DEBUG("AmbaVideoDec: DecodeBuffer done.\n");

#ifdef AM_DEBUG
        mdWriteSize = 0;
#endif

        if (err != ME_OK) {
            mpBuffer = NULL;
            msState = STATE_PENDING;
            return err;
        }
        mpBuffer = NULL;
        msState = STATE_IDLE;
    }

    return err;
}


inline AM_U8* CAmbaVideoDecoder::Find_StartCode(AM_U8* extradata, AM_INT size)
{
    AM_U8* ptr = extradata;
    if(NULL==ptr)//check for safety
        return NULL;

    while (ptr < extradata + size - 4) {
        if (*ptr == 0x00)
        {
            if (*(ptr + 1)== 0x00)
            {
                if (*(ptr + 2)== 0x01)
                {
                    return ptr;
                }
            }
        }
        ++ptr;
    }
    return NULL;
}

inline AM_U8* CAmbaVideoDecoder::Get_StartCode_info(AM_U8* porgaddr, AM_INT orgsize, AM_INT* pmdfsize)
{
    AM_U8* StartCode_ptr = Find_StartCode(porgaddr, orgsize);
    if (StartCode_ptr == NULL){
        *pmdfsize=0;
        return NULL;
    }
    AM_ASSERT(StartCode_ptr>=porgaddr);
    *pmdfsize=orgsize-(StartCode_ptr-porgaddr);
    return StartCode_ptr;
}

void CAmbaVideoDecoder::GenerateConfigData()
{
    mSeqConfigDataLen = 0;
    mPicConfigDataLen = 0;

    switch (mpCodec->codec_id) {

        case CODEC_ID_VC1: {
                AM_U8 startcode[4] = {0, 0, 1,0x0d};
                memcpy(mPicConfigData, startcode, sizeof(startcode));
                mPicConfigDataLen = sizeof(startcode);

                if (!mpCodec->extradata || (mpCodec->extradata_size == 0)) {
                    break;
                }
                AM_U8* StartCode_ptr = Find_StartCode(mpCodec->extradata, mpCodec->extradata_size);
                if (StartCode_ptr == NULL){
                    AMLOG_DEBUG("=========[WVC1] No start code in extradata!!!========\n");
                    break;
                }
                mSeqConfigDataLen = mpCodec->extradata_size + mpCodec->extradata -StartCode_ptr;
                AMLOG_DEBUG("mSeqConfigDataLen=%d.\n",mSeqConfigDataLen);
                if (mpCodec->extradata_size <= mSeqConfigDataSize ||
                    AdjustSeqConfigDataSize(mpCodec->extradata_size)) {
                    memcpy(mSeqConfigData, StartCode_ptr, mSeqConfigDataLen);
                }
           }
            break;

        case CODEC_ID_WMV3: {
                AM_U8 padding0[] = {0x78,0x16,0x00};
                AM_U8 padding1[] = {0xc5,0x04,0x00,0x00,0x00};
                AM_U8 padding2[] = {0x0c,0x00,0x00,0x00};
                AM_U8 padding3[] = {0xd5,0x0c,0x00,0x10,0x5d,0xf2,0x05,0x00,0x0c,0x00,0x00,0x00};

                memcpy(mSeqConfigData + mSeqConfigDataLen, padding0, sizeof(padding0));
                mSeqConfigDataLen += sizeof(padding0);

                memcpy(mSeqConfigData + mSeqConfigDataLen, padding1, sizeof(padding1));
                mSeqConfigDataLen += sizeof(padding1);

                memcpy(mSeqConfigData + mSeqConfigDataLen, mpCodec->extradata, 4);
                mSeqConfigDataLen += 4;

                memcpy(mSeqConfigData + mSeqConfigDataLen, (AM_U8 *)(&(mpCodec->height)), 4);
                mSeqConfigDataLen += 4;

                memcpy(mSeqConfigData + mSeqConfigDataLen, (AM_U8 *)(&(mpCodec->width)), 4);
                mSeqConfigDataLen += 4;

                memcpy(mSeqConfigData + mSeqConfigDataLen, padding2, sizeof(padding2));
                mSeqConfigDataLen += sizeof(padding2);

                memcpy(mSeqConfigData + mSeqConfigDataLen, padding3, sizeof(padding3));
                mSeqConfigDataLen += sizeof(padding3);
            }
            break;

        case CODEC_ID_MPEG1VIDEO:
            break;

        case CODEC_ID_MPEG2VIDEO:
            break;

        case CODEC_ID_MPEG4:
            if (mpCodec->extradata_size <= mSeqConfigDataSize || AdjustSeqConfigDataSize(mpCodec->extradata_size)) {
                AM_U8* StartCode_ptr = NULL;
                AM_INT mdfsize=0;
                //for wv1f
                if(mbMP4WV1F)
                {
                    AM_U8* WV1F4CC_ptr = _Check_WV1F4CC(mpCodec->extradata);
                    if(!WV1F4CC_ptr)
                    {
                        StartCode_ptr = Get_StartCode_info(mpCodec->extradata, mpCodec->extradata_size, &mdfsize);
                        if (StartCode_ptr == NULL){
                            AMLOG_DEBUG("mpeg4: no start_code(0x000001) found in extradata, no send seq config data.\n");
                            break;
                        }
                        memcpy(mSeqConfigData + mSeqConfigDataLen, StartCode_ptr, mdfsize);
                        mSeqConfigDataLen += mdfsize;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    StartCode_ptr = Get_StartCode_info(mpCodec->extradata, mpCodec->extradata_size, &mdfsize);
                    if (StartCode_ptr == NULL){
                        AMLOG_DEBUG("mpeg4: no start_code(0x000001) found in extradata, no send seq config data. And need find first vol header(0x00000120) in packets.\n");
                        mbNeedFindVOLHead = true;
                        break;
                    }
                    memcpy(mSeqConfigData + mSeqConfigDataLen, StartCode_ptr, mdfsize);
                    mSeqConfigDataLen += mdfsize;
                    /*memcpy(mSeqConfigData + mSeqConfigDataLen, mpCodec->extradata, mpCodec->extradata_size);
                    mSeqConfigDataLen += mpCodec->extradata_size;*///mdf for bug914/396
                }
            }
            break;

        case CODEC_ID_H264: {
                AM_U8 startcode[4] = {0, 0, 0, 0x01};
                memcpy(mPicConfigData, startcode, sizeof(startcode));
                mPicConfigDataLen = sizeof(startcode);

                if (!mpCodec->extradata || (mpCodec->extradata_size == 0)) {
                    break;
                }
                // extradata can be annex-b or AVCC format.
                // if extradata begins with sequence: 00 00 00 01, it's annex-b format,
                // else extradata is AVCC format, it needs to know what the is the size of the nal_size field, in bytes.
                // AVCC format: see ISO/IEC 14496-15 5.2.4.1
                AMLOG_INFO("extradata size:%d data:%02x %02x %02x %02x %02x.\n",
                    mpCodec->extradata_size,
                    mpCodec->extradata[0],
                    mpCodec->extradata[1],
                    mpCodec->extradata[2],
                    mpCodec->extradata[3],
                    mpCodec->extradata[4]);
                PrintBitstremBuffer(mpCodec->extradata, mpCodec->extradata_size);

                if (mpCodec->extradata[0] != 0x01) {
                    AMLOG_INFO("extradata is annex-b format.\n");
                    mH264DataFmt = H264_FMT_ANNEXB;

                    if (mpCodec->extradata_size <= mSeqConfigDataSize ||
                        AdjustSeqConfigDataSize(mpCodec->extradata_size)) {

                        memcpy(mSeqConfigData + mSeqConfigDataLen, mpCodec->extradata, mpCodec->extradata_size);
                        mSeqConfigDataLen += mpCodec->extradata_size;
                    }
                } else {
                    AMLOG_INFO("extradata is AVCC format.\n");
                    mH264DataFmt = H264_FMT_AVCC;

                    AM_INT spss = BE_16(mpCodec->extradata + 6);
                    AM_INT ppss = BE_16(mpCodec->extradata + 6 + 2 + spss + 1);
                    mH264AVCCNaluLen = 1 + (mpCodec->extradata[4] & 3);

                    // the configuration record shall contain no sequence or picture parameter sets
                    // (spss and ppss shall both have the value 0).
                    if (spss > 0 &&
                        ppss > 0 &&
                        spss + ppss < mpCodec->extradata_size) {

                        AM_INT size = 2 * sizeof(startcode) + spss + ppss;
                        if (size <= mSeqConfigDataSize ||
                            AdjustSeqConfigDataSize(size)) {

                            memcpy(mSeqConfigData + mSeqConfigDataLen, startcode, sizeof(startcode));
                            mSeqConfigDataLen += sizeof(startcode);

                            memcpy(mSeqConfigData + mSeqConfigDataLen, mpCodec->extradata + 8, spss);
                            mSeqConfigDataLen += spss;

                            memcpy(mSeqConfigData + mSeqConfigDataLen, startcode, sizeof(startcode));
                            mSeqConfigDataLen += sizeof(startcode);

                            memcpy(mSeqConfigData + mSeqConfigDataLen, mpCodec->extradata + 6 + 2 + spss + 1 + 2, ppss);
                            mSeqConfigDataLen += ppss;
                        }
                    }
                    AMLOG_INFO("-------spss:%d,ppss:%d NaluLen:%d-----\n", spss,ppss, mH264AVCCNaluLen);
                }
            }
            break;

        default:
            AM_ERROR("bad codec_id %d.\n", mpCodec->codec_id);
            break;
    }

    AM_ASSERT(mSeqConfigDataLen <= mSeqConfigDataSize);

    AMLOG_BINARY("Generate sequence config data: codec_id %d, mSeqConfigDataLen %d.\n", mpCodec->codec_id, mSeqConfigDataLen);
#ifdef AM_DEBUG
    PrintBitstremBuffer(mSeqConfigData ,mSeqConfigDataLen);
#endif

}

void CAmbaVideoDecoder::UpdatePicConfigData(AVPacket *mpPacket)
{
    switch (mpCodec->codec_id) {

        case CODEC_ID_WMV3: {
                AM_INT temp = mpPacket->size;
                AM_U8 *p = (AM_U8 *)(&temp);
                if (mpPacket->flags&PKT_FLAG_KEY)
                    *(p + 3) = 0x80;

                memcpy(mPicConfigData, p, 4);
                temp = 0;
                memcpy(mPicConfigData + 4, p, 4);
                mPicConfigDataLen = 8;
            }
            break;

        case CODEC_ID_VC1:
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
        case CODEC_ID_MPEG4:
        case CODEC_ID_H264:
            break;

        default:
            break;

    }

}

bool CAmbaVideoDecoder::AdjustSeqConfigDataSize(AM_INT size)
{
    AM_U8 *pBuf = (AM_U8 *)av_malloc(size);
    if (pBuf) {
        av_free(mSeqConfigData);
        mSeqConfigDataSize = size;
        mSeqConfigData = pBuf;
        AMLOG_INFO("AdjustSeqConfigDataSize success.\n");
    }

    return (pBuf != NULL);
}

bool CAmbaVideoDecoder::CheckH264SPSExist(AM_U8 *pData, AM_INT iDataLen, AM_INT *pSPSPos, AM_INT *pSPSLen)
{
    // REF ISO-IEC 14496-10 7.3
    // 1. find SPS
    // 2. if SPS is found, then find PPS
    // 3. if SPS&PPS are found, then calcaulte the length of SPS/PPS

    AM_INT i = 0;
    AM_UINT startCode = 0;

    bool bSPSFound = false;

    // find SPS
    startCode = -1;
    while (i < iDataLen) {
        if ((startCode == 0x000001) && ((pData[i]&0x1F) == 7)) {
            bSPSFound = true;
            *pSPSPos = (pData[i-4] == 0) ? (i-4) : (i-3);
            //AMLOG_INFO("1 startCode=%4X pData[i]=%X\n", startCode, pData[i]&0x1F);
            break;
        }

        startCode = ((startCode<<8)|pData[i])&0xFFFFFF;
        i++;
    }

    bool bPPSFound = false;
    if (bSPSFound) {

        // find PPS
        startCode = -1;
        while (i < iDataLen) {
            if ((startCode == 0x000001) && ((pData[i]&0x1F) == 8)) {
                bPPSFound = true;
                //AMLOG_INFO("2 startCode=%4X pData[i]=%X\n", startCode, pData[i]&0x1F);
                break;
            }

            startCode = ((startCode<<8)|pData[i])&0xFFFFFF;
            i++;
        }
    }

    if (bSPSFound && bPPSFound) {
        // count the length of SPS&PPS
        startCode = -1;
        while (i < iDataLen) {
            if ((startCode == 0x000001) && ((pData[i]&0x1F) != 8)) {
                *pSPSLen = (pData[i-4] == 0) ? (i-4-*pSPSPos) : (i-3-*pSPSPos);
                //AMLOG_INFO("3 startCode=%4X pData[i]=%X\n", startCode, pData[i]&0x1F);
                break;
            }

            startCode = ((startCode<<8)|pData[i])&0xFFFFFF;
            i++;
        }
        if (i >= iDataLen) {//mdf for bug#2750
            *pSPSLen = iDataLen-*pSPSPos;
        }
    }

    //AMLOG_INFO("bSPSFound=%d *pSPSPos=%d *pSPSLen=%d\n", bSPSFound, *pSPSPos, *pSPSLen);

    return bSPSFound;
}

void CAmbaVideoDecoder::FeedConfigDataWithUDECWrapper(AVPacket *mpPacket)
{
    AM_UINT auLength = 0;
    AM_UINT pesHeaderLen = 0;

    //send seq data
    if (!mbConfigData || ((mpCodec->codec_id == CODEC_ID_H264) && (mpPacket->flags&PKT_FLAG_KEY))) {

        if (!(mLogOutput & LogDisableNewPath)) {
            AMLOG_INFO("!!Feeding DUDEC_SEQ_HEADER %d.\n", mpSharedRes->is_mp4s_flag?DUDEC_SEQ_HEADER_EX_LENGTH:DUDEC_SEQ_HEADER_LENGTH);
            mpCurrAddr = CopyToBSB(mpCurrAddr, mUSEQHeader, mpSharedRes->is_mp4s_flag?DUDEC_SEQ_HEADER_EX_LENGTH:DUDEC_SEQ_HEADER_LENGTH);
        }else{
            if( mpCodec->codec_id == CODEC_ID_WMV3 ) mpCurrAddr = CopyToBSB(mpCurrAddr, mUSEQHeader, mpSharedRes->is_mp4s_flag?DUDEC_SEQ_HEADER_EX_LENGTH:DUDEC_SEQ_HEADER_LENGTH);
        }

        if (mpCodec->codec_id == CODEC_ID_H264) {
            // Pre-check SPS/PPS Purpose: to avoid send different SPS/PPS, when SPS/PPS has embedded in frame data and varied
            // 1. find sps/pps in limited packet header, in which checkSPSLen is an experienced value
            // 2. if not found, then copy SeqConfigData
            // 3. if found and varied, then update SeqConfigData but needn't copy SeqConfigData
            bool bSPSFound = false;
            int idxSPSStart = 0, idxSPSLen = 0;
            int checkSPSLen = (mpPacket->size > mSeqConfigDataLen) ? mSeqConfigDataLen : mpPacket->size;
            bSPSFound = CheckH264SPSExist(mpPacket->data, checkSPSLen, &idxSPSStart, &idxSPSLen);
            if (bSPSFound) {
                // check sps len, update SeqConfigData if sps varies.
                if (idxSPSLen != mSeqConfigDataLen) {
                    if (AdjustSeqConfigDataSize(idxSPSLen)) {
                        memcpy(mSeqConfigData, mpPacket->data+idxSPSStart, idxSPSLen);
                        mSeqConfigDataLen = idxSPSLen;
                    } else {
                        AMLOG_ERROR("AdjustSeqConfigDataSize Failure\n");
                    }
                } else {
                    AMLOG_WARN("idxSPSLen is equal to mSeqConfigDataLen\n");
                }
                mbConfigData = true;
            } else {
                if ((H264_FMT_AVCC == mH264DataFmt)
                    || ((mSeqConfigData[4]&0x1F) == 7)) {//mdf for bug#2750
                    mpCurrAddr = CopyToBSB(mpCurrAddr, mSeqConfigData, mSeqConfigDataLen);
                    AMLOG_INFO("h264 mSeqConfigData start with SPS.\n");
                } else {
                    int idxSPSPPSStart = 0, idxSPSPPSLen = 0;
                    AMLOG_INFO("h264 mSeqConfigData NOT start with SPS, find SPS&PPS manually.\n");
                    if (CheckH264SPSExist(mSeqConfigData, mSeqConfigDataLen, &idxSPSPPSStart, &idxSPSPPSLen)) {
                        mpCurrAddr = CopyToBSB(mpCurrAddr, mSeqConfigData+idxSPSPPSStart, idxSPSPPSLen);
                        AMLOG_INFO("h264 mSeqConfigData contribute SPS&PPS with offset=0x%x, len=%d.\n", idxSPSPPSStart, idxSPSPPSLen);
                    } else {
                        AMLOG_INFO("h264 mSeqConfigData has NO SPS&PPS.\n");
                    }
                }
                mbConfigData = true;
            }
        } else {
            mpCurrAddr = CopyToBSB(mpCurrAddr, mSeqConfigData, mSeqConfigDataLen);
            mbConfigData = true;
        }

#ifdef AM_DEBUG
        AMLOG_BINARY("Print UDEC SEQ: %d.\n", mpSharedRes->is_mp4s_flag?DUDEC_SEQ_HEADER_EX_LENGTH:DUDEC_SEQ_HEADER_LENGTH);
        PrintBitstremBuffer(mUSEQHeader, mpSharedRes->is_mp4s_flag?DUDEC_SEQ_HEADER_EX_LENGTH:DUDEC_SEQ_HEADER_LENGTH);
        AMLOG_BINARY("Print seq config data: %d.\n", mSeqConfigDataLen);
        PrintBitstremBuffer(mSeqConfigData, mSeqConfigDataLen);
#endif
    }

    UpdatePicConfigData(mpPacket);

    if(mpCodec->codec_id == CODEC_ID_VC1){
        if(*(mpPacket->data) == 0 && *(mpPacket->data+1) == 0 && *(mpPacket->data+2) == 1 ){
            mPicConfigDataLen = 0;
        }
    }

    auLength = mPicConfigDataLen + mpPacket->size;
    if (mpCodec->codec_id == CODEC_ID_H264 || mpCodec->codec_id == CODEC_ID_WMV3) {
        auLength -= 4;
    }

    if(mpCodec->codec_id == CODEC_ID_MPEG4)
    {
        AM_INT mdfsize=0;
        AM_U8* StartCode_ptr = NULL;
        AM_U8* WV1F4CC_ptr = NULL;

        //if(!mbMP4WV1F || !(WV1F4CC_ptr = _Check_WV1F4CC(mpPacket->data)))
        if(mbMP4WV1F && !(WV1F4CC_ptr = _Check_WV1F4CC(mpPacket->data)))
        {
            StartCode_ptr = Get_StartCode_info(mpPacket->data, mpPacket->size, &mdfsize);
            if (StartCode_ptr == NULL){
                AMLOG_DEBUG("mpeg4: no start_code(0x000001) found in mpPacket, no send pes header.\n");
                return;
            }
            auLength = mPicConfigDataLen + mdfsize;
        }
    }

    if (CODEC_ID_H264==mpCodec->codec_id
        && H264_FMT_ANNEXB== mH264DataFmt
        && !mH264SEIFound
        && Find_H264_SEI(mpPacket->data, mpPacket->size)) {//mdf for bug#2750
        AMLOG_WARN("h264 SEI found in packet, feed SPS&PPS first...... PES header will not filled with packet pts later, pls note that.\n");
        mpCurrAddr = CopyToBSB(mpCurrAddr, mSeqConfigData, mSeqConfigDataLen);
        mH264SEIFound = true;
    }

    if (!mSpecifiedFrameRateTick && !mH264SEIFound) {//mdf for bug#2750
        //generate pes header from pts, data length
        AMLOG_PTS("AmbaVdec [PTS], current pts %lld.\n", mpPacket->pts);
        if (mpPacket->pts >= 0) {
            pesHeaderLen = FillPESHeader(mUPESHeader, (AM_U32)(mpPacket->pts&0xffffffff), (AM_U32)(((AM_U64)mpPacket->pts) >> 32), auLength, 1, 0);
        } else {
            pesHeaderLen = FillPESHeader(mUPESHeader, 0, 0, auLength, 0, 0);
        }
    } else {
        //use seq frame rate
        pesHeaderLen = FillPESHeader(mUPESHeader, 0, 0, auLength, 0, 0);
    }
    AM_ASSERT(pesHeaderLen <= DUDEC_PES_HEADER_LENGTH);

    //send udec pes header
    if (!(mLogOutput & LogDisableNewPath)) {
        AMLOG_DEBUG("comes here?, pesHeaderLen %d, mpCurrAddr %p.\n", pesHeaderLen, mpCurrAddr);
        mpCurrAddr = CopyToBSB(mpCurrAddr, mUPESHeader, pesHeaderLen);
        AMLOG_DEBUG("done pesHeaderLen %d.mpCurrAddr %p.\n", pesHeaderLen, mpCurrAddr);
    }else{
        if( mpCodec->codec_id == CODEC_ID_WMV3 ) mpCurrAddr = CopyToBSB(mpCurrAddr, mUPESHeader, pesHeaderLen);
   }

    //send pic config data
    if (mPicConfigDataLen) {
//        AM_INFO("comes here 2?\n");
        mpCurrAddr = CopyToBSB(mpCurrAddr, mPicConfigData, mPicConfigDataLen);
//        AM_ASSERT(mpCodec->codec_id == CODEC_ID_H264 || mpCodec->codec_id == CODEC_ID_VC1 || mpCodec->codec_id == CODEC_ID_WMV3);
    }

#ifdef AM_DEBUG
    AMLOG_BINARY("Print UDEC PES Header: %d.\n", pesHeaderLen);
    PrintBitstremBuffer(mUPESHeader, pesHeaderLen);
    AMLOG_BINARY("Print pic config data: %d.\n", mPicConfigDataLen);
    PrintBitstremBuffer(mPicConfigData, mPicConfigDataLen);
#endif

}


void CAmbaVideoDecoder::FeedConfigData(AVPacket *mpPacket)
{
	switch (mpCodec->codec_id) {
	case CODEC_ID_VC1: {
			AM_U8 startcode[4] = {0, 0, 1,0x0d};
			if(!mbConfigData) {
				mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata + 1, mpCodec->extradata_size - 1);
				mbConfigData = true;
			}
			mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));

		}
		break;

	case CODEC_ID_WMV3: {
			AM_U8 padding0[] = {0x78,0x16,0x00};
			AM_U8 padding1[] = {0xc5,0x04,0x00,0x00,0x00};
			AM_U8 padding2[] = {0x0c,0x00,0x00,0x00};
			AM_U8 padding3[] = {0xd5,0x0c,0x00,0x10,0x5d,0xf2,0x05,0x00,0x0c,0x00,0x00,0x00};

			if(!mbConfigData) {
				//feed the number of frames
				//mpCurrAddr = CopyToBSB(mpCurrAddr, (AM_U8 *)(&(mpStream->nb_frames)), 3);
				mpCurrAddr = CopyToBSB(mpCurrAddr, padding0, sizeof(padding0));
				mpCurrAddr = CopyToBSB(mpCurrAddr, padding1, sizeof(padding1));
				mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata, /*mpCodec->extradata_size - 1*/4);
				mpCurrAddr = CopyToBSB(mpCurrAddr, (AM_U8 *)(&(mpCodec->height)), 4);
				mpCurrAddr = CopyToBSB(mpCurrAddr, (AM_U8 *)(&(mpCodec->width)), 4);
				mpCurrAddr = CopyToBSB(mpCurrAddr, padding2, sizeof(padding2));
				mpCurrAddr = CopyToBSB(mpCurrAddr, padding3, sizeof(padding3));
				mbConfigData = true;
			}

			//feed frame header
			AM_INT temp = mpPacket->size;
			AM_U8 *p = (AM_U8 *)(&temp);
			if (mpPacket->flags&PKT_FLAG_KEY)
				*(p + 3) = 0x80;

			mpCurrAddr = CopyToBSB(mpCurrAddr, p, 4);

			temp = 0;
			mpCurrAddr = CopyToBSB(mpCurrAddr, p, 4);
		}
		break;

	case CODEC_ID_MPEG1VIDEO:
               break;//for bug1301,etc, MPEG1 no feed seq config data

	case CODEC_ID_MPEG2VIDEO:{
                if(!mbConfigData) {
                    AM_U8 startcode[4] = {0, 0, 1,0xb3};
                    AM_U8* StartCode_ptr = mpCodec->extradata;
                    if (!mpCodec->extradata || (mpCodec->extradata_size == 0)) {
                        mbConfigData = true;
                        break;
                    }

                    while (StartCode_ptr < mpCodec->extradata + mpCodec->extradata_size - 4) {
                        if (*StartCode_ptr == 0x00)
                            if (*(StartCode_ptr + 1)== 0x01)
                                if (*(StartCode_ptr + 2)== 0x0b3){
                                    StartCode_ptr += 3;
                                    break;
                                }
                        ++StartCode_ptr;
                    }
                    if(StartCode_ptr >= mpCodec->extradata + mpCodec->extradata_size - 4){
                        AMLOG_INFO("=========[MPEG12] No start code in extradata!!!========\n");
                        mbConfigData = true;
                        break;
                    }
                    mSeqConfigDataLen = mpCodec->extradata_size + mpCodec->extradata - StartCode_ptr;
                    AMLOG_INFO("mSeqConfigDataLen=%d.\n",mSeqConfigDataLen);
                    if (mpCodec->extradata_size <= mSeqConfigDataSize ||
                        AdjustSeqConfigDataSize(mSeqConfigDataLen+4)) {
                        AMLOG_INFO("=========[MPEG12] Add extradata, mSeqConfigDataLen = %d.========\n", mSeqConfigDataLen);
                        memcpy(mSeqConfigData, startcode, 4);
                        memcpy(mSeqConfigData+4, StartCode_ptr, mSeqConfigDataLen);
                    }
                    mpCurrAddr = CopyToBSB(mpCurrAddr, mSeqConfigData, mSeqConfigDataLen);
                    mbConfigData = true;
                }
            }
            break;

	case CODEC_ID_MPEG4: {
			if(!mbConfigData) {
                            mbConfigData = true;
                            AM_U8* StartCode_ptr=NULL;
                            AM_INT mdfsize=0;
                            //for wv1f
                            if(mbMP4WV1F)
                            {
                                AM_U8* WV1F4CC_ptr = _Check_WV1F4CC(mpCodec->extradata);
                                if(!WV1F4CC_ptr)
                                {
                                    StartCode_ptr = Get_StartCode_info(mpCodec->extradata, mpCodec->extradata_size, &mdfsize);
                                    if (StartCode_ptr == NULL){
                                        AMLOG_DEBUG("mpeg4: no start_code(0x000001) found in extradata, no send seq config data.\n");
                                        break;
                                    }
                                    mpCurrAddr = CopyToBSB(mpCurrAddr, StartCode_ptr, mdfsize);
                                }
                                else
                                {
                                    break;
                                }
                            }
                            else
                            {
                                StartCode_ptr = Get_StartCode_info(mpCodec->extradata, mpCodec->extradata_size, &mdfsize);
                                if (StartCode_ptr == NULL){
                                    AMLOG_DEBUG("mpeg4: no start_code(0x000001) found in extradata, no send seq config data. And need find first vol header(0x00000120) in packets.\n");
                                    mbNeedFindVOLHead = true;
                                    break;
                                }
                                mpCurrAddr = CopyToBSB(mpCurrAddr, StartCode_ptr, mdfsize);
                                /*mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata, mpCodec->extradata_size);*///mdf for bug914/396
                            }
			}
		}
		break;

	case CODEC_ID_H264: {
			AM_U8 startcode[4] = {0, 0, 0,0x01};
			if (mpPacket->flags&PKT_FLAG_KEY) {
                            if ((H264_FMT_AVCC == mH264DataFmt)
                                || ((mSeqConfigData[4]&0x1F) == 7)) {//mdf for bug#2750
                                mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
                                AM_INT spss = BE_16(mpCodec->extradata + 6);
                                mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata + 8, spss);

                                mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
                                AM_INT ppss = BE_16(mpCodec->extradata + 6 + 2 + spss + 1);
                                mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata + 6 + 2 + spss + 1 + 2, ppss);
                                AMLOG_INFO("h264 extradata start with SPS -------spss:%d,ppss:%d-----\n",spss,ppss);
                            } else {
                                int idxSPSPPSStart = 0, idxSPSPPSLen = 0;
                                AMLOG_INFO("h264 extradata NOT start with SPS, find SPS&PPS manually.\n");
                                if (CheckH264SPSExist(mpCodec->extradata, mpCodec->extradata_size, &idxSPSPPSStart, &idxSPSPPSLen)) {
                                    mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata+idxSPSPPSStart, idxSPSPPSLen);
                                    AMLOG_INFO("h264 extradata contribute SPS&PPS with offset=0x%x, len=%d.\n", idxSPSPPSStart, idxSPSPPSLen);
                                } else {
                                    AMLOG_INFO("h264 extradata has NO SPS&PPS.\n");
                                }
                            }
			}
			mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
		}
		break;

	default:
		break;

	}

}


void CAmbaVideoDecoder::dumpEsData(AM_U8* pStart, AM_U8* pEnd)
{
    //dump data write to file
    if (mLogOutput & LogDumpTotalBinary) {
        if (!mpDumpFile) {
            snprintf(mDumpFilename, DAMF_MAX_FILENAME_LEN, "%s/es.data", AM_GetPath(AM_PathDump));
            mpDumpFile = fopen(mDumpFilename, "ab");
            //AMLOG_INFO("open  mpDumpFile %p.\n", mpDumpFile);
        }
        if (mpDumpFile) {
            //AM_INFO("write data.\n");
            AM_ASSERT(pEnd != pStart);
            if (pEnd < pStart) {
                //wrap around
                fwrite(pStart, 1, (size_t)(mpEndAddr - pStart), mpDumpFile);
                fwrite(mpStartAddr, 1, (size_t)(pEnd - mpStartAddr), mpDumpFile);
            } else {
                fwrite(pStart, 1, (size_t)(pEnd - pStart), mpDumpFile);
            }
            fclose(mpDumpFile);
            mpDumpFile = NULL;
        } else {
            //AMLOG_INFO("open  mpDumpFile fail.\n");
        }
    }

    if (mLogOutput & LogDumpSeparateBinary) {

        mDumpIndex++;
        if(mDumpIndex < mDumpStartFrame || mDumpIndex > mDumpEndFrame)
            return;

        snprintf(mDumpFilename, DAMF_MAX_FILENAME_LEN, "%s/dump/%d.dump", AM_GetPath(AM_PathDump), mDumpIndex);
        mpDumpFileSeparate = fopen(mDumpFilename, "wb");
        if (mpDumpFileSeparate) {
            AM_ASSERT(pEnd != pStart);
            if (pEnd < pStart) {
                //wrap around
                fwrite(pStart, 1, (size_t)(mpEndAddr - pStart), mpDumpFileSeparate);
                fwrite(mpStartAddr, 1, (size_t)(pEnd - mpStartAddr), mpDumpFileSeparate);
            } else {
                fwrite(pStart, 1, (size_t)(pEnd - pStart), mpDumpFileSeparate);
            }

        }
        fclose(mpDumpFileSeparate);
    }
}

AM_ERR CAmbaVideoDecoder::DecodeBuffer(CBuffer *pBuffer, AM_INT numPics, AM_U8* pStart, AM_U8* pEnd)
{
    AM_UINT udec_state;
    AM_UINT vout_state;
    AM_UINT error_code;
    iav_udec_decode_t dec;
    memset(&dec, 0, sizeof(dec));
    dec.udec_type = muDecType;
    dec.decoder_id = mUdecInfo[0].udec_id;
    dec.u.fifo.start_addr = pStart;
    dec.u.fifo.end_addr = pEnd;
    dec.num_pics = numPics;
    AMLOG_BINARY("DecodeBuffer length %d, %x.\n", pEnd-pStart, *pStart);

#ifdef AM_DEBUG
    dumpEsData(pStart, pEnd);
#endif

    if (mInPrefetching && (mpSharedRes->dspConfig.preset_prefetch_count > mCurrentPrefetchCount)) {
        if (!mpPrefetching) {
            mpPrefetching = pStart;
            AM_ASSERT(0 == mCurrentPrefetchCount);
            AMLOG_INFO("[prebuffering start], preset_prefetch_count %d, mpPrefetching %p\n", mpSharedRes->dspConfig.preset_prefetch_count, mpPrefetching);
        }

        mCurrentPrefetchCount ++;
        AMLOG_INFO("[prebuffering]: mCurrentPrefetchCount %d, preset_prefetch_count %d\n", mCurrentPrefetchCount, mpSharedRes->dspConfig.preset_prefetch_count);
        if (mpSharedRes->dspConfig.preset_prefetch_count == mCurrentPrefetchCount) {
            AMLOG_INFO("[prebuffering done]\n");
            mInPrefetching = 0;
            dec.u.fifo.start_addr = mpPrefetching;
            dec.num_pics = mCurrentPrefetchCount;
        }
    }

#ifndef fake_feeding
    if (!mInPrefetching) {
        if ((mRet = ::ioctl(mIavFd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
            perror("IAV_IOC_UDEC_DECODE");
            AMLOG_ERROR("!!!!!IAV_IOC_UDEC_DECODE error, ret %d.\n", mRet);
            if (mRet == (-EPERM)) {
                if (GetUdecState (mIavFd, &udec_state, &vout_state, &error_code)) {
                    pthread_mutex_lock(&mpSharedRes->mMutex);
                    if (mpSharedRes->udec_state != IAV_UDEC_STATE_ERROR && udec_state == IAV_UDEC_STATE_ERROR) {
                        mpSharedRes->udec_state = udec_state;
                        mpSharedRes->vout_state = vout_state;
                        mpSharedRes->error_code = error_code;
                        AMLOG_ERROR("IAV_IOC_UDEC_DECODE(1) error, post msg, udec state %d, vout state %d, error_code 0x%x.\n", udec_state, vout_state, error_code);
                        PostEngineErrorMsg(ME_UDEC_ERROR);
                    } else {
                        AMLOG_ERROR("IAV_IOC_UDEC_DECODE(1) error, (udec error has been reported, just print error code), udec state %d, vout state %d, error_code 0x%x, mRet %d.\n", udec_state, vout_state, error_code, mRet);
                    }
                    pthread_mutex_unlock(&mpSharedRes->mMutex);
                }
                pBuffer->Release();
                return ME_ERROR;
            } else {
                //AM_ERROR("----IAV_IOC_UDEC_DECODE----ME_ERROR");
                pBuffer->Release();
                return ME_ERROR;
            }
        }

        if (!mbStreamStart) {
            mbStreamStart = true;
            pthread_mutex_lock(&mpSharedRes->mMutex);
            GetUdecState(mIavFd, &mpSharedRes->udec_state, &mpSharedRes->vout_state, &mpSharedRes->error_code);
            pthread_mutex_unlock(&mpSharedRes->mMutex);
            AM_ASSERT(IAV_UDEC_STATE_RUN == mpSharedRes->udec_state);
            PostEngineMsg(IEngine::MSG_NOTIFY_UDEC_IS_RUNNING);
        }
    }
#endif

#ifdef DDEBUG_CHECK_DRAM_DATA_TRASH
    //check if data is trashed
    AMLOG_DEBUG("checking, count %d.\n", mCheckCount);
    checkDramDataTrashed();

    //store new, for next check
    AMLOG_DEBUG("store(0), count %d.\n", mCheckCount);
    storeDramData(0);
#endif

	pBuffer->Release();
	//AM_INFO("-------DecodeBuffer-----pStart:0x%p,pEnd:0x%p\n",pStart,pEnd);
	return ME_OK;

}

#ifdef AM_DEBUG
void CAmbaVideoDecoder::PrintState()
{
    AMLOG_INFO("CAmbaVideoDecoder: msState=%d, %d input data, %d free buffers.\n", msState, mpVideoInputPin->mpBufferQ->GetDataCnt(), mpBufferPool->GetFreeBufferCnt());
}
#endif



//-----------------------------------------------------------------------
//
// CAmbaVideoInput
//
//-----------------------------------------------------------------------
CAmbaVideoInput* CAmbaVideoInput::Create(CFilter *pFilter)
{
	CAmbaVideoInput *result = new CAmbaVideoInput(pFilter);
	if (result && result->Construct()) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaVideoInput::Construct()
{
	AM_ERR err = inherited::Construct(((CAmbaVideoDecoder*)mpFilter)->MsgQ());
	if (err != ME_OK)
		return err;
	return ME_OK;
}

CAmbaVideoInput::~CAmbaVideoInput()
{
}

AM_ERR CAmbaVideoInput::CheckMediaFormat(CMediaFormat *pFormat)
{
	return ((CAmbaVideoDecoder*)mpFilter)->SetInputFormat(pFormat);
}



//-----------------------------------------------------------------------
//
// CAmbaVideoOutput
//
//-----------------------------------------------------------------------
CAmbaVideoOutput *CAmbaVideoOutput::Create(CFilter *pFilter)
{
	CAmbaVideoOutput *result = new CAmbaVideoOutput(pFilter);
	if (result && result->Construct() != ME_OK) {
		AM_DELETE(result);
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaVideoOutput::Construct()
{
	return ME_OK;
}

CAmbaVideoOutput::~CAmbaVideoOutput()
{
	AM_INFO("~~CAmbaVideoOutput\n");
}

AM_ERR CAmbaVideoOutput::SetOutputFormat()
{
	mMediaFormat.pMediaType = &GUID_Amba_Decoded_Video;
	mMediaFormat.pSubType = &GUID_Video_YUV420NV12;
	mMediaFormat.pFormatType = &GUID_Format_FFMPEG_Media;
	mMediaFormat.format = ((CAmbaVideoDecoder*)mpFilter)->mIavFd;
	mMediaFormat.mDspIndex = ((CAmbaVideoDecoder*)mpFilter)->mDspIndex;
	mMediaFormat.picWidth = ((AVCodecContext*)((CAmbaVideoDecoder*)mpFilter)->mpCodec)->width;
	mMediaFormat.picHeight = ((AVCodecContext*)((CAmbaVideoDecoder*)mpFilter)->mpCodec)->height;
	return ME_OK;
}



