
/*
 * amba_video_sink.cpp
 *
 * History:
 *    2012/02/23 - [Zhi He] create file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "amba_video_sink"
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


#include "filter_list.h"

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "amdsp_common.h"
#include "am_util.h"
#include "amba_video_sink.h"

filter_entry g_amba_video_sink = {
	"AmbaVideoSink",
	CAmbaVideoSink::Create,
	NULL,
	CAmbaVideoSink::AcceptMedia,
};

#define MB  (1024 * 1024)

#define BE_16(x) (((unsigned char *)(x))[0] <<  8 | \
                    ((unsigned char *)(x))[1])

#define BE_32(x) ((((unsigned char *)(x))[0] << 24) | \
                  (((unsigned char *)(x))[1] << 16) | \
                  (((unsigned char *)(x))[2] << 8)  | \
                   ((unsigned char *)(x))[3])

IFilter* CreateAmbaVideoSink(IEngine *pEngine)
{
    return CAmbaVideoSink::Create(pEngine);
}

//-----------------------------------------------------------------------
//
// CAmbaVideoSink
//
//-----------------------------------------------------------------------
void CAmbaVideoSink::PrintBitstremBuffer(AM_U8* p, AM_UINT size)
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

void CAmbaVideoSink::dumpEsData(AM_U8* pStart, AM_U8* pEnd)
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

AM_ERR CAmbaVideoSink::ConfigDecoder()
{
    AM_INT ret = 0, i = 0;
    AM_ERR err;

    mMaxVoutWidth = 0;
    mMaxVoutHeight = 0;
    //parse voutconfig
    mVoutNumber = 0;
    mVoutStartIndex = 0;

    //get vout parameters
    for (i = 0; i < eVoutCnt; i++) {
        ret = getVoutPrams(mIavFd, i, &mVoutConfig[i]);
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
    }

    if (mVoutConfigMask & (1<<eVoutLCD)) {
        mVoutStartIndex = eVoutLCD;
    } else if (mVoutConfigMask & (1<<eVoutHDMI)) {
        mVoutStartIndex = eVoutHDMI;
    }
    AMLOG_INFO("mVoutStartIndex %d, mVoutNumber %d, mVoutConfigMask 0x%x.\n", mVoutStartIndex, mVoutNumber, mVoutConfigMask);

    AM_UINT vFormat = 0;

    AMLOG_INFO("stream frame rate num %d, den %d, %f.\n", mpStream->r_frame_rate.num, mpStream->r_frame_rate.den, (float)mpStream->r_frame_rate.num/(float)mpStream->r_frame_rate.num);

    mbAddUdecWarpper = (mpSharedRes->dspConfig.addVideoDataType == eAddVideoDataType_iOneUDEC);

    switch (mpCodec->codec_id) {
        case CODEC_ID_H264:
            AMLOG_INFO("CODEC_ID_H264\n");
            muDecType = UDEC_H264;
            vFormat = UDEC_VFormat_H264;
            break;
        default:
            AM_ERROR("not support format %d\n", mpCodec->codec_id);
            mbAddUdecWarpper = false;
            break;
    }
    AMLOG_INFO("try create udecoder %d\n", muDecType);

    if (mbAddUdecWarpper) {
        mpStream->r_frame_rate.num = 90000;
        mpStream->r_frame_rate.den = 3003;
        AMLOG_INFO("num = %d, den = %d.\n", mpStream->r_frame_rate.num, mpStream->r_frame_rate.den);
        FillUSEQHeader(mUSEQHeader, vFormat, mpStream->r_frame_rate.num, mpStream->r_frame_rate.den, mpSharedRes->is_mp4s_flag, mpSharedRes->vid_container_width, mpSharedRes->vid_container_height);
        InitUPESHeader(mUPESHeader, vFormat);
        GenerateConfigData();
    }

    AM_ASSERT(!mpSimpleDecAPI);
    mpSimpleDecAPI = CreateSimpleDecAPI(mIavFd, DSPMode_DuplexLowdelay, UDEC_H264, (void*)mpSharedRes);
    AM_ASSERT(mpSimpleDecAPI);
    AM_ASSERT(mpCodec);

    mDecParam.codec_type = UDEC_H264;
    mDecParam.dsp_mode = DSPMode_DuplexLowdelay;
    mDecParam.pic_width = mpCodec->width;
    mDecParam.pic_height = mpCodec->height;
    mDecParam.pic_width_aligned = (mDecParam.pic_width + 15)&(~15);
    mDecParam.pic_height_aligned = (mDecParam.pic_height + 15)&(~15);
    AMLOG_INFO("before mpSimpleDecAPI->InitDecoder, width %d, height %d.\n", mDecParam.pic_width, mDecParam.pic_height);

    mDisplayRectMap[0].rect[0].w = mDecParam.pic_width;
    mDisplayRectMap[0].rect[0].h = mDecParam.pic_height;

    //hardware limitation code
    if (1) {
        AMLOG_INFO("hardware limitaion for duplex mode: can only specify one vout for playback.\n");
        mVoutNumber = 1;
        if (eVoutHDMI == mpSharedRes->encoding_mode_config.playback_vout_index) {
            AMLOG_INFO("use HDMI for playback.\n");
            mVoutStartIndex = eVoutHDMI;
            mbVoutForPB[eVoutHDMI] = 1;
            mbVoutForPB[eVoutLCD] = 0;
        } else if (eVoutLCD == mpSharedRes->encoding_mode_config.playback_vout_index) {
            AMLOG_INFO("use LCD for playback.\n");
            mVoutStartIndex = eVoutLCD;
            mbVoutForPB[eVoutHDMI] = 0;
            mbVoutForPB[eVoutLCD] = 1;
        } else {
            AM_ERROR("BAD vout index %d, use HDMI as default.\n", mpSharedRes->encoding_mode_config.playback_vout_index);
            mVoutStartIndex = eVoutHDMI;
            mbVoutForPB[eVoutHDMI] = 1;
            mbVoutForPB[eVoutLCD] = 0;
        }
    }

    err = mpSimpleDecAPI->InitDecoder(mDspIndex, &mDecParam, mVoutNumber, &mVoutConfig[mVoutStartIndex]);
    AM_ASSERT(ME_OK == err);
    if (ME_OK != err) {
        AM_ERROR("mpSimpleDecAPI->InitDecoder fail, ret %d.\n", err);
        return ME_ERROR;
    }
    mbDecAPIStopped = false;

    AMLOG_INFO("after mpSimpleDecAPI->InitDecoder, param.bits_fifo_start %p, size %d.\n", mDecParam.bits_fifo_start, mDecParam.bits_fifo_size);
    mpStartAddr = mDecParam.bits_fifo_start;
    mpEndAddr = mDecParam.bits_fifo_start + mDecParam.bits_fifo_size;
    mSpace = mDecParam.bits_fifo_size;
    mpCurrAddr = mpStartAddr;

    AM_MSG msg;
    msg.code = IEngine::MSG_VIDEO_DECODING_START;
    msg.p1 = (AM_INTPTR)static_cast<IFilter*>(this);
    msg.p2 = mpCodec->width;
    msg.p3 = mpCodec->height;

    AMLOG_INFO("before post MSG_VIDEO_DECODING_START, width height %dx%d.\n", mpCodec->width, mpCodec->height);
    PostEngineMsg(msg);

    return ME_OK;
}

AM_U8 *CAmbaVideoSink::CopyToBSB(AM_U8 *ptr, AM_U8 *buffer, AM_UINT size)
{
    AMLOG_BINARY("copy to bsb %d, %x.\n", size, *buffer);
    if (ptr + size <= mpEndAddr) {
        memcpy(ptr, buffer, size);
        return ptr + size;
    } else {
        //AM_INFO("-------wrap happened--------\n");
        AM_INT room = mpEndAddr - ptr;
        AM_U8 *ptr2;
        memcpy(ptr, buffer, room);
        ptr2 = buffer + room;
        size -= room;
        memcpy(mpStartAddr, ptr2, size);
        return mpStartAddr + size;
    }
}

AM_U8 *CAmbaVideoSink::FillEOS(AM_U8 *ptr)
{
    AMLOG_INFO("CAmbaVideoSink fill eos.\n");
    switch (muDecType) {
        case UDEC_H264: {
            AM_U8 eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};
            mbFillEOS= true;
            return CopyToBSB(ptr, eos, sizeof(eos));
        }
        break;

    default:
        AM_ERROR("not implemented!\n");
        return 0;
    }
}

void CAmbaVideoSink::updateStatus()
{
    iav_duplex_get_decoder_status_t status;
    status.dec_id = mDspIndex;
    if (mIavFd >= 0) {
        ioctl(mIavFd, IAV_IOC_DUPLEX_GET_DECODER_STATUS, &status);
        mDSPErrorLevel = status.decoder_error_level;
        mDSPErrorType = status.decoder_error_type;
        mCurrentPbPTS = ((am_pts_t)status.last_pts_low) | ((am_pts_t)status.last_pts_high <<32);
        mbCurrentPbPTSValid = status.last_pts_valid;
        return;
    } else {
        AM_ERROR("BAD iav fd value %d.\n", mIavFd);
    }
}

void CAmbaVideoSink::clearDecoder()
{
    AM_ERR err;

    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    if (mpSimpleDecAPI && !mbDecAPIStopped) {
        AMLOG_INFO("[flow cmd]: Call mpSimpleDecAPI->Stop\n");
        mpSimpleDecAPI->Stop(mDspIndex, 0);
        AMLOG_INFO("[flow cmd]: Call mpSimpleDecAPI->Stop done\n");
        mbDecAPIStopped = true;
        mpSimpleDecAPI->ReleaseDecoder(mDspIndex);
        AMLOG_INFO("[flow cmd]: Call mpSimpleDecAPI->ReleaseDecoder done\n");
    }

    mbStreamStart = false;
    mbConfigData = false;
    mbFillEOS = false;

    mCurrentPbPTS = 0;
    mbCurrentPbPTSValid = 0;
    mDSPErrorLevel = 0;
    mDSPErrorType = 0;

    mbWaitFirstValidPTS = 1;
    mbRecievedSyncCmd = 0;
    mbAlreadySendSyncMsg = 0;

    mpCurrAddr = mpStartAddr;

    mbWaitKeyFrame = 1;

    err = mpSimpleDecAPI->InitDecoder(mDspIndex, &mDecParam, mVoutNumber, &mVoutConfig[mVoutStartIndex]);
    AM_ASSERT(ME_OK == err);
    if (ME_OK != err) {
        AM_ERROR("mpSimpleDecAPI->InitDecoder fail, ret %d.\n", err);
        return;
    }
    mbDecAPIStopped = false;

    AMLOG_INFO("after mpSimpleDecAPI->InitDecoder, param.bits_fifo_start %p, size %d.\n", mDecParam.bits_fifo_start, mDecParam.bits_fifo_size);
    mpStartAddr = mDecParam.bits_fifo_start;
    mpEndAddr = mDecParam.bits_fifo_start + mDecParam.bits_fifo_size;
    mSpace = mDecParam.bits_fifo_size;
    mpCurrAddr = mpStartAddr;

}

IFilter* CAmbaVideoSink::Create(IEngine *pEngine)
{
    CAmbaVideoSink *result = new CAmbaVideoSink(pEngine);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_INT CAmbaVideoSink::AcceptMedia(CMediaFormat& format)
{
    if (*format.pFormatType != GUID_Format_FFMPEG_Stream)
        return 0;

    if (*format.pMediaType == GUID_Video && *format.pSubType == GUID_AmbaVideoDecoder) {
        if (PreferWorkMode_UDEC == (format.preferWorkMode)) {
            AM_ERROR("NOT support in UDEC mode.\n");
            return 0;
        }
        return 1;
    }

    return 0;
}

AM_ERR CAmbaVideoSink::enterDuplexMode(void)
{
    AM_INT state = 0, ret = 0;
    ret = ioctl(mIavFd, IAV_IOC_GET_STATE, &state);
    if (ret < 0) {
        perror("IAV_IOC_GET_STATE");
        AM_ERROR("IAV_IOC_GET_STATE return %d, state %d.\n", ret, state);
    }

    if (IAV_STATE_IDLE == state) {
        AMLOG_INFO("[dsp mode]: (video sink) try Duplex mode, from IDLE.\n");
    } else if (IAV_STATE_DUPLEX == state) {
        AMLOG_INFO("[dsp mode]: (video sink) already in Duplex mode, enter it, add reference count.\n");
    } else {
        AMLOG_ERROR("[dsp mode]: (video sink): (%d) not in Duplex mode or IDLE mode, deny request.\n", state);
        return ME_OS_ERROR;
    }

    iav_duplex_mode_t mode;

    memset(&mode, 0, sizeof(mode));

    mode.num_of_enc_chans = mpSharedRes->encoding_mode_config.num_of_enc_chans;
    AM_ASSERT(1 == mode.num_of_enc_chans);
    mode.num_of_dec_chans = mpSharedRes->encoding_mode_config.num_of_dec_chans;
    AM_ASSERT(1 == mode.num_of_dec_chans);
    //hard code here, to do
    mode.num_of_enc_chans = 1;
    mode.num_of_dec_chans = 1;

    AM_ASSERT(!mpSharedRes->encoding_mode_config.playback_in_pip);
    if (eVoutLCD == mpSharedRes->encoding_mode_config.playback_vout_index) {
        mode.vout_mask = (1 << eVoutLCD);
        AM_ASSERT(!mpSharedRes->encoding_mode_config.preview_enabled);
    } else if (eVoutHDMI == mpSharedRes->encoding_mode_config.playback_vout_index) {
        mode.vout_mask = (1 << eVoutHDMI);
    }
    AMLOG_INFO("mode.vout_mask 0x%x, mModeConfig.playback_vout_index %d.\n", mode.vout_mask, mpSharedRes->encoding_mode_config.playback_vout_index);

    mode.input_config.specifided = 1;
    mode.input_config.preview_vout_index = mpSharedRes->encoding_mode_config.preview_vout_index;
    mode.input_config.preview_alpha = mpSharedRes->encoding_mode_config.preview_alpha;
    mode.input_config.vout_index = mpSharedRes->encoding_mode_config.playback_vout_index;

    mode.input_config.pb_display_enabled = mpSharedRes->encoding_mode_config.pb_display_enabled;
    mode.input_config.preview_display_enabled = mpSharedRes->encoding_mode_config.preview_enabled;
    mode.input_config.preview_in_pip = mpSharedRes->encoding_mode_config.preview_in_pip;
    mode.input_config.pb_display_in_pip = mpSharedRes->encoding_mode_config.playback_in_pip;

    mode.input_config.main_width = mpSharedRes->encoding_mode_config.main_win_width;
    mode.input_config.main_height = mpSharedRes->encoding_mode_config.main_win_height;

    mode.input_config.enc_left = mpSharedRes->encoding_mode_config.enc_offset_x;
    mode.input_config.enc_top = mpSharedRes->encoding_mode_config.enc_offset_y;
    mode.input_config.enc_width = mpSharedRes->encoding_mode_config.enc_width;
    mode.input_config.enc_height = mpSharedRes->encoding_mode_config.enc_height;

    mode.input_config.preview_left = mpSharedRes->encoding_mode_config.preview_left;
    mode.input_config.preview_top = mpSharedRes->encoding_mode_config.preview_top;
    mode.input_config.preview_width = mpSharedRes->encoding_mode_config.preview_width;
    mode.input_config.preview_height = mpSharedRes->encoding_mode_config.preview_height;

    mode.input_config.pb_display_left = mpSharedRes->encoding_mode_config.pb_display_left;
    mode.input_config.pb_display_top = mpSharedRes->encoding_mode_config.pb_display_top;
    mode.input_config.pb_display_width = mpSharedRes->encoding_mode_config.pb_display_width;
    mode.input_config.pb_display_height = mpSharedRes->encoding_mode_config.pb_display_height;

    //previewC related
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].enable = mpSharedRes->encoding_mode_config.previewc_rawdata_enabled;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].use_preview_buffer_id = DSP_PREVIEW_ID_C;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].scaled_width = mpSharedRes->encoding_mode_config.previewc_scaled_width;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].scaled_height = mpSharedRes->encoding_mode_config.previewc_scaled_height;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].crop_offset_x = mpSharedRes->encoding_mode_config.previewc_crop_offset_x;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].crop_offset_y = mpSharedRes->encoding_mode_config.previewc_crop_offset_y;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].crop_width = mpSharedRes->encoding_mode_config.previewc_crop_width;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].crop_height = mpSharedRes->encoding_mode_config.previewc_crop_height;

    AMLOG_INFO("[dsp mode]: video sink, enter duplex mode begin.\n");
    if ((ret = ioctl(mIavFd, IAV_IOC_DUPLEX_ENTER_MODE, &mode)) < 0) {
        perror("IAV_IOC_DUPLEX_ENTER_MODE");
        AM_ERROR("[dsp mode]: (video sink): IAV_IOC_DUPLEX_ENTER_MODE fail, ret %d.\n", ret);
        return ME_OS_ERROR;
    }
    AMLOG_INFO("[dsp mode]: video sink, enter duplex mode end.\n");

    //store back arguments, need mutex?
    pthread_mutex_lock(&mpSharedRes->mMutex);
    StoreBackDuplexSettings(&mpSharedRes->encoding_mode_config, &mode.input_config);
    pthread_mutex_unlock(&mpSharedRes->mMutex);

    AMLOG_INFO(" after duplex mode, mode.vout_mask 0x%x, mModeConfig.playback_vout_index %d.\n", mode.vout_mask, mpSharedRes->encoding_mode_config.playback_vout_index);
    if (eVoutLCD == mpSharedRes->encoding_mode_config.playback_vout_index) {
        AMLOG_INFO("only support LCD playback in duplex mode.\n");
        mVoutConfigMask = 1<<eVoutLCD;
    } else if (eVoutHDMI == mpSharedRes->encoding_mode_config.playback_vout_index) {
        AMLOG_INFO("only support HDMI playback in duplex mode.\n");
        mVoutConfigMask = 1<<eVoutHDMI;
    } else {
        AM_ERROR("BAD playback vout index %d.\n", mpSharedRes->encoding_mode_config.playback_vout_index);
        mVoutConfigMask = 1<<eVoutHDMI;
    }

    iav_duplex_start_vcap_t vcap;
    memset(&vcap, 0, sizeof(vcap));

    if ((ret = ioctl(mIavFd, IAV_IOC_DUPLEX_START_VCAP, &vcap)) < 0) {
        perror("IAV_IOC_DUPLEX_START_VCAP");
        AM_ERROR("[dsp mode]: (video sink): IAV_IOC_DUPLEX_START_VCAP, return %d.\n", ret);
    }
    mbEnterDuplexMode = true;
    return ME_OK;
}

AM_ERR CAmbaVideoSink::Construct()
{
    AM_INFO("****CAmbaVideoSink::Construct .\n");
    AM_ERR err = inherited::Construct();
    if (err != ME_OK) {
        AM_ERROR("CAmbaVideoSink::Construct fail err %d .\n", err);
        return err;
    }
    DSetModuleLogConfig(LogModuleAmbaVideoSink);

    AM_ASSERT(mpSharedRes);
    pthread_mutex_lock(&mpSharedRes->mMutex);
    //AM_ASSERT(mpSharedRes->mbIavInited);
    //AM_ASSERT(mpSharedRes->mIavFd >= 0);
    if (mpSharedRes->mbIavInited) {
        AMLOG_INFO("amba_video_sink: mpSharedRes->mbIavInited iavfd %d, use it.\n", mpSharedRes->mIavFd);
        mIavFd = mpSharedRes->mIavFd;
        AM_ASSERT(mIavFd >= 0);
    } else {
        AMLOG_WARN("amba_video_sink: have no iav fd? opend here.\n");
        if ((mIavFd = open("/dev/iav", O_RDWR, 0)) < 0) {
            AM_PERROR("/dev/iav");
            return ME_ERROR;
        }
        mbIavFdOwner = true;
        AM_ASSERT(mIavFd > 0);
        mpSharedRes->mIavFd = mIavFd;
        mpSharedRes->mbIavInited = 1;
    }
    pthread_mutex_unlock(&mpSharedRes->mMutex);

    if ((mpVideoInputPin = CAmbaVideoSinkInput::Create(this)) == NULL) {
        AM_ERROR("Create inputpin fail.\n");
        return ME_ERROR;
    }

    if ((mpSeqConfigData = (AM_U8 *)malloc(DUDEC_MAX_SEQ_CONFIGDATA_LEN)) == NULL) {
        AM_ERROR("NO memory.\n");
        return ME_NO_MEMORY;
    }
    mSeqConfigDataSize = DUDEC_MAX_SEQ_CONFIGDATA_LEN;

    mpClockManager = (IClockManager *)mpEngine->QueryEngineInterface(IID_IClockManager);
    if(!mpClockManager) {
        AM_ERROR("VideoSink::Construct: without mpClockManager?\n");
        return ME_ERROR;
    }

    return ME_OK;
}

CAmbaVideoSink::~CAmbaVideoSink()
{
    AMLOG_DESTRUCTOR("~CAmbaVideoSink start, mIavFd %d.\n", mIavFd);

    if (mpSimpleDecAPI) {
        //mpSimpleDecAPI->ReleaseDecoder(mDspIndex);
        DestroySimpleDecAPI(mpSimpleDecAPI);
        mpSimpleDecAPI = NULL;
    }

    if (mIavFd >=0 && mbEnterDuplexMode) {
        AMLOG_INFO("[dsp mode]: (video sink) leave duplex mode begin.\n");
         //leave duplex mode
         if (ioctl(mIavFd, IAV_IOC_DUPLEX_LEAVE_MODE) < 0) {
             AM_ERROR("IAV_IOC_DUPLEX_LEAVE_MODE");
         }
         mbEnterDuplexMode = false;
         AMLOG_INFO("[dsp mode]: (video sink) leave duplex mode done.\n");
    }

    if (mbIavFdOwner && mIavFd >=0) {
        AMLOG_INFO("close iavfd(%d) in amba video sink.\n", mIavFd);
        close(mIavFd);
        mIavFd = -1;
    }

    AM_DELETE(mpVideoInputPin);

    if (mpSeqConfigData) {
        free(mpSeqConfigData);
        mpSeqConfigData = NULL;
    }

    AMLOG_DESTRUCTOR("~CAmbaVideoSink end.\n");
}

void CAmbaVideoSink::Delete()
{
    AMLOG_DESTRUCTOR("CAmbaVideoSink::Delete start, mIavFd %d.\n", mIavFd);

    if (mpSimpleDecAPI) {
        //mpSimpleDecAPI->ReleaseDecoder(mDspIndex);
        DestroySimpleDecAPI(mpSimpleDecAPI);
        mpSimpleDecAPI = NULL;
    }

    if (mIavFd >=0 && mbEnterDuplexMode) {
        AMLOG_INFO("[dsp mode]: video sink, leave duplex mode begin.\n");
         //leave duplex mode
         if (ioctl(mIavFd, IAV_IOC_DUPLEX_LEAVE_MODE) < 0) {
             AM_ERROR("IAV_IOC_DUPLEX_LEAVE_MODE");
         }
         mbEnterDuplexMode = false;
         AMLOG_INFO("[dsp mode]: video sink, leave duplex mode done.\n");
    }

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


    if (mbIavFdOwner && mIavFd >=0) {
        AMLOG_INFO("close iavfd(%d) in amba video sink.\n", mIavFd);
        close(mIavFd);
        mIavFd = -1;
    }

    AM_DELETE(mpVideoInputPin);
    mpVideoInputPin = NULL;
    AMLOG_DESTRUCTOR("CAmbaVideoSink::Delete before inherited::Delete().\n");
    inherited::Delete();
    AMLOG_DESTRUCTOR("CAmbaVideoSink::Delete end.\n");
}

void *CAmbaVideoSink::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IClockObserver)
        return (IClockObserver*)this;
    else if (refiid == IID_IVideoOutput)
        return (IVideoOutput*)this;
    else if (refiid == IID_IRenderer)
        return (IRender*)this;
    return inherited::GetInterface(refiid);
}

bool CAmbaVideoSink::ReadInputData()
{
    AM_ASSERT(!mpBuffer);

    if (!mpVideoInputPin->PeekBuffer(mpBuffer)) {
        AM_ERROR("No buffer?\n");
        return false;
    }

    return true;
}

bool CAmbaVideoSink::ProcessCmd(CMD& cmd)
{
    AM_ERR err;
    AMLOG_CMD("CAmbaVideoSink::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);

    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            if (mpSimpleDecAPI && !mbDecAPIStopped) {
                AMLOG_INFO("[flow cmd]: Call mpSimpleDecAPI->Stop\n");
                mpSimpleDecAPI->Stop(mDspIndex, 0);
                mbDecAPIStopped = true;
                mpSimpleDecAPI->ReleaseDecoder(mDspIndex);
                AMLOG_INFO("[flow cmd]: Call mpSimpleDecAPI->Stop done\n");
            }
            CmdAck(ME_OK);
            AMLOG_INFO("CAmbaVideoSink::ProcessCmd, STOP cmd.\n");
            break;

        case CMD_PAUSE:
            AM_ASSERT(!mbPaused);
            mbPaused = true;
            break;

        case CMD_RESUME:
            if(msState == STATE_PENDING){
                msState = STATE_IDLE;
                AMLOG_INFO("CAmbaVideoSink: CMD_RESUME STATE %d .\n",msState);
            }
            mbPaused = false;
            break;

        case CMD_FLUSH:
            AMLOG_INFO("CAmbaVideoSink: CMD_FLUSH STATE %d .\n",msState);
            clearDecoder();
            msState = STATE_PENDING;
            CmdAck(ME_OK);
            AMLOG_INFO("CAmbaVideoSink: CMD_FLUSH done, STATE %d .\n", msState);
            break;

        case CMD_AVSYNC:
            CmdAck(ME_OK);
            break;

        case CMD_BEGIN_PLAYBACK:
            AM_ASSERT(msState == STATE_PENDING);
            AM_ASSERT(!mpBuffer);
            AMLOG_INFO("CAmbaVideoSink: CMD_BEGIN_PLAYBACK STATE %d .\n", msState);
            if (mpSimpleDecAPI && mbDecAPIStopped) {
                AMLOG_WARN("re-init dec api.\n");
                err = mpSimpleDecAPI->InitDecoder(mDspIndex, &mDecParam, mVoutNumber, &mVoutConfig[mVoutStartIndex]);
                AM_ASSERT(ME_OK == err);
                if (ME_OK != err) {
                    AM_ERROR("mpSimpleDecAPI->InitDecoder fail, ret %d.\n", err);
                    msState = STATE_ERROR;
                    break;
                }
                mbDecAPIStopped = false;
            }
            mbStreamStart = false;
            msState = STATE_IDLE;
            mbPaused = false;

            mTimeOffset = mpSharedRes->mPlaybackStartTime;
            break;

        case CMD_SOURCE_FILTER_BLOCKED:
            if (0 == mbAlreadySendSyncMsg) {
                AMLOG_INFO("source filter blocked, post avsync msg\n");
                PostEngineMsg(IEngine::MSG_AVSYNC);
                mbAlreadySendSyncMsg = 1;
            }
            break;

        case CMD_REALTIME_SPEEDUP:
            mbNeedSpeedUp = 1;
            break;

        default:
            AM_ERROR("wrong cmd %d.\n",cmd.code);
            break;
	}
	return false;
}

void CAmbaVideoSink::OnRun()
{
    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
//    AM_INT ret = 0;
    CmdAck(ME_OK);

    mbRun = true;
    mbFillEOS = false;
    msState = STATE_IDLE;

    PostEngineMsg(IEngine::MSG_READY);
    while (1) {
        AMLOG_INFO("CAmbaVideoSink, wait Start cmd...\n");
        GetCmd(cmd);
        if (cmd.code == AO::CMD_START) {
            AMLOG_INFO("CAmbaVideoSink, Start cmd comes.\n");
            CmdAck(ME_OK);
            break;
        } else if (cmd.code == AO::CMD_AVSYNC) {
            //must not comes here
            AM_ASSERT(0);
            CmdAck(ME_OK);
        } else if (cmd.code == AO::CMD_STOP) {
            AMLOG_INFO("CAmbaVideoSink, Stop cmd comes...\n");
            CmdAck(ME_OK);
            mbRun = false;
            break;
        } else {
            AM_ERROR("how to handle this cmd, %d.\n", cmd.code);
        }
    }

    //post avsync msg
    //PostEngineMsg(IEngine::MSG_AVSYNC);

    if (ME_OK != ConfigDecoder()) {
        msState = STATE_ERROR;
    }

    mEstimatedLatency = mpSharedRes->mVideoTicks/2;//to do

    while(mbRun) {
        AMLOG_STATE("Amba video sink: start switch, msState=%d, %d input data. \n", msState, mpVideoInputPin->mpBufferQ->GetDataCnt());

        switch (msState) {

            case STATE_IDLE:
                if(mbPaused) {
                    msState = STATE_PENDING;
                    if (mpSimpleDecAPI) {
                        mpSimpleDecAPI->TrickPlay(mDspIndex, 0);
                    }
                    AMLOG_INFO("Decoder: Enter STATE_PENDING .\n");
                    break;
                }

                if (mbNeedSpeedUp) {
                    speedUp();
                    mbNeedSpeedUp = 0;
                    break;
                }

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

            case STATE_ERROR:
            case STATE_PENDING:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
                if (!mbPaused && mpSimpleDecAPI) {
                    mpSimpleDecAPI->TrickPlay(mDspIndex, 1);
                }
                break;

            case STATE_READY:
                AM_ASSERT(mpBuffer);
                if (mpBuffer->GetType() == CBuffer::DATA) {
                    ProcessBuffer(mpBuffer);
                    if (mpBuffer) {
                        //need check if NULL: udec error will release this buffer
                        mpBuffer->Release();
                        mpBuffer = NULL;
                    }
                } else if (mpBuffer->GetType() == CBuffer::EOS) {
                    AMLOG_INFO("!!!!Notice, bit-stream comes to end, should be Legal bit-stream.\n");
                    if (!mbESMode) {
                        //fill eos
                        AM_U8* pFrameStart = mpCurrAddr;
                        mpCurrAddr = FillEOS(mpCurrAddr);
                        mpSimpleDecAPI->Decode(mDspIndex, pFrameStart, mpCurrAddr);
                        mpBuffer->Release();
                        mbESMode = 0;
                        mpBuffer = NULL;
                    }
                    PostEngineMsg(IEngine::MSG_EOS);
                    msState = STATE_PENDING;
                    AMLOG_INFO("Sending EOS...\n");
                } else if (mpBuffer->GetType() == CBuffer::TEST_ES) {
                    AMLOG_INFO("!!!!Notice, try send es mode.\n");
                    ProcessTestESBuffer();
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    msState = STATE_IDLE;
                    mbESMode = 1;
                }
                break;

            case STATE_WAIT_PROPER_START_TIME:
                AM_ASSERT(!mpBuffer);
                onStateWaitProperStartTime();
                break;

            default:
                AM_ERROR(" %d",(AM_UINT)msState);
                break;
        }
    }
}

void CAmbaVideoSink::onStateWaitProperStartTime()
{
    AM_ASSERT(!mbRecievedSyncCmd);
    CMD cmd;

    if (!mbRecievedSyncCmd) {
        //wait sync cmd first
        AMLOG_INFO("[VSink flow]: onStateWaitProperTime before wait sync cmd.\n");
        while (1) {
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            ProcessCmd(cmd);
            if (CMD_AVSYNC == cmd.code) {
                mbRecievedSyncCmd = 1;
                AMLOG_INFO("[VSink flow]: onStateWaitProperTime wait sync cmd done.\n");
                break;
            }
            if (msState != STATE_WAIT_PROPER_START_TIME || !mbRun) {
                AM_ERROR("[VR flow]: change state cmd comes, in waiting proper time, break out, msState %d, mbRun %d.\n", msState, mbRun);
                return;
            }
        }
    }

    AM_ASSERT(mbRecievedSyncCmd);
    if (mbRecievedSyncCmd) {
        //wait some time
        updateStatus();
    } else {
        AM_ERROR("Why comes here\n");
        return;
    }

    am_pts_t curtime = mpClockManager->GetCurrentTime();
    AM_UINT skip = 0;
    if (!mbCurrentPbPTSValid) {
        AMLOG_WARN("[VSink flow]: No valid video first pts, must not comes here, escape from waiting logic.\n");
        skip = 1;
    } else if ((mCurrentPbPTS) < (curtime + mEstimatedLatency + mWaitThreshold)) {
        AMLOG_INFO("[VSink flow]: exactly matched time, start immediately, curtime %llu, mCurrentPbPTS %llu.\n", curtime, mCurrentPbPTS);
        skip = 1;
    } else if ((mCurrentPbPTS) > (curtime + 100*mNotSyncThreshold + mEstimatedLatency)) {
        AMLOG_WARN("[VSink flow]: mCurrentPbPTS is wrong? gap greater than 100*mNotSyncThreshold.\n");
        skip = 1;
    } else {
        curtime = mCurrentPbPTS - mEstimatedLatency;
    }

    if (skip) {
        msState = STATE_IDLE;
        return;
    } else {
        AMLOG_INFO("[VSink flow]: set timer %llu, cur time %llu.\n", curtime, mpClockManager->GetCurrentTime());
        mpClockManager->SetTimer(this, curtime);
    }

    AMLOG_INFO("[VSink flow]: onStateWaitProperTime wait sync cmd done, wait proper time.\n");
    while (1) {
        mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
        ProcessCmd(cmd);
        if (CMD_TIMERNOTIFY == cmd.code) {
            AMLOG_INFO("[VSink flow]: onStateWaitProperTime wait timenotify done, start playback....\n");
            msState = STATE_IDLE;
            return;
        }
        if (msState != STATE_WAIT_PROPER_START_TIME || !mbRun) {
            AM_ERROR("[VSink flow]: change state cmd comes, in waiting proper time, break out, msState %d, mbRun %d.\n", msState, mbRun);
            return;
        }
    }
}

AM_ERR CAmbaVideoSink::Stop()
{
    AMLOG_INFO("CAmbaVideoSink before inherited::Stop()\n");
    inherited::Stop();
    AMLOG_INFO("CAmbaVideoSink after inherited::Stop()\n");
    mpCurrAddr = mpStartAddr;
    return ME_OK;
}

void CAmbaVideoSink::GetInfo(INFO& info)
{
    info.nInput = 1;
    info.nOutput = 0;
    info.mPriority = 8;
    info.mFlags = SYNC_FLAG;
    info.mIndex = mDspIndex;
    info.pName = "AmbaVSink";
}

IPin* CAmbaVideoSink::GetInputPin(AM_UINT index)
{
    if (0 == index)
        return mpVideoInputPin;
    return NULL;
}

AM_ERR CAmbaVideoSink::SetInputFormat(CMediaFormat *pFormat)
{
    mpStream = (AVStream*)pFormat->format;
    mpCodec = (AVCodecContext*)mpStream->codec;

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

    //check dsp mode
    if (ME_OK != enterDuplexMode()) {
        AM_ERROR("enterDuplexMode fail.\n");
        return ME_ERROR;
    }

    return ME_OK;
}

AM_UINT CAmbaVideoSink::_NextESDatapacket(AM_U8 * start, AM_U8 * end, AM_UINT * totFrames)
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

AM_ERR CAmbaVideoSink::ProcessTestESBuffer()
{
//    AM_ERR err;
    AM_U8 *pFrameStart = NULL;
    iav_udec_decode_t dec;
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
    char dumpfilePath[DAMF_MAX_FILENAME_LEN+1];
    snprintf(dumpfilePath, sizeof(dumpfilePath), "%s/es.data", AM_GetPath(AM_PathDump));

    pFile = fopen(dumpfilePath, "rb");
/*
#if PLATFORM_LINUX
    pFile = fopen("/tmp/mmcblk0p1/es.data", "rb");
#elif PLATFORM_ANDROID
    pFile = fopen("/sdcard/es.data", "rb");
#endif*/

    AMLOG_INFO("****start send es data directly.\n");

    if (!pFile) {
        AMLOG_ERROR("cannot open input es file.\n");
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

            AM_ASSERT(mpSimpleDecAPI);
            mpSimpleDecAPI->RequestBitStreamBuffer(mDspIndex, pFrameStart, sendsize);

            pFrameStart = mpCurrAddr;
            mpCurrAddr = CopyToBSB(mpCurrAddr, pCur, sendsize);

            memset(&dec, 0, sizeof(dec));
            dec.udec_type = muDecType;
            dec.decoder_id = mDspIndex;
            dec.u.fifo.start_addr = pFrameStart;
            dec.u.fifo.end_addr = mpCurrAddr;
            dec.num_pics = sendFrames;

            AMLOG_DEBUG("decoding size %d.\n", sendsize);
            AMLOG_DEBUG("pFrameStart %p, mpCurrAddr %p, diff %p.\n", pFrameStart, mpCurrAddr, (AM_U8*)(mpCurrAddr + mSpace - pFrameStart));
            mpSimpleDecAPI->Decode(mDspIndex, pFrameStart, mpCurrAddr);

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
                    mpSimpleDecAPI->RequestBitStreamBuffer(mDspIndex, mpCurrAddr, sendsize);
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
            mpSimpleDecAPI->RequestBitStreamBuffer(mDspIndex, mpCurrAddr, sendsize);
            mpCurrAddr = CopyToBSB(mpCurrAddr, pEs, size);
            break;//done
        }

    }

    AMLOG_INFO("****send es data done.\n");
    free(pEs);
    fclose(pFile);
    return ME_OK;
}

AM_ERR CAmbaVideoSink::ProcessBuffer(CBuffer *pBuffer)
{
    AM_ERR err;
    AM_U8 *pFrameStart;
    AM_ASSERT(!pBuffer->IsEOS());
    AM_ASSERT(mpSimpleDecAPI);

    if (mpCurrAddr == mpEndAddr)//for bits fifo wrap case
        mpCurrAddr = mpStartAddr;

    AM_U8 *pRecoveryAddr = mpCurrAddr;  // added for recovery when dropping packets

    AVPacket *mpPacket = (AVPacket*)((AM_U8*)pBuffer + sizeof(CBuffer));
    if (mpPacket->size <= 0) {
        AM_ERROR(" data size <= 0 (%d), %p?\n", mpPacket->size, mpPacket);
        msState = STATE_IDLE;
        return ME_OK;
    }

    //safe check, make sure begin with key frame(IDR)
    if (mbWaitKeyFrame) {
        if (!(mpPacket->flags & AV_PKT_FLAG_KEY)) {
            AM_ERROR(" begin non-key frame, discard them.\n");
            msState = STATE_IDLE;
            return ME_OK;
        } else {
            mbWaitKeyFrame = 0;
        }
    }

    err = mpSimpleDecAPI->RequestBitStreamBuffer(mDspIndex, mpCurrAddr, mpPacket->size + 1024);//for safe + 1024

    if (err == ME_ERROR) {
        msState = STATE_PENDING;
        AMLOG_ERROR("ambadec goto pending 1, %d.\n", msState);
    } else if (err == ME_UDEC_ERROR) {
        AM_ERROR("mpSimpleDecAPI->RequestBitStreamBuffer() return NO-PERMISSION.\n");
        updateStatus();
        if (mDSPErrorLevel >= DSP_DECODER_ERROR_LEVEL_FATAL) {
            AMLOG_ERROR("DSP report error, clear current decoding, and re-setup it .\n");
            clearDecoder();
        }
        msState = STATE_IDLE;
    } else {
        pFrameStart = mpCurrAddr;
        if (!mbAddUdecWarpper) {
            FeedConfigData(mpPacket);
        } else if (mpCodec->codec_id != CODEC_ID_MPEG1VIDEO&&mpCodec->codec_id != CODEC_ID_MPEG2VIDEO) {
            FeedConfigDataWithUDECWrapper(mpPacket);
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
            AM_U8 *ptr = mpPacket->data;
            AM_INT curPos = 0;
            AM_INT len = 0;

            if (mH264DataFmt == H264_FMT_AVCC) {
                //AM_ASSERT((mH264AVCCNaluLen == 2) || (mH264AVCCNaluLen == 3) || (mH264AVCCNaluLen == 4));

                while (1) {
                    len = 0;
                    for (AM_INT index = 0; index < mH264AVCCNaluLen; index++) {
                        len = len<<8 | ptr[index];
                    }

                    ptr += mH264AVCCNaluLen;
                    curPos += mH264AVCCNaluLen;

                    if (len <= 0 || (curPos + len > mpPacket->size)) {
                        AMLOG_ERROR("CAmbaVideoSink::ProcessBuffer error: pkt_size:%d len:%d mH264AVCCNaluLen:%d curPos:%d\n",
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
                    mpCurrAddr = CopyToBSB(mpCurrAddr, unitDelimiter, sizeof(startcode)/sizeof(startcode[0]));
                }
            }
        } else {
            AM_ERROR("add implement here.\n");
            mpCurrAddr = CopyToBSB(mpCurrAddr,mpPacket->data, mpPacket->size);
        }

        if (mpCurrAddr > pFrameStart)
            AMLOG_DEBUG("decoding size %d.\n", mpCurrAddr - pFrameStart);
        else
            AMLOG_DEBUG("decoding size %d.\n", mpCurrAddr + mSpace - pFrameStart);

#ifdef AM_DEBUG
        dumpEsData(pFrameStart, mpCurrAddr);
#endif

        AMLOG_DEBUG("pFrameStart %p, mpCurrAddr %p, diff %d.\n", pFrameStart, mpCurrAddr, mpCurrAddr + mSpace - pFrameStart);
        //AMLOG_DEBUG("AmbaVideoDec: DecodeBuffer start, start %p, end %p, diff %d, totdiff %d, %d.\n", pFrameStart, mpCurrAddr, mpCurrAddr - pFrameStart, mpCurrAddr - mpStartAddr, mpEndAddr - mpCurrAddr);
        err = mpSimpleDecAPI->Decode(mDspIndex, pFrameStart, mpCurrAddr);
        AMLOG_DEBUG("AmbaVideoDec: DecodeBuffer done.\n");

        if (err == ME_UDEC_ERROR) {
            AM_ERROR("mpSimpleDecAPI->Decode() return NO-PERMISSION.\n");
            updateStatus();
            if (mDSPErrorLevel >= DSP_DECODER_ERROR_LEVEL_FATAL) {
                AMLOG_ERROR("DSP report error, clear current decoding, and re-setup it .\n");
                clearDecoder();
            }
            msState = STATE_IDLE;
        } else if (err != ME_OK) {
            msState = STATE_PENDING;
            return err;
        } else {
            if (!mbDoAvSync) {
                if (0 == mbAlreadySendSyncMsg) {
                    PostEngineMsg(IEngine::MSG_AVSYNC);
                    mbAlreadySendSyncMsg = 1;
                }
                msState = STATE_IDLE;
                return ME_OK;
            }
            updateStatus();
            if (mbWaitFirstValidPTS) {
                if (mbCurrentPbPTSValid) {
                    mBeginPbPTS = mCurrentPbPTS;
                    mTimeOffset = mpSharedRes->mPlaybackStartTime;
                    mbWaitFirstValidPTS = 0;
                    AMLOG_INFO("[video sink flow]: get first pts(maybe not actual first pts in bit-stream) %llu, enter wait proper time state.\n", mCurrentPbPTS);
                    //post sync msg if needed
                    AM_ASSERT(!mbAlreadySendSyncMsg);
                    if (0 == mbAlreadySendSyncMsg) {
                        PostEngineMsg(IEngine::MSG_AVSYNC);
                        mbAlreadySendSyncMsg = 1;
                    }
                    msState = STATE_WAIT_PROPER_START_TIME;
                    return ME_OK;
                }
            } else {

                //simple sync on the fly
                if (mpClockManager) {
                    if (mbCurrentPbPTSValid) {
                        am_pts_t cur_time = mpClockManager->GetCurrentTime();
                        //would be master
                        if (((cur_time + mNotSyncThreshold) < (mCurrentPbPTS + 1500)) || ((mCurrentPbPTS + 1500 + mNotSyncThreshold) < cur_time)) {
                            AMLOG_WARN("[AV SYNC]: system clock has significant difference %lld, system clock %llu, last pb time %llu.\n", cur_time - mCurrentPbPTS - 1500, cur_time, mCurrentPbPTS);
                            mpClockManager->SetClockMgr(mCurrentPbPTS + 1500);//+ about 1/2 latency
                        } else {
                            AMLOG_PTS("[V Sink PTS]: diff %lld, system clock %llu, last pb time %llu.\n", cur_time - mCurrentPbPTS - 1500, cur_time, mCurrentPbPTS);
                        }
                    }
                }
            }
        }
        msState = STATE_IDLE;
    }

    return err;
}

void CAmbaVideoSink::GenerateConfigData()
{
    mSeqConfigDataLen = 0;
    mPicConfigDataLen = 0;

    switch (mpCodec->codec_id) {

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

                if (mpCodec->extradata[0] != 0x01) {
                    AMLOG_INFO("extradata is annex-b format.\n");
                    mH264DataFmt = H264_FMT_ANNEXB;

                    if (mpCodec->extradata_size <= mSeqConfigDataSize ||
                        AdjustSeqConfigDataSize(mpCodec->extradata_size)) {

                        memcpy(mpSeqConfigData + mSeqConfigDataLen, mpCodec->extradata, mpCodec->extradata_size);
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

                            memcpy(mpSeqConfigData + mSeqConfigDataLen, startcode, sizeof(startcode));
                            mSeqConfigDataLen += sizeof(startcode);

                            memcpy(mpSeqConfigData + mSeqConfigDataLen, mpCodec->extradata + 8, spss);
                            mSeqConfigDataLen += spss;

                            memcpy(mpSeqConfigData + mSeqConfigDataLen, startcode, sizeof(startcode));
                            mSeqConfigDataLen += sizeof(startcode);

                            memcpy(mpSeqConfigData + mSeqConfigDataLen, mpCodec->extradata + 6 + 2 + spss + 1 + 2, ppss);
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
    PrintBitstremBuffer(mpSeqConfigData ,mSeqConfigDataLen);
#endif

}

bool CAmbaVideoSink::AdjustSeqConfigDataSize(AM_INT size)
{
    AM_U8 *pBuf = (AM_U8 *)malloc(size);
    if (pBuf) {
        free(mpSeqConfigData);
        mSeqConfigDataSize = size;
        mpSeqConfigData = pBuf;
        AMLOG_INFO("AdjustSeqConfigDataSize success.\n");
        return true;
    } else {
        AM_ERROR("NO memory, request size %d.\n", size);
        return false;
    }
}

void CAmbaVideoSink::FeedConfigDataWithUDECWrapper(AVPacket *mpPacket)
{
    AM_UINT auLength = 0;
    AM_UINT pesHeaderLen = 0;

    //send seq data
    if (!mbConfigData) {
        mpCurrAddr = CopyToBSB(mpCurrAddr, mUSEQHeader, DUDEC_SEQ_HEADER_LENGTH);
        mpCurrAddr = CopyToBSB(mpCurrAddr, mpSeqConfigData, mSeqConfigDataLen);
        mbConfigData = true;

#ifdef AM_DEBUG
        AMLOG_BINARY("Print UDEC SEQ: %d.\n", DUDEC_SEQ_HEADER_LENGTH);
        PrintBitstremBuffer(mUSEQHeader, DUDEC_SEQ_HEADER_LENGTH);
        AMLOG_BINARY("Print seq config data: %d.\n", mSeqConfigDataLen);
        PrintBitstremBuffer(mpSeqConfigData, mSeqConfigDataLen);
#endif
    }

    auLength = mPicConfigDataLen + mpPacket->size;
    if (mpCodec->codec_id == CODEC_ID_H264 || mpCodec->codec_id == CODEC_ID_WMV3) {
        auLength -= 4;
    }

    AMLOG_PTS("AmbaVdec [PTS], current pts %lld.\n", mpPacket->pts);
    if (mpPacket->pts >= 0) {
        pesHeaderLen = FillPESHeader(mUPESHeader, (AM_U32)(mpPacket->pts&0xffffffff), (AM_U32)(((AM_U64)mpPacket->pts) >> 32), auLength, 1, 0);
    } else {
        pesHeaderLen = FillPESHeader(mUPESHeader, 0, 0, auLength, 0, 0);
    }

    AM_ASSERT(pesHeaderLen <= DUDEC_PES_HEADER_LENGTH);

    //send udec pes header
    AMLOG_DEBUG("comes here?, pesHeaderLen %d, mpCurrAddr %p.\n", pesHeaderLen, mpCurrAddr);
    mpCurrAddr = CopyToBSB(mpCurrAddr, mUPESHeader, pesHeaderLen);
    AMLOG_DEBUG("done pesHeaderLen %d.mpCurrAddr %p.\n", pesHeaderLen, mpCurrAddr);

    //send pic config data
    if (mPicConfigDataLen) {
        mpCurrAddr = CopyToBSB(mpCurrAddr, mPicConfigData, mPicConfigDataLen);
    }

#ifdef AM_DEBUG
    AMLOG_BINARY("Print UDEC PES Header: %d.\n", pesHeaderLen);
    PrintBitstremBuffer(mUPESHeader, pesHeaderLen);
    AMLOG_BINARY("Print pic config data: %d.\n", mPicConfigDataLen);
    PrintBitstremBuffer(mPicConfigData, mPicConfigDataLen);
#endif

}

void CAmbaVideoSink::FeedConfigData(AVPacket *mpPacket)
{
    switch (mpCodec->codec_id) {
        case CODEC_ID_H264: {
            AM_U8 startcode[4] = {0, 0, 0,0x01};
                if (mpPacket->flags&AV_PKT_FLAG_KEY) {
                    mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
                    AM_INT spss = BE_16(mpCodec->extradata + 6);
                    mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata + 8, spss);

                    mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
                    AM_INT ppss = BE_16(mpCodec->extradata + 6 + 2 + spss + 1);
                    mpCurrAddr = CopyToBSB(mpCurrAddr, mpCodec->extradata + 6 + 2 + spss + 1 + 2, ppss);
                    AM_INFO("-------spss:%d,ppss:%d-----\n",spss,ppss);
                }
                mpCurrAddr = CopyToBSB(mpCurrAddr, startcode, sizeof(startcode));
            }
            break;

        default:
            AM_ERROR("NOT support format %d.\n", CODEC_ID_H264);
            break;
    }
}

AM_ERR CAmbaVideoSink::OnTimer(am_pts_t curr_pts)
{
    mpWorkQ->PostMsg(CMD_TIMERNOTIFY);
    return ME_OK;
}

AM_INT CAmbaVideoSink::configVout(AM_INT vout, AM_INT video_rotate, AM_INT video_flip, AM_INT target_pos_x, AM_INT target_pos_y, AM_INT target_width, AM_INT target_height)
{
    iav_udec_vout_configs_t iav_cvs;
    iav_udec_vout_config_t  vout_config;

    iav_cvs.vout_config = &vout_config;
    iav_cvs.udec_id = mDspIndex;
    iav_cvs.num_vout = 1;
    iav_cvs.vout_config->disable = mVoutConfig[vout].enable ? 0 : 1;
    iav_cvs.vout_config->vout_id= (u8)vout;
    iav_cvs.vout_config->rotate = (u8)video_rotate;
    iav_cvs.vout_config->flip = (u8)video_flip;
    iav_cvs.vout_config->zoom_factor_x = mVoutConfig[vout].zoom_factor_x;
    iav_cvs.vout_config->zoom_factor_y = mVoutConfig[vout].zoom_factor_y;
    iav_cvs.input_center_x = mVoutConfig[vout].input_center_x;
    iav_cvs.input_center_y = mVoutConfig[vout].input_center_y;
    iav_cvs.vout_config->win_offset_x = (u16)target_pos_x;
    iav_cvs.vout_config->win_offset_y = (u16)target_pos_y >> mVoutConfig[vout].vout_mode;
    iav_cvs.vout_config->win_width = (u16)target_width;
    iav_cvs.vout_config->win_height = (u16)target_height >> mVoutConfig[vout].vout_mode;
    iav_cvs.vout_config->target_win_offset_x = (u16)target_pos_x;
    iav_cvs.vout_config->target_win_offset_y = (u16)target_pos_y >> mVoutConfig[vout].vout_mode;
    iav_cvs.vout_config->target_win_width = (u16)target_width;
    iav_cvs.vout_config->target_win_height = (u16)target_height >> mVoutConfig[vout].vout_mode;

    AMLOG_INFO("vout:%d,zoom_factor_x = 0x%x,zoom_factor_y = 0x%x,target_width: %d,target_height: %d\n",vout,iav_cvs.vout_config->zoom_factor_x,iav_cvs.vout_config->zoom_factor_y,target_width,target_height);
    return  ioctl(mIavFd, IAV_IOC_DUPLEX_UPDATE_DEC_VOUT_CONFIG, &iav_cvs);
}

AM_INT CAmbaVideoSink::SetInputCenter(AM_INT pic_x, AM_INT pic_y)
{
    iav_udec_vout_configs_t iav_cvs;
    iav_udec_vout_config_t  vout_config[eVoutCnt];
    AMLOG_INFO("SetInputCenter:%dx%d\n", pic_x, pic_y);
    iav_cvs.vout_config = &vout_config[mVoutStartIndex];
    iav_cvs.udec_id = mDspIndex;
    iav_cvs.num_vout = mVoutNumber;
    iav_cvs.input_center_x = pic_x / 2;
    iav_cvs.input_center_y = pic_y / 2;
    for(AM_UINT i = mVoutStartIndex; i < mVoutNumber; i++){
        iav_cvs.vout_config[i].vout_id= i;
        iav_cvs.vout_config[i].disable = mVoutConfig[i].enable ? 0 : 1;
        iav_cvs.vout_config[i].rotate = (u8)mVoutConfig[i].rotate;
        iav_cvs.vout_config[i].flip = (u8)mVoutConfig[i].flip;
        iav_cvs.vout_config[i].win_offset_x = (u16)mVoutConfig[i].pos_x;
        iav_cvs.vout_config[i].win_offset_y = (u16)mVoutConfig[i].pos_y >> mVoutConfig[i].vout_mode;
        iav_cvs.vout_config[i].win_width = (u16)mVoutConfig[i].size_x;
        iav_cvs.vout_config[i].win_height = (u16)mVoutConfig[i].size_y >> mVoutConfig[i].vout_mode;
        iav_cvs.vout_config[i].target_win_offset_x = (u16)mVoutConfig[i].pos_x;
        iav_cvs.vout_config[i].target_win_offset_y = (u16)mVoutConfig[i].pos_y >> mVoutConfig[i].vout_mode;
        iav_cvs.vout_config[i].target_win_width = (u16)mVoutConfig[i].size_x;
        iav_cvs.vout_config[i].target_win_height = (u16)mVoutConfig[i].size_y >> mVoutConfig[i].vout_mode;

        float ratio_x,ratio_y;
        ratio_x = (float)mVoutConfig[i].size_x / pic_x;
        ratio_y = (float)(mVoutConfig[i].size_y >> mVoutConfig[i].vout_mode)/ pic_y;
        //AM_INFO("ratio_x= %f, %f\n",ratio_x,ratio_y);
        iav_cvs.vout_config[i].zoom_factor_x = (int)(ratio_x * 0x10000);
        iav_cvs.vout_config[i].zoom_factor_y = (int)(ratio_y * 0x10000);

        //save input_center and zoom_factor
        mVoutConfig[i].input_center_x = iav_cvs.input_center_x;
        mVoutConfig[i].input_center_y = iav_cvs.input_center_y;
        mVoutConfig[i].zoom_factor_x = iav_cvs.vout_config[i].zoom_factor_x;
        mVoutConfig[i].zoom_factor_y = iav_cvs.vout_config[i].zoom_factor_y;

        mVoutConfig[i].video_x = pic_x;
        mVoutConfig[i].video_y = pic_y;
    }
    return  ioctl(mIavFd, IAV_IOC_DUPLEX_UPDATE_DEC_VOUT_CONFIG, &iav_cvs);
}

AM_ERR CAmbaVideoSink::ChangeInputCenter(AM_INT input_center_x, AM_INT input_center_y)
{
    AMLOG_INFO("ChangeInputCenter: input_center_x %d, input_center_y %d\n",input_center_x,input_center_y);
    if(input_center_x > mVoutConfig[0].input_center_x || input_center_y > mVoutConfig[0].input_center_y){
        AMLOG_ERROR("ChangeInputCenter: the input center out of range\n");
        return ME_BAD_PARAM;
    } else if(input_center_x == mVoutConfig[0].input_center_x && input_center_y == mVoutConfig[0].input_center_y){
        AMLOG_INFO("ChangeInputCenter: same input center, no change\n");
        return ME_OK;
    }
    AM_INT ret;
    ret = SetInputCenter(2*input_center_x, 2*input_center_y);
    if (ret) {
        AMLOG_ERROR("ChangeInputCenter fail. ret %d, input_center_x %d, input_center_y %d .\n", ret, input_center_x, input_center_y);
        return ME_OS_ERROR;
    }

    return ME_OK;
}

AM_ERR CAmbaVideoSink::SetDisplayPositionSize(AM_INT vout, AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height)
{
    AMLOG_INFO("SetDisplayPositionSize: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, pos_x, pos_y, width, height);
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayPositionSize: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    if (0 == mbVoutForPB[vout]) {
        AMLOG_ERROR("SetDisplayPositionSize: vout_id(%d) is not used by playback.\n", vout);
        return ME_BAD_PARAM;
    }

    //check valid
    if ((pos_x + width) > mVoutConfig[vout].width || (pos_y + height) > mVoutConfig[vout].height || (width < 0) || (height < 0) || (pos_x < 0) || (pos_y < 0)) {
        AMLOG_ERROR("SetDisplayPositionSize: pos&size out of range.\n");
        return ME_BAD_PARAM;
    }

    AM_U32 zoom_factor_x, zoom_factor_y;
    zoom_factor_x = mVoutConfig[vout].zoom_factor_x;
    zoom_factor_y = mVoutConfig[vout].zoom_factor_y;

    AM_INT ret;
    ret = configVout(vout, mVoutConfig[vout].rotate, mVoutConfig[vout].flip, pos_x, pos_y, width, height);
    if (ret) {
        AMLOG_ERROR("SetDisplayPositionSize fail. ret %d, pos_x %d, pos_y %d, width %d, height %d.\n", ret, pos_x, pos_y, width, height);
        mVoutConfig[vout].zoom_factor_x = zoom_factor_x;
        mVoutConfig[vout].zoom_factor_y = zoom_factor_y;
        return ME_OS_ERROR;
    }
    mVoutConfig[vout].pos_x = pos_x;
    mVoutConfig[vout].pos_y = pos_y;
    mVoutConfig[vout].size_x = width;
    mVoutConfig[vout].size_y = height;

    AMLOG_INFO("SetDisplayPositionSize done: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    return ME_OK;
}

AM_ERR CAmbaVideoSink::GetDisplayPositionSize(AM_INT vout, AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("GetDisplayPositionSize: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    if (0 == mbVoutForPB[vout]) {
        AMLOG_ERROR("GetDisplayPositionSize: vout_id(%d) is not used by playback.\n", vout);
        return ME_BAD_PARAM;
    }

    *pos_x = mVoutConfig[vout].pos_x;
    *pos_y = mVoutConfig[vout].pos_y;
    *width = mVoutConfig[vout].size_x;
    *height = mVoutConfig[vout].size_y;

    AMLOG_INFO("GetDisplayPositionSize done: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);
    return ME_OK;
}

AM_ERR CAmbaVideoSink::SetDisplayPosition(AM_INT vout, AM_INT pos_x, AM_INT pos_y)
{
    AMLOG_INFO("SetDisplayPosition: vout %d, pos_x %d, pos_y %d.\n", vout, pos_x, pos_y);

    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayPosition: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    if (0 == mbVoutForPB[vout]) {
        AMLOG_ERROR("SetDisplayPosition: vout_id(%d) is not used by playback.\n", vout);
        return ME_BAD_PARAM;
    }

    //check valid
    if ((pos_x + mVoutConfig[vout].size_x) > mVoutConfig[vout].width || (pos_y + mVoutConfig[vout].size_y) > mVoutConfig[vout].height || (pos_x < 0) || (pos_y < 0)) {
        AMLOG_ERROR("SetDisplayPosition: pos out of range.\n");
        return ME_BAD_PARAM;
    }

    AM_INT ret;
    ret = configVout(vout, mVoutConfig[vout].rotate, mVoutConfig[vout].flip, pos_x, pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);
    if (ret) {
        AMLOG_ERROR("SetDisplayPosition fail. ret %d, pos_x %d, pos_y %d .\n", ret, pos_x, pos_y);
        return ME_OS_ERROR;
    }
    mVoutConfig[vout].pos_x = pos_x;
    mVoutConfig[vout].pos_y = pos_y;

    AMLOG_INFO("SetDisplayPosition done: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    return ME_OK;
}

AM_ERR CAmbaVideoSink::SetDisplaySize(AM_INT vout, AM_INT width, AM_INT height)
{
    AMLOG_INFO("SetDisplaySize: vout %d, x_size %d, y_size %d.\n", vout, width, height);

    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplaySize: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    if (0 == mbVoutForPB[vout]) {
        AMLOG_ERROR("SetDisplaySize: vout_id(%d) is not used by playback.\n", vout);
        return ME_BAD_PARAM;
    }

    //check valid
    if ((mVoutConfig[vout].pos_x+ width) > mVoutConfig[vout].width || (mVoutConfig[vout].pos_y + height) > mVoutConfig[vout].height || (width < 0) || (height < 0) ) {
        AMLOG_ERROR("SetDisplaySize: size out of range.\n");
        return ME_BAD_PARAM;
    }

    AM_U32 zoom_factor_x, zoom_factor_y;
    zoom_factor_x = mVoutConfig[vout].zoom_factor_x;
    zoom_factor_y = mVoutConfig[vout].zoom_factor_y;

    AM_INT ret;

    ret = configVout(vout, mVoutConfig[vout].rotate, mVoutConfig[vout].flip, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, width, height);
    if (ret) {
        AMLOG_ERROR("SetDisplaySize fail. ret %d, width %d, height %d .\n", ret, width, height);
        mVoutConfig[vout].zoom_factor_x = zoom_factor_x;
        mVoutConfig[vout].zoom_factor_y = zoom_factor_y;
        return ME_OS_ERROR;
    }
    mVoutConfig[vout].size_x = width;
    mVoutConfig[vout].size_y = height;

    AMLOG_INFO("SetDisplaySize done: vout %d, pos_x %d, pos_y %d, x_size %d, y_size %d.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);
    return ME_OK;
}

AM_ERR CAmbaVideoSink::GetDisplayDimension(AM_INT vout, AM_INT* width, AM_INT* height)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("GetDisplayDimension: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    if (0 == mbVoutForPB[vout]) {
        AMLOG_ERROR("GetDisplayDimension: vout_id(%d) is not used by playback.\n", vout);
        return ME_BAD_PARAM;
    }

    *width = mVoutConfig[vout].width;
    *height = mVoutConfig[vout].height;

    return ME_OK;
}

AM_ERR CAmbaVideoSink::GetVideoPictureSize(AM_INT* width, AM_INT* height)
{
    *width = mDecParam.pic_width;
    *height = mDecParam.pic_height;
    AMLOG_INFO("GetVideoPictureSize, get picture width %d, height %d.\n", *width, *height);
    return ME_OK;
}

AM_ERR CAmbaVideoSink::GetCurrentVideoPictureSize(AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height)
{
    AM_INT vout;
    if(mpSharedRes->pbConfig.vout_config & (1<<eVoutHDMI)){
        vout = 1;
    } else {
        vout = 0;//no HDMI
    }
    *pos_x = mVoutConfig[vout].pos_x;
    *pos_y = mVoutConfig[vout].pos_y;
    *width = mVoutConfig[vout].video_x;
    *height = mVoutConfig[vout].video_y;
    AMLOG_INFO("GetCurrentVideoPictureSize, get current display picture width %d, height %d.\n", *width, *height);
    return ME_OK;
}

AM_ERR CAmbaVideoSink::VideoDisplayZoom(AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height)
{
    AMLOG_INFO("VideoDisplayZoom: pos_x %d, pos_y %d, x_size %d, y_size %d.\n", pos_x, pos_y, width, height);

    //check valid
    if ((pos_x + width) > (AM_INT)mDecParam.pic_width|| (pos_y + height) > (AM_INT)mDecParam.pic_height || (width < 0) || (height < 0) || (pos_x < 0) || (pos_y < 0)) {
        AMLOG_ERROR("VideoDisplayZoom: pos&size out of range.\n");
        return ME_BAD_PARAM;
    }

    AM_INT vout;
    if(mpSharedRes->pbConfig.vout_config & (1<<eVoutHDMI)){
        vout = 1;
    } else {
        vout = 0;//no HDMI
    }
    AM_INFO("mpSharedRes->pbConfig.vout_config = %d,vout = %d\n",mpSharedRes->pbConfig.vout_config,vout);

    AM_U32 zoom_factor_x, zoom_factor_y;
    zoom_factor_x = mVoutConfig[vout].zoom_factor_x;
    zoom_factor_y = mVoutConfig[vout].zoom_factor_y;

    float ratio_x,ratio_y;
    ratio_x = (float)mVoutConfig[vout].size_x/ width;
    ratio_y = (float)(mVoutConfig[vout].size_y >> mVoutConfig[vout].vout_mode)/ height;
    mVoutConfig[vout].zoom_factor_x = (int)(ratio_x * 0x10000);
    mVoutConfig[vout].zoom_factor_y = (int)(ratio_y * 0x10000);

    AM_INT ret;
    ret = configVout(vout, mVoutConfig[vout].rotate, mVoutConfig[vout].flip, pos_x, pos_y, mVoutConfig[vout].width, mVoutConfig[vout].height);
    if (ret) {
        AMLOG_ERROR("ambarella_vout_change_video_offset fail. ret %d, pos_x %d, pos_y %d .\n", ret, pos_x, pos_y);
        mVoutConfig[vout].zoom_factor_x = zoom_factor_x;
        mVoutConfig[vout].zoom_factor_y = zoom_factor_y;
        return ME_OS_ERROR;
    }
    mVoutConfig[vout].pos_x = pos_x;
    mVoutConfig[vout].pos_y = pos_y;
    mVoutConfig[vout].video_x = width;
    mVoutConfig[vout].video_y = height;

    AMLOG_INFO("VideoDisplayZoom done: vout %d, pos_x %d, pos_y %d, zoom_factor_x 0x%x, zoom_factor_y 0x%x.\n", vout, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].zoom_factor_x, mVoutConfig[vout].zoom_factor_y);

    return ME_OK;

}

AM_ERR CAmbaVideoSink::SetDisplayRotation(AM_INT vout, AM_INT degree)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayRotation: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }
    if (0 == mbVoutForPB[vout]) {
        AMLOG_ERROR("SetDisplayRotation: vout_id(%d) is not used by playback.\n", vout);
        return ME_BAD_PARAM;
    }

    AM_INT ret;
    if(degree == mVoutConfig[vout].rotate){
        AMLOG_INFO("same,need do nothing.\n");
        return ME_OK;
    }

    ret = configVout(vout, degree, mVoutConfig[vout].flip, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    if (!ret) {
        AMLOG_INFO("'SetDisplayRotation(%d) done, ori %d, target %d.\n", vout, mVoutConfig[vout].rotate, degree);
        mVoutConfig[vout].rotate = degree;
        return ME_OK;
    }

    AMLOG_ERROR("'SetDisplayRotation(%d) fail, ret %d, ori %d, target %d.\n", vout, ret, mVoutConfig[vout].rotate, degree);
    return ME_OS_ERROR;
}

AM_ERR CAmbaVideoSink::SetDisplayFlip(AM_INT vout, AM_INT flip)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("SetDisplayFlip: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }
    if (0 == mbVoutForPB[vout]) {
        AMLOG_ERROR("SetDisplayFlip: vout_id(%d) is not used by playback.\n", vout);
        return ME_BAD_PARAM;
    }

    AMLOG_INFO("'SetDisplayFlip(%d), flip %d, ori %d.\n", vout, flip, mVoutConfig[vout].flip);

    AM_INT ret;
    if(flip == mVoutConfig[vout].flip){
        AMLOG_INFO("need do nothing.\n");
        return ME_OK;
    }

    ret = configVout(vout, mVoutConfig[vout].rotate, flip, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);

    if (!ret) {
        AMLOG_INFO("'SetDisplayFlip(%d) done, ori %d, target %d.\n", vout, mVoutConfig[vout].flip, flip);
        mVoutConfig[vout].flip = flip;
        return ME_OK;
    }

    AMLOG_ERROR("'SetDisplayFlip(%d) fail, ret %d, ori %d, target %d.\n", vout, ret, mVoutConfig[vout].flip, flip);
    return ME_OS_ERROR;
}

AM_ERR CAmbaVideoSink::SetDisplayMirror(AM_INT vout, AM_INT mirror)
{
    return ME_NO_IMPL;
}

AM_ERR CAmbaVideoSink::EnableVout(AM_INT vout, AM_INT enable)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("EnableVout: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }
    if (0 == mbVoutForPB[vout]) {
        AMLOG_ERROR("EnableVout: vout_id(%d) is not used by playback.\n", vout);
        return ME_BAD_PARAM;
    }

    iav_vout_enable_video_t     iav_enable_video;

    iav_enable_video.vout_id = vout ? 1 : 0;
    iav_enable_video.video_en = enable ? 1 : 0;

    AM_INT ret =ioctl(mIavFd, IAV_IOC_VOUT_ENABLE_VIDEO, &iav_enable_video);
    if (ret) {
        AMLOG_ERROR("ambarella_vout_enable_video(%d, %d), fail ret = %d.\n", vout, enable, ret);
        return ME_OS_ERROR;
    }
    AM_INT origenable = mVoutConfig[vout].enable;
    mVoutConfig[vout].enable = enable ? 1 : 0;

    if(vout == 1){
        ret = configVout(vout, mVoutConfig[vout].rotate, mVoutConfig[vout].flip, mVoutConfig[vout].pos_x, mVoutConfig[vout].pos_y, mVoutConfig[vout].size_x, mVoutConfig[vout].size_y);
        if(ret){
            AMLOG_ERROR("disable/enable(%d) vout(%d), vout_setting failed\n", enable, vout);
            mVoutConfig[vout].enable = origenable;
            return ME_OS_ERROR;
        }
        AMLOG_INFO("disable/enable(%d) vout(%d), vout_setting successfully\n", enable, vout);
    }

    return ME_OK;
}

AM_ERR CAmbaVideoSink::EnableOSD(AM_INT vout, AM_INT enable)
{
    if (vout < 0 || vout >= eVoutCnt) {
        AMLOG_ERROR("EnableOSD: vout_id out of range.\n");
        return ME_BAD_PARAM;
    }

    if (0 == mbVoutForPB[vout]) {
        AMLOG_ERROR("EnableOSD: vout_id(%d) is not used by playback.\n", vout);
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

AM_ERR CAmbaVideoSink::EnableVoutAAR(AM_INT enable)
{
    return ME_NO_IMPL;
}

AM_ERR CAmbaVideoSink::SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h)
{
    AMLOG_INFO("'SetVideoSourceRect, x %d, y %d, w %d, h %d.\n", x, y, w, h);

    if ( x < 0 || y < 0 || w < 1 || h < 1 || ((AM_UINT)(x+w))>mDecParam.pic_width || ((AM_UINT)(y+h))>mDecParam.pic_height) {
        AMLOG_ERROR("SetVideoSourceRect: x y w h out of range, pic_width %d, pic_height %d.\n", mDecParam.pic_width, mDecParam.pic_height);
        return ME_BAD_PARAM;
    }

    mDisplayRectMap[0].rect[0].x = x;
    mDisplayRectMap[0].rect[0].y = y;
    mDisplayRectMap[0].rect[0].w = w;
    mDisplayRectMap[0].rect[0].h = h;

    return ME_OK;
}

AM_ERR CAmbaVideoSink::SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h)
{
    if (/*!mpFrameBufferPool || */vout_id < 0 || vout_id >= eVoutCnt) {
        AMLOG_ERROR("SetVideoDestRect: vout_id(%d) out of range.\n", vout_id);
        return ME_BAD_PARAM;
    }

    AMLOG_INFO("'SetVideoDestRect %d, x %d, y %d, w %d, h %d.\n", vout_id, x, y, w, h);

    if ( x < 0 || y < 0 || w < 1 || h < 1 || (x+w)>mVoutConfig[vout_id].width || (y+h)>mVoutConfig[vout_id].height) {
        AMLOG_ERROR("SetVideoDestRect: x y w h out of range, mVoutConfig[vout_id].width %d, mVoutConfig[vout_id].height %d.\n", mVoutConfig[vout_id].width, mVoutConfig[vout_id].height);
        return ME_BAD_PARAM;
    }

    mDisplayRectMap[vout_id].rect[1].x = x;
    mDisplayRectMap[vout_id].rect[1].y = y;
    mDisplayRectMap[vout_id].rect[1].w = w;
    mDisplayRectMap[vout_id].rect[1].h = h;

    return ME_OK;
}

AM_ERR CAmbaVideoSink::SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y)
{
    if (vout_id < 0 || vout_id >= eVoutCnt) {
        AMLOG_ERROR("SetVideoScaleMode: vout_id(%d) out of range.\n", vout_id);
        return ME_BAD_PARAM;
    }

    AMLOG_INFO("'SetVideoScaleMode %d, mode_x %d, mode_y %d.\n", vout_id, mode_x, mode_y);

    mDisplayRectMap[vout_id].factor_x = mode_x;
    mDisplayRectMap[vout_id].factor_y = mode_y;
    return ME_OK;
}

AM_ERR CAmbaVideoSink::GetCurrentTime(AM_U64& absoluteTimeMs, AM_U64& relativeTimeMs)
{
    if(!mbWaitFirstValidPTS) {
        absoluteTimeMs = mTimeOffset/90;//seek point
        relativeTimeMs = 0;
        AMLOG_PTS("CAmbaVideoSink::GetCurrentTime not started, %llu, %llu\n", absoluteTimeMs, relativeTimeMs);
        return ME_OK;
    }

    if (mCurrentPbPTS >= mBeginPbPTS) {
        relativeTimeMs = (mCurrentPbPTS - mBeginPbPTS)/90;
    } else {
        AM_ERROR("why comes here, buffer's PTS abnormal? mLastPTS %llu, mFirstPTS %llu.\n", mCurrentPbPTS, mBeginPbPTS);
        relativeTimeMs = mCurrentPbPTS/90;
    }

    absoluteTimeMs = relativeTimeMs + mTimeOffset/90;
    //AMLOG_PTS("mLastPTS %llu, mFirstPTS %llu, mTimeOffset %llu, diff %lld.\n", mLastPTS, mFirstPTS, mTimeOffset, mLastPTS - mFirstPTS);
    AMLOG_PTS("CAmbaVideoSink:GetCurrentTime absoluteTimeMs = %llu relativeTimeMs=%llu\n", absoluteTimeMs, relativeTimeMs);
    return ME_OK;
}

void CAmbaVideoSink::speedUp()
{
    AM_ASSERT(mpVideoInputPin);
    if (!mpVideoInputPin) {
        return;
    }

    AM_ASSERT(!mpBuffer);
    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    AMLOG_WARN("[Request speed up]: start purge input queue.\n");

    //purge input queue
    while (mpVideoInputPin->PeekBuffer(mpBuffer)) {
        if (CBuffer::DATA == mpBuffer->GetType()) {
            mpBuffer->Release();
            mpBuffer = NULL;
            AMLOG_WARN("[Request speed up]: discard input data.\n");
        } else if (CBuffer::EOS == mpBuffer->GetType()) {
            mpBuffer->Release();
            mpBuffer = NULL;
            PostEngineMsg(IEngine::MSG_EOS);
            AMLOG_WARN("[Request speed up]: get EOS buffer.\n");
            return;
        } else {
            AM_ERROR("BAD buffer type %d.\n", mpBuffer->GetType());
            mpBuffer->Release();
            mpBuffer = NULL;
        }
    }

    AMLOG_WARN("[Request speed up]: purge input queue done, restart DSP...\n");
    clearDecoder();
    AMLOG_WARN("[Request speed up]: DSP dec restart done.\n");

}

#ifdef AM_DEBUG
void CAmbaVideoSink::PrintState()
{
    AMLOG_INFO("CAmbaVideoSink: msState=%d, %d input data.\n", msState, mpVideoInputPin->mpBufferQ->GetDataCnt());
}
#endif

//-----------------------------------------------------------------------
//
// CAmbaVideoSinkInput
//
//-----------------------------------------------------------------------
CAmbaVideoSinkInput* CAmbaVideoSinkInput::Create(CFilter *pFilter)
{
    CAmbaVideoSinkInput *result = new CAmbaVideoSinkInput(pFilter);
    if (result && result->Construct()) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CAmbaVideoSinkInput::Construct()
{
    AM_ERR err = inherited::Construct(((CAmbaVideoSink*)mpFilter)->MsgQ());
    if (err != ME_OK)
        return err;
    return ME_OK;
}

CAmbaVideoSinkInput::~CAmbaVideoSinkInput()
{
}

AM_ERR CAmbaVideoSinkInput::CheckMediaFormat(CMediaFormat *pFormat)
{
    return ((CAmbaVideoSink*)mpFilter)->SetInputFormat(pFormat);
}


