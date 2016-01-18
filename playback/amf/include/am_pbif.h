
/**
 * am_pbif.h
 *
 * History:
 *    2009/12/16 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_PBIF_H__
#define __AM_PBIF_H__

//#include <media/MediaPlayerInterface.h>
//using namespace android;

extern const AM_IID IID_IPBControl;

//-----------------------------------------------------------------------
//
// IPBControl
//
//-----------------------------------------------------------------------
class IPBControl: public IMediaControl
{
public:
    enum {
        DEC_PROPERTY_SPEEDUP_REALTIME_PLAYBACK,

//debug use
        DEC_DEBUG_PROPERTY_DISCARD_HALF_AUDIO_PACKET,
        DEC_DEBUG_PROPERTY_COLOR_TEST,//fill ycbcr into frame buffer for test
        DSP_TILE_MODE,
        DSP_PREFETCH_COUNT,
        DSP_BITS_FIFO_SIZE,
        DSP_REF_CACHE_SIZE,
        DEC_DEBUG_PROPERTY_AUTO_VOUT
    };

    typedef enum {
        DIR_FORWARD,
        DIR_BACKWORD,
    } PB_DIR;

    typedef enum {
        SPEED_NORMAL,
        FAST_2x,
        FAST_4x,
        FAST_8x,
        FAST_16x,
        SLOW_2x,
        SLOW_4x,
        SLOW_8x,
        SLOW_16x,
    } PB_SPEED;

    typedef enum {
        STATE_IDLE,
        STATE_INITIALIZED,
        STATE_PREPARED,
        STATE_STARTED,
        STATE_PAUSED,
        STATE_STOPPED,
        STATE_COMPLETED,
        STATE_END,
        STATE_STEP,
        STATE_ERROR,
    } PB_STATE;

    struct PBINFO {
        PB_STATE	state;
        PB_DIR		dir;
        PB_SPEED	speed;
        AM_U64		length;
        AM_U64		position;
    };

public:
    DECLARE_INTERFACE(IPBControl, IID_IPBControl);
    virtual AM_ERR SetWorkingMode(AM_UINT dspMode, AM_UINT voutMask) = 0;

    virtual AM_ERR PlayFile(const char *pFileName) = 0;
    virtual AM_ERR PrepareFile(const char *pFileName) = 0;
    virtual AM_ERR StopPlay() = 0;
    virtual AM_ERR GetPBInfo(PBINFO& info) = 0;

    virtual AM_ERR PausePlay() = 0;
    virtual AM_ERR ResumePlay() = 0;
    virtual AM_ERR Loopplay(AM_U32 mode) = 0;

    virtual AM_ERR Seek(AM_U64 pts) = 0;

    //debug api
    virtual AM_ERR Step() = 0;
    virtual AM_ERR StartwithStepMode(AM_UINT startCnt) = 0;
    virtual AM_ERR StartwithPlayVideoESMode(AM_INT also_demuxfile, AM_INT param_0, AM_INT param_1) = 0;
    virtual AM_ERR SpecifyVideoFrameRate(AM_UINT framerate_num, AM_UINT framerate_den) = 0;//hardware format and hybird mpeg4 can use this

    virtual AM_ERR SetTrickMode(PB_DIR dir, PB_SPEED speed) = 0;
    virtual AM_ERR SetLooping(int loop) = 0;

    virtual AM_ERR SetDeinterlaceMode(AM_INT bEnable) = 0;
    virtual AM_ERR GetDeinterlaceMode(AM_INT *pbEnable) = 0;

    //vout config
    virtual AM_ERR ChangeInputCenter(AM_INT input_center_x, AM_INT input_center_y) = 0;
    virtual AM_ERR SetDisplayPositionSize(AM_INT vout, AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height) = 0;
    virtual AM_ERR GetDisplayPositionSize(AM_INT vout, AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height) = 0;
    virtual AM_ERR SetDisplayPosition(AM_INT vout, AM_INT pos_x, AM_INT pos_y) = 0;
    virtual AM_ERR SetDisplaySize(AM_INT vout, AM_INT width, AM_INT height) = 0;
    virtual AM_ERR GetDisplayDimension(AM_INT vout, AM_INT* width, AM_INT* height) = 0;
    virtual AM_ERR GetVideoPictureSize(AM_INT* width, AM_INT* height) = 0;
    virtual AM_ERR GetCurrentVideoPictureSize(AM_INT* pos_x, AM_INT* pos_y, AM_INT* width, AM_INT* height) = 0;
    virtual AM_ERR VideoDisplayZoom(AM_INT pos_x, AM_INT pos_y, AM_INT width, AM_INT height) = 0;
    virtual AM_ERR SetDisplayFlip(AM_INT vout, AM_INT flip) = 0;
    virtual AM_ERR SetDisplayMirror(AM_INT vout, AM_INT mirror) = 0;
    virtual AM_ERR SetDisplayRotation(AM_INT vout, AM_INT degree) = 0;
    virtual AM_ERR EnableVout(AM_INT vout, AM_INT enable) = 0;
    virtual AM_ERR EnableOSD(AM_INT vout, AM_INT enable) = 0;
    virtual AM_ERR EnableVoutAAR(AM_INT enable) = 0;
    //source/dest rect and scale mode config
    virtual AM_ERR SetVideoSourceRect(AM_INT x, AM_INT y, AM_INT w, AM_INT h) = 0;
    virtual AM_ERR SetVideoDestRect(AM_INT vout_id, AM_INT x, AM_INT y, AM_INT w, AM_INT h) = 0;
    virtual AM_ERR SetVideoScaleMode(AM_INT vout_id, AM_UINT mode_x, AM_UINT mode_y) = 0;

    virtual AM_ERR EnableDewarpFeature(AM_UINT enable, AM_UINT dewarp_param) = 0;
    virtual AM_ERR SetDeWarpControlWidth(AM_UINT enable, AM_UINT width_top, AM_UINT width_bottom) = 0;

//debug only
#ifdef AM_DEBUG
    virtual AM_ERR SetLogConfig(AM_UINT index, AM_UINT level, AM_UINT option) =0;
    virtual void PrintState() = 0;//print current pb-engine's(include filters) states, for debug
#endif
    /*A5S DV Playback Test */
    virtual AM_ERR  A5SPlayMode(AM_UINT mode) = 0;
    virtual AM_ERR  A5SPlayNM(AM_INT start_n, AM_INT end_m) = 0;
    /*end A5S DV Test*/

    //generic function
    virtual AM_ERR SetPBProperty(AM_UINT prop, AM_UINT value) = 0;

};


extern IPBControl* CreatePBControl(void *audiosink);
extern IPBControl* CreateActivePBControl(void *audiosink);

#endif

