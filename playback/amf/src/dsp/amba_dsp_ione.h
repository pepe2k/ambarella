/**
 * amba_dsp_ione.h
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

#ifndef AMBA_DSP_IONE_H
#define AMBA_DSP_IONE_H

typedef struct {
    AM_UINT dec_format;
    AM_UINT pic_width;
    AM_UINT pic_height;
    AM_UINT buf_width;
    AM_UINT buf_height;
    AM_UINT buf_number;
    AM_UINT ext_edge;
    AM_UINT tile_format;
} SVideoDecInfo;

class CGConfig;

class CAmbaDspIOne: public CObject, public IUDECHandler, public IVoutHandler
{
    typedef CObject inherited;
public:
    static CAmbaDspIOne* Create(SConsistentConfig* pShared);

private:
    CAmbaDspIOne(SConsistentConfig* pShared, DSPConfig* pConfig);
    AM_ERR Construct();
    ~CAmbaDspIOne();

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
    virtual AM_ERR EnterTranscodeMode();
    virtual AM_ERR ExitTranscodeMode();
    virtual AM_ERR EnterMdecMode();
    virtual AM_ERR ExitMdecMode();

    //udec instance init/exit
    virtual AM_ERR InitUDECInstance(AM_INT& wantUdecIndex, DSPConfig*pConfig, void* pHybirdAcc, AM_INT decFormat, AM_INT pic_width, AM_INT pic_height, AM_INT extEdge, bool autoAllocIndex = true);
    virtual AM_ERR InitTranscodeInstance(AM_INT& wantUdecIndex, DSPConfig*pConfig, void* pHybirdAcc, AM_INT decFormat, AM_INT pic_width, AM_INT pic_height, AM_INT extEdge, bool autoAllocIndex = true);
    virtual AM_ERR InitMdecInstance(AM_INT& wantUdecIndex, DSPConfig*pConfig, void* pHybirdAcc, AM_INT decFormat, AM_INT pic_width, AM_INT pic_height, AM_INT extEdge, bool autoAllocIndex = true);
    virtual AM_ERR StartUdec(AM_INT udecIndex, AM_UINT format, void* pacc);

    virtual AM_ERR ReleaseUDECInstance(AM_INT udecIndex);
    virtual AM_ERR SetUdecNums(AM_INT udecNums);

    //nvr transcode
    virtual AM_ERR InitMdecTranscoder(AM_INT width, AM_INT height, AM_INT bitrate);
    virtual AM_ERR ReleaseMdecTranscoder();
    virtual AM_ERR StartMdecTranscoder();
    virtual AM_ERR StopMdecTranscoder();
    virtual AM_ERR MdecTranscoderReadBits(void* bitInfo);
    virtual AM_ERR MdecTranscoderSetBitrate(AM_INT kbps);
    virtual AM_ERR MdecTranscoderSetFramerate(AM_INT fps, AM_INT reduction);
    virtual AM_ERR MdecTranscoderSetGOP(AM_INT M, AM_INT N, AM_INT interval, AM_INT structure);
    virtual AM_ERR MdecTranscoderDemandIDR(AM_BOOL now);

    //request input buffer (AllocBSB for hw mode)
    virtual AM_ERR RequestInputBuffer(AM_INT udecIndex, AM_UINT& size, AM_U8* pStart, AM_INT bufferCnt);
    virtual AM_ERR DecodeBitStream(AM_INT udecIndex, AM_U8*pStart, AM_U8*pEnd);
    virtual AM_ERR AccelDecoding(AM_INT udecIndex, void* pParam, bool needSync = false);
    inline void _Dump_es_data(AM_INT udecIndex, AM_U8* pStart, AM_U8* pEnd);

    //stop, flush, clear, trick play
    virtual AM_ERR StopUDEC(AM_INT udecIndex);
    virtual AM_ERR FlushUDEC(AM_INT udecIndex, AM_BOOL lastPic = AM_FALSE);
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
    virtual AM_ERR SetupVOUT(AM_INT type, AM_INT mode) { return ME_NO_IMPL;}
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

    //nvr
    virtual AM_ERR InitWindowRender(CGConfig* pConfig);
    virtual AM_ERR ConfigWindowRender(CGConfig* pConfig);
    virtual AM_ERR SwitchStream(AM_INT renIndex, AM_INT dspIndex, AM_INT flag);
    //virtual AM_ERR SwitchSpecifyToHD(AM_INT index, AM_INT flag);
    //virtual AM_ERR SwitchHDToSpecify(AM_INT index, AM_INT flag);
    virtual AM_ERR GetUDECState2(AM_INT udecIndex, AM_U8** dspRead );
    virtual AM_ERR DspBufferControl(AM_INT udecIndex, AM_UINT control, AM_UINT control_time);

    virtual AM_ERR PlaybackZoom(AM_INT renderIndex, AM_U16 input_win_width, AM_U16 input_win_height, AM_U16 center_x, AM_U16 center_y);

private:
    AM_INT getAvaiableUdecinstance();
    bool isUDECErrorState(AM_INT udecIndex);
    AM_INT getVoutParams(AM_INT iavFd, AM_INT vout_id, DSPVoutConfig* pConfig);
    void getDefaultDSPConfig(SConsistentConfig* mpShared);
    AM_INT setupVoutConfig(SConsistentConfig* mpShared);
    AM_ERR enterUdecMode();
    AM_ERR enterTranscodeMode();
    AM_ERR enterMdecMode();
    void setUdecModeConfig();
    void setTranscodeModeConfig();
    void setMdecModeConfig();
    void setMdecWindows(udec_window_t *window, AM_INT nr, AM_U8 enable_different_layout, AM_U8 display_layout);
    void setMdecRenders(udec_render_t *render, int nr);
    void updateSecondVoutSize();
    void setUdecConfig(AM_INT index);
    void setTranscConfig(AM_INT index);
    AM_ERR mapDSP(AM_INT index);
    AM_ERR unMapDSP(AM_INT index);
    AM_INT updateVoutSetting(AM_UINT voutID);
    AM_INT updateAllVoutSetting();
    AM_ERR InitMdecInternal(AM_INT index, DSPConfig*pConfig, AM_INT decFormat, AM_INT pic_width, AM_INT pic_height, AM_INT extEdge);
private:
    SConsistentConfig* mpSharedRes;
    DSPConfig* mpConfig;
    AM_INT mIavFd;
    AM_UINT mPpMode;
    bool mbUDECMode;

    SVideoDecInfo mDecInfo[DMAX_UDEC_INSTANCE_NUM];
    AM_UINT mbInstanceUsed[DMAX_UDEC_INSTANCE_NUM];

    iav_udec_mode_config_t mUdecModeConfig;
    iav_udec_deint_config_t mUdecDeintConfig;
    iav_udec_config_t mUdecConfig[DMAX_UDEC_INSTANCE_NUM];
    iav_udec_info_ex_t mUdecInfo[DMAX_UDEC_INSTANCE_NUM];
    iav_udec_vout_config_t mUdecVoutConfig[eVoutCnt];

    //transcode mode
    bool mbTranscodeMode;
    iav_transc_mode_config_t mTranscodeModeConfig;
    iav_udec_info2_t mTranscodeInfo[DMAX_UDEC_INSTANCE_NUM];

    //mdec mode
    bool mbMdecMode;
    iav_mdec_mode_config_t mdec_mode;
    iav_udec_mode_config_t *udec_mode;
    iav_udec_config_t *udec_configs;

    iav_postp_window_config_t iavWindows;
    iav_postp_render_config_t iavRenders;

    AM_INT mMaxVoutWidth;
    AM_INT mMaxVoutHeight;

    AM_UINT mVoutConfigMask;    // vout mask bits: 1<<0 LCD; 1<<1 HDMI
    AM_UINT mVoutStartIndex;
    AM_UINT mVoutNumber;

    AM_INT mTotalUDECNumber;
    AM_BOOL mbDSPMapped;

    //vout need picture related params(source rect), add a protection here, to prevent potential runtime risk(zero source width/height)
    //for multi-streams, there would be more source rect sets? source rect related code need implement for multi-streams in future
    AM_UINT mbPicParamBeenSet;
    AM_INT mGFlag;
protected:
    CMutex* mpMutex;
};

class CIOneDuplexSimpleDecAPI: public CObject, public ISimpleDecAPI
{
public:
    virtual AM_ERR InitDecoder(AM_UINT& dec_id, SDecoderParam* param, AM_UINT number_vout, SVoutConfig* vout_config);
    virtual AM_ERR ReleaseDecoder(AM_UINT dec_id);

    virtual AM_ERR RequestBitStreamBuffer(AM_UINT dec_id, AM_U8* pstart, AM_UINT room);
    virtual AM_ERR Decode(AM_UINT dec_id, AM_U8* pstart, AM_U8* pend);
    virtual AM_ERR Stop(AM_UINT dec_id, AM_UINT flag);
    virtual AM_ERR TrickPlay(AM_UINT dec_id, AM_UINT trickplay);

public:
    CIOneDuplexSimpleDecAPI(AM_INT fd, AM_UINT dsp_mode, AM_UINT dec_type, void* p):
        mIavFd(fd),
        mDSPMode(dsp_mode),
        mDecType(dec_type),
        mTotalStreamNumber(1)
    {
        AM_ASSERT(fd >= 0);
        AM_ASSERT(DSPMode_DuplexLowdelay == dsp_mode);
        DSetModuleLogConfig(LogModuleDSPHandler);
    }

    virtual ~CIOneDuplexSimpleDecAPI() {}

private:
    AM_INT mIavFd;
    AM_U16 mDSPMode;
    AM_U16 mDecType;//driver use
    AM_U16 mTotalStreamNumber;
    AM_U16 reserved0;
};

class CIOneUdecSimpleDecAPI: public CObject, public ISimpleDecAPI
{
public:
    virtual AM_ERR InitDecoder(AM_UINT& dec_id, SDecoderParam* param, AM_UINT number_vout, SVoutConfig* vout_config);
    virtual AM_ERR ReleaseDecoder(AM_UINT dec_id);

    virtual AM_ERR RequestBitStreamBuffer(AM_UINT dec_id, AM_U8* pstart, AM_UINT room);
    virtual AM_ERR Decode(AM_UINT dec_id, AM_U8* pstart, AM_U8* pend);
    virtual AM_ERR Stop(AM_UINT dec_id, AM_UINT stop_flag);
    virtual AM_ERR TrickPlay(AM_UINT dec_id, AM_UINT trickplay);

public:
    CIOneUdecSimpleDecAPI(AM_INT fd, AM_UINT dsp_mode, AM_UINT dec_type, void* p):
        mIavFd(fd),
        mDSPMode(dsp_mode),
        mDecType(dec_type),
        mTotalStreamNumber(1)
    {
        AM_ASSERT(fd >= 0);
        AM_ASSERT(p);
        AM_ASSERT(DSPMode_UDEC == dsp_mode);
        mpSharedRes = (SConsistentConfig*)p;
        DSetModuleLogConfig(LogModuleDSPHandler);
    }
    virtual ~CIOneUdecSimpleDecAPI() {}

private:
    AM_INT mIavFd;
    AM_U16 mDSPMode;
    AM_U16 mDecType;//driver use
    AM_U16 mTotalStreamNumber;
    AM_U16 reserved0;

private:
    SConsistentConfig* mpSharedRes;
};

class CIOneDuplexSimpleEncAPI: public CObject, public ISimpleEncAPI
{
public:
    virtual AM_ERR InitEncoder(AM_UINT& enc_id, SEncoderParam* param);
    virtual AM_ERR ReleaseEncoder(AM_UINT enc_id);

    virtual AM_ERR GetBitStreamBuffer(AM_UINT enc_id, SBitDescs* p_desc);
    virtual AM_ERR Start(AM_UINT enc_id);
    virtual AM_ERR Stop(AM_UINT enc_id, AM_UINT stop_flag);

public:
    CIOneDuplexSimpleEncAPI(AM_INT fd, AM_UINT dsp_mode, AM_UINT enc_type, void* p):
        mIavFd(fd),
        mDSPMode(dsp_mode),
        mMainStreamEnabled(1),
        mSecondStreamEnabled(0)
    {
        AM_ASSERT(fd >= 0);
        AM_ASSERT(DSPMode_DuplexLowdelay == dsp_mode);
        DSetModuleLogConfig(LogModuleDSPHandler);
    }

    virtual ~CIOneDuplexSimpleEncAPI() {}

private:
    AM_INT mIavFd;
    AM_U16 mDSPMode;
    //AM_U16 mEncType;//driver use

    AM_U8 mMainStreamEnabled;
    AM_U8 mSecondStreamEnabled;

    AM_U16 reserved0[2];
};

class CIOneRecordSimpleEncAPI: public CObject, public ISimpleEncAPI
{
public:
    virtual AM_ERR InitEncoder(AM_UINT& enc_id, SEncoderParam* param);
    virtual AM_ERR ReleaseEncoder(AM_UINT enc_id);

    virtual AM_ERR GetBitStreamBuffer(AM_UINT enc_id, SBitDescs* p_desc);
    virtual AM_ERR Start(AM_UINT enc_id);
    virtual AM_ERR Stop(AM_UINT enc_id, AM_UINT stop_flag);

public:
    CIOneRecordSimpleEncAPI(AM_INT fd, AM_UINT dsp_mode, AM_UINT enc_type, void* p):
        mIavFd(fd),
        mDSPMode(dsp_mode),
        //mEncType(enc_type),
        mTotalStreamNumber(1),
        mStreamMask(0x1),
        mEosFlag(0)
    {
        AM_ASSERT(fd >= 0);
        AM_ASSERT(DSPMode_CameraRecording == dsp_mode);
        DSetModuleLogConfig(LogModuleDSPHandler);
    }

    virtual ~CIOneRecordSimpleEncAPI() {}

private:
    AM_INT mIavFd;
    AM_U16 mDSPMode;
    //AM_U16 mEncType;//driver use
    AM_U16 mTotalStreamNumber;
    AM_U16 reserved0[2];

    AM_UINT mStreamMask;
    AM_UINT mEosFlag;
};

#endif
