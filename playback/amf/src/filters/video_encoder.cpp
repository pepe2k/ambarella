
/*
 * video_encoder.cpp
 *
 * History:
 *    2012/02/24 - [Zhi He] re-create file
 * Copyright (C) 2012, Ambarella, Inc.
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
#include "iav_duplex_drv.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
}

#include "am_dsp_if.h"
#include "amdsp_common.h"
#include "video_encoder.h"

//debug related
//#define __dump_preview_data_before_start_encoding__
#ifdef __dump_preview_data_before_start_encoding__
static unsigned AM_INT _dump_index = 0;
#endif

#define MAX_VIDEO_PTS  0x40000000

//-----------------------------------------------------------------------
//
// CVideoEncoderBufferPool
//
//-----------------------------------------------------------------------
void CVideoEncoderBufferPool::OnReleaseBuffer(CBuffer *pBuffer)
{
    //AM_WARNING("tYPE:%d, fd_id:%d, mFlags:%d\n", pBuffer->GetType(), pBuffer->mReserved, pBuffer->mFlags);
    if ((!(pBuffer->mFlags & IAV_ENC_BUFFER)) || (INVALID_FD_ID == pBuffer->mBufferId)) {
        return;
    }
    iav_frame_desc_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.enc_id = pBuffer->mEncId;
    frame.fd_id = pBuffer->mBufferId;

    if (ioctl(iavfd, IAV_IOC_RELEASE_ENCODED_FRAME, &frame) < 0) {
        //remove iav's check, ignore this msg now
        //AM_WARNING("!Just used an expired frame.\n");
    }
    pBuffer->mFlags = 0;
}

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

static AM_INT change_fps_to_hz(AM_UINT fps_q9, AM_UINT *fps_hz, char *fps)
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

static const AM_INT cShift = 16, cyr = 19595, cyg = 38470, cyb = 7471;
static const AM_INT cur = -11059, cug = -21709, cub = 32768;
static const AM_INT cvr = 32768, cvg = -27439, cvb = -5329;

// refer to convertArgbsToYuvs() in YuvImageTest.java
static void _convertArgbToYuv(AM_INT* argbData,AM_U8* yData,AM_U8* uvData, AM_INT width, AM_INT height,
                                                AM_U8* alpha_yData,AM_U8* alpha_uvData)
{
    AM_INT r,g,b,pixelData,alpha;
    AM_U8  y, u, v;

    for (AM_INT row = 0; row < height; ++row) {
        for (AM_INT col = 0; col < width; ++col) {
           AM_INT idx = row * width + col;
           pixelData=argbData[idx];

           alpha=((pixelData & 0xff000000) >> 24);
           r = ((pixelData & 0xff0000) >> 16);
           g = ((pixelData & 0xff00) >> 8);
           b = ((pixelData & 0xff)) ;

           if(alpha!=0xff){
               alpha_yData[idx]=0xff;
           }

           y = (AM_U8) ((cyr* r + cyg * g + cyb * b) >> cShift);
           u = (AM_U8) (((cur * r + cug * g + cub * b) >> cShift) + 128);
           v = (AM_U8) (((cvr * r + cvg * g + cvb * b) >> cShift) + 128);

           yData[idx] =y;
           if ((row & 1) == 0 && (col & 1) == 0) {
               AM_INT offset = row / 2 * width + col / 2 * 2;
               uvData[offset] = u;
               uvData[offset + 1] = v;
               if(alpha!=0xff){
                   alpha_uvData[offset]=0xff;
                   alpha_uvData[offset + 1]=0xff;
               }
           }
        }
    }
}

static void _convertOSDInputDataCLUT(SOSDArea_CLUT* osd_area, AM_U8* data, AM_U32* p_input_clut, AM_INT width, AM_INT height, AM_U8 transparent_index, IParameters::OSDInputDataFormat data_format)
{
    AM_ASSERT(osd_area);
    AM_ASSERT(data);
    AM_ASSERT(osd_area->p_dsp_data[0]);
    AM_ASSERT(osd_area->p_dsp_data[1]);
    AM_ASSERT(osd_area->p_dsp_clut);
    AM_ASSERT(osd_area->cur_buf_id < IAV_BLEND_AREA_BUFFER_NUMBER);

    if (IParameters::OSDInputDataFormat_YUVA_CLUT == data_format) {
        memcpy(osd_area->p_dsp_data[osd_area->cur_buf_id], data, osd_area->buffer_size);
        if (p_input_clut) {
            memcpy(osd_area->p_dsp_clut, p_input_clut, 256 * 4);
        }
        return;
    } else if (IParameters::OSDInputDataFormat_RGBA == data_format) {

        AM_INT* argbData = (AM_INT*) data;
        AM_INT index = 0, pixel_number = width * height;
        AM_INT r, g, b, pixelData;
        AM_U8  y, u, v;
        AM_ASSERT(0 == (((AM_UINT)data)&0x3));

        for (index = 0; index < pixel_number; ++index) {
            pixelData = argbData[index];

            r = ((pixelData & 0xff0000) >> 16);
            g = ((pixelData & 0xff00) >> 8);
            b = ((pixelData & 0xff)) ;

            if (0x0 != (pixelData & 0xff000000)) {
                y = (AM_U8) ((cyr* r + cyg * g + cyb * b) >> cShift);
                u = (AM_U8) (((cur * r + cug * g + cub * b) >> cShift) + 128);
                v = (AM_U8) (((cvr * r + cvg * g + cvb * b) >> cShift) + 128);
                osd_area->p_dsp_data[osd_area->cur_buf_id][index] = (y & 0xf0) | ((u & 0xc0) >> 4) | ((v & 0xc0) >> 6);
            } else {
                osd_area->p_dsp_data[osd_area->cur_buf_id][index] = transparent_index;
            }
        }

        if (p_input_clut) {
            memcpy(osd_area->p_dsp_clut, p_input_clut, 256 * 4);
        }
        return;
    } else {
        AM_ERROR("BAD input format %d\n", data_format);
    }
}

//-----------------------------------------------------------------------
//
// CVideoEncoder
//
//-----------------------------------------------------------------------


AM_ERR CVideoEncoder::Construct()
{
    AM_UINT i=0;

    AM_ERR err = inherited::Construct();
    if (err != ME_OK)
        return err;
    DSetModuleLogConfig(LogModuleVideoEncoder);

    pthread_mutex_lock(&mpSharedRes->mMutex);
    AM_ASSERT(!mpSharedRes->mbIavInited);
    AM_ASSERT((-1) == mpSharedRes->mIavFd);
    if (mpSharedRes->mbIavInited) {
        AMLOG_WARN("already have iavfd %d, use it.\n", mpSharedRes->mIavFd);
        mIavFd = mpSharedRes->mIavFd;
        AM_ASSERT(mpSharedRes->mbIavInited);
        AM_ASSERT(mIavFd >= 0);
    } else {
        if ((mIavFd = open("/dev/iav", O_RDWR, 0)) < 0) {
            AM_ERROR("/dev/iav");
            return ME_ERROR;
        }
        mbIavFdOwner = true;
        AM_ASSERT(mIavFd > 0);
        mpSharedRes->mIavFd = mIavFd;
        mpSharedRes->mbIavInited = 1;
        AMLOG_INFO("video encoder, open iavfd %d.\n", mIavFd);
    }
    pthread_mutex_unlock(&mpSharedRes->mMutex);

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

    if ((mpMutex = CMutex::Create(false)) == NULL){
        AM_ERROR("CFFMpegMuxer::Construct(), CMutex::Create() fail.\n");
        return ME_NO_MEMORY;;
    }

    mbEnableTransparentColor = 1;
    mReservedTransparentIndex = 1;
    GeneratePresetYCbCr422_YCbCrAlphaCLUT(mPreSetY4Cb2Cr2CLUT, mReservedTransparentIndex, mbEnableTransparentColor);

    return ME_OK;
}

CVideoEncoder::~CVideoEncoder()
{
    AMLOG_DESTRUCTOR("CVideoEncoder::~CVideoEncoder() start.\n");

    for(AM_UINT i = 0; i < mStreamNum; i++) {
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
        AM_ASSERT(mpSimpleEncAPI);
        if (mpSimpleEncAPI) {
            mpSimpleEncAPI->ReleaseEncoder(mDSPIndex);
            DestroySimpleEncAPI(mpSimpleEncAPI);
            mpSimpleEncAPI = NULL;
        }
        AMLOG_DESTRUCTOR("CVideoEncoder::~CVideoEncoder() before close(fd %d).\n", mIavFd);
        if(DSPMode_DuplexLowdelay == mModeConfig.dsp_mode){
            //stop vcap
            //if (ioctl(mIavFd, IAV_IOC_DUPLEX_STOP_VCAP) < 0) {
            //   AM_ERROR("IOC_DUPLEX_STOP_VCAP");
            //}
            //leave duplex mode
            AMLOG_INFO("[dsp mode]: (video encoder), leave duplex mode begin.\n");
            if (ioctl(mIavFd, IAV_IOC_DUPLEX_LEAVE_MODE) < 0) {
                AM_ERROR("IAV_IOC_DUPLEX_LEAVE_MODE");
            }
            AMLOG_INFO("[dsp mode]: (video encoder), leave duplex mode end.\n");
        } else {
            if (::ioctl(mIavFd, IAV_IOC_UNMAP_DSP) < 0) {
               perror("IAV_IOC_UNMAP_DSP");
               AM_ERROR("IAV_IOC_UNMAP_DSP");
            }
        }
        if (!mpSharedRes->not_start_encoding) {
            AMLOG_INFO("before mmap bsb\n");
            if (::ioctl(mIavFd,  IAV_IOC_UNMMAP_BIT_STREAM_BUFFER) < 0) {
                perror("IAV_IOC_UNMMAP_BIT_STREAM_BUFFER");
                AM_ERROR("IAV_IOC_UNMMAP_BIT_STREAM_BUFFER");
            }
            AMLOG_INFO("after mmap bsb\n");
        }
        if (mbIavFdOwner) {
            close(mIavFd);
        }
        mIavFd = -1;
    }

    AMLOG_DESTRUCTOR("CVideoEncoder::~CVideoEncoder() done.\n");
}

void CVideoEncoder::Delete()
{
    AMLOG_DESTRUCTOR("CVideoEncoder::Delete() before clean iav staff, mIavFd %d.\n", mIavFd);
    if (mIavFd >= 0) {
        AM_ASSERT(mpSimpleEncAPI);
        if (mpSimpleEncAPI) {
            mpSimpleEncAPI->ReleaseEncoder(mDSPIndex);
            DestroySimpleEncAPI(mpSimpleEncAPI);
            mpSimpleEncAPI = NULL;
        }
        AMLOG_DESTRUCTOR("CVideoEncoder::~CVideoEncoder() before close(fd %d).\n", mIavFd);
        if(DSPMode_DuplexLowdelay == mModeConfig.dsp_mode) {
            //stop vcap
            //if (ioctl(mIavFd, IAV_IOC_DUPLEX_STOP_VCAP) < 0) {
            //    AM_ERROR("IOC_DUPLEX_STOP_VCAP");
            //}
             //leave duplex mode
             AMLOG_INFO("[dsp mode]: (video encoder), leave duplex mode begin.\n");
             if (ioctl(mIavFd, IAV_IOC_DUPLEX_LEAVE_MODE) < 0) {
                 AM_ERROR("IAV_IOC_DUPLEX_LEAVE_MODE");
             }
             AMLOG_INFO("[dsp mode]: (video encoder), leave duplex mode end.\n");
        } else {
            if (::ioctl(mIavFd, IAV_IOC_UNMAP_DSP) < 0) {
               perror("IAV_IOC_UNMAP_DSP");
               AM_ERROR("IAV_IOC_UNMAP_DSP");
            }
        }
        if (!mpSharedRes->not_start_encoding) {
            AMLOG_INFO("before mmap bsb\n");
            if (::ioctl(mIavFd,  IAV_IOC_UNMMAP_BIT_STREAM_BUFFER) < 0) {
                perror("IAV_IOC_UNMMAP_BIT_STREAM_BUFFER");
                AM_ERROR("IAV_IOC_UNMMAP_BIT_STREAM_BUFFER");
            }
            AMLOG_INFO("after mmap bsb\n");
        }
        if (mbIavFdOwner) {
            close(mIavFd);
        }
        mIavFd = -1;
    }
    AMLOG_DESTRUCTOR("CVideoEncoder::Delete() done.\n");
    AM_DELETE(mpMutex);
    mpMutex = NULL;
    return inherited::Delete();
}

AM_ERR CVideoEncoder::SetModeConfig(SEncodingModeConfig* pconfig)
{
    //need parameters check, todo?
    AM_ERROR("please add check code here, mode %d.\n", pconfig->dsp_mode);

    memcpy((void*)&mModeConfig, (void*)pconfig, sizeof(SEncodingModeConfig));
    return ME_OK;
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

            if(mpOutputPin[i]->mbSkip == true && mpOutputPin[i]->msState == CVideoEncoderOutput::VideoEncOPin_Paused){
                mpOutputPin[i]->mbSkip = false;
            }
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
    } else {
        AM_ERROR("BAD format %d(not H264).\n", format);
        mConfig.stream_info[index].stream_format = IAV_ENCODE_H264;
    }

    mConfig.stream_info[index].requested = 1;
    mConfig.stream_info[index].width = param->video.pic_width;
    mConfig.stream_info[index].height = param->video.pic_height;
    mConfig.stream_info[index].offset_x = param->video.pic_offset_x;
    mConfig.stream_info[index].offset_y = param->video.pic_offset_y;

    mConfig.stream_info[index].M = param->video.M;
    mConfig.stream_info[index].N = param->video.N;
    mConfig.stream_info[index].IDRInterval = param->video.IDRInterval;
    AMLOG_INFO(" entropy_type %d.\n", mConfig.stream_info[index].entropy_type);
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

    //copy parameters to duplex config
    {
        mModeConfig.enc_width = param->video.pic_width;
        mModeConfig.enc_height = param->video.pic_height;
        mModeConfig.enc_offset_x = param->video.pic_offset_x;
        mModeConfig.enc_offset_y = param->video.pic_offset_y;
        mModeConfig.video_bitrate = param->video.bitrate;
        mModeConfig.M = param->video.M;
        mModeConfig.N = param->video.N;
        mModeConfig.IDRInterval = param->video.IDRInterval;
        if(mConfig.stream_info[index].entropy_type == IAV_ENTROPY_CABAC){
            mModeConfig.entropy_type = EntropyType_H264_CABAC;
        }else{
            mModeConfig.entropy_type = EntropyType_H264_CAVLC;
        }
    }
    return ME_OK;
}

bool CVideoEncoder::isVideoSettingMatched(iav_enc_info_t& enc_info)
{
    bool matched = true;

    //main stream should be enabled
    AM_ASSERT(mConfig.stream_info[0].requested);
    if (mConfig.stream_info[0].requested) {
        //check main stream
        if ((enc_info.main_img.width != (u16)mConfig.stream_info[0].width) || (enc_info.main_img.height != (u16)mConfig.stream_info[0].height)) {
            matched = false;
        }
    } else {
        //restore iav's current setting
        AMLOG_WARN("Restore iav's settings: width %d, height %d, (ori) w %d, h %d.\n", enc_info.main_img.width, enc_info.main_img.height, mConfig.stream_info[0].width, mConfig.stream_info[0].height);
        mConfig.stream_info[0].width = enc_info.main_img.width;
        mConfig.stream_info[0].height = enc_info.main_img.height;
    }

    if (mConfig.stream_info[1].requested) {
        //not check second is extractly matched
        if ((enc_info.second_img.width != (u16)mConfig.stream_info[1].width) || (enc_info.second_img.height != (u16)mConfig.stream_info[1].height)) {
            matched = false;
        }
    } else {
        //restore iav's current setting
        AMLOG_WARN("Restore iav's settings: width %d, height %d, (ori) w %d, h %d.\n", enc_info.second_img.width, enc_info.second_img.height, mConfig.stream_info[1].width, mConfig.stream_info[1].height);
        mConfig.stream_info[1].width = enc_info.second_img.width;
        mConfig.stream_info[1].height = enc_info.second_img.height;
    }

    AMLOG_INFO("isVideoSettingMatched, current video encoding settings: %dx%d,%dx%d.\n", enc_info.main_img.width, enc_info.main_img.height, enc_info.second_img.width, enc_info.second_img.height);
    AMLOG_INFO("isVideoSettingMatched, request video encoding settings: %dx%d,%dx%d.\n", (u16)mConfig.stream_info[0].width, (u16)mConfig.stream_info[0].height, (u16)mConfig.stream_info[1].width, (u16)mConfig.stream_info[1].height);

    if (true == matched) {
        AMLOG_INFO("isVideoSettingMatched matched.\n");
        return true;
    }

    //check second stream's size
    if (mConfig.stream_info[1].width > mConfig.stream_info[0].width || mConfig.stream_info[1].height > mConfig.stream_info[0].height) {
        AMLOG_WARN("The second stream's size should be smaller than that of main stream!\n");
        mConfig.stream_info[1].width = mConfig.stream_info[0].width;
        mConfig.stream_info[1].height = mConfig.stream_info[0].height;
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
AM_ERR CVideoEncoder::SetConfigInCameraRecordMode()
{
    iav_state_info_t info;
    iav_enc_mode_config_t mode;
    iav_enc_info_t enc_info;
    AM_UINT tot_bitrate = 0;
    AM_INT ret;

    AM_ASSERT(!mpSimpleEncAPI);
    mpSimpleEncAPI = CreateSimpleEncAPI(mIavFd, DSPMode_CameraRecording, UDEC_H264, NULL);
    if (!mpSimpleEncAPI) {
        AM_ERROR("CreateSimpleEncAPI fail.\n");
        return ME_OS_ERROR;
    }

    iav_mmap_info_t mmap;
    if ((ret = ::ioctl(mIavFd, IAV_IOC_MAP_DSP, &mmap)) < 0) {
        AM_PERROR("IAV_IOC_MAP_DSP");
        AM_ERROR("IAV_IOC_MAP_DSP ERROR ret %d.\n", ret);
        return ME_OS_ERROR;
    }

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
            AMLOG_WARN("video===>%dx%d,%dx%d matched.\n", enc_info.main_img.width, enc_info.main_img.height, enc_info.second_img.width, enc_info.second_img.height);
        } else {
            AMLOG_WARN("video===>%dx%d,%dx%d.\n", enc_info.main_img.width, enc_info.main_img.height, enc_info.second_img.width, enc_info.second_img.height);

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
            // 1: disable WARP and MCTF, default to 0
            enc_info.vcap_ppl_type = mModeConfig.vcap_ppl_type;
            if(enc_info.vcap_ppl_type == 1){
                 AMLOG_WARN("enc_info.vcap_ppl_type %d, no WARP and MCTF!\n", enc_info.vcap_ppl_type);
            }
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
            if (IAV_STATE_DUPLEX == info.state) {
                AMLOG_WARN("[dsp mode]: (video encoder), try camerarecording, but dsp is in Duplex mode.\n");
                return ME_TRYNEXT;
            } else {
                AM_ERROR("Wrong iav state %d, deny access.\n", info.state);
                return ME_OS_ERROR;
            }
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
        memset(&mode, 0x0, sizeof(mode));
        mode.keep_mode = 0;
        if (mpSharedRes->encoding_mode_config.dsp_piv_enabled) {
            AMLOG_WARN("enable PIV featrue!\n");
            mode.enable_piv = 1;
        }
        if (mConfig.stream_info[1].requested) {
            AMLOG_INFO("request second stream encoding.\n");
            mode.enable_lowres_encoding = 1;
        } else {
            AMLOG_INFO("not request second stream encoding.\n");
            mode.enable_lowres_encoding = 0;
        }
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

        // 1: disable WARP and MCTF, default to 0
        enc_info.vcap_ppl_type = mModeConfig.vcap_ppl_type;
        if(enc_info.vcap_ppl_type == 1){
            AMLOG_WARN("enc_info.vcap_ppl_type %d, no WARP and MCTF!\n", enc_info.vcap_ppl_type);
        }

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

    return ME_OK;
}

AM_ERR CVideoEncoder::enterDuplexMode(void)
{
    iav_duplex_mode_t mode;
    AM_INT ret = 0;

    memset(&mode, 0, sizeof(mode));

    mode.num_of_enc_chans = mModeConfig.num_of_enc_chans;
    AM_ASSERT(1 == mode.num_of_enc_chans);
    mode.num_of_dec_chans = mModeConfig.num_of_dec_chans;
    AM_ASSERT(1 == mode.num_of_dec_chans);
    //hard code here, to do
    mModeConfig.num_of_enc_chans = 1;
    mModeConfig.num_of_dec_chans = 1;

    AM_ASSERT(!mModeConfig.playback_in_pip);
    if (eVoutLCD == mModeConfig.playback_vout_index) {
        mode.vout_mask = (1 << eVoutLCD);
        AM_ASSERT(!mModeConfig.preview_enabled);
    } else if (eVoutHDMI == mModeConfig.playback_vout_index) {
        mode.vout_mask = (1 << eVoutHDMI);
    }
    AMLOG_WARN("mode.vout_mask 0x%x, mModeConfig.playback_vout_index %d.\n", mode.vout_mask, mModeConfig.playback_vout_index);

    mode.input_config.specifided = 1;
    mode.input_config.preview_vout_index = mModeConfig.preview_vout_index;
    mode.input_config.preview_alpha = mModeConfig.preview_alpha;
    mode.input_config.vout_index = mModeConfig.playback_vout_index;

    mode.input_config.main_width = mModeConfig.main_win_width;
    mode.input_config.main_height = mModeConfig.main_win_height;

    mode.input_config.enc_left = mModeConfig.enc_offset_x;
    mode.input_config.enc_top = mModeConfig.enc_offset_y;
    mode.input_config.enc_width = mModeConfig.enc_width;
    mode.input_config.enc_height = mModeConfig.enc_height;

    mode.input_config.preview_left = mModeConfig.preview_left;
    mode.input_config.preview_top = mModeConfig.preview_top;
    mode.input_config.preview_width = mModeConfig.preview_width;
    mode.input_config.preview_height = mModeConfig.preview_height;

    mode.input_config.pb_display_left = mModeConfig.pb_display_left;
    mode.input_config.pb_display_top = mModeConfig.pb_display_top;
    mode.input_config.pb_display_width = mModeConfig.pb_display_width;
    mode.input_config.pb_display_height = mModeConfig.pb_display_height;

    mode.input_config.pb_display_enabled = mModeConfig.pb_display_enabled;
    mode.input_config.preview_display_enabled = mModeConfig.preview_enabled;
    mode.input_config.preview_in_pip = mModeConfig.preview_in_pip;
    mode.input_config.pb_display_in_pip = mModeConfig.playback_in_pip;

    //previewC related
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].enable = mModeConfig.previewc_rawdata_enabled;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].use_preview_buffer_id = DSP_PREVIEW_ID_C;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].scaled_width = mModeConfig.previewc_scaled_width;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].scaled_height = mModeConfig.previewc_scaled_height;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].crop_offset_x = mModeConfig.previewc_crop_offset_x;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].crop_offset_y = mModeConfig.previewc_crop_offset_y;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].crop_width = mModeConfig.previewc_crop_width;
    mode.input_config.rawdata_config[DSP_PREVIEW_ID_C].crop_height = mModeConfig.previewc_crop_height;

    AMLOG_INFO("[dsp mode]: (video encoder), enter duplex mode begin.\n");
    if ((ret = ioctl(mIavFd, IAV_IOC_DUPLEX_ENTER_MODE, &mode)) < 0) {
        perror("IAV_IOC_DUPLEX_ENTER_MODE");
        AM_ERROR("IAV_IOC_DUPLEX_ENTER_MODE fail, ret %d.\n", ret);
        return ME_OS_ERROR;
    }
    AMLOG_INFO("[dsp mode]: (video encoder), enter duplex mode done.\n");

    //store back arguments, need mutex?
    pthread_mutex_lock(&mpSharedRes->mMutex);
    StoreBackDuplexSettings(&mModeConfig, &mode.input_config);
    mpSharedRes->encoding_mode_config = mModeConfig;
    pthread_mutex_unlock(&mpSharedRes->mMutex);

    iav_duplex_start_vcap_t vcap;
    memset(&vcap, 0, sizeof(vcap));

    if ((ret = ioctl(mIavFd, IAV_IOC_DUPLEX_START_VCAP, &vcap)) < 0) {
        perror("IAV_IOC_DUPLEX_START_VCAP");
        AM_ERROR("IAV_IOC_DUPLEX_START_VCAP, return %d.\n", ret);
        return ME_OS_ERROR;
    }

    return ME_OK;
}

AM_ERR CVideoEncoder::SetConfigInDuplexMode()
{
    iav_state_info_t info;

    AM_ASSERT(!mpSimpleEncAPI);
    mpSimpleEncAPI = CreateSimpleEncAPI(mIavFd, DSPMode_DuplexLowdelay, UDEC_H264, NULL);
    if (!mpSimpleEncAPI) {
        AM_ERROR("CreateSimpleEncAPI fail.\n");
        return ME_OS_ERROR;
    }

    memset(&info, 0, sizeof(iav_state_info_t));
    if (::ioctl(mIavFd, IAV_IOC_GET_STATE_INFO, &info) < 0) {
        AM_PERROR("IAV_IOC_GET_STATE_INFO\n");
        AM_ERROR("Fatal error: IAV_IOC_GET_STATE_INFO fail.\n");
        return ME_ERROR;
    }

    if(info.state == IAV_STATE_DUPLEX) {
        //need do nothing about enter idle, initvin, enter duplex mode
        AMLOG_INFO("***Init state is IAV_STATE_DUPLEX, not init vin, and enter encoding mode.\n");
        return enterDuplexMode();
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

        return enterDuplexMode();
    }

    return ME_OK;
}

void CVideoEncoder::DoStop()
{
    AM_ASSERT(mpSimpleEncAPI);
    if (mpSimpleEncAPI) {
        mpSimpleEncAPI->Stop(mDSPIndex, 0);
    }
    mbStop = true;
}

AM_ERR CVideoEncoder::ExitPreview()
{
    //this api should only be called by uint test(rectest/dutest)
    if ((DSPMode_CameraRecording == mModeConfig.dsp_mode)) {
        AMLOG_WARN("CVideoEncoder::ExitPreview() start, mIavFd %d, mbStop %d, mbRun %d.\n", mIavFd, mbStop, mbRun);
        if((::ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0)) < 0) {
            AMLOG_ERROR("UDEC enter IDLE mode fail.\n");
        } else {
            AMLOG_INFO("enter IDLE mode done.\n");
        }
        AMLOG_WARN("CVideoEncoder::ExitPreview() done.\n");
    }
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

AM_ERR CVideoEncoder::initEncoder(AM_UINT dspmode)
{
    AM_UINT tot_bitrate = 0;
    AM_INT ret;
    SEncoderParam param;

    param.codec_type = UDEC_H264;
    param.dsp_mode = dspmode;
    if (DSPMode_CameraRecording == dspmode) {
        param.main_width = mConfig.main_width;
        param.main_height = mConfig.main_height;
        param.enc_width = mConfig.stream_info[0].width;
        param.enc_height = mConfig.stream_info[0].height;
        param.enc_offset_x = mConfig.stream_info[0].offset_x;
        param.enc_offset_y = mConfig.stream_info[0].offset_y;
        param.bitrate = mConfig.stream_info[0].average_bitrate;
        param.M = mConfig.stream_info[0].M;
        param.N = mConfig.stream_info[0].N;
        param.use_cabac = (mConfig.stream_info[0].entropy_type == IAV_ENTROPY_CABAC);
        param.idr_interval = mConfig.stream_info[0].IDRInterval;
    } else if (DSPMode_DuplexLowdelay == dspmode) {
        param.main_width = mModeConfig.main_win_width;
        param.main_height = mModeConfig.main_win_height;
        param.enc_width = mModeConfig.enc_width;
        param.enc_height = mModeConfig.enc_height;
        param.enc_offset_x = mModeConfig.enc_offset_x;
        param.enc_offset_y = mModeConfig.enc_offset_y;
        param.bitrate = mModeConfig.video_bitrate;
        param.M = mModeConfig.M;
        param.N = mModeConfig.N;
        param.use_cabac = (mModeConfig.entropy_type == EntropyType_H264_CABAC);
        param.idr_interval = mModeConfig.IDRInterval;
    } else {
        AM_ERROR("NOT supported dsp mode %d.\n", dspmode);
        return ME_NOT_SUPPORTED;
    }

    AMLOG_INFO("[enc parameters]: (dsp mode %d): main win %dx%d, main enc size %dx%d, offset %dx%d, bitrate %d, M N Idr interval %d %d %d, use cabac %d.\n", dspmode,
        param.main_width, param.main_height, param.enc_width, param.enc_height, param.enc_offset_x, param.enc_offset_y, param.bitrate, param.M, param.N, param.idr_interval, param.use_cabac);

    param.profile = mConfig.profile;
    param.level = mConfig.level;
    param.gop_structure = mConfig.stream_info[0].gop_model;
    param.numRef_P = 1;//default value
    param.numRef_B = 2;//default value
    param.framerate_num = IParameters::TimeUnitDen_90khz;//hard code here
    param.framerate_den = IParameters::TimeUnitNum_fps29dot97;//hard code here

    param.quality_level = 0x83;//hard code here
    param.vbr_setting = 1;//hard code here, CBR
    param.calibration = mConfig.stream_info[0].calibration;
    param.vbr_ness = mConfig.stream_info[0].vbr_ness;
    param.min_vbr_rate_factor = mConfig.stream_info[0].min_vbr_rate_factor;
    param.max_vbr_rate_factor = mConfig.stream_info[0].max_vbr_rate_factor;

    if (DSPMode_CameraRecording == dspmode) {
        tot_bitrate = mConfig.stream_info[0].average_bitrate;

        //check second stream
        if (mStreamNum > 1) {
            param.second_stream_enabled = 1;
            tot_bitrate += mConfig.stream_info[1].average_bitrate;
        } else {
            param.second_stream_enabled = 0;
        }
        param.second_bitrate = mConfig.stream_info[1].average_bitrate;
        param.second_enc_width = mConfig.stream_info[1].width;
        param.second_enc_height = mConfig.stream_info[1].height;
        param.second_enc_offset_x = mConfig.stream_info[1].offset_x;
        param.second_enc_offset_y = mConfig.stream_info[1].offset_y;

        param.dsp_piv_enabled = mModeConfig.dsp_piv_enabled;
        //param.dsp_jpeg_active_win_w = mModeConfig.main_win_width;
        //param.dsp_jpeg_active_win_h = mModeConfig.main_win_height;
        param.dsp_jpeg_active_win_w = mModeConfig.dsp_jpeg_width;
        param.dsp_jpeg_active_win_h = mModeConfig.dsp_jpeg_height;
        param.dsp_jpeg_dram_w = mModeConfig.dsp_jpeg_width;
        param.dsp_jpeg_dram_h = mModeConfig.dsp_jpeg_height;

        AMLOG_INFO("active win w %d, h %d, dram jpeg w %d, h %d.\n", param.dsp_jpeg_active_win_w, param.dsp_jpeg_active_win_h, param.dsp_jpeg_dram_w, param.dsp_jpeg_dram_h);

        //estimate expire time
        AM_ASSERT(tot_bitrate);
        //hard code here, BSB is 12M now
        //use 12M *1/2, similar with driver, driver's dual stream 's descriptor number is same with single stream, so choose 1/2
        mFrameLifeTime = (((AM_U64)IParameters::TimeUnitDen_90khz)*(12*1024*1024)*8)/(tot_bitrate);
        mFrameLifeTime /= 2;

        AM_ASSERT(mFrameLifeTime > (IParameters::TimeUnitDen_90khz));//greater than 1 second
        AMLOG_INFO("mFrameLifeTime %u, tot_bitrate %u.\n", mFrameLifeTime, tot_bitrate);

#ifdef __dump_preview_data_before_start_encoding__
        iav_img_buf_t img_buf;
        memset(&img_buf, 0, sizeof(img_buf));
        img_buf.buffer_id= IAV_BUFFER_ID_PREVIEW_C;

        if (ioctl(mIavFd, IAV_IOC_GET_PREVIEW_BUFFER, &img_buf) < 0) {
            AM_ERROR("IAV_IOC_GET_PREVIEW_BUFFER\n");
            perror("IAV_IOC_GET_PREVIEW_BUFFER");
            return ME_OS_ERROR;
        }

        AM_DumpBinaryFile_withIndex("/sdcard/win/raw/previewc_y", _dump_index, (AM_U8*)img_buf.luma_addr, img_buf.buf_pitch*img_buf.buf_height);
        AM_DumpBinaryFile_withIndex("/sdcard/win/raw/previewc_uv", _dump_index, (AM_U8*)img_buf.chroma_addr, (img_buf.buf_pitch*img_buf.buf_height)/2);
        _dump_index++;
#endif

    } else if (DSPMode_DuplexLowdelay == dspmode) {
        param.second_stream_enabled = 0; //duplex not support dual stream now
        tot_bitrate = param.bitrate;
        //estimate expire time
        AM_ASSERT(tot_bitrate);
        //hard code here, BSB is 6M now
        //use 6M
        mFrameLifeTime = (((AM_U64)IParameters::TimeUnitDen_90khz)*(6*1024*1024)*8)/(tot_bitrate);

        AM_ASSERT(mFrameLifeTime > (IParameters::TimeUnitDen_90khz));//greater than 1 second
        AMLOG_INFO("mFrameLifeTime %u, tot_bitrate %u.\n", mFrameLifeTime, tot_bitrate);
    } else {
        AM_ERROR("NOT supported dsp mode %d.\n", dspmode);
        return ME_NOT_SUPPORTED;
    }

    AMLOG_INFO("[flow]: before mpSimpleEncAPI->InitEncoder mDSPIndex %d.\n", mDSPIndex);
    ret= mpSimpleEncAPI->InitEncoder(mDSPIndex, &param);
    if (ME_OK != ret) {
        AM_ERROR("mpSimpleEncAPI->InitEncoder fail, ret %d, mDSPIndex %d.\n", ret, mDSPIndex);
        return ME_ERROR;
    }

    AMLOG_INFO("before mmap bsb\n");
    if (::ioctl(mIavFd,  IAV_IOC_MMAP_BIT_STREAM_BUFFER) < 0) {
        perror("IAV_IOC_MMAP_BIT_STREAM_BUFFER");
        AM_ERROR("IAV_IOC_MMAP_BIT_STREAM_BUFFER");
    }
    AMLOG_INFO("after mmap bsb\n");

    AMLOG_INFO("[flow]: before mpSimpleEncAPI->Start mDSPIndex %d.\n", mDSPIndex);
    ret= mpSimpleEncAPI->Start(mDSPIndex);
    if (ME_OK != ret) {
        AM_ERROR("mpSimpleEncAPI->Start fail, ret %d, mDSPIndex %d.\n", ret, mDSPIndex);
        return ME_ERROR;
    }
    AMLOG_INFO("[flow]: after mpSimpleEncAPI->Start mDSPIndex %d.\n", mDSPIndex);

    return ME_OK;
}

AM_ERR CVideoEncoder::setupVideoEncoder()
{
    AM_ERR err;
    if ((DSPMode_CameraRecording != mModeConfig.dsp_mode) && (DSPMode_DuplexLowdelay != mModeConfig.dsp_mode)) {
        AM_ERROR("BAD dsp mode(%d) request in video encoder, use camera recoding as default.\n", mModeConfig.dsp_mode);
        mModeConfig.dsp_mode = DSPMode_CameraRecording;
    } else {
        AMLOG_WARN("setupVideoEncoder, request mode is %d.\n", mModeConfig.dsp_mode);
    }

    if (DSPMode_CameraRecording == mModeConfig.dsp_mode) {
        err = SetConfigInCameraRecordMode();
        if (ME_TRYNEXT == err) {
            mModeConfig.dsp_mode = DSPMode_DuplexLowdelay;
            err = SetConfigInDuplexMode();
            if (ME_OK != err) {
                AM_ERROR("try Duplex also failed, SetConfigInDuplexMode return %d.\n", err);
                return ME_OS_ERROR;
            }
        } else if (ME_OK != err) {
            AM_ERROR("SetConfigInCameraRecordMode Failed, ret %d\n", err);
            return ME_OS_ERROR;
        }
    } else if (DSPMode_DuplexLowdelay == mModeConfig.dsp_mode) {
        err = SetConfigInDuplexMode();
        if (ME_TRYNEXT == err) {
            mModeConfig.dsp_mode = DSPMode_CameraRecording;
            err = SetConfigInCameraRecordMode();
            if (ME_OK != err) {
                AM_ERROR("try CameraRecording mode also failed, SetConfigInCameraRecordMode return %d.\n", err);
                return ME_OS_ERROR;
            }
        } else if (ME_OK != err) {
            AM_ERROR("SetConfigInDuplexMode Failed, return %d\n", err);
            return ME_OS_ERROR;
        }
    } else {
        AM_ERROR("BAD dsp mode(%d), should not comets here.\n", mModeConfig.dsp_mode);
        return ME_BAD_PARAM;
    }

    if (!mpSharedRes->not_start_encoding) {
        return initEncoder(mModeConfig.dsp_mode);
    } else {
        AM_ERROR("[debug mode]: not start encoding.\n");
        return ME_DEBUG;
    }
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
    //CmdAck(ME_OK);
    CQueue::QType type;
    CQueue::WaitResult result;
    CMD cmd;
    AM_ERR err;

#ifdef __print_time_info__
    struct timeval tv_loop_last, tv_loop_current;
#endif

    err = setupVideoEncoder();
    if ((err != ME_OK) && (err != ME_DEBUG)) {
        AM_ERROR("setupVideoEncoder fail.\n");
        PostEngineMsg(IEngine::MSG_OS_ERROR);
        msState = STATE_ERROR;
    } else if (err == ME_DEBUG) {
        AMLOG_WARN("[debug]: not start dsp encoding.\n");
        msState = STATE_ERROR;
    } else {
        AMLOG_INFO("[flow]: setupVideoEncoder() ret OK.\n");
    }
    CmdAck(ME_OK);

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

        switch (msState) {

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

                err = ReadInputData();
                if (err == ME_OK) {
                    msState = STATE_READY;
                } else if (err == ME_CLOSED) {
                    AMLOG_INFO("[VE] all buffers comes out, send EOS.\n");
                    ProcessEOS();
                    mbRun = false;
                    msState = STATE_PENDING;
                } else if (err == ME_OS_ERROR) {
                    AM_ERROR("IAV Error, goto Error state.\n");
                    PostEngineMsg(IEngine::MSG_OS_ERROR);
                    msState = STATE_ERROR;
                } else if (err == ME_ERROR) {
                    AM_ERROR("How to handle this case?\n");
                    msState = STATE_ERROR;
                } else {
                    AM_ERROR("Not supposed return value, %d.\n", err);
                    msState = STATE_ERROR;
                }
                break;

            case STATE_READY:
                if (ProcessBuffer() == ME_OK) {
                    msState = STATE_IDLE;
                } else {
                    msState = STATE_ERROR;
                }
                break;

            case STATE_PENDING:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                //EOS
                if (cmd.code == CMD_FLOW_CONTROL && ((FlowControlType)cmd.flag == FlowControl_eos)) {
                    ProcessEOS();
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
    AM_ERR err;
    AM_UINT i = 0;

    AM_ASSERT(mpSimpleEncAPI);
    err = mpSimpleEncAPI->GetBitStreamBuffer(mDSPIndex, &mBitstreamDesc);
    AM_ASSERT(ME_OK == err);

#ifdef WOCHACHA
    for(i = 0; i < mBitstreamDesc.tot_desc_number; i ++){
        if(mBitstreamDesc.desc[i].pic_type == PredefinedPictureType_IDR){
            AM_MSG msg;
            msg.code = IEngine::MSG_EVENT_NOTIFICATION;
            msg.p1 = (AM_INTPTR)static_cast<IFilter*>(this);
            msg.p2 = mBitstreamDesc.desc[i].enc_id;
            //msg.p3 = MSG_TYPE_IDR;
            PostEngineMsg(msg);
        }
    }
#endif

    if (mpClockManager) {
        mLastFrameExpiredTime = mpClockManager->GetCurrentTime() + mFrameLifeTime;
    }

    if (ME_OK != err) {
        if(mbStop == true) {
            AMLOG_INFO(" [VideoEncoder]: ioctl error, CLOSED\n");
            return ME_CLOSED;
        } else {
            AM_ERROR("GetBitStreamBuffer error, err %d\n", err);
            return err;
        }
    }

    if (mBitstreamDesc.status & DFlagLastFrame) {
        AMLOG_INFO(" [VideoEncoder]: last frame flag, CLOSED\n");
        return ME_CLOSED;
    }

    //check pts
    if (!mbGetFirstPTS) {
        mbGetFirstPTS = true;
        if (mBitstreamDesc.desc[0].pts > mMaxStartPTS) {
            mStartPTSOffset = mBitstreamDesc.desc[0].pts - mMaxStartPTS;
        }
        AMLOG_INFO("First PTS from dsp is %llu, mStartPTSOffset %llu.\n", mBitstreamDesc.desc[0].pts, mStartPTSOffset);
        for (i = 0; i < mStreamNum; i++) {
            mpOutputPin[i]->mStartPTSOffset = mStartPTSOffset;
            mpOutputPin[i]->mLastPTS = 0;
            mpOutputPin[i]->mPTSLoop = 0;
            mpOutputPin[i]->mLoopPTSOffset = 0;
        }
    }

    return ME_OK;
}

AM_ERR CVideoEncoder::ProcessBuffer()
{
    AMLOG_DEBUG("ProcessBuffer tot_desc_number %d.\n", mBitstreamDesc.tot_desc_number);
    AM_UINT i = 0;
    for (i = 0; i < mBitstreamDesc.tot_desc_number; i ++) {
        AM_ASSERT(mBitstreamDesc.desc[i].enc_id < mStreamNum);
        AMLOG_DEBUG("desc(%d) enc_id %d, pstart %p, size %d, pic_type %d, fb_id %d, num %d, pts %llu.\n", i, mBitstreamDesc.desc[i].enc_id, mBitstreamDesc.desc[i].pstart, mBitstreamDesc.desc[i].size, mBitstreamDesc.desc[i].pic_type, mBitstreamDesc.desc[i].fb_id, mBitstreamDesc.desc[i].frame_number, mBitstreamDesc.desc[i].pts);

        if (mBitstreamDesc.desc[i].enc_id < mStreamNum) {
            if (mpOutputPin[mBitstreamDesc.desc[i].enc_id] && (false == mpOutputPin[mBitstreamDesc.desc[i].enc_id]->mbSkip)) {
                mpOutputPin[mBitstreamDesc.desc[i].enc_id]->ProcessBuffer(&mBitstreamDesc.desc[i], mLastFrameExpiredTime);
            } else {
                AMLOG_INFO("discard frame, because NULL pointer (%p), or mbSkip %d.\n", mpOutputPin[mBitstreamDesc.desc[i].enc_id], mpOutputPin[mBitstreamDesc.desc[i].enc_id]->mbSkip);
            }
        } else if (2 == mBitstreamDesc.desc[i].enc_id) {
            AMLOG_ERROR("debug log: jpeg from dsp....\n");
            AM_DumpBinaryFile(mJpegFileName, mBitstreamDesc.desc[i].pstart, mBitstreamDesc.desc[i].size);
            iav_frame_desc_t frame;
            memset(&frame, 0, sizeof(frame));
            frame.enc_id = mBitstreamDesc.desc[i].enc_id;
            frame.fd_id = mBitstreamDesc.desc[i].fb_id;

            if (ioctl(mIavFd, IAV_IOC_RELEASE_ENCODED_FRAME, &frame) < 0) {
                //remove iav's check, ignore this msg now
                //AM_WARNING("!Just used an expired frame.\n");
            }
        } else if (3 == mBitstreamDesc.desc[i].enc_id) {
            AMLOG_ERROR("debug log: mjpeg from dsp....\n");
            AM_AppendtoBinaryFile("test.mjpeg", mBitstreamDesc.desc[i].pstart, mBitstreamDesc.desc[i].size);
            iav_frame_desc_t frame;
            memset(&frame, 0, sizeof(frame));
            frame.enc_id = mBitstreamDesc.desc[i].enc_id;
            frame.fd_id = mBitstreamDesc.desc[i].fb_id;

            if (ioctl(mIavFd, IAV_IOC_RELEASE_ENCODED_FRAME, &frame) < 0) {
                //remove iav's check, ignore this msg now
                //AM_WARNING("!Just used an expired frame.\n");
            }
        } else {
            AMLOG_ERROR("mBitstreamDesc.desc[i].enc_id %d exceed max value %d.\n", mBitstreamDesc.desc[i].enc_id, mStreamNum);
        }
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
        //mVinSource = 0;
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

    //select camera
    mVinSource = mpSharedRes->select_camera_index;
    AMLOG_INFO("select camera %d\n", mVinSource);

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

    if (ioctl(mIavFd, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
        AM_PERROR("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
        return ME_OS_ERROR;
    }

    if (src_info.dev_type != AMBA_VIN_SRC_DEV_TYPE_DECODER) {
        if (mVinFrameRate) {
            if (ioctl(mIavFd, IAV_IOC_VIN_SRC_SET_FRAME_RATE, mVinFrameRate) < 0) {
                AM_PERROR("IAV_IOC_VIN_SRC_SET_FRAME_RATE");
                return ME_OS_ERROR;
            }
        }
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

bool CVideoEncoder::findAvaiableArea(AM_INT& i)
{
    for (i = 0; i < DMAX_OSD_AREA_NUM; i ++) {
        if (!mOsdArea[i].inited) {
            return true;
        }
    }

    return false;
}

AM_INT CVideoEncoder::AddOsdBlendArea(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height)
{
    AM_INT ret_index = 0;
    AM_INT ret = 0;
    AM_UINT remainig_size = 0;

    AUTO_LOCK(mpMutex);

    AMLOG_INFO("CVideoEncoder::addOsdBlendArea, x=%d, y=%d, size=%dx%d.\n", xPos, yPos, width, height);

    if (EOSD_MODE_INIT == mOSDMode) {
        mOSDMode = EOSD_MODE_YCbCrRaw;
    } else if (EOSD_MODE_YCbCrRaw != mOSDMode) {
        AM_ERROR("CVideoEncoder::AddOsdBlendArea , bad osd mode %d.\n", mOSDMode);
        return ME_BAD_PARAM;
    }

    if (mnOsdAreaNumber >= DMAX_OSD_AREA_NUM) {
        AM_ERROR("CVideoEncoder::addOsdBlendArea: mnOSDAreaNumber %d reach system limit.\n", mnOsdAreaNumber);
        return (-1);
    }

    if ((width <= 0) || (height <= 0)) {
        AM_ERROR("CVideoEncoder::addOsdBlendArea: BAD width %d height %d.\n", width, height);
        return (-2);
    }

    if (DMAX_OSD_BUFFER_MAX_SIZE < ((width * height) + mnTotalOsdBufferSize)) {
        AM_ERROR("CVideoEncoder::addOsdBlendArea: width %d height %d too big, request size %d, remainning size %d, (%d - %d).\n", width, height, width*height, DMAX_OSD_BUFFER_MAX_SIZE - mnTotalOsdBufferSize, DMAX_OSD_BUFFER_MAX_SIZE, mnTotalOsdBufferSize);
        return (-3);
    }

    if (false == findAvaiableArea(ret_index)) {
        AM_ERROR("CVideoEncoder::addOsdBlendArea: no avaiable area_id, must have error.\n", width, height);
        return (-4);
    }

    if(mIavFd <= 0){
        AM_ERROR("IAV fd = %d!\n", mIavFd);
        return (-5);
    }
    mOsdBlendInfo[ret_index].enc_id = ret_index;
    mOsdBlendInfo[ret_index].osd_start_x = xPos;
    mOsdBlendInfo[ret_index].osd_start_y = yPos;
    mOsdBlendInfo[ret_index].osd_width = width;
    mOsdBlendInfo[ret_index].osd_height = height;

    ret = ioctl(mIavFd, IAV_IOC_ADD_OSD_BLEND_AREA, &mOsdBlendInfo[ret_index]);
    if(0 != ret){
        AM_ERROR("IAV_IOC_ADD_OSD_BLEND_AREA error, ret %d.\n", ret);
        return (-6);
    }

    mOsdArea[ret_index].inited = 1;

    mOsdArea[ret_index].offset_x = xPos;
    mOsdArea[ret_index].offset_y = yPos;
    mOsdArea[ret_index].max_width = width;
    mOsdArea[ret_index].max_height = height;
    mOsdArea[ret_index].buffer_size = width*height;

    mnOsdAreaNumber ++;
    mnTotalOsdBufferSize += mOsdArea[ret_index].buffer_size;

    mOsdArea[ret_index].p_addr_y = (AM_U8*)mOsdBlendInfo[ret_index].osd_addr_y;
    mOsdArea[ret_index].p_addr_uv = (AM_U8*)mOsdBlendInfo[ret_index].osd_addr_uv;
    mOsdArea[ret_index].p_addr_alpha_y = (AM_U8*)mOsdBlendInfo[ret_index].alpha_addr_y;
    mOsdArea[ret_index].p_addr_alpha_uv = (AM_U8*)mOsdBlendInfo[ret_index].alpha_addr_uv;
    AMLOG_INFO("CVideoEncoder::addOsdBlendArea id(%d) return index %d, data 0x%p, 0x%p, 0x%p, 0x%p.\n", mOsdBlendInfo[ret_index].area_id, ret_index, mOsdArea[ret_index].p_addr_y, mOsdArea[ret_index].p_addr_uv, mOsdArea[ret_index].p_addr_alpha_y, mOsdArea[ret_index].p_addr_uv);

    return ret_index;
}

AM_ERR CVideoEncoder::UpdateOsdBlendArea(AM_INT index, AM_U8* data, AM_INT width, AM_INT height)
{
    AM_INT ret;
    AUTO_LOCK(mpMutex);

    if (EOSD_MODE_YCbCrRaw!= mOSDMode) {
        AM_ERROR("CVideoEncoder::UpdateOsdBlendArea , bad osd mode %d.\n", mOSDMode);
        return ME_BAD_PARAM;
    }

    if (!data || (width <= 0) || (height <= 0)) {
        AM_ERROR("CVideoEncoder::updateOsdBlendArea BAD params index %d, data %p, width %d, height %d.\n", index, data, width, height);
        return ME_BAD_PARAM;
    }

    if ((index >= DMAX_OSD_AREA_NUM) || (index < 0)) {
        AM_ERROR("CVideoEncoder::updateOsdBlendArea %d exceed system max value.\n", index);
        return ME_BAD_PARAM;
    }

    if (!mOsdArea[index].inited) {
        AM_ERROR("CVideoEncoder::updateOsdBlendArea(index %d), it's not initialized yet.\n", index);
        return ME_BAD_PARAM;
    }

    if ((width > mOsdArea[index].max_width) || (height > mOsdArea[index].max_height)) {
        LOGE("updateOsdBlendArea input width(%d)/height(%d) exceed area's %dx%d\n", width, height, mOsdArea[index].max_width, mOsdArea[index].max_height);
        return ME_BAD_PARAM;
    }

    if ((!mOsdArea[index].p_addr_y) || (!mOsdArea[index].p_addr_uv) || (!mOsdArea[index].p_addr_alpha_y) || (!mOsdArea[index].p_addr_alpha_uv)){
        AM_ERROR("CVideoEncoder::updateOsdBlendArea, internal NULL pointer, please check code\n");
        return ME_ERROR;
    }

    _convertArgbToYuv((AM_INT*)data, mOsdArea[index].p_addr_y, mOsdArea[index].p_addr_uv, width, height, mOsdArea[index].p_addr_alpha_y, mOsdArea[index].p_addr_alpha_uv);

    //AMLOG_INFO("CVideoEncoder::updateOsdBlendArea id(%d), index %d, data 0x%p, 0x%p, 0x%p, 0x%p.\n", mOsdBlendInfo.area_id, index, mOsdArea[index].p_addr_y, mOsdArea[index].p_addr_uv, mOsdArea[index].p_addr_alpha_y, mOsdArea[index].p_addr_uv);

    ret = ioctl(mIavFd, IAV_IOC_UPDATE_OSD_BLEND_AREA, mOsdBlendInfo[index].area_id);
    if (0 != ret) {
        AM_ERROR("IAV_IOC_UPDATE_OSD_BLEND_AREA error, ret %d.\n", ret);
        return ME_ERROR;
    }

    return ME_OK;
}

AM_ERR CVideoEncoder::RemoveOsdBlendArea(AM_INT index)
{
    AM_INT ret;
    AUTO_LOCK(mpMutex);

    AMLOG_INFO("CVideoEncoder::removeOsdBlendArea index %d\n", index);

    if (EOSD_MODE_YCbCrRaw!= mOSDMode) {
        AM_ERROR("CVideoEncoder::RemoveOsdBlendArea , bad osd mode %d.\n", mOSDMode);
        return ME_BAD_PARAM;
    }

    if ((index >= DMAX_OSD_AREA_NUM) || (index < 0)) {
        AM_ERROR("CVideoEncoder::removeOsdBlendArea %d exceed system max value.\n", index);
        return ME_BAD_PARAM;
    }

    if (!mOsdArea[index].inited) {
        AM_ERROR("CVideoEncoder::removeOsdBlendArea(index %d), it's not initialized yet.\n", index);
        return ME_BAD_PARAM;
    }

    mOsdArea[index].inited = 0;
    mnOsdAreaNumber --;
    mnTotalOsdBufferSize -= mOsdArea[index].buffer_size;

    if (mnOsdAreaNumber < 0 || mnTotalOsdBufferSize < 0) {
        LOGE("CVideoEncoder::removeOsdBlendArea(%d), cause Internal error, please check codemnOsdAreaNumber %d, mnTotalOsdBufferSize %d.\n", index, mnOsdAreaNumber, mnTotalOsdBufferSize);
        return ME_ERROR;
    }

    ret = ioctl(mIavFd, IAV_IOC_REMOVE_OSD_BLEND_AREA, mOsdBlendInfo[index].area_id);
    if(0 != ret){
        AMLOG_ERROR("IAV_IOC_REMOVE_OSD_BLEND_AREA error, ret %d.\n", ret);
        return ME_ERROR;
    }
    AMLOG_INFO("Osd area %d, index %d is removed.\n", mOsdBlendInfo[index].area_id, index);

    memset(&mOsdBlendInfo[index], 0, sizeof(iav_osd_blend_info_t));

    return ME_OK;
}

bool CVideoEncoder::findAvaiableAreaCLUT(AM_INT& i)
{
    for (i = 0; i < DMAX_OSD_AREA_NUM; i ++) {
        if (!mOsdAreaCLUT[i].inited) {
            return true;
        }
    }

    return false;
}

AM_INT CVideoEncoder::AddOsdBlendAreaCLUT(AM_INT xPos, AM_INT yPos, AM_INT width, AM_INT height)
{
    AM_INT ret_index = 0;
    AM_INT ret = 0;
    AM_UINT remainig_size = 0;

    AUTO_LOCK(mpMutex);

    AMLOG_INFO("CVideoEncoder::AddOsdBlendAreaCLUT, x=%d, y=%d, size=%dx%d.\n", xPos, yPos, width, height);

    if (EOSD_MODE_INIT == mOSDMode) {
        mOSDMode = EOSD_MODE_YCbCrCLUT;
    } else if (EOSD_MODE_YCbCrCLUT != mOSDMode) {
        AM_ERROR("CVideoEncoder::AddOsdBlendAreaCLUT , bad osd mode %d.\n", mOSDMode);
        return ME_BAD_PARAM;
    }

    if (mnOsdAreaNumberCLUT >= DMAX_OSD_AREA_NUM) {
        AM_ERROR("CVideoEncoder::addOsdBlendArea: mnOSDAreaNumber %d reach system limit.\n", mnOsdAreaNumberCLUT);
        return (-1);
    }

    if ((width <= 0) || (height <= 0)) {
        AM_ERROR("CVideoEncoder::AddOsdBlendAreaCLUT: BAD width %d height %d.\n", width, height);
        return (-2);
    }

    if (DMAX_OSD_BUFFER_MAX_SIZE_CLUT < ((width * height) + mnTotalOsdBufferSizeCLUT)) {
        AM_ERROR("CVideoEncoder::AddOsdBlendAreaCLUT: width %d height %d too big, request size %d, remainning size %d, (%d - %d).\n", width, height, width*height, DMAX_OSD_BUFFER_MAX_SIZE_CLUT - mnTotalOsdBufferSizeCLUT, DMAX_OSD_BUFFER_MAX_SIZE_CLUT, mnTotalOsdBufferSizeCLUT);
        return (-3);
    }

    if (false == findAvaiableAreaCLUT(ret_index)) {
        AM_ERROR("CVideoEncoder::AddOsdBlendAreaCLUT: no avaiable area_id, must have error.\n", width, height);
        return (-4);
    }

    if(mIavFd <= 0){
        AM_ERROR("IAV fd = %d!\n", mIavFd);
        return (-5);
    }

    mOsdBlendInfoCLUT[ret_index].area_id = ret_index;
    mOsdBlendInfoCLUT[ret_index].osd_start_x = xPos;
    mOsdBlendInfoCLUT[ret_index].osd_start_y = yPos;
    mOsdBlendInfoCLUT[ret_index].osd_width = width;
    mOsdBlendInfoCLUT[ret_index].osd_pitch = (width + 31) & (~31);
    mOsdBlendInfoCLUT[ret_index].osd_height = height;

    ret = ioctl(mIavFd, IAV_IOC_ADD_OSD_BLEND_AREA_EX, &mOsdBlendInfoCLUT[ret_index]);
    if(0 != ret){
        AM_ERROR("IAV_IOC_ADD_OSD_BLEND_AREA error, ret %d.\n", ret);
        return (-6);
    }

    mOsdAreaCLUT[ret_index].inited = 1;

    mOsdAreaCLUT[ret_index].offset_x = xPos;
    mOsdAreaCLUT[ret_index].offset_y = yPos;
    mOsdAreaCLUT[ret_index].width = width;
    mOsdAreaCLUT[ret_index].height = height;
    mOsdAreaCLUT[ret_index].pitch = mOsdBlendInfoCLUT[ret_index].osd_pitch;
    mOsdAreaCLUT[ret_index].buffer_size = mOsdBlendInfoCLUT[ret_index].osd_pitch*height;
    AMLOG_INFO("CVideoEncoder::addOsdBlendArea id(%d), ret_index %d: width %d, height %d, pitch %d, buffer_size %d.\n", mOsdBlendInfoCLUT[ret_index].area_id, ret_index, width, height, mOsdAreaCLUT[ret_index].pitch, mOsdAreaCLUT[ret_index].buffer_size);

    mnOsdAreaNumberCLUT ++;
    mnTotalOsdBufferSizeCLUT += mOsdAreaCLUT[ret_index].buffer_size;

    for (ret = 0; ret < IAV_BLEND_AREA_BUFFER_NUMBER; ret ++) {
        mOsdAreaCLUT[ret_index].p_dsp_data[ret] = (AM_U8*)mOsdBlendInfoCLUT[ret_index].osd_buf_dram_addr_usr[ret];
    }
    mOsdAreaCLUT[ret_index].p_dsp_clut = (AM_U8*)mOsdBlendInfoCLUT[ret_index].clut_dram_addr_usr;

    //use default color table as default
    mOsdAreaCLUT[ret_index].p_clut = (AM_U8*)mPreSetY4Cb2Cr2CLUT;
    mOsdAreaCLUT[ret_index].cur_clut_type = 0;
    mOsdAreaCLUT[ret_index].clut_need_update = 1;

    AMLOG_INFO("CVideoEncoder::addOsdBlendArea id(%d) return index %d, data 0x%p, 0x%p, clut 0x%p.\n", mOsdBlendInfoCLUT[ret_index].area_id, ret_index, mOsdAreaCLUT[ret_index].p_dsp_data[0], mOsdAreaCLUT[ret_index].p_dsp_data[1], mOsdAreaCLUT[ret_index].p_dsp_clut);

    return ret_index;
}

AM_ERR CVideoEncoder::UpdateOsdBlendAreaCLUT(AM_INT index, AM_U8* data, AM_U32* p_input_clut, AM_INT width, AM_INT height, IParameters::OSDInputDataFormat data_format)
{
    AM_INT ret;
    AM_U32 area_fb_id = 0;

#if 0
    char dumpfile_name[256] = {0};
#endif

    AUTO_LOCK(mpMutex);

    if (EOSD_MODE_YCbCrCLUT != mOSDMode) {
        AM_ERROR("CVideoEncoder::UpdateOsdBlendAreaCLUT , bad osd mode %d.\n", mOSDMode);
        return ME_BAD_PARAM;
    }

    if (!data || (width <= 0) || (height <= 0)) {
        AM_ERROR("CVideoEncoder::UpdateOsdBlendAreaCLUT BAD params index %d, data %p, width %d, height %d.\n", index, data, width, height);
        return ME_BAD_PARAM;
    }

    if ((index >= DMAX_OSD_AREA_NUM) || (index < 0)) {
        AM_ERROR("CVideoEncoder::UpdateOsdBlendAreaCLUT %d exceed system max value.\n", index);
        return ME_BAD_PARAM;
    }

    if (!mOsdAreaCLUT[index].inited) {
        AM_ERROR("CVideoEncoder::UpdateOsdBlendAreaCLUT(index %d), it's not initialized yet.\n", index);
        return ME_BAD_PARAM;
    }

    if ((width > mOsdAreaCLUT[index].width) || (height > mOsdAreaCLUT[index].height)) {
        LOGE("UpdateOsdBlendAreaCLUT input width(%d)/height(%d) exceed area's %dx%d\n", width, height, mOsdAreaCLUT[index].width, mOsdAreaCLUT[index].height);
        return ME_BAD_PARAM;
    }

    if ((!mOsdAreaCLUT[index].p_dsp_data[0]) || (!mOsdAreaCLUT[index].p_dsp_data[1]) || (!mOsdAreaCLUT[index].p_dsp_clut)){
        AM_ERROR("CVideoEncoder::UpdateOsdBlendAreaCLUT, internal NULL pointer, please check code\n");
        return ME_ERROR;
    }

    if (p_input_clut) {
        area_fb_id = (mOsdBlendInfoCLUT[index].area_id) | (mOsdAreaCLUT[index].cur_buf_id << 8) | IAV_BLEND_UPDATE_CLUT_FLAG;
        mOsdAreaCLUT[index].clut_need_update = 1;
        mOsdAreaCLUT[index].cur_clut_type = 1;
        mOsdAreaCLUT[index].p_clut = (AM_U8*)p_input_clut;
    } else {
        area_fb_id = (mOsdBlendInfoCLUT[index].area_id) | (mOsdAreaCLUT[index].cur_buf_id << 8);
    }

    if (mOsdAreaCLUT[index].clut_need_update) {
#if 0
        snprintf(dumpfile_name, 255, "/sdcard/dump/clut_%d_%d.data", index, mnDumpCLUTCount++);
        AM_DumpBinaryFile(dumpfile_name, mOsdAreaCLUT[index].p_clut, 256*4);
#endif
        AM_ASSERT(mOsdAreaCLUT[index].p_clut);
        _convertOSDInputDataCLUT(&mOsdAreaCLUT[index], data, (AM_U32*)mOsdAreaCLUT[index].p_clut, width, height, mReservedTransparentIndex, data_format);
        mOsdAreaCLUT[index].clut_need_update = 0;
    } else {
        _convertOSDInputDataCLUT(&mOsdAreaCLUT[index], data, NULL, width, height, mReservedTransparentIndex, data_format);
    }

#if 0
    snprintf(dumpfile_name, 255, "/sdcard/dump/color_%d_%d.data", index, mnDumpColorCount++);
    AM_DumpBinaryFile(dumpfile_name, data, width*height);
#endif

    ret = ioctl(mIavFd, IAV_IOC_UPDATE_OSD_BLEND_AREA_EX, area_fb_id);
    if (0 != ret) {
        AM_ERROR("IAV_IOC_UPDATE_OSD_BLEND_AREA error, ret %d.\n", ret);
        return ME_ERROR;
    }

    if (0 == mOsdAreaCLUT[index].cur_buf_id) {
        mOsdAreaCLUT[index].cur_buf_id = 1;
    } else {
        mOsdAreaCLUT[index].cur_buf_id = 0;
    }

    return ME_OK;
}

AM_ERR CVideoEncoder::zeroOsdBlendAreaCLUT(AM_INT index)
{
    AM_INT ret;
    AM_U32 area_fb_id = 0;
    AM_U32* p_transparent = (AM_U32*)mOsdAreaCLUT[index].p_dsp_clut;

    if (mOsdAreaCLUT[index].p_dsp_data[0] && mOsdAreaCLUT[index].buffer_size && mOsdAreaCLUT[index].p_dsp_clut) {
        p_transparent[mReservedTransparentIndex] = 0x00008080;
        memset(mOsdAreaCLUT[index].p_dsp_data[0], mReservedTransparentIndex, mOsdAreaCLUT[index].buffer_size);
        area_fb_id = (mOsdBlendInfoCLUT[index].area_id) | IAV_BLEND_UPDATE_CLUT_FLAG;
        ret = ioctl(mIavFd, IAV_IOC_UPDATE_OSD_BLEND_AREA_EX, area_fb_id);
        if (0 != ret) {
            AM_ERROR("IAV_IOC_UPDATE_OSD_BLEND_AREA error, ret %d.\n", ret);
            return ME_ERROR;
        }
    }

    return ME_OK;
}

AM_ERR CVideoEncoder::RemoveOsdBlendAreaCLUT(AM_INT index)
{
    AM_INT ret;
    AUTO_LOCK(mpMutex);

    AMLOG_INFO("CVideoEncoder::RemoveOsdBlendAreaCLUT index %d\n", index);

    if (EOSD_MODE_YCbCrCLUT != mOSDMode) {
        AM_ERROR("CVideoEncoder::RemoveOsdBlendAreaCLUT , bad osd mode %d.\n", mOSDMode);
        return ME_BAD_PARAM;
    }

    if ((index >= DMAX_OSD_AREA_NUM) || (index < 0)) {
        AM_ERROR("CVideoEncoder::RemoveOsdBlendAreaCLUT %d exceed system max value.\n", index);
        return ME_BAD_PARAM;
    }

    if (!mOsdAreaCLUT[index].inited) {
        AM_ERROR("CVideoEncoder::RemoveOsdBlendAreaCLUT(index %d), it's not initialized yet.\n", index);
        return ME_BAD_PARAM;
    }

    mOsdAreaCLUT[index].inited = 0;
    mnOsdAreaNumberCLUT --;
    mnTotalOsdBufferSizeCLUT -= mOsdAreaCLUT[index].buffer_size;

    if (mnOsdAreaNumberCLUT < 0 || mnTotalOsdBufferSizeCLUT < 0) {
        LOGE("CVideoEncoder::RemoveOsdBlendAreaCLUT(%d), cause Internal error, please check codemnOsdAreaNumber %d, mnTotalOsdBufferSize %d.\n", index, mnOsdAreaNumberCLUT, mnTotalOsdBufferSizeCLUT);
        return ME_ERROR;
    }

    //zeroOsdBlendAreaCLUT(index);

    ret = ioctl(mIavFd, IAV_IOC_REMOVE_OSD_BLEND_AREA_EX, mOsdBlendInfoCLUT[index].area_id);
    if(0 != ret){
        AMLOG_ERROR("IAV_IOC_REMOVE_OSD_BLEND_AREA error, ret %d.\n", ret);
        return ME_ERROR;
    }
    AMLOG_INFO("Osd area %d, index %d is removed.\n", mOsdBlendInfoCLUT[index].area_id, index);

    memset(&mOsdBlendInfoCLUT[index], 0, sizeof(iav_osd_blend_info_ex_t));

    return ME_OK;
}

AM_ERR CVideoEncoder::UpdatePreviewDisplay(AM_UINT width, AM_UINT height, AM_UINT pos_x, AM_UINT pos_y, AM_UINT alpha)
{
    if (DSPMode_DuplexLowdelay == mModeConfig.dsp_mode) {
        iav_duplex_display_mode_t mode;
        mode.set = 0;
        AM_U8 request_preview_enabled = (width && height);

        AMLOG_INFO("[dynamic display]: display mode start, request preview enable %d.\n", request_preview_enabled);
        //get
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_DISPLAY_MODE, &mode) < 0) {
            perror("IAV_IOC_DUPLEX_DISPLAY_MODE");
            AM_ERROR("IAV_IOC_DUPLEX_DISPLAY_MODE fail.\n");
            return ME_OS_ERROR;
        }

        if (request_preview_enabled && !mode.preview_display_enabled) {
            //enable
            mode.set = 1;
            mode.change_preview_params = 0;
            mode.change_pb_display_params = 0;

            mode.preview_display_enabled = request_preview_enabled;
            AMLOG_INFO("[dynamic display]: before ioctl, enable preview display, %d, %d, vout index %d, %d.\n", mode.preview_display_enabled, mode.pb_display_enabled, mode.preview_vout_index, mode.vout_index);
            if (ioctl(mIavFd, IAV_IOC_DUPLEX_DISPLAY_MODE, &mode) < 0) {
                perror("IAV_IOC_DUPLEX_DISPLAY_MODE");
                AM_ERROR("IAV_IOC_DUPLEX_DISPLAY_MODE fail.\n");
                return ME_OS_ERROR;
            }
            return ME_OK;
        } else if (!request_preview_enabled) {
            //disable
            mode.set = 1;
            mode.change_preview_params = 0;
            mode.change_pb_display_params = 0;
            mode.preview_display_enabled = request_preview_enabled;
            AMLOG_INFO("[dynamic display]: before ioctl, disable preview display, %d, %d, vout index %d, %d.\n", mode.preview_display_enabled, mode.pb_display_enabled, mode.preview_vout_index, mode.vout_index);
            if (ioctl(mIavFd, IAV_IOC_DUPLEX_DISPLAY_MODE, &mode) < 0) {
                perror("IAV_IOC_DUPLEX_DISPLAY_MODE");
                AM_ERROR("IAV_IOC_DUPLEX_DISPLAY_MODE fail.\n");
                return ME_OS_ERROR;
            }
            return ME_OK;
        }

        AM_ASSERT(width && height);
        iav_duplex_update_preview_vout_config_t voutconfigs;
        memset((void*)&voutconfigs, 0x0, sizeof(voutconfigs));

        AMLOG_INFO("[dynamic display]: update preview display to size(%dx%d), position(%dx%d), alpha %d.\n", width, height, pos_x, pos_y, alpha);
        voutconfigs.enc_id = 0;//hard code here
        voutconfigs.vout_index = mModeConfig.playback_vout_index;
        voutconfigs.preview_vout_index = mModeConfig.preview_vout_index;
        voutconfigs.preview_alpha = alpha;
        voutconfigs.preview_width = width;
        voutconfigs.preview_height = height;
        voutconfigs.preview_left = pos_x;
        voutconfigs.preview_top = pos_y;

        if (ioctl(mIavFd, IAV_IOC_DUPLEX_UPDATE_PREVIEW_VOUT_CONFIG, &voutconfigs) < 0) {
            perror("IAV_IOC_DUPLEX_UPDATE_PREVIEW_VOUT_CONFIG");
            AM_ERROR("IAV_IOC_DUPLEX_UPDATE_PREVIEW_VOUT_CONFIG fail.\n");
            return ME_OS_ERROR;
        }
        return ME_OK;
    }else {
        AM_ERROR("add implement.\n");
    }
    return ME_ERROR;
}

AM_ERR CVideoEncoder::DemandIDR(AM_UINT out_index)
{
    if (DSPMode_DuplexLowdelay == mModeConfig.dsp_mode) {
        iav_duplex_demand_idr_t demand_t;
        memset((void*)&demand_t, 0x0, sizeof(demand_t));

        demand_t.enc_id = out_index;
        demand_t.stream_type = DSP_STREAM_TYPE_FULL_RESOLUTION;//hard code, main stream
        demand_t.on_demand_idr = 1;
        demand_t.pts_to_change = 0;

        if (ioctl(mIavFd, IAV_IOC_DUPLEX_DEMAND_IDR, &demand_t) < 0) {
            perror("IAV_IOC_DUPLEX_DEMAND_IDR");
            AM_ERROR("IAV_IOC_DUPLEX_DEMAND_IDR fail.\n");
            return ME_OS_ERROR;
        }
        return ME_OK;
    } else {
        AM_ERROR("add implement.\n");
    }
    return ME_ERROR;
}

AM_ERR CVideoEncoder::CaptureRawData(AM_U8*& p_raw, AM_UINT target_width, AM_UINT target_height, IParameters::PixFormat pix_format)
{
    AMLOG_INFO("CaptureRawData start\n");
    iav_img_buf_t img_buf;
    memset(&img_buf, 0, sizeof(img_buf));

    if (DSPMode_CameraRecording == mModeConfig.dsp_mode) {
        if (ioctl(mIavFd, IAV_IOC_GET_PREVIEW_BUFFER, &img_buf) < 0) {
            img_buf.buffer_id= IAV_BUFFER_ID_MAIN;
            AM_ERROR("IAV_IOC_GET_PREVIEW_BUFFER\n");
            perror("IAV_IOC_GET_PREVIEW_BUFFER");
            return ME_OS_ERROR;
        }
        AM_WARNING("IAV_IOC_GET_PREVIEW_BUFFER Done!\n");
    } else if (DSPMode_DuplexLowdelay == mModeConfig.dsp_mode) {
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_GET_PREVIEW_BUFFER, &img_buf) < 0) {
            img_buf.buffer_id= IAV_BUFFER_ID_MAIN;
            AM_ERROR("IAV_IOC_GET_PREVIEW_BUFFER\n");
            perror("IAV_IOC_GET_PREVIEW_BUFFER");
            return ME_OS_ERROR;
        }
        AM_WARNING("IAV_IOC_GET_PREVIEW_BUFFER Done!\n");
    } else {
        AM_ERROR("BAD dsp mode %d!\n", mModeConfig.dsp_mode);
        return ME_ERROR;
    }

    AMLOG_INFO("CaptureRawData: get from iav done\n");

    //convert data to target buffer
    AM_U8* psrc[3];
    AM_INT src_stride[3];
    AM_U8* pdes[3];
    AM_INT des_stride[3];

    psrc[0] = (AM_U8*)img_buf.luma_addr;
    psrc[1] = (AM_U8*)img_buf.chroma_addr;

    src_stride[0] = img_buf.buf_pitch;
    src_stride[1] = img_buf.buf_pitch;

    pdes[0] = p_raw;
    pdes[1] = p_raw + target_width*target_height;

    des_stride[0] = target_width;
    des_stride[1] = target_width;

    AM_SimpleScale(IParameters::PixFormat_NV12, psrc, pdes, src_stride, des_stride, img_buf.buf_width, img_buf.buf_height, target_width, target_height);
    AMLOG_INFO("CaptureRawData: scale done\n");

    return ME_OK;
}

AM_ERR CVideoEncoder::CaptureYUVdata(AM_INT fd, AM_UINT& pitch, AM_UINT& height, SYUVData* yuvdata)
{
    AMLOG_INFO("CaptureYUVdata start\n");
    iav_img_buf_t img_buf;
    memset(&img_buf, 0, sizeof(img_buf));

    if(yuvdata)
        img_buf.buffer_id = yuvdata->buffer_id;

    if (DSPMode_CameraRecording == mModeConfig.dsp_mode) {
        if (ioctl(mIavFd, IAV_IOC_GET_PREVIEW_BUFFER, &img_buf) < 0) {
            img_buf.buffer_id= IAV_BUFFER_ID_MAIN;
            AMLOG_ERROR("IAV_IOC_GET_PREVIEW_BUFFER\n");
            perror("IAV_IOC_GET_PREVIEW_BUFFER");
            return ME_OS_ERROR;
        }
        AMLOG_WARN("IAV_IOC_GET_PREVIEW_BUFFER Done!\n");
    } else if (DSPMode_DuplexLowdelay == mModeConfig.dsp_mode) {
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_GET_PREVIEW_BUFFER, &img_buf) < 0) {
            img_buf.buffer_id= IAV_BUFFER_ID_MAIN;
            AM_ERROR("IAV_IOC_GET_PREVIEW_BUFFER\n");
            perror("IAV_IOC_GET_PREVIEW_BUFFER");
            return ME_OS_ERROR;
        }
        AMLOG_DEBUG("IAV_IOC_GET_PREVIEW_BUFFER Done!\n");
    } else {
        AMLOG_ERROR("BAD dsp mode %d!\n", mModeConfig.dsp_mode);
        return ME_ERROR;
    }

    if(fd == 0){
        AM_ASSERT(yuvdata);
        yuvdata->image_width = mConfig.stream_info[1].width;
        yuvdata->image_height = img_buf.buf_height;
        yuvdata->image_step = img_buf.buf_pitch;
        yuvdata->yData = img_buf.luma_addr;
        yuvdata->uvData = img_buf.chroma_addr;
        pitch = img_buf.buf_pitch;
        height = img_buf.buf_height;
        return ME_OK;
    }

    pitch = img_buf.buf_pitch;
    height = img_buf.buf_height;
    write(fd, img_buf.luma_addr, (size_t)(pitch*height));
    write(fd,img_buf.chroma_addr, (size_t)(pitch*height/2));
    AMLOG_INFO("CaptureYUVdata: done\n");
    return ME_OK;
}

AM_ERR CVideoEncoder::UpdateGOPStructure(AM_UINT out_index, AM_INT M, AM_INT N, AM_INT idr_interval)
{
    if (DSPMode_DuplexLowdelay == mModeConfig.dsp_mode) {
        iav_duplex_update_gop_t gop;
        memset((void*)&gop, 0x0, sizeof(gop));

        gop.enc_id = out_index;
        gop.stream_type = DSP_STREAM_TYPE_FULL_RESOLUTION;//hard code, main stream
        gop.change_gop_option = 2;//hard code, next I/P
        gop.follow_gop = 0;
        gop.fgop_max_N = 0;
        gop.fgop_min_N = 0;
        gop.M = 1;//hard code
        gop.N = N;
        gop.idr_interval = idr_interval;
        gop.gop_structure = 0;//hard code, fix me
        gop.pts_to_change = 0xffffffffffffffffLL;//hard code

        if (ioctl(mIavFd, IAV_IOC_DUPLEX_UPDATE_GOP_STRUCTURE, &gop) < 0) {
            perror("IAV_IOC_DUPLEX_UPDATE_GOP_STRUCTURE");
            AM_ERROR("IAV_IOC_DUPLEX_UPDATE_GOP_STRUCTURE fail.\n");
            return ME_OS_ERROR;
        }
        return ME_OK;
    } else {
        AM_ERROR("add implement.\n");
    }
    return ME_ERROR;
}

AM_ERR CVideoEncoder::CaptureJpeg(char* jpeg_filename, AM_UINT target_width, AM_UINT target_height)
{
    if (!jpeg_filename || !target_height || !target_width) {
        AM_ERROR("BAD parameters.\n");
        return ME_BAD_PARAM;
    }

    if (DSPMode_CameraRecording == mModeConfig.dsp_mode) {
        iav_piv_jpeg_capture_t capture;
        memset(&capture, 0, sizeof(capture));
        strncpy(mJpegFileName, jpeg_filename, DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN);
        mJpegFileName[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = 0;

        capture.main_jpeg_w = target_width;
        capture.main_jpeg_h = target_height;

        AMLOG_INFO("IAV_IOC_STILL_JPEG_CAPTURE start, filename %s, w %d, h %d.\n", mJpegFileName, capture.main_jpeg_w, capture.main_jpeg_h);
        if (ioctl(mIavFd, IAV_IOC_STILL_JPEG_CAPTURE, &capture) < 0) {
            AM_ERROR("IAV_IOC_STILL_JPEG_CAPTURE\n");
            perror("IAV_IOC_STILL_JPEG_CAPTURE");
            return ME_OS_ERROR;
        }
        AMLOG_INFO("IAV_IOC_STILL_JPEG_CAPTURE Done!\n");
    } else {
        AM_ERROR("not support capture jpeg from dsp in this mode.\n");
        return ME_OS_ERROR;
    }
    return ME_OK;
}

AM_ERR CVideoEncoder::SetDisplayMode(AM_UINT display_preview, AM_UINT display_playback, AM_UINT preview_vout, AM_UINT pb_vout)
{
    if (DSPMode_DuplexLowdelay == mModeConfig.dsp_mode) {
        iav_duplex_display_mode_t mode;
        mode.set = 1;

        AMLOG_INFO("[dynamic display]: SetDisplayMode, request preview enable %d, pb display enable %d, preview_out %d, pb_vout %d.\n", display_preview, display_playback, preview_vout, pb_vout);
        mode.pb_display_enabled = display_playback;
        mode.preview_display_enabled = display_preview;
        mode.preview_vout_index = preview_vout;
        mode.vout_index = pb_vout;
        //set
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_DISPLAY_MODE, &mode) < 0) {
            perror("IAV_IOC_DUPLEX_DISPLAY_MODE");
            AM_ERROR("IAV_IOC_DUPLEX_DISPLAY_MODE fail.\n");
            return ME_OS_ERROR;
        }
    }
    return ME_OK;
}

AM_ERR CVideoEncoder::GetDisplayMode(AM_UINT& display_preview, AM_UINT& display_playback, AM_UINT& preview_vout, AM_UINT& pb_vout)
{
    if (DSPMode_DuplexLowdelay == mModeConfig.dsp_mode) {
        iav_duplex_display_mode_t mode;
        mode.set = 0;
        //get
        if (ioctl(mIavFd, IAV_IOC_DUPLEX_DISPLAY_MODE, &mode) < 0) {
            perror("IAV_IOC_DUPLEX_DISPLAY_MODE");
            AM_ERROR("IAV_IOC_DUPLEX_DISPLAY_MODE fail.\n");
            return ME_OS_ERROR;
        }
        display_playback= mode.pb_display_enabled;
        display_preview= mode.preview_display_enabled;
        preview_vout = mode.preview_vout_index;
        pb_vout = mode.vout_index;
        AMLOG_INFO("[dynamic display]: GetDisplayMode, request preview enable %d, pb display enable %d, preview_out %d, pb_vout %d.\n", display_preview, display_playback, preview_vout, pb_vout);
    }
    return ME_OK;
}

AM_ERR CVideoEncoder::FreezeResumeHDMIPreview(AM_INT flag)
{
    AM_U32 mode = (AM_U32)flag;
    AMLOG_INFO("FreezeResumeHDMIPreview mode %u\n", mode);
    if (ioctl(mIavFd, IAV_IOC_FREEZE_RESUME_HDMI_PREVIEW, mode) < 0) {
        perror("IAV_IOC_FREEZE_RESUME_HDMI_PREVIEW");
        AM_ERROR("IAV_IOC_FREEZE_RESUME_HDMI_PREVIEW fail.\n");
        return ME_OS_ERROR;
    }
    return ME_OK;
}

//

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

AM_ERR CVideoEncoderOutput::SendVideoBuffer(SBitDesc* p_desc, AM_U64 expired_time)
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

    AMLOG_DEBUG("SendVideoBuffer frame num %d, stream id %d, pictype %d, pts %lld. DATA_DDR:%p, size %u\n"
        , p_desc->frame_number, p_desc->enc_id, p_desc->pic_type, p_desc->pts, p_desc->pstart, p_desc->size);

    pBuffer->mBufferId = p_desc->fb_id;
    pBuffer->mEncId = p_desc->enc_id;
    pBuffer->SetType(CBuffer::DATA);
    pBuffer->mFlags |= IAV_ENC_BUFFER ;
    //re-align pts
    if(mLastPTS > p_desc->pts){
        if((mLastPTS - p_desc->pts) > MAX_VIDEO_PTS/2){
            AM_WARNING("last frame pts is greater than the current frame pts, last pts: %llu, current pts %llu, mPTSLoop %d\n", mLastPTS, p_desc->pts, mPTSLoop);
            mPTSLoop += 1;
            mLoopPTSOffset += MAX_VIDEO_PTS;
        }
    }
    mLastPTS = p_desc->pts;
    pBuffer->mPTS = mLoopPTSOffset + p_desc->pts - mStartPTSOffset;
    pBuffer->mExpireTime = expired_time;
    pBuffer->mBlockSize = pBuffer->mDataSize = p_desc->size;
    pBuffer->mpData = (AM_U8*)p_desc->pstart;
    AM_ASSERT(pBuffer->mpData);
    pBuffer->mSeqNum = mnFrames++;
    pBuffer->mOriSeqNum = p_desc->frame_number;
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
    pBuffer->SetDataPtr(NULL);
    //pBuffer->mFlags = 0;
    SendBuffer(pBuffer);
    return ME_OK;
}

void CVideoEncoderOutput::ProcessBuffer(SBitDesc* p_desc, AM_U64 expired_time)
{
    SListNode* pNode;
    AMLOG_DEBUG("process p_desc frame num %d, stream id %d, pictype %d, pts %lld.\n", p_desc->frame_number, p_desc->enc_id, p_desc->pic_type, p_desc->pts);

    //send finalize file if needed
    if ((mbNeedSendFinalizeFile) && (PredefinedPictureType_IDR == p_desc->pic_type)) {
        mbNeedSendFinalizeFile = false;
        AM_ASSERT(VideoEncOPin_Runing == msState);
        SendFlowControlBuffer(IFilter::FlowControl_finalize_file, p_desc->pts);
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
                    if (PredefinedPictureType_B != p_desc->pic_type) {
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
                if (PredefinedPictureType_IDR == p_desc->pic_type) {
                    mbSkip = false;
                } else {
                    AMLOG_WARN("skip frame here.\n");
                    return;
                }
            }
            SendVideoBuffer(p_desc, expired_time);
            break;

        case VideoEncOPin_tobePaused:
            AM_ASSERT(mbSkip == false);
            //pause till reference picture comes
            if (PredefinedPictureType_B != p_desc->pic_type) {
                mbSkip = true;
                SendFlowControlBuffer(IFilter::FlowControl_pause);
                msState = VideoEncOPin_Paused;
                return;
            }
            SendVideoBuffer(p_desc, expired_time);
            break;

        case VideoEncOPin_Paused:
            //AM_ASSERT(mbSkip == true);
            pNode = mpFlowControlList->peek_first();
            if (pNode) {
                if (pNode->context == IFilter::FlowControl_resume) {
                    msState = VideoEncOPin_tobeResumed;
                    //remove node
                    mpFlowControlList->release(pNode);

                    //resume immediately when idr picture comes
                    if (PredefinedPictureType_IDR == p_desc->pic_type) {
                        SendFlowControlBuffer(IFilter::FlowControl_resume);
                        SendVideoBuffer(p_desc, expired_time);
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
            //AM_ASSERT(mbSkip == true);
            //pause till idr picture comes
            if (PredefinedPictureType_IDR == p_desc->pic_type) {
                SendFlowControlBuffer(IFilter::FlowControl_resume);
                SendVideoBuffer(p_desc, expired_time);
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


