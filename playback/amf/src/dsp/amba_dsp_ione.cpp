/**
 * amba_dsp_ione.cpp
 *
 * History:
 *  2011/05/31 - [QXZheng] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"

//for cgconfig
#include "general_header.h"

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "libavcodec/amba_dsp_define.h"
}

extern "C" {
#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_transcode_drv.h"
#include "iav_duplex_drv.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#if PLATFORM_ANDROID
#include "vout.h"
#endif
}

#include "am_dsp_if.h"
#include "amba_dsp_common.h"
#include "amba_dsp_ione.h"

//need mutex protect here? be caution to call this fuction
CAmbaDspIOne* CAmbaDspIOne::Create(SConsistentConfig* pShared)
{
    CAmbaDspIOne* mpInstance;
    if (!pShared) {
        AM_ASSERT(0);
        return NULL;
    }

    if (pShared->udecHandler) {
        AM_ASSERT(0);
        AM_ERROR("engine already has a udecHandler.\n");
        return (CAmbaDspIOne*)pShared->udecHandler;
    }

    AM_ASSERT(pShared->mIavFd < 0);
    AM_ASSERT(pShared->mbIavInited == 0);
    AM_ASSERT(pShared->voutHandler == NULL);

    mpInstance = new CAmbaDspIOne(pShared, &pShared->dspConfig);

    if(mpInstance && mpInstance->Construct() != ME_OK)
    {
        AM_ASSERT(0);
        delete mpInstance;
        mpInstance = NULL;
    }

    pShared->udecHandler = (IUDECHandler*)mpInstance;
    pShared->voutHandler = (IVoutHandler*)mpInstance;

    AM_INFO("CAmbaDspIOne Return\n");
    return mpInstance;
}

CAmbaDspIOne::CAmbaDspIOne(SConsistentConfig* pShared, DSPConfig* pConfig):
    mpSharedRes(pShared),
    mpConfig(pConfig),
    mIavFd(-1),
    mPpMode(2),
    mbUDECMode(false),
    mbTranscodeMode(false),
    mbMdecMode(false),
    mMaxVoutWidth(0),
    mMaxVoutHeight(0),
    mVoutConfigMask(3),
    mVoutStartIndex(0),
    mVoutNumber(0),
    mTotalUDECNumber(1),
    mbDSPMapped(false),
    mbPicParamBeenSet(0),
    mpMutex(NULL)
{
    AM_UINT i = 0;
    for (i=0; i<DMAX_UDEC_INSTANCE_NUM; i++) {
        mDecInfo[i].dec_format = UDEC_NONE;
        mbInstanceUsed[i] = 0;
    }
}

CAmbaDspIOne::~CAmbaDspIOne()
{
    for(int i=0; i<DMAX_UDEC_INSTANCE_NUM; i++){
        if(mbInstanceUsed[i]){
            ReleaseUDECInstance(i);
        }
    }
    if (mIavFd >= 0) {
        AMLOG_INFO("CAmbaDspIOne close(mIavFd).\n");
        close(mIavFd);
    }
}

//for dump
//#define fake_feeding
#ifdef AM_DEBUG
char g_dumpfilePath1[DAMF_MAX_FILENAME_LEN+1];
char g_dumpfilePath2[DAMF_MAX_FILENAME_LEN+1];
char g_dumpfilePath3[DAMF_MAX_FILENAME_LEN+1];
char g_dumpfilePath4[DAMF_MAX_FILENAME_LEN+1];
#endif

AM_ERR CAmbaDspIOne::Construct()
{
//    AM_ERR err;
    DSetModuleLogConfig(LogModuleDSPHandler);
    if ((mpMutex = CMutex::Create()) == NULL) {
        return ME_ERROR;
    }

    if ((mIavFd = open("/dev/iav", O_RDWR, 0)) < 0)
    {
        AM_PERROR("/dev/iav");
        return ME_ERROR;
    }

    mpSharedRes->mIavFd = mIavFd;
    mpSharedRes->mbIavInited = 1;

#ifdef AM_DEBUG
    //load dump file path
    snprintf(g_dumpfilePath1, sizeof(g_dumpfilePath1), "%s/es1.data", AM_GetPath(AM_PathDump));
    snprintf(g_dumpfilePath2, sizeof(g_dumpfilePath2), "%s/es2.data", AM_GetPath(AM_PathDump));
    snprintf(g_dumpfilePath3, sizeof(g_dumpfilePath3), "%s/es3.data", AM_GetPath(AM_PathDump));
    snprintf(g_dumpfilePath4, sizeof(g_dumpfilePath4), "%s/es4.data", AM_GetPath(AM_PathDump));
#endif

    return ME_OK;
}

AM_INT CAmbaDspIOne::getAvaiableUdecinstance()
{
    AM_INT i = 0;
    //for mdec
    if(16 == mpSharedRes->mDSPmode){
        for ( ; i < mTotalUDECNumber -1; i++){
            if (mbInstanceUsed[i] == 0){
                return i;
            }
        }
        if(mpConfig->hdWin == 0){
            if(mbInstanceUsed[mTotalUDECNumber -1] == 0)
                return mTotalUDECNumber -1;
        }
        return eInvalidUdecIndex;
    }
    //end for mdec
    for ( ;i < mTotalUDECNumber; i++) {
        if (mbInstanceUsed[i] == 0) {
            return i;
        }
    }

    return eInvalidUdecIndex;
}

//according to connected vouts
AM_INT CAmbaDspIOne::setupVoutConfig(SConsistentConfig* mpShared)
{
    AM_UINT i = 0;
    AM_INT ret = 0;
    DSPVoutConfig* pvout;

    if (!mpShared || (mpShared->mbIavInited==0) || (mpShared->mIavFd < 0)) {
        AM_ERROR("NULL mpShared pointer, or no iav handle in SetupVoutConfig.\n");
        return (-1);
    }

    mVoutConfigMask = mpShared->pbConfig.vout_config;

    //for each vout, get its parameters
    for (i = 0; i < eVoutCnt; i++) {

        //check the vout is requested
        /*if (!(mVoutConfigMask & (1<<i))) {
            AMLOG_INFO("VOUT id %d not requested, continue.\n", i);
            continue;
        }*/

        pvout = &mpShared->dspConfig.voutConfigs.voutConfig[i];
        ret = getVoutParams(mpShared->mIavFd, i, pvout);
        AMLOG_PRINTF("[VOUT settings] id %d, failed?%d.  size_x %d, size_y %d, pos_x %d, pos_y %d.\n", i, pvout->failed, pvout->size_x, pvout->size_y, pvout->pos_x, pvout->pos_y);
        AMLOG_PRINTF("    width %d, height %d, flip %d, rotate %d.\n", pvout->width, pvout->height, pvout->flip, pvout->rotate);
        if (ret < 0 || pvout->failed || !pvout->width || !pvout->height) {
            mVoutConfigMask &= ~(1<<i);
            AMLOG_ERROR("VOUT id %d failed.\n", i);
            pvout->failed = 1;
            continue;
        }

        if (mVoutConfigMask & (1<<i)) {
            mVoutNumber ++;
            if (pvout->width > mMaxVoutWidth) {
                mMaxVoutWidth = pvout->width;
            }
            if (pvout->height > mMaxVoutHeight) {
                mMaxVoutHeight = pvout->height;
            }
        }
    }

    if ((mVoutConfigMask == 0) || (mVoutNumber == 0)) {
        AM_ASSERT(mVoutConfigMask == 0);
        AM_ASSERT(mVoutNumber == 0);
        AM_ERROR("no VOUT valid.\n");
        return (-2);
    }

    if (mVoutConfigMask & (1<<eVoutLCD)) {
        mVoutStartIndex = eVoutLCD;
    } else if (mVoutConfigMask & (1<<eVoutHDMI)) {
        mVoutStartIndex = eVoutHDMI;
    } else {
        AM_ASSERT(0);
        AM_ERROR("should not comes here, must have bugs.\n");
        return (-3);
    }

    mpShared->dspConfig.voutConfigs.num_vouts = mVoutNumber;
    mpShared->dspConfig.voutConfigs.vout_start_index = mVoutStartIndex;
    mpShared->dspConfig.voutConfigs.vout_mask = mVoutConfigMask;
    mpShared->dspConfig.voutConfigs.pp_max_frm_width = (AM_U16)mMaxVoutWidth;
    mpShared->dspConfig.voutConfigs.pp_max_frm_height = (AM_U16)mMaxVoutHeight;

    AMLOG_PRINTF("[VOUT settings]mVoutStartIndex %d, mVoutNumber %d, mVoutConfigMask 0x%x.\n", mVoutStartIndex, mVoutNumber, mVoutConfigMask);
    AMLOG_PRINTF("[VOUT settings]mMaxVoutWidth %d, mMaxVoutHeight %d.\n", mMaxVoutWidth, mMaxVoutHeight);

    mpShared->pbConfig.vout_config = mVoutConfigMask;

    return 0;
}

AM_INT CAmbaDspIOne::getVoutParams(AM_INT iavFd, AM_INT vout_id, DSPVoutConfig* pConfig)
{
    AM_INT i, num = 0, sink_id = -1, sink_type;
    struct amba_vout_sink_info  sink_info;

    memset(pConfig, 0, sizeof(SVoutConfig));
    pConfig->failed = 1;

    if (::ioctl(iavFd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
        perror("IAV_IOC_VOUT_GET_SINK_NUM");
        return -1;
    }

    if (num < 1) {
        AM_ERROR("Please load vout driver!\n");
        return -1;
    }

    if (vout_id == 0) {
        sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;
    } else {
        sink_type = AMBA_VOUT_SINK_TYPE_HDMI;
    }

    for (i = num - 1; i >= 0; i--) {
        sink_info.id = i;
        if (::ioctl(iavFd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
            perror("IAV_IOC_VOUT_GET_SINK_INFO");
            AM_ERROR("**IAV_IOC_VOUT_GET_SINK_INFO fail!\n");
            return -1;
        }

        if (sink_info.source_id == vout_id &&
            sink_info.sink_type == sink_type) {
            sink_id = sink_info.id;
            break;
        }
    }

    pConfig->failed = 0;
    pConfig->enable = 1;
    pConfig->sink_id = sink_id;
    pConfig->width = sink_info.sink_mode.video_size.vout_width;
    pConfig->height = sink_info.sink_mode.video_size.vout_height;
    pConfig->size_x = sink_info.sink_mode.video_size.video_width;
    pConfig->size_y = sink_info.sink_mode.video_size.video_height;
    pConfig->pos_x = sink_info.sink_mode.video_offset.offset_x;
    pConfig->pos_y = sink_info.sink_mode.video_offset.offset_y;
    AM_PRINTF("vout(%d)'s original position/size, dimention: pos(%d,%d), size(%d,%d), dimention(%d,%d).\n", vout_id, pConfig->pos_x, pConfig->pos_y, pConfig->size_x, pConfig->size_y, pConfig->width, pConfig->height);

    pConfig->flip = (AM_INT)sink_info.sink_mode.video_flip;
    pConfig->rotate = (AM_INT)sink_info.sink_mode.video_rotate;
    AM_PRINTF("vout(%d)'s original flip and rotate state: %d %d.\n", vout_id, pConfig->flip, pConfig->rotate);

    return 0;
}

//to be removed
void CAmbaDspIOne::getDefaultDSPConfig(SConsistentConfig* mpShared)
{
    AM_ASSERT(0);
    AMLOG_ERROR("should not come here, this is debug only, the config should be set by external component.\n");
    AM_ERROR("should not come here, this is debug only, the config should be set by external component.\n");

    AM_UINT i = 0;//mMaxVoutWidth = 0, mMaxVoutHeight = 0,
   /* AM_INT ret = 0;
    AM_UINT mVoutNumber = 0;
    AM_UINT mVoutStartIndex = 0;*/

    if (!mpShared) {
        AM_ERROR("NULL mpShared pointer.\n");
        return;
    }

    memset(&mpShared->dspConfig, 0x0, sizeof(mpShared->dspConfig));

    //dsp mode related
    mpShared->dspConfig.modeConfig.postp_mode = 2;//default, can be set by user
    mpShared->dspConfig.modeConfig.enable_deint = 0;//default, can be set by user
    mpShared->dspConfig.modeConfig.pp_chroma_fmt_max = 2;

    mpShared->dspConfig.modeConfig.pp_max_frm_num = 5;
    //blow need set later
    //mpShared->dspConfig.modeConfig.pp_max_frm_width = 0;
    //mpShared->dspConfig.modeConfig.pp_max_frm_height = 0;
    //mpShared->dspConfig.modeConfig.vout_mask = 0;
    mpShared->dspConfig.modeConfig.num_udecs = 1;

    //de-interlace related, default value
    mpShared->dspConfig.deinterlaceConfig.init_tff = 1;
    mpShared->dspConfig.deinterlaceConfig.deint_lu_en = 1;
    mpShared->dspConfig.deinterlaceConfig.deint_ch_en = 1;
    mpShared->dspConfig.deinterlaceConfig.osd_en = 0;

    mpShared->dspConfig.deinterlaceConfig.deint_mode = 1;
    mpShared->dspConfig.deinterlaceConfig.deint_spatial_shift = 0;
    mpShared->dspConfig.deinterlaceConfig.deint_lowpass_shift = 7;

    mpShared->dspConfig.deinterlaceConfig.deint_lowpass_center_weight = 112;
    mpShared->dspConfig.deinterlaceConfig.deint_lowpass_hor_weight = 2;
    mpShared->dspConfig.deinterlaceConfig.deint_lowpass_ver_weight = 4;

    mpShared->dspConfig.deinterlaceConfig.deint_gradient_bias = 15;
    mpShared->dspConfig.deinterlaceConfig.deint_predict_bias = 15;
    mpShared->dspConfig.deinterlaceConfig.deint_candidate_bias = 10;

    mpShared->dspConfig.deinterlaceConfig.deint_spatial_score_bias = 5;
    mpShared->dspConfig.deinterlaceConfig.deint_temporal_score_bias = 5;

    //for each udec instance, default parameters
    for (i=0; i<DMAX_UDEC_INSTANCE_NUM; i++) {
        mpShared->dspConfig.udecInstanceConfig[i].tiled_mode = 0;
        mpShared->dspConfig.udecInstanceConfig[i].frm_chroma_fmt_max = UDEC_CFG_FRM_CHROMA_FMT_422; // 4:2:0
        mpShared->dspConfig.udecInstanceConfig[i].dec_types = mpSharedRes->codec_mask;
        mpShared->dspConfig.udecInstanceConfig[i].max_frm_num = 8;
        mpShared->dspConfig.udecInstanceConfig[i].max_frm_width = 1920; // todo
        mpShared->dspConfig.udecInstanceConfig[i].max_frm_height = 1088; // todo
        mpShared->dspConfig.udecInstanceConfig[i].max_fifo_size = 2*1024*1024;
        mpShared->dspConfig.udecInstanceConfig[i].concealment_mode = 0;
        mpShared->dspConfig.udecInstanceConfig[i].concealment_ref_frm_buf_id = 0;
    }

}

//--------------------------------------------------
//
//from IUDECHandler
//
//--------------------------------------------------

AM_ERR CAmbaDspIOne::EnterUDECMode()
{
    AUTO_LOCK(mpMutex);

    if (mbUDECMode != false) {
        AMLOG_WARN("CAmbaDspIOne::EnterUdecMode, already in UDEC mode\n");
        return ME_OK;
    }

    if (mbTranscodeMode != false) {
        AMLOG_ERROR("CAmbaDspIOne::EnterUdecMode, already in Transcode mode!!\n");
        return ME_ERROR;
    }

    return enterUdecMode();
}

AM_ERR CAmbaDspIOne::EnterTranscodeMode()
{
    AUTO_LOCK(mpMutex);

    if (mbTranscodeMode != false) {
        AMLOG_WARN("CAmbaDspIOne::EnterTranscodeMode, already in Transcode mode\n");
        return ME_OK;
    }

    if (mbUDECMode != false) {
        AMLOG_ERROR("CAmbaDspIOne::EnterTranscodeMode, already in UDEC mode!!\n");
        return ME_ERROR;
    }

    return enterTranscodeMode();
}

AM_ERR CAmbaDspIOne::ExitUDECMode()
{
    AM_INT state;
    AM_INT i;
    AUTO_LOCK(mpMutex);

    if (!mbUDECMode) {
        AMLOG_WARN("CAmbaDspIOne::ExitUdecMode, not in UDEC mode\n");
        return ME_OK;
    }

    AMLOG_INFO("exit udec mode start\n");
    i = ioctl(mIavFd, IAV_IOC_GET_STATE, &state);
    if (state != IAV_STATE_IDLE) {
        AMLOG_PRINTF("DSP Not in IDLE mode, enter IDLE mode.\n");
        i = ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
        if (i != 0) {
            AMLOG_ERROR("DSP enter IDLE mode fail, ret %d.\n", i);
            return ME_IO_ERROR;
        }
    }

    for (i=0; i<DMAX_UDEC_INSTANCE_NUM; i++) {
        mDecInfo[i].dec_format = UDEC_NONE;
        mbInstanceUsed[i] = 0;
    }

    mbUDECMode = false;
    AMLOG_INFO("exit udec mode done\n");
    return ME_OK;

}

AM_ERR CAmbaDspIOne::ExitTranscodeMode()
{
//    AM_INT state;
    AM_INT i;
    AUTO_LOCK(mpMutex);

    if (!mbTranscodeMode) {
        AMLOG_WARN("CAmbaDspIOne::ExitTranscodeMode, not in Transcode mode\n");
        return ME_OK;
    }
/*
    //can not leave encode mode, enc[] will be released when close iav
    AMLOG_INFO("exit transcode mode start\n");
    i = ioctl(mIavFd, IAV_IOC_GET_STATE, &state);
    if (state != IAV_STATE_IDLE) {
        AMLOG_PRINTF("DSP Not in IDLE mode, enter IDLE mode.\n");
        i = ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
        if (i != 0) {
            AMLOG_ERROR("DSP enter IDLE mode fail, ret %d.\n", i);
            return ME_IO_ERROR;
        }
    }
*/
    for (i=0; i<DMAX_UDEC_INSTANCE_NUM; i++) {
        mDecInfo[i].dec_format = UDEC_NONE;
        mbInstanceUsed[i] = 0;
    }

    mbTranscodeMode = false;
    AMLOG_INFO("exit udec mode done\n");
    return ME_OK;

}

AM_ERR CAmbaDspIOne::EnterMdecMode()
{
    AUTO_LOCK(mpMutex);

    if (mbMdecMode != false) {
        AMLOG_WARN("CAmbaDspIOne::EnterMdecMode, already in MdecMode mode\n");
        return ME_OK;
    }

    if ((mbUDECMode || mbTranscodeMode) != false) {
        AMLOG_ERROR("CAmbaDspIOne::EnterMdecMode, already in other mode!!\n");
        return ME_ERROR;
    }

    return enterMdecMode();
}

AM_ERR CAmbaDspIOne::ExitMdecMode()
{
    AM_INT state;
    AM_INT i;
    AUTO_LOCK(mpMutex);

    if (!mbMdecMode) {
        AMLOG_WARN("CAmbaDspIOne::ExitMdecMode, not in Mdec mode\n");
        return ME_OK;
    }

    AMLOG_INFO("exit MDEC mode start\n");
    i = ioctl(mIavFd, IAV_IOC_GET_STATE, &state);
    if (state != IAV_STATE_IDLE) {
        AMLOG_PRINTF("DSP Not in IDLE mode, enter IDLE mode.\n");
        i = ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
        if (i != 0) {
            AMLOG_ERROR("DSP enter IDLE mode fail, ret %d.\n", i);
            return ME_IO_ERROR;
        }
    }

    for (i=0; i<DMAX_UDEC_INSTANCE_NUM; i++) {
        mDecInfo[i].dec_format = UDEC_NONE;
        mbInstanceUsed[i] = 0;
    }

    mbMdecMode = false;
    AMLOG_INFO("exit mdec mode done\n");
    return ME_OK;

}

void CAmbaDspIOne::setUdecModeConfig()
{
    AM_ASSERT(mpSharedRes);
    memset(&mUdecModeConfig, 0, sizeof(iav_udec_mode_config_t));
    memset(&mUdecDeintConfig, 0, sizeof(iav_udec_deint_config_t));

    AM_ASSERT(mTotalUDECNumber <= DMAX_UDEC_INSTANCE_NUM);
    if (mTotalUDECNumber > DMAX_UDEC_INSTANCE_NUM) {
        AMLOG_ERROR("Request udec number out of range, %d.\n", mTotalUDECNumber);
        mTotalUDECNumber = DMAX_UDEC_INSTANCE_NUM;
    }

    mPpMode = mUdecModeConfig.postp_mode = mpSharedRes->ppmode;
    mpSharedRes->get_outpic = (1==mPpMode);
    AMLOG_PRINTF("[DSP settings] ppmode = %d. \n",mpSharedRes->ppmode);
    mUdecModeConfig.enable_deint = (AM_U8)mpSharedRes->dspConfig.enableDeinterlace;

    mUdecModeConfig.pp_chroma_fmt_max = 2;
    AM_ASSERT(mMaxVoutWidth);
    AM_ASSERT(mMaxVoutHeight);
    AM_ASSERT(mpSharedRes->dspConfig.voutConfigs.pp_max_frm_width == ((AM_U16)mMaxVoutWidth));
    AM_ASSERT(mpSharedRes->dspConfig.voutConfigs.pp_max_frm_height == ((AM_U16)mMaxVoutHeight));
    AMLOG_INFO("mMaxVoutWidth %d, mMaxVoutHeight %d.\n", mMaxVoutWidth, mMaxVoutHeight);
    mUdecModeConfig.pp_max_frm_width = mMaxVoutWidth;
    mUdecModeConfig.pp_max_frm_height = mMaxVoutHeight;

    mUdecModeConfig.pp_max_frm_num = (1 == mpSharedRes->mDSPmode)?4:5;
    AM_ASSERT(mpSharedRes->dspConfig.voutConfigs.vout_mask == mVoutConfigMask);
    mUdecModeConfig.vout_mask = mVoutConfigMask;
    mUdecModeConfig.num_udecs = mTotalUDECNumber;
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
}

void CAmbaDspIOne::setTranscodeModeConfig()
{
    iav_udec_mode_config_t *udec_mode = &mTranscodeModeConfig.udec_mode;
    AM_ASSERT(mpSharedRes);
    memset(&mTranscodeModeConfig, 0, sizeof(mTranscodeModeConfig));
    //memset(&mUdecDeintConfig, 0, sizeof(iav_udec_deint_config_t));

    AM_ASSERT(mTotalUDECNumber <= DMAX_UDEC_INSTANCE_NUM);
    if (mTotalUDECNumber > DMAX_UDEC_INSTANCE_NUM) {
        AMLOG_ERROR("Request udec number out of range, %d.\n", mTotalUDECNumber);
        mTotalUDECNumber = DMAX_UDEC_INSTANCE_NUM;
    }

    mPpMode = udec_mode->postp_mode = mpSharedRes->ppmode;//=3 now
    AM_ASSERT(3==mPpMode);
    mpSharedRes->get_outpic = (3==mPpMode);
    AMLOG_PRINTF("[DSP settings] ppmode = %d. \n",mpSharedRes->ppmode);
    udec_mode->enable_deint = (AM_U8)mpSharedRes->dspConfig.enableDeinterlace;

    udec_mode->pp_chroma_fmt_max = 2;
    AM_ASSERT(mMaxVoutWidth);
    AM_ASSERT(mMaxVoutHeight);
    AM_ASSERT(mpSharedRes->dspConfig.voutConfigs.pp_max_frm_width == ((AM_U16)mMaxVoutWidth));
    AM_ASSERT(mpSharedRes->dspConfig.voutConfigs.pp_max_frm_height == ((AM_U16)mMaxVoutHeight));
    AMLOG_INFO("mMaxVoutWidth %d, mMaxVoutHeight %d.\n", mMaxVoutWidth, mMaxVoutHeight);
    if(((AM_UINT)mMaxVoutWidth>=mpSharedRes->sTranscConfig.dec_w[0]) ||((AM_UINT)mMaxVoutHeight>=mpSharedRes->sTranscConfig.dec_h[0]) ){
        mpSharedRes->sTranscConfig.dec_w[0] = mMaxVoutWidth;
        mpSharedRes->sTranscConfig.dec_h[0] = mMaxVoutHeight;
    }
    udec_mode->pp_max_frm_width = mpSharedRes->sTranscConfig.dec_w[0];
    udec_mode->pp_max_frm_height = mpSharedRes->sTranscConfig.dec_h[0];

    udec_mode->pp_max_frm_num = 4;
    AM_ASSERT(mpSharedRes->dspConfig.voutConfigs.vout_mask == mVoutConfigMask);
    udec_mode->vout_mask = mVoutConfigMask;
    udec_mode->num_udecs = mTotalUDECNumber;
    if (mpSharedRes->dspConfig.enableDeinterlace) {
        udec_mode->enable_deint = 1;

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

        udec_mode->deint_config = &mUdecDeintConfig;
    }
}

void CAmbaDspIOne::setMdecModeConfig()
{
    memset(udec_mode, 0, sizeof(*udec_mode));

    udec_mode->postp_mode = 3;
    udec_mode->enable_deint = 0;
    udec_mode->pp_chroma_fmt_max = 2; // 4:2:2
    udec_mode->enable_error_mode = 0;
    udec_mode->pp_max_frm_width = mMaxVoutWidth;	// vout_width;
    udec_mode->pp_max_frm_height = mMaxVoutHeight;	// vout_height;
    udec_mode->pp_max_frm_num = 5;
    udec_mode->pp_background_Y = 0;
    udec_mode->pp_background_Cb = 128;
    udec_mode->pp_background_Cr = 128;

    udec_mode->vout_mask = mpConfig->voutMask;

    if (mpConfig->voutMask == 0x1) {
        udec_mode->primary_vout = 0;
    } else {
        udec_mode->primary_vout = 1;
    }
    udec_mode->num_udecs = mTotalUDECNumber;
    udec_mode->udec_config = udec_configs;
    mdec_mode.total_num_render_configs = iavRenders.num_configs;
    mdec_mode.total_num_win_configs = iavWindows.num_configs;
    mdec_mode.windows_config = iavWindows.configs;
    mdec_mode.render_config = iavRenders.configs;
    DSPVoutConfig* pvout = NULL;
    pvout = &(mpSharedRes->dspConfig.voutConfigs.voutConfig[udec_mode->primary_vout]);

    if(pvout->rotate == 1){
        //lcd may rotate
        mdec_mode.video_win_height = pvout->width;
        mdec_mode.video_win_width = pvout->height;
    }else{
        if(mpSharedRes->sTranscConfig.enableTranscode){
            mdec_mode.video_win_height = mpSharedRes->sTranscConfig.enc_h;
            mdec_mode.video_win_width = mpSharedRes->sTranscConfig.enc_w;
            AM_INFO("@@@@@@@set postp global size %dx%d.\n",
                mpSharedRes->sTranscConfig.enc_w, mdec_mode.video_win_height);
        }else{
            mdec_mode.video_win_height = pvout->height;
            mdec_mode.video_win_width = pvout->width;
        }
    }

    mdec_mode.frame_rate_in_ticks = 3003;

    mdec_mode.av_sync_enabled = 0;
    if (mpConfig->voutMask & 0x1) {
        mdec_mode.voutA_enabled = 1;
    }
    if (mpConfig->voutMask & 0x2) {
        mdec_mode.voutB_enabled = 1;
    }
    mdec_mode.audio_on_win_id = 0;
    AM_INFO("pre_buffer_len:%d, enable_ctrl:%d\n", mpConfig->pre_buffer_len, mpConfig->enable_dsp_pre_ctrl);
    mdec_mode.pre_buffer_len = mpConfig->pre_buffer_len;
    mdec_mode.enable_buffering_ctrl = mpConfig->enable_dsp_pre_ctrl;

    mpSharedRes->get_outpic = 0;
    mPpMode = 3;
    if(mpSharedRes->sTranscConfig.enableTranscode){
        udec_mode->enable_transcode = 1;
        udec_mode->udec_transcoder_config.total_channel_number = 1;
        udec_mode->udec_transcoder_config.transcoder_config[0].main_type = DSP_ENCRM_TYPE_H264;
        udec_mode->udec_transcoder_config.transcoder_config[0].pip_type = DSP_ENCRM_TYPE_NONE;
        udec_mode->udec_transcoder_config.transcoder_config[0].piv_type = DSP_ENCRM_TYPE_NONE;
        udec_mode->udec_transcoder_config.transcoder_config[0].mctf_flag = 1;
        udec_mode->udec_transcoder_config.transcoder_config[0].encoding_width = mpSharedRes->sTranscConfig.enc_w;
        udec_mode->udec_transcoder_config.transcoder_config[0].encoding_height = mpSharedRes->sTranscConfig.enc_h;
        udec_mode->udec_transcoder_config.transcoder_config[0].pip_encoding_width = 0;
        udec_mode->udec_transcoder_config.transcoder_config[0].pip_encoding_height = 0;
    }
    AMLOG_PRINTF("[DSP settings] ppmode = %d. \n",mPpMode);
}

void CAmbaDspIOne::setMdecRenders(udec_render_t *render, int nr)
{
    int i = 0, win = 0;
    AM_INT sdSource = mTotalUDECNumber - (mpConfig->hdWin ? 1 : 0);
    memset(render, 0, sizeof(udec_render_t) * nr);
    //AM_ASSERT(nr == mTotalUDECNumber);
    if(mpConfig->hdWin == 1 && 0){
        //use trick render to solve hd hided issue.
        render->render_id = 0;
        render->win_config_id = mTotalUDECNumber -1;
        render->win_config_id_2nd = 0xFF;
        render->udec_id = mTotalUDECNumber -1;
        render++;
        i++;
    }

    for (; i < nr; win++, i++, render++) {
        AM_INFO("setMdecRenders render:%d, total render:%d, vaild sd num:%d\n", i, nr, sdSource);
        if(i < sdSource){
            render->render_id = i;
            render->win_config_id = win;
            render->win_config_id_2nd = 0xFF;//hard code
            render->udec_id = win;
            render->input_source_type = MUDEC_INPUT_SRC_UDEC;
        }else{
            render->render_id = i;
            render->win_config_id = win;
            render->win_config_id_2nd = 0xFF;
            render->udec_id = 0xFF;
            render->input_source_type = MUDEC_INPUT_SRC_BACKGROUND_COLOR;
        }
    }
}

void CAmbaDspIOne::updateSecondVoutSize()
{
    if (udec_mode->vout_mask != 0x3) {
        //no need to config second vout size
        return;
    }

    int second_vout = udec_mode->primary_vout? 0 : 1;
    DSPVoutConfig* pvout = NULL;
    pvout = &(mpSharedRes->dspConfig.voutConfigs.voutConfig[second_vout]);
    AM_ASSERT(pvout);
//    int w_greater_h = (pvout->width > pvout->height)? 1 : 0;

    if (mpConfig->second_vout_scale_mode  == 1) {
        mpSharedRes->dspConfig.voutConfigs.second_vout_width = pvout->width;
        mpSharedRes->dspConfig.voutConfigs.second_vout_height = pvout->height;
    } /*else if (mpConfig->second_vout_scale_mode  == 0) {
        //TODO
        mpSharedRes->dspConfig.voutConfigs.second_vout_width = pvout->width;
        mpSharedRes->dspConfig.voutConfigs.second_vout_height = pvout->height;
    } */else if (mpConfig->second_vout_scale_mode  == 2) {
        //check rotation
        //
        if (mpSharedRes->dspConfig.voutConfigs.second_vout_width > pvout->width || mpSharedRes->dspConfig.voutConfigs.second_vout_width <= 0) {
            mpSharedRes->dspConfig.voutConfigs.second_vout_width = pvout->width;
        }
        if (mpSharedRes->dspConfig.voutConfigs.second_vout_height > pvout->height || mpSharedRes->dspConfig.voutConfigs.second_vout_height <= 0) {
            mpSharedRes->dspConfig.voutConfigs.second_vout_height = pvout->height;
        }
    }

    if(/*mpConfig->second_vout_scale_mode == 0 || */mpConfig->second_vout_scale_mode == 2){
        iav_vout_change_video_size_t video_size;
        memset(&video_size, 0, sizeof(video_size));
        video_size.vout_id = pvout->sink_id;
        video_size.width = mpSharedRes->dspConfig.voutConfigs.second_vout_width;
        video_size.height = mpSharedRes->dspConfig.voutConfigs.second_vout_height;
        if (ioctl(mIavFd, IAV_IOC_VOUT_CHANGE_VIDEO_SIZE, &video_size) < 0) {
            perror("IAV_IOC_VOUT_CHANGE_VIDEO_SIZE");
        }
    }
    return;
}

static AM_INT __display_layout(udec_window_t *window, AM_INT nr, AM_INT display_width, AM_INT display_height, AM_U8 layout)
{
#define SD_H_GAP 40
#define SD_V_GAP 40

    if (6 == layout) {
        //telepresense
        AM_INT rindex = 0;
        AM_INT win_id = 0;
        AM_INT rect_w, rect_h;

        if (!display_width || !display_height || (nr <= 0)) {
            AM_ERROR("__telepresence_layout: zero input!!\n");
            return (-1);
        }

        // config fullscreen first
        window->win_config_id = win_id;
        window->input_offset_x = 0;
        window->input_offset_y = 0;
        window->input_width = 0;
        window->input_height = 0;
        window->target_win_offset_x = 0;
        window->target_win_offset_y = 0;
        window->target_win_width = display_width;
        window->target_win_height = display_height;
        win_id++;
        window ++;

        // config average
        AM_INT left_stream_cnt = nr - 1;
        if (left_stream_cnt > 0) {
            rect_w = (display_width - (left_stream_cnt + 1)*SD_H_GAP)/left_stream_cnt;
            rect_h = (display_height - (left_stream_cnt + 1)*SD_V_GAP)/left_stream_cnt;
            rect_w = (rect_w>>2)<<2;
        }

        for (rindex = 0; rindex < left_stream_cnt; rindex++) {
            window->win_config_id = win_id;
            window->input_offset_x = 0;
            window->input_offset_y = 0;
            window->input_width = 0;
            window->input_height = 0;
            window->target_win_offset_x = (rect_w+SD_H_GAP)*rindex + SD_H_GAP;
            window->target_win_offset_y = (rect_h+SD_V_GAP)*(left_stream_cnt-1) + SD_V_GAP;
            window->target_win_width = rect_w;
            window->target_win_height = rect_h;
            win_id++;
            window ++;
        }
    } else {
        AM_ERROR("add implement here\n");
        return (-2);
    }

    return 0;
}

void CAmbaDspIOne::setMdecWindows(udec_window_t *window, AM_INT nr, AM_U8 enable_different_layout, AM_U8 display_layout)
{
    AM_INT voutA_width = 0;
    AM_INT voutA_height = 0;
    AM_INT voutB_width = 0;
    AM_INT voutB_height = 0;
    AM_INT vout_w = 0, vout_h = 0;
    AM_INT voutA_rotate = 0, voutB_rotate = 0;
//    AM_U32 vout_mask = 2;
    AM_INT rows;
    AM_INT cols;
    AM_INT rindex;
    AM_INT cindex;
    AM_INT i;
    DSPVoutConfig* pvout;

    //LCD info
    pvout = &(mpSharedRes->dspConfig.voutConfigs.voutConfig[0]);
    voutA_width = pvout->width;
    voutA_height = pvout->height;
    voutA_rotate = pvout->rotate;
    AM_INFO("VoutA: vout w:%d, vout h:%d, rotate:%d\n", voutA_width, voutA_height, voutA_rotate);
    pvout = &(mpSharedRes->dspConfig.voutConfigs.voutConfig[1]);
    voutB_width = pvout->width;
    voutB_height = pvout->height;
    voutB_rotate = pvout->rotate;
    AM_INFO("VoutB: vout w:%d, vout h:%d, rotate:%d\n", voutB_width, voutB_height,voutB_rotate);

    AM_INT smallNr = nr-1;
    if (smallNr == 1) {
        rows = 1;
        cols = 1;
    } else if (smallNr <= 4) {
        rows = 2;
        cols = 2;
    } else if (smallNr <= 6) {
        rows = 2;
        cols = 3;
    } else if (smallNr <= 9){
        rows = 3;
        cols = 3;
    } else {
        rows = 4;
        cols = 4;
    }

    if(voutB_width == 0 || voutB_height == 0)
    {
        AM_ASSERT(0);
        voutB_width = 1920;
        voutB_height = 1080;
    }
    memset(window, 0, sizeof(udec_window_t) * nr);

    if(mpConfig->voutMask == 0x1){
        //voutA is primary vout
        vout_w = (voutA_rotate == 0) ?  voutA_width : voutA_height;
        vout_h = (voutA_rotate == 0) ?  voutA_height : voutA_width;
    }else{
        //voutB is primary vout
        vout_w = (voutB_rotate == 0) ?  voutB_width : voutB_height;
        vout_h = (voutB_rotate == 0) ?  voutB_height : voutB_width;
    }

    //relayout on GeneralLayoutManager, here only set table layout
    if (enable_different_layout && 0) {
        if (0 == __display_layout(window, nr, vout_w, vout_h, display_layout)) {
            return;
        } else {
            AM_ERROR("customized layout(%d) fail or not supported\n", display_layout);
        }
    }

    AM_INFO("WinConfig: vout w:%d, vout h:%d\n", vout_w, vout_h);
    rindex = 0;
    cindex = 0;
    for (i = 0; i < smallNr; i++, window++) {
        AM_INFO("Set SD windows:%d of %d\n", i +1, smallNr);
        window->win_config_id = i;
        window->input_offset_x = 0;
        window->input_offset_y = 0;
        window->input_width = 0;
        window->input_height = 0;

        int w = (vout_w/cols) & (~0x3);
        if(w*cols == vout_w) {
            window->target_win_offset_x = cindex * vout_w / cols;
            window->target_win_offset_y = rindex * vout_h / rows;
            window->target_win_width = vout_w / cols;
            window->target_win_height = vout_h / rows;
        }else{
            window->target_win_offset_x = cindex * vout_w / cols
                -((((w + 4)*cols-vout_w)*rindex/cols)&(~0x1));
            window->target_win_offset_y = rindex * vout_h / rows;
            window->target_win_width = w+4;
            window->target_win_height = vout_h / rows;
        }

        if (++cindex == cols) {
            cindex = 0;
            rindex++;
        }
    }
    //set big windows
    ///*
    if(smallNr != nr){
        AM_INFO("Set HD window:%d\n", nr);
        window->win_config_id = smallNr;
        window->input_offset_x = 0;
        window->input_offset_y = 0;
        window->input_width = 0;
        window->input_height = 0;

        window->target_win_offset_x = 0;
        window->target_win_offset_y = 0;
        window->target_win_width = vout_w;
        window->target_win_height = vout_h;
    }
    //*/
}

void CAmbaDspIOne::setUdecConfig(AM_INT index)
{
    AM_ASSERT(index >=0 && index < mTotalUDECNumber);
    if (index < 0 || index >= mTotalUDECNumber) {
        AMLOG_ERROR("invalid udec index %d in setUdecConfig.\n", index);
        return;
    }

    memset(&mUdecConfig[index], 0, sizeof(iav_udec_config_t));

    //hard code here
    mUdecConfig[index].tiled_mode = mpConfig->preset_tilemode;
    mUdecConfig[index].frm_chroma_fmt_max = UDEC_CFG_FRM_CHROMA_FMT_422; // 4:2:0
    mUdecConfig[index].dec_types = mpSharedRes->codec_mask;
    mUdecConfig[index].max_frm_num = ((1 == mpSharedRes->mDSPmode)||(3 == mpSharedRes->mDSPmode))?10:8;
    mUdecConfig[index].max_frm_width = 1920; // todo
    mUdecConfig[index].max_frm_height = 1088; // todo
    mUdecConfig[index].max_fifo_size = 2*1024*1024;
}

void CAmbaDspIOne::setTranscConfig(AM_INT index)
{
    AM_ASSERT(index >=0 && index < mTotalUDECNumber);
    if (index < 0 || index >= mTotalUDECNumber) {
        AMLOG_ERROR("invalid udec index %d in setTranscConfig.\n", index);
        return;
    }

    memset(&mUdecConfig[index], 0, sizeof(iav_udec_config_t));

    //hard code here
    mUdecConfig[index].tiled_mode = mpConfig->preset_tilemode;
    mUdecConfig[index].frm_chroma_fmt_max = UDEC_CFG_FRM_CHROMA_FMT_422; // 4:2:0
    mUdecConfig[index].dec_types = 0x3F;
    mUdecConfig[index].max_frm_num = 8;
    mUdecConfig[index].max_frm_width = 1920;
        //(0==mpSharedRes->sTranscConfig.dec_w[index])?mpSharedRes->sTranscConfig.dec_w[index]:mMaxVoutWidth;
    mUdecConfig[index].max_frm_height = 1088;
        //(0==mpSharedRes->sTranscConfig.dec_h[index])?mpSharedRes->sTranscConfig.dec_h[index]:mMaxVoutHeight;
    mUdecConfig[index].max_fifo_size = 2*1024*1024;
}

AM_ERR CAmbaDspIOne::InitUDECInstance(AM_INT& wantUdecIndex, DSPConfig*pConfig, void* pHybirdAcc, AM_INT decFormat, AM_INT pic_width, AM_INT pic_height, AM_INT extEdge, bool autoAllocIndex)
{
    if(3==mpSharedRes->mDSPmode){
        return InitTranscodeInstance(wantUdecIndex, pConfig, pHybirdAcc, decFormat, pic_width, pic_height, extEdge, autoAllocIndex);
    }

    if(16==mpSharedRes->mDSPmode){
        return InitMdecInstance(wantUdecIndex, pConfig, pHybirdAcc, decFormat, pic_width, pic_height, extEdge, autoAllocIndex);
    }

    AUTO_LOCK(mpMutex);
    AM_INT index, i;
    amba_decoding_accelerator_t* pAcc = (amba_decoding_accelerator_t*)pHybirdAcc;

    if (!pConfig) {
        AMLOG_ERROR("NULL pConfig in CAmbaDspIOne::InitUDECInstance.\n");
        wantUdecIndex = eInvalidUdecIndex;
        return ME_BAD_PARAM;
    }

    AM_ASSERT(mpSharedRes);

    if (mbUDECMode == false) {
        if (EnterUDECMode() != ME_OK) {
            wantUdecIndex = eInvalidUdecIndex;
            return ME_OS_ERROR;
        }
    }

    if (!autoAllocIndex) {
        if (wantUdecIndex <0 || wantUdecIndex>=mTotalUDECNumber) {
            AMLOG_ERROR("Bad wantUdecIndex in CAmbaDspIOne::InitUDECInstance.\n");
            wantUdecIndex = eInvalidUdecIndex;
            return ME_BAD_PARAM;
        }
        if (mbInstanceUsed[wantUdecIndex]) {
            AMLOG_ERROR("Udec instance(%d) already in use. CAmbaDspIOne::InitUDECInstance.\n", wantUdecIndex);
            wantUdecIndex = eInvalidUdecIndex;
            return ME_BUSY;
        }
    }

    //debug only assertion
    AM_ASSERT(pConfig == mpConfig);
    AM_ASSERT(pConfig == (&mpSharedRes->dspConfig));

    // find a avaiable udec instance
    if (mbInstanceUsed[wantUdecIndex]) {
        wantUdecIndex = index = getAvaiableUdecinstance();
        if (index == eInvalidUdecIndex) {
            AMLOG_ERROR("No avaible Udec instance in CAmbaDspIOne::InitUDECInstance.\n");
            wantUdecIndex = eInvalidUdecIndex;
            return ME_BUSY;
        }
    } else {
        index = wantUdecIndex;
    }

    //try initialize the udec instance
    AM_ASSERT(index>=0);
    AM_ASSERT(index<mTotalUDECNumber);

    memset(&mUdecInfo[index], 0, sizeof(mUdecInfo[index]));
    memset(&mUdecVoutConfig[0], 0, eVoutCnt * sizeof(iav_udec_vout_config_t));
    mDecInfo[index].dec_format = decFormat;
    mDecInfo[index].ext_edge = extEdge;
    mDecInfo[index].pic_width = pic_width;
    mDecInfo[index].pic_height = pic_height;

    mUdecInfo[index].udec_id = index;
    mUdecInfo[index].enable_err_handle = mpSharedRes->dspConfig.errorHandlingConfig[index].enable_udec_error_handling;
    mUdecInfo[index].udec_type = mDecInfo[index].dec_format;
    mUdecInfo[index].enable_pp = 1;
    mUdecInfo[index].enable_deint = mUdecModeConfig.enable_deint;
    mUdecInfo[index].interlaced_out = 0;
    mUdecInfo[index].out_chroma_format = (1 == mpSharedRes->mDSPmode) ? 1:0;//out_chroma_format &packed_out control the out fmt
    mUdecInfo[index].packed_out = 0;
    if(mpSharedRes->force_decode){
        mUdecInfo[index].other_flags |= IAV_UDEC_FORCE_DECODE;
    }
    if(mpSharedRes->validation_only){//designed for video editing
        mUdecInfo[index].other_flags |= IAV_UDEC_VALIDATION_ONLY;
    }
    if (mUdecInfo[index].enable_err_handle) {
        AM_ASSERT(mpSharedRes->dspConfig.errorHandlingConfig[index].enable_udec_error_handling);
        mUdecInfo[index].concealment_mode = mpSharedRes->dspConfig.errorHandlingConfig[index].error_concealment_mode;
        mUdecInfo[index].concealment_ref_frm_buf_id = mpSharedRes->dspConfig.errorHandlingConfig[index].error_concealment_frame_id;
        AMLOG_PRINTF("enable udec error handling: concealment mode %d, frame id %d.\n",mUdecInfo[index].concealment_mode, mUdecInfo[index].concealment_ref_frm_buf_id);
    }

    AM_ASSERT(mVoutNumber == pConfig->voutConfigs.num_vouts);
    AM_ASSERT(mVoutStartIndex == pConfig->voutConfigs.vout_start_index);
    AM_ASSERT(mVoutConfigMask == pConfig->voutConfigs.vout_mask);
    //pConfig->voutConfigs.input_center_x = mDecInfo[index].pic_width / 2;
    //pConfig->voutConfigs.input_center_y = mDecInfo[index].pic_height / 2;

    //set source rect
    if (extEdge) {
        mpConfig->voutConfigs.picture_offset_x = 16;
        mpConfig->voutConfigs.picture_offset_y = 16;
    } else {
        mpConfig->voutConfigs.picture_offset_x = 0;
        mpConfig->voutConfigs.picture_offset_y = 0;
    }

    //source rect related
    mpConfig->voutConfigs.picture_width = mDecInfo[index].pic_width;
    mpConfig->voutConfigs.picture_height = mDecInfo[index].pic_height;
    mpConfig->voutConfigs.src_pos_x = mpConfig->voutConfigs.picture_offset_x;
    mpConfig->voutConfigs.src_pos_y = mpConfig->voutConfigs.picture_offset_y;
    mpConfig->voutConfigs.src_size_x = mpConfig->voutConfigs.picture_width;
    mpConfig->voutConfigs.src_size_y = mpConfig->voutConfigs.picture_height;

    if(mpSharedRes->mDSPmode == 0){
        mUdecInfo[index].vout_configs.num_vout = pConfig->voutConfigs.num_vouts;
        mUdecInfo[index].vout_configs.vout_config = &mUdecVoutConfig[pConfig->voutConfigs.vout_start_index];
    }else{
        mUdecInfo[index].vout_configs.num_vout = 1;//pConfig->voutConfigs.num_vouts;
        mUdecInfo[index].vout_configs.vout_config = (index==0)?&mUdecVoutConfig[0]:&mUdecVoutConfig[1];//&mUdecVoutConfig[pConfig->voutConfigs.vout_start_index];
    }
    mUdecInfo[index].vout_configs.input_center_x = (mpConfig->voutConfigs.src_size_x/2) + mpConfig->voutConfigs.src_pos_x;
    mUdecInfo[index].vout_configs.input_center_y = (mpConfig->voutConfigs.src_size_y/2) + mpConfig->voutConfigs.src_pos_y;
    mpConfig->voutConfigs.input_center_x = mUdecInfo[index].vout_configs.input_center_x;
    mpConfig->voutConfigs.input_center_y = mUdecInfo[index].vout_configs.input_center_y;
    //default zoom factor
    for (i = 0; i< eVoutCnt; i ++) {
        mpConfig->voutConfigs.voutConfig[i].zoom_factor_x = 1;
        mpConfig->voutConfigs.voutConfig[i].zoom_factor_y = 1;
    }
    AMLOG_PRINTF("dec_format %d, pic_width %d, pic_height %d.\n", mDecInfo[index].dec_format, mDecInfo[index].pic_width, mDecInfo[index].pic_height);

    mUdecInfo[index].bits_fifo_size = 4*1024*1024;
    mUdecInfo[index].ref_cache_size = 0;

    switch (decFormat) {
        case UDEC_H264:
            mUdecInfo[index].u.h264.pjpeg_buf_size = 4*1024*1024;
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
            mUdecInfo[index].u.jpeg.still_max_decode_width = mDecInfo[index].pic_width;
            mUdecInfo[index].u.jpeg.still_max_decode_height = mDecInfo[index].pic_height;
            break;

        case UDEC_VC1:
            break;

        case UDEC_MP4S:
        case UDEC_RV40:
            if (!pAcc) {
                AMLOG_ERROR("NULL pAcc in CAmbaDspIOne::InitUDECInstance.\n");
                return ME_BAD_PARAM;
            }

            memset((void*)pAcc, 0x0, sizeof(amba_decoding_accelerator_t));
            pAcc->amba_iav_fd = mIavFd;

            //for safe here, force 16 byte align
            pAcc->amba_picture_width = (pic_width+ 15)&(~15);//for ring buffer, pic size should align to MB(16x16)
            pAcc->amba_picture_height = (pic_height+ 15)&(~15);

            pAcc->amba_picture_real_width = pic_width;
            pAcc->amba_picture_real_height = pic_height;
            pAcc->amba_idct_buffer_size = (pAcc->amba_picture_width>>4) * (pAcc->amba_picture_height>>4) * sizeof(mb_idctcoef_t);

            if (decFormat == UDEC_RV40) {
                mapDSP(index);
                pAcc->amba_buffer_number = 2;
                pAcc->amba_mv_buffer_size = (pAcc->amba_picture_width>>4) * (pAcc->amba_picture_height>>4) * sizeof(RVDEC_MBINFO_t);

                mUdecInfo[index].bits_fifo_size = (pAcc->amba_idct_buffer_size + pAcc->amba_picture_width *48)*pAcc->amba_buffer_number;
                //mUdecInfo[index].bits_fifo_size = pAcc->amba_idct_buffer_size*pAcc->amba_buffer_number;
                mUdecInfo[index].u.rv40.mv_fifo_size = pAcc->amba_mv_buffer_size * pAcc->amba_buffer_number;
            } else if (decFormat == UDEC_MP4S) {
                //amba_buffer_number read from ppmod_config, if invalid, set default 2
                int buffer_number=mpSharedRes->pbConfig.input_buffer_number;
                if(buffer_number<=2)
                    buffer_number = 2;
                else if(buffer_number>4)//for 1080p, chunk number cannot bigger than 4, or dsp will assertion in memory operation
                    buffer_number = 4;
                pAcc->amba_buffer_number = buffer_number;
                pAcc->amba_mv_buffer_size = CHUNKHEAD_VOPINFO_SIZE + (pAcc->amba_picture_width>>4) * (pAcc->amba_picture_height>>4) * sizeof(mb_mv_B_t);
                pAcc->amba_mv_buffer_size = (pAcc->amba_mv_buffer_size+31)&(~31);

                mUdecInfo[index].u.mp4s.deblocking_flag = mpSharedRes->dspConfig.deblockingFlag;
                mUdecInfo[index].u.mp4s.pquant_mode = mpSharedRes->dspConfig.deblockingConfig.pquant_mode;
                for(i=0; i<32; i++ )
                {
                    mUdecInfo[index].u.mp4s.pquant_table[i] = (AM_U8)mpSharedRes->dspConfig.deblockingConfig.pquant_table[i];
                }
                AM_PRINTF("UDEC_MP4S deblocking_flag %d, pquant_mode %d.\n", mUdecInfo[index].u.mp4s.deblocking_flag, mUdecInfo[index].u.mp4s.pquant_mode);
                for (i = 0; i<4; i++) {
                    AM_PRINTF(" pquant_table[%d - %d]:\n", i*8, i*8+7);
                    AM_PRINTF(" %d, %d, %d, %d, %d, %d, %d, %d.\n", \
                        mUdecInfo[index].u.mp4s.pquant_table[i*8], mUdecInfo[index].u.mp4s.pquant_table[i*8+1], mUdecInfo[index].u.mp4s.pquant_table[i*8+2], mUdecInfo[index].u.mp4s.pquant_table[i*8+3], \
                        mUdecInfo[index].u.mp4s.pquant_table[i*8+4], mUdecInfo[index].u.mp4s.pquant_table[i*8+5], mUdecInfo[index].u.mp4s.pquant_table[i*8+6], mUdecInfo[index].u.mp4s.pquant_table[i*8+7] \
                    );
                }
                mUdecInfo[index].bits_fifo_size = pAcc->amba_idct_buffer_size*pAcc->amba_buffer_number;

                mUdecInfo[index].u.mp4s.mv_fifo_size = pAcc->amba_mv_buffer_size * pAcc->amba_buffer_number;
            }

            AM_PRINTF("amba_buffer_number %d, amba_mv_buffer_size=%d.\n",pAcc->amba_buffer_number, pAcc->amba_mv_buffer_size);

            break;

        case UDEC_SW:
            mUdecInfo[index].bits_fifo_size = 0;
            mapDSP(index);
            break;

        default:
            AMLOG_ERROR("udec type %d not implemented\n", mDecInfo[index].dec_format);
            return ME_BAD_PARAM;
    }
    if(1 == mPpMode)
        mapDSP(index);

    //set initial vout related settings
    for (i = 0; i< eVoutCnt; i ++) {
        mUdecVoutConfig[i].vout_id = i;
        mUdecVoutConfig[i].target_win_width = pConfig->voutConfigs.voutConfig[i].size_x;
        mUdecVoutConfig[i].target_win_height = pConfig->voutConfigs.voutConfig[i].size_y;
        mUdecVoutConfig[i].target_win_offset_x = pConfig->voutConfigs.voutConfig[i].pos_x;
        mUdecVoutConfig[i].target_win_offset_y = pConfig->voutConfigs.voutConfig[i].pos_y;
        mUdecVoutConfig[i].win_width = pConfig->voutConfigs.voutConfig[i].size_x;
        mUdecVoutConfig[i].win_height = pConfig->voutConfigs.voutConfig[i].size_y;
        mUdecVoutConfig[i].win_offset_x = pConfig->voutConfigs.voutConfig[i].pos_x;
        mUdecVoutConfig[i].win_offset_y = pConfig->voutConfigs.voutConfig[i].pos_y;
        mUdecVoutConfig[i].zoom_factor_x = pConfig->voutConfigs.voutConfig[i].zoom_factor_x;
        mUdecVoutConfig[i].zoom_factor_y = pConfig->voutConfigs.voutConfig[i].zoom_factor_y;
        mUdecVoutConfig[i].flip = pConfig->voutConfigs.voutConfig[i].flip;
        mUdecVoutConfig[i].rotate = pConfig->voutConfigs.voutConfig[i].rotate;
        mUdecVoutConfig[i].disable = pConfig->voutConfigs.voutConfig[i].enable ? 0 : 1;
    }

    //AM_ASSERT(mpSharedRes->mUdecState == _UDEC_IDLE);


    if(3==mpSharedRes->mDSPmode){
        AMLOG_PRINTF("start IAV_IOC_INIT_UDEC2 [%d]....\n",index);
        if (ioctl(mIavFd, IAV_IOC_INIT_UDEC2, &mUdecInfo[index]) < 0) {
            perror("IAV_IOC_INIT_UDEC2");
            wantUdecIndex = eInvalidUdecIndex;
            return ME_OS_ERROR;
        }
        AMLOG_PRINTF("IAV_IOC_INIT_UDEC2 [%d] done.\n",index);
    }
    else{
        AMLOG_PRINTF("start IAV_IOC_INIT_UDEC [%d]....\n",index);
        if (ioctl(mIavFd, IAV_IOC_INIT_UDEC, &mUdecInfo[index]) < 0) {
            perror("IAV_IOC_INIT_UDEC");
            wantUdecIndex = eInvalidUdecIndex;
            return ME_OS_ERROR;
        }
        AMLOG_PRINTF("IAV_IOC_INIT_UDEC [%d] done.\n",index);
    }

    if (decFormat == UDEC_RV40 || decFormat == UDEC_MP4S) {
        AM_ASSERT(mUdecInfo[index].bits_fifo_start);
        AM_ASSERT(mUdecInfo[index].mv_fifo_start);

        if (mUdecInfo[index].bits_fifo_start && mUdecInfo[index].mv_fifo_start) {
            //map bit stream and mv stream buffer
            pAcc->p_amba_idct_ringbuffer = mUdecInfo[index].bits_fifo_start;
            pAcc->p_amba_mv_ringbuffer = mUdecInfo[index].mv_fifo_start;
            pAcc->decode_id = mUdecInfo[index].udec_id;
            pAcc->decode_type = mUdecInfo[index].udec_type;
            if (1==mUdecModeConfig.postp_mode && decFormat == UDEC_MP4S) {
                pAcc->does_render_by_filter = 1;
            }
        } else {
            AM_ASSERT(0);
            AMLOG_ERROR("init Hybird UDEC, input buffer NULL, idct/coef %p, mv %p.\n", mUdecInfo[index].bits_fifo_start, mUdecInfo[index].mv_fifo_start);
            return ME_NO_MEMORY;
        }
    }

    pConfig->udecInstanceConfig[index].pbits_fifo_start = mUdecInfo[index].bits_fifo_start;
    pConfig->udecInstanceConfig[index].bits_fifo_size = mUdecInfo[index].bits_fifo_size;
    AMLOG_INFO("UDEC[%d], bits_fifo_start = %p, size = 0x%x.\n",index, mUdecInfo[index].bits_fifo_start, mUdecInfo[index].bits_fifo_size);

    wantUdecIndex = index;
    mbInstanceUsed[index] = 1;

    mbPicParamBeenSet = 1;
    return ME_OK;
}

AM_ERR CAmbaDspIOne::InitTranscodeInstance(AM_INT& wantUdecIndex, DSPConfig*pConfig, void* pHybirdAcc, AM_INT decFormat, AM_INT pic_width, AM_INT pic_height, AM_INT extEdge, bool autoAllocIndex)
{
    AUTO_LOCK(mpMutex);
    AM_INT index, i;
//    amba_decoding_accelerator_t* pAcc = (amba_decoding_accelerator_t*)pHybirdAcc;

    if (!pConfig) {
        AMLOG_ERROR("NULL pConfig in CAmbaDspIOne::InitTranscodeInstance.\n");
        wantUdecIndex = eInvalidUdecIndex;
        return ME_BAD_PARAM;
    }

    AM_ASSERT(mpSharedRes);

    if (mbTranscodeMode == false) {
        if (EnterTranscodeMode() != ME_OK) {
            wantUdecIndex = eInvalidUdecIndex;
            return ME_OS_ERROR;
        }
    }


    if (!autoAllocIndex) {
        if (wantUdecIndex <0 || wantUdecIndex>=mTotalUDECNumber) {
            AMLOG_ERROR("Bad wantUdecIndex in CAmbaDspIOne::InitTranscodeInstance.\n");
            wantUdecIndex = eInvalidUdecIndex;
            return ME_BAD_PARAM;
        }
        if (mbInstanceUsed[wantUdecIndex]) {
            AMLOG_ERROR("Udec instance(%d) already in use. CAmbaDspIOne::InitTranscodeInstance.\n", wantUdecIndex);
            wantUdecIndex = eInvalidUdecIndex;
            return ME_BUSY;
        }
    }

    //debug only assertion
    AM_ASSERT(pConfig == mpConfig);
    AM_ASSERT(pConfig == (&mpSharedRes->dspConfig));

    // find a avaiable udec instance
    if (mbInstanceUsed[wantUdecIndex]) {
        wantUdecIndex = index = getAvaiableUdecinstance();
        if (index == eInvalidUdecIndex) {
            AMLOG_ERROR("No avaible Udec instance in CAmbaDspIOne::InitTranscodeInstance.\n");
            wantUdecIndex = eInvalidUdecIndex;
            return ME_BUSY;
        }
    } else {
        index = wantUdecIndex;
    }

    //try initialize the udec instance
    AM_ASSERT(index>=0);
    AM_ASSERT(index<mTotalUDECNumber);

    memset(&mTranscodeInfo[index], 0, sizeof(mTranscodeInfo[index]));
    memset(&mUdecVoutConfig[0], 0, eVoutCnt * sizeof(iav_udec_vout_config_t));
    mDecInfo[index].dec_format = decFormat;
    mDecInfo[index].ext_edge = extEdge;
    mDecInfo[index].pic_width = pic_width;
    mDecInfo[index].pic_height = pic_height;

    mTranscodeInfo[index].udec_id = index;
    mTranscodeInfo[index].enable_err_handle = mpSharedRes->dspConfig.errorHandlingConfig[index].enable_udec_error_handling;
    mTranscodeInfo[index].udec_type = mDecInfo[index].dec_format;
    mTranscodeInfo[index].enable_pp = 1;
    mTranscodeInfo[index].enable_deint = mTranscodeModeConfig.udec_mode.enable_deint;
    mTranscodeInfo[index].interlaced_out = 0;
    mTranscodeInfo[index].out_chroma_format = mpSharedRes->sTranscConfig.chroma_fmt;//out_chroma_format &packed_out control the out fmt
    mTranscodeInfo[index].packed_out = 0;
    if(mpSharedRes->force_decode){
        mTranscodeInfo[index].other_flags |= IAV_UDEC_FORCE_DECODE;
    }
    if(mpSharedRes->validation_only){//designed for video editing
        mTranscodeInfo[index].other_flags |= IAV_UDEC_VALIDATION_ONLY;
    }
    if (mTranscodeInfo[index].enable_err_handle) {
        AM_ASSERT(mpSharedRes->dspConfig.errorHandlingConfig[index].enable_udec_error_handling);
        mTranscodeInfo[index].concealment_mode = mpSharedRes->dspConfig.errorHandlingConfig[index].error_concealment_mode;
        mTranscodeInfo[index].concealment_ref_frm_buf_id = mpSharedRes->dspConfig.errorHandlingConfig[index].error_concealment_frame_id;
        AMLOG_PRINTF("enable udec error handling: concealment mode %d, frame id %d.\n",mTranscodeInfo[index].concealment_mode, mTranscodeInfo[index].concealment_ref_frm_buf_id);
    }

    AM_ASSERT(mVoutNumber == pConfig->voutConfigs.num_vouts);
    AM_ASSERT(mVoutStartIndex == pConfig->voutConfigs.vout_start_index);
    AM_ASSERT(mVoutConfigMask == pConfig->voutConfigs.vout_mask);
    //pConfig->voutConfigs.input_center_x = mDecInfo[index].pic_width / 2;
    //pConfig->voutConfigs.input_center_y = mDecInfo[index].pic_height / 2;

    //set source rect
    if (extEdge) {
        mpConfig->voutConfigs.picture_offset_x = 16;
        mpConfig->voutConfigs.picture_offset_y = 16;
    } else {
        mpConfig->voutConfigs.picture_offset_x = 0;
        mpConfig->voutConfigs.picture_offset_y = 0;
    }

    //source rect related
    mpConfig->voutConfigs.picture_width = mDecInfo[index].pic_width;
    mpConfig->voutConfigs.picture_height = mDecInfo[index].pic_height;
    mpConfig->voutConfigs.src_pos_x = mpConfig->voutConfigs.picture_offset_x;
    mpConfig->voutConfigs.src_pos_y = mpConfig->voutConfigs.picture_offset_y;
    mpConfig->voutConfigs.src_size_x = mpConfig->voutConfigs.picture_width;
    mpConfig->voutConfigs.src_size_y = mpConfig->voutConfigs.picture_height;

    mTranscodeInfo[index].vout_configs.num_vout = 1;//pConfig->voutConfigs.num_vouts;
    mTranscodeInfo[index].vout_configs.vout_config = (index==0)?&mUdecVoutConfig[1]:&mUdecVoutConfig[0];//&mUdecVoutConfig[pConfig->voutConfigs.vout_start_index];

    mTranscodeInfo[index].vout_configs.input_center_x = (mpConfig->voutConfigs.src_size_x/2) + mpConfig->voutConfigs.src_pos_x;
    mTranscodeInfo[index].vout_configs.input_center_y = (mpConfig->voutConfigs.src_size_y/2) + mpConfig->voutConfigs.src_pos_y;
    mpConfig->voutConfigs.input_center_x = mTranscodeInfo[index].vout_configs.input_center_x;
    mpConfig->voutConfigs.input_center_y = mTranscodeInfo[index].vout_configs.input_center_y;

    mUdecConfig[index].max_frm_width =
        ((AM_UINT)mMaxVoutWidth>=mpSharedRes->sTranscConfig.dec_w[index])?mMaxVoutWidth:mpSharedRes->sTranscConfig.dec_w[index];
    mUdecConfig[index].max_frm_height =
        ((AM_UINT)mMaxVoutHeight>=mpSharedRes->sTranscConfig.dec_h[index])?mMaxVoutHeight:mpSharedRes->sTranscConfig.dec_h[index];
    // decpp config related
    mTranscodeInfo[index].input_center_x = ((AM_UINT)mMaxVoutWidth>=mpSharedRes->sTranscConfig.dec_w[index])?
        mMaxVoutWidth/2:mpSharedRes->sTranscConfig.dec_w[index]/2;
    mTranscodeInfo[index].input_center_y = ((AM_UINT)mMaxVoutHeight>=mpSharedRes->sTranscConfig.dec_h[index])?
        mMaxVoutHeight/2:mpSharedRes->sTranscConfig.dec_h[index]/2;
    mTranscodeInfo[index].out_win_offset_x = 0;
    mTranscodeInfo[index].out_win_offset_y = 0;
    mTranscodeInfo[index].out_win_width = ((AM_UINT)mMaxVoutWidth>=mpSharedRes->sTranscConfig.dec_w[index])?
        mMaxVoutWidth:mpSharedRes->sTranscConfig.dec_w[index];
    mTranscodeInfo[index].out_win_height = ((AM_UINT)mMaxVoutHeight>=mpSharedRes->sTranscConfig.dec_h[index])?
        mMaxVoutHeight:mpSharedRes->sTranscConfig.dec_h[index];

    //default zoom factor
    for (i = 0; i< eVoutCnt; i ++) {
        mpConfig->voutConfigs.voutConfig[i].zoom_factor_x = 1;
        mpConfig->voutConfigs.voutConfig[i].zoom_factor_y = 1;
    }
    AMLOG_PRINTF("dec_format %d, pic_width %d, pic_height %d.\n", mDecInfo[index].dec_format, mDecInfo[index].pic_width, mDecInfo[index].pic_height);

    mTranscodeInfo[index].bits_fifo_size = 4*1024*1024;
    mTranscodeInfo[index].ref_cache_size = 0;

    switch (decFormat) {
        case UDEC_H264:
            mTranscodeInfo[index].u.h264.pjpeg_buf_size = 4*1024*1024;
            break;

        case UDEC_MP12:
        case UDEC_MP4H:
            mTranscodeInfo[index].u.mpeg.deblocking_flag = mpSharedRes->dspConfig.deblockingFlag;
/*            mTranscodeInfo[index].u.mpeg.pquant_mode = mpSharedRes->dspConfig.deblockingConfig.pquant_mode;
            for(i=0; i<32; i++ )
            {
                mTranscodeInfo[index].u.mpeg.pquant_table[i] = (AM_U8)mpSharedRes->dspConfig.deblockingConfig.pquant_table[i];
            }
            mTranscodeInfo[index].u.mpeg.is_avi_flag = mpSharedRes->is_avi_flag;
            AMLOG_INFO("MPEG12/4 deblocking_flag %d, pquant_mode %d.\n", mTranscodeInfo[index].u.mpeg.deblocking_flag, mTranscodeInfo[index].u.mpeg.pquant_mode);
            for (i = 0; i<4; i++) {
                AMLOG_INFO(" pquant_table[%d - %d]:\n", i*8, i*8+7);
                AMLOG_INFO(" %d, %d, %d, %d, %d, %d, %d, %d.\n", \
                    mTranscodeInfo[index].u.mpeg.pquant_table[i*8], mTranscodeInfo[index].u.mpeg.pquant_table[i*8+1], mTranscodeInfo[index].u.mpeg.pquant_table[i*8+2], mTranscodeInfo[index].u.mpeg.pquant_table[i*8+3], \
                    mTranscodeInfo[index].u.mpeg.pquant_table[i*8+4], mTranscodeInfo[index].u.mpeg.pquant_table[i*8+5], mTranscodeInfo[index].u.mpeg.pquant_table[i*8+6], mTranscodeInfo[index].u.mpeg.pquant_table[i*8+7] \
                );
            }*/
            AMLOG_INFO("MPEG4 is_avi_flag %d.\n", mTranscodeInfo[index].u.mpeg.is_avi_flag);
            break;

        case UDEC_JPEG:
            mTranscodeInfo[index].u.jpeg.still_bits_circular = 0;
            mTranscodeInfo[index].u.jpeg.still_max_decode_width = mDecInfo[index].pic_width;
            mTranscodeInfo[index].u.jpeg.still_max_decode_height = mDecInfo[index].pic_height;
            break;

        case UDEC_VC1:
            break;

        case UDEC_MP4S:
        case UDEC_RV40:

            break;

        case UDEC_SW:
            mTranscodeInfo[index].bits_fifo_size = 0;
            mapDSP(index);
            break;

        default:
            AMLOG_ERROR("udec type %d not implemented\n", mDecInfo[index].dec_format);
            return ME_BAD_PARAM;
    }
    if(1 == mPpMode)
        mapDSP(index);

    //set initial vout related settings
    for (i = 0; i< eVoutCnt; i ++) {
        mUdecVoutConfig[i].vout_id = i;
        mUdecVoutConfig[i].target_win_width = pConfig->voutConfigs.voutConfig[i].size_x;
        mUdecVoutConfig[i].target_win_height = pConfig->voutConfigs.voutConfig[i].size_y;
        mUdecVoutConfig[i].target_win_offset_x = pConfig->voutConfigs.voutConfig[i].pos_x;
        mUdecVoutConfig[i].target_win_offset_y = pConfig->voutConfigs.voutConfig[i].pos_y;
        mUdecVoutConfig[i].win_width = pConfig->voutConfigs.voutConfig[i].size_x;
        mUdecVoutConfig[i].win_height = pConfig->voutConfigs.voutConfig[i].size_y;
        mUdecVoutConfig[i].win_offset_x = pConfig->voutConfigs.voutConfig[i].pos_x;
        mUdecVoutConfig[i].win_offset_y = pConfig->voutConfigs.voutConfig[i].pos_y;
        mUdecVoutConfig[i].zoom_factor_x = pConfig->voutConfigs.voutConfig[i].zoom_factor_x;
        mUdecVoutConfig[i].zoom_factor_y = pConfig->voutConfigs.voutConfig[i].zoom_factor_y;
        mUdecVoutConfig[i].flip = pConfig->voutConfigs.voutConfig[i].flip;
        mUdecVoutConfig[i].rotate = pConfig->voutConfigs.voutConfig[i].rotate;
        mUdecVoutConfig[i].disable = pConfig->voutConfigs.voutConfig[i].enable ? 0 : 1;
    }

    //AM_ASSERT(mpSharedRes->mUdecState == _UDEC_IDLE);

    AMLOG_PRINTF("start IAV_IOC_INIT_UDEC2 [%d]....\n",index);
    if (ioctl(mIavFd, IAV_IOC_INIT_UDEC2, &mTranscodeInfo[index]) < 0) {
        perror("IAV_IOC_INIT_UDEC2");
        wantUdecIndex = eInvalidUdecIndex;
        return ME_OS_ERROR;
    }
    AMLOG_PRINTF("IAV_IOC_INIT_UDEC2 [%d] done.\n",index);

    if (decFormat == UDEC_RV40 || decFormat == UDEC_MP4S) {
            AM_ASSERT(0);
            AMLOG_ERROR("init Hybird UDEC, input buffer NULL, idct/coef %p, mv %p.\n", mTranscodeInfo[index].bits_fifo_start, mTranscodeInfo[index].mv_fifo_start);
            return ME_NO_MEMORY;
    }

    pConfig->udecInstanceConfig[index].pbits_fifo_start = mTranscodeInfo[index].bits_fifo_start;
    pConfig->udecInstanceConfig[index].bits_fifo_size = mTranscodeInfo[index].bits_fifo_size;
    AMLOG_INFO("UDEC[%d], bits_fifo_start = %p, size = 0x%x.\n",index, mTranscodeInfo[index].bits_fifo_start, mTranscodeInfo[index].bits_fifo_size);

    wantUdecIndex = index;
    mbInstanceUsed[index] = 1;

    mbPicParamBeenSet = 1;
    return ME_OK;
}

AM_ERR CAmbaDspIOne::InitMdecInternal(AM_INT index, DSPConfig*pConfig, AM_INT decFormat, AM_INT pic_width, AM_INT pic_height, AM_INT extEdge)
{
    AM_INT i = 0;

    memset(&mUdecInfo[index], 0, sizeof(mUdecInfo[index]));
    memset(&mUdecVoutConfig[0], 0, eVoutCnt * sizeof(iav_udec_vout_config_t));
    mDecInfo[index].dec_format = decFormat;
    mDecInfo[index].ext_edge = extEdge;
    mDecInfo[index].pic_width = pic_width;
    mDecInfo[index].pic_height = pic_height;

    mUdecInfo[index].udec_id = index;
    mUdecInfo[index].udec_type = mDecInfo[index].dec_format;
    mUdecInfo[index].enable_pp = 1;
    mUdecInfo[index].enable_deint = mUdecModeConfig.enable_deint;
    mUdecInfo[index].interlaced_out = 0;
    mUdecInfo[index].out_chroma_format = 0;//out_chroma_format &packed_out control the out fmt
    mUdecInfo[index].packed_out = 0;
    //mUdecInfo[index].enable_err_handle = mpSharedRes->dspConfig.errorHandlingConfig[index].enable_udec_error_handling;
    mUdecInfo[index].enable_err_handle = 1;
    //mUdecInfo[index].vout_configs.num_vout = 1;
    //mUdecInfo[index].vout_configs.vout_config = &mUdecVoutConfig[1];
    mUdecInfo[index].vout_configs.num_vout = pConfig->voutConfigs.num_vouts;
    mUdecInfo[index].vout_configs.vout_config = &mUdecVoutConfig[pConfig->voutConfigs.vout_start_index];
    AM_INFO("vout_configs.num_vout= %u, vout_start_index=%u.\n",
        mUdecInfo[index].vout_configs.num_vout, pConfig->voutConfigs.vout_start_index);
    mUdecInfo[index].vout_configs.first_pts_low = 0;
    mUdecInfo[index].vout_configs.first_pts_high = 0;
    mUdecInfo[index].bits_fifo_size = 2*1024*1024;
    mUdecInfo[index].ref_cache_size = 0;
    if (mUdecInfo[index].enable_err_handle && 0) {
        mUdecInfo[index].concealment_mode = mpSharedRes->dspConfig.errorHandlingConfig[index].error_concealment_mode;
        mUdecInfo[index].concealment_ref_frm_buf_id = mpSharedRes->dspConfig.errorHandlingConfig[index].error_concealment_frame_id;
        AMLOG_PRINTF("enable udec error handling: concealment mode %d, frame id %d.\n",mUdecInfo[index].concealment_mode, mUdecInfo[index].concealment_ref_frm_buf_id);
    }

    //set initial vout related settings
    for (i = 0; i< eVoutCnt; i ++) {
        mUdecVoutConfig[i].vout_id = i;
        mUdecVoutConfig[i].target_win_width = pConfig->voutConfigs.voutConfig[i].size_x;
        mUdecVoutConfig[i].target_win_height = pConfig->voutConfigs.voutConfig[i].size_y;
        mUdecVoutConfig[i].target_win_offset_x = 0;
        mUdecVoutConfig[i].target_win_offset_y = 0;
        mUdecVoutConfig[i].zoom_factor_x = 1;
        mUdecVoutConfig[i].zoom_factor_y = 1;
        mUdecVoutConfig[i].disable = 0;
    }

    switch (decFormat) {
        case UDEC_H264:
            mUdecInfo[index].u.h264.pjpeg_buf_size = 4*1024*1024;
            break;

        case UDEC_MP12:
        case UDEC_MP4H:
            mUdecInfo[index].u.mpeg.deblocking_flag = 0;
            break;

        case UDEC_JPEG:
            mUdecInfo[index].u.jpeg.still_bits_circular = 0;
            mUdecInfo[index].u.jpeg.still_max_decode_width = mDecInfo[index].pic_width;
            mUdecInfo[index].u.jpeg.still_max_decode_height = mDecInfo[index].pic_height;
            break;

        case UDEC_VC1:
            break;

        case UDEC_SW:
            break;

        default:
            AMLOG_ERROR("udec type %d not implemented\n", mDecInfo[index].dec_format);
            return ME_BAD_PARAM;
    }

    AMLOG_INFO("start IAV_IOC_INIT_UDEC [%d]....\n",index);
    AMLOG_INFO("\tudec_id\t%u\n",mUdecInfo[index].udec_id);
    AMLOG_INFO("\tudec_type\t%u\n",mUdecInfo[index].udec_type);
    AMLOG_INFO("\tenable_pp\t%u\n",mUdecInfo[index].enable_pp);
    AMLOG_INFO("\tenable_deint\t%u\n",mUdecInfo[index].enable_deint);
    AMLOG_INFO("\tinterlaced_out\t%u\n",mUdecInfo[index].interlaced_out);
    AMLOG_INFO("\tenable_err_handle\t%u\n",mUdecInfo[index].enable_err_handle);
    AMLOG_INFO("\tpacked_out\t%u\n",mUdecInfo[index].packed_out);
    AMLOG_INFO("\tother_flags\t%u\n",mUdecInfo[index].other_flags);
    AMLOG_INFO("\tnum_vout\t%u\n",mUdecInfo[index].vout_configs.num_vout);
    AMLOG_INFO("\tbits_fifo_size\t%u\n",mUdecInfo[index].bits_fifo_size);
    AMLOG_INFO("\tbits_fifo_start\t%p\n",mUdecInfo[index].bits_fifo_start);
    AMLOG_INFO("\tmv_fifo_start\t%p\n",mUdecInfo[index].mv_fifo_start);
    AM_INFO("IAV_IOC_INIT_UDEC[%d]...\n", index);
    if (ioctl(mIavFd, IAV_IOC_INIT_UDEC, &mUdecInfo[index]) < 0) {
        perror("IAV_IOC_INIT_UDEC");
        return ME_OS_ERROR;
    }
    AMLOG_INFO("IAV_IOC_INIT_UDEC [%d] done.\n",index);

    pConfig->udecInstanceConfig[index].pbits_fifo_start = mUdecInfo[index].bits_fifo_start;
    pConfig->udecInstanceConfig[index].bits_fifo_size = mUdecInfo[index].bits_fifo_size;

    mbInstanceUsed[index] = 1;
    mbPicParamBeenSet = 1;

    return ME_OK;
}


AM_ERR CAmbaDspIOne::InitMdecInstance(AM_INT& wantUdecIndex, DSPConfig*pConfig, void* pHybirdAcc, AM_INT decFormat, AM_INT pic_width, AM_INT pic_height, AM_INT extEdge, bool autoAllocIndex)
{
    //Init everything for Mdec, 4sd: same as the first sd, hd:as hd.
    AUTO_LOCK(mpMutex);
    AM_INT index, i;
    AM_INT num;
    AM_ERR err = ME_OK;

    if (!pConfig) {
        AMLOG_ERROR("NULL pConfig in CAmbaDspIOne::InitMdecInstance.\n");
        wantUdecIndex = eInvalidUdecIndex;
        return ME_BAD_PARAM;
    }

    AM_ASSERT(mpSharedRes);

    if (mbMdecMode == false) {
        if (EnterMdecMode() != ME_OK) {
            wantUdecIndex = eInvalidUdecIndex;
            return ME_OS_ERROR;
        }
    }

    // find a avaiable udec instance
    if (!autoAllocIndex){
        if (wantUdecIndex <0 || wantUdecIndex>=mTotalUDECNumber){
            AMLOG_ERROR("Bad wantUdecIndex in CAmbaDspIOne::InitMdecInstance.\n");
            wantUdecIndex = eInvalidUdecIndex;
            return ME_BAD_PARAM;
        }
        if(pConfig->curInstanceIsHd){
            AM_ASSERT(wantUdecIndex == mTotalUDECNumber -1);
        }else{
            if(mbInstanceUsed[wantUdecIndex]){
                AM_INFO("InitMdecInstance %d OK\n", wantUdecIndex);
                return ME_OK;
            }
        }
        index = wantUdecIndex;
    }else{
        AM_ASSERT(0);
        AM_ASSERT(pConfig->curInstanceIsHd != -1);
        if(pConfig->curInstanceIsHd){
            index = mTotalUDECNumber - 1;
        }else{
            index = getAvaiableUdecinstance();
        }
        if(index == eInvalidUdecIndex) {
            AMLOG_ERROR("No avaible Udec instance in CAmbaDspIOne::InitMdecInstance.\n");
            wantUdecIndex = eInvalidUdecIndex;
            return ME_BUSY;
        }
        if(mbInstanceUsed[index]){
            AM_ASSERT(0);
            return ME_BUSY;
        }
        wantUdecIndex = index;
    }

    //debug only assertion
    AM_ASSERT(pConfig == mpConfig);
    AM_ASSERT(pConfig == (&mpSharedRes->dspConfig));

    //try initialize the udec instance
    AM_ASSERT(index>=0);
    AM_ASSERT(index<mTotalUDECNumber);

    if(pConfig->curInstanceIsHd ||mTotalUDECNumber == 1){
        err = InitMdecInternal(index, pConfig, decFormat, pic_width, pic_height, extEdge);
    }else{
        //All sd
        if(pConfig->hdWin == 1)
            num = mTotalUDECNumber -1;
        else
            num = mTotalUDECNumber;

        for(i = 0; i < num; i++){
            if(mbInstanceUsed[i] != 1)
                err = InitMdecInternal(i, pConfig, decFormat, pic_width, pic_height, extEdge);
        }
    }

    return err;
}

AM_ERR CAmbaDspIOne::StartUdec(AM_INT udecIndex, AM_UINT format, void* pacc)
{
    AUTO_LOCK(mpMutex);
    amba_decoding_accelerator_t* pAcc = (amba_decoding_accelerator_t*)pacc;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AMLOG_ERROR("Bad wantUdecIndex in CAmbaDspIOne::StartUdec, index %d, mTotalUDECNumber %d.\n", udecIndex, mTotalUDECNumber);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(mDecInfo[udecIndex].dec_format == format);
    AM_ASSERT(UDEC_SW == format || UDEC_RV40 == format);
    if (mDecInfo[udecIndex].dec_format != format) {
        AMLOG_ERROR("Bad format in CAmbaDspIOne::StartUdec, mDecInfo[udecIndex].dec_format %d, format %d.\n", mDecInfo[udecIndex].dec_format, format);
        return ME_BAD_PARAM;
    }

    //start udec, for vout
    if (mDecInfo[udecIndex].dec_format == UDEC_SW || mDecInfo[udecIndex].dec_format == UDEC_RV40) {
        iav_udec_decode_t info;
        memset(&info, 0, sizeof(info));
        info.udec_type = format;
        info.decoder_id = udecIndex;
        info.num_pics = 0;

        if (format == UDEC_RV40) {
            AM_ASSERT(pAcc);
            if (pAcc) {
                info.u.rv40.pic_coding_type = 1;//I picture, fake for start udec.
                info.u.rv40.residual_fifo_start = pAcc->p_amba_idct_ringbuffer;
                info.u.rv40.residual_fifo_end = pAcc->p_amba_idct_ringbuffer + pAcc->amba_idct_buffer_size;
                info.u.rv40.mv_fifo_start = pAcc->p_amba_mv_ringbuffer;
                info.u.rv40.mv_fifo_end = pAcc->p_amba_mv_ringbuffer + pAcc->amba_mv_buffer_size;
                info.u.rv40.pic_width = pAcc->amba_picture_width;
                info.u.rv40.pic_height = pAcc->amba_picture_height;
            } else {
                AMLOG_ERROR("NULL pacc in StartUdec(RV40).\n");
                return ME_BAD_PARAM;
            }
        }

        if(3==mpSharedRes->mDSPmode){
            AMLOG_PRINTF("** first start IAV_IOC_UDEC_DECODE2.\n");
            if (ioctl(mIavFd, IAV_IOC_UDEC_DECODE2, &info) < 0) {
                perror("IAV_IOC_UDEC_DECODE2");
                AMLOG_ERROR("** first IAV_IOC_UDEC_DECODE2 error.\n");
            }
            AMLOG_PRINTF("** first IAV_IOC_UDEC_DECODE2, done.\n");
        }
        else{
            AMLOG_PRINTF("** first start IAV_IOC_UDEC_DECODE.\n");
            if (ioctl(mIavFd, IAV_IOC_UDEC_DECODE, &info) < 0) {
                perror("IAV_IOC_UDEC_DECODE");
                AMLOG_ERROR("** first IAV_IOC_UDEC_DECODE error.\n");
            }
            AMLOG_PRINTF("** first IAV_IOC_UDEC_DECODE, done.\n");
        }
    }
    return ME_OK;
}

AM_ERR CAmbaDspIOne::ReleaseUDECInstance(AM_INT udecIndex)
{

    AUTO_LOCK(mpMutex);
    AM_INFO("ReleaseUDECInstance: %d.\n", udecIndex);
    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AMLOG_ERROR("Bad wantUdecIndex in CAmbaDspIOne::InitUDECInstance, index %d, mTotalUDECNumber %d.\n", udecIndex, mTotalUDECNumber);
        return ME_BAD_PARAM;
    }

    if (mbInstanceUsed[udecIndex] == 0) {
        AMLOG_ERROR("Udec instance(%d) not in use. CAmbaDspIOne::ReleaseUDECInstance.\n", udecIndex);
        return ME_NOT_EXIST;
    }

    AM_ASSERT(mpSharedRes);

    AMLOG_PRINTF("start ReleaseUdec %d...\n", udecIndex);
    if(3==mpSharedRes->mDSPmode){
        if (ioctl(mIavFd, IAV_IOC_RELEASE_UDEC2, udecIndex) < 0) {
            perror("IAV_IOC_DESTROY_UDEC2");
            AMLOG_ERROR("IAV_IOC_RELEASE_UDEC2 %d fail.\n", udecIndex);
            return ME_OS_ERROR;
        }
    }else{
        if (ioctl(mIavFd, IAV_IOC_RELEASE_UDEC, udecIndex) < 0) {
            perror("IAV_IOC_DESTROY_UDEC");
            AMLOG_ERROR("IAV_IOC_RELEASE_UDEC %d fail.\n", udecIndex);
            return ME_OS_ERROR;
        }
    }
    //AM_INFO("end ReleaseUdec\n");

    mbInstanceUsed[udecIndex] = 0;

    for(int i=0; i<DMAX_UDEC_INSTANCE_NUM; i++){
        if(mbInstanceUsed[i]){
            return ME_OK;
        }else{
            //AMLOG_INFO("All udec instances are released.\n");
            continue;
        }
    }

    if(mbDSPMapped){
        if(unMapDSP(udecIndex)!=ME_OK){
            AMLOG_ERROR("CAmbaVideoDecoder: UnMapDSP[%d] Failed!\n", udecIndex);
        }
    }

    if(3==mpSharedRes->mDSPmode){
        ExitTranscodeMode();
    }else if(16==mpSharedRes->mDSPmode){
        ExitMdecMode();
    }else{
        ExitUDECMode();
    }
    AM_INFO("ReleaseUDECInstance Done.\n");
    return ME_CLOSED;
}


AM_ERR CAmbaDspIOne::SetUdecNums(AM_INT udecNums)
{
    if(mbUDECMode)
        return ME_TOO_MANY;
    mTotalUDECNumber = udecNums;
    AMLOG_PRINTF("SetUdecNums %d...\n", udecNums);
    return ME_OK;
}

AM_ERR CAmbaDspIOne::InitMdecTranscoder(AM_INT width, AM_INT height, AM_INT bitrate)
{
    AM_INT ret;
    AM_ERR err = ME_OK;
    iav_udec_init_transcoder_t transcode;
    memset(&transcode, 0x0, sizeof(transcode));

    transcode.flags = 0;
    transcode.id = 0;//hardcode here
    transcode.stream_type = STREAM_TYPE_FULL_RESOLUTION;
    transcode.profile_idc = 0; //0/100 FREXT_HP, 66, base line, 77 main
    transcode.level_idc = 0;

    transcode.encoding_width = width;
    transcode.encoding_height = height;
    transcode.source_window_width = 1280;//hardcode here
    transcode.source_window_height = 720;//hardcode here

    transcode.source_window_offset_x = 0;
    transcode.source_window_offset_y = 0;

    transcode.num_mbrows_per_bitspart = 0;
    transcode.M = 1;
    transcode.N = 30;
    transcode.idr_interval = 1;
    transcode.gop_structure = 0;
    transcode.numRef_P = 1;
    transcode.numRef_B = 2;
    transcode.use_cabac = 1;
    transcode.quality_level = 0x83;

    transcode.average_bitrate = bitrate;

    transcode.vbr_setting = 1;
    transcode.calibration = 0;

    ret = ioctl(mIavFd, IAV_IOC_UDEC_INIT_TRANSCODER, &transcode);
    if (ret < 0) {
        AM_ERROR("!!!!!IAV_IOC_UDEC_INIT_TRANSCODER error, ret %d.\n", ret);
        err = ME_ERROR;
    }
    AM_INFO("IAV_IOC_UDEC_INIT_TRANSCODER done, bits_fifo_start %p, bits_fifo_size %d.\n", (void*)transcode.bits_fifo_start, transcode.bits_fifo_size);

    return err;
}

AM_ERR CAmbaDspIOne::ReleaseMdecTranscoder()
{
    AM_INT ret;
    AM_ERR err = ME_OK;

    AM_INFO("before IAV_IOC_UDEC_RELEASE_TRANSCODER.\n");
    ret = ioctl(mIavFd, IAV_IOC_UDEC_RELEASE_TRANSCODER, 0);
    if (ret < 0) {
        AM_ERROR("!!!!!IAV_IOC_UDEC_RELEASE_TRANSCODER error, ret %d.\n", ret);
        err = ME_ERROR;
    }
    AM_INFO("IAV_IOC_UDEC_RELEASE_TRANSCODER done\n");

    return err;
}

AM_ERR CAmbaDspIOne::StartMdecTranscoder()
{
    AM_INT ret;
    AM_ERR err = ME_OK;
    iav_udec_start_transcoder_t start;

    memset(&start, 0x0, sizeof(start));
    start.transcoder_id = 0;

    AM_INFO("before IAV_IOC_UDEC_START_TRANSCODER.\n");
    ret = ioctl(mIavFd, IAV_IOC_UDEC_START_TRANSCODER, &start);
    if (ret < 0) {
        AM_ERROR("!!!!!IAV_IOC_UDEC_START_TRANSCODER error, ret %d.\n", ret);
        err = ME_ERROR;
    }
    AM_INFO("IAV_IOC_UDEC_START_TRANSCODER done\n");

    return err;
}

AM_ERR CAmbaDspIOne::StopMdecTranscoder()
{
    AM_INT ret;
    AM_ERR err = ME_OK;
    iav_udec_stop_transcoder_t stop;
    memset(&stop, 0x0, sizeof(stop));

    stop.transcoder_id = 0;
    stop.stop_flag = 0;

    AM_INFO("before IAV_IOC_UDEC_STOP_TRANSCODER.\n");
    ret = ioctl(mIavFd, IAV_IOC_UDEC_STOP_TRANSCODER, &stop);
    if (ret < 0) {
        AM_ERROR("!!!!!IAV_IOC_UDEC_STOP_TRANSCODER error, ret %d.\n", ret);
        err = ME_ERROR;
    }
    AM_INFO("IAV_IOC_UDEC_STOP_TRANSCODER done\n");

    return err;
}

AM_ERR CAmbaDspIOne::MdecTranscoderReadBits(void* bitInfo)
{
    int ret;
    AM_ERR err = ME_OK;

    memset(bitInfo, 0x0, sizeof(iav_udec_transcoder_bs_info_t));

    ret = ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_READ_BITS, (iav_udec_transcoder_bs_info_t*)bitInfo);
    if (ret < 0) {
        if ((-EBUSY) == errno) {
            AM_ERROR("IAV_IOC_UDEC_TRANSCODER_READ_BITS, read last packet done.\n");
            err = ME_BUSY;
        }
        AM_ERROR("!!!!!IAV_IOC_UDEC_TRANSCODER_READ_BITS error, ret %d, error %d.\n", ret, errno);
        err = ME_ERROR;
    }

    return err;
}

AM_ERR CAmbaDspIOne::MdecTranscoderSetBitrate(AM_INT kbps)
{
    iav_udec_transcoder_update_bitrate_t bitrate_t;
    memset((void*)&bitrate_t, 0x0, sizeof(bitrate_t));

    bitrate_t.id = 0;
    bitrate_t.average_bitrate = kbps*1000;
    bitrate_t.pts_to_change_bitrate = 0;

    AM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE bitrate %dkbps\n", kbps);
    if (ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE, &bitrate_t) < 0) {
        AM_ERROR("IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE");
        return ME_ERROR;
    }
    AM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_BITRATE done\n");
    return ME_OK;
}

AM_ERR CAmbaDspIOne::MdecTranscoderSetFramerate(AM_INT fps, AM_INT reduction)
{
    iav_udec_transcoder_update_framerate_t framerate_t;
    memset((void*)&framerate_t, 0x0, sizeof(framerate_t));

    framerate_t.id = 0;
    framerate_t.framerate_reduction_factor = reduction;
    framerate_t.framerate_code = fps;

    AM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE %d %d.\n", fps, reduction);
    if (ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE, &framerate_t) < 0) {
        AM_ERROR("IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE");
        return ME_ERROR;
    }
    AM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_FRAMERATE done\n");

    return ME_OK;
}

AM_ERR CAmbaDspIOne::MdecTranscoderSetGOP(AM_INT M, AM_INT N, AM_INT interval, AM_INT structure)
{
    iav_udec_transcoder_update_gop_t gop;
    memset((void*)&gop, 0x0, sizeof(gop));

    gop.id = 0;
    gop.change_gop_option = 2;//hard code, next I/P
    gop.follow_gop = 0;
    gop.fgop_max_N = 0;
    gop.fgop_min_N = 0;
    gop.M = M;
    gop.N = N;
    gop.idr_interval = interval;
    gop.gop_structure = structure;
    gop.pts_to_change = 0xffffffffffffffffLL;//hard code

    AM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_GOP_STRUCTURE M %d, N %d, idr_interval %d, gop_structure %d\n", M, N, interval, structure);
    if (ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_UPDATE_GOP_STRUCTURE, &gop) < 0) {
        AM_ERROR("IAV_IOC_DUPLEX_UPDATE_GOP_STRUCTURE");
        return ME_ERROR;
    }
    AM_INFO("IAV_IOC_UDEC_TRANSCODER_UPDATE_GOP_STRUCTURE done\n");

    return ME_OK;
}

AM_ERR CAmbaDspIOne::MdecTranscoderDemandIDR(AM_BOOL now)
{
    iav_udec_transcoder_demand_idr_t demand_t;
    memset((void*)&demand_t, 0x0, sizeof(demand_t));

    demand_t.id = 0;
    demand_t.on_demand_idr = now?0:1;
    demand_t.pts_to_change = 0;

    AM_INFO("IAV_IOC_UDEC_TRANSCODER_DEMAND_IDR on_demand_idr %d\n", now);
    if (ioctl(mIavFd, IAV_IOC_UDEC_TRANSCODER_DEMAND_IDR, &demand_t) < 0) {
        AM_ERROR("IAV_IOC_UDEC_TRANSCODER_DEMAND_IDR");
        return ME_ERROR;
    }
    AM_INFO("IAV_IOC_UDEC_TRANSCODER_DEMAND_IDR done\n");

    return ME_OK;
}

AM_ERR CAmbaDspIOne::RequestInputBuffer(AM_INT udecIndex, AM_UINT& size, AM_U8* pStart, AM_INT bufferCnt)
{
    iav_wait_decoder_t wait;
    AM_INT ret;
    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::RequestInputBuffer.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    wait.emptiness.room = size + 1024;//for safe
    wait.emptiness.start_addr = pStart;
    wait.num_decoded_frames = bufferCnt;
    wait.flags = IAV_WAIT_BITS_FIFO;
/*    if(1 == mPpMode)
        wait.flags |= IAV_WAIT_OUTPIC;
*/
    wait.decoder_id = udecIndex;

    AMLOG_DEBUG("request start.\n");
    if(3==mpSharedRes->mDSPmode){
        if ((ret = ::ioctl(mIavFd, IAV_IOC_WAIT_DECODER2, &wait)) < 0) {
            perror("IAV_IOC_WAIT_DECODER2");
            AMLOG_ERROR("!!!!!IAV_IOC_WAIT_DECODER2 error, ret %d.\n", ret);
            if (ret == (-EPERM)) {
                /*if (isUDECErrorState(udecIndex)) {
                    return ME_UDEC_ERROR;
                }*/
                return ME_ERROR;
            }
        }
    }else{
        if ((ret = ::ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait)) < 0) {
            if(errno == EAGAIN){
                AM_INFO("IAV_IOC_WAIT_DECODER EINTR %d.\n", udecIndex);
                return ME_BUSY;
            }
            perror("IAV_IOC_WAIT_DECODER");
            AMLOG_ERROR("!!!!!IAV_IOC_WAIT_DECODER error, ret %d.\n", ret);
            if (ret == (-EPERM)) {
                if (isUDECErrorState(udecIndex)) {
                    return ME_UDEC_ERROR;
                }
                return ME_ERROR;
            }
        }

        //Add this part, to avoid decoder filter is blocked when request input buffer
        size = wait.emptiness.room;
        if((16==mpSharedRes->mDSPmode) && (wait.emptiness.room < 800000)){
            return ME_BUSY;
        }
    }
    AMLOG_DEBUG("request done.\n");
/*
    if (wait.flags == IAV_WAIT_BITS_FIFO) {
        return ME_OK;
    }
    if (wait.flags == IAV_WAIT_OUTPIC) {
        return ME_BUSY;
    }
*/
    return ME_OK;
}


inline void CAmbaDspIOne::_Dump_es_data(AM_INT udecIndex, AM_U8* pStart, AM_U8* pEnd)
{
#ifdef AM_DEBUG
    FILE* mpDumpFile = NULL;

    //AMLOG_INFO("in _Dump_es_data...mLogOutput = %x.\n",mLogOutput);
    //dump data write to file
    if (mLogOutput & LogDumpTotalBinary) {
        if(udecIndex==0){
            if (!mpDumpFile) {
                mpDumpFile = fopen(g_dumpfilePath1, "ab");
            }
        }else if(udecIndex==1){
            if (!mpDumpFile) {
                mpDumpFile = fopen(g_dumpfilePath2, "ab");
            }
        }else if(udecIndex==2){//designed for 3 udec instances
            if (!mpDumpFile) {
                mpDumpFile = fopen(g_dumpfilePath3, "ab");
            }
        }else if(udecIndex==3){//designed for 4 udec instances
            if (!mpDumpFile) {
                mpDumpFile = fopen(g_dumpfilePath4, "ab");
            }
        }
        AM_U8* pStartAddr = mpSharedRes->dspConfig.udecInstanceConfig[udecIndex].pbits_fifo_start;
        AM_U8* pEndAddr = mpSharedRes->dspConfig.udecInstanceConfig[udecIndex].pbits_fifo_start
                                      + mpSharedRes->dspConfig.udecInstanceConfig[udecIndex].bits_fifo_size;
        if (mpDumpFile) {
            //AM_INFO("write data.\n");
            AM_ASSERT(pEnd != pStart);
            if (pEnd < pStart) {
                //wrap around
                fwrite(pStart, 1, (size_t)(pEndAddr - pStart), mpDumpFile);
                fwrite(pStartAddr, 1, (size_t)(pEnd - pStartAddr), mpDumpFile);
            } else {
                fwrite(pStart, 1, (size_t)(pEnd - pStart), mpDumpFile);
            }
            fclose(mpDumpFile);
            mpDumpFile = NULL;
        } else {
            AMLOG_INFO("open  mpDumpFile fail.\n");
        }
    }
#endif
}

AM_ERR CAmbaDspIOne::DecodeBitStream(AM_INT udecIndex, AM_U8*pStart, AM_U8*pEnd)
{
    AM_INT ret;
    iav_udec_decode_t dec;
    AM_U8* pStartAddr = mpSharedRes->dspConfig.udecInstanceConfig[udecIndex].pbits_fifo_start;
    AM_U8* pEndAddr = mpSharedRes->dspConfig.udecInstanceConfig[udecIndex].pbits_fifo_start
                                      + mpSharedRes->dspConfig.udecInstanceConfig[udecIndex].bits_fifo_size;

    AM_ASSERT(pStart>=pStartAddr);
    AM_ASSERT(pStart<pEndAddr);
    AM_ASSERT(pEnd>=pStartAddr);
    AM_ASSERT(pEnd<=pEndAddr);

    if (pEnd < pStart)
        AMLOG_BINARY("Index[%d], 0x%6x ~ 0x%6x.  size = 0x%x. ><.\n", udecIndex, pStart-pStartAddr, pEnd-pStartAddr, 4*1020*1024+pEnd-pStart);
    else
        AMLOG_BINARY("Index[%d], 0x%6x ~ 0x%6x.  size = 0x%x.\n", udecIndex, pStart-pStartAddr, pEnd-pStartAddr, pEnd-pStart);

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::DecodeBitStream.\n", udecIndex);
        return ME_ERROR;
    }

    memset(&dec, 0, sizeof(dec));
    dec.udec_type = mDecInfo[udecIndex].dec_format;
    dec.decoder_id = udecIndex;
    dec.u.fifo.start_addr = pStart;
    dec.u.fifo.end_addr = pEnd;
    dec.num_pics = 1;
    AMLOG_BINARY("DecodeBuffer length %d, %x.\n", pEnd-pStart, *pStart);

    if(3==mpSharedRes->mDSPmode){
        if ((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_DECODE2, &dec)) < 0) {
            perror("IAV_IOC_UDEC_DECODE");
            AM_ERROR("!!!!!IAV_IOC_UDEC_DECODE error, ret %d.\n", ret);
            if (ret == (-EPERM)) {
                /*if (isUDECErrorState(udecIndex)) {
                    return ME_UDEC_ERROR;
                }*/
            }
            return ME_ERROR;
        }
    }else{
        if ((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
            perror("IAV_IOC_UDEC_DECODE");
            AM_ERROR("!!!!!IAV_IOC_UDEC_DECODE error, ret %d.\n", ret);
            if (ret == (-EPERM)) {
                if (isUDECErrorState(udecIndex)) {
                    return ME_UDEC_ERROR;
                }
            }
            return ME_ERROR;
        }
    }

    _Dump_es_data(udecIndex, pStart, pEnd);

    return ME_OK;
}

AM_ERR CAmbaDspIOne::AccelDecoding(AM_INT udecIndex, void* pParam, bool needSync)
{
    AM_INT ret;
    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::AccelDecoding.\n", udecIndex);
        return ME_UDEC_ERROR;
    }

#ifdef AM_DEBUG
    iav_udec_decode_s debug_iav_s;
    udec_decode_t debug_ff_t;
    //check iav's struct is sync with ffmpeg's struct
    AM_ASSERT(sizeof(debug_iav_s) == sizeof(debug_ff_t));
    AM_ASSERT(sizeof(debug_iav_s.u) == sizeof(debug_ff_t.uu));
    AM_ASSERT(sizeof(debug_iav_s.u.mp4s) == sizeof(debug_ff_t.uu.mpeg4s));
    AM_ASSERT(sizeof(debug_iav_s.u.rv40) == sizeof(debug_ff_t.uu.rv40));
    AM_ASSERT(sizeof(debug_iav_s.u.fifo) == sizeof(debug_ff_t.uu.fifo));
#endif

    //send cmd
    if(3==mpSharedRes->mDSPmode){
        AMLOG_DEBUG("**IAV_IOC_UDEC_DECODE2 start.\n");
        if((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_DECODE2, pParam))<0) {
            perror("IAV_IOC_UDEC_DECODE2");
            AMLOG_ERROR("!!!!!IAV_IOC_UDEC_DECODE2 error, ret %d.\n", ret);
            if (ret == (-EPERM)) {
                /*if (isUDECErrorState(udecIndex)) {
                    return ME_UDEC_ERROR;
                }*/
            }
            return ME_ERROR;
        }
        AMLOG_DEBUG("**IAV_IOC_UDEC_DECODE done, ret %d.\n", ret);
    }else{
        AMLOG_DEBUG("**IAV_IOC_UDEC_DECODE start.\n");
        if((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_DECODE, pParam))<0) {
            perror("IAV_IOC_UDEC_DECODE");
            AMLOG_ERROR("!!!!!IAV_IOC_UDEC_DECODE error, ret %d.\n", ret);
            if (ret == (-EPERM)) {
                if (isUDECErrorState(udecIndex)) {
                    return ME_UDEC_ERROR;
                }
            }
            return ME_ERROR;
        }
        AMLOG_DEBUG("**IAV_IOC_UDEC_DECODE done, ret %d.\n", ret);
    }

    if (needSync) {
        AM_ASSERT(mDecInfo[udecIndex].dec_format == UDEC_RV40);
        //wait msg done
        iav_frame_buffer_t frame;
        ::memset(&frame, 0, sizeof(frame));
        frame.flags = IAV_FRAME_NEED_SYNC;
        frame.decoder_id = udecIndex;

        AMLOG_DEBUG("**IAV_IOC_GET_DECODED_FRAME, frame.fb_id %d.\n", frame.fb_id);
        if ((ret = ::ioctl(mIavFd, IAV_IOC_GET_DECODED_FRAME, &frame)) < 0) {
            perror("IAV_IOC_UDEC_DECODE");
            AMLOG_ERROR("!!!!!IAV_IOC_GET_DECODED_FRAME error, ret %d.\n", ret);
            if (ret == (-EPERM)) {
                if (isUDECErrorState(udecIndex)) {
                    return ME_UDEC_ERROR;
                }
            }
            return ME_ERROR;
        } else {
            AM_ASSERT(mDecInfo[udecIndex].dec_format != UDEC_RV40);
        }
        AMLOG_DEBUG("**IAV_IOC_GET_DECODED_FRAME done, frame.fb_id %d.\n", frame.fb_id);
    }

    return ME_OK;
}

//stop, flush, clear, trick play
AM_ERR CAmbaDspIOne::StopUDEC(AM_INT udecIndex)
{
    AM_INT ret = 0;
    AM_UINT stop_code = udecIndex;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::StopUDEC.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    if(3==mpSharedRes->mDSPmode){
        AM_PRINTF("IAV_IOC_UDEC_STOP2 start, 0x%x.\n",stop_code);
        if ((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_STOP2, stop_code)) < 0) {
            AM_PRINTF("IAV_IOC_UDEC_STOP2 error %d.\n", ret);
            return ME_OS_ERROR;
        }
    }else{
        AM_PRINTF("IAV_IOC_UDEC_STOP start, 0x%x.\n",stop_code);
        if ((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_STOP, stop_code)) < 0) {
            AM_PRINTF("IAV_IOC_UDEC_STOP error %d.\n", ret);
            return ME_OS_ERROR;
        }
    }
    AM_PRINTF("IAV_IOC_UDEC_STOP done.\n");
    return ME_OK;
}

AM_ERR CAmbaDspIOne::FlushUDEC(AM_INT udecIndex, AM_BOOL lastPic)
{
    AM_INT ret = 0;
    AM_UINT stop_code;
    if(lastPic == AM_TRUE){
        stop_code = 0x01000000|udecIndex;
    }else{
        stop_code = 0x00000000|udecIndex;
    }

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::FlushUDEC.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    if(3==mpSharedRes->mDSPmode){
        AM_PRINTF("IAV_IOC_UDEC_STOP2 start, 0x%x.\n",stop_code);
        if ((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_STOP2, stop_code)) < 0) {
            AM_PRINTF("IAV_IOC_UDEC_STOP2 error %d.\n", ret);
            return ME_OS_ERROR;
        }
    }else{
        AM_INFO("IAV_IOC_UDEC_STOP start, 0x%x.\n",stop_code);
        if ((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_STOP, stop_code)) < 0) {
            AM_ERROR("IAV_IOC_UDEC_STOP error %d.\n", ret);
            return ME_OS_ERROR;
        }
    }
    AM_PRINTF("IAV_IOC_UDEC_STOP done.\n");
    return ME_OK;
}

AM_ERR CAmbaDspIOne::ClearUDEC(AM_INT udecIndex)
{
    AM_INT ret = 0;
    AM_UINT stop_code = 0xff000000|udecIndex;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::ClearUDEC.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    if(3==mpSharedRes->mDSPmode){
        AM_PRINTF("IAV_IOC_UDEC_STOP2 start, 0x%x.\n",stop_code);
        if ((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_STOP2, stop_code)) < 0) {
            AM_PRINTF("IAV_IOC_UDEC_STOP2 error %d.\n", ret);
            return ME_OS_ERROR;
        }
    }else{
        AM_PRINTF("IAV_IOC_UDEC_STOP start, 0x%x.\n",stop_code);
        if ((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_STOP, stop_code)) < 0) {
            AM_PRINTF("IAV_IOC_UDEC_STOP error %d.\n", ret);
            return ME_OS_ERROR;
        }
    }
    AM_PRINTF("IAV_IOC_UDEC_STOP done.\n");
    return ME_OK;
}

AM_ERR CAmbaDspIOne::PauseUDEC(AM_INT udecIndex)
{
    iav_udec_trickplay_t trickplay;
    AM_INT ret;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::PauseUDEC.\n", udecIndex);
        return ME_UDEC_ERROR;
    }

    AM_UINT udec_state, vout_state, error_code;
    GetUDECState(udecIndex, udec_state, vout_state, error_code);
    AM_ASSERT(vout_state == IAV_VOUT_STATE_RUN);
    AM_ASSERT(udec_state == IAV_UDEC_STATE_RUN);
    if (vout_state !=IAV_VOUT_STATE_RUN || udec_state != IAV_UDEC_STATE_RUN) {
        AMLOG_ERROR("**pause: but udec is not IAV_VOUT_STATE_RUN(%d), udec_state %d, return.\n", vout_state, udec_state);
        return ME_BAD_STATE;
    }

    trickplay.decoder_id = udecIndex;
    trickplay.mode = 0; //pause
    AM_PRINTF("**pause: start IAV_IOC_UDEC_TRICKPLAY.\n");
    ret = ioctl(mIavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
    if (!ret) {
        return ME_OK;
    } else {
        AMLOG_ERROR("**IAV_IOC_UDEC_TRICKPLAY fail, ret %d, retry.\n", ret);
        usleep(20000);
        ret = ioctl(mIavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
        if(!ret) {
            return ME_OK;
        } else {
            AMLOG_ERROR("**IAV_IOC_UDEC_TRICKPLAY fail, ret %d.\n", ret);
            return ME_ERROR;
        }
    }
}

AM_ERR CAmbaDspIOne::ResumeUDEC(AM_INT udecIndex)
{
    iav_udec_trickplay_t trickplay;
    AM_INT ret;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::ResumeUDEC.\n", udecIndex);
        return ME_UDEC_ERROR;
    }

//#ifdef AM_DEBUG
    AM_UINT udec_state, vout_state, error_code;
    GetUDECState(udecIndex, udec_state, vout_state, error_code);
    AM_ASSERT(vout_state == IAV_VOUT_STATE_PAUSE);
    AM_ASSERT(udec_state == IAV_UDEC_STATE_RUN);
    if (vout_state !=IAV_VOUT_STATE_PAUSE || udec_state != IAV_UDEC_STATE_RUN) {
        AM_ERROR("**pause: but udec is not IAV_VOUT_STATE_PAUSE(%d), udec %d, return.\n", vout_state, udec_state);
        //return ME_BAD_STATE;
    }
//#endif

    trickplay.decoder_id = udecIndex;
    trickplay.mode = 1; //resume
    AM_PRINTF("**resume: start IAV_IOC_UDEC_TRICKPLAY.\n");
    ret = ioctl(mIavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
    if (!ret) {
        return ME_OK;
    } else {
        AM_ERROR("**IAV_IOC_UDEC_TRICKPLAY fail, ret %d, retry.\n", ret);
        usleep(20000);
        ret = ioctl(mIavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
        if(!ret) {
            return ME_OK;
        } else {
            AM_ERROR("**IAV_IOC_UDEC_TRICKPLAY fail, ret %d.\n", ret);
            return ME_ERROR;
        }
    }

}

AM_ERR CAmbaDspIOne::StepPlay(AM_INT udecIndex)
{
    iav_udec_trickplay_t trickplay;
    AM_INT ret;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::StepPlay.\n", udecIndex);
        return ME_UDEC_ERROR;
    }

    trickplay.decoder_id = udecIndex;
    trickplay.mode = 2; //step
    AM_PRINTF("**resume: start IAV_IOC_UDEC_TRICKPLAY.\n");
    ret = ioctl(mIavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
    if (!ret) {
        return ME_OK;
    } else {
        return ME_ERROR;
    }
}

//frame buffer pool
AM_UINT CAmbaDspIOne::InitFrameBufferPool(AM_INT udecIndex, AM_UINT picWidth, AM_UINT picHeight, AM_UINT chromaFormat, AM_UINT tileFormat, AM_UINT hasEdge)
{
    iav_fbp_config_t fbp_config;
    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::InitFrameBufferPool.\n", udecIndex);
        return 0;
    }

    if (chromaFormat > 2 || tileFormat > 0) {
        AMLOG_ERROR("Bad wantUdecIndex in CAmbaDspIOne::InitFrameBufferPool chromaFormat %d, tileFormat %d.\n", chromaFormat, tileFormat);
        return 0;
    }

    mDecInfo[udecIndex].buf_width = (picWidth+ D_DSP_VIDEO_BUFFER_WIDTH_ALIGNMENT - 1)&(~(D_DSP_VIDEO_BUFFER_WIDTH_ALIGNMENT-1));
    mDecInfo[udecIndex].buf_height = (picHeight + D_DSP_VIDEO_BUFFER_HEIGHT_ALIGNMENT - 1)&(~(D_DSP_VIDEO_BUFFER_HEIGHT_ALIGNMENT - 1));
    if (hasEdge) {
        mDecInfo[udecIndex].buf_width += 32;
        mDecInfo[udecIndex].buf_height += 32;
    }
    mDecInfo[udecIndex].tile_format = tileFormat;
    AM_ASSERT((mDecInfo[udecIndex].dec_format == UDEC_SW) || (mDecInfo[udecIndex].dec_format == UDEC_RV40) || (mDecInfo[udecIndex].dec_format == UDEC_MP4S && mPpMode == 1));

    AMLOG_PRINTF("start InitFrameBufferPool format %d, picWidth %d, picHeight %d, buf_width %d, buf_height %d.\n", mDecInfo[udecIndex].dec_format, picWidth, picHeight, mDecInfo[udecIndex].buf_width, mDecInfo[udecIndex].buf_height);
    memset(&fbp_config, 0, sizeof(fbp_config));
    fbp_config.decoder_id = udecIndex;
    fbp_config.chroma_format = chromaFormat;
    fbp_config.tiled_mode = tileFormat;
    fbp_config.buf_width = mDecInfo[udecIndex].buf_width;
    fbp_config.buf_height = mDecInfo[udecIndex].buf_height;
    fbp_config.lu_width = picWidth;
    fbp_config.lu_height = picHeight;
    if(hasEdge){
        fbp_config.lu_row_offset = 16;
        fbp_config.lu_col_offset = 16;
        fbp_config.ch_row_offset = 8;
        fbp_config.ch_col_offset = 16;//nv12
    }else{
        fbp_config.lu_row_offset = 0;
        fbp_config.lu_col_offset = 0;
        fbp_config.ch_row_offset = 0;
        fbp_config.ch_col_offset = 0;
    }

    if (ioctl(mIavFd, IAV_IOC_CONFIG_FB_POOL, &fbp_config) < 0) {
        AM_PERROR("IAV_IOC_CONFIG_FB_POOL");
        AM_ERROR("IAV_IOC_CONFIG_FB_POOL FAILS [%d*%d]\n", mDecInfo[udecIndex].buf_width, mDecInfo[udecIndex].buf_height);
        return 0;
    }

    AM_ASSERT(mDecInfo[udecIndex].buf_width == fbp_config.buf_width);
    AM_ASSERT(mDecInfo[udecIndex].buf_height == fbp_config.buf_height);
    mDecInfo[udecIndex].buf_width = fbp_config.buf_width;
    mDecInfo[udecIndex].buf_height = fbp_config.buf_height;
    mDecInfo[udecIndex].buf_number =  fbp_config.num_frm_bufs;
    AMLOG_PRINTF("InitFrameBufferPool done, fbwidth %d, height %d, num %d.\n", mDecInfo[udecIndex].buf_width, mDecInfo[udecIndex].buf_height, mDecInfo[udecIndex].buf_number);

    return mDecInfo[udecIndex].buf_number;
}

AM_ERR CAmbaDspIOne::ResetFrameBufferPool(AM_INT udecIndex)
{
    return ME_OK;
}

//buffer id/data pointer in CVideoBuffer
AM_ERR CAmbaDspIOne::RequestFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer)
{
    iav_frame_buffer_t frame;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::RequestFrameBuffer.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    AM_ASSERT((mDecInfo[udecIndex].dec_format == UDEC_SW) || (mDecInfo[udecIndex].dec_format == UDEC_RV40) || (mDecInfo[udecIndex].dec_format == UDEC_MP4S && mPpMode == 1));

    memset(&frame, 0, sizeof(frame));
    frame.decoder_id = udecIndex;
    frame.pic_struct = 3;
    AMLOG_DEBUG("**IAV_IOC_REQUEST_FRAME, decode_id %d.\n", udecIndex);
    if (ioctl(mIavFd, IAV_IOC_REQUEST_FRAME, &frame) < 0) {
        //release buffer from inherited::AllocBuffer
        AM_PERROR("IAV_IOC_REQUEST_FRAME");
        AMLOG_ERROR("IAV_IOC_REQUEST_FRAME fail.\n");
        return ME_OS_ERROR;
    }
    AMLOG_DEBUG("**IAV_IOC_REQUEST_FRAME done, buffer id %d, real_fb_id %d, frame.buffer_pitch %d,frame.buffer_width %d.\n", frame.fb_id, frame.real_fb_id, frame.buffer_pitch, frame.buffer_width);

    AMLOG_DEBUG("**frame.lu_buf_addr %p, rframe.ch_buf_addr %p.\n", frame.lu_buf_addr, frame.ch_buf_addr);
    AMLOG_DEBUG("diff %d.\n", (AM_UINT)frame.ch_buf_addr - (AM_UINT)frame.lu_buf_addr);

    pBuffer->pLumaAddr = (AM_U8*)frame.lu_buf_addr;
    pBuffer->pChromaAddr = (AM_U8*)frame.ch_buf_addr;
    pBuffer->buffer_id= (AM_INT)frame.fb_id;
    pBuffer->real_buffer_id= (AM_INT)frame.real_fb_id;

    pBuffer->SetType(CBuffer::DATA);
    pBuffer->SetDataSize(0);
    pBuffer->SetDataPtr(NULL);

    pBuffer->picWidth = mDecInfo[udecIndex].pic_width;
    pBuffer->picHeight = mDecInfo[udecIndex].pic_height;
    pBuffer->fbWidth = mDecInfo[udecIndex].buf_width;
    pBuffer->fbHeight = mDecInfo[udecIndex].buf_height;
    //AM_INFO("CIAVVideoBufferPool mPicWidth %d, mPicHeight %d, mFbWidth %d, mFbHeight %d, ext %d.\n", mPicWidth, mPicHeight, mFbWidth, mFbHeight, mbExtEdge);
    if (mDecInfo[udecIndex].ext_edge) {
        pBuffer->picXoff = 16;
        pBuffer->picYoff = 16;
    } else {
        pBuffer->picXoff = 0;
        pBuffer->picYoff = 0;
    }

    pBuffer->flags = 0;//IAV_FRAME_NO_RELEASE;

    return ME_OK;
}

AM_ERR CAmbaDspIOne::RenderFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer, SRect* srcRect)
{
    AM_ASSERT(pBuffer);
    AM_ASSERT(srcRect);
    iav_frame_buffer_t frame;
    int ret=0;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::RenderFrameBuffer.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    memset(&frame, 0, sizeof(frame));
    frame.flags = IAV_FRAME_NO_RELEASE;//only display
    frame.fb_id = (u16)pBuffer->buffer_id;
    frame.decoder_id = udecIndex;
    frame.real_fb_id = (u16)pBuffer->real_buffer_id;

    //source rect
    frame.pic_width = srcRect->w;
    frame.pic_height = srcRect->h;
    frame.lu_off_x = pBuffer->picXoff + srcRect->x;
    frame.lu_off_y = pBuffer->picYoff + srcRect->y;
    frame.ch_off_x = frame.lu_off_x  / 2;
    frame.ch_off_y = frame.lu_off_y / 2;

    //need send pts to ucode
    am_pts_t pts = pBuffer->mPTS;
    frame.pts = (u32)(pts & 0xffffffff);
    frame.pts_high = (u32)(pts>>32);

    //AM_PRINTF("frame.pic_width=%d,frame.pic_height=%d.\n",frame.pic_width,frame.pic_height);
    //AM_PRINTF("frame.lu_off_x=%d,frame.lu_off_y=%d.\n",frame.lu_off_x,frame.lu_off_y);
    //AM_PRINTF("frame.ch_off_x=%d,frame.ch_off_y=%d.\n",frame.ch_off_x,frame.ch_off_y);
    AMLOG_DEBUG("IAV_IOC_POSTP_FRAME, fb_id=%d, real_fb_id %d frame.flags %d, pts high %u, low %u.\n",frame.fb_id, frame.real_fb_id, frame.flags, frame.pts_high, frame.pts);
    if ((ret=::ioctl(mIavFd, IAV_IOC_POSTP_FRAME, &frame)) < 0) {
        AM_PERROR("IAV_IOC_POSTP_FRAME.\n");
        AMLOG_ERROR("IAV_IOC_POSTP_FRAME fail,fb_id=%d,ret=%d.\n",frame.fb_id,ret);
        return ME_ERROR;
    }

    pBuffer->flags = 0;
    AMLOG_DEBUG("IAV_IOC_POSTP_FRAME end.\n");
    return ME_OK;
}

AM_ERR CAmbaDspIOne::ReleaseFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer)
{
    if(mPpMode == 1)
        return ME_OK;

    iav_decoded_frame_t frame;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::ReleaseFrameBuffer.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    memset(&frame, 0, sizeof(frame));
    frame.flags = (u16)pBuffer->flags;//0;
    frame.fb_id = (u16)pBuffer->buffer_id;
    frame.real_fb_id = (u16)pBuffer->real_buffer_id;
    AMLOG_DEBUG("**IAV_IOC_RELEASE_FRAME, frame.fb_id %d, real_fb_id %d, pVideoBuffer->flags %d.\n", frame.fb_id, frame.real_fb_id, pBuffer->flags);
    if (::ioctl(mIavFd, IAV_IOC_RELEASE_FRAME, &frame) < 0) {
        AM_PERROR("IAV_IOC_RELEASE_FRAME");
        AMLOG_ERROR("IAV_IOC_RELEASE_FRAME Fail, pVideoBuffer->buffer_id %d.\n", pBuffer->buffer_id);
        return ME_ERROR;
    }
    return ME_OK;
}

//avsync
AM_ERR CAmbaDspIOne::SetClockOffset(AM_INT udecIndex, AM_S32 diff)
{
    iav_audio_clk_offset_s offset;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::SetClockOffset.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    offset.decoder_id = udecIndex;
    offset.audio_clk_offset = diff;

    AMLOG_PTS(" [AV Sync], offset.audio_clk_offset %d.\n", offset.audio_clk_offset);
    if (ioctl(mIavFd, IAV_IOC_SET_AUDIO_CLK_OFFSET, &offset) < 0) {
        AM_PERROR("IAV_IOC_SET_AUDIO_CLK_OFFSET");
        AMLOG_PRINTF("error IAV_IOC_SET_AUDIO_CLK_OFFSET\n");
        return ME_ERROR;
    }
    return ME_OK;
}

AM_ERR CAmbaDspIOne::WakeVout(AM_INT udecIndex, AM_UINT voutMask)
{
    iav_wake_vout_s wake;
    AM_INT ret;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::WakeVout.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    wake.decoder_id = (AM_U8)udecIndex;

    AMLOG_PRINTF("[AV Sync] start wake vout[%d], IAV_IOC_WAKE_VOUT.\n", udecIndex);
    if ((ret = ioctl(mIavFd, IAV_IOC_WAKE_VOUT, &wake)) < 0) {
        perror("IAV_IOC_WAKE_VOUT");
        AMLOG_ERROR("IAV_IOC_WAKE_VOUT, ret %d.\n", ret);
        return ME_ERROR;
    }
    AMLOG_PRINTF("IAV_IOC_WAKE_VOUT done.\n");
    return ME_OK;
}

AM_ERR CAmbaDspIOne::GetUdecTimeEos(AM_INT udecIndex, AM_UINT& eos, AM_U64& udecTime, AM_INT nowait)
{
    iav_udec_status_t status;
    AM_INT ret;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::GetUdecTimeEos.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    memset(&status, 0, sizeof(status));
    status.decoder_id = (AM_U8)udecIndex;
    status.nowait = (AM_U8)nowait;
    status.only_query_current_pts = 1;
    //AMLOG_PRINTF("start IAV_IOC_WAIT_UDEC_STATUS\n");
    if ((ret = ioctl(mIavFd, IAV_IOC_WAIT_UDEC_STATUS, &status)) != 0) {
        perror("IAV_IOC_WAIT_UDEC_STATUS");
        AMLOG_PRINTF("!!!!!IAV_IOC_WAIT_UDEC_STATUS error, ret %d.\n", ret);
        if (ret == (-EPERM)) {
            if (isUDECErrorState(udecIndex)) {
                return ME_UDEC_ERROR;
            }
        }
        return ME_ERROR;
    }
    if(status.pts_valid == 0)
        return ME_CLOSED;

    eos = status.eos_flag;
    udecTime = ((AM_U64)status.pts_low) | (((AM_U64)status.pts_high)<<32);
    return ME_OK;
}

//query UDEC state
AM_ERR CAmbaDspIOne::GetUDECState(AM_INT udecIndex, AM_UINT& udecState, AM_UINT& voutState, AM_UINT& errorCode)
{
    AM_INT ret = 0;
    iav_udec_state_t state;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::GetUDECState.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    memset(&state, 0, sizeof(state));
    state.decoder_id = (AM_U8)udecIndex;
    state.flags = 0;

    ret = ioctl(mIavFd, IAV_IOC_GET_UDEC_STATE, &state);
    if (ret) {
        perror("IAV_IOC_GET_UDEC_STATE");
        AMLOG_ERROR("IAV_IOC_GET_UDEC_STATE %d.\n", ret);
        return ME_ERROR;
    }

    udecState = state.udec_state;
    voutState = state.vout_state;
    errorCode = state.error_code;

    if (state.udec_state == IAV_UDEC_STATE_ERROR) {
        return ME_UDEC_ERROR;
    }

    return ME_OK;
}

AM_ERR CAmbaDspIOne::GetDecodedFrame(AM_INT udecIndex, AM_INT& buffer_id, AM_INT& eos_invalid)
{
    int ret;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::GetDecodedFrame.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    //wait msg done
    iav_wait_decoder_t wait;
    ::memset(&wait, 0x0, sizeof(wait));
    wait.decoder_id = udecIndex;
    wait.flags = IAV_WAIT_OUTPIC;

    AMLOG_DEBUG("**IAV_IOC_WAIT_DECODER, mIavFd %d.\n", mIavFd);
    if ((ret = ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait)) < 0) {
        AM_PERROR("IAV_IOC_WAIT_DECODER");
        AMLOG_ERROR("IAV_IOC_WAIT_DECODER IAV_WAIT_OUTPIC fail, ret %d.\n",ret);
        return ME_OS_ERROR;
    }
    AM_ASSERT(wait.flags == IAV_WAIT_OUTPIC);
    AMLOG_DEBUG("**hybirdUDECGetframe wait done.\n");

    iav_frame_buffer_t frame;
    ::memset(&frame, 0, sizeof(frame));
    frame.flags = 0; //output_flag ? IAV_FRAME_NEED_ADDR : 0;
/*    if(1 == mPpMode){
        frame.flags |= IAV_FRAME_SYNC_VOUT;
        frame.flags |= IAV_FRAME_NEED_ADDR;
    }
*/
    frame.decoder_id = udecIndex;

    if ((ret = ::ioctl(mIavFd, IAV_IOC_GET_DECODED_FRAME, &frame)) < 0) {
        AM_PERROR("IAV_IOC_GET_DECODED_FRAME");
        AMLOG_ERROR("IAV_IOC_GET_DECODED_FRAME fail, ret %d.\n",ret);
        return ME_OS_ERROR;
    }
    AMLOG_DEBUG("**IAV_IOC_GET_DECODED_FRAME done, frame.fb_id %d.\n", frame.fb_id);
    buffer_id = frame.fb_id;

    if (IAV_INVALID_FB_ID==frame.fb_id && 1==frame.eos_flag) {
        eos_invalid = 1;
    }else
        eos_invalid = 0;
/*
    if(pVideoBuffer){
        CBuffer* pBuffer = (CBuffer*)pVideoBuffer;
        pBuffer->SetDataSize(0);
        pBuffer->SetDataPtr(NULL);
        pBuffer->SetPTS((AM_U64)(frame.pts) | (((AM_U64)(frame.pts_high))<<32));
        if(eos_invalid)
            pBuffer->SetType(CBuffer::EOS);
        else{
            pBuffer->SetType(CBuffer::DATA);
            pVideoBuffer->pLumaAddr = (AM_U8*)frame.lu_buf_addr;
            pVideoBuffer->pChromaAddr = (AM_U8*)frame.ch_buf_addr;
            pVideoBuffer->buffer_id = frame.fb_id;
            pVideoBuffer->real_buffer_id = frame.real_fb_id;
            pVideoBuffer->picWidth = frame.pic_width;
            pVideoBuffer->picHeight = frame.pic_height;
            pVideoBuffer->fbWidth = frame.buffer_width;
            pVideoBuffer->fbHeight = frame.buffer_height;
            pVideoBuffer->picXoff = frame.lu_off_x;
            pVideoBuffer->picYoff = frame.lu_off_y;
            pVideoBuffer->flags = frame.flags;
        }
    }
    eos_invalid = 0;
*/
    return ME_OK;
}

bool CAmbaDspIOne::isUDECErrorState(AM_INT udecIndex)
{
    AM_INT ret = 0;
    iav_udec_state_t state;

    memset(&state, 0, sizeof(state));
    state.decoder_id = (AM_U8)udecIndex;
    state.flags = 0;

    ret = ioctl(mIavFd, IAV_IOC_GET_UDEC_STATE, &state);
    if (ret) {
        perror("IAV_IOC_GET_UDEC_STATE");
        AMLOG_ERROR("IAV_IOC_GET_UDEC_STATE %d.\n", ret);
        return false;
    }

    if (state.udec_state == IAV_UDEC_STATE_ERROR) {
        return true;
    }

    return false;
}

AM_ERR CAmbaDspIOne::enterUdecMode()
{
    AM_INT state;
    AM_INT i;

    i = ioctl(mIavFd, IAV_IOC_GET_STATE, &state);
    if (state != IAV_STATE_IDLE) {
        AMLOG_PRINTF("DSP Not in IDLE mode, enter IDLE mode first.\n");
        i = ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
        if (i != 0) {
            AMLOG_ERROR("UDEC enter IDLE mode fail, ret %d.\n", i);
            return ME_OS_ERROR;
        }
    }

    for (i=0; i<DMAX_UDEC_INSTANCE_NUM; i++) {
        mDecInfo[i].dec_format = UDEC_NONE;
        mbInstanceUsed[i] = 0;
    }

    //getDefaultDSPConfig(mpSharedRes);
    //getVout parameters
    setupVoutConfig(mpSharedRes);

//    mTotalUDECNumber = 2; //mTotalUDECNumber is set by SetUdecNums();
    AMLOG_PRINTF("enterUdecMode: mTotalUDECNumber %d.\n",mTotalUDECNumber);

    setUdecModeConfig();

    AM_ASSERT(mTotalUDECNumber == mUdecModeConfig.num_udecs);
    AM_ASSERT(mTotalUDECNumber > 0);
    AM_ASSERT(mTotalUDECNumber <= DMAX_UDEC_INSTANCE_NUM);

    if ((mTotalUDECNumber > DMAX_UDEC_INSTANCE_NUM) || (mTotalUDECNumber < 0)) {
        AMLOG_ERROR("mTotalUDECNumber error %d.\n", mTotalUDECNumber);
        mTotalUDECNumber = 1;
    }

    for (i = 0; i < mTotalUDECNumber; i ++) {
        setUdecConfig(i);
    }

    mUdecModeConfig.udec_config = &mUdecConfig[0];
    AMLOG_INFO("udec mode config:\n");
    AMLOG_INFO("  ppmode %d, enable_deint %d, chroma_format %d.\n", mUdecModeConfig.postp_mode, mUdecModeConfig.enable_deint, mUdecModeConfig.pp_chroma_fmt_max);
    AMLOG_INFO("  pp max width %d, height %d, num of buffers %d.\n", mUdecModeConfig.pp_max_frm_width, mUdecModeConfig.pp_max_frm_height, mUdecModeConfig.pp_max_frm_num);
    AMLOG_INFO("  num_udecs %d, vout_mask %x.\n", mUdecModeConfig.num_udecs, mUdecModeConfig.vout_mask);

    AMLOG_INFO("enter udec mode start\n");
    if (ioctl(mIavFd, IAV_IOC_ENTER_UDEC_MODE, &mUdecModeConfig) < 0) {
        perror("IAV_IOC_ENTER_UDEC_MODE");
        return ME_OS_ERROR;
    }

    mbUDECMode = true;
    AMLOG_INFO("enter udec mode done\n");
    return ME_OK;
}

AM_ERR CAmbaDspIOne::enterTranscodeMode()
{
//    AM_INT state;
    AM_INT i;
/*
    i = ioctl(mIavFd, IAV_IOC_GET_STATE, &state);
    if (state != IAV_STATE_IDLE) {
        AMLOG_PRINTF("DSP Not in IDLE mode, enter IDLE mode first.\n");
        i = ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
        if (i != 0) {
            AMLOG_ERROR("UDEC enter IDLE mode fail, ret %d.\n", i);
            return ME_OS_ERROR;
        }
    }
*/
    for (i=0; i<DMAX_UDEC_INSTANCE_NUM; i++) {
        mDecInfo[i].dec_format = UDEC_NONE;
        mbInstanceUsed[i] = 0;
    }

    //getDefaultDSPConfig(mpSharedRes);
    //getVout parameters
    setupVoutConfig(mpSharedRes);

//    mTotalUDECNumber = 2; //mTotalUDECNumber is set by SetUdecNums();
    AMLOG_PRINTF("enterTranscodeMode: mTotalUDECNumber %d.\n",mTotalUDECNumber);

    setTranscodeModeConfig();

    AM_ASSERT(mTotalUDECNumber == mTranscodeModeConfig.udec_mode.num_udecs);
    AM_ASSERT(mTotalUDECNumber > 0);
    AM_ASSERT(mTotalUDECNumber <= DMAX_UDEC_INSTANCE_NUM);

    if ((mTotalUDECNumber > DMAX_UDEC_INSTANCE_NUM) || (mTotalUDECNumber < 0)) {
        AMLOG_ERROR("mTotalUDECNumber error %d.\n", mTotalUDECNumber);
        mTotalUDECNumber = 1;
    }

    for (i = 0; i < mTotalUDECNumber; i ++) {
        setTranscConfig(i);
    }

    mTranscodeModeConfig.udec_mode.udec_config = &mUdecConfig[0];

    AMLOG_INFO("udec mode config:\n");
    AMLOG_INFO("  ppmode %d, enable_deint %d, chroma_format %d.\n", mTranscodeModeConfig.udec_mode.postp_mode, mTranscodeModeConfig.udec_mode.enable_deint, mTranscodeModeConfig.udec_mode.pp_chroma_fmt_max);
    AMLOG_INFO("  pp max width %d, height %d, num of buffers %d.\n", mTranscodeModeConfig.udec_mode.pp_max_frm_width, mTranscodeModeConfig.udec_mode.pp_max_frm_height, mTranscodeModeConfig.udec_mode.pp_max_frm_num);
    AMLOG_INFO("  num_udecs %d, vout_mask %x.\n", mTranscodeModeConfig.udec_mode.num_udecs, mTranscodeModeConfig.udec_mode.vout_mask);

    AMLOG_INFO("enter transcode mode start\n");
    if (ioctl(mIavFd, IAV_IOC_ENTER_TRANSCODE_MODE, &mTranscodeModeConfig) < 0) {
        perror("IAV_IOC_ENTER_TRANSCODE_MODE");
        return ME_OS_ERROR;
    }

    mbTranscodeMode = true;
    AMLOG_INFO("enter transcode mode done\n");
    mapDSP(0);

    return ME_OK;
}

AM_ERR CAmbaDspIOne::enterMdecMode()
{
//    AM_INT state;
    AM_INT i;

    if ((mTotalUDECNumber > DMAX_UDEC_INSTANCE_NUM) || (mTotalUDECNumber < 0)) {
        AMLOG_ERROR("mTotalUDECNumber error %d.\n", mTotalUDECNumber);
        return ME_TOO_MANY;
    }

    for (i=0; i<DMAX_UDEC_INSTANCE_NUM; i++) {
        mDecInfo[i].dec_format = UDEC_NONE;
        mbInstanceUsed[i] = 0;
    }

    //setupVoutConfig(mpSharedRes);

    if ((udec_configs = (iav_udec_config_t*)malloc(mTotalUDECNumber * sizeof(iav_udec_config_t))) == NULL) {
        AMLOG_ERROR("no memory when alloc udec_configs\n");
        return ME_NO_MEMORY;
    }

    udec_mode = &mdec_mode.super;

    setMdecModeConfig();

    updateSecondVoutSize();

    iav_udec_config_t *udec_config = udec_configs;
    memset(udec_config, 0, sizeof(iav_udec_config_t) * mTotalUDECNumber);
    for (i = 0; i < mTotalUDECNumber - 1; i++, udec_config++) {
        udec_config->tiled_mode = mpConfig->preset_tilemode;
        udec_config->frm_chroma_fmt_max = 1; // 4:2:0
        udec_config->dec_types = 0x17; // no RV40, no hybrid MPEG4
        udec_config->max_frm_num = mpConfig->udecInstanceConfig[i].max_frm_num; // MAX_DECODE_FRAMES - todo
        udec_config->max_frm_width = mpConfig->udecInstanceConfig[i].max_frm_width;
        udec_config->max_frm_height = mpConfig->udecInstanceConfig[i].max_frm_height;
        udec_config->max_fifo_size = 2*1024*1024;
    }
    //udec_config++;
    if(mpConfig->hdWin == 1 || mTotalUDECNumber == 1){
        udec_config->tiled_mode = mpConfig->preset_tilemode;
        udec_config->frm_chroma_fmt_max = 1; // 4:2:0
        udec_config->dec_types = 0x17; // no RV40, no hybrid MPEG4
        udec_config->max_frm_num = mpConfig->udecInstanceConfig[i].max_frm_num; // MAX_DECODE_FRAMES - todo
        udec_config->max_frm_width = 1920;
        udec_config->max_frm_height = 1088;
        udec_config->max_fifo_size = 2*1024*1024;
    }else{
        udec_config->tiled_mode = mpConfig->preset_tilemode;
        udec_config->frm_chroma_fmt_max = 1; // 4:2:0
        udec_config->dec_types = 0x17; // no RV40, no hybrid MPEG4
        udec_config->max_frm_num = mpConfig->udecInstanceConfig[i].max_frm_num; // MAX_DECODE_FRAMES - todo
        udec_config->max_frm_width = mpConfig->udecInstanceConfig[i].max_frm_width;
        udec_config->max_frm_height = mpConfig->udecInstanceConfig[i].max_frm_height; // todo  we can all let here 1080p simply
        udec_config->max_fifo_size = 2*1024*1024;
    }

    //HD WIN NOT CONSIDER WHEN STEUP WINOUT.
    //iavWindows.num_configs = mTotalUDECNumber;
    //if(mpConfig->hdWin == 1){
        //iavRenders.num_configs = iavRenders.total_num_windows_to_render = mTotalUDECNumber -1;
    //}else{
        //iavRenders.num_configs = iavRenders.total_num_windows_to_render = mTotalUDECNumber;
    //}
    //AM_INFO("NUM:%d, %d\n", iavRenders.num_configs, mTotalUDECNumber);
    //setMdecWindows(iavWindows.configs, mTotalUDECNumber);
    //setMdecRenders(iavRenders.configs, iavRenders.num_configs);

    AMLOG_INFO("mdec mode config:\n");
    AMLOG_INFO("  ppmode %d, enable_deint %d, chroma_format %d.\n", udec_mode->postp_mode,udec_mode->enable_deint, udec_mode->pp_chroma_fmt_max);
    AMLOG_INFO("  pp max width %d, height %d, num of buffers %d.\n", udec_mode->pp_max_frm_width, udec_mode->pp_max_frm_height, udec_mode->pp_max_frm_num);
    AMLOG_INFO("  num_udecs %d, vout_mask %x.\n", udec_mode->num_udecs, udec_mode->vout_mask);

    AM_INFO("enter Mdec mode start\n");
    if (ioctl(mIavFd, IAV_IOC_ENTER_MDEC_MODE, &mdec_mode) < 0) {
        perror("IAV_IOC_ENTER_MDEC_MODE");
        return ME_OS_ERROR;
    }

    mbMdecMode = true;
    AM_INFO("enter Mdec mode done\n");

    if(mpSharedRes->sTranscConfig.enableTranscode){
        if(InitMdecTranscoder(  mpSharedRes->sTranscConfig.enc_w,
                                    mpSharedRes->sTranscConfig.enc_h,
                                    mpSharedRes->sTranscConfig.bitrate)!=ME_OK){
            return ME_ERROR;
        }
        return StartMdecTranscoder();
    }
    return ME_OK;
}

//to do, no index?
AM_ERR CAmbaDspIOne::mapDSP(AM_INT udecIndex)
{
    iav_mmap_info_t info;
    if(mbDSPMapped)
        return ME_OK;
    if (ioctl(mIavFd, IAV_IOC_MAP_DSP, &info) < 0) {
        perror("IAV_IOC_MAP_DSP");
        AMLOG_ERROR("mapDSP fail.\n");
        return ME_ERROR;
    }
    mbDSPMapped = true;
    AM_INFO("IAV_IOC_MAP_DSP[%d] done!!\n", udecIndex);
    return ME_OK;
}

AM_ERR CAmbaDspIOne::unMapDSP(AM_INT udecIndex)
{
    if(!mbDSPMapped)
        return ME_OK;
    if (::ioctl(mIavFd, IAV_IOC_UNMAP_DSP) < 0) {
        AM_PERROR("IAV_IOC_UNMAP_DSP");
        return ME_ERROR;
    }
    mbDSPMapped = false;
    AM_INFO("IAV_IOC_UNMAP_DSP[%d] done!!\n", udecIndex);
    return ME_OK;
}
//--------------------------------------------------
//
//from IVoutHandler
//
//--------------------------------------------------

AM_ERR CAmbaDspIOne::Enable(AM_UINT voutID, AM_INT enable)
{
    AMLOG_ERROR("enable vout is not supported on ione, need ucode's implement.\n");
    return ME_NO_IMPL;
}

AM_ERR CAmbaDspIOne::EnableOSD(AM_UINT vout, AM_INT enable)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("EnableOSD: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    iav_vout_fb_sel_t fb_sel;

    memset(&fb_sel, 0, sizeof(iav_vout_fb_sel_t));
    fb_sel.vout_id = vout;

    if (!enable) {
        AMLOG_WARN("disable osd on vout %d.\n", vout);
        fb_sel.fb_id = -1;
    } else {
        AMLOG_WARN("enable osd on vout %d.\n", vout);
        fb_sel.fb_id = 0;//link to fb 0, hard code here
    }

    if(ioctl(mIavFd, IAV_IOC_VOUT_SELECT_FB, &fb_sel)) {
        AM_ERROR("IAV_IOC_VOUT_SELECT_FB Failed!");
        perror("IAV_IOC_VOUT_SELECT_FB");
        return ME_OS_ERROR;
    }

    return ME_OK;
}

AM_ERR CAmbaDspIOne::GetSizePosition(AM_UINT voutID, AM_INT* pos_x, AM_INT* pos_y, AM_INT* size_x, AM_INT* size_y)
{
    AUTO_LOCK(mpMutex);
    AM_ASSERT(mpConfig);
    AM_ASSERT(voutID < eVoutCnt);

    if (!mpConfig || voutID >= eVoutCnt) {
        AM_ASSERT(0);
        return ME_BAD_PARAM;
    }

    *pos_x = mpConfig->voutConfigs.voutConfig[voutID].pos_x;
    *pos_y = mpConfig->voutConfigs.voutConfig[voutID].pos_y;
    *size_x = mpConfig->voutConfigs.voutConfig[voutID].size_x;
    *size_y = mpConfig->voutConfigs.voutConfig[voutID].size_y;
    return ME_OK;
}

AM_ERR CAmbaDspIOne::GetDimension(AM_UINT voutID, AM_INT* size_x, AM_INT* size_y)
{
    AUTO_LOCK(mpMutex);
    AM_ASSERT(mpConfig);
    AM_ASSERT(voutID < eVoutCnt);

    if (!mpConfig || voutID >= eVoutCnt) {
        AM_ASSERT(0);
        return ME_BAD_PARAM;
    }

    *size_x = mpConfig->voutConfigs.voutConfig[voutID].width;
    *size_y = mpConfig->voutConfigs.voutConfig[voutID].height;
    return ME_OK;
}

AM_ERR CAmbaDspIOne::GetPirtureSizeOffset(AM_INT* pic_width, AM_INT* pic_height, AM_INT* offset_x, AM_INT* offset_y)
{
    AUTO_LOCK(mpMutex);
    AM_ASSERT(mpConfig);

    if (!mpConfig) {
        AM_ASSERT(0);
        return ME_BAD_PARAM;
    }
    *pic_width = mpConfig->voutConfigs.picture_width;
    *pic_height = mpConfig->voutConfigs.picture_height;
    *offset_x = mpConfig->voutConfigs.picture_offset_x;
    *offset_y = mpConfig->voutConfigs.picture_offset_y;
    return ME_OK;
}

AM_ERR CAmbaDspIOne::GetSoureRect(AM_INT* pos_x, AM_INT* pos_y, AM_INT* size_x, AM_INT* size_y)
{
    AUTO_LOCK(mpMutex);
    AM_ASSERT(mpConfig);
    //should not here, vout not regard picture size
    if (!mpConfig) {
        return ME_BAD_PARAM;
    }

    *pos_x = mpConfig->voutConfigs.src_pos_x;
    *pos_y = mpConfig->voutConfigs.src_pos_y;
    *size_x = mpConfig->voutConfigs.src_size_x;
    *size_y = mpConfig->voutConfigs.src_size_y;

    return ME_OK;
}

AM_ERR CAmbaDspIOne::ChangeInputCenter(AM_INT input_center_x, AM_INT input_center_y)
{
    AUTO_LOCK(mpMutex);
    AM_INT ret;
//    AM_U32 bak_zoomfactor_x[eVoutCnt], bak_zoomfactor_y[eVoutCnt];
    AM_ASSERT(mpConfig);
    if (!mpConfig || input_center_x <= 0 || input_center_y <= 0) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad parameters in CAmbaDspIOne::ChangeInputCenter,center %d %d.\n", input_center_x, input_center_y);
        return ME_BAD_PARAM;
    }

    mpConfig->voutConfigs.input_center_x = input_center_x;
    mpConfig->voutConfigs.input_center_y = input_center_y;
    if ((ret =updateAllVoutSetting()) != 0) {
        //restore parameters
        AM_ASSERT(0);
        AMLOG_ERROR("CAmbaDspIOne::ChangeInputCenter fail, center %d %d, pic size %d %d, pic offset %d, %d.\n", input_center_x, input_center_y, mpConfig->voutConfigs.picture_width, mpConfig->voutConfigs.picture_height, mpConfig->voutConfigs.picture_offset_x, mpConfig->voutConfigs.picture_offset_y);
        mpConfig->voutConfigs.input_center_x = mpConfig->voutConfigs.src_pos_x + (mpConfig->voutConfigs.src_size_x/2);
        mpConfig->voutConfigs.input_center_y = mpConfig->voutConfigs.src_pos_y + (mpConfig->voutConfigs.src_size_y/2);
        return ME_ERROR;
    } else {
        //update new source position
        mpConfig->voutConfigs.src_pos_x += input_center_x - (mpConfig->voutConfigs.src_size_x/2 + mpConfig->voutConfigs.src_pos_x);
        mpConfig->voutConfigs.src_pos_y += input_center_y - (mpConfig->voutConfigs.src_size_y/2 + mpConfig->voutConfigs.src_pos_y);
    }

    //check params for safe
    AM_ASSERT((mpConfig->voutConfigs.src_size_x + mpConfig->voutConfigs.src_pos_x*2) == (mpConfig->voutConfigs.input_center_x*2));
    AM_ASSERT((mpConfig->voutConfigs.src_size_y + mpConfig->voutConfigs.src_pos_y*2) == (mpConfig->voutConfigs.input_center_y*2));

    return ME_OK;
}

AM_ERR CAmbaDspIOne::ChangeRoomFactor(AM_UINT voutID, float factor_x, float factor_y)
{
    AUTO_LOCK(mpMutex);
    AM_ASSERT(mpConfig);
    AM_UINT i;
    AM_INT ret;
    float ratio_x, ratio_y;
    AM_U32 bak_zoomfactor_x[eVoutCnt], bak_zoomfactor_y[eVoutCnt];
    AM_INT new_pos_x, new_pos_y, new_size_x, new_size_y;

    if (!mpConfig || voutID >= eVoutCnt) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad parameters in CAmbaDspIOne::ChangeRoomFactor,voutID %d, factor %f %f.\n", voutID, factor_x, factor_y);
        return ME_BAD_PARAM;
    }

    new_size_x = (float)mpConfig->voutConfigs.voutConfig[voutID].size_x/factor_x;
    new_size_y = (float)mpConfig->voutConfigs.voutConfig[voutID].size_y/factor_y;
    //input center not change
    new_pos_x = mpConfig->voutConfigs.src_pos_x - (new_size_x - mpConfig->voutConfigs.src_size_x)/2;
    new_pos_y = mpConfig->voutConfigs.src_pos_y - (new_size_y - mpConfig->voutConfigs.src_size_y)/2;
    AMLOG_PRINTF("new source pos %d %d, size %d %d.\n", new_pos_x, new_pos_y, new_size_x, new_size_y);

    for (i=0; i<eVoutCnt; i++) {
        ratio_x = (float)mpConfig->voutConfigs.voutConfig[i].size_x/new_size_x;
        ratio_y = (float)mpConfig->voutConfigs.voutConfig[i].size_y/new_size_y;
        bak_zoomfactor_x[i] = mpConfig->voutConfigs.voutConfig[i].zoom_factor_x;
        bak_zoomfactor_y[i] = mpConfig->voutConfigs.voutConfig[i].zoom_factor_y;
        mpConfig->voutConfigs.voutConfig[i].zoom_factor_x = (AM_U32)(ratio_x*0x10000);
        mpConfig->voutConfigs.voutConfig[i].zoom_factor_y = (AM_U32)(ratio_y*0x10000);

        //verify
        if (voutID == i) {
            AM_ASSERT((factor_x+0.005 - ratio_x) > 0);
            AM_ASSERT((factor_x+0.005 - ratio_x) < 0.01);
            AM_ASSERT((factor_y+0.005 - ratio_y) > 0);
            AM_ASSERT((factor_y+0.005 - ratio_y) < 0.01);
        }
    }

    if ((ret = updateAllVoutSetting()) != ME_OK) {
        //restore parameters
        AM_ASSERT(0);
        AMLOG_ERROR("CAmbaDspIOne::ChangeSourceRect fail, pos %d %d, size %d %d, ret %d.\n", new_pos_x, new_pos_y, new_size_x, new_size_y, ret);
        for (i=0; i<eVoutCnt; i++) {
            mpConfig->voutConfigs.voutConfig[i].zoom_factor_x = bak_zoomfactor_x[i];
            mpConfig->voutConfigs.voutConfig[i].zoom_factor_y = bak_zoomfactor_y[i];
        }
        return ME_ERROR;
    } else {
        mpConfig->voutConfigs.src_pos_x = new_pos_x;
        mpConfig->voutConfigs.src_pos_y = new_pos_y;
        mpConfig->voutConfigs.src_size_x = new_size_x;
        mpConfig->voutConfigs.src_size_y = new_size_y;
    }

    //check params for safe
    AM_ASSERT((mpConfig->voutConfigs.src_size_x + mpConfig->voutConfigs.src_pos_x*2) == (mpConfig->voutConfigs.input_center_x*2));
    AM_ASSERT((mpConfig->voutConfigs.src_size_y + mpConfig->voutConfigs.src_pos_y*2) == (mpConfig->voutConfigs.input_center_y*2));

    return ME_OK;
}

AM_ERR CAmbaDspIOne::UpdatePirtureSizeOffset(AM_INT pic_width, AM_INT pic_height, AM_INT offset_x, AM_INT offset_y)
{
    AUTO_LOCK(mpMutex);
    AM_ASSERT(mpConfig);

    if (!mpConfig) {
        return ME_BAD_PARAM;
    }
    mpConfig->voutConfigs.picture_width = pic_width;
    mpConfig->voutConfigs.picture_height = pic_height;
    mpConfig->voutConfigs.picture_offset_x = offset_x;
    mpConfig->voutConfigs.picture_offset_y = offset_y;
    return ME_OK;
}

//todo: mass apis about change size/position/flip/rotate/mirror/source rect
AM_ERR CAmbaDspIOne::ChangeSizePosition(AM_UINT voutID, AM_INT pos_x, AM_INT pos_y, AM_INT size_x, AM_INT size_y)
{
    AUTO_LOCK(mpMutex);
    AM_INT ret;
    float ratio_x, ratio_y;
    AM_INT bak_pos_x, bak_pos_y, bak_size_x, bak_size_y;
    AM_U32 bak_zoomfactor_x, bak_zoomfactor_y;

    if (mpConfig->voutConfigs.voutConfig[voutID].size_x == size_x && mpConfig->voutConfigs.voutConfig[voutID].size_y == size_y && mpConfig->voutConfigs.voutConfig[voutID].pos_x == pos_x && mpConfig->voutConfigs.voutConfig[voutID].pos_y == pos_y) {
        return ME_OK;
    }

    if (RectOutofRange(size_x, size_y, pos_x, pos_y, mpConfig->voutConfigs.voutConfig[voutID].width, mpConfig->voutConfigs.voutConfig[voutID].height, 0, 0)) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad parameters in CAmbaDspIOne::ChangeSizePosition, vout id %d, pos_x %d, pos_y %d, size_x %d, size_y %d.\n", voutID, pos_x, pos_y, size_x, size_y);
        return ME_BAD_PARAM;
    }

    bak_pos_x = mpConfig->voutConfigs.voutConfig[voutID].pos_x;
    bak_pos_y = mpConfig->voutConfigs.voutConfig[voutID].pos_y;
    bak_size_x = mpConfig->voutConfigs.voutConfig[voutID].size_x;
    bak_size_y = mpConfig->voutConfigs.voutConfig[voutID].size_y;

    mpConfig->voutConfigs.voutConfig[voutID].pos_x = pos_x;
    mpConfig->voutConfigs.voutConfig[voutID].pos_y = pos_y;
    mpConfig->voutConfigs.voutConfig[voutID].size_x = size_x;
    mpConfig->voutConfigs.voutConfig[voutID].size_y = size_y;

    //zoom factor changes
    ratio_x = (float)mpConfig->voutConfigs.voutConfig[voutID].size_x/mpConfig->voutConfigs.src_size_x;
    ratio_y = (float)mpConfig->voutConfigs.voutConfig[voutID].size_y/mpConfig->voutConfigs.src_size_y;
    bak_zoomfactor_x = mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_x;
    bak_zoomfactor_y = mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_y;
    mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_x = (AM_U32)(ratio_x*0x10000);
    mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_y = (AM_U32)(ratio_y*0x10000);

    ret = updateVoutSetting(voutID);
    if (ret) {
        AM_ASSERT(0);
        AMLOG_ERROR("CAmbaDspIOne::ChangeSizePosition error.\n");
        //restore back params
        mpConfig->voutConfigs.voutConfig[voutID].pos_x = bak_pos_x;
        mpConfig->voutConfigs.voutConfig[voutID].pos_y = bak_pos_y;
        mpConfig->voutConfigs.voutConfig[voutID].size_x = bak_size_x;
        mpConfig->voutConfigs.voutConfig[voutID].size_y = bak_size_y;
        mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_x = bak_zoomfactor_x;
        mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_y = bak_zoomfactor_y;
        return ME_OS_ERROR;
    }
    return ME_OK;
}

AM_ERR CAmbaDspIOne::ChangeSize(AM_UINT voutID, AM_INT size_x, AM_INT size_y)
{
    AUTO_LOCK(mpMutex);
    AM_INT ret;
    AM_INT bak_size_x, bak_size_y;
    float ratio_x, ratio_y;;
    AM_U32 bak_zoomfactor_x, bak_zoomfactor_y;

    if (mpConfig->voutConfigs.voutConfig[voutID].size_x == size_x && mpConfig->voutConfigs.voutConfig[voutID].size_y == size_y) {
        return ME_OK;
    }

    if (RectOutofRange(size_x, size_y, mpConfig->voutConfigs.voutConfig[voutID].pos_x, mpConfig->voutConfigs.voutConfig[voutID].pos_y, mpConfig->voutConfigs.voutConfig[voutID].width, mpConfig->voutConfigs.voutConfig[voutID].height, 0, 0)) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad parameters in CAmbaDspIOne::ChangeSize, vout id %d, size_x %d, size_y %d.\n", voutID, size_x, size_y);
        return ME_BAD_PARAM;
    }

    bak_size_x = mpConfig->voutConfigs.voutConfig[voutID].size_x;
    bak_size_y = mpConfig->voutConfigs.voutConfig[voutID].size_y;

    mpConfig->voutConfigs.voutConfig[voutID].size_x = size_x;
    mpConfig->voutConfigs.voutConfig[voutID].size_y = size_y;

    //zoom factor changes
    ratio_x = (float)mpConfig->voutConfigs.voutConfig[voutID].size_x/mpConfig->voutConfigs.src_size_x;
    ratio_y = (float)mpConfig->voutConfigs.voutConfig[voutID].size_y/mpConfig->voutConfigs.src_size_y;
    bak_zoomfactor_x = mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_x;
    bak_zoomfactor_y = mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_y;
    mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_x = (AM_U32)(ratio_x*0x10000);
    mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_y = (AM_U32)(ratio_y*0x10000);

    ret = updateVoutSetting(voutID);
    if (ret) {
        AM_ASSERT(0);
        AMLOG_ERROR("CAmbaDspIOne::ChangeSize error.\n");
        //restore back params
        mpConfig->voutConfigs.voutConfig[voutID].size_x = bak_size_x;
        mpConfig->voutConfigs.voutConfig[voutID].size_y = bak_size_y;
        mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_x = bak_zoomfactor_x;
        mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_y = bak_zoomfactor_y;
        return ME_OS_ERROR;
    }
    return ME_OK;
}

AM_ERR CAmbaDspIOne::ChangePosition(AM_UINT voutID, AM_INT pos_x, AM_INT pos_y)
{
    AUTO_LOCK(mpMutex);
    AM_INT ret;
    AM_INT bak_pos_x, bak_pos_y;

    if (mpConfig->voutConfigs.voutConfig[voutID].pos_x == pos_x && mpConfig->voutConfigs.voutConfig[voutID].pos_y == pos_y) {
        return ME_OK;
    }

    if (RectOutofRange(mpConfig->voutConfigs.voutConfig[voutID].size_x, mpConfig->voutConfigs.voutConfig[voutID].size_y, pos_x, pos_y, mpConfig->voutConfigs.voutConfig[voutID].width, mpConfig->voutConfigs.voutConfig[voutID].height, 0, 0)) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad parameters in CAmbaDspIOne::ChangeSizePosition, vout id %d, pos_x %d, pos_y %d.\n", voutID, pos_x, pos_y);
        return ME_BAD_PARAM;
    }

    bak_pos_x = mpConfig->voutConfigs.voutConfig[voutID].pos_x;
    bak_pos_y = mpConfig->voutConfigs.voutConfig[voutID].pos_y;

    mpConfig->voutConfigs.voutConfig[voutID].pos_x = pos_x;
    mpConfig->voutConfigs.voutConfig[voutID].pos_y = pos_y;

    ret = updateVoutSetting(voutID);
    if (ret) {
        AM_ASSERT(0);
        AMLOG_ERROR("CAmbaDspIOne::ChangePosition error.\n");
        //restore back params
        mpConfig->voutConfigs.voutConfig[voutID].pos_x = bak_pos_x;
        mpConfig->voutConfigs.voutConfig[voutID].pos_y = bak_pos_y;
        return ME_OS_ERROR;
    }
    return ME_OK;
}

AM_ERR CAmbaDspIOne::ChangeSourceRect(AM_INT pos_x, AM_INT pos_y, AM_INT size_x, AM_INT size_y)
{
    AUTO_LOCK(mpMutex);
    AM_INT i, ret;
    float ratio_x,ratio_y;
    AM_U32 bak_zoomfactor_x[eVoutCnt], bak_zoomfactor_y[eVoutCnt];
    AM_ASSERT(mpConfig);
    if (!mpConfig || size_x <= 0 || size_y <= 0 || pos_x < 0 || pos_y < 0) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad parameters in CAmbaDspIOne::ChangeSourceRect,pos %d %d, size %d %d.\n", pos_x, pos_y, size_x, size_y);
        return ME_BAD_PARAM;
    }

    mpConfig->voutConfigs.input_center_x = pos_x + (size_x/2);
    mpConfig->voutConfigs.input_center_y = pos_y + (size_y/2);
    for (i=0; i<eVoutCnt; i++) {
        ratio_x = (float)mpConfig->voutConfigs.voutConfig[i].size_x/size_x;
        ratio_y = (float)mpConfig->voutConfigs.voutConfig[i].size_y/size_y;
        bak_zoomfactor_x[i] = mpConfig->voutConfigs.voutConfig[i].zoom_factor_x;
        bak_zoomfactor_y[i] = mpConfig->voutConfigs.voutConfig[i].zoom_factor_y;
        mpConfig->voutConfigs.voutConfig[i].zoom_factor_x = (AM_U32)(ratio_x*0x10000);
        mpConfig->voutConfigs.voutConfig[i].zoom_factor_y = (AM_U32)(ratio_y*0x10000);
    }

    if ((ret = updateAllVoutSetting()) != ME_OK) {
        //restore parameters
        AM_ASSERT(0);
        AMLOG_ERROR("CAmbaDspIOne::ChangeSourceRect fail, pos %d %d, size %d %d, ret %d.\n", pos_x, pos_y, size_x, size_y, ret);
        for (i=0; i<eVoutCnt; i++) {
            mpConfig->voutConfigs.voutConfig[i].zoom_factor_x = bak_zoomfactor_x[i];
            mpConfig->voutConfigs.voutConfig[i].zoom_factor_y = bak_zoomfactor_y[i];
        }
        mpConfig->voutConfigs.input_center_x = mpConfig->voutConfigs.src_pos_x + (mpConfig->voutConfigs.src_size_x/2);
        mpConfig->voutConfigs.input_center_y = mpConfig->voutConfigs.src_pos_y + (mpConfig->voutConfigs.src_size_y/2);
        return ME_ERROR;
    } else {
        mpConfig->voutConfigs.src_pos_x = pos_x;
        mpConfig->voutConfigs.src_pos_y = pos_y;
        mpConfig->voutConfigs.src_size_x = size_x;
        mpConfig->voutConfigs.src_size_y = size_y;
    }

    //check params for safe
    AM_ASSERT((mpConfig->voutConfigs.src_size_x + mpConfig->voutConfigs.src_pos_x*2) == (mpConfig->voutConfigs.input_center_x*2));
    AM_ASSERT((mpConfig->voutConfigs.src_size_y + mpConfig->voutConfigs.src_pos_y*2) == (mpConfig->voutConfigs.input_center_y*2));

    return ME_OK;
}

AM_ERR CAmbaDspIOne::Flip(AM_UINT voutID, AM_INT param)
{
    AUTO_LOCK(mpMutex);
    AM_INT ret;
    AM_INT bak_param;

    if (mpConfig->voutConfigs.voutConfig[voutID].flip == param) {
        return ME_OK;
    }

    bak_param = mpConfig->voutConfigs.voutConfig[voutID].flip;
    mpConfig->voutConfigs.voutConfig[voutID].flip = param;

    ret = updateVoutSetting(voutID);
    if (ret) {
        AM_ASSERT(0);
        AMLOG_ERROR("CAmbaDspIOne::Flip error.\n");
        //restore back params
        mpConfig->voutConfigs.voutConfig[voutID].flip = bak_param;
        return ME_OS_ERROR;
    }
    return ME_OK;
}

AM_ERR CAmbaDspIOne::Rotate(AM_UINT voutID, AM_INT param)
{
    AUTO_LOCK(mpMutex);
    AM_INT ret;
    AM_INT bak_param;

    if (mpConfig->voutConfigs.voutConfig[voutID].rotate == param) {
        return ME_OK;
    }

    bak_param = mpConfig->voutConfigs.voutConfig[voutID].rotate;
    mpConfig->voutConfigs.voutConfig[voutID].rotate = param;

    ret = updateVoutSetting(voutID);
    if (ret) {
        AM_ASSERT(0);
        AMLOG_ERROR("CAmbaDspIOne::Rotate error.\n");
        //restore back params
        mpConfig->voutConfigs.voutConfig[voutID].rotate = bak_param;
        return ME_OS_ERROR;
    }
    return ME_OK;
}

AM_ERR CAmbaDspIOne::Mirror(AM_UINT voutID, AM_INT param)
{
    AMLOG_ERROR("CAmbaDspIOne::Mirror no implement yet.\n");
    return ME_NO_IMPL;
}

AM_INT CAmbaDspIOne::updateVoutSetting(AM_UINT voutID)
{
    iav_udec_vout_configs_t iav_cvs;
    iav_udec_vout_config_t  vout_config;

    memset(&iav_cvs, 0x0, sizeof(iav_cvs));

    iav_cvs.vout_config = &vout_config;
    iav_cvs.num_vout = 1;
    iav_cvs.vout_config->vout_id= (u8)voutID;
    iav_cvs.vout_config->rotate = (u8)mpConfig->voutConfigs.voutConfig[voutID].rotate;
    iav_cvs.vout_config->flip = (u8)mpConfig->voutConfigs.voutConfig[voutID].flip;
    iav_cvs.vout_config->zoom_factor_x = mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_x;
    iav_cvs.vout_config->zoom_factor_y = mpConfig->voutConfigs.voutConfig[voutID].zoom_factor_y;
    iav_cvs.input_center_x = mpConfig->voutConfigs.input_center_x;
    iav_cvs.input_center_y = mpConfig->voutConfigs.input_center_y;
    iav_cvs.vout_config->win_offset_x = (u16)mpConfig->voutConfigs.voutConfig[voutID].pos_x;
    iav_cvs.vout_config->win_offset_y = (u16)mpConfig->voutConfigs.voutConfig[voutID].pos_y;
    iav_cvs.vout_config->win_width = (u16)mpConfig->voutConfigs.voutConfig[voutID].size_x;
    iav_cvs.vout_config->win_height = (u16)mpConfig->voutConfigs.voutConfig[voutID].size_y;
    iav_cvs.vout_config->target_win_offset_x = (u16)mpConfig->voutConfigs.voutConfig[voutID].pos_x;
    iav_cvs.vout_config->target_win_offset_y = (u16)mpConfig->voutConfigs.voutConfig[voutID].pos_y;
    iav_cvs.vout_config->target_win_width = (u16)mpConfig->voutConfigs.voutConfig[voutID].size_x;
    iav_cvs.vout_config->target_win_height = (u16)mpConfig->voutConfigs.voutConfig[voutID].size_y;

    AMLOG_INFO("CAmbaDspIOne::updateVoutSetting, vout_id %d:\n", voutID);
    AMLOG_INFO("  rotate %d, flip %d, win offset %d, %d, w %d h %d.\n", iav_cvs.vout_config[0].rotate, iav_cvs.vout_config[0].flip, iav_cvs.vout_config[0].win_offset_x, iav_cvs.vout_config[0].win_offset_y, iav_cvs.vout_config[0].win_width, iav_cvs.vout_config[0].win_height);
    AMLOG_INFO("  target offset %d, %d, w %d h %d, zoom factor 0x%x, 0x%x.\n", iav_cvs.vout_config[0].target_win_offset_x, iav_cvs.vout_config[0].target_win_offset_y, iav_cvs.vout_config[0].target_win_width,  iav_cvs.vout_config[0].target_win_height, iav_cvs.vout_config[0].zoom_factor_x, iav_cvs.vout_config[0].zoom_factor_y);
    return  ioctl(mIavFd, IAV_IOC_UPDATE_VOUT_CONFIG, &iav_cvs);
}

AM_INT CAmbaDspIOne::updateAllVoutSetting()
{
    AM_UINT i;

    iav_udec_vout_configs_t iav_cvs;
    iav_udec_vout_config_t  vout_config[2];
    memset(&iav_cvs, 0x0, sizeof(iav_cvs));
    iav_cvs.vout_config = &vout_config[0];
    iav_cvs.num_vout = 2;//hard code here
    AM_ASSERT(mpConfig);

    for(i = 0;i < iav_cvs.num_vout;i++){
        iav_cvs.vout_config[i].vout_id= i;
        iav_cvs.vout_config[i].rotate = (u8)mpConfig->voutConfigs.voutConfig[i].rotate;
        iav_cvs.vout_config[i].flip = (u8)mpConfig->voutConfigs.voutConfig[i].flip;
        iav_cvs.vout_config[i].win_offset_x = (u16)mpConfig->voutConfigs.voutConfig[i].pos_x;
        iav_cvs.vout_config[i].win_offset_y = (u16)mpConfig->voutConfigs.voutConfig[i].pos_y;
        iav_cvs.vout_config[i].win_width = (u16)mpConfig->voutConfigs.voutConfig[i].size_x;
        iav_cvs.vout_config[i].win_height = (u16)mpConfig->voutConfigs.voutConfig[i].size_y;
        iav_cvs.vout_config[i].target_win_offset_x = (u16)mpConfig->voutConfigs.voutConfig[i].pos_x;
        iav_cvs.vout_config[i].target_win_offset_y = (u16)mpConfig->voutConfigs.voutConfig[i].pos_y;
        iav_cvs.vout_config[i].target_win_width = (u16)mpConfig->voutConfigs.voutConfig[i].size_x;
        iav_cvs.vout_config[i].target_win_height = (u16)mpConfig->voutConfigs.voutConfig[i].size_y;

        iav_cvs.vout_config[i].zoom_factor_x = mpConfig->voutConfigs.voutConfig[i].zoom_factor_x;
        iav_cvs.vout_config[i].zoom_factor_y = mpConfig->voutConfigs.voutConfig[i].zoom_factor_y;
        AMLOG_INFO("CAmbaDspIOne::updateAllVoutSetting, vout_id %d:\n", i);
        AMLOG_INFO("  rotate %d, flip %d, win offset %d, %d, w %d h %d.\n", iav_cvs.vout_config[i].rotate, iav_cvs.vout_config[i].flip, iav_cvs.vout_config[i].win_offset_x, iav_cvs.vout_config[i].win_offset_y, iav_cvs.vout_config[i].win_width, iav_cvs.vout_config[i].win_height);
        AMLOG_INFO("  target offset %d, %d, w %d h %d, zoom factor 0x%x, 0x%x.\n", iav_cvs.vout_config[i].target_win_offset_x, iav_cvs.vout_config[i].target_win_offset_y, iav_cvs.vout_config[i].target_win_width,  iav_cvs.vout_config[i].target_win_height, iav_cvs.vout_config[i].zoom_factor_x, iav_cvs.vout_config[i].zoom_factor_y);
    }
    iav_cvs.input_center_x = mpConfig->voutConfigs.input_center_x;
    iav_cvs.input_center_y = mpConfig->voutConfigs.input_center_y;

    return ioctl(mIavFd, IAV_IOC_UPDATE_VOUT_CONFIG, &iav_cvs);
}

//nvr
//default layout is table
AM_ERR CAmbaDspIOne::InitWindowRender(CGConfig* pConfig)
{
    AM_INT sdWin = 0;
    AM_INT sdUdec = mTotalUDECNumber - (mpConfig->hdWin ? 1 : 0);
    setupVoutConfig(mpSharedRes);
    mGFlag = pConfig->globalFlag;
    //we must deside how many windows we need, for hihd all the screen
    if(sdUdec <= 0)
        return ME_BAD_PARAM;
    if (sdUdec == 1) {
        sdWin = 1;
    } else if (sdUdec <= 4) {
        sdWin = 4;
    } else if (sdUdec <= 6) {
        sdWin = 6;
    } else if (sdUdec <= 9){
        sdWin = 9;
    } else {
        sdWin = 16;
    }
    iavWindows.num_configs = sdWin + 1;
    iavRenders.num_configs = iavRenders.total_num_windows_to_render = sdWin;
    setMdecWindows(iavWindows.configs, iavWindows.num_configs, pConfig->mbUseCustomizedDisplayLayout, pConfig->mCustomizedDisplayLayout);
    setMdecRenders(iavRenders.configs, iavRenders.num_configs);

    AM_INT i = 0;
    CDspRenConfig* renConfig = &(pConfig->dspRenConfig);
    CDspWinConfig* winConfig = &(pConfig->dspWinConfig);
    udec_window_t* curWin = NULL;
    udec_render_t* curRen = NULL;

    winConfig->winNumConfiged = iavWindows.num_configs;
    for(; i < winConfig->winNumConfiged; i++)
    {
        curWin = iavWindows.configs + i;
        winConfig->winConfig[i].winOffsetX = curWin->target_win_offset_x;
        winConfig->winConfig[i].winOffsetY = curWin->target_win_offset_y;
        winConfig->winConfig[i].winWidth = curWin->target_win_width;
        winConfig->winConfig[i].winHeight = curWin->target_win_height;
    }

    renConfig->renNumConfiged = iavRenders.num_configs;
    for(i = 0; i < iavRenders.num_configs; i++)
    {
        curRen = iavRenders.configs + i;
        AM_ASSERT(curRen->render_id == renConfig->renConfig[i].renIndex);
        renConfig->renConfig[i].dspIndex = curRen->udec_id;
        renConfig->renConfig[i].winIndex = curRen->win_config_id;
    }
    return ME_OK;
}

AM_ERR CAmbaDspIOne::ConfigWindowRender(CGConfig* pConfig)
{
    AM_INT i = 0;
    CDspRenConfig* renConfig = &(pConfig->dspRenConfig);
    CDspWinConfig* winConfig = &(pConfig->dspWinConfig);
    udec_window_t* curWin = NULL;
    udec_render_t* curRen = NULL;
    AM_BOOL winSeted = AM_FALSE;
    AM_BOOL renSeted = AM_FALSE;
//    AM_INT renNum = 0;

    if(winConfig->winChanged == AM_TRUE)
    {
        AM_INFO("ConfigWindowRender window Num:%d\n", winConfig->winNumNeedConfig);
        //if(winConfig->winNum > mTotalUDECNumber)
        //    return ME_ERROR;
        iavWindows.num_configs = winConfig->winNumNeedConfig;
        for(; i < winConfig->winNumNeedConfig; i++)
        {
            curWin = iavWindows.configs + i;
            curWin->target_win_offset_x = winConfig->winConfig[i].winOffsetX;
            curWin->target_win_offset_y = winConfig->winConfig[i].winOffsetY;
            curWin->target_win_width = winConfig->winConfig[i].winWidth;
            curWin->target_win_height = winConfig->winConfig[i].winHeight;
        }
        winSeted = AM_TRUE;
        winConfig->winNumConfiged = winConfig->winNumNeedConfig;
    }

//    AM_INT k = 0;
    if(renConfig->renChanged == AM_TRUE)
    {
        AM_INFO("ConfigWindowRender render Num:%d\n", renConfig->renNumNeedConfig);
        AM_ASSERT(renConfig->renNumNeedConfig >= 0 && renConfig->renNumNeedConfig <= winConfig->winNumConfiged);
        iavRenders.total_num_windows_to_render = renConfig->renNumNeedConfig;
        iavRenders.num_configs = renConfig->renNumNeedConfig;
        for(i = 0; i < renConfig->renNumNeedConfig; i++)
        {
            curRen = iavRenders.configs + i;
            curRen->render_id = renConfig->renConfig[i].renIndex;
            curRen->udec_id = renConfig->renConfig[i].dspIndex;
            curRen->win_config_id = renConfig->renConfig[i].winIndex;
            curRen->win_config_id_2nd = renConfig->renConfig[i].winIndex2;
        }
        renSeted = AM_TRUE;
        renConfig->renNumConfiged = renConfig->renNumNeedConfig;
    }
    //We have assigned. TRICK: first will be configed on Init..
    renConfig->renNumNeedConfig = 0;
    winConfig->winNumNeedConfig = 0;
    renConfig->renChanged = AM_FALSE;
    winConfig->winChanged = AM_FALSE;

    if(winSeted == AM_TRUE){
        //AM_ASSERT(0);
        AM_INFO("Config NVR Windows!\n");
        if (ioctl(mIavFd, IAV_IOC_POSTP_WINDOW_CONFIG, &iavWindows) < 0) {
            perror("IAV_IOC_POSTP_WINDOW_CONFIG");
            return ME_OS_ERROR;
        }
    }
    if(renSeted == AM_TRUE){
        AM_INFO("Config NVR Render!\n");
        if (ioctl(mIavFd, IAV_IOC_POSTP_RENDER_CONFIG, &iavRenders) < 0) {
            perror("IAV_IOC_POSTP_RENDER_CONFIG");
            return ME_OS_ERROR;
        }
    }
    AM_INFO("Config Done!\n");
    return ME_OK;
}

AM_ERR CAmbaDspIOne::SwitchStream(AM_INT renIndex, AM_INT dspIndex, AM_INT flag)
{
    if(mGFlag & NOTHING_DONOTHING)
        return ME_OK;
    if(renIndex < 0 || renIndex >= mTotalUDECNumber)
        return ME_BAD_PARAM;
    iav_postp_stream_switch_t switchCmd;
    memset(&switchCmd, 0, sizeof(iav_postp_stream_switch_t));
    switchCmd.num_config = 1;
    switchCmd.switch_config[0].render_id = renIndex;//render 0 for hd , and 1~totalnum for sd(setMdecRenderer)
    switchCmd.switch_config[0].new_udec_id = dspIndex;//the last one for hd udec-instance.
    switchCmd.switch_config[0].seamless = flag;
    AM_INFO("Config Nvr Switch : on Render %d, newDspIndex: %d!(Seamless:%d)\n", renIndex, dspIndex, flag);
    if (ioctl(mIavFd, IAV_IOC_POSTP_STREAM_SWITCH, &switchCmd) < 0) {
        perror("IAV_IOC_POSTP_WINDOW_CONFIG");
        return ME_OS_ERROR;
    }
    iav_wait_stream_switch_msg_t waitCmd;
    memset(&waitCmd, 0, sizeof(iav_wait_stream_switch_msg_t));
    waitCmd.render_id = renIndex;
    AM_INFO("Wait Dsp Done the Switch Cmd\n");
    if (ioctl(mIavFd, IAV_IOC_WAIT_STREAM_SWITCH_MSG, &waitCmd) < 0) {
        perror("IAV_IOC_WAIT_STREAM_SWITCH_MSG");
        //return ME_OS_ERROR;
    }
    AM_INFO("Check Dsp wait Switch ack::Succes:%d, RenderID:%d\n",
        waitCmd.switch_status, waitCmd.render_id);
    return ME_OK;
}

//query UDEC state
AM_ERR CAmbaDspIOne::GetUDECState2(AM_INT udecIndex, AM_U8** dspRead )
{
    AM_INT ret = 0;
    iav_udec_state_t state;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspIOne::GetUDECState.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    memset(&state, 0, sizeof(state));
    state.decoder_id = (AM_U8)udecIndex;
    state.flags = IAV_UDEC_STATE_DSP_READ_POINTER;

    ret = ioctl(mIavFd, IAV_IOC_GET_UDEC_STATE, &state);
    if (ret) {
        perror("IAV_IOC_GET_UDEC_STATE");
        AMLOG_ERROR("IAV_IOC_GET_UDEC_STATE %d.\n", ret);
        return ME_ERROR;
    }
    //AM_INFO("dEBUG: %p\n", state.dsp_current_read_bitsfifo_addr);
    *dspRead = state.dsp_current_read_bitsfifo_addr;
    if (state.udec_state == IAV_UDEC_STATE_ERROR) {
        AM_ASSERT(0);
        return ME_UDEC_ERROR;
    }

    return ME_OK;
}

AM_ERR CAmbaDspIOne::DspBufferControl(AM_INT udecIndex, AM_UINT control, AM_UINT control_time)
{
    AM_INT ret = 0;
    iav_postp_buffering_control_t buffer_control;
    memset(&buffer_control, 0, sizeof(buffer_control));

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad udecIndex(%d) in CAmbaDspIOne::DspBufferControl.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    buffer_control.stream_id = udecIndex;
    buffer_control.control_direction = control;
    buffer_control.frame_time = control_time;

    ret = ioctl(mIavFd, IAV_IOC_POSTP_BUFFERING_CONTROL, &buffer_control);
    if (ret){
        perror("IAV_IOC_POSTP_BUFFERING_CONTROL");
        AMLOG_ERROR("IAV_IOC_POSTP_BUFFERING_CONTROL %d.\n", ret);
        return ME_ERROR;
    }

    return ME_OK;
}
//END nvr

AM_ERR CAmbaDspIOne::PlaybackZoom(AM_INT renderIndex, AM_U16 input_win_width, AM_U16 input_win_height, AM_U16 center_x, AM_U16 center_y)
{
    int ret;
    iav_udec_zoom_t zoom;
    memset(&zoom, 0x0, sizeof(zoom));

    zoom.render_id = renderIndex;
    zoom.input_center_x = center_x;
    zoom.input_center_y = center_y;
    zoom.input_width = input_win_width;
    zoom.input_height = input_win_height;

    AMLOG_INFO("[cmd flow]: before IAV_IOC_UDEC_ZOOM(mode 2), render_id %d, input width %d, height %d, center x %d, center y %d.\n", zoom.render_id, zoom.input_width, zoom.input_height, zoom.input_center_x, zoom.input_center_y);
    if ((ret = ioctl(mIavFd, IAV_IOC_UDEC_ZOOM, &zoom)) < 0) {
        perror("IAV_IOC_UDEC_ZOOM");
        return ME_OS_ERROR;
    }
    AMLOG_INFO("[cmd flow]: after IAV_IOC_UDEC_ZOOM.\n");

    return ME_OK;
}

AM_ERR CAmbaDspIOne::TrickMode(AM_INT udecIndex, AM_UINT speed, AM_UINT backward)
{
    AM_INT ret;
    iav_udec_pb_speed_t speed_t;
    memset(&speed_t, 0x0, sizeof(speed_t));

    speed_t.speed = (speed & 0xffff);
    speed_t.decoder_id = udecIndex;
    speed_t.scan_mode = (speed >> 16) & 0x3;
    speed_t.direction = backward;

    AMLOG_INFO("before IAV_IOC_UDEC_PB_SPEED, decoder_id %d, speed 0x%04x, scan_mode %d, direction %d.\n", speed_t.decoder_id, speed_t.speed, speed_t.scan_mode, speed_t.direction);
    if ((ret = ioctl(mIavFd, IAV_IOC_UDEC_PB_SPEED, &speed_t)) < 0) {
        perror("IAV_IOC_UDEC_PB_SPEED");
        return ME_ERROR;
    }
    AMLOG_INFO("after IAV_IOC_UDEC_PB_SPEED.\n");

    return ME_OK;
}
#if 0
static void _set_vout_for_decoder(iav_duplex_init_decoder_t *decoder_t, iav_decoder_vout_config_t *vout, AM_UINT number_vout, SVoutConfig* vout_config)
{
    AM_UINT i = 0;
    memset(vout, 0, sizeof(iav_decoder_vout_config_t)*number_vout);
    decoder_t->display_config.num_vout = number_vout;
    decoder_t->display_config.vout_config = vout;

    for (i = 0; i < number_vout; i++) {
        vout[i].vout_id = i;
        vout[i].target_win_offset_x = 0;
        vout[i].target_win_offset_y = 0;
        vout[i].target_win_width = vout_config[i].size_x;
        vout[i].target_win_height = vout_config[i].size_y;
        vout[i].zoom_factor_x = vout_config[i].zoom_factor_x;
        vout[i].zoom_factor_y = vout_config[i].zoom_factor_y;
    }
}
#endif
//simple api
AM_ERR CIOneDuplexSimpleDecAPI::InitDecoder(AM_UINT& dec_id, SDecoderParam* param, AM_UINT number_vout, SVoutConfig* vout_config)
{
    AM_UINT i = 0;
    iav_duplex_init_decoder_t decoder_t;
    iav_decoder_vout_config_t vout[eVoutCnt];
    AM_ASSERT(number_vout <= eVoutCnt);
    AM_ASSERT(mIavFd >= 0);

    if (number_vout > 2) {
        AM_ERROR("number_vout(%d) exceed max value 2.\n", number_vout);
        number_vout = 2;
    }

    memset(&decoder_t, 0, sizeof(decoder_t));

    decoder_t.dec_id = 0;
    decoder_t.dec_type = mDecType;

    decoder_t.u.h264.enable_pic_info = 0;
    decoder_t.u.h264.use_tiled_dram = 1;

    //decoder_t.u.h264.rbuf_smem_size = 0;
    //decoder_t.u.h264.fbuf_dram_size = 0;
    //decoder_t.u.h264.pjpeg_buf_size = 0;
    decoder_t.u.h264.rbuf_smem_size = 0;
    decoder_t.u.h264.fbuf_dram_size = 40*1024*1024;
    decoder_t.u.h264.pjpeg_buf_size = 10*1024*1024;

    decoder_t.u.h264.svc_fbuf_dram_size = 0;
    decoder_t.u.h264.svc_pjpeg_buf_size = 0;
    decoder_t.u.h264.cabac_2_recon_delay = 0;
    decoder_t.u.h264.force_fld_tiled = 1;
    decoder_t.u.h264.ec_mode = 1;
    decoder_t.u.h264.svc_ext = 0;
    decoder_t.u.h264.warp_enable = 0;
    decoder_t.u.h264.max_frm_num_of_dpb = 0;	//18;
    decoder_t.u.h264.max_frm_buf_width = 0;	//1920;
    decoder_t.u.h264.max_frm_buf_height = 0;	//1088;

    AMLOG_INFO("set vout number_vout %d.\n", number_vout);
    for (i = 0; i< number_vout; i++) {
        vout[i].vout_id = vout_config[i].vout_id;
        vout[i].udec_id = dec_id;
        vout[i].disable = 0;
        vout[i].flip = vout_config[i].flip;
        vout[i].rotate = vout_config[i].rotate;
        vout[i].win_width = vout[i].target_win_width = vout_config[i].size_x;
        vout[i].win_height = vout[i].target_win_height = vout_config[i].size_y;
        vout[i].win_offset_x = vout[i].target_win_offset_x = vout_config[i].pos_x;
        vout[i].win_offset_y = vout[i].target_win_offset_y = vout_config[i].pos_y;
        vout[i].zoom_factor_x = vout[i].zoom_factor_y = 1;
        AMLOG_INFO("  vout %d: vout_id %d, udec_id %d, flip %d, rotate %d.\n", i, vout[i].vout_id, vout[i].udec_id, vout[i].flip, vout[i].rotate);
        AMLOG_INFO("                win_width %d, win_height %d, win_offset_x %d, win_offset_y %d.\n", vout[i].win_width, vout[i].win_height, vout[i].win_offset_x, vout[i].win_offset_y);
    }
    decoder_t.display_config.num_vout = number_vout;
    decoder_t.display_config.vout_config = &vout[0];

    decoder_t.display_config.first_pts_low = 0;
    decoder_t.display_config.first_pts_high = 0;
    decoder_t.display_config.input_center_x = param->pic_width / 2;
    decoder_t.display_config.input_center_y = param->pic_height / 2;

    if (ioctl(mIavFd, IAV_IOC_DUPLEX_INIT_DECODER, &decoder_t) < 0) {
        perror("IAV_IOC_DUPLEX_INIT_DECODER");
        AM_ERROR("IAV_IOC_DUPLEX_INIT_DECODER error.\n");
        return ME_ERROR;
    }

    param->bits_fifo_start = decoder_t.bits_fifo_start;
    param->bits_fifo_size = decoder_t.bits_fifo_size;


    mDSPMode = param->dsp_mode;
    mDecType = param->codec_type;
    dec_id = 0;//hard code

    AM_ASSERT(DSPMode_DuplexLowdelay == mDSPMode);
    AM_ASSERT(UDEC_H264 == mDecType);

    return ME_OK;
}


AM_ERR CIOneDuplexSimpleDecAPI::ReleaseDecoder(AM_UINT dec_id)
{
    AM_ASSERT(mIavFd >= 0);
    AM_ASSERT(DSPMode_DuplexLowdelay == mDSPMode);
    AM_ASSERT(UDEC_H264 == mDecType);

    if (ioctl(mIavFd, IAV_IOC_DUPLEX_RELEASE_DECODER, dec_id) < 0) {
        perror("IAV_IOC_DUPLEX_RELEASE_DECODER");
        AM_ERROR("IAV_IOC_DUPLEX_RELEASE_DECODER error.\n");
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CIOneDuplexSimpleDecAPI::RequestBitStreamBuffer(AM_UINT dec_id, AM_U8* pstart, AM_UINT room)
{
    AM_ASSERT(mIavFd >= 0);
    AM_ASSERT(DSPMode_DuplexLowdelay == mDSPMode);
    AM_ASSERT(UDEC_H264 == mDecType);
    AM_INT ret;

    iav_wait_decoder_t wait;
    wait.flags = IAV_WAIT_BSB;
    wait.decoder_id = dec_id;
    wait.emptiness.start_addr = pstart;
    wait.emptiness.room = room;

    if ((ret = ioctl(mIavFd, IAV_IOC_DUPLEX_WAIT_DECODER, &wait)) < 0) {
        perror("IAV_IOC_DUPLEX_WAIT_DECODER");
        AM_ERROR("IAV_IOC_DUPLEX_WAIT_DECODER error.\n");
        if ((-EPERM) == ret) {
            return ME_UDEC_ERROR;
        }
        return ME_ERROR;
    }
    return ME_OK;
}

AM_ERR CIOneDuplexSimpleDecAPI::Decode(AM_UINT dec_id, AM_U8* pstart, AM_U8* pend)
{
    AM_ASSERT(mIavFd >= 0);
    AM_ASSERT(DSPMode_DuplexLowdelay == mDSPMode);
    AM_ASSERT(UDEC_H264 == mDecType);
    AM_INT ret;

    iav_duplex_decode_t param;
    memset(&param, 0, sizeof(param));

    param.dec_id = dec_id;
    if (UDEC_H264 == mDecType) {
        param.dec_type = UDEC_H264;
        param.u.h264.start_addr = pstart;
        param.u.h264.end_addr = pend;
        param.u.h264.first_pts_high = 0;//hard code
        param.u.h264.first_pts_low = 0;
        param.u.h264.num_pics = 1;//hard code
        param.u.h264.num_frame_decode = 0;
    } else {
        AM_ERROR("Add implementation.\n");
    }

    if ((ret = ioctl(mIavFd, IAV_IOC_DUPLEX_DECODE, &param)) < 0) {
        AM_ERROR("IAV_IOC_DUPLEX_DECODE error.\n");
        perror("IAV_IOC_DUPLEX_DECODE");
        if ((-EPERM) == ret) {
            return ME_UDEC_ERROR;
        }
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CIOneDuplexSimpleDecAPI::Stop(AM_UINT dec_id, unsigned int stop_flag)
{
    AM_ASSERT(mIavFd >= 0);
    AM_ASSERT(DSPMode_DuplexLowdelay == mDSPMode);
    AM_ASSERT(UDEC_H264 == mDecType);

    iav_duplex_start_decoder_t stop;
    stop.dec_id = dec_id;
    stop.stop_flag = stop_flag;
    if (ioctl(mIavFd, IAV_IOC_DUPLEX_STOP_DECODER, &stop) < 0) {
        perror("IAV_IOC_DUPLEX_STOP_DECODER");
        AM_ERROR("IAV_IOC_DUPLEX_STOP_DECODER error.\n");
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CIOneDuplexSimpleDecAPI::TrickPlay(AM_UINT dec_id, AM_UINT trickplay_flag)
{
    AM_ASSERT(mIavFd >= 0);
    AM_ASSERT(DSPMode_DuplexLowdelay == mDSPMode);
    AM_ASSERT(UDEC_H264 == mDecType);
    iav_duplex_trick_play_t trickplay;

    trickplay.dec_id = dec_id;
    trickplay.tp_mode = trickplay_flag;
    if (ioctl(mIavFd, IAV_IOC_DUPLEX_DUPLEX_TRICK_PLAY, &trickplay) < 0) {
        perror("IAV_IOC_DUPLEX_DUPLEX_TRICK_PLAY");
        AM_ERROR("IAV_IOC_DUPLEX_DUPLEX_TRICK_PLAY error.\n");
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CIOneUdecSimpleDecAPI::InitDecoder(AM_UINT& dec_id, SDecoderParam* param, AM_UINT number_vout, SVoutConfig* vout_config)
{
    AM_UINT i;
    iav_udec_info_ex_t mUdecInfo;
    iav_udec_vout_config_t mUdecVoutConfig[eVoutCnt];

    AM_ASSERT(number_vout);
    AM_ASSERT(vout_config);
    memset(&mUdecInfo, 0, sizeof(mUdecInfo));
    memset(&mUdecVoutConfig[0], 0, eVoutCnt * sizeof(iav_udec_vout_config_t));

    for (i = 0; i< number_vout; i++) {
        mUdecVoutConfig[i].vout_id = vout_config[i].vout_id;
        mUdecVoutConfig[i].udec_id = dec_id;
        mUdecVoutConfig[i].disable = 0;
        mUdecVoutConfig[i].flip = vout_config[i].flip;
        mUdecVoutConfig[i].rotate = vout_config[i].rotate;
        mUdecVoutConfig[i].win_width = mUdecVoutConfig[i].target_win_width = vout_config[i].size_x;
        mUdecVoutConfig[i].win_height = mUdecVoutConfig[i].target_win_height = vout_config[i].size_y;
        mUdecVoutConfig[i].win_offset_x = mUdecVoutConfig[i].target_win_offset_x = vout_config[i].pos_x;
        mUdecVoutConfig[i].win_offset_y = mUdecVoutConfig[i].target_win_offset_y = vout_config[i].pos_y;
        mUdecVoutConfig[i].zoom_factor_x = mUdecVoutConfig[i].zoom_factor_y = 1;
    }

    mUdecInfo.udec_id = dec_id;
    mUdecInfo.enable_err_handle = mpSharedRes->dspConfig.errorHandlingConfig[dec_id].enable_udec_error_handling;

    mUdecInfo.enable_pp = 1;
    mUdecInfo.enable_deint = mpSharedRes->dspConfig.enableDeinterlace;
    mUdecInfo.interlaced_out = 0;
    mUdecInfo.out_chroma_format = (1 == mpSharedRes->mDSPmode) ? 1:0;//out_chroma_format &packed_out control the out fmt
    mUdecInfo.packed_out = 0;
    if(mpSharedRes->force_decode){
        mUdecInfo.other_flags |= IAV_UDEC_FORCE_DECODE;
    }
    if(mpSharedRes->validation_only){//designed for video editing
        mUdecInfo.other_flags |= IAV_UDEC_VALIDATION_ONLY;
    }
    if (mUdecInfo.enable_err_handle) {
        AM_ASSERT(mpSharedRes->dspConfig.errorHandlingConfig[dec_id].enable_udec_error_handling);
        mUdecInfo.concealment_mode = mpSharedRes->dspConfig.errorHandlingConfig[dec_id].error_concealment_mode;
        mUdecInfo.concealment_ref_frm_buf_id = mpSharedRes->dspConfig.errorHandlingConfig[dec_id].error_concealment_frame_id;
        AMLOG_PRINTF("enable udec error handling: concealment mode %d, frame id %d.\n",mUdecInfo.concealment_mode, mUdecInfo.concealment_ref_frm_buf_id);
    }

    mUdecInfo.vout_configs.num_vout = number_vout;
    mUdecInfo.vout_configs.vout_config = &mUdecVoutConfig[0];

    mUdecInfo.vout_configs.input_center_x = (mpSharedRes->dspConfig.voutConfigs.src_size_x/2) + mpSharedRes->dspConfig.voutConfigs.src_pos_x;
    mUdecInfo.vout_configs.input_center_y = (mpSharedRes->dspConfig.voutConfigs.src_size_y/2) + mpSharedRes->dspConfig.voutConfigs.src_pos_y;

    //default zoom factor
    for (i = 0; i< eVoutCnt; i ++) {
        mUdecInfo.vout_configs.vout_config[i].zoom_factor_x = 1;
        mUdecInfo.vout_configs.vout_config[i].zoom_factor_y = 1;
    }

    mUdecInfo.bits_fifo_size = 4*1024*1024;
    mUdecInfo.ref_cache_size = 0;
    mUdecInfo.udec_type = mDecType;

    switch (mDecType) {
        case UDEC_H264:
            mUdecInfo.u.h264.pjpeg_buf_size = 4*1024*1024;
            break;

        case UDEC_MP12:
        case UDEC_MP4H:
            mUdecInfo.u.mpeg.deblocking_flag = mpSharedRes->dspConfig.deblockingFlag;
            mUdecInfo.u.mpeg.pquant_mode = mpSharedRes->dspConfig.deblockingConfig.pquant_mode;
            for(i=0; i<32; i++ )
            {
                mUdecInfo.u.mpeg.pquant_table[i] = (AM_U8)mpSharedRes->dspConfig.deblockingConfig.pquant_table[i];
            }
            mUdecInfo.u.mpeg.is_avi_flag = mpSharedRes->is_avi_flag;
            AMLOG_INFO("MPEG12/4 deblocking_flag %d, pquant_mode %d.\n", mUdecInfo.u.mpeg.deblocking_flag, mUdecInfo.u.mpeg.pquant_mode);
            for (i = 0; i<4; i++) {
                AMLOG_INFO(" pquant_table[%d - %d]:\n", i*8, i*8+7);
                AMLOG_INFO(" %d, %d, %d, %d, %d, %d, %d, %d.\n", \
                    mUdecInfo.u.mpeg.pquant_table[i*8], mUdecInfo.u.mpeg.pquant_table[i*8+1], mUdecInfo.u.mpeg.pquant_table[i*8+2], mUdecInfo.u.mpeg.pquant_table[i*8+3], \
                    mUdecInfo.u.mpeg.pquant_table[i*8+4], mUdecInfo.u.mpeg.pquant_table[i*8+5], mUdecInfo.u.mpeg.pquant_table[i*8+6], mUdecInfo.u.mpeg.pquant_table[i*8+7] \
                );
            }
            AMLOG_INFO("MPEG4 is_avi_flag %d.\n", mUdecInfo.u.mpeg.is_avi_flag);
            break;

        case UDEC_VC1:
            break;

        default:
            AM_ERROR("udec type %d not implemented\n", mDecType);
            return ME_BAD_PARAM;
    }

    AMLOG_INFO("start IAV_IOC_INIT_UDEC [%d]....\n", dec_id);
    if (ioctl(mIavFd, IAV_IOC_INIT_UDEC, &mUdecInfo) < 0) {
        perror("IAV_IOC_INIT_UDEC");
        AM_ERROR("IAV_IOC_INIT_UDEC error.\n");
        return ME_OS_ERROR;
    }
    AMLOG_INFO("IAV_IOC_INIT_UDEC [%d] done.\n", dec_id);

    param->bits_fifo_start = mUdecInfo.bits_fifo_start;
    param->bits_fifo_size = mUdecInfo.bits_fifo_size;

    return ME_OK;
}

AM_ERR CIOneUdecSimpleDecAPI::ReleaseDecoder(AM_UINT dec_id)
{
    AMLOG_INFO("start ReleaseUdec %d...\n", dec_id);

    if (ioctl(mIavFd, IAV_IOC_RELEASE_UDEC, dec_id) < 0) {
        perror("IAV_IOC_DESTROY_UDEC");
        AM_ERROR("IAV_IOC_RELEASE_UDEC %d fail.\n", dec_id);
        return ME_OS_ERROR;
    }

    AMLOG_INFO("end ReleaseUdec\n");
    return ME_OK;
}

AM_ERR CIOneUdecSimpleDecAPI::RequestBitStreamBuffer(AM_UINT dec_id, AM_U8* pstart, AM_UINT room)
{
    iav_wait_decoder_t wait;
    AM_INT ret;

    wait.emptiness.room = room;
    wait.emptiness.start_addr = pstart;
    wait.flags = IAV_WAIT_BITS_FIFO;
    wait.decoder_id = dec_id;

    AMLOG_DEBUG("request start.\n");
    if ((ret = ::ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait)) < 0) {
        perror("IAV_IOC_WAIT_DECODER");
        AMLOG_ERROR("!!!!!IAV_IOC_WAIT_DECODER error, ret %d.\n", ret);
        if (ret == (-EPERM)) {
            return ME_UDEC_ERROR;
        }
        return ME_ERROR;
    }
    AMLOG_DEBUG("request done.\n");

    return ME_OK;
}

AM_ERR CIOneUdecSimpleDecAPI::Decode(AM_UINT dec_id, AM_U8* pstart, AM_U8* pend)
{
    AM_INT ret;
    iav_udec_decode_t dec;

    memset(&dec, 0, sizeof(dec));
    dec.udec_type = mDecType;
    dec.decoder_id = dec_id;
    dec.u.fifo.start_addr = pstart;
    dec.u.fifo.end_addr = pend;
    dec.num_pics = 1;
    AMLOG_BINARY("DecodeBuffer length %d, %x.\n", pend-pstart, *pstart);

    if ((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_DECODE, &dec)) < 0) {
        perror("IAV_IOC_UDEC_DECODE");
        AM_ERROR("!!!!!IAV_IOC_UDEC_DECODE error, ret %d.\n", ret);
        if (ret == (-EPERM)) {
            return ME_UDEC_ERROR;
        }
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CIOneUdecSimpleDecAPI::Stop(AM_UINT dec_id, AM_UINT flag)
{
    AM_INT ret = 0;
    AM_UINT stop_code = (((flag&0xff)<<24)|dec_id);

    AMLOG_INFO("IAV_IOC_UDEC_STOP start, 0x%x.\n",stop_code);
    if ((ret = ::ioctl(mIavFd, IAV_IOC_UDEC_STOP, stop_code)) < 0) {
        AM_ERROR("IAV_IOC_UDEC_STOP error %d.\n", ret);
        return ME_OS_ERROR;
    }

    AMLOG_INFO("IAV_IOC_UDEC_STOP done.\n");
    return ME_OK;
}

AM_ERR CIOneUdecSimpleDecAPI::TrickPlay(AM_UINT dec_id, AM_UINT trickplay_flag)
{
    iav_udec_trickplay_t trickplay;
    AM_INT ret;

    trickplay.decoder_id = dec_id;
    trickplay.mode = trickplay_flag;
    AM_PRINTF("start IAV_IOC_UDEC_TRICKPLAY.\n");

    ret = ioctl(mIavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
    if(!ret) {
        return ME_OK;
    } else {
        AM_ERROR("!!!!!IAV_IOC_UDEC_DECODE error, ret %d.\n", ret);
        if (ret == (-EPERM)) {
            return ME_UDEC_ERROR;
        }
        return ME_ERROR;
    }
}

AM_ERR CIOneDuplexSimpleEncAPI::InitEncoder(AM_UINT& enc_id, SEncoderParam* param)
{
    iav_duplex_init_encoder_t encoder_t;
    memset((void*)&encoder_t, 0x0, sizeof(encoder_t));

    AM_ASSERT(0 == enc_id);
    enc_id = 0; //hard code here
    encoder_t.enc_id = enc_id;
    mMainStreamEnabled = 1;

    encoder_t.profile_idc = param->profile;
    encoder_t.level_idc = param->level;
    encoder_t.num_mbrows_per_bitspart = 0;//default value

    encoder_t.encode_w_sz = param->enc_width;
    encoder_t.encode_h_sz = param->enc_height;
    encoder_t.encode_w_ofs = param->enc_offset_x;
    encoder_t.encode_h_ofs = param->enc_offset_y;

    encoder_t.second_stream_enabled = param->second_stream_enabled;
    encoder_t.second_encode_w_sz = param->second_enc_width;
    encoder_t.second_encode_h_sz = param->second_enc_height;
    encoder_t.second_encode_w_ofs = param->second_enc_offset_x;
    encoder_t.second_encode_h_ofs = param->second_enc_offset_y;
    encoder_t.second_average_bitrate = param->second_bitrate;
    if (encoder_t.second_stream_enabled) {
        mSecondStreamEnabled = 1;
    }

    encoder_t.M = param->M;
    encoder_t.N = param->N;
    encoder_t.idr_interval = param->idr_interval;

    encoder_t.gop_structure = param->gop_structure;
    encoder_t.numRef_P = param->numRef_P;
    encoder_t.numRef_B = param->numRef_B;

    encoder_t.use_cabac = param->use_cabac;
    encoder_t.quality_level = param->quality_level;

    encoder_t.average_bitrate = param->bitrate;
    encoder_t.vbr_setting = param->vbr_setting;

    //all zero for other fields

    if (ioctl(mIavFd, IAV_IOC_DUPLEX_INIT_ENCODER, &encoder_t) < 0) {
        perror("IAV_IOC_DUPLEX_INIT_ENCODER");
        AM_ERROR("IAV_IOC_DUPLEX_INIT_ENCODER fail.\n");
        return ME_ERROR;
    }

    param->bits_fifo_start = encoder_t.bits_fifo_start;
    param->bits_fifo_size = encoder_t.bits_fifo_size;

    return ME_OK;
}

AM_ERR CIOneDuplexSimpleEncAPI::ReleaseEncoder(AM_UINT enc_id)
{
    AM_ASSERT(0 == enc_id);
    if (ioctl(mIavFd, IAV_IOC_DUPLEX_RELEASE_ENCODER, 0) < 0) {
        perror("IAV_IOC_DUPLEX_RELEASE_ENCODER");
        AM_ERROR("IAV_IOC_DUPLEX_RELEASE_ENCODER fail.\n");
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CIOneDuplexSimpleEncAPI::GetBitStreamBuffer(AM_UINT enc_id, SBitDescs* p_desc)
{
    AM_INT ret = 0;
    AM_UINT i = 0;
    iav_duplex_bs_info_t read_t;
    memset((void*)&read_t, 0x0, sizeof(read_t));
    read_t.enc_id = 0;
    p_desc->status = 0;

    if ((ret = ioctl(mIavFd, IAV_IOC_DUPLEX_READ_BITS, &read_t)) < 0) {
        if ((-EBUSY) == ret) {
            AM_ERROR("read last frame, eos comes.\n");
            //eos
            p_desc->status |= DFlagLastFrame;
            p_desc->tot_desc_number = 0;
            return ME_OK;
        }
        perror("IOC_DUPLEX_READ_BITS");
        AM_ERROR("IOC_DUPLEX_READ_BITS fail.\n");
        return ME_OS_ERROR;
    }

    p_desc->tot_desc_number = read_t.count;
    AM_ASSERT(DUPLEX_NUM_USER_DESC == DMaxDescNumber);
    for (i=0; i<read_t.count; i++) {
        p_desc->desc[i].pic_type = read_t.desc[i].pic_type;
        p_desc->desc[i].pts = read_t.desc[i].PTS;
        p_desc->desc[i].pstart = (AM_U8*)read_t.desc[i].start_addr;
        p_desc->desc[i].size = read_t.desc[i].pic_size;

        p_desc->desc[i].frame_number = read_t.desc[i].frame_num;

        //ugly convert, fake enc_id
        if (DSP_ENC_STREAM_TYPE_FULL_RESOLUTION == read_t.desc[i].stream_id) {
            p_desc->desc[i].enc_id = 0;
        } else if (DSP_ENC_STREAM_TYPE_PIP_RESOLUTION == read_t.desc[i].stream_id) {
            p_desc->desc[i].enc_id = 1;
        } else {
            AM_ERROR("BAD stream_id %d.\n", read_t.desc[i].stream_id);
        }

        p_desc->desc[i].fb_id = INVALID_FD_ID;//avoid release, ugly here
    }

    return ME_OK;
}

AM_ERR CIOneDuplexSimpleEncAPI::Start(AM_UINT enc_id)
{
    AM_ASSERT(0 == enc_id);
    iav_duplex_start_encoder_t start;
    start.enc_id = enc_id;

    if (mMainStreamEnabled) {
        start.stream_type = DSP_ENC_STREAM_TYPE_FULL_RESOLUTION;
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_START_ENCODER, &start) < 0) {
            perror("IAV_IOC_DUPLEX_START_ENCODER");
            AM_ERROR("IAV_IOC_DUPLEX_START_ENCODER main stream failed.\n");
            return ME_ERROR;
        }
    }

    if (mSecondStreamEnabled) {
        start.stream_type = DSP_ENC_STREAM_TYPE_PIP_RESOLUTION;
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_START_ENCODER, &start) < 0) {
            perror("IAV_IOC_DUPLEX_START_ENCODER");
            AM_ERROR("IAV_IOC_DUPLEX_START_ENCODER second stream failed.\n");
            return ME_ERROR;
        }
    }

    return ME_OK;
}

AM_ERR CIOneDuplexSimpleEncAPI::Stop(AM_UINT enc_id, AM_UINT stop_flag)
{
    AM_ASSERT(0 == enc_id);
    iav_duplex_start_encoder_t stop;
    stop.enc_id = enc_id;

    if (mMainStreamEnabled) {
        stop.stream_type = DSP_ENC_STREAM_TYPE_FULL_RESOLUTION;
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_STOP_ENCODER, &stop) < 0) {
            perror("IAV_IOC_DUPLEX_STOP_ENCODER");
            AM_ERROR("IAV_IOC_DUPLEX_STOP_ENCODER main stream failed.\n");
            return ME_ERROR;
        }
    }

    if (mSecondStreamEnabled) {
        stop.stream_type = DSP_ENC_STREAM_TYPE_PIP_RESOLUTION;
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_STOP_ENCODER, &stop) < 0) {
            perror("IAV_IOC_DUPLEX_STOP_ENCODER");
            AM_ERROR("IAV_IOC_DUPLEX_STOP_ENCODER second stream failed.\n");
            return ME_ERROR;
        }
    }

    return ME_OK;
}

AM_ERR CIOneRecordSimpleEncAPI::InitEncoder(AM_UINT& enc_id, SEncoderParam* param)
{
//    AM_INT ret;

    mStreamMask = 0;
    mTotalStreamNumber = 0;
    //mEosFlag = 0;

    iav_enc_config_t mEncConfig[3];

    mEncConfig[0].flags = 0;
    mEncConfig[0].enc_id = enc_id;
    mEncConfig[0].encode_type = IAV_ENCODE_H264;
    mEncConfig[0].u.h264.M = param->M;
    mEncConfig[0].u.h264.N = param->N;
    mEncConfig[0].u.h264.idr_interval = param->idr_interval;
    mEncConfig[0].u.h264.gop_model = param->gop_structure;
    mEncConfig[0].u.h264.bitrate_control = param->vbr_setting;
    mEncConfig[0].u.h264.calibration = param->calibration;
    mEncConfig[0].u.h264.vbr_ness = param->vbr_ness;
    mEncConfig[0].u.h264.min_vbr_rate_factor = param->min_vbr_rate_factor;
    mEncConfig[0].u.h264.max_vbr_rate_factor = param->max_vbr_rate_factor;

    mEncConfig[0].u.h264.average_bitrate = param->bitrate;
    mEncConfig[0].u.h264.entropy_codec = param->use_cabac;

    if (param->second_stream_enabled) {
        mEncConfig[1].flags = 0;
        mEncConfig[1].enc_id = enc_id + 1;//hard code
        mEncConfig[1].encode_type = IAV_ENCODE_H264;
        mEncConfig[1].u.h264.M = param->M;
        mEncConfig[1].u.h264.N = param->N;
        mEncConfig[1].u.h264.idr_interval = param->idr_interval;
        mEncConfig[1].u.h264.gop_model = param->gop_structure;
        mEncConfig[1].u.h264.bitrate_control = param->vbr_setting;
        mEncConfig[1].u.h264.calibration = param->calibration;
        mEncConfig[1].u.h264.vbr_ness = param->vbr_ness;
        mEncConfig[1].u.h264.min_vbr_rate_factor = param->min_vbr_rate_factor;
        mEncConfig[1].u.h264.max_vbr_rate_factor = param->max_vbr_rate_factor;

        mEncConfig[1].u.h264.average_bitrate = param->second_bitrate;
        mEncConfig[1].u.h264.entropy_codec = param->use_cabac;
    }

    if (param->dsp_piv_enabled) {
        mEncConfig[2].flags = 0;
        mEncConfig[2].enc_id = enc_id + 2;//hard code
        mEncConfig[2].encode_type = IAV_ENCODE_MJPEG;

        mEncConfig[2].u.jpeg.chroma_format = 0;//hard code
        mEncConfig[2].u.jpeg.init_quality_level = 90;//hard code
        mEncConfig[2].u.jpeg.thumb_active_w = param->dsp_jpeg_active_win_w;
        mEncConfig[2].u.jpeg.thumb_active_h = param->dsp_jpeg_active_win_h;
        mEncConfig[2].u.jpeg.thumb_dram_w = param->dsp_jpeg_dram_w;
        mEncConfig[2].u.jpeg.thumb_dram_h = param->dsp_jpeg_dram_h;
    }

    if (::ioctl(mIavFd, IAV_IOC_ENCODE_SETUP, &mEncConfig[0]) < 0) {
        AM_PERROR("IAV_IOC_ENCODE_SETUP\n");
        AM_ERROR("IAV_IOC_ENCODE_SETUP\n");
        return ME_ERROR;
    }
    mStreamMask |= IAV_MAIN_STREAM;
    mTotalStreamNumber ++;

    AMLOG_INFO("IAV_IOC_ENCODE_SETUP main done, bit rate %u.\n", mEncConfig[0].u.h264.average_bitrate);

    if (param->second_stream_enabled) {
        if (::ioctl(mIavFd, IAV_IOC_ENCODE_SETUP, &mEncConfig[1]) < 0) {
            AM_PERROR("IAV_IOC_ENCODE_SETUP\n");
            AM_ERROR("IAV_IOC_ENCODE_SETUP\n");
            return ME_ERROR;
        }
        AMLOG_INFO("IAV_IOC_ENCODE_SETUP second done, bit rate %u.\n", mEncConfig[1].u.h264.average_bitrate);
        mStreamMask |= IAV_2ND_STREAM;
        mTotalStreamNumber ++;
    }

    if (param->dsp_piv_enabled) {
        AMLOG_INFO("IAV_IOC_ENCODE_SETUP third(piv) start, format %d, quality level %d, active w %d, h %d, dram w %d, h %d.\n", mEncConfig[2].u.jpeg.chroma_format, mEncConfig[2].u.jpeg.init_quality_level, mEncConfig[2].u.jpeg.thumb_active_w, mEncConfig[2].u.jpeg.thumb_active_h, mEncConfig[2].u.jpeg.thumb_dram_w, mEncConfig[2].u.jpeg.thumb_dram_h);
        if (::ioctl(mIavFd, IAV_IOC_ENCODE_SETUP, &mEncConfig[2]) < 0) {
            AM_PERROR("IAV_IOC_ENCODE_SETUP\n");
            AM_ERROR("IAV_IOC_ENCODE_SETUP\n");
            return ME_ERROR;
        }

        mStreamMask |= IAV_3RD_STREAM;
        mTotalStreamNumber ++;
    }

    AMLOG_INFO("CIOneRecordSimpleEncAPI::InitEncoder done.\n");
    return ME_OK;
}

AM_ERR CIOneRecordSimpleEncAPI::ReleaseEncoder(AM_UINT enc_id)
{
    return ME_OK;
}

AM_ERR CIOneRecordSimpleEncAPI::GetBitStreamBuffer(AM_UINT enc_id, SBitDescs* p_desc)
{
//    AM_UINT i = 0;
    AM_INT ret = 0;
    iav_frame_desc_t frame;

    p_desc->status = 0;
    p_desc->tot_desc_number = 0;
    memset(&frame, 0 , sizeof(iav_frame_desc_t));
    frame.enc_id = IAV_ENC_ID_MAIN;
    if(mTotalStreamNumber > 1)
        frame.enc_id = IAV_ENC_ID_ALL;

    while (1) {
        ret = ::ioctl(mIavFd, IAV_IOC_GET_ENCODED_FRAME, &frame);
        if (ret < 0) {
            if (ret == (-EINTR)) {
                continue;
            } else {
                perror("IAV_IOC_GET_ENCODED_FRAME");
                AM_ERROR("IAV_IOC_GET_ENCODED_FRAME error, ret %d.\n", ret);
                return ME_ERROR;
            }
        }
        break;
    }

    p_desc->tot_desc_number = 1;//hard code here
    p_desc->desc[0].pic_type = frame.pic_type;
    p_desc->desc[0].pts = frame.pts_64;
    p_desc->desc[0].pstart = (AM_U8*)frame.usr_start_addr;
    p_desc->desc[0].size = frame.pic_size;

    p_desc->desc[0].enc_id = frame.enc_id;
    p_desc->desc[0].fb_id = frame.fd_id;
    p_desc->desc[0].frame_number = frame.frame_num;

    if (frame.is_eos) {
        //AMLOG_WARN("enc_id %d, eos, mEosFlag 0x%x, mStreamMask 0x%x.\n", frame.enc_id, mEosFlag, mStreamMask);
        //mEosFlag |= 0x1 << frame.enc_id;
        //if (mEosFlag == mStreamMask) {
            //all eos
            AMLOG_WARN("all enc stream, eos.\n");
            p_desc->status |= DFlagLastFrame;
        //}
        return ME_OK;
    }

    p_desc->status |= DFlagNeedReleaseFrame;
#if 0
    //release it to iav driver
    if (ioctl(mIavFd, IAV_IOC_RELEASE_ENCODED_FRAME, &frame) < 0) {
        AM_ERROR("IAV_IOC_RELEASE_ENCODED_FRAME error.\n");
    }
#endif
    return ME_OK;
}

AM_ERR CIOneRecordSimpleEncAPI::Start(AM_UINT enc_id)
{
    AM_UINT i = 0;
    AM_INT ret, error =0;

    for (i = 0; i < mTotalStreamNumber; i++) {
        //skip jpeg, and mjpeg stream
        if (i >= 2) {
            continue;
        }
        AMLOG_INFO("before IAV_IOC_START_ENCODE %d.\n", i);
        ret = ioctl(mIavFd, IAV_IOC_START_ENCODE, i);
        if (ret < 0) {
            AM_ERROR("IAV_IOC_START_ENCODE %d, error, ret %d.\n", i, ret);
            error = 1;
        } else {
            AMLOG_INFO("IAV_IOC_START_ENCODE %d done.\n", i);
        }
    }

    if (!error) {
        return ME_OK;
    }
    return ME_ERROR;
}

AM_ERR CIOneRecordSimpleEncAPI::Stop(AM_UINT enc_id, AM_UINT stop_flag)
{
    AM_UINT i = 0;
    AM_INT ret, error =0;

    for (i = 0; i < mTotalStreamNumber; i++) {
        //skip jpeg, and mjpeg stream
        if (i >= 2) {
            continue;
        }
        AMLOG_INFO("before IAV_IOC_STOP_ENCODE %d.\n", i);
        ret = ioctl(mIavFd, IAV_IOC_STOP_ENCODE, i);
        if (ret < 0) {
            AM_ERROR("IAV_IOC_STOP_ENCODE %d, error, ret %d.\n", i, ret);
            error = 1;
        } else {
            AMLOG_INFO("before IAV_IOC_STOP_ENCODE %d done.\n", i);
        }
    }

    if (!error) {
        return ME_OK;
    }
    return ME_ERROR;
}

ISimpleDecAPI* CreateSimpleDecAPI(AM_INT iav_fd, AM_UINT dsp_mode, AM_UINT codec_type, void* p)
{
    ISimpleDecAPI* api = NULL;
    if (iav_fd >= 0) {
        switch (dsp_mode) {
            case DSPMode_UDEC:
                api = new CIOneUdecSimpleDecAPI(iav_fd, DSPMode_UDEC, codec_type, p);
                break;
            case DSPMode_DuplexLowdelay:
                api = new CIOneDuplexSimpleDecAPI(iav_fd, DSPMode_DuplexLowdelay, codec_type, p);
                break;
            default:
                AM_ERROR("BAD dsp_mode %d.\n", dsp_mode);
                break;
        }
    } else {
        AM_ERROR("BAD iav_fd %d.\n", iav_fd);
    }

    return api;
}

ISimpleEncAPI* CreateSimpleEncAPI(AM_INT iav_fd, AM_UINT dsp_mode, AM_UINT codec_type, void* p)
{
    ISimpleEncAPI* api = NULL;
    if (iav_fd >= 0) {
        switch (dsp_mode) {
            case DSPMode_CameraRecording:
                api = new CIOneRecordSimpleEncAPI(iav_fd, DSPMode_CameraRecording, UDEC_H264, p);
                break;
            case DSPMode_DuplexLowdelay:
                api = new CIOneDuplexSimpleEncAPI(iav_fd, DSPMode_DuplexLowdelay, UDEC_H264, p);
                break;
            default:
                AM_ERROR("BAD dsp_mode %d.\n", dsp_mode);
                break;
        }
    } else {
        AM_ERROR("BAD iav_fd %d.\n", iav_fd);
    }

    return api;
}

void DestroySimpleDecAPI(ISimpleDecAPI* p)
{
    if (p) {
        delete p;
    }
    return;
}

void DestroySimpleEncAPI(ISimpleEncAPI* p)
{
    if (p) {
        delete p;
    }
    return;
}

