/**
 * amba_dsp_a5s.cpp
 *
 * History:
 *  2011/07/05 - [Zhi He] created file
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

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "libavcodec/amba_dsp_define.h"
}

extern "C" {
#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#if PLATFORM_ANDROID
#include "vout.h"
#endif
}

#include "am_dsp_if.h"
#include "amba_dsp_common.h"
#include "amba_dsp_a5s.h"

//need mutex protect here? be caution to call this fuction
CAmbaDspA5s* CAmbaDspA5s::Create(SConsistentConfig* pShared)
{
    CAmbaDspA5s* mpInstance;
    if (!pShared) {
        AM_ASSERT(0);
        return NULL;
    }

    if (pShared->udecHandler) {
        AM_ASSERT(0);
        AM_ERROR("engine already has a udecHandler.\n");
        return (CAmbaDspA5s*)pShared->udecHandler;
    }

    AM_ASSERT(pShared->mIavFd < 0);
    AM_ASSERT(pShared->mbIavInited == 0);
    AM_ASSERT(pShared->voutHandler == NULL);

    mpInstance = new CAmbaDspA5s(pShared, &pShared->dspConfig);

    if(mpInstance && mpInstance->Construct() != ME_OK)
    {
        delete mpInstance;
        mpInstance = NULL;
    }

    pShared->udecHandler = (IUDECHandler*)mpInstance;
    pShared->voutHandler = (IVoutHandler*)mpInstance;

    return mpInstance;
}

CAmbaDspA5s::CAmbaDspA5s(SConsistentConfig* pShared, DSPConfig* pConfig):
    mpSharedRes(pShared),
    mpConfig(pConfig),
    mIavFd(-1),
    mbUDECMode(0),
    mbVoutSetup(0),
    mMaxVoutWidth(0),
    mMaxVoutHeight(0),
    mVoutConfigMask(3),
    mVoutStartIndex(0),
    mVoutNumber(0),
    mTotalUDECNumber(1)
{
    AM_UINT i = 0;
    for (i=0; i<DMAX_DECODER_INSTANCE_NUM_A5S; i++) {
        //mDecInfo[i].dec_format = UDEC_NONE;
        mbInstanceUsed[i] = 0;
    }
}

CAmbaDspA5s::~CAmbaDspA5s()
{
    if (mIavFd >= 0) {
        close(mIavFd);
    }
}

AM_ERR CAmbaDspA5s::Construct()
{
    AM_ERR err;
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

    //mLogLevel = LogDebugLevel;
    //mLogOption = 0xf;

    return ME_OK;
}

AM_INT CAmbaDspA5s::getAvaiableUdecinstance()
{
    AM_INT i = 0;
    for ( ;i < mTotalUDECNumber; i++) {
        if (mbInstanceUsed[i] == 0) {
            return i;
        }
    }

    return eInvalidUdecIndex;
}

#if 0
AM_ERR  CAmbaDspA5s::setVoutSize(AM_INT vout_id, AM_INT mode)
{
    size_t i;
    AM_ASSERT(mpConfig);
    AM_ASSERT(vout_id >= eVoutLCD);
    AM_ASSERT(vout_id < eVoutCnt);
    if (!mpConfig || vout_id < eVoutLCD || vout_id >= eVoutCnt) {
        AMLOG_ERROR("Bad params in setVoutSize, vout_id %d, mode %d, mpConfig %p.\n", vout_id, mode, mpConfig);
        return ME_BAD_PARAM;
    }

    for (i = 0; i < sizeof(G_vout_modes) / sizeof(G_vout_modes[0]); i++) {
        if (G_vout_modes[i].mode == mode) {
            mpConfig->voutConfigs.voutConfig[vout_id].width = G_vout_modes[i].width;
            mpConfig->voutConfigs.voutConfig[vout_id].height = G_vout_modes[i].height;
            mpConfig->voutConfigs.voutConfig[vout_id].size_x= G_vout_modes[i].width;
            mpConfig->voutConfigs.voutConfig[vout_id].size_y = G_vout_modes[i].height;
            return ME_OK;
        }
    }
    return ME_NOT_SUPPORTED;
}


AM_ERR CAmbaDspA5s::configVout (AM_INT vout_id, AM_INT mode, AM_INT sink_type)
{
    //For sink config
    AM_INT sink_id = -1;
    //Specify external device type for vout
    AM_ASSERT(mpConfig);
    AM_ASSERT(vout_id >= eVoutLCD);
    AM_ASSERT(vout_id < eVoutCnt);
    sink_id = mpConfig->voutConfigs.voutConfig[vout_id].sink_id;

    AMLOG_DEBUG("[DEBUG], IAV_IOC_VOUT_SELECT_DEV, sink_id %d.\n", sink_id);
    if (ioctl(mIavFd, IAV_IOC_VOUT_SELECT_DEV, sink_id) < 0) {
        AM_PERROR("IAV_IOC_VOUT_SELECT_DEV");
        switch (sink_type) {
            case AMBA_VOUT_SINK_TYPE_CVBS:
                AMLOG_PRINTF("No CVBS sink: ");
                AMLOG_PRINTF("Driver not loaded!\n");
                break;

            case AMBA_VOUT_SINK_TYPE_HDMI:
                AMLOG_PRINTF("No HDMI sink: ");
                AMLOG_PRINTF("Hdmi cable not plugged, ");
                AMLOG_PRINTF("driver not loaded, ");
                AMLOG_PRINTF("or hdmi output not supported!\n");
                break;

            default:
                break;
        }
        return ME_ERROR;
    }

    //Configure vout
    struct amba_video_sink_mode sink_cfg;
    memset(&sink_cfg, 0, sizeof(sink_cfg));
    sink_cfg.mode = mode;
    sink_cfg.ratio = AMBA_VIDEO_RATIO_AUTO;
    sink_cfg.bits = AMBA_VIDEO_BITS_AUTO;
    sink_cfg.type = sink_type;
    if (mode == AMBA_VIDEO_MODE_480I || mode == AMBA_VIDEO_MODE_576I
        || mode == AMBA_VIDEO_MODE_1080I
        || mode == AMBA_VIDEO_MODE_1080I_PAL)
        sink_cfg.format = AMBA_VIDEO_FORMAT_INTERLACE;
    else
        sink_cfg.format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

    sink_cfg.sink_type = AMBA_VOUT_SINK_TYPE_AUTO;
    sink_cfg.bg_color.y = 0x10;
    sink_cfg.bg_color.cb = 0x80;
    sink_cfg.bg_color.cr = 0x80;
    sink_cfg.lcd_cfg.mode = AMBA_VOUT_LCD_MODE_DISABLE;

    sink_cfg.id = sink_id;
    sink_cfg.frame_rate = AMBA_VIDEO_FPS_AUTO;
    sink_cfg.csc_en = 1;
    sink_cfg.hdmi_color_space = AMBA_VOUT_HDMI_CS_AUTO;
    sink_cfg.hdmi_3d_structure = DDD_RESERVED;
    sink_cfg.hdmi_overscan = AMBA_VOUT_HDMI_OVERSCAN_AUTO;
    sink_cfg.video_en = 1;

    //Options related to Video only
    struct amba_vout_video_size video_size = {0};
    struct amba_vout_video_offset video_offset = {0};

    setVoutSize(vout_id, mode);

    sink_cfg.video_size.video_width= mpConfig->voutConfigs.voutConfig[vout_id].size_x;
    sink_cfg.video_size.video_height = mpConfig->voutConfigs.voutConfig[vout_id].size_y;
    sink_cfg.video_size.vout_width = mpConfig->voutConfigs.voutConfig[vout_id].width;
    sink_cfg.video_size.vout_height = mpConfig->voutConfigs.voutConfig[vout_id].height;
    sink_cfg.video_size.specified = 1;

    sink_cfg.video_offset.offset_x = mpConfig->voutConfigs.voutConfig[vout_id].pos_x;
    sink_cfg.video_offset.offset_y = mpConfig->voutConfigs.voutConfig[vout_id].pos_y;
    sink_cfg.fb_id = -1;
    sink_cfg.video_rotate = AMBA_VOUT_ROTATE_NORMAL;

    //Options related to OSD only
    struct amba_vout_osd_rescale osd_rescale = {0};
    struct amba_vout_osd_offset osd_offset = {0};
    sink_cfg.osd_rotate = AMBA_VOUT_ROTATE_NORMAL;
    sink_cfg.osd_flip = AMBA_VOUT_FLIP_NORMAL;
    sink_cfg.osd_rescale = osd_rescale;
    sink_cfg.osd_offset = osd_offset;
    sink_cfg.display_input = AMBA_VOUT_INPUT_FROM_MIXER;

    AMLOG_DEBUG("[DEBUG], IAV_IOC_VOUT_CONFIGURE_SINK: sink_cfg: id %d.\n", sink_cfg.id);
    AMLOG_DEBUG("               mode %d, ratio %d, bits %d, type %d, format %d.\n", sink_cfg.mode, sink_cfg.ratio, sink_cfg.bits, sink_cfg.type, sink_cfg.format);
    AMLOG_DEBUG("               frame rate %d, csc en %d, bg_color y %d, cb %d, cr %d.\n", sink_cfg.frame_rate, sink_cfg.csc_en, sink_cfg.bg_color.y, sink_cfg.bg_color.cb, sink_cfg.bg_color.cr);
    AMLOG_DEBUG("               display_input %d, sink_type %d, video en y %d, flip %d, rotate %d.\n", sink_cfg.display_input, sink_cfg.sink_type, sink_cfg.video_en, sink_cfg.video_flip, sink_cfg.video_rotate);
    AMLOG_DEBUG("               video_width %d, video_height %d, vout_width %d, vout_height %d, spe %d.\n", sink_cfg.video_size.video_width, sink_cfg.video_size.video_height, sink_cfg.video_size.vout_width, sink_cfg.video_size.vout_height, sink_cfg.video_size.specified);
    AMLOG_DEBUG("               video_offset_x %d, video_offset_y %d, spe %d.\n", sink_cfg.video_offset.offset_x, sink_cfg.video_offset.offset_y, sink_cfg.video_offset.specified);
    AMLOG_DEBUG("               fb_id %d, osd_flip %d, osd_rotate %d, direct_to_dsp.\n", sink_cfg.fb_id, sink_cfg.osd_flip, sink_cfg.osd_rotate, sink_cfg.direct_to_dsp);
    AMLOG_DEBUG("               osd_w %d, osd_h %d, rescale_en %d, w %d, h %d.\n", sink_cfg.osd_size.width, sink_cfg.osd_size.height, sink_cfg.osd_rescale.enable, sink_cfg.osd_rescale.width, sink_cfg.osd_rescale.height);
    AMLOG_DEBUG("               lcd dclk_freq_hz %d, mode %d, model %d, seqb %d, seqt %d.\n", sink_cfg.lcd_cfg.dclk_freq_hz, sink_cfg.lcd_cfg.mode, sink_cfg.lcd_cfg.model, sink_cfg.lcd_cfg.seqb, sink_cfg.lcd_cfg.seqt);
    AMLOG_DEBUG("               hdmi_color_space %d, hdmi_3d_structure %d, hdmi_overscan %d.\n", sink_cfg.hdmi_color_space, sink_cfg.hdmi_3d_structure, sink_cfg.hdmi_overscan);

    if (ioctl(mIavFd, IAV_IOC_VOUT_CONFIGURE_SINK, &sink_cfg) < 0) {
        AM_PERROR("IAV_IOC_VOUT_CONFIGURE_SINK");
        return ME_ERROR;
    }

    AMLOG_PRINTF("init_vout done\n");
    return ME_OK;
}
#endif

//according to connected vouts
AM_INT CAmbaDspA5s::setupVoutConfig(SConsistentConfig* mpShared)
{
    AM_UINT i = 0;
    AM_INT ret = 0;
    DSPVoutConfig* pvout;

    if (!mpShared || (mpShared->mbIavInited==0) || (mpShared->mIavFd < 0)) {
        AM_ERROR("NULL mpShared pointer, or no iav handle in SetupVoutConfig.\n");
        return (-1);
    }
    mVoutNumber = 0;
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
        AM_ERROR("no VOUT valid.\n", i);
        return (-2);
    }

    if (mVoutConfigMask & (1<<eVoutLCD)) {
        mpShared->dspConfig.voutConfigs.vout_start_index = eVoutLCD;
    } else if (mVoutConfigMask & (1<<eVoutHDMI)) {
        mpShared->dspConfig.voutConfigs.vout_start_index = eVoutHDMI;
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

AM_INT CAmbaDspA5s::getVoutParams(AM_INT iavFd, AM_INT vout_id, DSPVoutConfig* pConfig)
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
    pConfig->sink_type = sink_type;
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


//--------------------------------------------------
//
//from IUDECHandler
//
//--------------------------------------------------

AM_ERR CAmbaDspA5s::enterUdecMode()
{
    AM_INT state;
    AM_INT i;

    mTotalUDECNumber = 1;//hard code here

    if (setupVoutConfig(mpSharedRes)) {
        //setup default vout settings
        SetupVOUT(AMBA_VOUT_SINK_TYPE_DIGITAL, AMBA_VIDEO_MODE_HVGA);
        SetupVOUT(AMBA_VOUT_SINK_TYPE_HDMI, AMBA_VIDEO_MODE_D1_NTSC);
        setupVoutConfig(mpSharedRes);
    }

    i = ioctl(mIavFd, IAV_IOC_GET_STATE, &state);
    AMLOG_INFO("DSP in state %d.\n", state);
    if (state != IAV_STATE_IDLE && state != IAV_STATE_DECODING) {
        AMLOG_INFO("DSP Not in IDLE & DECODING mode, enter IDLE mode first, state %d.\n", state);
        i = ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
        if (i != 0) {
            AMLOG_ERROR("UDEC enter IDLE mode fail, ret %d.\n", i);
            return ME_OS_ERROR;
        }
        AMLOG_INFO("enter IDLE mode done.\n");
    }

    mbUDECMode = 1;
    return ME_OK;
}

AM_ERR CAmbaDspA5s::EnterUDECMode()
{
    AUTO_LOCK(mpMutex);
    enterUdecMode();
    return ME_OK;
}

AM_ERR CAmbaDspA5s::ExitUDECMode()
{
    AUTO_LOCK(mpMutex);

    mbUDECMode = 0;
    return ME_OK;
}

AM_ERR CAmbaDspA5s::InitUDECInstance(AM_INT& wantUdecIndex, DSPConfig*pConfig, void* pHybirdAcc, AM_INT decFormat, AM_INT pic_width, AM_INT pic_height, AM_INT extEdge, bool autoAllocIndex)
{
    AUTO_LOCK(mpMutex);

    if (mbUDECMode == 0) {
        if (enterUdecMode() != ME_OK) {
            wantUdecIndex = eInvalidUdecIndex;
            return ME_OS_ERROR;
        }
    }

    if (!autoAllocIndex) {
        if (wantUdecIndex <0 || wantUdecIndex>=mTotalUDECNumber) {
            AMLOG_ERROR("Bad wantUdecIndex in CAmbaDspA5s::InitUDECInstance.\n");
            wantUdecIndex = eInvalidUdecIndex;
            return ME_BAD_PARAM;
        }
        if (mbInstanceUsed[wantUdecIndex]) {
            AMLOG_ERROR("Udec instance(%d) already in use. CAmbaDspA5s::InitUDECInstance.\n", wantUdecIndex);
            wantUdecIndex = eInvalidUdecIndex;
            return ME_BUSY;
        }
    }

    AM_ASSERT(mpSharedRes);
    //debug only assertion
    AM_ASSERT(pConfig == mpConfig);
    AM_ASSERT(pConfig == (&mpSharedRes->dspConfig));

    // find a avaiable udec instance
    if (mbInstanceUsed[wantUdecIndex]) {
        wantUdecIndex = getAvaiableUdecinstance();
        if (wantUdecIndex == eInvalidUdecIndex) {
            AMLOG_ERROR("No avaible Udec instance in CAmbaDspA5s::InitUDECInstance.\n");
            wantUdecIndex = eInvalidUdecIndex;
            return ME_BUSY;
        }
    }

    mbInstanceUsed[wantUdecIndex] = 1;

    if (ioctl(mIavFd, IAV_IOC_MAP_DECODE_BSB, &mMapInfo[wantUdecIndex]) < 0) {
        perror("IAV_IOC_MAP_DECODE_BSB");
        AMLOG_ERROR("Error in CAmbaDspA5s::IAV_IOC_MAP_DECODE_BSB.\n");
        wantUdecIndex = eInvalidUdecIndex;
        return ME_ERROR;
    }
    AMLOG_DEBUG("[DEBUG]: after IAV_IOC_MAP_DECODE_BSB addr %p, length %d.\n", mMapInfo[wantUdecIndex].addr, mMapInfo[wantUdecIndex].length);

    pConfig->udecInstanceConfig[wantUdecIndex].pbits_fifo_start = mMapInfo[wantUdecIndex].addr;
    pConfig->udecInstanceConfig[wantUdecIndex].bits_fifo_size = mMapInfo[wantUdecIndex].length;

    AMLOG_DEBUG("[DEBUG]: start IAV_IOC_MAP_DSP.\n");
    if (ioctl(mIavFd, IAV_IOC_MAP_DSP, &mMapInfo[wantUdecIndex]) < 0) {
        AM_PERROR("IAV_IOC_MAP_DSP");
        AMLOG_ERROR("Error in CAmbaDspA5s::IAV_IOC_MAP_DSP.\n");
        wantUdecIndex = eInvalidUdecIndex;
        return ME_ERROR;
    }

    AMLOG_DEBUG("[DEBUG]: start IAV_IOC_ENTER_IDLE.\n");
    if (ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0) < 0) {
        AM_PERROR("IAV_IOC_ENTER_IDLE");
        AMLOG_ERROR("Error in CAmbaDspA5s::IAV_IOC_ENTER_IDLE.\n");
        wantUdecIndex = eInvalidUdecIndex;
        return ME_ERROR;
    }

    //hard code here, config hdmi
    //configVout(eVoutHDMI, AMBA_VIDEO_MODE_D1_NTSC, AMBA_VOUT_SINK_TYPE_HDMI);

    AMLOG_DEBUG("[DEBUG]: start IAV_IOC_SELECT_CHANNEL.\n");
    if (ioctl(mIavFd, IAV_IOC_SELECT_CHANNEL, IAV_DEC_CHANNEL(0)) < 0) {
        AM_PERROR("IAV_IOC_START_DECODE");
        AMLOG_ERROR("Error in CAmbaDspA5s::IAV_IOC_SELECT_CHANNEL.\n");
        wantUdecIndex = eInvalidUdecIndex;
        return ME_ERROR;
    }

    AMLOG_DEBUG("[DEBUG]: start IAV_IOC_START_DECODE.\n");
    if (ioctl(mIavFd, IAV_IOC_START_DECODE, 0) < 0) {
        AM_PERROR("IAV_IOC_START_DECODE");
        AMLOG_ERROR("Error in CAmbaDspA5s::IAV_IOC_START_DECODE.\n");
        wantUdecIndex = eInvalidUdecIndex;
        return ME_ERROR;
    }

    if (TrickMode(wantUdecIndex, DEC_SPEED_1X, 0) != ME_OK) {
        wantUdecIndex = eInvalidUdecIndex;
        return ME_ERROR;
    }

    memset(&mUdecConfig[wantUdecIndex], 0, sizeof(mUdecConfig[wantUdecIndex]));
    mUdecConfig[wantUdecIndex].num_frame_buffer = 1;
    mUdecConfig[wantUdecIndex].flags = 0;
    mUdecConfig[wantUdecIndex].pic_width = pic_width;
    mUdecConfig[wantUdecIndex].pic_height = pic_height;
    mDecInfo[wantUdecIndex].pic_width = pic_width;
    mDecInfo[wantUdecIndex].pic_height = pic_height;

    AMLOG_DEBUG("[DEBUG], IAV_IOC_CONFIG_DECODER: chroma_format %d, decoder type %d.\n", mUdecConfig[wantUdecIndex].chroma_format, mUdecConfig[wantUdecIndex].decoder_type);
    AMLOG_DEBUG("               flags %d, fb w %d, h %d, pic w %d, h %d.\n", mUdecConfig[wantUdecIndex].flags, mUdecConfig[wantUdecIndex].fb_width, mUdecConfig[wantUdecIndex].fb_height, mUdecConfig[wantUdecIndex].pic_width, mUdecConfig[wantUdecIndex].pic_height);

    if (ioctl(mIavFd, IAV_IOC_CONFIG_DECODER, &mUdecConfig[wantUdecIndex]) < 0) {
        AM_PERROR("IAV_IOC_CONFIG_DECODER");
        AMLOG_ERROR("Error in CAmbaDspA5s::IAV_IOC_CONFIG_DECODER.\n");
        wantUdecIndex = eInvalidUdecIndex;
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CAmbaDspA5s::StartUdec(AM_INT udecIndex, AM_UINT format, void* pacc)
{
    //should do nothing
    return ME_OK;
}

AM_ERR CAmbaDspA5s::ReleaseUDECInstance(AM_INT udecIndex)
{
    AUTO_LOCK(mpMutex);

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AMLOG_ERROR("Bad wantUdecIndex in CAmbaDspA5s::InitUDECInstance, index %d, mTotalUDECNumber %d.\n", udecIndex, mTotalUDECNumber);
        return ME_BAD_PARAM;
    }

    if (mbInstanceUsed[udecIndex] == 0) {
        AMLOG_ERROR("Udec instance(%d) not in use. CAmbaDspA5s::ReleaseUDECInstance.\n", udecIndex);
        return ME_NOT_EXIST;
    }

    mbInstanceUsed[udecIndex] = 0;
    return ME_OK;
}

AM_ERR CAmbaDspA5s::RequestInputBuffer(AM_INT udecIndex, AM_UINT& size, AM_U8* pStart, AM_INT bufferCnt)
{
    iav_wait_decoder_t wait;
    AM_INT ret;
    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspA5s::RequestInputBuffer.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    // wait bsb, or decoded frame
    wait.flags = IAV_WAIT_BSB;
    wait.emptiness.room = size;
    wait.emptiness.start_addr = pStart;

    AMLOG_VERBOSE("[DEBUG]: IAV_IOC_WAIT_DECODER start, size %d, flags %d, number of frames %d.\n", wait.emptiness.room, wait.flags, wait.num_decoded_frames);
    if ((ret = ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait)) < 0) {
        if (errno != EAGAIN) {
            perror("IAV_IOC_WAIT_DECODER");
            AMLOG_PRINTF("!!!!!IAV_IOC_WAIT_DECODER error, ret %d.\n", ret);
            return ME_ERROR;
        }
        AMLOG_WARN("first wait will fail, related to a5s driver.\n");
    }
    AMLOG_VERBOSE("IAV_IOC_WAIT_DECODER done.\n");

    return ME_OK;
}

AM_ERR CAmbaDspA5s::DecodeBitStream(AM_INT udecIndex, AM_U8*pStart, AM_U8*pEnd)
{
    AM_INT ret;
    iav_h264_decode_t decode_info;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspA5s::DecodeBitStream.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    decode_info.start_addr = pStart;
    decode_info.end_addr = pEnd;
    decode_info.first_display_pts = 0;
    decode_info.num_pics = 1;
    decode_info.next_size = 0;
    decode_info.pic_width = 0;
    decode_info.pic_height = 0;

    AMLOG_VERBOSE("[DEBUG]: IAV_IOC_DECODE_H264 start, data[0] 0x%x, data[1] 0x%x.\n", *((AM_UINT*)decode_info.start_addr), *((AM_UINT*)decode_info.start_addr + 1));
    if ((ret = ioctl(mIavFd, IAV_IOC_DECODE_H264, &decode_info)) < 0) {
        perror("IAV_IOC_DECODE_H264");
        AMLOG_PRINTF("!!!!!IAV_IOC_DECODE_H264 error, ret %d.\n", ret);
        return ME_ERROR;
    }
    AMLOG_VERBOSE("IAV_IOC_DECODE_H264 done.\n");

    return ME_OK;
}

AM_ERR CAmbaDspA5s::AccelDecoding(AM_INT udecIndex, void* pParam, bool needSync)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::AccelDecoding no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

//stop, flush, clear, trick play
AM_ERR CAmbaDspA5s::StopUDEC(AM_INT udecIndex)
{
    AM_INT ret = 0;
    AM_UINT stop_code = udecIndex;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspA5s::StopUDEC.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    AM_PRINTF("IAV_IOC_STOP_DECODE start.\n");
    if (ioctl(mIavFd, IAV_IOC_STOP_DECODE, 1) < 0) {
        AM_PERROR("IAV_IOC_STOP_DECODE");
        AMLOG_PRINTF("!!!!!IAV_IOC_STOP_DECODE error, ret %d.\n", ret);
        return ME_ERROR;
    }
    AM_PRINTF("IAV_IOC_STOP_DECODE done.\n");

    return ME_OK;
}

AM_ERR CAmbaDspA5s::FlushUDEC(AM_INT udecIndex)
{
    AMLOG_DEBUG("[DEBUG]: start IAV_IOC_SELECT_CHANNEL.\n");
    if (ioctl(mIavFd, IAV_IOC_SELECT_CHANNEL, IAV_DEC_CHANNEL(0)) < 0) {
        AM_PERROR("IAV_IOC_START_DECODE");
        AMLOG_ERROR("Error in CAmbaDspA5s::IAV_IOC_SELECT_CHANNEL.\n");
        return ME_ERROR;
    }

    AMLOG_DEBUG("[DEBUG]: start IAV_IOC_START_DECODE.\n");
    if (ioctl(mIavFd, IAV_IOC_START_DECODE, 0) < 0) {
        AM_PERROR("IAV_IOC_START_DECODE");
        AMLOG_ERROR("Error in CAmbaDspA5s::IAV_IOC_START_DECODE.\n");
        return ME_ERROR;
    }

    if (TrickMode(udecIndex, DEC_SPEED_1X, 0) != ME_OK) {
        return ME_ERROR;
    }

    memset(&mUdecConfig[udecIndex], 0, sizeof(mUdecConfig[udecIndex]));
    mUdecConfig[udecIndex].num_frame_buffer = 1;
    mUdecConfig[udecIndex].flags = 0;
    mUdecConfig[udecIndex].pic_width = mDecInfo[udecIndex].pic_width;
    mUdecConfig[udecIndex].pic_height = mDecInfo[udecIndex].pic_height;

    AMLOG_DEBUG("[DEBUG], IAV_IOC_CONFIG_DECODER: chroma_format %d, decoder type %d.\n", mUdecConfig[udecIndex].chroma_format, mUdecConfig[udecIndex].decoder_type);
    AMLOG_DEBUG("               flags %d, fb w %d, h %d, pic w %d, h %d.\n", mUdecConfig[udecIndex].flags, mUdecConfig[udecIndex].fb_width, mUdecConfig[udecIndex].fb_height, mUdecConfig[udecIndex].pic_width, mUdecConfig[udecIndex].pic_height);

    if (ioctl(mIavFd, IAV_IOC_CONFIG_DECODER, &mUdecConfig[udecIndex]) < 0) {
        AM_PERROR("IAV_IOC_CONFIG_DECODER");
        AMLOG_ERROR("Error in CAmbaDspA5s::IAV_IOC_CONFIG_DECODER.\n");
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CAmbaDspA5s::ClearUDEC(AM_INT udecIndex)
{
    //need do nothing
    return ME_OK;
}

AM_ERR CAmbaDspA5s::PauseUDEC(AM_INT udecIndex)
{
    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspA5s::PauseUDEC.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    AMLOG_PRINTF("IAV_IOC_DECODE_PAUSE.\n");
    if (ioctl(mIavFd, IAV_IOC_DECODE_PAUSE, 0) < 0) {
        AM_PERROR("IAV_IOC_DECODE_PAUSE");
        AMLOG_PRINTF("!!!!!IAV_IOC_DECODE_PAUSE error.\n");
        return ME_ERROR;
    }
    return ME_OK;
}

AM_ERR CAmbaDspA5s::ResumeUDEC(AM_INT udecIndex)
{
    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad wantUdecIndex(%d) in CAmbaDspA5s::ResumeUDEC.\n", udecIndex);
        return ME_BAD_PARAM;
    }

    AMLOG_PRINTF("IAV_IOC_DECODE_RESUME.\n");
    if (ioctl(mIavFd, IAV_IOC_DECODE_RESUME, 0) < 0) {
        AM_PERROR("IAV_IOC_DECODE_RESUME");
        AMLOG_PRINTF("!!!!!IAV_IOC_DECODE_RESUME error.\n");
        return ME_ERROR;
    }
    return ME_OK;
}

AM_ERR CAmbaDspA5s::StepPlay(AM_INT udecIndex)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::StepPlay no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

AM_ERR CAmbaDspA5s::TrickMode(AM_INT udecIndex, AM_UINT speed, AM_UINT backward)
{
    AM_UINT play_speed = 0x100;
    iav_trick_play_t trick_play;

    if (udecIndex <0 || udecIndex>=mTotalUDECNumber) {
        AMLOG_ERROR("Bad wantUdecIndex in CAmbaDspA5s::TrickMode, index %d, mTotalUDECNumber %d.\n", udecIndex, mTotalUDECNumber);
        return ME_BAD_PARAM;
    }

    switch (speed) {
        case DEC_SPEED_1X:
            break;
        case DEC_SPEED_2X:
            play_speed <<= 1;
            break;
        case DEC_SPEED_4X:
            play_speed <<= 2;
            break;
        case DEC_SPEED_1BY2:
            play_speed >>= 1;
            break;
        case DEC_SPEED_1BY4:
            play_speed >>= 2;
            break;
        default:
            AMLOG_ERROR(" Invalid speed option : %d.\n", speed);
            return ME_NOT_SUPPORTED;
    }

    //support forward only
    AM_ASSERT(backward == 0);
    trick_play.speed = play_speed;
    trick_play.scan_mode = 1;
    trick_play.direction = backward;
    AMLOG_DEBUG("[DEBUG], IAV_IOC_TRICK_PLAY, speed %d, scan_mode %d, direction %d.\n", trick_play.speed, trick_play.scan_mode, trick_play.direction);
    if (ioctl(mIavFd, IAV_IOC_TRICK_PLAY, &trick_play) < 0) {
        AM_PERROR("IAV_IOC_TRICK_PLAY");
        AM_ASSERT(0);
        AMLOG_ERROR("IAV_IOC_TRICK_PLAY fail.\n");
        return ME_ERROR;
    }
    return ME_OK;
}

//frame buffer pool
AM_UINT CAmbaDspA5s::InitFrameBufferPool(AM_INT udecIndex, AM_UINT picWidth, AM_UINT picHeight, AM_UINT chromaFormat, AM_UINT tileFormat, AM_UINT hasEdge)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::InitFrameBufferPool no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

AM_ERR CAmbaDspA5s::ResetFrameBufferPool(AM_INT udecIndex)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::ResetFrameBufferPool no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

//buffer id/data pointer in CVideoBuffer
AM_ERR CAmbaDspA5s::RequestFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::RequestFrameBuffer no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

AM_ERR CAmbaDspA5s::RenderFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer, SRect* srcRect)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::RenderFrameBuffer no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

AM_ERR CAmbaDspA5s::ReleaseFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::ReleaseFrameBuffer no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

//avsync
AM_ERR CAmbaDspA5s::SetClockOffset(AM_INT udecIndex, AM_S32 diff)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::SetClockOffset no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

AM_ERR CAmbaDspA5s::WakeVout(AM_INT udecIndex, AM_UINT voutMask)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::WakeVout no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

AM_ERR CAmbaDspA5s::GetUdecTimeEos(AM_INT udecIndex, AM_UINT& eos, AM_U64& udecTime, AM_INT nowait)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::GetUdecTimeEos no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

//query UDEC state
AM_ERR CAmbaDspA5s::GetUDECState(AM_INT udecIndex, AM_UINT& udecState, AM_UINT& voutState, AM_UINT& errorCode)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::GetUDECState no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

AM_ERR CAmbaDspA5s::GetDecodedFrame(AM_INT udecIndex, AM_INT& buffer_id, AM_INT& eos_invalid)
{
    AM_ASSERT(0);
    AMLOG_ERROR("Error: CAmbaDspA5s::GetDecodedFrame no implement, must not comes here.\n");
    return ME_NO_IMPL;
}

//--------------------------------------------------
//
//from IVoutHandler
//
//--------------------------------------------------
AM_ERR CAmbaDspA5s::SetupVOUT(AM_INT sink_type, AM_INT mode)
{
    //For sink config
    AM_INT sink_id = -1;
    AM_UINT width, height;
    //Specify external device type for vout
    AM_ERR ret = ME_OK;
    ret = GetSinkId(mIavFd, sink_type, &sink_id);
    AMLOG_PRINTF( "sind_id = %d\n",sink_id);
    if (ret){
        return ret;
    }

    AMLOG_PRINTF("[DEBUG], IAV_IOC_VOUT_SELECT_DEV, sink_id %d.\n", sink_id);
    if (ioctl(mIavFd, IAV_IOC_VOUT_SELECT_DEV, sink_id) < 0) {
        AM_PERROR("IAV_IOC_VOUT_SELECT_DEV");
        switch (sink_type) {
            case AMBA_VOUT_SINK_TYPE_DIGITAL:
                AMLOG_PRINTF("No LCD: ");
                AMLOG_PRINTF("Driver not loaded!\n");
                break;

            case AMBA_VOUT_SINK_TYPE_CVBS:
                AMLOG_PRINTF("No CVBS sink: ");
                AMLOG_PRINTF("Driver not loaded!\n");
                break;

            case AMBA_VOUT_SINK_TYPE_HDMI:
                AMLOG_PRINTF("No HDMI sink: ");
                AMLOG_PRINTF("Hdmi cable not plugged, ");
                AMLOG_PRINTF("driver not loaded, ");
                AMLOG_PRINTF("or hdmi output not supported!\n");
                break;

            default:
                break;
        }
        return ME_ERROR;
    }

    //Configure vout
    struct amba_video_sink_mode sink_cfg;
    memset(&sink_cfg, 0, sizeof(sink_cfg));
    sink_cfg.mode = mode;
    sink_cfg.ratio = AMBA_VIDEO_RATIO_AUTO;
    sink_cfg.bits = AMBA_VIDEO_BITS_AUTO;
    sink_cfg.type = sink_type;
    if (mode == AMBA_VIDEO_MODE_480I || mode == AMBA_VIDEO_MODE_576I
        || mode == AMBA_VIDEO_MODE_1080I
        || mode == AMBA_VIDEO_MODE_1080I_PAL)
        sink_cfg.format = AMBA_VIDEO_FORMAT_INTERLACE;
    else
        sink_cfg.format = AMBA_VIDEO_FORMAT_PROGRESSIVE;

    sink_cfg.sink_type = AMBA_VOUT_SINK_TYPE_AUTO;
    sink_cfg.bg_color.y = 0x10;
    sink_cfg.bg_color.cb = 0x80;
    sink_cfg.bg_color.cr = 0x80;
    sink_cfg.lcd_cfg.mode = AMBA_VOUT_LCD_MODE_RGB565;
    //sink_cfg.lcd_cfg.mode = AMBA_VOUT_LCD_MODE_DISABLE;

    sink_cfg.id = sink_id;
    sink_cfg.frame_rate = AMBA_VIDEO_FPS_AUTO;
    sink_cfg.csc_en = 1;
    sink_cfg.hdmi_color_space = AMBA_VOUT_HDMI_CS_AUTO;
    sink_cfg.hdmi_3d_structure = DDD_RESERVED;
    sink_cfg.hdmi_overscan = AMBA_VOUT_HDMI_OVERSCAN_AUTO;
    sink_cfg.video_en = 1;

    //Options related to Video only
    struct amba_vout_video_offset video_offset = {0};

    ret = GetVoutSize(mode, &width, &height);
    if (ret){
        return ret;
    }
    sink_cfg.video_size.specified = 1;
    sink_cfg.video_size.vout_width = sink_cfg.video_size.video_width = width;
    sink_cfg.video_size.vout_height = sink_cfg.video_size.video_height = height;

    sink_cfg.video_offset = video_offset;
    sink_cfg.fb_id = -1;
    sink_cfg.video_rotate = AMBA_VOUT_ROTATE_NORMAL;
    //Options related to OSD only
    struct amba_vout_osd_rescale osd_rescale = {0};
    struct amba_vout_osd_offset osd_offset = {0};
    sink_cfg.osd_rotate = AMBA_VOUT_ROTATE_NORMAL;
    sink_cfg.osd_flip = AMBA_VOUT_FLIP_NORMAL;
    sink_cfg.osd_rescale = osd_rescale;
    sink_cfg.osd_offset = osd_offset;
    sink_cfg.display_input = AMBA_VOUT_INPUT_FROM_MIXER;

    AM_PRINTF("[DEBUG], IAV_IOC_VOUT_CONFIGURE_SINK: sink_cfg: id %d.\n", sink_cfg.id);
    AM_PRINTF("               mode %d, ratio %d, bits %d, type %d, format %d.\n", sink_cfg.mode, sink_cfg.ratio, sink_cfg.bits, sink_cfg.type, sink_cfg.format);
    AM_PRINTF("               frame rate %d, csc en %d, bg_color y %d, cb %d, cr %d.\n", sink_cfg.frame_rate, sink_cfg.csc_en, sink_cfg.bg_color.y, sink_cfg.bg_color.cb, sink_cfg.bg_color.cr);
    AM_PRINTF("               display_input %d, sink_type %d, video en y %d, flip %d, rotate %d.\n", sink_cfg.display_input, sink_cfg.sink_type, sink_cfg.video_en, sink_cfg.video_flip, sink_cfg.video_rotate);
    AM_PRINTF("               video_width %d, video_height %d, vout_width %d, vout_height %d, spe %d.\n", sink_cfg.video_size.video_width, sink_cfg.video_size.video_height, sink_cfg.video_size.vout_width, sink_cfg.video_size.vout_height, sink_cfg.video_size.specified);
    AM_PRINTF("               video_offset_x %d, video_offset_y %d, spe %d.\n", sink_cfg.video_offset.offset_x, sink_cfg.video_offset.offset_y, sink_cfg.video_offset.specified);
    AM_PRINTF("               fb_id %d, osd_flip %d, osd_rotate %d, direct_to_dsp.\n", sink_cfg.fb_id, sink_cfg.osd_flip, sink_cfg.osd_rotate, sink_cfg.direct_to_dsp);
    AM_PRINTF("               osd_w %d, osd_h %d, rescale_en %d, w %d, h %d.\n", sink_cfg.osd_size.width, sink_cfg.osd_size.height, sink_cfg.osd_rescale.enable, sink_cfg.osd_rescale.width, sink_cfg.osd_rescale.height);
    AM_PRINTF("               lcd dclk_freq_hz %d, mode %d, model %d, seqb %d, seqt %d.\n", sink_cfg.lcd_cfg.dclk_freq_hz, sink_cfg.lcd_cfg.mode, sink_cfg.lcd_cfg.model, sink_cfg.lcd_cfg.seqb, sink_cfg.lcd_cfg.seqt);
    AM_PRINTF("               hdmi_color_space %d, hdmi_3d_structure %d, hdmi_overscan %d.\n", sink_cfg.hdmi_color_space, sink_cfg.hdmi_3d_structure, sink_cfg.hdmi_overscan);

    if (ioctl(mIavFd, IAV_IOC_VOUT_CONFIGURE_SINK, &sink_cfg) < 0) {
        perror("IAV_IOC_VOUT_CONFIGURE_SINK");
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CAmbaDspA5s::Enable(AM_UINT voutID, AM_INT enable)
{
    AM_INT ret;
    iav_vout_enable_video_t     iav_enable_video;
    iav_enable_video.vout_id    =   voutID;
    iav_enable_video.video_en   =   enable ? 1 : 0;

    ret = ioctl(mIavFd, IAV_IOC_VOUT_ENABLE_VIDEO, &iav_enable_video);
    if (!ret) {
        mpConfig->voutConfigs.voutConfig[voutID].enable = enable;
        return ME_OK;
    }

    AM_ASSERT(0);
    AMLOG_ERROR("CAmbaDspA5s::Enable fail, vout_id %d, enable %d.\n", voutID, enable);
    return ME_ERROR;
}

AM_ERR CAmbaDspA5s::EnableOSD(AM_UINT vout, AM_INT enable)
{
    AM_ERROR("CAmbaDspA5s::EnableOSD not implement.\n");
    return ME_OK;
}

AM_ERR CAmbaDspA5s::GetSizePosition(AM_UINT voutID, AM_INT* pos_x, AM_INT* pos_y, AM_INT* size_x, AM_INT* size_y)
{
    AM_ASSERT(mpConfig);
    AM_ASSERT(voutID >= 0 && voutID < eVoutCnt);

    if (!mpConfig || voutID < 0 || voutID >= eVoutCnt) {
        AM_ASSERT(0);
        return ME_BAD_PARAM;
    }

    *pos_x = mpConfig->voutConfigs.voutConfig[voutID].pos_x;
    *pos_y = mpConfig->voutConfigs.voutConfig[voutID].pos_y;
    *size_x = mpConfig->voutConfigs.voutConfig[voutID].size_x;
    *size_y = mpConfig->voutConfigs.voutConfig[voutID].size_y;
    return ME_OK;
}

AM_ERR CAmbaDspA5s::GetDimension(AM_UINT voutID, AM_INT* size_x, AM_INT* size_y)
{
    AM_ASSERT(mpConfig);
    AM_ASSERT(voutID >= 0 && voutID < eVoutCnt);

    if (!mpConfig || voutID < 0 || voutID >= eVoutCnt) {
        AM_ASSERT(0);
        return ME_BAD_PARAM;
    }

    *size_x = mpConfig->voutConfigs.voutConfig[voutID].width;
    *size_y = mpConfig->voutConfigs.voutConfig[voutID].height;
    return ME_OK;
}

AM_ERR CAmbaDspA5s::GetPirtureSizeOffset(AM_INT* pic_width, AM_INT* pic_height, AM_INT* offset_x, AM_INT* offset_y)
{
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

AM_ERR CAmbaDspA5s::GetSoureRect(AM_INT* pos_x, AM_INT* pos_y, AM_INT* size_x, AM_INT* size_y)
{
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

AM_ERR CAmbaDspA5s::ChangeInputCenter(AM_INT pos_x, AM_INT pos_y)
{
    AUTO_LOCK(mpMutex);
    AM_ASSERT(mpConfig);
    if (!mpConfig) {
        return ME_BAD_PARAM;
    }

    //a5s no implement this yet
    AMLOG_ERROR("CAmbaDspA5s::ChangeInputCenter not supported.\n");
    return ME_NO_IMPL;
}

AM_ERR CAmbaDspA5s::ChangeRoomFactor(AM_UINT voutID, float factor_x, float factor_y)
{
    AUTO_LOCK(mpMutex);
    AM_ASSERT(mpConfig);
    if (!mpConfig) {
        return ME_BAD_PARAM;
    }

    //a5s no implement this yet
    AMLOG_ERROR("CAmbaDspA5s::ChangeRoomFactor not supported.\n");
    return ME_NO_IMPL;
}

AM_ERR CAmbaDspA5s::UpdatePirtureSizeOffset(AM_INT pic_width, AM_INT pic_height, AM_INT offset_x, AM_INT offset_y)
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
    return ME_NO_IMPL;
}

//todo: mass apis about change size/position/flip/rotate/mirror/source rect
AM_ERR CAmbaDspA5s::ChangeSizePosition(AM_UINT voutID, AM_INT pos_x, AM_INT pos_y, AM_INT size_x, AM_INT size_y)
{
    AUTO_LOCK(mpMutex);

    if (mpConfig->voutConfigs.voutConfig[voutID].size_x == size_x && mpConfig->voutConfigs.voutConfig[voutID].size_y == size_y && mpConfig->voutConfigs.voutConfig[voutID].pos_x == pos_x && mpConfig->voutConfigs.voutConfig[voutID].pos_y == pos_y) {
        AMLOG_PRINTF("size position not changed, no need to call this fuction.\n");
        return ME_OK;
    }

    if (RectOutofRange(size_x, size_y, pos_x, pos_y, mpConfig->voutConfigs.voutConfig[voutID].width, mpConfig->voutConfigs.voutConfig[voutID].height, 0, 0)) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad parameters in CAmbaDspA5s::ChangeSizePosition, vout id %d, pos_x %d, pos_y %d, size_x %d, size_y %d.\n", voutID, pos_x, pos_y, size_x, size_y);
        return ME_BAD_PARAM;
    }

    //check size need change?
    if (mpConfig->voutConfigs.voutConfig[voutID].size_x != size_x || mpConfig->voutConfigs.voutConfig[voutID].size_y != size_y) {
        if (changeSize(voutID, size_x, size_y) != ME_OK) {
            //change position first
            changePosition(voutID, pos_x, pos_y);
            return changeSize(voutID, size_x, size_y);
        } else {
            return changePosition(voutID, pos_x, pos_y);
        }
    }

    return changePosition(voutID, pos_x, pos_y);
}

AM_ERR CAmbaDspA5s::changeSize(AM_UINT voutID, AM_INT size_x, AM_INT size_y)
{
    AM_INT ret;
    iav_vout_change_video_size_t    iav_cvs;

    if (RectOutofRange(size_x, size_y, mpConfig->voutConfigs.voutConfig[voutID].pos_x, mpConfig->voutConfigs.voutConfig[voutID].pos_y, mpConfig->voutConfigs.voutConfig[voutID].width, mpConfig->voutConfigs.voutConfig[voutID].height, 0, 0)) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad parameters in CAmbaDspA5s::ChangeSize, vout id %d, pos_x %d, pos_y %d, size_x %d, size_y %d.\n", voutID, mpConfig->voutConfigs.voutConfig[voutID].pos_x, mpConfig->voutConfigs.voutConfig[voutID].pos_y, size_x, size_y);
        return ME_BAD_PARAM;
    }

    AMLOG_DEBUG("change size in CAmbaDspA5s::ChangeSizePosition, vout id %d, pos_x %d, pos_y %d, size_x %d, size_y %d.\n", voutID, mpConfig->voutConfigs.voutConfig[voutID].pos_x, mpConfig->voutConfigs.voutConfig[voutID].pos_y, size_x, size_y);
    iav_cvs.vout_id =   voutID;
    iav_cvs.width   =   size_x;
    iav_cvs.height  =   size_y;
    ret = ioctl(mIavFd, IAV_IOC_VOUT_CHANGE_VIDEO_SIZE, &iav_cvs);
    if (ret) {
        AM_PERROR("IAV_IOC_VOUT_CHANGE_VIDEO_SIZE");
        return ME_OS_ERROR;
    }
    AMLOG_DEBUG("IAV_IOC_VOUT_CHANGE_VIDEO_SIZE done.\n");
    mpConfig->voutConfigs.voutConfig[voutID].size_x = size_x;
    mpConfig->voutConfigs.voutConfig[voutID].size_y = size_y;

    return ME_OK;
}

AM_ERR CAmbaDspA5s::changePosition(AM_UINT voutID, AM_INT pos_x, AM_INT pos_y)
{
    AM_INT ret;
    iav_vout_change_video_offset_t  iav_cvo;

    if (RectOutofRange(mpConfig->voutConfigs.voutConfig[voutID].size_x, mpConfig->voutConfigs.voutConfig[voutID].size_y, pos_x, pos_y, mpConfig->voutConfigs.voutConfig[voutID].width, mpConfig->voutConfigs.voutConfig[voutID].height, 0, 0)) {
        AM_ASSERT(0);
        AMLOG_ERROR("Bad parameters in CAmbaDspA5s::ChangePosition, vout id %d, pos_x %d, pos_y %d, size_x %d, size_y %d.\n", voutID, pos_x, pos_y, mpConfig->voutConfigs.voutConfig[voutID].size_x, mpConfig->voutConfigs.voutConfig[voutID].size_y);
        return ME_BAD_PARAM;
    }

    AMLOG_DEBUG("change position in CAmbaDspA5s::ChangeSizePosition, vout id %d, pos_x %d, pos_y %d, size_x %d, size_y %d.\n", voutID, pos_x, pos_y, mpConfig->voutConfigs.voutConfig[voutID].size_x, mpConfig->voutConfigs.voutConfig[voutID].size_y);
    iav_cvo.vout_id =   voutID;
    iav_cvo.offset_x    =   pos_x;
    iav_cvo.offset_y    =   pos_y;
    ret = ioctl(mIavFd, IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET, &iav_cvo);
    if (ret) {
        AM_PERROR("IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET");
        return ME_OS_ERROR;
    }
    AMLOG_DEBUG("IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET done.\n");
    mpConfig->voutConfigs.voutConfig[voutID].pos_x = pos_x;
    mpConfig->voutConfigs.voutConfig[voutID].pos_y = pos_y;

    return ME_OK;
}

AM_ERR CAmbaDspA5s::ChangeSize(AM_UINT voutID, AM_INT size_x, AM_INT size_y)
{
    AUTO_LOCK(mpMutex);

    if (mpConfig->voutConfigs.voutConfig[voutID].size_x == size_x || mpConfig->voutConfigs.voutConfig[voutID].size_y == size_y) {
        return ME_OK;
    }

    return changeSize(voutID, size_x, size_y);
}

AM_ERR CAmbaDspA5s::ChangePosition(AM_UINT voutID, AM_INT pos_x, AM_INT pos_y)
{
    AUTO_LOCK(mpMutex);

    if (mpConfig->voutConfigs.voutConfig[voutID].pos_x == pos_x && mpConfig->voutConfigs.voutConfig[voutID].pos_y == pos_y) {
        return ME_OK;
    }

    return changePosition(voutID, pos_x, pos_y);
}

AM_ERR CAmbaDspA5s::ChangeSourceRect(AM_INT pos_x, AM_INT pos_y, AM_INT size_x, AM_INT size_y)
{
    AUTO_LOCK(mpMutex);
    AM_ASSERT(mpConfig);
    if (!mpConfig) {
        return ME_BAD_PARAM;
    }

    //a5s no implement this yet
    AMLOG_ERROR("CAmbaDspA5s::ChangeSourceRect not supported.\n");
    return ME_NO_IMPL;
}

AM_ERR CAmbaDspA5s::Flip(AM_UINT voutID, AM_INT param)
{
    AM_INT ret;
    iav_vout_flip_video_t   iav_flip_video;

    iav_flip_video.vout_id  =   voutID;
    switch (param) {
        case AMBA_VOUT_FLIP_HV:
            iav_flip_video.flip	= 1;
            break;

        case AMBA_VOUT_FLIP_HORIZONTAL:
            iav_flip_video.flip	= 2;
            break;

        case AMBA_VOUT_FLIP_VERTICAL:
            iav_flip_video.flip	= 3;
            break;

        default:
            iav_flip_video.flip	= 0;
            break;
    }

    ret = ioctl(mIavFd, IAV_IOC_VOUT_FLIP_VIDEO, &iav_flip_video);

    if (!ret) {
        mpConfig->voutConfigs.voutConfig[voutID].flip = param;
        return ME_OK;
    }

    AM_ASSERT(0);
    AMLOG_ERROR("CAmbaDspA5s::Flip fail, vout_id %d, param %d.\n", voutID, param);
    return ME_ERROR;
}

AM_ERR CAmbaDspA5s::Rotate(AM_UINT voutID, AM_INT param)
{
    AM_INT ret;
    iav_vout_rotate_video_t     iav_rotate_video;

    iav_rotate_video.vout_id    =   voutID;
    iav_rotate_video.rotate     =   param ? 1 : 0;

    ret = ioctl(mIavFd, IAV_IOC_VOUT_ROTATE_VIDEO, &iav_rotate_video);

    if (!ret) {
        mpConfig->voutConfigs.voutConfig[voutID].rotate= param;
        return ME_OK;
    }

    AM_ASSERT(0);
    AMLOG_ERROR("CAmbaDspA5s::Rotate fail, vout_id %d, param %d.\n", voutID, param);
    return ME_ERROR;
}

AM_ERR CAmbaDspA5s::Mirror(AM_UINT voutID, AM_INT param)
{
    return ME_NO_IMPL;
}

