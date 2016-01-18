
/*
 * video_encoder.cpp
 *
 * History:
 *    2011/7/8 - [Jay Zhang] create file
 *    2011/10/1- [ZQX] recreate file
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "video_encoder"
//#define AMDROID_DEBUG
//#define RECORD_TEST_FILE

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <basetypes.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#include "record_if.h"
#include "engine_guids.h"

extern "C" {
#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
}

#include "video_encoder_iav.h"

//debug related
//#define __dump_preview_data_before_start_encoding__
#ifdef __dump_preview_data_before_start_encoding__
static unsigned int _dump_index = 0;
#endif

IFilter* CreateVideoEncoderFilter(IEngine *pEngine, AM_UINT request_stream_number)
{
    return CVideoEncoder::Create(pEngine, request_stream_number);
}
//===============================================
//
//===============================================
IFilter* CVideoEncoder::Create(IEngine *pEngine, AM_UINT request_stream_number)
{
    CVideoEncoder *result = new CVideoEncoder(pEngine, request_stream_number);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

//===============================================
//vin related
//===============================================
#define CHECK_AMBA_VIDEO_FPS_LIST_SIZE  (256)

static inline enum amba_video_mode amba_video_mode_index2mode(AM_INT index)
{
    enum amba_video_mode    mode = AMBA_VIDEO_MODE_MAX;

    if (index < 0)
        goto amba_video_mode_index2mode_exit;


    if ((index >= 0) && (index < AMBA_VIDEO_MODE_MISC_NUM)) {
        mode = (enum amba_video_mode)index;
        goto amba_video_mode_index2mode_exit;
    }

    if ((index >= AMBA_VIDEO_MODE_MISC_NUM) && (index < (AMBA_VIDEO_MODE_MISC_NUM + AMBA_VIDEO_MODE_STILL_NUM))) {
        mode = (enum amba_video_mode)(AMBA_VIDEO_MODE_3M_4_3 + (index - AMBA_VIDEO_MODE_MISC_NUM));
        goto amba_video_mode_index2mode_exit;
    }

    if ((index >= (AMBA_VIDEO_MODE_MISC_NUM + AMBA_VIDEO_MODE_STILL_NUM)) && (index < AMBA_VIDEO_MODE_NUM)) {
        mode = (enum amba_video_mode)(AMBA_VIDEO_MODE_480I + (index - (AMBA_VIDEO_MODE_MISC_NUM + AMBA_VIDEO_MODE_STILL_NUM)));
        goto amba_video_mode_index2mode_exit;
    }

amba_video_mode_index2mode_exit:
    return mode;
}

static inline AM_UINT get_fps_list_count(struct amba_vin_source_mode_info *pinfo)
{
    AM_UINT	i;
    for (i = 0; i < pinfo->fps_table_size; i++) {
        if (pinfo->fps_table[i] == AMBA_VIDEO_FPS_AUTO)
            break;
    }
    return i;
}

static inline AM_INT set_vin_param(AM_INT mIavFd)
{
    AM_INT vin_eshutter_time = 60;  // 1/60 sec
    AM_INT vin_agc_db = 6;  // 0dB

    AM_UINT shutter_time_q9;
    AM_INT agc_db;

    shutter_time_q9 = 512000000/vin_eshutter_time;
    agc_db = vin_agc_db<<24;

    if (ioctl(mIavFd, IAV_IOC_VIN_SRC_SET_SHUTTER_TIME, shutter_time_q9) < 0) {
        perror("IAV_IOC_VIN_SRC_SET_SHUTTER_TIME");
        return -1;
    }

    if (ioctl(mIavFd, IAV_IOC_VIN_SRC_SET_AGC_DB, agc_db) < 0) {
        perror("IAV_IOC_VIN_SRC_SET_AGC_DB");
        return -1;
    }

    return 0;
}

static int change_fps_to_hz(AM_UINT fps_q9, AM_UINT *fps_hz, char *fps)
{
    const AM_UINT   delta = 10;
    AM_UINT     hz = 0;

    switch (fps_q9) {
        case AMBA_VIDEO_FPS_AUTO:
            snprintf(fps, 32, "%s", "AUTO");
            break;
        case AMBA_VIDEO_FPS_29_97:
            snprintf(fps, 32, "%s", "29.97");
            break;
        case AMBA_VIDEO_FPS_59_94:
            snprintf(fps, 32, "%s", "59.94");
            break;
        default:
            hz = (512000000 + delta) / fps_q9;  // to make 29.9999 to be 30fps
            snprintf(fps, 32, "%d", hz);
            break;
    }

    *fps_hz = hz;

    return 0;
}

static void _test_print_video_info( const struct amba_video_info *pvinfo, AM_UINT fps_list_size, AM_UINT *fps_list)
{
    char        format[32];
    char        fps[32];
    char        type[32];
    char        bits[32];
    char        ratio[32];
    char        system[32];
    AM_UINT     i;
    AM_UINT     fps_hz = 0;

    switch (pvinfo->format) {

        case AMBA_VIDEO_FORMAT_PROGRESSIVE:
            snprintf(format, 32, "%s", "P");
            break;
        case AMBA_VIDEO_FORMAT_INTERLACE:
            snprintf(format, 32, "%s", "I");
            break;
        case AMBA_VIDEO_FORMAT_AUTO:
            snprintf(format, 32, "%s", "Auto");
            break;
        default:
            snprintf(format, 32, "format?%d", pvinfo->format);
            break;
    }

    change_fps_to_hz(pvinfo->fps, &fps_hz, fps);

    switch (pvinfo->type) {
        case AMBA_VIDEO_TYPE_RGB_RAW:
            snprintf(type, 32, "%s", "RGB");
            break;
        case AMBA_VIDEO_TYPE_YUV_601:
            snprintf(type, 32, "%s", "YUV BT601");
            break;
        case AMBA_VIDEO_TYPE_YUV_656:
            snprintf(type, 32, "%s", "YUV BT656");
            break;
        case AMBA_VIDEO_TYPE_YUV_BT1120:
            snprintf(type, 32, "%s", "YUV BT1120");
            break;
        case AMBA_VIDEO_TYPE_RGB_601:
            snprintf(type, 32, "%s", "RGB BT601");
            break;
        case AMBA_VIDEO_TYPE_RGB_656:
            snprintf(type, 32, "%s", "RGB BT656");
            break;
        case AMBA_VIDEO_TYPE_RGB_BT1120:
            snprintf(type, 32, "%s", "RGB BT1120");
            break;
        default:
            snprintf(type, 32, "type?%d", pvinfo->type);
            break;
    }

    switch (pvinfo->bits) {
        case AMBA_VIDEO_BITS_AUTO:
            snprintf(bits, 32, "%s", "Bits Not Availiable");
            break;
        default:
            snprintf(bits, 32, "%dbits", pvinfo->bits);
            break;
    }

    switch (pvinfo->ratio) {
        case AMBA_VIDEO_RATIO_AUTO:
            snprintf(ratio, 32, "%s", "AUTO");
            break;
        case AMBA_VIDEO_RATIO_4_3:
            snprintf(ratio, 32, "%s", "4:3");
            break;
        case AMBA_VIDEO_RATIO_16_9:
            snprintf(ratio, 32, "%s", "16:9");
            break;
        default:
            snprintf(ratio, 32, "ratio?%d", pvinfo->ratio);
            break;
    }

    switch (pvinfo->system) {
        case AMBA_VIDEO_SYSTEM_AUTO:
            snprintf(system, 32, "%s", "AUTO");
            break;
        case AMBA_VIDEO_SYSTEM_NTSC:
            snprintf(system, 32, "%s", "NTSC");
            break;
        case AMBA_VIDEO_SYSTEM_PAL:
            snprintf(system, 32, "%s", "PAL");
            break;
        case AMBA_VIDEO_SYSTEM_SECAM:
            snprintf(system, 32, "%s", "SECAM");
            break;
        case AMBA_VIDEO_SYSTEM_ALL:
            snprintf(system, 32, "%s", "ALL");
            break;
        default:
            snprintf(system, 32, "system?%d", pvinfo->system);
            break;
    }

    AM_INFO("\t%dx%d%s\t%s\t%s\t%s\t%s\t%s\trev[%d]\n",
        pvinfo->width,
        pvinfo->height,
        format,
        fps,
        type,
        bits,
        ratio,
        system,
        pvinfo->rev
        );

    for (i = 0; i < fps_list_size; i++) {
        if (fps_list[i] == pvinfo->fps)
            continue;

        change_fps_to_hz(fps_list[i], &fps_hz, fps);
        AM_INFO("\t\t\t%s\n", fps);
    }
}
//-----------------------------------------------------------------------
//
// CVideoEncoderBufferPool
//
//-----------------------------------------------------------------------
void CVideoEncoderBufferPool::OnReleaseBuffer(CBuffer *pBuffer)
{
    //AM_WARNING("tYPE:%d, fd_id:%d, mFlags:%d\n", pBuffer->GetType(), pBuffer->mReserved, pBuffer->mFlags);
    if(!(pBuffer->mFlags & IAV_ENC_BUFFER))
    {
        return;
    }
    iav_frame_desc_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.enc_id = pBuffer->mEncId;
    frame.fd_id = pBuffer->mBufferId;
    //frame.fd_id = pBuffer->mReserved;
    if (ioctl(iavfd, IAV_IOC_RELEASE_ENCODED_FRAME, &frame) < 0) {
        //remove iav's check, ignore this msg now
        //AM_WARNING("!Just used an expired frame.\n");
    }
    //AM_WARNING("OnReleaseBuffer: enc_id %d, fd_id %d, pictype %d, mSeqNum %d.\n", frame.enc_id, frame.fd_id, pBuffer->mFrameType, pBuffer->mOriSeqNum);
    pBuffer->mFlags = 0;
}
//-----------------------------------------------------------------------
//
// CVideoEncoder
//
//-----------------------------------------------------------------------


AM_ERR CVideoEncoder::Construct()
{
    AM_UINT i=0;
    if ((mIavFd = open("/dev/iav", O_RDWR, 0)) < 0) {
        AM_PERROR("/dev/iav");
        return ME_ERROR;
    }
    AM_ASSERT(mIavFd > 0);

    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;
    DSetModuleLogConfig(LogModuleVideoEncoder);

    AMLOG_DEBUG("***CVideoEncoder::Construct number of pin %d.\n", mStreamNum);
    for(i=0; i < mStreamNum; i++)
    {
        mpOutputPin[i] = CVideoEncoderOutput::Create(this);
        if (mpOutputPin == NULL) {
            AM_ERROR("CVideoEncoderOutput::Create fail in CVideoEncoder::Construct().\n");
            return ME_NO_MEMORY;
        }
        mpOutputPin[i]->mpBSB = CVideoEncoderBufferPool::Create("EncBSB", 128, mIavFd);//it's better to sync this count with iav, related to bits-fifo's size
        if (mpOutputPin[i]->mpBSB == NULL) {
            AM_ERROR("CVideoEncoderBufferPool::Create fail in CVideoEncoder::Construct().\n");
            return ME_NO_MEMORY;
        }
        mpOutputPin[i]->SetBufferPool(mpOutputPin[i]->mpBSB);
    }

#ifdef RECORD_TEST_FILE
    mpVFile = CFileWriter::Create();
    err = mpVFile->CreateFile("/usr/local/record/info.info");
    //if (err != ME_OK)
        //return err;
#endif

    //memset(&mConfig, 0, sizeof(mConfig));
    //default settings
    //for (i=0; i<MAX_NUM_STREAMS; i++) {
    //    mConfig.stream_info[i].entropy_type = IAV_ENTROPY_CABAC;
    //}

    mpClockManager = (IClockManager *)mpEngine->QueryEngineInterface(IID_IClockManagerExt);
    if(!mpClockManager) {
        AM_ERROR("CVideoEncoder without mpClockManager?\n");
        return ME_ERROR;
    }

    return ME_OK;
}

CVideoEncoder::~CVideoEncoder()
{
    AMLOG_DESTRUCTOR("CVideoEncoder::~CVideoEncoder() start.\n");

    for(AM_UINT i = 0; i < mStreamNum; i++)
    {
        if (mpOutputPin[i]) {
            AM_RELEASE(mpOutputPin[i]->mpBSB);
        }
        AM_DELETE(mpOutputPin[i]);
    }
#ifdef RECORD_TEST_FILE
    mpVFile->CloseFile();
#endif

    AMLOG_DESTRUCTOR("CVideoEncoder::~CVideoEncoder() before clean iav staff, mIavFd %d.\n", mIavFd);

    if (mIavFd >= 0) {
        AM_INT ret;
        AMLOG_DESTRUCTOR("CVideoEncoder::~CVideoEncoder() before IAV_IOC_UNMAP_DSP.\n");
        if ((ret = ::ioctl(mIavFd, IAV_IOC_UNMAP_DSP, 0)) < 0) {
            AM_PERROR("IAV_IOC_UNMAP_DSP");
            AM_ERROR("IAV_IOC_UNMAP_DSP ERROR ret %d.\n", ret);
            return;
        }
        AMLOG_DESTRUCTOR("CVideoEncoder::~CVideoEncoder() before IAV_IOC_UNMAP_BSB.\n");
        if (mbMemMapped)
            ::ioctl(mIavFd, IAV_IOC_UNMAP_BSB, 0);

        AMLOG_DESTRUCTOR("CVideoEncoder::~CVideoEncoder() before close(fd %d).\n", mIavFd);
        close(mIavFd);
        mIavFd = -1;
    }

    AMLOG_DESTRUCTOR("CVideoEncoder::~CVideoEncoder() done.\n");
}

void CVideoEncoder::Delete()
{
    AMLOG_DESTRUCTOR("CVideoEncoder::Delete() before clean iav staff, mIavFd %d.\n", mIavFd);
    if (mIavFd >= 0) {
        AM_INT ret;
        AMLOG_DESTRUCTOR("CVideoEncoder::Delete() before IAV_IOC_UNMAP_DSP.\n");
        if ((ret = ::ioctl(mIavFd, IAV_IOC_UNMAP_DSP, 0)) < 0) {
            AM_PERROR("IAV_IOC_UNMAP_DSP");
            AM_ERROR("IAV_IOC_UNMAP_DSP ERROR ret %d.\n", ret);
            return;
        }
        AMLOG_DESTRUCTOR("CVideoEncoder::Delete() before IAV_IOC_UNMAP_BSB.\n");
        if (mbMemMapped)
            ::ioctl(mIavFd, IAV_IOC_UNMAP_BSB, 0);

        AMLOG_DESTRUCTOR("CVideoEncoder::Delete() before close(fd %d).\n", mIavFd);
        close(mIavFd);
        mIavFd = -1;
    }
    AMLOG_DESTRUCTOR("CVideoEncoder::Delete() done.\n");
    return inherited::Delete();
}

AM_ERR CVideoEncoder::FlowControl(FlowControlType type)
{
    CMD cmd0;

    cmd0.code = CMD_FLOW_CONTROL;
    cmd0.flag = (AM_U8)type;

    mpWorkQ->MsgQ()->PostMsg((void*)&cmd0, sizeof(cmd0));
    return ME_OK;
}

void CVideoEncoder::DoFlowControl(FlowControlType type)
{
    AM_UINT i;
    SListNode* pNode;

    for (i=0; i<mStreamNum; i++) {
        AM_ASSERT(mpOutputPin[i]);
        if (mpOutputPin[i]) {
            pNode = mpOutputPin[i]->mpFlowControlList->alloc();
            pNode->context = type;
            mpOutputPin[i]->mpFlowControlList->append_2end(pNode);
        }
    }
}

void CVideoEncoder::GetInfo(INFO& info)
{
    info.nInput = 0;
    info.nOutput = mStreamNum;
    info.pName = "AmbaVEnc";
}

IPin* CVideoEncoder::GetOutputPin(AM_UINT index)
{
    AMLOG_DEBUG("CVideoEncoder::GetOutputPin index %d, mStreamNum %d, %p.\n", index, mStreamNum, mpOutputPin[index]);
    if (index >= mStreamNum)
        return NULL;
    return mpOutputPin[index];
}

AM_ERR CVideoEncoder::SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index)
{
    AM_ASSERT(index < mStreamNum);
    if (index >= mStreamNum) {
        AMLOG_ERROR("WRONG index (%d), exceed max number(%d) in CVideoEncoder::SetParameters.\n", index, mStreamNum);
        return ME_BAD_PARAM;
    }

    if (!param) {
        AMLOG_ERROR("NULL param, in CVideoEncoder::SetParameters.\n");
        return ME_BAD_PARAM;
    }

    AM_ASSERT(type == IParameters::StreamType_Video);
    AM_ASSERT(format == IParameters::StreamFormat_H264);

    if (format == IParameters::StreamFormat_H264) {
        mConfig.stream_info[index].stream_format = IAV_ENCODE_H264;
    }
    mConfig.stream_info[index].width = param->video.pic_width;
    mConfig.stream_info[index].height = param->video.pic_height;

    mConfig.stream_info[index].M = param->video.M;
    mConfig.stream_info[index].N = param->video.N;
    mConfig.stream_info[index].IDRInterval = param->video.IDRInterval;
    AMLOG_INFO(" *****entropy_type %d.\n", mConfig.stream_info[index].entropy_type);
    if (param->video.entropy_type == IParameters::EntropyType_H264_CABAC) {
        mConfig.stream_info[index].entropy_type = IAV_ENTROPY_CABAC;
    } else if (param->video.entropy_type == IParameters::EntropyType_H264_CAVLC) {
        mConfig.stream_info[index].entropy_type = IAV_ENTROPY_CAVLC;
    } else {
        //default:
        AM_ASSERT(param->video.entropy_type == IParameters::EntropyType_NOTSet);
        mConfig.stream_info[index].entropy_type = IAV_ENTROPY_CAVLC;
    }

    mConfig.stream_info[index].average_bitrate = param->video.bitrate;
    //round frame rate
    mConfig.stream_info[index].framerate = (param->video.framerate_num + param->video.framerate_den/2) / param->video.framerate_den;

    AMLOG_INFO("CVideoEncoder::SetParameters pic_width %d, height %d, index %d.\n", mConfig.stream_info[index].width, mConfig.stream_info[index].height, index);
    AMLOG_INFO("  M %d, N %d, IDR interval %d, entropy_type %d.\n", mConfig.stream_info[index].M, mConfig.stream_info[index].N, mConfig.stream_info[index].IDRInterval, mConfig.stream_info[index].entropy_type);

    return ME_OK;
}

bool CVideoEncoder::isVideoSettingMatched(iav_enc_info_t& enc_info)
{
    bool matched = true;
    //check main stream
    if ((enc_info.main_img.width != (u16)mConfig.stream_info[0].width) || (enc_info.main_img.height != (u16)mConfig.stream_info[0].height)) {
        matched = false;
    }

    //not check second is extractly matched
    if ((enc_info.second_img.width != (u16)mConfig.stream_info[1].width) || (enc_info.second_img.height != (u16)mConfig.stream_info[1].height)) {
        matched = false;
    }

    AMLOG_INFO("isVideoSettingMatched, current video encoding settings: %dx%d,%dx%d.\n", enc_info.main_img.width, enc_info.main_img.height, enc_info.second_img.width, enc_info.second_img.height);
    AMLOG_INFO("isVideoSettingMatched, request video encoding settings: %dx%d,%dx%d.\n", (u16)mConfig.stream_info[0].width, (u16)mConfig.stream_info[0].height, (u16)mConfig.stream_info[1].width, (u16)mConfig.stream_info[1].height);

    if (true == matched) {
        AMLOG_INFO("isVideoSettingMatched matched.\n");
        return true;
    }

    AMLOG_WARN("isVideoSettingMatched not matched, main %dx%d --> %dx%d.\n", enc_info.main_img.width, enc_info.main_img.height, (u16)mConfig.stream_info[0].width, (u16)mConfig.stream_info[0].height);
    AMLOG_WARN("isVideoSettingMatched not matched, second %dx%d --> %dx%d.\n", enc_info.second_img.width, enc_info.second_img.height, (u16)mConfig.stream_info[1].width, (u16)mConfig.stream_info[1].height);

    enc_info.main_img.width = (u16)mConfig.stream_info[0].width;
    enc_info.main_img.height = (u16)mConfig.stream_info[0].height;
    enc_info.second_img.width = (u16)mConfig.stream_info[1].width;
    enc_info.second_img.height = (u16)mConfig.stream_info[1].height;

    if (!enc_info.second_img.width || !enc_info.second_img.height) {
        AM_ERROR("BAD preview C size, %d, %d.\n", mConfig.stream_info[1].width, mConfig.stream_info[1].height);
        enc_info.second_img.width = DefaultPreviewCWidth;
        enc_info.second_img.height = DefaultPreviewCHeight;
    }

    return false;
}

//===============================================
//
//===============================================
AM_ERR CVideoEncoder::SetConfig()
{
    iav_state_info_t info;
    iav_enc_mode_config_t mode;
    iav_enc_info_t enc_info;
    AM_UINT tot_bitrate = 0;

    memset(&info, 0, sizeof(iav_state_info_t));
    memset(&enc_info, 0, sizeof(iav_enc_info_t));
    if (::ioctl(mIavFd, IAV_IOC_GET_STATE_INFO, &info) < 0) {
        AM_PERROR("IAV_IOC_GET_STATE_INFO\n");
        AM_ERROR("Fatal error: IAV_IOC_GET_STATE_INFO fail.\n");
        return ME_ERROR;
    }

    if(info.state == IAV_STATE_ENCODING) {
        //need do nothing about enter idle, initvin, enter preview/encoding mode
        AMLOG_INFO("***Init state is IAV_STATE_ENCODING, not init vin, and enter encoding mode.\n");

        //check if need do IAV_IOC_INIT_ENCODE again, main stream/sub stream resolution, frequency
        if (::ioctl(mIavFd, IAV_IOC_INIT_ENCODE_INFO, &enc_info) < 0) {
            AMLOG_ERROR("IAV_IOC_INIT_ENCODE fail.\n");
            AMLOG_ERROR("IAV_IOC_INIT_ENCODE\n");
            return ME_ERROR;
        }

        if (true == isVideoSettingMatched(enc_info)) {
            //do nothing
        } else {
            AMLOG_INFO("%dx%d,%dx%d", enc_info.main_img.width, enc_info.main_img.height, enc_info.second_img.width, enc_info.second_img.height);

            /*use no_preview_on_vout based on lcd's osd. no used since may cause extra bugs.
            amba_vout_sink_info vout_info;
            memset(&vout_info, 0, sizeof(amba_vout_sink_info));
            //LCD
            vout_info.id = 0;
            if (::ioctl(mIavFd, IAV_IOC_VOUT_GET_SINK_INFO, &vout_info) < 0) {
                AM_ERROR("IAV_IOC_VOUT_GET_SINK_INFO");
                AM_ERROR("**IAV_IOC_VOUT_GET_SINK_INFO fail!\n");
            }

            if(vout_info.sink_mode.fb_id == -1)
            {
                AM_WARNING("no_preview_on_vout!!!!");
                enc_info.no_preview_on_vout = 1;
            }
            */
            if (::ioctl(mIavFd, IAV_IOC_INIT_ENCODE, &enc_info) < 0) {
                AMLOG_ERROR("IAV_IOC_INIT_ENCODE fail.\n");
                AMLOG_ERROR("IAV_IOC_INIT_ENCODE\n");
                return ME_ERROR;
            }
            AMLOG_INFO("IAV_IOC_INIT_ENCODE done.\n");
        }
    } else {
        AMLOG_INFO("***Init state is NOT IAV_STATE_ENCODING, do something in VideoEncoder Filter.\n");
        if (info.state != IAV_STATE_IDLE) {
            AM_ERROR("Wrong iav state %d, enter idle first.\n", info.state);
            if ((::ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0)) < 0) {
                AMLOG_ERROR("UDEC enter IDLE mode fail.\n");
                return ME_OS_ERROR;
            }
            AM_WARNING("enter IDLE mode done.\n");
        }

        //check if is iav idle
        if (::ioctl(mIavFd, IAV_IOC_GET_STATE_INFO, &info) < 0) {
            AM_PERROR("IAV_IOC_GET_STATE_INFO\n");
            AM_ERROR("Fatal error: IAV_IOC_GET_STATE_INFO fail.\n");
            return ME_ERROR;
        }

        if (info.state != IAV_STATE_IDLE) {
            AM_ERROR("Cannot enter idle, (iav state %d).\n", info.state);
            return ME_ERROR;
        }

        AMLOG_INFO("Init Vin in VideoEncoder filter.\n");
        //init vin first
        InitVin(AMBA_VIDEO_MODE_AUTO);

        AMLOG_INFO("IAV_IOC_ENTER_ENCODE_MODE start.\n");
        mode.keep_mode = 0;
        if (::ioctl(mIavFd, IAV_IOC_ENTER_ENCODE_MODE, &mode) < 0) {
            AMLOG_ERROR("IAV_IOC_ENTER_ENCODE_MODE fail.\n");
            AMLOG_ERROR("IAV_IOC_ENTER_ENCODE_MODE\n");
            return ME_ERROR;
        }
        AMLOG_INFO("IAV_IOC_ENTER_ENCODE_MODE done.\n");

        memset(&enc_info, 0, sizeof(iav_enc_info_t));
        enc_info.main_img.width = (u16)mConfig.stream_info[0].width;
        enc_info.main_img.height = (u16)mConfig.stream_info[0].height;
        enc_info.second_img.width = (u16)mConfig.stream_info[1].width;
        enc_info.second_img.height = (u16)mConfig.stream_info[1].height;

        AM_ASSERT(enc_info.second_img.width);
        AM_ASSERT(enc_info.second_img.height);

        if (!enc_info.second_img.width || !enc_info.second_img.height) {
            AM_ERROR("BAD preview C size, %d, %d.\n", mConfig.stream_info[1].width, mConfig.stream_info[1].height);
            enc_info.second_img.width = DefaultPreviewCWidth;
            enc_info.second_img.height = DefaultPreviewCHeight;
        }

        AMLOG_INFO("%dx%d,%dx%d", enc_info.main_img.width, enc_info.main_img.height, enc_info.second_img.width, enc_info.second_img.height);
        //enc_info.no_preview_on_vout = 1;
        if (::ioctl(mIavFd, IAV_IOC_INIT_ENCODE, &enc_info) < 0) {
            AMLOG_ERROR("IAV_IOC_INIT_ENCODE fail.\n");
            AMLOG_ERROR("IAV_IOC_INIT_ENCODE\n");
            return ME_ERROR;
        }
        AMLOG_INFO("IAV_IOC_INIT_ENCODE done.\n");

    }

    //testPrintConfig(&mConfig.stream_info[0]);
    mEncConfig[0].flags = 0;
    mEncConfig[0].enc_id = 0;
    mEncConfig[0].encode_type = IAV_ENCODE_H264;
    mEncConfig[0].u.h264.M = mConfig.stream_info[0].M;
    mEncConfig[0].u.h264.N = mConfig.stream_info[0].N;
    mEncConfig[0].u.h264.idr_interval = mConfig.stream_info[0].IDRInterval;
    mEncConfig[0].u.h264.gop_model = mConfig.stream_info[0].gop_model;
    mEncConfig[0].u.h264.bitrate_control = mConfig.stream_info[0].bitrate_control;
    mEncConfig[0].u.h264.calibration = mConfig.stream_info[0].calibration;
    mEncConfig[0].u.h264.vbr_ness = mConfig.stream_info[0].vbr_ness;
    mEncConfig[0].u.h264.min_vbr_rate_factor = mConfig.stream_info[0].min_vbr_rate_factor;
    mEncConfig[0].u.h264.max_vbr_rate_factor = mConfig.stream_info[0].max_vbr_rate_factor;

    mEncConfig[0].u.h264.average_bitrate = mConfig.stream_info[0].average_bitrate;
    mEncConfig[0].u.h264.entropy_codec = mConfig.stream_info[0].entropy_type;

    PrintH264Config(&mEncConfig[0]);

    AMLOG_INFO("===============================\n");
    //testPrintConfig(&mConfig.stream_info[1]);
    mEncConfig[1].flags = 0;
    mEncConfig[1].enc_id = 1;
    mEncConfig[1].encode_type = IAV_ENCODE_H264;
    mEncConfig[1].u.h264.M = mConfig.stream_info[1].M;
    mEncConfig[1].u.h264.N = mConfig.stream_info[1].N;
    mEncConfig[1].u.h264.idr_interval = mConfig.stream_info[1].IDRInterval;
    mEncConfig[1].u.h264.gop_model = mConfig.stream_info[1].gop_model;
    mEncConfig[1].u.h264.bitrate_control = mConfig.stream_info[1].bitrate_control;
    mEncConfig[1].u.h264.calibration = mConfig.stream_info[1].calibration;
    mEncConfig[1].u.h264.vbr_ness = mConfig.stream_info[1].vbr_ness;
    mEncConfig[1].u.h264.min_vbr_rate_factor = mConfig.stream_info[1].min_vbr_rate_factor;
    mEncConfig[1].u.h264.max_vbr_rate_factor = mConfig.stream_info[1].max_vbr_rate_factor;

    mEncConfig[1].u.h264.average_bitrate = mConfig.stream_info[1].average_bitrate;
    mEncConfig[1].u.h264.entropy_codec = mConfig.stream_info[1].entropy_type;

    PrintH264Config(&mEncConfig[1]);

    tot_bitrate += mEncConfig[0].u.h264.average_bitrate;
    if (::ioctl(mIavFd, IAV_IOC_ENCODE_SETUP, &mEncConfig[0]) < 0) {
        AM_PERROR("IAV_IOC_ENCODE_SETUP\n");
        AM_ERROR("IAV_IOC_ENCODE_SETUP\n");
        return ME_ERROR;
    }
    AMLOG_INFO("IAV_IOC_ENCODE_SETUP main done, bit rate %u.\n", mEncConfig[0].u.h264.average_bitrate);
    mStreamsMask |= IAV_MAIN_STREAM;

    if(mStreamNum > 1)
    {
        tot_bitrate += mEncConfig[1].u.h264.average_bitrate;
        if (::ioctl(mIavFd, IAV_IOC_ENCODE_SETUP, &mEncConfig[1]) < 0) {
            AM_PERROR("IAV_IOC_ENCODE_SETUP\n");
            AM_ERROR("IAV_IOC_ENCODE_SETUP\n");
            return ME_ERROR;
        }
        AMLOG_INFO("IAV_IOC_ENCODE_SETUP second done, bit rate %u.\n", mEncConfig[1].u.h264.average_bitrate);
        mStreamsMask |= IAV_2ND_STREAM;
    }

#ifdef __dump_preview_data_before_start_encoding__
    iav_img_buf_t img_buf;
    memset(&img_buf, 0, sizeof(img_buf));
    img_buf.buffer_id= IAV_BUFFER_ID_PREVIEW_C;

    if (ioctl(mIavFd, IAV_IOC_GET_PREVIEW_BUFFER, &img_buf) < 0) {
        AM_ERROR("IAV_IOC_GET_PREVIEW_BUFFER\n");
        perror("IAV_IOC_GET_PREVIEW_BUFFER");
        return ME_OS_ERROR;
    }

    char Dumpfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};
    snprintf(Dumpfilename, sizeof(Dumpfilename), "%s/win/raw/previewc_y", AM_GetPath(AM_PathDump));
    AM_DumpBinaryFile_withIndex(Dumpfilename, _dump_index, (AM_U8*)img_buf.luma_addr, img_buf.buf_pitch*img_buf.buf_height);
    snprintf(Dumpfilename, sizeof(Dumpfilename), "%s/win/raw/previewc_uv", AM_GetPath(AM_PathDump));
    AM_DumpBinaryFile_withIndex(Dumpfilename, _dump_index, (AM_U8*)img_buf.chroma_addr, (img_buf.buf_pitch*img_buf.buf_height)/2);
    _dump_index++;
#endif

    if (::ioctl(mIavFd, IAV_IOC_GET_STATE_INFO, &info) < 0) {
        AM_PERROR("IAV_IOC_GET_STATE_INFO\n");
        AM_ERROR("Fatal error: IAV_IOC_GET_STATE_INFO fail.\n");
        return ME_ERROR;
    }
    AMLOG_INFO("IAV_IOC_START_ENCODE state = %d  %d  %d\n", info.state, info.dsp_encode_state, info.dsp_encode_mode);
    AM_ASSERT(info.state == IAV_STATE_ENCODING);

    AMLOG_INFO("before IAV_IOC_START_ENCODE, mStreamsMask 0x%x, mStreamNum %d.\n", mStreamsMask, mStreamNum);
    // start encoding
    if (::ioctl(mIavFd, IAV_IOC_START_ENCODE, mEncConfig[0].enc_id) < 0) {
        AM_PERROR("IAV_IOC_START_ENCODE\n");
        AM_ERROR("IAV_IOC_START_ENCODE fail, mStreamsMask 0x%x.\n", mStreamsMask);
        return ME_ERROR;
    }
    if(mStreamNum > 1)
    {
        if (::ioctl(mIavFd, IAV_IOC_START_ENCODE, mEncConfig[1].enc_id) < 0) {
        AM_PERROR("IAV_IOC_START_ENCODE\n");
        AM_ERROR("IAV_IOC_START_ENCODE fail, mStreamsMask 0x%x.\n", mStreamsMask);
        return ME_ERROR;
        }
    }

    //estimate expire time
    AM_ASSERT(tot_bitrate);
    //hard code here, BSB is 12M now
    //use 12M *1/2, similar with driver, driver's dual stream 's descriptor number is same with single stream, so choose 1/2
    mFrameLifeTime = (((AM_U64)IParameters::TimeUnitDen_90khz)*(12*1024*1024)*8)/(tot_bitrate);
    mFrameLifeTime /= 2;

    AM_ASSERT(mFrameLifeTime > (IParameters::TimeUnitDen_90khz));//greater than 1 second
    AMLOG_INFO("IAV_IOC_START_ENCODE done, mFrameLifeTime %u, tot_bitrate %u.\n", mFrameLifeTime, tot_bitrate);

    return ME_OK;
}

void CVideoEncoder::DoStop()
{
    AM_INFO("^^^^CVideoEncoder DoStop()!\n");
    for(AM_UINT i = 0; i < mStreamNum; i++)
    {
        AM_ASSERT(mpOutputPin[i]);
        if (mpOutputPin[i]) {
            AM_INFO("^^^^Stream (%d) Statistic:\n", i);
            AM_INFO("^^^^TotalDrop Frame=>%d\n", mpOutputPin[i]->mTotalDrop);
            AM_INFO("^^^^Total IDR Frame Drop=>%d\n", mpOutputPin[i]->mIDRDrop);
            AM_INFO("^^^^============================^^^^\n");
        }
    }

    mbStop = true;
    AMLOG_INFO("before IAV_IOC_STOP_ENCODE, mStreamsMask 0x%x, mStreamNum %d.\n", mStreamsMask, mStreamNum);
    ioctl(mIavFd, IAV_IOC_STOP_ENCODE, mEncConfig[0].enc_id);
    if(mStreamNum > 1)
        ioctl(mIavFd, IAV_IOC_STOP_ENCODE, mEncConfig[1].enc_id);
    AMLOG_INFO("IAV_IOC_STOP_ENCODE done.\n");

/*
    if((::ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0)) < 0)
    {
        AMLOG_ERROR("UDEC enter IDLE mode fail.\n");
    }else{
        AMLOG_INFO("enter IDLE mode done.\n");
    }
    */
}

AM_ERR CVideoEncoder::ExitPreview()
{
    AMLOG_WARN("CVideoEncoder::ExitPreview() start, mIavFd %d, mbStop %d, mbRun %d.\n", mIavFd, mbStop, mbRun);

    if((::ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0)) < 0)
    {
        AMLOG_ERROR("UDEC enter IDLE mode fail.\n");
    }else{
        AMLOG_INFO("enter IDLE mode done.\n");
    }
    AMLOG_WARN("CVideoEncoder::ExitPreview() done.\n");
    return ME_OK;
}

bool CVideoEncoder::ProcessCmd(CMD& cmd)
{
    //AM_ERR err = ME_OK;
    AMLOG_CMD("process cmd %d.\n", cmd.code);
    switch(cmd.code)
    {
    case CMD_STOP:
        DoStop();
        if (msState != STATE_PENDING) {
            AMLOG_WARN("msState != STATE_PENDING, engine not send EOS first, send EOS here.\n");
            ProcessEOS();
        }
        mbRun = false;
        CmdAck(ME_OK);
        break;

    case CMD_RUN:
        //re-run
        AMLOG_INFO("CVideoEncoder re-Run, state %d.\n", msState);
        AM_ASSERT(STATE_PENDING == msState || STATE_ERROR == msState);
        msState = STATE_IDLE;
        CmdAck(ME_OK);
        break;

    case CMD_PAUSE:
        if(msState != STATE_PENDING)
        {
            for(AM_UINT i = 0; i < mStreamNum; i++){
                AM_ASSERT(mpOutputPin[i]);
                mpOutputPin[i]->mbSkip = false;
            }
            msState = STATE_PENDING;
        }
        break;

    case CMD_RESUME:
        if(msState == STATE_PENDING)
        {
            for(AM_UINT i = 0; i < mStreamNum; i++){
                AM_ASSERT(mpOutputPin[i]);
                mpOutputPin[i]->mbSkip = true;
            }
            msState = STATE_IDLE;
        }
        break;

    case CMD_OBUFNOTIFY:
        break;

    case CMD_FLOW_CONTROL:
        //EOS is special
        if ((FlowControlType)cmd.flag == FlowControl_eos) {
            //stop to and then read all buffers remainning in driver, cannot send eos before read all buffers out
            AM_ASSERT(false == mbStop);
            DoStop();
            //ProcessEOS();
            //msState = STATE_PENDING;
            break;
        }
        DoFlowControl((FlowControlType)cmd.flag);
        break;

    default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return 1;
}

AM_ERR CVideoEncoder::setupVideoEncoder()
{
    iav_mmap_info_t mmap;
    AM_INT ret;
    if ((ret = ::ioctl(mIavFd, IAV_IOC_MAP_DSP, &mmap)) < 0) {
        AM_PERROR("IAV_IOC_MAP_DSP");
        AM_ERROR("IAV_IOC_MAP_DSP ERROR ret %d.\n", ret);
        return ME_BAD_STATE;
    }
    AMLOG_INFO("Map_Dsp dsp_mem: 0x%p, size = 0x%x\n", mmap.addr, mmap.length);
    mbMemMapped = true;

    if(SetConfig() != ME_OK)
    {
        AM_PERROR("SetConfig Failed!\n");
        return ME_ERROR;
    }

    return ME_OK;
}

void CVideoEncoder::PrintState()
{
    AMLOG_WARN("VE: msState=%d.\n", msState);
    AM_UINT iindex = 0;
    for (iindex=0; iindex < mStreamNum; iindex ++) {
        AM_ASSERT(mpOutputPin[iindex]);
        if (mpOutputPin[iindex]) {
            AM_ASSERT(mpOutputPin[iindex]->mpBSB);
            AMLOG_WARN(" outputpin[%d], have free data cnt %d.\n", iindex, mpOutputPin[iindex]->mpBSB->GetFreeBufferCnt());
        }
    }
}

void CVideoEncoder::OnRun()
{
    CmdAck(ME_OK);
    CQueue::QType type;
    CQueue::WaitResult result;
    CMD cmd;
    AM_ERR err;

#ifdef __print_time_info__
    struct timeval tv_loop_last, tv_loop_current;
#endif

    err = setupVideoEncoder();
    if (err != ME_OK) {
        AM_ASSERT(0);
        AM_ERROR("setupVideoEncoder fail.\n");
        msState = STATE_ERROR;
    }

    for(AM_UINT i = 0; i < mStreamNum; i++)
        AM_ASSERT(mpOutputPin[i]->mpBSB == mpOutputPin[i]->mpBufferPool);
    mbRun = true;

    if (mpOutputPin[0] && mpOutputPin[0]->mpBSB) {
        mnReservedBufferNum = mpOutputPin[0]->mpBSB->GetFreeBufferCnt()/3;
        AMLOG_INFO(" [VE]mnReservedBufferNum %d.\n", mnReservedBufferNum);
    }

    while(mbRun)
    {
        AMLOG_STATE("CVideoEncoder::OnRun, state %d.\n", msState);

#ifdef __print_time_info__
        AMLOG_PRINTF("CVideoEncoder::OnRun, state %d.\n", msState);
        if (mpOutputPin[0] && mpOutputPin[0]->mpBSB) {
            AMLOG_PRINTF(" 0's buffer cnt %d.\n", mpOutputPin[0]->mpBSB->GetFreeBufferCnt());
        }
        if (mpOutputPin[1] && mpOutputPin[1]->mpBSB) {
            AMLOG_PRINTF(" 1's buffer cnt %d.\n", mpOutputPin[1]->mpBSB->GetFreeBufferCnt());
        }
        gettimeofday(&tv_loop_current,NULL);
        if (tv_loop_current.tv_usec >= tv_loop_last.tv_usec) {
            AMLOG_PRINTF("[VE] time gap from last loop %d sec, %d usec.\n", tv_loop_current.tv_sec - tv_loop_last.tv_sec, tv_loop_current.tv_usec - tv_loop_last.tv_usec);
        } else {
            AMLOG_PRINTF("[VE] time gap from last loop %d sec, %d usec.\n", tv_loop_current.tv_sec - tv_loop_last.tv_sec - 1, tv_loop_current.tv_usec + 1000000 - tv_loop_last.tv_usec);
        }
        tv_loop_last = tv_loop_current;
#endif

        switch (msState)
        {
        case STATE_IDLE:
            while (mpWorkQ->PeekCmd(cmd)) {
                ProcessCmd(cmd);
            }

            if (mbRun == false || msState != STATE_IDLE) {
                break;
            }

            //wait, release CPU to muxer
            while (mbRun && mpOutputPin[0] && mpOutputPin[0]->mpBSB->GetFreeBufferCnt() < mnReservedBufferNum) {
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
            }
            while (mbRun && mpOutputPin[1] && mpOutputPin[1]->mpBSB->GetFreeBufferCnt() < mnReservedBufferNum) {
                    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                ProcessCmd(cmd);
            }

            if (mbRun == false || msState != STATE_IDLE) {
                break;
            }

            //read data from iav driver
            {
                err = ReadInputData();
                if(err == ME_OK)
                {
                    msState = STATE_WAIT_OUTPUT;
                }else if(err == ME_BUSY){
                    AMLOG_WARN("read input busy?\n");
                    break;
                }else if(err == ME_CLOSED){
                    AMLOG_INFO("[VE] all buffers comes out, send EOS.\n");
                    ProcessEOS();
                    msState = STATE_PENDING;
                    mbRun = false;
                }else if (err == ME_OS_ERROR) {
                    AM_ERROR("IAV Error, goto Error state.\n");
                    PostEngineMsg(IEngine::MSG_OS_ERROR);
                    msState = STATE_ERROR;
                } else {
                    AM_ERROR("Not supposed return value, %d.\n", err);
                }
            }
            break;

        case STATE_WAIT_OUTPUT:
            //AM_ASSERT(mpBuffer);
            //if(mpBSB->GetFreeBufferCnt() < mFifoInfo.count)
            err = CheckBufferPool();
            NotifyFlowControl();
            DropFrame();
            if(err == ME_ERROR)
            {
                //Drop All
                AMLOG_ERROR("ALL blocked.\n");
                //need release to iav
                if (ioctl(mIavFd, IAV_IOC_RELEASE_ENCODED_FRAME, &mFrame) < 0) {
                    AMLOG_WARN("IAV_IOC_RELEASE_ENCODED_FRAME error.\n");
                }
                msState = STATE_IDLE;
            }else if(err == ME_OK){
                msState = STATE_READY;
            }
            break;

        case STATE_READY:
            if(ProcessBuffer() == ME_OK)
            {
                msState = STATE_IDLE;
            }else{
                msState = STATE_ERROR;
            }
            break;

        case STATE_PENDING:
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            //EOS
            if (cmd.code == CMD_FLOW_CONTROL && ((FlowControlType)cmd.flag == FlowControl_eos)) {
                ProcessEOS();
                msState = STATE_PENDING;
            } else {
                ProcessCmd(cmd);
            }
            break;

        case STATE_ERROR:
            //discard all data, wait eos
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            if (cmd.code == CMD_STOP) {
                ProcessEOS();
                mbRun = false;
                CmdAck(ME_OK);
            } else if (cmd.code == CMD_FLOW_CONTROL) {
                if ((FlowControlType)cmd.flag == FlowControl_eos) {
                    ProcessEOS();
                    msState = STATE_PENDING;
                } else {
                    ProcessCmd(cmd);
                }
            } else {
                //ignore other cmds
                AMLOG_WARN("how to handle this type of cmd %d, in Video Encoder error state.\n", cmd.code);
            }
            break;

        default:
            AM_ERROR("Not Support State Occur %d .\n", (AM_UINT)msState);
            break;
        }
    }

    AMLOG_INFO("CVideoEncoder::OnRun end.\n");
}

AM_ERR CVideoEncoder::ReadInputData()
{
    AMLOG_STATE("IAV_IOC_GET_ENCODED_FRAME!!!!!!!!!!!\n");
    AM_UINT i = 0;
    AM_INT ret = 0;
    memset(&mFrame, 0 , sizeof(iav_frame_desc_t));
    mFrame.enc_id = IAV_ENC_ID_MAIN;
    if(mStreamNum > 1)
        mFrame.enc_id = IAV_ENC_ID_ALL;

    //AMLOG_INFO("IAV_IOC_GET_ENCODED_FRAME!!!!!!!!!!!\n");
    ret = ::ioctl(mIavFd, IAV_IOC_GET_ENCODED_FRAME, &mFrame);

    if (mpClockManager) {
        mLastFrameExpiredTime = mpClockManager->GetCurrentTime() + mFrameLifeTime;
    }

    if (ret < 0) {
        AM_ERROR("IAV_IOC_READ_BITSTREAM error, ret %d.\n", ret);
        AM_PERROR("IAV_IOC_READ_BITSTREAM");
        if(mbStop == true) {
            AMLOG_INFO("CLOSED\n");
            return ME_CLOSED;
        } else {
            AMLOG_ERROR("ME_OS_ERROR\n");
            return ME_OS_ERROR;
        }
    }

    //AMLOG_WARN("GetVideoBuffer enc_id %d, fd_id %d, pictype %d, mSeqNum %d, pts %llu\n", mFrame.enc_id, mFrame.fd_id, mFrame.pic_type, mFrame.frame_num, mFrame.pts_64);

    if(mFrame.is_eos)
    {
        mEos |= (1 << mFrame.enc_id);
        if(mEos == mStreamsMask)
        {
            return ME_CLOSED;
        }else{
            return ME_BUSY;
        }
    }
    //AMLOG_INFO("[%d]Get Frame (%d)!\n", mFrame.enc_id, mFrame.frame_num);

    //error case, driver return NULL data pointer
    AM_ASSERT(mFrame.usr_start_addr);
    if (!mFrame.usr_start_addr) {
        AM_ERROR("NULL pointer(mFrame.usr_start_addr) from IAV, please check code.\n");
        return ME_OS_ERROR;
    }

    if (!mbGetFirstPTS) {
        mbGetFirstPTS = true;
        if (mFrame.pts_64 > mMaxStartPTS) {
            mStartPTSOffset = mFrame.pts_64 - mMaxStartPTS;
        }
        AMLOG_INFO("First PTS from dsp is %llu, mStartPTSOffset %llu.\n", mFrame.pts_64, mStartPTSOffset);
        for (i = 0; i < mStreamNum; i++) {
            mpOutputPin[i]->mStartPTSOffset = mStartPTSOffset;
        }
    }

    for(i = 0; i < mStreamNum; i++)
    {
        AM_ASSERT(mpOutputPin[i]);
        //curr stream should be skip to IDR
        if(mpOutputPin[i]->mbSkip)
        {
            //if curr stream has IDR, all this will be send to READY
            if(HasIDRFrame(i) == ME_OK)
            {
                return ME_OK;
                //mTotalDrop[i] = GetFrameNum(i); do this in procss buffer
            }
        }else{
            //a stream which no skip
            if(GetFrameNum(i) != 0)
                return ME_OK;
        }
    }
    for(i = 0; i < mStreamNum; i++)
    {
        AM_ASSERT(mpOutputPin[i]);
        AMLOG_WARN("^^^^ReadInputData: %d, Drop Frame: %d !\n", i, GetFrameNum(i));
        mpOutputPin[i]->mTotalDrop = GetFrameNum(i);
    }
    AMLOG_INFO("return ME_BUSY!!!!!\n");

    //need release to iav
    if (ioctl(mIavFd, IAV_IOC_RELEASE_ENCODED_FRAME, &mFrame) < 0) {
        AMLOG_WARN("IAV_IOC_RELEASE_ENCODED_FRAME error.\n");
    }

    return ME_BUSY;
}

inline AM_UINT CVideoEncoder::GetFrameNum(AM_UINT index)
{
    if(mFrame.enc_id == index)
        return 1;
    return 0;
}

//For Those should be skiped, no count the pre-no-idr frame.
inline AM_UINT CVideoEncoder::GetAliveFrameNum(AM_UINT index)
{
    if(mFrame.enc_id == index && mpOutputPin[index]->mbSkip == false)
        return 1;
    if(mFrame.pic_type == 1)
        return 1;
    return 0;
}

inline AM_ERR CVideoEncoder::HasIDRFrame(AM_INT index)
{
    if(mFrame.enc_id == index && mFrame.pic_type == PredefinedPictureType_IDR)
        return ME_OK;
    return ME_ERROR;
}

//set mbBlock here
inline AM_ERR CVideoEncoder::CheckBufferPool()
{
    AM_UINT i = 0;
    bool bAll = true;
    for(; i < mStreamNum; i++)
    {
        AM_ASSERT(mpOutputPin[i]);
        mpOutputPin[i]->mbBlock = false;
    }
    for(i = 0; i < mStreamNum; i++)
    {
        AM_ASSERT(mpOutputPin[i]);
        AM_ASSERT(mpOutputPin[i]->mpBSB);
        if(mpOutputPin[i]->mpBSB->GetFreeBufferCnt() < GetAliveFrameNum(i))
        {
            AMLOG_WARN("^^^^BufferPool: %d, Not Enough Free Buffer!\n", i);
            mpOutputPin[i]->mbBlock = true;
        }else{
            bAll = false;
        }
    }
    return (bAll == true) ? ME_ERROR : ME_OK;
}

AM_ERR CVideoEncoder::NotifyFlowControl()
{
    AM_UINT i = 0;
    for(; i < mStreamNum; i++)
    {
        AM_ASSERT(mpOutputPin[i]);
        if(mpOutputPin[i]->mbBlock == true) {
            AMLOG_WARN("!!!!!%d blocked!!!!.\n", i);
            //Notify(i);
        }
    }
    return ME_OK;
}

//set mbSkip Here
inline AM_ERR CVideoEncoder::DropFrame()
{
    AM_UINT i = 0;
    for(; i < mStreamNum; i++)
    {
        AM_ASSERT(mpOutputPin[i]);
        if(mpOutputPin[i]->mbBlock == true)
        {
            //Drop this way
            if(HasIDRFrame(i) == ME_OK){
                mpOutputPin[i]->mbSkip = true;
                mpOutputPin[i]->mIDRDrop++;
            }
            AMLOG_WARN("^^^^Not Output: %d, Drop Frame: %d, Has IDR Drop: %d !\n", i, GetFrameNum(i), mpOutputPin[i]->mbSkip);
            mpOutputPin[i]->mTotalDrop += GetFrameNum(i);
        }
    }
    return ME_OK;
}

AM_ERR CVideoEncoder::ProcessBuffer()
{
        //AMLOG_DEBUG("ProcessBuffer.\n");
        //safe check, todo
        if (mFrame.enc_id  >= mStreamNum) {
            AMLOG_WARN("stream id(%d) greater then expected(mStreamNum %d).\n", mFrame.enc_id, mStreamNum);
            return ME_OK;
        }

        if(mpOutputPin[mFrame.enc_id]->mbBlock == true && mFrame.enc_id <mStreamNum)
        {
            AMLOG_WARN("stream id(%d) blocked.\n", mFrame.enc_id);

            //need release to iav
            if (ioctl(mIavFd, IAV_IOC_RELEASE_ENCODED_FRAME, &mFrame) < 0) {
                AMLOG_WARN("IAV_IOC_RELEASE_ENCODED_FRAME error.\n");
            }
            return ME_OK;
        }
#if 1
        mpOutputPin[mFrame.enc_id]->ProcessBuffer(&mFrame, mLastFrameExpiredTime);
#else
        if(mpOutputPin[desc->stream_id]->mbSkip == true)
        {
            if(desc->pic_type != 1)
            {
                AMLOG_DEBUG("^^^^ProcessBuffer: %d, Drop Not IDR Frame!\n", desc->stream_id);
                mpOutputPin[desc->stream_id]->mTotalDrop++;
                continue;
            }else{
                mpOutputPin[desc->stream_id]->mbSkip = false;
            }
        }
        //todo
        CBuffer* pBuffer;
        AM_ASSERT(mpOutputPin[desc->stream_id]->mpBSB->GetFreeBufferCnt() > 0);
        if (!mpOutputPin[desc->stream_id]->AllocBuffer(pBuffer))
        {
            AM_ASSERT(0);
            return ME_ERROR;
        }
        pBuffer->mReserved = desc->stream_id; //test
        pBuffer->SetType(CBuffer::DATA);
        pBuffer->mFlags = 0;
        pBuffer->mPTS = desc->PTS;
        pBuffer->mBlockSize = pBuffer->mDataSize = (desc->pic_size + 31) & ~31;
        pBuffer->mpData = (AM_U8*)desc->start_addr;
        pBuffer->mSeqNum = mpOutputPin[desc->stream_id]->mnFrames++;
        pBuffer->mFrameType = desc->pic_type;
        mpOutputPin[desc->stream_id]->SendBuffer(pBuffer);
#endif
    return ME_OK;
}

AM_ERR CVideoEncoder::ProcessEOS()
{
    AMLOG_INFO("CVideoEncoder ProcessEOS()\n");
    CBuffer *pBuffer;
    for(AM_UINT i = 0; i < mStreamNum; i++)
    {
        if (!mpOutputPin[i]->AllocBuffer(pBuffer))
            return ME_ERROR;
        pBuffer->SetType(CBuffer::EOS);
        pBuffer->SetDataPtr(NULL);
        //AM_INFO("CVideoEncoder send EOS\n");
        pBuffer->mFlags = 0;
        mpOutputPin[i]->SendBuffer(pBuffer);
    }
    return ME_OK;
}

void CVideoEncoder::CheckSourceInfo(void)
{
    char    format[32];
    AM_UINT i;
    struct amba_vin_source_info src_info;
    struct amba_vin_source_mode_info    mode_info;
    AM_UINT     fps_list[CHECK_AMBA_VIDEO_FPS_LIST_SIZE];
    AM_UINT     fps_list_size;

    if (ioctl(mIavFd, IAV_IOC_VIN_SRC_GET_INFO, &src_info) < 0) {
        AM_PERROR("IAV_IOC_VIN_SRC_GET_INFO");
        return;
    }
    AMLOG_INFO("\nFind Vin Source %s ", src_info.name);

    if (src_info.dev_type != AMBA_VIN_SRC_DEV_TYPE_DECODER) {
        AMLOG_INFO("it supports:\n");

        for (i = 0; i < AMBA_VIDEO_MODE_NUM; i++) {
            mode_info.mode = amba_video_mode_index2mode(i);
            mode_info.fps_table_size = CHECK_AMBA_VIDEO_FPS_LIST_SIZE;
            mode_info.fps_table = fps_list;

            if (ioctl(mIavFd, IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE, &mode_info) < 0) {
                AM_PERROR("IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE");
                continue;
            }

            if (mode_info.is_supported == 1) {
                fps_list_size = get_fps_list_count(&mode_info);
                //_test_print_video_info(&mode_info.video_info,fps_list_size, fps_list);
            }
        }
        mVinSource = 0;
    } else {
        AM_UINT active_channel_id = src_info.active_channel_id;

        AMLOG_INFO("it supports %d channel.\n", src_info.total_channel_num);
        for (i = 0; i < src_info.total_channel_num; i++) {
            if (ioctl(mIavFd, IAV_IOC_SELECT_CHANNEL, i) < 0) {
                AM_PERROR("IAV_IOC_SELECT_CHANNEL");
                continue;
            }

            if (ioctl(mIavFd, IAV_IOC_VIN_SRC_GET_INFO, &src_info) < 0) {
                AM_PERROR("IAV_IOC_VIN_SRC_GET_INFO");
                continue;
            }

            switch(src_info.source_type.decoder) {
                case AMBA_VIN_DECODER_CHANNEL_TYPE_CVBS:
                    snprintf(format, 32, "%s", "CVBS");
                    break;
                case AMBA_VIN_DECODER_CHANNEL_TYPE_SVIDEO:
                    snprintf(format, 32, "%s", "S-Video");
                    break;
                case AMBA_VIN_DECODER_CHANNEL_TYPE_YPBPR:
                    snprintf(format, 32, "%s", "YPbPr");
                    break;
                case AMBA_VIN_DECODER_CHANNEL_TYPE_HDMI:
                    snprintf(format, 32, "%s", "HDMI");
                    break;
                case AMBA_VIN_DECODER_CHANNEL_TYPE_VGA:
                    snprintf(format, 32, "%s", "VGA");
                    break;
                default:
                    snprintf(format, 32, "format?%d", src_info.source_type.decoder);
                    break;
            }
            AMLOG_INFO("Channel[%s] %d's type is %s, ", src_info.name, src_info.active_channel_id, format);

            mode_info.mode = AMBA_VIDEO_MODE_AUTO;
            mode_info.fps_table_size = CHECK_AMBA_VIDEO_FPS_LIST_SIZE;
            mode_info.fps_table = fps_list;
            if (ioctl(mIavFd, IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE, &mode_info) < 0) {
                AM_PERROR("IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE");
                continue;
            }

            if (mode_info.is_supported == 1) {
                AMLOG_INFO("The signal is:\n");
                fps_list_size = get_fps_list_count(&mode_info);
                //_test_print_video_info(&mode_info.video_info, fps_list_size, fps_list);
                if (mVinSource == -1) {
                    mVinSource = src_info.id;
                }
            } else
                AMLOG_INFO("No signal yet.\n");
        }

        if (ioctl(mIavFd, IAV_IOC_SELECT_CHANNEL, active_channel_id) < 0) {
            AM_PERROR("IAV_IOC_SELECT_CHANNEL");
            return;
        }
    }
}

AM_ERR CVideoEncoder::InitVin(enum amba_video_mode mode)
{
    AM_UINT num, i;
    struct amba_video_info video_info;
    struct amba_vin_source_info src_info;

    num = 0;
    if (ioctl(mIavFd, IAV_IOC_VIN_GET_SOURCE_NUM, &num) < 0) {
        AM_PERROR("IAV_IOC_VIN_GET_SOURCE_NUM");
        return ME_OS_ERROR;
    }

    if (num < 1) {
        AM_ERROR("Please load source driver!\n");
        return ME_OS_ERROR;
    }

    for (i=0; i<num; i++) {
        if (ioctl(mIavFd, IAV_IOC_VIN_SET_CURRENT_SRC, &i) < 0) {
            AM_PERROR("IAV_IOC_VIN_SET_CURRENT_SRC");
            return ME_OS_ERROR;
        }

        CheckSourceInfo();
    }

    if (ioctl(mIavFd, IAV_IOC_VIN_SET_CURRENT_SRC, &mVinSource) < 0) {
        AM_PERROR("IAV_IOC_VIN_SET_CURRENT_SRC");
        return ME_OS_ERROR;
    }

    if (ioctl(mIavFd, IAV_IOC_VIN_SRC_SET_VIDEO_MODE, mode) < 0) {
        AM_PERROR("IAV_IOC_VIN_SRC_SET_VIDEO_MODE");
        return ME_OS_ERROR;
    }

    if (mVinMirrorMode >= 0 && mVinMirrorPattern >= 0) {
        if (ioctl(mIavFd, IAV_IOC_VIN_SRC_SET_MIRROR_MODE, &mVinMirrorMode) < 0) {
            AM_PERROR("IAV_IOC_VIN_SRC_SET_MIRROR_MODE");
            return ME_OS_ERROR;
        }
    }

    if (mVinAntiFlicker >= 0) {
        if (ioctl(mIavFd, IAV_IOC_VIN_SRC_SET_ANTI_FLICKER, mVinAntiFlicker) < 0) {
            AM_PERROR("IAV_IOC_VIN_SRC_SET_ANTI_FLICKER");
            return ME_OS_ERROR;
        }
    }

    if (ioctl(mIavFd, IAV_IOC_VIN_SRC_GET_INFO, &src_info) < 0) {
        AM_PERROR("IAV_IOC_VIN_SRC_GET_INFO");
        return ME_OS_ERROR;
    }

    if (src_info.dev_type != AMBA_VIN_SRC_DEV_TYPE_DECODER) {
        if (video_info.type == AMBA_VIDEO_TYPE_RGB_RAW) {
            if (set_vin_param(mIavFd) < 0)    //set aaa here
                return ME_OS_ERROR;
        }
    }

    if (src_info.dev_type != AMBA_VIN_SRC_DEV_TYPE_DECODER) {
        if (mVinFrameRate) {
            if (ioctl(mIavFd, IAV_IOC_VIN_SRC_SET_FRAME_RATE, mVinFrameRate) < 0) {
                AM_PERROR("IAV_IOC_VIN_SRC_SET_FRAME_RATE");
                return ME_OS_ERROR;
            }
        }
    }

    if (ioctl(mIavFd, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
        AM_PERROR("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
        return ME_OS_ERROR;
    }

    AMLOG_INFO("Active src %d's mode is:\n", mVinSource);
    //_test_print_video_info(&video_info, 0, NULL);

    AMLOG_INFO("init_vin done\n");

    return ME_OK;
}

void CVideoEncoder::testPrintConfig(StreamDesc* desc)
{
    if (desc==NULL) {
        AM_ERROR("NULL pointer.\n");
        return;
    }
    AMLOG_INFO("\t   width = %d, height %d, framerate %d.\n", desc->width, desc->height, desc->framerate);

    AMLOG_INFO("\t     entropy_type = %d\n", desc->entropy_type);
    AMLOG_INFO("\t           M = %d\n", desc->M);
    AMLOG_INFO("\t           N = %d\n", desc->N);
    AMLOG_INFO("\tidr interval = %d\n", desc->IDRInterval);
    AMLOG_INFO("\t   gop model = %d\n", desc->gop_model);
    AMLOG_INFO("\t     bitrate = %d bps\n", desc->average_bitrate);
    AMLOG_INFO("\tbitrate ctrl = %d\n", desc->bitrate_control);

    AMLOG_INFO("\t calibration = %d, desc->vbr_ness %d, min_vbr_rate_factor %d, max_vbr_rate_factor %d.\n", desc->calibration, desc->vbr_ness, desc->min_vbr_rate_factor, desc->max_vbr_rate_factor);
}

void CVideoEncoder::PrintH264Config(iav_enc_config_t *config)
{
    if (config==NULL) {
        AM_ERROR("NULL pointer.\n");
        return;
    }
    AMLOG_INFO("\t     profile = %s\n", (config->u.h264.entropy_codec == IAV_ENTROPY_CABAC)? "main":"baseline");
    AMLOG_INFO("\t           M = %d\n", config->u.h264.M);
    AMLOG_INFO("\t           N = %d\n", config->u.h264.N);
    AMLOG_INFO("\tidr interval = %d\n", config->u.h264.idr_interval);
    AMLOG_INFO("\t   gop model = %s\n", (config->u.h264.gop_model == 0)? "simple":"advanced");
    AMLOG_INFO("\t     bitrate = %d bps\n", config->u.h264.average_bitrate);
    AMLOG_INFO("\tbitrate ctrl = %s\n", (config->u.h264.bitrate_control == IAV_BRC_CBR)? "cbr":"vbr");
    if (config->u.h264.bitrate_control == IAV_BRC_VBR) {
        AMLOG_INFO("\tmin_vbr_rate = %d\n", config->u.h264.min_vbr_rate_factor);
        AMLOG_INFO("\tmax_vbr_rate = %d\n", config->u.h264.max_vbr_rate_factor);
    }
}

AM_ERR CVideoEncoder::addOsdBlendArea(int xPos, int yPos, int width, int height, void** addr_y, void** addr_uv, void** a_addr_y, void** a_addr_uv)
{
    AM_INT ret =0;
    AMLOG_INFO("addOsdBlendArea, x=%d, y=%d, size=%dx%d.\n", xPos, yPos, width, height);

    mOsdBlendInfo.enc_id = 0;
    mOsdBlendInfo.osd_start_x = xPos;
    mOsdBlendInfo.osd_start_y = yPos;
    mOsdBlendInfo.osd_width = width;
    mOsdBlendInfo.osd_height = height;
    if(mIavFd <= 0){
        AMLOG_ERROR("IAV fd = %d!\n", mIavFd);
        return ME_BAD_STATE;
    }
    ret = ioctl(mIavFd, IAV_IOC_ADD_OSD_BLEND_AREA, &mOsdBlendInfo);
    if(0 != ret){
        AMLOG_ERROR("IAV_IOC_ADD_OSD_BLEND_AREA error, ret %d.\n", ret);
        return ME_ERROR;
    }

    mbOsdInited = true;
    *addr_y = (void*)mOsdBlendInfo.osd_addr_y;
    *addr_uv = (void*)mOsdBlendInfo.osd_addr_uv;
    *a_addr_y = (void*)mOsdBlendInfo.alpha_addr_y;
    *a_addr_uv = (void*)mOsdBlendInfo.alpha_addr_uv;
    AMLOG_INFO("addOsdBlendArea id(%d) return 0x%p, 0x%p, 0x%p, 0x%p.\n", mOsdBlendInfo.area_id, *addr_y, *addr_uv, *a_addr_y, *a_addr_uv);
    return ME_OK;
}

AM_ERR CVideoEncoder::updateOsdBlendArea()
{
    AM_INT ret =0;
    AMLOG_DEBUG("updateOsdBlendArea.\n");

    if(!mbOsdInited){
        AMLOG_ERROR("Osd area is not added!\n");
        return ME_NOT_EXIST;
    }

    ret = ioctl(mIavFd, IAV_IOC_UPDATE_OSD_BLEND_AREA, mOsdBlendInfo.area_id);
    if(0 != ret){
        AMLOG_ERROR("IAV_IOC_UPDATE_OSD_BLEND_AREA error, ret %d.\n", ret);
        return ME_ERROR;
    }
    return ME_OK;
}

AM_ERR CVideoEncoder::removeOsdBlendArea()
{
    AM_INT ret =0;
    AMLOG_DEBUG("removeOsdBlendArea.\n");

    if(!mbOsdInited){
        AMLOG_ERROR("Osd area is not added!\n");
        return ME_NOT_EXIST;
    }

    ret = ioctl(mIavFd, IAV_IOC_REMOVE_OSD_BLEND_AREA, mOsdBlendInfo.area_id);
    if(0 != ret){
        AMLOG_ERROR("IAV_IOC_REMOVE_OSD_BLEND_AREA error, ret %d.\n", ret);
        return ME_ERROR;
    }
    AMLOG_INFO("Osd area %d is removed.\n", mOsdBlendInfo.area_id);

    memset(&mOsdBlendInfo, 0, sizeof(iav_osd_blend_info_t));
    mbOsdInited= false;

    return ME_OK;
}

//-----------------------------------------------------------------------
//
// CVideoEncoderOutput
//
//-----------------------------------------------------------------------
CVideoEncoderOutput* CVideoEncoderOutput::Create(CFilter *pFilter)
{
    CVideoEncoderOutput *result = new CVideoEncoderOutput(pFilter);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CVideoEncoderOutput::Construct()
{
    mpFlowControlList = new CList();
    return ME_OK;
}

AM_ERR CVideoEncoderOutput::SendVideoBuffer(iav_frame_desc_t* p_frame, AM_U64 expired_time)
{
#ifdef __print_time_info__
    struct timeval tv_current_buffer;
#endif

    CBuffer* pBuffer;
    AM_ASSERT(mpBSB->GetFreeBufferCnt() > 0);
    if (!AllocBuffer(pBuffer)) {
        AM_ASSERT(0);
        return ME_ERROR;
    }

    AMLOG_DEBUG("SendVideoBuffer frame num %d, stream id %d, pictype %d, pts %lld. DATA_DDR:%p\n"
        , p_frame->frame_num, p_frame->enc_id, p_frame->pic_type, p_frame->pts_64, p_frame->usr_start_addr);

    //pBuffer->mReserved = p_frame->fd_id; //release
    pBuffer->mBufferId = p_frame->fd_id;
    pBuffer->mEncId = p_frame->enc_id;
    pBuffer->SetType(CBuffer::DATA);
    pBuffer->mFlags |= IAV_ENC_BUFFER ;
    //re-align pts
    pBuffer->mPTS = p_frame->pts_64 - mStartPTSOffset;
    pBuffer->mExpireTime = expired_time;
    pBuffer->mBlockSize = pBuffer->mDataSize = p_frame->pic_size;
    pBuffer->mpData = (AM_U8*)p_frame->usr_start_addr;
    AM_ASSERT(pBuffer->mpData);
    pBuffer->mSeqNum = mnFrames++;
    pBuffer->mOriSeqNum = p_frame->frame_num;
    pBuffer->mFrameType = p_frame->pic_type;

#ifdef __print_time_info__
    gettimeofday(&tv_current_buffer,NULL);
    if (tv_current_buffer.tv_usec >= tv_last_buffer.tv_usec) {
        AMLOG_PRINTF("[VE] time gap from last send buffer %d sec, %d usec.\n", tv_current_buffer.tv_sec - tv_last_buffer.tv_sec, tv_current_buffer.tv_usec - tv_last_buffer.tv_usec);
    } else {
        AMLOG_PRINTF("[VE] time gap from last send buffer %d sec, %d usec.\n", tv_current_buffer.tv_sec - tv_last_buffer.tv_sec - 1, tv_current_buffer.tv_usec + 1000000 - tv_last_buffer.tv_usec);
    }
    tv_last_buffer = tv_current_buffer;
#endif

    SendBuffer(pBuffer);
    return ME_OK;
}

AM_ERR CVideoEncoderOutput::SendFlowControlBuffer(IFilter::FlowControlType type, AM_U64 pts)
{
    CBuffer* pBuffer;
    if (!AllocBuffer(pBuffer)) {
        AM_ASSERT(0);
        return ME_ERROR;
    }

    pBuffer->SetType(CBuffer::FLOW_CONTROL);
    pBuffer->SetFlags((AM_INT) type);
    pBuffer->SetPTS(pts);
    pBuffer->SetDataPtr(NULL);
    pBuffer->mFlags = 0;
    SendBuffer(pBuffer);
    return ME_OK;
}

void CVideoEncoderOutput::ProcessBuffer(iav_frame_desc_t* p_frame, AM_U64 expired_time)
{
    SListNode* pNode;
    AMLOG_DEBUG("process p_desc frame num %d, stream id %d, pictype %d, pts %lld.\n", p_frame->frame_num, p_frame->enc_id, p_frame->pic_type, p_frame->pts_64);

    //send finalize file if needed
    if ((mbNeedSendFinalizeFile) && (PredefinedPictureType_IDR == p_frame->pic_type)) {
        mbNeedSendFinalizeFile = false;
        AM_ASSERT(VideoEncOPin_Runing == msState);
        SendFlowControlBuffer(IFilter::FlowControl_finalize_file, p_frame->pts_64);
    }

    switch (msState) {
        case VideoEncOPin_Runing:
            pNode = mpFlowControlList->peek_first();
            if (pNode) {
                if (pNode->context == IFilter::FlowControl_pause) {
                    msState = VideoEncOPin_tobePaused;
                    //remove node
                    mpFlowControlList->release(pNode);

                    if (mbSkip) {
                        break;
                    }

                    //pause immediately when reference picture comes
                    if (PredefinedPictureType_B != p_frame->pic_type) {
                        //SendVideoBuffer(p_desc);
                        mbSkip = true;
                        SendFlowControlBuffer(IFilter::FlowControl_pause);
                        msState = VideoEncOPin_Paused;
                        AMLOG_INFO("pause comes.\n");
                        return;
                    }
                } else if (pNode->context == IFilter::FlowControl_finalize_file) {
                    AM_ASSERT(false == mbNeedSendFinalizeFile);
                    mbNeedSendFinalizeFile = true;
                } else {
                    AMLOG_WARN("should not comes here, how to handle it?\n");
                }
            }

            //skip data when output cannot handle quikly
            if (mbSkip == true) {
                //IDR resume skip
                if (PredefinedPictureType_IDR == p_frame->pic_type) {
                    mbSkip = false;
                } else {
                    AMLOG_WARN("skip frame here.\n");
                    mTotalDrop ++;
                    return;
                }
            }
            SendVideoBuffer(p_frame, expired_time);
            break;

        case VideoEncOPin_tobePaused:
            AM_ASSERT(mbSkip == false);
            //pause till reference picture comes
            if (PredefinedPictureType_B != p_frame->pic_type) {
                mbSkip = true;
                SendFlowControlBuffer(IFilter::FlowControl_pause);
                msState = VideoEncOPin_Paused;
                return;
            }
            SendVideoBuffer(p_frame, expired_time);
            break;

        case VideoEncOPin_Paused:
            AM_ASSERT(mbSkip == true);
            pNode = mpFlowControlList->peek_first();
            if (pNode) {
                if (pNode->context == IFilter::FlowControl_resume) {
                    msState = VideoEncOPin_tobeResumed;
                    //remove node
                    mpFlowControlList->release(pNode);

                    //resume immediately when idr picture comes
                    if (PredefinedPictureType_IDR == p_frame->pic_type) {
                        SendFlowControlBuffer(IFilter::FlowControl_resume);
                        SendVideoBuffer(p_frame, expired_time);
                        mbSkip = false;
                        msState = VideoEncOPin_Runing;
                        return;
                    }
                } else if (pNode->context == IFilter::FlowControl_finalize_file) {
                    AM_ASSERT(false == mbNeedSendFinalizeFile);
                    SendFlowControlBuffer(IFilter::FlowControl_finalize_file);
                } else {
                    AMLOG_WARN("should not comes here, how to handle it?\n");
                }
            }
            break;

        case VideoEncOPin_tobeResumed:
            AM_ASSERT(mbSkip == true);
            //pause till idr picture comes
            if (PredefinedPictureType_IDR == p_frame->pic_type) {
                SendFlowControlBuffer(IFilter::FlowControl_resume);
                SendVideoBuffer(p_frame, expired_time);
                mbSkip = false;
                msState = VideoEncOPin_Runing;
                return;
            }
            break;

        default:
            AM_ASSERT(0);
            AM_ERROR("must not comes here.\n");
    }

}


