
/*
 * video_encoder.cpp
 *
 * History:
 *    2011/7/8 - [Jay Zhang] create file
 *
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

#include "video_encoder.h"

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

void test_print_video_info( const struct amba_video_info *pvinfo, AM_UINT fps_list_size, AM_UINT *fps_list)
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
        mpOutputPin[i]->mpBSB = CSimpleBufferPool::Create("EncBSB", 64);
        if (mpOutputPin[i]->mpBSB == NULL) {
            AM_ERROR("CSimpleBufferPool::Create fail in CVideoEncoder::Construct().\n");
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

    memset(&mConfig, 0, sizeof(mConfig));
    //default settings
    for (i=0; i<MAX_NUM_STREAMS; i++) {
        mConfig.stream_info[i].entropy_type = IAV_ENTROPY_CABAC;
    }

    return ME_OK;
}

CVideoEncoder::~CVideoEncoder()
{
    if (mbMemMapped)
        ::ioctl(mIavFd, IAV_IOC_UNMAP_BSB, 0);

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
    if (mIavFd >= 0) {
        close(mIavFd);
        mIavFd = -1;
    }

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

    if (param->video.entropy_type == IParameters::EntropyType_H264_CABAC) {
        mConfig.stream_info[index].entropy_type = IAV_ENTROPY_CABAC;
    } else if (param->video.entropy_type == IParameters::EntropyType_H264_CAVLC) {
        mConfig.stream_info[index].entropy_type = IAV_ENTROPY_CAVLC;
    } else {
        //default:
        AM_ASSERT(param->video.entropy_type == IParameters::EntropyType_NOTSet);
        mConfig.stream_info[index].entropy_type = IAV_ENTROPY_CABAC;
    }

    //round frame rate
    mConfig.stream_info[index].framerate = (param->video.framerate_num + param->video.framerate_den/2) / param->video.framerate_den;

    return ME_OK;
}

//===============================================
//
//===============================================
AM_ERR CVideoEncoder::SetConfig()
{
    AM_INT size = sizeof(mH264Config);
    mH264Config.stream = 0;
    mH264Config.entropy_codec = mConfig.stream_info[0].entropy_type;

    mH264Config.idr_interval = mConfig.stream_info[0].IDRInterval;
    mH264Config.M = mConfig.stream_info[0].M;
    mH264Config.N = mConfig.stream_info[0].N;
    AMLOG_INFO("stream[0], idr_interval %d, M %d, N %d.\n", mH264Config.idr_interval, mH264Config.M, mH264Config.N);

    if (::ioctl(mIavFd, IAV_IOC_GET_H264_CONFIG, &mH264Config) < 0) {
        AM_ERROR("IAV_IOC_GET_H264_CONFIG fail.\n");
        AM_PERROR("IAV_IOC_GET_H264_CONFIG\n");
        return ME_ERROR;
    }
    PrintH264Config(&mH264Config);

    AM_INFO("===============================\n");
    mH264Config.stream = 1;
    mH264Config.entropy_codec = mConfig.stream_info[1].entropy_type;
    mH264Config.idr_interval = mConfig.stream_info[1].IDRInterval;
    mH264Config.M = mConfig.stream_info[1].M;
    mH264Config.N = mConfig.stream_info[1].N;
    AMLOG_INFO("stream[1], idr_interval %d, M %d, N %d.\n", mH264Config.idr_interval, mH264Config.M, mH264Config.N);

    if (::ioctl(mIavFd, IAV_IOC_GET_H264_CONFIG, &mH264Config) < 0) {
        AM_PERROR("IAV_IOC_GET_H264_CONFIG\n");
        AM_ERROR("IAV_IOC_GET_H264_CONFIG fail.\n");
        return ME_ERROR;
    }
    PrintH264Config(&mH264Config);

    iav_encode_format_t format;
    memset(&format, 0, sizeof(iav_encode_format_t));

    //main stream
    format.stream = 0;
    if (::ioctl(mIavFd, IAV_IOC_GET_ENCODE_FORMAT, &format) < 0) {
        AM_PERROR("IAV_IOC_GET_ENCODE_FORMAT\n");
        AM_ERROR("IAV_IOC_GET_ENCODE_FORMAT\n");
        return ME_ERROR;
    }
    AM_ASSERT(mConfig.stream_info[0].stream_format == IParameters::StreamFormat_H264);
    if (mConfig.stream_info[0].width && mConfig.stream_info[0].height && (mConfig.stream_info[0].stream_format == IParameters::StreamFormat_H264)) {
        format.encode_type = IAV_ENCODE_H264;
        format.encode_width = mConfig.stream_info[0].width;
        format.encode_height = mConfig.stream_info[0].height;
    } else {
        AM_ASSERT(0);
        AMLOG_ERROR("BAD parameters for main stream.\n");
    }
    AM_INFO(" StopPreview format.main_encode_type  %d, main_width %d, main_height  %d\n",
               format.encode_type, format.encode_width, format.encode_height);

    if (::ioctl(mIavFd, IAV_IOC_SET_ENCODE_FORMAT, &format) < 0) {
        AM_PERROR("IAV_IOC_SET_ENCODE_FORMAT");
        AM_ERROR("IAV_IOC_SET_ENCODE_FORMAT fail.\n");
        return ME_ERROR;
    }
    mStreamsMask |= IAV_MAIN_STREAM;

    //second stream
    if(mStreamNum > 1)
    {
        format.stream = 1;
        if (::ioctl(mIavFd, IAV_IOC_GET_ENCODE_FORMAT, &format) < 0) {
            AM_PERROR("IAV_IOC_GET_ENCODE_FORMAT\n");
            AM_ERROR("IAV_IOC_GET_ENCODE_FORMAT\n");
            return ME_ERROR;
        }
        AM_ASSERT(mConfig.stream_info[1].stream_format == IParameters::StreamFormat_H264);
        if (mConfig.stream_info[1].width && mConfig.stream_info[1].height && (mConfig.stream_info[1].stream_format == IParameters::StreamFormat_H264)) {
            format.encode_type = IAV_ENCODE_H264;
            format.encode_width = mConfig.stream_info[1].width;
            format.encode_height = mConfig.stream_info[1].height;
            AM_PRINTF(" StopPreview format.secondary_encode_type  %d, secondary_width %d, secondary_height  %d\n",
                   format.encode_type, format.encode_width, format.encode_height);
        } else {
            AM_ASSERT(0);
            AMLOG_ERROR("BAD parameters for secondary stream.\n");
        }
        if (::ioctl(mIavFd, IAV_IOC_SET_ENCODE_FORMAT, &format) < 0) {
            AM_PERROR("IAV_IOC_SET_ENCODE_FORMAT");
            AM_ERROR("IAV_IOC_SET_ENCODE_FORMAT fail.\n");
            return ME_ERROR;
        }
        mStreamsMask |= IAV_2ND_STREAM;

        //third stream
        if(mStreamNum > 2)
        {
            format.stream = 2;
            if (::ioctl(mIavFd, IAV_IOC_GET_ENCODE_FORMAT, &format) < 0) {
                AM_PERROR("IAV_IOC_GET_ENCODE_FORMAT\n");
                AM_ERROR("IAV_IOC_GET_ENCODE_FORMAT\n");
                return ME_ERROR;
            }
            AM_ASSERT(mConfig.stream_info[2].stream_format == IParameters::StreamFormat_H264);
            if (mConfig.stream_info[2].width && mConfig.stream_info[2].height && (mConfig.stream_info[2].stream_format == IParameters::StreamFormat_H264)) {
                format.encode_type = IAV_ENCODE_H264;
                format.encode_width = mConfig.stream_info[2].width;
                format.encode_height = mConfig.stream_info[2].height;
                AM_PRINTF(" StopPreview format.third_encode_type  %d, third_width %d, third_height  %d",
                    format.encode_type, format.encode_width, format.encode_height);
            } else {
                AM_ASSERT(0);
                AMLOG_ERROR("BAD parameters for third stream.\n");
            }
            if (::ioctl(mIavFd, IAV_IOC_SET_ENCODE_FORMAT, &format) < 0) {
                AM_PERROR("IAV_IOC_SET_ENCODE_FORMAT");
                AM_ERROR("IAV_IOC_SET_ENCODE_FORMAT fail.\n");
                return ME_ERROR;
            }
            mStreamsMask |= IAV_3RD_STREAM;
        }
    }


    iav_state_info_t info;
    if (::ioctl(mIavFd, IAV_IOC_GET_STATE_INFO, &info) < 0) {
        AM_PERROR("IAV_IOC_GET_STATE_INFO\n");
        AM_ERROR("Fatal error: IAV_IOC_GET_STATE_INFO fail.\n");
        return ME_ERROR;
    }
    AM_PRINTF("IAV_IOC_START_ENCODE state = %d  %d  %d\n", info.state, info.dsp_encode_state, info.dsp_encode_mode);

    //if not IDLE or Preview, enter idle first
    if (info.state != IAV_STATE_IDLE && info.state != IAV_STATE_PREVIEW) {
        AMLOG_INFO("DSP Not in IDLE & DECODING mode, enter IDLE mode first, state %d.\n", info.state);
        if ((::ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0)) < 0) {
            AMLOG_ERROR("UDEC enter IDLE mode fail.\n");
            return ME_OS_ERROR;
        }
        AMLOG_INFO("enter IDLE mode done.\n");

        //re-get iav state
        if (::ioctl(mIavFd, IAV_IOC_GET_STATE_INFO, &info) < 0) {
            AM_PERROR("IAV_IOC_GET_STATE_INFO\n");
            AM_ERROR("Fatal error: IAV_IOC_GET_STATE_INFO fail.\n");
            return ME_OS_ERROR;
        }
    }

    if(info.state == IAV_STATE_IDLE)
    {
        //init vin first, hard code here
        InitVin(AMBA_VIDEO_MODE_1080P);

        AM_PRINTF("Enter Preview!\n");
        if(::ioctl(mIavFd, IAV_IOC_ENABLE_PREVIEW, 0) < 0){
            AM_PERROR("IAV_IOC_ENABLE_PREVIEW\n");
            AM_ERROR("IAV_IOC_ENABLE_PREVIEW fail.\n");
            return ME_OS_ERROR;
        }
    }else if(info.state != IAV_STATE_PREVIEW){
        AM_ASSERT(0);
        AM_ERROR("Dsp State Neither In Idle nor Preview!\n");
        return ME_OS_ERROR;
    }

    AMLOG_DEBUG("before IAV_IOC_START_ENCODE, mStreamsMask 0x%x, mStreamNum %d.\n", mStreamsMask, mStreamNum);
    // start encoding
    if (::ioctl(mIavFd, IAV_IOC_START_ENCODE, mStreamsMask) < 0) {
        AM_PERROR("IAV_IOC_START_ENCODE\n");
        AM_ERROR("IAV_IOC_START_ENCODE fail, mStreamsMask 0x%x.\n", mStreamsMask);
        return ME_ERROR;
    }
    AMLOG_DEBUG("IAV_IOC_START_ENCODE done.\n");

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
    ioctl(mIavFd, IAV_IOC_STOP_ENCODE, mStreamsMask);
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
    AM_UINT i = 0;
    if(SetConfig() != ME_OK)
    {
        AM_PERROR("SetConfig Failed!\n");
        return ME_ERROR;
    }

    iav_mmap_info_t mmap;
    if (::ioctl(mIavFd, IAV_IOC_MAP_BSB2, &mmap) < 0) {
        AM_PERROR("IAV_IOC_MAP_BSB2");
        return ME_BAD_STATE;
    }
    AMLOG_INFO("mem_base: 0x%p, size = 0x%x\n", mmap.addr, mmap.length);
    mbMemMapped = true;
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
                }else {
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

    AMLOG_INFO("CVideoEncoder::OnRun end");
}

AM_ERR CVideoEncoder::ReadInputData()
{
    AM_UINT i = 0;
    AM_INT ret = 0;
    memset(&mFifoInfo, 0 , sizeof(bs_fifo_info_t));

    if ((ret = ::ioctl(mIavFd, IAV_IOC_READ_BITSTREAM, &mFifoInfo)) < 0)
    {
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

    for (i = 0; i < mFifoInfo.count; i++) {
        AMLOG_DEBUG(" %d's frame_num %d, pictype %d, pts %lld.\n", mFifoInfo.desc[i].stream_id, mFifoInfo.desc[i].frame_num, mFifoInfo.desc[i].pic_type, mFifoInfo.desc[i].PTS);
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
    AMLOG_WARN("return ME_BUSY!!!!!\n");
    return ME_BUSY;
}

inline AM_UINT CVideoEncoder::GetFrameNum(AM_UINT index)
{
    AM_INT num = 0;
    bits_info_t* desc = &mFifoInfo.desc[0];
    for (AM_UINT i = 0; i < mFifoInfo.count; i++, desc++)
    {
        if(desc->stream_id == index)
            num++;
    }
    return num;
}

//For Those should be skiped, no count the pre-no-idr frame.
inline AM_UINT CVideoEncoder::GetAliveFrameNum(AM_UINT index)
{
    AM_UINT num = 0;
    bool bFindIDR = false;
    bits_info_t* desc = &mFifoInfo.desc[0];
    AM_ASSERT(index < mStreamNum);

    for (AM_UINT i = 0; i < mFifoInfo.count; i++, desc++)
    {
        if(desc->stream_id == index)
        {
            AM_ASSERT(mpOutputPin[index]);
            if(mpOutputPin[index]->mbSkip == false)
            {
                num++;
            }else if(bFindIDR == true){
                num++;
            }else if(desc->pic_type == 1){
                bFindIDR = true;
                num++;
            }
        }
    }
    return num;

}

inline AM_ERR CVideoEncoder::HasIDRFrame(AM_INT index)
{
    bits_info_t* desc = &mFifoInfo.desc[0];
    for (AM_UINT i = 0; i < mFifoInfo.count; i++, desc++)
    {
        if(desc->pic_type == 1 && desc->stream_id == index)
            return ME_OK;
    }
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

inline AM_ERR CVideoEncoder::DropByPause()
{
    ReadInputData();
    for(AM_UINT i = 0; i < mStreamNum; i++){
        AM_ASSERT(mpOutputPin[i]);
        mpOutputPin[i]->mPauseDrop += GetFrameNum(i);
    }
    return ME_OK;
}

AM_ERR CVideoEncoder::ProcessBuffer()
{
    bits_info_t* desc = &mFifoInfo.desc[0];
    for (AM_UINT i = 0; i < mFifoInfo.count; i++, desc++)
    {
        AMLOG_DEBUG("process %d.\n", i);
        AM_ASSERT(desc->stream_id <mStreamNum);
        //safe check, todo
        if (desc->stream_id >= mStreamNum) {
            AMLOG_WARN("stream id(%d) greater then expected(mStreamNum %d).\n", desc->stream_id, mStreamNum);
            continue;
        }

        if(mpOutputPin[desc->stream_id]->mbBlock == true && desc->stream_id <mStreamNum)
        {
            AMLOG_WARN("stream id(%d) blocked.\n", desc->stream_id);
            continue;
        }
#if 1
        mpOutputPin[desc->stream_id]->ProcessBuffer(desc);
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
    }
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
        //AM_INFO("CVideoEncoder send EOS\n");
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
                //test_print_video_info(&mode_info.video_info,fps_list_size, fps_list);
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
                //test_print_video_info(&mode_info.video_info, fps_list_size, fps_list);
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
    test_print_video_info(&video_info, 0, NULL);

    AMLOG_INFO("init_vin done\n");

    return ME_OK;
}

void CVideoEncoder::PrintH264Config(iav_h264_config_t *config)
{
    if (config==NULL) return;
    AM_INFO("\t     profile = %s\n", (config->entropy_codec == IAV_ENTROPY_CABAC)? "main":"baseline");
    AM_INFO("\t           M = %d\n", config->M);
    AM_INFO("\t           N = %d\n", config->N);
    AM_INFO("\tidr interval = %d\n", config->idr_interval);
    AM_INFO("\t   gop model = %s\n", (config->gop_model == 0)? "simple":"advanced");
    AM_INFO("\t     bitrate = %d bps\n", config->average_bitrate);
    AM_INFO("\tbitrate ctrl = %s\n", (config->bitrate_control == IAV_BRC_CBR)? "cbr":"vbr");
    if (config->bitrate_control == IAV_BRC_VBR) {
        AM_INFO("\tmin_vbr_rate = %d\n", config->min_vbr_rate_factor);
        AM_INFO("\tmax_vbr_rate = %d\n", config->max_vbr_rate_factor);
    }
    AM_INFO("\t    de-intlc = %s\n", (config->deintlc_for_intlc_vin==0)? "off":"on");
    AM_INFO("\t        ar_x = %d\n", config->pic_info.ar_x);
    AM_INFO("\t        ar_y = %d\n", config->pic_info.ar_y);
    AM_INFO("\t  frame mode = %d\n", config->pic_info.frame_mode);
    AM_INFO("\t        rate = %d\n", config->pic_info.rate);
    AM_INFO("\t       scale = %d\n", config->pic_info.scale);
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

AM_ERR CVideoEncoderOutput::SendVideoBuffer(bits_info_t* p_desc)
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

    AMLOG_DEBUG("SendVideoBuffer frame num %d, stream id %d, pictype %d, pts %lld.\n", p_desc->frame_num, p_desc->stream_id, p_desc->pic_type, p_desc->PTS);

    pBuffer->mReserved = p_desc->stream_id; //test
    pBuffer->SetType(CBuffer::DATA);
    pBuffer->mFlags = 0;
    pBuffer->mPTS = p_desc->PTS;
    pBuffer->mBlockSize = pBuffer->mDataSize = (p_desc->pic_size + 31) & ~31;
    pBuffer->mpData = (AM_U8*)p_desc->start_addr;
    pBuffer->mSeqNum = mnFrames++;
    pBuffer->mFrameType = p_desc->pic_type;

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
    SendBuffer(pBuffer);
    return ME_OK;
}

void CVideoEncoderOutput::ProcessBuffer(bits_info_t* p_desc)
{
    SListNode* pNode;
    AMLOG_DEBUG("process p_desc frame num %d, stream id %d, pictype %d, pts %lld.\n", p_desc->frame_num, p_desc->stream_id, p_desc->pic_type, p_desc->PTS);

    //send finalize file if needed
    if ((mbNeedSendFinalizeFile) && (PredefinedPictureType_IDR == p_desc->pic_type)) {
        mbNeedSendFinalizeFile = false;
        AM_ASSERT(VideoEncOPin_Runing == msState);
        SendFlowControlBuffer(IFilter::FlowControl_finalize_file, p_desc->PTS);
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
                    if (p_desc->pic_type != 4) {
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
                if (p_desc->pic_type == 1) {
                    mbSkip = false;
                } else {
                    AMLOG_WARN("skip frame here.\n");
                    mTotalDrop ++;
                    return;
                }
            }
            SendVideoBuffer(p_desc);
            break;

        case VideoEncOPin_tobePaused:
            AM_ASSERT(mbSkip == false);
            //pause till reference picture comes
            if (p_desc->pic_type != 4) {
                mbSkip = true;
                SendFlowControlBuffer(IFilter::FlowControl_pause);
                msState = VideoEncOPin_Paused;
                return;
            }
            SendVideoBuffer(p_desc);
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
                    if (p_desc->pic_type == 1) {
                        SendFlowControlBuffer(IFilter::FlowControl_resume);
                        SendVideoBuffer(p_desc);
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
            if (p_desc->pic_type == 1) {
                SendFlowControlBuffer(IFilter::FlowControl_resume);
                SendVideoBuffer(p_desc);
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


