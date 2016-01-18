/**
 * am_dsp_if.h
 *
 * History:
 *  2011/05/31 - [Zhi He] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_DSP_IF_H__
#define __AM_DSP_IF_H__

enum {
    eInvalidUdecIndex = -1,
};

typedef struct {
    AM_U16 x,y,w,h;
} SRect;

typedef struct {
    AM_INT mode;
    AM_U32 width;
    AM_U32 height;
} SVoutModeDef;

enum {
    DEC_SPEED_1X = 0,
    DEC_SPEED_2X,
    DEC_SPEED_4X,
    DEC_SPEED_1BY2,
    DEC_SPEED_1BY4,
};

enum {
    TRICKPLAY_PAUSE = 0,
    TRICKPLAY_RESUME,
    TRICKPLAY_STEP,
};

struct DSPConfig;
class CVideoBuffer;
struct DSPVoutConfigs;
class CGConfig;
//instance_index based udec
class IUDECHandler: public IInterface
{

public:
    //dsp mode switch, usr can ignore this interface, if upper mw code need manage the two udec instance by itself, it can call these function either
    virtual AM_ERR EnterUDECMode() = 0;
    virtual AM_ERR ExitUDECMode() = 0;

    //udec instance init/exit
    virtual AM_ERR InitUDECInstance(AM_INT& wantUdecIndex, DSPConfig*pConfig, void* pHybirdAcc, AM_INT decFormat, AM_INT pic_width, AM_INT pic_height, AM_INT extEdge, bool autoAllocIndex = true) = 0;
    virtual AM_ERR StartUdec(AM_INT udecIndex, AM_UINT format, void* pacc) =0;
    virtual AM_ERR ReleaseUDECInstance(AM_INT udecIndex) = 0;
    virtual AM_ERR SetUdecNums(AM_INT udecNums) = 0;

    //request input buffer (AllocBSB for hw mode)
    virtual AM_ERR RequestInputBuffer(AM_INT udecIndex, AM_UINT& size, AM_U8* pStart, AM_INT bufferCnt) = 0;
    virtual AM_ERR DecodeBitStream(AM_INT udecIndex, AM_U8*pStart, AM_U8*pEnd) = 0;
    virtual AM_ERR AccelDecoding(AM_INT udecIndex, void* pParam, bool needSync = false) = 0;

    //stop, flush, clear, trick play
    virtual AM_ERR StopUDEC(AM_INT udecIndex) = 0;
    virtual AM_ERR FlushUDEC(AM_INT udecIndex, AM_BOOL lastPic = AM_FALSE) = 0;
    virtual AM_ERR ClearUDEC(AM_INT udecIndex) = 0;
    virtual AM_ERR PauseUDEC(AM_INT udecIndex) = 0;
    virtual AM_ERR ResumeUDEC(AM_INT udecIndex) = 0;
    virtual AM_ERR StepPlay(AM_INT udecIndex) = 0;
    virtual AM_ERR TrickMode(AM_INT udecIndex, AM_UINT speed, AM_UINT backward) = 0;

    //frame buffer pool
    virtual AM_UINT InitFrameBufferPool(AM_INT udecIndex, AM_UINT picWidth, AM_UINT picHeight, AM_UINT chromaFormat, AM_UINT tileFormat, AM_UINT hasEdge) = 0;
    virtual AM_ERR ResetFrameBufferPool(AM_INT udecIndex) = 0;

    //buffer id/data pointer in CVideoBuffer
    virtual AM_ERR RequestFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer) = 0;
    virtual AM_ERR RenderFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer, SRect* srcRect) = 0;
    virtual AM_ERR ReleaseFrameBuffer(AM_INT udecIndex, CVideoBuffer*& pBuffer) = 0;

    //avsync
    virtual AM_ERR SetClockOffset(AM_INT udecIndex, AM_S32 diff) = 0;
    virtual AM_ERR WakeVout(AM_INT udecIndex, AM_UINT voutMask) = 0;
    virtual AM_ERR GetUdecTimeEos(AM_INT udecIndex, AM_UINT& eos, AM_U64& udecTime, AM_INT nowait) = 0;

    //query UDEC state
    virtual AM_ERR GetUDECState(AM_INT udecIndex, AM_UINT& udecState, AM_UINT& voutState, AM_UINT& errorCode) = 0;
    //get decoded frame
    virtual AM_ERR GetDecodedFrame(AM_INT udecIndex, AM_INT& buffer_id, AM_INT& eos_invalid) = 0;

    //nvr
    virtual AM_ERR InitWindowRender(CGConfig* pConfig) = 0;
    virtual AM_ERR ConfigWindowRender(CGConfig* pConfig) = 0;
    virtual AM_ERR SwitchStream(AM_INT renIndex, AM_INT dspIndex, AM_INT flag) = 0;
    //virtual AM_ERR SwitchSpecifyToHD(AM_INT index, AM_INT flag) = 0;
    //virtual AM_ERR SwitchHDToSpecify(AM_INT index, AM_INT flag) = 0;
    virtual AM_ERR GetUDECState2(AM_INT udecIndex, AM_U8** dspRead ) = 0;
    virtual AM_ERR DspBufferControl(AM_INT udecIndex, AM_UINT control, AM_UINT control_time) = 0;
    //nvr transcode
    virtual AM_ERR InitMdecTranscoder(AM_INT width, AM_INT height, AM_INT bitrate) = 0;
    virtual AM_ERR ReleaseMdecTranscoder() = 0;
    virtual AM_ERR StartMdecTranscoder() = 0;
    virtual AM_ERR StopMdecTranscoder() = 0;
    virtual AM_ERR MdecTranscoderReadBits(void* bitInfo) = 0;
    virtual AM_ERR MdecTranscoderSetBitrate(AM_INT kbps) = 0;
    virtual AM_ERR MdecTranscoderSetFramerate(AM_INT fps, AM_INT reduction) = 0;
    virtual AM_ERR MdecTranscoderSetGOP(AM_INT M, AM_INT N, AM_INT interval, AM_INT structure) = 0;
    virtual AM_ERR MdecTranscoderDemandIDR(AM_BOOL now) = 0;

    //zoom mode 2:
    virtual AM_ERR PlaybackZoom(AM_INT renderIndex, AM_U16 input_win_width, AM_U16 input_win_height, AM_U16 center_x, AM_U16 center_y) = 0;
};

class IVoutHandler: public IInterface
{
//initialize/config vout
public:
    virtual AM_ERR SetupVOUT(AM_INT type, AM_INT mode) = 0;

//update vout settings
public:
    //obsolete, remove later
    //virtual AM_ERR GetCurrentVoutSettings(DSPConfig*pConfig, AM_UINT requestedVoutMask) = 0;

    virtual AM_ERR Enable(AM_UINT voutID, AM_INT enable) = 0;
    virtual AM_ERR EnableOSD(AM_UINT voutID, AM_INT enable) = 0;

    //direct config vout
    //obsolete
    //virtual DSPVoutConfigs* DirectVoutConfig() = 0;
    //obsolete
    //virtual AM_ERR UpdateVoutSettings() = 0;

    virtual AM_ERR GetSizePosition(AM_UINT voutID, AM_INT* pos_x, AM_INT* pos_y, AM_INT* size_x, AM_INT* size_y) = 0;
    virtual AM_ERR GetDimension(AM_UINT voutID, AM_INT* size_x, AM_INT* size_y) = 0;
    virtual AM_ERR GetPirtureSizeOffset(AM_INT* pic_width, AM_INT* pic_height, AM_INT* offset_x, AM_INT* offset_y) = 0;
    virtual AM_ERR GetSoureRect(AM_INT* pos_x, AM_INT* pos_y, AM_INT* size_x, AM_INT* size_y) = 0;

    //todo: mass apis about change size/position/flip/rotate/mirror/source rect
    virtual AM_ERR UpdatePirtureSizeOffset(AM_INT pic_width, AM_INT pic_height, AM_INT offset_x, AM_INT offset_y) = 0;
    virtual AM_ERR ChangeSizePosition(AM_UINT voutID, AM_INT pos_x, AM_INT pos_y, AM_INT size_x, AM_INT size_y) = 0;
    virtual AM_ERR ChangeSize(AM_UINT voutID, AM_INT size_x, AM_INT size_y) = 0;
    virtual AM_ERR ChangePosition(AM_UINT voutID, AM_INT pos_x, AM_INT pos_y) = 0;
    virtual AM_ERR ChangeSourceRect(AM_INT pos_x, AM_INT pos_y, AM_INT size_x, AM_INT size_y) = 0;
    virtual AM_ERR ChangeInputCenter(AM_INT pos_x, AM_INT pos_y) = 0;
    virtual AM_ERR ChangeRoomFactor(AM_UINT voutID, float factor_x, float factor_y) = 0;
    virtual AM_ERR Flip(AM_UINT voutID, AM_INT param) = 0;
    virtual AM_ERR Rotate(AM_UINT voutID, AM_INT param) = 0;
    virtual AM_ERR Mirror(AM_UINT voutID, AM_INT param) = 0;
};

AM_ERR AM_CreateDSPHandler(void* pShared);
AM_ERR AM_DeleteDSPHandler(void* pShared);


//simple API wrapper from driver level

enum {
    DSPVersion_Invalid = 0,
    DSPVersion_A5S,
    DSPVersion_IOne,
};

typedef struct {
    AM_UINT codec_type;
    AM_UINT dsp_mode, dsp_version;
    AM_UINT pic_width, pic_height;
    AM_UINT pic_width_aligned, pic_height_aligned;

    //out
    AM_U8* bits_fifo_start;
    AM_UINT bits_fifo_size;
} SDecoderParam;

typedef struct {
    AM_UINT codec_type;
    AM_UINT dsp_mode, dsp_version;
    AM_UINT main_width, main_height;
    AM_UINT enc_width, enc_height;
    AM_UINT enc_offset_x, enc_offset_y;

    AM_U8 profile, level;
    AM_U8 M, N;
    AM_U8 gop_structure;
    AM_U8 numRef_P;
    AM_U8 numRef_B;
    AM_U8 use_cabac;
    AM_UINT idr_interval;
    AM_UINT bitrate;
    AM_UINT framerate_num, framerate_den;

    AM_U16 quality_level;
    AM_U8 vbr_setting;
    AM_U8 calibration;
    AM_U8 vbr_ness;
    AM_U8 min_vbr_rate_factor;
    AM_U8 max_vbr_rate_factor;

    AM_U8 second_stream_enabled;
    AM_U8 dsp_piv_enabled;
    AM_U8 reserved1[2];
    AM_UINT second_bitrate;
    AM_UINT second_enc_width, second_enc_height;
    AM_UINT second_enc_offset_x, second_enc_offset_y;

    AM_U16 dsp_jpeg_active_win_w, dsp_jpeg_active_win_h;
    AM_U16 dsp_jpeg_dram_w, dsp_jpeg_dram_h;

    //out
    AM_U8* bits_fifo_start;
    AM_UINT bits_fifo_size;
} SEncoderParam;

typedef struct {
    //AM_UINT codec_type;
    //AM_UINT dsp_mode, dsp_version;
    //AM_UINT main_width, main_height;
    AM_UINT enc_width, enc_height;
    //AM_UINT enc_offset_x, enc_offset_y;
    //AM_UINT second_stream_enabled;
    AM_UINT second_enc_width, second_enc_height;
    //AM_UINT second_enc_offset_x, second_enc_offset_y;
} SPrepareEncodingParam;

typedef struct {
    AM_U8* pstart;
    AM_UINT size;
    AM_U64 pts;

    AM_U8 pic_type;
    //need here?
    AM_U8 enc_id;
    AM_U16 fb_id;

    AM_UINT frame_number;
} SBitDesc;

//same with driver
#define DMaxDescNumber 4
#define INVALID_FD_ID 0xffff
#define DFlagLastFrame (1<<0)
#define DFlagNeedReleaseFrame (1<<1)
typedef struct {
    AM_U16 tot_desc_number;
    AM_U16 status;
    SBitDesc desc[DMaxDescNumber];
} SBitDescs;

typedef struct {
    AM_INT enable;//video layer
    AM_INT osd_disable;//osd layer
    AM_INT sink_id;
    AM_INT width, height;//the size of device
    AM_INT pos_x, pos_y;
    AM_INT size_x, size_y;//target_win_size
    AM_INT video_x, video_y;
    AM_U32 zoom_factor_x, zoom_factor_y;
    AM_INT input_center_x, input_center_y;
    AM_INT flip, rotate;
    AM_INT failed;//checking/opening failed
    AM_INT vout_mode;// 1: interlace_mode

    AM_INT vout_id;//debug use
} SVoutConfig;

class ISimpleDecAPI
{
public:
    virtual AM_ERR InitDecoder(AM_UINT& dec_id, SDecoderParam* param, AM_UINT number_vout, SVoutConfig* vout_config) = 0;
    virtual AM_ERR ReleaseDecoder(AM_UINT dec_id) = 0;

    virtual AM_ERR RequestBitStreamBuffer(AM_UINT dec_id, AM_U8* pstart, AM_UINT room) = 0;
    virtual AM_ERR Decode(AM_UINT dec_id, AM_U8* pstart, AM_U8* pend) = 0;
    virtual AM_ERR Stop(AM_UINT dec_id, AM_UINT stop_flag) = 0;
    virtual AM_ERR TrickPlay(AM_UINT dec_id, AM_UINT trickplay) = 0;

    virtual ~ISimpleDecAPI() {}
};

class ISimpleEncAPI
{
public:
    virtual AM_ERR InitEncoder(AM_UINT& enc_id, SEncoderParam* param) = 0;
    virtual AM_ERR ReleaseEncoder(AM_UINT enc_id) = 0;

    virtual AM_ERR GetBitStreamBuffer(AM_UINT enc_id, SBitDescs* p_desc) = 0;
    virtual AM_ERR Start(AM_UINT enc_id) = 0;
    virtual AM_ERR Stop(AM_UINT enc_id, AM_UINT stop_flag) = 0;

    virtual ~ISimpleEncAPI() {}
};

//simple API entry
extern ISimpleDecAPI* CreateSimpleDecAPI(AM_INT iav_fd, AM_UINT dsp_mode, AM_UINT codec_type, void* p);
extern ISimpleEncAPI* CreateSimpleEncAPI(AM_INT iav_fd, AM_UINT dsp_mode, AM_UINT codec_type, void* p);
extern void DestroySimpleDecAPI(ISimpleDecAPI* p);
extern void DestroySimpleEncAPI(ISimpleEncAPI* p);

#endif

