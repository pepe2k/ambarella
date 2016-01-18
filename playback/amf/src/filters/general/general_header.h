/*
 * general_header.h
 *
 * History:
 *    2012/4/6 - [QingXiong Z] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __GENERAL_HEADER_HEADER_H__
#define __GENERAL_HEADER_HEADER_H__

#include "string.h"
#include "am_new.h"
#include "am_types.h"
#include "am_if.h"
#include "am_param.h"
#include "am_mw.h"
#include "osal.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#include "engine_guids.h"
#include "msgsys.h"
#include "audio_if.h"

#include "general_mw.h"

//AM_INFO
#undef AM_INFO
#undef AM_ERROR
#define AM_LOG(format, args...) \
    do{ \
        fprintf(stdout, format, ##args); \
    }while(0) \

#define AM_INFO(format, args...) AM_LOG("::: "format, ##args)
#define AM_ERROR(format, args...) AM_LOG("\n!!!(%s:%d) "format, __FILE__, __LINE__, ##args)

#define MDEC_SOURCE_MAX_NUM 64

#define GDE_GRE_AUDIO_BP_NUM 256

#define BSB_HOLD_SIZE_BEFORE_PLAY 32*1024

enum ENGINE_GLOBAL_FLAG
{
    NO_FILL_PES_SSP_HEADER = 0x00001,
    NO_AUTO_RESAMPLE_AUDIO = 0x00004,
    NO_AUDIO_ON_GMF = 0x00008,
    LOOP_FOR_LOCAL_FILE = 0x00002,
    USING_FOR_NET_PB = 0x00010,
    SAVE_ALL_STREAMS = 0x00020,
    ADD_MD_RECEIVER = 0x00040,
    HAVE_TRANSCODE = 0x00080,
    NO_HD_WIN_NVR_PB = 0x00100,//explain:set dsp to num small parts, no hd full windows, app need to protect not perform switch and soon cmd.
    NVR_ONLY_SHOW_ON_LCD = 0x00400, //show on lcd
    SWITCH_JUST_FOR_WIN_ARM = 0x01000,
    SWITCH_JUST_FOR_WIN_ARM2 = 0x02000,
    SWITCH_JUST_FOR_WIN_ARM3 = 0x04000,
    SWITCH_ARM_SEEK_FEED_DSP_DEBUG = 0x10000,
    SWITCH_ARM_SEEK_FEED_DSP_DEBUG2 = 0x20000,
    DEBUG_100MS_ON_07_23 = 0x100000,
    DEBUG_100MS_ON_07_23_2 = 0x200000,
    NO_QUERY_ANY_FROM_DSP = 0x400000,
    BUFFER_TO_IDR_OR_NOT_JUST_TEST_FOR_DEMUXER = 0x800000,
    MULTI_STREAMING_SETS = 0x1000000,
    NOTHING_DONOTHING = 0x2000000,
    NOTHING_NOFLUSH = 0x4000000,
};

//The Param explain for Engine::Construct
enum ENGINE_CONSTRUCT_PAR
{
    REQUEST_DSP_NUM,
    GLOBAL_FLAG,
    DSP_MAX_FRAME_NUM,
    NET_BUFFER_BEGIN_NUM,
    DSP_PRE_BUFFER_LEN,
    AUTO_SEPARATE_FILE_DURATION,
    AUTO_SEPARATE_FILE_MAXCOUNT,
    DISPLAY_LAYOUT,
    VOUT_MASK,
    DSP_TILEMODE,
    DECODER_CAP,
    CONSTRUCT_PAR_NUM,
};

enum SOURCE_FLAG
{
    SOURCE_ENABLE_AUDIO = 0x00001,
    SOURCE_ENABLE_VIDEO = 0x00010,
    SOURCE_FULL_HD = 0x00100,
    SOURCE_SAVE_INFO = 0x01000,
    //EDIT
    SOURCE_EDIT_REPLACE = 0x10000,
    SOURCE_EDIT_PLAY = 0x20000,
    SOURCE_EDIT_ADDBACKGROUND = 0x40000,
};

enum DUMP_FLAG
{
    DUMP_FLAG_FFMPEG = 0x000001,

};

enum SAVE_FLAG
{
    SAVE_INJECTOR_FLAG = 0x000008,
    SAVE_INJECTOR_RTSP_FLAG = 0x000010,
    SAVE_NAME_WITH_TIME_FLAG = 0x000007,
};

enum UPLOAD_NVR_TRANSOCDE_FLAG
{
    UPLOAD_NVR_INJECTOR_RTMP_FLAG = 0x000f00,
    UPLOAD_NVR_INJECTOR_RTSP_FLAG = 0x000f01,
    UPLOAD_NVR_HLS_FLAG = 0x000f02,
};
//=======================================================
//general config cmd
//
//=======================================================
enum GCONFIG_DECODER_CAP
{
    DECODER_CAP_DSP = 0x0001,
    DECODER_CAP_FFMPEG = 0x0002,
    DECODER_CAP_COREAVC = 0x0004,
};

enum GCONFIG_CMD
{
    DEMUXER_SPECIFY_GETBUFFER = 0x01000,
    RENDEER_SPECIFY_CONFIG = 0x00001,
    RENDERER_ENABLE_AUDIO = 0x00010,
    RENDERER_DISABLE_AUDIO = 0x00100, //discard
    RENDERER_DISCONNECT_HW = 0x10000,
};

enum RENDERER_SPECIFY_CMD
{
    REMOVE_RENDER_AOUT,
    ADD_RENDER_AOUT,//discard
    FLUSH_AUDIO_QUEUE,//discard
    PAUSE_RENDER_OUT,//discard
    RESUME_RENDER_OUT,//discard
    STEPPLAY_RENDER_OUT,
    CONFIG_WINDOW_RENDER,
    RENDER_SWITCH_HD,
    RENDER_SWITCH_BACK,
    RENDER_SWITCH_HD2,
    RENDER_SWITCH_BACK2,
};

typedef struct CDemuxerConfig
{
    AM_INT configIndex;
    AM_BOOL disableAudio;
    AM_BOOL disableVideo;
    AM_BOOL hasVideo;
    AM_BOOL hasAudio;
    AM_BOOL sendHandleV;
    AM_BOOL sendHandleA;
    AM_UINT pausedCnt;

    AM_BOOL paused;
    AM_BOOL hided;
    AM_BOOL needStart;
    AM_BOOL hd;
    AM_BOOL netused;
    AM_BOOL edited;
}CDemuxerConfig;

typedef struct CDecoderConfig
{
    //AM_BOOL isHdInstance; // which decoder is hd
    //AM_INT
    AM_INT dspInstance;
    AM_INT dspPpmod;
    AM_INT dspMode;
    AM_INT enDeinterlace;
    void* decoderEnv;
    AM_INT opaqueEnv;
    AM_BOOL onlyFeed;
}CDecoderConfig;

typedef struct CRendererConfig
{
    AM_INT configCmd;
    AM_BOOL videoReady;
    AM_BOOL audioReady;
}CRendererConfig;

typedef struct CDspGConfig
{
    AM_INT dspNumRequested;
    AM_INT eachDecBufferNum;
    AM_INT preBufferLen;
    AM_UINT enDspBufferCtrl;
    AM_INT iavHandle;
    IUDECHandler* udecHandler;
    AM_UINT addVideoDataType; //NOTE TO ASSIGEN
}CDspGConfig;

typedef struct CAudioConfig
{
    IAudioHAL* audioHandle;
    AM_BOOL isOpened;
}CAudioConfig;

typedef struct CMapTable
{
    AM_INT index;//same as demuxer index
    AM_S8 file[128];
    AM_BOOL isHd;
    AM_INT sourceGroup;
    AM_INT winIndex; //sourceGroup
    AM_INT decIndex;
    AM_INT renIndex;
    AM_INT dspIndex;
    AM_INT dsprenIndex;
}CMapTable;

//those is for win_cmd and ren_cmd
typedef struct CSubDspWinConfig
{
    AM_INT winIndex;//set by renderconfig
    AM_INT winOffsetX;
    AM_INT winOffsetY;
    AM_INT winWidth;
    AM_INT winHeight;
}CSubDspWinConfig;

typedef struct CSubDspRenConfig
{
    AM_INT renIndex; //use which one render(change this)
    AM_INT winIndex;
    AM_INT dspIndex;
    AM_INT winIndex2;
}CSubDspRenConfig;

typedef struct CDspRenConfig
{
    AM_BOOL renChanged;
    AM_INT renNumConfiged;
    AM_INT renNumNeedConfig;
    CSubDspRenConfig renConfig[MDEC_SOURCE_MAX_NUM];
}CDspRenConfig;

typedef struct CDspWinConfig
{
    AM_BOOL winChanged;
    AM_INT winNumConfiged;
    AM_INT winNumNeedConfig;
    CSubDspWinConfig winConfig[MDEC_SOURCE_MAX_NUM];
}CDspWinConfig;
//end win_cmd

class CGMuxerConfig;
class CGeneralAudioManager;
class CGConfig
{
public:
    //for ucode render/window - -
    AM_ERR SetDefaultWinRenMap(AM_INT index, AM_INT dspInstance);
    AM_ERR DemuxerSetDecoderEnv(AM_INT index, void* env, CParam& par);
    AM_INT GetAudioSource();
    void DisableAllAudio();
    AM_BOOL AllRenderOutReady();

    //create index info, thisis sorted by your input,
    AM_ERR InitMapTable(AM_INT index, const char* pFile, AM_INT sourceGroup);
    AM_ERR DeInitMap(AM_INT index);
    AM_ERR SetMapDec(AM_INT index, AM_INT dec);
    AM_ERR SetMapDsp(AM_INT index, AM_INT dsp);
    AM_ERR SetMapRen(AM_INT index, AM_INT ren);

    AM_ERR SetIavHandle(){ return ME_OK;}
    CGConfig();
    ~CGConfig();
public:
    AM_INT curHdIndex;
    AM_INT sourceNum;
    AM_INT curIndex;
    AM_INT version;
    //This too improve our flexialbe.
    AM_INT generalCmd;
    AM_INT specifyIndex;

    AM_INT globalFlag;

    //TODO
    void* oldCompatibility;
    void* engineptr;
    CAudioConfig audioConfig;
    CDspGConfig dspConfig;
    CDemuxerConfig demuxerConfig[MDEC_SOURCE_MAX_NUM];
    CDecoderConfig decoderConfig[MDEC_SOURCE_MAX_NUM];
    CRendererConfig rendererConfig[MDEC_SOURCE_MAX_NUM];

    //For Win&Ren
    CDspRenConfig dspRenConfig;
    CDspWinConfig dspWinConfig;
    //Table for map
    CMapTable indexTable[MDEC_SOURCE_MAX_NUM];

    //for muxer
    CGMuxerConfig* mainMuxer;
    //for audio
    CGeneralAudioManager* audioManager;
    CQueue* audioMainQ;

    //for display layout
    AM_U8 mInitialLayout;
    AM_U8 mbUseCustomizedDisplayLayout;
    AM_U8 mCustomizedDisplayLayout;
    AM_U8 mReserved1;
    AM_U8 mReserved2;
    AM_INT mDecoderCap;
};
//----------------------------------------------------
//
//----------------------------------------------------
enum BUFFER_FLAG
{
    HD_STREAM_HANDLE = 77,
};

enum OWNER_TYPE
{
    DEMUXER_FFMPEG,
    DECODER_FFMEPG,
    DECODER_DSP,
    TRANSCODER_DSP,
};

enum BUFFER_TYPE
{
    DATA_BUFFER,
    EOS_BUFFER,
    HANDLE_BUFFER,
    NOINITED_BUFFER,
};

enum STREAM_TYPE
{
    STREAM_NULL = -1,
    STREAM_AUDIO = 0,
    STREAM_VIDEO,
    STREAM_TYPE_NUM,
};

class CGBuffer : public CBuffer
{
public:
    struct AUDIO_INFO
    {
        AM_INT sampleSize;
        AM_INT sampleRate;
        AM_INT numChannels;
        AM_INT sampleFormat;
    };
public:
    void SetOwnerType(OWNER_TYPE o){ owner = o;}
    OWNER_TYPE GetOwnerType() const{return owner;}
    void SetBufferType(BUFFER_TYPE b){ bufferType = b;}
    BUFFER_TYPE GetBufferType() const{return bufferType;}
    void SetStreamType(STREAM_TYPE s){ streamType = s;}
    STREAM_TYPE GetStreamType() const{return streamType;}
    void SetIdentity(AM_INT i){ identity = i;}
    AM_INT GetIdentity() const{return identity;}
    void SetCount(AM_UINT i){ count = i;}
    AM_UINT GetCount() const{return count;}
    void SetGBufferPtr(AM_INTPTR ptr) { gbufferPtr = ptr; }
    AM_INTPTR GetGBufferPtr() const { return gbufferPtr; }
    void Clear(){ bufferType = NOINITED_BUFFER;}

    AM_ERR Retrieve();
    void Dump(const char* str);

    AM_ERR ReleaseContent();
    AM_ERR ReleaseFFMpegDec();
    AM_U8* PureDataPtr();
    AM_UINT PureDataSize();

    CGBuffer();
    ~CGBuffer(){}
public:
    const AM_GUID* codecType;
    AM_INT ffmpegStream;
public:
    AUDIO_INFO audioInfo;
private:
    AM_INT index; //index on demuxer side. same as decoder.

    AM_INT identity;
    OWNER_TYPE owner;
    BUFFER_TYPE bufferType;
    STREAM_TYPE streamType;
    AM_UINT count;

    //more ptr
    AM_INTPTR gbufferPtr;
};

//========================================================//
//  Muxer
//========================================================//
class CGeneralMuxer;
enum MuxerType
{
    MUXER_SIMPLE_ES,
    MUXER_FFMPEG_MULTI,
};
enum InjectorType
{
    INJECTOR_RTMP,
    INJECTOR_RTSP,
};

typedef struct CVideoMuxerInfo
{
    AM_INT width;
    AM_INT height;
    AM_INT bitrate;
    AM_INT codec;
    AM_INT extrasize;
    AM_U8 extradata[1024];
}CVideoMuxerInfo;

typedef struct CAudioMuxerInfo
{
    AM_INT channels;
    AM_INT samplerate;
    AM_INT bitrate;
    AM_INT samplefmt;
    AM_INT codec;
}CAudioMuxerInfo;

class CUintMuxerConfig
{
public:
    CUintMuxerConfig();
    ~CUintMuxerConfig();

public:
    AM_BOOL saveMe;
    AM_BOOL sendMe;
    AM_BOOL configVideo;
    AM_BOOL configAudio;

    AM_INT index;
    AM_BOOL nameWithTime;
    char fileName[128];
    char rtmpUrl[4096];

    MuxerType fileType;
    InjectorType injectorType;
    CVideoMuxerInfo videoInfo;
    CAudioMuxerInfo audioInfo;

    AM_INT framerate_den;
    AM_INT framerate_num;

    AM_INT ds_type_live;
    pthread_mutex_t mutex_;
};

class CGMuxerConfig
{
public:
    AM_ERR CheckAction(AM_INT index, AM_BOOL isSave, AM_INT flag);

    void SetGeneralMuxer(CGeneralMuxer* muxer);
    AM_ERR SetupMuxerInfo(AM_INT index, AM_BOOL isVideo, CParam& par);

    AM_ERR InitUnitConfig(AM_INT index, const char* saveName, AM_INT flag);
    AM_ERR DeInitUnitConfig(AM_INT index, AM_INT flag);
    MuxerType ParseFileType(char* file);

    CGMuxerConfig():
        generalMuxer(NULL)
     {}
public:
    CGeneralMuxer* generalMuxer;
    CUintMuxerConfig unitConfig[MDEC_SOURCE_MAX_NUM];
};
//========================================================//
//  Layout related
//========================================================//
typedef enum
{
    LAYOUT_TYPE_TABLE,
    LAYOUT_TYPE_TELECONFERENCE,
    LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN,
    LAYOUT_TYPE_SINGLE_HD_FULLSCREEN,
    LAYOUT_TYPE_RECOVER,
    LAYOUT_CNT
}LAYOUT_TYPE;

typedef enum
{
    LAYOUT_TYPE_PRE_TABLE = LAYOUT_CNT+1,
    LAYOUT_TYPE_PRE_TELECONFERENCE,
}LAYOUT_PRE_TYPE;

typedef enum
{
    ACTION_SWITCH,
    ACTION_BACK,
    ACTION_PRE_SWITCH,

}ACTION_TYPE;

typedef enum
{
    LAYOUT_TELE_HIDE_SMALL = 0x7fffff00,
    LAYOUT_TELE_SHOW_SMALL = 0x7fffff01
}TELE_SMALL_TYPE;
#endif
