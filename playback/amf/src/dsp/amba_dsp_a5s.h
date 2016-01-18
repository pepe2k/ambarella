/**
 * amba_dsp_a5s.h
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

#ifndef AMBA_DSP_A5S_H
#define AMBA_DSP_A5S_H

#define DMAX_DECODER_INSTANCE_NUM_A5S 4

typedef struct {
    //AM_UINT dec_format;//h264 only, for a5s
    AM_UINT pic_width;
    AM_UINT pic_height;
    AM_UINT buf_width;
    AM_UINT buf_height;
    AM_UINT buf_number;
    AM_UINT ext_edge;
} SA5SVideoDecInfo;

class CAmbaDspA5s: public CObject, public IUDECHandler, public IVoutHandler
{
    typedef CObject inherited;
public:
    static CAmbaDspA5s* Create(SConsistentConfig* pShared);

private:
    CAmbaDspA5s(SConsistentConfig* pShared, DSPConfig* pConfig);
    AM_ERR Construct();
    ~CAmbaDspA5s();

public:
    virtual void *GetInterface(AM_REFIID refiid)
    {
        if (refiid == IID_IUdecHandler) {
            return (IUDECHandler*)this;
        } else if (refiid == IID_IVoutHandler) {
            return (IVoutHandler*)this;
        } else {
            return inherited::GetInterface(refiid);
        }
    }

    virtual void Delete() {inherited::Delete();}

//UdecHandler
public:
    //dsp mode switch
    virtual AM_ERR EnterUDECMode();
    virtual AM_ERR ExitUDECMode();

    //udec instance init/exit
    virtual AM_ERR InitUDECInstance(AM_INT& wantUdecIndex, DSPConfig*pConfig, void* pHybirdAcc, AM_INT decFormat, AM_INT pic_width, AM_INT pic_height, AM_INT extEdge, bool autoAllocIndex = true);
    virtual AM_ERR StartUdec(AM_INT udecIndex, AM_UINT format, void* pacc);

    virtual AM_ERR ReleaseUDECInstance(AM_INT udecIndex);
    virtual AM_ERR SetUdecNums(AM_INT udecNums){return ME_NO_IMPL;};

    //request input buffer (AllocBSB for hw mode)
    virtual AM_ERR RequestInputBuffer(AM_INT udecIndex, AM_UINT& size, AM_U8* pStart, AM_INT bufferCnt);
    virtual AM_ERR DecodeBitStream(AM_INT udecIndex, AM_U8*pStart, AM_U8*pEnd);
    virtual AM_ERR AccelDecoding(AM_INT udecIndex, void* pParam, bool needSync = false);

    //stop, flush, clear, trick play
    virtual AM_ERR StopUDEC(AM_INT udecIndex);
    virtual AM_ERR FlushUDEC(AM_INT udecIndex);
    virtual AM_ERR ClearUDEC(AM_INT udecIndex);
    virtual AM_ERR PauseUDEC(AM_INT udecIndex);
    virtual AM_ERR ResumeUDEC(AM_INT udecIndex);
    virtual AM_ERR StepPlay(AM_INT udecIndex);
    virtual AM_ERR TrickMode(AM_INT udecIndex, AM_UINT speed, AM_UINT backward);

    //frame buffer pool
    virtual AM_UINT InitFrameBufferPool(AM_INT udecIndex, AM_UINT picWidth, AM_UINT picHeight, AM_UINT chromaFormat, AM_UINT tileFormat, AM_UINT hasEdge);
    virtual AM_ERR ResetFrameBufferPool(AM_INT udecIndex);

    //buffer id/data pointer in CVideoBuffer
    virtual AM_ERR RequestFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer);
    virtual AM_ERR RenderFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer, SRect* srcRect);
    virtual AM_ERR ReleaseFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer);

    //avsync
    virtual AM_ERR SetClockOffset(AM_INT udecIndex, AM_S32 diff);
    virtual AM_ERR WakeVout(AM_INT udecIndex, AM_UINT voutMask);
    virtual AM_ERR GetUdecTimeEos(AM_INT udecIndex, AM_UINT& eos, AM_U64& udecTime, AM_INT nowait);

    //query UDEC state
    virtual AM_ERR GetUDECState(AM_INT udecIndex, AM_UINT& udecState, AM_UINT& voutState, AM_UINT& errorCode);

    //get decoded frame
    virtual AM_ERR GetDecodedFrame(AM_INT udecIndex, AM_INT& buffer_id, AM_INT& eos_invalid);

//IVoutHandler
public:
    virtual AM_ERR SetupVOUT(AM_INT type, AM_INT mode);
    //virtual AM_ERR GetCurrentVoutSettings(DSPConfig*pConfig, AM_UINT requestedVoutMask);

    virtual AM_ERR Enable(AM_UINT voutID, AM_INT enable);
    virtual AM_ERR EnableOSD(AM_UINT voutID, AM_INT enable);

    //direct config vout
    //virtual DSPVoutConfigs* DirectVoutConfig();
    //virtual AM_ERR UpdateVoutSettings();

    virtual AM_ERR GetSizePosition(AM_UINT voutID, AM_INT* pos_x, AM_INT* pos_y, AM_INT* size_x, AM_INT* size_y);
    virtual AM_ERR GetDimension(AM_UINT voutID, AM_INT* size_x, AM_INT* size_y);
    virtual AM_ERR GetPirtureSizeOffset(AM_INT* pic_width, AM_INT* pic_height, AM_INT* offset_x, AM_INT* offset_y);
    virtual AM_ERR GetSoureRect(AM_INT* pos_x, AM_INT* pos_y, AM_INT* size_x, AM_INT* size_y);

    //todo: mass apis about change size/position/flip/rotate/mirror/source rect
    virtual AM_ERR UpdatePirtureSizeOffset(AM_INT pic_width, AM_INT pic_height, AM_INT offset_x, AM_INT offset_y);
    virtual AM_ERR ChangeSizePosition(AM_UINT voutID, AM_INT pos_x, AM_INT pos_y, AM_INT size_x, AM_INT size_y);
    virtual AM_ERR ChangeSize(AM_UINT voutID, AM_INT size_x, AM_INT size_y);
    virtual AM_ERR ChangePosition(AM_UINT voutID, AM_INT pos_x, AM_INT pos_y);
    virtual AM_ERR ChangeSourceRect(AM_INT pos_x, AM_INT pos_y, AM_INT size_x, AM_INT size_y);
    virtual AM_ERR ChangeInputCenter(AM_INT pos_x, AM_INT pos_y);
    virtual AM_ERR ChangeRoomFactor(AM_UINT voutID, float factor_x, float factor_y);
    virtual AM_ERR Flip(AM_UINT voutID, AM_INT param);
    virtual AM_ERR Rotate(AM_UINT voutID, AM_INT param);
    virtual AM_ERR Mirror(AM_UINT voutID, AM_INT param);

private:
    AM_INT getAvaiableUdecinstance();
    AM_INT getVoutParams(AM_INT iavFd, AM_INT vout_id, DSPVoutConfig* pConfig);
    AM_INT setupVoutConfig(SConsistentConfig* mpShared);
    AM_ERR enterUdecMode();
    AM_ERR changeSize(AM_UINT voutID, AM_INT size_x, AM_INT size_y);
    AM_ERR changePosition(AM_UINT voutID, AM_INT pos_x, AM_INT pos_y);

    //AM_ERR configVout (AM_INT vout_id, AM_INT mode, AM_INT sink_type);
    //AM_ERR  setVoutSize(AM_INT vout_id, AM_INT mode);

private:
    SConsistentConfig* mpSharedRes;
    DSPConfig* mpConfig;
    AM_INT mIavFd;
    AM_UINT mbUDECMode;
    AM_UINT mbVoutSetup;

    SA5SVideoDecInfo mDecInfo[DMAX_DECODER_INSTANCE_NUM_A5S];
    AM_UINT mbInstanceUsed[DMAX_DECODER_INSTANCE_NUM_A5S];

    iav_mmap_info_t mMapInfo[DMAX_DECODER_INSTANCE_NUM_A5S];
    iav_config_decoder_t mUdecConfig[DMAX_DECODER_INSTANCE_NUM_A5S];

    //iav_udec_vout_config_t mUdecVoutConfig[eVoutCnt];

    AM_INT mMaxVoutWidth;
    AM_INT mMaxVoutHeight;

    AM_UINT mVoutConfigMask;    // vout mask bits: 1<<0 LCD; 1<<1 HDMI
    AM_UINT mVoutStartIndex;
    AM_UINT mVoutNumber;

    AM_INT mTotalUDECNumber;

protected:
    CMutex* mpMutex;
};


#endif

