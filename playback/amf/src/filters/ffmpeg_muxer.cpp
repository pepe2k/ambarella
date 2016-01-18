/*
 * ffmpeg_muxer.cpp
 *
 * History:
 *    2011/7/21 - [Zhi He] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "ffmpeg_muxer"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#if PLATFORM_LINUX
#include <sys/time.h>
#include <arpa/inet.h>
#endif

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"

#include "am_pbif.h"
#include "pbif.h"
#include "engine_guids.h"
#include "filter_list.h"

#include "record_if.h"

extern "C" {
//#define INT64_C(a) (a ## LL)
#include "libavutil/avstring.h"
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec_export.h"
}

#include "streamming_if.h"

#include "ffmpeg_muxer.h"
#include "am_ffmpeg.h"

#define SPS_PPS_LEN (57+8+6)

IFilter* CreateFFmpegMuxer(IEngine *pEngine)
{
    return CFFMpegMuxer::Create(pEngine);
}

bool _get_sps_pps(AM_U8 *pBuffer, AM_UINT size, AM_U8*& p_spspps, AM_UINT& spspps_size, AM_U8*& p_IDR, AM_U8*& p_ret_data)
{
    AM_UINT checking_size = 0;
    AM_U32 start_code;
    bool find_sps = false;
    bool find_pps = false;
    bool find_IDR = false;
    bool find_SEI = false;
    AM_U8 type;

    AM_ASSERT(pBuffer);
    if (!pBuffer) {
        AM_ERROR("NULL pointer in _get_sps_pps.\n");
        return false;
    }

    AM_WARNING("[debug info], input %p, size %d, data [%02x %02x %02x %02x %02x]\n", pBuffer, size, pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3], pBuffer[4]);

    p_spspps = pBuffer;
    p_IDR = pBuffer;
    p_ret_data = NULL;
    spspps_size = 0;

    start_code = 0xffffffff;
    while ((checking_size + 1) < size) {
        start_code <<= 8;
        start_code |= pBuffer[checking_size];

        if (start_code == 0x00000001) {
            type = (0x1f & pBuffer[checking_size + 1]);
            if (0x05 == type) {
                AM_ASSERT(find_sps);
                AM_ASSERT(find_pps);
                AM_ASSERT(!find_IDR);
                p_IDR = pBuffer + checking_size - 3;
                find_IDR = true;
                AM_WARNING("[debug info], find IDR, checking_size - 3 %d\n", checking_size - 3);
                break;
            } else if (0x06 == type) {
                //to do
                find_SEI = true;
                AM_WARNING("[debug info], find SEI, checking_size - 3 %d, find_SEI=%d\n", checking_size - 3, find_SEI);
            } else if (0x07 == type) {
                AM_ASSERT(!find_sps);
                AM_ASSERT(!find_pps);
                AM_ASSERT(!find_IDR);
                p_spspps = pBuffer + checking_size - 3;
                find_sps = true;
                AM_WARNING("[debug info], find sps, checking_size - 3 %d\n", checking_size - 3);
            } else if (0x08 == type) {
                AM_ASSERT(find_sps);
                AM_ASSERT(!find_pps);
                AM_ASSERT(!find_IDR);
                find_pps = true;
                AM_WARNING("[debug info], find pps, checking_size - 3 %d\n", checking_size - 3);
            } else if (0x09 == type) {
                p_ret_data = pBuffer + checking_size - 3;
                AM_WARNING("[debug info], find delimiter, checking_size - 3 %d\n", checking_size - 3);
            } else {
                AM_WARNING("unknown type %d.\n", type);
            }
        }

        checking_size ++;
    }

    if (find_sps && find_IDR) {
        AM_ASSERT(find_pps);
        AM_ASSERT(((AM_UINT)p_IDR) > ((AM_UINT)p_spspps));
        if (((AM_UINT)p_IDR) > ((AM_UINT)p_spspps)) {
            spspps_size = ((AM_UINT)p_IDR) - ((AM_UINT)p_spspps);
        } else {
            AM_ERROR("internal error!!\n");
            return false;
        }
        AM_WARNING("[debug info], input %p, find IDR %p, sps %p, seq size %d, checking_size %d\n", pBuffer, p_IDR, p_spspps, spspps_size, checking_size);
        return true;
    }

    AM_WARNING("cannot find sps(%d), pps(%d), IDR header(%d), please check code!!\n", find_sps, find_pps, find_IDR);
    return false;
}

static AM_UINT _read_bit(AM_U8 *pBuffer, AM_INT *value, AM_U8 *bit_pos = 0, AM_UINT num = 1)
{
    *value = 0;
    AM_UINT i = 0, j;
    for (j = 0; j < num; j++) {
        if (*bit_pos == 8) {
            *bit_pos = 0;
            i++;
        }

        if (*bit_pos == 0) {
            if (pBuffer[i] == 0x03  &&
                pBuffer[i - 1] == 0 &&
                pBuffer[i - 2] == 0)
                i++;
        }

        *value <<= 1;
        *value += pBuffer[i] >> (7 - (*bit_pos)++) &0x1;
    }
    return i;
}

static AM_UINT _adts_header_length(AM_U8* pStart)
{
    AM_UINT header_length = 0;
    AM_INT syncword;
    AM_U8 bit_pos = 0;
    AM_U8* pAdts_header = pStart;
    pAdts_header += _read_bit (pAdts_header, &syncword, &bit_pos, 12);
    if (syncword != 0xFFF) {
        AM_ERROR("syncword != 0xFFF [%s-%s-%d]", __FILE__, __FUNCTION__, __LINE__);
        return 0;
    }

    bit_pos += 3;
    AM_INT protection_absent;
    pAdts_header += _read_bit (pAdts_header, &protection_absent, &bit_pos);
    pAdts_header = pStart + 6;
    bit_pos = 6;

    header_length = 7;
    AM_INT number_of_raw_data_blocks_in_frame;
    pAdts_header += _read_bit (pAdts_header, &number_of_raw_data_blocks_in_frame, &bit_pos, 2);
    if (number_of_raw_data_blocks_in_frame == 0) {
        if (protection_absent == 0) {
            header_length += 2;
        }
    }
    return header_length;
}

static bool _isTmeStampNear(am_pts_t t0, am_pts_t t1, am_pts_t max_gap)
{
    if ((t0 + max_gap < t1) || (t1 + max_gap < t0)) {
        return false;
    }
    return true;
}

static bool _pinNeedSync(IParameters::StreamType type)
{
    //only sync video/audio with precise PTS in current case
    if (IParameters::StreamType_Audio == type || IParameters::StreamType_Video == type) {
        return true;
    }
    return false;
}

typedef struct _SMemorySlip
{
    AM_U8* p_buffer_start;
    AM_UINT tot_size;
    AM_U8* p_last_ptr;
    AM_U8* p_current_ptr;

    AM_UINT index;
    struct _SMemorySlip* p_next;
}SMemorySlip;

//custom memory io, drived from AVIOContext
typedef struct _SDirectMemoryIO
{
    AVIOContext avio;

    //private
    SMemorySlip* current_slip;
    SMemorySlip* slip_list_head;

    //communication related
    void* p_outputpin;
}SDirectMemoryIO;

AM_INT DMIO_read_packet(void *opaque, AM_U8 *buf, AM_INT buf_size)
{
    //not implement
    AM_ASSERT(0);
    return 0;
}

AM_INT DMIO_write_packet(void *opaque, AM_U8 *buf, AM_INT buf_size)
{
    AM_ASSERT(opaque);
    AM_ASSERT(buf);
    AM_ASSERT(buf_size > 0);
    if (!opaque || !buf || buf_size <= 0) {
        AM_ERROR("must not comes here, corrupt params in DMIO_write_packet, opaque %p, buf %p, buf_size %d.\n", opaque, buf, buf_size);
        return -1;
    }


    return 0;
}

AM_S64 DMIO_seek(void *opaque, AM_S64 offset, AM_INT whence)
{
    //not implement
    AM_ASSERT(0);
    return 0;
}

unsigned long DMIO_update_checksum(unsigned long checksum, const AM_U8 *buf, AM_UINT size)
{
    return 0;
}

AM_INT DMIO_read_pause(void *opaque, AM_INT pause)
{
    //not implement
    AM_ASSERT(0);
    return 0;
}
AM_S64 DMIO_read_seek(void *opaque, AM_INT stream_index,AM_S64 timestamp, AM_INT flags)
{
    //not implement
    AM_ASSERT(0);
    return 0;
}

SDirectMemoryIO* CreateMemoryIO()
{
    return NULL;
}

void DestroyMemoryIO(SDirectMemoryIO* p)
{
    return;
}

//code from avc.c
static AM_U8 *_find_startcode_internal(AM_U8 *p, AM_U8 *end)
{
    AM_U8 *a = p + 4 - ((AM_INTPTR)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(const uint32_t*)p;
//      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
//      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
        if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p+1;
            }
            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p+2;
                if (p[4] == 0 && p[5] == 1)
                    return p+3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}

static AM_U8 *_find_startcode(AM_U8 *p, AM_U8 *end){
    AM_U8 *out= _find_startcode_internal(p, end);
    if(p<out && out<end && !out[-1]) out--;
    return out;
}

CFFMpegMuxerInput* CFFMpegMuxerInput::Create(CFilter *pFilter)
{
    CFFMpegMuxerInput* result = new CFFMpegMuxerInput(pFilter);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CFFMpegMuxerInput::Construct()
{
    AMLOG_DEBUG("CFFMpegMuxerInput::Construct.\n");
    AM_ASSERT(mpFilter);
    AM_ERR err = inherited::Construct(((CFFMpegMuxer*)mpFilter)->MsgQ());
    return err;
}

void CFFMpegMuxerInput::timeDiscontinue()
{
    mSession ++;
    mCurrentFrameCount = 0;
}

AM_ERR CFFMpegMuxerInput::SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index)
{
    AM_ASSERT(param);
    AMLOG_PRINTF("CFFMpegMuxerInput::SetParameters start: type %d, format %d, param %p.\n", type, format, param);

    switch (type) {
        case StreamType_Video:
            mType = StreamType_Video;
            mParams.format = format;
            mParams.specific.video = param->video;
            mParams.bitrate = param->video.bitrate;
            if (format == StreamFormat_H264) {
                mParams.codec_id = CODEC_ID_H264;
            } else {
                AM_ASSERT(0);
                AMLOG_ERROR("need implement, not supported.\n");
                return ME_NOT_SUPPORTED;
            }
            break;

        case StreamType_Audio:
            mType = StreamType_Audio;
            mParams.format = format;
            mParams.specific.audio = param->audio;
            mParams.bitrate = param->audio.bitrate;
            if (format == StreamFormat_AAC) {
                mParams.codec_id = CODEC_ID_AAC;
            } else if (format == StreamFormat_MP2) {
                mParams.codec_id = CODEC_ID_MP2;
            } else if (format == StreamFormat_AC3) {
                mParams.codec_id = CODEC_ID_AC3;
            } else if (format == StreamFormat_ADPCM) {
                mParams.codec_id = CODEC_ID_ADPCM_EA;
            } else if (format == StreamFormat_AMR_WB) {
                mParams.codec_id = CODEC_ID_AMR_WB;
            } else if (format == StreamFormat_AMR_NB) {
                mParams.codec_id = CODEC_ID_AMR_NB;
            } else {
                AM_ASSERT(0);
                AMLOG_ERROR("need implement, not supported audio format %d.\n", format);
                return ME_NOT_SUPPORTED;
            }
            break;

        case StreamType_Subtitle:
            //to do
            return ME_NOT_SUPPORTED;
            break;

        case StreamType_PrivateData:
            mType = StreamType_PrivateData;
            mDuration = param->pridata.duration;
            break;

        default:
            AM_ASSERT(0);
            AMLOG_ERROR("must not come here.\n");
            return ME_NOT_SUPPORTED;
    }
    return ME_OK;
}

CFFMpegMuxerInput::~CFFMpegMuxerInput()
{
    //mpPrivateData  used and managed by Muxer
/*
    if (mpPrivateData) {
        free(mpPrivateData);
    }
*/
    if (mpDumper) {
        mpDumper->CloseFile();
        mpDumper->Delete();
        mpDumper = NULL;
    }
    AMLOG_DESTRUCTOR("CFFMpegMuxerInput::~CFFMpegMuxerInput.\n");
}

IFilter* CFFMpegMuxer::Create(IEngine *pEngine)
{
    CFFMpegMuxer *result = new CFFMpegMuxer(pEngine);
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CFFMpegMuxer::Construct()
{
    AM_ERR err = inherited::Construct();
    if (err != ME_OK) {
        AM_ERROR("CFFMpegMuxer::Construct() fail.\n");
        return err;
    }

    DSetModuleLogConfig(LogModuleFFMpegMuxer);

    if ((mpMutex = CMutex::Create(false)) == NULL){
        AM_ERROR("CFFMpegMuxer::Construct(), CMutex::Create() fail.\n");
        return ME_NO_MEMORY;;
    }

    //avcodec_init();
    //av_register_all();
    mw_ffmpeg_init();

    //if enable streaming, malloc buffer here
    AM_ASSERT(mpConsistentConfig);
    if (mpConsistentConfig->streaming_enable) {
        mpRTPBuffer = (AM_U8*)malloc(DRecommandMaxUDPPayloadLength);
        if (mpRTPBuffer) {
            mRTPBufferTotalLength = DRecommandMaxUDPPayloadLength;
        } else {
            AM_ERROR("NO Memory.\n");
        }
    }

    mpClockManager = (IClockManager *)mpEngine->QueryEngineInterface(IID_IClockManagerExt);
    if(!mpClockManager) {
        AM_ERROR("CFFMpegMuxer without mpClockManager?\n");
        return ME_ERROR;
    }
    return ME_OK;
}

void CFFMpegMuxer::Delete()
{
    {
        AUTO_LOCK(mpMutex);
        stopRTPSendTask();
    }

    AM_UINT i = 0;
    for (i = 0; i < mTotalInputPinNumber; i++) {
        AM_ASSERT(mpInput[i]);
        AM_DELETE(mpInput[i]);
        mpInput[i] = NULL;
    }
    mTotalInputPinNumber = 0;

    AM_DELETE(mpMutex);
    mpMutex = NULL;

    if (mpRTPBuffer) {
        free(mpRTPBuffer);
        mpRTPBuffer = NULL;
    }

    if (mRTPSocket >= 0) {
        ::close(mRTPSocket);
        mRTPSocket = -1;
    }

    if (mRTCPSocket >= 0) {
        ::close(mRTCPSocket);
        mRTCPSocket = -1;
    }

    if (mpOutputFileName) {
        free(mpOutputFileName);
        mpOutputFileName = NULL;
    }

    if (mpOutputFileNameBase) {
        free(mpOutputFileNameBase);
        mpOutputFileNameBase = NULL;
    }

    if (mpThumbNailFileName) {
        free(mpThumbNailFileName);
        mpThumbNailFileName = NULL;
    }

    inherited::Delete();
}

CFFMpegMuxer::~CFFMpegMuxer()
{
    {
        AUTO_LOCK(mpMutex);
        stopRTPSendTask();
    }

    AM_UINT i = 0;
    AMLOG_DESTRUCTOR("CFFMpegMuxer::~CFFMpegMuxer.\n");
    for (i = 0; i < mTotalInputPinNumber; i++) {
        AM_ASSERT(mpInput[i]);
        AM_DELETE(mpInput[i]);
        mpInput[i] = NULL;
    }
    mTotalInputPinNumber = 0;

    if (mpRTPBuffer) {
        free(mpRTPBuffer);
        mpRTPBuffer = NULL;
    }

    if (mRTPSocket >= 0) {
        ::close(mRTPSocket);
        mRTPSocket = -1;
    }

    if (mpOutputFileName) {
        free(mpOutputFileName);
        mpOutputFileName = NULL;
    }

    if (mpOutputFileNameBase) {
        free(mpOutputFileNameBase);
        mpOutputFileNameBase = NULL;
    }

    if (mpThumbNailFileName) {
        free(mpThumbNailFileName);
        mpThumbNailFileName = NULL;
    }

    AM_DELETE(mpMutex);
    mpMutex = NULL;
    AMLOG_DESTRUCTOR("CFFMpegMuxer::~CFFMpegMuxer done.\n");
}

void *CFFMpegMuxer::GetInterface(AM_REFIID refiid)
{
    if (refiid == IID_IMuxerControl)
        return (IMuxerControl*)this;
    else if (refiid == IID_IMuxer)
        return (IMuxer*)this;
    else if (refiid == IID_IStreammingDataTransmiter)
        return (IStreammingDataTransmiter*)this;
    return inherited::GetInterface(refiid);
}

void CFFMpegMuxer::GetInfo(INFO& info)
{
    info.nInput = mTotalInputPinNumber;
    info.nOutput = 0;
    info.pName = "FFMpegMuxer";
}

IPin* CFFMpegMuxer::GetInputPin(AM_UINT index)
{
    if (index < mTotalInputPinNumber) {
        return (IPin*)mpInput[index];
    }
    return NULL;
}

AM_ERR CFFMpegMuxer::SetContainerType(IParameters::ContainerType container_type)
{
    //param check
    AM_ASSERT(container_type < MuxerContainer_TotolNum);
    if (container_type >= MuxerContainer_TotolNum) {
        AMLOG_ERROR("Bad container type %d, in CFFMpegMuxer::SetOutputFile.\n", container_type);
        return ME_BAD_PARAM;
    }

    mContainerType = container_type;
    AM_ASSERT(mContainerType < MuxerContainer_TotolNum);
    return ME_OK;
}

AM_ERR CFFMpegMuxer::SetOutputFile(const char *pFileName)
{
    //param check
    if (!pFileName) {
        AMLOG_ERROR("NULL params in CFFMpegMuxer::SetOutputFile.\n");
        return ME_BAD_PARAM;
    }

    AM_UINT currentLen = strlen(pFileName)+1;

    AMLOG_DEBUG("CFFMpegMuxer::SetOutputFile len %d, filename %s.\n", currentLen, pFileName);
    if (!mpOutputFileNameBase || (currentLen + 4) > mOutputFileNameBaseLength) {
        if (mpOutputFileNameBase) {
            free(mpOutputFileNameBase);
            mpOutputFileNameBase = NULL;
        }

        mOutputFileNameBaseLength = currentLen + 4;
        if ((mpOutputFileNameBase = (char*)malloc(mOutputFileNameBaseLength)) == NULL) {
            mOutputFileNameBaseLength = 0;
            return ME_NO_MEMORY;
        }
    }

    AM_ASSERT(mpOutputFileNameBase);
    strncpy(mpOutputFileNameBase, pFileName, currentLen - 1);
    mpOutputFileNameBase[currentLen - 1] = 0x0;
    AMLOG_DEBUG("CFFMpegMuxer::SetOutputFile done, len %d, filenamebase %s.\n", mOutputFileNameBaseLength, mpOutputFileNameBase);
    return ME_OK;
}

AM_ERR CFFMpegMuxer::SetThumbNailFile(const char *pThumbNailFileName)
{
    //param check
    if (!pThumbNailFileName) {
        AMLOG_ERROR("NULL params in CFFMpegMuxer::SetThumbNailFile.\n");
        return ME_BAD_PARAM;
    }

    AM_UINT currentLen = strlen(pThumbNailFileName)+1;

    AMLOG_INFO("CFFMpegMuxer::SetThumbNailFile len %d, filename %s.\n", currentLen, pThumbNailFileName);
    if (!mpThumbNailFileNameBase || (currentLen + 4) > mThumbNailFileNameBaseLength) {
        if (mpThumbNailFileNameBase) {
            free(mpThumbNailFileNameBase);
            mpThumbNailFileNameBase = NULL;
        }

        mThumbNailFileNameBaseLength = currentLen + 4;
        if ((mpThumbNailFileNameBase = (char*)malloc(mThumbNailFileNameBaseLength)) == NULL) {
            mThumbNailFileNameBaseLength = 0;
            AM_ERROR("NO memory.\n");
            return ME_NO_MEMORY;
        }
    }

    AM_ASSERT(mpThumbNailFileNameBase);
    strncpy(mpThumbNailFileNameBase, pThumbNailFileName, currentLen - 1);
    mpThumbNailFileNameBase[currentLen - 1] = 0x0;
    AMLOG_INFO("CFFMpegMuxer::SetThumbNailFile done, len %d, filenamebase %s.\n", mThumbNailFileNameBaseLength, mpThumbNailFileNameBase);
    return ME_OK;
}

AM_ERR CFFMpegMuxer::SetMaxFileNumber(AM_UINT max_file_num)
{
    mMaxTotalFileNumber = max_file_num;
    AMLOG_INFO("CFFMpegMuxer::SetMaxFileNumber %d.\n", max_file_num);
    return ME_OK;
}

AM_ERR CFFMpegMuxer::SetTotalFileNumber(AM_UINT total_file_num)
{
    mTotalRecFileNumber = total_file_num;
    AMLOG_INFO("CFFMpegMuxer::SetTotalFileNumber %d.\n", total_file_num);
    return ME_OK;
}

AM_ERR CFFMpegMuxer::SetSavingStrategy(IParameters::MuxerSavingFileStrategy strategy, IParameters::MuxerSavingCondition condition, IParameters::MuxerAutoFileNaming naming, AM_UINT param, bool PTSstartFromZero)
{
    mSavingFileStrategy = strategy;
    mSavingCondition = condition;
    mAutoFileNaming = naming;

    mbPTSStartFromZero = PTSstartFromZero;

    switch (mSavingCondition) {
        case IParameters::MuxerSavingCondition_FrameCount:
            mAutoSaveFrameCount = param;
            break;
        case IParameters::MuxerSavingCondition_InputPTS:
        case IParameters::MuxerSavingCondition_CalculatedPTS:
            mAutoSavingTimeDuration = param * IParameters::TimeUnitDen_90khz;
            mNextFileTimeThreshold = mAutoSavingTimeDuration; //set initialized value
            mAudioNextFileTimeThreshold = mAutoSavingTimeDuration;
            break;
        case IParameters::MuxerSavingCondition_Invalid:
        default:
            AM_ERROR("BAD mSavingCondition %d.\n", mSavingCondition);
            return ME_BAD_PARAM;
    }

    AMLOG_INFO("CFFMpegMuxer::SetSavingStrategy, mSavingFileStrategy %d, mSavingCondition %d, mAutoFileNaming %d, mAutoSaveFrameCount %d, mAutoSavingTimeDuration %llu, mNextFileTimeThreshold %llu.\n", mSavingFileStrategy, mSavingCondition, mAutoFileNaming, mAutoSaveFrameCount, mAutoSavingTimeDuration, mNextFileTimeThreshold);
    return ME_OK;
}

AM_ERR CFFMpegMuxer::SetParameters(StreamType type, StreamFormat format, UFormatSpecific* param, AM_UINT index)
{
    AUTO_LOCK(mpMutex);
    if (index >= mTotalInputPinNumber) {
        AM_ASSERT(0);
        AMLOG_ERROR("CFFMpegMuxer::SetParameters bad params: index %d out of range, mTotalInputPinNumber %d.\n", index, mTotalInputPinNumber);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(mpInput[index]);
    if (!mpInput[index]) {
        AMLOG_ERROR("CFFMpegMuxer::SetParameters NULL input pin? must have errors, index %d, tot number %d.\n", index, mTotalInputPinNumber);
        return ME_BAD_PARAM;
    }

    return mpInput[index]->SetParameters(type, format, param);
}

AM_ERR CFFMpegMuxer::AddInputPin(AM_UINT& index, AM_UINT type)
{
    AUTO_LOCK(mpMutex);
    AM_ASSERT((mTotalInputPinNumber+1) < MaxStreamNumber);
    if ((mTotalInputPinNumber+1) >= MaxStreamNumber) {
        AMLOG_ERROR("Max stream number reached, mTotalInputPinNumber %d, MaxStreamNumber %d", mTotalInputPinNumber, MaxStreamNumber);
        return ME_BAD_PARAM;
    }

    index = mTotalInputPinNumber;
    AM_ASSERT(mpInput[index] == NULL);
    if (mpInput[index]) {
        AMLOG_ERROR("mpInput[index] have context, must have error here, delete previous, create new one.\n");
        delete mpInput[index];
        mpInput[index] = NULL;
    }

    mpInput[index] = CFFMpegMuxerInput::Create((CFilter *)this);
    if (mpInput[index] == NULL) {
        AMLOG_ERROR("mpInput[index] create fail.\n");
        return ME_ERROR;
    }
    mpInput[index]->mType = type;
    mpInput[index]->mIndex = index;
    mTotalInputPinNumber ++;

    char dumpfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};

    if ((mpConsistentConfig->muxer_dump_audio && mpInput[index]->mType == IParameters::StreamType_Audio)) {
        mpInput[index]->mpDumper = CFileWriter::Create();
        AM_ASSERT(mpInput[index]->mpDumper);
        if (mpInput[index]->mpDumper) {
            snprintf(dumpfilename, sizeof(dumpfilename), "%s/audio_%d.es", AM_GetPath(AM_PathDump), index);
            AMLOG_INFO("dump audio es, filename %s.\n", dumpfilename);
            mpInput[index]->mpDumper->CreateFile(dumpfilename);
        } else {
            AM_ASSERT(0);
        }
    } else if ((mpConsistentConfig->muxer_dump_video && mpInput[index]->mType == IParameters::StreamType_Video)) {
        mpInput[index]->mpDumper = CFileWriter::Create();
        AM_ASSERT(mpInput[index]->mpDumper);
        if (mpInput[index]->mpDumper) {
            snprintf(dumpfilename, sizeof(dumpfilename), "%s/video_%d.es", AM_GetPath(AM_PathDump), index);
            AMLOG_INFO("dump video es, filename %s.\n", dumpfilename);
            mpInput[index]->mpDumper->CreateFile(dumpfilename);
        } else {
            AM_ASSERT(0);
        }
    } else if ((mpConsistentConfig->muxer_dump_pridata && mpInput[index]->mType == IParameters::StreamType_PrivateData)) {
        mpInput[index]->mpDumper = CFileWriter::Create();
        AM_ASSERT(mpInput[index]->mpDumper);
        if (mpInput[index]->mpDumper) {
            snprintf(dumpfilename, sizeof(dumpfilename), "%s/pridata_%d.data", AM_GetPath(AM_PathDump), index);
            AMLOG_INFO("dump private data, filename %s.\n", dumpfilename);
            mpInput[index]->mpDumper->CreateFile(dumpfilename);
        } else {
            AM_ASSERT(0);
        }
    }

    //set master inputpin, video have non-key frame, choose first video's pin, this feature not support multi-video streams now.
    if (NULL == mpMasterInput) {
        if (IParameters::StreamType_Video == type) {
            AMLOG_PRINTF(" Set master input pin %p, index %d.\n", mpMasterInput, index);
            mpMasterInput = mpInput[index];
        }
    }

    return ME_OK;
}

void CFFMpegMuxer::clearFFMpegContext()
{
    AM_UINT i;
    //clear resources?
    AM_ASSERT(mpOutFormat);
    AM_ASSERT(mpFormat);
    if (!mpFormat || !mpOutFormat) {
        return;
    }

    //free each inputpin's context
    for (i=0; i<mTotalInputPinNumber; i++) {
        switch (mpInput[i]->mType) {
            case StreamType_Audio:
                if (mpInput[i]->mpStream) {
                    if (mpInput[i]->mpStream->codec) {
                        avcodec_close(mpInput[i]->mpStream->codec);
                    }
                    //av_freep(mpInput[i]->mpStream);
                    mpInput[i]->mpStream = NULL;
                }
            case StreamType_Video:
                if (mpInput[i]->mpStream) {
                    //have not open video codec
                    //if (mpInput[i]->mpStream->codec) {
                    //    avcodec_close(mpInput[i]->mpStream->codec);
                    //}
                    //av_freep(mpInput[i]->mpStream);
                    mpInput[i]->mpStream = NULL;
                }
                break;
            case StreamType_PrivateData:
                if (mpInput[i]->mpStream) {
                    mpInput[i]->mpStream = NULL;
                }
                break;
            default:
                AM_ASSERT(0);
                AMLOG_ERROR("must not comes here.\n");
                break;
        }
    }

    if (!(mpOutFormat->flags & AVFMT_NOFILE)) {
        /* close the output file */
        avio_close(mpFormat->pb);
    }

    //memory allocated for mpPrivateData will be freed by avformat_free_context()
    //   and mpPrivateData should be set NULL for auto-split case
    for (i=0; i<mTotalInputPinNumber; i++) {
        mpInput[i]->mpPrivateData = NULL;
    }
    avformat_free_context(mpFormat);
    mpFormat = NULL;
    mpOutFormat = NULL;
}

AM_ERR CFFMpegMuxer::Finalize()
{
    if(mpConsistentConfig->disable_save_files){
        mCurrentTotalFilesize = 0;
        return ME_OK;
    }
    AM_MSG msg;
    //AM_ASSERT(mpFormat);
    if (!mpFormat || !mpOutFormat) {
        return ME_OK;
    }

    //estimate some info for file
    getFileInformation();

    //AV_TIME_BASE
    mpFormat->duration = (mFileDuration*AV_TIME_BASE)/IParameters::TimeUnitDen_90khz;
    mpFormat->file_size = mCurrentTotalFilesize;

    AMLOG_PRINTF("** start write trailer, duration %lld, size %lld, bitrate %d.\n", mpFormat->duration, mpFormat->file_size, mFileBitrate);
    if (av_write_trailer(mpFormat)<0) {
        AMLOG_ERROR(" av_write_trailer err\n");
        return ME_ERROR;
    }
    mTotalFileNumber ++;
    AMLOG_PRINTF("** write trailer done.\n");

    msg.code = IEngine::MSG_NEWFILE_GENERATED;
    msg.p1  = (AM_INTPTR)static_cast<IFilter*>(this);
    if (IParameters::MuxerAutoFileNaming_ByDateTime == mAutoFileNaming) {
        msg.p2 = (AM_INTPTR) mLastTime;
    } else {
        msg.p2 = mCurrentFileIndex;
    }
    msg.p5 = (AM_U8) mContainerType;
    PostEngineMsg(msg);

    clearFFMpegContext();
    AMLOG_PRINTF("** clearFFMpegContext done.\n");

    //clear some variables
    mCurrentTotalFilesize = 0;

    return ME_OK;
}

AM_ERR CFFMpegMuxer::Initialize()
{
    if(mpConsistentConfig->disable_save_files){
        AM_ASSERT(mTotalInputPinNumber > 0);
        AM_ASSERT(mTotalInputPinNumber < MaxStreamNumber);
        if (mTotalInputPinNumber == 0 || mTotalInputPinNumber >= MaxStreamNumber || !mpInput[0]) {
            AMLOG_ERROR("must not come here, check papameters, mTotalInputPinNumber %d, mpInput[0] %p.\n", mTotalInputPinNumber, mpInput[0]);
            return ME_BAD_STATE;
        }
        for (int i=0; i<(int)mTotalInputPinNumber; i++) {
            CFFMpegMuxerInput*pInput = mpInput[i];
            AM_ASSERT(pInput);
            if (pInput) {
                setParametersToMuxer(pInput);
                pInput->mbAutoBoundaryReached = false;
                pInput->mbAutoBoundaryStarted = false;
            }
        }
        mCurrentTotalFilesize = 0;
        return ME_OK;
    }
    AM_ASSERT(!mpFormat);
    AM_UINT i;
    AM_INT ret;
    CFFMpegMuxerInput* pInput;

    //appendFileExtention();

    //check
    AM_ASSERT(mTotalInputPinNumber > 0);
    AM_ASSERT(mTotalInputPinNumber < MaxStreamNumber);
    if (mTotalInputPinNumber == 0 || mTotalInputPinNumber >= MaxStreamNumber || !mpInput[0]) {
        //error case
        AMLOG_ERROR("must not come here, check papameters, mTotalInputPinNumber %d, mpInput[0] %p.\n", mTotalInputPinNumber, mpInput[0]);
        return ME_BAD_STATE;
    }

    mpOutFormat = av_guess_format(NULL, mpOutputFileName, NULL);
    if (!mpOutFormat) {
        AMLOG_ERROR("Could not deduce output format from file(%s) extension: using MPEG.\n", mpOutputFileName);
        mpOutFormat = av_guess_format(_getContainerString(mContainerType), NULL, NULL);
        if (!mpOutFormat) {
            AMLOG_ERROR("output format not supported, type %d, .%s.\n", mContainerType, _getContainerString(mContainerType));
        }
        return ME_ERROR;
    }

    mpFormat = avformat_alloc_context();
    if (!mpFormat) {
        AMLOG_ERROR("avformat_alloc_context fail.\n");
        return ME_NO_MEMORY;
    }

    mpFormat->oformat = mpOutFormat;
    AM_ASSERT(sizeof(mpFormat->filename) > mOutputFileNameLength);
    snprintf(mpFormat->filename, sizeof(mpFormat->filename), "%s", mpOutputFileName);

    memset(&mAVFormatParam, 0, sizeof(mAVFormatParam));
    if (av_set_parameters(mpFormat, &mAVFormatParam) < 0) {
        AMLOG_ERROR("av_set_parameters error.\n");
        return ME_ERROR;
    }

    //add streams
    for (i=0; i<mTotalInputPinNumber; i++) {
        pInput = mpInput[i];
        AM_ASSERT(pInput);
        if (pInput) {

            //donot set the first PTS to 0
            if (!mbPTSStartFromZero && (StreamType_Video == pInput->mType))
                mStreamStartPTS = pInput->mLastPTS + pInput->mDuration;

            AMLOG_PRINTF("** before av_new_stream.\n");
            pInput->mpStream = av_new_stream(mpFormat, mpFormat->nb_streams);
            pInput->mpStream->probe_data.filename = mpFormat->filename;
            setParametersToMuxer(pInput);

            //reset some parameters
            pInput->mbAutoBoundaryReached = false;
            pInput->mbAutoBoundaryStarted = false;
        }
    }

    av_dump_format(mpFormat, 0, mpOutputFileName, 1);

    AMLOG_PRINTF("** before avio_open, mpOutputFileName %s.\n", mpOutputFileName);
    /* open the output file, if needed */
    if (!(mpOutFormat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&mpFormat->pb, mpOutputFileName, AVIO_FLAG_WRITE)) < 0) {
            AMLOG_ERROR("Could not open '%s', ret %d, READ-ONLY file system, or BAD filepath/filename?\n", mpOutputFileName, ret);
            return ME_ERROR;
        }
    }
    AMLOG_PRINTF("** before av_write_header.\n");

    ret = av_write_header(mpFormat);
    if (ret < 0) {
        AMLOG_ERROR("Failed to write header, ret %d\n", ret);
        //some debug print
        AMLOG_ERROR("Maybe in-correct container/codec type, for-example, 'AMR' cannot in 'TS', 'PrivateData' should in 'TS'.\n");
        AMLOG_ERROR("Or Maybe container type not matched, SetContainerType(MP4) but with file name 'xxx.ts'.\n");
        return ME_ERROR;
    }

    checkVideoParameters();
    checkParameters();

    //clear some variables
    mCurrentTotalFilesize = 0;

    return ME_OK;
}

void CFFMpegMuxer::deletePartialFile(AM_UINT file_index)
{
    char* cmd = (char*)malloc(mOutputFileNameLength + 16);
    if (NULL == cmd) {
        AM_ERROR("Cannot malloc memory in CFFMpegMuxer::deletePartialFile, request size %d,\n", mOutputFileNameLength + 16);
        return;
    }
    memset(cmd, 0, mOutputFileNameLength + 16);
    AMLOG_INFO("start remove file %s_%06d.%s.\n", mpOutputFileNameBase, file_index, _getContainerString(mContainerType));
    snprintf(cmd, mOutputFileNameLength + 16, "rm %s_%06d.%s", mpOutputFileNameBase, file_index, _getContainerString(mContainerType));
    AMLOG_INFO("cmd: %s.\n", cmd);
    system(cmd);
    AMLOG_INFO("end remove file %s_%06d.%s.\n", mpOutputFileNameBase, file_index, _getContainerString(mContainerType));
    free(cmd);
}

void CFFMpegMuxer::analyseFileNameFormat(char* pstart)
{
    char*pcur = NULL;
    IParameters::ContainerType type_fromname;

    if (!mpOutputFileNameBase) {
        AM_ERROR("NULL mpOutputFileNameBase.\n");
        return;
    }

    AMLOG_INFO("[analyseFileNameFormat], input %s.\n", mpOutputFileNameBase);

    //check if need append
    pcur = strrchr(pstart, '.');
    if (pcur) {
        pcur ++;
        type_fromname = AM_GetContainerTypeFromString(pcur);
        if (IParameters::MuxerContainer_Invalid == type_fromname) {
            AMLOG_WARN("[analyseFileNameFormat], not supported extention %s, append correct file externtion.\n", pcur);
            mFileNameHanding1 = eFileNameHandling_appendExtention;
        } else {
            AMLOG_INFO("[analyseFileNameFormat], have valid file name externtion %s.\n", pcur);
            mFileNameHanding1 = eFileNameHandling_noAppendExtention;
            AM_ASSERT(type_fromname == mContainerType);
            if (type_fromname != mContainerType) {
                AMLOG_WARN("[analyseFileNameFormat], container type(%d) from file name not match preset value %d, app need invoke setOutputFormat(to AMRecorder), and make sure setOutput(to engine)'s parameters consist?\n", type_fromname,  mContainerType);
            }
        }
    } else {
        AMLOG_INFO("[analyseFileNameFormat], have no file name externtion.\n");
        mFileNameHanding1 = eFileNameHandling_appendExtention;
    }

    //check if need insert, only process one %, todo
    pcur = strchr(pstart, '%');
    //have '%'
    if (pcur) {
        pcur ++;
        if ('t' == (*pcur)) {
            //if have '%t'
            AMLOG_INFO("[analyseFileNameFormat], file name is insertDataTime mode.\n");
            mFileNameHanding2 = eFileNameHandling_insertDateTime;
            *pcur = 's';//use '%s', to do
        } else if (('d' == (*pcur)) || ('d' == (*(pcur+1))) || ('d' == (*(pcur+2)))) {
            //if have '%d' or '%06d'
            AMLOG_INFO("[analyseFileNameFormat], file name is insertFileNumber mode.\n");
            mFileNameHanding2 = eFileNameHandling_insertFileNumber;
        }
    } else {
        AMLOG_INFO("[analyseFileNameFormat], file name not need insert something.\n");
        mFileNameHanding2 = eFileNameHandling_noInsert;
    }

    AMLOG_INFO("[analyseFileNameFormat], done, input %s, result mFileNameHanding1 %d, mFileNameHanding2 %d.\n", mpOutputFileNameBase, mFileNameHanding1, mFileNameHanding2);
}

//update file name and thumbnail name
void CFFMpegMuxer::updateFileName(AM_UINT file_index, bool simpleautofilename)
{
    AMLOG_INFO("before append extntion: Muxing file name %s, mFileNameHanding1 %d, mFileNameHanding2 %d.\n", mpOutputFileName, mFileNameHanding1, mFileNameHanding2);

    //ffmpeg use extention to specify container type
    /*if (strncmp("sharedfd://", mpOutputFileName, strlen("sharedfd://")) == 0) {
        AMLOG_WARN("shared fd, not append file externtion.\n");
        return;
    }*/
    if (!mpOutputFileName || mOutputFileNameLength < (strlen(mpOutputFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4)) {
        if (mpOutputFileName) {
            //debug log
            AMLOG_WARN("filename buffer(%p) not enough, remalloc it, or len %d, request len %d.\n", mpOutputFileName, mOutputFileNameLength, (strlen(mpOutputFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4));
            free(mpOutputFileName);
            mpOutputFileName = NULL;
        }
        mOutputFileNameLength = strlen(mpOutputFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4;
        if ((mpOutputFileName = (char*)malloc(mOutputFileNameLength)) == NULL) {
            mOutputFileNameLength = 0;
            AM_ERROR("No Memory left, how to handle this issue.\n");
            return;
        }
    }

    if (eFileNameHandling_appendExtention == mFileNameHanding1) {
        if (eFileNameHandling_noInsert == mFileNameHanding2) {
            if (IParameters::MuxerSavingFileStrategy_ToTalFile == mSavingFileStrategy) {
                snprintf(mpOutputFileName, mOutputFileNameLength, "%s.%s", mpOutputFileNameBase, _getContainerString(mContainerType));
            } else {
                if (IParameters::MuxerAutoFileNaming_ByNumber == mAutoFileNaming) {
                    snprintf(mpOutputFileName, mOutputFileNameLength, "%s_%06d.%s", mpOutputFileNameBase, file_index, _getContainerString(mContainerType));
                } else if (IParameters::MuxerAutoFileNaming_ByDateTime == mAutoFileNaming) {
                    char datetime_buffer[128];
                    memset(datetime_buffer, 0, sizeof(datetime_buffer));
                    mLastTime = time(NULL);
                    strftime(datetime_buffer, sizeof(datetime_buffer), "%Z%Y%m%d_%H%M%S", gmtime(&mLastTime));
                    snprintf(mpOutputFileName, mOutputFileNameLength, "%s%s.%s", mpOutputFileNameBase, datetime_buffer, _getContainerString(mContainerType));
                }
            }
        } else if (eFileNameHandling_insertFileNumber == mFileNameHanding2) {
            snprintf(mpOutputFileName, mOutputFileNameLength - 8, mpOutputFileNameBase, file_index);
            strncat(mpOutputFileName, ".", mOutputFileNameLength);
            strncat(mpOutputFileName, _getContainerString(mContainerType), mOutputFileNameLength);
        } else if (eFileNameHandling_insertDateTime== mFileNameHanding2) {
            char datetime_buffer[128];
            memset(datetime_buffer, 0, sizeof(datetime_buffer));
            mLastTime = time(NULL);
            strftime(datetime_buffer, sizeof(datetime_buffer), "%Z%Y%m%d_%H%M%S", gmtime(&mLastTime));

            snprintf(mpOutputFileName, mOutputFileNameLength - 8, mpOutputFileNameBase, datetime_buffer);
            strncat(mpOutputFileName, ".", mOutputFileNameLength);
            strncat(mpOutputFileName, _getContainerString(mContainerType), mOutputFileNameLength);
        } else {
            AM_ASSERT(0);
        }
    } else if (eFileNameHandling_noAppendExtention == mFileNameHanding1) {
        if (eFileNameHandling_insertFileNumber == mFileNameHanding2) {
            snprintf(mpOutputFileName, mOutputFileNameLength, mpOutputFileNameBase, file_index);
        } else if (eFileNameHandling_insertDateTime == mFileNameHanding2) {
            char datetime_buffer[128];
            memset(datetime_buffer, 0, sizeof(datetime_buffer));
            mLastTime = time(NULL);
            strftime(datetime_buffer, sizeof(datetime_buffer), "%Z%Y%m%d_%H%M%S", gmtime(&mLastTime));
            snprintf(mpOutputFileName, mOutputFileNameLength, mpOutputFileNameBase, datetime_buffer);
        } else {
            if (eFileNameHandling_noInsert != mFileNameHanding2) {
                AM_ERROR("ERROR parameters(mFileNameHanding2 %d) here, please check.\n", mFileNameHanding2);
            }

            if (IParameters::MuxerSavingFileStrategy_ToTalFile == mSavingFileStrategy) {
                //do nothing
                strncpy(mpOutputFileName, mpOutputFileNameBase, mOutputFileNameLength - 1);
                mpOutputFileName[mOutputFileNameLength - 1] = 0x0;
            } else {
                char* ptmp = strrchr(mpOutputFileNameBase, '.');
                AM_ASSERT(ptmp);
                if (ptmp) {
                    *ptmp++ = '\0';

                    //insert if auto cut
                    if (IParameters::MuxerAutoFileNaming_ByNumber == mAutoFileNaming) {
                        snprintf(mpOutputFileName, mOutputFileNameLength, "%s%06d.%s", mpOutputFileNameBase, file_index, ptmp);
                    } else if (IParameters::MuxerAutoFileNaming_ByDateTime == mAutoFileNaming) {
                        char datetime_buffer[128];
                        memset(datetime_buffer, 0, sizeof(datetime_buffer));
                        mLastTime = time(NULL);
                        strftime(datetime_buffer, sizeof(datetime_buffer), "%Z%Y%m%d_%H%M%S", gmtime(&mLastTime));
                        snprintf(mpOutputFileName, mOutputFileNameLength, "%s%s.%s", mpOutputFileNameBase, datetime_buffer, ptmp);
                    } else {
                        AM_ASSERT(0);
                    }
                    *(ptmp -1) = '.';
                } else {
                    AM_ERROR("SHOULD not comes here, please check code.\n");
                    strncpy(mpOutputFileName, mpOutputFileNameBase, mOutputFileNameLength - 1);
                    mpOutputFileName[mOutputFileNameLength - 1] = 0x0;
                }
            }
        }
    }

    //skychen, 2012_11_22
    if(simpleautofilename){
        char index[32] = {0};
        char* ptr = NULL;
        ptr = strchr(mpOutputFileName, '.');
        snprintf(index, 16, "_%04d%s", file_index, ptr);
        ptr[0] = 0x0;
        strcat(mpOutputFileName, index);
    }

    AMLOG_INFO("after append extntion: Muxing file name %s\n", mpOutputFileName);
    AM_ASSERT((AM_UINT)(strlen(mpOutputFileName)) <= mOutputFileNameLength);

    if (mpSharedRes->encoding_mode_config.thumbnail_enabled && mpThumbNailFileNameBase) {
        AMLOG_INFO("before append extntion: Thumbnail file name %s", mpThumbNailFileName);
        //param check
        if(!mpThumbNailFileName || mThumbNailFileNameLength < (strlen(mpThumbNailFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4)){
            if (mpThumbNailFileName) {
                //debug log
                AMLOG_WARN("thumbnail file name buffer(%p) not enough, remalloc it, or len %d, request len %d.\n", mpThumbNailFileName, mThumbNailFileNameLength, (strlen(mpThumbNailFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4));
                free(mpThumbNailFileName);
                mpThumbNailFileName = NULL;
            }
            mThumbNailFileNameLength = strlen(mpThumbNailFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4;
            if ((mpThumbNailFileName = (char*)malloc(mThumbNailFileNameLength)) == NULL) {
                mThumbNailFileNameLength = 0;
                AM_ERROR("No Memory left, how to handle this issue.\n");
                return;
            }
        }

        //the name of thumbnail is like as .XXXXfile_index.thm
        snprintf(mpThumbNailFileName, mThumbNailFileNameLength, mpThumbNailFileNameBase, file_index);
        AMLOG_INFO("after append extntion: Thumbnail file name %s\n", mpThumbNailFileName);
    }
}

AM_ERR CFFMpegMuxer::setParametersToMuxer(CFFMpegMuxerInput* pInput)
{
    switch (pInput->mType) {
        case StreamType_Video:
            return setVideoParametersToMuxer(pInput);
        case StreamType_Audio:
            return setAudioParametersToMuxer(pInput);
        case StreamType_Subtitle:
            return setSubtitleParametersToMuxer(pInput);
        case StreamType_PrivateData:
            return setPrivateDataParametersToMuxer(pInput);
        default:
            AM_ASSERT(0);
            AMLOG_ERROR("Bad input type %d.\n", pInput->mType);
            return ME_BAD_PARAM;
    }
}

AM_ERR CFFMpegMuxer::setVideoParametersToMuxer(CFFMpegMuxerInput* pInput)
{
    AVStream* pstream = NULL;
    AVCodecContext * video_enc = NULL;
//    AVCodec *codec;

    if(!mpConsistentConfig->disable_save_files){
        AM_ASSERT(mpFormat);
    }
    if (!pInput || (!mpFormat && !mpConsistentConfig->disable_save_files)) {
        AMLOG_ERROR("CFFMpegMuxer::setVideoParametersToMuxer NULL pointer pInput %p, mpFormat %p.\n", pInput, mpFormat);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(pInput->mType == StreamType_Video);
    if (pInput->mType != StreamType_Video) {
        AMLOG_ERROR("CFFMpegMuxer::setVideoParametersToMuxer: pInput's type(%d) is not video.\n", pInput->mType);
        return ME_BAD_PARAM;
    }

    //safe check here(only support h264 now), remove later
    AM_ASSERT(pInput->mParams.format == StreamFormat_H264);
    if (pInput->mParams.format != StreamFormat_H264) {
        AMLOG_ERROR("CFFMpegMuxer::setVideoParametersToMuxer video format is not h264, pInput->mParams.format %d.\n", pInput->mParams.format);
        return ME_NOT_SUPPORTED;
    }

    if(!mpConsistentConfig->disable_save_files){
        pstream = pInput->mpStream;
        AM_ASSERT(pstream);
        if (!pstream || !pstream->codec) {
            AMLOG_ERROR("CFFMpegMuxer::setVideoParametersToMuxer NULL pointer pstream %p, pstream->codec %p.\n", pstream, pstream->codec);
            return ME_BAD_PARAM;
        }

        pstream->probe_data.filename = mpFormat->filename;

        AM_ASSERT(pstream->codec);
        avcodec_get_context_defaults2(pstream->codec, AVMEDIA_TYPE_VIDEO);
        video_enc = pstream->codec;
        if(mpFormat->oformat->flags & AVFMT_GLOBALHEADER)
            video_enc->flags |= CODEC_FLAG_GLOBAL_HEADER;

        switch (pInput->mParams.format) {
        case StreamFormat_H264:
            AMLOG_PRINTF("video format is AVC(h264).\n");
            video_enc->codec_id = CODEC_ID_H264;
            //mpOutFormat->video_codec = CODEC_ID_H264;
            //video_enc->codec_tag = MK_FOURCC_TAG('H', '2', '6', '4');
            pInput->mParams.codec_id = (AM_INT)CODEC_ID_H264;
            break;
        default:
            AM_ASSERT(0);
            AMLOG_ERROR("only support h264 now, unsupported pInput->mParams.format %d.\n", pInput->mParams.format);
            return ME_NOT_SUPPORTED;
        }

        /**
         * "codec->time_base"
         * This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identically 1.
        */
        video_enc->time_base.num = pInput->mParams.specific.video.framerate_den;
        video_enc->time_base.den = pInput->mParams.specific.video.framerate_num;

        pstream->time_base.num = 1;
        pstream->time_base.den = 90000;
    }

    //initialize params
    pInput->mStartPTS = mStreamStartPTS;
    pInput->mSessionPTSStartPoint = mStreamStartPTS;//each session start pts
    pInput->mCurrentPTS = mStreamStartPTS;
    pInput->mCurrentDTS = mStreamStartPTS;
    pInput->mSessionInputPTSStartPoint = 0;

    pInput->mTimeUintDen = IParameters::TimeUnitDen_90khz;
    pInput->mTimeUintNum = 1;

    pInput->mCurrentFrameCount = 0;
    pInput->mTotalFrameCount = 0;
    pInput->mSession = 0;
    pInput->mDuration = (AM_U64)pInput->mParams.specific.video.framerate_den * 90000 / pInput->mParams.specific.video.framerate_num;
    pInput->mExpectedDuration = pInput->mDuration;
    pInput->mPTSDTSGap = pInput->mPTSDTSGapDivideDuration * pInput->mDuration;
    AMLOG_INFO("mPTSDTSGapDivideDuration %d, mDuration%d, mPTSDTSGap %d.\n", pInput->mPTSDTSGapDivideDuration, pInput->mDuration, pInput->mPTSDTSGap);
    pInput->PrintTimeInfo();

    if(mpConsistentConfig->disable_save_files){
        return ME_OK;
    }
    //master input set sync value
    mSyncedStreamPTSDTSGap = pInput->mPTSDTSGap;

    video_enc->width = pInput->mParams.specific.video.pic_width;
    video_enc->height = pInput->mParams.specific.video.pic_height;

    //tmp code, for app to distiguwish the stream is main or not main
    if (video_enc->width > 1200) {
        isHD = true;
    } else {
        isHD = false;
    }

    video_enc->sample_aspect_ratio = pstream->sample_aspect_ratio = (AVRational) {(int)pInput->mParams.specific.video.sample_aspect_ratio_num, (int)pInput->mParams.specific.video.sample_aspect_ratio_den};
    video_enc->has_b_frames = 2;
    video_enc->bit_rate = 400000;// 4M
    video_enc->gop_size = 15;
    video_enc->pix_fmt = PIX_FMT_YUV420P;
    video_enc->codec_type = AVMEDIA_TYPE_VIDEO;
    if (pInput->mpPrivateData == NULL) {
        pInput->mpPrivateData = (AM_U8*)av_mallocz(200);
    }
    AM_ASSERT(pInput->mpPrivateData);

    video_enc->extradata_size = pInput->mParams.extradata_size = SPS_PPS_LEN;
    video_enc->extradata = pInput->mParams.p_extradata = pInput->mpPrivateData;

/*
    if (!(codec = avcodec_find_encoder(video_enc->codec_id))) {
        AMLOG_ERROR("Failed to find audio codec\n");
        return ME_ERROR;
    }
    if (avcodec_open(video_enc, codec) < 0) {
        AMLOG_ERROR("Failed to open audio codec\n");
        return ME_ERROR;
    }
*/
    AMLOG_PRINTF("       width %d, height %d, sample_aspect_ratio %d %d, extradata_size %d, pointer %p.\n", video_enc->width, video_enc->height, video_enc->sample_aspect_ratio.num, video_enc->sample_aspect_ratio.den, video_enc->extradata_size, video_enc->extradata);
    return ME_OK;
}

AM_ERR CFFMpegMuxer::setAudioParametersToMuxer(CFFMpegMuxerInput* pInput)
{
    AVStream* pstream = NULL;
    AVCodecContext * audio_enc = NULL;
    AVCodec *codec;
    SSimpleAudioSpecificConfig* p_simple_header = NULL;
    AM_U8 samplerate = 0;

    if(!mpConsistentConfig->disable_save_files){
        AM_ASSERT(mpFormat);
    }
    if (!pInput || (!mpFormat && !mpConsistentConfig->disable_save_files)) {
        AMLOG_ERROR("CFFMpegMuxer::setAudioParametersToMuxer NULL pointer pInput %p, mpFormat %p.\n", pInput, mpFormat);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(pInput->mType == StreamType_Audio);
    if (pInput->mType != StreamType_Audio) {
        AMLOG_ERROR("CFFMpegMuxer::setAudioParametersToMuxer: pInput's type(%d) is not audio.\n", pInput->mType);
        return ME_BAD_PARAM;
    }

    if(!mpConsistentConfig->disable_save_files){
        pstream = pInput->mpStream;
        AM_ASSERT(pstream);
        if (!pstream || !pstream->codec) {
            AMLOG_ERROR("CFFMpegMuxer::setVideoParameters NULL pointer pstream %p, pstream->codec %p.\n", pstream, pstream->codec);
            return ME_BAD_PARAM;
        }

        pstream->probe_data.filename = mpFormat->filename;

        AM_ASSERT(pstream->codec);
        avcodec_get_context_defaults2(pstream->codec, AVMEDIA_TYPE_AUDIO);
        audio_enc = pstream->codec;
        if(mpFormat->oformat->flags & AVFMT_GLOBALHEADER)
            audio_enc->flags |= CODEC_FLAG_GLOBAL_HEADER;

        switch (pInput->mParams.format) {
        case StreamFormat_MP2:
            AMLOG_PRINTF("audio format is MP2.\n");
            audio_enc->codec_id = CODEC_ID_MP2;
            //mpOutFormat->video_codec = CODEC_ID_MP2;
            pInput->mParams.codec_id = (AM_INT)CODEC_ID_MP2;
            //audio_enc->frame_size = 1024;
            break;
        case StreamFormat_AC3:
            AMLOG_PRINTF("audio format is AC3.\n");
            audio_enc->codec_id = CODEC_ID_AC3;
            pInput->mParams.codec_id = (AM_INT)CODEC_ID_AC3;
            break;
        case StreamFormat_ADPCM:
            AMLOG_PRINTF("audio format is ADPCM.\n");
            audio_enc->codec_id = CODEC_ID_ADPCM_EA;
            pInput->mParams.codec_id = (AM_INT)CODEC_ID_ADPCM_EA;
            break;
        case StreamFormat_AMR_NB:
            AMLOG_PRINTF("audio format is AMR_NB.\n");
            audio_enc->codec_id = CODEC_ID_AMR_NB;
            pInput->mParams.codec_id = (AM_INT)CODEC_ID_AMR_NB;
            break;
        case StreamFormat_AMR_WB:
            AMLOG_PRINTF("audio format is AMR_WB.\n");
            audio_enc->codec_id = CODEC_ID_AMR_WB;
            pInput->mParams.codec_id = (AM_INT)CODEC_ID_AMR_WB;
            break;
        default:
            AM_ASSERT(0);
            AMLOG_ERROR("only support aac now, unsupported pInput->mParams.format %d.\n", pInput->mParams.format);
        case StreamFormat_AAC:
            AMLOG_PRINTF("audio format is AAC.\n");
            audio_enc->codec_id = CODEC_ID_AAC;
            //mpOutFormat->video_codec = CODEC_ID_AAC;
            pInput->mParams.codec_id = (AM_INT)CODEC_ID_AAC;
            //audio_enc->codec_tag = 0x00ff;

            //extra data
            pInput->mpPrivateData = (AM_U8*)av_mallocz(2);
            pInput->mParams.extradata_size = 2;
            pInput->mParams.p_extradata = pInput->mpPrivateData;

#if 1
            p_simple_header = (SSimpleAudioSpecificConfig*)pInput->mpPrivateData;
            samplerate = 0;

            p_simple_header->audioObjectType = eAudioObjectType_AAC_LC;//hard code here
            switch (pInput->mParams.specific.audio.sample_rate) {
                case 44100:
                    samplerate = eSamplingFrequencyIndex_44100;
                    break;
                case 48000:
                    samplerate = eSamplingFrequencyIndex_48000;
                    break;
                case 24000:
                    samplerate = eSamplingFrequencyIndex_24000;
                    break;
                case 16000:
                    samplerate = eSamplingFrequencyIndex_16000;
                    break;
                case 8000:
                    samplerate = eSamplingFrequencyIndex_8000;
                    break;
                case 12000:
                    samplerate = eSamplingFrequencyIndex_12000;
                    break;
                case 32000:
                    samplerate = eSamplingFrequencyIndex_32000;
                    break;
                case 22050:
                    samplerate = eSamplingFrequencyIndex_22050;
                    break;
                case 11025:
                    samplerate = eSamplingFrequencyIndex_11025;
                    break;
                default:
                    AM_ERROR("ADD support here, audio sample rate %d.\n", pInput->mParams.specific.audio.sample_rate);
                    break;
            }
            p_simple_header->samplingFrequencyIndex_high = samplerate >> 1;
            p_simple_header->samplingFrequencyIndex_low = samplerate & 0x1;
            p_simple_header->channelConfiguration = pInput->mParams.specific.audio.channel_number;
            p_simple_header->bitLeft = 0;

            AMLOG_INFO("pInput->mParams.specific.audio.sample_rate %d, channel_number %d.\n", pInput->mParams.specific.audio.sample_rate, pInput->mParams.specific.audio.channel_number);
            AMLOG_INFO("samplingFrequencyIndex_high %x, samplingFrequencyIndex_low %x.\n", (AM_U8)p_simple_header->samplingFrequencyIndex_high, (AM_U8)p_simple_header->samplingFrequencyIndex_low);
            AMLOG_INFO("audioObjectType %x.\n", (AM_U8)p_simple_header->audioObjectType);
            AMLOG_INFO("channelConfiguration %x.\n", (AM_U8)p_simple_header->channelConfiguration);
            AMLOG_INFO("bitLeft %x.\n", (AM_U8)p_simple_header->bitLeft);
#else
            //Fix me, Hard code for AAC LC/44100/2/0/0/0
            if ((2 == pInput->mParams.specific.audio.channel_number) && (44100 == pInput->mParams.specific.audio.sample_rate)) {
                pInput->mParams.p_extradata[0] = 0x12;//0001 0010
                pInput->mParams.p_extradata[1] = 0x10;//0001 0000
            } else if ((2 == pInput->mParams.specific.audio.channel_number) && (48000 == pInput->mParams.specific.audio.sample_rate)) {
                pInput->mParams.p_extradata[0] = 0x11;//0001 0001
                pInput->mParams.p_extradata[1] = 0x90;//1001 0000
            } else if ((1 == pInput->mParams.specific.audio.channel_number) && (44100 == pInput->mParams.specific.audio.sample_rate)) {
                pInput->mParams.p_extradata[0] = 0x12;//0001 0010
                pInput->mParams.p_extradata[1] = 0x08;//0000 1000
            } else if ((1 == pInput->mParams.specific.audio.channel_number) && (48000 == pInput->mParams.specific.audio.sample_rate)) {
                pInput->mParams.p_extradata[0] = 0x11;//0001 0001
                pInput->mParams.p_extradata[1] = 0x88;//1000 1000
            } else {
                AM_ERROR("ADD support here, write correct aac header for channel number %d, sample rate %d.\n", pInput->mParams.specific.audio.channel_number, pInput->mParams.specific.audio.sample_rate);
            }
#endif
            AMLOG_INFO("Audio AAC config data %x, %x.\n", pInput->mpPrivateData[0], pInput->mpPrivateData[1]);

            //audio_enc->frame_size = 1024;
            break;
        }
    }
/*
    //timebase
    audio_enc->time_base.num = 1;
    audio_enc->time_base.den = 90000;
    pstream->time_base.num = 1;
    pstream->time_base.den = 90000;*/

    pInput->mTimeUintDen = pInput->mParams.specific.audio.pts_unit_den;
    pInput->mTimeUintNum = pInput->mParams.specific.audio.pts_unit_num;
    AMLOG_INFO("audio pts unit: num %d, den %d.\n", pInput->mTimeUintNum, pInput->mTimeUintDen);

    //debug assert
    AM_ASSERT(1 == pInput->mTimeUintNum);


    //initialize params
    pInput->mStartPTS = mStreamStartPTS;
    pInput->mSessionPTSStartPoint = mStreamStartPTS;
    pInput->mCurrentPTS = mStreamStartPTS;
    pInput->mCurrentDTS = mStreamStartPTS;
    pInput->mSessionInputPTSStartPoint = 0;

    pInput->mCurrentFrameCount = 0;
    pInput->mTotalFrameCount = 0;
    pInput->mSession = 0;
    pInput->mDuration = pInput->mParams.specific.audio.frame_size* IParameters::TimeUnitDen_90khz / pInput->mParams.specific.audio.sample_rate;
    pInput->mPTSDTSGap = pInput->mPTSDTSGapDivideDuration * pInput->mDuration;
    pInput->PrintTimeInfo();

    if(mpConsistentConfig->disable_save_files){
        return ME_OK;
    }
    audio_enc->extradata_size = pInput->mParams.extradata_size;
    audio_enc->extradata = pInput->mParams.p_extradata;
    //audio_enc->bit_rate = pInput->mParams.bitrate;
    audio_enc->sample_rate = pInput->mParams.specific.audio.sample_rate;
    audio_enc->channels = pInput->mParams.specific.audio.channel_number;

    if (1 == audio_enc->channels) {
        audio_enc->channel_layout = CH_LAYOUT_MONO;
    } else if (2 == audio_enc->channels) {
        audio_enc->channel_layout = CH_LAYOUT_STEREO;
    } else {
        AM_ERROR("channel >2 is not supported yet.\n");
        audio_enc->channels = 2;
        audio_enc->channel_layout = CH_LAYOUT_STEREO;
    }

    audio_enc->sample_fmt = (AVSampleFormat)pInput->mParams.specific.audio.sample_format;
    audio_enc->bit_rate = pInput->mParams.specific.audio.bitrate;
    audio_enc->codec_type = AVMEDIA_TYPE_AUDIO;

    AM_ASSERT(AV_SAMPLE_FMT_S16 == audio_enc->sample_fmt);
    //AM_ASSERT(128000 == audio_enc->bit_rate);
    AM_ASSERT(2 == audio_enc->channels || 1 == audio_enc->channels);
    AM_ASSERT(48000 >= audio_enc->sample_rate);

    if (!(codec = avcodec_find_encoder(audio_enc->codec_id))) {
        AMLOG_ERROR("Failed to find audio codec\n");
        return ME_ERROR;
    }
    if (avcodec_open(audio_enc, codec) < 0) {
        AMLOG_ERROR("Failed to open audio codec\n");
        return ME_ERROR;
    }

    AMLOG_PRINTF("&&&&&audio enc info: bit_rate %d, sample_rate %d, channels %d, channel_layout %lld, frame_size %d, flag 0x%x.\n", audio_enc->bit_rate, audio_enc->sample_rate, audio_enc->channels, audio_enc->channel_layout, audio_enc->frame_size, audio_enc->flags);
    AMLOG_PRINTF("&&&&&extradata_size %d, codec_type %d, pix_fmt %d, time base num %d, den %d.\n", audio_enc->extradata_size, audio_enc->codec_type, audio_enc->pix_fmt, audio_enc->time_base.num, audio_enc->time_base.den);
    return ME_OK;
}

AM_ERR CFFMpegMuxer::setSubtitleParametersToMuxer(CFFMpegMuxerInput* pInput)
{
    //to do
    return ME_NOT_SUPPORTED;
}

AM_ERR CFFMpegMuxer::setPrivateDataParametersToMuxer(CFFMpegMuxerInput* pInput)
{
    AVStream* pstream = NULL;

    if(!mpConsistentConfig->disable_save_files){
        AM_ASSERT(mpFormat);
    }
    if (!pInput || (!mpFormat && !mpConsistentConfig->disable_save_files)) {
        AMLOG_ERROR("CFFMpegMuxer::setPrivateDataParametersToMuxer NULL pointer pInput %p, mpFormat %p.\n", pInput, mpFormat);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(pInput->mType == StreamType_PrivateData);
    if (pInput->mType != StreamType_PrivateData) {
        AMLOG_ERROR("CFFMpegMuxer::setPrivateDataParametersToMuxer: pInput's type(%d) is not video.\n", pInput->mType);
        return ME_BAD_PARAM;
    }

    if(!mpConsistentConfig->disable_save_files){
        pstream = pInput->mpStream;
        AM_ASSERT(pstream);
        if (!pstream || !pstream->codec) {
            AMLOG_ERROR("CFFMpegMuxer::setVideoParametersToMuxer NULL pointer pstream %p, pstream->codec %p.\n", pstream, pstream->codec);
            return ME_BAD_PARAM;
        }

        pstream->probe_data.filename = mpFormat->filename;
        AMLOG_PRINTF("[pridata mux]: framerate num %d, den %d.\n", pstream->r_frame_rate.num, pstream->r_frame_rate.den);
        AMLOG_PRINTF("[pridata mux]: time_base num %d, den %d, pInput->mDuration %d.\n", pstream->time_base.num, pstream->time_base.den, pInput->mDuration);

        pstream->time_base.num = 1;
        pstream->time_base.den = 90000;
    }
    pInput->mTimeUintDen = IParameters::TimeUnitDen_90khz;
    pInput->mTimeUintNum = 1;

    //initialize params
    pInput->mStartPTS = mStreamStartPTS;
    pInput->mSessionPTSStartPoint = mStreamStartPTS;//each session start pts
    pInput->mCurrentPTS = mStreamStartPTS;
    pInput->mCurrentDTS = mStreamStartPTS;
    pInput->mSessionInputPTSStartPoint = 0;

    pInput->mCurrentFrameCount = 0;
    pInput->mTotalFrameCount = 0;
    pInput->mSession = 0;

    pInput->mExpectedDuration = pInput->mDuration;
    pInput->mPTSDTSGap = 0;
    AMLOG_INFO("mPTSDTSGapDivideDuration %d, mDuration%d, mPTSDTSGap %d.\n", pInput->mPTSDTSGapDivideDuration, pInput->mDuration, pInput->mPTSDTSGap);
    pInput->PrintTimeInfo();

    return ME_OK;
}

bool CFFMpegMuxer::allInputEos()
{
    AM_UINT index;
    for (index = 0; index < mTotalInputPinNumber; index ++) {
        AM_ASSERT(mpInput[index]);
        if (mpInput[index]) {
            if (mpInput[index]->mEOSComes == 0) {
                return false;
            }
        } else {
            AMLOG_ERROR("NULL mpInput[%d], should have error.\n", index);
        }
    }
    return true;
}

bool CFFMpegMuxer::isCommingPTSNotContinuous(CFFMpegMuxerInput* pInput, CBuffer *pBuffer)
{
    AM_ASSERT(pInput);
    AM_ASSERT(pBuffer);

    AM_U64 pts = pBuffer->GetPTS() - pInput->mSessionInputPTSStartPoint + pInput->mSessionPTSStartPoint;
    if ((pInput->mCurrentDTS > (pts + MaxDTSDrift * pInput->mDuration)) || (pts > (pInput->mCurrentDTS + MaxDTSDrift * pInput->mDuration))) {
        AMLOG_WARN("PTS gap more than %d x duration, DTS %lld, PTS %lld, BufferPTS %lld ,duration %d, up-stream filter discarded packet? re-align pts.\n",\
            MaxDTSDrift, pInput->mCurrentDTS, pts, pBuffer->GetPTS(),pInput->mDuration);
        return true;
    }
    return false;
}

void CFFMpegMuxer::writeVideoBuffer(CFFMpegMuxerInput* pInput, CBuffer *pBuffer)
{
#ifdef __print_time_info__
    struct timeval tv_current_start, tv_current_end;
#endif

    if (!pBuffer->GetDataPtr()) {
        AM_ERROR("NULL Data pointer, type %d?\n", pBuffer->GetType());
        return;
    }

    AVPacket packet;
    AM_INT ret = 0;

    AM_ASSERT(pInput);
    AM_ASSERT(pBuffer);
    AM_ASSERT(pInput->mType == StreamType_Video);

    AMLOG_DEBUG("writeVideoBuffer ...., pBuffer->mFrameType %d, pBuffer->GetPTS() %lld\n ", pBuffer->mFrameType, pBuffer->GetPTS());

    //check the if frame is expired
    if (mpClockManager) {
        AM_U64 curtime = mpClockManager->GetCurrentTime();
        if (curtime > pBuffer->mExpireTime) {
            AMLOG_WARN(" Frame Expired, write data too slow? cur time %llu, expired time %llu.\n", curtime, pBuffer->mExpireTime);
            msState = STATE_FLUSH_EXPIRED_FRAME;
            return;
        } else {
            //debug print
            AMLOG_DEBUG(" Frame not Expired, cur time %llu, expired time %llu.\n", curtime, pBuffer->mExpireTime);
        }
    }

    av_init_packet(&packet);
    packet.stream_index = pInput->mIndex;
    packet.size = pBuffer->GetDataSize();
    packet.data = pBuffer->GetDataPtr();

    //get first time threshold
    if (false == mbNextFileTimeThresholdSet && IParameters::MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy) {
        mVideoFirstPTS = pBuffer->GetPTS();
        if (IParameters::MuxerSavingCondition_InputPTS == mSavingCondition) {
            //start from input pts
            mNextFileTimeThreshold = pBuffer->GetPTS() + mAutoSavingTimeDuration;
            //audio stream first pts is 0
            mAudioNextFileTimeThreshold = mNextFileTimeThreshold - mVideoFirstPTS;
        } else if (IParameters::MuxerSavingCondition_CalculatedPTS == mSavingCondition) {
            //linear to start from 0
            mNextFileTimeThreshold = mAutoSavingTimeDuration;
            mAudioNextFileTimeThreshold = mNextFileTimeThreshold;
        }
        mbNextFileTimeThresholdSet = true;
        AMLOG_INFO("Get first Video PTS %llu.\n", pBuffer->GetPTS());
    }

    //ensure first frame are key frame
    if (pInput->mCurrentFrameCount == 0) {
        //AM_ASSERT(pBuffer->mFrameType == PredefinedPictureType_IDR);
        if (pBuffer->mFrameType == PredefinedPictureType_IDR) {
            AM_U8* pseq = packet.data;
            //extradata sps
            AMLOG_INFO("Beginning to calculate sps-pps's length.\n");
#if 0
            //there maybe trash data before sps-pps. and there maybe AUD before sps and SEI after pps.
            //AUD size is 6 usually, and SEI size is 36.
            AM_UINT DataOffset = 0;
            pInput->mParams.extradata_size = GetSpsPpsLen(pBuffer->GetDataPtr(), &DataOffset);
            if(DataOffset != 0){
                AM_ASSERT(DataOffset + 5 < packet.size);
                AMLOG_WARN("SPS offset = %u, size=%u.\n", DataOffset, packet.size);
                packet.size = pBuffer->GetDataSize() - DataOffset;
                pBuffer->SetDataSize(packet.size);
                packet.data = pBuffer->GetDataPtr() + DataOffset;
                pBuffer->SetDataPtr(packet.data);
                pseq = packet.data;
            }
            AM_WARNING("[debug info], in %p, size %d, pseq %p, size %d, DataOffset %d\n", pBuffer->GetDataPtr(), pBuffer->GetDataSize(), pseq, pInput->mParams.extradata_size, DataOffset);
            AM_WARNING("pseq: [%02x %02x %02x %02x, %02x %02x %02x %02x]\n", pseq[0], pseq[1], pseq[2], pseq[3], pseq[4], pseq[5], pseq[6], pseq[7]);
#else
            //get seq data for safe, not exceed boundary when data is corrupted by DSP
            AM_U8* pIDR = NULL;
            AM_U8* pstart = NULL;
            bool find_seq = _get_sps_pps(pBuffer->GetDataPtr(), pBuffer->GetDataSize(), pseq, pInput->mParams.extradata_size, pIDR, pstart);

            if (true == find_seq) {
                if (!pstart) {
                    AM_ASSERT(pBuffer->GetDataPtr() == pseq);
                    packet.data = pseq;
                    packet.size  -= (AM_U32)pseq - (AM_U32)pBuffer->GetDataPtr();
                    pBuffer->SetDataSize(packet.size);
                    pBuffer->SetDataPtr(packet.data);
                } else {
                    packet.data = pstart;
                    packet.size  -= (AM_U32)pstart - (AM_U32)pBuffer->GetDataPtr();
                    pBuffer->SetDataSize(packet.size);
                    pBuffer->SetDataPtr(packet.data);
                }
                AM_WARNING("[debug info], in %p, size %d, pseq %p, pIDR %p, pstart %p\n", pBuffer->GetDataPtr(), pBuffer->GetDataSize(), pseq, pIDR, pstart);
                AM_WARNING("pseq: [%02x %02x %02x %02x, %02x %02x %02x %02x]\n", pseq[0], pseq[1], pseq[2], pseq[3], pseq[4], pseq[5], pseq[6], pseq[7]);
                AM_WARNING("pIDR: [%02x %02x %02x %02x, %02x %02x %02x %02x]\n", pIDR[0], pIDR[1], pIDR[2], pIDR[3], pIDR[4], pIDR[5], pIDR[6], pIDR[7]);
                if (pstart) {
                    AM_WARNING("pstart: [%02x %02x %02x %02x, %02x %02x %02x %02x]\n", pstart[0], pstart[1], pstart[2], pstart[3], pstart[4], pstart[5], pstart[6], pstart[7]);
                }
            } else {
                //AM_ERROR("IDR data buffer corrupted!!, how to handle it?\n");
                //msState = STATE_ERROR;
                //return;
            }
#endif
            AMLOG_INFO("Calculate sps-pps's length completely, len=%d.\n", pInput->mParams.extradata_size);
            AM_ASSERT(pInput->mParams.extradata_size <= SPS_PPS_LEN);
            AM_ASSERT(pInput->mpPrivateData);
            if (!pInput->mpPrivateData) {
                pInput->mpPrivateData = (AM_U8*)av_mallocz(pInput->mParams.extradata_size + 8);
            }

            if (pInput->mpPrivateData) {
                memcpy(pInput->mpPrivateData, pseq, pInput->mParams.extradata_size + 8);
            } else {
                AM_ASSERT(0);
                pInput->mParams.extradata_size = 0;
            }

            AMLOG_INFO("I picture comes, pInput->mDuration %d, pBuffer->GetPTS() %lld.\n", pInput->mDuration, pBuffer->GetPTS());

            AM_ASSERT(pInput->mDuration);
            //refresh session pts related
            pInput->mSessionInputPTSStartPoint = pBuffer->GetPTS();
            if (pInput->mCurrentPTS != mStreamStartPTS) {
                pInput->mSessionPTSStartPoint = pInput->mCurrentPTS;
            }
            AMLOG_INFO(" video pInput->mSessionInputPTSStartPoint %lld, pInput->mSessionPTSStartPoint %lld.\n", pInput->mSessionInputPTSStartPoint, pInput->mSessionPTSStartPoint);
            if (!mbMasterStarted) {
                AMLOG_INFO("master started.\n");
                mbMasterStarted = true;

                //generate thumbnail file for first file here
                if (mpSharedRes->encoding_mode_config.thumbnail_enabled && mpThumbNailFileName) {
                    AM_MSG msg;
                    msg.code = IEngine::MSG_GENERATE_THUMBNAIL;
                    msg.p1  = (AM_INTPTR)static_cast<IFilter*>(this);
                    msg.p3 = (AM_INTPTR)mpThumbNailFileName;
                    PostEngineMsg(msg);
                }
            }
        } else {
            AMLOG_WARN("non-IDR frame (%d), data comes here, seq num [%d, %d], when stream not started, discard them.\n", pBuffer->mFrameType, pBuffer->mOriSeqNum, pBuffer->mSeqNum);
            return;
        }
    } else if (true == isCommingPTSNotContinuous(pInput, pBuffer)) {
        //refresh pts, connected to previous
        pInput->mSessionInputPTSStartPoint = pBuffer->GetPTS();
        pInput->mSessionPTSStartPoint = pInput->mCurrentPTS + pInput->mDuration;
        pInput->mCurrentFrameCount = 0;
    }

    pInput->mFrameCountFromLastBPicture ++;

    if (!pInput->mbAutoBoundaryStarted) {
        pInput->mbAutoBoundaryStarted = true;
        pInput->mInputSatrtPTS = pBuffer->GetPTS();
        AMLOG_INFO("[cut file, boundary start(video), pts %llu]", pInput->mInputSatrtPTS);
    }
    pInput->mInputEndPTS = pBuffer->GetPTS();
    mSyncAudioPTS.lastVideoPTS = pInput->mInputEndPTS;

    if (pBuffer->GetPTS() < pInput->mSessionInputPTSStartPoint) {
        AM_ASSERT(pBuffer->mFrameType == PredefinedPictureType_B);
        AMLOG_WARN(" PTS(%lld) < first IDR PTS(%lld), something should be wrong. or skip b frame(pBuffer->mFrameType %d) after first IDR, these b frame should be skiped, when pause--->resume.\n", pBuffer->GetPTS(), pInput->mSessionInputPTSStartPoint, pBuffer->mFrameType);
        return;
    }

    AM_ASSERT(pBuffer->GetPTS() >= pInput->mSessionInputPTSStartPoint);
    pInput->mCurrentPTS = pBuffer->GetPTS() - pInput->mSessionInputPTSStartPoint + pInput->mSessionPTSStartPoint;
    AMLOG_PTS("[PTS]:video mCurrentPTS %lld, mCurrentDTS %lld, pBuffer->GetPTS() %lld, mSessionInputPTSStartPoint %lld, mSessionPTSStartPoint %lld.\n", pInput->mCurrentPTS, pInput->mCurrentDTS, pBuffer->GetPTS(), pInput->mSessionInputPTSStartPoint, pInput->mSessionPTSStartPoint);

    if (pBuffer->mFrameType == PredefinedPictureType_IDR || pBuffer->mFrameType == PredefinedPictureType_I) {
        packet.flags |= AV_PKT_FLAG_KEY;
    }

    //muxer error conclealment code, maybe ugly
#if PLATFORM_ANDROID_ITRON
    //correct wrong pts duration(29.97fps may comes out pts gap 1501, not 3003)
    if (pBuffer->mFrameType = PredefinedPictureType_B) {
        //check the if the duration make sence
        if ((pInput->mCurrentDTS > (pInput->mCurrentPTS + MaxDTSDrift * pInput->mDuration)) || (pInput->mCurrentPTS > (pInput->mCurrentDTS + MaxDTSDrift * pInput->mDuration))) {
            AMLOG_WARN("DTS drift more than %d x duration, DTS %lld, PTS %lld, duration %d, \n", MaxDTSDrift, pInput->mCurrentDTS, pInput->mCurrentPTS, pInput->mDuration);
            if (pInput->mFrameCountFromLastBPicture && pInput->mLastBPicturePTS && (pInput->mCurrentPTS > pInput->mLastBPicturePTS)) {
                pInput->mDuration = (pInput->mCurrentPTS - pInput->mLastBPicturePTS) / pInput->mFrameCountFromLastBPicture;
            }
            AMLOG_WARN("modified duration %d (expected one %d), pInput->mFrameCountFromLastBPicture %d, last B's pts %lld.\n", pInput->mDuration, pInput->mExpectedDuration, pInput->mFrameCountFromLastBPicture, pInput->mLastBPicturePTS);
        }

        //sync dts when B pitcute comes
        //pInput->mCurrentDTS = pInput->mCurrentPTS;
        pInput->mLastBPicturePTS = pInput->mCurrentPTS;
        pInput->mFrameCountFromLastBPicture = 0;
        //AMLOG_PTS("SYNC DTS to PTS (%lld).\n", pInput->mCurrentDTS);
        AMLOG_DEBUG("current duration %d.\n", pInput->mDuration);
    }
#endif

    if (pInput->mNeedModifyPTS == 0 ) {
        packet.dts = pInput->mCurrentDTS;
        packet.pts = pInput->mCurrentPTS + pInput->mPTSDTSGap;
    } else {
        packet.dts = av_rescale_q(pInput->mCurrentDTS, (AVRational){1,IParameters::TimeUnitDen_90khz}, mpFormat->streams[packet.stream_index]->time_base);
        packet.pts = av_rescale_q(pInput->mCurrentPTS + pInput->mPTSDTSGap, (AVRational){1,IParameters::TimeUnitDen_90khz}, mpFormat->streams[packet.stream_index]->time_base);
    }
    packet.duration = pInput->mAVNormalizedDuration;

    pInput->mCurrentDTS += pInput->mDuration;
    AMLOG_PTS("[PTS]:write to file, video pts %lld, dts %lld, duration %d, pts gap %lld.\n", packet.pts, packet.dts, pInput->mDuration, packet.pts - pInput->mLastPTS);
    pInput->mLastPTS = packet.pts;

#ifdef __print_time_info__
    gettimeofday(&tv_current_start,NULL);
    if (tv_current_start.tv_usec >= tv_last_buffer.tv_usec) {
        AMLOG_PRINTF("[Muxer] time gap from previous buffer %d sec, %d usec.\n", tv_current_start.tv_sec - tv_last_buffer.tv_sec, tv_current_start.tv_usec - tv_last_buffer.tv_usec);
    } else {
        AMLOG_PRINTF("[Muxer] time gap from previous buffer %d sec, %d usec.\n", tv_current_start.tv_sec - tv_last_buffer.tv_sec - 1, tv_current_start.tv_usec + 1000000 - tv_last_buffer.tv_usec);
    }
#endif

    mCurrentTotalFilesize += packet.size;
    //AMLOG_WARN("packet->data %p, packet.size %d.\n", packet.data, packet.size);
    if ((!mpConsistentConfig->disable_save_files)  && (mpConsistentConfig->muxer_skip_video == 0)) {
        if ((ret = av_write_frame(mpFormat, &packet)) != 0) {

            //tmp code here, need modify av_write_frame's ret value, get correct status of storage full?
            if (mpFormat->pb->error) {

                //post msg to engine
                AM_MSG msg;
                msg.code = IEngine::MSG_STORAGE_ERROR;
                msg.p1  = (AM_INTPTR)static_cast<IFilter*>(this);
                msg.p2 = mFirstFileIndexCanbeDeleted;
                if (true == isHD) {
                    msg.p3 = 0;
                } else {
                    msg.p3 = 1;
                }
                PostEngineMsg(msg);

//do not delete file automatically
/*
                if (mCurrentFileIndex > mFirstFileIndexCanbeDeleted) {
                    AMLOG_WARN("start auto delete file, current index %d, first index %d, mTotalFileNumber %d.\n", mCurrentFileIndex, mFirstFileIndexCanbeDeleted, mTotalFileNumber);
                    deletePartialFile(mFirstFileIndexCanbeDeleted ++);
                    mTotalFileNumber --;
                }
*/
                AMLOG_ERROR("writeVideoBuffer (IO) error.\n");
            }else{
                AM_MSG msg;
                msg.code = IEngine::MSG_ERROR;
                msg.p1  = (AM_INTPTR)static_cast<IFilter*>(this);
                PostEngineMsg(msg);
            }
            msState = STATE_ERROR;
            AMLOG_ERROR("writeVideoBuffer error, ret %d!\n", ret);
            return;
        }
    }

    //AMLOG_INFO(" 1tmp %p, %u.\n", pBuffer->GetDataPtr(), pBuffer->GetDataSize());
    //streaming out if needed
    {
        AUTO_LOCK(mpMutex);
        if (mbStreammingEnabled){
            if (mVideoDstAddressList.NumberOfNodes()) {
                rtpProcessData(pBuffer->GetDataPtr(), pBuffer->GetDataSize(),(pBuffer->GetPTS() & 0xffffffff), IParameters::StreamFormat_H264);
            }
        }
    }

#ifdef __print_time_info__
    gettimeofday(&tv_current_end,NULL);
    if (tv_current_end.tv_usec >= tv_current_start.tv_usec) {
        AMLOG_PRINTF("[Muxer] time gap writing buffer %d sec, %d usec.\n", tv_current_end.tv_sec - tv_current_start.tv_sec, tv_current_end.tv_usec - tv_current_start.tv_usec);
    } else {
        AMLOG_PRINTF("[Muxer] time gap writing buffer %d sec, %d usec.\n", tv_current_end.tv_sec - tv_current_start.tv_sec - 1, tv_current_end.tv_usec + 1000000 - tv_current_start.tv_usec);
    }
    tv_last_buffer = tv_current_end;
#endif

    if (mpConsistentConfig->muxer_dump_video) {
        AM_ASSERT(pInput->mpDumper);
        if (pInput->mpDumper) {
            pInput->mpDumper->WriteFile((void*)packet.data, (AM_UINT)packet.size);
        }
    }

    pInput->mCurrentFrameCount ++;
    pInput->mTotalFrameCount ++;

    AMLOG_VERBOSE("writeVideoBuffer done,size:%d,pts:%lld, time base num %d, den %d, frame rate num %d, den %d.\n",packet.size,packet.pts, mpFormat->streams[packet.stream_index]->time_base.num, mpFormat->streams[packet.stream_index]->time_base.den, mpFormat->streams[packet.stream_index]->r_frame_rate.num, mpFormat->streams[packet.stream_index]->r_frame_rate.den);
}

void CFFMpegMuxer::writeAudioBuffer(CFFMpegMuxerInput* pInput, CBuffer *pBuffer)
{
    AVPacket packet;
    AM_INT ret = 0;
    AM_UINT header_length = 0;

    AM_ASSERT(pInput);
    AM_ASSERT(pBuffer);
    AM_ASSERT(pInput->mType == StreamType_Audio);

    AMLOG_DEBUG("writeAudioBuffer pBuffer->GetPTS() %lld, size %d...\n", pBuffer->GetPTS(), pBuffer->GetDataSize());
    av_init_packet(&packet);

    if (pInput->mParams.specific.audio.need_skip_adts_header) {
        header_length = _adts_header_length(pBuffer->GetDataPtr());
        AMLOG_DEBUG("adts header length %d.\n", header_length);
    }

    packet.stream_index = pInput->mIndex;
    packet.size = pBuffer->GetDataSize() - header_length;
    packet.data = pBuffer->GetDataPtr() + header_length;
    packet.pts = pBuffer->GetPTS();
    packet.flags |= AV_PKT_FLAG_KEY;
    //AMLOG_PTS("audio pts %lld.\n", packet.pts);

    //ensure first frame are key frame
    if (pInput->mCurrentFrameCount == 0) {
        AM_ASSERT(pInput->mDuration);
        //estimate current pts
        pInput->mSessionInputPTSStartPoint = pBuffer->GetPTS();
        if (pInput->mCurrentPTS != mStreamStartPTS) {
            pInput->mSessionPTSStartPoint = pInput->mCurrentPTS;
        }
        AMLOG_INFO(" audio mSessionInputPTSStartPoint %lld, mSessionPTSStartPoint %lld, pInput->mDuration %d.\n", pInput->mSessionInputPTSStartPoint, pInput->mSessionPTSStartPoint, pInput->mDuration);
    }

    if (!pInput->mbAutoBoundaryStarted) {
        pInput->mbAutoBoundaryStarted = true;
        pInput->mInputSatrtPTS = pBuffer->GetPTS();
        AMLOG_INFO("[cut file, boundary start(audio), pts %llu]", pInput->mInputSatrtPTS);
    }
    pInput->mInputEndPTS = pBuffer->GetPTS();

    if (false == mbAudioHALClosed)
        mSyncAudioPTS.lastAudioPTS = pInput->mInputEndPTS;

    mAudioLastPTS = pBuffer->GetPTS();

    pInput->mCurrentPTS = pBuffer->GetPTS() - pInput->mSessionInputPTSStartPoint + pInput->mSessionPTSStartPoint;
    AMLOG_PTS("[PTS]:audio mCurrentPTS %lld, pBuffer->GetPTS() %lld, pInput->mTimeUintNum %d, pInput->mTimeUintDen %d.\n", pInput->mCurrentPTS, pBuffer->GetPTS(), pInput->mTimeUintNum, pInput->mTimeUintDen);
    //AMLOG_INFO("[PTS]: time_base.num %d, den %d.\n", mpFormat->streams[packet.stream_index]->time_base.num, mpFormat->streams[packet.stream_index]->time_base.den);

    if (!pInput->mNeedModifyPTS) {
        //packet.dts = pInput->mCurrentPTS;
        packet.dts = packet.pts = pInput->mCurrentPTS + pInput->mPTSDTSGap;
    } else {
        //packet.dts = av_rescale_q(pInput->mCurrentPTS, (AVRational){pInput->mTimeUintNum,pInput->mTimeUintDen}, mpFormat->streams[packet.stream_index]->time_base);
        packet.dts = packet.pts = av_rescale_q(pInput->mCurrentPTS + pInput->mPTSDTSGap, (AVRational){(int)pInput->mTimeUintNum,(int)pInput->mTimeUintDen}, mpFormat->streams[packet.stream_index]->time_base);
    }

    AMLOG_PTS("[PTS]:audio write to file pts %lld, dts %lld, pts gap %lld, duration %d, pInput->mPTSDTSGap %d.\n", packet.pts, packet.dts, packet.pts - pInput->mLastPTS, pInput->mDuration, pInput->mPTSDTSGap);
    pInput->mLastPTS = packet.pts;
    packet.duration = pInput->mAVNormalizedDuration;
    mCurrentTotalFilesize += packet.size;

    //mRTPAudioTimeStamp = pBuffer->GetPTS() & 0xffffffff;
    {
        AUTO_LOCK(mpMutex);
        if(mbStreammingEnabled){
            mRTPAudioTimeStamp += 1024;//aac trunck size, hard code here
            if (mAudioDstAddressList.NumberOfNodes()) {
                rtpProcessData(packet.data, packet.size, mRTPAudioTimeStamp,IParameters::StreamFormat_AAC);
            }
        }
    }

    if ((!mpConsistentConfig->disable_save_files) && (mpConsistentConfig->muxer_skip_audio == 0)) {
        if ((ret = av_write_frame(mpFormat, &packet)) != 0) {
            //tmp code here, need modify av_write_frame's ret value, get correct status of storage full?
            if (mpFormat->pb->error) {

                //post msg to engine
                AM_MSG msg;
                msg.code = IEngine::MSG_STORAGE_ERROR;
                msg.p1  = (AM_INTPTR)static_cast<IFilter*>(this);
                msg.p2 = mFirstFileIndexCanbeDeleted;
                if (true == isHD) {
                    msg.p3 = 0;
                } else {
                    msg.p3 = 1;
                }
                PostEngineMsg(msg);
//do not delete file automatically
/*
                if (mCurrentFileIndex > mFirstFileIndexCanbeDeleted) {
                    AMLOG_WARN("start auto delete file, current index %d, first index %d, mTotalFileNumber %d.\n", mCurrentFileIndex, mFirstFileIndexCanbeDeleted, mTotalFileNumber);
                    deletePartialFile(mFirstFileIndexCanbeDeleted ++);
                    mTotalFileNumber --;
                }
*/
                AMLOG_ERROR("writeAudioBuffer (IO) error.\n");
            }
            AMLOG_ERROR("writeAudioBuffer error, ret %d!\n", ret);
            msState = STATE_ERROR;
            return;
        }
    }

    if (mpConsistentConfig->muxer_dump_audio) {
        AM_ASSERT(pInput->mpDumper);
        if (pInput->mpDumper) {
            pInput->mpDumper->WriteFile((void*)packet.data, (AM_UINT)packet.size);
        }
    }

    pInput->mCurrentFrameCount ++;
    pInput->mTotalFrameCount ++;
    AMLOG_DEBUG("writeAudioBuffer done,size:%d,pts:%lld, time base num %d, den %d.\n",packet.size,packet.pts, mpFormat->streams[packet.stream_index]->time_base.num, mpFormat->streams[packet.stream_index]->time_base.den);
}

void CFFMpegMuxer::writePridataBuffer(CFFMpegMuxerInput* pInput, CBuffer *pBuffer)
{
    AVPacket packet;
    AM_INT ret = 0;
    AM_UINT header_length = 0;

    AM_ASSERT(pInput);
    AM_ASSERT(pBuffer);
    AM_ASSERT(pInput->mType == StreamType_PrivateData);

    AMLOG_DEBUG("writePridataBuffer pBuffer->GetPTS() %lld...\n", pBuffer->GetPTS());
    av_init_packet(&packet);

    packet.stream_index = pInput->mIndex;
    packet.size = pBuffer->GetDataSize() - header_length;
    packet.data = pBuffer->GetDataPtr() + header_length;
    packet.pts = pBuffer->GetPTS();
    packet.flags |= AV_PKT_FLAG_KEY;

    //ensure first frame are key frame
    if (pInput->mCurrentFrameCount == 0) {
        AM_ASSERT(pInput->mDuration);
        //estimate current pts
        pInput->mSessionInputPTSStartPoint = pBuffer->GetPTS();
        if (pInput->mCurrentPTS != mStreamStartPTS) {
            pInput->mSessionPTSStartPoint = pInput->mCurrentPTS;
        }
        AMLOG_INFO(" pridata mSessionInputPTSStartPoint %lld, mSessionPTSStartPoint %lld, pInput->mDuration %d.\n", pInput->mSessionInputPTSStartPoint, pInput->mSessionPTSStartPoint, pInput->mDuration);
    }

    if (!pInput->mbAutoBoundaryStarted) {
        pInput->mbAutoBoundaryStarted = true;
        pInput->mInputSatrtPTS = pBuffer->GetPTS();
        AMLOG_INFO("[cut file, boundary start(private data), pts %llu]", pInput->mInputSatrtPTS);
    }
    pInput->mInputEndPTS = pBuffer->GetPTS();

    //AMLOG_INFO("pInput->mTimeUintNum %d,pInput->mTimeUintDen %d.\n", pInput->mTimeUintNum, pInput->mTimeUintDen);
    pInput->mCurrentPTS = pBuffer->GetPTS() - pInput->mSessionInputPTSStartPoint + pInput->mSessionPTSStartPoint;
    AMLOG_PTS("[PTS]:pridata mCurrentPTS %lld, pBuffer->GetPTS() %lld, pInput->mTimeUintNum %d, pInput->mTimeUintDen %d.\n", pInput->mCurrentPTS, pBuffer->GetPTS(), pInput->mTimeUintNum, pInput->mTimeUintDen);
    packet.dts = pInput->mCurrentPTS;//av_rescale_q(pInput->mCurrentPTS + pInput->mPTSDTSGap, (AVRational){pInput->mTimeUintNum,pInput->mTimeUintDen}, mpFormat->streams[packet.stream_index]->time_base);
    packet.pts = pInput->mCurrentPTS + pInput->mPTSDTSGap;

    //should set valid duration
    packet.duration = packet.pts - pInput->mLastPTS;
    if (((AM_UINT)packet.duration > 4*pInput->mAVNormalizedDuration) || (packet.duration <= 0)) {
        packet.duration = pInput->mAVNormalizedDuration;
    }
    AMLOG_PTS("[PTS]:mLastPTS %llu, mAVNormalizedDuration %d.\n", pInput->mLastPTS, pInput->mAVNormalizedDuration);
    AMLOG_PTS("[PTS]:pridata write to file pts %lld, dts %lld, pts gap %lld, packet.duration %d, duration %d, pBuffer->GetPTS() %llu.\n", packet.pts, packet.dts, packet.pts - pInput->mLastPTS, packet.duration, pInput->mDuration, pBuffer->GetPTS());
    pInput->mLastPTS = packet.pts;
    mCurrentTotalFilesize += packet.size;

    if ((!mpConsistentConfig->disable_save_files) && (mpConsistentConfig->muxer_skip_pridata == 0)) {
        if ((ret = av_write_frame(mpFormat, &packet)) != 0) {
            //tmp code here, need modify av_write_frame's ret value, get correct status of storage full?
            if (mpFormat->pb->error) {
                //post msg to engine
                AM_MSG msg;
                msg.code = IEngine::MSG_STORAGE_ERROR;
                msg.p1  = (AM_INTPTR)static_cast<IFilter*>(this);
                msg.p2 = mFirstFileIndexCanbeDeleted;
                if (true == isHD) {
                    msg.p3 = 0;
                } else {
                    msg.p3 = 1;
                }
                PostEngineMsg(msg);
//do not delete file automatically
/*
                if (mCurrentFileIndex > mFirstFileIndexCanbeDeleted) {
                    AMLOG_WARN("start auto delete file, current index %d, first index %d, mTotalFileNumber %d.\n", mCurrentFileIndex, mFirstFileIndexCanbeDeleted, mTotalFileNumber);
                    deletePartialFile(mFirstFileIndexCanbeDeleted ++);
                    mTotalFileNumber --;
                }
*/
                AMLOG_ERROR("writePridataBuffer (IO) error.\n");
            }
            msState = STATE_ERROR;
            AMLOG_ERROR("writePridataBuffer error, ret %d!\n", ret);
            return;
        }
    }

    if (mpConsistentConfig->muxer_dump_pridata) {
        AM_ASSERT(pInput->mpDumper);
        if (pInput->mpDumper) {
            pInput->mpDumper->WriteFile((void*)packet.data, (AM_UINT)packet.size);
        }
    }

    pInput->mCurrentFrameCount ++;
    pInput->mTotalFrameCount ++;
    AMLOG_DEBUG("writePridataBuffer done,size:%d,pts:%lld, time base num %d, den %d.\n",packet.size,packet.pts, mpFormat->streams[packet.stream_index]->time_base.num, mpFormat->streams[packet.stream_index]->time_base.den);
}

AM_ERR CFFMpegMuxer::AddDstAddress(SDstAddr* p_addr, IParameters::StreamType type)
{
    AUTO_LOCK(mpMutex);
    if (p_addr) {
        memset(&p_addr->rtcp_stat,0,sizeof(p_addr->rtcp_stat));
        p_addr->rtcp_stat.first_packet = 1;
        if (IParameters::StreamType_Video == type) {
            AMLOG_INFO("CFFMpegMuxer::AddDstAddress(video): type %d, addr %d, socket 0x%x, port %u, port_ext %u.\n", p_addr->protocol_type, p_addr->addr, p_addr->socket, p_addr->port, p_addr->port_ext);
            mVideoDstAddressList.InsertContent(NULL, (void*)p_addr, 0);
        } else if (IParameters::StreamType_Audio == type) {
            AMLOG_INFO("CFFMpegMuxer::AddDstAddress(audio): type %d, addr %d, socket 0x%x, port %u, port_ext %u.\n", p_addr->protocol_type, p_addr->addr, p_addr->socket, p_addr->port, p_addr->port_ext);
            mAudioDstAddressList.InsertContent(NULL, (void*)p_addr, 0);
        } else {
            AM_ERROR("BAD type %d.\n", type);
            return ME_BAD_PARAM;
        }
        if(mVideoDstAddressList.NumberOfNodes() + mAudioDstAddressList.NumberOfNodes() == 1){
            startRTPSendTask();
            AM_WARNING("CFFMpegMuxer::AddDstAddress ---- startRTPSendTask\n");
        }
        return ME_OK;
    }
    AM_ERROR("NULL pointer in CFFMpegMuxer::AddDstAddress.\n");
    return ME_BAD_PARAM;
}

AM_ERR CFFMpegMuxer::RemoveDstAddress(SDstAddr* p_addr, IParameters::StreamType type)
{
    AUTO_LOCK(mpMutex);
    if (p_addr) {
        if (IParameters::StreamType_Video == type) {
            AMLOG_INFO("CFFMpegMuxer::RemoveDstAddress(video): type %d, p_addr 0x%x, socket %d, port %u, port_ext %u.\n", p_addr->protocol_type, p_addr->addr, p_addr->socket, p_addr->port, p_addr->port_ext);
            mVideoDstAddressList.RemoveContent((void*)p_addr);
        } else if (IParameters::StreamType_Audio == type) {
            AMLOG_INFO("CFFMpegMuxer::RemoveDstAddress(audio): type %d, p_addr 0x%x, socket %d, port %u, port_ext %u.\n", p_addr->protocol_type, p_addr->addr, p_addr->socket, p_addr->port, p_addr->port_ext);
            mAudioDstAddressList.RemoveContent((void*)p_addr);
        } else {
            AM_ERROR("BAD type %d.\n", type);
            return ME_BAD_PARAM;
        }
        if(mVideoDstAddressList.NumberOfNodes() + mAudioDstAddressList.NumberOfNodes() == 0){
            mbStreammingEnabled = false;
            stopRTPSendTask();
            AMLOG_WARN("CFFMpegMuxer::RemoveDstAddress --- stopRTPSendTask\n");
        }
        return ME_OK;
    }
    AM_ERROR("NULL pointer in CFFMpegMuxer::RemoveDstAddress.\n");
    return ME_BAD_PARAM;
}

AM_ERR CFFMpegMuxer::SetSrcPort(AM_U16 port, AM_U16 port_ext, IParameters::StreamType type)
{
    AUTO_LOCK(mpMutex);
    if (IParameters::StreamType_Video == type) {
        if (false == mbRTPSocketSetup) {
            AMLOG_INFO("CFFMpegMuxer::SetSrcPort[video](%hu, %hu).\n", port, port_ext);
            mRTPPort = port;
            mRTCPPort = port_ext;

            setupUDPSocket(type);
            return ME_OK;
        } else {
            AM_ERROR("RTP socket(video) has been setup, port %hu, %hu.\n", mRTPPort, mRTCPPort);
            return ME_ERROR;
        }
    } else if (IParameters::StreamType_Audio == type) {
        if (false == mbAudioRTPSocketSetup) {
            AMLOG_INFO("CFFMpegMuxer::SetSrcPort[audio](%hu, %hu).\n", port, port_ext);
            mAudioRTPPort = port;
            mAudioRTCPPort = port_ext;

            setupUDPSocket(type);
            return ME_OK;
        } else {
            AM_ERROR("RTP socket(audio) has been setup, port %hu, %hu.\n", mAudioRTPPort, mAudioRTCPPort);
            return ME_ERROR;
        }
    }

    AM_ERROR("BAD type %d.\n", type);
    return ME_ERROR;
}

AM_ERR CFFMpegMuxer::GetSrcPort(AM_U16& port, AM_U16& port_ext, IParameters::StreamType type)
{
    AUTO_LOCK(mpMutex);
    if (IParameters::StreamType_Video == type) {
        port = mRTPPort;
        port_ext = mRTCPPort;
        AMLOG_INFO("CFFMpegMuxer::GetSrcPort(%hu, %hu).\n", port, port_ext);
    } else if (IParameters::StreamType_Audio == type) {
        port = mAudioRTPPort;
        port_ext = mAudioRTCPPort;
        AMLOG_INFO("CFFMpegMuxer::GetSrcPort(%hu, %hu).\n", port, port_ext);
    } else {
        AM_ERROR("BAD type %d.\n", type);
    }
    return ME_OK;
}

AM_UINT CFFMpegMuxer::GetSpsPpsLen(AM_U8 *pBuffer, AM_UINT* offset)
{
    AM_ASSERT(pBuffer);
#ifdef AM_DEBUG
    if (!pBuffer) {
        AM_ERROR("NULL pointer in CFFMpegMuxer::GetSpsPpsLen.\n");
        return 0;
    }
#endif
    AM_UINT start_code, times, pos;
    AM_U8 tmp;
    bool bFinddata = false;

    times = pos = 0;
    for (start_code = 0xffffffff; times < 4; pos++) {
        tmp = (AM_U8)pBuffer[pos];
        start_code <<= 8;
        start_code |= (AM_UINT)tmp;
        if (start_code == 0x00000001) {
            if(pBuffer[pos+1] == 0x09 && !bFinddata){//AUD first
                times = 1;
                bFinddata = true;
                *offset = pos - 3;
            }else if(pBuffer[pos+1] == 0x27 && !bFinddata){//SPS first
                times = 2;
                bFinddata = true;
                *offset = pos - 3;
            }else if(bFinddata){
                times++;
            }
        }
    }
    AMLOG_INFO("sps-pps length = %d\n", pos - 4 - *offset);
    return pos - 4 - *offset;
}

void CFFMpegMuxer::SendEOS()
{
    AMLOG_INFO("Post EOS to engine.\n");
    PostEngineMsg(IEngine::MSG_EOS);
}

bool CFFMpegMuxer::ReadInputData(CFFMpegMuxerInput* inputpin, CBuffer::Type& type)
{
    AM_ASSERT(!mpInputBuffer);
    AM_ASSERT(inputpin);
    AM_ERR ret;

    if (!inputpin->PeekBuffer(mpInputBuffer)) {
        AMLOG_ERROR("No buffer?\n");
        return false;
    }

    type = (CBuffer::Type) mpInputBuffer->GetType();

    if (mpInputBuffer->GetType() == CBuffer::EOS) {
        AMLOG_INFO("CFFMpegMuxer %p get EOS.\n", this);
        inputpin->mEOSComes = 1;
        return true;
    } else if (mpInputBuffer->GetType() == CBuffer::FLOW_CONTROL) {
        if (((IFilter::FlowControlType)mpInputBuffer->GetFlags()) == (IFilter::FlowControl_pause)) {
            AMLOG_INFO("pause packet comes.\n");
            inputpin->timeDiscontinue();
            inputpin->mbSkiped = true;

            if(mbMuxerPaused == false){
                mbMuxerPaused = true;
            }else{
                //get second stream's pause packet, finalize file
                AMLOG_INFO("Muxer stream get FlowControl_pause packet, finalize current file!\n");
                ret = Finalize();
                if(ret != ME_OK){
                    AM_ERROR("FlowControl_pause processing, finalize file failed!\n");
                }
            }
        } else if (((IFilter::FlowControlType)mpInputBuffer->GetFlags()) == (IFilter::FlowControl_resume)) {
            AMLOG_INFO("resume packet comes.\n");
            inputpin->mbSkiped = false;

            if(mbMuxerPaused){
                AMLOG_INFO("Muxer get FlowControl_resume packet, initialize new file!\n");
                updateFileName(mTotalFileNumber, true);
                ret = Initialize();
                if(ret != ME_OK){
                    AM_ERROR("FlowControl_resume processing, initialize file failed!\n");
                    inputpin->mbSkiped = true;
                }
                mbMuxerPaused = false;
            }
        } else if (((IFilter::FlowControlType)mpInputBuffer->GetFlags()) == (IFilter::FlowControl_close_audioHAL)) {
            //audio HAL is closed
            mbAudioHALClosed = true;
        } else if (((IFilter::FlowControlType)mpInputBuffer->GetFlags()) == (IFilter::FlowControl_reopen_audioHAL)) {
            mSyncAudioPTS.needmodifyPTS = true;
            mbAudioHALClosed = false;
        } else if (((IFilter::FlowControlType)mpInputBuffer->GetFlags()) == (IFilter::FlowControl_eos)) {
            AMLOG_INFO("CFFMpegMuxer %p get Flowcontrol EOS.\n", this);
            type = CBuffer::EOS;
            inputpin->mEOSComes = 1;
            return true;
        } else {
            AM_ERROR("mpInputBuffer->GetType() %d, mpInputBuffer->GetFlags() %d.\n", mpInputBuffer->GetType(), mpInputBuffer->GetFlags());
        }
        return true;
    }

    //AM_ASSERT(inputpin->mbSkiped == false);
    if (inputpin->mbSkiped == true) {
        AMLOG_WARN("recieved data between flow_control pause and flow_control resume, discard packet here.\n");
        mpInputBuffer->Release();
        mpInputBuffer = NULL;
        return false;
    }

    AM_ASSERT(mpInputBuffer);
    AM_ASSERT(mpInputBuffer->GetDataPtr());
#ifdef AM_DEBUG
    if (!mpInputBuffer->GetDataPtr()) {
        AMLOG_WARN("  Buffer type %d, size %d.\n", mpInputBuffer->GetType(), mpInputBuffer->GetDataSize());
    }
#endif
    AM_ASSERT(mpInputBuffer->GetDataSize());
    return true;
}

bool CFFMpegMuxer::ProcessCmd(CMD& cmd)
{
    AMLOG_CMD("****CFFMpegMuxer::ProcessCmd, cmd.code %d, state %d.\n", cmd.code, msState);
    switch (cmd.code) {
        case CMD_STOP:
            mbRun = false;
            AMLOG_DEBUG("****CFFMpegMuxer::ProcessCmd, STOP cmd.\n");
            CmdAck(ME_OK);
            break;

        case CMD_RUN:
            //re-run
            AMLOG_INFO("CFFMpegMuxer re-Run, state %d.\n", msState);
            AM_ASSERT(STATE_PENDING == msState || STATE_ERROR == msState);
            AM_ERR ret;
            ret = Initialize();
            AM_ASSERT(ret == ME_OK);
            if (ret != ME_OK) {
                AMLOG_ERROR("CFFMpegMuxer re-Run, Initialize fail, enter error state.\n");
                msState = STATE_ERROR;
            } else {
                msState = STATE_IDLE;
            }
            CmdAck(ME_OK);
            break;

        case CMD_PAUSE:
            //todo
            AM_ASSERT(!mbPaused);
            mbPaused = true;
            CmdAck(ME_OK);
            break;

        case CMD_RESUME:
            //todo
            if(msState == STATE_PENDING)
                msState = STATE_IDLE;
            mbPaused = false;
            CmdAck(ME_OK);
            break;

        case CMD_FLUSH:
            //should not comes here
            AM_ASSERT(0);
            msState = STATE_PENDING;
            if (mpInputBuffer) {
                mpInputBuffer->Release();
                mpInputBuffer = NULL;
            }
            CmdAck(ME_OK);
            break;

        case CMD_DELETE_FILE:
            AMLOG_INFO("recieve cmd, delete file here, mTotalFileNumber %d, mFirstFileIndexCanbeDeleted %d.\n", mTotalFileNumber, mFirstFileIndexCanbeDeleted);
            deletePartialFile(mFirstFileIndexCanbeDeleted);
            mTotalFileNumber --;
            mFirstFileIndexCanbeDeleted ++;
            break;

        default:
        AM_ERROR("wrong cmd %d.\n",cmd.code);
    }
    return false;
}

void CFFMpegMuxer::getFileInformation()
{
    AM_UINT i = 0, tot_validpins = 0;
    AM_UINT estimated_bitrate = 0;
    AM_U64 first_stream_start_pts, first_stream_end_pts, first_stream_duration, first_input_duration;
    AM_U64 diff_start_pts, diff_end_pts, diff_duration, diff_input_duration;

    //avsync related print: start pts's difference, end pts's difference, and duration's difference
    if (mpInput[0]) {
        //get first stream's
        first_stream_start_pts = mpInput[0]->mInputSatrtPTS;
        first_stream_end_pts = mpInput[0]->mInputEndPTS;
        first_input_duration = mpInput[0]->mInputEndPTS - mpInput[0]->mInputSatrtPTS + mpInput[0]->mDuration;
        first_stream_duration = mpInput[0]->mCurrentPTS - mpInput[0]->mSessionPTSStartPoint + mpInput[0]->mDuration;

        AMLOG_INFO("[file info]: avsync, first stream start input pts %llu, end input pts %llu, duration %llu.\n", first_stream_start_pts, first_stream_end_pts, first_stream_duration);
        for (i=1; i < mTotalInputPinNumber; i++) {
            if (mpInput[i]) {
                AMLOG_INFO("[file info]: avsync, stream(%d) start input pts %llu, end input pts %llu, duration %llu.\n", i, mpInput[i]->mInputSatrtPTS, mpInput[i]->mInputEndPTS, mpInput[i]->mCurrentPTS + mpInput[i]->mDuration - mpInput[i]->mSessionPTSStartPoint);

                if (IParameters::StreamType_Audio == mpInput[i]->mType) {
                    //audio has a duration gap, buffer in encoder
                    diff_start_pts = mpInput[i]->mInputSatrtPTS - mpInput[i]->mDuration - first_stream_start_pts;
                    diff_end_pts = mpInput[i]->mInputEndPTS - mpInput[i]->mDuration - first_stream_end_pts;
                } else {
                    diff_start_pts = mpInput[i]->mInputSatrtPTS - first_stream_start_pts;
                    diff_end_pts = mpInput[i]->mInputEndPTS - first_stream_end_pts;
                }

                diff_input_duration = mpInput[i]->mInputEndPTS - mpInput[i]->mInputSatrtPTS + mpInput[i]->mDuration - first_input_duration;
                diff_duration = mpInput[i]->mCurrentPTS - mpInput[i]->mSessionPTSStartPoint + mpInput[i]->mDuration - first_stream_duration;
                AMLOG_INFO("[file info]: avsync, stream(%d) differ with first stream: start input pts's diff %lld, end input pts's diff %lld, duration's diff %lld, input duration diff %lld.\n", i, diff_start_pts, diff_end_pts, diff_duration, diff_input_duration);
            }
        }
    }

    //file duration
    mFileDuration = 0;
    for (i=0; i < mTotalInputPinNumber; i++) {
        if (mpInput[i]) {
            if ((IParameters::StreamType_Video == mpInput[i]->mType) || (IParameters::StreamType_Audio == mpInput[i]->mType)) {
                mFileDuration += mpInput[i]->mCurrentPTS - mpInput[i]->mSessionPTSStartPoint + mpInput[i]->mDuration;
                AMLOG_INFO("[file info]: pin(%d)'s duration %llu.\n", i, mpInput[i]->mCurrentPTS - mpInput[i]->mSessionPTSStartPoint + mpInput[i]->mDuration);
                tot_validpins ++;
            }
        }
    }

    if (tot_validpins) {
        mFileDuration /= tot_validpins;
    }
    AMLOG_INFO("[file info]: avg duration is %llu, number of valid pins %d.\n", mFileDuration, tot_validpins);

    //file size
    //mCurrentTotalFilesize;
    AMLOG_INFO("[file info]: mCurrentTotalFilesize %llu.\n", mCurrentTotalFilesize);

    //file bitrate
    estimated_bitrate = 0;
    for (i=0; i < mTotalInputPinNumber; i++) {
        if (mpInput[i]) {
            if ((IParameters::StreamType_Video == mpInput[i]->mType) || (IParameters::StreamType_Audio == mpInput[i]->mType)) {
                estimated_bitrate += mpInput[i]->mParams.bitrate;
                AMLOG_INFO("[file info]: pin(%d)'s bitrate %u.\n", i, mpInput[i]->mParams.bitrate);
            }
        }
    }

    AM_ASSERT(estimated_bitrate);
    AM_ASSERT(mFileDuration);
    mFileBitrate = 0;
    if (mFileDuration) {
        mFileBitrate = (AM_UINT)(((float)mCurrentTotalFilesize*8*IParameters::TimeUnitDen_90khz)/((float)mFileDuration));
        //AMLOG_INFO("((float)mCurrentTotalFilesize*8*IParameters::TimeUnitDen_90khz) %f .\n", ((float)mCurrentTotalFilesize*8*IParameters::TimeUnitDen_90khz));
        //AMLOG_INFO("result %f.\n", (((float)mCurrentTotalFilesize*8*IParameters::TimeUnitDen_90khz)/((float)mFileDuration)));
        //AMLOG_INFO("mFileBitrate %d.\n", mFileBitrate);
    }

    AMLOG_INFO("[file info]: pre-estimated bitrate %u, calculated bitrate %u.\n", estimated_bitrate, mFileBitrate);
}

//video stream is generated by our dsp, parameters(resolution/pts&time unit) are hard coded, but some container not accept pre-set parameters
//we need detect this case, and feed parameters according to container's(like .MKV)
void CFFMpegMuxer::checkVideoParameters()
{
    AM_UINT video_stream_index;
    float frame_rate;

    //find first video stream index
    for (video_stream_index = 0; video_stream_index< mTotalInputPinNumber; video_stream_index++) {
        if (mpInput[video_stream_index]->mType == IParameters::StreamType_Video) {
            break;
        }
    }
    if (video_stream_index >= mTotalInputPinNumber) {
        //not found
        return;
    }

    frame_rate = ((float)mpFormat->streams[video_stream_index]->r_frame_rate.num)/mpFormat->streams[video_stream_index]->r_frame_rate.den;
    AMLOG_DEBUG("frame rate num %d, den %d, frame rate %f.\n", mpFormat->streams[video_stream_index]->r_frame_rate.den, mpFormat->streams[video_stream_index]->r_frame_rate.num, frame_rate);

    //frame rate is wrong?
    if (frame_rate < 1 || frame_rate > 241) {
        //use 29.97 as default
        mpFormat->streams[video_stream_index]->r_frame_rate.num = 90000;
        mpFormat->streams[video_stream_index]->r_frame_rate.den = 3003;
    }

    //need modify pts
    if (mpFormat->streams[video_stream_index]->time_base.num != 1 || mpFormat->streams[video_stream_index]->time_base.den != IParameters::TimeUnitDen_90khz) {
        mpInput[video_stream_index]->mNeedModifyPTS = 1;
    }

}

void CFFMpegMuxer::checkParameters()
{
    AM_ASSERT(mpFormat);
    if (NULL == mpFormat) {
        return;
    }

    //get durations
    AM_UINT iindex = 0;
    for (iindex=0; iindex < mTotalInputPinNumber; iindex ++) {
        AM_ASSERT(mpInput[iindex]);
        if (mpInput[iindex] && mpFormat->streams[iindex]) {
            mpInput[iindex]->mAVNormalizedDuration = av_rescale_q(mpInput[iindex]->mDuration, (AVRational){1,IParameters::TimeUnitDen_90khz}, mpFormat->streams[iindex]->time_base);
            AMLOG_INFO("[mAVNormalizedDuration]: (%d) is %d, mDuration is %d.\n", iindex, mpInput[iindex]->mAVNormalizedDuration, mpInput[iindex]->mDuration);
            AMLOG_INFO("   num %d, den %d.\n", mpFormat->streams[iindex]->time_base.num, mpFormat->streams[iindex]->time_base.den);
        }
    }

    AMLOG_DEBUG("mSyncedStreamPTSDTSGap %u.\n", mSyncedStreamPTSDTSGap);
    //sync start PTS, not DTS
    for (iindex=0; iindex < mTotalInputPinNumber; iindex ++) {
        AM_ASSERT(mpInput[iindex]);
        if (mpInput[iindex]) {
            mpInput[iindex]->mPTSDTSGap = mSyncedStreamPTSDTSGap;
            AMLOG_DEBUG("mpInput[%d]->mPTSDTSGap %u.\n", iindex, mpInput[iindex]->mPTSDTSGap);
        }
    }

    for (iindex=0; iindex < mTotalInputPinNumber; iindex ++) {
        AM_ASSERT(mpInput[iindex]);
        if (mpInput[iindex] && IParameters::StreamType_Audio == mpInput[iindex]->mType) {
            if (mpFormat->streams[iindex]->time_base.num != 1 || mpFormat->streams[iindex]->time_base.den != IParameters::TimeUnitDen_90khz) {
                mpInput[iindex]->mNeedModifyPTS = 1;
            }
        }
    }
}

#ifdef AM_DEBUG
void CFFMpegMuxer::PrintState()
{
    AMLOG_WARN("FFMuxer: msState=%d.\n", msState);
    AM_UINT iindex = 0;
    for (iindex=0; iindex < mTotalInputPinNumber; iindex ++) {
        AM_ASSERT(mpInput[iindex]);
        if (mpInput[iindex]) {
            AM_ASSERT(mpInput[iindex]->mpBufferQ);
            AMLOG_WARN(" inputpin[%d], have free data cnt %d.\n", iindex, mpInput[iindex]->mpBufferQ->GetDataCnt());
        }
    }
}
#endif

bool CFFMpegMuxer::isCommingBufferAutoFileBoundary(CFFMpegMuxerInput* pInputPin, CBuffer* pBuffer)
{
    AM_ASSERT(mpMasterInput);
    AM_ASSERT(pInputPin);
    AM_ASSERT(pBuffer);

    if ((pInputPin == mpMasterInput) && (PredefinedPictureType_IDR == pBuffer->mFrameType)) {
        //master video input pin
        AM_ASSERT(pInputPin->mType == IParameters::StreamType_Video);
        AMLOG_DEBUG("[AUTO-CUT]master's IDR comes.\n");
        if (!mbMasterStarted) {
            return false;
        }
        if (mSavingCondition == IParameters::MuxerSavingCondition_FrameCount) {
            if ((pInputPin->mCurrentFrameCount+1) < mAutoSaveFrameCount) {
                return false;
            }
            return true;
        } else if (mSavingCondition == IParameters::MuxerSavingCondition_InputPTS) {
            //check input pts
            if (pBuffer->GetPTS() < mNextFileTimeThreshold) {
                return false;
            }
            //AMLOG_WARN(" Last  PTS = %llu, mAutoSavingTimeDuration = %llu\n", pBuffer->GetPTS(),mAutoSavingTimeDuration);
            return true;
        } else if (mSavingCondition == IParameters::MuxerSavingCondition_CalculatedPTS) {
            //check calculated pts
            AM_U64 comming_pts = pBuffer->GetPTS() - pInputPin->mSessionInputPTSStartPoint + pInputPin->mSessionPTSStartPoint;
            if (comming_pts < mNextFileTimeThreshold) {
                return false;
            }
            //AMLOG_WARN(" Last  PTS = %llu, mAutoSavingTimeDuration = %llu\n", pBuffer->GetPTS(),mAutoSavingTimeDuration);
            return true;
        } else {
            AM_ERROR("NEED implement.\n");
            return false;
        }
    } else if ((pInputPin != mpMasterInput) && (pInputPin->mType != IParameters::StreamType_Video) && (_pinNeedSync((IParameters::StreamType)pInputPin->mType))) {

        if (!mpConsistentConfig->cutfile_with_precise_pts) {
            //ignore non-master(non-video)'s pts
            return false;
        }

        //no non-key frame condition
        if ((pBuffer->GetPTS()) < mAudioNextFileTimeThreshold) {
            return false;
        }
        return true;
    } else if ((pInputPin != mpMasterInput) && (pInputPin->mType == IParameters::StreamType_Video)) {
        //other cases, like multi-video inputpin's case, need implement
        AM_ERROR("NEED implement.\n");
    }

    return false;
}

bool CFFMpegMuxer::isReachPresetConditions(CFFMpegMuxerInput* pinput)
{
    if ((IParameters::StreamType_Video == pinput->mType) && (mPresetCheckFlag & EPresetCheckFlag_videoframecount)) {
        if (pinput->mTotalFrameCount >= mPresetVideoMaxFrameCnt) {
            mPresetReachType = EPresetCheckFlag_videoframecount;
            return true;
        }
    } else if ((IParameters::StreamType_Audio == pinput->mType) && (mPresetCheckFlag & EPresetCheckFlag_audioframecount)) {
        if (pinput->mTotalFrameCount >= mPresetAudioMaxFrameCnt) {
            mPresetReachType = EPresetCheckFlag_audioframecount;
            return true;
        }
    } else if ((mPresetCheckFlag & EPresetCheckFlag_filesize) && (mCurrentTotalFilesize >= mPresetMaxFilesize)) {
        mPresetReachType = EPresetCheckFlag_filesize;
        return true;
    }
    return false;
}

bool CFFMpegMuxer::hasPinNeedReachBoundary(AM_UINT& i)
{
    for (i=0; i< mTotalInputPinNumber; i++) {
        if (mpInput[i]) {
            if (_pinNeedSync((IParameters::StreamType)mpInput[i]->mType) && (false == mpInput[i]->mbAutoBoundaryReached)) {
                return true;
            }
        }
    }
    i = 0;
    return false;
}

AM_ERR CFFMpegMuxer::SetPresetMaxFrameCount(AM_U64 max_frame_count, IParameters::StreamType type)
{
    AMLOG_INFO("CFFMpegMuxer::SetPresetMaxFrameCount %llu, current flag 0x%x.\n", max_frame_count, mPresetCheckFlag);

    if (IParameters::StreamType_Video == type) {
        mPresetCheckFlag |= EPresetCheckFlag_videoframecount;
        mPresetVideoMaxFrameCnt = max_frame_count;
    } else if (IParameters::StreamType_Audio == type) {
        mPresetCheckFlag |= EPresetCheckFlag_audioframecount;
        mPresetAudioMaxFrameCnt = max_frame_count;
    } else {
        AM_ASSERT(0);
    }
    return ME_OK;
}

AM_U16 CFFMpegMuxer::GetRTPLastSeqNumber(IParameters::StreamType type)
{
    if (IParameters::StreamType_Video == type) {
        return mLastVideoRTPSeqNum;
    } else if (IParameters::StreamType_Audio == type) {
        return mLastAudioRTPSeqNum;
    }

    AM_ERROR("BAD Parameters %d.\n", type);
    return 0;
}

void CFFMpegMuxer::GetRTPLastTime(AM_U64& last_time, IParameters::StreamType type)
{
    if (IParameters::StreamType_Video == type) {
        last_time = mLastVideoStreammingTime;
        return;
    } else if (IParameters::StreamType_Audio == type) {
        last_time = mLastAudioStreammingTime;
        return;
    }

    AM_ERROR("BAD Parameters %d.\n", type);
}

AM_ERR CFFMpegMuxer::SetPresetMaxFilesize(AM_U64 max_file_size)
{
    AMLOG_INFO("CFFMpegMuxer::SetPresetMaxFilesize %llu, current flag 0x%x.\n", max_file_size, mPresetCheckFlag);
    mPresetCheckFlag |= EPresetCheckFlag_filesize;
    mPresetMaxFilesize = max_file_size;
    return ME_OK;
}

void CFFMpegMuxer::OnRun()
{
    CmdAck(ME_OK);
    AM_ERR err;

    CMD cmd;
    CQueue::QType type;
    CQueue::WaitResult result;
    mbRun = true;
//    bool start = false;
    CFFMpegMuxerInput* pInputPin = NULL;
    CBuffer::Type buffer_type;

    //saving file's variables
    AM_UINT inpin_index = 0;
    CBuffer* tmp_buffer = NULL;
    am_pts_t new_time_threshold = 0;
//    AM_U64 threshold_diff = 0;

    AMLOG_INFO("CFFMpegMuxer %p start OnRun loop.\n", this);
    if(mpConsistentConfig->disable_save_files == 0){
        AM_ASSERT(mpOutputFileNameBase);
        analyseFileNameFormat(mpOutputFileNameBase);
        updateFileName(mCurrentFileIndex);
    }
    err = Initialize();
    AM_ASSERT(err == ME_OK);
    if (err != ME_OK) {
        AMLOG_ERROR("CFFMpegMuxer::Initialize fail, enter error state.\n");
        msState = STATE_ERROR;
    } else {
        msState = STATE_IDLE;
    }

    if (IParameters::MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy && mpMasterInput) {
        AM_ASSERT(mpMasterInput->mParams.specific.video.framerate_num);
        //make sure duration is multi of master's frame interval(video)
        AM_UINT frame_interval = ((AM_U64)IParameters::TimeUnitDen_90khz) * mpMasterInput->mParams.specific.video.framerate_den/mpMasterInput->mParams.specific.video.framerate_num;
        mIDRFrameInterval = frame_interval*mpMasterInput->mParams.specific.video.M*mpMasterInput->mParams.specific.video.N*mpMasterInput->mParams.specific.video.IDRInterval;

        if (IParameters::MuxerSavingCondition_FrameCount == mSavingCondition) {
            //get frame duration from frame count
            mAutoSavingTimeDuration = mAutoSaveFrameCount * frame_interval;
            AMLOG_INFO("get duration from frame count(%u), frame_interval (%u), mAutoSavingTimeDuration is %llu.\n", mAutoSaveFrameCount, frame_interval, mAutoSavingTimeDuration);
        } else {
            AMLOG_INFO("before modification mAutoSavingTimeDuration is %llu, duration is %u, IDR Frame Interval is %u.\n", mAutoSavingTimeDuration, frame_interval, mIDRFrameInterval);
            mAutoSavingTimeDuration = ((mAutoSavingTimeDuration + mIDRFrameInterval/2) /mIDRFrameInterval)* mIDRFrameInterval;
            AMLOG_INFO("after modification mAutoSavingTimeDuration is %llu.\n", mAutoSavingTimeDuration);
        }
    }

    while (mbRun) {
        AMLOG_STATE("start switch, msState=%d.\n", msState);
#ifdef AM_DEBUG
        AM_UINT iindex = 0;
        for (iindex=0; iindex < mTotalInputPinNumber; iindex ++) {
            AM_ASSERT(mpInput[iindex]);
            if (mpInput[iindex]) {
                AM_ASSERT(mpInput[iindex]->mpBufferQ);
                AMLOG_STATE(" inputpin[%d], have free data cnt %d.\n", iindex, mpInput[iindex]->mpBufferQ->GetDataCnt());
            }
        }
#endif
        switch (msState) {

            case STATE_IDLE:
                if (mbPaused) {
                    msState = STATE_PENDING;
                    break;
                }

                type = mpWorkQ->WaitDataMsgCircularly(&cmd, sizeof(cmd),&result);
                if(type == CQueue::Q_MSG) {
                    ProcessCmd(cmd);
                } else {
                    pInputPin = (CFFMpegMuxerInput*)result.pOwner;
                    AM_ASSERT(pInputPin);
                    if (ReadInputData(pInputPin, buffer_type)) {
                        //AMLOG_INFO("mSavingFileStrategy %d, pInputPin %p, mpMasterInput %p, mpInputBuffer->GetPTS() %llu, mNextFileTimeThreshold %llu.\n", mSavingFileStrategy, pInputPin, mpMasterInput, mpInputBuffer->GetPTS(), mNextFileTimeThreshold);
                        if (buffer_type == CBuffer::DATA && mpInputBuffer) {

                            //update audio pts for case : close/open audio HAL on the fly
                            {
                                if (false == mbAudioHALClosed && pInputPin->mType == StreamType_Audio) {
                                    //TODO
                                    bool firstPacketInCurrentSessionAfterReopenHAL = false;
                                    if (true == mSyncAudioPTS.needmodifyPTS) {
                                        AM_U64 videoPTS = mSyncAudioPTS.lastAudioPTS + mVideoFirstPTS;
                                        mSyncAudioPTS.audioPTSoffset += (mSyncAudioPTS.lastVideoPTS - videoPTS);
                                        AMLOG_WARN("[MODIFY PTS]: audioPTSoffset %llu, mpInputBuffer->GetPTS() %llu, mSyncAudioPTS.lastVideoPTS %llu\n", mSyncAudioPTS.audioPTSoffset, mpInputBuffer->GetPTS(), mSyncAudioPTS.lastVideoPTS);
                                        AMLOG_WARN("[MODIFY PTS]: mVideoFirstPTS %llu, mAudioLastPTS %llu, mSyncAudioPTS.lastAudioPTS %llu\n", mVideoFirstPTS, mAudioLastPTS, mSyncAudioPTS.lastAudioPTS);
                                        firstPacketInCurrentSessionAfterReopenHAL = (pInputPin->mCurrentFrameCount == 0) ? true : false;
                                        mSyncAudioPTS.needmodifyPTS = false;
                                    }
                                    if (mSyncAudioPTS.audioPTSoffset > 0) {
                                        AM_U64 newPTS = mpInputBuffer->GetPTS() + mSyncAudioPTS.audioPTSoffset;
                                        mpInputBuffer->SetPTS(newPTS);
                                        if (true == firstPacketInCurrentSessionAfterReopenHAL) {
                                            pInputPin->mCurrentPTS = newPTS;
                                        }
                                    }
                                }
                            }

                            //detect auto saving condition
                            if (IParameters::MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy) {
                                if (true == isCommingBufferAutoFileBoundary(pInputPin, mpInputBuffer)) {
                                    if (!mpConsistentConfig->cutfile_with_precise_pts) {
                                        AM_ASSERT(NULL == pInputPin->mpCachedBuffer);
                                        AM_ASSERT(mpMasterInput == pInputPin);
                                        if (mpMasterInput == pInputPin) {
                                            AM_ASSERT(PredefinedPictureType_IDR == mpInputBuffer->mFrameType);
                                            new_time_threshold = mpInputBuffer->GetPTS();
                                            AMLOG_INFO("[Muxer], detected AUTO saving file, new video IDR threshold %llu, mNextFileTimeThreshold %llu, diff (%lld).\n", new_time_threshold, mNextFileTimeThreshold, new_time_threshold - mNextFileTimeThreshold);
                                            pInputPin->mpCachedBuffer = mpInputBuffer;
                                            mpInputBuffer = NULL;
                                            pInputPin->mbAutoBoundaryReached = true;
                                            msState = STATE_SAVING_PARTIAL_FILE;
                                        } else {
                                            AM_ERROR("[Muxer], detected AUTO saving file, wrong mpConsistentConfig->cutfile_with_precise_pts setting %d.\n", mpConsistentConfig->cutfile_with_precise_pts);
                                            if (IParameters::StreamType_Audio == pInputPin->mType) {
                                                writeAudioBuffer(pInputPin, mpInputBuffer);
                                            } else if (IParameters::StreamType_Video == pInputPin->mType) {
                                                writeVideoBuffer(pInputPin, mpInputBuffer);
                                            } else if (IParameters::StreamType_PrivateData == pInputPin->mType) {
                                                writePridataBuffer(pInputPin, mpInputBuffer);
                                            } else {
                                                AM_ERROR("BAD pin type %d.\n", pInputPin->mType);
                                            }
                                            mpInputBuffer->Release();
                                            mpInputBuffer = NULL;
                                            msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_MASTER_PIN;
                                            pInputPin->mbAutoBoundaryReached = true;
                                        }
                                    } else {
                                        AM_ASSERT(NULL == pInputPin->mpCachedBuffer);
                                        if (mpMasterInput == pInputPin) {
                                            AM_ASSERT(PredefinedPictureType_IDR == mpInputBuffer->mFrameType);
                                            new_time_threshold = mpInputBuffer->GetPTS();
                                            AMLOG_INFO("[Muxer], detected AUTO saving file, new video IDR threshold %llu, mNextFileTimeThreshold %llu, diff (%lld).\n", new_time_threshold, mNextFileTimeThreshold, new_time_threshold - mNextFileTimeThreshold);
                                            pInputPin->mpCachedBuffer = mpInputBuffer;
                                            mpInputBuffer = NULL;
                                            pInputPin->mbAutoBoundaryReached = true;
                                            msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                                        } else {
                                            new_time_threshold = mNextFileTimeThreshold;
                                            AMLOG_INFO("[Muxer], detected AUTO saving file, non-master pin detect boundary, pts %llu, mAudioNextFileTimeThreshold %llu, diff %lld, write buffer here.\n", mpInputBuffer->GetPTS(), mAudioNextFileTimeThreshold, mpInputBuffer->GetPTS() - mAudioNextFileTimeThreshold);
                                            if (IParameters::StreamType_Audio == pInputPin->mType) {
                                                writeAudioBuffer(pInputPin, mpInputBuffer);
                                            } else if (IParameters::StreamType_Video == pInputPin->mType) {
                                                writeVideoBuffer(pInputPin, mpInputBuffer);
                                            } else if (IParameters::StreamType_PrivateData == pInputPin->mType) {
                                                writePridataBuffer(pInputPin, mpInputBuffer);
                                            } else {
                                                AM_ERROR("BAD pin type %d.\n", pInputPin->mType);
                                            }
                                            mpInputBuffer->Release();
                                            mpInputBuffer = NULL;
                                            msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_MASTER_PIN;
                                            pInputPin->mbAutoBoundaryReached = true;
                                        }
                                    }
                                    break;
                                }
                            } else if (mPresetCheckFlag) {
                                //detect preset condition is reached
                                if (true == isReachPresetConditions(pInputPin)) {
                                    msState = STATE_SAVING_PRESET_TOTAL_FILE;
                                    AMLOG_INFO("[Muxer], detected Reach Preset condition, saving file, mCurrentTotalFrameCnt %llu, mCurrentTotalFilesize %llu, mPresetMaxFilesize %llu, mPresetCheckFlag 0x%x.\n", pInputPin->mTotalFrameCount, mCurrentTotalFilesize, mPresetMaxFilesize, mPresetCheckFlag);
                                    break;
                                }
                            }

                            msState = STATE_READY;

                        } else if (buffer_type == CBuffer::FLOW_CONTROL) {
                            AM_ASSERT(mpInputBuffer);
                            if (mpInputBuffer) {
                                if (mpInputBuffer->GetFlags() == IFilter::FlowControl_finalize_file) {
                                    AM_ASSERT(mpMasterInput);
                                    AM_ASSERT(pInputPin->mType == StreamType_Video);
                                    AM_ASSERT(NULL == pInputPin->mpCachedBuffer);
                                    //mpMasterInput = pInputPin;
                                    pInputPin->mbAutoBoundaryReached = true;
                                    msState = STATE_SAVING_PARTIAL_FILE;
                                    mNextFileTimeThreshold = mpInputBuffer->GetPTS();
                                    new_time_threshold = mNextFileTimeThreshold;
                                    //
                                    mAudioNextFileTimeThreshold = mNextFileTimeThreshold;
                                    AMLOG_INFO("[Muxer] manually saving file buffer comes, PTS %llu.\n", new_time_threshold);
                                    mpInputBuffer->Release();
                                    mpInputBuffer = NULL;
                                    break;
                                } else if (mpInputBuffer->GetFlags() == IFilter::FlowControl_eos) {
                                    AMLOG_INFO(" [Muxer] FlowControl EOS buffer comes.\n");
                                    mpInputBuffer->Release();
                                    mpInputBuffer = NULL;
                                    if (allInputEos()) {
                                        msState = STATE_PENDING;
                                        Finalize();
                                        SendEOS();
                                        break;
                                    }
                                } else {
                                    mpInputBuffer->Release();
                                    mpInputBuffer = NULL;
                                }
                            }
                        } else if (buffer_type == CBuffer::EOS) {
                            //check eos
                            AMLOG_INFO(" [Muxer] EOS buffer comes.\n");
                            AM_ASSERT(mpInputBuffer);
                            if (mpInputBuffer) {
                                mpInputBuffer->Release();
                                mpInputBuffer = NULL;
                            }
                            if (allInputEos()) {
                                Finalize();
                                SendEOS();
                                msState = STATE_PENDING;
                                break;
                            }
                        } else {
                            if (buffer_type != CBuffer::DATA) {
                                AM_ERROR("BAD buffer type, %d.\n", buffer_type);
                            }
                            AM_ASSERT(0);
                            if (mpInputBuffer) {
                                mpInputBuffer->Release();
                                mpInputBuffer = NULL;
                            }
                        }

                    } else if (pInputPin->mbSkiped != true) {
                        //peek buffer fail. must not comes here
                        AM_ERROR("peek buffer fail. must not comes here.\n");
                        msState = STATE_ERROR;
                    }
                }

                break;

            case STATE_READY:
                AM_ASSERT(pInputPin);
                AM_ASSERT(mpInputBuffer);
                AMLOG_DEBUG("ready to write data, type %d.\n", pInputPin->mType);
                if (pInputPin->mType == StreamType_Video) {
                    if (pInputPin->mpCachedBuffer) {
                        writeVideoBuffer(pInputPin, pInputPin->mpCachedBuffer);
                        pInputPin->mpCachedBuffer->Release();
                        pInputPin->mpCachedBuffer = NULL;
                        if (msState != STATE_READY) {
                            if (mpInputBuffer) {
                                mpInputBuffer->Release();
                                mpInputBuffer = NULL;
                            }
                            //AMLOG_WARN("something error happened, not goto idle state.\n");
                            break;
                        }
                    }
                    writeVideoBuffer(pInputPin, mpInputBuffer);
                    if (msState != STATE_READY) {
                        mpInputBuffer->Release();
                        mpInputBuffer = NULL;
                        //AMLOG_WARN("something error happened, not goto idle state.\n");
                        break;
                    }
                } else if (pInputPin->mType == StreamType_Audio) {
                    if (pInputPin->mpCachedBuffer) {
                        writeAudioBuffer(pInputPin, pInputPin->mpCachedBuffer);
                        pInputPin->mpCachedBuffer->Release();
                        pInputPin->mpCachedBuffer = NULL;
                    }
                    writeAudioBuffer(pInputPin, mpInputBuffer);
                } else if (pInputPin->mType == StreamType_Subtitle) {

                } else if (pInputPin->mType == StreamType_PrivateData) {
                    writePridataBuffer(pInputPin, mpInputBuffer);
                } else {
                    AM_ASSERT(0);
                    AMLOG_ERROR("must not comes here, BAD input type(%d)? pin %p, buffer %p.\n", pInputPin->mType, pInputPin, mpInputBuffer);
                }
                pInputPin = NULL;

                mpInputBuffer->Release();
                mpInputBuffer = NULL;
                if (msState != STATE_READY) {
                    AMLOG_WARN("something error happened, not goto idle state.\n");
                    break;
                }
                msState = STATE_IDLE;
                break;

            case STATE_FLUSH_EXPIRED_FRAME:
                {
                    AM_U64 cur_time = 0;
//                    AM_UINT find_idr = 0;
                    AM_ASSERT(pInputPin);
                    AM_ASSERT(!mpInputBuffer);
                    if (mpInputBuffer) {
                        mpInputBuffer->Release();
                        mpInputBuffer = NULL;
                    }
                    if (pInputPin->mpCachedBuffer) {
                        pInputPin->mpCachedBuffer->Release();
                        pInputPin->mpCachedBuffer = NULL;
                    }
                    //only video have this case
                    AM_ASSERT(IParameters::StreamType_Video == pInputPin->mType);
                    if (mpClockManager) {
                        cur_time = mpClockManager->GetCurrentTime();
                    }

                    //discard data to next IDR
                    while (true == ReadInputData(pInputPin, buffer_type)) {
                        if (CBuffer::DATA == mpInputBuffer->GetType()) {
                            AMLOG_WARN("Discard frame, expired time %llu, cur time %llu, frame type %d, Seq num[%d, %d].\n", mpInputBuffer->mExpireTime, cur_time, mpInputBuffer->mFrameType, mpInputBuffer->mOriSeqNum, mpInputBuffer->mSeqNum);
                            if (PredefinedPictureType_IDR != mpInputBuffer->mFrameType) {
                                mpInputBuffer->Release();
                                mpInputBuffer = NULL;
                            } else {
                                if (mpInputBuffer->mExpireTime > cur_time) {
                                    msState = STATE_READY;//continue write data
                                    AMLOG_INFO("Get valid IDR... cur time %llu, start time %llu, expird time %llu.\n", mpClockManager->GetCurrentTime(), cur_time, mpInputBuffer->mExpireTime);
                                    break;
                                } else {
                                    mpInputBuffer->Release();
                                    mpInputBuffer = NULL;
                                }
                            }
                        } else if (buffer_type == CBuffer::FLOW_CONTROL) {
                            AM_ASSERT(mpInputBuffer);
                            if (mpInputBuffer) {
                                if (mpInputBuffer->GetFlags() == IFilter::FlowControl_finalize_file) {
                                    AM_ASSERT(mpMasterInput);
                                    AM_ASSERT(pInputPin->mType == StreamType_Video);
                                    AM_ASSERT(NULL == pInputPin->mpCachedBuffer);
                                    //mpMasterInput = pInputPin;
                                    pInputPin->mbAutoBoundaryReached = true;
                                    msState = STATE_SAVING_PARTIAL_FILE;
                                    mNextFileTimeThreshold = mpInputBuffer->GetPTS();
                                    new_time_threshold = mNextFileTimeThreshold;
                                    AMLOG_INFO("[Muxer] manually saving file buffer comes, PTS %llu.\n", new_time_threshold);
                                    mpInputBuffer->Release();
                                    mpInputBuffer = NULL;
                                    break;
                                } else if (mpInputBuffer->GetFlags() == IFilter::FlowControl_eos) {
                                    AMLOG_INFO(" [Muxer] FlowControl EOS buffer comes.\n");
                                    mpInputBuffer->Release();
                                    mpInputBuffer = NULL;
                                    if (allInputEos()) {
                                        msState = STATE_PENDING;
                                        Finalize();
                                        SendEOS();
                                        break;
                                    }
                                } else {
                                    mpInputBuffer->Release();
                                    mpInputBuffer = NULL;
                                }
                            }
                            break;
                        }else if (buffer_type == CBuffer::EOS) {
                            //check eos
                            AMLOG_INFO(" [Muxer] EOS buffer comes.\n");
                            AM_ASSERT(mpInputBuffer);
                            if (mpInputBuffer) {
                                mpInputBuffer->Release();
                                mpInputBuffer = NULL;
                            }
                            if (allInputEos()) {
                                Finalize();
                                SendEOS();
                                msState = STATE_PENDING;
                            }
                            break;
                        } else {
                            if (buffer_type != CBuffer::DATA) {
                                AM_ERROR("BAD buffer type, %d.\n", buffer_type);
                            }
                            AM_ASSERT(0);
                            if (mpInputBuffer) {
                                mpInputBuffer->Release();
                                mpInputBuffer = NULL;
                            }
                            break;
                        }
                    }

                    //state not changed
                    if (STATE_FLUSH_EXPIRED_FRAME == msState) {
                        AMLOG_WARN("Flush all data in input pin, no valid IDR... cur time %llu, start time %llu.\n", mpClockManager->GetCurrentTime(), cur_time);
                        msState = STATE_IDLE;//clear all video buffers now
                    }
                }
                break;

            //if direct from STATE_IDLE
            //use robust but not very strict PTS strategy, target: cost least time, not BLOCK some real time task, like streamming out, capture data from driver, etc
            //report PTS gap, if gap too large, post warnning msg
            //need stop recording till IDR comes, audio's need try its best to match video's PTS

            //if from previous STATE_SAVING_PARTIAL_FILE_XXXX, av sync related pin should with more precise PTS boundary, but would be with more latency
            case STATE_SAVING_PARTIAL_FILE:
                AM_ASSERT(new_time_threshold >= mNextFileTimeThreshold);

                //write remainning packet
                AMLOG_INFO("[muxer %p] start saving patial file, peek all packet if PTS less than threshold.\n", this);
                for (inpin_index = 0; inpin_index < mTotalInputPinNumber; inpin_index++) {
                    if (mpMasterInput == mpInput[inpin_index] || mpInput[inpin_index]->mbAutoBoundaryReached) {
                        continue;
                    }

                    tmp_buffer = NULL;
                    while (true == mpInput[inpin_index]->PeekBuffer(tmp_buffer)) {
                        AM_ASSERT(tmp_buffer);
                        AMLOG_INFO("[muxer %p, inpin %d] peek packet, PTS %llu, PTS threshold %llu.\n", this, inpin_index, tmp_buffer->GetPTS(), new_time_threshold);
                        if (mpInput[inpin_index]->mType == StreamType_Video) {
                            writeVideoBuffer(mpInput[inpin_index], tmp_buffer);
                        } else if (mpInput[inpin_index]->mType == StreamType_Audio) {
                            writeAudioBuffer(mpInput[inpin_index], tmp_buffer);
                        } else if (mpInput[inpin_index]->mType == StreamType_PrivateData) {
                            writePridataBuffer(mpInput[inpin_index], tmp_buffer);
                        } else {
                            AM_ASSERT(0);
                        }

                        if (tmp_buffer->GetPTS() > new_time_threshold) {
                            tmp_buffer->Release();
                            tmp_buffer = NULL;
                            break;
                        }
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                    }

                }

                AMLOG_INFO("[muxer %p] finalize current file, %s.\n", this, mpOutputFileName);

                err = Finalize();

                if(err != ME_OK){
                    AMLOG_ERROR("CFFMpegMuxer::Finalize fail, enter error state.\n");
                    msState = STATE_ERROR;
                    break;
                }
                if (mMaxTotalFileNumber) {
                    if (mTotalFileNumber >= mMaxTotalFileNumber) {
                        deletePartialFile(mFirstFileIndexCanbeDeleted);
                        mFirstFileIndexCanbeDeleted ++;
                        mTotalFileNumber --;
                    }
                } else {
                    //do nothing
                }

                //corner case, little chance to comes here
                if (allInputEos()) {
                    AMLOG_WARN(" allInputEos() here, should not comes here, please check!!!!.\n");
                    //clear all cached buffer
                    for (inpin_index = 0; inpin_index < mTotalInputPinNumber; inpin_index++) {
                        if (mpInput[inpin_index]) {
                            if (mpInput[inpin_index]->mpCachedBuffer) {
                                mpInput[inpin_index]->mpCachedBuffer->Release();
                                mpInput[inpin_index]->mpCachedBuffer = NULL;
                            }
                        }
                    }

                    AMLOG_INFO("EOS buffer comes in state %d, enter pending.\n", msState);
                    SendEOS();
                    msState = STATE_PENDING;
                    break;
                }

                //skychen, 2012_7_17
                if(mTotalRecFileNumber > 0 && mTotalFileNumber >= mTotalRecFileNumber){
                    AMLOG_INFO("the num of rec file(%d) has reached the limit(%d), so stop recording\n", mTotalFileNumber, mTotalRecFileNumber);
                    AM_MSG msg;
                    msg.code = IEngine::MSG_FILE_NUM_REACHED_LIMIT;
                    msg.p1  = (AM_INTPTR)static_cast<IFilter*>(this);
                    msg.p3 = (AM_INTPTR)mTotalRecFileNumber;
                    PostEngineMsg(msg);
                    msState = STATE_PENDING;
                    break;
                }

                mNextFileTimeThreshold += mAutoSavingTimeDuration;
                if (mNextFileTimeThreshold < new_time_threshold) {
                    AMLOG_WARN("mNextFileTimeThreshold(%llu) + mAutoSavingTimeDuration(%llu) still less than new_time_threshold(%llu).\n", mNextFileTimeThreshold, mAutoSavingTimeDuration, new_time_threshold);
                    mNextFileTimeThreshold += mAutoSavingTimeDuration;
                }
                mAudioNextFileTimeThreshold = mNextFileTimeThreshold - mVideoFirstPTS;
                ++mCurrentFileIndex;
                updateFileName(mCurrentFileIndex);
                AMLOG_INFO("[muxer %p] initialize new file %s.\n", this, mpOutputFileName);

                //generate thumbnail file here
                if (mpSharedRes->encoding_mode_config.thumbnail_enabled && mpThumbNailFileName) {
                    AM_MSG msg;
                    msg.code = IEngine::MSG_GENERATE_THUMBNAIL;
                    msg.p1  = (AM_INTPTR)static_cast<IFilter*>(this);
                    msg.p3 = (AM_INTPTR)mpThumbNailFileName;
                    PostEngineMsg(msg);
                }

                err = Initialize();
                AM_ASSERT(err == ME_OK);
                if (err != ME_OK) {
                    AMLOG_ERROR("CFFMpegMuxer::Initialize fail, enter error state.\n");
                    msState = STATE_ERROR;
                    break;
                }
                AMLOG_INFO("[muxer %p] end saving patial file.\n", this);
                msState = STATE_IDLE;
                break;

            //with more precise pts, but with much latency for write new file/streaming
            case STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_MASTER_PIN:
                //wait master pin's gap(video IDR)
                AM_ASSERT(new_time_threshold >= mNextFileTimeThreshold);
                AM_ASSERT(pInputPin != mpMasterInput);
                AM_ASSERT(false == mpMasterInput->mbAutoBoundaryReached);

                AMLOG_INFO("[avsync]: (%p) PEEK buffers in master pin start.\n", this);

                if (pInputPin != mpMasterInput) {
                    //get master's time threshold
                    //peek master input(video) first, refresh the threshold, the duration maybe not very correct because h264's IDR time interval maybe large (related to M/N/IDR interval settings)
                    AM_ASSERT(IParameters::StreamType_Video != pInputPin->mType);
                    AM_ASSERT(IParameters::StreamType_Video == mpMasterInput->mType);
                    while (true == mpMasterInput->PeekBuffer(tmp_buffer)) {
                        AM_ASSERT(tmp_buffer);
                        if (CBuffer::DATA == tmp_buffer->GetType()) {
                            AMLOG_INFO("[master inpin] peek packet, PTS %llu, PTS threshold %llu.\n", tmp_buffer->GetPTS(), mNextFileTimeThreshold);
                            if ((tmp_buffer->GetPTS() >  mNextFileTimeThreshold) && (PredefinedPictureType_IDR == tmp_buffer->mFrameType)) {
                                AM_ASSERT(!mpMasterInput->mpCachedBuffer);
                                if (mpMasterInput->mpCachedBuffer) {
                                    AMLOG_WARN("why comes here, release cached buffer, pts %llu, type %d.\n", mpMasterInput->mpCachedBuffer->GetPTS(), mpMasterInput->mpCachedBuffer->mFrameType);
                                    mpMasterInput->mpCachedBuffer->Release();
                                }
                                mpMasterInput->mpCachedBuffer = tmp_buffer;
                                if (!_isTmeStampNear(new_time_threshold, tmp_buffer->GetPTS(), IParameters::TimeUnitNum_fps29dot97)) {
                                    //refresh pInputPin's threshold
                                    if((IParameters::StreamType)pInputPin->mType != StreamType_Audio){
                                        pInputPin->mbAutoBoundaryReached = false;
                                    }
                                }
                                new_time_threshold = tmp_buffer->GetPTS();
                                AMLOG_INFO("get IDR boundary, pts %llu, mNextFileTimeThreshold %llu, diff %lld.\n", new_time_threshold, mNextFileTimeThreshold, new_time_threshold - mNextFileTimeThreshold);
                                mpMasterInput->mbAutoBoundaryReached = true;
                                tmp_buffer = NULL;
                                break;
                            }
                            writeVideoBuffer(mpMasterInput, tmp_buffer);
                        } else {
                            if (CBuffer::EOS == tmp_buffer->GetType() || (CBuffer::FLOW_CONTROL== tmp_buffer->GetType() && IFilter::FlowControl_eos == tmp_buffer->GetFlags())) {
                                mpMasterInput->mEOSComes = 1;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                break;
                            } else {
                                //ignore
                            }
                        }
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                    }

                    if (true == mpMasterInput->mbAutoBoundaryReached) {
                        AMLOG_INFO("[avsync]: PEEK buffers in master pin done, go to non-master pin.\n");
                        msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                    } else {
                        AMLOG_INFO("[avsync]: PEEK buffers in master pin not finished, go to wait master boundary(IDR).\n");
                        msState = STATE_SAVING_PARTIAL_FILE_WAIT_MASTER_PIN;
                    }

                } else {
                    AM_ERROR("MUST not comes here, please check logic.\n");
                    msState = STATE_SAVING_PARTIAL_FILE;
                }
                break;

            case STATE_SAVING_PARTIAL_FILE_WAIT_MASTER_PIN:
                AMLOG_INFO("[avsync]: (%p) WAIT next IDR boundary start.\n", this);
                AM_ASSERT(false == mpMasterInput->mbAutoBoundaryReached);
                AM_ASSERT(!mpMasterInput->mpCachedBuffer);
                if (mpMasterInput->mpCachedBuffer) {
                    AMLOG_WARN("Already have get next IDR, goto next stage.\n");
                    mpMasterInput->mbAutoBoundaryReached = true;
                    msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                    break;
                }

                if (false == mpMasterInput->mbAutoBoundaryReached) {
                    type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpMasterInput->mpBufferQ);
                    if(type == CQueue::Q_MSG) {
                        ProcessCmd(cmd);
                    } else {
                        if (true == mpMasterInput->PeekBuffer(tmp_buffer)) {
                            if (CBuffer::DATA == tmp_buffer->GetType()) {
                                if (PredefinedPictureType_IDR == tmp_buffer->mFrameType && tmp_buffer->GetPTS() >= new_time_threshold) {
                                    mpMasterInput->mpCachedBuffer = tmp_buffer;
                                    if (!_isTmeStampNear(new_time_threshold, tmp_buffer->GetPTS(), IParameters::TimeUnitNum_fps29dot97)) {
                                        //refresh new time threshold, need recalculate non-masterpin
                                        if((IParameters::StreamType)pInputPin->mType != StreamType_Audio){
                                            pInputPin->mbAutoBoundaryReached = false;
                                        }
                                    }
                                    AMLOG_INFO("[avsync]: wait new IDR, pts %llu, previous time threshold %llu, diff %lld.\n", tmp_buffer->GetPTS(), new_time_threshold, tmp_buffer->GetPTS() - new_time_threshold);
                                    new_time_threshold = tmp_buffer->GetPTS();
                                    mpMasterInput->mbAutoBoundaryReached = true;
                                    msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;

                                    tmp_buffer = NULL;
                                    break;
                                }
                                writeVideoBuffer(mpMasterInput, tmp_buffer);
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                            } else if ((CBuffer::EOS == tmp_buffer->GetType()) || ((CBuffer::FLOW_CONTROL == tmp_buffer->GetType()) && (IFilter::FlowControl_eos == tmp_buffer->GetFlags()))) {
                                AMLOG_WARN("EOS comes here, how to handle it, msState %d.\n", msState);
                                mpMasterInput->mEOSComes = 1;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                mpMasterInput->mbAutoBoundaryReached = true;
                                msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                            } else {
                                //ignore
                                AM_ERROR("not processed buffer type %d.\n", tmp_buffer->GetType());
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                            }
                        } else {
                            AM_ERROR("MUST not comes here, please check WaitDataMsgWithSpecifiedQueue()\n");
                            msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                            mpMasterInput->mbAutoBoundaryReached = true;
                        }
                    }
                }
                break;

            case STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN:
                //for non-master pin
                AM_ASSERT(new_time_threshold >= mNextFileTimeThreshold);

                AMLOG_INFO("[avsync]: PEEK non-master pin(%p) start.\n", this);
                if (!hasPinNeedReachBoundary(inpin_index)) {
                    msState = STATE_SAVING_PARTIAL_FILE;
                    AMLOG_INFO("[avsync]: all pin(need sync) reaches boundary.\n");
                    break;
                }
                if (inpin_index >= mTotalInputPinNumber || !mpInput[inpin_index]) {
                    AM_ERROR("safe check fail, check code, inpin_index %d, mTotalInputPinNumber %d.\n", inpin_index, mTotalInputPinNumber);
                    msState = STATE_SAVING_PARTIAL_FILE;
                    break;
                }

                pInputPin = mpInput[inpin_index];
                AM_ASSERT(pInputPin);
                AM_ASSERT(pInputPin != mpMasterInput);
                AM_ASSERT(IParameters::StreamType_Video!= pInputPin->mType);
                AM_ASSERT(!pInputPin->mpCachedBuffer);
                if (pInputPin->mpCachedBuffer) {
                    AM_ERROR("why comes here, check code.\n");
                    pInputPin->mpCachedBuffer->Release();
                    pInputPin->mpCachedBuffer = NULL;
                }

                if (pInputPin != mpMasterInput) {
                    //peek non-master input
                    AM_ASSERT(IParameters::StreamType_Video != pInputPin->mType);
                    while (true == pInputPin->PeekBuffer(tmp_buffer)) {
                        AM_ASSERT(tmp_buffer);
                        if (CBuffer::DATA == tmp_buffer->GetType()) {
                            AMLOG_INFO("[non-master inpin] (%p) peek packet, PTS %llu, PTS threshold %llu.\n", this, tmp_buffer->GetPTS(), mNextFileTimeThreshold);
                            if (IParameters::StreamType_Audio == pInputPin->mType) {
                                writeAudioBuffer(pInputPin, tmp_buffer);
                            } else if (IParameters::StreamType_Video == pInputPin->mType) {
                                writeVideoBuffer(pInputPin, tmp_buffer);
                            } else if (IParameters::StreamType_PrivateData == pInputPin->mType) {
                                writePridataBuffer(pInputPin, tmp_buffer);
                            } else {
                                AM_ERROR("BAD pin type %d.\n", pInputPin->mType);
                            }

                            if(IParameters::StreamType_Audio != pInputPin->mType){
                                if (tmp_buffer->GetPTS() >=  mNextFileTimeThreshold) {
                                    AMLOG_INFO("[avsync]: non-master pin (%p) get boundary, pts %llu, mNextFileTimeThreshold %llu, diff %lld.\n", this, new_time_threshold, mNextFileTimeThreshold, new_time_threshold - mNextFileTimeThreshold);
                                    pInputPin->mbAutoBoundaryReached = true;
                                    tmp_buffer->Release();
                                    tmp_buffer = NULL;
                                    break;
                                }
                            }else {//audio stream
                                if(tmp_buffer->GetPTS() >=  mAudioNextFileTimeThreshold){
                                    AMLOG_INFO("[avsync]: audio input pin (%p) get boundary, pts %llu, mAudioNextFileTimeThreshold %llu, diff %lld.\n", this, new_time_threshold, mAudioNextFileTimeThreshold, new_time_threshold - mAudioNextFileTimeThreshold);
                                    pInputPin->mbAutoBoundaryReached = true;
                                    tmp_buffer->Release();
                                    tmp_buffer = NULL;
                                    break;
                                }
                            }
                            tmp_buffer->Release();
                            tmp_buffer = NULL;
                        } else {
                            if (CBuffer::EOS == tmp_buffer->GetType() || (CBuffer::FLOW_CONTROL== tmp_buffer->GetType() && IFilter::FlowControl_eos == tmp_buffer->GetFlags())) {
                                pInputPin->mEOSComes = 1;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                break;
                            } else if (CBuffer::FLOW_CONTROL== tmp_buffer->GetType() && IFilter::FlowControl_close_audioHAL == tmp_buffer->GetFlags()) {
                                mbAudioHALClosed = true;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                break;
                            } else {
                                //ignore
                            }
                            tmp_buffer->Release();
                            tmp_buffer = NULL;
                        }
                    }

                    if (false == pInputPin->mbAutoBoundaryReached) {
                        AMLOG_INFO("[avsync]: non-master pin need wait.\n");
                        msState = STATE_SAVING_PARTIAL_FILE_WAIT_NON_MASTER_PIN;
                    }

                    if (mbAudioHALClosed == true) {
                        AMLOG_INFO("[wait audio]: audio recorder has been closed, switch to STATE_SAVING_PARTIAL_FILE!\n");
                        pInputPin->mbAutoBoundaryReached = true;
                        msState = STATE_SAVING_PARTIAL_FILE;
                    }

                } else {
                    AM_ERROR("MUST not comes here, please check logic.\n");
                    msState = STATE_SAVING_PARTIAL_FILE;
                }
                break;

            case STATE_SAVING_PARTIAL_FILE_WAIT_NON_MASTER_PIN:
                AMLOG_INFO("[avsync]: (%p)WAIT till boundary(non-master pin).\n", this);

                //debug assert
                AM_ASSERT(pInputPin);
                AM_ASSERT(pInputPin != mpMasterInput);
                AM_ASSERT(IParameters::StreamType_Video!= pInputPin->mType);
                AM_ASSERT(!pInputPin->mpCachedBuffer);

                AM_ASSERT(false == pInputPin->mbAutoBoundaryReached);
                AM_ASSERT(!pInputPin->mpCachedBuffer);
                if (pInputPin->mpCachedBuffer) {
                    AMLOG_WARN("[avsync]: Already have get cached buffer, goto next stage.\n");
                    pInputPin->mbAutoBoundaryReached = true;
                    msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                    break;
                }

                if (false == pInputPin->mbAutoBoundaryReached) {
                    type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), pInputPin->mpBufferQ);
                    if(type == CQueue::Q_MSG) {
                        ProcessCmd(cmd);
                    } else {
                        if (true == pInputPin->PeekBuffer(tmp_buffer)) {
                            if (CBuffer::DATA == tmp_buffer->GetType()) {
                                if (IParameters::StreamType_Audio == pInputPin->mType) {
                                    writeAudioBuffer(pInputPin, tmp_buffer);
                                } else if (IParameters::StreamType_Video == pInputPin->mType) {
                                    writeVideoBuffer(pInputPin, tmp_buffer);
                                } else if (IParameters::StreamType_PrivateData == pInputPin->mType) {
                                    writePridataBuffer(pInputPin, tmp_buffer);
                                } else {
                                    AM_ERROR("BAD pin type %d.\n", pInputPin->mType);
                                }

                                if (tmp_buffer->GetPTS() >= new_time_threshold) {
                                    AMLOG_INFO("[avsync]: non-master pin(%p) wait new boundary, pts %llu, time threshold %llu, diff %lld.\n", this, tmp_buffer->GetPTS(), new_time_threshold, tmp_buffer->GetPTS() - new_time_threshold);
                                    pInputPin->mbAutoBoundaryReached = true;
                                    msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                                    tmp_buffer->Release();
                                    tmp_buffer = NULL;
                                    break;
                                }
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                            } else if ((CBuffer::EOS == tmp_buffer->GetType()) || ((CBuffer::FLOW_CONTROL == tmp_buffer->GetType()) && (IFilter::FlowControl_eos == tmp_buffer->GetFlags()))) {
                                AMLOG_WARN("EOS comes here, how to handle it, msState %d.\n", msState);
                                pInputPin->mEOSComes = 1;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                pInputPin->mbAutoBoundaryReached = true;
                                if (allInputEos()) {
                                    msState = STATE_PENDING;
                                    Finalize();
                                    SendEOS();
                                } else {
                                    msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                                }
                                break;
                            } else if (CBuffer::FLOW_CONTROL== tmp_buffer->GetType() && IFilter::FlowControl_close_audioHAL == tmp_buffer->GetFlags()) {
                                mbAudioHALClosed = true;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                                break;
                            } else {
                                //ignore
                                AM_ERROR("not processed buffer type %d.\n", tmp_buffer->GetType());
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                            }
                        } else {
                            AM_ERROR("MUST not comes here, please check WaitDataMsgWithSpecifiedQueue()\n");
                            msState = STATE_SAVING_PARTIAL_FILE_PEEK_REMAINING_BUFFER_NON_MASTER_PIN;
                            pInputPin->mbAutoBoundaryReached = true;
                        }
                    }
                }
                break;

            case STATE_SAVING_PRESET_TOTAL_FILE:
                //write data
                AM_ASSERT(mpInputBuffer);
                AM_ASSERT(pInputPin);
                AM_ASSERT(pInputPin->mType == StreamType_Video);

                AMLOG_INFO("[muxer %p] STATE_SAVING_PRESET_TOTAL_FILE, write remainning packet start.\n", this);
                if (pInputPin->mType == StreamType_Video) {
                    if (pInputPin->mpCachedBuffer) {
                        writeVideoBuffer(pInputPin, pInputPin->mpCachedBuffer);
                        pInputPin->mpCachedBuffer->Release();
                        pInputPin->mpCachedBuffer = NULL;
                    }
                    writeVideoBuffer(pInputPin, mpInputBuffer);
                } else if (pInputPin->mType == StreamType_Audio) {
                    if (pInputPin->mpCachedBuffer) {
                        AM_ASSERT(0);//current code should not come to this case
                        AMLOG_ERROR("current code should not come to this case.\n");
                        writeAudioBuffer(pInputPin, pInputPin->mpCachedBuffer);
                        pInputPin->mpCachedBuffer->Release();
                        pInputPin->mpCachedBuffer = NULL;
                    }
                    writeAudioBuffer(pInputPin, mpInputBuffer);
                } else if (pInputPin->mType == StreamType_Subtitle) {

                } else if (pInputPin->mType == StreamType_PrivateData) {
                    writePridataBuffer(pInputPin, mpInputBuffer);
                } else {
                    AM_ASSERT(0);
                    AMLOG_ERROR("must not comes here, BAD input type(%d)? pin %p, buffer %p.\n", pInputPin->mType, pInputPin, mpInputBuffer);
                }
                pInputPin = NULL;

                mpInputBuffer->Release();
                mpInputBuffer = NULL;
                msState = STATE_PENDING;

                //write remainning packet, todo

                //saving file
                AMLOG_INFO("[muxer %p] STATE_SAVING_PRESET_TOTAL_FILE, write remainning packet end, Finalize() start.\n", this);
                Finalize();
                AMLOG_INFO("[muxer %p] STATE_SAVING_PRESET_TOTAL_FILE, write remainning packet end, Finalize() done.\n", this);

                AMLOG_INFO("Post MSG_INTERNAL_ENDING to engine.\n");
                //PostEngineMsg(IEngine::MSG_INTERNAL_ENDING);
                AM_MSG msg;
                memset(&msg,0,sizeof(msg));
                msg.p0 = (AM_INTPTR)static_cast<IFilter*>(this);
                if (mPresetReachType | (EPresetCheckFlag_videoframecount|EPresetCheckFlag_audioframecount)){
                    msg.code = IEngine::MSG_DURATION;
                } else if (mPresetReachType == EPresetCheckFlag_filesize) {
                    msg.code = IEngine::MSG_FILESIZE;
                } else {
                    AM_ASSERT(0);
                };
                PostEngineMsg(msg);
                AMLOG_INFO("[muxer %p] STATE_SAVING_PRESET_TOTAL_FILE, MSG_INTERNAL_ENDING done.\n", this);

                break;

            case STATE_PENDING:
            case STATE_ERROR:
                //discard all data, wait eos
                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd),&result);
                if(type == CQueue::Q_MSG) {
                    ProcessCmd(cmd);
                } else {
                    pInputPin = (CFFMpegMuxerInput*)result.pOwner;
                    AM_ASSERT(pInputPin);
                    if (ReadInputData(pInputPin, buffer_type)) {
                        if (mpInputBuffer) {
                            AM_ASSERT(mpInputBuffer);
                            mpInputBuffer->Release();
                            mpInputBuffer = NULL;
                        }
                        if (buffer_type == CBuffer::EOS) {
                            //check eos
                            AMLOG_INFO("EOS buffer comes.\n");
                            if (allInputEos()) {
                                if (msState != STATE_ERROR) {
                                    Finalize();
                                }
                                SendEOS();
                            }
                        } else {
                            //AM_ERROR("BAD buffer type, %d.\n", buffer_type);
                        }
                    } else {
                        //peek buffer fail. must not comes here
                        AM_ERROR("peek buffer fail. must not comes here.\n");
                    }
                }
                break;

            default:
                AM_ERROR(" %d",(AM_UINT)msState);
                break;
        }
    }

    if(mpInputBuffer) {
        mpInputBuffer->Release();
        mpInputBuffer = NULL;
    }

    //for safe
    if (msState != STATE_ERROR) {
        Finalize();
    }

    AMLOG_INFO("CFFMpegMuxer %p OnRun exit, msState=%d.\n", this, msState);
}

AM_ERR CFFMpegMuxer::setupUDPSocket(IParameters::StreamType type)
{
    if (type == IParameters::StreamType_Video) {
        if (false == mbRTPSocketSetup) {
            mbRTPSocketSetup = true;
        } else {
            AMLOG_WARN("socket has been setup, please check code.\n");
            return ME_OK;
        }

        //AM_ASSERT(-1 == mRTPSocket);
        if (mRTPSocket >= 0) {
            AMLOG_WARN("close previous socket here, %d.\n", mRTPSocket);
            close(mRTPSocket);
            mRTPSocket = -1;
        }
        //AM_ASSERT(-1 == mRTCPSocket);
        if (mRTCPSocket >= 0) {
            AMLOG_WARN("close previous socket here, %d.\n", mRTCPSocket);
            close(mRTCPSocket);
            mRTCPSocket = -1;
        }

        AMLOG_INFO(" before SetupDatagramSocket, port %d.\n", mRTPPort);
        mRTPSocket = SetupDatagramSocket(INADDR_ANY, mRTPPort, 0);
        AMLOG_INFO(" mRTPSocket %d.\n", mRTPSocket);
        if (mRTPSocket < 0) {
            //need change port and retry?
            AM_ERROR("SetupDatagramSocket(RTP) fail, port %d, socket %d, need change port and retry, todo?\n", mRTPPort, mRTPSocket);
            return ME_ERROR;
        }
        /* limit the tx buf size to limit latency */
        int tmp = 32768;
        if (setsockopt(mRTPSocket, SOL_SOCKET, SO_SNDBUF, &tmp, sizeof(tmp)) < 0) {
            AM_ERROR("setsockopt(SO_SNDBUF): %s\n", strerror(errno));
            return ME_ERROR;
        }
        AMLOG_INFO(" before SetupDatagramSocket, port %d.\n", mRTCPPort);
        mRTCPSocket = SetupDatagramSocket(INADDR_ANY, mRTCPPort, 0);
        AMLOG_INFO(" mRTCPSocket %d.\n", mRTCPSocket);
        if (mRTCPSocket < 0) {
            //need change port and retry?
            AM_ERROR("SetupDatagramSocket(RTCP) fail, port %d, socket %d, need change port and retry, todo?\n", mRTCPPort, mRTCPSocket);
            return ME_ERROR;
        }
        tmp = 4096;
        if (setsockopt(mRTCPSocket, SOL_SOCKET, SO_SNDBUF, &tmp, sizeof(tmp)) < 0) {
            AM_ERROR("setsockopt(SO_SNDBUF): %s\n", strerror(errno));
            return ME_ERROR;
        }
    } else if (IParameters::StreamType_Audio == type) {
        if (false == mbAudioRTPSocketSetup) {
            mbAudioRTPSocketSetup = true;
        } else {
            AMLOG_WARN("socket has been setup, please check code.\n");
            return ME_OK;
        }

        //AM_ASSERT(-1 == mAudioRTPSocket);
        if (mAudioRTPSocket >= 0) {
            AMLOG_WARN("close previous socket here, %d.\n", mAudioRTPSocket);
            close(mAudioRTPSocket);
            mAudioRTPSocket = -1;
        }
        //AM_ASSERT(-1 == mAudioRTCPSocket);
        if (mAudioRTCPSocket >= 0) {
            AMLOG_WARN("close previous socket here, %d.\n", mAudioRTCPSocket);
            close(mAudioRTCPSocket);
            mAudioRTCPSocket = -1;
        }

        AMLOG_INFO(" before SetupDatagramSocket, port %d.\n", mAudioRTPPort);
        mAudioRTPSocket = SetupDatagramSocket(INADDR_ANY, mAudioRTPPort, 0);
        AMLOG_INFO(" mAudioRTPSocket %d.\n", mAudioRTPSocket);
        if (mAudioRTPSocket < 0) {
            //need change port and retry?
            AM_ERROR("SetupDatagramSocket(RTP) fail, port %d, socket %d, need change port and retry, todo?\n", mAudioRTPPort, mAudioRTPSocket);
            return ME_ERROR;
        }
         /* limit the tx buf size to limit latency */
        int tmp = 32768;
        if (setsockopt(mAudioRTPSocket, SOL_SOCKET, SO_SNDBUF, &tmp, sizeof(tmp)) < 0) {
            AM_ERROR("setsockopt(SO_SNDBUF): %s\n", strerror(errno));
            return ME_ERROR;
        }
        AMLOG_INFO(" before SetupDatagramSocket, port %d.\n", mAudioRTCPPort);
        mAudioRTCPSocket = SetupDatagramSocket(INADDR_ANY, mAudioRTCPPort, 0);
        AMLOG_INFO(" mRTCPSocket %d.\n", mAudioRTCPSocket);
        if (mAudioRTCPSocket < 0) {
            //need change port and retry?
            AM_ERROR("SetupDatagramSocket(RTCP) fail, port %d, socket %d, need change port and retry, todo?\n", mAudioRTCPPort, mAudioRTCPSocket);
            return ME_ERROR;
        }
        tmp = 4096;
        if (setsockopt(mAudioRTCPSocket, SOL_SOCKET, SO_SNDBUF, &tmp, sizeof(tmp)) < 0) {
            AM_ERROR("setsockopt(SO_SNDBUF): %s\n", strerror(errno));
            return ME_ERROR;
        }
    }else {
        AM_ERROR("BAD type %d.\n", type);
        return ME_BAD_PARAM;
    }

    return ME_OK;
}

int CFFMpegMuxer::updateRtpInfo(RtpDest &dest, AM_UINT timestamp,int is_video){
    CDoubleLinkedList *list = is_video ? &mVideoDstAddressList : &mAudioDstAddressList;
    dest.sock_fd = is_video ? mRTPSocket:mAudioRTPSocket;
    dest.addr_count = 0;
    SDstAddr*p_dest;
    CDoubleLinkedList::SNode* p_node = list->FirstNode();

    while (p_node) {
        p_dest = (SDstAddr*) p_node->p_context;
        if (p_dest) {
           p_dest->rtcp_stat.packet_count ++;//for RTCP SR
           dest.rtp_over_rtsp[dest.addr_count] = p_dest->parent->rtp_over_rtsp;
           if(!dest.rtp_over_rtsp[dest.addr_count]){
               dest.addr[dest.addr_count] = p_dest->addr_port;
           }else{
               dest.rtsp[dest.addr_count].rtsp_fd = p_dest->parent->rtsp_fd;
               dest.rtsp[dest.addr_count].channel = p_dest->parent->rtp_channel;
               dest.rtsp[dest.addr_count].callback = p_dest->parent->rtsp_callback;
           }
           if(!p_dest->stream_started){
              p_dest->start_pts = timestamp;
              p_dest->rtp_seq = p_dest->rtp_seq_base;
              p_dest->stream_started = 1;
           }
           dest.rtp_ts[dest.addr_count] =  htonl(p_dest->rtp_time_base + timestamp - p_dest->start_pts);
           dest.rtp_seq[dest.addr_count] = htons(p_dest->rtp_seq ++);
           dest.rtp_ssrc[dest.addr_count] = htonl(p_dest->rtp_ssrc);

           ++dest.addr_count;
           if(dest.addr_count >= MAX_DESTADDR_NUM){
               AM_WARNING("updateRtpInfo -- too many dest address\n");
               break;
           }
        }
        p_node = list->NextNode(p_node);
    }
    return 0;
}

AM_UINT CFFMpegMuxer::getVideoPacketDuration(AM_UINT timestamp)
{
    //TODO
    if(!mIsFirst){
        if(mRtpTimestamp == timestamp){
            return 0;
        }
        AM_UINT diff = 0;
        if(mRtpTimestamp < timestamp){
            diff = timestamp - mRtpTimestamp;
        }
        mRtpTimestamp = timestamp;
        AM_U32  duration = 100000 * diff/9;
        return duration;
    }else{
        mRtpTimestamp = timestamp;
        mIsFirst = false;
        return 0;
    }
}
void CFFMpegMuxer::nalSend(const AM_U8 *buf, AM_UINT size, AM_UINT last,AM_UINT timestamp)
{
    AMLOG_DEBUG("nalSend %x of len %d M=%d\n", buf[0] & 0x1F, size, last);
    if(!mVideoDstAddressList.NumberOfNodes()) return;
    if(!mRtpMemPool) return;
    if (size <= DRecommandMaxRTPPayloadLength) {
        CFixedMemPool::MemBlock block;
        if(mRtpMemPool->mallocBlock(block) < 0){
            AMLOG_ERROR("nalSend --- alloc memroy failed\n");
            return;
        }
        AM_U8* rtp_buf = block.buf + RTPBUF_RESERVED_SIZE;
        rtp_buf[0] = (RTP_VERSION << 6);
        rtp_buf[1] = (RTP_PT_H264 & 0x7f) | 0x80;//m and payload type
        memcpy(&rtp_buf[12], buf, size);
        RtpDest dest;
        updateRtpInfo(dest,timestamp);
        mRtpQueue.enqueue(&dest,block,size + 12,TYPE_VIDEO_RTP,getVideoPacketDuration(timestamp));
    } else {
        AM_U8 type = buf[0] & 0x1F;
        AM_U8 nri = buf[0] & 0x60;

        AMLOG_DEBUG("NAL size %d > %d\n", size, DRecommandMaxRTPPayloadLength);

        AM_U8 indicator = 28 | nri;
        AM_U8 fu_header = type | (1 << 7);
        buf += 1;
        size -= 1;
        while (size + 2 > DRecommandMaxRTPPayloadLength) {
            CFixedMemPool::MemBlock block;
            if(mRtpMemPool->mallocBlock(block) < 0){
                AMLOG_ERROR("nalSend --- alloc memroy failed\n");
                return;
            }
            AM_U8* rtp_buf = block.buf + RTPBUF_RESERVED_SIZE;
            //RTP header
            rtp_buf[0] = (RTP_VERSION << 6);
            rtp_buf[1] = (RTP_PT_H264 & 0x7f);//m and payload type
            rtp_buf[12] = indicator;
            rtp_buf[13] = fu_header;
            memcpy(&rtp_buf[12 + 2], buf, DRecommandMaxRTPPayloadLength - 2);
            RtpDest dest;
            updateRtpInfo(dest,timestamp);
            mRtpQueue.enqueue(&dest,block,DRecommandMaxRTPPayloadLength + 12,TYPE_VIDEO_RTP,getVideoPacketDuration(timestamp));
            buf += DRecommandMaxRTPPayloadLength - 2;
            size -= DRecommandMaxRTPPayloadLength - 2;
            fu_header &= ~(1 << 7);
        }
        fu_header |= 1 << 6;

        CFixedMemPool::MemBlock block;
        if(mRtpMemPool->mallocBlock(block) < 0){
            AMLOG_ERROR("nalSend --- alloc memroy failed\n");
            return;
        }
        AM_U8* rtp_buf = block.buf + RTPBUF_RESERVED_SIZE;
        rtp_buf[0] = (RTP_VERSION << 6);
        rtp_buf[1] = (RTP_PT_H264 & 0x7f) | 0x80;//m and payload type
        rtp_buf[12] = indicator;
        rtp_buf[13] = fu_header;
        memcpy(&rtp_buf[12 + 2], buf, size);
        RtpDest dest;
        updateRtpInfo(dest,timestamp);
        mRtpQueue.enqueue(&dest,block,size + 2 + 12,TYPE_VIDEO_RTP,getVideoPacketDuration(timestamp));
    }
}
void CFFMpegMuxer::rtpProcessData(AM_U8* srcbuf, AM_UINT srclen, AM_UINT timestamp,IParameters::StreamFormat format)
{
    AM_ASSERT(srcbuf && srclen);
    //AM_ASSERT(IParameters::StreamFormat_H264 == format);
    AM_U8* ptmp, *ptmp1;

    AMLOG_DEBUG("start 8 bytes(%p,%p,%u): %x %x %x %x, %x %x %x %x.\n", srcbuf, (srcbuf + srclen), srclen, srcbuf[0], srcbuf[1], srcbuf[2], srcbuf[3], srcbuf[4], srcbuf[5], srcbuf[6], srcbuf[7]);

    if (IParameters::StreamFormat_H264 == format) {
        ReceiveRtpRtcpPackets(1);
        check_send_rtcp_sr(timestamp,srclen,1);

        if (srcbuf && srclen) {
            //AMLOG_INFO("rtpProcessData srcbuf %p, srclen %u.\n", srcbuf, srclen);

            ptmp = _find_startcode(srcbuf, srcbuf + srclen);
            AMLOG_DEBUG("ptmp %p, offset %u.\n", ptmp, (AM_UINT)(ptmp - srcbuf));
            while (ptmp < (srcbuf + srclen)) {
                while (!*(ptmp++));

                ptmp1 = _find_startcode(ptmp, srcbuf + srclen);
                AMLOG_DEBUG("ptmp1 %p, offset %u.\n", ptmp1, (AM_UINT)(ptmp1 - srcbuf));
                nalSend(ptmp, ptmp1 - ptmp, (ptmp1 == (srcbuf + srclen)),timestamp);
                ptmp = ptmp1;
            }
        }
    } else if (IParameters::StreamFormat_AAC == format) {
        ReceiveRtpRtcpPackets(0);
        check_send_rtcp_sr(timestamp,srclen,0);
        aacSend(srcbuf, srclen,timestamp);
    } else {
        AM_ERROR("please check code, format %d.\n", format);
    }
}
void CFFMpegMuxer::aacSend(const AM_U8 *buf, AM_UINT size,AM_UINT timestamp)
{
#define AAC_HEADER_LEN 4
    AMLOG_DEBUG("aacSend len %d\n", size);
    if(!mAudioDstAddressList.NumberOfNodes()) return;
    if(!mRtpMemPool) return;
    if ((size + AAC_HEADER_LEN) <= DRecommandMaxRTPPayloadLength) {
        CFixedMemPool::MemBlock block;
        if(mRtpMemPool->mallocBlock(block) < 0){
            AMLOG_ERROR("aacSend --- alloc memroy failed\n");
            return;
        }
        AM_U8* rtp_buf = block.buf + RTPBUF_RESERVED_SIZE;
        //RTP header
        rtp_buf[0] = (RTP_VERSION << 6);
        rtp_buf[1] = (RTP_PT_AAC & 0x7f) | 0x80;//m and payload type
        rtp_buf[12] = 0x00;
        rtp_buf[13] = 0x10;
        rtp_buf[14] = (size & 0x1fe0) >> 5;
        rtp_buf[15] = (size & 0x1f) << 3;
        memcpy(&rtp_buf[16], buf, size);
        RtpDest dest;
        updateRtpInfo(dest,timestamp,0);
        mRtpQueue.enqueue(&dest,block, size + 12 + AAC_HEADER_LEN,TYPE_AUDIO_RTP);
    } else {
        AMLOG_WARN("aac size %d > %d\n", size, DRecommandMaxRTPPayloadLength);
/*
        //first fragment with header
        mpRTPBuffer[12] = 0x00;
        mpRTPBuffer[13] = 0x10;
        mpRTPBuffer[14] = ((DRecommandMaxRTPPayloadLength - AAC_HEADER_LEN) & 0x1fe0) >> 5;
        mpRTPBuffer[15] = ((DRecommandMaxRTPPayloadLength - AAC_HEADER_LEN) & 0x1f) << 3;

        mpRTPBuffer[2] = mRTPAudioSeqNumber >> 8;//seq number
        mpRTPBuffer[3] = mRTPAudioSeqNumber & 0xff;

        mRtpQueue.enqueue(mpRTPBuffer, DRecommandMaxRTPPayloadLength + 12,0,0);
        mRTPAudioSeqNumber ++;
        ++mRtcpStat[1].packet_count;
        buf += DRecommandMaxRTPPayloadLength - AAC_HEADER_LEN;
        size -= DRecommandMaxRTPPayloadLength - AAC_HEADER_LEN;

        //fragment without header
        while (size > DRecommandMaxRTPPayloadLength) {
            memcpy(&mpRTPBuffer[12], buf, DRecommandMaxRTPPayloadLength);

            mpRTPBuffer[2] = mRTPAudioSeqNumber >> 8;//seq number
            mpRTPBuffer[3] = mRTPAudioSeqNumber & 0xff;

            mRtpQueue.enqueue(mpRTPBuffer, DRecommandMaxRTPPayloadLength + 12,0,0);
            mRTPAudioSeqNumber ++;
            ++mRtcpStat[1].packet_count;
            buf += DRecommandMaxRTPPayloadLength;
            size -= DRecommandMaxRTPPayloadLength;
        }

        //RTP last marker
        mpRTPBuffer[1] |= 0x80;
        memcpy(&mpRTPBuffer[12], buf, size);

        mpRTPBuffer[2] = mRTPAudioSeqNumber >> 8;//seq number
        mpRTPBuffer[3] = mRTPAudioSeqNumber & 0xff;
        mRtpQueue.enqueue(mpRTPBuffer, size + 12,0,0);
        mRTPAudioSeqNumber ++;
        ++mRtcpStat[1].packet_count;
*/
    }
}

int CFFMpegMuxer::updateSendRtcp(RtpDest &dest, rtcp_stat_t *s,AM_UINT ssrc, AM_UINT timestamp,int is_video){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    AM_S64 systime = (AM_S64)tv.tv_sec * 1000000 + tv.tv_usec;

    #define NTP_OFFSET 2208988800ULL
    #define NTP_OFFSET_US (NTP_OFFSET * 1000000ULL)
    AM_S64 ntp_time = (systime/1000 * 1000) + NTP_OFFSET_US;

    AM_INT rtcp_bytes = (s->octet_count - s->last_octet_count) * 5/1000;
    if (s->first_packet || ((rtcp_bytes >= 28) &&
            (ntp_time - s->last_rtcp_ntp_time > 5000000) && (s->timestamp != timestamp))) {
        CFixedMemPool::MemBlock block;
        if(mRtpMemPool->mallocBlock(block) < 0){
            AMLOG_ERROR("rtcp --- alloc memroy failed\n");
            return -1;
        }
        AM_U8* rtcp_sr_buf = block.buf + RTPBUF_RESERVED_SIZE;
        if(s->first_packet){
            s->first_rtcp_ntp_time = ntp_time;
            s->base_timestamp = timestamp;
            s->first_packet = 0;
        }
        //timebase, hard code now
        AVRational time_base;
        if(is_video){
            time_base.den = 90000;
            time_base.num = 1;
        }else{
            CFFMpegMuxerInput* pInput= NULL;
            for(int i = 0; i < (int)mTotalInputPinNumber; i++){
                pInput = mpInput[i];
                if(pInput->mType == StreamType_Audio){
                    time_base.den = pInput->mParams.specific.audio.sample_rate;
                }
            }
            time_base.num = 1;
        }
        AM_UINT rtp_ts = av_rescale_q(ntp_time - s->first_rtcp_ntp_time, (AVRational){1, 1000000},time_base) + s->base_timestamp;
        int len = makeup_rtcp_sr(rtcp_sr_buf,DRecommandMaxUDPPayloadLength,ssrc,ntp_time,rtp_ts,s->packet_count,s->octet_count);
        if(is_video){
            mRtpQueue.enqueue(&dest,block,len, TYPE_VIDEO_RTCP);
        }else{
            mRtpQueue.enqueue(&dest,block,len, TYPE_AUDIO_RTCP);
        }
        //AMLOG_WARN("updateSendRtcp---- is_video = %d\n",is_video);
        s->last_octet_count = s->octet_count;
        s->last_rtcp_ntp_time = ntp_time;
        s->timestamp = timestamp;
    }
    return 0;
}

AM_INT CFFMpegMuxer::check_send_rtcp_sr(AM_UINT pts,int size,int is_video)
{
    if(is_video) {//video
        if(!mVideoDstAddressList.NumberOfNodes()) return 0;
    }else{
        if(!mAudioDstAddressList.NumberOfNodes()) return 0;
    }
    if(!mRtpMemPool) return 0;

    CDoubleLinkedList *list = is_video ?  &mVideoDstAddressList : &mAudioDstAddressList;
    SDstAddr*p_dest;
    CDoubleLinkedList::SNode* p_node = list->FirstNode();
    while (p_node) {
        p_dest = (SDstAddr*) p_node->p_context;
        if (p_dest) {
           p_dest->rtcp_stat.octet_count += size;
//           p_dest->rtp_time_base + pts - p_dest->start_pts;
           AM_UINT timestamp;
           if(p_dest->stream_started){
               timestamp = p_dest->rtp_time_base + pts - p_dest->start_pts;
           }else{
               timestamp = p_dest->rtp_time_base;
           }
           RtpDest dest;
           dest.sock_fd =is_video ? mRTCPSocket:mAudioRTCPSocket;
           dest.addr_count = 1;
           dest.rtp_over_rtsp[0] = p_dest->parent->rtp_over_rtsp;
           if(!dest.rtp_over_rtsp[0]){
               dest.addr[0] = p_dest->addr_port_ext;
           }else{
               dest.rtsp[0].rtsp_fd = p_dest->parent->rtsp_fd;
               dest.rtsp[0].channel = p_dest->parent->rtcp_channel;
               dest.rtsp[0].callback = p_dest->parent->rtsp_callback;
           }
           updateSendRtcp(dest,&p_dest->rtcp_stat,p_dest->rtp_ssrc, timestamp, is_video);
        }
        p_node = list->NextNode(p_node);
    }
    return 0;
}

AM_INT  CFFMpegMuxer::makeup_rtcp_sr(AM_U8 *buf, AM_INT len, AM_UINT ssrc, AM_S64 ntp_time,AM_UINT timestamp,AM_UINT packet_count,AM_UINT octet_count)
{
    if(!buf || len < 28) {
        return -1;
    }
    buf[0]  = (2 << 6); //RTP_VERSION
    buf[1]  = (200); //RTCP_SR
    buf[2]  = 0;
    buf[3]  = 6; // length in words - 1
    buf[4]  = (ssrc >> 24) & 0xff;
    buf[5]  = (ssrc >> 16) & 0xff;
    buf[6]  = (ssrc >> 8) & 0xff;
    buf[7]  = (ssrc >> 0) & 0xff;
    AM_UINT ntp_time_1 = ntp_time / 1000000;
    AM_UINT ntp_time_2 = ((ntp_time % 1000000) << 32) / 1000000;
    buf[8]  = (ntp_time_1 >> 24) & 0xff;
    buf[9]  = (ntp_time_1 >> 16) & 0xff;
    buf[10]  = (ntp_time_1 >> 8) & 0xff;
    buf[11]  = (ntp_time_1 >> 0) & 0xff;
    buf[12]  = (ntp_time_2 >> 24) & 0xff;
    buf[13]  = (ntp_time_2 >> 16) & 0xff;
    buf[14]  = (ntp_time_2 >> 8) & 0xff;
    buf[15]  = (ntp_time_2 >> 0) & 0xff;
    buf[16]  = (timestamp >> 24) & 0xff;
    buf[17]  = (timestamp >> 16) & 0xff;
    buf[18]  = (timestamp >> 8) & 0xff;
    buf[19]  = (timestamp >> 0) & 0xff;
    buf[20]  = (packet_count >> 24) & 0xff;
    buf[21]  = (packet_count >> 16) & 0xff;
    buf[22]  = (packet_count >> 8) & 0xff;
    buf[23]  = (packet_count >> 0) & 0xff;
    buf[24]  = (octet_count >> 24) & 0xff;
    buf[25]  = (octet_count >> 16) & 0xff;
    buf[26]  = (octet_count >> 8) & 0xff;
    buf[27]  = (octet_count >> 0) & 0xff;
    return 28;
}


//
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
void CFFMpegMuxer::ReceiveRtpRtcpPackets(int is_video)
{
    //#define POLLIN     0x0001  /* any readable data available */
    struct sockaddr from;
    socklen_t from_len;
    int len,n;
    struct pollfd p[2] = {{mRTCPSocket, POLLIN, 0}, {mRTPSocket, POLLIN, 0}};
    if(!is_video){
        p[0].fd = mAudioRTCPSocket;
        p[1].fd = mAudioRTPSocket;
    }
    n = poll(p, 2, 0);
    if (n > 0) {
        /*RTCP*/
        if (p[0].revents & POLLIN) {
        rtcp_retry:
            from_len = sizeof(from);
            len = recvfrom(p[0].fd, (uint8_t *)mpRTPBuffer, mRTPBufferTotalLength, 0, (struct sockaddr *)&from, &from_len);
            if (len < 0) {
                if (errno == EAGAIN ||errno == EINTR){
                    goto rtcp_retry;
                }
            }
            notifyRtcpPacket(is_video,mpRTPBuffer,len,(struct sockaddr_in*)&from);
            if(--n <= 0) {
                return;
            }
        }
        /*RTP*/
        if (p[1].revents & POLLIN) {
        rtp_retry:
            from_len = sizeof(from);
            len = recvfrom (p[1].fd, (uint8_t *)mpRTPBuffer, mRTPBufferTotalLength, 0, (struct sockaddr *)&from, &from_len);
            if (len < 0) {
               if (errno == EAGAIN ||errno == EINTR){
                   goto rtp_retry;
               }
           }
        }
    }
}

static  AM_S64  get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (AM_S64)tv.tv_sec * 1000000 + tv.tv_usec;
}
void CFFMpegMuxer::notifyRtcpPacket(int is_video,unsigned char *buf, int len, struct sockaddr_in *from){
    CDoubleLinkedList *list = is_video ?  &mVideoDstAddressList : &mAudioDstAddressList;
    SDstAddr *dest = NULL;
    CDoubleLinkedList::SNode* p_node = list->FirstNode();
    while (p_node) {
        SDstAddr *p_dest = (SDstAddr*) p_node->p_context;
        if (p_dest
          && p_dest->addr_port_ext.sin_addr.s_addr == from->sin_addr.s_addr
          && p_dest->addr_port_ext.sin_port == from->sin_port) {
            dest = p_dest;
            break;
        }
        p_node = list->NextNode(p_node);
    }
    if(dest && dest->parent && dest->parent->parent){
        pthread_mutex_lock(&dest->parent->parent->lock);
        dest->parent->parent->last_cmd_time = get_time();
        pthread_mutex_unlock(&dest->parent->parent->lock);
    }
}

