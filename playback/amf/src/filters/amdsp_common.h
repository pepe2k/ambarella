
/*
 * amdsp_common.h
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

#ifndef __AMDSP_COMMON_H__
#define __AMDSP_COMMON_H__

extern "C"{
#include "libavcodec/avcodec.h"
#include "libavcodec/amba_dsp_define.h"
#include <basetypes.h>
#include "ambas_common.h"
#include "ambas_vout.h"
}

//#define DMAX_UDEC_VOUT_NUM 2


//data wrapper for udec

//with Signature Code 0x2449504F "$IPO", tmp use blow macro
#define _add_signature_code_

#ifdef _add_signature_code_
#define DUDEC_SEQ_HEADER_LENGTH 20
#define DUDEC_SEQ_HEADER_EX_LENGTH 24//2011.09.23 has resolution info case by roy
#define DUDEC_PES_HEADER_LENGTH 24
#else
#define DUDEC_SEQ_HEADER_LENGTH 16
#define DUDEC_SEQ_HEADER_EX_LENGTH 20//2011.09.23 has resolution info case by roy
#define DUDEC_PES_HEADER_LENGTH 20
#endif

#define DUDEC_SEQ_STARTCODE_H264 0x7D
#ifdef _add_signature_code_
#define DUDEC_SEQ_STARTCODE_MPEG4 0xC4
#else
#define DUDEC_SEQ_STARTCODE_MPEG4 0xB7
#endif
#define DUDEC_SEQ_STARTCODE_VC1WMV3 0x71

#define DUDEC_PES_STARTCODE_H264 0x7B
#ifdef _add_signature_code_
#define DUDEC_PES_STARTCODE_MPEG4 0xC5
#else
#define DUDEC_PES_STARTCODE_MPEG4 0xB8
#endif
#define DUDEC_PES_STARTCODE_VC1WMV3 0x72
#define DUDEC_PES_STARTCODE_MPEG12 0xE0

#define DPRIVATE_GOP_NAL_HEADER_LENGTH 22

enum {
    UDEC_VFormat_H264 = 1,
    UDEC_VFormat_MPEG4 = 2,
    UDEC_VFormat_VC1 = 3,
    UDEC_VFormat_WMV3 = 4,
    UDEC_VFormat_MPEG12 = 5,//now mpeg12 ONLY add PES header, NO add seq header, 2012.01.05, roy mdf
};


//little endian
AM_UINT FillUSEQHeader(AM_U8* pHeader, AM_U32 vFormat, AM_U32 timeScale, AM_U32 tickNum, AM_U32 is_mp4s_flag, AM_S32 vid_container_width, AM_S32 vid_container_height);
void InitUPESHeader(AM_U8* pHeader, AM_U32 vFormat);
AM_UINT FillPESHeader(AM_U8* pHeader, AM_U32 ptsLow, AM_U32 ptsHigh, AM_U32 auDatalen, AM_UINT hasPTS, AM_UINT isPTSJump);
AM_UINT FillPrivateGopNalHeader(AM_U8* pHeader, AM_U32 timeScale, AM_U32 tickNum, AM_UINT m, AM_UINT n, AM_U32 pts_high, AM_U32 pts_low);

#if 0
//udec common
AM_INT FlushUDEC(AM_INT iavFd, AM_INT udec_id);
AM_INT ClearStopFlags(AM_INT iavFd, AM_INT udec_id);
#endif

//-----------------------------------------------------------------------
//
// CIAVVideoBufferPool
//
//-----------------------------------------------------------------------
class CIAVVideoBufferPool: public CSimpleBufferPool
{
public:
    typedef CSimpleBufferPool inherited;
    static CIAVVideoBufferPool *Create(AM_INT iavFd, AM_UINT picWidth, AM_UINT picHeight, bool extEdge, amba_decoding_accelerator_t* pAcc);

protected:
    CIAVVideoBufferPool(int iavFd, AM_UINT picWidth, AM_UINT picHeight, bool extEdge, amba_decoding_accelerator_t* pAcc):
        inherited("CIAVVideoBufferPool"),
        mIavFd(iavFd),
        mbDeviceOpen(false),
        mPicWidth(picWidth),
        mPicHeight(picHeight),
        mBufferNum(10),
        mReservedBufferNum(5),
        mbExtEdge(extEdge),
        mpAcc(pAcc)
    {
        mAlignedPicWidth = (picWidth+ D_DSP_VIDEO_BUFFER_WIDTH_ALIGNMENT - 1)&(~(D_DSP_VIDEO_BUFFER_WIDTH_ALIGNMENT-1));
        mAlignedPicHeight = (picHeight + D_DSP_VIDEO_BUFFER_HEIGHT_ALIGNMENT - 1)&(~(D_DSP_VIDEO_BUFFER_HEIGHT_ALIGNMENT - 1));

        if (extEdge) {
            mPicWidthWithEdge = picWidth + 32;
            mPicHeightWithEdge = picHeight + 32;
            mFbWidth = mAlignedPicWidth + 32;
            mFbHeight = mAlignedPicHeight + 32;
        } else {
            mPicWidthWithEdge = picWidth;
            mPicHeightWithEdge = picHeight;
            mFbWidth = mAlignedPicWidth;
            mFbHeight = mAlignedPicHeight;
        }
        if (mAlignedPicWidth != picWidth || mAlignedPicHeight != picHeight) {
            AMLOG_ERROR("Warning: original picture size is not aligned: pic width %u, height %u, aligned width %u, height %u.\n", picWidth, picHeight, mAlignedPicWidth, mAlignedPicHeight);
        }
    }
    AM_ERR Construct();
    virtual ~CIAVVideoBufferPool();

protected:
    virtual AM_ERR OpenDevice() {AMLOG_ERROR("should not come here.\n"); AM_ASSERT(0); return ME_ERROR;};
    virtual void CloseDevice() {AMLOG_ERROR("should not come here.\n"); AM_ASSERT(0);};

public:
    virtual bool AllocBuffer(CBuffer*& pBuffer, AM_UINT size);

protected:
    virtual void OnReleaseBuffer(CBuffer *pBuffer);

protected:
    int mIavFd;
    bool mbDeviceOpen;

protected:
    AM_UINT mPicWidth;
    AM_UINT mPicHeight;
    AM_UINT mFbWidth;
    AM_UINT mFbHeight;
    AM_UINT mBufferNum;
    AM_UINT mReservedBufferNum; //reserved buffer number, sync the available buffer numbers in iav bufferpool and CIAVVideoBufferPool
    bool mbExtEdge;
    amba_decoding_accelerator_t* mpAcc;
    AM_UINT mAlignedPicWidth;
    AM_UINT mAlignedPicHeight;
    AM_UINT mPicWidthWithEdge;
    AM_UINT mPicHeightWithEdge;
};

class DSPHandler
{
public:
    static DSPHandler *Create();

public:
    CIAVVideoBufferPool* CreateBufferPool(AM_UINT pic_width, AM_UINT pic_height, bool ext_edge, amba_decoding_accelerator_t* p_acc);

public:
    DSPHandler()
    {
        mVoutNumber = 0;
        mVoutStartIndex = 0;
        mIavFd = -1;
        mpSharedRes = NULL;
        //mbExtendedEdge = false;
    }

    virtual ~DSPHandler()
    {
        //AM_ASSERT(0);
        //AM_ERROR("should not coming here.\n");
    }

protected:
    virtual AM_ERR Construct()
    {
        AM_ASSERT(0);
        AM_ERROR("should not coming here.\n");
        return ME_OK;
    }

public:
    virtual AM_INT EnterUdecMode(int max_width,int max_height)
    {
        //AM_ASSERT(0);
        //AM_ERROR("should not come here.\n");
        return -1;
    }

    virtual amba_decoding_accelerator_t* CreateHybirdUDEC(AVCodecContext * codec, SConsistentConfig*mpSharedRes, AM_INT& error)
    {
        AM_ASSERT(0);
        AM_ERROR("should not come here.\n");
        return NULL;
    }

    virtual void DeleteHybirdUDEC(amba_decoding_accelerator_t* pAcc)
    {
        AM_ASSERT(0);
        AM_ERROR("should not come here.\n");
    }

    virtual AM_INT StartUdec(amba_decoding_accelerator_t* pAcc)
    {
        AM_ASSERT(0);
        AM_ERROR("should not come here.\n");
        return 0;
    }

public:
    void (*mfpRenderVideoBuffer)(int iavFd, CVideoBuffer *pVideoBuffer, SRect* srcRect);
    int (*mfpDecoderGetBuffer)(AVCodecContext *s, AVFrame *pic);
    void (*mfpDecoderReleaseBuffer)(AVCodecContext *s, AVFrame *pic);

public:
    AM_INT mIavFd;

protected:
    AM_INT muDecType;

protected:
    iav_udec_mode_config_t mUdecModeConfig;
    iav_udec_deint_config_t mUdecDeintConfig;
    iav_udec_config_t mUdecConfig[DMAX_UDEC_INSTANCE_NUM];
    iav_udec_info_ex_t mUdecInfo[DMAX_UDEC_INSTANCE_NUM];
    iav_udec_vout_config_t mUdecVoutConfig[eVoutCnt];
    SVoutConfig mVoutConfig[eVoutCnt];//0:lcd, 1:hdmi

protected:
    AM_INT mMaxVoutWidth;
    AM_INT mMaxVoutHeight;

    AM_UINT mVoutConfigMask;    // vout mask bits: 1<<0 LCD; 1<<1 HDMI
    AM_UINT mVoutNumber;
    AM_UINT mVoutStartIndex;

protected:
    SConsistentConfig* mpSharedRes;
};


#if TARGET_USE_AMBARELLA_A5S_DSP

class CA5SVideoBufferPool: public CIAVVideoBufferPool
{
    typedef CIAVVideoBufferPool inherited;
public:
    CA5SVideoBufferPool(int iavFd, AM_UINT picWidth, AM_UINT picHeight, bool extEdge, amba_decoding_accelerator_t* pAcc):
        inherited(iavFd, picWidth, picHeight, extEdge, pAcc)
    {
    }
    virtual ~CA5SVideoBufferPool();
protected:
    virtual AM_ERR OpenDevice();
    virtual void CloseDevice();
};


class A5SDSPHandler: public DSPHandler
{
public:
    A5SDSPHandler():
        DSPHandler()
    {
    }
    ~A5SDSPHandler();
    virtual AM_INT EnterUdecMode();
protected:
    AM_ERR Construct();
};

#endif

#if (TARGET_USE_AMBARELLA_I1_DSP||TARGET_USE_AMBARELLA_S2_DSP)
class CIOneVideoBufferPool: public CIAVVideoBufferPool
{
    typedef CIAVVideoBufferPool inherited;
public:
    CIOneVideoBufferPool(int iavFd, AM_UINT picWidth, AM_UINT picHeight, bool extEdge, amba_decoding_accelerator_t* pAcc):
        inherited(iavFd, picWidth, picHeight, extEdge, pAcc)
    {
    }
    virtual ~CIOneVideoBufferPool();
    bool AllocBuffer(CBuffer*& pBuffer, AM_UINT size);
protected:
    virtual AM_ERR OpenDevice();
    virtual void CloseDevice();
};


class IOneDSPHandler: public DSPHandler
{
public:
    IOneDSPHandler():
        DSPHandler()
    {
    }
    virtual AM_INT EnterUdecMode(int max_width,int max_height);
    virtual amba_decoding_accelerator_t* CreateHybirdUDEC(AVCodecContext * codec, SConsistentConfig*mpSharedRes, AM_INT& error);
    virtual void DeleteHybirdUDEC(amba_decoding_accelerator_t* pAcc);
    ~IOneDSPHandler();

public:
    void SetUdecModeConfig();
    void SetUdecConfig(AM_INT index,int max_width,int max_height);
    amba_decoding_accelerator_t* InitHybirdUdec(AM_INT index, AM_INT pic_width, AM_INT pic_height);
    AM_INT ReleaseUdec(AM_INT index);
    AM_INT StartUdec(amba_decoding_accelerator_t* pAcc);

protected:
    AM_ERR Construct();

};

#endif

void StoreBackDuplexSettings(SEncodingModeConfig* p_enc_config, iav_duplex_mode_config_t* drv_config);
int getVoutPrams(AM_INT iavFd, AM_INT vout_id, SVoutConfig* pConfig);
int getVoutParamsEx(AM_INT iavFd, AM_INT vout_id, SVoutConfig* pConfig, amba_video_sink_mode* p_sink_mode);
int getVoutParams(AM_INT iavFd, AM_INT vout_id, SVoutConfig* pConfig);

#endif
