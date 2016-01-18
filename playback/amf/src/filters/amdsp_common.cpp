
/*
 * amdsp_common.cpp
 *
 * History:
 *    2010/12/02 - [Zhi He] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
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

#define LOG_NDEBUG 0
#define LOG_TAG "amdsp_common"
//#define AMDROID_DEBUG

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

extern "C" {
#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_transcode_drv.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#if PLATFORM_ANDROID
#include "vout.h"
#include "amba_rv40_neon.h"
#include "amba_dec_dsputil.h"
#endif
}

#if (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
#include "amba_dsp_ione.h"
#elif TARGET_USE_AMBARELLA_A5S_DSP
#include "amba_dsp_a5s.h"
#endif

#include "amdsp_common.h"

typedef struct _SBitsWriter {
    AM_U8* pStart, *pCur;
    AM_UINT size;
    AM_UINT leftBits;
    AM_UINT leftBytes;
    AM_UINT full;
} SBitsWriter;

SBitsWriter* CreateBitsWriter(AM_U8* pStart, AM_UINT size)
{
    SBitsWriter* pWriter = (SBitsWriter*) malloc(sizeof(SBitsWriter));

    //memset(pStart, 0x0, size);
    pWriter->pStart = pStart;
    pWriter->pCur = pStart;
    pWriter->size = size;
    pWriter->leftBits = 8;
    pWriter->leftBytes = size;
    pWriter->full = 0;

    return pWriter;
}

void DeleteBitsWriter(SBitsWriter* pWriter)
{
    free(pWriter);
}

AM_INT WriteBits(SBitsWriter* pWriter, AM_UINT value, AM_UINT bits)
{
//    AM_UINT shift = 0;

    AM_ASSERT(!pWriter->full);
    AM_ASSERT(pWriter->size == (pWriter->pCur - pWriter->pStart + pWriter->leftBytes));
    AM_ASSERT(pWriter->leftBits <= 8);

    if (pWriter->full) {
        AM_ASSERT(0);
        return -1;
    }

    while (bits > 0 && !pWriter->full) {
        if (bits <= pWriter->leftBits) {
            AM_ASSERT(pWriter->leftBits <= 8);
            *pWriter->pCur |= (value << (32 - bits)) >> (32 - pWriter->leftBits);
            pWriter->leftBits -= bits;
            //value >>= bits;

            if (pWriter->leftBits == 0) {
                if (pWriter->leftBytes == 0) {
                    pWriter->full = 1;
                    return 0;
                }
                pWriter->leftBits = 8;
                pWriter->leftBytes --;
                pWriter->pCur ++;
            }
            return 0;
        } else {
            AM_ASSERT(pWriter->leftBits <= 8);
            *pWriter->pCur |= (value << (32 - bits)) >> (32 - pWriter->leftBits);
            value <<= 32 - bits + pWriter->leftBits;
            value >>= 32 - bits + pWriter->leftBits;
            bits -= pWriter->leftBits;

            if (pWriter->leftBytes == 0) {
                pWriter->full = 1;
                return 0;
            }
            pWriter->leftBits = 8;
            pWriter->leftBytes --;
            pWriter->pCur ++;
        }

    }
    return -2;
}


AM_UINT FillUSEQHeader(AM_U8* pHeader, AM_U32 vFormat, AM_U32 timeScale, AM_U32 tickNum, AM_U32 is_mp4s_flag, AM_S32 vid_container_width, AM_S32 vid_container_height)
{
    if(vFormat==UDEC_VFormat_MPEG12)//now mpeg12 ONLY add PES header, NO add seq header, 2012.01.05, roy mdf
        return 0;

    AM_ASSERT(pHeader);
    AM_ASSERT(vFormat>=UDEC_VFormat_H264);
    AM_ASSERT(vFormat<=UDEC_VFormat_WMV3);
    AM_INT ret = 0;


    SBitsWriter* pWriter = CreateBitsWriter(pHeader, is_mp4s_flag?DUDEC_SEQ_HEADER_EX_LENGTH:DUDEC_SEQ_HEADER_LENGTH);
    AM_ASSERT(pWriter);
    memset(pHeader, 0x0, is_mp4s_flag?DUDEC_SEQ_HEADER_EX_LENGTH:DUDEC_SEQ_HEADER_LENGTH);

    //start code prefix
    ret |= WriteBits(pWriter, 0, 16);
    ret |= WriteBits(pWriter, 1, 8);

    //start code
    if (vFormat == UDEC_VFormat_H264) {
        ret |= WriteBits(pWriter, DUDEC_SEQ_STARTCODE_H264, 8);
    } else if (vFormat == UDEC_VFormat_MPEG4) {
        ret |= WriteBits(pWriter, DUDEC_SEQ_STARTCODE_MPEG4, 8);
    } else if (vFormat == UDEC_VFormat_VC1 || vFormat == UDEC_VFormat_WMV3) {
        ret |= WriteBits(pWriter, DUDEC_SEQ_STARTCODE_VC1WMV3, 8);
    }

#ifdef _add_signature_code_
    //add Signature Code 0x2449504F "$IPO"
    ret |= WriteBits(pWriter, 0x2449504F, 32);
#endif

    //video format
    ret |= WriteBits(pWriter, vFormat, 8);

    //Time Scale low
    ret |= WriteBits(pWriter, (timeScale & 0xff00)>>8, 8);
    ret |= WriteBits(pWriter, timeScale & 0xff, 8);

    //markbit
    ret |= WriteBits(pWriter, 1, 1);

    //Time Scale high
    ret |= WriteBits(pWriter, (timeScale & 0xff000000) >> 24, 8);
    ret |= WriteBits(pWriter, (timeScale &0x00ff0000) >> 16, 8);

    //markbit
    ret |= WriteBits(pWriter, 1, 1);

    //num units tick low
    ret |= WriteBits(pWriter, (tickNum & 0xff00)>>8, 8);
    ret |= WriteBits(pWriter, tickNum & 0xff, 8);

    //markbit
    ret |= WriteBits(pWriter, 1, 1);

    //num units tick high
    ret |= WriteBits(pWriter, (tickNum & 0xff000000) >> 24, 8);
    ret |= WriteBits(pWriter, (tickNum &0x00ff0000)>> 16, 8);

    //resolution info flag(0:no res info, 1:has res info)
    ret |= WriteBits(pWriter, (is_mp4s_flag &0x1), 1);

    if(is_mp4s_flag)
    {
        //pic width
        ret |= WriteBits(pWriter, (vid_container_width & 0x7f00)>>8, 7);
        ret |= WriteBits(pWriter, vid_container_width & 0xff, 8);

        //markbit
        ret |= WriteBits(pWriter, 3, 2);

        //pic height
        ret |= WriteBits(pWriter, (vid_container_height & 0x7f00)>>8, 7);
        ret |= WriteBits(pWriter, vid_container_height & 0xff, 8);
    }

    //padding
    ret |= WriteBits(pWriter, 0xffffffff, 20);

    AM_ASSERT(pWriter->leftBits == 0 || pWriter->leftBits == 8);
    AM_ASSERT(pWriter->leftBytes == 0);
    AM_ASSERT(!pWriter->full);
    AM_ASSERT(!ret);

    DeleteBitsWriter(pWriter);

#if 0
    memset(pHeader, 0x0, is_mp4s_flag?DUDEC_SEQ_HEADER_EX_LENGTH:DUDEC_SEQ_HEADER_LENGTH);

    //start code prefix
    pHeader[2] = 0x1;

    //start code
    if (vFormat == UDEC_VFormat_H264) {
        pHeader[3] = DUDEC_STARTCODE_H264;
    } else if (vFormat == UDEC_VFormat_MPEG4) {
        pHeader[3] = DUDEC_STARTCODE_MPEG4;
    } else if (vFormat == UDEC_VFormat_VC1 || vFormat == UDEC_VFormat_WMV3) {
        pHeader[3] = DUDEC_STARTCODE_VC1WMV3;
    }

    //Video Format: 1:h264, 2:mpeg4, 3:vc-1 advanced profile, 4:wmv3(vc-1 simple&main profile)
    pHeader[4] = vFormat;

    //time scale low
    pHeader[5] = timeScale & 0xff;
    pHeader[6] = (timeScale & 0xff00) >> 8;

    //time scale high and mark bit
    pHeader[7] = ((timeScale & 0xfe0000) >> 16)|0x1;
    pHeader[8] = ((timeScale & 0x7f800000) >> 23);
    pHeader[9] = (timeScale >> 31) | 0x2;

    //num of ticks low
    pHeader[9] |= (tickNum & 0x3f) << 2;
    pHeader[10] = (tickNum & 0x3fc0) >> 6;
    pHeader[11] = ((tickNum & 0xc000) >> 14) | 0x4;

    //num of ticks high
    pHeader[11] |= (tickNum & 0x1f0000) >> 13;
    pHeader[12] =  (tickNum & 0x1fe00000) >> 21;
    pHeader[13] =  (tickNum & 0xe0000000) >> 29;

    //reserved = 2*8 + 5 = 21;
#endif

    return is_mp4s_flag?DUDEC_SEQ_HEADER_EX_LENGTH:DUDEC_SEQ_HEADER_LENGTH;
}

void InitUPESHeader(AM_U8* pHeader, AM_U32 vFormat)
{
    AM_ASSERT(pHeader);
    AM_ASSERT(vFormat>=UDEC_VFormat_H264);
    AM_ASSERT(vFormat<=UDEC_VFormat_MPEG12);

    memset(pHeader, 0x0, DUDEC_PES_HEADER_LENGTH);

    //start code prefix
    pHeader[2] = 0x1;

    //start code
    if (vFormat == UDEC_VFormat_H264) {
        pHeader[3] = DUDEC_PES_STARTCODE_H264;
    } else if (vFormat == UDEC_VFormat_MPEG4) {
        pHeader[3] = DUDEC_PES_STARTCODE_MPEG4;
    } else if (vFormat == UDEC_VFormat_VC1 || vFormat == UDEC_VFormat_WMV3) {
        pHeader[3] = DUDEC_PES_STARTCODE_VC1WMV3;
    }
    else if(vFormat == UDEC_VFormat_MPEG12) {
        pHeader[3] = DUDEC_PES_STARTCODE_MPEG12;
    }

#ifdef _add_signature_code_
    //add Signature Code 0x2449504F "$IPO"
    pHeader[4] = 0x24;
    pHeader[5] = 0x49;
    pHeader[6] = 0x50;
    pHeader[7] = 0x4F;
#endif
}

AM_UINT FillPESHeader(AM_U8* pHeader, AM_U32 ptsLow, AM_U32 ptsHigh, AM_U32 auDatalen, AM_UINT hasPTS, AM_UINT isPTSJump)
{
    AM_ASSERT(pHeader);

    AM_INT ret = 0;

#ifdef _add_signature_code_
    SBitsWriter* pWriter = CreateBitsWriter(pHeader + 8, DUDEC_PES_HEADER_LENGTH - 8);
    AM_ASSERT(pWriter);
    memset(pHeader + 8, 0x0, DUDEC_PES_HEADER_LENGTH - 8);
#else
    SBitsWriter* pWriter = CreateBitsWriter(pHeader + 4, DUDEC_PES_HEADER_LENGTH - 4);
    AM_ASSERT(pWriter);
    memset(pHeader + 4, 0x0, DUDEC_PES_HEADER_LENGTH - 4);
#endif

    ret |= WriteBits(pWriter, hasPTS, 1);

    if (hasPTS) {
        //pts 0
        ret |= WriteBits(pWriter, (ptsLow & 0xff00)>>8, 8);
        ret |= WriteBits(pWriter, ptsLow & 0xff, 8);
        //marker bit
        ret |= WriteBits(pWriter, 1, 1);
        //pts 1
        ret |= WriteBits(pWriter, (ptsLow & 0xff000000) >> 24, 8);
        ret |= WriteBits(pWriter, (ptsLow & 0xff0000) >> 16, 8);
        //marker bit
        ret |= WriteBits(pWriter, 1, 1);
        //pts 2
        ret |= WriteBits(pWriter, (ptsHigh & 0xff00)>>8, 8);
        ret |= WriteBits(pWriter, ptsHigh & 0xff, 8);

        //marker bit
        ret |= WriteBits(pWriter, 1, 1);
        //pts 3
        ret |= WriteBits(pWriter, (ptsHigh & 0xff000000) >> 24, 8);
        ret |= WriteBits(pWriter, (ptsHigh & 0xff0000) >> 16, 8);

        //marker bit
        ret |= WriteBits(pWriter, 1, 1);

        //PTS_JUMP
        ret |= WriteBits(pWriter, isPTSJump, 1);

        //padding
        ret |= WriteBits(pWriter, 0xffffffff, 27);
    }

    //au_data_length low
    //AM_PRINTF("auDatalen %d, 0x%x.\n", auDatalen, auDatalen);

    ret |= WriteBits(pWriter, (auDatalen & 0xff00)>>8, 8);
    ret |= WriteBits(pWriter, (auDatalen & 0xff), 8);
    //marker bit
    ret |= WriteBits(pWriter, 1, 1);
    //au_data_length high
    ret |= WriteBits(pWriter, (auDatalen & 0x07000000) >> 24, 3);
    ret |= WriteBits(pWriter, (auDatalen &0x00ff0000)>> 16, 8);
    //pading
    ret |= WriteBits(pWriter, 0xf, 3);

    AM_ASSERT(pWriter->leftBits == 0 || pWriter->leftBits == 8);
    AM_ASSERT(!ret);

#ifdef _add_signature_code_
    ret = pWriter->size - pWriter->leftBytes + 8;
#else
    ret = pWriter->size - pWriter->leftBytes + 4;
#endif

    DeleteBitsWriter(pWriter);
    return ret;

}

AM_UINT FillPrivateGopNalHeader(AM_U8* pHeader, AM_U32 timeScale, AM_U32 tickNum, AM_UINT m, AM_UINT n, AM_U32 pts_high, AM_U32 pts_low)
{
    AM_INT ret = 0;
    AM_ASSERT(pHeader);

    SBitsWriter* pWriter = CreateBitsWriter(pHeader, DPRIVATE_GOP_NAL_HEADER_LENGTH);
    AM_ASSERT(pWriter);
    memset(pHeader, 0x0, DPRIVATE_GOP_NAL_HEADER_LENGTH);

    AM_WARNING("print GOP header: time scale %d, tick %d, m %d, n %d, pts_high 0x%08x, pts_low 0x%08x\n", timeScale, tickNum, m, n, pts_high, pts_low);

    //start code prefix
    ret |= WriteBits(pWriter, 0, 24);
    ret |= WriteBits(pWriter, 1, 8);

    //start code
    ret |= WriteBits(pWriter, 0x7a, 8);

    //version main
    ret |= WriteBits(pWriter, 0x01, 8);
    //version main
    ret |= WriteBits(pWriter, 0x01, 8);

    ret |= WriteBits(pWriter, 0x00, 2);

    //num units tick high
    ret |= WriteBits(pWriter, (tickNum & 0xff000000) >> 24, 8);
    ret |= WriteBits(pWriter, (tickNum &0x00ff0000)>> 16, 8);

    //markbit
    ret |= WriteBits(pWriter, 1, 1);

    //num units tick low
    ret |= WriteBits(pWriter, (tickNum & 0xff00)>>8, 8);
    ret |= WriteBits(pWriter, tickNum & 0xff, 8);

    //markbit
    ret |= WriteBits(pWriter, 1, 1);

    //Time Scale high
    ret |= WriteBits(pWriter, (timeScale & 0xff000000) >> 24, 8);
    ret |= WriteBits(pWriter, (timeScale &0x00ff0000) >> 16, 8);

    //markbit
    ret |= WriteBits(pWriter, 1, 1);

    //Time Scale low
    ret |= WriteBits(pWriter, (timeScale & 0xff00)>>8, 8);
    ret |= WriteBits(pWriter, timeScale & 0xff, 8);

    //markbit
    ret |= WriteBits(pWriter, 1, 1);

    //Time PTS high
    ret |= WriteBits(pWriter, (pts_low& 0xff000000) >> 24, 8);
    ret |= WriteBits(pWriter, (pts_low &0x00ff0000) >> 16, 8);

    //markbit
    ret |= WriteBits(pWriter, 1, 1);

    //Time PTS low
    ret |= WriteBits(pWriter, (pts_low & 0xff00)>>8, 8);
    ret |= WriteBits(pWriter, pts_low & 0xff, 8);

    //markbit
    ret |= WriteBits(pWriter, 1, 1);

    //n
    ret |= WriteBits(pWriter, n, 8);

    //m
    ret |= WriteBits(pWriter, m, 4);

    //padding
    ret |= WriteBits(pWriter, 0xff, 4);

    AM_ASSERT(pWriter->leftBits == 0 || pWriter->leftBits == 8);
    AM_ASSERT(pWriter->leftBytes == 0);
    AM_ASSERT(!ret);

    DeleteBitsWriter(pWriter);

    return DPRIVATE_GOP_NAL_HEADER_LENGTH;
}

AM_ERR AM_CreateDSPHandler(void* pShared)
{
    if (!pShared) {
        AM_ASSERT(0);
        AM_ERROR("NULL pointer in AM_CreateDSPHandler.\n");
        return ME_BAD_PARAM;
    }

//open when compiled
#if 1
#if (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
    CAmbaDspIOne::Create((SConsistentConfig*)pShared);
#elif TARGET_USE_AMBARELLA_A5S_DSP
    CAmbaDspA5s::Create((SConsistentConfig*)pShared);
#endif
#else
    AM_ERROR("need enable compile CAmbaDspIOne.\n");
    return ME_ERROR;
#endif

    if (((SConsistentConfig*)pShared)->udecHandler == NULL) {
        AM_ERROR("NULL udecHandler in AM_CreateDSPHandler.\n");
        return ME_ERROR;
    }
    return ME_OK;
}

AM_ERR AM_DeleteDSPHandler(void* pShared)
{
    if (!pShared) {
        AM_ASSERT(0);
        AM_ERROR("NULL pointer in AM_CreateDSPHandler.\n");
        return ME_BAD_PARAM;
    }

    if (((SConsistentConfig*)pShared)->udecHandler == NULL) {
        AM_ERROR("NULL udecHandler in AM_DeleteDSPHandler.\n");
        return ME_ERROR;
    }

//open when compiled
#if 1
#if (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
    ((CAmbaDspIOne*)(((SConsistentConfig*)pShared)->udecHandler))->Delete();
#elif TARGET_USE_AMBARELLA_A5S_DSP
    ((CAmbaDspA5s*)(((SConsistentConfig*)pShared)->udecHandler))->Delete();
#endif
#else
    AM_ERROR("need enable compile CAmbaDspIOne.\n");
    return ME_ERROR;
#endif

    return ME_OK;
}
//-----------------------------------------------------------------------
//
// CIAVVideoBufferPool
//
//-----------------------------------------------------------------------
CIAVVideoBufferPool *CIAVVideoBufferPool::Create(int iavFd, AM_UINT picWidth, AM_UINT picHeight, bool extEdge, amba_decoding_accelerator_t* pAcc)
{
    CIAVVideoBufferPool *result = NULL;

#if TARGET_USE_AMBARELLA_A5S_DSP
    AM_ASSERT(!pAcc);
    result = new CA5SVideoBufferPool(iavFd, picWidth, picHeight, extEdge, pAcc);
#elif (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
    result = new CIOneVideoBufferPool(iavFd, picWidth, picHeight, extEdge, pAcc);
#endif
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

AM_ERR CIAVVideoBufferPool::Construct()
{
    AM_ERR err = OpenDevice();
    if (err != ME_OK || !mBufferNum)
        return ME_ERROR;

    err = inherited::Construct(mBufferNum, sizeof(CVideoBuffer));
    if (err != ME_OK) {
        CloseDevice();
        return err;
    }

    return ME_OK;
}


CIAVVideoBufferPool::~CIAVVideoBufferPool()
{
    AMLOG_DESTRUCTOR("~CIAVVideoBufferPool.\n");
    //CloseDevice(); //should be OVERRIDED and CALLED by sub-class and should not called here
    AMLOG_DESTRUCTOR("~CIAVVideoBufferPool done.\n");
}

bool CIAVVideoBufferPool::AllocBuffer(CBuffer*& pBuffer, AM_UINT size)
{
    if (!inherited::AllocBuffer(pBuffer, size)) {
        AM_ERROR("CIAVVideoBufferPool inherited::AllocBuffer fail.\n");
        return false;
    }
    iav_frame_buffer_t fb;

    if (::ioctl(mIavFd, IAV_IOC_GET_FRAME_BUFFER, &fb) < 0) {
        //release buffer from inherited::AllocBuffer
        pBuffer->Release();
        AM_ERROR("CIAVVideoBufferPool GET IAV buffer fail.\n");
        return false;
    }

    pBuffer->SetType(CBuffer::DATA);
    pBuffer->SetDataSize(0);
    pBuffer->SetDataPtr(NULL);

    CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pBuffer;
    pVideoBuffer->pLumaAddr = (AM_U8*)fb.lu_buf_addr;
    pVideoBuffer->pChromaAddr = (AM_U8*)fb.ch_buf_addr;
    pVideoBuffer->buffer_id= fb.fb_id;
    //pVideoBuffer->fbp_id= fb.fbp_id;
    pVideoBuffer->picWidth = mPicWidth;
    pVideoBuffer->picHeight = mPicHeight;
    pVideoBuffer->fbWidth = mFbWidth;
    pVideoBuffer->fbHeight = mFbHeight;
    //AM_INFO("CIAVVideoBufferPool mPicWidth %d, mPicHeight %d, mFbWidth %d, mFbHeight %d, ext %d.\n", mPicWidth, mPicHeight, mFbWidth, mFbHeight, mbExtEdge);
    if (mbExtEdge) {
        pVideoBuffer->picXoff = 16;
        pVideoBuffer->picYoff = 16;
    } else {
        pVideoBuffer->picXoff = 0;
        pVideoBuffer->picYoff = 0;
    }

    pVideoBuffer->flags = IAV_FRAME_FORCE_RELEASE;

    return true;
}

void CIAVVideoBufferPool::OnReleaseBuffer(CBuffer *pBuffer)
{
    AM_DEBUG_INFO("**OnReleaseBuffer, pBuffer_type = %d.\n", pBuffer->GetType());
    if (pBuffer->GetType() != CBuffer::DATA)
        return;

    CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pBuffer;
    iav_decoded_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.flags = (u16)pVideoBuffer->flags;//0;
    frame.fb_id = (u16)pVideoBuffer->buffer_id;
    frame.real_fb_id = (u16)pVideoBuffer->real_buffer_id;
    AM_DEBUG_INFO("**IAV_IOC_RELEASE_FRAME, frame.fb_id %d, real_fb_id %d, pVideoBuffer->flags %d.\n", frame.fb_id, frame.real_fb_id, pVideoBuffer->flags);
    if (::ioctl(mIavFd, IAV_IOC_RELEASE_FRAME, &frame) < 0) {
        AM_PERROR("IAV_IOC_RELEASE_FRAME");
        AM_ERROR("IAV_IOC_RELEASE_FRAME Fail, pVideoBuffer->buffer_id %d.\n", pVideoBuffer->buffer_id);
    }
}

DSPHandler *DSPHandler::Create()
{
    DSPHandler *result = NULL;
#if (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
    result = new IOneDSPHandler();
#elif TARGET_USE_AMBARELLA_A5S_DSP
    result = new A5SDSPHandler();
#else
    AM_ERROR("board macro take no effect?.\n");
    AM_ASSERT(0);
#endif
    if (result && result->Construct() != ME_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CIAVVideoBufferPool* DSPHandler::CreateBufferPool(AM_UINT pic_width, AM_UINT pic_height, bool ext_edge, amba_decoding_accelerator_t* p_acc)
{
    if (mIavFd < 0) {
        AM_ERROR(" !!!try create IAV buffer pool when IAV fd is not opened.\n");
        return NULL;
    }

    AM_PRINTF(" create IAV buffer pool pic width %u, pic_height %u, ext_edge %d, p_acc->video_buffer_number %d.\n", pic_width, pic_height, ext_edge, p_acc->video_buffer_number);
    return CIAVVideoBufferPool::Create(mIavFd, pic_width, pic_height, ext_edge, p_acc);
}


#if TARGET_USE_AMBARELLA_A5S_DSP

//tmp
bool GetUdecState(AM_INT iavFd, AM_UINT* udec_state, AM_UINT* vout_state, AM_UINT* error_code)
{
    return false;
}

int getVoutPrams(AM_INT iavFd, AM_INT vout_id, SVoutConfig* pConfig)
{
    return 0;
}

//obsolete
AM_INT PauseUdec(AM_INT iavFd, AM_INT index)
{
    return 0;
}

//obsolete
AM_INT ResumeUdec(AM_INT iavFd, AM_INT index)
{
    return 0;
}

AM_INT StopUDEC(AM_INT iavFd, AM_INT udec_id, AM_UINT stop_flag)
{
    return 0;
}


AM_INT UdecStepPlay(AM_INT iavFd, AM_INT index, AM_INT cnt)
{
    return 0;
}

CA5SVideoBufferPool::~CA5SVideoBufferPool()
{
	CloseDevice();
}

AM_ERR CA5SVideoBufferPool::OpenDevice()
{
    CloseDevice();

    if (::ioctl(mIavFd, IAV_IOC_START_DECODE, 0) < 0) {
        AM_INFO("IAV_IOC_START_DECODE FAILS\n");
        return ME_ERROR;
    }
    AM_INFO("IAV_IOC_START_DECODE success.\n");

    iav_config_decoder_t config;
    config.flags = 0;
    config.decoder_type = IAV_SOFTWARE_DECODER;
    config.chroma_format = 0;
    config.num_frame_buffer = 0;
    config.pic_width = mFbWidth;
    config.pic_height = mFbHeight;
    config.fb_width = 0;	// out
    config.fb_height = 0;// out

    if (::ioctl(mIavFd, IAV_IOC_CONFIG_DECODER, &config) < 0)
    {
        AM_ERROR("IAV_IOC_CONFIG_DECODER FAILS [%d*%d]\n", mFbWidth, mFbHeight);
        return ME_ERROR;
    }

    AM_INFO("IAV_IOC_CONFIG_DECODER success.\n");
    AM_INFO("config.fb_width %d, config.fb_height %d, config.num_frame_buffer %d.\n", config.fb_width, config.fb_height, config.num_frame_buffer);
    mFbWidth = config.fb_width;
    mFbHeight = config.fb_height;
    mBufferNum = config.num_frame_buffer - mReservedBufferNum;
    mbDeviceOpen = true;
    return ME_OK;
}

void CA5SVideoBufferPool::CloseDevice()
{
    AM_INT ret = 0;
    AM_INFO("CloseDevice start.\n");
    if (mbDeviceOpen) {
        ret = ::ioctl(mIavFd, IAV_IOC_STOP_DECODE, 0);
        AM_ERROR("IAV_IOC_STOP_DECODE, ret = %d.\n", ret);
        mbDeviceOpen = false;
    }

    if ((ret = ::ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0)) < 0) {
        AM_ERROR("IAV_IOC_ENTER_IDLE FAILS, ret = %d.\n", ret);
        return;
    }

    //workaroud for encoding mode
    //init vin

    int source = 0;
    if (::ioctl(mIavFd, IAV_IOC_VIN_SET_CURRENT_SRC, &source) < 0) {
        AM_PERROR("IAV_IOC_VIN_SET_CURRENT_SRC");
        return ;
    }

    int mode = 0;
    if (::ioctl(mIavFd, IAV_IOC_VIN_SRC_SET_VIDEO_MODE, mode) < 0) {
        AM_PERROR("IAV_IOC_VIN_SRC_SET_VIDEO_MODE");
        return ;
    }

    { //set_vin_param,is used to set 3A data, only for sensor that output RGB raw data
        int vin_eshutter_time = 60;	// 1/60 sec
        int vin_agc_db = 6;	// 0dB

        u32 shutter_time_q9;
        s32 agc_db;
        shutter_time_q9 = 512000000/vin_eshutter_time;
        agc_db = vin_agc_db<<24;

        if (::ioctl(mIavFd, IAV_IOC_VIN_SRC_SET_SHUTTER_TIME, shutter_time_q9) < 0) {
        AM_PERROR("IAV_IOC_VIN_SRC_SET_SHUTTER_TIME");
        return ;
        }

        if (::ioctl(mIavFd, IAV_IOC_VIN_SRC_SET_AGC_DB, agc_db) < 0) {
        AM_PERROR("IAV_IOC_VIN_SRC_SET_AGC_DB");
        return ;
        }
    }

    if (::ioctl(mIavFd, IAV_IOC_ENABLE_PREVIEW, 0) < 0) {
        AM_ERROR("IAV_IOC_ENABLE_PREVIEW fail");
        return ;
    }

    if ((ret = ::ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0)) < 0) {
        AM_ERROR("IAV_IOC_ENTER_IDLE FAILS, ret = %d.\n", ret);
        return;
    }
    AM_INFO("CloseDevice done.\n");
    //LOGD("feiyu close device .............+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    return ;
}

//A5S related
static void _A5s_renderVideoBuffer(int iavFd, CVideoBuffer *pVideoBuffer, SRect* srcRect)
{
    AM_ASSERT(pVideoBuffer);
    AM_ASSERT(srcRect);
    iav_decoded_frame_t frame;
    int ret=0;

    frame.flags = 0;
    frame.fb_id = (u16)pVideoBuffer->tag;

    //source rect
    frame.pic_width = srcRect->w;
    frame.pic_height = srcRect->h;
    frame.lu_off_x = pVideoBuffer->picXoff + srcRect->x;
    frame.lu_off_y = pVideoBuffer->picYoff + srcRect->y;
    frame.ch_off_x = frame.lu_off_x  / 2;
    frame.ch_off_y = frame.lu_off_y / 2;

    //AM_PRINTF("frame.pic_width=%d,frame.pic_height=%d.\n",frame.pic_width,frame.pic_height);
    //AM_PRINTF("frame.lu_off_x=%d,frame.lu_off_y=%d.\n",frame.lu_off_x,frame.lu_off_y);
    //AM_PRINTF("frame.ch_off_x=%d,frame.ch_off_y=%d.\n",frame.ch_off_x,frame.ch_off_y);
    //AM_PRINTF("IAV_IOC_RENDER_FRAME, fb_id=%d.\n",frame.fb_id);
    if ((ret=ioctl(iavFd, IAV_IOC_RENDER_FRAME, &frame)) < 0) {
        AM_ERROR("IAV_IOC_RENDER_FRAME fail,fb_id=%d,ret=%d.\n",frame.fb_id,ret);
    } else {
        pVideoBuffer->flags &= ~(IAV_FRAME_FORCE_RELEASE);
    }
    //AM_PRINTF("IAV_IOC_RENDER_FRAME end.\n");
}


AM_ERR A5SDSPHandler::Construct()
{
    AM_ASSERT(mIavFd == -1);

    if ((mIavFd = ::open("/dev/iav", O_RDWR, 0)) < 0) {
        AM_ERROR("open /dev/iav fail %d.\n", mIavFd);
        return ME_IO_ERROR;
    }
    mfpRenderVideoBuffer = _A5s_renderVideoBuffer;
    mfpDecoderGetBuffer = NULL;
    mfpDecoderReleaseBuffer = NULL;
    return ME_OK;
}

AM_INT A5SDSPHandler::EnterUdecMode()
{
    //a5s not support udec mode
    return -1;
}


A5SDSPHandler::~A5SDSPHandler()
{
    if (mIavFd >= 0) {
        ::close(mIavFd);
    }
}

#endif

#if (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
CIOneVideoBufferPool::~CIOneVideoBufferPool()
{
	CloseDevice();
}

AM_ERR CIOneVideoBufferPool::OpenDevice()
{
    iav_frame_buffer_t frame;
    //pure sw decoding need allocate iav buffer pool
    //todo change UDEC_NULL
    AM_ASSERT(mpAcc);
    AM_PRINTF("mpAcc->decode_type %d.\n", mpAcc->decode_type);
    if (mpAcc->decode_type == UDEC_SW || mpAcc->decode_type == UDEC_RV40) {
        iav_fbp_config_t fbp_config;
        memset(&fbp_config,0,sizeof(fbp_config));
        fbp_config.decoder_id = mpAcc->decode_id;
        fbp_config.chroma_format = 1; //420
        fbp_config.tiled_mode = 0;
        //use the real pic size, chuchen,2012_3_6
        fbp_config.lu_width = mpAcc->amba_picture_real_width;
        fbp_config.lu_height = mpAcc->amba_picture_real_height;
        if(mbExtEdge){
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
        if (::ioctl(mpAcc->amba_iav_fd, IAV_IOC_UDEC_UPDATE_FB_POOL_CONFIG, &fbp_config) < 0) {
            AM_PERROR("IAV_IOC_UDEC_UPDATE_FB_POOL_CONFIG");
            return ME_ERROR;
        }

        if (mpAcc->decode_type == UDEC_RV40) {
            AM_ASSERT(mpAcc->amba_buffer_number < 5 && mpAcc->amba_buffer_number > 0);

            //rv40 need reserve buffer for ucode return mc result
            for (AM_INT i = 0; i < mpAcc->amba_buffer_number; i++) {
                memset(&frame, 0, sizeof(frame));
                frame.decoder_id = mpAcc->decode_id;
                frame.pic_struct = 3;
                AM_PRINTF("**IAV_IOC_REQUEST_FRAME(reserved), mpAcc->decode_id %d.\n", mpAcc->decode_id);
                if (::ioctl(mIavFd, IAV_IOC_REQUEST_FRAME, &frame) < 0) {
                    //release buffer from inherited::AllocBuffer
                    AM_PERROR("IAV_IOC_REQUEST_FRAME");
                    AM_ERROR("CIAVVideoBufferPool IAV_IOC_REQUEST_FRAME IAV buffer fail.\n");
                    return ME_ERROR;
                }
                AM_PRINTF("**IAV_IOC_REQUEST_FRAME(reserved) done, buffer id %d, real_fb_id %d, frame.buffer_pitch %d,frame.buffer_width %d mpAcc->decode_id %d.\n", frame.fb_id, frame.real_fb_id, frame.buffer_pitch, frame.buffer_width, mpAcc->decode_id);

                mpAcc->p_mcresult_buffer[i] = (AM_U8*)frame.lu_buf_addr;
                mpAcc->mcresult_buffer_size[i] = frame.buffer_pitch * frame.buffer_height;
                mpAcc->mcresult_buffer_id[i] = (AM_UINT)frame.fb_id;
                mpAcc->mcresult_buffer_real_id[i] = (AM_UINT)frame.real_fb_id;
                //update mFbWidth
                mFbWidth = frame.buffer_pitch;
            }
        }
        else {
            iav_decoded_frame_t frame_release;
            memset(&frame, 0, sizeof(frame));
            frame.decoder_id = mpAcc->decode_id;
            frame.pic_struct = 3;
            if (::ioctl(mpAcc->amba_iav_fd, IAV_IOC_REQUEST_FRAME, &frame) < 0) {
                AM_PERROR("IAV_IOC_REQUEST_FRAME");
                return ME_ERROR;
            }

            //update mFbWidth
            mFbWidth = frame.buffer_pitch;

            memset(&frame_release, 0, sizeof(frame_release));
            frame_release.flags = 0;
            frame_release.fb_id = (u16) frame.fb_id;
            frame_release.real_fb_id = (u16)frame.real_fb_id;
            if (::ioctl(mpAcc->amba_iav_fd, IAV_IOC_RELEASE_FRAME, &frame_release) < 0) {
                AM_PERROR("IAV_IOC_RELEASE_FRAME");
                return ME_ERROR;
            }
        }

        mBufferNum = /*fbp_config.num_frm_bufs*/20 - mReservedBufferNum;
        mpAcc->video_buffer_number = mBufferNum - mReservedBufferNum;
        AM_PRINTF("fbwidth %d, height %d, num %d.\n", mFbWidth, mFbHeight, mBufferNum);
        AM_PRINTF("mpAcc->amba_picture_width %d, mpAcc->amba_picture_height %d.\n", mpAcc->amba_picture_width, mpAcc->amba_picture_height);
        mbDeviceOpen = true;
    } else {
        AM_ERROR("!!!!**** problem here: mpAcc->video_buffer_number %d.\n", mpAcc->video_buffer_number);
        mpAcc->video_buffer_number = 10;
        //no need create iav buffer pool, use decoder's attached buffer pool
        mBufferNum = mpAcc->video_buffer_number - mReservedBufferNum;
    }

    return ME_OK;
}

void CIOneVideoBufferPool::CloseDevice()
{
    if (mbDeviceOpen) {
        mbDeviceOpen = false;
        //todo destroy buffer pool
        //::ioctl(mIavFd, IAV_IOC_DESTROY_FB_POOL, NULL);
    }
}

bool CIOneVideoBufferPool::AllocBuffer(CBuffer*& pBuffer, AM_UINT size)
{
    iav_frame_buffer_t frame;
    if (!inherited::inherited::AllocBuffer(pBuffer, size)) {
        AM_ERROR("CIOneVideoBufferPool inherited::AllocBuffer fail.\n");
        return false;
    }
    CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pBuffer;

    //hybird mpeg4, should not alloc buffer
    if (mpAcc->decode_type != UDEC_MP4S) {
        AM_ASSERT(mpAcc->decode_type == UDEC_SW || mpAcc->decode_type == UDEC_RV40);
        memset(&frame, 0, sizeof(frame));
        frame.decoder_id = mpAcc->decode_id;
        frame.pic_struct = 3;
        AM_DEBUG_INFO("**IAV_IOC_REQUEST_FRAME, mpAcc->decode_id %d.\n",  mpAcc->decode_id);
        if (::ioctl(mIavFd, IAV_IOC_REQUEST_FRAME, &frame) < 0) {
            //release buffer from inherited::AllocBuffer
            AM_PERROR("IAV_IOC_REQUEST_FRAME");
            pBuffer->Release();
            AM_ERROR("CIAVVideoBufferPool IAV_IOC_REQUEST_FRAME IAV buffer fail.\n");
            return false;
        }
        AM_DEBUG_INFO("**IAV_IOC_REQUEST_FRAME done, buffer id %d, real_fb_id %d, frame.buffer_pitch %d,frame.buffer_width %d mpAcc->decode_id %d.\n", frame.fb_id, frame.real_fb_id, frame.buffer_pitch, frame.buffer_width, mpAcc->decode_id);

        AM_DEBUG_INFO("**frame.lu_buf_addr %p, rframe.ch_buf_addr %p.\n", frame.lu_buf_addr, frame.ch_buf_addr);
        AM_DEBUG_INFO("diff %d, w %d, h %d, w*h %d.\n", (AM_UINT)frame.ch_buf_addr - (AM_UINT)frame.lu_buf_addr, pVideoBuffer->picWidth, pVideoBuffer->picHeight, pVideoBuffer->picWidth*pVideoBuffer->picHeight);

        pVideoBuffer->pLumaAddr = (AM_U8*)frame.lu_buf_addr;
        pVideoBuffer->pChromaAddr = (AM_U8*)frame.ch_buf_addr;
        pVideoBuffer->buffer_id= (AM_INT)frame.fb_id;
        pVideoBuffer->real_buffer_id= (AM_INT)frame.real_fb_id;
        AM_ASSERT(mFbWidth == frame.buffer_pitch);
        //AM_ASSERT(mFbHeight == frame.buffer_height);
    }

    pBuffer->SetType(CBuffer::DATA);
    pBuffer->SetDataSize(0);
    pBuffer->SetDataPtr(NULL);

    pVideoBuffer->picWidth = mPicWidth;
    pVideoBuffer->picHeight = mPicHeight;
    pVideoBuffer->fbWidth = mFbWidth;
    pVideoBuffer->fbHeight = mFbHeight;
    //AM_INFO("CIAVVideoBufferPool mPicWidth %d, mPicHeight %d, mFbWidth %d, mFbHeight %d, ext %d.\n", mPicWidth, mPicHeight, mFbWidth, mFbHeight, mbExtEdge);
    if (mbExtEdge) {
        pVideoBuffer->picXoff = 16;
        pVideoBuffer->picYoff = 16;
    } else {
        pVideoBuffer->picXoff = 0;
        pVideoBuffer->picYoff = 0;
    }

    pVideoBuffer->flags = 0;//IAV_FRAME_NO_RELEASE;

    return true;
}

//IOne related
//#define CHK_TRASH
#if 0
static void _I1_deleteHybirdUDEC(void* p)
{
    amba_decoding_accelerator_t* pAcc = (amba_decoding_accelerator_t*) p;
    if (!pAcc || pAcc->amba_iav_fd < 0) {
        AM_ERROR("Already closed?.\n");
        return;
    }
    AM_ASSERT(pAcc);
    AM_ASSERT(pAcc->amba_iav_fd >= 0);

    //::close(pAcc->amba_iav_fd);
    pAcc->amba_iav_fd = -1;
    free(pAcc);
    return;
}
#endif
static int _I1_hybirdUDECDecode(void* p_context, void* p_param)
{
    int ret;
    amba_decoding_accelerator_t* pac = (amba_decoding_accelerator_t*)p_context;
    AM_DEBUG_INFO("**_I1_hybirdUDECDecode, iav_fd %d.\n", pac->amba_iav_fd);

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

#ifdef CHK_TRASH
    udec_decode_t* udec = (udec_decode_t*)p_param;
    unsigned int dec_cnt=pac->dec_cnt%pac->amba_buffer_number;
    unsigned char* dec=udec->uu.mpeg4s.vop_coef_daddr;
    unsigned char* dec_need=pac->p_amba_idct_ringbuffer+pac->amba_idct_buffer_size*dec_cnt;
    if(dec != dec_need)
        AM_PRINTF("[trash_dec], dec=%p, need=%p, cnt=%d.\n", dec, dec_need, dec_cnt);
    pac->dec_cnt++;
    if(pac->dec_cnt > pac->req_cnt)
        AM_PRINTF("[trash_dec_cnt], deccnt=%d, reqcnt=%d.\n", pac->dec_cnt, pac->req_cnt);
#endif

    //send cmd
    if((ret = ::ioctl(pac->amba_iav_fd, IAV_IOC_UDEC_DECODE, p_param))<0) {
        AM_PERROR("IAV_IOC_UDEC_DECODE");
        AM_PRINTF("-----------------------udec stopped, rtn %d in _I1_hybirdUDECDecode().\n",ret);
        return ret;
    }
    AM_DEBUG_INFO("**_I1_hybirdUDECDecode done, ret %d.\n", ret);
    AM_DEBUG_INFO("STOP_ISSUE:  sent_decode_pic_cnt=%d.\n", ++pac->sent_decode_pic_cnt);

    if (pac->decode_type == UDEC_RV40) {
        //wait msg done
       iav_frame_buffer_t frame;
        ::memset(&frame, 0, sizeof(frame));
        frame.flags = IAV_FRAME_NEED_SYNC;
        frame.decoder_id = pac->decode_id;

        AM_DEBUG_INFO("**hybirdUDECGetframe, frame.fb_id %d.\n", frame.fb_id);
        if ((ret = ::ioctl(pac->amba_iav_fd, IAV_IOC_GET_DECODED_FRAME, &frame)) < 0) {
            AM_PERROR("IAV_IOC_GET_DECODED_FRAME");
            AM_PRINTF("-----------------------udec stopped, rtn %d in _I1_hybirdUDECDecode(UDEC_RV40).\n",ret);
            return ret;
        }
        AM_DEBUG_INFO("**hybirdUDECGetframe get done, frame.fb_id %d.\n", frame.fb_id);
    }

    return 0;
}
/*
static void _I1_hybirdUDECDecEOSDummyFrame(void* p_context, void* p_param1, void* p_param2)
{
    int ret;
    //amba_decoding_accelerator_t* pac = (amba_decoding_accelerator_t*)p_context;
    udec_decode_t eos_udec={0};
    eos_udec.uu.mpeg4s.vop_coef_daddr = (unsigned char*)p_param1;//pac->p_amba_idct_ringbuffer;
    eos_udec.uu.mpeg4s.mv_coef_daddr = (unsigned char*)p_param2;//pac->p_amba_mv_ringbuffer;
    eos_udec.uu.mpeg4s.end_of_sequence = 1;
    _I1_hybirdUDECDecode(p_context, &eos_udec);
}
*/
static int _I1_hybirdUDECGetDecodedFrame(void* p_context, void* p_param)
{
    int ret;
    amba_decoding_accelerator_t* pac = (amba_decoding_accelerator_t*)p_context;

    //wait msg done
    iav_wait_decoder_t wait;
    ::memset(&wait, 0x0, sizeof(wait));
    wait.decoder_id = pac->decode_id;
    wait.flags = IAV_WAIT_OUTPIC;
    AM_DEBUG_INFO("**hybirdUDECGetframe start, iavfd %d.\n", pac->amba_iav_fd);
    if ((ret = ioctl(pac->amba_iav_fd, IAV_IOC_WAIT_DECODER, &wait)) < 0) {
        AM_PERROR("IAV_IOC_WAIT_DECODER");
        AM_PRINTF("-----------------------udec stopped, rtn %d in _I1_hybirdUDECGetDecodedFrame().\n",ret);
        return ret;
    }
    AM_ASSERT(wait.flags == IAV_WAIT_OUTPIC);
    AM_DEBUG_INFO("**hybirdUDECGetframe wait done.\n");
    iav_frame_buffer_t frame;
    ::memset(&frame, 0, sizeof(frame));
    frame.flags = 0; //output_flag ? IAV_FRAME_NEED_ADDR : 0;
    frame.decoder_id = pac->decode_id;

    if ((ret = ::ioctl(pac->amba_iav_fd, IAV_IOC_GET_DECODED_FRAME, &frame)) < 0) {
        AM_PERROR("IAV_IOC_GET_DECODED_FRAME");
        AM_PRINTF("-----------------------udec stopped, rtn %d in _I1_hybirdUDECGetDecodedFrame().\n",ret);
        return ret;
    }
    AM_DEBUG_INFO("**hybirdUDECGetframe get done, frame.fb_id %d.\n", frame.fb_id);
    AM_DEBUG_INFO("STOP_ISSUE:  got_decoded_pic_cnt=%d.\n", ++pac->got_decoded_pic_cnt);
    AVFrame* pic = (AVFrame*)p_param;
    CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pic->opaque;
    if(NULL==pVideoBuffer)
    {
        AM_WARNING("opaque warning!! %s,%d\n",__FILE__,__LINE__);
        return -EPERM;
    }
    pic->user_buffer_id = frame.fb_id;
    //pic->fbp_id = frame.fbp_id;
    pVideoBuffer->buffer_id = frame.fb_id;
    //pVideoBuffer->fbp_id = frame.fbp_id;
    pic->is_eos = (IAV_INVALID_FB_ID==frame.fb_id && 1==frame.eos_flag);

    //render immediately
    //::ioctl(pac->amba_iav_fd, IAV_IOC_RENDER_FRAME, &frame);

    return 0;
}

static int _I1_hybirdUDECRequestInputbuffer(void* p_context, int num_of_frame, unsigned char* start)
{
    int ret;
    amba_decoding_accelerator_t* pac = (amba_decoding_accelerator_t*)p_context;

    //wait msg done
    iav_wait_decoder_t wait;
    ::memset(&wait, 0x0, sizeof(wait));
    wait.decoder_id = pac->decode_id;
    wait.flags = IAV_WAIT_BITS_FIFO;
    wait.emptiness.room = pac->amba_idct_buffer_size+128;//more request 128 bytes avoid data trash
    wait.emptiness.start_addr = start;
    AM_DEBUG_INFO("**_I1_hybirdUDECRequestInputbuffer start, wait.emptiness.room %d, %p.\n", wait.emptiness.room, start);
    if ((ret = ioctl(pac->amba_iav_fd, IAV_IOC_WAIT_DECODER, &wait)) < 0) {
        AM_PERROR("IAV_IOC_WAIT_DECODER");
        AM_PRINTF("-----------------------udec stopped, rtn %d in _I1_hybirdUDECRequestInputbuffer().\n",ret);
        return ret;
    }
    AM_DEBUG_INFO("**_I1_hybirdUDECRequestInputbuffer, ret %d.\n", ret);
    AM_ASSERT(wait.flags == IAV_WAIT_BITS_FIFO);

#ifdef CHK_TRASH
    unsigned int req_cnt=pac->req_cnt%pac->amba_buffer_number;
    unsigned char* start_need=pac->p_amba_idct_ringbuffer+pac->amba_idct_buffer_size*req_cnt;
    if(start != start_need)
        AM_PRINTF("[trash_req], req=%p, need=%p, cnt=%d.\n", start, start_need,req_cnt);
    pac->req_cnt++;
    if(pac->dec_cnt > pac->req_cnt)
        AM_PRINTF("[trash_req_cnt], deccnt=%d, reqcnt=%d.\n", pac->dec_cnt, pac->req_cnt);
#endif

    return 0;
}

static int _I1_getBuffer(AVCodecContext *s, AVFrame *pic)
{
    CBuffer *pBuffer=NULL;
    pic->opaque=NULL;//for bug#2308 case 2: sw pipeline decoder, quick seek at file beginning, Segmentation fault issue
    IBufferPool *pBufferPool = (IBufferPool*)s->opaque;
    if(NULL==pBufferPool)
    {
        AM_WARNING("opaque warning!! %s,%d\n",__FILE__,__LINE__);
        return -EPERM;
    }

    if (!pBufferPool->AllocBuffer(pBuffer, 0)) {
        //AM_ASSERT(0);
        //return -1;
        AM_PRINTF("-----------------------udec stopped, rtn -EPERM in _I1_getBuffer().\n");
        return -EPERM;
    }
    pBuffer->AddRef();

    CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pBuffer;
    //pVideoBuffer->picXoff = 0;
    //pVideoBuffer->picYoff = 0;

    //update picture width & height, resolution maybe changed during playback
    pVideoBuffer->picWidth = s->width;
    pVideoBuffer->picHeight = s->height;

    //pts
    pBuffer->SetPTS((AM_U64)s->reordered_opaque);

    //only nv12 format comes here
    if (pVideoBuffer->picXoff > 0) {
        pic->data[0] = (uint8_t*)pVideoBuffer->pLumaAddr +
            pVideoBuffer->picYoff * pVideoBuffer->fbWidth +
            pVideoBuffer->picXoff;
        pic->linesize[0] = pVideoBuffer->fbWidth;

        pic->data[1] = (uint8_t*)pVideoBuffer->pChromaAddr +
            (pVideoBuffer->picYoff / 2) * pVideoBuffer->fbWidth +
            pVideoBuffer->picXoff;
        pic->linesize[1] = pVideoBuffer->fbWidth;

        pic->data[2] = pic->data[1] + 1;
        pic->linesize[2] = pVideoBuffer->fbWidth;
    } else {
        AM_ASSERT(!pVideoBuffer->picYoff);
        pic->data[0] = (uint8_t*)pVideoBuffer->pLumaAddr;
        pic->linesize[0] = pVideoBuffer->fbWidth;

        pic->data[1] = (uint8_t*)pVideoBuffer->pChromaAddr;
        pic->linesize[1] = pVideoBuffer->fbWidth;

        pic->data[2] = pic->data[1] + 1;
        pic->linesize[2] = pVideoBuffer->fbWidth;
    }
    pic->user_buffer_id = pVideoBuffer->buffer_id;
    pic->age = 256;
    pic->type = FF_BUFFER_TYPE_USER;
    pic->opaque = (void*)pBuffer;
    pic->real_fb_id = pVideoBuffer->real_buffer_id;
    return 0;

}

static void _I1_releaseBuffer(AVCodecContext *s, AVFrame *pic)
{
    CBuffer *pBuffer = (CBuffer*)pic->opaque;
    if(NULL==pBuffer)//for bug#2308 case 2: sw pipeline decoder, quick seek at file beginning, Segmentation fault issue
    {
        AM_WARNING("opaque warning!! %s,%d\n",__FILE__,__LINE__);
        return;
    }
    pBuffer->Release();
    if (pic->type == DFlushing_frame) {
        //flushing, CBuffer will not send to video_renderer for render/release, so release here
        pBuffer->Release();
    }

    pic->data[0] = NULL;
    pic->type = 0;
}

#define fake_addr 0x12345678
#define fake_stride 256

static int _Is_filter_buffer_needed(AVCodecContext *s)
{
    amba_decoding_accelerator_t* aac=(amba_decoding_accelerator_t*)s->p_extern_accelerator;
    if(!aac)//for safety here, perhaps sw decoding without aac
        return 0;
    if(aac->does_render_by_filter)
        return 1;

    return 0;
}

static int _I1_fake_getBuffer(AVCodecContext *s, AVFrame *pic)
{
    CBuffer *pBuffer=NULL;
    pic->opaque=NULL;
    if(_Is_filter_buffer_needed(s)) {
        IBufferPool *pBufferPool = (IBufferPool*)s->opaque;
        if(NULL==pBufferPool)
        {
            AM_WARNING("opaque warning!! %s,%d\n",__FILE__,__LINE__);
            return -EPERM;
        }
        if (!pBufferPool->AllocBuffer(pBuffer, 0)) {
            //AM_ASSERT(0);
            //return -1;
            AM_PRINTF("-----------------------udec stopped, rtn -EPERM in _I1_fake_getBuffer(ppmod=1).\n");
            return -EPERM;
        }
        pBuffer->AddRef();
    }

    pic->data[0] = pic->data[1] = pic->data[2] = (unsigned char*)fake_addr;
    pic->linesize[0] = pic->linesize[1] = pic->linesize[2] = fake_stride;

    pic->age = 256;
    pic->type = FF_BUFFER_TYPE_USER;
    pic->opaque = (void*)pBuffer;
    return 0;

}

static void _I1_fake_releaseBuffer(AVCodecContext *s, AVFrame *pic)
{
   if(_Is_filter_buffer_needed(s)) {
        CBuffer *pBuffer = (CBuffer*)pic->opaque;
        if(NULL==pBuffer)
        {
            AM_WARNING("opaque warning!! %s,%d\n",__FILE__,__LINE__);
            return;
        }
        pBuffer->Release();
        if (pic->type == DFlushing_frame) {
            //flushing, CBuffer will not send to video_renderer for render/release, so release here
            pBuffer->Release();
        }
    }

    pic->data[0] = NULL;
    pic->type = 0;
}

static void _I1_renderVideoBuffer_mpeg4hybird(int iavFd, CVideoBuffer *pVideoBuffer, SRect* srcRect)
{
    AM_ASSERT(pVideoBuffer);
    AM_ASSERT(srcRect);
    iav_frame_buffer_t frame;
    int ret=0;
    memset(&frame, 0, sizeof(frame));
    frame.flags = IAV_FRAME_NO_RELEASE;//only display
    frame.fb_id = (u16)pVideoBuffer->buffer_id;
    frame.decoder_id = 0;//todo
    //frame.fbp_id = (u8)pVideoBuffer->fbp_id;

    //source rect
    frame.pic_width = srcRect->w;
    frame.pic_height = srcRect->h;
    frame.lu_off_x = pVideoBuffer->picXoff + srcRect->x;
    frame.lu_off_y = pVideoBuffer->picYoff + srcRect->y;
    frame.ch_off_x = frame.lu_off_x  / 2;
    frame.ch_off_y = frame.lu_off_y / 2;

    //need send pts to ucode
    am_pts_t pts = pVideoBuffer->mPTS;
    frame.pts = (u32)(pts & 0xffffffff);
    frame.pts_high = (u32)(pts>>32);

    //AM_PRINTF("frame.pic_width=%d,frame.pic_height=%d.\n",frame.pic_width,frame.pic_height);
    //AM_PRINTF("frame.lu_off_x=%d,frame.lu_off_y=%d.\n",frame.lu_off_x,frame.lu_off_y);
    //AM_PRINTF("frame.ch_off_x=%d,frame.ch_off_y=%d.\n",frame.ch_off_x,frame.ch_off_y);
    AM_DEBUG_INFO("IAV_IOC_RENDER_FRAME, fb_id=%d.\n",frame.fb_id);
    if ((ret=::ioctl(iavFd, IAV_IOC_RENDER_FRAME, &frame)) < 0) {
        AM_PERROR("IAV_IOC_RENDER_FRAME.\n");
        AM_ERROR("IAV_IOC_RENDER_FRAME fail,fb_id=%d,ret=%d, pts high %u, low %u.\n",frame.fb_id, ret, frame.pts_high, frame.pts);
    }

    pVideoBuffer->flags = 0;
    AM_DEBUG_INFO("IAV_IOC_RENDER_FRAME end.\n");
}

static void _I1_renderVideoBuffer(int iavFd, CVideoBuffer *pVideoBuffer, SRect* srcRect)
{
    AM_ASSERT(pVideoBuffer);
    AM_ASSERT(srcRect);
    iav_frame_buffer_t frame;
    int ret=0;
    memset(&frame, 0, sizeof(frame));
    frame.flags = IAV_FRAME_NO_RELEASE;//only display
    frame.fb_id = (u16)pVideoBuffer->buffer_id;
    frame.decoder_id = 0;//todo
    frame.real_fb_id = (u16)pVideoBuffer->real_buffer_id;

    //source rect
    frame.pic_width = srcRect->w;
    frame.pic_height = srcRect->h;
    frame.lu_off_x = pVideoBuffer->picXoff + srcRect->x;
    frame.lu_off_y = pVideoBuffer->picYoff + srcRect->y;
    frame.ch_off_x = frame.lu_off_x  / 2;
    frame.ch_off_y = frame.lu_off_y / 2;

    //need send pts to ucode
    am_pts_t pts = pVideoBuffer->mPTS;
    frame.pts = (u32)(pts & 0xffffffff);
    frame.pts_high = (u32)(pts>>32);

    //AM_PRINTF("frame.pic_width=%d,frame.pic_height=%d.\n",frame.pic_width,frame.pic_height);
    //AM_PRINTF("frame.lu_off_x=%d,frame.lu_off_y=%d.\n",frame.lu_off_x,frame.lu_off_y);
    //AM_PRINTF("frame.ch_off_x=%d,frame.ch_off_y=%d.\n",frame.ch_off_x,frame.ch_off_y);
    AM_DEBUG_INFO("IAV_IOC_POSTP_FRAME, fb_id=%d, real_fb_id %d frame.flags %d, pts high %u, low %u.\n",frame.fb_id, frame.real_fb_id, frame.flags, frame.pts_high, frame.pts);
    if ((ret=::ioctl(iavFd, IAV_IOC_POSTP_FRAME, &frame)) < 0) {
        AM_PERROR("IAV_IOC_POSTP_FRAME.\n");
        AM_ERROR("IAV_IOC_POSTP_FRAME fail,fb_id=%d,ret=%d.\n",frame.fb_id,ret);
    }

    pVideoBuffer->flags = 0;
    AM_DEBUG_INFO("IAV_IOC_POSTP_FRAME end.\n");
}

bool GetUdecState(AM_INT iavFd, AM_UINT* udec_state, AM_UINT* vout_state, AM_UINT* error_code)
{
    AM_INT ret = 0;
    iav_udec_state_t state;
    memset(&state, 0, sizeof(state));
    state.decoder_id = 0;//hard code here
    state.flags = 0;

    ret = ioctl(iavFd, IAV_IOC_GET_UDEC_STATE, &state);
    //AM_ASSERT(ret == 0);
    if (ret) {
        perror("IAV_IOC_GET_UDEC_STATE");
        AM_ERROR("IAV_IOC_GET_UDEC_STATE %d.\n", ret);
    }
    //AM_PRINTF("**IAV_IOC_GET_UDEC_STATE %d.\n", ret);
    *udec_state = state.udec_state;
    *vout_state = state.vout_state;
    *error_code = state.error_code;

#ifdef AM_DEBUG
    //for error code record
    RecordUdecErrorCode((AM_UINT)state.error_code);
#endif

    return (state.udec_state == IAV_UDEC_STATE_ERROR);
}

int getVoutPrams(AM_INT iavFd, AM_INT vout_id, SVoutConfig* pConfig)
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

    if(sink_info.state != AMBA_VOUT_SINK_STATE_RUNNING){
        AM_WARNING("the state of vout %d is not RUNNING, its state is %d\n", vout_id, sink_info.state);
        return -1;
    }

    pConfig->failed = 0;
    pConfig->enable = sink_info.sink_mode.video_en;
    AM_ASSERT(1 == pConfig->enable);

    if (sink_info.sink_mode.fb_id < 0) {
        //osd disabled
        pConfig->osd_disable = 1;
    } else {
        //osd enabled
        pConfig->osd_disable = 0;
    }

    pConfig->enable = 1;
    pConfig->vout_id = vout_id;
    pConfig->sink_id = sink_id;
    pConfig->width = sink_info.sink_mode.video_size.vout_width;
    pConfig->height = sink_info.sink_mode.video_size.vout_height;
    pConfig->size_x = sink_info.sink_mode.video_size.video_width;
    pConfig->size_y = sink_info.sink_mode.video_size.video_height;
    pConfig->pos_x = sink_info.sink_mode.video_offset.offset_x;
    pConfig->pos_y = sink_info.sink_mode.video_offset.offset_y;
    pConfig->vout_mode = (sink_info.sink_mode.format == AMBA_VIDEO_FORMAT_INTERLACE) ? 1 : 0;
    AM_PRINTF("vout(%d)'s original position/size, dimention: pos(%d,%d), size(%d,%d), dimention(%d,%d).\n", vout_id, pConfig->pos_x, pConfig->pos_y, pConfig->size_x, pConfig->size_y, pConfig->width, pConfig->height);

    pConfig->flip = (AM_INT)sink_info.sink_mode.video_flip;
    pConfig->rotate = (AM_INT)sink_info.sink_mode.video_rotate;
    AM_PRINTF("vout(%d)'s original flip and rotate state: %d %d.\n", vout_id, pConfig->flip, pConfig->rotate);

    return 0;
}

int getVoutParamsEx(AM_INT iavFd, AM_INT vout_id, SVoutConfig* pConfig, amba_video_sink_mode* p_sink_mode)
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

    if(sink_info.state != AMBA_VOUT_SINK_STATE_RUNNING){
        AM_WARNING("the state of vout %d is not RUNNING, its state is %d\n", vout_id, sink_info.state);
        return -1;
    }

    pConfig->failed = 0;
    pConfig->enable = sink_info.sink_mode.video_en;
    AM_ASSERT(1 == pConfig->enable);

    if (sink_info.sink_mode.fb_id < 0) {
        //osd disabled
        pConfig->osd_disable = 1;
    } else {
        //osd enabled
        pConfig->osd_disable = 0;
    }

    pConfig->enable = 1;
    pConfig->vout_id = vout_id;
    pConfig->sink_id = sink_id;
    pConfig->width = sink_info.sink_mode.video_size.vout_width;
    pConfig->height = sink_info.sink_mode.video_size.vout_height;
    pConfig->size_x = sink_info.sink_mode.video_size.video_width;
    pConfig->size_y = sink_info.sink_mode.video_size.video_height;
    pConfig->pos_x = sink_info.sink_mode.video_offset.offset_x;
    pConfig->pos_y = sink_info.sink_mode.video_offset.offset_y;
    pConfig->vout_mode = (sink_info.sink_mode.format == AMBA_VIDEO_FORMAT_INTERLACE) ? 1 : 0;
    AM_PRINTF("vout(%d)'s original position/size, dimention: pos(%d,%d), size(%d,%d), dimention(%d,%d).\n", vout_id, pConfig->pos_x, pConfig->pos_y, pConfig->size_x, pConfig->size_y, pConfig->width, pConfig->height);

    pConfig->flip = (AM_INT)sink_info.sink_mode.video_flip;
    pConfig->rotate = (AM_INT)sink_info.sink_mode.video_rotate;
    AM_ASSERT(p_sink_mode);
    *p_sink_mode= sink_info.sink_mode;
    AM_PRINTF("getVoutParamsEx, vout(%d)'s original flip and rotate state: %d %d. export sink_mode to %p\n", vout_id, pConfig->flip, pConfig->rotate, p_sink_mode);

    return 0;
}

//obsolete
AM_INT PauseUdec(AM_INT iavFd, AM_INT index)
{
    iav_udec_trickplay_t trickplay;
    AM_INT ret;

//#ifdef AM_DEBUG
    AM_UINT udec_state, vout_state, error_code;
    GetUdecState(iavFd, &udec_state, &vout_state, &error_code);
    AM_ASSERT(vout_state == IAV_VOUT_STATE_RUN);
    AM_ASSERT(udec_state == IAV_UDEC_STATE_RUN);
    if (vout_state !=IAV_VOUT_STATE_RUN || udec_state != IAV_UDEC_STATE_RUN) {
        AM_ERROR("**pause: but udec is not IAV_VOUT_STATE_RUN(%d), udec_state %d, return.\n", vout_state, udec_state);
        return 1;
    }
//#endif

    trickplay.decoder_id = index;
    trickplay.mode = 0; //pause
    AM_ERROR("**pause: start IAV_IOC_UDEC_TRICKPLAY.\n");
    ret = ioctl(iavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
    if (!ret) {
        return 0;
    } else {
        AM_ERROR("**IAV_IOC_UDEC_TRICKPLAY fail, ret %d, retry.\n", ret);
        usleep(20);
        ret = ioctl(iavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
        if(!ret) {
            return 0;
        } else {
            AM_ERROR("**IAV_IOC_UDEC_TRICKPLAY fail, ret %d.\n", ret);
            return -1;
        }
    }
}

//obsolete in the future
AM_INT ResumeUdec(AM_INT iavFd, AM_INT index)
{
#if 0
    return 0;
    iav_udec_trickplay_t trickplay;
    AM_INT ret;

//#ifdef AM_DEBUG
    AM_UINT udec_state, vout_state, error_code;
    GetUdecState(iavFd, &udec_state, &vout_state, &error_code);
    AM_ASSERT(vout_state == IAV_VOUT_STATE_PAUSE);
    AM_ASSERT(udec_state == IAV_UDEC_STATE_RUN);
    if (vout_state !=IAV_VOUT_STATE_PAUSE || udec_state != IAV_UDEC_STATE_RUN) {
        AM_ERROR("**pause: but udec is not IAV_VOUT_STATE_PAUSE(%d), udec %d, return.\n", vout_state, udec_state);
        return 1;
    }
//#endif

    trickplay.decoder_id = index;
    trickplay.mode = 1; //resume
    AM_ERROR("**resume: start IAV_IOC_UDEC_TRICKPLAY.\n");
    ret = ioctl(iavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
    if (!ret) {
        return 0;
    } else {
        AM_ERROR("**IAV_IOC_UDEC_TRICKPLAY fail, ret %d, retry.\n", ret);
        usleep(20);
        ret = ioctl(iavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
        if(!ret) {
            return 0;
        } else {
            AM_ERROR("**IAV_IOC_UDEC_TRICKPLAY fail, ret %d.\n", ret);
            return -1;
        }
    }
#else//fix bug#2308 case 1: hy pipeline decoder, pause+quit, hang issue
    iav_udec_trickplay_t trickplay;
    AM_INT ret;

    AM_UINT udec_state, vout_state, error_code;
    GetUdecState(iavFd, &udec_state, &vout_state, &error_code);
    AM_ASSERT(vout_state == IAV_VOUT_STATE_PAUSE);
    AM_ASSERT(udec_state == IAV_UDEC_STATE_RUN);
    if (vout_state !=IAV_VOUT_STATE_PAUSE || udec_state != IAV_UDEC_STATE_RUN) {
        AM_ERROR("**pause: but udec is not IAV_VOUT_STATE_PAUSE(%d), udec %d, return.\n", vout_state, udec_state);
        return 1;
    }

    trickplay.decoder_id = index;
    trickplay.mode = 1; //resume
    AM_INFO("[VD flow cmd]: ResumeUdec start.\n");
    ret = ioctl(iavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
    if (!ret) {
        AM_INFO("[VD flow cmd]: ResumeUdec done.\n");
        return 0;
    } else {
        AM_ERROR("[VD flow cmd]: ResumeUdec fail, ret %d.\n", ret);
        return -1;
    }
#endif
}

AM_INT UdecStepPlay(AM_INT iavFd, AM_INT index, AM_INT cnt)
{
    iav_udec_trickplay_t trickplay;
    AM_INT ret;

    trickplay.decoder_id = index;
    trickplay.mode = 2; //step
    AM_PRINTF("**resume: start IAV_IOC_UDEC_TRICKPLAY.\n");
    ret = ioctl(iavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
    if (!ret) {
        return 0;
    } else {
        AM_ERROR("**IAV_IOC_UDEC_TRICKPLAY fail, ret %d, retry.\n", ret);
        usleep(20);
        ret = ioctl(iavFd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
        if(!ret) {
            return 0;
        } else {
            AM_ERROR("**IAV_IOC_UDEC_TRICKPLAY fail, ret %d.\n", ret);
            return -1;
        }
    }
}

AM_INT getVoutParams(AM_INT iavFd, AM_INT vout_id, DSPVoutConfig* pConfig)
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

    if(sink_info.state != AMBA_VOUT_SINK_STATE_RUNNING){
        AM_WARNING("the state of vout %d is not RUNNING, its state is %d\n", vout_id, sink_info.state);
        return -1;
    }

    pConfig->failed = 0;

    pConfig->enable = sink_info.sink_mode.video_en;
    AM_ASSERT(1 == pConfig->enable);

    if (sink_info.sink_mode.fb_id < 0) {
        //osd disabled
        pConfig->osd_disable = 1;
    } else {
        //osd enabled
        pConfig->osd_disable = 0;
    }

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

AM_INT StopUDEC(AM_INT iavFd, AM_INT udec_id, AM_UINT stop_flag)
{
    AM_INT ret = 0;
    AM_UINT stop_code = (((stop_flag&0xff)<<24)|udec_id);
    AM_ASSERT(0 == (stop_flag & 0xffffff00));

    AM_WARNING("[flow cmd]: IAV_IOC_UDEC_STOP start, 0x%x.\n",stop_code);
    if ((ret = ::ioctl(iavFd, IAV_IOC_UDEC_STOP, stop_code)) < 0) {
        AM_PRINTF("IAV_IOC_UDEC_STOP error %d.\n", ret);
        return ret;
    }
    AM_WARNING("[flow cmd]: IAV_IOC_UDEC_STOP done.\n");
    return 0;
}

static void map_dsp(int fd_iav)
{
    iav_mmap_info_t info;

    if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
        perror("IAV_IOC_MAP_DSP");
    }
}

static void unmap_dsp(int fd_iav)
{
    if (ioctl(fd_iav, IAV_IOC_UNMAP_DSP) < 0) {
        perror("IAV_IOC_UNMAP_DSP");
    }
}

//for rv40 decoding protocol
unsigned int* get_sync_counter_pointer(int fd_iav)
{
    iav_udec_map_t map;
    map.rv_sync_counter = 0;
    ioctl(fd_iav, IAV_IOC_UDEC_MAP, &map);
    AM_PRINTF("pointer is %p.\n", map.rv_sync_counter);
    return map.rv_sync_counter;
}

static void release_sync_counter_pointer(int fd_iav)
{
    ioctl(fd_iav, IAV_IOC_UDEC_UNMAP);
}

AM_INT IOneDSPHandler::StartUdec(amba_decoding_accelerator_t* pAcc)
{
    AM_ASSERT(pAcc);
    if (!pAcc) {
        AM_ERROR("IOneDSPHandler::StartUdec NULL pAcc.\n");
        return -2;
    }
    iav_udec_decode_t info;
    memset(&info, 0, sizeof(info));
    //only one instance now
    AM_ASSERT(pAcc->decode_id == 0);
    AM_ASSERT(pAcc->decode_type == UDEC_SW || pAcc->decode_type == UDEC_RV40);
    if (pAcc->decode_type == UDEC_SW) {
        info.udec_type = UDEC_SW;
        info.decoder_id = pAcc->decode_id;
        info.num_pics = 0;

        if (ioctl(mIavFd, IAV_IOC_UDEC_DECODE, &info) < 0) {
            perror("IAV_IOC_UDEC_DECODE");
            return -1;
        }

        return 0;
    } else if (pAcc->decode_type == UDEC_RV40) {
        info.udec_type = UDEC_RV40;
        info.decoder_id = pAcc->decode_id;
        info.num_pics = 0;
        info.u.rv40.pic_coding_type = 1;//I picture, fake for start udec.
        info.u.rv40.residual_fifo_start = pAcc->p_amba_idct_ringbuffer;
        info.u.rv40.residual_fifo_end = pAcc->p_amba_idct_ringbuffer + pAcc->amba_idct_buffer_size;
        info.u.rv40.mv_fifo_start = pAcc->p_amba_mv_ringbuffer;
        info.u.rv40.mv_fifo_end = pAcc->p_amba_mv_ringbuffer + pAcc->amba_mv_buffer_size;
        info.u.rv40.pic_width = pAcc->amba_picture_width;
        info.u.rv40.pic_height = pAcc->amba_picture_height;

        AM_PRINTF("** start UDEC_RV40, IAV_IOC_UDEC_DECODE.\n");
        if (ioctl(mIavFd, IAV_IOC_UDEC_DECODE, &info) < 0) {
            perror("IAV_IOC_UDEC_DECODE");
            return -1;
        }
        AM_PRINTF("** UDEC_RV40, IAV_IOC_UDEC_DECODE, done.\n");
        return 0;
    }
    return -3;
}


AM_ERR IOneDSPHandler::Construct()
{
    AM_ASSERT(mIavFd == -1);

    if ((mIavFd = ::open("/dev/iav", O_RDWR, 0)) < 0) {
        AM_ERROR("open /dev/iav fail %d.\n", mIavFd);
        return ME_IO_ERROR;
    }
    mfpRenderVideoBuffer = _I1_renderVideoBuffer;
    mfpDecoderGetBuffer = _I1_getBuffer;
    mfpDecoderReleaseBuffer = _I1_releaseBuffer;
    return ME_OK;
}

void IOneDSPHandler::SetUdecModeConfig()
{
    AM_ASSERT(mpSharedRes);
    memset(&mUdecModeConfig, 0, sizeof(iav_udec_mode_config_t));
    memset(&mUdecDeintConfig, 0, sizeof(iav_udec_deint_config_t));

    //hard code here, to do
    if (muDecType != UDEC_MP4S)
        mUdecModeConfig.postp_mode = 2;
    else
        mUdecModeConfig.postp_mode = mpSharedRes->ppmode;
    AM_PRINTF("****mUdecModeConfig.postp_mode %d, muDecType %d.\n", mUdecModeConfig.postp_mode, muDecType);
    mUdecModeConfig.enable_deint = 0;
#if 0
    //mUdecModeConfig.enable_error_mode is obsolete
    //mUdecModeConfig.enable_error_mode = mpSharedRes->enable_udec_error_handling;
#endif
    mUdecModeConfig.pp_chroma_fmt_max = 2;
    AM_ASSERT(mMaxVoutWidth);
    AM_ASSERT(mMaxVoutHeight);
    AM_PRINTF("mMaxVoutWidth %d, mMaxVoutHeight %d.\n", mMaxVoutWidth, mMaxVoutHeight);
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

}

void IOneDSPHandler::SetUdecConfig(AM_INT index,int max_width,int max_height)
{
    AM_ASSERT(index >=0 && index < DMAX_UDEC_INSTANCE_NUM);
    if (index < 0 || index >= DMAX_UDEC_INSTANCE_NUM) {
        AM_ERROR("invalid udec index %d in SetUdecConfig.\n", index);
        return;
    }

    memset(&mUdecConfig[index], 0, sizeof(iav_udec_config_t));

    //hard code here
    mUdecConfig[index].tiled_mode = 0;
    mUdecConfig[index].frm_chroma_fmt_max = UDEC_CFG_FRM_CHROMA_FMT_420; // 4:2:0
    mUdecConfig[index].dec_types = mpSharedRes->codec_mask;
    mUdecConfig[index].max_frm_num = 20;
    mUdecConfig[index].max_frm_width = max_width;
    mUdecConfig[index].max_frm_height = max_height;
    mUdecConfig[index].max_fifo_size = 4*(1024*1024);

}

AM_INT IOneDSPHandler::EnterUdecMode(int max_width,int max_height)
{
    AM_INT state;
    AM_INT ret;
    ret = ioctl(mIavFd, IAV_IOC_GET_STATE, &state);
    //AM_ASSERT(state == IAV_STATE_IDLE);
    if (state != IAV_STATE_IDLE) {
        AM_PRINTF("UDEC Not in IDLE mode, enter IDLE mode first.\n");
        ret = ioctl(mIavFd, IAV_IOC_ENTER_IDLE, 0);
        if (ret != 0) {
            perror("IAV_IOC_ENTER_IDLE");
            AM_ERROR("UDEC enter IDLE mode fail, ret %d.\n", ret);
            return -1;
        }
    }

    //preset vout's osd before enter udec mode
    AM_UINT i = 0;
    iav_vout_fb_sel_t fb_sel;
    for (i = 0; i < eVoutCnt; i++) {
        /*if (mVoutConfig[i].failed) {
            continue;
        }*/
        if (mpSharedRes->dspConfig.voutConfigs.voutConfig[i].osd_disable/* != mVoutConfig[i].osd_disable*/) {
            AM_WARNING("vout(osd) %i have different setting, request osd_disable %d, current osd_disable %d.\n", i, mpSharedRes->dspConfig.voutConfigs.voutConfig[i].osd_disable, mVoutConfig[i].osd_disable);
            memset(&fb_sel, 0, sizeof(iav_vout_fb_sel_t));
            fb_sel.vout_id = i;

            if (mpSharedRes->dspConfig.voutConfigs.voutConfig[i].osd_disable) {
                AM_WARNING("disable osd on vout %d.\n", i);
                fb_sel.fb_id = -1;
            } else {
                AM_WARNING("enable osd on vout %d.\n", i);
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
    SetUdecConfig(0,max_width,max_height);
    mUdecModeConfig.udec_config = &mUdecConfig[0];

    AM_INFO("feature constrains: enable %d, codec mask 0x%x, request always disable l2 %d.\n", mUdecModeConfig.feature_constrains.set_constrains_enable, mpSharedRes->codec_mask, mUdecModeConfig.feature_constrains.always_disable_l2_cache);
    AM_INFO("           h264 no fmo %d, no lcd %d, no hdmi %d, no deinterlacer %d.\n", mUdecModeConfig.feature_constrains.h264_no_fmo, mUdecModeConfig.feature_constrains.vout_no_lcd, mUdecModeConfig.feature_constrains.vout_no_hdmi, mUdecModeConfig.feature_constrains.no_deinterlacer);

    AM_INFO("enter udec mode start\n");
    if (ioctl(mIavFd, IAV_IOC_ENTER_UDEC_MODE, &mUdecModeConfig) < 0) {
        perror("IAV_IOC_ENTER_UDEC_MODE");
        return -1;
    }

    AM_INFO("enter udec mode done\n");
    return 0;
}

extern "C" void rv40_change_resolution_impl(struct amba_decoding_accelerator_s *pAcc)
{
    /*release targe frame buffers requested
    */
    for (AM_INT i = 0; i < pAcc->amba_buffer_number; i++) {
        if(pAcc->p_mcresult_buffer[i])
        {
            iav_decoded_frame_t frame;
            memset(&frame, 0, sizeof(frame));
            frame.flags = 0;
            frame.fb_id = (u16) pAcc->mcresult_buffer_id[i];
            frame.real_fb_id = (u16) pAcc->mcresult_buffer_real_id[i];
            if (::ioctl(pAcc->amba_iav_fd, IAV_IOC_RELEASE_FRAME, &frame) < 0) {
                perror("IAV_IOC_RELEASE_FRAME");
                return;
            }
            pAcc->p_mcresult_buffer[i]  = NULL;
        }
    }
    /*update_image_info_only
    */
    iav_fbp_config_t config;
    memset(&config,0,sizeof(config));
    config.decoder_id = pAcc->decode_id;
    config.chroma_format = 1; //420
    config.tiled_mode = 0;
    //use the real pic size, chuchen,2012_3_6
    config.lu_width = pAcc->amba_picture_real_width;
    config.lu_height = pAcc->amba_picture_real_height;
    if (::ioctl(pAcc->amba_iav_fd, IAV_IOC_UDEC_UPDATE_FB_POOL_CONFIG, &config) < 0) {
        perror("IAV_IOC_UDEC_UPDATE_FB_POOL_CONFIG");
        return;
    }
   /* request new  target frame buffers*/
    for (AM_INT i = 0; i < pAcc->amba_buffer_number; i++) {
        iav_frame_buffer_t frame;
        memset(&frame, 0, sizeof(frame));
        frame.decoder_id = pAcc->decode_id;
        frame.pic_struct = 3;
        if (::ioctl(pAcc->amba_iav_fd, IAV_IOC_REQUEST_FRAME, &frame) < 0) {
            perror("IAV_IOC_REQUEST_FRAME");
            return;
        }
        pAcc->p_mcresult_buffer[i] = (AM_U8*)frame.lu_buf_addr;
        pAcc->mcresult_buffer_size[i] = frame.buffer_pitch * frame.buffer_height;
        pAcc->mcresult_buffer_id[i] = (AM_UINT)frame.fb_id;
        pAcc->mcresult_buffer_real_id[i] = (AM_UINT)frame.real_fb_id;
    }
 }
amba_decoding_accelerator_t* IOneDSPHandler::InitHybirdUdec(AM_INT index, AM_INT pic_width, AM_INT pic_height)
{
    AM_INT i;
    AM_ASSERT(mpSharedRes);
    AM_ASSERT(mVoutConfigMask == mpSharedRes->pbConfig.vout_config);
    AM_ASSERT(mIavFd >= 0);
    AM_ASSERT(muDecType > UDEC_NONE);
    AM_ASSERT(muDecType < UDEC_LAST);

    AM_ASSERT(pic_width>0);
    AM_ASSERT(pic_height>0);
    AM_ASSERT(!(pic_width&(15)));
    AM_ASSERT(!(pic_height&(15)));

    AM_ASSERT(index >=0 && index < DMAX_UDEC_INSTANCE_NUM);
    if (index < 0 || index >= DMAX_UDEC_INSTANCE_NUM) {
        AM_ERROR("invalid udec index %d in InitUdec.\n", index);
        return NULL;
    }
    AM_PRINTF("InitHybirdUdec mIavFd %d, udec_type %d, width %d, height %d, vout_config %d, vout_start %d, vout_num %d.\n", mIavFd, muDecType, pic_width, pic_height, mVoutConfigMask, mVoutStartIndex, mVoutNumber);

    amba_decoding_accelerator_t* pAcc = (amba_decoding_accelerator_t*) malloc(sizeof(amba_decoding_accelerator_t));

    if (!pAcc) {
        AM_PRINTF("allocate pAcc fail.\n");
        return NULL;
    }
    memset((void*)pAcc, 0x0, sizeof(amba_decoding_accelerator_t));
    pAcc->amba_iav_fd = mIavFd;

    //for safe here, force 16 byte align
    pAcc->amba_picture_width = (pic_width+ 15)&(~15);//for ring buffer, pic size should align to MB(16x16)
    pAcc->amba_picture_height = (pic_height+ 15)&(~15);

    pAcc->amba_picture_real_width = pic_width;
    pAcc->amba_picture_real_height = pic_height;

    if (muDecType == UDEC_RV40) {
        pAcc->amba_buffer_number = 2;
        pAcc->amba_mv_buffer_size = (pAcc->amba_picture_width>>4) * (pAcc->amba_picture_height>>4) * sizeof(RVDEC_MBINFO_t);
        pAcc->rv40_change_resolution = rv40_change_resolution_impl;
    } else if (muDecType == UDEC_MP4S) {
        //amba_buffer_number read from ppmod_config, if invalid, set default 2
        int buffer_number=mpSharedRes->pbConfig.input_buffer_number;
        if(buffer_number<=2)
            buffer_number = 2;
        else if(buffer_number>4)//for 1080p, chunk number cannot bigger than 4, or dsp will assertion in memory operation
            buffer_number = 4;
        pAcc->amba_buffer_number = buffer_number;
        pAcc->amba_mv_buffer_size = CHUNKHEAD_VOPINFO_SIZE + (pAcc->amba_picture_width>>4) * (pAcc->amba_picture_height>>4) * sizeof(mb_mv_B_t);
        //guarantee mv fifo chunk size 32Bytes aligned here
        AM_PRINTF("UDEC_MP4S before aligned to 32Bytes, amba_mv_buffer_size=%d.\n",pAcc->amba_mv_buffer_size);
        pAcc->amba_mv_buffer_size = (pAcc->amba_mv_buffer_size+31)&(~31);
        AM_PRINTF("UDEC_MP4S after aligned to 32Bytes, amba_mv_buffer_size=%d.\n",pAcc->amba_mv_buffer_size);
    } else {
        AM_ASSERT(muDecType == UDEC_SW);
        pAcc->amba_buffer_number = 0;
    }

    pAcc->amba_idct_buffer_size = (pAcc->amba_picture_width>>4) * (pAcc->amba_picture_height>>4) * sizeof(mb_idctcoef_t);
    //pAcc->amba_mv_buffer_size = (pAcc->amba_picture_width>>4) * (pAcc->amba_picture_height>>4) * sizeof(mb_mv_B_t);


    //config mUdecInfo[index] and related vout
    memset(&mUdecInfo[index], 0, sizeof(mUdecInfo[index]));
    //set vout 1, hdmi, hard code here
    memset(&mUdecVoutConfig[0], 0, eVoutCnt * sizeof(iav_udec_vout_config_t));

    mUdecInfo[index].udec_id = 0;
    mUdecInfo[index].udec_type = muDecType;
    mUdecInfo[index].enable_err_handle = mpSharedRes->dspConfig.errorHandlingConfig[0].enable_udec_error_handling;
    mUdecInfo[index].enable_pp = 1;
    mUdecInfo[index].enable_deint = mUdecModeConfig.enable_deint;
    mUdecInfo[index].interlaced_out = 0;
    if (mUdecInfo[index].enable_err_handle) {
        AM_ASSERT(mpSharedRes->dspConfig.errorHandlingConfig[0].enable_udec_error_handling);
        mUdecInfo[index].concealment_mode = mpSharedRes->dspConfig.errorHandlingConfig[0].error_concealment_mode;
        mUdecInfo[index].concealment_ref_frm_buf_id = mpSharedRes->dspConfig.errorHandlingConfig[0].error_concealment_frame_id;
        AM_PRINTF("enable udec error handling: concealment mode %d, frame id %d.\n",mUdecInfo[index].concealment_mode, mUdecInfo[index].concealment_ref_frm_buf_id);
    }

#if 0
    mUdecInfo[index].num_vout = mVoutNumber;
    mUdecInfo[index].vout_config = &mUdecVoutConfig[mVoutStartIndex];
    mUdecInfo[index].input_center_x = pic_width / 2;
    mUdecInfo[index].input_center_y = pic_height / 2;
#else
    mUdecInfo[index].vout_configs.num_vout = mVoutNumber;
    mUdecInfo[index].vout_configs.vout_config = &mUdecVoutConfig[mVoutStartIndex];
    mUdecInfo[index].vout_configs.input_center_x = pic_width / 2;
    mUdecInfo[index].vout_configs.input_center_y = pic_height / 2;
#endif

    mUdecInfo[index].udec_type = muDecType;
    //for hybird dec, bit_fifo is IDCT coef buffer or DCT residual buffer
    mUdecInfo[index].ref_cache_size = 0;

    switch (muDecType) {
        case UDEC_MP4S:
            mUdecInfo[index].u.mp4s.deblocking_flag = mpSharedRes->dspConfig.deblockingFlag;
            mUdecInfo[index].u.mp4s.pquant_mode = mpSharedRes->dspConfig.deblockingConfig.pquant_mode;
            for(i=0; i<32; i++ )
            {
                mUdecInfo[index].u.mp4s.pquant_table[i] = (AM_U8)mpSharedRes->dspConfig.deblockingConfig.pquant_table[i];
            }
            mUdecInfo[index].bits_fifo_size = pAcc->amba_idct_buffer_size*pAcc->amba_buffer_number;

            mUdecInfo[index].u.mp4s.mv_fifo_size = pAcc->amba_mv_buffer_size * pAcc->amba_buffer_number;
            break;

        case UDEC_RV40:
            //for new residue layout
            mUdecInfo[index].bits_fifo_size = (pAcc->amba_idct_buffer_size + pAcc->amba_picture_width *48)*pAcc->amba_buffer_number;
            //mUdecInfo[index].bits_fifo_size = pAcc->amba_idct_buffer_size*pAcc->amba_buffer_number;
            mUdecInfo[index].u.rv40.mv_fifo_size = pAcc->amba_mv_buffer_size * pAcc->amba_buffer_number;
            break;

        case UDEC_SW:
            break;
        default:
            AM_ERROR("wrong hybird udec type %d not implemented\n", muDecType);
            free(pAcc);
            return NULL;
    }

    //vout setting
    for (i = 0; i< eVoutCnt; i++) {
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
    //caculate zoom_factor
    //zoom_factor = target_win_size / real_pic_size
    /*AM_INT j = 0;
    if((!mpSharedRes->dspConfig.enableDeinterlace)&&((pic_width&(15)) || (pic_height&(15)))){//for NO-HDMI playback hang issue, temporarily mdf for Interlaced files, roy 2012.02.17
        //the real video pic size is not 16-align
        float ratio_x,ratio_y;
        for(j = 0;j< eVoutCnt;j++){
            ratio_x = (float)mUdecVoutConfig[j].target_win_width / pic_width;
            ratio_y = (float)mUdecVoutConfig[j].target_win_height/ pic_height;
            mUdecVoutConfig[j].zoom_factor_x = (int)(ratio_x * 0x10000);
            mUdecVoutConfig[j].zoom_factor_y = (int)(ratio_y * 0x10000);
        }
    }else{
        //the real video pic size is 16-align
        for(j = 0;j< eVoutCnt;j++){
            mUdecVoutConfig[j].zoom_factor_x = 1;
            mUdecVoutConfig[j].zoom_factor_y = 1;
        }
    }*/

    AM_PRINTF("sizeof(mb_mv_B_t) %d, sizeof(RVDEC_MBINFO_t) %d.\n", sizeof(mb_mv_B_t), sizeof(RVDEC_MBINFO_t));
    AM_PRINTF("request bits fifo size %d, buffer number %d\n", mUdecInfo[index].bits_fifo_size, pAcc->amba_buffer_number);
    AM_PRINTF("request mUdecInfo[index].u.rv40.mv_fifo_size %d, mUdecInfo[index].u.mp4s.mv_fifo_size %d.\n", mUdecInfo[index].u.mp4s.mv_fifo_size, mUdecInfo[index].u.rv40.mv_fifo_size);
    AM_PRINTF("UDEC_MP4S deblocking_flag %d, pquant_mode %d.\n", mUdecInfo[index].u.mp4s.deblocking_flag, mUdecInfo[index].u.mp4s.pquant_mode);
    for (i = 0; i<4; i++) {
        AM_PRINTF(" pquant_table[%d - %d]:\n", i*8, i*8+7);
        AM_PRINTF(" %d, %d, %d, %d, %d, %d, %d, %d.\n", \
            mUdecInfo[index].u.mp4s.pquant_table[i*8], mUdecInfo[index].u.mp4s.pquant_table[i*8+1], mUdecInfo[index].u.mp4s.pquant_table[i*8+2], mUdecInfo[index].u.mp4s.pquant_table[i*8+3], \
            mUdecInfo[index].u.mp4s.pquant_table[i*8+4], mUdecInfo[index].u.mp4s.pquant_table[i*8+5], mUdecInfo[index].u.mp4s.pquant_table[i*8+6], mUdecInfo[index].u.mp4s.pquant_table[i*8+7] \
        );
    }

    AM_PRINTF("IAV_IOC_INIT_UDEC start.\n");
    if (ioctl(mIavFd, IAV_IOC_INIT_UDEC, &mUdecInfo[index]) < 0) {
        perror("IAV_IOC_INIT_UDEC");
        AM_ERROR("IAV_IOC_INIT_UDEC %d fail.\n", muDecType);
        free(pAcc);
        return NULL;
    }
    AM_PRINTF("IAV_IOC_INIT_UDEC done.\n");

    AM_ASSERT(mUdecInfo[index].bits_fifo_start);
    AM_ASSERT(mUdecInfo[index].mv_fifo_start);

    //map bit stream and mv stream buffer
    pAcc->p_amba_idct_ringbuffer = mUdecInfo[index].bits_fifo_start;
    pAcc->p_amba_mv_ringbuffer = mUdecInfo[index].mv_fifo_start;
    pAcc->decode_id = mUdecInfo[index].udec_id;
    pAcc->decode_type = mUdecInfo[index].udec_type;
    pAcc->does_render_by_filter = (1==mUdecModeConfig.postp_mode);
#if PLATFORM_ANDROID
    register_rv40_neon(&pAcc->rv40_accel);
#endif
    AM_PRINTF("**decoder id %d, decoder type %d.\n", pAcc->decode_id, pAcc->decode_type);
    AM_PRINTF("**createhybirdUDEC success, iavfd %d, decode_id %d.\n", pAcc->amba_iav_fd, pAcc->decode_id);
    AM_PRINTF(" info.bits_fifo_start %p, info.mv_fifo_start %p.\n", pAcc->p_amba_idct_ringbuffer, pAcc->p_amba_mv_ringbuffer);

#ifdef AM_DEBUG
    if (pAcc->decode_type!= UDEC_SW) {
        AM_ASSERT(pAcc->p_amba_idct_ringbuffer);
        AM_ASSERT(pAcc->p_amba_mv_ringbuffer);
    }
#endif

    return pAcc;
}


AM_INT IOneDSPHandler::ReleaseUdec(AM_INT index)
{
    AM_ASSERT(index >=0 && index < DMAX_UDEC_INSTANCE_NUM);
    if (index < 0 || index >= DMAX_UDEC_INSTANCE_NUM) {
        AM_ERROR("invalid udec index %d in ReleaseUdec.\n", index);
        return -1;
    }

    AM_PRINTF("start ReleaseUdec...\n");
    if (ioctl(mIavFd, IAV_IOC_RELEASE_UDEC, index) < 0) {
        perror("IAV_IOC_DESTROY_UDEC");
        AM_ERROR("IAV_IOC_RELEASE_UDEC %d fail.\n", index);
        return -2;
    }
    AM_PRINTF("end ReleaseUdec\n");
    return 0;
}

amba_decoding_accelerator_t* IOneDSPHandler::CreateHybirdUDEC(AVCodecContext * codec, SConsistentConfig*pSharedRes, AM_INT& error)
{
    AM_INT ret = 0, i = 0;
    error = 0;
    unsigned int* psync = NULL;

    amba_decoding_accelerator_t* pAcc = NULL;
    if (!codec ||!pSharedRes) {
        AM_ERROR("**Error: CreateHybirdUDEC NULL input!\n");
        error = 1;
        return NULL;
    }
    mpSharedRes = pSharedRes;
    mVoutConfigMask = mpSharedRes->pbConfig.vout_config;
    AM_ASSERT(mVoutConfigMask == (1<<eVoutLCD) || mVoutConfigMask == (1<<eVoutHDMI) || (mVoutConfigMask == ((1<<eVoutHDMI)|(1<<eVoutLCD))));

    mMaxVoutWidth = 0;
    mMaxVoutHeight = 0;
    //parse voutconfig
    mVoutNumber = 0;
    mVoutStartIndex = 0;

    //get vout parameters
    for (i = 0; i < eVoutCnt; i++) {
        /*if (!(mVoutConfigMask&(1<<i))) {
            AM_INFO(" not config vout %d.\n", i);
            continue;
        }*/
        ret = getVoutPrams(mIavFd, i, &mVoutConfig[i]);
        AM_INFO("VOUT id %d, failed?%d.  size_x %d, size_y %d, pos_x %d, pos_y %d.\n", i, mVoutConfig[i].failed, mVoutConfig[i].size_x, mVoutConfig[i].size_y, mVoutConfig[i].pos_x, mVoutConfig[i].pos_y);
        AM_INFO("    width %d, height %d, flip %d, rotate %d.\n", mVoutConfig[i].width, mVoutConfig[i].height, mVoutConfig[i].flip, mVoutConfig[i].rotate);
        if (ret < 0 || mVoutConfig[i].failed || !mVoutConfig[i].width || !mVoutConfig[i].height) {
            mVoutConfigMask &= ~(1<<i);
            mpSharedRes->pbConfig.vout_config = mVoutConfigMask;
            AM_ERROR("vout %d failed.\n", i);
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
    AM_PRINTF("mVoutStartIndex %d, mVoutNumber %d, mVoutConfigMask 0x%x.\n", mVoutStartIndex, mVoutNumber, mVoutConfigMask);

    if (!mVoutNumber) {
        AM_ERROR("InitHybirdUdec invalid mVoutConfigMask %x.\n", mVoutConfigMask);
        error = 2;
        return NULL;
    }

    switch (codec->codec_id) {
        case CODEC_ID_AMBA_P_RV40:
            muDecType = (DEC_HYBRID==mpSharedRes->decCurrType)?UDEC_RV40:UDEC_SW;
            break;

        case CODEC_ID_AMBA_P_MPEG4:
        case CODEC_ID_AMBA_P_H263:
        case CODEC_ID_AMBA_P_MSMPEG4V1:
        case CODEC_ID_AMBA_P_MSMPEG4V2:
        case CODEC_ID_AMBA_P_MSMPEG4V3:
        case CODEC_ID_AMBA_P_WMV1:
        case CODEC_ID_AMBA_P_H263I:
        case CODEC_ID_AMBA_P_FLV1:
            muDecType = (DEC_HYBRID==mpSharedRes->decCurrType)?UDEC_MP4S:UDEC_SW;
            break;

        //pure sw decoder, can use nv12 buffer
        case CODEC_ID_AMBA_RV40:
        case CODEC_ID_AMBA_MPEG4:
        case CODEC_ID_AMBA_H263:
        case CODEC_ID_AMBA_MSMPEG4V1:
        case CODEC_ID_AMBA_MSMPEG4V2:
        case CODEC_ID_AMBA_MSMPEG4V3:
        case CODEC_ID_AMBA_WMV1:
        case CODEC_ID_AMBA_H263I:
        case CODEC_ID_AMBA_FLV1:
            muDecType = UDEC_SW;
            break;

        //use pure sw decoder, use yuv420 buffer, cannot use nv12 buffer
        default:
            muDecType = UDEC_SW;
            break;
    }

    int max_width = (codec->width+ 15)&(~15);
    int max_height = (codec->height + 15)&(~15);
    if ((ret = EnterUdecMode(max_width, max_height)) != 0) {
        AM_ERROR("**Error: EnterUdecMode fail ret %d.\n", ret);
        error = ret;
        return NULL;
    }

    switch (codec->codec_id) {
        case CODEC_ID_AMBA_P_RV40:
            map_dsp(mIavFd);
            if (muDecType == UDEC_RV40) {
                psync = get_sync_counter_pointer(mIavFd);
            }

            pAcc = InitHybirdUdec(0, codec->width, codec->height);
            if (pAcc && (muDecType == UDEC_RV40)) {
                codec->p_extern_accelerator =(void*)(pAcc);
                codec->acceleration = _I1_hybirdUDECDecode;//blocked until dsp process done
//                codec->dec_eos_dummy_frame = NULL;
                codec->get_decoded_frame = NULL;
                codec->request_inputbuffer = _I1_hybirdUDECRequestInputbuffer;
                codec->extern_accelerator_type = accelerator_type_amba_hybirdrv40_mc;

                codec->get_buffer = _I1_getBuffer;
                codec->release_buffer = _I1_releaseBuffer;

                pAcc->p_sync_counter = psync;//get_sync_counter_pointer(mIavFd);
            } else {
                codec->p_extern_accelerator =(void*)(pAcc);
                codec->acceleration = NULL;
                codec->get_decoded_frame = NULL;
                codec->request_inputbuffer = NULL;
                codec->extern_accelerator_type = accelerator_type_amba_sw_direct_rendering;

                codec->get_buffer = _I1_getBuffer;
                codec->release_buffer = _I1_releaseBuffer;
            }
            return pAcc;
            break;

        case CODEC_ID_AMBA_P_MPEG4:
        case CODEC_ID_AMBA_P_H263:
        case CODEC_ID_AMBA_P_MSMPEG4V1:
        case CODEC_ID_AMBA_P_MSMPEG4V2:
        case CODEC_ID_AMBA_P_MSMPEG4V3:
        case CODEC_ID_AMBA_P_WMV1:
        case CODEC_ID_AMBA_P_H263I:
        case CODEC_ID_AMBA_P_FLV1:
            if(DEC_HYBRID==mpSharedRes->decCurrType)
            {
                pAcc = InitHybirdUdec(0, codec->width, codec->height);
                if (pAcc) {
                    codec->p_extern_accelerator =(void*)(pAcc);
                    codec->acceleration = _I1_hybirdUDECDecode;
//                    codec->dec_eos_dummy_frame = _I1_hybirdUDECDecEOSDummyFrame;
                    codec->get_decoded_frame = _I1_hybirdUDECGetDecodedFrame;
                    codec->request_inputbuffer = _I1_hybirdUDECRequestInputbuffer;
                    codec->extern_accelerator_type = accelerator_type_amba_hybirdmpeg4_idctmc;

                    //hybird mpeg4, dsp will take idct/mc, sw decoding need not video buffer
                    mfpDecoderGetBuffer = codec->get_buffer = _I1_fake_getBuffer;
                    mfpDecoderReleaseBuffer = codec->release_buffer = _I1_fake_releaseBuffer;
                    mfpRenderVideoBuffer = _I1_renderVideoBuffer_mpeg4hybird;
                    return pAcc;
                }
                break;
            }
        //pure sw decoder, can use nv12 buffer
        case CODEC_ID_AMBA_RV40:
        case CODEC_ID_AMBA_MPEG4:
        case CODEC_ID_AMBA_H263:
        case CODEC_ID_AMBA_MSMPEG4V1:
        case CODEC_ID_AMBA_MSMPEG4V2:
        case CODEC_ID_AMBA_MSMPEG4V3:
        case CODEC_ID_AMBA_WMV1:
        case CODEC_ID_AMBA_H263I:
        case CODEC_ID_AMBA_FLV1:
            map_dsp(mIavFd);
            pAcc = InitHybirdUdec(0, codec->width, codec->height);
            if (pAcc) {
                codec->p_extern_accelerator =(void*)(pAcc);
                codec->acceleration = NULL;
//                codec->dec_eos_dummy_frame - NULL;
                codec->get_decoded_frame = NULL;
                codec->request_inputbuffer = NULL;
                codec->extern_accelerator_type = accelerator_type_amba_sw_direct_rendering;

                codec->get_buffer = _I1_getBuffer;
                codec->release_buffer = _I1_releaseBuffer;
                return pAcc;
            }
            break;

        //use pure sw decoder, use yuv420 buffer, cannot use nv12 buffer
        default:
            map_dsp(mIavFd);
            pAcc = InitHybirdUdec(0, codec->width, codec->height);
            if (pAcc) {
                codec->p_extern_accelerator =(void*)(pAcc);
                codec->acceleration = NULL;
//                codec->dec_eos_dummy_frame - NULL;
                codec->get_decoded_frame = NULL;
                codec->request_inputbuffer = NULL;
                codec->extern_accelerator_type = accelerator_type_none;

                return pAcc;
            }

            break;
    }

    codec->p_extern_accelerator = NULL;
    codec->acceleration = NULL;
//    codec->dec_eos_dummy_frame - NULL;
    codec->extern_accelerator_type = accelerator_type_none;
    codec->request_inputbuffer = NULL;
    codec->get_decoded_frame = NULL;
    return NULL;
}

void IOneDSPHandler::DeleteHybirdUDEC(amba_decoding_accelerator_t* pAcc)
{
    if (!pAcc) {
        AM_ERROR("Already closed?.\n");
        return;
    }

    if (pAcc->amba_iav_fd >= 0) {
        if (pAcc->decode_type == UDEC_RV40) {
            release_sync_counter_pointer(pAcc->amba_iav_fd);
        }
        ReleaseUdec(0);
        pAcc->amba_iav_fd = -1;
        if(muDecType != UDEC_MP4S){
            unmap_dsp(mIavFd);
        }
    }

    free(pAcc);
    return;
}

IOneDSPHandler::~IOneDSPHandler()
{
    //restore vout's osd if needed
    AM_UINT i = 0;
    iav_vout_fb_sel_t fb_sel;
    for (i = 0; i < eVoutCnt; i++) {
        if (mpSharedRes->dspConfig.voutConfigs.need_restore_osd[i]) {
            AM_WARNING("vout(osd) %i need restore, current mVoutConfig[i].osd_disable %d.\n", i, mVoutConfig[i].osd_disable);
            memset(&fb_sel, 0, sizeof(iav_vout_fb_sel_t));
            fb_sel.vout_id = i;

            if (mpSharedRes->dspConfig.voutConfigs.voutConfig[i].osd_disable) {
                AM_WARNING("restore osd setting, enable osd on vout %d.\n", i);
                fb_sel.fb_id = 0;//link to fb 0, hard code here
            } else {
                AM_WARNING("restore osd setting, disable osd on vout %d.\n", i);
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

    if (mIavFd >= 0) {
        ::close(mIavFd);
    }
}

void StoreBackDuplexSettings(SEncodingModeConfig* p_enc_config, iav_duplex_mode_config_t* drv_config)
{
    AM_ASSERT(p_enc_config);
    AM_ASSERT(drv_config);
    p_enc_config->preview_vout_index = drv_config->preview_vout_index;
    p_enc_config->preview_alpha = drv_config->preview_alpha;
    p_enc_config->playback_vout_index = drv_config->vout_index;

    p_enc_config->pb_display_enabled = drv_config->pb_display_enabled;
    p_enc_config->preview_enabled = drv_config->preview_display_enabled;

    p_enc_config->preview_in_pip = drv_config->preview_in_pip;
    p_enc_config->playback_in_pip = drv_config->pb_display_in_pip;
    p_enc_config->main_win_width = drv_config->main_width;
    p_enc_config->main_win_height = drv_config->main_height;

    p_enc_config->enc_offset_x = drv_config->enc_left;
    p_enc_config->enc_offset_y = drv_config->enc_top;
    p_enc_config->enc_width = drv_config->enc_width;
    p_enc_config->enc_height = drv_config->enc_height;

    p_enc_config->preview_left = drv_config->preview_left;
    p_enc_config->preview_top = drv_config->preview_top;
    p_enc_config->preview_width = drv_config->preview_width;
    p_enc_config->preview_height = drv_config->preview_height;

    p_enc_config->pb_display_left = drv_config->pb_display_left;
    p_enc_config->pb_display_top = drv_config->pb_display_top;
    p_enc_config->pb_display_width = drv_config->pb_display_width;
    p_enc_config->pb_display_height = drv_config->pb_display_height;

    p_enc_config->previewc_rawdata_enabled = drv_config->rawdata_config[DSP_PREVIEW_ID_C].enable;
    p_enc_config->previewc_scaled_width = drv_config->rawdata_config[DSP_PREVIEW_ID_C].scaled_width;
    p_enc_config->previewc_scaled_height = drv_config->rawdata_config[DSP_PREVIEW_ID_C].scaled_height;
    p_enc_config->previewc_scaled_width = drv_config->rawdata_config[DSP_PREVIEW_ID_C].scaled_width;
    p_enc_config->previewc_crop_offset_x = drv_config->rawdata_config[DSP_PREVIEW_ID_C].crop_offset_x;
    p_enc_config->previewc_crop_offset_y = drv_config->rawdata_config[DSP_PREVIEW_ID_C].crop_offset_y;

    p_enc_config->previewc_crop_width = drv_config->rawdata_config[DSP_PREVIEW_ID_C].crop_width;
    p_enc_config->previewc_crop_height = drv_config->rawdata_config[DSP_PREVIEW_ID_C].crop_height;
}

#endif




